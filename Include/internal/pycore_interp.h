#ifndef Py_INTERNAL_INTERP_H
#define Py_INTERNAL_INTERP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>              // bool

#include "pycore_runtime_structs.h"
#include "pycore_code.h"          // struct callable_cache
#include "pycore_floatobject.h"   // struct _Py_float_state
#include "pycore_gc.h"            // struct _gc_runtime_state
#include "pycore_genobject.h"     // _PyGen_FetchStopIterationValue
#include "pycore_import.h"        // struct _import_state
#include "pycore_optimizer.h"     // _PyExecutorObject
#include "pycore_typeobject.h"    // struct types_state
#include "pycore_unicodeobject.h" // struct _Py_unicode_state
#include "pycore_warnings.h"      // struct _warnings_runtime_state


/* interpreter state */

#define _PyInterpreterState_WHENCE_NOTSET -1
#define _PyInterpreterState_WHENCE_UNKNOWN 0
#define _PyInterpreterState_WHENCE_RUNTIME 1
#define _PyInterpreterState_WHENCE_LEGACY_CAPI 2
#define _PyInterpreterState_WHENCE_CAPI 3
#define _PyInterpreterState_WHENCE_XI 4
#define _PyInterpreterState_WHENCE_STDLIB 5
#define _PyInterpreterState_WHENCE_MAX 5


/* other API */

extern void _PyInterpreterState_Clear(PyThreadState *tstate);


static inline PyThreadState*
_PyInterpreterState_GetFinalizing(PyInterpreterState *interp) {
    return (PyThreadState*)_Py_atomic_load_ptr_relaxed(&interp->_finalizing);
}

static inline unsigned long
_PyInterpreterState_GetFinalizingID(PyInterpreterState *interp) {
    return _Py_atomic_load_ulong_relaxed(&interp->_finalizing_id);
}

static inline void
_PyInterpreterState_SetFinalizing(PyInterpreterState *interp, PyThreadState *tstate) {
    _Py_atomic_store_ptr_relaxed(&interp->_finalizing, tstate);
    if (tstate == NULL) {
        _Py_atomic_store_ulong_relaxed(&interp->_finalizing_id, 0);
    }
    else {
        // XXX Re-enable this assert once gh-109860 is fixed.
        //assert(tstate->thread_id == PyThread_get_thread_ident());
        _Py_atomic_store_ulong_relaxed(&interp->_finalizing_id,
                                       tstate->thread_id);
    }
}



// Exports for the _testinternalcapi module.
PyAPI_FUNC(int64_t) _PyInterpreterState_ObjectToID(PyObject *);
PyAPI_FUNC(PyInterpreterState *) _PyInterpreterState_LookUpID(int64_t);
PyAPI_FUNC(PyInterpreterState *) _PyInterpreterState_LookUpIDObject(PyObject *);
PyAPI_FUNC(void) _PyInterpreterState_IDIncref(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_IDDecref(PyInterpreterState *);

PyAPI_FUNC(int) _PyInterpreterState_IsReady(PyInterpreterState *interp);

PyAPI_FUNC(long) _PyInterpreterState_GetWhence(PyInterpreterState *interp);
extern void _PyInterpreterState_SetWhence(
    PyInterpreterState *interp,
    long whence);

/*
Runtime Feature Flags

Each flag indicate whether or not a specific runtime feature
is available in a given context.  For example, forking the process
might not be allowed in the current interpreter (i.e. os.fork() would fail).
*/

/* Set if the interpreter share obmalloc runtime state
   with the main interpreter. */
#define Py_RTFLAGS_USE_MAIN_OBMALLOC (1UL << 5)

/* Set if import should check a module for subinterpreter support. */
#define Py_RTFLAGS_MULTI_INTERP_EXTENSIONS (1UL << 8)

/* Set if threads are allowed. */
#define Py_RTFLAGS_THREADS (1UL << 10)

/* Set if daemon threads are allowed. */
#define Py_RTFLAGS_DAEMON_THREADS (1UL << 11)

/* Set if os.fork() is allowed. */
#define Py_RTFLAGS_FORK (1UL << 15)

/* Set if os.exec*() is allowed. */
#define Py_RTFLAGS_EXEC (1UL << 16)

extern int _PyInterpreterState_HasFeature(PyInterpreterState *interp,
                                          unsigned long feature);

PyAPI_FUNC(PyStatus) _PyInterpreterState_New(
    PyThreadState *tstate,
    PyInterpreterState **pinterp);

extern const PyConfig* _PyInterpreterState_GetConfig(
    PyInterpreterState *interp);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INTERP_H */
