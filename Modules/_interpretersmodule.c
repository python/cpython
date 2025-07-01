/* interpreters module */
/* low-level access to interpreter primitives */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_code.h"          // _PyCode_HAS_EXECUTORS()
#include "pycore_crossinterp.h"   // _PyXIData_t
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_interp.h"        // _PyInterpreterState_IDIncref()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_namespace.h"     // _PyNamespace_New()
#include "pycore_pybuffer.h"      // _PyBuffer_ReleaseInInterpreterAndRawFree()
#include "pycore_pylifecycle.h"   // _PyInterpreterConfig_AsDict()
#include "pycore_pystate.h"       // _PyInterpreterState_IsRunningMain()

#include "marshal.h"              // PyMarshal_ReadObjectFromString()

#include "_interpreters_common.h"


#define MODULE_NAME _interpreters
#define MODULE_NAME_STR Py_STRINGIFY(MODULE_NAME)
#define MODINIT_FUNC_NAME RESOLVE_MODINIT_FUNC_NAME(MODULE_NAME)


static PyInterpreterState *
_get_current_interp(void)
{
    // PyInterpreterState_Get() aborts if lookup fails, so don't need
    // to check the result for NULL.
    return PyInterpreterState_Get();
}

#define look_up_interp _PyInterpreterState_LookUpIDObject


static PyObject *
_get_current_module(void)
{
    PyObject *name = PyUnicode_FromString(MODULE_NAME_STR);
    if (name == NULL) {
        return NULL;
    }
    PyObject *mod = PyImport_GetModule(name);
    Py_DECREF(name);
    if (mod == NULL) {
        return NULL;
    }
    assert(mod != Py_None);
    return mod;
}


static int
is_running_main(PyInterpreterState *interp)
{
    if (_PyInterpreterState_IsRunningMain(interp)) {
        return 1;
    }
    // Unlike with the general C-API, we can be confident that someone
    // using this module for the main interpreter is doing so through
    // the main program.  Thus we can make this extra check.  This benefits
    // applications that embed Python but haven't been updated yet
    // to call _PyInterpreterState_SetRunningMain().
    if (_Py_IsMainInterpreter(interp)) {
        return 1;
    }
    return 0;
}


static inline int
is_notshareable_raised(PyThreadState *tstate)
{
    PyObject *exctype = _PyXIData_GetNotShareableErrorType(tstate);
    return _PyErr_ExceptionMatches(tstate, exctype);
}

static void
unwrap_not_shareable(PyThreadState *tstate, _PyXI_failure *failure)
{
    if (_PyXI_UnwrapNotShareableError(tstate, failure) < 0) {
        _PyErr_Clear(tstate);
    }
}


/* Cross-interpreter Buffer Views *******************************************/

/* When a memoryview object is "shared" between interpreters,
 * its underlying "buffer" memory is actually shared, rather than just
 * copied.  This facilitates efficient use of that data where otherwise
 * interpreters are strictly isolated.  However, this also means that
 * the underlying data is subject to the complexities of thread-safety,
 * which the user must manage carefully.
 *
 * When the memoryview is "shared", it is essentially copied in the same
 * way as PyMemory_FromObject() does, but in another interpreter.
 * The Py_buffer value is copied like normal, including the "buf" pointer,
 * with one key exception.
 *
 * When a Py_buffer is released and it holds a reference to an object,
 * that object gets a chance to call its bf_releasebuffer() (if any)
 * before the object is decref'ed.  The same is true with the memoryview
 * tp_dealloc, which essentially calls PyBuffer_Release().
 *
 * The problem for a Py_buffer shared between two interpreters is that
 * the naive approach breaks interpreter isolation.  Operations on an
 * object must only happen while that object's interpreter is active.
 * If the copied mv->view.obj pointed to the original memoryview then
 * the PyBuffer_Release() would happen under the wrong interpreter.
 *
 * To work around this, we set mv->view.obj on the copied memoryview
 * to a wrapper object with the only job of releasing the original
 * buffer in a cross-interpreter-safe way.
 */

// XXX Note that there is still an issue to sort out, where the original
// interpreter is destroyed but code in another interpreter is still
// using dependent buffers.  Using such buffers segfaults.  This will
// require a careful fix.  In the meantime, users will have to be
// diligent about avoiding the problematic situation.

typedef struct {
    PyObject base;
    Py_buffer *view;
    int64_t interpid;
} xibufferview;

static PyObject *
xibufferview_from_buffer(PyTypeObject *cls, Py_buffer *view, int64_t interpid)
{
    assert(interpid >= 0);

    Py_buffer *copied = PyMem_RawMalloc(sizeof(Py_buffer));
    if (copied == NULL) {
        return NULL;
    }
    /* This steals the view->obj reference  */
    *copied = *view;

    xibufferview *self = PyObject_Malloc(sizeof(xibufferview));
    if (self == NULL) {
        PyMem_RawFree(copied);
        return NULL;
    }
    PyObject_Init(&self->base, cls);
    *self = (xibufferview){
        .base = self->base,
        .view = copied,
        .interpid = interpid,
    };
    return (PyObject *)self;
}

static void
xibufferview_dealloc(PyObject *op)
{
    xibufferview *self = (xibufferview *)op;
    if (self->view != NULL) {
        PyInterpreterState *interp =
                        _PyInterpreterState_LookUpID(self->interpid);
        if (interp == NULL) {
            /* The interpreter is no longer alive. */
            PyErr_Clear();
            PyMem_RawFree(self->view);
        }
        else {
            if (_PyBuffer_ReleaseInInterpreterAndRawFree(interp,
                                                         self->view) < 0)
            {
                // XXX Emit a warning?
                PyErr_Clear();
            }
        }
    }

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    /* "Instances of heap-allocated types hold a reference to their type."
     * See: https://docs.python.org/3.11/howto/isolating-extensions.html#garbage-collection-protocol
     * See: https://docs.python.org/3.11/c-api/typeobj.html#c.PyTypeObject.tp_traverse
    */
    // XXX Why don't we implement Py_TPFLAGS_HAVE_GC, e.g. Py_tp_traverse,
    // like we do for _abc._abc_data?
    Py_DECREF(tp);
}

static int
xibufferview_getbuf(PyObject *op, Py_buffer *view, int flags)
{
    /* Only PyMemoryView_FromObject() should ever call this,
       via _memoryview_from_xid() below. */
    xibufferview *self = (xibufferview *)op;
    *view = *self->view;
    /* This is the workaround mentioned earlier. */
    view->obj = op;
    // XXX Should we leave it alone?
    view->internal = NULL;
    return 0;
}

static PyType_Slot XIBufferViewType_slots[] = {
    {Py_tp_dealloc, xibufferview_dealloc},
    {Py_bf_getbuffer, xibufferview_getbuf},
    // We don't bother with Py_bf_releasebuffer since we don't need it.
    {0, NULL},
};

static PyType_Spec XIBufferViewType_spec = {
    .name = MODULE_NAME_STR ".CrossInterpreterBufferView",
    .basicsize = sizeof(xibufferview),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = XIBufferViewType_slots,
};


static PyTypeObject * _get_current_xibufferview_type(void);


struct xibuffer {
    Py_buffer view;
    int used;
};

static PyObject *
_memoryview_from_xid(_PyXIData_t *data)
{
    assert(_PyXIData_DATA(data) != NULL);
    assert(_PyXIData_OBJ(data) == NULL);
    assert(_PyXIData_INTERPID(data) >= 0);
    struct xibuffer *view = (struct xibuffer *)_PyXIData_DATA(data);
    assert(!view->used);

    PyTypeObject *cls = _get_current_xibufferview_type();
    if (cls == NULL) {
        return NULL;
    }

    PyObject *obj = xibufferview_from_buffer(
                        cls, &view->view, _PyXIData_INTERPID(data));
    if (obj == NULL) {
        return NULL;
    }
    PyObject *res = PyMemoryView_FromObject(obj);
    if (res == NULL) {
        Py_DECREF(obj);
        return NULL;
    }
    view->used = 1;
    return res;
}

static void
_pybuffer_shared_free(void* data)
{
    struct xibuffer *view = (struct xibuffer *)data;
    if (!view->used) {
        PyBuffer_Release(&view->view);
    }
    PyMem_RawFree(data);
}

static int
_pybuffer_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    struct xibuffer *view = PyMem_RawMalloc(sizeof(struct xibuffer));
    if (view == NULL) {
        return -1;
    }
    view->used = 0;
    /* This will increment the memoryview's export count, which won't get
     * decremented until the view sent to other interpreters is released. */
    if (PyObject_GetBuffer(obj, &view->view, PyBUF_FULL_RO) < 0) {
        PyMem_RawFree(view);
        return -1;
    }
    /* The view holds a reference to the object, so we don't worry
     * about also tracking it on the cross-interpreter data. */
    _PyXIData_Init(data, tstate->interp, view, NULL, _memoryview_from_xid);
    data->free = _pybuffer_shared_free;
    return 0;
}

static int
register_memoryview_xid(PyObject *mod, PyTypeObject **p_state)
{
    // XIBufferView
    assert(*p_state == NULL);
    PyTypeObject *cls = (PyTypeObject *)PyType_FromModuleAndSpec(
                mod, &XIBufferViewType_spec, NULL);
    if (cls == NULL) {
        return -1;
    }
    if (PyModule_AddType(mod, cls) < 0) {
        Py_DECREF(cls);
        return -1;
    }
    *p_state = cls;

    // Register XID for the builtin memoryview type.
    if (ensure_xid_class(&PyMemoryView_Type, GETDATA(_pybuffer_shared)) < 0) {
        return -1;
    }
    // We don't ever bother un-registering memoryview.

    return 0;
}



/* module state *************************************************************/

typedef struct {
    int _notused;

    /* heap types */
    PyTypeObject *XIBufferViewType;
} module_state;

static inline module_state *
get_module_state(PyObject *mod)
{
    assert(mod != NULL);
    module_state *state = PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static module_state *
_get_current_module_state(void)
{
    PyObject *mod = _get_current_module();
    if (mod == NULL) {
        mod = PyImport_ImportModule(MODULE_NAME_STR);
        if (mod == NULL) {
            return NULL;
        }
    }
    module_state *state = get_module_state(mod);
    Py_DECREF(mod);
    return state;
}

static int
traverse_module_state(module_state *state, visitproc visit, void *arg)
{
    /* heap types */
    Py_VISIT(state->XIBufferViewType);

    return 0;
}

static int
clear_module_state(module_state *state)
{
    /* heap types */
    Py_CLEAR(state->XIBufferViewType);

    return 0;
}


static PyTypeObject *
_get_current_xibufferview_type(void)
{
    module_state *state = _get_current_module_state();
    if (state == NULL) {
        return NULL;
    }
    return state->XIBufferViewType;
}


/* interpreter-specific code ************************************************/

static int
init_named_config(PyInterpreterConfig *config, const char *name)
{
    if (name == NULL
            || strcmp(name, "") == 0
            || strcmp(name, "default") == 0)
    {
        name = "isolated";
    }

    if (strcmp(name, "isolated") == 0) {
        *config = (PyInterpreterConfig)_PyInterpreterConfig_INIT;
    }
    else if (strcmp(name, "legacy") == 0) {
        *config = (PyInterpreterConfig)_PyInterpreterConfig_LEGACY_INIT;
    }
    else if (strcmp(name, "empty") == 0) {
        *config = (PyInterpreterConfig){0};
    }
    else {
        PyErr_Format(PyExc_ValueError,
                     "unsupported config name '%s'", name);
        return -1;
    }
    return 0;
}

static int
config_from_object(PyObject *configobj, PyInterpreterConfig *config)
{
    if (configobj == NULL || configobj == Py_None) {
        if (init_named_config(config, NULL) < 0) {
            return -1;
        }
    }
    else if (PyUnicode_Check(configobj)) {
        const char *utf8name = PyUnicode_AsUTF8(configobj);
        if (utf8name == NULL) {
            return -1;
        }
        if (init_named_config(config, utf8name) < 0) {
            return -1;
        }
    }
    else {
        PyObject *dict = PyObject_GetAttrString(configobj, "__dict__");
        if (dict == NULL) {
            PyErr_Format(PyExc_TypeError, "bad config %R", configobj);
            return -1;
        }
        int res = _PyInterpreterConfig_InitFromDict(config, dict);
        Py_DECREF(dict);
        if (res < 0) {
            return -1;
        }
    }
    return 0;
}


struct interp_call {
    _PyXIData_t *func;
    _PyXIData_t *args;
    _PyXIData_t *kwargs;
    struct {
        _PyXIData_t func;
        _PyXIData_t args;
        _PyXIData_t kwargs;
    } _preallocated;
};

static void
_interp_call_clear(struct interp_call *call)
{
    if (call->func != NULL) {
        _PyXIData_Clear(NULL, call->func);
    }
    if (call->args != NULL) {
        _PyXIData_Clear(NULL, call->args);
    }
    if (call->kwargs != NULL) {
        _PyXIData_Clear(NULL, call->kwargs);
    }
    *call = (struct interp_call){0};
}

static int
_interp_call_pack(PyThreadState *tstate, struct interp_call *call,
                  PyObject *func, PyObject *args, PyObject *kwargs)
{
    xidata_fallback_t fallback = _PyXIDATA_FULL_FALLBACK;
    assert(call->func == NULL);
    assert(call->args == NULL);
    assert(call->kwargs == NULL);
    // Handle the func.
    if (!PyCallable_Check(func)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "expected a callable, got %R", func);
        return -1;
    }
    if (_PyFunction_GetXIData(tstate, func, &call->_preallocated.func) < 0) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        if (_PyPickle_GetXIData(tstate, func, &call->_preallocated.func) < 0) {
            _PyErr_SetRaisedException(tstate, exc);
            return -1;
        }
        Py_DECREF(exc);
    }
    call->func = &call->_preallocated.func;
    // Handle the args.
    if (args == NULL || args == Py_None) {
        // Leave it empty.
    }
    else {
        assert(PyTuple_Check(args));
        if (PyTuple_GET_SIZE(args) > 0) {
            if (_PyObject_GetXIData(
                    tstate, args, fallback, &call->_preallocated.args) < 0)
            {
                _interp_call_clear(call);
                return -1;
            }
            call->args = &call->_preallocated.args;
        }
    }
    // Handle the kwargs.
    if (kwargs == NULL || kwargs == Py_None) {
        // Leave it empty.
    }
    else {
        assert(PyDict_Check(kwargs));
        if (PyDict_GET_SIZE(kwargs) > 0) {
            if (_PyObject_GetXIData(
                    tstate, kwargs, fallback, &call->_preallocated.kwargs) < 0)
            {
                _interp_call_clear(call);
                return -1;
            }
            call->kwargs = &call->_preallocated.kwargs;
        }
    }
    return 0;
}

static void
wrap_notshareable(PyThreadState *tstate, const char *label)
{
    if (!is_notshareable_raised(tstate)) {
        return;
    }
    assert(label != NULL && strlen(label) > 0);
    PyObject *cause = _PyErr_GetRaisedException(tstate);
    _PyXIData_FormatNotShareableError(tstate, "%s not shareable", label);
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    PyException_SetCause(exc, cause);
    _PyErr_SetRaisedException(tstate, exc);
}

static int
_interp_call_unpack(struct interp_call *call,
                    PyObject **p_func, PyObject **p_args, PyObject **p_kwargs)
{
    PyThreadState *tstate = PyThreadState_Get();

    // Unpack the func.
    PyObject *func = _PyXIData_NewObject(call->func);
    if (func == NULL) {
        wrap_notshareable(tstate, "func");
        return -1;
    }
    // Unpack the args.
    PyObject *args;
    if (call->args == NULL) {
        args = PyTuple_New(0);
        if (args == NULL) {
            Py_DECREF(func);
            return -1;
        }
    }
    else {
        args = _PyXIData_NewObject(call->args);
        if (args == NULL) {
            wrap_notshareable(tstate, "args");
            Py_DECREF(func);
            return -1;
        }
        assert(PyTuple_Check(args));
    }
    // Unpack the kwargs.
    PyObject *kwargs = NULL;
    if (call->kwargs != NULL) {
        kwargs = _PyXIData_NewObject(call->kwargs);
        if (kwargs == NULL) {
            wrap_notshareable(tstate, "kwargs");
            Py_DECREF(func);
            Py_DECREF(args);
            return -1;
        }
        assert(PyDict_Check(kwargs));
    }
    *p_func = func;
    *p_args = args;
    *p_kwargs = kwargs;
    return 0;
}

static int
_make_call(struct interp_call *call,
           PyObject **p_result, _PyXI_failure *failure)
{
    assert(call != NULL && call->func != NULL);
    PyThreadState *tstate = _PyThreadState_GET();

    // Get the func and args.
    PyObject *func = NULL, *args = NULL, *kwargs = NULL;
    if (_interp_call_unpack(call, &func, &args, &kwargs) < 0) {
        assert(func == NULL);
        assert(args == NULL);
        assert(kwargs == NULL);
        _PyXI_InitFailure(failure, _PyXI_ERR_OTHER, NULL);
        unwrap_not_shareable(tstate, failure);
        return -1;
    }
    assert(!_PyErr_Occurred(tstate));

    // Make the call.
    PyObject *resobj = PyObject_Call(func, args, kwargs);
    Py_DECREF(func);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    if (resobj == NULL) {
        return -1;
    }
    *p_result = resobj;
    return 0;
}

static int
_run_script(_PyXIData_t *script, PyObject *ns, _PyXI_failure *failure)
{
    PyObject *code = _PyXIData_NewObject(script);
    if (code == NULL) {
        _PyXI_InitFailure(failure, _PyXI_ERR_NOT_SHAREABLE, NULL);
        return -1;
    }
    PyObject *result = PyEval_EvalCode(code, ns, ns);
    Py_DECREF(code);
    if (result == NULL) {
        _PyXI_InitFailure(failure, _PyXI_ERR_UNCAUGHT_EXCEPTION, NULL);
        return -1;
    }
    assert(result == Py_None);
    Py_DECREF(result);  // We throw away the result.
    return 0;
}

struct run_result {
    PyObject *result;
    PyObject *excinfo;
};

static void
_run_result_clear(struct run_result *runres)
{
    Py_CLEAR(runres->result);
    Py_CLEAR(runres->excinfo);
}

static int
_run_in_interpreter(PyThreadState *tstate, PyInterpreterState *interp,
                     _PyXIData_t *script, struct interp_call *call,
                     PyObject *shareables, struct run_result *runres)
{
    assert(!_PyErr_Occurred(tstate));
    int res = -1;
    _PyXI_failure *failure = _PyXI_NewFailure();
    if (failure == NULL) {
        return -1;
    }
    _PyXI_session *session = _PyXI_NewSession();
    if (session == NULL) {
        _PyXI_FreeFailure(failure);
        return -1;
    }
    _PyXI_session_result result = {0};

    // Prep and switch interpreters.
    if (_PyXI_Enter(session, interp, shareables, &result) < 0) {
        // If an error occured at this step, it means that interp
        // was not prepared and switched.
        _PyXI_FreeSession(session);
        _PyXI_FreeFailure(failure);
        assert(result.excinfo == NULL);
        return -1;
    }

    // Run in the interpreter.
    if (script != NULL) {
        assert(call == NULL);
        PyObject *mainns = _PyXI_GetMainNamespace(session, failure);
        if (mainns == NULL) {
            goto finally;
        }
        res = _run_script(script, mainns, failure);
    }
    else {
        assert(call != NULL);
        PyObject *resobj;
        res = _make_call(call, &resobj, failure);
        if (res == 0) {
            res = _PyXI_Preserve(session, "resobj", resobj, failure);
            Py_DECREF(resobj);
            if (res < 0) {
                goto finally;
            }
        }
    }

finally:
    // Clean up and switch back.
    (void)res;
    int exitres = _PyXI_Exit(session, failure, &result);
    assert(res == 0 || exitres != 0);
    _PyXI_FreeSession(session);
    _PyXI_FreeFailure(failure);

    res = exitres;
    if (_PyErr_Occurred(tstate)) {
        // It's a directly propagated exception.
        assert(res < 0);
    }
    else if (res < 0) {
        assert(result.excinfo != NULL);
        runres->excinfo = Py_NewRef(result.excinfo);
        res = -1;
    }
    else {
        assert(result.excinfo == NULL);
        runres->result = _PyXI_GetPreserved(&result, "resobj");
        if (_PyErr_Occurred(tstate)) {
            res = -1;
        }
    }
    _PyXI_ClearResult(&result);
    return res;
}


/* module level code ********************************************************/

static long
get_whence(PyInterpreterState *interp)
{
    return _PyInterpreterState_GetWhence(interp);
}


static PyInterpreterState *
resolve_interp(PyObject *idobj, int restricted, int reqready, const char *op)
{
    PyInterpreterState *interp;
    if (idobj == NULL) {
        interp = PyInterpreterState_Get();
    }
    else {
        interp = look_up_interp(idobj);
        if (interp == NULL) {
            return NULL;
        }
    }

    if (reqready && !_PyInterpreterState_IsReady(interp)) {
        if (idobj == NULL) {
            PyErr_Format(PyExc_InterpreterError,
                         "cannot %s current interpreter (not ready)", op);
        }
        else {
            PyErr_Format(PyExc_InterpreterError,
                         "cannot %s interpreter %R (not ready)", op, idobj);
        }
        return NULL;
    }

    if (restricted && get_whence(interp) != _PyInterpreterState_WHENCE_STDLIB) {
        if (idobj == NULL) {
            PyErr_Format(PyExc_InterpreterError,
                         "cannot %s unrecognized current interpreter", op);
        }
        else {
            PyErr_Format(PyExc_InterpreterError,
                         "cannot %s unrecognized interpreter %R", op, idobj);
        }
        return NULL;
    }

    return interp;
}


static PyObject *
get_summary(PyInterpreterState *interp)
{
    PyObject *idobj = _PyInterpreterState_GetIDObject(interp);
    if (idobj == NULL) {
        return NULL;
    }
    PyObject *whenceobj = PyLong_FromLong(
                            get_whence(interp));
    if (whenceobj == NULL) {
        Py_DECREF(idobj);
        return NULL;
    }
    PyObject *res = PyTuple_Pack(2, idobj, whenceobj);
    Py_DECREF(idobj);
    Py_DECREF(whenceobj);
    return res;
}


static PyObject *
interp_new_config(PyObject *self, PyObject *args, PyObject *kwds)
{
    const char *name = NULL;
    if (!PyArg_ParseTuple(args, "|s:" MODULE_NAME_STR ".new_config",
                          &name))
    {
        return NULL;
    }
    PyObject *overrides = kwds;

    PyInterpreterConfig config;
    if (init_named_config(&config, name) < 0) {
        return NULL;
    }

    if (overrides != NULL && PyDict_GET_SIZE(overrides) > 0) {
        if (_PyInterpreterConfig_UpdateFromDict(&config, overrides) < 0) {
            return NULL;
        }
    }

    PyObject *dict = _PyInterpreterConfig_AsDict(&config);
    if (dict == NULL) {
        return NULL;
    }

    PyObject *configobj = _PyNamespace_New(dict);
    Py_DECREF(dict);
    return configobj;
}

PyDoc_STRVAR(new_config_doc,
"new_config(name='isolated', /, **overrides) -> type.SimpleNamespace\n\
\n\
Return a representation of a new PyInterpreterConfig.\n\
\n\
The name determines the initial values of the config.  Supported named\n\
configs are: default, isolated, legacy, and empty.\n\
\n\
Any keyword arguments are set on the corresponding config fields,\n\
overriding the initial values.");


static PyObject *
interp_create(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"config", "reqrefs", NULL};
    PyObject *configobj = NULL;
    int reqrefs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O$p:create", kwlist,
                                     &configobj, &reqrefs)) {
        return NULL;
    }

    PyInterpreterConfig config;
    if (config_from_object(configobj, &config) < 0) {
        return NULL;
    }

    long whence = _PyInterpreterState_WHENCE_STDLIB;
    PyInterpreterState *interp = \
            _PyXI_NewInterpreter(&config, &whence, NULL, NULL);
    if (interp == NULL) {
        // XXX Move the chained exception to interpreters.create()?
        PyObject *exc = PyErr_GetRaisedException();
        assert(exc != NULL);
        PyErr_SetString(PyExc_InterpreterError, "interpreter creation failed");
        _PyErr_ChainExceptions1(exc);
        return NULL;
    }
    assert(_PyInterpreterState_IsReady(interp));

    PyObject *idobj = _PyInterpreterState_GetIDObject(interp);
    if (idobj == NULL) {
        _PyXI_EndInterpreter(interp, NULL, NULL);
        return NULL;
    }

    if (reqrefs) {
        // Decref to 0 will destroy the interpreter.
        _PyInterpreterState_RequireIDRef(interp, 1);
    }

    return idobj;
}


PyDoc_STRVAR(create_doc,
"create([config], *, reqrefs=False) -> ID\n\
\n\
Create a new interpreter and return a unique generated ID.\n\
\n\
The caller is responsible for destroying the interpreter before exiting,\n\
typically by using _interpreters.destroy().  This can be managed \n\
automatically by passing \"reqrefs=True\" and then using _incref() and\n\
_decref() appropriately.\n\
\n\
\"config\" must be a valid interpreter config or the name of a\n\
predefined config (\"isolated\" or \"legacy\").  The default\n\
is \"isolated\".");


static PyObject *
interp_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "restrict", NULL};
    PyObject *id;
    int restricted = 0;
    // XXX Use "L" for id?
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|$p:destroy", kwlist, &id, &restricted))
    {
        return NULL;
    }

    // Look up the interpreter.
    int reqready = 0;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "destroy");
    if (interp == NULL) {
        return NULL;
    }

    // Ensure we don't try to destroy the current interpreter.
    PyInterpreterState *current = _get_current_interp();
    if (current == NULL) {
        return NULL;
    }
    if (interp == current) {
        PyErr_SetString(PyExc_InterpreterError,
                        "cannot destroy the current interpreter");
        return NULL;
    }

    // Ensure the interpreter isn't running.
    /* XXX We *could* support destroying a running interpreter but
       aren't going to worry about it for now. */
    if (is_running_main(interp)) {
        PyErr_Format(PyExc_InterpreterError, "interpreter running");
        return NULL;
    }

    // Destroy the interpreter.
    _PyXI_EndInterpreter(interp, NULL, NULL);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(destroy_doc,
"destroy(id, *, restrict=False)\n\
\n\
Destroy the identified interpreter.\n\
\n\
Attempting to destroy the current interpreter raises InterpreterError.\n\
So does an unrecognized ID.");


static PyObject *
interp_list_all(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"require_ready", NULL};
    int reqready = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "|$p:" MODULE_NAME_STR ".list_all",
                                     kwlist, &reqready))
    {
        return NULL;
    }

    PyObject *ids = PyList_New(0);
    if (ids == NULL) {
        return NULL;
    }

    PyInterpreterState *interp = PyInterpreterState_Head();
    while (interp != NULL) {
        if (!reqready || _PyInterpreterState_IsReady(interp)) {
            PyObject *item = get_summary(interp);
            if (item == NULL) {
                Py_DECREF(ids);
                return NULL;
            }

            // insert at front of list
            int res = PyList_Insert(ids, 0, item);
            Py_DECREF(item);
            if (res < 0) {
                Py_DECREF(ids);
                return NULL;
            }
        }
        interp = PyInterpreterState_Next(interp);
    }

    return ids;
}

PyDoc_STRVAR(list_all_doc,
"list_all() -> [(ID, whence)]\n\
\n\
Return a list containing the ID of every existing interpreter.");


static PyObject *
interp_get_current(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyInterpreterState *interp =_get_current_interp();
    if (interp == NULL) {
        return NULL;
    }
    assert(_PyInterpreterState_IsReady(interp));
    return get_summary(interp);
}

PyDoc_STRVAR(get_current_doc,
"get_current() -> (ID, whence)\n\
\n\
Return the ID of current interpreter.");


static PyObject *
interp_get_main(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyInterpreterState *interp = _PyInterpreterState_Main();
    assert(_PyInterpreterState_IsReady(interp));
    return get_summary(interp);
}

PyDoc_STRVAR(get_main_doc,
"get_main() -> (ID, whence)\n\
\n\
Return the ID of main interpreter.");


static PyObject *
interp_set___main___attrs(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"id", "updates", "restrict", NULL};
    PyObject *id, *updates;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "OO!|$p:" MODULE_NAME_STR ".set___main___attrs",
                                     kwlist, &id, &PyDict_Type, &updates, &restricted))
    {
        return NULL;
    }

    // Look up the interpreter.
    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "update __main__ for");
    if (interp == NULL) {
        return NULL;
    }

    // Check the updates.
    Py_ssize_t size = PyDict_Size(updates);
    if (size < 0) {
        return NULL;
    }
    if (size == 0) {
        PyErr_SetString(PyExc_ValueError,
                        "arg 2 must be a non-empty dict");
        return NULL;
    }

    _PyXI_session *session = _PyXI_NewSession();
    if (session == NULL) {
        return NULL;
    }

    // Prep and switch interpreters, including apply the updates.
    if (_PyXI_Enter(session, interp, updates, NULL) < 0) {
        _PyXI_FreeSession(session);
        return NULL;
    }

    // Clean up and switch back.
    assert(!PyErr_Occurred());
    int res = _PyXI_Exit(session, NULL, NULL);
    _PyXI_FreeSession(session);
    assert(res == 0);
    if (res < 0) {
        // unreachable
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError, "unresolved error");
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(set___main___attrs_doc,
"set___main___attrs(id, ns, *, restrict=False)\n\
\n\
Bind the given attributes in the interpreter's __main__ module.");


static PyObject *
_handle_script_error(struct run_result *runres)
{
    assert(runres->result == NULL);
    if (runres->excinfo == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    return runres->excinfo;
}

static PyObject *
interp_exec(PyObject *self, PyObject *args, PyObject *kwds)
{
#define FUNCNAME MODULE_NAME_STR ".exec"
    PyThreadState *tstate = _PyThreadState_GET();
    static char *kwlist[] = {"id", "code", "shared", "restrict", NULL};
    PyObject *id, *code;
    PyObject *shared = NULL;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OO|O!$p:" FUNCNAME, kwlist,
                                     &id, &code, &PyDict_Type, &shared,
                                     &restricted))
    {
        return NULL;
    }

    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "exec code for");
    if (interp == NULL) {
        return NULL;
    }

    // We don't need the script to be "pure", which means it can use
    // global variables.  They will be resolved against __main__.
    _PyXIData_t xidata = {0};
    if (_PyCode_GetScriptXIData(tstate, code, &xidata) < 0) {
        unwrap_not_shareable(tstate, NULL);
        return NULL;
    }

    struct run_result runres = {0};
    int res = _run_in_interpreter(
                    tstate, interp, &xidata, NULL, shared, &runres);
    _PyXIData_Release(&xidata);
    if (res < 0) {
        return _handle_script_error(&runres);
    }
    assert(runres.result == NULL);
    Py_RETURN_NONE;
#undef FUNCNAME
}

PyDoc_STRVAR(exec_doc,
"exec(id, code, shared=None, *, restrict=False)\n\
\n\
Execute the provided code in the identified interpreter.\n\
This is equivalent to running the builtin exec() under the target\n\
interpreter, using the __dict__ of its __main__ module as both\n\
globals and locals.\n\
\n\
\"code\" may be a string containing the text of a Python script.\n\
\n\
Functions (and code objects) are also supported, with some restrictions.\n\
The code/function must not take any arguments or be a closure\n\
(i.e. have cell vars).  Methods and other callables are not supported.\n\
\n\
If a function is provided, its code object is used and all its state\n\
is ignored, including its __globals__ dict.");

static PyObject *
interp_run_string(PyObject *self, PyObject *args, PyObject *kwds)
{
#define FUNCNAME MODULE_NAME_STR ".run_string"
    PyThreadState *tstate = _PyThreadState_GET();
    static char *kwlist[] = {"id", "script", "shared", "restrict", NULL};
    PyObject *id, *script;
    PyObject *shared = NULL;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OU|O!$p:" FUNCNAME, kwlist,
                                     &id, &script, &PyDict_Type, &shared,
                                     &restricted))
    {
        return NULL;
    }

    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "run a string in");
    if (interp == NULL) {
        return NULL;
    }

    if (PyFunction_Check(script) || PyCode_Check(script)) {
        _PyArg_BadArgument(FUNCNAME, "argument 2", "a string", script);
        return NULL;
    }

    _PyXIData_t xidata = {0};
    if (_PyCode_GetScriptXIData(tstate, script, &xidata) < 0) {
        unwrap_not_shareable(tstate, NULL);
        return NULL;
    }

    struct run_result runres = {0};
    int res = _run_in_interpreter(
                    tstate, interp, &xidata, NULL, shared, &runres);
    _PyXIData_Release(&xidata);
    if (res < 0) {
        return _handle_script_error(&runres);
    }
    assert(runres.result == NULL);
    Py_RETURN_NONE;
#undef FUNCNAME
}

PyDoc_STRVAR(run_string_doc,
"run_string(id, script, shared=None, *, restrict=False)\n\
\n\
Execute the provided string in the identified interpreter.\n\
\n\
(See " MODULE_NAME_STR ".exec().");

static PyObject *
interp_run_func(PyObject *self, PyObject *args, PyObject *kwds)
{
#define FUNCNAME MODULE_NAME_STR ".run_func"
    PyThreadState *tstate = _PyThreadState_GET();
    static char *kwlist[] = {"id", "func", "shared", "restrict", NULL};
    PyObject *id, *func;
    PyObject *shared = NULL;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OO|O!$p:" FUNCNAME, kwlist,
                                     &id, &func, &PyDict_Type, &shared,
                                     &restricted))
    {
        return NULL;
    }

    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "run a function in");
    if (interp == NULL) {
        return NULL;
    }

    // We don't worry about checking globals.  They will be resolved
    // against __main__.
    PyObject *code;
    if (PyFunction_Check(func)) {
        code = PyFunction_GET_CODE(func);
    }
    else if (PyCode_Check(func)) {
        code = func;
    }
    else {
        _PyArg_BadArgument(FUNCNAME, "argument 2", "a function", func);
        return NULL;
    }

    _PyXIData_t xidata = {0};
    if (_PyCode_GetScriptXIData(tstate, code, &xidata) < 0) {
        unwrap_not_shareable(tstate, NULL);
        return NULL;
    }

    struct run_result runres = {0};
    int res = _run_in_interpreter(
                    tstate, interp, &xidata, NULL, shared, &runres);
    _PyXIData_Release(&xidata);
    if (res < 0) {
        return _handle_script_error(&runres);
    }
    assert(runres.result == NULL);
    Py_RETURN_NONE;
#undef FUNCNAME
}

PyDoc_STRVAR(run_func_doc,
"run_func(id, func, shared=None, *, restrict=False)\n\
\n\
Execute the body of the provided function in the identified interpreter.\n\
Code objects are also supported.  In both cases, closures and args\n\
are not supported.  Methods and other callables are not supported either.\n\
\n\
(See " MODULE_NAME_STR ".exec().");

static PyObject *
interp_call(PyObject *self, PyObject *args, PyObject *kwds)
{
#define FUNCNAME MODULE_NAME_STR ".call"
    PyThreadState *tstate = _PyThreadState_GET();
    static char *kwlist[] = {"id", "callable", "args", "kwargs",
                             "preserve_exc", "restrict", NULL};
    PyObject *id, *callable;
    PyObject *args_obj = NULL;
    PyObject *kwargs_obj = NULL;
    int preserve_exc = 0;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OO|O!O!$pp:" FUNCNAME, kwlist,
                                     &id, &callable,
                                     &PyTuple_Type, &args_obj,
                                     &PyDict_Type, &kwargs_obj,
                                     &preserve_exc, &restricted))
    {
        return NULL;
    }

    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "make a call in");
    if (interp == NULL) {
        return NULL;
    }

    struct interp_call call = {0};
    if (_interp_call_pack(tstate, &call, callable, args_obj, kwargs_obj) < 0) {
        return NULL;
    }

    PyObject *res_and_exc = NULL;
    struct run_result runres = {0};
    if (_run_in_interpreter(tstate, interp, NULL, &call, NULL, &runres) < 0) {
        if (runres.excinfo == NULL) {
            assert(_PyErr_Occurred(tstate));
            goto finally;
        }
        assert(!_PyErr_Occurred(tstate));
    }
    assert(runres.result == NULL || runres.excinfo == NULL);
    res_and_exc = Py_BuildValue("OO",
                                (runres.result ? runres.result : Py_None),
                                (runres.excinfo ? runres.excinfo : Py_None));

finally:
    _interp_call_clear(&call);
    _run_result_clear(&runres);
    return res_and_exc;
#undef FUNCNAME
}

PyDoc_STRVAR(call_doc,
"call(id, callable, args=None, kwargs=None, *, restrict=False)\n\
\n\
Call the provided object in the identified interpreter.\n\
Pass the given args and kwargs, if possible.");


static PyObject *
object_is_shareable(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", NULL};
    PyObject *obj;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:is_shareable", kwlist, &obj)) {
        return NULL;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyObject_CheckXIData(tstate, obj) == 0) {
        Py_RETURN_TRUE;
    }
    PyErr_Clear();
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_shareable_doc,
"is_shareable(obj) -> bool\n\
\n\
Return True if the object's data may be shared between interpreters and\n\
False otherwise.");


static PyObject *
interp_is_running(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "restrict", NULL};
    PyObject *id;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|$p:is_running", kwlist,
                                     &id, &restricted))
    {
        return NULL;
    }

    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "check if running for");
    if (interp == NULL) {
        return NULL;
    }

    if (is_running_main(interp)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_running_doc,
"is_running(id, *, restrict=False) -> bool\n\
\n\
Return whether or not the identified interpreter is running.");


static PyObject *
interp_get_config(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "restrict", NULL};
    PyObject *idobj = NULL;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O?|$p:get_config", kwlist,
                                     &idobj, &restricted))
    {
        return NULL;
    }

    int reqready = 0;
    PyInterpreterState *interp = \
            resolve_interp(idobj, restricted, reqready, "get the config of");
    if (interp == NULL) {
        return NULL;
    }

    PyInterpreterConfig config;
    if (_PyInterpreterConfig_InitFromState(&config, interp) < 0) {
        return NULL;
    }
    PyObject *dict = _PyInterpreterConfig_AsDict(&config);
    if (dict == NULL) {
        return NULL;
    }

    PyObject *configobj = _PyNamespace_New(dict);
    Py_DECREF(dict);
    return configobj;
}

PyDoc_STRVAR(get_config_doc,
"get_config(id, *, restrict=False) -> types.SimpleNamespace\n\
\n\
Return a representation of the config used to initialize the interpreter.");


static PyObject *
interp_whence(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    PyObject *id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:whence", kwlist, &id))
    {
        return NULL;
    }

    PyInterpreterState *interp = look_up_interp(id);
    if (interp == NULL) {
        return NULL;
    }

    long whence = get_whence(interp);
    return PyLong_FromLong(whence);
}

PyDoc_STRVAR(whence_doc,
"whence(id) -> int\n\
\n\
Return an identifier for where the interpreter was created.");


static PyObject *
interp_incref(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "implieslink", "restrict", NULL};
    PyObject *id;
    int implieslink = 0;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|$pp:incref", kwlist,
                                     &id, &implieslink, &restricted))
    {
        return NULL;
    }

    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "incref");
    if (interp == NULL) {
        return NULL;
    }

    if (implieslink) {
        // Decref to 0 will destroy the interpreter.
        _PyInterpreterState_RequireIDRef(interp, 1);
    }
    _PyInterpreterState_IDIncref(interp);

    Py_RETURN_NONE;
}


static PyObject *
interp_decref(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "restrict", NULL};
    PyObject *id;
    int restricted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|$p:decref", kwlist, &id, &restricted))
    {
        return NULL;
    }

    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "decref");
    if (interp == NULL) {
        return NULL;
    }

    _PyInterpreterState_IDDecref(interp);

    Py_RETURN_NONE;
}


static PyObject *
capture_exception(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"exc", NULL};
    PyObject *exc_arg = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|O?:capture_exception", kwlist,
                                     &exc_arg))
    {
        return NULL;
    }

    PyObject *exc = exc_arg;
    if (exc == NULL) {
        exc = PyErr_GetRaisedException();
        if (exc == NULL) {
            Py_RETURN_NONE;
        }
    }
    else if (!PyExceptionInstance_Check(exc)) {
        PyErr_Format(PyExc_TypeError, "expected exception, got %R", exc);
        return NULL;
    }
    PyObject *captured = NULL;

    _PyXI_excinfo *info = _PyXI_NewExcInfo(exc);
    if (info == NULL) {
        goto finally;
    }
    captured = _PyXI_ExcInfoAsObject(info);
    if (captured == NULL) {
        goto finally;
    }

    PyObject *formatted = _PyXI_FormatExcInfo(info);
    if (formatted == NULL) {
        Py_CLEAR(captured);
        goto finally;
    }
    int res = PyObject_SetAttrString(captured, "formatted", formatted);
    Py_DECREF(formatted);
    if (res < 0) {
        Py_CLEAR(captured);
        goto finally;
    }

finally:
    _PyXI_FreeExcInfo(info);
    if (exc != exc_arg) {
        if (PyErr_Occurred()) {
            PyErr_SetRaisedException(exc);
        }
        else {
            _PyErr_ChainExceptions1(exc);
        }
    }
    return captured;
}

PyDoc_STRVAR(capture_exception_doc,
"capture_exception(exc=None) -> types.SimpleNamespace\n\
\n\
Return a snapshot of an exception.  If \"exc\" is None\n\
then the current exception, if any, is used (but not cleared).\n\
\n\
The returned snapshot is the same as what _interpreters.exec() returns.");


static PyMethodDef module_functions[] = {
    {"new_config",                _PyCFunction_CAST(interp_new_config),
     METH_VARARGS | METH_KEYWORDS, new_config_doc},

    {"create",                    _PyCFunction_CAST(interp_create),
     METH_VARARGS | METH_KEYWORDS, create_doc},
    {"destroy",                   _PyCFunction_CAST(interp_destroy),
     METH_VARARGS | METH_KEYWORDS, destroy_doc},
    {"list_all",                  _PyCFunction_CAST(interp_list_all),
     METH_VARARGS | METH_KEYWORDS, list_all_doc},
    {"get_current",               interp_get_current,
     METH_NOARGS, get_current_doc},
    {"get_main",                  interp_get_main,
     METH_NOARGS, get_main_doc},

    {"is_running",                _PyCFunction_CAST(interp_is_running),
     METH_VARARGS | METH_KEYWORDS, is_running_doc},
    {"get_config",                _PyCFunction_CAST(interp_get_config),
     METH_VARARGS | METH_KEYWORDS, get_config_doc},
    {"whence",                    _PyCFunction_CAST(interp_whence),
     METH_VARARGS | METH_KEYWORDS, whence_doc},
    {"exec",                      _PyCFunction_CAST(interp_exec),
     METH_VARARGS | METH_KEYWORDS, exec_doc},
    {"call",                      _PyCFunction_CAST(interp_call),
     METH_VARARGS | METH_KEYWORDS, call_doc},
    {"run_string",                _PyCFunction_CAST(interp_run_string),
     METH_VARARGS | METH_KEYWORDS, run_string_doc},
    {"run_func",                  _PyCFunction_CAST(interp_run_func),
     METH_VARARGS | METH_KEYWORDS, run_func_doc},

    {"set___main___attrs",        _PyCFunction_CAST(interp_set___main___attrs),
     METH_VARARGS | METH_KEYWORDS, set___main___attrs_doc},

    {"incref",                    _PyCFunction_CAST(interp_incref),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"decref",                    _PyCFunction_CAST(interp_decref),
     METH_VARARGS | METH_KEYWORDS, NULL},

    {"is_shareable",              _PyCFunction_CAST(object_is_shareable),
     METH_VARARGS | METH_KEYWORDS, is_shareable_doc},

    {"capture_exception",         _PyCFunction_CAST(capture_exception),
     METH_VARARGS | METH_KEYWORDS, capture_exception_doc},

    {NULL,                        NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static int
module_exec(PyObject *mod)
{
    PyThreadState *tstate = _PyThreadState_GET();
    module_state *state = get_module_state(mod);

#define ADD_WHENCE(NAME) \
    if (PyModule_AddIntConstant(mod, "WHENCE_" #NAME,                   \
                                _PyInterpreterState_WHENCE_##NAME) < 0) \
    {                                                                   \
        goto error;                                                     \
    }
    ADD_WHENCE(UNKNOWN)
    ADD_WHENCE(RUNTIME)
    ADD_WHENCE(LEGACY_CAPI)
    ADD_WHENCE(CAPI)
    ADD_WHENCE(XI)
    ADD_WHENCE(STDLIB)
#undef ADD_WHENCE

    // exceptions
    if (PyModule_AddType(mod, (PyTypeObject *)PyExc_InterpreterError) < 0) {
        goto error;
    }
    if (PyModule_AddType(mod, (PyTypeObject *)PyExc_InterpreterNotFoundError) < 0) {
        goto error;
    }
    PyObject *exctype = _PyXIData_GetNotShareableErrorType(tstate);
    if (PyModule_AddType(mod, (PyTypeObject *)exctype) < 0) {
        goto error;
    }

    if (register_memoryview_xid(mod, &state->XIBufferViewType) < 0) {
        goto error;
    }

    return 0;

error:
    return -1;
}

static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    return traverse_module_state(state, visit, arg);
}

static int
module_clear(PyObject *mod)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    return clear_module_state(state);
}

static void
module_free(void *mod)
{
    module_state *state = get_module_state((PyObject *)mod);
    assert(state != NULL);
    (void)clear_module_state(state);
}

static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME_STR,
    .m_doc = module_doc,
    .m_size = sizeof(module_state),
    .m_methods = module_functions,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = module_free,
};

PyMODINIT_FUNC
MODINIT_FUNC_NAME(void)
{
    return PyModuleDef_Init(&moduledef);
}
