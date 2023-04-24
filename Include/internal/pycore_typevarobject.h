#ifndef Py_INTERNAL_TYPEVAROBJECT_H
#define Py_INTERNAL_TYPEVAROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyTypeObject _PyTypeVar_Type;
extern PyTypeObject _PyTypeVarTuple_Type;
extern PyTypeObject _PyParamSpec_Type;
extern PyTypeObject _PyParamSpecArgs_Type;
extern PyTypeObject _PyParamSpecKwargs_Type;
extern PyTypeObject _PyTypeAlias_Type;

extern PyObject *_Py_make_typevar(const char *, PyObject *);
extern PyObject *_Py_make_paramspec(const char *);
extern PyObject *_Py_make_typevartuple(const char *);
extern PyObject *_Py_make_typealias(PyThreadState* unused, PyObject *);
extern PyObject *_Py_subscript_generic(PyObject *);
extern int _Py_initialize_generic(PyInterpreterState *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEVAROBJECT_H */
