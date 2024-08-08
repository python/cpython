// Need limited C API version 3.13 for Py_GetConstantBorrowed()
#include "pyconfig.h"   // Py_GIL_DISABLED
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030d0000
#endif

#include "parts.h"
#include "util.h"


static PyObject *
tuple_check(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyTuple_Check(obj));
}


static PyObject *
tuple_check_exact(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyTuple_CheckExact(obj));
}


static PyObject *
tuple_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyTuple_Size(obj));
}


static PyObject *
tuple_getitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyTuple_GetItem(obj, i));
}


static PyObject *
tuple_getslice(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t ilow, ihigh;
    if (!PyArg_ParseTuple(args, "Onn", &obj, &ilow, &ihigh)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyTuple_GetSlice(obj, ilow, ihigh);
}


static PyObject *
test_tuple_new(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Test PyTuple_New() and PyTuple_SetItem()
    PyObject *tuple = PyTuple_New(2);
    if (tuple == NULL) {
        return NULL;
    }
    assert(PyTuple_CheckExact(tuple));

    PyObject *zero = Py_GetConstantBorrowed(Py_CONSTANT_ZERO);
    PyObject *one = Py_GetConstantBorrowed(Py_CONSTANT_ONE);

    if (PyTuple_SetItem(tuple, 0, zero) < 0) {
        Py_DECREF(tuple);
        return NULL;
    }
    if (PyTuple_SetItem(tuple, 1, one) < 0) {
        Py_DECREF(tuple);
        return NULL;
    }

    assert(PyTuple_Size(tuple) == 2);
    assert(PyTuple_GetItem(tuple, 0) == zero);
    assert(PyTuple_GetItem(tuple, 1) == one);
    Py_DECREF(tuple);

    Py_RETURN_NONE;
}


static PyObject *
test_tuple_pack(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Test PyTuple_Pack()
    PyObject *zero = Py_GetConstantBorrowed(Py_CONSTANT_ZERO);
    PyObject *one = Py_GetConstantBorrowed(Py_CONSTANT_ONE);

    PyObject *tuple = PyTuple_Pack(2, zero, one);
    if (tuple == NULL) {
        return NULL;
    }
    assert(PyTuple_CheckExact(tuple));
    assert(PyTuple_Size(tuple) == 2);
    assert(PyTuple_GetItem(tuple, 0) == zero);
    assert(PyTuple_GetItem(tuple, 1) == one);
    Py_DECREF(tuple);

    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"tuple_check", tuple_check, METH_O},
    {"tuple_check_exact", tuple_check_exact, METH_O},
    {"tuple_size", tuple_size, METH_O},
    {"tuple_getitem", tuple_getitem, METH_VARARGS},
    {"tuple_getslice", tuple_getslice, METH_VARARGS},
    {"test_tuple_new", test_tuple_new, METH_NOARGS},
    {"test_tuple_pack", test_tuple_pack, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Tuple(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
