#ifndef Py_INTERNAL_BLTINMODULE_H
#define Py_INTERNAL_BLTINMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyObject * _PyBuiltin_Init(PyInterpreterState *interp);
extern PyStatus _PyBuiltins_AddExceptions(PyObject * bltinmod,
                                           PyInterpreterState *interp);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BLTINMODULE_H */
