/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_functools_cmp_to_key__doc__,
"cmp_to_key($module, /, mycmp)\n"
"--\n"
"\n"
"Convert a cmp= function into a key= function.\n"
"\n"
"  mycmp\n"
"    Function that compares two objects.");

#define _FUNCTOOLS_CMP_TO_KEY_METHODDEF    \
    {"cmp_to_key", _PyCFunction_CAST(_functools_cmp_to_key), METH_FASTCALL|METH_KEYWORDS, _functools_cmp_to_key__doc__},

static PyObject *
_functools_cmp_to_key_impl(PyObject *module, PyObject *mycmp);

static PyObject *
_functools_cmp_to_key(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(mycmp), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mycmp", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cmp_to_key",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *mycmp;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    mycmp = args[0];
    return_value = _functools_cmp_to_key_impl(module, mycmp);

exit:
    return return_value;
}

PyDoc_STRVAR(_functools_reduce__doc__,
"reduce($module, function, iterable, /, initial=<unrepresentable>)\n"
"--\n"
"\n"
"Apply a function of two arguments cumulatively to the items of an iterable, from left to right.\n"
"\n"
"This effectively reduces the iterable to a single value.  If initial is present,\n"
"it is placed before the items of the iterable in the calculation, and serves as\n"
"a default when the iterable is empty.\n"
"\n"
"For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5])\n"
"calculates ((((1 + 2) + 3) + 4) + 5).");

#define _FUNCTOOLS_REDUCE_METHODDEF    \
    {"reduce", _PyCFunction_CAST(_functools_reduce), METH_FASTCALL|METH_KEYWORDS, _functools_reduce__doc__},

static PyObject *
_functools_reduce_impl(PyObject *module, PyObject *func, PyObject *seq,
                       PyObject *result);

static PyObject *
_functools_reduce(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(initial), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "initial", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "reduce",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *func;
    PyObject *seq;
    PyObject *result = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    func = args[0];
    seq = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    result = args[2];
skip_optional_pos:
    return_value = _functools_reduce_impl(module, func, seq, result);

exit:
    return return_value;
}

PyDoc_STRVAR(_functools__lru_cache_wrapper_cache_info__doc__,
"cache_info($self, /)\n"
"--\n"
"\n"
"Report cache statistics");

#define _FUNCTOOLS__LRU_CACHE_WRAPPER_CACHE_INFO_METHODDEF    \
    {"cache_info", (PyCFunction)_functools__lru_cache_wrapper_cache_info, METH_NOARGS, _functools__lru_cache_wrapper_cache_info__doc__},

static PyObject *
_functools__lru_cache_wrapper_cache_info_impl(PyObject *self);

static PyObject *
_functools__lru_cache_wrapper_cache_info(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _functools__lru_cache_wrapper_cache_info_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_functools__lru_cache_wrapper_cache_clear__doc__,
"cache_clear($self, /)\n"
"--\n"
"\n"
"Clear the cache and cache statistics");

#define _FUNCTOOLS__LRU_CACHE_WRAPPER_CACHE_CLEAR_METHODDEF    \
    {"cache_clear", (PyCFunction)_functools__lru_cache_wrapper_cache_clear, METH_NOARGS, _functools__lru_cache_wrapper_cache_clear__doc__},

static PyObject *
_functools__lru_cache_wrapper_cache_clear_impl(PyObject *self);

static PyObject *
_functools__lru_cache_wrapper_cache_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _functools__lru_cache_wrapper_cache_clear_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=7f2abc718fcc35d5 input=a9049054013a1b77]*/
