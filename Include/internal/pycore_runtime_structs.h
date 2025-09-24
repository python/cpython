/* This file contains the struct definitions for the runtime, interpreter
 * and thread states, plus all smaller structs contained therein */

#ifndef Py_INTERNAL_RUNTIME_STRUCTS_H
#define Py_INTERNAL_RUNTIME_STRUCTS_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pycore_interp_structs.h" // _PyGC_Head_UNUSED
#include "pycore_obmalloc.h"      // struct _obmalloc_global_state

/************ Runtime state ************/

typedef struct {
    /* We tag each block with an API ID in order to tag API violations */
    char api_id;
    PyMemAllocatorEx alloc;
} debug_alloc_api_t;

struct _pymem_allocators {
    PyMutex mutex;
    struct {
        PyMemAllocatorEx raw;
        PyMemAllocatorEx mem;
        PyMemAllocatorEx obj;
    } standard;
    struct {
        debug_alloc_api_t raw;
        debug_alloc_api_t mem;
        debug_alloc_api_t obj;
    } debug;
    int is_debug_enabled;
    PyObjectArenaAllocator obj_arena;
};

enum _py_float_format_type {
    _py_float_format_unknown,
    _py_float_format_ieee_big_endian,
    _py_float_format_ieee_little_endian,
};

struct _Py_float_runtime_state {
    enum _py_float_format_type float_format;
    enum _py_float_format_type double_format;
};

struct pyhash_runtime_state {
    struct {
#ifndef MS_WINDOWS
        int fd;
        dev_t st_dev;
        ino_t st_ino;
#else
    // This is a placeholder so the struct isn't empty on Windows.
    int _not_used;
#endif
    } urandom_cache;
};

#include "pycore_tracemalloc.h"   // struct _tracemalloc_runtime_state

struct _fileutils_state {
    int force_ascii;
};

#include "pycore_interpframe_structs.h" // _PyInterpreterFrame
#include "pycore_debug_offsets.h" // _Py_DebugOffsets
#include "pycore_signal.h"        // struct _signals_runtime_state
#include "pycore_faulthandler.h"  // struct _faulthandler_runtime_state
#include "pycore_ast.h"           // struct _expr

#ifdef Py_DEBUG
#define _PYPEGEN_NSTATISTICS 2000
#endif

struct _parser_runtime_state {
#ifdef Py_DEBUG
    long memo_statistics[_PYPEGEN_NSTATISTICS];
#ifdef Py_GIL_DISABLED
    PyMutex mutex;
#endif
#else
    int _not_used;
#endif
    struct _expr dummy_name;
};

typedef struct {
    PyTime_t numer;
    PyTime_t denom;
} _PyTimeFraction;

struct _Py_time_runtime_state {
#if defined(MS_WINDOWS) || defined(__APPLE__)
    _PyTimeFraction base;
#else
    char _unused;
#endif
};


struct _Py_cached_objects {
    // XXX We could statically allocate the hashtable.
    _Py_hashtable_t *interned_strings;
};

// These would be in pycore_long.h if it weren't for an include cycle.
#define _PY_NSMALLPOSINTS           1025
#define _PY_NSMALLNEGINTS           5

#include "pycore_global_strings.h" // struct _Py_global_strings

struct _Py_static_objects {
    struct {
        /* Small integers are preallocated in this array so that they
         * can be shared.
         * The integers that are preallocated are those in the range
         * -_PY_NSMALLNEGINTS (inclusive) to _PY_NSMALLPOSINTS (exclusive).
         */
        PyLongObject small_ints[_PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS];

        PyBytesObject bytes_empty;
        struct {
            PyBytesObject ob;
            char eos;
        } bytes_characters[256];

        struct _Py_global_strings strings;

        _PyGC_Head_UNUSED _tuple_empty_gc_not_used;
        PyTupleObject tuple_empty;

        _PyGC_Head_UNUSED _hamt_bitmap_node_empty_gc_not_used;
        PyHamtNode_Bitmap hamt_bitmap_node_empty;
        _PyContextTokenMissing context_token_missing;
    } singletons;
};

/* Full Python runtime state */

/* _PyRuntimeState holds the global state for the CPython runtime.
   That data is exported by the internal API as a global variable
   (_PyRuntime, defined near the top of pylifecycle.c).
   */
struct pyruntimestate {
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
        struct _Py_AuditHookEntry *head;
    } audit_hooks;

    struct _py_object_runtime_state object_state;
    struct _Py_float_runtime_state float_state;
    struct _Py_unicode_runtime_state unicode_state;
    struct _types_runtime_state types;
    struct _Py_time_runtime_state time;

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
};


#ifdef __cplusplus
}
#endif
#endif /* Py_INTERNAL_RUNTIME_STRUCTS_H */
