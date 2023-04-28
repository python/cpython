#include "parts.h"

static PyObject *
test_immortal_bool(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* objects[] = {Py_True, Py_False};
    Py_ssize_t n = Py_ARRAY_LENGTH(objects);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject* obj = objects[i];
        assert(_Py_IsImmortal(obj));
        Py_ssize_t old_count = Py_REFCNT(obj);
        for (int j = 0; j < 10000; j++) {
            Py_DECREF(obj);
        }
        Py_ssize_t current_count = Py_REFCNT(obj);
        assert(old_count == current_count);
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_none(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(_Py_IsImmortal(Py_None));
    Py_ssize_t old_count = Py_REFCNT(Py_None);
    for (int i = 0; i < 10000; i++) {
        Py_DECREF(Py_None);
    }
    Py_ssize_t current_count = Py_REFCNT(Py_None);
    assert(old_count == current_count);
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_small_ints(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    for (int i = -5; i <= 256; i++) {
        PyObject *small_int = PyLong_FromLong(i);
        assert(_Py_IsImmortal(small_int));
        Py_ssize_t old_count = Py_REFCNT(small_int);
        for (int j = 0; j < 10000; j++) {
            Py_DECREF(small_int);
        }
        Py_ssize_t current_count = Py_REFCNT(small_int);
        assert(old_count == current_count);
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_ellipsis(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(_Py_IsImmortal(Py_Ellipsis));
    Py_ssize_t old_count = Py_REFCNT(Py_Ellipsis);
    for (int i = 0; i < 10000; i++) {
        Py_DECREF(Py_Ellipsis);
    }
    Py_ssize_t current_count = Py_REFCNT(Py_Ellipsis);
    assert(old_count == current_count);
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
