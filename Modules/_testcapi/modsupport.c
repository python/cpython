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

static PyMethodDef TestMethods[] = {
    {"pyabiinfo_check", pyabiinfo_check, METH_VARARGS},
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
