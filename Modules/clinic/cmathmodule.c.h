/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(cmath_acos__doc__,
"acos($module, z, /)\n"
"--\n"
"\n"
"Return the arc cosine of z.");

#define CMATH_ACOS_METHODDEF    \
    {"acos", (PyCFunction)cmath_acos, METH_O, cmath_acos__doc__},

static Py_complex
cmath_acos_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_acos(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_acos_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_acosh__doc__,
"acosh($module, z, /)\n"
"--\n"
"\n"
"Return the inverse hyperbolic cosine of z.");

#define CMATH_ACOSH_METHODDEF    \
    {"acosh", (PyCFunction)cmath_acosh, METH_O, cmath_acosh__doc__},

static Py_complex
cmath_acosh_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_acosh(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_acosh_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_asin__doc__,
"asin($module, z, /)\n"
"--\n"
"\n"
"Return the arc sine of z.");

#define CMATH_ASIN_METHODDEF    \
    {"asin", (PyCFunction)cmath_asin, METH_O, cmath_asin__doc__},

static Py_complex
cmath_asin_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_asin(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_asin_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_asinh__doc__,
"asinh($module, z, /)\n"
"--\n"
"\n"
"Return the inverse hyperbolic sine of z.");

#define CMATH_ASINH_METHODDEF    \
    {"asinh", (PyCFunction)cmath_asinh, METH_O, cmath_asinh__doc__},

static Py_complex
cmath_asinh_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_asinh(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_asinh_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_atan__doc__,
"atan($module, z, /)\n"
"--\n"
"\n"
"Return the arc tangent of z.");

#define CMATH_ATAN_METHODDEF    \
    {"atan", (PyCFunction)cmath_atan, METH_O, cmath_atan__doc__},

static Py_complex
cmath_atan_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_atan(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_atan_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_atanh__doc__,
"atanh($module, z, /)\n"
"--\n"
"\n"
"Return the inverse hyperbolic tangent of z.");

#define CMATH_ATANH_METHODDEF    \
    {"atanh", (PyCFunction)cmath_atanh, METH_O, cmath_atanh__doc__},

static Py_complex
cmath_atanh_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_atanh(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_atanh_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_cos__doc__,
"cos($module, z, /)\n"
"--\n"
"\n"
"Return the cosine of z.");

#define CMATH_COS_METHODDEF    \
    {"cos", (PyCFunction)cmath_cos, METH_O, cmath_cos__doc__},

static Py_complex
cmath_cos_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_cos(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_cos_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_cosh__doc__,
"cosh($module, z, /)\n"
"--\n"
"\n"
"Return the hyperbolic cosine of z.");

#define CMATH_COSH_METHODDEF    \
    {"cosh", (PyCFunction)cmath_cosh, METH_O, cmath_cosh__doc__},

static Py_complex
cmath_cosh_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_cosh(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_cosh_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_exp__doc__,
"exp($module, z, /)\n"
"--\n"
"\n"
"Return the exponential value e**z.");

#define CMATH_EXP_METHODDEF    \
    {"exp", (PyCFunction)cmath_exp, METH_O, cmath_exp__doc__},

static Py_complex
cmath_exp_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_exp(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_exp_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_log10__doc__,
"log10($module, z, /)\n"
"--\n"
"\n"
"Return the base-10 logarithm of z.");

#define CMATH_LOG10_METHODDEF    \
    {"log10", (PyCFunction)cmath_log10, METH_O, cmath_log10__doc__},

static Py_complex
cmath_log10_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_log10(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_log10_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_sin__doc__,
"sin($module, z, /)\n"
"--\n"
"\n"
"Return the sine of z.");

#define CMATH_SIN_METHODDEF    \
    {"sin", (PyCFunction)cmath_sin, METH_O, cmath_sin__doc__},

static Py_complex
cmath_sin_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_sin(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_sin_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_sinh__doc__,
"sinh($module, z, /)\n"
"--\n"
"\n"
"Return the hyperbolic sine of z.");

#define CMATH_SINH_METHODDEF    \
    {"sinh", (PyCFunction)cmath_sinh, METH_O, cmath_sinh__doc__},

static Py_complex
cmath_sinh_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_sinh(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_sinh_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_sqrt__doc__,
"sqrt($module, z, /)\n"
"--\n"
"\n"
"Return the square root of z.");

#define CMATH_SQRT_METHODDEF    \
    {"sqrt", (PyCFunction)cmath_sqrt, METH_O, cmath_sqrt__doc__},

static Py_complex
cmath_sqrt_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_sqrt(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_sqrt_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_tan__doc__,
"tan($module, z, /)\n"
"--\n"
"\n"
"Return the tangent of z.");

#define CMATH_TAN_METHODDEF    \
    {"tan", (PyCFunction)cmath_tan, METH_O, cmath_tan__doc__},

static Py_complex
cmath_tan_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_tan(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_tan_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_tanh__doc__,
"tanh($module, z, /)\n"
"--\n"
"\n"
"Return the hyperbolic tangent of z.");

#define CMATH_TANH_METHODDEF    \
    {"tanh", (PyCFunction)cmath_tanh, METH_O, cmath_tanh__doc__},

static Py_complex
cmath_tanh_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_tanh(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0; PyFPE_START_PROTECT("complex function", goto exit);
    _return_value = cmath_tanh_impl(module, z);
    PyFPE_END_PROTECT(_return_value);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = PyComplex_FromCComplex(_return_value);
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_log__doc__,
"log($module, z, base=<unrepresentable>, /)\n"
"--\n"
"\n"
"log(z[, base]) -> the logarithm of z to the given base.\n"
"\n"
"If the base not specified, returns the natural logarithm (base e) of z.");

#define CMATH_LOG_METHODDEF    \
    {"log", (PyCFunction)(void(*)(void))cmath_log, METH_FASTCALL, cmath_log__doc__},

static PyObject *
cmath_log_impl(PyObject *module, Py_complex x, PyObject *y_obj);

static PyObject *
cmath_log(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_complex x;
    PyObject *y_obj = NULL;

    if (!_PyArg_CheckPositional("log", nargs, 1, 2)) {
        goto exit;
    }
    x = PyComplex_AsCComplex(args[0]);
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    y_obj = args[1];
skip_optional:
    return_value = cmath_log_impl(module, x, y_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_phase__doc__,
"phase($module, z, /)\n"
"--\n"
"\n"
"Return argument, also known as the phase angle, of a complex.");

#define CMATH_PHASE_METHODDEF    \
    {"phase", (PyCFunction)cmath_phase, METH_O, cmath_phase__doc__},

static PyObject *
cmath_phase_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_phase(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_phase_impl(module, z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_polar__doc__,
"polar($module, z, /)\n"
"--\n"
"\n"
"Convert a complex from rectangular coordinates to polar coordinates.\n"
"\n"
"r is the distance from 0 and phi the phase angle.");

#define CMATH_POLAR_METHODDEF    \
    {"polar", (PyCFunction)cmath_polar, METH_O, cmath_polar__doc__},

static PyObject *
cmath_polar_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_polar(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_polar_impl(module, z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_rect__doc__,
"rect($module, r, phi, /)\n"
"--\n"
"\n"
"Convert from polar coordinates to rectangular coordinates.");

#define CMATH_RECT_METHODDEF    \
    {"rect", (PyCFunction)(void(*)(void))cmath_rect, METH_FASTCALL, cmath_rect__doc__},

static PyObject *
cmath_rect_impl(PyObject *module, double r, double phi);

static PyObject *
cmath_rect(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double r;
    double phi;

    if (!_PyArg_CheckPositional("rect", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        r = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        r = PyFloat_AsDouble(args[0]);
        if (r == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        phi = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        phi = PyFloat_AsDouble(args[1]);
        if (phi == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = cmath_rect_impl(module, r, phi);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isfinite__doc__,
"isfinite($module, z, /)\n"
"--\n"
"\n"
"Return True if both the real and imaginary parts of z are finite, else False.");

#define CMATH_ISFINITE_METHODDEF    \
    {"isfinite", (PyCFunction)cmath_isfinite, METH_O, cmath_isfinite__doc__},

static PyObject *
cmath_isfinite_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_isfinite(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_isfinite_impl(module, z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isnan__doc__,
"isnan($module, z, /)\n"
"--\n"
"\n"
"Checks if the real or imaginary part of z not a number (NaN).");

#define CMATH_ISNAN_METHODDEF    \
    {"isnan", (PyCFunction)cmath_isnan, METH_O, cmath_isnan__doc__},

static PyObject *
cmath_isnan_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_isnan(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_isnan_impl(module, z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isinf__doc__,
"isinf($module, z, /)\n"
"--\n"
"\n"
"Checks if the real or imaginary part of z is infinite.");

#define CMATH_ISINF_METHODDEF    \
    {"isinf", (PyCFunction)cmath_isinf, METH_O, cmath_isinf__doc__},

static PyObject *
cmath_isinf_impl(PyObject *module, Py_complex z);

static PyObject *
cmath_isinf(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_complex z;

    z = PyComplex_AsCComplex(arg);
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_isinf_impl(module, z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isclose__doc__,
"isclose($module, /, a, b, *, rel_tol=1e-09, abs_tol=0.0)\n"
"--\n"
"\n"
"Determine whether two complex numbers are close in value.\n"
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
"For the values to be considered close, the difference between them must be\n"
"smaller than at least one of the tolerances.\n"
"\n"
"-inf, inf and NaN behave similarly to the IEEE 754 Standard. That is, NaN is\n"
"not close to anything, even itself. inf and -inf are only close to themselves.");

#define CMATH_ISCLOSE_METHODDEF    \
    {"isclose", (PyCFunction)(void(*)(void))cmath_isclose, METH_FASTCALL|METH_KEYWORDS, cmath_isclose__doc__},

static int
cmath_isclose_impl(PyObject *module, Py_complex a, Py_complex b,
                   double rel_tol, double abs_tol);

static PyObject *
cmath_isclose(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"a", "b", "rel_tol", "abs_tol", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "isclose", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    Py_complex a;
    Py_complex b;
    double rel_tol = 1e-09;
    double abs_tol = 0.0;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyComplex_AsCComplex(args[0]);
    if (PyErr_Occurred()) {
        goto exit;
    }
    b = PyComplex_AsCComplex(args[1]);
    if (PyErr_Occurred()) {
        goto exit;
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
    _return_value = cmath_isclose_impl(module, a, b, rel_tol, abs_tol);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=3edc4484b10ae752 input=a9049054013a1b77]*/
