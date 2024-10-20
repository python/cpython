/* interpreter-internal types for templatelib */

#ifndef Py_BUILD_CORE
#define Py_BUILD_CORE
#endif

#include "Python.h"
#include "pycore_template.h"
#include "pycore_interpolation.h"

static int
_templatelib_exec(PyObject *m)
{
    if (PyModule_AddObjectRef(m, "Template", (PyObject *)&_PyTemplate_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Interpolation", (PyObject *)&_PyInterpolation_Type) < 0) {
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(_templatelib_doc,
"Interpreter-internal types for t-string templates.\n");

static struct PyModuleDef_Slot _templatelib_slots[] = {
    {Py_mod_exec, _templatelib_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _templatemodule = {
    PyModuleDef_HEAD_INIT,
    "_templatelib",
    _templatelib_doc,
    0,
    NULL,
    _templatelib_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__templatelib(void)
{
    return PyModuleDef_Init(&_templatemodule);
}
