// gh-116869: Basic C test extension to check that the Python C API
// does not emit C compiler warnings.

// Always enable assertions
#undef NDEBUG

#include "Python.h"

#if defined (__STDC_VERSION__) && __STDC_VERSION__ > 201710L
#  define NAME _test_c2x_ext
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define NAME _test_c11_ext
#else
#  define NAME _test_c99_ext
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
    long i, j;
    if (!PyArg_ParseTuple(args, "ll:foo", &i, &j)) {
        return NULL;
    }
    long res = i + j;
    return PyLong_FromLong(res);
}


static PyMethodDef _testcext_methods[] = {
    {"add", _testcext_add, METH_VARARGS, _testcext_add_doc},
    {NULL, NULL, 0, NULL}  // sentinel
};


static int
_testcext_exec(PyObject *module)
{
    if (PyModule_AddIntMacro(module, __STDC_VERSION__) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot _testcext_slots[] = {
    {Py_mod_exec, _testcext_exec},
    {0, NULL}
};


PyDoc_STRVAR(_testcext_doc, "C test extension.");

static struct PyModuleDef _testcext_module = {
    PyModuleDef_HEAD_INIT,  // m_base
    STR(NAME),  // m_name
    _testcext_doc,  // m_doc
    0,  // m_size
    _testcext_methods,  // m_methods
    _testcext_slots,  // m_slots
    NULL,  // m_traverse
    NULL,  // m_clear
    NULL,  // m_free
};


#define _FUNC_NAME(NAME) PyInit_ ## NAME
#define FUNC_NAME(NAME) _FUNC_NAME(NAME)

PyMODINIT_FUNC
FUNC_NAME(NAME)(void)
{
    return PyModuleDef_Init(&_testcext_module);
}
