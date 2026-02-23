/* interpreters module */
/* low-level access to interpreter primitives */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_GetANext()
#include "pycore_code.h"          // _PyCode_HAS_EXECUTORS()
#include "pycore_crossinterp.h"   // _PyXIData_t
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_interp.h"        // _PyInterpreterState_IDIncref()
#include "pycore_genobject.h"     // _PyCoro_GetAwaitableIter()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_namespace.h"     // _PyNamespace_New()
#include "pycore_pybuffer.h"      // _PyBuffer_ReleaseInInterpreterAndRawFree()
#include "pycore_pylifecycle.h"   // _PyInterpreterConfig_AsDict()
#include "pycore_pystate.h"       // _PyInterpreterState_IsRunningMain()
#include "pycore_runtime_structs.h"
#include "pycore_pyatomic_ft_wrappers.h"

#include "marshal.h"              // PyMarshal_ReadObjectFromString()

#include "_interpreters_common.h"

#include "clinic/_interpretersmodule.c.h"

#define MODULE_NAME _interpreters
#define MODULE_NAME_STR Py_STRINGIFY(MODULE_NAME)
#define MODINIT_FUNC_NAME RESOLVE_MODINIT_FUNC_NAME(MODULE_NAME)


/*[clinic input]
module _interpreters
class _interpreters.SharedObjectProxy "SharedObjectProxy *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4bb543de3f19aa0b]*/

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
    PyTypeObject *SharedObjectProxyType;
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
    Py_VISIT(state->SharedObjectProxyType);

    return 0;
}

static int
clear_module_state(module_state *state)
{
    /* heap types */
    Py_CLEAR(state->XIBufferViewType);
    Py_CLEAR(state->SharedObjectProxyType);

    return 0;
}

/* Shared object proxies. */

typedef struct _Py_shared_object_proxy {
    PyObject_HEAD
    PyInterpreterState *interp;
    PyObject *object;
#ifdef Py_GIL_DISABLED
    struct {
        _Py_hashtable_t *table;
        PyMutex mutex;
    } thread_states;
#else
    _Py_hashtable_t *thread_states;
#endif
} SharedObjectProxy;

static PyTypeObject *
_get_current_sharedobjectproxy_type(void);
#define SharedObjectProxy_CAST(op) ((SharedObjectProxy *)op)
#define SharedObjectProxy_OBJECT(op) FT_ATOMIC_LOAD_PTR_RELAXED(SharedObjectProxy_CAST(op)->object)

typedef struct {
    PyThreadState *to_restore;
    PyThreadState *for_call;
} _PyXI_proxy_state;

static int
_sharedobjectproxy_enter(SharedObjectProxy *self, _PyXI_proxy_state *state)
{
    PyThreadState *tstate = _PyThreadState_GET();
    assert(self != NULL);
    assert(tstate != NULL);
    if (tstate->interp == self->interp) {
        // No need to switch; already in the correct interpreter
        state->to_restore = NULL;
        state->for_call = NULL;
        return 0;
    }
    state->to_restore = tstate;
    PyThreadState *for_call = _PyThreadState_NewForExec(self->interp);
    state->for_call = for_call;
    if (for_call == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    _PyThreadState_Detach(tstate);
    _PyThreadState_Attach(state->for_call);
    assert(_PyInterpreterState_GET() == self->interp);
    return 0;
}

static int
_sharedobjectproxy_exit(SharedObjectProxy *self, _PyXI_proxy_state *state)
{
    assert(_PyInterpreterState_GET() == self->interp);
    if (state->to_restore == NULL) {
        // Nothing to do. We were already in the correct interpreter.
        return PyErr_Occurred() == NULL ? 0 : -1;
    }

    PyThreadState *tstate = state->for_call;
    int should_throw = 0;
    if (_PyErr_Occurred(tstate)) {
        // TODO: Serialize and transfer the exception to the calling
        // interpreter.
        PyErr_FormatUnraisable("Exception occured in interpreter");
        should_throw = 1;
    }

    assert(state->for_call == _PyThreadState_GET());
    PyThreadState_Swap(state->to_restore);
    // If we created a new thread state, we don't want to delete it.
    // It's likely that it will be used again, but if not, the interpreter
    // will clean it up at the end anyway.

    if (should_throw) {
        _PyErr_SetString(state->to_restore, PyExc_RuntimeError, "exception in interpreter");
        return -1;
    }

    return 0;
}

static SharedObjectProxy *
sharedobjectproxy_alloc(PyTypeObject *type)
{
    assert(type != NULL);
    assert(PyType_Check(type));
    SharedObjectProxy *self = SharedObjectProxy_CAST(type->tp_alloc(type, 0));
    if (self == NULL) {
        return NULL;
    }

    self->interp = _PyInterpreterState_GET();
#ifndef NDEBUG
    self->object = NULL;
#endif

    return self;
}

/*[clinic input]
@classmethod
_interpreters.SharedObjectProxy.__new__ as sharedobjectproxy_new

    obj: object,
    /

Create a new cross-interpreter proxy.
[clinic start generated code]*/

static PyObject *
sharedobjectproxy_new_impl(PyTypeObject *type, PyObject *obj)
/*[clinic end generated code: output=42ed0a0bc47ecedf input=fce004d93517c6df]*/
{
    SharedObjectProxy *self = sharedobjectproxy_alloc(type);
    if (self == NULL) {
        return NULL;
    }

    self->object = Py_NewRef(obj);
    return (PyObject *)self;
}

PyObject *
_sharedobjectproxy_create(PyObject *object, PyInterpreterState *owning_interp)
{
    assert(object != NULL);
    assert(owning_interp != NULL);

    PyTypeObject *type = _get_current_sharedobjectproxy_type();
    if (type == NULL) {
        return NULL;
    }
    assert(Py_TYPE(object) != type);
    SharedObjectProxy *proxy = sharedobjectproxy_alloc(type);
    if (proxy == NULL) {
        return NULL;
    }

    assert(PyObject_GC_IsTracked((PyObject *)proxy));
    proxy->object = NULL;
    proxy->interp = owning_interp;

    // We have to be in the correct interpreter to increment the object's
    // reference count.
    _PyXI_proxy_state state;
    if (_sharedobjectproxy_enter(proxy, &state) < 0) {
        Py_DECREF(proxy);
        return NULL;
    }

    proxy->object = Py_NewRef(object);

    if (_sharedobjectproxy_exit(proxy, &state)) {
        Py_DECREF(proxy);
        return NULL;
    }

    return (PyObject *)proxy;
}


static int
sharedobjectproxy_clear(PyObject *op)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    if (self->object == NULL) {
        return 0;
    }

    _PyXI_proxy_state state;
    if (_sharedobjectproxy_enter(self, &state) < 0) {
        // The object leaks :(
        return -1;
    }
    Py_CLEAR(self->object);
    return _sharedobjectproxy_exit(self, &state);
}

static int
sharedobjectproxy_traverse(PyObject *op, visitproc visit, void *arg)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    if (self->interp != _PyInterpreterState_GET()) {
        // Don't traverse from another interpreter
        return 0;
    }

    Py_VISIT(self->object);
    return 0;
}

static void
sharedobjectproxy_dealloc(PyObject *op)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *err = PyErr_GetRaisedException();
    if (sharedobjectproxy_clear(op) < 0) {
        PyErr_FormatUnraisable("Exception in proxy destructor");
    };
    PyObject_GC_UnTrack(self);
    tp->tp_free(self);
    Py_DECREF(tp);
    PyErr_SetRaisedException(err);
}

typedef struct {
    _PyXIData_t *xidata;
    PyObject *object_to_wrap;
    PyInterpreterState *owner;
} _PyXI_proxy_share;

/* Use this in the calling interpreter. */
static int
_sharedobjectproxy_init_share(_PyXI_proxy_share *share,
                              SharedObjectProxy *self, PyObject *op)
{
    assert(op != NULL);
    assert(share != NULL);
    _PyXIData_t *xidata = _PyXIData_New();
    if (xidata == NULL) {
        return -1;
    }
    share->owner = _PyInterpreterState_GET();

    if (_PyObject_GetXIData(_PyThreadState_GET(), op,
                            _PyXIDATA_XIDATA_ONLY, xidata) < 0) {
        PyErr_Clear();
        share->object_to_wrap = Py_NewRef(op);
        share->xidata = NULL;
        PyMem_RawFree(xidata);
    } else {
        share->object_to_wrap = NULL;
        share->xidata = xidata;
    }

    return 0;
}

/* Use this in the switched interpreter. */
static PyObject *
_sharedobjectproxy_copy_for_interp(_PyXI_proxy_share *share)
{
    assert(share != NULL);
    _PyXIData_t *xidata = share->xidata;
    if (xidata == NULL) {
        // Not shareable; use an object proxy
        return _sharedobjectproxy_create(share->object_to_wrap, share->owner);
    } else {
        assert(share->object_to_wrap == NULL);
        PyObject *result = _PyXIData_NewObject(xidata);
        return result;
    }
}

static void
_sharedobjectproxy_finish_share(_PyXI_proxy_share *share)
{
    if (share->xidata != NULL) {
        assert(share->object_to_wrap == NULL);
        _PyXIData_Free(share->xidata);
    } else {
        assert(share->object_to_wrap != NULL);
        Py_DECREF(share->object_to_wrap);
    }
#ifdef Py_DEBUG
    share->xidata = NULL;
    share->object_to_wrap = NULL;
#endif
}

static PyObject *
_sharedobjectproxy_wrap_result(SharedObjectProxy *self, PyObject *result,
                               _PyXI_proxy_state *state)
{
    if (result == NULL) {
        (void)_sharedobjectproxy_exit(self, state);
        return NULL;
    }

    assert(result != NULL);
    _PyXI_proxy_share shared_result;
    if (_sharedobjectproxy_init_share(&shared_result, self, result) < 0) {
        Py_DECREF(result);
        (void)_sharedobjectproxy_exit(self, state);
        return NULL;
    }

    Py_DECREF(result);
    PyObject *ret;
    if (state->to_restore != NULL) {
        PyThreadState *save = PyThreadState_Swap(state->to_restore);
        ret = _sharedobjectproxy_copy_for_interp(&shared_result);
        PyThreadState_Swap(save);
    } else {
        ret = _sharedobjectproxy_copy_for_interp(&shared_result);
    }

    _sharedobjectproxy_finish_share(&shared_result);
    if (_sharedobjectproxy_exit(self, state) < 0) {
        return NULL;
    }

    return ret;
}

static _PyXI_proxy_share *
_sharedobjectproxy_init_shared_args(PyObject *args, SharedObjectProxy *self)
{
    assert(PyTuple_Check(args));
    Py_ssize_t args_size = PyTuple_GET_SIZE(args);
    _PyXI_proxy_share *shared_args_state = PyMem_RawCalloc(args_size, sizeof(_PyXI_proxy_share));
    if (shared_args_state == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    for (Py_ssize_t i = 0; i < args_size; ++i) {
        PyObject *arg = PyTuple_GET_ITEM(args, i);
        if (_sharedobjectproxy_init_share(&shared_args_state[i], self, arg) < 0) {
            PyMem_RawFree(shared_args_state);
            for (int x = 0; x < i; ++x) {
                _sharedobjectproxy_finish_share(&shared_args_state[i]);
            }
            return NULL;
        }
    }
    return shared_args_state;
}

static void
_sharedobjectproxy_close_shared_args(PyObject *args, _PyXI_proxy_share *shared_args_state)
{
    Py_ssize_t args_size = PyTuple_GET_SIZE(args);
    for (Py_ssize_t i = 0; i < args_size; ++i) {
        _sharedobjectproxy_finish_share(&shared_args_state[i]);
    }
    PyMem_RawFree(shared_args_state);
}

static PyObject *
_sharedobjectproxy_construct_shared_args(PyObject *args, _PyXI_proxy_share *shared_args_state)
{
    Py_ssize_t args_size = PyTuple_GET_SIZE(args);
    PyObject *shared_args = PyTuple_New(args_size);
    if (shared_args == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < args_size; ++i) {
        PyObject *shared = _sharedobjectproxy_copy_for_interp(&shared_args_state[i]);
        if (shared == NULL) {
            Py_DECREF(shared_args);
            return NULL;
        }
        PyTuple_SET_ITEM(shared_args, i, shared);
    }

    return shared_args;
}

typedef struct {
    const char *key;
    Py_ssize_t key_length;
    _PyXI_proxy_share value;
} _SharedObjectProxy_dict_pair;

static int
_sharedobjectproxy_init_shared_kwargs(PyObject *kwargs, SharedObjectProxy *self,
                                      _SharedObjectProxy_dict_pair **kwarg_pairs)
{
    if (kwargs == NULL) {
        return 0;
    }
    Py_ssize_t kwarg_size = PyDict_GET_SIZE(kwargs);
    *kwarg_pairs = PyMem_RawCalloc(kwarg_size, sizeof(_SharedObjectProxy_dict_pair));
    if (*kwarg_pairs == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        // XXX Can kwarg keys be dictionary subclasses?
        assert(PyUnicode_Check(key));
        Py_ssize_t index = pos - 1;
        assert(index >= 0);
        assert(index < kwarg_size);
        _SharedObjectProxy_dict_pair *pair = kwarg_pairs[index];
        assert(pair->key == NULL);
        assert(pair->key_length == 0);
        const char *key_str = PyUnicode_AsUTF8AndSize(key, &pair->key_length);
        if (key_str == NULL) {
            for (Py_ssize_t i = 0; i < pos; ++i) {
                _sharedobjectproxy_finish_share(&kwarg_pairs[i]->value);
            }
            PyMem_RawFree(kwarg_pairs);
            return -1;

        }
        pair->key = key_str;
        if (_sharedobjectproxy_init_share(&pair->value, self, value) < 0) {
            for (Py_ssize_t i = 0; i < pos; ++i) {
                _sharedobjectproxy_finish_share(&kwarg_pairs[i]->value);
            }
            PyMem_RawFree(kwarg_pairs);
            return -1;
        }
    }

    return 0;
}

static PyObject *
_sharedobjectproxy_construct_shared_kwargs(PyObject *kwargs, _SharedObjectProxy_dict_pair *pairs)
{
    if (kwargs == NULL) {
        return NULL;
    }
    PyObject *shared_kwargs = PyDict_New();
    if (shared_kwargs == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < PyDict_GET_SIZE(kwargs); ++i) {
        _SharedObjectProxy_dict_pair *pair = &pairs[i];
        assert(pair->key != NULL);
        PyObject *key = PyUnicode_FromStringAndSize(pair->key, pair->key_length);
        if (key == NULL) {
            Py_DECREF(shared_kwargs);
            return NULL;
        }
        PyObject *shared_kwarg = _sharedobjectproxy_copy_for_interp(&pair->value);
        if (shared_kwarg == NULL) {
            Py_DECREF(shared_kwargs);
            Py_DECREF(key);
            return NULL;
        }
        int res = PyDict_SetItem(shared_kwargs, key, shared_kwarg);
        Py_DECREF(key);
        Py_DECREF(shared_kwarg);
        if (res < 0) {
            Py_DECREF(shared_kwargs);
            return NULL;
        }
    }

    return shared_kwargs;
}

static void
_sharedobjectproxy_close_shared_kwargs(PyObject *kwargs, _SharedObjectProxy_dict_pair *pairs)
{
    if (kwargs == NULL) {
        return;
    }
    Py_ssize_t size = PyDict_GET_SIZE(kwargs);
    for (Py_ssize_t i = 0; i < size; ++i) {
        _sharedobjectproxy_finish_share(&pairs[i].value);
    }
    PyMem_RawFree(pairs);
}

static PyObject *
sharedobjectproxy_tp_call(PyObject *op, PyObject *args, PyObject *kwargs) {
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_share *args_state = _sharedobjectproxy_init_shared_args(args, self);
    if (args_state == NULL) {
        return NULL;
    }

    _SharedObjectProxy_dict_pair *kwarg_pairs;
    if (_sharedobjectproxy_init_shared_kwargs(kwargs, self, &kwarg_pairs) < 0) {
        _sharedobjectproxy_close_shared_args(args, args_state);
        return NULL;
    }

    _PyXI_proxy_state state;
    if (_sharedobjectproxy_enter(self, &state) < 0) {
        return NULL;
    }

    PyObject *shared_args = _sharedobjectproxy_construct_shared_args(args, args_state);
    if (shared_args == NULL) {
        (void)_sharedobjectproxy_exit(self, &state);
        _sharedobjectproxy_close_shared_args(args, args_state);
        _sharedobjectproxy_close_shared_kwargs(kwargs, kwarg_pairs);
        return NULL;
    }


    PyObject *shared_kwargs = _sharedobjectproxy_construct_shared_kwargs(kwargs, kwarg_pairs);
    if (shared_kwargs == NULL && PyErr_Occurred()) {
        Py_DECREF(shared_args);
        (void)_sharedobjectproxy_exit(self, &state);
        _sharedobjectproxy_close_shared_args(args, args_state);
        _sharedobjectproxy_close_shared_kwargs(kwargs, kwarg_pairs);
        return NULL;
    }

    PyObject *res = PyObject_Call(SharedObjectProxy_OBJECT(self),
                                  shared_args, shared_kwargs);
    Py_DECREF(shared_args);
    Py_XDECREF(shared_kwargs);

    PyObject *ret = _sharedobjectproxy_wrap_result(self, res, &state);
    _sharedobjectproxy_close_shared_args(args, args_state);
    _sharedobjectproxy_close_shared_kwargs(kwargs, kwarg_pairs);
    return ret;
}

static PyObject *
_sharedobjectproxy_no_arg(PyObject *op, unaryfunc call)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    if (_sharedobjectproxy_enter(self, &state) < 0) {
        return NULL;
    }

    PyObject *result = call(SharedObjectProxy_OBJECT(op));
    return _sharedobjectproxy_wrap_result(self, result, &state);
}

static PyObject *
_sharedobjectproxy_single_share_common(SharedObjectProxy *self, PyObject *to_share,
                                       _PyXI_proxy_state *state,
                                       _PyXI_proxy_share *shared_arg)
{
    if (_sharedobjectproxy_init_share(shared_arg, self, to_share) < 0) {
        return NULL;
    }
    if (_sharedobjectproxy_enter(self, state) < 0) {
        _sharedobjectproxy_finish_share(shared_arg);
        return NULL;
    }
    PyObject *shared_obj = _sharedobjectproxy_copy_for_interp(shared_arg);
    if (shared_obj == NULL) {
        (void)_sharedobjectproxy_exit(self, state);
        _sharedobjectproxy_finish_share(shared_arg);
        return NULL;
    }
    return shared_obj;
}

static PyObject *
_sharedobjectproxy_single_share(PyObject *op, PyObject *other, binaryfunc call)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    _PyXI_proxy_share shared_arg;
    PyObject *shared_obj = _sharedobjectproxy_single_share_common(self, other,
                                                                  &state, &shared_arg);
    if (shared_obj == NULL) {
        return NULL;
    }
    PyObject *result = call(SharedObjectProxy_OBJECT(op), shared_obj);
    Py_DECREF(shared_obj);
    PyObject *ret = _sharedobjectproxy_wrap_result(self, result, &state);
    _sharedobjectproxy_finish_share(&shared_arg);
    return ret;
}

static int
_sharedobjectproxy_double_share_common(SharedObjectProxy *self,
                                       _PyXI_proxy_state *state,
                                       _PyXI_proxy_share *shared_first,
                                       PyObject *first,
                                       PyObject **first_ptr,
                                       _PyXI_proxy_share *shared_second,
                                       PyObject *second,
                                       PyObject **second_ptr)
{
    if (_sharedobjectproxy_init_share(shared_first, self, first) < 0) {
        return -1;
    }
    if (_sharedobjectproxy_init_share(shared_second, self, second) < 0) {
        _sharedobjectproxy_finish_share(shared_first);
        return -1;
    }
    if (_sharedobjectproxy_enter(self, state) < 0) {
        _sharedobjectproxy_finish_share(shared_first);
        _sharedobjectproxy_finish_share(shared_second);
        return -1;
    }
    PyObject *first_obj = _sharedobjectproxy_copy_for_interp(shared_first);
    if (first_obj == NULL) {
        (void)_sharedobjectproxy_exit(self, state);
        _sharedobjectproxy_finish_share(shared_first);
        _sharedobjectproxy_finish_share(shared_second);
        return -1;
    }
    PyObject *second_obj = _sharedobjectproxy_copy_for_interp(shared_second);
    if (second_obj == NULL) {
        Py_DECREF(first_obj);
        (void)_sharedobjectproxy_exit(self, state);
        _sharedobjectproxy_finish_share(shared_first);
        _sharedobjectproxy_finish_share(shared_second);
        return -1;
    }

    *first_ptr = first_obj;
    *second_ptr = second_obj;
    return 0;
}

static PyObject *
_sharedobjectproxy_double_share(PyObject *op, PyObject *first,
                                PyObject *second, ternaryfunc call)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    _PyXI_proxy_share shared_first;
    _PyXI_proxy_share shared_second;
    PyObject *first_obj;
    PyObject *second_obj;
    if (_sharedobjectproxy_double_share_common(self, &state, &shared_first,
                                               first, &first_obj, &shared_second,
                                               second, &second_obj) < 0) {
        return NULL;
    }
    PyObject *result = call(SharedObjectProxy_OBJECT(op), first_obj, second_obj);
    Py_DECREF(first_obj);
    Py_DECREF(second_obj);
    PyObject *ret = _sharedobjectproxy_wrap_result(self, result, &state);
    _sharedobjectproxy_finish_share(&shared_first);
    _sharedobjectproxy_finish_share(&shared_second);
    return ret;
}

static int
_sharedobjectproxy_double_share_int(PyObject *op, PyObject *first,
                                    PyObject *second, objobjargproc call)
{

    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    _PyXI_proxy_share shared_first;
    _PyXI_proxy_share shared_second;
    PyObject *first_obj;
    PyObject *second_obj;
    if (_sharedobjectproxy_double_share_common(self, &state, &shared_first,
                                               first, &first_obj, &shared_second,
                                               second, &second_obj) < 0) {
        return -1;
    }
    int result = call(SharedObjectProxy_OBJECT(op), first_obj, second_obj);
    Py_DECREF(first_obj);
    Py_DECREF(second_obj);
    if (_sharedobjectproxy_exit(self, &state) < 0) {
        _sharedobjectproxy_finish_share(&shared_first);
        _sharedobjectproxy_finish_share(&shared_second);
        return -1;
    }
    _sharedobjectproxy_finish_share(&shared_first);
    _sharedobjectproxy_finish_share(&shared_second);
    return result;
}


static PyObject *
_sharedobjectproxy_ssize_arg(PyObject *op, Py_ssize_t count, ssizeargfunc call)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    if (_sharedobjectproxy_enter(self, &state) < 0) {
        return NULL;
    }
    PyObject *result = call(SharedObjectProxy_OBJECT(op), count);
    return _sharedobjectproxy_wrap_result(self, result, &state);
}

static Py_ssize_t
_sharedobjectproxy_ssize_result(PyObject *op, lenfunc call)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    if (_sharedobjectproxy_enter(self, &state) < 0) {
        return -1;
    }
    Py_ssize_t result = call(SharedObjectProxy_OBJECT(op));
    if (_sharedobjectproxy_exit(self, &state) < 0) {
        return -1;
    }

    return result;
}


#define _SharedObjectProxy_ONE_ARG(name, func)              \
static PyObject *                                           \
sharedobjectproxy_ ##name(PyObject *op, PyObject *other)    \
{                                                           \
    return _sharedobjectproxy_single_share(op, other, func);\
}                                                           \

#define _SharedObjectProxy_TWO_ARG(name, func)                              \
static PyObject *                                                           \
sharedobjectproxy_ ##name(PyObject *op, PyObject *first, PyObject *second)  \
{                                                                           \
    return _sharedobjectproxy_double_share(op, first, second, func);        \
}                                                                           \

#define _SharedObjectProxy_TWO_ARG_INT(name, func)                          \
static int                                                                  \
sharedobjectproxy_ ##name(PyObject *op, PyObject *first, PyObject *second)  \
{                                                                           \
    return _sharedobjectproxy_double_share_int(op, first, second, func);    \
}                                                                           \

#define _SharedObjectProxy_NO_ARG(name, func)   \
static PyObject *                               \
sharedobjectproxy_ ##name(PyObject *op)         \
{                                               \
    return _sharedobjectproxy_no_arg(op, func); \
}                                               \

#define _SharedObjectProxy_SSIZE_ARG(name, func)            \
static PyObject *                                           \
sharedobjectproxy_ ##name(PyObject *op, Py_ssize_t count)   \
{                                                           \
    return _sharedobjectproxy_ssize_arg(op, count, func);   \
}

#define _SharedObjectProxy_SSIZE_RETURN(name, func)     \
static Py_ssize_t                                       \
sharedobjectproxy_ ##name(PyObject *op)                 \
{                                                       \
    return _sharedobjectproxy_ssize_result(op, func);   \
}

_SharedObjectProxy_NO_ARG(tp_iter, PyObject_GetIter);
_SharedObjectProxy_NO_ARG(tp_iternext, PyIter_Next);
_SharedObjectProxy_NO_ARG(tp_str, PyObject_Str);
_SharedObjectProxy_NO_ARG(tp_repr, PyObject_Repr);
_SharedObjectProxy_ONE_ARG(tp_getattro, PyObject_GetAttr);
_SharedObjectProxy_TWO_ARG_INT(tp_setattro, PyObject_SetAttr);

static Py_hash_t
sharedobjectproxy_tp_hash(PyObject *op)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    if (_sharedobjectproxy_enter(self, &state) < 0) {
        return -1;
    }

    Py_hash_t result = PyObject_Hash(SharedObjectProxy_OBJECT(op));

    if (_sharedobjectproxy_exit(self, &state) < 0) {
        return -1;
    }

    return result;
}

/* Number wrappers */
_SharedObjectProxy_ONE_ARG(nb_add, PyNumber_Add);
_SharedObjectProxy_ONE_ARG(nb_subtract, PyNumber_Subtract);
_SharedObjectProxy_ONE_ARG(nb_multiply, PyNumber_Multiply);
_SharedObjectProxy_ONE_ARG(nb_remainder, PyNumber_Remainder);
_SharedObjectProxy_ONE_ARG(nb_divmod, PyNumber_Divmod);
_SharedObjectProxy_TWO_ARG(nb_power, PyNumber_Power);
_SharedObjectProxy_NO_ARG(nb_negative, PyNumber_Negative);
_SharedObjectProxy_NO_ARG(nb_positive, PyNumber_Positive);
_SharedObjectProxy_NO_ARG(nb_absolute, PyNumber_Absolute);
_SharedObjectProxy_NO_ARG(nb_invert, PyNumber_Invert);
_SharedObjectProxy_ONE_ARG(nb_lshift, PyNumber_Lshift);
_SharedObjectProxy_ONE_ARG(nb_rshift, PyNumber_Rshift);
_SharedObjectProxy_ONE_ARG(nb_and, PyNumber_And);
_SharedObjectProxy_ONE_ARG(nb_xor, PyNumber_Xor);
_SharedObjectProxy_ONE_ARG(nb_or, PyNumber_Or);
_SharedObjectProxy_NO_ARG(nb_int, PyNumber_Long);
_SharedObjectProxy_NO_ARG(nb_float, PyNumber_Float);
_SharedObjectProxy_ONE_ARG(nb_inplace_add, PyNumber_InPlaceAdd);
_SharedObjectProxy_ONE_ARG(nb_inplace_subtract, PyNumber_InPlaceSubtract);
_SharedObjectProxy_ONE_ARG(nb_inplace_multiply, PyNumber_InPlaceMultiply);
_SharedObjectProxy_ONE_ARG(nb_inplace_remainder, PyNumber_InPlaceRemainder);
_SharedObjectProxy_TWO_ARG(nb_inplace_power, PyNumber_InPlacePower);
_SharedObjectProxy_ONE_ARG(nb_inplace_lshift, PyNumber_InPlaceLshift);
_SharedObjectProxy_ONE_ARG(nb_inplace_rshift, PyNumber_InPlaceRshift);
_SharedObjectProxy_ONE_ARG(nb_inplace_and, PyNumber_InPlaceAnd);
_SharedObjectProxy_ONE_ARG(nb_inplace_xor, PyNumber_InPlaceXor);
_SharedObjectProxy_ONE_ARG(nb_inplace_or, PyNumber_InPlaceOr);
_SharedObjectProxy_ONE_ARG(nb_floor_divide, PyNumber_FloorDivide);
_SharedObjectProxy_ONE_ARG(nb_true_divide, PyNumber_TrueDivide);
_SharedObjectProxy_ONE_ARG(nb_inplace_floor_divide, PyNumber_InPlaceFloorDivide);
_SharedObjectProxy_ONE_ARG(nb_inplace_true_divide, PyNumber_InPlaceTrueDivide);
_SharedObjectProxy_NO_ARG(nb_index, PyNumber_Index);
_SharedObjectProxy_ONE_ARG(nb_matrix_multiply, PyNumber_MatrixMultiply);
_SharedObjectProxy_ONE_ARG(nb_inplace_matrix_multiply, PyNumber_InPlaceMatrixMultiply);

/* Async wrappers */
_SharedObjectProxy_NO_ARG(am_await, _PyCoro_GetAwaitableIter);
_SharedObjectProxy_NO_ARG(am_aiter, PyObject_GetAIter);
_SharedObjectProxy_NO_ARG(am_anext, _PyEval_GetANext);

/* Sequence wrappers */
_SharedObjectProxy_SSIZE_RETURN(sq_length, PySequence_Size);
_SharedObjectProxy_ONE_ARG(sq_concat, PySequence_Concat);
_SharedObjectProxy_SSIZE_ARG(sq_repeat, PySequence_Repeat)
_SharedObjectProxy_SSIZE_ARG(sq_item, PySequence_GetItem);
_SharedObjectProxy_ONE_ARG(sq_inplace_concat, PySequence_InPlaceConcat);
_SharedObjectProxy_SSIZE_ARG(sq_inplace_repeat, PySequence_InPlaceRepeat);

static int
sharedobjectproxy_sq_ass_item(PyObject *op, Py_ssize_t index, PyObject *item)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    _PyXI_proxy_share shared_arg;
    PyObject *shared_obj = _sharedobjectproxy_single_share_common(self, item,
                                                                  &state, &shared_arg);
    if (shared_obj == NULL) {
        Py_DECREF(shared_obj);
        (void)_sharedobjectproxy_exit(self, &state);
        _sharedobjectproxy_finish_share(&shared_arg);
        return -1;
    }
    int result = PySequence_SetItem(SharedObjectProxy_OBJECT(op), index, shared_obj);
    Py_DECREF(shared_obj);
    if (_sharedobjectproxy_exit(self, &state) < 0) {
        _sharedobjectproxy_finish_share(&shared_arg);
        return -1;
    }
    _sharedobjectproxy_finish_share(&shared_arg);
    return result;
}

static int
sharedobjectproxy_sq_contains(PyObject *op, PyObject *item)
{
    SharedObjectProxy *self = SharedObjectProxy_CAST(op);
    _PyXI_proxy_state state;
    _PyXI_proxy_share shared_arg;
    PyObject *shared_obj = _sharedobjectproxy_single_share_common(self, item,
                                                                  &state, &shared_arg);
    if (shared_obj == NULL) {
        Py_DECREF(shared_obj);
        (void)_sharedobjectproxy_exit(self, &state);
        _sharedobjectproxy_finish_share(&shared_arg);
        return -1;
    }
    int result = PySequence_Contains(SharedObjectProxy_OBJECT(op), shared_obj);
    Py_DECREF(shared_obj);
    if (_sharedobjectproxy_exit(self, &state) < 0) {
        _sharedobjectproxy_finish_share(&shared_arg);
        return -1;
    }
    _sharedobjectproxy_finish_share(&shared_arg);
    return result;
}

/* Mapping wrappers */

_SharedObjectProxy_SSIZE_RETURN(mp_length, PyMapping_Length);
_SharedObjectProxy_ONE_ARG(mp_subscript, PyObject_GetItem);
_SharedObjectProxy_TWO_ARG_INT(mp_ass_subscript, PyObject_SetItem);

#define _SharedObjectProxy_FIELD(name) {Py_ ##name, sharedobjectproxy_ ##name}

static PyType_Slot SharedObjectProxy_slots[] = {
    {Py_tp_new, sharedobjectproxy_new},
    {Py_tp_traverse, sharedobjectproxy_traverse},
    {Py_tp_clear, sharedobjectproxy_clear},
    {Py_tp_dealloc, sharedobjectproxy_dealloc},
    _SharedObjectProxy_FIELD(tp_getattro),
    _SharedObjectProxy_FIELD(tp_setattro),
    _SharedObjectProxy_FIELD(tp_call),
    _SharedObjectProxy_FIELD(tp_repr),
    _SharedObjectProxy_FIELD(tp_str),
    _SharedObjectProxy_FIELD(tp_iter),
    _SharedObjectProxy_FIELD(tp_iternext),
    _SharedObjectProxy_FIELD(tp_hash),
    _SharedObjectProxy_FIELD(mp_length),
    _SharedObjectProxy_FIELD(mp_subscript),
    _SharedObjectProxy_FIELD(mp_ass_subscript),
    _SharedObjectProxy_FIELD(sq_concat),
    _SharedObjectProxy_FIELD(sq_length),
    _SharedObjectProxy_FIELD(sq_repeat),
    _SharedObjectProxy_FIELD(sq_item),
    _SharedObjectProxy_FIELD(sq_inplace_concat),
    _SharedObjectProxy_FIELD(sq_inplace_repeat),
    _SharedObjectProxy_FIELD(sq_ass_item),
    _SharedObjectProxy_FIELD(sq_contains),
    _SharedObjectProxy_FIELD(am_await),
    _SharedObjectProxy_FIELD(am_aiter),
    _SharedObjectProxy_FIELD(am_anext),
    _SharedObjectProxy_FIELD(nb_add),
    _SharedObjectProxy_FIELD(nb_subtract),
    _SharedObjectProxy_FIELD(nb_multiply),
    _SharedObjectProxy_FIELD(nb_remainder),
    _SharedObjectProxy_FIELD(nb_power),
    _SharedObjectProxy_FIELD(nb_divmod),
    _SharedObjectProxy_FIELD(nb_negative),
    _SharedObjectProxy_FIELD(nb_positive),
    _SharedObjectProxy_FIELD(nb_absolute),
    _SharedObjectProxy_FIELD(nb_invert),
    _SharedObjectProxy_FIELD(nb_lshift),
    _SharedObjectProxy_FIELD(nb_rshift),
    _SharedObjectProxy_FIELD(nb_and),
    _SharedObjectProxy_FIELD(nb_xor),
    _SharedObjectProxy_FIELD(nb_or),
    _SharedObjectProxy_FIELD(nb_int),
    _SharedObjectProxy_FIELD(nb_float),
    _SharedObjectProxy_FIELD(nb_inplace_add),
    _SharedObjectProxy_FIELD(nb_inplace_subtract),
    _SharedObjectProxy_FIELD(nb_inplace_multiply),
    _SharedObjectProxy_FIELD(nb_inplace_remainder),
    _SharedObjectProxy_FIELD(nb_inplace_power),
    _SharedObjectProxy_FIELD(nb_inplace_lshift),
    _SharedObjectProxy_FIELD(nb_inplace_rshift),
    _SharedObjectProxy_FIELD(nb_inplace_and),
    _SharedObjectProxy_FIELD(nb_inplace_xor),
    _SharedObjectProxy_FIELD(nb_inplace_or),
    _SharedObjectProxy_FIELD(nb_floor_divide),
    _SharedObjectProxy_FIELD(nb_true_divide),
    _SharedObjectProxy_FIELD(nb_inplace_floor_divide),
    _SharedObjectProxy_FIELD(nb_inplace_true_divide),
    _SharedObjectProxy_FIELD(nb_index),
    _SharedObjectProxy_FIELD(nb_matrix_multiply),
    _SharedObjectProxy_FIELD(nb_inplace_matrix_multiply),
    {0, NULL},
};

static PyType_Spec SharedObjectProxy_spec = {
    .name = MODULE_NAME_STR ".SharedObjectProxy",
    .basicsize = sizeof(SharedObjectProxy),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE
              | Py_TPFLAGS_HAVE_GC),
    .slots = SharedObjectProxy_slots,
};


static PyObject *
sharedobjectproxy_xid(_PyXIData_t *data)
{
    SharedObjectProxy *proxy = SharedObjectProxy_CAST(data->obj);
    return _sharedobjectproxy_create(proxy->object, proxy->interp);
}

static void
sharedobjectproxy_shared_free(void *data)
{
    SharedObjectProxy *proxy = SharedObjectProxy_CAST(data);
    Py_DECREF(proxy);
}

static int
sharedobjectproxy_shared(PyThreadState *tstate, PyObject *obj, _PyXIData_t *data)
{
    _PyXIData_Init(data, tstate->interp, NULL, obj, sharedobjectproxy_xid);
    data->free = sharedobjectproxy_shared_free;
    return 0;
}

static int
register_sharedobjectproxy(PyObject *mod, PyTypeObject **p_state)
{
    assert(*p_state == NULL);
    PyTypeObject *cls = (PyTypeObject *)PyType_FromModuleAndSpec(
                mod, &SharedObjectProxy_spec, NULL);
    if (cls == NULL) {
        return -1;
    }
    if (PyModule_AddType(mod, cls) < 0) {
        Py_DECREF(cls);
        return -1;
    }
    *p_state = cls;

    if (ensure_xid_class(cls, GETDATA(sharedobjectproxy_shared)) < 0) {
        return -1;
    }

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

static PyTypeObject *
_get_current_sharedobjectproxy_type(void)
{
    module_state *state = _get_current_module_state();
    if (state == NULL) {
        return NULL;
    }

    return state->SharedObjectProxyType;
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
        // If an error occurred at this step, it means that interp
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


// Not converted to Argument Clinic because the function uses ``**kwargs``.
static PyObject *
interp_new_config(PyObject *self, PyObject *args, PyObject *kwds)
{
    const char *name = NULL;
    if (!PyArg_ParseTuple(args, "|s:" MODULE_NAME_STR ".new_config", &name))
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
"new_config($module, name='isolated', /, **overrides)\n\
--\n\
\n\
Return a representation of a new PyInterpreterConfig.\n\
\n\
The name determines the initial values of the config.  Supported named\n\
configs are: default, isolated, legacy, and empty.\n\
\n\
Any keyword arguments are set on the corresponding config fields,\n\
overriding the initial values.");


/*[clinic input]
_interpreters.create
    config as configobj: object(py_default="'isolated'") = NULL
    *
    reqrefs: bool = False

Create a new interpreter and return a unique generated ID.

The caller is responsible for destroying the interpreter before exiting,
typically by using _interpreters.destroy().  This can be managed
automatically by passing "reqrefs=True" and then using _incref() and
_decref() appropriately.

"config" must be a valid interpreter config or the name of a
predefined config ('isolated' or 'legacy').  The default
is 'isolated'.
[clinic start generated code]*/

static PyObject *
_interpreters_create_impl(PyObject *module, PyObject *configobj, int reqrefs)
/*[clinic end generated code: output=c1cc6835b1277c16 input=235ce396a23624d5]*/
{
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


/*[clinic input]
_interpreters.destroy
    id: object
    *
    restrict as restricted: bool = False

Destroy the identified interpreter.

Attempting to destroy the current interpreter raises InterpreterError.
So does an unrecognized ID.
[clinic start generated code]*/

static PyObject *
_interpreters_destroy_impl(PyObject *module, PyObject *id, int restricted)
/*[clinic end generated code: output=0bc20da8700ab4dd input=561bdd6537639d40]*/
{
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


/*[clinic input]
_interpreters.list_all
    *
    require_ready as reqready: bool = False

Return a list containing the ID of every existing interpreter.
[clinic start generated code]*/

static PyObject *
_interpreters_list_all_impl(PyObject *module, int reqready)
/*[clinic end generated code: output=3f21c1a7c78043c0 input=35bae91c381a2cf9]*/
{
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


/*[clinic input]
_interpreters.get_current

Return (ID, whence) of the current interpreter.
[clinic start generated code]*/

static PyObject *
_interpreters_get_current_impl(PyObject *module)
/*[clinic end generated code: output=03161c8fcc0136eb input=37fb2c067c14d543]*/
{
    PyInterpreterState *interp =_get_current_interp();
    if (interp == NULL) {
        return NULL;
    }
    assert(_PyInterpreterState_IsReady(interp));
    return get_summary(interp);
}


/*[clinic input]
_interpreters.get_main

Return (ID, whence) of the main interpreter.
[clinic start generated code]*/

static PyObject *
_interpreters_get_main_impl(PyObject *module)
/*[clinic end generated code: output=9647288aff735557 input=b4ace23ca562146f]*/
{
    PyInterpreterState *interp = _PyInterpreterState_Main();
    assert(_PyInterpreterState_IsReady(interp));
    return get_summary(interp);
}


/*[clinic input]
_interpreters.set___main___attrs
    id: object
    updates: object(subclass_of='&PyDict_Type')
    *
    restrict as restricted: bool = False

Bind the given attributes in the interpreter's __main__ module.
[clinic start generated code]*/

static PyObject *
_interpreters_set___main___attrs_impl(PyObject *module, PyObject *id,
                                      PyObject *updates, int restricted)
/*[clinic end generated code: output=f3803010cb452bf0 input=d16ab8d81371f86a]*/
{
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

/*[clinic input]
_interpreters.exec
    id: object
    code: object
    shared: object(subclass_of='&PyDict_Type', c_default='NULL') = {}
    *
    restrict as restricted: bool = False

Execute the provided code in the identified interpreter.

This is equivalent to running the builtin exec() under the target
interpreter, using the __dict__ of its __main__ module as both
globals and locals.

"code" may be a string containing the text of a Python script.

Functions (and code objects) are also supported, with some restrictions.
The code/function must not take any arguments or be a closure
(i.e. have cell vars).  Methods and other callables are not supported.

If a function is provided, its code object is used and all its state
is ignored, including its __globals__ dict.
[clinic start generated code]*/

static PyObject *
_interpreters_exec_impl(PyObject *module, PyObject *id, PyObject *code,
                        PyObject *shared, int restricted)
/*[clinic end generated code: output=492057c4f10dc304 input=5a22c1ed0c5dbcf3]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
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
}

/*[clinic input]
_interpreters.run_string
    id: object
    script: unicode
    shared: object(subclass_of='&PyDict_Type', c_default='NULL') = {}
    *
    restrict as restricted: bool = False

Execute the provided string in the identified interpreter.

(See _interpreters.exec().)
[clinic start generated code]*/

static PyObject *
_interpreters_run_string_impl(PyObject *module, PyObject *id,
                              PyObject *script, PyObject *shared,
                              int restricted)
/*[clinic end generated code: output=a30a64fb9ad396a2 input=51ce549b9a8dbe21]*/
{
#define FUNCNAME MODULE_NAME_STR ".run_string"
    PyThreadState *tstate = _PyThreadState_GET();
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

/*[clinic input]
_interpreters.run_func
    id: object
    func: object
    shared: object(subclass_of='&PyDict_Type', c_default='NULL') = {}
    *
    restrict as restricted: bool = False

Execute the body of the provided function in the identified interpreter.

Code objects are also supported.  In both cases, closures and args
are not supported.  Methods and other callables are not supported either.

(See _interpreters.exec().)
[clinic start generated code]*/

static PyObject *
_interpreters_run_func_impl(PyObject *module, PyObject *id, PyObject *func,
                            PyObject *shared, int restricted)
/*[clinic end generated code: output=131f7202ca4a0c5e input=2d62bb9b9eaf4948]*/
{
#define FUNCNAME MODULE_NAME_STR ".run_func"
    PyThreadState *tstate = _PyThreadState_GET();
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

/*[clinic input]
_interpreters.call
    id: object
    callable: object
    args: object(subclass_of='&PyTuple_Type', c_default='NULL') = ()
    kwargs: object(subclass_of='&PyDict_Type', c_default='NULL') = {}
    *
    preserve_exc: bool = False
    restrict as restricted: bool = False

Call the provided object in the identified interpreter.

Pass the given args and kwargs, if possible.
[clinic start generated code]*/

static PyObject *
_interpreters_call_impl(PyObject *module, PyObject *id, PyObject *callable,
                        PyObject *args, PyObject *kwargs, int preserve_exc,
                        int restricted)
/*[clinic end generated code: output=b7a4a27d72df3ebc input=b026d0b212a575e6]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "make a call in");
    if (interp == NULL) {
        return NULL;
    }

    struct interp_call call = {0};
    if (_interp_call_pack(tstate, &call, callable, args, kwargs) < 0) {
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
}


/*[clinic input]
@permit_long_summary
_interpreters.is_shareable
    obj: object

Return True if the object's data may be shared between interpreters and False otherwise.
[clinic start generated code]*/

static PyObject *
_interpreters_is_shareable_impl(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=227856926a22940b input=95f888d35a6d4bb3]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyObject_CheckXIData(tstate, obj) == 0) {
        Py_RETURN_TRUE;
    }
    PyErr_Clear();
    Py_RETURN_FALSE;
}


/*[clinic input]
_interpreters.is_running
    id: object
    *
    restrict as restricted: bool = False

Return whether or not the identified interpreter is running.
[clinic start generated code]*/

static PyObject *
_interpreters_is_running_impl(PyObject *module, PyObject *id, int restricted)
/*[clinic end generated code: output=32a6225d5ded9bdb input=3291578d04231125]*/
{
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


/*[clinic input]
_interpreters.get_config
    id: object
    *
    restrict as restricted: bool = False

Return a representation of the config used to initialize the interpreter.
[clinic start generated code]*/

static PyObject *
_interpreters_get_config_impl(PyObject *module, PyObject *id, int restricted)
/*[clinic end generated code: output=56773353b9b7224a input=59519a01c22d96d1]*/
{
    if (id == Py_None) {
        id = NULL;
    }

    int reqready = 0;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "get the config of");
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


/*[clinic input]
_interpreters.whence
    id: object

Return an identifier for where the interpreter was created.
[clinic start generated code]*/

static PyObject *
_interpreters_whence_impl(PyObject *module, PyObject *id)
/*[clinic end generated code: output=ef2c21ab106c2c20 input=eeede0a2fbfa2968]*/
{
    PyInterpreterState *interp = look_up_interp(id);
    if (interp == NULL) {
        return NULL;
    }

    long whence = get_whence(interp);
    return PyLong_FromLong(whence);
}


/*[clinic input]
_interpreters.incref
    id: object
    *
    implieslink: bool = False
    restrict as restricted: bool = False

[clinic start generated code]*/

static PyObject *
_interpreters_incref_impl(PyObject *module, PyObject *id, int implieslink,
                          int restricted)
/*[clinic end generated code: output=eccaa4e03fbe8ee2 input=a0a614748f2e348c]*/
{
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


/*[clinic input]
_interpreters.decref
    id: object
    *
    restrict as restricted: bool = False

[clinic start generated code]*/

static PyObject *
_interpreters_decref_impl(PyObject *module, PyObject *id, int restricted)
/*[clinic end generated code: output=5c54db4b22086171 input=c4aa34f09c44e62a]*/
{
    int reqready = 1;
    PyInterpreterState *interp = \
            resolve_interp(id, restricted, reqready, "decref");
    if (interp == NULL) {
        return NULL;
    }

    _PyInterpreterState_IDDecref(interp);

    Py_RETURN_NONE;
}


/*[clinic input]
@permit_long_docstring_body
_interpreters.capture_exception
    exc as exc_arg: object = None

Return a snapshot of an exception.

If "exc" is None then the current exception, if any, is used (but not cleared).
The returned snapshot is the same as what _interpreters.exec() returns.
[clinic start generated code]*/

static PyObject *
_interpreters_capture_exception_impl(PyObject *module, PyObject *exc_arg)
/*[clinic end generated code: output=ef3f5393ef9c88a6 input=6c4dcb78fb722217]*/
{
    PyObject *exc = exc_arg;
    if (exc == NULL || exc == Py_None) {
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

static PyObject *
call_share_method_steal(PyThreadState *tstate, PyObject *method)
{
    assert(tstate != NULL);
    assert(method != NULL);
    PyObject *result = PyObject_CallNoArgs(method);
    Py_DECREF(method);
    if (result == NULL) {
        return NULL;
    }

    if (_PyObject_CheckXIData(tstate, result) < 0) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        _PyXIData_FormatNotShareableError(tstate, "__share__() returned unshareable object: %R", result);
        PyObject *new_exc = _PyErr_GetRaisedException(tstate);
        PyException_SetCause(new_exc, exc);
        PyErr_SetRaisedException(new_exc);
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

/*[clinic input]
_interpreters.share
    op: object,
    /


Wrap an object in a shareable proxy that allows cross-interpreter access.
[clinic start generated code]*/

static PyObject *
_interpreters_share(PyObject *module, PyObject *op)
/*[clinic end generated code: output=e2ce861ae3b58508 input=5fb300b5598bb7d2]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyObject_CheckXIData(tstate, op) == 0) {
        return Py_NewRef(op);
    }
    PyErr_Clear();

    PyObject *share_method;
    if (PyObject_GetOptionalAttrString(op, "__share__", &share_method) < 0) {
        return NULL;
    }
    if (share_method != NULL) {
        return call_share_method_steal(tstate, share_method /* stolen */);
    }

    return _sharedobjectproxy_create(op, _PyInterpreterState_GET());
}

static PyMethodDef module_functions[] = {
    {"new_config",                _PyCFunction_CAST(interp_new_config),
     METH_VARARGS | METH_KEYWORDS, new_config_doc},

    _INTERPRETERS_CREATE_METHODDEF
    _INTERPRETERS_DESTROY_METHODDEF
    _INTERPRETERS_LIST_ALL_METHODDEF
    _INTERPRETERS_GET_CURRENT_METHODDEF
    _INTERPRETERS_GET_MAIN_METHODDEF

    _INTERPRETERS_IS_RUNNING_METHODDEF
    _INTERPRETERS_GET_CONFIG_METHODDEF
    _INTERPRETERS_WHENCE_METHODDEF
    _INTERPRETERS_EXEC_METHODDEF
    _INTERPRETERS_CALL_METHODDEF
    _INTERPRETERS_RUN_STRING_METHODDEF
    _INTERPRETERS_RUN_FUNC_METHODDEF

    _INTERPRETERS_SET___MAIN___ATTRS_METHODDEF

    _INTERPRETERS_INCREF_METHODDEF
    _INTERPRETERS_DECREF_METHODDEF

    _INTERPRETERS_IS_SHAREABLE_METHODDEF
    _INTERPRETERS_CAPTURE_EXCEPTION_METHODDEF

    _INTERPRETERS_SHARE_METHODDEF
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

    if (register_sharedobjectproxy(mod, &state->SharedObjectProxyType) < 0) {
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
