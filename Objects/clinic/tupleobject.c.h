/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(tuple_index__doc__,
"index($self, value, start=0, stop=sys.maxsize, /, *,\n"
"      key=<unrepresentable>)\n"
"--\n"
"\n"
"Return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.\n"
"A callable \'key\' can be provided to transform each item before comparison with \'value\'.");

#define TUPLE_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(tuple_index), METH_FASTCALL|METH_KEYWORDS, tuple_index__doc__},

static PyObject *
tuple_index_impl(PyTupleObject *self, PyObject *value, Py_ssize_t start,
                 Py_ssize_t stop, PyObject *key);

static PyObject *
tuple_index(PyTupleObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "index",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;
    PyObject *key = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (args == NULL) {
        goto exit;
    }
    value = args[0];
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    if (!_PyEval_SliceIndexNotNone(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional_posonly;
    }
    noptargs--;
    if (!_PyEval_SliceIndexNotNone(args[2], &stop)) {
        goto exit;
    }
skip_optional_posonly:
    if (noptargs == 0) {
        goto skip_optional_kwonly;
    }
    key = args[3];
skip_optional_kwonly:
    return_value = tuple_index_impl(self, value, start, stop, key);

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
    PyTypeObject *base_tp = &PyTuple_Type;
    PyObject *iterable = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("tuple", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("tuple", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    iterable = PyTuple_GET_ITEM(args, 0);
skip_optional:
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
/*[clinic end generated code: output=a6a9abba5d121f4c input=a9049054013a1b77]*/
