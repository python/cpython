/* 
 * Portable condition variable support for windows and pthreads.
 * Everything is inline, this header can be included where needed.
 *
 * APIs generally return 0 on success and non-zero on error,
 * and the caller needs to use its platform's error mechanism to
 * discover the error (errno, or GetLastError())
 *
 * Note that some implementations cannot distinguish between a
 * condition variable wait time-out and successful wait. Most often
 * the difference is moot anyway since the wait condition must be
 * re-checked.
 * PyCOND_TIMEDWAIT, in addition to returning negative on error,
 * thus returns 0 on regular success, 1 on timeout
 * or 2 if it can't tell.
 *
 * There are at least two caveats with using these condition variables,
 * due to the fact that they may be emulated with Semaphores on
 * Windows:
 * 1) While PyCOND_SIGNAL() will wake up at least one thread, we
 *    cannot currently guarantee that it will be one of the threads
 *    already waiting in a PyCOND_WAIT() call.  It _could_ cause
 *    the wakeup of a subsequent thread to try a PyCOND_WAIT(),
 *    including the thread doing the PyCOND_SIGNAL() itself.
 *    The same applies to PyCOND_BROADCAST(), if N threads are waiting
 *    then at least N threads will be woken up, but not necessarily
 *    those already waiting.
 *    For this reason, don't make the scheduling assumption that a
 *    specific other thread will get the wakeup signal
 * 2) The _mutex_ must be held when calling PyCOND_SIGNAL() and
 *    PyCOND_BROADCAST().
 *    While e.g. the posix standard strongly recommends that the mutex
 *    associated with the condition variable is held when a
 *    pthread_cond_signal() call is made, this is not a hard requirement,
 *    although scheduling will not be "reliable" if it isn't.  Here
 *    the mutex is used for internal synchronization of the emulated
 *    Condition Variable.
 */

#ifndef _CONDVAR_H_
#define _CONDVAR_H_

#include "Python.h"

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
#define Py_HAVE_CONDVAR

#include <pthread.h>

#define PyCOND_ADD_MICROSECONDS(tv, interval) \
do { \
    tv.tv_usec += (long) interval; \
    tv.tv_sec += tv.tv_usec / 1000000; \
    tv.tv_usec %= 1000000; \
} while (0)

/* We assume all modern POSIX systems have gettimeofday() */
#ifdef GETTIMEOFDAY_NO_TZ
#define PyCOND_GETTIMEOFDAY(ptv) gettimeofday(ptv)
#else
#define PyCOND_GETTIMEOFDAY(ptv) gettimeofday(ptv, (struct timezone *)NULL)
#endif

/* The following functions return 0 on success, nonzero on error */
#define PyMUTEX_T pthread_mutex_t
#define PyMUTEX_INIT(mut)       pthread_mutex_init((mut), NULL)
#define PyMUTEX_FINI(mut)       pthread_mutex_destroy(mut)
#define PyMUTEX_LOCK(mut)       pthread_mutex_lock(mut)
#define PyMUTEX_UNLOCK(mut)     pthread_mutex_unlock(mut)

#define PyCOND_T pthread_cond_t
#define PyCOND_INIT(cond)       pthread_cond_init((cond), NULL)
#define PyCOND_FINI(cond)       pthread_cond_destroy(cond)
#define PyCOND_SIGNAL(cond)     pthread_cond_signal(cond)
#define PyCOND_BROADCAST(cond)  pthread_cond_broadcast(cond)
#define PyCOND_WAIT(cond, mut)  pthread_cond_wait((cond), (mut))

/* return 0 for success, 1 on timeout, -1 on error */
Py_LOCAL_INLINE(int)
PyCOND_TIMEDWAIT(PyCOND_T *cond, PyMUTEX_T *mut, long us)
{
    int r;
    struct timespec ts;
    struct timeval deadline;

    PyCOND_GETTIMEOFDAY(&deadline);
    PyCOND_ADD_MICROSECONDS(deadline, us);
    ts.tv_sec = deadline.tv_sec;
    ts.tv_nsec = deadline.tv_usec * 1000;

    r = pthread_cond_timedwait((cond), (mut), &ts);
    if (r == ETIMEDOUT)
        return 1;
    else if (r)
        return -1;
    else 
        return 0;
}

#elif defined(NT_THREADS)
/*
 * Windows (XP, 2003 server and later, as well as (hopefully) CE) support
 *
 * Emulated condition variables ones that work with XP and later, plus
 * example native support on VISTA and onwards.
 */
#define Py_HAVE_CONDVAR


/* include windows if it hasn't been done before */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* options */
/* non-emulated condition variables are provided for those that want
 * to target Windows Vista.  Modify this macro to enable them.
 */
#ifndef _PY_EMULATED_WIN_CV
#define _PY_EMULATED_WIN_CV 1  /* use emulated condition variables */
#endif

/* fall back to emulation if not targeting Vista */
#if !defined NTDDI_VISTA || NTDDI_VERSION < NTDDI_VISTA
#undef _PY_EMULATED_WIN_CV
#define _PY_EMULATED_WIN_CV 1
#endif


#if _PY_EMULATED_WIN_CV

/* The mutex is a CriticalSection object and
   The condition variables is emulated with the help of a semaphore.
   Semaphores are available on Windows XP (2003 server) and later.
   We use a Semaphore rather than an auto-reset event, because although
   an auto-resent event might appear to solve the lost-wakeup bug (race
   condition between releasing the outer lock and waiting) because it
   maintains state even though a wait hasn't happened, there is still
   a lost wakeup problem if more than one thread are interrupted in the
   critical place.  A semaphore solves that, because its state is counted,
   not Boolean.
   Because it is ok to signal a condition variable with no one
   waiting, we need to keep track of the number of
   waiting threads.  Otherwise, the semaphore's state could rise
   without bound.  This also helps reduce the number of "spurious wakeups"
   that would otherwise happen.

   This implementation still has the problem that the threads woken
   with a "signal" aren't necessarily those that are already
   waiting.  It corresponds to listing 2 in:
   http://birrell.org/andrew/papers/ImplementingCVs.pdf

   Generic emulations of the pthread_cond_* API using
   earlier Win32 functions can be found on the Web.
   The following read can be edificating (or not):
   http://www.cse.wustl.edu/~schmidt/win32-cv-1.html

   See also 
*/

typedef CRITICAL_SECTION PyMUTEX_T;

Py_LOCAL_INLINE(int)
PyMUTEX_INIT(PyMUTEX_T *cs)
{
    InitializeCriticalSection(cs);
    return 0;
}

Py_LOCAL_INLINE(int)
PyMUTEX_FINI(PyMUTEX_T *cs)
{
    DeleteCriticalSection(cs);
    return 0;
}

Py_LOCAL_INLINE(int)
PyMUTEX_LOCK(PyMUTEX_T *cs)
{
    EnterCriticalSection(cs);
    return 0;
}

Py_LOCAL_INLINE(int)
PyMUTEX_UNLOCK(PyMUTEX_T *cs)
{
    LeaveCriticalSection(cs);
    return 0;
}

/* The ConditionVariable object.  From XP onwards it is easily emulated with
 * a Semaphore
 */

typedef struct _PyCOND_T
{
    HANDLE sem;
    int waiting; /* to allow PyCOND_SIGNAL to be a no-op */
} PyCOND_T;

Py_LOCAL_INLINE(int)
PyCOND_INIT(PyCOND_T *cv)
{
    /* A semaphore with a "large" max value,  The positive value
     * is only needed to catch those "lost wakeup" events and
     * race conditions when a timed wait elapses.
     */
    cv->sem = CreateSemaphore(NULL, 0, 100000, NULL);
    if (cv->sem==NULL)
        return -1;
    cv->waiting = 0;
    return 0;
}

Py_LOCAL_INLINE(int)
PyCOND_FINI(PyCOND_T *cv)
{
    return CloseHandle(cv->sem) ? 0 : -1;
}

/* this implementation can detect a timeout.  Returns 1 on timeout,
 * 0 otherwise (and -1 on error)
 */
Py_LOCAL_INLINE(int)
_PyCOND_WAIT_MS(PyCOND_T *cv, PyMUTEX_T *cs, DWORD ms)
{
    DWORD wait;
    cv->waiting++;
    PyMUTEX_UNLOCK(cs);
    /* "lost wakeup bug" would occur if the caller were interrupted here,
     * but we are safe because we are using a semaphore wich has an internal
     * count.
     */
    wait = WaitForSingleObjectEx(cv->sem, ms, FALSE);
    PyMUTEX_LOCK(cs);
    if (wait != WAIT_OBJECT_0)
        --cv->waiting;
        /* Here we have a benign race condition with PyCOND_SIGNAL.
         * When failure occurs or timeout, it is possible that
         * PyCOND_SIGNAL also decrements this value
         * and signals releases the mutex.  This is benign because it
         * just means an extra spurious wakeup for a waiting thread.
         * ('waiting' corresponds to the semaphore's "negative" count and
         * we may end up with e.g. (waiting == -1 && sem.count == 1).  When
         * a new thread comes along, it will pass right throuhgh, having
         * adjusted it to (waiting == 0 && sem.count == 0).
         */
         
    if (wait == WAIT_FAILED)
        return -1;
    /* return 0 on success, 1 on timeout */
    return wait != WAIT_OBJECT_0;
}

Py_LOCAL_INLINE(int)
PyCOND_WAIT(PyCOND_T *cv, PyMUTEX_T *cs)
{
    int result = _PyCOND_WAIT_MS(cv, cs, INFINITE);
    return result >= 0 ? 0 : result;
}

Py_LOCAL_INLINE(int)
PyCOND_TIMEDWAIT(PyCOND_T *cv, PyMUTEX_T *cs, long us)
{
    return _PyCOND_WAIT_MS(cv, cs, us/1000);
}

Py_LOCAL_INLINE(int)
PyCOND_SIGNAL(PyCOND_T *cv)
{
    /* this test allows PyCOND_SIGNAL to be a no-op unless required
     * to wake someone up, thus preventing an unbounded increase of
     * the semaphore's internal counter.
     */
    if (cv->waiting > 0) {
        /* notifying thread decreases the cv->waiting count so that
         * a delay between notify and actual wakeup of the target thread
         * doesn't cause a number of extra ReleaseSemaphore calls.
         */
        cv->waiting--;
        return ReleaseSemaphore(cv->sem, 1, NULL) ? 0 : -1;
    }
    return 0;
}

Py_LOCAL_INLINE(int)
PyCOND_BROADCAST(PyCOND_T *cv)
{
    if (cv->waiting > 0) {
        return ReleaseSemaphore(cv->sem, cv->waiting, NULL) ? 0 : -1;
		cv->waiting = 0;
    }
    return 0;
}

#else

/* Use native Win7 primitives if build target is Win7 or higher */

/* SRWLOCK is faster and better than CriticalSection */
typedef SRWLOCK PyMUTEX_T;

Py_LOCAL_INLINE(int)
PyMUTEX_INIT(PyMUTEX_T *cs)
{
    InitializeSRWLock(cs);
    return 0;
}

Py_LOCAL_INLINE(int)
PyMUTEX_FINI(PyMUTEX_T *cs)
{
    return 0;
}

Py_LOCAL_INLINE(int)
PyMUTEX_LOCK(PyMUTEX_T *cs)
{
    AcquireSRWLockExclusive(cs);
    return 0;
}

Py_LOCAL_INLINE(int)
PyMUTEX_UNLOCK(PyMUTEX_T *cs)
{
    ReleaseSRWLockExclusive(cs);
    return 0;
}


typedef CONDITION_VARIABLE  PyCOND_T;

Py_LOCAL_INLINE(int)
PyCOND_INIT(PyCOND_T *cv)
{
    InitializeConditionVariable(cv);
    return 0;
}
Py_LOCAL_INLINE(int)
PyCOND_FINI(PyCOND_T *cv)
{
    return 0;
}

Py_LOCAL_INLINE(int)
PyCOND_WAIT(PyCOND_T *cv, PyMUTEX_T *cs)
{
    return SleepConditionVariableSRW(cv, cs, INFINITE, 0) ? 0 : -1;
}

/* This implementation makes no distinction about timeouts.  Signal
 * 2 to indicate that we don't know.
 */
Py_LOCAL_INLINE(int)
PyCOND_TIMEDWAIT(PyCOND_T *cv, PyMUTEX_T *cs, long us)
{
    return SleepConditionVariableSRW(cv, cs, us/1000, 0) ? 2 : -1;
}

Py_LOCAL_INLINE(int)
PyCOND_SIGNAL(PyCOND_T *cv)
{
     WakeConditionVariable(cv);
     return 0;
}

Py_LOCAL_INLINE(int)
PyCOND_BROADCAST(PyCOND_T *cv)
{
     WakeAllConditionVariable(cv);
     return 0;
}


#endif /* _PY_EMULATED_WIN_CV */

#endif /* _POSIX_THREADS, NT_THREADS */

#endif /* _CONDVAR_H_ */
