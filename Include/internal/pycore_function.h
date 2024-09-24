#ifndef Py_INTERNAL_FUNCTION_H
#define Py_INTERNAL_FUNCTION_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pycore_lock.h"

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

struct _func_version_cache_item {
    PyFunctionObject *func;
    PyObject *code;
};

struct _py_func_state {
#ifdef Py_GIL_DISABLED
    // Protects next_version
    PyMutex mutex;
#endif

    uint32_t next_version;
    // Borrowed references to function and code objects whose
    // func_version % FUNC_VERSION_CACHE_SIZE
    // once was equal to the index in the table.
    // They are cleared when the function or code object is deallocated.
    struct _func_version_cache_item func_version_cache[FUNC_VERSION_CACHE_SIZE];
};

extern PyFunctionObject* _PyFunction_FromConstructor(PyFrameConstructor *constr);

extern uint32_t _PyFunction_GetVersionForCurrentState(PyFunctionObject *func);
PyAPI_FUNC(void) _PyFunction_SetVersion(PyFunctionObject *func, uint32_t version);
void _PyFunction_ClearCodeByVersion(uint32_t version);
PyFunctionObject *_PyFunction_LookupByVersion(uint32_t version, PyObject **p_code);

extern PyObject *_Py_set_function_type_params(
    PyThreadState* unused, PyObject *func, PyObject *type_params);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FUNCTION_H */
