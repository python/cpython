#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
   // Need limited C API 3.5 for PyModule_AddFunctions()
#  define Py_LIMITED_API 0x03050000
#endif

#include "parts.h"
#include "util.h"


static PyObject *
pyweakref_check(PyObject *module, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyWeakref_Check(obj));
}

static PyObject *
pyweakref_checkref(PyObject *module, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyWeakref_CheckRef(obj));
}

static PyObject *
pyweakref_checkrefexact(PyObject *module, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyWeakref_CheckRefExact(obj));
}

static PyObject *
pyweakref_checkproxy(PyObject *module, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyWeakref_CheckProxy(obj));
}

static PyObject *
pyweakref_newref(PyObject *module, PyObject *args)
{
    PyObject *obj;
    PyObject *callback = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &obj, &callback)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyWeakref_NewRef(obj, callback);
}

static PyObject *
pyweakref_newproxy(PyObject *module, PyObject *args)
{
    PyObject *obj;
    PyObject *callback = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &obj, &callback)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyWeakref_NewProxy(obj, callback);
}


static PyMethodDef test_methods[] = {
    {"pyweakref_check", pyweakref_check, METH_O},
    {"pyweakref_checkref", pyweakref_checkref, METH_O},
    {"pyweakref_checkrefexact", pyweakref_checkrefexact, METH_O},
    {"pyweakref_checkproxy", pyweakref_checkproxy, METH_O},
    {"pyweakref_newref", pyweakref_newref, METH_VARARGS},
    {"pyweakref_newproxy", pyweakref_newproxy, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Weakref(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
