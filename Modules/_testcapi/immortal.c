#include "parts.h"

int verify_immortality(PyObject *object) 
{
    assert(_Py_IsImmortal(object));
    Py_ssize_t old_count = Py_REFCNT(object);
    for (int j = 0; j < 10000; j++) {
        Py_DECREF(object);
    }
    Py_ssize_t current_count = Py_REFCNT(object);
    return old_count == current_count;
}

static PyObject *
test_immortal_bool(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *objects[] = {Py_True, Py_False};
    Py_ssize_t n = Py_ARRAY_LENGTH(objects);
    for (Py_ssize_t i = 0; i < n; i++) {
        assert(verify_immortality(objects[i]));
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_none(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(verify_immortality(Py_None));
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_small_ints(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    for (int i = -5; i <= 256; i++) {
        assert(verify_immortality(PyLong_FromLong(i)));
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_ellipsis(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(verify_immortality(Py_Ellipsis));
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"test_immortal_bool",       test_immortal_bool,         METH_NOARGS},
    {"test_immortal_none",       test_immortal_none,         METH_NOARGS},
    {"test_immortal_small_ints", test_immortal_small_ints,   METH_NOARGS},
    {"test_immortal_ellipsis",   test_immortal_ellipsis,     METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Immortal(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
