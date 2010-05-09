/* This code implemented by cvale@netcom.com */

#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#include "os2.h"
#include "limits.h"

#include "process.h"

#if defined(PYCC_GCC)
#include <sys/builtin.h>
#include <sys/fmutex.h>
#else
long PyThread_get_thread_ident(void);
#endif

/* default thread stack size of 64kB */
#if !defined(THREAD_STACK_SIZE)
#define THREAD_STACK_SIZE       0x10000
#endif

#define OS2_STACKSIZE(x)        (x ? x : THREAD_STACK_SIZE)

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
long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    int thread_id;

    thread_id = _beginthread(func,
                            NULL,
                            OS2_STACKSIZE(_pythread_stacksize),
                            arg);

    if (thread_id == -1) {
        dprintf(("_beginthread failed. return %ld\n", errno));
    }

    return thread_id;
}

long
PyThread_get_thread_ident(void)
{
#if !defined(PYCC_GCC)
    PPIB pib;
    PTIB tib;
#endif

    if (!initialized)
        PyThread_init_thread();

#if defined(PYCC_GCC)
    return _gettid();
#else
    DosGetInfoBlocks(&tib, &pib);
    return tib->tib_ptib2->tib2_ultid;
#endif
}

void
PyThread_exit_thread(void)
{
    dprintf(("%ld: PyThread_exit_thread called\n",
             PyThread_get_thread_ident()));
    if (!initialized)
        exit(0);
    _endthread();
}

/*
 * Lock support.  This is implemented with an event semaphore and critical
 * sections to make it behave more like a posix mutex than its OS/2
 * counterparts.
 */

typedef struct os2_lock_t {
    int is_set;
    HEV changed;
} *type_os2_lock;

PyThread_type_lock
PyThread_allocate_lock(void)
{
#if defined(PYCC_GCC)
    _fmutex *sem = malloc(sizeof(_fmutex));
    if (!initialized)
        PyThread_init_thread();
    dprintf(("%ld: PyThread_allocate_lock() -> %lx\n",
             PyThread_get_thread_ident(),
             (long)sem));
    if (_fmutex_create(sem, 0)) {
        free(sem);
        sem = NULL;
    }
    return (PyThread_type_lock)sem;
#else
    APIRET rc;
    type_os2_lock lock = (type_os2_lock)malloc(sizeof(struct os2_lock_t));

    dprintf(("PyThread_allocate_lock called\n"));
    if (!initialized)
        PyThread_init_thread();

    lock->is_set = 0;

    DosCreateEventSem(NULL, &lock->changed, 0, 0);

    dprintf(("%ld: PyThread_allocate_lock() -> %p\n",
             PyThread_get_thread_ident(),
             lock->changed));

    return (PyThread_type_lock)lock;
#endif
}

void
PyThread_free_lock(PyThread_type_lock aLock)
{
#if !defined(PYCC_GCC)
    type_os2_lock lock = (type_os2_lock)aLock;
#endif

    dprintf(("%ld: PyThread_free_lock(%p) called\n",
             PyThread_get_thread_ident(),aLock));

#if defined(PYCC_GCC)
    if (aLock) {
        _fmutex_close((_fmutex *)aLock);
        free((_fmutex *)aLock);
    }
#else
    DosCloseEventSem(lock->changed);
    free(aLock);
#endif
}

/*
 * Return 1 on success if the lock was acquired
 *
 * and 0 if the lock was not acquired.
 */
int
PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
#if !defined(PYCC_GCC)
    int   done = 0;
    ULONG count;
    PID   pid = 0;
    TID   tid = 0;
    type_os2_lock lock = (type_os2_lock)aLock;
#endif

    dprintf(("%ld: PyThread_acquire_lock(%p, %d) called\n",
             PyThread_get_thread_ident(),
             aLock,
             waitflag));

#if defined(PYCC_GCC)
    /* always successful if the lock doesn't exist */
    if (aLock &&
        _fmutex_request((_fmutex *)aLock, waitflag ? 0 : _FMR_NOWAIT))
        return 0;
#else
    while (!done) {
        /* if the lock is currently set, we have to wait for
         * the state to change
         */
        if (lock->is_set) {
            if (!waitflag)
                return 0;
            DosWaitEventSem(lock->changed, SEM_INDEFINITE_WAIT);
        }

        /* enter a critical section and try to get the semaphore.  If
         * it is still locked, we will try again.
         */
        if (DosEnterCritSec())
            return 0;

        if (!lock->is_set) {
            lock->is_set = 1;
            DosResetEventSem(lock->changed, &count);
            done = 1;
        }

        DosExitCritSec();
    }
#endif

    return 1;
}

void
PyThread_release_lock(PyThread_type_lock aLock)
{
#if !defined(PYCC_GCC)
    type_os2_lock lock = (type_os2_lock)aLock;
#endif

    dprintf(("%ld: PyThread_release_lock(%p) called\n",
             PyThread_get_thread_ident(),
             aLock));

#if defined(PYCC_GCC)
    if (aLock)
        _fmutex_release((_fmutex *)aLock);
#else
    if (!lock->is_set) {
        dprintf(("%ld: Could not PyThread_release_lock(%p) error: %l\n",
                 PyThread_get_thread_ident(),
                 aLock,
                 GetLastError()));
        return;
    }

    if (DosEnterCritSec()) {
        dprintf(("%ld: Could not PyThread_release_lock(%p) error: %l\n",
                 PyThread_get_thread_ident(),
                 aLock,
                 GetLastError()));
        return;
    }

    lock->is_set = 0;
    DosPostEventSem(lock->changed);

    DosExitCritSec();
#endif
}

/* minimum/maximum thread stack sizes supported */
#define THREAD_MIN_STACKSIZE    0x8000          /* 32kB */
#define THREAD_MAX_STACKSIZE    0x2000000       /* 32MB */

/* set the thread stack size.
 * Return 0 if size is valid, -1 otherwise.
 */
static int
_pythread_os2_set_stacksize(size_t size)
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

#define THREAD_SET_STACKSIZE(x) _pythread_os2_set_stacksize(x)
