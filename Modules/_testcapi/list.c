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
    if (!PyArg_ParseTuple(args, "On", &obj, &i)){
        return NULL;
    }
    NULLABLE(obj);
    return PyList_GetItem(obj, i);
}

static PyObject *
list_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)){
        return NULL;
    }
    NULLABLE(obj);
    return PyList_GET_ITEM(obj, i);
}


static PyMethodDef test_methods[] = {
    {"list_check", list_check, METH_O},
    {"list_check_exact", list_check_exact, METH_O},
    {"list_new", list_new, METH_O},
    {"list_size", list_size, METH_O},
    {"list_getitem", list_getitem, METH_VARARGS},
    {"list_get_item", list_get_item, METH_VARARGS},
    // {"list_setitem", list_setitem, METH_VARARGS},
    // {"list_insert"},
    // {"list_append"},
    // {"list_get_slice"},
    // {"list_set_slice"},
    // {"list_sort"},
    // {"list_reverse"},
    // {"list_as_tuple"},

    
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}