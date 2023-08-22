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

#define FUNC_MAX_WATCHERS 8

#define FUNC_VERSION_CACHE_SIZE (1<<12)  /* Must be a power of 2 */
struct _py_func_state {
    uint32_t next_version;
    // Borrowed references to function objects whose
    // func_version % FUNC_VERSION_CACHE_SIZE
    // once was equal to the index in the table.
    // They are cleared when the function is deallocated.
    PyFunctionObject *func_version_cache[FUNC_VERSION_CACHE_SIZE];
};

extern PyFunctionObject* _PyFunction_FromConstructor(PyFrameConstructor *constr);

extern uint32_t _PyFunction_GetVersionForCurrentState(PyFunctionObject *func);
extern void _PyFunction_SetVersion(PyFunctionObject *func, uint32_t version);
PyFunctionObject *_PyFunction_LookupByVersion(uint32_t version);

extern PyObject *_Py_set_function_type_params(
    PyThreadState* unused, PyObject *func, PyObject *type_params);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FUNCTION_H */
