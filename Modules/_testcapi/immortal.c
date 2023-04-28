#include "parts.h"

static PyObject*
test_immortal_bool(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* objects[] = {Py_True, Py_False};
    Py_ssize_t n = sizeof(objects) / sizeof(objects[0]);
    for(Py_ssize_t i = 0; i < n; i++) {
        PyObject* obj = objects[i];
        if (!_Py_IsImmortal(obj)) {
            PyErr_Format(PyExc_RuntimeError, "%R should be the immportal object.", obj);
            return NULL;     
        }
        Py_ssize_t old_count = Py_REFCNT(obj);
        for (int i = 0; i < 10000; i++) {
            Py_DECREF(obj);
        }
        Py_ssize_t current_count = Py_REFCNT(obj);
        if (old_count != current_count) {
            PyErr_SetString(PyExc_RuntimeError, "Reference count should not be changed.");
            return NULL;   
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_none(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* none = Py_None;
    if (!_Py_IsImmortal(none)) {
        PyErr_Format(PyExc_RuntimeError, "%R should be the immportal object.", none);
        return NULL;     
    }
    Py_ssize_t old_count = Py_REFCNT(none);
    for (int i = 0; i < 10000; i++) {
        Py_DECREF(none);
    }
    Py_ssize_t current_count = Py_REFCNT(none);
    if (old_count != current_count) {
        PyErr_SetString(PyExc_RuntimeError, "Reference count should not be changed.");
        return NULL;   
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_small_ints(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    for(int i = -5; i < 255; i++) {
        PyObject *small_int = PyLong_FromLong(i);
        if (!_Py_IsImmortal(small_int)) {
            PyErr_Format(PyExc_RuntimeError, "Small int(%d) object should be the immportal object.", i);
            return NULL;     
        }
        Py_ssize_t old_count = Py_REFCNT(small_int);
        for (int i = 0; i < 10000; i++) {
            Py_DECREF(small_int);
        }
        Py_ssize_t current_count = Py_REFCNT(small_int);
        if (old_count != current_count) {
            PyErr_Format(PyExc_RuntimeError, "Reference count of %d should not be changed.", i);
            return NULL;   
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_ellipsis(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* ellipsis = Py_Ellipsis;
    if (!_Py_IsImmortal(ellipsis)) {
        PyErr_SetString(PyExc_RuntimeError, "Ellipsis object should be the immportal object.");
        return NULL;     
    }
    Py_ssize_t old_count = Py_REFCNT(ellipsis);
    for (int i = 0; i < 10000; i++) {
        Py_DECREF(ellipsis);
    }
    Py_ssize_t current_count = Py_REFCNT(ellipsis);
    if (old_count != current_count) {
        PyErr_SetString(PyExc_RuntimeError, "Reference count should not be changed.");
        return NULL;   
    }
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