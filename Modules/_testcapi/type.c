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

static PyObject *
get_heaptype_for_name(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyType_FromSpec(&HeapTypeNameType_Spec);
}


static PyObject *
get_type_name(PyObject *self, PyObject *type)
{
    assert(PyType_Check(type));
    return PyType_GetName((PyTypeObject *)type);
}


static PyObject *
get_type_qualname(PyObject *self, PyObject *type)
{
    assert(PyType_Check(type));
    return PyType_GetQualName((PyTypeObject *)type);
}


static PyObject *
get_type_fullyqualname(PyObject *self, PyObject *type)
{
    assert(PyType_Check(type));
    return PyType_GetFullyQualifiedName((PyTypeObject *)type);
}


static PyObject *
get_type_module_name(PyObject *self, PyObject *type)
{
    assert(PyType_Check(type));
    return PyType_GetModuleName((PyTypeObject *)type);
}


static PyObject *
test_get_type_dict(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    /* Test for PyType_GetDict */

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

    void *over_value = PyType_GetSlot(&PyLong_Type, Py_bf_releasebuffer + 1);
    if (over_value != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: max+1 of long");
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
type_get_version(PyObject *self, PyObject *type)
{
    if (!PyType_Check(type)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    PyObject *res = PyLong_FromUnsignedLong(
        ((PyTypeObject *)type)->tp_version_tag);
    if (res == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return res;
}

static PyObject *
type_modified(PyObject *self, PyObject *arg)
{
    if (!PyType_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    PyTypeObject *type = (PyTypeObject*)arg;

    PyType_Modified(type);
    Py_RETURN_NONE;
}


static PyObject *
type_assign_version(PyObject *self, PyObject *arg)
{
    if (!PyType_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    PyTypeObject *type = (PyTypeObject*)arg;

    int res = PyUnstable_Type_AssignVersionTag(type);
    return PyLong_FromLong(res);
}


static PyObject *
type_get_tp_bases(PyObject *self, PyObject *arg)
{
    if (!PyType_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    PyTypeObject *type = (PyTypeObject*)arg;

    PyObject *bases = type->tp_bases;
    if (bases == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(bases);
}

static PyObject *
type_get_tp_mro(PyObject *self, PyObject *arg)
{
    if (!PyType_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    PyTypeObject *type = (PyTypeObject*)arg;

    PyObject *mro = ((PyTypeObject *)type)->tp_mro;
    if (mro == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(mro);
}


static PyObject *
type_freeze(PyObject *module, PyObject *arg)
{
    if (!PyType_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    PyTypeObject *type = (PyTypeObject*)arg;

    if (PyType_Freeze(type) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"get_heaptype_for_name", get_heaptype_for_name, METH_NOARGS},
    {"get_type_name", get_type_name, METH_O},
    {"get_type_qualname",  get_type_qualname, METH_O},
    {"get_type_fullyqualname", get_type_fullyqualname, METH_O},
    {"get_type_module_name", get_type_module_name, METH_O},
    {"test_get_type_dict", test_get_type_dict, METH_NOARGS},
    {"test_get_statictype_slots", test_get_statictype_slots,     METH_NOARGS},
    {"type_get_version", type_get_version, METH_O, PyDoc_STR("type->tp_version_tag")},
    {"type_modified", type_modified, METH_O, PyDoc_STR("PyType_Modified")},
    {"type_assign_version", type_assign_version, METH_O, PyDoc_STR("PyUnstable_Type_AssignVersionTag")},
    {"type_get_tp_bases", type_get_tp_bases, METH_O},
    {"type_get_tp_mro", type_get_tp_mro, METH_O},
    {"type_freeze", type_freeze, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Type(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
