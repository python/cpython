/* Reproducer: a user builtin module (via Modules/Setup.local) that needs a
   symbol defined only in Modules/getpath.o. On main this breaks the link of
   Programs/_freeze_module, which links Modules/getpath_noop.o instead. */

#include "Python.h"
#include "pycore_initconfig.h"    // _Py_Get_Getpath_CodeObject()

static PyObject *
getpathuser_code(PyObject *module, PyObject *Py_UNUSED(args))
{
    return _Py_Get_Getpath_CodeObject();
}

static PyMethodDef methods[] = {
    {"getpath_code", getpathuser_code, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "_getpathuser", NULL, 0, methods,
};

PyMODINIT_FUNC
PyInit__getpathuser(void)
{
    return PyModuleDef_Init(&moduledef);
}
