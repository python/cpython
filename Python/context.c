#include "Python.h"

#include "structmember.h"
#include "internal/pystate.h"
#include "internal/context.h"
#include "internal/hamt.h"


#define FREELIST_CONTEXT_MAXLEN 255
static PyContext *ctx_freelist = NULL;
static Py_ssize_t ctx_freelist_len = 0;


#include "clinic/context.c.h"
/*[clinic input]
module _contextvars
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a0955718c8b8cea6]*/


/////////////////////////// Context API


static PyContext *
context_new(PyHamtObject *vars);

static PyContextToken *
token_new(PyContextVar *var, PyObject *val);

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


PyContext *
PyContext_New(void)
{
    return context_new(NULL);
}


PyContext *
PyContext_Copy(void)
{
    PyThreadState *ts = PyThreadState_Get();
    PyHamtObject *vars = (PyHamtObject *)ts->contextvars;
    if (vars == NULL) {
        vars = _PyHamt_New();
        if (vars == NULL) {
            return NULL;
        }
        ts->contextvars = (PyObject *)vars;
    }
    return context_new(vars);
}


int
PyContext_Enter(PyContext *ctx)
{
    if (ctx->ctx_prev_set) {
        PyErr_Format(PyExc_RuntimeError,
                     "cannot enter: Context %R was already "
                     "entered before, but not exited", ctx);
        return -1;
    }

    PyThreadState *ts = PyThreadState_Get();
    ctx->ctx_prev = (PyHamtObject *)ts->contextvars;  /* borrow */
    ctx->ctx_prev_set = 1;

    Py_INCREF(ctx->ctx_vars);
    ts->contextvars = (PyObject *)ctx->ctx_vars;
    ts->contextvars_stack_ver++;

    return 0;
}


int
PyContext_Exit(PyContext *ctx)
{
    if (!ctx->ctx_prev_set) {
        PyErr_Format(PyExc_RuntimeError,
                     "cannot exit: Context %R was not entered before", ctx);
        return -1;
    }

    PyThreadState *ts = PyThreadState_Get();

    Py_DECREF(ctx->ctx_vars);
    ctx->ctx_vars = (PyHamtObject *)ts->contextvars;

    ts->contextvars = (PyObject *)ctx->ctx_prev;  /* borrow */
    ts->contextvars_stack_ver++;

    ctx->ctx_prev = NULL;
    ctx->ctx_prev_set = 0;

    return 0;
}


PyContextVar *
PyContextVar_New(const char *name, PyObject *def)
{
    PyObject *pyname = PyUnicode_FromString(name);
    if (pyname == NULL) {
        return NULL;
    }
    return contextvar_new(pyname, def);
}


int
PyContextVar_Get(PyContextVar *var, PyObject *def, PyObject **val)
{
    assert(PyContextVar_CheckExact(var));

    PyThreadState *ts = PyThreadState_Get();
    if (ts->contextvars == NULL) {
        goto not_found;
    }

    if (var->var_cached != NULL &&
            var->var_cached_tsid == ts->id &&
            var->var_cached_tsver == ts->contextvars_stack_ver)
    {
        *val = var->var_cached;
        goto found;
    }

    assert(PyHamt_Check(ts->contextvars));
    PyHamtObject *vars = (PyHamtObject *)ts->contextvars;

    PyObject *found = NULL;
    int res = _PyHamt_Find(vars, (PyObject*)var, &found);
    if (res < 0) {
        goto error;
    }
    if (res == 1) {
        assert(found != NULL);
        var->var_cached = found;  /* borrow */
        var->var_cached_tsid = ts->id;
        var->var_cached_tsver = ts->contextvars_stack_ver;

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


PyContextToken *
PyContextVar_Set(PyContextVar *var, PyObject *val)
{
    if (!PyContextVar_CheckExact(var)) {
        PyErr_SetString(
            PyExc_TypeError, "an instance of ContextVar was expected");
        return NULL;
    }

    PyThreadState *ts = PyThreadState_Get();

    if (ts->contextvars == NULL) {
        ts->contextvars = (PyObject *)_PyHamt_New();
        if (ts->contextvars == NULL) {
            return NULL;
        }
    }

    assert(PyHamt_Check(ts->contextvars));
    PyHamtObject *vars = (PyHamtObject *)ts->contextvars;

    PyObject *old_val = NULL;
    int found = _PyHamt_Find(vars, (PyObject *)var, &old_val);
    if (found < 0) {
        return NULL;
    }

    Py_XINCREF(old_val);
    PyContextToken *tok = token_new(var, old_val);
    Py_XDECREF(old_val);

    if (contextvar_set(var, val)) {
        Py_DECREF(tok);
        return NULL;
    }

    return tok;
}


int
PyContextVar_Reset(PyContextVar *var, PyContextToken *tok)
{
    if (var != tok->tok_var) {
        PyErr_SetString(PyExc_ValueError,
                        "Token was created by a different ContextVar");
        return -1;
    }

    if (tok->tok_used) {
        return 0;
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


static PyContext *
context_new(PyHamtObject *vars)
{
    PyContext *ctx;
    if (ctx_freelist_len) {
        ctx_freelist_len--;
        ctx = ctx_freelist;
        ctx_freelist = (PyContext *)ctx->ctx_weakreflist;
        ctx->ctx_weakreflist = NULL;
        _Py_NewReference((PyObject *)ctx);
    }
    else {
        ctx = PyObject_GC_New(PyContext, &PyContext_Type);
        if (ctx == NULL) {
            return NULL;
        }
    }

    if (vars == NULL) {
        ctx->ctx_vars = _PyHamt_New();
        if (ctx->ctx_vars == NULL) {
            Py_DECREF(ctx);
            return NULL;
        }
    }
    else {
        assert(PyHamt_Check(vars));
        Py_INCREF(vars);
        ctx->ctx_vars = vars;
    }

    ctx->ctx_prev = NULL;
    ctx->ctx_prev_set = 0;

    ctx->ctx_weakreflist = NULL;
    PyObject_GC_Track(ctx);
    return ctx;
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
    return (PyObject *)PyContext_New();
}

static int
context_tp_clear(PyContext *self)
{
    Py_CLEAR(self->ctx_prev);
    Py_CLEAR(self->ctx_vars);
    return 0;
}

static int
context_tp_traverse(PyContext *self, visitproc visit, void *arg)
{
    Py_VISIT(self->ctx_prev);
    Py_VISIT(self->ctx_vars);
    return 0;
}

static void
context_tp_dealloc(PyContext *self)
{
    PyObject_GC_UnTrack(self);
    if (self->ctx_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }
    (void)context_tp_clear(self);

    if (ctx_freelist_len < FREELIST_CONTEXT_MAXLEN) {
        ctx_freelist_len++;
        self->ctx_weakreflist = (PyObject *)ctx_freelist;
        ctx_freelist = self;
    }
    else {
        Py_TYPE(self)->tp_free(self);
    }
}

static PyObject *
context_tp_iter(PyContext *self)
{
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
context_tp_len(PyContext *self)
{
    return _PyHamt_Len(self->ctx_vars);
}

static PyObject *
context_tp_subscript(PyContext *self, PyObject *key)
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
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    Py_INCREF(val);
    return val;
}

static int
context_tp_contains(PyContext *self, PyObject *key)
{
    if (context_check_key_type(key)) {
        return -1;
    }
    PyObject *val = NULL;
    return _PyHamt_Find(self->ctx_vars, key, &val);
}


/*[clinic input]
_contextvars.Context.get
    key: object
    default: object = None
    /
[clinic start generated code]*/

static PyObject *
_contextvars_Context_get_impl(PyContext *self, PyObject *key,
                              PyObject *default_value)
/*[clinic end generated code: output=0c54aa7664268189 input=8d4c33c8ecd6d769]*/
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
        Py_INCREF(default_value);
        return default_value;
    }
    Py_INCREF(val);
    return val;
}


/*[clinic input]
_contextvars.Context.items
[clinic start generated code]*/

static PyObject *
_contextvars_Context_items_impl(PyContext *self)
/*[clinic end generated code: output=fa1655c8a08502af input=2d570d1455004979]*/
{
    return _PyHamt_NewIterItems(self->ctx_vars);
}


/*[clinic input]
_contextvars.Context.keys
[clinic start generated code]*/

static PyObject *
_contextvars_Context_keys_impl(PyContext *self)
/*[clinic end generated code: output=177227c6b63ec0e2 input=13005e142fbbf37d]*/
{
    return _PyHamt_NewIterKeys(self->ctx_vars);
}


/*[clinic input]
_contextvars.Context.values
[clinic start generated code]*/

static PyObject *
_contextvars_Context_values_impl(PyContext *self)
/*[clinic end generated code: output=d286dabfc8db6dde input=c2cbc40a4470e905]*/
{
    return _PyHamt_NewIterValues(self->ctx_vars);
}


static PyObject *
context_run(PyContext *self, PyObject *const *args,
            Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs < 1) {
        PyErr_SetString(PyExc_TypeError,
                        "run() missing 1 required positional argument");
        return NULL;
    }

    if (PyContext_Enter(self)) {
        return NULL;
    }

    PyObject *call_result = _PyObject_FastCallKeywords(
        args[0], args + 1, nargs - 1, kwnames);

    if (PyContext_Exit(self)) {
        return NULL;
    }

    return call_result;
}


static PyMethodDef PyContext_methods[] = {
    _CONTEXTVARS_CONTEXT_GET_METHODDEF
    _CONTEXTVARS_CONTEXT_ITEMS_METHODDEF
    _CONTEXTVARS_CONTEXT_KEYS_METHODDEF
    _CONTEXTVARS_CONTEXT_VALUES_METHODDEF
    {"run", (PyCFunction)context_run, METH_FASTCALL | METH_KEYWORDS, NULL},
    {NULL, NULL}
};

static PySequenceMethods PyContext_as_sequence = {
    0,                                   /* sq_length */
    0,                                   /* sq_concat */
    0,                                   /* sq_repeat */
    0,                                   /* sq_item */
    0,                                   /* sq_slice */
    0,                                   /* sq_ass_item */
    0,                                   /* sq_ass_slice */
    (objobjproc)context_tp_contains,     /* sq_contains */
    0,                                   /* sq_inplace_concat */
    0,                                   /* sq_inplace_repeat */
};

static PyMappingMethods PyContext_as_mapping = {
    (lenfunc)context_tp_len,             /* mp_length */
    (binaryfunc)context_tp_subscript,    /* mp_subscript */
};

PyTypeObject PyContext_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "Context",
    sizeof(PyContext),
    .tp_methods = PyContext_methods,
    .tp_as_mapping = &PyContext_as_mapping,
    .tp_as_sequence = &PyContext_as_sequence,
    .tp_iter = (getiterfunc)context_tp_iter,
    .tp_dealloc = (destructor)context_tp_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_richcompare = context_tp_richcompare,
    .tp_traverse = (traverseproc)context_tp_traverse,
    .tp_clear = (inquiry)context_tp_clear,
    .tp_new = context_tp_new,
    .tp_weaklistoffset = offsetof(PyContext, ctx_weakreflist),
    .tp_hash = PyObject_HashNotImplemented,
};


/////////////////////////// ContextVar


static int
contextvar_set(PyContextVar *var, PyObject *val)
{
    var->var_cached = NULL;

    PyThreadState *ts = PyThreadState_Get();
    if (ts->contextvars == NULL) {
        ts->contextvars = (PyObject *)_PyHamt_New();
        if (ts->contextvars == NULL) {
            return -1;
        }
    }

    assert(PyHamt_Check(ts->contextvars));
    PyHamtObject *vars = (PyHamtObject *)ts->contextvars;

    PyHamtObject *new_vars = _PyHamt_Assoc(vars, (PyObject *)var, val);
    if (new_vars == NULL) {
        return -1;
    }

    Py_SETREF(ts->contextvars, (PyObject *)new_vars);

    var->var_cached = val;  /* borrow */
    var->var_cached_tsid = ts->id;
    var->var_cached_tsver = ts->contextvars_stack_ver;
    return 0;
}

static int
contextvar_del(PyContextVar *var)
{
    var->var_cached = NULL;

    PyThreadState *ts = PyThreadState_Get();
    if (ts->contextvars == NULL) {
        return 0;
    }

    assert(PyHamt_Check(ts->contextvars));
    PyHamtObject *vars = (PyHamtObject *)ts->contextvars;

    PyHamtObject *new_vars = _PyHamt_Without(vars, (PyObject *)var);
    if (new_vars == NULL) {
        return -1;
    }

    if (vars == new_vars) {
        Py_DECREF(new_vars);
        PyErr_SetObject(PyExc_LookupError, (PyObject *)var);
        return -1;
    }

    ts->contextvars = (PyObject *)new_vars;
    Py_DECREF(vars);
    return 0;
}

static Py_hash_t
contextvar_generate_hash(void *addr, PyObject *name)
{
    /* Take hash of the name and XOR it with the default object hash.
       We do this to ensure that even sequentially allocated ContextVar
       objects have drastically different hashes.

       OTOH, we can't just return the hash of name.  For two variables
       below:

            c1 = ContextVar('somename')
            c2 = ContextVar('somename')

       c1 and c2 are completely different variables that happen to
       have the same name.
    */

    Py_hash_t name_hash = PyObject_Hash(name);
    if (name_hash == -1) {
        return -1;
    }

    Py_hash_t res = _Py_HashPointer(addr) ^ name_hash;
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

    var->var_hash = contextvar_generate_hash(var, name);
    if (var->var_hash == -1) {
        Py_DECREF(var);
        return NULL;
    }

    Py_INCREF(name);
    var->var_name = name;

    Py_XINCREF(def);
    var->var_default = def;

    var->var_cached = NULL;
    var->var_cached_tsid = 0;
    var->var_cached_tsver = 0;

    PyObject_GC_Track(var);
    return var;
}


/*[clinic input]
class _contextvars.ContextVar "PyContextVar *" "&PyContextVar_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=445da935fa8883c3]*/


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
contextvar_tp_clear(PyContextVar *self)
{
    Py_CLEAR(self->var_name);
    Py_CLEAR(self->var_default);
    self->var_cached = NULL;
    self->var_cached_tsid = 0;
    self->var_cached_tsver = 0;
    return 0;
}

static int
contextvar_tp_traverse(PyContextVar *self, visitproc visit, void *arg)
{
    Py_VISIT(self->var_name);
    Py_VISIT(self->var_default);
    return 0;
}

static void
contextvar_tp_dealloc(PyContextVar *self)
{
    PyObject_GC_UnTrack(self);
    (void)contextvar_tp_clear(self);
    Py_TYPE(self)->tp_free(self);
}

static Py_hash_t
contextvar_tp_hash(PyContextVar *self)
{
    return self->var_hash;
}

static PyObject *
contextvar_tp_repr(PyContextVar *self)
{
    _PyUnicodeWriter writer;

    _PyUnicodeWriter_Init(&writer);
    writer.min_length = 33;  /* len of "<ContextVar name= at 0x10b0cd0f0>" */

    if (_PyUnicodeWriter_WriteASCIIString(
            &writer, "<ContextVar name=", 17) < 0)
    {
        goto error;
    }

    PyObject *name = PyObject_Repr(self->var_name);
    if (name == NULL) {
        goto error;
    }
    if (_PyUnicodeWriter_WriteStr(&writer, name) < 0) {
        Py_DECREF(name);
        goto error;
    }
    Py_DECREF(name);

    if (self->var_default != NULL) {
        if (_PyUnicodeWriter_WriteASCIIString(&writer, " default=", 9) < 0) {
            goto error;
        }

        PyObject *def = PyObject_Repr(self->var_default);
        if (def == NULL) {
            goto error;
        }
        if (_PyUnicodeWriter_WriteStr(&writer, def) < 0) {
            Py_DECREF(def);
            goto error;
        }
        Py_DECREF(def);
    }

    PyObject *addr = PyUnicode_FromFormat(" at %p>", self);
    if (addr == NULL) {
        goto error;
    }
    if (_PyUnicodeWriter_WriteStr(&writer, addr) < 0) {
        Py_DECREF(addr);
        goto error;
    }
    Py_DECREF(addr);

    return _PyUnicodeWriter_Finish(&writer);

error:
    _PyUnicodeWriter_Dealloc(&writer);
    return NULL;
}


/*[clinic input]
_contextvars.ContextVar.get
    default: object = NULL
    /
[clinic start generated code]*/

static PyObject *
_contextvars_ContextVar_get_impl(PyContextVar *self, PyObject *default_value)
/*[clinic end generated code: output=0746bd0aa2ced7bf input=8d002b02eebbb247]*/
{
    if (!PyContextVar_CheckExact(self)) {
        PyErr_SetString(
            PyExc_TypeError, "an instance of ContextVar was expected");
        return NULL;
    }

    PyObject *val;
    if (PyContextVar_Get(self, default_value, &val) < 0) {
        return NULL;
    }

    if (val == NULL) {
        PyErr_SetObject(PyExc_LookupError, (PyObject *)self);
        return NULL;
    }

    return val;
}

/*[clinic input]
_contextvars.ContextVar.set
    value: object
    /
[clinic start generated code]*/

static PyObject *
_contextvars_ContextVar_set(PyContextVar *self, PyObject *value)
/*[clinic end generated code: output=446ed5e820d6d60b input=a2d88f57c6d86f7c]*/
{
    return (PyObject *)PyContextVar_Set(self, value);
}

/*[clinic input]
_contextvars.ContextVar.reset
    token: object
    /
[clinic start generated code]*/

static PyObject *
_contextvars_ContextVar_reset(PyContextVar *self, PyObject *token)
/*[clinic end generated code: output=d4ee34d0742d62ee input=4c871b6f1f31a65f]*/
{
    if (!PyContextToken_CheckExact(token)) {
        PyErr_Format(PyExc_TypeError,
                     "expected an instance of Token, got %R", token);
        return NULL;
    }

    if (PyContextVar_Reset(self, (PyContextToken *)token)) {
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
contextvar_cls_getitem(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}


static PyMethodDef PyContextVar_methods[] = {
    _CONTEXTVARS_CONTEXTVAR_GET_METHODDEF
    _CONTEXTVARS_CONTEXTVAR_SET_METHODDEF
    _CONTEXTVARS_CONTEXTVAR_RESET_METHODDEF
    {"__class_getitem__", contextvar_cls_getitem,
        METH_VARARGS | METH_STATIC, NULL},
    {NULL, NULL}
};

PyTypeObject PyContextVar_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "ContextVar",
    sizeof(PyContextVar),
    .tp_methods = PyContextVar_methods,
    .tp_dealloc = (destructor)contextvar_tp_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)contextvar_tp_traverse,
    .tp_clear = (inquiry)contextvar_tp_clear,
    .tp_new = contextvar_tp_new,
    .tp_free = PyObject_GC_Del,
    .tp_hash = (hashfunc)contextvar_tp_hash,
    .tp_repr = (reprfunc)contextvar_tp_repr,
};


/////////////////////////// Token

static PyObject * get_token_missing(void);


/*[clinic input]
class _contextvars.Token "PyContextToken *" "&PyContextToken_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=338a5e2db13d3f5b]*/


static PyObject *
token_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_RuntimeError,
                    "Tokens can only be created by ContextVars");
    return NULL;
}

static int
token_tp_clear(PyContextToken *self)
{
    Py_CLEAR(self->tok_var);
    Py_CLEAR(self->tok_oldval);
    return 0;
}

static int
token_tp_traverse(PyContextToken *self, visitproc visit, void *arg)
{
    Py_VISIT(self->tok_var);
    Py_VISIT(self->tok_oldval);
    return 0;
}

static void
token_tp_dealloc(PyContextToken *self)
{
    PyObject_GC_UnTrack(self);
    (void)token_tp_clear(self);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
token_get_var(PyContextToken *self)
{
    Py_INCREF(self->tok_var);
    return (PyObject *)self->tok_var;
}

static PyObject *
token_get_old_val(PyContextToken *self)
{
    if (self->tok_oldval == NULL) {
        return get_token_missing();
    }

    Py_INCREF(self->tok_oldval);
    return self->tok_oldval;
}

static PyGetSetDef PyContextTokenType_getsetlist[] = {
    {"var", (getter)token_get_var, NULL, NULL},
    {"old_val", (getter)token_get_old_val, NULL, NULL},
    {NULL}
};

PyTypeObject PyContextToken_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "Token",
    sizeof(PyContextToken),
    .tp_getset = PyContextTokenType_getsetlist,
    .tp_dealloc = (destructor)token_tp_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)token_tp_traverse,
    .tp_clear = (inquiry)token_tp_clear,
    .tp_new = token_tp_new,
    .tp_free = PyObject_GC_Del,
    .tp_hash = PyObject_HashNotImplemented,
};

static PyContextToken *
token_new(PyContextVar *var, PyObject *val)
{
    PyContextToken *tok = PyObject_GC_New(PyContextToken, &PyContextToken_Type);
    if (tok == NULL) {
        return NULL;
    }

    Py_INCREF(var);
    tok->tok_var = var;

    Py_XINCREF(val);
    tok->tok_oldval = val;

    tok->tok_used = 0;

    PyObject_GC_Track(tok);
    return tok;
}


/////////////////////////// Token.MISSING


static PyObject *_token_missing;


typedef struct {
    PyObject_HEAD
} PyContextTokenMissing;


static PyObject *
context_token_missing_tp_repr(PyObject *self)
{
    return PyUnicode_FromString("<Token.MISSING>");
}


PyTypeObject PyContextTokenMissing_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "Token.MISSING",
    sizeof(PyContextTokenMissing),
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_repr = context_token_missing_tp_repr,
};


static PyObject *
get_token_missing(void)
{
    if (_token_missing != NULL) {
        Py_INCREF(_token_missing);
        return _token_missing;
    }

    _token_missing = (PyObject *)PyObject_New(
        PyContextTokenMissing, &PyContextTokenMissing_Type);
    if (_token_missing == NULL) {
        return NULL;
    }

    Py_INCREF(_token_missing);
    return _token_missing;
}


///////////////////////////


int
PyContext_ClearFreeList(void)
{
    int size = ctx_freelist_len;
    while (ctx_freelist_len) {
        PyContext *ctx = ctx_freelist;
        ctx_freelist = (PyContext *)ctx->ctx_weakreflist;
        ctx->ctx_weakreflist = NULL;
        PyObject_GC_Del(ctx);
        ctx_freelist_len--;
    }
    return size;
}


void
_PyContext_Fini(void)
{
    Py_CLEAR(_token_missing);
    (void)PyContext_ClearFreeList();
    (void)_PyHamt_Fini();
}


int
_PyContext_Init(void)
{
    if (!_PyHamt_Init()) {
        return 0;
    }

    if ((PyType_Ready(&PyContext_Type) < 0) ||
        (PyType_Ready(&PyContextVar_Type) < 0) ||
        (PyType_Ready(&PyContextToken_Type) < 0) ||
        (PyType_Ready(&PyContextTokenMissing_Type) < 0))
    {
        return 0;
    }

    PyObject *missing = get_token_missing();
    if (PyDict_SetItemString(
        PyContextToken_Type.tp_dict, "MISSING", missing))
    {
        Py_DECREF(missing);
        return 0;
    }
    Py_DECREF(missing);

    return 1;
}
