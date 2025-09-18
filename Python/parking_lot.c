#include "Python.h"

#include "pycore_llist.h"
#include "pycore_lock.h"          // _PyRawMutex
#include "pycore_parking_lot.h"
#include "pycore_pyerrors.h"      // _Py_FatalErrorFormat
#include "pycore_pystate.h"       // _PyThreadState_GET
#include "pycore_semaphore.h"     // _PySemaphore
#include "pycore_time.h"          // _PyTime_Add()

#include <stdbool.h>


typedef struct {
    // The mutex protects the waiter queue and the num_waiters counter.
    _PyRawMutex mutex;

    // Linked list of `struct wait_entry` waiters in this bucket.
    struct llist_node root;
    size_t num_waiters;
} Bucket;

struct wait_entry {
    void *park_arg;
    uintptr_t addr;
    _PySemaphore sema;
    struct llist_node node;
    bool is_unparking;
};

// Prime number to avoid correlations with memory addresses.
// We want this to be roughly proportional to the number of CPU cores
// to minimize contention on the bucket locks, but not too big to avoid
// wasting memory. The exact choice does not matter much.
#define NUM_BUCKETS 257

#define BUCKET_INIT(b, i) [i] = { .root = LLIST_INIT(b[i].root) }
#define BUCKET_INIT_2(b, i)   BUCKET_INIT(b, i),     BUCKET_INIT(b, i+1)
#define BUCKET_INIT_4(b, i)   BUCKET_INIT_2(b, i),   BUCKET_INIT_2(b, i+2)
#define BUCKET_INIT_8(b, i)   BUCKET_INIT_4(b, i),   BUCKET_INIT_4(b, i+4)
#define BUCKET_INIT_16(b, i)  BUCKET_INIT_8(b, i),   BUCKET_INIT_8(b, i+8)
#define BUCKET_INIT_32(b, i)  BUCKET_INIT_16(b, i),  BUCKET_INIT_16(b, i+16)
#define BUCKET_INIT_64(b, i)  BUCKET_INIT_32(b, i),  BUCKET_INIT_32(b, i+32)
#define BUCKET_INIT_128(b, i) BUCKET_INIT_64(b, i),  BUCKET_INIT_64(b, i+64)
#define BUCKET_INIT_256(b, i) BUCKET_INIT_128(b, i), BUCKET_INIT_128(b, i+128)

// Table of waiters (hashed by address)
static Bucket buckets[NUM_BUCKETS] = {
    BUCKET_INIT_256(buckets, 0),
    BUCKET_INIT(buckets, 256),
};

void
_PySemaphore_Init(_PySemaphore *sema)
{
#if defined(MS_WINDOWS)
    sema->platform_sem = CreateSemaphore(
        NULL,   //  attributes
        0,      //  initial count
        10,     //  maximum count
        NULL    //  unnamed
    );
    if (!sema->platform_sem) {
        Py_FatalError("parking_lot: CreateSemaphore failed");
    }
#elif defined(_Py_USE_SEMAPHORES)
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
    sema->counter = 0;
#endif
}

void
_PySemaphore_Destroy(_PySemaphore *sema)
{
#if defined(MS_WINDOWS)
    CloseHandle(sema->platform_sem);
#elif defined(_Py_USE_SEMAPHORES)
    sem_destroy(&sema->platform_sem);
#else
    pthread_mutex_destroy(&sema->mutex);
    pthread_cond_destroy(&sema->cond);
#endif
}

int
_PySemaphore_Wait(_PySemaphore *sema, PyTime_t timeout)
{
    int res;
#if defined(MS_WINDOWS)
    DWORD wait;
    DWORD millis = 0;
    if (timeout < 0) {
        millis = INFINITE;
    }
    else {
        PyTime_t div = _PyTime_AsMilliseconds(timeout, _PyTime_ROUND_TIMEOUT);
        // Prevent overflow with clamping the result
        if ((PyTime_t)PY_DWORD_MAX < div) {
            millis = PY_DWORD_MAX;
        }
        else {
            millis = (DWORD) div;
        }
    }

    HANDLE handles[2] = { sema->platform_sem, NULL };
    HANDLE sigint_event = NULL;
    DWORD count = 1;
    if (_Py_IsMainThread()) {
        // gh-135099: Wait on the SIGINT event only in the main thread. Other
        // threads would ignore the result anyways, and accessing
        // `_PyOS_SigintEvent()` from non-main threads may race with
        // interpreter shutdown, which closes the event handle. Note that
        // non-main interpreters will ignore the result.
        sigint_event = _PyOS_SigintEvent();
        if (sigint_event != NULL) {
            handles[1] = sigint_event;
            count = 2;
        }
    }
    wait = WaitForMultipleObjects(count, handles, FALSE, millis);
    if (wait == WAIT_OBJECT_0) {
        res = Py_PARK_OK;
    }
    else if (wait == WAIT_OBJECT_0 + 1) {
        assert(sigint_event != NULL);
        ResetEvent(sigint_event);
        res = Py_PARK_INTR;
    }
    else if (wait == WAIT_TIMEOUT) {
        res = Py_PARK_TIMEOUT;
    }
    else {
        _Py_FatalErrorFormat(__func__,
            "unexpected error from semaphore: %u (error: %u)",
            wait, GetLastError());
    }
#elif defined(_Py_USE_SEMAPHORES)
    int err;
    if (timeout >= 0) {
        struct timespec ts;

#if defined(CLOCK_MONOTONIC) && defined(HAVE_SEM_CLOCKWAIT) && !defined(_Py_THREAD_SANITIZER)
        PyTime_t now;
        // silently ignore error: cannot report error to the caller
        (void)PyTime_MonotonicRaw(&now);
        PyTime_t deadline = _PyTime_Add(now, timeout);
        _PyTime_AsTimespec_clamp(deadline, &ts);

        err = sem_clockwait(&sema->platform_sem, CLOCK_MONOTONIC, &ts);
#else
        PyTime_t now;
        // silently ignore error: cannot report error to the caller
        (void)PyTime_TimeRaw(&now);
        PyTime_t deadline = _PyTime_Add(now, timeout);

        _PyTime_AsTimespec_clamp(deadline, &ts);

        err = sem_timedwait(&sema->platform_sem, &ts);
#endif
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
    int err = 0;
    if (sema->counter == 0) {
        if (timeout >= 0) {
            struct timespec ts;
#if defined(HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP)
            _PyTime_AsTimespec_clamp(timeout, &ts);
            err = pthread_cond_timedwait_relative_np(&sema->cond, &sema->mutex, &ts);
#else
            PyTime_t now;
            (void)PyTime_TimeRaw(&now);
            PyTime_t deadline = _PyTime_Add(now, timeout);
            _PyTime_AsTimespec_clamp(deadline, &ts);

            err = pthread_cond_timedwait(&sema->cond, &sema->mutex, &ts);
#endif // HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
        }
        else {
            err = pthread_cond_wait(&sema->cond, &sema->mutex);
        }
    }
    if (sema->counter > 0) {
        sema->counter--;
        res = Py_PARK_OK;
    }
    else if (err) {
        res = Py_PARK_TIMEOUT;
    }
    else {
        res = Py_PARK_INTR;
    }
    pthread_mutex_unlock(&sema->mutex);
#endif
    return res;
}

void
_PySemaphore_Wakeup(_PySemaphore *sema)
{
#if defined(MS_WINDOWS)
    if (!ReleaseSemaphore(sema->platform_sem, 1, NULL)) {
        Py_FatalError("parking_lot: ReleaseSemaphore failed");
    }
#elif defined(_Py_USE_SEMAPHORES)
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

static void
enqueue(Bucket *bucket, const void *address, struct wait_entry *wait)
{
    llist_insert_tail(&bucket->root, &wait->node);
    ++bucket->num_waiters;
}

static struct wait_entry *
dequeue(Bucket *bucket, const void *address)
{
    // find the first waiter that is waiting on `address`
    struct llist_node *root = &bucket->root;
    struct llist_node *node;
    llist_for_each(node, root) {
        struct wait_entry *wait = llist_data(node, struct wait_entry, node);
        if (wait->addr == (uintptr_t)address) {
            llist_remove(node);
            --bucket->num_waiters;
            wait->is_unparking = true;
            return wait;
        }
    }
    return NULL;
}

static void
dequeue_all(Bucket *bucket, const void *address, struct llist_node *dst)
{
    // remove and append all matching waiters to dst
    struct llist_node *root = &bucket->root;
    struct llist_node *node;
    llist_for_each_safe(node, root) {
        struct wait_entry *wait = llist_data(node, struct wait_entry, node);
        if (wait->addr == (uintptr_t)address) {
            llist_remove(node);
            llist_insert_tail(dst, node);
            --bucket->num_waiters;
            wait->is_unparking = true;
        }
    }
}

// Checks that `*addr == *expected` (only works for 1, 2, 4, or 8 bytes)
static int
atomic_memcmp(const void *addr, const void *expected, size_t addr_size)
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
                   PyTime_t timeout_ns, void *park_arg, int detach)
{
    struct wait_entry wait = {
        .park_arg = park_arg,
        .addr = (uintptr_t)addr,
        .is_unparking = false,
    };

    Bucket *bucket = &buckets[((uintptr_t)addr) % NUM_BUCKETS];

    _PyRawMutex_Lock(&bucket->mutex);
    if (!atomic_memcmp(addr, expected, size)) {
        _PyRawMutex_Unlock(&bucket->mutex);
        return Py_PARK_AGAIN;
    }
    _PySemaphore_Init(&wait.sema);
    enqueue(bucket, addr, &wait);
    _PyRawMutex_Unlock(&bucket->mutex);

    PyThreadState *tstate = NULL;
    if (detach) {
        tstate = _PyThreadState_GET();
        if (tstate && _PyThreadState_IsAttached(tstate)) {
            // Only detach if we are attached
            PyEval_ReleaseThread(tstate);
        }
        else {
            tstate = NULL;
        }
    }

    int res = _PySemaphore_Wait(&wait.sema, timeout_ns);
    if (res == Py_PARK_OK) {
        goto done;
    }

    // timeout or interrupt
    _PyRawMutex_Lock(&bucket->mutex);
    if (wait.is_unparking) {
        _PyRawMutex_Unlock(&bucket->mutex);
        // Another thread has started to unpark us. Wait until we process the
        // wakeup signal.
        do {
            res = _PySemaphore_Wait(&wait.sema, -1);
        } while (res != Py_PARK_OK);
        goto done;
    }
    else {
        llist_remove(&wait.node);
        --bucket->num_waiters;
    }
    _PyRawMutex_Unlock(&bucket->mutex);

done:
    _PySemaphore_Destroy(&wait.sema);
    if (tstate) {
        PyEval_AcquireThread(tstate);
    }
    return res;

}

void
_PyParkingLot_Unpark(const void *addr, _Py_unpark_fn_t *fn, void *arg)
{
    Bucket *bucket = &buckets[((uintptr_t)addr) % NUM_BUCKETS];

    // Find the first waiter that is waiting on `addr`
    _PyRawMutex_Lock(&bucket->mutex);
    struct wait_entry *waiter = dequeue(bucket, addr);
    if (waiter) {
        int has_more_waiters = (bucket->num_waiters > 0);
        fn(arg, waiter->park_arg, has_more_waiters);
    }
    else {
        fn(arg, NULL, 0);
    }
    _PyRawMutex_Unlock(&bucket->mutex);

    if (waiter) {
        // Wakeup the waiter outside of the bucket lock
        _PySemaphore_Wakeup(&waiter->sema);
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
        _PySemaphore_Wakeup(&waiter->sema);
    }
}

void
_PyParkingLot_AfterFork(void)
{
    // After a fork only one thread remains. That thread cannot be blocked
    // so all entries in the parking lot are for dead threads.
    memset(buckets, 0, sizeof(buckets));
    for (Py_ssize_t i = 0; i < NUM_BUCKETS; i++) {
        llist_init(&buckets[i].root);
    }
}
