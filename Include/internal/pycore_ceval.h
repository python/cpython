#ifndef Py_INTERNAL_CEVAL_H
#define Py_INTERNAL_CEVAL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Forward declarations */
struct pyruntimestate;
struct _ceval_runtime_state;

#include "pycore_interp.h"   /* PyInterpreterState.eval_frame */

extern void _Py_FinishPendingCalls(PyThreadState *tstate);
extern void _PyEval_InitRuntimeState(struct _ceval_runtime_state *);
extern int _PyEval_InitState(struct _ceval_state *ceval);
extern void _PyEval_FiniState(struct _ceval_state *ceval);
PyAPI_FUNC(void) _PyEval_SignalReceived(PyInterpreterState *interp);
PyAPI_FUNC(int) _PyEval_AddPendingCall(
    PyInterpreterState *interp,
    int (*func)(void *),
    void *arg);
PyAPI_FUNC(void) _PyEval_SignalAsyncExc(PyThreadState *tstate);
#ifdef HAVE_FORK
extern void _PyEval_ReInitThreads(struct pyruntimestate *runtime);
#endif
PyAPI_FUNC(void) _PyEval_SetCoroutineOriginTrackingDepth(
    PyThreadState *tstate,
    int new_depth);

/* Private function */
void _PyEval_Fini(void);

static inline PyObject*
_PyEval_EvalFrame(PyThreadState *tstate, PyFrameObject *f, int throwflag)
{
    return tstate->interp->eval_frame(tstate, f, throwflag);
}

extern PyObject *_PyEval_EvalCode(
    PyThreadState *tstate,
    PyObject *_co, PyObject *globals, PyObject *locals,
    PyObject *const *args, Py_ssize_t argcount,
    PyObject *const *kwnames, PyObject *const *kwargs,
    Py_ssize_t kwcount, int kwstep,
    PyObject *const *defs, Py_ssize_t defcount,
    PyObject *kwdefs, PyObject *closure,
    PyObject *name, PyObject *qualname);

extern int _PyEval_ThreadsInitialized(struct pyruntimestate *runtime);
extern PyStatus _PyEval_InitGIL(PyThreadState *tstate);
extern void _PyEval_FiniGIL(PyThreadState *tstate);

extern void _PyEval_ReleaseLock(PyThreadState *tstate);


/* --- _Py_EnterRecursiveCall() ----------------------------------------- */

PyAPI_DATA(int) _Py_CheckRecursionLimit;

#ifdef USE_STACKCHECK
/* With USE_STACKCHECK macro defined, trigger stack checks in
   _Py_CheckRecursiveCall() on every 64th call to Py_EnterRecursiveCall. */
static inline int _Py_MakeRecCheck(PyThreadState *tstate)  {
    return (++tstate->recursion_depth > tstate->interp->ceval.recursion_limit
            || ++tstate->stackcheck_counter > 64);
}
#else
static inline int _Py_MakeRecCheck(PyThreadState *tstate) {
    return (++tstate->recursion_depth > tstate->interp->ceval.recursion_limit);
}
#endif

PyAPI_FUNC(int) _Py_CheckRecursiveCall(
    PyThreadState *tstate,
    const char *where);

static inline int _Py_EnterRecursiveCall(PyThreadState *tstate,
                                         const char *where) {
    return (_Py_MakeRecCheck(tstate) && _Py_CheckRecursiveCall(tstate, where));
}

static inline int _Py_EnterRecursiveCall_inline(const char *where) {
    PyThreadState *tstate = PyThreadState_GET();
    return _Py_EnterRecursiveCall(tstate, where);
}

#define Py_EnterRecursiveCall(where) _Py_EnterRecursiveCall_inline(where)

/* Compute the "lower-water mark" for a recursion limit. When
 * Py_LeaveRecursiveCall() is called with a recursion depth below this mark,
 * the overflowed flag is reset to 0. */
static inline int _Py_RecursionLimitLowerWaterMark(int limit) {
    if (limit > 200) {
        return (limit - 50);
    }
    else {
        return (3 * (limit >> 2));
    }
}

static inline void _Py_LeaveRecursiveCall(PyThreadState *tstate)  {
    tstate->recursion_depth--;
    int limit = tstate->interp->ceval.recursion_limit;
    if (tstate->recursion_depth < _Py_RecursionLimitLowerWaterMark(limit)) {
        tstate->overflowed = 0;
    }
}

static inline void _Py_LeaveRecursiveCall_inline(void)  {
    PyThreadState *tstate = PyThreadState_GET();
    _Py_LeaveRecursiveCall(tstate);
}

#define Py_LeaveRecursiveCall() _Py_LeaveRecursiveCall_inline()


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_H */
