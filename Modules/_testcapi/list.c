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
list_new(PyObject *self, PyObject *obj)
{
    return PyList_New(obj);
}


static PyMethodDef test_methods[] = {
    {"list_check", list_check, METH_O},
    {"list_check_exact", list_check_exact, METH_O},
    {"list_new", list_new, METH_NOARGS},
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}