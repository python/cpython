#include "pycore_weakref.h"       // _PyWeakref_GET_REF()


typedef struct _dlcontext {
    _PyXIData_lookup_t *global;
    _PyXIData_lookup_t *local;
} dlcontext_t;
typedef _PyXIData_registry_t dlregistry_t;
typedef _PyXIData_regitem_t dlregitem_t;


// forward
static void _xidregistry_init(dlregistry_t *);
static void _xidregistry_fini(dlregistry_t *);
static _PyXIData_getdata_t _lookup_getdata_from_registry(
                                            dlcontext_t *, PyObject *);


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

static int
get_lookup_context(PyThreadState *tstate, dlcontext_t *res)
{
    _PyXI_global_state_t *global = _PyXI_GET_GLOBAL_STATE(tstate->interp);
    if (global == NULL) {
        assert(PyErr_Occurred());
        return -1;
    }
    _PyXI_state_t *local = _PyXI_GET_STATE(tstate->interp);
    if (local == NULL) {
        assert(PyErr_Occurred());
        return -1;
    }
    *res = (dlcontext_t){
        .global = &global->data_lookup,
        .local = &local->data_lookup,
    };
    return 0;
}

static _PyXIData_getdata_t
lookup_getdata(dlcontext_t *ctx, PyObject *obj)
{
   /* Cross-interpreter objects are looked up by exact match on the class.
      We can reassess this policy when we move from a global registry to a
      tp_* slot. */
    return _lookup_getdata_from_registry(ctx, obj);
}


/* exported API */

PyObject *
_PyXIData_GetNotShareableErrorType(PyThreadState *tstate)
{
    PyObject *exctype = get_notshareableerror_type(tstate);
    assert(exctype != NULL);
    return exctype;
}

void
_PyXIData_SetNotShareableError(PyThreadState *tstate, const char *msg)
{
    PyObject *cause = NULL;
    set_notshareableerror(tstate, cause, 1, msg);
}

void
_PyXIData_FormatNotShareableError(PyThreadState *tstate,
                                  const char *format, ...)
{
    PyObject *cause = NULL;
    va_list vargs;
    va_start(vargs, format);
    format_notshareableerror_v(tstate, cause, 1, format, vargs);
    va_end(vargs);
}

int
_PyXI_UnwrapNotShareableError(PyThreadState * tstate, _PyXI_failure *failure)
{
    PyObject *exctype = get_notshareableerror_type(tstate);
    assert(exctype != NULL);
    if (!_PyErr_ExceptionMatches(tstate, exctype)) {
        return -1;
    }
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    if (failure != NULL) {
        _PyXI_errcode code = _PyXI_ERR_NOT_SHAREABLE;
        if (_PyXI_InitFailure(failure, code, exc) < 0) {
            return -1;
        }
    }
    PyObject *cause = PyException_GetCause(exc);
    if (cause != NULL) {
        Py_DECREF(exc);
        exc = cause;
    }
    else {
        assert(PyException_GetContext(exc) == NULL);
    }
    _PyErr_SetRaisedException(tstate, exc);
    return 0;
}


_PyXIData_getdata_t
_PyXIData_Lookup(PyThreadState *tstate, PyObject *obj)
{
    dlcontext_t ctx;
    if (get_lookup_context(tstate, &ctx) < 0) {
        return (_PyXIData_getdata_t){0};
    }
    return lookup_getdata(&ctx, obj);
}


/***********************************************/
/* a registry of {type -> _PyXIData_getdata_t} */
/***********************************************/

/* For now we use a global registry of shareable classes.
   An alternative would be to add a tp_* slot for a class's
   _PyXIData_getdata_t.  It would be simpler and more efficient. */


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

static _PyXIData_getdata_t
_lookup_getdata_from_registry(dlcontext_t *ctx, PyObject *obj)
{
    PyTypeObject *cls = Py_TYPE(obj);

    dlregistry_t *xidregistry = _get_xidregistry_for_type(ctx, cls);
    _xidregistry_lock(xidregistry);

    dlregitem_t *matched = _xidregistry_find_type(xidregistry, cls);
    _PyXIData_getdata_t getdata = matched != NULL
        ? matched->getdata
        : (_PyXIData_getdata_t){0};

    _xidregistry_unlock(xidregistry);
    return getdata;
}


/* updating the registry */

static int
_xidregistry_add_type(dlregistry_t *xidregistry,
                      PyTypeObject *cls, _PyXIData_getdata_t getdata)
{
    dlregitem_t *newhead = PyMem_RawMalloc(sizeof(dlregitem_t));
    if (newhead == NULL) {
        return -1;
    }
    assert((getdata.basic == NULL) != (getdata.fallback == NULL));
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
_PyXIData_RegisterClass(PyThreadState *tstate,
                        PyTypeObject *cls, _PyXIData_getdata_t getdata)
{
    if (!PyType_Check(cls)) {
        PyErr_Format(PyExc_ValueError, "only classes may be registered");
        return -1;
    }
    if (getdata.basic == NULL && getdata.fallback == NULL) {
        PyErr_Format(PyExc_ValueError, "missing 'getdata' func");
        return -1;
    }

    int res = 0;
    dlcontext_t ctx;
    if (get_lookup_context(tstate, &ctx) < 0) {
        return -1;
    }
    dlregistry_t *xidregistry = _get_xidregistry_for_type(&ctx, cls);
    _xidregistry_lock(xidregistry);

    dlregitem_t *matched = _xidregistry_find_type(xidregistry, cls);
    if (matched != NULL) {
        assert(matched->getdata.basic == getdata.basic);
        assert(matched->getdata.fallback == getdata.fallback);
        matched->refcount += 1;
        goto finally;
    }

    res = _xidregistry_add_type(xidregistry, cls, getdata);

finally:
    _xidregistry_unlock(xidregistry);
    return res;
}

int
_PyXIData_UnregisterClass(PyThreadState *tstate, PyTypeObject *cls)
{
    int res = 0;
    dlcontext_t ctx;
    if (get_lookup_context(tstate, &ctx) < 0) {
        return -1;
    }
    dlregistry_t *xidregistry = _get_xidregistry_for_type(&ctx, cls);
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

int
_PyBytes_GetData(PyObject *obj, _PyBytes_data_t *data)
{
    if (!PyBytes_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "expected bytes, got %R", obj);
        return -1;
    }
    char *bytes;
    Py_ssize_t len;
    if (PyBytes_AsStringAndSize(obj, &bytes, &len) < 0) {
        return -1;
    }
    *data = (_PyBytes_data_t){
        .bytes = bytes,
        .len = len,
    };
    return 0;
}

PyObject *
_PyBytes_FromData(_PyBytes_data_t *data)
{
    return PyBytes_FromStringAndSize(data->bytes, data->len);
}

PyObject *
_PyBytes_FromXIData(_PyXIData_t *xidata)
{
    _PyBytes_data_t *data = (_PyBytes_data_t *)xidata->data;
    assert(_PyXIData_OBJ(xidata) != NULL
            && PyBytes_Check(_PyXIData_OBJ(xidata)));
    return _PyBytes_FromData(data);
}

static int
_bytes_shared(PyThreadState *tstate,
              PyObject *obj, size_t size, xid_newobjfunc newfunc,
              _PyXIData_t *xidata)
{
    assert(size >= sizeof(_PyBytes_data_t));
    assert(newfunc != NULL);
    if (_PyXIData_InitWithSize(
                        xidata, tstate->interp, size, obj, newfunc) < 0)
    {
        return -1;
    }
    _PyBytes_data_t *data = (_PyBytes_data_t *)xidata->data;
    if (_PyBytes_GetData(obj, data) < 0) {
        _PyXIData_Clear(tstate->interp, xidata);
        return -1;
    }
    return 0;
}

int
_PyBytes_GetXIData(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    if (!PyBytes_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "expected bytes, got %R", obj);
        return -1;
    }
    size_t size = sizeof(_PyBytes_data_t);
    return _bytes_shared(tstate, obj, size, _PyBytes_FromXIData, xidata);
}

_PyBytes_data_t *
_PyBytes_GetXIDataWrapped(PyThreadState *tstate,
                          PyObject *obj, size_t size, xid_newobjfunc newfunc,
                          _PyXIData_t *xidata)
{
    if (!PyBytes_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "expected bytes, got %R", obj);
        return NULL;
    }
    if (size < sizeof(_PyBytes_data_t)) {
        PyErr_Format(PyExc_ValueError, "expected size >= %d, got %d",
                     sizeof(_PyBytes_data_t), size);
        return NULL;
    }
    if (newfunc == NULL) {
        if (size == sizeof(_PyBytes_data_t)) {
            PyErr_SetString(PyExc_ValueError, "missing new_object func");
            return NULL;
        }
        newfunc = _PyBytes_FromXIData;
    }
    if (_bytes_shared(tstate, obj, size, newfunc, xidata) < 0) {
        return NULL;
    }
    return (_PyBytes_data_t *)xidata->data;
}

// str

struct _shared_str_data {
    int kind;
    const void *buffer;
    Py_ssize_t len;
};

static PyObject *
_new_str_object(_PyXIData_t *xidata)
{
    struct _shared_str_data *shared = (struct _shared_str_data *)(xidata->data);
    return PyUnicode_FromKindAndData(shared->kind, shared->buffer, shared->len);
}

static int
_str_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    if (_PyXIData_InitWithSize(
            xidata, tstate->interp, sizeof(struct _shared_str_data), obj,
            _new_str_object
            ) < 0)
    {
        return -1;
    }
    struct _shared_str_data *shared = (struct _shared_str_data *)xidata->data;
    shared->kind = PyUnicode_KIND(obj);
    shared->buffer = PyUnicode_DATA(obj);
    shared->len = PyUnicode_GET_LENGTH(obj);
    return 0;
}

// int

static PyObject *
_new_long_object(_PyXIData_t *xidata)
{
    return PyLong_FromSsize_t((Py_ssize_t)(xidata->data));
}

static int
_long_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
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
    _PyXIData_Init(xidata, tstate->interp, (void *)value, NULL, _new_long_object);
    // xidata->obj and xidata->free remain NULL
    return 0;
}

// float

static PyObject *
_new_float_object(_PyXIData_t *xidata)
{
    double * value_ptr = xidata->data;
    return PyFloat_FromDouble(*value_ptr);
}

static int
_float_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    if (_PyXIData_InitWithSize(
            xidata, tstate->interp, sizeof(double), NULL,
            _new_float_object
            ) < 0)
    {
        return -1;
    }
    double *shared = (double *)xidata->data;
    *shared = PyFloat_AsDouble(obj);
    return 0;
}

// None

static PyObject *
_new_none_object(_PyXIData_t *xidata)
{
    // XXX Singleton refcounts are problematic across interpreters...
    return Py_NewRef(Py_None);
}

static int
_none_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    _PyXIData_Init(xidata, tstate->interp, NULL, NULL, _new_none_object);
    // xidata->data, xidata->obj and xidata->free remain NULL
    return 0;
}

// bool

static PyObject *
_new_bool_object(_PyXIData_t *xidata)
{
    if (xidata->data){
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static int
_bool_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    _PyXIData_Init(xidata, tstate->interp,
            (void *) (Py_IsTrue(obj) ? (uintptr_t) 1 : (uintptr_t) 0), NULL,
            _new_bool_object);
    // xidata->obj and xidata->free remain NULL
    return 0;
}

// tuple

struct _shared_tuple_data {
    Py_ssize_t len;
    _PyXIData_t **items;
};

static PyObject *
_new_tuple_object(_PyXIData_t *xidata)
{
    struct _shared_tuple_data *shared = (struct _shared_tuple_data *)(xidata->data);
    PyObject *tuple = PyTuple_New(shared->len);
    if (tuple == NULL) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < shared->len; i++) {
        PyObject *item = _PyXIData_NewObject(shared->items[i]);
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
        if (shared->items[i] != NULL) {
            assert(_PyXIData_INTERPID(shared->items[i]) == interpid);
            _PyXIData_Release(shared->items[i]);
            PyMem_RawFree(shared->items[i]);
            shared->items[i] = NULL;
        }
    }
    PyMem_Free(shared->items);
    PyMem_RawFree(shared);
}

static int
_tuple_shared(PyThreadState *tstate, PyObject *obj, xidata_fallback_t fallback,
              _PyXIData_t *xidata)
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
    shared->items = (_PyXIData_t **) PyMem_Calloc(shared->len, sizeof(_PyXIData_t *));
    if (shared->items == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    for (Py_ssize_t i = 0; i < shared->len; i++) {
        _PyXIData_t *xidata_i = _PyXIData_New();
        if (xidata_i == NULL) {
            goto error;  // PyErr_NoMemory already set
        }
        PyObject *item = PyTuple_GET_ITEM(obj, i);

        int res = -1;
        if (!_Py_EnterRecursiveCallTstate(tstate, " while sharing a tuple")) {
            res = _PyObject_GetXIData(tstate, item, fallback, xidata_i);
            _Py_LeaveRecursiveCallTstate(tstate);
        }
        if (res < 0) {
            PyMem_RawFree(xidata_i);
            goto error;
        }
        shared->items[i] = xidata_i;
    }
    _PyXIData_Init(xidata, tstate->interp, shared, obj, _new_tuple_object);
    _PyXIData_SET_FREE(xidata, _tuple_shared_free);
    return 0;

error:
    _tuple_shared_free(shared);
    return -1;
}

// code

PyObject *
_PyCode_FromXIData(_PyXIData_t *xidata)
{
    return _PyMarshal_ReadObjectFromXIData(xidata);
}

int
_PyCode_GetXIData(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    if (!PyCode_Check(obj)) {
        _PyXIData_FormatNotShareableError(tstate, "expected code, got %R", obj);
        return -1;
    }
    if (_PyMarshal_GetXIData(tstate, obj, xidata) < 0) {
        return -1;
    }
    assert(_PyXIData_CHECK_NEW_OBJECT(xidata, _PyMarshal_ReadObjectFromXIData));
    _PyXIData_SET_NEW_OBJECT(xidata, _PyCode_FromXIData);
    return 0;
}

// function

PyObject *
_PyFunction_FromXIData(_PyXIData_t *xidata)
{
    // For now "stateless" functions are the only ones we must accommodate.

    PyObject *code = _PyMarshal_ReadObjectFromXIData(xidata);
    if (code == NULL) {
        return NULL;
    }
    // Create a new function.
    // For stateless functions (no globals) we use __main__ as __globals__,
    // just like we do for builtins like exec().
    assert(PyCode_Check(code));
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *globals = _PyEval_GetGlobalsFromRunningMain(tstate);  // borrowed
    if (globals == NULL) {
        if (_PyErr_Occurred(tstate)) {
            Py_DECREF(code);
            return NULL;
        }
        globals = PyDict_New();
        if (globals == NULL) {
            Py_DECREF(code);
            return NULL;
        }
    }
    else {
        Py_INCREF(globals);
    }
    if (_PyEval_EnsureBuiltins(tstate, globals, NULL) < 0) {
        Py_DECREF(code);
        Py_DECREF(globals);
        return NULL;
    }
    PyObject *func = PyFunction_New(code, globals);
    Py_DECREF(code);
    Py_DECREF(globals);
    return func;
}

int
_PyFunction_GetXIData(PyThreadState *tstate, PyObject *func,
                      _PyXIData_t *xidata)
{
    if (!PyFunction_Check(func)) {
        const char *msg = "expected a function, got %R";
        format_notshareableerror(tstate, NULL, 0, msg, func);
        return -1;
    }
    if (_PyFunction_VerifyStateless(tstate, func) < 0) {
        PyObject *cause = _PyErr_GetRaisedException(tstate);
        assert(cause != NULL);
        const char *msg = "only stateless functions are shareable";
        set_notshareableerror(tstate, cause, 0, msg);
        Py_DECREF(cause);
        return -1;
    }
    PyObject *code = PyFunction_GET_CODE(func);

    // Ideally code objects would be immortal and directly shareable.
    // In the meantime, we use marshal.
    if (_PyMarshal_GetXIData(tstate, code, xidata) < 0) {
        return -1;
    }
    // Replace _PyMarshal_ReadObjectFromXIData.
    // (_PyFunction_FromXIData() will call it.)
    _PyXIData_SET_NEW_OBJECT(xidata, _PyFunction_FromXIData);
    return 0;
}


// registration

static void
_register_builtins_for_crossinterpreter_data(dlregistry_t *xidregistry)
{
#define REGISTER(TYPE, GETDATA) \
    _xidregistry_add_type(xidregistry, (PyTypeObject *)TYPE, \
                          ((_PyXIData_getdata_t){.basic=(GETDATA)}))
#define REGISTER_FALLBACK(TYPE, GETDATA) \
    _xidregistry_add_type(xidregistry, (PyTypeObject *)TYPE, \
                          ((_PyXIData_getdata_t){.fallback=(GETDATA)}))
    // None
    if (REGISTER(Py_TYPE(Py_None), _none_shared) != 0) {
        Py_FatalError("could not register None for cross-interpreter sharing");
    }

    // int
    if (REGISTER(&PyLong_Type, _long_shared) != 0) {
        Py_FatalError("could not register int for cross-interpreter sharing");
    }

    // bytes
    if (REGISTER(&PyBytes_Type, _PyBytes_GetXIData) != 0) {
        Py_FatalError("could not register bytes for cross-interpreter sharing");
    }

    // str
    if (REGISTER(&PyUnicode_Type, _str_shared) != 0) {
        Py_FatalError("could not register str for cross-interpreter sharing");
    }

    // bool
    if (REGISTER(&PyBool_Type, _bool_shared) != 0) {
        Py_FatalError("could not register bool for cross-interpreter sharing");
    }

    // float
    if (REGISTER(&PyFloat_Type, _float_shared) != 0) {
        Py_FatalError("could not register float for cross-interpreter sharing");
    }

    // tuple
    if (REGISTER_FALLBACK(&PyTuple_Type, _tuple_shared) != 0) {
        Py_FatalError("could not register tuple for cross-interpreter sharing");
    }

    // For now, we do not register PyCode_Type or PyFunction_Type.
#undef REGISTER
#undef REGISTER_FALLBACK
}
