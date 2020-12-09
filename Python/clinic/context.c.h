/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_contextvars_Context_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXT_GET_METHODDEF    \
    {"get", (PyCFunction)_contextvars_Context_get, METH_FASTCALL, _contextvars_Context_get__doc__},

static PyObject *
_contextvars_Context_get_impl(PyContext *self, PyObject *key,
                              PyObject *default_value);

static PyObject *
_contextvars_Context_get(PyContext *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "get",
        1, 2,
        &key, &default_value)) {
        goto exit;
    }
    return_value = _contextvars_Context_get_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_contextvars_Context_items__doc__,
"items($self, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXT_ITEMS_METHODDEF    \
    {"items", (PyCFunction)_contextvars_Context_items, METH_NOARGS, _contextvars_Context_items__doc__},

static PyObject *
_contextvars_Context_items_impl(PyContext *self);

static PyObject *
_contextvars_Context_items(PyContext *self, PyObject *Py_UNUSED(ignored))
{
    return _contextvars_Context_items_impl(self);
}

PyDoc_STRVAR(_contextvars_Context_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXT_KEYS_METHODDEF    \
    {"keys", (PyCFunction)_contextvars_Context_keys, METH_NOARGS, _contextvars_Context_keys__doc__},

static PyObject *
_contextvars_Context_keys_impl(PyContext *self);

static PyObject *
_contextvars_Context_keys(PyContext *self, PyObject *Py_UNUSED(ignored))
{
    return _contextvars_Context_keys_impl(self);
}

PyDoc_STRVAR(_contextvars_Context_values__doc__,
"values($self, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXT_VALUES_METHODDEF    \
    {"values", (PyCFunction)_contextvars_Context_values, METH_NOARGS, _contextvars_Context_values__doc__},

static PyObject *
_contextvars_Context_values_impl(PyContext *self);

static PyObject *
_contextvars_Context_values(PyContext *self, PyObject *Py_UNUSED(ignored))
{
    return _contextvars_Context_values_impl(self);
}

PyDoc_STRVAR(_contextvars_Context_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXT_COPY_METHODDEF    \
    {"copy", (PyCFunction)_contextvars_Context_copy, METH_NOARGS, _contextvars_Context_copy__doc__},

static PyObject *
_contextvars_Context_copy_impl(PyContext *self);

static PyObject *
_contextvars_Context_copy(PyContext *self, PyObject *Py_UNUSED(ignored))
{
    return _contextvars_Context_copy_impl(self);
}

PyDoc_STRVAR(_contextvars_ContextVar_get__doc__,
"get($self, default=None, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXTVAR_GET_METHODDEF    \
    {"get", (PyCFunction)_contextvars_ContextVar_get, METH_FASTCALL, _contextvars_ContextVar_get__doc__},

static PyObject *
_contextvars_ContextVar_get_impl(PyContextVar *self, PyObject *default_value);

static PyObject *
_contextvars_ContextVar_get(PyContextVar *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *default_value = NULL;

    if (!_PyArg_UnpackStack(args, nargs, "get",
        0, 1,
        &default_value)) {
        goto exit;
    }
    return_value = _contextvars_ContextVar_get_impl(self, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_contextvars_ContextVar_set__doc__,
"set($self, value, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXTVAR_SET_METHODDEF    \
    {"set", (PyCFunction)_contextvars_ContextVar_set, METH_O, _contextvars_ContextVar_set__doc__},

PyDoc_STRVAR(_contextvars_ContextVar_reset__doc__,
"reset($self, token, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_CONTEXTVAR_RESET_METHODDEF    \
    {"reset", (PyCFunction)_contextvars_ContextVar_reset, METH_O, _contextvars_ContextVar_reset__doc__},
/*[clinic end generated code: output=d9a675e3a52a14fc input=a9049054013a1b77]*/
