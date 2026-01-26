#include "parts.h"
#include "util.h"

static PyObject *
set_check(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PySet_Check(obj));
}

static PyObject *
set_checkexact(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PySet_CheckExact(obj));
}

static PyObject *
frozenset_check(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyFrozenSet_Check(obj));
}

static PyObject *
frozenset_checkexact(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyFrozenSet_CheckExact(obj));
}

static PyObject *
anyset_check(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyAnySet_Check(obj));
}

static PyObject *
anyset_checkexact(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyAnySet_CheckExact(obj));
}

static PyObject *
set_new(PyObject *self, PyObject *args)
{
    PyObject *iterable = NULL;
    if (!PyArg_ParseTuple(args, "|O", &iterable)) {
        return NULL;
    }
    return PySet_New(iterable);
}

static PyObject *
frozenset_new(PyObject *self, PyObject *args)
{
    PyObject *iterable = NULL;
    if (!PyArg_ParseTuple(args, "|O", &iterable)) {
        return NULL;
    }
    return PyFrozenSet_New(iterable);
}

static PyObject *
set_size(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PySet_Size(obj));
}

static PyObject *
set_contains(PyObject *self, PyObject *args)
{
    PyObject *obj, *item;
    if (!PyArg_ParseTuple(args, "OO", &obj, &item)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(item);
    RETURN_INT(PySet_Contains(obj, item));
}

static PyObject *
set_add(PyObject *self, PyObject *args)
{
    PyObject *obj, *item;
    if (!PyArg_ParseTuple(args, "OO", &obj, &item)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(item);
    RETURN_INT(PySet_Add(obj, item));
}

static PyObject *
set_discard(PyObject *self, PyObject *args)
{
    PyObject *obj, *item;
    if (!PyArg_ParseTuple(args, "OO", &obj, &item)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(item);
    RETURN_INT(PySet_Discard(obj, item));
}

static PyObject *
set_pop(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    return PySet_Pop(obj);
}

static PyObject *
set_clear(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PySet_Clear(obj));
}

/* Raise AssertionError with test_name + ": " + msg, and return NULL. */

static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
    PyObject *exc = PyErr_GetRaisedException();
    PyErr_Format(exc, "%s: %s", test_name, msg);
    return NULL;
}

static PyObject *
test_pyset_add_exact_set(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Test: Adding to a regular set
    PyObject *set = PySet_New(NULL);
    if (set == NULL) {
        return NULL;
    }
    PyObject *one = PyLong_FromLong(1);
    assert(one);

    if (PySet_Add(set, one) < 0) {
        Py_DECREF(set);
        return raiseTestError("test_pyset_add_exact_set",
                              "PySet_Add to empty set failed");
    }
    if (PySet_Size(set) != 1) {
        Py_DECREF(set);
        return raiseTestError("test_pyset_add_exact_set",
                              "set size should be 1 after adding one element");
    }
    if (PySet_Contains(set, one) != 1) {
        Py_DECREF(set);
        return raiseTestError("test_pyset_add_exact_set",
                              "set should contain the added element");
    }
    Py_DECREF(set);

    // Test: Adding unhashable item should raise TypeError
    set = PySet_New(NULL);
    if (set == NULL) {
        return NULL;
    }
    PyObject *unhashable = PyList_New(0);
    if (unhashable == NULL) {
        Py_DECREF(set);
        return NULL;
    }
    if (PySet_Add(set, unhashable) != -1) {
        Py_DECREF(unhashable);
        Py_DECREF(set);
        return raiseTestError("test_pyset_add_exact_set",
                              "PySet_Add with unhashable should fail");
    }
    if (!PyErr_ExceptionMatches(PyExc_TypeError)) {
        Py_DECREF(unhashable);
        Py_DECREF(set);
        return raiseTestError("test_pyset_add_exact_set",
                              "PySet_Add with unhashable should raise TypeError");
    }
    PyErr_Clear();
    Py_DECREF(unhashable);
    Py_DECREF(set);

    Py_RETURN_NONE;
}

static PyObject *
test_frozenset_add_in_capi(PyObject *self, PyObject *Py_UNUSED(obj))
{
    // Test that `frozenset` can be used with `PySet_Add`,
    // when frozenset is just created in CAPI.
    PyObject *fs = PyFrozenSet_New(NULL);
    if (fs == NULL) {
        return NULL;
    }
    PyObject *num = PyLong_FromLong(1);
    if (num == NULL) {
        goto error;
    }
    if (PySet_Add(fs, num) < 0) {
        goto error;
    }
    int contains = PySet_Contains(fs, num);
    if (contains < 0) {
        goto error;
    }
    else if (contains == 0) {
        goto unexpected;
    }
    Py_DECREF(fs);
    Py_DECREF(num);
    Py_RETURN_NONE;

unexpected:
    PyErr_SetString(PyExc_ValueError, "set does not contain expected value");
error:
    Py_DECREF(fs);
    Py_XDECREF(num);
    return NULL;
}

static PyObject *
test_pyset_add_frozenset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *one = PyLong_FromLong(1);
    assert(one);

    // Test: Adding to uniquely-referenced frozenset should succeed
    PyObject *frozenset = PyFrozenSet_New(NULL);
    if (frozenset == NULL) {
        return NULL;
    }

    // frozenset is uniquely referenced here, so PySet_Add should work
    if (PySet_Add(frozenset, one) < 0) {
        Py_DECREF(frozenset);
        return raiseTestError("test_pyset_add_frozenset",
                              "PySet_Add to uniquely-referenced frozenset failed");
    }
    Py_DECREF(frozenset);

    // Test: Adding to non-uniquely-referenced frozenset should raise SystemError
    frozenset = PyFrozenSet_New(NULL);
    if (frozenset == NULL) {
        return NULL;
    }
    Py_INCREF(frozenset);  // Make it non-uniquely referenced

    if (PySet_Add(frozenset, one) != -1) {
        Py_DECREF(frozenset);
        Py_DECREF(frozenset);
        return raiseTestError("test_pyset_add_frozenset",
                              "PySet_Add to non-uniquely-referenced frozenset should fail");
    }
    if (!PyErr_ExceptionMatches(PyExc_SystemError)) {
        Py_DECREF(frozenset);
        Py_DECREF(frozenset);
        return raiseTestError("test_pyset_add_frozenset",
                              "PySet_Add to non-uniquely-referenced frozenset should raise SystemError");
    }
    PyErr_Clear();
    Py_DECREF(frozenset);
    Py_DECREF(frozenset);

    // Test: GC tracking - frozenset with only immutable items should not be tracked
    frozenset = PyFrozenSet_New(NULL);
    if (frozenset == NULL) {
        return NULL;
    }
    if (PySet_Add(frozenset, one) < 0) {
        Py_DECREF(frozenset);
        return NULL;
    }
    if (PyObject_GC_IsTracked(frozenset)) {
        Py_DECREF(frozenset);
        return raiseTestError("test_pyset_add_frozenset",
                              "frozenset with only int should not be GC tracked");
    }
    Py_DECREF(frozenset);

    // Test: GC tracking - frozenset with tracked object should be tracked
    frozenset = PyFrozenSet_New(NULL);
    if (frozenset == NULL) {
        return NULL;
    }

    PyObject *tracked_obj = PyErr_NewException("_testlimitedcapi.py_set_add", NULL, NULL);
    if (tracked_obj == NULL) {
        Py_DECREF(frozenset);
        return NULL;
    }
    if (!PyObject_GC_IsTracked(tracked_obj)) {
        return raiseTestError("test_pyset_add_frozenset",
                              "test object should be tracked");
    }
    if (PySet_Add(frozenset, tracked_obj) < 0) {
        Py_DECREF(frozenset);
        Py_DECREF(tracked_obj);
        return NULL;
    }
    if (!PyObject_GC_IsTracked(frozenset)) {
        Py_DECREF(frozenset);
        Py_DECREF(tracked_obj);
        return raiseTestError("test_pyset_add_frozenset",
                              "frozenset with with GC tracked object should be tracked");
    }

    Py_RETURN_NONE;
}


static PyObject *
test_set_contains_does_not_convert_unhashable_key(PyObject *self, PyObject *Py_UNUSED(obj))
{
    // See https://docs.python.org/3/c-api/set.html#c.PySet_Contains
    PyObject *outer_set = PySet_New(NULL);

    PyObject *needle = PySet_New(NULL);
    if (needle == NULL) {
        Py_DECREF(outer_set);
        return NULL;
    }

    PyObject *num = PyLong_FromLong(42);
    if (num == NULL) {
        Py_DECREF(outer_set);
        Py_DECREF(needle);
        return NULL;
    }

    if (PySet_Add(needle, num) < 0) {
        Py_DECREF(outer_set);
        Py_DECREF(needle);
        Py_DECREF(num);
        return NULL;
    }

    int result = PySet_Contains(outer_set, needle);

    Py_DECREF(num);
    Py_DECREF(needle);
    Py_DECREF(outer_set);

    if (result < 0) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        return NULL;
    }

    PyErr_SetString(PyExc_AssertionError,
                    "PySet_Contains should have raised TypeError for unhashable key");
    return NULL;
}


static PyMethodDef test_methods[] = {
    {"set_check", set_check, METH_O},
    {"set_checkexact", set_checkexact, METH_O},
    {"frozenset_check", frozenset_check, METH_O},
    {"frozenset_checkexact", frozenset_checkexact, METH_O},
    {"anyset_check", anyset_check, METH_O},
    {"anyset_checkexact", anyset_checkexact, METH_O},

    {"set_new", set_new, METH_VARARGS},
    {"frozenset_new", frozenset_new, METH_VARARGS},

    {"set_size", set_size, METH_O},
    {"set_contains", set_contains, METH_VARARGS},
    {"set_add", set_add, METH_VARARGS},
    {"set_discard", set_discard, METH_VARARGS},
    {"set_pop", set_pop, METH_O},
    {"set_clear", set_clear, METH_O},

    {"test_frozenset_add_in_capi", test_frozenset_add_in_capi, METH_NOARGS},
    {"test_set_contains_does_not_convert_unhashable_key",
     test_set_contains_does_not_convert_unhashable_key, METH_NOARGS},
    {"test_pyset_add_exact_set", test_pyset_add_exact_set, METH_NOARGS},
    {"test_pyset_add_frozenset", test_pyset_add_frozenset, METH_NOARGS},

    {NULL},
};

int
_PyTestLimitedCAPI_Init_Set(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
