
static crossinterpdatafunc _lookup_getdata_from_registry(
                                            PyInterpreterState *, PyObject *);

static crossinterpdatafunc
lookup_getdata(PyInterpreterState *interp, PyObject *obj)
{
   /* Cross-interpreter objects are looked up by exact match on the class.
      We can reassess this policy when we move from a global registry to a
      tp_* slot. */
    return _lookup_getdata_from_registry(interp, obj);
}

crossinterpdatafunc
_PyCrossInterpreterData_Lookup(PyObject *obj)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    return lookup_getdata(interp, obj);
}


/***********************************************/
/* a registry of {type -> crossinterpdatafunc} */
/***********************************************/

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   crossinterpdatafunc. It would be simpler and more efficient.  */


/* registry lifecycle */

static void _register_builtins_for_crossinterpreter_data(struct _xidregistry *);

static void
_xidregistry_init(struct _xidregistry *registry)
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

static void _xidregistry_clear(struct _xidregistry *);

static void
_xidregistry_fini(struct _xidregistry *registry)
{
    if (!registry->initialized) {
        return;
    }
    registry->initialized = 0;

    _xidregistry_clear(registry);
}

static inline struct _xidregistry * _get_global_xidregistry(_PyRuntimeState *);
static inline struct _xidregistry * _get_xidregistry(PyInterpreterState *);

static void
xid_lookup_init(PyInterpreterState *interp)
{
    if (_Py_IsMainInterpreter(interp)) {
        _xidregistry_init(_get_global_xidregistry(interp->runtime));
    }
    _xidregistry_init(_get_xidregistry(interp));
}

static void
xid_lookup_fini(PyInterpreterState *interp)
{
    _xidregistry_fini(_get_xidregistry(interp));
    if (_Py_IsMainInterpreter(interp)) {
        _xidregistry_fini(_get_global_xidregistry(interp->runtime));
    }
}


/* registry thread safety */

static void
_xidregistry_lock(struct _xidregistry *registry)
{
    if (registry->global) {
        PyMutex_Lock(&registry->mutex);
    }
    // else: Within an interpreter we rely on the GIL instead of a separate lock.
}

static void
_xidregistry_unlock(struct _xidregistry *registry)
{
    if (registry->global) {
        PyMutex_Unlock(&registry->mutex);
    }
}


/* accessing the registry */

static inline struct _xidregistry *
_get_global_xidregistry(_PyRuntimeState *runtime)
{
    return &runtime->xi.registry;
}

static inline struct _xidregistry *
_get_xidregistry(PyInterpreterState *interp)
{
    return &interp->xi.registry;
}

static inline struct _xidregistry *
_get_xidregistry_for_type(PyInterpreterState *interp, PyTypeObject *cls)
{
    struct _xidregistry *registry = _get_global_xidregistry(interp->runtime);
    if (cls->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        registry = _get_xidregistry(interp);
    }
    return registry;
}

static struct _xidregitem * _xidregistry_remove_entry(
        struct _xidregistry *, struct _xidregitem *);

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

static crossinterpdatafunc
_lookup_getdata_from_registry(PyInterpreterState *interp, PyObject *obj)
{
    PyTypeObject *cls = Py_TYPE(obj);

    struct _xidregistry *xidregistry = _get_xidregistry_for_type(interp, cls);
    _xidregistry_lock(xidregistry);

    struct _xidregitem *matched = _xidregistry_find_type(xidregistry, cls);
    crossinterpdatafunc func = matched != NULL ? matched->getdata : NULL;

    _xidregistry_unlock(xidregistry);
    return func;
}


/* updating the registry */

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

static void
_xidregistry_clear(struct _xidregistry *xidregistry)
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
    struct _xidregistry *xidregistry = _get_xidregistry_for_type(interp, cls);
    _xidregistry_lock(xidregistry);

    struct _xidregitem *matched = _xidregistry_find_type(xidregistry, cls);
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
_PyCrossInterpreterData_UnregisterClass(PyTypeObject *cls)
{
    int res = 0;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _xidregistry *xidregistry = _get_xidregistry_for_type(interp, cls);
    _xidregistry_lock(xidregistry);

    struct _xidregitem *matched = _xidregistry_find_type(xidregistry, cls);
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

// str

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

// int

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

// float

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

// None

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

// bool

static PyObject *
_new_bool_object(_PyCrossInterpreterData *data)
{
    if (data->data){
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static int
_bool_shared(PyThreadState *tstate, PyObject *obj,
             _PyCrossInterpreterData *data)
{
    _PyCrossInterpreterData_Init(data, tstate->interp,
            (void *) (Py_IsTrue(obj) ? (uintptr_t) 1 : (uintptr_t) 0), NULL,
            _new_bool_object);
    // data->obj and data->free remain NULL
    return 0;
}

// tuple

struct _shared_tuple_data {
    Py_ssize_t len;
    _PyCrossInterpreterData **data;
};

static PyObject *
_new_tuple_object(_PyCrossInterpreterData *data)
{
    struct _shared_tuple_data *shared = (struct _shared_tuple_data *)(data->data);
    PyObject *tuple = PyTuple_New(shared->len);
    if (tuple == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < shared->len; i++) {
        PyObject *item = _PyCrossInterpreterData_NewObject(shared->data[i]);
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
            assert(_PyCrossInterpreterData_INTERPID(shared->data[i]) == interpid);
            _PyCrossInterpreterData_Release(shared->data[i]);
            PyMem_RawFree(shared->data[i]);
            shared->data[i] = NULL;
        }
    }
    PyMem_Free(shared->data);
    PyMem_RawFree(shared);
}

static int
_tuple_shared(PyThreadState *tstate, PyObject *obj,
             _PyCrossInterpreterData *data)
{
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
    shared->data = (_PyCrossInterpreterData **) PyMem_Calloc(shared->len, sizeof(_PyCrossInterpreterData *));
    if (shared->data == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    for (Py_ssize_t i = 0; i < shared->len; i++) {
        _PyCrossInterpreterData *data = _PyCrossInterpreterData_New();
        if (data == NULL) {
            goto error;  // PyErr_NoMemory already set
        }
        PyObject *item = PyTuple_GET_ITEM(obj, i);

        int res = -1;
        if (!_Py_EnterRecursiveCallTstate(tstate, " while sharing a tuple")) {
            res = _PyObject_GetCrossInterpreterData(item, data);
            _Py_LeaveRecursiveCallTstate(tstate);
        }
        if (res < 0) {
            PyMem_RawFree(data);
            goto error;
        }
        shared->data[i] = data;
    }
    _PyCrossInterpreterData_Init(
            data, tstate->interp, shared, obj, _new_tuple_object);
    data->free = _tuple_shared_free;
    return 0;

error:
    _tuple_shared_free(shared);
    return -1;
}

// registration

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
