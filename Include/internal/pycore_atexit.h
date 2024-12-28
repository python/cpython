#ifndef Py_INTERNAL_ATEXIT_H
#define Py_INTERNAL_ATEXIT_H

#include "pycore_lock.h"        // PyMutex

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


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

typedef void (*atexit_datacallbackfunc)(void *);

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

#ifdef Py_GIL_DISABLED
#  define _PyAtExit_LockCallbacks(state) PyMutex_Lock(&state->ll_callbacks_lock);
#  define _PyAtExit_UnlockCallbacks(state) PyMutex_Unlock(&state->ll_callbacks_lock);
#else
#  define _PyAtExit_LockCallbacks(state)
#  define _PyAtExit_UnlockCallbacks(state)
#endif

// Export for '_interpchannels' shared extension
PyAPI_FUNC(int) _Py_AtExit(
    PyInterpreterState *interp,
    atexit_datacallbackfunc func,
    void *data);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ATEXIT_H */
