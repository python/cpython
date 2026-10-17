/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_contextlib__setup_genmgr_ctx__doc__,
"_setup_genmgr_ctx($module, /, exception)\n"
"--\n"
"\n"
"Force the provided exception to be set as the current exception\n"
"\n"
"Subsequent calls to sys.exception()/sys.exc_info() will return\n"
"the provided exception until another exception is caught in the\n"
"current thread or the execution stack returns to a frame where\n"
"another exception is being handled.");

#define _CONTEXTLIB__SETUP_GENMGR_CTX_METHODDEF    \
    {"_setup_genmgr_ctx", _PyCFunction_CAST(_contextlib__setup_genmgr_ctx), METH_FASTCALL|METH_KEYWORDS, _contextlib__setup_genmgr_ctx__doc__},

static PyObject *
_contextlib__setup_genmgr_ctx_impl(PyObject *module, PyObject *exception);

static PyObject *
_contextlib__setup_genmgr_ctx(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(exception), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"exception", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_setup_genmgr_ctx",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *exception;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    exception = args[0];
    return_value = _contextlib__setup_genmgr_ctx_impl(module, exception);

exit:
    return return_value;
}
/*[clinic end generated code: output=3c01ac3e1fdf06bf input=a9049054013a1b77]*/
