
/* API for managing interactions between isolated interpreters */

#include "Python.h"
#include "pycore_ceval.h"         // _Py_simple_func
#include "pycore_crossinterp.h"   // struct _xid
#include "pycore_pyerrors.h"      // _PyErr_Clear()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_weakref.h"       // _PyWeakref_GET_REF()


/***************************/
/* cross-interpreter calls */
/***************************/

int
_Py_CallInInterpreter(PyInterpreterState *interp,
                      _Py_simple_func func, void *arg)
{
    if (interp == _PyThreadState_GetCurrent()->interp) {
        return func(arg);
    }
    // XXX Emit a warning if this fails?
    _PyEval_AddPendingCall(interp, (_Py_pending_call_func)func, arg, 0);
    return 0;
}

int
_Py_CallInInterpreterAndRawFree(PyInterpreterState *interp,
                                _Py_simple_func func, void *arg)
{
    if (interp == _PyThreadState_GetCurrent()->interp) {
        int res = func(arg);
        PyMem_RawFree(arg);
        return res;
    }
    // XXX Emit a warning if this fails?
    _PyEval_AddPendingCall(interp, func, arg, _Py_PENDING_RAWFREE);
    return 0;
}


/**************************/
/* cross-interpreter data */
/**************************/

_PyCrossInterpreterData *
_PyCrossInterpreterData_New(void)
{
    _PyCrossInterpreterData *xid = PyMem_RawMalloc(
                                            sizeof(_PyCrossInterpreterData));
    if (xid == NULL) {
        PyErr_NoMemory();
    }
    return xid;
}

void
_PyCrossInterpreterData_Free(_PyCrossInterpreterData *xid)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    _PyCrossInterpreterData_Clear(interp, xid);
    PyMem_RawFree(xid);
}


/* defining cross-interpreter data */

static inline void
_xidata_init(_PyCrossInterpreterData *data)
{
    // If the value is being reused
    // then _xidata_clear() should have been called already.
    assert(data->data == NULL);
    assert(data->obj == NULL);
    *data = (_PyCrossInterpreterData){0};
    data->interpid = -1;
}

static inline void
_xidata_clear(_PyCrossInterpreterData *data)
{
    // _PyCrossInterpreterData only has two members that need to be
    // cleaned up, if set: "data" must be freed and "obj" must be decref'ed.
    // In both cases the original (owning) interpreter must be used,
    // which is the caller's responsibility to ensure.
    if (data->data != NULL) {
        if (data->free != NULL) {
            data->free(data->data);
        }
        data->data = NULL;
    }
    Py_CLEAR(data->obj);
}

void
_PyCrossInterpreterData_Init(_PyCrossInterpreterData *data,
                             PyInterpreterState *interp,
                             void *shared, PyObject *obj,
                             xid_newobjectfunc new_object)
{
    assert(data != NULL);
    assert(new_object != NULL);
    _xidata_init(data);
    data->data = shared;
    if (obj != NULL) {
        assert(interp != NULL);
        // released in _PyCrossInterpreterData_Clear()
        data->obj = Py_NewRef(obj);
    }
    // Ideally every object would know its owning interpreter.
    // Until then, we have to rely on the caller to identify it
    // (but we don't need it in all cases).
    data->interpid = (interp != NULL) ? interp->id : -1;
    data->new_object = new_object;
}

int
_PyCrossInterpreterData_InitWithSize(_PyCrossInterpreterData *data,
                                     PyInterpreterState *interp,
                                     const size_t size, PyObject *obj,
                                     xid_newobjectfunc new_object)
{
    assert(size > 0);
    // For now we always free the shared data in the same interpreter
    // where it was allocated, so the interpreter is required.
    assert(interp != NULL);
    _PyCrossInterpreterData_Init(data, interp, NULL, obj, new_object);
    data->data = PyMem_RawMalloc(size);
    if (data->data == NULL) {
        return -1;
    }
    data->free = PyMem_RawFree;
    return 0;
}

void
_PyCrossInterpreterData_Clear(PyInterpreterState *interp,
                              _PyCrossInterpreterData *data)
{
    assert(data != NULL);
    // This must be called in the owning interpreter.
    assert(interp == NULL
           || data->interpid == -1
           || data->interpid == interp->id);
    _xidata_clear(data);
}


/* using cross-interpreter data */

static int
_check_xidata(PyThreadState *tstate, _PyCrossInterpreterData *data)
{
    // data->data can be anything, including NULL, so we don't check it.

    // data->obj may be NULL, so we don't check it.

    if (data->interpid < 0) {
        _PyErr_SetString(tstate, PyExc_SystemError, "missing interp");
        return -1;
    }

    if (data->new_object == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "missing new_object func");
        return -1;
    }

    // data->free may be NULL, so we don't check it.

    return 0;
}

crossinterpdatafunc _PyCrossInterpreterData_Lookup(PyObject *);

/* This is a separate func from _PyCrossInterpreterData_Lookup in order
   to keep the registry code separate. */
static crossinterpdatafunc
_lookup_getdata(PyObject *obj)
{
    crossinterpdatafunc getdata = _PyCrossInterpreterData_Lookup(obj);
    if (getdata == NULL && PyErr_Occurred() == 0)
        PyErr_Format(PyExc_ValueError,
                     "%S does not support cross-interpreter data", obj);
    return getdata;
}

int
_PyObject_CheckCrossInterpreterData(PyObject *obj)
{
    crossinterpdatafunc getdata = _lookup_getdata(obj);
    if (getdata == NULL) {
        return -1;
    }
    return 0;
}

int
_PyObject_GetCrossInterpreterData(PyObject *obj, _PyCrossInterpreterData *data)
{
    PyThreadState *tstate = _PyThreadState_GetCurrent();
#ifdef Py_DEBUG
    // The caller must hold the GIL
    _Py_EnsureTstateNotNULL(tstate);
#endif
    PyInterpreterState *interp = tstate->interp;

    // Reset data before re-populating.
    *data = (_PyCrossInterpreterData){0};
    data->interpid = -1;

    // Call the "getdata" func for the object.
    Py_INCREF(obj);
    crossinterpdatafunc getdata = _lookup_getdata(obj);
    if (getdata == NULL) {
        Py_DECREF(obj);
        return -1;
    }
    int res = getdata(tstate, obj, data);
    Py_DECREF(obj);
    if (res != 0) {
        return -1;
    }

    // Fill in the blanks and validate the result.
    data->interpid = interp->id;
    if (_check_xidata(tstate, data) != 0) {
        (void)_PyCrossInterpreterData_Release(data);
        return -1;
    }

    return 0;
}

PyObject *
_PyCrossInterpreterData_NewObject(_PyCrossInterpreterData *data)
{
    return data->new_object(data);
}

static int
_call_clear_xidata(void *data)
{
    _xidata_clear((_PyCrossInterpreterData *)data);
    return 0;
}

static int
_xidata_release(_PyCrossInterpreterData *data, int rawfree)
{
    if ((data->data == NULL || data->free == NULL) && data->obj == NULL) {
        // Nothing to release!
        if (rawfree) {
            PyMem_RawFree(data);
        }
        else {
            data->data = NULL;
        }
        return 0;
    }

    // Switch to the original interpreter.
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(data->interpid);
    if (interp == NULL) {
        // The interpreter was already destroyed.
        // This function shouldn't have been called.
        // XXX Someone leaked some memory...
        assert(PyErr_Occurred());
        if (rawfree) {
            PyMem_RawFree(data);
        }
        return -1;
    }

    // "Release" the data and/or the object.
    if (rawfree) {
        return _Py_CallInInterpreterAndRawFree(interp, _call_clear_xidata, data);
    }
    else {
        return _Py_CallInInterpreter(interp, _call_clear_xidata, data);
    }
}

int
_PyCrossInterpreterData_Release(_PyCrossInterpreterData *data)
{
    return _xidata_release(data, 0);
}

int
_PyCrossInterpreterData_ReleaseAndRawFree(_PyCrossInterpreterData *data)
{
    return _xidata_release(data, 1);
}


/* registry of {type -> crossinterpdatafunc} */

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   crossinterpdatafunc. It would be simpler and more efficient. */

static int
_xidregistry_add_type(struct _xidregistry *xidregistry,
                      PyTypeObject *cls, crossinterpdatafunc getdata)
{
    struct _xidregitem *newhead = PyMem_RawMalloc(sizeof(struct _xidregitem));
    if (newhead == NULL) {
        return -1;
    }
    *newhead = (struct _xidregitem){
        // We do not keep a reference, to avoid keeping the class alive.
        .cls = cls,
        .refcount = 1,
        .getdata = getdata,
    };
    if (cls->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        // XXX Assign a callback to clear the entry from the registry?
        newhead->weakref = PyWeakref_NewRef((PyObject *)cls, NULL);
        if (newhead->weakref == NULL) {
            PyMem_RawFree(newhead);
            return -1;
        }
    }
    newhead->next = xidregistry->head;
    if (newhead->next != NULL) {
        newhead->next->prev = newhead;
    }
    xidregistry->head = newhead;
    return 0;
}

static struct _xidregitem *
_xidregistry_remove_entry(struct _xidregistry *xidregistry,
                          struct _xidregitem *entry)
{
    struct _xidregitem *next = entry->next;
    if (entry->prev != NULL) {
        assert(entry->prev->next == entry);
        entry->prev->next = next;
    }
    else {
        assert(xidregistry->head == entry);
        xidregistry->head = next;
    }
    if (next != NULL) {
        next->prev = entry->prev;
    }
    Py_XDECREF(entry->weakref);
    PyMem_RawFree(entry);
    return next;
}

// This is used in pystate.c (for now).
void
_Py_xidregistry_clear(struct _xidregistry *xidregistry)
{
    struct _xidregitem *cur = xidregistry->head;
    xidregistry->head = NULL;
    while (cur != NULL) {
        struct _xidregitem *next = cur->next;
        Py_XDECREF(cur->weakref);
        PyMem_RawFree(cur);
        cur = next;
    }
}

static struct _xidregitem *
_xidregistry_find_type(struct _xidregistry *xidregistry, PyTypeObject *cls)
{
    struct _xidregitem *cur = xidregistry->head;
    while (cur != NULL) {
        if (cur->weakref != NULL) {
            // cur is/was a heap type.
            PyObject *registered = _PyWeakref_GET_REF(cur->weakref);
            if (registered == NULL) {
                // The weakly ref'ed object was freed.
                cur = _xidregistry_remove_entry(xidregistry, cur);
                continue;
            }
            assert(PyType_Check(registered));
            assert(cur->cls == (PyTypeObject *)registered);
            assert(cur->cls->tp_flags & Py_TPFLAGS_HEAPTYPE);
            Py_DECREF(registered);
        }
        if (cur->cls == cls) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static inline struct _xidregistry *
_get_xidregistry(PyInterpreterState *interp, PyTypeObject *cls)
{
    struct _xidregistry *xidregistry = &interp->runtime->xidregistry;
    if (cls->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        assert(interp->xidregistry.mutex == xidregistry->mutex);
        xidregistry = &interp->xidregistry;
    }
    return xidregistry;
}

static void _register_builtins_for_crossinterpreter_data(struct _xidregistry *xidregistry);

static inline void
_ensure_builtins_xid(PyInterpreterState *interp, struct _xidregistry *xidregistry)
{
    if (xidregistry != &interp->xidregistry) {
        assert(xidregistry == &interp->runtime->xidregistry);
        if (xidregistry->head == NULL) {
            _register_builtins_for_crossinterpreter_data(xidregistry);
        }
    }
}

int
_PyCrossInterpreterData_RegisterClass(PyTypeObject *cls,
                                      crossinterpdatafunc getdata)
{
    if (!PyType_Check(cls)) {
        PyErr_Format(PyExc_ValueError, "only classes may be registered");
        return -1;
    }
    if (getdata == NULL) {
        PyErr_Format(PyExc_ValueError, "missing 'getdata' func");
        return -1;
    }

    int res = 0;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _xidregistry *xidregistry = _get_xidregistry(interp, cls);
    PyThread_acquire_lock(xidregistry->mutex, WAIT_LOCK);

    _ensure_builtins_xid(interp, xidregistry);

    struct _xidregitem *matched = _xidregistry_find_type(xidregistry, cls);
    if (matched != NULL) {
        assert(matched->getdata == getdata);
        matched->refcount += 1;
        goto finally;
    }

    res = _xidregistry_add_type(xidregistry, cls, getdata);

finally:
    PyThread_release_lock(xidregistry->mutex);
    return res;
}

int
_PyCrossInterpreterData_UnregisterClass(PyTypeObject *cls)
{
    int res = 0;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _xidregistry *xidregistry = _get_xidregistry(interp, cls);
    PyThread_acquire_lock(xidregistry->mutex, WAIT_LOCK);

    struct _xidregitem *matched = _xidregistry_find_type(xidregistry, cls);
    if (matched != NULL) {
        assert(matched->refcount > 0);
        matched->refcount -= 1;
        if (matched->refcount == 0) {
            (void)_xidregistry_remove_entry(xidregistry, matched);
        }
        res = 1;
    }

    PyThread_release_lock(xidregistry->mutex);
    return res;
}


/* Cross-interpreter objects are looked up by exact match on the class.
   We can reassess this policy when we move from a global registry to a
   tp_* slot. */

crossinterpdatafunc
_PyCrossInterpreterData_Lookup(PyObject *obj)
{
    PyTypeObject *cls = Py_TYPE(obj);

    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _xidregistry *xidregistry = _get_xidregistry(interp, cls);
    PyThread_acquire_lock(xidregistry->mutex, WAIT_LOCK);

    _ensure_builtins_xid(interp, xidregistry);

    struct _xidregitem *matched = _xidregistry_find_type(xidregistry, cls);
    crossinterpdatafunc func = matched != NULL ? matched->getdata : NULL;

    PyThread_release_lock(xidregistry->mutex);
    return func;
}

/* cross-interpreter data for builtin types */

struct _shared_bytes_data {
    char *bytes;
    Py_ssize_t len;
};

static PyObject *
_new_bytes_object(_PyCrossInterpreterData *data)
{
    struct _shared_bytes_data *shared = (struct _shared_bytes_data *)(data->data);
    return PyBytes_FromStringAndSize(shared->bytes, shared->len);
}

static int
_bytes_shared(PyThreadState *tstate, PyObject *obj,
              _PyCrossInterpreterData *data)
{
    if (_PyCrossInterpreterData_InitWithSize(
            data, tstate->interp, sizeof(struct _shared_bytes_data), obj,
            _new_bytes_object
            ) < 0)
    {
        return -1;
    }
    struct _shared_bytes_data *shared = (struct _shared_bytes_data *)data->data;
    if (PyBytes_AsStringAndSize(obj, &shared->bytes, &shared->len) < 0) {
        _PyCrossInterpreterData_Clear(tstate->interp, data);
        return -1;
    }
    return 0;
}

struct _shared_str_data {
    int kind;
    const void *buffer;
    Py_ssize_t len;
};

static PyObject *
_new_str_object(_PyCrossInterpreterData *data)
{
    struct _shared_str_data *shared = (struct _shared_str_data *)(data->data);
    return PyUnicode_FromKindAndData(shared->kind, shared->buffer, shared->len);
}

static int
_str_shared(PyThreadState *tstate, PyObject *obj,
            _PyCrossInterpreterData *data)
{
    if (_PyCrossInterpreterData_InitWithSize(
            data, tstate->interp, sizeof(struct _shared_str_data), obj,
            _new_str_object
            ) < 0)
    {
        return -1;
    }
    struct _shared_str_data *shared = (struct _shared_str_data *)data->data;
    shared->kind = PyUnicode_KIND(obj);
    shared->buffer = PyUnicode_DATA(obj);
    shared->len = PyUnicode_GET_LENGTH(obj);
    return 0;
}

static PyObject *
_new_long_object(_PyCrossInterpreterData *data)
{
    return PyLong_FromSsize_t((Py_ssize_t)(data->data));
}

static int
_long_shared(PyThreadState *tstate, PyObject *obj,
             _PyCrossInterpreterData *data)
{
    /* Note that this means the size of shareable ints is bounded by
     * sys.maxsize.  Hence on 32-bit architectures that is half the
     * size of maximum shareable ints on 64-bit.
     */
    Py_ssize_t value = PyLong_AsSsize_t(obj);
    if (value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            PyErr_SetString(PyExc_OverflowError, "try sending as bytes");
        }
        return -1;
    }
    _PyCrossInterpreterData_Init(data, tstate->interp, (void *)value, NULL,
            _new_long_object);
    // data->obj and data->free remain NULL
    return 0;
}

static PyObject *
_new_float_object(_PyCrossInterpreterData *data)
{
    double * value_ptr = data->data;
    return PyFloat_FromDouble(*value_ptr);
}

static int
_float_shared(PyThreadState *tstate, PyObject *obj,
             _PyCrossInterpreterData *data)
{
    if (_PyCrossInterpreterData_InitWithSize(
            data, tstate->interp, sizeof(double), NULL,
            _new_float_object
            ) < 0)
    {
        return -1;
    }
    double *shared = (double *)data->data;
    *shared = PyFloat_AsDouble(obj);
    return 0;
}

static PyObject *
_new_none_object(_PyCrossInterpreterData *data)
{
    // XXX Singleton refcounts are problematic across interpreters...
    return Py_NewRef(Py_None);
}

static int
_none_shared(PyThreadState *tstate, PyObject *obj,
             _PyCrossInterpreterData *data)
{
    _PyCrossInterpreterData_Init(data, tstate->interp, NULL, NULL,
            _new_none_object);
    // data->data, data->obj and data->free remain NULL
    return 0;
}

static void
_register_builtins_for_crossinterpreter_data(struct _xidregistry *xidregistry)
{
    // None
    if (_xidregistry_add_type(xidregistry, (PyTypeObject *)PyObject_Type(Py_None), _none_shared) != 0) {
        Py_FatalError("could not register None for cross-interpreter sharing");
    }

    // int
    if (_xidregistry_add_type(xidregistry, &PyLong_Type, _long_shared) != 0) {
        Py_FatalError("could not register int for cross-interpreter sharing");
    }

    // bytes
    if (_xidregistry_add_type(xidregistry, &PyBytes_Type, _bytes_shared) != 0) {
        Py_FatalError("could not register bytes for cross-interpreter sharing");
    }

    // str
    if (_xidregistry_add_type(xidregistry, &PyUnicode_Type, _str_shared) != 0) {
        Py_FatalError("could not register str for cross-interpreter sharing");
    }

    // float
    if (_xidregistry_add_type(xidregistry, &PyFloat_Type, _float_shared) != 0) {
        Py_FatalError("could not register float for cross-interpreter sharing");
    }
}
