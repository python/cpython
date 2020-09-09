#ifndef Py_INTERNAL_UNIONOBJECT_H
#define Py_INTERNAL_UNIONOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

PyAPI_FUNC(PyObject *) _Py_Union(PyObject *args);
PyAPI_DATA(PyTypeObject) _Py_UnionType;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNIONOBJECT_H */
