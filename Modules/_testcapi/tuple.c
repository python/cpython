#include "parts.h"
#include "util.h"


static PyObject *
tuple_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyTuple_GET_SIZE(obj));
}


static PyObject *
tuple_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyTuple_GET_ITEM(obj, i));
}


static PyObject *
test_tuple_set_item(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Test PyTuple_New() and PyTuple_SET_ITEM()
    PyObject *tuple = PyTuple_New(2);
    if (tuple == NULL) {
        return NULL;
    }
    assert(PyTuple_CheckExact(tuple));

    PyObject *zero = Py_GetConstantBorrowed(Py_CONSTANT_ZERO);
    PyObject *one = Py_GetConstantBorrowed(Py_CONSTANT_ONE);

    PyTuple_SET_ITEM(tuple, 0, zero);
    PyTuple_SET_ITEM(tuple, 1, one);

    assert(PyTuple_Size(tuple) == 2);
    assert(PyTuple_GetItem(tuple, 0) == zero);
    assert(PyTuple_GetItem(tuple, 1) == one);
    Py_DECREF(tuple);

    Py_RETURN_NONE;
}

static PyObject *
test_tuple_resize(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Test _PyTuple_Resize()
    PyObject *zero = Py_GetConstantBorrowed(Py_CONSTANT_ZERO);
    PyObject *one = Py_GetConstantBorrowed(Py_CONSTANT_ONE);
    PyObject *tuple = PyTuple_Pack(2, zero, one);
    if (tuple == NULL) {
        return NULL;
    }

    if (_PyTuple_Resize(&tuple, 1) < 0) {
        Py_XDECREF(tuple);
        return NULL;
    }

    assert(PyTuple_CheckExact(tuple));
    assert(PyTuple_Size(tuple) == 1);
    assert(PyTuple_GetItem(tuple, 0) == zero);
    Py_DECREF(tuple);

    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"tuple_get_size", tuple_get_size, METH_O},
    {"tuple_get_item", tuple_get_item, METH_VARARGS},
    {"test_tuple_set_item", test_tuple_set_item, METH_NOARGS},
    {"test_tuple_set_item", test_tuple_set_item, METH_NOARGS},
    {"test_tuple_resize", test_tuple_resize, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Tuple(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
