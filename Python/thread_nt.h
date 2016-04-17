
/* This code implemented by Dag.Gruneau@elsa.preseco.comm.se */
/* Fast NonRecursiveMutex support by Yakov Markovitch, markovitch@iso.ru */
/* Eliminated some memory leaks, gsw@agere.com */

#include <windows.h>
#include <limits.h>
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

typedef struct NRMUTEX {
    LONG   owned ;
    DWORD  thread_id ;
    HANDLE hevent ;
} NRMUTEX, *PNRMUTEX ;


BOOL
InitializeNonRecursiveMutex(PNRMUTEX mutex)
{
    mutex->owned = -1 ;  /* No threads have entered NonRecursiveMutex */
    mutex->thread_id = 0 ;
    mutex->hevent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    return mutex->hevent != NULL ;      /* TRUE if the mutex is created */
}

VOID
DeleteNonRecursiveMutex(PNRMUTEX mutex)
{
    /* No in-use check */
    CloseHandle(mutex->hevent) ;
    mutex->hevent = NULL ; /* Just in case */
}

DWORD
EnterNonRecursiveMutex(PNRMUTEX mutex, BOOL wait)
{
    /* Assume that the thread waits successfully */
    DWORD ret ;

    /* InterlockedIncrement(&mutex->owned) == 0 means that no thread currently owns the mutex */
    if (!wait)
    {
        if (InterlockedCompareExchange(&mutex->owned, 0, -1) != -1)
            return WAIT_TIMEOUT ;
        ret = WAIT_OBJECT_0 ;
    }
    else
        ret = InterlockedIncrement(&mutex->owned) ?
            /* Some thread owns the mutex, let's wait... */
            WaitForSingleObject(mutex->hevent, INFINITE) : WAIT_OBJECT_0 ;

    mutex->thread_id = GetCurrentThreadId() ; /* We own it */
    return ret ;
}

BOOL
LeaveNonRecursiveMutex(PNRMUTEX mutex)
{
    /* We don't own the mutex */
    mutex->thread_id = 0 ;
    return
        InterlockedDecrement(&mutex->owned) < 0 ||
        SetEvent(mutex->hevent) ; /* Other threads are waiting, wake one on them up */
}

PNRMUTEX
AllocNonRecursiveMutex(void)
{
    PNRMUTEX mutex = (PNRMUTEX)malloc(sizeof(NRMUTEX)) ;
    if (mutex && !InitializeNonRecursiveMutex(mutex))
    {
        free(mutex) ;
        mutex = NULL ;
    }
    return mutex ;
}

void
FreeNonRecursiveMutex(PNRMUTEX mutex)
{
    if (mutex)
    {
        DeleteNonRecursiveMutex(mutex) ;
        free(mutex) ;
    }
}

long PyThread_get_thread_ident(void);

/*
 * Initialization of the C package, should not be needed.
 */
static void
PyThread__init_thread(void)
{
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
#if defined(MS_WINCE)
static DWORD WINAPI
#else
static unsigned __stdcall
#endif
bootstrap(void *call)
{
    callobj *obj = (callobj*)call;
    void (*func)(void*) = obj->func;
    void *arg = obj->arg;
    HeapFree(GetProcessHeap(), 0, obj);
    func(arg);
    return 0;
}

long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    HANDLE hThread;
    unsigned threadID;
    callobj *obj;

    dprintf(("%ld: PyThread_start_new_thread called\n",
             PyThread_get_thread_ident()));
    if (!initialized)
        PyThread_init_thread();

    obj = (callobj*)HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    if (!obj)
        return -1;
    obj->func = func;
    obj->arg = arg;
#if defined(MS_WINCE)
    hThread = CreateThread(NULL,
                           Py_SAFE_DOWNCAST(_pythread_stacksize, Py_ssize_t, SIZE_T),
                           bootstrap, obj, 0, &threadID);
#else
    hThread = (HANDLE)_beginthreadex(0,
                      Py_SAFE_DOWNCAST(_pythread_stacksize,
                                       Py_ssize_t, unsigned int),
                      bootstrap, obj,
                      0, &threadID);
#endif
    if (hThread == 0) {
#if defined(MS_WINCE)
        /* Save error in variable, to prevent PyThread_get_thread_ident
           from clobbering it. */
        unsigned e = GetLastError();
        dprintf(("%ld: PyThread_start_new_thread failed, win32 error code %u\n",
                 PyThread_get_thread_ident(), e));
#else
        /* I've seen errno == EAGAIN here, which means "there are
         * too many threads".
         */
        int e = errno;
        dprintf(("%ld: PyThread_start_new_thread failed, errno %d\n",
                 PyThread_get_thread_ident(), e));
#endif
        threadID = (unsigned)-1;
        HeapFree(GetProcessHeap(), 0, obj);
    }
    else {
        dprintf(("%ld: PyThread_start_new_thread succeeded: %p\n",
                 PyThread_get_thread_ident(), (void*)hThread));
        CloseHandle(hThread);
    }
    return (long) threadID;
}

/*
 * Return the thread Id instead of a handle. The Id is said to uniquely identify the
 * thread in the system
 */
long
PyThread_get_thread_ident(void)
{
    if (!initialized)
        PyThread_init_thread();

    return GetCurrentThreadId();
}

void
PyThread_exit_thread(void)
{
    dprintf(("%ld: PyThread_exit_thread called\n", PyThread_get_thread_ident()));
    if (!initialized)
        exit(0);
#if defined(MS_WINCE)
    ExitThread(0);
#else
    _endthreadex(0);
#endif
}

/*
 * Lock support. It has too be implemented as semaphores.
 * I [Dag] tried to implement it with mutex but I could find a way to
 * tell whether a thread already own the lock or not.
 */
PyThread_type_lock
PyThread_allocate_lock(void)
{
    PNRMUTEX aLock;

    dprintf(("PyThread_allocate_lock called\n"));
    if (!initialized)
        PyThread_init_thread();

    aLock = AllocNonRecursiveMutex() ;

    dprintf(("%ld: PyThread_allocate_lock() -> %p\n", PyThread_get_thread_ident(), aLock));

    return (PyThread_type_lock) aLock;
}

void
PyThread_free_lock(PyThread_type_lock aLock)
{
    dprintf(("%ld: PyThread_free_lock(%p) called\n", PyThread_get_thread_ident(),aLock));

    FreeNonRecursiveMutex(aLock) ;
}

/*
 * Return 1 on success if the lock was acquired
 *
 * and 0 if the lock was not acquired. This means a 0 is returned
 * if the lock has already been acquired by this thread!
 */
int
PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
    int success ;

    dprintf(("%ld: PyThread_acquire_lock(%p, %d) called\n", PyThread_get_thread_ident(),aLock, waitflag));

    success = aLock && EnterNonRecursiveMutex((PNRMUTEX) aLock, (waitflag ? INFINITE : 0)) == WAIT_OBJECT_0 ;

    dprintf(("%ld: PyThread_acquire_lock(%p, %d) -> %d\n", PyThread_get_thread_ident(),aLock, waitflag, success));

    return success;
}

void
PyThread_release_lock(PyThread_type_lock aLock)
{
    dprintf(("%ld: PyThread_release_lock(%p) called\n", PyThread_get_thread_ident(),aLock));

    if (!(aLock && LeaveNonRecursiveMutex((PNRMUTEX) aLock)))
        dprintf(("%ld: Could not PyThread_release_lock(%p) error: %ld\n", PyThread_get_thread_ident(), aLock, GetLastError()));
}

/* minimum/maximum thread stack sizes supported */
#define THREAD_MIN_STACKSIZE    0x8000          /* 32kB */
#define THREAD_MAX_STACKSIZE    0x10000000      /* 256MB */

/* set the thread stack size.
 * Return 0 if size is valid, -1 otherwise.
 */
static int
_pythread_nt_set_stacksize(size_t size)
{
    /* set to default */
    if (size == 0) {
        _pythread_stacksize = 0;
        return 0;
    }

    /* valid range? */
    if (size >= THREAD_MIN_STACKSIZE && size < THREAD_MAX_STACKSIZE) {
        _pythread_stacksize = size;
        return 0;
    }

    return -1;
}

#define THREAD_SET_STACKSIZE(x) _pythread_nt_set_stacksize(x)


/* use native Windows TLS functions */
#define Py_HAVE_NATIVE_TLS

#ifdef Py_HAVE_NATIVE_TLS
int
PyThread_create_key(void)
{
    return (int) TlsAlloc();
}

void
PyThread_delete_key(int key)
{
    TlsFree(key);
}

/* We must be careful to emulate the strange semantics implemented in thread.c,
 * where the value is only set if it hasn't been set before.
 */
int
PyThread_set_key_value(int key, void *value)
{
    BOOL ok;
    void *oldvalue;

    assert(value != NULL);
    oldvalue = TlsGetValue(key);
    if (oldvalue != NULL)
        /* ignore value if already set */
        return 0;
    ok = TlsSetValue(key, value);
    if (!ok)
        return -1;
    return 0;
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
{}

#endif
