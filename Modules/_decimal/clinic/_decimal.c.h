/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_decimal_Decimal_from_float__doc__,
"from_float($type, f, /)\n"
"--\n"
"\n"
"Class method that converts a float to a decimal number, exactly.\n"
"\n"
"Since 0.1 is not exactly representable in binary floating point,\n"
"Decimal.from_float(0.1) is not the same as Decimal(\'0.1\').\n"
"\n"
"    >>> Decimal.from_float(0.1)\n"
"    Decimal(\'0.1000000000000000055511151231257827021181583404541015625\')\n"
"    >>> Decimal.from_float(float(\'nan\'))\n"
"    Decimal(\'NaN\')\n"
"    >>> Decimal.from_float(float(\'inf\'))\n"
"    Decimal(\'Infinity\')\n"
"    >>> Decimal.from_float(float(\'-inf\'))\n"
"    Decimal(\'-Infinity\')");

#define _DECIMAL_DECIMAL_FROM_FLOAT_METHODDEF    \
    {"from_float", (PyCFunction)_decimal_Decimal_from_float, METH_O|METH_CLASS, _decimal_Decimal_from_float__doc__},

static PyObject *
_decimal_Decimal_from_float_impl(PyTypeObject *type, PyObject *pyfloat);

static PyObject *
_decimal_Decimal_from_float(PyObject *type, PyObject *pyfloat)
{
    PyObject *return_value = NULL;

    return_value = _decimal_Decimal_from_float_impl((PyTypeObject *)type, pyfloat);

    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_from_number__doc__,
"from_number($type, number, /)\n"
"--\n"
"\n"
"Class method that converts a real number to a decimal number, exactly.\n"
"\n"
"    >>> Decimal.from_number(314)              # int\n"
"    Decimal(\'314\')\n"
"    >>> Decimal.from_number(0.1)              # float\n"
"    Decimal(\'0.1000000000000000055511151231257827021181583404541015625\')\n"
"    >>> Decimal.from_number(Decimal(\'3.14\'))  # another decimal instance\n"
"    Decimal(\'3.14\')");

#define _DECIMAL_DECIMAL_FROM_NUMBER_METHODDEF    \
    {"from_number", (PyCFunction)_decimal_Decimal_from_number, METH_O|METH_CLASS, _decimal_Decimal_from_number__doc__},

static PyObject *
_decimal_Decimal_from_number_impl(PyTypeObject *type, PyObject *number);

static PyObject *
_decimal_Decimal_from_number(PyObject *type, PyObject *number)
{
    PyObject *return_value = NULL;

    return_value = _decimal_Decimal_from_number_impl((PyTypeObject *)type, number);

    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_as_integer_ratio__doc__,
"as_integer_ratio($self, /)\n"
"--\n"
"\n"
"Return a pair of integers whose ratio is exactly equal to the original.\n"
"\n"
"The ratio is in lowest terms and with a positive denominator.\n"
"Raise OverflowError on infinities and a ValueError on NaNs.");

#define _DECIMAL_DECIMAL_AS_INTEGER_RATIO_METHODDEF    \
    {"as_integer_ratio", (PyCFunction)_decimal_Decimal_as_integer_ratio, METH_NOARGS, _decimal_Decimal_as_integer_ratio__doc__},

static PyObject *
_decimal_Decimal_as_integer_ratio_impl(PyObject *self);

static PyObject *
_decimal_Decimal_as_integer_ratio(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_as_integer_ratio_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_to_integral_value__doc__,
"to_integral_value($self, /, rounding=None, context=None)\n"
"--\n"
"\n"
"Round to the nearest integer without signaling Inexact or Rounded.\n"
"\n"
"The rounding mode is determined by the rounding parameter if given, else by\n"
"the given context. If neither parameter is given, then the rounding mode of\n"
"the current default context is used.");

#define _DECIMAL_DECIMAL_TO_INTEGRAL_VALUE_METHODDEF    \
    {"to_integral_value", _PyCFunction_CAST(_decimal_Decimal_to_integral_value), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_to_integral_value__doc__},

static PyObject *
_decimal_Decimal_to_integral_value_impl(PyObject *self, PyObject *rounding,
                                        PyObject *context);

static PyObject *
_decimal_Decimal_to_integral_value(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    return_value = _decimal_Decimal_to_integral_value_impl(self, rounding, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_to_integral__doc__,
"to_integral($self, /, rounding=None, context=None)\n"
"--\n"
"\n"
"Identical to the to_integral_value() method.\n"
"\n"
"The to_integral() name has been kept for compatibility with older versions.");

#define _DECIMAL_DECIMAL_TO_INTEGRAL_METHODDEF    \
    {"to_integral", _PyCFunction_CAST(_decimal_Decimal_to_integral), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_to_integral__doc__},

static PyObject *
_decimal_Decimal_to_integral_impl(PyObject *self, PyObject *rounding,
                                  PyObject *context);

static PyObject *
_decimal_Decimal_to_integral(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    return_value = _decimal_Decimal_to_integral_impl(self, rounding, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_to_integral_exact__doc__,
"to_integral_exact($self, /, rounding=None, context=None)\n"
"--\n"
"\n"
"Round to the nearest integer.\n"
"\n"
"Decimal.to_integral_exact() signals Inexact or Rounded as appropriate if\n"
"rounding occurs.  The rounding mode is determined by the rounding parameter\n"
"if given, else by the given context. If neither parameter is given, then the\n"
"rounding mode of the current default context is used.");

#define _DECIMAL_DECIMAL_TO_INTEGRAL_EXACT_METHODDEF    \
    {"to_integral_exact", _PyCFunction_CAST(_decimal_Decimal_to_integral_exact), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_to_integral_exact__doc__},

static PyObject *
_decimal_Decimal_to_integral_exact_impl(PyObject *self, PyObject *rounding,
                                        PyObject *context);

static PyObject *
_decimal_Decimal_to_integral_exact(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    return_value = _decimal_Decimal_to_integral_exact_impl(self, rounding, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_as_tuple__doc__,
"as_tuple($self, /)\n"
"--\n"
"\n"
"Return a tuple representation of the number.");

#define _DECIMAL_DECIMAL_AS_TUPLE_METHODDEF    \
    {"as_tuple", (PyCFunction)_decimal_Decimal_as_tuple, METH_NOARGS, _decimal_Decimal_as_tuple__doc__},

static PyObject *
_decimal_Decimal_as_tuple_impl(PyObject *self);

static PyObject *
_decimal_Decimal_as_tuple(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_as_tuple_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_exp__doc__,
"exp($self, /, context=None)\n"
"--\n"
"\n"
"Return the value of the (natural) exponential function e**x at the given number.\n"
"\n"
"The function always uses the ROUND_HALF_EVEN mode and the result is correctly\n"
"rounded.");

#define _DECIMAL_DECIMAL_EXP_METHODDEF    \
    {"exp", _PyCFunction_CAST(_decimal_Decimal_exp), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_exp__doc__},

static PyObject *
_decimal_Decimal_exp_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_exp(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_exp_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_ln__doc__,
"ln($self, /, context=None)\n"
"--\n"
"\n"
"Return the natural (base e) logarithm of the operand.\n"
"\n"
"The function always uses the ROUND_HALF_EVEN mode and the result is correctly\n"
"rounded.");

#define _DECIMAL_DECIMAL_LN_METHODDEF    \
    {"ln", _PyCFunction_CAST(_decimal_Decimal_ln), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_ln__doc__},

static PyObject *
_decimal_Decimal_ln_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_ln(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_ln_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_log10__doc__,
"log10($self, /, context=None)\n"
"--\n"
"\n"
"Return the base ten logarithm of the operand.\n"
"\n"
"The function always uses the ROUND_HALF_EVEN mode and the result is correctly\n"
"rounded.");

#define _DECIMAL_DECIMAL_LOG10_METHODDEF    \
    {"log10", _PyCFunction_CAST(_decimal_Decimal_log10), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_log10__doc__},

static PyObject *
_decimal_Decimal_log10_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_log10(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_log10_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_next_minus__doc__,
"next_minus($self, /, context=None)\n"
"--\n"
"\n"
"Returns the largest representable number smaller than itself.");

#define _DECIMAL_DECIMAL_NEXT_MINUS_METHODDEF    \
    {"next_minus", _PyCFunction_CAST(_decimal_Decimal_next_minus), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_next_minus__doc__},

static PyObject *
_decimal_Decimal_next_minus_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_next_minus(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_next_minus_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_next_plus__doc__,
"next_plus($self, /, context=None)\n"
"--\n"
"\n"
"Returns the smallest representable number larger than itself.");

#define _DECIMAL_DECIMAL_NEXT_PLUS_METHODDEF    \
    {"next_plus", _PyCFunction_CAST(_decimal_Decimal_next_plus), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_next_plus__doc__},

static PyObject *
_decimal_Decimal_next_plus_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_next_plus(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_next_plus_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_normalize__doc__,
"normalize($self, /, context=None)\n"
"--\n"
"\n"
"Normalize the number by stripping trailing 0s, change anything equal to 0 to 0e0.\n"
"\n"
"Used for producing canonical values for members of an equivalence class.  For\n"
"example, Decimal(\'32.100\') and Decimal(\'0.321000e+2\') both normalize to the\n"
"equivalent value Decimal(\'32.1\').");

#define _DECIMAL_DECIMAL_NORMALIZE_METHODDEF    \
    {"normalize", _PyCFunction_CAST(_decimal_Decimal_normalize), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_normalize__doc__},

static PyObject *
_decimal_Decimal_normalize_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_normalize(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_normalize_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_sqrt__doc__,
"sqrt($self, /, context=None)\n"
"--\n"
"\n"
"Return the square root of the argument to full precision.\n"
"\n"
"The result is correctly rounded using the ROUND_HALF_EVEN rounding mode.");

#define _DECIMAL_DECIMAL_SQRT_METHODDEF    \
    {"sqrt", _PyCFunction_CAST(_decimal_Decimal_sqrt), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_sqrt__doc__},

static PyObject *
_decimal_Decimal_sqrt_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_sqrt(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_sqrt_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_compare__doc__,
"compare($self, /, other, context=None)\n"
"--\n"
"\n"
"Compare self to other.\n"
"\n"
"Return a decimal value:\n"
"\n"
"    a or b is a NaN ==> Decimal(\'NaN\')\n"
"    a < b           ==> Decimal(\'-1\')\n"
"    a == b          ==> Decimal(\'0\')\n"
"    a > b           ==> Decimal(\'1\')");

#define _DECIMAL_DECIMAL_COMPARE_METHODDEF    \
    {"compare", _PyCFunction_CAST(_decimal_Decimal_compare), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_compare__doc__},

static PyObject *
_decimal_Decimal_compare_impl(PyObject *self, PyObject *other,
                              PyObject *context);

static PyObject *
_decimal_Decimal_compare(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_compare_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_compare_signal__doc__,
"compare_signal($self, /, other, context=None)\n"
"--\n"
"\n"
"Identical to compare, except that all NaNs signal.");

#define _DECIMAL_DECIMAL_COMPARE_SIGNAL_METHODDEF    \
    {"compare_signal", _PyCFunction_CAST(_decimal_Decimal_compare_signal), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_compare_signal__doc__},

static PyObject *
_decimal_Decimal_compare_signal_impl(PyObject *self, PyObject *other,
                                     PyObject *context);

static PyObject *
_decimal_Decimal_compare_signal(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_compare_signal_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_max__doc__,
"max($self, /, other, context=None)\n"
"--\n"
"\n"
"Maximum of self and other.\n"
"\n"
"If one operand is a quiet NaN and the other is numeric, the numeric operand is\n"
"returned.");

#define _DECIMAL_DECIMAL_MAX_METHODDEF    \
    {"max", _PyCFunction_CAST(_decimal_Decimal_max), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_max__doc__},

static PyObject *
_decimal_Decimal_max_impl(PyObject *self, PyObject *other, PyObject *context);

static PyObject *
_decimal_Decimal_max(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_max_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_max_mag__doc__,
"max_mag($self, /, other, context=None)\n"
"--\n"
"\n"
"Similar to the max() method, but the comparison is done using the absolute values of the operands.");

#define _DECIMAL_DECIMAL_MAX_MAG_METHODDEF    \
    {"max_mag", _PyCFunction_CAST(_decimal_Decimal_max_mag), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_max_mag__doc__},

static PyObject *
_decimal_Decimal_max_mag_impl(PyObject *self, PyObject *other,
                              PyObject *context);

static PyObject *
_decimal_Decimal_max_mag(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_max_mag_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_min__doc__,
"min($self, /, other, context=None)\n"
"--\n"
"\n"
"Minimum of self and other.\n"
"\n"
"If one operand is a quiet NaN and the other is numeric, the numeric operand is\n"
"returned.");

#define _DECIMAL_DECIMAL_MIN_METHODDEF    \
    {"min", _PyCFunction_CAST(_decimal_Decimal_min), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_min__doc__},

static PyObject *
_decimal_Decimal_min_impl(PyObject *self, PyObject *other, PyObject *context);

static PyObject *
_decimal_Decimal_min(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_min_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_min_mag__doc__,
"min_mag($self, /, other, context=None)\n"
"--\n"
"\n"
"Similar to the min() method, but the comparison is done using the absolute values of the operands.");

#define _DECIMAL_DECIMAL_MIN_MAG_METHODDEF    \
    {"min_mag", _PyCFunction_CAST(_decimal_Decimal_min_mag), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_min_mag__doc__},

static PyObject *
_decimal_Decimal_min_mag_impl(PyObject *self, PyObject *other,
                              PyObject *context);

static PyObject *
_decimal_Decimal_min_mag(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_min_mag_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_next_toward__doc__,
"next_toward($self, /, other, context=None)\n"
"--\n"
"\n"
"Returns the number closest to self, in the direction towards other.\n"
"\n"
"If the two operands are unequal, return the number closest to the first\n"
"operand in the direction of the second operand.  If both operands are\n"
"numerically equal, return a copy of the first operand with the sign set\n"
"to be the same as the sign of the second operand.");

#define _DECIMAL_DECIMAL_NEXT_TOWARD_METHODDEF    \
    {"next_toward", _PyCFunction_CAST(_decimal_Decimal_next_toward), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_next_toward__doc__},

static PyObject *
_decimal_Decimal_next_toward_impl(PyObject *self, PyObject *other,
                                  PyObject *context);

static PyObject *
_decimal_Decimal_next_toward(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_next_toward_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_remainder_near__doc__,
"remainder_near($self, /, other, context=None)\n"
"--\n"
"\n"
"Return the remainder from dividing self by other.\n"
"\n"
"This differs from self % other in that the sign of the remainder is chosen so\n"
"as to minimize its absolute value. More precisely, the return value is self -\n"
"n * other where n is the integer nearest to the exact value of self / other,\n"
"and if two integers are equally near then the even one is chosen.\n"
"\n"
"If the result is zero then its sign will be the sign of self.");

#define _DECIMAL_DECIMAL_REMAINDER_NEAR_METHODDEF    \
    {"remainder_near", _PyCFunction_CAST(_decimal_Decimal_remainder_near), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_remainder_near__doc__},

static PyObject *
_decimal_Decimal_remainder_near_impl(PyObject *self, PyObject *other,
                                     PyObject *context);

static PyObject *
_decimal_Decimal_remainder_near(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_remainder_near_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_adjusted__doc__,
"adjusted($self, /)\n"
"--\n"
"\n"
"Return the adjusted exponent of the number.  Defined as exp + digits - 1.");

#define _DECIMAL_DECIMAL_ADJUSTED_METHODDEF    \
    {"adjusted", (PyCFunction)_decimal_Decimal_adjusted, METH_NOARGS, _decimal_Decimal_adjusted__doc__},

static PyObject *
_decimal_Decimal_adjusted_impl(PyObject *self);

static PyObject *
_decimal_Decimal_adjusted(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_adjusted_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_canonical__doc__,
"canonical($self, /)\n"
"--\n"
"\n"
"Return the canonical encoding of the argument.\n"
"\n"
"Currently, the encoding of a Decimal instance is always canonical, so this\n"
"operation returns its argument unchanged.");

#define _DECIMAL_DECIMAL_CANONICAL_METHODDEF    \
    {"canonical", (PyCFunction)_decimal_Decimal_canonical, METH_NOARGS, _decimal_Decimal_canonical__doc__},

static PyObject *
_decimal_Decimal_canonical_impl(PyObject *self);

static PyObject *
_decimal_Decimal_canonical(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_canonical_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return self.");

#define _DECIMAL_DECIMAL_CONJUGATE_METHODDEF    \
    {"conjugate", (PyCFunction)_decimal_Decimal_conjugate, METH_NOARGS, _decimal_Decimal_conjugate__doc__},

static PyObject *
_decimal_Decimal_conjugate_impl(PyObject *self);

static PyObject *
_decimal_Decimal_conjugate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_conjugate_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_radix__doc__,
"radix($self, /)\n"
"--\n"
"\n"
"Return Decimal(10).\n"
"\n"
"This is the radix (base) in which the Decimal class does\n"
"all its arithmetic. Included for compatibility with the specification.");

#define _DECIMAL_DECIMAL_RADIX_METHODDEF    \
    {"radix", (PyCFunction)_decimal_Decimal_radix, METH_NOARGS, _decimal_Decimal_radix__doc__},

static PyObject *
_decimal_Decimal_radix_impl(PyObject *self);

static PyObject *
_decimal_Decimal_radix(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_radix_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_copy_abs__doc__,
"copy_abs($self, /)\n"
"--\n"
"\n"
"Return the absolute value of the argument.\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are changed and\n"
"no rounding is performed.");

#define _DECIMAL_DECIMAL_COPY_ABS_METHODDEF    \
    {"copy_abs", (PyCFunction)_decimal_Decimal_copy_abs, METH_NOARGS, _decimal_Decimal_copy_abs__doc__},

static PyObject *
_decimal_Decimal_copy_abs_impl(PyObject *self);

static PyObject *
_decimal_Decimal_copy_abs(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_copy_abs_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_copy_negate__doc__,
"copy_negate($self, /)\n"
"--\n"
"\n"
"Return the negation of the argument.\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are changed and\n"
"no rounding is performed.");

#define _DECIMAL_DECIMAL_COPY_NEGATE_METHODDEF    \
    {"copy_negate", (PyCFunction)_decimal_Decimal_copy_negate, METH_NOARGS, _decimal_Decimal_copy_negate__doc__},

static PyObject *
_decimal_Decimal_copy_negate_impl(PyObject *self);

static PyObject *
_decimal_Decimal_copy_negate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_copy_negate_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_logical_invert__doc__,
"logical_invert($self, /, context=None)\n"
"--\n"
"\n"
"Return the digit-wise inversion of the (logical) operand.");

#define _DECIMAL_DECIMAL_LOGICAL_INVERT_METHODDEF    \
    {"logical_invert", _PyCFunction_CAST(_decimal_Decimal_logical_invert), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_logical_invert__doc__},

static PyObject *
_decimal_Decimal_logical_invert_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_logical_invert(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_logical_invert_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_logb__doc__,
"logb($self, /, context=None)\n"
"--\n"
"\n"
"Return the adjusted exponent of the operand as a Decimal instance.\n"
"\n"
"If the operand is a zero, then Decimal(\'-Infinity\') is returned and the\n"
"DivisionByZero condition is raised. If the operand is an infinity then\n"
"Decimal(\'Infinity\') is returned.");

#define _DECIMAL_DECIMAL_LOGB_METHODDEF    \
    {"logb", _PyCFunction_CAST(_decimal_Decimal_logb), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_logb__doc__},

static PyObject *
_decimal_Decimal_logb_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_logb(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_logb_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_number_class__doc__,
"number_class($self, /, context=None)\n"
"--\n"
"\n"
"Return a string describing the class of the operand.\n"
"\n"
"The returned value is one of the following ten strings:\n"
"\n"
"    * \'-Infinity\', indicating that the operand is negative infinity.\n"
"    * \'-Normal\', indicating that the operand is a negative normal number.\n"
"    * \'-Subnormal\', indicating that the operand is negative and subnormal.\n"
"    * \'-Zero\', indicating that the operand is a negative zero.\n"
"    * \'+Zero\', indicating that the operand is a positive zero.\n"
"    * \'+Subnormal\', indicating that the operand is positive and subnormal.\n"
"    * \'+Normal\', indicating that the operand is a positive normal number.\n"
"    * \'+Infinity\', indicating that the operand is positive infinity.\n"
"    * \'NaN\', indicating that the operand is a quiet NaN (Not a Number).\n"
"    * \'sNaN\', indicating that the operand is a signaling NaN.");

#define _DECIMAL_DECIMAL_NUMBER_CLASS_METHODDEF    \
    {"number_class", _PyCFunction_CAST(_decimal_Decimal_number_class), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_number_class__doc__},

static PyObject *
_decimal_Decimal_number_class_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_number_class(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_number_class_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_to_eng_string__doc__,
"to_eng_string($self, /, context=None)\n"
"--\n"
"\n"
"Convert to an engineering-type string.\n"
"\n"
"Engineering notation has an exponent which is a multiple of 3, so there are up\n"
"to 3 digits left of the decimal place. For example, Decimal(\'123E+1\') is\n"
"converted to Decimal(\'1.23E+3\').\n"
"\n"
"The value of context.capitals determines whether the exponent sign is lower\n"
"or upper case. Otherwise, the context does not affect the operation.");

#define _DECIMAL_DECIMAL_TO_ENG_STRING_METHODDEF    \
    {"to_eng_string", _PyCFunction_CAST(_decimal_Decimal_to_eng_string), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_to_eng_string__doc__},

static PyObject *
_decimal_Decimal_to_eng_string_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_to_eng_string(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[0];
skip_optional_pos:
    return_value = _decimal_Decimal_to_eng_string_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_compare_total__doc__,
"compare_total($self, /, other, context=None)\n"
"--\n"
"\n"
"Compare two operands using their abstract representation rather than their numerical value.\n"
"\n"
"Similar to the compare() method, but the result\n"
"gives a total ordering on Decimal instances.  Two Decimal instances with\n"
"the same numeric value but different representations compare unequal\n"
"in this ordering:\n"
"\n"
"    >>> Decimal(\'12.0\').compare_total(Decimal(\'12\'))\n"
"    Decimal(\'-1\')\n"
"\n"
"Quiet and signaling NaNs are also included in the total ordering. The result\n"
"of this function is Decimal(\'0\') if both operands have the same representation,\n"
"Decimal(\'-1\') if the first operand is lower in the total order than the second,\n"
"and Decimal(\'1\') if the first operand is higher in the total order than the\n"
"second operand. See the specification for details of the total order.\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are changed\n"
"and no rounding is performed. As an exception, the C version may raise\n"
"InvalidOperation if the second operand cannot be converted exactly.");

#define _DECIMAL_DECIMAL_COMPARE_TOTAL_METHODDEF    \
    {"compare_total", _PyCFunction_CAST(_decimal_Decimal_compare_total), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_compare_total__doc__},

static PyObject *
_decimal_Decimal_compare_total_impl(PyObject *self, PyObject *other,
                                    PyObject *context);

static PyObject *
_decimal_Decimal_compare_total(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_compare_total_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_compare_total_mag__doc__,
"compare_total_mag($self, /, other, context=None)\n"
"--\n"
"\n"
"Compare two operands using their abstract representation rather than their value as in compare_total(), but ignoring the sign of each operand.\n"
"\n"
"x.compare_total_mag(y) is equivalent to x.copy_abs().compare_total(y.copy_abs()).\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are changed\n"
"and no rounding is performed. As an exception, the C version may raise\n"
"InvalidOperation if the second operand cannot be converted exactly.");

#define _DECIMAL_DECIMAL_COMPARE_TOTAL_MAG_METHODDEF    \
    {"compare_total_mag", _PyCFunction_CAST(_decimal_Decimal_compare_total_mag), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_compare_total_mag__doc__},

static PyObject *
_decimal_Decimal_compare_total_mag_impl(PyObject *self, PyObject *other,
                                        PyObject *context);

static PyObject *
_decimal_Decimal_compare_total_mag(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_compare_total_mag_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_copy_sign__doc__,
"copy_sign($self, /, other, context=None)\n"
"--\n"
"\n"
"Return a copy of *self* with the sign set to be the same as the sign of *other*.\n"
"\n"
"For example:\n"
"\n"
"    >>> Decimal(\'2.3\').copy_sign(Decimal(\'-1.5\'))\n"
"    Decimal(\'-2.3\')\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are changed\n"
"and no rounding is performed. As an exception, the C version may raise\n"
"InvalidOperation if the second operand cannot be converted exactly.");

#define _DECIMAL_DECIMAL_COPY_SIGN_METHODDEF    \
    {"copy_sign", _PyCFunction_CAST(_decimal_Decimal_copy_sign), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_copy_sign__doc__},

static PyObject *
_decimal_Decimal_copy_sign_impl(PyObject *self, PyObject *other,
                                PyObject *context);

static PyObject *
_decimal_Decimal_copy_sign(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_copy_sign_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_same_quantum__doc__,
"same_quantum($self, /, other, context=None)\n"
"--\n"
"\n"
"Test whether self and other have the same exponent or whether both are NaN.\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are changed\n"
"and no rounding is performed. As an exception, the C version may raise\n"
"InvalidOperation if the second operand cannot be converted exactly.");

#define _DECIMAL_DECIMAL_SAME_QUANTUM_METHODDEF    \
    {"same_quantum", _PyCFunction_CAST(_decimal_Decimal_same_quantum), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_same_quantum__doc__},

static PyObject *
_decimal_Decimal_same_quantum_impl(PyObject *self, PyObject *other,
                                   PyObject *context);

static PyObject *
_decimal_Decimal_same_quantum(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_same_quantum_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_logical_and__doc__,
"logical_and($self, /, other, context=None)\n"
"--\n"
"\n"
"Return the digit-wise \'and\' of the two (logical) operands.");

#define _DECIMAL_DECIMAL_LOGICAL_AND_METHODDEF    \
    {"logical_and", _PyCFunction_CAST(_decimal_Decimal_logical_and), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_logical_and__doc__},

static PyObject *
_decimal_Decimal_logical_and_impl(PyObject *self, PyObject *other,
                                  PyObject *context);

static PyObject *
_decimal_Decimal_logical_and(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_logical_and_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_logical_or__doc__,
"logical_or($self, /, other, context=None)\n"
"--\n"
"\n"
"Return the digit-wise \'or\' of the two (logical) operands.");

#define _DECIMAL_DECIMAL_LOGICAL_OR_METHODDEF    \
    {"logical_or", _PyCFunction_CAST(_decimal_Decimal_logical_or), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_logical_or__doc__},

static PyObject *
_decimal_Decimal_logical_or_impl(PyObject *self, PyObject *other,
                                 PyObject *context);

static PyObject *
_decimal_Decimal_logical_or(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_logical_or_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_logical_xor__doc__,
"logical_xor($self, /, other, context=None)\n"
"--\n"
"\n"
"Return the digit-wise \'xor\' of the two (logical) operands.");

#define _DECIMAL_DECIMAL_LOGICAL_XOR_METHODDEF    \
    {"logical_xor", _PyCFunction_CAST(_decimal_Decimal_logical_xor), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_logical_xor__doc__},

static PyObject *
_decimal_Decimal_logical_xor_impl(PyObject *self, PyObject *other,
                                  PyObject *context);

static PyObject *
_decimal_Decimal_logical_xor(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_logical_xor_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_rotate__doc__,
"rotate($self, /, other, context=None)\n"
"--\n"
"\n"
"Return the result of rotating the digits of the first operand by an amount specified by the second operand.\n"
"\n"
"The second operand must be an integer in the range -precision through\n"
"precision. The absolute value of the second operand gives the number of places\n"
"to rotate. If the second operand is positive then rotation is to the left;\n"
"otherwise rotation is to the right.  The coefficient of the first operand is\n"
"padded on the left with zeros to length precision if necessary. The sign and\n"
"exponent of the first operand are unchanged.");

#define _DECIMAL_DECIMAL_ROTATE_METHODDEF    \
    {"rotate", _PyCFunction_CAST(_decimal_Decimal_rotate), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_rotate__doc__},

static PyObject *
_decimal_Decimal_rotate_impl(PyObject *self, PyObject *other,
                             PyObject *context);

static PyObject *
_decimal_Decimal_rotate(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_rotate_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_scaleb__doc__,
"scaleb($self, /, other, context=None)\n"
"--\n"
"\n"
"Return the first operand with the exponent adjusted the second.\n"
"\n"
"Equivalently, return the first operand multiplied by 10**other. The second\n"
"operand must be an integer.");

#define _DECIMAL_DECIMAL_SCALEB_METHODDEF    \
    {"scaleb", _PyCFunction_CAST(_decimal_Decimal_scaleb), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_scaleb__doc__},

static PyObject *
_decimal_Decimal_scaleb_impl(PyObject *self, PyObject *other,
                             PyObject *context);

static PyObject *
_decimal_Decimal_scaleb(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_scaleb_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_shift__doc__,
"shift($self, /, other, context=None)\n"
"--\n"
"\n"
"Return the result of shifting the digits of the first operand by an amount specified by the second operand.\n"
"\n"
"The second operand must be an integer in the range -precision through\n"
"precision. The absolute value of the second operand gives the number of places\n"
"to shift. If the second operand is positive, then the shift is to the left;\n"
"otherwise the shift is to the right. Digits shifted into the coefficient are\n"
"zeros. The sign and exponent of the first operand are unchanged.");

#define _DECIMAL_DECIMAL_SHIFT_METHODDEF    \
    {"shift", _PyCFunction_CAST(_decimal_Decimal_shift), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_shift__doc__},

static PyObject *
_decimal_Decimal_shift_impl(PyObject *self, PyObject *other,
                            PyObject *context);

static PyObject *
_decimal_Decimal_shift(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    other = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    context = args[1];
skip_optional_pos:
    return_value = _decimal_Decimal_shift_impl(self, other, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_quantize__doc__,
"quantize($self, /, exp, rounding=None, context=None)\n"
"--\n"
"\n"
"Return a value equal to *self* after rounding, with the exponent of *other*.\n"
"\n"
"    >>> Decimal(\'1.41421356\').quantize(Decimal(\'1.000\'))\n"
"    Decimal(\'1.414\')\n"
"\n"
"Unlike other operations, if the length of the coefficient after the quantize\n"
"operation would be greater than precision, then an InvalidOperation is signaled.\n"
"This guarantees that, unless there is an error condition, the quantized exponent\n"
"is always equal to that of the right-hand operand.\n"
"\n"
"Also unlike other operations, quantize never signals Underflow, even if the\n"
"result is subnormal and inexact.\n"
"\n"
"If the exponent of the second operand is larger than that of the first, then\n"
"rounding may be necessary. In this case, the rounding mode is determined by the\n"
"rounding argument if given, else by the given context argument; if neither\n"
"argument is given, the rounding mode of the current thread\'s context is used.");

#define _DECIMAL_DECIMAL_QUANTIZE_METHODDEF    \
    {"quantize", _PyCFunction_CAST(_decimal_Decimal_quantize), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_quantize__doc__},

static PyObject *
_decimal_Decimal_quantize_impl(PyObject *self, PyObject *w,
                               PyObject *rounding, PyObject *context);

static PyObject *
_decimal_Decimal_quantize(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    return_value = _decimal_Decimal_quantize_impl(self, w, rounding, context);

exit:
    return return_value;
}
/*[clinic end generated code: output=a72c95123d7107e1 input=a9049054013a1b77]*/
