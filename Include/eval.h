
/* Interface to execute compiled code */

#ifndef Py_EVAL_H
#define Py_EVAL_H
#ifdef __cplusplus
extern "C" {
#endif

DL_IMPORT(PyObject *) PyEval_EvalCode(PyCodeObject *, PyObject *, PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_EVAL_H */
