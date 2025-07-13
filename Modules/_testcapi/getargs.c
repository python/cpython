/*
 * Tests for Python/getargs.c and Python/modsupport.c;
 * APIs that parse and build arguments.
 */

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
            PyErr_SetString(PyExc_ValueError,
                "parse_tuple_and_keywords: "
                "keywords must be str or bytes");
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
getargs_w_star_opt(PyObject *self, PyObject *args)
{
    Py_buffer buffer;
    Py_buffer buf2;
    int number = 1;

    if (!PyArg_ParseTuple(args, "w*|w*i:getargs_w_star",
                          &buffer, &buf2, &number)) {
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

/* Test the old w and w# codes that no longer work */
static PyObject *
test_w_code_invalid(PyObject *self, PyObject *arg)
{
    static const char * const keywords[] = {"a", "b", "c", "d", NULL};
    char *formats_3[] = {"O|w#$O",
                         "O|w$O",
                         "O|w#O",
                         "O|wO",
                         NULL};
    char *formats_4[] = {"O|w#O$O",
                         "O|wO$O",
                         "O|Ow#O",
                         "O|OwO",
                         "O|Ow#$O",
                         "O|Ow$O",
                         NULL};
    size_t n;
    PyObject *args;
    PyObject *kwargs;
    PyObject *tmp;

    if (!(args = PyTuple_Pack(1, Py_None))) {
        return NULL;
    }

    kwargs = PyDict_New();
    if (!kwargs) {
        Py_DECREF(args);
        return NULL;
    }

    if (PyDict_SetItemString(kwargs, "c", Py_None)) {
        Py_DECREF(args);
        Py_XDECREF(kwargs);
        return NULL;
    }

    for (n = 0; formats_3[n]; ++n) {
        if (PyArg_ParseTupleAndKeywords(args, kwargs, formats_3[n],
                                        (char**) keywords,
                                        &tmp, &tmp, &tmp)) {
            Py_DECREF(args);
            Py_DECREF(kwargs);
            PyErr_Format(PyExc_AssertionError,
                         "test_w_code_invalid_suffix: %s",
                         formats_3[n]);
            return NULL;
        }
        else {
            if (!PyErr_ExceptionMatches(PyExc_SystemError)) {
                Py_DECREF(args);
                Py_DECREF(kwargs);
                return NULL;
            }
            PyErr_Clear();
        }
    }

    if (PyDict_DelItemString(kwargs, "c") ||
        PyDict_SetItemString(kwargs, "d", Py_None)) {

        Py_DECREF(kwargs);
        Py_DECREF(args);
        return NULL;
    }

    for (n = 0; formats_4[n]; ++n) {
        if (PyArg_ParseTupleAndKeywords(args, kwargs, formats_4[n],
                                        (char**) keywords,
                                        &tmp, &tmp, &tmp, &tmp)) {
            Py_DECREF(args);
            Py_DECREF(kwargs);
            PyErr_Format(PyExc_AssertionError,
                         "test_w_code_invalid_suffix: %s",
                         formats_4[n]);
            return NULL;
        }
        else {
            if (!PyErr_ExceptionMatches(PyExc_SystemError)) {
                Py_DECREF(args);
                Py_DECREF(kwargs);
                return NULL;
            }
            PyErr_Clear();
        }
    }

    Py_DECREF(args);
    Py_DECREF(kwargs);
    Py_RETURN_NONE;
}

static PyObject *
getargs_empty(PyObject *self, PyObject *args, PyObject *kwargs)
{
    /* Test that formats can begin with '|'. See issue #4720. */
    assert(PyTuple_CheckExact(args));
    assert(kwargs == NULL || PyDict_CheckExact(kwargs));

    int result;
    if (kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0) {
        static char *kwlist[] = {NULL};
        result = PyArg_ParseTupleAndKeywords(args, kwargs, "|:getargs_empty",
                                             kwlist);
    }
    else {
        result = PyArg_ParseTuple(args, "|:getargs_empty");
    }
    if (!result) {
        return NULL;
    }
    return PyLong_FromLong(result);
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
    {"getargs_s_star",          getargs_s_star,                  METH_VARARGS},
    {"getargs_tuple",           getargs_tuple,                   METH_VARARGS},
    {"getargs_w_star",          getargs_w_star,                  METH_VARARGS},
    {"getargs_w_star_opt",      getargs_w_star_opt,              METH_VARARGS},
    {"getargs_empty",           _PyCFunction_CAST(getargs_empty), METH_VARARGS|METH_KEYWORDS},
    {"getargs_y",               getargs_y,                       METH_VARARGS},
    {"getargs_y_hash",          getargs_y_hash,                  METH_VARARGS},
    {"getargs_y_star",          getargs_y_star,                  METH_VARARGS},
    {"getargs_z",               getargs_z,                       METH_VARARGS},
    {"getargs_z_hash",          getargs_z_hash,                  METH_VARARGS},
    {"getargs_z_star",          getargs_z_star,                  METH_VARARGS},
    {"parse_tuple_and_keywords", parse_tuple_and_keywords,       METH_VARARGS},
    {"gh_99240_clear_args",     gh_99240_clear_args,             METH_VARARGS},
    {"test_w_code_invalid",     test_w_code_invalid,             METH_NOARGS},
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
