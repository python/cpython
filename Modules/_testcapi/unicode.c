#include <stddef.h>               // ptrdiff_t

#include "parts.h"
#include "util.h"

/* Test PyUnicode_New() */
static PyObject *
unicode_new(PyObject *self, PyObject *args)
{
    Py_ssize_t size;
    unsigned int maxchar;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "nI", &size, &maxchar)) {
        return NULL;
    }

    result = PyUnicode_New(size, (Py_UCS4)maxchar);
    if (!result) {
        return NULL;
    }
    if (size > 0 && maxchar <= 0x10ffff &&
        PyUnicode_Fill(result, 0, size, (Py_UCS4)maxchar) < 0)
    {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}


static PyObject *
unicode_copy(PyObject *unicode)
{
    PyObject *copy;

    if (!unicode) {
        return NULL;
    }
    if (!PyUnicode_Check(unicode)) {
        Py_INCREF(unicode);
        return unicode;
    }

    copy = PyUnicode_New(PyUnicode_GET_LENGTH(unicode),
                         PyUnicode_MAX_CHAR_VALUE(unicode));
    if (!copy) {
        return NULL;
    }
    if (PyUnicode_CopyCharacters(copy, 0, unicode,
                                 0, PyUnicode_GET_LENGTH(unicode)) < 0)
    {
        Py_DECREF(copy);
        return NULL;
    }
    return copy;
}


/* Test PyUnicode_Fill() */
static PyObject *
unicode_fill(PyObject *self, PyObject *args)
{
    PyObject *to, *to_copy;
    Py_ssize_t start, length, filled;
    unsigned int fill_char;

    if (!PyArg_ParseTuple(args, "OnnI", &to, &start, &length, &fill_char)) {
        return NULL;
    }

    NULLABLE(to);
    if (!(to_copy = unicode_copy(to)) && to) {
        return NULL;
    }

    filled = PyUnicode_Fill(to_copy, start, length, (Py_UCS4)fill_char);
    if (filled == -1 && PyErr_Occurred()) {
        Py_DECREF(to_copy);
        return NULL;
    }
    return Py_BuildValue("(Nn)", to_copy, filled);
}


/* Test PyUnicode_FromKindAndData() */
static PyObject *
unicode_fromkindanddata(PyObject *self, PyObject *args)
{
    int kind;
    void *buffer;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;

    if (!PyArg_ParseTuple(args, "iz#|n", &kind, &buffer, &bsize, &size)) {
        return NULL;
    }

    if (size == -100) {
        size = bsize;
    }
    if (kind && size % kind) {
        PyErr_SetString(PyExc_AssertionError,
                        "invalid size in unicode_fromkindanddata()");
        return NULL;
    }
    return PyUnicode_FromKindAndData(kind, buffer, kind ? size / kind : 0);
}


// Test PyUnicode_AsUCS4().
// Part of the limited C API, but the test needs PyUnicode_FromKindAndData().
static PyObject *
unicode_asucs4(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    Py_UCS4 *buffer;
    int copy_null;
    Py_ssize_t str_len, buf_len;

    if (!PyArg_ParseTuple(args, "Onp:unicode_asucs4", &unicode, &str_len, &copy_null)) {
        return NULL;
    }

    NULLABLE(unicode);
    buf_len = str_len + 1;
    buffer = PyMem_NEW(Py_UCS4, buf_len);
    if (buffer == NULL) {
        return PyErr_NoMemory();
    }
    memset(buffer, 0, sizeof(Py_UCS4)*buf_len);
    buffer[str_len] = 0xffffU;

    if (!PyUnicode_AsUCS4(unicode, buffer, buf_len, copy_null)) {
        PyMem_Free(buffer);
        return NULL;
    }

    result = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, buffer, buf_len);
    PyMem_Free(buffer);
    return result;
}


// Test PyUnicode_AsUCS4Copy().
// Part of the limited C API, but the test needs PyUnicode_FromKindAndData().
static PyObject *
unicode_asucs4copy(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_UCS4 *buffer;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O", &unicode)) {
        return NULL;
    }

    NULLABLE(unicode);
    buffer = PyUnicode_AsUCS4Copy(unicode);
    if (buffer == NULL) {
        return NULL;
    }
    result = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND,
                                       buffer,
                                       PyUnicode_GET_LENGTH(unicode) + 1);
    PyMem_FREE(buffer);
    return result;
}


/* Test PyUnicode_AsUTF8() */
static PyObject *
unicode_asutf8(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t buflen;
    const char *s;

    if (!PyArg_ParseTuple(args, "On", &unicode, &buflen))
        return NULL;

    NULLABLE(unicode);
    s = PyUnicode_AsUTF8(unicode);
    if (s == NULL)
        return NULL;

    return PyBytes_FromStringAndSize(s, buflen);
}


/* Test PyUnicode_CopyCharacters() */
static PyObject *
unicode_copycharacters(PyObject *self, PyObject *args)
{
    PyObject *from, *to, *to_copy;
    Py_ssize_t from_start, to_start, how_many, copied;

    if (!PyArg_ParseTuple(args, "UnOnn", &to, &to_start,
                          &from, &from_start, &how_many)) {
        return NULL;
    }

    NULLABLE(from);
    if (!(to_copy = PyUnicode_New(PyUnicode_GET_LENGTH(to),
                                  PyUnicode_MAX_CHAR_VALUE(to)))) {
        return NULL;
    }
    if (PyUnicode_Fill(to_copy, 0, PyUnicode_GET_LENGTH(to_copy), 0U) < 0) {
        Py_DECREF(to_copy);
        return NULL;
    }

    copied = PyUnicode_CopyCharacters(to_copy, to_start, from,
                                      from_start, how_many);
    if (copied == -1 && PyErr_Occurred()) {
        Py_DECREF(to_copy);
        return NULL;
    }

    return Py_BuildValue("(Nn)", to_copy, copied);
}


static PyMethodDef TestMethods[] = {
    {"unicode_new",              unicode_new,                    METH_VARARGS},
    {"unicode_fill",             unicode_fill,                   METH_VARARGS},
    {"unicode_fromkindanddata",  unicode_fromkindanddata,        METH_VARARGS},
    {"unicode_asucs4",           unicode_asucs4,                 METH_VARARGS},
    {"unicode_asucs4copy",       unicode_asucs4copy,             METH_VARARGS},
    {"unicode_asutf8",           unicode_asutf8,                 METH_VARARGS},
    {"unicode_copycharacters",   unicode_copycharacters,         METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Unicode(PyObject *m) {
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
