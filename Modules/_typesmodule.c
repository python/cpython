/* _types module */

#include "Python.h"
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_lazyimportobject.h" // PyLazyImport_Type
#include "pycore_namespace.h"     // _PyNamespace_Type
#include "pycore_object.h"        // _PyNone_Type, _PyNotImplemented_Type
#include "pycore_unionobject.h"   // _PyUnion_Type
#include "pycore_typeobject.h"    // _PyObject_LookupSpecial
#include "pycore_modsupport.h"    // _PyArg_CheckPositional

PyDoc_STRVAR(lookup_special_doc,
"lookup_special(object, name[, default], /)\n\
\n\
Lookup method name `name` on `object` skipping the instance dictionary.\n\
`name` must be a string. If the named special attribute does not\n\
exist,`default` is returned if provided, otherwise `AttributeError` is raised.");

/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
_types_lookup_special_impl(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *v, *name, *result;

    if (!_PyArg_CheckPositional("lookup_special", nargs, 2, 3))
        return NULL;

    v = args[0];
    name = args[1];
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return NULL;
    }
    result = _PyObject_LookupSpecial(v, name);
    if (result == NULL) {
        if (nargs > 2) {
            PyObject *dflt = args[2];
            return Py_NewRef(dflt);
        } else {
            PyErr_Format(PyExc_AttributeError,
                         "'%.50s' object has no special attribute '%U'",
                         Py_TYPE(v)->tp_name, name);
        }
    }
    return result;
}

static int
_types_exec(PyObject *m)
{
#define EXPORT_STATIC_TYPE(NAME, TYPE)                                   \
    do {                                                                 \
        assert(PyUnstable_IsImmortal((PyObject *)&(TYPE)));              \
        if (PyModule_AddObjectRef(m, (NAME), (PyObject *)&(TYPE)) < 0) { \
            return -1;                                                   \
        }                                                                \
    } while (0)

    EXPORT_STATIC_TYPE("AsyncGeneratorType", PyAsyncGen_Type);
    EXPORT_STATIC_TYPE("BuiltinFunctionType", PyCFunction_Type);
    // BuiltinMethodType is the same as BuiltinFunctionType
    EXPORT_STATIC_TYPE("BuiltinMethodType", PyCFunction_Type);
    EXPORT_STATIC_TYPE("CapsuleType", PyCapsule_Type);
    EXPORT_STATIC_TYPE("CellType", PyCell_Type);
    EXPORT_STATIC_TYPE("ClassMethodDescriptorType", PyClassMethodDescr_Type);
    EXPORT_STATIC_TYPE("CodeType", PyCode_Type);
    EXPORT_STATIC_TYPE("CoroutineType", PyCoro_Type);
    EXPORT_STATIC_TYPE("EllipsisType", PyEllipsis_Type);
    EXPORT_STATIC_TYPE("FrameType", PyFrame_Type);
    EXPORT_STATIC_TYPE("FrameLocalsProxyType", PyFrameLocalsProxy_Type);
    EXPORT_STATIC_TYPE("FunctionType", PyFunction_Type);
    EXPORT_STATIC_TYPE("GeneratorType", PyGen_Type);
    EXPORT_STATIC_TYPE("GenericAlias", Py_GenericAliasType);
    EXPORT_STATIC_TYPE("GetSetDescriptorType", PyGetSetDescr_Type);
    // LambdaType is the same as FunctionType
    EXPORT_STATIC_TYPE("LambdaType", PyFunction_Type);
    EXPORT_STATIC_TYPE("LazyImportType", PyLazyImport_Type);
    EXPORT_STATIC_TYPE("MappingProxyType", PyDictProxy_Type);
    EXPORT_STATIC_TYPE("MemberDescriptorType", PyMemberDescr_Type);
    EXPORT_STATIC_TYPE("MethodDescriptorType", PyMethodDescr_Type);
    EXPORT_STATIC_TYPE("MethodType", PyMethod_Type);
    EXPORT_STATIC_TYPE("MethodWrapperType", _PyMethodWrapper_Type);
    EXPORT_STATIC_TYPE("ModuleType", PyModule_Type);
    EXPORT_STATIC_TYPE("NoneType", _PyNone_Type);
    EXPORT_STATIC_TYPE("NotImplementedType", _PyNotImplemented_Type);
    EXPORT_STATIC_TYPE("SimpleNamespace", _PyNamespace_Type);
    EXPORT_STATIC_TYPE("TracebackType", PyTraceBack_Type);
    EXPORT_STATIC_TYPE("UnionType", _PyUnion_Type);
    EXPORT_STATIC_TYPE("WrapperDescriptorType", PyWrapperDescr_Type);
#undef EXPORT_STATIC_TYPE
    return 0;
}

static struct PyModuleDef_Slot _typesmodule_slots[] = {
    {Py_mod_exec, _types_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static PyMethodDef _typesmodule_methods[] = {
    {"lookup_special", _PyCFunction_CAST(_types_lookup_special_impl),
     METH_FASTCALL, lookup_special_doc},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef typesmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_types",
    .m_doc = "Define names for built-in types.",
    .m_size = 0,
    .m_slots = _typesmodule_slots,
    .m_methods = _typesmodule_methods,
};

PyMODINIT_FUNC
PyInit__types(void)
{
    return PyModuleDef_Init(&typesmodule);
}
