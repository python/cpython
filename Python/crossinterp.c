
/* API for managing interactions between isolated interpreters */

#include "Python.h"
#include "pycore_ceval.h"         // _Py_simple_func
#include "pycore_crossinterp.h"   // _PyXIData_t
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_namespace.h"     // _PyNamespace_New()
#include "pycore_typeobject.h"    // _PyStaticType_InitBuiltin()


/**************/
/* exceptions */
/**************/

typedef struct xi_exceptions exceptions_t;
static int init_static_exctypes(exceptions_t *, PyInterpreterState *);
static void fini_static_exctypes(exceptions_t *, PyInterpreterState *);
static int init_heap_exctypes(exceptions_t *);
static void fini_heap_exctypes(exceptions_t *);
#include "crossinterp_exceptions.h"


/***************************/
/* cross-interpreter calls */
/***************************/

int
_Py_CallInInterpreter(PyInterpreterState *interp,
                      _Py_simple_func func, void *arg)
{
    if (interp == PyInterpreterState_Get()) {
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
    if (interp == PyInterpreterState_Get()) {
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

/* registry of {type -> xidatafunc} */

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   xidatafunc. It would be simpler and more efficient. */

static void xid_lookup_init(_PyXIData_lookup_t *);
static void xid_lookup_fini(_PyXIData_lookup_t *);
static xidatafunc lookup_getdata(_PyXIData_lookup_context_t *, PyObject *);
#include "crossinterp_data_lookup.h"


/* lifecycle */

_PyXIData_t *
_PyXIData_New(void)
{
    _PyXIData_t *xid = PyMem_RawMalloc(sizeof(_PyXIData_t));
    if (xid == NULL) {
        PyErr_NoMemory();
    }
    return xid;
}

void
_PyXIData_Free(_PyXIData_t *xid)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    _PyXIData_Clear(interp, xid);
    PyMem_RawFree(xid);
}


/* defining cross-interpreter data */

static inline void
_xidata_init(_PyXIData_t *data)
{
    // If the value is being reused
    // then _xidata_clear() should have been called already.
    assert(data->data == NULL);
    assert(data->obj == NULL);
    *data = (_PyXIData_t){0};
    _PyXIData_INTERPID(data) = -1;
}

static inline void
_xidata_clear(_PyXIData_t *data)
{
    // _PyXIData_t only has two members that need to be
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
_PyXIData_Init(_PyXIData_t *data,
               PyInterpreterState *interp,
               void *shared, PyObject *obj,
               xid_newobjfunc new_object)
{
    assert(data != NULL);
    assert(new_object != NULL);
    _xidata_init(data);
    data->data = shared;
    if (obj != NULL) {
        assert(interp != NULL);
        // released in _PyXIData_Clear()
        data->obj = Py_NewRef(obj);
    }
    // Ideally every object would know its owning interpreter.
    // Until then, we have to rely on the caller to identify it
    // (but we don't need it in all cases).
    _PyXIData_INTERPID(data) = (interp != NULL)
        ? PyInterpreterState_GetID(interp)
        : -1;
    data->new_object = new_object;
}

int
_PyXIData_InitWithSize(_PyXIData_t *data,
                       PyInterpreterState *interp,
                       const size_t size, PyObject *obj,
                       xid_newobjfunc new_object)
{
    assert(size > 0);
    // For now we always free the shared data in the same interpreter
    // where it was allocated, so the interpreter is required.
    assert(interp != NULL);
    _PyXIData_Init(data, interp, NULL, obj, new_object);
    data->data = PyMem_RawMalloc(size);
    if (data->data == NULL) {
        return -1;
    }
    data->free = PyMem_RawFree;
    return 0;
}

void
_PyXIData_Clear(PyInterpreterState *interp, _PyXIData_t *data)
{
    assert(data != NULL);
    // This must be called in the owning interpreter.
    assert(interp == NULL
           || _PyXIData_INTERPID(data) == -1
           || _PyXIData_INTERPID(data) == PyInterpreterState_GetID(interp));
    _xidata_clear(data);
}


/* using cross-interpreter data */

static int
_check_xidata(PyThreadState *tstate, _PyXIData_t *data)
{
    // data->data can be anything, including NULL, so we don't check it.

    // data->obj may be NULL, so we don't check it.

    if (_PyXIData_INTERPID(data) < 0) {
        PyErr_SetString(PyExc_SystemError, "missing interp");
        return -1;
    }

    if (data->new_object == NULL) {
        PyErr_SetString(PyExc_SystemError, "missing new_object func");
        return -1;
    }

    // data->free may be NULL, so we don't check it.

    return 0;
}

static inline void
_set_xid_lookup_failure(dlcontext_t *ctx, PyObject *obj, const char *msg)
{
    PyObject *exctype = ctx->PyExc_NotShareableError;
    assert(exctype != NULL);
    if (msg != NULL) {
        assert(obj == NULL);
        PyErr_SetString(exctype, msg);
    }
    else if (obj == NULL) {
        PyErr_SetString(exctype,
                        "object does not support cross-interpreter data");
    }
    else {
        PyErr_Format(exctype,
                     "%S does not support cross-interpreter data", obj);
    }
}

int
_PyObject_CheckXIData(_PyXIData_lookup_context_t *ctx, PyObject *obj)
{
    xidatafunc getdata = lookup_getdata(ctx, obj);
    if (getdata == NULL) {
        if (!PyErr_Occurred()) {
            _set_xid_lookup_failure(ctx, obj, NULL);
        }
        return -1;
    }
    return 0;
}

int
_PyObject_GetXIData(_PyXIData_lookup_context_t *ctx,
                    PyObject *obj, _PyXIData_t *data)
{
    PyThreadState *tstate = PyThreadState_Get();
    PyInterpreterState *interp = tstate->interp;

    // Reset data before re-populating.
    *data = (_PyXIData_t){0};
    _PyXIData_INTERPID(data) = -1;

    // Call the "getdata" func for the object.
    Py_INCREF(obj);
    xidatafunc getdata = lookup_getdata(ctx, obj);
    if (getdata == NULL) {
        Py_DECREF(obj);
        if (!PyErr_Occurred()) {
            _set_xid_lookup_failure(ctx, obj, NULL);
        }
        return -1;
    }
    int res = getdata(tstate, obj, data);
    Py_DECREF(obj);
    if (res != 0) {
        return -1;
    }

    // Fill in the blanks and validate the result.
    _PyXIData_INTERPID(data) = PyInterpreterState_GetID(interp);
    if (_check_xidata(tstate, data) != 0) {
        (void)_PyXIData_Release(data);
        return -1;
    }

    return 0;
}

PyObject *
_PyXIData_NewObject(_PyXIData_t *data)
{
    return data->new_object(data);
}

static int
_call_clear_xidata(void *data)
{
    _xidata_clear((_PyXIData_t *)data);
    return 0;
}

static int
_xidata_release(_PyXIData_t *data, int rawfree)
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
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(
                                        _PyXIData_INTERPID(data));
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
_PyXIData_Release(_PyXIData_t *data)
{
    return _xidata_release(data, 0);
}

int
_PyXIData_ReleaseAndRawFree(_PyXIData_t *data)
{
    return _xidata_release(data, 1);
}


/*************************/
/* convenience utilities */
/*************************/

static const char *
_copy_string_obj_raw(PyObject *strobj, Py_ssize_t *p_size)
{
    Py_ssize_t size = -1;
    const char *str = PyUnicode_AsUTF8AndSize(strobj, &size);
    if (str == NULL) {
        return NULL;
    }

    if (size != (Py_ssize_t)strlen(str)) {
        PyErr_SetString(PyExc_ValueError, "found embedded NULL character");
        return NULL;
    }

    char *copied = PyMem_RawMalloc(size+1);
    if (copied == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    strcpy(copied, str);
    if (p_size != NULL) {
        *p_size = size;
    }
    return copied;
}


static int
_convert_exc_to_TracebackException(PyObject *exc, PyObject **p_tbexc)
{
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *create = NULL;

    // This is inspired by _PyErr_Display().
    PyObject *tbexc_type = PyImport_ImportModuleAttrString(
        "traceback",
        "TracebackException");
    if (tbexc_type == NULL) {
        return -1;
    }
    create = PyObject_GetAttrString(tbexc_type, "from_exception");
    Py_DECREF(tbexc_type);
    if (create == NULL) {
        return -1;
    }

    args = PyTuple_Pack(1, exc);
    if (args == NULL) {
        goto error;
    }

    kwargs = PyDict_New();
    if (kwargs == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(kwargs, "save_exc_type", Py_False) < 0) {
        goto error;
    }
    if (PyDict_SetItemString(kwargs, "lookup_lines", Py_False) < 0) {
        goto error;
    }

    PyObject *tbexc = PyObject_Call(create, args, kwargs);
    Py_DECREF(args);
    Py_DECREF(kwargs);
    Py_DECREF(create);
    if (tbexc == NULL) {
        goto error;
    }

    *p_tbexc = tbexc;
    return 0;

error:
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    Py_XDECREF(create);
    return -1;
}

// We accommodate backports here.
#ifndef _Py_EMPTY_STR
# define _Py_EMPTY_STR &_Py_STR(empty)
#endif

static const char *
_format_TracebackException(PyObject *tbexc)
{
    PyObject *lines = PyObject_CallMethod(tbexc, "format", NULL);
    if (lines == NULL) {
        return NULL;
    }
    assert(_Py_EMPTY_STR != NULL);
    PyObject *formatted_obj = PyUnicode_Join(_Py_EMPTY_STR, lines);
    Py_DECREF(lines);
    if (formatted_obj == NULL) {
        return NULL;
    }

    Py_ssize_t size = -1;
    const char *formatted = _copy_string_obj_raw(formatted_obj, &size);
    Py_DECREF(formatted_obj);
    // We remove trailing the newline added by TracebackException.format().
    assert(formatted[size-1] == '\n');
    ((char *)formatted)[size-1] = '\0';
    return formatted;
}


static int
_release_xid_data(_PyXIData_t *data, int rawfree)
{
    PyObject *exc = PyErr_GetRaisedException();
    int res = rawfree
        ? _PyXIData_Release(data)
        : _PyXIData_ReleaseAndRawFree(data);
    if (res < 0) {
        /* The owning interpreter is already destroyed. */
        _PyXIData_Clear(NULL, data);
        // XXX Emit a warning?
        PyErr_Clear();
    }
    PyErr_SetRaisedException(exc);
    return res;
}


/***********************/
/* exception snapshots */
/***********************/

static int
_excinfo_init_type_from_exception(struct _excinfo_type *info, PyObject *exc)
{
    /* Note that this copies directly rather than into an intermediate
       struct and does not clear on error.  If we need that then we
       should have a separate function to wrap this one
       and do all that there. */
    PyObject *strobj = NULL;

    PyTypeObject *type = Py_TYPE(exc);
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        assert(_Py_IsImmortal((PyObject *)type));
        info->builtin = type;
    }
    else {
        // Only builtin types are preserved.
        info->builtin = NULL;
    }

    // __name__
    strobj = PyType_GetName(type);
    if (strobj == NULL) {
        return -1;
    }
    info->name = _copy_string_obj_raw(strobj, NULL);
    Py_DECREF(strobj);
    if (info->name == NULL) {
        return -1;
    }

    // __qualname__
    strobj = PyType_GetQualName(type);
    if (strobj == NULL) {
        return -1;
    }
    info->qualname = _copy_string_obj_raw(strobj, NULL);
    Py_DECREF(strobj);
    if (info->qualname == NULL) {
        return -1;
    }

    // __module__
    strobj = PyType_GetModuleName(type);
    if (strobj == NULL) {
        return -1;
    }
    info->module = _copy_string_obj_raw(strobj, NULL);
    Py_DECREF(strobj);
    if (info->module == NULL) {
        return -1;
    }

    return 0;
}

static int
_excinfo_init_type_from_object(struct _excinfo_type *info, PyObject *exctype)
{
    PyObject *strobj = NULL;

    // __name__
    strobj = PyObject_GetAttrString(exctype, "__name__");
    if (strobj == NULL) {
        return -1;
    }
    info->name = _copy_string_obj_raw(strobj, NULL);
    Py_DECREF(strobj);
    if (info->name == NULL) {
        return -1;
    }

    // __qualname__
    strobj = PyObject_GetAttrString(exctype, "__qualname__");
    if (strobj == NULL) {
        return -1;
    }
    info->qualname = _copy_string_obj_raw(strobj, NULL);
    Py_DECREF(strobj);
    if (info->qualname == NULL) {
        return -1;
    }

    // __module__
    strobj = PyObject_GetAttrString(exctype, "__module__");
    if (strobj == NULL) {
        return -1;
    }
    info->module = _copy_string_obj_raw(strobj, NULL);
    Py_DECREF(strobj);
    if (info->module == NULL) {
        return -1;
    }

    return 0;
}

static void
_excinfo_clear_type(struct _excinfo_type *info)
{
    if (info->builtin != NULL) {
        assert(info->builtin->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN);
        assert(_Py_IsImmortal((PyObject *)info->builtin));
    }
    if (info->name != NULL) {
        PyMem_RawFree((void *)info->name);
    }
    if (info->qualname != NULL) {
        PyMem_RawFree((void *)info->qualname);
    }
    if (info->module != NULL) {
        PyMem_RawFree((void *)info->module);
    }
    *info = (struct _excinfo_type){NULL};
}

static void
_excinfo_normalize_type(struct _excinfo_type *info,
                        const char **p_module, const char **p_qualname)
{
    if (info->name == NULL) {
        assert(info->builtin == NULL);
        assert(info->qualname == NULL);
        assert(info->module == NULL);
        // This is inspired by TracebackException.format_exception_only().
        *p_module = NULL;
        *p_qualname = NULL;
        return;
    }

    const char *module = info->module;
    const char *qualname = info->qualname;
    if (qualname == NULL) {
        qualname = info->name;
    }
    assert(module != NULL);
    if (strcmp(module, "builtins") == 0) {
        module = NULL;
    }
    else if (strcmp(module, "__main__") == 0) {
        module = NULL;
    }
    *p_qualname = qualname;
    *p_module = module;
}

static void
_PyXI_excinfo_Clear(_PyXI_excinfo *info)
{
    _excinfo_clear_type(&info->type);
    if (info->msg != NULL) {
        PyMem_RawFree((void *)info->msg);
    }
    if (info->errdisplay != NULL) {
        PyMem_RawFree((void *)info->errdisplay);
    }
    *info = (_PyXI_excinfo){{NULL}};
}

PyObject *
_PyXI_excinfo_format(_PyXI_excinfo *info)
{
    const char *module, *qualname;
    _excinfo_normalize_type(&info->type, &module, &qualname);
    if (qualname != NULL) {
        if (module != NULL) {
            if (info->msg != NULL) {
                return PyUnicode_FromFormat("%s.%s: %s",
                                            module, qualname, info->msg);
            }
            else {
                return PyUnicode_FromFormat("%s.%s", module, qualname);
            }
        }
        else {
            if (info->msg != NULL) {
                return PyUnicode_FromFormat("%s: %s", qualname, info->msg);
            }
            else {
                return PyUnicode_FromString(qualname);
            }
        }
    }
    else if (info->msg != NULL) {
        return PyUnicode_FromString(info->msg);
    }
    else {
        Py_RETURN_NONE;
    }
}

static const char *
_PyXI_excinfo_InitFromException(_PyXI_excinfo *info, PyObject *exc)
{
    assert(exc != NULL);

    if (PyErr_GivenExceptionMatches(exc, PyExc_MemoryError)) {
        _PyXI_excinfo_Clear(info);
        return NULL;
    }
    const char *failure = NULL;

    if (_excinfo_init_type_from_exception(&info->type, exc) < 0) {
        failure = "error while initializing exception type snapshot";
        goto error;
    }

    // Extract the exception message.
    PyObject *msgobj = PyObject_Str(exc);
    if (msgobj == NULL) {
        failure = "error while formatting exception";
        goto error;
    }
    info->msg = _copy_string_obj_raw(msgobj, NULL);
    Py_DECREF(msgobj);
    if (info->msg == NULL) {
        failure = "error while copying exception message";
        goto error;
    }

    // Pickle a traceback.TracebackException.
    PyObject *tbexc = NULL;
    if (_convert_exc_to_TracebackException(exc, &tbexc) < 0) {
#ifdef Py_DEBUG
        PyErr_FormatUnraisable("Exception ignored while creating TracebackException");
#endif
        PyErr_Clear();
    }
    else {
        info->errdisplay = _format_TracebackException(tbexc);
        Py_DECREF(tbexc);
        if (info->errdisplay == NULL) {
#ifdef Py_DEBUG
            PyErr_FormatUnraisable("Exception ignored while formatting TracebackException");
#endif
            PyErr_Clear();
        }
    }

    return NULL;

error:
    assert(failure != NULL);
    _PyXI_excinfo_Clear(info);
    return failure;
}

static const char *
_PyXI_excinfo_InitFromObject(_PyXI_excinfo *info, PyObject *obj)
{
    const char *failure = NULL;

    PyObject *exctype = PyObject_GetAttrString(obj, "type");
    if (exctype == NULL) {
        failure = "exception snapshot missing 'type' attribute";
        goto error;
    }
    int res = _excinfo_init_type_from_object(&info->type, exctype);
    Py_DECREF(exctype);
    if (res < 0) {
        failure = "error while initializing exception type snapshot";
        goto error;
    }

    // Extract the exception message.
    PyObject *msgobj = PyObject_GetAttrString(obj, "msg");
    if (msgobj == NULL) {
        failure = "exception snapshot missing 'msg' attribute";
        goto error;
    }
    info->msg = _copy_string_obj_raw(msgobj, NULL);
    Py_DECREF(msgobj);
    if (info->msg == NULL) {
        failure = "error while copying exception message";
        goto error;
    }

    // Pickle a traceback.TracebackException.
    PyObject *errdisplay = PyObject_GetAttrString(obj, "errdisplay");
    if (errdisplay == NULL) {
        failure = "exception snapshot missing 'errdisplay' attribute";
        goto error;
    }
    info->errdisplay = _copy_string_obj_raw(errdisplay, NULL);
    Py_DECREF(errdisplay);
    if (info->errdisplay == NULL) {
        failure = "error while copying exception error display";
        goto error;
    }

    return NULL;

error:
    assert(failure != NULL);
    _PyXI_excinfo_Clear(info);
    return failure;
}

static void
_PyXI_excinfo_Apply(_PyXI_excinfo *info, PyObject *exctype)
{
    PyObject *tbexc = NULL;
    if (info->errdisplay != NULL) {
        tbexc = PyUnicode_FromString(info->errdisplay);
        if (tbexc == NULL) {
            PyErr_Clear();
        }
    }

    PyObject *formatted = _PyXI_excinfo_format(info);
    PyErr_SetObject(exctype, formatted);
    Py_DECREF(formatted);

    if (tbexc != NULL) {
        PyObject *exc = PyErr_GetRaisedException();
        if (PyObject_SetAttrString(exc, "_errdisplay", tbexc) < 0) {
#ifdef Py_DEBUG
            PyErr_FormatUnraisable("Exception ignored while "
                                   "setting _errdisplay");
#endif
            PyErr_Clear();
        }
        Py_DECREF(tbexc);
        PyErr_SetRaisedException(exc);
    }
}

static PyObject *
_PyXI_excinfo_TypeAsObject(_PyXI_excinfo *info)
{
    PyObject *ns = _PyNamespace_New(NULL);
    if (ns == NULL) {
        return NULL;
    }
    int empty = 1;

    if (info->type.name != NULL) {
        PyObject *name = PyUnicode_FromString(info->type.name);
        if (name == NULL) {
            goto error;
        }
        int res = PyObject_SetAttrString(ns, "__name__", name);
        Py_DECREF(name);
        if (res < 0) {
            goto error;
        }
        empty = 0;
    }

    if (info->type.qualname != NULL) {
        PyObject *qualname = PyUnicode_FromString(info->type.qualname);
        if (qualname == NULL) {
            goto error;
        }
        int res = PyObject_SetAttrString(ns, "__qualname__", qualname);
        Py_DECREF(qualname);
        if (res < 0) {
            goto error;
        }
        empty = 0;
    }

    if (info->type.module != NULL) {
        PyObject *module = PyUnicode_FromString(info->type.module);
        if (module == NULL) {
            goto error;
        }
        int res = PyObject_SetAttrString(ns, "__module__", module);
        Py_DECREF(module);
        if (res < 0) {
            goto error;
        }
        empty = 0;
    }

    if (empty) {
        Py_CLEAR(ns);
    }

    return ns;

error:
    Py_DECREF(ns);
    return NULL;
}

static PyObject *
_PyXI_excinfo_AsObject(_PyXI_excinfo *info)
{
    PyObject *ns = _PyNamespace_New(NULL);
    if (ns == NULL) {
        return NULL;
    }
    int res;

    PyObject *type = _PyXI_excinfo_TypeAsObject(info);
    if (type == NULL) {
        if (PyErr_Occurred()) {
            goto error;
        }
        type = Py_NewRef(Py_None);
    }
    res = PyObject_SetAttrString(ns, "type", type);
    Py_DECREF(type);
    if (res < 0) {
        goto error;
    }

    PyObject *msg = info->msg != NULL
        ? PyUnicode_FromString(info->msg)
        : Py_NewRef(Py_None);
    if (msg == NULL) {
        goto error;
    }
    res = PyObject_SetAttrString(ns, "msg", msg);
    Py_DECREF(msg);
    if (res < 0) {
        goto error;
    }

    PyObject *formatted = _PyXI_excinfo_format(info);
    if (formatted == NULL) {
        goto error;
    }
    res = PyObject_SetAttrString(ns, "formatted", formatted);
    Py_DECREF(formatted);
    if (res < 0) {
        goto error;
    }

    if (info->errdisplay != NULL) {
        PyObject *tbexc = PyUnicode_FromString(info->errdisplay);
        if (tbexc == NULL) {
            PyErr_Clear();
        }
        else {
            res = PyObject_SetAttrString(ns, "errdisplay", tbexc);
            Py_DECREF(tbexc);
            if (res < 0) {
                goto error;
            }
        }
    }

    return ns;

error:
    Py_DECREF(ns);
    return NULL;
}


int
_PyXI_InitExcInfo(_PyXI_excinfo *info, PyObject *exc)
{
    assert(!PyErr_Occurred());
    if (exc == NULL || exc == Py_None) {
        PyErr_SetString(PyExc_ValueError, "missing exc");
        return -1;
    }
    const char *failure;
    if (PyExceptionInstance_Check(exc) || PyExceptionClass_Check(exc)) {
        failure = _PyXI_excinfo_InitFromException(info, exc);
    }
    else {
        failure = _PyXI_excinfo_InitFromObject(info, exc);
    }
    if (failure != NULL) {
        PyErr_SetString(PyExc_Exception, failure);
        return -1;
    }
    return 0;
}

PyObject *
_PyXI_FormatExcInfo(_PyXI_excinfo *info)
{
    return _PyXI_excinfo_format(info);
}

PyObject *
_PyXI_ExcInfoAsObject(_PyXI_excinfo *info)
{
    return _PyXI_excinfo_AsObject(info);
}

void
_PyXI_ClearExcInfo(_PyXI_excinfo *info)
{
    _PyXI_excinfo_Clear(info);
}


/***************************/
/* short-term data sharing */
/***************************/

/* error codes */

static int
_PyXI_ApplyErrorCode(_PyXI_errcode code, PyInterpreterState *interp)
{
    dlcontext_t ctx;

    assert(!PyErr_Occurred());
    switch (code) {
    case _PyXI_ERR_NO_ERROR: _Py_FALLTHROUGH;
    case _PyXI_ERR_UNCAUGHT_EXCEPTION:
        // There is nothing to apply.
#ifdef Py_DEBUG
        Py_UNREACHABLE();
#endif
        return 0;
    case _PyXI_ERR_OTHER:
        // XXX msg?
        PyErr_SetNone(PyExc_InterpreterError);
        break;
    case _PyXI_ERR_NO_MEMORY:
        PyErr_NoMemory();
        break;
    case _PyXI_ERR_ALREADY_RUNNING:
        assert(interp != NULL);
        _PyErr_SetInterpreterAlreadyRunning();
        break;
    case _PyXI_ERR_MAIN_NS_FAILURE:
        PyErr_SetString(PyExc_InterpreterError,
                        "failed to get __main__ namespace");
        break;
    case _PyXI_ERR_APPLY_NS_FAILURE:
        PyErr_SetString(PyExc_InterpreterError,
                        "failed to apply namespace to __main__");
        break;
    case _PyXI_ERR_NOT_SHAREABLE:
        if (_PyXIData_GetLookupContext(interp, &ctx) < 0) {
            return -1;
        }
        _set_xid_lookup_failure(&ctx, NULL, NULL);
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

static const char *
_PyXI_InitError(_PyXI_error *error, PyObject *excobj, _PyXI_errcode code)
{
    if (error->interp == NULL) {
        error->interp = PyInterpreterState_Get();
    }

    const char *failure = NULL;
    if (code == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // There is an unhandled exception we need to propagate.
        failure = _PyXI_excinfo_InitFromException(&error->uncaught, excobj);
        if (failure != NULL) {
            // We failed to initialize error->uncaught.
            // XXX Print the excobj/traceback?  Emit a warning?
            // XXX Print the current exception/traceback?
            if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
                error->code = _PyXI_ERR_NO_MEMORY;
            }
            else {
                error->code = _PyXI_ERR_OTHER;
            }
            PyErr_Clear();
        }
        else {
            error->code = code;
        }
        assert(error->code != _PyXI_ERR_NO_ERROR);
    }
    else {
        // There is an error code we need to propagate.
        assert(excobj == NULL);
        assert(code != _PyXI_ERR_NO_ERROR);
        error->code = code;
        _PyXI_excinfo_Clear(&error->uncaught);
    }
    return failure;
}

PyObject *
_PyXI_ApplyError(_PyXI_error *error)
{
    if (error->code == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // Raise an exception that proxies the propagated exception.
       return _PyXI_excinfo_AsObject(&error->uncaught);
    }
    else if (error->code == _PyXI_ERR_NOT_SHAREABLE) {
        // Propagate the exception directly.
        dlcontext_t ctx;
        if (_PyXIData_GetLookupContext(error->interp, &ctx) < 0) {
            return NULL;
        }
        _set_xid_lookup_failure(&ctx, NULL, error->uncaught.msg);
    }
    else {
        // Raise an exception corresponding to the code.
        assert(error->code != _PyXI_ERR_NO_ERROR);
        (void)_PyXI_ApplyErrorCode(error->code, error->interp);
        if (error->uncaught.type.name != NULL || error->uncaught.msg != NULL) {
            // __context__ will be set to a proxy of the propagated exception.
            PyObject *exc = PyErr_GetRaisedException();
            _PyXI_excinfo_Apply(&error->uncaught, PyExc_InterpreterError);
            PyObject *exc2 = PyErr_GetRaisedException();
            PyException_SetContext(exc, exc2);
            PyErr_SetRaisedException(exc);
        }
    }
    assert(PyErr_Occurred());
    return NULL;
}

/* shared namespaces */

/* Shared namespaces are expected to have relatively short lifetimes.
   This means dealloc of a shared namespace will normally happen "soon".
   Namespace items hold cross-interpreter data, which must get released.
   If the namespace/items are cleared in a different interpreter than
   where the items' cross-interpreter data was set then that will cause
   pending calls to be used to release the cross-interpreter data.
   The tricky bit is that the pending calls can happen sufficiently
   later that the namespace/items might already be deallocated.  This is
   a problem if the cross-interpreter data is allocated as part of a
   namespace item.  If that's the case then we must ensure the shared
   namespace is only cleared/freed *after* that data has been released. */

typedef struct _sharednsitem {
    const char *name;
    _PyXIData_t *data;
    // We could have a "PyXIData _data" field, so it would
    // be allocated as part of the item and avoid an extra allocation.
    // However, doing so adds a bunch of complexity because we must
    // ensure the item isn't freed before a pending call might happen
    // in a different interpreter to release the XI data.
} _PyXI_namespace_item;

static int
_sharednsitem_is_initialized(_PyXI_namespace_item *item)
{
    if (item->name != NULL) {
        return 1;
    }
    return 0;
}

static int
_sharednsitem_init(_PyXI_namespace_item *item, PyObject *key)
{
    item->name = _copy_string_obj_raw(key, NULL);
    if (item->name == NULL) {
        assert(!_sharednsitem_is_initialized(item));
        return -1;
    }
    item->data = NULL;
    assert(_sharednsitem_is_initialized(item));
    return 0;
}

static int
_sharednsitem_has_value(_PyXI_namespace_item *item, int64_t *p_interpid)
{
    if (item->data == NULL) {
        return 0;
    }
    if (p_interpid != NULL) {
        *p_interpid = _PyXIData_INTERPID(item->data);
    }
    return 1;
}

static int
_sharednsitem_set_value(_PyXI_namespace_item *item, PyObject *value)
{
    assert(_sharednsitem_is_initialized(item));
    assert(item->data == NULL);
    item->data = PyMem_RawMalloc(sizeof(_PyXIData_t));
    if (item->data == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    PyInterpreterState *interp = PyInterpreterState_Get();
    dlcontext_t ctx;
    if (_PyXIData_GetLookupContext(interp, &ctx) < 0) {
        return -1;
    }
    if (_PyObject_GetXIData(&ctx, value, item->data) != 0) {
        PyMem_RawFree(item->data);
        item->data = NULL;
        // The caller may want to propagate PyExc_NotShareableError
        // if currently switched between interpreters.
        return -1;
    }
    return 0;
}

static void
_sharednsitem_clear_value(_PyXI_namespace_item *item)
{
    _PyXIData_t *data = item->data;
    if (data != NULL) {
        item->data = NULL;
        int rawfree = 1;
        (void)_release_xid_data(data, rawfree);
    }
}

static void
_sharednsitem_clear(_PyXI_namespace_item *item)
{
    if (item->name != NULL) {
        PyMem_RawFree((void *)item->name);
        item->name = NULL;
    }
    _sharednsitem_clear_value(item);
}

static int
_sharednsitem_copy_from_ns(struct _sharednsitem *item, PyObject *ns)
{
    assert(item->name != NULL);
    assert(item->data == NULL);
    PyObject *value = PyDict_GetItemString(ns, item->name);  // borrowed
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        // When applied, this item will be set to the default (or fail).
        return 0;
    }
    if (_sharednsitem_set_value(item, value) < 0) {
        return -1;
    }
    return 0;
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
        value = _PyXIData_NewObject(item->data);
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
    Py_ssize_t len;
    _PyXI_namespace_item *items;
};

static _PyXI_namespace *
_sharedns_new(void)
{
    _PyXI_namespace *ns = PyMem_RawCalloc(sizeof(_PyXI_namespace), 1);
    if (ns == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    *ns = (_PyXI_namespace){ 0 };
    return ns;
}

static int
_sharedns_is_initialized(_PyXI_namespace *ns)
{
    if (ns->len == 0) {
        assert(ns->items == NULL);
        return 0;
    }

    assert(ns->len > 0);
    assert(ns->items != NULL);
    assert(_sharednsitem_is_initialized(&ns->items[0]));
    assert(ns->len == 1
           || _sharednsitem_is_initialized(&ns->items[ns->len - 1]));
    return 1;
}

#define HAS_COMPLETE_DATA 1
#define HAS_PARTIAL_DATA 2

static int
_sharedns_has_xidata(_PyXI_namespace *ns, int64_t *p_interpid)
{
    // We expect _PyXI_namespace to always be initialized.
    assert(_sharedns_is_initialized(ns));
    int res = 0;
    _PyXI_namespace_item *item0 = &ns->items[0];
    if (!_sharednsitem_is_initialized(item0)) {
        return 0;
    }
    int64_t interpid0 = -1;
    if (!_sharednsitem_has_value(item0, &interpid0)) {
        return 0;
    }
    if (ns->len > 1) {
        // At this point we know it is has at least partial data.
        _PyXI_namespace_item *itemN = &ns->items[ns->len-1];
        if (!_sharednsitem_is_initialized(itemN)) {
            res = HAS_PARTIAL_DATA;
            goto finally;
        }
        int64_t interpidN = -1;
        if (!_sharednsitem_has_value(itemN, &interpidN)) {
            res = HAS_PARTIAL_DATA;
            goto finally;
        }
        assert(interpidN == interpid0);
    }
    res = HAS_COMPLETE_DATA;
    *p_interpid = interpid0;

finally:
    return res;
}

static void
_sharedns_clear(_PyXI_namespace *ns)
{
    if (!_sharedns_is_initialized(ns)) {
        return;
    }

    // If the cross-interpreter data were allocated as part of
    // _PyXI_namespace_item (instead of dynamically), this is where
    // we would need verify that we are clearing the items in the
    // correct interpreter, to avoid a race with releasing the XI data
    // via a pending call.  See _sharedns_has_xidata().
    for (Py_ssize_t i=0; i < ns->len; i++) {
        _sharednsitem_clear(&ns->items[i]);
    }
    PyMem_RawFree(ns->items);
    ns->items = NULL;
    ns->len = 0;
}

static void
_sharedns_free(_PyXI_namespace *ns)
{
    _sharedns_clear(ns);
    PyMem_RawFree(ns);
}

static int
_sharedns_init(_PyXI_namespace *ns, PyObject *names)
{
    assert(!_sharedns_is_initialized(ns));
    assert(names != NULL);
    Py_ssize_t len = PyDict_CheckExact(names)
        ? PyDict_Size(names)
        : PySequence_Size(names);
    if (len < 0) {
        return -1;
    }
    if (len == 0) {
        PyErr_SetString(PyExc_ValueError, "empty namespaces not allowed");
        return -1;
    }
    assert(len > 0);

    // Allocate the items.
    _PyXI_namespace_item *items =
            PyMem_RawCalloc(sizeof(struct _sharednsitem), len);
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    // Fill in the names.
    Py_ssize_t i = -1;
    if (PyDict_CheckExact(names)) {
        Py_ssize_t pos = 0;
        for (i=0; i < len; i++) {
            PyObject *key;
            if (!PyDict_Next(names, &pos, &key, NULL)) {
                // This should not be possible.
                assert(0);
                goto error;
            }
            if (_sharednsitem_init(&items[i], key) < 0) {
                goto error;
            }
        }
    }
    else if (PySequence_Check(names)) {
        for (i=0; i < len; i++) {
            PyObject *key = PySequence_GetItem(names, i);
            if (key == NULL) {
                goto error;
            }
            int res = _sharednsitem_init(&items[i], key);
            Py_DECREF(key);
            if (res < 0) {
                goto error;
            }
        }
    }
    else {
        PyErr_SetString(PyExc_NotImplementedError,
                        "non-sequence namespace not supported");
        goto error;
    }

    ns->items = items;
    ns->len = len;
    assert(_sharedns_is_initialized(ns));
    return 0;

error:
    for (Py_ssize_t j=0; j < i; j++) {
        _sharednsitem_clear(&items[j]);
    }
    PyMem_RawFree(items);
    assert(!_sharedns_is_initialized(ns));
    return -1;
}

void
_PyXI_FreeNamespace(_PyXI_namespace *ns)
{
    if (!_sharedns_is_initialized(ns)) {
        return;
    }

    int64_t interpid = -1;
    if (!_sharedns_has_xidata(ns, &interpid)) {
        _sharedns_free(ns);
        return;
    }

    if (interpid == PyInterpreterState_GetID(PyInterpreterState_Get())) {
        _sharedns_free(ns);
    }
    else {
        // If we weren't always dynamically allocating the cross-interpreter
        // data in each item then we would need to using a pending call
        // to call _sharedns_free(), to avoid the race between freeing
        // the shared namespace and releasing the XI data.
        _sharedns_free(ns);
    }
}

_PyXI_namespace *
_PyXI_NamespaceFromNames(PyObject *names)
{
    if (names == NULL || names == Py_None) {
        return NULL;
    }

    _PyXI_namespace *ns = _sharedns_new();
    if (ns == NULL) {
        return NULL;
    }

    if (_sharedns_init(ns, names) < 0) {
        PyMem_RawFree(ns);
        if (PySequence_Size(names) == 0) {
            PyErr_Clear();
        }
        return NULL;
    }

    return ns;
}

#ifndef NDEBUG
static int _session_is_active(_PyXI_session *);
#endif
static void _propagate_not_shareable_error(_PyXI_session *);

int
_PyXI_FillNamespaceFromDict(_PyXI_namespace *ns, PyObject *nsobj,
                            _PyXI_session *session)
{
    // session must be entered already, if provided.
    assert(session == NULL || _session_is_active(session));
    assert(_sharedns_is_initialized(ns));
    for (Py_ssize_t i=0; i < ns->len; i++) {
        _PyXI_namespace_item *item = &ns->items[i];
        if (_sharednsitem_copy_from_ns(item, nsobj) < 0) {
            _propagate_not_shareable_error(session);
            // Clear out the ones we set so far.
            for (Py_ssize_t j=0; j < i; j++) {
                _sharednsitem_clear_value(&ns->items[j]);
            }
            return -1;
        }
    }
    return 0;
}

// All items are expected to be shareable.
static _PyXI_namespace *
_PyXI_NamespaceFromDict(PyObject *nsobj, _PyXI_session *session)
{
    // session must be entered already, if provided.
    assert(session == NULL || _session_is_active(session));
    if (nsobj == NULL || nsobj == Py_None) {
        return NULL;
    }
    if (!PyDict_CheckExact(nsobj)) {
        PyErr_SetString(PyExc_TypeError, "expected a dict");
        return NULL;
    }

    _PyXI_namespace *ns = _sharedns_new();
    if (ns == NULL) {
        return NULL;
    }

    if (_sharedns_init(ns, nsobj) < 0) {
        if (PyDict_Size(nsobj) == 0) {
            PyMem_RawFree(ns);
            PyErr_Clear();
            return NULL;
        }
        goto error;
    }

    if (_PyXI_FillNamespaceFromDict(ns, nsobj, session) < 0) {
        goto error;
    }

    return ns;

error:
    assert(PyErr_Occurred()
           || (session != NULL && session->error_override != NULL));
    _sharedns_free(ns);
    return NULL;
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
    assert(session->error_override == NULL);
    // Set elsewhere and cleared in _PyXI_ApplyCapturedException().
    assert(session->error == NULL);

    // Switch to interpreter.
    PyThreadState *tstate = PyThreadState_Get();
    PyThreadState *prev = tstate;
    if (interp != tstate->interp) {
        tstate = _PyThreadState_NewBound(interp, _PyThreadState_WHENCE_EXEC);
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

#ifndef NDEBUG
static int
_session_is_active(_PyXI_session *session)
{
    return (session->init_tstate != NULL);
}
#endif

static void
_propagate_not_shareable_error(_PyXI_session *session)
{
    if (session == NULL) {
        return;
    }
    PyInterpreterState *interp = PyInterpreterState_Get();
    dlcontext_t ctx;
    if (_PyXIData_GetLookupContext(interp, &ctx) < 0) {
        PyErr_FormatUnraisable(
                "Exception ignored while propagating not shareable error");
        return;
    }
    if (PyErr_ExceptionMatches(ctx.PyExc_NotShareableError)) {
        // We want to propagate the exception directly.
        session->_error_override = _PyXI_ERR_NOT_SHAREABLE;
        session->error_override = &session->_error_override;
    }
}

static void
_capture_current_exception(_PyXI_session *session)
{
    assert(session->error == NULL);
    if (!PyErr_Occurred()) {
        assert(session->error_override == NULL);
        return;
    }

    // Handle the exception override.
    _PyXI_errcode *override = session->error_override;
    session->error_override = NULL;
    _PyXI_errcode errcode = override != NULL
        ? *override
        : _PyXI_ERR_UNCAUGHT_EXCEPTION;

    // Pop the exception object.
    PyObject *excval = NULL;
    if (errcode == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // We want to actually capture the current exception.
        excval = PyErr_GetRaisedException();
    }
    else if (errcode == _PyXI_ERR_ALREADY_RUNNING) {
        // We don't need the exception info.
        PyErr_Clear();
    }
    else {
        // We could do a variety of things here, depending on errcode.
        // However, for now we simply capture the exception and save
        // the errcode.
        excval = PyErr_GetRaisedException();
    }

    // Capture the exception.
    _PyXI_error *err = &session->_error;
    *err = (_PyXI_error){
        .interp = session->init_tstate->interp,
    };
    const char *failure;
    if (excval == NULL) {
        failure = _PyXI_InitError(err, NULL, errcode);
    }
    else {
        failure = _PyXI_InitError(err, excval, _PyXI_ERR_UNCAUGHT_EXCEPTION);
        Py_DECREF(excval);
        if (failure == NULL && override != NULL) {
            err->code = errcode;
        }
    }

    // Handle capture failure.
    if (failure != NULL) {
        // XXX Make this error message more generic.
        fprintf(stderr,
                "RunFailedError: script raised an uncaught exception (%s)",
                failure);
        err = NULL;
    }

    // Finished!
    assert(!PyErr_Occurred());
    session->error  = err;
}

PyObject *
_PyXI_ApplyCapturedException(_PyXI_session *session)
{
    assert(!PyErr_Occurred());
    assert(session->error != NULL);
    PyObject *res = _PyXI_ApplyError(session->error);
    assert((res == NULL) != (PyErr_Occurred() == NULL));
    session->error = NULL;
    return res;
}

int
_PyXI_HasCapturedException(_PyXI_session *session)
{
    return session->error != NULL;
}

int
_PyXI_Enter(_PyXI_session *session,
            PyInterpreterState *interp, PyObject *nsupdates)
{
    // Convert the attrs for cross-interpreter use.
    _PyXI_namespace *sharedns = NULL;
    if (nsupdates != NULL) {
        sharedns = _PyXI_NamespaceFromDict(nsupdates, NULL);
        if (sharedns == NULL && PyErr_Occurred()) {
            assert(session->error == NULL);
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
        errcode = _PyXI_ERR_MAIN_NS_FAILURE;
        goto error;
    }
    PyObject *ns = PyModule_GetDict(main_mod);  // borrowed
    Py_DECREF(main_mod);
    if (ns == NULL) {
        errcode = _PyXI_ERR_MAIN_NS_FAILURE;
        goto error;
    }
    session->main_ns = Py_NewRef(ns);

    // Apply the cross-interpreter data.
    if (sharedns != NULL) {
        if (_PyXI_ApplyNamespace(sharedns, ns, NULL) < 0) {
            errcode = _PyXI_ERR_APPLY_NS_FAILURE;
            goto error;
        }
        _PyXI_FreeNamespace(sharedns);
    }

    errcode = _PyXI_ERR_NO_ERROR;
    assert(!PyErr_Occurred());
    return 0;

error:
    assert(PyErr_Occurred());
    // We want to propagate all exceptions here directly (best effort).
    assert(errcode != _PyXI_ERR_UNCAUGHT_EXCEPTION);
    session->error_override = &errcode;
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


/*********************/
/* runtime lifecycle */
/*********************/

int
_Py_xi_global_state_init(_PyXI_global_state_t *state)
{
    assert(state != NULL);
    xid_lookup_init(&state->data_lookup);
    return 0;
}

void
_Py_xi_global_state_fini(_PyXI_global_state_t *state)
{
    assert(state != NULL);
    xid_lookup_fini(&state->data_lookup);
}

int
_Py_xi_state_init(_PyXI_state_t *state, PyInterpreterState *interp)
{
    assert(state != NULL);
    assert(interp == NULL || state == _PyXI_GET_STATE(interp));

    xid_lookup_init(&state->data_lookup);

    // Initialize exceptions.
    if (interp != NULL) {
        if (init_static_exctypes(&state->exceptions, interp) < 0) {
            fini_heap_exctypes(&state->exceptions);
            return -1;
        }
    }
    if (init_heap_exctypes(&state->exceptions) < 0) {
        return -1;
    }

    return 0;
}

void
_Py_xi_state_fini(_PyXI_state_t *state, PyInterpreterState *interp)
{
    assert(state != NULL);
    assert(interp == NULL || state == _PyXI_GET_STATE(interp));

    fini_heap_exctypes(&state->exceptions);
    if (interp != NULL) {
        fini_static_exctypes(&state->exceptions, interp);
    }

    xid_lookup_fini(&state->data_lookup);
}


PyStatus
_PyXI_Init(PyInterpreterState *interp)
{
    if (_Py_IsMainInterpreter(interp)) {
        _PyXI_global_state_t *global_state = _PyXI_GET_GLOBAL_STATE(interp);
        if (global_state == NULL) {
            PyErr_PrintEx(0);
            return _PyStatus_ERR(
                    "failed to get global cross-interpreter state");
        }
        if (_Py_xi_global_state_init(global_state) < 0) {
            PyErr_PrintEx(0);
            return _PyStatus_ERR(
                    "failed to initialize  global cross-interpreter state");
        }
    }

    _PyXI_state_t *state = _PyXI_GET_STATE(interp);
    if (state == NULL) {
        PyErr_PrintEx(0);
        return _PyStatus_ERR(
                "failed to get interpreter's cross-interpreter state");
    }
    // The static types were already initialized in _PyXI_InitTypes(),
    // so we pass in NULL here to avoid initializing them again.
    if (_Py_xi_state_init(state, NULL) < 0) {
        PyErr_PrintEx(0);
        return _PyStatus_ERR(
                "failed to initialize interpreter's cross-interpreter state");
    }

    return _PyStatus_OK();
}

// _PyXI_Fini() must be called before the interpreter is cleared,
// since we must clear some heap objects.

void
_PyXI_Fini(PyInterpreterState *interp)
{
    _PyXI_state_t *state = _PyXI_GET_STATE(interp);
#ifndef NDEBUG
    if (state == NULL) {
        PyErr_PrintEx(0);
        return;
    }
#endif
    // The static types will be finalized soon in _PyXI_FiniTypes(),
    // so we pass in NULL here to avoid finalizing them right now.
    _Py_xi_state_fini(state, NULL);

    if (_Py_IsMainInterpreter(interp)) {
        _PyXI_global_state_t *global_state = _PyXI_GET_GLOBAL_STATE(interp);
        _Py_xi_global_state_fini(global_state);
    }
}

PyStatus
_PyXI_InitTypes(PyInterpreterState *interp)
{
    if (init_static_exctypes(&_PyXI_GET_STATE(interp)->exceptions, interp) < 0) {
        PyErr_PrintEx(0);
        return _PyStatus_ERR(
                "failed to initialize the cross-interpreter exception types");
    }
    // We would initialize heap types here too but that leads to ref leaks.
    // Instead, we intialize them in _PyXI_Init().
    return _PyStatus_OK();
}

void
_PyXI_FiniTypes(PyInterpreterState *interp)
{
    // We would finalize heap types here too but that leads to ref leaks.
    // Instead, we finalize them in _PyXI_Fini().
    fini_static_exctypes(&_PyXI_GET_STATE(interp)->exceptions, interp);
}


/*************/
/* other API */
/*************/

PyInterpreterState *
_PyXI_NewInterpreter(PyInterpreterConfig *config, long *maybe_whence,
                     PyThreadState **p_tstate, PyThreadState **p_save_tstate)
{
    PyThreadState *save_tstate = PyThreadState_Swap(NULL);
    assert(save_tstate != NULL);

    PyThreadState *tstate;
    PyStatus status = Py_NewInterpreterFromConfig(&tstate, config);
    if (PyStatus_Exception(status)) {
        // Since no new thread state was created, there is no exception
        // to propagate; raise a fresh one after swapping back in the
        // old thread state.
        PyThreadState_Swap(save_tstate);
        _PyErr_SetFromPyStatus(status);
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_InterpreterError,
                        "sub-interpreter creation failed");
        _PyErr_ChainExceptions1(exc);
        return NULL;
    }
    assert(tstate != NULL);
    PyInterpreterState *interp = PyThreadState_GetInterpreter(tstate);

    long whence = _PyInterpreterState_WHENCE_XI;
    if (maybe_whence != NULL) {
        whence = *maybe_whence;
    }
    _PyInterpreterState_SetWhence(interp, whence);

    if (p_tstate != NULL) {
        // We leave the new thread state as the current one.
        *p_tstate = tstate;
    }
    else {
        // Throw away the initial tstate.
        PyThreadState_Clear(tstate);
        PyThreadState_Swap(save_tstate);
        PyThreadState_Delete(tstate);
        save_tstate = NULL;
    }
    if (p_save_tstate != NULL) {
        *p_save_tstate = save_tstate;
    }
    return interp;
}

void
_PyXI_EndInterpreter(PyInterpreterState *interp,
                     PyThreadState *tstate, PyThreadState **p_save_tstate)
{
#ifndef NDEBUG
    long whence = _PyInterpreterState_GetWhence(interp);
#endif
    assert(whence != _PyInterpreterState_WHENCE_RUNTIME);

    if (!_PyInterpreterState_IsReady(interp)) {
        assert(whence == _PyInterpreterState_WHENCE_UNKNOWN);
        // PyInterpreterState_Clear() requires the GIL,
        // which a not-ready does not have, so we don't clear it.
        // That means there may be leaks here until clearing the
        // interpreter is fixed.
        PyInterpreterState_Delete(interp);
        return;
    }
    assert(whence != _PyInterpreterState_WHENCE_UNKNOWN);

    PyThreadState *save_tstate = NULL;
    PyThreadState *cur_tstate = PyThreadState_GET();
    if (tstate == NULL) {
        if (PyThreadState_GetInterpreter(cur_tstate) == interp) {
            tstate = cur_tstate;
        }
        else {
            tstate = _PyThreadState_NewBound(interp, _PyThreadState_WHENCE_FINI);
            assert(tstate != NULL);
            save_tstate = PyThreadState_Swap(tstate);
        }
    }
    else {
        assert(PyThreadState_GetInterpreter(tstate) == interp);
        if (tstate != cur_tstate) {
            assert(PyThreadState_GetInterpreter(cur_tstate) != interp);
            save_tstate = PyThreadState_Swap(tstate);
        }
    }

    Py_EndInterpreter(tstate);

    if (p_save_tstate != NULL) {
        save_tstate = *p_save_tstate;
    }
    PyThreadState_Swap(save_tstate);
}
