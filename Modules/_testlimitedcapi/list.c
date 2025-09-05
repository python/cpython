// Need limited C API version 3.13 for PyList_GetItemRef()
#include "pyconfig.h"   // Py_GIL_DISABLED
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030d0000
#endif

#include "parts.h"
#include "util.h"

static PyObject *
list_check(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyList_Check(obj));
}

static PyObject *
list_check_exact(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyList_CheckExact(obj));
}

static PyObject *
list_new(PyObject* Py_UNUSED(module), PyObject *obj)
{
    return PyList_New(PyLong_AsSsize_t(obj));
}

static PyObject *
list_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_Size(obj));
}

static PyObject *
list_getitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyList_GetItem(obj, i));
}

static PyObject *
list_get_item_ref(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyList_GetItemRef(obj, i);
}

static PyObject *
list_setitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_SetItem(obj, i, Py_XNewRef(value)));

}

static PyObject *
list_insert(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t where;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &where, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_Insert(obj, where, Py_XNewRef(value)));

}

static PyObject *
list_append(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    if (!PyArg_ParseTuple(args, "OO", &obj, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_Append(obj, value));
}

static PyObject *
list_getslice(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t ilow, ihigh;
    if (!PyArg_ParseTuple(args, "Onn", &obj, &ilow, &ihigh)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyList_GetSlice(obj, ilow, ihigh);

}

static PyObject *
list_setslice(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t ilow, ihigh;
    if (!PyArg_ParseTuple(args, "OnnO", &obj, &ilow, &ihigh, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    RETURN_INT(PyList_SetSlice(obj, ilow, ihigh, value));
}

static PyObject *
list_sort(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Sort(obj));
}

static PyObject *
list_reverse(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Reverse(obj));
}

static PyObject *
list_astuple(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyList_AsTuple(obj);
}


static PyMethodDef test_methods[] = {
    {"list_check", list_check, METH_O},
    {"list_check_exact", list_check_exact, METH_O},
    {"list_new", list_new, METH_O},
    {"list_size", list_size, METH_O},
    {"list_getitem", list_getitem, METH_VARARGS},
    {"list_get_item_ref", list_get_item_ref, METH_VARARGS},
    {"list_setitem", list_setitem, METH_VARARGS},
    {"list_insert", list_insert, METH_VARARGS},
    {"list_append", list_append, METH_VARARGS},
    {"list_getslice", list_getslice, METH_VARARGS},
    {"list_setslice", list_setslice, METH_VARARGS},
    {"list_sort", list_sort, METH_O},
    {"list_reverse", list_reverse, METH_O},
    {"list_astuple", list_astuple, METH_O},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_List(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
