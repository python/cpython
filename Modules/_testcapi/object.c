#include "parts.h"
#include "util.h"

static PyObject *
call_pyobject_print(PyObject *self, PyObject * args)
{
    PyObject *object;
    PyObject *filename;
    PyObject *print_raw;
    FILE *fp;
    int flags = 0;

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 3, 3,
                           &object, &filename, &print_raw)) {
        return NULL;
    }

    fp = Py_fopen(filename, "w+");

    if (Py_IsTrue(print_raw)) {
        flags = Py_PRINT_RAW;
    }

    if (PyObject_Print(object, fp, flags) < 0) {
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    Py_RETURN_NONE;
}

static PyObject *
pyobject_print_null(PyObject *self, PyObject *args)
{
    PyObject *filename;
    FILE *fp;

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 1, 1, &filename)) {
        return NULL;
    }

    fp = Py_fopen(filename, "w+");

    if (PyObject_Print(NULL, fp, 0) < 0) {
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    Py_RETURN_NONE;
}

static PyObject *
pyobject_print_noref_object(PyObject *self, PyObject *args)
{
    PyObject *test_string;
    PyObject *filename;
    FILE *fp;
    char correct_string[100];

    test_string = PyUnicode_FromString("Spam spam spam");

    Py_SET_REFCNT(test_string, 0);

    PyOS_snprintf(correct_string, 100, "<refcnt %zd at %p>",
                  Py_REFCNT(test_string), (void *)test_string);

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 1, 1, &filename)) {
        return NULL;
    }

    fp = Py_fopen(filename, "w+");

    if (PyObject_Print(test_string, fp, 0) < 0){
        fclose(fp);
        Py_SET_REFCNT(test_string, 1);
        Py_DECREF(test_string);
        return NULL;
    }

    fclose(fp);

    Py_SET_REFCNT(test_string, 1);
    Py_DECREF(test_string);

    return PyUnicode_FromString(correct_string);
}

static PyObject *
pyobject_print_os_error(PyObject *self, PyObject *args)
{
    PyObject *test_string;
    PyObject *filename;
    FILE *fp;

    test_string = PyUnicode_FromString("Spam spam spam");

    if (!PyArg_UnpackTuple(args, "call_pyobject_print", 1, 1, &filename)) {
        return NULL;
    }

    // open file in read mode to induce OSError
    fp = Py_fopen(filename, "r");

    if (PyObject_Print(test_string, fp, 0) < 0) {
        fclose(fp);
        Py_DECREF(test_string);
        return NULL;
    }

    fclose(fp);
    Py_DECREF(test_string);

    Py_RETURN_NONE;
}

static PyObject *
pyobject_clear_weakrefs_no_callbacks(PyObject *self, PyObject *obj)
{
    PyUnstable_Object_ClearWeakRefsNoCallbacks(obj);
    Py_RETURN_NONE;
}

static PyObject *
pyobject_enable_deferred_refcount(PyObject *self, PyObject *obj)
{
    int result = PyUnstable_Object_EnableDeferredRefcount(obj);
    return PyLong_FromLong(result);
}

static PyObject *
pyobject_is_unique_temporary(PyObject *self, PyObject *obj)
{
    int result = PyUnstable_Object_IsUniqueReferencedTemporary(obj);
    return PyLong_FromLong(result);
}

static int MyObject_dealloc_called = 0;

static void
MyObject_dealloc(PyObject *op)
{
    // PyUnstable_TryIncRef should return 0 if object is being deallocated
    assert(Py_REFCNT(op) == 0);
    assert(!PyUnstable_TryIncRef(op));
    assert(Py_REFCNT(op) == 0);

    MyObject_dealloc_called++;
    Py_TYPE(op)->tp_free(op);
}

static PyTypeObject MyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "MyType",
    .tp_basicsize = sizeof(PyObject),
    .tp_dealloc = MyObject_dealloc,
};

static PyObject *
test_py_try_inc_ref(PyObject *self, PyObject *unused)
{
    if (PyType_Ready(&MyType) < 0) {
        return NULL;
    }

    MyObject_dealloc_called = 0;

    PyObject *op = PyObject_New(PyObject, &MyType);
    if (op == NULL) {
        return NULL;
    }

    PyUnstable_EnableTryIncRef(op);
#ifdef Py_GIL_DISABLED
    // PyUnstable_EnableTryIncRef sets the shared flags to
    // `_Py_REF_MAYBE_WEAKREF` if the flags are currently zero to ensure that
    // the shared reference count is merged on deallocation.
    assert((op->ob_ref_shared & _Py_REF_SHARED_FLAG_MASK) >= _Py_REF_MAYBE_WEAKREF);
#endif

    if (!PyUnstable_TryIncRef(op)) {
        PyErr_SetString(PyExc_AssertionError, "PyUnstable_TryIncRef failed");
        Py_DECREF(op);
        return NULL;
    }
    Py_DECREF(op);  // undo try-incref
    Py_DECREF(op);  // dealloc
    assert(MyObject_dealloc_called == 1);
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


#ifdef Py_REF_DEBUG
static PyObject *
negative_refcount(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *obj = PyUnicode_FromString("negative_refcount");
    if (obj == NULL) {
        return NULL;
    }
    assert(Py_REFCNT(obj) == 1);

    Py_SET_REFCNT(obj,  0);
    /* Py_DECREF() must call _Py_NegativeRefcount() and abort Python */
    Py_DECREF(obj);

    Py_RETURN_NONE;
}


static PyObject *
decref_freed_object(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *obj = PyUnicode_FromString("decref_freed_object");
    if (obj == NULL) {
        return NULL;
    }
    assert(Py_REFCNT(obj) == 1);

    // Deallocate the memory
    Py_DECREF(obj);
    // obj is a now a dangling pointer

    // gh-109496: If Python is built in debug mode, Py_DECREF() must call
    // _Py_NegativeRefcount() and abort Python.
    Py_DECREF(obj);

    Py_RETURN_NONE;
}
#endif


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


static PyObject *
clear_managed_dict(PyObject *self, PyObject *obj)
{
    PyObject_ClearManagedDict(obj);
    Py_RETURN_NONE;
}


static PyObject *
is_uniquely_referenced(PyObject *self, PyObject *op)
{
    return PyBool_FromLong(PyUnstable_Object_IsUniquelyReferenced(op));
}


static PyObject *
pyobject_dump(PyObject *self, PyObject *args)
{
    PyObject *op;
    int release_gil = 0;

    if (!PyArg_ParseTuple(args, "O|i", &op, &release_gil)) {
        return NULL;
    }
    NULLABLE(op);

    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        PyUnstable_Object_Dump(op);
        Py_END_ALLOW_THREADS

    }
    else {
        PyUnstable_Object_Dump(op);
    }
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"call_pyobject_print", call_pyobject_print, METH_VARARGS},
    {"pyobject_print_null", pyobject_print_null, METH_VARARGS},
    {"pyobject_print_noref_object", pyobject_print_noref_object, METH_VARARGS},
    {"pyobject_print_os_error", pyobject_print_os_error, METH_VARARGS},
    {"pyobject_clear_weakrefs_no_callbacks", pyobject_clear_weakrefs_no_callbacks, METH_O},
    {"pyobject_enable_deferred_refcount", pyobject_enable_deferred_refcount, METH_O},
    {"pyobject_is_unique_temporary", pyobject_is_unique_temporary, METH_O},
    {"test_py_try_inc_ref", test_py_try_inc_ref, METH_NOARGS},
    {"test_xincref_doesnt_leak",test_xincref_doesnt_leak,        METH_NOARGS},
    {"test_incref_doesnt_leak", test_incref_doesnt_leak,         METH_NOARGS},
    {"test_xdecref_doesnt_leak",test_xdecref_doesnt_leak,        METH_NOARGS},
    {"test_decref_doesnt_leak", test_decref_doesnt_leak,         METH_NOARGS},
    {"test_incref_decref_API",  test_incref_decref_API,          METH_NOARGS},
#ifdef Py_REF_DEBUG
    {"negative_refcount", negative_refcount, METH_NOARGS},
    {"decref_freed_object", decref_freed_object, METH_NOARGS},
#endif
    {"test_py_clear", test_py_clear, METH_NOARGS},
    {"test_py_setref", test_py_setref, METH_NOARGS},
    {"test_refcount_macros", test_refcount_macros, METH_NOARGS},
    {"test_refcount_funcs", test_refcount_funcs, METH_NOARGS},
    {"test_py_is_macros", test_py_is_macros, METH_NOARGS},
    {"test_py_is_funcs", test_py_is_funcs, METH_NOARGS},
    {"clear_managed_dict", clear_managed_dict, METH_O, NULL},
    {"is_uniquely_referenced", is_uniquely_referenced, METH_O},
    {"pyobject_dump", pyobject_dump, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Object(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
