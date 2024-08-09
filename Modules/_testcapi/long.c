#define PY_SSIZE_T_CLEAN
#include "parts.h"
#include "util.h"


static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
    PyErr_Format(PyExc_AssertionError, "%s: %s", test_name, msg);
    return NULL;
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
test_long_api(PyObject* self, PyObject *Py_UNUSED(ignored))
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
test_long_and_overflow(PyObject *self, PyObject *Py_UNUSED(ignored))
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
    Py_SETREF(num, temp);
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
    Py_SETREF(num, temp);
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
test_long_long_and_overflow(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *num, *one, *temp;
    long long value;
    int overflow;

    /* Test that overflow is set properly for a large value. */
    /* num is a number larger than LLONG_MAX on a typical machine. */
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

    /* Same again, with num = LLONG_MAX + 1 */
    num = PyLong_FromLongLong(LLONG_MAX);
    if (num == NULL)
        return NULL;
    one = PyLong_FromLong(1L);
    if (one == NULL) {
        Py_DECREF(num);
        return NULL;
    }
    temp = PyNumber_Add(num, one);
    Py_DECREF(one);
    Py_SETREF(num, temp);
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
    /* num is a number smaller than LLONG_MIN on a typical platform */
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

    /* Same again, with num = LLONG_MIN - 1 */
    num = PyLong_FromLongLong(LLONG_MIN);
    if (num == NULL)
        return NULL;
    one = PyLong_FromLong(1L);
    if (one == NULL) {
        Py_DECREF(num);
        return NULL;
    }
    temp = PyNumber_Subtract(num, one);
    Py_DECREF(one);
    Py_SETREF(num, temp);
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

    num = PyLong_FromLongLong(LLONG_MAX);
    if (num == NULL)
        return NULL;
    overflow = 1234;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != LLONG_MAX)
        return raiseTestError("test_long_long_and_overflow",
            "expected return value LLONG_MAX");
    if (overflow != 0)
        return raiseTestError("test_long_long_and_overflow",
            "overflow was not cleared");

    num = PyLong_FromLongLong(LLONG_MIN);
    if (num == NULL)
        return NULL;
    overflow = 0;
    value = PyLong_AsLongLongAndOverflow(num, &overflow);
    Py_DECREF(num);
    if (value == -1 && PyErr_Occurred())
        return NULL;
    if (value != LLONG_MIN)
        return raiseTestError("test_long_long_and_overflow",
            "expected return value LLONG_MIN");
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
test_long_as_size_t(PyObject *self, PyObject *Py_UNUSED(ignored))
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

static PyObject *
test_long_as_unsigned_long_long_mask(PyObject *self,
                                     PyObject *Py_UNUSED(ignored))
{
    unsigned long long res = PyLong_AsUnsignedLongLongMask(NULL);

    if (res != (unsigned long long)-1 || !PyErr_Occurred()) {
        return raiseTestError("test_long_as_unsigned_long_long_mask",
                              "PyLong_AsUnsignedLongLongMask(NULL) didn't "
                              "complain");
    }
    if (!PyErr_ExceptionMatches(PyExc_SystemError)) {
        return raiseTestError("test_long_as_unsigned_long_long_mask",
                              "PyLong_AsUnsignedLongLongMask(NULL) raised "
                              "something other than SystemError");
    }
    PyErr_Clear();
    Py_RETURN_NONE;
}

/* Test the PyLong_AsDouble API. At present this just tests that
   non-integer arguments are handled correctly.
 */

static PyObject *
test_long_as_double(PyObject *self, PyObject *Py_UNUSED(ignored))
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

/* Simple test of _PyLong_NumBits and _PyLong_Sign. */
static PyObject *
test_long_numbits(PyObject *self, PyObject *Py_UNUSED(ignored))
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

static PyObject *
check_long_compact_api(PyObject *self, PyObject *arg)
{
    assert(PyLong_Check(arg));
    int is_compact = PyUnstable_Long_IsCompact((PyLongObject*)arg);
    Py_ssize_t value = -1;
    if (is_compact) {
        value = PyUnstable_Long_CompactValue((PyLongObject*)arg);
    }
    return Py_BuildValue("in", is_compact, value);
}

static PyObject *
pylong_check(PyObject *module, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyLong_Check(obj));
}

static PyObject *
pylong_checkexact(PyObject *module, PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PyLong_CheckExact(obj));
}

static PyObject *
pylong_fromdouble(PyObject *module, PyObject *arg)
{
    double value;
    if (!PyArg_Parse(arg, "d", &value)) {
        return NULL;
    }
    return PyLong_FromDouble(value);
}

static PyObject *
pylong_fromstring(PyObject *module, PyObject *args)
{
    const char *str;
    Py_ssize_t len;
    int base;
    char *end = UNINITIALIZED_PTR;
    if (!PyArg_ParseTuple(args, "z#i", &str, &len, &base)) {
        return NULL;
    }

    PyObject *result = PyLong_FromString(str, &end, base);
    if (result == NULL) {
        // XXX 'end' is not always set.
        return NULL;
    }
    return Py_BuildValue("Nn", result, (Py_ssize_t)(end - str));
}

static PyObject *
pylong_fromunicodeobject(PyObject *module, PyObject *args)
{
    PyObject *unicode;
    int base;
    if (!PyArg_ParseTuple(args, "Oi", &unicode, &base)) {
        return NULL;
    }

    NULLABLE(unicode);
    return PyLong_FromUnicodeObject(unicode, base);
}

static PyObject *
pylong_fromvoidptr(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    return PyLong_FromVoidPtr((void *)arg);
}

static PyObject *
pylong_aslong(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    long value = PyLong_AsLong(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLong(value);
}

static PyObject *
pylong_aslongandoverflow(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    int overflow = UNINITIALIZED_INT;
    long value = PyLong_AsLongAndOverflow(arg, &overflow);
    if (value == -1 && PyErr_Occurred()) {
        assert(overflow == -1);
        return NULL;
    }
    return Py_BuildValue("li", value, overflow);
}

static PyObject *
pylong_asunsignedlong(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    unsigned long value = PyLong_AsUnsignedLong(arg);
    if (value == (unsigned long)-1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromUnsignedLong(value);
}

static PyObject *
pylong_asunsignedlongmask(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    unsigned long value = PyLong_AsUnsignedLongMask(arg);
    if (value == (unsigned long)-1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromUnsignedLong(value);
}

static PyObject *
pylong_aslonglong(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    long long value = PyLong_AsLongLong(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLongLong(value);
}

static PyObject *
pylong_aslonglongandoverflow(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    int overflow = UNINITIALIZED_INT;
    long long value = PyLong_AsLongLongAndOverflow(arg, &overflow);
    if (value == -1 && PyErr_Occurred()) {
        assert(overflow == -1);
        return NULL;
    }
    return Py_BuildValue("Li", value, overflow);
}

static PyObject *
pylong_asunsignedlonglong(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    unsigned long long value = PyLong_AsUnsignedLongLong(arg);
    if (value == (unsigned long long)-1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromUnsignedLongLong(value);
}

static PyObject *
pylong_asunsignedlonglongmask(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    unsigned long long value = PyLong_AsUnsignedLongLongMask(arg);
    if (value == (unsigned long long)-1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromUnsignedLongLong(value);
}

static PyObject *
pylong_as_ssize_t(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    Py_ssize_t value = PyLong_AsSsize_t(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromSsize_t(value);
}

static PyObject *
pylong_as_size_t(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    size_t value = PyLong_AsSize_t(arg);
    if (value == (size_t)-1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromSize_t(value);
}

static PyObject *
pylong_asdouble(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    double value = PyLong_AsDouble(arg);
    if (value == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    return PyFloat_FromDouble(value);
}

static PyObject *
pylong_asvoidptr(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    void *value = PyLong_AsVoidPtr(arg);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        Py_RETURN_NONE;
    }
    return Py_NewRef((PyObject *)value);
}

static PyObject *
pylong_aspid(PyObject *module, PyObject *arg)
{
    NULLABLE(arg);
    pid_t value = PyLong_AsPid(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromPid(value);
}


static PyMethodDef test_methods[] = {
    {"test_long_and_overflow",  test_long_and_overflow,          METH_NOARGS},
    {"test_long_api",           test_long_api,                   METH_NOARGS},
    {"test_long_as_double",     test_long_as_double,             METH_NOARGS},
    {"test_long_as_size_t",     test_long_as_size_t,             METH_NOARGS},
    {"test_long_as_unsigned_long_long_mask", test_long_as_unsigned_long_long_mask, METH_NOARGS},
    {"test_long_long_and_overflow",test_long_long_and_overflow,  METH_NOARGS},
    {"test_long_numbits",       test_long_numbits,               METH_NOARGS},
    {"test_longlong_api",       test_longlong_api,               METH_NOARGS},
    {"call_long_compact_api",   check_long_compact_api,          METH_O},
    {"pylong_check",                pylong_check,               METH_O},
    {"pylong_checkexact",           pylong_checkexact,          METH_O},
    {"pylong_fromdouble",           pylong_fromdouble,          METH_O},
    {"pylong_fromstring",           pylong_fromstring,          METH_VARARGS},
    {"pylong_fromunicodeobject",    pylong_fromunicodeobject,   METH_VARARGS},
    {"pylong_fromvoidptr",          pylong_fromvoidptr,         METH_O},
    {"pylong_aslong",               pylong_aslong,              METH_O},
    {"pylong_aslongandoverflow",    pylong_aslongandoverflow,   METH_O},
    {"pylong_asunsignedlong",       pylong_asunsignedlong,      METH_O},
    {"pylong_asunsignedlongmask",   pylong_asunsignedlongmask,  METH_O},
    {"pylong_aslonglong",           pylong_aslonglong,          METH_O},
    {"pylong_aslonglongandoverflow", pylong_aslonglongandoverflow, METH_O},
    {"pylong_asunsignedlonglong",   pylong_asunsignedlonglong,  METH_O},
    {"pylong_asunsignedlonglongmask", pylong_asunsignedlonglongmask, METH_O},
    {"pylong_as_ssize_t",           pylong_as_ssize_t,          METH_O},
    {"pylong_as_size_t",            pylong_as_size_t,           METH_O},
    {"pylong_asdouble",             pylong_asdouble,            METH_O},
    {"pylong_asvoidptr",            pylong_asvoidptr,           METH_O},
    {"pylong_aspid",                pylong_aspid,               METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Long(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
