/*
 * C Extension module to test Python interpreter C APIs.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi.py.
 */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include <float.h>
#include "structmember.h"
#include "datetime.h"

#ifdef WITH_THREAD
#include "pythread.h"
#endif /* WITH_THREAD */
static PyObject *TestError;     /* set to exception object in init */

/* Raise TestError with test_name + ": " + msg, and return NULL. */

static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
    char buf[2048];

    if (strlen(test_name) + strlen(msg) > sizeof(buf) - 50)
        PyErr_SetString(TestError, "internal error msg too large");
    else {
        PyOS_snprintf(buf, sizeof(buf), "%s: %s", test_name, msg);
        PyErr_SetString(TestError, buf);
    }
    return NULL;
}

/* Test #defines from pyconfig.h (particularly the SIZEOF_* defines).

   The ones derived from autoconf on the UNIX-like OSes can be relied
   upon (in the absence of sloppy cross-compiling), but the Windows
   platforms have these hardcoded.  Better safe than sorry.
*/
static PyObject*
sizeof_error(const char* fatname, const char* typname,
    int expected, int got)
{
    char buf[1024];
    PyOS_snprintf(buf, sizeof(buf),
        "%.200s #define == %d but sizeof(%.200s) == %d",
        fatname, expected, typname, got);
    PyErr_SetString(TestError, buf);
    return (PyObject*)NULL;
}

static PyObject*
test_config(PyObject *self)
{
#define CHECK_SIZEOF(FATNAME, TYPE) \
            if (FATNAME != sizeof(TYPE)) \
                return sizeof_error(#FATNAME, #TYPE, FATNAME, sizeof(TYPE))

    CHECK_SIZEOF(SIZEOF_SHORT, short);
    CHECK_SIZEOF(SIZEOF_INT, int);
    CHECK_SIZEOF(SIZEOF_LONG, long);
    CHECK_SIZEOF(SIZEOF_VOID_P, void*);
    CHECK_SIZEOF(SIZEOF_TIME_T, time_t);
#ifdef HAVE_LONG_LONG
    CHECK_SIZEOF(SIZEOF_LONG_LONG, PY_LONG_LONG);
#endif

#undef CHECK_SIZEOF

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
test_list_api(PyObject *self)
{
    PyObject* list;
    int i;

    /* SF bug 132008:  PyList_Reverse segfaults */
#define NLIST 30
    list = PyList_New(NLIST);
    if (list == (PyObject*)NULL)
        return (PyObject*)NULL;
    /* list = range(NLIST) */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyLong_FromLong(i);
        if (anint == (PyObject*)NULL) {
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
        PyList_SET_ITEM(list, i, anint);
    }
    /* list.reverse(), via PyList_Reverse() */
    i = PyList_Reverse(list);   /* should not blow up! */
    if (i != 0) {
        Py_DECREF(list);
        return (PyObject*)NULL;
    }
    /* Check that list == range(29, -1, -1) now */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyList_GET_ITEM(list, i);
        if (PyLong_AS_LONG(anint) != NLIST-1-i) {
            PyErr_SetString(TestError,
                            "test_list_api: reverse screwed up");
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
    }
    Py_DECREF(list);
#undef NLIST

    Py_INCREF(Py_None);
    return Py_None;
}

static int
test_dict_inner(int count)
{
    Py_ssize_t pos = 0, iterations = 0;
    int i;
    PyObject *dict = PyDict_New();
    PyObject *v, *k;

    if (dict == NULL)
        return -1;

    for (i = 0; i < count; i++) {
        v = PyLong_FromLong(i);
        PyDict_SetItem(dict, v, v);
        Py_DECREF(v);
    }

    while (PyDict_Next(dict, &pos, &k, &v)) {
        PyObject *o;
        iterations++;

        i = PyLong_AS_LONG(v) + 1;
        o = PyLong_FromLong(i);
        if (o == NULL)
            return -1;
        if (PyDict_SetItem(dict, k, o) < 0) {
            Py_DECREF(o);
            return -1;
        }
        Py_DECREF(o);
    }

    Py_DECREF(dict);

    if (iterations != count) {
        PyErr_SetString(
            TestError,
            "test_dict_iteration: dict iteration went wrong ");
        return -1;
    } else {
        return 0;
    }
}

static PyObject*
test_dict_iteration(PyObject* self)
{
    int i;

    for (i = 0; i < 200; i++) {
        if (test_dict_inner(i) < 0) {
            return NULL;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}


/* Issue #4701: Check that PyObject_Hash implicitly calls
 *   PyType_Ready if it hasn't already been called
 */
static PyTypeObject _HashInheritanceTester_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "hashinheritancetester",            /* Name of this type */
    sizeof(PyObject),           /* Basic object size */
    0,                          /* Item size for varobject */
    (destructor)PyObject_Del, /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_reserved */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    PyObject_GenericGetAttr,  /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    0,                          /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    0,                          /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    0,                          /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};

static PyObject*
test_lazy_hash_inheritance(PyObject* self)
{
    PyTypeObject *type;
    PyObject *obj;
    Py_hash_t hash;

    type = &_HashInheritanceTester_Type;

    if (type->tp_dict != NULL)
        /* The type has already been initialized. This probably means
           -R is being used. */
        Py_RETURN_NONE;


    obj = PyObject_New(PyObject, type);
    if (obj == NULL) {
        PyErr_Clear();
        PyErr_SetString(
            TestError,
            "test_lazy_hash_inheritance: failed to create object");
        return NULL;
    }

    if (type->tp_dict != NULL) {
        PyErr_SetString(
            TestError,
            "test_lazy_hash_inheritance: type initialised too soon");
        Py_DECREF(obj);
        return NULL;
    }

    hash = PyObject_Hash(obj);
    if ((hash == -1) && PyErr_Occurred()) {
        PyErr_Clear();
        PyErr_SetString(
            TestError,
            "test_lazy_hash_inheritance: could not hash object");
        Py_DECREF(obj);
        return NULL;
    }

    if (type->tp_dict == NULL) {
        PyErr_SetString(
            TestError,
            "test_lazy_hash_inheritance: type not initialised by hash()");
        Py_DECREF(obj);
        return NULL;
    }

    if (type->tp_hash != PyType_Type.tp_hash) {
        PyErr_SetString(
            TestError,
            "test_lazy_hash_inheritance: unexpected hash function");
        Py_DECREF(obj);
        return NULL;
    }

    Py_DECREF(obj);

    Py_RETURN_NONE;
}


/* Issue #7385: Check that memoryview() does not crash
 *   when bf_getbuffer returns an error
 */

static int
broken_buffer_getbuffer(PyObject *self, Py_buffer *view, int flags)
{
    PyErr_SetString(
        TestError,
        "test_broken_memoryview: expected error in bf_getbuffer");
    return -1;
}

static PyBufferProcs memoryviewtester_as_buffer = {
    (getbufferproc)broken_buffer_getbuffer,     /* bf_getbuffer */
    0,          /* bf_releasebuffer */
};

static PyTypeObject _MemoryViewTester_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "memoryviewtester",         /* Name of this type */
    sizeof(PyObject),           /* Basic object size */
    0,                          /* Item size for varobject */
    (destructor)PyObject_Del, /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_compare */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    PyObject_GenericGetAttr,  /* tp_getattro */
    0,                          /* tp_setattro */
    &memoryviewtester_as_buffer,                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    0,                          /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    0,                          /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    0,                          /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};

static PyObject*
test_broken_memoryview(PyObject* self)
{
    PyObject *obj = PyObject_New(PyObject, &_MemoryViewTester_Type);
    PyObject *res;

    if (obj == NULL) {
        PyErr_Clear();
        PyErr_SetString(
            TestError,
            "test_broken_memoryview: failed to create object");
        return NULL;
    }

    res = PyMemoryView_FromObject(obj);
    if (res || !PyErr_Occurred()){
        PyErr_SetString(
            TestError,
            "test_broken_memoryview: memoryview() didn't raise an Exception");
        Py_XDECREF(res);
        Py_DECREF(obj);
        return NULL;
    }

    PyErr_Clear();
    Py_DECREF(obj);
    Py_RETURN_NONE;
}


/* Tests of PyLong_{As, From}{Unsigned,}Long(), and (#ifdef HAVE_LONG_LONG)
   PyLong_{As, From}{Unsigned,}LongLong().

   Note that the meat of the test is contained in testcapi_long.h.
   This is revolting, but delicate code duplication is worse:  "almost
   exactly the same" code is needed to test PY_LONG_LONG, but the ubiquitous
   dependence on type names makes it impossible to use a parameterized
   function.  A giant macro would be even worse than this.  A C++ template
   would be perfect.

   The "report an error" functions are deliberately not part of the #include
   file:  if the test fails, you can set a breakpoint in the appropriate
   error function directly, and crawl back from there in the debugger.
*/

#define UNBIND(X)  Py_DECREF(X); (X) = NULL

static PyObject *
raise_test_long_error(const char* msg)
{
    return raiseTestError("test_long_api", msg);
}

#define TESTNAME        test_long_api_inner
#define TYPENAME        long
#define F_S_TO_PY       PyLong_FromLong
#define F_PY_TO_S       PyLong_AsLong
#define F_U_TO_PY       PyLong_FromUnsignedLong
#define F_PY_TO_U       PyLong_AsUnsignedLong

#include "testcapi_long.h"

static PyObject *
test_long_api(PyObject* self)
{
    return TESTNAME(raise_test_long_error);
}

#undef TESTNAME
#undef TYPENAME
#undef F_S_TO_PY
#undef F_PY_TO_S
#undef F_U_TO_PY
#undef F_PY_TO_U

#ifdef HAVE_LONG_LONG

static PyObject *
raise_test_longlong_error(const char* msg)
{
    return raiseTestError("test_longlong_api", msg);
}

#define TESTNAME        test_longlong_api_inner
#define TYPENAME        PY_LONG_LONG
#define F_S_TO_PY       PyLong_FromLongLong
#define F_PY_TO_S       PyLong_AsLongLong
#define F_U_TO_PY       PyLong_FromUnsignedLongLong
#define F_PY_TO_U       PyLong_AsUnsignedLongLong

#include "testcapi_long.h"

static PyObject *
test_longlong_api(PyObject* self, PyObject *args)
{
    return TESTNAME(raise_test_longlong_error);
}

#undef TESTNAME
#undef TYPENAME
#undef F_S_TO_PY
#undef F_PY_TO_S
#undef F_U_TO_PY
#undef F_PY_TO_U

/* Test the PyLong_AsLongAndOverflow API. General conversion to PY_LONG
   is tested by test_long_api_inner. This test will concentrate on proper
   handling of overflow.
*/

static PyObject *
test_long_and_overflow(PyObject *self)
{
    PyObject *num, *one, *temp;
    long value;
    int overflow;

    /* Test that overflow is set properly for a large value. */
    /* num is a number larger than LONG_MAX even on 64-bit platforms */
    num = PyLong_FromString("FFFFFFFFFFFFFFFFFFFFFFFF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_and_overflow",
            "return value was not set to -1");
    if (overflow != 1)
        return raiseTestError("test_long_and_overflow",
            "overflow was not set to 1");

    /* Same again, with num = LONG_MAX + 1 */
    num = PyLong_FromLong(LONG_MAX);
    if (num == NULL)
        return NULL;
    one = PyLong_FromLong(1L);
    if (one == NULL) {
        Py_DECREF(num);
        return NULL;
    }
    temp = PyNumber_Add(num, one);
    Py_DECREF(one);
    Py_DECREF(num);
    num = temp;
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_and_overflow",
            "return value was not set to -1");
    if (overflow != 1)
        return raiseTestError("test_long_and_overflow",
            "overflow was not set to 1");

    /* Test that overflow is set properly for a large negative value. */
    /* num is a number smaller than LONG_MIN even on 64-bit platforms */
    num = PyLong_FromString("-FFFFFFFFFFFFFFFFFFFFFFFF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_and_overflow",
            "return value was not set to -1");
    if (overflow != -1)
        return raiseTestError("test_long_and_overflow",
            "overflow was not set to -1");

    /* Same again, with num = LONG_MIN - 1 */
    num = PyLong_FromLong(LONG_MIN);
    if (num == NULL)
        return NULL;
    one = PyLong_FromLong(1L);
    if (one == NULL) {
        Py_DECREF(num);
        return NULL;
    }
    temp = PyNumber_Subtract(num, one);
    Py_DECREF(one);
    Py_DECREF(num);
    num = temp;
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_and_overflow",
            "return value was not set to -1");
    if (overflow != -1)
        return raiseTestError("test_long_and_overflow",
            "overflow was not set to -1");

    /* Test that overflow is cleared properly for small values. */
    num = PyLong_FromString("FF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != 0xFF)
        return raiseTestError("test_long_and_overflow",
            "expected return value 0xFF");
    if (overflow != 0)
        return raiseTestError("test_long_and_overflow",
            "overflow was not cleared");

    num = PyLong_FromString("-FF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -0xFF)
        return raiseTestError("test_long_and_overflow",
            "expected return value 0xFF");
    if (overflow != 0)
        return raiseTestError("test_long_and_overflow",
            "overflow was set incorrectly");

    num = PyLong_FromLong(LONG_MAX);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != LONG_MAX)
        return raiseTestError("test_long_and_overflow",
            "expected return value LONG_MAX");
    if (overflow != 0)
        return raiseTestError("test_long_and_overflow",
            "overflow was not cleared");

    num = PyLong_FromLong(LONG_MIN);
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != LONG_MIN)
        return raiseTestError("test_long_and_overflow",
            "expected return value LONG_MIN");
    if (overflow != 0)
        return raiseTestError("test_long_and_overflow",
            "overflow was not cleared");

    Py_INCREF(Py_None);
    return Py_None;
}

/* Test the PyLong_AsLongLongAndOverflow API. General conversion to
   PY_LONG_LONG is tested by test_long_api_inner. This test will
   concentrate on proper handling of overflow.
*/

static PyObject *
test_long_long_and_overflow(PyObject *self)
{
    PyObject *num, *one, *temp;
    PY_LONG_LONG value;
    int overflow;

    /* Test that overflow is set properly for a large value. */
    /* num is a number larger than PY_LLONG_MAX on a typical machine. */
    num = PyLong_FromString("FFFFFFFFFFFFFFFFFFFFFFFF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_long_and_overflow",
            "return value was not set to -1");
    if (overflow != 1)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not set to 1");

    /* Same again, with num = PY_LLONG_MAX + 1 */
    num = PyLong_FromLongLong(PY_LLONG_MAX);
    if (num == NULL)
        return NULL;
    one = PyLong_FromLong(1L);
    if (one == NULL) {
        Py_DECREF(num);
        return NULL;
    }
    temp = PyNumber_Add(num, one);
    Py_DECREF(one);
    Py_DECREF(num);
    num = temp;
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_long_and_overflow",
            "return value was not set to -1");
    if (overflow != 1)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not set to 1");

    /* Test that overflow is set properly for a large negative value. */
    /* num is a number smaller than PY_LLONG_MIN on a typical platform */
    num = PyLong_FromString("-FFFFFFFFFFFFFFFFFFFFFFFF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_long_and_overflow",
            "return value was not set to -1");
    if (overflow != -1)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not set to -1");

    /* Same again, with num = PY_LLONG_MIN - 1 */
    num = PyLong_FromLongLong(PY_LLONG_MIN);
    if (num == NULL)
        return NULL;
    one = PyLong_FromLong(1L);
    if (one == NULL) {
        Py_DECREF(num);
        return NULL;
    }
    temp = PyNumber_Subtract(num, one);
    Py_DECREF(one);
    Py_DECREF(num);
    num = temp;
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -1)
        return raiseTestError("test_long_long_and_overflow",
            "return value was not set to -1");
    if (overflow != -1)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not set to -1");

    /* Test that overflow is cleared properly for small values. */
    num = PyLong_FromString("FF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != 0xFF)
        return raiseTestError("test_long_long_and_overflow",
            "expected return value 0xFF");
    if (overflow != 0)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not cleared");

    num = PyLong_FromString("-FF", NULL, 16);
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != -0xFF)
        return raiseTestError("test_long_long_and_overflow",
            "expected return value 0xFF");
    if (overflow != 0)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was set incorrectly");

    num = PyLong_FromLongLong(PY_LLONG_MAX);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != PY_LLONG_MAX)
        return raiseTestError("test_long_long_and_overflow",
            "expected return value PY_LLONG_MAX");
    if (overflow != 0)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not cleared");

    num = PyLong_FromLongLong(PY_LLONG_MIN);
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != PY_LLONG_MIN)
        return raiseTestError("test_long_long_and_overflow",
            "expected return value PY_LLONG_MIN");
    if (overflow != 0)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not cleared");

    Py_INCREF(Py_None);
    return Py_None;
}

/* Test the L code for PyArg_ParseTuple.  This should deliver a PY_LONG_LONG
   for both long and int arguments.  The test may leak a little memory if
   it fails.
*/
static PyObject *
test_L_code(PyObject *self)
{
    PyObject *tuple, *num;
    PY_LONG_LONG value;

    tuple = PyTuple_New(1);
    if (tuple == NULL)
        return NULL;

    num = PyLong_FromLong(42);
    if (num == NULL)
        return NULL;

    PyTuple_SET_ITEM(tuple, 0, num);

    value = -1;
    if (PyArg_ParseTuple(tuple, "L:test_L_code", &value) < 0)
        return NULL;
    if (value != 42)
        return raiseTestError("test_L_code",
            "L code returned wrong value for long 42");

    Py_DECREF(num);
    num = PyLong_FromLong(42);
    if (num == NULL)
        return NULL;

    PyTuple_SET_ITEM(tuple, 0, num);

    value = -1;
    if (PyArg_ParseTuple(tuple, "L:test_L_code", &value) < 0)
        return NULL;
    if (value != 42)
        return raiseTestError("test_L_code",
            "L code returned wrong value for int 42");

    Py_DECREF(tuple);
    Py_INCREF(Py_None);
    return Py_None;
}

#endif  /* ifdef HAVE_LONG_LONG */

/* Test tuple argument processing */
static PyObject *
getargs_tuple(PyObject *self, PyObject *args)
{
    int a, b, c;
    if (!PyArg_ParseTuple(args, "i(ii)", &a, &b, &c))
        return NULL;
    return Py_BuildValue("iii", a, b, c);
}

/* test PyArg_ParseTupleAndKeywords */
static PyObject *getargs_keywords(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"arg1","arg2","arg3","arg4","arg5", NULL};
    static char *fmt="(ii)i|(i(ii))(iii)i";
    int int_args[10]={-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, fmt, keywords,
        &int_args[0], &int_args[1], &int_args[2], &int_args[3], &int_args[4],
        &int_args[5], &int_args[6], &int_args[7], &int_args[8], &int_args[9]))
        return NULL;
    return Py_BuildValue("iiiiiiiiii",
        int_args[0], int_args[1], int_args[2], int_args[3], int_args[4],
        int_args[5], int_args[6], int_args[7], int_args[8], int_args[9]);
}

/* Functions to call PyArg_ParseTuple with integer format codes,
   and return the result.
*/
static PyObject *
getargs_b(PyObject *self, PyObject *args)
{
    unsigned char value;
    if (!PyArg_ParseTuple(args, "b", &value))
        return NULL;
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_B(PyObject *self, PyObject *args)
{
    unsigned char value;
    if (!PyArg_ParseTuple(args, "B", &value))
        return NULL;
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_h(PyObject *self, PyObject *args)
{
    short value;
    if (!PyArg_ParseTuple(args, "h", &value))
        return NULL;
    return PyLong_FromLong((long)value);
}

static PyObject *
getargs_H(PyObject *self, PyObject *args)
{
    unsigned short value;
    if (!PyArg_ParseTuple(args, "H", &value))
        return NULL;
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_I(PyObject *self, PyObject *args)
{
    unsigned int value;
    if (!PyArg_ParseTuple(args, "I", &value))
        return NULL;
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_k(PyObject *self, PyObject *args)
{
    unsigned long value;
    if (!PyArg_ParseTuple(args, "k", &value))
        return NULL;
    return PyLong_FromUnsignedLong(value);
}

static PyObject *
getargs_i(PyObject *self, PyObject *args)
{
    int value;
    if (!PyArg_ParseTuple(args, "i", &value))
        return NULL;
    return PyLong_FromLong((long)value);
}

static PyObject *
getargs_l(PyObject *self, PyObject *args)
{
    long value;
    if (!PyArg_ParseTuple(args, "l", &value))
        return NULL;
    return PyLong_FromLong(value);
}

static PyObject *
getargs_n(PyObject *self, PyObject *args)
{
    Py_ssize_t value;
    if (!PyArg_ParseTuple(args, "n", &value))
        return NULL;
    return PyLong_FromSsize_t(value);
}

#ifdef HAVE_LONG_LONG
static PyObject *
getargs_L(PyObject *self, PyObject *args)
{
    PY_LONG_LONG value;
    if (!PyArg_ParseTuple(args, "L", &value))
        return NULL;
    return PyLong_FromLongLong(value);
}

static PyObject *
getargs_K(PyObject *self, PyObject *args)
{
    unsigned PY_LONG_LONG value;
    if (!PyArg_ParseTuple(args, "K", &value))
        return NULL;
    return PyLong_FromUnsignedLongLong(value);
}
#endif

/* This function not only tests the 'k' getargs code, but also the
   PyLong_AsUnsignedLongMask() and PyLong_AsUnsignedLongMask() functions. */
static PyObject *
test_k_code(PyObject *self)
{
    PyObject *tuple, *num;
    unsigned long value;

    tuple = PyTuple_New(1);
    if (tuple == NULL)
        return NULL;

    /* a number larger than ULONG_MAX even on 64-bit platforms */
    num = PyLong_FromString("FFFFFFFFFFFFFFFFFFFFFFFF", NULL, 16);
    if (num == NULL)
        return NULL;

    value = PyLong_AsUnsignedLongMask(num);
    if (value != ULONG_MAX)
        return raiseTestError("test_k_code",
        "PyLong_AsUnsignedLongMask() returned wrong value for long 0xFFF...FFF");

    PyTuple_SET_ITEM(tuple, 0, num);

    value = 0;
    if (PyArg_ParseTuple(tuple, "k:test_k_code", &value) < 0)
        return NULL;
    if (value != ULONG_MAX)
        return raiseTestError("test_k_code",
            "k code returned wrong value for long 0xFFF...FFF");

    Py_DECREF(num);
    num = PyLong_FromString("-FFFFFFFF000000000000000042", NULL, 16);
    if (num == NULL)
        return NULL;

    value = PyLong_AsUnsignedLongMask(num);
    if (value != (unsigned long)-0x42)
        return raiseTestError("test_k_code",
        "PyLong_AsUnsignedLongMask() returned wrong value for long 0xFFF...FFF");

    PyTuple_SET_ITEM(tuple, 0, num);

    value = 0;
    if (PyArg_ParseTuple(tuple, "k:test_k_code", &value) < 0)
        return NULL;
    if (value != (unsigned long)-0x42)
        return raiseTestError("test_k_code",
            "k code returned wrong value for long -0xFFF..000042");

    Py_DECREF(tuple);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
getargs_s(PyObject *self, PyObject *args)
{
    char *str;
    if (!PyArg_ParseTuple(args, "s", &str))
        return NULL;
    return PyBytes_FromString(str);
}

static PyObject *
getargs_s_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "s*", &buffer))
        return NULL;
    bytes = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    PyBuffer_Release(&buffer);
    return bytes;
}

static PyObject *
getargs_s_hash(PyObject *self, PyObject *args)
{
    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "s#", &str, &size))
        return NULL;
    return PyBytes_FromStringAndSize(str, size);
}

static PyObject *
getargs_z(PyObject *self, PyObject *args)
{
    char *str;
    if (!PyArg_ParseTuple(args, "z", &str))
        return NULL;
    if (str != NULL)
        return PyBytes_FromString(str);
    else
        Py_RETURN_NONE;
}

static PyObject *
getargs_z_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "z*", &buffer))
        return NULL;
    if (buffer.buf != NULL)
        bytes = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    else {
        Py_INCREF(Py_None);
        bytes = Py_None;
    }
    PyBuffer_Release(&buffer);
    return bytes;
}

static PyObject *
getargs_z_hash(PyObject *self, PyObject *args)
{
    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "z#", &str, &size))
        return NULL;
    if (str != NULL)
        return PyBytes_FromStringAndSize(str, size);
    else
        Py_RETURN_NONE;
}

static PyObject *
getargs_y(PyObject *self, PyObject *args)
{
    char *str;
    if (!PyArg_ParseTuple(args, "y", &str))
        return NULL;
    return PyBytes_FromString(str);
}

static PyObject *
getargs_y_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "y*", &buffer))
        return NULL;
    bytes = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    PyBuffer_Release(&buffer);
    return bytes;
}

static PyObject *
getargs_y_hash(PyObject *self, PyObject *args)
{
    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "y#", &str, &size))
        return NULL;
    return PyBytes_FromStringAndSize(str, size);
}

static PyObject *
getargs_u(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "u", &str))
        return NULL;
    size = Py_UNICODE_strlen(str);
    return PyUnicode_FromUnicode(str, size);
}

static PyObject *
getargs_u_hash(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "u#", &str, &size))
        return NULL;
    return PyUnicode_FromUnicode(str, size);
}

static PyObject *
getargs_Z(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Z", &str))
        return NULL;
    if (str != NULL) {
        size = Py_UNICODE_strlen(str);
        return PyUnicode_FromUnicode(str, size);
    } else
        Py_RETURN_NONE;
}

static PyObject *
getargs_Z_hash(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Z#", &str, &size))
        return NULL;
    if (str != NULL)
        return PyUnicode_FromUnicode(str, size);
    else
        Py_RETURN_NONE;
}

/* Test the s and z codes for PyArg_ParseTuple.
*/
static PyObject *
test_s_code(PyObject *self)
{
    /* Unicode strings should be accepted */
    PyObject *tuple, *obj;
    char *value;

    tuple = PyTuple_New(1);
    if (tuple == NULL)
    return NULL;

    obj = PyUnicode_Decode("t\xeate", strlen("t\xeate"),
                           "latin-1", NULL);
    if (obj == NULL)
    return NULL;

    PyTuple_SET_ITEM(tuple, 0, obj);

    /* These two blocks used to raise a TypeError:
     * "argument must be string without null bytes, not str"
     */
    if (PyArg_ParseTuple(tuple, "s:test_s_code1", &value) < 0)
    return NULL;

    if (PyArg_ParseTuple(tuple, "z:test_s_code2", &value) < 0)
    return NULL;

    Py_DECREF(tuple);
    Py_RETURN_NONE;
}

static PyObject *
test_bug_7414(PyObject *self)
{
    /* Issue #7414: for PyArg_ParseTupleAndKeywords, 'C' code wasn't being
       skipped properly in skipitem() */
    int a = 0, b = 0, result;
    char *kwlist[] = {"a", "b", NULL};
    PyObject *tuple = NULL, *dict = NULL, *b_str;

    tuple = PyTuple_New(0);
    if (tuple == NULL)
        goto failure;
    dict = PyDict_New();
    if (dict == NULL)
        goto failure;
    b_str = PyUnicode_FromString("b");
    if (b_str == NULL)
        goto failure;
    result = PyDict_SetItemString(dict, "b", b_str);
    Py_DECREF(b_str);
    if (result < 0)
        goto failure;

    result = PyArg_ParseTupleAndKeywords(tuple, dict, "|CC",
                                         kwlist, &a, &b);
    if (!result)
        goto failure;

    if (a != 0)
        return raiseTestError("test_bug_7414",
            "C format code not skipped properly");
    if (b != 'b')
        return raiseTestError("test_bug_7414",
            "C format code returned wrong value");

    Py_DECREF(dict);
    Py_DECREF(tuple);
    Py_RETURN_NONE;

  failure:
    Py_XDECREF(dict);
    Py_XDECREF(tuple);
    return NULL;
}


static volatile int x;

/* Test the u and u# codes for PyArg_ParseTuple. May leak memory in case
   of an error.
*/
static PyObject *
test_u_code(PyObject *self)
{
    PyObject *tuple, *obj;
    Py_UNICODE *value;
    Py_ssize_t len;

    /* issue4122: Undefined reference to _Py_ascii_whitespace on Windows */
    /* Just use the macro and check that it compiles */
    x = Py_UNICODE_ISSPACE(25);

    tuple = PyTuple_New(1);
    if (tuple == NULL)
        return NULL;

    obj = PyUnicode_Decode("test", strlen("test"),
                           "ascii", NULL);
    if (obj == NULL)
        return NULL;

    PyTuple_SET_ITEM(tuple, 0, obj);

    value = 0;
    if (PyArg_ParseTuple(tuple, "u:test_u_code", &value) < 0)
        return NULL;
    if (value != PyUnicode_AS_UNICODE(obj))
        return raiseTestError("test_u_code",
            "u code returned wrong value for u'test'");
    value = 0;
    if (PyArg_ParseTuple(tuple, "u#:test_u_code", &value, &len) < 0)
        return NULL;
    if (value != PyUnicode_AS_UNICODE(obj) ||
        len != PyUnicode_GET_SIZE(obj))
        return raiseTestError("test_u_code",
            "u# code returned wrong values for u'test'");

    Py_DECREF(tuple);
    Py_INCREF(Py_None);
    return Py_None;
}

/* Test Z and Z# codes for PyArg_ParseTuple */
static PyObject *
test_Z_code(PyObject *self)
{
    PyObject *tuple, *obj;
    Py_UNICODE *value1, *value2;
    Py_ssize_t len1, len2;

    tuple = PyTuple_New(2);
    if (tuple == NULL)
        return NULL;

    obj = PyUnicode_FromString("test");
    PyTuple_SET_ITEM(tuple, 0, obj);
    Py_INCREF(Py_None);
    PyTuple_SET_ITEM(tuple, 1, Py_None);

    /* swap values on purpose */
    value1 = NULL;
    value2 = PyUnicode_AS_UNICODE(obj);

    /* Test Z for both values */
    if (PyArg_ParseTuple(tuple, "ZZ:test_Z_code", &value1, &value2) < 0)
        return NULL;
    if (value1 != PyUnicode_AS_UNICODE(obj))
        return raiseTestError("test_Z_code",
            "Z code returned wrong value for 'test'");
    if (value2 != NULL)
        return raiseTestError("test_Z_code",
            "Z code returned wrong value for None");

    value1 = NULL;
    value2 = PyUnicode_AS_UNICODE(obj);
    len1 = -1;
    len2 = -1;

    /* Test Z# for both values */
    if (PyArg_ParseTuple(tuple, "Z#Z#:test_Z_code", &value1, &len1,
                         &value2, &len2) < 0)
        return NULL;
    if (value1 != PyUnicode_AS_UNICODE(obj) ||
        len1 != PyUnicode_GET_SIZE(obj))
        return raiseTestError("test_Z_code",
            "Z# code returned wrong values for 'test'");
    if (value2 != NULL ||
        len2 != 0)
        return raiseTestError("test_Z_code",
            "Z# code returned wrong values for None'");

    Py_DECREF(tuple);
    Py_RETURN_NONE;
}

static PyObject *
test_widechar(PyObject *self)
{
#if defined(SIZEOF_WCHAR_T) && (SIZEOF_WCHAR_T == 4)
    const wchar_t wtext[2] = {(wchar_t)0x10ABCDu};
    size_t wtextlen = 1;
#else
    const wchar_t wtext[3] = {(wchar_t)0xDBEAu, (wchar_t)0xDFCDu};
    size_t wtextlen = 2;
#endif
    PyObject *wide, *utf8;

    wide = PyUnicode_FromWideChar(wtext, wtextlen);
    if (wide == NULL)
        return NULL;

    utf8 = PyUnicode_FromString("\xf4\x8a\xaf\x8d");
    if (utf8 == NULL) {
        Py_DECREF(wide);
        return NULL;
    }

    if (PyUnicode_GET_SIZE(wide) != PyUnicode_GET_SIZE(utf8)) {
        Py_DECREF(wide);
        Py_DECREF(utf8);
        return raiseTestError("test_widechar",
                              "wide string and utf8 string "
                              "have different length");
    }
    if (PyUnicode_Compare(wide, utf8)) {
        Py_DECREF(wide);
        Py_DECREF(utf8);
        if (PyErr_Occurred())
            return NULL;
        return raiseTestError("test_widechar",
                              "wide string and utf8 string "
                              "are different");
    }

    Py_DECREF(wide);
    Py_DECREF(utf8);
    Py_RETURN_NONE;
}

static PyObject *
unicode_aswidechar(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    Py_ssize_t buflen, size;
    wchar_t *buffer;

    if (!PyArg_ParseTuple(args, "Un", &unicode, &buflen))
        return NULL;
    buffer = PyMem_Malloc(buflen * sizeof(wchar_t));
    if (buffer == NULL)
        return PyErr_NoMemory();

    size = PyUnicode_AsWideChar((PyUnicodeObject*)unicode, buffer, buflen);
    if (size == -1) {
        PyMem_Free(buffer);
        return NULL;
    }

    if (size < buflen)
        buflen = size + 1;
    else
        buflen = size;
    result = PyUnicode_FromWideChar(buffer, buflen);
    PyMem_Free(buffer);
    if (result == NULL)
        return NULL;

    return Py_BuildValue("(Nn)", result, size);
}

static PyObject *
unicode_aswidecharstring(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    Py_ssize_t size;
    wchar_t *buffer;

    if (!PyArg_ParseTuple(args, "U", &unicode))
        return NULL;

    buffer = PyUnicode_AsWideCharString(unicode, &size);
    if (buffer == NULL)
        return NULL;

    result = PyUnicode_FromWideChar(buffer, size + 1);
    PyMem_Free(buffer);
    if (result == NULL)
        return NULL;
    return Py_BuildValue("(Nn)", result, size);
}

static PyObject *
getargs_w_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *result;
    char *str;

    if (!PyArg_ParseTuple(args, "w*:getargs_w_star", &buffer))
        return NULL;

    if (2 <= buffer.len) {
        str = buffer.buf;
        str[0] = '[';
        str[buffer.len-1] = ']';
    }

    result = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    PyBuffer_Release(&buffer);
    return result;
}


static PyObject *
test_empty_argparse(PyObject *self)
{
    /* Test that formats can begin with '|'. See issue #4720. */
    PyObject *tuple, *dict = NULL;
    static char *kwlist[] = {NULL};
    int result;
    tuple = PyTuple_New(0);
    if (!tuple)
        return NULL;
    if ((result = PyArg_ParseTuple(tuple, "|:test_empty_argparse")) < 0)
        goto done;
    dict = PyDict_New();
    if (!dict)
        goto done;
    result = PyArg_ParseTupleAndKeywords(tuple, dict, "|:test_empty_argparse", kwlist);
  done:
    Py_DECREF(tuple);
    Py_XDECREF(dict);
    if (result < 0)
        return NULL;
    else {
        Py_RETURN_NONE;
    }
}

static PyObject *
codec_incrementalencoder(PyObject *self, PyObject *args)
{
    const char *encoding, *errors = NULL;
    if (!PyArg_ParseTuple(args, "s|s:test_incrementalencoder",
                          &encoding, &errors))
        return NULL;
    return PyCodec_IncrementalEncoder(encoding, errors);
}

static PyObject *
codec_incrementaldecoder(PyObject *self, PyObject *args)
{
    const char *encoding, *errors = NULL;
    if (!PyArg_ParseTuple(args, "s|s:test_incrementaldecoder",
                          &encoding, &errors))
        return NULL;
    return PyCodec_IncrementalDecoder(encoding, errors);
}


/* Simple test of _PyLong_NumBits and _PyLong_Sign. */
static PyObject *
test_long_numbits(PyObject *self)
{
    struct triple {
        long input;
        size_t nbits;
        int sign;
    } testcases[] = {{0, 0, 0},
                     {1L, 1, 1},
                     {-1L, 1, -1},
                     {2L, 2, 1},
                     {-2L, 2, -1},
                     {3L, 2, 1},
                     {-3L, 2, -1},
                     {4L, 3, 1},
                     {-4L, 3, -1},
                     {0x7fffL, 15, 1},          /* one Python long digit */
             {-0x7fffL, 15, -1},
             {0xffffL, 16, 1},
             {-0xffffL, 16, -1},
             {0xfffffffL, 28, 1},
             {-0xfffffffL, 28, -1}};
    int i;

    for (i = 0; i < sizeof(testcases) / sizeof(struct triple); ++i) {
        PyObject *plong = PyLong_FromLong(testcases[i].input);
        size_t nbits = _PyLong_NumBits(plong);
        int sign = _PyLong_Sign(plong);

        Py_DECREF(plong);
        if (nbits != testcases[i].nbits)
            return raiseTestError("test_long_numbits",
                            "wrong result for _PyLong_NumBits");
        if (sign != testcases[i].sign)
            return raiseTestError("test_long_numbits",
                            "wrong result for _PyLong_Sign");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

/* Example passing NULLs to PyObject_Str(NULL). */

static PyObject *
test_null_strings(PyObject *self)
{
    PyObject *o1 = PyObject_Str(NULL), *o2 = PyObject_Str(NULL);
    PyObject *tuple = PyTuple_Pack(2, o1, o2);
    Py_XDECREF(o1);
    Py_XDECREF(o2);
    return tuple;
}

static PyObject *
raise_exception(PyObject *self, PyObject *args)
{
    PyObject *exc;
    PyObject *exc_args, *v;
    int num_args, i;

    if (!PyArg_ParseTuple(args, "Oi:raise_exception",
                          &exc, &num_args))
        return NULL;

    exc_args = PyTuple_New(num_args);
    if (exc_args == NULL)
        return NULL;
    for (i = 0; i < num_args; ++i) {
        v = PyLong_FromLong(i);
        if (v == NULL) {
            Py_DECREF(exc_args);
            return NULL;
        }
        PyTuple_SET_ITEM(exc_args, i, v);
    }
    PyErr_SetObject(exc, exc_args);
    Py_DECREF(exc_args);
    return NULL;
}


static int test_run_counter = 0;

static PyObject *
test_datetime_capi(PyObject *self, PyObject *args) {
    if (PyDateTimeAPI) {
        if (test_run_counter) {
            /* Probably regrtest.py -R */
            Py_RETURN_NONE;
        }
        else {
            PyErr_SetString(PyExc_AssertionError,
                            "PyDateTime_CAPI somehow initialized");
            return NULL;
        }
    }
    test_run_counter++;
    PyDateTime_IMPORT;
    if (PyDateTimeAPI)
        Py_RETURN_NONE;
    else
        return NULL;
}


#ifdef WITH_THREAD

/* test_thread_state spawns a thread of its own, and that thread releases
 * `thread_done` when it's finished.  The driver code has to know when the
 * thread finishes, because the thread uses a PyObject (the callable) that
 * may go away when the driver finishes.  The former lack of this explicit
 * synchronization caused rare segfaults, so rare that they were seen only
 * on a Mac buildbot (although they were possible on any box).
 */
static PyThread_type_lock thread_done = NULL;

static int
_make_call(void *callable)
{
    PyObject *rc;
    int success;
    PyGILState_STATE s = PyGILState_Ensure();
    rc = PyObject_CallFunction((PyObject *)callable, "");
    success = (rc != NULL);
    Py_XDECREF(rc);
    PyGILState_Release(s);
    return success;
}

/* Same thing, but releases `thread_done` when it returns.  This variant
 * should be called only from threads spawned by test_thread_state().
 */
static void
_make_call_from_thread(void *callable)
{
    _make_call(callable);
    PyThread_release_lock(thread_done);
}

static PyObject *
test_thread_state(PyObject *self, PyObject *args)
{
    PyObject *fn;
    int success = 1;

    if (!PyArg_ParseTuple(args, "O:test_thread_state", &fn))
        return NULL;

    if (!PyCallable_Check(fn)) {
        PyErr_Format(PyExc_TypeError, "'%s' object is not callable",
            fn->ob_type->tp_name);
        return NULL;
    }

    /* Ensure Python is set up for threading */
    PyEval_InitThreads();
    thread_done = PyThread_allocate_lock();
    if (thread_done == NULL)
        return PyErr_NoMemory();
    PyThread_acquire_lock(thread_done, 1);

    /* Start a new thread with our callback. */
    PyThread_start_new_thread(_make_call_from_thread, fn);
    /* Make the callback with the thread lock held by this thread */
    success &= _make_call(fn);
    /* Do it all again, but this time with the thread-lock released */
    Py_BEGIN_ALLOW_THREADS
    success &= _make_call(fn);
    PyThread_acquire_lock(thread_done, 1);  /* wait for thread to finish */
    Py_END_ALLOW_THREADS

    /* And once more with and without a thread
       XXX - should use a lock and work out exactly what we are trying
       to test <wink>
    */
    Py_BEGIN_ALLOW_THREADS
    PyThread_start_new_thread(_make_call_from_thread, fn);
    success &= _make_call(fn);
    PyThread_acquire_lock(thread_done, 1);  /* wait for thread to finish */
    Py_END_ALLOW_THREADS

    /* Release lock we acquired above.  This is required on HP-UX. */
    PyThread_release_lock(thread_done);

    PyThread_free_lock(thread_done);
    if (!success)
        return NULL;
    Py_RETURN_NONE;
}

/* test Py_AddPendingCalls using threads */
static int _pending_callback(void *arg)
{
    /* we assume the argument is callable object to which we own a reference */
    PyObject *callable = (PyObject *)arg;
    PyObject *r = PyObject_CallObject(callable, NULL);
    Py_DECREF(callable);
    Py_XDECREF(r);
    return r != NULL ? 0 : -1;
}

/* The following requests n callbacks to _pending_callback.  It can be
 * run from any python thread.
 */
PyObject *pending_threadfunc(PyObject *self, PyObject *arg)
{
    PyObject *callable;
    int r;
    if (PyArg_ParseTuple(arg, "O", &callable) == 0)
        return NULL;

    /* create the reference for the callbackwhile we hold the lock */
    Py_INCREF(callable);

    Py_BEGIN_ALLOW_THREADS
    r = Py_AddPendingCall(&_pending_callback, callable);
    Py_END_ALLOW_THREADS

    if (r<0) {
        Py_DECREF(callable); /* unsuccessful add, destroy the extra reference */
        Py_INCREF(Py_False);
        return Py_False;
    }
    Py_INCREF(Py_True);
    return Py_True;
}
#endif

/* Some tests of PyUnicode_FromFormat().  This needs more tests. */
static PyObject *
test_string_from_format(PyObject *self, PyObject *args)
{
    PyObject *result;
    char *msg;

#define CHECK_1_FORMAT(FORMAT, TYPE)                    \
    result = PyUnicode_FromFormat(FORMAT, (TYPE)1);     \
    if (result == NULL)                                 \
        return NULL;                                    \
    if (strcmp(_PyUnicode_AsString(result), "1")) {     \
        msg = FORMAT " failed at 1";                    \
        goto Fail;                                      \
    }                                                   \
    Py_DECREF(result)

    CHECK_1_FORMAT("%d", int);
    CHECK_1_FORMAT("%ld", long);
    /* The z width modifier was added in Python 2.5. */
    CHECK_1_FORMAT("%zd", Py_ssize_t);

    /* The u type code was added in Python 2.5. */
    CHECK_1_FORMAT("%u", unsigned int);
    CHECK_1_FORMAT("%lu", unsigned long);
    CHECK_1_FORMAT("%zu", size_t);

    /* "%lld" and "%llu" support added in Python 2.7. */
#ifdef HAVE_LONG_LONG
    CHECK_1_FORMAT("%llu", unsigned PY_LONG_LONG);
    CHECK_1_FORMAT("%lld", PY_LONG_LONG);
#endif

    Py_RETURN_NONE;

 Fail:
    Py_XDECREF(result);
    return raiseTestError("test_string_from_format", msg);

#undef CHECK_1_FORMAT
}


static PyObject *
test_unicode_compare_with_ascii(PyObject *self) {
    PyObject *py_s = PyUnicode_FromStringAndSize("str\0", 4);
    int result;
    if (py_s == NULL)
        return NULL;
    result = PyUnicode_CompareWithASCIIString(py_s, "str");
    Py_DECREF(py_s);
    if (!result) {
        PyErr_SetString(TestError, "Python string ending in NULL "
                        "should not compare equal to c string.");
        return NULL;
    }
    Py_RETURN_NONE;
};

/* This is here to provide a docstring for test_descr. */
static PyObject *
test_with_docstring(PyObject *self)
{
    Py_RETURN_NONE;
}

/* Test PyOS_string_to_double. */
static PyObject *
test_string_to_double(PyObject *self) {
    double result;
    char *msg;

#define CHECK_STRING(STR, expected)                             \
    result = PyOS_string_to_double(STR, NULL, NULL);            \
    if (result == -1.0 && PyErr_Occurred())                     \
        return NULL;                                            \
    if (result != expected) {                                   \
        msg = "conversion of " STR " to float failed";          \
        goto fail;                                              \
    }

#define CHECK_INVALID(STR)                                              \
    result = PyOS_string_to_double(STR, NULL, NULL);                    \
    if (result == -1.0 && PyErr_Occurred()) {                           \
        if (PyErr_ExceptionMatches(PyExc_ValueError))                   \
            PyErr_Clear();                                              \
        else                                                            \
            return NULL;                                                \
    }                                                                   \
    else {                                                              \
        msg = "conversion of " STR " didn't raise ValueError";          \
        goto fail;                                                      \
    }

    CHECK_STRING("0.1", 0.1);
    CHECK_STRING("1.234", 1.234);
    CHECK_STRING("-1.35", -1.35);
    CHECK_STRING(".1e01", 1.0);
    CHECK_STRING("2.e-2", 0.02);

    CHECK_INVALID(" 0.1");
    CHECK_INVALID("\t\n-3");
    CHECK_INVALID(".123 ");
    CHECK_INVALID("3\n");
    CHECK_INVALID("123abc");

    Py_RETURN_NONE;
  fail:
    return raiseTestError("test_string_to_double", msg);
#undef CHECK_STRING
#undef CHECK_INVALID
}


/* Coverage testing of capsule objects. */

static const char *capsule_name = "capsule name";
static       char *capsule_pointer = "capsule pointer";
static       char *capsule_context = "capsule context";
static const char *capsule_error = NULL;
static int
capsule_destructor_call_count = 0;

static void
capsule_destructor(PyObject *o) {
    capsule_destructor_call_count++;
    if (PyCapsule_GetContext(o) != capsule_context) {
        capsule_error = "context did not match in destructor!";
    } else if (PyCapsule_GetDestructor(o) != capsule_destructor) {
        capsule_error = "destructor did not match in destructor!  (woah!)";
    } else if (PyCapsule_GetName(o) != capsule_name) {
        capsule_error = "name did not match in destructor!";
    } else if (PyCapsule_GetPointer(o, capsule_name) != capsule_pointer) {
        capsule_error = "pointer did not match in destructor!";
    }
}

typedef struct {
    char *name;
    char *module;
    char *attribute;
} known_capsule;

static PyObject *
test_capsule(PyObject *self, PyObject *args)
{
    PyObject *object;
    const char *error = NULL;
    void *pointer;
    void *pointer2;
    known_capsule known_capsules[] = {
        #define KNOWN_CAPSULE(module, name)             { module "." name, module, name }
        KNOWN_CAPSULE("_socket", "CAPI"),
        KNOWN_CAPSULE("_curses", "_C_API"),
        KNOWN_CAPSULE("datetime", "datetime_CAPI"),
        { NULL, NULL },
    };
    known_capsule *known = &known_capsules[0];

#define FAIL(x) { error = (x); goto exit; }

#define CHECK_DESTRUCTOR \
    if (capsule_error) { \
        FAIL(capsule_error); \
    } \
    else if (!capsule_destructor_call_count) {          \
        FAIL("destructor not called!"); \
    } \
    capsule_destructor_call_count = 0; \

    object = PyCapsule_New(capsule_pointer, capsule_name, capsule_destructor);
    PyCapsule_SetContext(object, capsule_context);
    capsule_destructor(object);
    CHECK_DESTRUCTOR;
    Py_DECREF(object);
    CHECK_DESTRUCTOR;

    object = PyCapsule_New(known, "ignored", NULL);
    PyCapsule_SetPointer(object, capsule_pointer);
    PyCapsule_SetName(object, capsule_name);
    PyCapsule_SetDestructor(object, capsule_destructor);
    PyCapsule_SetContext(object, capsule_context);
    capsule_destructor(object);
    CHECK_DESTRUCTOR;
    /* intentionally access using the wrong name */
    pointer2 = PyCapsule_GetPointer(object, "the wrong name");
    if (!PyErr_Occurred()) {
        FAIL("PyCapsule_GetPointer should have failed but did not!");
    }
    PyErr_Clear();
    if (pointer2) {
        if (pointer2 == capsule_pointer) {
            FAIL("PyCapsule_GetPointer should not have"
                     " returned the internal pointer!");
        } else {
            FAIL("PyCapsule_GetPointer should have "
                     "returned NULL pointer but did not!");
        }
    }
    PyCapsule_SetDestructor(object, NULL);
    Py_DECREF(object);
    if (capsule_destructor_call_count) {
        FAIL("destructor called when it should not have been!");
    }

    for (known = &known_capsules[0]; known->module != NULL; known++) {
        /* yeah, ordinarily I wouldn't do this either,
           but it's fine for this test harness.
        */
        static char buffer[256];
#undef FAIL
#define FAIL(x) \
        { \
        sprintf(buffer, "%s module: \"%s\" attribute: \"%s\"", \
            x, known->module, known->attribute); \
        error = buffer; \
        goto exit; \
        } \

        PyObject *module = PyImport_ImportModule(known->module);
        if (module) {
            pointer = PyCapsule_Import(known->name, 0);
            if (!pointer) {
                Py_DECREF(module);
                FAIL("PyCapsule_GetPointer returned NULL unexpectedly!");
            }
            object = PyObject_GetAttrString(module, known->attribute);
            if (!object) {
                Py_DECREF(module);
                return NULL;
            }
            pointer2 = PyCapsule_GetPointer(object,
                                    "weebles wobble but they don't fall down");
            if (!PyErr_Occurred()) {
                Py_DECREF(object);
                Py_DECREF(module);
                FAIL("PyCapsule_GetPointer should have failed but did not!");
            }
            PyErr_Clear();
            if (pointer2) {
                Py_DECREF(module);
                Py_DECREF(object);
                if (pointer2 == pointer) {
                    FAIL("PyCapsule_GetPointer should not have"
                             " returned its internal pointer!");
                } else {
                    FAIL("PyCapsule_GetPointer should have"
                             " returned NULL pointer but did not!");
                }
            }
            Py_DECREF(object);
            Py_DECREF(module);
        }
        else
            PyErr_Clear();
    }

  exit:
    if (error) {
        return raiseTestError("test_capsule", error);
    }
    Py_RETURN_NONE;
#undef FAIL
}

#ifdef HAVE_GETTIMEOFDAY
/* Profiling of integer performance */
static void print_delta(int test, struct timeval *s, struct timeval *e)
{
    e->tv_sec -= s->tv_sec;
    e->tv_usec -= s->tv_usec;
    if (e->tv_usec < 0) {
        e->tv_sec -=1;
        e->tv_usec += 1000000;
    }
    printf("Test %d: %d.%06ds\n", test, (int)e->tv_sec, (int)e->tv_usec);
}

static PyObject *
profile_int(PyObject *self, PyObject* args)
{
    int i, k;
    struct timeval start, stop;
    PyObject *single, **multiple, *op1, *result;

    /* Test 1: Allocate and immediately deallocate
       many small integers */
    gettimeofday(&start, NULL);
    for(k=0; k < 20000; k++)
        for(i=0; i < 1000; i++) {
            single = PyLong_FromLong(i);
            Py_DECREF(single);
        }
    gettimeofday(&stop, NULL);
    print_delta(1, &start, &stop);

    /* Test 2: Allocate and immediately deallocate
       many large integers */
    gettimeofday(&start, NULL);
    for(k=0; k < 20000; k++)
        for(i=0; i < 1000; i++) {
            single = PyLong_FromLong(i+1000000);
            Py_DECREF(single);
        }
    gettimeofday(&stop, NULL);
    print_delta(2, &start, &stop);

    /* Test 3: Allocate a few integers, then release
       them all simultaneously. */
    multiple = malloc(sizeof(PyObject*) * 1000);
    gettimeofday(&start, NULL);
    for(k=0; k < 20000; k++) {
        for(i=0; i < 1000; i++) {
            multiple[i] = PyLong_FromLong(i+1000000);
        }
        for(i=0; i < 1000; i++) {
            Py_DECREF(multiple[i]);
        }
    }
    gettimeofday(&stop, NULL);
    print_delta(3, &start, &stop);

    /* Test 4: Allocate many integers, then release
       them all simultaneously. */
    multiple = malloc(sizeof(PyObject*) * 1000000);
    gettimeofday(&start, NULL);
    for(k=0; k < 20; k++) {
        for(i=0; i < 1000000; i++) {
            multiple[i] = PyLong_FromLong(i+1000000);
        }
        for(i=0; i < 1000000; i++) {
            Py_DECREF(multiple[i]);
        }
    }
    gettimeofday(&stop, NULL);
    print_delta(4, &start, &stop);

    /* Test 5: Allocate many integers < 32000 */
    multiple = malloc(sizeof(PyObject*) * 1000000);
    gettimeofday(&start, NULL);
    for(k=0; k < 10; k++) {
        for(i=0; i < 1000000; i++) {
            multiple[i] = PyLong_FromLong(i+1000);
        }
        for(i=0; i < 1000000; i++) {
            Py_DECREF(multiple[i]);
        }
    }
    gettimeofday(&stop, NULL);
    print_delta(5, &start, &stop);

    /* Test 6: Perform small int addition */
    op1 = PyLong_FromLong(1);
    gettimeofday(&start, NULL);
    for(i=0; i < 10000000; i++) {
        result = PyNumber_Add(op1, op1);
        Py_DECREF(result);
    }
    gettimeofday(&stop, NULL);
    Py_DECREF(op1);
    print_delta(6, &start, &stop);

    /* Test 7: Perform medium int addition */
    op1 = PyLong_FromLong(1000);
    gettimeofday(&start, NULL);
    for(i=0; i < 10000000; i++) {
        result = PyNumber_Add(op1, op1);
        Py_DECREF(result);
    }
    gettimeofday(&stop, NULL);
    Py_DECREF(op1);
    print_delta(7, &start, &stop);

    Py_INCREF(Py_None);
    return Py_None;
}
#endif

/* To test the format of tracebacks as printed out. */
static PyObject *
traceback_print(PyObject *self, PyObject *args)
{
    PyObject *file;
    PyObject *traceback;
    int result;

    if (!PyArg_ParseTuple(args, "OO:traceback_print",
                            &traceback, &file))
        return NULL;

    result = PyTraceBack_Print(traceback, file);
    if (result < 0)
        return NULL;
    Py_RETURN_NONE;
}

/* To test the format of exceptions as printed out. */
static PyObject *
exception_print(PyObject *self, PyObject *args)
{
    PyObject *value;
    PyObject *tb;

    if (!PyArg_ParseTuple(args, "O:exception_print",
                            &value))
        return NULL;
    if (!PyExceptionInstance_Check(value)) {
        PyErr_Format(PyExc_TypeError, "an exception instance is required");
        return NULL;
    }

    tb = PyException_GetTraceback(value);
    PyErr_Display((PyObject *) Py_TYPE(value), value, tb);
    Py_XDECREF(tb);

    Py_RETURN_NONE;
}




/* reliably raise a MemoryError */
static PyObject *
raise_memoryerror(PyObject *self)
{
    PyErr_NoMemory();
    return NULL;
}

/* Issue 6012 */
static PyObject *str1, *str2;
static int
failing_converter(PyObject *obj, void *arg)
{
    /* Clone str1, then let the conversion fail. */
    assert(str1);
    str2 = str1;
    Py_INCREF(str2);
    return 0;
}
static PyObject*
argparsing(PyObject *o, PyObject *args)
{
    PyObject *res;
    str1 = str2 = NULL;
    if (!PyArg_ParseTuple(args, "O&O&",
                          PyUnicode_FSConverter, &str1,
                          failing_converter, &str2)) {
        if (!str2)
            /* argument converter not called? */
            return NULL;
        /* Should be 1 */
        res = PyLong_FromLong(Py_REFCNT(str2));
        Py_DECREF(str2);
        PyErr_Clear();
        return res;
    }
    Py_RETURN_NONE;
}

/* To test that the result of PyCode_NewEmpty has the right members. */
static PyObject *
code_newempty(PyObject *self, PyObject *args)
{
    const char *filename;
    const char *funcname;
    int firstlineno;

    if (!PyArg_ParseTuple(args, "ssi:code_newempty",
                          &filename, &funcname, &firstlineno))
        return NULL;

    return (PyObject *)PyCode_NewEmpty(filename, funcname, firstlineno);
}

/* Test PyErr_NewExceptionWithDoc (also exercise PyErr_NewException).
   Run via Lib/test/test_exceptions.py */
static PyObject *
make_exception_with_doc(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *name;
    const char *doc = NULL;
    PyObject *base = NULL;
    PyObject *dict = NULL;

    static char *kwlist[] = {"name", "doc", "base", "dict", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                    "s|sOO:make_exception_with_doc", kwlist,
                                     &name, &doc, &base, &dict))
        return NULL;

    return PyErr_NewExceptionWithDoc(name, doc, base, dict);
}

/* Test that the fatal error from not having a current thread doesn't
   cause an infinite loop.  Run via Lib/test/test_capi.py */
static PyObject *
crash_no_current_thread(PyObject *self)
{
    Py_BEGIN_ALLOW_THREADS
    /* Using PyThreadState_Get() directly allows the test to pass in
       !pydebug mode. However, the test only actually tests anything
       in pydebug mode, since that's where the infinite loop was in
       the first place. */
    PyThreadState_Get();
    Py_END_ALLOW_THREADS
    return NULL;
}

static PyObject *
format_unicode(PyObject *self, PyObject *args)
{
    const char *format;
    PyObject *arg;
    if (!PyArg_ParseTuple(args, "yU", &format, &arg))
        return NULL;
    return PyUnicode_FromFormat(format, arg);

}

static PyMethodDef TestMethods[] = {
    {"raise_exception",         raise_exception,                 METH_VARARGS},
    {"raise_memoryerror",   (PyCFunction)raise_memoryerror,  METH_NOARGS},
    {"test_config",             (PyCFunction)test_config,        METH_NOARGS},
    {"test_datetime_capi",  test_datetime_capi,              METH_NOARGS},
    {"test_list_api",           (PyCFunction)test_list_api,      METH_NOARGS},
    {"test_dict_iteration",     (PyCFunction)test_dict_iteration,METH_NOARGS},
    {"test_lazy_hash_inheritance",      (PyCFunction)test_lazy_hash_inheritance,METH_NOARGS},
    {"test_broken_memoryview",          (PyCFunction)test_broken_memoryview,METH_NOARGS},
    {"test_long_api",           (PyCFunction)test_long_api,      METH_NOARGS},
    {"test_long_and_overflow", (PyCFunction)test_long_and_overflow,
     METH_NOARGS},
    {"test_long_numbits",       (PyCFunction)test_long_numbits,  METH_NOARGS},
    {"test_k_code",             (PyCFunction)test_k_code,        METH_NOARGS},
    {"test_empty_argparse", (PyCFunction)test_empty_argparse,METH_NOARGS},
    {"test_bug_7414", (PyCFunction)test_bug_7414, METH_NOARGS},
    {"test_null_strings",       (PyCFunction)test_null_strings,  METH_NOARGS},
    {"test_string_from_format", (PyCFunction)test_string_from_format, METH_NOARGS},
    {"test_with_docstring", (PyCFunction)test_with_docstring, METH_NOARGS,
     PyDoc_STR("This is a pretty normal docstring.")},
    {"test_string_to_double", (PyCFunction)test_string_to_double, METH_NOARGS},
    {"test_unicode_compare_with_ascii", (PyCFunction)test_unicode_compare_with_ascii, METH_NOARGS},
    {"test_capsule", (PyCFunction)test_capsule, METH_NOARGS},
    {"getargs_tuple",           getargs_tuple,                   METH_VARARGS},
    {"getargs_keywords", (PyCFunction)getargs_keywords,
      METH_VARARGS|METH_KEYWORDS},
    {"getargs_b",               getargs_b,                       METH_VARARGS},
    {"getargs_B",               getargs_B,                       METH_VARARGS},
    {"getargs_h",               getargs_h,                       METH_VARARGS},
    {"getargs_H",               getargs_H,                       METH_VARARGS},
    {"getargs_I",               getargs_I,                       METH_VARARGS},
    {"getargs_k",               getargs_k,                       METH_VARARGS},
    {"getargs_i",               getargs_i,                       METH_VARARGS},
    {"getargs_l",               getargs_l,                       METH_VARARGS},
    {"getargs_n",               getargs_n,                       METH_VARARGS},
#ifdef HAVE_LONG_LONG
    {"getargs_L",               getargs_L,                       METH_VARARGS},
    {"getargs_K",               getargs_K,                       METH_VARARGS},
    {"test_longlong_api",       test_longlong_api,               METH_NOARGS},
    {"test_long_long_and_overflow",
        (PyCFunction)test_long_long_and_overflow, METH_NOARGS},
    {"test_L_code",             (PyCFunction)test_L_code,        METH_NOARGS},
#endif
    {"getargs_s",               getargs_s,                       METH_VARARGS},
    {"getargs_s_star",          getargs_s_star,                  METH_VARARGS},
    {"getargs_s_hash",          getargs_s_hash,                  METH_VARARGS},
    {"getargs_z",               getargs_z,                       METH_VARARGS},
    {"getargs_z_star",          getargs_z_star,                  METH_VARARGS},
    {"getargs_z_hash",          getargs_z_hash,                  METH_VARARGS},
    {"getargs_y",               getargs_y,                       METH_VARARGS},
    {"getargs_y_star",          getargs_y_star,                  METH_VARARGS},
    {"getargs_y_hash",          getargs_y_hash,                  METH_VARARGS},
    {"getargs_u",               getargs_u,                       METH_VARARGS},
    {"getargs_u_hash",          getargs_u_hash,                  METH_VARARGS},
    {"getargs_Z",               getargs_Z,                       METH_VARARGS},
    {"getargs_Z_hash",          getargs_Z_hash,                  METH_VARARGS},
    {"getargs_w_star",          getargs_w_star,                  METH_VARARGS},
    {"codec_incrementalencoder",
     (PyCFunction)codec_incrementalencoder,                      METH_VARARGS},
    {"codec_incrementaldecoder",
     (PyCFunction)codec_incrementaldecoder,                      METH_VARARGS},
    {"test_s_code",             (PyCFunction)test_s_code,        METH_NOARGS},
    {"test_u_code",             (PyCFunction)test_u_code,        METH_NOARGS},
    {"test_Z_code",             (PyCFunction)test_Z_code,        METH_NOARGS},
    {"test_widechar",           (PyCFunction)test_widechar,      METH_NOARGS},
    {"unicode_aswidechar",      unicode_aswidechar,                 METH_VARARGS},
    {"unicode_aswidecharstring",unicode_aswidecharstring,           METH_VARARGS},
#ifdef WITH_THREAD
    {"_test_thread_state",      test_thread_state,               METH_VARARGS},
    {"_pending_threadfunc",     pending_threadfunc,              METH_VARARGS},
#endif
#ifdef HAVE_GETTIMEOFDAY
    {"profile_int",             profile_int,                     METH_NOARGS},
#endif
    {"traceback_print",         traceback_print,                 METH_VARARGS},
    {"exception_print",         exception_print,                 METH_VARARGS},
    {"argparsing",              argparsing,                      METH_VARARGS},
    {"code_newempty",           code_newempty,                   METH_VARARGS},
    {"make_exception_with_doc", (PyCFunction)make_exception_with_doc,
     METH_VARARGS | METH_KEYWORDS},
    {"crash_no_current_thread", (PyCFunction)crash_no_current_thread, METH_NOARGS},
    {"format_unicode",          format_unicode,                 METH_VARARGS},
    {NULL, NULL} /* sentinel */
};

#define AddSym(d, n, f, v) {PyObject *o = f(v); PyDict_SetItemString(d, n, o); Py_DECREF(o);}

typedef struct {
    char bool_member;
    char byte_member;
    unsigned char ubyte_member;
    short short_member;
    unsigned short ushort_member;
    int int_member;
    unsigned int uint_member;
    long long_member;
    unsigned long ulong_member;
    Py_ssize_t pyssizet_member;
    float float_member;
    double double_member;
    char inplace_member[6];
#ifdef HAVE_LONG_LONG
    PY_LONG_LONG longlong_member;
    unsigned PY_LONG_LONG ulonglong_member;
#endif
} all_structmembers;

typedef struct {
    PyObject_HEAD
    all_structmembers structmembers;
} test_structmembers;

static struct PyMemberDef test_members[] = {
    {"T_BOOL", T_BOOL, offsetof(test_structmembers, structmembers.bool_member), 0, NULL},
    {"T_BYTE", T_BYTE, offsetof(test_structmembers, structmembers.byte_member), 0, NULL},
    {"T_UBYTE", T_UBYTE, offsetof(test_structmembers, structmembers.ubyte_member), 0, NULL},
    {"T_SHORT", T_SHORT, offsetof(test_structmembers, structmembers.short_member), 0, NULL},
    {"T_USHORT", T_USHORT, offsetof(test_structmembers, structmembers.ushort_member), 0, NULL},
    {"T_INT", T_INT, offsetof(test_structmembers, structmembers.int_member), 0, NULL},
    {"T_UINT", T_UINT, offsetof(test_structmembers, structmembers.uint_member), 0, NULL},
    {"T_LONG", T_LONG, offsetof(test_structmembers, structmembers.long_member), 0, NULL},
    {"T_ULONG", T_ULONG, offsetof(test_structmembers, structmembers.ulong_member), 0, NULL},
    {"T_PYSSIZET", T_PYSSIZET, offsetof(test_structmembers, structmembers.pyssizet_member), 0, NULL},
    {"T_FLOAT", T_FLOAT, offsetof(test_structmembers, structmembers.float_member), 0, NULL},
    {"T_DOUBLE", T_DOUBLE, offsetof(test_structmembers, structmembers.double_member), 0, NULL},
    {"T_STRING_INPLACE", T_STRING_INPLACE, offsetof(test_structmembers, structmembers.inplace_member), 0, NULL},
#ifdef HAVE_LONG_LONG
    {"T_LONGLONG", T_LONGLONG, offsetof(test_structmembers, structmembers.longlong_member), 0, NULL},
    {"T_ULONGLONG", T_ULONGLONG, offsetof(test_structmembers, structmembers.ulonglong_member), 0, NULL},
#endif
    {NULL}
};


static PyObject *
test_structmembers_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {
        "T_BOOL", "T_BYTE", "T_UBYTE", "T_SHORT", "T_USHORT",
        "T_INT", "T_UINT", "T_LONG", "T_ULONG", "T_PYSSIZET",
        "T_FLOAT", "T_DOUBLE", "T_STRING_INPLACE",
#ifdef HAVE_LONG_LONG
        "T_LONGLONG", "T_ULONGLONG",
#endif
        NULL};
    static char *fmt = "|bbBhHiIlknfds#"
#ifdef HAVE_LONG_LONG
        "LK"
#endif
        ;
    test_structmembers *ob;
    const char *s = NULL;
    Py_ssize_t string_len = 0;
    ob = PyObject_New(test_structmembers, type);
    if (ob == NULL)
        return NULL;
    memset(&ob->structmembers, 0, sizeof(all_structmembers));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, fmt, keywords,
                                     &ob->structmembers.bool_member,
                                     &ob->structmembers.byte_member,
                                     &ob->structmembers.ubyte_member,
                                     &ob->structmembers.short_member,
                                     &ob->structmembers.ushort_member,
                                     &ob->structmembers.int_member,
                                     &ob->structmembers.uint_member,
                                     &ob->structmembers.long_member,
                                     &ob->structmembers.ulong_member,
                                     &ob->structmembers.pyssizet_member,
                                     &ob->structmembers.float_member,
                                     &ob->structmembers.double_member,
                                     &s, &string_len
#ifdef HAVE_LONG_LONG
                                     , &ob->structmembers.longlong_member,
                                     &ob->structmembers.ulonglong_member
#endif
        )) {
        Py_DECREF(ob);
        return NULL;
    }
    if (s != NULL) {
        if (string_len > 5) {
            Py_DECREF(ob);
            PyErr_SetString(PyExc_ValueError, "string too long");
            return NULL;
        }
        strcpy(ob->structmembers.inplace_member, s);
    }
    else {
        strcpy(ob->structmembers.inplace_member, "");
    }
    return (PyObject *)ob;
}

static void
test_structmembers_free(PyObject *ob)
{
    PyObject_FREE(ob);
}

static PyTypeObject test_structmembersType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "test_structmembersType",
    sizeof(test_structmembers),         /* tp_basicsize */
    0,                                  /* tp_itemsize */
    test_structmembers_free,            /* destructor tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    0,                                  /* tp_flags */
    "Type containing all structmember types",
    0,                                  /* traverseproc tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    test_members,                       /* tp_members */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    test_structmembers_new,             /* tp_new */
};



static struct PyModuleDef _testcapimodule = {
    PyModuleDef_HEAD_INIT,
    "_testcapi",
    NULL,
    -1,
    TestMethods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__testcapi(void)
{
    PyObject *m;

    m = PyModule_Create(&_testcapimodule);
    if (m == NULL)
        return NULL;

    Py_TYPE(&_HashInheritanceTester_Type)=&PyType_Type;
    Py_TYPE(&_MemoryViewTester_Type)=&PyType_Type;

    Py_TYPE(&test_structmembersType)=&PyType_Type;
    Py_INCREF(&test_structmembersType);
    /* don't use a name starting with "test", since we don't want
       test_capi to automatically call this */
    PyModule_AddObject(m, "_test_structmembersType", (PyObject *)&test_structmembersType);

    PyModule_AddObject(m, "CHAR_MAX", PyLong_FromLong(CHAR_MAX));
    PyModule_AddObject(m, "CHAR_MIN", PyLong_FromLong(CHAR_MIN));
    PyModule_AddObject(m, "UCHAR_MAX", PyLong_FromLong(UCHAR_MAX));
    PyModule_AddObject(m, "SHRT_MAX", PyLong_FromLong(SHRT_MAX));
    PyModule_AddObject(m, "SHRT_MIN", PyLong_FromLong(SHRT_MIN));
    PyModule_AddObject(m, "USHRT_MAX", PyLong_FromLong(USHRT_MAX));
    PyModule_AddObject(m, "INT_MAX",  PyLong_FromLong(INT_MAX));
    PyModule_AddObject(m, "INT_MIN",  PyLong_FromLong(INT_MIN));
    PyModule_AddObject(m, "UINT_MAX",  PyLong_FromUnsignedLong(UINT_MAX));
    PyModule_AddObject(m, "LONG_MAX", PyLong_FromLong(LONG_MAX));
    PyModule_AddObject(m, "LONG_MIN", PyLong_FromLong(LONG_MIN));
    PyModule_AddObject(m, "ULONG_MAX", PyLong_FromUnsignedLong(ULONG_MAX));
    PyModule_AddObject(m, "FLT_MAX", PyFloat_FromDouble(FLT_MAX));
    PyModule_AddObject(m, "FLT_MIN", PyFloat_FromDouble(FLT_MIN));
    PyModule_AddObject(m, "DBL_MAX", PyFloat_FromDouble(DBL_MAX));
    PyModule_AddObject(m, "DBL_MIN", PyFloat_FromDouble(DBL_MIN));
    PyModule_AddObject(m, "LLONG_MAX", PyLong_FromLongLong(PY_LLONG_MAX));
    PyModule_AddObject(m, "LLONG_MIN", PyLong_FromLongLong(PY_LLONG_MIN));
    PyModule_AddObject(m, "ULLONG_MAX", PyLong_FromUnsignedLongLong(PY_ULLONG_MAX));
    PyModule_AddObject(m, "PY_SSIZE_T_MAX", PyLong_FromSsize_t(PY_SSIZE_T_MAX));
    PyModule_AddObject(m, "PY_SSIZE_T_MIN", PyLong_FromSsize_t(PY_SSIZE_T_MIN));
    PyModule_AddObject(m, "SIZEOF_PYGC_HEAD", PyLong_FromSsize_t(sizeof(PyGC_Head)));
    Py_INCREF(&PyInstanceMethod_Type);
    PyModule_AddObject(m, "instancemethod", (PyObject *)&PyInstanceMethod_Type);

    TestError = PyErr_NewException("_testcapi.error", NULL, NULL);
    Py_INCREF(TestError);
    PyModule_AddObject(m, "error", TestError);
    return m;
}
