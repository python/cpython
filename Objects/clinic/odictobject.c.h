/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(OrderedDict_fromkeys__doc__,
"fromkeys($type, /, iterable, value=None)\n"
"--\n"
"\n"
"Create a new ordered dictionary with keys from iterable and values set to value.");

#define ORDEREDDICT_FROMKEYS_METHODDEF    \
    {"fromkeys", (PyCFunction)OrderedDict_fromkeys, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, OrderedDict_fromkeys__doc__},

static PyObject *
OrderedDict_fromkeys_impl(PyTypeObject *type, PyObject *seq, PyObject *value);

static PyObject *
OrderedDict_fromkeys(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "value", NULL};
    static _PyArg_Parser _parser = {"O|O:fromkeys", _keywords, 0};
    PyObject *seq;
    PyObject *value = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &seq, &value)) {
        goto exit;
    }
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
    {"setdefault", (PyCFunction)OrderedDict_setdefault, METH_FASTCALL|METH_KEYWORDS, OrderedDict_setdefault__doc__},

static PyObject *
OrderedDict_setdefault_impl(PyODictObject *self, PyObject *key,
                            PyObject *default_value);

static PyObject *
OrderedDict_setdefault(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    {"popitem", (PyCFunction)OrderedDict_popitem, METH_FASTCALL|METH_KEYWORDS, OrderedDict_popitem__doc__},

static PyObject *
OrderedDict_popitem_impl(PyODictObject *self, int last);

static PyObject *
OrderedDict_popitem(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
OrderedDict_move_to_end(PyODictObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
/*[clinic end generated code: output=7f23d569eda2a558 input=a9049054013a1b77]*/
