// Need limited C API version 3.7 for PyImport_GetModule()
#include "pyconfig.h"   // Py_GIL_DISABLED
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030d0000
#endif

#include "parts.h"
#include "util.h"


/* Test PyImport_GetMagicNumber() */
static PyObject *
pyimport_getmagicnumber(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    long magic = PyImport_GetMagicNumber();
    return PyLong_FromLong(magic);
}


/* Test PyImport_GetMagicTag() */
static PyObject *
pyimport_getmagictag(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    const char *tag = PyImport_GetMagicTag();
    return PyUnicode_FromString(tag);
}


/* Test PyImport_GetModuleDict() */
static PyObject *
pyimport_getmoduledict(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    return Py_XNewRef(PyImport_GetModuleDict());
}


/* Test PyImport_GetModule() */
static PyObject *
pyimport_getmodule(PyObject *Py_UNUSED(module), PyObject *name)
{
    assert(!PyErr_Occurred());
    PyObject *module = PyImport_GetModule(name);
    if (module == NULL && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return module;
}


/* Test PyImport_AddModuleObject() */
static PyObject *
pyimport_addmoduleobject(PyObject *Py_UNUSED(module), PyObject *name)
{
    return Py_XNewRef(PyImport_AddModuleObject(name));
}


/* Test PyImport_AddModule() */
static PyObject *
pyimport_addmodule(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    return Py_XNewRef(PyImport_AddModule(name));
}


/* Test PyImport_AddModuleRef() */
static PyObject *
pyimport_addmoduleref(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    return PyImport_AddModuleRef(name);
}


/* Test PyImport_Import() */
static PyObject *
pyimport_import(PyObject *Py_UNUSED(module), PyObject *name)
{
    return PyImport_Import(name);
}


/* Test PyImport_ImportModule() */
static PyObject *
pyimport_importmodule(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    return PyImport_ImportModule(name);
}


/* Test PyImport_ImportModuleNoBlock() */
static PyObject *
pyimport_importmodulenoblock(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    return PyImport_ImportModuleNoBlock(name);
    _Py_COMP_DIAG_POP
}


static PyMethodDef test_methods[] = {
    {"PyImport_GetMagicNumber", pyimport_getmagicnumber, METH_NOARGS},
    {"PyImport_GetMagicTag", pyimport_getmagictag, METH_NOARGS},
    {"PyImport_GetModuleDict", pyimport_getmoduledict, METH_NOARGS},
    {"PyImport_GetModule", pyimport_getmodule, METH_O},
    {"PyImport_AddModuleObject", pyimport_addmoduleobject, METH_O},
    {"PyImport_AddModule", pyimport_addmodule, METH_VARARGS},
    {"PyImport_AddModuleRef", pyimport_addmoduleref, METH_VARARGS},
    {"PyImport_Import", pyimport_import, METH_O},
    {"PyImport_ImportModule", pyimport_importmodule, METH_VARARGS},
    {"PyImport_ImportModuleNoBlock", pyimport_importmodulenoblock, METH_VARARGS},
    {NULL},
};


int
_PyTestLimitedCAPI_Init_Import(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}
