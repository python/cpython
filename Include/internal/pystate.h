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
    Py_tss_t autoTSSkey;
};

/* hook for PyEval_GetFrame(), requested for Psyco */
#define _PyThreadState_GetFrame _PyRuntime.gilstate.getframe

/* Issue #26558: Flag to disable PyGILState_Check().
   If set to non-zero, PyGILState_Check() always return 1. */
#define _PyGILState_check_enabled _PyRuntime.gilstate.check_enabled


typedef struct _PyPathConfig {
    /* Full path to the Python program */
    wchar_t *program_full_path;
    wchar_t *prefix;
#ifdef MS_WINDOWS
    wchar_t *dll_path;
#else
    wchar_t *exec_prefix;
#endif
    /* Set by Py_SetPath(), or computed by _PyPathConfig_Init() */
    wchar_t *module_search_path;
    /* Python program name */
    wchar_t *program_name;
    /* Set by Py_SetPythonHome() or PYTHONHOME environment variable */
    wchar_t *home;
    /* isolated and site_import are used to set Py_IsolatedFlag and
       Py_NoSiteFlag flags on Windows in read_pth_file(). These fields
       are ignored when their value are equal to -1 (unset). */
    int isolated;
    int site_import;
} _PyPathConfig;

#define _PyPathConfig_INIT \
    {.module_search_path = NULL, \
     .isolated = -1, \
     .site_import = -1}
/* Note: _PyPathConfig_INIT sets other fields to 0/NULL */

PyAPI_DATA(_PyPathConfig) _Py_path_config;

PyAPI_FUNC(_PyInitError) _PyPathConfig_Calculate_impl(
    _PyPathConfig *config,
    const _PyCoreConfig *core_config);
PyAPI_FUNC(void) _PyPathConfig_ClearGlobal(void);

PyAPI_FUNC(_PyInitError) _Py_wstrlist_append(
    int *len,
    wchar_t ***list,
    const wchar_t *str);

/* interpreter state */

PyAPI_FUNC(PyInterpreterState *) _PyInterpreterState_LookUpID(PY_INT64_T);

PyAPI_FUNC(int) _PyInterpreterState_IDInitref(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_IDIncref(PyInterpreterState *);
PyAPI_FUNC(void) _PyInterpreterState_IDDecref(PyInterpreterState *);


/* cross-interpreter data */

struct _xid;

// _PyCrossInterpreterData is similar to Py_buffer as an effectively
// opaque struct that holds data outside the object machinery.  This
// is necessary to pass safely between interpreters in the same process.
typedef struct _xid {
    // data is the cross-interpreter-safe derivation of a Python object
    // (see _PyObject_GetCrossInterpreterData).  It will be NULL if the
    // new_object func (below) encodes the data.
    void *data;
    // obj is the Python object from which the data was derived.  This
    // is non-NULL only if the data remains bound to the object in some
    // way, such that the object must be "released" (via a decref) when
    // the data is released.  In that case the code that sets the field,
    // likely a registered "crossinterpdatafunc", is responsible for
    // ensuring it owns the reference (i.e. incref).
    PyObject *obj;
    // interp is the ID of the owning interpreter of the original
    // object.  It corresponds to the active interpreter when
    // _PyObject_GetCrossInterpreterData() was called.  This should only
    // be set by the cross-interpreter machinery.
    //
    // We use the ID rather than the PyInterpreterState to avoid issues
    // with deleted interpreters.
    int64_t interp;
    // new_object is a function that returns a new object in the current
    // interpreter given the data.  The resulting object (a new
    // reference) will be equivalent to the original object.  This field
    // is required.
    PyObject *(*new_object)(struct _xid *);
    // free is called when the data is released.  If it is NULL then
    // nothing will be done to free the data.  For some types this is
    // okay (e.g. bytes) and for those types this field should be set
    // to NULL.  However, for most the data was allocated just for
    // cross-interpreter use, so it must be freed when
    // _PyCrossInterpreterData_Release is called or the memory will
    // leak.  In that case, at the very least this field should be set
    // to PyMem_RawFree (the default if not explicitly set to NULL).
    // The call will happen with the original interpreter activated.
    void (*free)(void *);
} _PyCrossInterpreterData;

typedef int (*crossinterpdatafunc)(PyObject *, _PyCrossInterpreterData *);
PyAPI_FUNC(int) _PyObject_CheckCrossInterpreterData(PyObject *);

PyAPI_FUNC(int) _PyObject_GetCrossInterpreterData(PyObject *, _PyCrossInterpreterData *);
PyAPI_FUNC(PyObject *) _PyCrossInterpreterData_NewObject(_PyCrossInterpreterData *);
PyAPI_FUNC(void) _PyCrossInterpreterData_Release(_PyCrossInterpreterData *);

/* cross-interpreter data registry */

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   crossinterpdatafunc. It would be simpler and more efficient. */

PyAPI_FUNC(int) _PyCrossInterpreterData_Register_Class(PyTypeObject *, crossinterpdatafunc);
PyAPI_FUNC(crossinterpdatafunc) _PyCrossInterpreterData_Lookup(PyObject *);

struct _xidregitem;

struct _xidregitem {
    PyTypeObject *cls;
    crossinterpdatafunc getdata;
    struct _xidregitem *next;
};


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
    // XXX Remove this field once we have a tp_* slot.
    struct _xidregistry {
        PyThread_type_lock mutex;
        struct _xidregitem *head;
    } xidregistry;

#define NEXITFUNCS 32
    void (*exitfuncs[NEXITFUNCS])(void);
    int nexitfuncs;

    struct _gc_runtime_state gc;
    struct _warnings_runtime_state warnings;
    struct _ceval_runtime_state ceval;
    struct _gilstate_runtime_state gilstate;

    // XXX Consolidate globals found via the check-c-globals script.
} _PyRuntimeState;

#define _PyRuntimeState_INIT {.initialized = 0, .core_initialized = 0}
/* Note: _PyRuntimeState_INIT sets other fields to 0/NULL */

PyAPI_DATA(_PyRuntimeState) _PyRuntime;
PyAPI_FUNC(_PyInitError) _PyRuntimeState_Init(_PyRuntimeState *);
PyAPI_FUNC(void) _PyRuntimeState_Fini(_PyRuntimeState *);

/* Initialize _PyRuntimeState.
   Return NULL on success, or return an error message on failure. */
PyAPI_FUNC(_PyInitError) _PyRuntime_Initialize(void);

#define _Py_CURRENTLY_FINALIZING(tstate) \
    (_PyRuntime.finalizing == tstate)


/* Other */

PyAPI_FUNC(_PyInitError) _PyInterpreterState_Enable(_PyRuntimeState *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYSTATE_H */
