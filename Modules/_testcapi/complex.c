#include "parts.h"
#include "util.h"
#define Py_BUILD_CORE
#include "pycore_pymath.h"        // _Py_ADJUST_ERANGE2()


static PyObject *
complex_check(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyComplex_Check(obj));
}

static PyObject *
complex_checkexact(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyComplex_CheckExact(obj));
}

static PyObject *
complex_fromccomplex(PyObject *Py_UNUSED(module), PyObject *obj)
{
    Py_complex complex;

    if (!PyArg_Parse(obj, "D", &complex)) {
        return NULL;
    }

    return PyComplex_FromCComplex(complex);
}

static PyObject *
complex_fromdoubles(PyObject *Py_UNUSED(module), PyObject *args)
{
    double real, imag;

    if (!PyArg_ParseTuple(args, "dd", &real, &imag)) {
        return NULL;
    }

    return PyComplex_FromDoubles(real, imag);
}

static PyObject *
complex_realasdouble(PyObject *Py_UNUSED(module), PyObject *obj)
{
    double real;

    NULLABLE(obj);
    real = PyComplex_RealAsDouble(obj);

    if (real == -1. && PyErr_Occurred()) {
        return NULL;
    }

    return PyFloat_FromDouble(real);
}

static PyObject *
complex_imagasdouble(PyObject *Py_UNUSED(module), PyObject *obj)
{
    double imag;

    NULLABLE(obj);
    imag = PyComplex_ImagAsDouble(obj);

    if (imag == -1. && PyErr_Occurred()) {
        return NULL;
    }

    return PyFloat_FromDouble(imag);
}

static PyObject *
complex_asccomplex(PyObject *Py_UNUSED(module), PyObject *obj)
{
    Py_complex complex;

    NULLABLE(obj);
    complex = PyComplex_AsCComplex(obj);

    if (complex.real == -1. && PyErr_Occurred()) {
        return NULL;
    }

    return PyComplex_FromCComplex(complex);
}

static PyObject*
_py_c_neg(PyObject *Py_UNUSED(module), PyObject *num)
{
    Py_complex complex;

    complex = PyComplex_AsCComplex(num);
    if (complex.real == -1. && PyErr_Occurred()) {
        return NULL;
    }

    return PyComplex_FromCComplex(_Py_c_neg(complex));
}

#define _PY_C_FUNC2(suffix)                                      \
    static PyObject *                                            \
    _py_c_##suffix(PyObject *Py_UNUSED(module), PyObject *args)  \
    {                                                            \
        Py_complex num, exp;                                     \
        PyObject *res, *err;                                     \
                                                                 \
        if (!PyArg_ParseTuple(args, "DD", &num, &exp)) {         \
            return NULL;                                         \
        }                                                        \
                                                                 \
        errno = 0;                                               \
        num = _Py_c_##suffix(num, exp);                          \
        _Py_ADJUST_ERANGE2(num.real, num.imag);                  \
                                                                 \
        res = PyComplex_FromCComplex(num);                       \
        if (!res) {                                              \
            return NULL;                                         \
        }                                                        \
        err = PyLong_FromLong(errno);                            \
        if (!err) {                                              \
            return NULL;                                         \
        }                                                        \
                                                                 \
        return PyTuple_Pack(2, res, err);                        \
    };

_PY_C_FUNC2(sum)
_PY_C_FUNC2(diff)
_PY_C_FUNC2(prod)
_PY_C_FUNC2(quot)
_PY_C_FUNC2(pow)

static PyObject*
_py_c_abs(PyObject *Py_UNUSED(module), PyObject* obj)
{
    Py_complex complex;
    PyObject *res, *err;
    double val;

    NULLABLE(obj);
    complex = PyComplex_AsCComplex(obj);

    if (complex.real == -1. && PyErr_Occurred()) {
        return NULL;
    }

    errno = 0;
    val = _Py_c_abs(complex);

    res = PyFloat_FromDouble(val);
    if (!res) {
        return NULL;
    }
    err = PyLong_FromLong(errno);
    if (!err) {
        return NULL;
    }

    return PyTuple_Pack(2, res, err);
}


static PyMethodDef test_methods[] = {
    {"complex_check", complex_check, METH_O},
    {"complex_checkexact", complex_checkexact, METH_O},
    {"complex_fromccomplex", complex_fromccomplex, METH_O},
    {"complex_fromdoubles", complex_fromdoubles, METH_VARARGS},
    {"complex_realasdouble", complex_realasdouble, METH_O},
    {"complex_imagasdouble", complex_imagasdouble, METH_O},
    {"complex_asccomplex", complex_asccomplex, METH_O},
    {"_py_c_sum", _py_c_sum, METH_VARARGS},
    {"_py_c_diff", _py_c_diff, METH_VARARGS},
    {"_py_c_neg", _py_c_neg, METH_O},
    {"_py_c_prod", _py_c_prod, METH_VARARGS},
    {"_py_c_quot", _py_c_quot, METH_VARARGS},
    {"_py_c_pow", _py_c_pow, METH_VARARGS},
    {"_py_c_abs", _py_c_abs, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Complex(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
