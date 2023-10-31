#include "parts.h"
#include "util.h"

static PyObject *
list_check(PyObject* self, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyList_Check(obj));
}

static PyMethodDef test_methods[] = {
    {"list_check", list_check, METH_O}
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}