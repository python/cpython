#ifndef Py_INTERNAL_LONG_H
#define Py_INTERNAL_LONG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interp.h"        // PyInterpreterState.small_ints
#include "pycore_pystate.h"       // _PyThreadState_GET()

// Don't call this function but _PyLong_GetZero() and _PyLong_GetOne()
static inline PyObject* __PyLong_GetSmallInt_internal(int value)
{
    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    _Py_EnsureTstateNotNULL(tstate);
#endif
    assert(-_PY_NSMALLNEGINTS <= value && value < _PY_NSMALLPOSINTS);
    size_t index = _PY_NSMALLNEGINTS + value;
    PyObject *obj = (PyObject*)tstate->interp->small_ints[index];
    // _PyLong_GetZero() and _PyLong_GetOne() must not be called
    // before _PyLong_Init() nor after _PyLong_Fini()
    assert(obj != NULL);
    return obj;
}

// Return a borrowed reference to the zero singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetZero(void)
{ return __PyLong_GetSmallInt_internal(0); }

// Return a borrowed reference to the one singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetOne(void)
{ return __PyLong_GetSmallInt_internal(1); }

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_H */
