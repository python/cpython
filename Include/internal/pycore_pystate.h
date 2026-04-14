#ifndef Py_INTERNAL_PYSTATE_H
#define Py_INTERNAL_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pythonrun.h"     // _PyOS_STACK_MARGIN_SHIFT
#include "pycore_typedefs.h"      // _PyRuntimeState
#include "pycore_tstate.h"


// Values for PyThreadState.state. A thread must be in the "attached" state
// before calling most Python APIs. If the GIL is enabled, then "attached"
// implies that the thread holds the GIL and "detached" implies that the
// thread does not hold the GIL (or is in the process of releasing it). In
// `--disable-gil` builds, multiple threads may be "attached" to the same
// interpreter at the same time. Only the "bound" thread may perform the
// transitions between "attached" and "detached" on its own PyThreadState.
//
// The "suspended" state is used to implement stop-the-world pauses, such as
// for cyclic garbage collection. It is only used in `--disable-gil` builds.
// The "suspended" state is similar to the "detached" state in that in both
// states the thread is not allowed to call most Python APIs. However, unlike
// the "detached" state, a thread may not transition itself out from the
// "suspended" state. Only the thread performing a stop-the-world pause may
// transition a thread from the "suspended" state back to the "detached" state.
//
// The "shutting down" state is used when the interpreter is being finalized.
// Threads in this state can't do anything other than block the OS thread.
// (See _PyThreadState_HangThread).
//
// State transition diagram:
//
//            (bound thread)        (stop-the-world thread)
// [attached]       <->       [detached]       <->       [suspended]
//   |                                                        ^
//   +---------------------------->---------------------------+
//                          (bound thread)
//
// The (bound thread) and (stop-the-world thread) labels indicate which thread
// is allowed to perform the transition.
#define _Py_THREAD_DETACHED         0
#define _Py_THREAD_ATTACHED         1
#define _Py_THREAD_SUSPENDED        2
#define _Py_THREAD_SHUTTING_DOWN    3


/* Check if the current thread is the main thread.
   Use _Py_IsMainInterpreter() to check if it's the main interpreter. */
extern int _Py_IsMainThread(void);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyInterpreterState*) _PyInterpreterState_Main(void);

static inline int
_Py_IsMainInterpreter(PyInterpreterState *interp)
{
    return (interp == _PyInterpreterState_Main());
}

extern int _Py_IsMainInterpreterFinalizing(PyInterpreterState *interp);

// Export for _interpreters module.
PyAPI_FUNC(PyObject *) _PyInterpreterState_GetIDObject(PyInterpreterState *);

// Export for _interpreters module.
PyAPI_FUNC(int) _PyInterpreterState_SetRunningMain(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_SetNotRunningMain(PyInterpreterState *);
PyAPI_FUNC(int) _PyInterpreterState_IsRunningMain(PyInterpreterState *);
PyAPI_FUNC(void) _PyErr_SetInterpreterAlreadyRunning(void);

extern int _PyThreadState_IsRunningMain(PyThreadState *);
extern void _PyInterpreterState_ReinitRunningMain(PyThreadState *);
extern const PyConfig* _Py_GetMainConfig(void);


/* Only handle signals on the main thread of the main interpreter. */
static inline int
_Py_ThreadCanHandleSignals(PyInterpreterState *interp)
{
    return (_Py_IsMainThread() && _Py_IsMainInterpreter(interp));
}


/* Variable and static inline functions for in-line access to current thread
   and interpreter state */

#if !defined(Py_BUILD_CORE_MODULE)
extern _Py_thread_local PyThreadState *_Py_tss_tstate;
extern _Py_thread_local PyInterpreterState *_Py_tss_interp;
#endif

#ifndef NDEBUG
extern int _PyThreadState_CheckConsistency(PyThreadState *tstate);
#endif

extern int _PyThreadState_MustExit(PyThreadState *tstate);
extern void _PyThreadState_HangThread(PyThreadState *tstate);

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
#if !defined(Py_BUILD_CORE_MODULE)
    return _Py_tss_tstate;
#else
    return _PyThreadState_GetCurrent();
#endif
}

static inline int
_PyThreadState_IsAttached(PyThreadState *tstate)
{
    return (_Py_atomic_load_int_relaxed(&tstate->state) == _Py_THREAD_ATTACHED);
}

// Attaches the current thread to the interpreter.
//
// This may block while acquiring the GIL (if the GIL is enabled) or while
// waiting for a stop-the-world pause (if the GIL is disabled).
//
// High-level code should generally call PyEval_RestoreThread() instead, which
// calls this function.
extern void _PyThreadState_Attach(PyThreadState *tstate);

// Detaches the current thread from the interpreter.
//
// High-level code should generally call PyEval_SaveThread() instead, which
// calls this function.
extern void _PyThreadState_Detach(PyThreadState *tstate);

// Detaches the current thread to the "suspended" state if a stop-the-world
// pause is in progress.
//
// If there is no stop-the-world pause in progress, then the thread switches
// to the "detached" state.
extern void _PyThreadState_Suspend(PyThreadState *tstate);

// Mark the thread state as "shutting down". This is used during interpreter
// and runtime finalization. The thread may no longer attach to the
// interpreter and will instead block via _PyThreadState_HangThread().
extern void _PyThreadState_SetShuttingDown(PyThreadState *tstate);

// Perform a stop-the-world pause for all threads in the all interpreters.
//
// Threads in the "attached" state are paused and transitioned to the "GC"
// state. Threads in the "detached" state switch to the "GC" state, preventing
// them from reattaching until the stop-the-world pause is complete.
//
// NOTE: This is a no-op outside of Py_GIL_DISABLED builds.
extern void _PyEval_StopTheWorldAll(_PyRuntimeState *runtime);
extern void _PyEval_StartTheWorldAll(_PyRuntimeState *runtime);

// Perform a stop-the-world pause for threads in the specified interpreter.
//
// NOTE: This is a no-op outside of Py_GIL_DISABLED builds.
extern PyAPI_FUNC(void) _PyEval_StopTheWorld(PyInterpreterState *interp);
extern PyAPI_FUNC(void) _PyEval_StartTheWorld(PyInterpreterState *interp);


static inline void
_Py_EnsureFuncTstateNotNULL(const char *func, PyThreadState *tstate)
{
    if (tstate == NULL) {
#ifndef Py_GIL_DISABLED
        _Py_FatalErrorFunc(func,
            "the function must be called with the GIL held, "
            "after Python initialization and before Python finalization, "
            "but the GIL is released (the current Python thread state is NULL)");
#else
        _Py_FatalErrorFunc(func,
            "the function must be called with an active thread state, "
            "after Python initialization and before Python finalization, "
            "but it was called without an active thread state. "
            "Are you trying to call the C API inside of a Py_BEGIN_ALLOW_THREADS block?");
#endif
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
#ifdef Py_DEBUG
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);
#endif
#if !defined(Py_BUILD_CORE_MODULE)
    return _Py_tss_interp;
#else
    return _PyThreadState_GET()->interp;
#endif
}


// PyThreadState functions

// Export for _testinternalcapi
PyAPI_FUNC(PyThreadState *) _PyThreadState_New(
    PyInterpreterState *interp,
    int whence);
extern void _PyThreadState_Bind(PyThreadState *tstate);
PyAPI_FUNC(PyThreadState *) _PyThreadState_NewBound(
    PyInterpreterState *interp,
    int whence);
extern PyThreadState * _PyThreadState_RemoveExcept(PyThreadState *tstate);
extern void _PyThreadState_DeleteList(PyThreadState *list, int is_after_fork);
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

#define _Py_FOR_EACH_TSTATE_UNLOCKED(interp, t) \
    for (PyThreadState *t = interp->threads.head; t; t = t->next)
#define _Py_FOR_EACH_TSTATE_BEGIN(interp, t) \
    HEAD_LOCK(interp->runtime); \
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, t)
#define _Py_FOR_EACH_TSTATE_END(interp) \
    HEAD_UNLOCK(interp->runtime)


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

extern PyObject * _Py_GetMainModule(PyThreadState *);
extern int _Py_CheckMainModule(PyObject *module);

#ifndef NDEBUG
/* Modern equivalent of assert(PyGILState_Check()) */
static inline void
_Py_AssertHoldsTstateFunc(const char *func)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureFuncTstateNotNULL(func, tstate);
}
#define _Py_AssertHoldsTstate() _Py_AssertHoldsTstateFunc(__func__)
#else
#define _Py_AssertHoldsTstate()
#endif

#if !_Py__has_builtin(__builtin_frame_address) && !defined(__GNUC__) && !defined(_MSC_VER)
static uintptr_t return_pointer_as_int(char* p) {
    return (uintptr_t)p;
}
#endif

static inline uintptr_t
_Py_get_machine_stack_pointer(void) {
#if _Py__has_builtin(__builtin_frame_address) || defined(__GNUC__)
    return (uintptr_t)__builtin_frame_address(0);
#elif defined(_MSC_VER)
    return (uintptr_t)_AddressOfReturnAddress();
#else
    char here;
    /* Avoid compiler warning about returning stack address */
    return return_pointer_as_int(&here);
#endif
}

static inline intptr_t
_Py_RecursionLimit_GetMargin(PyThreadState *tstate)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    assert(_tstate->c_stack_hard_limit != 0);
    intptr_t here_addr = _Py_get_machine_stack_pointer();
#if _Py_STACK_GROWS_DOWN
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, here_addr - (intptr_t)_tstate->c_stack_soft_limit, _PyOS_STACK_MARGIN_SHIFT);
#else
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (intptr_t)_tstate->c_stack_soft_limit - here_addr, _PyOS_STACK_MARGIN_SHIFT);
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
