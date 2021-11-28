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
#include "pycore_condvar.h"

#ifdef _POSIX_THREADS
/*
 * POSIX support
 */

/* These private functions are implemented in Python/thread_pthread.h */
int _PyThread_cond_init(PyCOND_T *cond);
void _PyThread_cond_after(long long us, struct timespec *abs);

/* The following functions return 0 on success, nonzero on error */
#define PyMUTEX_INIT(mut)       pthread_mutex_init((mut), NULL)
#define PyMUTEX_FINI(mut)       pthread_mutex_destroy(mut)
#define PyMUTEX_LOCK(mut)       pthread_mutex_lock(mut)
#define PyMUTEX_UNLOCK(mut)     pthread_mutex_unlock(mut)

#define PyCOND_INIT(cond)       _PyThread_cond_init(cond)
#define PyCOND_FINI(cond)       pthread_cond_destroy(cond)
#define PyCOND_SIGNAL(cond)     pthread_cond_signal(cond)
#define PyCOND_BROADCAST(cond)  pthread_cond_broadcast(cond)
#define PyCOND_WAIT(cond, mut)  pthread_cond_wait((cond), (mut))

/* return 0 for success, 1 on timeout, -1 on error */
Py_LOCAL_INLINE(int)
PyCOND_TIMEDWAIT(PyCOND_T *cond, PyMUTEX_T *mut, long long us)
{
    struct timespec abs;
    _PyThread_cond_after(us, &abs);
    int ret = pthread_cond_timedwait(cond, mut, &abs);
    if (ret == ETIMEDOUT) {
        return 1;
    }
    if (ret) {
        return -1;
    }
    return 0;
}

#elif defined(NT_THREADS)
/*
 * Windows (XP, 2003 server and later, as well as (hopefully) CE) support
 *
 * Emulated condition variables ones that work with XP and later, plus
 * example native support on VISTA and onwards.
 */

#if _PY_EMULATED_WIN_CV

/* The mutex is a CriticalSection object and
   The condition variables is emulated with the help of a semaphore.

   This implementation still has the problem that the threads woken
   with a "signal" aren't necessarily those that are already
   waiting.  It corresponds to listing 2 in:
   http://birrell.org/andrew/papers/ImplementingCVs.pdf

   Generic emulations of the pthread_cond_* API using
   earlier Win32 functions can be found on the web.
   The following read can be give background information to these issues,
   but the implementations are all broken in some way.
   http://www.cse.wustl.edu/~schmidt/win32-cv-1.html
*/

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
     * but we are safe because we are using a semaphore which has an internal
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
         * a new thread comes along, it will pass right through, having
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
PyCOND_TIMEDWAIT(PyCOND_T *cv, PyMUTEX_T *cs, long long us)
{
    return _PyCOND_WAIT_MS(cv, cs, (DWORD)(us/1000));
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
    int waiting = cv->waiting;
    if (waiting > 0) {
        cv->waiting = 0;
        return ReleaseSemaphore(cv->sem, waiting, NULL) ? 0 : -1;
    }
    return 0;
}

#else /* !_PY_EMULATED_WIN_CV */

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


#endif /* _PY_EMULATED_WIN_CV */

#endif /* _POSIX_THREADS, NT_THREADS */

#endif /* _CONDVAR_IMPL_H_ */
