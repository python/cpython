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

PyAPI_FUNC(void) _Py_FinishPendingCalls(PyThreadState *tstate);
PyAPI_FUNC(void) _PyEval_Initialize(struct _ceval_runtime_state *);
PyAPI_FUNC(void) _PyEval_FiniThreads(
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
    return tstate->interp->eval_frame(f, throwflag);
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

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CEVAL_H */
