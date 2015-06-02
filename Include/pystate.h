
/* Thread and interpreter state structures and their interfaces */


#ifndef Py_PYSTATE_H
#define Py_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

/* State shared between threads */

struct _ts; /* Forward */
struct _is; /* Forward */

#ifdef Py_LIMITED_API
typedef struct _is PyInterpreterState;
#else
typedef struct _is {

    struct _is *next;
    struct _ts *tstate_head;

    PyObject *modules;
    PyObject *modules_by_index;
    PyObject *sysdict;
    PyObject *builtins;
    PyObject *importlib;

    PyObject *codec_search_path;
    PyObject *codec_search_cache;
    PyObject *codec_error_registry;
    int codecs_initialized;
    int fscodec_initialized;

#ifdef HAVE_DLOPEN
    int dlopenflags;
#endif
#ifdef WITH_TSC
    int tscdump;
#endif

    PyObject *builtins_copy;
} PyInterpreterState;
#endif


/* State unique per thread */

struct _frame; /* Avoid including frameobject.h */

#ifndef Py_LIMITED_API
/* Py_tracefunc return -1 when raising an exception, or 0 for success. */
typedef int (*Py_tracefunc)(PyObject *, struct _frame *, int, PyObject *);

/* The following values are used for 'what' for tracefunc functions: */
#define PyTrace_CALL 0
#define PyTrace_EXCEPTION 1
#define PyTrace_LINE 2
#define PyTrace_RETURN 3
#define PyTrace_C_CALL 4
#define PyTrace_C_EXCEPTION 5
#define PyTrace_C_RETURN 6
#endif

#ifdef Py_LIMITED_API
typedef struct _ts PyThreadState;
#else
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
    /* 'tracing' keeps track of the execution depth when tracing/profiling.
       This is to prevent the actual trace/profile code from being recorded in
       the trace/profile. */
    int tracing;
    int use_tracing;

    Py_tracefunc c_profilefunc;
    Py_tracefunc c_tracefunc;
    PyObject *c_profileobj;
    PyObject *c_traceobj;

    PyObject *curexc_type;
    PyObject *curexc_value;
    PyObject *curexc_traceback;

    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_traceback;

    PyObject *dict;  /* Stores per-thread state */

    int gilstate_counter;

    PyObject *async_exc; /* Asynchronous exception to raise */
    long thread_id; /* Thread id where this tstate was created */

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

    PyObject *coroutine_wrapper;
    int in_coroutine_wrapper;

    /* XXX signal handlers should also be here */

} PyThreadState;
#endif


PyAPI_FUNC(PyInterpreterState *) PyInterpreterState_New(void);
PyAPI_FUNC(void) PyInterpreterState_Clear(PyInterpreterState *);
PyAPI_FUNC(void) PyInterpreterState_Delete(PyInterpreterState *);
PyAPI_FUNC(int) _PyState_AddModule(PyObject*, struct PyModuleDef*);
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
PyAPI_FUNC(PyThreadState *) _PyThreadState_Prealloc(PyInterpreterState *);
PyAPI_FUNC(void) _PyThreadState_Init(PyThreadState *);
PyAPI_FUNC(void) PyThreadState_Clear(PyThreadState *);
PyAPI_FUNC(void) PyThreadState_Delete(PyThreadState *);
PyAPI_FUNC(void) _PyThreadState_DeleteExcept(PyThreadState *tstate);
#ifdef WITH_THREAD
PyAPI_FUNC(void) PyThreadState_DeleteCurrent(void);
PyAPI_FUNC(void) _PyGILState_Reinit(void);
#endif

PyAPI_FUNC(PyThreadState *) PyThreadState_Get(void);
PyAPI_FUNC(PyThreadState *) PyThreadState_Swap(PyThreadState *);
PyAPI_FUNC(PyObject *) PyThreadState_GetDict(void);
PyAPI_FUNC(int) PyThreadState_SetAsyncExc(long, PyObject *);


/* Variable and macro for in-line access to current thread state */

/* Assuming the current thread holds the GIL, this is the
   PyThreadState for the current thread.

   Issue #23644: pyatomic.h is incompatible with C++ (yet). Disable
   PyThreadState_GET() optimization: declare it as an alias to
   PyThreadState_Get(), as done for limited API. */
#if !defined(Py_LIMITED_API) && !defined(__cplusplus)
PyAPI_DATA(_Py_atomic_address) _PyThreadState_Current;
#endif

#if defined(Py_DEBUG) || defined(Py_LIMITED_API) || defined(__cplusplus)
#define PyThreadState_GET() PyThreadState_Get()
#else
#define PyThreadState_GET() \
    ((PyThreadState*)_Py_atomic_load_relaxed(&_PyThreadState_Current))
#endif

typedef
    enum {PyGILState_LOCKED, PyGILState_UNLOCKED}
        PyGILState_STATE;

#ifdef WITH_THREAD

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

/* Helper/diagnostic function - return 1 if the current thread
 * currently holds the GIL, 0 otherwise
 */
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) PyGILState_Check(void);
#endif

#endif   /* #ifdef WITH_THREAD */

/* The implementation of sys._current_frames()  Returns a dict mapping
   thread id to that thread's current frame.
*/
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyThread_CurrentFrames(void);
#endif

/* Routines for advanced debuggers, requested by David Beazley.
   Don't use unless you know what you are doing! */
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyInterpreterState *) PyInterpreterState_Head(void);
PyAPI_FUNC(PyInterpreterState *) PyInterpreterState_Next(PyInterpreterState *);
PyAPI_FUNC(PyThreadState *) PyInterpreterState_ThreadHead(PyInterpreterState *);
PyAPI_FUNC(PyThreadState *) PyThreadState_Next(PyThreadState *);

typedef struct _frame *(*PyThreadFrameGetter)(PyThreadState *self_);
#endif

/* hook for PyEval_GetFrame(), requested for Psyco */
#ifndef Py_LIMITED_API
PyAPI_DATA(PyThreadFrameGetter) _PyThreadState_GetFrame;
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATE_H */
