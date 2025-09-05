#include "parts.h"

#define Py_BUILD_CORE
#include "internal/pycore_long.h"   // IMMORTALITY_BIT_MASK

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
test_immortal_builtins(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *objects[] = {Py_True, Py_False, Py_None, Py_Ellipsis};
    Py_ssize_t n = Py_ARRAY_LENGTH(objects);
    for (Py_ssize_t i = 0; i < n; i++) {
        assert(verify_immortality(objects[i]));
    }
    Py_RETURN_NONE;
}

static PyObject *
test_immortal_small_ints(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    for (int i = -5; i <= 256; i++) {
        PyObject *obj = PyLong_FromLong(i);
        assert(verify_immortality(obj));
        int has_int_immortal_bit = ((PyLongObject *)obj)->long_value.lv_tag & IMMORTALITY_BIT_MASK;
        assert(has_int_immortal_bit);
    }
    for (int i = 257; i <= 260; i++) {
        PyObject *obj = PyLong_FromLong(i);
        assert(obj);
        int has_int_immortal_bit = ((PyLongObject *)obj)->long_value.lv_tag & IMMORTALITY_BIT_MASK;
        assert(!has_int_immortal_bit);
        Py_DECREF(obj);
    }
    Py_RETURN_NONE;
}

static PyObject *
is_immortal(PyObject *self, PyObject *op)
{
    return PyBool_FromLong(PyUnstable_IsImmortal(op));
}

static PyMethodDef test_methods[] = {
    {"test_immortal_builtins",   test_immortal_builtins,     METH_NOARGS},
    {"test_immortal_small_ints", test_immortal_small_ints,   METH_NOARGS},
    {"is_immortal",              is_immortal,                METH_O},
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
