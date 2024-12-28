#ifndef Py_INTERNAL_OBJECT_STATE_H
#define Py_INTERNAL_OBJECT_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist_state.h"  // _Py_freelists
#include "pycore_hashtable.h"       // _Py_hashtable_t


/* Reference tracer state */
struct _reftracer_runtime_state {
    PyRefTracer tracer_func;
    void* tracer_data;
};


struct _py_object_runtime_state {
#ifdef Py_REF_DEBUG
    Py_ssize_t interpreter_leaks;
#endif
    int _not_used;
};

struct _py_object_state {
#if !defined(Py_GIL_DISABLED)
    struct _Py_freelists freelists;
#endif
#ifdef Py_REF_DEBUG
    Py_ssize_t reftotal;
#endif
#ifdef Py_TRACE_REFS
    // Hash table storing all objects. The key is the object pointer
    // (PyObject*) and the value is always the number 1 (as uintptr_t).
    // See _PyRefchain_IsTraced() and _PyRefchain_Trace() functions.
    _Py_hashtable_t *refchain;
#endif
    int _not_used;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OBJECT_STATE_H */
