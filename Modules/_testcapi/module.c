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
module_from_slots_repeat_slot(PyObject *self, PyObject *spec)
{
    PyObject *slot_id_obj = PyObject_GetAttrString(spec, "_test_slot_id");
    if (slot_id_obj == NULL) {
        return NULL;
    }
    int slot_id = PyLong_AsLong(slot_id_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, "currently ignored..."},
        {slot_id, "currently ignored..."},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}


static PyObject *
module_from_slots_null_slot(PyObject *self, PyObject *spec)
{
    PyObject *slot_id_obj = PyObject_GetAttrString(spec, "_test_slot_id");
    if (slot_id_obj == NULL) {
        return NULL;
    }
    int slot_id = PyLong_AsLong(slot_id_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, NULL},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_def_name(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_name, "currently ignored..."},
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
    {"module_from_slots_repeat_slot", module_from_slots_repeat_slot, METH_O},
    {"module_from_slots_null_slot", module_from_slots_null_slot, METH_O},
    {"module_from_def_name", module_from_def_name, METH_O},
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
