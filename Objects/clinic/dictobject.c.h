/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(dict_fromkeys__doc__,
"fromkeys($type, /, iterable, value=None)\n"
"--\n"
"\n"
"Create a new dictionary with keys from iterable and values set to value.");

#define DICT_FROMKEYS_METHODDEF    \
    {"fromkeys", (PyCFunction)dict_fromkeys, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, dict_fromkeys__doc__},

static PyObject *
dict_fromkeys_impl(PyTypeObject *type, PyObject *iterable, PyObject *value);

static PyObject *
dict_fromkeys(PyTypeObject *type, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "value", NULL};
    static _PyArg_Parser _parser = {"O|O:fromkeys", _keywords, 0};
    PyObject *iterable;
    PyObject *value = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
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
dict_get(PyDictObject *self, PyObject **args, Py_ssize_t nargs)
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
"setdefault($self, /, key, default=None)\n"
"--\n"
"\n"
"Insert key with a value of default if key is not in the dictionary.\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define DICT_SETDEFAULT_METHODDEF    \
    {"setdefault", (PyCFunction)dict_setdefault, METH_FASTCALL|METH_KEYWORDS, dict_setdefault__doc__},

static PyObject *
dict_setdefault_impl(PyDictObject *self, PyObject *key,
                     PyObject *default_value);

static PyObject *
dict_setdefault(PyDictObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "default", NULL};
    static _PyArg_Parser _parser = {"O|O:setdefault", _keywords, 0};
    PyObject *key;
    PyObject *default_value = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &key, &default_value)) {
        goto exit;
    }
    return_value = dict_setdefault_impl(self, key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_popitem__doc__,
"popitem($self, /, last=True)\n"
"--\n"
"\n"
"Remove and return a (key, value) pair from the dictionary.\n"
"\n"
"Pairs are returned in LIFO order if last is true or FIFO order if false.");

#define ORDEREDDICT_POPITEM_METHODDEF    \
    {"popitem", (PyCFunction)OrderedDict_popitem, METH_FASTCALL|METH_KEYWORDS, OrderedDict_popitem__doc__},

static PyObject *
OrderedDict_popitem_impl(PyODictObject *self, int last);

static PyObject *
OrderedDict_popitem(PyODictObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"last", NULL};
    static _PyArg_Parser _parser = {"|p:popitem", _keywords, 0};
    int last = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &last)) {
        goto exit;
    }
    return_value = OrderedDict_popitem_impl(self, last);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_move_to_end__doc__,
"move_to_end($self, /, key, last=True)\n"
"--\n"
"\n"
"Move an existing element to the end (or beginning if last is false).\n"
"\n"
"Raise KeyError if the element does not exist.");

#define ORDEREDDICT_MOVE_TO_END_METHODDEF    \
    {"move_to_end", (PyCFunction)OrderedDict_move_to_end, METH_FASTCALL|METH_KEYWORDS, OrderedDict_move_to_end__doc__},

static PyObject *
OrderedDict_move_to_end_impl(PyODictObject *self, PyObject *key, int last);

static PyObject *
OrderedDict_move_to_end(PyODictObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "last", NULL};
    static _PyArg_Parser _parser = {"O|p:move_to_end", _keywords, 0};
    PyObject *key;
    int last = 1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &key, &last)) {
        goto exit;
    }
    return_value = OrderedDict_move_to_end_impl(self, key, last);

exit:
    return return_value;
}
/*[clinic end generated code: output=96a451cf353bd36f input=a9049054013a1b77]*/
