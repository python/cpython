#ifndef Py_INTERNAL_RUNTIME_H
#define Py_INTERNAL_RUNTIME_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_atexit.h"          // struct _atexit_runtime_state
#include "pycore_audit.h"           // _Py_AuditHookEntry
#include "pycore_ceval_state.h"     // struct _ceval_runtime_state
#include "pycore_crossinterp.h"     // _PyXI_global_state_t
#include "pycore_debug_offsets.h"   // _Py_DebugOffsets
#include "pycore_faulthandler.h"    // struct _faulthandler_runtime_state
#include "pycore_floatobject.h"     // struct _Py_float_runtime_state
#include "pycore_import.h"          // struct _import_runtime_state
#include "pycore_interp.h"          // PyInterpreterState
#include "pycore_object_state.h"    // struct _py_object_runtime_state
#include "pycore_parser.h"          // struct _parser_runtime_state
#include "pycore_pyhash.h"          // struct pyhash_runtime_state
#include "pycore_pymem.h"           // struct _pymem_allocators
#include "pycore_pythread.h"        // struct _pythread_runtime_state
#include "pycore_signal.h"          // struct _signals_runtime_state
#include "pycore_time.h"            // struct _PyTime_runtime_state
#include "pycore_tracemalloc.h"     // struct _tracemalloc_runtime_state
#include "pycore_typeobject.h"      // struct _types_runtime_state
#include "pycore_unicodeobject.h"   // struct _Py_unicode_runtime_state


/* Full Python runtime state */

/* _PyRuntimeState holds the global state for the CPython runtime.
   That data is exported by the internal API as a global variable
   (_PyRuntime, defined near the top of pylifecycle.c).
   */
typedef struct pyruntimestate {
    /* This field must be first to facilitate locating it by out of process
     * debuggers. Out of process debuggers will use the offsets contained in this
     * field to be able to locate other fields in several interpreter structures
     * in a way that doesn't require them to know the exact layout of those
     * structures.
     *
     * IMPORTANT:
     * This struct is **NOT** backwards compatible between minor version of the
     * interpreter and the members, order of members and size can change between
     * minor versions. This struct is only guaranteed to be stable between patch
     * versions for a given minor version of the interpreter.
     */
    _Py_DebugOffsets debug_offsets;

    /* Has been initialized to a safe state.

       In order to be effective, this must be set to 0 during or right
       after allocation. */
    int _initialized;

    /* Is running Py_PreInitialize()? */
    int preinitializing;

    /* Is Python preinitialized? Set to 1 by Py_PreInitialize() */
    int preinitialized;

    /* Is Python core initialized? Set to 1 by _Py_InitializeCore() */
    int core_initialized;

    /* Is Python fully initialized? Set to 1 by Py_Initialize() */
    int initialized;

    /* Set by Py_FinalizeEx(). Only reset to NULL if Py_Initialize()
       is called again.

       Use _PyRuntimeState_GetFinalizing() and _PyRuntimeState_SetFinalizing()
       to access it, don't access it directly. */
    PyThreadState *_finalizing;
    /* The ID of the OS thread in which we are finalizing. */
    unsigned long _finalizing_id;

    struct pyinterpreters {
        PyMutex mutex;
        /* The linked list of interpreters, newest first. */
        PyInterpreterState *head;
        /* The runtime's initial interpreter, which has a special role
           in the operation of the runtime.  It is also often the only
           interpreter. */
        PyInterpreterState *main;
        /* next_id is an auto-numbered sequence of small
           integers.  It gets initialized in _PyInterpreterState_Enable(),
           which is called in Py_Initialize(), and used in
           PyInterpreterState_New().  A negative interpreter ID
           indicates an error occurred.  The main interpreter will
           always have an ID of 0.  Overflow results in a RuntimeError.
           If that becomes a problem later then we can adjust, e.g. by
           using a Python int. */
        int64_t next_id;
    } interpreters;

    /* Platform-specific identifier and PyThreadState, respectively, for the
       main thread in the main interpreter. */
    unsigned long main_thread;
    PyThreadState *main_tstate;

    /* ---------- IMPORTANT ---------------------------
     The fields above this line are declared as early as
     possible to facilitate out-of-process observability
     tools. */

    /* cross-interpreter data and utils */
    _PyXI_global_state_t xi;

    struct _pymem_allocators allocators;
    struct _obmalloc_global_state obmalloc;
    struct pyhash_runtime_state pyhash_state;
    struct _pythread_runtime_state threads;
    struct _signals_runtime_state signals;

    /* Used for the thread state bound to the current thread. */
    Py_tss_t autoTSSkey;

    /* Used instead of PyThreadState.trash when there is not current tstate. */
    Py_tss_t trashTSSkey;

    PyWideStringList orig_argv;

    struct _parser_runtime_state parser;

    struct _atexit_runtime_state atexit;

    struct _import_runtime_state imports;
    struct _ceval_runtime_state ceval;
    struct _gilstate_runtime_state {
        /* bpo-26558: Flag to disable PyGILState_Check().
           If set to non-zero, PyGILState_Check() always return 1. */
        int check_enabled;
        /* The single PyInterpreterState used by this process'
           GILState implementation
        */
        /* TODO: Given interp_main, it may be possible to kill this ref */
        PyInterpreterState *autoInterpreterState;
    } gilstate;
    struct _getargs_runtime_state {
        struct _PyArg_Parser *static_parsers;
    } getargs;
    struct _fileutils_state fileutils;
    struct _faulthandler_runtime_state faulthandler;
    struct _tracemalloc_runtime_state tracemalloc;
    struct _reftracer_runtime_state ref_tracer;

    // The rwmutex is used to prevent overlapping global and per-interpreter
    // stop-the-world events. Global stop-the-world events lock the mutex
    // exclusively (as a "writer"), while per-interpreter stop-the-world events
    // lock it non-exclusively (as "readers").
    _PyRWMutex stoptheworld_mutex;
    struct _stoptheworld_state stoptheworld;

    PyPreConfig preconfig;

    // Audit values must be preserved when Py_Initialize()/Py_Finalize()
    // is called multiple times.
    Py_OpenCodeHookFunction open_code_hook;
    void *open_code_userdata;
    struct {
        PyMutex mutex;
        _Py_AuditHookEntry *head;
    } audit_hooks;

    struct _py_object_runtime_state object_state;
    struct _Py_float_runtime_state float_state;
    struct _Py_unicode_runtime_state unicode_state;
    struct _types_runtime_state types;
    struct _Py_time_runtime_state time;

#if defined(__EMSCRIPTEN__) && defined(PY_CALL_TRAMPOLINE)
    // Used in "Python/emscripten_trampoline.c" to choose between type
    // reflection trampoline and EM_JS trampoline.
    int (*emscripten_count_args_function)(PyCFunctionWithKeywords func);
#endif

    /* All the objects that are shared by the runtime's interpreters. */
    struct _Py_cached_objects cached_objects;
    struct _Py_static_objects static_objects;

    /* The following fields are here to avoid allocation during init.
       The data is exposed through _PyRuntimeState pointer fields.
       These fields should not be accessed directly outside of init.

       All other _PyRuntimeState pointer fields are populated when
       needed and default to NULL.

       For now there are some exceptions to that rule, which require
       allocation during init.  These will be addressed on a case-by-case
       basis.  Most notably, we don't pre-allocated the several mutex
       (PyThread_type_lock) fields, because on Windows we only ever get
       a pointer type.
       */

    /* _PyRuntimeState.interpreters.main */
    PyInterpreterState _main_interpreter;
    // _main_interpreter should be the last field of _PyRuntimeState.
    // See https://github.com/python/cpython/issues/127117.
} _PyRuntimeState;


/* other API */

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
