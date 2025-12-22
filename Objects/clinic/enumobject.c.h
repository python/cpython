/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(enum_new__doc__,
"enumerate(iterable, start=0)\n"
"--\n"
"\n"
"Return an enumerate object.\n"
"\n"
"  iterable\n"
"    an object supporting iteration\n"
"\n"
"The enumerate object yields pairs containing a count (from start, which\n"
"defaults to zero) and a value yielded by the iterable argument.\n"
"\n"
"enumerate is useful for obtaining an indexed list:\n"
"    (0, seq[0]), (1, seq[1]), (2, seq[2]), ...");

static PyObject *
enum_new_impl(PyTypeObject *type, PyObject *iterable, PyObject *start);

static PyObject *
enum_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(iterable), &_Py_ID(start), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "start", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enumerate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *iterable;
    PyObject *start = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    iterable = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    start = fastargs[1];
skip_optional_pos:
    return_value = enum_new_impl(type, iterable, start);

exit:
    return return_value;
}

PyDoc_STRVAR(reversed_new__doc__,
"reversed(object, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the values of the given sequence.");

static PyObject *
reversed_new_impl(PyTypeObject *type, PyObject *seq);

static PyObject *
reversed_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyReversed_Type;
    PyObject *seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("reversed", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("reversed", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    seq = PyTuple_GET_ITEM(args, 0);
    return_value = reversed_new_impl(type, seq);

exit:
    return return_value;
}
/*[clinic end generated code: output=155cc9483d5f9eab input=a9049054013a1b77]*/
