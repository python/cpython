/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(signaldict_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n");

#define SIGNALDICT_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(signaldict_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, signaldict_copy__doc__},

static PyObject *
signaldict_copy_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
signaldict_copy(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return signaldict_copy_impl(self, cls);
}

PyDoc_STRVAR(context_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_copy");

#define CONTEXT_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(context_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, context_copy__doc__},

static PyObject *
context_copy_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
context_copy(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return context_copy_impl(self, cls);
}

PyDoc_STRVAR(context_copy___doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define CONTEXT_COPY__METHODDEF    \
    {"__copy__", _PyCFunction_CAST(context_copy_), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, context_copy___doc__},

static PyObject *
context_copy__impl(PyObject *self, PyTypeObject *cls);

static PyObject *
context_copy_(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        return NULL;
    }
    return context_copy__impl(self, cls);
}

PyDoc_STRVAR(context_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define CONTEXT_REDUCE_METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(context_reduce), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, context_reduce__doc__},

static PyObject *
context_reduce_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
context_reduce(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__reduce__() takes no arguments");
        return NULL;
    }
    return context_reduce_impl(self, cls);
}

PyDoc_STRVAR(ctx_from_float__doc__,
"create_decimal_from_float($self, f, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_create_decimal_from_float");

#define CTX_FROM_FLOAT_METHODDEF    \
    {"create_decimal_from_float", _PyCFunction_CAST(ctx_from_float), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_from_float__doc__},

static PyObject *
ctx_from_float_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_from_float(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create_decimal_from_float",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_from_float_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_create_decimal__doc__,
"create_decimal($self, num=0, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_create_decimal");

#define CTX_CREATE_DECIMAL_METHODDEF    \
    {"create_decimal", _PyCFunction_CAST(ctx_create_decimal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_create_decimal__doc__},

static PyObject *
ctx_create_decimal_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_create_decimal(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create_decimal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    v = args[0];
skip_optional_posonly:
    return_value = ctx_create_decimal_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_format__doc__,
"__format__($self, fmtarg, override=<unrepresentable>, /)\n"
"--\n"
"\n");

#define DEC_FORMAT_METHODDEF    \
    {"__format__", _PyCFunction_CAST(dec_format), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_format__doc__},

static PyObject *
dec_format_impl(PyObject *dec, PyTypeObject *cls, PyObject *fmtarg,
                PyObject *override);

static PyObject *
dec_format(PyObject *dec, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__format__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *fmtarg;
    PyObject *override = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fmtarg = args[0];
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    override = args[1];
skip_optional_posonly:
    return_value = dec_format_impl(dec, cls, fmtarg, override);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_as_integer_ratio__doc__,
"as_integer_ratio($self, /)\n"
"--\n"
"\n"
"TODO: doc_as_integer_ratio");

#define DEC_AS_INTEGER_RATIO_METHODDEF    \
    {"as_integer_ratio", _PyCFunction_CAST(dec_as_integer_ratio), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_as_integer_ratio__doc__},

static PyObject *
dec_as_integer_ratio_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
dec_as_integer_ratio(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "as_integer_ratio() takes no arguments");
        return NULL;
    }
    return dec_as_integer_ratio_impl(self, cls);
}

PyDoc_STRVAR(PyDec_ToIntegralValue__doc__,
"to_integral_value($self, /, rounding=None, context=None)\n"
"--\n"
"\n"
"TODO: doc_to_integral_value");

#define PYDEC_TOINTEGRALVALUE_METHODDEF    \
    {"to_integral_value", _PyCFunction_CAST(PyDec_ToIntegralValue), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyDec_ToIntegralValue__doc__},

static PyObject *
PyDec_ToIntegralValue_impl(PyObject *dec, PyTypeObject *cls,
                           PyObject *rounding, PyObject *context);

static PyObject *
PyDec_ToIntegralValue(PyObject *dec, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(rounding), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"rounding", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_integral_value",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *rounding = Py_None;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        rounding = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    context = args[1];
skip_optional_pos:
    return_value = PyDec_ToIntegralValue_impl(dec, cls, rounding, context);

exit:
    return return_value;
}

PyDoc_STRVAR(PyDec_ToIntegralValue___doc__,
"to_integral($self, /, rounding=None, context=None)\n"
"--\n"
"\n"
"TODO: doc_to_integral");

#define PYDEC_TOINTEGRALVALUE__METHODDEF    \
    {"to_integral", _PyCFunction_CAST(PyDec_ToIntegralValue_), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyDec_ToIntegralValue___doc__},

static PyObject *
PyDec_ToIntegralValue__impl(PyObject *dec, PyTypeObject *cls,
                            PyObject *rounding, PyObject *context);

static PyObject *
PyDec_ToIntegralValue_(PyObject *dec, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(rounding), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"rounding", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_integral",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *rounding = Py_None;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        rounding = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    context = args[1];
skip_optional_pos:
    return_value = PyDec_ToIntegralValue__impl(dec, cls, rounding, context);

exit:
    return return_value;
}

PyDoc_STRVAR(PyDec_ToIntegralExact__doc__,
"to_integral_exact($self, /, rounding=None, context=None)\n"
"--\n"
"\n"
"TODO: doc_to_integral_exact");

#define PYDEC_TOINTEGRALEXACT_METHODDEF    \
    {"to_integral_exact", _PyCFunction_CAST(PyDec_ToIntegralExact), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyDec_ToIntegralExact__doc__},

static PyObject *
PyDec_ToIntegralExact_impl(PyObject *dec, PyTypeObject *cls,
                           PyObject *rounding, PyObject *context);

static PyObject *
PyDec_ToIntegralExact(PyObject *dec, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(rounding), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"rounding", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_integral_exact",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *rounding = Py_None;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        rounding = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    context = args[1];
skip_optional_pos:
    return_value = PyDec_ToIntegralExact_impl(dec, cls, rounding, context);

exit:
    return return_value;
}

PyDoc_STRVAR(PyDec_Round__doc__,
"__round__($self, x=<unrepresentable>, /)\n"
"--\n"
"\n");

#define PYDEC_ROUND_METHODDEF    \
    {"__round__", _PyCFunction_CAST(PyDec_Round), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyDec_Round__doc__},

static PyObject *
PyDec_Round_impl(PyObject *dec, PyTypeObject *cls, PyObject *x);

static PyObject *
PyDec_Round(PyObject *dec, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__round__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *x = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    x = args[0];
skip_optional_posonly:
    return_value = PyDec_Round_impl(dec, cls, x);

exit:
    return return_value;
}

PyDoc_STRVAR(PyDec_AsTuple__doc__,
"as_tuple($self, /)\n"
"--\n"
"\n"
"TODO: doc_as_tuple");

#define PYDEC_ASTUPLE_METHODDEF    \
    {"as_tuple", _PyCFunction_CAST(PyDec_AsTuple), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyDec_AsTuple__doc__},

static PyObject *
PyDec_AsTuple_impl(PyObject *dec, PyTypeObject *cls);

static PyObject *
PyDec_AsTuple(PyObject *dec, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "as_tuple() takes no arguments");
        return NULL;
    }
    return PyDec_AsTuple_impl(dec, cls);
}

PyDoc_STRVAR(dec_mpd_qexp__doc__,
"exp($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_exp");

#define DEC_MPD_QEXP_METHODDEF    \
    {"exp", _PyCFunction_CAST(dec_mpd_qexp), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qexp__doc__},

static PyObject *
dec_mpd_qexp_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qexp(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "exp",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qexp_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qln__doc__,
"ln($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_ln");

#define DEC_MPD_QLN_METHODDEF    \
    {"ln", _PyCFunction_CAST(dec_mpd_qln), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qln__doc__},

static PyObject *
dec_mpd_qln_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qln(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ln",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qln_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qlog10__doc__,
"log10($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_log10");

#define DEC_MPD_QLOG10_METHODDEF    \
    {"log10", _PyCFunction_CAST(dec_mpd_qlog10), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qlog10__doc__},

static PyObject *
dec_mpd_qlog10_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qlog10(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "log10",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qlog10_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qnext_minus__doc__,
"next_minus($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_next_minus");

#define DEC_MPD_QNEXT_MINUS_METHODDEF    \
    {"next_minus", _PyCFunction_CAST(dec_mpd_qnext_minus), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qnext_minus__doc__},

static PyObject *
dec_mpd_qnext_minus_impl(PyObject *self, PyTypeObject *cls,
                         PyObject *context);

static PyObject *
dec_mpd_qnext_minus(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "next_minus",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qnext_minus_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qnext_plus__doc__,
"next_plus($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_next_plus");

#define DEC_MPD_QNEXT_PLUS_METHODDEF    \
    {"next_plus", _PyCFunction_CAST(dec_mpd_qnext_plus), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qnext_plus__doc__},

static PyObject *
dec_mpd_qnext_plus_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qnext_plus(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "next_plus",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qnext_plus_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qreduce__doc__,
"normalize($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_normalize");

#define DEC_MPD_QREDUCE_METHODDEF    \
    {"normalize", _PyCFunction_CAST(dec_mpd_qreduce), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qreduce__doc__},

static PyObject *
dec_mpd_qreduce_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qreduce(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "normalize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qreduce_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qsqrt__doc__,
"sqrt($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_sqrt");

#define DEC_MPD_QSQRT_METHODDEF    \
    {"sqrt", _PyCFunction_CAST(dec_mpd_qsqrt), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qsqrt__doc__},

static PyObject *
dec_mpd_qsqrt_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qsqrt(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sqrt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qsqrt_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qcompare__doc__,
"compare($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_compare");

#define DEC_MPD_QCOMPARE_METHODDEF    \
    {"compare", _PyCFunction_CAST(dec_mpd_qcompare), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qcompare__doc__},

static PyObject *
dec_mpd_qcompare_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                      PyObject *context);

static PyObject *
dec_mpd_qcompare(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qcompare_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qcompare_signal__doc__,
"compare_signal($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_compare_signal");

#define DEC_MPD_QCOMPARE_SIGNAL_METHODDEF    \
    {"compare_signal", _PyCFunction_CAST(dec_mpd_qcompare_signal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qcompare_signal__doc__},

static PyObject *
dec_mpd_qcompare_signal_impl(PyObject *self, PyTypeObject *cls,
                             PyObject *other, PyObject *context);

static PyObject *
dec_mpd_qcompare_signal(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare_signal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qcompare_signal_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qmax__doc__,
"max($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_max");

#define DEC_MPD_QMAX_METHODDEF    \
    {"max", _PyCFunction_CAST(dec_mpd_qmax), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qmax__doc__},

static PyObject *
dec_mpd_qmax_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                  PyObject *context);

static PyObject *
dec_mpd_qmax(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "max",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qmax_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qmax_mag__doc__,
"max_mag($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_max_mag");

#define DEC_MPD_QMAX_MAG_METHODDEF    \
    {"max_mag", _PyCFunction_CAST(dec_mpd_qmax_mag), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qmax_mag__doc__},

static PyObject *
dec_mpd_qmax_mag_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                      PyObject *context);

static PyObject *
dec_mpd_qmax_mag(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "max_mag",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qmax_mag_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qmin__doc__,
"min($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_min");

#define DEC_MPD_QMIN_METHODDEF    \
    {"min", _PyCFunction_CAST(dec_mpd_qmin), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qmin__doc__},

static PyObject *
dec_mpd_qmin_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                  PyObject *context);

static PyObject *
dec_mpd_qmin(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "min",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qmin_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qmin_mag__doc__,
"min_mag($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_min_mag");

#define DEC_MPD_QMIN_MAG_METHODDEF    \
    {"min_mag", _PyCFunction_CAST(dec_mpd_qmin_mag), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qmin_mag__doc__},

static PyObject *
dec_mpd_qmin_mag_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                      PyObject *context);

static PyObject *
dec_mpd_qmin_mag(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "min_mag",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qmin_mag_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qnext_toward__doc__,
"next_toward($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_next_toward");

#define DEC_MPD_QNEXT_TOWARD_METHODDEF    \
    {"next_toward", _PyCFunction_CAST(dec_mpd_qnext_toward), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qnext_toward__doc__},

static PyObject *
dec_mpd_qnext_toward_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                          PyObject *context);

static PyObject *
dec_mpd_qnext_toward(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "next_toward",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qnext_toward_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qrem_near__doc__,
"remainder_near($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_remainder_near");

#define DEC_MPD_QREM_NEAR_METHODDEF    \
    {"remainder_near", _PyCFunction_CAST(dec_mpd_qrem_near), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qrem_near__doc__},

static PyObject *
dec_mpd_qrem_near_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                       PyObject *context);

static PyObject *
dec_mpd_qrem_near(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "remainder_near",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qrem_near_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qfma__doc__,
"fma($self, /, other, third, context=None)\n"
"--\n"
"\n"
"TODO: doc_fma");

#define DEC_MPD_QFMA_METHODDEF    \
    {"fma", _PyCFunction_CAST(dec_mpd_qfma), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qfma__doc__},

static PyObject *
dec_mpd_qfma_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                  PyObject *third, PyObject *context);

static PyObject *
dec_mpd_qfma(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(third), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "third", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fma",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *other;
    PyObject *third;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    third = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[2];
skip_optional_pos:
    return_value = dec_mpd_qfma_impl(self, cls, other, third, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_isnormal__doc__,
"is_normal($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_is_normal");

#define DEC_MPD_ISNORMAL_METHODDEF    \
    {"is_normal", _PyCFunction_CAST(dec_mpd_isnormal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_isnormal__doc__},

static PyObject *
dec_mpd_isnormal_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_isnormal(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_normal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_isnormal_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_issubnormal__doc__,
"is_subnormal($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_is_subnormal");

#define DEC_MPD_ISSUBNORMAL_METHODDEF    \
    {"is_subnormal", _PyCFunction_CAST(dec_mpd_issubnormal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_issubnormal__doc__},

static PyObject *
dec_mpd_issubnormal_impl(PyObject *self, PyTypeObject *cls,
                         PyObject *context);

static PyObject *
dec_mpd_issubnormal(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_subnormal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_issubnormal_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_radix__doc__,
"radix($self, /)\n"
"--\n"
"\n"
"TODO: doc_radix");

#define DEC_MPD_RADIX_METHODDEF    \
    {"radix", _PyCFunction_CAST(dec_mpd_radix), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_radix__doc__},

static PyObject *
dec_mpd_radix_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
dec_mpd_radix(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "radix() takes no arguments");
        return NULL;
    }
    return dec_mpd_radix_impl(self, cls);
}

PyDoc_STRVAR(dec_mpd_qcopy_abs__doc__,
"copy_abs($self, /)\n"
"--\n"
"\n"
"TODO: doc_copy_abs");

#define DEC_MPD_QCOPY_ABS_METHODDEF    \
    {"copy_abs", _PyCFunction_CAST(dec_mpd_qcopy_abs), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qcopy_abs__doc__},

static PyObject *
dec_mpd_qcopy_abs_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
dec_mpd_qcopy_abs(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy_abs() takes no arguments");
        return NULL;
    }
    return dec_mpd_qcopy_abs_impl(self, cls);
}

PyDoc_STRVAR(dec_mpd_qcopy_negate__doc__,
"copy_negate($self, /)\n"
"--\n"
"\n"
"TODO: doc_copy_negate");

#define DEC_MPD_QCOPY_NEGATE_METHODDEF    \
    {"copy_negate", _PyCFunction_CAST(dec_mpd_qcopy_negate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qcopy_negate__doc__},

static PyObject *
dec_mpd_qcopy_negate_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
dec_mpd_qcopy_negate(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy_negate() takes no arguments");
        return NULL;
    }
    return dec_mpd_qcopy_negate_impl(self, cls);
}

PyDoc_STRVAR(dec_mpd_qinvert__doc__,
"logical_invert($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_logical_invert");

#define DEC_MPD_QINVERT_METHODDEF    \
    {"logical_invert", _PyCFunction_CAST(dec_mpd_qinvert), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qinvert__doc__},

static PyObject *
dec_mpd_qinvert_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qinvert(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_invert",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qinvert_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qlogb__doc__,
"logb($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_logb");

#define DEC_MPD_QLOGB_METHODDEF    \
    {"logb", _PyCFunction_CAST(dec_mpd_qlogb), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qlogb__doc__},

static PyObject *
dec_mpd_qlogb_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_qlogb(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logb",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_qlogb_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_class__doc__,
"number_class($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_number_class");

#define DEC_MPD_CLASS_METHODDEF    \
    {"number_class", _PyCFunction_CAST(dec_mpd_class), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_class__doc__},

static PyObject *
dec_mpd_class_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_class(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "number_class",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_class_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_to_eng__doc__,
"to_eng_string($self, /, context=None)\n"
"--\n"
"\n"
"TODO: doc_to_eng_string");

#define DEC_MPD_TO_ENG_METHODDEF    \
    {"to_eng_string", _PyCFunction_CAST(dec_mpd_to_eng), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_to_eng__doc__},

static PyObject *
dec_mpd_to_eng_impl(PyObject *self, PyTypeObject *cls, PyObject *context);

static PyObject *
dec_mpd_to_eng(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_eng_string",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = dec_mpd_to_eng_impl(self, cls, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_compare_total__doc__,
"compare_total($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_compare_total");

#define DEC_MPD_COMPARE_TOTAL_METHODDEF    \
    {"compare_total", _PyCFunction_CAST(dec_mpd_compare_total), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_compare_total__doc__},

static PyObject *
dec_mpd_compare_total_impl(PyObject *self, PyTypeObject *cls,
                           PyObject *other, PyObject *context);

static PyObject *
dec_mpd_compare_total(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare_total",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_compare_total_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_compare_total_mag__doc__,
"compare_total_mag($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_compare_total_mag");

#define DEC_MPD_COMPARE_TOTAL_MAG_METHODDEF    \
    {"compare_total_mag", _PyCFunction_CAST(dec_mpd_compare_total_mag), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_compare_total_mag__doc__},

static PyObject *
dec_mpd_compare_total_mag_impl(PyObject *self, PyTypeObject *cls,
                               PyObject *other, PyObject *context);

static PyObject *
dec_mpd_compare_total_mag(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare_total_mag",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_compare_total_mag_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qcopy_sign__doc__,
"copy_sign($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_copy_sign");

#define DEC_MPD_QCOPY_SIGN_METHODDEF    \
    {"copy_sign", _PyCFunction_CAST(dec_mpd_qcopy_sign), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qcopy_sign__doc__},

static PyObject *
dec_mpd_qcopy_sign_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                        PyObject *context);

static PyObject *
dec_mpd_qcopy_sign(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "copy_sign",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qcopy_sign_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_same_quantum__doc__,
"same_quantum($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_same_quantum");

#define DEC_MPD_SAME_QUANTUM_METHODDEF    \
    {"same_quantum", _PyCFunction_CAST(dec_mpd_same_quantum), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_same_quantum__doc__},

static PyObject *
dec_mpd_same_quantum_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                          PyObject *context);

static PyObject *
dec_mpd_same_quantum(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "same_quantum",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_same_quantum_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qand__doc__,
"logical_and($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_logical_and");

#define DEC_MPD_QAND_METHODDEF    \
    {"logical_and", _PyCFunction_CAST(dec_mpd_qand), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qand__doc__},

static PyObject *
dec_mpd_qand_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                  PyObject *context);

static PyObject *
dec_mpd_qand(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_and",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qand_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qor__doc__,
"logical_or($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_logical_or");

#define DEC_MPD_QOR_METHODDEF    \
    {"logical_or", _PyCFunction_CAST(dec_mpd_qor), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qor__doc__},

static PyObject *
dec_mpd_qor_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                 PyObject *context);

static PyObject *
dec_mpd_qor(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_or",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qor_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qxor__doc__,
"logical_xor($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_logical_xor");

#define DEC_MPD_QXOR_METHODDEF    \
    {"logical_xor", _PyCFunction_CAST(dec_mpd_qxor), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qxor__doc__},

static PyObject *
dec_mpd_qxor_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                  PyObject *context);

static PyObject *
dec_mpd_qxor(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_xor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qxor_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qrotate__doc__,
"rotate($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_rotate");

#define DEC_MPD_QROTATE_METHODDEF    \
    {"rotate", _PyCFunction_CAST(dec_mpd_qrotate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qrotate__doc__},

static PyObject *
dec_mpd_qrotate_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                     PyObject *context);

static PyObject *
dec_mpd_qrotate(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "rotate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qrotate_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qscaleb__doc__,
"scaleb($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_scaleb");

#define DEC_MPD_QSCALEB_METHODDEF    \
    {"scaleb", _PyCFunction_CAST(dec_mpd_qscaleb), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qscaleb__doc__},

static PyObject *
dec_mpd_qscaleb_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                     PyObject *context);

static PyObject *
dec_mpd_qscaleb(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "scaleb",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qscaleb_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qshift__doc__,
"shift($self, /, other, context=None)\n"
"--\n"
"\n"
"TODO: doc_shift");

#define DEC_MPD_QSHIFT_METHODDEF    \
    {"shift", _PyCFunction_CAST(dec_mpd_qshift), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qshift__doc__},

static PyObject *
dec_mpd_qshift_impl(PyObject *self, PyTypeObject *cls, PyObject *other,
                    PyObject *context);

static PyObject *
dec_mpd_qshift(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(other), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"other", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "shift",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *other;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = dec_mpd_qshift_impl(self, cls, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_mpd_qquantize__doc__,
"quantize($self, /, exp, rounding=None, context=None)\n"
"--\n"
"\n"
"TODO: doc_quantize");

#define DEC_MPD_QQUANTIZE_METHODDEF    \
    {"quantize", _PyCFunction_CAST(dec_mpd_qquantize), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_mpd_qquantize__doc__},

static PyObject *
dec_mpd_qquantize_impl(PyObject *v, PyTypeObject *cls, PyObject *w,
                       PyObject *rounding, PyObject *context);

static PyObject *
dec_mpd_qquantize(PyObject *v, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(exp), &_Py_ID(rounding), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"exp", "rounding", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "quantize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *w;
    PyObject *rounding = Py_None;
    PyObject *context = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    w = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        rounding = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    context = args[2];
skip_optional_pos:
    return_value = dec_mpd_qquantize_impl(v, cls, w, rounding, context);

exit:
    return return_value;
}

PyDoc_STRVAR(dec_ceil__doc__,
"__ceil__($self, /)\n"
"--\n"
"\n");

#define DEC_CEIL_METHODDEF    \
    {"__ceil__", _PyCFunction_CAST(dec_ceil), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_ceil__doc__},

static PyObject *
dec_ceil_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
dec_ceil(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__ceil__() takes no arguments");
        return NULL;
    }
    return dec_ceil_impl(self, cls);
}

PyDoc_STRVAR(dec_floor__doc__,
"__floor__($self, /)\n"
"--\n"
"\n");

#define DEC_FLOOR_METHODDEF    \
    {"__floor__", _PyCFunction_CAST(dec_floor), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_floor__doc__},

static PyObject *
dec_floor_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
dec_floor(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__floor__() takes no arguments");
        return NULL;
    }
    return dec_floor_impl(self, cls);
}

PyDoc_STRVAR(dec_trunc__doc__,
"__trunc__($self, /)\n"
"--\n"
"\n");

#define DEC_TRUNC_METHODDEF    \
    {"__trunc__", _PyCFunction_CAST(dec_trunc), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, dec_trunc__doc__},

static PyObject *
dec_trunc_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
dec_trunc(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__trunc__() takes no arguments");
        return NULL;
    }
    return dec_trunc_impl(self, cls);
}

PyDoc_STRVAR(ctx_mpd_qabs__doc__,
"abs($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_abs");

#define CTX_MPD_QABS_METHODDEF    \
    {"abs", _PyCFunction_CAST(ctx_mpd_qabs), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qabs__doc__},

static PyObject *
ctx_mpd_qabs_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qabs(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "abs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qabs_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qexp__doc__,
"exp($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_exp");

#define CTX_MPD_QEXP_METHODDEF    \
    {"exp", _PyCFunction_CAST(ctx_mpd_qexp), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qexp__doc__},

static PyObject *
ctx_mpd_qexp_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qexp(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "exp",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qexp_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qln__doc__,
"ln($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_ln");

#define CTX_MPD_QLN_METHODDEF    \
    {"ln", _PyCFunction_CAST(ctx_mpd_qln), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qln__doc__},

static PyObject *
ctx_mpd_qln_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qln(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ln",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qln_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qlog10__doc__,
"log10($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_log10");

#define CTX_MPD_QLOG10_METHODDEF    \
    {"log10", _PyCFunction_CAST(ctx_mpd_qlog10), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qlog10__doc__},

static PyObject *
ctx_mpd_qlog10_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qlog10(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "log10",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qlog10_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qminus__doc__,
"minus($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_minus");

#define CTX_MPD_QMINUS_METHODDEF    \
    {"minus", _PyCFunction_CAST(ctx_mpd_qminus), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qminus__doc__},

static PyObject *
ctx_mpd_qminus_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qminus(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "minus",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qminus_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qnext_minus__doc__,
"next_minus($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_next_minus");

#define CTX_MPD_QNEXT_MINUS_METHODDEF    \
    {"next_minus", _PyCFunction_CAST(ctx_mpd_qnext_minus), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qnext_minus__doc__},

static PyObject *
ctx_mpd_qnext_minus_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qnext_minus(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "next_minus",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qnext_minus_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qnext_plus__doc__,
"next_plus($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_next_plus");

#define CTX_MPD_QNEXT_PLUS_METHODDEF    \
    {"next_plus", _PyCFunction_CAST(ctx_mpd_qnext_plus), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qnext_plus__doc__},

static PyObject *
ctx_mpd_qnext_plus_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qnext_plus(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "next_plus",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qnext_plus_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qplus__doc__,
"plus($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_plus");

#define CTX_MPD_QPLUS_METHODDEF    \
    {"plus", _PyCFunction_CAST(ctx_mpd_qplus), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qplus__doc__},

static PyObject *
ctx_mpd_qplus_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qplus(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "plus",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qplus_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qreduce__doc__,
"normalize($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_normalize");

#define CTX_MPD_QREDUCE_METHODDEF    \
    {"normalize", _PyCFunction_CAST(ctx_mpd_qreduce), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qreduce__doc__},

static PyObject *
ctx_mpd_qreduce_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qreduce(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "normalize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qreduce_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qround_to_int__doc__,
"to_integral_value($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_to_integral_value");

#define CTX_MPD_QROUND_TO_INT_METHODDEF    \
    {"to_integral_value", _PyCFunction_CAST(ctx_mpd_qround_to_int), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qround_to_int__doc__},

static PyObject *
ctx_mpd_qround_to_int_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qround_to_int(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_integral_value",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qround_to_int_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qround_to_int___doc__,
"to_integral($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_to_integral");

#define CTX_MPD_QROUND_TO_INT__METHODDEF    \
    {"to_integral", _PyCFunction_CAST(ctx_mpd_qround_to_int_), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qround_to_int___doc__},

static PyObject *
ctx_mpd_qround_to_int__impl(PyObject *context, PyTypeObject *cls,
                            PyObject *v);

static PyObject *
ctx_mpd_qround_to_int_(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_integral",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qround_to_int__impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qround_to_intx__doc__,
"to_integral_exact($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_to_integral_exact");

#define CTX_MPD_QROUND_TO_INTX_METHODDEF    \
    {"to_integral_exact", _PyCFunction_CAST(ctx_mpd_qround_to_intx), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qround_to_intx__doc__},

static PyObject *
ctx_mpd_qround_to_intx_impl(PyObject *context, PyTypeObject *cls,
                            PyObject *v);

static PyObject *
ctx_mpd_qround_to_intx(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_integral_exact",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qround_to_intx_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qsqrt__doc__,
"sqrt($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_sqrt");

#define CTX_MPD_QSQRT_METHODDEF    \
    {"sqrt", _PyCFunction_CAST(ctx_mpd_qsqrt), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qsqrt__doc__},

static PyObject *
ctx_mpd_qsqrt_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qsqrt(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sqrt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qsqrt_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qadd__doc__,
"add($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_add");

#define CTX_MPD_QADD_METHODDEF    \
    {"add", _PyCFunction_CAST(ctx_mpd_qadd), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qadd__doc__},

static PyObject *
ctx_mpd_qadd_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qadd(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "add",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qadd_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qcompare__doc__,
"compare($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_compare");

#define CTX_MPD_QCOMPARE_METHODDEF    \
    {"compare", _PyCFunction_CAST(ctx_mpd_qcompare), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qcompare__doc__},

static PyObject *
ctx_mpd_qcompare_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                      PyObject *w);

static PyObject *
ctx_mpd_qcompare(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qcompare_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qcompare_signal__doc__,
"compare_signal($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_compare_signal");

#define CTX_MPD_QCOMPARE_SIGNAL_METHODDEF    \
    {"compare_signal", _PyCFunction_CAST(ctx_mpd_qcompare_signal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qcompare_signal__doc__},

static PyObject *
ctx_mpd_qcompare_signal_impl(PyObject *context, PyTypeObject *cls,
                             PyObject *v, PyObject *w);

static PyObject *
ctx_mpd_qcompare_signal(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare_signal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qcompare_signal_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qdiv__doc__,
"divide($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_divide");

#define CTX_MPD_QDIV_METHODDEF    \
    {"divide", _PyCFunction_CAST(ctx_mpd_qdiv), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qdiv__doc__},

static PyObject *
ctx_mpd_qdiv_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qdiv(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "divide",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qdiv_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qdivint__doc__,
"divide_int($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_divide_int");

#define CTX_MPD_QDIVINT_METHODDEF    \
    {"divide_int", _PyCFunction_CAST(ctx_mpd_qdivint), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qdivint__doc__},

static PyObject *
ctx_mpd_qdivint_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                     PyObject *w);

static PyObject *
ctx_mpd_qdivint(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "divide_int",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qdivint_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qmax__doc__,
"max($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_max");

#define CTX_MPD_QMAX_METHODDEF    \
    {"max", _PyCFunction_CAST(ctx_mpd_qmax), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qmax__doc__},

static PyObject *
ctx_mpd_qmax_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qmax(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "max",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qmax_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qmax_mag__doc__,
"max_mag($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_max_mag");

#define CTX_MPD_QMAX_MAG_METHODDEF    \
    {"max_mag", _PyCFunction_CAST(ctx_mpd_qmax_mag), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qmax_mag__doc__},

static PyObject *
ctx_mpd_qmax_mag_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                      PyObject *w);

static PyObject *
ctx_mpd_qmax_mag(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "max_mag",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qmax_mag_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qmin__doc__,
"min($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_min");

#define CTX_MPD_QMIN_METHODDEF    \
    {"min", _PyCFunction_CAST(ctx_mpd_qmin), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qmin__doc__},

static PyObject *
ctx_mpd_qmin_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qmin(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "min",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qmin_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qmin_mag__doc__,
"min_mag($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_min_mag");

#define CTX_MPD_QMIN_MAG_METHODDEF    \
    {"min_mag", _PyCFunction_CAST(ctx_mpd_qmin_mag), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qmin_mag__doc__},

static PyObject *
ctx_mpd_qmin_mag_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                      PyObject *w);

static PyObject *
ctx_mpd_qmin_mag(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "min_mag",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qmin_mag_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qmul__doc__,
"multiply($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_multiply");

#define CTX_MPD_QMUL_METHODDEF    \
    {"multiply", _PyCFunction_CAST(ctx_mpd_qmul), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qmul__doc__},

static PyObject *
ctx_mpd_qmul_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qmul(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "multiply",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qmul_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qnext_toward__doc__,
"next_toward($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_next_toward");

#define CTX_MPD_QNEXT_TOWARD_METHODDEF    \
    {"next_toward", _PyCFunction_CAST(ctx_mpd_qnext_toward), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qnext_toward__doc__},

static PyObject *
ctx_mpd_qnext_toward_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                          PyObject *w);

static PyObject *
ctx_mpd_qnext_toward(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "next_toward",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qnext_toward_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qquantize__doc__,
"quantize($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_quantize");

#define CTX_MPD_QQUANTIZE_METHODDEF    \
    {"quantize", _PyCFunction_CAST(ctx_mpd_qquantize), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qquantize__doc__},

static PyObject *
ctx_mpd_qquantize_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                       PyObject *w);

static PyObject *
ctx_mpd_qquantize(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "quantize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qquantize_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qrem__doc__,
"remainder($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_remainder");

#define CTX_MPD_QREM_METHODDEF    \
    {"remainder", _PyCFunction_CAST(ctx_mpd_qrem), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qrem__doc__},

static PyObject *
ctx_mpd_qrem_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qrem(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "remainder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qrem_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qrem_near__doc__,
"remainder_near($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_remainder_near");

#define CTX_MPD_QREM_NEAR_METHODDEF    \
    {"remainder_near", _PyCFunction_CAST(ctx_mpd_qrem_near), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qrem_near__doc__},

static PyObject *
ctx_mpd_qrem_near_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                       PyObject *w);

static PyObject *
ctx_mpd_qrem_near(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "remainder_near",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qrem_near_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qsub__doc__,
"subtract($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_subtract");

#define CTX_MPD_QSUB_METHODDEF    \
    {"subtract", _PyCFunction_CAST(ctx_mpd_qsub), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qsub__doc__},

static PyObject *
ctx_mpd_qsub_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qsub(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "subtract",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qsub_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qdivmod__doc__,
"divmod($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_divmod");

#define CTX_MPD_QDIVMOD_METHODDEF    \
    {"divmod", _PyCFunction_CAST(ctx_mpd_qdivmod), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qdivmod__doc__},

static PyObject *
ctx_mpd_qdivmod_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                     PyObject *w);

static PyObject *
ctx_mpd_qdivmod(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "divmod",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qdivmod_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qpow__doc__,
"power($self, /, a, b, modulo=None)\n"
"--\n"
"\n"
"TODO: doc_ctx_power");

#define CTX_MPD_QPOW_METHODDEF    \
    {"power", _PyCFunction_CAST(ctx_mpd_qpow), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qpow__doc__},

static PyObject *
ctx_mpd_qpow_impl(PyObject *context, PyTypeObject *cls, PyObject *base,
                  PyObject *exp, PyObject *mod);

static PyObject *
ctx_mpd_qpow(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(b), &_Py_ID(modulo), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "modulo", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "power",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *base;
    PyObject *exp;
    PyObject *mod = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    base = args[0];
    exp = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    mod = args[2];
skip_optional_pos:
    return_value = ctx_mpd_qpow_impl(context, cls, base, exp, mod);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qfma__doc__,
"fma($self, a, b, c, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_fma");

#define CTX_MPD_QFMA_METHODDEF    \
    {"fma", _PyCFunction_CAST(ctx_mpd_qfma), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qfma__doc__},

static PyObject *
ctx_mpd_qfma_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w, PyObject *x);

static PyObject *
ctx_mpd_qfma(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fma",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject *v;
    PyObject *w;
    PyObject *x;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    x = args[2];
    return_value = ctx_mpd_qfma_impl(context, cls, v, w, x);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_radix__doc__,
"radix($self, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_radix");

#define CTX_MPD_RADIX_METHODDEF    \
    {"radix", _PyCFunction_CAST(ctx_mpd_radix), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_radix__doc__},

static PyObject *
ctx_mpd_radix_impl(PyObject *context, PyTypeObject *cls);

static PyObject *
ctx_mpd_radix(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "radix() takes no arguments");
        return NULL;
    }
    return ctx_mpd_radix_impl(context, cls);
}

PyDoc_STRVAR(ctx_mpd_isnormal__doc__,
"is_normal($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_normal");

#define CTX_MPD_ISNORMAL_METHODDEF    \
    {"is_normal", _PyCFunction_CAST(ctx_mpd_isnormal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_isnormal__doc__},

static PyObject *
ctx_mpd_isnormal_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_isnormal(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_normal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_isnormal_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_issubnormal__doc__,
"is_subnormal($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_subnormal");

#define CTX_MPD_ISSUBNORMAL_METHODDEF    \
    {"is_subnormal", _PyCFunction_CAST(ctx_mpd_issubnormal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_issubnormal__doc__},

static PyObject *
ctx_mpd_issubnormal_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_issubnormal(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_subnormal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_issubnormal_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_isfinite__doc__,
"is_finite($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_finite");

#define CTX_MPD_ISFINITE_METHODDEF    \
    {"is_finite", _PyCFunction_CAST(ctx_mpd_isfinite), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_isfinite__doc__},

static PyObject *
ctx_mpd_isfinite_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_isfinite(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_finite",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_isfinite_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_isinfinite__doc__,
"is_infinite($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_infinite");

#define CTX_MPD_ISINFINITE_METHODDEF    \
    {"is_infinite", _PyCFunction_CAST(ctx_mpd_isinfinite), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_isinfinite__doc__},

static PyObject *
ctx_mpd_isinfinite_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_isinfinite(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_infinite",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_isinfinite_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_isnan__doc__,
"is_nan($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_nan");

#define CTX_MPD_ISNAN_METHODDEF    \
    {"is_nan", _PyCFunction_CAST(ctx_mpd_isnan), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_isnan__doc__},

static PyObject *
ctx_mpd_isnan_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_isnan(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_nan",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_isnan_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_isqnan__doc__,
"is_qnan($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_qnan");

#define CTX_MPD_ISQNAN_METHODDEF    \
    {"is_qnan", _PyCFunction_CAST(ctx_mpd_isqnan), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_isqnan__doc__},

static PyObject *
ctx_mpd_isqnan_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_isqnan(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_qnan",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_isqnan_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_issigned__doc__,
"is_signed($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_signed");

#define CTX_MPD_ISSIGNED_METHODDEF    \
    {"is_signed", _PyCFunction_CAST(ctx_mpd_issigned), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_issigned__doc__},

static PyObject *
ctx_mpd_issigned_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_issigned(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_signed",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_issigned_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_issnan__doc__,
"is_snan($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_snan");

#define CTX_MPD_ISSNAN_METHODDEF    \
    {"is_snan", _PyCFunction_CAST(ctx_mpd_issnan), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_issnan__doc__},

static PyObject *
ctx_mpd_issnan_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_issnan(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_snan",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_issnan_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_iszero__doc__,
"is_zero($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_zero");

#define CTX_MPD_ISZERO_METHODDEF    \
    {"is_zero", _PyCFunction_CAST(ctx_mpd_iszero), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_iszero__doc__},

static PyObject *
ctx_mpd_iszero_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_iszero(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_zero",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_iszero_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_iscanonical__doc__,
"is_canonical($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_is_canonical");

#define CTX_ISCANONICAL_METHODDEF    \
    {"is_canonical", _PyCFunction_CAST(ctx_iscanonical), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_iscanonical__doc__},

static PyObject *
ctx_iscanonical_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_iscanonical(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_canonical",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_iscanonical_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(PyDecContext_Apply__doc__,
"apply($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_apply");

#define PYDECCONTEXT_APPLY_METHODDEF    \
    {"apply", _PyCFunction_CAST(PyDecContext_Apply), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyDecContext_Apply__doc__},

static PyObject *
PyDecContext_Apply_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
PyDecContext_Apply(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "apply",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = PyDecContext_Apply_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(PyDecContext_Apply___doc__,
"_apply($self, v, /)\n"
"--\n"
"\n");

#define PYDECCONTEXT_APPLY__METHODDEF    \
    {"_apply", _PyCFunction_CAST(PyDecContext_Apply_), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyDecContext_Apply___doc__},

static PyObject *
PyDecContext_Apply__impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
PyDecContext_Apply_(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_apply",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = PyDecContext_Apply__impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_canonical__doc__,
"canonical($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_canonical");

#define CTX_CANONICAL_METHODDEF    \
    {"canonical", _PyCFunction_CAST(ctx_canonical), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_canonical__doc__},

static PyObject *
ctx_canonical_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_canonical(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "canonical",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_canonical_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qcopy_abs__doc__,
"copy_abs($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_copy_abs");

#define CTX_MPD_QCOPY_ABS_METHODDEF    \
    {"copy_abs", _PyCFunction_CAST(ctx_mpd_qcopy_abs), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qcopy_abs__doc__},

static PyObject *
ctx_mpd_qcopy_abs_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qcopy_abs(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "copy_abs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qcopy_abs_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_copy_decimal__doc__,
"copy_decimal($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_copy_decimal");

#define CTX_COPY_DECIMAL_METHODDEF    \
    {"copy_decimal", _PyCFunction_CAST(ctx_copy_decimal), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_copy_decimal__doc__},

static PyObject *
ctx_copy_decimal_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_copy_decimal(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "copy_decimal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_copy_decimal_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qcopy_negate__doc__,
"copy_negate($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_copy_negate");

#define CTX_MPD_QCOPY_NEGATE_METHODDEF    \
    {"copy_negate", _PyCFunction_CAST(ctx_mpd_qcopy_negate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qcopy_negate__doc__},

static PyObject *
ctx_mpd_qcopy_negate_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qcopy_negate(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "copy_negate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qcopy_negate_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qlogb__doc__,
"logb($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_logb");

#define CTX_MPD_QLOGB_METHODDEF    \
    {"logb", _PyCFunction_CAST(ctx_mpd_qlogb), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qlogb__doc__},

static PyObject *
ctx_mpd_qlogb_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qlogb(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logb",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qlogb_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qinvert__doc__,
"logical_invert($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_logical_invert");

#define CTX_MPD_QINVERT_METHODDEF    \
    {"logical_invert", _PyCFunction_CAST(ctx_mpd_qinvert), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qinvert__doc__},

static PyObject *
ctx_mpd_qinvert_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_qinvert(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_invert",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_qinvert_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_class__doc__,
"number_class($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_number_class");

#define CTX_MPD_CLASS_METHODDEF    \
    {"number_class", _PyCFunction_CAST(ctx_mpd_class), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_class__doc__},

static PyObject *
ctx_mpd_class_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_class(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "number_class",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_class_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_to_sci__doc__,
"to_sci_string($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_to_sci_string");

#define CTX_MPD_TO_SCI_METHODDEF    \
    {"to_sci_string", _PyCFunction_CAST(ctx_mpd_to_sci), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_to_sci__doc__},

static PyObject *
ctx_mpd_to_sci_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_to_sci(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_sci_string",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_to_sci_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_to_eng__doc__,
"to_eng_string($self, a, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_to_eng_string");

#define CTX_MPD_TO_ENG_METHODDEF    \
    {"to_eng_string", _PyCFunction_CAST(ctx_mpd_to_eng), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_to_eng__doc__},

static PyObject *
ctx_mpd_to_eng_impl(PyObject *context, PyTypeObject *cls, PyObject *v);

static PyObject *
ctx_mpd_to_eng(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_eng_string",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *v;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    return_value = ctx_mpd_to_eng_impl(context, cls, v);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_compare_total__doc__,
"compare_total($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_compare_total");

#define CTX_MPD_COMPARE_TOTAL_METHODDEF    \
    {"compare_total", _PyCFunction_CAST(ctx_mpd_compare_total), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_compare_total__doc__},

static PyObject *
ctx_mpd_compare_total_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                           PyObject *w);

static PyObject *
ctx_mpd_compare_total(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare_total",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_compare_total_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_compare_total_mag__doc__,
"compare_total_mag($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_compare_total_mag");

#define CTX_MPD_COMPARE_TOTAL_MAG_METHODDEF    \
    {"compare_total_mag", _PyCFunction_CAST(ctx_mpd_compare_total_mag), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_compare_total_mag__doc__},

static PyObject *
ctx_mpd_compare_total_mag_impl(PyObject *context, PyTypeObject *cls,
                               PyObject *v, PyObject *w);

static PyObject *
ctx_mpd_compare_total_mag(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compare_total_mag",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_compare_total_mag_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qcopy_sign__doc__,
"copy_sign($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_copy_sign");

#define CTX_MPD_QCOPY_SIGN_METHODDEF    \
    {"copy_sign", _PyCFunction_CAST(ctx_mpd_qcopy_sign), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qcopy_sign__doc__},

static PyObject *
ctx_mpd_qcopy_sign_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                        PyObject *w);

static PyObject *
ctx_mpd_qcopy_sign(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "copy_sign",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qcopy_sign_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qand__doc__,
"logical_and($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_logical_and");

#define CTX_MPD_QAND_METHODDEF    \
    {"logical_and", _PyCFunction_CAST(ctx_mpd_qand), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qand__doc__},

static PyObject *
ctx_mpd_qand_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qand(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_and",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qand_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qor__doc__,
"logical_or($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_logical_or");

#define CTX_MPD_QOR_METHODDEF    \
    {"logical_or", _PyCFunction_CAST(ctx_mpd_qor), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qor__doc__},

static PyObject *
ctx_mpd_qor_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                 PyObject *w);

static PyObject *
ctx_mpd_qor(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_or",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qor_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qxor__doc__,
"logical_xor($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_logical_xor");

#define CTX_MPD_QXOR_METHODDEF    \
    {"logical_xor", _PyCFunction_CAST(ctx_mpd_qxor), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qxor__doc__},

static PyObject *
ctx_mpd_qxor_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                  PyObject *w);

static PyObject *
ctx_mpd_qxor(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "logical_xor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qxor_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qrotate__doc__,
"rotate($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_rotate");

#define CTX_MPD_QROTATE_METHODDEF    \
    {"rotate", _PyCFunction_CAST(ctx_mpd_qrotate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qrotate__doc__},

static PyObject *
ctx_mpd_qrotate_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                     PyObject *w);

static PyObject *
ctx_mpd_qrotate(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "rotate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qrotate_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qscaleb__doc__,
"scaleb($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_scaleb");

#define CTX_MPD_QSCALEB_METHODDEF    \
    {"scaleb", _PyCFunction_CAST(ctx_mpd_qscaleb), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qscaleb__doc__},

static PyObject *
ctx_mpd_qscaleb_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                     PyObject *w);

static PyObject *
ctx_mpd_qscaleb(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "scaleb",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qscaleb_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_qshift__doc__,
"shift($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_shift");

#define CTX_MPD_QSHIFT_METHODDEF    \
    {"shift", _PyCFunction_CAST(ctx_mpd_qshift), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_qshift__doc__},

static PyObject *
ctx_mpd_qshift_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                    PyObject *w);

static PyObject *
ctx_mpd_qshift(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "shift",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_qshift_impl(context, cls, v, w);

exit:
    return return_value;
}

PyDoc_STRVAR(ctx_mpd_same_quantum__doc__,
"same_quantum($self, a, b, /)\n"
"--\n"
"\n"
"TODO: doc_ctx_same_quantum");

#define CTX_MPD_SAME_QUANTUM_METHODDEF    \
    {"same_quantum", _PyCFunction_CAST(ctx_mpd_same_quantum), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, ctx_mpd_same_quantum__doc__},

static PyObject *
ctx_mpd_same_quantum_impl(PyObject *context, PyTypeObject *cls, PyObject *v,
                          PyObject *w);

static PyObject *
ctx_mpd_same_quantum(PyObject *context, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "same_quantum",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *v;
    PyObject *w;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    v = args[0];
    w = args[1];
    return_value = ctx_mpd_same_quantum_impl(context, cls, v, w);

exit:
    return return_value;
}
/*[clinic end generated code: output=5bef837deb0a20c9 input=a9049054013a1b77]*/
