/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_wmi_exec_query__doc__,
"exec_query($module, /, query)\n"
"--\n"
"\n"
"Runs a WMI query against the local machine.\n"
"\n"
"This returns a single string with \'name=value\' pairs in a flat array separated\n"
"by null characters.");

#define _WMI_EXEC_QUERY_METHODDEF    \
    {"exec_query", _PyCFunction_CAST(_wmi_exec_query), METH_FASTCALL|METH_KEYWORDS, _wmi_exec_query__doc__},

static PyObject *
_wmi_exec_query_impl(PyObject *module, PyObject *query);

static PyObject *
_wmi_exec_query(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(query), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"query", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "exec_query",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *query;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("exec_query", "argument 'query'", "str", args[0]);
        goto exit;
    }
    query = args[0];
    return_value = _wmi_exec_query_impl(module, query);

exit:
    return return_value;
}
/*[clinic end generated code: output=ba04920d127f3ceb input=a9049054013a1b77]*/
