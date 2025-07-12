/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_decimal_Decimal_copy_sign__doc__,
"copy_sign($self, /, other, context=None)\n"
"--\n"
"\n"
"Returns self with the sign of other.\n"
"\n"
"For example:\n"
"\n"
"    >>> Decimal(\'2.3\').copy_sign(Decimal(\'-1.5\'))\n"
"    Decimal(\'-2.3\')\n"
"\n"
"This operation is unaffected by context and is quiet: no flags are changed\n"
"and no rounding is performed.  As an exception, the C version may raise\n"
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

PyDoc_STRVAR(_decimal_Decimal_quantize__doc__,
"quantize($self, /, exp, rounding=None, context=None)\n"
"--\n"
"\n"
"Quantize self so its exponent is the same as that of exp.\n"
"\n"
"Return a value equal to the first operand after rounding and having the\n"
"exponent of the second operand.\n"
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
/*[clinic end generated code: output=6c9fd93f1bdb644a input=a9049054013a1b77]*/
