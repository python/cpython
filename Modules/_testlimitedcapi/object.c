// Need limited C API version 3.13 for Py_GetConstant()
#include "pyconfig.h"   // Py_GIL_DISABLED
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030d0000
#endif

#include "parts.h"
#include "util.h"


/* Test Py_GetConstant() */
static PyObject *
get_constant(PyObject *Py_UNUSED(module), PyObject *args)
{
    int constant_id;
    if (!PyArg_ParseTuple(args, "i", &constant_id)) {
        return NULL;
    }

    PyObject *obj = Py_GetConstant(constant_id);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return obj;
}


/* Test Py_GetConstantBorrowed() */
static PyObject *
get_constant_borrowed(PyObject *Py_UNUSED(module), PyObject *args)
{
    int constant_id;
    if (!PyArg_ParseTuple(args, "i", &constant_id)) {
        return NULL;
    }

    PyObject *obj = Py_GetConstantBorrowed(constant_id);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return Py_NewRef(obj);
}


/* Test constants */
static PyObject *
test_constants(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Test that implementation of constants in the limited C API:
    // check that the C code compiles.
    //
    // Test also that constants and Py_GetConstant() return the same
    // objects.
    assert(Py_None == Py_GetConstant(Py_CONSTANT_NONE));
    assert(Py_False == Py_GetConstant(Py_CONSTANT_FALSE));
    assert(Py_True == Py_GetConstant(Py_CONSTANT_TRUE));
    assert(Py_Ellipsis == Py_GetConstant(Py_CONSTANT_ELLIPSIS));
    assert(Py_NotImplemented == Py_GetConstant(Py_CONSTANT_NOT_IMPLEMENTED));
    // Other constants are tested in test_capi.test_object
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"get_constant", get_constant, METH_VARARGS},
    {"get_constant_borrowed", get_constant_borrowed, METH_VARARGS},
    {"test_constants", test_constants, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Object(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
