// gh-116869: Basic C test extension to check that the Python C API
// does not emit C compiler warnings.
//
// Test also the internal C API if the TEST_INTERNAL_C_API macro is defined.

// Always enable assertions
#undef NDEBUG

#ifdef TEST_INTERNAL_C_API
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "datetime.h"

#ifdef TEST_INTERNAL_C_API
   // gh-135906: Check for compiler warnings in the internal C API.
   // - Cython uses pycore_critical_section.h, pycore_frame.h and
   //   pycore_template.h.
   // - greenlet uses pycore_frame.h, pycore_interpframe_structs.h and
   //   pycore_interpframe.h.
#  include "internal/pycore_critical_section.h"
#  include "internal/pycore_frame.h"
#  include "internal/pycore_gc.h"
#  include "internal/pycore_interp.h"
#  include "internal/pycore_interpframe.h"
#  include "internal/pycore_interpframe_structs.h"
#  include "internal/pycore_object.h"
#  include "internal/pycore_pystate.h"
#  include "internal/pycore_template.h"
#endif

#ifndef MODULE_NAME
#  error "MODULE_NAME macro must be defined"
#endif

#define _STR(NAME) #NAME
#define STR(NAME) _STR(NAME)

PyDoc_STRVAR(_testcext_add_doc,
"add(x, y)\n"
"\n"
"Return the sum of two integers: x + y.");

static PyObject *
_testcext_add(PyObject *Py_UNUSED(module), PyObject *args)
{
    long i, j, res;
    if (!PyArg_ParseTuple(args, "ll:foo", &i, &j)) {
        return NULL;
    }
    res = i + j;
    return PyLong_FromLong(res);
}


static PyObject *
test_datetime(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // datetime.h is excluded from the limited C API
#ifndef Py_LIMITED_API
    PyDateTime_IMPORT;
    if (PyErr_Occurred()) {
        return NULL;
    }
#endif

    Py_RETURN_NONE;
}


static PyMethodDef _testcext_methods[] = {
    {"add", _testcext_add, METH_VARARGS, _testcext_add_doc},
    {"test_datetime", test_datetime, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}  // sentinel
};


static int
_testcext_exec(PyObject *module)
{
    PyObject *result, *obj;

#ifdef __STDC_VERSION__
    if (PyModule_AddIntMacro(module, __STDC_VERSION__) < 0) {
        return -1;
    }
#endif

    result = PyObject_CallMethod(module, "test_datetime", "");
    if (!result) return -1;
    Py_DECREF(result);

    // test Py_BUILD_ASSERT() and Py_BUILD_ASSERT_EXPR()
    Py_BUILD_ASSERT(sizeof(int) == sizeof(unsigned int));
    assert(Py_BUILD_ASSERT_EXPR(sizeof(int) == sizeof(unsigned int)) == 0);

    // Test Py_CLEAR()
    obj = NULL;
    Py_CLEAR(obj);

    return 0;
}

#define _FUNC_NAME(NAME) PyModExport_ ## NAME
#define FUNC_NAME(NAME) _FUNC_NAME(NAME)

// Converting from function pointer to void* has undefined behavior, but
// works on all known platforms, and CPython's module and type slots currently
// need it.
// (GCC doesn't have a narrower category for this than -Wpedantic.)
_Py_COMP_DIAG_PUSH
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wcast-qual"
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-Wcast-qual"
#endif

PyDoc_STRVAR(_testcext_doc, "C test extension.");

static PyModuleDef_Slot _testcext_slots[] = {
    {Py_mod_name, STR(MODULE_NAME)},
    {Py_mod_doc, (void*)(char*)_testcext_doc},
    {Py_mod_exec, (void*)_testcext_exec},
    {Py_mod_methods, _testcext_methods},
    {0, NULL}
};

_Py_COMP_DIAG_POP

PyMODEXPORT_FUNC
FUNC_NAME(MODULE_NAME)(void)
{
    return _testcext_slots;
}

// Also define the soft-deprecated entrypoint to ensure it isn't called

#define _INITFUNC_NAME(NAME) PyInit_ ## NAME
#define INITFUNC_NAME(NAME) _INITFUNC_NAME(NAME)

PyMODINIT_FUNC
INITFUNC_NAME(MODULE_NAME)(void)
{
    PyErr_SetString(
        PyExc_AssertionError,
        "PyInit_* function called while a PyModExport_* one is available");
    return NULL;
}
