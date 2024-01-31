#ifndef Py_CPYTHON_INTERPRETERIDOBJECT_H
#  error "this header file must not be included directly"
#endif

/* Interpreter ID Object */

PyAPI_DATA(PyTypeObject) PyInterpreterID_Type;

PyAPI_FUNC(PyObject *) PyInterpreterID_New(int64_t);
PyAPI_FUNC(PyObject *) PyInterpreterState_GetIDObject(PyInterpreterState *);
PyAPI_FUNC(PyInterpreterState *) PyInterpreterID_LookUp(PyObject *);
