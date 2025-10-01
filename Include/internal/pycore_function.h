#ifndef Py_INTERNAL_FUNCTION_H
#define Py_INTERNAL_FUNCTION_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyObject* _PyFunction_Vectorcall(
    PyObject *func,
    PyObject *const *stack,
    size_t nargsf,
    PyObject *kwnames);


#define FUNC_VERSION_UNSET 0
#define FUNC_VERSION_CLEARED 1
#define FUNC_VERSION_FIRST_VALID 2

extern PyFunctionObject* _PyFunction_FromConstructor(PyFrameConstructor *constr);

static inline int
_PyFunction_IsVersionValid(uint32_t version)
{
    return version >= FUNC_VERSION_FIRST_VALID;
}

extern uint32_t _PyFunction_GetVersionForCurrentState(PyFunctionObject *func);
PyAPI_FUNC(void) _PyFunction_SetVersion(PyFunctionObject *func, uint32_t version);
void _PyFunction_ClearCodeByVersion(uint32_t version);
PyFunctionObject *_PyFunction_LookupByVersion(uint32_t version, PyObject **p_code);

extern PyObject *_Py_set_function_type_params(
    PyThreadState* unused, PyObject *func, PyObject *type_params);


/* See pycore_code.h for explanation about what "stateless" means. */

PyAPI_FUNC(int)
_PyFunction_VerifyStateless(PyThreadState *, PyObject *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FUNCTION_H */
