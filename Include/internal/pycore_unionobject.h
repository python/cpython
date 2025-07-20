#ifndef Py_INTERNAL_UNIONOBJECT_H
#define Py_INTERNAL_UNIONOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// For extensions created by test_peg_generator
PyAPI_DATA(PyTypeObject) _PyUnion_Type;
PyAPI_FUNC(PyObject *) _Py_union_type_or(PyObject *, PyObject *);

#define _PyUnion_Check(op) Py_IS_TYPE((op), &_PyUnion_Type)

#define _PyGenericAlias_Check(op) PyObject_TypeCheck((op), &Py_GenericAliasType)
extern PyObject *_Py_subs_parameters(PyObject *, PyObject *, PyObject *, PyObject *);
extern PyObject *_Py_make_parameters(PyObject *);
extern PyObject *_Py_union_args(PyObject *self);
extern PyObject *_Py_union_from_tuple(PyObject *args);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNIONOBJECT_H */
