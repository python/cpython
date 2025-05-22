#include "parts.h"
#include "util.h"

// Test PyModule_* API

static PyObject *
module_from_slots_empty(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_name(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_name, "currently ignored..."},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_doc(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_doc, "the docstring"},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_size(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_size, (void*)123},
        {0},
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return NULL;
    }
    Py_ssize_t size;
    if (PyModule_GetSize(mod, &size) < 0) {
        return NULL;
    }
    if (PyModule_AddIntConstant(mod, "size", size) < 0) {
        return NULL;
    }
    return mod;
}

static PyObject *
a_method(PyObject *self, PyObject *arg)
{
    return PyTuple_Pack(2, self, arg);
}

static PyMethodDef a_methoddef_array[] = {
    {"a_method", a_method, METH_O},
    {0},
};

static PyObject *
module_from_slots_methods(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_methods, a_methoddef_array},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}


static int
slot_from_object(PyObject *obj)
{
    PyObject *slot_id_obj = PyObject_GetAttrString(obj, "_test_slot_id");
    if (slot_id_obj == NULL) {
        return -1;
    }
    int slot_id = PyLong_AsLong(slot_id_obj);
    if (PyErr_Occurred()) {
        return -1;
    }
    return slot_id;
}

static PyObject *
module_from_slots_repeat_slot(PyObject *self, PyObject *spec)
{
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, "anything"},
        {slot_id, "anything else"},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_null_slot(PyObject *self, PyObject *spec)
{
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, NULL},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_def_slot(PyObject *self, PyObject *spec)
{
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, "anything"},
        {0},
    };
    PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "currently ignored",
        .m_slots = slots,
    };
    return PyModule_FromDefAndSpec(&def, spec);
}

static PyMethodDef test_methods[] = {
    {"module_from_slots_empty", module_from_slots_empty, METH_O},
    {"module_from_slots_name", module_from_slots_name, METH_O},
    {"module_from_slots_doc", module_from_slots_doc, METH_O},
    {"module_from_slots_size", module_from_slots_size, METH_O},
    {"module_from_slots_methods", module_from_slots_methods, METH_O},
    {"module_from_slots_repeat_slot", module_from_slots_repeat_slot, METH_O},
    {"module_from_slots_null_slot", module_from_slots_null_slot, METH_O},
    {"module_from_def_slot", module_from_def_slot, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Module(PyObject *m)
{
#define ADD_INT_MACRO(C) if (PyModule_AddIntConstant(m, #C, C) < 0) return -1;
    ADD_INT_MACRO(Py_mod_create);
    ADD_INT_MACRO(Py_mod_exec);
    ADD_INT_MACRO(Py_mod_multiple_interpreters);
    ADD_INT_MACRO(Py_mod_gil);
    ADD_INT_MACRO(Py_mod_name);
    ADD_INT_MACRO(Py_mod_doc);
    ADD_INT_MACRO(Py_mod_size);
    ADD_INT_MACRO(Py_mod_methods);
    ADD_INT_MACRO(Py_mod_traverse);
    ADD_INT_MACRO(Py_mod_clear);
    ADD_INT_MACRO(Py_mod_free);
    ADD_INT_MACRO(Py_mod_token);
#undef ADD_INT_MACRO
    return PyModule_AddFunctions(m, test_methods);
}
