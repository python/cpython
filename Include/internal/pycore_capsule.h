#ifndef Py_INTERNAL_PYCAPSULE_H
#define Py_INTERNAL_PYCAPSULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_socket' shared extension
PyAPI_FUNC(int) _PyCapsule_SetTraverse(PyObject *op, traverseproc traverse_func, inquiry clear_func);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYCAPSULE_H */
