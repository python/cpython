#include "parts.h"

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
        PyMem_Free(buffer);
        return NULL;
    }

    result = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, buffer, buf_len);
    PyMem_Free(buffer);
    return result;
}

static PyObject *
unicode_asutf8(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    const char *buffer;

    if (!PyArg_ParseTuple(args, "U", &unicode)) {
        return NULL;
    }

    buffer = PyUnicode_AsUTF8(unicode);
    if (buffer == NULL) {
        return NULL;
    }

    return PyBytes_FromString(buffer);
}

static PyObject *
unicode_asutf8andsize(PyObject *self, PyObject *args)
{
    PyObject *unicode, *result;
    const char *buffer;
    Py_ssize_t utf8_len;

    if(!PyArg_ParseTuple(args, "U", &unicode)) {
        return NULL;
    }

    buffer = PyUnicode_AsUTF8AndSize(unicode, &utf8_len);
    if (buffer == NULL) {
        return NULL;
    }

    result = PyBytes_FromString(buffer);
    if (result == NULL) {
        return NULL;
    }

    return Py_BuildValue("(Nn)", result, utf8_len);
}

static PyObject *
unicode_count(PyObject *self, PyObject *args)
{
    PyObject *str;
    PyObject *substr;
    Py_ssize_t result;
    Py_ssize_t start, end;

    if (!PyArg_ParseTuple(args, "UUnn:unicode_count", &str, &substr,
                          &start, &end)) {
        return NULL;
    }

    result = PyUnicode_Count(str, substr, start, end);
    if (result == -1)
        return NULL;
    else
        return PyLong_FromSsize_t(result);
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
    else if (!_PyUnicode_EqualToASCIIString(result, EXPECTED)) {    \
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
    CHECK_FORMAT_1( "%2c", "c", 'c');
    CHECK_FORMAT_1("%02c", "c", 'c');
    CHECK_FORMAT_1("%.0c", "c", 'c');
    CHECK_FORMAT_1("%.2c", "c", 'c');

    // Integers
    CHECK_FORMAT_1("%d",             "123",                (int)123);
    CHECK_FORMAT_1("%i",             "123",                (int)123);
    CHECK_FORMAT_1("%u",             "123",       (unsigned int)123);
    CHECK_FORMAT_1("%ld",            "123",               (long)123);
    CHECK_FORMAT_1("%li",            "123",               (long)123);
    CHECK_FORMAT_1("%lu",            "123",      (unsigned long)123);
    CHECK_FORMAT_1("%lld",           "123",          (long long)123);
    CHECK_FORMAT_1("%lli",           "123",          (long long)123);
    CHECK_FORMAT_1("%llu",           "123", (unsigned long long)123);
    CHECK_FORMAT_1("%zd",            "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%zi",            "123",         (Py_ssize_t)123);
    CHECK_FORMAT_1("%zu",            "123",             (size_t)123);
    CHECK_FORMAT_1("%x",              "7b",                (int)123);

    CHECK_FORMAT_1("%d",            "-123",               (int)-123);
    CHECK_FORMAT_1("%i",            "-123",               (int)-123);
    CHECK_FORMAT_1("%ld",           "-123",              (long)-123);
    CHECK_FORMAT_1("%li",           "-123",              (long)-123);
    CHECK_FORMAT_1("%lld",          "-123",         (long long)-123);
    CHECK_FORMAT_1("%lli",          "-123",         (long long)-123);
    CHECK_FORMAT_1("%zd",           "-123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%zi",           "-123",        (Py_ssize_t)-123);
    CHECK_FORMAT_1("%x",        "ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%1x",       "ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%9x",      " ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%09x",     "0ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%.1x",      "ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%.9x",     "0ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%10.9x",  " 0ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%010.9x", "00ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%9.10x",  "00ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%09.10x", "00ffffff85",               (int)-123);

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
    CHECK_FORMAT_1("%U",     "None", unicode);
    CHECK_FORMAT_1("%A",     "None", Py_None);
    CHECK_FORMAT_1("%S",     "None", Py_None);
    CHECK_FORMAT_1("%R",     "None", Py_None);
    CHECK_FORMAT_2("%V",     "None", unicode, "ignored");
    CHECK_FORMAT_2("%V",     "None",    NULL,    "None");

    // Strings: width < length
    CHECK_FORMAT_1("%1s",    "None",  "None");
    CHECK_FORMAT_1("%1U",    "None", unicode);
    CHECK_FORMAT_1("%1A",    "None", Py_None);
    CHECK_FORMAT_1("%1S",    "None", Py_None);
    CHECK_FORMAT_1("%1R",    "None", Py_None);
    CHECK_FORMAT_2("%1V",    "None", unicode, "ignored");
    CHECK_FORMAT_2("%1V",    "None",    NULL,    "None");

    // Strings: width > length
    CHECK_FORMAT_1("%5s",   " None",  "None");
    CHECK_FORMAT_1("%5U",   " None", unicode);
    CHECK_FORMAT_1("%5A",   " None", Py_None);
    CHECK_FORMAT_1("%5S",   " None", Py_None);
    CHECK_FORMAT_1("%5R",   " None", Py_None);
    CHECK_FORMAT_2("%5V",   " None", unicode, "ignored");
    CHECK_FORMAT_2("%5V",   " None",    NULL,    "None");

    // Strings: precision < length
    CHECK_FORMAT_1("%.1s",      "N",  "None");
    CHECK_FORMAT_1("%.1U",      "N", unicode);
    CHECK_FORMAT_1("%.1A",      "N", Py_None);
    CHECK_FORMAT_1("%.1S",      "N", Py_None);
    CHECK_FORMAT_1("%.1R",      "N", Py_None);
    CHECK_FORMAT_2("%.1V",      "N", unicode, "ignored");
    CHECK_FORMAT_2("%.1V",      "N",    NULL,    "None");

    // Strings: precision > length
    CHECK_FORMAT_1("%.5s",   "None",  "None");
    CHECK_FORMAT_1("%.5U",   "None", unicode);
    CHECK_FORMAT_1("%.5A",   "None", Py_None);
    CHECK_FORMAT_1("%.5S",   "None", Py_None);
    CHECK_FORMAT_1("%.5R",   "None", Py_None);
    CHECK_FORMAT_2("%.5V",   "None", unicode, "ignored");
    CHECK_FORMAT_2("%.5V",   "None",    NULL,    "None");

    // Strings: precision < length, width > length
    CHECK_FORMAT_1("%5.1s", "    N",  "None");
    CHECK_FORMAT_1("%5.1U", "    N", unicode);
    CHECK_FORMAT_1("%5.1A", "    N", Py_None);
    CHECK_FORMAT_1("%5.1S", "    N", Py_None);
    CHECK_FORMAT_1("%5.1R", "    N", Py_None);
    CHECK_FORMAT_2("%5.1V", "    N", unicode, "ignored");
    CHECK_FORMAT_2("%5.1V", "    N",    NULL,    "None");

    // Strings: width < length, precision > length
    CHECK_FORMAT_1("%1.5s",  "None",  "None");
    CHECK_FORMAT_1("%1.5U",  "None", unicode);
    CHECK_FORMAT_1("%1.5A",  "None", Py_None);
    CHECK_FORMAT_1("%1.5S",  "None", Py_None);
    CHECK_FORMAT_1("%1.5R",  "None", Py_None);
    CHECK_FORMAT_2("%1.5V",  "None", unicode, "ignored");
    CHECK_FORMAT_2("%1.5V",  "None",    NULL,    "None");

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
    {"unicode_aswidechar",       unicode_aswidechar,             METH_VARARGS},
    {"unicode_aswidecharstring", unicode_aswidecharstring,       METH_VARARGS},
    {"unicode_asucs4",           unicode_asucs4,                 METH_VARARGS},
    {"unicode_asutf8",           unicode_asutf8,                 METH_VARARGS},
    {"unicode_asutf8andsize",    unicode_asutf8andsize,          METH_VARARGS},
    {"unicode_count",            unicode_count,                  METH_VARARGS},
    {"unicode_findchar",         unicode_findchar,               METH_VARARGS},
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
