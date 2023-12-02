#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "parts.h"
#include "util.h"
#include "clinic/long.c.h"

/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/


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

// Test PyLong_FromLong()/PyLong_AsLong()
// and PyLong_FromUnsignedLong()/PyLong_AsUnsignedLong().

#define TESTNAME        test_long_api_inner
#define TYPENAME        long
#define F_S_TO_PY       PyLong_FromLong
#define F_PY_TO_S       PyLong_AsLong
#define F_U_TO_PY       PyLong_FromUnsignedLong
#define F_PY_TO_U       PyLong_AsUnsignedLong

#include "testcapi_long.h"

/*[clinic input]
_testcapi.test_long_api
[clinic start generated code]*/

static PyObject *
_testcapi_test_long_api_impl(PyObject *module)
/*[clinic end generated code: output=4405798ca1e9f444 input=e9b8880d7331c688]*/
{
    return TESTNAME(raise_test_long_error);
}

#undef TESTNAME
#undef TYPENAME
#undef F_S_TO_PY
#undef F_PY_TO_S
#undef F_U_TO_PY
#undef F_PY_TO_U

// Test PyLong_FromLongLong()/PyLong_AsLongLong()
// and PyLong_FromUnsignedLongLong()/PyLong_AsUnsignedLongLong().

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

/*[clinic input]
_testcapi.test_longlong_api
[clinic start generated code]*/

static PyObject *
_testcapi_test_longlong_api_impl(PyObject *module)
/*[clinic end generated code: output=2b3414ba8c31dfe6 input=ccbb2a48c2b3c4a5]*/
{
    return TESTNAME(raise_test_longlong_error);
}

#undef TESTNAME
#undef TYPENAME
#undef F_S_TO_PY
#undef F_PY_TO_S
#undef F_U_TO_PY
#undef F_PY_TO_U


/*[clinic input]
_testcapi.test_long_and_overflow

Test the PyLong_AsLongAndOverflow API.

General conversion to PY_LONG is tested by test_long_api_inner.
This test will concentrate on proper handling of overflow.
[clinic start generated code]*/

static PyObject *
_testcapi_test_long_and_overflow_impl(PyObject *module)
/*[clinic end generated code: output=f8460ca115e31d8e input=762f6b62da0a3cdc]*/
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

/*[clinic input]
_testcapi.test_long_long_and_overflow

Test the PyLong_AsLongLongAndOverflow API.

General conversion to long long is tested by test_long_api_inner.
This test will concentrate on proper handling of overflow.
[clinic start generated code]*/

static PyObject *
_testcapi_test_long_long_and_overflow_impl(PyObject *module)
/*[clinic end generated code: output=0b92330786f45483 input=544bb0aefe5e8a9e]*/
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

/*[clinic input]
_testcapi.test_long_as_size_t

Test the PyLong_As{Size,Ssize}_t API.

At present this just tests that non-integer arguments are handled correctly.
It should be extended to test overflow handling.
[clinic start generated code]*/

static PyObject *
_testcapi_test_long_as_size_t_impl(PyObject *module)
/*[clinic end generated code: output=f6490ea2b41e6173 input=922990c4a3edfb0d]*/
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

/*[clinic input]
_testcapi.test_long_as_unsigned_long_long_mask
[clinic start generated code]*/

static PyObject *
_testcapi_test_long_as_unsigned_long_long_mask_impl(PyObject *module)
/*[clinic end generated code: output=e3e16cd0189440cc input=eb2438493ae7b9af]*/
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

/*[clinic input]
_testcapi.test_long_as_double
[clinic start generated code]*/

static PyObject *
_testcapi_test_long_as_double_impl(PyObject *module)
/*[clinic end generated code: output=deca0898e15adde5 input=c77bc88ef5a1de76]*/
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

/*[clinic input]
_testcapi.call_long_compact_api
    arg: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_call_long_compact_api(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=7e3894f611b1b2b7 input=87b87396967af14c]*/

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

/*[clinic input]
_testcapi.PyLong_AsInt
    arg: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_PyLong_AsInt(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=0df9f19de5fa575b input=9561b97105493a67]*/
{
    NULLABLE(arg);
    assert(!PyErr_Occurred());
    int value = PyLong_AsInt(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLong(value);
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

static PyMethodDef test_methods[] = {
    _TESTCAPI_TEST_LONG_AND_OVERFLOW_METHODDEF
    _TESTCAPI_TEST_LONG_API_METHODDEF
    _TESTCAPI_TEST_LONG_AS_DOUBLE_METHODDEF
    _TESTCAPI_TEST_LONG_AS_SIZE_T_METHODDEF
    _TESTCAPI_TEST_LONG_AS_UNSIGNED_LONG_LONG_MASK_METHODDEF
    _TESTCAPI_TEST_LONG_LONG_AND_OVERFLOW_METHODDEF
    _TESTCAPI_TEST_LONGLONG_API_METHODDEF
    _TESTCAPI_CALL_LONG_COMPACT_API_METHODDEF
    {"pylong_check",                pylong_check,               METH_O},
    {"pylong_checkexact",           pylong_checkexact,          METH_O},
    {"pylong_fromdouble",           pylong_fromdouble,          METH_O},
    {"pylong_fromstring",           pylong_fromstring,          METH_VARARGS},
    {"pylong_fromunicodeobject",    pylong_fromunicodeobject,   METH_VARARGS},
    {"pylong_fromvoidptr",          pylong_fromvoidptr,         METH_O},
    _TESTCAPI_PYLONG_ASINT_METHODDEF
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
