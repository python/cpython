#ifndef Py_INTERNAL_MODSUPPORT_H
#define Py_INTERNAL_MODSUPPORT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


extern int _PyArg_NoKwnames(const char *funcname, PyObject *kwnames);
#define _PyArg_NoKwnames(funcname, kwnames) \
    ((kwnames) == NULL || _PyArg_NoKwnames((funcname), (kwnames)))

extern PyObject ** _Py_VaBuildStack(
    PyObject **small_stack,
    Py_ssize_t small_stack_len,
    const char *format,
    va_list va,
    Py_ssize_t *p_nargs);

extern PyObject* _PyModule_CreateInitialized(PyModuleDef*, int apiver);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_MODSUPPORT_H

