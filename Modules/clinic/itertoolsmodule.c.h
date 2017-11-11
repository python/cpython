/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(itertools_groupby__doc__,
"groupby(iterable, key=None)\n"
"--\n"
"\n"
"Return an iterator of groups of items as (key, group) pairs.\n"
"\n"
"  iterable\n"
"    Elements to divide into groups according to the key function.\n"
"  key\n"
"    Function computing a key value for each element.\n"
"    If None, the elements themselves are used for grouping.\n"
"\n"
"This groups together consecutive items from the given iterator.  Each group\n"
"contains items for which the key gives the same result.\n"
"\n"
"For each group, a (key, group) 2-tuple is yielded, where \"key\" is the common\n"
"key value for the group and \"group\" is an iterator of the items in the group.");

static PyObject *
itertools_groupby_impl(PyTypeObject *type, PyObject *iterable, PyObject *key);

static PyObject *
itertools_groupby(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "key", NULL};
    static _PyArg_Parser _parser = {"O|O:groupby", _keywords, 0};
    PyObject *iterable;
    PyObject *key = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &iterable, &key)) {
        goto exit;
    }
    return_value = itertools_groupby_impl(type, iterable, key);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_groupby___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_GROUPBY___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_groupby___reduce__, METH_NOARGS, itertools_groupby___reduce____doc__},

static PyObject *
itertools_groupby___reduce___impl(groupbyobject *lz);

static PyObject *
itertools_groupby___reduce__(groupbyobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_groupby___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_groupby___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_GROUPBY___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_groupby___setstate__, METH_O, itertools_groupby___setstate____doc__},

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

PyDoc_STRVAR(itertools__grouper___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS__GROUPER___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools__grouper___reduce__, METH_NOARGS, itertools__grouper___reduce____doc__},

static PyObject *
itertools__grouper___reduce___impl(_grouperobject *lz);

static PyObject *
itertools__grouper___reduce__(_grouperobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools__grouper___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_teedataobject___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_TEEDATAOBJECT___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_teedataobject___reduce__, METH_NOARGS, itertools_teedataobject___reduce____doc__},

static PyObject *
itertools_teedataobject___reduce___impl(teedataobject *tdo);

static PyObject *
itertools_teedataobject___reduce__(teedataobject *tdo, PyObject *Py_UNUSED(ignored))
{
    return itertools_teedataobject___reduce___impl(tdo);
}

PyDoc_STRVAR(itertools_teedataobject__doc__,
"teedataobject(iterable, values, next, /)\n"
"--\n"
"\n"
"Data container common to multiple tee objects.");

static PyObject *
itertools_teedataobject_impl(PyTypeObject *type, PyObject *iterable,
                             PyObject *values, PyObject *next);

static PyObject *
itertools_teedataobject(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    PyObject *values;
    PyObject *next;

    if ((type == &teedataobject_type) &&
        !_PyArg_NoKeywords("teedataobject", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "OO!O:teedataobject",
        &iterable, &PyList_Type, &values, &next)) {
        goto exit;
    }
    return_value = itertools_teedataobject_impl(type, iterable, values, next);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools__tee___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n"
"Returns an independent iterator.");

#define ITERTOOLS__TEE___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)itertools__tee___copy__, METH_NOARGS, itertools__tee___copy____doc__},

static PyObject *
itertools__tee___copy___impl(teeobject *to);

static PyObject *
itertools__tee___copy__(teeobject *to, PyObject *Py_UNUSED(ignored))
{
    return itertools__tee___copy___impl(to);
}

PyDoc_STRVAR(itertools__tee__doc__,
"_tee(iterable, /)\n"
"--\n"
"\n"
"Create a _tee object.\n"
"\n"
"An iterator wrapped to make it copyable.");

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

PyDoc_STRVAR(itertools__tee___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS__TEE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools__tee___reduce__, METH_NOARGS, itertools__tee___reduce____doc__},

static PyObject *
itertools__tee___reduce___impl(teeobject *to);

static PyObject *
itertools__tee___reduce__(teeobject *to, PyObject *Py_UNUSED(ignored))
{
    return itertools__tee___reduce___impl(to);
}

PyDoc_STRVAR(itertools__tee___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS__TEE___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools__tee___setstate__, METH_O, itertools__tee___setstate____doc__},

PyDoc_STRVAR(itertools_tee__doc__,
"tee($module, iterable, n=2, /)\n"
"--\n"
"\n"
"Returns a tuple of n independent iterators from a single iterable.\n"
"\n"
"  n\n"
"    number of independent iterators to return\n"
"\n"
"Once this has been called, the original iterable should not be used anywhere\n"
"else; otherwise, the iterable could get advanced without the tee objects (those\n"
"in the returned tuple) being informed.");

#define ITERTOOLS_TEE_METHODDEF    \
    {"tee", (PyCFunction)itertools_tee, METH_FASTCALL, itertools_tee__doc__},

static PyObject *
itertools_tee_impl(PyObject *module, PyObject *iterable, Py_ssize_t n);

static PyObject *
itertools_tee(PyObject *module, PyObject **args, Py_ssize_t nargs)
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
"Create a cycle object.\n"
"\n"
"This object will return elements from the iterable until it is exhausted.\n"
"Then it will repeat the sequence indefinitely.");

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

PyDoc_STRVAR(itertools_cycle___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_CYCLE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_cycle___reduce__, METH_NOARGS, itertools_cycle___reduce____doc__},

static PyObject *
itertools_cycle___reduce___impl(cycleobject *lz);

static PyObject *
itertools_cycle___reduce__(cycleobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_cycle___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_cycle___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_CYCLE___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_cycle___setstate__, METH_O, itertools_cycle___setstate____doc__},

PyDoc_STRVAR(itertools_dropwhile__doc__,
"dropwhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Create a dropwhile object.\n"
"\n"
"Drops items from the iterable while predicate(item) is true.\n"
"Afterwards, returns every element until the iterable is exhausted.");

static PyObject *
itertools_dropwhile_impl(PyTypeObject *type, PyObject *predicate,
                         PyObject *iterable);

static PyObject *
itertools_dropwhile(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *predicate;
    PyObject *iterable;

    if ((type == &dropwhile_type) &&
        !_PyArg_NoKeywords("dropwhile", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "dropwhile",
        2, 2,
        &predicate, &iterable)) {
        goto exit;
    }
    return_value = itertools_dropwhile_impl(type, predicate, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_dropwhile___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_DROPWHILE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_dropwhile___reduce__, METH_NOARGS, itertools_dropwhile___reduce____doc__},

static PyObject *
itertools_dropwhile___reduce___impl(dropwhileobject *lz);

static PyObject *
itertools_dropwhile___reduce__(dropwhileobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_dropwhile___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_dropwhile___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_DROPWHILE___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_dropwhile___setstate__, METH_O, itertools_dropwhile___setstate____doc__},

PyDoc_STRVAR(itertools_takewhile__doc__,
"takewhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Create a takewhile object.\n"
"\n"
"Return successive entries from an iterable as long as the\n"
"predicate evaluates to true for each entry.");

static PyObject *
itertools_takewhile_impl(PyTypeObject *type, PyObject *predicate,
                         PyObject *iterable);

static PyObject *
itertools_takewhile(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *predicate;
    PyObject *iterable;

    if ((type == &takewhile_type) &&
        !_PyArg_NoKeywords("takewhile", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "takewhile",
        2, 2,
        &predicate, &iterable)) {
        goto exit;
    }
    return_value = itertools_takewhile_impl(type, predicate, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_takewhile___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_TAKEWHILE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_takewhile___reduce__, METH_NOARGS, itertools_takewhile___reduce____doc__},

static PyObject *
itertools_takewhile___reduce___impl(takewhileobject *lz);

static PyObject *
itertools_takewhile___reduce__(takewhileobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_takewhile___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_takewhile___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_TAKEWHILE___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_takewhile___setstate__, METH_O, itertools_takewhile___setstate____doc__},

PyDoc_STRVAR(itertools_islice___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_ISLICE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_islice___reduce__, METH_NOARGS, itertools_islice___reduce____doc__},

static PyObject *
itertools_islice___reduce___impl(isliceobject *lz);

static PyObject *
itertools_islice___reduce__(isliceobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_islice___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_islice___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_ISLICE___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_islice___setstate__, METH_O, itertools_islice___setstate____doc__},

PyDoc_STRVAR(itertools_starmap__doc__,
"starmap(function, iterable, /)\n"
"--\n"
"\n"
"Create a starmap object.\n"
"\n"
"Return an iterator whose values are returned from the function evaluated\n"
"with argument tuples taken from the given iterable.");

static PyObject *
itertools_starmap_impl(PyTypeObject *type, PyObject *function,
                       PyObject *iterable);

static PyObject *
itertools_starmap(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *function;
    PyObject *iterable;

    if ((type == &starmap_type) &&
        !_PyArg_NoKeywords("starmap", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "starmap",
        2, 2,
        &function, &iterable)) {
        goto exit;
    }
    return_value = itertools_starmap_impl(type, function, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_starmap___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_STARMAP___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_starmap___reduce__, METH_NOARGS, itertools_starmap___reduce____doc__},

static PyObject *
itertools_starmap___reduce___impl(starmapobject *lz);

static PyObject *
itertools_starmap___reduce__(starmapobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_starmap___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_chain_from_iterable__doc__,
"from_iterable($type, iterable, /)\n"
"--\n"
"\n"
"Create a chain object.\n"
"\n"
"Alternative chain() constructor taking a single iterable argument\n"
"that evaluates lazily.");

#define ITERTOOLS_CHAIN_FROM_ITERABLE_METHODDEF    \
    {"from_iterable", (PyCFunction)itertools_chain_from_iterable, METH_O|METH_CLASS, itertools_chain_from_iterable__doc__},

PyDoc_STRVAR(itertools_chain___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_CHAIN___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_chain___reduce__, METH_NOARGS, itertools_chain___reduce____doc__},

static PyObject *
itertools_chain___reduce___impl(chainobject *lz);

static PyObject *
itertools_chain___reduce__(chainobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_chain___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_chain___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_CHAIN___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_chain___setstate__, METH_O, itertools_chain___setstate____doc__},

PyDoc_STRVAR(itertools_product___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define ITERTOOLS_PRODUCT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)itertools_product___sizeof__, METH_NOARGS, itertools_product___sizeof____doc__},

static PyObject *
itertools_product___sizeof___impl(productobject *lz);

static PyObject *
itertools_product___sizeof__(productobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_product___sizeof___impl(lz);
}

PyDoc_STRVAR(itertools_product___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_PRODUCT___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_product___reduce__, METH_NOARGS, itertools_product___reduce____doc__},

static PyObject *
itertools_product___reduce___impl(productobject *lz);

static PyObject *
itertools_product___reduce__(productobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_product___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_product___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_PRODUCT___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_product___setstate__, METH_O, itertools_product___setstate____doc__},

PyDoc_STRVAR(itertools_combinations__doc__,
"combinations(iterable, r)\n"
"--\n"
"\n"
"Create a combinations object.\n"
"\n"
"Returns successive r-length combinations of elements in the iterable.\n"
"\n"
"Example:\n"
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

PyDoc_STRVAR(itertools_combinations___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define ITERTOOLS_COMBINATIONS___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)itertools_combinations___sizeof__, METH_NOARGS, itertools_combinations___sizeof____doc__},

static PyObject *
itertools_combinations___sizeof___impl(combinationsobject *co);

static PyObject *
itertools_combinations___sizeof__(combinationsobject *co, PyObject *Py_UNUSED(ignored))
{
    return itertools_combinations___sizeof___impl(co);
}

PyDoc_STRVAR(itertools_combinations___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_COMBINATIONS___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_combinations___reduce__, METH_NOARGS, itertools_combinations___reduce____doc__},

static PyObject *
itertools_combinations___reduce___impl(combinationsobject *lz);

static PyObject *
itertools_combinations___reduce__(combinationsobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_combinations___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_combinations___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_COMBINATIONS___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_combinations___setstate__, METH_O, itertools_combinations___setstate____doc__},

PyDoc_STRVAR(itertools_combinations_with_replacement__doc__,
"combinations_with_replacement(iterable, r)\n"
"--\n"
"\n"
"Create a combinations object.\n"
"\n"
"Returns successive r-length combinations of elements in the iterable allowing\n"
"individual elements to have successive repeats.\n"
"\n"
"Example:\n"
"combinations_with_replacement(\'ABC\', 2) --> AA AB AC BB BC CC");

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

PyDoc_STRVAR(itertools_combinations_with_replacement___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define ITERTOOLS_COMBINATIONS_WITH_REPLACEMENT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)itertools_combinations_with_replacement___sizeof__, METH_NOARGS, itertools_combinations_with_replacement___sizeof____doc__},

static PyObject *
itertools_combinations_with_replacement___sizeof___impl(cwrobject *co);

static PyObject *
itertools_combinations_with_replacement___sizeof__(cwrobject *co, PyObject *Py_UNUSED(ignored))
{
    return itertools_combinations_with_replacement___sizeof___impl(co);
}

PyDoc_STRVAR(itertools_combinations_with_replacement___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_COMBINATIONS_WITH_REPLACEMENT___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_combinations_with_replacement___reduce__, METH_NOARGS, itertools_combinations_with_replacement___reduce____doc__},

static PyObject *
itertools_combinations_with_replacement___reduce___impl(cwrobject *lz);

static PyObject *
itertools_combinations_with_replacement___reduce__(cwrobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_combinations_with_replacement___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_combinations_with_replacement___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_COMBINATIONS_WITH_REPLACEMENT___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_combinations_with_replacement___setstate__, METH_O, itertools_combinations_with_replacement___setstate____doc__},

PyDoc_STRVAR(itertools_permutations__doc__,
"permutations(iterable, r=None)\n"
"--\n"
"\n"
"Create a permutations object.\n"
"\n"
"Returns successive r-length permutations of elements in the iterable.\n"
"\n"
"Example:\n"
"permutations(range(3), 2) --> (0,1), (0,2), (1,0), (1,2), (2,0), (2,1)");

static PyObject *
itertools_permutations_impl(PyTypeObject *type, PyObject *iterable,
                            PyObject *robj);

static PyObject *
itertools_permutations(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {"O|O:permutations", _keywords, 0};
    PyObject *iterable;
    PyObject *robj = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &iterable, &robj)) {
        goto exit;
    }
    return_value = itertools_permutations_impl(type, iterable, robj);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_permutations___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define ITERTOOLS_PERMUTATIONS___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)itertools_permutations___sizeof__, METH_NOARGS, itertools_permutations___sizeof____doc__},

static PyObject *
itertools_permutations___sizeof___impl(permutationsobject *po);

static PyObject *
itertools_permutations___sizeof__(permutationsobject *po, PyObject *Py_UNUSED(ignored))
{
    return itertools_permutations___sizeof___impl(po);
}

PyDoc_STRVAR(itertools_permutations___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_PERMUTATIONS___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_permutations___reduce__, METH_NOARGS, itertools_permutations___reduce____doc__},

static PyObject *
itertools_permutations___reduce___impl(permutationsobject *po);

static PyObject *
itertools_permutations___reduce__(permutationsobject *po, PyObject *Py_UNUSED(ignored))
{
    return itertools_permutations___reduce___impl(po);
}

PyDoc_STRVAR(itertools_permutations___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_PERMUTATIONS___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_permutations___setstate__, METH_O, itertools_permutations___setstate____doc__},

PyDoc_STRVAR(itertools_accumulate__doc__,
"accumulate(iterable, func=None)\n"
"--\n"
"\n"
"Create an accumulate object.\n"
"\n"
"Return series of accumulated sums (or other binary function results).");

static PyObject *
itertools_accumulate_impl(PyTypeObject *type, PyObject *iterable,
                          PyObject *func);

static PyObject *
itertools_accumulate(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable", "func", NULL};
    static _PyArg_Parser _parser = {"O|O:accumulate", _keywords, 0};
    PyObject *iterable;
    PyObject *func = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &iterable, &func)) {
        goto exit;
    }
    return_value = itertools_accumulate_impl(type, iterable, func);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_accumulate___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_ACCUMULATE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_accumulate___reduce__, METH_NOARGS, itertools_accumulate___reduce____doc__},

static PyObject *
itertools_accumulate___reduce___impl(accumulateobject *lz);

static PyObject *
itertools_accumulate___reduce__(accumulateobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_accumulate___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_accumulate___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_ACCUMULATE___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_accumulate___setstate__, METH_O, itertools_accumulate___setstate____doc__},

PyDoc_STRVAR(itertools_compress__doc__,
"compress(data, selectors)\n"
"--\n"
"\n"
"Create a compress object.\n"
"\n"
"Return data elements corresponding to true selector elements.\n"
"Forms a shorter iterator from selected data elements using the\n"
"selectors to choose the data elements.");

static PyObject *
itertools_compress_impl(PyTypeObject *type, PyObject *data,
                        PyObject *selectors);

static PyObject *
itertools_compress(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "selectors", NULL};
    static _PyArg_Parser _parser = {"OO:compress", _keywords, 0};
    PyObject *data;
    PyObject *selectors;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &data, &selectors)) {
        goto exit;
    }
    return_value = itertools_compress_impl(type, data, selectors);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_compress___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_COMPRESS___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_compress___reduce__, METH_NOARGS, itertools_compress___reduce____doc__},

static PyObject *
itertools_compress___reduce___impl(compressobject *lz);

static PyObject *
itertools_compress___reduce__(compressobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_compress___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_filterfalse__doc__,
"filterfalse(function, iterable, /)\n"
"--\n"
"\n"
"Create a filterfalse object.\n"
"\n"
"Return those items of iterable for which function(item) is false.\n"
"If function is None, return the items that are false.");

static PyObject *
itertools_filterfalse_impl(PyTypeObject *type, PyObject *function,
                           PyObject *iterable);

static PyObject *
itertools_filterfalse(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *function;
    PyObject *iterable;

    if ((type == &filterfalse_type) &&
        !_PyArg_NoKeywords("filterfalse", kwargs)) {
        goto exit;
    }
    if (!PyArg_UnpackTuple(args, "filterfalse",
        2, 2,
        &function, &iterable)) {
        goto exit;
    }
    return_value = itertools_filterfalse_impl(type, function, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_filterfalse___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_FILTERFALSE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_filterfalse___reduce__, METH_NOARGS, itertools_filterfalse___reduce____doc__},

static PyObject *
itertools_filterfalse___reduce___impl(filterfalseobject *lz);

static PyObject *
itertools_filterfalse___reduce__(filterfalseobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_filterfalse___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_count__doc__,
"count(start=0, step=1)\n"
"--\n"
"\n"
"Return an iterator which returns consecutive values.\n"
"\n"
"Equivalent to:\n"
"\n"
"    def count(firstval=0, step=1):\n"
"        x = firstval\n"
"        while 1:\n"
"            yield x\n"
"            x += step");

static PyObject *
itertools_count_impl(PyTypeObject *type, PyObject *long_cnt,
                     PyObject *long_step);

static PyObject *
itertools_count(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"start", "step", NULL};
    static _PyArg_Parser _parser = {"|OO:count", _keywords, 0};
    PyObject *long_cnt = NULL;
    PyObject *long_step = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &long_cnt, &long_step)) {
        goto exit;
    }
    return_value = itertools_count_impl(type, long_cnt, long_step);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_count___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_COUNT___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_count___reduce__, METH_NOARGS, itertools_count___reduce____doc__},

static PyObject *
itertools_count___reduce___impl(countobject *lz);

static PyObject *
itertools_count___reduce__(countobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_count___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_repeat___length_hint____doc__,
"__length_hint__($self, /)\n"
"--\n"
"\n"
"Private method returning an estimate of len(list(self)).");

#define ITERTOOLS_REPEAT___LENGTH_HINT___METHODDEF    \
    {"__length_hint__", (PyCFunction)itertools_repeat___length_hint__, METH_NOARGS, itertools_repeat___length_hint____doc__},

static PyObject *
itertools_repeat___length_hint___impl(repeatobject *ro);

static PyObject *
itertools_repeat___length_hint__(repeatobject *ro, PyObject *Py_UNUSED(ignored))
{
    return itertools_repeat___length_hint___impl(ro);
}

PyDoc_STRVAR(itertools_repeat___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_REPEAT___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_repeat___reduce__, METH_NOARGS, itertools_repeat___reduce____doc__},

static PyObject *
itertools_repeat___reduce___impl(repeatobject *ro);

static PyObject *
itertools_repeat___reduce__(repeatobject *ro, PyObject *Py_UNUSED(ignored))
{
    return itertools_repeat___reduce___impl(ro);
}

PyDoc_STRVAR(itertools_zip_longest___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ITERTOOLS_ZIP_LONGEST___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)itertools_zip_longest___reduce__, METH_NOARGS, itertools_zip_longest___reduce____doc__},

static PyObject *
itertools_zip_longest___reduce___impl(ziplongestobject *lz);

static PyObject *
itertools_zip_longest___reduce__(ziplongestobject *lz, PyObject *Py_UNUSED(ignored))
{
    return itertools_zip_longest___reduce___impl(lz);
}

PyDoc_STRVAR(itertools_zip_longest___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ITERTOOLS_ZIP_LONGEST___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)itertools_zip_longest___setstate__, METH_O, itertools_zip_longest___setstate____doc__},
/*[clinic end generated code: output=c5bfdde0d01522db input=a9049054013a1b77]*/
