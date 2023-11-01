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

static PyMethodDef test_methods[] = {
    {"tuple_check", tuple_check, METH_O},
    {"tuple_check_exact", tuple_checkexact, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Tuple(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0){
        return -1;
    }

    return 0;
}
