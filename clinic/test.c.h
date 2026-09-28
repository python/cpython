/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(foo_bar__doc__,
"bar($module, /, *args, a)\n"
"--\n"
"\n");

#define FOO_BAR_METHODDEF    \
    {"bar", _PyCFunction_CAST(foo_bar), METH_FASTCALL|METH_KEYWORDS, foo_bar__doc__},

static PyObject *
foo_bar_impl(PyObject *module, PyObject *args, int a);

static PyObject *
foo_bar(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bar",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    PyObject *__clinic_args = NULL;
    int a;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 1, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyLong_AsInt(fastargs[0]);
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    __clinic_args = PyTuple_FromArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = foo_bar_impl(module, __clinic_args, a);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}
/*[clinic end generated code: output=aa3dc5fffe57e996 input=a9049054013a1b77]*/
