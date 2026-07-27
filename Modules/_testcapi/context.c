#include "parts.h"
#include "util.h"


typedef PyObject *(*contextvar_ctor)(const char *, PyObject *);

static PyObject *
call_contextvar_ctor(contextvar_ctor ctor, PyObject *args)
{
    const char *name;
    PyObject *def = NULL;
    // Omitting the default passes NULL, which is not the same as Py_None.
    if (!PyArg_ParseTuple(args, "s|O", &name, &def)) {
        return NULL;
    }
    return ctor(name, def);
}

static PyObject *
contextvar_new(PyObject *Py_UNUSED(module), PyObject *args)
{
    return call_contextvar_ctor(PyContextVar_New, args);
}

static PyObject *
contextvar_new_with_flags(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    int flags;
    PyObject *def = NULL;
    // Put flags before the optional default so omitting the default passes
    // NULL, which is not the same as Py_None.
    if (!PyArg_ParseTuple(args, "si|O", &name, &flags, &def)) {
        return NULL;
    }
    return PyContextVar_NewWithFlags(name, def, flags);
}


static PyMethodDef test_methods[] = {
    {"contextvar_new", contextvar_new, METH_VARARGS},
    {"contextvar_new_with_flags", contextvar_new_with_flags, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Context(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, Py_CONTEXTVAR_INHERIT_THREAD_DEFAULT) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, Py_CONTEXTVAR_INHERIT_THREAD_NEVER) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, Py_CONTEXTVAR_INHERIT_THREAD_ALWAYS) < 0) {
        return -1;
    }
    return 0;
}
