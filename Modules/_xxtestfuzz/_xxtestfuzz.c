#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>
#include <inttypes.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static PyObject* _fuzz_run(PyObject* self, PyObject* args) {
    const char* buf;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "s#", &buf, &size)) {
        return NULL;
    }
    int rv = LLVMFuzzerTestOneInput((const uint8_t*)buf, size);
    if (PyErr_Occurred()) {
        return NULL;
    }
    if (rv != 0) {
        // Nonzero return codes are reserved for future use.
        PyErr_Format(
            PyExc_RuntimeError, "Nonzero return code from fuzzer: %d", rv);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef module_methods[] = {
    {"run", (PyCFunction)_fuzz_run, METH_VARARGS, ""},
    {NULL},
};

static struct PyModuleDef _fuzzmodule = {
        PyModuleDef_HEAD_INIT,
        "_fuzz",
        NULL,
        0,
        module_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__xxtestfuzz(void)
{
    return PyModule_Create(&_fuzzmodule);
}
