#include "Python.h"

#include "pycore_llist.h"
#include "pycore_lock.h"        // _PyRawMutex
#include "pycore_parking_lot.h"
#include "pycore_pyerrors.h"    // _Py_FatalErrorFormat
#include "pycore_pystate.h"     // _PyThreadState_GET

#ifdef _WIN32
#   include <windows.h>
#elif (defined(_POSIX_SEMAPHORES) && !defined(HAVE_BROKEN_POSIX_SEMAPHORES) && \
     defined(HAVE_SEM_TIMEDWAIT))
#   define USE_SEMAPHORES
#   include <semaphore.h>
#elif defined(HAVE_PTHREAD_H)
#   include <pthread.h>
#elif defined(HAVE_PTHREAD_STUBS)
#   include "cpython/pthread_stubs.h"
#else
#   error "Require native threads. See https://bugs.python.org/issue31370"
#endif

// A simple, cross-platform binary semaphore that can be used to implement
// wakeup/sleep.
struct _PySemaphore {
#if defined(_WIN32)
    HANDLE platform_sem;
#elif defined(USE_SEMAPHORES)
    sem_t platform_sem;
#else
    PyMUTEX_T mutex;
    PyCOND_T cond;
    int counter;
#endif
};

typedef struct {
    // The mutex protects the waiter queue and the num_waiters counter.
    _PyRawMutex mutex;

    // Linked list of `struct wait_entry` waiters in this bucket.
    struct llist_node root;
    size_t num_waiters;
} Bucket;

struct wait_entry {
    struct _PyUnpark unpark;
    uintptr_t addr;
    _PySemaphore *sema;
    struct llist_node node;
};

#define MAX_SEMA_DEPTH 3

typedef struct {
    Py_ssize_t refcount;

    int depth;
    _PySemaphore semas[MAX_SEMA_DEPTH];
} ThreadData;

#define NUM_BUCKETS 251

// Table of waiters (hashed by address)
static Bucket buckets[NUM_BUCKETS];

#ifdef HAVE_THREAD_LOCAL
static _Py_thread_local ThreadData *thread_data = NULL;
#else
#error "no supported thread-local variable storage classifier"
#endif

static void
_PySemaphore_Init(_PySemaphore *sema)
{
#if defined(_WIN32)
    sema->platform_sem = CreateSemaphore(
        NULL,   //  attributes
        0,      //  initial count
        10,     //  maximum count
        NULL    //  unnamed
    );
    if (!sema->platform_sem) {
        Py_FatalError("parking_lot: CreateSemaphore failed");
    }
#elif defined(USE_SEMAPHORES)
    if (sem_init(&sema->platform_sem, /*pshared=*/0, /*value=*/0) < 0) {
        Py_FatalError("parking_lot: sem_init failed");
    }
#else
    if (pthread_mutex_init(&sema->mutex, NULL) != 0) {
        Py_FatalError("parking_lot: pthread_mutex_init failed");
    }
    if (pthread_cond_init(&sema->cond, NULL)) {
        Py_FatalError("parking_lot: pthread_cond_init failed");
    }
#endif
}

static void
_PySemaphore_Destroy(_PySemaphore *sema)
{
#if defined(_WIN32)
    CloseHandle(sema->platform_sem);
#elif defined(USE_SEMAPHORES)
    sem_destroy(&sema->platform_sem);
#else
    pthread_mutex_destroy(&sema->mutex);
    pthread_cond_destroy(&sema->cond);
#endif
}

static int
_PySemaphore_PlatformWait(_PySemaphore *sema, _PyTime_t timeout)
{
    int res = Py_PARK_INTR;
#if defined(_WIN32)
    DWORD wait;
    DWORD millis = 0;
    if (timeout < 0) {
        millis = INFINITE;
    }
    else {
        millis = (DWORD) (timeout / 1000000);
    }
    wait = WaitForSingleObjectEx(sema->platform_sem, millis, FALSE);
    if (wait == WAIT_OBJECT_0) {
        res = Py_PARK_OK;
    }
    else if (wait == WAIT_TIMEOUT) {
        res = Py_PARK_TIMEOUT;
    }
#elif defined(USE_SEMAPHORES)
    int err;
    if (timeout >= 0) {
        struct timespec ts;

        _PyTime_t deadline = _PyTime_Add(_PyTime_GetSystemClock(), timeout);
        _PyTime_AsTimespec(deadline, &ts);

        err = sem_timedwait(&sema->platform_sem, &ts);
    }
    else {
        err = sem_wait(&sema->platform_sem);
    }
    if (err == -1) {
        err = errno;
        if (err == EINTR) {
            res = Py_PARK_INTR;
        }
        else if (err == ETIMEDOUT) {
            res = Py_PARK_TIMEOUT;
        }
        else {
            _Py_FatalErrorFormat(__func__,
                "unexpected error from semaphore: %d",
                err);
        }
    }
    else {
        res = Py_PARK_OK;
    }
#else
    pthread_mutex_lock(&sema->mutex);
    if (sema->counter == 0) {
        int err;
        if (timeout >= 0) {
            struct timespec ts;

            _PyTime_t deadline = _PyTime_Add(_PyTime_GetSystemClock(), timeout);
            _PyTime_AsTimespec(deadline, &ts);

            err = pthread_cond_timedwait(&sema->cond, &sema->mutex, &ts);
        }
        else {
            err = pthread_cond_wait(&sema->cond, &sema->mutex);
        }
        if (err) {
            res = Py_PARK_TIMEOUT;
        }
    }
    if (sema->counter > 0) {
        sema->counter--;
        res = Py_PARK_OK;
    }
    pthread_mutex_unlock(&sema->mutex);
#endif
    return res;
}

int
_PySemaphore_Wait(_PySemaphore *sema, _PyTime_t timeout, int detach)
{
    PyThreadState *tstate = _PyThreadState_GET();
    int was_attached = 0;
    if (tstate) {
        was_attached = (tstate->_status.active);
        if (was_attached && detach) {
            PyEval_ReleaseThread(tstate);
        }
    }

    int res = _PySemaphore_PlatformWait(sema, timeout);

    if (tstate) {
        if (was_attached && detach) {
            PyEval_AcquireThread(tstate);
        }
    }
    return res;
}

void
_PySemaphore_Wakeup(_PySemaphore *sema)
{
#if defined(_WIN32)
    if (!ReleaseSemaphore(sema->platform_sem, 1, NULL)) {
        Py_FatalError("parking_lot: ReleaseSemaphore failed");
    }
#elif defined(USE_SEMAPHORES)
    int err = sem_post(&sema->platform_sem);
    if (err != 0) {
        Py_FatalError("parking_lot: sem_post failed");
    }
#else
    pthread_mutex_lock(&sema->mutex);
    sema->counter++;
    pthread_cond_signal(&sema->cond);
    pthread_mutex_unlock(&sema->mutex);
#endif
}

_PySemaphore *
_PySemaphore_Alloc(void)
{
    // Make sure we have a valid thread_data. We need to acquire
    // some locks before we have a fully initialized PyThreadState.
    _PyParkingLot_InitThread();

    ThreadData *this_thread = thread_data;
    if (this_thread->depth >= MAX_SEMA_DEPTH) {
        Py_FatalError("_PySemaphore_Alloc(): too many calls");
    }
    return &this_thread->semas[this_thread->depth++];
}

void
_PySemaphore_Free(_PySemaphore *sema)
{
    ThreadData *this_thread = thread_data;
    this_thread->depth--;
    if (&this_thread->semas[this_thread->depth] != sema) {
        Py_FatalError("_PySemaphore_Free(): mismatch wakeup");
    }
    _PyParkingLot_DeinitThread();
}

void
_PyParkingLot_InitThread(void)
{
    if (thread_data != NULL) {
        thread_data->refcount++;
        return;
    }
    ThreadData *this_thread = PyMem_RawMalloc(sizeof(ThreadData));
    if (this_thread == NULL) {
        Py_FatalError("_PyParkingLot_InitThread: unable to allocate thread data");
    }
    memset(this_thread, 0, sizeof(*this_thread));
    this_thread->refcount = 1;
    this_thread->depth = 0;
    for (int i = 0; i < MAX_SEMA_DEPTH; i++) {
        _PySemaphore_Init(&this_thread->semas[i]);
    }
    thread_data = this_thread;
}

void
_PyParkingLot_DeinitThread(void)
{
    ThreadData *td = thread_data;
    if (td == NULL) {
        return;
    }

    if (--td->refcount != 0) {
        assert(td->refcount > 0);
        return;
    }

    thread_data = NULL;
    for (int i = 0; i < MAX_SEMA_DEPTH; i++) {
        _PySemaphore_Destroy(&td->semas[i]);
    }

    PyMem_RawFree(td);
}

static void
enqueue(Bucket *bucket, const void *address, struct wait_entry *wait)
{
    if (!bucket->root.next) {
        // initialize bucket
        llist_init(&bucket->root);
    }
    llist_insert_tail(&bucket->root, &wait->node);
    ++bucket->num_waiters;
}

static struct wait_entry *
dequeue(Bucket *bucket, const void *address)
{
    struct llist_node *root = &bucket->root;
    if (!root->next) {
        // bucket was not yet initialized
        return NULL;
    }

    // find the first waiter that is waiting on `address`
    struct llist_node *node;
    llist_for_each(node, root) {
        struct wait_entry *wait = llist_data(node, struct wait_entry, node);
        if (wait->addr == (uintptr_t)address) {
            llist_remove(node);
            --bucket->num_waiters;
            return wait;
        }
    }
    return NULL;
}

static void
dequeue_all(Bucket *bucket, const void *address, struct llist_node *dst)
{
    struct llist_node *root = &bucket->root;
    if (!root->next) {
        // bucket was not yet initialized
        return;
    }

    // remove and append all matching waiters to dst
    struct llist_node *node;
    llist_for_each_safe(node, root) {
        struct wait_entry *wait = llist_data(node, struct wait_entry, node);
        if (wait->addr == (uintptr_t)address) {
            llist_remove(node);
            llist_insert_tail(dst, node);
            --bucket->num_waiters;
        }
    }
}

// Checks that `*addr == *expected`
static int
validate_addr(const void *addr, const void *expected, size_t addr_size)
{
    switch (addr_size) {
    case 1: return _Py_atomic_load_uint8(addr) == *(const uint8_t *)expected;
    case 2: return _Py_atomic_load_uint16(addr) == *(const uint16_t *)expected;
    case 4: return _Py_atomic_load_uint32(addr) == *(const uint32_t *)expected;
    case 8: return _Py_atomic_load_uint64(addr) == *(const uint64_t *)expected;
    default: Py_UNREACHABLE();
    }
}

int
_PyParkingLot_Park(const void *addr, const void *expected, size_t size,
                   _PyTime_t timeout_ns, void *arg, int detach)
{
    struct wait_entry wait;
    wait.unpark.arg = arg;
    wait.addr = (uintptr_t)addr;

    Bucket *bucket = &buckets[((uintptr_t)addr) % NUM_BUCKETS];

    _PyRawMutex_Lock(&bucket->mutex);
    if (!validate_addr(addr, expected, size)) {
        _PyRawMutex_Unlock(&bucket->mutex);
        return Py_PARK_AGAIN;
    }
    wait.sema = _PySemaphore_Alloc();
    enqueue(bucket, addr, &wait);
    _PyRawMutex_Unlock(&bucket->mutex);

    int res = _PySemaphore_Wait(wait.sema, timeout_ns, detach);
    if (res == Py_PARK_OK) {
        goto done;
    }

    // timeout or interrupt
    _PyRawMutex_Lock(&bucket->mutex);
    if (wait.node.next == NULL) {
        _PyRawMutex_Unlock(&bucket->mutex);
        // We've been removed the waiter queue. Wait until we process the
        // wakeup signal.
        do {
            res = _PySemaphore_Wait(wait.sema, -1, detach);
        } while (res != Py_PARK_OK);
        goto done;
    }
    else {
        llist_remove(&wait.node);
        --bucket->num_waiters;
    }
    _PyRawMutex_Unlock(&bucket->mutex);

done:
    _PySemaphore_Free(wait.sema);
    return res;

}

struct _PyUnpark *
_PyParkingLot_BeginUnpark(const void *addr)
{
    Bucket *bucket = &buckets[((uintptr_t)addr) % NUM_BUCKETS];
    _PyRawMutex_Lock(&bucket->mutex);

    struct wait_entry *waiter = dequeue(bucket, addr);
    if (!waiter) {
        return NULL;
    }

    waiter->unpark.more_waiters = (bucket->num_waiters > 0);
    return &waiter->unpark;
}

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

void
_PyParkingLot_FinishUnpark(const void *addr, struct _PyUnpark *unpark)
{
    Bucket *bucket = &buckets[((uintptr_t)addr) % NUM_BUCKETS];
    _PyRawMutex_Unlock(&bucket->mutex);

    if (unpark) {
        struct wait_entry *waiter;
        waiter = container_of(unpark, struct wait_entry, unpark);

        _PySemaphore_Wakeup(waiter->sema);
    }
}

void
_PyParkingLot_UnparkAll(const void *addr)
{
    struct llist_node head = LLIST_INIT(head);
    Bucket *bucket = &buckets[((uintptr_t)addr) % NUM_BUCKETS];

    _PyRawMutex_Lock(&bucket->mutex);
    dequeue_all(bucket, addr, &head);
    _PyRawMutex_Unlock(&bucket->mutex);

    struct llist_node *node;
    llist_for_each_safe(node, &head) {
        struct wait_entry *waiter = llist_data(node, struct wait_entry, node);
        llist_remove(node);
        _PySemaphore_Wakeup(waiter->sema);
    }
}

void
_PyParkingLot_AfterFork(void)
{
    // After a fork only one thread remains. That thread cannot be blocked
    // so all entries in the parking lot are for dead threads.
    memset(buckets, 0, sizeof(buckets));
}
