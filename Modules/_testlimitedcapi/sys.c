#include "pyconfig.h"   // Py_GIL_DISABLED
// Need limited C API version 3.15 for PySys_GetAttr() etc
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030f0000
#endif
#include "parts.h"
#include "util.h"


static PyObject *
sys_getattr(PyObject *Py_UNUSED(module), PyObject *name)
{
    NULLABLE(name);
    return PySys_GetAttr(name);
}

static PyObject *
sys_getattrstring(PyObject *Py_UNUSED(module), PyObject *arg)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return NULL;
    }
    return PySys_GetAttrString(name);
}

static PyObject *
sys_getoptionalattr(PyObject *Py_UNUSED(module), PyObject *name)
{
    PyObject *value = UNINITIALIZED_PTR;
    NULLABLE(name);

    switch (PySys_GetOptionalAttr(name, &value)) {
        case -1:
            assert(value == NULL);
            assert(PyErr_Occurred());
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_AttributeError);
        case 1:
            return value;
        default:
            Py_FatalError("PySys_GetOptionalAttr() returned invalid code");
    }
}

static PyObject *
sys_getoptionalattrstring(PyObject *Py_UNUSED(module), PyObject *arg)
{
    PyObject *value = UNINITIALIZED_PTR;
    const char *name;
    Py_ssize_t size;
    if (!PyArg_Parse(arg, "z#", &name, &size)) {
        return NULL;
    }

    switch (PySys_GetOptionalAttrString(name, &value)) {
        case -1:
            assert(value == NULL);
            assert(PyErr_Occurred());
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_AttributeError);
        case 1:
            return value;
        default:
            Py_FatalError("PySys_GetOptionalAttrString() returned invalid code");
    }
}

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
    {"sys_getattr", sys_getattr, METH_O},
    {"sys_getattrstring", sys_getattrstring, METH_O},
    {"sys_getoptionalattr", sys_getoptionalattr, METH_O},
    {"sys_getoptionalattrstring", sys_getoptionalattrstring, METH_O},
    {"sys_getobject", sys_getobject, METH_O},
    {"sys_setobject", sys_setobject, METH_VARARGS},
    {"sys_getxoptions", sys_getxoptions, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Sys(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
