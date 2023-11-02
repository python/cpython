#include "parts.h"
#include "util.h"

static Py_ssize_t
get_code_extra_index(PyInterpreterState* interp) {
    Py_ssize_t result = -1;

    static const char *key = "_testcapi.frame_evaluation.code_index";

    PyObject *interp_dict = PyInterpreterState_GetDict(interp); // borrowed
    assert(interp_dict);  // real users would handle missing dict... somehow

    PyObject *index_obj;
    if (PyDict_GetItemStringRef(interp_dict, key, &index_obj) < 0) {
        goto finally;
    }
    Py_ssize_t index = 0;
    if (!index_obj) {
        index = PyUnstable_Eval_RequestCodeExtraIndex(NULL);
        if (index < 0 || PyErr_Occurred()) {
            goto finally;
        }
        index_obj = PyLong_FromSsize_t(index); // strong ref
        if (!index_obj) {
            goto finally;
        }
        int res = PyDict_SetItemString(interp_dict, key, index_obj);
        Py_DECREF(index_obj);
        if (res < 0) {
            goto finally;
        }
    }
    else {
        index = PyLong_AsSsize_t(index_obj);
        Py_DECREF(index_obj);
        if (index == -1 && PyErr_Occurred()) {
            goto finally;
        }
    }

    result = index;
finally:
    return result;
}

static PyObject *
test_code_extra(PyObject* self, PyObject *Py_UNUSED(callable))
{
    PyObject *result = NULL;
    PyObject *test_module = NULL;
    PyObject *test_func = NULL;

    // Get or initialize interpreter-specific code object storage index
    PyInterpreterState *interp = PyInterpreterState_Get();
    if (!interp) {
        return NULL;
    }
    Py_ssize_t code_extra_index = get_code_extra_index(interp);
    if (PyErr_Occurred()) {
        goto finally;
    }

    // Get a function to test with
    // This can be any Python function. Use `test.test_misc.testfunction`.
    test_module = PyImport_ImportModule("test.test_capi.test_misc");
    if (!test_module) {
        goto finally;
    }
    test_func = PyObject_GetAttrString(test_module, "testfunction");
    if (!test_func) {
        goto finally;
    }
    PyObject *test_func_code = PyFunction_GetCode(test_func);  // borrowed
    if (!test_func_code) {
        goto finally;
    }

    // Check the value is initially NULL
    void *extra = UNINITIALIZED_PTR;
    int res = PyUnstable_Code_GetExtra(test_func_code, code_extra_index, &extra);
    if (res < 0) {
        goto finally;
    }
    assert (extra == NULL);

    // Set another code extra value
    res = PyUnstable_Code_SetExtra(test_func_code, code_extra_index, (void*)(uintptr_t)77);
    if (res < 0) {
        goto finally;
    }
    // Assert it was set correctly
    extra = UNINITIALIZED_PTR;
    res = PyUnstable_Code_GetExtra(test_func_code, code_extra_index, &extra);
    if (res < 0) {
        goto finally;
    }
    assert ((uintptr_t)extra == 77);
    // Revert to initial code extra value.
    res = PyUnstable_Code_SetExtra(test_func_code, code_extra_index, NULL);
    if (res < 0) {
        goto finally;
    }
    result = Py_NewRef(Py_None);
finally:
    Py_XDECREF(test_module);
    Py_XDECREF(test_func);
    return result;
}

static PyMethodDef TestMethods[] = {
    {"test_code_extra", test_code_extra, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Code(PyObject *m) {
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    return 0;
}
