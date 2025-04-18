/* interpreter-internal types for string.templatelib */

#include "Python.h"
#include "pycore_interpolation.h" // _PyInterpolation_Type
#include "pycore_template.h"      // _PyTemplate_Type

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

static struct PyModuleDef_Slot _templatelib_slots[] = {
    {Py_mod_exec, _templatelib_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _templatemodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_templatelib",
    .m_doc = "Interpreter types for template string literals (t-strings).",
    .m_size = 0,
    .m_slots = _templatelib_slots,
};

PyMODINIT_FUNC
PyInit__templatelib(void)
{
    return PyModuleDef_Init(&_templatemodule);
}
