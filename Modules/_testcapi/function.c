#include "parts.h"
#include "util.h"


static PyObject *
function_get_code(PyObject *self, PyObject *func)
{
    PyObject *code = PyFunction_GetCode(func);
    if (code != NULL) {
        return Py_NewRef(code);
    } else {
        return NULL;
    }
}


static PyObject *
function_get_globals(PyObject *self, PyObject *func)
{
    PyObject *globals = PyFunction_GetGlobals(func);
    if (globals != NULL) {
        return Py_NewRef(globals);
    } else {
        return NULL;
    }
}


static PyObject *
function_get_module(PyObject *self, PyObject *func)
{
    PyObject *module = PyFunction_GetModule(func);
    if (module != NULL) {
        return Py_NewRef(module);
    } else {
        return NULL;
    }
}


static PyObject *
function_get_defaults(PyObject *self, PyObject *func)
{
    PyObject *defaults = PyFunction_GetDefaults(func);
    if (defaults != NULL) {
        return Py_NewRef(defaults);
    } else if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;  // This can happen when `defaults` are set to `None`
    }
}


static PyObject *
function_set_defaults(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *defaults = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &defaults)) {
        return NULL;
    }
    int result = PyFunction_SetDefaults(func, defaults);
    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}


static PyObject *
function_get_kw_defaults(PyObject *self, PyObject *func)
{
    PyObject *defaults = PyFunction_GetKwDefaults(func);
    if (defaults != NULL) {
        return Py_NewRef(defaults);
    } else if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;  // This can happen when `kwdefaults` are set to `None`
    }
}


static PyObject *
function_set_kw_defaults(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *defaults = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &defaults)) {
        return NULL;
    }
    int result = PyFunction_SetKwDefaults(func, defaults);
    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}


static PyObject *
function_get_closure(PyObject *self, PyObject *func)
{
    PyObject *closure = PyFunction_GetClosure(func);
    if (closure != NULL) {
        return Py_NewRef(closure);
    } else if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;  // This can happen when `closure` is set to `None`
    }
}


static PyObject *
function_set_closure(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *closure = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &closure)) {
        return NULL;
    }
    int result = PyFunction_SetClosure(func, closure);
    if (result == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"function_get_code", function_get_code, METH_O, NULL},
    {"function_get_globals", function_get_globals, METH_O, NULL},
    {"function_get_module", function_get_module, METH_O, NULL},
    {"function_get_defaults", function_get_defaults, METH_O, NULL},
    {"function_set_defaults", function_set_defaults, METH_VARARGS, NULL},
    {"function_get_kw_defaults", function_get_kw_defaults, METH_O, NULL},
    {"function_set_kw_defaults", function_set_kw_defaults, METH_VARARGS, NULL},
    {"function_get_closure", function_get_closure, METH_O, NULL},
    {"function_set_closure", function_set_closure, METH_VARARGS, NULL},
    {NULL},
};

int
_PyTestCapi_Init_Function(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
