/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_decimal_Context_Etiny__doc__,
"Etiny($self, /)\n"
"--\n"
"\n"
"Return a value equal to Emin - prec + 1.\n"
"\n"
"This is the minimum exponent value for subnormal results.  When\n"
"underflow occurs, the exponent is set to Etiny.");

#define _DECIMAL_CONTEXT_ETINY_METHODDEF    \
    {"Etiny", (PyCFunction)_decimal_Context_Etiny, METH_NOARGS, _decimal_Context_Etiny__doc__},

static PyObject *
_decimal_Context_Etiny_impl(PyObject *self);

static PyObject *
_decimal_Context_Etiny(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Context_Etiny_impl(self);
}

PyDoc_STRVAR(_decimal_Context_Etop__doc__,
"Etop($self, /)\n"
"--\n"
"\n"
"Return a value equal to Emax - prec + 1.\n"
"\n"
"This is the maximum exponent if the _clamp field of the context is set\n"
"to 1 (IEEE clamp mode).  Etop() must not be negative.");

#define _DECIMAL_CONTEXT_ETOP_METHODDEF    \
    {"Etop", (PyCFunction)_decimal_Context_Etop, METH_NOARGS, _decimal_Context_Etop__doc__},

static PyObject *
_decimal_Context_Etop_impl(PyObject *self);

static PyObject *
_decimal_Context_Etop(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Context_Etop_impl(self);
}

PyDoc_STRVAR(_decimal_IEEEContext__doc__,
"IEEEContext($module, bits, /)\n"
"--\n"
"\n"
"Return a context, initialized as one of the IEEE interchange formats.\n"
"\n"
"The argument must be a multiple of 32 and less than\n"
"IEEE_CONTEXT_MAX_BITS.");

#define _DECIMAL_IEEECONTEXT_METHODDEF    \
    {"IEEEContext", (PyCFunction)_decimal_IEEEContext, METH_O, _decimal_IEEEContext__doc__},

static PyObject *
_decimal_IEEEContext_impl(PyObject *module, Py_ssize_t bits);

static PyObject *
_decimal_IEEEContext(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t bits;

    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        bits = ival;
    }
    return_value = _decimal_IEEEContext_impl(module, bits);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_getcontext__doc__,
"getcontext($module, /)\n"
"--\n"
"\n"
"Get the current default context.");

#define _DECIMAL_GETCONTEXT_METHODDEF    \
    {"getcontext", (PyCFunction)_decimal_getcontext, METH_NOARGS, _decimal_getcontext__doc__},

static PyObject *
_decimal_getcontext_impl(PyObject *module);

static PyObject *
_decimal_getcontext(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _decimal_getcontext_impl(module);
}

PyDoc_STRVAR(_decimal_setcontext__doc__,
"setcontext($module, context, /)\n"
"--\n"
"\n"
"Set a new default context.");

#define _DECIMAL_SETCONTEXT_METHODDEF    \
    {"setcontext", (PyCFunction)_decimal_setcontext, METH_O, _decimal_setcontext__doc__},

PyDoc_STRVAR(_decimal_localcontext__doc__,
"localcontext($module, /, ctx=None, **kwargs)\n"
"--\n"
"\n"
"Return a context manager for a copy of the supplied context.\n"
"\n"
"That will set the default context to a copy of ctx on entry to the\n"
"with-statement and restore the previous default context when exiting\n"
"the with-statement. If no context is specified, a copy of the current\n"
"default context is used.");

#define _DECIMAL_LOCALCONTEXT_METHODDEF    \
    {"localcontext", _PyCFunction_CAST(_decimal_localcontext), METH_FASTCALL|METH_KEYWORDS, _decimal_localcontext__doc__},

static PyObject *
_decimal_localcontext_impl(PyObject *module, PyObject *local, PyObject *prec,
                           PyObject *rounding, PyObject *Emin,
                           PyObject *Emax, PyObject *capitals,
                           PyObject *clamp, PyObject *flags, PyObject *traps);

static PyObject *
_decimal_localcontext(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(ctx), &_Py_ID(prec), &_Py_ID(rounding), &_Py_ID(Emin), &_Py_ID(Emax), &_Py_ID(capitals), &_Py_ID(clamp), &_Py_ID(flags), &_Py_ID(traps), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"ctx", "prec", "rounding", "Emin", "Emax", "capitals", "clamp", "flags", "traps", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "localcontext",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[9];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *local = Py_None;
    PyObject *prec = Py_None;
    PyObject *rounding = Py_None;
    PyObject *Emin = Py_None;
    PyObject *Emax = Py_None;
    PyObject *capitals = Py_None;
    PyObject *clamp = Py_None;
    PyObject *flags = Py_None;
    PyObject *traps = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        local = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        prec = args[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        rounding = args[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        Emin = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[4]) {
        Emax = args[4];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[5]) {
        capitals = args[5];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[6]) {
        clamp = args[6];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[7]) {
        flags = args[7];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    traps = args[8];
skip_optional_kwonly:
    return_value = _decimal_localcontext_impl(module, local, prec, rounding, Emin, Emax, capitals, clamp, flags, traps);

exit:
    return return_value;
}

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

PyDoc_STRVAR(dec_new__doc__,
"Decimal(value=\'0\', context=None)\n"
"--\n"
"\n"
"Construct a new Decimal object.\n"
"\n"
"value can be an integer, string, tuple, or another Decimal object.  If\n"
"no value is given, return Decimal(\'0\'). The context does not affect\n"
"the conversion and is only passed to determine if the InvalidOperation\n"
"trap is active.");

static PyObject *
dec_new_impl(PyTypeObject *type, PyObject *value, PyObject *context);

static PyObject *
dec_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(value), &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"value", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Decimal",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *value = NULL;
    PyObject *context = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        value = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    context = fastargs[1];
skip_optional_pos:
    return_value = dec_new_impl(type, value, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal___format____doc__,
"__format__($self, format_spec, override=<unrepresentable>, /)\n"
"--\n"
"\n"
"Formats the Decimal according to format_spec.");

#define _DECIMAL_DECIMAL___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(_decimal_Decimal___format__), METH_FASTCALL, _decimal_Decimal___format____doc__},

static PyObject *
_decimal_Decimal___format___impl(PyObject *dec, PyObject *fmtarg,
                                 PyObject *override);

static PyObject *
_decimal_Decimal___format__(PyObject *dec, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *fmtarg;
    PyObject *override = NULL;

    if (!_PyArg_CheckPositional("__format__", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("__format__", "argument 1", "str", args[0]);
        goto exit;
    }
    fmtarg = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    override = args[1];
skip_optional:
    return_value = _decimal_Decimal___format___impl(dec, fmtarg, override);

exit:
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
"The rounding mode is determined by the rounding parameter if given,\n"
"else by the given context. If neither parameter is given, then the\n"
"rounding mode of the current default context is used.");

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
"The to_integral() name has been kept for compatibility with older\n"
"versions.");

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
"Decimal.to_integral_exact() signals Inexact or Rounded as appropriate\n"
"if rounding occurs.  The rounding mode is determined by the rounding\n"
"parameter if given, else by the given context. If neither parameter is\n"
"given, then the rounding mode of the current default context is used.");

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

PyDoc_STRVAR(_decimal_Decimal___round____doc__,
"__round__($self, ndigits=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return the Integral closest to self, rounding half toward even.");

#define _DECIMAL_DECIMAL___ROUND___METHODDEF    \
    {"__round__", _PyCFunction_CAST(_decimal_Decimal___round__), METH_FASTCALL, _decimal_Decimal___round____doc__},

static PyObject *
_decimal_Decimal___round___impl(PyObject *self, PyObject *ndigits);

static PyObject *
_decimal_Decimal___round__(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ndigits = NULL;

    if (!_PyArg_CheckPositional("__round__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ndigits = args[0];
skip_optional:
    return_value = _decimal_Decimal___round___impl(self, ndigits);

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
"Return the value of the (natural) exponential function e**x.\n"
"\n"
"The function always uses the ROUND_HALF_EVEN mode and the result is\n"
"correctly rounded.");

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
"The function always uses the ROUND_HALF_EVEN mode and the result is\n"
"correctly rounded.");

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
"The function always uses the ROUND_HALF_EVEN mode and the result is\n"
"correctly rounded.");

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
"Normalize the number by stripping trailing 0s\n"
"\n"
"This also change anything equal to 0 to 0e0.  Used for producing\n"
"canonical values for members of an equivalence class.  For example,\n"
"Decimal(\'32.100\') and Decimal(\'0.321000e+2\') both normalize to\n"
"the equivalent value Decimal(\'32.1\').");

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
"If one operand is a quiet NaN and the other is numeric, the numeric\n"
"operand is returned.");

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
"As the max() method, but compares the absolute values of the operands.");

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
"If one operand is a quiet NaN and the other is numeric, the numeric\n"
"operand is returned.");

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
"As the min() method, but compares the absolute values of the operands.");

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
"This differs from self % other in that the sign of the remainder is\n"
"chosen so as to minimize its absolute value. More precisely, the return\n"
"value is self - n * other where n is the integer nearest to the exact\n"
"value of self / other, and if two integers are equally near then the\n"
"even one is chosen.\n"
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

PyDoc_STRVAR(_decimal_Decimal_fma__doc__,
"fma($self, /, other, third, context=None)\n"
"--\n"
"\n"
"Fused multiply-add.\n"
"\n"
"Return self*other+third with no rounding of the intermediate product\n"
"self*other.\n"
"\n"
"    >>> Decimal(2).fma(3, 5)\n"
"    Decimal(\'11\')");

#define _DECIMAL_DECIMAL_FMA_METHODDEF    \
    {"fma", _PyCFunction_CAST(_decimal_Decimal_fma), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_fma__doc__},

static PyObject *
_decimal_Decimal_fma_impl(PyObject *self, PyObject *other, PyObject *third,
                          PyObject *context);

static PyObject *
_decimal_Decimal_fma(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    return_value = _decimal_Decimal_fma_impl(self, other, third, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_is_canonical__doc__,
"is_canonical($self, /)\n"
"--\n"
"\n"
"Return True if the argument is canonical and False otherwise.\n"
"\n"
"Currently, a Decimal instance is always canonical, so this operation\n"
"always returns True.");

#define _DECIMAL_DECIMAL_IS_CANONICAL_METHODDEF    \
    {"is_canonical", (PyCFunction)_decimal_Decimal_is_canonical, METH_NOARGS, _decimal_Decimal_is_canonical__doc__},

static PyObject *
_decimal_Decimal_is_canonical_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_canonical(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_canonical_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_finite__doc__,
"is_finite($self, /)\n"
"--\n"
"\n"
"Return True if the argument is a finite number, and False otherwise.");

#define _DECIMAL_DECIMAL_IS_FINITE_METHODDEF    \
    {"is_finite", (PyCFunction)_decimal_Decimal_is_finite, METH_NOARGS, _decimal_Decimal_is_finite__doc__},

static PyObject *
_decimal_Decimal_is_finite_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_finite(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_finite_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_infinite__doc__,
"is_infinite($self, /)\n"
"--\n"
"\n"
"Return True if the argument is infinite, and False otherwise.");

#define _DECIMAL_DECIMAL_IS_INFINITE_METHODDEF    \
    {"is_infinite", (PyCFunction)_decimal_Decimal_is_infinite, METH_NOARGS, _decimal_Decimal_is_infinite__doc__},

static PyObject *
_decimal_Decimal_is_infinite_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_infinite(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_infinite_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_nan__doc__,
"is_nan($self, /)\n"
"--\n"
"\n"
"Return True if the argument is a (quiet or signaling) NaN, else False.");

#define _DECIMAL_DECIMAL_IS_NAN_METHODDEF    \
    {"is_nan", (PyCFunction)_decimal_Decimal_is_nan, METH_NOARGS, _decimal_Decimal_is_nan__doc__},

static PyObject *
_decimal_Decimal_is_nan_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_nan(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_nan_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_qnan__doc__,
"is_qnan($self, /)\n"
"--\n"
"\n"
"Return True if the argument is a quiet NaN, and False otherwise.");

#define _DECIMAL_DECIMAL_IS_QNAN_METHODDEF    \
    {"is_qnan", (PyCFunction)_decimal_Decimal_is_qnan, METH_NOARGS, _decimal_Decimal_is_qnan__doc__},

static PyObject *
_decimal_Decimal_is_qnan_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_qnan(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_qnan_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_snan__doc__,
"is_snan($self, /)\n"
"--\n"
"\n"
"Return True if the argument is a signaling NaN and False otherwise.");

#define _DECIMAL_DECIMAL_IS_SNAN_METHODDEF    \
    {"is_snan", (PyCFunction)_decimal_Decimal_is_snan, METH_NOARGS, _decimal_Decimal_is_snan__doc__},

static PyObject *
_decimal_Decimal_is_snan_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_snan(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_snan_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_signed__doc__,
"is_signed($self, /)\n"
"--\n"
"\n"
"Return True if the argument has a negative sign and False otherwise.\n"
"\n"
"Note that both zeros and NaNs can carry signs.");

#define _DECIMAL_DECIMAL_IS_SIGNED_METHODDEF    \
    {"is_signed", (PyCFunction)_decimal_Decimal_is_signed, METH_NOARGS, _decimal_Decimal_is_signed__doc__},

static PyObject *
_decimal_Decimal_is_signed_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_signed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_signed_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_zero__doc__,
"is_zero($self, /)\n"
"--\n"
"\n"
"Return True if the argument is a zero and False otherwise.");

#define _DECIMAL_DECIMAL_IS_ZERO_METHODDEF    \
    {"is_zero", (PyCFunction)_decimal_Decimal_is_zero, METH_NOARGS, _decimal_Decimal_is_zero__doc__},

static PyObject *
_decimal_Decimal_is_zero_impl(PyObject *self);

static PyObject *
_decimal_Decimal_is_zero(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal_is_zero_impl(self);
}

PyDoc_STRVAR(_decimal_Decimal_is_normal__doc__,
"is_normal($self, /, context=None)\n"
"--\n"
"\n"
"Return True if the argument is a normal number and False otherwise.\n"
"\n"
"Normal number is a finite nonzero number, which is not subnormal.");

#define _DECIMAL_DECIMAL_IS_NORMAL_METHODDEF    \
    {"is_normal", _PyCFunction_CAST(_decimal_Decimal_is_normal), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_is_normal__doc__},

static PyObject *
_decimal_Decimal_is_normal_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_is_normal(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "is_normal",
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
    return_value = _decimal_Decimal_is_normal_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_is_subnormal__doc__,
"is_subnormal($self, /, context=None)\n"
"--\n"
"\n"
"Return True if the argument is subnormal, and False otherwise.\n"
"\n"
"A number is subnormal if it is non-zero, finite, and has an adjusted\n"
"exponent less than Emin.");

#define _DECIMAL_DECIMAL_IS_SUBNORMAL_METHODDEF    \
    {"is_subnormal", _PyCFunction_CAST(_decimal_Decimal_is_subnormal), METH_FASTCALL|METH_KEYWORDS, _decimal_Decimal_is_subnormal__doc__},

static PyObject *
_decimal_Decimal_is_subnormal_impl(PyObject *self, PyObject *context);

static PyObject *
_decimal_Decimal_is_subnormal(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "is_subnormal",
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
    return_value = _decimal_Decimal_is_subnormal_impl(self, context);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Decimal_adjusted__doc__,
"adjusted($self, /)\n"
"--\n"
"\n"
"Return the adjusted exponent (exp + digits - 1) of the number.");

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
"Currently, the encoding of a Decimal instance is always canonical,\n"
"so this operation returns its argument unchanged.");

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
"This operation is unaffected by context and is quiet: no flags are\n"
"changed and no rounding is performed.");

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
"This operation is unaffected by context and is quiet: no flags are\n"
"changed and no rounding is performed.");

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
"    * \'-Normal\', indicating that the operand is a negative normal\n"
"      number.\n"
"    * \'-Subnormal\', indicating that the operand is negative and\n"
"      subnormal.\n"
"    * \'-Zero\', indicating that the operand is a negative zero.\n"
"    * \'+Zero\', indicating that the operand is a positive zero.\n"
"    * \'+Subnormal\', indicating that the operand is positive and\n"
"      subnormal.\n"
"    * \'+Normal\', indicating that the operand is a positive normal\n"
"      number.\n"
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
"Engineering notation has an exponent which is a multiple of 3, so there\n"
"are up to 3 digits left of the decimal place. For example,\n"
"Decimal(\'123E+1\') is converted to Decimal(\'1.23E+3\').\n"
"\n"
"The value of context.capitals determines whether the exponent sign is\n"
"lower or upper case. Otherwise, the context does not affect the\n"
"operation.");

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
"Compare two operands using their abstract representation.\n"
"\n"
"Similar to the compare() method, but the result\n"
"gives a total ordering on Decimal instances.  Two Decimal instances with\n"
"the same numeric value but different representations compare unequal\n"
"in this ordering:\n"
"\n"
"    >>> Decimal(\'12.0\').compare_total(Decimal(\'12\'))\n"
"    Decimal(\'-1\')\n"
"\n"
"Quiet and signaling NaNs are also included in the total ordering. The\n"
"result of this function is Decimal(\'0\') if both operands have the same\n"
"representation, Decimal(\'-1\') if the first operand is lower in the\n"
"total order than the second, and Decimal(\'1\') if the first operand is\n"
"higher in the total order than the second operand. See the\n"
"specification for details of the total order.\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are\n"
"changed and no rounding is performed. As an exception, the C version\n"
"may raise InvalidOperation if the second operand cannot be converted\n"
"exactly.");

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
"As compare_total(), but ignores the sign of each operand.\n"
"\n"
"x.compare_total_mag(y) is equivalent to\n"
"x.copy_abs().compare_total(y.copy_abs()).\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are\n"
"changed and no rounding is performed. As an exception, the C version\n"
"may raise InvalidOperation if the second operand cannot be converted\n"
"exactly.");

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
"Return a copy of *self* with the sign of *other*.\n"
"\n"
"For example:\n"
"\n"
"    >>> Decimal(\'2.3\').copy_sign(Decimal(\'-1.5\'))\n"
"    Decimal(\'-2.3\')\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are\n"
"changed and no rounding is performed. As an exception, the C version\n"
"may raise InvalidOperation if the second operand cannot be converted\n"
"exactly.");

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
"Test whether self and other have the same exponent or both are NaN.\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are\n"
"changed and no rounding is performed. As an exception, the C version\n"
"may raise InvalidOperation if the second operand cannot be converted\n"
"exactly.");

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
"Returns a rotated copy of self\'s digits, value-of-other times.\n"
"\n"
"The second operand must be an integer in the range -precision through\n"
"precision. The absolute value of the second operand gives the number of\n"
"places to rotate. If the second operand is positive then rotation is to\n"
"the left; otherwise rotation is to the right.  The coefficient of the\n"
"first operand is padded on the left with zeros to length precision if\n"
"necessary. The sign and exponent of the first operand are unchanged.");

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
"Equivalently, return the first operand multiplied by 10**other. The\n"
"second operand must be an integer.");

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
"Returns a shifted copy of self\'s digits, value-of-other times.\n"
"\n"
"The second operand must be an integer in the range -precision through\n"
"precision. The absolute value of the second operand gives the number\n"
"of places to shift. If the second operand is positive, then the shift\n"
"is to the left; otherwise the shift is to the right. Digits shifted\n"
"into the coefficient are zeros. The sign and exponent of the first\n"
"operand are unchanged.");

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
"Quantize *self* so its exponent is the same as that of *exp*.\n"
"\n"
"Return a value equal to *self* after rounding, with the exponent\n"
"of *exp*.\n"
"\n"
"    >>> Decimal(\'1.41421356\').quantize(Decimal(\'1.000\'))\n"
"    Decimal(\'1.414\')\n"
"\n"
"Unlike other operations, if the length of the coefficient after the\n"
"quantize operation would be greater than precision, then an\n"
"InvalidOperation is signaled.  This guarantees that, unless there\n"
"is an error condition, the quantized exponent is always equal to\n"
"that of the right-hand operand.\n"
"\n"
"Also unlike other operations, quantize never signals Underflow, even\n"
"if the result is subnormal and inexact.\n"
"\n"
"If the exponent of the second operand is larger than that of the first,\n"
"then rounding may be necessary. In this case, the rounding mode is\n"
"determined by the rounding argument if given, else by the given context\n"
"argument; if neither argument is given, the rounding mode of the\n"
"current thread\'s context is used.");

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

PyDoc_STRVAR(_decimal_Decimal___ceil____doc__,
"__ceil__($self, /)\n"
"--\n"
"\n"
"Return the ceiling as an Integral.");

#define _DECIMAL_DECIMAL___CEIL___METHODDEF    \
    {"__ceil__", (PyCFunction)_decimal_Decimal___ceil__, METH_NOARGS, _decimal_Decimal___ceil____doc__},

static PyObject *
_decimal_Decimal___ceil___impl(PyObject *self);

static PyObject *
_decimal_Decimal___ceil__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal___ceil___impl(self);
}

PyDoc_STRVAR(_decimal_Decimal___complex____doc__,
"__complex__($self, /)\n"
"--\n"
"\n"
"Convert this value to exact type complex.");

#define _DECIMAL_DECIMAL___COMPLEX___METHODDEF    \
    {"__complex__", (PyCFunction)_decimal_Decimal___complex__, METH_NOARGS, _decimal_Decimal___complex____doc__},

static PyObject *
_decimal_Decimal___complex___impl(PyObject *self);

static PyObject *
_decimal_Decimal___complex__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal___complex___impl(self);
}

PyDoc_STRVAR(_decimal_Decimal___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _DECIMAL_DECIMAL___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_decimal_Decimal___copy__, METH_NOARGS, _decimal_Decimal___copy____doc__},

static PyObject *
_decimal_Decimal___copy___impl(PyObject *self);

static PyObject *
_decimal_Decimal___copy__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal___copy___impl(self);
}

PyDoc_STRVAR(_decimal_Decimal___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _DECIMAL_DECIMAL___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)_decimal_Decimal___deepcopy__, METH_O, _decimal_Decimal___deepcopy____doc__},

PyDoc_STRVAR(_decimal_Decimal___floor____doc__,
"__floor__($self, /)\n"
"--\n"
"\n"
"Return the floor as an Integral.");

#define _DECIMAL_DECIMAL___FLOOR___METHODDEF    \
    {"__floor__", (PyCFunction)_decimal_Decimal___floor__, METH_NOARGS, _decimal_Decimal___floor____doc__},

static PyObject *
_decimal_Decimal___floor___impl(PyObject *self);

static PyObject *
_decimal_Decimal___floor__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal___floor___impl(self);
}

PyDoc_STRVAR(_decimal_Decimal___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define _DECIMAL_DECIMAL___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)_decimal_Decimal___reduce__, METH_NOARGS, _decimal_Decimal___reduce____doc__},

static PyObject *
_decimal_Decimal___reduce___impl(PyObject *self);

static PyObject *
_decimal_Decimal___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal___reduce___impl(self);
}

PyDoc_STRVAR(_decimal_Decimal___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes");

#define _DECIMAL_DECIMAL___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_decimal_Decimal___sizeof__, METH_NOARGS, _decimal_Decimal___sizeof____doc__},

static PyObject *
_decimal_Decimal___sizeof___impl(PyObject *v);

static PyObject *
_decimal_Decimal___sizeof__(PyObject *v, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal___sizeof___impl(v);
}

PyDoc_STRVAR(_decimal_Decimal___trunc____doc__,
"__trunc__($self, /)\n"
"--\n"
"\n"
"Return the Integral closest to x between 0 and x.");

#define _DECIMAL_DECIMAL___TRUNC___METHODDEF    \
    {"__trunc__", (PyCFunction)_decimal_Decimal___trunc__, METH_NOARGS, _decimal_Decimal___trunc____doc__},

static PyObject *
_decimal_Decimal___trunc___impl(PyObject *self);

static PyObject *
_decimal_Decimal___trunc__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Decimal___trunc___impl(self);
}

PyDoc_STRVAR(_decimal_Context_abs__doc__,
"abs($self, x, /)\n"
"--\n"
"\n"
"Return the absolute value of x.");

#define _DECIMAL_CONTEXT_ABS_METHODDEF    \
    {"abs", (PyCFunction)_decimal_Context_abs, METH_O, _decimal_Context_abs__doc__},

PyDoc_STRVAR(_decimal_Context_exp__doc__,
"exp($self, x, /)\n"
"--\n"
"\n"
"Return e ** x.");

#define _DECIMAL_CONTEXT_EXP_METHODDEF    \
    {"exp", (PyCFunction)_decimal_Context_exp, METH_O, _decimal_Context_exp__doc__},

PyDoc_STRVAR(_decimal_Context_ln__doc__,
"ln($self, x, /)\n"
"--\n"
"\n"
"Return the natural (base e) logarithm of x.");

#define _DECIMAL_CONTEXT_LN_METHODDEF    \
    {"ln", (PyCFunction)_decimal_Context_ln, METH_O, _decimal_Context_ln__doc__},

PyDoc_STRVAR(_decimal_Context_log10__doc__,
"log10($self, x, /)\n"
"--\n"
"\n"
"Return the base 10 logarithm of x.");

#define _DECIMAL_CONTEXT_LOG10_METHODDEF    \
    {"log10", (PyCFunction)_decimal_Context_log10, METH_O, _decimal_Context_log10__doc__},

PyDoc_STRVAR(_decimal_Context_minus__doc__,
"minus($self, x, /)\n"
"--\n"
"\n"
"Minus corresponds to unary prefix minus in Python.\n"
"\n"
"This operation applies the context to the result.");

#define _DECIMAL_CONTEXT_MINUS_METHODDEF    \
    {"minus", (PyCFunction)_decimal_Context_minus, METH_O, _decimal_Context_minus__doc__},

PyDoc_STRVAR(_decimal_Context_next_minus__doc__,
"next_minus($self, x, /)\n"
"--\n"
"\n"
"Return the largest representable number smaller than x.");

#define _DECIMAL_CONTEXT_NEXT_MINUS_METHODDEF    \
    {"next_minus", (PyCFunction)_decimal_Context_next_minus, METH_O, _decimal_Context_next_minus__doc__},

PyDoc_STRVAR(_decimal_Context_next_plus__doc__,
"next_plus($self, x, /)\n"
"--\n"
"\n"
"Return the smallest representable number larger than x.");

#define _DECIMAL_CONTEXT_NEXT_PLUS_METHODDEF    \
    {"next_plus", (PyCFunction)_decimal_Context_next_plus, METH_O, _decimal_Context_next_plus__doc__},

PyDoc_STRVAR(_decimal_Context_normalize__doc__,
"normalize($self, x, /)\n"
"--\n"
"\n"
"Reduce x to its simplest form. Alias for reduce(x).");

#define _DECIMAL_CONTEXT_NORMALIZE_METHODDEF    \
    {"normalize", (PyCFunction)_decimal_Context_normalize, METH_O, _decimal_Context_normalize__doc__},

PyDoc_STRVAR(_decimal_Context_plus__doc__,
"plus($self, x, /)\n"
"--\n"
"\n"
"Plus corresponds to the unary prefix plus operator in Python.\n"
"\n"
"This operation applies the context to the result.");

#define _DECIMAL_CONTEXT_PLUS_METHODDEF    \
    {"plus", (PyCFunction)_decimal_Context_plus, METH_O, _decimal_Context_plus__doc__},

PyDoc_STRVAR(_decimal_Context_to_integral_value__doc__,
"to_integral_value($self, x, /)\n"
"--\n"
"\n"
"Round to an integer.");

#define _DECIMAL_CONTEXT_TO_INTEGRAL_VALUE_METHODDEF    \
    {"to_integral_value", (PyCFunction)_decimal_Context_to_integral_value, METH_O, _decimal_Context_to_integral_value__doc__},

PyDoc_STRVAR(_decimal_Context_to_integral_exact__doc__,
"to_integral_exact($self, x, /)\n"
"--\n"
"\n"
"Round to an integer. Signal if the result is rounded or inexact.");

#define _DECIMAL_CONTEXT_TO_INTEGRAL_EXACT_METHODDEF    \
    {"to_integral_exact", (PyCFunction)_decimal_Context_to_integral_exact, METH_O, _decimal_Context_to_integral_exact__doc__},

PyDoc_STRVAR(_decimal_Context_to_integral__doc__,
"to_integral($self, x, /)\n"
"--\n"
"\n"
"Identical to to_integral_value(x).");

#define _DECIMAL_CONTEXT_TO_INTEGRAL_METHODDEF    \
    {"to_integral", (PyCFunction)_decimal_Context_to_integral, METH_O, _decimal_Context_to_integral__doc__},

PyDoc_STRVAR(_decimal_Context_sqrt__doc__,
"sqrt($self, x, /)\n"
"--\n"
"\n"
"Square root of a non-negative number to context precision.");

#define _DECIMAL_CONTEXT_SQRT_METHODDEF    \
    {"sqrt", (PyCFunction)_decimal_Context_sqrt, METH_O, _decimal_Context_sqrt__doc__},

PyDoc_STRVAR(_decimal_Context_add__doc__,
"add($self, x, y, /)\n"
"--\n"
"\n"
"Return the sum of x and y.");

#define _DECIMAL_CONTEXT_ADD_METHODDEF    \
    {"add", _PyCFunction_CAST(_decimal_Context_add), METH_FASTCALL, _decimal_Context_add__doc__},

static PyObject *
_decimal_Context_add_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_add(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("add", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_add_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_compare__doc__,
"compare($self, x, y, /)\n"
"--\n"
"\n"
"Compare x and y numerically.");

#define _DECIMAL_CONTEXT_COMPARE_METHODDEF    \
    {"compare", _PyCFunction_CAST(_decimal_Context_compare), METH_FASTCALL, _decimal_Context_compare__doc__},

static PyObject *
_decimal_Context_compare_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_compare(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("compare", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_compare_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_compare_signal__doc__,
"compare_signal($self, x, y, /)\n"
"--\n"
"\n"
"Compare x and y numerically.  All NaNs signal.");

#define _DECIMAL_CONTEXT_COMPARE_SIGNAL_METHODDEF    \
    {"compare_signal", _PyCFunction_CAST(_decimal_Context_compare_signal), METH_FASTCALL, _decimal_Context_compare_signal__doc__},

static PyObject *
_decimal_Context_compare_signal_impl(PyObject *context, PyObject *x,
                                     PyObject *y);

static PyObject *
_decimal_Context_compare_signal(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("compare_signal", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_compare_signal_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_divide__doc__,
"divide($self, x, y, /)\n"
"--\n"
"\n"
"Return x divided by y.");

#define _DECIMAL_CONTEXT_DIVIDE_METHODDEF    \
    {"divide", _PyCFunction_CAST(_decimal_Context_divide), METH_FASTCALL, _decimal_Context_divide__doc__},

static PyObject *
_decimal_Context_divide_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_divide(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("divide", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_divide_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_divide_int__doc__,
"divide_int($self, x, y, /)\n"
"--\n"
"\n"
"Return x divided by y, truncated to an integer.");

#define _DECIMAL_CONTEXT_DIVIDE_INT_METHODDEF    \
    {"divide_int", _PyCFunction_CAST(_decimal_Context_divide_int), METH_FASTCALL, _decimal_Context_divide_int__doc__},

static PyObject *
_decimal_Context_divide_int_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_divide_int(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("divide_int", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_divide_int_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_max__doc__,
"max($self, x, y, /)\n"
"--\n"
"\n"
"Compare the values numerically and return the maximum.");

#define _DECIMAL_CONTEXT_MAX_METHODDEF    \
    {"max", _PyCFunction_CAST(_decimal_Context_max), METH_FASTCALL, _decimal_Context_max__doc__},

static PyObject *
_decimal_Context_max_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_max(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("max", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_max_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_max_mag__doc__,
"max_mag($self, x, y, /)\n"
"--\n"
"\n"
"Compare the values numerically with their sign ignored.");

#define _DECIMAL_CONTEXT_MAX_MAG_METHODDEF    \
    {"max_mag", _PyCFunction_CAST(_decimal_Context_max_mag), METH_FASTCALL, _decimal_Context_max_mag__doc__},

static PyObject *
_decimal_Context_max_mag_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_max_mag(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("max_mag", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_max_mag_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_min__doc__,
"min($self, x, y, /)\n"
"--\n"
"\n"
"Compare the values numerically and return the minimum.");

#define _DECIMAL_CONTEXT_MIN_METHODDEF    \
    {"min", _PyCFunction_CAST(_decimal_Context_min), METH_FASTCALL, _decimal_Context_min__doc__},

static PyObject *
_decimal_Context_min_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_min(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("min", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_min_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_min_mag__doc__,
"min_mag($self, x, y, /)\n"
"--\n"
"\n"
"Compare the values numerically with their sign ignored.");

#define _DECIMAL_CONTEXT_MIN_MAG_METHODDEF    \
    {"min_mag", _PyCFunction_CAST(_decimal_Context_min_mag), METH_FASTCALL, _decimal_Context_min_mag__doc__},

static PyObject *
_decimal_Context_min_mag_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_min_mag(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("min_mag", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_min_mag_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_multiply__doc__,
"multiply($self, x, y, /)\n"
"--\n"
"\n"
"Return the product of x and y.");

#define _DECIMAL_CONTEXT_MULTIPLY_METHODDEF    \
    {"multiply", _PyCFunction_CAST(_decimal_Context_multiply), METH_FASTCALL, _decimal_Context_multiply__doc__},

static PyObject *
_decimal_Context_multiply_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_multiply(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("multiply", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_multiply_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_next_toward__doc__,
"next_toward($self, x, y, /)\n"
"--\n"
"\n"
"Return the number closest to x, in the direction towards y.");

#define _DECIMAL_CONTEXT_NEXT_TOWARD_METHODDEF    \
    {"next_toward", _PyCFunction_CAST(_decimal_Context_next_toward), METH_FASTCALL, _decimal_Context_next_toward__doc__},

static PyObject *
_decimal_Context_next_toward_impl(PyObject *context, PyObject *x,
                                  PyObject *y);

static PyObject *
_decimal_Context_next_toward(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("next_toward", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_next_toward_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_quantize__doc__,
"quantize($self, x, y, /)\n"
"--\n"
"\n"
"Return a value equal to x (rounded), having the exponent of y.");

#define _DECIMAL_CONTEXT_QUANTIZE_METHODDEF    \
    {"quantize", _PyCFunction_CAST(_decimal_Context_quantize), METH_FASTCALL, _decimal_Context_quantize__doc__},

static PyObject *
_decimal_Context_quantize_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_quantize(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("quantize", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_quantize_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_remainder__doc__,
"remainder($self, x, y, /)\n"
"--\n"
"\n"
"Return the remainder from integer division.\n"
"\n"
"The sign of the result, if non-zero, is the same as that of the\n"
"original dividend.");

#define _DECIMAL_CONTEXT_REMAINDER_METHODDEF    \
    {"remainder", _PyCFunction_CAST(_decimal_Context_remainder), METH_FASTCALL, _decimal_Context_remainder__doc__},

static PyObject *
_decimal_Context_remainder_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_remainder(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("remainder", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_remainder_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_remainder_near__doc__,
"remainder_near($self, x, y, /)\n"
"--\n"
"\n"
"Return x - y * n.\n"
"\n"
"Here n is the integer nearest the exact value of x / y (if the result\n"
"is 0 then its sign will be the sign of x).");

#define _DECIMAL_CONTEXT_REMAINDER_NEAR_METHODDEF    \
    {"remainder_near", _PyCFunction_CAST(_decimal_Context_remainder_near), METH_FASTCALL, _decimal_Context_remainder_near__doc__},

static PyObject *
_decimal_Context_remainder_near_impl(PyObject *context, PyObject *x,
                                     PyObject *y);

static PyObject *
_decimal_Context_remainder_near(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("remainder_near", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_remainder_near_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_subtract__doc__,
"subtract($self, x, y, /)\n"
"--\n"
"\n"
"Return the difference between x and y.");

#define _DECIMAL_CONTEXT_SUBTRACT_METHODDEF    \
    {"subtract", _PyCFunction_CAST(_decimal_Context_subtract), METH_FASTCALL, _decimal_Context_subtract__doc__},

static PyObject *
_decimal_Context_subtract_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_subtract(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("subtract", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_subtract_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_power__doc__,
"power($self, /, a, b, modulo=None)\n"
"--\n"
"\n"
"Compute a**b.\n"
"\n"
"If \'a\' is negative, then \'b\' must be integral. The result will be\n"
"inexact unless \'a\' is integral and the result is finite and can be\n"
"expressed exactly in \'precision\' digits.  In the Python version the\n"
"result is always correctly rounded, in the C version the result is\n"
"almost always correctly rounded.\n"
"\n"
"If modulo is given, compute (a**b) % modulo. The following\n"
"restrictions hold:\n"
"\n"
"    * all three arguments must be integral\n"
"    * \'b\' must be nonnegative\n"
"    * at least one of \'a\' or \'b\' must be nonzero\n"
"    * modulo must be nonzero and less than 10**prec in absolute value");

#define _DECIMAL_CONTEXT_POWER_METHODDEF    \
    {"power", _PyCFunction_CAST(_decimal_Context_power), METH_FASTCALL|METH_KEYWORDS, _decimal_Context_power__doc__},

static PyObject *
_decimal_Context_power_impl(PyObject *context, PyObject *base, PyObject *exp,
                            PyObject *mod);

static PyObject *
_decimal_Context_power(PyObject *context, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), &_Py_ID(modulo), },
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    return_value = _decimal_Context_power_impl(context, base, exp, mod);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_fma__doc__,
"fma($self, x, y, z, /)\n"
"--\n"
"\n"
"Return x multiplied by y, plus z.");

#define _DECIMAL_CONTEXT_FMA_METHODDEF    \
    {"fma", _PyCFunction_CAST(_decimal_Context_fma), METH_FASTCALL, _decimal_Context_fma__doc__},

static PyObject *
_decimal_Context_fma_impl(PyObject *context, PyObject *x, PyObject *y,
                          PyObject *z);

static PyObject *
_decimal_Context_fma(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;
    PyObject *z;

    if (!_PyArg_CheckPositional("fma", nargs, 3, 3)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    z = args[2];
    return_value = _decimal_Context_fma_impl(context, x, y, z);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_radix__doc__,
"radix($self, /)\n"
"--\n"
"\n"
"Return 10.");

#define _DECIMAL_CONTEXT_RADIX_METHODDEF    \
    {"radix", (PyCFunction)_decimal_Context_radix, METH_NOARGS, _decimal_Context_radix__doc__},

static PyObject *
_decimal_Context_radix_impl(PyObject *context);

static PyObject *
_decimal_Context_radix(PyObject *context, PyObject *Py_UNUSED(ignored))
{
    return _decimal_Context_radix_impl(context);
}

PyDoc_STRVAR(_decimal_Context_is_normal__doc__,
"is_normal($self, x, /)\n"
"--\n"
"\n"
"Return True if x is a normal number, False otherwise.");

#define _DECIMAL_CONTEXT_IS_NORMAL_METHODDEF    \
    {"is_normal", (PyCFunction)_decimal_Context_is_normal, METH_O, _decimal_Context_is_normal__doc__},

PyDoc_STRVAR(_decimal_Context_is_subnormal__doc__,
"is_subnormal($self, x, /)\n"
"--\n"
"\n"
"Return True if x is subnormal, False otherwise.");

#define _DECIMAL_CONTEXT_IS_SUBNORMAL_METHODDEF    \
    {"is_subnormal", (PyCFunction)_decimal_Context_is_subnormal, METH_O, _decimal_Context_is_subnormal__doc__},

PyDoc_STRVAR(_decimal_Context_is_finite__doc__,
"is_finite($self, x, /)\n"
"--\n"
"\n"
"Return True if x is finite, False otherwise.");

#define _DECIMAL_CONTEXT_IS_FINITE_METHODDEF    \
    {"is_finite", (PyCFunction)_decimal_Context_is_finite, METH_O, _decimal_Context_is_finite__doc__},

PyDoc_STRVAR(_decimal_Context_is_infinite__doc__,
"is_infinite($self, x, /)\n"
"--\n"
"\n"
"Return True if x is infinite, False otherwise.");

#define _DECIMAL_CONTEXT_IS_INFINITE_METHODDEF    \
    {"is_infinite", (PyCFunction)_decimal_Context_is_infinite, METH_O, _decimal_Context_is_infinite__doc__},

PyDoc_STRVAR(_decimal_Context_is_nan__doc__,
"is_nan($self, x, /)\n"
"--\n"
"\n"
"Return True if x is a qNaN or sNaN, False otherwise.");

#define _DECIMAL_CONTEXT_IS_NAN_METHODDEF    \
    {"is_nan", (PyCFunction)_decimal_Context_is_nan, METH_O, _decimal_Context_is_nan__doc__},

PyDoc_STRVAR(_decimal_Context_is_qnan__doc__,
"is_qnan($self, x, /)\n"
"--\n"
"\n"
"Return True if x is a quiet NaN, False otherwise.");

#define _DECIMAL_CONTEXT_IS_QNAN_METHODDEF    \
    {"is_qnan", (PyCFunction)_decimal_Context_is_qnan, METH_O, _decimal_Context_is_qnan__doc__},

PyDoc_STRVAR(_decimal_Context_is_snan__doc__,
"is_snan($self, x, /)\n"
"--\n"
"\n"
"Return True if x is a signaling NaN, False otherwise.");

#define _DECIMAL_CONTEXT_IS_SNAN_METHODDEF    \
    {"is_snan", (PyCFunction)_decimal_Context_is_snan, METH_O, _decimal_Context_is_snan__doc__},

PyDoc_STRVAR(_decimal_Context_is_signed__doc__,
"is_signed($self, x, /)\n"
"--\n"
"\n"
"Return True if x is negative, False otherwise.");

#define _DECIMAL_CONTEXT_IS_SIGNED_METHODDEF    \
    {"is_signed", (PyCFunction)_decimal_Context_is_signed, METH_O, _decimal_Context_is_signed__doc__},

PyDoc_STRVAR(_decimal_Context_is_zero__doc__,
"is_zero($self, x, /)\n"
"--\n"
"\n"
"Return True if x is a zero, False otherwise.");

#define _DECIMAL_CONTEXT_IS_ZERO_METHODDEF    \
    {"is_zero", (PyCFunction)_decimal_Context_is_zero, METH_O, _decimal_Context_is_zero__doc__},

PyDoc_STRVAR(_decimal_Context_logb__doc__,
"logb($self, x, /)\n"
"--\n"
"\n"
"Return the exponent of the magnitude of the operand\'s MSD.");

#define _DECIMAL_CONTEXT_LOGB_METHODDEF    \
    {"logb", (PyCFunction)_decimal_Context_logb, METH_O, _decimal_Context_logb__doc__},

PyDoc_STRVAR(_decimal_Context_logical_invert__doc__,
"logical_invert($self, x, /)\n"
"--\n"
"\n"
"Invert all digits of x.");

#define _DECIMAL_CONTEXT_LOGICAL_INVERT_METHODDEF    \
    {"logical_invert", (PyCFunction)_decimal_Context_logical_invert, METH_O, _decimal_Context_logical_invert__doc__},

PyDoc_STRVAR(_decimal_Context_compare_total__doc__,
"compare_total($self, x, y, /)\n"
"--\n"
"\n"
"Compare x and y using their abstract representation.");

#define _DECIMAL_CONTEXT_COMPARE_TOTAL_METHODDEF    \
    {"compare_total", _PyCFunction_CAST(_decimal_Context_compare_total), METH_FASTCALL, _decimal_Context_compare_total__doc__},

static PyObject *
_decimal_Context_compare_total_impl(PyObject *context, PyObject *x,
                                    PyObject *y);

static PyObject *
_decimal_Context_compare_total(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("compare_total", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_compare_total_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_compare_total_mag__doc__,
"compare_total_mag($self, x, y, /)\n"
"--\n"
"\n"
"Compare x and y using their abstract representation, ignoring sign.");

#define _DECIMAL_CONTEXT_COMPARE_TOTAL_MAG_METHODDEF    \
    {"compare_total_mag", _PyCFunction_CAST(_decimal_Context_compare_total_mag), METH_FASTCALL, _decimal_Context_compare_total_mag__doc__},

static PyObject *
_decimal_Context_compare_total_mag_impl(PyObject *context, PyObject *x,
                                        PyObject *y);

static PyObject *
_decimal_Context_compare_total_mag(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("compare_total_mag", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_compare_total_mag_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_logical_and__doc__,
"logical_and($self, x, y, /)\n"
"--\n"
"\n"
"Digit-wise and of x and y.");

#define _DECIMAL_CONTEXT_LOGICAL_AND_METHODDEF    \
    {"logical_and", _PyCFunction_CAST(_decimal_Context_logical_and), METH_FASTCALL, _decimal_Context_logical_and__doc__},

static PyObject *
_decimal_Context_logical_and_impl(PyObject *context, PyObject *x,
                                  PyObject *y);

static PyObject *
_decimal_Context_logical_and(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("logical_and", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_logical_and_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_logical_or__doc__,
"logical_or($self, x, y, /)\n"
"--\n"
"\n"
"Digit-wise or of x and y.");

#define _DECIMAL_CONTEXT_LOGICAL_OR_METHODDEF    \
    {"logical_or", _PyCFunction_CAST(_decimal_Context_logical_or), METH_FASTCALL, _decimal_Context_logical_or__doc__},

static PyObject *
_decimal_Context_logical_or_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_logical_or(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("logical_or", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_logical_or_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_logical_xor__doc__,
"logical_xor($self, x, y, /)\n"
"--\n"
"\n"
"Digit-wise xor of x and y.");

#define _DECIMAL_CONTEXT_LOGICAL_XOR_METHODDEF    \
    {"logical_xor", _PyCFunction_CAST(_decimal_Context_logical_xor), METH_FASTCALL, _decimal_Context_logical_xor__doc__},

static PyObject *
_decimal_Context_logical_xor_impl(PyObject *context, PyObject *x,
                                  PyObject *y);

static PyObject *
_decimal_Context_logical_xor(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("logical_xor", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_logical_xor_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_rotate__doc__,
"rotate($self, x, y, /)\n"
"--\n"
"\n"
"Return a copy of x, rotated by y places.");

#define _DECIMAL_CONTEXT_ROTATE_METHODDEF    \
    {"rotate", _PyCFunction_CAST(_decimal_Context_rotate), METH_FASTCALL, _decimal_Context_rotate__doc__},

static PyObject *
_decimal_Context_rotate_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_rotate(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("rotate", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_rotate_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_scaleb__doc__,
"scaleb($self, x, y, /)\n"
"--\n"
"\n"
"Return the first operand after adding the second value to its exp.");

#define _DECIMAL_CONTEXT_SCALEB_METHODDEF    \
    {"scaleb", _PyCFunction_CAST(_decimal_Context_scaleb), METH_FASTCALL, _decimal_Context_scaleb__doc__},

static PyObject *
_decimal_Context_scaleb_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_scaleb(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("scaleb", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_scaleb_impl(context, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(_decimal_Context_shift__doc__,
"shift($self, x, y, /)\n"
"--\n"
"\n"
"Return a copy of x, shifted by y places.");

#define _DECIMAL_CONTEXT_SHIFT_METHODDEF    \
    {"shift", _PyCFunction_CAST(_decimal_Context_shift), METH_FASTCALL, _decimal_Context_shift__doc__},

static PyObject *
_decimal_Context_shift_impl(PyObject *context, PyObject *x, PyObject *y);

static PyObject *
_decimal_Context_shift(PyObject *context, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("shift", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = _decimal_Context_shift_impl(context, x, y);

exit:
    return return_value;
}
/*[clinic end generated code: output=db00589bcc7a168d input=a9049054013a1b77]*/
