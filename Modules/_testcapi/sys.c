#include "parts.h"
#include "util.h"


static PyObject *
pysys_getattr(PyObject *self, PyObject *name)
{
    NULLABLE(name);
    return PySys_GetAttr(name);
}


static PyObject *
pysys_getattrstring(PyObject *self, PyObject *arg)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return NULL;
    }

    return PySys_GetAttrString(name);
}


static PyMethodDef test_methods[] = {
    {"PySys_GetAttr", pysys_getattr, METH_O},
    {"PySys_GetAttrString", pysys_getattrstring, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Sys(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
