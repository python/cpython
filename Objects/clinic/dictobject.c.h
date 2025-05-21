/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(dict_fromkeys__doc__,
"fromkeys($type, iterable, value=None, /)\n"
"--\n"
"\n"
"Create a new dictionary with keys from iterable and values set to value.");

#define DICT_FROMKEYS_METHODDEF    \
    {"fromkeys", _PyCFunction_CAST(dict_fromkeys), METH_FASTCALL|METH_CLASS, dict_fromkeys__doc__},

static PyObject *
dict_fromkeys_impl(PyTypeObject *type, PyObject *iterable, PyObject *value);

static PyObject *
dict_fromkeys(PyObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    PyObject *value = Py_None;

    if (!_PyArg_CheckPositional("fromkeys", nargs, 1, 2)) {
        goto exit;
    }
    iterable = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    value = args[1];
skip_optional:
    return_value = dict_fromkeys_impl((PyTypeObject *)type, iterable, value);

exit:
    return return_value;
}

PyDoc_STRVAR(dict_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of the dict.");

#define DICT_COPY_METHODDEF    \
    {"copy", (PyCFunction)dict_copy, METH_NOARGS, dict_copy__doc__},

static PyObject *
dict_copy_impl(PyDictObject *self);

static PyObject *
dict_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return dict_copy_impl((PyDictObject *)self);
}

PyDoc_STRVAR(dict___contains____doc__,
"__contains__($self, key, /)\n"
"--\n"
"\n"
"True if the dictionary has the specified key, else False.");

#define DICT___CONTAINS___METHODDEF    \
    {"__contains__", (PyCFunction)dict___contains__, METH_O|METH_COEXIST, dict___contains____doc__},

static PyObject *
dict___contains___impl(PyDictObject *self, PyObject *key);

static PyObject *
dict___contains__(PyObject *self, PyObject *key)
{
    PyObject *return_value = NULL;

    return_value = dict___contains___impl((PyDictObject *)self, key);

    return return_value;
}

PyDoc_STRVAR(dict_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define DICT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(dict_get), METH_FASTCALL, dict_get__doc__},

static PyObject *
dict_get_impl(PyDictObject *self, PyObject *key, PyObject *default_value);

static PyObject *
dict_get(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = dict_get_impl((PyDictObject *)self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(dict_setdefault__doc__,
"setdefault($self, key, default=None, /)\n"
"--\n"
"\n"
"Insert key with a value of default if key is not in the dictionary.\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define DICT_SETDEFAULT_METHODDEF    \
    {"setdefault", _PyCFunction_CAST(dict_setdefault), METH_FASTCALL, dict_setdefault__doc__},

static PyObject *
dict_setdefault_impl(PyDictObject *self, PyObject *key,
                     PyObject *default_value);

static PyObject *
dict_setdefault(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_CheckPositional("setdefault", nargs, 1, 2)) {
        goto exit;
    }
    key = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dict_setdefault_impl((PyDictObject *)self, key, default_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(dict_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the dict.");

#define DICT_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)dict_clear, METH_NOARGS, dict_clear__doc__},

static PyObject *
dict_clear_impl(PyDictObject *self);

static PyObject *
dict_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return dict_clear_impl((PyDictObject *)self);
}

PyDoc_STRVAR(dict_pop__doc__,
"pop($self, key, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"D.pop(k[,d]) -> v, remove specified key and return the corresponding value.\n"
"\n"
"If the key is not found, return the default if given; otherwise,\n"
"raise a KeyError.");

#define DICT_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(dict_pop), METH_FASTCALL, dict_pop__doc__},

static PyObject *
dict_pop_impl(PyDictObject *self, PyObject *key, PyObject *default_value);

static PyObject *
dict_pop(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = NULL;

    if (!_PyArg_CheckPositional("pop", nargs, 1, 2)) {
        goto exit;
    }
    key = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = dict_pop_impl((PyDictObject *)self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(dict_popitem__doc__,
"popitem($self, /)\n"
"--\n"
"\n"
"Remove and return a (key, value) pair as a 2-tuple.\n"
"\n"
"Pairs are returned in LIFO (last-in, first-out) order.\n"
"Raises KeyError if the dict is empty.");

#define DICT_POPITEM_METHODDEF    \
    {"popitem", (PyCFunction)dict_popitem, METH_NOARGS, dict_popitem__doc__},

static PyObject *
dict_popitem_impl(PyDictObject *self);

static PyObject *
dict_popitem(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dict_popitem_impl((PyDictObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(dict___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the dict in memory, in bytes.");

#define DICT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)dict___sizeof__, METH_NOARGS, dict___sizeof____doc__},

static PyObject *
dict___sizeof___impl(PyDictObject *self);

static PyObject *
dict___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return dict___sizeof___impl((PyDictObject *)self);
}

PyDoc_STRVAR(dict___reversed____doc__,
"__reversed__($self, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the dict keys.");

#define DICT___REVERSED___METHODDEF    \
    {"__reversed__", (PyCFunction)dict___reversed__, METH_NOARGS, dict___reversed____doc__},

static PyObject *
dict___reversed___impl(PyDictObject *self);

static PyObject *
dict___reversed__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return dict___reversed___impl((PyDictObject *)self);
}

PyDoc_STRVAR(dict_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Return a set-like object providing a view on the dict\'s keys.");

#define DICT_KEYS_METHODDEF    \
    {"keys", (PyCFunction)dict_keys, METH_NOARGS, dict_keys__doc__},

static PyObject *
dict_keys_impl(PyDictObject *self);

static PyObject *
dict_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return dict_keys_impl((PyDictObject *)self);
}

PyDoc_STRVAR(dict_items__doc__,
"items($self, /)\n"
"--\n"
"\n"
"Return a set-like object providing a view on the dict\'s items.");

#define DICT_ITEMS_METHODDEF    \
    {"items", (PyCFunction)dict_items, METH_NOARGS, dict_items__doc__},

static PyObject *
dict_items_impl(PyDictObject *self);

static PyObject *
dict_items(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return dict_items_impl((PyDictObject *)self);
}

PyDoc_STRVAR(dict_values__doc__,
"values($self, /)\n"
"--\n"
"\n"
"Return an object providing a view on the dict\'s values.");

#define DICT_VALUES_METHODDEF    \
    {"values", (PyCFunction)dict_values, METH_NOARGS, dict_values__doc__},

static PyObject *
dict_values_impl(PyDictObject *self);

static PyObject *
dict_values(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return dict_values_impl((PyDictObject *)self);
}
/*[clinic end generated code: output=9007b74432217017 input=a9049054013a1b77]*/
