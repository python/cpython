#include "parts.h"
#include "util.h"


static PyObject *
tuple_check(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyTuple_Check(obj));
}

static PyObject *
tuple_checkexact(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyTuple_CheckExact(obj));
}

static PyObject *
tuple_new(PyObject* Py_UNUSED(module), PyObject *len)
{
    return PyTuple_New(PyLong_AsSsize_t(len));
}

static PyObject *
tuple_pack(PyObject *Py_UNUSED(module), PyObject *args)
{
    Py_ssize_t size = PyLong_AsSize_t(PyTuple_GetItem(args, 0));
    if (size == 1) {
        PyObject *arg1 = PyTuple_GetItem(args, 1);
        NULLABLE(arg1);
        return PyTuple_Pack(1, arg1);
    }
    else if (size == 2) {
        PyObject *arg1 = PyTuple_GetItem(args, 1);
        PyObject *arg2 = PyTuple_GetItem(args, 2);
        NULLABLE(arg1);
        NULLABLE(arg2);
        return PyTuple_Pack(2, arg1, arg2);
    }
    return PyTuple_Pack(size);
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
tuple_setitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value, *newtuple;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    if (obj) {
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
        newtuple = obj;
    }
    if (PyTuple_SetItem(newtuple, i, Py_XNewRef(value)) == -1) {
        return NULL;
    }
    return newtuple;
}


static PyMethodDef test_methods[] = {
    {"tuple_check", tuple_check, METH_O},
    {"tuple_checkexact", tuple_checkexact, METH_O},
    {"tuple_new", tuple_new, METH_O},
    {"tuple_pack", tuple_pack, METH_VARARGS},
    {"tuple_size", tuple_size, METH_O},
    {"tuple_getitem", tuple_getitem, METH_VARARGS},
    {"tuple_getslice", tuple_getslice, METH_VARARGS},
    {"tuple_setitem", tuple_setitem, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Tuple(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
