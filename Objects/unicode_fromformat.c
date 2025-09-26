#include "Python.h"
#include "pycore_format.h"        // F_LJUST
#include "pycore_typeobject.h"    // _PyType_GetFullyQualifiedName()
#include "pycore_unicodeobject.h" // _PyUnicode_FindMaxChar()


/* Compilation of templated routines */

#include "stringlib/ucs1lib.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"


/* maximum number of characters required for output of %jo or %jd or %p.
   We need at most ceil(log8(256)*sizeof(intmax_t)) digits,
   plus 1 for the sign, plus 2 for the 0x prefix (for %p),
   plus 1 for the terminal NUL. */
#define MAX_INTMAX_CHARS (5 + (sizeof(intmax_t)*8-1) / 3)

static int
unicode_fromformat_write_str(_PyUnicodeWriter *writer, PyObject *str,
                             Py_ssize_t width, Py_ssize_t precision, int flags)
{
    Py_ssize_t length, fill, arglen;
    Py_UCS4 maxchar;

    length = PyUnicode_GET_LENGTH(str);
    if ((precision == -1 || precision >= length)
        && width <= length)
        return _PyUnicodeWriter_WriteStr(writer, str);

    if (precision != -1)
        length = Py_MIN(precision, length);

    arglen = Py_MAX(length, width);
    if (PyUnicode_MAX_CHAR_VALUE(str) > writer->maxchar)
        maxchar = _PyUnicode_FindMaxChar(str, 0, length);
    else
        maxchar = writer->maxchar;

    if (_PyUnicodeWriter_Prepare(writer, arglen, maxchar) == -1)
        return -1;

    fill = Py_MAX(width - length, 0);
    if (fill && !(flags & F_LJUST)) {
        if (PyUnicode_Fill(writer->buffer, writer->pos, fill, ' ') == -1)
            return -1;
        writer->pos += fill;
    }

    _PyUnicode_FastCopyCharacters(writer->buffer, writer->pos,
                                  str, 0, length);
    writer->pos += length;

    if (fill && (flags & F_LJUST)) {
        if (PyUnicode_Fill(writer->buffer, writer->pos, fill, ' ') == -1)
            return -1;
        writer->pos += fill;
    }

    return 0;
}

static int
unicode_fromformat_write_utf8(_PyUnicodeWriter *writer, const char *str,
                              Py_ssize_t width, Py_ssize_t precision, int flags)
{
    /* UTF-8 */
    Py_ssize_t *pconsumed = NULL;
    Py_ssize_t length;
    if (precision == -1) {
        length = strlen(str);
    }
    else {
        length = 0;
        while (length < precision && str[length]) {
            length++;
        }
        if (length == precision) {
            /* The input string is not NUL-terminated.  If it ends with an
             * incomplete UTF-8 sequence, truncate the string just before it.
             * Incomplete sequences in the middle and sequences which cannot
             * be valid prefixes are still treated as errors and replaced
             * with \xfffd. */
            pconsumed = &length;
        }
    }

    if (width < 0) {
        return _PyUnicode_DecodeUTF8Writer(writer, str, length,
                                           _Py_ERROR_REPLACE, "replace", pconsumed);
    }

    PyObject *unicode = PyUnicode_DecodeUTF8Stateful(str, length,
                                                     "replace", pconsumed);
    if (unicode == NULL)
        return -1;

    int res = unicode_fromformat_write_str(writer, unicode,
                                           width, -1, flags);
    Py_DECREF(unicode);
    return res;
}

static int
unicode_fromformat_write_wcstr(_PyUnicodeWriter *writer, const wchar_t *str,
                              Py_ssize_t width, Py_ssize_t precision, int flags)
{
    Py_ssize_t length;
    if (precision == -1) {
        length = wcslen(str);
    }
    else {
        length = 0;
        while (length < precision && str[length]) {
            length++;
        }
    }

    if (width < 0) {
        return PyUnicodeWriter_WriteWideChar((PyUnicodeWriter*)writer,
                                             str, length);
    }

    PyObject *unicode = PyUnicode_FromWideChar(str, length);
    if (unicode == NULL)
        return -1;

    int res = unicode_fromformat_write_str(writer, unicode, width, -1, flags);
    Py_DECREF(unicode);
    return res;
}

#define F_LONG 1
#define F_LONGLONG 2
#define F_SIZE 3
#define F_PTRDIFF 4
#define F_INTMAX 5

static const char*
unicode_fromformat_arg(_PyUnicodeWriter *writer,
                       const char *f, va_list *vargs)
{
    const char *p;
    Py_ssize_t len;
    int flags = 0;
    Py_ssize_t width;
    Py_ssize_t precision;

    p = f;
    f++;
    if (*f == '%') {
        if (_PyUnicodeWriter_WriteCharInline(writer, '%') < 0)
            return NULL;
        f++;
        return f;
    }

    /* Parse flags. Example: "%-i" => flags=F_LJUST. */
    /* Flags '+', ' ' and '#' are not particularly useful.
     * They are not worth the implementation and maintenance costs.
     * In addition, '#' should add "0" for "o" conversions for compatibility
     * with printf, but it would confuse Python users. */
    while (1) {
        switch (*f++) {
        case '-': flags |= F_LJUST; continue;
        case '0': flags |= F_ZERO; continue;
        case '#': flags |= F_ALT; continue;
        }
        f--;
        break;
    }

    /* parse the width.precision part, e.g. "%2.5s" => width=2, precision=5 */
    width = -1;
    if (*f == '*') {
        width = va_arg(*vargs, int);
        if (width < 0) {
            flags |= F_LJUST;
            width = -width;
        }
        f++;
    }
    else if (Py_ISDIGIT((unsigned)*f)) {
        width = *f - '0';
        f++;
        while (Py_ISDIGIT((unsigned)*f)) {
            if (width > (PY_SSIZE_T_MAX - ((int)*f - '0')) / 10) {
                PyErr_SetString(PyExc_ValueError,
                                "width too big");
                return NULL;
            }
            width = (width * 10) + (*f - '0');
            f++;
        }
    }
    precision = -1;
    if (*f == '.') {
        f++;
        if (*f == '*') {
            precision = va_arg(*vargs, int);
            if (precision < 0) {
                precision = -2;
            }
            f++;
        }
        else if (Py_ISDIGIT((unsigned)*f)) {
            precision = (*f - '0');
            f++;
            while (Py_ISDIGIT((unsigned)*f)) {
                if (precision > (PY_SSIZE_T_MAX - ((int)*f - '0')) / 10) {
                    PyErr_SetString(PyExc_ValueError,
                                    "precision too big");
                    return NULL;
                }
                precision = (precision * 10) + (*f - '0');
                f++;
            }
        }
    }

    int sizemod = 0;
    if (*f == 'l') {
        if (f[1] == 'l') {
            sizemod = F_LONGLONG;
            f += 2;
        }
        else {
            sizemod = F_LONG;
            ++f;
        }
    }
    else if (*f == 'z') {
        sizemod = F_SIZE;
        ++f;
    }
    else if (*f == 't') {
        sizemod = F_PTRDIFF;
        ++f;
    }
    else if (*f == 'j') {
        sizemod = F_INTMAX;
        ++f;
    }
    if (f[0] != '\0' && f[1] == '\0')
        writer->overallocate = 0;

    switch (*f) {
    case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
        break;
    case 'c': case 'p':
        if (sizemod || width >= 0 || precision >= 0) goto invalid_format;
        break;
    case 's':
    case 'V':
        if (sizemod && sizemod != F_LONG) goto invalid_format;
        break;
    default:
        if (sizemod) goto invalid_format;
        break;
    }

    switch (*f) {
    case 'c':
    {
        int ordinal = va_arg(*vargs, int);
        if (ordinal < 0 || ordinal > _Py_MAX_UNICODE) {
            PyErr_SetString(PyExc_OverflowError,
                            "character argument not in range(0x110000)");
            return NULL;
        }
        if (_PyUnicodeWriter_WriteCharInline(writer, ordinal) < 0)
            return NULL;
        break;
    }

    case 'd': case 'i':
    case 'o': case 'u': case 'x': case 'X':
    {
        char buffer[MAX_INTMAX_CHARS];

        // Fill buffer using sprinf, with one of many possible format
        // strings, like "%llX" for `long long` in hexadecimal.
        // The type/size is in `sizemod`; the format is in `*f`.

        // Use macros with nested switches to keep the sprintf format strings
        // as compile-time literals, avoiding warnings and maybe allowing
        // optimizations.

        // `SPRINT` macro does one sprintf
        // Example usage: SPRINT("l", "X", unsigned long) expands to
        // sprintf(buffer, "%" "l" "X", va_arg(*vargs, unsigned long))
        #define SPRINT(SIZE_SPEC, FMT_CHAR, TYPE) \
            sprintf(buffer, "%" SIZE_SPEC FMT_CHAR, va_arg(*vargs, TYPE))

        // One inner switch to handle all format variants
        #define DO_SPRINTS(SIZE_SPEC, SIGNED_TYPE, UNSIGNED_TYPE)             \
            switch (*f) {                                                     \
                case 'o': len = SPRINT(SIZE_SPEC, "o", UNSIGNED_TYPE); break; \
                case 'u': len = SPRINT(SIZE_SPEC, "u", UNSIGNED_TYPE); break; \
                case 'x': len = SPRINT(SIZE_SPEC, "x", UNSIGNED_TYPE); break; \
                case 'X': len = SPRINT(SIZE_SPEC, "X", UNSIGNED_TYPE); break; \
                default:  len = SPRINT(SIZE_SPEC, "d", SIGNED_TYPE); break;   \
            }

        // Outer switch to handle all the sizes/types
        switch (sizemod) {
            case F_LONG:     DO_SPRINTS("l", long, unsigned long); break;
            case F_LONGLONG: DO_SPRINTS("ll", long long, unsigned long long); break;
            case F_SIZE:     DO_SPRINTS("z", Py_ssize_t, size_t); break;
            case F_PTRDIFF:  DO_SPRINTS("t", ptrdiff_t, ptrdiff_t); break;
            case F_INTMAX:   DO_SPRINTS("j", intmax_t, uintmax_t); break;
            default:         DO_SPRINTS("", int, unsigned int); break;
        }
        #undef SPRINT
        #undef DO_SPRINTS

        assert(len >= 0);

        int sign = (buffer[0] == '-');
        len -= sign;

        precision = Py_MAX(precision, len);
        width = Py_MAX(width, precision + sign);
        if ((flags & F_ZERO) && !(flags & F_LJUST)) {
            precision = width - sign;
        }

        Py_ssize_t spacepad = Py_MAX(width - precision - sign, 0);
        Py_ssize_t zeropad = Py_MAX(precision - len, 0);

        if (_PyUnicodeWriter_Prepare(writer, width, 127) == -1)
            return NULL;

        if (spacepad && !(flags & F_LJUST)) {
            if (PyUnicode_Fill(writer->buffer, writer->pos, spacepad, ' ') == -1)
                return NULL;
            writer->pos += spacepad;
        }

        if (sign) {
            if (_PyUnicodeWriter_WriteChar(writer, '-') == -1)
                return NULL;
        }

        if (zeropad) {
            if (PyUnicode_Fill(writer->buffer, writer->pos, zeropad, '0') == -1)
                return NULL;
            writer->pos += zeropad;
        }

        if (_PyUnicodeWriter_WriteASCIIString(writer, &buffer[sign], len) < 0)
            return NULL;

        if (spacepad && (flags & F_LJUST)) {
            if (PyUnicode_Fill(writer->buffer, writer->pos, spacepad, ' ') == -1)
                return NULL;
            writer->pos += spacepad;
        }
        break;
    }

    case 'p':
    {
        char number[MAX_INTMAX_CHARS];

        len = sprintf(number, "%p", va_arg(*vargs, void*));
        assert(len >= 0);

        /* %p is ill-defined:  ensure leading 0x. */
        if (number[1] == 'X')
            number[1] = 'x';
        else if (number[1] != 'x') {
            memmove(number + 2, number,
                    strlen(number) + 1);
            number[0] = '0';
            number[1] = 'x';
            len += 2;
        }

        if (_PyUnicodeWriter_WriteASCIIString(writer, number, len) < 0)
            return NULL;
        break;
    }

    case 's':
    {
        if (sizemod) {
            const wchar_t *s = va_arg(*vargs, const wchar_t*);
            if (unicode_fromformat_write_wcstr(writer, s, width, precision, flags) < 0)
                return NULL;
        }
        else {
            /* UTF-8 */
            const char *s = va_arg(*vargs, const char*);
            if (unicode_fromformat_write_utf8(writer, s, width, precision, flags) < 0)
                return NULL;
        }
        break;
    }

    case 'U':
    {
        PyObject *obj = va_arg(*vargs, PyObject *);
        assert(obj && _PyUnicode_CHECK(obj));

        if (unicode_fromformat_write_str(writer, obj, width, precision, flags) == -1)
            return NULL;
        break;
    }

    case 'V':
    {
        PyObject *obj = va_arg(*vargs, PyObject *);
        const char *str;
        const wchar_t *wstr;
        if (sizemod) {
            wstr = va_arg(*vargs, const wchar_t*);
        }
        else {
            str = va_arg(*vargs, const char *);
        }
        if (obj) {
            assert(_PyUnicode_CHECK(obj));
            if (unicode_fromformat_write_str(writer, obj, width, precision, flags) == -1)
                return NULL;
        }
        else if (sizemod) {
            assert(wstr != NULL);
            if (unicode_fromformat_write_wcstr(writer, wstr, width, precision, flags) < 0)
                return NULL;
        }
        else {
            assert(str != NULL);
            if (unicode_fromformat_write_utf8(writer, str, width, precision, flags) < 0)
                return NULL;
        }
        break;
    }

    case 'S':
    {
        PyObject *obj = va_arg(*vargs, PyObject *);
        PyObject *str;
        assert(obj);
        str = PyObject_Str(obj);
        if (!str)
            return NULL;
        if (unicode_fromformat_write_str(writer, str, width, precision, flags) == -1) {
            Py_DECREF(str);
            return NULL;
        }
        Py_DECREF(str);
        break;
    }

    case 'R':
    {
        PyObject *obj = va_arg(*vargs, PyObject *);
        PyObject *repr;
        assert(obj);
        repr = PyObject_Repr(obj);
        if (!repr)
            return NULL;
        if (unicode_fromformat_write_str(writer, repr, width, precision, flags) == -1) {
            Py_DECREF(repr);
            return NULL;
        }
        Py_DECREF(repr);
        break;
    }

    case 'A':
    {
        PyObject *obj = va_arg(*vargs, PyObject *);
        PyObject *ascii;
        assert(obj);
        ascii = PyObject_ASCII(obj);
        if (!ascii)
            return NULL;
        if (unicode_fromformat_write_str(writer, ascii, width, precision, flags) == -1) {
            Py_DECREF(ascii);
            return NULL;
        }
        Py_DECREF(ascii);
        break;
    }

    case 'T':
    {
        PyObject *obj = va_arg(*vargs, PyObject *);
        PyTypeObject *type = (PyTypeObject *)Py_NewRef(Py_TYPE(obj));

        PyObject *type_name;
        if (flags & F_ALT) {
            type_name = _PyType_GetFullyQualifiedName(type, ':');
        }
        else {
            type_name = PyType_GetFullyQualifiedName(type);
        }
        Py_DECREF(type);
        if (!type_name) {
            return NULL;
        }

        if (unicode_fromformat_write_str(writer, type_name,
                                         width, precision, flags) == -1) {
            Py_DECREF(type_name);
            return NULL;
        }
        Py_DECREF(type_name);
        break;
    }

    case 'N':
    {
        PyObject *type_raw = va_arg(*vargs, PyObject *);
        assert(type_raw != NULL);

        if (!PyType_Check(type_raw)) {
            PyErr_SetString(PyExc_TypeError, "%N argument must be a type");
            return NULL;
        }
        PyTypeObject *type = (PyTypeObject*)type_raw;

        PyObject *type_name;
        if (flags & F_ALT) {
            type_name = _PyType_GetFullyQualifiedName(type, ':');
        }
        else {
            type_name = PyType_GetFullyQualifiedName(type);
        }
        if (!type_name) {
            return NULL;
        }
        if (unicode_fromformat_write_str(writer, type_name,
                                         width, precision, flags) == -1) {
            Py_DECREF(type_name);
            return NULL;
        }
        Py_DECREF(type_name);
        break;
    }

    default:
    invalid_format:
        PyErr_Format(PyExc_SystemError, "invalid format string: %s", p);
        return NULL;
    }

    f++;
    return f;
}

static int
unicode_from_format(_PyUnicodeWriter *writer, const char *format, va_list vargs)
{
    Py_ssize_t len = strlen(format);
    writer->min_length += len + 100;
    writer->overallocate = 1;

    // Copy varags to be able to pass a reference to a subfunction.
    va_list vargs2;
    va_copy(vargs2, vargs);

    // _PyUnicodeWriter_WriteASCIIString() below requires the format string
    // to be encoded to ASCII.
    int is_ascii = (ucs1lib_find_max_char((Py_UCS1*)format, (Py_UCS1*)format + len) < 128);
    if (!is_ascii) {
        Py_ssize_t i;
        for (i=0; i < len && (unsigned char)format[i] <= 127; i++);
        PyErr_Format(PyExc_ValueError,
            "PyUnicode_FromFormatV() expects an ASCII-encoded format "
            "string, got a non-ASCII byte: 0x%02x",
            (unsigned char)format[i]);
        goto fail;
    }

    for (const char *f = format; *f; ) {
        if (*f == '%') {
            f = unicode_fromformat_arg(writer, f, &vargs2);
            if (f == NULL)
                goto fail;
        }
        else {
            const char *p = strchr(f, '%');
            if (p != NULL) {
                len = p - f;
            }
            else {
                len = strlen(f);
                writer->overallocate = 0;
            }

            if (_PyUnicodeWriter_WriteASCIIString(writer, f, len) < 0) {
                goto fail;
            }
            f += len;
        }
    }
    va_end(vargs2);
    return 0;

  fail:
    va_end(vargs2);
    return -1;
}

PyObject *
PyUnicode_FromFormatV(const char *format, va_list vargs)
{
    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);

    if (unicode_from_format(&writer, format, vargs) < 0) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyUnicodeWriter_Finish(&writer);
}

PyObject *
PyUnicode_FromFormat(const char *format, ...)
{
    PyObject* ret;
    va_list vargs;

    va_start(vargs, format);
    ret = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    return ret;
}

int
_PyUnicodeWriter_FormatV(PyUnicodeWriter *writer, const char *format,
                         va_list vargs)
{
    _PyUnicodeWriter *_writer = (_PyUnicodeWriter*)writer;
    Py_ssize_t old_pos = _writer->pos;

    int res = unicode_from_format(_writer, format, vargs);

    if (res < 0) {
        _writer->pos = old_pos;
    }
    return res;
}

int
PyUnicodeWriter_Format(PyUnicodeWriter *writer, const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    int res = _PyUnicodeWriter_FormatV(writer, format, vargs);
    va_end(vargs);
    return res;
}
