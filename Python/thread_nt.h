#include "pycore_interp.h"    // _PyInterpreterState.threads.stacksize

/* This code implemented by Dag.Gruneau@elsa.preseco.comm.se */
/* Fast NonRecursiveMutex support by Yakov Markovitch, markovitch@iso.ru */
/* Eliminated some memory leaks, gsw@agere.com */

#include <windows.h>
#include <limits.h>
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

/* options */
#ifndef _PY_USE_CV_LOCKS
#define _PY_USE_CV_LOCKS 1     /* use locks based on cond vars */
#endif

/* Now, define a non-recursive mutex using either condition variables
 * and critical sections (fast) or using operating system mutexes
 * (slow)
 */

#if _PY_USE_CV_LOCKS

#include "condvar.h"

typedef struct _NRMUTEX
{
    PyMUTEX_T cs;
    PyCOND_T cv;
    int locked;
} NRMUTEX;
typedef NRMUTEX *PNRMUTEX;

static PNRMUTEX
AllocNonRecursiveMutex(void)
{
    PNRMUTEX m = (PNRMUTEX)PyMem_RawMalloc(sizeof(NRMUTEX));
    if (!m)
        return NULL;
    if (PyCOND_INIT(&m->cv))
        goto fail;
    if (PyMUTEX_INIT(&m->cs)) {
        PyCOND_FINI(&m->cv);
        goto fail;
    }
    m->locked = 0;
    return m;
fail:
    PyMem_RawFree(m);
    return NULL;
}

static VOID
FreeNonRecursiveMutex(PNRMUTEX mutex)
{
    if (mutex) {
        PyCOND_FINI(&mutex->cv);
        PyMUTEX_FINI(&mutex->cs);
        PyMem_RawFree(mutex);
    }
}

static DWORD
EnterNonRecursiveMutex(PNRMUTEX mutex, DWORD milliseconds)
{
    DWORD result = WAIT_OBJECT_0;
    if (PyMUTEX_LOCK(&mutex->cs))
        return WAIT_FAILED;
    if (milliseconds == INFINITE) {
        while (mutex->locked) {
            if (PyCOND_WAIT(&mutex->cv, &mutex->cs)) {
                result = WAIT_FAILED;
                break;
            }
        }
    } else if (milliseconds != 0) {
        /* wait at least until the deadline */
        _PyTime_t nanoseconds = _PyTime_FromNanoseconds((_PyTime_t)milliseconds * 1000000);
        _PyTime_t deadline = _PyTime_Add(_PyTime_GetPerfCounter(), nanoseconds);
        while (mutex->locked) {
            _PyTime_t microseconds = _PyTime_AsMicroseconds(nanoseconds,
                                                            _PyTime_ROUND_TIMEOUT);
            if (PyCOND_TIMEDWAIT(&mutex->cv, &mutex->cs, microseconds) < 0) {
                result = WAIT_FAILED;
                break;
            }
            nanoseconds = deadline - _PyTime_GetPerfCounter();
            if (nanoseconds <= 0) {
                break;
            }
        }
    }
    if (!mutex->locked) {
        mutex->locked = 1;
        result = WAIT_OBJECT_0;
    } else if (result == WAIT_OBJECT_0)
        result = WAIT_TIMEOUT;
    /* else, it is WAIT_FAILED */
    PyMUTEX_UNLOCK(&mutex->cs); /* must ignore result here */
    return result;
}

static BOOL
LeaveNonRecursiveMutex(PNRMUTEX mutex)
{
    BOOL result;
    if (PyMUTEX_LOCK(&mutex->cs))
        return FALSE;
    mutex->locked = 0;
    /* condvar APIs return 0 on success. We need to return TRUE on success. */
    result = !PyCOND_SIGNAL(&mutex->cv);
    PyMUTEX_UNLOCK(&mutex->cs);
    return result;
}

#else /* if ! _PY_USE_CV_LOCKS */

/* NR-locks based on a kernel mutex */
#define PNRMUTEX HANDLE

static PNRMUTEX
AllocNonRecursiveMutex(void)
{
    return CreateSemaphore(NULL, 1, 1, NULL);
}

static VOID
FreeNonRecursiveMutex(PNRMUTEX mutex)
{
    /* No in-use check */
    CloseHandle(mutex);
}

static DWORD
EnterNonRecursiveMutex(PNRMUTEX mutex, DWORD milliseconds)
{
    return WaitForSingleObjectEx(mutex, milliseconds, FALSE);
}

static BOOL
LeaveNonRecursiveMutex(PNRMUTEX mutex)
{
    return ReleaseSemaphore(mutex, 1, NULL);
}
#endif /* _PY_USE_CV_LOCKS */

unsigned long PyThread_get_thread_ident(void);

#ifdef PY_HAVE_THREAD_NATIVE_ID
unsigned long PyThread_get_thread_native_id(void);
#endif

/*
 * Initialization for the current runtime.
 */
static void
PyThread__init_thread(void)
{
    // Initialization of the C package should not be needed.
}

/*
 * Thread support.
 */

typedef struct {
    void (*func)(void*);
    void *arg;
} callobj;

/* thunker to call adapt between the function type used by the system's
thread start function and the internally used one. */
static unsigned __stdcall
bootstrap(void *call)
{
    callobj *obj = (callobj*)call;
    void (*func)(void*) = obj->func;
    void *arg = obj->arg;
    HeapFree(GetProcessHeap(), 0, obj);
    func(arg);
    return 0;
}

int
PyThread_start_joinable_thread(void (*func)(void *), void *arg,
                               PyThread_ident_t* ident, PyThread_handle_t* handle) {
    HANDLE hThread;
    unsigned threadID;
    callobj *obj;

    if (!initialized)
        PyThread_init_thread();

    obj = (callobj*)HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    if (!obj)
        return -1;
    obj->func = func;
    obj->arg = arg;
    PyThreadState *tstate = _PyThreadState_GET();
    size_t stacksize = tstate ? tstate->interp->threads.stacksize : 0;
    hThread = (HANDLE)_beginthreadex(0,
                      Py_SAFE_DOWNCAST(stacksize, Py_ssize_t, unsigned int),
                      bootstrap, obj,
                      0, &threadID);
    if (hThread == 0) {
        /* I've seen errno == EAGAIN here, which means "there are
         * too many threads".
         */
        HeapFree(GetProcessHeap(), 0, obj);
        return -1;
    }
    *ident = threadID;
    // The cast is safe since HANDLE is pointer-sized
    *handle = (PyThread_handle_t) hThread;
    return 0;
}

unsigned long
PyThread_start_new_thread(void (*func)(void *), void *arg) {
    PyThread_handle_t handle;
    PyThread_ident_t ident;
    if (PyThread_start_joinable_thread(func, arg, &ident, &handle)) {
        return PYTHREAD_INVALID_THREAD_ID;
    }
    CloseHandle((HANDLE) handle);
    // The cast is safe since the ident is really an unsigned int
    return (unsigned long) ident;
}

int
PyThread_join_thread(PyThread_handle_t handle) {
    HANDLE hThread = (HANDLE) handle;
    int errored = (WaitForSingleObject(hThread, INFINITE) != WAIT_OBJECT_0);
    CloseHandle(hThread);
    return errored;
}

int
PyThread_detach_thread(PyThread_handle_t handle) {
    HANDLE hThread = (HANDLE) handle;
    return (CloseHandle(hThread) == 0);
}

void
PyThread_update_thread_after_fork(PyThread_ident_t* ident, PyThread_handle_t* handle) {
}

/*
 * Return the thread Id instead of a handle. The Id is said to uniquely identify the
 * thread in the system
 */
PyThread_ident_t
PyThread_get_thread_ident_ex(void)
{
    if (!initialized)
        PyThread_init_thread();

    return GetCurrentThreadId();
}

unsigned long
PyThread_get_thread_ident(void)
{
    return (unsigned long) PyThread_get_thread_ident_ex();
}


#ifdef PY_HAVE_THREAD_NATIVE_ID
/*
 * Return the native Thread ID (TID) of the calling thread.
 * The native ID of a thread is valid and guaranteed to be unique system-wide
 * from the time the thread is created until the thread has been terminated.
 */
unsigned long
PyThread_get_thread_native_id(void)
{
    if (!initialized) {
        PyThread_init_thread();
    }

    DWORD native_id;
    native_id = GetCurrentThreadId();
    return (unsigned long) native_id;
}
#endif

void _Py_NO_RETURN
PyThread_exit_thread(void)
{
    if (!initialized)
        exit(0);
    _endthreadex(0);
}

/*
 * Lock support. It has to be implemented as semaphores.
 * I [Dag] tried to implement it with mutex but I could find a way to
 * tell whether a thread already own the lock or not.
 */
PyThread_type_lock
PyThread_allocate_lock(void)
{
    PNRMUTEX mutex;

    if (!initialized)
        PyThread_init_thread();

    mutex = AllocNonRecursiveMutex() ;

    PyThread_type_lock aLock = (PyThread_type_lock) mutex;
    assert(aLock);

    return aLock;
}

void
PyThread_free_lock(PyThread_type_lock aLock)
{
    FreeNonRecursiveMutex(aLock) ;
}

// WaitForSingleObject() accepts timeout in milliseconds in the range
// [0; 0xFFFFFFFE] (DWORD type). INFINITE value (0xFFFFFFFF) means no
// timeout. 0xFFFFFFFE milliseconds is around 49.7 days.
const DWORD TIMEOUT_MS_MAX = 0xFFFFFFFE;

/*
 * Return 1 on success if the lock was acquired
 *
 * and 0 if the lock was not acquired. This means a 0 is returned
 * if the lock has already been acquired by this thread!
 */
PyLockStatus
PyThread_acquire_lock_timed(PyThread_type_lock aLock,
                            PY_TIMEOUT_T microseconds, int intr_flag)
{
    assert(aLock);

    /* Fow now, intr_flag does nothing on Windows, and lock acquires are
     * uninterruptible.  */
    PyLockStatus success;
    PY_TIMEOUT_T milliseconds;

    if (microseconds >= 0) {
        milliseconds = microseconds / 1000;
        // Round milliseconds away from zero
        if (microseconds % 1000 > 0) {
            milliseconds++;
        }
        if (milliseconds > (PY_TIMEOUT_T)TIMEOUT_MS_MAX) {
            // bpo-41710: PyThread_acquire_lock_timed() cannot report timeout
            // overflow to the caller, so clamp the timeout to
            // [0, TIMEOUT_MS_MAX] milliseconds.
            //
            // _thread.Lock.acquire() and _thread.RLock.acquire() raise an
            // OverflowError if microseconds is greater than PY_TIMEOUT_MAX.
            milliseconds = TIMEOUT_MS_MAX;
        }
        assert(milliseconds != INFINITE);
    }
    else {
        milliseconds = INFINITE;
    }

    if (EnterNonRecursiveMutex((PNRMUTEX)aLock,
                               (DWORD)milliseconds) == WAIT_OBJECT_0) {
        success = PY_LOCK_ACQUIRED;
    }
    else {
        success = PY_LOCK_FAILURE;
    }

    return success;
}
int
PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
    return PyThread_acquire_lock_timed(aLock, waitflag ? -1 : 0, 0);
}

void
PyThread_release_lock(PyThread_type_lock aLock)
{
    assert(aLock);
    (void)LeaveNonRecursiveMutex((PNRMUTEX) aLock);
}

/* minimum/maximum thread stack sizes supported */
#define THREAD_MIN_STACKSIZE    0x8000          /* 32 KiB */
#define THREAD_MAX_STACKSIZE    0x10000000      /* 256 MiB */

/* set the thread stack size.
 * Return 0 if size is valid, -1 otherwise.
 */
static int
_pythread_nt_set_stacksize(size_t size)
{
    /* set to default */
    if (size == 0) {
        _PyInterpreterState_GET()->threads.stacksize = 0;
        return 0;
    }

    /* valid range? */
    if (size >= THREAD_MIN_STACKSIZE && size < THREAD_MAX_STACKSIZE) {
        _PyInterpreterState_GET()->threads.stacksize = size;
        return 0;
    }

    return -1;
}

#define THREAD_SET_STACKSIZE(x) _pythread_nt_set_stacksize(x)


/* Thread Local Storage (TLS) API

   This API is DEPRECATED since Python 3.7.  See PEP 539 for details.
*/

int
PyThread_create_key(void)
{
    DWORD result = TlsAlloc();
    if (result == TLS_OUT_OF_INDEXES)
        return -1;
    return (int)result;
}

void
PyThread_delete_key(int key)
{
    TlsFree(key);
}

int
PyThread_set_key_value(int key, void *value)
{
    BOOL ok = TlsSetValue(key, value);
    return ok ? 0 : -1;
}

void *
PyThread_get_key_value(int key)
{
    /* because TLS is used in the Py_END_ALLOW_THREAD macro,
     * it is necessary to preserve the windows error state, because
     * it is assumed to be preserved across the call to the macro.
     * Ideally, the macro should be fixed, but it is simpler to
     * do it here.
     */
    DWORD error = GetLastError();
    void *result = TlsGetValue(key);
    SetLastError(error);
    return result;
}

void
PyThread_delete_key_value(int key)
{
    /* NULL is used as "key missing", and it is also the default
     * given by TlsGetValue() if nothing has been set yet.
     */
    TlsSetValue(key, NULL);
}


/* reinitialization of TLS is not necessary after fork when using
 * the native TLS functions.  And forking isn't supported on Windows either.
 */
void
PyThread_ReInitTLS(void)
{
}


/* Thread Specific Storage (TSS) API

   Platform-specific components of TSS API implementation.
*/

int
PyThread_tss_create(Py_tss_t *key)
{
    assert(key != NULL);
    /* If the key has been created, function is silently skipped. */
    if (key->_is_initialized) {
        return 0;
    }

    DWORD result = TlsAlloc();
    if (result == TLS_OUT_OF_INDEXES) {
        return -1;
    }
    /* In Windows, platform-specific key type is DWORD. */
    key->_key = result;
    key->_is_initialized = 1;
    return 0;
}

void
PyThread_tss_delete(Py_tss_t *key)
{
    assert(key != NULL);
    /* If the key has not been created, function is silently skipped. */
    if (!key->_is_initialized) {
        return;
    }

    TlsFree(key->_key);
    key->_key = TLS_OUT_OF_INDEXES;
    key->_is_initialized = 0;
}

int
PyThread_tss_set(Py_tss_t *key, void *value)
{
    assert(key != NULL);
    BOOL ok = TlsSetValue(key->_key, value);
    return ok ? 0 : -1;
}

void *
PyThread_tss_get(Py_tss_t *key)
{
    assert(key != NULL);
    /* because TSS is used in the Py_END_ALLOW_THREAD macro,
     * it is necessary to preserve the windows error state, because
     * it is assumed to be preserved across the call to the macro.
     * Ideally, the macro should be fixed, but it is simpler to
     * do it here.
     */
    DWORD error = GetLastError();
    void *result = TlsGetValue(key->_key);
    SetLastError(error);
    return result;
}
