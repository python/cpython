// Thin wrappers to PyType functions.
// Do no check PyType_Check() so Python tests can pass arbitrary objects,
// even if it's likely to crash.

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


// Test for PyType_GetDict()
static PyObject *
test_get_type_dict(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Assert ints have a `to_bytes` method
    PyObject *long_dict = PyType_GetDict(&PyLong_Type);
    assert(long_dict);
    assert(PyDict_GetItemString(long_dict, "to_bytes")); // borrowed ref
    Py_DECREF(long_dict);

    // Make a new type, add an attribute to it and assert it's there
    PyObject *HeapTypeNameType = PyType_FromSpec(&HeapTypeNameType_Spec);
    assert(HeapTypeNameType);
    assert(PyObject_SetAttrString(
        HeapTypeNameType, "new_attr", Py_NewRef(Py_None)) >= 0);
    PyObject *type_dict = PyType_GetDict((PyTypeObject*)HeapTypeNameType);
    assert(type_dict);
    assert(PyDict_GetItemString(type_dict, "new_attr")); // borrowed ref
    Py_DECREF(HeapTypeNameType);
    Py_DECREF(type_dict);
    Py_RETURN_NONE;
}


// Test PyType_GetSlot()
static PyObject *
test_get_statictype_slots(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    newfunc tp_new = PyType_GetSlot(&PyLong_Type, Py_tp_new);
    if (PyLong_Type.tp_new != tp_new) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: tp_new of long");
        return NULL;
    }

    reprfunc tp_repr = PyType_GetSlot(&PyLong_Type, Py_tp_repr);
    if (PyLong_Type.tp_repr != tp_repr) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: tp_repr of long");
        return NULL;
    }

    ternaryfunc tp_call = PyType_GetSlot(&PyLong_Type, Py_tp_call);
    if (tp_call != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: tp_call of long");
        return NULL;
    }

    binaryfunc nb_add = PyType_GetSlot(&PyLong_Type, Py_nb_add);
    if (PyLong_Type.tp_as_number->nb_add != nb_add) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: nb_add of long");
        return NULL;
    }

    lenfunc mp_length = PyType_GetSlot(&PyLong_Type, Py_mp_length);
    if (mp_length != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: mp_length of long");
        return NULL;
    }

    void *over_value = PyType_GetSlot(&PyLong_Type, Py_mod_name + 1);
    if (over_value != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: mod_name of long");
        return NULL;
    }

    tp_new = PyType_GetSlot(&PyLong_Type, 0);
    if (tp_new != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: slot 0 of long");
        return NULL;
    }
    if (PyErr_ExceptionMatches(PyExc_SystemError)) {
        // This is the right exception
        PyErr_Clear();
    }
    else {
        return NULL;
    }

    Py_RETURN_NONE;
}


// Get type->tp_version_tag
static PyObject *
type_get_version(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    PyObject *res = PyLong_FromUnsignedLong(type->tp_version_tag);
    if (res == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return res;
}


// Test PyUnstable_Type_AssignVersionTag()
static PyObject *
type_assign_version(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    int res = PyUnstable_Type_AssignVersionTag(type);
    return PyLong_FromLong(res);
}


// Get PyTypeObject.tp_bases
static PyObject *
type_get_tp_bases(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    PyObject *bases = type->tp_bases;
    if (bases == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(bases);
}


// Get PyTypeObject.tp_mro
static PyObject *
type_get_tp_mro(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    PyTypeObject *type = (PyTypeObject*)arg;

    PyObject *mro = type->tp_mro;
    if (mro == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(mro);
}


static PyMethodDef test_methods[] = {
    {"test_get_type_dict", test_get_type_dict, METH_NOARGS},
    {"test_get_statictype_slots", test_get_statictype_slots,     METH_NOARGS},
    {"type_get_version", type_get_version, METH_O, PyDoc_STR("type->tp_version_tag")},
    {"type_assign_version", type_assign_version, METH_O, PyDoc_STR("PyUnstable_Type_AssignVersionTag")},
    {"type_get_tp_bases", type_get_tp_bases, METH_O},
    {"type_get_tp_mro", type_get_tp_mro, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Type(PyObject *m)
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

    // Flags excluded from the limited C API
    ADD_INT(_Py_TPFLAGS_STATIC_BUILTIN);
    ADD_INT(Py_TPFLAGS_INLINE_VALUES);
    ADD_INT(Py_TPFLAGS_MANAGED_WEAKREF);
    ADD_INT(Py_TPFLAGS_MANAGED_DICT);
    ADD_INT(Py_TPFLAGS_SEQUENCE);
    ADD_INT(Py_TPFLAGS_MAPPING);

#undef ADD_INT
    return 0;
}
