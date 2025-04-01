/* _types module */

#include "Python.h"
#include "pycore_namespace.h"     // _PyNamespace_Type

static int
_types_exec(PyObject *m)
{
    if (PyModule_AddObjectRef(m, "CapsuleType", (PyObject *)&PyCapsule_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "SimpleNamespace", (PyObject *)&_PyNamespace_Type) < 0) {
        return -1;
    }
    return 0;
}

static struct PyModuleDef_Slot _typesmodule_slots[] = {
    {Py_mod_exec, _types_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef typesmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_types",
    .m_doc = "Define names for built-in types.",
    .m_size = 0,
    .m_slots = _typesmodule_slots,
};

PyMODINIT_FUNC
PyInit__types(void)
{
    return PyModuleDef_Init(&typesmodule);
}
