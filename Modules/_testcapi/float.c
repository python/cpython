// clinic/float.c.h uses internal pycore_modsupport.h API
#define PYTESTCAPI_NEED_INTERNAL_API

#include "parts.h"
#include "util.h"
#include "clinic/float.c.h"


/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/

/*[clinic input]
_testcapi.float_pack

    size: int
    d: double
    le: int
    /

Test PyFloat_Pack2(), PyFloat_Pack4() and PyFloat_Pack8()
[clinic start generated code]*/

static PyObject *
_testcapi_float_pack_impl(PyObject *module, int size, double d, int le)
/*[clinic end generated code: output=7899bd98f8b6cb04 input=52c9115121999c98]*/
{
    switch (size)
    {
    case 2:
    {
        char data[2];
        if (PyFloat_Pack2(d, data, le) < 0) {
            return NULL;
        }
        return PyBytes_FromStringAndSize(data, Py_ARRAY_LENGTH(data));
    }
    case 4:
    {
        char data[4];
        if (PyFloat_Pack4(d, data, le) < 0) {
            return NULL;
        }
        return PyBytes_FromStringAndSize(data, Py_ARRAY_LENGTH(data));
    }
    case 8:
    {
        char data[8];
        if (PyFloat_Pack8(d, data, le) < 0) {
            return NULL;
        }
        return PyBytes_FromStringAndSize(data, Py_ARRAY_LENGTH(data));
    }
    default: break;
    }

    PyErr_SetString(PyExc_ValueError, "size must 2, 4 or 8");
    return NULL;
}


/*[clinic input]
_testcapi.float_unpack

    data: str(accept={robuffer}, zeroes=True)
    le: int
    /

Test PyFloat_Unpack2(), PyFloat_Unpack4() and PyFloat_Unpack8()
[clinic start generated code]*/

static PyObject *
_testcapi_float_unpack_impl(PyObject *module, const char *data,
                            Py_ssize_t data_length, int le)
/*[clinic end generated code: output=617059f889ddbfe4 input=c095e4bb75a696cd]*/
{
    assert(!PyErr_Occurred());
    double d;
    switch (data_length)
    {
    case 2:
        d = PyFloat_Unpack2(data, le);
        break;
    case 4:
        d = PyFloat_Unpack4(data, le);
        break;
    case 8:
        d = PyFloat_Unpack8(data, le);
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "data length must 2, 4 or 8 bytes");
        return NULL;
    }

    if (d == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    return PyFloat_FromDouble(d);
}


/* Test PyOS_string_to_double. */
static PyObject *
test_string_to_double(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    double result;
    const char *msg;

#define CHECK_STRING(STR, expected) \
    do { \
        result = PyOS_string_to_double(STR, NULL, NULL); \
        if (result == -1.0 && PyErr_Occurred()) { \
            return NULL; \
        } \
        if (result != (double)expected) { \
            msg = "conversion of " STR " to float failed"; \
            goto fail; \
        } \
    } while (0)

#define CHECK_INVALID(STR) \
    do { \
        result = PyOS_string_to_double(STR, NULL, NULL); \
        if (result == -1.0 && PyErr_Occurred()) { \
            if (PyErr_ExceptionMatches(PyExc_ValueError)) { \
                PyErr_Clear(); \
            } \
            else { \
                return NULL; \
            } \
        } \
        else { \
            msg = "conversion of " STR " didn't raise ValueError"; \
            goto fail; \
        } \
    } while (0)

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
    PyErr_Format(PyExc_AssertionError, "test_string_to_double: %s", msg);
    return NULL;
#undef CHECK_STRING
#undef CHECK_INVALID
}


static PyMethodDef test_methods[] = {
    _TESTCAPI_FLOAT_PACK_METHODDEF
    _TESTCAPI_FLOAT_UNPACK_METHODDEF
    {"test_string_to_double", test_string_to_double, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Float(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
