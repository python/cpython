/* Function object interface */

#ifndef Py_LIMITED_API
#ifndef Py_FUNCOBJECT_H
#define Py_FUNCOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _functionobject PyFunctionObject;

PyAPI_DATA(PyTypeObject) PyFunction_Type;

#define PyFunction_Check(op) Py_IS_TYPE(op, &PyFunction_Type)

PyAPI_FUNC(PyObject *) PyFunction_New(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_NewWithQualName(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetCode(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetGlobals(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetModule(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetDefaults(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetKwDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetKwDefaults(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetClosure(PyObject *);
PyAPI_FUNC(int) PyFunction_SetClosure(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetAnnotations(PyObject *);
PyAPI_FUNC(int) PyFunction_SetAnnotations(PyObject *, PyObject *);

PyAPI_FUNC(PyObject *) _PyFunction_Vectorcall(
    PyObject *func,
    PyObject *const *stack,
    size_t nargsf,
    PyObject *kwnames);

/* Macros for backwards compatibility. */
#define PyFunction_GET_CODE(func) PyFunction_GetCode(func)
#define PyFunction_GET_GLOBALS(func) PyFunction_GetGlobals(func)
#define PyFunction_GET_MODULE(func) PyFunction_GetModule(func)
#define PyFunction_GET_DEFAULTS(func) PyFunction_GetDefaults(func)
#define PyFunction_GET_KW_DEFAULTS(func) PyFunction_GetKwDefaults(func)
#define PyFunction_GET_CLOSURE(func) PyFunction_GetClosure(func)
#define PyFunction_GET_ANNOTATIONS(func) PyFunction_GetAnnotations(func)

/* The classmethod and staticmethod types lives here, too */
PyAPI_DATA(PyTypeObject) PyClassMethod_Type;
PyAPI_DATA(PyTypeObject) PyStaticMethod_Type;

PyAPI_FUNC(PyObject *) PyClassMethod_New(PyObject *);
PyAPI_FUNC(PyObject *) PyStaticMethod_New(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_FUNCOBJECT_H */
#endif /* Py_LIMITED_API */
