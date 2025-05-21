#include "parts.h"
#include "util.h"

// Test PyModule_* API

static PyObject *
module_from_slots_and_spec(PyObject *self, PyObject *py_slots)
{
    assert(PyList_Check(py_slots));
    Py_ssize_t n_slots = PyList_GET_SIZE(py_slots);
    PyModuleDef_Slot *slots = PyMem_Calloc(n_slots + 1,
                                           sizeof(PyModuleDef_Slot));
    if (!slots) {
        return PyErr_NoMemory();
    }

    return PyModule_FromSlotsAndSpec(slots, Py_None);
}


static PyMethodDef test_methods[] = {
    {"module_from_slots_and_spec", module_from_slots_and_spec, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Module(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
