#ifndef Py_INTERNAL_CEVAL_H
#define Py_INTERNAL_CEVAL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "dynamic_annotations.h" // _Py_ANNOTATE_RWLOCK_CREATE

#include "pycore_interp.h"        // PyInterpreterState.eval_frame
#include "pycore_pystate.h"       // _PyThreadState_GET()

/* Forward declarations */
struct pyruntimestate;
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
extern void _PyEval_SignalReceived(PyInterpreterState *interp);

// bitwise flags:
#define _Py_PENDING_MAINTHREADONLY 1
#define _Py_PENDING_RAWFREE 2

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(int) _PyEval_AddPendingCall(
    PyInterpreterState *interp,
    _Py_pending_call_func func,
    void *arg,
    int flags);

extern void _PyEval_SignalAsyncExc(PyInterpreterState *interp);
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
extern PyObject* _PyEval_BuiltinsFromGlobals(
    PyThreadState *tstate,
    PyObject *globals);

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
#endif

static inline PyObject*
_PyEval_EvalFrame(PyThreadState *tstate, struct _PyInterpreterFrame *frame, int throwflag)
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
extern void _PyEval_ReleaseLock(PyInterpreterState *, PyThreadState *);

extern void _PyEval_DeactivateOpCache(void);


/* --- _Py_EnterRecursiveCall() ----------------------------------------- */

#ifdef USE_STACKCHECK
/* With USE_STACKCHECK macro defined, trigger stack checks in
   _Py_CheckRecursiveCall() on every 64th call to _Py_EnterRecursiveCall. */
static inline int _Py_MakeRecCheck(PyThreadState *tstate)  {
    return (tstate->c_recursion_remaining-- <= 0
            || (tstate->c_recursion_remaining & 63) == 0);
}
#else
static inline int _Py_MakeRecCheck(PyThreadState *tstate) {
    return tstate->c_recursion_remaining-- <= 0;
}
#endif

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

static inline void _Py_LeaveRecursiveCallTstate(PyThreadState *tstate)  {
    tstate->c_recursion_remaining++;
}

static inline void _Py_LeaveRecursiveCall(void)  {
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_LeaveRecursiveCallTstate(tstate);
}

extern struct _PyInterpreterFrame* _PyEval_GetFrame(void);

extern PyObject* _Py_MakeCoro(PyFunctionObject *func);

/* Handle signals, pending calls, GIL drop request
   and asynchronous exception */
extern int _Py_HandlePending(PyThreadState *tstate);

extern PyObject * _PyEval_GetFrameLocals(void);

extern const binaryfunc _PyEval_BinaryOps[];
int _PyEval_CheckExceptStarTypeValid(PyThreadState *tstate, PyObject* right);
int _PyEval_CheckExceptTypeValid(PyThreadState *tstate, PyObject* right);
int _PyEval_ExceptionGroupMatch(PyObject* exc_value, PyObject *match_type, PyObject **match, PyObject **rest);
void _PyEval_FormatAwaitableError(PyThreadState *tstate, PyTypeObject *type, int oparg);
void _PyEval_FormatExcCheckArg(PyThreadState *tstate, PyObject *exc, const char *format_str, PyObject *obj);
void _PyEval_FormatExcUnbound(PyThreadState *tstate, PyCodeObject *co, int oparg);
void _PyEval_FormatKwargsError(PyThreadState *tstate, PyObject *func, PyObject *kwargs);
PyObject *_PyEval_MatchClass(PyThreadState *tstate, PyObject *subject, PyObject *type, Py_ssize_t nargs, PyObject *kwargs);
PyObject *_PyEval_MatchKeys(PyThreadState *tstate, PyObject *map, PyObject *keys);
int _PyEval_UnpackIterable(PyThreadState *tstate, PyObject *v, int argcnt, int argcntafter, PyObject **sp);
void _PyEval_FrameClearAndPop(PyThreadState *tstate, _PyInterpreterFrame *frame);


#define _PY_GIL_DROP_REQUEST_BIT 0
#define _PY_SIGNALS_PENDING_BIT 1
#define _PY_CALLS_TO_DO_BIT 2
#define _PY_ASYNC_EXCEPTION_BIT 3
#define _PY_GC_SCHEDULED_BIT 4

/* Reserve a few bits for future use */
#define _PY_EVAL_EVENTS_BITS 8
#define _PY_EVAL_EVENTS_MASK ((1 << _PY_EVAL_EVENTS_BITS)-1)

static inline void
_Py_set_eval_breaker_bit(PyInterpreterState *interp, uint32_t bit, uint32_t set)
{
    assert(set == 0 || set == 1);
    uintptr_t to_set = set << bit;
    uintptr_t mask = ((uintptr_t)1) << bit;
    uintptr_t old = _Py_atomic_load_uintptr(&interp->ceval.eval_breaker);
    if ((old & mask) == to_set) {
        return;
    }
    uintptr_t new;
    do {
        new = (old & ~mask) | to_set;
    } while (!_Py_atomic_compare_exchange_uintptr(&interp->ceval.eval_breaker, &old, new));
}

static inline bool
_Py_eval_breaker_bit_is_set(PyInterpreterState *interp, int32_t bit)
{
    return _Py_atomic_load_uintptr_relaxed(&interp->ceval.eval_breaker) & (((uintptr_t)1) << bit);
}


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_H */
