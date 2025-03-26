#ifndef Py_INTERNAL_CROSSINTERP_H
#define Py_INTERNAL_CROSSINTERP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pyerrors.h"


/**************/
/* exceptions */
/**************/

PyAPI_DATA(PyObject *) PyExc_InterpreterError;
PyAPI_DATA(PyObject *) PyExc_InterpreterNotFoundError;


/***************************/
/* cross-interpreter calls */
/***************************/

typedef int (*_Py_simple_func)(void *);
extern int _Py_CallInInterpreter(
    PyInterpreterState *interp,
    _Py_simple_func func,
    void *arg);
extern int _Py_CallInInterpreterAndRawFree(
    PyInterpreterState *interp,
    _Py_simple_func func,
    void *arg);


/**************************/
/* cross-interpreter data */
/**************************/

typedef struct _xidata _PyXIData_t;
typedef PyObject *(*xid_newobjfunc)(_PyXIData_t *);
typedef void (*xid_freefunc)(void *);

// _PyXIData_t is similar to Py_buffer as an effectively
// opaque struct that holds data outside the object machinery.  This
// is necessary to pass safely between interpreters in the same process.
struct _xidata {
    // data is the cross-interpreter-safe derivation of a Python object
    // (see _PyObject_GetXIData).  It will be NULL if the
    // new_object func (below) encodes the data.
    void *data;
    // obj is the Python object from which the data was derived.  This
    // is non-NULL only if the data remains bound to the object in some
    // way, such that the object must be "released" (via a decref) when
    // the data is released.  In that case the code that sets the field,
    // likely a registered "xidatafunc", is responsible for
    // ensuring it owns the reference (i.e. incref).
    PyObject *obj;
    // interp is the ID of the owning interpreter of the original
    // object.  It corresponds to the active interpreter when
    // _PyObject_GetXIData() was called.  This should only
    // be set by the cross-interpreter machinery.
    //
    // We use the ID rather than the PyInterpreterState to avoid issues
    // with deleted interpreters.  Note that IDs are never re-used, so
    // each one will always correspond to a specific interpreter
    // (whether still alive or not).
    int64_t interpid;
    // new_object is a function that returns a new object in the current
    // interpreter given the data.  The resulting object (a new
    // reference) will be equivalent to the original object.  This field
    // is required.
    xid_newobjfunc new_object;
    // free is called when the data is released.  If it is NULL then
    // nothing will be done to free the data.  For some types this is
    // okay (e.g. bytes) and for those types this field should be set
    // to NULL.  However, for most the data was allocated just for
    // cross-interpreter use, so it must be freed when
    // _PyXIData_Release is called or the memory will
    // leak.  In that case, at the very least this field should be set
    // to PyMem_RawFree (the default if not explicitly set to NULL).
    // The call will happen with the original interpreter activated.
    xid_freefunc free;
};

PyAPI_FUNC(_PyXIData_t *) _PyXIData_New(void);
PyAPI_FUNC(void) _PyXIData_Free(_PyXIData_t *data);

#define _PyXIData_DATA(DATA) ((DATA)->data)
#define _PyXIData_OBJ(DATA) ((DATA)->obj)
#define _PyXIData_INTERPID(DATA) ((DATA)->interpid)
// Users should not need getters for "new_object" or "free".


/* getting cross-interpreter data */

typedef int (*xidatafunc)(PyThreadState *tstate, PyObject *, _PyXIData_t *);

typedef struct _xid_lookup_state _PyXIData_lookup_t;

typedef struct {
    _PyXIData_lookup_t *global;
    _PyXIData_lookup_t *local;
    PyObject *PyExc_NotShareableError;
} _PyXIData_lookup_context_t;

PyAPI_FUNC(int) _PyXIData_GetLookupContext(
        PyInterpreterState *,
        _PyXIData_lookup_context_t *);

PyAPI_FUNC(xidatafunc) _PyXIData_Lookup(
        _PyXIData_lookup_context_t *,
        PyObject *);
PyAPI_FUNC(int) _PyObject_CheckXIData(
        _PyXIData_lookup_context_t *,
        PyObject *);
PyAPI_FUNC(int) _PyObject_GetXIData(
        _PyXIData_lookup_context_t *,
        PyObject *,
        _PyXIData_t *);


/* using cross-interpreter data */

PyAPI_FUNC(PyObject *) _PyXIData_NewObject(_PyXIData_t *);
PyAPI_FUNC(int) _PyXIData_Release(_PyXIData_t *);
PyAPI_FUNC(int) _PyXIData_ReleaseAndRawFree(_PyXIData_t *);


/* defining cross-interpreter data */

PyAPI_FUNC(void) _PyXIData_Init(
        _PyXIData_t *data,
        PyInterpreterState *interp, void *shared, PyObject *obj,
        xid_newobjfunc new_object);
PyAPI_FUNC(int) _PyXIData_InitWithSize(
        _PyXIData_t *,
        PyInterpreterState *interp, const size_t, PyObject *,
        xid_newobjfunc);
PyAPI_FUNC(void) _PyXIData_Clear( PyInterpreterState *, _PyXIData_t *);

// Normally the Init* functions are sufficient.  The only time
// additional initialization might be needed is to set the "free" func,
// though that should be infrequent.
#define _PyXIData_SET_FREE(DATA, FUNC) \
    do { \
        (DATA)->free = (FUNC); \
    } while (0)
// Additionally, some shareable types are essentially light wrappers
// around other shareable types.  The xidatafunc of the wrapper
// can often be implemented by calling the wrapped object's
// xidatafunc and then changing the "new_object" function.
// We have _PyXIData_SET_NEW_OBJECT() here for that,
// but might be better to have a function like
// _PyXIData_AdaptToWrapper() instead.
#define _PyXIData_SET_NEW_OBJECT(DATA, FUNC) \
    do { \
        (DATA)->new_object = (FUNC); \
    } while (0)


/* cross-interpreter data registry */

#define Py_CORE_CROSSINTERP_DATA_REGISTRY_H
#include "pycore_crossinterp_data_registry.h"
#undef Py_CORE_CROSSINTERP_DATA_REGISTRY_H


/*****************************/
/* runtime state & lifecycle */
/*****************************/

typedef struct {
    // builtin types
    _PyXIData_lookup_t data_lookup;
} _PyXI_global_state_t;

typedef struct {
    // heap types
    _PyXIData_lookup_t data_lookup;

    struct xi_exceptions {
        // static types
        PyObject *PyExc_InterpreterError;
        PyObject *PyExc_InterpreterNotFoundError;
        // heap types
        PyObject *PyExc_NotShareableError;
    } exceptions;
} _PyXI_state_t;

#define _PyXI_GET_GLOBAL_STATE(interp) (&(interp)->runtime->xi)
#define _PyXI_GET_STATE(interp) (&(interp)->xi)

#ifndef Py_BUILD_CORE_MODULE
extern PyStatus _PyXI_Init(PyInterpreterState *interp);
extern void _PyXI_Fini(PyInterpreterState *interp);
extern PyStatus _PyXI_InitTypes(PyInterpreterState *interp);
extern void _PyXI_FiniTypes(PyInterpreterState *interp);
#endif  // Py_BUILD_CORE_MODULE

int _Py_xi_global_state_init(_PyXI_global_state_t *);
void _Py_xi_global_state_fini(_PyXI_global_state_t *);
int _Py_xi_state_init(_PyXI_state_t *, PyInterpreterState *);
void _Py_xi_state_fini(_PyXI_state_t *, PyInterpreterState *);


/***************************/
/* short-term data sharing */
/***************************/

// Ultimately we'd like to preserve enough information about the
// exception and traceback that we could re-constitute (or at least
// simulate, a la traceback.TracebackException), and even chain, a copy
// of the exception in the calling interpreter.

typedef struct _excinfo {
    struct _excinfo_type {
        PyTypeObject *builtin;
        const char *name;
        const char *qualname;
        const char *module;
    } type;
    const char *msg;
    const char *errdisplay;
} _PyXI_excinfo;

PyAPI_FUNC(int) _PyXI_InitExcInfo(_PyXI_excinfo *info, PyObject *exc);
PyAPI_FUNC(PyObject *) _PyXI_FormatExcInfo(_PyXI_excinfo *info);
PyAPI_FUNC(PyObject *) _PyXI_ExcInfoAsObject(_PyXI_excinfo *info);
PyAPI_FUNC(void) _PyXI_ClearExcInfo(_PyXI_excinfo *info);


typedef enum error_code {
    _PyXI_ERR_NO_ERROR = 0,
    _PyXI_ERR_UNCAUGHT_EXCEPTION = -1,
    _PyXI_ERR_OTHER = -2,
    _PyXI_ERR_NO_MEMORY = -3,
    _PyXI_ERR_ALREADY_RUNNING = -4,
    _PyXI_ERR_MAIN_NS_FAILURE = -5,
    _PyXI_ERR_APPLY_NS_FAILURE = -6,
    _PyXI_ERR_NOT_SHAREABLE = -7,
} _PyXI_errcode;


typedef struct _sharedexception {
    // The originating interpreter.
    PyInterpreterState *interp;
    // The kind of error to propagate.
    _PyXI_errcode code;
    // The exception information to propagate, if applicable.
    // This is populated only for some error codes,
    // but always for _PyXI_ERR_UNCAUGHT_EXCEPTION.
    _PyXI_excinfo uncaught;
} _PyXI_error;

PyAPI_FUNC(PyObject *) _PyXI_ApplyError(_PyXI_error *err);


typedef struct xi_session _PyXI_session;
typedef struct _sharedns _PyXI_namespace;

PyAPI_FUNC(void) _PyXI_FreeNamespace(_PyXI_namespace *ns);
PyAPI_FUNC(_PyXI_namespace *) _PyXI_NamespaceFromNames(PyObject *names);
PyAPI_FUNC(int) _PyXI_FillNamespaceFromDict(
    _PyXI_namespace *ns,
    PyObject *nsobj,
    _PyXI_session *session);
PyAPI_FUNC(int) _PyXI_ApplyNamespace(
    _PyXI_namespace *ns,
    PyObject *nsobj,
    PyObject *dflt);


// A cross-interpreter session involves entering an interpreter
// (_PyXI_Enter()), doing some work with it, and finally exiting
// that interpreter (_PyXI_Exit()).
//
// At the boundaries of the session, both entering and exiting,
// data may be exchanged between the previous interpreter and the
// target one in a thread-safe way that does not violate the
// isolation between interpreters.  This includes setting objects
// in the target's __main__ module on the way in, and capturing
// uncaught exceptions on the way out.
struct xi_session {
    // Once a session has been entered, this is the tstate that was
    // current before the session.  If it is different from cur_tstate
    // then we must have switched interpreters.  Either way, this will
    // be the current tstate once we exit the session.
    PyThreadState *prev_tstate;
    // Once a session has been entered, this is the current tstate.
    // It must be current when the session exits.
    PyThreadState *init_tstate;
    // This is true if init_tstate needs cleanup during exit.
    int own_init_tstate;

    // This is true if, while entering the session, init_thread took
    // "ownership" of the interpreter's __main__ module.  This means
    // it is the only thread that is allowed to run code there.
    // (Caveat: for now, users may still run exec() against the
    // __main__ module's dict, though that isn't advisable.)
    int running;
    // This is a cached reference to the __dict__ of the entered
    // interpreter's __main__ module.  It is looked up when at the
    // beginning of the session as a convenience.
    PyObject *main_ns;

    // This is set if the interpreter is entered and raised an exception
    // that needs to be handled in some special way during exit.
    _PyXI_errcode *error_override;
    // This is set if exit captured an exception to propagate.
    _PyXI_error *error;

    // -- pre-allocated memory --
    _PyXI_error _error;
    _PyXI_errcode _error_override;
};

PyAPI_FUNC(int) _PyXI_Enter(
    _PyXI_session *session,
    PyInterpreterState *interp,
    PyObject *nsupdates);
PyAPI_FUNC(void) _PyXI_Exit(_PyXI_session *session);

PyAPI_FUNC(PyObject *) _PyXI_ApplyCapturedException(_PyXI_session *session);
PyAPI_FUNC(int) _PyXI_HasCapturedException(_PyXI_session *session);


/*************/
/* other API */
/*************/

// Export for _testinternalcapi shared extension
PyAPI_FUNC(PyInterpreterState *) _PyXI_NewInterpreter(
    PyInterpreterConfig *config,
    long *maybe_whence,
    PyThreadState **p_tstate,
    PyThreadState **p_save_tstate);
PyAPI_FUNC(void) _PyXI_EndInterpreter(
    PyInterpreterState *interp,
    PyThreadState *tstate,
    PyThreadState **p_save_tstate);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CROSSINTERP_H */
