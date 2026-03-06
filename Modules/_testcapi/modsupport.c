#include "parts.h"



static PyObject *
pyabiinfo_check(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *modname;
    unsigned long maj, min, flags, buildver, abiver;

    if (!PyArg_ParseTuple(args, "zkkkkk",
        &modname, &maj, &min, &flags, &buildver, &abiver))
    {
        return NULL;
    }
    PyABIInfo info = {
        .abiinfo_major_version = (uint8_t)maj,
        .abiinfo_minor_version = (uint8_t)min,
        .flags = (uint16_t)flags,
        .build_version = (uint32_t)buildver,
        .abi_version = (uint32_t)abiver};
    if (PyABIInfo_Check(&info, modname) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
pyarg_parsearray(PyObject* self, PyObject* const* args, Py_ssize_t nargs)
{
    int a, b, c = 0;
    if (!PyArg_ParseArray(args, nargs, "ii|i", &a, &b, &c)) {
        return NULL;
    }
    return Py_BuildValue("iii", a, b, c);
}

static PyObject *
pyarg_parsearrayandkeywords(PyObject* self, PyObject* const* args,
                            Py_ssize_t nargs, PyObject* kwnames)
{
    int a, b, c = 0;
    const char *kwlist[] = {"a", "b", "c", NULL};
    if (!PyArg_ParseArrayAndKeywords(args, nargs, kwnames,
                                     "ii|i", kwlist,
                                     &a, &b, &c)) {
        return NULL;
    }
    return Py_BuildValue("iii", a, b, c);
}

static PyMethodDef TestMethods[] = {
    {"pyabiinfo_check", pyabiinfo_check, METH_VARARGS},
    {"pyarg_parsearray", _PyCFunction_CAST(pyarg_parsearray), METH_FASTCALL},
    {"pyarg_parsearrayandkeywords",
     _PyCFunction_CAST(pyarg_parsearrayandkeywords),
     METH_FASTCALL | METH_KEYWORDS},
    {NULL},
};

int
_PyTestCapi_Init_Modsupport(PyObject *m)
{
    if (PyModule_AddIntMacro(m, PyABIInfo_STABLE) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, PyABIInfo_INTERNAL) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, PyABIInfo_GIL) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, PyABIInfo_FREETHREADED) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, PyABIInfo_FREETHREADING_AGNOSTIC) < 0) {
        return -1;
    }
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
