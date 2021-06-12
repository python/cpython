#ifndef Py_INTERNAL_JUMPTABLE_H
#define Py_INTERNAL_JUMPTABE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

PyAPI_FUNC(int) _PyJumpTable_Get(PyObject *mapping, PyObject *key);
PyAPI_FUNC(PyObject *) _PyJumpTable_New(PyObject *mapping);
PyAPI_FUNC(int) _PyJumpTable_Check(PyObject *x);
PyAPI_FUNC(PyObject *) _PyJumpTable_DictCopy(PyObject *mapping);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JUMPTABLE_H */
