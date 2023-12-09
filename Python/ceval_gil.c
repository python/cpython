
#include "Python.h"
#include "pycore_atomic.h"        // _Py_atomic_int
#include "pycore_ceval.h"         // _PyEval_SignalReceived()
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pylifecycle.h"   // _PyErr_Print()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // _Py_RunGC()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()

/*
   Notes about the implementation:

   - The GIL is just a boolean variable (locked) whose access is protected
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
     the value of last_holder is changed to something else than its
     own thread state pointer, indicating that another thread was able to
     take the GIL.

     This is meant to prohibit the latency-adverse behaviour on multi-core
     machines where one thread would speculatively release the GIL, but still
     run and end up being the first to re-acquire it, making the "timeslices"
     much longer than expected.
     (Note: this mechanism is enabled with FORCE_SWITCHING above)
*/

// GH-89279: Force inlining by using a macro.
#if defined(_MSC_VER) && SIZEOF_INT == 4
#define _Py_atomic_load_relaxed_int32(ATOMIC_VAL) (assert(sizeof((ATOMIC_VAL)->_value) == 4), *((volatile int*)&((ATOMIC_VAL)->_value)))
#else
#define _Py_atomic_load_relaxed_int32(ATOMIC_VAL) _Py_atomic_load_relaxed(ATOMIC_VAL)
#endif

/* This can set eval_breaker to 0 even though gil_drop_request became
   1.  We believe this is all right because the eval loop will release
   the GIL eventually anyway. */
static inline void
COMPUTE_EVAL_BREAKER(PyInterpreterState *interp,
                     struct _ceval_runtime_state *ceval,
                     struct _ceval_state *ceval2)
{
    _Py_atomic_store_relaxed(&ceval2->eval_breaker,
        _Py_atomic_load_relaxed_int32(&ceval2->gil_drop_request)
        | (_Py_atomic_load_relaxed_int32(&ceval->signals_pending)
           && _Py_ThreadCanHandleSignals(interp))
        | (_Py_atomic_load_relaxed_int32(&ceval2->pending.calls_to_do))
        | (_Py_IsMainThread() && _Py_IsMainInterpreter(interp)
           &&_Py_atomic_load_relaxed_int32(&ceval->pending_mainthread.calls_to_do))
        | ceval2->pending.async_exc
        | _Py_atomic_load_relaxed_int32(&ceval2->gc_scheduled));
}


static inline void
SET_GIL_DROP_REQUEST(PyInterpreterState *interp)
{
    struct _ceval_state *ceval2 = &interp->ceval;
    _Py_atomic_store_relaxed(&ceval2->gil_drop_request, 1);
    _Py_atomic_store_relaxed(&ceval2->eval_breaker, 1);
}


static inline void
RESET_GIL_DROP_REQUEST(PyInterpreterState *interp)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
    _Py_atomic_store_relaxed(&ceval2->gil_drop_request, 0);
    COMPUTE_EVAL_BREAKER(interp, ceval, ceval2);
}


static inline void
SIGNAL_PENDING_CALLS(struct _pending_calls *pending, PyInterpreterState *interp)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
    _Py_atomic_store_relaxed(&pending->calls_to_do, 1);
    COMPUTE_EVAL_BREAKER(interp, ceval, ceval2);
}


static inline void
UNSIGNAL_PENDING_CALLS(PyInterpreterState *interp)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
    if (_Py_IsMainThread() && _Py_IsMainInterpreter(interp)) {
        _Py_atomic_store_relaxed(&ceval->pending_mainthread.calls_to_do, 0);
    }
    _Py_atomic_store_relaxed(&ceval2->pending.calls_to_do, 0);
    COMPUTE_EVAL_BREAKER(interp, ceval, ceval2);
}


static inline void
SIGNAL_PENDING_SIGNALS(PyInterpreterState *interp, int force)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
    _Py_atomic_store_relaxed(&ceval->signals_pending, 1);
    if (force) {
        _Py_atomic_store_relaxed(&ceval2->eval_breaker, 1);
    }
    else {
        /* eval_breaker is not set to 1 if thread_can_handle_signals() is false */
        COMPUTE_EVAL_BREAKER(interp, ceval, ceval2);
    }
}


static inline void
UNSIGNAL_PENDING_SIGNALS(PyInterpreterState *interp)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
    _Py_atomic_store_relaxed(&ceval->signals_pending, 0);
    COMPUTE_EVAL_BREAKER(interp, ceval, ceval2);
}


static inline void
SIGNAL_ASYNC_EXC(PyInterpreterState *interp)
{
    struct _ceval_state *ceval2 = &interp->ceval;
    ceval2->pending.async_exc = 1;
    _Py_atomic_store_relaxed(&ceval2->eval_breaker, 1);
}


static inline void
UNSIGNAL_ASYNC_EXC(PyInterpreterState *interp)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
    ceval2->pending.async_exc = 0;
    COMPUTE_EVAL_BREAKER(interp, ceval, ceval2);
}


/*
 * Implementation of the Global Interpreter Lock (GIL).
 */

#include <stdlib.h>
#include <errno.h>

#include "pycore_atomic.h"


#include "condvar.h"

#define MUTEX_INIT(mut) \
    if (PyMUTEX_INIT(&(mut))) { \
        Py_FatalError("PyMUTEX_INIT(" #mut ") failed"); };
#define MUTEX_FINI(mut) \
    if (PyMUTEX_FINI(&(mut))) { \
        Py_FatalError("PyMUTEX_FINI(" #mut ") failed"); };
#define MUTEX_LOCK(mut) \
    if (PyMUTEX_LOCK(&(mut))) { \
        Py_FatalError("PyMUTEX_LOCK(" #mut ") failed"); };
#define MUTEX_UNLOCK(mut) \
    if (PyMUTEX_UNLOCK(&(mut))) { \
        Py_FatalError("PyMUTEX_UNLOCK(" #mut ") failed"); };

#define COND_INIT(cond) \
    if (PyCOND_INIT(&(cond))) { \
        Py_FatalError("PyCOND_INIT(" #cond ") failed"); };
#define COND_FINI(cond) \
    if (PyCOND_FINI(&(cond))) { \
        Py_FatalError("PyCOND_FINI(" #cond ") failed"); };
#define COND_SIGNAL(cond) \
    if (PyCOND_SIGNAL(&(cond))) { \
        Py_FatalError("PyCOND_SIGNAL(" #cond ") failed"); };
#define COND_WAIT(cond, mut) \
    if (PyCOND_WAIT(&(cond), &(mut))) { \
        Py_FatalError("PyCOND_WAIT(" #cond ") failed"); };
#define COND_TIMED_WAIT(cond, mut, microseconds, timeout_result) \
    { \
        int r = PyCOND_TIMEDWAIT(&(cond), &(mut), (microseconds)); \
        if (r < 0) \
            Py_FatalError("PyCOND_WAIT(" #cond ") failed"); \
        if (r) /* 1 == timeout, 2 == impl. can't say, so assume timeout */ \
            timeout_result = 1; \
        else \
            timeout_result = 0; \
    } \


#define DEFAULT_INTERVAL 5000

static void _gil_initialize(struct _gil_runtime_state *gil)
{
    _Py_atomic_int uninitialized = {-1};
    gil->locked = uninitialized;
    gil->interval = DEFAULT_INTERVAL;
}

static int gil_created(struct _gil_runtime_state *gil)
{
    if (gil == NULL) {
        return 0;
    }
    return (_Py_atomic_load_explicit(&gil->locked, _Py_memory_order_acquire) >= 0);
}

static void create_gil(struct _gil_runtime_state *gil)
{
    MUTEX_INIT(gil->mutex);
#ifdef FORCE_SWITCHING
    MUTEX_INIT(gil->switch_mutex);
#endif
    COND_INIT(gil->cond);
#ifdef FORCE_SWITCHING
    COND_INIT(gil->switch_cond);
#endif
    _Py_atomic_store_relaxed(&gil->last_holder, 0);
    _Py_ANNOTATE_RWLOCK_CREATE(&gil->locked);
    _Py_atomic_store_explicit(&gil->locked, 0, _Py_memory_order_release);
}

static void destroy_gil(struct _gil_runtime_state *gil)
{
    /* some pthread-like implementations tie the mutex to the cond
     * and must have the cond destroyed first.
     */
    COND_FINI(gil->cond);
    MUTEX_FINI(gil->mutex);
#ifdef FORCE_SWITCHING
    COND_FINI(gil->switch_cond);
    MUTEX_FINI(gil->switch_mutex);
#endif
    _Py_atomic_store_explicit(&gil->locked, -1,
                              _Py_memory_order_release);
    _Py_ANNOTATE_RWLOCK_DESTROY(&gil->locked);
}

#ifdef HAVE_FORK
static void recreate_gil(struct _gil_runtime_state *gil)
{
    _Py_ANNOTATE_RWLOCK_DESTROY(&gil->locked);
    /* XXX should we destroy the old OS resources here? */
    create_gil(gil);
}
#endif

static void
drop_gil(struct _ceval_state *ceval, PyThreadState *tstate)
{
    /* If tstate is NULL, the caller is indicating that we're releasing
       the GIL for the last time in this thread.  This is particularly
       relevant when the current thread state is finalizing or its
       interpreter is finalizing (either may be in an inconsistent
       state).  In that case the current thread will definitely
       never try to acquire the GIL again. */
    // XXX It may be more correct to check tstate->_status.finalizing.
    // XXX assert(tstate == NULL || !tstate->_status.cleared);

    struct _gil_runtime_state *gil = ceval->gil;
    if (!_Py_atomic_load_relaxed(&gil->locked)) {
        Py_FatalError("drop_gil: GIL is not locked");
    }

    /* tstate is allowed to be NULL (early interpreter init) */
    if (tstate != NULL) {
        /* Sub-interpreter support: threads might have been switched
           under our feet using PyThreadState_Swap(). Fix the GIL last
           holder variable so that our heuristics work. */
        _Py_atomic_store_relaxed(&gil->last_holder, (uintptr_t)tstate);
    }

    MUTEX_LOCK(gil->mutex);
    _Py_ANNOTATE_RWLOCK_RELEASED(&gil->locked, /*is_write=*/1);
    _Py_atomic_store_relaxed(&gil->locked, 0);
    COND_SIGNAL(gil->cond);
    MUTEX_UNLOCK(gil->mutex);

#ifdef FORCE_SWITCHING
    /* We check tstate first in case we might be releasing the GIL for
       the last time in this thread.  In that case there's a possible
       race with tstate->interp getting deleted after gil->mutex is
       unlocked and before the following code runs, leading to a crash.
       We can use (tstate == NULL) to indicate the thread is done with
       the GIL, and that's the only time we might delete the
       interpreter, so checking tstate first prevents the crash.
       See https://github.com/python/cpython/issues/104341. */
    if (tstate != NULL && _Py_atomic_load_relaxed(&ceval->gil_drop_request)) {
        MUTEX_LOCK(gil->switch_mutex);
        /* Not switched yet => wait */
        if (((PyThreadState*)_Py_atomic_load_relaxed(&gil->last_holder)) == tstate)
        {
            assert(_PyThreadState_CheckConsistency(tstate));
            RESET_GIL_DROP_REQUEST(tstate->interp);
            /* NOTE: if COND_WAIT does not atomically start waiting when
               releasing the mutex, another thread can run through, take
               the GIL and drop it again, and reset the condition
               before we even had a chance to wait for it. */
            COND_WAIT(gil->switch_cond, gil->switch_mutex);
        }
        MUTEX_UNLOCK(gil->switch_mutex);
    }
#endif
}


/* Take the GIL.

   The function saves errno at entry and restores its value at exit.

   tstate must be non-NULL. */
static void
take_gil(PyThreadState *tstate)
{
    int err = errno;

    assert(tstate != NULL);
    /* We shouldn't be using a thread state that isn't viable any more. */
    // XXX It may be more correct to check tstate->_status.finalizing.
    // XXX assert(!tstate->_status.cleared);

    if (_PyThreadState_MustExit(tstate)) {
        /* bpo-39877: If Py_Finalize() has been called and tstate is not the
           thread which called Py_Finalize(), exit immediately the thread.

           This code path can be reached by a daemon thread after Py_Finalize()
           completes. In this case, tstate is a dangling pointer: points to
           PyThreadState freed memory. */
        PyThread_exit_thread();
    }

    assert(_PyThreadState_CheckConsistency(tstate));
    PyInterpreterState *interp = tstate->interp;
    struct _ceval_state *ceval = &interp->ceval;
    struct _gil_runtime_state *gil = ceval->gil;

    /* Check that _PyEval_InitThreads() was called to create the lock */
    assert(gil_created(gil));

    MUTEX_LOCK(gil->mutex);

    if (!_Py_atomic_load_relaxed(&gil->locked)) {
        goto _ready;
    }

    int drop_requested = 0;
    while (_Py_atomic_load_relaxed(&gil->locked)) {
        unsigned long saved_switchnum = gil->switch_number;

        unsigned long interval = (gil->interval >= 1 ? gil->interval : 1);
        int timed_out = 0;
        COND_TIMED_WAIT(gil->cond, gil->mutex, interval, timed_out);

        /* If we timed out and no switch occurred in the meantime, it is time
           to ask the GIL-holding thread to drop it. */
        if (timed_out &&
            _Py_atomic_load_relaxed(&gil->locked) &&
            gil->switch_number == saved_switchnum)
        {
            if (_PyThreadState_MustExit(tstate)) {
                MUTEX_UNLOCK(gil->mutex);
                // gh-96387: If the loop requested a drop request in a previous
                // iteration, reset the request. Otherwise, drop_gil() can
                // block forever waiting for the thread which exited. Drop
                // requests made by other threads are also reset: these threads
                // may have to request again a drop request (iterate one more
                // time).
                if (drop_requested) {
                    RESET_GIL_DROP_REQUEST(interp);
                }
                PyThread_exit_thread();
            }
            assert(_PyThreadState_CheckConsistency(tstate));

            SET_GIL_DROP_REQUEST(interp);
            drop_requested = 1;
        }
    }

_ready:
#ifdef FORCE_SWITCHING
    /* This mutex must be taken before modifying gil->last_holder:
       see drop_gil(). */
    MUTEX_LOCK(gil->switch_mutex);
#endif
    /* We now hold the GIL */
    _Py_atomic_store_relaxed(&gil->locked, 1);
    _Py_ANNOTATE_RWLOCK_ACQUIRED(&gil->locked, /*is_write=*/1);

    if (tstate != (PyThreadState*)_Py_atomic_load_relaxed(&gil->last_holder)) {
        _Py_atomic_store_relaxed(&gil->last_holder, (uintptr_t)tstate);
        ++gil->switch_number;
    }

#ifdef FORCE_SWITCHING
    COND_SIGNAL(gil->switch_cond);
    MUTEX_UNLOCK(gil->switch_mutex);
#endif

    if (_PyThreadState_MustExit(tstate)) {
        /* bpo-36475: If Py_Finalize() has been called and tstate is not
           the thread which called Py_Finalize(), exit immediately the
           thread.

           This code path can be reached by a daemon thread which was waiting
           in take_gil() while the main thread called
           wait_for_thread_shutdown() from Py_Finalize(). */
        MUTEX_UNLOCK(gil->mutex);
        drop_gil(ceval, tstate);
        PyThread_exit_thread();
    }
    assert(_PyThreadState_CheckConsistency(tstate));

    if (_Py_atomic_load_relaxed(&ceval->gil_drop_request)) {
        RESET_GIL_DROP_REQUEST(interp);
    }
    else {
        /* bpo-40010: eval_breaker should be recomputed to be set to 1 if there
           is a pending signal: signal received by another thread which cannot
           handle signals.

           Note: RESET_GIL_DROP_REQUEST() calls COMPUTE_EVAL_BREAKER(). */
        COMPUTE_EVAL_BREAKER(interp, &_PyRuntime.ceval, ceval);
    }

    /* Don't access tstate if the thread must exit */
    if (tstate->async_exc != NULL) {
        _PyEval_SignalAsyncExc(tstate->interp);
    }

    MUTEX_UNLOCK(gil->mutex);

    errno = err;
}

void _PyEval_SetSwitchInterval(unsigned long microseconds)
{
    PyInterpreterState *interp = _PyInterpreterState_Get();
    struct _gil_runtime_state *gil = interp->ceval.gil;
    assert(gil != NULL);
    gil->interval = microseconds;
}

unsigned long _PyEval_GetSwitchInterval(void)
{
    PyInterpreterState *interp = _PyInterpreterState_Get();
    struct _gil_runtime_state *gil = interp->ceval.gil;
    assert(gil != NULL);
    return gil->interval;
}


int
_PyEval_ThreadsInitialized(void)
{
    /* XXX This is only needed for an assert in PyGILState_Ensure(),
     * which currently does not work with subinterpreters.
     * Thus we only use the main interpreter. */
    PyInterpreterState *interp = _PyInterpreterState_Main();
    if (interp == NULL) {
        return 0;
    }
    struct _gil_runtime_state *gil = interp->ceval.gil;
    return gil_created(gil);
}

int
PyEval_ThreadsInitialized(void)
{
    return _PyEval_ThreadsInitialized();
}

static inline int
current_thread_holds_gil(struct _gil_runtime_state *gil, PyThreadState *tstate)
{
    if (((PyThreadState*)_Py_atomic_load_relaxed(&gil->last_holder)) != tstate) {
        return 0;
    }
    return _Py_atomic_load_relaxed(&gil->locked);
}

static void
init_shared_gil(PyInterpreterState *interp, struct _gil_runtime_state *gil)
{
    assert(gil_created(gil));
    interp->ceval.gil = gil;
    interp->ceval.own_gil = 0;
}

static void
init_own_gil(PyInterpreterState *interp, struct _gil_runtime_state *gil)
{
    assert(!gil_created(gil));
    create_gil(gil);
    assert(gil_created(gil));
    interp->ceval.gil = gil;
    interp->ceval.own_gil = 1;
}

PyStatus
_PyEval_InitGIL(PyThreadState *tstate, int own_gil)
{
    assert(tstate->interp->ceval.gil == NULL);
    int locked;
    if (!own_gil) {
        /* The interpreter will share the main interpreter's instead. */
        PyInterpreterState *main_interp = _PyInterpreterState_Main();
        assert(tstate->interp != main_interp);
        struct _gil_runtime_state *gil = main_interp->ceval.gil;
        init_shared_gil(tstate->interp, gil);
        locked = current_thread_holds_gil(gil, tstate);
    }
    else {
        PyThread_init_thread();
        init_own_gil(tstate->interp, &tstate->interp->_gil);
        locked = 0;
    }
    if (!locked) {
        take_gil(tstate);
    }

    return _PyStatus_OK();
}

void
_PyEval_FiniGIL(PyInterpreterState *interp)
{
    struct _gil_runtime_state *gil = interp->ceval.gil;
    if (gil == NULL) {
        /* It was already finalized (or hasn't been initialized yet). */
        assert(!interp->ceval.own_gil);
        return;
    }
    else if (!interp->ceval.own_gil) {
#ifdef Py_DEBUG
        PyInterpreterState *main_interp = _PyInterpreterState_Main();
        assert(main_interp != NULL && interp != main_interp);
        assert(interp->ceval.gil == main_interp->ceval.gil);
#endif
        interp->ceval.gil = NULL;
        return;
    }

    if (!gil_created(gil)) {
        /* First Py_InitializeFromConfig() call: the GIL doesn't exist
           yet: do nothing. */
        return;
    }

    destroy_gil(gil);
    assert(!gil_created(gil));
    interp->ceval.gil = NULL;
}

void
PyEval_InitThreads(void)
{
    /* Do nothing: kept for backward compatibility */
}

void
_PyEval_Fini(void)
{
#ifdef Py_STATS
    _Py_PrintSpecializationStats(1);
#endif
}
void
PyEval_AcquireLock(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);

    take_gil(tstate);
}

void
PyEval_ReleaseLock(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    /* This function must succeed when the current thread state is NULL.
       We therefore avoid PyThreadState_Get() which dumps a fatal error
       in debug mode. */
    struct _ceval_state *ceval = &tstate->interp->ceval;
    drop_gil(ceval, tstate);
}

void
_PyEval_AcquireLock(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);
    take_gil(tstate);
}

void
_PyEval_ReleaseLock(PyInterpreterState *interp, PyThreadState *tstate)
{
    /* If tstate is NULL then we do not expect the current thread
       to acquire the GIL ever again. */
    assert(tstate == NULL || tstate->interp == interp);
    struct _ceval_state *ceval = &interp->ceval;
    drop_gil(ceval, tstate);
}

void
PyEval_AcquireThread(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);

    take_gil(tstate);

    if (_PyThreadState_SwapNoGIL(tstate) != NULL) {
        Py_FatalError("non-NULL old thread state");
    }
}

void
PyEval_ReleaseThread(PyThreadState *tstate)
{
    assert(_PyThreadState_CheckConsistency(tstate));

    PyThreadState *new_tstate = _PyThreadState_SwapNoGIL(NULL);
    if (new_tstate != tstate) {
        Py_FatalError("wrong thread state");
    }
    struct _ceval_state *ceval = &tstate->interp->ceval;
    drop_gil(ceval, tstate);
}

#ifdef HAVE_FORK
/* This function is called from PyOS_AfterFork_Child to destroy all threads
   which are not running in the child process, and clear internal locks
   which might be held by those threads. */
PyStatus
_PyEval_ReInitThreads(PyThreadState *tstate)
{
    assert(tstate->interp == _PyInterpreterState_Main());

    struct _gil_runtime_state *gil = tstate->interp->ceval.gil;
    if (!gil_created(gil)) {
        return _PyStatus_OK();
    }
    recreate_gil(gil);

    take_gil(tstate);

    struct _pending_calls *pending = &tstate->interp->ceval.pending;
    if (_PyThread_at_fork_reinit(&pending->lock) < 0) {
        return _PyStatus_ERR("Can't reinitialize pending calls lock");
    }

    /* Destroy all threads except the current one */
    _PyThreadState_DeleteExcept(tstate);
    return _PyStatus_OK();
}
#endif

/* This function is used to signal that async exceptions are waiting to be
   raised. */

void
_PyEval_SignalAsyncExc(PyInterpreterState *interp)
{
    SIGNAL_ASYNC_EXC(interp);
}

PyThreadState *
PyEval_SaveThread(void)
{
    PyThreadState *tstate = _PyThreadState_SwapNoGIL(NULL);
    _Py_EnsureTstateNotNULL(tstate);

    struct _ceval_state *ceval = &tstate->interp->ceval;
    assert(gil_created(ceval->gil));
    drop_gil(ceval, tstate);
    return tstate;
}

void
PyEval_RestoreThread(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);

    take_gil(tstate);

    _PyThreadState_SwapNoGIL(tstate);
}


/* Mechanism whereby asynchronously executing callbacks (e.g. UNIX
   signal handlers or Mac I/O completion routines) can schedule calls
   to a function to be called synchronously.
   The synchronous function is called with one void* argument.
   It should return 0 for success or -1 for failure -- failure should
   be accompanied by an exception.

   If registry succeeds, the registry function returns 0; if it fails
   (e.g. due to too many pending calls) it returns -1 (without setting
   an exception condition).

   Note that because registry may occur from within signal handlers,
   or other asynchronous events, calling malloc() is unsafe!

   Any thread can schedule pending calls, but only the main thread
   will execute them.
   There is no facility to schedule calls to a particular thread, but
   that should be easy to change, should that ever be required.  In
   that case, the static variables here should go into the python
   threadstate.
*/

void
_PyEval_SignalReceived(PyInterpreterState *interp)
{
#ifdef MS_WINDOWS
    // bpo-42296: On Windows, _PyEval_SignalReceived() is called from a signal
    // handler which can run in a thread different than the Python thread, in
    // which case _Py_ThreadCanHandleSignals() is wrong. Ignore
    // _Py_ThreadCanHandleSignals() and always set eval_breaker to 1.
    //
    // The next eval_frame_handle_pending() call will call
    // _Py_ThreadCanHandleSignals() to recompute eval_breaker.
    int force = 1;
#else
    int force = 0;
#endif
    /* bpo-30703: Function called when the C signal handler of Python gets a
       signal. We cannot queue a callback using _PyEval_AddPendingCall() since
       that function is not async-signal-safe. */
    SIGNAL_PENDING_SIGNALS(interp, force);
}

/* Push one item onto the queue while holding the lock. */
static int
_push_pending_call(struct _pending_calls *pending,
                   int (*func)(void *), void *arg)
{
    int i = pending->last;
    int j = (i + 1) % NPENDINGCALLS;
    if (j == pending->first) {
        return -1; /* Queue full */
    }
    pending->calls[i].func = func;
    pending->calls[i].arg = arg;
    pending->last = j;
    return 0;
}

static int
_next_pending_call(struct _pending_calls *pending,
                   int (**func)(void *), void **arg)
{
    int i = pending->first;
    if (i == pending->last) {
        /* Queue empty */
        assert(pending->calls[i].func == NULL);
        return -1;
    }
    *func = pending->calls[i].func;
    *arg = pending->calls[i].arg;
    return i;
}

/* Pop one item off the queue while holding the lock. */
static void
_pop_pending_call(struct _pending_calls *pending,
                  int (**func)(void *), void **arg)
{
    int i = _next_pending_call(pending, func, arg);
    if (i >= 0) {
        pending->calls[i] = (struct _pending_call){0};
        pending->first = (i + 1) % NPENDINGCALLS;
    }
}

/* This implementation is thread-safe.  It allows
   scheduling to be made from any thread, and even from an executing
   callback.
 */

int
_PyEval_AddPendingCall(PyInterpreterState *interp,
                       int (*func)(void *), void *arg,
                       int mainthreadonly)
{
    assert(!mainthreadonly || _Py_IsMainInterpreter(interp));
    struct _pending_calls *pending = &interp->ceval.pending;
    if (mainthreadonly) {
        /* The main thread only exists in the main interpreter. */
        assert(_Py_IsMainInterpreter(interp));
        pending = &_PyRuntime.ceval.pending_mainthread;
    }
    /* Ensure that _PyEval_InitState() was called
       and that _PyEval_FiniState() is not called yet. */
    assert(pending->lock != NULL);

    PyThread_acquire_lock(pending->lock, WAIT_LOCK);
    int result = _push_pending_call(pending, func, arg);
    PyThread_release_lock(pending->lock);

    /* signal main loop */
    SIGNAL_PENDING_CALLS(pending, interp);
    return result;
}

int
Py_AddPendingCall(int (*func)(void *), void *arg)
{
    /* Legacy users of this API will continue to target the main thread
       (of the main interpreter). */
    PyInterpreterState *interp = _PyInterpreterState_Main();
    return _PyEval_AddPendingCall(interp, func, arg, 1);
}

static int
handle_signals(PyThreadState *tstate)
{
    assert(_PyThreadState_CheckConsistency(tstate));
    if (!_Py_ThreadCanHandleSignals(tstate->interp)) {
        return 0;
    }

    UNSIGNAL_PENDING_SIGNALS(tstate->interp);
    if (_PyErr_CheckSignalsTstate(tstate) < 0) {
        /* On failure, re-schedule a call to handle_signals(). */
        SIGNAL_PENDING_SIGNALS(tstate->interp, 0);
        return -1;
    }
    return 0;
}

static inline int
maybe_has_pending_calls(PyInterpreterState *interp)
{
    struct _pending_calls *pending = &interp->ceval.pending;
    if (_Py_atomic_load_relaxed_int32(&pending->calls_to_do)) {
        return 1;
    }
    if (!_Py_IsMainThread() || !_Py_IsMainInterpreter(interp)) {
        return 0;
    }
    pending = &_PyRuntime.ceval.pending_mainthread;
    return _Py_atomic_load_relaxed_int32(&pending->calls_to_do);
}

static int
_make_pending_calls(struct _pending_calls *pending)
{
    /* perform a bounded number of calls, in case of recursion */
    for (int i=0; i<NPENDINGCALLS; i++) {
        int (*func)(void *) = NULL;
        void *arg = NULL;

        /* pop one item off the queue while holding the lock */
        PyThread_acquire_lock(pending->lock, WAIT_LOCK);
        _pop_pending_call(pending, &func, &arg);
        PyThread_release_lock(pending->lock);

        /* having released the lock, perform the callback */
        if (func == NULL) {
            break;
        }
        if (func(arg) != 0) {
            return -1;
        }
    }
    return 0;
}

static int
make_pending_calls(PyInterpreterState *interp)
{
    struct _pending_calls *pending = &interp->ceval.pending;
    struct _pending_calls *pending_main = &_PyRuntime.ceval.pending_mainthread;

    /* Only one thread (per interpreter) may run the pending calls
       at once.  In the same way, we don't do recursive pending calls. */
    PyThread_acquire_lock(pending->lock, WAIT_LOCK);
    if (pending->busy) {
        /* A pending call was added after another thread was already
           handling the pending calls (and had already "unsignaled").
           Once that thread is done, it may have taken care of all the
           pending calls, or there might be some still waiting.
           Regardless, this interpreter's pending calls will stay
           "signaled" until that first thread has finished.  At that
           point the next thread to trip the eval breaker will take
           care of any remaining pending calls.  Until then, though,
           all the interpreter's threads will be tripping the eval
           breaker every time it's checked. */
        PyThread_release_lock(pending->lock);
        return 0;
    }
    pending->busy = 1;
    PyThread_release_lock(pending->lock);

    /* unsignal before starting to call callbacks, so that any callback
       added in-between re-signals */
    UNSIGNAL_PENDING_CALLS(interp);

    if (_make_pending_calls(pending) != 0) {
        pending->busy = 0;
        /* There might not be more calls to make, but we play it safe. */
        SIGNAL_PENDING_CALLS(pending, interp);
        return -1;
    }

    if (_Py_IsMainThread() && _Py_IsMainInterpreter(interp)) {
        if (_make_pending_calls(pending_main) != 0) {
            pending->busy = 0;
            /* There might not be more calls to make, but we play it safe. */
            SIGNAL_PENDING_CALLS(pending_main, interp);
            return -1;
        }
    }

    pending->busy = 0;
    return 0;
}

void
_Py_FinishPendingCalls(PyThreadState *tstate)
{
    assert(PyGILState_Check());
    assert(_PyThreadState_CheckConsistency(tstate));

    if (make_pending_calls(tstate->interp) < 0) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        PyErr_BadInternalCall();
        _PyErr_ChainExceptions1(exc);
        _PyErr_Print(tstate);
    }
}

int
_PyEval_MakePendingCalls(PyThreadState *tstate)
{
    int res;

    if (_Py_IsMainThread() && _Py_IsMainInterpreter(tstate->interp)) {
        /* Python signal handler doesn't really queue a callback:
           it only signals that a signal was received,
           see _PyEval_SignalReceived(). */
        res = handle_signals(tstate);
        if (res != 0) {
            return res;
        }
    }

    res = make_pending_calls(tstate->interp);
    if (res != 0) {
        return res;
    }

    return 0;
}

/* Py_MakePendingCalls() is a simple wrapper for the sake
   of backward-compatibility. */
int
Py_MakePendingCalls(void)
{
    assert(PyGILState_Check());

    PyThreadState *tstate = _PyThreadState_GET();
    assert(_PyThreadState_CheckConsistency(tstate));

    /* Only execute pending calls on the main thread. */
    if (!_Py_IsMainThread() || !_Py_IsMainInterpreter(tstate->interp)) {
        return 0;
    }
    return _PyEval_MakePendingCalls(tstate);
}

void
_PyEval_InitState(PyInterpreterState *interp, PyThread_type_lock pending_lock)
{
    _gil_initialize(&interp->_gil);

    struct _pending_calls *pending = &interp->ceval.pending;
    assert(pending->lock == NULL);
    pending->lock = pending_lock;
}

void
_PyEval_FiniState(struct _ceval_state *ceval)
{
    struct _pending_calls *pending = &ceval->pending;
    if (pending->lock != NULL) {
        PyThread_free_lock(pending->lock);
        pending->lock = NULL;
    }
}

/* Handle signals, pending calls, GIL drop request
   and asynchronous exception */
int
_Py_HandlePending(PyThreadState *tstate)
{
    _PyRuntimeState * const runtime = &_PyRuntime;
    struct _ceval_runtime_state *ceval = &runtime->ceval;
    struct _ceval_state *interp_ceval_state = &tstate->interp->ceval;

    /* Pending signals */
    if (_Py_atomic_load_relaxed_int32(&ceval->signals_pending)) {
        if (handle_signals(tstate) != 0) {
            return -1;
        }
    }

    /* Pending calls */
    if (maybe_has_pending_calls(tstate->interp)) {
        if (make_pending_calls(tstate->interp) != 0) {
            return -1;
        }
    }

    /* GC scheduled to run */
    if (_Py_atomic_load_relaxed_int32(&interp_ceval_state->gc_scheduled)) {
        _Py_atomic_store_relaxed(&interp_ceval_state->gc_scheduled, 0);
        COMPUTE_EVAL_BREAKER(tstate->interp, ceval, interp_ceval_state);
        _Py_RunGC(tstate);
    }

    /* GIL drop request */
    if (_Py_atomic_load_relaxed_int32(&interp_ceval_state->gil_drop_request)) {
        /* Give another thread a chance */
        if (_PyThreadState_SwapNoGIL(NULL) != tstate) {
            Py_FatalError("tstate mix-up");
        }
        drop_gil(interp_ceval_state, tstate);

        /* Other threads may run now */

        take_gil(tstate);

        if (_PyThreadState_SwapNoGIL(tstate) != NULL) {
            Py_FatalError("orphan tstate");
        }
    }

    /* Check for asynchronous exception. */
    if (tstate->async_exc != NULL) {
        PyObject *exc = tstate->async_exc;
        tstate->async_exc = NULL;
        UNSIGNAL_ASYNC_EXC(tstate->interp);
        _PyErr_SetNone(tstate, exc);
        Py_DECREF(exc);
        return -1;
    }


    // It is possible that some of the conditions that trigger the eval breaker
    // are called in a different thread than the Python thread. An example of
    // this is bpo-42296: On Windows, _PyEval_SignalReceived() can be called in
    // a different thread than the Python thread, in which case
    // _Py_ThreadCanHandleSignals() is wrong. Recompute eval_breaker in the
    // current Python thread with the correct _Py_ThreadCanHandleSignals()
    // value. It prevents to interrupt the eval loop at every instruction if
    // the current Python thread cannot handle signals (if
    // _Py_ThreadCanHandleSignals() is false).
    COMPUTE_EVAL_BREAKER(tstate->interp, ceval, interp_ceval_state);

    return 0;
}

