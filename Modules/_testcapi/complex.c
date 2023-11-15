#include "parts.h"
#include "util.h"


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


static PyObject *
test_py_complex(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Basic tests on Py_complex functions:
    //
    // - Py_complex_sum()
    // - Py_complex_diff()
    // - Py_complex_neg()
    // - Py_complex_prod()
    // - Py_complex_quot()
    // - Py_complex_pow()
    // - Py_complex_abs()

    Py_complex a = {1.0, 5.0};
    Py_complex b = {1.5, 0.5};
    Py_complex x = Py_complex_sum(a, b);
    assert(x.real == 2.5 && x.imag == 5.5);

    x = Py_complex_diff(a, b);
    assert(x.real == -0.5 && x.imag == 4.5);

    x = Py_complex_neg(a);
    assert(x.real == -1.0 && x.imag == -5.0);

    a = (Py_complex){1.0, -2.0};
    b = (Py_complex){3.0, 4.0};
    x = Py_complex_prod(a, b);
    assert(x.real == 11.0 && x.imag == -2.0);

    a = (Py_complex){6.9, -3.2};
    x = Py_complex_quot(a, a);
    assert(x.real == 1.0 && x.imag == 0.0);

    a = (Py_complex){1.3, 2.7};
    Py_complex zero = {0.0, 0.0};
    x = Py_complex_pow(a, zero);
    assert(x.real == 1.0 && x.imag == 0.0);

    x = (Py_complex){3.0, 4.0};
    double mod = Py_complex_abs(x);
    assert(mod == hypot(3.0, 4.0));

    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"complex_check", complex_check, METH_O},
    {"complex_checkexact", complex_checkexact, METH_O},
    {"complex_fromccomplex", complex_fromccomplex, METH_O},
    {"complex_fromdoubles", complex_fromdoubles, METH_VARARGS},
    {"complex_realasdouble", complex_realasdouble, METH_O},
    {"complex_imagasdouble", complex_imagasdouble, METH_O},
    {"complex_asccomplex", complex_asccomplex, METH_O},
    {"test_py_complex", test_py_complex, METH_NOARGS},
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
