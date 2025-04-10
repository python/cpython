/* typing accelerator C extension: _typing module. */

#ifndef Py_BUILD_CORE
#define Py_BUILD_CORE
#endif

#include "Python.h"
#include "internal/pycore_interp.h"
#include "internal/pycore_typevarobject.h"
#include "internal/pycore_unionobject.h"  // _PyUnion_Type
#include "pycore_pystate.h"       // _PyInterpreterState_GET()

PyDoc_STRVAR(typing_doc,
"Primitives and accelerators for the typing module.\n");

static int
_typing_exec(PyObject *m)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

#define EXPORT_TYPE(name, typename) \
    if (PyModule_AddObjectRef(m, name, \
                              (PyObject *)interp->cached_objects.typename) < 0) { \
        return -1; \
    }

    EXPORT_TYPE("TypeVar", typevar_type);
    EXPORT_TYPE("TypeVarTuple", typevartuple_type);
    EXPORT_TYPE("ParamSpec", paramspec_type);
    EXPORT_TYPE("ParamSpecArgs", paramspecargs_type);
    EXPORT_TYPE("ParamSpecKwargs", paramspeckwargs_type);
    EXPORT_TYPE("Generic", generic_type);
#undef EXPORT_TYPE
    if (PyModule_AddObjectRef(m, "TypeAliasType", (PyObject *)&_PyTypeAlias_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Union", (PyObject *)&_PyUnion_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "NoDefault", (PyObject *)&_Py_NoDefaultStruct) < 0) {
        return -1;
    }
    return 0;
}

static struct PyModuleDef_Slot _typingmodule_slots[] = {
    {Py_mod_exec, _typing_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef typingmodule = {
        PyModuleDef_HEAD_INIT,
        "_typing",
        typing_doc,
        0,
        NULL,
        _typingmodule_slots,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__typing(void)
{
    return PyModuleDef_Init(&typingmodule);
}
