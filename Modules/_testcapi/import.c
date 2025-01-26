#include "parts.h"
#include "util.h"

// Test PyImport_GetModuleAttr()
static PyObject *
pyimport_getmoduleattr(PyObject *self, PyObject *args)
{
    PyObject *mod_name, *attr_name;
    if (!PyArg_ParseTuple(args, "OO", &mod_name, &attr_name)) {
        return NULL;
    }
    NULLABLE(mod_name);
    NULLABLE(attr_name);

    return PyImport_GetModuleAttr(mod_name, attr_name);
}


// Test PyImport_GetModuleAttrString()
static PyObject *
pyimport_getmoduleattrstring(PyObject *self, PyObject *args)
{
    const char *mod_name, *attr_name;
    Py_ssize_t len;
    if (!PyArg_ParseTuple(args, "z#z#", &mod_name, &len, &attr_name, &len)) {
        return NULL;
    }

    return PyImport_GetModuleAttrString(mod_name, attr_name);
}


static PyMethodDef test_methods[] = {
    {"PyImport_GetModuleAttr", pyimport_getmoduleattr, METH_VARARGS},
    {"PyImport_GetModuleAttrString", pyimport_getmoduleattrstring, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Import(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}

