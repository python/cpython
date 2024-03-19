#include "parts.h"
#include "util.h"


static PyObject *
list_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_GET_SIZE(obj));
}


static PyObject *
list_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyList_GET_ITEM(obj, i));
}


static PyObject *
list_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    PyList_SET_ITEM(obj, i, Py_XNewRef(value));
    Py_RETURN_NONE;

}


static PyObject *
list_clear(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Clear(obj));
}


static PyObject *
list_extend(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *arg;
    if (!PyArg_ParseTuple(args, "OO", &obj, &arg)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(arg);
    RETURN_INT(PyList_Extend(obj, arg));
}


static PyMethodDef test_methods[] = {
    {"list_get_size", list_get_size, METH_O},
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_set_item", list_set_item, METH_VARARGS},
    {"list_clear", list_clear, METH_O},
    {"list_extend", list_extend, METH_VARARGS},

    {NULL},
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
