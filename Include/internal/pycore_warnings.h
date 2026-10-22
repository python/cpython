#ifndef Py_INTERNAL_WARNINGS_H
#define Py_INTERNAL_WARNINGS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern int _PyWarnings_InitState(PyInterpreterState *interp);

extern PyObject* _PyWarnings_Init(void);

extern void _PyErr_WarnUnawaitedCoroutine(PyObject *coro);
extern void _PyErr_WarnUnawaitedAgenMethod(PyAsyncGenObject *agen, PyObject *method);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_WARNINGS_H */
