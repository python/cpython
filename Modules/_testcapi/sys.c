#define PY_SSIZE_T_CLEAN
#include "parts.h"
#include "util.h"


static PyObject *
sys_getobject(PyObject *Py_UNUSED(module), PyObject *arg)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return NULL;
    }
    PyObject *result = PySys_GetObject(name);
    if (result == NULL) {
        result = PyExc_AttributeError;
    }
    return Py_NewRef(result);
}

static PyObject *
sys_setobject(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    PyObject *value;
    if (!PyArg_ParseTuple(args, "z#O", &name, &size, &value)) {
        return NULL;
    }
    NULLABLE(value);
    RETURN_INT(PySys_SetObject(name, value));
}

static PyObject *
sys_getxoptions(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(ignored))
{
    PyObject *result = PySys_GetXOptions();
    return Py_XNewRef(result);
}


static PyMethodDef test_methods[] = {
    {"sys_getobject", sys_getobject, METH_O},
    {"sys_setobject", sys_setobject, METH_VARARGS},
    {"sys_getxoptions", sys_getxoptions, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Sys(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
