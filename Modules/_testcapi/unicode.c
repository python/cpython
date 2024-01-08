#include <stddef.h>               // ptrdiff_t

#include "parts.h"
#include "util.h"

static struct PyModuleDef *_testcapimodule = NULL;  // set at initialization

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

static PyObject *
test_unicode_compare_with_ascii(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    PyObject *py_s = PyUnicode_FromStringAndSize("str\0", 4);
    int result;
    if (py_s == NULL)
        return NULL;
    result = PyUnicode_CompareWithASCIIString(py_s, "str");
    Py_DECREF(py_s);
    if (!result) {
        PyErr_SetString(PyExc_AssertionError, "Python string ending in NULL "
                        "should not compare equal to c string.");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
test_widechar(PyObject *self, PyObject *Py_UNUSED(ignored))
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
        PyErr_SetString(PyExc_AssertionError,
                        "test_widechar: "
                        "wide string and utf8 string "
                        "have different length");
        return NULL;
    }
    if (PyUnicode_Compare(wide, utf8)) {
        Py_DECREF(wide);
        Py_DECREF(utf8);
        if (PyErr_Occurred())
            return NULL;
        PyErr_SetString(PyExc_AssertionError,
                        "test_widechar: "
                        "wide string and utf8 string "
                        "are different");
        return NULL;
    }

    Py_DECREF(wide);
    Py_DECREF(utf8);

#if defined(SIZEOF_WCHAR_T) && (SIZEOF_WCHAR_T == 4)
    wide = PyUnicode_FromWideChar(invalid, 1);
    if (wide == NULL)
        PyErr_Clear();
    else {
        PyErr_SetString(PyExc_AssertionError,
                        "test_widechar: "
                        "PyUnicode_FromWideChar(L\"\\U00110000\", 1) didn't fail");
        return NULL;
    }
#endif
    Py_RETURN_NONE;
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

/* Test PyUnicode_WriteChar() */
static PyObject *
unicode_writechar(PyObject *self, PyObject *args)
{
    PyObject *to, *to_copy;
    Py_ssize_t index;
    unsigned int character;
    int result;

    if (!PyArg_ParseTuple(args, "OnI", &to, &index, &character)) {
        return NULL;
    }

    NULLABLE(to);
    if (!(to_copy = unicode_copy(to)) && to) {
        return NULL;
    }

    result = PyUnicode_WriteChar(to_copy, index, (Py_UCS4)character);
    if (result == -1 && PyErr_Occurred()) {
        Py_DECREF(to_copy);
        return NULL;
    }
    return Py_BuildValue("(Ni)", to_copy, result);
}

/* Test PyUnicode_Resize() */
static PyObject *
unicode_resize(PyObject *self, PyObject *args)
{
    PyObject *obj, *copy;
    Py_ssize_t length;
    int result;

    if (!PyArg_ParseTuple(args, "On", &obj, &length)) {
        return NULL;
    }

    NULLABLE(obj);
    if (!(copy = unicode_copy(obj)) && obj) {
        return NULL;
    }
    result = PyUnicode_Resize(&copy, length);
    if (result == -1 && PyErr_Occurred()) {
        Py_XDECREF(copy);
        return NULL;
    }
    if (obj && PyUnicode_Check(obj) && length > PyUnicode_GET_LENGTH(obj)) {
        if (PyUnicode_Fill(copy, PyUnicode_GET_LENGTH(obj), length, 0U) < 0) {
            Py_DECREF(copy);
            return NULL;
        }
    }
    return Py_BuildValue("(Ni)", copy, result);
}

/* Test PyUnicode_Append() */
static PyObject *
unicode_append(PyObject *self, PyObject *args)
{
    PyObject *left, *right, *left_copy;

    if (!PyArg_ParseTuple(args, "OO", &left, &right))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    if (!(left_copy = unicode_copy(left)) && left) {
        return NULL;
    }
    PyUnicode_Append(&left_copy, right);
    return left_copy;
}

/* Test PyUnicode_AppendAndDel() */
static PyObject *
unicode_appendanddel(PyObject *self, PyObject *args)
{
    PyObject *left, *right, *left_copy;

    if (!PyArg_ParseTuple(args, "OO", &left, &right))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    if (!(left_copy = unicode_copy(left)) && left) {
        return NULL;
    }
    Py_XINCREF(right);
    PyUnicode_AppendAndDel(&left_copy, right);
    return left_copy;
}

/* Test PyUnicode_FromStringAndSize() */
static PyObject *
unicode_fromstringandsize(PyObject *self, PyObject *args)
{
    const char *s;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;

    if (!PyArg_ParseTuple(args, "z#|n", &s, &bsize, &size)) {
        return NULL;
    }

    if (size == -100) {
        size = bsize;
    }
    return PyUnicode_FromStringAndSize(s, size);
}

/* Test PyUnicode_FromString() */
static PyObject *
unicode_fromstring(PyObject *self, PyObject *arg)
{
    const char *s;
    Py_ssize_t size;

    if (!PyArg_Parse(arg, "z#", &s, &size)) {
        return NULL;
    }
    return PyUnicode_FromString(s);
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

/* Test PyUnicode_Substring() */
static PyObject *
unicode_substring(PyObject *self, PyObject *args)
{
    PyObject *str;
    Py_ssize_t start, end;

    if (!PyArg_ParseTuple(args, "Onn", &str, &start, &end)) {
        return NULL;
    }

    NULLABLE(str);
    return PyUnicode_Substring(str, start, end);
}

/* Test PyUnicode_GetLength() */
static PyObject *
unicode_getlength(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    RETURN_SIZE(PyUnicode_GetLength(arg));
}

/* Test PyUnicode_ReadChar() */
static PyObject *
unicode_readchar(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t index;
    Py_UCS4 result;

    if (!PyArg_ParseTuple(args, "On", &unicode, &index)) {
        return NULL;
    }

    NULLABLE(unicode);
    result = PyUnicode_ReadChar(unicode, index);
    if (result == (Py_UCS4)-1)
        return NULL;
    return PyLong_FromUnsignedLong(result);
}

/* Test PyUnicode_FromEncodedObject() */
static PyObject *
unicode_fromencodedobject(PyObject *self, PyObject *args)
{
    PyObject *obj;
    const char *encoding;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "Oz|z", &obj, &encoding, &errors)) {
        return NULL;
    }

    NULLABLE(obj);
    return PyUnicode_FromEncodedObject(obj, encoding, errors);
}

/* Test PyUnicode_FromObject() */
static PyObject *
unicode_fromobject(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_FromObject(arg);
}

/* Test PyUnicode_InternInPlace() */
static PyObject *
unicode_interninplace(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    Py_XINCREF(arg);
    PyUnicode_InternInPlace(&arg);
    return arg;
}

/* Test PyUnicode_InternFromString() */
static PyObject *
unicode_internfromstring(PyObject *self, PyObject *arg)
{
    const char *s;
    Py_ssize_t size;

    if (!PyArg_Parse(arg, "z#", &s, &size)) {
        return NULL;
    }
    return PyUnicode_InternFromString(s);
}

/* Test PyUnicode_FromWideChar() */
static PyObject *
unicode_fromwidechar(PyObject *self, PyObject *args)
{
    const char *s;
    Py_ssize_t bsize;
    Py_ssize_t size = -100;

    if (!PyArg_ParseTuple(args, "z#|n", &s, &bsize, &size)) {
        return NULL;
    }
    if (size == -100) {
        if (bsize % SIZEOF_WCHAR_T) {
            PyErr_SetString(PyExc_AssertionError,
                            "invalid size in unicode_fromwidechar()");
            return NULL;
        }
        size = bsize / SIZEOF_WCHAR_T;
    }
    return PyUnicode_FromWideChar((const wchar_t *)s, size);
}

/* Test PyUnicode_AsWideChar() */
static PyObject *
unicode_aswidechar(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    Py_ssize_t buflen, size;
    wchar_t *buffer;

    if (!PyArg_ParseTuple(args, "On", &unicode, &buflen))
        return NULL;
    NULLABLE(unicode);
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

/* Test PyUnicode_AsWideCharString() with NULL as buffer */
static PyObject *
unicode_aswidechar_null(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t buflen;

    if (!PyArg_ParseTuple(args, "On", &unicode, &buflen))
        return NULL;
    NULLABLE(unicode);
    RETURN_SIZE(PyUnicode_AsWideChar(unicode, NULL, buflen));
}

/* Test PyUnicode_AsWideCharString() */
static PyObject *
unicode_aswidecharstring(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    Py_ssize_t size = UNINITIALIZED_SIZE;
    wchar_t *buffer;

    if (!PyArg_ParseTuple(args, "O", &unicode))
        return NULL;

    NULLABLE(unicode);
    buffer = PyUnicode_AsWideCharString(unicode, &size);
    if (buffer == NULL) {
        assert(size == UNINITIALIZED_SIZE);
        return NULL;
    }

    result = PyUnicode_FromWideChar(buffer, size + 1);
    PyMem_Free(buffer);
    if (result == NULL)
        return NULL;
    return Py_BuildValue("(Nn)", result, size);
}

/* Test PyUnicode_AsWideCharString() with NULL as the size address */
static PyObject *
unicode_aswidecharstring_null(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    wchar_t *buffer;

    if (!PyArg_ParseTuple(args, "O", &unicode))
        return NULL;

    NULLABLE(unicode);
    buffer = PyUnicode_AsWideCharString(unicode, NULL);
    if (buffer == NULL)
        return NULL;

    result = PyUnicode_FromWideChar(buffer, -1);
    PyMem_Free(buffer);
    if (result == NULL)
        return NULL;
    return result;
}

/* Test PyUnicode_AsUCS4() */
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

/* Test PyUnicode_AsUCS4Copy() */
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

/* Test PyUnicode_FromOrdinal() */
static PyObject *
unicode_fromordinal(PyObject *self, PyObject *args)
{
    int ordinal;

    if (!PyArg_ParseTuple(args, "i", &ordinal))
        return NULL;

    return PyUnicode_FromOrdinal(ordinal);
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

/* Test PyUnicode_AsUTF8AndSize() */
static PyObject *
unicode_asutf8andsize(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t buflen;
    const char *s;
    Py_ssize_t size = UNINITIALIZED_SIZE;

    if (!PyArg_ParseTuple(args, "On", &unicode, &buflen))
        return NULL;

    NULLABLE(unicode);
    s = PyUnicode_AsUTF8AndSize(unicode, &size);
    if (s == NULL) {
        assert(size == -1);
        return NULL;
    }

    return Py_BuildValue("(y#n)", s, buflen, size);
}

/* Test PyUnicode_AsUTF8AndSize() with NULL as the size address */
static PyObject *
unicode_asutf8andsize_null(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    Py_ssize_t buflen;
    const char *s;

    if (!PyArg_ParseTuple(args, "On", &unicode, &buflen))
        return NULL;

    NULLABLE(unicode);
    s = PyUnicode_AsUTF8AndSize(unicode, NULL);
    if (s == NULL)
        return NULL;

    return PyBytes_FromStringAndSize(s, buflen);
}

/* Test PyUnicode_GetDefaultEncoding() */
static PyObject *
unicode_getdefaultencoding(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    const char *s = PyUnicode_GetDefaultEncoding();
    if (s == NULL)
        return NULL;

    return PyBytes_FromString(s);
}

/* Test PyUnicode_Decode() */
static PyObject *
unicode_decode(PyObject *self, PyObject *args)
{
    const char *s;
    Py_ssize_t size;
    const char *encoding;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#z|z", &s, &size, &encoding, &errors))
        return NULL;

    return PyUnicode_Decode(s, size, encoding, errors);
}

/* Test PyUnicode_AsEncodedString() */
static PyObject *
unicode_asencodedstring(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    const char *encoding;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "Oz|z", &unicode, &encoding, &errors))
        return NULL;

    NULLABLE(unicode);
    return PyUnicode_AsEncodedString(unicode, encoding, errors);
}

/* Test PyUnicode_BuildEncodingMap() */
static PyObject *
unicode_buildencodingmap(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_BuildEncodingMap(arg);
}

/* Test PyUnicode_DecodeUTF7() */
static PyObject *
unicode_decodeutf7(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeUTF7(data, size, errors);
}

/* Test PyUnicode_DecodeUTF7Stateful() */
static PyObject *
unicode_decodeutf7stateful(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    Py_ssize_t consumed = UNINITIALIZED_SIZE;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeUTF7Stateful(data, size, errors, &consumed);
    if (!result) {
        assert(consumed == UNINITIALIZED_SIZE);
        return NULL;
    }
    return Py_BuildValue("(Nn)", result, consumed);
}

/* Test PyUnicode_DecodeUTF8() */
static PyObject *
unicode_decodeutf8(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeUTF8(data, size, errors);
}

/* Test PyUnicode_DecodeUTF8Stateful() */
static PyObject *
unicode_decodeutf8stateful(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    Py_ssize_t consumed = UNINITIALIZED_SIZE;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeUTF8Stateful(data, size, errors, &consumed);
    if (!result) {
        assert(consumed == UNINITIALIZED_SIZE);
        return NULL;
    }
    return Py_BuildValue("(Nn)", result, consumed);
}

/* Test PyUnicode_AsUTF8String() */
static PyObject *
unicode_asutf8string(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsUTF8String(arg);
}

/* Test PyUnicode_DecodeUTF32() */
static PyObject *
unicode_decodeutf32(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = UNINITIALIZED_INT;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "iy#|z", &byteorder, &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeUTF32(data, size, errors, &byteorder);
    if (!result) {
        return NULL;
    }
    return Py_BuildValue("(iN)", byteorder, result);
}

/* Test PyUnicode_DecodeUTF32Stateful() */
static PyObject *
unicode_decodeutf32stateful(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = UNINITIALIZED_INT;
    Py_ssize_t consumed = UNINITIALIZED_SIZE;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "iy#|z", &byteorder, &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeUTF32Stateful(data, size, errors, &byteorder, &consumed);
    if (!result) {
        assert(consumed == UNINITIALIZED_SIZE);
        return NULL;
    }
    return Py_BuildValue("(iNn)", byteorder, result, consumed);
}

/* Test PyUnicode_AsUTF32String() */
static PyObject *
unicode_asutf32string(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsUTF32String(arg);
}

/* Test PyUnicode_DecodeUTF16() */
static PyObject *
unicode_decodeutf16(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = UNINITIALIZED_INT;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "iy#|z", &byteorder, &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeUTF16(data, size, errors, &byteorder);
    if (!result) {
        return NULL;
    }
    return Py_BuildValue("(iN)", byteorder, result);
}

/* Test PyUnicode_DecodeUTF16Stateful() */
static PyObject *
unicode_decodeutf16stateful(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = UNINITIALIZED_INT;
    Py_ssize_t consumed = UNINITIALIZED_SIZE;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "iy#|z", &byteorder, &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeUTF16Stateful(data, size, errors, &byteorder, &consumed);
    if (!result) {
        assert(consumed == UNINITIALIZED_SIZE);
        return NULL;
    }
    return Py_BuildValue("(iNn)", byteorder, result, consumed);
}

/* Test PyUnicode_AsUTF16String() */
static PyObject *
unicode_asutf16string(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsUTF16String(arg);
}

/* Test PyUnicode_DecodeUnicodeEscape() */
static PyObject *
unicode_decodeunicodeescape(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeUnicodeEscape(data, size, errors);
}

/* Test PyUnicode_AsUnicodeEscapeString() */
static PyObject *
unicode_asunicodeescapestring(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsUnicodeEscapeString(arg);
}

static PyObject *
unicode_decoderawunicodeescape(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeRawUnicodeEscape(data, size, errors);
}

/* Test PyUnicode_AsRawUnicodeEscapeString() */
static PyObject *
unicode_asrawunicodeescapestring(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsRawUnicodeEscapeString(arg);
}

static PyObject *
unicode_decodelatin1(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeLatin1(data, size, errors);
}

/* Test PyUnicode_AsLatin1String() */
static PyObject *
unicode_aslatin1string(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsLatin1String(arg);
}

/* Test PyUnicode_DecodeASCII() */
static PyObject *
unicode_decodeascii(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeASCII(data, size, errors);
}

/* Test PyUnicode_AsASCIIString() */
static PyObject *
unicode_asasciistring(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsASCIIString(arg);
}

/* Test PyUnicode_DecodeCharmap() */
static PyObject *
unicode_decodecharmap(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    PyObject *mapping;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#O|z", &data, &size, &mapping, &errors))
        return NULL;

    NULLABLE(mapping);
    return PyUnicode_DecodeCharmap(data, size, mapping, errors);
}

/* Test PyUnicode_AsCharmapString() */
static PyObject *
unicode_ascharmapstring(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    PyObject *mapping;

    if (!PyArg_ParseTuple(args, "OO", &unicode, &mapping))
        return NULL;

    NULLABLE(unicode);
    NULLABLE(mapping);
    return PyUnicode_AsCharmapString(unicode, mapping);
}

#ifdef MS_WINDOWS

/* Test PyUnicode_DecodeMBCS() */
static PyObject *
unicode_decodembcs(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeMBCS(data, size, errors);
}

/* Test PyUnicode_DecodeMBCSStateful() */
static PyObject *
unicode_decodembcsstateful(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    Py_ssize_t consumed = UNINITIALIZED_SIZE;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeMBCSStateful(data, size, errors, &consumed);
    if (!result) {
        assert(consumed == UNINITIALIZED_SIZE);
        return NULL;
    }
    return Py_BuildValue("(Nn)", result, consumed);
}

/* Test PyUnicode_DecodeCodePageStateful() */
static PyObject *
unicode_decodecodepagestateful(PyObject *self, PyObject *args)
{
    int code_page;
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    Py_ssize_t consumed = UNINITIALIZED_SIZE;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "iy#|z", &code_page, &data, &size, &errors))
        return NULL;

    result = PyUnicode_DecodeCodePageStateful(code_page, data, size, errors, &consumed);
    if (!result) {
        assert(consumed == UNINITIALIZED_SIZE);
        return NULL;
    }
    return Py_BuildValue("(Nn)", result, consumed);
}

/* Test PyUnicode_AsMBCSString() */
static PyObject *
unicode_asmbcsstring(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_AsMBCSString(arg);
}

/* Test PyUnicode_EncodeCodePage() */
static PyObject *
unicode_encodecodepage(PyObject *self, PyObject *args)
{
    int code_page;
    PyObject *unicode;
    const char *errors;

    if (!PyArg_ParseTuple(args, "iO|z", &code_page, &unicode, &errors))
        return NULL;

    NULLABLE(unicode);
    return PyUnicode_EncodeCodePage(code_page, unicode, errors);
}

#endif /* MS_WINDOWS */

/* Test PyUnicode_DecodeLocaleAndSize() */
static PyObject *
unicode_decodelocaleandsize(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeLocaleAndSize(data, size, errors);
}

/* Test PyUnicode_DecodeLocale() */
static PyObject *
unicode_decodelocale(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors;

    if (!PyArg_ParseTuple(args, "y#|z", &data, &size, &errors))
        return NULL;

    return PyUnicode_DecodeLocale(data, errors);
}

/* Test PyUnicode_EncodeLocale() */
static PyObject *
unicode_encodelocale(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    const char *errors;

    if (!PyArg_ParseTuple(args, "O|z", &unicode, &errors))
        return NULL;

    NULLABLE(unicode);
    return PyUnicode_EncodeLocale(unicode, errors);
}

/* Test PyUnicode_DecodeFSDefault() */
static PyObject *
unicode_decodefsdefault(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "y#", &data, &size))
        return NULL;

    return PyUnicode_DecodeFSDefault(data);
}

/* Test PyUnicode_DecodeFSDefaultAndSize() */
static PyObject *
unicode_decodefsdefaultandsize(PyObject *self, PyObject *args)
{
    const char *data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "y#|n", &data, &size, &size))
        return NULL;

    return PyUnicode_DecodeFSDefaultAndSize(data, size);
}

/* Test PyUnicode_EncodeFSDefault() */
static PyObject *
unicode_encodefsdefault(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    return PyUnicode_EncodeFSDefault(arg);
}

/* Test PyUnicode_Concat() */
static PyObject *
unicode_concat(PyObject *self, PyObject *args)
{
    PyObject *left;
    PyObject *right;

    if (!PyArg_ParseTuple(args, "OO", &left, &right))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    return PyUnicode_Concat(left, right);
}

/* Test PyUnicode_Split() */
static PyObject *
unicode_split(PyObject *self, PyObject *args)
{
    PyObject *s;
    PyObject *sep;
    Py_ssize_t maxsplit = -1;

    if (!PyArg_ParseTuple(args, "OO|n", &s, &sep, &maxsplit))
        return NULL;

    NULLABLE(s);
    NULLABLE(sep);
    return PyUnicode_Split(s, sep, maxsplit);
}

/* Test PyUnicode_RSplit() */
static PyObject *
unicode_rsplit(PyObject *self, PyObject *args)
{
    PyObject *s;
    PyObject *sep;
    Py_ssize_t maxsplit = -1;

    if (!PyArg_ParseTuple(args, "OO|n", &s, &sep, &maxsplit))
        return NULL;

    NULLABLE(s);
    NULLABLE(sep);
    return PyUnicode_RSplit(s, sep, maxsplit);
}

/* Test PyUnicode_Splitlines() */
static PyObject *
unicode_splitlines(PyObject *self, PyObject *args)
{
    PyObject *s;
    int keepends = 0;

    if (!PyArg_ParseTuple(args, "O|i", &s, &keepends))
        return NULL;

    NULLABLE(s);
    return PyUnicode_Splitlines(s, keepends);
}

/* Test PyUnicode_Partition() */
static PyObject *
unicode_partition(PyObject *self, PyObject *args)
{
    PyObject *s;
    PyObject *sep;

    if (!PyArg_ParseTuple(args, "OO", &s, &sep))
        return NULL;

    NULLABLE(s);
    NULLABLE(sep);
    return PyUnicode_Partition(s, sep);
}

/* Test PyUnicode_RPartition() */
static PyObject *
unicode_rpartition(PyObject *self, PyObject *args)
{
    PyObject *s;
    PyObject *sep;

    if (!PyArg_ParseTuple(args, "OO", &s, &sep))
        return NULL;

    NULLABLE(s);
    NULLABLE(sep);
    return PyUnicode_RPartition(s, sep);
}

/* Test PyUnicode_Translate() */
static PyObject *
unicode_translate(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyObject *table;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "OO|z", &obj, &table, &errors))
        return NULL;

    NULLABLE(obj);
    NULLABLE(table);
    return PyUnicode_Translate(obj, table, errors);
}

/* Test PyUnicode_Join() */
static PyObject *
unicode_join(PyObject *self, PyObject *args)
{
    PyObject *sep;
    PyObject *seq;

    if (!PyArg_ParseTuple(args, "OO", &sep, &seq))
        return NULL;

    NULLABLE(sep);
    NULLABLE(seq);
    return PyUnicode_Join(sep, seq);
}

/* Test PyUnicode_Count() */
static PyObject *
unicode_count(PyObject *self, PyObject *args)
{
    PyObject *str;
    PyObject *substr;
    Py_ssize_t start;
    Py_ssize_t end;

    if (!PyArg_ParseTuple(args, "OOnn", &str, &substr, &start, &end))
        return NULL;

    NULLABLE(str);
    NULLABLE(substr);
    RETURN_SIZE(PyUnicode_Count(str, substr, start, end));
}

/* Test PyUnicode_Find() */
static PyObject *
unicode_find(PyObject *self, PyObject *args)
{
    PyObject *str;
    PyObject *substr;
    Py_ssize_t start;
    Py_ssize_t end;
    int direction;
    Py_ssize_t result;

    if (!PyArg_ParseTuple(args, "OOnni", &str, &substr, &start, &end, &direction))
        return NULL;

    NULLABLE(str);
    NULLABLE(substr);
    result = PyUnicode_Find(str, substr, start, end, direction);
    if (result == -2) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    return PyLong_FromSsize_t(result);
}

/* Test PyUnicode_Tailmatch() */
static PyObject *
unicode_tailmatch(PyObject *self, PyObject *args)
{
    PyObject *str;
    PyObject *substr;
    Py_ssize_t start;
    Py_ssize_t end;
    int direction;

    if (!PyArg_ParseTuple(args, "OOnni", &str, &substr, &start, &end, &direction))
        return NULL;

    NULLABLE(str);
    NULLABLE(substr);
    RETURN_SIZE(PyUnicode_Tailmatch(str, substr, start, end, direction));
}

/* Test PyUnicode_FindChar() */
static PyObject *
unicode_findchar(PyObject *self, PyObject *args)
{
    PyObject *str;
    int direction;
    unsigned int ch;
    Py_ssize_t result;
    Py_ssize_t start, end;

    if (!PyArg_ParseTuple(args, "OInni:unicode_findchar", &str, &ch,
                          &start, &end, &direction)) {
        return NULL;
    }
    NULLABLE(str);
    result = PyUnicode_FindChar(str, (Py_UCS4)ch, start, end, direction);
    if (result == -2) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    return PyLong_FromSsize_t(result);
}

/* Test PyUnicode_Replace() */
static PyObject *
unicode_replace(PyObject *self, PyObject *args)
{
    PyObject *str;
    PyObject *substr;
    PyObject *replstr;
    Py_ssize_t maxcount = -1;

    if (!PyArg_ParseTuple(args, "OOO|n", &str, &substr, &replstr, &maxcount))
        return NULL;

    NULLABLE(str);
    NULLABLE(substr);
    NULLABLE(replstr);
    return PyUnicode_Replace(str, substr, replstr, maxcount);
}

/* Test PyUnicode_Compare() */
static PyObject *
unicode_compare(PyObject *self, PyObject *args)
{
    PyObject *left;
    PyObject *right;
    int result;

    if (!PyArg_ParseTuple(args, "OO", &left, &right))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    result = PyUnicode_Compare(left, right);
    if (result == -1 && PyErr_Occurred()) {
        return NULL;
    }
    assert(!PyErr_Occurred());
    return PyLong_FromLong(result);
}

/* Test PyUnicode_CompareWithASCIIString() */
static PyObject *
unicode_comparewithasciistring(PyObject *self, PyObject *args)
{
    PyObject *left;
    const char *right = NULL;
    Py_ssize_t right_len;
    int result;

    if (!PyArg_ParseTuple(args, "O|y#", &left, &right, &right_len))
        return NULL;

    NULLABLE(left);
    result = PyUnicode_CompareWithASCIIString(left, right);
    if (result == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLong(result);
}

/* Test PyUnicode_EqualToUTF8() */
static PyObject *
unicode_equaltoutf8(PyObject *self, PyObject *args)
{
    PyObject *left;
    const char *right = NULL;
    Py_ssize_t right_len;
    int result;

    if (!PyArg_ParseTuple(args, "Oz#", &left, &right, &right_len)) {
        return NULL;
    }

    NULLABLE(left);
    result = PyUnicode_EqualToUTF8(left, right);
    assert(!PyErr_Occurred());
    return PyLong_FromLong(result);
}

/* Test PyUnicode_EqualToUTF8AndSize() */
static PyObject *
unicode_equaltoutf8andsize(PyObject *self, PyObject *args)
{
    PyObject *left;
    const char *right = NULL;
    Py_ssize_t right_len;
    Py_ssize_t size = -100;
    int result;

    if (!PyArg_ParseTuple(args, "Oz#|n", &left, &right, &right_len, &size)) {
        return NULL;
    }

    NULLABLE(left);
    if (size == -100) {
        size = right_len;
    }
    result = PyUnicode_EqualToUTF8AndSize(left, right, size);
    assert(!PyErr_Occurred());
    return PyLong_FromLong(result);
}

/* Test PyUnicode_RichCompare() */
static PyObject *
unicode_richcompare(PyObject *self, PyObject *args)
{
    PyObject *left;
    PyObject *right;
    int op;

    if (!PyArg_ParseTuple(args, "OOi", &left, &right, &op))
        return NULL;

    NULLABLE(left);
    NULLABLE(right);
    return PyUnicode_RichCompare(left, right, op);
}

/* Test PyUnicode_Format() */
static PyObject *
unicode_format(PyObject *self, PyObject *args)
{
    PyObject *format;
    PyObject *fargs;

    if (!PyArg_ParseTuple(args, "OO", &format, &fargs))
        return NULL;

    NULLABLE(format);
    NULLABLE(fargs);
    return PyUnicode_Format(format, fargs);
}

/* Test PyUnicode_Contains() */
static PyObject *
unicode_contains(PyObject *self, PyObject *args)
{
    PyObject *container;
    PyObject *element;

    if (!PyArg_ParseTuple(args, "OO", &container, &element))
        return NULL;

    NULLABLE(container);
    NULLABLE(element);
    RETURN_INT(PyUnicode_Contains(container, element));
}

/* Test PyUnicode_IsIdentifier() */
static PyObject *
unicode_isidentifier(PyObject *self, PyObject *arg)
{
    NULLABLE(arg);
    RETURN_INT(PyUnicode_IsIdentifier(arg));
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

static int
check_raised_systemerror(PyObject *result, char* msg)
{
    if (result) {
        // no exception
        PyErr_Format(PyExc_AssertionError,
                     "SystemError not raised: %s",
                     msg);
        return 0;
    }
    if (PyErr_ExceptionMatches(PyExc_SystemError)) {
        // expected exception
        PyErr_Clear();
        return 1;
    }
    // unexpected exception
    return 0;
}

static PyObject *
test_string_from_format(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *result;
    PyObject *unicode = PyUnicode_FromString("None");

#define CHECK_FORMAT_2(FORMAT, EXPECTED, ARG1, ARG2)                \
    result = PyUnicode_FromFormat(FORMAT, ARG1, ARG2);              \
    if (EXPECTED == NULL) {                                         \
        if (!check_raised_systemerror(result, FORMAT)) {            \
            goto Fail;                                              \
        }                                                           \
    }                                                               \
    else if (result == NULL)                                        \
        return NULL;                                                \
    else if (PyUnicode_CompareWithASCIIString(result, EXPECTED) != 0) { \
        PyErr_Format(PyExc_AssertionError,                          \
                     "test_string_from_format: failed at \"%s\" "   \
                     "expected \"%s\" got \"%s\"",                  \
                     FORMAT, EXPECTED, PyUnicode_AsUTF8(result));   \
        goto Fail;                                                  \
    }                                                               \
    Py_XDECREF(result)

#define CHECK_FORMAT_1(FORMAT, EXPECTED, ARG)                       \
    CHECK_FORMAT_2(FORMAT, EXPECTED, ARG, 0)

#define CHECK_FORMAT_0(FORMAT, EXPECTED)                            \
    CHECK_FORMAT_2(FORMAT, EXPECTED, 0, 0)

    // Unrecognized
    CHECK_FORMAT_2("%u %? %u", NULL, 1, 2);

    // "%%" (options are rejected)
    CHECK_FORMAT_0(  "%%", "%");
    CHECK_FORMAT_0( "%0%", NULL);
    CHECK_FORMAT_0("%00%", NULL);
    CHECK_FORMAT_0( "%2%", NULL);
    CHECK_FORMAT_0("%02%", NULL);
    CHECK_FORMAT_0("%.0%", NULL);
    CHECK_FORMAT_0("%.2%", NULL);

    // "%c"
    CHECK_FORMAT_1(  "%c", "c", 'c');
    CHECK_FORMAT_1( "%0c", "c", 'c');
    CHECK_FORMAT_1("%00c", "c", 'c');
    CHECK_FORMAT_1( "%2c", NULL, 'c');
    CHECK_FORMAT_1("%02c", NULL, 'c');
    CHECK_FORMAT_1("%.0c", NULL, 'c');
    CHECK_FORMAT_1("%.2c", NULL, 'c');

    // Integers
    CHECK_FORMAT_1("%d",             "123",                (int)123);
    CHECK_FORMAT_1("%i",             "123",                (int)123);
    CHECK_FORMAT_1("%u",             "123",       (unsigned int)123);
    CHECK_FORMAT_1("%x",              "7b",       (unsigned int)123);
    CHECK_FORMAT_1("%X",              "7B",       (unsigned int)123);
    CHECK_FORMAT_1("%o",             "173",       (unsigned int)123);
    CHECK_FORMAT_1("%ld",            "123",               (long)123);
    CHECK_FORMAT_1("%li",            "123",               (long)123);
    CHECK_FORMAT_1("%lu",            "123",      (unsigned long)123);
    CHECK_FORMAT_1("%lx",             "7b",      (unsigned long)123);
    CHECK_FORMAT_1("%lX",             "7B",      (unsigned long)123);
    CHECK_FORMAT_1("%lo",            "173",      (unsigned long)123);
    CHECK_FORMAT_1("%lld",           "123",          (long long)123);
    CHECK_FORMAT_1("%lli",           "123",          (long long)123);
    CHECK_FORMAT_1("%llu",           "123", (unsigned long long)123);
    CHECK_FORMAT_1("%llx",            "7b", (unsigned long long)123);
    CHECK_FORMAT_1("%llX",            "7B", (unsigned long long)123);
    CHECK_FORMAT_1("%llo",           "173", (unsigned long long)123);
    CHECK_FORMAT_1("%zd",            "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%zi",            "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%zu",            "123",             (size_t)123);
    CHECK_FORMAT_1("%zx",             "7b",             (size_t)123);
    CHECK_FORMAT_1("%zX",             "7B",             (size_t)123);
    CHECK_FORMAT_1("%zo",            "173",             (size_t)123);
    CHECK_FORMAT_1("%td",            "123",          (ptrdiff_t)123);
    CHECK_FORMAT_1("%ti",            "123",          (ptrdiff_t)123);
    CHECK_FORMAT_1("%tu",            "123",          (ptrdiff_t)123);
    CHECK_FORMAT_1("%tx",             "7b",          (ptrdiff_t)123);
    CHECK_FORMAT_1("%tX",             "7B",          (ptrdiff_t)123);
    CHECK_FORMAT_1("%to",            "173",          (ptrdiff_t)123);
    CHECK_FORMAT_1("%jd",            "123",           (intmax_t)123);
    CHECK_FORMAT_1("%ji",            "123",           (intmax_t)123);
    CHECK_FORMAT_1("%ju",            "123",          (uintmax_t)123);
    CHECK_FORMAT_1("%jx",             "7b",          (uintmax_t)123);
    CHECK_FORMAT_1("%jX",             "7B",          (uintmax_t)123);
    CHECK_FORMAT_1("%jo",            "173",          (uintmax_t)123);

    CHECK_FORMAT_1("%d",            "-123",               (int)-123);
    CHECK_FORMAT_1("%i",            "-123",               (int)-123);
    CHECK_FORMAT_1("%ld",           "-123",              (long)-123);
    CHECK_FORMAT_1("%li",           "-123",              (long)-123);
    CHECK_FORMAT_1("%lld",          "-123",         (long long)-123);
    CHECK_FORMAT_1("%lli",          "-123",         (long long)-123);
    CHECK_FORMAT_1("%zd",           "-123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%zi",           "-123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%td",           "-123",         (ptrdiff_t)-123);
    CHECK_FORMAT_1("%ti",           "-123",         (ptrdiff_t)-123);
    CHECK_FORMAT_1("%jd",           "-123",          (intmax_t)-123);
    CHECK_FORMAT_1("%ji",           "-123",          (intmax_t)-123);

    // Integers: width < length
    CHECK_FORMAT_1("%1d",            "123",                (int)123);
    CHECK_FORMAT_1("%1i",            "123",                (int)123);
    CHECK_FORMAT_1("%1u",            "123",       (unsigned int)123);
    CHECK_FORMAT_1("%1ld",           "123",               (long)123);
    CHECK_FORMAT_1("%1li",           "123",               (long)123);
    CHECK_FORMAT_1("%1lu",           "123",      (unsigned long)123);
    CHECK_FORMAT_1("%1lld",          "123",          (long long)123);
    CHECK_FORMAT_1("%1lli",          "123",          (long long)123);
    CHECK_FORMAT_1("%1llu",          "123", (unsigned long long)123);
    CHECK_FORMAT_1("%1zd",           "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%1zi",           "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%1zu",           "123",             (size_t)123);
    CHECK_FORMAT_1("%1x",             "7b",                (int)123);

    CHECK_FORMAT_1("%1d",           "-123",               (int)-123);
    CHECK_FORMAT_1("%1i",           "-123",               (int)-123);
    CHECK_FORMAT_1("%1ld",          "-123",              (long)-123);
    CHECK_FORMAT_1("%1li",          "-123",              (long)-123);
    CHECK_FORMAT_1("%1lld",         "-123",         (long long)-123);
    CHECK_FORMAT_1("%1lli",         "-123",         (long long)-123);
    CHECK_FORMAT_1("%1zd",          "-123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%1zi",          "-123",        (Py_ssize_t)-123);

    // Integers: width > length
    CHECK_FORMAT_1("%5d",          "  123",                (int)123);
    CHECK_FORMAT_1("%5i",          "  123",                (int)123);
    CHECK_FORMAT_1("%5u",          "  123",       (unsigned int)123);
    CHECK_FORMAT_1("%5ld",         "  123",               (long)123);
    CHECK_FORMAT_1("%5li",         "  123",               (long)123);
    CHECK_FORMAT_1("%5lu",         "  123",      (unsigned long)123);
    CHECK_FORMAT_1("%5lld",        "  123",          (long long)123);
    CHECK_FORMAT_1("%5lli",        "  123",          (long long)123);
    CHECK_FORMAT_1("%5llu",        "  123", (unsigned long long)123);
    CHECK_FORMAT_1("%5zd",         "  123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%5zi",         "  123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%5zu",         "  123",             (size_t)123);
    CHECK_FORMAT_1("%5x",          "   7b",                (int)123);

    CHECK_FORMAT_1("%5d",          " -123",               (int)-123);
    CHECK_FORMAT_1("%5i",          " -123",               (int)-123);
    CHECK_FORMAT_1("%5ld",         " -123",              (long)-123);
    CHECK_FORMAT_1("%5li",         " -123",              (long)-123);
    CHECK_FORMAT_1("%5lld",        " -123",         (long long)-123);
    CHECK_FORMAT_1("%5lli",        " -123",         (long long)-123);
    CHECK_FORMAT_1("%5zd",         " -123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%5zi",         " -123",        (Py_ssize_t)-123);

    // Integers: width > length, 0-flag
    CHECK_FORMAT_1("%05d",         "00123",                (int)123);
    CHECK_FORMAT_1("%05i",         "00123",                (int)123);
    CHECK_FORMAT_1("%05u",         "00123",       (unsigned int)123);
    CHECK_FORMAT_1("%05ld",        "00123",               (long)123);
    CHECK_FORMAT_1("%05li",        "00123",               (long)123);
    CHECK_FORMAT_1("%05lu",        "00123",      (unsigned long)123);
    CHECK_FORMAT_1("%05lld",       "00123",          (long long)123);
    CHECK_FORMAT_1("%05lli",       "00123",          (long long)123);
    CHECK_FORMAT_1("%05llu",       "00123", (unsigned long long)123);
    CHECK_FORMAT_1("%05zd",        "00123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%05zi",        "00123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%05zu",        "00123",             (size_t)123);
    CHECK_FORMAT_1("%05x",         "0007b",                (int)123);

    CHECK_FORMAT_1("%05d",         "-0123",               (int)-123);
    CHECK_FORMAT_1("%05i",         "-0123",               (int)-123);
    CHECK_FORMAT_1("%05ld",        "-0123",              (long)-123);
    CHECK_FORMAT_1("%05li",        "-0123",              (long)-123);
    CHECK_FORMAT_1("%05lld",       "-0123",         (long long)-123);
    CHECK_FORMAT_1("%05lli",       "-0123",         (long long)-123);
    CHECK_FORMAT_1("%05zd",        "-0123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%05zi",        "-0123",        (Py_ssize_t)-123);

    // Integers: precision < length
    CHECK_FORMAT_1("%.1d",           "123",                (int)123);
    CHECK_FORMAT_1("%.1i",           "123",                (int)123);
    CHECK_FORMAT_1("%.1u",           "123",       (unsigned int)123);
    CHECK_FORMAT_1("%.1ld",          "123",               (long)123);
    CHECK_FORMAT_1("%.1li",          "123",               (long)123);
    CHECK_FORMAT_1("%.1lu",          "123",      (unsigned long)123);
    CHECK_FORMAT_1("%.1lld",         "123",          (long long)123);
    CHECK_FORMAT_1("%.1lli",         "123",          (long long)123);
    CHECK_FORMAT_1("%.1llu",         "123", (unsigned long long)123);
    CHECK_FORMAT_1("%.1zd",          "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%.1zi",          "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%.1zu",          "123",             (size_t)123);
    CHECK_FORMAT_1("%.1x",            "7b",                (int)123);

    CHECK_FORMAT_1("%.1d",          "-123",               (int)-123);
    CHECK_FORMAT_1("%.1i",          "-123",               (int)-123);
    CHECK_FORMAT_1("%.1ld",         "-123",              (long)-123);
    CHECK_FORMAT_1("%.1li",         "-123",              (long)-123);
    CHECK_FORMAT_1("%.1lld",        "-123",         (long long)-123);
    CHECK_FORMAT_1("%.1lli",        "-123",         (long long)-123);
    CHECK_FORMAT_1("%.1zd",         "-123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%.1zi",         "-123",        (Py_ssize_t)-123);

    // Integers: precision > length
    CHECK_FORMAT_1("%.5d",         "00123",                (int)123);
    CHECK_FORMAT_1("%.5i",         "00123",                (int)123);
    CHECK_FORMAT_1("%.5u",         "00123",       (unsigned int)123);
    CHECK_FORMAT_1("%.5ld",        "00123",               (long)123);
    CHECK_FORMAT_1("%.5li",        "00123",               (long)123);
    CHECK_FORMAT_1("%.5lu",        "00123",      (unsigned long)123);
    CHECK_FORMAT_1("%.5lld",       "00123",          (long long)123);
    CHECK_FORMAT_1("%.5lli",       "00123",          (long long)123);
    CHECK_FORMAT_1("%.5llu",       "00123", (unsigned long long)123);
    CHECK_FORMAT_1("%.5zd",        "00123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%.5zi",        "00123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%.5zu",        "00123",             (size_t)123);
    CHECK_FORMAT_1("%.5x",         "0007b",                (int)123);

    CHECK_FORMAT_1("%.5d",        "-00123",               (int)-123);
    CHECK_FORMAT_1("%.5i",        "-00123",               (int)-123);
    CHECK_FORMAT_1("%.5ld",       "-00123",              (long)-123);
    CHECK_FORMAT_1("%.5li",       "-00123",              (long)-123);
    CHECK_FORMAT_1("%.5lld",      "-00123",         (long long)-123);
    CHECK_FORMAT_1("%.5lli",      "-00123",         (long long)-123);
    CHECK_FORMAT_1("%.5zd",       "-00123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%.5zi",       "-00123",        (Py_ssize_t)-123);

    // Integers: width > precision > length
    CHECK_FORMAT_1("%7.5d",      "  00123",                (int)123);
    CHECK_FORMAT_1("%7.5i",      "  00123",                (int)123);
    CHECK_FORMAT_1("%7.5u",      "  00123",       (unsigned int)123);
    CHECK_FORMAT_1("%7.5ld",     "  00123",               (long)123);
    CHECK_FORMAT_1("%7.5li",     "  00123",               (long)123);
    CHECK_FORMAT_1("%7.5lu",     "  00123",      (unsigned long)123);
    CHECK_FORMAT_1("%7.5lld",    "  00123",          (long long)123);
    CHECK_FORMAT_1("%7.5lli",    "  00123",          (long long)123);
    CHECK_FORMAT_1("%7.5llu",    "  00123", (unsigned long long)123);
    CHECK_FORMAT_1("%7.5zd",     "  00123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%7.5zi",     "  00123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%7.5zu",     "  00123",             (size_t)123);
    CHECK_FORMAT_1("%7.5x",      "  0007b",                (int)123);

    CHECK_FORMAT_1("%7.5d",      " -00123",               (int)-123);
    CHECK_FORMAT_1("%7.5i",      " -00123",               (int)-123);
    CHECK_FORMAT_1("%7.5ld",     " -00123",              (long)-123);
    CHECK_FORMAT_1("%7.5li",     " -00123",              (long)-123);
    CHECK_FORMAT_1("%7.5lld",    " -00123",         (long long)-123);
    CHECK_FORMAT_1("%7.5lli",    " -00123",         (long long)-123);
    CHECK_FORMAT_1("%7.5zd",     " -00123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%7.5zi",     " -00123",        (Py_ssize_t)-123);

    // Integers: width > precision > length, 0-flag
    CHECK_FORMAT_1("%07.5d",     "0000123",                (int)123);
    CHECK_FORMAT_1("%07.5i",     "0000123",                (int)123);
    CHECK_FORMAT_1("%07.5u",     "0000123",       (unsigned int)123);
    CHECK_FORMAT_1("%07.5ld",    "0000123",               (long)123);
    CHECK_FORMAT_1("%07.5li",    "0000123",               (long)123);
    CHECK_FORMAT_1("%07.5lu",    "0000123",      (unsigned long)123);
    CHECK_FORMAT_1("%07.5lld",   "0000123",          (long long)123);
    CHECK_FORMAT_1("%07.5lli",   "0000123",          (long long)123);
    CHECK_FORMAT_1("%07.5llu",   "0000123", (unsigned long long)123);
    CHECK_FORMAT_1("%07.5zd",    "0000123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%07.5zi",    "0000123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%07.5zu",    "0000123",             (size_t)123);
    CHECK_FORMAT_1("%07.5x",     "000007b",                (int)123);

    CHECK_FORMAT_1("%07.5d",     "-000123",               (int)-123);
    CHECK_FORMAT_1("%07.5i",     "-000123",               (int)-123);
    CHECK_FORMAT_1("%07.5ld",    "-000123",              (long)-123);
    CHECK_FORMAT_1("%07.5li",    "-000123",              (long)-123);
    CHECK_FORMAT_1("%07.5lld",   "-000123",         (long long)-123);
    CHECK_FORMAT_1("%07.5lli",   "-000123",         (long long)-123);
    CHECK_FORMAT_1("%07.5zd",    "-000123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%07.5zi",    "-000123",        (Py_ssize_t)-123);

    // Integers: precision > width > length
    CHECK_FORMAT_1("%5.7d",      "0000123",                (int)123);
    CHECK_FORMAT_1("%5.7i",      "0000123",                (int)123);
    CHECK_FORMAT_1("%5.7u",      "0000123",       (unsigned int)123);
    CHECK_FORMAT_1("%5.7ld",     "0000123",               (long)123);
    CHECK_FORMAT_1("%5.7li",     "0000123",               (long)123);
    CHECK_FORMAT_1("%5.7lu",     "0000123",      (unsigned long)123);
    CHECK_FORMAT_1("%5.7lld",    "0000123",          (long long)123);
    CHECK_FORMAT_1("%5.7lli",    "0000123",          (long long)123);
    CHECK_FORMAT_1("%5.7llu",    "0000123", (unsigned long long)123);
    CHECK_FORMAT_1("%5.7zd",     "0000123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%5.7zi",     "0000123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%5.7zu",     "0000123",             (size_t)123);
    CHECK_FORMAT_1("%5.7x",      "000007b",                (int)123);

    CHECK_FORMAT_1("%5.7d",     "-0000123",               (int)-123);
    CHECK_FORMAT_1("%5.7i",     "-0000123",               (int)-123);
    CHECK_FORMAT_1("%5.7ld",    "-0000123",              (long)-123);
    CHECK_FORMAT_1("%5.7li",    "-0000123",              (long)-123);
    CHECK_FORMAT_1("%5.7lld",   "-0000123",         (long long)-123);
    CHECK_FORMAT_1("%5.7lli",   "-0000123",         (long long)-123);
    CHECK_FORMAT_1("%5.7zd",    "-0000123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%5.7zi",    "-0000123",        (Py_ssize_t)-123);

    // Integers: precision > width > length, 0-flag
    CHECK_FORMAT_1("%05.7d",     "0000123",                (int)123);
    CHECK_FORMAT_1("%05.7i",     "0000123",                (int)123);
    CHECK_FORMAT_1("%05.7u",     "0000123",       (unsigned int)123);
    CHECK_FORMAT_1("%05.7ld",    "0000123",               (long)123);
    CHECK_FORMAT_1("%05.7li",    "0000123",               (long)123);
    CHECK_FORMAT_1("%05.7lu",    "0000123",      (unsigned long)123);
    CHECK_FORMAT_1("%05.7lld",   "0000123",          (long long)123);
    CHECK_FORMAT_1("%05.7lli",   "0000123",          (long long)123);
    CHECK_FORMAT_1("%05.7llu",   "0000123", (unsigned long long)123);
    CHECK_FORMAT_1("%05.7zd",    "0000123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%05.7zi",    "0000123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%05.7zu",    "0000123",             (size_t)123);
    CHECK_FORMAT_1("%05.7x",     "000007b",                (int)123);

    CHECK_FORMAT_1("%05.7d",    "-0000123",               (int)-123);
    CHECK_FORMAT_1("%05.7i",    "-0000123",               (int)-123);
    CHECK_FORMAT_1("%05.7ld",   "-0000123",              (long)-123);
    CHECK_FORMAT_1("%05.7li",   "-0000123",              (long)-123);
    CHECK_FORMAT_1("%05.7lld",  "-0000123",         (long long)-123);
    CHECK_FORMAT_1("%05.7lli",  "-0000123",         (long long)-123);
    CHECK_FORMAT_1("%05.7zd",   "-0000123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%05.7zi",   "-0000123",        (Py_ssize_t)-123);

    // Integers: precision = 0, arg = 0 (empty string in C)
    CHECK_FORMAT_1("%.0d",             "0",                  (int)0);
    CHECK_FORMAT_1("%.0i",             "0",                  (int)0);
    CHECK_FORMAT_1("%.0u",             "0",         (unsigned int)0);
    CHECK_FORMAT_1("%.0ld",            "0",                 (long)0);
    CHECK_FORMAT_1("%.0li",            "0",                 (long)0);
    CHECK_FORMAT_1("%.0lu",            "0",        (unsigned long)0);
    CHECK_FORMAT_1("%.0lld",           "0",            (long long)0);
    CHECK_FORMAT_1("%.0lli",           "0",            (long long)0);
    CHECK_FORMAT_1("%.0llu",           "0",   (unsigned long long)0);
    CHECK_FORMAT_1("%.0zd",            "0",           (Py_ssize_t)0);
    CHECK_FORMAT_1("%.0zi",            "0",           (Py_ssize_t)0);
    CHECK_FORMAT_1("%.0zu",            "0",               (size_t)0);
    CHECK_FORMAT_1("%.0x",             "0",                  (int)0);

    // Strings
    CHECK_FORMAT_1("%s",     "None",  "None");
    CHECK_FORMAT_1("%ls",    "None", L"None");
    CHECK_FORMAT_1("%U",     "None", unicode);
    CHECK_FORMAT_1("%A",     "None", Py_None);
    CHECK_FORMAT_1("%S",     "None", Py_None);
    CHECK_FORMAT_1("%R",     "None", Py_None);
    CHECK_FORMAT_2("%V",     "None", unicode, "ignored");
    CHECK_FORMAT_2("%V",     "None",    NULL,    "None");
    CHECK_FORMAT_2("%lV",    "None",    NULL,   L"None");

    // Strings: width < length
    CHECK_FORMAT_1("%1s",    "None",  "None");
    CHECK_FORMAT_1("%1ls",   "None", L"None");
    CHECK_FORMAT_1("%1U",    "None", unicode);
    CHECK_FORMAT_1("%1A",    "None", Py_None);
    CHECK_FORMAT_1("%1S",    "None", Py_None);
    CHECK_FORMAT_1("%1R",    "None", Py_None);
    CHECK_FORMAT_2("%1V",    "None", unicode, "ignored");
    CHECK_FORMAT_2("%1V",    "None",    NULL,    "None");
    CHECK_FORMAT_2("%1lV",   "None",    NULL,    L"None");

    // Strings: width > length
    CHECK_FORMAT_1("%5s",   " None",  "None");
    CHECK_FORMAT_1("%5ls",  " None", L"None");
    CHECK_FORMAT_1("%5U",   " None", unicode);
    CHECK_FORMAT_1("%5A",   " None", Py_None);
    CHECK_FORMAT_1("%5S",   " None", Py_None);
    CHECK_FORMAT_1("%5R",   " None", Py_None);
    CHECK_FORMAT_2("%5V",   " None", unicode, "ignored");
    CHECK_FORMAT_2("%5V",   " None",    NULL,    "None");
    CHECK_FORMAT_2("%5lV",  " None",    NULL,   L"None");

    // Strings: precision < length
    CHECK_FORMAT_1("%.1s",      "N",  "None");
    CHECK_FORMAT_1("%.1ls",     "N", L"None");
    CHECK_FORMAT_1("%.1U",      "N", unicode);
    CHECK_FORMAT_1("%.1A",      "N", Py_None);
    CHECK_FORMAT_1("%.1S",      "N", Py_None);
    CHECK_FORMAT_1("%.1R",      "N", Py_None);
    CHECK_FORMAT_2("%.1V",      "N", unicode, "ignored");
    CHECK_FORMAT_2("%.1V",      "N",    NULL,    "None");
    CHECK_FORMAT_2("%.1lV",     "N",    NULL,   L"None");

    // Strings: precision > length
    CHECK_FORMAT_1("%.5s",   "None",  "None");
    CHECK_FORMAT_1("%.5ls",  "None", L"None");
    CHECK_FORMAT_1("%.5U",   "None", unicode);
    CHECK_FORMAT_1("%.5A",   "None", Py_None);
    CHECK_FORMAT_1("%.5S",   "None", Py_None);
    CHECK_FORMAT_1("%.5R",   "None", Py_None);
    CHECK_FORMAT_2("%.5V",   "None", unicode, "ignored");
    CHECK_FORMAT_2("%.5V",   "None",    NULL,    "None");
    CHECK_FORMAT_2("%.5lV",  "None",    NULL,   L"None");

    // Strings: precision < length, width > length
    CHECK_FORMAT_1("%5.1s", "    N",  "None");
    CHECK_FORMAT_1("%5.1ls","    N", L"None");
    CHECK_FORMAT_1("%5.1U", "    N", unicode);
    CHECK_FORMAT_1("%5.1A", "    N", Py_None);
    CHECK_FORMAT_1("%5.1S", "    N", Py_None);
    CHECK_FORMAT_1("%5.1R", "    N", Py_None);
    CHECK_FORMAT_2("%5.1V", "    N", unicode, "ignored");
    CHECK_FORMAT_2("%5.1V", "    N",    NULL,    "None");
    CHECK_FORMAT_2("%5.1lV","    N",    NULL,   L"None");

    // Strings: width < length, precision > length
    CHECK_FORMAT_1("%1.5s",  "None",  "None");
    CHECK_FORMAT_1("%1.5ls", "None",  L"None");
    CHECK_FORMAT_1("%1.5U",  "None", unicode);
    CHECK_FORMAT_1("%1.5A",  "None", Py_None);
    CHECK_FORMAT_1("%1.5S",  "None", Py_None);
    CHECK_FORMAT_1("%1.5R",  "None", Py_None);
    CHECK_FORMAT_2("%1.5V",  "None", unicode, "ignored");
    CHECK_FORMAT_2("%1.5V",  "None",    NULL,    "None");
    CHECK_FORMAT_2("%1.5lV", "None",    NULL,   L"None");

    Py_XDECREF(unicode);
    Py_RETURN_NONE;

 Fail:
    Py_XDECREF(result);
    Py_XDECREF(unicode);
    return NULL;

#undef CHECK_FORMAT_2
#undef CHECK_FORMAT_1
#undef CHECK_FORMAT_0
}

static PyMethodDef TestMethods[] = {
    {"codec_incrementalencoder", codec_incrementalencoder,       METH_VARARGS},
    {"codec_incrementaldecoder", codec_incrementaldecoder,       METH_VARARGS},
    {"test_unicode_compare_with_ascii",
     test_unicode_compare_with_ascii,                            METH_NOARGS},
    {"test_string_from_format",  test_string_from_format,        METH_NOARGS},
    {"test_widechar",            test_widechar,                  METH_NOARGS},
    {"unicode_new",              unicode_new,                    METH_VARARGS},
    {"unicode_fill",             unicode_fill,                   METH_VARARGS},
    {"unicode_writechar",        unicode_writechar,              METH_VARARGS},
    {"unicode_resize",           unicode_resize,                 METH_VARARGS},
    {"unicode_append",           unicode_append,                 METH_VARARGS},
    {"unicode_appendanddel",     unicode_appendanddel,           METH_VARARGS},
    {"unicode_fromstringandsize",unicode_fromstringandsize,      METH_VARARGS},
    {"unicode_fromstring",       unicode_fromstring,             METH_O},
    {"unicode_fromkindanddata",  unicode_fromkindanddata,        METH_VARARGS},
    {"unicode_substring",        unicode_substring,              METH_VARARGS},
    {"unicode_getlength",        unicode_getlength,              METH_O},
    {"unicode_readchar",         unicode_readchar,               METH_VARARGS},
    {"unicode_fromencodedobject",unicode_fromencodedobject,      METH_VARARGS},
    {"unicode_fromobject",       unicode_fromobject,             METH_O},
    {"unicode_interninplace",    unicode_interninplace,          METH_O},
    {"unicode_internfromstring", unicode_internfromstring,       METH_O},
    {"unicode_fromwidechar",     unicode_fromwidechar,           METH_VARARGS},
    {"unicode_aswidechar",       unicode_aswidechar,             METH_VARARGS},
    {"unicode_aswidechar_null",  unicode_aswidechar_null,        METH_VARARGS},
    {"unicode_aswidecharstring", unicode_aswidecharstring,       METH_VARARGS},
    {"unicode_aswidecharstring_null",unicode_aswidecharstring_null,METH_VARARGS},
    {"unicode_asucs4",           unicode_asucs4,                 METH_VARARGS},
    {"unicode_asucs4copy",       unicode_asucs4copy,             METH_VARARGS},
    {"unicode_fromordinal",      unicode_fromordinal,            METH_VARARGS},
    {"unicode_asutf8",           unicode_asutf8,                 METH_VARARGS},
    {"unicode_asutf8andsize",    unicode_asutf8andsize,          METH_VARARGS},
    {"unicode_asutf8andsize_null",unicode_asutf8andsize_null,    METH_VARARGS},
    {"unicode_getdefaultencoding",unicode_getdefaultencoding,    METH_NOARGS},
    {"unicode_decode",           unicode_decode,                 METH_VARARGS},
    {"unicode_asencodedstring",  unicode_asencodedstring,        METH_VARARGS},
    {"unicode_buildencodingmap", unicode_buildencodingmap,       METH_O},
    {"unicode_decodeutf7",       unicode_decodeutf7,             METH_VARARGS},
    {"unicode_decodeutf7stateful",unicode_decodeutf7stateful,    METH_VARARGS},
    {"unicode_decodeutf8",       unicode_decodeutf8,             METH_VARARGS},
    {"unicode_decodeutf8stateful",unicode_decodeutf8stateful,    METH_VARARGS},
    {"unicode_asutf8string",     unicode_asutf8string,           METH_O},
    {"unicode_decodeutf16",      unicode_decodeutf16,            METH_VARARGS},
    {"unicode_decodeutf16stateful",unicode_decodeutf16stateful,  METH_VARARGS},
    {"unicode_asutf16string",    unicode_asutf16string,          METH_O},
    {"unicode_decodeutf32",      unicode_decodeutf32,            METH_VARARGS},
    {"unicode_decodeutf32stateful",unicode_decodeutf32stateful,  METH_VARARGS},
    {"unicode_asutf32string",    unicode_asutf32string,          METH_O},
    {"unicode_decodeunicodeescape",unicode_decodeunicodeescape,  METH_VARARGS},
    {"unicode_asunicodeescapestring",unicode_asunicodeescapestring,METH_O},
    {"unicode_decoderawunicodeescape",unicode_decoderawunicodeescape,METH_VARARGS},
    {"unicode_asrawunicodeescapestring",unicode_asrawunicodeescapestring,METH_O},
    {"unicode_decodelatin1",     unicode_decodelatin1,           METH_VARARGS},
    {"unicode_aslatin1string",   unicode_aslatin1string,         METH_O},
    {"unicode_decodeascii",      unicode_decodeascii,            METH_VARARGS},
    {"unicode_asasciistring",    unicode_asasciistring,          METH_O},
    {"unicode_decodecharmap",    unicode_decodecharmap,          METH_VARARGS},
    {"unicode_ascharmapstring",  unicode_ascharmapstring,        METH_VARARGS},
#ifdef MS_WINDOWS
    {"unicode_decodembcs",       unicode_decodembcs,             METH_VARARGS},
    {"unicode_decodembcsstateful",unicode_decodembcsstateful,    METH_VARARGS},
    {"unicode_decodecodepagestateful",unicode_decodecodepagestateful,METH_VARARGS},
    {"unicode_asmbcsstring",     unicode_asmbcsstring,           METH_O},
    {"unicode_encodecodepage",   unicode_encodecodepage,         METH_VARARGS},
#endif /* MS_WINDOWS */
    {"unicode_decodelocaleandsize",unicode_decodelocaleandsize,  METH_VARARGS},
    {"unicode_decodelocale",     unicode_decodelocale,           METH_VARARGS},
    {"unicode_encodelocale",     unicode_encodelocale,           METH_VARARGS},
    {"unicode_decodefsdefault",  unicode_decodefsdefault,        METH_VARARGS},
    {"unicode_decodefsdefaultandsize",unicode_decodefsdefaultandsize,METH_VARARGS},
    {"unicode_encodefsdefault",  unicode_encodefsdefault,        METH_O},
    {"unicode_concat",           unicode_concat,                 METH_VARARGS},
    {"unicode_splitlines",       unicode_splitlines,             METH_VARARGS},
    {"unicode_split",            unicode_split,                  METH_VARARGS},
    {"unicode_rsplit",           unicode_rsplit,                 METH_VARARGS},
    {"unicode_partition",        unicode_partition,              METH_VARARGS},
    {"unicode_rpartition",       unicode_rpartition,             METH_VARARGS},
    {"unicode_translate",        unicode_translate,              METH_VARARGS},
    {"unicode_join",             unicode_join,                   METH_VARARGS},
    {"unicode_count",            unicode_count,                  METH_VARARGS},
    {"unicode_tailmatch",        unicode_tailmatch,              METH_VARARGS},
    {"unicode_find",             unicode_find,                   METH_VARARGS},
    {"unicode_findchar",         unicode_findchar,               METH_VARARGS},
    {"unicode_replace",          unicode_replace,                METH_VARARGS},
    {"unicode_compare",          unicode_compare,                METH_VARARGS},
    {"unicode_comparewithasciistring",unicode_comparewithasciistring,METH_VARARGS},
    {"unicode_equaltoutf8",      unicode_equaltoutf8,            METH_VARARGS},
    {"unicode_equaltoutf8andsize",unicode_equaltoutf8andsize,    METH_VARARGS},
    {"unicode_richcompare",      unicode_richcompare,            METH_VARARGS},
    {"unicode_format",           unicode_format,                 METH_VARARGS},
    {"unicode_contains",         unicode_contains,               METH_VARARGS},
    {"unicode_isidentifier",     unicode_isidentifier,           METH_O},
    {"unicode_copycharacters",   unicode_copycharacters,         METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Unicode(PyObject *m) {
    _testcapimodule = PyModule_GetDef(m);

    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    return 0;
}
