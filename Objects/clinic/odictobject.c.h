/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(OrderedDict_fromkeys__doc__,
"fromkeys($type, /, iterable, value=None)\n"
"--\n"
"\n"
"Create a new ordered dictionary with keys from iterable and values set to value.");

#define ORDEREDDICT_FROMKEYS_METHODDEF    \
    {"fromkeys", (PyCFunction)(void(*)(void))OrderedDict_fromkeys, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, OrderedDict_fromkeys__doc__},

static PyObject *
OrderedDict_fromkeys_impl(PyTypeObject *type, PyObject *seq, PyObject *value);

static PyObject *
OrderedDict_fromkeys(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "value", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fromkeys", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *seq;
    PyObject *value = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    seq = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    value = args[1];
skip_optional_pos:
    return_value = OrderedDict_fromkeys_impl(type, seq, value);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_setdefault__doc__,
"setdefault($self, /, key, default=None)\n"
"--\n"
"\n"
"Insert key with a value of default if key is not in the dictionary.\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define ORDEREDDICT_SETDEFAULT_METHODDEF    \
    {"setdefault", (PyCFunction)(void(*)(void))OrderedDict_setdefault, METH_FASTCALL|METH_KEYWORDS, OrderedDict_setdefault__doc__},

static PyObject *
OrderedDict_setdefault_impl(PyODictObject *self, PyObject *key,
                            PyObject *default_value);

static PyObject *
OrderedDict_setdefault(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "default", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "setdefault", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *key;
    PyObject *default_value = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = args[1];
skip_optional_pos:
    return_value = OrderedDict_setdefault_impl(self, key, default_value);

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
    {"popitem", (PyCFunction)(void(*)(void))OrderedDict_popitem, METH_FASTCALL|METH_KEYWORDS, OrderedDict_popitem__doc__},

static PyObject *
OrderedDict_popitem_impl(PyODictObject *self, int last);

static PyObject *
OrderedDict_popitem(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"last", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "popitem", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int last = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    last = PyObject_IsTrue(args[0]);
    if (last < 0) {
        goto exit;
    }
skip_optional_pos:
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
    {"move_to_end", (PyCFunction)(void(*)(void))OrderedDict_move_to_end, METH_FASTCALL|METH_KEYWORDS, OrderedDict_move_to_end__doc__},

static PyObject *
OrderedDict_move_to_end_impl(PyODictObject *self, PyObject *key, int last);

static PyObject *
OrderedDict_move_to_end(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"key", "last", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "move_to_end", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *key;
    int last = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    last = PyObject_IsTrue(args[1]);
    if (last < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = OrderedDict_move_to_end_impl(self, key, last);

exit:
    return return_value;
}
/*[clinic end generated code: output=8eb1296df9142908 input=a9049054013a1b77]*/
