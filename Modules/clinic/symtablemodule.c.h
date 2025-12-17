/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_symtable_symtable__doc__,
"symtable($module, source, filename, startstr, /, *, module=None)\n"
"--\n"
"\n"
"Return symbol and scope dictionaries used internally by compiler.");

#define _SYMTABLE_SYMTABLE_METHODDEF    \
    {"symtable", _PyCFunction_CAST(_symtable_symtable), METH_FASTCALL|METH_KEYWORDS, _symtable_symtable__doc__},

static PyObject *
_symtable_symtable_impl(PyObject *module, PyObject *source,
                        PyObject *filename, const char *startstr,
                        PyObject *modname);

static PyObject *
_symtable_symtable(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(module), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "", "module", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "symtable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    PyObject *source;
    PyObject *filename = NULL;
    const char *startstr;
    PyObject *modname = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    source = args[0];
    if (!PyUnicode_FSDecoder(args[1], &filename)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("symtable", "argument 3", "str", args[2]);
        goto exit;
    }
    Py_ssize_t startstr_length;
    startstr = PyUnicode_AsUTF8AndSize(args[2], &startstr_length);
    if (startstr == NULL) {
        goto exit;
    }
    if (strlen(startstr) != (size_t)startstr_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    modname = args[3];
skip_optional_kwonly:
    return_value = _symtable_symtable_impl(module, source, filename, startstr, modname);

exit:
    /* Cleanup for filename */
    Py_XDECREF(filename);

    return return_value;
}
/*[clinic end generated code: output=0137be60c487c841 input=a9049054013a1b77]*/
