#include "parts.h"
#include "pycore_list.h"


static PyObject *
list_capacity(PyObject* Py_UNUSED(module), PyObject *obj)
{
    if (!PyList_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "expect list, got %T", obj);
        return NULL;
    }
    PyListObject *list = (PyListObject*)obj;

    return PyLong_FromSsize_t(_PyList_Capacity(list));
}


static PyMethodDef TestMethods[] = {
    {"list_capacity", list_capacity, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_List(PyObject *m)
{
    return PyModule_AddFunctions(m, TestMethods);
}
