#ifndef Py_INTERNAL_INTERP_H
#define Py_INTERNAL_INTERP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>              // bool

#include "pycore_ast_state.h"     // struct ast_state
#include "pycore_atexit.h"        // struct atexit_state
#include "pycore_ceval_state.h"   // struct _ceval_state
#include "pycore_code.h"          // struct callable_cache
#include "pycore_context.h"       // struct _Py_context_state
#include "pycore_crossinterp.h"   // struct _xidregistry
#include "pycore_dict_state.h"    // struct _Py_dict_state
#include "pycore_dtoa.h"          // struct _dtoa_state
#include "pycore_exceptions.h"    // struct _Py_exc_state
#include "pycore_floatobject.h"   // struct _Py_float_state
#include "pycore_function.h"      // FUNC_MAX_WATCHERS
#include "pycore_gc.h"            // struct _gc_runtime_state
#include "pycore_genobject.h"     // struct _Py_async_gen_state
#include "pycore_global_objects.h"// struct _Py_interp_cached_objects
#include "pycore_import.h"        // struct _import_state
#include "pycore_instruments.h"   // _PY_MONITORING_EVENTS
#include "pycore_list.h"          // struct _Py_list_state
#include "pycore_mimalloc.h"      // struct _mimalloc_interp_state
#include "pycore_object_state.h"  // struct _py_object_state
#include "pycore_obmalloc.h"      // struct _obmalloc_state
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_tuple.h"         // struct _Py_tuple_state
#include "pycore_typeobject.h"    // struct types_state
#include "pycore_unicodeobject.h" // struct _Py_unicode_state
#include "pycore_warnings.h"      // struct _warnings_runtime_state


struct _Py_long_state {
    int max_str_digits;
};


/* cross-interpreter data registry */


/* interpreter state */

/* PyInterpreterState holds the global state for one of the runtime's
   interpreters.  Typically the initial (main) interpreter is the only one.

   The PyInterpreterState typedef is in Include/pytypedefs.h.
   */
struct _is {

    /* This struct countains the eval_breaker,
     * which is by far the hottest field in this struct
     * and should be placed at the beginning. */
    struct _ceval_state ceval;

    PyInterpreterState *next;

    int64_t id;
    int64_t id_refcount;
    int requires_idref;
    PyThread_type_lock id_mutex;

    /* Has been initialized to a safe state.

       In order to be effective, this must be set to 0 during or right
       after allocation. */
    int _initialized;
    int finalizing;

    uintptr_t last_restart_version;
    struct pythreads {
        uint64_t next_unique_id;
        /* The linked list of threads, newest first. */
        PyThreadState *head;
        /* The thread currently executing in the __main__ module, if any. */
        PyThreadState *main;
        /* Used in Modules/_threadmodule.c. */
        long count;
        /* Support for runtime thread stack size tuning.
           A value of 0 means using the platform's default stack size
           or the size specified by the THREAD_STACK_SIZE macro. */
        /* Used in Python/thread.c. */
        size_t stacksize;
    } threads;

    /* Reference to the _PyRuntime global variable. This field exists
       to not have to pass runtime in addition to tstate to a function.
       Get runtime from tstate: tstate->interp->runtime. */
    struct pyruntimestate *runtime;

    /* Set by Py_EndInterpreter().

       Use _PyInterpreterState_GetFinalizing()
       and _PyInterpreterState_SetFinalizing()
       to access it, don't access it directly. */
    PyThreadState* _finalizing;
    /* The ID of the OS thread in which we are finalizing. */
    unsigned long _finalizing_id;

    struct _gc_runtime_state gc;

    /* The following fields are here to avoid allocation during init.
       The data is exposed through PyInterpreterState pointer fields.
       These fields should not be accessed directly outside of init.

       All other PyInterpreterState pointer fields are populated when
       needed and default to NULL.

       For now there are some exceptions to that rule, which require
       allocation during init.  These will be addressed on a case-by-case
       basis.  Also see _PyRuntimeState regarding the various mutex fields.
       */

    // Dictionary of the sys module
    PyObject *sysdict;

    // Dictionary of the builtins module
    PyObject *builtins;

    struct _import_state imports;

    /* The per-interpreter GIL, which might not be used. */
    struct _gil_runtime_state _gil;

     /* ---------- IMPORTANT ---------------------------
     The fields above this line are declared as early as
     possible to facilitate out-of-process observability
     tools. */

    PyObject *codec_search_path;
    PyObject *codec_search_cache;
    PyObject *codec_error_registry;
    int codecs_initialized;

    PyConfig config;
    unsigned long feature_flags;

    PyObject *dict;  /* Stores per-interpreter state */

    PyObject *sysdict_copy;
    PyObject *builtins_copy;
    // Initialized to _PyEval_EvalFrameDefault().
    _PyFrameEvalFunction eval_frame;

    PyFunction_WatchCallback func_watchers[FUNC_MAX_WATCHERS];
    // One bit is set for each non-NULL entry in func_watchers
    uint8_t active_func_watchers;

    Py_ssize_t co_extra_user_count;
    freefunc co_extra_freefuncs[MAX_CO_EXTRA_USERS];

    /* cross-interpreter data and utils */
    struct _xi_state xi;

#ifdef HAVE_FORK
    PyObject *before_forkers;
    PyObject *after_forkers_parent;
    PyObject *after_forkers_child;
#endif

    struct _warnings_runtime_state warnings;
    struct atexit_state atexit;

#if defined(Py_GIL_DISABLED)
    struct _mimalloc_interp_state mimalloc;
#endif

    struct _obmalloc_state obmalloc;

    PyObject *audit_hooks;
    PyType_WatchCallback type_watchers[TYPE_MAX_WATCHERS];
    PyCode_WatchCallback code_watchers[CODE_MAX_WATCHERS];
    // One bit is set for each non-NULL entry in code_watchers
    uint8_t active_code_watchers;

#if !defined(Py_GIL_DISABLED)
    struct _Py_freelist_state freelist_state;
#endif
    struct _py_object_state object_state;
    struct _Py_unicode_state unicode;
    struct _Py_float_state float_state;
    struct _Py_long_state long_state;
    struct _dtoa_state dtoa;
    struct _py_func_state func_state;
    /* Using a cache is very effective since typically only a single slice is
       created and then deleted again. */
    PySliceObject *slice_cache;

    struct _Py_tuple_state tuple;
    struct _Py_dict_state dict_state;
    struct _Py_async_gen_state async_gen;
    struct _Py_context_state context;
    struct _Py_exc_state exc_state;

    struct ast_state ast;
    struct types_state types;
    struct callable_cache callable_cache;
    _PyOptimizerObject *optimizer;
    _PyExecutorObject *executor_list_head;
    uint16_t optimizer_resume_threshold;
    uint16_t optimizer_backedge_threshold;
    uint32_t next_func_version;

    _Py_GlobalMonitors monitors;
    bool sys_profile_initialized;
    bool sys_trace_initialized;
    Py_ssize_t sys_profiling_threads; /* Count of threads with c_profilefunc set */
    Py_ssize_t sys_tracing_threads; /* Count of threads with c_tracefunc set */
    PyObject *monitoring_callables[PY_MONITORING_TOOL_IDS][_PY_MONITORING_EVENTS];
    PyObject *monitoring_tool_names[PY_MONITORING_TOOL_IDS];

    struct _Py_interp_cached_objects cached_objects;
    struct _Py_interp_static_objects static_objects;

    /* the initial PyInterpreterState.threads.head */
    _PyThreadStateImpl _initial_thread;
    Py_ssize_t _interactive_src_count;
};


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


// Export for the _xxinterpchannels module.
PyAPI_FUNC(PyInterpreterState *) _PyInterpreterState_LookUpID(int64_t);

PyAPI_FUNC(int) _PyInterpreterState_IDInitref(PyInterpreterState *);
PyAPI_FUNC(int) _PyInterpreterState_IDIncref(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_IDDecref(PyInterpreterState *);

extern const PyConfig* _PyInterpreterState_GetConfig(PyInterpreterState *interp);

// Get a copy of the current interpreter configuration.
//
// Return 0 on success. Raise an exception and return -1 on error.
//
// The caller must initialize 'config', using PyConfig_InitPythonConfig()
// for example.
//
// Python must be preinitialized to call this method.
// The caller must hold the GIL.
//
// Once done with the configuration, PyConfig_Clear() must be called to clear
// it.
//
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(int) _PyInterpreterState_GetConfigCopy(
    struct PyConfig *config);

// Set the configuration of the current interpreter.
//
// This function should be called during or just after the Python
// initialization.
//
// Update the sys module with the new configuration. If the sys module was
// modified directly after the Python initialization, these changes are lost.
//
// Some configuration like faulthandler or warnoptions can be updated in the
// configuration, but don't reconfigure Python (don't enable/disable
// faulthandler and don't reconfigure warnings filters).
//
// Return 0 on success. Raise an exception and return -1 on error.
//
// The configuration should come from _PyInterpreterState_GetConfigCopy().
//
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(int) _PyInterpreterState_SetConfig(
    const struct PyConfig *config);


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


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INTERP_H */
