#include "parts.h"
#include "util.h"


static PyObject *
number_check(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyNumber_Check(obj));
}

static PyObject *
number_add(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Add(o1, o2);
}

static PyObject *
number_subtract(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Subtract(o1, o2);
}

static PyObject *
number_multiply(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Multiply(o1, o2);
}

static PyObject *
number_matrixmultiply(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_MatrixMultiply(o1, o2);
}

static PyObject *
number_floordivide(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_FloorDivide(o1, o2);
}

static PyObject *
number_truedivide(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_TrueDivide(o1, o2);
}

static PyObject *
number_remainder(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Remainder(o1, o2);
}

static PyObject *
number_divmod(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Divmod(o1, o2);
}

static PyObject *
number_power(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2, *o3;

    if (!PyArg_ParseTuple(args, "OOO", &o1, &o2, &o3)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Power(o1, o2, o3);
}

static PyObject *
number_negative(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyNumber_Negative(obj);
}

static PyObject *
number_positive(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyNumber_Positive(obj);
}

static PyObject *
number_absolute(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyNumber_Absolute(obj);
}

static PyObject *
number_invert(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyNumber_Invert(obj);
}

static PyObject *
number_lshift(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Lshift(o1, o2);
}

static PyObject *
number_rshift(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Rshift(o1, o2);
}

static PyObject *
number_and(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_And(o1, o2);
}

static PyObject *
number_xor(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Xor(o1, o2);
}

static PyObject *
number_or(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_Or(o1, o2);
}

static PyObject *
number_inplaceadd(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceAdd(o1, o2);
}

static PyObject *
number_inplacesubtract(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceSubtract(o1, o2);
}

static PyObject *
number_inplacemultiply(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceMultiply(o1, o2);
}

static PyObject *
number_inplacematrixmultiply(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceMatrixMultiply(o1, o2);
}

static PyObject *
number_inplacefloordivide(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceFloorDivide(o1, o2);
}

static PyObject *
number_inplacetruedivide(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceTrueDivide(o1, o2);
}

static PyObject *
number_inplaceremainder(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceRemainder(o1, o2);
}

static PyObject *
number_inplacepower(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2, *o3;

    if (!PyArg_ParseTuple(args, "OOO", &o1, &o2, &o3)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    NULLABLE(o3);
    return PyNumber_InPlacePower(o1, o2, o3);
}

static PyObject *
number_inplacelshift(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceLshift(o1, o2);
}

static PyObject *
number_inplacershift(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceRshift(o1, o2);
}

static PyObject *
number_inplaceand(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceAnd(o1, o2);
}

static PyObject *
number_inplacexor(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceXor(o1, o2);
}

static PyObject *
number_inplaceor(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;

    if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {
        return NULL;
    }

    NULLABLE(o1);
    NULLABLE(o2);
    return PyNumber_InPlaceOr(o1, o2);
}

static PyObject *
number_long(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyNumber_Long(obj);
}

static PyObject *
number_float(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyNumber_Float(obj);
}

static PyObject *
number_index(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyNumber_Index(obj);
}

static PyObject *
number_tobase(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *n;
    int base;

    if (!PyArg_ParseTuple(args, "Oi", &n, &base)) {
        return NULL;
    }

    NULLABLE(n);
    return PyNumber_ToBase(n, base);
}

static PyObject *
number_asssizet(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *exc;
    Py_ssize_t ret;

    if (!PyArg_ParseTuple(args, "OO", &o, &exc)) {
        return NULL;
    }

    NULLABLE(o);
    NULLABLE(exc);
    ret = PyNumber_AsSsize_t(o, exc);

    if (ret == (Py_ssize_t)(-1) && PyErr_Occurred()) {
        return NULL;
    }

    return PyLong_FromSsize_t(ret);
}


static PyMethodDef test_methods[] = {
    {"number_check", number_check, METH_O},
    {"number_add", number_add, METH_VARARGS},
    {"number_subtract", number_subtract, METH_VARARGS},
    {"number_multiply", number_multiply, METH_VARARGS},
    {"number_matrixmultiply", number_matrixmultiply, METH_VARARGS},
    {"number_floordivide", number_floordivide, METH_VARARGS},
    {"number_truedivide", number_truedivide, METH_VARARGS},
    {"number_remainder", number_remainder, METH_VARARGS},
    {"number_divmod", number_divmod, METH_VARARGS},
    {"number_power", number_power, METH_VARARGS},
    {"number_negative", number_negative, METH_O},
    {"number_positive", number_positive, METH_O},
    {"number_absolute", number_absolute, METH_O},
    {"number_invert", number_invert, METH_O},
    {"number_lshift", number_lshift, METH_VARARGS},
    {"number_rshift", number_rshift, METH_VARARGS},
    {"number_and", number_and, METH_VARARGS},
    {"number_xor", number_xor, METH_VARARGS},
    {"number_or", number_or, METH_VARARGS},
    {"number_inplaceadd", number_inplaceadd, METH_VARARGS},
    {"number_inplacesubtract", number_inplacesubtract, METH_VARARGS},
    {"number_inplacemultiply", number_inplacemultiply, METH_VARARGS},
    {"number_inplacematrixmultiply", number_inplacematrixmultiply, METH_VARARGS},
    {"number_inplacefloordivide", number_inplacefloordivide, METH_VARARGS},
    {"number_inplacetruedivide", number_inplacetruedivide, METH_VARARGS},
    {"number_inplaceremainder", number_inplaceremainder, METH_VARARGS},
    {"number_inplacepower", number_inplacepower, METH_VARARGS},
    {"number_inplacelshift", number_inplacelshift, METH_VARARGS},
    {"number_inplacershift", number_inplacershift, METH_VARARGS},
    {"number_inplaceand", number_inplaceand, METH_VARARGS},
    {"number_inplacexor", number_inplacexor, METH_VARARGS},
    {"number_inplaceor", number_inplaceor, METH_VARARGS},
    {"number_long", number_long, METH_O},
    {"number_float", number_float, METH_O},
    {"number_index", number_index, METH_O},
    {"number_tobase", number_tobase, METH_VARARGS},
    {"number_asssizet", number_asssizet, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Numbers(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
