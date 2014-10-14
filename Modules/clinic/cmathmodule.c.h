/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(cmath_acos__doc__,
"acos($module, z, /)\n"
"--\n"
"\n"
"Return the arc cosine of z.");

#define CMATH_ACOS_METHODDEF    \
    {"acos", (PyCFunction)cmath_acos, METH_VARARGS, cmath_acos__doc__},

static Py_complex
cmath_acos_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_acos(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:acos",
        &z))
        goto exit;
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
"Return the hyperbolic arccosine of z.");

#define CMATH_ACOSH_METHODDEF    \
    {"acosh", (PyCFunction)cmath_acosh, METH_VARARGS, cmath_acosh__doc__},

static Py_complex
cmath_acosh_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_acosh(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:acosh",
        &z))
        goto exit;
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
    {"asin", (PyCFunction)cmath_asin, METH_VARARGS, cmath_asin__doc__},

static Py_complex
cmath_asin_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_asin(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:asin",
        &z))
        goto exit;
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
"Return the hyperbolic arc sine of z.");

#define CMATH_ASINH_METHODDEF    \
    {"asinh", (PyCFunction)cmath_asinh, METH_VARARGS, cmath_asinh__doc__},

static Py_complex
cmath_asinh_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_asinh(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:asinh",
        &z))
        goto exit;
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
    {"atan", (PyCFunction)cmath_atan, METH_VARARGS, cmath_atan__doc__},

static Py_complex
cmath_atan_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_atan(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:atan",
        &z))
        goto exit;
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
"Return the hyperbolic arc tangent of z.");

#define CMATH_ATANH_METHODDEF    \
    {"atanh", (PyCFunction)cmath_atanh, METH_VARARGS, cmath_atanh__doc__},

static Py_complex
cmath_atanh_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_atanh(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:atanh",
        &z))
        goto exit;
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
    {"cos", (PyCFunction)cmath_cos, METH_VARARGS, cmath_cos__doc__},

static Py_complex
cmath_cos_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_cos(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:cos",
        &z))
        goto exit;
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
    {"cosh", (PyCFunction)cmath_cosh, METH_VARARGS, cmath_cosh__doc__},

static Py_complex
cmath_cosh_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_cosh(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:cosh",
        &z))
        goto exit;
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
    {"exp", (PyCFunction)cmath_exp, METH_VARARGS, cmath_exp__doc__},

static Py_complex
cmath_exp_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_exp(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:exp",
        &z))
        goto exit;
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
    {"log10", (PyCFunction)cmath_log10, METH_VARARGS, cmath_log10__doc__},

static Py_complex
cmath_log10_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_log10(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:log10",
        &z))
        goto exit;
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
    {"sin", (PyCFunction)cmath_sin, METH_VARARGS, cmath_sin__doc__},

static Py_complex
cmath_sin_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_sin(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:sin",
        &z))
        goto exit;
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
    {"sinh", (PyCFunction)cmath_sinh, METH_VARARGS, cmath_sinh__doc__},

static Py_complex
cmath_sinh_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_sinh(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:sinh",
        &z))
        goto exit;
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
    {"sqrt", (PyCFunction)cmath_sqrt, METH_VARARGS, cmath_sqrt__doc__},

static Py_complex
cmath_sqrt_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_sqrt(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:sqrt",
        &z))
        goto exit;
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
    {"tan", (PyCFunction)cmath_tan, METH_VARARGS, cmath_tan__doc__},

static Py_complex
cmath_tan_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_tan(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:tan",
        &z))
        goto exit;
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
    {"tanh", (PyCFunction)cmath_tanh, METH_VARARGS, cmath_tanh__doc__},

static Py_complex
cmath_tanh_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_tanh(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;
    Py_complex _return_value;

    if (!PyArg_ParseTuple(args,
        "D:tanh",
        &z))
        goto exit;
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
"log($module, x, y_obj=None, /)\n"
"--\n"
"\n"
"The logarithm of z to the given base.\n"
"\n"
"If the base not specified, returns the natural logarithm (base e) of z.");

#define CMATH_LOG_METHODDEF    \
    {"log", (PyCFunction)cmath_log, METH_VARARGS, cmath_log__doc__},

static PyObject *
cmath_log_impl(PyModuleDef *module, Py_complex x, PyObject *y_obj);

static PyObject *
cmath_log(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex x;
    PyObject *y_obj = NULL;

    if (!PyArg_ParseTuple(args,
        "D|O:log",
        &x, &y_obj))
        goto exit;
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
    {"phase", (PyCFunction)cmath_phase, METH_VARARGS, cmath_phase__doc__},

static PyObject *
cmath_phase_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_phase(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;

    if (!PyArg_ParseTuple(args,
        "D:phase",
        &z))
        goto exit;
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
    {"polar", (PyCFunction)cmath_polar, METH_VARARGS, cmath_polar__doc__},

static PyObject *
cmath_polar_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_polar(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;

    if (!PyArg_ParseTuple(args,
        "D:polar",
        &z))
        goto exit;
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
    {"rect", (PyCFunction)cmath_rect, METH_VARARGS, cmath_rect__doc__},

static PyObject *
cmath_rect_impl(PyModuleDef *module, double r, double phi);

static PyObject *
cmath_rect(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    double r;
    double phi;

    if (!PyArg_ParseTuple(args,
        "dd:rect",
        &r, &phi))
        goto exit;
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
    {"isfinite", (PyCFunction)cmath_isfinite, METH_VARARGS, cmath_isfinite__doc__},

static PyObject *
cmath_isfinite_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_isfinite(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;

    if (!PyArg_ParseTuple(args,
        "D:isfinite",
        &z))
        goto exit;
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
    {"isnan", (PyCFunction)cmath_isnan, METH_VARARGS, cmath_isnan__doc__},

static PyObject *
cmath_isnan_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_isnan(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;

    if (!PyArg_ParseTuple(args,
        "D:isnan",
        &z))
        goto exit;
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
    {"isinf", (PyCFunction)cmath_isinf, METH_VARARGS, cmath_isinf__doc__},

static PyObject *
cmath_isinf_impl(PyModuleDef *module, Py_complex z);

static PyObject *
cmath_isinf(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_complex z;

    if (!PyArg_ParseTuple(args,
        "D:isinf",
        &z))
        goto exit;
    return_value = cmath_isinf_impl(module, z);

exit:
    return return_value;
}
/*[clinic end generated code: output=4407f898ae07c83d input=a9049054013a1b77]*/
