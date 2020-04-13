#ifndef Py_INTERNAL_PYSTATE_H
#define Py_INTERNAL_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_gil.h"       /* struct _gil_runtime_state  */
#include "pycore_pymem.h"     /* struct _gc_runtime_state */
#include "pycore_warnings.h"  /* struct _warnings_runtime_state */
#include "pycore_runtime.h"   /* PyRuntimestate */


/* ceval state */

struct _pending_calls {
    PyThread_type_lock lock;
    /* Request for running pending calls. */
    _Py_atomic_int calls_to_do;
    /* Request for looking at the `async_exc` field of the current
       thread state.
       Guarded by the GIL. */
    int async_exc;
#define NPENDINGCALLS 32
    struct {
        int (*func)(void *);
        void *arg;
    } calls[NPENDINGCALLS];
    int first;
    int last;
};

struct _ceval_state {
    /* Records whether tracing is on for any thread.  Counts the number
       of threads for which tstate->c_tracefunc is non-NULL, so if the
       value is 0, we know we don't have to check this thread's
       c_tracefunc.  This speeds up the if statement in
       _PyEval_EvalFrameDefault() after fast_next_opcode. */
    int tracing_possible;
    /* This single variable consolidates all requests to break out of
       the fast path in the eval loop. */
    _Py_atomic_int eval_breaker;
    struct _pending_calls pending;
};

/* interpreter state */

#define _PY_NSMALLPOSINTS           257
#define _PY_NSMALLNEGINTS           5

// The PyInterpreterState typedef is in Include/pystate.h.
struct _is {

    struct _is *next;
    struct _ts *tstate_head;

    /* Reference to the _PyRuntime global variable. This field exists
       to not have to pass runtime in addition to tstate to a function.
       Get runtime from tstate: tstate->interp->runtime. */
    struct pyruntimestate *runtime;

    int64_t id;
    int64_t id_refcount;
    int requires_idref;
    PyThread_type_lock id_mutex;

    int finalizing;

    struct _ceval_state ceval;
    struct _gc_runtime_state gc;

    PyObject *modules;
    PyObject *modules_by_index;
    PyObject *sysdict;
    PyObject *builtins;
    PyObject *importlib;

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
        int utf8;         /* encoding=="utf-8"? */
        char *errors;     /* Filesystem errors (encoded to UTF-8) */
        _Py_error_handler error_handler;
    } fs_codec;

    PyConfig config;
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

    struct _warnings_runtime_state warnings;

    PyObject *audit_hooks;

    struct {
        struct {
            int level;
            int atbol;
        } listnode;
    } parser;

#if _PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS > 0
    /* Small integers are preallocated in this array so that they
       can be shared.
       The integers that are preallocated are those in the range
       -_PY_NSMALLNEGINTS (inclusive) to _PY_NSMALLPOSINTS (not inclusive).
    */
    PyLongObject* small_ints[_PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS];
#endif
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


/* Check if the current thread is the main thread.
   Use _Py_IsMainInterpreter() to check if it's the main interpreter. */
static inline int
_Py_IsMainThread(void)
{
    unsigned long thread = PyThread_get_thread_ident();
    return (thread == _PyRuntime.main_thread);
}


static inline int
_Py_IsMainInterpreter(PyThreadState* tstate)
{
    /* Use directly _PyRuntime rather than tstate->interp->runtime, since
       this function is used in performance critical code path (ceval) */
    return (tstate->interp == _PyRuntime.interpreters.main);
}


/* Only handle signals on the main thread of the main interpreter. */
static inline int
_Py_ThreadCanHandleSignals(PyInterpreterState *interp)
{
    return (_Py_IsMainThread() && interp == _PyRuntime.interpreters.main);
}


/* Only execute pending calls on the main thread. */
static inline int
_Py_ThreadCanHandlePendingCalls(void)
{
    return _Py_IsMainThread();
}


/* Variable and macro for in-line access to current thread
   and interpreter state */

static inline PyThreadState* _PyRuntimeState_GetThreadState(_PyRuntimeState *runtime) {
    return (PyThreadState*)_Py_atomic_load_relaxed(&runtime->gilstate.tstate_current);
}

/* Get the current Python thread state.

   Efficient macro reading directly the 'gilstate.tstate_current' atomic
   variable. The macro is unsafe: it does not check for error and it can
   return NULL.

   The caller must hold the GIL.

   See also PyThreadState_Get() and PyThreadState_GET(). */
static inline PyThreadState *_PyThreadState_GET(void) {
    return _PyRuntimeState_GetThreadState(&_PyRuntime);
}

/* Redefine PyThreadState_GET() as an alias to _PyThreadState_GET() */
#undef PyThreadState_GET
#define PyThreadState_GET() _PyThreadState_GET()

/* Get the current interpreter state.

   The macro is unsafe: it does not check for error and it can return NULL.

   The caller must hold the GIL.

   See also _PyInterpreterState_Get()
   and _PyGILState_GetInterpreterStateUnsafe(). */
static inline PyInterpreterState* _PyInterpreterState_GET_UNSAFE(void) {
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->interp;
}


/* Other */

PyAPI_FUNC(void) _PyThreadState_Init(
    PyThreadState *tstate);
PyAPI_FUNC(void) _PyThreadState_DeleteExcept(
    _PyRuntimeState *runtime,
    PyThreadState *tstate);

PyAPI_FUNC(PyThreadState *) _PyThreadState_Swap(
    struct _gilstate_runtime_state *gilstate,
    PyThreadState *newts);

PyAPI_FUNC(PyStatus) _PyInterpreterState_Enable(_PyRuntimeState *runtime);
PyAPI_FUNC(void) _PyInterpreterState_DeleteExceptMain(_PyRuntimeState *runtime);

/* Used by _PyImport_Cleanup() */
extern void _PyInterpreterState_ClearModules(PyInterpreterState *interp);

extern PyStatus _PyInterpreterState_SetConfig(
    PyInterpreterState *interp,
    const PyConfig *config);

PyAPI_FUNC(void) _PyGILState_Reinit(_PyRuntimeState *runtime);


PyAPI_FUNC(int) _PyState_AddModule(
    PyThreadState *tstate,
    PyObject* module,
    struct PyModuleDef* def);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
