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
PyAPI_FUNC(PyObject *) _Py_union_type_or(PyObject* self, PyObject* param);

PyAPI_FUNC(PyObject *) _Py_subs_parameters(PyObject *, PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _Py_make_parameters(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNIONOBJECT_H */
