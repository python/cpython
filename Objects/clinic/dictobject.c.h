/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(dict_fromkeys__doc__,
"fromkeys($type, iterable, value=None, /)\n"
"--\n"
"\n"
"Create a new dictionary with keys from iterable and values set to value.");

#define DICT_FROMKEYS_METHODDEF    \
    {"fromkeys", (PyCFunction)dict_fromkeys, METH_FASTCALL|METH_CLASS, dict_fromkeys__doc__},

static PyObject *
dict_fromkeys_impl(PyTypeObject *type, PyObject *iterable, PyObject *value);

static PyObject *
dict_fromkeys(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    PyObject *value = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "fromkeys",
        1, 2,
        &iterable, &value)) {
        goto exit;
    }
    return_value = dict_fromkeys_impl(type, iterable, value);

exit:
    return return_value;
}

PyDoc_STRVAR(dict___contains____doc__,
"__contains__($self, key, /)\n"
"--\n"
"\n"
"True if the dictionary has the specified key, else False.");

#define DICT___CONTAINS___METHODDEF    \
    {"__contains__", (PyCFunction)dict___contains__, METH_O|METH_COEXIST, dict___contains____doc__},

PyDoc_STRVAR(dict_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define DICT_GET_METHODDEF    \
    {"get", (PyCFunction)dict_get, METH_FASTCALL, dict_get__doc__},

static PyObject *
dict_get_impl(PyDictObject *self, PyObject *key, PyObject *default_value);

static PyObject *
dict_get(PyDictObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "get",
        1, 2,
        &key, &default_value)) {
        goto exit;
    }
    return_value = dict_get_impl(self, key, default_value);

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
    {"setdefault", (PyCFunction)dict_setdefault, METH_FASTCALL, dict_setdefault__doc__},

static PyObject *
dict_setdefault_impl(PyDictObject *self, PyObject *key,
                     PyObject *default_value);

static PyObject *
dict_setdefault(PyDictObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "setdefault",
        1, 2,
        &key, &default_value)) {
        goto exit;
    }
    return_value = dict_setdefault_impl(self, key, default_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=d7508c5091609a23 input=a9049054013a1b77]*/
