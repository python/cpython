#ifndef Py_CPYTHON_LONGOBJECT_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(PyObject*) PyLong_FromUnicodeObject(PyObject *u, int base);

PyAPI_FUNC(int) PyUnstable_Long_IsCompact(const PyLongObject* op);
PyAPI_FUNC(Py_ssize_t) PyUnstable_Long_CompactValue(const PyLongObject* op);

