/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

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

PyDoc_STRVAR(math_fmax__doc__,
"fmax($module, x, y, /)\n"
"--\n"
"\n"
"Return the larger of two floating-point arguments.");

#define MATH_FMAX_METHODDEF    \
    {"fmax", _PyCFunction_CAST(math_fmax), METH_FASTCALL, math_fmax__doc__},

static double
math_fmax_impl(PyObject *module, double x, double y);

static PyObject *
math_fmax(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;
    double _return_value;

    if (!_PyArg_CheckPositional("fmax", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        x = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        x = PyFloat_AsDouble(args[0]);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        y = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        y = PyFloat_AsDouble(args[1]);
        if (y == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = math_fmax_impl(module, x, y);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(math_fmin__doc__,
"fmin($module, x, y, /)\n"
"--\n"
"\n"
"Return the smaller of two floating-point arguments.");

#define MATH_FMIN_METHODDEF    \
    {"fmin", _PyCFunction_CAST(math_fmin), METH_FASTCALL, math_fmin__doc__},

static double
math_fmin_impl(PyObject *module, double x, double y);

static PyObject *
math_fmin(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;
    double _return_value;

    if (!_PyArg_CheckPositional("fmin", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        x = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        x = PyFloat_AsDouble(args[0]);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        y = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        y = PyFloat_AsDouble(args[1]);
        if (y == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = math_fmin_impl(module, x, y);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(math_signbit__doc__,
"signbit($module, x, /)\n"
"--\n"
"\n"
"Return True if the sign of x is negative and False otherwise.");

#define MATH_SIGNBIT_METHODDEF    \
    {"signbit", (PyCFunction)math_signbit, METH_O, math_signbit__doc__},

static PyObject *
math_signbit_impl(PyObject *module, double x);

static PyObject *
math_signbit(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_signbit_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_fsum__doc__,
"fsum($module, seq, /)\n"
"--\n"
"\n"
"Return an accurate floating-point sum of values in the iterable seq.\n"
"\n"
"Assumes IEEE-754 floating-point arithmetic.");

#define MATH_FSUM_METHODDEF    \
    {"fsum", (PyCFunction)math_fsum, METH_O, math_fsum__doc__},

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

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
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
    {"ldexp", _PyCFunction_CAST(math_ldexp), METH_FASTCALL, math_ldexp__doc__},

static PyObject *
math_ldexp_impl(PyObject *module, double x, PyObject *i);

static PyObject *
math_ldexp(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    PyObject *i;

    if (!_PyArg_CheckPositional("ldexp", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        x = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        x = PyFloat_AsDouble(args[0]);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    i = args[1];
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

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_modf_impl(module, x);

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

PyDoc_STRVAR(math_fma__doc__,
"fma($module, x, y, z, /)\n"
"--\n"
"\n"
"Fused multiply-add operation.\n"
"\n"
"Compute (x * y) + z with a single round.");

#define MATH_FMA_METHODDEF    \
    {"fma", _PyCFunction_CAST(math_fma), METH_FASTCALL, math_fma__doc__},

static PyObject *
math_fma_impl(PyObject *module, double x, double y, double z);

static PyObject *
math_fma(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;
    double z;

    if (!_PyArg_CheckPositional("fma", nargs, 3, 3)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        x = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        x = PyFloat_AsDouble(args[0]);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        y = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        y = PyFloat_AsDouble(args[1]);
        if (y == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[2])) {
        z = PyFloat_AS_DOUBLE(args[2]);
    }
    else
    {
        z = PyFloat_AsDouble(args[2]);
        if (z == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_fma_impl(module, x, y, z);

exit:
    return return_value;
}

PyDoc_STRVAR(math_fmod__doc__,
"fmod($module, x, y, /)\n"
"--\n"
"\n"
"Return fmod(x, y), according to platform C.\n"
"\n"
"x % y may differ.");

#define MATH_FMOD_METHODDEF    \
    {"fmod", _PyCFunction_CAST(math_fmod), METH_FASTCALL, math_fmod__doc__},

static PyObject *
math_fmod_impl(PyObject *module, double x, double y);

static PyObject *
math_fmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;

    if (!_PyArg_CheckPositional("fmod", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        x = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        x = PyFloat_AsDouble(args[0]);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        y = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        y = PyFloat_AsDouble(args[1]);
        if (y == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_fmod_impl(module, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(math_dist__doc__,
"dist($module, p, q, /)\n"
"--\n"
"\n"
"Return the Euclidean distance between two points p and q.\n"
"\n"
"The points should be specified as sequences (or iterables) of\n"
"coordinates.  Both inputs must have the same dimension.\n"
"\n"
"Roughly equivalent to:\n"
"    sqrt(sum((px - qx) ** 2.0 for px, qx in zip(p, q)))");

#define MATH_DIST_METHODDEF    \
    {"dist", _PyCFunction_CAST(math_dist), METH_FASTCALL, math_dist__doc__},

static PyObject *
math_dist_impl(PyObject *module, PyObject *p, PyObject *q);

static PyObject *
math_dist(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *p;
    PyObject *q;

    if (!_PyArg_CheckPositional("dist", nargs, 2, 2)) {
        goto exit;
    }
    p = args[0];
    q = args[1];
    return_value = math_dist_impl(module, p, q);

exit:
    return return_value;
}

PyDoc_STRVAR(math_hypot__doc__,
"hypot($module, /, *coordinates)\n"
"--\n"
"\n"
"Multidimensional Euclidean distance from the origin to a point.\n"
"\n"
"Roughly equivalent to:\n"
"    sqrt(sum(x**2 for x in coordinates))\n"
"\n"
"For a two dimensional point (x, y), gives the hypotenuse\n"
"using the Pythagorean theorem:  sqrt(x*x + y*y).\n"
"\n"
"For example, the hypotenuse of a 3/4/5 right triangle is:\n"
"\n"
"    >>> hypot(3.0, 4.0)\n"
"    5.0");

#define MATH_HYPOT_METHODDEF    \
    {"hypot", _PyCFunction_CAST(math_hypot), METH_FASTCALL, math_hypot__doc__},

static PyObject *
math_hypot_impl(PyObject *module, PyObject * const *args,
                Py_ssize_t args_length);

static PyObject *
math_hypot(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject * const *__clinic_args;
    Py_ssize_t args_length;

    __clinic_args = args;
    args_length = nargs;
    return_value = math_hypot_impl(module, __clinic_args, args_length);

    return return_value;
}

PyDoc_STRVAR(math_sumprod__doc__,
"sumprod($module, p, q, /)\n"
"--\n"
"\n"
"Return the sum of products of values from two iterables p and q.\n"
"\n"
"Roughly equivalent to:\n"
"\n"
"    sum(map(operator.mul, p, q, strict=True))\n"
"\n"
"For float and mixed int/float inputs, the intermediate products\n"
"and sums are computed with extended precision.");

#define MATH_SUMPROD_METHODDEF    \
    {"sumprod", _PyCFunction_CAST(math_sumprod), METH_FASTCALL, math_sumprod__doc__},

static PyObject *
math_sumprod_impl(PyObject *module, PyObject *p, PyObject *q);

static PyObject *
math_sumprod(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *p;
    PyObject *q;

    if (!_PyArg_CheckPositional("sumprod", nargs, 2, 2)) {
        goto exit;
    }
    p = args[0];
    q = args[1];
    return_value = math_sumprod_impl(module, p, q);

exit:
    return return_value;
}

PyDoc_STRVAR(math_pow__doc__,
"pow($module, x, y, /)\n"
"--\n"
"\n"
"Return x**y (x to the power of y).");

#define MATH_POW_METHODDEF    \
    {"pow", _PyCFunction_CAST(math_pow), METH_FASTCALL, math_pow__doc__},

static PyObject *
math_pow_impl(PyObject *module, double x, double y);

static PyObject *
math_pow(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double x;
    double y;

    if (!_PyArg_CheckPositional("pow", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        x = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        x = PyFloat_AsDouble(args[0]);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        y = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        y = PyFloat_AsDouble(args[1]);
        if (y == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
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

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
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

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
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

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_isfinite_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_isnormal__doc__,
"isnormal($module, x, /)\n"
"--\n"
"\n"
"Return True if x is normal, and False otherwise.");

#define MATH_ISNORMAL_METHODDEF    \
    {"isnormal", (PyCFunction)math_isnormal, METH_O, math_isnormal__doc__},

static PyObject *
math_isnormal_impl(PyObject *module, double x);

static PyObject *
math_isnormal(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_isnormal_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_issubnormal__doc__,
"issubnormal($module, x, /)\n"
"--\n"
"\n"
"Return True if x is subnormal, and False otherwise.");

#define MATH_ISSUBNORMAL_METHODDEF    \
    {"issubnormal", (PyCFunction)math_issubnormal, METH_O, math_issubnormal__doc__},

static PyObject *
math_issubnormal_impl(PyObject *module, double x);

static PyObject *
math_issubnormal(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_issubnormal_impl(module, x);

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

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
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

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = math_isinf_impl(module, x);

exit:
    return return_value;
}

PyDoc_STRVAR(math_isclose__doc__,
"isclose($module, /, a, b, *, rel_tol=1e-09, abs_tol=0.0)\n"
"--\n"
"\n"
"Determine whether two floating-point numbers are close in value.\n"
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
    {"isclose", _PyCFunction_CAST(math_isclose), METH_FASTCALL|METH_KEYWORDS, math_isclose__doc__},

static int
math_isclose_impl(PyObject *module, double a, double b, double rel_tol,
                  double abs_tol);

static PyObject *
math_isclose(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), &_Py_ID(rel_tol), &_Py_ID(abs_tol), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "rel_tol", "abs_tol", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "isclose",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    double a;
    double b;
    double rel_tol = 1e-09;
    double abs_tol = 0.0;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        a = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        a = PyFloat_AsDouble(args[0]);
        if (a == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        b = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        b = PyFloat_AsDouble(args[1]);
        if (b == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[2]) {
        if (PyFloat_CheckExact(args[2])) {
            rel_tol = PyFloat_AS_DOUBLE(args[2]);
        }
        else
        {
            rel_tol = PyFloat_AsDouble(args[2]);
            if (rel_tol == -1.0 && PyErr_Occurred()) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (PyFloat_CheckExact(args[3])) {
        abs_tol = PyFloat_AS_DOUBLE(args[3]);
    }
    else
    {
        abs_tol = PyFloat_AsDouble(args[3]);
        if (abs_tol == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
skip_optional_kwonly:
    _return_value = math_isclose_impl(module, a, b, rel_tol, abs_tol);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(math_prod__doc__,
"prod($module, iterable, /, *, start=1)\n"
"--\n"
"\n"
"Calculate the product of all the elements in the input iterable.\n"
"\n"
"The default start value for the product is 1.\n"
"\n"
"When the iterable is empty, return the start value.  This function is\n"
"intended specifically for use with numeric values and may reject\n"
"non-numeric types.");

#define MATH_PROD_METHODDEF    \
    {"prod", _PyCFunction_CAST(math_prod), METH_FASTCALL|METH_KEYWORDS, math_prod__doc__},

static PyObject *
math_prod_impl(PyObject *module, PyObject *iterable, PyObject *start);

static PyObject *
math_prod(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(start), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "start", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "prod",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *iterable;
    PyObject *start = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    iterable = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    start = args[1];
skip_optional_kwonly:
    return_value = math_prod_impl(module, iterable, start);

exit:
    return return_value;
}

PyDoc_STRVAR(math_nextafter__doc__,
"nextafter($module, x, y, /, *, steps=None)\n"
"--\n"
"\n"
"Return the floating-point value the given number of steps after x towards y.\n"
"\n"
"If steps is not specified or is None, it defaults to 1.\n"
"\n"
"Raises a TypeError, if x or y is not a double, or if steps is not an integer.\n"
"Raises ValueError if steps is negative.");

#define MATH_NEXTAFTER_METHODDEF    \
    {"nextafter", _PyCFunction_CAST(math_nextafter), METH_FASTCALL|METH_KEYWORDS, math_nextafter__doc__},

static PyObject *
math_nextafter_impl(PyObject *module, double x, double y, PyObject *steps);

static PyObject *
math_nextafter(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(steps), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "steps", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "nextafter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    double x;
    double y;
    PyObject *steps = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        x = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        x = PyFloat_AsDouble(args[0]);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        y = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        y = PyFloat_AsDouble(args[1]);
        if (y == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    steps = args[2];
skip_optional_kwonly:
    return_value = math_nextafter_impl(module, x, y, steps);

exit:
    return return_value;
}

PyDoc_STRVAR(math_ulp__doc__,
"ulp($module, x, /)\n"
"--\n"
"\n"
"Return the value of the least significant bit of the float x.");

#define MATH_ULP_METHODDEF    \
    {"ulp", (PyCFunction)math_ulp, METH_O, math_ulp__doc__},

static double
math_ulp_impl(PyObject *module, double x);

static PyObject *
math_ulp(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double x;
    double _return_value;

    if (PyFloat_CheckExact(arg)) {
        x = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        x = PyFloat_AsDouble(arg);
        if (x == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = math_ulp_impl(module, x);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=23b2453ba77453e5 input=a9049054013a1b77]*/
