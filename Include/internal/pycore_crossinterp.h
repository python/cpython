#ifndef Py_INTERNAL_CROSSINTERP_H
#define Py_INTERNAL_CROSSINTERP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"            // PyMutex
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

typedef struct _xid _PyCrossInterpreterData;
typedef PyObject *(*xid_newobjectfunc)(_PyCrossInterpreterData *);
typedef void (*xid_freefunc)(void *);

// _PyCrossInterpreterData is similar to Py_buffer as an effectively
// opaque struct that holds data outside the object machinery.  This
// is necessary to pass safely between interpreters in the same process.
struct _xid {
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
    // with deleted interpreters.  Note that IDs are never re-used, so
    // each one will always correspond to a specific interpreter
    // (whether still alive or not).
    int64_t interpid;
    // new_object is a function that returns a new object in the current
    // interpreter given the data.  The resulting object (a new
    // reference) will be equivalent to the original object.  This field
    // is required.
    xid_newobjectfunc new_object;
    // free is called when the data is released.  If it is NULL then
    // nothing will be done to free the data.  For some types this is
    // okay (e.g. bytes) and for those types this field should be set
    // to NULL.  However, for most the data was allocated just for
    // cross-interpreter use, so it must be freed when
    // _PyCrossInterpreterData_Release is called or the memory will
    // leak.  In that case, at the very least this field should be set
    // to PyMem_RawFree (the default if not explicitly set to NULL).
    // The call will happen with the original interpreter activated.
    xid_freefunc free;
};

PyAPI_FUNC(_PyCrossInterpreterData *) _PyCrossInterpreterData_New(void);
PyAPI_FUNC(void) _PyCrossInterpreterData_Free(_PyCrossInterpreterData *data);

#define _PyCrossInterpreterData_DATA(DATA) ((DATA)->data)
#define _PyCrossInterpreterData_OBJ(DATA) ((DATA)->obj)
#define _PyCrossInterpreterData_INTERPID(DATA) ((DATA)->interpid)
// Users should not need getters for "new_object" or "free".


/* defining cross-interpreter data */

PyAPI_FUNC(void) _PyCrossInterpreterData_Init(
        _PyCrossInterpreterData *data,
        PyInterpreterState *interp, void *shared, PyObject *obj,
        xid_newobjectfunc new_object);
PyAPI_FUNC(int) _PyCrossInterpreterData_InitWithSize(
        _PyCrossInterpreterData *,
        PyInterpreterState *interp, const size_t, PyObject *,
        xid_newobjectfunc);
PyAPI_FUNC(void) _PyCrossInterpreterData_Clear(
        PyInterpreterState *, _PyCrossInterpreterData *);

// Normally the Init* functions are sufficient.  The only time
// additional initialization might be needed is to set the "free" func,
// though that should be infrequent.
#define _PyCrossInterpreterData_SET_FREE(DATA, FUNC) \
    do { \
        (DATA)->free = (FUNC); \
    } while (0)
// Additionally, some shareable types are essentially light wrappers
// around other shareable types.  The crossinterpdatafunc of the wrapper
// can often be implemented by calling the wrapped object's
// crossinterpdatafunc and then changing the "new_object" function.
// We have _PyCrossInterpreterData_SET_NEW_OBJECT() here for that,
// but might be better to have a function like
// _PyCrossInterpreterData_AdaptToWrapper() instead.
#define _PyCrossInterpreterData_SET_NEW_OBJECT(DATA, FUNC) \
    do { \
        (DATA)->new_object = (FUNC); \
    } while (0)


/* using cross-interpreter data */

PyAPI_FUNC(int) _PyObject_CheckCrossInterpreterData(PyObject *);
PyAPI_FUNC(int) _PyObject_GetCrossInterpreterData(PyObject *, _PyCrossInterpreterData *);
PyAPI_FUNC(PyObject *) _PyCrossInterpreterData_NewObject(_PyCrossInterpreterData *);
PyAPI_FUNC(int) _PyCrossInterpreterData_Release(_PyCrossInterpreterData *);
PyAPI_FUNC(int) _PyCrossInterpreterData_ReleaseAndRawFree(_PyCrossInterpreterData *);


/* cross-interpreter data registry */

// For now we use a global registry of shareable classes.  An
// alternative would be to add a tp_* slot for a class's
// crossinterpdatafunc. It would be simpler and more efficient.

typedef int (*crossinterpdatafunc)(PyThreadState *tstate, PyObject *,
                                   _PyCrossInterpreterData *);

struct _xidregitem;

struct _xidregitem {
    struct _xidregitem *prev;
    struct _xidregitem *next;
    /* This can be a dangling pointer, but only if weakref is set. */
    PyTypeObject *cls;
    /* This is NULL for builtin types. */
    PyObject *weakref;
    size_t refcount;
    crossinterpdatafunc getdata;
};

struct _xidregistry {
    int global;  /* builtin types or heap types */
    int initialized;
    PyMutex mutex;
    struct _xidregitem *head;
};

PyAPI_FUNC(int) _PyCrossInterpreterData_RegisterClass(PyTypeObject *, crossinterpdatafunc);
PyAPI_FUNC(int) _PyCrossInterpreterData_UnregisterClass(PyTypeObject *);
PyAPI_FUNC(crossinterpdatafunc) _PyCrossInterpreterData_Lookup(PyObject *);


/*****************************/
/* runtime state & lifecycle */
/*****************************/

struct _xi_runtime_state {
    // builtin types
    // XXX Remove this field once we have a tp_* slot.
    struct _xidregistry registry;
};

struct _xi_state {
    // heap types
    // XXX Remove this field once we have a tp_* slot.
    struct _xidregistry registry;

    // heap types
    PyObject *PyExc_NotShareableError;
};

extern PyStatus _PyXI_Init(PyInterpreterState *interp);
extern void _PyXI_Fini(PyInterpreterState *interp);

extern PyStatus _PyXI_InitTypes(PyInterpreterState *interp);
extern void _PyXI_FiniTypes(PyInterpreterState *interp);

#define _PyInterpreterState_GetXIState(interp) (&(interp)->xi)


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
