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

#ifdef TEST_INTERNAL_C_API
   // gh-135906: Check for compiler warnings in the internal C API.
   // - Cython uses pycore_frame.h.
   // - greenlet uses pycore_frame.h, pycore_interpframe_structs.h and
   //   pycore_interpframe.h.
#  include "internal/pycore_frame.h"
#  include "internal/pycore_gc.h"
#  include "internal/pycore_interp.h"
#  include "internal/pycore_interpframe.h"
#  include "internal/pycore_interpframe_structs.h"
#  include "internal/pycore_object.h"
#  include "internal/pycore_pystate.h"
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


static PyMethodDef _testcext_methods[] = {
    {"add", _testcext_add, METH_VARARGS, _testcext_add_doc},
    {NULL, NULL, 0, NULL}  // sentinel
};


static int
_testcext_exec(
#ifdef __STDC_VERSION__
    PyObject *module
#else
    PyObject *Py_UNUSED(module)
#endif
    )
{
#ifdef __STDC_VERSION__
    if (PyModule_AddIntMacro(module, __STDC_VERSION__) < 0) {
        return -1;
    }
#endif

    // test Py_BUILD_ASSERT() and Py_BUILD_ASSERT_EXPR()
    Py_BUILD_ASSERT(sizeof(int) == sizeof(unsigned int));
    assert(Py_BUILD_ASSERT_EXPR(sizeof(int) == sizeof(unsigned int)) == 0);

    return 0;
}

#define _FUNC_NAME(NAME) PyInit_ ## NAME
#define FUNC_NAME(NAME) _FUNC_NAME(NAME)

// Converting from function pointer to void* has undefined behavior, but
// works on all known platforms, and CPython's module and type slots currently
// need it.
// (GCC doesn't have a narrower category for this than -Wpedantic.)
_Py_COMP_DIAG_PUSH
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wpedantic"
#endif

static PyModuleDef_Slot _testcext_slots[] = {
    {Py_mod_exec, (void*)_testcext_exec},
    {0, NULL}
};

_Py_COMP_DIAG_POP

PyDoc_STRVAR(_testcext_doc, "C test extension.");

#ifndef _Py_OPAQUE_PYOBJECT

static struct PyModuleDef _testcext_module = {
    PyModuleDef_HEAD_INIT,  // m_base
    STR(MODULE_NAME),  // m_name
    _testcext_doc,  // m_doc
    0,  // m_size
    _testcext_methods,  // m_methods
    _testcext_slots,  // m_slots
    NULL,  // m_traverse
    NULL,  // m_clear
    NULL,  // m_free
};


PyMODINIT_FUNC
FUNC_NAME(MODULE_NAME)(void)
{
    return PyModuleDef_Init(&_testcext_module);
}

#else  // _Py_OPAQUE_PYOBJECT

// Opaque PyObject means that PyModuleDef is also opaque and cannot be
// declared statically. See PEP 793.
// So, this part of module creation is split into a separate source file
// which uses non-limited API.

// (repeated definition to avoid creating a header)
extern PyObject *testcext_create_moduledef(
    const char *name, const char *doc,
    PyMethodDef *methods, PyModuleDef_Slot *slots);


PyMODINIT_FUNC
FUNC_NAME(MODULE_NAME)(void)
{
    return testcext_create_moduledef(
        STR(MODULE_NAME), _testcext_doc, _testcext_methods, _testcext_slots);
}

#endif  // _Py_OPAQUE_PYOBJECT
