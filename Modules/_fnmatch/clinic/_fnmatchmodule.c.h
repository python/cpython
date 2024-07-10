/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_fnmatch_filter__doc__,
"filter($module, /, names, pat)\n"
"--\n"
"\n");

#define _FNMATCH_FILTER_METHODDEF    \
    {"filter", _PyCFunction_CAST(_fnmatch_filter), METH_FASTCALL|METH_KEYWORDS, _fnmatch_filter__doc__},

static PyObject *
_fnmatch_filter_impl(PyObject *module, PyObject *names, PyObject *pat);

static PyObject *
_fnmatch_filter(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(names), &_Py_ID(pat), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"names", "pat", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "filter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *names;
    PyObject *pat;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    names = args[0];
    pat = args[1];
    return_value = _fnmatch_filter_impl(module, names, pat);

exit:
    return return_value;
}

PyDoc_STRVAR(_fnmatch_fnmatch__doc__,
"fnmatch($module, /, name, pat)\n"
"--\n"
"\n");

#define _FNMATCH_FNMATCH_METHODDEF    \
    {"fnmatch", _PyCFunction_CAST(_fnmatch_fnmatch), METH_FASTCALL|METH_KEYWORDS, _fnmatch_fnmatch__doc__},

static int
_fnmatch_fnmatch_impl(PyObject *module, PyObject *name, PyObject *pat);

static PyObject *
_fnmatch_fnmatch(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(name), &_Py_ID(pat), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "pat", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fnmatch",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *name;
    PyObject *pat;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = args[0];
    pat = args[1];
    _return_value = _fnmatch_fnmatch_impl(module, name, pat);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_fnmatch_fnmatchcase__doc__,
"fnmatchcase($module, /, name, pat)\n"
"--\n"
"\n"
"Test whether `name` matches `pattern`, including case.\n"
"\n"
"This is a version of fnmatch() which doesn\'t case-normalize\n"
"its arguments.");

#define _FNMATCH_FNMATCHCASE_METHODDEF    \
    {"fnmatchcase", _PyCFunction_CAST(_fnmatch_fnmatchcase), METH_FASTCALL|METH_KEYWORDS, _fnmatch_fnmatchcase__doc__},

static int
_fnmatch_fnmatchcase_impl(PyObject *module, PyObject *name, PyObject *pat);

static PyObject *
_fnmatch_fnmatchcase(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(name), &_Py_ID(pat), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "pat", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fnmatchcase",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *name;
    PyObject *pat;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = args[0];
    pat = args[1];
    _return_value = _fnmatch_fnmatchcase_impl(module, name, pat);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_fnmatch_translate__doc__,
"translate($module, /, pat)\n"
"--\n"
"\n");

#define _FNMATCH_TRANSLATE_METHODDEF    \
    {"translate", _PyCFunction_CAST(_fnmatch_translate), METH_FASTCALL|METH_KEYWORDS, _fnmatch_translate__doc__},

static PyObject *
_fnmatch_translate_impl(PyObject *module, PyObject *pattern);

static PyObject *
_fnmatch_translate(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(pat), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pat", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "translate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *pattern;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    pattern = args[0];
    return_value = _fnmatch_translate_impl(module, pattern);

exit:
    return return_value;
}
/*[clinic end generated code: output=d9bb3df00c5c2b5e input=a9049054013a1b77]*/
