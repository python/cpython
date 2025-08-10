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
/*[clinic end generated code: output=304ce135eee594cc input=a9049054013a1b77]*/
