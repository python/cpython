
/* API for managing interactions between isolated interpreters */

#include "Python.h"
#include "marshal.h"              // PyMarshal_WriteObjectToString()
#include "osdefs.h"               // MAXPATHLEN
#include "pycore_ceval.h"         // _Py_simple_func
#include "pycore_crossinterp.h"   // _PyXIData_t
#include "pycore_function.h"      // _PyFunction_VerifyStateless()
#include "pycore_global_strings.h"  // _Py_ID()
#include "pycore_import.h"        // _PyImport_SetModule()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_namespace.h"     // _PyNamespace_New()
#include "pycore_pythonrun.h"     // _Py_SourceAsString()
#include "pycore_runtime.h"       // _PyRuntime
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include "pycore_typeobject.h"    // _PyStaticType_InitBuiltin()


static Py_ssize_t
_Py_GetMainfile(char *buffer, size_t maxlen)
{
    // We don't expect subinterpreters to have the __main__ module's
    // __name__ set, but proceed just in case.
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *module = _Py_GetMainModule(tstate);
    if (_Py_CheckMainModule(module) < 0) {
        Py_XDECREF(module);
        return -1;
    }
    Py_ssize_t size = _PyModule_GetFilenameUTF8(module, buffer, maxlen);
    Py_DECREF(module);
    return size;
}


static PyObject *
runpy_run_path(const char *filename, const char *modname)
{
    PyObject *run_path = PyImport_ImportModuleAttrString("runpy", "run_path");
    if (run_path == NULL) {
        return NULL;
    }
    PyObject *args = Py_BuildValue("(sOs)", filename, Py_None, modname);
    if (args == NULL) {
        Py_DECREF(run_path);
        return NULL;
    }
    PyObject *ns = PyObject_Call(run_path, args, NULL);
    Py_DECREF(run_path);
    Py_DECREF(args);
    return ns;
}


static void
set_exc_with_cause(PyObject *exctype, const char *msg)
{
    PyObject *cause = PyErr_GetRaisedException();
    PyErr_SetString(exctype, msg);
    PyObject *exc = PyErr_GetRaisedException();
    PyException_SetCause(exc, cause);
    PyErr_SetRaisedException(exc);
}


/****************************/
/* module duplication utils */
/****************************/

struct sync_module_result {
    PyObject *module;
    PyObject *loaded;
    PyObject *failed;
};

struct sync_module {
    const char *filename;
    char _filename[MAXPATHLEN+1];
    struct sync_module_result cached;
};

static void
sync_module_clear(struct sync_module *data)
{
    data->filename = NULL;
    Py_CLEAR(data->cached.module);
    Py_CLEAR(data->cached.loaded);
    Py_CLEAR(data->cached.failed);
}

static void
sync_module_capture_exc(PyThreadState *tstate, struct sync_module *data)
{
    assert(_PyErr_Occurred(tstate));
    PyObject *context = data->cached.failed;
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    _PyErr_SetRaisedException(tstate, Py_NewRef(exc));
    if (context != NULL) {
        PyException_SetContext(exc, context);
    }
    data->cached.failed = exc;
}


static int
ensure_isolated_main(PyThreadState *tstate, struct sync_module *main)
{
    // Load the module from the original file (or from a cache).

    // First try the local cache.
    if (main->cached.failed != NULL) {
        // We'll deal with this in apply_isolated_main().
        assert(main->cached.module == NULL);
        assert(main->cached.loaded == NULL);
        return 0;
    }
    else if (main->cached.loaded != NULL) {
        assert(main->cached.module != NULL);
        return 0;
    }
    assert(main->cached.module == NULL);

    if (main->filename == NULL) {
        _PyErr_SetString(tstate, PyExc_NotImplementedError, "");
        return -1;
    }

    // It wasn't in the local cache so we'll need to populate it.
    PyObject *mod = _Py_GetMainModule(tstate);
    if (_Py_CheckMainModule(mod) < 0) {
        // This is probably unrecoverable, so don't bother caching the error.
        assert(_PyErr_Occurred(tstate));
        Py_XDECREF(mod);
        return -1;
    }
    PyObject *loaded = NULL;

    // Try the per-interpreter cache for the loaded module.
    // XXX Store it in sys.modules?
    PyObject *interpns = PyInterpreterState_GetDict(tstate->interp);
    assert(interpns != NULL);
    PyObject *key = PyUnicode_FromString("CACHED_MODULE_NS___main__");
    if (key == NULL) {
        // It's probably unrecoverable, so don't bother caching the error.
        Py_DECREF(mod);
        return -1;
    }
    else if (PyDict_GetItemRef(interpns, key, &loaded) < 0) {
        // It's probably unrecoverable, so don't bother caching the error.
        Py_DECREF(mod);
        Py_DECREF(key);
        return -1;
    }
    else if (loaded == NULL) {
        // It wasn't already loaded from file.
        loaded = PyModule_NewObject(&_Py_ID(__main__));
        if (loaded == NULL) {
            goto error;
        }
        PyObject *ns = _PyModule_GetDict(loaded);

        // We don't want to trigger "if __name__ == '__main__':",
        // so we use a bogus module name.
        PyObject *loaded_ns =
                    runpy_run_path(main->filename, "<fake __main__>");
        if (loaded_ns == NULL) {
            goto error;
        }
        int res = PyDict_Update(ns, loaded_ns);
        Py_DECREF(loaded_ns);
        if (res < 0) {
            goto error;
        }

        // Set the per-interpreter cache entry.
        if (PyDict_SetItem(interpns, key, loaded) < 0) {
            goto error;
        }
    }

    Py_DECREF(key);
    main->cached = (struct sync_module_result){
       .module = mod,
       .loaded = loaded,
    };
    return 0;

error:
    sync_module_capture_exc(tstate, main);
    Py_XDECREF(loaded);
    Py_DECREF(mod);
    Py_XDECREF(key);
    return -1;
}

#ifndef NDEBUG
static int
main_mod_matches(PyObject *expected)
{
    PyObject *mod = PyImport_GetModule(&_Py_ID(__main__));
    Py_XDECREF(mod);
    return mod == expected;
}
#endif

static int
apply_isolated_main(PyThreadState *tstate, struct sync_module *main)
{
    assert((main->cached.loaded == NULL) == (main->cached.loaded == NULL));
    if (main->cached.failed != NULL) {
        // It must have failed previously.
        assert(main->cached.loaded == NULL);
        _PyErr_SetRaisedException(tstate, main->cached.failed);
        return -1;
    }
    assert(main->cached.loaded != NULL);

    assert(main_mod_matches(main->cached.module));
    if (_PyImport_SetModule(&_Py_ID(__main__), main->cached.loaded) < 0) {
        sync_module_capture_exc(tstate, main);
        return -1;
    }
    return 0;
}

static void
restore_main(PyThreadState *tstate, struct sync_module *main)
{
    assert(main->cached.failed == NULL);
    assert(main->cached.module != NULL);
    assert(main->cached.loaded != NULL);
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    assert(main_mod_matches(main->cached.loaded));
    int res = _PyImport_SetModule(&_Py_ID(__main__), main->cached.module);
    assert(res == 0);
    if (res < 0) {
        PyErr_FormatUnraisable("Exception ignored while restoring __main__");
    }
    _PyErr_SetRaisedException(tstate, exc);
}


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

/* registry of {type -> _PyXIData_getdata_t} */

/* For now we use a global registry of shareable classes.
   An alternative would be to add a tp_* slot for a class's
   _PyXIData_getdata_t.  It would be simpler and more efficient. */

static void xid_lookup_init(_PyXIData_lookup_t *);
static void xid_lookup_fini(_PyXIData_lookup_t *);
struct _dlcontext;
static _PyXIData_getdata_t lookup_getdata(struct _dlcontext *, PyObject *);
#include "crossinterp_data_lookup.h"


/* lifecycle */

_PyXIData_t *
_PyXIData_New(void)
{
    _PyXIData_t *xid = PyMem_RawCalloc(1, sizeof(_PyXIData_t));
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
_xidata_init(_PyXIData_t *xidata)
{
    // If the value is being reused
    // then _xidata_clear() should have been called already.
    assert(xidata->data == NULL);
    assert(xidata->obj == NULL);
    *xidata = (_PyXIData_t){0};
    _PyXIData_INTERPID(xidata) = -1;
}

static inline void
_xidata_clear(_PyXIData_t *xidata)
{
    // _PyXIData_t only has two members that need to be
    // cleaned up, if set: "xidata" must be freed and "obj" must be decref'ed.
    // In both cases the original (owning) interpreter must be used,
    // which is the caller's responsibility to ensure.
    if (xidata->data != NULL) {
        if (xidata->free != NULL) {
            xidata->free(xidata->data);
        }
        xidata->data = NULL;
    }
    Py_CLEAR(xidata->obj);
}

void
_PyXIData_Init(_PyXIData_t *xidata,
               PyInterpreterState *interp,
               void *shared, PyObject *obj,
               xid_newobjfunc new_object)
{
    assert(xidata != NULL);
    assert(new_object != NULL);
    _xidata_init(xidata);
    xidata->data = shared;
    if (obj != NULL) {
        assert(interp != NULL);
        // released in _PyXIData_Clear()
        xidata->obj = Py_NewRef(obj);
    }
    // Ideally every object would know its owning interpreter.
    // Until then, we have to rely on the caller to identify it
    // (but we don't need it in all cases).
    _PyXIData_INTERPID(xidata) = (interp != NULL)
        ? PyInterpreterState_GetID(interp)
        : -1;
    xidata->new_object = new_object;
}

int
_PyXIData_InitWithSize(_PyXIData_t *xidata,
                       PyInterpreterState *interp,
                       const size_t size, PyObject *obj,
                       xid_newobjfunc new_object)
{
    assert(size > 0);
    // For now we always free the shared data in the same interpreter
    // where it was allocated, so the interpreter is required.
    assert(interp != NULL);
    _PyXIData_Init(xidata, interp, NULL, obj, new_object);
    xidata->data = PyMem_RawCalloc(1, size);
    if (xidata->data == NULL) {
        return -1;
    }
    xidata->free = PyMem_RawFree;
    return 0;
}

void
_PyXIData_Clear(PyInterpreterState *interp, _PyXIData_t *xidata)
{
    assert(xidata != NULL);
    // This must be called in the owning interpreter.
    assert(interp == NULL
           || _PyXIData_INTERPID(xidata) == -1
           || _PyXIData_INTERPID(xidata) == PyInterpreterState_GetID(interp));
    _xidata_clear(xidata);
}


/* getting cross-interpreter data */

static inline void
_set_xid_lookup_failure(PyThreadState *tstate, PyObject *obj, const char *msg,
                        PyObject *cause)
{
    if (msg != NULL) {
        assert(obj == NULL);
        set_notshareableerror(tstate, cause, 0, msg);
    }
    else if (obj == NULL) {
        msg = "object does not support cross-interpreter data";
        set_notshareableerror(tstate, cause, 0, msg);
    }
    else {
        msg = "%R does not support cross-interpreter data";
        format_notshareableerror(tstate, cause, 0, msg, obj);
    }
}


int
_PyObject_CheckXIData(PyThreadState *tstate, PyObject *obj)
{
    dlcontext_t ctx;
    if (get_lookup_context(tstate, &ctx) < 0) {
        return -1;
    }
    _PyXIData_getdata_t getdata = lookup_getdata(&ctx, obj);
    if (getdata.basic == NULL && getdata.fallback == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            _set_xid_lookup_failure(tstate, obj, NULL, NULL);
        }
        return -1;
    }
    return 0;
}

static int
_check_xidata(PyThreadState *tstate, _PyXIData_t *xidata)
{
    // xidata->data can be anything, including NULL, so we don't check it.

    // xidata->obj may be NULL, so we don't check it.

    if (_PyXIData_INTERPID(xidata) < 0) {
        PyErr_SetString(PyExc_SystemError, "missing interp");
        return -1;
    }

    if (xidata->new_object == NULL) {
        PyErr_SetString(PyExc_SystemError, "missing new_object func");
        return -1;
    }

    // xidata->free may be NULL, so we don't check it.

    return 0;
}

static int
_get_xidata(PyThreadState *tstate,
            PyObject *obj, xidata_fallback_t fallback, _PyXIData_t *xidata)
{
    PyInterpreterState *interp = tstate->interp;

    assert(xidata->data == NULL);
    assert(xidata->obj == NULL);
    if (xidata->data != NULL || xidata->obj != NULL) {
        _PyErr_SetString(tstate, PyExc_ValueError, "xidata not cleared");
        return -1;
    }

    // Call the "getdata" func for the object.
    dlcontext_t ctx;
    if (get_lookup_context(tstate, &ctx) < 0) {
        return -1;
    }
    Py_INCREF(obj);
    _PyXIData_getdata_t getdata = lookup_getdata(&ctx, obj);
    if (getdata.basic == NULL && getdata.fallback == NULL) {
        if (PyErr_Occurred()) {
            Py_DECREF(obj);
            return -1;
        }
        // Fall back to obj
        Py_DECREF(obj);
        if (!_PyErr_Occurred(tstate)) {
            _set_xid_lookup_failure(tstate, obj, NULL, NULL);
        }
        return -1;
    }
    int res = getdata.basic != NULL
        ? getdata.basic(tstate, obj, xidata)
        : getdata.fallback(tstate, obj, fallback, xidata);
    Py_DECREF(obj);
    if (res != 0) {
        PyObject *cause = _PyErr_GetRaisedException(tstate);
        assert(cause != NULL);
        _set_xid_lookup_failure(tstate, obj, NULL, cause);
        Py_XDECREF(cause);
        return -1;
    }

    // Fill in the blanks and validate the result.
    _PyXIData_INTERPID(xidata) = PyInterpreterState_GetID(interp);
    if (_check_xidata(tstate, xidata) != 0) {
        (void)_PyXIData_Release(xidata);
        return -1;
    }

    return 0;
}

int
_PyObject_GetXIDataNoFallback(PyThreadState *tstate,
                              PyObject *obj, _PyXIData_t *xidata)
{
    return _get_xidata(tstate, obj, _PyXIDATA_XIDATA_ONLY, xidata);
}

int
_PyObject_GetXIData(PyThreadState *tstate,
                    PyObject *obj, xidata_fallback_t fallback,
                    _PyXIData_t *xidata)
{
    switch (fallback) {
        case _PyXIDATA_XIDATA_ONLY:
            return _get_xidata(tstate, obj, fallback, xidata);
        case _PyXIDATA_FULL_FALLBACK:
            if (_get_xidata(tstate, obj, fallback, xidata) == 0) {
                return 0;
            }
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            if (PyFunction_Check(obj)) {
                if (_PyFunction_GetXIData(tstate, obj, xidata) == 0) {
                    Py_DECREF(exc);
                    return 0;
                }
                _PyErr_Clear(tstate);
            }
            // We could try _PyMarshal_GetXIData() but we won't for now.
            if (_PyPickle_GetXIData(tstate, obj, xidata) == 0) {
                Py_DECREF(exc);
                return 0;
            }
            // Raise the original exception.
            _PyErr_SetRaisedException(tstate, exc);
            return -1;
        default:
#ifdef Py_DEBUG
            Py_FatalError("unsupported xidata fallback option");
#endif
            _PyErr_SetString(tstate, PyExc_SystemError,
                             "unsupported xidata fallback option");
            return -1;
    }
}


/* pickle C-API */

struct _pickle_context {
    PyThreadState *tstate;
};

static PyObject *
_PyPickle_Dumps(struct _pickle_context *ctx, PyObject *obj)
{
    PyObject *dumps = PyImport_ImportModuleAttrString("pickle", "dumps");
    if (dumps == NULL) {
        return NULL;
    }
    PyObject *bytes = PyObject_CallOneArg(dumps, obj);
    Py_DECREF(dumps);
    return bytes;
}


struct _unpickle_context {
    PyThreadState *tstate;
    // We only special-case the __main__ module,
    // since other modules behave consistently.
    struct sync_module main;
};

static void
_unpickle_context_clear(struct _unpickle_context *ctx)
{
    sync_module_clear(&ctx->main);
}

static int
check_missing___main___attr(PyObject *exc)
{
    assert(!PyErr_Occurred());
    if (!PyErr_GivenExceptionMatches(exc, PyExc_AttributeError)) {
        return 0;
    }

    // Get the error message.
    PyObject *args = PyException_GetArgs(exc);
    if (args == NULL || args == Py_None || PyObject_Size(args) < 1) {
        assert(!PyErr_Occurred());
        return 0;
    }
    PyObject *msgobj = args;
    if (!PyUnicode_Check(msgobj)) {
        msgobj = PySequence_GetItem(args, 0);
        Py_DECREF(args);
        if (msgobj == NULL) {
            PyErr_Clear();
            return 0;
        }
    }
    const char *err = PyUnicode_AsUTF8(msgobj);

    // Check if it's a missing __main__ attr.
    int cmp = strncmp(err, "module '__main__' has no attribute '", 36);
    Py_DECREF(msgobj);
    return cmp == 0;
}

static PyObject *
_PyPickle_Loads(struct _unpickle_context *ctx, PyObject *pickled)
{
    PyThreadState *tstate = ctx->tstate;

    PyObject *exc = NULL;
    PyObject *loads = PyImport_ImportModuleAttrString("pickle", "loads");
    if (loads == NULL) {
        return NULL;
    }

    // Make an initial attempt to unpickle.
    PyObject *obj = PyObject_CallOneArg(loads, pickled);
    if (obj != NULL) {
        goto finally;
    }
    assert(_PyErr_Occurred(tstate));
    if (ctx == NULL) {
        goto finally;
    }
    exc = _PyErr_GetRaisedException(tstate);
    if (!check_missing___main___attr(exc)) {
        goto finally;
    }

    // Temporarily swap in a fake __main__ loaded from the original
    // file and cached.  Note that functions will use the cached ns
    // for __globals__, // not the actual module.
    if (ensure_isolated_main(tstate, &ctx->main) < 0) {
        goto finally;
    }
    if (apply_isolated_main(tstate, &ctx->main) < 0) {
        goto finally;
    }

    // Try to unpickle once more.
    obj = PyObject_CallOneArg(loads, pickled);
    restore_main(tstate, &ctx->main);
    if (obj == NULL) {
        goto finally;
    }
    Py_CLEAR(exc);

finally:
    if (exc != NULL) {
        if (_PyErr_Occurred(tstate)) {
            sync_module_capture_exc(tstate, &ctx->main);
        }
        // We restore the original exception.
        // It might make sense to chain it (__context__).
        _PyErr_SetRaisedException(tstate, exc);
    }
    Py_DECREF(loads);
    return obj;
}


/* pickle wrapper */

struct _pickle_xid_context {
    // __main__.__file__
    struct {
        const char *utf8;
        size_t len;
        char _utf8[MAXPATHLEN+1];
    } mainfile;
};

static int
_set_pickle_xid_context(PyThreadState *tstate, struct _pickle_xid_context *ctx)
{
    // Set mainfile if possible.
    Py_ssize_t len = _Py_GetMainfile(ctx->mainfile._utf8, MAXPATHLEN);
    if (len < 0) {
        // For now we ignore any exceptions.
        PyErr_Clear();
    }
    else if (len > 0) {
        ctx->mainfile.utf8 = ctx->mainfile._utf8;
        ctx->mainfile.len = (size_t)len;
    }

    return 0;
}


struct _shared_pickle_data {
    _PyBytes_data_t pickled;  // Must be first if we use _PyBytes_FromXIData().
    struct _pickle_xid_context ctx;
};

PyObject *
_PyPickle_LoadFromXIData(_PyXIData_t *xidata)
{
    PyThreadState *tstate = _PyThreadState_GET();
    struct _shared_pickle_data *shared =
                            (struct _shared_pickle_data *)xidata->data;
    // We avoid copying the pickled data by wrapping it in a memoryview.
    // The alternative is to get a bytes object using _PyBytes_FromXIData().
    PyObject *pickled = PyMemoryView_FromMemory(
            (char *)shared->pickled.bytes, shared->pickled.len, PyBUF_READ);
    if (pickled == NULL) {
        return NULL;
    }

    // Unpickle the object.
    struct _unpickle_context ctx = {
        .tstate = tstate,
        .main = {
            .filename = shared->ctx.mainfile.utf8,
        },
    };
    PyObject *obj = _PyPickle_Loads(&ctx, pickled);
    Py_DECREF(pickled);
    _unpickle_context_clear(&ctx);
    if (obj == NULL) {
        PyObject *cause = _PyErr_GetRaisedException(tstate);
        assert(cause != NULL);
        _set_xid_lookup_failure(
                    tstate, NULL, "object could not be unpickled", cause);
        Py_DECREF(cause);
    }
    return obj;
}


int
_PyPickle_GetXIData(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    // Pickle the object.
    struct _pickle_context ctx = {
        .tstate = tstate,
    };
    PyObject *bytes = _PyPickle_Dumps(&ctx, obj);
    if (bytes == NULL) {
        PyObject *cause = _PyErr_GetRaisedException(tstate);
        assert(cause != NULL);
        _set_xid_lookup_failure(
                    tstate, NULL, "object could not be pickled", cause);
        Py_DECREF(cause);
        return -1;
    }

    // If we had an "unwrapper" mechnanism, we could call
    // _PyObject_GetXIData() on the bytes object directly and add
    // a simple unwrapper to call pickle.loads() on the bytes.
    size_t size = sizeof(struct _shared_pickle_data);
    struct _shared_pickle_data *shared =
            (struct _shared_pickle_data *)_PyBytes_GetXIDataWrapped(
                    tstate, bytes, size, _PyPickle_LoadFromXIData, xidata);
    Py_DECREF(bytes);
    if (shared == NULL) {
        return -1;
    }

    // If it mattered, we could skip getting __main__.__file__
    // when "__main__" doesn't show up in the pickle bytes.
    if (_set_pickle_xid_context(tstate, &shared->ctx) < 0) {
        _xidata_clear(xidata);
        return -1;
    }

    return 0;
}


/* marshal wrapper */

PyObject *
_PyMarshal_ReadObjectFromXIData(_PyXIData_t *xidata)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBytes_data_t *shared = (_PyBytes_data_t *)xidata->data;
    PyObject *obj = PyMarshal_ReadObjectFromString(shared->bytes, shared->len);
    if (obj == NULL) {
        PyObject *cause = _PyErr_GetRaisedException(tstate);
        assert(cause != NULL);
        _set_xid_lookup_failure(
                    tstate, NULL, "object could not be unmarshalled", cause);
        Py_DECREF(cause);
        return NULL;
    }
    return obj;
}

int
_PyMarshal_GetXIData(PyThreadState *tstate, PyObject *obj, _PyXIData_t *xidata)
{
    PyObject *bytes = PyMarshal_WriteObjectToString(obj, Py_MARSHAL_VERSION);
    if (bytes == NULL) {
        PyObject *cause = _PyErr_GetRaisedException(tstate);
        assert(cause != NULL);
        _set_xid_lookup_failure(
                    tstate, NULL, "object could not be marshalled", cause);
        Py_DECREF(cause);
        return -1;
    }
    size_t size = sizeof(_PyBytes_data_t);
    _PyBytes_data_t *shared = _PyBytes_GetXIDataWrapped(
            tstate, bytes, size, _PyMarshal_ReadObjectFromXIData, xidata);
    Py_DECREF(bytes);
    if (shared == NULL) {
        return -1;
    }
    return 0;
}


/* script wrapper */

static int
verify_script(PyThreadState *tstate, PyCodeObject *co, int checked, int pure)
{
    // Make sure it isn't a closure and (optionally) doesn't use globals.
    PyObject *builtins = NULL;
    if (pure) {
        builtins = _PyEval_GetBuiltins(tstate);
        assert(builtins != NULL);
    }
    if (checked) {
        assert(_PyCode_VerifyStateless(tstate, co, NULL, NULL, builtins) == 0);
    }
    else if (_PyCode_VerifyStateless(tstate, co, NULL, NULL, builtins) < 0) {
        return -1;
    }
    // Make sure it doesn't have args.
    if (co->co_argcount > 0
        || co->co_posonlyargcount > 0
        || co->co_kwonlyargcount > 0
        || co->co_flags & (CO_VARARGS | CO_VARKEYWORDS))
    {
        _PyErr_SetString(tstate, PyExc_ValueError,
                         "code with args not supported");
        return -1;
    }
    // Make sure it doesn't return anything.
    if (!_PyCode_ReturnsOnlyNone(co)) {
        _PyErr_SetString(tstate, PyExc_ValueError,
                         "code that returns a value is not a script");
        return -1;
    }
    return 0;
}

static int
get_script_xidata(PyThreadState *tstate, PyObject *obj, int pure,
                  _PyXIData_t *xidata)
{
    // Get the corresponding code object.
    PyObject *code = NULL;
    int checked = 0;
    if (PyCode_Check(obj)) {
        code = obj;
        Py_INCREF(code);
    }
    else if (PyFunction_Check(obj)) {
        code = PyFunction_GET_CODE(obj);
        assert(code != NULL);
        Py_INCREF(code);
        if (pure) {
            if (_PyFunction_VerifyStateless(tstate, obj) < 0) {
                goto error;
            }
            checked = 1;
        }
    }
    else {
        const char *filename = "<script>";
        int optimize = 0;
        PyCompilerFlags cf = _PyCompilerFlags_INIT;
        cf.cf_flags = PyCF_SOURCE_IS_UTF8;
        PyObject *ref = NULL;
        const char *script = _Py_SourceAsString(obj, "???", "???", &cf, &ref);
        if (script == NULL) {
            if (!_PyObject_SupportedAsScript(obj)) {
                // We discard the raised exception.
                _PyErr_Format(tstate, PyExc_TypeError,
                              "unsupported script %R", obj);
            }
            goto error;
        }
#ifdef Py_GIL_DISABLED
        // Don't immortalize code constants to avoid memory leaks.
        ((_PyThreadStateImpl *)tstate)->suppress_co_const_immortalization++;
#endif
        code = Py_CompileStringExFlags(
                    script, filename, Py_file_input, &cf, optimize);
#ifdef Py_GIL_DISABLED
        ((_PyThreadStateImpl *)tstate)->suppress_co_const_immortalization--;
#endif
        Py_XDECREF(ref);
        if (code == NULL) {
            goto error;
        }
        // Compiled text can't have args or any return statements,
        // nor be a closure.  It can use globals though.
        if (!pure) {
            // We don't need to check for globals either.
            checked = 1;
        }
    }

    // Make sure it's actually a script.
    if (verify_script(tstate, (PyCodeObject *)code, checked, pure) < 0) {
        goto error;
    }

    // Convert the code object.
    int res = _PyCode_GetXIData(tstate, code, xidata);
    Py_DECREF(code);
    if (res < 0) {
        return -1;
    }
    return 0;

error:
    Py_XDECREF(code);
    PyObject *cause = _PyErr_GetRaisedException(tstate);
    assert(cause != NULL);
    _set_xid_lookup_failure(
                tstate, NULL, "object not a valid script", cause);
    Py_DECREF(cause);
    return -1;
}

int
_PyCode_GetScriptXIData(PyThreadState *tstate,
                        PyObject *obj, _PyXIData_t *xidata)
{
    return get_script_xidata(tstate, obj, 0, xidata);
}

int
_PyCode_GetPureScriptXIData(PyThreadState *tstate,
                            PyObject *obj, _PyXIData_t *xidata)
{
    return get_script_xidata(tstate, obj, 1, xidata);
}


/* using cross-interpreter data */

PyObject *
_PyXIData_NewObject(_PyXIData_t *xidata)
{
    return xidata->new_object(xidata);
}

static int
_call_clear_xidata(void *data)
{
    _xidata_clear((_PyXIData_t *)data);
    return 0;
}

static int
_xidata_release(_PyXIData_t *xidata, int rawfree)
{
    if ((xidata->data == NULL || xidata->free == NULL) && xidata->obj == NULL) {
        // Nothing to release!
        if (rawfree) {
            PyMem_RawFree(xidata);
        }
        else {
            xidata->data = NULL;
        }
        return 0;
    }

    // Switch to the original interpreter.
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(
                                        _PyXIData_INTERPID(xidata));
    if (interp == NULL) {
        // The interpreter was already destroyed.
        // This function shouldn't have been called.
        // XXX Someone leaked some memory...
        assert(PyErr_Occurred());
        if (rawfree) {
            PyMem_RawFree(xidata);
        }
        return -1;
    }

    // "Release" the data and/or the object.
    if (rawfree) {
        return _Py_CallInInterpreterAndRawFree(interp, _call_clear_xidata, xidata);
    }
    else {
        return _Py_CallInInterpreter(interp, _call_clear_xidata, xidata);
    }
}

int
_PyXIData_Release(_PyXIData_t *xidata)
{
    return _xidata_release(xidata, 0);
}

int
_PyXIData_ReleaseAndRawFree(_PyXIData_t *xidata)
{
    return _xidata_release(xidata, 1);
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
_release_xid_data(_PyXIData_t *xidata, int rawfree)
{
    PyObject *exc = PyErr_GetRaisedException();
    int res = rawfree
        ? _PyXIData_Release(xidata)
        : _PyXIData_ReleaseAndRawFree(xidata);
    if (res < 0) {
        /* The owning interpreter is already destroyed. */
        _PyXIData_Clear(NULL, xidata);
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

static int
excinfo_is_set(_PyXI_excinfo *info)
{
    return info->type.name != NULL || info->msg != NULL;
}

static void
_PyXI_excinfo_clear(_PyXI_excinfo *info)
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
        _PyXI_excinfo_clear(info);
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
    _PyXI_excinfo_clear(info);
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
    _PyXI_excinfo_clear(info);
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
        else {
            PyErr_SetObject(exctype, tbexc);
            Py_DECREF(tbexc);
            return;
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


_PyXI_excinfo *
_PyXI_NewExcInfo(PyObject *exc)
{
    assert(!PyErr_Occurred());
    if (exc == NULL || exc == Py_None) {
        PyErr_SetString(PyExc_ValueError, "missing exc");
        return NULL;
    }
    _PyXI_excinfo *info = PyMem_RawCalloc(1, sizeof(_PyXI_excinfo));
    if (info == NULL) {
        return NULL;
    }
    const char *failure;
    if (PyExceptionInstance_Check(exc) || PyExceptionClass_Check(exc)) {
        failure = _PyXI_excinfo_InitFromException(info, exc);
    }
    else {
        failure = _PyXI_excinfo_InitFromObject(info, exc);
    }
    if (failure != NULL) {
        PyMem_RawFree(info);
        set_exc_with_cause(PyExc_Exception, failure);
        return NULL;
    }
    return info;
}

void
_PyXI_FreeExcInfo(_PyXI_excinfo *info)
{
    _PyXI_excinfo_clear(info);
    PyMem_RawFree(info);
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


/***************************/
/* short-term data sharing */
/***************************/

/* error codes */

static int
_PyXI_ApplyErrorCode(_PyXI_errcode code, PyInterpreterState *interp)
{
    PyThreadState *tstate = _PyThreadState_GET();

    assert(!PyErr_Occurred());
    assert(code != _PyXI_ERR_NO_ERROR);
    assert(code != _PyXI_ERR_UNCAUGHT_EXCEPTION);
    switch (code) {
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
    case _PyXI_ERR_PRESERVE_FAILURE:
        PyErr_SetString(PyExc_InterpreterError,
                        "failed to preserve objects across session");
        break;
    case _PyXI_ERR_EXC_PROPAGATION_FAILURE:
        PyErr_SetString(PyExc_InterpreterError,
                        "failed to transfer exception between interpreters");
        break;
    case _PyXI_ERR_NOT_SHAREABLE:
        _set_xid_lookup_failure(tstate, NULL, NULL, NULL);
        break;
    default:
#ifdef Py_DEBUG
        Py_FatalError("unsupported error code");
#else
        PyErr_Format(PyExc_RuntimeError, "unsupported error code %d", code);
#endif
    }
    assert(PyErr_Occurred());
    return -1;
}

/* basic failure info */

struct xi_failure {
    // The kind of error to propagate.
    _PyXI_errcode code;
    // The propagated message.
    const char *msg;
    int msg_owned;
};  // _PyXI_failure

#define XI_FAILURE_INIT (_PyXI_failure){ .code = _PyXI_ERR_NO_ERROR }

static void
clear_xi_failure(_PyXI_failure *failure)
{
    if (failure->msg != NULL && failure->msg_owned) {
        PyMem_RawFree((void*)failure->msg);
    }
    *failure = XI_FAILURE_INIT;
}

static void
copy_xi_failure(_PyXI_failure *dest, _PyXI_failure *src)
{
    *dest = *src;
    dest->msg_owned = 0;
}

_PyXI_failure *
_PyXI_NewFailure(void)
{
    _PyXI_failure *failure = PyMem_RawMalloc(sizeof(_PyXI_failure));
    if (failure == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    *failure = XI_FAILURE_INIT;
    return failure;
}

void
_PyXI_FreeFailure(_PyXI_failure *failure)
{
    clear_xi_failure(failure);
    PyMem_RawFree(failure);
}

_PyXI_errcode
_PyXI_GetFailureCode(_PyXI_failure *failure)
{
    if (failure == NULL) {
        return _PyXI_ERR_NO_ERROR;
    }
    return failure->code;
}

void
_PyXI_InitFailureUTF8(_PyXI_failure *failure,
                      _PyXI_errcode code, const char *msg)
{
    *failure = (_PyXI_failure){
        .code = code,
        .msg = msg,
        .msg_owned = 0,
    };
}

int
_PyXI_InitFailure(_PyXI_failure *failure, _PyXI_errcode code, PyObject *obj)
{
    PyObject *msgobj = PyObject_Str(obj);
    if (msgobj == NULL) {
        return -1;
    }
    // This will leak if not paired with clear_xi_failure().
    // That happens automatically in _capture_current_exception().
    const char *msg = _copy_string_obj_raw(msgobj, NULL);
    Py_DECREF(msgobj);
    if (PyErr_Occurred()) {
        return -1;
    }
    *failure = (_PyXI_failure){
        .code = code,
        .msg = msg,
        .msg_owned = 1,
    };
    return 0;
}

/* shared exceptions */

typedef struct {
    // The originating interpreter.
    PyInterpreterState *interp;
    // The error to propagate, if different from the uncaught exception.
    _PyXI_failure *override;
    _PyXI_failure _override;
    // The exception information to propagate, if applicable.
    // This is populated only for some error codes,
    // but always for _PyXI_ERR_UNCAUGHT_EXCEPTION.
    _PyXI_excinfo uncaught;
} _PyXI_error;

static void
xi_error_clear(_PyXI_error *err)
{
    err->interp = NULL;
    if (err->override != NULL) {
        clear_xi_failure(err->override);
    }
    _PyXI_excinfo_clear(&err->uncaught);
}

static int
xi_error_is_set(_PyXI_error *error)
{
    if (error->override != NULL) {
        assert(error->override->code != _PyXI_ERR_NO_ERROR);
        assert(error->override->code != _PyXI_ERR_UNCAUGHT_EXCEPTION
               || excinfo_is_set(&error->uncaught));
        return 1;
    }
    return excinfo_is_set(&error->uncaught);
}

static int
xi_error_has_override(_PyXI_error *err)
{
    if (err->override == NULL) {
        return 0;
    }
    return (err->override->code != _PyXI_ERR_NO_ERROR
            && err->override->code != _PyXI_ERR_UNCAUGHT_EXCEPTION);
}

static PyObject *
xi_error_resolve_current_exc(PyThreadState *tstate,
                             _PyXI_failure *override)
{
    assert(override == NULL || override->code != _PyXI_ERR_NO_ERROR);

    PyObject *exc = _PyErr_GetRaisedException(tstate);
    if (exc == NULL) {
        assert(override == NULL
               || override->code != _PyXI_ERR_UNCAUGHT_EXCEPTION);
    }
    else if (override == NULL) {
        // This is equivalent to _PyXI_ERR_UNCAUGHT_EXCEPTION.
    }
    else if (override->code == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // We want to actually capture the current exception.
    }
    else if (exc != NULL) {
        // It might make sense to do similarly for other codes.
        if (override->code == _PyXI_ERR_ALREADY_RUNNING) {
            // We don't need the exception info.
            Py_CLEAR(exc);
        }
        // ...else we want to actually capture the current exception.
    }
    return exc;
}

static void
xi_error_set_override(PyThreadState *tstate, _PyXI_error *err,
                      _PyXI_failure *override)
{
    assert(err->override == NULL);
    assert(override != NULL);
    assert(override->code != _PyXI_ERR_NO_ERROR);
    // Use xi_error_set_exc() instead of setting _PyXI_ERR_UNCAUGHT_EXCEPTION..
    assert(override->code != _PyXI_ERR_UNCAUGHT_EXCEPTION);
    err->override = &err->_override;
    // The caller still owns override->msg.
    copy_xi_failure(&err->_override, override);
    err->interp = tstate->interp;
}

static void
xi_error_set_override_code(PyThreadState *tstate, _PyXI_error *err,
                           _PyXI_errcode code)
{
    _PyXI_failure override = XI_FAILURE_INIT;
    override.code = code;
    xi_error_set_override(tstate, err, &override);
}

static const char *
xi_error_set_exc(PyThreadState *tstate, _PyXI_error *err, PyObject *exc)
{
    assert(!_PyErr_Occurred(tstate));
    assert(!xi_error_is_set(err));
    assert(err->override == NULL);
    assert(err->interp == NULL);
    assert(exc != NULL);
    const char *failure =
                _PyXI_excinfo_InitFromException(&err->uncaught, exc);
    if (failure != NULL) {
        // We failed to initialize err->uncaught.
        // XXX Print the excobj/traceback?  Emit a warning?
        // XXX Print the current exception/traceback?
        if (_PyErr_ExceptionMatches(tstate, PyExc_MemoryError)) {
            xi_error_set_override_code(tstate, err, _PyXI_ERR_NO_MEMORY);
        }
        else {
            xi_error_set_override_code(tstate, err, _PyXI_ERR_OTHER);
        }
        PyErr_Clear();
    }
    return failure;
}

static PyObject *
_PyXI_ApplyError(_PyXI_error *error, const char *failure)
{
    PyThreadState *tstate = PyThreadState_Get();

    if (failure != NULL) {
        xi_error_clear(error);
        return NULL;
    }

    _PyXI_errcode code = _PyXI_ERR_UNCAUGHT_EXCEPTION;
    if (error->override != NULL) {
        code = error->override->code;
        assert(code != _PyXI_ERR_NO_ERROR);
    }

    if (code == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
        // We will raise an exception that proxies the propagated exception.
       return _PyXI_excinfo_AsObject(&error->uncaught);
    }
    else if (code == _PyXI_ERR_NOT_SHAREABLE) {
        // Propagate the exception directly.
        assert(!_PyErr_Occurred(tstate));
        PyObject *cause = NULL;
        if (excinfo_is_set(&error->uncaught)) {
            // Maybe instead set a PyExc_ExceptionSnapshot as __cause__?
            // That type doesn't exist currently
            // but would look like interpreters.ExecutionFailed.
            _PyXI_excinfo_Apply(&error->uncaught, PyExc_Exception);
            cause = _PyErr_GetRaisedException(tstate);
        }
        const char *msg = error->override != NULL
            ? error->override->msg
            : error->uncaught.msg;
        _set_xid_lookup_failure(tstate, NULL, msg, cause);
        Py_XDECREF(cause);
    }
    else {
        // Raise an exception corresponding to the code.
        (void)_PyXI_ApplyErrorCode(code, error->interp);
        assert(error->override == NULL || error->override->msg == NULL);
        if (excinfo_is_set(&error->uncaught)) {
            // __context__ will be set to a proxy of the propagated exception.
            // (or use PyExc_ExceptionSnapshot like _PyXI_ERR_NOT_SHAREABLE?)
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            _PyXI_excinfo_Apply(&error->uncaught, PyExc_InterpreterError);
            PyObject *exc2 = _PyErr_GetRaisedException(tstate);
            PyException_SetContext(exc, exc2);
            _PyErr_SetRaisedException(tstate, exc);
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
    _PyXIData_t *xidata;
    // We could have a "PyXIData _data" field, so it would
    // be allocated as part of the item and avoid an extra allocation.
    // However, doing so adds a bunch of complexity because we must
    // ensure the item isn't freed before a pending call might happen
    // in a different interpreter to release the XI data.
} _PyXI_namespace_item;

#ifndef NDEBUG
static int
_sharednsitem_is_initialized(_PyXI_namespace_item *item)
{
    if (item->name != NULL) {
        return 1;
    }
    return 0;
}
#endif

static int
_sharednsitem_init(_PyXI_namespace_item *item, PyObject *key)
{
    item->name = _copy_string_obj_raw(key, NULL);
    if (item->name == NULL) {
        assert(!_sharednsitem_is_initialized(item));
        return -1;
    }
    item->xidata = NULL;
    assert(_sharednsitem_is_initialized(item));
    return 0;
}

static int
_sharednsitem_has_value(_PyXI_namespace_item *item, int64_t *p_interpid)
{
    if (item->xidata == NULL) {
        return 0;
    }
    if (p_interpid != NULL) {
        *p_interpid = _PyXIData_INTERPID(item->xidata);
    }
    return 1;
}

static int
_sharednsitem_set_value(_PyXI_namespace_item *item, PyObject *value,
                        xidata_fallback_t fallback)
{
    assert(_sharednsitem_is_initialized(item));
    assert(item->xidata == NULL);
    item->xidata = _PyXIData_New();
    if (item->xidata == NULL) {
        return -1;
    }
    PyThreadState *tstate = PyThreadState_Get();
    if (_PyObject_GetXIData(tstate, value, fallback, item->xidata) < 0) {
        PyMem_RawFree(item->xidata);
        item->xidata = NULL;
        // The caller may want to propagate PyExc_NotShareableError
        // if currently switched between interpreters.
        return -1;
    }
    return 0;
}

static void
_sharednsitem_clear_value(_PyXI_namespace_item *item)
{
    _PyXIData_t *xidata = item->xidata;
    if (xidata != NULL) {
        item->xidata = NULL;
        int rawfree = 1;
        (void)_release_xid_data(xidata, rawfree);
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
_sharednsitem_copy_from_ns(struct _sharednsitem *item, PyObject *ns,
                           xidata_fallback_t fallback)
{
    assert(item->name != NULL);
    assert(item->xidata == NULL);
    PyObject *value = PyDict_GetItemString(ns, item->name);  // borrowed
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        // When applied, this item will be set to the default (or fail).
        return 0;
    }
    if (_sharednsitem_set_value(item, value, fallback) < 0) {
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
    if (item->xidata != NULL) {
        value = _PyXIData_NewObject(item->xidata);
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


typedef struct {
    Py_ssize_t maxitems;
    Py_ssize_t numnames;
    Py_ssize_t numvalues;
    _PyXI_namespace_item items[1];
} _PyXI_namespace;

#ifndef NDEBUG
static int
_sharedns_check_counts(_PyXI_namespace *ns)
{
    if (ns->maxitems <= 0) {
        return 0;
    }
    if (ns->numnames < 0) {
        return 0;
    }
    if (ns->numnames > ns->maxitems) {
        return 0;
    }
    if (ns->numvalues < 0) {
        return 0;
    }
    if (ns->numvalues > ns->numnames) {
        return 0;
    }
    return 1;
}

static int
_sharedns_check_consistency(_PyXI_namespace *ns)
{
    if (!_sharedns_check_counts(ns)) {
        return 0;
    }

    Py_ssize_t i = 0;
    _PyXI_namespace_item *item;
    if (ns->numvalues > 0) {
        item = &ns->items[0];
        if (!_sharednsitem_is_initialized(item)) {
            return 0;
        }
        int64_t interpid0 = -1;
        if (!_sharednsitem_has_value(item, &interpid0)) {
            return 0;
        }
        i += 1;
        for (; i < ns->numvalues; i++) {
            item = &ns->items[i];
            if (!_sharednsitem_is_initialized(item)) {
                return 0;
            }
            int64_t interpid = -1;
            if (!_sharednsitem_has_value(item, &interpid)) {
                return 0;
            }
            if (interpid != interpid0) {
                return 0;
            }
        }
    }
    for (; i < ns->numnames; i++) {
        item = &ns->items[i];
        if (!_sharednsitem_is_initialized(item)) {
            return 0;
        }
        if (_sharednsitem_has_value(item, NULL)) {
            return 0;
        }
    }
    for (; i < ns->maxitems; i++) {
        item = &ns->items[i];
        if (_sharednsitem_is_initialized(item)) {
            return 0;
        }
        if (_sharednsitem_has_value(item, NULL)) {
            return 0;
        }
    }
    return 1;
}
#endif

static _PyXI_namespace *
_sharedns_alloc(Py_ssize_t maxitems)
{
    if (maxitems < 0) {
        if (!PyErr_Occurred()) {
            PyErr_BadInternalCall();
        }
        return NULL;
    }
    else if (maxitems == 0) {
        PyErr_SetString(PyExc_ValueError, "empty namespaces not allowed");
        return NULL;
    }

    // Check for overflow.
    size_t fixedsize = sizeof(_PyXI_namespace) - sizeof(_PyXI_namespace_item);
    if ((size_t)maxitems >
        ((size_t)PY_SSIZE_T_MAX - fixedsize) / sizeof(_PyXI_namespace_item))
    {
        PyErr_NoMemory();
        return NULL;
    }

    // Allocate the value, including items.
    size_t size = fixedsize + sizeof(_PyXI_namespace_item) * maxitems;

    _PyXI_namespace *ns = PyMem_RawCalloc(size, 1);
    if (ns == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    ns->maxitems = maxitems;
    assert(_sharedns_check_consistency(ns));
    return ns;
}

static void
_sharedns_free(_PyXI_namespace *ns)
{
    // If we weren't always dynamically allocating the cross-interpreter
    // data in each item then we would need to use a pending call
    // to call _sharedns_free(), to avoid the race between freeing
    // the shared namespace and releasing the XI data.
    assert(_sharedns_check_counts(ns));
    Py_ssize_t i = 0;
    _PyXI_namespace_item *item;
    if (ns->numvalues > 0) {
        // One or more items may have interpreter-specific data.
#ifndef NDEBUG
        int64_t interpid = PyInterpreterState_GetID(PyInterpreterState_Get());
        int64_t interpid_i;
#endif
        for (; i < ns->numvalues; i++) {
            item = &ns->items[i];
            assert(_sharednsitem_is_initialized(item));
            // While we do want to ensure consistency across items,
            // technically they don't need to match the current
            // interpreter.  However, we keep the constraint for
            // simplicity, by giving _PyXI_FreeNamespace() the exclusive
            // responsibility of dealing with the owning interpreter.
            assert(_sharednsitem_has_value(item, &interpid_i));
            assert(interpid_i == interpid);
            _sharednsitem_clear(item);
        }
    }
    for (; i < ns->numnames; i++) {
        item = &ns->items[i];
        assert(_sharednsitem_is_initialized(item));
        assert(!_sharednsitem_has_value(item, NULL));
        _sharednsitem_clear(item);
    }
#ifndef NDEBUG
    for (; i < ns->maxitems; i++) {
        item = &ns->items[i];
        assert(!_sharednsitem_is_initialized(item));
        assert(!_sharednsitem_has_value(item, NULL));
    }
#endif

    PyMem_RawFree(ns);
}

static _PyXI_namespace *
_create_sharedns(PyObject *names)
{
    assert(names != NULL);
    Py_ssize_t numnames = PyDict_CheckExact(names)
        ? PyDict_Size(names)
        : PySequence_Size(names);

    _PyXI_namespace *ns = _sharedns_alloc(numnames);
    if (ns == NULL) {
        return NULL;
    }
    _PyXI_namespace_item *items = ns->items;

    // Fill in the names.
    if (PyDict_CheckExact(names)) {
        Py_ssize_t i = 0;
        Py_ssize_t pos = 0;
        PyObject *name;
        while(PyDict_Next(names, &pos, &name, NULL)) {
            if (_sharednsitem_init(&items[i], name) < 0) {
                goto error;
            }
            ns->numnames += 1;
            i += 1;
        }
    }
    else if (PySequence_Check(names)) {
        for (Py_ssize_t i = 0; i < numnames; i++) {
            PyObject *name = PySequence_GetItem(names, i);
            if (name == NULL) {
                goto error;
            }
            int res = _sharednsitem_init(&items[i], name);
            Py_DECREF(name);
            if (res < 0) {
                goto error;
            }
            ns->numnames += 1;
        }
    }
    else {
        PyErr_SetString(PyExc_NotImplementedError,
                        "non-sequence namespace not supported");
        goto error;
    }
    assert(ns->numnames == ns->maxitems);
    return ns;

error:
    _sharedns_free(ns);
    return NULL;
}

static void _propagate_not_shareable_error(PyThreadState *,
                                           _PyXI_failure *);

static int
_fill_sharedns(_PyXI_namespace *ns, PyObject *nsobj,
               xidata_fallback_t fallback, _PyXI_failure *p_err)
{
    // All items are expected to be shareable.
    assert(_sharedns_check_counts(ns));
    assert(ns->numnames == ns->maxitems);
    assert(ns->numvalues == 0);
    PyThreadState *tstate = PyThreadState_Get();
    for (Py_ssize_t i=0; i < ns->maxitems; i++) {
        if (_sharednsitem_copy_from_ns(&ns->items[i], nsobj, fallback) < 0) {
            if (p_err != NULL) {
                _propagate_not_shareable_error(tstate, p_err);
            }
            // Clear out the ones we set so far.
            for (Py_ssize_t j=0; j < i; j++) {
                _sharednsitem_clear_value(&ns->items[j]);
                ns->numvalues -= 1;
            }
            return -1;
        }
        ns->numvalues += 1;
    }
    return 0;
}

static int
_sharedns_free_pending(void *data)
{
    _sharedns_free((_PyXI_namespace *)data);
    return 0;
}

static void
_destroy_sharedns(_PyXI_namespace *ns)
{
    assert(_sharedns_check_counts(ns));
    assert(ns->numnames == ns->maxitems);
    if (ns->numvalues == 0) {
        _sharedns_free(ns);
        return;
    }

    int64_t interpid0;
    if (!_sharednsitem_has_value(&ns->items[0], &interpid0)) {
        // This shouldn't have been possible.
        // We can deal with it in _sharedns_free().
        _sharedns_free(ns);
        return;
    }
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(interpid0);
    if (interp == PyInterpreterState_Get()) {
        _sharedns_free(ns);
        return;
    }

    // One or more items may have interpreter-specific data.
    // Currently the xidata for each value is dynamically allocated,
    // so technically we don't need to worry about that.
    // However, explicitly adding a pending call here is simpler.
    (void)_Py_CallInInterpreter(interp, _sharedns_free_pending, ns);
}

static int
_apply_sharedns(_PyXI_namespace *ns, PyObject *nsobj, PyObject *dflt)
{
    for (Py_ssize_t i=0; i < ns->maxitems; i++) {
        if (_sharednsitem_apply(&ns->items[i], nsobj, dflt) != 0) {
            return -1;
        }
    }
    return 0;
}


/*********************************/
/* switched-interpreter sessions */
/*********************************/

struct xi_session {
#define SESSION_UNUSED 0
#define SESSION_ACTIVE 1
    int status;
    int switched;

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

    // This is a dict of objects that will be available (via sharing)
    // once the session exits.  Do not access this directly; use
    // _PyXI_Preserve() and _PyXI_GetPreserved() instead;
    PyObject *_preserved;
};

_PyXI_session *
_PyXI_NewSession(void)
{
    _PyXI_session *session = PyMem_RawCalloc(1, sizeof(_PyXI_session));
    if (session == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    return session;
}

void
_PyXI_FreeSession(_PyXI_session *session)
{
    assert(session->status == SESSION_UNUSED);
    PyMem_RawFree(session);
}


static inline int
_session_is_active(_PyXI_session *session)
{
    return session->status == SESSION_ACTIVE;
}


/* enter/exit a cross-interpreter session */

static void
_enter_session(_PyXI_session *session, PyInterpreterState *interp)
{
    // Set here and cleared in _exit_session().
    assert(session->status == SESSION_UNUSED);
    assert(!session->own_init_tstate);
    assert(session->init_tstate == NULL);
    assert(session->prev_tstate == NULL);
    // Set elsewhere and cleared in _exit_session().
    assert(!session->running);
    assert(session->main_ns == NULL);

    // Switch to interpreter.
    PyThreadState *tstate = PyThreadState_Get();
    PyThreadState *prev = tstate;
    int same_interp = (interp == tstate->interp);
    if (!same_interp) {
        tstate = _PyThreadState_NewBound(interp, _PyThreadState_WHENCE_EXEC);
        // XXX Possible GILState issues?
        PyThreadState *swapped = PyThreadState_Swap(tstate);
        assert(swapped == prev);
        (void)swapped;
    }

    *session = (_PyXI_session){
        .status = SESSION_ACTIVE,
        .switched = !same_interp,
        .init_tstate = tstate,
        .prev_tstate = prev,
        .own_init_tstate = !same_interp,
    };
}

static void
_exit_session(_PyXI_session *session)
{
    PyThreadState *tstate = session->init_tstate;
    assert(tstate != NULL);
    assert(PyThreadState_Get() == tstate);
    assert(!_PyErr_Occurred(tstate));

    // Release any of the entered interpreters resources.
    Py_CLEAR(session->main_ns);
    Py_CLEAR(session->_preserved);

    // Ensure this thread no longer owns __main__.
    if (session->running) {
        _PyInterpreterState_SetNotRunningMain(tstate->interp);
        assert(!_PyErr_Occurred(tstate));
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

    *session = (_PyXI_session){0};
}

static void
_propagate_not_shareable_error(PyThreadState *tstate,
                               _PyXI_failure *override)
{
    assert(override != NULL);
    PyObject *exctype = get_notshareableerror_type(tstate);
    if (exctype == NULL) {
        PyErr_FormatUnraisable(
                "Exception ignored while propagating not shareable error");
        return;
    }
    if (PyErr_ExceptionMatches(exctype)) {
        // We want to propagate the exception directly.
        *override = (_PyXI_failure){
            .code = _PyXI_ERR_NOT_SHAREABLE,
        };
    }
}


static int _ensure_main_ns(_PyXI_session *, _PyXI_failure *);
static const char * capture_session_error(_PyXI_session *, _PyXI_error *,
                                          _PyXI_failure *);

int
_PyXI_Enter(_PyXI_session *session,
            PyInterpreterState *interp, PyObject *nsupdates,
            _PyXI_session_result *result)
{
#ifndef NDEBUG
    PyThreadState *tstate = _PyThreadState_GET();  // Only used for asserts
#endif

    // Convert the attrs for cross-interpreter use.
    _PyXI_namespace *sharedns = NULL;
    if (nsupdates != NULL) {
        assert(PyDict_Check(nsupdates));
        Py_ssize_t len = PyDict_Size(nsupdates);
        if (len < 0) {
            if (result != NULL) {
                result->errcode = _PyXI_ERR_APPLY_NS_FAILURE;
            }
            return -1;
        }
        if (len > 0) {
            sharedns = _create_sharedns(nsupdates);
            if (sharedns == NULL) {
                if (result != NULL) {
                    result->errcode = _PyXI_ERR_APPLY_NS_FAILURE;
                }
                return -1;
            }
            // For now we limit it to shareable objects.
            xidata_fallback_t fallback = _PyXIDATA_XIDATA_ONLY;
            _PyXI_failure _err = XI_FAILURE_INIT;
            if (_fill_sharedns(sharedns, nsupdates, fallback, &_err) < 0) {
                assert(_PyErr_Occurred(tstate));
                if (_err.code == _PyXI_ERR_NO_ERROR) {
                    _err.code = _PyXI_ERR_UNCAUGHT_EXCEPTION;
                }
                _destroy_sharedns(sharedns);
                if (result != NULL) {
                    assert(_err.msg == NULL);
                    result->errcode = _err.code;
                }
                return -1;
            }
        }
    }

    // Switch to the requested interpreter (if necessary).
    _enter_session(session, interp);
    _PyXI_failure override = XI_FAILURE_INIT;
    override.code = _PyXI_ERR_UNCAUGHT_EXCEPTION;
#ifndef NDEBUG
    tstate = _PyThreadState_GET();
#endif

    // Ensure this thread owns __main__.
    if (_PyInterpreterState_SetRunningMain(interp) < 0) {
        // In the case where we didn't switch interpreters, it would
        // be more efficient to leave the exception in place and return
        // immediately.  However, life is simpler if we don't.
        override.code = _PyXI_ERR_ALREADY_RUNNING;
        goto error;
    }
    session->running = 1;

    // Apply the cross-interpreter data.
    if (sharedns != NULL) {
        if (_ensure_main_ns(session, &override) < 0) {
            goto error;
        }
        if (_apply_sharedns(sharedns, session->main_ns, NULL) < 0) {
            override.code = _PyXI_ERR_APPLY_NS_FAILURE;
            goto error;
        }
        _destroy_sharedns(sharedns);
    }

    override.code = _PyXI_ERR_NO_ERROR;
    assert(!_PyErr_Occurred(tstate));
    return 0;

error:
    // We want to propagate all exceptions here directly (best effort).
    assert(override.code != _PyXI_ERR_NO_ERROR);
    _PyXI_error err = {0};
    const char *failure = capture_session_error(session, &err, &override);

    // Exit the session.
    _exit_session(session);
#ifndef NDEBUG
    tstate = _PyThreadState_GET();
#endif

    if (sharedns != NULL) {
        _destroy_sharedns(sharedns);
    }

    // Apply the error from the other interpreter.
    PyObject *excinfo = _PyXI_ApplyError(&err, failure);
    xi_error_clear(&err);
    if (excinfo != NULL) {
        if (result != NULL) {
            result->excinfo = excinfo;
        }
        else {
#ifdef Py_DEBUG
            fprintf(stderr, "_PyXI_Enter(): uncaught exception discarded");
#endif
            Py_DECREF(excinfo);
        }
    }
    assert(_PyErr_Occurred(tstate));

    return -1;
}

static int _pop_preserved(_PyXI_session *, _PyXI_namespace **, PyObject **,
                          _PyXI_failure *);
static int _finish_preserved(_PyXI_namespace *, PyObject **);

int
_PyXI_Exit(_PyXI_session *session, _PyXI_failure *override,
           _PyXI_session_result *result)
{
    PyThreadState *tstate = _PyThreadState_GET();
    int res = 0;

    // Capture the raised exception, if any.
    _PyXI_error err = {0};
    const char *failure = NULL;
    if (override != NULL && override->code == _PyXI_ERR_NO_ERROR) {
        assert(override->msg == NULL);
        override = NULL;
    }
    if (_PyErr_Occurred(tstate)) {
        failure = capture_session_error(session, &err, override);
    }
    else {
        assert(override == NULL);
    }

    // Capture the preserved namespace.
    _PyXI_namespace *preserved = NULL;
    PyObject *preservedobj = NULL;
    if (result != NULL) {
        assert(!_PyErr_Occurred(tstate));
        _PyXI_failure _override = XI_FAILURE_INIT;
        if (_pop_preserved(
                    session, &preserved, &preservedobj, &_override) < 0)
        {
            assert(preserved == NULL);
            assert(preservedobj == NULL);
            if (xi_error_is_set(&err)) {
                // XXX Chain the exception (i.e. set __context__)?
                PyErr_FormatUnraisable(
                    "Exception ignored while capturing preserved objects");
                clear_xi_failure(&_override);
            }
            else {
                if (_override.code == _PyXI_ERR_NO_ERROR) {
                    _override.code = _PyXI_ERR_UNCAUGHT_EXCEPTION;
                }
                failure = capture_session_error(session, &err, &_override);
            }
        }
    }

    // Exit the session.
    assert(!_PyErr_Occurred(tstate));
    _exit_session(session);
    tstate = _PyThreadState_GET();

    // Restore the preserved namespace.
    assert(preserved == NULL || preservedobj == NULL);
    if (_finish_preserved(preserved, &preservedobj) < 0) {
        assert(preservedobj == NULL);
        if (xi_error_is_set(&err)) {
            // XXX Chain the exception (i.e. set __context__)?
            PyErr_FormatUnraisable(
                "Exception ignored while capturing preserved objects");
        }
        else {
            xi_error_set_override_code(
                            tstate, &err, _PyXI_ERR_PRESERVE_FAILURE);
            _propagate_not_shareable_error(tstate, err.override);
        }
    }
    if (result != NULL) {
        result->preserved = preservedobj;
        result->errcode = err.override != NULL
            ? err.override->code
            : _PyXI_ERR_NO_ERROR;
    }

    // Apply the error from the other interpreter, if any.
    if (xi_error_is_set(&err)) {
        res = -1;
        assert(!_PyErr_Occurred(tstate));
        PyObject *excinfo = _PyXI_ApplyError(&err, failure);
        if (excinfo == NULL) {
            assert(_PyErr_Occurred(tstate));
            if (result != NULL && !xi_error_has_override(&err)) {
                _PyXI_ClearResult(result);
                *result = (_PyXI_session_result){
                    .errcode = _PyXI_ERR_EXC_PROPAGATION_FAILURE,
                };
            }
        }
        else if (result != NULL) {
            result->excinfo = excinfo;
        }
        else {
#ifdef Py_DEBUG
            fprintf(stderr, "_PyXI_Exit(): uncaught exception discarded");
#endif
            Py_DECREF(excinfo);
        }
        xi_error_clear(&err);
    }
    return res;
}


/* in an active cross-interpreter session */

static const char *
capture_session_error(_PyXI_session *session, _PyXI_error *err,
                      _PyXI_failure *override)
{
    assert(_session_is_active(session));
    assert(!xi_error_is_set(err));
    PyThreadState *tstate = session->init_tstate;

    // Normalize the exception override.
    if (override != NULL) {
        if (override->code == _PyXI_ERR_UNCAUGHT_EXCEPTION) {
            assert(override->msg == NULL);
            override = NULL;
        }
        else {
            assert(override->code != _PyXI_ERR_NO_ERROR);
        }
    }

    // Handle the exception, if any.
    const char *failure = NULL;
    PyObject *exc = xi_error_resolve_current_exc(tstate, override);
    if (exc != NULL) {
        // There is an unhandled exception we need to preserve.
        failure = xi_error_set_exc(tstate, err, exc);
        Py_DECREF(exc);
        if (_PyErr_Occurred(tstate)) {
            PyErr_FormatUnraisable(failure);
        }
    }

    // Handle the override.
    if (override != NULL && failure == NULL) {
        xi_error_set_override(tstate, err, override);
    }

    // Finished!
    assert(!_PyErr_Occurred(tstate));
    return failure;
}

static int
_ensure_main_ns(_PyXI_session *session, _PyXI_failure *failure)
{
    assert(_session_is_active(session));
    PyThreadState *tstate = session->init_tstate;
    if (session->main_ns != NULL) {
        return 0;
    }
    // Cache __main__.__dict__.
    PyObject *main_mod = _Py_GetMainModule(tstate);
    if (_Py_CheckMainModule(main_mod) < 0) {
        Py_XDECREF(main_mod);
        if (failure != NULL) {
            *failure = (_PyXI_failure){
                .code = _PyXI_ERR_MAIN_NS_FAILURE,
            };
        }
        return -1;
    }
    PyObject *ns = PyModule_GetDict(main_mod);  // borrowed
    Py_DECREF(main_mod);
    if (ns == NULL) {
        if (failure != NULL) {
            *failure = (_PyXI_failure){
                .code = _PyXI_ERR_MAIN_NS_FAILURE,
            };
        }
        return -1;
    }
    session->main_ns = Py_NewRef(ns);
    return 0;
}

PyObject *
_PyXI_GetMainNamespace(_PyXI_session *session, _PyXI_failure *failure)
{
    if (!_session_is_active(session)) {
        PyErr_SetString(PyExc_RuntimeError, "session not active");
        return NULL;
    }
    if (_ensure_main_ns(session, failure) < 0) {
        return NULL;
    }
    return session->main_ns;
}


static int
_pop_preserved(_PyXI_session *session,
               _PyXI_namespace **p_xidata, PyObject **p_obj,
               _PyXI_failure *p_failure)
{
    _PyXI_failure failure = XI_FAILURE_INIT;
    _PyXI_namespace *xidata = NULL;
    assert(_PyThreadState_GET() == session->init_tstate);  // active session

    if (session->_preserved == NULL) {
        *p_xidata = NULL;
        *p_obj = NULL;
        return 0;
    }
    if (session->init_tstate == session->prev_tstate) {
        // We did not switch interpreters.
        *p_xidata = NULL;
        *p_obj = session->_preserved;
        session->_preserved = NULL;
        return 0;
    }
    *p_obj = NULL;

    // We did switch interpreters.
    Py_ssize_t len = PyDict_Size(session->_preserved);
    if (len < 0) {
        failure.code = _PyXI_ERR_PRESERVE_FAILURE;
        goto error;
    }
    else if (len == 0) {
        *p_xidata = NULL;
    }
    else {
        _PyXI_namespace *xidata = _create_sharedns(session->_preserved);
        if (xidata == NULL) {
            failure.code = _PyXI_ERR_PRESERVE_FAILURE;
            goto error;
        }
        if (_fill_sharedns(xidata, session->_preserved,
                           _PyXIDATA_FULL_FALLBACK, &failure) < 0)
        {
            if (failure.code != _PyXI_ERR_NOT_SHAREABLE) {
                assert(failure.msg != NULL);
                failure.code = _PyXI_ERR_PRESERVE_FAILURE;
            }
            goto error;
        }
        *p_xidata = xidata;
    }
    Py_CLEAR(session->_preserved);
    return 0;

error:
    if (p_failure != NULL) {
        *p_failure = failure;
    }
    if (xidata != NULL) {
        _destroy_sharedns(xidata);
    }
    return -1;
}

static int
_finish_preserved(_PyXI_namespace *xidata, PyObject **p_preserved)
{
    if (xidata == NULL) {
        return 0;
    }
    int res = -1;
    if (p_preserved != NULL) {
        PyObject *ns = PyDict_New();
        if (ns == NULL) {
            goto finally;
        }
        if (_apply_sharedns(xidata, ns, NULL) < 0) {
            Py_CLEAR(ns);
            goto finally;
        }
        *p_preserved = ns;
    }
    res = 0;

finally:
    _destroy_sharedns(xidata);
    return res;
}

int
_PyXI_Preserve(_PyXI_session *session, const char *name, PyObject *value,
               _PyXI_failure *p_failure)
{
    _PyXI_failure failure = XI_FAILURE_INIT;
    if (!_session_is_active(session)) {
        PyErr_SetString(PyExc_RuntimeError, "session not active");
        return -1;
    }
    if (session->_preserved == NULL) {
        session->_preserved = PyDict_New();
        if (session->_preserved == NULL) {
            set_exc_with_cause(PyExc_RuntimeError,
                               "failed to initialize preserved objects");
            failure.code = _PyXI_ERR_PRESERVE_FAILURE;
            goto error;
        }
    }
    if (PyDict_SetItemString(session->_preserved, name, value) < 0) {
        set_exc_with_cause(PyExc_RuntimeError, "failed to preserve object");
        failure.code = _PyXI_ERR_PRESERVE_FAILURE;
        goto error;
    }
    return 0;

error:
    if (p_failure != NULL) {
        *p_failure = failure;
    }
    return -1;
}

PyObject *
_PyXI_GetPreserved(_PyXI_session_result *result, const char *name)
{
    PyObject *value = NULL;
    if (result->preserved != NULL) {
        (void)PyDict_GetItemStringRef(result->preserved, name, &value);
    }
    return value;
}

void
_PyXI_ClearResult(_PyXI_session_result *result)
{
    Py_CLEAR(result->preserved);
    Py_CLEAR(result->excinfo);
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
