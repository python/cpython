#ifndef Py_UNIONTYPEOBJECT_H
#define Py_UNIONTYPEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) Py_Union(PyObject *);
PyAPI_DATA(PyTypeObject) Py_UnionType;

#ifdef __cplusplus
}
#endif
#endif /* !Py_UNIONTYPEOBJECT_H */
