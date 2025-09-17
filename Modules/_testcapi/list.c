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


static PyObject*
test_list_api(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* list;
    int i;

    /* SF bug 132008:  PyList_Reverse segfaults */
#define NLIST 30
    list = PyList_New(NLIST);
    if (list == (PyObject*)NULL)
        return (PyObject*)NULL;
    /* list = range(NLIST) */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyLong_FromLong(i);
        if (anint == (PyObject*)NULL) {
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
        PyList_SET_ITEM(list, i, anint);
    }
    /* list.reverse(), via PyList_Reverse() */
    i = PyList_Reverse(list);   /* should not blow up! */
    if (i != 0) {
        Py_DECREF(list);
        return (PyObject*)NULL;
    }
    /* Check that list == range(29, -1, -1) now */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyList_GET_ITEM(list, i);
        if (PyLong_AS_LONG(anint) != NLIST-1-i) {
            PyErr_SetString(PyExc_AssertionError,
                            "test_list_api: reverse screwed up");
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
    }
    Py_DECREF(list);
#undef NLIST

    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"list_get_size", list_get_size, METH_O},
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_set_item", list_set_item, METH_VARARGS},
    {"list_clear", list_clear, METH_O},
    {"list_extend", list_extend, METH_VARARGS},
    {"test_list_api", test_list_api, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
