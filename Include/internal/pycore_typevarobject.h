#ifndef Py_INTERNAL_TYPEVAROBJECT_H
#define Py_INTERNAL_TYPEVAROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyObject *_Py_make_typevar(PyObject *, PyObject *, PyObject *);
extern PyObject *_Py_make_paramspec(PyThreadState *, PyObject *);
extern PyObject *_Py_make_typevartuple(PyThreadState *, PyObject *);
extern PyObject *_Py_make_typealias(PyThreadState *, PyObject *);
extern PyObject *_Py_subscript_generic(PyThreadState *, PyObject *);
extern int _Py_initialize_generic(PyInterpreterState *);
extern void _Py_clear_generic_types(PyInterpreterState *);

extern PyTypeObject _PyTypeAlias_Type;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEVAROBJECT_H */
