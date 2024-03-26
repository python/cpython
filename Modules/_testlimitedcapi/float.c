#include "parts.h"
#include "util.h"


static PyObject *
float_check(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyFloat_Check(obj));
}

static PyObject *
float_checkexact(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyFloat_CheckExact(obj));
}

static PyObject *
float_fromstring(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyFloat_FromString(obj);
}

static PyObject *
float_fromdouble(PyObject *Py_UNUSED(module), PyObject *obj)
{
    double d;

    if (!PyArg_Parse(obj, "d", &d)) {
        return NULL;
    }

    return PyFloat_FromDouble(d);
}

static PyObject *
float_asdouble(PyObject *Py_UNUSED(module), PyObject *obj)
{
    double d;

    NULLABLE(obj);
    d = PyFloat_AsDouble(obj);
    if (d == -1. && PyErr_Occurred()) {
        return NULL;
    }

    return PyFloat_FromDouble(d);
}

static PyObject *
float_getinfo(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    return PyFloat_GetInfo();
}

static PyObject *
float_getmax(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    return PyFloat_FromDouble(PyFloat_GetMax());
}

static PyObject *
float_getmin(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    return PyFloat_FromDouble(PyFloat_GetMin());
}


static PyMethodDef test_methods[] = {
    {"float_check", float_check, METH_O},
    {"float_checkexact", float_checkexact, METH_O},
    {"float_fromstring", float_fromstring, METH_O},
    {"float_fromdouble", float_fromdouble, METH_O},
    {"float_asdouble", float_asdouble, METH_O},
    {"float_getinfo", float_getinfo, METH_NOARGS},
    {"float_getmax", float_getmax, METH_NOARGS},
    {"float_getmin", float_getmin, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Float(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
