#ifndef Py_INTERNAL_PYSTATE_H
#define Py_INTERNAL_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist.h"      // _PyFreeListState
#include "pycore_runtime.h"       // _PyRuntime
#include "pycore_tstate.h"        // _PyThreadStateImpl


// Values for PyThreadState.state. A thread must be in the "attached" state
// before calling most Python APIs. If the GIL is enabled, then "attached"
// implies that the thread holds the GIL and "detached" implies that the
// thread does not hold the GIL (or is in the process of releasing it). In
// `--disable-gil` builds, multiple threads may be "attached" to the same
// interpreter at the same time. Only the "bound" thread may perform the
// transitions between "attached" and "detached" on its own PyThreadState.
//
// The "gc" state is used to implement stop-the-world pauses, such as for
// cyclic garbage collection. It is only used in `--disable-gil` builds. It is
// similar to the "detached" state, but only the thread performing a
// stop-the-world pause may transition threads between the "detached" and "gc"
// states. A thread trying to "attach" from the "gc" state will block until
// it is transitioned back to "detached" when the stop-the-world pause is
// complete.
//
// State transition diagram:
//
//            (bound thread)        (stop-the-world thread)
// [attached]       <->       [detached]       <->       [gc]
//
// See `_PyThreadState_Attach()` and `_PyThreadState_Detach()`.
#define _Py_THREAD_DETACHED     0
#define _Py_THREAD_ATTACHED     1
#define _Py_THREAD_GC           2


/* Check if the current thread is the main thread.
   Use _Py_IsMainInterpreter() to check if it's the main interpreter. */
static inline int
_Py_IsMainThread(void)
{
    unsigned long thread = PyThread_get_thread_ident();
    return (thread == _PyRuntime.main_thread);
}


static inline PyInterpreterState *
_PyInterpreterState_Main(void)
{
    return _PyRuntime.interpreters.main;
}

static inline int
_Py_IsMainInterpreter(PyInterpreterState *interp)
{
    return (interp == _PyInterpreterState_Main());
}

static inline int
_Py_IsMainInterpreterFinalizing(PyInterpreterState *interp)
{
    /* bpo-39877: Access _PyRuntime directly rather than using
       tstate->interp->runtime to support calls from Python daemon threads.
       After Py_Finalize() has been called, tstate can be a dangling pointer:
       point to PyThreadState freed memory. */
    return (_PyRuntimeState_GetFinalizing(&_PyRuntime) != NULL &&
            interp == &_PyRuntime._main_interpreter);
}

// Export for _xxsubinterpreters module.
PyAPI_FUNC(int) _PyInterpreterState_SetRunningMain(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_SetNotRunningMain(PyInterpreterState *);
PyAPI_FUNC(int) _PyInterpreterState_IsRunningMain(PyInterpreterState *);
PyAPI_FUNC(int) _PyInterpreterState_FailIfRunningMain(PyInterpreterState *);


static inline const PyConfig *
_Py_GetMainConfig(void)
{
    PyInterpreterState *interp = _PyInterpreterState_Main();
    if (interp == NULL) {
        return NULL;
    }
    return _PyInterpreterState_GetConfig(interp);
}


/* Only handle signals on the main thread of the main interpreter. */
static inline int
_Py_ThreadCanHandleSignals(PyInterpreterState *interp)
{
    return (_Py_IsMainThread() && _Py_IsMainInterpreter(interp));
}


/* Variable and static inline functions for in-line access to current thread
   and interpreter state */

#if defined(HAVE_THREAD_LOCAL) && !defined(Py_BUILD_CORE_MODULE)
extern _Py_thread_local PyThreadState *_Py_tss_tstate;
#endif

#ifndef NDEBUG
extern int _PyThreadState_CheckConsistency(PyThreadState *tstate);
#endif

int _PyThreadState_MustExit(PyThreadState *tstate);

// Export for most shared extensions, used via _PyThreadState_GET() static
// inline function.
PyAPI_FUNC(PyThreadState *) _PyThreadState_GetCurrent(void);

/* Get the current Python thread state.

   This function is unsafe: it does not check for error and it can return NULL.

   The caller must hold the GIL.

   See also PyThreadState_Get() and PyThreadState_GetUnchecked(). */
static inline PyThreadState*
_PyThreadState_GET(void)
{
#if defined(HAVE_THREAD_LOCAL) && !defined(Py_BUILD_CORE_MODULE)
    return _Py_tss_tstate;
#else
    return _PyThreadState_GetCurrent();
#endif
}

// Attaches the current thread to the interpreter.
//
// This may block while acquiring the GIL (if the GIL is enabled) or while
// waiting for a stop-the-world pause (if the GIL is disabled).
//
// High-level code should generally call PyEval_RestoreThread() instead, which
// calls this function.
void _PyThreadState_Attach(PyThreadState *tstate);

// Detaches the current thread from the interpreter.
//
// High-level code should generally call PyEval_SaveThread() instead, which
// calls this function.
void _PyThreadState_Detach(PyThreadState *tstate);


static inline void
_Py_EnsureFuncTstateNotNULL(const char *func, PyThreadState *tstate)
{
    if (tstate == NULL) {
        _Py_FatalErrorFunc(func,
            "the function must be called with the GIL held, "
            "after Python initialization and before Python finalization, "
            "but the GIL is released (the current Python thread state is NULL)");
    }
}

// Call Py_FatalError() if tstate is NULL
#define _Py_EnsureTstateNotNULL(tstate) \
    _Py_EnsureFuncTstateNotNULL(__func__, (tstate))


/* Get the current interpreter state.

   The function is unsafe: it does not check for error and it can return NULL.

   The caller must hold the GIL.

   See also PyInterpreterState_Get()
   and _PyGILState_GetInterpreterStateUnsafe(). */
static inline PyInterpreterState* _PyInterpreterState_GET(void) {
    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    _Py_EnsureTstateNotNULL(tstate);
#endif
    return tstate->interp;
}


// PyThreadState functions

extern PyThreadState * _PyThreadState_New(
    PyInterpreterState *interp,
    int whence);
extern void _PyThreadState_Bind(PyThreadState *tstate);
extern void _PyThreadState_DeleteExcept(PyThreadState *tstate);
extern void _PyThreadState_ClearMimallocHeaps(PyThreadState *tstate);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyObject*) _PyThreadState_GetDict(PyThreadState *tstate);

/* The implementation of sys._current_exceptions()  Returns a dict mapping
   thread id to that thread's current exception.
*/
extern PyObject* _PyThread_CurrentExceptions(void);


/* Other */

extern PyThreadState * _PyThreadState_Swap(
    _PyRuntimeState *runtime,
    PyThreadState *newts);

extern PyStatus _PyInterpreterState_Enable(_PyRuntimeState *runtime);

#ifdef HAVE_FORK
extern PyStatus _PyInterpreterState_DeleteExceptMain(_PyRuntimeState *runtime);
extern void _PySignal_AfterFork(void);
#endif

// Export for the stable ABI
PyAPI_FUNC(int) _PyState_AddModule(
    PyThreadState *tstate,
    PyObject* module,
    PyModuleDef* def);


extern int _PyOS_InterruptOccurred(PyThreadState *tstate);

#define HEAD_LOCK(runtime) \
    PyMutex_LockFlags(&(runtime)->interpreters.mutex, _Py_LOCK_DONT_DETACH)
#define HEAD_UNLOCK(runtime) \
    PyMutex_Unlock(&(runtime)->interpreters.mutex)

// Get the configuration of the current interpreter.
// The caller must hold the GIL.
// Export for test_peg_generator.
PyAPI_FUNC(const PyConfig*) _Py_GetConfig(void);

// Get the single PyInterpreterState used by this process' GILState
// implementation.
//
// This function doesn't check for error. Return NULL before _PyGILState_Init()
// is called and after _PyGILState_Fini() is called.
//
// See also PyInterpreterState_Get() and _PyInterpreterState_GET().
extern PyInterpreterState* _PyGILState_GetInterpreterStateUnsafe(void);

static inline _PyFreeListState* _PyFreeListState_GET(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    _Py_EnsureTstateNotNULL(tstate);
#endif

#ifdef Py_GIL_DISABLED
    return &((_PyThreadStateImpl*)tstate)->freelist_state;
#else
    return &tstate->interp->freelist_state;
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
