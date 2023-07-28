#ifndef Py_INTERNAL_PYSTATE_H
#define Py_INTERNAL_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_runtime.h"   /* PyRuntimeState */


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
    return (_PyRuntimeState_GetFinalizing(interp->runtime) != NULL &&
            interp == &interp->runtime->_main_interpreter);
}


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
PyAPI_DATA(PyThreadState *) _PyThreadState_GetCurrent(void);

/* Get the current Python thread state.

   This function is unsafe: it does not check for error and it can return NULL.

   The caller must hold the GIL.

   See also PyThreadState_Get() and _PyThreadState_UncheckedGet(). */
static inline PyThreadState*
_PyThreadState_GET(void)
{
#if defined(HAVE_THREAD_LOCAL) && !defined(Py_BUILD_CORE_MODULE)
    return _Py_tss_tstate;
#else
    return _PyThreadState_GetCurrent();
#endif
}


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

   See also _PyInterpreterState_Get()
   and _PyGILState_GetInterpreterStateUnsafe(). */
static inline PyInterpreterState* _PyInterpreterState_GET(void) {
    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    _Py_EnsureTstateNotNULL(tstate);
#endif
    return tstate->interp;
}


// PyThreadState functions

PyAPI_FUNC(PyThreadState *) _PyThreadState_New(PyInterpreterState *interp);
PyAPI_FUNC(void) _PyThreadState_Bind(PyThreadState *tstate);
// We keep this around exclusively for stable ABI compatibility.
PyAPI_FUNC(void) _PyThreadState_Init(
    PyThreadState *tstate);
PyAPI_FUNC(void) _PyThreadState_DeleteExcept(PyThreadState *tstate);


/* Other */

PyAPI_FUNC(PyThreadState *) _PyThreadState_Swap(
    _PyRuntimeState *runtime,
    PyThreadState *newts);

PyAPI_FUNC(PyStatus) _PyInterpreterState_Enable(_PyRuntimeState *runtime);

#ifdef HAVE_FORK
extern PyStatus _PyInterpreterState_DeleteExceptMain(_PyRuntimeState *runtime);
extern void _PySignal_AfterFork(void);
#endif


PyAPI_FUNC(int) _PyState_AddModule(
    PyThreadState *tstate,
    PyObject* module,
    PyModuleDef* def);


PyAPI_FUNC(int) _PyOS_InterruptOccurred(PyThreadState *tstate);

#define HEAD_LOCK(runtime) \
    PyThread_acquire_lock((runtime)->interpreters.mutex, WAIT_LOCK)
#define HEAD_UNLOCK(runtime) \
    PyThread_release_lock((runtime)->interpreters.mutex)


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
