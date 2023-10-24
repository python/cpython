#ifndef Py_CPYTHON_CROSSINTERP_H
#  error "this header file must not be included directly"
#endif


/* cross-interpreter data */

// _PyCrossInterpreterData is similar to Py_buffer as an effectively
// opaque struct that holds data outside the object machinery.  This
// is necessary to pass safely between interpreters in the same process.
typedef struct _xid _PyCrossInterpreterData;

typedef PyObject *(*xid_newobjectfunc)(_PyCrossInterpreterData *);
typedef void (*xid_freefunc)(void *);

PyAPI_FUNC(_PyCrossInterpreterData *) _PyCrossInterpreterData_New(void);
PyAPI_FUNC(void) _PyCrossInterpreterData_Free(_PyCrossInterpreterData *data);

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

PyAPI_FUNC(int) _PyObject_GetCrossInterpreterData(PyObject *, _PyCrossInterpreterData *);
PyAPI_FUNC(PyObject *) _PyCrossInterpreterData_NewObject(_PyCrossInterpreterData *);
PyAPI_FUNC(int) _PyCrossInterpreterData_Release(_PyCrossInterpreterData *);
PyAPI_FUNC(int) _PyCrossInterpreterData_ReleaseAndRawFree(_PyCrossInterpreterData *);

PyAPI_FUNC(int) _PyObject_CheckCrossInterpreterData(PyObject *);

/* cross-interpreter data registry */

typedef int (*crossinterpdatafunc)(PyThreadState *tstate, PyObject *,
                                   _PyCrossInterpreterData *);

PyAPI_FUNC(int) _PyCrossInterpreterData_RegisterClass(PyTypeObject *, crossinterpdatafunc);
PyAPI_FUNC(int) _PyCrossInterpreterData_UnregisterClass(PyTypeObject *);
PyAPI_FUNC(crossinterpdatafunc) _PyCrossInterpreterData_Lookup(PyObject *);
