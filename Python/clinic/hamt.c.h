/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(hamt_set__doc__,
"set($self, key, val, /)\n"
"--\n"
"\n"
"Return a copy of the mapping with the key set to the value.");

#define HAMT_SET_METHODDEF    \
    {"set", _PyCFunction_CAST(hamt_set), METH_FASTCALL, hamt_set__doc__},

static PyObject *
hamt_set_impl(PyHamtObject *self, PyObject *key, PyObject *val);

static PyObject *
hamt_set(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *val;

    if (!_PyArg_CheckPositional("set", nargs, 2, 2)) {
        goto exit;
    }
    key = args[0];
    val = args[1];
    return_value = hamt_set_impl((PyHamtObject *)self, key, val);

exit:
    return return_value;
}

PyDoc_STRVAR(hamt_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for the key, or the default if it is not found.");

#define HAMT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(hamt_get), METH_FASTCALL, hamt_get__doc__},

static PyObject *
hamt_get_impl(PyHamtObject *self, PyObject *key, PyObject *def);

static PyObject *
hamt_get(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *def = Py_None;

    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    def = args[1];
skip_optional:
    return_value = hamt_get_impl((PyHamtObject *)self, key, def);

exit:
    return return_value;
}
/*[clinic end generated code: output=2531b93a9a37546f input=a9049054013a1b77]*/
