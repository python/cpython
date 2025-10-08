#include "Python.h"
#include "pycore_call.h"          // _PyObject_VectorcallTstate()
#include "pycore_context.h"
#include "pycore_freelist.h"      // _Py_FREELIST_FREE(), _Py_FREELIST_POP()
#include "pycore_gc.h"            // _PyObject_GC_MAY_BE_TRACKED()
#include "pycore_hamt.h"
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_object.h"
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()



#include "clinic/context.c.h"
/*[clinic input]
module _contextvars
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a0955718c8b8cea6]*/


#define ENSURE_Context(o, err_ret)                                  \
    if (!PyContext_CheckExact(o)) {                                 \
        PyErr_SetString(PyExc_TypeError,                            \
                        "an instance of Context was expected");     \
        return err_ret;                                             \
    }

#define ENSURE_ContextVar(o, err_ret)                               \
    if (!PyContextVar_CheckExact(o)) {                              \
        PyErr_SetString(PyExc_TypeError,                            \
                       "an instance of ContextVar was expected");   \
        return err_ret;                                             \
    }

#define ENSURE_ContextToken(o, err_ret)                             \
    if (!PyContextToken_CheckExact(o)) {                            \
        PyErr_SetString(PyExc_TypeError,                            \
                        "an instance of Token was expected");       \
        return err_ret;                                             \
    }


/////////////////////////// Context API


static PyContext *
context_new_empty(void);

static PyContext *
context_new_from_vars(PyHamtObject *vars);

static inline PyContext *
context_get(void);

static PyContextToken *
token_new(PyContext *ctx, PyContextVar *var, PyObject *val);

static PyContextVar *
contextvar_new(PyObject *name, PyObject *def);

static int
contextvar_set(PyContextVar *var, PyObject *val);

static int
contextvar_del(PyContextVar *var);


PyObject *
_PyContext_NewHamtForTests(void)
{
    return (PyObject *)_PyHamt_New();
}


PyObject *
PyContext_New(void)
{
    return (PyObject *)context_new_empty();
}


PyObject *
PyContext_Copy(PyObject * octx)
{
    ENSURE_Context(octx, NULL)
    PyContext *ctx = (PyContext *)octx;
    return (PyObject *)context_new_from_vars(ctx->ctx_vars);
}


PyObject *
PyContext_CopyCurrent(void)
{
    PyContext *ctx = context_get();
    if (ctx == NULL) {
        return NULL;
    }

    return (PyObject *)context_new_from_vars(ctx->ctx_vars);
}

static const char *
context_event_name(PyContextEvent event) {
    switch (event) {
        case Py_CONTEXT_SWITCHED:
            return "Py_CONTEXT_SWITCHED";
        default:
            return "?";
    }
    Py_UNREACHABLE();
}

static void
notify_context_watchers(PyThreadState *ts, PyContextEvent event, PyObject *ctx)
{
    if (ctx == NULL) {
        // This will happen after exiting the last context in the stack, which
        // can occur if context_get was never called before entering a context
        // (e.g., called `contextvars.Context().run()` on a fresh thread, as
        // PyContext_Enter doesn't call context_get).
        ctx = Py_None;
    }
    assert(Py_REFCNT(ctx) > 0);
    PyInterpreterState *interp = ts->interp;
    assert(interp->_initialized);
    uint8_t bits = interp->active_context_watchers;
    int i = 0;
    while (bits) {
        assert(i < CONTEXT_MAX_WATCHERS);
        if (bits & 1) {
            PyContext_WatchCallback cb = interp->context_watchers[i];
            assert(cb != NULL);
            if (cb(event, ctx) < 0) {
                PyErr_FormatUnraisable(
                    "Exception ignored in %s watcher callback for %R",
                    context_event_name(event), ctx);
            }
        }
        i++;
        bits >>= 1;
    }
}


int
PyContext_AddWatcher(PyContext_WatchCallback callback)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);

    for (int i = 0; i < CONTEXT_MAX_WATCHERS; i++) {
        if (!interp->context_watchers[i]) {
            interp->context_watchers[i] = callback;
            interp->active_context_watchers |= (1 << i);
            return i;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "no more context watcher IDs available");
    return -1;
}


int
PyContext_ClearWatcher(int watcher_id)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);
    if (watcher_id < 0 || watcher_id >= CONTEXT_MAX_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "Invalid context watcher ID %d", watcher_id);
        return -1;
    }
    if (!interp->context_watchers[watcher_id]) {
        PyErr_Format(PyExc_ValueError, "No context watcher set for ID %d", watcher_id);
        return -1;
    }
    interp->context_watchers[watcher_id] = NULL;
    interp->active_context_watchers &= ~(1 << watcher_id);
    return 0;
}


static inline void
context_switched(PyThreadState *ts)
{
    ts->context_ver++;
    // ts->context is used instead of context_get() because context_get() might
    // throw if ts->context is NULL.
    notify_context_watchers(ts, Py_CONTEXT_SWITCHED, ts->context);
}


static int
_PyContext_Enter(PyThreadState *ts, PyObject *octx)
{
    ENSURE_Context(octx, -1)
    PyContext *ctx = (PyContext *)octx;

    if (ctx->ctx_entered) {
        _PyErr_Format(ts, PyExc_RuntimeError,
                      "cannot enter context: %R is already entered", ctx);
        return -1;
    }

    ctx->ctx_prev = (PyContext *)ts->context;  /* borrow */
    ctx->ctx_entered = 1;

    ts->context = Py_NewRef(ctx);
    context_switched(ts);
    return 0;
}


int
PyContext_Enter(PyObject *octx)
{
    PyThreadState *ts = _PyThreadState_GET();
    assert(ts != NULL);
    return _PyContext_Enter(ts, octx);
}


static int
_PyContext_Exit(PyThreadState *ts, PyObject *octx)
{
    ENSURE_Context(octx, -1)
    PyContext *ctx = (PyContext *)octx;

    if (!ctx->ctx_entered) {
        PyErr_Format(PyExc_RuntimeError,
                     "cannot exit context: %R has not been entered", ctx);
        return -1;
    }

    if (ts->context != (PyObject *)ctx) {
        /* Can only happen if someone misuses the C API */
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot exit context: thread state references "
                        "a different context object");
        return -1;
    }

    Py_SETREF(ts->context, (PyObject *)ctx->ctx_prev);

    ctx->ctx_prev = NULL;
    ctx->ctx_entered = 0;
    context_switched(ts);
    return 0;
}

int
PyContext_Exit(PyObject *octx)
{
    PyThreadState *ts = _PyThreadState_GET();
    assert(ts != NULL);
    return _PyContext_Exit(ts, octx);
}


PyObject *
PyContextVar_New(const char *name, PyObject *def)
{
    PyObject *pyname = PyUnicode_FromString(name);
    if (pyname == NULL) {
        return NULL;
    }
    PyContextVar *var = contextvar_new(pyname, def);
    Py_DECREF(pyname);
    return (PyObject *)var;
}


int
PyContextVar_Get(PyObject *ovar, PyObject *def, PyObject **val)
{
    ENSURE_ContextVar(ovar, -1)
    PyContextVar *var = (PyContextVar *)ovar;

    PyThreadState *ts = _PyThreadState_GET();
    assert(ts != NULL);
    if (ts->context == NULL) {
        goto not_found;
    }

#ifndef Py_GIL_DISABLED
    if (var->var_cached != NULL &&
            var->var_cached_tsid == ts->id &&
            var->var_cached_tsver == ts->context_ver)
    {
        *val = var->var_cached;
        goto found;
    }
#endif

    assert(PyContext_CheckExact(ts->context));
    PyHamtObject *vars = ((PyContext *)ts->context)->ctx_vars;

    PyObject *found = NULL;
    int res = _PyHamt_Find(vars, (PyObject*)var, &found);
    if (res < 0) {
        goto error;
    }
    if (res == 1) {
        assert(found != NULL);
#ifndef Py_GIL_DISABLED
        var->var_cached = found;  /* borrow */
        var->var_cached_tsid = ts->id;
        var->var_cached_tsver = ts->context_ver;
#endif

        *val = found;
        goto found;
    }

not_found:
    if (def == NULL) {
        if (var->var_default != NULL) {
            *val = var->var_default;
            goto found;
        }

        *val = NULL;
        goto found;
    }
    else {
        *val = def;
        goto found;
   }

found:
    Py_XINCREF(*val);
    return 0;

error:
    *val = NULL;
    return -1;
}


PyObject *
PyContextVar_Set(PyObject *ovar, PyObject *val)
{
    ENSURE_ContextVar(ovar, NULL)
    PyContextVar *var = (PyContextVar *)ovar;

    if (!PyContextVar_CheckExact(var)) {
        PyErr_SetString(
            PyExc_TypeError, "an instance of ContextVar was expected");
        return NULL;
    }

    PyContext *ctx = context_get();
    if (ctx == NULL) {
        return NULL;
    }

    PyObject *old_val = NULL;
    int found = _PyHamt_Find(ctx->ctx_vars, (PyObject *)var, &old_val);
    if (found < 0) {
        return NULL;
    }

    Py_XINCREF(old_val);
    PyContextToken *tok = token_new(ctx, var, old_val);
    Py_XDECREF(old_val);

    if (contextvar_set(var, val)) {
        Py_DECREF(tok);
        return NULL;
    }

    return (PyObject *)tok;
}


int
PyContextVar_Reset(PyObject *ovar, PyObject *otok)
{
    ENSURE_ContextVar(ovar, -1)
    ENSURE_ContextToken(otok, -1)
    PyContextVar *var = (PyContextVar *)ovar;
    PyContextToken *tok = (PyContextToken *)otok;

    if (tok->tok_used) {
        PyErr_Format(PyExc_RuntimeError,
                     "%R has already been used once", tok);
        return -1;
    }

    if (var != tok->tok_var) {
        PyErr_Format(PyExc_ValueError,
                     "%R was created by a different ContextVar", tok);
        return -1;
    }

    PyContext *ctx = context_get();
    if (ctx != tok->tok_ctx) {
        PyErr_Format(PyExc_ValueError,
                     "%R was created in a different Context", tok);
        return -1;
    }

    tok->tok_used = 1;

    if (tok->tok_oldval == NULL) {
        return contextvar_del(var);
    }
    else {
        return contextvar_set(var, tok->tok_oldval);
    }
}


/////////////////////////// PyContext

/*[clinic input]
class _contextvars.Context "PyContext *" "&PyContext_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=bdf87f8e0cb580e8]*/


#define _PyContext_CAST(op)     ((PyContext *)(op))


static inline PyContext *
_context_alloc(void)
{
    PyContext *ctx = _Py_FREELIST_POP(PyContext, contexts);
    if (ctx == NULL) {
        ctx = PyObject_GC_New(PyContext, &PyContext_Type);
        if (ctx == NULL) {
            return NULL;
        }
    }

    ctx->ctx_vars = NULL;
    ctx->ctx_prev = NULL;
    ctx->ctx_entered = 0;
    ctx->ctx_weakreflist = NULL;

    return ctx;
}


static PyContext *
context_new_empty(void)
{
    PyContext *ctx = _context_alloc();
    if (ctx == NULL) {
        return NULL;
    }

    ctx->ctx_vars = _PyHamt_New();
    if (ctx->ctx_vars == NULL) {
        Py_DECREF(ctx);
        return NULL;
    }

    _PyObject_GC_TRACK(ctx);
    return ctx;
}


static PyContext *
context_new_from_vars(PyHamtObject *vars)
{
    PyContext *ctx = _context_alloc();
    if (ctx == NULL) {
        return NULL;
    }

    ctx->ctx_vars = (PyHamtObject*)Py_NewRef(vars);

    _PyObject_GC_TRACK(ctx);
    return ctx;
}


static inline PyContext *
context_get(void)
{
    PyThreadState *ts = _PyThreadState_GET();
    assert(ts != NULL);
    PyContext *current_ctx = (PyContext *)ts->context;
    if (current_ctx == NULL) {
        current_ctx = context_new_empty();
        if (current_ctx == NULL) {
            return NULL;
        }
        ts->context = (PyObject *)current_ctx;
    }
    return current_ctx;
}

static int
context_check_key_type(PyObject *key)
{
    if (!PyContextVar_CheckExact(key)) {
        // abort();
        PyErr_Format(PyExc_TypeError,
                     "a ContextVar key was expected, got %R", key);
        return -1;
    }
    return 0;
}

static PyObject *
context_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (PyTuple_Size(args) || (kwds != NULL && PyDict_Size(kwds))) {
        PyErr_SetString(
            PyExc_TypeError, "Context() does not accept any arguments");
        return NULL;
    }
    return PyContext_New();
}

static int
context_tp_clear(PyObject *op)
{
    PyContext *self = _PyContext_CAST(op);
    Py_CLEAR(self->ctx_prev);
    Py_CLEAR(self->ctx_vars);
    return 0;
}

static int
context_tp_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyContext *self = _PyContext_CAST(op);
    Py_VISIT(self->ctx_prev);
    Py_VISIT(self->ctx_vars);
    return 0;
}

static void
context_tp_dealloc(PyObject *self)
{
    _PyObject_GC_UNTRACK(self);
    PyContext *ctx = _PyContext_CAST(self);
    if (ctx->ctx_weakreflist != NULL) {
        PyObject_ClearWeakRefs(self);
    }
    (void)context_tp_clear(self);

    _Py_FREELIST_FREE(contexts, self, Py_TYPE(self)->tp_free);
}

static PyObject *
context_tp_iter(PyObject *op)
{
    PyContext *self = _PyContext_CAST(op);
    return _PyHamt_NewIterKeys(self->ctx_vars);
}

static PyObject *
context_tp_richcompare(PyObject *v, PyObject *w, int op)
{
    if (!PyContext_CheckExact(v) || !PyContext_CheckExact(w) ||
            (op != Py_EQ && op != Py_NE))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }

    int res = _PyHamt_Eq(
        ((PyContext *)v)->ctx_vars, ((PyContext *)w)->ctx_vars);
    if (res < 0) {
        return NULL;
    }

    if (op == Py_NE) {
        res = !res;
    }

    if (res) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

static Py_ssize_t
context_tp_len(PyObject *op)
{
    PyContext *self = _PyContext_CAST(op);
    return _PyHamt_Len(self->ctx_vars);
}

static PyObject *
context_tp_subscript(PyObject *op, PyObject *key)
{
    if (context_check_key_type(key)) {
        return NULL;
    }
    PyObject *val = NULL;
    PyContext *self = _PyContext_CAST(op);
    int found = _PyHamt_Find(self->ctx_vars, key, &val);
    if (found < 0) {
        return NULL;
    }
    if (found == 0) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    return Py_NewRef(val);
}

static int
context_tp_contains(PyObject *op, PyObject *key)
{
    if (context_check_key_type(key)) {
        return -1;
    }
    PyObject *val = NULL;
    PyContext *self = _PyContext_CAST(op);
    return _PyHamt_Find(self->ctx_vars, key, &val);
}


/*[clinic input]
_contextvars.Context.get
    key: object
    default: object = None
    /

Return the value for `key` if `key` has the value in the context object.

If `key` does not exist, return `default`. If `default` is not given,
return None.
[clinic start generated code]*/

static PyObject *
_contextvars_Context_get_impl(PyContext *self, PyObject *key,
                              PyObject *default_value)
/*[clinic end generated code: output=0c54aa7664268189 input=c8eeb81505023995]*/
{
    if (context_check_key_type(key)) {
        return NULL;
    }

    PyObject *val = NULL;
    int found = _PyHamt_Find(self->ctx_vars, key, &val);
    if (found < 0) {
        return NULL;
    }
    if (found == 0) {
        return Py_NewRef(default_value);
    }
    return Py_NewRef(val);
}


/*[clinic input]
_contextvars.Context.items

Return all variables and their values in the context object.

The result is returned as a list of 2-tuples (variable, value).
[clinic start generated code]*/

static PyObject *
_contextvars_Context_items_impl(PyContext *self)
/*[clinic end generated code: output=fa1655c8a08502af input=00db64ae379f9f42]*/
{
    return _PyHamt_NewIterItems(self->ctx_vars);
}


/*[clinic input]
_contextvars.Context.keys

Return a list of all variables in the context object.
[clinic start generated code]*/

static PyObject *
_contextvars_Context_keys_impl(PyContext *self)
/*[clinic end generated code: output=177227c6b63ec0e2 input=114b53aebca3449c]*/
{
    return _PyHamt_NewIterKeys(self->ctx_vars);
}


/*[clinic input]
_contextvars.Context.values

Return a list of all variables' values in the context object.
[clinic start generated code]*/

static PyObject *
_contextvars_Context_values_impl(PyContext *self)
/*[clinic end generated code: output=d286dabfc8db6dde input=ce8075d04a6ea526]*/
{
    return _PyHamt_NewIterValues(self->ctx_vars);
}


/*[clinic input]
_contextvars.Context.copy

Return a shallow copy of the context object.
[clinic start generated code]*/

static PyObject *
_contextvars_Context_copy_impl(PyContext *self)
/*[clinic end generated code: output=30ba8896c4707a15 input=ebafdbdd9c72d592]*/
{
    return (PyObject *)context_new_from_vars(self->ctx_vars);
}


static PyObject *
context_run(PyObject *self, PyObject *const *args,
            Py_ssize_t nargs, PyObject *kwnames)
{
    PyThreadState *ts = _PyThreadState_GET();

    if (nargs < 1) {
        _PyErr_SetString(ts, PyExc_TypeError,
                         "run() missing 1 required positional argument");
        return NULL;
    }

    if (_PyContext_Enter(ts, self)) {
        return NULL;
    }

    PyObject *call_result = _PyObject_VectorcallTstate(
        ts, args[0], args + 1, nargs - 1, kwnames);

    if (_PyContext_Exit(ts, self)) {
        Py_XDECREF(call_result);
        return NULL;
    }

    return call_result;
}


static PyMethodDef PyContext_methods[] = {
    _CONTEXTVARS_CONTEXT_GET_METHODDEF
    _CONTEXTVARS_CONTEXT_ITEMS_METHODDEF
    _CONTEXTVARS_CONTEXT_KEYS_METHODDEF
    _CONTEXTVARS_CONTEXT_VALUES_METHODDEF
    _CONTEXTVARS_CONTEXT_COPY_METHODDEF
    {"run", _PyCFunction_CAST(context_run), METH_FASTCALL | METH_KEYWORDS, NULL},
    {NULL, NULL}
};

static PySequenceMethods PyContext_as_sequence = {
    .sq_contains = context_tp_contains
};

static PyMappingMethods PyContext_as_mapping = {
    .mp_length = context_tp_len,
    .mp_subscript = context_tp_subscript
};

PyTypeObject PyContext_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "_contextvars.Context",
    sizeof(PyContext),
    .tp_methods = PyContext_methods,
    .tp_as_mapping = &PyContext_as_mapping,
    .tp_as_sequence = &PyContext_as_sequence,
    .tp_iter = context_tp_iter,
    .tp_dealloc = context_tp_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_richcompare = context_tp_richcompare,
    .tp_traverse = context_tp_traverse,
    .tp_clear = context_tp_clear,
    .tp_new = context_tp_new,
    .tp_weaklistoffset = offsetof(PyContext, ctx_weakreflist),
    .tp_hash = PyObject_HashNotImplemented,
};


/////////////////////////// ContextVar


static int
contextvar_set(PyContextVar *var, PyObject *val)
{
#ifndef Py_GIL_DISABLED
    var->var_cached = NULL;
    PyThreadState *ts = _PyThreadState_GET();
#endif

    PyContext *ctx = context_get();
    if (ctx == NULL) {
        return -1;
    }

    PyHamtObject *new_vars = _PyHamt_Assoc(
        ctx->ctx_vars, (PyObject *)var, val);
    if (new_vars == NULL) {
        return -1;
    }

    Py_SETREF(ctx->ctx_vars, new_vars);

#ifndef Py_GIL_DISABLED
    var->var_cached = val;  /* borrow */
    var->var_cached_tsid = ts->id;
    var->var_cached_tsver = ts->context_ver;
#endif
    return 0;
}

static int
contextvar_del(PyContextVar *var)
{
#ifndef Py_GIL_DISABLED
    var->var_cached = NULL;
#endif

    PyContext *ctx = context_get();
    if (ctx == NULL) {
        return -1;
    }

    PyHamtObject *vars = ctx->ctx_vars;
    PyHamtObject *new_vars = _PyHamt_Without(vars, (PyObject *)var);
    if (new_vars == NULL) {
        return -1;
    }

    if (vars == new_vars) {
        Py_DECREF(new_vars);
        PyErr_SetObject(PyExc_LookupError, (PyObject *)var);
        return -1;
    }

    Py_SETREF(ctx->ctx_vars, new_vars);
    return 0;
}

static Py_hash_t
contextvar_generate_hash(void *addr, PyObject *name)
{
    /* Take hash of `name` and XOR it with the object's addr.

       The structure of the tree is encoded in objects' hashes, which
       means that sufficiently similar hashes would result in tall trees
       with many Collision nodes.  Which would, in turn, result in slower
       get and set operations.

       The XORing helps to ensure that:

       (1) sequentially allocated ContextVar objects have
           different hashes;

       (2) context variables with equal names have
           different hashes.
    */

    Py_hash_t name_hash = PyObject_Hash(name);
    if (name_hash == -1) {
        return -1;
    }

    Py_hash_t res = Py_HashPointer(addr) ^ name_hash;
    return res == -1 ? -2 : res;
}

static PyContextVar *
contextvar_new(PyObject *name, PyObject *def)
{
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "context variable name must be a str");
        return NULL;
    }

    PyContextVar *var = PyObject_GC_New(PyContextVar, &PyContextVar_Type);
    if (var == NULL) {
        return NULL;
    }

    var->var_name = Py_NewRef(name);
    var->var_default = Py_XNewRef(def);

#ifndef Py_GIL_DISABLED
    var->var_cached = NULL;
    var->var_cached_tsid = 0;
    var->var_cached_tsver = 0;
#endif

    var->var_hash = contextvar_generate_hash(var, name);
    if (var->var_hash == -1) {
        Py_DECREF(var);
        return NULL;
    }

    if (_PyObject_GC_MAY_BE_TRACKED(name) ||
            (def != NULL && _PyObject_GC_MAY_BE_TRACKED(def)))
    {
        PyObject_GC_Track(var);
    }
    return var;
}


/*[clinic input]
class _contextvars.ContextVar "PyContextVar *" "&PyContextVar_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=445da935fa8883c3]*/


#define _PyContextVar_CAST(op)  ((PyContextVar *)(op))


static PyObject *
contextvar_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"", "default", NULL};
    PyObject *name;
    PyObject *def = NULL;

    if (!PyArg_ParseTupleAndKeywords(
            args, kwds, "O|$O:ContextVar", kwlist, &name, &def))
    {
        return NULL;
    }

    return (PyObject *)contextvar_new(name, def);
}

static int
contextvar_tp_clear(PyObject *op)
{
    PyContextVar *self = _PyContextVar_CAST(op);
    Py_CLEAR(self->var_name);
    Py_CLEAR(self->var_default);
#ifndef Py_GIL_DISABLED
    self->var_cached = NULL;
    self->var_cached_tsid = 0;
    self->var_cached_tsver = 0;
#endif
    return 0;
}

static int
contextvar_tp_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyContextVar *self = _PyContextVar_CAST(op);
    Py_VISIT(self->var_name);
    Py_VISIT(self->var_default);
    return 0;
}

static void
contextvar_tp_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    (void)contextvar_tp_clear(self);
    Py_TYPE(self)->tp_free(self);
}

static Py_hash_t
contextvar_tp_hash(PyObject *op)
{
    PyContextVar *self = _PyContextVar_CAST(op);
    return self->var_hash;
}

static PyObject *
contextvar_tp_repr(PyObject *op)
{
    PyContextVar *self = _PyContextVar_CAST(op);
    // Estimation based on the shortest name and default value,
    // but maximize the pointer size.
    // "<ContextVar name='a' at 0x1234567812345678>"
    // "<ContextVar name='a' default=1 at 0x1234567812345678>"
    Py_ssize_t estimate = self->var_default ? 53 : 43;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(estimate);
    if (writer == NULL) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteASCII(writer, "<ContextVar name=", 17) < 0) {
        goto error;
    }
    if (PyUnicodeWriter_WriteRepr(writer, self->var_name) < 0) {
        goto error;
    }

    if (self->var_default != NULL) {
        if (PyUnicodeWriter_WriteASCII(writer, " default=", 9) < 0) {
            goto error;
        }
        if (PyUnicodeWriter_WriteRepr(writer, self->var_default) < 0) {
            goto error;
        }
    }

    if (PyUnicodeWriter_Format(writer, " at %p>", self) < 0) {
        goto error;
    }
    return PyUnicodeWriter_Finish(writer);

error:
    PyUnicodeWriter_Discard(writer);
    return NULL;
}


/*[clinic input]
@permit_long_docstring_body
_contextvars.ContextVar.get
    default: object = NULL
    /

Return a value for the context variable for the current context.

If there is no value for the variable in the current context, the method will:
 * return the value of the default argument of the method, if provided; or
 * return the default value for the context variable, if it was created
   with one; or
 * raise a LookupError.
[clinic start generated code]*/

static PyObject *
_contextvars_ContextVar_get_impl(PyContextVar *self, PyObject *default_value)
/*[clinic end generated code: output=0746bd0aa2ced7bf input=da66664d5d0af4ad]*/
{
    if (!PyContextVar_CheckExact(self)) {
        PyErr_SetString(
            PyExc_TypeError, "an instance of ContextVar was expected");
        return NULL;
    }

    PyObject *val;
    if (PyContextVar_Get((PyObject *)self, default_value, &val) < 0) {
        return NULL;
    }

    if (val == NULL) {
        PyErr_SetObject(PyExc_LookupError, (PyObject *)self);
        return NULL;
    }

    return val;
}

/*[clinic input]
@permit_long_docstring_body
_contextvars.ContextVar.set
    value: object
    /

Call to set a new value for the context variable in the current context.

The required value argument is the new value for the context variable.

Returns a Token object that can be used to restore the variable to its previous
value via the `ContextVar.reset()` method.
[clinic start generated code]*/

static PyObject *
_contextvars_ContextVar_set_impl(PyContextVar *self, PyObject *value)
/*[clinic end generated code: output=1b562d35cc79c806 input=73ebbbfc7c98f6cd]*/
{
    return PyContextVar_Set((PyObject *)self, value);
}

/*[clinic input]
@permit_long_docstring_body
_contextvars.ContextVar.reset
    token: object
    /

Reset the context variable.

The variable is reset to the value it had before the `ContextVar.set()` that
created the token was used.
[clinic start generated code]*/

static PyObject *
_contextvars_ContextVar_reset_impl(PyContextVar *self, PyObject *token)
/*[clinic end generated code: output=3205d2bdff568521 input=b8bc514a9245242a]*/
{
    if (!PyContextToken_CheckExact(token)) {
        PyErr_Format(PyExc_TypeError,
                     "expected an instance of Token, got %R", token);
        return NULL;
    }

    if (PyContextVar_Reset((PyObject *)self, token)) {
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyMemberDef PyContextVar_members[] = {
    {"name", _Py_T_OBJECT, offsetof(PyContextVar, var_name), Py_READONLY},
    {NULL}
};

static PyMethodDef PyContextVar_methods[] = {
    _CONTEXTVARS_CONTEXTVAR_GET_METHODDEF
    _CONTEXTVARS_CONTEXTVAR_SET_METHODDEF
    _CONTEXTVARS_CONTEXTVAR_RESET_METHODDEF
    {"__class_getitem__", Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    {NULL, NULL}
};

PyTypeObject PyContextVar_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "_contextvars.ContextVar",
    sizeof(PyContextVar),
    .tp_methods = PyContextVar_methods,
    .tp_members = PyContextVar_members,
    .tp_dealloc = contextvar_tp_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = contextvar_tp_traverse,
    .tp_clear = contextvar_tp_clear,
    .tp_new = contextvar_tp_new,
    .tp_free = PyObject_GC_Del,
    .tp_hash = contextvar_tp_hash,
    .tp_repr = contextvar_tp_repr,
};


/////////////////////////// Token

static PyObject * get_token_missing(void);


/*[clinic input]
class _contextvars.Token "PyContextToken *" "&PyContextToken_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=338a5e2db13d3f5b]*/


#define _PyContextToken_CAST(op)    ((PyContextToken *)(op))


static PyObject *
token_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_RuntimeError,
                    "Tokens can only be created by ContextVars");
    return NULL;
}

static int
token_tp_clear(PyObject *op)
{
    PyContextToken *self = _PyContextToken_CAST(op);
    Py_CLEAR(self->tok_ctx);
    Py_CLEAR(self->tok_var);
    Py_CLEAR(self->tok_oldval);
    return 0;
}

static int
token_tp_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyContextToken *self = _PyContextToken_CAST(op);
    Py_VISIT(self->tok_ctx);
    Py_VISIT(self->tok_var);
    Py_VISIT(self->tok_oldval);
    return 0;
}

static void
token_tp_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    (void)token_tp_clear(self);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
token_tp_repr(PyObject *op)
{
    PyContextToken *self = _PyContextToken_CAST(op);
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }
    if (PyUnicodeWriter_WriteASCII(writer, "<Token", 6) < 0) {
        goto error;
    }
    if (self->tok_used) {
        if (PyUnicodeWriter_WriteASCII(writer, " used", 5) < 0) {
            goto error;
        }
    }
    if (PyUnicodeWriter_WriteASCII(writer, " var=", 5) < 0) {
        goto error;
    }
    if (PyUnicodeWriter_WriteRepr(writer, (PyObject *)self->tok_var) < 0) {
        goto error;
    }
    if (PyUnicodeWriter_Format(writer, " at %p>", self) < 0) {
        goto error;
    }
    return PyUnicodeWriter_Finish(writer);

error:
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

static PyObject *
token_get_var(PyObject *op, void *Py_UNUSED(ignored))
{
    PyContextToken *self = _PyContextToken_CAST(op);
    return Py_NewRef(self->tok_var);;
}

static PyObject *
token_get_old_value(PyObject *op, void *Py_UNUSED(ignored))
{
    PyContextToken *self = _PyContextToken_CAST(op);
    if (self->tok_oldval == NULL) {
        return get_token_missing();
    }

    return Py_NewRef(self->tok_oldval);
}

static PyGetSetDef PyContextTokenType_getsetlist[] = {
    {"var", token_get_var, NULL, NULL},
    {"old_value", token_get_old_value, NULL, NULL},
    {NULL}
};

/*[clinic input]
_contextvars.Token.__enter__ as token_enter

Enter into Token context manager.
[clinic start generated code]*/

static PyObject *
token_enter_impl(PyContextToken *self)
/*[clinic end generated code: output=9af4d2054e93fb75 input=41a3d6c4195fd47a]*/
{
    return Py_NewRef(self);
}

/*[clinic input]
_contextvars.Token.__exit__ as token_exit

    type: object
    val: object
    tb: object
    /

Exit from Token context manager, restore the linked ContextVar.
[clinic start generated code]*/

static PyObject *
token_exit_impl(PyContextToken *self, PyObject *type, PyObject *val,
                PyObject *tb)
/*[clinic end generated code: output=3e6a1c95d3da703a input=7f117445f0ccd92e]*/
{
    int ret = PyContextVar_Reset((PyObject *)self->tok_var, (PyObject *)self);
    if (ret < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef PyContextTokenType_methods[] = {
    {"__class_getitem__",    Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    TOKEN_ENTER_METHODDEF
    TOKEN_EXIT_METHODDEF
    {NULL}
};

PyTypeObject PyContextToken_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "_contextvars.Token",
    sizeof(PyContextToken),
    .tp_methods = PyContextTokenType_methods,
    .tp_getset = PyContextTokenType_getsetlist,
    .tp_dealloc = token_tp_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = token_tp_traverse,
    .tp_clear = token_tp_clear,
    .tp_new = token_tp_new,
    .tp_free = PyObject_GC_Del,
    .tp_hash = PyObject_HashNotImplemented,
    .tp_repr = token_tp_repr,
};

static PyContextToken *
token_new(PyContext *ctx, PyContextVar *var, PyObject *val)
{
    PyContextToken *tok = PyObject_GC_New(PyContextToken, &PyContextToken_Type);
    if (tok == NULL) {
        return NULL;
    }

    tok->tok_ctx = (PyContext*)Py_NewRef(ctx);

    tok->tok_var = (PyContextVar*)Py_NewRef(var);

    tok->tok_oldval = Py_XNewRef(val);

    tok->tok_used = 0;

    PyObject_GC_Track(tok);
    return tok;
}


/////////////////////////// Token.MISSING


static PyObject *
context_token_missing_tp_repr(PyObject *self)
{
    return PyUnicode_FromString("<Token.MISSING>");
}

static void
context_token_missing_tp_dealloc(PyObject *Py_UNUSED(self))
{
#ifdef Py_DEBUG
    /* The singleton is statically allocated. */
    _Py_FatalRefcountError("deallocating the token missing singleton");
#else
    return;
#endif
}


PyTypeObject _PyContextTokenMissing_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "Token.MISSING",
    sizeof(_PyContextTokenMissing),
    .tp_dealloc = context_token_missing_tp_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_repr = context_token_missing_tp_repr,
};


static PyObject *
get_token_missing(void)
{
    return (PyObject *)&_Py_SINGLETON(context_token_missing);
}


///////////////////////////


PyStatus
_PyContext_Init(PyInterpreterState *interp)
{
    PyObject *missing = get_token_missing();
    assert(PyUnstable_IsImmortal(missing));
    if (PyDict_SetItemString(
        _PyType_GetDict(&PyContextToken_Type), "MISSING", missing))
    {
        Py_DECREF(missing);
        return _PyStatus_ERR("can't init context types");
    }
    Py_DECREF(missing);

    return _PyStatus_OK();
}
