
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
}

/*************************/
/* convenience utilities */
/*************************/

static const char *
_copy_string_obj_raw(PyObject *strobj)
{
    const char *str = PyUnicode_AsUTF8(strobj);
    if (str == NULL) {
        return NULL;
    }

    char *copied = PyMem_RawMalloc(strlen(str)+1);
    if (copied == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    strcpy(copied, str);
    return copied;
}

static int
_release_xid_data(_PyCrossInterpreterData *data, int rawfree)
{
    PyObject *exc = PyErr_GetRaisedException();
    int res = rawfree
        ? _PyCrossInterpreterData_Release(data)
        : _PyCrossInterpreterData_ReleaseAndRawFree(data);
    if (res < 0) {
        /* The owning interpreter is already destroyed. */
        _PyCrossInterpreterData_Clear(NULL, data);
        // XXX Emit a warning?
        PyErr_Clear();
    }
    PyErr_SetRaisedException(exc);
    return res;
}


/***************************/
/* short-term data sharing */
/***************************/

/* error codes */

int
_PyXI_ApplyErrorCode(_PyXI_errcode code, PyInterpreterState *interp)
{
    assert(!PyErr_Occurred());
    switch (code) {
    case _PyXI_ERR_NO_ERROR:  // fall through
    case _PyXI_ERR_UNCAUGHT_EXCEPTION:
        // There is nothing to apply.
#ifdef Py_DEBUG
        Py_UNREACHABLE();
#endif
        return 0;
    case _PyXI_ERR_OTHER:
        // XXX msg?
        PyErr_SetNone(PyExc_RuntimeError);
        break;
    case _PyXI_ERR_NO_MEMORY:
        PyErr_NoMemory();
        break;
    case _PyXI_ERR_ALREADY_RUNNING:
        assert(interp != NULL);
        assert(_PyInterpreterState_IsRunningMain(interp));
        _PyInterpreterState_FailIfRunningMain(interp);
        break;
    default:
#ifdef Py_DEBUG
        Py_UNREACHABLE();
#else
        PyErr_Format(PyExc_RuntimeError, "unsupported error code %d", code);
#endif
    }
    assert(PyErr_Occurred());
    return -1;
}

/* shared exceptions */

const char *
_PyXI_InitExceptionInfo(_PyXI_exception_info *info,
                        PyObject *excobj, _PyXI_errcode code)
{
    if (info->interp == NULL) {
        info->interp = PyInterpreterState_Get();
    }

    const char *failure = NULL;
    if (code == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // There is an unhandled exception we need to propagate.
        failure = _Py_excinfo_InitFromException(&info->uncaught, excobj);
        if (failure != NULL) {
            // We failed to initialize info->uncaught.
            // XXX Print the excobj/traceback?  Emit a warning?
            // XXX Print the current exception/traceback?
            if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
                info->code = _PyXI_ERR_NO_MEMORY;
            }
            else {
                info->code = _PyXI_ERR_OTHER;
            }
            PyErr_Clear();
        }
        else {
            info->code = code;
        }
        assert(info->code != _PyXI_ERR_NO_ERROR);
    }
    else {
        // There is an error code we need to propagate.
        assert(excobj == NULL);
        assert(code != _PyXI_ERR_NO_ERROR);
        info->code = code;
        _Py_excinfo_Clear(&info->uncaught);
    }
    return failure;
}

void
_PyXI_ApplyExceptionInfo(_PyXI_exception_info *info, PyObject *exctype)
{
    if (info->code == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // Raise an exception that proxies the propagated exception.
        _Py_excinfo_Apply(&info->uncaught, exctype);
    }
    else {
        // Raise an exception corresponding to the code.
        assert(info->code != _PyXI_ERR_NO_ERROR);
        (void)_PyXI_ApplyErrorCode(info->code, info->interp);
    }
    assert(PyErr_Occurred());
}

/* shared namespaces */

typedef struct _sharednsitem {
    int64_t interpid;
    const char *name;
    _PyCrossInterpreterData *data;
    _PyCrossInterpreterData _data;
} _PyXI_namespace_item;

static void _sharednsitem_clear(_PyXI_namespace_item *);  // forward

static int
_sharednsitem_init(_PyXI_namespace_item *item, int64_t interpid, PyObject *key)
{
    assert(interpid >= 0);
    item->interpid = interpid;
    item->name = _copy_string_obj_raw(key);
    if (item->name == NULL) {
        return -1;
    }
    item->data = NULL;
    return 0;
}

static int
_sharednsitem_set_value(_PyXI_namespace_item *item, PyObject *value)
{
    assert(item->name != NULL);
    assert(item->data == NULL);
    item->data = &item->_data;
    if (item->interpid == PyInterpreterState_GetID(PyInterpreterState_Get())) {
        item->data = &item->_data;
    }
    else {
        item->data = PyMem_RawMalloc(sizeof(_PyCrossInterpreterData));
        if (item->data == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }
    if (_PyObject_GetCrossInterpreterData(value, item->data) != 0) {
        if (item->data != &item->_data) {
            PyMem_RawFree(item->data);
        }
        item->data = NULL;
        return -1;
    }
    return 0;
}

static void
_sharednsitem_clear(_PyXI_namespace_item *item)
{
    if (item->name != NULL) {
        PyMem_RawFree((void *)item->name);
        item->name = NULL;
    }
    _PyCrossInterpreterData *data = item->data;
    if (data != NULL) {
        item->data = NULL;
        int rawfree = (data == &item->_data);
        (void)_release_xid_data(data, rawfree);
    }
}

static int
_sharednsitem_apply(_PyXI_namespace_item *item, PyObject *ns, PyObject *dflt)
{
    PyObject *name = PyUnicode_FromString(item->name);
    if (name == NULL) {
        return -1;
    }
    PyObject *value;
    if (item->data != NULL) {
        value = _PyCrossInterpreterData_NewObject(item->data);
        if (value == NULL) {
            Py_DECREF(name);
            return -1;
        }
    }
    else {
        value = Py_NewRef(dflt);
    }
    int res = PyDict_SetItem(ns, name, value);
    Py_DECREF(name);
    Py_DECREF(value);
    return res;
}

struct _sharedns {
    PyInterpreterState *interp;
    Py_ssize_t len;
    _PyXI_namespace_item *items;
};

static _PyXI_namespace *
_sharedns_new(Py_ssize_t len)
{
    _PyXI_namespace *shared = PyMem_RawCalloc(sizeof(_PyXI_namespace), 1);
    if (shared == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    shared->len = len;
    shared->items = PyMem_RawCalloc(sizeof(struct _sharednsitem), len);
    if (shared->items == NULL) {
        PyErr_NoMemory();
        PyMem_RawFree(shared);
        return NULL;
    }
    return shared;
}

static void
_free_xi_namespace(_PyXI_namespace *ns)
{
    for (Py_ssize_t i=0; i < ns->len; i++) {
        _sharednsitem_clear(&ns->items[i]);
    }
    PyMem_RawFree(ns->items);
    PyMem_RawFree(ns);
}

static int
_pending_free_xi_namespace(void *arg)
{
    _PyXI_namespace *ns = (_PyXI_namespace *)arg;
    _free_xi_namespace(ns);
    return 0;
}

void
_PyXI_FreeNamespace(_PyXI_namespace *ns)
{
    if (ns->len == 0) {
        return;
    }
    PyInterpreterState *interp = ns->interp;
    if (interp == NULL) {
        assert(ns->items[0].name == NULL);
        // No data was actually set, so we can free the items
        // without clearing each item's XI data.
        PyMem_RawFree(ns->items);
        PyMem_RawFree(ns);
    }
    else {
        // We can assume the first item represents all items.
        assert(ns->items[0].data->interpid == interp->id);
        if (interp == PyInterpreterState_Get()) {
            // We can avoid pending calls.
            _free_xi_namespace(ns);
        }
        else {
            // We have to use a pending call due to data in another interpreter.
            // XXX Make sure the pending call was added?
            _PyEval_AddPendingCall(interp, _pending_free_xi_namespace, ns, 0);
        }
    }
}

_PyXI_namespace *
_PyXI_NamespaceFromNames(PyObject *names)
{
    if (names == NULL || names == Py_None) {
        return NULL;
    }

    Py_ssize_t len = PySequence_Size(names);
    if (len <= 0) {
        return NULL;
    }

    _PyXI_namespace *ns = _sharedns_new(len);
    if (ns == NULL) {
        return NULL;
    }
    int64_t interpid = PyInterpreterState_Get()->id;
    for (Py_ssize_t i=0; i < len; i++) {
        PyObject *key = PySequence_GetItem(names, i);
        if (key == NULL) {
            break;
        }
        struct _sharednsitem *item = &ns->items[i];
        int res = _sharednsitem_init(item, interpid, key);
        Py_DECREF(key);
        if (res < 0) {
            break;
        }
    }
    if (PyErr_Occurred()) {
        _PyXI_FreeNamespace(ns);
        return NULL;
    }
    return ns;
}

_PyXI_namespace *
_PyXI_NamespaceFromDict(PyObject *nsobj)
{
    if (nsobj == NULL || nsobj == Py_None) {
        return NULL;
    }
    if (!PyDict_CheckExact(nsobj)) {
        PyErr_SetString(PyExc_TypeError, "expected a dict");
        return NULL;
    }

    Py_ssize_t len = PyDict_Size(nsobj);
    if (len == 0) {
        return NULL;
    }

    _PyXI_namespace *ns = _sharedns_new(len);
    if (ns == NULL) {
        return NULL;
    }
    ns->interp = PyInterpreterState_Get();
    int64_t interpid = ns->interp->id;

    Py_ssize_t pos = 0;
    for (Py_ssize_t i=0; i < len; i++) {
        PyObject *key, *value;
        if (!PyDict_Next(nsobj, &pos, &key, &value)) {
            break;
        }
        _PyXI_namespace_item *item = &ns->items[i];
        if (_sharednsitem_init(item, interpid, key) != 0) {
            break;
        }
        if (_sharednsitem_set_value(item, value) < 0) {
            _sharednsitem_clear(item);
            break;
        }
    }
    if (PyErr_Occurred()) {
        _PyXI_FreeNamespace(ns);
        return NULL;
    }
    return ns;
}

int
_PyXI_ApplyNamespace(_PyXI_namespace *ns, PyObject *nsobj, PyObject *dflt)
{
    for (Py_ssize_t i=0; i < ns->len; i++) {
        if (_sharednsitem_apply(&ns->items[i], nsobj, dflt) != 0) {
            return -1;
        }
    }
    return 0;
}


/**********************/
/* high-level helpers */
/**********************/

/* enter/exit a cross-interpreter session */

static void
_enter_session(_PyXI_session *session, PyInterpreterState *interp)
{
    // Set here and cleared in _exit_session().
    assert(!session->own_init_tstate);
    assert(session->init_tstate == NULL);
    assert(session->prev_tstate == NULL);
    // Set elsewhere and cleared in _exit_session().
    assert(!session->running);
    assert(session->main_ns == NULL);
    // Set elsewhere and cleared in _capture_current_exception().
    assert(session->exc_override == NULL);
    // Set elsewhere and cleared in _PyXI_ApplyCapturedException().
    assert(session->exc == NULL);

    // Switch to interpreter.
    PyThreadState *tstate = PyThreadState_Get();
    PyThreadState *prev = tstate;
    if (interp != tstate->interp) {
        tstate = PyThreadState_New(interp);
        tstate->_whence = _PyThreadState_WHENCE_EXEC;
        // XXX Possible GILState issues?
        session->prev_tstate = PyThreadState_Swap(tstate);
        assert(session->prev_tstate == prev);
        session->own_init_tstate = 1;
    }
    session->init_tstate = tstate;
    session->prev_tstate = prev;
}

static void
_exit_session(_PyXI_session *session)
{
    PyThreadState *tstate = session->init_tstate;
    assert(tstate != NULL);
    assert(PyThreadState_Get() == tstate);

    // Release any of the entered interpreters resources.
    if (session->main_ns != NULL) {
        Py_CLEAR(session->main_ns);
    }

    // Ensure this thread no longer owns __main__.
    if (session->running) {
        _PyInterpreterState_SetNotRunningMain(tstate->interp);
        assert(!PyErr_Occurred());
        session->running = 0;
    }

    // Switch back.
    assert(session->prev_tstate != NULL);
    if (session->prev_tstate != session->init_tstate) {
        assert(session->own_init_tstate);
        session->own_init_tstate = 0;
        PyThreadState_Clear(tstate);
        PyThreadState_Swap(session->prev_tstate);
        PyThreadState_Delete(tstate);
    }
    else {
        assert(!session->own_init_tstate);
    }
    session->prev_tstate = NULL;
    session->init_tstate = NULL;
}

static void
_capture_current_exception(_PyXI_session *session)
{
    assert(session->exc == NULL);
    if (!PyErr_Occurred()) {
        assert(session->exc_override == NULL);
        return;
    }

    // Handle the exception override.
    _PyXI_errcode errcode = session->exc_override != NULL
        ? *session->exc_override
        : _PyXI_ERR_UNCAUGHT_EXCEPTION;
    session->exc_override = NULL;

    // Pop the exception object.
    PyObject *excval = NULL;
    if (errcode == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // We want to actually capture the current exception.
        excval = PyErr_GetRaisedException();
    }
    else {
        // We could do a variety of things here, depending on errcode.
        // However, for now we simply ignore the exception and rely
        // strictly on errcode.
        PyErr_Clear();
    }

    // Capture the exception.
    _PyXI_exception_info *exc = &session->_exc;
    *exc = (_PyXI_exception_info){
        .interp = session->init_tstate->interp,
    };
    const char *failure = _PyXI_InitExceptionInfo(exc, excval, errcode);

    // Handle capture failure.
    if (failure != NULL) {
        // XXX Make this error message more generic.
        fprintf(stderr,
                "RunFailedError: script raised an uncaught exception (%s)",
                failure);
        exc = NULL;
    }

    // a temporary hack  (famous last words)
    if (excval != NULL) {
        // XXX Store the traceback info (or rendered traceback) on
        // _PyXI_excinfo, attach it to the exception when applied,
        // and teach PyErr_Display() to print it.
#ifdef Py_DEBUG
        // XXX Drop this once _Py_excinfo picks up the slack.
        PyErr_Display(NULL, excval, NULL);
#endif
        Py_DECREF(excval);
    }

    // Finished!
    assert(!PyErr_Occurred());
    session->exc = exc;
}

void
_PyXI_ApplyCapturedException(_PyXI_session *session, PyObject *excwrapper)
{
    assert(!PyErr_Occurred());
    assert(session->exc != NULL);
    _PyXI_ApplyExceptionInfo(session->exc, excwrapper);
    assert(PyErr_Occurred());
    session->exc = NULL;
}

int
_PyXI_HasCapturedException(_PyXI_session *session)
{
    return session->exc != NULL;
}

int
_PyXI_Enter(PyInterpreterState *interp, PyObject *nsupdates,
            _PyXI_session *session)
{
    // Convert the attrs for cross-interpreter use.
    _PyXI_namespace *sharedns = NULL;
    if (nsupdates != NULL) {
        sharedns = _PyXI_NamespaceFromDict(nsupdates);
        if (sharedns == NULL && PyErr_Occurred()) {
            assert(session->exc == NULL);
            return -1;
        }
    }

    // Switch to the requested interpreter (if necessary).
    _enter_session(session, interp);
    _PyXI_errcode errcode = _PyXI_ERR_UNCAUGHT_EXCEPTION;

    // Ensure this thread owns __main__.
    if (_PyInterpreterState_SetRunningMain(interp) < 0) {
        // In the case where we didn't switch interpreters, it would
        // be more efficient to leave the exception in place and return
        // immediately.  However, life is simpler if we don't.
        errcode = _PyXI_ERR_ALREADY_RUNNING;
        goto error;
    }
    session->running = 1;

    // Cache __main__.__dict__.
    PyObject *main_mod = PyUnstable_InterpreterState_GetMainModule(interp);
    if (main_mod == NULL) {
        goto error;
    }
    PyObject *ns = PyModule_GetDict(main_mod);  // borrowed
    Py_DECREF(main_mod);
    if (ns == NULL) {
        goto error;
    }
    session->main_ns = Py_NewRef(ns);

    // Apply the cross-interpreter data.
    if (sharedns != NULL) {
        if (_PyXI_ApplyNamespace(sharedns, ns, NULL) < 0) {
            goto error;
        }
        _PyXI_FreeNamespace(sharedns);
    }

    errcode = _PyXI_ERR_NO_ERROR;
    return 0;

error:
    assert(PyErr_Occurred());
    if (errcode != _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        session->exc_override = &errcode;
    }
    _capture_current_exception(session);
    _exit_session(session);
    if (sharedns != NULL) {
        _PyXI_FreeNamespace(sharedns);
    }
    return -1;
}

void
_PyXI_Exit(_PyXI_session *session)
{
    _capture_current_exception(session);
    _exit_session(session);
}
