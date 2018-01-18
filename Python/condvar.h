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

#ifndef _CONDVAR_IMPL_H_
#define _CONDVAR_IMPL_H_

#include "Python.h"
#include "internal/condvar.h"

#ifdef _POSIX_THREADS
/*
 * POSIX support
 */

#define PyCOND_ADD_MICROSECONDS(tv, interval) \
do { /* TODO: add overflow and truncation checks */ \
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
#define PyMUTEX_INIT(mut)       pthread_mutex_init((mut), NULL)
#define PyMUTEX_FINI(mut)       pthread_mutex_destroy(mut)
#define PyMUTEX_LOCK(mut)       pthread_mutex_lock(mut)
#define PyMUTEX_UNLOCK(mut)     pthread_mutex_unlock(mut)

#define PyCOND_INIT(cond)       pthread_cond_init((cond), NULL)
#define PyCOND_FINI(cond)       pthread_cond_destroy(cond)
#define PyCOND_SIGNAL(cond)     pthread_cond_signal(cond)
#define PyCOND_BROADCAST(cond)  pthread_cond_broadcast(cond)
#define PyCOND_WAIT(cond, mut)  pthread_cond_wait((cond), (mut))

/* return 0 for success, 1 on timeout, -1 on error */
Py_LOCAL_INLINE(int)
PyCOND_TIMEDWAIT(PyCOND_T *cond, PyMUTEX_T *mut, long long us)
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
PyCOND_TIMEDWAIT(PyCOND_T *cv, PyMUTEX_T *cs, long long us)
{
    return SleepConditionVariableSRW(cv, cs, (DWORD)(us/1000), 0) ? 2 : -1;
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

#endif /* _POSIX_THREADS, NT_THREADS */

#endif /* _CONDVAR_IMPL_H_ */
