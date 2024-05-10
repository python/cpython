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
tuple_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value, *newtuple;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(value);
    if (PyTuple_CheckExact(obj)) {
        Py_ssize_t size = PyTuple_Size(obj);
        newtuple = PyTuple_New(size);
        if (!newtuple) {
            return NULL;
        }
        for (Py_ssize_t n = 0; n < size; n++) {
            PyTuple_SetItem(newtuple, n, Py_XNewRef(PyTuple_GetItem(obj, n)));
        }
    }
    else {
        NULLABLE(obj);
        newtuple = obj;
    }
    PyTuple_SET_ITEM(newtuple, i, Py_XNewRef(value));
    return newtuple;
}

static PyObject *
tuple_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *tup;
    Py_ssize_t newsize;
    int new = 1;
    if (!PyArg_ParseTuple(args, "nO|p", &newsize, &tup, &new)) {
        return NULL;
    }
    if (new) {
        Py_ssize_t size = PyTuple_Size(tup);
        PyObject *newtup = PyTuple_New(size);
        for (Py_ssize_t n = 0; n < size; n++) {
            PyTuple_SetItem(newtup, n, Py_XNewRef(PyTuple_GetItem(tup, n)));
        }
        tup = newtup;
    }
    else {
        Py_XINCREF(tup);
    }
    int r = _PyTuple_Resize(&tup, newsize);
    if (r == -1) {
        return NULL;
    }
    return tup;
}


static PyMethodDef test_methods[] = {
    {"tuple_get_size", tuple_get_size, METH_O},
    {"tuple_get_item", tuple_get_item, METH_VARARGS},
    {"tuple_set_item", tuple_set_item, METH_VARARGS},
    {"tuple_resize", tuple_resize, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Tuple(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
