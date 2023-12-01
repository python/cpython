#include "parts.h"


static PyObject *
_testcapi_config_get(PyObject *module, PyObject *name_obj)
{
    const char *name;
    if (PyArg_Parse(name_obj, "s", &name) < 0) {
        return NULL;
    }

    return PyConfig_Get(name);
}


static PyObject *
_testcapi_config_getint(PyObject *module, PyObject *name_obj)
{
    const char *name;
    if (PyArg_Parse(name_obj, "s", &name) < 0) {
        return NULL;
    }

    int value;
    if (PyConfig_GetInt(name, &value) < 0) {
        return NULL;
    }
    return PyLong_FromLong(value);
}


static PyMethodDef test_methods[] = {
    {"config_get", _testcapi_config_get, METH_O},
    {"config_getint", _testcapi_config_getint, METH_O},
    {NULL}
};

int _PyTestCapi_Init_Config(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
