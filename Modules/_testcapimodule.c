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
#include "marshal.h"
#include <signal.h>

#ifdef MS_WINDOWS
#  include <winsock2.h>         /* struct timeval */
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>           /* For W_STOPCODE */
#endif

#include "pythread.h"
static PyObject *TestError;     /* set to exception object in init */

/* Raise TestError with test_name + ": " + msg, and return NULL. */

static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
    PyErr_Format(TestError, "%s: %s", test_name, msg);
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
    PyErr_Format(TestError,
        "%s #define == %d but sizeof(%s) == %d",
        fatname, expected, typname, got);
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
    CHECK_SIZEOF(SIZEOF_LONG_LONG, long long);

#undef CHECK_SIZEOF

    Py_RETURN_NONE;
}

static PyObject*
test_sizeof_c_types(PyObject *self)
{
#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5)))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#define CHECK_SIZEOF(TYPE, EXPECTED)         \
    if (EXPECTED != sizeof(TYPE))  {         \
        PyErr_Format(TestError,              \
            "sizeof(%s) = %u instead of %u", \
            #TYPE, sizeof(TYPE), EXPECTED);  \
        return (PyObject*)NULL;              \
    }
#define IS_SIGNED(TYPE) (((TYPE)-1) < (TYPE)0)
#define CHECK_SIGNNESS(TYPE, SIGNED)         \
    if (IS_SIGNED(TYPE) != SIGNED) {         \
        PyErr_Format(TestError,              \
            "%s signness is, instead of %i",  \
            #TYPE, IS_SIGNED(TYPE), SIGNED); \
        return (PyObject*)NULL;              \
    }

    /* integer types */
    CHECK_SIZEOF(Py_UCS1, 1);
    CHECK_SIZEOF(Py_UCS2, 2);
    CHECK_SIZEOF(Py_UCS4, 4);
    CHECK_SIGNNESS(Py_UCS1, 0);
    CHECK_SIGNNESS(Py_UCS2, 0);
    CHECK_SIGNNESS(Py_UCS4, 0);
    CHECK_SIZEOF(int32_t, 4);
    CHECK_SIGNNESS(int32_t, 1);
    CHECK_SIZEOF(uint32_t, 4);
    CHECK_SIGNNESS(uint32_t, 0);
    CHECK_SIZEOF(int64_t, 8);
    CHECK_SIGNNESS(int64_t, 1);
    CHECK_SIZEOF(uint64_t, 8);
    CHECK_SIGNNESS(uint64_t, 0);

    /* pointer/size types */
    CHECK_SIZEOF(size_t, sizeof(void *));
    CHECK_SIGNNESS(size_t, 0);
    CHECK_SIZEOF(Py_ssize_t, sizeof(void *));
    CHECK_SIGNNESS(Py_ssize_t, 1);

    CHECK_SIZEOF(uintptr_t, sizeof(void *));
    CHECK_SIGNNESS(uintptr_t, 0);
    CHECK_SIZEOF(intptr_t, sizeof(void *));
    CHECK_SIGNNESS(intptr_t, 1);

    Py_RETURN_NONE;

#undef IS_SIGNED
#undef CHECK_SIGNESS
#undef CHECK_SIZEOF
#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5)))
#pragma GCC diagnostic pop
#endif
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

    Py_RETURN_NONE;
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
        if (v == NULL) {
            return -1;
        }
        if (PyDict_SetItem(dict, v, v) < 0) {
            Py_DECREF(v);
            return -1;
        }
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

    Py_RETURN_NONE;
}

static PyObject*
dict_getitem_knownhash(PyObject *self, PyObject *args)
{
    PyObject *mp, *key, *result;
    Py_ssize_t hash;

    if (!PyArg_ParseTuple(args, "OOn:dict_getitem_knownhash",
                          &mp, &key, &hash)) {
        return NULL;
    }

    result = _PyDict_GetItem_KnownHash(mp, key, (Py_hash_t)hash);
    if (result == NULL && !PyErr_Occurred()) {
        _PyErr_SetKeyError(key);
        return NULL;
    }

    Py_XINCREF(result);
    return result;
}

static PyObject*
dict_hassplittable(PyObject *self, PyObject *arg)
{
    if (!PyDict_Check(arg)) {
        PyErr_Format(PyExc_TypeError,
                     "dict_hassplittable() argument must be dict, not '%s'",
                     arg->ob_type->tp_name);
        return NULL;
    }

    return PyBool_FromLong(_PyDict_HasSplitTable((PyDictObject*)arg));
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


/* Tests of PyLong_{As, From}{Unsigned,}Long(), and
   PyLong_{As, From}{Unsigned,}LongLong().

   Note that the meat of the test is contained in testcapi_long.h.
   This is revolting, but delicate code duplication is worse:  "almost
   exactly the same" code is needed to test long long, but the ubiquitous
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

static PyObject *
raise_test_longlong_error(const char* msg)
{
    return raiseTestError("test_longlong_api", msg);
}

#define TESTNAME        test_longlong_api_inner
#define TYPENAME        long long
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

    Py_RETURN_NONE;
}

/* Test the PyLong_AsLongLongAndOverflow API. General conversion to
   long long is tested by test_long_api_inner. This test will
   concentrate on proper handling of overflow.
*/

static PyObject *
test_long_long_and_overflow(PyObject *self)
{
    PyObject *num, *one, *temp;
    long long value;
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

    Py_RETURN_NONE;
}

/* Test the PyLong_As{Size,Ssize}_t API. At present this just tests that
   non-integer arguments are handled correctly. It should be extended to
   test overflow handling.
 */

static PyObject *
test_long_as_size_t(PyObject *self)
{
    size_t out_u;
    Py_ssize_t out_s;

    Py_INCREF(Py_None);

    out_u = PyLong_AsSize_t(Py_None);
    if (out_u != (size_t)-1 || !PyErr_Occurred())
        return raiseTestError("test_long_as_size_t",
                              "PyLong_AsSize_t(None) didn't complain");
    if (!PyErr_ExceptionMatches(PyExc_TypeError))
        return raiseTestError("test_long_as_size_t",
                              "PyLong_AsSize_t(None) raised "
                              "something other than TypeError");
    PyErr_Clear();

    out_s = PyLong_AsSsize_t(Py_None);
    if (out_s != (Py_ssize_t)-1 || !PyErr_Occurred())
        return raiseTestError("test_long_as_size_t",
                              "PyLong_AsSsize_t(None) didn't complain");
    if (!PyErr_ExceptionMatches(PyExc_TypeError))
        return raiseTestError("test_long_as_size_t",
                              "PyLong_AsSsize_t(None) raised "
                              "something other than TypeError");
    PyErr_Clear();

    /* Py_INCREF(Py_None) omitted - we already have a reference to it. */
    return Py_None;
}

/* Test the PyLong_AsDouble API. At present this just tests that
   non-integer arguments are handled correctly.
 */

static PyObject *
test_long_as_double(PyObject *self)
{
    double out;

    Py_INCREF(Py_None);

    out = PyLong_AsDouble(Py_None);
    if (out != -1.0 || !PyErr_Occurred())
        return raiseTestError("test_long_as_double",
                              "PyLong_AsDouble(None) didn't complain");
    if (!PyErr_ExceptionMatches(PyExc_TypeError))
        return raiseTestError("test_long_as_double",
                              "PyLong_AsDouble(None) raised "
                              "something other than TypeError");
    PyErr_Clear();

    /* Py_INCREF(Py_None) omitted - we already have a reference to it. */
    return Py_None;
}

/* Test the L code for PyArg_ParseTuple.  This should deliver a long long
   for both long and int arguments.  The test may leak a little memory if
   it fails.
*/
static PyObject *
test_L_code(PyObject *self)
{
    PyObject *tuple, *num;
    long long value;

    tuple = PyTuple_New(1);
    if (tuple == NULL)
        return NULL;

    num = PyLong_FromLong(42);
    if (num == NULL)
        return NULL;

    PyTuple_SET_ITEM(tuple, 0, num);

    value = -1;
    if (!PyArg_ParseTuple(tuple, "L:test_L_code", &value)) {
        return NULL;
    }
    if (value != 42)
        return raiseTestError("test_L_code",
            "L code returned wrong value for long 42");

    Py_DECREF(num);
    num = PyLong_FromLong(42);
    if (num == NULL)
        return NULL;

    PyTuple_SET_ITEM(tuple, 0, num);

    value = -1;
    if (!PyArg_ParseTuple(tuple, "L:test_L_code", &value)) {
        return NULL;
    }
    if (value != 42)
        return raiseTestError("test_L_code",
            "L code returned wrong value for int 42");

    Py_DECREF(tuple);
    Py_RETURN_NONE;
}

static PyObject *
return_none(void *unused)
{
    Py_RETURN_NONE;
}

static PyObject *
raise_error(void *unused)
{
    PyErr_SetNone(PyExc_ValueError);
    return NULL;
}

static int
test_buildvalue_N_error(const char *fmt)
{
    PyObject *arg, *res;

    arg = PyList_New(0);
    if (arg == NULL) {
        return -1;
    }

    Py_INCREF(arg);
    res = Py_BuildValue(fmt, return_none, NULL, arg);
    if (res == NULL) {
        return -1;
    }
    Py_DECREF(res);
    if (Py_REFCNT(arg) != 1) {
        PyErr_Format(TestError, "test_buildvalue_N: "
                     "arg was not decrefed in successful "
                     "Py_BuildValue(\"%s\")", fmt);
        return -1;
    }

    Py_INCREF(arg);
    res = Py_BuildValue(fmt, raise_error, NULL, arg);
    if (res != NULL || !PyErr_Occurred()) {
        PyErr_Format(TestError, "test_buildvalue_N: "
                     "Py_BuildValue(\"%s\") didn't complain", fmt);
        return -1;
    }
    PyErr_Clear();
    if (Py_REFCNT(arg) != 1) {
        PyErr_Format(TestError, "test_buildvalue_N: "
                     "arg was not decrefed in failed "
                     "Py_BuildValue(\"%s\")", fmt);
        return -1;
    }
    Py_DECREF(arg);
    return 0;
}

static PyObject *
test_buildvalue_N(PyObject *self, PyObject *noargs)
{
    PyObject *arg, *res;

    arg = PyList_New(0);
    if (arg == NULL) {
        return NULL;
    }
    Py_INCREF(arg);
    res = Py_BuildValue("N", arg);
    if (res == NULL) {
        return NULL;
    }
    if (res != arg) {
        return raiseTestError("test_buildvalue_N",
                              "Py_BuildValue(\"N\") returned wrong result");
    }
    if (Py_REFCNT(arg) != 2) {
        return raiseTestError("test_buildvalue_N",
                              "arg was not decrefed in Py_BuildValue(\"N\")");
    }
    Py_DECREF(res);
    Py_DECREF(arg);

    if (test_buildvalue_N_error("O&N") < 0)
        return NULL;
    if (test_buildvalue_N_error("(O&N)") < 0)
        return NULL;
    if (test_buildvalue_N_error("[O&N]") < 0)
        return NULL;
    if (test_buildvalue_N_error("{O&N}") < 0)
        return NULL;
    if (test_buildvalue_N_error("{()O&(())N}") < 0)
        return NULL;

    Py_RETURN_NONE;
}


static PyObject *
get_args(PyObject *self, PyObject *args)
{
    if (args == NULL) {
        args = Py_None;
    }
    Py_INCREF(args);
    return args;
}

static PyObject *
get_kwargs(PyObject *self, PyObject *args, PyObject *kwargs)
{
    if (kwargs == NULL) {
        kwargs = Py_None;
    }
    Py_INCREF(kwargs);
    return kwargs;
}

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
static PyObject *
getargs_keywords(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"arg1","arg2","arg3","arg4","arg5", NULL};
    static const char fmt[] = "(ii)i|(i(ii))(iii)i";
    int int_args[10]={-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, fmt, keywords,
        &int_args[0], &int_args[1], &int_args[2], &int_args[3], &int_args[4],
        &int_args[5], &int_args[6], &int_args[7], &int_args[8], &int_args[9]))
        return NULL;
    return Py_BuildValue("iiiiiiiiii",
        int_args[0], int_args[1], int_args[2], int_args[3], int_args[4],
        int_args[5], int_args[6], int_args[7], int_args[8], int_args[9]);
}

/* test PyArg_ParseTupleAndKeywords keyword-only arguments */
static PyObject *
getargs_keyword_only(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"required", "optional", "keyword_only", NULL};
    int required = -1;
    int optional = -1;
    int keyword_only = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|i$i", keywords,
                                     &required, &optional, &keyword_only))
        return NULL;
    return Py_BuildValue("iii", required, optional, keyword_only);
}

/* test PyArg_ParseTupleAndKeywords positional-only arguments */
static PyObject *
getargs_positional_only_and_keywords(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"", "", "keyword", NULL};
    int required = -1;
    int optional = -1;
    int keyword = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|ii", keywords,
                                     &required, &optional, &keyword))
        return NULL;
    return Py_BuildValue("iii", required, optional, keyword);
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

static PyObject *
getargs_p(PyObject *self, PyObject *args)
{
    int value;
    if (!PyArg_ParseTuple(args, "p", &value))
        return NULL;
    return PyLong_FromLong(value);
}

static PyObject *
getargs_L(PyObject *self, PyObject *args)
{
    long long value;
    if (!PyArg_ParseTuple(args, "L", &value))
        return NULL;
    return PyLong_FromLongLong(value);
}

static PyObject *
getargs_K(PyObject *self, PyObject *args)
{
    unsigned long long value;
    if (!PyArg_ParseTuple(args, "K", &value))
        return NULL;
    return PyLong_FromUnsignedLongLong(value);
}

/* This function not only tests the 'k' getargs code, but also the
   PyLong_AsUnsignedLongMask() function. */
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
    if (!PyArg_ParseTuple(tuple, "k:test_k_code", &value)) {
        return NULL;
    }
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
                              "PyLong_AsUnsignedLongMask() returned wrong "
                              "value for long -0xFFF..000042");

    PyTuple_SET_ITEM(tuple, 0, num);

    value = 0;
    if (!PyArg_ParseTuple(tuple, "k:test_k_code", &value)) {
        return NULL;
    }
    if (value != (unsigned long)-0x42)
        return raiseTestError("test_k_code",
            "k code returned wrong value for long -0xFFF..000042");

    Py_DECREF(tuple);
    Py_RETURN_NONE;
}

static PyObject *
getargs_f(PyObject *self, PyObject *args)
{
    float f;
    if (!PyArg_ParseTuple(args, "f", &f))
        return NULL;
    return PyFloat_FromDouble(f);
}

static PyObject *
getargs_d(PyObject *self, PyObject *args)
{
    double d;
    if (!PyArg_ParseTuple(args, "d", &d))
        return NULL;
    return PyFloat_FromDouble(d);
}

static PyObject *
getargs_D(PyObject *self, PyObject *args)
{
    Py_complex cval;
    if (!PyArg_ParseTuple(args, "D", &cval))
        return NULL;
    return PyComplex_FromCComplex(cval);
}

static PyObject *
getargs_S(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "S", &obj))
        return NULL;
    Py_INCREF(obj);
    return obj;
}

static PyObject *
getargs_Y(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "Y", &obj))
        return NULL;
    Py_INCREF(obj);
    return obj;
}

static PyObject *
getargs_U(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "U", &obj))
        return NULL;
    Py_INCREF(obj);
    return obj;
}

static PyObject *
getargs_c(PyObject *self, PyObject *args)
{
    char c;
    if (!PyArg_ParseTuple(args, "c", &c))
        return NULL;
    return PyLong_FromLong((unsigned char)c);
}

static PyObject *
getargs_C(PyObject *self, PyObject *args)
{
    int c;
    if (!PyArg_ParseTuple(args, "C", &c))
        return NULL;
    return PyLong_FromLong(c);
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
    if (!PyArg_ParseTuple(args, "u", &str))
        return NULL;
    return PyUnicode_FromWideChar(str, -1);
}

static PyObject *
getargs_u_hash(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "u#", &str, &size))
        return NULL;
    return PyUnicode_FromWideChar(str, size);
}

static PyObject *
getargs_Z(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    if (!PyArg_ParseTuple(args, "Z", &str))
        return NULL;
    if (str != NULL) {
        return PyUnicode_FromWideChar(str, -1);
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
        return PyUnicode_FromWideChar(str, size);
    else
        Py_RETURN_NONE;
}

static PyObject *
getargs_es(PyObject *self, PyObject *args)
{
    PyObject *arg, *result;
    const char *encoding = NULL;
    char *str;

    if (!PyArg_ParseTuple(args, "O|s", &arg, &encoding))
        return NULL;
    if (!PyArg_Parse(arg, "es", encoding, &str))
        return NULL;
    result = PyBytes_FromString(str);
    PyMem_Free(str);
    return result;
}

static PyObject *
getargs_et(PyObject *self, PyObject *args)
{
    PyObject *arg, *result;
    const char *encoding = NULL;
    char *str;

    if (!PyArg_ParseTuple(args, "O|s", &arg, &encoding))
        return NULL;
    if (!PyArg_Parse(arg, "et", encoding, &str))
        return NULL;
    result = PyBytes_FromString(str);
    PyMem_Free(str);
    return result;
}

static PyObject *
getargs_es_hash(PyObject *self, PyObject *args)
{
    PyObject *arg, *result;
    const char *encoding = NULL;
    PyByteArrayObject *buffer = NULL;
    char *str = NULL;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "O|sY", &arg, &encoding, &buffer))
        return NULL;
    if (buffer != NULL) {
        str = PyByteArray_AS_STRING(buffer);
        size = PyByteArray_GET_SIZE(buffer);
    }
    if (!PyArg_Parse(arg, "es#", encoding, &str, &size))
        return NULL;
    result = PyBytes_FromStringAndSize(str, size);
    if (buffer == NULL)
        PyMem_Free(str);
    return result;
}

static PyObject *
getargs_et_hash(PyObject *self, PyObject *args)
{
    PyObject *arg, *result;
    const char *encoding = NULL;
    PyByteArrayObject *buffer = NULL;
    char *str = NULL;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "O|sY", &arg, &encoding, &buffer))
        return NULL;
    if (buffer != NULL) {
        str = PyByteArray_AS_STRING(buffer);
        size = PyByteArray_GET_SIZE(buffer);
    }
    if (!PyArg_Parse(arg, "et#", encoding, &str, &size))
        return NULL;
    result = PyBytes_FromStringAndSize(str, size);
    if (buffer == NULL)
        PyMem_Free(str);
    return result;
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
    if (!PyArg_ParseTuple(tuple, "s:test_s_code1", &value)) {
        return NULL;
    }

    if (!PyArg_ParseTuple(tuple, "z:test_s_code2", &value)) {
        return NULL;
    }

    Py_DECREF(tuple);
    Py_RETURN_NONE;
}

static PyObject *
parse_tuple_and_keywords(PyObject *self, PyObject *args)
{
    PyObject *sub_args;
    PyObject *sub_kwargs;
    const char *sub_format;
    PyObject *sub_keywords;

    Py_ssize_t i, size;
    char *keywords[8 + 1]; /* space for NULL at end */
    PyObject *o;
    PyObject *converted[8];

    int result;
    PyObject *return_value = NULL;

    double buffers[8][4]; /* double ensures alignment where necessary */

    if (!PyArg_ParseTuple(args, "OOsO:parse_tuple_and_keywords",
        &sub_args, &sub_kwargs,
        &sub_format, &sub_keywords))
        return NULL;

    if (!(PyList_CheckExact(sub_keywords) || PyTuple_CheckExact(sub_keywords))) {
        PyErr_SetString(PyExc_ValueError,
            "parse_tuple_and_keywords: sub_keywords must be either list or tuple");
        return NULL;
    }

    memset(buffers, 0, sizeof(buffers));
    memset(converted, 0, sizeof(converted));
    memset(keywords, 0, sizeof(keywords));

    size = PySequence_Fast_GET_SIZE(sub_keywords);
    if (size > 8) {
        PyErr_SetString(PyExc_ValueError,
            "parse_tuple_and_keywords: too many keywords in sub_keywords");
        goto exit;
    }

    for (i = 0; i < size; i++) {
        o = PySequence_Fast_GET_ITEM(sub_keywords, i);
        if (!PyUnicode_FSConverter(o, (void *)(converted + i))) {
            PyErr_Format(PyExc_ValueError,
                "parse_tuple_and_keywords: could not convert keywords[%zd] to narrow string", i);
            goto exit;
        }
        keywords[i] = PyBytes_AS_STRING(converted[i]);
    }

    result = PyArg_ParseTupleAndKeywords(sub_args, sub_kwargs,
        sub_format, keywords,
        buffers + 0, buffers + 1, buffers + 2, buffers + 3,
        buffers + 4, buffers + 5, buffers + 6, buffers + 7);

    if (result) {
        return_value = Py_None;
        Py_INCREF(Py_None);
    }

exit:
    size = sizeof(converted) / sizeof(converted[0]);
    for (i = 0; i < size; i++) {
        Py_XDECREF(converted[i]);
    }
    return return_value;
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
    if (!PyArg_ParseTuple(tuple, "u:test_u_code", &value)) {
        return NULL;
    }
    if (value != PyUnicode_AS_UNICODE(obj))
        return raiseTestError("test_u_code",
            "u code returned wrong value for u'test'");
    value = 0;
    if (!PyArg_ParseTuple(tuple, "u#:test_u_code", &value, &len)) {
        return NULL;
    }
    if (value != PyUnicode_AS_UNICODE(obj) ||
        len != PyUnicode_GET_SIZE(obj))
        return raiseTestError("test_u_code",
            "u# code returned wrong values for u'test'");

    Py_DECREF(tuple);
    Py_RETURN_NONE;
}

/* Test Z and Z# codes for PyArg_ParseTuple */
static PyObject *
test_Z_code(PyObject *self)
{
    PyObject *tuple, *obj;
    const Py_UNICODE *value1, *value2;
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
    if (!PyArg_ParseTuple(tuple, "ZZ:test_Z_code", &value1, &value2)) {
        return NULL;
    }
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
    if (!PyArg_ParseTuple(tuple, "Z#Z#:test_Z_code", &value1, &len1,
                          &value2, &len2))
    {
        return NULL;
    }
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
    const wchar_t invalid[1] = {(wchar_t)0x110000u};
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

    if (PyUnicode_GET_LENGTH(wide) != PyUnicode_GET_LENGTH(utf8)) {
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

#if defined(SIZEOF_WCHAR_T) && (SIZEOF_WCHAR_T == 4)
    wide = PyUnicode_FromWideChar(invalid, 1);
    if (wide == NULL)
        PyErr_Clear();
    else
        return raiseTestError("test_widechar",
                              "PyUnicode_FromWideChar(L\"\\U00110000\", 1) didn't fail");

    wide = PyUnicode_FromUnicode(invalid, 1);
    if (wide == NULL)
        PyErr_Clear();
    else
        return raiseTestError("test_widechar",
                              "PyUnicode_FromUnicode(L\"\\U00110000\", 1) didn't fail");

    wide = PyUnicode_FromUnicode(NULL, 1);
    if (wide == NULL)
        return NULL;
    PyUnicode_AS_UNICODE(wide)[0] = invalid[0];
    if (_PyUnicode_Ready(wide) < 0) {
        Py_DECREF(wide);
        PyErr_Clear();
    }
    else {
        Py_DECREF(wide);
        return raiseTestError("test_widechar",
                              "PyUnicode_Ready() didn't fail");
    }
#endif

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
    buffer = PyMem_New(wchar_t, buflen);
    if (buffer == NULL)
        return PyErr_NoMemory();

    size = PyUnicode_AsWideChar(unicode, buffer, buflen);
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
unicode_asucs4(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    Py_UCS4 *buffer;
    int copy_null;
    Py_ssize_t str_len, buf_len;

    if (!PyArg_ParseTuple(args, "Unp:unicode_asucs4", &unicode, &str_len, &copy_null)) {
        return NULL;
    }

    buf_len = str_len + 1;
    buffer = PyMem_NEW(Py_UCS4, buf_len);
    if (buffer == NULL) {
        return PyErr_NoMemory();
    }
    memset(buffer, 0, sizeof(Py_UCS4)*buf_len);
    buffer[str_len] = 0xffffU;

    if (!PyUnicode_AsUCS4(unicode, buffer, buf_len, copy_null)) {
        PyMem_FREE(buffer);
        return NULL;
    }

    result = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, buffer, buf_len);
    PyMem_FREE(buffer);
    return result;
}

static PyObject *
unicode_findchar(PyObject *self, PyObject *args)
{
    PyObject *str;
    int direction;
    unsigned int ch;
    Py_ssize_t result;
    Py_ssize_t start, end;

    if (!PyArg_ParseTuple(args, "UInni:unicode_findchar", &str, &ch,
                          &start, &end, &direction)) {
        return NULL;
    }

    result = PyUnicode_FindChar(str, (Py_UCS4)ch, start, end, direction);
    if (result == -2)
        return NULL;
    else
        return PyLong_FromSsize_t(result);
}

static PyObject *
unicode_copycharacters(PyObject *self, PyObject *args)
{
    PyObject *from, *to, *to_copy;
    Py_ssize_t from_start, to_start, how_many, copied;

    if (!PyArg_ParseTuple(args, "UnOnn:unicode_copycharacters", &to, &to_start,
                          &from, &from_start, &how_many)) {
        return NULL;
    }

    if (!(to_copy = PyUnicode_New(PyUnicode_GET_LENGTH(to),
                                  PyUnicode_MAX_CHAR_VALUE(to)))) {
        return NULL;
    }
    if (PyUnicode_Fill(to_copy, 0, PyUnicode_GET_LENGTH(to_copy), 0U) < 0) {
        Py_DECREF(to_copy);
        return NULL;
    }

    if ((copied = PyUnicode_CopyCharacters(to_copy, to_start, from,
                                           from_start, how_many)) < 0) {
        Py_DECREF(to_copy);
        return NULL;
    }

    return Py_BuildValue("(Nn)", to_copy, copied);
}

static PyObject *
unicode_encodedecimal(PyObject *self, PyObject *args)
{
    Py_UNICODE *unicode;
    Py_ssize_t length;
    char *errors = NULL;
    PyObject *decimal;
    Py_ssize_t decimal_length, new_length;
    int res;

    if (!PyArg_ParseTuple(args, "u#|s", &unicode, &length, &errors))
        return NULL;

    decimal_length = length * 7; /* len('&#8364;') */
    decimal = PyBytes_FromStringAndSize(NULL, decimal_length);
    if (decimal == NULL)
        return NULL;

    res = PyUnicode_EncodeDecimal(unicode, length,
                                  PyBytes_AS_STRING(decimal),
                                  errors);
    if (res < 0) {
        Py_DECREF(decimal);
        return NULL;
    }

    new_length = strlen(PyBytes_AS_STRING(decimal));
    assert(new_length <= decimal_length);
    res = _PyBytes_Resize(&decimal, new_length);
    if (res < 0)
        return NULL;

    return decimal;
}

static PyObject *
unicode_transformdecimaltoascii(PyObject *self, PyObject *args)
{
    Py_UNICODE *unicode;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "u#|s", &unicode, &length))
        return NULL;
    return PyUnicode_TransformDecimalToASCII(unicode, length);
}

static PyObject *
unicode_legacy_string(PyObject *self, PyObject *args)
{
    Py_UNICODE *data;
    Py_ssize_t len;
    PyObject *u;

    if (!PyArg_ParseTuple(args, "u#", &data, &len))
        return NULL;

    u = PyUnicode_FromUnicode(NULL, len);
    if (u == NULL)
        return NULL;

    memcpy(PyUnicode_AS_UNICODE(u), data, len * sizeof(Py_UNICODE));

    if (len > 0) { /* The empty string is always ready. */
        assert(!PyUnicode_IS_READY(u));
    }

    return u;
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
    if (!(result = PyArg_ParseTuple(tuple, "|:test_empty_argparse"))) {
        goto done;
    }
    dict = PyDict_New();
    if (!dict)
        goto done;
    result = PyArg_ParseTupleAndKeywords(tuple, dict, "|:test_empty_argparse", kwlist);
  done:
    Py_DECREF(tuple);
    Py_XDECREF(dict);
    if (!result) {
        return NULL;
    }
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
                     {0x7fffL, 15, 1},          /* one Python int digit */
             {-0x7fffL, 15, -1},
             {0xffffL, 16, 1},
             {-0xffffL, 16, -1},
             {0xfffffffL, 28, 1},
             {-0xfffffffL, 28, -1}};
    size_t i;

    for (i = 0; i < Py_ARRAY_LENGTH(testcases); ++i) {
        size_t nbits;
        int sign;
        PyObject *plong;

        plong = PyLong_FromLong(testcases[i].input);
        if (plong == NULL)
            return NULL;
        nbits = _PyLong_NumBits(plong);
        sign = _PyLong_Sign(plong);

        Py_DECREF(plong);
        if (nbits != testcases[i].nbits)
            return raiseTestError("test_long_numbits",
                            "wrong result for _PyLong_NumBits");
        if (sign != testcases[i].sign)
            return raiseTestError("test_long_numbits",
                            "wrong result for _PyLong_Sign");
    }
    Py_RETURN_NONE;
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

static PyObject *
set_errno(PyObject *self, PyObject *args)
{
    int new_errno;

    if (!PyArg_ParseTuple(args, "i:set_errno", &new_errno))
        return NULL;

    errno = new_errno;
    Py_RETURN_NONE;
}

static PyObject *
test_set_exc_info(PyObject *self, PyObject *args)
{
    PyObject *orig_exc;
    PyObject *new_type, *new_value, *new_tb;
    PyObject *type, *value, *tb;
    if (!PyArg_ParseTuple(args, "OOO:test_set_exc_info",
                          &new_type, &new_value, &new_tb))
        return NULL;

    PyErr_GetExcInfo(&type, &value, &tb);

    Py_INCREF(new_type);
    Py_INCREF(new_value);
    Py_INCREF(new_tb);
    PyErr_SetExcInfo(new_type, new_value, new_tb);

    orig_exc = PyTuple_Pack(3, type ? type : Py_None, value ? value : Py_None, tb ? tb : Py_None);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tb);
    return orig_exc;
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

/* Functions exposing the C API type checking for testing */
#define MAKE_DATETIME_CHECK_FUNC(check_method, exact_method)    \
    PyObject *obj;                                              \
    int exact = 0;                                              \
    if (!PyArg_ParseTuple(args, "O|p", &obj, &exact)) {         \
        return NULL;                                            \
    }                                                           \
    int rv = exact?exact_method(obj):check_method(obj);         \
    if (rv) {                                                   \
        Py_RETURN_TRUE;                                         \
    } else {                                                    \
        Py_RETURN_FALSE;                                        \
    }

static PyObject *
datetime_check_date(PyObject *self, PyObject *args) {
    MAKE_DATETIME_CHECK_FUNC(PyDate_Check, PyDate_CheckExact)
}

static PyObject *
datetime_check_time(PyObject *self, PyObject *args) {
    MAKE_DATETIME_CHECK_FUNC(PyTime_Check, PyTime_CheckExact)
}

static PyObject *
datetime_check_datetime(PyObject *self, PyObject *args) {
    MAKE_DATETIME_CHECK_FUNC(PyDateTime_Check, PyDateTime_CheckExact)
}

static PyObject *
datetime_check_delta(PyObject *self, PyObject *args) {
    MAKE_DATETIME_CHECK_FUNC(PyDelta_Check, PyDelta_CheckExact)
}

static PyObject *
datetime_check_tzinfo(PyObject *self, PyObject *args) {
    MAKE_DATETIME_CHECK_FUNC(PyTZInfo_Check, PyTZInfo_CheckExact)
}


/* Makes three variations on timezone representing UTC-5:
   1. timezone with offset and name from PyDateTimeAPI
   2. timezone with offset and name from PyTimeZone_FromOffsetAndName
   3. timezone with offset (no name) from PyTimeZone_FromOffset
*/
static PyObject *
make_timezones_capi(PyObject *self, PyObject *args) {
    PyObject *offset = PyDelta_FromDSU(0, -18000, 0);
    PyObject *name = PyUnicode_FromString("EST");

    PyObject *est_zone_capi = PyDateTimeAPI->TimeZone_FromTimeZone(offset, name);
    PyObject *est_zone_macro = PyTimeZone_FromOffsetAndName(offset, name);
    PyObject *est_zone_macro_noname = PyTimeZone_FromOffset(offset);

    Py_DecRef(offset);
    Py_DecRef(name);

    PyObject *rv = PyTuple_New(3);

    PyTuple_SET_ITEM(rv, 0, est_zone_capi);
    PyTuple_SET_ITEM(rv, 1, est_zone_macro);
    PyTuple_SET_ITEM(rv, 2, est_zone_macro_noname);

    return rv;
}

static PyObject *
get_timezone_utc_capi(PyObject* self, PyObject *args) {
    int macro = 0;
    if (!PyArg_ParseTuple(args, "|p", &macro)) {
        return NULL;
    }
    if (macro) {
        Py_INCREF(PyDateTime_TimeZone_UTC);
        return PyDateTime_TimeZone_UTC;
    } else {
        Py_INCREF(PyDateTimeAPI->TimeZone_UTC);
        return PyDateTimeAPI->TimeZone_UTC;
    }
}


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
    rc = _PyObject_CallNoArg((PyObject *)callable);
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
    PyObject *r = _PyObject_CallNoArg(callable);
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
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/* Some tests of PyUnicode_FromFormat().  This needs more tests. */
static PyObject *
test_string_from_format(PyObject *self, PyObject *args)
{
    PyObject *result;
    char *msg;

#define CHECK_1_FORMAT(FORMAT, TYPE)                                \
    result = PyUnicode_FromFormat(FORMAT, (TYPE)1);                 \
    if (result == NULL)                                             \
        return NULL;                                                \
    if (!_PyUnicode_EqualToASCIIString(result, "1")) {              \
        msg = FORMAT " failed at 1";                                \
        goto Fail;                                                  \
    }                                                               \
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
    CHECK_1_FORMAT("%llu", unsigned long long);
    CHECK_1_FORMAT("%lld", long long);

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
}

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
    const char *msg;

#define CHECK_STRING(STR, expected)                             \
    result = PyOS_string_to_double(STR, NULL, NULL);            \
    if (result == -1.0 && PyErr_Occurred())                     \
        return NULL;                                            \
    if (result != (double)expected) {                           \
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
    if (multiple == NULL)
        return PyErr_NoMemory();
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
    free(multiple);

    /* Test 4: Allocate many integers, then release
       them all simultaneously. */
    multiple = malloc(sizeof(PyObject*) * 1000000);
    if (multiple == NULL)
        return PyErr_NoMemory();
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
    free(multiple);

    /* Test 5: Allocate many integers < 32000 */
    multiple = malloc(sizeof(PyObject*) * 1000000);
    if (multiple == NULL)
        return PyErr_NoMemory();
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
    free(multiple);

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
    if (op1 == NULL)
        return NULL;
    gettimeofday(&start, NULL);
    for(i=0; i < 10000000; i++) {
        result = PyNumber_Add(op1, op1);
        Py_XDECREF(result);
    }
    gettimeofday(&stop, NULL);
    Py_DECREF(op1);
    print_delta(7, &start, &stop);

    Py_RETURN_NONE;
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
        res = PyLong_FromSsize_t(Py_REFCNT(str2));
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

static PyObject *
make_memoryview_from_NULL_pointer(PyObject *self)
{
    Py_buffer info;
    if (PyBuffer_FillInfo(&info, NULL, NULL, 1, 1, PyBUF_FULL_RO) < 0)
        return NULL;
    return PyMemoryView_FromBuffer(&info);
}

static PyObject *
test_from_contiguous(PyObject* self, PyObject *noargs)
{
    int data[9] = {-1,-1,-1,-1,-1,-1,-1,-1,-1};
    int init[5] = {0, 1, 2, 3, 4};
    Py_ssize_t itemsize = sizeof(int);
    Py_ssize_t shape = 5;
    Py_ssize_t strides = 2 * itemsize;
    Py_buffer view = {
        data,
        NULL,
        5 * itemsize,
        itemsize,
        1,
        1,
        NULL,
        &shape,
        &strides,
        NULL,
        NULL
    };
    int *ptr;
    int i;

    PyBuffer_FromContiguous(&view, init, view.len, 'C');
    ptr = view.buf;
    for (i = 0; i < 5; i++) {
        if (ptr[2*i] != i) {
            PyErr_SetString(TestError,
                "test_from_contiguous: incorrect result");
            return NULL;
        }
    }

    view.buf = &data[8];
    view.strides[0] = -2 * itemsize;

    PyBuffer_FromContiguous(&view, init, view.len, 'C');
    ptr = view.buf;
    for (i = 0; i < 5; i++) {
        if (*(ptr-2*i) != i) {
            PyErr_SetString(TestError,
                "test_from_contiguous: incorrect result");
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

#if (defined(__linux__) || defined(__FreeBSD__)) && defined(__GNUC__)
extern PyTypeObject _PyBytesIOBuffer_Type;

static PyObject *
test_pep3118_obsolete_write_locks(PyObject* self, PyObject *noargs)
{
    PyTypeObject *type = &_PyBytesIOBuffer_Type;
    PyObject *b;
    char *dummy[1];
    int ret, match;

    /* PyBuffer_FillInfo() */
    ret = PyBuffer_FillInfo(NULL, NULL, dummy, 1, 0, PyBUF_SIMPLE);
    match = PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_BufferError);
    PyErr_Clear();
    if (ret != -1 || match == 0)
        goto error;

    /* bytesiobuf_getbuffer() */
    b = type->tp_alloc(type, 0);
    if (b == NULL) {
        return NULL;
    }

    ret = PyObject_GetBuffer(b, NULL, PyBUF_SIMPLE);
    Py_DECREF(b);
    match = PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_BufferError);
    PyErr_Clear();
    if (ret != -1 || match == 0)
        goto error;

    Py_RETURN_NONE;

error:
    PyErr_SetString(TestError,
        "test_pep3118_obsolete_write_locks: failure");
    return NULL;
}
#endif

/* This tests functions that historically supported write locks.  It is
   wrong to call getbuffer() with view==NULL and a compliant getbufferproc
   is entitled to segfault in that case. */
static PyObject *
getbuffer_with_null_view(PyObject* self, PyObject *obj)
{
    if (PyObject_GetBuffer(obj, NULL, PyBUF_SIMPLE) < 0)
        return NULL;

    Py_RETURN_NONE;
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

/* To run some code in a sub-interpreter. */
static PyObject *
run_in_subinterp(PyObject *self, PyObject *args)
{
    const char *code;
    int r;
    PyThreadState *substate, *mainstate;

    if (!PyArg_ParseTuple(args, "s:run_in_subinterp",
                          &code))
        return NULL;

    mainstate = PyThreadState_Get();

    PyThreadState_Swap(NULL);

    substate = Py_NewInterpreter();
    if (substate == NULL) {
        /* Since no new thread state was created, there is no exception to
           propagate; raise a fresh one after swapping in the old thread
           state. */
        PyThreadState_Swap(mainstate);
        PyErr_SetString(PyExc_RuntimeError, "sub-interpreter creation failed");
        return NULL;
    }
    r = PyRun_SimpleString(code);
    Py_EndInterpreter(substate);

    PyThreadState_Swap(mainstate);

    return PyLong_FromLong(r);
}

static int
check_time_rounding(int round)
{
    if (round != _PyTime_ROUND_FLOOR
        && round != _PyTime_ROUND_CEILING
        && round != _PyTime_ROUND_HALF_EVEN
        && round != _PyTime_ROUND_UP) {
        PyErr_SetString(PyExc_ValueError, "invalid rounding");
        return -1;
    }
    return 0;
}

static PyObject *
test_pytime_object_to_time_t(PyObject *self, PyObject *args)
{
    PyObject *obj;
    time_t sec;
    int round;
    if (!PyArg_ParseTuple(args, "Oi:pytime_object_to_time_t", &obj, &round))
        return NULL;
    if (check_time_rounding(round) < 0)
        return NULL;
    if (_PyTime_ObjectToTime_t(obj, &sec, round) == -1)
        return NULL;
    return _PyLong_FromTime_t(sec);
}

static PyObject *
test_pytime_object_to_timeval(PyObject *self, PyObject *args)
{
    PyObject *obj;
    time_t sec;
    long usec;
    int round;
    if (!PyArg_ParseTuple(args, "Oi:pytime_object_to_timeval", &obj, &round))
        return NULL;
    if (check_time_rounding(round) < 0)
        return NULL;
    if (_PyTime_ObjectToTimeval(obj, &sec, &usec, round) == -1)
        return NULL;
    return Py_BuildValue("Nl", _PyLong_FromTime_t(sec), usec);
}

static PyObject *
test_pytime_object_to_timespec(PyObject *self, PyObject *args)
{
    PyObject *obj;
    time_t sec;
    long nsec;
    int round;
    if (!PyArg_ParseTuple(args, "Oi:pytime_object_to_timespec", &obj, &round))
        return NULL;
    if (check_time_rounding(round) < 0)
        return NULL;
    if (_PyTime_ObjectToTimespec(obj, &sec, &nsec, round) == -1)
        return NULL;
    return Py_BuildValue("Nl", _PyLong_FromTime_t(sec), nsec);
}

static void
slot_tp_del(PyObject *self)
{
    _Py_IDENTIFIER(__tp_del__);
    PyObject *del, *res;
    PyObject *error_type, *error_value, *error_traceback;

    /* Temporarily resurrect the object. */
    assert(self->ob_refcnt == 0);
    self->ob_refcnt = 1;

    /* Save the current exception, if any. */
    PyErr_Fetch(&error_type, &error_value, &error_traceback);

    /* Execute __del__ method, if any. */
    del = _PyObject_LookupSpecial(self, &PyId___tp_del__);
    if (del != NULL) {
        res = _PyObject_CallNoArg(del);
        if (res == NULL)
            PyErr_WriteUnraisable(del);
        else
            Py_DECREF(res);
        Py_DECREF(del);
    }

    /* Restore the saved exception. */
    PyErr_Restore(error_type, error_value, error_traceback);

    /* Undo the temporary resurrection; can't use DECREF here, it would
     * cause a recursive call.
     */
    assert(self->ob_refcnt > 0);
    if (--self->ob_refcnt == 0)
        return;         /* this is the normal path out */

    /* __del__ resurrected it!  Make it look like the original Py_DECREF
     * never happened.
     */
    {
        Py_ssize_t refcnt = self->ob_refcnt;
        _Py_NewReference(self);
        self->ob_refcnt = refcnt;
    }
    assert(!PyType_IS_GC(Py_TYPE(self)) ||
           _Py_AS_GC(self)->gc.gc_refs != _PyGC_REFS_UNTRACKED);
    /* If Py_REF_DEBUG, _Py_NewReference bumped _Py_RefTotal, so
     * we need to undo that. */
    _Py_DEC_REFTOTAL;
    /* If Py_TRACE_REFS, _Py_NewReference re-added self to the object
     * chain, so no more to do there.
     * If COUNT_ALLOCS, the original decref bumped tp_frees, and
     * _Py_NewReference bumped tp_allocs:  both of those need to be
     * undone.
     */
#ifdef COUNT_ALLOCS
    --Py_TYPE(self)->tp_frees;
    --Py_TYPE(self)->tp_allocs;
#endif
}

static PyObject *
with_tp_del(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyTypeObject *tp;

    if (!PyArg_ParseTuple(args, "O:with_tp_del", &obj))
        return NULL;
    tp = (PyTypeObject *) obj;
    if (!PyType_Check(obj) || !PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
        PyErr_Format(PyExc_TypeError,
                     "heap type expected, got %R", obj);
        return NULL;
    }
    tp->tp_del = slot_tp_del;
    Py_INCREF(obj);
    return obj;
}

static PyMethodDef ml;

static PyObject *
create_cfunction(PyObject *self, PyObject *args)
{
    return PyCFunction_NewEx(&ml, self, NULL);
}

static PyMethodDef ml = {
    "create_cfunction",
    create_cfunction,
    METH_NOARGS,
    NULL
};

static PyObject *
_test_incref(PyObject *ob)
{
    Py_INCREF(ob);
    return ob;
}

static PyObject *
test_xincref_doesnt_leak(PyObject *ob)
{
    PyObject *obj = PyLong_FromLong(0);
    Py_XINCREF(_test_incref(obj));
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_RETURN_NONE;
}

static PyObject *
test_incref_doesnt_leak(PyObject *ob)
{
    PyObject *obj = PyLong_FromLong(0);
    Py_INCREF(_test_incref(obj));
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_RETURN_NONE;
}

static PyObject *
test_xdecref_doesnt_leak(PyObject *ob)
{
    Py_XDECREF(PyLong_FromLong(0));
    Py_RETURN_NONE;
}

static PyObject *
test_decref_doesnt_leak(PyObject *ob)
{
    Py_DECREF(PyLong_FromLong(0));
    Py_RETURN_NONE;
}

static PyObject *
test_incref_decref_API(PyObject *ob)
{
    PyObject *obj = PyLong_FromLong(0);
    Py_IncRef(obj);
    Py_DecRef(obj);
    Py_DecRef(obj);
    Py_RETURN_NONE;
}

static PyObject *
test_pymem_alloc0(PyObject *self)
{
    void *ptr;

    ptr = PyMem_RawMalloc(0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyMem_RawMalloc(0) returns NULL");
        return NULL;
    }
    PyMem_RawFree(ptr);

    ptr = PyMem_RawCalloc(0, 0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyMem_RawCalloc(0, 0) returns NULL");
        return NULL;
    }
    PyMem_RawFree(ptr);

    ptr = PyMem_Malloc(0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyMem_Malloc(0) returns NULL");
        return NULL;
    }
    PyMem_Free(ptr);

    ptr = PyMem_Calloc(0, 0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyMem_Calloc(0, 0) returns NULL");
        return NULL;
    }
    PyMem_Free(ptr);

    ptr = PyObject_Malloc(0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyObject_Malloc(0) returns NULL");
        return NULL;
    }
    PyObject_Free(ptr);

    ptr = PyObject_Calloc(0, 0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyObject_Calloc(0, 0) returns NULL");
        return NULL;
    }
    PyObject_Free(ptr);

    Py_RETURN_NONE;
}

typedef struct {
    PyMemAllocatorEx alloc;

    size_t malloc_size;
    size_t calloc_nelem;
    size_t calloc_elsize;
    void *realloc_ptr;
    size_t realloc_new_size;
    void *free_ptr;
    void *ctx;
} alloc_hook_t;

static void* hook_malloc(void* ctx, size_t size)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->malloc_size = size;
    return hook->alloc.malloc(hook->alloc.ctx, size);
}

static void* hook_calloc(void* ctx, size_t nelem, size_t elsize)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->calloc_nelem = nelem;
    hook->calloc_elsize = elsize;
    return hook->alloc.calloc(hook->alloc.ctx, nelem, elsize);
}

static void* hook_realloc(void* ctx, void* ptr, size_t new_size)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->realloc_ptr = ptr;
    hook->realloc_new_size = new_size;
    return hook->alloc.realloc(hook->alloc.ctx, ptr, new_size);
}

static void hook_free(void *ctx, void *ptr)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->free_ptr = ptr;
    hook->alloc.free(hook->alloc.ctx, ptr);
}

static PyObject *
test_setallocators(PyMemAllocatorDomain domain)
{
    PyObject *res = NULL;
    const char *error_msg;
    alloc_hook_t hook;
    PyMemAllocatorEx alloc;
    size_t size, size2, nelem, elsize;
    void *ptr, *ptr2;

    memset(&hook, 0, sizeof(hook));

    alloc.ctx = &hook;
    alloc.malloc = &hook_malloc;
    alloc.calloc = &hook_calloc;
    alloc.realloc = &hook_realloc;
    alloc.free = &hook_free;
    PyMem_GetAllocator(domain, &hook.alloc);
    PyMem_SetAllocator(domain, &alloc);

    /* malloc, realloc, free */
    size = 42;
    hook.ctx = NULL;
    switch(domain)
    {
    case PYMEM_DOMAIN_RAW: ptr = PyMem_RawMalloc(size); break;
    case PYMEM_DOMAIN_MEM: ptr = PyMem_Malloc(size); break;
    case PYMEM_DOMAIN_OBJ: ptr = PyObject_Malloc(size); break;
    default: ptr = NULL; break;
    }

#define CHECK_CTX(FUNC) \
    if (hook.ctx != &hook) { \
        error_msg = FUNC " wrong context"; \
        goto fail; \
    } \
    hook.ctx = NULL;  /* reset for next check */

    if (ptr == NULL) {
        error_msg = "malloc failed";
        goto fail;
    }
    CHECK_CTX("malloc");
    if (hook.malloc_size != size) {
        error_msg = "malloc invalid size";
        goto fail;
    }

    size2 = 200;
    switch(domain)
    {
    case PYMEM_DOMAIN_RAW: ptr2 = PyMem_RawRealloc(ptr, size2); break;
    case PYMEM_DOMAIN_MEM: ptr2 = PyMem_Realloc(ptr, size2); break;
    case PYMEM_DOMAIN_OBJ: ptr2 = PyObject_Realloc(ptr, size2); break;
    default: ptr2 = NULL; break;
    }

    if (ptr2 == NULL) {
        error_msg = "realloc failed";
        goto fail;
    }
    CHECK_CTX("realloc");
    if (hook.realloc_ptr != ptr
        || hook.realloc_new_size != size2) {
        error_msg = "realloc invalid parameters";
        goto fail;
    }

    switch(domain)
    {
    case PYMEM_DOMAIN_RAW: PyMem_RawFree(ptr2); break;
    case PYMEM_DOMAIN_MEM: PyMem_Free(ptr2); break;
    case PYMEM_DOMAIN_OBJ: PyObject_Free(ptr2); break;
    }

    CHECK_CTX("free");
    if (hook.free_ptr != ptr2) {
        error_msg = "free invalid pointer";
        goto fail;
    }

    /* calloc, free */
    nelem = 2;
    elsize = 5;
    switch(domain)
    {
    case PYMEM_DOMAIN_RAW: ptr = PyMem_RawCalloc(nelem, elsize); break;
    case PYMEM_DOMAIN_MEM: ptr = PyMem_Calloc(nelem, elsize); break;
    case PYMEM_DOMAIN_OBJ: ptr = PyObject_Calloc(nelem, elsize); break;
    default: ptr = NULL; break;
    }

    if (ptr == NULL) {
        error_msg = "calloc failed";
        goto fail;
    }
    CHECK_CTX("calloc");
    if (hook.calloc_nelem != nelem || hook.calloc_elsize != elsize) {
        error_msg = "calloc invalid nelem or elsize";
        goto fail;
    }

    hook.free_ptr = NULL;
    switch(domain)
    {
    case PYMEM_DOMAIN_RAW: PyMem_RawFree(ptr); break;
    case PYMEM_DOMAIN_MEM: PyMem_Free(ptr); break;
    case PYMEM_DOMAIN_OBJ: PyObject_Free(ptr); break;
    }

    CHECK_CTX("calloc free");
    if (hook.free_ptr != ptr) {
        error_msg = "calloc free invalid pointer";
        goto fail;
    }

    Py_INCREF(Py_None);
    res = Py_None;
    goto finally;

fail:
    PyErr_SetString(PyExc_RuntimeError, error_msg);

finally:
    PyMem_SetAllocator(domain, &hook.alloc);
    return res;

#undef CHECK_CTX
}

static PyObject *
test_pymem_setrawallocators(PyObject *self)
{
    return test_setallocators(PYMEM_DOMAIN_RAW);
}

static PyObject *
test_pymem_setallocators(PyObject *self)
{
    return test_setallocators(PYMEM_DOMAIN_MEM);
}

static PyObject *
test_pyobject_setallocators(PyObject *self)
{
    return test_setallocators(PYMEM_DOMAIN_OBJ);
}

/* Most part of the following code is inherited from the pyfailmalloc project
 * written by Victor Stinner. */
static struct {
    int installed;
    PyMemAllocatorEx raw;
    PyMemAllocatorEx mem;
    PyMemAllocatorEx obj;
} FmHook;

static struct {
    int start;
    int stop;
    Py_ssize_t count;
} FmData;

static int
fm_nomemory(void)
{
    FmData.count++;
    if (FmData.count > FmData.start &&
            (FmData.stop <= 0 || FmData.count <= FmData.stop)) {
        return 1;
    }
    return 0;
}

static void *
hook_fmalloc(void *ctx, size_t size)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    if (fm_nomemory()) {
        return NULL;
    }
    return alloc->malloc(alloc->ctx, size);
}

static void *
hook_fcalloc(void *ctx, size_t nelem, size_t elsize)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    if (fm_nomemory()) {
        return NULL;
    }
    return alloc->calloc(alloc->ctx, nelem, elsize);
}

static void *
hook_frealloc(void *ctx, void *ptr, size_t new_size)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    if (fm_nomemory()) {
        return NULL;
    }
    return alloc->realloc(alloc->ctx, ptr, new_size);
}

static void
hook_ffree(void *ctx, void *ptr)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    alloc->free(alloc->ctx, ptr);
}

static void
fm_setup_hooks(void)
{
    PyMemAllocatorEx alloc;

    if (FmHook.installed) {
        return;
    }
    FmHook.installed = 1;

    alloc.malloc = hook_fmalloc;
    alloc.calloc = hook_fcalloc;
    alloc.realloc = hook_frealloc;
    alloc.free = hook_ffree;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &FmHook.raw);
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &FmHook.mem);
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &FmHook.obj);

    alloc.ctx = &FmHook.raw;
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);

    alloc.ctx = &FmHook.mem;
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);

    alloc.ctx = &FmHook.obj;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);
}

static void
fm_remove_hooks(void)
{
    if (FmHook.installed) {
        FmHook.installed = 0;
        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &FmHook.raw);
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &FmHook.mem);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &FmHook.obj);
    }
}

static PyObject*
set_nomemory(PyObject *self, PyObject *args)
{
    /* Memory allocation fails after 'start' allocation requests, and until
     * 'stop' allocation requests except when 'stop' is negative or equal
     * to 0 (default) in which case allocation failures never stop. */
    FmData.count = 0;
    FmData.stop = 0;
    if (!PyArg_ParseTuple(args, "i|i", &FmData.start, &FmData.stop)) {
        return NULL;
    }
    fm_setup_hooks();
    Py_RETURN_NONE;
}

static PyObject*
remove_mem_hooks(PyObject *self)
{
    fm_remove_hooks();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(docstring_empty,
""
);

PyDoc_STRVAR(docstring_no_signature,
"This docstring has no signature."
);

PyDoc_STRVAR(docstring_with_invalid_signature,
"docstring_with_invalid_signature($module, /, boo)\n"
"\n"
"This docstring has an invalid signature."
);

PyDoc_STRVAR(docstring_with_invalid_signature2,
"docstring_with_invalid_signature2($module, /, boo)\n"
"\n"
"--\n"
"\n"
"This docstring also has an invalid signature."
);

PyDoc_STRVAR(docstring_with_signature,
"docstring_with_signature($module, /, sig)\n"
"--\n"
"\n"
"This docstring has a valid signature."
);

PyDoc_STRVAR(docstring_with_signature_but_no_doc,
"docstring_with_signature_but_no_doc($module, /, sig)\n"
"--\n"
"\n"
);

PyDoc_STRVAR(docstring_with_signature_and_extra_newlines,
"docstring_with_signature_and_extra_newlines($module, /, parameter)\n"
"--\n"
"\n"
"\n"
"This docstring has a valid signature and some extra newlines."
);

PyDoc_STRVAR(docstring_with_signature_with_defaults,
"docstring_with_signature_with_defaults(module, s='avocado',\n"
"        b=b'bytes', d=3.14, i=35, n=None, t=True, f=False,\n"
"        local=the_number_three, sys=sys.maxsize,\n"
"        exp=sys.maxsize - 1)\n"
"--\n"
"\n"
"\n"
"\n"
"This docstring has a valid signature with parameters,\n"
"and the parameters take defaults of varying types."
);

typedef struct {
    PyThread_type_lock start_event;
    PyThread_type_lock exit_event;
    PyObject *callback;
} test_c_thread_t;

static void
temporary_c_thread(void *data)
{
    test_c_thread_t *test_c_thread = data;
    PyGILState_STATE state;
    PyObject *res;

    PyThread_release_lock(test_c_thread->start_event);

    /* Allocate a Python thread state for this thread */
    state = PyGILState_Ensure();

    res = _PyObject_CallNoArg(test_c_thread->callback);
    Py_CLEAR(test_c_thread->callback);

    if (res == NULL) {
        PyErr_Print();
    }
    else {
        Py_DECREF(res);
    }

    /* Destroy the Python thread state for this thread */
    PyGILState_Release(state);

    PyThread_release_lock(test_c_thread->exit_event);

    PyThread_exit_thread();
}

static PyObject *
call_in_temporary_c_thread(PyObject *self, PyObject *callback)
{
    PyObject *res = NULL;
    test_c_thread_t test_c_thread;
    long thread;

    PyEval_InitThreads();

    test_c_thread.start_event = PyThread_allocate_lock();
    test_c_thread.exit_event = PyThread_allocate_lock();
    test_c_thread.callback = NULL;
    if (!test_c_thread.start_event || !test_c_thread.exit_event) {
        PyErr_SetString(PyExc_RuntimeError, "could not allocate lock");
        goto exit;
    }

    Py_INCREF(callback);
    test_c_thread.callback = callback;

    PyThread_acquire_lock(test_c_thread.start_event, 1);
    PyThread_acquire_lock(test_c_thread.exit_event, 1);

    thread = PyThread_start_new_thread(temporary_c_thread, &test_c_thread);
    if (thread == -1) {
        PyErr_SetString(PyExc_RuntimeError, "unable to start the thread");
        PyThread_release_lock(test_c_thread.start_event);
        PyThread_release_lock(test_c_thread.exit_event);
        goto exit;
    }

    PyThread_acquire_lock(test_c_thread.start_event, 1);
    PyThread_release_lock(test_c_thread.start_event);

    Py_BEGIN_ALLOW_THREADS
        PyThread_acquire_lock(test_c_thread.exit_event, 1);
        PyThread_release_lock(test_c_thread.exit_event);
    Py_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    res = Py_None;

exit:
    Py_CLEAR(test_c_thread.callback);
    if (test_c_thread.start_event)
        PyThread_free_lock(test_c_thread.start_event);
    if (test_c_thread.exit_event)
        PyThread_free_lock(test_c_thread.exit_event);
    return res;
}

static PyObject*
test_raise_signal(PyObject* self, PyObject *args)
{
    int signum, err;

    if (!PyArg_ParseTuple(args, "i:raise_signal", &signum)) {
        return NULL;
    }

    err = raise(signum);
    if (err)
        return PyErr_SetFromErrno(PyExc_OSError);

    if (PyErr_CheckSignals() < 0)
        return NULL;

    Py_RETURN_NONE;
}

/* marshal */

static PyObject*
pymarshal_write_long_to_file(PyObject* self, PyObject *args)
{
    long value;
    char *filename;
    int version;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "lsi:pymarshal_write_long_to_file",
                          &value, &filename, &version))
        return NULL;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    PyMarshal_WriteLongToFile(value, fp, version);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

static PyObject*
pymarshal_write_object_to_file(PyObject* self, PyObject *args)
{
    PyObject *obj;
    char *filename;
    int version;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "Osi:pymarshal_write_object_to_file",
                          &obj, &filename, &version))
        return NULL;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    PyMarshal_WriteObjectToFile(obj, fp, version);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

static PyObject*
pymarshal_read_short_from_file(PyObject* self, PyObject *args)
{
    int value;
    long pos;
    char *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "s:pymarshal_read_short_from_file", &filename))
        return NULL;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    value = PyMarshal_ReadShortFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    return Py_BuildValue("il", value, pos);
}

static PyObject*
pymarshal_read_long_from_file(PyObject* self, PyObject *args)
{
    long value, pos;
    char *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "s:pymarshal_read_long_from_file", &filename))
        return NULL;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    value = PyMarshal_ReadLongFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    return Py_BuildValue("ll", value, pos);
}

static PyObject*
pymarshal_read_last_object_from_file(PyObject* self, PyObject *args)
{
    PyObject *obj;
    long pos;
    char *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "s:pymarshal_read_last_object_from_file", &filename))
        return NULL;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    obj = PyMarshal_ReadLastObjectFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    return Py_BuildValue("Nl", obj, pos);
}

static PyObject*
pymarshal_read_object_from_file(PyObject* self, PyObject *args)
{
    PyObject *obj;
    long pos;
    char *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "s:pymarshal_read_object_from_file", &filename))
        return NULL;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    obj = PyMarshal_ReadObjectFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    return Py_BuildValue("Nl", obj, pos);
}

static PyObject*
return_null_without_error(PyObject *self, PyObject *args)
{
    /* invalid call: return NULL without setting an error,
     * _Py_CheckFunctionResult() must detect such bug at runtime. */
    PyErr_Clear();
    return NULL;
}

static PyObject*
return_result_with_error(PyObject *self, PyObject *args)
{
    /* invalid call: return a result with an error set,
     * _Py_CheckFunctionResult() must detect such bug at runtime. */
    PyErr_SetNone(PyExc_ValueError);
    Py_RETURN_NONE;
}

static PyObject *
test_pytime_fromseconds(PyObject *self, PyObject *args)
{
    int seconds;
    _PyTime_t ts;

    if (!PyArg_ParseTuple(args, "i", &seconds))
        return NULL;
    ts = _PyTime_FromSeconds(seconds);
    return _PyTime_AsNanosecondsObject(ts);
}

static PyObject *
test_pytime_fromsecondsobject(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    _PyTime_t ts;

    if (!PyArg_ParseTuple(args, "Oi", &obj, &round))
        return NULL;
    if (check_time_rounding(round) < 0)
        return NULL;
    if (_PyTime_FromSecondsObject(&ts, obj, round) == -1)
        return NULL;
    return _PyTime_AsNanosecondsObject(ts);
}

static PyObject *
test_pytime_assecondsdouble(PyObject *self, PyObject *args)
{
    PyObject *obj;
    _PyTime_t ts;
    double d;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    if (_PyTime_FromNanosecondsObject(&ts, obj) < 0) {
        return NULL;
    }
    d = _PyTime_AsSecondsDouble(ts);
    return PyFloat_FromDouble(d);
}

static PyObject *
test_PyTime_AsTimeval(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    _PyTime_t t;
    struct timeval tv;
    PyObject *seconds;

    if (!PyArg_ParseTuple(args, "Oi", &obj, &round))
        return NULL;
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    if (_PyTime_AsTimeval(t, &tv, round) < 0) {
        return NULL;
    }

    seconds = PyLong_FromLongLong(tv.tv_sec);
    if (seconds == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", seconds, tv.tv_usec);
}

#ifdef HAVE_CLOCK_GETTIME
static PyObject *
test_PyTime_AsTimespec(PyObject *self, PyObject *args)
{
    PyObject *obj;
    _PyTime_t t;
    struct timespec ts;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    if (_PyTime_AsTimespec(t, &ts) == -1) {
        return NULL;
    }
    return Py_BuildValue("Nl", _PyLong_FromTime_t(ts.tv_sec), ts.tv_nsec);
}
#endif

static PyObject *
test_PyTime_AsMilliseconds(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    _PyTime_t t, ms;

    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
        return NULL;
    }
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    ms = _PyTime_AsMilliseconds(t, round);
    /* This conversion rely on the fact that _PyTime_t is a number of
       nanoseconds */
    return _PyTime_AsNanosecondsObject(ms);
}

static PyObject *
test_PyTime_AsMicroseconds(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    _PyTime_t t, ms;

    if (!PyArg_ParseTuple(args, "Oi", &obj, &round))
        return NULL;
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    ms = _PyTime_AsMicroseconds(t, round);
    /* This conversion rely on the fact that _PyTime_t is a number of
       nanoseconds */
    return _PyTime_AsNanosecondsObject(ms);
}

static PyObject*
get_recursion_depth(PyObject *self, PyObject *args)
{
    PyThreadState *tstate = PyThreadState_GET();

    /* subtract one to ignore the frame of the get_recursion_depth() call */
    return PyLong_FromLong(tstate->recursion_depth - 1);
}

static PyObject*
pymem_buffer_overflow(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate buffer overflow to check that PyMem_Free() detects
       the overflow when debug hooks are installed. */
    buffer = PyMem_Malloc(16);
    buffer[16] = 'x';
    PyMem_Free(buffer);

    Py_RETURN_NONE;
}

static PyObject*
pymem_api_misuse(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate misusage of Python allocators:
       allococate with PyMem but release with PyMem_Raw. */
    buffer = PyMem_Malloc(16);
    PyMem_RawFree(buffer);

    Py_RETURN_NONE;
}

static PyObject*
pymem_malloc_without_gil(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate bug to test debug hooks on Python memory allocators:
       call PyMem_Malloc() without holding the GIL */
    Py_BEGIN_ALLOW_THREADS
    buffer = PyMem_Malloc(10);
    Py_END_ALLOW_THREADS

    PyMem_Free(buffer);

    Py_RETURN_NONE;
}


static PyObject*
test_pymem_getallocatorsname(PyObject *self, PyObject *args)
{
    const char *name = _PyMem_GetAllocatorsName();
    if (name == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "cannot get allocators name");
        return NULL;
    }
    return PyUnicode_FromString(name);
}


static PyObject*
pyobject_malloc_without_gil(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate bug to test debug hooks on Python memory allocators:
       call PyObject_Malloc() without holding the GIL */
    Py_BEGIN_ALLOW_THREADS
    buffer = PyObject_Malloc(10);
    Py_END_ALLOW_THREADS

    PyObject_Free(buffer);

    Py_RETURN_NONE;
}

static PyObject *
tracemalloc_track(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;
    void *ptr;
    Py_ssize_t size;
    int release_gil = 0;
    int res;

    if (!PyArg_ParseTuple(args, "IOn|i", &domain, &ptr_obj, &size, &release_gil))
        return NULL;
    ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred())
        return NULL;

    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        res = PyTraceMalloc_Track(domain, (uintptr_t)ptr, size);
        Py_END_ALLOW_THREADS
    }
    else {
        res = PyTraceMalloc_Track(domain, (uintptr_t)ptr, size);
    }

    if (res < 0) {
        PyErr_SetString(PyExc_RuntimeError, "PyTraceMalloc_Track error");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
tracemalloc_untrack(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;
    void *ptr;
    int res;

    if (!PyArg_ParseTuple(args, "IO", &domain, &ptr_obj))
        return NULL;
    ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred())
        return NULL;

    res = PyTraceMalloc_Untrack(domain, (uintptr_t)ptr);
    if (res < 0) {
        PyErr_SetString(PyExc_RuntimeError, "PyTraceMalloc_Untrack error");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
tracemalloc_get_traceback(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;
    void *ptr;

    if (!PyArg_ParseTuple(args, "IO", &domain, &ptr_obj))
        return NULL;
    ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred())
        return NULL;

    return _PyTraceMalloc_GetTraceback(domain, (uintptr_t)ptr);
}

static PyObject *
dict_get_version(PyObject *self, PyObject *args)
{
    PyDictObject *dict;
    uint64_t version;

    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
        return NULL;

    version = dict->ma_version_tag;

    Py_BUILD_ASSERT(sizeof(unsigned PY_LONG_LONG) >= sizeof(version));
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)version);
}


static PyObject *
raise_SIGINT_then_send_None(PyObject *self, PyObject *args)
{
    PyGenObject *gen;

    if (!PyArg_ParseTuple(args, "O!", &PyGen_Type, &gen))
        return NULL;

    /* This is used in a test to check what happens if a signal arrives just
       as we're in the process of entering a yield from chain (see
       bpo-30039).

       Needs to be done in C, because:
       - we don't have a Python wrapper for raise()
       - we need to make sure that the Python-level signal handler doesn't run
         *before* we enter the generator frame, which is impossible in Python
         because we check for signals before every bytecode operation.
     */
    raise(SIGINT);
    return _PyGen_Send(gen, Py_None);
}


static int
fastcall_args(PyObject *args, PyObject ***stack, Py_ssize_t *nargs)
{
    if (args == Py_None) {
        *stack = NULL;
        *nargs = 0;
    }
    else if (PyTuple_Check(args)) {
        *stack = &PyTuple_GET_ITEM(args, 0);
        *nargs = PyTuple_GET_SIZE(args);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "args must be None or a tuple");
        return -1;
    }
    return 0;
}


static PyObject *
test_pyobject_fastcall(PyObject *self, PyObject *args)
{
    PyObject *func, *func_args;
    PyObject **stack;
    Py_ssize_t nargs;

    if (!PyArg_ParseTuple(args, "OO", &func, &func_args)) {
        return NULL;
    }

    if (fastcall_args(func_args, &stack, &nargs) < 0) {
        return NULL;
    }
    return _PyObject_FastCall(func, stack, nargs);
}


static PyObject *
test_pyobject_fastcalldict(PyObject *self, PyObject *args)
{
    PyObject *func, *func_args, *kwargs;
    PyObject **stack;
    Py_ssize_t nargs;

    if (!PyArg_ParseTuple(args, "OOO", &func, &func_args, &kwargs)) {
        return NULL;
    }

    if (fastcall_args(func_args, &stack, &nargs) < 0) {
        return NULL;
    }

    if (kwargs == Py_None) {
        kwargs = NULL;
    }
    else if (!PyDict_Check(kwargs)) {
        PyErr_SetString(PyExc_TypeError, "kwnames must be None or a dict");
        return NULL;
    }

    return _PyObject_FastCallDict(func, stack, nargs, kwargs);
}


static PyObject *
test_pyobject_fastcallkeywords(PyObject *self, PyObject *args)
{
    PyObject *func, *func_args, *kwnames = NULL;
    PyObject **stack;
    Py_ssize_t nargs, nkw;

    if (!PyArg_ParseTuple(args, "OOO", &func, &func_args, &kwnames)) {
        return NULL;
    }

    if (fastcall_args(func_args, &stack, &nargs) < 0) {
        return NULL;
    }

    if (kwnames == Py_None) {
        kwnames = NULL;
    }
    else if (PyTuple_Check(kwnames)) {
        nkw = PyTuple_GET_SIZE(kwnames);
        if (nargs < nkw) {
            PyErr_SetString(PyExc_ValueError, "kwnames longer than args");
            return NULL;
        }
        nargs -= nkw;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "kwnames must be None or a tuple");
        return NULL;
    }
    return _PyObject_FastCallKeywords(func, stack, nargs, kwnames);
}


static PyObject*
stack_pointer(PyObject *self, PyObject *args)
{
    int v = 5;
    return PyLong_FromVoidPtr(&v);
}


#ifdef W_STOPCODE
static PyObject*
py_w_stopcode(PyObject *self, PyObject *args)
{
    int sig, status;
    if (!PyArg_ParseTuple(args, "i", &sig)) {
        return NULL;
    }
    status = W_STOPCODE(sig);
    return PyLong_FromLong(status);
}
#endif


static PyObject *
get_mapping_keys(PyObject* self, PyObject *obj)
{
    return PyMapping_Keys(obj);
}

static PyObject *
get_mapping_values(PyObject* self, PyObject *obj)
{
    return PyMapping_Values(obj);
}

static PyObject *
get_mapping_items(PyObject* self, PyObject *obj)
{
    return PyMapping_Items(obj);
}


static PyObject *
test_pythread_tss_key_state(PyObject *self, PyObject *args)
{
    Py_tss_t tss_key = Py_tss_NEEDS_INIT;
    if (PyThread_tss_is_created(&tss_key)) {
        return raiseTestError("test_pythread_tss_key_state",
                              "TSS key not in an uninitialized state at "
                              "creation time");
    }
    if (PyThread_tss_create(&tss_key) != 0) {
        PyErr_SetString(PyExc_RuntimeError, "PyThread_tss_create failed");
        return NULL;
    }
    if (!PyThread_tss_is_created(&tss_key)) {
        return raiseTestError("test_pythread_tss_key_state",
                              "PyThread_tss_create succeeded, "
                              "but with TSS key in an uninitialized state");
    }
    if (PyThread_tss_create(&tss_key) != 0) {
        return raiseTestError("test_pythread_tss_key_state",
                              "PyThread_tss_create unsuccessful with "
                              "an already initialized key");
    }
#define CHECK_TSS_API(expr) \
        (void)(expr); \
        if (!PyThread_tss_is_created(&tss_key)) { \
            return raiseTestError("test_pythread_tss_key_state", \
                                  "TSS key initialization state was not " \
                                  "preserved after calling " #expr); }
    CHECK_TSS_API(PyThread_tss_set(&tss_key, NULL));
    CHECK_TSS_API(PyThread_tss_get(&tss_key));
#undef CHECK_TSS_API
    PyThread_tss_delete(&tss_key);
    if (PyThread_tss_is_created(&tss_key)) {
        return raiseTestError("test_pythread_tss_key_state",
                              "PyThread_tss_delete called, but did not "
                              "set the key state to uninitialized");
    }

    Py_tss_t *ptr_key = PyThread_tss_alloc();
    if (ptr_key == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyThread_tss_alloc failed");
        return NULL;
    }
    if (PyThread_tss_is_created(ptr_key)) {
        return raiseTestError("test_pythread_tss_key_state",
                              "TSS key not in an uninitialized state at "
                              "allocation time");
    }
    PyThread_tss_free(ptr_key);
    ptr_key = NULL;
    Py_RETURN_NONE;
}


static PyObject*
new_hamt(PyObject *self, PyObject *args)
{
    return _PyContext_NewHamtForTests();
}


static PyMethodDef TestMethods[] = {
    {"raise_exception",         raise_exception,                 METH_VARARGS},
    {"raise_memoryerror",   (PyCFunction)raise_memoryerror,  METH_NOARGS},
    {"set_errno",               set_errno,                       METH_VARARGS},
    {"test_config",             (PyCFunction)test_config,        METH_NOARGS},
    {"test_sizeof_c_types",     (PyCFunction)test_sizeof_c_types, METH_NOARGS},
    {"test_datetime_capi",  test_datetime_capi,              METH_NOARGS},
    {"datetime_check_date",     datetime_check_date,             METH_VARARGS},
    {"datetime_check_time",     datetime_check_time,             METH_VARARGS},
    {"datetime_check_datetime",     datetime_check_datetime,     METH_VARARGS},
    {"datetime_check_delta",     datetime_check_delta,           METH_VARARGS},
    {"datetime_check_tzinfo",     datetime_check_tzinfo,         METH_VARARGS},
    {"make_timezones_capi",     make_timezones_capi,             METH_NOARGS},
    {"get_timezone_utc_capi",    get_timezone_utc_capi,            METH_VARARGS},
    {"test_list_api",           (PyCFunction)test_list_api,      METH_NOARGS},
    {"test_dict_iteration",     (PyCFunction)test_dict_iteration,METH_NOARGS},
    {"dict_getitem_knownhash",  dict_getitem_knownhash,          METH_VARARGS},
    {"dict_hassplittable",      dict_hassplittable,              METH_O},
    {"test_lazy_hash_inheritance",      (PyCFunction)test_lazy_hash_inheritance,METH_NOARGS},
    {"test_long_api",           (PyCFunction)test_long_api,      METH_NOARGS},
    {"test_xincref_doesnt_leak",(PyCFunction)test_xincref_doesnt_leak,      METH_NOARGS},
    {"test_incref_doesnt_leak", (PyCFunction)test_incref_doesnt_leak,      METH_NOARGS},
    {"test_xdecref_doesnt_leak",(PyCFunction)test_xdecref_doesnt_leak,      METH_NOARGS},
    {"test_decref_doesnt_leak", (PyCFunction)test_decref_doesnt_leak,      METH_NOARGS},
    {"test_incref_decref_API",  (PyCFunction)test_incref_decref_API,       METH_NOARGS},
    {"test_long_and_overflow", (PyCFunction)test_long_and_overflow,
     METH_NOARGS},
    {"test_long_as_double",     (PyCFunction)test_long_as_double,METH_NOARGS},
    {"test_long_as_size_t",     (PyCFunction)test_long_as_size_t,METH_NOARGS},
    {"test_long_numbits",       (PyCFunction)test_long_numbits,  METH_NOARGS},
    {"test_k_code",             (PyCFunction)test_k_code,        METH_NOARGS},
    {"test_empty_argparse", (PyCFunction)test_empty_argparse,METH_NOARGS},
    {"parse_tuple_and_keywords", parse_tuple_and_keywords, METH_VARARGS},
    {"test_null_strings",       (PyCFunction)test_null_strings,  METH_NOARGS},
    {"test_string_from_format", (PyCFunction)test_string_from_format, METH_NOARGS},
    {"test_with_docstring", (PyCFunction)test_with_docstring, METH_NOARGS,
     PyDoc_STR("This is a pretty normal docstring.")},
    {"test_string_to_double", (PyCFunction)test_string_to_double, METH_NOARGS},
    {"test_unicode_compare_with_ascii", (PyCFunction)test_unicode_compare_with_ascii, METH_NOARGS},
    {"test_capsule", (PyCFunction)test_capsule, METH_NOARGS},
    {"test_from_contiguous", (PyCFunction)test_from_contiguous, METH_NOARGS},
#if (defined(__linux__) || defined(__FreeBSD__)) && defined(__GNUC__)
    {"test_pep3118_obsolete_write_locks", (PyCFunction)test_pep3118_obsolete_write_locks, METH_NOARGS},
#endif
    {"getbuffer_with_null_view", getbuffer_with_null_view, METH_O},
    {"test_buildvalue_N",       test_buildvalue_N,               METH_NOARGS},
    {"get_args", get_args, METH_VARARGS},
    {"get_kwargs", (PyCFunction)get_kwargs, METH_VARARGS|METH_KEYWORDS},
    {"getargs_tuple",           getargs_tuple,                   METH_VARARGS},
    {"getargs_keywords", (PyCFunction)getargs_keywords,
      METH_VARARGS|METH_KEYWORDS},
    {"getargs_keyword_only", (PyCFunction)getargs_keyword_only,
      METH_VARARGS|METH_KEYWORDS},
    {"getargs_positional_only_and_keywords",
      (PyCFunction)getargs_positional_only_and_keywords,
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
    {"getargs_p",               getargs_p,                       METH_VARARGS},
    {"getargs_L",               getargs_L,                       METH_VARARGS},
    {"getargs_K",               getargs_K,                       METH_VARARGS},
    {"test_longlong_api",       test_longlong_api,               METH_NOARGS},
    {"test_long_long_and_overflow",
        (PyCFunction)test_long_long_and_overflow, METH_NOARGS},
    {"test_L_code",             (PyCFunction)test_L_code,        METH_NOARGS},
    {"getargs_f",               getargs_f,                       METH_VARARGS},
    {"getargs_d",               getargs_d,                       METH_VARARGS},
    {"getargs_D",               getargs_D,                       METH_VARARGS},
    {"getargs_S",               getargs_S,                       METH_VARARGS},
    {"getargs_Y",               getargs_Y,                       METH_VARARGS},
    {"getargs_U",               getargs_U,                       METH_VARARGS},
    {"getargs_c",               getargs_c,                       METH_VARARGS},
    {"getargs_C",               getargs_C,                       METH_VARARGS},
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
    {"getargs_es",              getargs_es,                      METH_VARARGS},
    {"getargs_et",              getargs_et,                      METH_VARARGS},
    {"getargs_es_hash",         getargs_es_hash,                 METH_VARARGS},
    {"getargs_et_hash",         getargs_et_hash,                 METH_VARARGS},
    {"codec_incrementalencoder",
     (PyCFunction)codec_incrementalencoder,                      METH_VARARGS},
    {"codec_incrementaldecoder",
     (PyCFunction)codec_incrementaldecoder,                      METH_VARARGS},
    {"test_s_code",             (PyCFunction)test_s_code,        METH_NOARGS},
    {"test_u_code",             (PyCFunction)test_u_code,        METH_NOARGS},
    {"test_Z_code",             (PyCFunction)test_Z_code,        METH_NOARGS},
    {"test_widechar",           (PyCFunction)test_widechar,      METH_NOARGS},
    {"unicode_aswidechar",      unicode_aswidechar,              METH_VARARGS},
    {"unicode_aswidecharstring",unicode_aswidecharstring,        METH_VARARGS},
    {"unicode_asucs4",          unicode_asucs4,                  METH_VARARGS},
    {"unicode_findchar",        unicode_findchar,                METH_VARARGS},
    {"unicode_copycharacters",  unicode_copycharacters,          METH_VARARGS},
    {"unicode_encodedecimal",   unicode_encodedecimal,           METH_VARARGS},
    {"unicode_transformdecimaltoascii", unicode_transformdecimaltoascii, METH_VARARGS},
    {"unicode_legacy_string",   unicode_legacy_string,           METH_VARARGS},
    {"_test_thread_state",      test_thread_state,               METH_VARARGS},
    {"_pending_threadfunc",     pending_threadfunc,              METH_VARARGS},
#ifdef HAVE_GETTIMEOFDAY
    {"profile_int",             profile_int,                     METH_NOARGS},
#endif
    {"traceback_print",         traceback_print,                 METH_VARARGS},
    {"exception_print",         exception_print,                 METH_VARARGS},
    {"set_exc_info",            test_set_exc_info,               METH_VARARGS},
    {"argparsing",              argparsing,                      METH_VARARGS},
    {"code_newempty",           code_newempty,                   METH_VARARGS},
    {"make_exception_with_doc", (PyCFunction)make_exception_with_doc,
     METH_VARARGS | METH_KEYWORDS},
    {"make_memoryview_from_NULL_pointer", (PyCFunction)make_memoryview_from_NULL_pointer,
     METH_NOARGS},
    {"crash_no_current_thread", (PyCFunction)crash_no_current_thread, METH_NOARGS},
    {"run_in_subinterp",        run_in_subinterp,                METH_VARARGS},
    {"pytime_object_to_time_t", test_pytime_object_to_time_t,  METH_VARARGS},
    {"pytime_object_to_timeval", test_pytime_object_to_timeval,  METH_VARARGS},
    {"pytime_object_to_timespec", test_pytime_object_to_timespec,  METH_VARARGS},
    {"with_tp_del",             with_tp_del,                     METH_VARARGS},
    {"create_cfunction",        create_cfunction,                METH_NOARGS},
    {"test_pymem_alloc0",
     (PyCFunction)test_pymem_alloc0, METH_NOARGS},
    {"test_pymem_setrawallocators",
     (PyCFunction)test_pymem_setrawallocators, METH_NOARGS},
    {"test_pymem_setallocators",
     (PyCFunction)test_pymem_setallocators, METH_NOARGS},
    {"test_pyobject_setallocators",
     (PyCFunction)test_pyobject_setallocators, METH_NOARGS},
    {"set_nomemory", (PyCFunction)set_nomemory, METH_VARARGS,
     PyDoc_STR("set_nomemory(start:int, stop:int = 0)")},
    {"remove_mem_hooks", (PyCFunction)remove_mem_hooks, METH_NOARGS,
     PyDoc_STR("Remove memory hooks.")},
    {"no_docstring",
        (PyCFunction)test_with_docstring, METH_NOARGS},
    {"docstring_empty",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_empty},
    {"docstring_no_signature",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_no_signature},
    {"docstring_with_invalid_signature",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_invalid_signature},
    {"docstring_with_invalid_signature2",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_invalid_signature2},
    {"docstring_with_signature",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature},
    {"docstring_with_signature_but_no_doc",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature_but_no_doc},
    {"docstring_with_signature_and_extra_newlines",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature_and_extra_newlines},
    {"docstring_with_signature_with_defaults",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature_with_defaults},
    {"raise_signal",
     (PyCFunction)test_raise_signal, METH_VARARGS},
    {"call_in_temporary_c_thread", call_in_temporary_c_thread, METH_O,
     PyDoc_STR("set_error_class(error_class) -> None")},
    {"pymarshal_write_long_to_file",
        pymarshal_write_long_to_file, METH_VARARGS},
    {"pymarshal_write_object_to_file",
        pymarshal_write_object_to_file, METH_VARARGS},
    {"pymarshal_read_short_from_file",
        pymarshal_read_short_from_file, METH_VARARGS},
    {"pymarshal_read_long_from_file",
        pymarshal_read_long_from_file, METH_VARARGS},
    {"pymarshal_read_last_object_from_file",
        pymarshal_read_last_object_from_file, METH_VARARGS},
    {"pymarshal_read_object_from_file",
        pymarshal_read_object_from_file, METH_VARARGS},
    {"return_null_without_error",
        return_null_without_error, METH_NOARGS},
    {"return_result_with_error",
        return_result_with_error, METH_NOARGS},
    {"PyTime_FromSeconds", test_pytime_fromseconds,  METH_VARARGS},
    {"PyTime_FromSecondsObject", test_pytime_fromsecondsobject,  METH_VARARGS},
    {"PyTime_AsSecondsDouble", test_pytime_assecondsdouble, METH_VARARGS},
    {"PyTime_AsTimeval", test_PyTime_AsTimeval, METH_VARARGS},
#ifdef HAVE_CLOCK_GETTIME
    {"PyTime_AsTimespec", test_PyTime_AsTimespec, METH_VARARGS},
#endif
    {"PyTime_AsMilliseconds", test_PyTime_AsMilliseconds, METH_VARARGS},
    {"PyTime_AsMicroseconds", test_PyTime_AsMicroseconds, METH_VARARGS},
    {"get_recursion_depth", get_recursion_depth, METH_NOARGS},
    {"pymem_buffer_overflow", pymem_buffer_overflow, METH_NOARGS},
    {"pymem_api_misuse", pymem_api_misuse, METH_NOARGS},
    {"pymem_malloc_without_gil", pymem_malloc_without_gil, METH_NOARGS},
    {"pymem_getallocatorsname", test_pymem_getallocatorsname, METH_NOARGS},
    {"pyobject_malloc_without_gil", pyobject_malloc_without_gil, METH_NOARGS},
    {"tracemalloc_track", tracemalloc_track, METH_VARARGS},
    {"tracemalloc_untrack", tracemalloc_untrack, METH_VARARGS},
    {"tracemalloc_get_traceback", tracemalloc_get_traceback, METH_VARARGS},
    {"dict_get_version", dict_get_version, METH_VARARGS},
    {"raise_SIGINT_then_send_None", raise_SIGINT_then_send_None, METH_VARARGS},
    {"pyobject_fastcall", test_pyobject_fastcall, METH_VARARGS},
    {"pyobject_fastcalldict", test_pyobject_fastcalldict, METH_VARARGS},
    {"pyobject_fastcallkeywords", test_pyobject_fastcallkeywords, METH_VARARGS},
    {"stack_pointer", stack_pointer, METH_NOARGS},
#ifdef W_STOPCODE
    {"W_STOPCODE", py_w_stopcode, METH_VARARGS},
#endif
    {"get_mapping_keys", get_mapping_keys, METH_O},
    {"get_mapping_values", get_mapping_values, METH_O},
    {"get_mapping_items", get_mapping_items, METH_O},
    {"test_pythread_tss_key_state", test_pythread_tss_key_state, METH_VARARGS},
    {"hamt", new_hamt, METH_NOARGS},
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
    long long longlong_member;
    unsigned long long ulonglong_member;
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
    {"T_LONGLONG", T_LONGLONG, offsetof(test_structmembers, structmembers.longlong_member), 0, NULL},
    {"T_ULONGLONG", T_ULONGLONG, offsetof(test_structmembers, structmembers.ulonglong_member), 0, NULL},
    {NULL}
};


static PyObject *
test_structmembers_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {
        "T_BOOL", "T_BYTE", "T_UBYTE", "T_SHORT", "T_USHORT",
        "T_INT", "T_UINT", "T_LONG", "T_ULONG", "T_PYSSIZET",
        "T_FLOAT", "T_DOUBLE", "T_STRING_INPLACE",
        "T_LONGLONG", "T_ULONGLONG",
        NULL};
    static const char fmt[] = "|bbBhHiIlknfds#LK";
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
                                     , &ob->structmembers.longlong_member,
                                     &ob->structmembers.ulonglong_member
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


typedef struct {
    PyObject_HEAD
} matmulObject;

static PyObject *
matmulType_matmul(PyObject *self, PyObject *other)
{
    return Py_BuildValue("(sOO)", "matmul", self, other);
}

static PyObject *
matmulType_imatmul(PyObject *self, PyObject *other)
{
    return Py_BuildValue("(sOO)", "imatmul", self, other);
}

static void
matmulType_dealloc(PyObject *self)
{
    Py_TYPE(self)->tp_free(self);
}

static PyNumberMethods matmulType_as_number = {
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_remainde r*/
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* tp_positive */
    0,                          /* tp_absolute */
    0,                          /* tp_bool */
    0,                          /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    0,                          /* nb_and */
    0,                          /* nb_xor */
    0,                          /* nb_or */
    0,                          /* nb_int */
    0,                          /* nb_reserved */
    0,                          /* nb_float */
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                          /* nb_floor_divide */
    0,                          /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    0,                          /* nb_index */
    matmulType_matmul,        /* nb_matrix_multiply */
    matmulType_imatmul        /* nb_matrix_inplace_multiply */
};

static PyTypeObject matmulType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "matmulType",
    sizeof(matmulObject),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    matmulType_dealloc,                 /* destructor tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    &matmulType_as_number,              /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    0,                                  /* tp_flags */
    "C level type with matrix operations defined",
    0,                                  /* traverseproc tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    PyType_GenericNew,                  /* tp_new */
    PyObject_Del,                       /* tp_free */
};


typedef struct {
    PyObject_HEAD
    PyObject *ao_iterator;
} awaitObject;


static PyObject *
awaitObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *v;
    awaitObject *ao;

    if (!PyArg_UnpackTuple(args, "awaitObject", 1, 1, &v))
        return NULL;

    ao = (awaitObject *)type->tp_alloc(type, 0);
    if (ao == NULL) {
        return NULL;
    }

    Py_INCREF(v);
    ao->ao_iterator = v;

    return (PyObject *)ao;
}


static void
awaitObject_dealloc(awaitObject *ao)
{
    Py_CLEAR(ao->ao_iterator);
    Py_TYPE(ao)->tp_free(ao);
}


static PyObject *
awaitObject_await(awaitObject *ao)
{
    Py_INCREF(ao->ao_iterator);
    return ao->ao_iterator;
}

static PyAsyncMethods awaitType_as_async = {
    (unaryfunc)awaitObject_await,           /* am_await */
    0,                                      /* am_aiter */
    0                                       /* am_anext */
};


static PyTypeObject awaitType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "awaitType",
    sizeof(awaitObject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)awaitObject_dealloc,    /* destructor tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    &awaitType_as_async,                /* tp_as_async */
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
    "C level type with tp_as_async",
    0,                                  /* traverseproc tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    awaitObject_new,                    /* tp_new */
    PyObject_Del,                       /* tp_free */
};


static int recurse_infinitely_error_init(PyObject *, PyObject *, PyObject *);

static PyTypeObject PyRecursingInfinitelyError_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "RecursingInfinitelyError",   /* tp_name */
    sizeof(PyBaseExceptionObject), /* tp_basicsize */
    0,                          /* tp_itemsize */
    0,                          /* tp_dealloc */
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
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Instantiating this exception starts infinite recursion.", /* tp_doc */
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
    (initproc)recurse_infinitely_error_init, /* tp_init */
    0,                          /* tp_alloc */
    0,                          /* tp_new */
};

static int
recurse_infinitely_error_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *type = (PyObject *)&PyRecursingInfinitelyError_Type;

    /* Instantiating this exception starts infinite recursion. */
    Py_INCREF(type);
    PyErr_SetObject(type, NULL);
    return -1;
}


/* Test PEP 560 */

typedef struct {
    PyObject_HEAD
    PyObject *item;
} PyGenericAliasObject;

static void
generic_alias_dealloc(PyGenericAliasObject *self)
{
    Py_CLEAR(self->item);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
generic_alias_mro_entries(PyGenericAliasObject *self, PyObject *bases)
{
    return PyTuple_Pack(1, self->item);
}

static PyMethodDef generic_alias_methods[] = {
    {"__mro_entries__", (PyCFunction) generic_alias_mro_entries, METH_O, NULL},
    {NULL}  /* sentinel */
};

PyTypeObject GenericAlias_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "GenericAlias",
    sizeof(PyGenericAliasObject),
    0,
    .tp_dealloc = (destructor)generic_alias_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = generic_alias_methods,
};

static PyObject *
generic_alias_new(PyObject *item)
{
    PyGenericAliasObject *o = PyObject_New(PyGenericAliasObject, &GenericAlias_Type);
    if (o == NULL) {
        return NULL;
    }
    Py_INCREF(item);
    o->item = item;
    return (PyObject*) o;
}

typedef struct {
    PyObject_HEAD
} PyGenericObject;

static PyObject *
generic_class_getitem(PyObject *type, PyObject *item)
{
    return generic_alias_new(item);
}

static PyMethodDef generic_methods[] = {
    {"__class_getitem__", generic_class_getitem, METH_O|METH_CLASS, NULL},
    {NULL}  /* sentinel */
};

PyTypeObject Generic_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Generic",
    sizeof(PyGenericObject),
    0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = generic_methods,
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

/* Per PEP 489, this module will not be converted to multi-phase initialization
 */

PyMODINIT_FUNC
PyInit__testcapi(void)
{
    PyObject *m;

    m = PyModule_Create(&_testcapimodule);
    if (m == NULL)
        return NULL;

    Py_TYPE(&_HashInheritanceTester_Type)=&PyType_Type;

    Py_TYPE(&test_structmembersType)=&PyType_Type;
    Py_INCREF(&test_structmembersType);
    /* don't use a name starting with "test", since we don't want
       test_capi to automatically call this */
    PyModule_AddObject(m, "_test_structmembersType", (PyObject *)&test_structmembersType);
    if (PyType_Ready(&matmulType) < 0)
        return NULL;
    Py_INCREF(&matmulType);
    PyModule_AddObject(m, "matmulType", (PyObject *)&matmulType);

    if (PyType_Ready(&awaitType) < 0)
        return NULL;
    Py_INCREF(&awaitType);
    PyModule_AddObject(m, "awaitType", (PyObject *)&awaitType);

    if (PyType_Ready(&GenericAlias_Type) < 0)
        return NULL;
    Py_INCREF(&GenericAlias_Type);
    PyModule_AddObject(m, "GenericAlias", (PyObject *)&GenericAlias_Type);

    if (PyType_Ready(&Generic_Type) < 0)
        return NULL;
    Py_INCREF(&Generic_Type);
    PyModule_AddObject(m, "Generic", (PyObject *)&Generic_Type);

    PyRecursingInfinitelyError_Type.tp_base = (PyTypeObject *)PyExc_Exception;
    if (PyType_Ready(&PyRecursingInfinitelyError_Type) < 0) {
        return NULL;
    }
    Py_INCREF(&PyRecursingInfinitelyError_Type);
    PyModule_AddObject(m, "RecursingInfinitelyError",
                       (PyObject *)&PyRecursingInfinitelyError_Type);

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
    PyModule_AddObject(m, "SIZEOF_TIME_T", PyLong_FromSsize_t(sizeof(time_t)));
    Py_INCREF(&PyInstanceMethod_Type);
    PyModule_AddObject(m, "instancemethod", (PyObject *)&PyInstanceMethod_Type);

    PyModule_AddIntConstant(m, "the_number_three", 3);
#ifdef WITH_PYMALLOC
    PyModule_AddObject(m, "WITH_PYMALLOC", Py_True);
#else
    PyModule_AddObject(m, "WITH_PYMALLOC", Py_False);
#endif

    TestError = PyErr_NewException("_testcapi.error", NULL, NULL);
    Py_INCREF(TestError);
    PyModule_AddObject(m, "error", TestError);
    return m;
}
