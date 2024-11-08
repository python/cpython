/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(batched_new__doc__,
"batched(iterable, n, *, strict=False)\n"
"--\n"
"\n"
"Batch data into tuples of length n. The last batch may be shorter than n.\n"
"\n"
"Loops over the input iterable and accumulates data into tuples\n"
"up to size n.  The input is consumed lazily, just enough to\n"
"fill a batch.  The result is yielded as soon as a batch is full\n"
"or when the input iterable is exhausted.\n"
"\n"
"    >>> for batch in batched(\'ABCDEFG\', 3):\n"
"    ...     print(batch)\n"
"    ...\n"
"    (\'A\', \'B\', \'C\')\n"
"    (\'D\', \'E\', \'F\')\n"
"    (\'G\',)\n"
"\n"
"If \"strict\" is True, raises a ValueError if the final batch is shorter\n"
"than n.");

static PyObject *
batched_new_impl(PyTypeObject *type, PyObject *iterable, Py_ssize_t n,
                 int strict);

static PyObject *
batched_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), _Py_LATIN1_CHR('n'), &_Py_ID(strict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "n", "strict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "batched",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 2;
    PyObject *iterable;
    Py_ssize_t n;
    int strict = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    iterable = fastargs[0];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    strict = PyObject_IsTrue(fastargs[2]);
    if (strict < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = batched_new_impl(type, iterable, n, strict);

exit:
    return return_value;
}

PyDoc_STRVAR(pairwise_new__doc__,
"pairwise(iterable, /)\n"
"--\n"
"\n"
"Return an iterator of overlapping pairs taken from the input iterator.\n"
"\n"
"    s -> (s0,s1), (s1,s2), (s2, s3), ...");

static PyObject *
pairwise_new_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
pairwise_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->pairwise_type;
    PyObject *iterable;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("pairwise", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("pairwise", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    iterable = PyTuple_GET_ITEM(args, 0);
    return_value = pairwise_new_impl(type, iterable);

exit:
    return return_value;
}

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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "groupby",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *it;
    PyObject *keyfunc = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    it = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    keyfunc = fastargs[1];
skip_optional_pos:
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
    PyTypeObject *base_tp = clinic_state()->_grouper_type;
    PyObject *parent;
    PyObject *tgtkey;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("_grouper", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_grouper", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    if (!PyObject_TypeCheck(PyTuple_GET_ITEM(args, 0), clinic_state_by_cls()->groupby_type)) {
        _PyArg_BadArgument("_grouper", "argument 1", (clinic_state_by_cls()->groupby_type)->tp_name, PyTuple_GET_ITEM(args, 0));
        goto exit;
    }
    parent = PyTuple_GET_ITEM(args, 0);
    tgtkey = PyTuple_GET_ITEM(args, 1);
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
    PyTypeObject *base_tp = clinic_state()->teedataobject_type;
    PyObject *it;
    PyObject *values;
    PyObject *next;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("teedataobject", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("teedataobject", PyTuple_GET_SIZE(args), 3, 3)) {
        goto exit;
    }
    it = PyTuple_GET_ITEM(args, 0);
    if (!PyList_Check(PyTuple_GET_ITEM(args, 1))) {
        _PyArg_BadArgument("teedataobject", "argument 2", "list", PyTuple_GET_ITEM(args, 1));
        goto exit;
    }
    values = PyTuple_GET_ITEM(args, 1);
    next = PyTuple_GET_ITEM(args, 2);
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
    PyTypeObject *base_tp = clinic_state()->tee_type;
    PyObject *iterable;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("_tee", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_tee", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    iterable = PyTuple_GET_ITEM(args, 0);
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
    {"tee", _PyCFunction_CAST(itertools_tee), METH_FASTCALL, itertools_tee__doc__},

static PyObject *
itertools_tee_impl(PyObject *module, PyObject *iterable, Py_ssize_t n);

static PyObject *
itertools_tee(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *iterable;
    Py_ssize_t n = 2;

    if (!_PyArg_CheckPositional("tee", nargs, 1, 2)) {
        goto exit;
    }
    iterable = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
    }
skip_optional:
    return_value = itertools_tee_impl(module, iterable, n);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_cycle__doc__,
"cycle(iterable, /)\n"
"--\n"
"\n"
"Return elements from the iterable until it is exhausted. Then repeat the sequence indefinitely.");

static PyObject *
itertools_cycle_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
itertools_cycle(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->cycle_type;
    PyObject *iterable;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("cycle", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("cycle", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    iterable = PyTuple_GET_ITEM(args, 0);
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
    PyTypeObject *base_tp = clinic_state()->dropwhile_type;
    PyObject *func;
    PyObject *seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("dropwhile", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("dropwhile", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    func = PyTuple_GET_ITEM(args, 0);
    seq = PyTuple_GET_ITEM(args, 1);
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
    PyTypeObject *base_tp = clinic_state()->takewhile_type;
    PyObject *func;
    PyObject *seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("takewhile", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("takewhile", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    func = PyTuple_GET_ITEM(args, 0);
    seq = PyTuple_GET_ITEM(args, 1);
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
    PyTypeObject *base_tp = clinic_state()->starmap_type;
    PyObject *func;
    PyObject *seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("starmap", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("starmap", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    func = PyTuple_GET_ITEM(args, 0);
    seq = PyTuple_GET_ITEM(args, 1);
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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), _Py_LATIN1_CHR('r'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "combinations",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *iterable;
    Py_ssize_t r;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    iterable = fastargs[0];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        r = ival;
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
"combinations_with_replacement(\'ABC\', 2) --> (\'A\',\'A\'), (\'A\',\'B\'), (\'A\',\'C\'), (\'B\',\'B\'), (\'B\',\'C\'), (\'C\',\'C\')");

static PyObject *
itertools_combinations_with_replacement_impl(PyTypeObject *type,
                                             PyObject *iterable,
                                             Py_ssize_t r);

static PyObject *
itertools_combinations_with_replacement(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), _Py_LATIN1_CHR('r'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "combinations_with_replacement",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *iterable;
    Py_ssize_t r;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    iterable = fastargs[0];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        r = ival;
    }
    return_value = itertools_combinations_with_replacement_impl(type, iterable, r);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_permutations__doc__,
"permutations(iterable, r=None)\n"
"--\n"
"\n"
"Return successive r-length permutations of elements in the iterable.\n"
"\n"
"permutations(range(3), 2) --> (0,1), (0,2), (1,0), (1,2), (2,0), (2,1)");

static PyObject *
itertools_permutations_impl(PyTypeObject *type, PyObject *iterable,
                            PyObject *robj);

static PyObject *
itertools_permutations(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), _Py_LATIN1_CHR('r'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "permutations",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *iterable;
    PyObject *robj = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    iterable = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    robj = fastargs[1];
skip_optional_pos:
    return_value = itertools_permutations_impl(type, iterable, robj);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_accumulate__doc__,
"accumulate(iterable, func=None, *, initial=None)\n"
"--\n"
"\n"
"Return series of accumulated sums (or other binary function results).");

static PyObject *
itertools_accumulate_impl(PyTypeObject *type, PyObject *iterable,
                          PyObject *binop, PyObject *initial);

static PyObject *
itertools_accumulate(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), &_Py_ID(func), &_Py_ID(initial), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "func", "initial", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "accumulate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *iterable;
    PyObject *binop = Py_None;
    PyObject *initial = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    iterable = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[1]) {
        binop = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    initial = fastargs[2];
skip_optional_kwonly:
    return_value = itertools_accumulate_impl(type, iterable, binop, initial);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_compress__doc__,
"compress(data, selectors)\n"
"--\n"
"\n"
"Return data elements corresponding to true selector elements.\n"
"\n"
"Forms a shorter iterator from selected data elements using the selectors to\n"
"choose the data elements.");

static PyObject *
itertools_compress_impl(PyTypeObject *type, PyObject *seq1, PyObject *seq2);

static PyObject *
itertools_compress(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(data), &_Py_ID(selectors), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "selectors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *seq1;
    PyObject *seq2;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    seq1 = fastargs[0];
    seq2 = fastargs[1];
    return_value = itertools_compress_impl(type, seq1, seq2);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_filterfalse__doc__,
"filterfalse(function, iterable, /)\n"
"--\n"
"\n"
"Return those items of iterable for which function(item) is false.\n"
"\n"
"If function is None, return the items that are false.");

static PyObject *
itertools_filterfalse_impl(PyTypeObject *type, PyObject *func, PyObject *seq);

static PyObject *
itertools_filterfalse(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->filterfalse_type;
    PyObject *func;
    PyObject *seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("filterfalse", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("filterfalse", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    func = PyTuple_GET_ITEM(args, 0);
    seq = PyTuple_GET_ITEM(args, 1);
    return_value = itertools_filterfalse_impl(type, func, seq);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_count__doc__,
"count(start=0, step=1)\n"
"--\n"
"\n"
"Return a count object whose .__next__() method returns consecutive values.\n"
"\n"
"Equivalent to:\n"
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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(start), &_Py_ID(step), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"start", "step", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "count",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *long_cnt = NULL;
    PyObject *long_step = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        long_cnt = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    long_step = fastargs[1];
skip_optional_pos:
    return_value = itertools_count_impl(type, long_cnt, long_step);

exit:
    return return_value;
}
/*[clinic end generated code: output=7b13be3075f2d6d3 input=a9049054013a1b77]*/
