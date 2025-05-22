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


static PyMethodDef test_methods[] = {
    {"module_from_slots_empty", module_from_slots_empty, METH_O},
    {"module_from_slots_name", module_from_slots_name, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Module(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
