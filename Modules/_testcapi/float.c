// clinic/float.c.h uses internal pycore_modsupport.h API
#define PYTESTCAPI_NEED_INTERNAL_API

#include "parts.h"
#include "util.h"
#include "clinic/float.c.h"


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

/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/

/*[clinic input]
_testcapi.float_pack

    size: int
    d: double
    le: int
    /

Test PyFloat_Pack2(), PyFloat_Pack4() and PyFloat_Pack8()
[clinic start generated code]*/

static PyObject *
_testcapi_float_pack_impl(PyObject *module, int size, double d, int le)
/*[clinic end generated code: output=7899bd98f8b6cb04 input=52c9115121999c98]*/
{
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


/*[clinic input]
_testcapi.float_unpack

    data: str(accept={robuffer}, zeroes=True)
    le: int
    /

Test PyFloat_Unpack2(), PyFloat_Unpack4() and PyFloat_Unpack8()
[clinic start generated code]*/

static PyObject *
_testcapi_float_unpack_impl(PyObject *module, const char *data,
                            Py_ssize_t data_length, int le)
/*[clinic end generated code: output=617059f889ddbfe4 input=c095e4bb75a696cd]*/
{
    assert(!PyErr_Occurred());
    double d;
    switch (data_length)
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
    {"float_check", float_check, METH_O},
    {"float_checkexact", float_checkexact, METH_O},
    {"float_fromstring", float_fromstring, METH_O},
    {"float_fromdouble", float_fromdouble, METH_O},
    {"float_asdouble", float_asdouble, METH_O},
    {"float_getinfo", float_getinfo, METH_NOARGS},
    {"float_getmax", float_getmax, METH_NOARGS},
    {"float_getmin", float_getmin, METH_NOARGS},
    _TESTCAPI_FLOAT_PACK_METHODDEF
    _TESTCAPI_FLOAT_UNPACK_METHODDEF
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
