#ifndef Py_INTERNAL_FUNCTION_H
#define Py_INTERNAL_FUNCTION_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define FUNC_MAX_WATCHERS 8

struct _py_func_state {
    uint32_t next_version;
};

extern PyFunctionObject* _PyFunction_FromConstructor(PyFrameConstructor *constr);

extern uint32_t _PyFunction_GetVersionForCurrentState(PyFunctionObject *func);
extern PyObject *_Py_set_function_type_params(
    PyThreadState* unused, PyObject *func, PyObject *type_params);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FUNCTION_H */
