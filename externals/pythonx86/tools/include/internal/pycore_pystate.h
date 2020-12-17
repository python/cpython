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

static inline PyThreadState*
_PyRuntimeState_GetThreadState(_PyRuntimeState *runtime)
{
    return (PyThreadState*)_Py_atomic_load_relaxed(&runtime->gilstate.tstate_current);
}

/* Get the current Python thread state.

   Efficient macro reading directly the 'gilstate.tstate_current' atomic
   variable. The macro is unsafe: it does not check for error and it can
   return NULL.

   The caller must hold the GIL.

   See also PyThreadState_Get() and PyThreadState_GET(). */
static inline PyThreadState*
_PyThreadState_GET(void)
{
    return _PyRuntimeState_GetThreadState(&_PyRuntime);
}

/* Redefine PyThreadState_GET() as an alias to _PyThreadState_GET() */
#undef PyThreadState_GET
#define PyThreadState_GET() _PyThreadState_GET()

PyAPI_FUNC(void) _Py_NO_RETURN _Py_FatalError_TstateNULL(const char *func);

static inline void
_Py_EnsureFuncTstateNotNULL(const char *func, PyThreadState *tstate)
{
    if (tstate == NULL) {
        _Py_FatalError_TstateNULL(func);
    }
}

// Call Py_FatalError() if tstate is NULL
#define _Py_EnsureTstateNotNULL(tstate) \
    _Py_EnsureFuncTstateNotNULL(__func__, tstate)


/* Get the current interpreter state.

   The macro is unsafe: it does not check for error and it can return NULL.

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

PyAPI_FUNC(void) _PyGILState_Reinit(_PyRuntimeState *runtime);


PyAPI_FUNC(int) _PyState_AddModule(
    PyThreadState *tstate,
    PyObject* module,
    struct PyModuleDef* def);


PyAPI_FUNC(int) _PyOS_InterruptOccurred(PyThreadState *tstate);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
