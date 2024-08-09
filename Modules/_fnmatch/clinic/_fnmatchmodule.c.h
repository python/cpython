/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(fnmatch_filter__doc__,
"filter($module, /, names, pat)\n"
"--\n"
"\n"
"Construct a list from the names in *names* matching *pat*.");

#define FNMATCH_FILTER_METHODDEF    \
    {"filter", _PyCFunction_CAST(fnmatch_filter), METH_FASTCALL|METH_KEYWORDS, fnmatch_filter__doc__},

static PyObject *
fnmatch_filter_impl(PyObject *module, PyObject *names, PyObject *pattern);

static PyObject *
fnmatch_filter(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    PyObject *pattern;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    names = args[0];
    pattern = args[1];
    return_value = fnmatch_filter_impl(module, names, pattern);

exit:
    return return_value;
}

PyDoc_STRVAR(fnmatch_fnmatch__doc__,
"fnmatch($module, /, name, pat)\n"
"--\n"
"\n"
"Test whether *name* matches *pat*.\n"
"\n"
"Patterns are Unix shell style:\n"
"\n"
"*       matches everything\n"
"?       matches any single character\n"
"[seq]   matches any character in seq\n"
"[!seq]  matches any char not in seq\n"
"\n"
"An initial period in *name* is not special.\n"
"Both *name* and *pat* are first case-normalized\n"
"if the operating system requires it.\n"
"\n"
"If you don\'t want this, use fnmatchcase(name, pat).");

#define FNMATCH_FNMATCH_METHODDEF    \
    {"fnmatch", _PyCFunction_CAST(fnmatch_fnmatch), METH_FASTCALL|METH_KEYWORDS, fnmatch_fnmatch__doc__},

static int
fnmatch_fnmatch_impl(PyObject *module, PyObject *name, PyObject *pattern);

static PyObject *
fnmatch_fnmatch(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    PyObject *pattern;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = args[0];
    pattern = args[1];
    _return_value = fnmatch_fnmatch_impl(module, name, pattern);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(fnmatch_fnmatchcase__doc__,
"fnmatchcase($module, /, name, pat)\n"
"--\n"
"\n"
"Test whether *name* matches *pat*, including case.\n"
"\n"
"This is a version of fnmatch() which doesn\'t case-normalize\n"
"its arguments.");

#define FNMATCH_FNMATCHCASE_METHODDEF    \
    {"fnmatchcase", _PyCFunction_CAST(fnmatch_fnmatchcase), METH_FASTCALL|METH_KEYWORDS, fnmatch_fnmatchcase__doc__},

static int
fnmatch_fnmatchcase_impl(PyObject *module, PyObject *name, PyObject *pattern);

static PyObject *
fnmatch_fnmatchcase(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    PyObject *pattern;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = args[0];
    pattern = args[1];
    _return_value = fnmatch_fnmatchcase_impl(module, name, pattern);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(fnmatch_translate__doc__,
"translate($module, /, pat)\n"
"--\n"
"\n"
"Translate a shell pattern *pat* to a regular expression.\n"
"\n"
"There is no way to quote meta-characters.");

#define FNMATCH_TRANSLATE_METHODDEF    \
    {"translate", _PyCFunction_CAST(fnmatch_translate), METH_FASTCALL|METH_KEYWORDS, fnmatch_translate__doc__},

static PyObject *
fnmatch_translate_impl(PyObject *module, PyObject *pattern);

static PyObject *
fnmatch_translate(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = fnmatch_translate_impl(module, pattern);

exit:
    return return_value;
}
/*[clinic end generated code: output=50f858ef4bfb569a input=a9049054013a1b77]*/
