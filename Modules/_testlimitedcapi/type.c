// Thin wrappers to PyType functions.
// Do no check PyType_Check() so Python tests can pass arbitrary objects,
// even if it's likely to crash.

// Need limited C API version 3.14 for PyType_Freeze()
#include "pyconfig.h"   // Py_GIL_DISABLED
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030e0000
#endif

#include "parts.h"
#include "util.h"


static PyType_Slot HeapTypeNameType_slots[] = {
    {0},
};

static PyType_Spec HeapTypeNameType_Spec = {
    .name = "_testcapi.HeapTypeNameType",
    .basicsize = sizeof(PyObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = HeapTypeNameType_slots,
};


// Test PyType_FromSpec() with a minimum PyType_Spec
static PyObject*
get_heaptype_for_name(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyType_FromSpec(&HeapTypeNameType_Spec);
}


// Test PyType_GetName()
static PyObject*
get_type_name(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyType_GetName(type);
}


// Test PyType_GetQualName()
static PyObject*
get_type_qualname(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyType_GetQualName(type);
}


// Test PyType_GetFullyQualifiedName()
static PyObject*
get_type_fullyqualname(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyType_GetFullyQualifiedName(type);
}


// Test PyType_GetModuleName()
static PyObject*
get_type_module_name(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    return PyType_GetModuleName(type);
}


// Test PyType_Modified()
static PyObject*
type_modified(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    PyType_Modified(type);
    Py_RETURN_NONE;
}


// Test PyType_Ready()
static PyObject*
type_ready(PyObject *self, PyObject *arg)
{
    assert(!PyErr_Occurred());
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    if (PyType_Ready(type) < 0) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    Py_RETURN_NONE;
}


// Test PyType_Freeze()
static PyObject *
type_freeze(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    if (PyType_Freeze(type) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyType_ClearCache()
static PyObject *
type_clearcache(PyObject *module, PyObject *Py_UNUSED(arg))
{
    // Since Python 3.16, PyType_ClearCache() is a no-op as the type cache is
    // now implemented per-type. It still returns the current version tag.
    unsigned int version_tag = PyType_ClearCache();
    assert(!PyErr_Occurred());
    return PyLong_FromUnsignedLong(version_tag);
}


// Test PyType_GetFlags()
static PyObject *
type_getflags(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    unsigned long flags = PyType_GetFlags(type);
    assert(!PyErr_Occurred());
    return PyLong_FromUnsignedLong(flags);
}


// Test PyType_IsSubtype()
static PyObject *
type_issubtype(PyObject *module, PyObject *args)
{
    PyTypeObject *type1, *type2;
    if (!PyArg_ParseTuple(args, "O!O!",
                          &PyType_Type, &type1,
                          &PyType_Type, &type2)) {
        return NULL;
    }

    int is_subtype = PyType_IsSubtype(type1, type2);
    return PyLong_FromLong(is_subtype);
}


static PyMethodDef test_methods[] = {
    {"get_heaptype_for_name", get_heaptype_for_name, METH_NOARGS},
    {"get_type_name", get_type_name, METH_O},
    {"get_type_qualname",  get_type_qualname, METH_O},
    {"get_type_fullyqualname", get_type_fullyqualname, METH_O},
    {"get_type_module_name", get_type_module_name, METH_O},
    {"type_ready", type_ready, METH_O},
    {"type_modified", type_modified, METH_O},
    {"type_freeze", type_freeze, METH_O},
    {"type_clearcache", type_clearcache, METH_NOARGS},
    {"type_getflags", type_getflags, METH_O},
    {"type_issubtype", type_issubtype, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Type(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

#define ADD_INT(macro) \
    do { \
        if (PyModule_AddIntConstant(m, #macro, macro) < 0) { \
            return -1; \
        } \
    } while (0)

    ADD_INT(Py_TPFLAGS_DEFAULT);

    ADD_INT(Py_TPFLAGS_HAVE_FINALIZE);
    ADD_INT(Py_TPFLAGS_HAVE_GC);
    ADD_INT(Py_TPFLAGS_HAVE_VERSION_TAG);
    ADD_INT(Py_TPFLAGS_HAVE_VECTORCALL);

    ADD_INT(Py_TPFLAGS_DISALLOW_INSTANTIATION );
    ADD_INT(Py_TPFLAGS_IMMUTABLETYPE);
    ADD_INT(Py_TPFLAGS_HEAPTYPE);
    ADD_INT(Py_TPFLAGS_BASETYPE);
    ADD_INT(Py_TPFLAGS_READY);
    ADD_INT(Py_TPFLAGS_READYING);
    ADD_INT(Py_TPFLAGS_METHOD_DESCRIPTOR);
    ADD_INT(Py_TPFLAGS_VALID_VERSION_TAG);
    ADD_INT(Py_TPFLAGS_IS_ABSTRACT);
    ADD_INT(_Py_TPFLAGS_MATCH_SELF);
    ADD_INT(Py_TPFLAGS_ITEMS_AT_END);

    ADD_INT(Py_TPFLAGS_LONG_SUBCLASS);
    ADD_INT(Py_TPFLAGS_LIST_SUBCLASS);
    ADD_INT(Py_TPFLAGS_TUPLE_SUBCLASS);
    ADD_INT(Py_TPFLAGS_BYTES_SUBCLASS);
    ADD_INT(Py_TPFLAGS_UNICODE_SUBCLASS);
    ADD_INT(Py_TPFLAGS_DICT_SUBCLASS);
    ADD_INT(Py_TPFLAGS_BASE_EXC_SUBCLASS);
    ADD_INT(Py_TPFLAGS_TYPE_SUBCLASS);

#undef ADD_INT
    return 0;
}
