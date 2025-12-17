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

static PyObject*
unicode_GET_CACHED_HASH(PyObject *self, PyObject *arg)
{
    return PyLong_FromSsize_t(PyUnstable_Unicode_GET_CACHED_HASH(arg));
}


// --- PyUnicodeWriter type -------------------------------------------------

typedef struct {
    PyObject_HEAD
    PyUnicodeWriter *writer;
} WriterObject;


static PyObject *
writer_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    WriterObject *self = (WriterObject *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    self->writer = NULL;
    return (PyObject*)self;
}


static int
writer_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    WriterObject *self = (WriterObject *)self_raw;

    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "n", &size)) {
        return -1;
    }

    if (self->writer) {
        PyUnicodeWriter_Discard(self->writer);
    }

    self->writer = PyUnicodeWriter_Create(size);
    if (self->writer == NULL) {
        return -1;
    }
    return 0;
}


static void
writer_dealloc(PyObject *self_raw)
{
    WriterObject *self = (WriterObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    if (self->writer) {
        PyUnicodeWriter_Discard(self->writer);
    }
    tp->tp_free(self);
    Py_DECREF(tp);
}


static inline int
writer_check(WriterObject *self)
{
    if (self->writer == NULL) {
        PyErr_SetString(PyExc_ValueError, "operation on finished writer");
        return -1;
    }
    return 0;
}


static PyObject*
writer_write_char(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str;
    if (!PyArg_ParseTuple(args, "U", &str)) {
        return NULL;
    }
    if (PyUnicode_GET_LENGTH(str) != 1) {
        PyErr_SetString(PyExc_ValueError, "expect a single character");
    }
    Py_UCS4 ch = PyUnicode_READ_CHAR(str, 0);

    if (PyUnicodeWriter_WriteChar(self->writer, ch) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_utf8(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "yn", &str, &size)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteUTF8(self->writer, str, size) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_ascii(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "yn", &str, &size)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteASCII(self->writer, str, size) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_widechar(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str;
    if (!PyArg_ParseTuple(args, "U", &str)) {
        return NULL;
    }

    Py_ssize_t size;
    wchar_t *wstr = PyUnicode_AsWideCharString(str, &size);
    if (wstr == NULL) {
        return NULL;
    }

    int res = PyUnicodeWriter_WriteWideChar(self->writer, wstr, size);
    PyMem_Free(wstr);
    if (res < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_ucs4(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "Un", &str, &size)) {
        return NULL;
    }
    Py_ssize_t len = PyUnicode_GET_LENGTH(str);
    size = Py_MIN(size, len);

    Py_UCS4 *ucs4 = PyUnicode_AsUCS4Copy(str);
    if (ucs4 == NULL) {
        return NULL;
    }

    int res = PyUnicodeWriter_WriteUCS4(self->writer, ucs4, size);
    PyMem_Free(ucs4);
    if (res < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_str(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteStr(self->writer, obj) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_repr(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteRepr(self->writer, obj) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_write_substring(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str;
    Py_ssize_t start, end;
    if (!PyArg_ParseTuple(args, "Unn", &str, &start, &end)) {
        return NULL;
    }

    if (PyUnicodeWriter_WriteSubstring(self->writer, str, start, end) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_decodeutf8stateful(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    const char *str;
    Py_ssize_t len;
    const char *errors;
    int use_consumed = 0;
    if (!PyArg_ParseTuple(args, "yny|i", &str, &len, &errors, &use_consumed)) {
        return NULL;
    }

    Py_ssize_t consumed = 12345;
    Py_ssize_t *pconsumed = use_consumed ? &consumed : NULL;
    if (PyUnicodeWriter_DecodeUTF8Stateful(self->writer, str, len,
                                           errors, pconsumed) < 0) {
        if (use_consumed) {
            assert(consumed == 0);
        }
        return NULL;
    }

    if (use_consumed) {
        return PyLong_FromSsize_t(consumed);
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_get_pointer(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    return PyLong_FromVoidPtr(self->writer);
}


static PyObject*
writer_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str = PyUnicodeWriter_Finish(self->writer);
    self->writer = NULL;
    return str;
}


static PyMethodDef writer_methods[] = {
    {"write_char", _PyCFunction_CAST(writer_write_char), METH_VARARGS},
    {"write_utf8", _PyCFunction_CAST(writer_write_utf8), METH_VARARGS},
    {"write_ascii", _PyCFunction_CAST(writer_write_ascii), METH_VARARGS},
    {"write_widechar", _PyCFunction_CAST(writer_write_widechar), METH_VARARGS},
    {"write_ucs4", _PyCFunction_CAST(writer_write_ucs4), METH_VARARGS},
    {"write_str", _PyCFunction_CAST(writer_write_str), METH_VARARGS},
    {"write_repr", _PyCFunction_CAST(writer_write_repr), METH_VARARGS},
    {"write_substring", _PyCFunction_CAST(writer_write_substring), METH_VARARGS},
    {"decodeutf8stateful", _PyCFunction_CAST(writer_decodeutf8stateful), METH_VARARGS},
    {"get_pointer", _PyCFunction_CAST(writer_get_pointer), METH_VARARGS},
    {"finish", _PyCFunction_CAST(writer_finish), METH_NOARGS},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot Writer_Type_slots[] = {
    {Py_tp_new, writer_new},
    {Py_tp_init, writer_init},
    {Py_tp_dealloc, writer_dealloc},
    {Py_tp_methods, writer_methods},
    {0, 0},  /* sentinel */
};

static PyType_Spec Writer_spec = {
    .name = "_testcapi.PyUnicodeWriter",
    .basicsize = sizeof(WriterObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Writer_Type_slots,
};


static PyMethodDef TestMethods[] = {
    {"unicode_new",              unicode_new,                    METH_VARARGS},
    {"unicode_fill",             unicode_fill,                   METH_VARARGS},
    {"unicode_fromkindanddata",  unicode_fromkindanddata,        METH_VARARGS},
    {"unicode_asucs4",           unicode_asucs4,                 METH_VARARGS},
    {"unicode_asucs4copy",       unicode_asucs4copy,             METH_VARARGS},
    {"unicode_asutf8",           unicode_asutf8,                 METH_VARARGS},
    {"unicode_copycharacters",   unicode_copycharacters,         METH_VARARGS},
    {"unicode_GET_CACHED_HASH",  unicode_GET_CACHED_HASH,        METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Unicode(PyObject *m) {
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    PyTypeObject *writer_type = (PyTypeObject *)PyType_FromSpec(&Writer_spec);
    if (writer_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, writer_type) < 0) {
        Py_DECREF(writer_type);
        return -1;
    }
    Py_DECREF(writer_type);

    return 0;
}
