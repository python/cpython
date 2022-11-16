#define PY_SSIZE_T_CLEAN

#include "parts.h"


// Test PyFloat_Pack2(), PyFloat_Pack4() and PyFloat_Pack8()
static PyObject *
test_float_pack(PyObject *self, PyObject *args)
{
    int size;
    double d;
    int le;
    if (!PyArg_ParseTuple(args, "idi", &size, &d, &le)) {
        return NULL;
    }
    switch (size)
    {
    case 2:
    {
        char data[2];
        if (PyFloat_Pack2(d, data, le) < 0) {
            return NULL;
        }
        return PyBytes_FromStringAndSize(data, Py_ARRAY_LENGTH(data));
    }
    case 4:
    {
        char data[4];
        if (PyFloat_Pack4(d, data, le) < 0) {
            return NULL;
        }
        return PyBytes_FromStringAndSize(data, Py_ARRAY_LENGTH(data));
    }
    case 8:
    {
        char data[8];
        if (PyFloat_Pack8(d, data, le) < 0) {
            return NULL;
        }
        return PyBytes_FromStringAndSize(data, Py_ARRAY_LENGTH(data));
    }
    default: break;
    }

    PyErr_SetString(PyExc_ValueError, "size must 2, 4 or 8");
    return NULL;
}


// Test PyFloat_Unpack2(), PyFloat_Unpack4() and PyFloat_Unpack8()
static PyObject *
test_float_unpack(PyObject *self, PyObject *args)
{
    assert(!PyErr_Occurred());
    const char *data;
    Py_ssize_t size;
    int le;
    if (!PyArg_ParseTuple(args, "y#i", &data, &size, &le)) {
        return NULL;
    }
    double d;
    switch (size)
    {
    case 2:
        d = PyFloat_Unpack2(data, le);
        break;
    case 4:
        d = PyFloat_Unpack4(data, le);
        break;
    case 8:
        d = PyFloat_Unpack8(data, le);
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "data length must 2, 4 or 8 bytes");
        return NULL;
    }

    if (d == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    return PyFloat_FromDouble(d);
}

static PyMethodDef test_methods[] = {
    {"float_pack", test_float_pack, METH_VARARGS, NULL},
    {"float_unpack", test_float_unpack, METH_VARARGS, NULL},
    {NULL},
};

int
_PyTestCapi_Init_Float(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
