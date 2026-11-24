#include "parts.h"
#include "util.h"


static PyObject*
return_string(const char *str)
{
    assert(str != NULL);
    return PyUnicode_FromString(str);
}


/* Test Py_GetVersion() */
static PyObject*
py_getversion(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    return return_string(Py_GetVersion());
}


/* Test Py_GetPlatform() */
static PyObject*
py_getplatform(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    return return_string(Py_GetPlatform());
}


/* Test Py_GetCopyright() */
static PyObject*
py_getcopyright(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    return return_string(Py_GetCopyright());
}


/* Test Py_GetCompiler() */
static PyObject*
py_getcompiler(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    return return_string(Py_GetCompiler());
}


/* Test Py_GetBuildInfo() */
static PyObject*
py_getbuildinfo(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    return return_string(Py_GetBuildInfo());
}


/* Test _Py_GetBuiltWithAssert() */
static PyObject*
_py_getbuiltwithassert(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Function only exported for _testlimitedcapi
    PyAPI_FUNC(int) _Py_GetBuiltWithAssert(void);

    return PyLong_FromLong(_Py_GetBuiltWithAssert());
}


static PyMethodDef test_methods[] = {
    {"py_getversion", py_getversion, METH_NOARGS},
    {"py_getplatform", py_getplatform, METH_NOARGS},
    {"py_getcopyright", py_getcopyright, METH_NOARGS},
    {"py_getcompiler", py_getcompiler, METH_NOARGS},
    {"py_getbuildinfo", py_getbuildinfo, METH_NOARGS},
    {"_py_getbuiltwithassert", _py_getbuiltwithassert, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Build(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
