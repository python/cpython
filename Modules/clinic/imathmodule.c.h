/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(imath_gcd__doc__,
"gcd($module, x, y, /)\n"
"--\n"
"\n"
"Greatest common divisor of x and y.");

#define IMATH_GCD_METHODDEF    \
    {"gcd", (PyCFunction)(void(*)(void))imath_gcd, METH_FASTCALL, imath_gcd__doc__},

static PyObject *
imath_gcd_impl(PyObject *module, PyObject *x, PyObject *y);

static PyObject *
imath_gcd(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("gcd", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = imath_gcd_impl(module, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(imath_ilog2__doc__,
"ilog2($module, n, /)\n"
"--\n"
"\n"
"Return the integer part of the base 2 logarithm of the input.");

#define IMATH_ILOG2_METHODDEF    \
    {"ilog2", (PyCFunction)imath_ilog2, METH_O, imath_ilog2__doc__},

PyDoc_STRVAR(imath_isqrt__doc__,
"isqrt($module, n, /)\n"
"--\n"
"\n"
"Return the integer part of the square root of the input.");

#define IMATH_ISQRT_METHODDEF    \
    {"isqrt", (PyCFunction)imath_isqrt, METH_O, imath_isqrt__doc__},

PyDoc_STRVAR(imath_factorial__doc__,
"factorial($module, x, /)\n"
"--\n"
"\n"
"Find x!.\n"
"\n"
"Raise a TypeError if x is not an integer.\n"
"Raise a ValueError if x is negative integer.");

#define IMATH_FACTORIAL_METHODDEF    \
    {"factorial", (PyCFunction)imath_factorial, METH_O, imath_factorial__doc__},

PyDoc_STRVAR(imath_perm__doc__,
"perm($module, n, k, /)\n"
"--\n"
"\n"
"Number of ways to choose k items from n items without repetition and with order.\n"
"\n"
"It is mathematically equal to the expression n! / (n - k)!.\n"
"\n"
"Raises TypeError if the arguments are not integers.\n"
"Raises ValueError if the arguments are negative or if k > n.");

#define IMATH_PERM_METHODDEF    \
    {"perm", (PyCFunction)(void(*)(void))imath_perm, METH_FASTCALL, imath_perm__doc__},

static PyObject *
imath_perm_impl(PyObject *module, PyObject *n, PyObject *k);

static PyObject *
imath_perm(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *n;
    PyObject *k;

    if (!_PyArg_CheckPositional("perm", nargs, 2, 2)) {
        goto exit;
    }
    n = args[0];
    k = args[1];
    return_value = imath_perm_impl(module, n, k);

exit:
    return return_value;
}

PyDoc_STRVAR(imath_comb__doc__,
"comb($module, n, k, /)\n"
"--\n"
"\n"
"Number of ways to choose k items from n items without repetition and without order.\n"
"\n"
"Also called the binomial coefficient. It is mathematically equal to the expression\n"
"n! / (k! * (n - k)!). It is equivalent to the coefficient of k-th term in\n"
"polynomial expansion of the expression (1 + x)**n.\n"
"\n"
"Raises TypeError if the arguments are not integers.\n"
"Raises ValueError if the arguments are negative or if k > n.");

#define IMATH_COMB_METHODDEF    \
    {"comb", (PyCFunction)(void(*)(void))imath_comb, METH_FASTCALL, imath_comb__doc__},

static PyObject *
imath_comb_impl(PyObject *module, PyObject *n, PyObject *k);

static PyObject *
imath_comb(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *n;
    PyObject *k;

    if (!_PyArg_CheckPositional("comb", nargs, 2, 2)) {
        goto exit;
    }
    n = args[0];
    k = args[1];
    return_value = imath_comb_impl(module, n, k);

exit:
    return return_value;
}
/*[clinic end generated code: output=47736df1c2f249c7 input=a9049054013a1b77]*/
