#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_SignalReceived()
#include "pycore_gc.h"            // _Py_RunGC()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_optimizer.h"     // _Py_Executors_InvalidateCold()
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pylifecycle.h"   // _PyErr_Print()
#include "pycore_pystats.h"       // _Py_PrintSpecializationStats()
#include "pycore_runtime.h"       // _PyRuntime


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

// Atomically copy the bits indicated by mask between two values.
static inline void
copy_eval_breaker_bits(uintptr_t *from, uintptr_t *to, uintptr_t mask)
{
    uintptr_t from_bits = _Py_atomic_load_uintptr_relaxed(from) & mask;
    uintptr_t old_value = _Py_atomic_load_uintptr_relaxed(to);
    uintptr_t to_bits = old_value & mask;
    if (from_bits == to_bits) {
        return;
    }

    uintptr_t new_value;
    do {
        new_value = (old_value & ~mask) | from_bits;
    } while (!_Py_atomic_compare_exchange_uintptr(to, &old_value, new_value));
}

// When attaching a thread, set the global instrumentation version and
// _PY_CALLS_TO_DO_BIT from the current state of the interpreter.
static inline void
update_eval_breaker_for_thread(PyInterpreterState *interp, PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    // Free-threaded builds eagerly update the eval_breaker on *all* threads as
    // needed, so this function doesn't apply.
    return;
#endif

    int32_t npending = _Py_atomic_load_int32_relaxed(
        &interp->ceval.pending.npending);
    if (npending) {
        _Py_set_eval_breaker_bit(tstate, _PY_CALLS_TO_DO_BIT);
    }
    else if (_Py_IsMainThread()) {
        npending = _Py_atomic_load_int32_relaxed(
            &_PyRuntime.ceval.pending_mainthread.npending);
        if (npending) {
            _Py_set_eval_breaker_bit(tstate, _PY_CALLS_TO_DO_BIT);
        }
    }

    // _PY_CALLS_TO_DO_BIT was derived from other state above, so the only bits
    // we copy from our interpreter's state are the instrumentation version.
    copy_eval_breaker_bits(&interp->ceval.instrumentation_version,
                           &tstate->eval_breaker,
                           ~_PY_EVAL_EVENTS_MASK);
}

/*
 * Implementation of the Global Interpreter Lock (GIL).
 */

#include <stdlib.h>
#include <errno.h>

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
    gil->locked = -1;
    gil->interval = DEFAULT_INTERVAL;
}

static int gil_created(struct _gil_runtime_state *gil)
{
    if (gil == NULL) {
        return 0;
    }
    return (_Py_atomic_load_int_acquire(&gil->locked) >= 0);
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
    _Py_atomic_store_ptr_relaxed(&gil->last_holder, 0);
    _Py_ANNOTATE_RWLOCK_CREATE(&gil->locked);
    _Py_atomic_store_int_release(&gil->locked, 0);
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
    _Py_atomic_store_int_release(&gil->locked, -1);
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

static inline void
drop_gil_impl(PyThreadState *tstate, struct _gil_runtime_state *gil)
{
    MUTEX_LOCK(gil->mutex);
    _Py_ANNOTATE_RWLOCK_RELEASED(&gil->locked, /*is_write=*/1);
    _Py_atomic_store_int_relaxed(&gil->locked, 0);
    if (tstate != NULL) {
        tstate->holds_gil = 0;
    }
    COND_SIGNAL(gil->cond);
    MUTEX_UNLOCK(gil->mutex);
}

static void
drop_gil(PyInterpreterState *interp, PyThreadState *tstate, int final_release)
{
    struct _ceval_state *ceval = &interp->ceval;
    /* If final_release is true, the caller is indicating that we're releasing
       the GIL for the last time in this thread.  This is particularly
       relevant when the current thread state is finalizing or its
       interpreter is finalizing (either may be in an inconsistent
       state).  In that case the current thread will definitely
       never try to acquire the GIL again. */
    // XXX It may be more correct to check tstate->_status.finalizing.
    // XXX assert(final_release || !tstate->_status.cleared);

    assert(final_release || tstate != NULL);
    struct _gil_runtime_state *gil = ceval->gil;
#ifdef Py_GIL_DISABLED
    // Check if we have the GIL before dropping it. tstate will be NULL if
    // take_gil() detected that this thread has been destroyed, in which case
    // we know we have the GIL.
    if (tstate != NULL && !tstate->holds_gil) {
        return;
    }
#endif
    if (!_Py_atomic_load_int_relaxed(&gil->locked)) {
        Py_FatalError("drop_gil: GIL is not locked");
    }

    if (!final_release) {
        /* Sub-interpreter support: threads might have been switched
           under our feet using PyThreadState_Swap(). Fix the GIL last
           holder variable so that our heuristics work. */
        _Py_atomic_store_ptr_relaxed(&gil->last_holder, tstate);
    }

    drop_gil_impl(tstate, gil);

#ifdef FORCE_SWITCHING
    /* We might be releasing the GIL for the last time in this thread.  In that
       case there's a possible race with tstate->interp getting deleted after
       gil->mutex is unlocked and before the following code runs, leading to a
       crash.  We can use final_release to indicate the thread is done with the
       GIL, and that's the only time we might delete the interpreter.  See
       https://github.com/python/cpython/issues/104341. */
    if (!final_release &&
        _Py_eval_breaker_bit_is_set(tstate, _PY_GIL_DROP_REQUEST_BIT)) {
        MUTEX_LOCK(gil->switch_mutex);
        /* Not switched yet => wait */
        if (((PyThreadState*)_Py_atomic_load_ptr_relaxed(&gil->last_holder)) == tstate)
        {
            assert(_PyThreadState_CheckConsistency(tstate));
            _Py_unset_eval_breaker_bit(tstate, _PY_GIL_DROP_REQUEST_BIT);
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
   It may hang rather than return if the interpreter has been finalized.

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
           thread which called Py_Finalize(), this thread cannot continue.

           This code path can be reached by a daemon thread after Py_Finalize()
           completes.

           This used to call a *thread_exit API, but that was not safe as it
           lacks stack unwinding and local variable destruction important to
           C++. gh-87135: The best that can be done is to hang the thread as
           the public APIs calling this have no error reporting mechanism (!).
         */
        _PyThreadState_HangThread(tstate);
    }

    assert(_PyThreadState_CheckConsistency(tstate));
    PyInterpreterState *interp = tstate->interp;
    struct _gil_runtime_state *gil = interp->ceval.gil;
#ifdef Py_GIL_DISABLED
    if (!_Py_atomic_load_int_relaxed(&gil->enabled)) {
        return;
    }
#endif

    /* Check that _PyEval_InitThreads() was called to create the lock */
    assert(gil_created(gil));

    MUTEX_LOCK(gil->mutex);

    int drop_requested = 0;
    while (_Py_atomic_load_int_relaxed(&gil->locked)) {
        unsigned long saved_switchnum = gil->switch_number;

        unsigned long interval = _Py_atomic_load_ulong_relaxed(&gil->interval);
        if (interval < 1) {
            interval = 1;
        }
        int timed_out = 0;
        COND_TIMED_WAIT(gil->cond, gil->mutex, interval, timed_out);

        /* If we timed out and no switch occurred in the meantime, it is time
           to ask the GIL-holding thread to drop it. */
        if (timed_out &&
            _Py_atomic_load_int_relaxed(&gil->locked) &&
            gil->switch_number == saved_switchnum)
        {
            PyThreadState *holder_tstate =
                (PyThreadState*)_Py_atomic_load_ptr_relaxed(&gil->last_holder);
            if (_PyThreadState_MustExit(tstate)) {
                MUTEX_UNLOCK(gil->mutex);
                // gh-96387: If the loop requested a drop request in a previous
                // iteration, reset the request. Otherwise, drop_gil() can
                // block forever waiting for the thread which exited. Drop
                // requests made by other threads are also reset: these threads
                // may have to request again a drop request (iterate one more
                // time).
                if (drop_requested) {
                    _Py_unset_eval_breaker_bit(holder_tstate, _PY_GIL_DROP_REQUEST_BIT);
                }
                // gh-87135: hang the thread as *thread_exit() is not a safe
                // API. It lacks stack unwind and local variable destruction.
                _PyThreadState_HangThread(tstate);
            }
            assert(_PyThreadState_CheckConsistency(tstate));

            _Py_set_eval_breaker_bit(holder_tstate, _PY_GIL_DROP_REQUEST_BIT);
            drop_requested = 1;
        }
    }

#ifdef Py_GIL_DISABLED
    if (!_Py_atomic_load_int_relaxed(&gil->enabled)) {
        // Another thread disabled the GIL between our check above and
        // now. Don't take the GIL, signal any other waiting threads, and
        // return.
        COND_SIGNAL(gil->cond);
        MUTEX_UNLOCK(gil->mutex);
        return;
    }
#endif

#ifdef FORCE_SWITCHING
    /* This mutex must be taken before modifying gil->last_holder:
       see drop_gil(). */
    MUTEX_LOCK(gil->switch_mutex);
#endif
    /* We now hold the GIL */
    _Py_atomic_store_int_relaxed(&gil->locked, 1);
    _Py_ANNOTATE_RWLOCK_ACQUIRED(&gil->locked, /*is_write=*/1);

    if (tstate != (PyThreadState*)_Py_atomic_load_ptr_relaxed(&gil->last_holder)) {
        _Py_atomic_store_ptr_relaxed(&gil->last_holder, tstate);
        ++gil->switch_number;
    }

#ifdef FORCE_SWITCHING
    COND_SIGNAL(gil->switch_cond);
    MUTEX_UNLOCK(gil->switch_mutex);
#endif

    if (_PyThreadState_MustExit(tstate)) {
        /* bpo-36475: If Py_Finalize() has been called and tstate is not
           the thread which called Py_Finalize(), gh-87135: hang the
           thread.

           This code path can be reached by a daemon thread which was waiting
           in take_gil() while the main thread called
           wait_for_thread_shutdown() from Py_Finalize(). */
        MUTEX_UNLOCK(gil->mutex);
        /* tstate could be a dangling pointer, so don't pass it to
           drop_gil(). */
        drop_gil(interp, NULL, 1);
        _PyThreadState_HangThread(tstate);
    }
    assert(_PyThreadState_CheckConsistency(tstate));

    tstate->holds_gil = 1;
    _Py_unset_eval_breaker_bit(tstate, _PY_GIL_DROP_REQUEST_BIT);
    update_eval_breaker_for_thread(interp, tstate);

    MUTEX_UNLOCK(gil->mutex);

    errno = err;
    return;
}

void _PyEval_SetSwitchInterval(unsigned long microseconds)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _gil_runtime_state *gil = interp->ceval.gil;
    assert(gil != NULL);
    _Py_atomic_store_ulong_relaxed(&gil->interval, microseconds);
}

unsigned long _PyEval_GetSwitchInterval(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _gil_runtime_state *gil = interp->ceval.gil;
    assert(gil != NULL);
    return _Py_atomic_load_ulong_relaxed(&gil->interval);
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

// Function removed in the Python 3.13 API but kept in the stable ABI.
PyAPI_FUNC(int)
PyEval_ThreadsInitialized(void)
{
    return _PyEval_ThreadsInitialized();
}

#ifndef NDEBUG
static inline int
current_thread_holds_gil(struct _gil_runtime_state *gil, PyThreadState *tstate)
{
    int holds_gil = tstate->holds_gil;

    // holds_gil is the source of truth; check that last_holder and gil->locked
    // are consistent with it.
    int locked = _Py_atomic_load_int_relaxed(&gil->locked);
    int is_last_holder =
        ((PyThreadState*)_Py_atomic_load_ptr_relaxed(&gil->last_holder)) == tstate;
    assert(!holds_gil || locked);
    assert(!holds_gil || is_last_holder);

    return holds_gil;
}
#endif

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
#ifdef Py_GIL_DISABLED
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);
    gil->enabled = config->enable_gil == _PyConfig_GIL_ENABLE ? INT_MAX : 0;
#endif
    create_gil(gil);
    assert(gil_created(gil));
    interp->ceval.gil = gil;
    interp->ceval.own_gil = 1;
}

void
_PyEval_InitGIL(PyThreadState *tstate, int own_gil)
{
    assert(tstate->interp->ceval.gil == NULL);
    if (!own_gil) {
        /* The interpreter will share the main interpreter's instead. */
        PyInterpreterState *main_interp = _PyInterpreterState_Main();
        assert(tstate->interp != main_interp);
        struct _gil_runtime_state *gil = main_interp->ceval.gil;
        init_shared_gil(tstate->interp, gil);
        assert(!current_thread_holds_gil(gil, tstate));
    }
    else {
        PyThread_init_thread();
        init_own_gil(tstate->interp, &tstate->interp->_gil);
    }

    // Lock the GIL and mark the current thread as attached.
    _PyThreadState_Attach(tstate);
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

// Function removed in the Python 3.13 API but kept in the stable ABI.
PyAPI_FUNC(void)
PyEval_AcquireLock(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);

    take_gil(tstate);
}

// Function removed in the Python 3.13 API but kept in the stable ABI.
PyAPI_FUNC(void)
PyEval_ReleaseLock(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    /* This function must succeed when the current thread state is NULL.
       We therefore avoid PyThreadState_Get() which dumps a fatal error
       in debug mode. */
    drop_gil(tstate->interp, tstate, 0);
}

void
_PyEval_AcquireLock(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);
    take_gil(tstate);
}

void
_PyEval_ReleaseLock(PyInterpreterState *interp,
                    PyThreadState *tstate,
                    int final_release)
{
    assert(tstate != NULL);
    assert(tstate->interp == interp);
    drop_gil(interp, tstate, final_release);
}

void
PyEval_AcquireThread(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);
    _PyThreadState_Attach(tstate);
}

void
PyEval_ReleaseThread(PyThreadState *tstate)
{
    assert(_PyThreadState_CheckConsistency(tstate));
    _PyThreadState_Detach(tstate);
}

#ifdef HAVE_FORK
/* This function is called from PyOS_AfterFork_Child to re-initialize the
   GIL and pending calls lock. */
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
    _PyMutex_at_fork_reinit(&pending->mutex);

    return _PyStatus_OK();
}
#endif

PyThreadState *
PyEval_SaveThread(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyThreadState_Detach(tstate);
    return tstate;
}

void
PyEval_RestoreThread(PyThreadState *tstate)
{
#ifdef MS_WINDOWS
    int err = GetLastError();
#endif

    _Py_EnsureTstateNotNULL(tstate);
    _PyThreadState_Attach(tstate);

#ifdef MS_WINDOWS
    SetLastError(err);
#endif
}


void
_PyEval_SignalReceived(void)
{
    _Py_set_eval_breaker_bit(_PyRuntime.main_tstate, _PY_SIGNALS_PENDING_BIT);
}


#ifndef Py_GIL_DISABLED
static void
signal_active_thread(PyInterpreterState *interp, uintptr_t bit)
{
    struct _gil_runtime_state *gil = interp->ceval.gil;

    // If a thread from the targeted interpreter is holding the GIL, signal
    // that thread. Otherwise, the next thread to run from the targeted
    // interpreter will have its bit set as part of taking the GIL.
    MUTEX_LOCK(gil->mutex);
    if (_Py_atomic_load_int_relaxed(&gil->locked)) {
        PyThreadState *holder = (PyThreadState*)_Py_atomic_load_ptr_relaxed(&gil->last_holder);
        if (holder->interp == interp) {
            _Py_set_eval_breaker_bit(holder, bit);
        }
    }
    MUTEX_UNLOCK(gil->mutex);
}
#endif


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

/* Push one item onto the queue while holding the lock. */
static int
_push_pending_call(struct _pending_calls *pending,
                   _Py_pending_call_func func, void *arg, int flags)
{
    if (pending->npending == pending->max) {
        return _Py_ADD_PENDING_FULL;
    }
    assert(pending->npending < pending->max);

    int i = pending->next;
    assert(pending->calls[i].func == NULL);

    pending->calls[i].func = func;
    pending->calls[i].arg = arg;
    pending->calls[i].flags = flags;

    assert(pending->npending < PENDINGCALLSARRAYSIZE);
    _Py_atomic_add_int32(&pending->npending, 1);

    pending->next = (i + 1) % PENDINGCALLSARRAYSIZE;
    assert(pending->next != pending->first
            || pending->npending == pending->max);

    return _Py_ADD_PENDING_SUCCESS;
}

static int
_next_pending_call(struct _pending_calls *pending,
                   int (**func)(void *), void **arg, int *flags)
{
    int i = pending->first;
    if (pending->npending == 0) {
        /* Queue empty */
        assert(i == pending->next);
        assert(pending->calls[i].func == NULL);
        return -1;
    }
    *func = pending->calls[i].func;
    *arg = pending->calls[i].arg;
    *flags = pending->calls[i].flags;
    return i;
}

/* Pop one item off the queue while holding the lock. */
static void
_pop_pending_call(struct _pending_calls *pending,
                  int (**func)(void *), void **arg, int *flags)
{
    int i = _next_pending_call(pending, func, arg, flags);
    if (i >= 0) {
        pending->calls[i] = (struct _pending_call){0};
        pending->first = (i + 1) % PENDINGCALLSARRAYSIZE;
        assert(pending->npending > 0);
        _Py_atomic_add_int32(&pending->npending, -1);
    }
}

/* This implementation is thread-safe.  It allows
   scheduling to be made from any thread, and even from an executing
   callback.
 */

_Py_add_pending_call_result
_PyEval_AddPendingCall(PyInterpreterState *interp,
                       _Py_pending_call_func func, void *arg, int flags)
{
    struct _pending_calls *pending = &interp->ceval.pending;
    int main_only = (flags & _Py_PENDING_MAINTHREADONLY) != 0;
    if (main_only) {
        /* The main thread only exists in the main interpreter. */
        assert(_Py_IsMainInterpreter(interp));
        pending = &_PyRuntime.ceval.pending_mainthread;
    }

    PyMutex_Lock(&pending->mutex);
    _Py_add_pending_call_result result =
        _push_pending_call(pending, func, arg, flags);
    PyMutex_Unlock(&pending->mutex);

    if (main_only) {
        _Py_set_eval_breaker_bit(_PyRuntime.main_tstate, _PY_CALLS_TO_DO_BIT);
    }
    else {
#ifdef Py_GIL_DISABLED
        _Py_set_eval_breaker_bit_all(interp, _PY_CALLS_TO_DO_BIT);
#else
        signal_active_thread(interp, _PY_CALLS_TO_DO_BIT);
#endif
    }

    return result;
}

int
Py_AddPendingCall(_Py_pending_call_func func, void *arg)
{
    /* Legacy users of this API will continue to target the main thread
       (of the main interpreter). */
    PyInterpreterState *interp = _PyInterpreterState_Main();
    _Py_add_pending_call_result r =
        _PyEval_AddPendingCall(interp, func, arg, _Py_PENDING_MAINTHREADONLY);
    if (r == _Py_ADD_PENDING_FULL) {
        return -1;
    }
    else {
        assert(r == _Py_ADD_PENDING_SUCCESS);
        return 0;
    }
}

static int
handle_signals(PyThreadState *tstate)
{
    assert(_PyThreadState_CheckConsistency(tstate));
    _Py_unset_eval_breaker_bit(tstate, _PY_SIGNALS_PENDING_BIT);
    if (!_Py_ThreadCanHandleSignals(tstate->interp)) {
        return 0;
    }
    if (_PyErr_CheckSignalsTstate(tstate) < 0) {
        /* On failure, re-schedule a call to handle_signals(). */
        _Py_set_eval_breaker_bit(tstate, _PY_SIGNALS_PENDING_BIT);
        return -1;
    }
    return 0;
}

static int
_make_pending_calls(struct _pending_calls *pending, int32_t *p_npending)
{
    int res = 0;
    int32_t npending = -1;

    assert(sizeof(pending->max) <= sizeof(size_t)
            && ((size_t)pending->max) <= Py_ARRAY_LENGTH(pending->calls));
    int32_t maxloop = pending->maxloop;
    if (maxloop == 0) {
        maxloop = pending->max;
    }
    assert(maxloop > 0 && maxloop <= pending->max);

    /* perform a bounded number of calls, in case of recursion */
    for (int i=0; i<maxloop; i++) {
        _Py_pending_call_func func = NULL;
        void *arg = NULL;
        int flags = 0;

        /* pop one item off the queue while holding the lock */
        PyMutex_Lock(&pending->mutex);
        _pop_pending_call(pending, &func, &arg, &flags);
        npending = pending->npending;
        PyMutex_Unlock(&pending->mutex);

        /* Check if there are any more pending calls. */
        if (func == NULL) {
            assert(npending == 0);
            break;
        }

        /* having released the lock, perform the callback */
        res = func(arg);
        if ((flags & _Py_PENDING_RAWFREE) && arg != NULL) {
            PyMem_RawFree(arg);
        }
        if (res != 0) {
            res = -1;
            goto finally;
        }
    }

finally:
    *p_npending = npending;
    return res;
}

static void
signal_pending_calls(PyThreadState *tstate, PyInterpreterState *interp)
{
#ifdef Py_GIL_DISABLED
    _Py_set_eval_breaker_bit_all(interp, _PY_CALLS_TO_DO_BIT);
#else
    _Py_set_eval_breaker_bit(tstate, _PY_CALLS_TO_DO_BIT);
#endif
}

static void
unsignal_pending_calls(PyThreadState *tstate, PyInterpreterState *interp)
{
#ifdef Py_GIL_DISABLED
    _Py_unset_eval_breaker_bit_all(interp, _PY_CALLS_TO_DO_BIT);
#else
    _Py_unset_eval_breaker_bit(tstate, _PY_CALLS_TO_DO_BIT);
#endif
}

static void
clear_pending_handling_thread(struct _pending_calls *pending)
{
#ifdef Py_GIL_DISABLED
    PyMutex_Lock(&pending->mutex);
    pending->handling_thread = NULL;
    PyMutex_Unlock(&pending->mutex);
#else
    pending->handling_thread = NULL;
#endif
}

static int
make_pending_calls(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    struct _pending_calls *pending = &interp->ceval.pending;
    struct _pending_calls *pending_main = &_PyRuntime.ceval.pending_mainthread;

    /* Only one thread (per interpreter) may run the pending calls
       at once.  In the same way, we don't do recursive pending calls. */
    PyMutex_Lock(&pending->mutex);
    if (pending->handling_thread != NULL) {
        /* A pending call was added after another thread was already
           handling the pending calls (and had already "unsignaled").
           Once that thread is done, it may have taken care of all the
           pending calls, or there might be some still waiting.
           To avoid all threads constantly stopping on the eval breaker,
           we clear the bit for this thread and make sure it is set
           for the thread currently handling the pending call. */
        _Py_set_eval_breaker_bit(pending->handling_thread, _PY_CALLS_TO_DO_BIT);
        _Py_unset_eval_breaker_bit(tstate, _PY_CALLS_TO_DO_BIT);
        PyMutex_Unlock(&pending->mutex);
        return 0;
    }
    pending->handling_thread = tstate;
    PyMutex_Unlock(&pending->mutex);

    /* unsignal before starting to call callbacks, so that any callback
       added in-between re-signals */
    unsignal_pending_calls(tstate, interp);

    int32_t npending;
    if (_make_pending_calls(pending, &npending) != 0) {
        clear_pending_handling_thread(pending);
        /* There might not be more calls to make, but we play it safe. */
        signal_pending_calls(tstate, interp);
        return -1;
    }
    if (npending > 0) {
        /* We hit pending->maxloop. */
        signal_pending_calls(tstate, interp);
    }

    if (_Py_IsMainThread() && _Py_IsMainInterpreter(interp)) {
        if (_make_pending_calls(pending_main, &npending) != 0) {
            clear_pending_handling_thread(pending);
            /* There might not be more calls to make, but we play it safe. */
            signal_pending_calls(tstate, interp);
            return -1;
        }
        if (npending > 0) {
            /* We hit pending_main->maxloop. */
            signal_pending_calls(tstate, interp);
        }
    }

    clear_pending_handling_thread(pending);
    return 0;
}


void
_Py_set_eval_breaker_bit_all(PyInterpreterState *interp, uintptr_t bit)
{
    _Py_FOR_EACH_TSTATE_BEGIN(interp, tstate) {
        _Py_set_eval_breaker_bit(tstate, bit);
    }
    _Py_FOR_EACH_TSTATE_END(interp);
}

void
_Py_unset_eval_breaker_bit_all(PyInterpreterState *interp, uintptr_t bit)
{
    _Py_FOR_EACH_TSTATE_BEGIN(interp, tstate) {
        _Py_unset_eval_breaker_bit(tstate, bit);
    }
    _Py_FOR_EACH_TSTATE_END(interp);
}

void
_Py_FinishPendingCalls(PyThreadState *tstate)
{
    _Py_AssertHoldsTstate();
    assert(_PyThreadState_CheckConsistency(tstate));

    struct _pending_calls *pending = &tstate->interp->ceval.pending;
    struct _pending_calls *pending_main =
            _Py_IsMainThread() && _Py_IsMainInterpreter(tstate->interp)
            ? &_PyRuntime.ceval.pending_mainthread
            : NULL;
    /* make_pending_calls() may return early without making all pending
       calls, so we keep trying until we're actually done. */
    int32_t npending;
#ifndef NDEBUG
    int32_t npending_prev = INT32_MAX;
#endif
    do {
        if (make_pending_calls(tstate) < 0) {
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            PyErr_BadInternalCall();
            _PyErr_ChainExceptions1(exc);
            _PyErr_Print(tstate);
        }

        npending = _Py_atomic_load_int32_relaxed(&pending->npending);
        if (pending_main != NULL) {
            npending += _Py_atomic_load_int32_relaxed(&pending_main->npending);
        }
#ifndef NDEBUG
        assert(npending_prev > npending);
        npending_prev = npending;
#endif
    } while (npending > 0);
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

    res = make_pending_calls(tstate);
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
    _Py_AssertHoldsTstate();

    PyThreadState *tstate = _PyThreadState_GET();
    assert(_PyThreadState_CheckConsistency(tstate));

    /* Only execute pending calls on the main thread. */
    if (!_Py_IsMainThread() || !_Py_IsMainInterpreter(tstate->interp)) {
        return 0;
    }
    return _PyEval_MakePendingCalls(tstate);
}

void
_PyEval_InitState(PyInterpreterState *interp)
{
    _gil_initialize(&interp->_gil);
}

#ifdef Py_GIL_DISABLED
int
_PyEval_EnableGILTransient(PyThreadState *tstate)
{
    const PyConfig *config = _PyInterpreterState_GetConfig(tstate->interp);
    if (config->enable_gil != _PyConfig_GIL_DEFAULT) {
        return 0;
    }
    struct _gil_runtime_state *gil = tstate->interp->ceval.gil;

    int enabled = _Py_atomic_load_int_relaxed(&gil->enabled);
    if (enabled == INT_MAX) {
        // The GIL is already enabled permanently.
        return 0;
    }
    if (enabled == INT_MAX - 1) {
        Py_FatalError("Too many transient requests to enable the GIL");
    }
    if (enabled > 0) {
        // If enabled is nonzero, we know we hold the GIL. This means that no
        // other threads are attached, and nobody else can be concurrently
        // mutating it.
        _Py_atomic_store_int_relaxed(&gil->enabled, enabled + 1);
        return 0;
    }

    // Enabling the GIL changes what it means to be an "attached" thread. To
    // safely make this transition, we:
    // 1. Detach the current thread.
    // 2. Stop the world to detach (and suspend) all other threads.
    // 3. Enable the GIL, if nobody else did between our check above and when
    //    our stop-the-world begins.
    // 4. Start the world.
    // 5. Attach the current thread. Other threads may attach and hold the GIL
    //    before this thread, which is harmless.
    _PyThreadState_Detach(tstate);

    // This could be an interpreter-local stop-the-world in situations where we
    // know that this interpreter's GIL is not shared, and that it won't become
    // shared before the stop-the-world begins. For now, we always stop all
    // interpreters for simplicity.
    _PyEval_StopTheWorldAll(&_PyRuntime);

    enabled = _Py_atomic_load_int_relaxed(&gil->enabled);
    int this_thread_enabled = enabled == 0;
    _Py_atomic_store_int_relaxed(&gil->enabled, enabled + 1);

    _PyEval_StartTheWorldAll(&_PyRuntime);
    _PyThreadState_Attach(tstate);

    return this_thread_enabled;
}

int
_PyEval_EnableGILPermanent(PyThreadState *tstate)
{
    const PyConfig *config = _PyInterpreterState_GetConfig(tstate->interp);
    if (config->enable_gil != _PyConfig_GIL_DEFAULT) {
        return 0;
    }

    struct _gil_runtime_state *gil = tstate->interp->ceval.gil;
    assert(current_thread_holds_gil(gil, tstate));

    int enabled = _Py_atomic_load_int_relaxed(&gil->enabled);
    if (enabled == INT_MAX) {
        return 0;
    }

    _Py_atomic_store_int_relaxed(&gil->enabled, INT_MAX);
    return 1;
}

int
_PyEval_DisableGIL(PyThreadState *tstate)
{
    const PyConfig *config = _PyInterpreterState_GetConfig(tstate->interp);
    if (config->enable_gil != _PyConfig_GIL_DEFAULT) {
        return 0;
    }

    struct _gil_runtime_state *gil = tstate->interp->ceval.gil;
    assert(current_thread_holds_gil(gil, tstate));

    int enabled = _Py_atomic_load_int_relaxed(&gil->enabled);
    if (enabled == INT_MAX) {
        return 0;
    }

    assert(enabled >= 1);
    enabled--;

    // Disabling the GIL is much simpler than enabling it, since we know we are
    // the only attached thread. Other threads may start free-threading as soon
    // as this store is complete, if it sets gil->enabled to 0.
    _Py_atomic_store_int_relaxed(&gil->enabled, enabled);

    if (enabled == 0) {
        // We're attached, so we know the GIL will remain disabled until at
        // least the next time we detach, which must be after this function
        // returns.
        //
        // Drop the GIL, which will wake up any threads waiting in take_gil()
        // and let them resume execution without the GIL.
        drop_gil_impl(tstate, gil);

        // If another thread asked us to drop the GIL, they should be
        // free-threading by now. Remove any such request so we have a clean
        // slate if/when the GIL is enabled again.
        _Py_unset_eval_breaker_bit(tstate, _PY_GIL_DROP_REQUEST_BIT);
        return 1;
    }
    return 0;
}
#endif

#if defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)
// Note that this function is inline to avoid creating a PLT entry
// that would be an easy target for a ROP gadget.
static inline int run_remote_debugger_source(PyObject *source)
{
    const char *str = PyBytes_AsString(source);
    if (!str) {
        return -1;
    }

    PyObject *ns = PyDict_New();
    if (!ns) {
        return -1;
    }

    PyObject *res = PyRun_String(str, Py_file_input, ns, ns);
    Py_DECREF(ns);
    if (!res) {
        return -1;
    }
    Py_DECREF(res);
    return 0;
}

// Note that this function is inline to avoid creating a PLT entry
// that would be an easy target for a ROP gadget.
static inline void run_remote_debugger_script(PyObject *path)
{
    if (0 != PySys_Audit("remote_debugger_script", "O", path)) {
        PyErr_FormatUnraisable(
            "Audit hook failed for remote debugger script %U", path);
        return;
    }

    // Open the debugger script with the open code hook, and reopen the
    // resulting file object to get a C FILE* object.
    PyObject* fileobj = PyFile_OpenCodeObject(path);
    if (!fileobj) {
        PyErr_FormatUnraisable("Can't open debugger script %U", path);
        return;
    }

    PyObject* source = PyObject_CallMethodNoArgs(fileobj, &_Py_ID(read));
    if (!source) {
        PyErr_FormatUnraisable("Error reading debugger script %U", path);
    }

    PyObject* res = PyObject_CallMethodNoArgs(fileobj, &_Py_ID(close));
    if (!res) {
        PyErr_FormatUnraisable("Error closing debugger script %U", path);
    } else {
        Py_DECREF(res);
    }
    Py_DECREF(fileobj);

    if (source) {
        if (0 != run_remote_debugger_source(source)) {
            PyErr_FormatUnraisable("Error executing debugger script %U", path);
        }
        Py_DECREF(source);
    }
}

int _PyRunRemoteDebugger(PyThreadState *tstate)
{
    const PyConfig *config = _PyInterpreterState_GetConfig(tstate->interp);
    if (config->remote_debug == 1
         && tstate->remote_debugger_support.debugger_pending_call == 1)
    {
        tstate->remote_debugger_support.debugger_pending_call = 0;

        // Immediately make a copy in case of a race with another debugger
        // process that's trying to write to the buffer. At least this way
        // we'll be internally consistent: what we audit is what we run.
        const size_t pathsz
            = sizeof(tstate->remote_debugger_support.debugger_script_path);

        char *path = PyMem_Malloc(pathsz);
        if (path) {
            // And don't assume the debugger correctly null terminated it.
            memcpy(
                path,
                tstate->remote_debugger_support.debugger_script_path,
                pathsz);
            path[pathsz - 1] = '\0';
            if (*path) {
                PyObject *path_obj = PyUnicode_DecodeFSDefault(path);
                if (path_obj == NULL) {
                    PyErr_FormatUnraisable("Can't decode debugger script");
                }
                else {
                    run_remote_debugger_script(path_obj);
                    Py_DECREF(path_obj);
                }
            }
            PyMem_Free(path);
        }
    }
    return 0;
}

#endif

/* Do periodic things, like check for signals and async I/0.
* We need to do reasonably frequently, but not too frequently.
* All loops should include a check of the eval breaker.
* We also check on return from any builtin function.
*
* ## More Details ###
*
* The eval loop (this function) normally executes the instructions
* of a code object sequentially.  However, the runtime supports a
* number of out-of-band execution scenarios that may pause that
* sequential execution long enough to do that out-of-band work
* in the current thread using the current PyThreadState.
*
* The scenarios include:
*
*  - cyclic garbage collection
*  - GIL drop requests
*  - "async" exceptions
*  - "pending calls"  (some only in the main thread)
*  - signal handling (only in the main thread)
*
* When the need for one of the above is detected, the eval loop
* pauses long enough to handle the detected case.  Then, if doing
* so didn't trigger an exception, the eval loop resumes executing
* the sequential instructions.
*
* To make this work, the eval loop periodically checks if any
* of the above needs to happen.  The individual checks can be
* expensive if computed each time, so a while back we switched
* to using pre-computed, per-interpreter variables for the checks,
* and later consolidated that to a single "eval breaker" variable
* (now a PyInterpreterState field).
*
* For the longest time, the eval breaker check would happen
* frequently, every 5 or so times through the loop, regardless
* of what instruction ran last or what would run next.  Then, in
* early 2021 (gh-18334, commit 4958f5d), we switched to checking
* the eval breaker less frequently, by hard-coding the check to
* specific places in the eval loop (e.g. certain instructions).
* The intent then was to check after returning from calls
* and on the back edges of loops.
*
* In addition to being more efficient, that approach keeps
* the eval loop from running arbitrary code between instructions
* that don't handle that well.  (See gh-74174.)
*
* Currently, the eval breaker check happens on back edges in
* the control flow graph, which pretty much applies to all loops,
* and most calls.
* (See bytecodes.c for exact information.)
*
* One consequence of this approach is that it might not be obvious
* how to force any specific thread to pick up the eval breaker,
* or for any specific thread to not pick it up.  Mostly this
* involves judicious uses of locks and careful ordering of code,
* while avoiding code that might trigger the eval breaker
* until so desired.
*/
int
_Py_HandlePending(PyThreadState *tstate)
{
    uintptr_t breaker = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker);

    /* Stop-the-world */
    if ((breaker & _PY_EVAL_PLEASE_STOP_BIT) != 0) {
        _Py_unset_eval_breaker_bit(tstate, _PY_EVAL_PLEASE_STOP_BIT);
        _PyThreadState_Suspend(tstate);

        /* The attach blocks until the stop-the-world event is complete. */
        _PyThreadState_Attach(tstate);
    }

    /* Pending signals */
    if ((breaker & _PY_SIGNALS_PENDING_BIT) != 0) {
        if (handle_signals(tstate) != 0) {
            return -1;
        }
    }

    /* Pending calls */
    if ((breaker & _PY_CALLS_TO_DO_BIT) != 0) {
        if (make_pending_calls(tstate) != 0) {
            return -1;
        }
    }

#ifdef Py_GIL_DISABLED
    /* Objects with refcounts to merge */
    if ((breaker & _PY_EVAL_EXPLICIT_MERGE_BIT) != 0) {
        _Py_unset_eval_breaker_bit(tstate, _PY_EVAL_EXPLICIT_MERGE_BIT);
        _Py_brc_merge_refcounts(tstate);
    }
#endif

    /* GC scheduled to run */
    if ((breaker & _PY_GC_SCHEDULED_BIT) != 0) {
        _Py_unset_eval_breaker_bit(tstate, _PY_GC_SCHEDULED_BIT);
        _Py_RunGC(tstate);
    }

    if ((breaker & _PY_EVAL_JIT_INVALIDATE_COLD_BIT) != 0) {
        _Py_unset_eval_breaker_bit(tstate, _PY_EVAL_JIT_INVALIDATE_COLD_BIT);
        _Py_Executors_InvalidateCold(tstate->interp);
        tstate->interp->trace_run_counter = JIT_CLEANUP_THRESHOLD;
    }

    /* GIL drop request */
    if ((breaker & _PY_GIL_DROP_REQUEST_BIT) != 0) {
        /* Give another thread a chance */
        _PyThreadState_Detach(tstate);

        /* Other threads may run now */

        _PyThreadState_Attach(tstate);
    }

    /* Check for asynchronous exception. */
    if ((breaker & _PY_ASYNC_EXCEPTION_BIT) != 0) {
        _Py_unset_eval_breaker_bit(tstate, _PY_ASYNC_EXCEPTION_BIT);
        PyObject *exc = _Py_atomic_exchange_ptr(&tstate->async_exc, NULL);
        if (exc != NULL) {
            _PyErr_SetNone(tstate, exc);
            Py_DECREF(exc);
            return -1;
        }
    }

#if defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)
    _PyRunRemoteDebugger(tstate);
#endif

    return 0;
}
