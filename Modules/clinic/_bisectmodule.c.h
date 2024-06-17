/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_bisect_bisect_right__doc__,
"bisect_right($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Return the index where to insert item x in list a, assuming a is sorted.\n"
"\n"
"The return value i is such that all e in a[:i] have e <= x, and all e in\n"
"a[i:] have e > x.  So if x already appears in the list, a.insert(i, x) will\n"
"insert just after the rightmost x already there.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_BISECT_RIGHT_METHODDEF    \
    {"bisect_right", _PyCFunction_CAST(_bisect_bisect_right), METH_FASTCALL|METH_KEYWORDS, _bisect_bisect_right__doc__},

static Py_ssize_t
_bisect_bisect_right_impl(PyObject *module, PyObject *a, PyObject *x,
                          Py_ssize_t lo, Py_ssize_t hi, PyObject *key);

static PyObject *
_bisect_bisect_right(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(x), &_Py_ID(lo), &_Py_ID(hi), &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bisect_right",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyObject *key = Py_None;
    Py_ssize_t _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    x = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        if (!_Py_convert_optional_to_ssize_t(args[3], &hi)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    key = args[4];
skip_optional_kwonly:
    _return_value = _bisect_bisect_right_impl(module, a, x, lo, hi, key);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_bisect_insort_right__doc__,
"insort_right($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Insert item x in list a, and keep it sorted assuming a is sorted.\n"
"\n"
"If x is already in a, insert it to the right of the rightmost x.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_INSORT_RIGHT_METHODDEF    \
    {"insort_right", _PyCFunction_CAST(_bisect_insort_right), METH_FASTCALL|METH_KEYWORDS, _bisect_insort_right__doc__},

static PyObject *
_bisect_insort_right_impl(PyObject *module, PyObject *a, PyObject *x,
                          Py_ssize_t lo, Py_ssize_t hi, PyObject *key);

static PyObject *
_bisect_insort_right(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(x), &_Py_ID(lo), &_Py_ID(hi), &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "insort_right",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyObject *key = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    x = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        if (!_Py_convert_optional_to_ssize_t(args[3], &hi)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    key = args[4];
skip_optional_kwonly:
    return_value = _bisect_insort_right_impl(module, a, x, lo, hi, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_bisect_bisect_left__doc__,
"bisect_left($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Return the index where to insert item x in list a, assuming a is sorted.\n"
"\n"
"The return value i is such that all e in a[:i] have e < x, and all e in\n"
"a[i:] have e >= x.  So if x already appears in the list, a.insert(i, x) will\n"
"insert just before the leftmost x already there.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_BISECT_LEFT_METHODDEF    \
    {"bisect_left", _PyCFunction_CAST(_bisect_bisect_left), METH_FASTCALL|METH_KEYWORDS, _bisect_bisect_left__doc__},

static Py_ssize_t
_bisect_bisect_left_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi, PyObject *key);

static PyObject *
_bisect_bisect_left(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(x), &_Py_ID(lo), &_Py_ID(hi), &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bisect_left",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyObject *key = Py_None;
    Py_ssize_t _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    x = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        if (!_Py_convert_optional_to_ssize_t(args[3], &hi)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    key = args[4];
skip_optional_kwonly:
    _return_value = _bisect_bisect_left_impl(module, a, x, lo, hi, key);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_bisect_insort_left__doc__,
"insort_left($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Insert item x in list a, and keep it sorted assuming a is sorted.\n"
"\n"
"If x is already in a, insert it to the left of the leftmost x.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_INSORT_LEFT_METHODDEF    \
    {"insort_left", _PyCFunction_CAST(_bisect_insort_left), METH_FASTCALL|METH_KEYWORDS, _bisect_insort_left__doc__},

static PyObject *
_bisect_insort_left_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi, PyObject *key);

static PyObject *
_bisect_insort_left(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(x), &_Py_ID(lo), &_Py_ID(hi), &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "insort_left",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyObject *key = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    x = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        if (!_Py_convert_optional_to_ssize_t(args[3], &hi)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    key = args[4];
skip_optional_kwonly:
    return_value = _bisect_insort_left_impl(module, a, x, lo, hi, key);

exit:
    return return_value;
}
/*[clinic end generated code: output=4af5bd405149bf5f input=a9049054013a1b77]*/
