/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

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
/*[clinic end generated code: output=34697570c923a3af input=a9049054013a1b77]*/
