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
module_from_slots_repeat_name(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_name, "currently ignored..."},
        {Py_mod_name, "currently ignored..."},
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
    {"module_from_slots_repeat_name", module_from_slots_repeat_name, METH_O},
    {"module_from_def_name", module_from_def_name, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Module(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
