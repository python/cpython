/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(grp_getgrgid__doc__,
"getgrgid($module, /, id)\n"
"--\n"
"\n"
"Return the group database entry for the given numeric group ID.\n"
"\n"
"If id is not valid, raise KeyError.");

#define GRP_GETGRGID_METHODDEF    \
    {"getgrgid", _PyCFunction_CAST(grp_getgrgid), METH_FASTCALL|METH_KEYWORDS, grp_getgrgid__doc__},

static PyObject *
grp_getgrgid_impl(PyObject *module, PyObject *id);

static PyObject *
grp_getgrgid(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getgrgid",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *id;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    return_value = grp_getgrgid_impl(module, id);

exit:
    return return_value;
}

PyDoc_STRVAR(grp_getgrnam__doc__,
"getgrnam($module, /, name)\n"
"--\n"
"\n"
"Return the group database entry for the given group name.\n"
"\n"
"If name is not valid, raise KeyError.");

#define GRP_GETGRNAM_METHODDEF    \
    {"getgrnam", _PyCFunction_CAST(grp_getgrnam), METH_FASTCALL|METH_KEYWORDS, grp_getgrnam__doc__},

static PyObject *
grp_getgrnam_impl(PyObject *module, PyObject *name);

static PyObject *
grp_getgrnam(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getgrnam",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *name;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("getgrnam", "argument 'name'", "str", args[0]);
        goto exit;
    }
    name = args[0];
    return_value = grp_getgrnam_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(grp_getgrall__doc__,
"getgrall($module, /)\n"
"--\n"
"\n"
"Return a list of all available group entries, in arbitrary order.\n"
"\n"
"An entry whose name starts with \'+\' or \'-\' represents an instruction\n"
"to use YP/NIS and may not be accessible via getgrnam or getgrgid.");

#define GRP_GETGRALL_METHODDEF    \
    {"getgrall", (PyCFunction)grp_getgrall, METH_NOARGS, grp_getgrall__doc__},

static PyObject *
grp_getgrall_impl(PyObject *module);

static PyObject *
grp_getgrall(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return grp_getgrall_impl(module);
}
/*[clinic end generated code: output=2f7011384604d38d input=a9049054013a1b77]*/
