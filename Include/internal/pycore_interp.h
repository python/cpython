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
#include "pycore_codecs.h"        // struct codecs_state
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
#include "pycore_optimizer.h"     // _PyOptimizerObject
#include "pycore_obmalloc.h"      // struct _obmalloc_state
#include "pycore_qsbr.h"          // struct _qsbr_state
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_tuple.h"         // struct _Py_tuple_state
#include "pycore_typeobject.h"    // struct types_state
#include "pycore_unicodeobject.h" // struct _Py_unicode_state
#include "pycore_warnings.h"      // struct _warnings_runtime_state


struct _Py_long_state {
    int max_str_digits;
};

// Support for stop-the-world events. This exists in both the PyRuntime struct
// for global pauses and in each PyInterpreterState for per-interpreter pauses.
struct _stoptheworld_state {
    PyMutex mutex;       // Serializes stop-the-world attempts.

    // NOTE: The below fields are protected by HEAD_LOCK(runtime), not by the
    // above mutex.
    bool requested;      // Set when a pause is requested.
    bool world_stopped;  // Set when the world is stopped.
    bool is_global;      // Set when contained in PyRuntime struct.

    PyEvent stop_event;  // Set when thread_countdown reaches zero.
    Py_ssize_t thread_countdown;  // Number of threads that must pause.

    PyThreadState *requester; // Thread that requested the pause (may be NULL).
};

#ifdef Py_GIL_DISABLED
// This should be prime but otherwise the choice is arbitrary. A larger value
// increases concurrency at the expense of memory.
#  define NUM_WEAKREF_LIST_LOCKS 127
#endif

/* cross-interpreter data registry */

/* Tracks some rare events per-interpreter, used by the optimizer to turn on/off
   specific optimizations. */
typedef struct _rare_events {
    /* Setting an object's class, obj.__class__ = ... */
    uint8_t set_class;
    /* Setting the bases of a class, cls.__bases__ = ... */
    uint8_t set_bases;
    /* Setting the PEP 523 frame eval function, _PyInterpreterState_SetFrameEvalFunc() */
    uint8_t set_eval_frame_func;
    /* Modifying the builtins,  __builtins__.__dict__[var] = ... */
    uint8_t builtin_dict;
    /* Modifying a function, e.g. func.__defaults__ = ..., etc. */
    uint8_t func_modification;
} _rare_events;

/* interpreter state */

/* PyInterpreterState holds the global state for one of the runtime's
   interpreters.  Typically the initial (main) interpreter is the only one.

   The PyInterpreterState typedef is in Include/pytypedefs.h.
   */
struct _is {

    /* This struct contains the eval_breaker,
     * which is by far the hottest field in this struct
     * and should be placed at the beginning. */
    struct _ceval_state ceval;

    PyInterpreterState *next;

    int64_t id;
    int64_t id_refcount;
    int requires_idref;
    PyThread_type_lock id_mutex;

#define _PyInterpreterState_WHENCE_NOTSET -1
#define _PyInterpreterState_WHENCE_UNKNOWN 0
#define _PyInterpreterState_WHENCE_RUNTIME 1
#define _PyInterpreterState_WHENCE_LEGACY_CAPI 2
#define _PyInterpreterState_WHENCE_CAPI 3
#define _PyInterpreterState_WHENCE_XI 4
#define _PyInterpreterState_WHENCE_STDLIB 5
#define _PyInterpreterState_WHENCE_MAX 5
    long _whence;

    /* Has been initialized to a safe state.

       In order to be effective, this must be set to 0 during or right
       after allocation. */
    int _initialized;
    /* Has been fully initialized via pylifecycle.c. */
    int _ready;
    int finalizing;

    uintptr_t last_restart_version;
    struct pythreads {
        uint64_t next_unique_id;
        /* The linked list of threads, newest first. */
        PyThreadState *head;
        /* The thread currently executing in the __main__ module, if any. */
        PyThreadState *main;
        /* Used in Modules/_threadmodule.c. */
        Py_ssize_t count;
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

    struct codecs_state codecs;

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
    struct _stoptheworld_state stoptheworld;
    struct _qsbr_shared qsbr;

#if defined(Py_GIL_DISABLED)
    struct _mimalloc_interp_state mimalloc;
    struct _brc_state brc;  // biased reference counting state
    PyMutex weakref_locks[NUM_WEAKREF_LIST_LOCKS];
#endif

    // Per-interpreter state for the obmalloc allocator.  For the main
    // interpreter and for all interpreters that don't have their
    // own obmalloc state, this points to the static structure in
    // obmalloc.c obmalloc_state_main.  For other interpreters, it is
    // heap allocated by _PyMem_init_obmalloc() and freed when the
    // interpreter structure is freed.  In the case of a heap allocated
    // obmalloc state, it is not safe to hold on to or use memory after
    // the interpreter is freed. The obmalloc state corresponding to
    // that allocated memory is gone.  See free_obmalloc_arenas() for
    // more comments.
    struct _obmalloc_state *obmalloc;

    PyObject *audit_hooks;
    PyType_WatchCallback type_watchers[TYPE_MAX_WATCHERS];
    PyCode_WatchCallback code_watchers[CODE_MAX_WATCHERS];
    // One bit is set for each non-NULL entry in code_watchers
    uint8_t active_code_watchers;

    struct _py_object_state object_state;
    struct _Py_unicode_state unicode;
    struct _Py_long_state long_state;
    struct _dtoa_state dtoa;
    struct _py_func_state func_state;
    struct _py_code_state code_state;

    struct _Py_dict_state dict_state;
    struct _Py_exc_state exc_state;
    struct _Py_mem_interp_free_queue mem_free_queue;

    struct ast_state ast;
    struct types_state types;
    struct callable_cache callable_cache;
    _PyOptimizerObject *optimizer;
    _PyExecutorObject *executor_list_head;

    _rare_events rare_events;
    PyDict_WatchCallback builtins_dict_watcher;

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



// Exports for the _testinternalcapi module.
PyAPI_FUNC(int64_t) _PyInterpreterState_ObjectToID(PyObject *);
PyAPI_FUNC(PyInterpreterState *) _PyInterpreterState_LookUpID(int64_t);
PyAPI_FUNC(PyInterpreterState *) _PyInterpreterState_LookUpIDObject(PyObject *);
PyAPI_FUNC(int) _PyInterpreterState_IDInitref(PyInterpreterState *);
PyAPI_FUNC(int) _PyInterpreterState_IDIncref(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_IDDecref(PyInterpreterState *);

PyAPI_FUNC(int) _PyInterpreterState_IsReady(PyInterpreterState *interp);

PyAPI_FUNC(long) _PyInterpreterState_GetWhence(PyInterpreterState *interp);
extern void _PyInterpreterState_SetWhence(
    PyInterpreterState *interp,
    long whence);

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


#define RARE_EVENT_INTERP_INC(interp, name) \
    do { \
        /* saturating add */ \
        int val = FT_ATOMIC_LOAD_UINT8_RELAXED(interp->rare_events.name); \
        if (val < UINT8_MAX) { \
            FT_ATOMIC_STORE_UINT8(interp->rare_events.name, val + 1); \
        } \
        RARE_EVENT_STAT_INC(name); \
    } while (0); \

#define RARE_EVENT_INC(name) \
    do { \
        PyInterpreterState *interp = PyInterpreterState_Get(); \
        RARE_EVENT_INTERP_INC(interp, name); \
    } while (0); \

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INTERP_H */
