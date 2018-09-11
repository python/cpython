/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(itertools_groupby__doc__,
"groupby(iterable, key=None)\n"
"--\n"
"\n"
"make an iterator that returns consecutive keys and groups from the iterable\n"
"\n"
"  iterable\n"
"    Elements to divide into groups according to the key function.\n"
"  key\n"
"    A function for computing the group category for each element.\n"
"    If the key function is not specified or is None, the element itself\n"
"    is used for grouping.");

static PyObject *
itertools_groupby_impl(PyTypeObject *type, PyObject *it, PyObject *keyfunc);

static PyObject *
itertools_groupby(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "key", NULL};
    static _PyArg_Parser _parser = {"O|O:groupby", _keywords, 0};
    PyObject *it;
    PyObject *keyfunc = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &it, &keyfunc)) {
        goto exit;
    }
    return_value = itertools_groupby_impl(type, it, keyfunc);

exit:
    return return_value;
}

static PyObject *
itertools__grouper_impl(PyTypeObject *type, PyObject *parent,
                        PyObject *tgtkey);

static PyObject *
itertools__grouper(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *parent;
    PyObject *tgtkey;

    if ((type == &_grouper_type) &&
        !_PyArg_NoKeywords("_grouper", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "O!O:_grouper",
        &groupby_type, &parent, &tgtkey)) {
        goto exit;
    }
    return_value = itertools__grouper_impl(type, parent, tgtkey);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_teedataobject__doc__,
"teedataobject(iterable, values, next, /)\n"
"--\n"
"\n"
"Data container common to multiple tee objects.");

static PyObject *
itertools_teedataobject_impl(PyTypeObject *type, PyObject *it,
                             PyObject *values, PyObject *next);

static PyObject *
itertools_teedataobject(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *it;
    PyObject *values;
    PyObject *next;

    if ((type == &teedataobject_type) &&
        !_PyArg_NoKeywords("teedataobject", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "OO!O:teedataobject",
        &it, &PyList_Type, &values, &next)) {
        goto exit;
    }
    return_value = itertools_teedataobject_impl(type, it, values, next);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools__tee__doc__,
"_tee(iterable, /)\n"
"--\n"
"\n"
"Iterator wrapped to make it copyable.");

static PyObject *
itertools__tee_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
itertools__tee(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;

    if ((type == &tee_type) &&
        !_PyArg_NoKeywords("_tee", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "_tee",
        1, 1,
        &iterable)) {
        goto exit;
    }
    return_value = itertools__tee_impl(type, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_tee__doc__,
"tee($module, iterable, n=2, /)\n"
"--\n"
"\n"
"Returns a tuple of n independent iterators.");

#define ITERTOOLS_TEE_METHODDEF    \
    {"tee", (PyCFunction)itertools_tee, METH_FASTCALL, itertools_tee__doc__},

static PyObject *
itertools_tee_impl(PyObject *module, PyObject *iterable, Py_ssize_t n);

static PyObject *
itertools_tee(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    Py_ssize_t n = 2;

    if (!_PyArg_ParseStack(args, nargs, "O|n:tee",
        &iterable, &n)) {
        goto exit;
    }
    return_value = itertools_tee_impl(module, iterable, n);

exit:
    return return_value;
}
/*[clinic end generated code: output=93e1ec70438f4ebb input=a9049054013a1b77]*/
