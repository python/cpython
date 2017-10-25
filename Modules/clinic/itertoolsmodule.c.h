/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(groupby_new__doc__,
"groupby(iterable, key=None)\n"
"--\n"
"\n"
"Create a groupby object.\n"
"\n"
"  iterable\n"
"    Elements to divide into groups according to the key function.\n"
"  key\n"
"    Function computing a key value for each element.\n"
"    If None, the elements themselves are used for grouping.\n"
"\n"
"This makes an iterator of (key, sub-iterator) pairs. In each such pair, the\n"
"sub-iterator is a group of consecutive elements from the input iterable which\n"
"all have the same key. The common key for the group is the first item in the\n"
"pair.\n"
"\n"
"Example:\n"
">>> # group numbers by their absolute value\n"
">>> elements = [1, -1, 2, 1]\n"
">>> for (key, group) in itertools.groupby(elements, key=abs):\n"
"...     print(\'key={} group={}\'.format(key, list(group)))\n"
"...\n"
"key=1 group=[1, -1]\n"
"key=2 group=[2]\n"
"key=1 group=[1]");

static PyObject *
groupby_new_impl(PyTypeObject *type, PyObject *iterable, PyObject *key);

static PyObject *
groupby_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = groupby_new_impl(type, iterable, key);

exit:
    return return_value;
}

PyDoc_STRVAR(groupby_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define GROUPBY_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)groupby_reduce, METH_NOARGS, groupby_reduce__doc__},

static PyObject *
groupby_reduce_impl(groupbyobject *lz);

static PyObject *
groupby_reduce(groupbyobject *lz, PyObject *Py_UNUSED(ignored))
{
    return groupby_reduce_impl(lz);
}

PyDoc_STRVAR(groupby_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define GROUPBY_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)groupby_setstate, METH_O, groupby_setstate__doc__},

static PyObject *
_grouper_new_impl(PyTypeObject *type, PyObject *parent, PyObject *tgtkey);

static PyObject *
_grouper_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = _grouper_new_impl(type, parent, tgtkey);

exit:
    return return_value;
}

PyDoc_STRVAR(_grouper_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define _GROUPER_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)_grouper_reduce, METH_NOARGS, _grouper_reduce__doc__},

static PyObject *
_grouper_reduce_impl(_grouperobject *lz);

static PyObject *
_grouper_reduce(_grouperobject *lz, PyObject *Py_UNUSED(ignored))
{
    return _grouper_reduce_impl(lz);
}

PyDoc_STRVAR(teedataobject_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define TEEDATAOBJECT_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)teedataobject_reduce, METH_NOARGS, teedataobject_reduce__doc__},

static PyObject *
teedataobject_reduce_impl(teedataobject *tdo);

static PyObject *
teedataobject_reduce(teedataobject *tdo, PyObject *Py_UNUSED(ignored))
{
    return teedataobject_reduce_impl(tdo);
}

PyDoc_STRVAR(teedataobject_new__doc__,
"teedataobject(iterable, values, next, /)\n"
"--\n"
"\n"
"Data container common to multiple tee objects.");

static PyObject *
teedataobject_new_impl(PyTypeObject *type, PyObject *iterable,
                       PyObject *values, PyObject *next);

static PyObject *
teedataobject_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = teedataobject_new_impl(type, iterable, values, next);

exit:
    return return_value;
}

PyDoc_STRVAR(tee_copy__doc__,
"__copy__($self, /)\n"
"--\n"
"\n"
"Returns an independent iterator.");

#define TEE_COPY_METHODDEF    \
    {"__copy__", (PyCFunction)tee_copy, METH_NOARGS, tee_copy__doc__},

static PyObject *
tee_copy_impl(teeobject *to);

static PyObject *
tee_copy(teeobject *to, PyObject *Py_UNUSED(ignored))
{
    return tee_copy_impl(to);
}

PyDoc_STRVAR(tee_new__doc__,
"_tee(iterable, /)\n"
"--\n"
"\n"
"Create a _tee object.\n"
"\n"
"An iterator wrapped to make it copyable.");

static PyObject *
tee_new_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
tee_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = tee_new_impl(type, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(tee_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define TEE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)tee_reduce, METH_NOARGS, tee_reduce__doc__},

static PyObject *
tee_reduce_impl(teeobject *to);

static PyObject *
tee_reduce(teeobject *to, PyObject *Py_UNUSED(ignored))
{
    return tee_reduce_impl(to);
}

PyDoc_STRVAR(tee_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define TEE_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)tee_setstate, METH_O, tee_setstate__doc__},

PyDoc_STRVAR(tee__doc__,
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

#define TEE_METHODDEF    \
    {"tee", (PyCFunction)tee, METH_FASTCALL, tee__doc__},

static PyObject *
tee_impl(PyObject *module, PyObject *iterable, Py_ssize_t n);

static PyObject *
tee(PyObject *module, PyObject **args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    Py_ssize_t n = 2;

    if (!_PyArg_ParseStack(args, nargs, "O|n:tee",
        &iterable, &n)) {
        goto exit;
    }
    return_value = tee_impl(module, iterable, n);

exit:
    return return_value;
}

PyDoc_STRVAR(cycle_new__doc__,
"cycle(iterable, /)\n"
"--\n"
"\n"
"Create a cycle object.\n"
"\n"
"This object will return elements from the iterable until it is exhausted.\n"
"Then it will repeat the sequence indefinitely.");

static PyObject *
cycle_new_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
cycle_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = cycle_new_impl(type, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(cycle_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define CYCLE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)cycle_reduce, METH_NOARGS, cycle_reduce__doc__},

static PyObject *
cycle_reduce_impl(cycleobject *lz);

static PyObject *
cycle_reduce(cycleobject *lz, PyObject *Py_UNUSED(ignored))
{
    return cycle_reduce_impl(lz);
}

PyDoc_STRVAR(cycle_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define CYCLE_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)cycle_setstate, METH_O, cycle_setstate__doc__},

PyDoc_STRVAR(dropwhile_new__doc__,
"dropwhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Create a dropwhile object.\n"
"\n"
"Drops items from the iterable while predicate(item) is true.\n"
"Afterwards, returns every element until the iterable is exhausted.");

static PyObject *
dropwhile_new_impl(PyTypeObject *type, PyObject *predicate,
                   PyObject *iterable);

static PyObject *
dropwhile_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = dropwhile_new_impl(type, predicate, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(dropwhile_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define DROPWHILE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)dropwhile_reduce, METH_NOARGS, dropwhile_reduce__doc__},

static PyObject *
dropwhile_reduce_impl(dropwhileobject *lz);

static PyObject *
dropwhile_reduce(dropwhileobject *lz, PyObject *Py_UNUSED(ignored))
{
    return dropwhile_reduce_impl(lz);
}

PyDoc_STRVAR(dropwhile_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define DROPWHILE_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)dropwhile_setstate, METH_O, dropwhile_setstate__doc__},

PyDoc_STRVAR(takewhile_new__doc__,
"takewhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Create a takewhile object.\n"
"\n"
"Return successive entries from an iterable as long as the\n"
"predicate evaluates to true for each entry.");

static PyObject *
takewhile_new_impl(PyTypeObject *type, PyObject *predicate,
                   PyObject *iterable);

static PyObject *
takewhile_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = takewhile_new_impl(type, predicate, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(takewhile_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define TAKEWHILE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)takewhile_reduce, METH_NOARGS, takewhile_reduce__doc__},

static PyObject *
takewhile_reduce_impl(takewhileobject *lz);

static PyObject *
takewhile_reduce(takewhileobject *lz, PyObject *Py_UNUSED(ignored))
{
    return takewhile_reduce_impl(lz);
}

PyDoc_STRVAR(takewhile_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define TAKEWHILE_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)takewhile_setstate, METH_O, takewhile_setstate__doc__},

PyDoc_STRVAR(islice_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ISLICE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)islice_reduce, METH_NOARGS, islice_reduce__doc__},

static PyObject *
islice_reduce_impl(isliceobject *lz);

static PyObject *
islice_reduce(isliceobject *lz, PyObject *Py_UNUSED(ignored))
{
    return islice_reduce_impl(lz);
}

PyDoc_STRVAR(islice_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ISLICE_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)islice_setstate, METH_O, islice_setstate__doc__},

PyDoc_STRVAR(starmap_new__doc__,
"starmap(function, iterable)\n"
"--\n"
"\n"
"Create a starmap object.\n"
"\n"
"Return an iterator whose values are returned from the function evaluated\n"
"with a argument tuple taken from the given iterable.");

static PyObject *
starmap_new_impl(PyTypeObject *type, PyObject *function, PyObject *iterable);

static PyObject *
starmap_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"function", "iterable", NULL};
    static _PyArg_Parser _parser = {"OO:starmap", _keywords, 0};
    PyObject *function;
    PyObject *iterable;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &function, &iterable)) {
        goto exit;
    }
    return_value = starmap_new_impl(type, function, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(starmap_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define STARMAP_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)starmap_reduce, METH_NOARGS, starmap_reduce__doc__},

static PyObject *
starmap_reduce_impl(starmapobject *lz);

static PyObject *
starmap_reduce(starmapobject *lz, PyObject *Py_UNUSED(ignored))
{
    return starmap_reduce_impl(lz);
}

PyDoc_STRVAR(chain_new_from_iterable__doc__,
"from_iterable($type, iterable, /)\n"
"--\n"
"\n"
"Create a chain object.\n"
"\n"
"Alternate chain() contructor taking a single iterable argument\n"
"that evaluates lazily.");

#define CHAIN_NEW_FROM_ITERABLE_METHODDEF    \
    {"from_iterable", (PyCFunction)chain_new_from_iterable, METH_O|METH_CLASS, chain_new_from_iterable__doc__},

PyDoc_STRVAR(chain_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define CHAIN_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)chain_reduce, METH_NOARGS, chain_reduce__doc__},

static PyObject *
chain_reduce_impl(chainobject *lz);

static PyObject *
chain_reduce(chainobject *lz, PyObject *Py_UNUSED(ignored))
{
    return chain_reduce_impl(lz);
}

PyDoc_STRVAR(chain_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define CHAIN_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)chain_setstate, METH_O, chain_setstate__doc__},

PyDoc_STRVAR(product_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define PRODUCT_SIZEOF_METHODDEF    \
    {"__sizeof__", (PyCFunction)product_sizeof, METH_NOARGS, product_sizeof__doc__},

static PyObject *
product_sizeof_impl(productobject *lz);

static PyObject *
product_sizeof(productobject *lz, PyObject *Py_UNUSED(ignored))
{
    return product_sizeof_impl(lz);
}

PyDoc_STRVAR(product_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define PRODUCT_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)product_reduce, METH_NOARGS, product_reduce__doc__},

static PyObject *
product_reduce_impl(productobject *lz);

static PyObject *
product_reduce(productobject *lz, PyObject *Py_UNUSED(ignored))
{
    return product_reduce_impl(lz);
}

PyDoc_STRVAR(product_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define PRODUCT_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)product_setstate, METH_O, product_setstate__doc__},

PyDoc_STRVAR(combinations_new__doc__,
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
combinations_new_impl(PyTypeObject *type, PyObject *iterable, Py_ssize_t r);

static PyObject *
combinations_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = combinations_new_impl(type, iterable, r);

exit:
    return return_value;
}

PyDoc_STRVAR(combinations_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define COMBINATIONS_SIZEOF_METHODDEF    \
    {"__sizeof__", (PyCFunction)combinations_sizeof, METH_NOARGS, combinations_sizeof__doc__},

static PyObject *
combinations_sizeof_impl(combinationsobject *co);

static PyObject *
combinations_sizeof(combinationsobject *co, PyObject *Py_UNUSED(ignored))
{
    return combinations_sizeof_impl(co);
}

PyDoc_STRVAR(combinations_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define COMBINATIONS_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)combinations_reduce, METH_NOARGS, combinations_reduce__doc__},

static PyObject *
combinations_reduce_impl(combinationsobject *lz);

static PyObject *
combinations_reduce(combinationsobject *lz, PyObject *Py_UNUSED(ignored))
{
    return combinations_reduce_impl(lz);
}

PyDoc_STRVAR(combinations_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define COMBINATIONS_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)combinations_setstate, METH_O, combinations_setstate__doc__},

PyDoc_STRVAR(cwr_new__doc__,
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
cwr_new_impl(PyTypeObject *type, PyObject *iterable, Py_ssize_t r);

static PyObject *
cwr_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = cwr_new_impl(type, iterable, r);

exit:
    return return_value;
}

PyDoc_STRVAR(cwr_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define CWR_SIZEOF_METHODDEF    \
    {"__sizeof__", (PyCFunction)cwr_sizeof, METH_NOARGS, cwr_sizeof__doc__},

static PyObject *
cwr_sizeof_impl(cwrobject *co);

static PyObject *
cwr_sizeof(cwrobject *co, PyObject *Py_UNUSED(ignored))
{
    return cwr_sizeof_impl(co);
}

PyDoc_STRVAR(cwr_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define CWR_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)cwr_reduce, METH_NOARGS, cwr_reduce__doc__},

static PyObject *
cwr_reduce_impl(cwrobject *lz);

static PyObject *
cwr_reduce(cwrobject *lz, PyObject *Py_UNUSED(ignored))
{
    return cwr_reduce_impl(lz);
}

PyDoc_STRVAR(cwr_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define CWR_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)cwr_setstate, METH_O, cwr_setstate__doc__},

PyDoc_STRVAR(permutations_new__doc__,
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
permutations_new_impl(PyTypeObject *type, PyObject *iterable, PyObject *robj);

static PyObject *
permutations_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = permutations_new_impl(type, iterable, robj);

exit:
    return return_value;
}

PyDoc_STRVAR(permutations_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define PERMUTATIONS_SIZEOF_METHODDEF    \
    {"__sizeof__", (PyCFunction)permutations_sizeof, METH_NOARGS, permutations_sizeof__doc__},

static PyObject *
permutations_sizeof_impl(permutationsobject *po);

static PyObject *
permutations_sizeof(permutationsobject *po, PyObject *Py_UNUSED(ignored))
{
    return permutations_sizeof_impl(po);
}

PyDoc_STRVAR(permutations_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define PERMUTATIONS_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)permutations_reduce, METH_NOARGS, permutations_reduce__doc__},

static PyObject *
permutations_reduce_impl(permutationsobject *po);

static PyObject *
permutations_reduce(permutationsobject *po, PyObject *Py_UNUSED(ignored))
{
    return permutations_reduce_impl(po);
}

PyDoc_STRVAR(permutations_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define PERMUTATIONS_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)permutations_setstate, METH_O, permutations_setstate__doc__},

PyDoc_STRVAR(accumulate_new__doc__,
"accumulate(iterable, func=None)\n"
"--\n"
"\n"
"Create an accumulate object.\n"
"\n"
"Return series of accumulated sums (or other binary function results).");

static PyObject *
accumulate_new_impl(PyTypeObject *type, PyObject *iterable, PyObject *func);

static PyObject *
accumulate_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = accumulate_new_impl(type, iterable, func);

exit:
    return return_value;
}

PyDoc_STRVAR(accumulate_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ACCUMULATE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)accumulate_reduce, METH_NOARGS, accumulate_reduce__doc__},

static PyObject *
accumulate_reduce_impl(accumulateobject *lz);

static PyObject *
accumulate_reduce(accumulateobject *lz, PyObject *Py_UNUSED(ignored))
{
    return accumulate_reduce_impl(lz);
}

PyDoc_STRVAR(accumulate_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ACCUMULATE_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)accumulate_setstate, METH_O, accumulate_setstate__doc__},

PyDoc_STRVAR(compress_new__doc__,
"compress(data, selectors)\n"
"--\n"
"\n"
"Create a compress object.\n"
"\n"
"Return data elements corresponding to true selector elements.\n"
"Forms a shorter iterator from selected data elements using the\n"
"selectors to choose the data elements.");

static PyObject *
compress_new_impl(PyTypeObject *type, PyObject *data, PyObject *selectors);

static PyObject *
compress_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = compress_new_impl(type, data, selectors);

exit:
    return return_value;
}

PyDoc_STRVAR(compress_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define COMPRESS_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)compress_reduce, METH_NOARGS, compress_reduce__doc__},

static PyObject *
compress_reduce_impl(compressobject *lz);

static PyObject *
compress_reduce(compressobject *lz, PyObject *Py_UNUSED(ignored))
{
    return compress_reduce_impl(lz);
}

PyDoc_STRVAR(filterfalse_new__doc__,
"filterfalse(function, iterable, /)\n"
"--\n"
"\n"
"Create a filterfalse object.\n"
"\n"
"Return those items of iterable for which function(item) is false.\n"
"If function is None, return the items that are false.");

static PyObject *
filterfalse_new_impl(PyTypeObject *type, PyObject *function,
                     PyObject *iterable);

static PyObject *
filterfalse_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
    return_value = filterfalse_new_impl(type, function, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(filterfalse_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define FILTERFALSE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)filterfalse_reduce, METH_NOARGS, filterfalse_reduce__doc__},

static PyObject *
filterfalse_reduce_impl(filterfalseobject *lz);

static PyObject *
filterfalse_reduce(filterfalseobject *lz, PyObject *Py_UNUSED(ignored))
{
    return filterfalse_reduce_impl(lz);
}

PyDoc_STRVAR(count_new__doc__,
"count(start=None, step=None)\n"
"--\n"
"\n"
"Create a count object.\n"
"\n"
"Return a count object whose .__next__() method returns consecutive values.\n"
"Equivalent to:\n"
"\n"
"    def count(firstval=0, step=1):\n"
"        x = firstval\n"
"        while 1:\n"
"            yield x\n"
"            x += step");

static PyObject *
count_new_impl(PyTypeObject *type, PyObject *start, PyObject *step);

static PyObject *
count_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"start", "step", NULL};
    static _PyArg_Parser _parser = {"|OO:count", _keywords, 0};
    PyObject *start = NULL;
    PyObject *step = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &start, &step)) {
        goto exit;
    }
    return_value = count_new_impl(type, start, step);

exit:
    return return_value;
}

PyDoc_STRVAR(count_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define COUNT_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)count_reduce, METH_NOARGS, count_reduce__doc__},

static PyObject *
count_reduce_impl(countobject *lz);

static PyObject *
count_reduce(countobject *lz, PyObject *Py_UNUSED(ignored))
{
    return count_reduce_impl(lz);
}

PyDoc_STRVAR(repeat_len__doc__,
"__length_hint__($self, /)\n"
"--\n"
"\n"
"Private method returning an estimate of len(list(it)).");

#define REPEAT_LEN_METHODDEF    \
    {"__length_hint__", (PyCFunction)repeat_len, METH_NOARGS, repeat_len__doc__},

static PyObject *
repeat_len_impl(repeatobject *ro);

static PyObject *
repeat_len(repeatobject *ro, PyObject *Py_UNUSED(ignored))
{
    return repeat_len_impl(ro);
}

PyDoc_STRVAR(repeat_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define REPEAT_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)repeat_reduce, METH_NOARGS, repeat_reduce__doc__},

static PyObject *
repeat_reduce_impl(repeatobject *ro);

static PyObject *
repeat_reduce(repeatobject *ro, PyObject *Py_UNUSED(ignored))
{
    return repeat_reduce_impl(ro);
}

PyDoc_STRVAR(zip_longest_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ZIP_LONGEST_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)zip_longest_reduce, METH_NOARGS, zip_longest_reduce__doc__},

static PyObject *
zip_longest_reduce_impl(ziplongestobject *lz);

static PyObject *
zip_longest_reduce(ziplongestobject *lz, PyObject *Py_UNUSED(ignored))
{
    return zip_longest_reduce_impl(lz);
}

PyDoc_STRVAR(zip_longest_setstate__doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ZIP_LONGEST_SETSTATE_METHODDEF    \
    {"__setstate__", (PyCFunction)zip_longest_setstate, METH_O, zip_longest_setstate__doc__},
/*[clinic end generated code: output=a35685ba24a0567d input=a9049054013a1b77]*/
