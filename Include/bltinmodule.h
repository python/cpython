#ifndef Py_BLTINMODULE_H
#define Py_BLTINMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyFilter_Type;
PyAPI_DATA(PyTypeObject) PyMap_Type;
PyAPI_DATA(PyTypeObject) PyZip_Type;

#ifdef Py_BUILD_CORE
PyObject* _PyBuiltin_Print(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_BLTINMODULE_H */
