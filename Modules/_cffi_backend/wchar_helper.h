/*
 * wchar_t helpers
 */

typedef uint16_t cffi_char16_t;
typedef uint32_t cffi_char32_t;


#if Py_UNICODE_SIZE == 2

/* Before Python 2.7, PyUnicode_FromWideChar is not able to convert
   wchar_t values greater than 65535 into two-unicode-characters surrogates.
   But even the Python 2.7 version doesn't detect wchar_t values that are
   out of range(1114112), and just returns nonsense.

   From cffi 1.11 we can't use it anyway, because we need a version
   with char32_t input types.
*/
static PyObject *
_my_PyUnicode_FromChar32(const cffi_char32_t *w, Py_ssize_t size)
{
    PyObject *unicode;
    register Py_ssize_t i;
    Py_ssize_t alloc;
    const cffi_char32_t *orig_w;

    alloc = size;
    orig_w = w;
    for (i = size; i > 0; i--) {
        if (*w > 0xFFFF)
            alloc++;
        w++;
    }
    w = orig_w;
    unicode = PyUnicode_FromUnicode(NULL, alloc);
    if (!unicode)
        return NULL;

    /* Copy the wchar_t data into the new object */
    {
        register Py_UNICODE *u;
        u = PyUnicode_AS_UNICODE(unicode);
        for (i = size; i > 0; i--) {
            if (*w > 0xFFFF) {
                cffi_char32_t ordinal;
                if (*w > 0x10FFFF) {
                    PyErr_Format(PyExc_ValueError,
                                 "char32_t out of range for "
                                 "conversion to unicode: 0x%x", (int)*w);
                    Py_DECREF(unicode);
                    return NULL;
                }
                ordinal = *w++;
                ordinal -= 0x10000;
                *u++ = 0xD800 | (ordinal >> 10);
                *u++ = 0xDC00 | (ordinal & 0x3FF);
            }
            else
                *u++ = *w++;
        }
    }
    return unicode;
}

static PyObject *
_my_PyUnicode_FromChar16(const cffi_char16_t *w, Py_ssize_t size)
{
    return PyUnicode_FromUnicode((const Py_UNICODE *)w, size);
}

#else   /* Py_UNICODE_SIZE == 4 */

static PyObject *
_my_PyUnicode_FromChar32(const cffi_char32_t *w, Py_ssize_t size)
{
    return PyUnicode_FromUnicode((const Py_UNICODE *)w, size);
}

static PyObject *
_my_PyUnicode_FromChar16(const cffi_char16_t *w, Py_ssize_t size)
{
    /* 'size' is the length of the 'w' array */
    PyObject *result = PyUnicode_FromUnicode(NULL, size);

    if (result != NULL) {
        Py_UNICODE *u_base = PyUnicode_AS_UNICODE(result);
        Py_UNICODE *u = u_base;

        if (size == 1) {      /* performance only */
            *u = (cffi_char32_t)*w;
        }
        else {
            while (size > 0) {
                cffi_char32_t ch = *w++;
                size--;
                if (0xD800 <= ch && ch <= 0xDBFF && size > 0) {
                    cffi_char32_t ch2 = *w;
                    if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
                        ch = (((ch & 0x3FF)<<10) | (ch2 & 0x3FF)) + 0x10000;
                        w++;
                        size--;
                    }
                }
                *u++ = ch;
            }
            if (PyUnicode_Resize(&result, u - u_base) < 0) {
                Py_DECREF(result);
                return NULL;
            }
        }
    }
    return result;
}

#endif


#define IS_SURROGATE(u)   (0xD800 <= (u)[0] && (u)[0] <= 0xDBFF &&   \
                           0xDC00 <= (u)[1] && (u)[1] <= 0xDFFF)
#define AS_SURROGATE(u)   (0x10000 + (((u)[0] - 0xD800) << 10) +     \
                                     ((u)[1] - 0xDC00))

static int
_my_PyUnicode_AsSingleChar16(PyObject *unicode, cffi_char16_t *result,
                             char *err_got)
{
    Py_UNICODE *u = PyUnicode_AS_UNICODE(unicode);
    if (PyUnicode_GET_SIZE(unicode) != 1) {
        sprintf(err_got, "unicode string of length %zd",
                PyUnicode_GET_SIZE(unicode));
        return -1;
    }
#if Py_UNICODE_SIZE == 4
    if (((unsigned int)u[0]) > 0xFFFF)
    {
        sprintf(err_got, "larger-than-0xFFFF character");
        return -1;
    }
#endif
    *result = (cffi_char16_t)u[0];
    return 0;
}

static int
_my_PyUnicode_AsSingleChar32(PyObject *unicode, cffi_char32_t *result,
                             char *err_got)
{
    Py_UNICODE *u = PyUnicode_AS_UNICODE(unicode);
    if (PyUnicode_GET_SIZE(unicode) == 1) {
        *result = (cffi_char32_t)u[0];
        return 0;
    }
#if Py_UNICODE_SIZE == 2
    if (PyUnicode_GET_SIZE(unicode) == 2 && IS_SURROGATE(u)) {
        *result = AS_SURROGATE(u);
        return 0;
    }
#endif
    sprintf(err_got, "unicode string of length %zd",
            PyUnicode_GET_SIZE(unicode));
    return -1;
}

static Py_ssize_t _my_PyUnicode_SizeAsChar16(PyObject *unicode)
{
    Py_ssize_t length = PyUnicode_GET_SIZE(unicode);
    Py_ssize_t result = length;

#if Py_UNICODE_SIZE == 4
    Py_UNICODE *u = PyUnicode_AS_UNICODE(unicode);
    Py_ssize_t i;

    for (i=0; i<length; i++) {
        if (u[i] > 0xFFFF)
            result++;
    }
#endif
    return result;
}

static Py_ssize_t _my_PyUnicode_SizeAsChar32(PyObject *unicode)
{
    Py_ssize_t length = PyUnicode_GET_SIZE(unicode);
    Py_ssize_t result = length;

#if Py_UNICODE_SIZE == 2
    Py_UNICODE *u = PyUnicode_AS_UNICODE(unicode);
    Py_ssize_t i;

    for (i=0; i<length-1; i++) {
        if (IS_SURROGATE(u+i))
            result--;
    }
#endif
    return result;
}

static int _my_PyUnicode_AsChar16(PyObject *unicode,
                                  cffi_char16_t *result,
                                  Py_ssize_t resultlen)
{
    Py_ssize_t len = PyUnicode_GET_SIZE(unicode);
    Py_UNICODE *u = PyUnicode_AS_UNICODE(unicode);
    Py_ssize_t i;
    for (i=0; i<len; i++) {
#if Py_UNICODE_SIZE == 2
        cffi_char16_t ordinal = u[i];
#else
        cffi_char32_t ordinal = u[i];
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
            continue;
        }
#endif
        *result++ = ordinal;
    }
    return 0;
}

static int _my_PyUnicode_AsChar32(PyObject *unicode,
                                  cffi_char32_t *result,
                                  Py_ssize_t resultlen)
{
    Py_UNICODE *u = PyUnicode_AS_UNICODE(unicode);
    Py_ssize_t i;
    for (i=0; i<resultlen; i++) {
        cffi_char32_t ordinal = *u;
#if Py_UNICODE_SIZE == 2
        if (IS_SURROGATE(u)) {
            ordinal = AS_SURROGATE(u);
            u++;
        }
#endif
        result[i] = ordinal;
        u++;
    }
    return 0;
}
