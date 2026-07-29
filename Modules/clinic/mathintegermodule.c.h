/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(math_integer_gcd__doc__,
"gcd($module, /, *integers)\n"
"--\n"
"\n"
"Greatest Common Divisor.");

#define MATH_INTEGER_GCD_METHODDEF    \
    {"gcd", _PyCFunction_CAST(math_integer_gcd), METH_FASTCALL, math_integer_gcd__doc__},

static PyObject *
math_integer_gcd_impl(PyObject *module, PyObject * const *args,
                      Py_ssize_t args_length);

static PyObject *
math_integer_gcd(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject * const *__clinic_args;
    Py_ssize_t args_length;

    __clinic_args = args;
    args_length = nargs;
    return_value = math_integer_gcd_impl(module, __clinic_args, args_length);

    return return_value;
}

PyDoc_STRVAR(math_integer_lcm__doc__,
"lcm($module, /, *integers)\n"
"--\n"
"\n"
"Least Common Multiple.");

#define MATH_INTEGER_LCM_METHODDEF    \
    {"lcm", _PyCFunction_CAST(math_integer_lcm), METH_FASTCALL, math_integer_lcm__doc__},

static PyObject *
math_integer_lcm_impl(PyObject *module, PyObject * const *args,
                      Py_ssize_t args_length);

static PyObject *
math_integer_lcm(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject * const *__clinic_args;
    Py_ssize_t args_length;

    __clinic_args = args;
    args_length = nargs;
    return_value = math_integer_lcm_impl(module, __clinic_args, args_length);

    return return_value;
}

PyDoc_STRVAR(math_integer_isqrt__doc__,
"isqrt($module, n, /)\n"
"--\n"
"\n"
"Return the integer part of the square root of the input.");

#define MATH_INTEGER_ISQRT_METHODDEF    \
    {"isqrt", (PyCFunction)math_integer_isqrt, METH_O, math_integer_isqrt__doc__},

PyDoc_STRVAR(math_integer_isprime__doc__,
"isprime($module, n, /)\n"
"--\n"
"\n"
"Return True if n is a prime number, False otherwise.\n"
"\n"
"The argument must be less than 2**64.");

#define MATH_INTEGER_ISPRIME_METHODDEF    \
    {"isprime", (PyCFunction)math_integer_isprime, METH_O, math_integer_isprime__doc__},

static int
math_integer_isprime_impl(PyObject *module, PyObject *n);

static PyObject *
math_integer_isprime(PyObject *module, PyObject *n)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = math_integer_isprime_impl(module, n);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(math_integer_primes__doc__,
"primes($module, /, start=2, stop=None)\n"
"--\n"
"\n"
"Return an iterator of the prime numbers in the range [start, stop).\n"
"\n"
"If stop is None, the iteration does not stop.\n"
"The bounds must be less than 2**64.");

#define MATH_INTEGER_PRIMES_METHODDEF    \
    {"primes", _PyCFunction_CAST(math_integer_primes), METH_FASTCALL|METH_KEYWORDS, math_integer_primes__doc__},

static PyObject *
math_integer_primes_impl(PyObject *module, PyObject *start, PyObject *stop);

static PyObject *
math_integer_primes(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(start), &_Py_ID(stop), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"start", "stop", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "primes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *start = NULL;
    PyObject *stop = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        start = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    stop = args[1];
skip_optional_pos:
    return_value = math_integer_primes_impl(module, start, stop);

exit:
    return return_value;
}

PyDoc_STRVAR(math_integer_factorial__doc__,
"factorial($module, n, /)\n"
"--\n"
"\n"
"Find n!.");

#define MATH_INTEGER_FACTORIAL_METHODDEF    \
    {"factorial", (PyCFunction)math_integer_factorial, METH_O, math_integer_factorial__doc__},

PyDoc_STRVAR(math_integer_perm__doc__,
"perm($module, n, k=None, /)\n"
"--\n"
"\n"
"Number of ways to choose k items from n items without repetition and with order.\n"
"\n"
"Evaluates to n! / (n - k)! when k <= n and evaluates\n"
"to zero when k > n.\n"
"\n"
"If k is not specified or is None, then k defaults to n\n"
"and the function returns n!.\n"
"\n"
"Raises ValueError if either of the arguments are negative.");

#define MATH_INTEGER_PERM_METHODDEF    \
    {"perm", _PyCFunction_CAST(math_integer_perm), METH_FASTCALL, math_integer_perm__doc__},

static PyObject *
math_integer_perm_impl(PyObject *module, PyObject *n, PyObject *k);

static PyObject *
math_integer_perm(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *n;
    PyObject *k = Py_None;

    if (!_PyArg_CheckPositional("perm", nargs, 1, 2)) {
        goto exit;
    }
    n = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    k = args[1];
skip_optional:
    return_value = math_integer_perm_impl(module, n, k);

exit:
    return return_value;
}

PyDoc_STRVAR(math_integer_comb__doc__,
"comb($module, n, k, /)\n"
"--\n"
"\n"
"Number of ways to choose k items from n items without repetition and without order.\n"
"\n"
"Evaluates to n! / (k! * (n - k)!) when k <= n and evaluates\n"
"to zero when k > n.\n"
"\n"
"Also called the binomial coefficient because it is equivalent\n"
"to the coefficient of k-th term in polynomial expansion of the\n"
"expression (1 + x)**n.\n"
"\n"
"Raises ValueError if either of the arguments are negative.");

#define MATH_INTEGER_COMB_METHODDEF    \
    {"comb", _PyCFunction_CAST(math_integer_comb), METH_FASTCALL, math_integer_comb__doc__},

static PyObject *
math_integer_comb_impl(PyObject *module, PyObject *n, PyObject *k);

static PyObject *
math_integer_comb(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *n;
    PyObject *k;

    if (!_PyArg_CheckPositional("comb", nargs, 2, 2)) {
        goto exit;
    }
    n = args[0];
    k = args[1];
    return_value = math_integer_comb_impl(module, n, k);

exit:
    return return_value;
}
/*[clinic end generated code: output=172b302a40542e1c input=a9049054013a1b77]*/
