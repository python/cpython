/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

static PyObject *
interpolation_new_impl(PyTypeObject *type, PyObject *value,
                       PyObject *expression, PyObject *conversion,
                       PyObject *format_spec);

static PyObject *
interpolation_new_helper(PyTypeObject *type, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(value), &_Py_ID(expression), &_Py_ID(conversion), &_Py_ID(format_spec), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"value", "expression", "conversion", "format_spec", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Interpolation",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 1;
    PyObject *value;
    PyObject *expression = &_Py_STR(empty);
    PyObject *conversion = Py_None;
    PyObject *format_spec = &_Py_STR(empty);

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    value = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[1]) {
        if (!PyUnicode_Check(fastargs[1])) {
            _PyArg_BadArgument("Interpolation", "argument 'expression'", "str", fastargs[1]);
            goto exit;
        }
        expression = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        if (!_conversion_converter(fastargs[2], &conversion)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyUnicode_Check(fastargs[3])) {
        _PyArg_BadArgument("Interpolation", "argument 'format_spec'", "str", fastargs[3]);
        goto exit;
    }
    format_spec = fastargs[3];
skip_optional_pos:
    return_value = interpolation_new_impl(type, value, expression, conversion, format_spec);

exit:
    return return_value;
}

static PyObject *
interpolation_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return interpolation_new_helper(type, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
interpolation_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *value;
    PyObject *expression = &_Py_STR(empty);
    PyObject *conversion = Py_None;
    PyObject *format_spec = &_Py_STR(empty);

    assert(_PyType_CAST(type)->tp_new == interpolation_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs < 1 || nargs > 4) {
        return interpolation_new_helper(_PyType_CAST(type), args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames);
    }
    value = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("Interpolation", "argument 'expression'", "str", args[1]);
        goto exit;
    }
    expression = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_conversion_converter(args[2], &conversion)) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[3])) {
        _PyArg_BadArgument("Interpolation", "argument 'format_spec'", "str", args[3]);
        goto exit;
    }
    format_spec = args[3];
skip_optional:
    return_value = interpolation_new_impl(_PyType_CAST(type), value, expression, conversion, format_spec);

exit:
    return return_value;
}
/*[clinic end generated code: output=d07caaaa0e78ca15 input=a9049054013a1b77]*/
