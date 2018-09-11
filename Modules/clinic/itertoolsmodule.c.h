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

PyDoc_STRVAR(itertools_cycle__doc__,
"cycle(iterable, /)\n"
"--\n"
"\n"
"Return elements from the iterable until it is exhausted, then repeat the sequence indefinitely.");

static PyObject *
itertools_cycle_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
itertools_cycle(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;

    if ((type == &cycle_type) &&
        !_PyArg_NoKeywords("cycle", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "cycle",
        1, 1,
        &iterable)) {
        goto exit;
    }
    return_value = itertools_cycle_impl(type, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_dropwhile__doc__,
"dropwhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Drop items from the iterable while predicate(item) is true.\n"
"\n"
"Afterwards, return every element until the iterable is exhausted.");

static PyObject *
itertools_dropwhile_impl(PyTypeObject *type, PyObject *func, PyObject *seq);

static PyObject *
itertools_dropwhile(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *seq;

    if ((type == &dropwhile_type) &&
        !_PyArg_NoKeywords("dropwhile", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "dropwhile",
        2, 2,
        &func, &seq)) {
        goto exit;
    }
    return_value = itertools_dropwhile_impl(type, func, seq);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_takewhile__doc__,
"takewhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Return successive entries from an iterable as long as the predicate evaluates to true for each entry.");

static PyObject *
itertools_takewhile_impl(PyTypeObject *type, PyObject *func, PyObject *seq);

static PyObject *
itertools_takewhile(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *seq;

    if ((type == &takewhile_type) &&
        !_PyArg_NoKeywords("takewhile", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "takewhile",
        2, 2,
        &func, &seq)) {
        goto exit;
    }
    return_value = itertools_takewhile_impl(type, func, seq);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_starmap__doc__,
"starmap(function, iterable, /)\n"
"--\n"
"\n"
"Return an iterator whose values are returned from the function evaluated with an argument tuple taken from the given sequence.");

static PyObject *
itertools_starmap_impl(PyTypeObject *type, PyObject *func, PyObject *seq);

static PyObject *
itertools_starmap(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *seq;

    if ((type == &starmap_type) &&
        !_PyArg_NoKeywords("starmap", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "starmap",
        2, 2,
        &func, &seq)) {
        goto exit;
    }
    return_value = itertools_starmap_impl(type, func, seq);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_chain_from_iterable__doc__,
"from_iterable($type, iterable, /)\n"
"--\n"
"\n"
"Alternative chain() constructor taking a single iterable argument that evaluates lazily.");

#define ITERTOOLS_CHAIN_FROM_ITERABLE_METHODDEF    \
    {"from_iterable", (PyCFunction)itertools_chain_from_iterable, METH_O|METH_CLASS, itertools_chain_from_iterable__doc__},

PyDoc_STRVAR(itertools_combinations__doc__,
"combinations(iterable, r)\n"
"--\n"
"\n"
"Return successive r-length combinations of elements in the iterable.\n"
"\n"
"combinations(range(4), 3) --> (0,1,2), (0,1,3), (0,2,3), (1,2,3)");

static PyObject *
itertools_combinations_impl(PyTypeObject *type, PyObject *iterable,
                            Py_ssize_t r);

static PyObject *
itertools_combinations(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {"On:combinations", _keywords, 0};
    PyObject *iterable;
    Py_ssize_t r;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &iterable, &r)) {
        goto exit;
    }
    return_value = itertools_combinations_impl(type, iterable, r);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_combinations_with_replacement__doc__,
"combinations_with_replacement(iterable, r)\n"
"--\n"
"\n"
"Return successive r-length combinations of elements in the iterable allowing individual elements to have successive repeats.\n"
"\n"
"combinations_with_replacement(\'ABC\', 2) --> AA AB AC BB BC CC\"");

static PyObject *
itertools_combinations_with_replacement_impl(PyTypeObject *type,
                                             PyObject *iterable,
                                             Py_ssize_t r);

static PyObject *
itertools_combinations_with_replacement(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {"On:combinations_with_replacement", _keywords, 0};
    PyObject *iterable;
    Py_ssize_t r;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &iterable, &r)) {
        goto exit;
    }
    return_value = itertools_combinations_with_replacement_impl(type, iterable, r);

exit:
    return return_value;
}
/*[clinic end generated code: output=51d46b2146b18d23 input=a9049054013a1b77]*/
