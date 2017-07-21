#include <Python.h>
#include <stdlib.h>
#include <inttypes.h>

// List all fuzz functions here, and in the _fuzz_run_all function.
#include "fuzz_builtin_hash.inc"
#include "fuzz_builtin_int.inc"
#include "fuzz_builtin_float.inc"
#include "fuzz_builtin_unicode.inc"

// Runs fuzzer and returns nonzero if an error occurred.
int _run_fuzz(int(*fuzzer)(const char* , size_t)) {
    int rv = fuzzer("", 0);
    if (PyErr_Occurred()) {
        return 1;
    }
    if (rv != 0) {
        PyErr_Format(
            PyExc_RuntimeError, "Nonzero return code from fuzzer: %d", rv);
        return 1;
    }
    return 0;
}


static PyObject* _fuzz_run_all(PyObject* self) {
#define _Py_FUZZ_STRINGIZE(x) _Py_FUZZ_STRINGIZE2(x)
#define _Py_FUZZ_STRINGIZE2(x) #x
#define _Py_FUZZ_RUN(f) \
        do {\
            printf("%s()\n", _Py_FUZZ_STRINGIZE(f));\
            if (_run_fuzz(f)) return NULL; \
        } while (0)
    _Py_FUZZ_RUN(fuzz_builtin_hash);
    _Py_FUZZ_RUN(fuzz_builtin_int);
    _Py_FUZZ_RUN(fuzz_builtin_float);
    _Py_FUZZ_RUN(fuzz_builtin_unicode);
#undef _Py_FUZZ_RUN
#undef _Py_FUZZ_STRINGIZE
#undef _Py_FUZZ_STRINGIZE2
    Py_RETURN_NONE;
}

static PyMethodDef module_methods[] = {
    {"_fuzz_run_all", (PyCFunction)_fuzz_run_all, METH_NOARGS, ""},
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
PyInit__fuzz(void)
{
    PyObject *m = NULL;

    if ((m = PyModule_Create(&_fuzzmodule)) == NULL) {
        return NULL;
    }
    return m;
}
