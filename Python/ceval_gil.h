/*
 * Implementation of the Global Interpreter Lock (GIL).
 */

#include <stdlib.h>
#include <errno.h>


/* First some general settings */

/* microseconds (the Python API uses seconds, though) */
#define DEFAULT_INTERVAL 5000
static unsigned long gil_interval = DEFAULT_INTERVAL;
#define INTERVAL (gil_interval >= 1 ? gil_interval : 1)

/* Enable if you want to force the switching of threads at least every `gil_interval` */
#undef FORCE_SWITCHING
#define FORCE_SWITCHING


/*
   Notes about the implementation:

   - The GIL is just a boolean variable (gil_locked) whose access is protected
     by a mutex (gil_mutex), and whose changes are signalled by a condition
     variable (gil_cond). gil_mutex is taken for short periods of time,
     and therefore mostly uncontended.

   - In the GIL-holding thread, the main loop (PyEval_EvalFrameEx) must be
     able to release the GIL on demand by another thread. A volatile boolean
     variable (gil_drop_request) is used for that purpose, which is checked
     at every turn of the eval loop. That variable is set after a wait of
     `interval` microseconds on `gil_cond` has timed out.
      
      [Actually, another volatile boolean variable (eval_breaker) is used
       which ORs several conditions into one. Volatile booleans are
       sufficient as inter-thread signalling means since Python is run
       on cache-coherent architectures only.]

   - A thread wanting to take the GIL will first let pass a given amount of
     time (`interval` microseconds) before setting gil_drop_request. This
     encourages a defined switching period, but doesn't enforce it since
     opcodes can take an arbitrary time to execute.
 
     The `interval` value is available for the user to read and modify
     using the Python API `sys.{get,set}switchinterval()`.

   - When a thread releases the GIL and gil_drop_request is set, that thread
     ensures that another GIL-awaiting thread gets scheduled.
     It does so by waiting on a condition variable (switch_cond) until
     the value of gil_last_holder is changed to something else than its
     own thread state pointer, indicating that another thread was able to
     take the GIL.
 
     This is meant to prohibit the latency-adverse behaviour on multi-core
     machines where one thread would speculatively release the GIL, but still
     run and end up being the first to re-acquire it, making the "timeslices"
     much longer than expected.
     (Note: this mechanism is enabled with FORCE_SWITCHING above)
*/

#ifndef _POSIX_THREADS
/* This means pthreads are not implemented in libc headers, hence the macro
   not present in unistd.h. But they still can be implemented as an external
   library (e.g. gnu pth in pthread emulation) */
# ifdef HAVE_PTHREAD_H
#  include <pthread.h> /* _POSIX_THREADS */
# endif
#endif


#ifdef _POSIX_THREADS

/*
 * POSIX support
 */

#include <pthread.h>

#define ADD_MICROSECONDS(tv, interval) \
do { \
    tv.tv_usec += (long) interval; \
    tv.tv_sec += tv.tv_usec / 1000000; \
    tv.tv_usec %= 1000000; \
} while (0)

/* We assume all modern POSIX systems have gettimeofday() */
#ifdef GETTIMEOFDAY_NO_TZ
#define GETTIMEOFDAY(ptv) gettimeofday(ptv)
#else
#define GETTIMEOFDAY(ptv) gettimeofday(ptv, (struct timezone *)NULL)
#endif

#define MUTEX_T pthread_mutex_t
#define MUTEX_INIT(mut) \
    if (pthread_mutex_init(&mut, NULL)) { \
        Py_FatalError("pthread_mutex_init(" #mut ") failed"); };
#define MUTEX_FINI(mut) \
    if (pthread_mutex_destroy(&mut)) { \
        Py_FatalError("pthread_mutex_destroy(" #mut ") failed"); };
#define MUTEX_LOCK(mut) \
    if (pthread_mutex_lock(&mut)) { \
        Py_FatalError("pthread_mutex_lock(" #mut ") failed"); };
#define MUTEX_UNLOCK(mut) \
    if (pthread_mutex_unlock(&mut)) { \
        Py_FatalError("pthread_mutex_unlock(" #mut ") failed"); };

#define COND_T pthread_cond_t
#define COND_INIT(cond) \
    if (pthread_cond_init(&cond, NULL)) { \
        Py_FatalError("pthread_cond_init(" #cond ") failed"); };
#define COND_FINI(cond) \
    if (pthread_cond_destroy(&cond)) { \
        Py_FatalError("pthread_cond_destroy(" #cond ") failed"); };
#define COND_SIGNAL(cond) \
    if (pthread_cond_signal(&cond)) { \
        Py_FatalError("pthread_cond_signal(" #cond ") failed"); };
#define COND_WAIT(cond, mut) \
    if (pthread_cond_wait(&cond, &mut)) { \
        Py_FatalError("pthread_cond_wait(" #cond ") failed"); };
#define COND_TIMED_WAIT(cond, mut, microseconds, timeout_result) \
    { \
        int r; \
        struct timespec ts; \
        struct timeval deadline; \
        \
        GETTIMEOFDAY(&deadline); \
        ADD_MICROSECONDS(deadline, microseconds); \
        ts.tv_sec = deadline.tv_sec; \
        ts.tv_nsec = deadline.tv_usec * 1000; \
        \
        r = pthread_cond_timedwait(&cond, &mut, &ts); \
        if (r == ETIMEDOUT) \
            timeout_result = 1; \
        else if (r) \
            Py_FatalError("pthread_cond_timedwait(" #cond ") failed"); \
        else \
            timeout_result = 0; \
    } \

#elif defined(NT_THREADS)

/*
 * Windows (2000 and later, as well as (hopefully) CE) support
 */

#include <windows.h>

#define MUTEX_T CRITICAL_SECTION
#define MUTEX_INIT(mut) do { \
    if (!(InitializeCriticalSectionAndSpinCount(&(mut), 4000))) \
        Py_FatalError("CreateMutex(" #mut ") failed"); \
} while (0)
#define MUTEX_FINI(mut) \
    DeleteCriticalSection(&(mut))
#define MUTEX_LOCK(mut) \
    EnterCriticalSection(&(mut))
#define MUTEX_UNLOCK(mut) \
    LeaveCriticalSection(&(mut))

/* We emulate condition variables with a semaphore.
   We use a Semaphore rather than an auto-reset event, because although
   an auto-resent event might appear to solve the lost-wakeup bug (race
   condition between releasing the outer lock and waiting) because it
   maintains state even though a wait hasn't happened, there is still
   a lost wakeup problem if more than one thread are interrupted in the
   critical place.  A semaphore solves that.
   Because it is ok to signal a condition variable with no one
   waiting, we need to keep track of the number of
   waiting threads.  Otherwise, the semaphore's state could rise
   without bound.

   Generic emulations of the pthread_cond_* API using
   Win32 functions can be found on the Web.
   The following read can be edificating (or not):
   http://www.cse.wustl.edu/~schmidt/win32-cv-1.html
*/
typedef struct COND_T
{
    HANDLE sem;    /* the semaphore */
    int n_waiting; /* how many are unreleased */
} COND_T;

__inline static void _cond_init(COND_T *cond)
{
    /* A semaphore with a large max value,  The positive value
     * is only needed to catch those "lost wakeup" events and
     * race conditions when a timed wait elapses.
     */
    if (!(cond->sem = CreateSemaphore(NULL, 0, 1000, NULL)))
        Py_FatalError("CreateSemaphore() failed");
    cond->n_waiting = 0;
}

__inline static void _cond_fini(COND_T *cond)
{
    BOOL ok = CloseHandle(cond->sem);
    if (!ok)
        Py_FatalError("CloseHandle() failed");
}

__inline static void _cond_wait(COND_T *cond, MUTEX_T *mut)
{
    ++cond->n_waiting;
    MUTEX_UNLOCK(*mut);
    /* "lost wakeup bug" would occur if the caller were interrupted here,
     * but we are safe because we are using a semaphore wich has an internal
     * count.
     */
    if (WaitForSingleObject(cond->sem, INFINITE) == WAIT_FAILED)
        Py_FatalError("WaitForSingleObject() failed");
    MUTEX_LOCK(*mut);
}

__inline static int _cond_timed_wait(COND_T *cond, MUTEX_T *mut,
                              int us)
{
    DWORD r;
    ++cond->n_waiting;
    MUTEX_UNLOCK(*mut);
    r = WaitForSingleObject(cond->sem, us / 1000);
    if (r == WAIT_FAILED)
        Py_FatalError("WaitForSingleObject() failed");
    MUTEX_LOCK(*mut);
    if (r == WAIT_TIMEOUT)
        --cond->n_waiting;
        /* Here we have a benign race condition with _cond_signal.  If the
         * wait operation has timed out, but before we can acquire the
         * mutex again to decrement n_waiting, a thread holding the mutex
         * still sees a positive n_waiting value and may call
         * ReleaseSemaphore and decrement n_waiting.
         * This will cause n_waiting to be decremented twice.
         * This is benign, though, because ReleaseSemaphore will also have
         * been called, leaving the semaphore state positive.  We may
         * thus end up with semaphore in state 1, and n_waiting == -1, and
         * the next time someone calls _cond_wait(), that thread will
         * pass right through, decrementing the semaphore state and
         * incrementing n_waiting, thus correcting the extra _cond_signal.
         */
    return r == WAIT_TIMEOUT;
}

__inline static void _cond_signal(COND_T  *cond) {
    /* NOTE: This must be called with the mutex held */
    if (cond->n_waiting > 0) {
        if (!ReleaseSemaphore(cond->sem, 1, NULL))
            Py_FatalError("ReleaseSemaphore() failed");
        --cond->n_waiting;
    }
}

#define COND_INIT(cond) \
    _cond_init(&(cond))
#define COND_FINI(cond) \
    _cond_fini(&(cond))
#define COND_SIGNAL(cond) \
    _cond_signal(&(cond))
#define COND_WAIT(cond, mut) \
    _cond_wait(&(cond), &(mut))
#define COND_TIMED_WAIT(cond, mut, us, timeout_result) do { \
    (timeout_result) = _cond_timed_wait(&(cond), &(mut), us); \
} while (0)

#else

#error You need either a POSIX-compatible or a Windows system!

#endif /* _POSIX_THREADS, NT_THREADS */


/* Whether the GIL is already taken (-1 if uninitialized). This is atomic
   because it can be read without any lock taken in ceval.c. */
static _Py_atomic_int gil_locked = {-1};
/* Number of GIL switches since the beginning. */
static unsigned long gil_switch_number = 0;
/* Last PyThreadState holding / having held the GIL. This helps us know
   whether anyone else was scheduled after we dropped the GIL. */
static _Py_atomic_address gil_last_holder = {NULL};

/* This condition variable allows one or several threads to wait until
   the GIL is released. In addition, the mutex also protects the above
   variables. */
static COND_T gil_cond;
static MUTEX_T gil_mutex;

#ifdef FORCE_SWITCHING
/* This condition variable helps the GIL-releasing thread wait for
   a GIL-awaiting thread to be scheduled and take the GIL. */
static COND_T switch_cond;
static MUTEX_T switch_mutex;
#endif


static int gil_created(void)
{
    return _Py_atomic_load_explicit(&gil_locked, _Py_memory_order_acquire) >= 0;
}

static void create_gil(void)
{
    MUTEX_INIT(gil_mutex);
#ifdef FORCE_SWITCHING
    MUTEX_INIT(switch_mutex);
#endif
    COND_INIT(gil_cond);
#ifdef FORCE_SWITCHING
    COND_INIT(switch_cond);
#endif
    _Py_atomic_store_relaxed(&gil_last_holder, NULL);
    _Py_ANNOTATE_RWLOCK_CREATE(&gil_locked);
    _Py_atomic_store_explicit(&gil_locked, 0, _Py_memory_order_release);
}

static void destroy_gil(void)
{
    MUTEX_FINI(gil_mutex);
#ifdef FORCE_SWITCHING
    MUTEX_FINI(switch_mutex);
#endif
    COND_FINI(gil_cond);
#ifdef FORCE_SWITCHING
    COND_FINI(switch_cond);
#endif
    _Py_atomic_store_explicit(&gil_locked, -1, _Py_memory_order_release);
    _Py_ANNOTATE_RWLOCK_DESTROY(&gil_locked);
}

static void recreate_gil(void)
{
    _Py_ANNOTATE_RWLOCK_DESTROY(&gil_locked);
    /* XXX should we destroy the old OS resources here? */
    create_gil();
}

static void drop_gil(PyThreadState *tstate)
{
    if (!_Py_atomic_load_relaxed(&gil_locked))
        Py_FatalError("drop_gil: GIL is not locked");
    /* tstate is allowed to be NULL (early interpreter init) */
    if (tstate != NULL) {
        /* Sub-interpreter support: threads might have been switched
           under our feet using PyThreadState_Swap(). Fix the GIL last
           holder variable so that our heuristics work. */
        _Py_atomic_store_relaxed(&gil_last_holder, tstate);
    }

    MUTEX_LOCK(gil_mutex);
    _Py_ANNOTATE_RWLOCK_RELEASED(&gil_locked, /*is_write=*/1);
    _Py_atomic_store_relaxed(&gil_locked, 0);
    COND_SIGNAL(gil_cond);
    MUTEX_UNLOCK(gil_mutex);
    
#ifdef FORCE_SWITCHING
    if (_Py_atomic_load_relaxed(&gil_drop_request) && tstate != NULL) {
        MUTEX_LOCK(switch_mutex);
        /* Not switched yet => wait */
        if (_Py_atomic_load_relaxed(&gil_last_holder) == tstate) {
	    RESET_GIL_DROP_REQUEST();
            /* NOTE: if COND_WAIT does not atomically start waiting when
               releasing the mutex, another thread can run through, take
               the GIL and drop it again, and reset the condition
               before we even had a chance to wait for it. */
            COND_WAIT(switch_cond, switch_mutex);
	}
        MUTEX_UNLOCK(switch_mutex);
    }
#endif
}

static void take_gil(PyThreadState *tstate)
{
    int err;
    if (tstate == NULL)
        Py_FatalError("take_gil: NULL tstate");

    err = errno;
    MUTEX_LOCK(gil_mutex);

    if (!_Py_atomic_load_relaxed(&gil_locked))
        goto _ready;
    
    while (_Py_atomic_load_relaxed(&gil_locked)) {
        int timed_out = 0;
        unsigned long saved_switchnum;

        saved_switchnum = gil_switch_number;
        COND_TIMED_WAIT(gil_cond, gil_mutex, INTERVAL, timed_out);
        /* If we timed out and no switch occurred in the meantime, it is time
           to ask the GIL-holding thread to drop it. */
        if (timed_out &&
            _Py_atomic_load_relaxed(&gil_locked) &&
            gil_switch_number == saved_switchnum) {
            SET_GIL_DROP_REQUEST();
        }
    }
_ready:
#ifdef FORCE_SWITCHING
    /* This mutex must be taken before modifying gil_last_holder (see drop_gil()). */
    MUTEX_LOCK(switch_mutex);
#endif
    /* We now hold the GIL */
    _Py_atomic_store_relaxed(&gil_locked, 1);
    _Py_ANNOTATE_RWLOCK_ACQUIRED(&gil_locked, /*is_write=*/1);

    if (tstate != _Py_atomic_load_relaxed(&gil_last_holder)) {
        _Py_atomic_store_relaxed(&gil_last_holder, tstate);
        ++gil_switch_number;
    }

#ifdef FORCE_SWITCHING
    COND_SIGNAL(switch_cond);
    MUTEX_UNLOCK(switch_mutex);
#endif
    if (_Py_atomic_load_relaxed(&gil_drop_request)) {
        RESET_GIL_DROP_REQUEST();
    }
    if (tstate->async_exc != NULL) {
        _PyEval_SignalAsyncExc();
    }
    
    MUTEX_UNLOCK(gil_mutex);
    errno = err;
}

void _PyEval_SetSwitchInterval(unsigned long microseconds)
{
    gil_interval = microseconds;
}

unsigned long _PyEval_GetSwitchInterval()
{
    return gil_interval;
}
