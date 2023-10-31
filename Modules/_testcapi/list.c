#include "parts.h"
#include "util.h"

static PyObject *
list_check(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyList_Check(obj));
}

static PyObject *
list_checkexact(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyList_CheckExact(obj));
}

static PyObject *
list_new(PyObject *self, PyObject *args)
{
    Py_ssize_t n;
    if (!PyArg_ParseTuple(args, "n", &n)) {
        return NULL;
    }

    PyObject *list = PyList_New(n);
    if (list == NULL) {
        return NULL;
    }

    for (Py_ssize_t i=0; i < n; i++) {
        PyList_SET_ITEM(list, i, Py_NewRef(Py_None));
    }
    return list;
}

static PyObject *
list_size(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_Size(obj));
}

static PyObject *
list_getitem(PyObject *self, PyObject *args)
{
    PyObject *list;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &list, &index)) {
        return NULL;
    }
    NULLABLE(list);

    PyObject *value = PyList_GetItem(list, index);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        return Py_NewRef(PyExc_IndexError);
    }
    return Py_NewRef(value);
}

static PyObject *
list_setitem(PyObject *self, PyObject *args)
{
    PyObject *list, *item;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "OnO", &list, &index, &item)) {
        return NULL;
    }
    NULLABLE(list);
    NULLABLE(item);
    RETURN_INT(PyList_SetItem(list, index, item));
}

static PyObject *
list_insert(PyObject *self, PyObject *args)
{
    PyObject *list, *item;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "OnO", &list, &index, &item)) {
        return NULL;
    }
    NULLABLE(list);
    NULLABLE(item);
    RETURN_INT(PyList_Insert(list, index, item));
}

static PyObject *
list_append(PyObject *self, PyObject *args)
{
    PyObject *list, *item;
    if (!PyArg_ParseTuple(args, "OO", &list, &item)) {
        return NULL;
    }
    NULLABLE(list);
    NULLABLE(item);
    RETURN_INT(PyList_Append(list, item));
}

static PyObject *
list_sort(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Sort(obj));
}

static PyObject *
list_astuple(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PyList_AsTuple(obj);
}

static PyObject *
list_reverse(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Reverse(obj));
}

static PyObject *
list_getslice(PyObject *self, PyObject *args)
{
    PyObject *list;
    Py_ssize_t low, high;
    if (!PyArg_ParseTuple(args, "Onn", &list, &low, &high)) {
        return NULL;
    }
    NULLABLE(list);
    return PyList_GetSlice(list, low, high);
}

static PyObject *
list_setslice(PyObject *self, PyObject *args)
{
    PyObject *list, *value;
    Py_ssize_t low, high;
    if (!PyArg_ParseTuple(args, "OnnO", &list, &low, &high, &value)) {
        return NULL;
    }
    NULLABLE(list);
    NULLABLE(value);
    RETURN_INT(PyList_SetSlice(list, low, high, value));
}


static PyMethodDef test_methods[] = {
    {"list_check", list_check, METH_O},
    {"list_checkexact", list_checkexact, METH_O},
    {"list_new", list_new, METH_VARARGS},
    {"list_size", list_size, METH_O},
    {"list_getitem", list_getitem, METH_VARARGS},
    {"list_setitem", list_setitem, METH_VARARGS},
    {"list_insert", list_insert, METH_VARARGS},
    {"list_append", list_append, METH_VARARGS},
    {"list_sort", list_sort, METH_O},
    {"list_astuple", list_astuple, METH_O},
    {"list_reverse", list_reverse, METH_O},
    {"list_getslice", list_getslice, METH_VARARGS},
    {"list_setslice", list_setslice, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
