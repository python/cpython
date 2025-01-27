#ifndef Py_CPYTHON_SYS_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(PyObject *) PySys_GetAttr(PyObject *name);
PyAPI_FUNC(PyObject *) PySys_GetAttrString(const char *name);
