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
struct _frame;

#include "pycore_pystate.h"   /* PyInterpreterState.eval_frame */

extern void _Py_FinishPendingCalls(PyThreadState *tstate);
extern void _PyEval_InitRuntimeState(struct _ceval_runtime_state *);
extern void _PyEval_InitState(struct _ceval_state *);
extern void _PyEval_FiniThreads(
    struct _ceval_runtime_state *ceval);
PyAPI_FUNC(void) _PyEval_SignalReceived(
    struct _ceval_runtime_state *ceval);
PyAPI_FUNC(int) _PyEval_AddPendingCall(
    PyThreadState *tstate,
    struct _ceval_runtime_state *ceval,
    int (*func)(void *),
    void *arg);
PyAPI_FUNC(void) _PyEval_SignalAsyncExc(
    struct _ceval_runtime_state *ceval);
PyAPI_FUNC(void) _PyEval_ReInitThreads(
    struct pyruntimestate *runtime);
PyAPI_FUNC(void) _PyEval_SetCoroutineOriginTrackingDepth(
    PyThreadState *tstate,
    int new_depth);

/* Private function */
void _PyEval_Fini(void);

static inline PyObject*
_PyEval_EvalFrame(PyThreadState *tstate, struct _frame *f, int throwflag)
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

extern int _PyEval_ThreadsInitialized(_PyRuntimeState *runtime);
extern PyStatus _PyEval_InitThreads(PyThreadState *tstate);


/* --- _Py_EnterRecursiveCall() ----------------------------------------- */

PyAPI_DATA(int) _Py_CheckRecursionLimit;

#ifdef USE_STACKCHECK
/* With USE_STACKCHECK macro defined, trigger stack checks in
   _Py_CheckRecursiveCall() on every 64th call to Py_EnterRecursiveCall. */
static inline int _Py_MakeRecCheck(PyThreadState *tstate)  {
    return (++tstate->recursion_depth > _Py_CheckRecursionLimit
            || ++tstate->stackcheck_counter > 64);
}
#else
static inline int _Py_MakeRecCheck(PyThreadState *tstate) {
    return (++tstate->recursion_depth > _Py_CheckRecursionLimit);
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
#define _Py_RecursionLimitLowerWaterMark(limit) \
    (((limit) > 200) \
        ? ((limit) - 50) \
        : (3 * ((limit) >> 2)))

#define _Py_MakeEndRecCheck(x) \
    (--(x) < _Py_RecursionLimitLowerWaterMark(_Py_CheckRecursionLimit))

static inline void _Py_LeaveRecursiveCall(PyThreadState *tstate)  {
    if (_Py_MakeEndRecCheck(tstate->recursion_depth)) {
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
