/* Thread and interpreter state structures and their interfaces */


#ifndef Py_PYSTATE_H
#define Py_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pythread.h"

/* This limitation is for performance and simplicity. If needed it can be
removed (with effort). */
#define MAX_CO_EXTRA_USERS 255

/* State shared between threads */

struct _ts; /* Forward */
struct _is; /* Forward */
struct _frame; /* Forward declaration for PyFrameObject. */

#ifdef Py_LIMITED_API
typedef struct _is PyInterpreterState;
#else
typedef PyObject* (*_PyFrameEvalFunction)(struct _frame *, int);


typedef struct {
    /* Install signal handlers? Yes by default. */
    int install_signal_handlers;

    /* If greater than 0: use environment variables.
       Set to 0 by -E command line option. If set to -1 (default), it is
       set to !Py_IgnoreEnvironmentFlag. */
    int use_environment;

    int use_hash_seed;      /* PYTHONHASHSEED=x */
    unsigned long hash_seed;

    const char *allocator;  /* Memory allocator: PYTHONMALLOC */
    int dev_mode;           /* PYTHONDEVMODE, -X dev */

    /* Enable faulthandler?
       Set to 1 by -X faulthandler and PYTHONFAULTHANDLER. -1 means unset. */
    int faulthandler;

    /* Enable tracemalloc?
       Set by -X tracemalloc=N and PYTHONTRACEMALLOC. -1 means unset */
    int tracemalloc;

    int import_time;        /* PYTHONPROFILEIMPORTTIME, -X importtime */
    int show_ref_count;     /* -X showrefcount */
    int show_alloc_count;   /* -X showalloccount */
    int dump_refs;          /* PYTHONDUMPREFS */
    int malloc_stats;       /* PYTHONMALLOCSTATS */
    int coerce_c_locale;    /* PYTHONCOERCECLOCALE, -1 means unknown */
    int coerce_c_locale_warn; /* PYTHONCOERCECLOCALE=warn */
    int utf8_mode;          /* PYTHONUTF8, -X utf8; -1 means unknown */
    wchar_t *pycache_prefix; /* PYTHONPYCACHEPREFIX, -X pycache_prefix=PATH */

    wchar_t *program_name;  /* Program name, see also Py_GetProgramName() */
    int argc;               /* Number of command line arguments,
                               -1 means unset */
    wchar_t **argv;         /* Command line arguments */
    wchar_t *program;       /* argv[0] or "" */

    int nxoption;           /* Number of -X options */
    wchar_t **xoptions;     /* -X options */

    int nwarnoption;        /* Number of warnings options */
    wchar_t **warnoptions;  /* Warnings options */

    /* Path configuration inputs */
    wchar_t *module_search_path_env; /* PYTHONPATH environment variable */
    wchar_t *home;          /* PYTHONHOME environment variable,
                               see also Py_SetPythonHome(). */

    /* Path configuration outputs */
    int nmodule_search_path;        /* Number of sys.path paths,
                                       -1 means unset */
    wchar_t **module_search_paths;  /* sys.path paths */
    wchar_t *executable;    /* sys.executable */
    wchar_t *prefix;        /* sys.prefix */
    wchar_t *base_prefix;   /* sys.base_prefix */
    wchar_t *exec_prefix;   /* sys.exec_prefix */
    wchar_t *base_exec_prefix;  /* sys.base_exec_prefix */
#ifdef MS_WINDOWS
    wchar_t *dll_path;      /* Windows DLL path */
#endif

    /* If greater than 0, enable isolated mode: sys.path contains
       neither the script's directory nor the user's site-packages directory.

       Set to 1 by the -I command line option. If set to -1 (default), inherit
       Py_IsolatedFlag value. */
    int isolated;

    /* If equal to zero, disable the import of the module site and the
       site-dependent manipulations of sys.path that it entails. Also disable
       these manipulations if site is explicitly imported later (call
       site.main() if you want them to be triggered).

       Set to 0 by the -S command line option. If set to -1 (default), it is
       set to !Py_NoSiteFlag. */
    int site_import;

    /* Bytes warnings:

       * If equal to 1, issue a warning when comparing bytes or bytearray with
         str or bytes with int.
       * If equal or greater to 2, issue an error.

       Incremented by the -b command line option. If set to -1 (default), inherit
       Py_BytesWarningFlag value. */
    int bytes_warning;

    /* If greater than 0, enable inspect: when a script is passed as first
       argument or the -c option is used, enter interactive mode after
       executing the script or the command, even when sys.stdin does not appear
       to be a terminal.

       Incremented by the -i command line option. Set to 1 if the PYTHONINSPECT
       environment variable is non-empty. If set to -1 (default), inherit
       Py_InspectFlag value. */
    int inspect;

    /* If greater than 0: enable the interactive mode (REPL).

       Incremented by the -i command line option. If set to -1 (default),
       inherit Py_InteractiveFlag value. */
    int interactive;

    /* Optimization level.

       Incremented by the -O command line option. Set by the PYTHONOPTIMIZE
       environment variable. If set to -1 (default), inherit Py_OptimizeFlag
       value. */
    int optimization_level;

    /* If greater than 0, enable the debug mode: turn on parser debugging
       output (for expert only, depending on compilation options).

       Incremented by the -d command line option. Set by the PYTHONDEBUG
       environment variable. If set to -1 (default), inherit Py_DebugFlag
       value. */
    int debug;

    /* If equal to 0, Python won't try to write ``.pyc`` files on the
       import of source modules.

       Set to 0 by the -B command line option and the PYTHONDONTWRITEBYTECODE
       environment variable. If set to -1 (default), it is set to
       !Py_DontWriteBytecodeFlag. */
    int write_bytecode;

    /* If greater than 0, enable the verbose mode: print a message each time a
       module is initialized, showing the place (filename or built-in module)
       from which it is loaded.

       If greater or equal to 2, print a message for each file that is checked
       for when searching for a module. Also provides information on module
       cleanup at exit.

       Incremented by the -v option. Set by the PYTHONVERBOSE environment
       variable. If set to -1 (default), inherit Py_VerboseFlag value. */
    int verbose;

    /* If greater than 0, enable the quiet mode: Don't display the copyright
       and version messages even in interactive mode.

       Incremented by the -q option. If set to -1 (default), inherit
       Py_QuietFlag value. */
    int quiet;

   /* If greater than 0, don't add the user site-packages directory to
      sys.path.

      Set to 0 by the -s and -I command line options , and the PYTHONNOUSERSITE
      environment variable. If set to -1 (default), it is set to
      !Py_NoUserSiteDirectory. */
    int user_site_directory;

    /* If greater than 0, enable unbuffered mode: force the stdout and stderr
       streams to be unbuffered.

       Set to 1 by the -u option. Set by the PYTHONUNBUFFERED environment
       variable. If set to -1 (default), inherit Py_UnbufferedStdioFlag
       value. */
    int unbuffered_stdio;

#ifdef MS_WINDOWS
    /* If greater than 1, use the "mbcs" encoding instead of the UTF-8
       encoding for the filesystem encoding.

       Set to 1 if the PYTHONLEGACYWINDOWSFSENCODING environment variable is
       set to a non-empty string. If set to -1 (default), inherit
       Py_LegacyWindowsFSEncodingFlag value.

       See PEP 529 for more details. */
    int legacy_windows_fs_encoding;

    /* If greater than zero, use io.FileIO instead of WindowsConsoleIO for sys
       standard streams.

       Set to 1 if the PYTHONLEGACYWINDOWSSTDIO environment variable is set to
       a non-empty string. If set to -1 (default), inherit
       Py_LegacyWindowsStdioFlag value.

       See PEP 528 for more details. */
    int legacy_windows_stdio;
#endif

    /* --- Private fields -------- */

    /* Install importlib? If set to 0, importlib is not initialized at all.
       Needed by freeze_importlib. */
    int _install_importlib;

    /* Value of the --check-hash-based-pycs configure option. Valid values:

       - "default" means the 'check_source' flag in hash-based pycs
         determines invalidation
       - "always" causes the interpreter to hash the source file for
         invalidation regardless of value of 'check_source' bit
       - "never" causes the interpreter to always assume hash-based pycs are
         valid

       Set by the --check-hash-based-pycs command line option.
       If set to NULL (default), inherit _Py_CheckHashBasedPycsMode value.

       See PEP 552 "Deterministic pycs" for more details. */
    const char *_check_hash_pycs_mode;
} _PyCoreConfig;

#ifdef MS_WINDOWS
#  define _PyCoreConfig_WINDOWS_INIT \
        .legacy_windows_fs_encoding = -1, \
        .legacy_windows_stdio = -1,
#else
#  define _PyCoreConfig_WINDOWS_INIT
#endif

#define _PyCoreConfig_INIT \
    (_PyCoreConfig){ \
        .install_signal_handlers = 1, \
        .use_environment = -1, \
        .use_hash_seed = -1, \
        .faulthandler = -1, \
        .tracemalloc = -1, \
        .coerce_c_locale = -1, \
        .utf8_mode = -1, \
        .argc = -1, \
        .nmodule_search_path = -1, \
        .isolated = -1, \
        .site_import = -1, \
        .bytes_warning = -1, \
        .inspect = -1, \
        .interactive = -1, \
        .optimization_level = -1, \
        .debug= -1, \
        .write_bytecode = -1, \
        .verbose = -1, \
        .quiet = -1, \
        .user_site_directory = -1, \
        .unbuffered_stdio = -1, \
        _PyCoreConfig_WINDOWS_INIT \
        ._install_importlib = 1}
/* Note: _PyCoreConfig_INIT sets other fields to 0/NULL */

/* Placeholders while working on the new configuration API
 *
 * See PEP 432 for final anticipated contents
 */
typedef struct {
    int install_signal_handlers;   /* Install signal handlers? -1 means unset */
    PyObject *argv;                /* sys.argv list, can be NULL */
    PyObject *executable;          /* sys.executable str */
    PyObject *prefix;              /* sys.prefix str */
    PyObject *base_prefix;         /* sys.base_prefix str, can be NULL */
    PyObject *exec_prefix;         /* sys.exec_prefix str */
    PyObject *base_exec_prefix;    /* sys.base_exec_prefix str, can be NULL */
    PyObject *warnoptions;         /* sys.warnoptions list, can be NULL */
    PyObject *xoptions;            /* sys._xoptions dict, can be NULL */
    PyObject *module_search_path;  /* sys.path list */
    PyObject *pycache_prefix;      /* sys.pycache_prefix str, can be NULL */
} _PyMainInterpreterConfig;

#define _PyMainInterpreterConfig_INIT \
    (_PyMainInterpreterConfig){.install_signal_handlers = -1}
/* Note: _PyMainInterpreterConfig_INIT sets other fields to 0/NULL */

typedef struct _is {

    struct _is *next;
    struct _ts *tstate_head;

    int64_t id;
    int64_t id_refcount;
    PyThread_type_lock id_mutex;

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
    int fscodec_initialized;

    _PyCoreConfig core_config;
    _PyMainInterpreterConfig config;
#ifdef HAVE_DLOPEN
    int dlopenflags;
#endif

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
} PyInterpreterState;
#endif   /* !Py_LIMITED_API */


/* State unique per thread */

#ifndef Py_LIMITED_API
/* Py_tracefunc return -1 when raising an exception, or 0 for success. */
typedef int (*Py_tracefunc)(PyObject *, struct _frame *, int, PyObject *);

/* The following values are used for 'what' for tracefunc functions
 *
 * To add a new kind of trace event, also update "trace_init" in
 * Python/sysmodule.c to define the Python level event name
 */
#define PyTrace_CALL 0
#define PyTrace_EXCEPTION 1
#define PyTrace_LINE 2
#define PyTrace_RETURN 3
#define PyTrace_C_CALL 4
#define PyTrace_C_EXCEPTION 5
#define PyTrace_C_RETURN 6
#define PyTrace_OPCODE 7
#endif   /* Py_LIMITED_API */

#ifdef Py_LIMITED_API
typedef struct _ts PyThreadState;
#else

typedef struct _err_stackitem {
    /* This struct represents an entry on the exception stack, which is a
     * per-coroutine state. (Coroutine in the computer science sense,
     * including the thread and generators).
     * This ensures that the exception state is not impacted by "yields"
     * from an except handler.
     */
    PyObject *exc_type, *exc_value, *exc_traceback;

    struct _err_stackitem *previous_item;

} _PyErr_StackItem;


typedef struct _ts {
    /* See Python/ceval.c for comments explaining most fields */

    struct _ts *prev;
    struct _ts *next;
    PyInterpreterState *interp;

    struct _frame *frame;
    int recursion_depth;
    char overflowed; /* The stack has overflowed. Allow 50 more calls
                        to handle the runtime error. */
    char recursion_critical; /* The current calls must not cause
                                a stack overflow. */
    int stackcheck_counter;

    /* 'tracing' keeps track of the execution depth when tracing/profiling.
       This is to prevent the actual trace/profile code from being recorded in
       the trace/profile. */
    int tracing;
    int use_tracing;

    Py_tracefunc c_profilefunc;
    Py_tracefunc c_tracefunc;
    PyObject *c_profileobj;
    PyObject *c_traceobj;

    /* The exception currently being raised */
    PyObject *curexc_type;
    PyObject *curexc_value;
    PyObject *curexc_traceback;

    /* The exception currently being handled, if no coroutines/generators
     * are present. Always last element on the stack referred to be exc_info.
     */
    _PyErr_StackItem exc_state;

    /* Pointer to the top of the stack of the exceptions currently
     * being handled */
    _PyErr_StackItem *exc_info;

    PyObject *dict;  /* Stores per-thread state */

    int gilstate_counter;

    PyObject *async_exc; /* Asynchronous exception to raise */
    unsigned long thread_id; /* Thread id where this tstate was created */

    int trash_delete_nesting;
    PyObject *trash_delete_later;

    /* Called when a thread state is deleted normally, but not when it
     * is destroyed after fork().
     * Pain:  to prevent rare but fatal shutdown errors (issue 18808),
     * Thread.join() must wait for the join'ed thread's tstate to be unlinked
     * from the tstate chain.  That happens at the end of a thread's life,
     * in pystate.c.
     * The obvious way doesn't quite work:  create a lock which the tstate
     * unlinking code releases, and have Thread.join() wait to acquire that
     * lock.  The problem is that we _are_ at the end of the thread's life:
     * if the thread holds the last reference to the lock, decref'ing the
     * lock will delete the lock, and that may trigger arbitrary Python code
     * if there's a weakref, with a callback, to the lock.  But by this time
     * _PyThreadState_Current is already NULL, so only the simplest of C code
     * can be allowed to run (in particular it must not be possible to
     * release the GIL).
     * So instead of holding the lock directly, the tstate holds a weakref to
     * the lock:  that's the value of on_delete_data below.  Decref'ing a
     * weakref is harmless.
     * on_delete points to _threadmodule.c's static release_sentinel() function.
     * After the tstate is unlinked, release_sentinel is called with the
     * weakref-to-lock (on_delete_data) argument, and release_sentinel releases
     * the indirectly held lock.
     */
    void (*on_delete)(void *);
    void *on_delete_data;

    int coroutine_origin_tracking_depth;

    PyObject *coroutine_wrapper;
    int in_coroutine_wrapper;

    PyObject *async_gen_firstiter;
    PyObject *async_gen_finalizer;

    PyObject *context;
    uint64_t context_ver;

    /* Unique thread state id. */
    uint64_t id;

    /* XXX signal handlers should also be here */

} PyThreadState;
#endif   /* !Py_LIMITED_API */


PyAPI_FUNC(PyInterpreterState *) PyInterpreterState_New(void);
PyAPI_FUNC(void) PyInterpreterState_Clear(PyInterpreterState *);
PyAPI_FUNC(void) PyInterpreterState_Delete(PyInterpreterState *);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03070000
/* New in 3.7 */
PyAPI_FUNC(int64_t) PyInterpreterState_GetID(PyInterpreterState *);
#endif
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyState_AddModule(PyObject*, struct PyModuleDef*);
#endif /* !Py_LIMITED_API */
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
/* New in 3.3 */
PyAPI_FUNC(int) PyState_AddModule(PyObject*, struct PyModuleDef*);
PyAPI_FUNC(int) PyState_RemoveModule(struct PyModuleDef*);
#endif
PyAPI_FUNC(PyObject*) PyState_FindModule(struct PyModuleDef*);
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyState_ClearModules(void);
#endif

PyAPI_FUNC(PyThreadState *) PyThreadState_New(PyInterpreterState *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyThreadState *) _PyThreadState_Prealloc(PyInterpreterState *);
PyAPI_FUNC(void) _PyThreadState_Init(PyThreadState *);
#endif /* !Py_LIMITED_API */
PyAPI_FUNC(void) PyThreadState_Clear(PyThreadState *);
PyAPI_FUNC(void) PyThreadState_Delete(PyThreadState *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyThreadState_DeleteExcept(PyThreadState *tstate);
#endif /* !Py_LIMITED_API */
PyAPI_FUNC(void) PyThreadState_DeleteCurrent(void);
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyGILState_Reinit(void);
#endif /* !Py_LIMITED_API */

/* Return the current thread state. The global interpreter lock must be held.
 * When the current thread state is NULL, this issues a fatal error (so that
 * the caller needn't check for NULL). */
PyAPI_FUNC(PyThreadState *) PyThreadState_Get(void);

#ifndef Py_LIMITED_API
/* Similar to PyThreadState_Get(), but don't issue a fatal error
 * if it is NULL. */
PyAPI_FUNC(PyThreadState *) _PyThreadState_UncheckedGet(void);
#endif /* !Py_LIMITED_API */

PyAPI_FUNC(PyThreadState *) PyThreadState_Swap(PyThreadState *);
PyAPI_FUNC(PyObject *) PyThreadState_GetDict(void);
PyAPI_FUNC(int) PyThreadState_SetAsyncExc(unsigned long, PyObject *);


/* Variable and macro for in-line access to current thread state */

/* Assuming the current thread holds the GIL, this is the
   PyThreadState for the current thread. */
#ifdef Py_BUILD_CORE
#  define _PyThreadState_Current _PyRuntime.gilstate.tstate_current
#  define PyThreadState_GET() \
             ((PyThreadState*)_Py_atomic_load_relaxed(&_PyThreadState_Current))
#else
#  define PyThreadState_GET() PyThreadState_Get()
#endif

typedef
    enum {PyGILState_LOCKED, PyGILState_UNLOCKED}
        PyGILState_STATE;


/* Ensure that the current thread is ready to call the Python
   C API, regardless of the current state of Python, or of its
   thread lock.  This may be called as many times as desired
   by a thread so long as each call is matched with a call to
   PyGILState_Release().  In general, other thread-state APIs may
   be used between _Ensure() and _Release() calls, so long as the
   thread-state is restored to its previous state before the Release().
   For example, normal use of the Py_BEGIN_ALLOW_THREADS/
   Py_END_ALLOW_THREADS macros are acceptable.

   The return value is an opaque "handle" to the thread state when
   PyGILState_Ensure() was called, and must be passed to
   PyGILState_Release() to ensure Python is left in the same state. Even
   though recursive calls are allowed, these handles can *not* be shared -
   each unique call to PyGILState_Ensure must save the handle for its
   call to PyGILState_Release.

   When the function returns, the current thread will hold the GIL.

   Failure is a fatal error.
*/
PyAPI_FUNC(PyGILState_STATE) PyGILState_Ensure(void);

/* Release any resources previously acquired.  After this call, Python's
   state will be the same as it was prior to the corresponding
   PyGILState_Ensure() call (but generally this state will be unknown to
   the caller, hence the use of the GILState API.)

   Every call to PyGILState_Ensure must be matched by a call to
   PyGILState_Release on the same thread.
*/
PyAPI_FUNC(void) PyGILState_Release(PyGILState_STATE);

/* Helper/diagnostic function - get the current thread state for
   this thread.  May return NULL if no GILState API has been used
   on the current thread.  Note that the main thread always has such a
   thread-state, even if no auto-thread-state call has been made
   on the main thread.
*/
PyAPI_FUNC(PyThreadState *) PyGILState_GetThisThreadState(void);

#ifndef Py_LIMITED_API
/* Helper/diagnostic function - return 1 if the current thread
   currently holds the GIL, 0 otherwise.

   The function returns 1 if _PyGILState_check_enabled is non-zero. */
PyAPI_FUNC(int) PyGILState_Check(void);

/* Unsafe function to get the single PyInterpreterState used by this process'
   GILState implementation.

   Return NULL before _PyGILState_Init() is called and after _PyGILState_Fini()
   is called. */
PyAPI_FUNC(PyInterpreterState *) _PyGILState_GetInterpreterStateUnsafe(void);
#endif   /* !Py_LIMITED_API */


/* The implementation of sys._current_frames()  Returns a dict mapping
   thread id to that thread's current frame.
*/
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyThread_CurrentFrames(void);
#endif

/* Routines for advanced debuggers, requested by David Beazley.
   Don't use unless you know what you are doing! */
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyInterpreterState *) PyInterpreterState_Main(void);
PyAPI_FUNC(PyInterpreterState *) PyInterpreterState_Head(void);
PyAPI_FUNC(PyInterpreterState *) PyInterpreterState_Next(PyInterpreterState *);
PyAPI_FUNC(PyThreadState *) PyInterpreterState_ThreadHead(PyInterpreterState *);
PyAPI_FUNC(PyThreadState *) PyThreadState_Next(PyThreadState *);

typedef struct _frame *(*PyThreadFrameGetter)(PyThreadState *self_);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATE_H */
