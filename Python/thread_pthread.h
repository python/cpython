#include "pycore_interp.h"    // _PyInterpreterState.threads.stacksize

/* Posix threads interface */

#include <stdlib.h>
#include <string.h>
#if defined(__APPLE__) || defined(HAVE_PTHREAD_DESTRUCTOR)
#define destructor xxdestructor
#endif
#ifndef HAVE_PTHREAD_STUBS
#  include <pthread.h>
#endif
#if defined(__APPLE__) || defined(HAVE_PTHREAD_DESTRUCTOR)
#undef destructor
#endif
#include <signal.h>

#if defined(__linux__)
#   include <sys/syscall.h>     /* syscall(SYS_gettid) */
#elif defined(__FreeBSD__)
#   include <pthread_np.h>      /* pthread_getthreadid_np() */
#elif defined(__OpenBSD__)
#   include <unistd.h>          /* getthrid() */
#elif defined(_AIX)
#   include <sys/thread.h>      /* thread_self() */
#elif defined(__NetBSD__)
#   include <lwp.h>             /* _lwp_self() */
#endif

/* The POSIX spec requires that use of pthread_attr_setstacksize
   be conditional on _POSIX_THREAD_ATTR_STACKSIZE being defined. */
#ifdef _POSIX_THREAD_ATTR_STACKSIZE
#ifndef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE       0       /* use default stack size */
#endif

/* The default stack size for new threads on BSD is small enough that
 * we'll get hard crashes instead of 'maximum recursion depth exceeded'
 * exceptions.
 *
 * The default stack size below is the empirically determined minimal stack
 * sizes where a simple recursive function doesn't cause a hard crash.
 *
 * For macOS the value of THREAD_STACK_SIZE is determined in configure.ac
 * as it also depends on the other configure options like chosen sanitizer
 * runtimes.
 */
#if defined(__FreeBSD__) && defined(THREAD_STACK_SIZE) && THREAD_STACK_SIZE == 0
#undef  THREAD_STACK_SIZE
#define THREAD_STACK_SIZE       0x400000
#endif
#if defined(_AIX) && defined(THREAD_STACK_SIZE) && THREAD_STACK_SIZE == 0
#undef  THREAD_STACK_SIZE
#define THREAD_STACK_SIZE       0x200000
#endif
/* bpo-38852: test_threading.test_recursion_limit() checks that 1000 recursive
   Python calls (default recursion limit) doesn't crash, but raise a regular
   RecursionError exception. In debug mode, Python function calls allocates
   more memory on the stack, so use a stack of 8 MiB. */
#if defined(__ANDROID__) && defined(THREAD_STACK_SIZE) && THREAD_STACK_SIZE == 0
#   ifdef Py_DEBUG
#   undef  THREAD_STACK_SIZE
#   define THREAD_STACK_SIZE    0x800000
#   endif
#endif
#if defined(__VXWORKS__) && defined(THREAD_STACK_SIZE) && THREAD_STACK_SIZE == 0
#undef  THREAD_STACK_SIZE
#define THREAD_STACK_SIZE       0x100000
#endif
/* for safety, ensure a viable minimum stacksize */
#define THREAD_STACK_MIN        0x8000  /* 32 KiB */
#else  /* !_POSIX_THREAD_ATTR_STACKSIZE */
#ifdef THREAD_STACK_SIZE
#error "THREAD_STACK_SIZE defined but _POSIX_THREAD_ATTR_STACKSIZE undefined"
#endif
#endif

/* The POSIX spec says that implementations supporting the sem_*
   family of functions must indicate this by defining
   _POSIX_SEMAPHORES. */
#ifdef _POSIX_SEMAPHORES
/* On FreeBSD 4.x, _POSIX_SEMAPHORES is defined empty, so
   we need to add 0 to make it work there as well. */
#if (_POSIX_SEMAPHORES+0) == -1
#define HAVE_BROKEN_POSIX_SEMAPHORES
#else
#include <semaphore.h>
#include <errno.h>
#endif
#endif


/* Whether or not to use semaphores directly rather than emulating them with
 * mutexes and condition variables:
 */
#if (defined(_POSIX_SEMAPHORES) && !defined(HAVE_BROKEN_POSIX_SEMAPHORES) && \
     (defined(HAVE_SEM_TIMEDWAIT) || defined(HAVE_SEM_CLOCKWAIT)))
#  define USE_SEMAPHORES
#else
#  undef USE_SEMAPHORES
#endif


/* On platforms that don't use standard POSIX threads pthread_sigmask()
 * isn't present.  DEC threads uses sigprocmask() instead as do most
 * other UNIX International compliant systems that don't have the full
 * pthread implementation.
 */
#if defined(HAVE_PTHREAD_SIGMASK) && !defined(HAVE_BROKEN_PTHREAD_SIGMASK)
#  define SET_THREAD_SIGMASK pthread_sigmask
#else
#  define SET_THREAD_SIGMASK sigprocmask
#endif


#define MICROSECONDS_TO_TIMESPEC(microseconds, ts) \
do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    tv.tv_usec += microseconds % 1000000; \
    tv.tv_sec += microseconds / 1000000; \
    tv.tv_sec += tv.tv_usec / 1000000; \
    tv.tv_usec %= 1000000; \
    ts.tv_sec = tv.tv_sec; \
    ts.tv_nsec = tv.tv_usec * 1000; \
} while(0)


/*
 * pthread_cond support
 */

#if defined(HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
// monotonic is supported statically.  It doesn't mean it works on runtime.
#define CONDATTR_MONOTONIC
#endif

// NULL when pthread_condattr_setclock(CLOCK_MONOTONIC) is not supported.
static pthread_condattr_t *condattr_monotonic = NULL;

static void
init_condattr(void)
{
#ifdef CONDATTR_MONOTONIC
    static pthread_condattr_t ca;
    pthread_condattr_init(&ca);
    if (pthread_condattr_setclock(&ca, CLOCK_MONOTONIC) == 0) {
        condattr_monotonic = &ca;  // Use monotonic clock
    }
#endif
}

int
_PyThread_cond_init(PyCOND_T *cond)
{
    return pthread_cond_init(cond, condattr_monotonic);
}

void
_PyThread_cond_after(long long us, struct timespec *abs)
{
#ifdef CONDATTR_MONOTONIC
    if (condattr_monotonic) {
        clock_gettime(CLOCK_MONOTONIC, abs);
        abs->tv_sec  += us / 1000000;
        abs->tv_nsec += (us % 1000000) * 1000;
        abs->tv_sec  += abs->tv_nsec / 1000000000;
        abs->tv_nsec %= 1000000000;
        return;
    }
#endif

    struct timespec ts;
    MICROSECONDS_TO_TIMESPEC(us, ts);
    *abs = ts;
}


/* A pthread mutex isn't sufficient to model the Python lock type
 * because, according to Draft 5 of the docs (P1003.4a/D5), both of the
 * following are undefined:
 *  -> a thread tries to lock a mutex it already has locked
 *  -> a thread tries to unlock a mutex locked by a different thread
 * pthread mutexes are designed for serializing threads over short pieces
 * of code anyway, so wouldn't be an appropriate implementation of
 * Python's locks regardless.
 *
 * The pthread_lock struct implements a Python lock as a "locked?" bit
 * and a <condition, mutex> pair.  In general, if the bit can be acquired
 * instantly, it is, else the pair is used to block the thread until the
 * bit is cleared.     9 May 1994 tim@ksr.com
 */

typedef struct {
    char             locked; /* 0=unlocked, 1=locked */
    /* a <cond, mutex> pair to handle an acquire of a locked lock */
    pthread_cond_t   lock_released;
    pthread_mutex_t  mut;
} pthread_lock;

#define CHECK_STATUS(name)  if (status != 0) { perror(name); error = 1; }
#define CHECK_STATUS_PTHREAD(name)  if (status != 0) { fprintf(stderr, \
    "%s: %s\n", name, strerror(status)); error = 1; }

/*
 * Initialization.
 */
static void
PyThread__init_thread(void)
{
#if defined(_AIX) && defined(__GNUC__)
    extern void pthread_init(void);
    pthread_init();
#endif
    init_condattr();
}

/*
 * Thread support.
 */

/* bpo-33015: pythread_callback struct and pythread_wrapper() cast
   "void func(void *)" to "void* func(void *)": always return NULL.

   PyThread_start_new_thread() uses "void func(void *)" type, whereas
   pthread_create() requires a void* return value. */
typedef struct {
    void (*func) (void *);
    void *arg;
} pythread_callback;

static void *
pythread_wrapper(void *arg)
{
    /* copy func and func_arg and free the temporary structure */
    pythread_callback *callback = arg;
    void (*func)(void *) = callback->func;
    void *func_arg = callback->arg;
    PyMem_RawFree(arg);

    func(func_arg);
    return NULL;
}

unsigned long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    pthread_t th;
    int status;
#if defined(THREAD_STACK_SIZE) || defined(PTHREAD_SYSTEM_SCHED_SUPPORTED)
    pthread_attr_t attrs;
#endif
#if defined(THREAD_STACK_SIZE)
    size_t      tss;
#endif

    dprintf(("PyThread_start_new_thread called\n"));
    if (!initialized)
        PyThread_init_thread();

#if defined(THREAD_STACK_SIZE) || defined(PTHREAD_SYSTEM_SCHED_SUPPORTED)
    if (pthread_attr_init(&attrs) != 0)
        return PYTHREAD_INVALID_THREAD_ID;
#endif
#if defined(THREAD_STACK_SIZE)
    PyThreadState *tstate = _PyThreadState_GET();
    size_t stacksize = tstate ? tstate->interp->threads.stacksize : 0;
    tss = (stacksize != 0) ? stacksize : THREAD_STACK_SIZE;
    if (tss != 0) {
        if (pthread_attr_setstacksize(&attrs, tss) != 0) {
            pthread_attr_destroy(&attrs);
            return PYTHREAD_INVALID_THREAD_ID;
        }
    }
#endif
#if defined(PTHREAD_SYSTEM_SCHED_SUPPORTED)
    pthread_attr_setscope(&attrs, PTHREAD_SCOPE_SYSTEM);
#endif

    pythread_callback *callback = PyMem_RawMalloc(sizeof(pythread_callback));

    if (callback == NULL) {
      return PYTHREAD_INVALID_THREAD_ID;
    }

    callback->func = func;
    callback->arg = arg;

    status = pthread_create(&th,
#if defined(THREAD_STACK_SIZE) || defined(PTHREAD_SYSTEM_SCHED_SUPPORTED)
                             &attrs,
#else
                             (pthread_attr_t*)NULL,
#endif
                             pythread_wrapper, callback);

#if defined(THREAD_STACK_SIZE) || defined(PTHREAD_SYSTEM_SCHED_SUPPORTED)
    pthread_attr_destroy(&attrs);
#endif

    if (status != 0) {
        PyMem_RawFree(callback);
        return PYTHREAD_INVALID_THREAD_ID;
    }

    pthread_detach(th);

#if SIZEOF_PTHREAD_T <= SIZEOF_LONG
    return (unsigned long) th;
#else
    return (unsigned long) *(unsigned long *) &th;
#endif
}

/* XXX This implementation is considered (to quote Tim Peters) "inherently
   hosed" because:
     - It does not guarantee the promise that a non-zero integer is returned.
     - The cast to unsigned long is inherently unsafe.
     - It is not clear that the 'volatile' (for AIX?) are any longer necessary.
*/
unsigned long
PyThread_get_thread_ident(void)
{
    volatile pthread_t threadid;
    if (!initialized)
        PyThread_init_thread();
    threadid = pthread_self();
    return (unsigned long) threadid;
}

#ifdef PY_HAVE_THREAD_NATIVE_ID
unsigned long
PyThread_get_thread_native_id(void)
{
    if (!initialized)
        PyThread_init_thread();
#ifdef __APPLE__
    uint64_t native_id;
    (void) pthread_threadid_np(NULL, &native_id);
#elif defined(__linux__)
    pid_t native_id;
    native_id = syscall(SYS_gettid);
#elif defined(__FreeBSD__)
    int native_id;
    native_id = pthread_getthreadid_np();
#elif defined(__OpenBSD__)
    pid_t native_id;
    native_id = getthrid();
#elif defined(_AIX)
    tid_t native_id;
    native_id = thread_self();
#elif defined(__NetBSD__)
    lwpid_t native_id;
    native_id = _lwp_self();
#endif
    return (unsigned long) native_id;
}
#endif

void _Py_NO_RETURN
PyThread_exit_thread(void)
{
    dprintf(("PyThread_exit_thread called\n"));
    if (!initialized)
        exit(0);
    pthread_exit(0);
}

#ifdef USE_SEMAPHORES

/*
 * Lock support.
 */

PyThread_type_lock
PyThread_allocate_lock(void)
{
    sem_t *lock;
    int status, error = 0;

    dprintf(("PyThread_allocate_lock called\n"));
    if (!initialized)
        PyThread_init_thread();

    lock = (sem_t *)PyMem_RawMalloc(sizeof(sem_t));

    if (lock) {
        status = sem_init(lock,0,1);
        CHECK_STATUS("sem_init");

        if (error) {
            PyMem_RawFree((void *)lock);
            lock = NULL;
        }
    }

    dprintf(("PyThread_allocate_lock() -> %p\n", (void *)lock));
    return (PyThread_type_lock)lock;
}

void
PyThread_free_lock(PyThread_type_lock lock)
{
    sem_t *thelock = (sem_t *)lock;
    int status, error = 0;

    (void) error; /* silence unused-but-set-variable warning */
    dprintf(("PyThread_free_lock(%p) called\n", lock));

    if (!thelock)
        return;

    status = sem_destroy(thelock);
    CHECK_STATUS("sem_destroy");

    PyMem_RawFree((void *)thelock);
}

/*
 * As of February 2002, Cygwin thread implementations mistakenly report error
 * codes in the return value of the sem_ calls (like the pthread_ functions).
 * Correct implementations return -1 and put the code in errno. This supports
 * either.
 */
static int
fix_status(int status)
{
    return (status == -1) ? errno : status;
}

PyLockStatus
PyThread_acquire_lock_timed(PyThread_type_lock lock, PY_TIMEOUT_T microseconds,
                            int intr_flag)
{
    PyLockStatus success;
    sem_t *thelock = (sem_t *)lock;
    int status, error = 0;

    (void) error; /* silence unused-but-set-variable warning */
    dprintf(("PyThread_acquire_lock_timed(%p, %lld, %d) called\n",
             lock, microseconds, intr_flag));

    _PyTime_t timeout;  // relative timeout
    if (microseconds >= 0) {
        _PyTime_t ns;
        if (microseconds <= _PyTime_MAX / 1000) {
            ns = microseconds * 1000;
        }
        else {
            // bpo-41710: PyThread_acquire_lock_timed() cannot report timeout
            // overflow to the caller, so clamp the timeout to
            // [_PyTime_MIN, _PyTime_MAX].
            //
            // _PyTime_MAX nanoseconds is around 292.3 years.
            //
            // _thread.Lock.acquire() and _thread.RLock.acquire() raise an
            // OverflowError if microseconds is greater than PY_TIMEOUT_MAX.
            ns = _PyTime_MAX;
        }
        timeout = _PyTime_FromNanoseconds(ns);
    }
    else {
        timeout = _PyTime_FromNanoseconds(-1);
    }

#ifdef HAVE_SEM_CLOCKWAIT
    struct timespec abs_timeout;
    // Local scope for deadline
    {
        _PyTime_t deadline = _PyTime_Add(_PyTime_GetMonotonicClock(), timeout);
        _PyTime_AsTimespec_clamp(deadline, &abs_timeout);
    }
#else
    _PyTime_t deadline = 0;
    if (timeout > 0 && !intr_flag) {
        deadline = _PyDeadline_Init(timeout);
    }
#endif

    while (1) {
        if (timeout > 0) {
#ifdef HAVE_SEM_CLOCKWAIT
            status = fix_status(sem_clockwait(thelock, CLOCK_MONOTONIC,
                                              &abs_timeout));
#else
            _PyTime_t abs_time = _PyTime_Add(_PyTime_GetSystemClock(),
                                             timeout);
            struct timespec ts;
            _PyTime_AsTimespec_clamp(abs_time, &ts);
            status = fix_status(sem_timedwait(thelock, &ts));
#endif
        }
        else if (timeout == 0) {
            status = fix_status(sem_trywait(thelock));
        }
        else {
            status = fix_status(sem_wait(thelock));
        }

        /* Retry if interrupted by a signal, unless the caller wants to be
           notified.  */
        if (intr_flag || status != EINTR) {
            break;
        }

        // sem_clockwait() uses an absolute timeout, there is no need
        // to recompute the relative timeout.
#ifndef HAVE_SEM_CLOCKWAIT
        if (timeout > 0) {
            /* wait interrupted by a signal (EINTR): recompute the timeout */
            timeout = _PyDeadline_Get(deadline);
            if (timeout < 0) {
                status = ETIMEDOUT;
                break;
            }
        }
#endif
    }

    /* Don't check the status if we're stopping because of an interrupt.  */
    if (!(intr_flag && status == EINTR)) {
        if (timeout > 0) {
            if (status != ETIMEDOUT) {
#ifdef HAVE_SEM_CLOCKWAIT
                CHECK_STATUS("sem_clockwait");
#else
                CHECK_STATUS("sem_timedwait");
#endif
            }
        }
        else if (timeout == 0) {
            if (status != EAGAIN) {
                CHECK_STATUS("sem_trywait");
            }
        }
        else {
            CHECK_STATUS("sem_wait");
        }
    }

    if (status == 0) {
        success = PY_LOCK_ACQUIRED;
    } else if (intr_flag && status == EINTR) {
        success = PY_LOCK_INTR;
    } else {
        success = PY_LOCK_FAILURE;
    }

    dprintf(("PyThread_acquire_lock_timed(%p, %lld, %d) -> %d\n",
             lock, microseconds, intr_flag, success));
    return success;
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
    sem_t *thelock = (sem_t *)lock;
    int status, error = 0;

    (void) error; /* silence unused-but-set-variable warning */
    dprintf(("PyThread_release_lock(%p) called\n", lock));

    status = sem_post(thelock);
    CHECK_STATUS("sem_post");
}

#else /* USE_SEMAPHORES */

/*
 * Lock support.
 */
PyThread_type_lock
PyThread_allocate_lock(void)
{
    pthread_lock *lock;
    int status, error = 0;

    dprintf(("PyThread_allocate_lock called\n"));
    if (!initialized)
        PyThread_init_thread();

    lock = (pthread_lock *) PyMem_RawCalloc(1, sizeof(pthread_lock));
    if (lock) {
        lock->locked = 0;

        status = pthread_mutex_init(&lock->mut, NULL);
        CHECK_STATUS_PTHREAD("pthread_mutex_init");
        /* Mark the pthread mutex underlying a Python mutex as
           pure happens-before.  We can't simply mark the
           Python-level mutex as a mutex because it can be
           acquired and released in different threads, which
           will cause errors. */
        _Py_ANNOTATE_PURE_HAPPENS_BEFORE_MUTEX(&lock->mut);

        status = _PyThread_cond_init(&lock->lock_released);
        CHECK_STATUS_PTHREAD("pthread_cond_init");

        if (error) {
            PyMem_RawFree((void *)lock);
            lock = 0;
        }
    }

    dprintf(("PyThread_allocate_lock() -> %p\n", (void *)lock));
    return (PyThread_type_lock) lock;
}

void
PyThread_free_lock(PyThread_type_lock lock)
{
    pthread_lock *thelock = (pthread_lock *)lock;
    int status, error = 0;

    (void) error; /* silence unused-but-set-variable warning */
    dprintf(("PyThread_free_lock(%p) called\n", lock));

    /* some pthread-like implementations tie the mutex to the cond
     * and must have the cond destroyed first.
     */
    status = pthread_cond_destroy( &thelock->lock_released );
    CHECK_STATUS_PTHREAD("pthread_cond_destroy");

    status = pthread_mutex_destroy( &thelock->mut );
    CHECK_STATUS_PTHREAD("pthread_mutex_destroy");

    PyMem_RawFree((void *)thelock);
}

PyLockStatus
PyThread_acquire_lock_timed(PyThread_type_lock lock, PY_TIMEOUT_T microseconds,
                            int intr_flag)
{
    PyLockStatus success = PY_LOCK_FAILURE;
    pthread_lock *thelock = (pthread_lock *)lock;
    int status, error = 0;

    dprintf(("PyThread_acquire_lock_timed(%p, %lld, %d) called\n",
             lock, microseconds, intr_flag));

    if (microseconds == 0) {
        status = pthread_mutex_trylock( &thelock->mut );
        if (status != EBUSY)
            CHECK_STATUS_PTHREAD("pthread_mutex_trylock[1]");
    }
    else {
        status = pthread_mutex_lock( &thelock->mut );
        CHECK_STATUS_PTHREAD("pthread_mutex_lock[1]");
    }
    if (status == 0) {
        if (thelock->locked == 0) {
            success = PY_LOCK_ACQUIRED;
        }
        else if (microseconds != 0) {
            struct timespec abs;
            if (microseconds > 0) {
                _PyThread_cond_after(microseconds, &abs);
            }
            /* continue trying until we get the lock */

            /* mut must be locked by me -- part of the condition
             * protocol */
            while (success == PY_LOCK_FAILURE) {
                if (microseconds > 0) {
                    status = pthread_cond_timedwait(
                        &thelock->lock_released,
                        &thelock->mut, &abs);
                    if (status == 1) {
                        break;
                    }
                    if (status == ETIMEDOUT)
                        break;
                    CHECK_STATUS_PTHREAD("pthread_cond_timedwait");
                }
                else {
                    status = pthread_cond_wait(
                        &thelock->lock_released,
                        &thelock->mut);
                    CHECK_STATUS_PTHREAD("pthread_cond_wait");
                }

                if (intr_flag && status == 0 && thelock->locked) {
                    /* We were woken up, but didn't get the lock.  We probably received
                     * a signal.  Return PY_LOCK_INTR to allow the caller to handle
                     * it and retry.  */
                    success = PY_LOCK_INTR;
                    break;
                }
                else if (status == 0 && !thelock->locked) {
                    success = PY_LOCK_ACQUIRED;
                }
            }
        }
        if (success == PY_LOCK_ACQUIRED) thelock->locked = 1;
        status = pthread_mutex_unlock( &thelock->mut );
        CHECK_STATUS_PTHREAD("pthread_mutex_unlock[1]");
    }

    if (error) success = PY_LOCK_FAILURE;
    dprintf(("PyThread_acquire_lock_timed(%p, %lld, %d) -> %d\n",
             lock, microseconds, intr_flag, success));
    return success;
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
    pthread_lock *thelock = (pthread_lock *)lock;
    int status, error = 0;

    (void) error; /* silence unused-but-set-variable warning */
    dprintf(("PyThread_release_lock(%p) called\n", lock));

    status = pthread_mutex_lock( &thelock->mut );
    CHECK_STATUS_PTHREAD("pthread_mutex_lock[3]");

    thelock->locked = 0;

    /* wake up someone (anyone, if any) waiting on the lock */
    status = pthread_cond_signal( &thelock->lock_released );
    CHECK_STATUS_PTHREAD("pthread_cond_signal");

    status = pthread_mutex_unlock( &thelock->mut );
    CHECK_STATUS_PTHREAD("pthread_mutex_unlock[3]");
}

#endif /* USE_SEMAPHORES */

int
_PyThread_at_fork_reinit(PyThread_type_lock *lock)
{
    PyThread_type_lock new_lock = PyThread_allocate_lock();
    if (new_lock == NULL) {
        return -1;
    }

    /* bpo-6721, bpo-40089: The old lock can be in an inconsistent state.
       fork() can be called in the middle of an operation on the lock done by
       another thread. So don't call PyThread_free_lock(*lock).

       Leak memory on purpose. Don't release the memory either since the
       address of a mutex is relevant. Putting two mutexes at the same address
       can lead to problems. */

    *lock = new_lock;
    return 0;
}

int
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
    return PyThread_acquire_lock_timed(lock, waitflag ? -1 : 0, /*intr_flag=*/0);
}

/* set the thread stack size.
 * Return 0 if size is valid, -1 if size is invalid,
 * -2 if setting stack size is not supported.
 */
static int
_pythread_pthread_set_stacksize(size_t size)
{
#if defined(THREAD_STACK_SIZE)
    pthread_attr_t attrs;
    size_t tss_min;
    int rc = 0;
#endif

    /* set to default */
    if (size == 0) {
        _PyInterpreterState_GET()->threads.stacksize = 0;
        return 0;
    }

#if defined(THREAD_STACK_SIZE)
#if defined(PTHREAD_STACK_MIN)
    tss_min = PTHREAD_STACK_MIN > THREAD_STACK_MIN ? PTHREAD_STACK_MIN
                                                   : THREAD_STACK_MIN;
#else
    tss_min = THREAD_STACK_MIN;
#endif
    if (size >= tss_min) {
        /* validate stack size by setting thread attribute */
        if (pthread_attr_init(&attrs) == 0) {
            rc = pthread_attr_setstacksize(&attrs, size);
            pthread_attr_destroy(&attrs);
            if (rc == 0) {
                _PyInterpreterState_GET()->threads.stacksize = size;
                return 0;
            }
        }
    }
    return -1;
#else
    return -2;
#endif
}

#define THREAD_SET_STACKSIZE(x) _pythread_pthread_set_stacksize(x)


/* Thread Local Storage (TLS) API

   This API is DEPRECATED since Python 3.7.  See PEP 539 for details.
*/

/* Issue #25658: On platforms where native TLS key is defined in a way that
   cannot be safely cast to int, PyThread_create_key returns immediately a
   failure status and other TLS functions all are no-ops.  This indicates
   clearly that the old API is not supported on platforms where it cannot be
   used reliably, and that no effort will be made to add such support.

   Note: PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT will be unnecessary after
   removing this API.
*/

int
PyThread_create_key(void)
{
#ifdef PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT
    pthread_key_t key;
    int fail = pthread_key_create(&key, NULL);
    if (fail)
        return -1;
    if (key > INT_MAX) {
        /* Issue #22206: handle integer overflow */
        pthread_key_delete(key);
        errno = ENOMEM;
        return -1;
    }
    return (int)key;
#else
    return -1;  /* never return valid key value. */
#endif
}

void
PyThread_delete_key(int key)
{
#ifdef PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT
    pthread_key_delete(key);
#endif
}

void
PyThread_delete_key_value(int key)
{
#ifdef PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT
    pthread_setspecific(key, NULL);
#endif
}

int
PyThread_set_key_value(int key, void *value)
{
#ifdef PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT
    int fail = pthread_setspecific(key, value);
    return fail ? -1 : 0;
#else
    return -1;
#endif
}

void *
PyThread_get_key_value(int key)
{
#ifdef PTHREAD_KEY_T_IS_COMPATIBLE_WITH_INT
    return pthread_getspecific(key);
#else
    return NULL;
#endif
}


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

    int fail = pthread_key_create(&(key->_key), NULL);
    if (fail) {
        return -1;
    }
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

    pthread_key_delete(key->_key);
    /* pthread has not provided the defined invalid value for the key. */
    key->_is_initialized = 0;
}

int
PyThread_tss_set(Py_tss_t *key, void *value)
{
    assert(key != NULL);
    int fail = pthread_setspecific(key->_key, value);
    return fail ? -1 : 0;
}

void *
PyThread_tss_get(Py_tss_t *key)
{
    assert(key != NULL);
    return pthread_getspecific(key->_key);
}
