#ifndef Py_INTERNAL_CEVAL_H
#define Py_INTERNAL_CEVAL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "dynamic_annotations.h"  // _Py_ANNOTATE_RWLOCK_CREATE

#include "pycore_code.h"          // _PyCode_GetTLBCFast()
#include "pycore_interp.h"        // PyInterpreterState.eval_frame
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_stats.h"         // EVAL_CALL_STAT_INC()
#include "pycore_typedefs.h"      // _PyInterpreterFrame


/* Forward declarations */
struct _ceval_runtime_state;

// Export for '_lsprof' shared extension
PyAPI_FUNC(int) _PyEval_SetProfile(PyThreadState *tstate, Py_tracefunc func, PyObject *arg);

extern int _PyEval_SetTrace(PyThreadState *tstate, Py_tracefunc func, PyObject *arg);

extern int _PyEval_SetOpcodeTrace(PyFrameObject *f, bool enable);

// Helper to look up a builtin object
// Export for 'array' shared extension
PyAPI_FUNC(PyObject*) _PyEval_GetBuiltin(PyObject *);

extern PyObject* _PyEval_GetBuiltinId(_Py_Identifier *);

extern void _PyEval_SetSwitchInterval(unsigned long microseconds);
extern unsigned long _PyEval_GetSwitchInterval(void);

// Export for '_queue' shared extension
PyAPI_FUNC(int) _PyEval_MakePendingCalls(PyThreadState *);

#ifndef Py_DEFAULT_RECURSION_LIMIT
#  define Py_DEFAULT_RECURSION_LIMIT 1000
#endif

extern void _Py_FinishPendingCalls(PyThreadState *tstate);
extern void _PyEval_InitState(PyInterpreterState *);
extern void _PyEval_SignalReceived(void);

// bitwise flags:
#define _Py_PENDING_MAINTHREADONLY 1
#define _Py_PENDING_RAWFREE 2

typedef int _Py_add_pending_call_result;
#define _Py_ADD_PENDING_SUCCESS 0
#define _Py_ADD_PENDING_FULL -1

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(_Py_add_pending_call_result) _PyEval_AddPendingCall(
    PyInterpreterState *interp,
    _Py_pending_call_func func,
    void *arg,
    int flags);

#ifdef HAVE_FORK
extern PyStatus _PyEval_ReInitThreads(PyThreadState *tstate);
#endif

// Used by sys.call_tracing()
extern PyObject* _PyEval_CallTracing(PyObject *func, PyObject *args);

// Used by sys.get_asyncgen_hooks()
extern PyObject* _PyEval_GetAsyncGenFirstiter(void);
extern PyObject* _PyEval_GetAsyncGenFinalizer(void);

// Used by sys.set_asyncgen_hooks()
extern int _PyEval_SetAsyncGenFirstiter(PyObject *);
extern int _PyEval_SetAsyncGenFinalizer(PyObject *);

// Used by sys.get_coroutine_origin_tracking_depth()
// and sys.set_coroutine_origin_tracking_depth()
extern int _PyEval_GetCoroutineOriginTrackingDepth(void);
extern int _PyEval_SetCoroutineOriginTrackingDepth(int depth);

extern void _PyEval_Fini(void);


extern PyObject* _PyEval_GetBuiltins(PyThreadState *tstate);

// Trampoline API

typedef struct {
    // Callback to initialize the trampoline state
    void* (*init_state)(void);
    // Callback to register every trampoline being created
    void (*write_state)(void* state, const void *code_addr,
                        unsigned int code_size, PyCodeObject* code);
    // Callback to free the trampoline state
    int (*free_state)(void* state);
} _PyPerf_Callbacks;

extern int _PyPerfTrampoline_SetCallbacks(_PyPerf_Callbacks *);
extern void _PyPerfTrampoline_GetCallbacks(_PyPerf_Callbacks *);
extern int _PyPerfTrampoline_Init(int activate);
extern int _PyPerfTrampoline_Fini(void);
extern void _PyPerfTrampoline_FreeArenas(void);
extern int _PyIsPerfTrampolineActive(void);
extern PyStatus _PyPerfTrampoline_AfterFork_Child(void);
#ifdef PY_HAVE_PERF_TRAMPOLINE
extern _PyPerf_Callbacks _Py_perfmap_callbacks;
extern _PyPerf_Callbacks _Py_perfmap_jit_callbacks;
#endif

static inline PyObject*
_PyEval_EvalFrame(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    EVAL_CALL_STAT_INC(EVAL_CALL_TOTAL);
    if (tstate->interp->eval_frame == NULL) {
        return _PyEval_EvalFrameDefault(tstate, frame, throwflag);
    }
    return tstate->interp->eval_frame(tstate, frame, throwflag);
}

extern PyObject*
_PyEval_Vector(PyThreadState *tstate,
            PyFunctionObject *func, PyObject *locals,
            PyObject* const* args, size_t argcount,
            PyObject *kwnames);

extern int _PyEval_ThreadsInitialized(void);
extern void _PyEval_InitGIL(PyThreadState *tstate, int own_gil);
extern void _PyEval_FiniGIL(PyInterpreterState *interp);

extern void _PyEval_AcquireLock(PyThreadState *tstate);

extern void _PyEval_ReleaseLock(PyInterpreterState *, PyThreadState *,
                                int final_release);

#ifdef Py_GIL_DISABLED
// Returns 0 or 1 if the GIL for the given thread's interpreter is disabled or
// enabled, respectively.
//
// The enabled state of the GIL will not change while one or more threads are
// attached.
static inline int
_PyEval_IsGILEnabled(PyThreadState *tstate)
{
    struct _gil_runtime_state *gil = tstate->interp->ceval.gil;
    return _Py_atomic_load_int_relaxed(&gil->enabled) != 0;
}

// Enable or disable the GIL used by the interpreter that owns tstate, which
// must be the current thread. This may affect other interpreters, if the GIL
// is shared. All three functions will be no-ops (and return 0) if the
// interpreter's `enable_gil' config is not _PyConfig_GIL_DEFAULT.
//
// Every call to _PyEval_EnableGILTransient() must be paired with exactly one
// call to either _PyEval_EnableGILPermanent() or
// _PyEval_DisableGIL(). _PyEval_EnableGILPermanent() and _PyEval_DisableGIL()
// must only be called while the GIL is enabled from a call to
// _PyEval_EnableGILTransient().
//
// _PyEval_EnableGILTransient() returns 1 if it enabled the GIL, or 0 if the
// GIL was already enabled, whether transiently or permanently. The caller will
// hold the GIL upon return.
//
// _PyEval_EnableGILPermanent() returns 1 if it permanently enabled the GIL
// (which must already be enabled), or 0 if it was already permanently
// enabled. Once _PyEval_EnableGILPermanent() has been called once, all
// subsequent calls to any of the three functions will be no-ops.
//
// _PyEval_DisableGIL() returns 1 if it disabled the GIL, or 0 if the GIL was
// kept enabled because of another request, whether transient or permanent.
//
// All three functions must be called by an attached thread (this implies that
// if the GIL is enabled, the current thread must hold it).
extern int _PyEval_EnableGILTransient(PyThreadState *tstate);
extern int _PyEval_EnableGILPermanent(PyThreadState *tstate);
extern int _PyEval_DisableGIL(PyThreadState *state);


static inline _Py_CODEUNIT *
_PyEval_GetExecutableCode(PyThreadState *tstate, PyCodeObject *co)
{
    _Py_CODEUNIT *bc = _PyCode_GetTLBCFast(tstate, co);
    if (bc != NULL) {
        return bc;
    }
    return _PyCode_GetTLBC(co);
}

#endif

extern void _PyEval_DeactivateOpCache(void);


/* --- _Py_EnterRecursiveCall() ----------------------------------------- */

static inline int _Py_MakeRecCheck(PyThreadState *tstate)  {
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    return here_addr < _tstate->c_stack_soft_limit;
}

// Export for '_json' shared extension, used via _Py_EnterRecursiveCall()
// static inline function.
PyAPI_FUNC(int) _Py_CheckRecursiveCall(
    PyThreadState *tstate,
    const char *where);

int _Py_CheckRecursiveCallPy(
    PyThreadState *tstate);

static inline int _Py_EnterRecursiveCallTstate(PyThreadState *tstate,
                                               const char *where) {
    return (_Py_MakeRecCheck(tstate) && _Py_CheckRecursiveCall(tstate, where));
}

static inline int _Py_EnterRecursiveCall(const char *where) {
    PyThreadState *tstate = _PyThreadState_GET();
    return _Py_EnterRecursiveCallTstate(tstate, where);
}

static inline void _Py_LeaveRecursiveCallTstate(PyThreadState *tstate) {
    (void)tstate;
}

PyAPI_FUNC(void) _Py_InitializeRecursionLimits(PyThreadState *tstate);

static inline int _Py_ReachedRecursionLimit(PyThreadState *tstate)  {
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    assert(_tstate->c_stack_hard_limit != 0);
    return here_addr <= _tstate->c_stack_soft_limit;
}

static inline void _Py_LeaveRecursiveCall(void)  {
}

extern _PyInterpreterFrame* _PyEval_GetFrame(void);

PyAPI_FUNC(PyObject *)_Py_MakeCoro(PyFunctionObject *func);

/* Handle signals, pending calls, GIL drop request
   and asynchronous exception */
PyAPI_FUNC(int) _Py_HandlePending(PyThreadState *tstate);

extern PyObject * _PyEval_GetFrameLocals(void);

typedef PyObject *(*conversion_func)(PyObject *);

PyAPI_DATA(const binaryfunc) _PyEval_BinaryOps[];
PyAPI_DATA(const conversion_func) _PyEval_ConversionFuncs[];

typedef struct _special_method {
    PyObject *name;
    const char *error;
    const char *error_suggestion;  // improved optional suggestion
} _Py_SpecialMethod;

PyAPI_DATA(const _Py_SpecialMethod) _Py_SpecialMethods[];
PyAPI_DATA(const size_t) _Py_FunctionAttributeOffsets[];

PyAPI_FUNC(int) _PyEval_CheckExceptStarTypeValid(PyThreadState *tstate, PyObject* right);
PyAPI_FUNC(int) _PyEval_CheckExceptTypeValid(PyThreadState *tstate, PyObject* right);
PyAPI_FUNC(int) _PyEval_ExceptionGroupMatch(_PyInterpreterFrame *, PyObject* exc_value, PyObject *match_type, PyObject **match, PyObject **rest);
PyAPI_FUNC(void) _PyEval_FormatAwaitableError(PyThreadState *tstate, PyTypeObject *type, int oparg);
PyAPI_FUNC(void) _PyEval_FormatExcCheckArg(PyThreadState *tstate, PyObject *exc, const char *format_str, PyObject *obj);
PyAPI_FUNC(void) _PyEval_FormatExcUnbound(PyThreadState *tstate, PyCodeObject *co, int oparg);
PyAPI_FUNC(void) _PyEval_FormatKwargsError(PyThreadState *tstate, PyObject *func, PyObject *kwargs);
PyAPI_FUNC(PyObject *) _PyEval_ImportFrom(PyThreadState *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyEval_ImportName(PyThreadState *, _PyInterpreterFrame *, PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *)_PyEval_MatchClass(PyThreadState *tstate, PyObject *subject, PyObject *type, Py_ssize_t nargs, PyObject *kwargs);
PyAPI_FUNC(PyObject *)_PyEval_MatchKeys(PyThreadState *tstate, PyObject *map, PyObject *keys);
PyAPI_FUNC(void) _PyEval_MonitorRaise(PyThreadState *tstate, _PyInterpreterFrame *frame, _Py_CODEUNIT *instr);
PyAPI_FUNC(int) _PyEval_UnpackIterableStackRef(PyThreadState *tstate, PyObject *v, int argcnt, int argcntafter, _PyStackRef *sp);
PyAPI_FUNC(void) _PyEval_FrameClearAndPop(PyThreadState *tstate, _PyInterpreterFrame *frame);
PyAPI_FUNC(PyObject **) _PyObjectArray_FromStackRefArray(_PyStackRef *input, Py_ssize_t nargs, PyObject **scratch);

PyAPI_FUNC(void) _PyObjectArray_Free(PyObject **array, PyObject **scratch);

PyAPI_FUNC(PyObject *) _PyEval_GetANext(PyObject *aiter);
PyAPI_FUNC(void) _PyEval_LoadGlobalStackRef(PyObject *globals, PyObject *builtins, PyObject *name, _PyStackRef *writeto);
PyAPI_FUNC(PyObject *) _PyEval_GetAwaitable(PyObject *iterable, int oparg);
PyAPI_FUNC(PyObject *) _PyEval_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject *name);
PyAPI_FUNC(int)
_Py_Check_ArgsIterable(PyThreadState *tstate, PyObject *func, PyObject *args);

/*
 * Indicate whether a special method of given 'oparg' can use the (improved)
 * alternative error message instead. Only methods loaded by LOAD_SPECIAL
 * support alternative error messages.
 *
 * Symbol is exported for the JIT (see discussion on GH-132218).
 */
PyAPI_FUNC(int)
_PyEval_SpecialMethodCanSuggest(PyObject *self, int oparg);

/* Bits that can be set in PyThreadState.eval_breaker */
#define _PY_GIL_DROP_REQUEST_BIT (1U << 0)
#define _PY_SIGNALS_PENDING_BIT (1U << 1)
#define _PY_CALLS_TO_DO_BIT (1U << 2)
#define _PY_ASYNC_EXCEPTION_BIT (1U << 3)
#define _PY_GC_SCHEDULED_BIT (1U << 4)
#define _PY_EVAL_PLEASE_STOP_BIT (1U << 5)
#define _PY_EVAL_EXPLICIT_MERGE_BIT (1U << 6)
#define _PY_EVAL_JIT_INVALIDATE_COLD_BIT (1U << 7)

/* Reserve a few bits for future use */
#define _PY_EVAL_EVENTS_BITS 8
#define _PY_EVAL_EVENTS_MASK ((1 << _PY_EVAL_EVENTS_BITS)-1)

static inline void
_Py_set_eval_breaker_bit(PyThreadState *tstate, uintptr_t bit)
{
    _Py_atomic_or_uintptr(&tstate->eval_breaker, bit);
}

static inline void
_Py_unset_eval_breaker_bit(PyThreadState *tstate, uintptr_t bit)
{
    _Py_atomic_and_uintptr(&tstate->eval_breaker, ~bit);
}

static inline int
_Py_eval_breaker_bit_is_set(PyThreadState *tstate, uintptr_t bit)
{
    uintptr_t b = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker);
    return (b & bit) != 0;
}

// Free-threaded builds use these functions to set or unset a bit on all
// threads in the given interpreter.
void _Py_set_eval_breaker_bit_all(PyInterpreterState *interp, uintptr_t bit);
void _Py_unset_eval_breaker_bit_all(PyInterpreterState *interp, uintptr_t bit);

PyAPI_FUNC(_PyStackRef) _PyFloat_FromDouble_ConsumeInputs(_PyStackRef left, _PyStackRef right, double value);

#ifndef Py_SUPPORTS_REMOTE_DEBUG
    #if defined(__APPLE__)
    #include <TargetConditionals.h>
    #  if !defined(TARGET_OS_OSX)
// Older macOS SDKs do not define TARGET_OS_OSX
    #     define TARGET_OS_OSX 1
    #  endif
    #endif
    #if ((defined(__APPLE__) && TARGET_OS_OSX) || defined(MS_WINDOWS) || (defined(__linux__) && HAVE_PROCESS_VM_READV))
    #    define Py_SUPPORTS_REMOTE_DEBUG 1
    #endif
#endif

#if defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)
extern int _PyRunRemoteDebugger(PyThreadState *tstate);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_H */
