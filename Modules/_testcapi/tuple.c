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
tuple_copy(PyObject *tuple)
{
    Py_ssize_t size = PyTuple_GET_SIZE(tuple);
    PyObject *newtuple = PyTuple_New(size);
    if (!newtuple) {
        return NULL;
    }
    for (Py_ssize_t n = 0; n < size; n++) {
        PyTuple_SET_ITEM(newtuple, n, Py_XNewRef(PyTuple_GET_ITEM(tuple, n)));
    }
    return newtuple;
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
        newtuple = tuple_copy(obj);
        if (!newtuple) {
            return NULL;
        }

        PyObject *val = PyTuple_GET_ITEM(newtuple, i);
        PyTuple_SET_ITEM(newtuple, i, Py_XNewRef(value));
        Py_DECREF(val);
        return newtuple;
    }
    else {
        NULLABLE(obj);

        PyObject *val = PyTuple_GET_ITEM(obj, i);
        PyTuple_SET_ITEM(obj, i, Py_XNewRef(value));
        Py_DECREF(val);
        return Py_XNewRef(obj);
    }
}

static PyObject *
_tuple_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *tup;
    Py_ssize_t newsize;
    int new = 1;
    if (!PyArg_ParseTuple(args, "On|p", &tup, &newsize, &new)) {
        return NULL;
    }
    if (new) {
        tup = tuple_copy(tup);
        if (!tup) {
            return NULL;
        }
    }
    else {
        NULLABLE(tup);
        Py_XINCREF(tup);
    }
    int r = _PyTuple_Resize(&tup, newsize);
    if (r == -1) {
        assert(tup == NULL);
        return NULL;
    }
    return tup;
}

static PyObject *
_check_tuple_item_is_NULL(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    return PyLong_FromLong(PyTuple_GET_ITEM(obj, i) == NULL);
}


static PyObject *
tuple_fromarray(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *src;
    Py_ssize_t size = UNINITIALIZED_SIZE;
    if (!PyArg_ParseTuple(args, "O|n", &src, &size)) {
        return NULL;
    }
    if (src != Py_None && !PyTuple_Check(src)) {
        PyErr_SetString(PyExc_TypeError, "expect a tuple");
        return NULL;
    }

    PyObject **items;
    if (src != Py_None) {
        items = &PyTuple_GET_ITEM(src, 0);
        if (size == UNINITIALIZED_SIZE) {
            size = PyTuple_GET_SIZE(src);
        }
    }
    else {
        items = NULL;
    }
    return PyTuple_FromArray(items, size);
}


static PyMethodDef test_methods[] = {
    {"tuple_get_size", tuple_get_size, METH_O},
    {"tuple_get_item", tuple_get_item, METH_VARARGS},
    {"tuple_set_item", tuple_set_item, METH_VARARGS},
    {"_tuple_resize", _tuple_resize, METH_VARARGS},
    {"_check_tuple_item_is_NULL", _check_tuple_item_is_NULL, METH_VARARGS},
    {"tuple_fromarray", tuple_fromarray, METH_VARARGS},
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
