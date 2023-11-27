/*
 * Tests for Python/getargs.c and Python/modsupport.c;
 * APIs that parse and build arguments.
 */

#define PY_SSIZE_T_CLEAN

#include "parts.h"

static PyObject *
parse_tuple_and_keywords(PyObject *self, PyObject *args)
{
    PyObject *sub_args;
    PyObject *sub_kwargs;
    const char *sub_format;
    PyObject *sub_keywords;

#define MAX_PARAMS 8
    double buffers[MAX_PARAMS][4]; /* double ensures alignment where necessary */
    char *keywords[MAX_PARAMS + 1]; /* space for NULL at end */

    PyObject *return_value = NULL;

    if (!PyArg_ParseTuple(args, "OOsO:parse_tuple_and_keywords",
                          &sub_args, &sub_kwargs, &sub_format, &sub_keywords))
    {
        return NULL;
    }

    if (!(PyList_CheckExact(sub_keywords) ||
        PyTuple_CheckExact(sub_keywords)))
    {
        PyErr_SetString(PyExc_ValueError,
            "parse_tuple_and_keywords: "
            "sub_keywords must be either list or tuple");
        return NULL;
    }

    memset(buffers, 0, sizeof(buffers));
    memset(keywords, 0, sizeof(keywords));

    Py_ssize_t size = PySequence_Fast_GET_SIZE(sub_keywords);
    if (size > MAX_PARAMS) {
        PyErr_SetString(PyExc_ValueError,
            "parse_tuple_and_keywords: too many keywords in sub_keywords");
        goto exit;
    }

    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject *o = PySequence_Fast_GET_ITEM(sub_keywords, i);
        if (PyUnicode_Check(o)) {
            keywords[i] = (char *)PyUnicode_AsUTF8(o);
            if (keywords[i] == NULL) {
                goto exit;
            }
        }
        else if (PyBytes_Check(o)) {
            keywords[i] = PyBytes_AS_STRING(o);
        }
        else {
            PyErr_Format(PyExc_ValueError,
                "parse_tuple_and_keywords: "
                "keywords must be str or bytes", i);
            goto exit;
        }
    }

    assert(MAX_PARAMS == 8);
    int result = PyArg_ParseTupleAndKeywords(sub_args, sub_kwargs,
        sub_format, keywords,
        buffers + 0, buffers + 1, buffers + 2, buffers + 3,
        buffers + 4, buffers + 5, buffers + 6, buffers + 7);

    if (result) {
        int objects_only = 1;
        int count = 0;
        for (const char *f = sub_format; *f; f++) {
            if (Py_ISALNUM(*f)) {
                if (strchr("OSUY", *f) == NULL) {
                    objects_only = 0;
                    break;
                }
                count++;
            }
        }
        if (objects_only) {
            return_value = PyTuple_New(count);
            if (return_value == NULL) {
                goto exit;
            }
            for (Py_ssize_t i = 0; i < count; i++) {
                PyObject *arg = *(PyObject **)(buffers + i);
                if (arg == NULL) {
                    arg = Py_None;
                }
                PyTuple_SET_ITEM(return_value, i, Py_NewRef(arg));
            }
        }
        else {
            return_value = Py_NewRef(Py_None);
        }
    }

exit:
    return return_value;
}

static PyObject *
get_args(PyObject *self, PyObject *args)
{
    if (args == NULL) {
        args = Py_None;
    }
    return Py_NewRef(args);
}

static PyObject *
get_kwargs(PyObject *self, PyObject *args, PyObject *kwargs)
{
    if (kwargs == NULL) {
        kwargs = Py_None;
    }
    return Py_NewRef(kwargs);
}

static PyObject *
getargs_w_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;

    if (!PyArg_ParseTuple(args, "w*:getargs_w_star", &buffer)) {
        return NULL;
    }

    if (2 <= buffer.len) {
        char *str = buffer.buf;
        str[0] = '[';
        str[buffer.len-1] = ']';
    }

    PyObject *result = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    PyBuffer_Release(&buffer);
    return result;
}

static PyObject *
test_empty_argparse(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    /* Test that formats can begin with '|'. See issue #4720. */
    PyObject *dict = NULL;
    static char *kwlist[] = {NULL};
    PyObject *tuple = PyTuple_New(0);
    if (!tuple) {
        return NULL;
    }
    int result;
    if (!(result = PyArg_ParseTuple(tuple, "|:test_empty_argparse"))) {
        goto done;
    }
    dict = PyDict_New();
    if (!dict) {
        goto done;
    }
    result = PyArg_ParseTupleAndKeywords(tuple, dict, "|:test_empty_argparse",
                                         kwlist);
  done:
    Py_DECREF(tuple);
    Py_XDECREF(dict);
    if (!result) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Test tuple argument processing */
static PyObject *
getargs_tuple(PyObject *self, PyObject *args)
{
    int a, b, c;
    if (!PyArg_ParseTuple(args, "i(ii)", &a, &b, &c)) {
        return NULL;
    }
    return Py_BuildValue("iii", a, b, c);
}

/* test PyArg_ParseTupleAndKeywords */
static PyObject *
getargs_keywords(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"arg1","arg2","arg3","arg4","arg5", NULL};
    static const char fmt[] = "(ii)i|(i(ii))(iii)i";
    int int_args[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, fmt, keywords,
        &int_args[0], &int_args[1], &int_args[2], &int_args[3], &int_args[4],
        &int_args[5], &int_args[6], &int_args[7], &int_args[8], &int_args[9]))
    {
        return NULL;
    }
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
    {
        return NULL;
    }
    return Py_BuildValue("iii", required, optional, keyword_only);
}

/* test PyArg_ParseTupleAndKeywords positional-only arguments */
static PyObject *
getargs_positional_only_and_keywords(PyObject *self, PyObject *args,
                                     PyObject *kwargs)
{
    static char *keywords[] = {"", "", "keyword", NULL};
    int required = -1;
    int optional = -1;
    int keyword = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|ii", keywords,
                                     &required, &optional, &keyword))
    {
        return NULL;
    }
    return Py_BuildValue("iii", required, optional, keyword);
}

/* Functions to call PyArg_ParseTuple with integer format codes,
   and return the result.
*/
static PyObject *
getargs_b(PyObject *self, PyObject *args)
{
    unsigned char value;
    if (!PyArg_ParseTuple(args, "b", &value)) {
        return NULL;
    }
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_B(PyObject *self, PyObject *args)
{
    unsigned char value;
    if (!PyArg_ParseTuple(args, "B", &value)) {
        return NULL;
    }
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_h(PyObject *self, PyObject *args)
{
    short value;
    if (!PyArg_ParseTuple(args, "h", &value)) {
        return NULL;
    }
    return PyLong_FromLong((long)value);
}

static PyObject *
getargs_H(PyObject *self, PyObject *args)
{
    unsigned short value;
    if (!PyArg_ParseTuple(args, "H", &value)) {
        return NULL;
    }
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_I(PyObject *self, PyObject *args)
{
    unsigned int value;
    if (!PyArg_ParseTuple(args, "I", &value)) {
        return NULL;
    }
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_k(PyObject *self, PyObject *args)
{
    unsigned long value;
    if (!PyArg_ParseTuple(args, "k", &value)) {
        return NULL;
    }
    return PyLong_FromUnsignedLong(value);
}

static PyObject *
getargs_i(PyObject *self, PyObject *args)
{
    int value;
    if (!PyArg_ParseTuple(args, "i", &value)) {
        return NULL;
    }
    return PyLong_FromLong((long)value);
}

static PyObject *
getargs_l(PyObject *self, PyObject *args)
{
    long value;
    if (!PyArg_ParseTuple(args, "l", &value)) {
        return NULL;
    }
    return PyLong_FromLong(value);
}

static PyObject *
getargs_n(PyObject *self, PyObject *args)
{
    Py_ssize_t value;
    if (!PyArg_ParseTuple(args, "n", &value)) {
        return NULL;
    }
    return PyLong_FromSsize_t(value);
}

static PyObject *
getargs_p(PyObject *self, PyObject *args)
{
    int value;
    if (!PyArg_ParseTuple(args, "p", &value)) {
        return NULL;
    }
    return PyLong_FromLong(value);
}

static PyObject *
getargs_L(PyObject *self, PyObject *args)
{
    long long value;
    if (!PyArg_ParseTuple(args, "L", &value)) {
        return NULL;
    }
    return PyLong_FromLongLong(value);
}

static PyObject *
getargs_K(PyObject *self, PyObject *args)
{
    unsigned long long value;
    if (!PyArg_ParseTuple(args, "K", &value)) {
        return NULL;
    }
    return PyLong_FromUnsignedLongLong(value);
}

/* This function not only tests the 'k' getargs code, but also the
   PyLong_AsUnsignedLongMask() function. */
static PyObject *
test_k_code(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *tuple = PyTuple_New(1);
    if (tuple == NULL) {
        return NULL;
    }

    /* a number larger than ULONG_MAX even on 64-bit platforms */
    PyObject *num = PyLong_FromString("FFFFFFFFFFFFFFFFFFFFFFFF", NULL, 16);
    if (num == NULL) {
        goto error;
    }

    unsigned long value = PyLong_AsUnsignedLongMask(num);
    if (value == (unsigned long)-1 && PyErr_Occurred()) {
        Py_DECREF(num);
        goto error;
    }
    else if (value != ULONG_MAX) {
        Py_DECREF(num);
        PyErr_SetString(PyExc_AssertionError,
            "test_k_code: "
            "PyLong_AsUnsignedLongMask() returned wrong value for long 0xFFF...FFF");
        goto error;
    }

    PyTuple_SET_ITEM(tuple, 0, num);

    value = 0;
    if (!PyArg_ParseTuple(tuple, "k:test_k_code", &value)) {
        goto error;
    }
    if (value != ULONG_MAX) {
        PyErr_SetString(PyExc_AssertionError,
            "test_k_code: k code returned wrong value for long 0xFFF...FFF");
        goto error;
    }

    Py_DECREF(tuple);  // also clears `num`
    tuple = PyTuple_New(1);
    if (tuple == NULL) {
        return NULL;
    }
    num = PyLong_FromString("-FFFFFFFF000000000000000042", NULL, 16);
    if (num == NULL) {
        goto error;
    }

    value = PyLong_AsUnsignedLongMask(num);
    if (value == (unsigned long)-1 && PyErr_Occurred()) {
        Py_DECREF(num);
        goto error;
    }
    else if (value != (unsigned long)-0x42) {
        Py_DECREF(num);
        PyErr_SetString(PyExc_AssertionError,
            "test_k_code: "
            "PyLong_AsUnsignedLongMask() returned wrong value for long -0xFFF..000042");
        goto error;
    }

    PyTuple_SET_ITEM(tuple, 0, num);

    value = 0;
    if (!PyArg_ParseTuple(tuple, "k:test_k_code", &value)) {
        goto error;
    }
    if (value != (unsigned long)-0x42) {
        PyErr_SetString(PyExc_AssertionError,
            "test_k_code: k code returned wrong value for long -0xFFF..000042");
        goto error;
    }

    Py_DECREF(tuple);
    Py_RETURN_NONE;

error:
    Py_DECREF(tuple);
    return NULL;
}

static PyObject *
getargs_f(PyObject *self, PyObject *args)
{
    float f;
    if (!PyArg_ParseTuple(args, "f", &f)) {
        return NULL;
    }
    return PyFloat_FromDouble(f);
}

static PyObject *
getargs_d(PyObject *self, PyObject *args)
{
    double d;
    if (!PyArg_ParseTuple(args, "d", &d)) {
        return NULL;
    }
    return PyFloat_FromDouble(d);
}

static PyObject *
getargs_D(PyObject *self, PyObject *args)
{
    Py_complex cval;
    if (!PyArg_ParseTuple(args, "D", &cval)) {
        return NULL;
    }
    return PyComplex_FromCComplex(cval);
}

static PyObject *
getargs_S(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "S", &obj)) {
        return NULL;
    }
    return Py_NewRef(obj);
}

static PyObject *
getargs_Y(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "Y", &obj)) {
        return NULL;
    }
    return Py_NewRef(obj);
}

static PyObject *
getargs_U(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "U", &obj)) {
        return NULL;
    }
    return Py_NewRef(obj);
}

static PyObject *
getargs_c(PyObject *self, PyObject *args)
{
    char c;
    if (!PyArg_ParseTuple(args, "c", &c)) {
        return NULL;
    }
    return PyLong_FromLong((unsigned char)c);
}

static PyObject *
getargs_C(PyObject *self, PyObject *args)
{
    int c;
    if (!PyArg_ParseTuple(args, "C", &c)) {
        return NULL;
    }
    return PyLong_FromLong(c);
}

static PyObject *
getargs_s(PyObject *self, PyObject *args)
{
    char *str;
    if (!PyArg_ParseTuple(args, "s", &str)) {
        return NULL;
    }
    return PyBytes_FromString(str);
}

static PyObject *
getargs_s_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "s*", &buffer)) {
        return NULL;
    }
    bytes = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    PyBuffer_Release(&buffer);
    return bytes;
}

static PyObject *
getargs_s_hash(PyObject *self, PyObject *args)
{
    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "s#", &str, &size)) {
        return NULL;
    }
    return PyBytes_FromStringAndSize(str, size);
}

static PyObject *
getargs_z(PyObject *self, PyObject *args)
{
    char *str;
    if (!PyArg_ParseTuple(args, "z", &str)) {
        return NULL;
    }
    if (str != NULL) {
        return PyBytes_FromString(str);
    }
    Py_RETURN_NONE;
}

static PyObject *
getargs_z_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "z*", &buffer)) {
        return NULL;
    }
    if (buffer.buf != NULL) {
        bytes = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    }
    else {
        bytes = Py_NewRef(Py_None);
    }
    PyBuffer_Release(&buffer);
    return bytes;
}

static PyObject *
getargs_z_hash(PyObject *self, PyObject *args)
{
    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "z#", &str, &size)) {
        return NULL;
    }
    if (str != NULL) {
        return PyBytes_FromStringAndSize(str, size);
    }
    Py_RETURN_NONE;
}

static PyObject *
getargs_y(PyObject *self, PyObject *args)
{
    char *str;
    if (!PyArg_ParseTuple(args, "y", &str)) {
        return NULL;
    }
    return PyBytes_FromString(str);
}

static PyObject *
getargs_y_star(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    if (!PyArg_ParseTuple(args, "y*", &buffer)) {
        return NULL;
    }
    PyObject *bytes = PyBytes_FromStringAndSize(buffer.buf, buffer.len);
    PyBuffer_Release(&buffer);
    return bytes;
}

static PyObject *
getargs_y_hash(PyObject *self, PyObject *args)
{
    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "y#", &str, &size)) {
        return NULL;
    }
    return PyBytes_FromStringAndSize(str, size);
}

static PyObject *
getargs_u(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    if (!PyArg_ParseTuple(args, "u", &str)) {
        return NULL;
    }
    return PyUnicode_FromWideChar(str, -1);
}

static PyObject *
getargs_u_hash(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "u#", &str, &size)) {
        return NULL;
    }
    return PyUnicode_FromWideChar(str, size);
}

static PyObject *
getargs_Z(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    if (!PyArg_ParseTuple(args, "Z", &str)) {
        return NULL;
    }
    if (str != NULL) {
        return PyUnicode_FromWideChar(str, -1);
    }
    Py_RETURN_NONE;
}

static PyObject *
getargs_Z_hash(PyObject *self, PyObject *args)
{
    Py_UNICODE *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Z#", &str, &size)) {
        return NULL;
    }
    if (str != NULL) {
        return PyUnicode_FromWideChar(str, size);
    }
    Py_RETURN_NONE;
}

static PyObject *
getargs_es(PyObject *self, PyObject *args)
{
    PyObject *arg;
    const char *encoding = NULL;
    char *str;

    if (!PyArg_ParseTuple(args, "O|s", &arg, &encoding)) {
        return NULL;
    }
    if (!PyArg_Parse(arg, "es", encoding, &str)) {
        return NULL;
    }
    PyObject *result = PyBytes_FromString(str);
    PyMem_Free(str);
    return result;
}

static PyObject *
getargs_et(PyObject *self, PyObject *args)
{
    PyObject *arg;
    const char *encoding = NULL;
    char *str;

    if (!PyArg_ParseTuple(args, "O|s", &arg, &encoding)) {
        return NULL;
    }
    if (!PyArg_Parse(arg, "et", encoding, &str)) {
        return NULL;
    }
    PyObject *result = PyBytes_FromString(str);
    PyMem_Free(str);
    return result;
}

static PyObject *
getargs_es_hash(PyObject *self, PyObject *args)
{
    PyObject *arg;
    const char *encoding = NULL;
    PyByteArrayObject *buffer = NULL;
    char *str = NULL;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "O|sY", &arg, &encoding, &buffer)) {
        return NULL;
    }
    if (buffer != NULL) {
        str = PyByteArray_AS_STRING(buffer);
        size = PyByteArray_GET_SIZE(buffer);
    }
    if (!PyArg_Parse(arg, "es#", encoding, &str, &size)) {
        return NULL;
    }
    PyObject *result = PyBytes_FromStringAndSize(str, size);
    if (buffer == NULL) {
        PyMem_Free(str);
    }
    return result;
}

static PyObject *
getargs_et_hash(PyObject *self, PyObject *args)
{
    PyObject *arg;
    const char *encoding = NULL;
    PyByteArrayObject *buffer = NULL;
    char *str = NULL;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "O|sY", &arg, &encoding, &buffer)) {
        return NULL;
    }
    if (buffer != NULL) {
        str = PyByteArray_AS_STRING(buffer);
        size = PyByteArray_GET_SIZE(buffer);
    }
    if (!PyArg_Parse(arg, "et#", encoding, &str, &size)) {
        return NULL;
    }
    PyObject *result = PyBytes_FromStringAndSize(str, size);
    if (buffer == NULL) {
        PyMem_Free(str);
    }
    return result;
}

/* Test the L code for PyArg_ParseTuple.  This should deliver a long long
   for both long and int arguments.  The test may leak a little memory if
   it fails.
*/
static PyObject *
test_L_code(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *tuple = PyTuple_New(1);
    if (tuple == NULL) {
        return NULL;
    }

    PyObject *num = PyLong_FromLong(42);
    if (num == NULL) {
        goto error;
    }

    PyTuple_SET_ITEM(tuple, 0, num);

    long long value = -1;
    if (!PyArg_ParseTuple(tuple, "L:test_L_code", &value)) {
        goto error;
    }
    if (value != 42) {
        PyErr_SetString(PyExc_AssertionError,
            "test_L_code: L code returned wrong value for long 42");
        goto error;
    }

    Py_DECREF(tuple);  // also clears `num`
    tuple = PyTuple_New(1);
    if (tuple == NULL) {
        return NULL;
    }
    num = PyLong_FromLong(42);
    if (num == NULL) {
        goto error;
    }

    PyTuple_SET_ITEM(tuple, 0, num);

    value = -1;
    if (!PyArg_ParseTuple(tuple, "L:test_L_code", &value)) {
        goto error;
    }
    if (value != 42) {
        PyErr_SetString(PyExc_AssertionError,
            "test_L_code: L code returned wrong value for int 42");
        goto error;
    }

    Py_DECREF(tuple);
    Py_RETURN_NONE;

error:
    Py_DECREF(tuple);
    return NULL;
}

/* Test the s and z codes for PyArg_ParseTuple.
*/
static PyObject *
test_s_code(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    /* Unicode strings should be accepted */
    PyObject *tuple = PyTuple_New(1);
    if (tuple == NULL) {
        return NULL;
    }

    PyObject *obj = PyUnicode_Decode("t\xeate", strlen("t\xeate"),
                                     "latin-1", NULL);
    if (obj == NULL) {
        goto error;
    }

    PyTuple_SET_ITEM(tuple, 0, obj);

    /* These two blocks used to raise a TypeError:
     * "argument must be string without null bytes, not str"
     */
    char *value;
    if (!PyArg_ParseTuple(tuple, "s:test_s_code1", &value)) {
        goto error;
    }

    if (!PyArg_ParseTuple(tuple, "z:test_s_code2", &value)) {
        goto error;
    }

    Py_DECREF(tuple);
    Py_RETURN_NONE;

error:
    Py_DECREF(tuple);
    return NULL;
}

#undef PyArg_ParseTupleAndKeywords
PyAPI_FUNC(int) PyArg_ParseTupleAndKeywords(PyObject *, PyObject *,
                                            const char *, char **, ...);

static PyObject *
getargs_s_hash_int(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"", "", "x", NULL};
    Py_buffer buf = {NULL};
    const char *s;
    int len;
    int i = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "w*|s#i", keywords,
                                     &buf, &s, &len, &i))
    {
        return NULL;
    }
    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyObject *
getargs_s_hash_int2(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"", "", "x", NULL};
    Py_buffer buf = {NULL};
    const char *s;
    int len;
    int i = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "w*|(s#)i", keywords,
                                     &buf, &s, &len, &i))
    {
        return NULL;
    }
    PyBuffer_Release(&buf);
    Py_RETURN_NONE;
}

static PyObject *
gh_99240_clear_args(PyObject *self, PyObject *args)
{
    char *a = NULL;
    char *b = NULL;

    if (!PyArg_ParseTuple(args, "eses", "idna", &a, "idna", &b)) {
        if (a || b) {
            PyErr_Clear();
            PyErr_SetString(PyExc_AssertionError, "Arguments are not cleared.");
        }
        return NULL;
    }
    PyMem_Free(a);
    PyMem_Free(b);
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"get_args",                get_args,                        METH_VARARGS},
    {"get_kwargs", _PyCFunction_CAST(get_kwargs), METH_VARARGS|METH_KEYWORDS},
    {"getargs_B",               getargs_B,                       METH_VARARGS},
    {"getargs_C",               getargs_C,                       METH_VARARGS},
    {"getargs_D",               getargs_D,                       METH_VARARGS},
    {"getargs_H",               getargs_H,                       METH_VARARGS},
    {"getargs_I",               getargs_I,                       METH_VARARGS},
    {"getargs_K",               getargs_K,                       METH_VARARGS},
    {"getargs_L",               getargs_L,                       METH_VARARGS},
    {"getargs_S",               getargs_S,                       METH_VARARGS},
    {"getargs_U",               getargs_U,                       METH_VARARGS},
    {"getargs_Y",               getargs_Y,                       METH_VARARGS},
    {"getargs_Z",               getargs_Z,                       METH_VARARGS},
    {"getargs_Z_hash",          getargs_Z_hash,                  METH_VARARGS},
    {"getargs_b",               getargs_b,                       METH_VARARGS},
    {"getargs_c",               getargs_c,                       METH_VARARGS},
    {"getargs_d",               getargs_d,                       METH_VARARGS},
    {"getargs_es",              getargs_es,                      METH_VARARGS},
    {"getargs_es_hash",         getargs_es_hash,                 METH_VARARGS},
    {"getargs_et",              getargs_et,                      METH_VARARGS},
    {"getargs_et_hash",         getargs_et_hash,                 METH_VARARGS},
    {"getargs_f",               getargs_f,                       METH_VARARGS},
    {"getargs_h",               getargs_h,                       METH_VARARGS},
    {"getargs_i",               getargs_i,                       METH_VARARGS},
    {"getargs_k",               getargs_k,                       METH_VARARGS},
    {"getargs_keyword_only", _PyCFunction_CAST(getargs_keyword_only), METH_VARARGS|METH_KEYWORDS},
    {"getargs_keywords", _PyCFunction_CAST(getargs_keywords), METH_VARARGS|METH_KEYWORDS},
    {"getargs_l",               getargs_l,                       METH_VARARGS},
    {"getargs_n",               getargs_n,                       METH_VARARGS},
    {"getargs_p",               getargs_p,                       METH_VARARGS},
    {"getargs_positional_only_and_keywords", _PyCFunction_CAST(getargs_positional_only_and_keywords), METH_VARARGS|METH_KEYWORDS},
    {"getargs_s",               getargs_s,                       METH_VARARGS},
    {"getargs_s_hash",          getargs_s_hash,                  METH_VARARGS},
    {"getargs_s_hash_int", _PyCFunction_CAST(getargs_s_hash_int), METH_VARARGS|METH_KEYWORDS},
    {"getargs_s_hash_int2", _PyCFunction_CAST(getargs_s_hash_int2), METH_VARARGS|METH_KEYWORDS},
    {"getargs_s_star",          getargs_s_star,                  METH_VARARGS},
    {"getargs_tuple",           getargs_tuple,                   METH_VARARGS},
    {"getargs_u",               getargs_u,                       METH_VARARGS},
    {"getargs_u_hash",          getargs_u_hash,                  METH_VARARGS},
    {"getargs_w_star",          getargs_w_star,                  METH_VARARGS},
    {"getargs_y",               getargs_y,                       METH_VARARGS},
    {"getargs_y_hash",          getargs_y_hash,                  METH_VARARGS},
    {"getargs_y_star",          getargs_y_star,                  METH_VARARGS},
    {"getargs_z",               getargs_z,                       METH_VARARGS},
    {"getargs_z_hash",          getargs_z_hash,                  METH_VARARGS},
    {"getargs_z_star",          getargs_z_star,                  METH_VARARGS},
    {"parse_tuple_and_keywords", parse_tuple_and_keywords,       METH_VARARGS},
    {"test_L_code",             test_L_code,                     METH_NOARGS},
    {"test_empty_argparse",     test_empty_argparse,             METH_NOARGS},
    {"test_k_code",             test_k_code,                     METH_NOARGS},
    {"test_s_code",             test_s_code,                     METH_NOARGS},
    {"gh_99240_clear_args",     gh_99240_clear_args,             METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_GetArgs(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
