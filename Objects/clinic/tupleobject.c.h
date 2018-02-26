/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(tuple_index__doc__,
"index($self, value, start=0, stop=sys.maxsize, /)\n"
"--\n"
"\n"
"Return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define TUPLE_INDEX_METHODDEF    \
    {"index", (PyCFunction)tuple_index, METH_FASTCALL, tuple_index__doc__},

static PyObject *
tuple_index_impl(PyTupleObject *self, PyObject *value, Py_ssize_t start,
                 Py_ssize_t stop);

static PyObject *
tuple_index(PyTupleObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;

    if (!_PyArg_ParseStack(args, nargs, "O|O&O&:index",
        &value, _PyEval_SliceIndexNotNone, &start, _PyEval_SliceIndexNotNone, &stop)) {
        goto exit;
    }
    return_value = tuple_index_impl(self, value, start, stop);

exit:
    return return_value;
}

PyDoc_STRVAR(tuple_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"Return number of occurrences of value.");

#define TUPLE_COUNT_METHODDEF    \
    {"count", (PyCFunction)tuple_count, METH_O, tuple_count__doc__},

PyDoc_STRVAR(tuple_new__doc__,
"tuple(iterable=(), /)\n"
"--\n"
"\n"
"Built-in immutable sequence.\n"
"\n"
"If no argument is given, the constructor returns an empty tuple.\n"
"If iterable is specified the tuple is initialized from iterable\'s items.\n"
"\n"
"If the argument is a tuple, the return value is the same object.");

static PyObject *
tuple_new_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
tuple_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable = NULL;

    if ((type == &PyTuple_Type) &&
        !_PyArg_NoKeywords("tuple", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "tuple",
        0, 1,
        &iterable)) {
        goto exit;
    }
    return_value = tuple_new_impl(type, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(tuple___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define TUPLE___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)tuple___getnewargs__, METH_NOARGS, tuple___getnewargs____doc__},

static PyObject *
tuple___getnewargs___impl(PyTupleObject *self);

static PyObject *
tuple___getnewargs__(PyTupleObject *self, PyObject *Py_UNUSED(ignored))
{
    return tuple___getnewargs___impl(self);
}
/*[clinic end generated code: output=0fbf4321fb4365ac input=a9049054013a1b77]*/
