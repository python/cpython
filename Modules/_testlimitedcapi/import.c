// Need limited C API version 3.13 for PyImport_AddModuleRef()
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
    NULLABLE(name);
    PyObject *module = PyImport_GetModule(name);
    if (module == NULL && !PyErr_Occurred()) {
        return Py_NewRef(PyExc_KeyError);
    }
    return module;
}


/* Test PyImport_AddModuleObject() */
static PyObject *
pyimport_addmoduleobject(PyObject *Py_UNUSED(module), PyObject *name)
{
    NULLABLE(name);
    return Py_XNewRef(PyImport_AddModuleObject(name));
}


/* Test PyImport_AddModule() */
static PyObject *
pyimport_addmodule(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "z#", &name, &size)) {
        return NULL;
    }

    return Py_XNewRef(PyImport_AddModule(name));
}


/* Test PyImport_AddModuleRef() */
static PyObject *
pyimport_addmoduleref(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "z#", &name, &size)) {
        return NULL;
    }

    return PyImport_AddModuleRef(name);
}


/* Test PyImport_Import() */
static PyObject *
pyimport_import(PyObject *Py_UNUSED(module), PyObject *name)
{
    NULLABLE(name);
    return PyImport_Import(name);
}


/* Test PyImport_ImportModule() */
static PyObject *
pyimport_importmodule(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "z#", &name, &size)) {
        return NULL;
    }

    return PyImport_ImportModule(name);
}


/* Test PyImport_ImportModuleNoBlock() */
static PyObject *
pyimport_importmodulenoblock(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "z#", &name, &size)) {
        return NULL;
    }

    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    return PyImport_ImportModuleNoBlock(name);
    _Py_COMP_DIAG_POP
}


/* Test PyImport_ImportModuleEx() */
static PyObject *
pyimport_importmoduleex(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    PyObject *globals, *locals, *fromlist;
    if (!PyArg_ParseTuple(args, "z#OOO",
                          &name, &size, &globals, &locals, &fromlist)) {
        return NULL;
    }
    NULLABLE(globals);
    NULLABLE(locals);
    NULLABLE(fromlist);

    return PyImport_ImportModuleEx(name, globals, locals, fromlist);
}


/* Test PyImport_ImportModuleLevel() */
static PyObject *
pyimport_importmodulelevel(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    PyObject *globals, *locals, *fromlist;
    int level;
    if (!PyArg_ParseTuple(args, "z#OOOi",
                          &name, &size, &globals, &locals, &fromlist, &level)) {
        return NULL;
    }
    NULLABLE(globals);
    NULLABLE(locals);
    NULLABLE(fromlist);

    return PyImport_ImportModuleLevel(name, globals, locals, fromlist, level);
}


/* Test PyImport_ImportModuleLevelObject() */
static PyObject *
pyimport_importmodulelevelobject(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *name, *globals, *locals, *fromlist;
    int level;
    if (!PyArg_ParseTuple(args, "OOOOi",
                          &name, &globals, &locals, &fromlist, &level)) {
        return NULL;
    }
    NULLABLE(name);
    NULLABLE(globals);
    NULLABLE(locals);
    NULLABLE(fromlist);

    return PyImport_ImportModuleLevelObject(name, globals, locals, fromlist, level);
}


/* Test PyImport_ImportFrozenModule() */
static PyObject *
pyimport_importfrozenmodule(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "z#", &name, &size)) {
        return NULL;
    }

    RETURN_INT(PyImport_ImportFrozenModule(name));
}


/* Test PyImport_ImportFrozenModuleObject() */
static PyObject *
pyimport_importfrozenmoduleobject(PyObject *Py_UNUSED(module), PyObject *name)
{
    NULLABLE(name);
    RETURN_INT(PyImport_ImportFrozenModuleObject(name));
}


/* Test PyImport_ExecCodeModule() */
static PyObject *
pyimport_executecodemodule(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    PyObject *code;
    if (!PyArg_ParseTuple(args, "z#O", &name, &size, &code)) {
        return NULL;
    }
    NULLABLE(code);

    return PyImport_ExecCodeModule(name, code);
}


/* Test PyImport_ExecCodeModuleEx() */
static PyObject *
pyimport_executecodemoduleex(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    PyObject *code;
    const char *pathname;
    if (!PyArg_ParseTuple(args, "z#Oz#", &name, &size, &code, &pathname, &size)) {
        return NULL;
    }
    NULLABLE(code);

    return PyImport_ExecCodeModuleEx(name, code, pathname);
}


/* Test PyImport_ExecCodeModuleWithPathnames() */
static PyObject *
pyimport_executecodemodulewithpathnames(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    Py_ssize_t size;
    PyObject *code;
    const char *pathname;
    const char *cpathname;
    if (!PyArg_ParseTuple(args, "z#Oz#z#", &name, &size, &code, &pathname, &size, &cpathname, &size)) {
        return NULL;
    }
    NULLABLE(code);

    return PyImport_ExecCodeModuleWithPathnames(name, code,
                                                pathname, cpathname);
}


/* Test PyImport_ExecCodeModuleObject() */
static PyObject *
pyimport_executecodemoduleobject(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *name, *code, *pathname, *cpathname;
    if (!PyArg_ParseTuple(args, "OOOO", &name, &code, &pathname, &cpathname)) {
        return NULL;
    }
    NULLABLE(name);
    NULLABLE(code);
    NULLABLE(pathname);
    NULLABLE(cpathname);

    return PyImport_ExecCodeModuleObject(name, code, pathname, cpathname);
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
    {"PyImport_ImportModuleEx", pyimport_importmoduleex, METH_VARARGS},
    {"PyImport_ImportModuleLevel", pyimport_importmodulelevel, METH_VARARGS},
    {"PyImport_ImportModuleLevelObject", pyimport_importmodulelevelobject, METH_VARARGS},
    {"PyImport_ImportFrozenModule", pyimport_importfrozenmodule, METH_VARARGS},
    {"PyImport_ImportFrozenModuleObject", pyimport_importfrozenmoduleobject, METH_O},
    {"PyImport_ExecCodeModule", pyimport_executecodemodule, METH_VARARGS},
    {"PyImport_ExecCodeModuleEx", pyimport_executecodemoduleex, METH_VARARGS},
    {"PyImport_ExecCodeModuleWithPathnames", pyimport_executecodemodulewithpathnames, METH_VARARGS},
    {"PyImport_ExecCodeModuleObject", pyimport_executecodemoduleobject, METH_VARARGS},
    {NULL},
};


int
_PyTestLimitedCAPI_Init_Import(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}
