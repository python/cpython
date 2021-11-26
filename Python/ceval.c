/* Execute compiled code */

/* XXX TO DO:
   XXX speed up searching for keywords by using a dictionary
   XXX document it!
   */

/* enable more aggressive intra-module optimizations, where available */
/* affects both release and debug builds - see bpo-43271 */
#define PY_LOCAL_AGGRESSIVE

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_call.h"          // _PyObject_FastCallDictTstate()
#include "pycore_ceval.h"         // _PyEval_SignalAsyncExc()
#include "pycore_code.h"
#include "pycore_function.h"
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_pyerrors.h"      // _PyErr_Fetch()
#include "pycore_pylifecycle.h"   // _PyErr_Print()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_sysmodule.h"     // _PySys_Audit()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

#include "code.h"
#include "pycore_dict.h"
#include "dictobject.h"
#include "frameobject.h"
#include "pycore_frame.h"
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

#include <ctype.h>

#ifdef Py_DEBUG
/* For debugging the interpreter: */
#define LLTRACE  1      /* Low-level trace feature */
#define CHECKEXC 1      /* Double-check exception checking */
#endif

#if !defined(Py_BUILD_CORE)
#  error "ceval.c must be build with Py_BUILD_CORE define for best performance"
#endif

_Py_IDENTIFIER(__name__);

/* Forward declarations */
static PyObject *trace_call_function(
    PyThreadState *tstate, PyObject *callable, PyObject **stack,
    Py_ssize_t oparg, PyObject *kwnames);
static PyObject * do_call_core(
    PyThreadState *tstate, PyObject *func,
    PyObject *callargs, PyObject *kwdict, int use_tracing);

#ifdef LLTRACE
static int lltrace;
static int prtrace(PyThreadState *, PyObject *, const char *);
static void lltrace_instruction(InterpreterFrame *frame, int opcode, int oparg)
{
    if (HAS_ARG(opcode)) {
        printf("%d: %d, %d\n",
                frame->f_lasti, opcode, oparg);
    }
    else {
        printf("%d: %d\n",
                frame->f_lasti, opcode);
    }
}
#endif
static int call_trace(Py_tracefunc, PyObject *,
                      PyThreadState *, InterpreterFrame *,
                      int, PyObject *);
static int call_trace_protected(Py_tracefunc, PyObject *,
                                PyThreadState *, InterpreterFrame *,
                                int, PyObject *);
static void call_exc_trace(Py_tracefunc, PyObject *,
                           PyThreadState *, InterpreterFrame *);
static int maybe_call_line_trace(Py_tracefunc, PyObject *,
                                 PyThreadState *, InterpreterFrame *, int);
static void maybe_dtrace_line(InterpreterFrame *, PyTraceInfo *, int);
static void dtrace_function_entry(InterpreterFrame *);
static void dtrace_function_return(InterpreterFrame *);

static PyObject * import_name(PyThreadState *, InterpreterFrame *,
                              PyObject *, PyObject *, PyObject *);
static PyObject * import_from(PyThreadState *, PyObject *, PyObject *);
static int import_all_from(PyThreadState *, PyObject *, PyObject *);
static void format_exc_check_arg(PyThreadState *, PyObject *, const char *, PyObject *);
static void format_exc_unbound(PyThreadState *tstate, PyCodeObject *co, int oparg);
static int check_args_iterable(PyThreadState *, PyObject *func, PyObject *vararg);
static int check_except_type_valid(PyThreadState *tstate, PyObject* right);
static void format_kwargs_error(PyThreadState *, PyObject *func, PyObject *kwargs);
static void format_awaitable_error(PyThreadState *, PyTypeObject *, int, int);
static int get_exception_handler(PyCodeObject *, int, int*, int*, int*);
static InterpreterFrame *
_PyEvalFramePushAndInit(PyThreadState *tstate, PyFunctionObject *func,
                        PyObject *locals, PyObject* const* args,
                        size_t argcount, PyObject *kwnames);
static int
_PyEvalFrameClearAndPop(PyThreadState *tstate, InterpreterFrame * frame);

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"
#define UNBOUNDLOCAL_ERROR_MSG \
    "cannot access local variable '%s' where it is not associated with a value"
#define UNBOUNDFREE_ERROR_MSG \
    "cannot access free variable '%s' where it is not associated with a" \
    " value in enclosing scope"

/* Dynamic execution profile */
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
static long dxpairs[257][256];
#define dxp dxpairs[256]
#else
static long dxp[256];
#endif
#endif

#ifndef NDEBUG
/* Ensure that tstate is valid: sanity check for PyEval_AcquireThread() and
   PyEval_RestoreThread(). Detect if tstate memory was freed. It can happen
   when a thread continues to run after Python finalization, especially
   daemon threads. */
static int
is_tstate_valid(PyThreadState *tstate)
{
    assert(!_PyMem_IsPtrFreed(tstate));
    assert(!_PyMem_IsPtrFreed(tstate->interp));
    return 1;
}
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
        _Py_atomic_load_relaxed(&ceval2->gil_drop_request)
        | (_Py_atomic_load_relaxed(&ceval->signals_pending)
           && _Py_ThreadCanHandleSignals(interp))
        | (_Py_atomic_load_relaxed(&ceval2->pending.calls_to_do)
           && _Py_ThreadCanHandlePendingCalls())
        | ceval2->pending.async_exc);
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
SIGNAL_PENDING_CALLS(PyInterpreterState *interp)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
    _Py_atomic_store_relaxed(&ceval2->pending.calls_to_do, 1);
    COMPUTE_EVAL_BREAKER(interp, ceval, ceval2);
}


static inline void
UNSIGNAL_PENDING_CALLS(PyInterpreterState *interp)
{
    struct _ceval_runtime_state *ceval = &interp->runtime->ceval;
    struct _ceval_state *ceval2 = &interp->ceval;
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


#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "ceval_gil.h"

void _Py_NO_RETURN
_Py_FatalError_TstateNULL(const char *func)
{
    _Py_FatalErrorFunc(func,
                       "the function must be called with the GIL held, "
                       "but the GIL is released "
                       "(the current Python thread state is NULL)");
}

#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
int
_PyEval_ThreadsInitialized(PyInterpreterState *interp)
{
    return gil_created(&interp->ceval.gil);
}

int
PyEval_ThreadsInitialized(void)
{
    // Fatal error if there is no current interpreter
    PyInterpreterState *interp = PyInterpreterState_Get();
    return _PyEval_ThreadsInitialized(interp);
}
#else
int
_PyEval_ThreadsInitialized(_PyRuntimeState *runtime)
{
    return gil_created(&runtime->ceval.gil);
}

int
PyEval_ThreadsInitialized(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    return _PyEval_ThreadsInitialized(runtime);
}
#endif

PyStatus
_PyEval_InitGIL(PyThreadState *tstate)
{
#ifndef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    if (!_Py_IsMainInterpreter(tstate->interp)) {
        /* Currently, the GIL is shared by all interpreters,
           and only the main interpreter is responsible to create
           and destroy it. */
        return _PyStatus_OK();
    }
#endif

#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    struct _gil_runtime_state *gil = &tstate->interp->ceval.gil;
#else
    struct _gil_runtime_state *gil = &tstate->interp->runtime->ceval.gil;
#endif
    assert(!gil_created(gil));

    PyThread_init_thread();
    create_gil(gil);

    take_gil(tstate);

    assert(gil_created(gil));
    return _PyStatus_OK();
}

void
_PyEval_FiniGIL(PyInterpreterState *interp)
{
#ifndef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    if (!_Py_IsMainInterpreter(interp)) {
        /* Currently, the GIL is shared by all interpreters,
           and only the main interpreter is responsible to create
           and destroy it. */
        return;
    }
#endif

#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    struct _gil_runtime_state *gil = &interp->ceval.gil;
#else
    struct _gil_runtime_state *gil = &interp->runtime->ceval.gil;
#endif
    if (!gil_created(gil)) {
        /* First Py_InitializeFromConfig() call: the GIL doesn't exist
           yet: do nothing. */
        return;
    }

    destroy_gil(gil);
    assert(!gil_created(gil));
}

void
PyEval_InitThreads(void)
{
    /* Do nothing: kept for backward compatibility */
}

void
_PyEval_Fini(void)
{
#if PRINT_SPECIALIZATION_STATS
    _Py_PrintSpecializationStats();
#endif
}

void
PyEval_AcquireLock(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    PyThreadState *tstate = _PyRuntimeState_GetThreadState(runtime);
    _Py_EnsureTstateNotNULL(tstate);

    take_gil(tstate);
}

void
PyEval_ReleaseLock(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    PyThreadState *tstate = _PyRuntimeState_GetThreadState(runtime);
    /* This function must succeed when the current thread state is NULL.
       We therefore avoid PyThreadState_Get() which dumps a fatal error
       in debug mode. */
    struct _ceval_runtime_state *ceval = &runtime->ceval;
    struct _ceval_state *ceval2 = &tstate->interp->ceval;
    drop_gil(ceval, ceval2, tstate);
}

void
_PyEval_ReleaseLock(PyThreadState *tstate)
{
    struct _ceval_runtime_state *ceval = &tstate->interp->runtime->ceval;
    struct _ceval_state *ceval2 = &tstate->interp->ceval;
    drop_gil(ceval, ceval2, tstate);
}

void
PyEval_AcquireThread(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);

    take_gil(tstate);

    struct _gilstate_runtime_state *gilstate = &tstate->interp->runtime->gilstate;
#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    (void)_PyThreadState_Swap(gilstate, tstate);
#else
    if (_PyThreadState_Swap(gilstate, tstate) != NULL) {
        Py_FatalError("non-NULL old thread state");
    }
#endif
}

void
PyEval_ReleaseThread(PyThreadState *tstate)
{
    assert(is_tstate_valid(tstate));

    _PyRuntimeState *runtime = tstate->interp->runtime;
    PyThreadState *new_tstate = _PyThreadState_Swap(&runtime->gilstate, NULL);
    if (new_tstate != tstate) {
        Py_FatalError("wrong thread state");
    }
    struct _ceval_runtime_state *ceval = &runtime->ceval;
    struct _ceval_state *ceval2 = &tstate->interp->ceval;
    drop_gil(ceval, ceval2, tstate);
}

#ifdef HAVE_FORK
/* This function is called from PyOS_AfterFork_Child to destroy all threads
   which are not running in the child process, and clear internal locks
   which might be held by those threads. */
PyStatus
_PyEval_ReInitThreads(PyThreadState *tstate)
{
    _PyRuntimeState *runtime = tstate->interp->runtime;

#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    struct _gil_runtime_state *gil = &tstate->interp->ceval.gil;
#else
    struct _gil_runtime_state *gil = &runtime->ceval.gil;
#endif
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
    _PyThreadState_DeleteExcept(runtime, tstate);
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
    _PyRuntimeState *runtime = &_PyRuntime;
#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    PyThreadState *old_tstate = _PyThreadState_GET();
    PyThreadState *tstate = _PyThreadState_Swap(&runtime->gilstate, old_tstate);
#else
    PyThreadState *tstate = _PyThreadState_Swap(&runtime->gilstate, NULL);
#endif
    _Py_EnsureTstateNotNULL(tstate);

    struct _ceval_runtime_state *ceval = &runtime->ceval;
    struct _ceval_state *ceval2 = &tstate->interp->ceval;
#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    assert(gil_created(&ceval2->gil));
#else
    assert(gil_created(&ceval->gil));
#endif
    drop_gil(ceval, ceval2, tstate);
    return tstate;
}

void
PyEval_RestoreThread(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);

    take_gil(tstate);

    struct _gilstate_runtime_state *gilstate = &tstate->interp->runtime->gilstate;
    _PyThreadState_Swap(gilstate, tstate);
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

/* Pop one item off the queue while holding the lock. */
static void
_pop_pending_call(struct _pending_calls *pending,
                  int (**func)(void *), void **arg)
{
    int i = pending->first;
    if (i == pending->last) {
        return; /* Queue empty */
    }

    *func = pending->calls[i].func;
    *arg = pending->calls[i].arg;
    pending->first = (i + 1) % NPENDINGCALLS;
}

/* This implementation is thread-safe.  It allows
   scheduling to be made from any thread, and even from an executing
   callback.
 */

int
_PyEval_AddPendingCall(PyInterpreterState *interp,
                       int (*func)(void *), void *arg)
{
    struct _pending_calls *pending = &interp->ceval.pending;

    /* Ensure that _PyEval_InitPendingCalls() was called
       and that _PyEval_FiniPendingCalls() is not called yet. */
    assert(pending->lock != NULL);

    PyThread_acquire_lock(pending->lock, WAIT_LOCK);
    int result = _push_pending_call(pending, func, arg);
    PyThread_release_lock(pending->lock);

    /* signal main loop */
    SIGNAL_PENDING_CALLS(interp);
    return result;
}

int
Py_AddPendingCall(int (*func)(void *), void *arg)
{
    /* Best-effort to support subinterpreters and calls with the GIL released.

       First attempt _PyThreadState_GET() since it supports subinterpreters.

       If the GIL is released, _PyThreadState_GET() returns NULL . In this
       case, use PyGILState_GetThisThreadState() which works even if the GIL
       is released.

       Sadly, PyGILState_GetThisThreadState() doesn't support subinterpreters:
       see bpo-10915 and bpo-15751.

       Py_AddPendingCall() doesn't require the caller to hold the GIL. */
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        tstate = PyGILState_GetThisThreadState();
    }

    PyInterpreterState *interp;
    if (tstate != NULL) {
        interp = tstate->interp;
    }
    else {
        /* Last resort: use the main interpreter */
        interp = _PyRuntime.interpreters.main;
    }
    return _PyEval_AddPendingCall(interp, func, arg);
}

static int
handle_signals(PyThreadState *tstate)
{
    assert(is_tstate_valid(tstate));
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

static int
make_pending_calls(PyInterpreterState *interp)
{
    /* only execute pending calls on main thread */
    if (!_Py_ThreadCanHandlePendingCalls()) {
        return 0;
    }

    /* don't perform recursive pending calls */
    static int busy = 0;
    if (busy) {
        return 0;
    }
    busy = 1;

    /* unsignal before starting to call callbacks, so that any callback
       added in-between re-signals */
    UNSIGNAL_PENDING_CALLS(interp);
    int res = 0;

    /* perform a bounded number of calls, in case of recursion */
    struct _pending_calls *pending = &interp->ceval.pending;
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
        res = func(arg);
        if (res) {
            goto error;
        }
    }

    busy = 0;
    return res;

error:
    busy = 0;
    SIGNAL_PENDING_CALLS(interp);
    return res;
}

void
_Py_FinishPendingCalls(PyThreadState *tstate)
{
    assert(PyGILState_Check());
    assert(is_tstate_valid(tstate));

    struct _pending_calls *pending = &tstate->interp->ceval.pending;

    if (!_Py_atomic_load_relaxed(&(pending->calls_to_do))) {
        return;
    }

    if (make_pending_calls(tstate->interp) < 0) {
        PyObject *exc, *val, *tb;
        _PyErr_Fetch(tstate, &exc, &val, &tb);
        PyErr_BadInternalCall();
        _PyErr_ChainExceptions(exc, val, tb);
        _PyErr_Print(tstate);
    }
}

/* Py_MakePendingCalls() is a simple wrapper for the sake
   of backward-compatibility. */
int
Py_MakePendingCalls(void)
{
    assert(PyGILState_Check());

    PyThreadState *tstate = _PyThreadState_GET();
    assert(is_tstate_valid(tstate));

    /* Python signal handler doesn't really queue a callback: it only signals
       that a signal was received, see _PyEval_SignalReceived(). */
    int res = handle_signals(tstate);
    if (res != 0) {
        return res;
    }

    res = make_pending_calls(tstate->interp);
    if (res != 0) {
        return res;
    }

    return 0;
}

/* The interpreter's recursion limit */

#ifndef Py_DEFAULT_RECURSION_LIMIT
#  define Py_DEFAULT_RECURSION_LIMIT 1000
#endif

void
_PyEval_InitRuntimeState(struct _ceval_runtime_state *ceval)
{
#ifndef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    _gil_initialize(&ceval->gil);
#endif
}

int
_PyEval_InitState(struct _ceval_state *ceval)
{
    ceval->recursion_limit = Py_DEFAULT_RECURSION_LIMIT;

    struct _pending_calls *pending = &ceval->pending;
    assert(pending->lock == NULL);

    pending->lock = PyThread_allocate_lock();
    if (pending->lock == NULL) {
        return -1;
    }

#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    _gil_initialize(&ceval->gil);
#endif

    return 0;
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

int
Py_GetRecursionLimit(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return interp->ceval.recursion_limit;
}

void
Py_SetRecursionLimit(int new_limit)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    interp->ceval.recursion_limit = new_limit;
    for (PyThreadState *p = interp->tstate_head; p != NULL; p = p->next) {
        int depth = p->recursion_limit - p->recursion_remaining;
        p->recursion_limit = new_limit;
        p->recursion_remaining = new_limit - depth;
    }
}

/* The function _Py_EnterRecursiveCall() only calls _Py_CheckRecursiveCall()
   if the recursion_depth reaches recursion_limit. */
int
_Py_CheckRecursiveCall(PyThreadState *tstate, const char *where)
{
    /* Check against global limit first. */
    int depth = tstate->recursion_limit - tstate->recursion_remaining;
    if (depth < tstate->interp->ceval.recursion_limit) {
        tstate->recursion_limit = tstate->interp->ceval.recursion_limit;
        tstate->recursion_remaining = tstate->recursion_limit - depth;
        assert(tstate->recursion_remaining > 0);
        return 0;
    }
#ifdef USE_STACKCHECK
    if (PyOS_CheckStack()) {
        ++tstate->recursion_remaining;
        _PyErr_SetString(tstate, PyExc_MemoryError, "Stack overflow");
        return -1;
    }
#endif
    if (tstate->recursion_headroom) {
        if (tstate->recursion_remaining < -50) {
            /* Overflowing while handling an overflow. Give up. */
            Py_FatalError("Cannot recover from stack overflow.");
        }
    }
    else {
        if (tstate->recursion_remaining <= 0) {
            tstate->recursion_headroom++;
            _PyErr_Format(tstate, PyExc_RecursionError,
                        "maximum recursion depth exceeded%s",
                        where);
            tstate->recursion_headroom--;
            ++tstate->recursion_remaining;
            return -1;
        }
    }
    return 0;
}


static const binaryfunc binary_ops[] = {
    [NB_ADD] = PyNumber_Add,
    [NB_AND] = PyNumber_And,
    [NB_FLOOR_DIVIDE] = PyNumber_FloorDivide,
    [NB_LSHIFT] = PyNumber_Lshift,
    [NB_MATRIX_MULTIPLY] = PyNumber_MatrixMultiply,
    [NB_MULTIPLY] = PyNumber_Multiply,
    [NB_REMAINDER] = PyNumber_Remainder,
    [NB_OR] = PyNumber_Or,
    [NB_POWER] = _PyNumber_PowerNoMod,
    [NB_RSHIFT] = PyNumber_Rshift,
    [NB_SUBTRACT] = PyNumber_Subtract,
    [NB_TRUE_DIVIDE] = PyNumber_TrueDivide,
    [NB_XOR] = PyNumber_Xor,
    [NB_INPLACE_ADD] = PyNumber_InPlaceAdd,
    [NB_INPLACE_AND] = PyNumber_InPlaceAnd,
    [NB_INPLACE_FLOOR_DIVIDE] = PyNumber_InPlaceFloorDivide,
    [NB_INPLACE_LSHIFT] = PyNumber_InPlaceLshift,
    [NB_INPLACE_MATRIX_MULTIPLY] = PyNumber_InPlaceMatrixMultiply,
    [NB_INPLACE_MULTIPLY] = PyNumber_InPlaceMultiply,
    [NB_INPLACE_REMAINDER] = PyNumber_InPlaceRemainder,
    [NB_INPLACE_OR] = PyNumber_InPlaceOr,
    [NB_INPLACE_POWER] = _PyNumber_InPlacePowerNoMod,
    [NB_INPLACE_RSHIFT] = PyNumber_InPlaceRshift,
    [NB_INPLACE_SUBTRACT] = PyNumber_InPlaceSubtract,
    [NB_INPLACE_TRUE_DIVIDE] = PyNumber_InPlaceTrueDivide,
    [NB_INPLACE_XOR] = PyNumber_InPlaceXor,
};


// PEP 634: Structural Pattern Matching


// Return a tuple of values corresponding to keys, with error checks for
// duplicate/missing keys.
static PyObject*
match_keys(PyThreadState *tstate, PyObject *map, PyObject *keys)
{
    assert(PyTuple_CheckExact(keys));
    Py_ssize_t nkeys = PyTuple_GET_SIZE(keys);
    if (!nkeys) {
        // No keys means no items.
        return PyTuple_New(0);
    }
    PyObject *seen = NULL;
    PyObject *dummy = NULL;
    PyObject *values = NULL;
    PyObject *get_name = NULL;
    PyObject *get = NULL;
    // We use the two argument form of map.get(key, default) for two reasons:
    // - Atomically check for a key and get its value without error handling.
    // - Don't cause key creation or resizing in dict subclasses like
    //   collections.defaultdict that define __missing__ (or similar).
    _Py_IDENTIFIER(get);
    get_name = _PyUnicode_FromId(&PyId_get); // borrowed
    if (get_name == NULL) {
        return NULL;
    }
    int meth_found = _PyObject_GetMethod(map, get_name, &get);
    if (get == NULL) {
        goto fail;
    }
    seen = PySet_New(NULL);
    if (seen == NULL) {
        goto fail;
    }
    // dummy = object()
    dummy = _PyObject_CallNoArgs((PyObject *)&PyBaseObject_Type);
    if (dummy == NULL) {
        goto fail;
    }
    values = PyTuple_New(nkeys);
    if (values == NULL) {
        goto fail;
    }
    for (Py_ssize_t i = 0; i < nkeys; i++) {
        PyObject *key = PyTuple_GET_ITEM(keys, i);
        if (PySet_Contains(seen, key) || PySet_Add(seen, key)) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                _PyErr_Format(tstate, PyExc_ValueError,
                              "mapping pattern checks duplicate key (%R)", key);
            }
            goto fail;
        }
        PyObject *args[] = { map, key, dummy };
        PyObject *value = NULL;
        if (meth_found) {
            value = PyObject_Vectorcall(get, args, 3, NULL);
        }
        else {
            value = PyObject_Vectorcall(get, &args[1], 2, NULL);
        }
        if (value == NULL) {
            goto fail;
        }
        if (value == dummy) {
            // key not in map!
            Py_DECREF(value);
            Py_DECREF(values);
            // Return None:
            Py_INCREF(Py_None);
            values = Py_None;
            goto done;
        }
        PyTuple_SET_ITEM(values, i, value);
    }
    // Success:
done:
    Py_DECREF(get);
    Py_DECREF(seen);
    Py_DECREF(dummy);
    return values;
fail:
    Py_XDECREF(get);
    Py_XDECREF(seen);
    Py_XDECREF(dummy);
    Py_XDECREF(values);
    return NULL;
}

// Extract a named attribute from the subject, with additional bookkeeping to
// raise TypeErrors for repeated lookups. On failure, return NULL (with no
// error set). Use _PyErr_Occurred(tstate) to disambiguate.
static PyObject*
match_class_attr(PyThreadState *tstate, PyObject *subject, PyObject *type,
                 PyObject *name, PyObject *seen)
{
    assert(PyUnicode_CheckExact(name));
    assert(PySet_CheckExact(seen));
    if (PySet_Contains(seen, name) || PySet_Add(seen, name)) {
        if (!_PyErr_Occurred(tstate)) {
            // Seen it before!
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%s() got multiple sub-patterns for attribute %R",
                          ((PyTypeObject*)type)->tp_name, name);
        }
        return NULL;
    }
    PyObject *attr = PyObject_GetAttr(subject, name);
    if (attr == NULL && _PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
        _PyErr_Clear(tstate);
    }
    return attr;
}

// On success (match), return a tuple of extracted attributes. On failure (no
// match), return NULL. Use _PyErr_Occurred(tstate) to disambiguate.
static PyObject*
match_class(PyThreadState *tstate, PyObject *subject, PyObject *type,
            Py_ssize_t nargs, PyObject *kwargs)
{
    if (!PyType_Check(type)) {
        const char *e = "called match pattern must be a type";
        _PyErr_Format(tstate, PyExc_TypeError, e);
        return NULL;
    }
    assert(PyTuple_CheckExact(kwargs));
    // First, an isinstance check:
    if (PyObject_IsInstance(subject, type) <= 0) {
        return NULL;
    }
    // So far so good:
    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        return NULL;
    }
    PyObject *attrs = PyList_New(0);
    if (attrs == NULL) {
        Py_DECREF(seen);
        return NULL;
    }
    // NOTE: From this point on, goto fail on failure:
    PyObject *match_args = NULL;
    // First, the positional subpatterns:
    if (nargs) {
        int match_self = 0;
        match_args = PyObject_GetAttrString(type, "__match_args__");
        if (match_args) {
            if (!PyTuple_CheckExact(match_args)) {
                const char *e = "%s.__match_args__ must be a tuple (got %s)";
                _PyErr_Format(tstate, PyExc_TypeError, e,
                              ((PyTypeObject *)type)->tp_name,
                              Py_TYPE(match_args)->tp_name);
                goto fail;
            }
        }
        else if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
            _PyErr_Clear(tstate);
            // _Py_TPFLAGS_MATCH_SELF is only acknowledged if the type does not
            // define __match_args__. This is natural behavior for subclasses:
            // it's as if __match_args__ is some "magic" value that is lost as
            // soon as they redefine it.
            match_args = PyTuple_New(0);
            match_self = PyType_HasFeature((PyTypeObject*)type,
                                            _Py_TPFLAGS_MATCH_SELF);
        }
        else {
            goto fail;
        }
        assert(PyTuple_CheckExact(match_args));
        Py_ssize_t allowed = match_self ? 1 : PyTuple_GET_SIZE(match_args);
        if (allowed < nargs) {
            const char *plural = (allowed == 1) ? "" : "s";
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%s() accepts %d positional sub-pattern%s (%d given)",
                          ((PyTypeObject*)type)->tp_name,
                          allowed, plural, nargs);
            goto fail;
        }
        if (match_self) {
            // Easy. Copy the subject itself, and move on to kwargs.
            PyList_Append(attrs, subject);
        }
        else {
            for (Py_ssize_t i = 0; i < nargs; i++) {
                PyObject *name = PyTuple_GET_ITEM(match_args, i);
                if (!PyUnicode_CheckExact(name)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "__match_args__ elements must be strings "
                                  "(got %s)", Py_TYPE(name)->tp_name);
                    goto fail;
                }
                PyObject *attr = match_class_attr(tstate, subject, type, name,
                                                  seen);
                if (attr == NULL) {
                    goto fail;
                }
                PyList_Append(attrs, attr);
                Py_DECREF(attr);
            }
        }
        Py_CLEAR(match_args);
    }
    // Finally, the keyword subpatterns:
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(kwargs); i++) {
        PyObject *name = PyTuple_GET_ITEM(kwargs, i);
        PyObject *attr = match_class_attr(tstate, subject, type, name, seen);
        if (attr == NULL) {
            goto fail;
        }
        PyList_Append(attrs, attr);
        Py_DECREF(attr);
    }
    Py_SETREF(attrs, PyList_AsTuple(attrs));
    Py_DECREF(seen);
    return attrs;
fail:
    // We really don't care whether an error was raised or not... that's our
    // caller's problem. All we know is that the match failed.
    Py_XDECREF(match_args);
    Py_DECREF(seen);
    Py_DECREF(attrs);
    return NULL;
}


static int do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause);
static int unpack_iterable(PyThreadState *, PyObject *, int, int, PyObject **);

#ifdef Py_DEBUG
static void
_assert_exception_type_is_redundant(PyObject* type, PyObject* val)
{
    if (type == NULL || type == Py_None) {
        assert(val == NULL || val == Py_None);
    }
    else {
        assert(PyExceptionInstance_Check(val));
        assert(PyExceptionInstance_Class(val) == type);
    }
}

#define ASSERT_EXC_TYPE_IS_REDUNDANT(t, v) _assert_exception_type_is_redundant(t, v)
#else
#define ASSERT_EXC_TYPE_IS_REDUNDANT(t, v)
#endif

PyObject *
PyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (locals == NULL) {
        locals = globals;
    }
    PyObject *builtins = _PyEval_BuiltinsFromGlobals(tstate, globals); // borrowed ref
    if (builtins == NULL) {
        return NULL;
    }
    PyFrameConstructor desc = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = ((PyCodeObject *)co)->co_name,
        .fc_qualname = ((PyCodeObject *)co)->co_name,
        .fc_code = co,
        .fc_defaults = NULL,
        .fc_kwdefaults = NULL,
        .fc_closure = NULL
    };
    PyFunctionObject *func = _PyFunction_FromConstructor(&desc);
    if (func == NULL) {
        return NULL;
    }
    PyObject *res = _PyEval_Vector(tstate, func, locals, NULL, 0, NULL);
    Py_DECREF(func);
    return res;
}


/* Interpreter main loop */

PyObject *
PyEval_EvalFrame(PyFrameObject *f)
{
    /* Function kept for backward compatibility */
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f->f_frame, 0);
}

PyObject *
PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f->f_frame, throwflag);
}


/* Handle signals, pending calls, GIL drop request
   and asynchronous exception */
static int
eval_frame_handle_pending(PyThreadState *tstate)
{
    _PyRuntimeState * const runtime = &_PyRuntime;
    struct _ceval_runtime_state *ceval = &runtime->ceval;

    /* Pending signals */
    if (_Py_atomic_load_relaxed(&ceval->signals_pending)) {
        if (handle_signals(tstate) != 0) {
            return -1;
        }
    }

    /* Pending calls */
    struct _ceval_state *ceval2 = &tstate->interp->ceval;
    if (_Py_atomic_load_relaxed(&ceval2->pending.calls_to_do)) {
        if (make_pending_calls(tstate->interp) != 0) {
            return -1;
        }
    }

    /* GIL drop request */
    if (_Py_atomic_load_relaxed(&ceval2->gil_drop_request)) {
        /* Give another thread a chance */
        if (_PyThreadState_Swap(&runtime->gilstate, NULL) != tstate) {
            Py_FatalError("tstate mix-up");
        }
        drop_gil(ceval, ceval2, tstate);

        /* Other threads may run now */

        take_gil(tstate);

#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
        (void)_PyThreadState_Swap(&runtime->gilstate, tstate);
#else
        if (_PyThreadState_Swap(&runtime->gilstate, tstate) != NULL) {
            Py_FatalError("orphan tstate");
        }
#endif
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

#ifdef MS_WINDOWS
    // bpo-42296: On Windows, _PyEval_SignalReceived() can be called in a
    // different thread than the Python thread, in which case
    // _Py_ThreadCanHandleSignals() is wrong. Recompute eval_breaker in the
    // current Python thread with the correct _Py_ThreadCanHandleSignals()
    // value. It prevents to interrupt the eval loop at every instruction if
    // the current Python thread cannot handle signals (if
    // _Py_ThreadCanHandleSignals() is false).
    COMPUTE_EVAL_BREAKER(tstate->interp, ceval, ceval2);
#endif

    return 0;
}


/* Computed GOTOs, or
       the-optimization-commonly-but-improperly-known-as-"threaded code"
   using gcc's labels-as-values extension
   (http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html).

   The traditional bytecode evaluation loop uses a "switch" statement, which
   decent compilers will optimize as a single indirect branch instruction
   combined with a lookup table of jump addresses. However, since the
   indirect jump instruction is shared by all opcodes, the CPU will have a
   hard time making the right prediction for where to jump next (actually,
   it will be always wrong except in the uncommon case of a sequence of
   several identical opcodes).

   "Threaded code" in contrast, uses an explicit jump table and an explicit
   indirect jump instruction at the end of each opcode. Since the jump
   instruction is at a different address for each opcode, the CPU will make a
   separate prediction for each of these instructions, which is equivalent to
   predicting the second opcode of each opcode pair. These predictions have
   a much better chance to turn out valid, especially in small bytecode loops.

   A mispredicted branch on a modern CPU flushes the whole pipeline and
   can cost several CPU cycles (depending on the pipeline depth),
   and potentially many more instructions (depending on the pipeline width).
   A correctly predicted branch, however, is nearly free.

   At the time of this writing, the "threaded code" version is up to 15-20%
   faster than the normal "switch" version, depending on the compiler and the
   CPU architecture.

   We disable the optimization if DYNAMIC_EXECUTION_PROFILE is defined,
   because it would render the measurements invalid.


   NOTE: care must be taken that the compiler doesn't try to "optimize" the
   indirect jumps by sharing them between all opcodes. Such optimizations
   can be disabled on gcc by using the -fno-gcse flag (or possibly
   -fno-crossjumping).
*/

/* Use macros rather than inline functions, to make it as clear as possible
 * to the C compiler that the tracing check is a simple test then branch.
 * We want to be sure that the compiler knows this before it generates
 * the CFG.
 */
#ifdef LLTRACE
#define LLTRACE_INSTR() if (lltrace) { lltrace_instruction(frame, opcode, oparg); }
#else
#define LLTRACE_INSTR() ((void)0)
#endif

#ifdef WITH_DTRACE
#define OR_DTRACE_LINE | (PyDTrace_LINE_ENABLED() ? 255 : 0)
#else
#define OR_DTRACE_LINE
#endif

#ifdef DYNAMIC_EXECUTION_PROFILE
#undef USE_COMPUTED_GOTOS
#define USE_COMPUTED_GOTOS 0
#endif

#ifdef HAVE_COMPUTED_GOTOS
    #ifndef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 1
    #endif
#else
    #if defined(USE_COMPUTED_GOTOS) && USE_COMPUTED_GOTOS
    #error "Computed gotos are not supported on this compiler."
    #endif
    #undef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 0
#endif

#define INSTRUCTION_START() frame->f_lasti = INSTR_OFFSET(); next_instr++

#if USE_COMPUTED_GOTOS
#define TARGET(op) TARGET_##op: INSTRUCTION_START();
#define DISPATCH_GOTO() goto *opcode_targets[opcode]
#else
#define TARGET(op) case op: INSTRUCTION_START();
#define DISPATCH_GOTO() goto dispatch_opcode
#endif

/* RECORD_DXPROFILE()  records the dxprofile information, if enabled. Normally a no-op */
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
#define RECORD_DXPROFILE() \
    do { \
        dxpairs[lastopcode][opcode]++; \
        lastopcode = opcode; \
        dxp[opcode]++; \
    } while (0)
#else
  #define RECORD_DXPROFILE() \
    do { \
        dxp[opcode]++; \
    } while (0)
#endif
#else
#define RECORD_DXPROFILE() ((void)0)
#endif

/* PRE_DISPATCH_GOTO() does lltrace and dxprofile if either is enabled. Normally a no-op */
#ifndef LLTRACE
#ifndef DYNAMIC_EXECUTION_PROFILE
#define PRE_DISPATCH_GOTO() ((void)0)
#endif
#endif
#ifndef PRE_DISPATCH_GOTO
#define PRE_DISPATCH_GOTO() do { LLTRACE_INSTR(); RECORD_DXPROFILE(); } while (0)
#endif

#define NOTRACE_DISPATCH() \
    { \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

/* Do interpreter dispatch accounting for tracing and instrumentation */
#define DISPATCH() \
    { \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        assert(cframe.use_tracing == 0 || cframe.use_tracing == 255); \
        opcode |= cframe.use_tracing OR_DTRACE_LINE; \
        DISPATCH_GOTO(); \
    }

#define CHECK_EVAL_BREAKER() \
    if (_Py_atomic_load_relaxed(eval_breaker)) { \
        goto check_eval_breaker; \
    }


/* Tuple access macros */

#ifndef Py_DEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((PyTupleObject *)(v), (i))
#else
#define GETITEM(v, i) PyTuple_GetItem((v), (i))
#endif

/* Code access macros */

/* The integer overflow is checked by an assertion below. */
#define INSTR_OFFSET() ((int)(next_instr - first_instr))
#define NEXTOPARG()  do { \
        _Py_CODEUNIT word = *next_instr; \
        opcode = _Py_OPCODE(word); \
        oparg = _Py_OPARG(word); \
    } while (0)
#define JUMPTO(x)       (next_instr = first_instr + (x))
#define JUMPBY(x)       (next_instr += (x))

/* Get opcode and oparg from original instructions, not quickened form. */
#define TRACING_NEXTOPARG() do { \
        _Py_CODEUNIT word = ((_Py_CODEUNIT *)PyBytes_AS_STRING(co->co_code))[INSTR_OFFSET()]; \
        opcode = _Py_OPCODE(word); \
        oparg = _Py_OPARG(word); \
    } while (0)

/* OpCode prediction macros
    Some opcodes tend to come in pairs thus making it possible to
    predict the second code when the first is run.  For example,
    COMPARE_OP is often followed by POP_JUMP_IF_FALSE or POP_JUMP_IF_TRUE.

    Verifying the prediction costs a single high-speed test of a register
    variable against a constant.  If the pairing was good, then the
    processor's own internal branch predication has a high likelihood of
    success, resulting in a nearly zero-overhead transition to the
    next opcode.  A successful prediction saves a trip through the eval-loop
    including its unpredictable switch-case branch.  Combined with the
    processor's internal branch prediction, a successful PREDICT has the
    effect of making the two opcodes run as if they were a single new opcode
    with the bodies combined.

    If collecting opcode statistics, your choices are to either keep the
    predictions turned-on and interpret the results as if some opcodes
    had been combined or turn-off predictions so that the opcode frequency
    counter updates for both opcodes.

    Opcode prediction is disabled with threaded code, since the latter allows
    the CPU to record separate branch prediction information for each
    opcode.

*/

#define PREDICT_ID(op)          PRED_##op

#if defined(DYNAMIC_EXECUTION_PROFILE) || USE_COMPUTED_GOTOS
#define PREDICT(op)             if (0) goto PREDICT_ID(op)
#else
#define PREDICT(op) \
    do { \
        _Py_CODEUNIT word = *next_instr; \
        opcode = _Py_OPCODE(word) | cframe.use_tracing OR_DTRACE_LINE; \
        if (opcode == op) { \
            oparg = _Py_OPARG(word); \
            INSTRUCTION_START(); \
            goto PREDICT_ID(op); \
        } \
    } while(0)
#endif
#define PREDICTED(op)           PREDICT_ID(op):


/* Stack manipulation macros */

/* The stack can grow at most MAXINT deep, as co_nlocals and
   co_stacksize are ints. */
#define STACK_LEVEL()     ((int)(stack_pointer - _PyFrame_Stackbase(frame)))
#define EMPTY()           (STACK_LEVEL() == 0)
#define TOP()             (stack_pointer[-1])
#define SECOND()          (stack_pointer[-2])
#define THIRD()           (stack_pointer[-3])
#define FOURTH()          (stack_pointer[-4])
#define PEEK(n)           (stack_pointer[-(n)])
#define SET_TOP(v)        (stack_pointer[-1] = (v))
#define SET_SECOND(v)     (stack_pointer[-2] = (v))
#define SET_THIRD(v)      (stack_pointer[-3] = (v))
#define SET_FOURTH(v)     (stack_pointer[-4] = (v))
#define BASIC_STACKADJ(n) (stack_pointer += n)
#define BASIC_PUSH(v)     (*stack_pointer++ = (v))
#define BASIC_POP()       (*--stack_pointer)

#ifdef LLTRACE
#define PUSH(v)         { (void)(BASIC_PUSH(v), \
                          lltrace && prtrace(tstate, TOP(), "push")); \
                          assert(STACK_LEVEL() <= co->co_stacksize); }
#define POP()           ((void)(lltrace && prtrace(tstate, TOP(), "pop")), \
                         BASIC_POP())
#define STACK_GROW(n)   do { \
                          assert(n >= 0); \
                          (void)(BASIC_STACKADJ(n), \
                          lltrace && prtrace(tstate, TOP(), "stackadj")); \
                          assert(STACK_LEVEL() <= co->co_stacksize); \
                        } while (0)
#define STACK_SHRINK(n) do { \
                            assert(n >= 0); \
                            (void)(lltrace && prtrace(tstate, TOP(), "stackadj")); \
                            (void)(BASIC_STACKADJ(-(n))); \
                            assert(STACK_LEVEL() <= co->co_stacksize); \
                        } while (0)
#define EXT_POP(STACK_POINTER) ((void)(lltrace && \
                                prtrace(tstate, (STACK_POINTER)[-1], "ext_pop")), \
                                *--(STACK_POINTER))
#else
#define PUSH(v)                BASIC_PUSH(v)
#define POP()                  BASIC_POP()
#define STACK_GROW(n)          BASIC_STACKADJ(n)
#define STACK_SHRINK(n)        BASIC_STACKADJ(-(n))
#define EXT_POP(STACK_POINTER) (*--(STACK_POINTER))
#endif

/* Local variable macros */

#define GETLOCAL(i)     (frame->localsplus[i])

/* The SETLOCAL() macro must not DECREF the local variable in-place and
   then store the new value; it must copy the old value to a temporary
   value, then store the new value, and then DECREF the temporary value.
   This is because it is possible that during the DECREF the frame is
   accessed by other code (e.g. a __del__ method or gc.collect()) and the
   variable would be pointing to already-freed memory. */
#define SETLOCAL(i, value)      do { PyObject *tmp = GETLOCAL(i); \
                                     GETLOCAL(i) = value; \
                                     Py_XDECREF(tmp); } while (0)

#define JUMP_TO_INSTRUCTION(op) goto PREDICT_ID(op)

#define GET_CACHE() \
    _GetSpecializedCacheEntryForInstruction(first_instr, INSTR_OFFSET(), oparg)


#define DEOPT_IF(cond, instname) if (cond) { goto instname ## _miss; }

#define UPDATE_PREV_INSTR_OPARG(instr, oparg) ((uint8_t*)(instr))[-1] = (oparg)


#define GLOBALS() frame->f_globals
#define BUILTINS() frame->f_builtins
#define LOCALS() frame->f_locals

/* Shared opcode macros */

// shared by LOAD_ATTR_MODULE and LOAD_METHOD_MODULE
#define LOAD_MODULE_ATTR_OR_METHOD(attr_or_method) \
    SpecializedCacheEntry *caches = GET_CACHE(); \
    _PyAdaptiveEntry *cache0 = &caches[0].adaptive; \
    _PyAttrCache *cache1 = &caches[-1].attr; \
    DEOPT_IF(!PyModule_CheckExact(owner), LOAD_##attr_or_method); \
    PyDictObject *dict = (PyDictObject *)((PyModuleObject *)owner)->md_dict; \
    assert(dict != NULL); \
    DEOPT_IF(dict->ma_keys->dk_version != cache1->dk_version_or_hint, \
        LOAD_##attr_or_method); \
    assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE); \
    assert(cache0->index < dict->ma_keys->dk_nentries); \
    PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + cache0->index; \
    res = ep->me_value; \
    DEOPT_IF(res == NULL, LOAD_##attr_or_method); \
    STAT_INC(LOAD_##attr_or_method, hit); \
    Py_INCREF(res);

static int
trace_function_entry(PyThreadState *tstate, InterpreterFrame *frame)
{
    if (tstate->c_tracefunc != NULL) {
        /* tstate->c_tracefunc, if defined, is a
            function that will be called on *every* entry
            to a code block.  Its return value, if not
            None, is a function that will be called at
            the start of each executed line of code.
            (Actually, the function must return itself
            in order to continue tracing.)  The trace
            functions are called with three arguments:
            a pointer to the current frame, a string
            indicating why the function is called, and
            an argument which depends on the situation.
            The global trace function is also called
            whenever an exception is detected. */
        if (call_trace_protected(tstate->c_tracefunc,
                                    tstate->c_traceobj,
                                    tstate, frame,
                                    PyTrace_CALL, Py_None)) {
            /* Trace function raised an error */
            return -1;
        }
    }
    if (tstate->c_profilefunc != NULL) {
        /* Similar for c_profilefunc, except it needn't
            return itself and isn't called for "line" events */
        if (call_trace_protected(tstate->c_profilefunc,
                                    tstate->c_profileobj,
                                    tstate, frame,
                                    PyTrace_CALL, Py_None)) {
            /* Profile function raised an error */
            return -1;
        }
    }
    return 0;
}

static PyObject *
make_coro(PyThreadState *tstate, PyFunctionObject *func,
          PyObject *locals,
          PyObject* const* args, size_t argcount,
          PyObject *kwnames);

static int
skip_backwards_over_extended_args(PyCodeObject *code, int offset) {
    _Py_CODEUNIT *instrs = (_Py_CODEUNIT *)PyBytes_AS_STRING(code->co_code);
    while (offset > 0 && _Py_OPCODE(instrs[offset-1]) == EXTENDED_ARG) {
        offset--;
    }
    return offset;
}

PyObject* _Py_HOT_FUNCTION
_PyEval_EvalFrameDefault(PyThreadState *tstate, InterpreterFrame *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);

#if USE_COMPUTED_GOTOS
/* Import the static jump table */
#include "opcode_targets.h"
#endif

#ifdef DXPAIRS
    int lastopcode = 0;
#endif
    int opcode;        /* Current opcode */
    int oparg;         /* Current opcode argument, if any */
    PyObject *retval = NULL;            /* Return value */
    _Py_atomic_int * const eval_breaker = &tstate->interp->ceval.eval_breaker;

    CFrame cframe;

    /* WARNING: Because the CFrame lives on the C stack,
     * but can be accessed from a heap allocated object (tstate)
     * strict stack discipline must be maintained.
     */
    CFrame *prev_cframe = tstate->cframe;
    cframe.use_tracing = prev_cframe->use_tracing;
    cframe.previous = prev_cframe;
    tstate->cframe = &cframe;

    assert(frame->depth == 0);
    /* Push frame */
    frame->previous = prev_cframe->current_frame;
    cframe.current_frame = frame;

start_frame:
    if (_Py_EnterRecursiveCall(tstate, "")) {
        tstate->recursion_remaining--;
        goto exit_eval_frame;
    }

    assert(tstate->cframe == &cframe);
    assert(frame == cframe.current_frame);

    if (cframe.use_tracing) {
        if (trace_function_entry(tstate, frame)) {
            goto exit_eval_frame;
        }
    }

    if (PyDTrace_FUNCTION_ENTRY_ENABLED())
        dtrace_function_entry(frame);

    PyCodeObject *co = frame->f_code;
    /* Increment the warmup counter and quicken if warm enough
     * _Py_Quicken is idempotent so we don't worry about overflow */
    if (!PyCodeObject_IsWarmedUp(co)) {
        PyCodeObject_IncrementWarmup(co);
        if (PyCodeObject_IsWarmedUp(co)) {
            if (_Py_Quicken(co)) {
                goto exit_eval_frame;
            }
        }
    }

resume_frame:
    co = frame->f_code;
    PyObject *names = co->co_names;
    PyObject *consts = co->co_consts;
    _Py_CODEUNIT *first_instr = co->co_firstinstr;
    /*
       frame->f_lasti refers to the index of the last instruction,
       unless it's -1 in which case next_instr should be first_instr.

       YIELD_FROM sets frame->f_lasti to itself, in order to repeatedly yield
       multiple values.

       When the PREDICT() macros are enabled, some opcode pairs follow in
       direct succession. A successful prediction effectively links the two
       codes together as if they were a single new opcode, but the value
       of frame->f_lasti is correctly updated so potential inlined calls
       or lookups of frame->f_lasti are aways correct when the macros are used.
    */
    assert(frame->f_lasti >= -1);
    _Py_CODEUNIT *next_instr = first_instr + frame->f_lasti + 1;
    PyObject **stack_pointer = _PyFrame_GetStackPointer(frame);
    /* Set stackdepth to -1.
     * Update when returning or calling trace function.
       Having stackdepth <= 0 ensures that invalid
       values are not visible to the cycle GC.
       We choose -1 rather than 0 to assist debugging.
     */
    frame->stacktop = -1;
    frame->f_state = FRAME_EXECUTING;

#ifdef LLTRACE
    _Py_IDENTIFIER(__ltrace__);
    {
        int r = _PyDict_ContainsId(GLOBALS(), &PyId___ltrace__);
        if (r < 0) {
            goto exit_eval_frame;
        }
        lltrace = r;
    }
#endif

    if (throwflag) { /* support for generator.throw() */
        throwflag = 0;
        goto error;
    }

#ifdef Py_DEBUG
    /* _PyEval_EvalFrameDefault() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

check_eval_breaker:
    {
        assert(STACK_LEVEL() >= 0); /* else underflow */
        assert(STACK_LEVEL() <= co->co_stacksize);  /* else overflow */
        assert(!_PyErr_Occurred(tstate));

        /* Do periodic things.  Doing this every time through
           the loop would add too much overhead, so we do it
           only every Nth instruction.  We also do it if
           ``pending.calls_to_do'' is set, i.e. when an asynchronous
           event needs attention (e.g. a signal handler or
           async I/O handler); see Py_AddPendingCall() and
           Py_MakePendingCalls() above. */

        if (_Py_atomic_load_relaxed(eval_breaker)) {
            opcode = _Py_OPCODE(*next_instr);
            if (opcode != BEFORE_ASYNC_WITH &&
                opcode != YIELD_FROM) {
                /* Few cases where we skip running signal handlers and other
                   pending calls:
                   - If we're about to enter the 'with:'. It will prevent
                     emitting a resource warning in the common idiom
                     'with open(path) as file:'.
                   - If we're about to enter the 'async with:'.
                   - If we're about to enter the 'try:' of a try/finally (not
                     *very* useful, but might help in some cases and it's
                     traditional)
                   - If we're resuming a chain of nested 'yield from' or
                     'await' calls, then each frame is parked with YIELD_FROM
                     as its next opcode. If the user hit control-C we want to
                     wait until we've reached the innermost frame before
                     running the signal handler and raising KeyboardInterrupt
                     (see bpo-30039).
                */
                if (eval_frame_handle_pending(tstate) != 0) {
                    goto error;
                }
             }
        }

    DISPATCH();

    /* Start instructions */
#if USE_COMPUTED_GOTOS
    {
#else
    dispatch_opcode:
        switch (opcode) {
#endif

        /* Variables used for making calls */
        PyObject *kwnames;
        int nargs;
        int postcall_shrink;

        /* BEWARE!
           It is essential that any operation that fails must goto error
           and that all operation that succeed call DISPATCH() ! */

        TARGET(NOP) {
            DISPATCH();
        }

        TARGET(LOAD_CLOSURE) {
            /* We keep LOAD_CLOSURE so that the bytecode stays more readable. */
            PyObject *value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            Py_INCREF(value);
            PUSH(value);
            DISPATCH();
        }

        TARGET(LOAD_FAST) {
            PyObject *value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            Py_INCREF(value);
            PUSH(value);
            DISPATCH();
        }

        TARGET(LOAD_CONST) {
            PREDICTED(LOAD_CONST);
            PyObject *value = GETITEM(consts, oparg);
            Py_INCREF(value);
            PUSH(value);
            DISPATCH();
        }

        TARGET(STORE_FAST) {
            PREDICTED(STORE_FAST);
            PyObject *value = POP();
            SETLOCAL(oparg, value);
            DISPATCH();
        }

        TARGET(LOAD_FAST__LOAD_FAST) {
            PyObject *value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            NEXTOPARG();
            next_instr++;
            Py_INCREF(value);
            PUSH(value);
            value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            Py_INCREF(value);
            PUSH(value);
            NOTRACE_DISPATCH();
        }

        TARGET(LOAD_FAST__LOAD_CONST) {
            PyObject *value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            NEXTOPARG();
            next_instr++;
            Py_INCREF(value);
            PUSH(value);
            value = GETITEM(consts, oparg);
            Py_INCREF(value);
            PUSH(value);
            NOTRACE_DISPATCH();
        }

        TARGET(STORE_FAST__LOAD_FAST) {
            PyObject *value = POP();
            SETLOCAL(oparg, value);
            NEXTOPARG();
            next_instr++;
            value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            Py_INCREF(value);
            PUSH(value);
            NOTRACE_DISPATCH();
        }

        TARGET(STORE_FAST__STORE_FAST) {
            PyObject *value = POP();
            SETLOCAL(oparg, value);
            NEXTOPARG();
            next_instr++;
            value = POP();
            SETLOCAL(oparg, value);
            NOTRACE_DISPATCH();
        }

        TARGET(LOAD_CONST__LOAD_FAST) {
            PyObject *value = GETITEM(consts, oparg);
            NEXTOPARG();
            next_instr++;
            Py_INCREF(value);
            PUSH(value);
            value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            Py_INCREF(value);
            PUSH(value);
            NOTRACE_DISPATCH();
        }

        TARGET(POP_TOP) {
            PyObject *value = POP();
            Py_DECREF(value);
            DISPATCH();
        }

        TARGET(ROT_TWO) {
            PyObject *top = TOP();
            PyObject *second = SECOND();
            SET_TOP(second);
            SET_SECOND(top);
            DISPATCH();
        }

        TARGET(ROT_THREE) {
            PyObject *top = TOP();
            PyObject *second = SECOND();
            PyObject *third = THIRD();
            SET_TOP(second);
            SET_SECOND(third);
            SET_THIRD(top);
            DISPATCH();
        }

        TARGET(ROT_FOUR) {
            PyObject *top = TOP();
            PyObject *second = SECOND();
            PyObject *third = THIRD();
            PyObject *fourth = FOURTH();
            SET_TOP(second);
            SET_SECOND(third);
            SET_THIRD(fourth);
            SET_FOURTH(top);
            DISPATCH();
        }

        TARGET(DUP_TOP) {
            PyObject *top = TOP();
            Py_INCREF(top);
            PUSH(top);
            DISPATCH();
        }

        TARGET(DUP_TOP_TWO) {
            PyObject *top = TOP();
            PyObject *second = SECOND();
            Py_INCREF(top);
            Py_INCREF(second);
            STACK_GROW(2);
            SET_TOP(top);
            SET_SECOND(second);
            DISPATCH();
        }

        TARGET(UNARY_POSITIVE) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Positive(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(UNARY_NEGATIVE) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Negative(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(UNARY_NOT) {
            PyObject *value = TOP();
            int err = PyObject_IsTrue(value);
            Py_DECREF(value);
            if (err == 0) {
                Py_INCREF(Py_True);
                SET_TOP(Py_True);
                DISPATCH();
            }
            else if (err > 0) {
                Py_INCREF(Py_False);
                SET_TOP(Py_False);
                DISPATCH();
            }
            STACK_SHRINK(1);
            goto error;
        }

        TARGET(UNARY_INVERT) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Invert(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_OP_MULTIPLY_INT) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyLong_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *prod = _PyLong_Multiply((PyLongObject *)left, (PyLongObject *)right);
            SET_SECOND(prod);
            Py_DECREF(right);
            Py_DECREF(left);
            STACK_SHRINK(1);
            if (prod == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_MULTIPLY_FLOAT) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyFloat_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dprod = ((PyFloatObject *)left)->ob_fval *
                ((PyFloatObject *)right)->ob_fval;
            PyObject *prod = PyFloat_FromDouble(dprod);
            SET_SECOND(prod);
            Py_DECREF(right);
            Py_DECREF(left);
            STACK_SHRINK(1);
            if (prod == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_SUBTRACT_INT) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyLong_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *sub = _PyLong_Subtract((PyLongObject *)left, (PyLongObject *)right);
            SET_SECOND(sub);
            Py_DECREF(right);
            Py_DECREF(left);
            STACK_SHRINK(1);
            if (sub == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_SUBTRACT_FLOAT) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyFloat_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dsub = ((PyFloatObject *)left)->ob_fval - ((PyFloatObject *)right)->ob_fval;
            PyObject *sub = PyFloat_FromDouble(dsub);
            SET_SECOND(sub);
            Py_DECREF(right);
            Py_DECREF(left);
            STACK_SHRINK(1);
            if (sub == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_ADD_UNICODE) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyUnicode_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *res = PyUnicode_Concat(left, right);
            STACK_SHRINK(1);
            SET_TOP(res);
            Py_DECREF(left);
            Py_DECREF(right);
            if (TOP() == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_INPLACE_ADD_UNICODE) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyUnicode_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            DEOPT_IF(Py_REFCNT(left) != 2, BINARY_OP);
            int next_oparg = _Py_OPARG(*next_instr);
            assert(_Py_OPCODE(*next_instr) == STORE_FAST);
            /* In the common case, there are 2 references to the value
            * stored in 'variable' when the v = v + ... is performed: one
            * on the value stack (in 'v') and one still stored in the
            * 'variable'.  We try to delete the variable now to reduce
            * the refcnt to 1.
            */
            PyObject *var = GETLOCAL(next_oparg);
            DEOPT_IF(var != left, BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            GETLOCAL(next_oparg) = NULL;
            Py_DECREF(left);
            STACK_SHRINK(1);
            PyUnicode_Append(&TOP(), right);
            Py_DECREF(right);
            if (TOP() == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_ADD_FLOAT) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dsum = ((PyFloatObject *)left)->ob_fval +
                ((PyFloatObject *)right)->ob_fval;
            PyObject *sum = PyFloat_FromDouble(dsum);
            SET_SECOND(sum);
            Py_DECREF(right);
            Py_DECREF(left);
            STACK_SHRINK(1);
            if (sum == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_ADD_INT) {
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *sum = _PyLong_Add((PyLongObject *)left, (PyLongObject *)right);
            SET_SECOND(sum);
            Py_DECREF(right);
            Py_DECREF(left);
            STACK_SHRINK(1);
            if (sum == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_SUBSCR) {
            PREDICTED(BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, unquickened);
            PyObject *sub = POP();
            PyObject *container = TOP();
            PyObject *res = PyObject_GetItem(container, sub);
            Py_DECREF(container);
            Py_DECREF(sub);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(BINARY_SUBSCR_ADAPTIVE) {
            SpecializedCacheEntry *cache = GET_CACHE();
            if (cache->adaptive.counter == 0) {
                PyObject *sub = TOP();
                PyObject *container = SECOND();
                next_instr--;
                if (_Py_Specialize_BinarySubscr(container, sub, next_instr, cache) < 0) {
                    goto error;
                }
                DISPATCH();
            }
            else {
                STAT_INC(BINARY_SUBSCR, deferred);
                cache->adaptive.counter--;
                assert(cache->adaptive.original_oparg == 0);
                /* No need to set oparg here; it isn't used by BINARY_SUBSCR */
                STAT_DEC(BINARY_SUBSCR, unquickened);
                JUMP_TO_INSTRUCTION(BINARY_SUBSCR);
            }
        }

        TARGET(BINARY_SUBSCR_LIST_INT) {
            PyObject *sub = TOP();
            PyObject *list = SECOND();
            DEOPT_IF(!PyLong_CheckExact(sub), BINARY_SUBSCR);
            DEOPT_IF(!PyList_CheckExact(list), BINARY_SUBSCR);

            // Deopt unless 0 <= sub < PyList_Size(list)
            Py_ssize_t signed_magnitude = Py_SIZE(sub);
            DEOPT_IF(((size_t)signed_magnitude) > 1, BINARY_SUBSCR);
            assert(((PyLongObject *)_PyLong_GetZero())->ob_digit[0] == 0);
            Py_ssize_t index = ((PyLongObject*)sub)->ob_digit[0];
            DEOPT_IF(index >= PyList_GET_SIZE(list), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res = PyList_GET_ITEM(list, index);
            assert(res != NULL);
            Py_INCREF(res);
            STACK_SHRINK(1);
            Py_DECREF(sub);
            SET_TOP(res);
            Py_DECREF(list);
            DISPATCH();
        }

        TARGET(BINARY_SUBSCR_TUPLE_INT) {
            PyObject *sub = TOP();
            PyObject *tuple = SECOND();
            DEOPT_IF(!PyLong_CheckExact(sub), BINARY_SUBSCR);
            DEOPT_IF(!PyTuple_CheckExact(tuple), BINARY_SUBSCR);

            // Deopt unless 0 <= sub < PyTuple_Size(list)
            Py_ssize_t signed_magnitude = Py_SIZE(sub);
            DEOPT_IF(((size_t)signed_magnitude) > 1, BINARY_SUBSCR);
            assert(((PyLongObject *)_PyLong_GetZero())->ob_digit[0] == 0);
            Py_ssize_t index = ((PyLongObject*)sub)->ob_digit[0];
            DEOPT_IF(index >= PyTuple_GET_SIZE(tuple), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res = PyTuple_GET_ITEM(tuple, index);
            assert(res != NULL);
            Py_INCREF(res);
            STACK_SHRINK(1);
            Py_DECREF(sub);
            SET_TOP(res);
            Py_DECREF(tuple);
            DISPATCH();
        }

        TARGET(BINARY_SUBSCR_DICT) {
            PyObject *dict = SECOND();
            DEOPT_IF(!PyDict_CheckExact(SECOND()), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *sub = TOP();
            PyObject *res = PyDict_GetItemWithError(dict, sub);
            if (res == NULL) {
                goto binary_subscr_dict_error;
            }
            Py_INCREF(res);
            STACK_SHRINK(1);
            Py_DECREF(sub);
            SET_TOP(res);
            Py_DECREF(dict);
            DISPATCH();
        }

        TARGET(BINARY_SUBSCR_GETITEM) {
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyObjectCache *cache1 = &caches[-1].obj;
            PyFunctionObject *getitem = (PyFunctionObject *)cache1->obj;
            DEOPT_IF(Py_TYPE(container)->tp_version_tag != cache0->version, BINARY_SUBSCR);
            DEOPT_IF(getitem->func_version != cache0->index, BINARY_SUBSCR);
            PyCodeObject *code = (PyCodeObject *)getitem->func_code;
            size_t size = code->co_nlocalsplus + code->co_stacksize + FRAME_SPECIALS_SIZE;
            assert(code->co_argcount == 2);
            InterpreterFrame *new_frame = _PyThreadState_BumpFramePointer(tstate, size);
            if (new_frame == NULL) {
                goto error;
            }
            _PyFrame_InitializeSpecials(new_frame, getitem,
                                        NULL, code->co_nlocalsplus);
            STACK_SHRINK(2);
            new_frame->localsplus[0] = container;
            new_frame->localsplus[1] = sub;
            for (int i = 2; i < code->co_nlocalsplus; i++) {
                new_frame->localsplus[i] = NULL;
            }
            _PyFrame_SetStackPointer(frame, stack_pointer);
            new_frame->previous = frame;
            frame = cframe.current_frame = new_frame;
            new_frame->depth = frame->depth + 1;
            goto start_frame;
        }

        TARGET(LIST_APPEND) {
            PyObject *v = POP();
            PyObject *list = PEEK(oparg);
            int err;
            err = PyList_Append(list, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            PREDICT(JUMP_ABSOLUTE);
            DISPATCH();
        }

        TARGET(SET_ADD) {
            PyObject *v = POP();
            PyObject *set = PEEK(oparg);
            int err;
            err = PySet_Add(set, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            PREDICT(JUMP_ABSOLUTE);
            DISPATCH();
        }

        TARGET(STORE_SUBSCR) {
            PREDICTED(STORE_SUBSCR);
            STAT_INC(STORE_SUBSCR, unquickened);
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            PyObject *v = THIRD();
            int err;
            STACK_SHRINK(3);
            /* container[sub] = v */
            err = PyObject_SetItem(container, sub, v);
            Py_DECREF(v);
            Py_DECREF(container);
            Py_DECREF(sub);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(STORE_SUBSCR_ADAPTIVE) {
            if (oparg == 0) {
                PyObject *sub = TOP();
                PyObject *container = SECOND();
                next_instr--;
                if (_Py_Specialize_StoreSubscr(container, sub, next_instr) < 0) {
                    goto error;
                }
                DISPATCH();
            }
            else {
                STAT_INC(STORE_SUBSCR, deferred);
                // oparg is the adaptive cache counter
                UPDATE_PREV_INSTR_OPARG(next_instr, oparg - 1);
                STAT_DEC(STORE_SUBSCR, unquickened);
                JUMP_TO_INSTRUCTION(STORE_SUBSCR);
            }
        }

        TARGET(STORE_SUBSCR_LIST_INT) {
            PyObject *sub = TOP();
            PyObject *list = SECOND();
            PyObject *value = THIRD();
            DEOPT_IF(!PyLong_CheckExact(sub), STORE_SUBSCR);
            DEOPT_IF(!PyList_CheckExact(list), STORE_SUBSCR);

            // Ensure nonnegative, zero-or-one-digit ints.
            DEOPT_IF(((size_t)Py_SIZE(sub)) > 1, STORE_SUBSCR);
            Py_ssize_t index = ((PyLongObject*)sub)->ob_digit[0];
            // Ensure index < len(list)
            DEOPT_IF(index >= PyList_GET_SIZE(list), STORE_SUBSCR);
            STAT_INC(STORE_SUBSCR, hit);

            PyObject *old_value = PyList_GET_ITEM(list, index);
            PyList_SET_ITEM(list, index, value);
            STACK_SHRINK(3);
            assert(old_value != NULL);
            Py_DECREF(old_value);
            Py_DECREF(sub);
            Py_DECREF(list);
            DISPATCH();
        }

        TARGET(STORE_SUBSCR_DICT) {
            PyObject *sub = TOP();
            PyObject *dict = SECOND();
            PyObject *value = THIRD();
            DEOPT_IF(!PyDict_CheckExact(dict), STORE_SUBSCR);
            STACK_SHRINK(3);
            STAT_INC(STORE_SUBSCR, hit);
            int err = _PyDict_SetItem_Take2((PyDictObject *)dict, sub, value);
            Py_DECREF(dict);
            if (err != 0) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(DELETE_SUBSCR) {
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            int err;
            STACK_SHRINK(2);
            /* del container[sub] */
            err = PyObject_DelItem(container, sub);
            Py_DECREF(container);
            Py_DECREF(sub);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(PRINT_EXPR) {
            _Py_IDENTIFIER(displayhook);
            PyObject *value = POP();
            PyObject *hook = _PySys_GetObjectId(&PyId_displayhook);
            PyObject *res;
            if (hook == NULL) {
                _PyErr_SetString(tstate, PyExc_RuntimeError,
                                 "lost sys.displayhook");
                Py_DECREF(value);
                goto error;
            }
            res = PyObject_CallOneArg(hook, value);
            Py_DECREF(value);
            if (res == NULL)
                goto error;
            Py_DECREF(res);
            DISPATCH();
        }

        TARGET(RAISE_VARARGS) {
            PyObject *cause = NULL, *exc = NULL;
            switch (oparg) {
            case 2:
                cause = POP(); /* cause */
                /* fall through */
            case 1:
                exc = POP(); /* exc */
                /* fall through */
            case 0:
                if (do_raise(tstate, exc, cause)) {
                    goto exception_unwind;
                }
                break;
            default:
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "bad RAISE_VARARGS oparg");
                break;
            }
            goto error;
        }

        TARGET(RETURN_VALUE) {
            retval = POP();
            assert(EMPTY());
            frame->f_state = FRAME_RETURNED;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            goto exiting;
        }

        TARGET(GET_AITER) {
            unaryfunc getter = NULL;
            PyObject *iter = NULL;
            PyObject *obj = TOP();
            PyTypeObject *type = Py_TYPE(obj);

            if (type->tp_as_async != NULL) {
                getter = type->tp_as_async->am_aiter;
            }

            if (getter != NULL) {
                iter = (*getter)(obj);
                Py_DECREF(obj);
                if (iter == NULL) {
                    SET_TOP(NULL);
                    goto error;
                }
            }
            else {
                SET_TOP(NULL);
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' requires an object with "
                              "__aiter__ method, got %.100s",
                              type->tp_name);
                Py_DECREF(obj);
                goto error;
            }

            if (Py_TYPE(iter)->tp_as_async == NULL ||
                    Py_TYPE(iter)->tp_as_async->am_anext == NULL) {

                SET_TOP(NULL);
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' received an object from __aiter__ "
                              "that does not implement __anext__: %.100s",
                              Py_TYPE(iter)->tp_name);
                Py_DECREF(iter);
                goto error;
            }

            SET_TOP(iter);
            DISPATCH();
        }

        TARGET(GET_ANEXT) {
            unaryfunc getter = NULL;
            PyObject *next_iter = NULL;
            PyObject *awaitable = NULL;
            PyObject *aiter = TOP();
            PyTypeObject *type = Py_TYPE(aiter);

            if (PyAsyncGen_CheckExact(aiter)) {
                awaitable = type->tp_as_async->am_anext(aiter);
                if (awaitable == NULL) {
                    goto error;
                }
            } else {
                if (type->tp_as_async != NULL){
                    getter = type->tp_as_async->am_anext;
                }

                if (getter != NULL) {
                    next_iter = (*getter)(aiter);
                    if (next_iter == NULL) {
                        goto error;
                    }
                }
                else {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'async for' requires an iterator with "
                                  "__anext__ method, got %.100s",
                                  type->tp_name);
                    goto error;
                }

                awaitable = _PyCoro_GetAwaitableIter(next_iter);
                if (awaitable == NULL) {
                    _PyErr_FormatFromCause(
                        PyExc_TypeError,
                        "'async for' received an invalid object "
                        "from __anext__: %.100s",
                        Py_TYPE(next_iter)->tp_name);

                    Py_DECREF(next_iter);
                    goto error;
                } else {
                    Py_DECREF(next_iter);
                }
            }

            PUSH(awaitable);
            PREDICT(LOAD_CONST);
            DISPATCH();
        }

        TARGET(GET_AWAITABLE) {
            PREDICTED(GET_AWAITABLE);
            PyObject *iterable = TOP();
            PyObject *iter = _PyCoro_GetAwaitableIter(iterable);

            if (iter == NULL) {
                int opcode_at_minus_3 = 0;
                if ((next_instr - first_instr) > 2) {
                    opcode_at_minus_3 = _Py_OPCODE(next_instr[-3]);
                }
                format_awaitable_error(tstate, Py_TYPE(iterable),
                                       opcode_at_minus_3,
                                       _Py_OPCODE(next_instr[-2]));
            }

            Py_DECREF(iterable);

            if (iter != NULL && PyCoro_CheckExact(iter)) {
                PyObject *yf = _PyGen_yf((PyGenObject*)iter);
                if (yf != NULL) {
                    /* `iter` is a coroutine object that is being
                       awaited, `yf` is a pointer to the current awaitable
                       being awaited on. */
                    Py_DECREF(yf);
                    Py_CLEAR(iter);
                    _PyErr_SetString(tstate, PyExc_RuntimeError,
                                     "coroutine is being awaited already");
                    /* The code below jumps to `error` if `iter` is NULL. */
                }
            }

            SET_TOP(iter); /* Even if it's NULL */

            if (iter == NULL) {
                goto error;
            }

            PREDICT(LOAD_CONST);
            DISPATCH();
        }

        TARGET(YIELD_FROM) {
            PyObject *v = POP();
            PyObject *receiver = TOP();
            PySendResult gen_status;
            if (tstate->c_tracefunc == NULL) {
                gen_status = PyIter_Send(receiver, v, &retval);
            } else {
                _Py_IDENTIFIER(send);
                if (Py_IsNone(v) && PyIter_Check(receiver)) {
                    retval = Py_TYPE(receiver)->tp_iternext(receiver);
                }
                else {
                    retval = _PyObject_CallMethodIdOneArg(receiver, &PyId_send, v);
                }
                if (retval == NULL) {
                    if (tstate->c_tracefunc != NULL
                            && _PyErr_ExceptionMatches(tstate, PyExc_StopIteration))
                        call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, frame);
                    if (_PyGen_FetchStopIterationValue(&retval) == 0) {
                        gen_status = PYGEN_RETURN;
                    }
                    else {
                        gen_status = PYGEN_ERROR;
                    }
                }
                else {
                    gen_status = PYGEN_NEXT;
                }
            }
            Py_DECREF(v);
            if (gen_status == PYGEN_ERROR) {
                assert (retval == NULL);
                goto error;
            }
            if (gen_status == PYGEN_RETURN) {
                assert (retval != NULL);

                Py_DECREF(receiver);
                SET_TOP(retval);
                retval = NULL;
                DISPATCH();
            }
            assert (gen_status == PYGEN_NEXT);
            /* receiver remains on stack, retval is value to be yielded */
            /* and repeat... */
            assert(frame->f_lasti > 0);
            frame->f_lasti -= 1;
            frame->f_state = FRAME_SUSPENDED;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            goto exiting;
        }

        TARGET(YIELD_VALUE) {
            retval = POP();

            if (co->co_flags & CO_ASYNC_GENERATOR) {
                PyObject *w = _PyAsyncGenValueWrapperNew(retval);
                Py_DECREF(retval);
                if (w == NULL) {
                    retval = NULL;
                    goto error;
                }
                retval = w;
            }
            frame->f_state = FRAME_SUSPENDED;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            goto exiting;
        }

        TARGET(GEN_START) {
            PyObject *none = POP();
            Py_DECREF(none);
            if (!Py_IsNone(none)) {
                if (oparg > 2) {
                    _PyErr_SetString(tstate, PyExc_SystemError,
                        "Illegal kind for GEN_START");
                }
                else {
                    static const char *gen_kind[3] = {
                        "generator",
                        "coroutine",
                        "async generator"
                    };
                    _PyErr_Format(tstate, PyExc_TypeError,
                        "can't send non-None value to a "
                                "just-started %s",
                                gen_kind[oparg]);
                }
                goto error;
            }
            DISPATCH();
        }

        TARGET(POP_EXCEPT) {
            PyObject *type, *value, *traceback;
            _PyErr_StackItem *exc_info;
            exc_info = tstate->exc_info;
            type = exc_info->exc_type;
            value = exc_info->exc_value;
            traceback = exc_info->exc_traceback;
            exc_info->exc_type = POP();
            exc_info->exc_value = POP();
            exc_info->exc_traceback = POP();
            ASSERT_EXC_TYPE_IS_REDUNDANT(exc_info->exc_type, exc_info->exc_value);
            Py_XDECREF(type);
            Py_XDECREF(value);
            Py_XDECREF(traceback);
            DISPATCH();
        }

        TARGET(POP_EXCEPT_AND_RERAISE) {
            PyObject *lasti = PEEK(4);
            if (PyLong_Check(lasti)) {
                frame->f_lasti = PyLong_AsLong(lasti);
                assert(!_PyErr_Occurred(tstate));
            }
            else {
                _PyErr_SetString(tstate, PyExc_SystemError, "lasti is not an int");
                goto error;
            }
            PyObject *type, *value, *traceback;
            _PyErr_StackItem *exc_info;
            type = POP();
            value = POP();
            traceback = POP();
            ASSERT_EXC_TYPE_IS_REDUNDANT(type, value);
            Py_DECREF(POP()); /* lasti */
            _PyErr_Restore(tstate, type, value, traceback);
            exc_info = tstate->exc_info;
            type = exc_info->exc_type;
            value = exc_info->exc_value;
            traceback = exc_info->exc_traceback;
            exc_info->exc_type = POP();
            exc_info->exc_value = POP();
            exc_info->exc_traceback = POP();
            ASSERT_EXC_TYPE_IS_REDUNDANT(exc_info->exc_type, exc_info->exc_value);
            Py_XDECREF(type);
            Py_XDECREF(value);
            Py_XDECREF(traceback);
            goto exception_unwind;
        }

        TARGET(RERAISE) {
            if (oparg) {
                PyObject *lasti = PEEK(oparg+3);
                if (PyLong_Check(lasti)) {
                    frame->f_lasti = PyLong_AsLong(lasti);
                    assert(!_PyErr_Occurred(tstate));
                }
                else {
                    assert(PyLong_Check(lasti));
                    _PyErr_SetString(tstate, PyExc_SystemError, "lasti is not an int");
                    goto error;
                }
            }
            PyObject *exc = POP();
            PyObject *val = POP();
            PyObject *tb = POP();
            ASSERT_EXC_TYPE_IS_REDUNDANT(exc, val);
            assert(PyExceptionClass_Check(exc));
            _PyErr_Restore(tstate, exc, val, tb);
            goto exception_unwind;
        }

        TARGET(END_ASYNC_FOR) {
            PyObject *exc = POP();
            PyObject *val = POP();
            PyObject *tb = POP();
            ASSERT_EXC_TYPE_IS_REDUNDANT(exc, val);
            assert(PyExceptionClass_Check(exc));
            if (PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration)) {
                Py_DECREF(exc);
                Py_DECREF(val);
                Py_DECREF(tb);
                Py_DECREF(POP());
                DISPATCH();
            }
            else {
                _PyErr_Restore(tstate, exc, val, tb);
                goto exception_unwind;
            }
        }

        TARGET(LOAD_ASSERTION_ERROR) {
            PyObject *value = PyExc_AssertionError;
            Py_INCREF(value);
            PUSH(value);
            DISPATCH();
        }

        TARGET(LOAD_BUILD_CLASS) {
            _Py_IDENTIFIER(__build_class__);

            PyObject *bc;
            if (PyDict_CheckExact(BUILTINS())) {
                bc = _PyDict_GetItemIdWithError(BUILTINS(), &PyId___build_class__);
                if (bc == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        _PyErr_SetString(tstate, PyExc_NameError,
                                         "__build_class__ not found");
                    }
                    goto error;
                }
                Py_INCREF(bc);
            }
            else {
                PyObject *build_class_str = _PyUnicode_FromId(&PyId___build_class__);
                if (build_class_str == NULL)
                    goto error;
                bc = PyObject_GetItem(BUILTINS(), build_class_str);
                if (bc == NULL) {
                    if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                        _PyErr_SetString(tstate, PyExc_NameError,
                                         "__build_class__ not found");
                    goto error;
                }
            }
            PUSH(bc);
            DISPATCH();
        }

        TARGET(STORE_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *v = POP();
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when storing %R", name);
                Py_DECREF(v);
                goto error;
            }
            if (PyDict_CheckExact(ns))
                err = PyDict_SetItem(ns, name, v);
            else
                err = PyObject_SetItem(ns, name, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(DELETE_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when deleting %R", name);
                goto error;
            }
            err = PyObject_DelItem(ns, name);
            if (err != 0) {
                format_exc_check_arg(tstate, PyExc_NameError,
                                     NAME_ERROR_MSG,
                                     name);
                goto error;
            }
            DISPATCH();
        }

        TARGET(UNPACK_SEQUENCE) {
            PREDICTED(UNPACK_SEQUENCE);
            PyObject *seq = POP(), *item, **items;
            if (PyTuple_CheckExact(seq) &&
                PyTuple_GET_SIZE(seq) == oparg) {
                items = ((PyTupleObject *)seq)->ob_item;
                while (oparg--) {
                    item = items[oparg];
                    Py_INCREF(item);
                    PUSH(item);
                }
            } else if (PyList_CheckExact(seq) &&
                       PyList_GET_SIZE(seq) == oparg) {
                items = ((PyListObject *)seq)->ob_item;
                while (oparg--) {
                    item = items[oparg];
                    Py_INCREF(item);
                    PUSH(item);
                }
            } else if (unpack_iterable(tstate, seq, oparg, -1,
                                       stack_pointer + oparg)) {
                STACK_GROW(oparg);
            } else {
                /* unpack_iterable() raised an exception */
                Py_DECREF(seq);
                goto error;
            }
            Py_DECREF(seq);
            DISPATCH();
        }

        TARGET(UNPACK_EX) {
            int totalargs = 1 + (oparg & 0xFF) + (oparg >> 8);
            PyObject *seq = POP();

            if (unpack_iterable(tstate, seq, oparg & 0xFF, oparg >> 8,
                                stack_pointer + totalargs)) {
                stack_pointer += totalargs;
            } else {
                Py_DECREF(seq);
                goto error;
            }
            Py_DECREF(seq);
            DISPATCH();
        }

        TARGET(STORE_ATTR) {
            PREDICTED(STORE_ATTR);
            STAT_INC(STORE_ATTR, unquickened);
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = TOP();
            PyObject *v = SECOND();
            int err;
            STACK_SHRINK(2);
            err = PyObject_SetAttr(owner, name, v);
            Py_DECREF(v);
            Py_DECREF(owner);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(DELETE_ATTR) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = POP();
            int err;
            err = PyObject_SetAttr(owner, name, (PyObject *)NULL);
            Py_DECREF(owner);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(STORE_GLOBAL) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *v = POP();
            int err;
            err = PyDict_SetItem(GLOBALS(), name, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(DELETE_GLOBAL) {
            PyObject *name = GETITEM(names, oparg);
            int err;
            err = PyDict_DelItem(GLOBALS(), name);
            if (err != 0) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                    format_exc_check_arg(tstate, PyExc_NameError,
                                         NAME_ERROR_MSG, name);
                }
                goto error;
            }
            DISPATCH();
        }

        TARGET(LOAD_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *locals = LOCALS();
            PyObject *v;
            if (locals == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when loading %R", name);
                goto error;
            }
            if (PyDict_CheckExact(locals)) {
                v = PyDict_GetItemWithError(locals, name);
                if (v != NULL) {
                    Py_INCREF(v);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            }
            else {
                v = PyObject_GetItem(locals, name);
                if (v == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                        goto error;
                    _PyErr_Clear(tstate);
                }
            }
            if (v == NULL) {
                v = PyDict_GetItemWithError(GLOBALS(), name);
                if (v != NULL) {
                    Py_INCREF(v);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
                else {
                    if (PyDict_CheckExact(BUILTINS())) {
                        v = PyDict_GetItemWithError(BUILTINS(), name);
                        if (v == NULL) {
                            if (!_PyErr_Occurred(tstate)) {
                                format_exc_check_arg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                            }
                            goto error;
                        }
                        Py_INCREF(v);
                    }
                    else {
                        v = PyObject_GetItem(BUILTINS(), name);
                        if (v == NULL) {
                            if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                                format_exc_check_arg(
                                            tstate, PyExc_NameError,
                                            NAME_ERROR_MSG, name);
                            }
                            goto error;
                        }
                    }
                }
            }
            PUSH(v);
            DISPATCH();
        }

        TARGET(LOAD_GLOBAL) {
            PREDICTED(LOAD_GLOBAL);
            STAT_INC(LOAD_GLOBAL, unquickened);
            PyObject *name = GETITEM(names, oparg);
            PyObject *v;
            if (PyDict_CheckExact(GLOBALS())
                && PyDict_CheckExact(BUILTINS()))
            {
                v = _PyDict_LoadGlobal((PyDictObject *)GLOBALS(),
                                       (PyDictObject *)BUILTINS(),
                                       name);
                if (v == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        /* _PyDict_LoadGlobal() returns NULL without raising
                         * an exception if the key doesn't exist */
                        format_exc_check_arg(tstate, PyExc_NameError,
                                             NAME_ERROR_MSG, name);
                    }
                    goto error;
                }
                Py_INCREF(v);
            }
            else {
                /* Slow-path if globals or builtins is not a dict */

                /* namespace 1: globals */
                name = GETITEM(names, oparg);
                v = PyObject_GetItem(GLOBALS(), name);
                if (v == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);

                    /* namespace 2: builtins */
                    v = PyObject_GetItem(BUILTINS(), name);
                    if (v == NULL) {
                        if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                            format_exc_check_arg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                        }
                        goto error;
                    }
                }
            }
            PUSH(v);
            DISPATCH();
        }

        TARGET(LOAD_GLOBAL_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            SpecializedCacheEntry *cache = GET_CACHE();
            if (cache->adaptive.counter == 0) {
                PyObject *name = GETITEM(names, cache->adaptive.original_oparg);
                next_instr--;
                if (_Py_Specialize_LoadGlobal(GLOBALS(), BUILTINS(), next_instr, name, cache) < 0) {
                    goto error;
                }
                DISPATCH();
            }
            else {
                STAT_INC(LOAD_GLOBAL, deferred);
                cache->adaptive.counter--;
                oparg = cache->adaptive.original_oparg;
                STAT_DEC(LOAD_GLOBAL, unquickened);
                JUMP_TO_INSTRUCTION(LOAD_GLOBAL);
            }
        }

        TARGET(LOAD_GLOBAL_MODULE) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(GLOBALS()), LOAD_GLOBAL);
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyLoadGlobalCache *cache1 = &caches[-1].load_global;
            DEOPT_IF(dict->ma_keys->dk_version != cache1->module_keys_version, LOAD_GLOBAL);
            PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + cache0->index;
            PyObject *res = ep->me_value;
            DEOPT_IF(res == NULL, LOAD_GLOBAL);
            STAT_INC(LOAD_GLOBAL, hit);
            Py_INCREF(res);
            PUSH(res);
            DISPATCH();
        }

        TARGET(LOAD_GLOBAL_BUILTIN) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(GLOBALS()), LOAD_GLOBAL);
            DEOPT_IF(!PyDict_CheckExact(BUILTINS()), LOAD_GLOBAL);
            PyDictObject *mdict = (PyDictObject *)GLOBALS();
            PyDictObject *bdict = (PyDictObject *)BUILTINS();
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyLoadGlobalCache *cache1 = &caches[-1].load_global;
            DEOPT_IF(mdict->ma_keys->dk_version != cache1->module_keys_version, LOAD_GLOBAL);
            DEOPT_IF(bdict->ma_keys->dk_version != cache1->builtin_keys_version, LOAD_GLOBAL);
            PyDictKeyEntry *ep = DK_ENTRIES(bdict->ma_keys) + cache0->index;
            PyObject *res = ep->me_value;
            DEOPT_IF(res == NULL, LOAD_GLOBAL);
            STAT_INC(LOAD_GLOBAL, hit);
            Py_INCREF(res);
            PUSH(res);
            DISPATCH();
        }

        TARGET(DELETE_FAST) {
            PyObject *v = GETLOCAL(oparg);
            if (v != NULL) {
                SETLOCAL(oparg, NULL);
                DISPATCH();
            }
            format_exc_check_arg(
                tstate, PyExc_UnboundLocalError,
                UNBOUNDLOCAL_ERROR_MSG,
                PyTuple_GetItem(co->co_localsplusnames, oparg)
                );
            goto error;
        }

        TARGET(MAKE_CELL) {
            // "initial" is probably NULL but not if it's an arg (or set
            // via PyFrame_LocalsToFast() before MAKE_CELL has run).
            PyObject *initial = GETLOCAL(oparg);
            PyObject *cell = PyCell_New(initial);
            if (cell == NULL) {
                goto error;
            }
            SETLOCAL(oparg, cell);
            DISPATCH();
        }

        TARGET(DELETE_DEREF) {
            PyObject *cell = GETLOCAL(oparg);
            PyObject *oldobj = PyCell_GET(cell);
            if (oldobj != NULL) {
                PyCell_SET(cell, NULL);
                Py_DECREF(oldobj);
                DISPATCH();
            }
            format_exc_unbound(tstate, co, oparg);
            goto error;
        }

        TARGET(LOAD_CLASSDEREF) {
            PyObject *name, *value, *locals = LOCALS();
            assert(locals);
            assert(oparg >= 0 && oparg < co->co_nlocalsplus);
            name = PyTuple_GET_ITEM(co->co_localsplusnames, oparg);
            if (PyDict_CheckExact(locals)) {
                value = PyDict_GetItemWithError(locals, name);
                if (value != NULL) {
                    Py_INCREF(value);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            }
            else {
                value = PyObject_GetItem(locals, name);
                if (value == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                }
            }
            if (!value) {
                PyObject *cell = GETLOCAL(oparg);
                value = PyCell_GET(cell);
                if (value == NULL) {
                    format_exc_unbound(tstate, co, oparg);
                    goto error;
                }
                Py_INCREF(value);
            }
            PUSH(value);
            DISPATCH();
        }

        TARGET(LOAD_DEREF) {
            PyObject *cell = GETLOCAL(oparg);
            PyObject *value = PyCell_GET(cell);
            if (value == NULL) {
                format_exc_unbound(tstate, co, oparg);
                goto error;
            }
            Py_INCREF(value);
            PUSH(value);
            DISPATCH();
        }

        TARGET(STORE_DEREF) {
            PyObject *v = POP();
            PyObject *cell = GETLOCAL(oparg);
            PyObject *oldobj = PyCell_GET(cell);
            PyCell_SET(cell, v);
            Py_XDECREF(oldobj);
            DISPATCH();
        }

        TARGET(COPY_FREE_VARS) {
            /* Copy closure variables to free variables */
            PyCodeObject *co = frame->f_code;
            PyObject *closure = frame->f_func->func_closure;
            int offset = co->co_nlocals + co->co_nplaincellvars;
            assert(oparg == co->co_nfreevars);
            for (int i = 0; i < oparg; ++i) {
                PyObject *o = PyTuple_GET_ITEM(closure, i);
                Py_INCREF(o);
                frame->localsplus[offset + i] = o;
            }
            DISPATCH();
        }

        TARGET(BUILD_STRING) {
            PyObject *str;
            PyObject *empty = PyUnicode_New(0, 0);
            if (empty == NULL) {
                goto error;
            }
            str = _PyUnicode_JoinArray(empty, stack_pointer - oparg, oparg);
            Py_DECREF(empty);
            if (str == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                Py_DECREF(item);
            }
            PUSH(str);
            DISPATCH();
        }

        TARGET(BUILD_TUPLE) {
            PyObject *tup = PyTuple_New(oparg);
            if (tup == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                PyTuple_SET_ITEM(tup, oparg, item);
            }
            PUSH(tup);
            DISPATCH();
        }

        TARGET(BUILD_LIST) {
            PyObject *list =  PyList_New(oparg);
            if (list == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                PyList_SET_ITEM(list, oparg, item);
            }
            PUSH(list);
            DISPATCH();
        }

        TARGET(LIST_TO_TUPLE) {
            PyObject *list = POP();
            PyObject *tuple = PyList_AsTuple(list);
            Py_DECREF(list);
            if (tuple == NULL) {
                goto error;
            }
            PUSH(tuple);
            DISPATCH();
        }

        TARGET(LIST_EXTEND) {
            PyObject *iterable = POP();
            PyObject *list = PEEK(oparg);
            PyObject *none_val = _PyList_Extend((PyListObject *)list, iterable);
            if (none_val == NULL) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
                   (Py_TYPE(iterable)->tp_iter == NULL && !PySequence_Check(iterable)))
                {
                    _PyErr_Clear(tstate);
                    _PyErr_Format(tstate, PyExc_TypeError,
                          "Value after * must be an iterable, not %.200s",
                          Py_TYPE(iterable)->tp_name);
                }
                Py_DECREF(iterable);
                goto error;
            }
            Py_DECREF(none_val);
            Py_DECREF(iterable);
            DISPATCH();
        }

        TARGET(SET_UPDATE) {
            PyObject *iterable = POP();
            PyObject *set = PEEK(oparg);
            int err = _PySet_Update(set, iterable);
            Py_DECREF(iterable);
            if (err < 0) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BUILD_SET) {
            PyObject *set = PySet_New(NULL);
            int err = 0;
            int i;
            if (set == NULL)
                goto error;
            for (i = oparg; i > 0; i--) {
                PyObject *item = PEEK(i);
                if (err == 0)
                    err = PySet_Add(set, item);
                Py_DECREF(item);
            }
            STACK_SHRINK(oparg);
            if (err != 0) {
                Py_DECREF(set);
                goto error;
            }
            PUSH(set);
            DISPATCH();
        }

        TARGET(BUILD_MAP) {
            Py_ssize_t i;
            PyObject *map = _PyDict_NewPresized((Py_ssize_t)oparg);
            if (map == NULL)
                goto error;
            for (i = oparg; i > 0; i--) {
                int err;
                PyObject *key = PEEK(2*i);
                PyObject *value = PEEK(2*i - 1);
                err = PyDict_SetItem(map, key, value);
                if (err != 0) {
                    Py_DECREF(map);
                    goto error;
                }
            }

            while (oparg--) {
                Py_DECREF(POP());
                Py_DECREF(POP());
            }
            PUSH(map);
            DISPATCH();
        }

        TARGET(SETUP_ANNOTATIONS) {
            _Py_IDENTIFIER(__annotations__);
            int err;
            PyObject *ann_dict;
            if (LOCALS() == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when setting up annotations");
                goto error;
            }
            /* check if __annotations__ in locals()... */
            if (PyDict_CheckExact(LOCALS())) {
                ann_dict = _PyDict_GetItemIdWithError(LOCALS(),
                                             &PyId___annotations__);
                if (ann_dict == NULL) {
                    if (_PyErr_Occurred(tstate)) {
                        goto error;
                    }
                    /* ...if not, create a new one */
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    err = _PyDict_SetItemId(LOCALS(),
                                            &PyId___annotations__, ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                }
            }
            else {
                /* do the same if locals() is not a dict */
                PyObject *ann_str = _PyUnicode_FromId(&PyId___annotations__);
                if (ann_str == NULL) {
                    goto error;
                }
                ann_dict = PyObject_GetItem(LOCALS(), ann_str);
                if (ann_dict == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    err = PyObject_SetItem(LOCALS(), ann_str, ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                }
                else {
                    Py_DECREF(ann_dict);
                }
            }
            DISPATCH();
        }

        TARGET(BUILD_CONST_KEY_MAP) {
            Py_ssize_t i;
            PyObject *map;
            PyObject *keys = TOP();
            if (!PyTuple_CheckExact(keys) ||
                PyTuple_GET_SIZE(keys) != (Py_ssize_t)oparg) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "bad BUILD_CONST_KEY_MAP keys argument");
                goto error;
            }
            map = _PyDict_NewPresized((Py_ssize_t)oparg);
            if (map == NULL) {
                goto error;
            }
            for (i = oparg; i > 0; i--) {
                int err;
                PyObject *key = PyTuple_GET_ITEM(keys, oparg - i);
                PyObject *value = PEEK(i + 1);
                err = PyDict_SetItem(map, key, value);
                if (err != 0) {
                    Py_DECREF(map);
                    goto error;
                }
            }

            Py_DECREF(POP());
            while (oparg--) {
                Py_DECREF(POP());
            }
            PUSH(map);
            DISPATCH();
        }

        TARGET(DICT_UPDATE) {
            PyObject *update = POP();
            PyObject *dict = PEEK(oparg);
            if (PyDict_Update(dict, update) < 0) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                    "'%.200s' object is not a mapping",
                                    Py_TYPE(update)->tp_name);
                }
                Py_DECREF(update);
                goto error;
            }
            Py_DECREF(update);
            DISPATCH();
        }

        TARGET(DICT_MERGE) {
            PyObject *update = POP();
            PyObject *dict = PEEK(oparg);

            if (_PyDict_MergeEx(dict, update, 2) < 0) {
                format_kwargs_error(tstate, PEEK(2 + oparg), update);
                Py_DECREF(update);
                goto error;
            }
            Py_DECREF(update);
            PREDICT(CALL_FUNCTION_EX);
            DISPATCH();
        }

        TARGET(MAP_ADD) {
            PyObject *value = TOP();
            PyObject *key = SECOND();
            PyObject *map;
            STACK_SHRINK(2);
            map = PEEK(oparg);                      /* dict */
            assert(PyDict_CheckExact(map));
            /* map[key] = value */
            if (_PyDict_SetItem_Take2((PyDictObject *)map, key, value) != 0) {
                goto error;
            }
            PREDICT(JUMP_ABSOLUTE);
            DISPATCH();
        }

        TARGET(LOAD_ATTR) {
            PREDICTED(LOAD_ATTR);
            STAT_INC(LOAD_ATTR, unquickened);
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = TOP();
            PyObject *res = PyObject_GetAttr(owner, name);
            if (res == NULL) {
                goto error;
            }
            Py_DECREF(owner);
            SET_TOP(res);
            DISPATCH();
        }

        TARGET(LOAD_ATTR_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            SpecializedCacheEntry *cache = GET_CACHE();
            if (cache->adaptive.counter == 0) {
                PyObject *owner = TOP();
                PyObject *name = GETITEM(names, cache->adaptive.original_oparg);
                next_instr--;
                if (_Py_Specialize_LoadAttr(owner, next_instr, name, cache) < 0) {
                    goto error;
                }
                DISPATCH();
            }
            else {
                STAT_INC(LOAD_ATTR, deferred);
                cache->adaptive.counter--;
                oparg = cache->adaptive.original_oparg;
                STAT_DEC(LOAD_ATTR, unquickened);
                JUMP_TO_INSTRUCTION(LOAD_ATTR);
            }
        }

        TARGET(LOAD_ATTR_INSTANCE_VALUE) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            PyTypeObject *tp = Py_TYPE(owner);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyAttrCache *cache1 = &caches[-1].attr;
            assert(cache1->tp_version != 0);
            DEOPT_IF(tp->tp_version_tag != cache1->tp_version, LOAD_ATTR);
            assert(tp->tp_dictoffset > 0);
            assert(tp->tp_inline_values_offset > 0);
            PyDictValues *values = *(PyDictValues **)(((char *)owner) + tp->tp_inline_values_offset);
            DEOPT_IF(values == NULL, LOAD_ATTR);
            res = values->values[cache0->index];
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            SET_TOP(res);
            Py_DECREF(owner);
            DISPATCH();
        }

        TARGET(LOAD_ATTR_MODULE) {
            assert(cframe.use_tracing == 0);
            // shared with LOAD_METHOD_MODULE
            PyObject *owner = TOP();
            PyObject *res;
            LOAD_MODULE_ATTR_OR_METHOD(ATTR);
            SET_TOP(res);
            Py_DECREF(owner);
            DISPATCH();
        }

        TARGET(LOAD_ATTR_WITH_HINT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            PyTypeObject *tp = Py_TYPE(owner);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyAttrCache *cache1 = &caches[-1].attr;
            assert(cache1->tp_version != 0);
            DEOPT_IF(tp->tp_version_tag != cache1->tp_version, LOAD_ATTR);
            assert(tp->tp_dictoffset > 0);
            PyDictObject *dict = *(PyDictObject **)(((char *)owner) + tp->tp_dictoffset);
            DEOPT_IF(dict == NULL, LOAD_ATTR);
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(names, cache0->original_oparg);
            uint32_t hint = cache1->dk_version_or_hint;
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries, LOAD_ATTR);
            PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + hint;
            DEOPT_IF(ep->me_key != name, LOAD_ATTR);
            res = ep->me_value;
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            SET_TOP(res);
            Py_DECREF(owner);
            DISPATCH();
        }

        TARGET(LOAD_ATTR_SLOT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            PyTypeObject *tp = Py_TYPE(owner);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyAttrCache *cache1 = &caches[-1].attr;
            assert(cache1->tp_version != 0);
            DEOPT_IF(tp->tp_version_tag != cache1->tp_version, LOAD_ATTR);
            char *addr = (char *)owner + cache0->index;
            res = *(PyObject **)addr;
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            SET_TOP(res);
            Py_DECREF(owner);
            DISPATCH();
        }

        TARGET(STORE_ATTR_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            SpecializedCacheEntry *cache = GET_CACHE();
            if (cache->adaptive.counter == 0) {
                PyObject *owner = TOP();
                PyObject *name = GETITEM(names, cache->adaptive.original_oparg);
                next_instr--;
                if (_Py_Specialize_StoreAttr(owner, next_instr, name, cache) < 0) {
                    goto error;
                }
                DISPATCH();
            }
            else {
                STAT_INC(STORE_ATTR, deferred);
                cache->adaptive.counter--;
                oparg = cache->adaptive.original_oparg;
                STAT_DEC(STORE_ATTR, unquickened);
                JUMP_TO_INSTRUCTION(STORE_ATTR);
            }
        }

        TARGET(STORE_ATTR_INSTANCE_VALUE) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyTypeObject *tp = Py_TYPE(owner);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyAttrCache *cache1 = &caches[-1].attr;
            assert(cache1->tp_version != 0);
            DEOPT_IF(tp->tp_version_tag != cache1->tp_version, STORE_ATTR);
            assert(tp->tp_dictoffset > 0);
            assert(tp->tp_inline_values_offset > 0);
            PyDictValues *values = *(PyDictValues **)(((char *)owner) + tp->tp_inline_values_offset);
            DEOPT_IF(values == NULL, STORE_ATTR);
            STAT_INC(STORE_ATTR, hit);
            int index = cache0->index;
            STACK_SHRINK(1);
            PyObject *value = POP();
            PyObject *old_value = values->values[index];
            values->values[index] = value;
            if (old_value == NULL) {
                assert(index < 16);
                values->mv_order = (values->mv_order << 4) | index;
            }
            else {
                Py_DECREF(old_value);
            }
            Py_DECREF(owner);
            DISPATCH();
        }

        TARGET(STORE_ATTR_WITH_HINT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyTypeObject *tp = Py_TYPE(owner);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyAttrCache *cache1 = &caches[-1].attr;
            assert(cache1->tp_version != 0);
            DEOPT_IF(tp->tp_version_tag != cache1->tp_version, STORE_ATTR);
            assert(tp->tp_dictoffset > 0);
            PyDictObject *dict = *(PyDictObject **)(((char *)owner) + tp->tp_dictoffset);
            DEOPT_IF(dict == NULL, STORE_ATTR);
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(names, cache0->original_oparg);
            uint32_t hint = cache1->dk_version_or_hint;
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries, STORE_ATTR);
            PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + hint;
            DEOPT_IF(ep->me_key != name, STORE_ATTR);
            PyObject *old_value = ep->me_value;
            DEOPT_IF(old_value == NULL, STORE_ATTR);
            STAT_INC(STORE_ATTR, hit);
            STACK_SHRINK(1);
            PyObject *value = POP();
            ep->me_value = value;
            Py_DECREF(old_value);
            /* Ensure dict is GC tracked if it needs to be */
            if (!_PyObject_GC_IS_TRACKED(dict) && _PyObject_GC_MAY_BE_TRACKED(value)) {
                _PyObject_GC_TRACK(dict);
            }
            /* PEP 509 */
            dict->ma_version_tag = DICT_NEXT_VERSION();
            Py_DECREF(owner);
            DISPATCH();
        }

        TARGET(STORE_ATTR_SLOT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyTypeObject *tp = Py_TYPE(owner);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            _PyAttrCache *cache1 = &caches[-1].attr;
            assert(cache1->tp_version != 0);
            DEOPT_IF(tp->tp_version_tag != cache1->tp_version, STORE_ATTR);
            char *addr = (char *)owner + cache0->index;
            STAT_INC(STORE_ATTR, hit);
            STACK_SHRINK(1);
            PyObject *value = POP();
            PyObject *old_value = *(PyObject **)addr;
            *(PyObject **)addr = value;
            Py_XDECREF(old_value);
            Py_DECREF(owner);
            DISPATCH();
        }

        TARGET(COMPARE_OP) {
            assert(oparg <= Py_GE);
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyObject_RichCompare(left, right, oparg);
            SET_TOP(res);
            Py_DECREF(left);
            Py_DECREF(right);
            if (res == NULL)
                goto error;
            PREDICT(POP_JUMP_IF_FALSE);
            PREDICT(POP_JUMP_IF_TRUE);
            DISPATCH();
        }

        TARGET(IS_OP) {
            PyObject *right = POP();
            PyObject *left = TOP();
            int res = Py_Is(left, right) ^ oparg;
            PyObject *b = res ? Py_True : Py_False;
            Py_INCREF(b);
            SET_TOP(b);
            Py_DECREF(left);
            Py_DECREF(right);
            PREDICT(POP_JUMP_IF_FALSE);
            PREDICT(POP_JUMP_IF_TRUE);
            DISPATCH();
        }

        TARGET(CONTAINS_OP) {
            PyObject *right = POP();
            PyObject *left = POP();
            int res = PySequence_Contains(right, left);
            Py_DECREF(left);
            Py_DECREF(right);
            if (res < 0) {
                goto error;
            }
            PyObject *b = (res^oparg) ? Py_True : Py_False;
            Py_INCREF(b);
            PUSH(b);
            PREDICT(POP_JUMP_IF_FALSE);
            PREDICT(POP_JUMP_IF_TRUE);
            DISPATCH();
        }

        TARGET(JUMP_IF_NOT_EXC_MATCH) {
            PyObject *right = POP();
            PyObject *left = TOP();
            if (check_except_type_valid(tstate, right) < 0) {
                 Py_DECREF(right);
                 goto error;
            }
            int res = PyErr_GivenExceptionMatches(left, right);
            Py_DECREF(right);
            if (res > 0) {
                /* Exception matches -- Do nothing */;
            }
            else if (res == 0) {
                JUMPTO(oparg);
            }
            else {
                goto error;
            }
            DISPATCH();
        }

        TARGET(IMPORT_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *fromlist = POP();
            PyObject *level = TOP();
            PyObject *res;
            res = import_name(tstate, frame, name, fromlist, level);
            Py_DECREF(level);
            Py_DECREF(fromlist);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(IMPORT_STAR) {
            PyObject *from = POP(), *locals;
            int err;
            if (_PyFrame_FastToLocalsWithError(frame) < 0) {
                Py_DECREF(from);
                goto error;
            }

            locals = LOCALS();
            if (locals == NULL) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "no locals found during 'import *'");
                Py_DECREF(from);
                goto error;
            }
            err = import_all_from(tstate, locals, from);
            _PyFrame_LocalsToFast(frame, 0);
            Py_DECREF(from);
            if (err != 0)
                goto error;
            DISPATCH();
        }

        TARGET(IMPORT_FROM) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *from = TOP();
            PyObject *res;
            res = import_from(tstate, from, name);
            PUSH(res);
            if (res == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(JUMP_FORWARD) {
            JUMPBY(oparg);
            DISPATCH();
        }

        TARGET(POP_JUMP_IF_FALSE) {
            PREDICTED(POP_JUMP_IF_FALSE);
            PyObject *cond = POP();
            int err;
            if (Py_IsTrue(cond)) {
                Py_DECREF(cond);
                DISPATCH();
            }
            if (Py_IsFalse(cond)) {
                Py_DECREF(cond);
                JUMPTO(oparg);
                CHECK_EVAL_BREAKER();
                DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            Py_DECREF(cond);
            if (err > 0)
                ;
            else if (err == 0) {
                JUMPTO(oparg);
                CHECK_EVAL_BREAKER();
            }
            else
                goto error;
            DISPATCH();
        }

        TARGET(POP_JUMP_IF_TRUE) {
            PREDICTED(POP_JUMP_IF_TRUE);
            PyObject *cond = POP();
            int err;
            if (Py_IsFalse(cond)) {
                Py_DECREF(cond);
                DISPATCH();
            }
            if (Py_IsTrue(cond)) {
                Py_DECREF(cond);
                JUMPTO(oparg);
                CHECK_EVAL_BREAKER();
                DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            Py_DECREF(cond);
            if (err > 0) {
                JUMPTO(oparg);
                CHECK_EVAL_BREAKER();
            }
            else if (err == 0)
                ;
            else
                goto error;
            DISPATCH();
        }

        TARGET(JUMP_IF_FALSE_OR_POP) {
            PyObject *cond = TOP();
            int err;
            if (Py_IsTrue(cond)) {
                STACK_SHRINK(1);
                Py_DECREF(cond);
                DISPATCH();
            }
            if (Py_IsFalse(cond)) {
                JUMPTO(oparg);
                DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            if (err > 0) {
                STACK_SHRINK(1);
                Py_DECREF(cond);
            }
            else if (err == 0)
                JUMPTO(oparg);
            else
                goto error;
            DISPATCH();
        }

        TARGET(JUMP_IF_TRUE_OR_POP) {
            PyObject *cond = TOP();
            int err;
            if (Py_IsFalse(cond)) {
                STACK_SHRINK(1);
                Py_DECREF(cond);
                DISPATCH();
            }
            if (Py_IsTrue(cond)) {
                JUMPTO(oparg);
                DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            if (err > 0) {
                JUMPTO(oparg);
            }
            else if (err == 0) {
                STACK_SHRINK(1);
                Py_DECREF(cond);
            }
            else
                goto error;
            DISPATCH();
        }

        TARGET(JUMP_ABSOLUTE) {
            PREDICTED(JUMP_ABSOLUTE);
            assert(oparg < INSTR_OFFSET());
            /* Increment the warmup counter and quicken if warm enough
            * _Py_Quicken is idempotent so we don't worry about overflow */
            if (!PyCodeObject_IsWarmedUp(co)) {
                PyCodeObject_IncrementWarmup(co);
                if (PyCodeObject_IsWarmedUp(co)) {
                    if (_Py_Quicken(co)) {
                        goto error;
                    }
                    int nexti = INSTR_OFFSET();
                    first_instr = co->co_firstinstr;
                    next_instr = first_instr + nexti;
                }
            }
            JUMPTO(oparg);
            CHECK_EVAL_BREAKER();
            DISPATCH();
        }

        TARGET(JUMP_ABSOLUTE_QUICK) {
            assert(oparg < INSTR_OFFSET());
            JUMPTO(oparg);
            CHECK_EVAL_BREAKER();
            DISPATCH();
        }

        TARGET(GET_LEN) {
            // PUSH(len(TOS))
            Py_ssize_t len_i = PyObject_Length(TOP());
            if (len_i < 0) {
                goto error;
            }
            PyObject *len_o = PyLong_FromSsize_t(len_i);
            if (len_o == NULL) {
                goto error;
            }
            PUSH(len_o);
            DISPATCH();
        }

        TARGET(MATCH_CLASS) {
            // Pop TOS and TOS1. Set TOS to a tuple of attributes on success, or
            // None on failure.
            PyObject *names = POP();
            PyObject *type = POP();
            PyObject *subject = TOP();
            assert(PyTuple_CheckExact(names));
            PyObject *attrs = match_class(tstate, subject, type, oparg, names);
            Py_DECREF(names);
            Py_DECREF(type);
            if (attrs) {
                // Success!
                assert(PyTuple_CheckExact(attrs));
                SET_TOP(attrs);
            }
            else if (_PyErr_Occurred(tstate)) {
                // Error!
                goto error;
            }
            else {
                // Failure!
                Py_INCREF(Py_None);
                SET_TOP(Py_None);
            }
            Py_DECREF(subject);
            DISPATCH();
        }

        TARGET(MATCH_MAPPING) {
            PyObject *subject = TOP();
            int match = Py_TYPE(subject)->tp_flags & Py_TPFLAGS_MAPPING;
            PyObject *res = match ? Py_True : Py_False;
            Py_INCREF(res);
            PUSH(res);
            PREDICT(POP_JUMP_IF_FALSE);
            DISPATCH();
        }

        TARGET(MATCH_SEQUENCE) {
            PyObject *subject = TOP();
            int match = Py_TYPE(subject)->tp_flags & Py_TPFLAGS_SEQUENCE;
            PyObject *res = match ? Py_True : Py_False;
            Py_INCREF(res);
            PUSH(res);
            PREDICT(POP_JUMP_IF_FALSE);
            DISPATCH();
        }

        TARGET(MATCH_KEYS) {
            // On successful match, PUSH(values). Otherwise, PUSH(None).
            PyObject *keys = TOP();
            PyObject *subject = SECOND();
            PyObject *values_or_none = match_keys(tstate, subject, keys);
            if (values_or_none == NULL) {
                goto error;
            }
            PUSH(values_or_none);
            DISPATCH();
        }

        TARGET(GET_ITER) {
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable = TOP();
            PyObject *iter = PyObject_GetIter(iterable);
            Py_DECREF(iterable);
            SET_TOP(iter);
            if (iter == NULL)
                goto error;
            PREDICT(FOR_ITER);
            PREDICT(CALL_FUNCTION);
            DISPATCH();
        }

        TARGET(GET_YIELD_FROM_ITER) {
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable = TOP();
            PyObject *iter;
            if (PyCoro_CheckExact(iterable)) {
                /* `iterable` is a coroutine */
                if (!(co->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))) {
                    /* and it is used in a 'yield from' expression of a
                       regular generator. */
                    Py_DECREF(iterable);
                    SET_TOP(NULL);
                    _PyErr_SetString(tstate, PyExc_TypeError,
                                     "cannot 'yield from' a coroutine object "
                                     "in a non-coroutine generator");
                    goto error;
                }
            }
            else if (!PyGen_CheckExact(iterable)) {
                /* `iterable` is not a generator. */
                iter = PyObject_GetIter(iterable);
                Py_DECREF(iterable);
                SET_TOP(iter);
                if (iter == NULL)
                    goto error;
            }
            PREDICT(LOAD_CONST);
            DISPATCH();
        }

        TARGET(FOR_ITER) {
            PREDICTED(FOR_ITER);
            /* before: [iter]; after: [iter, iter()] *or* [] */
            PyObject *iter = TOP();
            PyObject *next = (*Py_TYPE(iter)->tp_iternext)(iter);
            if (next != NULL) {
                PUSH(next);
                PREDICT(STORE_FAST);
                PREDICT(UNPACK_SEQUENCE);
                DISPATCH();
            }
            if (_PyErr_Occurred(tstate)) {
                if (!_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
                    goto error;
                }
                else if (tstate->c_tracefunc != NULL) {
                    call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, frame);
                }
                _PyErr_Clear(tstate);
            }
            /* iterator ended normally */
            STACK_SHRINK(1);
            Py_DECREF(iter);
            JUMPBY(oparg);
            DISPATCH();
        }

        TARGET(BEFORE_ASYNC_WITH) {
            _Py_IDENTIFIER(__aenter__);
            _Py_IDENTIFIER(__aexit__);
            PyObject *mgr = TOP();
            PyObject *res;
            PyObject *enter = _PyObject_LookupSpecial(mgr, &PyId___aenter__);
            if (enter == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "asynchronous context manager protocol",
                                  Py_TYPE(mgr)->tp_name);
                }
                goto error;
            }
            PyObject *exit = _PyObject_LookupSpecial(mgr, &PyId___aexit__);
            if (exit == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "asynchronous context manager protocol "
                                  "(missed __aexit__ method)",
                                  Py_TYPE(mgr)->tp_name);
                }
                Py_DECREF(enter);
                goto error;
            }
            SET_TOP(exit);
            Py_DECREF(mgr);
            res = _PyObject_CallNoArgs(enter);
            Py_DECREF(enter);
            if (res == NULL)
                goto error;
            PUSH(res);
            PREDICT(GET_AWAITABLE);
            DISPATCH();
        }

        TARGET(BEFORE_WITH) {
            _Py_IDENTIFIER(__enter__);
            _Py_IDENTIFIER(__exit__);
            PyObject *mgr = TOP();
            PyObject *res;
            PyObject *enter = _PyObject_LookupSpecial(mgr, &PyId___enter__);
            if (enter == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "context manager protocol",
                                  Py_TYPE(mgr)->tp_name);
                }
                goto error;
            }
            PyObject *exit = _PyObject_LookupSpecial(mgr, &PyId___exit__);
            if (exit == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "context manager protocol "
                                  "(missed __exit__ method)",
                                  Py_TYPE(mgr)->tp_name);
                }
                Py_DECREF(enter);
                goto error;
            }
            SET_TOP(exit);
            Py_DECREF(mgr);
            res = _PyObject_CallNoArgs(enter);
            Py_DECREF(enter);
            if (res == NULL) {
                goto error;
            }
            PUSH(res);
            DISPATCH();
        }

        TARGET(WITH_EXCEPT_START) {
            /* At the top of the stack are 8 values:
               - (TOP, SECOND, THIRD) = exc_info()
               - (FOURTH, FIFTH, SIXTH) = previous exception
               - SEVENTH: lasti of exception in exc_info()
               - EIGHTH: the context.__exit__ bound method
               We call EIGHTH(TOP, SECOND, THIRD).
               Then we push again the TOP exception and the __exit__
               return value.
            */
            PyObject *exit_func;
            PyObject *exc, *val, *tb, *res;

            exc = TOP();
            val = SECOND();
            tb = THIRD();
            ASSERT_EXC_TYPE_IS_REDUNDANT(exc, val);
            assert(!Py_IsNone(exc));
            assert(!PyLong_Check(exc));
            assert(PyLong_Check(PEEK(7)));
            exit_func = PEEK(8);
            PyObject *stack[4] = {NULL, exc, val, tb};
            res = PyObject_Vectorcall(exit_func, stack + 1,
                    3 | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
            if (res == NULL)
                goto error;

            PUSH(res);
            DISPATCH();
        }

        TARGET(PUSH_EXC_INFO) {
            PyObject *type = TOP();
            PyObject *value = SECOND();
            PyObject *tb = THIRD();
            ASSERT_EXC_TYPE_IS_REDUNDANT(type, value);
            _PyErr_StackItem *exc_info = tstate->exc_info;
            SET_THIRD(exc_info->exc_traceback);
            SET_SECOND(exc_info->exc_value);
            if (exc_info->exc_type != NULL) {
                SET_TOP(exc_info->exc_type);
            }
            else {
                Py_INCREF(Py_None);
                SET_TOP(Py_None);
            }
            Py_INCREF(tb);
            PUSH(tb);
            exc_info->exc_traceback = tb;

            Py_INCREF(value);
            PUSH(value);
            assert(PyExceptionInstance_Check(value));
            exc_info->exc_value = value;

            Py_INCREF(type);
            PUSH(type);
            assert(PyExceptionClass_Check(type));
            exc_info->exc_type = type;

            DISPATCH();
        }

        TARGET(LOAD_METHOD) {
            PREDICTED(LOAD_METHOD);
            STAT_INC(LOAD_METHOD, unquickened);
            /* Designed to work in tandem with CALL_METHOD. */
            PyObject *name = GETITEM(names, oparg);
            PyObject *obj = TOP();
            PyObject *meth = NULL;

            int meth_found = _PyObject_GetMethod(obj, name, &meth);

            if (meth == NULL) {
                /* Most likely attribute wasn't found. */
                goto error;
            }

            if (meth_found) {
                /* We can bypass temporary bound method object.
                   meth is unbound method and obj is self.

                   meth | self | arg1 | ... | argN
                 */
                SET_TOP(meth);
                PUSH(obj);  // self
            }
            else {
                /* meth is not an unbound method (but a regular attr, or
                   something was returned by a descriptor protocol).  Set
                   the second element of the stack to NULL, to signal
                   CALL_METHOD that it's not a method call.

                   NULL | meth | arg1 | ... | argN
                */
                SET_TOP(NULL);
                Py_DECREF(obj);
                PUSH(meth);
            }
            DISPATCH();
        }

        TARGET(LOAD_METHOD_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            SpecializedCacheEntry *cache = GET_CACHE();
            if (cache->adaptive.counter == 0) {
                PyObject *owner = TOP();
                PyObject *name = GETITEM(names, cache->adaptive.original_oparg);
                next_instr--;
                if (_Py_Specialize_LoadMethod(owner, next_instr, name, cache) < 0) {
                    goto error;
                }
                DISPATCH();
            }
            else {
                STAT_INC(LOAD_METHOD, deferred);
                cache->adaptive.counter--;
                oparg = cache->adaptive.original_oparg;
                STAT_DEC(LOAD_METHOD, unquickened);
                JUMP_TO_INSTRUCTION(LOAD_METHOD);
            }
        }

        TARGET(LOAD_METHOD_CACHED) {
            /* LOAD_METHOD, with cached method object */
            assert(cframe.use_tracing == 0);
            PyObject *self = TOP();
            PyTypeObject *self_cls = Py_TYPE(self);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAttrCache *cache1 = &caches[-1].attr;
            _PyObjectCache *cache2 = &caches[-2].obj;

            DEOPT_IF(self_cls->tp_version_tag != cache1->tp_version, LOAD_METHOD);
            assert(self_cls->tp_dictoffset > 0);
            assert(self_cls->tp_inline_values_offset > 0);
            PyDictObject *dict = *(PyDictObject **)(((char *)self) + self_cls->tp_dictoffset);
            DEOPT_IF(dict != NULL, LOAD_METHOD);
            DEOPT_IF(((PyHeapTypeObject *)self_cls)->ht_cached_keys->dk_version != cache1->dk_version_or_hint, LOAD_METHOD);
            STAT_INC(LOAD_METHOD, hit);
            PyObject *res = cache2->obj;
            assert(res != NULL);
            assert(_PyType_HasFeature(Py_TYPE(res), Py_TPFLAGS_METHOD_DESCRIPTOR));
            Py_INCREF(res);
            SET_TOP(res);
            PUSH(self);
            DISPATCH();
        }

        TARGET(LOAD_METHOD_NO_DICT) {
            PyObject *self = TOP();
            PyTypeObject *self_cls = Py_TYPE(self);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAttrCache *cache1 = &caches[-1].attr;
            _PyObjectCache *cache2 = &caches[-2].obj;
            DEOPT_IF(self_cls->tp_version_tag != cache1->tp_version, LOAD_METHOD);
            assert(self_cls->tp_dictoffset == 0);
            STAT_INC(LOAD_METHOD, hit);
            PyObject *res = cache2->obj;
            assert(res != NULL);
            assert(_PyType_HasFeature(Py_TYPE(res), Py_TPFLAGS_METHOD_DESCRIPTOR));
            Py_INCREF(res);
            SET_TOP(res);
            PUSH(self);
            DISPATCH();
        }

        TARGET(LOAD_METHOD_MODULE) {
            /* LOAD_METHOD, for module methods */
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            LOAD_MODULE_ATTR_OR_METHOD(METHOD);
            SET_TOP(NULL);
            Py_DECREF(owner);
            PUSH(res);
            DISPATCH();
        }

        TARGET(LOAD_METHOD_CLASS) {
            /* LOAD_METHOD, for class methods */
            assert(cframe.use_tracing == 0);
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAttrCache *cache1 = &caches[-1].attr;
            _PyObjectCache *cache2 = &caches[-2].obj;

            PyObject *cls = TOP();
            DEOPT_IF(!PyType_Check(cls), LOAD_METHOD);
            DEOPT_IF(((PyTypeObject *)cls)->tp_version_tag != cache1->tp_version,
                LOAD_METHOD);
            assert(cache1->tp_version != 0);

            STAT_INC(LOAD_METHOD, hit);
            PyObject *res = cache2->obj;
            assert(res != NULL);
            Py_INCREF(res);
            SET_TOP(NULL);
            Py_DECREF(cls);
            PUSH(res);
            DISPATCH();
        }

        TARGET(CALL_METHOD) {
            /* Designed to work in tamdem with LOAD_METHOD. */
            /* `meth` is NULL when LOAD_METHOD thinks that it's not
                a method call.

                Stack layout:

                       ... | NULL | callable | arg1 | ... | argN
                                                            ^- TOP()
                                               ^- (-oparg)
                                    ^- (-oparg-1)
                             ^- (-oparg-2)

                `callable` will be POPed by call_function.
                NULL will will be POPed manually later.
                If `meth` isn't NULL, it's a method call.  Stack layout:

                     ... | method | self | arg1 | ... | argN
                                                        ^- TOP()
                                           ^- (-oparg)
                                    ^- (-oparg-1)
                           ^- (-oparg-2)

               `self` and `method` will be POPed by call_function.
               We'll be passing `oparg + 1` to call_function, to
               make it accept the `self` as a first argument.
            */
            int is_method = (PEEK(oparg + 2) != NULL);
            oparg += is_method;
            nargs = oparg;
            kwnames = NULL;
            postcall_shrink = 2-is_method;
            goto call_function;
        }

        TARGET(CALL_METHOD_KW) {
            /* Designed to work in tandem with LOAD_METHOD. Same as CALL_METHOD
            but pops TOS to get a tuple of keyword names. */
            kwnames = POP();
            int is_method = (PEEK(oparg + 2) != NULL);
            oparg += is_method;
            nargs = oparg - (int)PyTuple_GET_SIZE(kwnames);
            postcall_shrink = 2-is_method;
            goto call_function;
        }

        TARGET(CALL_FUNCTION_KW) {
            kwnames = POP();
            nargs = oparg - (int)PyTuple_GET_SIZE(kwnames);
            postcall_shrink = 1;
            goto call_function;
        }

        TARGET(CALL_FUNCTION) {
            PREDICTED(CALL_FUNCTION);
            STAT_INC(CALL_FUNCTION, unquickened);
            PyObject *function;
            nargs = oparg;
            kwnames = NULL;
            postcall_shrink = 1;
        call_function:
            function = PEEK(oparg + 1);
            if (Py_TYPE(function) == &PyMethod_Type) {
                PyObject *meth = ((PyMethodObject *)function)->im_func;
                PyObject *self = ((PyMethodObject *)function)->im_self;
                Py_INCREF(meth);
                Py_INCREF(self);
                PEEK(oparg + 1) = self;
                Py_DECREF(function);
                function = meth;
                oparg++;
                nargs++;
                assert(postcall_shrink >= 1);
                postcall_shrink--;
            }
            // Check if the call can be inlined or not
            if (Py_TYPE(function) == &PyFunction_Type && tstate->interp->eval_frame == NULL) {
                int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(function))->co_flags;
                int is_generator = code_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR);
                if (!is_generator) {
                    PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : PyFunction_GET_GLOBALS(function);
                    STACK_SHRINK(oparg);
                    InterpreterFrame *new_frame = _PyEvalFramePushAndInit(
                        tstate, (PyFunctionObject *)function, locals,
                        stack_pointer, nargs, kwnames
                    );
                    STACK_SHRINK(postcall_shrink);
                    // The frame has stolen all the arguments from the stack,
                    // so there is no need to clean them up.
                    Py_XDECREF(kwnames);
                    Py_DECREF(function);
                    if (new_frame == NULL) {
                        goto error;
                    }
                    _PyFrame_SetStackPointer(frame, stack_pointer);
                    new_frame->previous = frame;
                    cframe.current_frame = frame = new_frame;
                    new_frame->depth = frame->depth + 1;
                    goto start_frame;
                }
            }
            /* Callable is not a normal Python function */
            PyObject *res;
            if (cframe.use_tracing) {
                res = trace_call_function(tstate, function, stack_pointer-oparg, nargs, kwnames);
            }
            else {
                res = PyObject_Vectorcall(function, stack_pointer-oparg,
                                          nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
            }
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            Py_DECREF(function);
            Py_XDECREF(kwnames);
            /* Clear the stack */
            STACK_SHRINK(oparg);
            for (int i = 0; i < oparg; i++) {
                Py_DECREF(stack_pointer[i]);
            }
            STACK_SHRINK(postcall_shrink);
            PUSH(res);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
            DISPATCH();
        }

        TARGET(CALL_FUNCTION_ADAPTIVE) {
            SpecializedCacheEntry *cache = GET_CACHE();
            nargs = cache->adaptive.original_oparg;
            if (cache->adaptive.counter == 0) {
                next_instr--;
                if (_Py_Specialize_CallFunction(
                    PEEK(nargs + 1), next_instr, nargs, cache, BUILTINS()) < 0) {
                    goto error;
                }
                DISPATCH();
            }
            else {
                STAT_INC(CALL_FUNCTION, deferred);
                cache->adaptive.counter--;
                oparg = nargs;
                kwnames = NULL;
                postcall_shrink = 1;
                goto call_function;
            }
        }

        TARGET(CALL_FUNCTION_PY_SIMPLE) {
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            int argcount = cache0->original_oparg;
            _PyCallCache *cache1 = &caches[-1].call;
            PyObject *callable = PEEK(argcount+1);
            DEOPT_IF(!PyFunction_Check(callable), CALL_FUNCTION);
            PyFunctionObject *func = (PyFunctionObject *)callable;
            DEOPT_IF(func->func_version != cache1->func_version, CALL_FUNCTION);
            /* PEP 523 */
            DEOPT_IF(tstate->interp->eval_frame != NULL, CALL_FUNCTION);
            STAT_INC(CALL_FUNCTION, hit);
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            size_t size = code->co_nlocalsplus + code->co_stacksize + FRAME_SPECIALS_SIZE;
            InterpreterFrame *new_frame = _PyThreadState_BumpFramePointer(tstate, size);
            if (new_frame == NULL) {
                goto error;
            }
            _PyFrame_InitializeSpecials(new_frame, func,
                                        NULL, code->co_nlocalsplus);
            STACK_SHRINK(argcount);
            for (int i = 0; i < argcount; i++) {
                new_frame->localsplus[i] = stack_pointer[i];
            }
            int deflen = cache1->defaults_len;
            for (int i = 0; i < deflen; i++) {
                PyObject *def = PyTuple_GET_ITEM(func->func_defaults, cache1->defaults_start+i);
                Py_INCREF(def);
                new_frame->localsplus[argcount+i] = def;
            }
            for (int i = argcount+deflen; i < code->co_nlocalsplus; i++) {
                new_frame->localsplus[i] = NULL;
            }
            STACK_SHRINK(1);
            Py_DECREF(func);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            new_frame->previous = frame;
            frame = cframe.current_frame = new_frame;
            new_frame->depth = frame->depth + 1;
            goto start_frame;
        }

        TARGET(CALL_FUNCTION_BUILTIN_O) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_O functions */

            PyObject *callable = SECOND();
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL_FUNCTION);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) != METH_O, CALL_FUNCTION);
            STAT_INC(CALL_FUNCTION, hit);

            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable);
            // This is slower but CPython promises to check all non-vectorcall
            // function calls.
            if (_Py_EnterRecursiveCall(tstate, " while calling a Python object")) {
                goto error;
            }
            PyObject *arg = POP();
            PyObject *res = cfunc(PyCFunction_GET_SELF(callable), arg);
            _Py_LeaveRecursiveCall(tstate);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Clear the stack of the function object. */
            Py_DECREF(arg);
            Py_DECREF(callable);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(CALL_FUNCTION_BUILTIN_FAST) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_FASTCALL functions, without keywords */
            SpecializedCacheEntry *caches = GET_CACHE();
            _PyAdaptiveEntry *cache0 = &caches[0].adaptive;
            int nargs = cache0->original_oparg;
            PyObject **pfunc = &PEEK(nargs + 1);
            PyObject *callable = *pfunc;
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL_FUNCTION);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) != METH_FASTCALL,
                CALL_FUNCTION);
            STAT_INC(CALL_FUNCTION, hit);

            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable);
            /* res = func(self, args, nargs) */
            PyObject *res = ((_PyCFunctionFast)(void(*)(void))cfunc)(
                PyCFunction_GET_SELF(callable),
                &PEEK(nargs),
                nargs);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Clear the stack of the function object. */
            while (stack_pointer > pfunc) {
                PyObject *x = POP();
                Py_DECREF(x);
            }
            PUSH(res);
            if (res == NULL) {
                /* Not deopting because this doesn't mean our optimization was
                   wrong. `res` can be NULL for valid reasons. Eg. getattr(x,
                   'invalid'). In those cases an exception is set, so we must
                   handle it.
                */
                goto error;
            }
            DISPATCH();
        }

        TARGET(CALL_FUNCTION_LEN) {
            assert(cframe.use_tracing == 0);
            /* len(o) */
            SpecializedCacheEntry *caches = GET_CACHE();
            assert(caches[0].adaptive.original_oparg == 1);
            _PyObjectCache *cache1 = &caches[-1].obj;

            PyObject *callable = SECOND();
            DEOPT_IF(callable != cache1->obj, CALL_FUNCTION);
            STAT_INC(CALL_FUNCTION, hit);

            Py_ssize_t len_i = PyObject_Length(TOP());
            if (len_i < 0) {
                goto error;
            }
            PyObject *res = PyLong_FromSsize_t(len_i);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Clear the stack of the function object. */
            Py_DECREF(POP());
            Py_DECREF(callable);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(CALL_FUNCTION_ISINSTANCE) {
            assert(cframe.use_tracing == 0);
            /* isinstance(o, o2) */
            SpecializedCacheEntry *caches = GET_CACHE();
            assert(caches[0].adaptive.original_oparg == 2);
            _PyObjectCache *cache1 = &caches[-1].obj;

            PyObject *callable = THIRD();
            DEOPT_IF(callable != cache1->obj, CALL_FUNCTION);
            STAT_INC(CALL_FUNCTION, hit);

            int retval = PyObject_IsInstance(SECOND(), TOP());
            if (retval < 0) {
                goto error;
            }
            PyObject *res = PyBool_FromLong(retval);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Clear the stack of the function object. */
            Py_DECREF(POP());
            Py_DECREF(POP());
            Py_DECREF(callable);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(CALL_FUNCTION_EX) {
            PREDICTED(CALL_FUNCTION_EX);
            PyObject *func, *callargs, *kwargs = NULL, *result;
            if (oparg & 0x01) {
                kwargs = POP();
                if (!PyDict_CheckExact(kwargs)) {
                    PyObject *d = PyDict_New();
                    if (d == NULL)
                        goto error;
                    if (_PyDict_MergeEx(d, kwargs, 2) < 0) {
                        Py_DECREF(d);
                        format_kwargs_error(tstate, SECOND(), kwargs);
                        Py_DECREF(kwargs);
                        goto error;
                    }
                    Py_DECREF(kwargs);
                    kwargs = d;
                }
                assert(PyDict_CheckExact(kwargs));
            }
            callargs = POP();
            func = TOP();
            if (!PyTuple_CheckExact(callargs)) {
                if (check_args_iterable(tstate, func, callargs) < 0) {
                    Py_DECREF(callargs);
                    goto error;
                }
                Py_SETREF(callargs, PySequence_Tuple(callargs));
                if (callargs == NULL) {
                    goto error;
                }
            }
            assert(PyTuple_CheckExact(callargs));

            result = do_call_core(tstate, func, callargs, kwargs, cframe.use_tracing);
            Py_DECREF(func);
            Py_DECREF(callargs);
            Py_XDECREF(kwargs);

            SET_TOP(result);
            if (result == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
            DISPATCH();
        }

        TARGET(MAKE_FUNCTION) {
            PyObject *codeobj = POP();
            PyFunctionObject *func = (PyFunctionObject *)
                PyFunction_New(codeobj, GLOBALS());

            Py_DECREF(codeobj);
            if (func == NULL) {
                goto error;
            }

            if (oparg & 0x08) {
                assert(PyTuple_CheckExact(TOP()));
                func->func_closure = POP();
            }
            if (oparg & 0x04) {
                assert(PyTuple_CheckExact(TOP()));
                func->func_annotations = POP();
            }
            if (oparg & 0x02) {
                assert(PyDict_CheckExact(TOP()));
                func->func_kwdefaults = POP();
            }
            if (oparg & 0x01) {
                assert(PyTuple_CheckExact(TOP()));
                func->func_defaults = POP();
            }

            PUSH((PyObject *)func);
            DISPATCH();
        }

        TARGET(BUILD_SLICE) {
            PyObject *start, *stop, *step, *slice;
            if (oparg == 3)
                step = POP();
            else
                step = NULL;
            stop = POP();
            start = TOP();
            slice = PySlice_New(start, stop, step);
            Py_DECREF(start);
            Py_DECREF(stop);
            Py_XDECREF(step);
            SET_TOP(slice);
            if (slice == NULL)
                goto error;
            DISPATCH();
        }

        TARGET(FORMAT_VALUE) {
            /* Handles f-string value formatting. */
            PyObject *result;
            PyObject *fmt_spec;
            PyObject *value;
            PyObject *(*conv_fn)(PyObject *);
            int which_conversion = oparg & FVC_MASK;
            int have_fmt_spec = (oparg & FVS_MASK) == FVS_HAVE_SPEC;

            fmt_spec = have_fmt_spec ? POP() : NULL;
            value = POP();

            /* See if any conversion is specified. */
            switch (which_conversion) {
            case FVC_NONE:  conv_fn = NULL;           break;
            case FVC_STR:   conv_fn = PyObject_Str;   break;
            case FVC_REPR:  conv_fn = PyObject_Repr;  break;
            case FVC_ASCII: conv_fn = PyObject_ASCII; break;
            default:
                _PyErr_Format(tstate, PyExc_SystemError,
                              "unexpected conversion flag %d",
                              which_conversion);
                goto error;
            }

            /* If there's a conversion function, call it and replace
               value with that result. Otherwise, just use value,
               without conversion. */
            if (conv_fn != NULL) {
                result = conv_fn(value);
                Py_DECREF(value);
                if (result == NULL) {
                    Py_XDECREF(fmt_spec);
                    goto error;
                }
                value = result;
            }

            /* If value is a unicode object, and there's no fmt_spec,
               then we know the result of format(value) is value
               itself. In that case, skip calling format(). I plan to
               move this optimization in to PyObject_Format()
               itself. */
            if (PyUnicode_CheckExact(value) && fmt_spec == NULL) {
                /* Do nothing, just transfer ownership to result. */
                result = value;
            } else {
                /* Actually call format(). */
                result = PyObject_Format(value, fmt_spec);
                Py_DECREF(value);
                Py_XDECREF(fmt_spec);
                if (result == NULL) {
                    goto error;
                }
            }

            PUSH(result);
            DISPATCH();
        }

        TARGET(ROT_N) {
            PyObject *top = TOP();
            memmove(&PEEK(oparg - 1), &PEEK(oparg),
                    sizeof(PyObject*) * (oparg - 1));
            PEEK(oparg) = top;
            DISPATCH();
        }

        TARGET(COPY) {
            assert(oparg != 0);
            PyObject *peek = PEEK(oparg);
            Py_INCREF(peek);
            PUSH(peek);
            DISPATCH();
        }

        TARGET(BINARY_OP) {
            PREDICTED(BINARY_OP);
            STAT_INC(BINARY_OP, unquickened);
            PyObject *rhs = POP();
            PyObject *lhs = TOP();
            assert(0 <= oparg);
            assert((unsigned)oparg < Py_ARRAY_LENGTH(binary_ops));
            assert(binary_ops[oparg]);
            PyObject *res = binary_ops[oparg](lhs, rhs);
            Py_DECREF(lhs);
            Py_DECREF(rhs);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            DISPATCH();
        }

        TARGET(BINARY_OP_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            SpecializedCacheEntry *cache = GET_CACHE();
            if (cache->adaptive.counter == 0) {
                PyObject *lhs = SECOND();
                PyObject *rhs = TOP();
                next_instr--;
                _Py_Specialize_BinaryOp(lhs, rhs, next_instr, cache);
                DISPATCH();
            }
            else {
                STAT_INC(BINARY_OP, deferred);
                cache->adaptive.counter--;
                oparg = cache->adaptive.original_oparg;
                STAT_DEC(BINARY_OP, unquickened);
                JUMP_TO_INSTRUCTION(BINARY_OP);
            }
        }

        TARGET(EXTENDED_ARG) {
            int oldoparg = oparg;
            NEXTOPARG();
            oparg |= oldoparg << 8;
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }

#if USE_COMPUTED_GOTOS
        TARGET_DO_TRACING: {
#else
        case DO_TRACING: {
#endif
            int instr_prev = skip_backwards_over_extended_args(co, frame->f_lasti);
            frame->f_lasti = INSTR_OFFSET();
            TRACING_NEXTOPARG();
            if (PyDTrace_LINE_ENABLED()) {
                maybe_dtrace_line(frame, &tstate->trace_info, instr_prev);
            }
            /* line-by-line tracing support */

            if (cframe.use_tracing &&
                tstate->c_tracefunc != NULL && !tstate->tracing) {
                int err;
                /* see maybe_call_line_trace()
                for expository comments */
                _PyFrame_SetStackPointer(frame, stack_pointer);

                err = maybe_call_line_trace(tstate->c_tracefunc,
                                            tstate->c_traceobj,
                                            tstate, frame, instr_prev);
                if (err) {
                    /* trace function raised an exception */
                    next_instr++;
                    goto error;
                }
                /* Reload possibly changed frame fields */
                JUMPTO(frame->f_lasti);

                stack_pointer = _PyFrame_GetStackPointer(frame);
                frame->stacktop = -1;
                TRACING_NEXTOPARG();
            }
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }


#if USE_COMPUTED_GOTOS
        _unknown_opcode:
#else
        default:
#endif
            fprintf(stderr,
                "XXX lineno: %d, opcode: %d\n",
                PyCode_Addr2Line(frame->f_code, frame->f_lasti*sizeof(_Py_CODEUNIT)),
                opcode);
            _PyErr_SetString(tstate, PyExc_SystemError, "unknown opcode");
            goto error;

        } /* End instructions */

        /* This should never be reached. Every opcode should end with DISPATCH()
           or goto error. */
        Py_UNREACHABLE();

/* Specialization misses */

#define MISS_WITH_CACHE(opname) \
opname ## _miss: \
    { \
        STAT_INC(opname, miss); \
        _PyAdaptiveEntry *cache = &GET_CACHE()->adaptive; \
        cache->counter--; \
        if (cache->counter == 0) { \
            next_instr[-1] = _Py_MAKECODEUNIT(opname ## _ADAPTIVE, _Py_OPARG(next_instr[-1])); \
            STAT_INC(opname, deopt); \
            cache_backoff(cache); \
        } \
        oparg = cache->original_oparg; \
        STAT_DEC(opname, unquickened); \
        JUMP_TO_INSTRUCTION(opname); \
    }

#define MISS_WITH_OPARG_COUNTER(opname) \
opname ## _miss: \
    { \
        STAT_INC(opname, miss); \
        uint8_t oparg = _Py_OPARG(next_instr[-1])-1; \
        UPDATE_PREV_INSTR_OPARG(next_instr, oparg); \
        assert(_Py_OPARG(next_instr[-1]) == oparg); \
        if (oparg == 0) /* too many cache misses */ { \
            oparg = ADAPTIVE_CACHE_BACKOFF; \
            next_instr[-1] = _Py_MAKECODEUNIT(opname ## _ADAPTIVE, oparg); \
            STAT_INC(opname, deopt); \
        } \
        STAT_DEC(opname, unquickened); \
        JUMP_TO_INSTRUCTION(opname); \
    }

MISS_WITH_CACHE(LOAD_ATTR)
MISS_WITH_CACHE(STORE_ATTR)
MISS_WITH_CACHE(LOAD_GLOBAL)
MISS_WITH_CACHE(LOAD_METHOD)
MISS_WITH_CACHE(CALL_FUNCTION)
MISS_WITH_CACHE(BINARY_OP)
MISS_WITH_CACHE(BINARY_SUBSCR)
MISS_WITH_OPARG_COUNTER(STORE_SUBSCR)

binary_subscr_dict_error:
        {
            PyObject *sub = POP();
            if (!_PyErr_Occurred(tstate)) {
                _PyErr_SetKeyError(sub);
            }
            Py_DECREF(sub);
            goto error;
        }

unbound_local_error:
        {
            format_exc_check_arg(tstate, PyExc_UnboundLocalError,
                UNBOUNDLOCAL_ERROR_MSG,
                PyTuple_GetItem(co->co_localsplusnames, oparg)
            );
            goto error;
        }

error:
        /* Double-check exception status. */
#ifdef NDEBUG
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_SystemError,
                             "error return without exception set");
        }
#else
        assert(_PyErr_Occurred(tstate));
#endif

        /* Log traceback info. */
        PyFrameObject *f = _PyFrame_GetFrameObject(frame);
        if (f != NULL) {
            PyTraceBack_Here(f);
        }

        if (tstate->c_tracefunc != NULL) {
            /* Make sure state is set to FRAME_EXECUTING for tracing */
            assert(frame->f_state == FRAME_EXECUTING);
            frame->f_state = FRAME_UNWINDING;
            call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj,
                           tstate, frame);
        }

exception_unwind:
        frame->f_state = FRAME_UNWINDING;
        /* We can't use frame->f_lasti here, as RERAISE may have set it */
        int offset = INSTR_OFFSET()-1;
        int level, handler, lasti;
        if (get_exception_handler(co, offset, &level, &handler, &lasti) == 0) {
            // No handlers, so exit.
            assert(retval == NULL);
            assert(_PyErr_Occurred(tstate));

            /* Pop remaining stack entries. */
            PyObject **stackbase = _PyFrame_Stackbase(frame);
            while (stack_pointer > stackbase) {
                PyObject *o = POP();
                Py_XDECREF(o);
            }
            assert(STACK_LEVEL() == 0);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            frame->f_state = FRAME_RAISED;
            goto exiting;
        }

        assert(STACK_LEVEL() >= level);
        PyObject **new_top = _PyFrame_Stackbase(frame) + level;
        while (stack_pointer > new_top) {
            PyObject *v = POP();
            Py_XDECREF(v);
        }
        PyObject *exc, *val, *tb;
        if (lasti) {
            PyObject *lasti = PyLong_FromLong(frame->f_lasti);
            if (lasti == NULL) {
                goto exception_unwind;
            }
            PUSH(lasti);
        }
        _PyErr_Fetch(tstate, &exc, &val, &tb);
        /* Make the raw exception data
            available to the handler,
            so a program can emulate the
            Python main loop. */
        _PyErr_NormalizeException(tstate, &exc, &val, &tb);
        if (tb != NULL)
            PyException_SetTraceback(val, tb);
        else
            PyException_SetTraceback(val, Py_None);
        if (tb == NULL) {
            tb = Py_None;
            Py_INCREF(Py_None);
        }
        PUSH(tb);
        PUSH(val);
        PUSH(exc);
        ASSERT_EXC_TYPE_IS_REDUNDANT(exc, val);
        JUMPTO(handler);
        /* Resume normal execution */
        frame->f_state = FRAME_EXECUTING;
        DISPATCH();
    }

exiting:
    if (cframe.use_tracing) {
        if (tstate->c_tracefunc) {
            if (call_trace_protected(tstate->c_tracefunc, tstate->c_traceobj,
                                     tstate, frame, PyTrace_RETURN, retval)) {
                Py_CLEAR(retval);
            }
        }
        if (tstate->c_profilefunc) {
            if (call_trace_protected(tstate->c_profilefunc, tstate->c_profileobj,
                                     tstate, frame, PyTrace_RETURN, retval)) {
                Py_CLEAR(retval);
            }
        }
    }

    /* pop frame */
exit_eval_frame:
    if (PyDTrace_FUNCTION_RETURN_ENABLED())
        dtrace_function_return(frame);
    _Py_LeaveRecursiveCall(tstate);

    if (frame->depth) {
        cframe.current_frame = frame->previous;
        _PyFrame_StackPush(cframe.current_frame, retval);
        if (_PyEvalFrameClearAndPop(tstate, frame)) {
            retval = NULL;
        }
        frame = cframe.current_frame;
        if (retval == NULL) {
            assert(_PyErr_Occurred(tstate));
            throwflag = 1;
        }
        retval = NULL;
        goto resume_frame;
    }

    /* Restore previous cframe. */
    tstate->cframe = cframe.previous;
    tstate->cframe->use_tracing = cframe.use_tracing;
    assert(tstate->cframe->current_frame == frame->previous);
    return _Py_CheckFunctionResult(tstate, NULL, retval, __func__);
}

static void
format_missing(PyThreadState *tstate, const char *kind,
               PyCodeObject *co, PyObject *names, PyObject *qualname)
{
    int err;
    Py_ssize_t len = PyList_GET_SIZE(names);
    PyObject *name_str, *comma, *tail, *tmp;

    assert(PyList_CheckExact(names));
    assert(len >= 1);
    /* Deal with the joys of natural language. */
    switch (len) {
    case 1:
        name_str = PyList_GET_ITEM(names, 0);
        Py_INCREF(name_str);
        break;
    case 2:
        name_str = PyUnicode_FromFormat("%U and %U",
                                        PyList_GET_ITEM(names, len - 2),
                                        PyList_GET_ITEM(names, len - 1));
        break;
    default:
        tail = PyUnicode_FromFormat(", %U, and %U",
                                    PyList_GET_ITEM(names, len - 2),
                                    PyList_GET_ITEM(names, len - 1));
        if (tail == NULL)
            return;
        /* Chop off the last two objects in the list. This shouldn't actually
           fail, but we can't be too careful. */
        err = PyList_SetSlice(names, len - 2, len, NULL);
        if (err == -1) {
            Py_DECREF(tail);
            return;
        }
        /* Stitch everything up into a nice comma-separated list. */
        comma = PyUnicode_FromString(", ");
        if (comma == NULL) {
            Py_DECREF(tail);
            return;
        }
        tmp = PyUnicode_Join(comma, names);
        Py_DECREF(comma);
        if (tmp == NULL) {
            Py_DECREF(tail);
            return;
        }
        name_str = PyUnicode_Concat(tmp, tail);
        Py_DECREF(tmp);
        Py_DECREF(tail);
        break;
    }
    if (name_str == NULL)
        return;
    _PyErr_Format(tstate, PyExc_TypeError,
                  "%U() missing %i required %s argument%s: %U",
                  qualname,
                  len,
                  kind,
                  len == 1 ? "" : "s",
                  name_str);
    Py_DECREF(name_str);
}

static void
missing_arguments(PyThreadState *tstate, PyCodeObject *co,
                  Py_ssize_t missing, Py_ssize_t defcount,
                  PyObject **localsplus, PyObject *qualname)
{
    Py_ssize_t i, j = 0;
    Py_ssize_t start, end;
    int positional = (defcount != -1);
    const char *kind = positional ? "positional" : "keyword-only";
    PyObject *missing_names;

    /* Compute the names of the arguments that are missing. */
    missing_names = PyList_New(missing);
    if (missing_names == NULL)
        return;
    if (positional) {
        start = 0;
        end = co->co_argcount - defcount;
    }
    else {
        start = co->co_argcount;
        end = start + co->co_kwonlyargcount;
    }
    for (i = start; i < end; i++) {
        if (localsplus[i] == NULL) {
            PyObject *raw = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            PyObject *name = PyObject_Repr(raw);
            if (name == NULL) {
                Py_DECREF(missing_names);
                return;
            }
            PyList_SET_ITEM(missing_names, j++, name);
        }
    }
    assert(j == missing);
    format_missing(tstate, kind, co, missing_names, qualname);
    Py_DECREF(missing_names);
}

static void
too_many_positional(PyThreadState *tstate, PyCodeObject *co,
                    Py_ssize_t given, PyObject *defaults,
                    PyObject **localsplus, PyObject *qualname)
{
    int plural;
    Py_ssize_t kwonly_given = 0;
    Py_ssize_t i;
    PyObject *sig, *kwonly_sig;
    Py_ssize_t co_argcount = co->co_argcount;

    assert((co->co_flags & CO_VARARGS) == 0);
    /* Count missing keyword-only args. */
    for (i = co_argcount; i < co_argcount + co->co_kwonlyargcount; i++) {
        if (localsplus[i] != NULL) {
            kwonly_given++;
        }
    }
    Py_ssize_t defcount = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    if (defcount) {
        Py_ssize_t atleast = co_argcount - defcount;
        plural = 1;
        sig = PyUnicode_FromFormat("from %zd to %zd", atleast, co_argcount);
    }
    else {
        plural = (co_argcount != 1);
        sig = PyUnicode_FromFormat("%zd", co_argcount);
    }
    if (sig == NULL)
        return;
    if (kwonly_given) {
        const char *format = " positional argument%s (and %zd keyword-only argument%s)";
        kwonly_sig = PyUnicode_FromFormat(format,
                                          given != 1 ? "s" : "",
                                          kwonly_given,
                                          kwonly_given != 1 ? "s" : "");
        if (kwonly_sig == NULL) {
            Py_DECREF(sig);
            return;
        }
    }
    else {
        /* This will not fail. */
        kwonly_sig = PyUnicode_FromString("");
        assert(kwonly_sig != NULL);
    }
    _PyErr_Format(tstate, PyExc_TypeError,
                  "%U() takes %U positional argument%s but %zd%U %s given",
                  qualname,
                  sig,
                  plural ? "s" : "",
                  given,
                  kwonly_sig,
                  given == 1 && !kwonly_given ? "was" : "were");
    Py_DECREF(sig);
    Py_DECREF(kwonly_sig);
}

static int
positional_only_passed_as_keyword(PyThreadState *tstate, PyCodeObject *co,
                                  Py_ssize_t kwcount, PyObject* kwnames,
                                  PyObject *qualname)
{
    int posonly_conflicts = 0;
    PyObject* posonly_names = PyList_New(0);

    for(int k=0; k < co->co_posonlyargcount; k++){
        PyObject* posonly_name = PyTuple_GET_ITEM(co->co_localsplusnames, k);

        for (int k2=0; k2<kwcount; k2++){
            /* Compare the pointers first and fallback to PyObject_RichCompareBool*/
            PyObject* kwname = PyTuple_GET_ITEM(kwnames, k2);
            if (kwname == posonly_name){
                if(PyList_Append(posonly_names, kwname) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
                continue;
            }

            int cmp = PyObject_RichCompareBool(posonly_name, kwname, Py_EQ);

            if ( cmp > 0) {
                if(PyList_Append(posonly_names, kwname) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
            } else if (cmp < 0) {
                goto fail;
            }

        }
    }
    if (posonly_conflicts) {
        PyObject* comma = PyUnicode_FromString(", ");
        if (comma == NULL) {
            goto fail;
        }
        PyObject* error_names = PyUnicode_Join(comma, posonly_names);
        Py_DECREF(comma);
        if (error_names == NULL) {
            goto fail;
        }
        _PyErr_Format(tstate, PyExc_TypeError,
                      "%U() got some positional-only arguments passed"
                      " as keyword arguments: '%U'",
                      qualname, error_names);
        Py_DECREF(error_names);
        goto fail;
    }

    Py_DECREF(posonly_names);
    return 0;

fail:
    Py_XDECREF(posonly_names);
    return 1;

}

/* Exception table parsing code.
 * See Objects/exception_table_notes.txt for details.
 */

static inline unsigned char *
parse_varint(unsigned char *p, int *result) {
    int val = p[0] & 63;
    while (p[0] & 64) {
        p++;
        val = (val << 6) | (p[0] & 63);
    }
    *result = val;
    return p+1;
}

static inline unsigned char *
scan_back_to_entry_start(unsigned char *p) {
    for (; (p[0]&128) == 0; p--);
    return p;
}

static inline unsigned char *
skip_to_next_entry(unsigned char *p, unsigned char *end) {
    while (p < end && ((p[0] & 128) == 0)) {
        p++;
    }
    return p;
}


#define MAX_LINEAR_SEARCH 40

static int
get_exception_handler(PyCodeObject *code, int index, int *level, int *handler, int *lasti)
{
    unsigned char *start = (unsigned char *)PyBytes_AS_STRING(code->co_exceptiontable);
    unsigned char *end = start + PyBytes_GET_SIZE(code->co_exceptiontable);
    /* Invariants:
     * start_table == end_table OR
     * start_table points to a legal entry and end_table points
     * beyond the table or to a legal entry that is after index.
     */
    if (end - start > MAX_LINEAR_SEARCH) {
        int offset;
        parse_varint(start, &offset);
        if (offset > index) {
            return 0;
        }
        do {
            unsigned char * mid = start + ((end-start)>>1);
            mid = scan_back_to_entry_start(mid);
            parse_varint(mid, &offset);
            if (offset > index) {
                end = mid;
            }
            else {
                start = mid;
            }

        } while (end - start > MAX_LINEAR_SEARCH);
    }
    unsigned char *scan = start;
    while (scan < end) {
        int start_offset, size;
        scan = parse_varint(scan, &start_offset);
        if (start_offset > index) {
            break;
        }
        scan = parse_varint(scan, &size);
        if (start_offset + size > index) {
            scan = parse_varint(scan, handler);
            int depth_and_lasti;
            parse_varint(scan, &depth_and_lasti);
            *level = depth_and_lasti >> 1;
            *lasti = depth_and_lasti & 1;
            return 1;
        }
        scan = skip_to_next_entry(scan, end);
    }
    return 0;
}

static int
initialize_locals(PyThreadState *tstate, PyFunctionObject *func,
    PyObject **localsplus, PyObject *const *args,
    Py_ssize_t argcount, PyObject *kwnames)
{
    PyCodeObject *co = (PyCodeObject*)func->func_code;
    const Py_ssize_t total_args = co->co_argcount + co->co_kwonlyargcount;

    /* Create a dictionary for keyword parameters (**kwags) */
    PyObject *kwdict;
    Py_ssize_t i;
    if (co->co_flags & CO_VARKEYWORDS) {
        kwdict = PyDict_New();
        if (kwdict == NULL) {
            goto fail_pre_positional;
        }
        i = total_args;
        if (co->co_flags & CO_VARARGS) {
            i++;
        }
        assert(localsplus[i] == NULL);
        localsplus[i] = kwdict;
    }
    else {
        kwdict = NULL;
    }

    /* Copy all positional arguments into local variables */
    Py_ssize_t j, n;
    if (argcount > co->co_argcount) {
        n = co->co_argcount;
    }
    else {
        n = argcount;
    }
    for (j = 0; j < n; j++) {
        PyObject *x = args[j];
        assert(localsplus[j] == NULL);
        localsplus[j] = x;
    }

    /* Pack other positional arguments into the *args argument */
    if (co->co_flags & CO_VARARGS) {
        PyObject *u = NULL;
        u = _PyTuple_FromArraySteal(args + n, argcount - n);
        if (u == NULL) {
            goto fail_post_positional;
        }
        assert(localsplus[total_args] == NULL);
        localsplus[total_args] = u;
    }
    else if (argcount > n) {
        /* Too many postional args. Error is reported later */
        for (j = n; j < argcount; j++) {
            Py_DECREF(args[j]);
        }
    }

    /* Handle keyword arguments */
    if (kwnames != NULL) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (i = 0; i < kwcount; i++) {
            PyObject **co_varnames;
            PyObject *keyword = PyTuple_GET_ITEM(kwnames, i);
            PyObject *value = args[i+argcount];
            Py_ssize_t j;

            if (keyword == NULL || !PyUnicode_Check(keyword)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() keywords must be strings",
                          func->func_qualname);
                goto kw_fail;
            }

            /* Speed hack: do raw pointer compares. As names are
            normally interned this should almost always hit. */
            co_varnames = ((PyTupleObject *)(co->co_localsplusnames))->ob_item;
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = co_varnames[j];
                if (varname == keyword) {
                    goto kw_found;
                }
            }

            /* Slow fallback, just in case */
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = co_varnames[j];
                int cmp = PyObject_RichCompareBool( keyword, varname, Py_EQ);
                if (cmp > 0) {
                    goto kw_found;
                }
                else if (cmp < 0) {
                    goto kw_fail;
                }
            }

            assert(j >= total_args);
            if (kwdict == NULL) {

                if (co->co_posonlyargcount
                    && positional_only_passed_as_keyword(tstate, co,
                                                        kwcount, kwnames,
                                                        func->func_qualname))
                {
                    goto kw_fail;
                }

                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() got an unexpected keyword argument '%S'",
                          func->func_qualname, keyword);
                goto kw_fail;
            }

            if (PyDict_SetItem(kwdict, keyword, value) == -1) {
                goto kw_fail;
            }
            Py_DECREF(value);
            continue;

        kw_fail:
            for (;i < kwcount; i++) {
                PyObject *value = args[i+argcount];
                Py_DECREF(value);
            }
            goto fail_post_args;

        kw_found:
            if (localsplus[j] != NULL) {
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() got multiple values for argument '%S'",
                          func->func_qualname, keyword);
                goto kw_fail;
            }
            localsplus[j] = value;
        }
    }

    /* Check the number of positional arguments */
    if ((argcount > co->co_argcount) && !(co->co_flags & CO_VARARGS)) {
        too_many_positional(tstate, co, argcount, func->func_defaults, localsplus,
                            func->func_qualname);
        goto fail_post_args;
    }

    /* Add missing positional arguments (copy default values from defs) */
    if (argcount < co->co_argcount) {
        Py_ssize_t defcount = func->func_defaults == NULL ? 0 : PyTuple_GET_SIZE(func->func_defaults);
        Py_ssize_t m = co->co_argcount - defcount;
        Py_ssize_t missing = 0;
        for (i = argcount; i < m; i++) {
            if (localsplus[i] == NULL) {
                missing++;
            }
        }
        if (missing) {
            missing_arguments(tstate, co, missing, defcount, localsplus,
                              func->func_qualname);
            goto fail_post_args;
        }
        if (n > m)
            i = n - m;
        else
            i = 0;
        if (defcount) {
            PyObject **defs = &PyTuple_GET_ITEM(func->func_defaults, 0);
            for (; i < defcount; i++) {
                if (localsplus[m+i] == NULL) {
                    PyObject *def = defs[i];
                    Py_INCREF(def);
                    localsplus[m+i] = def;
                }
            }
        }
    }

    /* Add missing keyword arguments (copy default values from kwdefs) */
    if (co->co_kwonlyargcount > 0) {
        Py_ssize_t missing = 0;
        for (i = co->co_argcount; i < total_args; i++) {
            if (localsplus[i] != NULL)
                continue;
            PyObject *varname = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            if (func->func_kwdefaults != NULL) {
                PyObject *def = PyDict_GetItemWithError(func->func_kwdefaults, varname);
                if (def) {
                    Py_INCREF(def);
                    localsplus[i] = def;
                    continue;
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto fail_post_args;
                }
            }
            missing++;
        }
        if (missing) {
            missing_arguments(tstate, co, missing, -1, localsplus,
                              func->func_qualname);
            goto fail_post_args;
        }
    }
    return 0;

fail_pre_positional:
    for (j = 0; j < argcount; j++) {
        Py_DECREF(args[j]);
    }
    /* fall through */
fail_post_positional:
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (j = argcount; j < argcount+kwcount; j++) {
            Py_DECREF(args[j]);
        }
    }
    /* fall through */
fail_post_args:
    return -1;
}

static InterpreterFrame *
make_coro_frame(PyThreadState *tstate,
           PyFunctionObject *func, PyObject *locals,
           PyObject *const *args, Py_ssize_t argcount,
           PyObject *kwnames)
{
    assert(is_tstate_valid(tstate));
    assert(func->func_defaults == NULL || PyTuple_CheckExact(func->func_defaults));
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int size = code->co_nlocalsplus+code->co_stacksize + FRAME_SPECIALS_SIZE;
    InterpreterFrame *frame = (InterpreterFrame *)PyMem_Malloc(sizeof(PyObject *)*size);
    if (frame == NULL) {
        goto fail_no_memory;
    }
    _PyFrame_InitializeSpecials(frame, func, locals, code->co_nlocalsplus);
    for (int i = 0; i < code->co_nlocalsplus; i++) {
        frame->localsplus[i] = NULL;
    }
    assert(frame->frame_obj == NULL);
    if (initialize_locals(tstate, func, frame->localsplus, args, argcount, kwnames)) {
        _PyFrame_Clear(frame, 1);
        return NULL;
    }
    return frame;
fail_no_memory:
    /* Consume the references */
    for (Py_ssize_t i = 0; i < argcount; i++) {
        Py_DECREF(args[i]);
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            Py_DECREF(args[i+argcount]);
        }
    }
    PyErr_NoMemory();
    return NULL;
}

/* Consumes all the references to the args */
static PyObject *
make_coro(PyThreadState *tstate, PyFunctionObject *func,
          PyObject *locals,
          PyObject* const* args, size_t argcount,
          PyObject *kwnames)
{
    assert (((PyCodeObject *)func->func_code)->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR));
    InterpreterFrame *frame = make_coro_frame(tstate, func, locals, args, argcount, kwnames);
    if (frame == NULL) {
        return NULL;
    }
    PyObject *gen = _Py_MakeCoro(func, frame);
    if (gen == NULL) {
        return NULL;
    }
    return gen;
}

/* Consumes all the references to the args */
static InterpreterFrame *
_PyEvalFramePushAndInit(PyThreadState *tstate, PyFunctionObject *func,
                        PyObject *locals, PyObject* const* args,
                        size_t argcount, PyObject *kwnames)
{
    PyCodeObject * code = (PyCodeObject *)func->func_code;
    size_t size = code->co_nlocalsplus + code->co_stacksize + FRAME_SPECIALS_SIZE;
    InterpreterFrame *frame = _PyThreadState_BumpFramePointer(tstate, size);
    if (frame == NULL) {
        goto fail;
    }
    _PyFrame_InitializeSpecials(frame, func, locals, code->co_nlocalsplus);
    PyObject **localsarray = &frame->localsplus[0];
    for (int i = 0; i < code->co_nlocalsplus; i++) {
        localsarray[i] = NULL;
    }
    if (initialize_locals(tstate, func, localsarray, args, argcount, kwnames)) {
        _PyFrame_Clear(frame, 0);
        return NULL;
    }
    return frame;
fail:
    /* Consume the references */
    for (size_t i = 0; i < argcount; i++) {
        Py_DECREF(args[i]);
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            Py_DECREF(args[i+argcount]);
        }
    }
    PyErr_NoMemory();
    return NULL;
}

static int
_PyEvalFrameClearAndPop(PyThreadState *tstate, InterpreterFrame * frame)
{
    --tstate->recursion_remaining;
    assert(frame->frame_obj == NULL || frame->frame_obj->f_own_locals_memory == 0);
    if (_PyFrame_Clear(frame, 0)) {
        ++tstate->recursion_remaining;
        return -1;
    }
    ++tstate->recursion_remaining;
    _PyThreadState_PopFrame(tstate, frame);
    return 0;
}

PyObject *
_PyEval_Vector(PyThreadState *tstate, PyFunctionObject *func,
               PyObject *locals,
               PyObject* const* args, size_t argcount,
               PyObject *kwnames)
{
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    /* _PyEvalFramePushAndInit and make_coro consume
     * all the references to their arguments
     */
    for (size_t i = 0; i < argcount; i++) {
        Py_INCREF(args[i]);
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            Py_INCREF(args[i+argcount]);
        }
    }
    int is_coro = code->co_flags &
        (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR);
    if (is_coro) {
        return make_coro(tstate, func, locals, args, argcount, kwnames);
    }
    InterpreterFrame *frame = _PyEvalFramePushAndInit(
        tstate, func, locals, args, argcount, kwnames);
    if (frame == NULL) {
        return NULL;
    }
    PyObject *retval = _PyEval_EvalFrame(tstate, frame, 0);
    assert(_PyFrame_GetStackPointer(frame) == _PyFrame_Stackbase(frame));
    if (_PyEvalFrameClearAndPop(tstate, frame)) {
        retval = NULL;
    }
    return retval;
}

/* Legacy API */
PyObject *
PyEval_EvalCodeEx(PyObject *_co, PyObject *globals, PyObject *locals,
                  PyObject *const *args, int argcount,
                  PyObject *const *kws, int kwcount,
                  PyObject *const *defs, int defcount,
                  PyObject *kwdefs, PyObject *closure)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *res;
    PyObject *defaults = _PyTuple_FromArray(defs, defcount);
    if (defaults == NULL) {
        return NULL;
    }
    PyObject *builtins = _PyEval_BuiltinsFromGlobals(tstate, globals); // borrowed ref
    if (builtins == NULL) {
        Py_DECREF(defaults);
        return NULL;
    }
    if (locals == NULL) {
        locals = globals;
    }
    PyObject *kwnames;
    PyObject *const *allargs;
    PyObject **newargs;
    if (kwcount == 0) {
        allargs = args;
        kwnames = NULL;
    }
    else {
        kwnames = PyTuple_New(kwcount);
        if (kwnames == NULL) {
            res = NULL;
            goto fail;
        }
        newargs = PyMem_Malloc(sizeof(PyObject *)*(kwcount+argcount));
        if (newargs == NULL) {
            res = NULL;
            Py_DECREF(kwnames);
            goto fail;
        }
        for (int i = 0; i < argcount; i++) {
            newargs[i] = args[i];
        }
        for (int i = 0; i < kwcount; i++) {
            Py_INCREF(kws[2*i]);
            PyTuple_SET_ITEM(kwnames, i, kws[2*i]);
            newargs[argcount+i] = kws[2*i+1];
        }
        allargs = newargs;
    }
    PyObject **kwargs = PyMem_Malloc(sizeof(PyObject *)*kwcount);
    if (kwargs == NULL) {
        res = NULL;
        Py_DECREF(kwnames);
        goto fail;
    }
    for (int i = 0; i < kwcount; i++) {
        Py_INCREF(kws[2*i]);
        PyTuple_SET_ITEM(kwnames, i, kws[2*i]);
        kwargs[i] = kws[2*i+1];
    }
    PyFrameConstructor constr = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = ((PyCodeObject *)_co)->co_name,
        .fc_qualname = ((PyCodeObject *)_co)->co_name,
        .fc_code = _co,
        .fc_defaults = defaults,
        .fc_kwdefaults = kwdefs,
        .fc_closure = closure
    };
    PyFunctionObject *func = _PyFunction_FromConstructor(&constr);
    if (func == NULL) {
        return NULL;
    }
    res = _PyEval_Vector(tstate, func, locals,
                         allargs, argcount,
                         kwnames);
    Py_DECREF(func);
    if (kwcount) {
        Py_DECREF(kwnames);
        PyMem_Free(newargs);
    }
fail:
    Py_DECREF(defaults);
    return res;
}


/* Logic for the raise statement (too complicated for inlining).
   This *consumes* a reference count to each of its arguments. */
static int
do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause)
{
    PyObject *type = NULL, *value = NULL;

    if (exc == NULL) {
        /* Reraise */
        _PyErr_StackItem *exc_info = _PyErr_GetTopmostException(tstate);
        PyObject *tb;
        type = exc_info->exc_type;
        value = exc_info->exc_value;
        tb = exc_info->exc_traceback;
        if (Py_IsNone(type) || type == NULL) {
            _PyErr_SetString(tstate, PyExc_RuntimeError,
                             "No active exception to reraise");
            return 0;
        }
        Py_XINCREF(type);
        Py_XINCREF(value);
        Py_XINCREF(tb);
        _PyErr_Restore(tstate, type, value, tb);
        return 1;
    }

    /* We support the following forms of raise:
       raise
       raise <instance>
       raise <type> */

    if (PyExceptionClass_Check(exc)) {
        type = exc;
        value = _PyObject_CallNoArgs(exc);
        if (value == NULL)
            goto raise_error;
        if (!PyExceptionInstance_Check(value)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "calling %R should have returned an instance of "
                          "BaseException, not %R",
                          type, Py_TYPE(value));
             goto raise_error;
        }
    }
    else if (PyExceptionInstance_Check(exc)) {
        value = exc;
        type = PyExceptionInstance_Class(exc);
        Py_INCREF(type);
    }
    else {
        /* Not something you can raise.  You get an exception
           anyway, just not what you specified :-) */
        Py_DECREF(exc);
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "exceptions must derive from BaseException");
        goto raise_error;
    }

    assert(type != NULL);
    assert(value != NULL);

    if (cause) {
        PyObject *fixed_cause;
        if (PyExceptionClass_Check(cause)) {
            fixed_cause = _PyObject_CallNoArgs(cause);
            if (fixed_cause == NULL)
                goto raise_error;
            Py_DECREF(cause);
        }
        else if (PyExceptionInstance_Check(cause)) {
            fixed_cause = cause;
        }
        else if (Py_IsNone(cause)) {
            Py_DECREF(cause);
            fixed_cause = NULL;
        }
        else {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "exception causes must derive from "
                             "BaseException");
            goto raise_error;
        }
        PyException_SetCause(value, fixed_cause);
    }

    _PyErr_SetObject(tstate, type, value);
    /* _PyErr_SetObject incref's its arguments */
    Py_DECREF(value);
    Py_DECREF(type);
    return 0;

raise_error:
    Py_XDECREF(value);
    Py_XDECREF(type);
    Py_XDECREF(cause);
    return 0;
}

/* Iterate v argcnt times and store the results on the stack (via decreasing
   sp).  Return 1 for success, 0 if error.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target.
*/

static int
unpack_iterable(PyThreadState *tstate, PyObject *v,
                int argcnt, int argcntafter, PyObject **sp)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it;  /* iter(v) */
    PyObject *w;
    PyObject *l = NULL; /* variable list */

    assert(v != NULL);

    it = PyObject_GetIter(v);
    if (it == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
            Py_TYPE(v)->tp_iter == NULL && !PySequence_Check(v))
        {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "cannot unpack non-iterable %.200s object",
                          Py_TYPE(v)->tp_name);
        }
        return 0;
    }

    for (; i < argcnt; i++) {
        w = PyIter_Next(it);
        if (w == NULL) {
            /* Iterator done, via error or exhaustion. */
            if (!_PyErr_Occurred(tstate)) {
                if (argcntafter == -1) {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "not enough values to unpack "
                                  "(expected %d, got %d)",
                                  argcnt, i);
                }
                else {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "not enough values to unpack "
                                  "(expected at least %d, got %d)",
                                  argcnt + argcntafter, i);
                }
            }
            goto Error;
        }
        *--sp = w;
    }

    if (argcntafter == -1) {
        /* We better have exhausted the iterator now. */
        w = PyIter_Next(it);
        if (w == NULL) {
            if (_PyErr_Occurred(tstate))
                goto Error;
            Py_DECREF(it);
            return 1;
        }
        Py_DECREF(w);
        _PyErr_Format(tstate, PyExc_ValueError,
                      "too many values to unpack (expected %d)",
                      argcnt);
        goto Error;
    }

    l = PySequence_List(it);
    if (l == NULL)
        goto Error;
    *--sp = l;
    i++;

    ll = PyList_GET_SIZE(l);
    if (ll < argcntafter) {
        _PyErr_Format(tstate, PyExc_ValueError,
            "not enough values to unpack (expected at least %d, got %zd)",
            argcnt + argcntafter, argcnt + ll);
        goto Error;
    }

    /* Pop the "after-variable" args off the list. */
    for (j = argcntafter; j > 0; j--, i++) {
        *--sp = PyList_GET_ITEM(l, ll - j);
    }
    /* Resize the list. */
    Py_SET_SIZE(l, ll - argcntafter);
    Py_DECREF(it);
    return 1;

Error:
    for (; i > 0; i--, sp++)
        Py_DECREF(*sp);
    Py_XDECREF(it);
    return 0;
}

#ifdef LLTRACE
static int
prtrace(PyThreadState *tstate, PyObject *v, const char *str)
{
    printf("%s ", str);
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    if (PyObject_Print(v, stdout, 0) != 0) {
        /* Don't know what else to do */
        _PyErr_Clear(tstate);
    }
    printf("\n");
    PyErr_Restore(type, value, traceback);
    return 1;
}
#endif

static void
call_exc_trace(Py_tracefunc func, PyObject *self,
               PyThreadState *tstate,
               InterpreterFrame *f)
{
    PyObject *type, *value, *traceback, *orig_traceback, *arg;
    int err;
    _PyErr_Fetch(tstate, &type, &value, &orig_traceback);
    if (value == NULL) {
        value = Py_None;
        Py_INCREF(value);
    }
    _PyErr_NormalizeException(tstate, &type, &value, &orig_traceback);
    traceback = (orig_traceback != NULL) ? orig_traceback : Py_None;
    arg = PyTuple_Pack(3, type, value, traceback);
    if (arg == NULL) {
        _PyErr_Restore(tstate, type, value, orig_traceback);
        return;
    }
    err = call_trace(func, self, tstate, f, PyTrace_EXCEPTION, arg);
    Py_DECREF(arg);
    if (err == 0) {
        _PyErr_Restore(tstate, type, value, orig_traceback);
    }
    else {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(orig_traceback);
    }
}

static int
call_trace_protected(Py_tracefunc func, PyObject *obj,
                     PyThreadState *tstate, InterpreterFrame *frame,
                     int what, PyObject *arg)
{
    PyObject *type, *value, *traceback;
    int err;
    _PyErr_Fetch(tstate, &type, &value, &traceback);
    err = call_trace(func, obj, tstate, frame, what, arg);
    if (err == 0)
    {
        _PyErr_Restore(tstate, type, value, traceback);
        return 0;
    }
    else {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
        return -1;
    }
}

static void
initialize_trace_info(PyTraceInfo *trace_info, InterpreterFrame *frame)
{
    PyCodeObject *code = frame->f_code;
    if (trace_info->code != code) {
        trace_info->code = code;
        _PyCode_InitAddressRange(code, &trace_info->bounds);
    }
}

static int
call_trace(Py_tracefunc func, PyObject *obj,
           PyThreadState *tstate, InterpreterFrame *frame,
           int what, PyObject *arg)
{
    int result;
    if (tstate->tracing)
        return 0;
    tstate->tracing++;
    _PyThreadState_PauseTracing(tstate);
    PyFrameObject *f = _PyFrame_GetFrameObject(frame);
    if (f == NULL) {
        return -1;
    }
    if (frame->f_lasti < 0) {
        f->f_lineno = frame->f_code->co_firstlineno;
    }
    else {
        initialize_trace_info(&tstate->trace_info, frame);
        f->f_lineno = _PyCode_CheckLineNumber(frame->f_lasti*sizeof(_Py_CODEUNIT), &tstate->trace_info.bounds);
    }
    result = func(obj, f, what, arg);
    f->f_lineno = 0;
    _PyThreadState_ResumeTracing(tstate);
    tstate->tracing--;
    return result;
}

PyObject *
_PyEval_CallTracing(PyObject *func, PyObject *args)
{
    PyThreadState *tstate = _PyThreadState_GET();
    int save_tracing = tstate->tracing;
    int save_use_tracing = tstate->cframe->use_tracing;
    PyObject *result;

    tstate->tracing = 0;
    _PyThreadState_ResumeTracing(tstate);
    result = PyObject_Call(func, args, NULL);
    tstate->tracing = save_tracing;
    tstate->cframe->use_tracing = save_use_tracing;
    return result;
}

/* See Objects/lnotab_notes.txt for a description of how tracing works. */
static int
maybe_call_line_trace(Py_tracefunc func, PyObject *obj,
                      PyThreadState *tstate, InterpreterFrame *frame, int instr_prev)
{
    int result = 0;

    /* If the last instruction falls at the start of a line or if it
       represents a jump backwards, update the frame's line number and
       then call the trace function if we're tracing source lines.
    */
    initialize_trace_info(&tstate->trace_info, frame);
    int lastline = _PyCode_CheckLineNumber(instr_prev*sizeof(_Py_CODEUNIT), &tstate->trace_info.bounds);
    int line = _PyCode_CheckLineNumber(frame->f_lasti*sizeof(_Py_CODEUNIT), &tstate->trace_info.bounds);
    PyFrameObject *f = _PyFrame_GetFrameObject(frame);
    if (f == NULL) {
        return -1;
    }
    if (line != -1 && f->f_trace_lines) {
        /* Trace backward edges or if line number has changed */
        if (frame->f_lasti < instr_prev || line != lastline) {
            result = call_trace(func, obj, tstate, frame, PyTrace_LINE, Py_None);
        }
    }
    /* Always emit an opcode event if we're tracing all opcodes. */
    if (f->f_trace_opcodes) {
        result = call_trace(func, obj, tstate, frame, PyTrace_OPCODE, Py_None);
    }
    return result;
}

int
_PyEval_SetProfile(PyThreadState *tstate, Py_tracefunc func, PyObject *arg)
{
    assert(is_tstate_valid(tstate));
    /* The caller must hold the GIL */
    assert(PyGILState_Check());

    /* Call _PySys_Audit() in the context of the current thread state,
       even if tstate is not the current thread state. */
    PyThreadState *current_tstate = _PyThreadState_GET();
    if (_PySys_Audit(current_tstate, "sys.setprofile", NULL) < 0) {
        return -1;
    }

    PyObject *profileobj = tstate->c_profileobj;

    tstate->c_profilefunc = NULL;
    tstate->c_profileobj = NULL;
    /* Must make sure that tracing is not ignored if 'profileobj' is freed */
    _PyThreadState_ResumeTracing(tstate);
    Py_XDECREF(profileobj);

    Py_XINCREF(arg);
    tstate->c_profileobj = arg;
    tstate->c_profilefunc = func;

    /* Flag that tracing or profiling is turned on */
    _PyThreadState_ResumeTracing(tstate);
    return 0;
}

void
PyEval_SetProfile(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyEval_SetProfile(tstate, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        _PyErr_WriteUnraisableMsg("in PyEval_SetProfile", NULL);
    }
}

int
_PyEval_SetTrace(PyThreadState *tstate, Py_tracefunc func, PyObject *arg)
{
    assert(is_tstate_valid(tstate));
    /* The caller must hold the GIL */
    assert(PyGILState_Check());

    /* Call _PySys_Audit() in the context of the current thread state,
       even if tstate is not the current thread state. */
    PyThreadState *current_tstate = _PyThreadState_GET();
    if (_PySys_Audit(current_tstate, "sys.settrace", NULL) < 0) {
        return -1;
    }

    PyObject *traceobj = tstate->c_traceobj;

    tstate->c_tracefunc = NULL;
    tstate->c_traceobj = NULL;
    /* Must make sure that profiling is not ignored if 'traceobj' is freed */
    _PyThreadState_ResumeTracing(tstate);
    Py_XDECREF(traceobj);

    Py_XINCREF(arg);
    tstate->c_traceobj = arg;
    tstate->c_tracefunc = func;

    /* Flag that tracing or profiling is turned on */
    _PyThreadState_ResumeTracing(tstate);

    return 0;
}

void
PyEval_SetTrace(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyEval_SetTrace(tstate, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        _PyErr_WriteUnraisableMsg("in PyEval_SetTrace", NULL);
    }
}


void
_PyEval_SetCoroutineOriginTrackingDepth(PyThreadState *tstate, int new_depth)
{
    assert(new_depth >= 0);
    tstate->coroutine_origin_tracking_depth = new_depth;
}

int
_PyEval_GetCoroutineOriginTrackingDepth(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->coroutine_origin_tracking_depth;
}

int
_PyEval_SetAsyncGenFirstiter(PyObject *firstiter)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (_PySys_Audit(tstate, "sys.set_asyncgen_hook_firstiter", NULL) < 0) {
        return -1;
    }

    Py_XINCREF(firstiter);
    Py_XSETREF(tstate->async_gen_firstiter, firstiter);
    return 0;
}

PyObject *
_PyEval_GetAsyncGenFirstiter(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->async_gen_firstiter;
}

int
_PyEval_SetAsyncGenFinalizer(PyObject *finalizer)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (_PySys_Audit(tstate, "sys.set_asyncgen_hook_finalizer", NULL) < 0) {
        return -1;
    }

    Py_XINCREF(finalizer);
    Py_XSETREF(tstate->async_gen_finalizer, finalizer);
    return 0;
}

PyObject *
_PyEval_GetAsyncGenFinalizer(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->async_gen_finalizer;
}

InterpreterFrame *
_PyEval_GetFrame(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->cframe->current_frame;
}

PyFrameObject *
PyEval_GetFrame(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate->cframe->current_frame == NULL) {
        return NULL;
    }
    PyFrameObject *f = _PyFrame_GetFrameObject(tstate->cframe->current_frame);
    if (f == NULL) {
        PyErr_Clear();
    }
    return f;
}

PyObject *
_PyEval_GetBuiltins(PyThreadState *tstate)
{
    InterpreterFrame *frame = tstate->cframe->current_frame;
    if (frame != NULL) {
        return frame->f_builtins;
    }
    return tstate->interp->builtins;
}

PyObject *
PyEval_GetBuiltins(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_GetBuiltins(tstate);
}

/* Convenience function to get a builtin from its name */
PyObject *
_PyEval_GetBuiltinId(_Py_Identifier *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *attr = _PyDict_GetItemIdWithError(PyEval_GetBuiltins(), name);
    if (attr) {
        Py_INCREF(attr);
    }
    else if (!_PyErr_Occurred(tstate)) {
        _PyErr_SetObject(tstate, PyExc_AttributeError, _PyUnicode_FromId(name));
    }
    return attr;
}

PyObject *
PyEval_GetLocals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
     InterpreterFrame *current_frame = tstate->cframe->current_frame;
    if (current_frame == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "frame does not exist");
        return NULL;
    }

    if (_PyFrame_FastToLocalsWithError(current_frame) < 0) {
        return NULL;
    }

    PyObject *locals = current_frame->f_locals;
    assert(locals != NULL);
    return locals;
}

PyObject *
PyEval_GetGlobals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    InterpreterFrame *current_frame = tstate->cframe->current_frame;
    if (current_frame == NULL) {
        return NULL;
    }
    return current_frame->f_globals;
}

int
PyEval_MergeCompilerFlags(PyCompilerFlags *cf)
{
    PyThreadState *tstate = _PyThreadState_GET();
    InterpreterFrame *current_frame = tstate->cframe->current_frame;
    int result = cf->cf_flags != 0;

    if (current_frame != NULL) {
        const int codeflags = current_frame->f_code->co_flags;
        const int compilerflags = codeflags & PyCF_MASK;
        if (compilerflags) {
            result = 1;
            cf->cf_flags |= compilerflags;
        }
#if 0 /* future keyword */
        if (codeflags & CO_GENERATOR_ALLOWED) {
            result = 1;
            cf->cf_flags |= CO_GENERATOR_ALLOWED;
        }
#endif
    }
    return result;
}


const char *
PyEval_GetFuncName(PyObject *func)
{
    if (PyMethod_Check(func))
        return PyEval_GetFuncName(PyMethod_GET_FUNCTION(func));
    else if (PyFunction_Check(func))
        return PyUnicode_AsUTF8(((PyFunctionObject*)func)->func_name);
    else if (PyCFunction_Check(func))
        return ((PyCFunctionObject*)func)->m_ml->ml_name;
    else
        return Py_TYPE(func)->tp_name;
}

const char *
PyEval_GetFuncDesc(PyObject *func)
{
    if (PyMethod_Check(func))
        return "()";
    else if (PyFunction_Check(func))
        return "()";
    else if (PyCFunction_Check(func))
        return "()";
    else
        return " object";
}

#define C_TRACE(x, call) \
if (use_tracing && tstate->c_profilefunc) { \
    if (call_trace(tstate->c_profilefunc, tstate->c_profileobj, \
        tstate, tstate->cframe->current_frame, \
        PyTrace_C_CALL, func)) { \
        x = NULL; \
    } \
    else { \
        x = call; \
        if (tstate->c_profilefunc != NULL) { \
            if (x == NULL) { \
                call_trace_protected(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate, tstate->cframe->current_frame, \
                    PyTrace_C_EXCEPTION, func); \
                /* XXX should pass (type, value, tb) */ \
            } else { \
                if (call_trace(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate, tstate->cframe->current_frame, \
                    PyTrace_C_RETURN, func)) { \
                    Py_DECREF(x); \
                    x = NULL; \
                } \
            } \
        } \
    } \
} else { \
    x = call; \
    }


static PyObject *
trace_call_function(PyThreadState *tstate,
                    PyObject *func,
                    PyObject **args, Py_ssize_t nargs,
                    PyObject *kwnames)
{
    int use_tracing = 1;
    PyObject *x;
    if (PyCFunction_CheckExact(func) || PyCMethod_CheckExact(func)) {
        C_TRACE(x, PyObject_Vectorcall(func, args, nargs, kwnames));
        return x;
    }
    else if (Py_IS_TYPE(func, &PyMethodDescr_Type) && nargs > 0) {
        /* We need to create a temporary bound method as argument
           for profiling.

           If nargs == 0, then this cannot work because we have no
           "self". In any case, the call itself would raise
           TypeError (foo needs an argument), so we just skip
           profiling. */
        PyObject *self = args[0];
        func = Py_TYPE(func)->tp_descr_get(func, self, (PyObject*)Py_TYPE(self));
        if (func == NULL) {
            return NULL;
        }
        C_TRACE(x, PyObject_Vectorcall(func,
                                        args+1, nargs-1,
                                        kwnames));
        Py_DECREF(func);
        return x;
    }
    return PyObject_Vectorcall(func, args, nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
}

static PyObject *
do_call_core(PyThreadState *tstate,
             PyObject *func,
             PyObject *callargs,
             PyObject *kwdict,
             int use_tracing
            )
{
    PyObject *result;

    if (PyCFunction_CheckExact(func) || PyCMethod_CheckExact(func)) {
        C_TRACE(result, PyObject_Call(func, callargs, kwdict));
        return result;
    }
    else if (Py_IS_TYPE(func, &PyMethodDescr_Type)) {
        Py_ssize_t nargs = PyTuple_GET_SIZE(callargs);
        if (nargs > 0 && use_tracing) {
            /* We need to create a temporary bound method as argument
               for profiling.

               If nargs == 0, then this cannot work because we have no
               "self". In any case, the call itself would raise
               TypeError (foo needs an argument), so we just skip
               profiling. */
            PyObject *self = PyTuple_GET_ITEM(callargs, 0);
            func = Py_TYPE(func)->tp_descr_get(func, self, (PyObject*)Py_TYPE(self));
            if (func == NULL) {
                return NULL;
            }

            C_TRACE(result, _PyObject_FastCallDictTstate(
                                    tstate, func,
                                    &_PyTuple_ITEMS(callargs)[1],
                                    nargs - 1,
                                    kwdict));
            Py_DECREF(func);
            return result;
        }
    }
    return PyObject_Call(func, callargs, kwdict);
}

/* Extract a slice index from a PyLong or an object with the
   nb_index slot defined, and store in *pi.
   Silently reduce values larger than PY_SSIZE_T_MAX to PY_SSIZE_T_MAX,
   and silently boost values less than PY_SSIZE_T_MIN to PY_SSIZE_T_MIN.
   Return 0 on error, 1 on success.
*/
int
_PyEval_SliceIndex(PyObject *v, Py_ssize_t *pi)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!Py_IsNone(v)) {
        Py_ssize_t x;
        if (_PyIndex_Check(v)) {
            x = PyNumber_AsSsize_t(v, NULL);
            if (x == -1 && _PyErr_Occurred(tstate))
                return 0;
        }
        else {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "slice indices must be integers or "
                             "None or have an __index__ method");
            return 0;
        }
        *pi = x;
    }
    return 1;
}

int
_PyEval_SliceIndexNotNone(PyObject *v, Py_ssize_t *pi)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t x;
    if (_PyIndex_Check(v)) {
        x = PyNumber_AsSsize_t(v, NULL);
        if (x == -1 && _PyErr_Occurred(tstate))
            return 0;
    }
    else {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "slice indices must be integers or "
                         "have an __index__ method");
        return 0;
    }
    *pi = x;
    return 1;
}

static PyObject *
import_name(PyThreadState *tstate, InterpreterFrame *frame,
            PyObject *name, PyObject *fromlist, PyObject *level)
{
    _Py_IDENTIFIER(__import__);
    PyObject *import_func, *res;
    PyObject* stack[5];

    import_func = _PyDict_GetItemIdWithError(frame->f_builtins, &PyId___import__);
    if (import_func == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_ImportError, "__import__ not found");
        }
        return NULL;
    }
    PyObject *locals = frame->f_locals;
    /* Fast path for not overloaded __import__. */
    if (import_func == tstate->interp->import_func) {
        int ilevel = _PyLong_AsInt(level);
        if (ilevel == -1 && _PyErr_Occurred(tstate)) {
            return NULL;
        }
        res = PyImport_ImportModuleLevelObject(
                        name,
                        frame->f_globals,
                        locals == NULL ? Py_None :locals,
                        fromlist,
                        ilevel);
        return res;
    }

    Py_INCREF(import_func);

    stack[0] = name;
    stack[1] = frame->f_globals;
    stack[2] = locals == NULL ? Py_None : locals;
    stack[3] = fromlist;
    stack[4] = level;
    res = _PyObject_FastCall(import_func, stack, 5);
    Py_DECREF(import_func);
    return res;
}

static PyObject *
import_from(PyThreadState *tstate, PyObject *v, PyObject *name)
{
    PyObject *x;
    PyObject *fullmodname, *pkgname, *pkgpath, *pkgname_or_unknown, *errmsg;

    if (_PyObject_LookupAttr(v, name, &x) != 0) {
        return x;
    }
    /* Issue #17636: in case this failed because of a circular relative
       import, try to fallback on reading the module directly from
       sys.modules. */
    pkgname = _PyObject_GetAttrId(v, &PyId___name__);
    if (pkgname == NULL) {
        goto error;
    }
    if (!PyUnicode_Check(pkgname)) {
        Py_CLEAR(pkgname);
        goto error;
    }
    fullmodname = PyUnicode_FromFormat("%U.%U", pkgname, name);
    if (fullmodname == NULL) {
        Py_DECREF(pkgname);
        return NULL;
    }
    x = PyImport_GetModule(fullmodname);
    Py_DECREF(fullmodname);
    if (x == NULL && !_PyErr_Occurred(tstate)) {
        goto error;
    }
    Py_DECREF(pkgname);
    return x;
 error:
    pkgpath = PyModule_GetFilenameObject(v);
    if (pkgname == NULL) {
        pkgname_or_unknown = PyUnicode_FromString("<unknown module name>");
        if (pkgname_or_unknown == NULL) {
            Py_XDECREF(pkgpath);
            return NULL;
        }
    } else {
        pkgname_or_unknown = pkgname;
    }

    if (pkgpath == NULL || !PyUnicode_Check(pkgpath)) {
        _PyErr_Clear(tstate);
        errmsg = PyUnicode_FromFormat(
            "cannot import name %R from %R (unknown location)",
            name, pkgname_or_unknown
        );
        /* NULL checks for errmsg and pkgname done by PyErr_SetImportError. */
        PyErr_SetImportError(errmsg, pkgname, NULL);
    }
    else {
        _Py_IDENTIFIER(__spec__);
        PyObject *spec = _PyObject_GetAttrId(v, &PyId___spec__);
        const char *fmt =
            _PyModuleSpec_IsInitializing(spec) ?
            "cannot import name %R from partially initialized module %R "
            "(most likely due to a circular import) (%S)" :
            "cannot import name %R from %R (%S)";
        Py_XDECREF(spec);

        errmsg = PyUnicode_FromFormat(fmt, name, pkgname_or_unknown, pkgpath);
        /* NULL checks for errmsg and pkgname done by PyErr_SetImportError. */
        PyErr_SetImportError(errmsg, pkgname, pkgpath);
    }

    Py_XDECREF(errmsg);
    Py_XDECREF(pkgname_or_unknown);
    Py_XDECREF(pkgpath);
    return NULL;
}

static int
import_all_from(PyThreadState *tstate, PyObject *locals, PyObject *v)
{
    _Py_IDENTIFIER(__all__);
    _Py_IDENTIFIER(__dict__);
    PyObject *all, *dict, *name, *value;
    int skip_leading_underscores = 0;
    int pos, err;

    if (_PyObject_LookupAttrId(v, &PyId___all__, &all) < 0) {
        return -1; /* Unexpected error */
    }
    if (all == NULL) {
        if (_PyObject_LookupAttrId(v, &PyId___dict__, &dict) < 0) {
            return -1;
        }
        if (dict == NULL) {
            _PyErr_SetString(tstate, PyExc_ImportError,
                    "from-import-* object has no __dict__ and no __all__");
            return -1;
        }
        all = PyMapping_Keys(dict);
        Py_DECREF(dict);
        if (all == NULL)
            return -1;
        skip_leading_underscores = 1;
    }

    for (pos = 0, err = 0; ; pos++) {
        name = PySequence_GetItem(all, pos);
        if (name == NULL) {
            if (!_PyErr_ExceptionMatches(tstate, PyExc_IndexError)) {
                err = -1;
            }
            else {
                _PyErr_Clear(tstate);
            }
            break;
        }
        if (!PyUnicode_Check(name)) {
            PyObject *modname = _PyObject_GetAttrId(v, &PyId___name__);
            if (modname == NULL) {
                Py_DECREF(name);
                err = -1;
                break;
            }
            if (!PyUnicode_Check(modname)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "module __name__ must be a string, not %.100s",
                              Py_TYPE(modname)->tp_name);
            }
            else {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%s in %U.%s must be str, not %.100s",
                              skip_leading_underscores ? "Key" : "Item",
                              modname,
                              skip_leading_underscores ? "__dict__" : "__all__",
                              Py_TYPE(name)->tp_name);
            }
            Py_DECREF(modname);
            Py_DECREF(name);
            err = -1;
            break;
        }
        if (skip_leading_underscores) {
            if (PyUnicode_READY(name) == -1) {
                Py_DECREF(name);
                err = -1;
                break;
            }
            if (PyUnicode_READ_CHAR(name, 0) == '_') {
                Py_DECREF(name);
                continue;
            }
        }
        value = PyObject_GetAttr(v, name);
        if (value == NULL)
            err = -1;
        else if (PyDict_CheckExact(locals))
            err = PyDict_SetItem(locals, name, value);
        else
            err = PyObject_SetItem(locals, name, value);
        Py_DECREF(name);
        Py_XDECREF(value);
        if (err != 0)
            break;
    }
    Py_DECREF(all);
    return err;
}


#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

static int
check_except_type_valid(PyThreadState *tstate, PyObject* right)
{
    if (PyTuple_Check(right)) {
        Py_ssize_t i, length;
        length = PyTuple_GET_SIZE(right);
        for (i = 0; i < length; i++) {
            PyObject *exc = PyTuple_GET_ITEM(right, i);
            if (!PyExceptionClass_Check(exc)) {
                _PyErr_SetString(tstate, PyExc_TypeError,
                    CANNOT_CATCH_MSG);
                return -1;
            }
        }
    }
    else {
        if (!PyExceptionClass_Check(right)) {
            _PyErr_SetString(tstate, PyExc_TypeError,
                CANNOT_CATCH_MSG);
            return -1;
        }
    }
    return 0;
}

static int
check_args_iterable(PyThreadState *tstate, PyObject *func, PyObject *args)
{
    if (Py_TYPE(args)->tp_iter == NULL && !PySequence_Check(args)) {
        /* check_args_iterable() may be called with a live exception:
         * clear it to prevent calling _PyObject_FunctionStr() with an
         * exception set. */
        _PyErr_Clear(tstate);
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%U argument after * must be an iterable, not %.200s",
                          funcstr, Py_TYPE(args)->tp_name);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    return 0;
}

static void
format_kwargs_error(PyThreadState *tstate, PyObject *func, PyObject *kwargs)
{
    /* _PyDict_MergeEx raises attribute
     * error (percolated from an attempt
     * to get 'keys' attribute) instead of
     * a type error if its second argument
     * is not a mapping.
     */
    if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
        _PyErr_Clear(tstate);
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            _PyErr_Format(
                tstate, PyExc_TypeError,
                "%U argument after ** must be a mapping, not %.200s",
                funcstr, Py_TYPE(kwargs)->tp_name);
            Py_DECREF(funcstr);
        }
    }
    else if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
        PyObject *exc, *val, *tb;
        _PyErr_Fetch(tstate, &exc, &val, &tb);
        if (val && PyTuple_Check(val) && PyTuple_GET_SIZE(val) == 1) {
            _PyErr_Clear(tstate);
            PyObject *funcstr = _PyObject_FunctionStr(func);
            if (funcstr != NULL) {
                PyObject *key = PyTuple_GET_ITEM(val, 0);
                _PyErr_Format(
                    tstate, PyExc_TypeError,
                    "%U got multiple values for keyword argument '%S'",
                    funcstr, key);
                Py_DECREF(funcstr);
            }
            Py_XDECREF(exc);
            Py_XDECREF(val);
            Py_XDECREF(tb);
        }
        else {
            _PyErr_Restore(tstate, exc, val, tb);
        }
    }
}

static void
format_exc_check_arg(PyThreadState *tstate, PyObject *exc,
                     const char *format_str, PyObject *obj)
{
    const char *obj_str;

    if (!obj)
        return;

    obj_str = PyUnicode_AsUTF8(obj);
    if (!obj_str)
        return;

    _PyErr_Format(tstate, exc, format_str, obj_str);

    if (exc == PyExc_NameError) {
        // Include the name in the NameError exceptions to offer suggestions later.
        _Py_IDENTIFIER(name);
        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        PyErr_NormalizeException(&type, &value, &traceback);
        if (PyErr_GivenExceptionMatches(value, PyExc_NameError)) {
            // We do not care if this fails because we are going to restore the
            // NameError anyway.
            (void)_PyObject_SetAttrId(value, &PyId_name, obj);
        }
        PyErr_Restore(type, value, traceback);
    }
}

static void
format_exc_unbound(PyThreadState *tstate, PyCodeObject *co, int oparg)
{
    PyObject *name;
    /* Don't stomp existing exception */
    if (_PyErr_Occurred(tstate))
        return;
    name = PyTuple_GET_ITEM(co->co_localsplusnames, oparg);
    if (oparg < co->co_nplaincellvars + co->co_nlocals) {
        format_exc_check_arg(tstate, PyExc_UnboundLocalError,
                             UNBOUNDLOCAL_ERROR_MSG, name);
    } else {
        format_exc_check_arg(tstate, PyExc_NameError,
                             UNBOUNDFREE_ERROR_MSG, name);
    }
}

static void
format_awaitable_error(PyThreadState *tstate, PyTypeObject *type, int prevprevopcode, int prevopcode)
{
    if (type->tp_as_async == NULL || type->tp_as_async->am_await == NULL) {
        if (prevopcode == BEFORE_ASYNC_WITH) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'async with' received an object from __aenter__ "
                          "that does not implement __await__: %.100s",
                          type->tp_name);
        }
        else if (prevopcode == WITH_EXCEPT_START || (prevopcode == CALL_FUNCTION && prevprevopcode == DUP_TOP)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'async with' received an object from __aexit__ "
                          "that does not implement __await__: %.100s",
                          type->tp_name);
        }
    }
}

#ifdef DYNAMIC_EXECUTION_PROFILE

static PyObject *
getarray(long a[256])
{
    int i;
    PyObject *l = PyList_New(256);
    if (l == NULL) return NULL;
    for (i = 0; i < 256; i++) {
        PyObject *x = PyLong_FromLong(a[i]);
        if (x == NULL) {
            Py_DECREF(l);
            return NULL;
        }
        PyList_SET_ITEM(l, i, x);
    }
    for (i = 0; i < 256; i++)
        a[i] = 0;
    return l;
}

PyObject *
_Py_GetDXProfile(PyObject *self, PyObject *args)
{
#ifndef DXPAIRS
    return getarray(dxp);
#else
    int i;
    PyObject *l = PyList_New(257);
    if (l == NULL) return NULL;
    for (i = 0; i < 257; i++) {
        PyObject *x = getarray(dxpairs[i]);
        if (x == NULL) {
            Py_DECREF(l);
            return NULL;
        }
        PyList_SET_ITEM(l, i, x);
    }
    return l;
#endif
}

#endif

Py_ssize_t
_PyEval_RequestCodeExtraIndex(freefunc free)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    Py_ssize_t new_index;

    if (interp->co_extra_user_count == MAX_CO_EXTRA_USERS - 1) {
        return -1;
    }
    new_index = interp->co_extra_user_count++;
    interp->co_extra_freefuncs[new_index] = free;
    return new_index;
}

static void
dtrace_function_entry(InterpreterFrame *frame)
{
    const char *filename;
    const char *funcname;
    int lineno;

    PyCodeObject *code = frame->f_code;
    filename = PyUnicode_AsUTF8(code->co_filename);
    funcname = PyUnicode_AsUTF8(code->co_name);
    lineno = PyCode_Addr2Line(frame->f_code, frame->f_lasti*sizeof(_Py_CODEUNIT));

    PyDTrace_FUNCTION_ENTRY(filename, funcname, lineno);
}

static void
dtrace_function_return(InterpreterFrame *frame)
{
    const char *filename;
    const char *funcname;
    int lineno;

    PyCodeObject *code = frame->f_code;
    filename = PyUnicode_AsUTF8(code->co_filename);
    funcname = PyUnicode_AsUTF8(code->co_name);
    lineno = PyCode_Addr2Line(frame->f_code, frame->f_lasti*sizeof(_Py_CODEUNIT));

    PyDTrace_FUNCTION_RETURN(filename, funcname, lineno);
}

/* DTrace equivalent of maybe_call_line_trace. */
static void
maybe_dtrace_line(InterpreterFrame *frame,
                  PyTraceInfo *trace_info,
                  int instr_prev)
{
    const char *co_filename, *co_name;

    /* If the last instruction executed isn't in the current
       instruction window, reset the window.
    */
    initialize_trace_info(trace_info, frame);
    int lastline = _PyCode_CheckLineNumber(instr_prev*sizeof(_Py_CODEUNIT), &trace_info->bounds);
    int line = _PyCode_CheckLineNumber(frame->f_lasti*sizeof(_Py_CODEUNIT), &trace_info->bounds);
    if (line != -1) {
        /* Trace backward edges or first instruction of a new line */
        if (frame->f_lasti < instr_prev ||
            (line != lastline && frame->f_lasti*sizeof(_Py_CODEUNIT) == (unsigned int)trace_info->bounds.ar_start))
        {
            co_filename = PyUnicode_AsUTF8(frame->f_code->co_filename);
            if (!co_filename) {
                co_filename = "?";
            }
            co_name = PyUnicode_AsUTF8(frame->f_code->co_name);
            if (!co_name) {
                co_name = "?";
            }
            PyDTrace_LINE(co_filename, co_name, line);
        }
    }
}

/* Implement Py_EnterRecursiveCall() and Py_LeaveRecursiveCall() as functions
   for the limited API. */

#undef Py_EnterRecursiveCall

int Py_EnterRecursiveCall(const char *where)
{
    return _Py_EnterRecursiveCall_inline(where);
}

#undef Py_LeaveRecursiveCall

void Py_LeaveRecursiveCall(void)
{
    _Py_LeaveRecursiveCall_inline();
}
