/*
 * wchar_t helpers, version CPython >= 3.3.
 *
 * CPython 3.3 added support for sys.maxunicode == 0x10FFFF on all
 * platforms, even ones with wchar_t limited to 2 bytes.  As such,
 * this code here works from the outside like wchar_helper.h in the
 * case Py_UNICODE_SIZE == 4, but the implementation is very different.
 */

typedef uint16_t cffi_char16_t;
typedef uint32_t cffi_char32_t;


static PyObject *
_my_PyUnicode_FromChar32(const cffi_char32_t *w, Py_ssize_t size)
{
    return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, w, size);
}

static PyObject *
_my_PyUnicode_FromChar16(const cffi_char16_t *w, Py_ssize_t size)
{
    /* are there any surrogate pairs, and if so, how many? */
    Py_ssize_t i, count_surrogates = 0;
    for (i = 0; i < size - 1; i++) {
        if (0xD800 <= w[i] && w[i] <= 0xDBFF &&
                0xDC00 <= w[i+1] && w[i+1] <= 0xDFFF)
            count_surrogates++;
    }
    if (count_surrogates == 0) {
        /* no, fast path */
        return PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, w, size);
    }
    else
    {
        PyObject *result = PyUnicode_New(size - count_surrogates, 0x10FFFF);
        Py_UCS4 *data;
        assert(PyUnicode_KIND(result) == PyUnicode_4BYTE_KIND);
        data = PyUnicode_4BYTE_DATA(result);

        for (i = 0; i < size; i++)
        {
            cffi_char32_t ch = w[i];
            if (0xD800 <= ch && ch <= 0xDBFF && i < size - 1) {
                cffi_char32_t ch2 = w[i + 1];
                if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
                    ch = (((ch & 0x3FF)<<10) | (ch2 & 0x3FF)) + 0x10000;
                    i++;
                }
            }
            *data++ = ch;
        }
        return result;
    }
}

static int
_my_PyUnicode_AsSingleChar16(PyObject *unicode, cffi_char16_t *result,
                             char *err_got)
{
    cffi_char32_t ch;
    if (PyUnicode_GET_LENGTH(unicode) != 1) {
        sprintf(err_got, "unicode string of length %zd",
                PyUnicode_GET_LENGTH(unicode));
        return -1;
    }
    ch = PyUnicode_READ_CHAR(unicode, 0);

    if (ch > 0xFFFF)
    {
        sprintf(err_got, "larger-than-0xFFFF character");
        return -1;
    }
    *result = (cffi_char16_t)ch;
    return 0;
}

static int
_my_PyUnicode_AsSingleChar32(PyObject *unicode, cffi_char32_t *result,
                             char *err_got)
{
    if (PyUnicode_GET_LENGTH(unicode) != 1) {
        sprintf(err_got, "unicode string of length %zd",
                PyUnicode_GET_LENGTH(unicode));
        return -1;
    }
    *result = PyUnicode_READ_CHAR(unicode, 0);
    return 0;
}

static Py_ssize_t _my_PyUnicode_SizeAsChar16(PyObject *unicode)
{
    Py_ssize_t length = PyUnicode_GET_LENGTH(unicode);
    Py_ssize_t result = length;
    unsigned int kind = PyUnicode_KIND(unicode);

    if (kind == PyUnicode_4BYTE_KIND)
    {
        Py_UCS4 *data = PyUnicode_4BYTE_DATA(unicode);
        Py_ssize_t i;
        for (i = 0; i < length; i++) {
            if (data[i] > 0xFFFF)
                result++;
        }
    }
    return result;
}

static Py_ssize_t _my_PyUnicode_SizeAsChar32(PyObject *unicode)
{
    return PyUnicode_GET_LENGTH(unicode);
}

static int _my_PyUnicode_AsChar16(PyObject *unicode,
                                  cffi_char16_t *result,
                                  Py_ssize_t resultlen)
{
    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    unsigned int kind = PyUnicode_KIND(unicode);
    void *data = PyUnicode_DATA(unicode);
    Py_ssize_t i;

    for (i = 0; i < len; i++) {
        cffi_char32_t ordinal = PyUnicode_READ(kind, data, i);
        if (ordinal > 0xFFFF) {
            if (ordinal > 0x10FFFF) {
                PyErr_Format(PyExc_ValueError,
                             "unicode character out of range for "
                             "conversion to char16_t: 0x%x", (int)ordinal);
                return -1;
            }
            ordinal -= 0x10000;
            *result++ = 0xD800 | (ordinal >> 10);
            *result++ = 0xDC00 | (ordinal & 0x3FF);
        }
        else
            *result++ = ordinal;
    }
    return 0;
}

static int _my_PyUnicode_AsChar32(PyObject *unicode,
                                  cffi_char32_t *result,
                                  Py_ssize_t resultlen)
{
    if (PyUnicode_AsUCS4(unicode, (Py_UCS4 *)result, resultlen, 0) == NULL)
        return -1;
    return 0;
}
