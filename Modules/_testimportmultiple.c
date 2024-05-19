/*
 * C extensions module to test importing multiple modules from one compiled
 * file (issue16421). This file defines 3 modules (_testimportmodule,
 * foo, bar), only the first one is called the same as the compiled file.
 */

#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include <Python.h>

static PyModuleDef_Slot shared_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef _testimportmultiple = {
    PyModuleDef_HEAD_INIT,
    "_testimportmultiple",
    "_testimportmultiple doc",
    0,
    NULL,
    shared_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__testimportmultiple(void)
{
    return PyModuleDef_Init(&_testimportmultiple);
}

static struct PyModuleDef _foomodule = {
    PyModuleDef_HEAD_INIT,
    "_testimportmultiple_foo",
    "_testimportmultiple_foo doc",
    0,
    NULL,
    shared_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__testimportmultiple_foo(void)
{
    return PyModuleDef_Init(&_foomodule);
}

static struct PyModuleDef _barmodule = {
    PyModuleDef_HEAD_INIT,
    "_testimportmultiple_bar",
    "_testimportmultiple_bar doc",
    0,
    NULL,
    shared_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__testimportmultiple_bar(void){
    return PyModuleDef_Init(&_barmodule);
}
