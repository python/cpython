#ifndef Py_INTERNAL_PYSTATE_H
#define Py_INTERNAL_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "cpython/coreconfig.h"
#include "pystate.h"
#include "pythread.h"

#include "pycore_ceval.h"
#include "pycore_pathconfig.h"
#include "pycore_pymem.h"
#include "pycore_warnings.h"


/* interpreter state */

typedef PyObject* (*_PyFrameEvalFunction)(struct _frame *, int);

// The PyInterpreterState typedef is in Include/pystate.h.
struct _is {

    struct _is *next;
    struct _ts *tstate_head;

    int64_t id;
    int64_t id_refcount;
    int requires_idref;
    PyThread_type_lock id_mutex;

    int finalizing;

    PyObject *modules;
    PyObject *modules_by_index;
    PyObject *sysdict;
    PyObject *builtins;
    PyObject *importlib;

    /* Used in Python/sysmodule.c. */
    int check_interval;

    /* Used in Modules/_threadmodule.c. */
    long num_threads;
    /* Support for runtime thread stack size tuning.
       A value of 0 means using the platform's default stack size
       or the size specified by the THREAD_STACK_SIZE macro. */
    /* Used in Python/thread.c. */
    size_t pythread_stacksize;

    PyObject *codec_search_path;
    PyObject *codec_search_cache;
    PyObject *codec_error_registry;
    int codecs_initialized;

    /* fs_codec.encoding is initialized to NULL.
       Later, it is set to a non-NULL string by _PyUnicode_InitEncodings(). */
    struct {
        char *encoding;   /* Filesystem encoding (encoded to UTF-8) */
        char *errors;     /* Filesystem errors (encoded to UTF-8) */
        _Py_error_handler error_handler;
    } fs_codec;

    _PyCoreConfig core_config;
#ifdef HAVE_DLOPEN
    int dlopenflags;
#endif

    PyObject *dict;  /* Stores per-interpreter state */

    PyObject *builtins_copy;
    PyObject *import_func;
    /* Initialized to PyEval_EvalFrameDefault(). */
    _PyFrameEvalFunction eval_frame;

    Py_ssize_t co_extra_user_count;
    freefunc co_extra_freefuncs[MAX_CO_EXTRA_USERS];

#ifdef HAVE_FORK
    PyObject *before_forkers;
    PyObject *after_forkers_parent;
    PyObject *after_forkers_child;
#endif
    /* AtExit module */
    void (*pyexitfunc)(PyObject *);
    PyObject *pyexitmodule;

    uint64_t tstate_next_unique_id;
};

PyAPI_FUNC(struct _is*) _PyInterpreterState_LookUpID(PY_INT64_T);

PyAPI_FUNC(int) _PyInterpreterState_IDInitref(struct _is *);
PyAPI_FUNC(void) _PyInterpreterState_IDIncref(struct _is *);
PyAPI_FUNC(void) _PyInterpreterState_IDDecref(struct _is *);


/* cross-interpreter data registry */

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   crossinterpdatafunc. It would be simpler and more efficient. */

struct _xidregitem;

struct _xidregitem {
    PyTypeObject *cls;
    crossinterpdatafunc getdata;
    struct _xidregitem *next;
};


/* GIL state */

struct _gilstate_runtime_state {
    int check_enabled;
    /* Assuming the current thread holds the GIL, this is the
       PyThreadState for the current thread. */
    _Py_atomic_address tstate_current;
    PyThreadFrameGetter getframe;
    /* The single PyInterpreterState used by this process'
       GILState implementation
    */
    /* TODO: Given interp_main, it may be possible to kill this ref */
    PyInterpreterState *autoInterpreterState;
    Py_tss_t autoTSSkey;
};

/* hook for PyEval_GetFrame(), requested for Psyco */
#define _PyThreadState_GetFrame _PyRuntime.gilstate.getframe

/* Issue #26558: Flag to disable PyGILState_Check().
   If set to non-zero, PyGILState_Check() always return 1. */
#define _PyGILState_check_enabled _PyRuntime.gilstate.check_enabled


/* Full Python runtime state */

typedef struct pyruntimestate {
    /* Is Python pre-initialized? Set to 1 by _Py_PreInitialize() */
    int pre_initialized;

    /* Is Python core initialized? Set to 1 by _Py_InitializeCore() */
    int core_initialized;

    /* Is Python fully initialized? Set to 1 by Py_Initialize() */
    int initialized;

    PyThreadState *finalizing;

    struct pyinterpreters {
        PyThread_type_lock mutex;
        PyInterpreterState *head;
        PyInterpreterState *main;
        /* _next_interp_id is an auto-numbered sequence of small
           integers.  It gets initialized in _PyInterpreterState_Init(),
           which is called in Py_Initialize(), and used in
           PyInterpreterState_New().  A negative interpreter ID
           indicates an error occurred.  The main interpreter will
           always have an ID of 0.  Overflow results in a RuntimeError.
           If that becomes a problem later then we can adjust, e.g. by
           using a Python int. */
        int64_t next_id;
    } interpreters;
    // XXX Remove this field once we have a tp_* slot.
    struct _xidregistry {
        PyThread_type_lock mutex;
        struct _xidregitem *head;
    } xidregistry;

    unsigned long main_thread;

#define NEXITFUNCS 32
    void (*exitfuncs[NEXITFUNCS])(void);
    int nexitfuncs;

    struct _gc_runtime_state gc;
    struct _warnings_runtime_state warnings;
    struct _ceval_runtime_state ceval;
    struct _gilstate_runtime_state gilstate;

    _PyPreConfig preconfig;
    // XXX Consolidate globals found via the check-c-globals script.
} _PyRuntimeState;

#define _PyRuntimeState_INIT \
    {.pre_initialized = 0, .core_initialized = 0, .initialized = 0}
/* Note: _PyRuntimeState_INIT sets other fields to 0/NULL */

PyAPI_DATA(_PyRuntimeState) _PyRuntime;
PyAPI_FUNC(_PyInitError) _PyRuntimeState_Init(_PyRuntimeState *runtime);
PyAPI_FUNC(void) _PyRuntimeState_Fini(_PyRuntimeState *runtime);
PyAPI_FUNC(void) _PyRuntimeState_ReInitThreads(_PyRuntimeState *runtime);

/* Initialize _PyRuntimeState.
   Return NULL on success, or return an error message on failure. */
PyAPI_FUNC(_PyInitError) _PyRuntime_Initialize(void);

PyAPI_FUNC(void) _PyRuntime_Finalize(void);

#define _Py_CURRENTLY_FINALIZING(tstate) \
    (_PyRuntime.finalizing == tstate)


/* Variable and macro for in-line access to current thread
   and interpreter state */

/* Get the current Python thread state.

   Efficient macro reading directly the 'gilstate.tstate_current' atomic
   variable. The macro is unsafe: it does not check for error and it can
   return NULL.

   The caller must hold the GIL.

   See also PyThreadState_Get() and PyThreadState_GET(). */
#define _PyThreadState_GET() \
    ((PyThreadState*)_Py_atomic_load_relaxed(&_PyRuntime.gilstate.tstate_current))

/* Redefine PyThreadState_GET() as an alias to _PyThreadState_GET() */
#undef PyThreadState_GET
#define PyThreadState_GET() _PyThreadState_GET()

/* Get the current interpreter state.

   The macro is unsafe: it does not check for error and it can return NULL.

   The caller must hold the GIL.

   See also _PyInterpreterState_Get()
   and _PyGILState_GetInterpreterStateUnsafe(). */
#define _PyInterpreterState_GET_UNSAFE() (_PyThreadState_GET()->interp)


/* Other */

PyAPI_FUNC(void) _PyThreadState_Init(
    _PyRuntimeState *runtime,
    PyThreadState *tstate);
PyAPI_FUNC(void) _PyThreadState_DeleteExcept(PyThreadState *tstate);

PyAPI_FUNC(_PyInitError) _PyInterpreterState_Enable(_PyRuntimeState *runtime);
PyAPI_FUNC(void) _PyInterpreterState_DeleteExceptMain(_PyRuntimeState *runtime);

PyAPI_FUNC(void) _PyGILState_Reinit(_PyRuntimeState *runtime);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
