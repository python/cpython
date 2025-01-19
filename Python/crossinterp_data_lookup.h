#include "pycore_weakref.h"       // _PyWeakref_GET_REF()


typedef _PyXIData_lookup_context_t dlcontext_t;
typedef _PyXIData_registry_t dlregistry_t;
typedef _PyXIData_regitem_t dlregitem_t;


// forward
static void _xidregistry_init(dlregistry_t *);
static void _xidregistry_fini(dlregistry_t *);
static xidatafunc _lookup_getdata_from_registry(dlcontext_t *, PyObject *);


/* used in crossinterp.c */

static void
xid_lookup_init(_PyXIData_lookup_t *state)
{
    _xidregistry_init(&state->registry);
}

static void
xid_lookup_fini(_PyXIData_lookup_t *state)
{
    _xidregistry_fini(&state->registry);
}

static xidatafunc
lookup_getdata(dlcontext_t *ctx, PyObject *obj)
{
   /* Cross-interpreter objects are looked up by exact match on the class.
      We can reassess this policy when we move from a global registry to a
      tp_* slot. */
    return _lookup_getdata_from_registry(ctx, obj);
}


/* exported API */

int
_PyXIData_GetLookupContext(PyInterpreterState *interp,
                           _PyXIData_lookup_context_t *res)
{
    _PyXI_global_state_t *global = _PyXI_GET_GLOBAL_STATE(interp);
    if (global == NULL) {
        assert(PyErr_Occurred());
        return -1;
    }
    _PyXI_state_t *local = _PyXI_GET_STATE(interp);
    if (local == NULL) {
        assert(PyErr_Occurred());
        return -1;
    }
    *res = (dlcontext_t){
        .global = &global->data_lookup,
        .local = &local->data_lookup,
        .PyExc_NotShareableError = local->exceptions.PyExc_NotShareableError,
    };
    return 0;
}

xidatafunc
_PyXIData_Lookup(_PyXIData_lookup_context_t *ctx, PyObject *obj)
{
    return lookup_getdata(ctx, obj);
}


/***********************************************/
/* a registry of {type -> xidatafunc} */
/***********************************************/

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   xidatafunc. It would be simpler and more efficient.  */


/* registry lifecycle */

static void _register_builtins_for_crossinterpreter_data(dlregistry_t *);

static void
_xidregistry_init(dlregistry_t *registry)
{
    if (registry->initialized) {
        return;
    }
    registry->initialized = 1;

    if (registry->global) {
        // Registering the builtins is cheap so we don't bother doing it lazily.
        assert(registry->head == NULL);
        _register_builtins_for_crossinterpreter_data(registry);
    }
}

static void _xidregistry_clear(dlregistry_t *);

static void
_xidregistry_fini(dlregistry_t *registry)
{
    if (!registry->initialized) {
        return;
    }
    registry->initialized = 0;

    _xidregistry_clear(registry);
}


/* registry thread safety */

static void
_xidregistry_lock(dlregistry_t *registry)
{
    if (registry->global) {
        PyMutex_Lock(&registry->mutex);
    }
    // else: Within an interpreter we rely on the GIL instead of a separate lock.
}

static void
_xidregistry_unlock(dlregistry_t *registry)
{
    if (registry->global) {
        PyMutex_Unlock(&registry->mutex);
    }
}


/* accessing the registry */

static inline dlregistry_t *
_get_xidregistry_for_type(dlcontext_t *ctx, PyTypeObject *cls)
{
    if (cls->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        return &ctx->local->registry;
    }
    return &ctx->global->registry;
}

static dlregitem_t* _xidregistry_remove_entry(dlregistry_t *, dlregitem_t *);

static dlregitem_t *
_xidregistry_find_type(dlregistry_t *xidregistry, PyTypeObject *cls)
{
    dlregitem_t *cur = xidregistry->head;
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

static xidatafunc
_lookup_getdata_from_registry(dlcontext_t *ctx, PyObject *obj)
{
    PyTypeObject *cls = Py_TYPE(obj);

    dlregistry_t *xidregistry = _get_xidregistry_for_type(ctx, cls);
    _xidregistry_lock(xidregistry);

    dlregitem_t *matched = _xidregistry_find_type(xidregistry, cls);
    xidatafunc func = matched != NULL ? matched->getdata : NULL;

    _xidregistry_unlock(xidregistry);
    return func;
}


/* updating the registry */

static int
_xidregistry_add_type(dlregistry_t *xidregistry,
                      PyTypeObject *cls, xidatafunc getdata)
{
    dlregitem_t *newhead = PyMem_RawMalloc(sizeof(dlregitem_t));
    if (newhead == NULL) {
        return -1;
    }
    *newhead = (dlregitem_t){
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

static dlregitem_t *
_xidregistry_remove_entry(dlregistry_t *xidregistry, dlregitem_t *entry)
{
    dlregitem_t *next = entry->next;
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

static void
_xidregistry_clear(dlregistry_t *xidregistry)
{
    dlregitem_t *cur = xidregistry->head;
    xidregistry->head = NULL;
    while (cur != NULL) {
        dlregitem_t *next = cur->next;
        Py_XDECREF(cur->weakref);
        PyMem_RawFree(cur);
        cur = next;
    }
}

int
_PyXIData_RegisterClass(_PyXIData_lookup_context_t *ctx,
                        PyTypeObject *cls, xidatafunc getdata)
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
    dlregistry_t *xidregistry = _get_xidregistry_for_type(ctx, cls);
    _xidregistry_lock(xidregistry);

    dlregitem_t *matched = _xidregistry_find_type(xidregistry, cls);
    if (matched != NULL) {
        assert(matched->getdata == getdata);
        matched->refcount += 1;
        goto finally;
    }

    res = _xidregistry_add_type(xidregistry, cls, getdata);

finally:
    _xidregistry_unlock(xidregistry);
    return res;
}

int
_PyXIData_UnregisterClass(_PyXIData_lookup_context_t *ctx, PyTypeObject *cls)
{
    int res = 0;
    dlregistry_t *xidregistry = _get_xidregistry_for_type(ctx, cls);
    _xidregistry_lock(xidregistry);

    dlregitem_t *matched = _xidregistry_find_type(xidregistry, cls);
    if (matched != NULL) {
        assert(matched->refcount > 0);
        matched->refcount -= 1;
        if (matched->refcount == 0) {
            (void)_xidregistry_remove_entry(xidregistry, matched);
        }
        res = 1;
    }

    _xidregistry_unlock(xidregistry);
    return res;
}


/********************************************/
/* cross-interpreter data for builtin types */
/********************************************/

// bytes

struct _shared_bytes_data {
    char *bytes;
    Py_ssize_t len;
};

static PyObject *
_new_bytes_object(_PyXIData_t *data)
{
    struct _shared_bytes_data *shared = (struct _shared_bytes_data *)(data->data);
    return PyBytes_FromStringAndSize(shared->bytes, shared->len);
}

static int
_bytes_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    if (_PyXIData_InitWithSize(
            data, tstate->interp, sizeof(struct _shared_bytes_data), obj,
            _new_bytes_object
            ) < 0)
    {
        return -1;
    }
    struct _shared_bytes_data *shared = (struct _shared_bytes_data *)data->data;
    if (PyBytes_AsStringAndSize(obj, &shared->bytes, &shared->len) < 0) {
        _PyXIData_Clear(tstate->interp, data);
        return -1;
    }
    return 0;
}

// str

struct _shared_str_data {
    int kind;
    const void *buffer;
    Py_ssize_t len;
};

static PyObject *
_new_str_object(_PyXIData_t *data)
{
    struct _shared_str_data *shared = (struct _shared_str_data *)(data->data);
    return PyUnicode_FromKindAndData(shared->kind, shared->buffer, shared->len);
}

static int
_str_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    if (_PyXIData_InitWithSize(
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

// int

static PyObject *
_new_long_object(_PyXIData_t *data)
{
    return PyLong_FromSsize_t((Py_ssize_t)(data->data));
}

static int
_long_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
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
    _PyXIData_Init(data, tstate->interp, (void *)value, NULL, _new_long_object);
    // data->obj and data->free remain NULL
    return 0;
}

// float

static PyObject *
_new_float_object(_PyXIData_t *data)
{
    double * value_ptr = data->data;
    return PyFloat_FromDouble(*value_ptr);
}

static int
_float_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    if (_PyXIData_InitWithSize(
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

// None

static PyObject *
_new_none_object(_PyXIData_t *data)
{
    // XXX Singleton refcounts are problematic across interpreters...
    return Py_NewRef(Py_None);
}

static int
_none_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    _PyXIData_Init(data, tstate->interp, NULL, NULL, _new_none_object);
    // data->data, data->obj and data->free remain NULL
    return 0;
}

// bool

static PyObject *
_new_bool_object(_PyXIData_t *data)
{
    if (data->data){
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static int
_bool_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    _PyXIData_Init(data, tstate->interp,
            (void *) (Py_IsTrue(obj) ? (uintptr_t) 1 : (uintptr_t) 0), NULL,
            _new_bool_object);
    // data->obj and data->free remain NULL
    return 0;
}

// tuple

struct _shared_tuple_data {
    Py_ssize_t len;
    _PyXIData_t **data;
};

static PyObject *
_new_tuple_object(_PyXIData_t *data)
{
    struct _shared_tuple_data *shared = (struct _shared_tuple_data *)(data->data);
    PyObject *tuple = PyTuple_New(shared->len);
    if (tuple == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < shared->len; i++) {
        PyObject *item = _PyXIData_NewObject(shared->data[i]);
        if (item == NULL){
            Py_DECREF(tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, i, item);
    }
    return tuple;
}

static void
_tuple_shared_free(void* data)
{
    struct _shared_tuple_data *shared = (struct _shared_tuple_data *)(data);
#ifndef NDEBUG
    int64_t interpid = PyInterpreterState_GetID(_PyInterpreterState_GET());
#endif
    for (Py_ssize_t i = 0; i < shared->len; i++) {
        if (shared->data[i] != NULL) {
            assert(_PyXIData_INTERPID(shared->data[i]) == interpid);
            _PyXIData_Release(shared->data[i]);
            PyMem_RawFree(shared->data[i]);
            shared->data[i] = NULL;
        }
    }
    PyMem_Free(shared->data);
    PyMem_RawFree(shared);
}

static int
_tuple_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    dlcontext_t ctx;
    if (_PyXIData_GetLookupContext(tstate->interp, &ctx) < 0) {
        return -1;
    }

    Py_ssize_t len = PyTuple_GET_SIZE(obj);
    if (len < 0) {
        return -1;
    }
    struct _shared_tuple_data *shared = PyMem_RawMalloc(sizeof(struct _shared_tuple_data));
    if (shared == NULL){
        PyErr_NoMemory();
        return -1;
    }

    shared->len = len;
    shared->data = (_PyXIData_t **) PyMem_Calloc(shared->len, sizeof(_PyXIData_t *));
    if (shared->data == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    for (Py_ssize_t i = 0; i < shared->len; i++) {
        _PyXIData_t *data = _PyXIData_New();
        if (data == NULL) {
            goto error;  // PyErr_NoMemory already set
        }
        PyObject *item = PyTuple_GET_ITEM(obj, i);

        int res = -1;
        if (!_Py_EnterRecursiveCallTstate(tstate, " while sharing a tuple")) {
            res = _PyObject_GetXIData(&ctx, item, data);
            _Py_LeaveRecursiveCallTstate(tstate);
        }
        if (res < 0) {
            PyMem_RawFree(data);
            goto error;
        }
        shared->data[i] = data;
    }
    _PyXIData_Init(data, tstate->interp, shared, obj, _new_tuple_object);
    data->free = _tuple_shared_free;
    return 0;

error:
    _tuple_shared_free(shared);
    return -1;
}

// registration

static void
_register_builtins_for_crossinterpreter_data(dlregistry_t *xidregistry)
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

    // bool
    if (_xidregistry_add_type(xidregistry, &PyBool_Type, _bool_shared) != 0) {
        Py_FatalError("could not register bool for cross-interpreter sharing");
    }

    // float
    if (_xidregistry_add_type(xidregistry, &PyFloat_Type, _float_shared) != 0) {
        Py_FatalError("could not register float for cross-interpreter sharing");
    }

    // tuple
    if (_xidregistry_add_type(xidregistry, &PyTuple_Type, _tuple_shared) != 0) {
        Py_FatalError("could not register tuple for cross-interpreter sharing");
    }
}
