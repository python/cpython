#ifndef Py_INTERNAL_RUNTIME_H
#define Py_INTERNAL_RUNTIME_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_runtime_structs.h" // _PyRuntimeState


/* API */

// Export _PyRuntime for shared extensions which use it in static inline
// functions for best performance, like _Py_IsMainThread() or _Py_ID().
// It's also made accessible for debuggers and profilers.
PyAPI_DATA(_PyRuntimeState) _PyRuntime;

extern PyStatus _PyRuntimeState_Init(_PyRuntimeState *runtime);
extern void _PyRuntimeState_Fini(_PyRuntimeState *runtime);

#ifdef HAVE_FORK
extern PyStatus _PyRuntimeState_ReInitThreads(_PyRuntimeState *runtime);
#endif

/* Initialize _PyRuntimeState.
   Return NULL on success, or return an error message on failure. */
extern PyStatus _PyRuntime_Initialize(void);

extern void _PyRuntime_Finalize(void);


static inline PyThreadState*
_PyRuntimeState_GetFinalizing(_PyRuntimeState *runtime) {
    return (PyThreadState*)_Py_atomic_load_ptr_relaxed(&runtime->_finalizing);
}

static inline unsigned long
_PyRuntimeState_GetFinalizingID(_PyRuntimeState *runtime) {
    return _Py_atomic_load_ulong_relaxed(&runtime->_finalizing_id);
}

static inline void
_PyRuntimeState_SetFinalizing(_PyRuntimeState *runtime, PyThreadState *tstate) {
    _Py_atomic_store_ptr_relaxed(&runtime->_finalizing, tstate);
    if (tstate == NULL) {
        _Py_atomic_store_ulong_relaxed(&runtime->_finalizing_id, 0);
    }
    else {
        // XXX Re-enable this assert once gh-109860 is fixed.
        //assert(tstate->thread_id == PyThread_get_thread_ident());
        _Py_atomic_store_ulong_relaxed(&runtime->_finalizing_id,
                                       tstate->thread_id);
    }
}


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_RUNTIME_H */
