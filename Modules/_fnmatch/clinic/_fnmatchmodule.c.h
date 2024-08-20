/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

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
/*[clinic end generated code: output=eab39d3bb9f3a13d input=a9049054013a1b77]*/
