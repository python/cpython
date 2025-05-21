#include "parts.h"
#include "util.h"

// Test PyModule_* API

static PyObject *
module_from_slots_and_spec(PyObject *self, PyObject *args)
{
    PyObject *spec;
    PyObject *py_slots;
    if(PyArg_UnpackTuple(args, "module_from_slots_and_spec", 2, 2,
                         &py_slots, &spec) < 1)
    {
        return NULL;
    }
    assert(PyList_Check(py_slots));
    Py_ssize_t n_slots = PyList_GET_SIZE(py_slots);
    PyModuleDef_Slot *slots = PyMem_Calloc(n_slots + 1,
                                           sizeof(PyModuleDef_Slot));
    if (!slots) {
        return PyErr_NoMemory();
    }

    PyObject *result = PyModule_FromSlotsAndSpec(slots, spec);
    PyMem_Free(slots);
    return result;
}


static PyMethodDef test_methods[] = {
    {"module_from_slots_and_spec", module_from_slots_and_spec, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Module(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
