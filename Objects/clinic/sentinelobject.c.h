/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

static PyObject *
sentinel_new_impl(PyTypeObject *type, PyObject *name, PyObject *repr);

static PyObject *
sentinel_new_helper(PyTypeObject *type, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(repr), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "repr", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sentinel",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 1;
    PyObject *name;
    PyObject *repr = Py_None;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("sentinel", "argument 1", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    repr = fastargs[1];
skip_optional_kwonly:
    return_value = sentinel_new_impl(type, name, repr);

exit:
    return return_value;
}

static PyObject *
sentinel_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return sentinel_new_helper(type, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
sentinel_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);

    assert(_PyType_CAST(type)->tp_new == sentinel_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    return sentinel_new_helper(_PyType_CAST(type), args, nargs,
        kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
        NULL, kwnames);
}
/*[clinic end generated code: output=388b220fe03ae62c input=a9049054013a1b77]*/
