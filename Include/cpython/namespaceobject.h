// Simple namespace object interface

#ifndef Py_LIMITED_API
#ifndef Py_NAMESPACEOBJECT_H
#define Py_NAMESPACEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) _PySimpleNamespace_Type;

PyAPI_FUNC(PyObject *) PySimpleNamespace_New(PyObject *attrs);

#ifdef __cplusplus
}
#endif
#endif  // !Py_NAMESPACEOBJECT_H
#endif  // !Py_LIMITED_API
