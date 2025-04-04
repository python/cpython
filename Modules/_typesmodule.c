/* _types module */

#include "Python.h"
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_namespace.h"     // _PyNamespace_Type
#include "pycore_object.h"       // _PyNone_Type, _PyNotImplemented_Type
#include "pycore_unionobject.h"  // _PyUnion_Type

static int
_types_exec(PyObject *m)
{
    if (PyModule_AddObjectRef(m, "AsyncGeneratorType", (PyObject *)&PyAsyncGen_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "BuiltinFunctionType", (PyObject *)&PyCFunction_Type) < 0) {
        return -1;
    }
    // Same as BuiltinMethodType
    if (PyModule_AddObjectRef(m, "BuiltinMethodType", (PyObject *)&PyCFunction_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "CapsuleType", (PyObject *)&PyCapsule_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "CellType", (PyObject *)&PyCell_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "ClassMethodDescriptorType", (PyObject *)&PyClassMethodDescr_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "CodeType", (PyObject *)&PyCode_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "CoroutineType", (PyObject *)&PyCoro_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "EllipsisType", (PyObject *)&PyEllipsis_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "FrameType", (PyObject *)&PyFrame_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "FunctionType", (PyObject *)&PyFunction_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "GeneratorType", (PyObject *)&PyGen_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "GenericAlias", (PyObject *)&Py_GenericAliasType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "GetSetDescriptorType", (PyObject *)&PyGetSetDescr_Type) < 0) {
        return -1;
    }
    // Same as FunctionType
    if (PyModule_AddObjectRef(m, "LambdaType", (PyObject *)&PyFunction_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "MappingProxyType", (PyObject *)&PyDictProxy_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "MemberDescriptorType", (PyObject *)&PyMemberDescr_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "MethodDescriptorType", (PyObject *)&PyMethodDescr_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "MethodType", (PyObject *)&PyMethod_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "MethodWrapperType", (PyObject *)&_PyMethodWrapper_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "ModuleType", (PyObject *)&PyModule_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "NoneType", (PyObject *)&_PyNone_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "NotImplementedType", (PyObject *)&_PyNotImplemented_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "SimpleNamespace", (PyObject *)&_PyNamespace_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "TracebackType", (PyObject *)&PyTraceBack_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "UnionType", (PyObject *)&_PyUnion_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "WrapperDescriptorType", (PyObject *)&PyWrapperDescr_Type) < 0) {
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
