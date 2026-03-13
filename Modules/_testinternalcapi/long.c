#include "parts.h"
#include "../_testcapi/util.h"

#define Py_BUILD_CORE
#include "pycore_long.h"


static PyObject *
_pylong_is_small_int(PyObject *Py_UNUSED(module), PyObject *arg)
{
    NULLABLE(arg);
    if (!PyLong_CheckExact(arg)) {
        PyErr_SetString(PyExc_TypeError, "arg must be int");
        return NULL;
    }
    return PyBool_FromLong(((PyLongObject *)arg)->long_value.lv_tag
                           & IMMORTALITY_BIT_MASK);
}

static PyMethodDef test_methods[] = {
    {"_pylong_is_small_int", _pylong_is_small_int, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_Long(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
