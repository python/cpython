#include "pyconfig.h"   // Py_GIL_DISABLED
#ifdef Py_GIL_DISABLED
#  define Py_TARGET_ABI3T 0x030f0000
#else
   // Need limited C API version 3.13 for Py_GetConstant()
#  define Py_LIMITED_API 0x030d0000
#endif

#include "parts.h"
#include "util.h"


/* Test Py_GetConstant() */
static PyObject *
get_constant(PyObject *Py_UNUSED(module), PyObject *args)
{
    int constant_id;
    if (!PyArg_ParseTuple(args, "i", &constant_id)) {
        return NULL;
    }

    PyObject *obj = Py_GetConstant(constant_id);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return obj;
}


/* Test Py_GetConstantBorrowed() */
static PyObject *
get_constant_borrowed(PyObject *Py_UNUSED(module), PyObject *args)
{
    int constant_id;
    if (!PyArg_ParseTuple(args, "i", &constant_id)) {
        return NULL;
    }

    PyObject *obj = Py_GetConstantBorrowed(constant_id);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return Py_NewRef(obj);
}


/* Test constants */
static PyObject *
test_constants(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Test that implementation of constants in the limited C API:
    // check that the C code compiles.
    //
    // Test also that constants and Py_GetConstant() return the same
    // objects.
    assert(Py_None == Py_GetConstant(Py_CONSTANT_NONE));
    assert(Py_False == Py_GetConstant(Py_CONSTANT_FALSE));
    assert(Py_True == Py_GetConstant(Py_CONSTANT_TRUE));
    assert(Py_Ellipsis == Py_GetConstant(Py_CONSTANT_ELLIPSIS));
    assert(Py_NotImplemented == Py_GetConstant(Py_CONSTANT_NOT_IMPLEMENTED));
    // Other constants are tested in test_capi.test_object
    Py_RETURN_NONE;
}


static PyObject *
_test_incref(PyObject *ob)
{
    return Py_NewRef(ob);
}

static PyObject *
test_xincref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyLong_FromLong(0);
    Py_XINCREF(_test_incref(obj));
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyObject *
test_incref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyLong_FromLong(0);
    Py_INCREF(_test_incref(obj));
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyObject *
test_xdecref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    Py_XDECREF(PyLong_FromLong(0));
    Py_RETURN_NONE;
}


static PyObject *
test_decref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    Py_DECREF(PyLong_FromLong(0));
    Py_RETURN_NONE;
}


static PyObject *
test_incref_decref_API(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyLong_FromLong(0);
    Py_IncRef(obj);
    Py_DecRef(obj);
    Py_DecRef(obj);
    Py_RETURN_NONE;
}


// Test Py_CLEAR() macro
static PyObject*
test_py_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // simple case with a variable
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }
    Py_CLEAR(obj);
    assert(obj == NULL);

    // gh-98724: complex case, Py_CLEAR() argument has a side effect
    PyObject* array[1];
    array[0] = PyList_New(0);
    if (array[0] == NULL) {
        return NULL;
    }

    PyObject **p = array;
    Py_CLEAR(*p++);
    assert(array[0] == NULL);
    assert(p == array + 1);

    Py_RETURN_NONE;
}


// Test Py_SETREF() and Py_XSETREF() macros, similar to test_py_clear()
static PyObject*
test_py_setref(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Py_SETREF() simple case with a variable
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }
    Py_SETREF(obj, NULL);
    assert(obj == NULL);

    // Py_XSETREF() simple case with a variable
    PyObject *obj2 = PyList_New(0);
    if (obj2 == NULL) {
        return NULL;
    }
    Py_XSETREF(obj2, NULL);
    assert(obj2 == NULL);
    // test Py_XSETREF() when the argument is NULL
    Py_XSETREF(obj2, NULL);
    assert(obj2 == NULL);

    // gh-98724: complex case, Py_SETREF() argument has a side effect
    PyObject* array[1];
    array[0] = PyList_New(0);
    if (array[0] == NULL) {
        return NULL;
    }

    PyObject **p = array;
    Py_SETREF(*p++, NULL);
    assert(array[0] == NULL);
    assert(p == array + 1);

    // gh-98724: complex case, Py_XSETREF() argument has a side effect
    PyObject* array2[1];
    array2[0] = PyList_New(0);
    if (array2[0] == NULL) {
        return NULL;
    }

    PyObject **p2 = array2;
    Py_XSETREF(*p2++, NULL);
    assert(array2[0] == NULL);
    assert(p2 == array2 + 1);

    // test Py_XSETREF() when the argument is NULL
    p2 = array2;
    Py_XSETREF(*p2++, NULL);
    assert(array2[0] == NULL);
    assert(p2 == array2 + 1);

    Py_RETURN_NONE;
}


#define TEST_REFCOUNT() \
    do { \
        PyObject *obj = PyList_New(0); \
        if (obj == NULL) { \
            return NULL; \
        } \
        assert(Py_REFCNT(obj) == 1); \
        \
        /* test Py_NewRef() */ \
        PyObject *ref = Py_NewRef(obj); \
        assert(ref == obj); \
        assert(Py_REFCNT(obj) == 2); \
        Py_DECREF(ref); \
        \
        /* test Py_XNewRef() */ \
        PyObject *xref = Py_XNewRef(obj); \
        assert(xref == obj); \
        assert(Py_REFCNT(obj) == 2); \
        Py_DECREF(xref); \
        \
        assert(Py_XNewRef(NULL) == NULL); \
        \
        Py_DECREF(obj); \
        Py_RETURN_NONE; \
    } while (0)


// Test Py_NewRef() and Py_XNewRef() macros
static PyObject*
test_refcount_macros(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_REFCOUNT();
}


// Test Py_MIN(), Py_MAX() and Py_ABS() macros.
// On GCC/clang, they are implemented as a statement expression to only
// evaluate each argument only once.
static PyObject*
test_min_max_abs_macros(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int x = 5, y = 7, z = -11;

    // Simple usage
    assert(Py_MIN(x, y) == 5);
    assert(Py_MAX(x, y) == 7);
    assert(Py_ABS(z) == 11);

    // Combined macros
    assert(Py_MIN(x, Py_MIN(y, z)) == -11);
    assert(Py_MAX(x, Py_MAX(y, z)) == 7);
    assert(Py_MIN(x, Py_MAX(y, z)) == 5);
    assert(Py_MAX(x, Py_MIN(y, z)) == 5);
    assert(Py_ABS(Py_ABS(z)) == 11);

    // A few more tests
    assert(Py_MIN(2, Py_MIN(6, 12)) == 2);
    assert(Py_MAX(2, Py_MAX(6, 12)) == 12);
    assert(Py_MIN(2, Py_MAX(6, 12)) == 2);
    assert(Py_MAX(2, Py_MIN(6, 12)) == 6);
    assert(Py_MIN(Py_MIN(2, 6), 12) == 2);
    assert(Py_MAX(Py_MAX(2, 6), 12) == 12);
    assert(Py_MIN(Py_MAX(2, 6), 12) == 6);
    assert(Py_MAX(Py_MIN(2, 6), 12) == 12);
    assert(Py_ABS(Py_ABS(12)) == 12);
    assert(Py_ABS(Py_ABS(12)) == 12);

    // Integer limits
    assert(Py_MIN(123, INT_MIN) == INT_MIN);
    assert(Py_MAX(123, INT_MAX) == INT_MAX);
    assert(Py_ABS(INT_MAX) == INT_MAX);
    // Do not tests Py_ABS(INT_MIN), since the behavior is documented.
    // This limitation is documented in Py_ABS() documentation.

#if ((defined(__GNUC__) || defined(__clang__)) \
     && defined(_Py_TYPEOF) && !defined(__cplusplus))
    // Check that arguments are only evaluated once
    int a = 5, b = 7, c = -11;
    assert(Py_MIN(++a, ++b) == 6);
    assert(a == 6);
    assert(b == 8);
    assert(Py_MAX(++a, ++b) == 9);
    assert(a == 7);
    assert(b == 9);
    assert(Py_ABS(--c) == 12);
    assert(c == -12);
#endif

    Py_RETURN_NONE;
}

#undef Py_NewRef
#undef Py_XNewRef

// Test Py_NewRef() and Py_XNewRef() functions, after undefining macros.
static PyObject*
test_refcount_funcs(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_REFCOUNT();
}


// Test Py_Is() function
#define TEST_PY_IS() \
    do { \
        PyObject *o_none = Py_None; \
        PyObject *o_true = Py_True; \
        PyObject *o_false = Py_False; \
        PyObject *obj = PyList_New(0); \
        if (obj == NULL) { \
            return NULL; \
        } \
        \
        /* test Py_Is() */ \
        assert(Py_Is(obj, obj)); \
        assert(!Py_Is(obj, o_none)); \
        \
        /* test Py_None */ \
        assert(Py_Is(o_none, o_none)); \
        assert(!Py_Is(obj, o_none)); \
        \
        /* test Py_True */ \
        assert(Py_Is(o_true, o_true)); \
        assert(!Py_Is(o_false, o_true)); \
        assert(!Py_Is(obj, o_true)); \
        \
        /* test Py_False */ \
        assert(Py_Is(o_false, o_false)); \
        assert(!Py_Is(o_true, o_false)); \
        assert(!Py_Is(obj, o_false)); \
        \
        Py_DECREF(obj); \
        Py_RETURN_NONE; \
    } while (0)

// Test Py_Is() macro
static PyObject*
test_py_is_macros(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_PY_IS();
}

#undef Py_Is

// Test Py_Is() function, after undefining its macro.
static PyObject*
test_py_is_funcs(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_PY_IS();
}


static PyMethodDef test_methods[] = {
    {"get_constant", get_constant, METH_VARARGS},
    {"get_constant_borrowed", get_constant_borrowed, METH_VARARGS},
    {"test_constants", test_constants, METH_NOARGS},
    {"test_xincref_doesnt_leak",test_xincref_doesnt_leak,        METH_NOARGS},
    {"test_incref_doesnt_leak", test_incref_doesnt_leak,         METH_NOARGS},
    {"test_xdecref_doesnt_leak",test_xdecref_doesnt_leak,        METH_NOARGS},
    {"test_decref_doesnt_leak", test_decref_doesnt_leak,         METH_NOARGS},
    {"test_incref_decref_API",  test_incref_decref_API,          METH_NOARGS},
    {"test_py_clear", test_py_clear, METH_NOARGS},
    {"test_py_setref", test_py_setref, METH_NOARGS},
    {"test_refcount_macros", test_refcount_macros, METH_NOARGS},
    {"test_refcount_funcs", test_refcount_funcs, METH_NOARGS},
    {"test_min_max_abs_macros", test_min_max_abs_macros, METH_NOARGS},
    {"test_py_is_macros", test_py_is_macros, METH_NOARGS},
    {"test_py_is_funcs", test_py_is_funcs, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Object(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
