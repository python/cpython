#ifndef Py_SEMISTABLE_PYSTATE_H
#  error "this header file must not be included directly"
#endif

/* Frame evaluation API (added for PEP-523) */

struct _PyInterpreterFrame;
typedef PyObject* (*PyFrameEvalFunction)(PyThreadState *tstate, struct _PyInterpreterFrame *, int);

_Py_NEWLY_SEMISTABLE(3.11)
PyAPI_FUNC(PyFrameEvalFunction) PyInterpreterState_GetEvalFrameFunc(
    PyInterpreterState *interp);

_Py_NEWLY_SEMISTABLE(3.11)
PyAPI_FUNC(void) PyInterpreterState_SetEvalFrameFunc(
    PyInterpreterState *interp,
    PyFrameEvalFunction eval_frame);

/* Underscore-prefixed names for the above.
 * To be removed when the API changes.
 */
#define _PyFrameEvalFunction PyFrameEvalFunction
#define _PyInterpreterState_GetEvalFrameFunc PyInterpreterState_GetEvalFrameFunc
#define _PyInterpreterState_SetEvalFrameFunc PyInterpreterState_SetEvalFrameFunc
