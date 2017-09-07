#ifndef Py_INTERNAL_PYSTATE_H
#define Py_INTERNAL_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pystate.h"
#include "pyatomic.h"
#include "pythread.h"

#include "internal/mem.h"
#include "internal/ceval.h"
#include "internal/warnings.h"


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
    int autoTLSkey;
};

/* hook for PyEval_GetFrame(), requested for Psyco */
#define _PyThreadState_GetFrame _PyRuntime.gilstate.getframe

/* Issue #26558: Flag to disable PyGILState_Check().
   If set to non-zero, PyGILState_Check() always return 1. */
#define _PyGILState_check_enabled _PyRuntime.gilstate.check_enabled


/* Full Python runtime state */

typedef struct pyruntimestate {
    int initialized;
    int core_initialized;
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

#define NEXITFUNCS 32
    void (*exitfuncs[NEXITFUNCS])(void);
    int nexitfuncs;
    void (*pyexitfunc)(void);

    struct _pyobj_runtime_state obj;
    struct _gc_runtime_state gc;
    struct _pymem_runtime_state mem;
    struct _warnings_runtime_state warnings;
    struct _ceval_runtime_state ceval;
    struct _gilstate_runtime_state gilstate;

    // XXX Consolidate globals found via the check-c-globals script.
} _PyRuntimeState;

PyAPI_DATA(_PyRuntimeState) _PyRuntime;
PyAPI_FUNC(void) _PyRuntimeState_Init(_PyRuntimeState *);
PyAPI_FUNC(void) _PyRuntimeState_Fini(_PyRuntimeState *);

#define _Py_CURRENTLY_FINALIZING(tstate) \
    (_PyRuntime.finalizing == tstate)


/* Other */

PyAPI_FUNC(void) _PyInterpreterState_Enable(_PyRuntimeState *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
