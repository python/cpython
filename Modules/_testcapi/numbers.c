#include "parts.h"
#include "util.h"


static PyObject *
number_check(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyNumber_Check(obj));
}

#define BINARYFUNC(funcsuffix, methsuffix)                           \
    static PyObject *                                                \
    number_##methsuffix(PyObject *Py_UNUSED(module), PyObject *args) \
    {                                                                \
        PyObject *o1, *o2;                                           \
                                                                     \
        if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) {               \
            return NULL;                                             \
        }                                                            \
                                                                     \
        NULLABLE(o1);                                                \
        NULLABLE(o2);                                                \
        return PyNumber_##funcsuffix(o1, o2);                        \
    };

BINARYFUNC(Add, add)
BINARYFUNC(Subtract, subtract)
BINARYFUNC(Multiply, multiply)
BINARYFUNC(MatrixMultiply, matrixmultiply)
BINARYFUNC(FloorDivide, floordivide)
BINARYFUNC(TrueDivide, truedivide)
BINARYFUNC(Remainder, remainder)
BINARYFUNC(Divmod, divmod)

#define TERNARYFUNC(funcsuffix, methsuffix)                          \
    static PyObject *                                                \
    number_##methsuffix(PyObject *Py_UNUSED(module), PyObject *args) \
    {                                                                \
        PyObject *o1, *o2, *o3 = Py_None;                            \
                                                                     \
        if (!PyArg_ParseTuple(args, "OO|O", &o1, &o2, &o3)) {        \
            return NULL;                                             \
        }                                                            \
                                                                     \
        NULLABLE(o1);                                                \
        NULLABLE(o2);                                                \
        return PyNumber_##funcsuffix(o1, o2, o3);                    \
    };

TERNARYFUNC(Power, power)

#define UNARYFUNC(funcsuffix, methsuffix)                            \
    static PyObject *                                                \
    number_##methsuffix(PyObject *Py_UNUSED(module), PyObject *obj)  \
    {                                                                \
        NULLABLE(obj);                                               \
        return PyNumber_##funcsuffix(obj);                           \
    };

UNARYFUNC(Negative, negative)
UNARYFUNC(Positive, positive)
UNARYFUNC(Absolute, absolute)
UNARYFUNC(Invert, invert)

BINARYFUNC(Lshift, lshift)
BINARYFUNC(Rshift, rshift)
BINARYFUNC(And, and)
BINARYFUNC(Xor, xor)
BINARYFUNC(Or, or)

BINARYFUNC(InPlaceAdd, inplaceadd)
BINARYFUNC(InPlaceSubtract, inplacesubtract)
BINARYFUNC(InPlaceMultiply, inplacemultiply)
BINARYFUNC(InPlaceMatrixMultiply, inplacematrixmultiply)
BINARYFUNC(InPlaceFloorDivide, inplacefloordivide)
BINARYFUNC(InPlaceTrueDivide, inplacetruedivide)
BINARYFUNC(InPlaceRemainder, inplaceremainder)

TERNARYFUNC(InPlacePower, inplacepower)

BINARYFUNC(InPlaceLshift, inplacelshift)
BINARYFUNC(InPlaceRshift, inplacershift)
BINARYFUNC(InPlaceAnd, inplaceand)
BINARYFUNC(InPlaceXor, inplacexor)
BINARYFUNC(InPlaceOr, inplaceor)

UNARYFUNC(Long, long)
UNARYFUNC(Float, float)
UNARYFUNC(Index, index)

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
