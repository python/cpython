/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_testinternalcapi_optimize_cfg__doc__,
"optimize_cfg($module, /, instructions, consts)\n"
"--\n"
"\n"
"Apply compiler optimizations to an instruction list.");

#define _TESTINTERNALCAPI_OPTIMIZE_CFG_METHODDEF    \
    {"optimize_cfg", _PyCFunction_CAST(_testinternalcapi_optimize_cfg), METH_FASTCALL|METH_KEYWORDS, _testinternalcapi_optimize_cfg__doc__},

static PyObject *
_testinternalcapi_optimize_cfg_impl(PyObject *module, PyObject *instructions,
                                    PyObject *consts);

static PyObject *
_testinternalcapi_optimize_cfg(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #define NUM_KEYWORDS 2
    #if NUM_KEYWORDS == 0

    #  if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #    define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #  else
    #    define KWTUPLE NULL
    #  endif

    #else  // NUM_KEYWORDS != 0
    #  if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(instructions), &_Py_ID(consts), },
    };
    #  define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #  else  // !Py_BUILD_CORE
    #    define KWTUPLE NULL
    #  endif  // !Py_BUILD_CORE
    #endif  // NUM_KEYWORDS != 0
    #undef NUM_KEYWORDS

    static const char * const _keywords[] = {"instructions", "consts", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "optimize_cfg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *instructions;
    PyObject *consts;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    instructions = args[0];
    consts = args[1];
    return_value = _testinternalcapi_optimize_cfg_impl(module, instructions, consts);

exit:
    return return_value;
}
/*[clinic end generated code: output=1fc905a6561ed071 input=a9049054013a1b77]*/
