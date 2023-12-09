#ifndef Py_INTERNAL_INTERP_H
#define Py_INTERNAL_INTERP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>

#include "pycore_ast_state.h"     // struct ast_state
#include "pycore_atexit.h"        // struct atexit_state
#include "pycore_atomic.h"        // _Py_atomic_address
#include "pycore_ceval_state.h"   // struct _ceval_state
#include "pycore_code.h"          // struct callable_cache
#include "pycore_context.h"       // struct _Py_context_state
#include "pycore_dict_state.h"    // struct _Py_dict_state
#include "pycore_dtoa.h"          // struct _dtoa_state
#include "pycore_exceptions.h"    // struct _Py_exc_state
#include "pycore_floatobject.h"   // struct _Py_float_state
#include "pycore_function.h"      // FUNC_MAX_WATCHERS
#include "pycore_genobject.h"     // struct _Py_async_gen_state
#include "pycore_gc.h"            // struct _gc_runtime_state
#include "pycore_global_objects.h"  // struct _Py_interp_static_objects
#include "pycore_import.h"        // struct _import_state
#include "pycore_instruments.h"   // _PY_MONITORING_EVENTS
#include "pycore_list.h"          // struct _Py_list_state
#include "pycore_object_state.h"   // struct _py_object_state
#include "pycore_obmalloc.h"      // struct obmalloc_state
#include "pycore_tuple.h"         // struct _Py_tuple_state
#include "pycore_typeobject.h"    // struct type_cache
#include "pycore_unicodeobject.h" // struct _Py_unicode_state
#include "pycore_warnings.h"      // struct _warnings_runtime_state


struct _Py_long_state {
    int max_str_digits;
};

/* interpreter state */

/* PyInterpreterState holds the global state for one of the runtime's
   interpreters.  Typically the initial (main) interpreter is the only one.

   The PyInterpreterState typedef is in Include/pytypedefs.h.
   */
struct _is {

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

    uint64_t monitoring_version;
    uint64_t last_restart_version;
    struct pythreads {
        uint64_t next_unique_id;
        /* The linked list of threads, newest first. */
        PyThreadState *head;
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
    _Py_atomic_address _finalizing;

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

    struct _ceval_state ceval;

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

#ifdef HAVE_FORK
    PyObject *before_forkers;
    PyObject *after_forkers_parent;
    PyObject *after_forkers_child;
#endif

    struct _warnings_runtime_state warnings;
    struct atexit_state atexit;

    struct _obmalloc_state obmalloc;

    PyObject *audit_hooks;
    PyType_WatchCallback type_watchers[TYPE_MAX_WATCHERS];
    PyCode_WatchCallback code_watchers[CODE_MAX_WATCHERS];
    // One bit is set for each non-NULL entry in code_watchers
    uint8_t active_code_watchers;

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
    struct _Py_list_state list;
    struct _Py_dict_state dict_state;
    struct _Py_async_gen_state async_gen;
    struct _Py_context_state context;
    struct _Py_exc_state exc_state;

    struct ast_state ast;
    struct types_state types;
    struct callable_cache callable_cache;
    PyCodeObject *interpreter_trampoline;

    _Py_GlobalMonitors monitors;
    bool f_opcode_trace_set;
    bool sys_profile_initialized;
    bool sys_trace_initialized;
    Py_ssize_t sys_profiling_threads; /* Count of threads with c_profilefunc set */
    Py_ssize_t sys_tracing_threads; /* Count of threads with c_tracefunc set */
    PyObject *monitoring_callables[PY_MONITORING_TOOL_IDS][_PY_MONITORING_EVENTS];
    PyObject *monitoring_tool_names[PY_MONITORING_TOOL_IDS];

    struct _Py_interp_cached_objects cached_objects;
    struct _Py_interp_static_objects static_objects;

   /* the initial PyInterpreterState.threads.head */
    PyThreadState _initial_thread;
};


/* other API */

extern void _PyInterpreterState_Clear(PyThreadState *tstate);


static inline PyThreadState*
_PyInterpreterState_GetFinalizing(PyInterpreterState *interp) {
    return (PyThreadState*)_Py_atomic_load_relaxed(&interp->_finalizing);
}

static inline void
_PyInterpreterState_SetFinalizing(PyInterpreterState *interp, PyThreadState *tstate) {
    _Py_atomic_store_relaxed(&interp->_finalizing, (uintptr_t)tstate);
}


/* cross-interpreter data registry */

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   crossinterpdatafunc. It would be simpler and more efficient. */

struct _xidregitem;

struct _xidregitem {
    struct _xidregitem *prev;
    struct _xidregitem *next;
    PyObject *cls;  // weakref to a PyTypeObject
    crossinterpdatafunc getdata;
};

PyAPI_FUNC(PyInterpreterState*) _PyInterpreterState_LookUpID(int64_t);

PyAPI_FUNC(int) _PyInterpreterState_IDInitref(PyInterpreterState *);
PyAPI_FUNC(int) _PyInterpreterState_IDIncref(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_IDDecref(PyInterpreterState *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INTERP_H */
