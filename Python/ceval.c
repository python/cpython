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
#include "pycore_code.h"          // _PyCode_InitOpcache()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_pyerrors.h"      // _PyErr_Fetch()
#include "pycore_pylifecycle.h"   // _PyErr_Print()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_sysmodule.h"     // _PySys_Audit()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

#include "code.h"
#include "dictobject.h"
#include "frameobject.h"
#if defined(Py_DEBUG) || defined(LLTRACE)
#define NEED_STRING_FOR_ENUM
#endif
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

#include <stdbool.h>

typedef struct {
    PyCodeObject *code; // The code object for the bounds. May be NULL.
    PyCodeAddressRange bounds; // Only valid if code != NULL.
    CFrame cframe;
} PyTraceInfo;


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
Py_LOCAL_INLINE(PyObject *) call_function(
    PyThreadState *tstate, PyTraceInfo *trace_info,
    PyObject *func, PyObject **args, Py_ssize_t nargs, PyObject *kwnames);
static PyObject * do_call_core(
        PyThreadState *tstate, PyTraceInfo *, PyObject *func,
        PyObject *callargs, PyObject *kwdict);

#ifdef LLTRACE
static int lltrace;
#endif
static int call_trace(Py_tracefunc, PyObject *,
                      PyThreadState *, PyFrameObject *,
                      PyTraceInfo *,
                      int, PyObject *);
static int call_trace_protected(Py_tracefunc, PyObject *,
                                PyThreadState *, PyFrameObject *,
                                PyTraceInfo *,
                                int, PyObject *);
static void call_exc_trace(Py_tracefunc, PyObject *,
                           PyThreadState *, PyFrameObject *,
                           PyTraceInfo *trace_info);
static int maybe_call_line_trace(Py_tracefunc, PyObject *,
                                 PyThreadState *, PyFrameObject *,
                                 PyTraceInfo *, _Py_CODEUNIT *);
static void maybe_dtrace_line(PyFrameObject *, PyTraceInfo *, _Py_CODEUNIT *);
static void dtrace_function_entry(PyFrameObject *);
static void dtrace_function_return(PyFrameObject *);

static PyObject * import_name(PyThreadState *, PyFrameObject *,
                              PyObject *, PyObject *);
static PyObject * import_from(PyThreadState *, PyObject *, PyObject *);
static int import_all_from(PyThreadState *, PyObject *, PyObject *);
static PyObject * special_lookup(PyThreadState *, PyObject *, _Py_Identifier *);

static void format_kwargs_error(PyThreadState *, PyObject *, PyObject *);
static void format_exc_unbound_fast(PyThreadState *, PyCodeObject *, int);
static void format_exc_unbound_deref(PyThreadState *, PyCodeObject *, int);
static void format_exc_name_error(PyThreadState *, PyObject *, bool);

/* Dynamic execution profile */
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
static long dxpairs[257][256];
#define dxp dxpairs[256]
#else
static long dxp[256];
#endif
#endif

/* per opcode cache */
static int opcache_min_runs = 1024;  /* create opcache when code executed this many times */
#define OPCODE_CACHE_MAX_TRIES 20
#define OPCACHE_STATS 0  /* Enable stats */

// This function allows to deactivate the opcode cache. As different cache mechanisms may hold
// references, this can mess with the reference leak detector functionality so the cache needs
// to be deactivated in such scenarios to avoid false positives. See bpo-3714 for more information.
void
_PyEval_DeactivateOpCache(void)
{
    opcache_min_runs = 0;
}

#if OPCACHE_STATS
static size_t opcache_code_objects = 0;
static size_t opcache_code_objects_extra_mem = 0;

static size_t opcache_global_opts = 0;
static size_t opcache_global_hits = 0;
static size_t opcache_global_misses = 0;

static size_t opcache_attr_opts = 0;
static size_t opcache_attr_hits = 0;
static size_t opcache_attr_misses = 0;
static size_t opcache_attr_deopts = 0;
static size_t opcache_attr_total = 0;
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
#if OPCACHE_STATS
    fprintf(stderr, "-- Opcode cache number of objects  = %zd\n",
            opcache_code_objects);

    fprintf(stderr, "-- Opcode cache total extra mem    = %zd\n",
            opcache_code_objects_extra_mem);

    fprintf(stderr, "\n");

    fprintf(stderr, "-- Opcode cache LOAD_GLOBAL hits   = %zd (%d%%)\n",
            opcache_global_hits,
            (int) (100.0 * opcache_global_hits /
                (opcache_global_hits + opcache_global_misses)));

    fprintf(stderr, "-- Opcode cache LOAD_GLOBAL misses = %zd (%d%%)\n",
            opcache_global_misses,
            (int) (100.0 * opcache_global_misses /
                (opcache_global_hits + opcache_global_misses)));

    fprintf(stderr, "-- Opcode cache LOAD_GLOBAL opts   = %zd\n",
            opcache_global_opts);

    fprintf(stderr, "\n");

    fprintf(stderr, "-- Opcode cache LOAD_ATTR hits     = %zd (%d%%)\n",
            opcache_attr_hits,
            (int) (100.0 * opcache_attr_hits /
                opcache_attr_total));

    fprintf(stderr, "-- Opcode cache LOAD_ATTR misses   = %zd (%d%%)\n",
            opcache_attr_misses,
            (int) (100.0 * opcache_attr_misses /
                opcache_attr_total));

    fprintf(stderr, "-- Opcode cache LOAD_ATTR opts     = %zd\n",
            opcache_attr_opts);

    fprintf(stderr, "-- Opcode cache LOAD_ATTR deopts   = %zd\n",
            opcache_attr_deopts);

    fprintf(stderr, "-- Opcode cache LOAD_ATTR total    = %zd\n",
            opcache_attr_total);
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
    PyThreadState *tstate = _PyThreadState_GET();
    tstate->interp->ceval.recursion_limit = new_limit;
}

/* The function _Py_EnterRecursiveCall() only calls _Py_CheckRecursiveCall()
   if the recursion_depth reaches recursion_limit.
   If USE_STACKCHECK, the macro decrements recursion_limit
   to guarantee that _Py_CheckRecursiveCall() is regularly called.
   Without USE_STACKCHECK, there is no need for this. */
int
_Py_CheckRecursiveCall(PyThreadState *tstate, const char *where)
{
    int recursion_limit = tstate->interp->ceval.recursion_limit;

#ifdef USE_STACKCHECK
    tstate->stackcheck_counter = 0;
    if (PyOS_CheckStack()) {
        --tstate->recursion_depth;
        _PyErr_SetString(tstate, PyExc_MemoryError, "Stack overflow");
        return -1;
    }
#endif
    if (tstate->recursion_headroom) {
        if (tstate->recursion_depth > recursion_limit + 50) {
            /* Overflowing while handling an overflow. Give up. */
            Py_FatalError("Cannot recover from stack overflow.");
        }
    }
    else {
        if (tstate->recursion_depth > recursion_limit) {
            tstate->recursion_headroom++;
            _PyErr_Format(tstate, PyExc_RecursionError,
                        "maximum recursion depth exceeded%s",
                        where);
            tstate->recursion_headroom--;
            --tstate->recursion_depth;
            return -1;
        }
    }
    return 0;
}


// PEP 634: Structural Pattern Matching


// Return a tuple of values corresponding to keys, with error checks for
// duplicate/missing keys.
static PyObject*
match_keys(PyThreadState *tstate, PyObject *map, PyObject *keys)
{
    assert(PyTuple_CheckExact(keys));
    if (PyTuple_GET_SIZE(keys) == 0) {
        Py_INCREF(Py_True);
        return Py_True;
    }

    Py_ssize_t nkeys = PyTuple_GET_SIZE(keys);
    PyObject *seen = NULL;
    PyObject *dummy = NULL;
    PyObject *values = NULL;
    PyObject *ret_values = NULL;
    // We use the two argument form of map.get(key, default) for two reasons:
    // - Atomically check for a key and get its value without error handling.
    // - Don't cause key creation or resizing in dict subclasses like
    //   collections.defaultdict that define __missing__ (or similar).
    _Py_IDENTIFIER(get);
    PyObject *get = _PyObject_GetAttrId(map, &PyId_get);
    if (get == NULL) {
        goto done;
    }
    seen = PySet_New(NULL);
    if (seen == NULL) {
        goto done;
    }
    // dummy = object()
    dummy = _PyObject_CallNoArg((PyObject *)&PyBaseObject_Type);
    if (dummy == NULL) {
        goto done;
    }
    values = PyTuple_New(PyTuple_GET_SIZE(keys));
    if (values == NULL) {
        goto done;
    }

    for (Py_ssize_t i = 0; i < nkeys; i++) {
        PyObject *key = PyTuple_GET_ITEM(keys, i);
        if (PySet_Contains(seen, key) || PySet_Add(seen, key)) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                _PyErr_Format(tstate, PyExc_ValueError,
                              "mapping pattern checks duplicate key (%R)", key);
            }
            goto done;
        }
        PyObject *value = PyObject_CallFunctionObjArgs(get, key, dummy, NULL);
        if (value == NULL) {
            goto done;
        }
        PyTuple_SET_ITEM(values, i, value);
        if (value == dummy) {
            // key not in map! Return False:
            Py_INCREF(Py_False);
            ret_values = Py_False;
            goto done;
        }
    }
    ret_values = values;
    values = NULL;

done:
    Py_XDECREF(get);
    Py_XDECREF(seen);
    Py_XDECREF(dummy);
    Py_XDECREF(values);
    return ret_values;
}

/* 1: attribute extracted and save to *save_to.
 * 0: no such attribute.
 * -1: failed, an error is set. */
static int
match_class_attr(PyThreadState *tstate, PyObject *subject, PyTypeObject *type_obj,
                 PyObject *name, PyObject *seen, PyObject **save_to)
{
    assert(PyUnicode_CheckExact(name));
    assert(PySet_CheckExact(seen));
    if (PySet_Contains(seen, name) || PySet_Add(seen, name)) {
        if (!_PyErr_Occurred(tstate)) {
            // Seen it before!
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%s() got multiple sub-patterns for attribute %R",
                          type_obj->tp_name, name);
        }
        return -1;
    }
    PyObject *attr = PyObject_GetAttr(subject, name);
    if (attr) {
        *save_to = attr;
        return 1;
    } else if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
        _PyErr_Clear(tstate);
        return 0;
    }
    return -1;
}

/* On failure, return NULL and an error is set.
 * If no attributes are extracted, return a bool result of isinstance check.
 * On success match, return a tuple of extracted attributes.
 * Otherwise, return False and no error is set. */
static PyObject*
match_class(PyThreadState *tstate, PyObject *subject, PyObject *type,
            Py_ssize_t nargs, PyObject *kwargs)
{
    assert(kwargs == NULL || PyTuple_CheckExact(kwargs));
    Py_ssize_t nkwargs = kwargs ? PyTuple_GET_SIZE(kwargs) : 0;
    if (!PyType_Check(type)) {
        const char *e = "called match pattern must be a type";
        _PyErr_Format(tstate, PyExc_TypeError, e);
        return NULL;
    }
    PyTypeObject* type_obj = (PyTypeObject *)type;
    PyObject *match_args = NULL;
    PyObject *seen = NULL;
    PyObject *values = NULL;

    // First, an isinstance check:
    if (PyObject_IsInstance(subject, type) <= 0) {
        goto non_match;
    }
    // Match class only, just return a bool:
    if (!nargs && !nkwargs) {
        Py_INCREF(Py_True);
        return Py_True;
    }

    seen = PySet_New(NULL);
    if (seen == NULL) {
        goto failure;
    }
    values = PyTuple_New(nargs + nkwargs);
    if (values == NULL) {
        goto failure;
    }
    PyObject **value_arr = &PyTuple_GET_ITEM(values, 0);

    // The positional subpatterns:
    if (nargs) {
        match_args = PyObject_GetAttrString(type, "__match_args__");
        if (match_args) {
            if (!PyTuple_CheckExact(match_args)) {
                const char *e = "%s.__match_args__ must be a tuple (got %s)";
                _PyErr_Format(tstate, PyExc_TypeError, e,
                              type_obj->tp_name,
                              Py_TYPE(match_args)->tp_name);
                goto failure;
            }
            Py_ssize_t allowed = PyTuple_GET_SIZE(match_args);
            if (allowed < nargs) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%s() accepts %d positional sub-pattern%s (%d given)",
                              type_obj->tp_name,
                              allowed, (allowed == 1) ? "" : "s", nargs);
                goto failure;
            }
            for (Py_ssize_t i = 0; i < nargs; i++) {
                PyObject *name = PyTuple_GET_ITEM(match_args, i);
                if (!PyUnicode_CheckExact(name)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "__match_args__ elements must be strings "
                                  "(got %s)", Py_TYPE(name)->tp_name);
                    goto failure;
                }
                int res = match_class_attr(tstate, subject, type_obj, name, seen, &value_arr[i]);
                if (res < 0) {
                    goto failure;
                }
                if (!res) {
                    goto non_match;
                }
            }
            Py_CLEAR(match_args);
        } else if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
            _PyErr_Clear(tstate);
            // _Py_TPFLAGS_MATCH_SELF is only acknowledged if the type does not
            // define __match_args__. This is natural behavior for subclasses:
            // it's as if __match_args__ is some "magic" value that is lost as
            // soon as they redefine it.
            if (!PyType_HasFeature(type_obj, _Py_TPFLAGS_MATCH_SELF)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%s() does not allow positional sub-pattern",
                              type_obj->tp_name);
                goto failure;
            }
            if (nargs > 1) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%s() allows only single positional sub-pattern "
                              "to match itself (%d given)",
                              type_obj->tp_name, nargs);
                goto failure;
            }
            Py_INCREF(subject);
            PyTuple_SET_ITEM(values, 0, subject);
        } else {
            goto failure;
        }
    }

    // The keyword subpatterns:
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *name = PyTuple_GET_ITEM(kwargs, i);
        int res = match_class_attr(tstate, subject, type_obj, name, seen, &value_arr[nargs + i]);
        if (res < 0) {
            goto failure;
        }
        if (!res) {
            goto non_match;
        }
    }
    Py_DECREF(seen);
    return values;

non_match:
    Py_XDECREF(match_args);
    Py_XDECREF(seen);
    Py_XDECREF(values);
    Py_INCREF(Py_False);
    return Py_False;

failure:
    Py_XDECREF(match_args);
    Py_DECREF(seen);
    Py_DECREF(values);
    assert(_PyErr_Occurred(tstate));
    return NULL;
}


static int do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause);
static int unpack_iterable(PyThreadState *, PyObject *, _Py_OPARG, _Py_OPARG, PyObject **);

static void restore_exc(PyThreadState *tstate, PyObject **sextuple, bool exc) {
    PyObject *tp = sextuple[0];
    PyObject *val = sextuple[1];
    PyObject *tb = sextuple[2];
    sextuple[0] = Py_Undefined;
    sextuple[1] = Py_Undefined;
    sextuple[2] = Py_Undefined;
    _PyErr_StackItem *exc_info = tstate->exc_info;
    PyObject *old_tp = exc_info->exc_type;
    PyObject *old_val = exc_info->exc_value;
    PyObject *old_tb = exc_info->exc_traceback;
    exc_info->exc_type = tp;
    exc_info->exc_value = val;
    exc_info->exc_traceback = tb;
    Py_DECREF(old_tp);
    Py_DECREF(old_val);
    Py_DECREF(old_tb);

    tp = sextuple[3];
    val = sextuple[4];
    tb = sextuple[5];
    sextuple[3] = Py_Undefined;
    sextuple[4] = Py_Undefined;
    sextuple[5] = Py_Undefined;
    if (exc) {
        _PyErr_Restore(tstate, tp, val, tb);
    } else {
        Py_DECREF(tp);
        Py_DECREF(val);
        Py_DECREF(tb);
    }
}

PyObject *
PyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    PyThreadState *tstate = PyThreadState_GET();
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
    return _PyEval_Vector(tstate, &desc, locals, NULL, 0, NULL);
}


/* Interpreter main loop */

PyObject *
PyEval_EvalFrame(PyFrameObject *f)
{
    /* Function kept for backward compatibility */
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f, 0);
}

PyObject *
PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f, throwflag);
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
#define OR_LLTRACE || lltrace
#else
#define OR_LLTRACE
#endif

#ifdef WITH_DTRACE
#define OR_DTRACE_LINE || PyDTrace_LINE_ENABLED()
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

#ifndef ON_DEMAND_OPARG_DECODING
    #define ON_DEMAND_OPARG_DECODING 1
#endif

#define ARG_YES(X) X
#define ARG_NO(X)

/* OpCode prediction macros
    Some opcodes tend to come in pairs thus making it possible to
    predict the second code when the first is run.  For example,
    COMPARE_XXX is often followed by JUMP_IF_FALSE or JUMP_IF_TRUE.

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
#define PRED_OP_LABEL(X) PRED_##X
#if defined(DYNAMIC_EXECUTION_PROFILE) || USE_COMPUTED_GOTOS
#define USE_PREDICT(X)
#define PREDICT(X)
#else
#define USE_PREDICT(X) X
#define PREDICT(X) \
    do { \
        if (_Py_OPCODE(*next_instr) == X) { \
            f->f_last_instr = next_instr; \
            FETCH_NEXT_INSTR(); \
            goto PRED_OP_LABEL(X); \
        } \
    } while(0)
#endif

#if ON_DEMAND_OPARG_DECODING

#if USE_COMPUTED_GOTOS
#define FETCH_NEXT_INSTR() \
    do { \
        fetched_instr = *next_instr++; \
        opcode = _Py_OPCODE(fetched_instr); \
    } while (0)
#define OP_LABEL(X) TARGET_##X
#define EXT_OP_LABEL(X) EXT_TARGET_##X
#else
#define FETCH_NEXT_INSTR() \
    do { \
        fetched_instr = *next_instr++; \
        opcode = (int) _Py_OPCODE(fetched_instr); \
    } while (0)
#define OP_LABEL(X) case(X)
#define EXT_OP_LABEL(X) case(X + 0x100)
#endif

#define CASE_OP_IMPL(X, NEED_OPARG1, NEED_OPARG2, NEED_OPARG3, NEED_PRED, ...) \
    EXT_OP_LABEL(X): { \
            NEED_OPARG1(_Py_OPARG oparg1 = ext_oparg1;) \
            NEED_OPARG2(_Py_OPARG oparg2 = ext_oparg2;) \
            NEED_OPARG3(_Py_OPARG oparg3 = ext_oparg3;) \
            goto TARGET_BODY_##X; \
        NEED_PRED(PRED_OP_LABEL(X):) \
        OP_LABEL(X): \
            NEED_OPARG1(oparg1 = _Py_OPARG1(fetched_instr);) \
            NEED_OPARG2(oparg2 = _Py_OPARG2(fetched_instr);) \
            NEED_OPARG3(oparg3 = _Py_OPARG3(fetched_instr);) \
        TARGET_BODY_##X: ;

#else

#define FETCH_NEXT_INSTR() \
    do { \
        _Py_CODEUNIT fetched_instr = *next_instr++; \
        opcode = _Py_OPCODE(fetched_instr); \
        oparg1 = _Py_OPARG1(fetched_instr); \
        oparg2 = _Py_OPARG2(fetched_instr); \
        oparg3 = _Py_OPARG3(fetched_instr); \
    } while (0)
#if USE_COMPUTED_GOTOS
#define CASE_OP_IMPL(X, _1, _2, _3, NEED_PRED, ...) TARGET_##X: {
#define OP_LABEL(X) TARGET_##X
#else
#define CASE_OP_IMPL(X, _1, _2, _3, NEED_PRED, ...) NEED_PRED(PRED_OP_LABEL(X):) case(X): {
#define OP_LABEL(X) case(X)
#endif

#endif

#if USE_COMPUTED_GOTOS
#define SWITCH_OP() goto *opcode_targets[opcode];
#define DISPATCH() \
    { \
        if (trace_info.cframe.use_tracing OR_DTRACE_LINE OR_LLTRACE) { \
            goto tracing_dispatch; \
        } \
        f->f_last_instr = next_instr; \
        FETCH_NEXT_INSTR(); \
        goto *opcode_targets[opcode]; \
    }
#else
#define SWITCH_OP() switch (opcode)
#define DISPATCH() goto predispatch;
#endif
#define CASE_OP(...) CASE_OP_IMPL(__VA_ARGS__, ARG_NO)
#define END_CASE_OP }

#define CHECK_EVAL_BREAKER() \
    if (_Py_atomic_load_relaxed(eval_breaker)) { \
        goto main_loop; \
    }

#define GET_INSTR_INDEX() ((_Py_OPARG)(next_instr - first_instr))
#define SET_INSTR_POINTER(X) next_instr = (X)
#define SET_INSTR_INDEX(X) SET_INSTR_POINTER(first_instr + (X))
#define ADJUST_INSTR_POINTER(B, F) SET_INSTR_POINTER(next_instr - (B) + (F))
#define INCREASE_INSTR_POINTER(F) SET_INSTR_POINTER(next_instr + (F))


#define PREG(I)           (&fastlocals[I])
#define SET_FRAME_VALUE(LV, RV) do { \
        PyObject *_py_tmp = LV; \
        LV = RV; \
        Py_DECREF(_py_tmp); \
    } while(0)

#ifdef LLTRACE
static void
regtrace(PyThreadState *tstate, const char *str, _Py_OPARG i, PyObject *v)
{
    printf("%s REG[%"_Py_OPARG_PRI"]=", str, i);
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    if (PyObject_Print(v, stdout, 0) != 0) {
        /* Don't know what else to do */
        _PyErr_Clear(tstate);
    }
    printf("\n");
    PyErr_Restore(type, value, traceback);
}

static inline PyObject *
regtrace_get(PyThreadState *tstate, PyCodeObject *co, PyObject **pv, _Py_OPARG i)
{
    assert(i < frame_local_num(co) + frame_tmp_num(co) + frame_const_num(co));
    if (lltrace) {
        regtrace(tstate, "GET", i, pv[i]);
    }
    return pv[i];
}

static inline void
regtrace_set(PyThreadState *tstate, PyCodeObject *co, PyObject **pv, _Py_OPARG i, PyObject *v)
{
    assert(i < frame_local_num(co) + frame_tmp_num(co));
    if (lltrace) {
        regtrace(tstate, "SET", i, v);
    }
    SET_FRAME_VALUE(pv[i], v);
}

/* The SET_REG() macro must not DECREF the local variable in-place and
   then store the new value; it must copy the old value to a temporary
   value, then store the new value, and then DECREF the temporary value.
   This is because it is possible that during the DECREF the frame is
   accessed by other code (e.g. a __del__ method or gc.collect()) and the
   variable would be pointing to already-freed memory. */
#define REG(I)              regtrace_get(tstate, co, fastlocals, I)
#define SET_REG(I, V)       regtrace_set(tstate, co, fastlocals, I, V)
#else
#define REG(I)              (fastlocals[I])
#define SET_REG(I, V)       SET_FRAME_VALUE(fastlocals[I], V)
#endif

/* Tuple access macros */
#ifndef Py_DEBUG
#define GET_NAME(I) PyTuple_GET_ITEM(names, (I))
#else
#define GET_NAME(I) PyTuple_GET_ITEM(names, (I))
#endif

    /* macros for opcode cache */
#define OPCACHE_CHECK() \
    do { \
        co_opcache = NULL; \
        if (co->co_opcache != NULL) { \
            unsigned char co_opcache_offset = \
                co->co_opcache_map[GET_INSTR_INDEX()]; \
            if (co_opcache_offset > 0) { \
                assert(co_opcache_offset <= co->co_opcache_size); \
                co_opcache = &co->co_opcache[co_opcache_offset - 1]; \
                assert(co_opcache != NULL); \
            } \
        } \
    } while (0)

#define OPCACHE_DEOPT() \
    do { \
        if (co_opcache != NULL) { \
            co_opcache->optimized = -1; \
            unsigned char co_opcache_offset = \
                co->co_opcache_map[GET_INSTR_INDEX()]; \
            assert(co_opcache_offset <= co->co_opcache_size); \
            co->co_opcache_map[co_opcache_offset] = 0; \
            co_opcache = NULL; \
        } \
    } while (0)

#define OPCACHE_DEOPT_LOAD_ATTR() \
    do { \
        if (co_opcache != NULL) { \
            OPCACHE_STAT_ATTR_DEOPT(); \
            OPCACHE_DEOPT(); \
        } \
    } while (0)

#define OPCACHE_MAYBE_DEOPT_LOAD_ATTR() \
    do { \
        if (co_opcache != NULL && --co_opcache->optimized <= 0) { \
            OPCACHE_DEOPT_LOAD_ATTR(); \
        } \
    } while (0)

#if OPCACHE_STATS

#define OPCACHE_STAT_GLOBAL_HIT() \
    do { \
        if (co->co_opcache != NULL) opcache_global_hits++; \
    } while (0)

#define OPCACHE_STAT_GLOBAL_MISS() \
    do { \
        if (co->co_opcache != NULL) opcache_global_misses++; \
    } while (0)

#define OPCACHE_STAT_GLOBAL_OPT() \
    do { \
        if (co->co_opcache != NULL) opcache_global_opts++; \
    } while (0)

#define OPCACHE_STAT_ATTR_HIT() \
    do { \
        if (co->co_opcache != NULL) opcache_attr_hits++; \
    } while (0)

#define OPCACHE_STAT_ATTR_MISS() \
    do { \
        if (co->co_opcache != NULL) opcache_attr_misses++; \
    } while (0)

#define OPCACHE_STAT_ATTR_OPT() \
    do { \
        if (co->co_opcache!= NULL) opcache_attr_opts++; \
    } while (0)

#define OPCACHE_STAT_ATTR_DEOPT() \
    do { \
        if (co->co_opcache != NULL) opcache_attr_deopts++; \
    } while (0)

#define OPCACHE_STAT_ATTR_TOTAL() \
    do { \
        if (co->co_opcache != NULL) opcache_attr_total++; \
    } while (0)

#else /* OPCACHE_STATS */

#define OPCACHE_STAT_GLOBAL_HIT()
#define OPCACHE_STAT_GLOBAL_MISS()
#define OPCACHE_STAT_GLOBAL_OPT()

#define OPCACHE_STAT_ATTR_HIT()
#define OPCACHE_STAT_ATTR_MISS()
#define OPCACHE_STAT_ATTR_OPT()
#define OPCACHE_STAT_ATTR_DEOPT()
#define OPCACHE_STAT_ATTR_TOTAL()

#endif


PyObject* _Py_HOT_FUNCTION
_PyEval_EvalFrameDefault(PyThreadState *tstate, PyFrameObject *f, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);

#if USE_COMPUTED_GOTOS
/* Make the static jump table */
#define DEF_GOTO_TARGET(X, ...) &&OP_LABEL(X),
#define NO_GOTO_TARGET(I) &&_unknown_opcode,
static void *opcode_targets[256] = {
    FOREACH_OPCODE(DEF_GOTO_TARGET, NO_GOTO_TARGET)
};
#if ON_DEMAND_OPARG_DECODING
#define DEF_GOTO_EXT_TARGET(X, ...) &&EXT_OP_LABEL(X),
static void *ext_opcode_targets[256] = {
        FOREACH_OPCODE(DEF_GOTO_EXT_TARGET, NO_GOTO_TARGET)
};
#endif
#endif
    /* Return value */
    PyObject *retval = NULL;
    _Py_atomic_int *const eval_breaker = &tstate->interp->ceval.eval_breaker;
    _PyOpcache *co_opcache;

#ifdef LLTRACE
    _Py_IDENTIFIER(__ltrace__);
#endif

    if (_Py_EnterRecursiveCall(tstate, "")) {
        return NULL;
    }

    CFrame *prev_cframe = tstate->cframe;
    PyTraceInfo trace_info = {
            /* Mark trace_info as uninitialized */
            .code = NULL,
            /* WARNING: Because the CFrame lives on the C stack,
             * but can be accessed from a heap allocated object (tstate)
             * strict stack discipline must be maintained.
             */
            .cframe = {
                    .use_tracing = prev_cframe->use_tracing,
                    .previous = prev_cframe,
            }
    };
    /* push frame */
    tstate->cframe = &trace_info.cframe;
    tstate->frame = f;

    if (trace_info.cframe.use_tracing) {
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
                                     tstate, f, &trace_info,
                                     PyTrace_CALL, Py_None)) {
                /* Trace function raised an error */
                goto exit_eval_frame;
            }
        }
        if (tstate->c_profilefunc != NULL) {
            /* Similar for c_profilefunc, except it needn't
               return itself and isn't called for "line" events */
            if (call_trace_protected(tstate->c_profilefunc,
                                     tstate->c_profileobj,
                                     tstate, f, &trace_info,
                                     PyTrace_CALL, Py_None)) {
                /* Profile function raised an error */
                goto exit_eval_frame;
            }
        }
    }

    if (PyDTrace_FUNCTION_ENTRY_ENABLED())
        dtrace_function_entry(f);

    PyCodeObject *co = f->f_code;
    assert(PyBytes_Check(co->co_code));
    assert(PyBytes_GET_SIZE(co->co_code) <= INT_MAX);
    assert(PyBytes_GET_SIZE(co->co_code) % sizeof(_Py_CODEUNIT) == 0);
    assert(_Py_IS_ALIGNED(PyBytes_AS_STRING(co->co_code), sizeof(_Py_CODEUNIT)));
    PyObject *names = co->co_names;
    assert(PyTuple_CheckExact(names));
    PyObject **fastlocals = f->f_localsplus;
    PyObject **cell_and_free_vars = frame_cells_and_frees(f);

    /*
       f->f_last_instr points to the last instruction,
       unless it's less than first_instr
       in which case next_instr should be first_instr.

       YIELD_FROM sets f_last_instr to itself, in order to repeatedly yield
       multiple values.

       When the PREDICT() macros are enabled, some opcode pairs follow in
       direct succession without updating f->f_last_instr.  A successful
       prediction effectively links the two codes together as if they
       were a single new opcode; accordingly,f->f_last_instr will point to
       the first code in the pair (for instance, GET_ITER followed by
       FOR_ITER is effectively a single opcode and f->f_last_instr will point
       to the beginning of the combined pair.)
    */
    f->f_state = FRAME_EXECUTING;
    _Py_CODEUNIT *const first_instr = frame_first_instr(f);
    _Py_CODEUNIT *next_instr;
    SET_INSTR_POINTER(f->f_last_instr + 1);
    assert(next_instr >= first_instr);

#if ON_DEMAND_OPARG_DECODING
#if USE_COMPUTED_GOTOS
    Opcode opcode;
#else
    int opcode;
#endif
    _Py_CODEUNIT fetched_instr;
    _Py_OPARG ext_oparg1 = 0, ext_oparg2 = 0, ext_oparg3 = 0;
#else
    Opcode opcode;
    _Py_OPARG oparg1 = 0, oparg2 = 0, oparg3 = 0;
#endif

    if (co->co_opcache_flag < opcache_min_runs) {
        co->co_opcache_flag++;
        if (co->co_opcache_flag == opcache_min_runs) {
            if (_PyCode_InitOpcache(co) < 0) {
                goto exit_eval_frame;
            }
#if OPCACHE_STATS
            opcache_code_objects_extra_mem +=
                PyBytes_Size(co->co_code) / sizeof(_Py_CODEUNIT) +
                sizeof(_PyOpcache) * co->co_opcache_size;
            opcache_code_objects++;
#endif
        }
    }

#ifdef LLTRACE
    {
        int r = _PyDict_ContainsId(f->f_globals, &PyId___ltrace__);
        if (r < 0) {
            goto exit_eval_frame;
        }
        lltrace = r;
    }
#endif

    if (throwflag) { /* support for generator.throw() */
        goto error;
    }

#ifdef Py_DEBUG
    /* _PyEval_EvalFrameDefault() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

main_loop:
    assert(!_PyErr_Occurred(tstate));

    /* Do periodic things.  Doing this every time through
       the loop would add too much overhead, so we do it
       only every Nth instruction.  We also do it if
       ``pending.calls_to_do'' is set, i.e. when an asynchronous
       event needs attention (e.g. a signal handler or
       async I/O handler); see Py_AddPendingCall() and
       Py_MakePendingCalls() above. */

    if (_Py_atomic_load_relaxed(eval_breaker)) {
        Opcode nextop = _Py_OPCODE(*next_instr);
        if (nextop != START_TRY
            && nextop != ENTER_WITH
            && nextop != YIELD_FROM) {
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

tracing_dispatch:
    {
        _Py_CODEUNIT *prev_instr = f->f_last_instr;
        f->f_last_instr = next_instr;

        if (PyDTrace_LINE_ENABLED())
            maybe_dtrace_line(f, &trace_info, prev_instr);

        /* line-by-line tracing support */

        if (trace_info.cframe.use_tracing &&
            tstate->c_tracefunc != NULL && !tstate->tracing) {
            int err;
            /* see maybe_call_line_trace() for expository comments */
            err = maybe_call_line_trace(tstate->c_tracefunc,
                                        tstate->c_traceobj,
                                        tstate, f,
                                        &trace_info, prev_instr);
            /* Reload possibly changed frame fields */
            SET_INSTR_POINTER(f->f_last_instr);
            if (err) {
                /* trace function raised an exception */
                goto error;
            }
        }
        FETCH_NEXT_INSTR();
    }

#ifdef LLTRACE
    if (lltrace) {
        /* No need to consider EXTENDED_ARG since it never jumps here. */
#if ON_DEMAND_OPARG_DECODING
        _Py_OPARG oparg1 = _Py_OPARG1(fetched_instr);
        _Py_OPARG oparg2 = _Py_OPARG2(fetched_instr);
        _Py_OPARG oparg3 = _Py_OPARG3(fetched_instr);
#endif
        printf("%d: %s %"_Py_OPARG_PRI" %"_Py_OPARG_PRI" %"_Py_OPARG_PRI"\n",
               frame_instr_offset(f),
               opcode_names[opcode], oparg1, oparg2, oparg3);
    }
#endif

#if USE_COMPUTED_GOTOS == 0
    goto dispatch_opcode;

predispatch:
    if (trace_info.cframe.use_tracing OR_DTRACE_LINE OR_LLTRACE) {
        goto tracing_dispatch;
    }
    f->f_last_instr = next_instr;
    FETCH_NEXT_INSTR();
dispatch_opcode:
#endif

#ifdef DYNAMIC_EXECUTION_PROFILE
    {
        int op = opcode;
#if ON_DEMAND_OPARG_DECODING && !USE_COMPUTED_GOTOS
        op = op & 0xff;
#endif
        dxp[op]++;
#ifdef DXPAIRS
        if (f->f_last_instr >= first_instr) {
            dxpairs[_Py_OPCODE(*f->f_last_instr)][op]++;
        }
#endif
    }
#endif

    /* BEWARE!
       It is essential that any operation that fails must goto error
       and that all operation that succeed call DISPATCH() ! */
    SWITCH_OP() {
        CASE_OP(NOP, ARG_NO, ARG_NO, ARG_NO)
            DISPATCH();
        END_CASE_OP

        CASE_OP(EXTENDED_ARG, ARG_YES, ARG_YES, ARG_YES)
#if ON_DEMAND_OPARG_DECODING
            FETCH_NEXT_INSTR();
            ext_oparg1 = oparg1 << 8 | _Py_OPARG1(fetched_instr);
            ext_oparg2 = oparg2 << 8 | _Py_OPARG2(fetched_instr);
            ext_oparg3 = oparg3 << 8 | _Py_OPARG3(fetched_instr);
#if USE_COMPUTED_GOTOS
            goto *ext_opcode_targets[opcode];
#else
            opcode += 0x100;
            goto dispatch_opcode;
#endif
#else
            _Py_OPARG old_oparg1 = oparg1;
            _Py_OPARG old_oparg2 = oparg2;
            _Py_OPARG old_oparg3 = oparg3;
            FETCH_NEXT_INSTR();
            oparg1 |= old_oparg1 << 8;
            oparg2 |= old_oparg2 << 8;
            oparg3 |= old_oparg3 << 8;
#if USE_COMPUTED_GOTOS
            goto *opcode_targets[opcode];
#else
            goto dispatch_opcode;
#endif
#endif
        END_CASE_OP

        CASE_OP(MOVE_FAST, ARG_YES, ARG_NO, ARG_YES, USE_PREDICT)
            PyObject *value = REG(oparg1);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg1);
                goto error;
            }
            Py_INCREF(value);
            SET_REG(oparg3, value);
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_TWO_FAST, ARG_YES, ARG_YES, ARG_YES)
            PyObject *value = REG(oparg1);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg1);
                goto error;
            }
            Py_INCREF(value);
            SET_REG(oparg3, value);
            value = REG(oparg2);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg2);
                goto error;
            }
            Py_INCREF(value);
            SET_REG(oparg3 + 1, value);
            DISPATCH();
        END_CASE_OP

        CASE_OP(STORE_TWO_FAST, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *value = REG(oparg3);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg3);
                goto error;
            }
            Py_INCREF(value);
            SET_REG(oparg1, value);
            value = REG(oparg3 + 1);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg3 + 1);
                goto error;
            }
            Py_INCREF(value);
            SET_REG(oparg2, value);
            DISPATCH();
        END_CASE_OP

        CASE_OP(DELETE_FAST, ARG_YES, ARG_YES, ARG_NO)
            PyObject *v = REG(oparg1);
            if (v == Py_Undefined && !oparg2) {
                format_exc_unbound_fast(tstate, co, oparg1);
                goto error;
            }
            SET_REG(oparg1, Py_Undefined);
            PREDICT(END_FINALLY);
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_DEREF, ARG_YES, ARG_NO, ARG_YES)
            PyObject *cell = cell_and_free_vars[oparg1];
            PyObject *value = PyCell_GET(cell);
            if (value == NULL) {
                format_exc_unbound_deref(tstate, co, oparg1);
                goto error;
            }
            Py_INCREF(value);
            SET_REG(oparg3, value);
            DISPATCH();
        END_CASE_OP

        CASE_OP(STORE_DEREF, ARG_YES, ARG_NO, ARG_YES)
            PyObject *cell = cell_and_free_vars[oparg1];
            PyObject *value = REG(oparg3);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg3);
                goto error;
            }
            PyObject *oldobj = PyCell_GET(cell);
            Py_INCREF(value);
            PyCell_SET(cell, value);
            Py_XDECREF(oldobj);
            DISPATCH();
        END_CASE_OP

        CASE_OP(DELETE_DEREF, ARG_YES, ARG_YES, ARG_NO)
            PyObject *cell = cell_and_free_vars[oparg1];
            PyObject *oldobj = PyCell_GET(cell);
            if (oldobj == NULL && !oparg2) {
                format_exc_unbound_deref(tstate, co, oparg1);
                goto error;
            }
            PyCell_SET(cell, NULL);
            Py_DECREF(oldobj);
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_CLASSDEREF, ARG_YES, ARG_NO, ARG_YES)
            assert(oparg1 >= frame_cell_num(co));
            Py_ssize_t idx = oparg1 - frame_cell_num(co);
            assert(idx < frame_free_num(co));
            PyObject *name = PyTuple_GET_ITEM(co->co_freevars, idx);
            PyObject *value;
            assert(f->f_locals);
            if (PyDict_CheckExact(f->f_locals)) {
                value = PyDict_GetItemWithError(f->f_locals, name);
                if (value != NULL) {
                    Py_INCREF(value);
                } else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            } else {
                value = PyObject_GetItem(f->f_locals, name);
                if (value == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                }
            }
            if (!value) {
                PyObject *cell = cell_and_free_vars[oparg1];
                value = PyCell_GET(cell);
                if (value == NULL) {
                    format_exc_unbound_deref(tstate, co, oparg1);
                    goto error;
                }
                Py_INCREF(value);
            }
            SET_REG(oparg3, value);
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_GLOBAL, ARG_YES, ARG_NO, ARG_YES)
            PyObject *value;
            if (PyDict_CheckExact(f->f_globals) && PyDict_CheckExact(f->f_builtins)) {
                OPCACHE_CHECK();
                if (co_opcache != NULL && co_opcache->optimized > 0) {
                    _PyOpcache_LoadGlobal *lg = &co_opcache->u.lg;

                    if (lg->globals_ver ==
                            ((PyDictObject *)f->f_globals)->ma_version_tag
                        && lg->builtins_ver ==
                           ((PyDictObject *)f->f_builtins)->ma_version_tag) {
                        PyObject *ptr = lg->ptr;
                        OPCACHE_STAT_GLOBAL_HIT();
                        assert(ptr != NULL);
                        Py_INCREF(ptr);
                        SET_REG(oparg3, ptr);
                        DISPATCH();
                    }
                }

                PyObject *name = GET_NAME(oparg1);
                value = _PyDict_LoadGlobal((PyDictObject *)f->f_globals,
                                       (PyDictObject *)f->f_builtins,
                                       name);
                if (value == NULL) {
                    /* _PyDict_LoadGlobal() returns NULL without raising
                     * an exception if the key doesn't exist */
                    format_exc_name_error(tstate, name, false);
                    goto error;
                }
                Py_INCREF(value);

                if (co_opcache != NULL) {
                    _PyOpcache_LoadGlobal *lg = &co_opcache->u.lg;

                    if (co_opcache->optimized == 0) {
                        /* Wasn't optimized before. */
                        OPCACHE_STAT_GLOBAL_OPT();
                    } else {
                        OPCACHE_STAT_GLOBAL_MISS();
                    }

                    co_opcache->optimized = 1;
                    lg->globals_ver =
                        ((PyDictObject *)f->f_globals)->ma_version_tag;
                    lg->builtins_ver =
                        ((PyDictObject *)f->f_builtins)->ma_version_tag;
                    lg->ptr = value; /* borrowed */
                }
            } else {
                /* Slow-path if globals or builtins is not a dict */

                /* namespace 1: globals */
                PyObject *name = GET_NAME(oparg1);
                value = PyObject_GetItem(f->f_globals, name);
                if (value == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);

                    /* namespace 2: builtins */
                    value = PyObject_GetItem(f->f_builtins, name);
                    if (value == NULL) {
                        format_exc_name_error(tstate, name, true);
                        goto error;
                    }
                }
            }
            SET_REG(oparg3, value);
            DISPATCH();
        END_CASE_OP

        CASE_OP(STORE_GLOBAL, ARG_YES, ARG_NO, ARG_YES)
            PyObject *name = GET_NAME(oparg1);
            PyObject *value = REG(oparg3);
            int err = PyDict_SetItem(f->f_globals, name, value);
            if (err != 0)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(DELETE_GLOBAL, ARG_YES, ARG_YES, ARG_NO)
            PyObject *name = GET_NAME(oparg1);
            int err = PyDict_DelItem(f->f_globals, name);
            if (err != 0) {
                if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                    goto error;
                if (!oparg2) {
                    format_exc_name_error(tstate, name, true);
                    goto error;
                }
                _PyErr_Clear(tstate);
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_NAME, ARG_YES, ARG_NO, ARG_YES)
            PyObject *name = GET_NAME(oparg1);
            PyObject *locals = f->f_locals;
            PyObject *value;
            if (locals == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when loading %R", name);
                goto error;
            }
            if (PyDict_CheckExact(locals)) {
                value = PyDict_GetItemWithError(locals, name);
                if (value != NULL) {
                    Py_INCREF(value);
                } else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            } else {
                value = PyObject_GetItem(f->f_locals, name);
                if (value == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                }
            }
            if (value == NULL) {
                value = PyDict_GetItemWithError(f->f_globals, name);
                if (value != NULL) {
                    Py_INCREF(value);
                } else if (_PyErr_Occurred(tstate)) {
                    goto error;
                } else {
                    if (PyDict_CheckExact(f->f_builtins)) {
                        value = PyDict_GetItemWithError(f->f_builtins, name);
                        if (value == NULL) {
                            format_exc_name_error(tstate, name, false);
                            goto error;
                        }
                        Py_INCREF(value);
                    } else {
                        value = PyObject_GetItem(f->f_builtins, name);
                        if (value == NULL) {
                            format_exc_name_error(tstate, name, true);
                            goto error;
                        }
                    }
                }
            }
            SET_REG(oparg3, value);
            DISPATCH();
        END_CASE_OP

        CASE_OP(STORE_NAME, ARG_YES, ARG_NO, ARG_YES, USE_PREDICT)
            PyObject *name = GET_NAME(oparg1);
            PyObject *value = REG(oparg3);
            PyObject *ns = f->f_locals;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when storing %R", name);
                goto error;
            }
            int err;
            if (PyDict_CheckExact(ns)) {
                err = PyDict_SetItem(ns, name, value);
            } else {
                err = PyObject_SetItem(ns, name, value);
            }
            if (err != 0)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(DELETE_NAME, ARG_YES, ARG_YES, ARG_NO)
            PyObject *name = GET_NAME(oparg1);
            if (f->f_locals == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when deleting %R", name);
                goto error;
            }
            int err = PyObject_DelItem(f->f_locals, name);
            if (err != 0) {
                if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                    goto error;
                if (!oparg2) {
                    format_exc_name_error(tstate, name, true);
                    goto error;
                }
                _PyErr_Clear(tstate);
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_ATTR, ARG_YES, ARG_YES, ARG_YES)
            PyObject *owner = REG(oparg1);
            PyTypeObject *type = Py_TYPE(owner);
            PyObject *name = GET_NAME(oparg2);
            PyObject *res;

            OPCACHE_STAT_ATTR_TOTAL();

            OPCACHE_CHECK();
            if (co_opcache != NULL && PyType_HasFeature(type, Py_TPFLAGS_VALID_VERSION_TAG)) {
                if (co_opcache->optimized > 0) {
                    // Fast path -- cache hit makes LOAD_ATTR ~30% faster.
                    _PyOpCodeOpt_LoadAttr *la = &co_opcache->u.la;
                    if (la->type == type && la->tp_version_tag == type->tp_version_tag) {
                        // Hint >= 0 is a dict index; hint == -1 is a dict miss.
                        // Hint < -1 is an inverted slot offset: offset is strictly > 0,
                        // so ~offset is strictly < -1 (assuming 2's complement).
                        if (la->hint < -1) {
                            // Even faster path -- slot hint.
                            Py_ssize_t offset = ~la->hint;
                            // fprintf(stderr, "Using hint for offset %zd\n", offset);
                            char *addr = (char *)owner + offset;
                            res = *(PyObject **)addr;
                            if (res != NULL) {
                                Py_INCREF(res);
                                SET_REG(oparg3, res);
                                DISPATCH();
                            }
                            // Else slot is NULL.  Fall through to slow path to raise AttributeError(name).
                            // Don't DEOPT, since the slot is still there.
                        } else {
                            // Fast path for dict.
                            assert(type->tp_dict != NULL);
                            assert(type->tp_dictoffset > 0);

                            PyObject *dict = *((PyObject **) ((char *)owner + type->tp_dictoffset));
                            if (dict != NULL && PyDict_CheckExact(dict)) {
                                Py_ssize_t hint = la->hint;
                                Py_INCREF(dict);
                                res = NULL;
                                assert(!_PyErr_Occurred(tstate));
                                la->hint = _PyDict_GetItemHint((PyDictObject*)dict, name, hint, &res);
                                if (res != NULL) {
                                    assert(la->hint >= 0);
                                    if (la->hint == hint && hint >= 0) {
                                        // Our hint has helped -- cache hit.
                                        OPCACHE_STAT_ATTR_HIT();
                                    } else {
                                        // The hint we provided didn't work.
                                        // Maybe next time?
                                        OPCACHE_MAYBE_DEOPT_LOAD_ATTR();
                                    }

                                    Py_INCREF(res);
                                    SET_REG(oparg3, res);
                                    Py_DECREF(dict);
                                    DISPATCH();
                                } else {
                                    _PyErr_Clear(tstate);
                                    // This attribute can be missing sometimes;
                                    // we don't want to optimize this lookup.
                                    OPCACHE_DEOPT_LOAD_ATTR();
                                    Py_DECREF(dict);
                                }
                            } else {
                                // There is no dict, or __dict__ doesn't satisfy PyDict_CheckExact.
                                OPCACHE_DEOPT_LOAD_ATTR();
                            }
                        }
                    } else {
                        // The type of the object has either been updated,
                        // or is different.  Maybe it will stabilize?
                        OPCACHE_MAYBE_DEOPT_LOAD_ATTR();
                    }
                    OPCACHE_STAT_ATTR_MISS();
                }

                // co_opcache can be NULL after a DEOPT() call.
                if (co_opcache != NULL && type->tp_getattro == PyObject_GenericGetAttr) {
                    if (type->tp_dict == NULL) {
                        if (PyType_Ready(type) < 0) {
                            goto error;
                        }
                    }
                    PyObject *descr = _PyType_Lookup(type, name);
                    if (descr != NULL) {
                        // We found an attribute with a data-like descriptor.
                        PyTypeObject *dtype = Py_TYPE(descr);
                        if (dtype == &PyMemberDescr_Type) {  // It's a slot
                            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
                            struct PyMemberDef *dmem = member->d_member;
                            if (dmem->type == T_OBJECT_EX) {
                                Py_ssize_t offset = dmem->offset;
                                assert(offset > 0);  // 0 would be confused with dict hint == -1 (miss).

                                if (co_opcache->optimized == 0) {
                                    // First time we optimize this opcode.
                                    OPCACHE_STAT_ATTR_OPT();
                                    co_opcache->optimized = OPCODE_CACHE_MAX_TRIES;
                                    // fprintf(stderr, "Setting hint for %s, offset %zd\n", dmem->name, offset);
                                }

                                _PyOpCodeOpt_LoadAttr *la = &co_opcache->u.la;
                                la->type = type;
                                la->tp_version_tag = type->tp_version_tag;
                                la->hint = ~offset;

                                char *addr = (char *)owner + offset;
                                res = *(PyObject **)addr;
                                if (res != NULL) {
                                    Py_INCREF(res);
                                    SET_REG(oparg3, res);
                                    DISPATCH();
                                }
                                // Else slot is NULL.  Fall through to slow path to raise AttributeError(name).
                            }
                            // Else it's a slot of a different type.  We don't handle those.
                        }
                        // Else it's some other kind of descriptor that we don't handle.
                        OPCACHE_DEOPT_LOAD_ATTR();
                    }  else if (type->tp_dictoffset > 0) {
                        // We found an instance with a __dict__.
                        PyObject *dict = *((PyObject **) ((char *)owner + type->tp_dictoffset));

                        if (dict != NULL && PyDict_CheckExact(dict)) {
                            Py_INCREF(dict);
                            res = NULL;
                            assert(!_PyErr_Occurred(tstate));
                            Py_ssize_t hint = _PyDict_GetItemHint((PyDictObject*)dict, name, -1, &res);
                            if (res != NULL) {
                                Py_INCREF(res);
                                Py_DECREF(dict);
                                SET_REG(oparg3, res);

                                if (co_opcache->optimized == 0) {
                                    // First time we optimize this opcode.
                                    OPCACHE_STAT_ATTR_OPT();
                                    co_opcache->optimized = OPCODE_CACHE_MAX_TRIES;
                                }

                                _PyOpCodeOpt_LoadAttr *la = &co_opcache->u.la;
                                la->type = type;
                                la->tp_version_tag = type->tp_version_tag;
                                assert(hint >= 0);
                                la->hint = hint;

                                DISPATCH();
                            } else {
                                _PyErr_Clear(tstate);
                            }
                            Py_DECREF(dict);
                        } else {
                            // There is no dict, or __dict__ doesn't satisfy PyDict_CheckExact.
                            OPCACHE_DEOPT_LOAD_ATTR();
                        }
                    } else {
                        // The object's class does not have a tp_dictoffset we can use.
                        OPCACHE_DEOPT_LOAD_ATTR();
                    }
                } else if (type->tp_getattro != PyObject_GenericGetAttr) {
                    OPCACHE_DEOPT_LOAD_ATTR();
                }
            }

            // Slow path.
            res = PyObject_GetAttr(owner, name);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(STORE_ATTR, ARG_YES, ARG_YES, ARG_YES)
            PyObject *owner = REG(oparg1);
            PyObject *name = GET_NAME(oparg2);
            PyObject *value = REG(oparg3);
            int err = PyObject_SetAttr(owner, name, value);
            if (err != 0)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(DELETE_ATTR, ARG_YES, ARG_YES, ARG_NO)
            PyObject *owner = REG(oparg1);
            PyObject *name = GET_NAME(oparg2);
            int err = PyObject_SetAttr(owner, name, NULL);
            if (err != 0)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_METHOD, ARG_YES, ARG_YES, ARG_YES)
            PyObject *owner = REG(oparg1);
            PyObject *name = GET_NAME(oparg2);
            PyObject *meth = NULL;
            int meth_found = _PyObject_GetMethod(owner, name, &meth);
            if (meth == NULL) {
                /* Most likely attribute wasn't found. */
                goto error;
            }
            if (meth_found) {
                /* We can bypass temporary bound method object.
                   meth is unbound method and obj is self.
                   meth | self | arg1 | ... | argN */
                Py_INCREF(owner);
                SET_REG(oparg3, meth);
                SET_REG(oparg3 + 1, owner);
            } else {
                /* meth is not an unbound method (but a regular attr,
                   or something was returned by a descriptor protocol).
                   The first element is set to Py_Undefined, to signal
                   CALL_METHOD(_EX) that it's not a method call.
                   Py_Undefined | meth | arg1 | ... | argN */
                SET_REG(oparg3, Py_Undefined);
                SET_REG(oparg3 + 1, meth);
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(LOAD_SUBSCR, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *container = REG(oparg1);
            PyObject *sub = REG(oparg2);
            PyObject *res = PyObject_GetItem(container, sub);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(STORE_SUBSCR, ARG_YES, ARG_YES, ARG_YES)
            PyObject *container = REG(oparg1);
            PyObject *sub = REG(oparg2);
            PyObject *value = REG(oparg3);
            int err = PyObject_SetItem(container, sub, value);
            if (err != 0)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(DELETE_SUBSCR, ARG_YES, ARG_YES, ARG_NO)
            PyObject *container = REG(oparg1);
            PyObject *sub = REG(oparg2);
            int err = PyObject_DelItem(container, sub);
            if (err != 0)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(CALL_FUNCTION, ARG_YES, ARG_YES, ARG_YES)
            PyObject *func = REG(oparg2);
            PyObject **args = PREG(oparg2 + 1);
            Py_ssize_t nargs = oparg1;
            PyObject *res = call_function(tstate, &trace_info, func, args, nargs, NULL);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            CHECK_EVAL_BREAKER();
            DISPATCH();
        END_CASE_OP

        CASE_OP(CALL_METHOD, ARG_YES, ARG_YES, ARG_YES)
            PyObject **func_args = PREG(oparg2);
            Py_ssize_t nargs = oparg1;
            Py_ssize_t is_method_call = *func_args != Py_Undefined;
            func_args += !is_method_call;
            nargs += is_method_call;
            PyObject *func = *func_args;
            PyObject **args = func_args + 1;
            PyObject *res = call_function(tstate, &trace_info, func, args, nargs, NULL);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            CHECK_EVAL_BREAKER();
            DISPATCH();
        END_CASE_OP

        CASE_OP(CALL_FUNCTION_KW, ARG_YES, ARG_YES, ARG_YES)
            PyObject *func = REG(oparg2);
            PyObject **args = PREG(oparg2 + 1);
            PyObject *kwnames = REG(oparg2 + 1 + oparg1);
            Py_ssize_t nargs = oparg1 - PyTuple_GET_SIZE(kwnames);
            PyObject *res = call_function(tstate, &trace_info, func, args, nargs, kwnames);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            CHECK_EVAL_BREAKER();
            DISPATCH();
        END_CASE_OP

        CASE_OP(CALL_METHOD_KW, ARG_YES, ARG_YES, ARG_YES)
            PyObject **func_args = PREG(oparg2);
            Py_ssize_t nargs = oparg1;
            PyObject *kwnames = REG(oparg2 + 2 + oparg1);
            Py_ssize_t is_method_call = *func_args != Py_Undefined;
            func_args += !is_method_call;
            nargs += is_method_call - PyTuple_GET_SIZE(kwnames);
            PyObject *func = *func_args;
            PyObject **args = func_args + 1;
            PyObject *res = call_function(tstate, &trace_info, func, args, nargs, kwnames);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            CHECK_EVAL_BREAKER();
            DISPATCH();
        END_CASE_OP

        CASE_OP(CALL_FUNCTION_EX, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *func = REG(oparg2);
            PyObject *args = REG(oparg2 + 1);
            PyObject *kwargs = oparg1 ? REG(oparg2 + 2) : NULL;
            PyObject *res = do_call_core(tstate, &trace_info, func, args, kwargs);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            CHECK_EVAL_BREAKER();
            DISPATCH();
        END_CASE_OP

        CASE_OP(UNARY_INVERT, ARG_YES, ARG_NO, ARG_YES)
            PyObject *value = REG(oparg1);
            PyObject *res = PyNumber_Invert(value);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(BINARY_AND);
            DISPATCH();
        END_CASE_OP

        CASE_OP(UNARY_NOT, ARG_YES, ARG_NO, ARG_YES)
            PyObject *value = REG(oparg1);
            int is_true = PyObject_IsTrue(value);
            if (is_true < 0)
                goto error;
            PyObject *res = is_true ? Py_False : Py_True;
            Py_INCREF(res);
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(UNARY_POSITIVE, ARG_YES, ARG_NO, ARG_YES)
            PyObject *value = REG(oparg1);
            PyObject *res = PyNumber_Positive(value);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(UNARY_NEGATIVE, ARG_YES, ARG_NO, ARG_YES)
            PyObject *value = REG(oparg1);
            PyObject *res = PyNumber_Negative(value);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_ADD, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Add(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_ADD, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceAdd(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_SUBTRACT, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Subtract(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_SUBTRACT, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceSubtract(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_MULTIPLY, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Multiply(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_MULTIPLY, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceMultiply(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_MATRIX_MULTIPLY, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_MatrixMultiply(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_MATRIX_MULTIPLY, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceMatrixMultiply(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_TRUE_DIVIDE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_TrueDivide(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_TRUE_DIVIDE, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceTrueDivide(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_MODULO, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res;
            if (PyUnicode_CheckExact(left) && (
                  !PyUnicode_Check(right) || PyUnicode_CheckExact(right))) {
              // fast path; string formatting, but not if the RHS is a str subclass
              // (see issue28598)
              res = PyUnicode_Format(left, right);
            } else {
              res = PyNumber_Remainder(left, right);
            }
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_MODULO, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceRemainder(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_POWER, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Power(left, right, Py_None);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_POWER, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlacePower(left, right, Py_None);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_LSHIFT, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Lshift(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_LSHIFT, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceLshift(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_RSHIFT, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Rshift(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_RSHIFT, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceRshift(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_OR, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Or(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_OR, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceOr(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_XOR, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_Xor(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_XOR, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceXor(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_AND, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_And(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_AND, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceAnd(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BINARY_FLOOR_DIVIDE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_FloorDivide(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(INPLACE_FLOOR_DIVIDE, ARG_YES, ARG_YES, ARG_NO)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyNumber_InPlaceFloorDivide(left, right);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(COMPARE_LT, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyObject_RichCompare(left, right, Py_LT);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_TRUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(COMPARE_LE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyObject_RichCompare(left, right, Py_LE);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_TRUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(COMPARE_EQ, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyObject_RichCompare(left, right, Py_EQ);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_TRUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(COMPARE_NE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyObject_RichCompare(left, right, Py_NE);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_TRUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(COMPARE_GT, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyObject_RichCompare(left, right, Py_GT);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_TRUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(COMPARE_GE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            PyObject *res = PyObject_RichCompare(left, right, Py_GE);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_TRUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(IS_OP, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            if (left == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg1);
                goto error;
            }
            PyObject *right = REG(oparg2);
            if (right == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg2);
                goto error;
            }
            int res = Py_Is(left, right);
            PyObject *b = res ? Py_True : Py_False;
            Py_INCREF(b);
            SET_REG(oparg3, b);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_FALSE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(CONTAINS_OP, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);
            int res = PySequence_Contains(right, left);
            if (res < 0)
                goto error;
            PyObject *b = res ? Py_True : Py_False;
            Py_INCREF(b);
            SET_REG(oparg3, b);
            PREDICT(JUMP_IF_FALSE);
            PREDICT(JUMP_IF_FALSE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(RETURN_VALUE, ARG_YES, ARG_NO, ARG_NO, USE_PREDICT)
            PyObject *value = REG(oparg1);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg1);
                goto error;
            }
            retval = value;
            Py_INCREF(retval);
            f->f_state = FRAME_RETURNED;
            goto clean_exit;
        END_CASE_OP

        CASE_OP(YIELD_VALUE, ARG_YES, ARG_NO, ARG_YES)
            PyObject *value = REG(oparg1);
            if (value == Py_Undefined) {
                format_exc_unbound_fast(tstate, co, oparg1);
                goto error;
            }
            if (co->co_flags & CO_ASYNC_GENERATOR) {
                retval = _PyAsyncGenValueWrapperNew(value);
                if (retval == NULL) {
                    goto error;
                }
            } else {
                retval = value;
                Py_INCREF(value);
            }
            PyObject **p_arg = PREG(oparg3);
            SET_REG(oparg3, Py_Undefined);
            if (f->f_gen) {
                ((PyGenObject *)f->f_gen)->gi_arg = p_arg;
            }
            f->f_state = FRAME_SUSPENDED;
            goto exiting;
        END_CASE_OP

        CASE_OP(GET_YIELD_FROM_ITER, ARG_YES, ARG_NO, ARG_YES)
            PyObject *value = REG(oparg1);
            PyObject *iterator;
            if (PyCoro_CheckExact(value)) {
                /* `iterable` is a coroutine */
                if (!(co->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))) {
                    /* and it is used in a 'yield from' expression of a
                       regular generator. */
                    _PyErr_SetString(tstate, PyExc_TypeError,
                                     "cannot 'yield from' a coroutine object "
                                     "in a non-coroutine generator");
                    goto error;
                }
                iterator = value;
                Py_INCREF(iterator);
            } else if (PyGen_CheckExact(value)) {
                iterator = value;
                Py_INCREF(iterator);
            } else {
                /* `iterable` is not a generator. */
                iterator = PyObject_GetIter(value);
                if (iterator == NULL) {
                    goto error;
                }
            }
            SET_REG(oparg3, iterator);
            Py_INCREF(Py_None);
            SET_REG(oparg3 + 1, Py_None);
            PREDICT(YIELD_FROM);
            DISPATCH();
        END_CASE_OP

        CASE_OP(YIELD_FROM, ARG_YES, ARG_NO, ARG_YES, USE_PREDICT)
            PyObject *receiver = REG(oparg1);
            PyObject **p_arg = PREG(oparg1 + 1);
            PyObject *arg = *p_arg;
            *p_arg = Py_Undefined; /* set it to Py_Undefined for the stolen reference */
            ((PyGenObject *)f->f_gen)->gi_arg = p_arg;

            PySendResult gen_status;
            if (tstate->c_tracefunc == NULL) {
                gen_status = PyIter_Send(receiver, arg, &retval);
            } else {
                if (PyIter_Check(receiver)) {
                    retval = Py_TYPE(receiver)->tp_iternext(receiver);
                } else {
                    _Py_IDENTIFIER(send);
                    retval = _PyObject_CallMethodIdOneArg(receiver, &PyId_send, arg);
                }
                if (retval == NULL) {
                    if (_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
                        call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, f, &trace_info);
                    }
                    gen_status = _PyGen_FetchStopIterationValue(&retval) ? PYGEN_ERROR : PYGEN_RETURN;
                } else {
                    gen_status = PYGEN_NEXT;
                }
            }
            Py_DECREF(arg);
            if (gen_status == PYGEN_ERROR) {
                assert (retval == NULL);
                goto error;
            }
            if (gen_status == PYGEN_RETURN) {
                assert (retval != NULL);
                SET_REG(oparg3, retval);
                retval = NULL;
                DISPATCH();
            }
            assert (gen_status == PYGEN_NEXT);
            /* receiver at [oparg1] is not touched, retval is value to be yielded,
               and repeat... */
            f->f_last_instr--;
            f->f_state = FRAME_SUSPENDED;
            goto exiting;
        END_CASE_OP

        CASE_OP(RAISE_EXCEPTION, ARG_YES, ARG_YES, ARG_YES)
            bool assert_error = oparg1 & 0b001;
            bool with_exc = oparg1 & 0b010;
            PyObject *exc = NULL;
            PyObject *cause = NULL;
            if (assert_error) {
                if (with_exc) {
                    exc = call_function(tstate, &trace_info, PyExc_AssertionError, PREG(oparg2), 1, NULL);
                    if (!exc) {
                        goto error;
                    }
                } else {
                    exc = PyExc_AssertionError;
                    Py_INCREF(exc);
                }
            } else {
                if (with_exc) {
                    exc = REG(oparg2);
                    Py_INCREF(exc);
                }
                bool with_cause = oparg1 & 0b100;
                if (with_cause) {
                    cause = REG(oparg3);
                    Py_INCREF(cause);
                }
            }
            int res = do_raise(tstate, exc, cause);
            if (!res)
                goto error;
            goto exception_unwind;
        END_CASE_OP

        CASE_OP(GET_CLOSURES, ARG_YES, ARG_YES, ARG_YES)
            if (oparg1) {
                PyObject *cell = cell_and_free_vars[oparg2];
                Py_INCREF(cell);
                SET_REG(oparg3, cell);
            } else {
                // Note: No need to check overflow for PyLong_AsSsize_t here.
                PyObject *indexes = REG(oparg2);
                Py_ssize_t num = PyTuple_GET_SIZE(indexes);
                PyObject *closures = PyTuple_New(num);
                if (!closures) {
                    goto error;
                }
                while (num--) {
                    Py_ssize_t index = PyLong_AsSsize_t(PyTuple_GET_ITEM(indexes, num));
                    PyObject *cell = cell_and_free_vars[index];
                    Py_INCREF(cell);
                    PyTuple_SET_ITEM(closures, num, cell);
                }
                SET_REG(oparg3, closures);
            }
            PREDICT(MAKE_FUNCTION);
            DISPATCH();
        END_CASE_OP

        CASE_OP(MAKE_FUNCTION, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *qualname = REG(oparg2);
            oparg2++;
            PyObject *codeobj = REG(oparg2);
            PyFunctionObject *func = (PyFunctionObject *)
                PyFunction_NewWithQualName(codeobj, f->f_globals, qualname);
            if (func == NULL)
                goto error;

            PyObject *value;
            if (oparg1 & 1) {
                oparg2++;
                value = REG(oparg2);
                assert(PyTuple_CheckExact(value));
                Py_INCREF(value);
                func->func_closure = value;
            }
            if (oparg1 & 0b10) {
                oparg2++;
                value = REG(oparg2);
                assert(PyTuple_CheckExact(value));
                Py_INCREF(value);
                func->func_defaults = value;
            }
            if (oparg1 & 0b100) {
                oparg2++;
                value = REG(oparg2);
                assert(PyDict_CheckExact(value));
                Py_INCREF(value);
                func->func_kwdefaults = value;
            }
            if (oparg1 & 0b1000) {
                oparg2++;
                value = REG(oparg2);
                assert(PyTuple_CheckExact(value));
                Py_INCREF(value);
                func->func_annotations = value;
            }

            SET_REG(oparg3, (PyObject *)func);
            DISPATCH();
        END_CASE_OP

        CASE_OP(SETUP_CLASS, ARG_YES, ARG_YES, ARG_YES)
            _Py_IDENTIFIER(__build_class__);
            PyObject *bc;
            if (PyDict_CheckExact(f->f_builtins)) {
                bc = _PyDict_GetItemIdWithError(f->f_builtins, &PyId___build_class__);
                if (bc == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        _PyErr_SetString(tstate, PyExc_NameError, "__build_class__ not found");
                    }
                    goto error;
                }
                Py_INCREF(bc);
            } else {
                PyObject *build_class_str = _PyUnicode_FromId(&PyId___build_class__);
                if (build_class_str == NULL) {
                    goto error;
                }
                bc = PyObject_GetItem(f->f_builtins, build_class_str);
                if (bc == NULL) {
                    if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                        _PyErr_SetString(tstate, PyExc_NameError, "__build_class__ not found");
                    goto error;
                }
            }

            PyObject *codeobj = REG(oparg2);
            PyObject *qualname = ((PyCodeObject *)codeobj)->co_name;
            PyObject *func = PyFunction_NewWithQualName(codeobj, f->f_globals, qualname);
            if (func == NULL)
                goto error;

            PyObject *closures = REG(oparg1);
            Py_INCREF(closures);
            ((PyFunctionObject *)func)->func_closure = closures;

            SET_REG(oparg3, bc);
            SET_REG(oparg3 + 1, (PyObject *)func);
            Py_INCREF(qualname);
            SET_REG(oparg3 + 2, qualname);
            DISPATCH();
        END_CASE_OP

        CASE_OP(JUMP_ALWAYS, ARG_NO, ARG_YES, ARG_YES, USE_PREDICT)
            ADJUST_INSTR_POINTER(oparg2, oparg3);
            CHECK_EVAL_BREAKER();
            PREDICT(FOR_ITER);
            DISPATCH();
        END_CASE_OP

        CASE_OP(JUMP_IF_FALSE, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *cond = REG(oparg1);
            if (Py_IsTrue(cond)) {
                DISPATCH();
            }
            if (Py_IsFalse(cond)) {
                ADJUST_INSTR_POINTER(oparg2, oparg3);
                CHECK_EVAL_BREAKER();
                DISPATCH();
            }
            int is_true = PyObject_IsTrue(cond);
            if (is_true == 0) {
                ADJUST_INSTR_POINTER(oparg2, oparg3);
                CHECK_EVAL_BREAKER();
            } else if (is_true < 0) {
                goto error;
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(JUMP_IF_TRUE, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *cond = REG(oparg1);
            if (Py_IsFalse(cond)) {
                DISPATCH();
            }
            if (Py_IsTrue(cond)) {
                ADJUST_INSTR_POINTER(oparg2, oparg3);
                CHECK_EVAL_BREAKER();
                DISPATCH();
            }
            int is_true = PyObject_IsTrue(cond);
            if (is_true > 0) {
                ADJUST_INSTR_POINTER(oparg2, oparg3);
                CHECK_EVAL_BREAKER();
            } else if (is_true < 0) {
                goto error;
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(JUMP_IF_NOT_EXC_MATCH, ARG_YES, ARG_YES, ARG_YES)
            PyObject *left = REG(oparg1);
            PyObject *right = REG(oparg2);

            const char *error_msg = "catching classes that do not " \
                                    "inherit from BaseException is not allowed";
            if (PyTuple_Check(right)) {
                Py_ssize_t n = PyTuple_GET_SIZE(right);
                for (Py_ssize_t i = 0; i < n; i++) {
                    PyObject *exc = PyTuple_GET_ITEM(right, i);
                    if (!PyExceptionClass_Check(exc)) {
                        _PyErr_SetString(tstate, PyExc_TypeError, error_msg);
                        goto error;
                    }
                }
            } else {
                if (!PyExceptionClass_Check(right)) {
                    _PyErr_SetString(tstate, PyExc_TypeError, error_msg);
                    goto error;
                }
            }
            int res = PyErr_GivenExceptionMatches(left, right);
            if (res < 0)
                goto error; else if (res == 0) {
                INCREASE_INSTR_POINTER(oparg3);
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(GET_ITER, ARG_YES, ARG_NO, ARG_YES)
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable = REG(oparg1);
            PyObject *iter = PyObject_GetIter(iterable);
            if (iter == NULL)
                goto error;
            SET_REG(oparg3, iter);
            PREDICT(FOR_ITER);
            DISPATCH();
        END_CASE_OP

        CASE_OP(FOR_ITER, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            /* before: [iter]; after: [iter, iter()] *or* [] */
            PyObject *iter = REG(oparg1);
            PyObject *next = (*Py_TYPE(iter)->tp_iternext)(iter);
            if (next != NULL) {
                SET_REG(oparg2, next);
                DISPATCH();
            }
            if (_PyErr_Occurred(tstate)) {
                if (!_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
                    goto error;
                } else if (tstate->c_tracefunc != NULL) {
                    call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, f, &trace_info);
                }
                _PyErr_Clear(tstate);
            }
            /* iterator ended normally */
            INCREASE_INSTR_POINTER(oparg3);
            DISPATCH();
        END_CASE_OP

        CASE_OP(START_TRY, ARG_YES, ARG_NO, ARG_YES)
            PyFrame_BlockPush(f, TRY_BLOCK_READY, GET_INSTR_INDEX() + oparg3, oparg1);
            DISPATCH();
        END_CASE_OP

        CASE_OP(REVOKE_EXCEPT, ARG_NO, ARG_NO, ARG_NO)
            PyTryBlock *b = PyFrame_BlockPop(f);
            if (b->b_type == TRY_BLOCK_RAISED) {
                restore_exc(tstate, PREG(b->b_sextuple), false);
            }
            PREDICT(JUMP_ALWAYS);
            DISPATCH();
        END_CASE_OP

        CASE_OP(CALL_FINALLY, ARG_NO, ARG_NO, ARG_YES)
            PyTryBlock *b = PyFrame_BlockPop(f);
            int handler = b->b_handler;
            PyFrame_BlockPush(f, TRY_BLOCK_PASSED, GET_INSTR_INDEX() + oparg3, 0);
            SET_INSTR_INDEX(handler);
            DISPATCH();
        END_CASE_OP

        CASE_OP(END_FINALLY, ARG_YES, ARG_NO, ARG_NO, USE_PREDICT)
            PyTryBlock *b = PyFrame_BlockPop(f);
            if (b->b_type == TRY_BLOCK_RAISED) {
                restore_exc(tstate, PREG(b->b_sextuple), true);
                if (oparg1) {
                    f->f_last_instr = first_instr + b->b_handler;
                }
                goto exception_unwind;
            }
            assert(b->b_type == TRY_BLOCK_PASSED);
            SET_INSTR_INDEX(b->b_handler);
            PREDICT(RETURN_VALUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(ENTER_WITH, ARG_YES, ARG_YES, ARG_YES)
            _Py_IDENTIFIER(__enter__);
            _Py_IDENTIFIER(__exit__);
            PyObject *mgr = REG(oparg1);
            PyObject *enter = special_lookup(tstate, mgr, &PyId___enter__);
            if (enter == NULL)
                goto error;
            PyObject *exit = special_lookup(tstate, mgr, &PyId___exit__);
            if (exit == NULL) {
                Py_DECREF(enter);
                goto error;
            }
            PyObject *res = _PyObject_CallNoArg(enter);
            Py_DECREF(enter);
            if (res == NULL) {
                Py_DECREF(exit);
                goto error;
            }
            /* Note, exit should be set after res. */
            SET_REG(oparg2, res);
            SET_REG(oparg1, exit);
            PyFrame_BlockPush(f, TRY_BLOCK_READY, GET_INSTR_INDEX() + oparg3, oparg1 + 1);
            DISPATCH();
        END_CASE_OP

        CASE_OP(EXIT_WITH, ARG_YES, ARG_NO, ARG_NO)
            PyTryBlock *b = PyFrame_BlockTop(f);
            PyObject *exit_func = REG(oparg1);
            if (b->b_type == TRY_BLOCK_RAISED) {
                _Py_OPARG sextuple = b->b_sextuple;
                PyObject **args = PREG(sextuple + 3);
                PyObject *res = call_function(tstate, &trace_info, exit_func, args, 3, NULL);
                if (res == NULL) {
                    goto error;
                }
                int is_true = PyObject_IsTrue(res);
                Py_DECREF(res);
                if (is_true < 0) {
                    goto error;
                }
                PyFrame_BlockPop(f);
                if (is_true) {
                    restore_exc(tstate, PREG(b->b_sextuple), false);
                } else {
                    restore_exc(tstate, PREG(b->b_sextuple), true);
                    f->f_last_instr = first_instr + b->b_handler;
                    goto exception_unwind;
                }
            } else {
                PyFrame_BlockPop(f);
                /* NB, the array may be modified,  (see method_vectorcall)
                 * so we do not use static variables, and add an extra NULL. */
                PyObject *nones[4] = {NULL, Py_None, Py_None, Py_None};
                PyObject *res = call_function(tstate, &trace_info, exit_func, &nones[1], 3, NULL);
                if (res == NULL) {
                    goto error;
                }
                Py_DECREF(res);
                if (b->b_type == TRY_BLOCK_PASSED) {
                    SET_INSTR_INDEX(b->b_handler);
                }
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(TUPLE_BUILD, ARG_YES, ARG_YES, ARG_YES)
            PyObject *tuple = PyTuple_New(oparg1);
            if (!tuple) {
                goto error;
            }
            for (Py_ssize_t i = 0; i < oparg1; i++) {
                PyObject *value = REG(oparg2 + i);
                Py_INCREF(value);
                PyTuple_SET_ITEM(tuple, i, value);
            }
            SET_REG(oparg3, tuple);
            DISPATCH();
        END_CASE_OP

        CASE_OP(TUPLE_FROM_LIST, ARG_YES, ARG_NO, ARG_YES)
            PyObject *list = REG(oparg1);
            PyObject *tuple = PyList_AsTuple(list);
            if (tuple == NULL) {
                goto error;
            }
            SET_REG(oparg3, tuple);
            DISPATCH();
        END_CASE_OP

        CASE_OP(LIST_BUILD, ARG_YES, ARG_YES, ARG_YES)
            PyObject *list = PyList_New(oparg1);
            if (!list) {
                goto error;
            }
            for (Py_ssize_t i = 0; i < oparg1; i++) {
                PyObject *value = REG(oparg2 + i);
                Py_INCREF(value);
                PyList_SET_ITEM(list, i, value);
            }
            SET_REG(oparg3, list);
            DISPATCH();
        END_CASE_OP

        CASE_OP(LIST_UPDATE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *value = REG(oparg2);
            PyObject *list = REG(oparg3);
            if (oparg1) {
                PyObject *none_val = _PyList_Extend((PyListObject *)list, value);
                if (none_val == NULL) {
                    if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
                        Py_TYPE(value)->tp_iter == NULL &&
                        !PySequence_Check(value)){
                        _PyErr_Clear(tstate);
                        _PyErr_Format(tstate, PyExc_TypeError,
                                      "value after * must be an iterable, not %.200s",
                                      Py_TYPE(value)->tp_name);
                    }
                    goto error;
                }
                Py_DECREF(none_val);
            } else {
                int err = PyList_Append(list, value);
                if (err != 0) {
                    goto error;
                }
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(SET_BUILD, ARG_YES, ARG_YES, ARG_YES)
            PyObject *set = PySet_New(NULL);
            if (!set) {
                goto error;
            }
            for (Py_ssize_t i = 0; i < oparg1; i++) {
                PyObject *value = REG(oparg2 + i);
                int err = PySet_Add(set, value);
                if (err < 0) {
                    Py_DECREF(set);
                    goto error;
                }
            }
            SET_REG(oparg3, set);
            DISPATCH();
        END_CASE_OP

        CASE_OP(SET_UPDATE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *value = REG(oparg2);
            PyObject *set = REG(oparg3);
            if (oparg1) {
                int err = _PySet_Update(set, value);
                if (err < 0) {
                    goto error;
                }
            } else {
                int err = PySet_Add(set, value);
                if (err < 0) {
                    goto error;
                }
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(DICT_BUILD, ARG_YES, ARG_YES, ARG_YES)
            PyObject *dict = _PyDict_NewPresized(oparg1);
            if (!dict) {
                goto error;
            }
            while (oparg1--) {
                PyObject *key = REG(oparg2++);
                PyObject *value = REG(oparg2++);
                int err = PyDict_SetItem(dict, key, value);
                if (err != 0) {
                    Py_DECREF(dict);
                    goto error;
                }
            }
            SET_REG(oparg3, dict);
            DISPATCH();
        END_CASE_OP

        CASE_OP(DICT_WITH_CONST_KEYS, ARG_YES, ARG_YES, ARG_YES)
            PyObject *key_tuple = REG(oparg1);
            Py_ssize_t size = PyTuple_GET_SIZE(key_tuple);
            PyObject *dict = _PyDict_NewPresized(size);
            if (!dict) {
                goto error;
            }
            PyObject **keys = &PyTuple_GET_ITEM(key_tuple, 0);
            PyObject **values = PREG(oparg2);
            for (Py_ssize_t i = 0; i < size; i++) {
                int err = PyDict_SetItem(dict, keys[i], values[i]);
                if (err != 0) {
                    Py_DECREF(dict);
                    goto error;
                }
            }
            SET_REG(oparg3, dict);
            DISPATCH();
        END_CASE_OP

        CASE_OP(DICT_INSERT, ARG_YES, ARG_YES, ARG_YES)
            PyObject *key = REG(oparg1);
            PyObject *value = REG(oparg2);
            PyObject *dict = REG(oparg3);
            int err = PyDict_SetItem(dict, key, value);
            if (err != 0) {
                goto error;
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(DICT_UPDATE, ARG_YES, ARG_NO, ARG_YES)
            PyObject *unpacked = REG(oparg1);
            PyObject *dict = REG(oparg3);
            int res = PyDict_Update(dict, unpacked);
            if (res < 0) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object is not a mapping",
                                  Py_TYPE(unpacked)->tp_name);
                }
                goto error;
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(DICT_MERGE, ARG_YES, ARG_NO, ARG_YES)
            PyObject *unpacked = REG(oparg1);
            PyObject *dict = REG(oparg3);
            int res = _PyDict_MergeEx(dict, unpacked, 2);
            if (res < 0) {
                format_kwargs_error(tstate, REG(oparg3 - 2), unpacked);
                goto error;
            }
            DISPATCH();
            PREDICT(CALL_FUNCTION_EX);
            DISPATCH();
        END_CASE_OP

        CASE_OP(UNPACK_SEQUENCE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *iterable = REG(oparg2);
            if (PyTuple_CheckExact(iterable) && PyTuple_GET_SIZE(iterable) == oparg1) {
                PyObject **items = &PyTuple_GET_ITEM(iterable, 0);
                while (oparg1--) {
                    PyObject *item = items[oparg1];
                    Py_INCREF(item);
                    SET_REG(oparg3 + oparg1, item);
                }
            } else if (PyList_CheckExact(iterable) && PyList_GET_SIZE(iterable) == oparg1) {
                PyObject **items = &PyList_GET_ITEM(iterable, 0);
                while (oparg1--) {
                    PyObject *item = items[oparg1];
                    Py_INCREF(item);
                    SET_REG(oparg3 + oparg1, item);
                }
            } else {
                PyObject **outputs = PREG(oparg3);
                int res = unpack_iterable(tstate, iterable, oparg1, 0, outputs);
                if (!res) {
                    goto error;
                }
            }
            PREDICT(STORE_TWO_FAST);
            DISPATCH();
        END_CASE_OP

        CASE_OP(UNPACK_EX, ARG_YES, ARG_YES, ARG_YES)
            _Py_OPARG before = 0;
            _Py_OPARG after = 0;
            do {
                before = (before << 4) | (oparg1 & 0b1111);
                oparg1 >>= 4;
                after = (after << 4) | (oparg1 & 0b1111);
                oparg1 >>= 4;
            } while (oparg1);
            PyObject *iterable = REG(oparg2);
            PyObject **outputs = PREG(oparg3);
            int res = unpack_iterable(tstate, iterable, before, after + 1, outputs);
            if (!res)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(STRINGIFY_VALUE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *value = REG(oparg2);
            PyObject *result;
            switch (oparg1) {
            case 0:
                result = PyObject_Str(value);
                break;
            case 1:
                result = PyObject_Repr(value);
                break;
            case 2:
                result = PyObject_ASCII(value);
                break;
            default:
                _PyErr_Format(tstate, PyExc_SystemError,
                              "unexpected conversion flag %d",
                              oparg1);
                goto error;
            }
            SET_REG(oparg3, result);
            PREDICT(FORMAT_VALUE);
            DISPATCH();
        END_CASE_OP

        CASE_OP(FORMAT_VALUE, ARG_YES, ARG_YES, ARG_YES, USE_PREDICT)
            PyObject *value = REG(oparg1);
            PyObject *spec = REG(oparg2);
            PyObject *result = PyObject_Format(value, spec);
            if (result == NULL)
                goto error;
            SET_REG(oparg3, result);
            PREDICT(MOVE_FAST);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BUILD_STRING, ARG_YES, ARG_YES, ARG_YES)
            static PyObject *empty;
            if (empty == NULL) {
                if (!(empty = PyUnicode_New(0, 0))) {
                    goto error;
                }
            }
            PyObject **values;
            if (oparg1) {
                values = PREG(oparg2);
            } else {
                PyObject *tuple = REG(oparg2);
                values = &PyTuple_GET_ITEM(tuple, 0);
                oparg1 = PyTuple_GET_SIZE(tuple);
            }
            PyObject *result = _PyUnicode_JoinArray(empty, values, oparg1);
            if (result == NULL)
                goto error;
            SET_REG(oparg3, result);
            DISPATCH();
        END_CASE_OP

        CASE_OP(BUILD_SLICE, ARG_YES, ARG_YES, ARG_YES)
            PyObject *start = Py_None;
            PyObject *stop = Py_None;
            PyObject *step = Py_None;
            if (oparg1 & 0b1) {
                start = REG(oparg2);
                oparg2++;
            }
            if (oparg1 & 0b10) {
                stop = REG(oparg2);
                oparg2++;
            }
            if (oparg1 & 0b100) {
                step = REG(oparg2);
            }
            PyObject *slice = PySlice_New(start, stop, step);
            if (slice == NULL)
                goto error;
            SET_REG(oparg3, slice);
            PREDICT(LOAD_SUBSCR);
            DISPATCH();
        END_CASE_OP

        CASE_OP(IMPORT_NAME, ARG_YES, ARG_YES, ARG_YES)
            PyObject *name = GET_NAME(oparg1);
            PyObject *fromlist = REG(oparg2);
            PyObject *res = import_name(tstate, f, name, fromlist);
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(IMPORT_FROM, ARG_YES, ARG_YES, ARG_YES)
            PyObject *res = import_from(tstate, REG(oparg1), GET_NAME(oparg2));
            if (res == NULL)
                goto error;
            SET_REG(oparg3, res);
            PREDICT(STORE_NAME);
            DISPATCH();
        END_CASE_OP

        CASE_OP(IMPORT_STAR, ARG_YES, ARG_NO, ARG_NO)
            if (PyFrame_FastToLocalsWithError(f) < 0)
                goto error;
            if (f->f_locals == NULL) {
                _PyErr_SetString(tstate, PyExc_SystemError, "no locals found during 'import *'");
                goto error;
            }

            int err = import_all_from(tstate, f->f_locals, REG(oparg1));
            PyFrame_LocalsToFast(f, 0);
            if (err != 0)
                goto error;
            DISPATCH();
        END_CASE_OP

        CASE_OP(GEN_START, ARG_YES, ARG_NO, ARG_NO)
            PyObject *arg = (PyObject *)((PyGenObject *)f->f_gen)->gi_arg;
            ((PyGenObject *)f->f_gen)->gi_arg = NULL;
            bool arg_is_none = Py_IsNone(arg);
            Py_DECREF(arg);
            if (!arg_is_none) {
                static const char *gen_kind[2][2] = {
                        {"<bad>", "generator"},
                        {"coroutine", "async generator"}
                };
                _PyErr_Format(tstate, PyExc_TypeError,
                              "can't send non-None value to a just-started %s",
                              gen_kind[(oparg1 & 0b10) >> 1][oparg1 & 1]);
                goto error;
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(GET_AWAITABLE, ARG_YES, ARG_NO, ARG_YES)
            PyObject *target = REG(oparg1);
            PyObject *awaitable = _PyCoro_GetAwaitableIter(target);
            if (awaitable == NULL)
                goto error;
            if (PyCoro_CheckExact(awaitable)) {
                PyObject *yf = _PyGen_yf((PyGenObject*) awaitable);
                if (yf != NULL) {
                    /* `iter` is a coroutine object that is being
                       awaited, `yf` is a pointer to the current awaitable
                       being awaited on. */
                    Py_DECREF(yf);
                    Py_DECREF(awaitable);
                    _PyErr_SetString(tstate, PyExc_RuntimeError,
                                     "coroutine is being awaited already");
                    goto error;
                }
            }
            SET_REG(oparg3, awaitable);
            Py_INCREF(Py_None);
            SET_REG(oparg3 + 1, Py_None);
            PREDICT(YIELD_FROM);
            DISPATCH();
        END_CASE_OP

        CASE_OP(ENTER_ASYNC_WITH, ARG_YES, ARG_NO, ARG_YES)
            _Py_IDENTIFIER(__aenter__);
            _Py_IDENTIFIER(__aexit__);
            PyObject *mgr = REG(oparg1);
            PyObject *enter = special_lookup(tstate, mgr, &PyId___aenter__);
            if (enter == NULL)
                goto error;
            PyObject *exit = special_lookup(tstate, mgr, &PyId___aexit__);
            if (exit == NULL) {
                Py_DECREF(enter);
                goto error;
            }
            PyObject *enter_res = _PyObject_CallNoArg(enter);
            Py_DECREF(enter);
            if (enter_res == NULL) {
                Py_DECREF(exit);
                goto error;
            }
            SET_REG(oparg3, exit);
            SET_REG(oparg3 + 1, enter_res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(EXIT_ASYNC_WITH, ARG_YES, ARG_NO, ARG_NO)
            PyTryBlock *b = PyFrame_BlockTop(f);
            PyObject *exit_func = REG(oparg1);
            PyObject **args;
            assert(b->b_type != TRY_BLOCK_READY);
            /* NB, the array may be modified,  (see method_vectorcall)
             * so we do not use static variables, and add an extra NULL. */
            PyObject *nones[4] = {NULL, Py_None, Py_None, Py_None};
            args = b->b_type == TRY_BLOCK_RAISED ? PREG(b->b_sextuple + 3) : &nones[1];
            PyObject *res = call_function(tstate, &trace_info, exit_func, args, 3, NULL);
            if (res == NULL)
                goto error;
            SET_REG(oparg1, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(GET_ASYNC_ITER, ARG_YES, ARG_NO, ARG_YES)
            PyObject *iterable = REG(oparg1);
            PyTypeObject *type = Py_TYPE(iterable);

            if (!type->tp_as_async || !type->tp_as_async->am_aiter) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' requires an object with "
                              "__aiter__ method, got %.100s",
                              type->tp_name);
                goto error;
            }
            PyObject *iterator = type->tp_as_async->am_aiter(iterable);
            if (iterator == NULL)
                goto error;
            SET_REG(oparg3, iterator);
            DISPATCH();
        END_CASE_OP

        CASE_OP(GET_ASYNC_NEXT, ARG_YES, ARG_NO, ARG_YES)
            PyObject *iterator = REG(oparg1);
            PyTypeObject *type = Py_TYPE(iterator);
            PyObject *awaitable;

            if (PyAsyncGen_CheckExact(iterator)) {
                awaitable = type->tp_as_async->am_anext(iterator);
                if (awaitable == NULL) {
                    goto error;
                }
            } else {
                if (!type->tp_as_async || !type->tp_as_async->am_anext) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'async for' requires an iterator with "
                                  "__anext__ method, got %.100s",
                                  type->tp_name);
                    goto error;
                }

                PyObject *next_iter = type->tp_as_async->am_anext(iterator);
                if (next_iter == NULL) {
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
                }
                Py_DECREF(next_iter);
            }

            SET_REG(oparg3, awaitable);
            Py_INCREF(Py_None);
            SET_REG(oparg3 + 1, Py_None);
            PREDICT(YIELD_FROM);
            DISPATCH();
        END_CASE_OP

        CASE_OP(END_ASYNC_FOR, ARG_NO, ARG_NO, ARG_NO)
            PyTryBlock *b = PyFrame_BlockPop(f);
            assert(b->b_type == TRY_BLOCK_RAISED);
            PyObject *exc_tp = REG(b->b_sextuple + 3);
            assert(PyExceptionClass_Check(exc_tp));
            if (!PyErr_GivenExceptionMatches(exc_tp, PyExc_StopAsyncIteration)) {
                restore_exc(tstate, PREG(b->b_sextuple), true);
                goto exception_unwind;
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(SETUP_ANNOTATIONS, ARG_NO, ARG_NO, ARG_NO)
            if (f->f_locals == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when setting up annotations");
                goto error;
            }
            _Py_IDENTIFIER(__annotations__);
            /* check if __annotations__ in locals()... */
            if (PyDict_CheckExact(f->f_locals)) {
                PyObject *ann_dict = _PyDict_GetItemIdWithError(f->f_locals, &PyId___annotations__);
                if (ann_dict == NULL) {
                    if (_PyErr_Occurred(tstate)) {
                        goto error;
                    }
                    /* ...if not, create a new one */
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    int err = _PyDict_SetItemId(f->f_locals, &PyId___annotations__, ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                }
            } else {
                /* do the same if locals() is not a dict */
                PyObject *ann_str = _PyUnicode_FromId(&PyId___annotations__);
                if (ann_str == NULL) {
                    goto error;
                }
                PyObject *ann_dict = PyObject_GetItem(f->f_locals, ann_str);
                if (ann_dict == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    int err = PyObject_SetItem(f->f_locals, ann_str, ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                } else {
                    Py_DECREF(ann_dict);
                }
            }
            DISPATCH();
        END_CASE_OP

        CASE_OP(PRINT_EXPR, ARG_YES, ARG_NO, ARG_NO)
            _Py_IDENTIFIER(displayhook);
            PyObject *value = REG(oparg1);
            PyObject *hook = _PySys_GetObjectId(&PyId_displayhook);
            if (hook == NULL) {
                _PyErr_SetString(tstate, PyExc_RuntimeError,
                                 "lost sys.displayhook");
                goto error;
            }
            PyObject *res = PyObject_CallOneArg(hook, value);
            if (res == NULL)
                goto error;
            Py_DECREF(res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(MATCH_SEQUENCE, ARG_YES, ARG_YES, ARG_YES)
            bool check_size = oparg1 & 1;
            bool has_star = oparg1 & 0b10;
            _Py_OPARG target_size = oparg1 >> 2;
            PyObject *subject = REG(oparg2);
            PyObject *res;
            if (Py_TYPE(subject)->tp_flags & Py_TPFLAGS_SEQUENCE) {
                if (check_size) {
                    Py_ssize_t real_size = PyObject_Length(subject);
                    if (real_size < 0) {
                        goto error;
                    }
                    if (has_star) {
                        Py_ssize_t extra_plus = real_size - target_size + 1;
                        res = PyLong_FromSsize_t(extra_plus > 0 ? extra_plus : 0);
                        if (res == NULL) {
                            goto error;
                        }
                    } else {
                        res = real_size == target_size ? Py_True : Py_False;
                        Py_INCREF(res);
                    }
                } else {
                    res = Py_True;
                    Py_INCREF(Py_True);
                }
            } else {
                res = Py_False;
                Py_INCREF(Py_False);
            }
            SET_REG(oparg3, res);
            DISPATCH();
        END_CASE_OP

        CASE_OP(MATCH_DICT, ARG_YES, ARG_YES, ARG_YES)
            PyObject *keys = REG(oparg1);
            PyObject *subject = REG(oparg2);
            int match = Py_TYPE(subject)->tp_flags & Py_TPFLAGS_MAPPING;
            PyObject *values;
            if (match) {
                values = match_keys(tstate, subject, keys);
                if (values == NULL) {
                    goto error;
                }
            } else {
                values = Py_False;
                Py_INCREF(Py_False);
            }
            SET_REG(oparg3, values);
            DISPATCH();
        END_CASE_OP

        CASE_OP(COPY_DICT_WITHOUT_KEYS, ARG_YES, ARG_YES, ARG_YES)
            // rest = dict(TOS1)
            // for key in TOS:
            //     del rest[key]
            // SET_TOP(rest)
            PyObject *keys = REG(oparg1);
            PyObject *subject = REG(oparg2);
            PyObject *rest = PyDict_New();
            if (rest == NULL)
                goto error;
            if (PyDict_Update(rest, subject)) {
                Py_DECREF(rest);
                goto error;
            }
            // This may seem a bit inefficient, but keys is rarely big enough to
            // actually impact runtime.
            assert(PyTuple_CheckExact(keys));
            for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(keys); i++) {
                if (PyDict_DelItem(rest, PyTuple_GET_ITEM(keys, i))) {
                    Py_DECREF(rest);
                    goto error;
                }
            }
            SET_REG(oparg3, rest);
            DISPATCH();
        END_CASE_OP

        CASE_OP(MATCH_CLASS, ARG_YES, ARG_YES, ARG_YES)
            Py_ssize_t nargs = oparg1 >> 1;
            bool with_kwargs = oparg1 & 1;
            PyObject *subject = REG(oparg2);
            PyObject *type = REG(oparg3);
            PyObject *attrs = with_kwargs ? REG(oparg3 + 1) : NULL;
            PyObject *values = match_class(tstate, subject, type, nargs, attrs);
            if (values == NULL)
                goto error;
            SET_REG(oparg3, values);
            DISPATCH();
        END_CASE_OP

#if USE_COMPUTED_GOTOS
        _unknown_opcode:
#else
        default:
#endif
            fprintf(stderr, "XXX lineno: %d, opcode: %d\n",
                    PyFrame_GetLineNumber(f),
                    (int) _Py_OPCODE(*f->f_last_instr));
            _PyErr_SetString(tstate, PyExc_SystemError, "unknown opcode");
            goto error;

    } /* switch */

    /* This should never be reached. Every opcode should end with DISPATCH()
       or goto error. */
    Py_UNREACHABLE();

error:
    /* Double-check exception status. */
#ifdef NDEBUG
    if (!_PyErr_Occurred(tstate)) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                         "error return without exception set");
    }
#endif

    /* Log traceback info. */
    PyTraceBack_Here(f);

    if (tstate->c_tracefunc != NULL) {
        /* Make sure state is set to FRAME_EXECUTING for tracing */
        assert(f->f_state == FRAME_EXECUTING);
        f->f_state = FRAME_UNWINDING;
        call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj,
                       tstate, f, &trace_info);
    }
exception_unwind:
    f->f_state = FRAME_UNWINDING;
    while (f->f_iblock > 0) {
        /* Pop the current block. */
        PyTryBlock *b = PyFrame_BlockPop(f);
        if (b->b_type == TRY_BLOCK_PASSED) {
            continue;
        }

        _Py_OPARG sextuple = b->b_sextuple;
        if (b->b_type == TRY_BLOCK_RAISED) {
            restore_exc(tstate, PREG(b->b_sextuple), false);
            continue;
        }

        assert(b->b_type == TRY_BLOCK_READY);
        int handler = b->b_handler;
        PyFrame_BlockPush(f, TRY_BLOCK_RAISED, f->f_last_instr - first_instr, sextuple);
        _PyErr_StackItem *exc_info = tstate->exc_info;

        PyObject *exc_tp = exc_info->exc_type ? exc_info->exc_type : Py_NewRef(Py_None);
        PyObject *exc_val = exc_info->exc_value ? exc_info->exc_value : Py_NewRef(Py_None);
        PyObject *exc_tb = exc_info->exc_traceback ? exc_info->exc_traceback : Py_NewRef(Py_None);
        SET_REG(sextuple, exc_tp);
        SET_REG(sextuple + 1, exc_val);
        SET_REG(sextuple + 2, exc_tb);

        /* Make the raw exception data available to the handler,
           so a program can emulate the Python main loop. */
        _PyErr_Fetch(tstate, &exc_tp, &exc_val, &exc_tb);
        _PyErr_NormalizeException(tstate, &exc_tp, &exc_val, &exc_tb);
        exc_tb = exc_tb ? exc_tb : Py_NewRef(Py_None);
        PyException_SetTraceback(exc_val, exc_tb);
        exc_info->exc_type = exc_tp;
        exc_info->exc_value = exc_val;
        exc_info->exc_traceback = exc_tb;
        Py_INCREF(exc_tp);
        Py_INCREF(exc_val);
        Py_INCREF(exc_tb);
        SET_REG(sextuple + 3, exc_tp);
        SET_REG(sextuple + 4, exc_val);
        SET_REG(sextuple + 5, exc_tb);

        SET_INSTR_INDEX(handler);
        f->f_state = FRAME_EXECUTING;
        goto main_loop;
    }

    /* End the loop as we still have an error */
    f->f_state = FRAME_RAISED;
    assert(retval == NULL && _PyErr_Occurred(tstate));
    goto clean_exit;
    /* end of main loop */

clean_exit:
    assert(f->f_iblock == 0);
    /* Clean temporary registers. */
    {
        PyObject **tmps = frame_tmps(f);
        Py_ssize_t num = frame_tmp_num(co);
        while (num--) {
            Py_DECREF(tmps[num]);
            tmps[num] = Py_Undefined;
        }
    }

exiting:
    if (trace_info.cframe.use_tracing) {
        if (tstate->c_tracefunc) {
            if (call_trace_protected(tstate->c_tracefunc, tstate->c_traceobj,
                                     tstate, f, &trace_info, PyTrace_RETURN, retval)) {
                Py_CLEAR(retval);
            }
        }
        if (tstate->c_profilefunc) {
            if (call_trace_protected(tstate->c_profilefunc, tstate->c_profileobj,
                                     tstate, f, &trace_info, PyTrace_RETURN, retval)) {
                Py_CLEAR(retval);
            }
        }
    }

    /* pop frame */
exit_eval_frame:
    /* Restore previous cframe */
    tstate->cframe = trace_info.cframe.previous;
    tstate->cframe->use_tracing = trace_info.cframe.use_tracing;

    if (PyDTrace_FUNCTION_RETURN_ENABLED())
        dtrace_function_return(f);
    _Py_LeaveRecursiveCall(tstate);
    tstate->frame = f->f_back;

    assert((retval != NULL) ^ !!_PyErr_Occurred(tstate));
    return retval;
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
                  PyObject **params, PyObject *qualname)
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
        if (params[i] == Py_Undefined) {
            PyObject *raw = PyTuple_GET_ITEM(co->co_varnames, i);
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
                    PyObject **params, PyObject *qualname)
{
    int plural;
    Py_ssize_t kwonly_given = 0;
    Py_ssize_t i;
    PyObject *sig, *kwonly_sig;
    Py_ssize_t co_argcount = co->co_argcount;

    assert((co->co_flags & CO_VARARGS) == 0);
    /* Count missing keyword-only args. */
    for (i = co_argcount; i < co_argcount + co->co_kwonlyargcount; i++) {
        if (params[i] != Py_Undefined) {
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
    if (!co->co_posonlyargcount) {
        return 0;
    }

    int posonly_conflicts = 0;
    PyObject* posonly_names = PyList_New(0);

    for(int k=0; k < co->co_posonlyargcount; k++){
        PyObject* posonly_name = PyTuple_GET_ITEM(co->co_varnames, k);

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


PyFrameObject *
_PyEval_MakeFrameVector(PyThreadState *tstate,
                        PyFrameConstructor *con, PyObject *locals,
                        PyObject *const *args, Py_ssize_t nargs,
                        PyObject *kwnames)
{
    assert(is_tstate_valid(tstate));
    assert(con->fc_defaults == NULL || PyTuple_CheckExact(con->fc_defaults));

    /* Create the frame */
    PyFrameObject *f = _PyFrame_New_NoTrack(tstate, con, locals);
    if (f == NULL) {
        return NULL;
    }

    PyCodeObject *co = (PyCodeObject*)con->fc_code;
    PyObject **params = frame_locals(f);
    PyObject **cellvars = frame_cells_and_frees(f);
    Py_ssize_t ncellvars = frame_cell_num(co);
    PyObject **freevars = cellvars + ncellvars;
    Py_ssize_t nfreevars = frame_free_num(co);

    Py_ssize_t nposparams = co->co_argcount;
    Py_ssize_t well_nposparams = nargs < nposparams ? nargs : nposparams;
    Py_ssize_t over_nposparams = nargs - well_nposparams;
    Py_ssize_t under_nposparams = nposparams - well_nposparams;
    Py_ssize_t nkwparams = co->co_kwonlyargcount;
    Py_ssize_t nparams = nposparams + nkwparams;

    /* Copy all positional arguments into local variables */
    for (Py_ssize_t i = 0; i < well_nposparams; i++) {
        PyObject *x = args[i];
        Py_INCREF(x);
        SET_FRAME_VALUE(params[i], x);
    }
    /* Pack other positional arguments into the *args argument */
    if (co->co_flags & CO_VARARGS) {
        PyObject *u = _PyTuple_FromArray(args + well_nposparams, over_nposparams);
        if (u == NULL) {
            goto fail;
        }
        SET_FRAME_VALUE(params[nparams], u);
    }

    /* Create a dictionary for keyword parameters (**kwags) */
    PyObject *kwdict;
    if (co->co_flags & CO_VARKEYWORDS) {
        kwdict = PyDict_New();
        if (kwdict == NULL) {
            goto fail;
        }
        Py_ssize_t i = nparams + ((co->co_flags & CO_VARARGS) != 0);
        SET_FRAME_VALUE(params[i], kwdict);
    } else {
        kwdict = NULL;
    }

    /* Handle keyword arguments */
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            PyObject *keyword = PyTuple_GET_ITEM(kwnames, i);
            PyObject *value = args[nargs + i];
            Py_ssize_t found;

            if (!keyword || !PyUnicode_Check(keyword)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() keywords must be strings",
                            con->fc_qualname);
                goto fail;
            }

            /* Speed hack: do raw pointer compares. As names are
            normally interned this should almost always hit. */
            PyObject **co_varnames = &PyTuple_GET_ITEM(co->co_varnames, 0);
            for (found = co->co_posonlyargcount; found < nparams; found++) {
                PyObject *varname = co_varnames[found];
                if (varname == keyword) {
                    goto kw_found;
                }
            }
            /* Slow fallback, just in case */
            for (found = co->co_posonlyargcount; found < nparams; found++) {
                PyObject *varname = co_varnames[found];
                int cmp = PyObject_RichCompareBool(keyword, varname, Py_EQ);
                if (cmp > 0) {
                    goto kw_found;
                } else if (cmp < 0) {
                    goto fail;
                }
            }

            assert(found >= nparams);
            if (kwdict) {
                if (PyDict_SetItem(kwdict, keyword, value) == -1) {
                    goto fail;
                }
                continue;
            }
            if (positional_only_passed_as_keyword(
                    tstate, co, kwcount, kwnames, con->fc_qualname)) {
                goto fail;
            }
            _PyErr_Format(tstate, PyExc_TypeError,
                        "%U() got an unexpected keyword argument '%S'",
                        con->fc_qualname, keyword);
            goto fail;

        kw_found:
            if (params[found] != Py_Undefined) {
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%U() got multiple values for argument '%S'",
                            con->fc_qualname, keyword);
                goto fail;
            }
            Py_INCREF(value);
            SET_FRAME_VALUE(params[found], value);
        }
    }

    /* Check the number of positional arguments */
    if (over_nposparams && !(co->co_flags & CO_VARARGS)) {
        too_many_positional(tstate, co, nargs, con->fc_defaults, params, con->fc_qualname);
        goto fail;
    }

    /* Add missing positional arguments (copy default values from defs) */
    if (under_nposparams) {
        Py_ssize_t defcount = con->fc_defaults ? PyTuple_GET_SIZE(con->fc_defaults) : 0;
        Py_ssize_t defstart = nposparams - defcount;
        Py_ssize_t missing = 0;
        for (Py_ssize_t i = nargs; i < defstart; i++) {
            missing += params[i] == Py_Undefined;
        }
        if (missing) {
            missing_arguments(tstate, co, missing, defcount, params, con->fc_qualname);
            goto fail;
        }
        if (defcount) {
            PyObject **defs = &PyTuple_GET_ITEM(con->fc_defaults, 0);
            Py_ssize_t i = well_nposparams > defstart ? well_nposparams - defstart : 0;
            for (; i < defcount; i++) {
                if (params[defstart + i] == Py_Undefined) {
                    PyObject *def = defs[i];
                    Py_INCREF(def);
                    SET_FRAME_VALUE(params[defstart + i], def);
                }
            }
        }
    }

    /* Add missing keyword arguments (copy default values from kwdefs) */
    if (nkwparams) {
        Py_ssize_t missing = 0;
        for (Py_ssize_t i = nposparams; i < nparams; i++) {
            if (params[i] != Py_Undefined) {
                continue;
            }
            PyObject *varname = PyTuple_GET_ITEM(co->co_varnames, i);
            if (con->fc_kwdefaults) {
                PyObject *def = PyDict_GetItemWithError(con->fc_kwdefaults, varname);
                if (def) {
                    Py_INCREF(def);
                    SET_FRAME_VALUE(params[i], def);
                    continue;
                } else if (_PyErr_Occurred(tstate)) {
                    goto fail;
                }
            }
            missing++;
        }
        if (missing) {
            missing_arguments(tstate, co, missing, -1, params, con->fc_qualname);
            goto fail;
        }
    }

    /* Allocate and initialize storage for cell vars, and copy free
       vars into frame. */
    for (Py_ssize_t i = 0; i < ncellvars; i++) {
        PyObject *c;
        /* Possibly account for the cell variable being an argument. */
        if (co->co_cell2arg && co->co_cell2arg[i] != CO_CELL_NOT_AN_ARG) {
            Py_ssize_t arg = co->co_cell2arg[i];
            c = PyCell_New(params[arg]);
            /* Clear the local copy. */
            SET_FRAME_VALUE(params[arg], Py_Undefined);
        } else {
            c = PyCell_New(NULL);
        }
        if (c == NULL) {
            goto fail;
        }
        assert(cellvars[i] == NULL);
        cellvars[i] = c;
    }

    /* Copy closure variables to free variables */
    for (Py_ssize_t i = 0; i < nfreevars; i++) {
        PyObject *o = PyTuple_GET_ITEM(con->fc_closure, i);
        Py_INCREF(o);
        assert(freevars[i] == NULL);
        freevars[i] = o;
    }

    return f;

fail: /* Jump here from prelude on failure */

    /* decref'ing the frame can cause __del__ methods to get invoked,
       which can call back into Python.  While we're done with the
       current Python frame (f), the associated C stack is still in use,
       so recursion_depth must be boosted for the duration.
    */
    if (Py_REFCNT(f) > 1) {
        Py_DECREF(f);
        _PyObject_GC_TRACK(f);
    } else {
        ++tstate->recursion_depth;
        Py_DECREF(f);
        --tstate->recursion_depth;
    }
    return NULL;
}

static PyObject *
make_coro(PyFrameConstructor *con, PyFrameObject *f)
{
    assert (((PyCodeObject *)con->fc_code)->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR));
    PyObject *gen;
    int is_coro = ((PyCodeObject *)con->fc_code)->co_flags & CO_COROUTINE;

    /* Don't need to keep the reference to f_back, it will be set
        * when the generator is resumed. */
    Py_CLEAR(f->f_back);

    /* Create a new generator that owns the ready to run frame
        * and return that as the value. */
    if (is_coro) {
            gen = PyCoro_New(f, con->fc_name, con->fc_qualname);
    } else if (((PyCodeObject *)con->fc_code)->co_flags & CO_ASYNC_GENERATOR) {
            gen = PyAsyncGen_New(f, con->fc_name, con->fc_qualname);
    } else {
            gen = PyGen_NewWithQualName(f, con->fc_name, con->fc_qualname);
    }
    if (gen == NULL) {
        return NULL;
    }

    _PyObject_GC_TRACK(f);

    return gen;
}

PyObject *
_PyEval_Vector(PyThreadState *tstate, PyFrameConstructor *con,
               PyObject *locals,
               PyObject* const* args, size_t argcount,
               PyObject *kwnames)
{
    PyFrameObject *f = _PyEval_MakeFrameVector(
        tstate, con, locals, args, argcount, kwnames);
    if (f == NULL) {
        return NULL;
    }
    if (((PyCodeObject *)con->fc_code)->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        return make_coro(con, f);
    }
    PyObject *retval = _PyEval_EvalFrame(tstate, f, 0);

    /* decref'ing the frame can cause __del__ methods to get invoked,
       which can call back into Python.  While we're done with the
       current Python frame (f), the associated C stack is still in use,
       so recursion_depth must be boosted for the duration.
    */
    if (Py_REFCNT(f) > 1) {
        Py_DECREF(f);
        _PyObject_GC_TRACK(f);
    }
    else {
        ++tstate->recursion_depth;
        Py_DECREF(f);
        --tstate->recursion_depth;
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
    res = _PyEval_Vector(tstate, &constr, locals,
                         allargs, argcount,
                         kwnames);
    if (kwcount) {
        Py_DECREF(kwnames);
        PyMem_Free(newargs);
    }
fail:
    Py_DECREF(defaults);
    return res;
}


static PyObject *
special_lookup(PyThreadState *tstate, PyObject *o, _Py_Identifier *id)
{
    PyObject *res;
    res = _PyObject_LookupSpecial(o, id);
    if (res == NULL && !_PyErr_Occurred(tstate)) {
        _PyErr_SetObject(tstate, PyExc_AttributeError, _PyUnicode_FromId(id));
        return NULL;
    }
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
        value = _PyObject_CallNoArg(exc);
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
            fixed_cause = _PyObject_CallNoArg(cause);
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

/* Iterate iterable num times and store the results to outputs[].
   If star_num == 0, do a simple unpack.
   Otherwise, do an unpack with a variable target.
   Return 1 for success, 0 if error.
*/
static int
unpack_iterable(PyThreadState *tstate, PyObject *iterable,
                _Py_OPARG num, _Py_OPARG star_num, PyObject **outputs)
{
    PyObject *iterator = PyObject_GetIter(iterable);
    if (iterator == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
            Py_TYPE(iterable)->tp_iter == NULL &&
            !PySequence_Check(iterable)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "cannot unpack non-iterable %.200s object",
                          Py_TYPE(iterable)->tp_name);
        }
        return 0;
    }

    for (_Py_OPARG i = 0; i < num; i++) {
        PyObject *item = PyIter_Next(iterator);
        if (item == NULL) {
            /* Iterator done, via error or exhaustion. */
            if (!_PyErr_Occurred(tstate)) {
                const char *at_lest = star_num ? "at least " : "";
                _PyErr_Format(tstate, PyExc_ValueError,
                              "not enough values to unpack (expected %s%u, got %u)",
                              at_lest, num, i);
            }
            Py_DECREF(iterator);
            return 0;
        }
        SET_FRAME_VALUE(outputs[i], item);
    }

    if (!star_num) {
        /* We better have exhausted the iterator now. */
        PyObject *item = PyIter_Next(iterator);
        Py_DECREF(iterator);
        if (item == NULL) {
            return !_PyErr_Occurred(tstate);
        }
        Py_DECREF(item);
        _PyErr_Format(tstate, PyExc_ValueError,
                      "too many values to unpack (expected %u)",
                      num);
        return 0;
    }

    PyObject *list = PySequence_List(iterator);
    Py_DECREF(iterator);
    if (list == NULL) {
        return 0;
    }
    SET_FRAME_VALUE(outputs[num], list);
    --star_num;

    Py_ssize_t remaining_len = PyList_GET_SIZE(list);
    if (remaining_len < star_num) {
        _PyErr_Format(tstate, PyExc_ValueError,
                      "not enough values to unpack (expected at least %u, got %zd)",
                      num + star_num, num + remaining_len);
        return 0;
    }

    /* Pop the "after-variable" args off the list. */
    outputs += num + 1;
    remaining_len -= star_num;
    for (_Py_OPARG i = 0; i < star_num; i++) {
        PyObject *v = PyList_GET_ITEM(list, remaining_len + i);
        SET_FRAME_VALUE(outputs[i], v);
    }
    Py_SET_SIZE(list, remaining_len);
    return 1;
}

static void
call_exc_trace(Py_tracefunc func, PyObject *self,
               PyThreadState *tstate,
               PyFrameObject *f,
               PyTraceInfo *trace_info)
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
    err = call_trace(func, self, tstate, f, trace_info, PyTrace_EXCEPTION, arg);
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
                     PyThreadState *tstate, PyFrameObject *frame,
                     PyTraceInfo *trace_info,
                     int what, PyObject *arg)
{
    PyObject *type, *value, *traceback;
    int err;
    _PyErr_Fetch(tstate, &type, &value, &traceback);
    err = call_trace(func, obj, tstate, frame, trace_info, what, arg);
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
initialize_trace_info(PyTraceInfo *trace_info, PyFrameObject *frame)
{
    if (trace_info->code != frame->f_code) {
        trace_info->code = frame->f_code;
        _PyCode_InitAddressRange(frame->f_code, &trace_info->bounds);
    }
}

static int
call_trace(Py_tracefunc func, PyObject *obj,
           PyThreadState *tstate, PyFrameObject *frame,
           PyTraceInfo *trace_info,
           int what, PyObject *arg)
{
    int result;
    if (tstate->tracing)
        return 0;
    tstate->tracing++;
    tstate->cframe->use_tracing = 0;
    if (!frame_has_executed(frame)) {
        frame->f_lineno = frame->f_code->co_firstlineno;
    }
    else {
        initialize_trace_info(trace_info, frame);
        frame->f_lineno = _PyCode_CheckLineNumber(frame_instr_offset(frame), &trace_info->bounds);
    }
    result = func(obj, frame, what, arg);
    frame->f_lineno = 0;
    tstate->cframe->use_tracing = ((tstate->c_tracefunc != NULL)
                           || (tstate->c_profilefunc != NULL));
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
    tstate->cframe->use_tracing = ((tstate->c_tracefunc != NULL)
                           || (tstate->c_profilefunc != NULL));
    result = PyObject_Call(func, args, NULL);
    tstate->tracing = save_tracing;
    tstate->cframe->use_tracing = save_use_tracing;
    return result;
}

/* See Objects/lnotab_notes.txt for a description of how tracing works. */
static int
maybe_call_line_trace(Py_tracefunc func, PyObject *obj,
                      PyThreadState *tstate, PyFrameObject *frame,
                      PyTraceInfo *trace_info, _Py_CODEUNIT *prev_instr)
{
    int result = 0;

    /* If the last instruction falls at the start of a line or if it
       represents a jump backwards, update the frame's line number and
       then call the trace function if we're tracing source lines.
    */
    initialize_trace_info(trace_info, frame);
    int line = _PyCode_CheckLineNumber(frame_instr_offset(frame), &trace_info->bounds);
    if (line != -1 && frame->f_trace_lines) {
        int lastline = _PyCode_CheckLineNumber(
                (char *) prev_instr - (char *) frame_first_instr(frame),
                &trace_info->bounds);
        /* Trace backward edges or if line number has changed */
        if (frame->f_last_instr < prev_instr || line != lastline) {
            result = call_trace(func, obj, tstate, frame, trace_info, PyTrace_LINE, Py_None);
        }
    }
    /* Always emit an opcode event if we're tracing all opcodes. */
    if (frame->f_trace_opcodes) {
        result = call_trace(func, obj, tstate, frame, trace_info, PyTrace_OPCODE, Py_None);
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
    tstate->cframe->use_tracing = tstate->c_tracefunc != NULL;
    Py_XDECREF(profileobj);

    Py_XINCREF(arg);
    tstate->c_profileobj = arg;
    tstate->c_profilefunc = func;

    /* Flag that tracing or profiling is turned on */
    tstate->cframe->use_tracing = (func != NULL) || (tstate->c_tracefunc != NULL);
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
    tstate->cframe->use_tracing = (tstate->c_profilefunc != NULL);
    Py_XDECREF(traceobj);

    Py_XINCREF(arg);
    tstate->c_traceobj = arg;
    tstate->c_tracefunc = func;

    /* Flag that tracing or profiling is turned on */
    tstate->cframe->use_tracing = ((func != NULL)
                           || (tstate->c_profilefunc != NULL));

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

PyFrameObject *
PyEval_GetFrame(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->frame;
}

PyObject *
_PyEval_GetBuiltins(PyThreadState *tstate)
{
    PyFrameObject *frame = tstate->frame;
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
    PyFrameObject *current_frame = tstate->frame;
    if (current_frame == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "frame does not exist");
        return NULL;
    }

    if (PyFrame_FastToLocalsWithError(current_frame) < 0) {
        return NULL;
    }

    assert(current_frame->f_locals != NULL);
    return current_frame->f_locals;
}

PyObject *
PyEval_GetGlobals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyFrameObject *current_frame = tstate->frame;
    if (current_frame == NULL) {
        return NULL;
    }

    assert(current_frame->f_globals != NULL);
    return current_frame->f_globals;
}

int
PyEval_MergeCompilerFlags(PyCompilerFlags *cf)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyFrameObject *current_frame = tstate->frame;
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
if (trace_info->cframe.use_tracing && tstate->c_profilefunc) { \
    if (call_trace(tstate->c_profilefunc, tstate->c_profileobj, \
        tstate, tstate->frame, trace_info, \
        PyTrace_C_CALL, func)) { \
        x = NULL; \
    } \
    else { \
        x = call; \
        if (tstate->c_profilefunc != NULL) { \
            if (x == NULL) { \
                call_trace_protected(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate, tstate->frame, trace_info, \
                    PyTrace_C_EXCEPTION, func); \
                /* XXX should pass (type, value, tb) */ \
            } else { \
                if (call_trace(tstate->c_profilefunc, \
                    tstate->c_profileobj, \
                    tstate, tstate->frame, trace_info, \
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
                    PyTraceInfo *trace_info,
                    PyObject *func,
                    PyObject **args, Py_ssize_t nargs,
                    PyObject *kwnames)
{
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

/* Issue #29227: Inline call_function() into _PyEval_EvalFrameDefault()
   to reduce the stack consumption. */
Py_LOCAL_INLINE(PyObject *) _Py_HOT_FUNCTION
call_function(PyThreadState *tstate, PyTraceInfo *trace_info,
              PyObject *func, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (trace_info->cframe.use_tracing) {
        return trace_call_function(tstate, trace_info, func, args, nargs, kwnames);
    } else {
        return PyObject_Vectorcall(func, args, nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
    }
}

static PyObject *
do_call_core(PyThreadState *tstate,
             PyTraceInfo *trace_info,
             PyObject *func,
             PyObject *callargs,
             PyObject *kwdict)
{
    PyObject *result;

    if (PyCFunction_CheckExact(func) || PyCMethod_CheckExact(func)) {
        C_TRACE(result, PyObject_Call(func, callargs, kwdict));
        return result;
    }
    else if (Py_IS_TYPE(func, &PyMethodDescr_Type)) {
        Py_ssize_t nargs = PyTuple_GET_SIZE(callargs);
        if (nargs > 0 && trace_info->cframe.use_tracing) {
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
import_name(PyThreadState *tstate, PyFrameObject *f,
            PyObject *name, PyObject *fromlist)
{
    _Py_IDENTIFIER(__import__);

    PyObject *import_func = _PyDict_GetItemIdWithError(f->f_builtins, &PyId___import__);
    if (import_func == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_ImportError, "__import__ not found");
        }
        return NULL;
    }

    int level = 0;
    const char *name_str = PyUnicode_AsUTF8(name);
    assert(name_str);
    while (*name_str++ == '.') {
        level++;
    }

    if (level) {
        name = PyUnicode_Substring(name, level, PyUnicode_GET_LENGTH(name));
    } else {
        Py_INCREF(name);
    }

    PyObject *res = NULL;
    /* Fast path for not overloaded __import__. */
    if (import_func == tstate->interp->import_func) {
        res = PyImport_ImportModuleLevelObject(
                        name,
                        f->f_globals,
                        f->f_locals == NULL ? Py_None : f->f_locals,
                        fromlist,
                        level);
    } else {
        PyObject *level_obj = PyLong_FromLong(level);
        if (level_obj) {
            PyObject* stack[5];
            Py_INCREF(import_func);
            stack[0] = name;
            stack[1] = f->f_globals;
            stack[2] = f->f_locals == NULL ? Py_None : f->f_locals;
            stack[3] = fromlist;
            stack[4] = level_obj;
            res = _PyObject_FastCall(import_func, stack, 5);
            Py_DECREF(import_func);
        }
    }

    Py_DECREF(name);
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
format_exc_unbound_fast(PyThreadState *tstate, PyCodeObject *co, int oparg)
{
    /* Don't stomp existing exception */
    if (_PyErr_Occurred(tstate)) {
        return;
    }
    /* If tmp register is used, it can never be undefined.
     * This is guaranteed by our execution model. */
    assert(oparg < frame_local_num(co));
    PyObject *name = PyTuple_GET_ITEM(co->co_varnames, oparg);
    format_exc_check_arg(tstate, PyExc_UnboundLocalError,
                         "local variable '%.200s' referenced before assignment",
                         name);
}

static void
format_exc_unbound_deref(PyThreadState *tstate, PyCodeObject *co, int oparg)
{
    if (_PyErr_Occurred(tstate)) {
        return;
    }
    Py_ssize_t ncells = frame_cell_num(co);
    if (oparg < ncells) {
        format_exc_check_arg(tstate, PyExc_UnboundLocalError,
                             "free variable '%.200s' referenced before assignment "
                             "in local scope",
                             PyTuple_GET_ITEM(co->co_cellvars, oparg));
    } else {
        oparg -= ncells;
        format_exc_check_arg(tstate, PyExc_NameError,
                             "free variable '%.200s' referenced before assignment " \
                             "in enclosing scope",
                             PyTuple_GET_ITEM(co->co_freevars, oparg));
    }
}

static void
format_exc_name_error(PyThreadState *tstate, PyObject *name, bool overwrite)
{
    if (overwrite) {
        assert(_PyErr_Occurred(tstate));
        if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
            return;
        }
    } else {
        if (_PyErr_Occurred(tstate)) {
            return;
        }
    }
    format_exc_check_arg(tstate, PyExc_NameError,
                         "name '%.200s' is not defined", name);
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
dtrace_function_entry(PyFrameObject *f)
{
    const char *filename;
    const char *funcname;
    int lineno;

    PyCodeObject *code = f->f_code;
    filename = PyUnicode_AsUTF8(code->co_filename);
    funcname = PyUnicode_AsUTF8(code->co_name);
    lineno = PyFrame_GetLineNumber(f);

    PyDTrace_FUNCTION_ENTRY(filename, funcname, lineno);
}

static void
dtrace_function_return(PyFrameObject *f)
{
    const char *filename;
    const char *funcname;
    int lineno;

    PyCodeObject *code = f->f_code;
    filename = PyUnicode_AsUTF8(code->co_filename);
    funcname = PyUnicode_AsUTF8(code->co_name);
    lineno = PyFrame_GetLineNumber(f);

    PyDTrace_FUNCTION_RETURN(filename, funcname, lineno);
}

/* DTrace equivalent of maybe_call_line_trace. */
static void
maybe_dtrace_line(PyFrameObject *frame,
                  PyTraceInfo *trace_info, _Py_CODEUNIT *prev_instr)
{
    const char *co_filename, *co_name;

    /* If the last instruction executed isn't in the current
       instruction window, reset the window.
    */
    initialize_trace_info(trace_info, frame);
    int line = _PyCode_CheckLineNumber(frame_instr_offset(frame), &trace_info->bounds);
    /* If the last instruction falls at the start of a line or if
       it represents a jump backwards, update the frame's line
       number and call the trace function. */
    if (line != frame->f_lineno || frame->f_last_instr < prev_instr) {
        if (line != -1) {
            frame->f_lineno = line;
            co_filename = PyUnicode_AsUTF8(frame->f_code->co_filename);
            if (!co_filename)
                co_filename = "?";
            co_name = PyUnicode_AsUTF8(frame->f_code->co_name);
            if (!co_name)
                co_name = "?";
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
