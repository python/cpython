/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(math_gcd__doc__,
"gcd($module, x, y, /)\n"
"--\n"
"\n"
"greatest common divisor of x and y");

#define MATH_GCD_METHODDEF    \
    {"gcd", (PyCFunction)math_gcd, METH_FASTCALL, math_gcd__doc__},

static PyObject *
math_gcd_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
math_gcd(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_UnpackStack(args, nargs, "gcd",
        2, 2,
        &a, &b)) {
        goto exit;
    }
    return_value = math_gcd_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(math_ceil__doc__,
"ceil($module, x, /)\n"
"--\n"
"\n"
"Return the ceiling of x as an Integral.\n"
"\n"
"This is the smallest integer >= x.");

#define MATH_CEIL_METHODDEF    \
    {"ceil", (PyCFunction)math_ceil, METH_O, math_ceil__doc__},

PyDoc_STRVAR(math_floor__doc__,
"floor($module, x, /)\n"
"--\n"
"\n"
"Return the floor of x as an Integral.\n"
"\n"
"This is the largest integer <= x.");

#define MATH_FLOOR_METHODDEF    \
    {"floor", (PyCFunction)math_floor, METH_O, math_floor__doc__},

PyDoc_STRVAR(math_fsum__doc__,
"fsum($module, seq, /)\n"
"--\n"
"\n"
"Return an accurate floating point sum of values in the iterable seq.\n"
"\n"
"Assumes IEEE-754 floating point arithmetic.");

#define MATH_FSUM_METHODDEF    \
    {"fsum", (PyCFunction)math_fsum, METH_O, math_fsum__doc__},

PyDoc_STRVAR(math_factorial__doc__,
"factorial($module, x, /)\n"
"--\n"
"\n"
"Find x!.\n"
"\n"
"Raise a ValueError if x is negative or non-integral.");

#define MATH_FACTORIAL_METHODDEF    \
    {"factorial", (PyCFunction)math_factorial, METH_O, math_factorial__doc__},

PyDoc_STRVAR(math_trunc__doc__,
"trunc($module, x, /)\n"
"--\n"
"\n"
"Truncates the Real x to the nearest Integral toward 0.\n"
"\n"
"Uses the __trunc__ magic method.");

#define MATH_TRUNC_METHODDEF    \
    {"trunc", (PyCFunction)math_trunc, METH_O, math_trunc__doc__},

PyDoc_STRVAR(math_frexp__doc__,
"frexp($module, x, /)\n"
"--\n"
"\n"
"Return the mantissa and exponent of x, as pair (m, e).\n"
"\n"
"m is a float and e is an int, such that x = m * 2.**e.\n"
"If x is 0, m and e are both 0.  Else 0.5 <= abs(m) < 1.0.");

#define MATH_FREXP_METHODDEF    \
    {"frexp", (PyCFunction)math_frexp, METH_O, math_frexp__doc__},

static PyObject *
math_frexp_impl(PyObject *module, double x);

static PyObject *
math_frexp(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (!PyArg_Parse(arg, "d:frexp", &x)) {
        goto exit;
    }
    return_value = math_frexp_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_ldexp__doc__,
"ldexp($module, x, i, /)\n"
"--\n"
"\n"
"Return x * (2**i).\n"
"\n"
"This is essentially the inverse of frexp().");

#define MATH_LDEXP_METHODDEF    \
    {"ldexp", (PyCFunction)math_ldexp, METH_FASTCALL, math_ldexp__doc__},

static PyObject *
math_ldexp_impl(PyObject *module, double x, PyObject *i);

static PyObject *
math_ldexp(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    PyObject *i;

    if (!_PyArg_ParseStack(args, nargs, "dO:ldexp",
        &x, &i)) {
        goto exit;
    }
    return_value = math_ldexp_impl(module, x, i);

exit:
    return return_value;
}

PyDoc_STRVAR(math_modf__doc__,
"modf($module, x, /)\n"
"--\n"
"\n"
"Return the fractional and integer parts of x.\n"
"\n"
"Both results carry the sign of x and are floats.");

#define MATH_MODF_METHODDEF    \
    {"modf", (PyCFunction)math_modf, METH_O, math_modf__doc__},

static PyObject *
math_modf_impl(PyObject *module, double x);

static PyObject *
math_modf(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (!PyArg_Parse(arg, "d:modf", &x)) {
        goto exit;
    }
    return_value = math_modf_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_log__doc__,
"log(x, [base=math.e])\n"
"Return the logarithm of x to the given base.\n"
"\n"
"If the base not specified, returns the natural logarithm (base e) of x.");

#define MATH_LOG_METHODDEF    \
    {"log", (PyCFunction)math_log, METH_VARARGS, math_log__doc__},

static PyObject *
math_log_impl(PyObject *module, PyObject *x, int group_right_1,
              PyObject *base);

static PyObject *
math_log(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *x;
    int group_right_1 = 0;
    PyObject *base = NULL;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O:log", &x)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "OO:log", &x, &base)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "math.log requires 1 to 2 arguments");
            goto exit;
    }
    return_value = math_log_impl(module, x, group_right_1, base);

exit:
    return return_value;
}

PyDoc_STRVAR(math_log2__doc__,
"log2($module, x, /)\n"
"--\n"
"\n"
"Return the base 2 logarithm of x.");

#define MATH_LOG2_METHODDEF    \
    {"log2", (PyCFunction)math_log2, METH_O, math_log2__doc__},

PyDoc_STRVAR(math_log10__doc__,
"log10($module, x, /)\n"
"--\n"
"\n"
"Return the base 10 logarithm of x.");

#define MATH_LOG10_METHODDEF    \
    {"log10", (PyCFunction)math_log10, METH_O, math_log10__doc__},

PyDoc_STRVAR(math_fmod__doc__,
"fmod($module, x, y, /)\n"
"--\n"
"\n"
"Return fmod(x, y), according to platform C.\n"
"\n"
"x % y may differ.");

#define MATH_FMOD_METHODDEF    \
    {"fmod", (PyCFunction)math_fmod, METH_FASTCALL, math_fmod__doc__},

static PyObject *
math_fmod_impl(PyObject *module, double x, double y);

static PyObject *
math_fmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;

    if (!_PyArg_ParseStack(args, nargs, "dd:fmod",
        &x, &y)) {
        goto exit;
    }
    return_value = math_fmod_impl(module, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(math_hypot__doc__,
"hypot($module, x, y, /)\n"
"--\n"
"\n"
"Return the Euclidean distance, sqrt(x*x + y*y).");

#define MATH_HYPOT_METHODDEF    \
    {"hypot", (PyCFunction)math_hypot, METH_FASTCALL, math_hypot__doc__},

static PyObject *
math_hypot_impl(PyObject *module, double x, double y);

static PyObject *
math_hypot(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;

    if (!_PyArg_ParseStack(args, nargs, "dd:hypot",
        &x, &y)) {
        goto exit;
    }
    return_value = math_hypot_impl(module, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(math_pow__doc__,
"pow($module, x, y, /)\n"
"--\n"
"\n"
"Return x**y (x to the power of y).");

#define MATH_POW_METHODDEF    \
    {"pow", (PyCFunction)math_pow, METH_FASTCALL, math_pow__doc__},

static PyObject *
math_pow_impl(PyObject *module, double x, double y);

static PyObject *
math_pow(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;

    if (!_PyArg_ParseStack(args, nargs, "dd:pow",
        &x, &y)) {
        goto exit;
    }
    return_value = math_pow_impl(module, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(math_degrees__doc__,
"degrees($module, x, /)\n"
"--\n"
"\n"
"Convert angle x from radians to degrees.");

#define MATH_DEGREES_METHODDEF    \
    {"degrees", (PyCFunction)math_degrees, METH_O, math_degrees__doc__},

static PyObject *
math_degrees_impl(PyObject *module, double x);

static PyObject *
math_degrees(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (!PyArg_Parse(arg, "d:degrees", &x)) {
        goto exit;
    }
    return_value = math_degrees_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_radians__doc__,
"radians($module, x, /)\n"
"--\n"
"\n"
"Convert angle x from degrees to radians.");

#define MATH_RADIANS_METHODDEF    \
    {"radians", (PyCFunction)math_radians, METH_O, math_radians__doc__},

static PyObject *
math_radians_impl(PyObject *module, double x);

static PyObject *
math_radians(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (!PyArg_Parse(arg, "d:radians", &x)) {
        goto exit;
    }
    return_value = math_radians_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_isfinite__doc__,
"isfinite($module, x, /)\n"
"--\n"
"\n"
"Return True if x is neither an infinity nor a NaN, and False otherwise.");

#define MATH_ISFINITE_METHODDEF    \
    {"isfinite", (PyCFunction)math_isfinite, METH_O, math_isfinite__doc__},

static PyObject *
math_isfinite_impl(PyObject *module, double x);

static PyObject *
math_isfinite(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (!PyArg_Parse(arg, "d:isfinite", &x)) {
        goto exit;
    }
    return_value = math_isfinite_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_isnan__doc__,
"isnan($module, x, /)\n"
"--\n"
"\n"
"Return True if x is a NaN (not a number), and False otherwise.");

#define MATH_ISNAN_METHODDEF    \
    {"isnan", (PyCFunction)math_isnan, METH_O, math_isnan__doc__},

static PyObject *
math_isnan_impl(PyObject *module, double x);

static PyObject *
math_isnan(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (!PyArg_Parse(arg, "d:isnan", &x)) {
        goto exit;
    }
    return_value = math_isnan_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_isinf__doc__,
"isinf($module, x, /)\n"
"--\n"
"\n"
"Return True if x is a positive or negative infinity, and False otherwise.");

#define MATH_ISINF_METHODDEF    \
    {"isinf", (PyCFunction)math_isinf, METH_O, math_isinf__doc__},

static PyObject *
math_isinf_impl(PyObject *module, double x);

static PyObject *
math_isinf(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (!PyArg_Parse(arg, "d:isinf", &x)) {
        goto exit;
    }
    return_value = math_isinf_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_isclose__doc__,
"isclose($module, /, a, b, *, rel_tol=1e-09, abs_tol=0.0)\n"
"--\n"
"\n"
"Determine whether two floating point numbers are close in value.\n"
"\n"
"  rel_tol\n"
"    maximum difference for being considered \"close\", relative to the\n"
"    magnitude of the input values\n"
"  abs_tol\n"
"    maximum difference for being considered \"close\", regardless of the\n"
"    magnitude of the input values\n"
"\n"
"Return True if a is close in value to b, and False otherwise.\n"
"\n"
"For the values to be considered close, the difference between them\n"
"must be smaller than at least one of the tolerances.\n"
"\n"
"-inf, inf and NaN behave similarly to the IEEE 754 Standard.  That\n"
"is, NaN is not close to anything, even itself.  inf and -inf are\n"
"only close to themselves.");

#define MATH_ISCLOSE_METHODDEF    \
    {"isclose", (PyCFunction)math_isclose, METH_FASTCALL|METH_KEYWORDS, math_isclose__doc__},

static int
math_isclose_impl(PyObject *module, double a, double b, double rel_tol,
                  double abs_tol);

static PyObject *
math_isclose(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", "rel_tol", "abs_tol", NULL};
    static _PyArg_Parser _parser = {"dd|$dd:isclose", _keywords, 0};
    double a;
    double b;
    double rel_tol = 1e-09;
    double abs_tol = 0.0;
    int _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &a, &b, &rel_tol, &abs_tol)) {
        goto exit;
    }
    _return_value = math_isclose_impl(module, a, b, rel_tol, abs_tol);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=e554bad553045546 input=a9049054013a1b77]*/
