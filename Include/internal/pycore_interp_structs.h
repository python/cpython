/* This file contains the struct definitions for interpreter state
 * and other necessary structs */

#ifndef Py_INTERNAL_INTERP_STRUCTS_H
#define Py_INTERNAL_INTERP_STRUCTS_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pycore_ast_state.h"     // struct ast_state
#include "pycore_llist.h"         // struct llist_node
#include "pycore_opcode_utils.h"  // NUM_COMMON_CONSTANTS
#include "pycore_pymath.h"        // _PY_SHORT_FLOAT_REPR
#include "pycore_structs.h"       // PyHamtObject
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_typedefs.h"      // _PyRuntimeState

#define CODE_MAX_WATCHERS 8
#define CONTEXT_MAX_WATCHERS 8
#define FUNC_MAX_WATCHERS 8
#define TYPE_MAX_WATCHERS 8


#ifdef Py_GIL_DISABLED
// This should be prime but otherwise the choice is arbitrary. A larger value
// increases concurrency at the expense of memory.
#  define NUM_WEAKREF_LIST_LOCKS 127
#endif

typedef int (*_Py_pending_call_func)(void *);

struct _pending_call {
    _Py_pending_call_func func;
    void *arg;
    int flags;
};

#define PENDINGCALLSARRAYSIZE 300

struct _pending_calls {
    PyThreadState *handling_thread;
    PyMutex mutex;
    /* Request for running pending calls. */
    int32_t npending;
    /* The maximum allowed number of pending calls.
       If the queue fills up to this point then _PyEval_AddPendingCall()
       will return _Py_ADD_PENDING_FULL. */
    int32_t max;
    /* We don't want a flood of pending calls to interrupt any one thread
       for too long, so we keep a limit on the number handled per pass.
       A value of 0 means there is no limit (other than the maximum
       size of the list of pending calls). */
    int32_t maxloop;
    struct _pending_call calls[PENDINGCALLSARRAYSIZE];
    int first;
    int next;
};

typedef enum {
    PERF_STATUS_FAILED = -1,  // Perf trampoline is in an invalid state
    PERF_STATUS_NO_INIT = 0,  // Perf trampoline is not initialized
    PERF_STATUS_OK = 1,       // Perf trampoline is ready to be executed
} perf_status_t;

#ifdef PY_HAVE_PERF_TRAMPOLINE
struct code_arena_st;

struct trampoline_api_st {
    void* (*init_state)(void);
    void (*write_state)(void* state, const void *code_addr,
                        unsigned int code_size, PyCodeObject* code);
    int (*free_state)(void* state);
    void *state;
    Py_ssize_t code_padding;
    Py_ssize_t code_alignment;
};
#endif


struct _ceval_runtime_state {
    struct {
#ifdef PY_HAVE_PERF_TRAMPOLINE
        perf_status_t status;
        int perf_trampoline_type;
        Py_ssize_t extra_code_index;
        struct code_arena_st *code_arena;
        struct trampoline_api_st trampoline_api;
        FILE *map_file;
        Py_ssize_t persist_after_fork;
       _PyFrameEvalFunction prev_eval_frame;
#else
        int _not_used;
#endif
    } perf;
    /* Pending calls to be made only on the main thread. */
    // The signal machinery falls back on this
    // so it must be especially stable and efficient.
    // For example, we use a preallocated array
    // for the list of pending calls.
    struct _pending_calls pending_mainthread;
};


struct _ceval_state {
    /* This variable holds the global instrumentation version. When a thread is
       running, this value is overlaid onto PyThreadState.eval_breaker so that
       changes in the instrumentation version will trigger the eval breaker. */
    uintptr_t instrumentation_version;
    int recursion_limit;
    struct _gil_runtime_state *gil;
    int own_gil;
    struct _pending_calls pending;
};


//###############
// runtime atexit

typedef void (*atexit_callbackfunc)(void);

struct _atexit_runtime_state {
    PyMutex mutex;
#define NEXITFUNCS 32
    atexit_callbackfunc callbacks[NEXITFUNCS];
    int ncallbacks;
};


//###################
// interpreter atexit

typedef struct atexit_callback {
    atexit_datacallbackfunc func;
    void *data;
    struct atexit_callback *next;
} atexit_callback;

struct atexit_state {
#ifdef Py_GIL_DISABLED
    PyMutex ll_callbacks_lock;
#endif
    atexit_callback *ll_callbacks;

    // XXX The rest of the state could be moved to the atexit module state
    // and a low-level callback added for it during module exec.
    // For the moment we leave it here.

    // List containing tuples with callback information.
    // e.g. [(func, args, kwargs), ...]
    PyObject *callbacks;
};


/****** Garbage collector **********/

/* GC information is stored BEFORE the object structure. */
typedef struct {
    // Tagged pointer to next object in the list.
    // 0 means the object is not tracked
    _Py_ALIGNED_DEF(_PyObject_MIN_ALIGNMENT, uintptr_t) _gc_next;

    // Tagged pointer to previous object in the list.
    // Lowest two bits are used for flags documented later.
    // Those bits are made available by the struct's minimum alignment.
    uintptr_t _gc_prev;
} PyGC_Head;

#define _PyGC_Head_UNUSED PyGC_Head

struct gc_generation {
    PyGC_Head head;
    int threshold; /* collection threshold */
    int count; /* count of allocations or collections of younger
                  generations */
};

struct gc_collection_stats {
    /* number of collected objects */
    Py_ssize_t collected;
    /* total number of uncollectable objects (put into gc.garbage) */
    Py_ssize_t uncollectable;
};

/* Running stats per generation */
struct gc_generation_stats {
    /* total number of collections */
    Py_ssize_t collections;
    /* total number of collected objects */
    Py_ssize_t collected;
    /* total number of uncollectable objects (put into gc.garbage) */
    Py_ssize_t uncollectable;
};

enum _GCPhase {
    GC_PHASE_MARK = 0,
    GC_PHASE_COLLECT = 1
};

/* If we change this, we need to change the default value in the
   signature of gc.collect. */
#define NUM_GENERATIONS 3

struct _gc_runtime_state {
    /* List of objects that still need to be cleaned up, singly linked
     * via their gc headers' gc_prev pointers.  */
    PyObject *trash_delete_later;
    /* Current call-stack depth of tp_dealloc calls. */
    int trash_delete_nesting;

    /* Is automatic collection enabled? */
    int enabled;
    int debug;
    /* linked lists of container objects */
    struct gc_generation young;
    struct gc_generation old[2];
    /* a permanent generation which won't be collected */
    struct gc_generation permanent_generation;
    struct gc_generation_stats generation_stats[NUM_GENERATIONS];
    /* true if we are currently running the collector */
    int collecting;
    /* list of uncollectable objects */
    PyObject *garbage;
    /* a list of callbacks to be invoked when collection is performed */
    PyObject *callbacks;

    Py_ssize_t heap_size;
    Py_ssize_t work_to_do;
    /* Which of the old spaces is the visited space */
    int visited_space;
    int phase;

#ifdef Py_GIL_DISABLED
    /* This is the number of objects that survived the last full
       collection. It approximates the number of long lived objects
       tracked by the GC.

       (by "full collection", we mean a collection of the oldest
       generation). */
    Py_ssize_t long_lived_total;
    /* This is the number of objects that survived all "non-full"
       collections, and are awaiting to undergo a full collection for
       the first time. */
    Py_ssize_t long_lived_pending;

    /* True if gc.freeze() has been used. */
    int freeze_active;

    /* Memory usage of the process (RSS + swap) after last GC. */
    Py_ssize_t last_mem;

    /* This accumulates the new object count whenever collection is deferred
       due to the RSS increase condition not being meet.  Reset on collection. */
    Py_ssize_t deferred_count;

    /* Mutex held for gc_should_collect_mem_usage(). */
    PyMutex mutex;
#endif
};

#include "pycore_gil.h"           // struct _gil_runtime_state

/**** Import ********/

struct _import_runtime_state {
    /* The builtin modules (defined in config.c). */
    struct _inittab *inittab;
    /* The most recent value assigned to a PyModuleDef.m_base.m_index.
       This is incremented each time PyModuleDef_Init() is called,
       which is just about every time an extension module is imported.
       See PyInterpreterState.modules_by_index for more info. */
    Py_ssize_t last_module_index;
    struct {
        /* A lock to guard the cache. */
        PyMutex mutex;
        /* The actual cache of (filename, name, PyModuleDef) for modules.
           Only legacy (single-phase init) extension modules are added
           and only if they support multiple initialization (m_size >= 0)
           or are imported in the main interpreter.
           This is initialized lazily in fix_up_extension() in import.c.
           Modules are added there and looked up in _imp.find_extension(). */
        struct _Py_hashtable_t *hashtable;
    } extensions;
    /* Package context -- the full module name for package imports */
    const char * pkgcontext;
};

struct _import_state {
    /* cached sys.modules dictionary */
    PyObject *modules;
    /* This is the list of module objects for all legacy (single-phase init)
       extension modules ever loaded in this process (i.e. imported
       in this interpreter or in any other).  Py_None stands in for
       modules that haven't actually been imported in this interpreter.

       A module's index (PyModuleDef.m_base.m_index) is used to look up
       the corresponding module object for this interpreter, if any.
       (See PyState_FindModule().)  When any extension module
       is initialized during import, its moduledef gets initialized by
       PyModuleDef_Init(), and the first time that happens for each
       PyModuleDef, its index gets set to the current value of
       a global counter (see _PyRuntimeState.imports.last_module_index).
       The entry for that index in this interpreter remains unset until
       the module is actually imported here.  (Py_None is used as
       a placeholder.)  Note that multi-phase init modules always get
       an index for which there will never be a module set.

       This is initialized lazily in PyState_AddModule(), which is also
       where modules get added. */
    PyObject *modules_by_index;
    /* importlib module._bootstrap */
    PyObject *importlib;
    /* override for config->use_frozen_modules (for tests)
       (-1: "off", 1: "on", 0: no override) */
    int override_frozen_modules;
    int override_multi_interp_extensions_check;
#ifdef HAVE_DLOPEN
    int dlopenflags;
#endif
    PyObject *import_func;
    /* The global import lock. */
    _PyRecursiveMutex lock;
    /* diagnostic info in PyImport_ImportModuleLevelObject() */
    struct {
        int import_level;
        PyTime_t accumulated;
        int header;
    } find_and_load;
};



/********** Interpreter state **************/

#include "pycore_object_state.h"  // struct _py_object_state
#include "pycore_crossinterp.h"   // _PyXI_state_t


struct _Py_long_state {
    int max_str_digits;
};

struct codecs_state {
    // A list of callable objects used to search for codecs.
    PyObject *search_path;

    // A dict mapping codec names to codecs returned from a callable in
    // search_path.
    PyObject *search_cache;

    // A dict mapping error handling strategies to functions to implement them.
    PyObject *error_registry;

#ifdef Py_GIL_DISABLED
    // Used to safely delete a specific item from search_path.
    PyMutex search_path_mutex;
#endif

    // Whether or not the rest of the state is initialized.
    int initialized;
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

struct
Bigint {
    struct Bigint *next;
    int k, maxwds, sign, wds;
    uint32_t x[1];
};

#if defined(Py_USING_MEMORY_DEBUGGER) || _PY_SHORT_FLOAT_REPR == 0

struct _dtoa_state {
    int _not_used;
};

#else  // !Py_USING_MEMORY_DEBUGGER && _PY_SHORT_FLOAT_REPR != 0

/* The size of the Bigint freelist */
#define Bigint_Kmax 7

/* The size of the cached powers of 5 array */
#define Bigint_Pow5size 8

#ifndef PRIVATE_MEM
#define PRIVATE_MEM 2304
#endif
#define Bigint_PREALLOC_SIZE \
    ((PRIVATE_MEM+sizeof(double)-1)/sizeof(double))

struct _dtoa_state {
    // p5s is an array of powers of 5 of the form:
    // 5**(2**(i+2)) for 0 <= i < Bigint_Pow5size
    struct Bigint *p5s[Bigint_Pow5size];
    // XXX This should be freed during runtime fini.
    struct Bigint *freelist[Bigint_Kmax+1];
    double preallocated[Bigint_PREALLOC_SIZE];
    double *preallocated_next;
};

#endif  // !Py_USING_MEMORY_DEBUGGER

struct _py_code_state {
    PyMutex mutex;
    // Interned constants from code objects. Used by the free-threaded build.
    struct _Py_hashtable_t *constants;
};

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

#include "pycore_dict_state.h"    // struct _Py_dict_state
#include "pycore_exceptions.h"    // struct _Py_exc_state


/****** type state *********/

/* For now we hard-code this to a value for which we are confident
   all the static builtin types will fit (for all builds). */
#define _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES 200
#define _Py_MAX_MANAGED_STATIC_EXT_TYPES 10
#define _Py_MAX_MANAGED_STATIC_TYPES \
    (_Py_MAX_MANAGED_STATIC_BUILTIN_TYPES + _Py_MAX_MANAGED_STATIC_EXT_TYPES)

struct _types_runtime_state {
    /* Used to set PyTypeObject.tp_version_tag for core static types. */
    // bpo-42745: next_version_tag remains shared by all interpreters
    // because of static types.
    unsigned int next_version_tag;

    struct {
        struct {
            PyTypeObject *type;
            int64_t interp_count;
        } types[_Py_MAX_MANAGED_STATIC_TYPES];
    } managed_static;
};


// Type attribute lookup cache: speed up attribute and method lookups,
// see _PyType_Lookup().
struct type_cache_entry {
    unsigned int version;  // initialized from type->tp_version_tag
#ifdef Py_GIL_DISABLED
   _PySeqLock sequence;
#endif
    PyObject *name;        // reference to exactly a str or None
    PyObject *value;       // borrowed reference or NULL
};

#define MCACHE_SIZE_EXP 12

struct type_cache {
    struct type_cache_entry hashtable[1 << MCACHE_SIZE_EXP];
};

typedef struct {
    PyTypeObject *type;
    int isbuiltin;
    int readying;
    int ready;
    // XXX tp_dict can probably be statically allocated,
    // instead of dynamically and stored on the interpreter.
    PyObject *tp_dict;
    PyObject *tp_subclasses;
    /* We never clean up weakrefs for static builtin types since
       they will effectively never get triggered.  However, there
       are also some diagnostic uses for the list of weakrefs,
       so we still keep it. */
    PyObject *tp_weaklist;
} managed_static_type_state;

#define TYPE_VERSION_CACHE_SIZE (1<<12)  /* Must be a power of 2 */

struct types_state {
    /* Used to set PyTypeObject.tp_version_tag.
       It starts at _Py_MAX_GLOBAL_TYPE_VERSION_TAG + 1,
       where all those lower numbers are used for core static types. */
    unsigned int next_version_tag;

    struct type_cache type_cache;

    /* Every static builtin type is initialized for each interpreter
       during its own initialization, including for the main interpreter
       during global runtime initialization.  This is done by calling
       _PyStaticType_InitBuiltin().

       The first time a static builtin type is initialized, all the
       normal PyType_Ready() stuff happens.  The only difference from
       normal is that there are three PyTypeObject fields holding
       objects which are stored here (on PyInterpreterState) rather
       than in the corresponding PyTypeObject fields.  Those are:
       tp_dict (cls.__dict__), tp_subclasses (cls.__subclasses__),
       and tp_weaklist.

       When a subinterpreter is initialized, each static builtin type
       is still initialized, but only the interpreter-specific portion,
       namely those three objects.

       Those objects are stored in the PyInterpreterState.types.builtins
       array, at the index corresponding to each specific static builtin
       type.  That index (a size_t value) is stored in the tp_subclasses
       field.  For static builtin types, we re-purposed the now-unused
       tp_subclasses to avoid adding another field to PyTypeObject.
       In all other cases tp_subclasses holds a dict like before.
       (The field was previously defined as PyObject*, but is now void*
       to reflect its dual use.)

       The index for each static builtin type isn't statically assigned.
       Instead it is calculated the first time a type is initialized
       (by the main interpreter).  The index matches the order in which
       the type was initialized relative to the others.  The actual
       value comes from the current value of num_builtins_initialized,
       as each type is initialized for the main interpreter.

       num_builtins_initialized is incremented once for each static
       builtin type.  Once initialization is over for a subinterpreter,
       the value will be the same as for all other interpreters.  */
    struct {
        size_t num_initialized;
        managed_static_type_state initialized[_Py_MAX_MANAGED_STATIC_BUILTIN_TYPES];
    } builtins;
    /* We apply a similar strategy for managed extension modules. */
    struct {
        size_t num_initialized;
        size_t next_index;
        managed_static_type_state initialized[_Py_MAX_MANAGED_STATIC_EXT_TYPES];
    } for_extensions;
    PyMutex mutex;

    // Borrowed references to type objects whose
    // tp_version_tag % TYPE_VERSION_CACHE_SIZE
    // once was equal to the index in the table.
    // They are cleared when the type object is deallocated.
    PyTypeObject *type_version_cache[TYPE_VERSION_CACHE_SIZE];
};

struct _warnings_runtime_state {
    /* Both 'filters' and 'onceregistry' can be set in warnings.py;
       get_warnings_attr() will reset these variables accordingly. */
    PyObject *filters;  /* List */
    PyObject *once_registry;  /* Dict */
    PyObject *default_action; /* String */
    _PyRecursiveMutex lock;
    long filters_version;
    PyObject *context;
};

struct _Py_mem_interp_free_queue {
    int has_work;   // true if the queue is not empty
    PyMutex mutex;  // protects the queue
    struct llist_node head;  // queue of _mem_work_chunk items
};


/****** Unicode state *********/

typedef enum {
    _Py_ERROR_UNKNOWN=0,
    _Py_ERROR_STRICT,
    _Py_ERROR_SURROGATEESCAPE,
    _Py_ERROR_REPLACE,
    _Py_ERROR_IGNORE,
    _Py_ERROR_BACKSLASHREPLACE,
    _Py_ERROR_SURROGATEPASS,
    _Py_ERROR_XMLCHARREFREPLACE,
    _Py_ERROR_OTHER
} _Py_error_handler;

struct _Py_unicode_runtime_ids {
    PyMutex mutex;
    // next_index value must be preserved when Py_Initialize()/Py_Finalize()
    // is called multiple times: see _PyUnicode_FromId() implementation.
    Py_ssize_t next_index;
};

struct _Py_unicode_runtime_state {
    struct _Py_unicode_runtime_ids ids;
};

/* fs_codec.encoding is initialized to NULL.
   Later, it is set to a non-NULL string by _PyUnicode_InitEncodings(). */
struct _Py_unicode_fs_codec {
    char *encoding;   // Filesystem encoding (encoded to UTF-8)
    int utf8;         // encoding=="utf-8"?
    char *errors;     // Filesystem errors (encoded to UTF-8)
    _Py_error_handler error_handler;
};

struct _Py_unicode_ids {
    Py_ssize_t size;
    PyObject **array;
};

#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI

struct _Py_unicode_state {
    struct _Py_unicode_fs_codec fs_codec;

    _PyUnicode_Name_CAPI *ucnhash_capi;

    // Unicode identifiers (_Py_Identifier): see _PyUnicode_FromId()
    struct _Py_unicode_ids ids;
};

// Borrowed references to common callables:
struct callable_cache {
    PyObject *isinstance;
    PyObject *len;
    PyObject *list_append;
    PyObject *object__getattribute__;
};

/* Length of array of slotdef pointers used to store slots with the
   same __name__.  There should be at most MAX_EQUIV-1 slotdef entries with
   the same __name__, for any __name__. Since that's a static property, it is
   appropriate to declare fixed-size arrays for this. */
#define MAX_EQUIV 10

typedef struct wrapperbase pytype_slotdef;


struct _Py_interp_cached_objects {
#ifdef Py_GIL_DISABLED
    PyMutex interned_mutex;
#endif
    PyObject *interned_strings;

    /* object.__reduce__ */
    PyObject *objreduce;
#ifndef Py_GIL_DISABLED
    /* resolve_slotdups() */
    PyObject *type_slots_pname;
    pytype_slotdef *type_slots_ptrs[MAX_EQUIV];
#endif

    /* TypeVar and related types */
    PyTypeObject *generic_type;
    PyTypeObject *typevar_type;
    PyTypeObject *typevartuple_type;
    PyTypeObject *paramspec_type;
    PyTypeObject *paramspecargs_type;
    PyTypeObject *paramspeckwargs_type;
    PyTypeObject *constevaluator_type;

    /* Descriptors for __dict__ and __weakref__ */
#ifdef Py_GIL_DISABLED
    PyMutex descriptor_mutex;
#endif
    PyObject *dict_descriptor;
    PyObject *weakref_descriptor;
};

struct _Py_interp_static_objects {
    struct {
        int _not_used;
        // hamt_empty is here instead of global because of its weakreflist.
        _PyGC_Head_UNUSED _hamt_empty_gc_not_used;
        PyHamtObject hamt_empty;
        PyBaseExceptionObject last_resort_memory_error;
    } singletons;
};

#include "pycore_instruments.h"   // PY_MONITORING_TOOL_IDS


#ifdef Py_GIL_DISABLED

// A min-heap of indices
typedef struct _PyIndexHeap {
    int32_t *values;

    // Number of items stored in values
    Py_ssize_t size;

    // Maximum number of items that can be stored in values
    Py_ssize_t capacity;
} _PyIndexHeap;

// An unbounded pool of indices. Indices are allocated starting from 0. They
// may be released back to the pool once they are no longer in use.
typedef struct _PyIndexPool {
    PyMutex mutex;

    // Min heap of indices available for allocation
    _PyIndexHeap free_indices;

    // Next index to allocate if no free indices are available
    int32_t next_index;

    // Generation counter incremented on thread creation/destruction
    // Used for TLBC cache invalidation in remote debugging
    uint32_t tlbc_generation;
} _PyIndexPool;

typedef union _Py_unique_id_entry {
    // Points to the next free type id, when part of the freelist
    union _Py_unique_id_entry *next;

    // Stores the object when the id is assigned
    PyObject *obj;
} _Py_unique_id_entry;

struct _Py_unique_id_pool {
    PyMutex mutex;

    // combined table of object with allocated unique ids and unallocated ids.
    _Py_unique_id_entry *table;

    // Next entry to allocate inside 'table' or NULL
    _Py_unique_id_entry *freelist;

    // size of 'table'
    Py_ssize_t size;
};

#endif

typedef _Py_CODEUNIT *(*_PyJitEntryFuncPtr)(struct _PyExecutorObject *exec, _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate);

/* PyInterpreterState holds the global state for one of the runtime's
   interpreters.  Typically the initial (main) interpreter is the only one.

   The PyInterpreterState typedef is in Include/pytypedefs.h.
   */
struct _is {

    /* This struct contains the eval_breaker,
     * which is by far the hottest field in this struct
     * and should be placed at the beginning. */
    struct _ceval_state ceval;

    /* This structure is carefully allocated so that it's correctly aligned
     * to avoid undefined behaviors during LOAD and STORE. The '_malloced'
     * field stores the allocated pointer address that will later be freed.
     */
    void *_malloced;

    PyInterpreterState *next;

    int64_t id;
    Py_ssize_t id_refcount;
    int requires_idref;

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
        _PyThreadStateImpl *preallocated;
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
    _PyRuntimeState *runtime;

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

    uint64_t _code_object_generation;

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
    _PyXI_state_t xi;

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
    struct _Py_unique_id_pool unique_ids;  // object ids for per-thread refcounts
    PyMutex weakref_locks[NUM_WEAKREF_LIST_LOCKS];
    _PyIndexPool tlbc_indices;
#endif
    // Per-interpreter list of tasks, any lingering tasks from thread
    // states gets added here and removed from the corresponding
    // thread state's list.
    struct llist_node asyncio_tasks_head;
    // `asyncio_tasks_lock` is used when tasks are moved
    // from thread's list to interpreter's list.
    PyMutex asyncio_tasks_lock;

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
    PyContext_WatchCallback context_watchers[CONTEXT_MAX_WATCHERS];
    // One bit is set for each non-NULL entry in code_watchers
    uint8_t active_code_watchers;
    uint8_t active_context_watchers;

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
    PyObject *common_consts[NUM_COMMON_CONSTANTS];
    bool jit;
    struct _PyExecutorObject *executor_list_head;
    struct _PyExecutorObject *executor_deletion_list_head;
    struct _PyExecutorObject *cold_executor;
    int executor_deletion_list_remaining_capacity;
    size_t trace_run_counter;
    _rare_events rare_events;
    PyDict_WatchCallback builtins_dict_watcher;

    _Py_GlobalMonitors monitors;
    _PyOnceFlag sys_profile_once_flag;
    _PyOnceFlag sys_trace_once_flag;
    Py_ssize_t sys_profiling_threads; /* Count of threads with c_profilefunc set */
    Py_ssize_t sys_tracing_threads; /* Count of threads with c_tracefunc set */
    PyObject *monitoring_callables[PY_MONITORING_TOOL_IDS][_PY_MONITORING_EVENTS];
    PyObject *monitoring_tool_names[PY_MONITORING_TOOL_IDS];
    uintptr_t monitoring_tool_versions[PY_MONITORING_TOOL_IDS];

    struct _Py_interp_cached_objects cached_objects;
    struct _Py_interp_static_objects static_objects;

    Py_ssize_t _interactive_src_count;

#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
    uint64_t next_stackref;
    _Py_hashtable_t *open_stackrefs_table;
#  ifdef Py_STACKREF_CLOSE_DEBUG
    _Py_hashtable_t *closed_stackrefs_table;
#  endif
#endif

    /* the initial PyInterpreterState.threads.head */
    _PyThreadStateImpl _initial_thread;
    // _initial_thread should be the last field of PyInterpreterState.
    // See https://github.com/python/cpython/issues/127117.
};


#ifdef __cplusplus
}
#endif
#endif /* Py_INTERNAL_INTERP_STRUCTS_H */
