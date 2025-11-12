/*

Unicode implementation based on original code by Fredrik Lundh,
modified by Marc-Andre Lemburg <mal@lemburg.com>.

Major speed upgrades to the method implementations at the Reykjavik
NeedForSpeed sprint, by Fredrik Lundh and Andrew Dalke.

Copyright (c) Corporation for National Research Initiatives.

--------------------------------------------------------------------
The original string type implementation is:

  Copyright (c) 1999 by Secret Labs AB
  Copyright (c) 1999 by Fredrik Lundh

By obtaining, using, and/or copying this software and/or its
associated documentation, you agree that you have read, understood,
and will comply with the following terms and conditions:

Permission to use, copy, modify, and distribute this software and its
associated documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies, and that both that copyright notice and this permission notice
appear in supporting documentation, and that the name of Secret Labs
AB or the author not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS.  IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
--------------------------------------------------------------------

*/

#include "Python.h"
#include "pycore_bytesobject.h"   // PyBytesWriter structure
#include "pycore_codecs.h"        // _PyCodec_Lookup()
#include "pycore_critical_section.h" // Py_*_CRITICAL_SECTION_SEQUENCE_FAST
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // _PyInterpreterState_GetFinalizing()
#include "pycore_object.h"        // _PyObject_Init()
#include "pycore_pylifecycle.h"   // _Py_SetFileSystemEncoding()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_runtime.h"       // _Py_LATIN1_CHR
#include "pycore_unicodeobject.h" // _Py_MAX_UNICODE

struct encoding_map;
#include "clinic/unicode_codecs.c.h"

#define MAX_UNICODE _Py_MAX_UNICODE
#define ensure_unicode _PyUnicode_EnsureUnicode
#define unicode_result _PyUnicode_Result

#define _PyUnicode_LENGTH(op) \
    (_PyASCIIObject_CAST(op)->length)

#define _Py_RETURN_UNICODE_EMPTY()   \
    do {                             \
        return _PyUnicode_GetEmpty();\
    } while (0)


/* Forward declaration */
static PyObject *
unicode_encode_utf8(PyObject *unicode, _Py_error_handler error_handler,
                    const char *errors);
static PyObject *
unicode_decode_utf8(const char *s, Py_ssize_t size,
                    _Py_error_handler error_handler, const char *errors,
                    Py_ssize_t *consumed);
static PyObject *
unicode_encode_call_errorhandler(const char *errors,
       PyObject **errorHandler,const char *encoding, const char *reason,
       PyObject *unicode, PyObject **exceptionObject,
       Py_ssize_t startpos, Py_ssize_t endpos, Py_ssize_t *newpos);
static void
raise_encode_exception(PyObject **exceptionObject,
                       const char *encoding,
                       PyObject *unicode,
                       Py_ssize_t startpos, Py_ssize_t endpos,
                       const char *reason);
static int
init_fs_codec(PyInterpreterState *interp);


/* Compilation of templated routines */

#define STRINGLIB_GET_EMPTY() _PyUnicode_GetEmpty()

#include "stringlib/asciilib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/find.h"
#include "stringlib/undef.h"

#include "stringlib/ucs1lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/find.h"
#include "stringlib/undef.h"

#include "stringlib/ucs2lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/find.h"
#include "stringlib/undef.h"

#include "stringlib/ucs4lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/find.h"
#include "stringlib/undef.h"

#undef STRINGLIB_GET_EMPTY


static inline char* _PyUnicode_UTF8(PyObject *op)
{
    return FT_ATOMIC_LOAD_PTR_ACQUIRE(_PyCompactUnicodeObject_CAST(op)->utf8);
}

static inline char* PyUnicode_UTF8(PyObject *op)
{
    assert(PyUnicode_Check(op));
    if (PyUnicode_IS_COMPACT_ASCII(op)) {
        return ((char*)(_PyASCIIObject_CAST(op) + 1));
    }
    else {
         return _PyUnicode_UTF8(op);
    }
}


static inline void PyUnicode_SET_UTF8(PyObject *op, char *utf8)
{
    FT_ATOMIC_STORE_PTR_RELEASE(_PyCompactUnicodeObject_CAST(op)->utf8, utf8);
}


static inline Py_ssize_t PyUnicode_UTF8_LENGTH(PyObject *op)
{
    assert(PyUnicode_Check(op));
    if (PyUnicode_IS_COMPACT_ASCII(op)) {
         return _PyASCIIObject_CAST(op)->length;
    }
    else {
         return _PyCompactUnicodeObject_CAST(op)->utf8_length;
    }
}


static inline void PyUnicode_SET_UTF8_LENGTH(PyObject *op, Py_ssize_t length)
{
    _PyCompactUnicodeObject_CAST(op)->utf8_length = length;
}


#define LATIN1 _Py_LATIN1_CHR

static PyObject*
get_latin1_char(Py_UCS1 ch)
{
    PyObject *o = LATIN1(ch);
    return o;
}


static inline Py_ssize_t
findchar(const void *s, int kind,
         Py_ssize_t size, Py_UCS4 ch,
         int direction)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        if ((Py_UCS1) ch != ch)
            return -1;
        if (direction > 0)
            return ucs1lib_find_char((const Py_UCS1 *) s, size, (Py_UCS1) ch);
        else
            return ucs1lib_rfind_char((const Py_UCS1 *) s, size, (Py_UCS1) ch);
    case PyUnicode_2BYTE_KIND:
        if ((Py_UCS2) ch != ch)
            return -1;
        if (direction > 0)
            return ucs2lib_find_char((const Py_UCS2 *) s, size, (Py_UCS2) ch);
        else
            return ucs2lib_rfind_char((const Py_UCS2 *) s, size, (Py_UCS2) ch);
    case PyUnicode_4BYTE_KIND:
        if (direction > 0)
            return ucs4lib_find_char((const Py_UCS4 *) s, size, ch);
        else
            return ucs4lib_rfind_char((const Py_UCS4 *) s, size, ch);
    default:
        Py_UNREACHABLE();
    }
}


static _Py_error_handler
get_error_handler_wide(const wchar_t *errors)
{
    if (errors == NULL || wcscmp(errors, L"strict") == 0) {
        return _Py_ERROR_STRICT;
    }
    if (wcscmp(errors, L"surrogateescape") == 0) {
        return _Py_ERROR_SURROGATEESCAPE;
    }
    if (wcscmp(errors, L"replace") == 0) {
        return _Py_ERROR_REPLACE;
    }
    if (wcscmp(errors, L"ignore") == 0) {
        return _Py_ERROR_IGNORE;
    }
    if (wcscmp(errors, L"backslashreplace") == 0) {
        return _Py_ERROR_BACKSLASHREPLACE;
    }
    if (wcscmp(errors, L"surrogatepass") == 0) {
        return _Py_ERROR_SURROGATEPASS;
    }
    if (wcscmp(errors, L"xmlcharrefreplace") == 0) {
        return _Py_ERROR_XMLCHARREFREPLACE;
    }
    return _Py_ERROR_OTHER;
}


static inline int
unicode_check_encoding_errors(const char *encoding, const char *errors)
{
    if (encoding == NULL && errors == NULL) {
        return 0;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
#ifndef Py_DEBUG
    /* In release mode, only check in development mode (-X dev) */
    if (!_PyInterpreterState_GetConfig(interp)->dev_mode) {
        return 0;
    }
#else
    /* Always check in debug mode */
#endif

    /* Avoid calling _PyCodec_Lookup() and PyCodec_LookupError() before the
       codec registry is ready: before_PyUnicode_InitEncodings() is called. */
    if (!interp->unicode.fs_codec.encoding) {
        return 0;
    }

    /* Disable checks during Python finalization. For example, it allows to
       call _PyObject_Dump() during finalization for debugging purpose. */
    if (_PyInterpreterState_GetFinalizing(interp) != NULL) {
        return 0;
    }

    if (encoding != NULL
        // Fast path for the most common built-in encodings. Even if the codec
        // is cached, _PyCodec_Lookup() decodes the bytes string from UTF-8 to
        // create a temporary Unicode string (the key in the cache).
        && strcmp(encoding, "utf-8") != 0
        && strcmp(encoding, "utf8") != 0
        && strcmp(encoding, "ascii") != 0)
    {
        PyObject *handler = _PyCodec_Lookup(encoding);
        if (handler == NULL) {
            return -1;
        }
        Py_DECREF(handler);
    }

    if (errors != NULL
        // Fast path for the most common built-in error handlers.
        && strcmp(errors, "strict") != 0
        && strcmp(errors, "ignore") != 0
        && strcmp(errors, "replace") != 0
        && strcmp(errors, "surrogateescape") != 0
        && strcmp(errors, "surrogatepass") != 0)
    {
        PyObject *handler = PyCodec_LookupError(errors);
        if (handler == NULL) {
            return -1;
        }
        Py_DECREF(handler);
    }
    return 0;
}


/* Implementation of the "backslashreplace" error handler for 8-bit encodings:
   ASCII, Latin1, UTF-8, etc. */
static char*
backslashreplace(PyBytesWriter *writer, char *str,
                 PyObject *unicode, Py_ssize_t collstart, Py_ssize_t collend)
{
    Py_ssize_t size, i;
    Py_UCS4 ch;
    int kind;
    const void *data;

    kind = PyUnicode_KIND(unicode);
    data = PyUnicode_DATA(unicode);

    size = 0;
    /* determine replacement size */
    for (i = collstart; i < collend; ++i) {
        Py_ssize_t incr;

        ch = PyUnicode_READ(kind, data, i);
        if (ch < 0x100)
            incr = 2+2;
        else if (ch < 0x10000)
            incr = 2+4;
        else {
            assert(ch <= MAX_UNICODE);
            incr = 2+8;
        }
        if (size > PY_SSIZE_T_MAX - incr) {
            PyErr_SetString(PyExc_OverflowError,
                            "encoded result is too long for a Python string");
            return NULL;
        }
        size += incr;
    }

    str = PyBytesWriter_GrowAndUpdatePointer(writer, size, str);
    if (str == NULL) {
        return NULL;
    }

    /* generate replacement */
    for (i = collstart; i < collend; ++i) {
        ch = PyUnicode_READ(kind, data, i);
        *str++ = '\\';
        if (ch >= 0x00010000) {
            *str++ = 'U';
            *str++ = Py_hexdigits[(ch>>28)&0xf];
            *str++ = Py_hexdigits[(ch>>24)&0xf];
            *str++ = Py_hexdigits[(ch>>20)&0xf];
            *str++ = Py_hexdigits[(ch>>16)&0xf];
            *str++ = Py_hexdigits[(ch>>12)&0xf];
            *str++ = Py_hexdigits[(ch>>8)&0xf];
        }
        else if (ch >= 0x100) {
            *str++ = 'u';
            *str++ = Py_hexdigits[(ch>>12)&0xf];
            *str++ = Py_hexdigits[(ch>>8)&0xf];
        }
        else
            *str++ = 'x';
        *str++ = Py_hexdigits[(ch>>4)&0xf];
        *str++ = Py_hexdigits[ch&0xf];
    }
    return str;
}


/* Implementation of the "xmlcharrefreplace" error handler for 8-bit encodings:
   ASCII, Latin1, UTF-8, etc. */
static char*
xmlcharrefreplace(PyBytesWriter *writer, char *str,
                  PyObject *unicode, Py_ssize_t collstart, Py_ssize_t collend)
{
    Py_ssize_t size, i;
    Py_UCS4 ch;
    int kind;
    const void *data;

    kind = PyUnicode_KIND(unicode);
    data = PyUnicode_DATA(unicode);

    size = 0;
    /* determine replacement size */
    for (i = collstart; i < collend; ++i) {
        Py_ssize_t incr;

        ch = PyUnicode_READ(kind, data, i);
        if (ch < 10)
            incr = 2+1+1;
        else if (ch < 100)
            incr = 2+2+1;
        else if (ch < 1000)
            incr = 2+3+1;
        else if (ch < 10000)
            incr = 2+4+1;
        else if (ch < 100000)
            incr = 2+5+1;
        else if (ch < 1000000)
            incr = 2+6+1;
        else {
            assert(ch <= MAX_UNICODE);
            incr = 2+7+1;
        }
        if (size > PY_SSIZE_T_MAX - incr) {
            PyErr_SetString(PyExc_OverflowError,
                            "encoded result is too long for a Python string");
            return NULL;
        }
        size += incr;
    }

    str = PyBytesWriter_GrowAndUpdatePointer(writer, size, str);
    if (str == NULL) {
        return NULL;
    }

    /* generate replacement */
    for (i = collstart; i < collend; ++i) {
        size = sprintf(str, "&#%d;", PyUnicode_READ(kind, data, i));
        if (size < 0) {
            return NULL;
        }
        str += size;
    }
    return str;
}


/* Normalize an encoding name like encodings.normalize_encoding()
   but allow to convert to lowercase if *to_lower* is true.
   Return 1 on success, or 0 on error (encoding is longer than lower_len-1). */
int
_Py_normalize_encoding(const char *encoding,
                       char *lower,
                       size_t lower_len,
                       int to_lower)
{
    const char *e;
    char *l;
    char *l_end;
    int punct;

    assert(encoding != NULL);

    e = encoding;
    l = lower;
    l_end = &lower[lower_len - 1];
    punct = 0;
    while (1) {
        char c = *e;
        if (c == 0) {
            break;
        }

        if (Py_ISALNUM(c) || c == '.') {
            if (punct && l != lower) {
                if (l == l_end) {
                    return 0;
                }
                *l++ = '_';
            }
            punct = 0;

            if (l == l_end) {
                return 0;
            }
            *l++ = to_lower ? Py_TOLOWER(c) : c;
        }
        else {
            punct = 1;
        }

        e++;
    }
    *l = '\0';
    return 1;
}


PyObject *
PyUnicode_Decode(const char *s,
                 Py_ssize_t size,
                 const char *encoding,
                 const char *errors)
{
    PyObject *buffer = NULL, *unicode;
    Py_buffer info;
    char buflower[11];   /* strlen("iso-8859-1\0") == 11, longest shortcut */

    if (unicode_check_encoding_errors(encoding, errors) < 0) {
        return NULL;
    }

    if (size == 0) {
        _Py_RETURN_UNICODE_EMPTY();
    }

    if (encoding == NULL) {
        return PyUnicode_DecodeUTF8Stateful(s, size, errors, NULL);
    }

    /* Shortcuts for common default encodings */
    if (_Py_normalize_encoding(encoding, buflower, sizeof(buflower), 1)) {
        char *lower = buflower;

        /* Fast paths */
        if (lower[0] == 'u' && lower[1] == 't' && lower[2] == 'f') {
            lower += 3;
            if (*lower == '_') {
                /* Match "utf8" and "utf_8" */
                lower++;
            }

            if (lower[0] == '8' && lower[1] == 0) {
                return PyUnicode_DecodeUTF8Stateful(s, size, errors, NULL);
            }
            else if (lower[0] == '1' && lower[1] == '6' && lower[2] == 0) {
                return PyUnicode_DecodeUTF16(s, size, errors, 0);
            }
            else if (lower[0] == '3' && lower[1] == '2' && lower[2] == 0) {
                return PyUnicode_DecodeUTF32(s, size, errors, 0);
            }
        }
        else {
            if (strcmp(lower, "ascii") == 0
                || strcmp(lower, "us_ascii") == 0) {
                return PyUnicode_DecodeASCII(s, size, errors);
            }
    #ifdef MS_WINDOWS
            else if (strcmp(lower, "mbcs") == 0) {
                return PyUnicode_DecodeMBCS(s, size, errors);
            }
    #endif
            else if (strcmp(lower, "latin1") == 0
                     || strcmp(lower, "latin_1") == 0
                     || strcmp(lower, "iso_8859_1") == 0
                     || strcmp(lower, "iso8859_1") == 0) {
                return PyUnicode_DecodeLatin1(s, size, errors);
            }
        }
    }

    /* Decode via the codec registry */
    buffer = NULL;
    if (PyBuffer_FillInfo(&info, NULL, (void *)s, size, 1, PyBUF_FULL_RO) < 0)
        goto onError;
    buffer = PyMemoryView_FromBuffer(&info);
    if (buffer == NULL)
        goto onError;
    unicode = _PyCodec_DecodeText(buffer, encoding, errors);
    if (unicode == NULL)
        goto onError;
    if (!PyUnicode_Check(unicode)) {
        PyErr_Format(PyExc_TypeError,
                     "'%.400s' decoder returned '%.400s' instead of 'str'; "
                     "use codecs.decode() to decode to arbitrary types",
                     encoding,
                     Py_TYPE(unicode)->tp_name);
        Py_DECREF(unicode);
        goto onError;
    }
    Py_DECREF(buffer);
    return unicode_result(unicode);

  onError:
    Py_XDECREF(buffer);
    return NULL;
}


PyAPI_FUNC(PyObject *)
PyUnicode_AsDecodedObject(PyObject *unicode,
                          const char *encoding,
                          const char *errors)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Decode via the codec registry */
    return PyCodec_Decode(unicode, encoding, errors);
}


PyAPI_FUNC(PyObject *)
PyUnicode_AsDecodedUnicode(PyObject *unicode,
                           const char *encoding,
                           const char *errors)
{
    PyObject *v;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        goto onError;
    }

    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Decode via the codec registry */
    v = PyCodec_Decode(unicode, encoding, errors);
    if (v == NULL)
        goto onError;
    if (!PyUnicode_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "'%.400s' decoder returned '%.400s' instead of 'str'; "
                     "use codecs.decode() to decode to arbitrary types",
                     encoding,
                     Py_TYPE(unicode)->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return unicode_result(v);

  onError:
    return NULL;
}


PyAPI_FUNC(PyObject *)
PyUnicode_AsEncodedObject(PyObject *unicode,
                          const char *encoding,
                          const char *errors)
{
    PyObject *v;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        goto onError;
    }

    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Encode via the codec registry */
    v = PyCodec_Encode(unicode, encoding, errors);
    if (v == NULL)
        goto onError;
    return v;

  onError:
    return NULL;
}


PyObject *
PyUnicode_AsEncodedString(PyObject *unicode,
                          const char *encoding,
                          const char *errors)
{
    PyObject *v;
    char buflower[11];   /* strlen("iso_8859_1\0") == 11, longest shortcut */

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    if (unicode_check_encoding_errors(encoding, errors) < 0) {
        return NULL;
    }

    if (encoding == NULL) {
        return _PyUnicode_AsUTF8String(unicode, errors);
    }

    /* Shortcuts for common default encodings */
    if (_Py_normalize_encoding(encoding, buflower, sizeof(buflower), 1)) {
        char *lower = buflower;

        /* Fast paths */
        if (lower[0] == 'u' && lower[1] == 't' && lower[2] == 'f') {
            lower += 3;
            if (*lower == '_') {
                /* Match "utf8" and "utf_8" */
                lower++;
            }

            if (lower[0] == '8' && lower[1] == 0) {
                return _PyUnicode_AsUTF8String(unicode, errors);
            }
            else if (lower[0] == '1' && lower[1] == '6' && lower[2] == 0) {
                return _PyUnicode_EncodeUTF16(unicode, errors, 0);
            }
            else if (lower[0] == '3' && lower[1] == '2' && lower[2] == 0) {
                return _PyUnicode_EncodeUTF32(unicode, errors, 0);
            }
        }
        else {
            if (strcmp(lower, "ascii") == 0
                || strcmp(lower, "us_ascii") == 0) {
                return _PyUnicode_AsASCIIString(unicode, errors);
            }
#ifdef MS_WINDOWS
            else if (strcmp(lower, "mbcs") == 0) {
                return PyUnicode_EncodeCodePage(CP_ACP, unicode, errors);
            }
#endif
            else if (strcmp(lower, "latin1") == 0 ||
                     strcmp(lower, "latin_1") == 0 ||
                     strcmp(lower, "iso_8859_1") == 0 ||
                     strcmp(lower, "iso8859_1") == 0) {
                return _PyUnicode_AsLatin1String(unicode, errors);
            }
        }
    }

    /* Encode via the codec registry */
    v = _PyCodec_EncodeText(unicode, encoding, errors);
    if (v == NULL)
        return NULL;

    /* The normal path */
    if (PyBytes_Check(v))
        return v;

    /* If the codec returns a buffer, raise a warning and convert to bytes */
    if (PyByteArray_Check(v)) {
        int error;
        PyObject *b;

        error = PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
            "encoder %s returned bytearray instead of bytes; "
            "use codecs.encode() to encode to arbitrary types",
            encoding);
        if (error) {
            Py_DECREF(v);
            return NULL;
        }

        b = PyBytes_FromStringAndSize(PyByteArray_AS_STRING(v),
                                      PyByteArray_GET_SIZE(v));
        Py_DECREF(v);
        return b;
    }

    PyErr_Format(PyExc_TypeError,
                 "'%.400s' encoder returned '%.400s' instead of 'bytes'; "
                 "use codecs.encode() to encode to arbitrary types",
                 encoding,
                 Py_TYPE(v)->tp_name);
    Py_DECREF(v);
    return NULL;
}


PyAPI_FUNC(PyObject *)
PyUnicode_AsEncodedUnicode(PyObject *unicode,
                           const char *encoding,
                           const char *errors)
{
    PyObject *v;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        goto onError;
    }

    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Encode via the codec registry */
    v = PyCodec_Encode(unicode, encoding, errors);
    if (v == NULL)
        goto onError;
    if (!PyUnicode_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "'%.400s' encoder returned '%.400s' instead of 'str'; "
                     "use codecs.encode() to encode to arbitrary types",
                     encoding,
                     Py_TYPE(v)->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return v;

  onError:
    return NULL;
}


PyObject *
PyUnicode_FromEncodedObject(PyObject *obj,
                            const char *encoding,
                            const char *errors)
{
    Py_buffer buffer;
    PyObject *v;

    if (obj == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* Decoding bytes objects is the most common case and should be fast */
    if (PyBytes_Check(obj)) {
        if (PyBytes_GET_SIZE(obj) == 0) {
            if (unicode_check_encoding_errors(encoding, errors) < 0) {
                return NULL;
            }
            _Py_RETURN_UNICODE_EMPTY();
        }
        return PyUnicode_Decode(
                PyBytes_AS_STRING(obj), PyBytes_GET_SIZE(obj),
                encoding, errors);
    }

    if (PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "decoding str is not supported");
        return NULL;
    }

    /* Retrieve a bytes buffer view through the PEP 3118 buffer interface */
    if (PyObject_GetBuffer(obj, &buffer, PyBUF_SIMPLE) < 0) {
        PyErr_Format(PyExc_TypeError,
                     "decoding to str: need a bytes-like object, %.80s found",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }

    if (buffer.len == 0) {
        PyBuffer_Release(&buffer);
        if (unicode_check_encoding_errors(encoding, errors) < 0) {
            return NULL;
        }
        _Py_RETURN_UNICODE_EMPTY();
    }

    v = PyUnicode_Decode((char*) buffer.buf, buffer.len, encoding, errors);
    PyBuffer_Release(&buffer);
    return v;
}


static PyObject *
unicode_encode_locale(PyObject *unicode, _Py_error_handler error_handler,
                      int current_locale)
{
    Py_ssize_t wlen;
    wchar_t *wstr = PyUnicode_AsWideCharString(unicode, &wlen);
    if (wstr == NULL) {
        return NULL;
    }

    if ((size_t)wlen != wcslen(wstr)) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        PyMem_Free(wstr);
        return NULL;
    }

    char *str;
    size_t error_pos;
    const char *reason;
    int res = _Py_EncodeLocaleEx(wstr, &str, &error_pos, &reason,
                                 current_locale, error_handler);
    PyMem_Free(wstr);

    if (res != 0) {
        if (res == -2) {
            PyObject *exc;
            exc = PyObject_CallFunction(PyExc_UnicodeEncodeError, "sOnns",
                    "locale", unicode,
                    (Py_ssize_t)error_pos,
                    (Py_ssize_t)(error_pos+1),
                    reason);
            if (exc != NULL) {
                PyCodec_StrictErrors(exc);
                Py_DECREF(exc);
            }
        }
        else if (res == -3) {
            PyErr_SetString(PyExc_ValueError, "unsupported error handler");
        }
        else {
            PyErr_NoMemory();
        }
        return NULL;
    }

    PyObject *bytes = PyBytes_FromString(str);
    PyMem_RawFree(str);
    return bytes;
}


PyObject *
PyUnicode_EncodeLocale(PyObject *unicode, const char *errors)
{
    _Py_error_handler error_handler = _Py_GetErrorHandler(errors);
    return unicode_encode_locale(unicode, error_handler, 1);
}


PyObject *
PyUnicode_EncodeFSDefault(PyObject *unicode)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_unicode_fs_codec *fs_codec = &interp->unicode.fs_codec;
    if (fs_codec->utf8) {
        return unicode_encode_utf8(unicode,
                                   fs_codec->error_handler,
                                   fs_codec->errors);
    }
#ifndef _Py_FORCE_UTF8_FS_ENCODING
    else if (fs_codec->encoding) {
        return PyUnicode_AsEncodedString(unicode,
                                         fs_codec->encoding,
                                         fs_codec->errors);
    }
#endif
    else {
        /* Before _PyUnicode_InitEncodings() is called, the Python codec
           machinery is not ready and so cannot be used:
           use wcstombs() in this case. */
        const PyConfig *config = _PyInterpreterState_GetConfig(interp);
        const wchar_t *filesystem_errors = config->filesystem_errors;
        assert(filesystem_errors != NULL);
        _Py_error_handler errors = get_error_handler_wide(filesystem_errors);
        assert(errors != _Py_ERROR_UNKNOWN);
#ifdef _Py_FORCE_UTF8_FS_ENCODING
        return unicode_encode_utf8(unicode, errors, NULL);
#else
        return unicode_encode_locale(unicode, errors, 0);
#endif
    }
}


static PyObject*
unicode_decode_locale(const char *str, Py_ssize_t len,
                      _Py_error_handler errors, int current_locale)
{
    if (str[len] != '\0' || (size_t)len != strlen(str))  {
        PyErr_SetString(PyExc_ValueError, "embedded null byte");
        return NULL;
    }

    wchar_t *wstr;
    size_t wlen;
    const char *reason;
    int res = _Py_DecodeLocaleEx(str, &wstr, &wlen, &reason,
                                 current_locale, errors);
    if (res != 0) {
        if (res == -2) {
            PyObject *exc;
            exc = PyObject_CallFunction(PyExc_UnicodeDecodeError, "sy#nns",
                                        "locale", str, len,
                                        (Py_ssize_t)wlen,
                                        (Py_ssize_t)(wlen + 1),
                                        reason);
            if (exc != NULL) {
                PyCodec_StrictErrors(exc);
                Py_DECREF(exc);
            }
        }
        else if (res == -3) {
            PyErr_SetString(PyExc_ValueError, "unsupported error handler");
        }
        else {
            PyErr_NoMemory();
        }
        return NULL;
    }

    PyObject *unicode = PyUnicode_FromWideChar(wstr, wlen);
    PyMem_RawFree(wstr);
    return unicode;
}


PyObject*
PyUnicode_DecodeLocaleAndSize(const char *str, Py_ssize_t len,
                              const char *errors)
{
    _Py_error_handler error_handler = _Py_GetErrorHandler(errors);
    return unicode_decode_locale(str, len, error_handler, 1);
}


PyObject*
PyUnicode_DecodeLocale(const char *str, const char *errors)
{
    Py_ssize_t size = (Py_ssize_t)strlen(str);
    _Py_error_handler error_handler = _Py_GetErrorHandler(errors);
    return unicode_decode_locale(str, size, error_handler, 1);
}


PyObject*
PyUnicode_DecodeFSDefault(const char *s) {
    Py_ssize_t size = (Py_ssize_t)strlen(s);
    return PyUnicode_DecodeFSDefaultAndSize(s, size);
}


PyObject*
PyUnicode_DecodeFSDefaultAndSize(const char *s, Py_ssize_t size)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_unicode_fs_codec *fs_codec = &interp->unicode.fs_codec;
    if (fs_codec->utf8) {
        return unicode_decode_utf8(s, size,
                                   fs_codec->error_handler,
                                   fs_codec->errors,
                                   NULL);
    }
#ifndef _Py_FORCE_UTF8_FS_ENCODING
    else if (fs_codec->encoding) {
        return PyUnicode_Decode(s, size,
                                fs_codec->encoding,
                                fs_codec->errors);
    }
#endif
    else {
        /* Before _PyUnicode_InitEncodings() is called, the Python codec
           machinery is not ready and so cannot be used:
           use mbstowcs() in this case. */
        const PyConfig *config = _PyInterpreterState_GetConfig(interp);
        const wchar_t *filesystem_errors = config->filesystem_errors;
        assert(filesystem_errors != NULL);
        _Py_error_handler errors = get_error_handler_wide(filesystem_errors);
        assert(errors != _Py_ERROR_UNKNOWN);
#ifdef _Py_FORCE_UTF8_FS_ENCODING
        return unicode_decode_utf8(s, size, errors, NULL, NULL);
#else
        return unicode_decode_locale(s, size, errors, 0);
#endif
    }
}


int
PyUnicode_FSConverter(PyObject* arg, void* addr)
{
    PyObject *path = NULL;
    PyObject *output = NULL;
    Py_ssize_t size;
    const char *data;
    if (arg == NULL) {
        Py_DECREF(*(PyObject**)addr);
        *(PyObject**)addr = NULL;
        return 1;
    }
    path = PyOS_FSPath(arg);
    if (path == NULL) {
        return 0;
    }
    if (PyBytes_Check(path)) {
        output = path;
    }
    else {  // PyOS_FSPath() guarantees its returned value is bytes or str.
        output = PyUnicode_EncodeFSDefault(path);
        Py_DECREF(path);
        if (!output) {
            return 0;
        }
        assert(PyBytes_Check(output));
    }

    size = PyBytes_GET_SIZE(output);
    data = PyBytes_AS_STRING(output);
    if ((size_t)size != strlen(data)) {
        PyErr_SetString(PyExc_ValueError, "embedded null byte");
        Py_DECREF(output);
        return 0;
    }
    *(PyObject**)addr = output;
    return Py_CLEANUP_SUPPORTED;
}


int
PyUnicode_FSDecoder(PyObject* arg, void* addr)
{
    if (arg == NULL) {
        Py_DECREF(*(PyObject**)addr);
        *(PyObject**)addr = NULL;
        return 1;
    }

    PyObject *path = PyOS_FSPath(arg);
    if (path == NULL) {
        return 0;
    }

    PyObject *output = NULL;
    if (PyUnicode_Check(path)) {
        output = path;
    }
    else if (PyBytes_Check(path)) {
        output = PyUnicode_DecodeFSDefaultAndSize(PyBytes_AS_STRING(path),
                                                  PyBytes_GET_SIZE(path));
        Py_DECREF(path);
        if (!output) {
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "path should be string, bytes, or os.PathLike, not %.200s",
                     Py_TYPE(arg)->tp_name);
        Py_DECREF(path);
        return 0;
    }

    if (findchar(PyUnicode_DATA(output), PyUnicode_KIND(output),
                 PyUnicode_GET_LENGTH(output), 0, 1) >= 0) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        Py_DECREF(output);
        return 0;
    }
    *(PyObject**)addr = output;
    return Py_CLEANUP_SUPPORTED;
}


/* create or adjust a UnicodeDecodeError */
static void
make_decode_exception(PyObject **exceptionObject,
                      const char *encoding,
                      const char *input, Py_ssize_t length,
                      Py_ssize_t startpos, Py_ssize_t endpos,
                      const char *reason)
{
    if (*exceptionObject == NULL) {
        *exceptionObject = PyUnicodeDecodeError_Create(
            encoding, input, length, startpos, endpos, reason);
    }
    else {
        if (PyUnicodeDecodeError_SetStart(*exceptionObject, startpos))
            goto onError;
        if (PyUnicodeDecodeError_SetEnd(*exceptionObject, endpos))
            goto onError;
        if (PyUnicodeDecodeError_SetReason(*exceptionObject, reason))
            goto onError;
    }
    return;

onError:
    Py_CLEAR(*exceptionObject);
}


#ifdef MS_WINDOWS
static int
widechar_resize(wchar_t **buf, Py_ssize_t *size, Py_ssize_t newsize)
{
    if (newsize > *size) {
        wchar_t *newbuf = *buf;
        if (PyMem_Resize(newbuf, wchar_t, newsize) == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        *buf = newbuf;
    }
    *size = newsize;
    return 0;
}

/* error handling callback helper:
   build arguments, call the callback and check the arguments,
   if no exception occurred, copy the replacement to the output
   and adjust various state variables.
   return 0 on success, -1 on error
*/

static int
unicode_decode_call_errorhandler_wchar(
    const char *errors, PyObject **errorHandler,
    const char *encoding, const char *reason,
    const char **input, const char **inend, Py_ssize_t *startinpos,
    Py_ssize_t *endinpos, PyObject **exceptionObject, const char **inptr,
    wchar_t **buf, Py_ssize_t *bufsize, Py_ssize_t *outpos)
{
    static const char *argparse = "Un;decoding error handler must return (str, int) tuple";

    PyObject *restuple = NULL;
    PyObject *repunicode = NULL;
    Py_ssize_t outsize;
    Py_ssize_t insize;
    Py_ssize_t requiredsize;
    Py_ssize_t newpos;
    PyObject *inputobj = NULL;
    Py_ssize_t repwlen;

    if (*errorHandler == NULL) {
        *errorHandler = PyCodec_LookupError(errors);
        if (*errorHandler == NULL)
            goto onError;
    }

    make_decode_exception(exceptionObject,
        encoding,
        *input, *inend - *input,
        *startinpos, *endinpos,
        reason);
    if (*exceptionObject == NULL)
        goto onError;

    restuple = PyObject_CallOneArg(*errorHandler, *exceptionObject);
    if (restuple == NULL)
        goto onError;
    if (!PyTuple_Check(restuple)) {
        PyErr_SetString(PyExc_TypeError, &argparse[3]);
        goto onError;
    }
    if (!PyArg_ParseTuple(restuple, argparse, &repunicode, &newpos))
        goto onError;

    /* Copy back the bytes variables, which might have been modified by the
       callback */
    inputobj = PyUnicodeDecodeError_GetObject(*exceptionObject);
    if (!inputobj)
        goto onError;
    *input = PyBytes_AS_STRING(inputobj);
    insize = PyBytes_GET_SIZE(inputobj);
    *inend = *input + insize;
    /* we can DECREF safely, as the exception has another reference,
       so the object won't go away. */
    Py_DECREF(inputobj);

    if (newpos<0)
        newpos = insize+newpos;
    if (newpos<0 || newpos>insize) {
        PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", newpos);
        goto onError;
    }

    repwlen = PyUnicode_AsWideChar(repunicode, NULL, 0);
    if (repwlen < 0)
        goto onError;
    repwlen--;
    /* need more space? (at least enough for what we
       have+the replacement+the rest of the string (starting
       at the new input position), so we won't have to check space
       when there are no errors in the rest of the string) */
    requiredsize = *outpos;
    if (requiredsize > PY_SSIZE_T_MAX - repwlen)
        goto overflow;
    requiredsize += repwlen;
    if (requiredsize > PY_SSIZE_T_MAX - (insize - newpos))
        goto overflow;
    requiredsize += insize - newpos;
    outsize = *bufsize;
    if (requiredsize > outsize) {
        if (outsize <= PY_SSIZE_T_MAX/2 && requiredsize < 2*outsize)
            requiredsize = 2*outsize;
        if (widechar_resize(buf, bufsize, requiredsize) < 0) {
            goto onError;
        }
    }
    PyUnicode_AsWideChar(repunicode, *buf + *outpos, repwlen);
    *outpos += repwlen;
    *endinpos = newpos;
    *inptr = *input + newpos;

    /* we made it! */
    Py_DECREF(restuple);
    return 0;

  overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "decoded result is too long for a Python string");

  onError:
    Py_XDECREF(restuple);
    return -1;
}
#endif   /* MS_WINDOWS */


static int
unicode_decode_call_errorhandler_writer(
    const char *errors, PyObject **errorHandler,
    const char *encoding, const char *reason,
    const char **input, const char **inend, Py_ssize_t *startinpos,
    Py_ssize_t *endinpos, PyObject **exceptionObject, const char **inptr,
    _PyUnicodeWriter *writer /* PyObject **output, Py_ssize_t *outpos */)
{
    static const char *argparse = "Un;decoding error handler must return (str, int) tuple";

    PyObject *restuple = NULL;
    PyObject *repunicode = NULL;
    Py_ssize_t insize;
    Py_ssize_t newpos;
    Py_ssize_t replen;
    Py_ssize_t remain;
    PyObject *inputobj = NULL;
    int need_to_grow = 0;
    const char *new_inptr;

    if (*errorHandler == NULL) {
        *errorHandler = PyCodec_LookupError(errors);
        if (*errorHandler == NULL)
            goto onError;
    }

    make_decode_exception(exceptionObject,
        encoding,
        *input, *inend - *input,
        *startinpos, *endinpos,
        reason);
    if (*exceptionObject == NULL)
        goto onError;

    restuple = PyObject_CallOneArg(*errorHandler, *exceptionObject);
    if (restuple == NULL)
        goto onError;
    if (!PyTuple_Check(restuple)) {
        PyErr_SetString(PyExc_TypeError, &argparse[3]);
        goto onError;
    }
    if (!PyArg_ParseTuple(restuple, argparse, &repunicode, &newpos))
        goto onError;

    /* Copy back the bytes variables, which might have been modified by the
       callback */
    inputobj = PyUnicodeDecodeError_GetObject(*exceptionObject);
    if (!inputobj)
        goto onError;
    remain = *inend - *input - *endinpos;
    *input = PyBytes_AS_STRING(inputobj);
    insize = PyBytes_GET_SIZE(inputobj);
    *inend = *input + insize;
    /* we can DECREF safely, as the exception has another reference,
       so the object won't go away. */
    Py_DECREF(inputobj);

    if (newpos<0)
        newpos = insize+newpos;
    if (newpos<0 || newpos>insize) {
        PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", newpos);
        goto onError;
    }

    replen = PyUnicode_GET_LENGTH(repunicode);
    if (replen > 1) {
        writer->min_length += replen - 1;
        need_to_grow = 1;
    }
    new_inptr = *input + newpos;
    if (*inend - new_inptr > remain) {
        /* We don't know the decoding algorithm here so we make the worst
           assumption that one byte decodes to one unicode character.
           If unfortunately one byte could decode to more unicode characters,
           the decoder may write out-of-bound then.  Is it possible for the
           algorithms using this function? */
        writer->min_length += *inend - new_inptr - remain;
        need_to_grow = 1;
    }
    if (need_to_grow) {
        writer->overallocate = 1;
        if (_PyUnicodeWriter_Prepare(writer, writer->min_length - writer->pos,
                            PyUnicode_MAX_CHAR_VALUE(repunicode)) == -1)
            goto onError;
    }
    if (_PyUnicodeWriter_WriteStr(writer, repunicode) == -1)
        goto onError;

    *endinpos = newpos;
    *inptr = new_inptr;

    /* we made it! */
    Py_DECREF(restuple);
    return 0;

  onError:
    Py_XDECREF(restuple);
    return -1;
}


/* --- UTF-7 Codec -------------------------------------------------------- */

/* See RFC2152 for details.  We encode conservatively and decode liberally. */

/* Three simple macros defining base-64. */

/* Is c a base-64 character? */

#define IS_BASE64(c) \
    (((c) >= 'A' && (c) <= 'Z') ||     \
     ((c) >= 'a' && (c) <= 'z') ||     \
     ((c) >= '0' && (c) <= '9') ||     \
     (c) == '+' || (c) == '/')

/* given that c is a base-64 character, what is its base-64 value? */

#define FROM_BASE64(c)                                                  \
    (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' :                           \
     ((c) >= 'a' && (c) <= 'z') ? (c) - 'a' + 26 :                      \
     ((c) >= '0' && (c) <= '9') ? (c) - '0' + 52 :                      \
     (c) == '+' ? 62 : 63)

/* What is the base-64 character of the bottom 6 bits of n? */

#define TO_BASE64(n)  \
    ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(n) & 0x3f])

/* DECODE_DIRECT: this byte encountered in a UTF-7 string should be
 * decoded as itself.  We are permissive on decoding; the only ASCII
 * byte not decoding to itself is the + which begins a base64
 * string. */

#define DECODE_DIRECT(c)                                \
    ((c) <= 127 && (c) != '+')

/* The UTF-7 encoder treats ASCII characters differently according to
 * whether they are Set D, Set O, Whitespace, or special (i.e. none of
 * the above).  See RFC2152.  This array identifies these different
 * sets:
 * 0 : "Set D"
 *     alphanumeric and '(),-./:?
 * 1 : "Set O"
 *     !"#$%&*;<=>@[]^_`{|}
 * 2 : "whitespace"
 *     ht nl cr sp
 * 3 : special (must be base64 encoded)
 *     everything else (i.e. +\~ and non-printing codes 0-8 11-12 14-31 127)
 */

static
char utf7_category[128] = {
/* nul soh stx etx eot enq ack bel bs  ht  nl  vt  np  cr  so  si  */
    3,  3,  3,  3,  3,  3,  3,  3,  3,  2,  2,  3,  3,  2,  3,  3,
/* dle dc1 dc2 dc3 dc4 nak syn etb can em  sub esc fs  gs  rs  us  */
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
/* sp   !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /  */
    2,  1,  1,  1,  1,  1,  1,  0,  0,  0,  1,  3,  0,  0,  0,  0,
/*  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?  */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,
/*  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O  */
    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _  */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  3,  1,  1,  1,
/*  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o  */
    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~  del */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  3,  3,
};

/* ENCODE_DIRECT: this character should be encoded as itself.  The
 * answer depends on whether we are encoding set O as itself, and also
 * on whether we are encoding whitespace as itself.  RFC 2152 makes it
 * clear that the answers to these questions vary between
 * applications, so this code needs to be flexible.  */

#define ENCODE_DIRECT(c) \
    ((c) < 128 && (c) > 0 && ((utf7_category[(c)] != 3)))

PyObject *
PyUnicode_DecodeUTF7(const char *s,
                     Py_ssize_t size,
                     const char *errors)
{
    return PyUnicode_DecodeUTF7Stateful(s, size, errors, NULL);
}

/* The decoder.  The only state we preserve is our read position,
 * i.e. how many characters we have consumed.  So if we end in the
 * middle of a shift sequence we have to back off the read position
 * and the output to the beginning of the sequence, otherwise we lose
 * all the shift state (seen bits, number of bits seen, high
 * surrogate). */

PyObject *
PyUnicode_DecodeUTF7Stateful(const char *s,
                             Py_ssize_t size,
                             const char *errors,
                             Py_ssize_t *consumed)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    const char *e;
    _PyUnicodeWriter writer;
    const char *errmsg = "";
    int inShift = 0;
    Py_ssize_t shiftOutStart;
    unsigned int base64bits = 0;
    unsigned long base64buffer = 0;
    Py_UCS4 surrogate = 0;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    if (size == 0) {
        if (consumed)
            *consumed = 0;
        _Py_RETURN_UNICODE_EMPTY();
    }

    /* Start off assuming it's all ASCII. Widen later as necessary. */
    _PyUnicodeWriter_Init(&writer);
    writer.min_length = size;

    shiftOutStart = 0;
    e = s + size;

    while (s < e) {
        Py_UCS4 ch;
      restart:
        ch = (unsigned char) *s;

        if (inShift) { /* in a base-64 section */
            if (IS_BASE64(ch)) { /* consume a base-64 character */
                base64buffer = (base64buffer << 6) | FROM_BASE64(ch);
                base64bits += 6;
                s++;
                if (base64bits >= 16) {
                    /* we have enough bits for a UTF-16 value */
                    Py_UCS4 outCh = (Py_UCS4)(base64buffer >> (base64bits-16));
                    base64bits -= 16;
                    base64buffer &= (1 << base64bits) - 1; /* clear high bits */
                    assert(outCh <= 0xffff);
                    if (surrogate) {
                        /* expecting a second surrogate */
                        if (Py_UNICODE_IS_LOW_SURROGATE(outCh)) {
                            Py_UCS4 ch2 = Py_UNICODE_JOIN_SURROGATES(surrogate, outCh);
                            if (_PyUnicodeWriter_WriteCharInline(&writer, ch2) < 0)
                                goto onError;
                            surrogate = 0;
                            continue;
                        }
                        else {
                            if (_PyUnicodeWriter_WriteCharInline(&writer, surrogate) < 0)
                                goto onError;
                            surrogate = 0;
                        }
                    }
                    if (Py_UNICODE_IS_HIGH_SURROGATE(outCh)) {
                        /* first surrogate */
                        surrogate = outCh;
                    }
                    else {
                        if (_PyUnicodeWriter_WriteCharInline(&writer, outCh) < 0)
                            goto onError;
                    }
                }
            }
            else { /* now leaving a base-64 section */
                inShift = 0;
                if (base64bits > 0) { /* left-over bits */
                    if (base64bits >= 6) {
                        /* We've seen at least one base-64 character */
                        s++;
                        errmsg = "partial character in shift sequence";
                        goto utf7Error;
                    }
                    else {
                        /* Some bits remain; they should be zero */
                        if (base64buffer != 0) {
                            s++;
                            errmsg = "non-zero padding bits in shift sequence";
                            goto utf7Error;
                        }
                    }
                }
                if (surrogate && DECODE_DIRECT(ch)) {
                    if (_PyUnicodeWriter_WriteCharInline(&writer, surrogate) < 0)
                        goto onError;
                }
                surrogate = 0;
                if (ch == '-') {
                    /* '-' is absorbed; other terminating
                       characters are preserved */
                    s++;
                }
            }
        }
        else if ( ch == '+' ) {
            startinpos = s-starts;
            s++; /* consume '+' */
            if (s < e && *s == '-') { /* '+-' encodes '+' */
                s++;
                if (_PyUnicodeWriter_WriteCharInline(&writer, '+') < 0)
                    goto onError;
            }
            else if (s < e && !IS_BASE64(*s)) {
                s++;
                errmsg = "ill-formed sequence";
                goto utf7Error;
            }
            else { /* begin base64-encoded section */
                inShift = 1;
                surrogate = 0;
                shiftOutStart = writer.pos;
                base64bits = 0;
                base64buffer = 0;
            }
        }
        else if (DECODE_DIRECT(ch)) { /* character decodes as itself */
            s++;
            if (_PyUnicodeWriter_WriteCharInline(&writer, ch) < 0)
                goto onError;
        }
        else {
            startinpos = s-starts;
            s++;
            errmsg = "unexpected special character";
            goto utf7Error;
        }
        continue;
utf7Error:
        endinpos = s-starts;
        if (unicode_decode_call_errorhandler_writer(
                errors, &errorHandler,
                "utf7", errmsg,
                &starts, &e, &startinpos, &endinpos, &exc, &s,
                &writer))
            goto onError;
    }

    /* end of string */

    if (inShift && !consumed) { /* in shift sequence, no more to follow */
        /* if we're in an inconsistent state, that's an error */
        inShift = 0;
        if (surrogate ||
                (base64bits >= 6) ||
                (base64bits > 0 && base64buffer != 0)) {
            endinpos = size;
            if (unicode_decode_call_errorhandler_writer(
                    errors, &errorHandler,
                    "utf7", "unterminated shift sequence",
                    &starts, &e, &startinpos, &endinpos, &exc, &s,
                    &writer))
                goto onError;
            if (s < e)
                goto restart;
        }
    }

    /* return state */
    if (consumed) {
        if (inShift) {
            *consumed = startinpos;
            if (writer.pos != shiftOutStart && writer.maxchar > 127) {
                PyObject *result = PyUnicode_FromKindAndData(
                        writer.kind, writer.data, shiftOutStart);
                Py_XDECREF(errorHandler);
                Py_XDECREF(exc);
                _PyUnicodeWriter_Dealloc(&writer);
                return result;
            }
            writer.pos = shiftOutStart; /* back off output */
        }
        else {
            *consumed = s-starts;
        }
    }

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    _PyUnicodeWriter_Dealloc(&writer);
    return NULL;
}


PyObject *
_PyUnicode_EncodeUTF7(PyObject *str,
                      const char *errors)
{
    Py_ssize_t len = PyUnicode_GET_LENGTH(str);
    if (len == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    int kind = PyUnicode_KIND(str);
    const void *data = PyUnicode_DATA(str);

    /* It might be possible to tighten this worst case */
    if (len > PY_SSIZE_T_MAX / 8) {
        return PyErr_NoMemory();
    }
    PyBytesWriter *writer = PyBytesWriter_Create(len * 8);
    if (writer == NULL) {
        return NULL;
    }

    int inShift = 0;
    unsigned int base64bits = 0;
    unsigned long base64buffer = 0;
    char *out = PyBytesWriter_GetData(writer);
    for (Py_ssize_t i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        if (inShift) {
            if (ENCODE_DIRECT(ch)) {
                /* shifting out */
                if (base64bits) { /* output remaining bits */
                    *out++ = TO_BASE64(base64buffer << (6-base64bits));
                    base64buffer = 0;
                    base64bits = 0;
                }
                inShift = 0;
                /* Characters not in the BASE64 set implicitly unshift the sequence
                   so no '-' is required, except if the character is itself a '-' */
                if (IS_BASE64(ch) || ch == '-') {
                    *out++ = '-';
                }
                *out++ = (char) ch;
            }
            else {
                goto encode_char;
            }
        }
        else { /* not in a shift sequence */
            if (ch == '+') {
                *out++ = '+';
                        *out++ = '-';
            }
            else if (ENCODE_DIRECT(ch)) {
                *out++ = (char) ch;
            }
            else {
                *out++ = '+';
                inShift = 1;
                goto encode_char;
            }
        }
        continue;
encode_char:
        if (ch >= 0x10000) {
            assert(ch <= MAX_UNICODE);

            /* code first surrogate */
            base64bits += 16;
            base64buffer = (base64buffer << 16) | Py_UNICODE_HIGH_SURROGATE(ch);
            while (base64bits >= 6) {
                *out++ = TO_BASE64(base64buffer >> (base64bits-6));
                base64bits -= 6;
            }
            /* prepare second surrogate */
            ch = Py_UNICODE_LOW_SURROGATE(ch);
        }
        base64bits += 16;
        base64buffer = (base64buffer << 16) | ch;
        while (base64bits >= 6) {
            *out++ = TO_BASE64(base64buffer >> (base64bits-6));
            base64bits -= 6;
        }
    }
    if (base64bits)
        *out++= TO_BASE64(base64buffer << (6-base64bits) );
    if (inShift)
        *out++ = '-';
    return PyBytesWriter_FinishWithPointer(writer, out);
}

#undef IS_BASE64
#undef FROM_BASE64
#undef TO_BASE64
#undef DECODE_DIRECT
#undef ENCODE_DIRECT


/* --- UTF-8 Codec -------------------------------------------------------- */

PyObject *
PyUnicode_DecodeUTF8(const char *s,
                     Py_ssize_t size,
                     const char *errors)
{
    return PyUnicode_DecodeUTF8Stateful(s, size, errors, NULL);
}

#include "stringlib/asciilib.h"
#include "stringlib/codecs.h"
#include "stringlib/undef.h"

#include "stringlib/ucs1lib.h"
#include "stringlib/codecs.h"
#include "stringlib/undef.h"

#include "stringlib/ucs2lib.h"
#include "stringlib/codecs.h"
#include "stringlib/undef.h"

#include "stringlib/ucs4lib.h"
#include "stringlib/codecs.h"
#include "stringlib/undef.h"

#if (SIZEOF_SIZE_T == 8)
/* Mask to quickly check whether a C 'size_t' contains a
   non-ASCII, UTF8-encoded char. */
# define ASCII_CHAR_MASK 0x8080808080808080ULL
// used to count codepoints in UTF-8 string.
# define VECTOR_0101     0x0101010101010101ULL
# define VECTOR_00FF     0x00ff00ff00ff00ffULL
#elif (SIZEOF_SIZE_T == 4)
# define ASCII_CHAR_MASK 0x80808080U
# define VECTOR_0101     0x01010101U
# define VECTOR_00FF     0x00ff00ffU
#else
# error C 'size_t' size should be either 4 or 8!
#endif

#if (defined(__clang__) || defined(__GNUC__))
#define HAVE_CTZ 1
static inline unsigned int
ctz(size_t v)
{
    return __builtin_ctzll((unsigned long long)v);
}
#elif defined(_MSC_VER)
#define HAVE_CTZ 1
static inline unsigned int
ctz(size_t v)
{
    unsigned long pos;
#if SIZEOF_SIZE_T == 4
    _BitScanForward(&pos, v);
#else
    _BitScanForward64(&pos, v);
#endif /* SIZEOF_SIZE_T */
    return pos;
}
#else
#define HAVE_CTZ 0
#endif

#if HAVE_CTZ && PY_LITTLE_ENDIAN
// load p[0]..p[size-1] as a size_t without unaligned access nor read ahead.
static size_t
load_unaligned(const unsigned char *p, size_t size)
{
    union {
        size_t s;
        unsigned char b[SIZEOF_SIZE_T];
    } u;
    u.s = 0;
    // This switch statement assumes little endian because:
    // * union is faster than bitwise or and shift.
    // * big endian machine is rare and hard to maintain.
    switch (size) {
    default:
#if SIZEOF_SIZE_T == 8
    case 8:
        u.b[7] = p[7];
        _Py_FALLTHROUGH;
    case 7:
        u.b[6] = p[6];
        _Py_FALLTHROUGH;
    case 6:
        u.b[5] = p[5];
        _Py_FALLTHROUGH;
    case 5:
        u.b[4] = p[4];
        _Py_FALLTHROUGH;
#endif
    case 4:
        u.b[3] = p[3];
        _Py_FALLTHROUGH;
    case 3:
        u.b[2] = p[2];
        _Py_FALLTHROUGH;
    case 2:
        u.b[1] = p[1];
        _Py_FALLTHROUGH;
    case 1:
        u.b[0] = p[0];
        break;
    case 0:
        break;
    }
    return u.s;
}
#endif

/*
 * Find the first non-ASCII character in a byte sequence.
 *
 * This function scans a range of bytes from `start` to `end` and returns the
 * index of the first byte that is not an ASCII character (i.e., has the most
 * significant bit set). If all characters in the range are ASCII, it returns
 * `end - start`.
 */
static Py_ssize_t
find_first_nonascii(const unsigned char *start, const unsigned char *end)
{
    // The search is done in `size_t` chunks.
    // The start and end might not be aligned at `size_t` boundaries,
    // so they're handled specially.

    const unsigned char *p = start;

    if (end - start >= SIZEOF_SIZE_T) {
        // Avoid unaligned read.
#if PY_LITTLE_ENDIAN && HAVE_CTZ
        size_t u;
        memcpy(&u, p, sizeof(size_t));
        u &= ASCII_CHAR_MASK;
        if (u) {
            return (ctz(u) - 7) / 8;
        }
        p = _Py_ALIGN_DOWN(p + SIZEOF_SIZE_T, SIZEOF_SIZE_T);
#else /* PY_LITTLE_ENDIAN && HAVE_CTZ */
        const unsigned char *p2 = _Py_ALIGN_UP(p, SIZEOF_SIZE_T);
        while (p < p2) {
            if (*p & 0x80) {
                return p - start;
            }
            p++;
        }
#endif

        const unsigned char *e = end - SIZEOF_SIZE_T;
        while (p <= e) {
            size_t u = (*(const size_t *)p) & ASCII_CHAR_MASK;
            if (u) {
#if PY_LITTLE_ENDIAN && HAVE_CTZ
                return p - start + (ctz(u) - 7) / 8;
#else
                // big endian and minor compilers are difficult to test.
                // fallback to per byte check.
                break;
#endif
            }
            p += SIZEOF_SIZE_T;
        }
    }
#if PY_LITTLE_ENDIAN && HAVE_CTZ
    assert((end - p) < SIZEOF_SIZE_T);
    // we can not use *(const size_t*)p to avoid buffer overrun.
    size_t u = load_unaligned(p, end - p) & ASCII_CHAR_MASK;
    if (u) {
        return p - start + (ctz(u) - 7) / 8;
    }
    return end - start;
#else
    while (p < end) {
        if (*p & 0x80) {
            break;
        }
        p++;
    }
    return p - start;
#endif
}

static inline int
scalar_utf8_start_char(unsigned int ch)
{
    // 0xxxxxxx or 11xxxxxx are first byte.
    return (~ch >> 7 | ch >> 6) & 1;
}

static inline size_t
vector_utf8_start_chars(size_t v)
{
    return ((~v >> 7) | (v >> 6)) & VECTOR_0101;
}


// Count the number of UTF-8 code points in a given byte sequence.
static Py_ssize_t
utf8_count_codepoints(const unsigned char *s, const unsigned char *end)
{
    Py_ssize_t len = 0;

    if (end - s >= SIZEOF_SIZE_T) {
        while (!_Py_IS_ALIGNED(s, ALIGNOF_SIZE_T)) {
            len += scalar_utf8_start_char(*s++);
        }

        while (s + SIZEOF_SIZE_T <= end) {
            const unsigned char *e = end;
            if (e - s > SIZEOF_SIZE_T * 255) {
                e = s + SIZEOF_SIZE_T * 255;
            }
            Py_ssize_t vstart = 0;
            while (s + SIZEOF_SIZE_T <= e) {
                size_t v = *(size_t*)s;
                size_t vs = vector_utf8_start_chars(v);
                vstart += vs;
                s += SIZEOF_SIZE_T;
            }
            vstart = (vstart & VECTOR_00FF) + ((vstart >> 8) & VECTOR_00FF);
            vstart += vstart >> 16;
#if SIZEOF_SIZE_T == 8
            vstart += vstart >> 32;
#endif
            len += vstart & 0x7ff;
        }
    }
    while (s < end) {
        len += scalar_utf8_start_char(*s++);
    }
    return len;
}

static Py_ssize_t
ascii_decode(const char *start, const char *end, Py_UCS1 *dest)
{
#if SIZEOF_SIZE_T <= SIZEOF_VOID_P
    if (_Py_IS_ALIGNED(start, ALIGNOF_SIZE_T)
        && _Py_IS_ALIGNED(dest, ALIGNOF_SIZE_T))
    {
        /* Fast path, see in STRINGLIB(utf8_decode) for
           an explanation. */
        const char *p = start;
        Py_UCS1 *q = dest;
        while (p + SIZEOF_SIZE_T <= end) {
            size_t value = *(const size_t *) p;
            if (value & ASCII_CHAR_MASK)
                break;
            *((size_t *)q) = value;
            p += SIZEOF_SIZE_T;
            q += SIZEOF_SIZE_T;
        }
        while (p < end) {
            if ((unsigned char)*p & 0x80)
                break;
            *q++ = *p++;
        }
        return p - start;
    }
#endif
    Py_ssize_t pos = find_first_nonascii((const unsigned char*)start,
                                         (const unsigned char*)end);
    memcpy(dest, start, pos);
    return pos;
}

static int
unicode_decode_utf8_impl(_PyUnicodeWriter *writer,
                         const char *starts, const char *s, const char *end,
                         _Py_error_handler error_handler,
                         const char *errors,
                         Py_ssize_t *consumed)
{
    Py_ssize_t startinpos, endinpos;
    const char *errmsg = "";
    PyObject *error_handler_obj = NULL;
    PyObject *exc = NULL;

    while (s < end) {
        Py_UCS4 ch;
        int kind = writer->kind;

        if (kind == PyUnicode_1BYTE_KIND) {
            if (PyUnicode_IS_ASCII(writer->buffer))
                ch = asciilib_utf8_decode(&s, end, writer->data, &writer->pos);
            else
                ch = ucs1lib_utf8_decode(&s, end, writer->data, &writer->pos);
        } else if (kind == PyUnicode_2BYTE_KIND) {
            ch = ucs2lib_utf8_decode(&s, end, writer->data, &writer->pos);
        } else {
            assert(kind == PyUnicode_4BYTE_KIND);
            ch = ucs4lib_utf8_decode(&s, end, writer->data, &writer->pos);
        }

        switch (ch) {
        case 0:
            if (s == end || consumed)
                goto End;
            errmsg = "unexpected end of data";
            startinpos = s - starts;
            endinpos = end - starts;
            break;
        case 1:
            errmsg = "invalid start byte";
            startinpos = s - starts;
            endinpos = startinpos + 1;
            break;
        case 2:
            if (consumed && (unsigned char)s[0] == 0xED && end - s == 2
                && (unsigned char)s[1] >= 0xA0 && (unsigned char)s[1] <= 0xBF)
            {
                /* Truncated surrogate code in range D800-DFFF */
                goto End;
            }
            _Py_FALLTHROUGH;
        case 3:
        case 4:
            errmsg = "invalid continuation byte";
            startinpos = s - starts;
            endinpos = startinpos + ch - 1;
            break;
        default:
            // ch doesn't fit into kind, so change the buffer kind to write
            // the character
            if (_PyUnicodeWriter_WriteCharInline(writer, ch) < 0)
                goto onError;
            continue;
        }

        if (error_handler == _Py_ERROR_UNKNOWN)
            error_handler = _Py_GetErrorHandler(errors);

        switch (error_handler) {
        case _Py_ERROR_IGNORE:
            s += (endinpos - startinpos);
            break;

        case _Py_ERROR_REPLACE:
            if (_PyUnicodeWriter_WriteCharInline(writer, 0xfffd) < 0)
                goto onError;
            s += (endinpos - startinpos);
            break;

        case _Py_ERROR_SURROGATEESCAPE:
        {
            Py_ssize_t i;

            if (_PyUnicodeWriter_PrepareKind(writer, PyUnicode_2BYTE_KIND) < 0)
                goto onError;
            for (i=startinpos; i<endinpos; i++) {
                ch = (Py_UCS4)(unsigned char)(starts[i]);
                PyUnicode_WRITE(writer->kind, writer->data, writer->pos,
                                ch + 0xdc00);
                writer->pos++;
            }
            s += (endinpos - startinpos);
            break;
        }

        default:
            if (unicode_decode_call_errorhandler_writer(
                    errors, &error_handler_obj,
                    "utf-8", errmsg,
                    &starts, &end, &startinpos, &endinpos, &exc, &s,
                    writer)) {
                goto onError;
            }

            if (_PyUnicodeWriter_Prepare(writer, end - s, 127) < 0) {
                return -1;
            }
        }
    }

End:
    if (consumed)
        *consumed = s - starts;

    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return 0;

onError:
    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return -1;
}


static PyObject *
unicode_decode_utf8(const char *s, Py_ssize_t size,
                    _Py_error_handler error_handler, const char *errors,
                    Py_ssize_t *consumed)
{
    if (size == 0) {
        if (consumed) {
            *consumed = 0;
        }
        _Py_RETURN_UNICODE_EMPTY();
    }

    /* ASCII is equivalent to the first 128 ordinals in Unicode. */
    if (size == 1 && (unsigned char)s[0] < 128) {
        if (consumed) {
            *consumed = 1;
        }
        return get_latin1_char((unsigned char)s[0]);
    }

    // I don't know this check is necessary or not. But there is a test
    // case that requires size=PY_SSIZE_T_MAX cause MemoryError.
    if (PY_SSIZE_T_MAX - sizeof(PyCompactUnicodeObject) < (size_t)size) {
        PyErr_NoMemory();
        return NULL;
    }

    const char *starts = s;
    const char *end = s + size;

    Py_ssize_t pos = find_first_nonascii((const unsigned char*)starts, (const unsigned char*)end);
    if (pos == size) {  // fast path: ASCII string.
        PyObject *u = PyUnicode_New(size, 127);
        if (u == NULL) {
            return NULL;
        }
        memcpy(PyUnicode_1BYTE_DATA(u), s, size);
        if (consumed) {
            *consumed = size;
        }
        return u;
    }

    int maxchr = 127;
    Py_ssize_t maxsize = size;

    unsigned char ch = (unsigned char)(s[pos]);
    // error handler other than strict may remove/replace the invalid byte.
    // consumed != NULL allows 1~3 bytes remainings.
    // 0x80 <= ch < 0xc2 is invalid start byte that cause UnicodeDecodeError.
    // otherwise: check the input and decide the maxchr and maxsize to reduce
    // reallocation and copy.
    if (error_handler == _Py_ERROR_STRICT && !consumed && ch >= 0xc2) {
        // we only calculate the number of codepoints and don't determine the exact maxchr.
        // This is because writing fast and portable SIMD code to find maxchr is difficult.
        // If reallocation occurs for a larger maxchar, knowing the exact number of codepoints
        // means that it is no longer necessary to allocate several times the required amount
        // of memory.
        maxsize = utf8_count_codepoints((const unsigned char *)s, (const unsigned char *)end);
        if (ch < 0xc4) { // latin1
            maxchr = 0xff;
        }
        else if (ch < 0xf0) { // ucs2
            maxchr = 0xffff;
        }
        else { // ucs4
            maxchr = 0x10ffff;
        }
    }
    PyObject *u = PyUnicode_New(maxsize, maxchr);
    if (!u) {
        return NULL;
    }

    // Use _PyUnicodeWriter after fast path is failed.
    _PyUnicodeWriter writer;
    _PyUnicodeWriter_InitWithBuffer(&writer, u);
    if (maxchr <= 255) {
        memcpy(PyUnicode_1BYTE_DATA(u), s, pos);
        s += pos;
        writer.pos = pos;
    }

    if (unicode_decode_utf8_impl(&writer, starts, s, end,
                                 error_handler, errors,
                                 consumed) < 0) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyUnicodeWriter_Finish(&writer);
}


// Used by PyUnicodeWriter_WriteUTF8() implementation
int
_PyUnicode_DecodeUTF8Writer(_PyUnicodeWriter *writer,
                            const char *s, Py_ssize_t size,
                            _Py_error_handler error_handler, const char *errors,
                            Py_ssize_t *consumed)
{
    if (size == 0) {
        if (consumed) {
            *consumed = 0;
        }
        return 0;
    }

    // fast path: try ASCII string.
    if (_PyUnicodeWriter_Prepare(writer, size, 127) < 0) {
        return -1;
    }

    const char *starts = s;
    const char *end = s + size;
    Py_ssize_t decoded = 0;
    Py_UCS1 *dest = (Py_UCS1*)writer->data + writer->pos * writer->kind;
    if (writer->kind == PyUnicode_1BYTE_KIND) {
        decoded = ascii_decode(s, end, dest);
        writer->pos += decoded;

        if (decoded == size) {
            if (consumed) {
                *consumed = size;
            }
            return 0;
        }
        s += decoded;
    }

    return unicode_decode_utf8_impl(writer, starts, s, end,
                                    error_handler, errors, consumed);
}


PyObject *
PyUnicode_DecodeUTF8Stateful(const char *s,
                             Py_ssize_t size,
                             const char *errors,
                             Py_ssize_t *consumed)
{
    return unicode_decode_utf8(s, size,
                               errors ? _Py_ERROR_UNKNOWN : _Py_ERROR_STRICT,
                               errors, consumed);
}


/* UTF-8 decoder: use surrogateescape error handler if 'surrogateescape' is
   non-zero, use strict error handler otherwise.

   On success, write a pointer to a newly allocated wide character string into
   *wstr (use PyMem_RawFree() to free the memory) and write the output length
   (in number of wchar_t units) into *wlen (if wlen is set).

   On memory allocation failure, return -1.

   On decoding error (if surrogateescape is zero), return -2. If wlen is
   non-NULL, write the start of the illegal byte sequence into *wlen. If reason
   is not NULL, write the decoding error message into *reason. */
int
_Py_DecodeUTF8Ex(const char *s, Py_ssize_t size, wchar_t **wstr, size_t *wlen,
                 const char **reason, _Py_error_handler errors)
{
    const char *orig_s = s;
    const char *e;
    wchar_t *unicode;
    Py_ssize_t outpos;

    int surrogateescape = 0;
    int surrogatepass = 0;
    switch (errors)
    {
    case _Py_ERROR_STRICT:
        break;
    case _Py_ERROR_SURROGATEESCAPE:
        surrogateescape = 1;
        break;
    case _Py_ERROR_SURROGATEPASS:
        surrogatepass = 1;
        break;
    default:
        return -3;
    }

    /* Note: size will always be longer than the resulting Unicode
       character count */
    if (PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(wchar_t) - 1 < size) {
        return -1;
    }

    unicode = PyMem_RawMalloc((size + 1) * sizeof(wchar_t));
    if (!unicode) {
        return -1;
    }

    /* Unpack UTF-8 encoded data */
    e = s + size;
    outpos = 0;
    while (s < e) {
        Py_UCS4 ch;
#if SIZEOF_WCHAR_T == 4
        ch = ucs4lib_utf8_decode(&s, e, (Py_UCS4 *)unicode, &outpos);
#else
        ch = ucs2lib_utf8_decode(&s, e, (Py_UCS2 *)unicode, &outpos);
#endif
        if (ch > 0xFF) {
#if SIZEOF_WCHAR_T == 4
            Py_UNREACHABLE();
#else
            assert(ch > 0xFFFF && ch <= MAX_UNICODE);
            /* write a surrogate pair */
            unicode[outpos++] = (wchar_t)Py_UNICODE_HIGH_SURROGATE(ch);
            unicode[outpos++] = (wchar_t)Py_UNICODE_LOW_SURROGATE(ch);
#endif
        }
        else {
            if (!ch && s == e) {
                break;
            }

            if (surrogateescape) {
                unicode[outpos++] = 0xDC00 + (unsigned char)*s++;
            }
            else {
                /* Is it a valid three-byte code? */
                if (surrogatepass
                    && (e - s) >= 3
                    && (s[0] & 0xf0) == 0xe0
                    && (s[1] & 0xc0) == 0x80
                    && (s[2] & 0xc0) == 0x80)
                {
                    ch = ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + (s[2] & 0x3f);
                    s += 3;
                    unicode[outpos++] = ch;
                }
                else {
                    PyMem_RawFree(unicode );
                    if (reason != NULL) {
                        switch (ch) {
                        case 0:
                            *reason = "unexpected end of data";
                            break;
                        case 1:
                            *reason = "invalid start byte";
                            break;
                        /* 2, 3, 4 */
                        default:
                            *reason = "invalid continuation byte";
                            break;
                        }
                    }
                    if (wlen != NULL) {
                        *wlen = s - orig_s;
                    }
                    return -2;
                }
            }
        }
    }
    unicode[outpos] = L'\0';
    if (wlen) {
        *wlen = outpos;
    }
    *wstr = unicode;
    return 0;
}


wchar_t*
_Py_DecodeUTF8_surrogateescape(const char *arg, Py_ssize_t arglen,
                               size_t *wlen)
{
    wchar_t *wstr;
    int res = _Py_DecodeUTF8Ex(arg, arglen,
                               &wstr, wlen,
                               NULL, _Py_ERROR_SURROGATEESCAPE);
    if (res != 0) {
        /* _Py_DecodeUTF8Ex() must support _Py_ERROR_SURROGATEESCAPE */
        assert(res != -3);
        if (wlen) {
            *wlen = (size_t)res;
        }
        return NULL;
    }
    return wstr;
}


/* UTF-8 encoder.

   On success, return 0 and write the newly allocated character string (use
   PyMem_Free() to free the memory) into *str.

   On encoding failure, return -2 and write the position of the invalid
   surrogate character into *error_pos (if error_pos is set) and the decoding
   error message into *reason (if reason is set).

   On memory allocation failure, return -1. */
int
_Py_EncodeUTF8Ex(const wchar_t *text, char **str, size_t *error_pos,
                 const char **reason, int raw_malloc, _Py_error_handler errors)
{
    const Py_ssize_t max_char_size = 4;
    Py_ssize_t len = wcslen(text);

    assert(len >= 0);

    int surrogateescape = 0;
    int surrogatepass = 0;
    switch (errors)
    {
    case _Py_ERROR_STRICT:
        break;
    case _Py_ERROR_SURROGATEESCAPE:
        surrogateescape = 1;
        break;
    case _Py_ERROR_SURROGATEPASS:
        surrogatepass = 1;
        break;
    default:
        return -3;
    }

    if (len > PY_SSIZE_T_MAX / max_char_size - 1) {
        return -1;
    }
    char *bytes;
    if (raw_malloc) {
        bytes = PyMem_RawMalloc((len + 1) * max_char_size);
    }
    else {
        bytes = PyMem_Malloc((len + 1) * max_char_size);
    }
    if (bytes == NULL) {
        return -1;
    }

    char *p = bytes;
    Py_ssize_t i;
    for (i = 0; i < len; ) {
        Py_ssize_t ch_pos = i;
        Py_UCS4 ch = text[i];
        i++;
#if Py_UNICODE_SIZE == 2
        if (Py_UNICODE_IS_HIGH_SURROGATE(ch)
            && i < len
            && Py_UNICODE_IS_LOW_SURROGATE(text[i]))
        {
            ch = Py_UNICODE_JOIN_SURROGATES(ch, text[i]);
            i++;
        }
#endif

        if (ch < 0x80) {
            /* Encode ASCII */
            *p++ = (char) ch;

        }
        else if (ch < 0x0800) {
            /* Encode Latin-1 */
            *p++ = (char)(0xc0 | (ch >> 6));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
        else if (Py_UNICODE_IS_SURROGATE(ch) && !surrogatepass) {
            /* surrogateescape error handler */
            if (!surrogateescape || !(0xDC80 <= ch && ch <= 0xDCFF)) {
                if (error_pos != NULL) {
                    *error_pos = (size_t)ch_pos;
                }
                if (reason != NULL) {
                    *reason = "encoding error";
                }
                if (raw_malloc) {
                    PyMem_RawFree(bytes);
                }
                else {
                    PyMem_Free(bytes);
                }
                return -2;
            }
            *p++ = (char)(ch & 0xff);
        }
        else if (ch < 0x10000) {
            *p++ = (char)(0xe0 | (ch >> 12));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
        else {  /* ch >= 0x10000 */
            assert(ch <= MAX_UNICODE);
            /* Encode UCS4 Unicode ordinals */
            *p++ = (char)(0xf0 | (ch >> 18));
            *p++ = (char)(0x80 | ((ch >> 12) & 0x3f));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
    }
    *p++ = '\0';

    size_t final_size = (p - bytes);
    char *bytes2;
    if (raw_malloc) {
        bytes2 = PyMem_RawRealloc(bytes, final_size);
    }
    else {
        bytes2 = PyMem_Realloc(bytes, final_size);
    }
    if (bytes2 == NULL) {
        if (error_pos != NULL) {
            *error_pos = (size_t)-1;
        }
        if (raw_malloc) {
            PyMem_RawFree(bytes);
        }
        else {
            PyMem_Free(bytes);
        }
        return -1;
    }
    *str = bytes2;
    return 0;
}


/* Primary internal function which creates utf8 encoded bytes objects.

   Allocation strategy:  if the string is short, convert into a stack buffer
   and allocate exactly as much space needed at the end.  Else allocate the
   maximum possible needed (4 result bytes per Unicode character), and return
   the excess memory at the end.
*/
static PyObject *
unicode_encode_utf8(PyObject *unicode, _Py_error_handler error_handler,
                    const char *errors)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    if (PyUnicode_UTF8(unicode))
        return PyBytes_FromStringAndSize(PyUnicode_UTF8(unicode),
                                         PyUnicode_UTF8_LENGTH(unicode));

    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    Py_ssize_t size = PyUnicode_GET_LENGTH(unicode);

    PyBytesWriter *writer;
    char *end;

    switch (kind) {
    default:
        Py_UNREACHABLE();
    case PyUnicode_1BYTE_KIND:
        /* the string cannot be ASCII, or PyUnicode_UTF8() would be set */
        assert(!PyUnicode_IS_ASCII(unicode));
        writer = ucs1lib_utf8_encoder(unicode, data, size,
                                      error_handler, errors, &end);
        break;
    case PyUnicode_2BYTE_KIND:
        writer = ucs2lib_utf8_encoder(unicode, data, size,
                                      error_handler, errors, &end);
        break;
    case PyUnicode_4BYTE_KIND:
        writer = ucs4lib_utf8_encoder(unicode, data, size,
                                      error_handler, errors, &end);
        break;
    }

    if (writer == NULL) {
        PyBytesWriter_Discard(writer);
        return NULL;
    }
    return PyBytesWriter_FinishWithPointer(writer, end);
}

static int
unicode_fill_utf8(PyObject *unicode)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(unicode);
    /* the string cannot be ASCII, or PyUnicode_UTF8() would be set */
    assert(!PyUnicode_IS_ASCII(unicode));

    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    Py_ssize_t size = PyUnicode_GET_LENGTH(unicode);

    PyBytesWriter *writer;
    char *end;

    switch (kind) {
    default:
        Py_UNREACHABLE();
    case PyUnicode_1BYTE_KIND:
        writer = ucs1lib_utf8_encoder(unicode, data, size,
                                      _Py_ERROR_STRICT, NULL, &end);
        break;
    case PyUnicode_2BYTE_KIND:
        writer = ucs2lib_utf8_encoder(unicode, data, size,
                                      _Py_ERROR_STRICT, NULL, &end);
        break;
    case PyUnicode_4BYTE_KIND:
        writer = ucs4lib_utf8_encoder(unicode, data, size,
                                      _Py_ERROR_STRICT, NULL, &end);
        break;
    }
    if (writer == NULL) {
        return -1;
    }

    const char *start = PyBytesWriter_GetData(writer);
    Py_ssize_t len = end - start;

    char *cache = PyMem_Malloc(len + 1);
    if (cache == NULL) {
        PyBytesWriter_Discard(writer);
        PyErr_NoMemory();
        return -1;
    }
    memcpy(cache, start, len);
    cache[len] = '\0';
    PyUnicode_SET_UTF8_LENGTH(unicode, len);
    PyUnicode_SET_UTF8(unicode, cache);
    PyBytesWriter_Discard(writer);
    return 0;
}

PyObject *
_PyUnicode_AsUTF8String(PyObject *unicode, const char *errors)
{
    return unicode_encode_utf8(unicode, _Py_ERROR_UNKNOWN, errors);
}


PyObject *
PyUnicode_AsUTF8String(PyObject *unicode)
{
    return _PyUnicode_AsUTF8String(unicode, NULL);
}


static int
unicode_ensure_utf8(PyObject *unicode)
{
    int err = 0;
    if (PyUnicode_UTF8(unicode) == NULL) {
        Py_BEGIN_CRITICAL_SECTION(unicode);
        if (PyUnicode_UTF8(unicode) == NULL) {
            err = unicode_fill_utf8(unicode);
        }
        Py_END_CRITICAL_SECTION();
    }
    return err;
}

const char *
PyUnicode_AsUTF8AndSize(PyObject *unicode, Py_ssize_t *psize)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        if (psize) {
            *psize = -1;
        }
        return NULL;
    }

    if (unicode_ensure_utf8(unicode) == -1) {
        if (psize) {
            *psize = -1;
        }
        return NULL;
    }

    if (psize) {
        *psize = PyUnicode_UTF8_LENGTH(unicode);
    }
    return PyUnicode_UTF8(unicode);
}

const char *
PyUnicode_AsUTF8(PyObject *unicode)
{
    return PyUnicode_AsUTF8AndSize(unicode, NULL);
}

const char *
_PyUnicode_AsUTF8NoNUL(PyObject *unicode)
{
    Py_ssize_t size;
    const char *s = PyUnicode_AsUTF8AndSize(unicode, &size);
    if (s && strlen(s) != (size_t)size) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        return NULL;
    }
    return s;
}


/* --- UTF-32 Codec ------------------------------------------------------- */

PyObject *
PyUnicode_DecodeUTF32(const char *s,
                      Py_ssize_t size,
                      const char *errors,
                      int *byteorder)
{
    return PyUnicode_DecodeUTF32Stateful(s, size, errors, byteorder, NULL);
}


PyObject *
PyUnicode_DecodeUTF32Stateful(const char *s,
                              Py_ssize_t size,
                              const char *errors,
                              int *byteorder,
                              Py_ssize_t *consumed)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    _PyUnicodeWriter writer;
    const unsigned char *q, *e;
    int le, bo = 0;       /* assume native ordering by default */
    const char *encoding;
    const char *errmsg = "";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    q = (const unsigned char *)s;
    e = q + size;

    if (byteorder)
        bo = *byteorder;

    /* Check for BOM marks (U+FEFF) in the input and adjust current
       byte order setting accordingly. In native mode, the leading BOM
       mark is skipped, in all other modes, it is copied to the output
       stream as-is (giving a ZWNBSP character). */
    if (bo == 0 && size >= 4) {
        Py_UCS4 bom = ((unsigned int)q[3] << 24) | (q[2] << 16) | (q[1] << 8) | q[0];
        if (bom == 0x0000FEFF) {
            bo = -1;
            q += 4;
        }
        else if (bom == 0xFFFE0000) {
            bo = 1;
            q += 4;
        }
        if (byteorder)
            *byteorder = bo;
    }

    if (q == e) {
        if (consumed)
            *consumed = size;
        _Py_RETURN_UNICODE_EMPTY();
    }

#ifdef WORDS_BIGENDIAN
    le = bo < 0;
#else
    le = bo <= 0;
#endif
    encoding = le ? "utf-32-le" : "utf-32-be";

    _PyUnicodeWriter_Init(&writer);
    writer.min_length = (e - q + 3) / 4;
    if (_PyUnicodeWriter_Prepare(&writer, writer.min_length, 127) == -1)
        goto onError;

    while (1) {
        Py_UCS4 ch = 0;
        Py_UCS4 maxch = PyUnicode_MAX_CHAR_VALUE(writer.buffer);

        if (e - q >= 4) {
            int kind = writer.kind;
            void *data = writer.data;
            const unsigned char *last = e - 4;
            Py_ssize_t pos = writer.pos;
            if (le) {
                do {
                    ch = ((unsigned int)q[3] << 24) | (q[2] << 16) | (q[1] << 8) | q[0];
                    if (ch > maxch)
                        break;
                    if (kind != PyUnicode_1BYTE_KIND &&
                        Py_UNICODE_IS_SURROGATE(ch))
                        break;
                    PyUnicode_WRITE(kind, data, pos++, ch);
                    q += 4;
                } while (q <= last);
            }
            else {
                do {
                    ch = ((unsigned int)q[0] << 24) | (q[1] << 16) | (q[2] << 8) | q[3];
                    if (ch > maxch)
                        break;
                    if (kind != PyUnicode_1BYTE_KIND &&
                        Py_UNICODE_IS_SURROGATE(ch))
                        break;
                    PyUnicode_WRITE(kind, data, pos++, ch);
                    q += 4;
                } while (q <= last);
            }
            writer.pos = pos;
        }

        if (Py_UNICODE_IS_SURROGATE(ch)) {
            errmsg = "code point in surrogate code point range(0xd800, 0xe000)";
            startinpos = ((const char *)q) - starts;
            endinpos = startinpos + 4;
        }
        else if (ch <= maxch) {
            if (q == e || consumed)
                break;
            /* remaining bytes at the end? (size should be divisible by 4) */
            errmsg = "truncated data";
            startinpos = ((const char *)q) - starts;
            endinpos = ((const char *)e) - starts;
        }
        else {
            if (ch < 0x110000) {
                if (_PyUnicodeWriter_WriteCharInline(&writer, ch) < 0)
                    goto onError;
                q += 4;
                continue;
            }
            errmsg = "code point not in range(0x110000)";
            startinpos = ((const char *)q) - starts;
            endinpos = startinpos + 4;
        }

        /* The remaining input chars are ignored if the callback
           chooses to skip the input */
        if (unicode_decode_call_errorhandler_writer(
                errors, &errorHandler,
                encoding, errmsg,
                &starts, (const char **)&e, &startinpos, &endinpos, &exc, (const char **)&q,
                &writer))
            goto onError;
    }

    if (consumed)
        *consumed = (const char *)q-starts;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}


PyObject *
_PyUnicode_EncodeUTF32(PyObject *str,
                       const char *errors,
                       int byteorder)
{
    if (!PyUnicode_Check(str)) {
        PyErr_BadArgument();
        return NULL;
    }
    int kind = PyUnicode_KIND(str);
    const void *data = PyUnicode_DATA(str);
    Py_ssize_t len = PyUnicode_GET_LENGTH(str);

    if (len > PY_SSIZE_T_MAX / 4 - (byteorder == 0))
        return PyErr_NoMemory();
    Py_ssize_t nsize = len + (byteorder == 0);

#if PY_LITTLE_ENDIAN
    int native_ordering = byteorder <= 0;
#else
    int native_ordering = byteorder >= 0;
#endif

    if (kind == PyUnicode_1BYTE_KIND) {
        // gh-139156: Don't use PyBytesWriter API here since it has an overhead
        // on short strings
        PyObject *v = PyBytes_FromStringAndSize(NULL, nsize * 4);
        if (v == NULL) {
            return NULL;
        }

        /* output buffer is 4-bytes aligned */
        assert(_Py_IS_ALIGNED(PyBytes_AS_STRING(v), 4));
        uint32_t *out = (uint32_t *)PyBytes_AS_STRING(v);
        if (byteorder == 0) {
            *out++ = 0xFEFF;
        }
        if (len > 0) {
            ucs1lib_utf32_encode((const Py_UCS1 *)data, len,
                                 &out, native_ordering);
        }
        return v;
    }

    PyBytesWriter *writer = PyBytesWriter_Create(nsize * 4);
    if (writer == NULL) {
        return NULL;
    }

    /* output buffer is 4-bytes aligned */
    assert(_Py_IS_ALIGNED(PyBytesWriter_GetData(writer), 4));
    uint32_t *out = (uint32_t *)PyBytesWriter_GetData(writer);
    if (byteorder == 0) {
        *out++ = 0xFEFF;
    }
    if (len == 0) {
        return PyBytesWriter_Finish(writer);
    }

    const char *encoding;
    if (byteorder == -1)
        encoding = "utf-32-le";
    else if (byteorder == 1)
        encoding = "utf-32-be";
    else
        encoding = "utf-32";

    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    PyObject *rep = NULL;

    for (Py_ssize_t pos = 0; pos < len; ) {
        if (kind == PyUnicode_2BYTE_KIND) {
            pos += ucs2lib_utf32_encode((const Py_UCS2 *)data + pos, len - pos,
                                        &out, native_ordering);
        }
        else {
            assert(kind == PyUnicode_4BYTE_KIND);
            pos += ucs4lib_utf32_encode((const Py_UCS4 *)data + pos, len - pos,
                                        &out, native_ordering);
        }
        if (pos == len)
            break;

        Py_ssize_t newpos;
        rep = unicode_encode_call_errorhandler(
                errors, &errorHandler,
                encoding, "surrogates not allowed",
                str, &exc, pos, pos + 1, &newpos);
        if (!rep)
            goto error;

        Py_ssize_t repsize, moreunits;
        if (PyBytes_Check(rep)) {
            repsize = PyBytes_GET_SIZE(rep);
            if (repsize & 3) {
                raise_encode_exception(&exc, encoding,
                                       str, pos, pos + 1,
                                       "surrogates not allowed");
                goto error;
            }
            moreunits = repsize / 4;
        }
        else {
            assert(PyUnicode_Check(rep));
            moreunits = repsize = PyUnicode_GET_LENGTH(rep);
            if (!PyUnicode_IS_ASCII(rep)) {
                raise_encode_exception(&exc, encoding,
                                       str, pos, pos + 1,
                                       "surrogates not allowed");
                goto error;
            }
        }
        moreunits += pos - newpos;
        pos = newpos;

        /* four bytes are reserved for each surrogate */
        if (moreunits > 0) {
            out = PyBytesWriter_GrowAndUpdatePointer(writer, 4 * moreunits, out);
            if (out == NULL) {
                goto error;
            }
        }

        if (PyBytes_Check(rep)) {
            memcpy(out, PyBytes_AS_STRING(rep), repsize);
            out += repsize / 4;
        }
        else {
            /* rep is unicode */
            assert(PyUnicode_KIND(rep) == PyUnicode_1BYTE_KIND);
            ucs1lib_utf32_encode(PyUnicode_1BYTE_DATA(rep), repsize,
                                 &out, native_ordering);
        }

        Py_CLEAR(rep);
    }

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);

    /* Cut back to size actually needed. This is necessary for, for example,
       encoding of a string containing isolated surrogates and the 'ignore'
       handler is used. */
    return PyBytesWriter_FinishWithPointer(writer, out);

  error:
    Py_XDECREF(rep);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    PyBytesWriter_Discard(writer);
    return NULL;
}


PyObject *
PyUnicode_AsUTF32String(PyObject *unicode)
{
    return _PyUnicode_EncodeUTF32(unicode, NULL, 0);
}


/* --- UTF-16 Codec ------------------------------------------------------- */

PyObject *
PyUnicode_DecodeUTF16(const char *s,
                      Py_ssize_t size,
                      const char *errors,
                      int *byteorder)
{
    return PyUnicode_DecodeUTF16Stateful(s, size, errors, byteorder, NULL);
}


PyObject *
PyUnicode_DecodeUTF16Stateful(const char *s,
                              Py_ssize_t size,
                              const char *errors,
                              int *byteorder,
                              Py_ssize_t *consumed)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    _PyUnicodeWriter writer;
    const unsigned char *q, *e;
    int bo = 0;       /* assume native ordering by default */
    int native_ordering;
    const char *errmsg = "";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    const char *encoding;

    q = (const unsigned char *)s;
    e = q + size;

    if (byteorder)
        bo = *byteorder;

    /* Check for BOM marks (U+FEFF) in the input and adjust current
       byte order setting accordingly. In native mode, the leading BOM
       mark is skipped, in all other modes, it is copied to the output
       stream as-is (giving a ZWNBSP character). */
    if (bo == 0 && size >= 2) {
        const Py_UCS4 bom = (q[1] << 8) | q[0];
        if (bom == 0xFEFF) {
            q += 2;
            bo = -1;
        }
        else if (bom == 0xFFFE) {
            q += 2;
            bo = 1;
        }
        if (byteorder)
            *byteorder = bo;
    }

    if (q == e) {
        if (consumed)
            *consumed = size;
        _Py_RETURN_UNICODE_EMPTY();
    }

#if PY_LITTLE_ENDIAN
    native_ordering = bo <= 0;
    encoding = bo <= 0 ? "utf-16-le" : "utf-16-be";
#else
    native_ordering = bo >= 0;
    encoding = bo >= 0 ? "utf-16-be" : "utf-16-le";
#endif

    /* Note: size will always be longer than the resulting Unicode
       character count normally.  Error handler will take care of
       resizing when needed. */
    _PyUnicodeWriter_Init(&writer);
    writer.min_length = (e - q + 1) / 2;
    if (_PyUnicodeWriter_Prepare(&writer, writer.min_length, 127) == -1)
        goto onError;

    while (1) {
        Py_UCS4 ch = 0;
        if (e - q >= 2) {
            int kind = writer.kind;
            if (kind == PyUnicode_1BYTE_KIND) {
                if (PyUnicode_IS_ASCII(writer.buffer))
                    ch = asciilib_utf16_decode(&q, e,
                            (Py_UCS1*)writer.data, &writer.pos,
                            native_ordering);
                else
                    ch = ucs1lib_utf16_decode(&q, e,
                            (Py_UCS1*)writer.data, &writer.pos,
                            native_ordering);
            } else if (kind == PyUnicode_2BYTE_KIND) {
                ch = ucs2lib_utf16_decode(&q, e,
                        (Py_UCS2*)writer.data, &writer.pos,
                        native_ordering);
            } else {
                assert(kind == PyUnicode_4BYTE_KIND);
                ch = ucs4lib_utf16_decode(&q, e,
                        (Py_UCS4*)writer.data, &writer.pos,
                        native_ordering);
            }
        }

        switch (ch)
        {
        case 0:
            /* remaining byte at the end? (size should be even) */
            if (q == e || consumed)
                goto End;
            errmsg = "truncated data";
            startinpos = ((const char *)q) - starts;
            endinpos = ((const char *)e) - starts;
            break;
            /* The remaining input chars are ignored if the callback
               chooses to skip the input */
        case 1:
            q -= 2;
            if (consumed)
                goto End;
            errmsg = "unexpected end of data";
            startinpos = ((const char *)q) - starts;
            endinpos = ((const char *)e) - starts;
            break;
        case 2:
            errmsg = "illegal encoding";
            startinpos = ((const char *)q) - 2 - starts;
            endinpos = startinpos + 2;
            break;
        case 3:
            errmsg = "illegal UTF-16 surrogate";
            startinpos = ((const char *)q) - 4 - starts;
            endinpos = startinpos + 2;
            break;
        default:
            if (_PyUnicodeWriter_WriteCharInline(&writer, ch) < 0)
                goto onError;
            continue;
        }

        if (unicode_decode_call_errorhandler_writer(
                errors,
                &errorHandler,
                encoding, errmsg,
                &starts,
                (const char **)&e,
                &startinpos,
                &endinpos,
                &exc,
                (const char **)&q,
                &writer))
            goto onError;
    }

End:
    if (consumed)
        *consumed = (const char *)q-starts;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}


PyObject *
_PyUnicode_EncodeUTF16(PyObject *str,
                       const char *errors,
                       int byteorder)
{
    if (!PyUnicode_Check(str)) {
        PyErr_BadArgument();
        return NULL;
    }
    int kind = PyUnicode_KIND(str);
    const void *data = PyUnicode_DATA(str);
    Py_ssize_t len = PyUnicode_GET_LENGTH(str);

    Py_ssize_t pairs = 0;
    if (kind == PyUnicode_4BYTE_KIND) {
        const Py_UCS4 *in = (const Py_UCS4 *)data;
        const Py_UCS4 *end = in + len;
        while (in < end) {
            if (*in++ >= 0x10000) {
                pairs++;
            }
        }
    }
    if (len > PY_SSIZE_T_MAX / 2 - pairs - (byteorder == 0)) {
        return PyErr_NoMemory();
    }
    Py_ssize_t nsize = len + pairs + (byteorder == 0);

#if PY_BIG_ENDIAN
    int native_ordering = byteorder >= 0;
#else
    int native_ordering = byteorder <= 0;
#endif

    if (kind == PyUnicode_1BYTE_KIND) {
        // gh-139156: Don't use PyBytesWriter API here since it has an overhead
        // on short strings
        PyObject *v = PyBytes_FromStringAndSize(NULL, nsize * 2);
        if (v == NULL) {
            return NULL;
        }

        /* output buffer is 2-bytes aligned */
        assert(_Py_IS_ALIGNED(PyBytes_AS_STRING(v), 2));
        unsigned short *out = (unsigned short *)PyBytes_AS_STRING(v);
        if (byteorder == 0) {
            *out++ = 0xFEFF;
        }
        if (len > 0) {
            ucs1lib_utf16_encode((const Py_UCS1 *)data, len, &out, native_ordering);
        }
        return v;
    }

    PyBytesWriter *writer = PyBytesWriter_Create(nsize * 2);
    if (writer == NULL) {
        return NULL;
    }

    /* output buffer is 2-bytes aligned */
    assert(_Py_IS_ALIGNED(PyBytesWriter_GetData(writer), 2));
    unsigned short *out = PyBytesWriter_GetData(writer);
    if (byteorder == 0) {
        *out++ = 0xFEFF;
    }
    if (len == 0) {
        return PyBytesWriter_Finish(writer);
    }

    const char *encoding;
    if (byteorder < 0) {
        encoding = "utf-16-le";
    }
    else if (byteorder > 0) {
        encoding = "utf-16-be";
    }
    else {
        encoding = "utf-16";
    }

    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    PyObject *rep = NULL;

    for (Py_ssize_t pos = 0; pos < len; ) {
        if (kind == PyUnicode_2BYTE_KIND) {
            pos += ucs2lib_utf16_encode((const Py_UCS2 *)data + pos, len - pos,
                                        &out, native_ordering);
        }
        else {
            assert(kind == PyUnicode_4BYTE_KIND);
            pos += ucs4lib_utf16_encode((const Py_UCS4 *)data + pos, len - pos,
                                        &out, native_ordering);
        }
        if (pos == len)
            break;

        Py_ssize_t newpos;
        rep = unicode_encode_call_errorhandler(
                errors, &errorHandler,
                encoding, "surrogates not allowed",
                str, &exc, pos, pos + 1, &newpos);
        if (!rep)
            goto error;

        Py_ssize_t repsize, moreunits;
        if (PyBytes_Check(rep)) {
            repsize = PyBytes_GET_SIZE(rep);
            if (repsize & 1) {
                raise_encode_exception(&exc, encoding,
                                       str, pos, pos + 1,
                                       "surrogates not allowed");
                goto error;
            }
            moreunits = repsize / 2;
        }
        else {
            assert(PyUnicode_Check(rep));
            moreunits = repsize = PyUnicode_GET_LENGTH(rep);
            if (!PyUnicode_IS_ASCII(rep)) {
                raise_encode_exception(&exc, encoding,
                                       str, pos, pos + 1,
                                       "surrogates not allowed");
                goto error;
            }
        }
        moreunits += pos - newpos;
        pos = newpos;

        /* two bytes are reserved for each surrogate */
        if (moreunits > 0) {
            out = PyBytesWriter_GrowAndUpdatePointer(writer, 2 * moreunits, out);
            if (out == NULL) {
                goto error;
            }
        }

        if (PyBytes_Check(rep)) {
            memcpy(out, PyBytes_AS_STRING(rep), repsize);
            out += repsize / 2;
        } else {
            /* rep is unicode */
            assert(PyUnicode_KIND(rep) == PyUnicode_1BYTE_KIND);
            ucs1lib_utf16_encode(PyUnicode_1BYTE_DATA(rep), repsize,
                                 &out, native_ordering);
        }

        Py_CLEAR(rep);
    }

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);

    /* Cut back to size actually needed. This is necessary for, for example,
    encoding of a string containing isolated surrogates and the 'ignore' handler
    is used. */
    return PyBytesWriter_FinishWithPointer(writer, out);

  error:
    Py_XDECREF(rep);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    PyBytesWriter_Discard(writer);
    return NULL;
}


PyObject *
PyUnicode_AsUTF16String(PyObject *unicode)
{
    return _PyUnicode_EncodeUTF16(unicode, NULL, 0);
}


_PyUnicode_Name_CAPI *
_PyUnicode_GetNameCAPI(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_Name_CAPI *ucnhash_capi;

    ucnhash_capi = _Py_atomic_load_ptr(&interp->unicode.ucnhash_capi);
    if (ucnhash_capi == NULL) {
        ucnhash_capi = (_PyUnicode_Name_CAPI *)PyCapsule_Import(
                PyUnicodeData_CAPSULE_NAME, 1);

        // It's fine if we overwrite the value here. It's always the same value.
        _Py_atomic_store_ptr(&interp->unicode.ucnhash_capi, ucnhash_capi);
    }
    return ucnhash_capi;
}


/* --- Unicode Escape Codec ----------------------------------------------- */

PyObject *
_PyUnicode_DecodeUnicodeEscapeInternal2(const char *s,
                               Py_ssize_t size,
                               const char *errors,
                               Py_ssize_t *consumed,
                               int *first_invalid_escape_char,
                               const char **first_invalid_escape_ptr)
{
    const char *starts = s;
    const char *initial_starts = starts;
    _PyUnicodeWriter writer;
    const char *end;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    _PyUnicode_Name_CAPI *ucnhash_capi;

    // so we can remember if we've seen an invalid escape char or not
    *first_invalid_escape_char = -1;
    *first_invalid_escape_ptr = NULL;

    if (size == 0) {
        if (consumed) {
            *consumed = 0;
        }
        _Py_RETURN_UNICODE_EMPTY();
    }
    /* Escaped strings will always be longer than the resulting
       Unicode string, so we start with size here and then reduce the
       length after conversion to the true value.
       (but if the error callback returns a long replacement string
       we'll have to allocate more space) */
    _PyUnicodeWriter_Init(&writer);
    writer.min_length = size;
    if (_PyUnicodeWriter_Prepare(&writer, size, 127) < 0) {
        goto onError;
    }

    end = s + size;
    while (s < end) {
        unsigned char c = (unsigned char) *s++;
        Py_UCS4 ch;
        int count;
        const char *message;

#define WRITE_ASCII_CHAR(ch)                                                  \
            do {                                                              \
                assert(ch <= 127);                                            \
                assert(writer.pos < writer.size);                             \
                PyUnicode_WRITE(writer.kind, writer.data, writer.pos++, ch);  \
            } while(0)

#define WRITE_CHAR(ch)                                                        \
            do {                                                              \
                if (ch <= writer.maxchar) {                                   \
                    assert(writer.pos < writer.size);                         \
                    PyUnicode_WRITE(writer.kind, writer.data, writer.pos++, ch); \
                }                                                             \
                else if (_PyUnicodeWriter_WriteCharInline(&writer, ch) < 0) { \
                    goto onError;                                             \
                }                                                             \
            } while(0)

        /* Non-escape characters are interpreted as Unicode ordinals */
        if (c != '\\') {
            WRITE_CHAR(c);
            continue;
        }

        Py_ssize_t startinpos = s - starts - 1;
        /* \ - Escapes */
        if (s >= end) {
            message = "\\ at end of string";
            goto incomplete;
        }
        c = (unsigned char) *s++;

        assert(writer.pos < writer.size);
        switch (c) {

            /* \x escapes */
        case '\n': continue;
        case '\\': WRITE_ASCII_CHAR('\\'); continue;
        case '\'': WRITE_ASCII_CHAR('\''); continue;
        case '\"': WRITE_ASCII_CHAR('\"'); continue;
        case 'b': WRITE_ASCII_CHAR('\b'); continue;
        /* FF */
        case 'f': WRITE_ASCII_CHAR('\014'); continue;
        case 't': WRITE_ASCII_CHAR('\t'); continue;
        case 'n': WRITE_ASCII_CHAR('\n'); continue;
        case 'r': WRITE_ASCII_CHAR('\r'); continue;
        /* VT */
        case 'v': WRITE_ASCII_CHAR('\013'); continue;
        /* BEL, not classic C */
        case 'a': WRITE_ASCII_CHAR('\007'); continue;

            /* \OOO (octal) escapes */
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            ch = c - '0';
            if (s < end && '0' <= *s && *s <= '7') {
                ch = (ch<<3) + *s++ - '0';
                if (s < end && '0' <= *s && *s <= '7') {
                    ch = (ch<<3) + *s++ - '0';
                }
            }
            if (ch > 0377) {
                if (*first_invalid_escape_char == -1) {
                    *first_invalid_escape_char = ch;
                    if (starts == initial_starts) {
                        /* Back up 3 chars, since we've already incremented s. */
                        *first_invalid_escape_ptr = s - 3;
                    }
                }
            }
            WRITE_CHAR(ch);
            continue;

            /* hex escapes */
            /* \xXX */
        case 'x':
            count = 2;
            message = "truncated \\xXX escape";
            goto hexescape;

            /* \uXXXX */
        case 'u':
            count = 4;
            message = "truncated \\uXXXX escape";
            goto hexescape;

            /* \UXXXXXXXX */
        case 'U':
            count = 8;
            message = "truncated \\UXXXXXXXX escape";
        hexescape:
            for (ch = 0; count; ++s, --count) {
                if (s >= end) {
                    goto incomplete;
                }
                c = (unsigned char)*s;
                ch <<= 4;
                if (c >= '0' && c <= '9') {
                    ch += c - '0';
                }
                else if (c >= 'a' && c <= 'f') {
                    ch += c - ('a' - 10);
                }
                else if (c >= 'A' && c <= 'F') {
                    ch += c - ('A' - 10);
                }
                else {
                    goto error;
                }
            }

            /* when we get here, ch is a 32-bit unicode character */
            if (ch > MAX_UNICODE) {
                message = "illegal Unicode character";
                goto error;
            }

            WRITE_CHAR(ch);
            continue;

            /* \N{name} */
        case 'N':
            ucnhash_capi = _PyUnicode_GetNameCAPI();
            if (ucnhash_capi == NULL) {
                PyErr_SetString(
                        PyExc_UnicodeError,
                        "\\N escapes not supported (can't load unicodedata module)"
                );
                goto onError;
            }

            message = "malformed \\N character escape";
            if (s >= end) {
                goto incomplete;
            }
            if (*s == '{') {
                const char *start = ++s;
                size_t namelen;
                /* look for the closing brace */
                while (s < end && *s != '}')
                    s++;
                if (s >= end) {
                    goto incomplete;
                }
                namelen = s - start;
                if (namelen) {
                    /* found a name.  look it up in the unicode database */
                    s++;
                    ch = 0xffffffff; /* in case 'getcode' messes up */
                    if (namelen <= INT_MAX &&
                        ucnhash_capi->getcode(start, (int)namelen,
                                              &ch, 0)) {
                        assert(ch <= MAX_UNICODE);
                        WRITE_CHAR(ch);
                        continue;
                    }
                    message = "unknown Unicode character name";
                }
            }
            goto error;

        default:
            if (*first_invalid_escape_char == -1) {
                *first_invalid_escape_char = c;
                if (starts == initial_starts) {
                    /* Back up one char, since we've already incremented s. */
                    *first_invalid_escape_ptr = s - 1;
                }
            }
            WRITE_ASCII_CHAR('\\');
            WRITE_CHAR(c);
            continue;
        }

      incomplete:
        if (consumed) {
            *consumed = startinpos;
            break;
        }
      error:;
        Py_ssize_t endinpos = s-starts;
        writer.min_length = end - s + writer.pos;
        if (unicode_decode_call_errorhandler_writer(
                errors, &errorHandler,
                "unicodeescape", message,
                &starts, &end, &startinpos, &endinpos, &exc, &s,
                &writer)) {
            goto onError;
        }
        assert(end - s <= writer.size - writer.pos);

#undef WRITE_ASCII_CHAR
#undef WRITE_CHAR
    }

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}


PyObject *
_PyUnicode_DecodeUnicodeEscapeStateful(const char *s,
                              Py_ssize_t size,
                              const char *errors,
                              Py_ssize_t *consumed)
{
    int first_invalid_escape_char;
    const char *first_invalid_escape_ptr;
    PyObject *result = _PyUnicode_DecodeUnicodeEscapeInternal2(s, size, errors,
                                                      consumed,
                                                      &first_invalid_escape_char,
                                                      &first_invalid_escape_ptr);
    if (result == NULL)
        return NULL;
    if (first_invalid_escape_char != -1) {
        if (first_invalid_escape_char > 0xff) {
            if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                                 "\"\\%o\" is an invalid octal escape sequence. "
                                 "Such sequences will not work in the future. ",
                                 first_invalid_escape_char) < 0)
            {
                Py_DECREF(result);
                return NULL;
            }
        }
        else {
            if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                                 "\"\\%c\" is an invalid escape sequence. "
                                 "Such sequences will not work in the future. ",
                                 first_invalid_escape_char) < 0)
            {
                Py_DECREF(result);
                return NULL;
            }
        }
    }
    return result;
}


PyObject *
PyUnicode_DecodeUnicodeEscape(const char *s,
                              Py_ssize_t size,
                              const char *errors)
{
    return _PyUnicode_DecodeUnicodeEscapeStateful(s, size, errors, NULL);
}


/* Return a Unicode-Escape string version of the Unicode object. */
PyObject *
PyUnicode_AsUnicodeEscapeString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    if (len == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);

    /* Initial allocation is based on the longest-possible character
     * escape.
     *
     * For UCS1 strings it's '\xxx', 4 bytes per source character.
     * For UCS2 strings it's '\uxxxx', 6 bytes per source character.
     * For UCS4 strings it's '\U00xxxxxx', 10 bytes per source character. */
    Py_ssize_t expandsize = kind * 2 + 2;
    if (len > PY_SSIZE_T_MAX / expandsize) {
        return PyErr_NoMemory();
    }

    PyBytesWriter *writer = PyBytesWriter_Create(expandsize * len);
    if (writer == NULL) {
        return NULL;
    }
    char *p = PyBytesWriter_GetData(writer);

    for (Py_ssize_t i = 0; i < len; i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        /* U+0000-U+00ff range */
        if (ch < 0x100) {
            if (ch >= ' ' && ch < 127) {
                if (ch != '\\') {
                    /* Copy printable US ASCII as-is */
                    *p++ = (char) ch;
                }
                /* Escape backslashes */
                else {
                    *p++ = '\\';
                    *p++ = '\\';
                }
            }

            /* Map special whitespace to '\t', \n', '\r' */
            else if (ch == '\t') {
                *p++ = '\\';
                *p++ = 't';
            }
            else if (ch == '\n') {
                *p++ = '\\';
                *p++ = 'n';
            }
            else if (ch == '\r') {
                *p++ = '\\';
                *p++ = 'r';
            }

            /* Map non-printable US ASCII and 8-bit characters to '\xHH' */
            else {
                *p++ = '\\';
                *p++ = 'x';
                *p++ = Py_hexdigits[(ch >> 4) & 0x000F];
                *p++ = Py_hexdigits[ch & 0x000F];
            }
        }
        /* U+0100-U+ffff range: Map 16-bit characters to '\uHHHH' */
        else if (ch < 0x10000) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = Py_hexdigits[(ch >> 12) & 0x000F];
            *p++ = Py_hexdigits[(ch >> 8) & 0x000F];
            *p++ = Py_hexdigits[(ch >> 4) & 0x000F];
            *p++ = Py_hexdigits[ch & 0x000F];
        }
        /* U+010000-U+10ffff range: Map 21-bit characters to '\U00HHHHHH' */
        else {

            /* Make sure that the first two digits are zero */
            assert(ch <= MAX_UNICODE && MAX_UNICODE <= 0x10ffff);
            *p++ = '\\';
            *p++ = 'U';
            *p++ = '0';
            *p++ = '0';
            *p++ = Py_hexdigits[(ch >> 20) & 0x0000000F];
            *p++ = Py_hexdigits[(ch >> 16) & 0x0000000F];
            *p++ = Py_hexdigits[(ch >> 12) & 0x0000000F];
            *p++ = Py_hexdigits[(ch >> 8) & 0x0000000F];
            *p++ = Py_hexdigits[(ch >> 4) & 0x0000000F];
            *p++ = Py_hexdigits[ch & 0x0000000F];
        }
    }

    return PyBytesWriter_FinishWithPointer(writer, p);
}


/* --- Raw Unicode Escape Codec ------------------------------------------- */

PyObject *
_PyUnicode_DecodeRawUnicodeEscapeStateful(const char *s,
                                          Py_ssize_t size,
                                          const char *errors,
                                          Py_ssize_t *consumed)
{
    const char *starts = s;
    _PyUnicodeWriter writer;
    const char *end;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    if (size == 0) {
        if (consumed) {
            *consumed = 0;
        }
        _Py_RETURN_UNICODE_EMPTY();
    }

    /* Escaped strings will always be longer than the resulting
       Unicode string, so we start with size here and then reduce the
       length after conversion to the true value. (But decoding error
       handler might have to resize the string) */
    _PyUnicodeWriter_Init(&writer);
    writer.min_length = size;
    if (_PyUnicodeWriter_Prepare(&writer, size, 127) < 0) {
        goto onError;
    }

    end = s + size;
    while (s < end) {
        unsigned char c = (unsigned char) *s++;
        Py_UCS4 ch;
        int count;
        const char *message;

#define WRITE_CHAR(ch)                                                        \
            do {                                                              \
                if (ch <= writer.maxchar) {                                   \
                    assert(writer.pos < writer.size);                         \
                    PyUnicode_WRITE(writer.kind, writer.data, writer.pos++, ch); \
                }                                                             \
                else if (_PyUnicodeWriter_WriteCharInline(&writer, ch) < 0) { \
                    goto onError;                                             \
                }                                                             \
            } while(0)

        /* Non-escape characters are interpreted as Unicode ordinals */
        if (c != '\\' || (s >= end && !consumed)) {
            WRITE_CHAR(c);
            continue;
        }

        Py_ssize_t startinpos = s - starts - 1;
        /* \ - Escapes */
        if (s >= end) {
            assert(consumed);
            // Set message to silent compiler warning.
            // Actually it is never used.
            message = "\\ at end of string";
            goto incomplete;
        }

        c = (unsigned char) *s++;
        if (c == 'u') {
            count = 4;
            message = "truncated \\uXXXX escape";
        }
        else if (c == 'U') {
            count = 8;
            message = "truncated \\UXXXXXXXX escape";
        }
        else {
            assert(writer.pos < writer.size);
            PyUnicode_WRITE(writer.kind, writer.data, writer.pos++, '\\');
            WRITE_CHAR(c);
            continue;
        }

        /* \uHHHH with 4 hex digits, \U00HHHHHH with 8 */
        for (ch = 0; count; ++s, --count) {
            if (s >= end) {
                goto incomplete;
            }
            c = (unsigned char)*s;
            ch <<= 4;
            if (c >= '0' && c <= '9') {
                ch += c - '0';
            }
            else if (c >= 'a' && c <= 'f') {
                ch += c - ('a' - 10);
            }
            else if (c >= 'A' && c <= 'F') {
                ch += c - ('A' - 10);
            }
            else {
                goto error;
            }
        }
        if (ch > MAX_UNICODE) {
            message = "\\Uxxxxxxxx out of range";
            goto error;
        }
        WRITE_CHAR(ch);
        continue;

      incomplete:
        if (consumed) {
            *consumed = startinpos;
            break;
        }
      error:;
        Py_ssize_t endinpos = s-starts;
        writer.min_length = end - s + writer.pos;
        if (unicode_decode_call_errorhandler_writer(
                errors, &errorHandler,
                "rawunicodeescape", message,
                &starts, &end, &startinpos, &endinpos, &exc, &s,
                &writer)) {
            goto onError;
        }
        assert(end - s <= writer.size - writer.pos);

#undef WRITE_CHAR
    }
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}


PyObject *
PyUnicode_DecodeRawUnicodeEscape(const char *s,
                                 Py_ssize_t size,
                                 const char *errors)
{
    return _PyUnicode_DecodeRawUnicodeEscapeStateful(s, size, errors, NULL);
}


PyObject *
PyUnicode_AsRawUnicodeEscapeString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    if (len == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    if (kind == PyUnicode_1BYTE_KIND) {
        return PyBytes_FromStringAndSize(data, len);
    }

    /* 4 byte characters can take up 10 bytes, 2 byte characters can take up 6
       bytes, and 1 byte characters 4. */
    Py_ssize_t expandsize = kind * 2 + 2;
    if (len > PY_SSIZE_T_MAX / expandsize) {
        return PyErr_NoMemory();
    }

    PyBytesWriter *writer = PyBytesWriter_Create(expandsize * len);
    if (writer == NULL) {
        return NULL;
    }
    char *p = PyBytesWriter_GetData(writer);

    for (Py_ssize_t pos = 0; pos < len; pos++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, pos);

        /* U+0000-U+00ff range: Copy 8-bit characters as-is */
        if (ch < 0x100) {
            *p++ = (char) ch;
        }
        /* U+0100-U+ffff range: Map 16-bit characters to '\uHHHH' */
        else if (ch < 0x10000) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = Py_hexdigits[(ch >> 12) & 0xf];
            *p++ = Py_hexdigits[(ch >> 8) & 0xf];
            *p++ = Py_hexdigits[(ch >> 4) & 0xf];
            *p++ = Py_hexdigits[ch & 15];
        }
        /* U+010000-U+10ffff range: Map 32-bit characters to '\U00HHHHHH' */
        else {
            assert(ch <= MAX_UNICODE && MAX_UNICODE <= 0x10ffff);
            *p++ = '\\';
            *p++ = 'U';
            *p++ = '0';
            *p++ = '0';
            *p++ = Py_hexdigits[(ch >> 20) & 0xf];
            *p++ = Py_hexdigits[(ch >> 16) & 0xf];
            *p++ = Py_hexdigits[(ch >> 12) & 0xf];
            *p++ = Py_hexdigits[(ch >> 8) & 0xf];
            *p++ = Py_hexdigits[(ch >> 4) & 0xf];
            *p++ = Py_hexdigits[ch & 15];
        }
    }

    return PyBytesWriter_FinishWithPointer(writer, p);
}


/* --- Latin-1 Codec ------------------------------------------------------ */

PyObject *
PyUnicode_DecodeLatin1(const char *s,
                       Py_ssize_t size,
                       const char *errors)
{
    /* Latin-1 is equivalent to the first 256 ordinals in Unicode. */
    return _PyUnicode_FromUCS1((const unsigned char*)s, size);
}


/* create or adjust a UnicodeEncodeError */
static void
make_encode_exception(PyObject **exceptionObject,
                      const char *encoding,
                      PyObject *unicode,
                      Py_ssize_t startpos, Py_ssize_t endpos,
                      const char *reason)
{
    if (*exceptionObject == NULL) {
        *exceptionObject = PyObject_CallFunction(
            PyExc_UnicodeEncodeError, "sOnns",
            encoding, unicode, startpos, endpos, reason);
    }
    else {
        if (PyUnicodeEncodeError_SetStart(*exceptionObject, startpos))
            goto onError;
        if (PyUnicodeEncodeError_SetEnd(*exceptionObject, endpos))
            goto onError;
        if (PyUnicodeEncodeError_SetReason(*exceptionObject, reason))
            goto onError;
        return;
      onError:
        Py_CLEAR(*exceptionObject);
    }
}


/* raises a UnicodeEncodeError */
static void
raise_encode_exception(PyObject **exceptionObject,
                       const char *encoding,
                       PyObject *unicode,
                       Py_ssize_t startpos, Py_ssize_t endpos,
                       const char *reason)
{
    make_encode_exception(exceptionObject,
                          encoding, unicode, startpos, endpos, reason);
    if (*exceptionObject != NULL)
        PyCodec_StrictErrors(*exceptionObject);
}


/* error handling callback helper:
   build arguments, call the callback and check the arguments,
   put the result into newpos and return the replacement string, which
   has to be freed by the caller */
static PyObject *
unicode_encode_call_errorhandler(const char *errors,
                                 PyObject **errorHandler,
                                 const char *encoding, const char *reason,
                                 PyObject *unicode, PyObject **exceptionObject,
                                 Py_ssize_t startpos, Py_ssize_t endpos,
                                 Py_ssize_t *newpos)
{
    static const char *argparse = "On;encoding error handler must return (str/bytes, int) tuple";
    Py_ssize_t len;
    PyObject *restuple;
    PyObject *resunicode;

    if (*errorHandler == NULL) {
        *errorHandler = PyCodec_LookupError(errors);
        if (*errorHandler == NULL)
            return NULL;
    }

    len = PyUnicode_GET_LENGTH(unicode);

    make_encode_exception(exceptionObject,
                          encoding, unicode, startpos, endpos, reason);
    if (*exceptionObject == NULL)
        return NULL;

    restuple = PyObject_CallOneArg(*errorHandler, *exceptionObject);
    if (restuple == NULL)
        return NULL;
    if (!PyTuple_Check(restuple)) {
        PyErr_SetString(PyExc_TypeError, &argparse[3]);
        Py_DECREF(restuple);
        return NULL;
    }
    if (!PyArg_ParseTuple(restuple, argparse,
                          &resunicode, newpos)) {
        Py_DECREF(restuple);
        return NULL;
    }
    if (!PyUnicode_Check(resunicode) && !PyBytes_Check(resunicode)) {
        PyErr_SetString(PyExc_TypeError, &argparse[3]);
        Py_DECREF(restuple);
        return NULL;
    }
    if (*newpos<0)
        *newpos = len + *newpos;
    if (*newpos<0 || *newpos>len) {
        PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", *newpos);
        Py_DECREF(restuple);
        return NULL;
    }
    Py_INCREF(resunicode);
    Py_DECREF(restuple);
    return resunicode;
}


static PyObject *
unicode_encode_ucs1(PyObject *unicode,
                    const char *errors,
                    const Py_UCS4 limit)
{
    /* input state */
    Py_ssize_t pos=0, size;
    int kind;
    const void *data;
    const char *encoding = (limit == 256) ? "latin-1" : "ascii";
    const char *reason = (limit == 256) ? "ordinal not in range(256)" : "ordinal not in range(128)";
    PyObject *error_handler_obj = NULL;
    PyObject *exc = NULL;
    _Py_error_handler error_handler = _Py_ERROR_UNKNOWN;
    PyObject *rep = NULL;

    size = PyUnicode_GET_LENGTH(unicode);
    kind = PyUnicode_KIND(unicode);
    data = PyUnicode_DATA(unicode);
    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    if (size == 0)
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);

    /* output object */
    PyBytesWriter *writer = PyBytesWriter_Create(size);
    if (writer == NULL) {
        return NULL;
    }
    /* pointer into the output */
    char *str = PyBytesWriter_GetData(writer);

    while (pos < size) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, pos);

        /* can we encode this? */
        if (ch < limit) {
            /* no overflow check, because we know that the space is enough */
            *str++ = (char)ch;
            ++pos;
        }
        else {
            Py_ssize_t newpos, i;
            /* startpos for collecting unencodable chars */
            Py_ssize_t collstart = pos;
            Py_ssize_t collend = collstart + 1;
            /* find all unecodable characters */

            while ((collend < size) && (PyUnicode_READ(kind, data, collend) >= limit))
                ++collend;

            /* Only overallocate the buffer if it's not the last write */
            writer->overallocate = (collend < size);

            /* cache callback name lookup (if not done yet, i.e. it's the first error) */
            if (error_handler == _Py_ERROR_UNKNOWN)
                error_handler = _Py_GetErrorHandler(errors);

            switch (error_handler) {
            case _Py_ERROR_STRICT:
                raise_encode_exception(&exc, encoding, unicode, collstart, collend, reason);
                goto onError;

            case _Py_ERROR_REPLACE:
                memset(str, '?', collend - collstart);
                str += (collend - collstart);
                _Py_FALLTHROUGH;
            case _Py_ERROR_IGNORE:
                pos = collend;
                break;

            case _Py_ERROR_BACKSLASHREPLACE:
                /* subtract preallocated bytes */
                writer->size -= (collend - collstart);
                str = backslashreplace(writer, str,
                                       unicode, collstart, collend);
                if (str == NULL)
                    goto onError;
                pos = collend;
                break;

            case _Py_ERROR_XMLCHARREFREPLACE:
                /* subtract preallocated bytes */
                writer->size -= (collend - collstart);
                str = xmlcharrefreplace(writer, str,
                                        unicode, collstart, collend);
                if (str == NULL)
                    goto onError;
                pos = collend;
                break;

            case _Py_ERROR_SURROGATEESCAPE:
                for (i = collstart; i < collend; ++i) {
                    ch = PyUnicode_READ(kind, data, i);
                    if (ch < 0xdc80 || 0xdcff < ch) {
                        /* Not a UTF-8b surrogate */
                        break;
                    }
                    *str++ = (char)(ch - 0xdc00);
                    ++pos;
                }
                if (i >= collend)
                    break;
                collstart = pos;
                assert(collstart != collend);
                _Py_FALLTHROUGH;

            default:
                rep = unicode_encode_call_errorhandler(errors, &error_handler_obj,
                                                       encoding, reason, unicode, &exc,
                                                       collstart, collend, &newpos);
                if (rep == NULL)
                    goto onError;

                if (newpos < collstart) {
                    writer->overallocate = 1;
                    str = PyBytesWriter_GrowAndUpdatePointer(writer,
                                                             collstart - newpos,
                                                             str);
                    if (str == NULL) {
                        goto onError;
                    }
                }
                else {
                    /* subtract preallocated bytes */
                    writer->size -= newpos - collstart;
                    /* Only overallocate the buffer if it's not the last write */
                    writer->overallocate = (newpos < size);
                }

                char *rep_str;
                Py_ssize_t rep_len;
                if (PyBytes_Check(rep)) {
                    /* Directly copy bytes result to output. */
                    rep_str = PyBytes_AS_STRING(rep);
                    rep_len = PyBytes_GET_SIZE(rep);
                }
                else {
                    assert(PyUnicode_Check(rep));

                    if (limit == 256 ?
                        PyUnicode_KIND(rep) != PyUnicode_1BYTE_KIND :
                        !PyUnicode_IS_ASCII(rep))
                    {
                        /* Not all characters are smaller than limit */
                        raise_encode_exception(&exc, encoding, unicode,
                                               collstart, collend, reason);
                        goto onError;
                    }
                    assert(PyUnicode_KIND(rep) == PyUnicode_1BYTE_KIND);
                    rep_str = PyUnicode_DATA(rep);
                    rep_len = PyUnicode_GET_LENGTH(rep);
                }

                str = PyBytesWriter_GrowAndUpdatePointer(writer, rep_len, str);
                if (str == NULL) {
                    goto onError;
                }
                memcpy(str, rep_str, rep_len);
                str += rep_len;

                pos = newpos;
                Py_CLEAR(rep);
            }

            /* If overallocation was disabled, ensure that it was the last
               write. Otherwise, we missed an optimization */
            assert(writer->overallocate || pos == size);
        }
    }

    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return PyBytesWriter_FinishWithPointer(writer, str);

  onError:
    Py_XDECREF(rep);
    PyBytesWriter_Discard(writer);
    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return NULL;
}


PyObject *
_PyUnicode_AsLatin1String(PyObject *unicode, const char *errors)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    /* Fast path: if it is a one-byte string, construct
       bytes object directly. */
    if (PyUnicode_KIND(unicode) == PyUnicode_1BYTE_KIND)
        return PyBytes_FromStringAndSize(PyUnicode_DATA(unicode),
                                         PyUnicode_GET_LENGTH(unicode));
    /* Non-Latin-1 characters present. Defer to above function to
       raise the exception. */
    return unicode_encode_ucs1(unicode, errors, 256);
}


PyObject*
PyUnicode_AsLatin1String(PyObject *unicode)
{
    return _PyUnicode_AsLatin1String(unicode, NULL);
}


/* --- 7-bit ASCII Codec -------------------------------------------------- */

PyObject *
PyUnicode_DecodeASCII(const char *s,
                      Py_ssize_t size,
                      const char *errors)
{
    const char *starts = s;
    const char *e = s + size;
    PyObject *error_handler_obj = NULL;
    PyObject *exc = NULL;
    _Py_error_handler error_handler = _Py_ERROR_UNKNOWN;

    if (size == 0)
        _Py_RETURN_UNICODE_EMPTY();

    /* ASCII is equivalent to the first 128 ordinals in Unicode. */
    if (size == 1 && (unsigned char)s[0] < 128) {
        return get_latin1_char((unsigned char)s[0]);
    }

    // Shortcut for simple case
    PyObject *u = PyUnicode_New(size, 127);
    if (u == NULL) {
        return NULL;
    }
    Py_ssize_t outpos = ascii_decode(s, e, PyUnicode_1BYTE_DATA(u));
    if (outpos == size) {
        return u;
    }

    _PyUnicodeWriter writer;
    _PyUnicodeWriter_InitWithBuffer(&writer, u);
    writer.pos = outpos;

    s += outpos;
    int kind = writer.kind;
    void *data = writer.data;
    Py_ssize_t startinpos, endinpos;

    while (s < e) {
        unsigned char c = (unsigned char)*s;
        if (c < 128) {
            PyUnicode_WRITE(kind, data, writer.pos, c);
            writer.pos++;
            ++s;
            continue;
        }

        /* byte outsize range 0x00..0x7f: call the error handler */

        if (error_handler == _Py_ERROR_UNKNOWN)
            error_handler = _Py_GetErrorHandler(errors);

        switch (error_handler)
        {
        case _Py_ERROR_REPLACE:
        case _Py_ERROR_SURROGATEESCAPE:
            /* Fast-path: the error handler only writes one character,
               but we may switch to UCS2 at the first write */
            if (_PyUnicodeWriter_PrepareKind(&writer, PyUnicode_2BYTE_KIND) < 0)
                goto onError;
            kind = writer.kind;
            data = writer.data;

            if (error_handler == _Py_ERROR_REPLACE)
                PyUnicode_WRITE(kind, data, writer.pos, 0xfffd);
            else
                PyUnicode_WRITE(kind, data, writer.pos, c + 0xdc00);
            writer.pos++;
            ++s;
            break;

        case _Py_ERROR_IGNORE:
            ++s;
            break;

        default:
            startinpos = s-starts;
            endinpos = startinpos + 1;
            if (unicode_decode_call_errorhandler_writer(
                    errors, &error_handler_obj,
                    "ascii", "ordinal not in range(128)",
                    &starts, &e, &startinpos, &endinpos, &exc, &s,
                    &writer))
                goto onError;
            kind = writer.kind;
            data = writer.data;
        }
    }
    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *
_PyUnicode_AsASCIIString(PyObject *unicode, const char *errors)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    /* Fast path: if it is an ASCII-only string, construct bytes object
       directly. Else defer to above function to raise the exception. */
    if (PyUnicode_IS_ASCII(unicode))
        return PyBytes_FromStringAndSize(PyUnicode_DATA(unicode),
                                         PyUnicode_GET_LENGTH(unicode));
    return unicode_encode_ucs1(unicode, errors, 128);
}

PyObject *
PyUnicode_AsASCIIString(PyObject *unicode)
{
    return _PyUnicode_AsASCIIString(unicode, NULL);
}


#ifdef MS_WINDOWS

/* --- MBCS codecs for Windows -------------------------------------------- */

#if SIZEOF_INT < SIZEOF_SIZE_T
#define NEED_RETRY
#endif

/* INT_MAX is the theoretical largest chunk (or INT_MAX / 2 when
   transcoding from UTF-16), but INT_MAX / 4 performs better in
   both cases also and avoids partial characters overrunning the
   length limit in MultiByteToWideChar on Windows */
#define DECODING_CHUNK_SIZE (INT_MAX/4)

#ifndef WC_ERR_INVALID_CHARS
#  define WC_ERR_INVALID_CHARS 0x0080
#endif

static const char*
code_page_name(UINT code_page, PyObject **obj)
{
    *obj = NULL;
    if (code_page == CP_ACP)
        return "mbcs";

    *obj = PyBytes_FromFormat("cp%u", code_page);
    if (*obj == NULL)
        return NULL;
    return PyBytes_AS_STRING(*obj);
}


static DWORD
decode_code_page_flags(UINT code_page)
{
    if (code_page == CP_UTF7) {
        /* The CP_UTF7 decoder only supports flags=0 */
        return 0;
    }
    else
        return MB_ERR_INVALID_CHARS;
}


/*
 * Decode a byte string from a Windows code page into unicode object in strict
 * mode.
 *
 * Returns consumed size if succeed, returns -2 on decode error, or raise an
 * OSError and returns -1 on other error.
 */
static int
decode_code_page_strict(UINT code_page,
                        wchar_t **buf,
                        Py_ssize_t *bufsize,
                        const char *in,
                        int insize)
{
    DWORD flags = MB_ERR_INVALID_CHARS;
    wchar_t *out;
    DWORD outsize;

    /* First get the size of the result */
    assert(insize > 0);
    while ((outsize = MultiByteToWideChar(code_page, flags,
                                          in, insize, NULL, 0)) <= 0)
    {
        if (!flags || GetLastError() != ERROR_INVALID_FLAGS) {
            goto error;
        }
        /* For some code pages (e.g. UTF-7) flags must be set to 0. */
        flags = 0;
    }

    /* Extend a wchar_t* buffer */
    Py_ssize_t n = *bufsize;   /* Get the current length */
    if (widechar_resize(buf, bufsize, n + outsize) < 0) {
        return -1;
    }
    out = *buf + n;

    /* Do the conversion */
    outsize = MultiByteToWideChar(code_page, flags, in, insize, out, outsize);
    if (outsize <= 0)
        goto error;
    return insize;

error:
    if (GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
        return -2;
    PyErr_SetFromWindowsErr(0);
    return -1;
}


/*
 * Decode a byte string from a code page into unicode object with an error
 * handler.
 *
 * Returns consumed size if succeed, or raise an OSError or
 * UnicodeDecodeError exception and returns -1 on error.
 */
static int
decode_code_page_errors(UINT code_page,
                        wchar_t **buf,
                        Py_ssize_t *bufsize,
                        const char *in, const int size,
                        const char *errors, int final)
{
    const char *startin = in;
    const char *endin = in + size;
    DWORD flags = MB_ERR_INVALID_CHARS;
    /* Ideally, we should get reason from FormatMessage. This is the Windows
       2000 English version of the message. */
    const char *reason = "No mapping for the Unicode character exists "
                         "in the target code page.";
    /* each step cannot decode more than 1 character, but a character can be
       represented as a surrogate pair */
    wchar_t buffer[2], *out;
    int insize;
    Py_ssize_t outsize;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    PyObject *encoding_obj = NULL;
    const char *encoding;
    DWORD err;
    int ret = -1;

    assert(size > 0);

    encoding = code_page_name(code_page, &encoding_obj);
    if (encoding == NULL)
        return -1;

    if ((errors == NULL || strcmp(errors, "strict") == 0) && final) {
        /* The last error was ERROR_NO_UNICODE_TRANSLATION, then we raise a
           UnicodeDecodeError. */
        make_decode_exception(&exc, encoding, in, size, 0, 0, reason);
        if (exc != NULL) {
            PyCodec_StrictErrors(exc);
            Py_CLEAR(exc);
        }
        goto error;
    }

    /* Extend a wchar_t* buffer */
    Py_ssize_t n = *bufsize;   /* Get the current length */
    if (size > (PY_SSIZE_T_MAX - n) / (Py_ssize_t)Py_ARRAY_LENGTH(buffer)) {
        PyErr_NoMemory();
        goto error;
    }
    if (widechar_resize(buf, bufsize, n + size * Py_ARRAY_LENGTH(buffer)) < 0) {
        goto error;
    }
    out = *buf + n;

    /* Decode the byte string character per character */
    while (in < endin)
    {
        /* Decode a character */
        insize = 1;
        do
        {
            outsize = MultiByteToWideChar(code_page, flags,
                                          in, insize,
                                          buffer, Py_ARRAY_LENGTH(buffer));
            if (outsize > 0)
                break;
            err = GetLastError();
            if (err == ERROR_INVALID_FLAGS && flags) {
                /* For some code pages (e.g. UTF-7) flags must be set to 0. */
                flags = 0;
                continue;
            }
            if (err != ERROR_NO_UNICODE_TRANSLATION
                && err != ERROR_INSUFFICIENT_BUFFER)
            {
                PyErr_SetFromWindowsErr(err);
                goto error;
            }
            insize++;
        }
        /* 4=maximum length of a UTF-8 sequence */
        while (insize <= 4 && (in + insize) <= endin);

        if (outsize <= 0) {
            Py_ssize_t startinpos, endinpos, outpos;

            /* last character in partial decode? */
            if (in + insize >= endin && !final)
                break;

            startinpos = in - startin;
            endinpos = startinpos + 1;
            outpos = out - *buf;
            if (unicode_decode_call_errorhandler_wchar(
                    errors, &errorHandler,
                    encoding, reason,
                    &startin, &endin, &startinpos, &endinpos, &exc, &in,
                    buf, bufsize, &outpos))
            {
                goto error;
            }
            out = *buf + outpos;
        }
        else {
            in += insize;
            memcpy(out, buffer, outsize * sizeof(wchar_t));
            out += outsize;
        }
    }

    /* Shrink the buffer */
    assert(out - *buf <= *bufsize);
    *bufsize = out - *buf;
    /* (in - startin) <= size and size is an int */
    ret = Py_SAFE_DOWNCAST(in - startin, Py_ssize_t, int);

error:
    Py_XDECREF(encoding_obj);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return ret;
}


static PyObject *
decode_code_page_stateful(int code_page,
                          const char *s, Py_ssize_t size,
                          const char *errors, Py_ssize_t *consumed)
{
    wchar_t *buf = NULL;
    Py_ssize_t bufsize = 0;
    int chunk_size, final, converted, done;

    if (code_page < 0) {
        PyErr_SetString(PyExc_ValueError, "invalid code page number");
        return NULL;
    }
    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (consumed)
        *consumed = 0;

    do
    {
#ifdef NEED_RETRY
        if (size > DECODING_CHUNK_SIZE) {
            chunk_size = DECODING_CHUNK_SIZE;
            final = 0;
            done = 0;
        }
        else
#endif
        {
            chunk_size = (int)size;
            final = (consumed == NULL);
            done = 1;
        }

        if (chunk_size == 0 && done) {
            if (buf != NULL)
                break;
            _Py_RETURN_UNICODE_EMPTY();
        }

        converted = decode_code_page_strict(code_page, &buf, &bufsize,
                                            s, chunk_size);
        if (converted == -2)
            converted = decode_code_page_errors(code_page, &buf, &bufsize,
                                                s, chunk_size,
                                                errors, final);
        assert(converted != 0 || done);

        if (converted < 0) {
            PyMem_Free(buf);
            return NULL;
        }

        if (consumed)
            *consumed += converted;

        s += converted;
        size -= converted;
    } while (!done);

    PyObject *v = PyUnicode_FromWideChar(buf, bufsize);
    PyMem_Free(buf);
    return v;
}


PyObject *
PyUnicode_DecodeCodePageStateful(int code_page,
                                 const char *s,
                                 Py_ssize_t size,
                                 const char *errors,
                                 Py_ssize_t *consumed)
{
    return decode_code_page_stateful(code_page, s, size, errors, consumed);
}


PyObject *
PyUnicode_DecodeMBCSStateful(const char *s,
                             Py_ssize_t size,
                             const char *errors,
                             Py_ssize_t *consumed)
{
    return decode_code_page_stateful(CP_ACP, s, size, errors, consumed);
}


PyObject *
PyUnicode_DecodeMBCS(const char *s,
                     Py_ssize_t size,
                     const char *errors)
{
    return PyUnicode_DecodeMBCSStateful(s, size, errors, NULL);
}


static DWORD
encode_code_page_flags(UINT code_page, const char *errors)
{
    if (code_page == CP_UTF8) {
        return WC_ERR_INVALID_CHARS;
    }
    else if (code_page == CP_UTF7) {
        /* CP_UTF7 only supports flags=0 */
        return 0;
    }
    else {
        if (errors != NULL && strcmp(errors, "replace") == 0)
            return 0;
        else
            return WC_NO_BEST_FIT_CHARS;
    }
}


/*
 * Encode a Unicode string to a Windows code page into a byte string in strict
 * mode.
 *
 * Returns consumed characters if succeed, returns -2 on encode error, or raise
 * an OSError and returns -1 on other error.
 */
static int
encode_code_page_strict(UINT code_page, PyBytesWriter **writer,
                        PyObject *unicode, Py_ssize_t offset, int len,
                        const char* errors)
{
    BOOL usedDefaultChar = FALSE;
    BOOL *pusedDefaultChar = &usedDefaultChar;
    int outsize;
    wchar_t *p;
    Py_ssize_t size;
    const DWORD flags = encode_code_page_flags(code_page, NULL);
    char *out;
    /* Create a substring so that we can get the UTF-16 representation
       of just the slice under consideration. */
    PyObject *substring;
    int ret = -1;

    assert(len > 0);

    if (code_page != CP_UTF8 && code_page != CP_UTF7)
        pusedDefaultChar = &usedDefaultChar;
    else
        pusedDefaultChar = NULL;

    substring = PyUnicode_Substring(unicode, offset, offset+len);
    if (substring == NULL)
        return -1;
    p = PyUnicode_AsWideCharString(substring, &size);
    Py_CLEAR(substring);
    if (p == NULL) {
        return -1;
    }
    assert(size <= INT_MAX);

    /* First get the size of the result */
    outsize = WideCharToMultiByte(code_page, flags,
                                  p, (int)size,
                                  NULL, 0,
                                  NULL, pusedDefaultChar);
    if (outsize <= 0)
        goto error;
    /* If we used a default char, then we failed! */
    if (pusedDefaultChar && *pusedDefaultChar) {
        ret = -2;
        goto done;
    }

    if (*writer == NULL) {
        /* Create string object */
        *writer = PyBytesWriter_Create(outsize);
        if (*writer == NULL) {
            goto done;
        }
        out = PyBytesWriter_GetData(*writer);
    }
    else {
        /* Extend string object */
        Py_ssize_t n = PyBytesWriter_GetSize(*writer);
        if (PyBytesWriter_Grow(*writer, outsize) < 0) {
            goto done;
        }
        out = (char*)PyBytesWriter_GetData(*writer) + n;
    }

    /* Do the conversion */
    outsize = WideCharToMultiByte(code_page, flags,
                                  p, (int)size,
                                  out, outsize,
                                  NULL, pusedDefaultChar);
    if (outsize <= 0)
        goto error;
    if (pusedDefaultChar && *pusedDefaultChar) {
        ret = -2;
        goto done;
    }
    ret = 0;

done:
    PyMem_Free(p);
    return ret;

error:
    if (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) {
        ret = -2;
        goto done;
    }
    PyErr_SetFromWindowsErr(0);
    goto done;
}


/*
 * Encode a Unicode string to a Windows code page into a byte string using an
 * error handler.
 *
 * Returns consumed characters if succeed, or raise an OSError and returns
 * -1 on other error.
 */
static int
encode_code_page_errors(UINT code_page, PyBytesWriter **writer,
                        PyObject *unicode, Py_ssize_t unicode_offset,
                        Py_ssize_t insize, const char* errors)
{
    const DWORD flags = encode_code_page_flags(code_page, errors);
    Py_ssize_t pos = unicode_offset;
    Py_ssize_t endin = unicode_offset + insize;
    /* Ideally, we should get reason from FormatMessage. This is the Windows
       2000 English version of the message. */
    const char *reason = "invalid character";
    /* 4=maximum length of a UTF-8 sequence */
    char buffer[4];
    BOOL usedDefaultChar = FALSE, *pusedDefaultChar;
    Py_ssize_t outsize;
    char *out;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    PyObject *encoding_obj = NULL;
    const char *encoding;
    Py_ssize_t newpos;
    PyObject *rep;
    int ret = -1;

    assert(insize > 0);

    encoding = code_page_name(code_page, &encoding_obj);
    if (encoding == NULL)
        return -1;

    if (errors == NULL || strcmp(errors, "strict") == 0) {
        /* The last error was ERROR_NO_UNICODE_TRANSLATION,
           then we raise a UnicodeEncodeError. */
        make_encode_exception(&exc, encoding, unicode, 0, 0, reason);
        if (exc != NULL) {
            PyCodec_StrictErrors(exc);
            Py_DECREF(exc);
        }
        Py_XDECREF(encoding_obj);
        return -1;
    }

    if (code_page != CP_UTF8 && code_page != CP_UTF7)
        pusedDefaultChar = &usedDefaultChar;
    else
        pusedDefaultChar = NULL;

    if (Py_ARRAY_LENGTH(buffer) > PY_SSIZE_T_MAX / insize) {
        PyErr_NoMemory();
        goto error;
    }
    outsize = insize * Py_ARRAY_LENGTH(buffer);

    if (*writer == NULL) {
        /* Create string object */
        *writer = PyBytesWriter_Create(outsize);
        if (*writer == NULL) {
            goto error;
        }
        out = PyBytesWriter_GetData(*writer);
    }
    else {
        /* Extend string object */
        Py_ssize_t n = PyBytesWriter_GetSize(*writer);
        if (PyBytesWriter_Grow(*writer, outsize) < 0) {
            goto error;
        }
        out = (char*)PyBytesWriter_GetData(*writer) + n;
    }

    /* Encode the string character per character */
    while (pos < endin)
    {
        Py_UCS4 ch = PyUnicode_READ_CHAR(unicode, pos);
        wchar_t chars[2];
        int charsize;
        if (ch < 0x10000) {
            chars[0] = (wchar_t)ch;
            charsize = 1;
        }
        else {
            chars[0] = Py_UNICODE_HIGH_SURROGATE(ch);
            chars[1] = Py_UNICODE_LOW_SURROGATE(ch);
            charsize = 2;
        }

        outsize = WideCharToMultiByte(code_page, flags,
                                      chars, charsize,
                                      buffer, Py_ARRAY_LENGTH(buffer),
                                      NULL, pusedDefaultChar);
        if (outsize > 0) {
            if (pusedDefaultChar == NULL || !(*pusedDefaultChar))
            {
                pos++;
                memcpy(out, buffer, outsize);
                out += outsize;
                continue;
            }
        }
        else if (GetLastError() != ERROR_NO_UNICODE_TRANSLATION) {
            PyErr_SetFromWindowsErr(0);
            goto error;
        }

        rep = unicode_encode_call_errorhandler(
                  errors, &errorHandler, encoding, reason,
                  unicode, &exc,
                  pos, pos + 1, &newpos);
        if (rep == NULL)
            goto error;

        Py_ssize_t morebytes = pos - newpos;
        if (PyBytes_Check(rep)) {
            outsize = PyBytes_GET_SIZE(rep);
            morebytes += outsize;
            if (morebytes > 0) {
                out = PyBytesWriter_GrowAndUpdatePointer(*writer, morebytes, out);
                if (out == NULL) {
                    Py_DECREF(rep);
                    goto error;
                }
            }
            memcpy(out, PyBytes_AS_STRING(rep), outsize);
            out += outsize;
        }
        else {
            Py_ssize_t i;
            int kind;
            const void *data;

            outsize = PyUnicode_GET_LENGTH(rep);
            morebytes += outsize;
            if (morebytes > 0) {
                out = PyBytesWriter_GrowAndUpdatePointer(*writer, morebytes, out);
                if (out == NULL) {
                    Py_DECREF(rep);
                    goto error;
                }
            }
            kind = PyUnicode_KIND(rep);
            data = PyUnicode_DATA(rep);
            for (i=0; i < outsize; i++) {
                Py_UCS4 ch = PyUnicode_READ(kind, data, i);
                if (ch > 127) {
                    raise_encode_exception(&exc,
                        encoding, unicode,
                        pos, pos + 1,
                        "unable to encode error handler result to ASCII");
                    Py_DECREF(rep);
                    goto error;
                }
                *out = (unsigned char)ch;
                out++;
            }
        }
        pos = newpos;
        Py_DECREF(rep);
    }
    /* write a NUL byte */
    *out = 0;
    outsize = out - (char*)PyBytesWriter_GetData(*writer);
    assert(outsize <= PyBytesWriter_GetSize(*writer));
    if (PyBytesWriter_Resize(*writer, outsize) < 0) {
        goto error;
    }
    ret = 0;

error:
    Py_XDECREF(encoding_obj);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return ret;
}


PyObject *
PyUnicode_EncodeCodePage(int code_page,
                         PyObject *unicode,
                         const char *errors)
{
    Py_ssize_t len;
    PyBytesWriter *writer = NULL;
    Py_ssize_t offset;
    int chunk_len, ret, done;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    len = PyUnicode_GET_LENGTH(unicode);

    if (code_page < 0) {
        PyErr_SetString(PyExc_ValueError, "invalid code page number");
        return NULL;
    }

    if (len == 0)
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);

    offset = 0;
    do
    {
#ifdef NEED_RETRY
        if (len > DECODING_CHUNK_SIZE) {
            chunk_len = DECODING_CHUNK_SIZE;
            done = 0;
        }
        else
#endif
        {
            chunk_len = (int)len;
            done = 1;
        }

        ret = encode_code_page_strict(code_page, &writer,
                                      unicode, offset, chunk_len,
                                      errors);
        if (ret == -2)
            ret = encode_code_page_errors(code_page, &writer,
                                          unicode, offset,
                                          chunk_len, errors);
        if (ret < 0) {
            PyBytesWriter_Discard(writer);
            return NULL;
        }

        offset += chunk_len;
        len -= chunk_len;
    } while (!done);

    return PyBytesWriter_Finish(writer);
}


PyObject *
PyUnicode_AsMBCSString(PyObject *unicode)
{
    return PyUnicode_EncodeCodePage(CP_ACP, unicode, NULL);
}

#undef NEED_RETRY

#endif /* MS_WINDOWS */


/* --- Character Mapping Codec -------------------------------------------- */

static int
charmap_decode_string(const char *s,
                      Py_ssize_t size,
                      PyObject *mapping,
                      const char *errors,
                      _PyUnicodeWriter *writer)
{
    const char *starts = s;
    const char *e;
    Py_ssize_t startinpos, endinpos;
    PyObject *errorHandler = NULL, *exc = NULL;
    Py_ssize_t maplen;
    int mapkind;
    const void *mapdata;
    Py_UCS4 x;
    unsigned char ch;

    maplen = PyUnicode_GET_LENGTH(mapping);
    mapdata = PyUnicode_DATA(mapping);
    mapkind = PyUnicode_KIND(mapping);

    e = s + size;

    if (mapkind == PyUnicode_1BYTE_KIND && maplen >= 256) {
        /* fast-path for cp037, cp500 and iso8859_1 encodings. iso8859_1
         * is disabled in encoding aliases, latin1 is preferred because
         * its implementation is faster. */
        const Py_UCS1 *mapdata_ucs1 = (const Py_UCS1 *)mapdata;
        Py_UCS1 *outdata = (Py_UCS1 *)writer->data;
        Py_UCS4 maxchar = writer->maxchar;

        assert (writer->kind == PyUnicode_1BYTE_KIND);
        while (s < e) {
            ch = *s;
            x = mapdata_ucs1[ch];
            if (x > maxchar) {
                if (_PyUnicodeWriter_Prepare(writer, 1, 0xff) == -1)
                    goto onError;
                maxchar = writer->maxchar;
                outdata = (Py_UCS1 *)writer->data;
            }
            outdata[writer->pos] = x;
            writer->pos++;
            ++s;
        }
        return 0;
    }

    while (s < e) {
        if (mapkind == PyUnicode_2BYTE_KIND && maplen >= 256) {
            int outkind = writer->kind;
            const Py_UCS2 *mapdata_ucs2 = (const Py_UCS2 *)mapdata;
            if (outkind == PyUnicode_1BYTE_KIND) {
                Py_UCS1 *outdata = (Py_UCS1 *)writer->data;
                Py_UCS4 maxchar = writer->maxchar;
                while (s < e) {
                    ch = *s;
                    x = mapdata_ucs2[ch];
                    if (x > maxchar)
                        goto Error;
                    outdata[writer->pos] = x;
                    writer->pos++;
                    ++s;
                }
                break;
            }
            else if (outkind == PyUnicode_2BYTE_KIND) {
                Py_UCS2 *outdata = (Py_UCS2 *)writer->data;
                while (s < e) {
                    ch = *s;
                    x = mapdata_ucs2[ch];
                    if (x == 0xFFFE)
                        goto Error;
                    outdata[writer->pos] = x;
                    writer->pos++;
                    ++s;
                }
                break;
            }
        }
        ch = *s;

        if (ch < maplen)
            x = PyUnicode_READ(mapkind, mapdata, ch);
        else
            x = 0xfffe; /* invalid value */
Error:
        if (x == 0xfffe)
        {
            /* undefined mapping */
            startinpos = s-starts;
            endinpos = startinpos+1;
            if (unicode_decode_call_errorhandler_writer(
                    errors, &errorHandler,
                    "charmap", "character maps to <undefined>",
                    &starts, &e, &startinpos, &endinpos, &exc, &s,
                    writer)) {
                goto onError;
            }
            continue;
        }

        if (_PyUnicodeWriter_WriteCharInline(writer, x) < 0)
            goto onError;
        ++s;
    }
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return 0;

onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return -1;
}

static int
charmap_decode_mapping(const char *s,
                       Py_ssize_t size,
                       PyObject *mapping,
                       const char *errors,
                       _PyUnicodeWriter *writer)
{
    const char *starts = s;
    const char *e;
    Py_ssize_t startinpos, endinpos;
    PyObject *errorHandler = NULL, *exc = NULL;
    unsigned char ch;
    PyObject *key, *item = NULL;

    e = s + size;

    while (s < e) {
        ch = *s;

        /* Get mapping (char ordinal -> integer, Unicode char or None) */
        key = PyLong_FromLong((long)ch);
        if (key == NULL)
            goto onError;

        int rc = PyMapping_GetOptionalItem(mapping, key, &item);
        Py_DECREF(key);
        if (rc == 0) {
            /* No mapping found means: mapping is undefined. */
            goto Undefined;
        }
        if (item == NULL) {
            if (PyErr_ExceptionMatches(PyExc_LookupError)) {
                /* No mapping found means: mapping is undefined. */
                PyErr_Clear();
                goto Undefined;
            } else
                goto onError;
        }

        /* Apply mapping */
        if (item == Py_None)
            goto Undefined;
        if (PyLong_Check(item)) {
            long value = PyLong_AsLong(item);
            if (value == 0xFFFE)
                goto Undefined;
            if (value < 0 || value > MAX_UNICODE) {
                PyErr_Format(PyExc_TypeError,
                             "character mapping must be in range(0x%x)",
                             (unsigned long)MAX_UNICODE + 1);
                goto onError;
            }

            if (_PyUnicodeWriter_WriteCharInline(writer, value) < 0)
                goto onError;
        }
        else if (PyUnicode_Check(item)) {
            if (PyUnicode_GET_LENGTH(item) == 1) {
                Py_UCS4 value = PyUnicode_READ_CHAR(item, 0);
                if (value == 0xFFFE)
                    goto Undefined;
                if (_PyUnicodeWriter_WriteCharInline(writer, value) < 0)
                    goto onError;
            }
            else {
                writer->overallocate = 1;
                if (_PyUnicodeWriter_WriteStr(writer, item) == -1)
                    goto onError;
            }
        }
        else {
            /* wrong return value */
            PyErr_SetString(PyExc_TypeError,
                            "character mapping must return integer, None or str");
            goto onError;
        }
        Py_CLEAR(item);
        ++s;
        continue;

Undefined:
        /* undefined mapping */
        Py_CLEAR(item);
        startinpos = s-starts;
        endinpos = startinpos+1;
        if (unicode_decode_call_errorhandler_writer(
                errors, &errorHandler,
                "charmap", "character maps to <undefined>",
                &starts, &e, &startinpos, &endinpos, &exc, &s,
                writer)) {
            goto onError;
        }
    }
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return 0;

onError:
    Py_XDECREF(item);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return -1;
}

PyObject *
PyUnicode_DecodeCharmap(const char *s,
                        Py_ssize_t size,
                        PyObject *mapping,
                        const char *errors)
{
    _PyUnicodeWriter writer;

    /* Default to Latin-1 */
    if (mapping == NULL)
        return PyUnicode_DecodeLatin1(s, size, errors);

    if (size == 0)
        _Py_RETURN_UNICODE_EMPTY();
    _PyUnicodeWriter_Init(&writer);
    writer.min_length = size;
    if (_PyUnicodeWriter_Prepare(&writer, writer.min_length, 127) == -1)
        goto onError;

    if (PyUnicode_CheckExact(mapping)) {
        if (charmap_decode_string(s, size, mapping, errors, &writer) < 0)
            goto onError;
    }
    else {
        if (charmap_decode_mapping(s, size, mapping, errors, &writer) < 0)
            goto onError;
    }
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    _PyUnicodeWriter_Dealloc(&writer);
    return NULL;
}

/* Charmap encoding: the lookup table */

/*[clinic input]
class EncodingMap "struct encoding_map *" "&_Py_EncodingMapType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=14e46bbb6c522d22]*/

struct encoding_map {
    PyObject_HEAD
    unsigned char level1[32];
    int count2, count3;
    unsigned char level23[1];
};

/*[clinic input]
EncodingMap.size

Return the size (in bytes) of this object.
[clinic start generated code]*/

static PyObject *
EncodingMap_size_impl(struct encoding_map *self)
/*[clinic end generated code: output=c4c969e4c99342a4 input=004ff13f26bb5366]*/
{
    return PyLong_FromLong((sizeof(*self) - 1) + 16*self->count2 +
                           128*self->count3);
}

static PyMethodDef encoding_map_methods[] = {
    ENCODINGMAP_SIZE_METHODDEF
    {NULL, NULL}
};

PyTypeObject _Py_EncodingMapType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "EncodingMap",
    .tp_basicsize = sizeof(struct encoding_map),
    /* methods */
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = encoding_map_methods,
};

PyObject*
PyUnicode_BuildEncodingMap(PyObject* string)
{
    PyObject *result;
    struct encoding_map *mresult;
    int i;
    int need_dict = 0;
    unsigned char level1[32];
    unsigned char level2[512];
    unsigned char *mlevel1, *mlevel2, *mlevel3;
    int count2 = 0, count3 = 0;
    int kind;
    const void *data;
    int length;
    Py_UCS4 ch;

    if (!PyUnicode_Check(string) || !PyUnicode_GET_LENGTH(string)) {
        PyErr_BadArgument();
        return NULL;
    }
    kind = PyUnicode_KIND(string);
    data = PyUnicode_DATA(string);
    length = (int)Py_MIN(PyUnicode_GET_LENGTH(string), 256);
    memset(level1, 0xFF, sizeof level1);
    memset(level2, 0xFF, sizeof level2);

    /* If there isn't a one-to-one mapping of NULL to \0,
       or if there are non-BMP characters, we need to use
       a mapping dictionary. */
    if (PyUnicode_READ(kind, data, 0) != 0)
        need_dict = 1;
    for (i = 1; i < length; i++) {
        int l1, l2;
        ch = PyUnicode_READ(kind, data, i);
        if (ch == 0 || ch > 0xFFFF) {
            need_dict = 1;
            break;
        }
        if (ch == 0xFFFE)
            /* unmapped character */
            continue;
        l1 = ch >> 11;
        l2 = ch >> 7;
        if (level1[l1] == 0xFF)
            level1[l1] = count2++;
        if (level2[l2] == 0xFF)
            level2[l2] = count3++;
    }

    if (count2 >= 0xFF || count3 >= 0xFF)
        need_dict = 1;

    if (need_dict) {
        PyObject *result = PyDict_New();
        if (!result)
            return NULL;
        for (i = 0; i < length; i++) {
            Py_UCS4 c = PyUnicode_READ(kind, data, i);
            PyObject *key = PyLong_FromLong(c);
            if (key == NULL) {
                Py_DECREF(result);
                return NULL;
            }
            PyObject *value = PyLong_FromLong(i);
            if (value == NULL) {
                Py_DECREF(key);
                Py_DECREF(result);
                return NULL;
            }
            int rc = PyDict_SetItem(result, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            if (rc < 0) {
                Py_DECREF(result);
                return NULL;
            }
        }
        return result;
    }

    /* Create a three-level trie */
    result = PyObject_Malloc(sizeof(struct encoding_map) +
                             16*count2 + 128*count3 - 1);
    if (!result) {
        return PyErr_NoMemory();
    }

    _PyObject_Init(result, &_Py_EncodingMapType);
    mresult = (struct encoding_map*)result;
    mresult->count2 = count2;
    mresult->count3 = count3;
    mlevel1 = mresult->level1;
    mlevel2 = mresult->level23;
    mlevel3 = mresult->level23 + 16*count2;
    memcpy(mlevel1, level1, 32);
    memset(mlevel2, 0xFF, 16*count2);
    memset(mlevel3, 0, 128*count3);
    count3 = 0;
    for (i = 1; i < length; i++) {
        int o1, o2, o3, i2, i3;
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch == 0xFFFE)
            /* unmapped character */
            continue;
        o1 = ch>>11;
        o2 = (ch>>7) & 0xF;
        i2 = 16*mlevel1[o1] + o2;
        if (mlevel2[i2] == 0xFF)
            mlevel2[i2] = count3++;
        o3 = ch & 0x7F;
        i3 = 128*mlevel2[i2] + o3;
        mlevel3[i3] = i;
    }
    return result;
}

static int
encoding_map_lookup(Py_UCS4 c, PyObject *mapping)
{
    struct encoding_map *map = (struct encoding_map*)mapping;
    int l1 = c>>11;
    int l2 = (c>>7) & 0xF;
    int l3 = c & 0x7F;
    int i;

    if (c > 0xFFFF)
        return -1;
    if (c == 0)
        return 0;
    /* level 1*/
    i = map->level1[l1];
    if (i == 0xFF) {
        return -1;
    }
    /* level 2*/
    i = map->level23[16*i+l2];
    if (i == 0xFF) {
        return -1;
    }
    /* level 3 */
    i = map->level23[16*map->count2 + 128*i + l3];
    if (i == 0) {
        return -1;
    }
    return i;
}

/* Lookup the character in the mapping.
   On success, return PyLong, PyBytes or None (if the character can't be found).
   If the result is PyLong, put its value in replace.
   On error, return NULL.
   */
static PyObject *
charmapencode_lookup(Py_UCS4 c, PyObject *mapping, unsigned char *replace)
{
    PyObject *w = PyLong_FromLong((long)c);
    PyObject *x;

    if (w == NULL)
        return NULL;
    int rc = PyMapping_GetOptionalItem(mapping, w, &x);
    Py_DECREF(w);
    if (rc == 0) {
        /* No mapping found means: mapping is undefined. */
        Py_RETURN_NONE;
    }
    if (x == NULL) {
        if (PyErr_ExceptionMatches(PyExc_LookupError)) {
            /* No mapping found means: mapping is undefined. */
            PyErr_Clear();
            Py_RETURN_NONE;
        } else
            return NULL;
    }
    else if (x == Py_None)
        return x;
    else if (PyLong_Check(x)) {
        long value = PyLong_AsLong(x);
        if (value < 0 || value > 255) {
            PyErr_SetString(PyExc_TypeError,
                            "character mapping must be in range(256)");
            Py_DECREF(x);
            return NULL;
        }
        *replace = (unsigned char)value;
        return x;
    }
    else if (PyBytes_Check(x))
        return x;
    else {
        /* wrong return value */
        PyErr_Format(PyExc_TypeError,
                     "character mapping must return integer, bytes or None, not %.400s",
                     Py_TYPE(x)->tp_name);
        Py_DECREF(x);
        return NULL;
    }
}

static int
charmapencode_resize(PyBytesWriter *writer, Py_ssize_t *outpos, Py_ssize_t requiredsize)
{
    Py_ssize_t outsize = PyBytesWriter_GetSize(writer);
    /* exponentially overallocate to minimize reallocations */
    if (requiredsize < 2 * outsize)
        requiredsize = 2 * outsize;
    return PyBytesWriter_Resize(writer, requiredsize);
}

typedef enum charmapencode_result {
    enc_SUCCESS, enc_FAILED, enc_EXCEPTION
} charmapencode_result;
/* lookup the character, put the result in the output string and adjust
   various state variables. Resize the output bytes object if not enough
   space is available. Return a new reference to the object that
   was put in the output buffer, or Py_None, if the mapping was undefined
   (in which case no character was written) or NULL, if a
   reallocation error occurred. The caller must decref the result */
static charmapencode_result
charmapencode_output(Py_UCS4 c, PyObject *mapping,
                     PyBytesWriter *writer, Py_ssize_t *outpos)
{
    PyObject *rep;
    unsigned char replace;
    char *outstart;
    Py_ssize_t outsize = _PyBytesWriter_GetSize(writer);

    if (Py_IS_TYPE(mapping, &_Py_EncodingMapType)) {
        int res = encoding_map_lookup(c, mapping);
        Py_ssize_t requiredsize = *outpos+1;
        if (res == -1) {
            return enc_FAILED;
        }

        if (outsize<requiredsize) {
            if (charmapencode_resize(writer, outpos, requiredsize)) {
                return enc_EXCEPTION;
            }
        }
        outstart = _PyBytesWriter_GetData(writer);
        outstart[(*outpos)++] = (char)res;
        return enc_SUCCESS;
    }

    rep = charmapencode_lookup(c, mapping, &replace);
    if (rep==NULL)
        return enc_EXCEPTION;
    else if (rep==Py_None) {
        Py_DECREF(rep);
        return enc_FAILED;
    } else {
        if (PyLong_Check(rep)) {
            Py_ssize_t requiredsize = *outpos+1;
            if (outsize<requiredsize)
                if (charmapencode_resize(writer, outpos, requiredsize)) {
                    Py_DECREF(rep);
                    return enc_EXCEPTION;
                }
            outstart = _PyBytesWriter_GetData(writer);
            outstart[(*outpos)++] = (char)replace;
        }
        else {
            const char *repchars = PyBytes_AS_STRING(rep);
            Py_ssize_t repsize = PyBytes_GET_SIZE(rep);
            Py_ssize_t requiredsize = *outpos+repsize;
            if (outsize<requiredsize)
                if (charmapencode_resize(writer, outpos, requiredsize)) {
                    Py_DECREF(rep);
                    return enc_EXCEPTION;
                }
            outstart = _PyBytesWriter_GetData(writer);
            memcpy(outstart + *outpos, repchars, repsize);
            *outpos += repsize;
        }
    }
    Py_DECREF(rep);
    return enc_SUCCESS;
}

/* handle an error in _PyUnicode_EncodeCharmap()
   Return 0 on success, -1 on error */
static int
charmap_encoding_error(
    PyObject *unicode, Py_ssize_t *inpos, PyObject *mapping,
    PyObject **exceptionObject,
    _Py_error_handler *error_handler, PyObject **error_handler_obj, const char *errors,
    PyBytesWriter *writer, Py_ssize_t *respos)
{
    PyObject *repunicode = NULL; /* initialize to prevent gcc warning */
    Py_ssize_t size, repsize;
    Py_ssize_t newpos;
    int kind;
    const void *data;
    Py_ssize_t index;
    /* startpos for collecting unencodable chars */
    Py_ssize_t collstartpos = *inpos;
    Py_ssize_t collendpos = *inpos+1;
    Py_ssize_t collpos;
    const char *encoding = "charmap";
    const char *reason = "character maps to <undefined>";
    charmapencode_result x;
    Py_UCS4 ch;
    int val;

    size = PyUnicode_GET_LENGTH(unicode);
    /* find all unencodable characters */
    while (collendpos < size) {
        PyObject *rep;
        unsigned char replace;
        if (Py_IS_TYPE(mapping, &_Py_EncodingMapType)) {
            ch = PyUnicode_READ_CHAR(unicode, collendpos);
            val = encoding_map_lookup(ch, mapping);
            if (val != -1)
                break;
            ++collendpos;
            continue;
        }

        ch = PyUnicode_READ_CHAR(unicode, collendpos);
        rep = charmapencode_lookup(ch, mapping, &replace);
        if (rep==NULL)
            return -1;
        else if (rep!=Py_None) {
            Py_DECREF(rep);
            break;
        }
        Py_DECREF(rep);
        ++collendpos;
    }
    /* cache callback name lookup
     * (if not done yet, i.e. it's the first error) */
    if (*error_handler == _Py_ERROR_UNKNOWN)
        *error_handler = _Py_GetErrorHandler(errors);

    switch (*error_handler) {
    case _Py_ERROR_STRICT:
        raise_encode_exception(exceptionObject, encoding, unicode, collstartpos, collendpos, reason);
        return -1;

    case _Py_ERROR_REPLACE:
        for (collpos = collstartpos; collpos<collendpos; ++collpos) {
            x = charmapencode_output('?', mapping, writer, respos);
            if (x==enc_EXCEPTION) {
                return -1;
            }
            else if (x==enc_FAILED) {
                raise_encode_exception(exceptionObject, encoding, unicode, collstartpos, collendpos, reason);
                return -1;
            }
        }
        _Py_FALLTHROUGH;
    case _Py_ERROR_IGNORE:
        *inpos = collendpos;
        break;

    case _Py_ERROR_XMLCHARREFREPLACE:
        /* generate replacement (temporarily (mis)uses p) */
        for (collpos = collstartpos; collpos < collendpos; ++collpos) {
            char buffer[2+29+1+1];
            char *cp;
            sprintf(buffer, "&#%d;", (int)PyUnicode_READ_CHAR(unicode, collpos));
            for (cp = buffer; *cp; ++cp) {
                x = charmapencode_output(*cp, mapping, writer, respos);
                if (x==enc_EXCEPTION)
                    return -1;
                else if (x==enc_FAILED) {
                    raise_encode_exception(exceptionObject, encoding, unicode, collstartpos, collendpos, reason);
                    return -1;
                }
            }
        }
        *inpos = collendpos;
        break;

    default:
        repunicode = unicode_encode_call_errorhandler(errors, error_handler_obj,
                                                      encoding, reason, unicode, exceptionObject,
                                                      collstartpos, collendpos, &newpos);
        if (repunicode == NULL)
            return -1;
        if (PyBytes_Check(repunicode)) {
            /* Directly copy bytes result to output. */
            Py_ssize_t outsize = PyBytesWriter_GetSize(writer);
            Py_ssize_t requiredsize;
            repsize = PyBytes_Size(repunicode);
            requiredsize = *respos + repsize;
            if (requiredsize > outsize)
                /* Make room for all additional bytes. */
                if (charmapencode_resize(writer, respos, requiredsize)) {
                    Py_DECREF(repunicode);
                    return -1;
                }
            memcpy((char*)PyBytesWriter_GetData(writer) + *respos,
                   PyBytes_AsString(repunicode),  repsize);
            *respos += repsize;
            *inpos = newpos;
            Py_DECREF(repunicode);
            break;
        }
        /* generate replacement  */
        repsize = PyUnicode_GET_LENGTH(repunicode);
        data = PyUnicode_DATA(repunicode);
        kind = PyUnicode_KIND(repunicode);
        for (index = 0; index < repsize; index++) {
            Py_UCS4 repch = PyUnicode_READ(kind, data, index);
            x = charmapencode_output(repch, mapping, writer, respos);
            if (x==enc_EXCEPTION) {
                Py_DECREF(repunicode);
                return -1;
            }
            else if (x==enc_FAILED) {
                Py_DECREF(repunicode);
                raise_encode_exception(exceptionObject, encoding, unicode, collstartpos, collendpos, reason);
                return -1;
            }
        }
        *inpos = newpos;
        Py_DECREF(repunicode);
    }
    return 0;
}

PyObject *
_PyUnicode_EncodeCharmap(PyObject *unicode,
                         PyObject *mapping,
                         const char *errors)
{
    /* Default to Latin-1 */
    if (mapping == NULL) {
        return unicode_encode_ucs1(unicode, errors, 256);
    }

    Py_ssize_t size = PyUnicode_GET_LENGTH(unicode);
    if (size == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    const void *data = PyUnicode_DATA(unicode);
    int kind = PyUnicode_KIND(unicode);

    PyObject *error_handler_obj = NULL;
    PyObject *exc = NULL;

    /* output object */
    PyBytesWriter *writer;
    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    writer = PyBytesWriter_Create(size);
    if (writer == NULL) {
        goto onError;
    }

    /* current input position */
    Py_ssize_t inpos = 0;
    /* current output position */
    Py_ssize_t respos = 0;
    _Py_error_handler error_handler = _Py_ERROR_UNKNOWN;

    if (Py_IS_TYPE(mapping, &_Py_EncodingMapType)) {
        char *outstart = _PyBytesWriter_GetData(writer);
        Py_ssize_t outsize = _PyBytesWriter_GetSize(writer);

        while (inpos<size) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, inpos);

            /* try to encode it */
            int res = encoding_map_lookup(ch, mapping);
            Py_ssize_t requiredsize = respos+1;
            if (res == -1) {
                goto enc_FAILED;
            }

            if (outsize<requiredsize) {
                if (charmapencode_resize(writer, &respos, requiredsize)) {
                    goto onError;
                }
                outstart = _PyBytesWriter_GetData(writer);
                outsize = _PyBytesWriter_GetSize(writer);
            }
            outstart[respos++] = (char)res;

            /* done with this character => adjust input position */
            ++inpos;
            continue;

enc_FAILED:
            if (charmap_encoding_error(unicode, &inpos, mapping,
                                       &exc,
                                       &error_handler, &error_handler_obj, errors,
                                       writer, &respos)) {
                goto onError;
            }
            outstart = _PyBytesWriter_GetData(writer);
            outsize = _PyBytesWriter_GetSize(writer);
        }
    }
    else {
        while (inpos<size) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, inpos);
            /* try to encode it */
            charmapencode_result x = charmapencode_output(ch, mapping, writer, &respos);
            if (x==enc_EXCEPTION) { /* error */
                goto onError;
            }
            if (x==enc_FAILED) { /* unencodable character */
                if (charmap_encoding_error(unicode, &inpos, mapping,
                                           &exc,
                                           &error_handler, &error_handler_obj, errors,
                                           writer, &respos)) {
                    goto onError;
                }
            }
            else {
                /* done with this character => adjust input position */
                ++inpos;
            }
        }
    }

    Py_XDECREF(exc);
    Py_XDECREF(error_handler_obj);

    /* Resize if we allocated too much */
    return PyBytesWriter_FinishWithSize(writer, respos);

  onError:
    PyBytesWriter_Discard(writer);
    Py_XDECREF(exc);
    Py_XDECREF(error_handler_obj);
    return NULL;
}

PyObject *
PyUnicode_AsCharmapString(PyObject *unicode,
                          PyObject *mapping)
{
    if (!PyUnicode_Check(unicode) || mapping == NULL) {
        PyErr_BadArgument();
        return NULL;
    }
    return _PyUnicode_EncodeCharmap(unicode, mapping, NULL);
}

/* create or adjust a UnicodeTranslateError */
static void
make_translate_exception(PyObject **exceptionObject,
                         PyObject *unicode,
                         Py_ssize_t startpos, Py_ssize_t endpos,
                         const char *reason)
{
    if (*exceptionObject == NULL) {
        *exceptionObject = _PyUnicodeTranslateError_Create(
            unicode, startpos, endpos, reason);
    }
    else {
        if (PyUnicodeTranslateError_SetStart(*exceptionObject, startpos))
            goto onError;
        if (PyUnicodeTranslateError_SetEnd(*exceptionObject, endpos))
            goto onError;
        if (PyUnicodeTranslateError_SetReason(*exceptionObject, reason))
            goto onError;
        return;
      onError:
        Py_CLEAR(*exceptionObject);
    }
}

/* error handling callback helper:
   build arguments, call the callback and check the arguments,
   put the result into newpos and return the replacement string, which
   has to be freed by the caller */
static PyObject *
unicode_translate_call_errorhandler(const char *errors,
                                    PyObject **errorHandler,
                                    const char *reason,
                                    PyObject *unicode, PyObject **exceptionObject,
                                    Py_ssize_t startpos, Py_ssize_t endpos,
                                    Py_ssize_t *newpos)
{
    static const char *argparse = "Un;translating error handler must return (str, int) tuple";

    Py_ssize_t i_newpos;
    PyObject *restuple;
    PyObject *resunicode;

    if (*errorHandler == NULL) {
        *errorHandler = PyCodec_LookupError(errors);
        if (*errorHandler == NULL)
            return NULL;
    }

    make_translate_exception(exceptionObject,
                             unicode, startpos, endpos, reason);
    if (*exceptionObject == NULL)
        return NULL;

    restuple = PyObject_CallOneArg(*errorHandler, *exceptionObject);
    if (restuple == NULL)
        return NULL;
    if (!PyTuple_Check(restuple)) {
        PyErr_SetString(PyExc_TypeError, &argparse[3]);
        Py_DECREF(restuple);
        return NULL;
    }
    if (!PyArg_ParseTuple(restuple, argparse,
                          &resunicode, &i_newpos)) {
        Py_DECREF(restuple);
        return NULL;
    }
    if (i_newpos<0)
        *newpos = PyUnicode_GET_LENGTH(unicode)+i_newpos;
    else
        *newpos = i_newpos;
    if (*newpos<0 || *newpos>PyUnicode_GET_LENGTH(unicode)) {
        PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", *newpos);
        Py_DECREF(restuple);
        return NULL;
    }
    Py_INCREF(resunicode);
    Py_DECREF(restuple);
    return resunicode;
}

/* Lookup the character ch in the mapping and put the result in result,
   which must be decrefed by the caller.
   The result can be PyLong, PyUnicode, None or NULL.
   If the result is PyLong, put its value in replace.
   Return 0 on success, -1 on error */
static int
charmaptranslate_lookup(Py_UCS4 c, PyObject *mapping, PyObject **result, Py_UCS4 *replace)
{
    PyObject *w = PyLong_FromLong((long)c);
    PyObject *x;

    if (w == NULL)
        return -1;
    int rc = PyMapping_GetOptionalItem(mapping, w, &x);
    Py_DECREF(w);
    if (rc == 0) {
        /* No mapping found means: use 1:1 mapping. */
        *result = NULL;
        return 0;
    }
    if (x == NULL) {
        if (PyErr_ExceptionMatches(PyExc_LookupError)) {
            /* No mapping found means: use 1:1 mapping. */
            PyErr_Clear();
            *result = NULL;
            return 0;
        } else
            return -1;
    }
    else if (x == Py_None) {
        *result = x;
        return 0;
    }
    else if (PyLong_Check(x)) {
        long value = PyLong_AsLong(x);
        if (value < 0 || value > MAX_UNICODE) {
            PyErr_Format(PyExc_ValueError,
                         "character mapping must be in range(0x%x)",
                         MAX_UNICODE+1);
            Py_DECREF(x);
            return -1;
        }
        *result = x;
        *replace = (Py_UCS4)value;
        return 0;
    }
    else if (PyUnicode_Check(x)) {
        *result = x;
        return 0;
    }
    else {
        /* wrong return value */
        PyErr_SetString(PyExc_TypeError,
                        "character mapping must return integer, None or str");
        Py_DECREF(x);
        return -1;
    }
}

/* lookup the character, write the result into the writer.
   Return 1 if the result was written into the writer, return 0 if the mapping
   was undefined, raise an exception return -1 on error. */
static int
charmaptranslate_output(Py_UCS4 ch, PyObject *mapping,
                        _PyUnicodeWriter *writer)
{
    PyObject *item;
    Py_UCS4 replace;

    if (charmaptranslate_lookup(ch, mapping, &item, &replace))
        return -1;

    if (item == NULL) {
        /* not found => default to 1:1 mapping */
        if (_PyUnicodeWriter_WriteCharInline(writer, ch) < 0) {
            return -1;
        }
        return 1;
    }

    if (item == Py_None) {
        Py_DECREF(item);
        return 0;
    }

    if (PyLong_Check(item)) {
        if (_PyUnicodeWriter_WriteCharInline(writer, replace) < 0) {
            Py_DECREF(item);
            return -1;
        }
        Py_DECREF(item);
        return 1;
    }

    if (!PyUnicode_Check(item)) {
        Py_DECREF(item);
        return -1;
    }

    if (_PyUnicodeWriter_WriteStr(writer, item) < 0) {
        Py_DECREF(item);
        return -1;
    }

    Py_DECREF(item);
    return 1;
}

static int
unicode_fast_translate_lookup(PyObject *mapping, Py_UCS1 ch,
                              Py_UCS1 *translate)
{
    PyObject *item = NULL;
    Py_UCS4 replace;
    int ret = 0;

    if (charmaptranslate_lookup(ch, mapping, &item, &replace)) {
        return -1;
    }

    if (item == Py_None) {
        /* deletion */
        translate[ch] = 0xfe;
    }
    else if (item == NULL) {
        /* not found => default to 1:1 mapping */
        translate[ch] = ch;
        return 1;
    }
    else if (PyLong_Check(item)) {
        if (replace > 127) {
            /* invalid character or character outside ASCII:
               skip the fast translate */
            goto exit;
        }
        translate[ch] = (Py_UCS1)replace;
    }
    else if (PyUnicode_Check(item)) {
        if (PyUnicode_GET_LENGTH(item) != 1)
            goto exit;

        replace = PyUnicode_READ_CHAR(item, 0);
        if (replace > 127)
            goto exit;
        translate[ch] = (Py_UCS1)replace;
    }
    else {
        /* not None, NULL, long or unicode */
        goto exit;
    }
    ret = 1;

  exit:
    Py_DECREF(item);
    return ret;
}

/* Fast path for ascii => ascii translation. Return 1 if the whole string
   was translated into writer, return 0 if the input string was partially
   translated into writer, raise an exception and return -1 on error. */
static int
unicode_fast_translate(PyObject *input, PyObject *mapping,
                       _PyUnicodeWriter *writer, int ignore,
                       Py_ssize_t *input_pos)
{
    Py_UCS1 ascii_table[128], ch, ch2;
    Py_ssize_t len;
    const Py_UCS1 *in, *end;
    Py_UCS1 *out;
    int res = 0;

    len = PyUnicode_GET_LENGTH(input);

    memset(ascii_table, 0xff, 128);

    in = PyUnicode_1BYTE_DATA(input);
    end = in + len;

    assert(PyUnicode_IS_ASCII(writer->buffer));
    assert(PyUnicode_GET_LENGTH(writer->buffer) == len);
    out = PyUnicode_1BYTE_DATA(writer->buffer);

    for (; in < end; in++) {
        ch = *in;
        ch2 = ascii_table[ch];
        if (ch2 == 0xff) {
            int translate = unicode_fast_translate_lookup(mapping, ch,
                                                          ascii_table);
            if (translate < 0)
                return -1;
            if (translate == 0)
                goto exit;
            ch2 = ascii_table[ch];
        }
        if (ch2 == 0xfe) {
            if (ignore)
                continue;
            goto exit;
        }
        assert(ch2 < 128);
        *out = ch2;
        out++;
    }
    res = 1;

exit:
    writer->pos = out - PyUnicode_1BYTE_DATA(writer->buffer);
    *input_pos = in - PyUnicode_1BYTE_DATA(input);
    return res;
}


PyObject *
_PyUnicode_TranslateCharmap(PyObject *input,
                            PyObject *mapping,
                            const char *errors)
{
    /* input object */
    const void *data;
    Py_ssize_t size, i;
    int kind;
    /* output buffer */
    _PyUnicodeWriter writer;
    /* error handler */
    const char *reason = "character maps to <undefined>";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    int ignore;
    int res;

    if (mapping == NULL) {
        PyErr_BadArgument();
        return NULL;
    }

    data = PyUnicode_DATA(input);
    kind = PyUnicode_KIND(input);
    size = PyUnicode_GET_LENGTH(input);

    if (size == 0)
        return PyUnicode_FromObject(input);

    /* allocate enough for a simple 1:1 translation without
       replacements, if we need more, we'll resize */
    _PyUnicodeWriter_Init(&writer);
    if (_PyUnicodeWriter_Prepare(&writer, size, 127) == -1)
        goto onError;

    ignore = (errors != NULL && strcmp(errors, "ignore") == 0);

    if (PyUnicode_IS_ASCII(input)) {
        res = unicode_fast_translate(input, mapping, &writer, ignore, &i);
        if (res < 0) {
            _PyUnicodeWriter_Dealloc(&writer);
            return NULL;
        }
        if (res == 1)
            return _PyUnicodeWriter_Finish(&writer);
    }
    else {
        i = 0;
    }

    while (i<size) {
        /* try to encode it */
        int translate;
        PyObject *repunicode = NULL; /* initialize to prevent gcc warning */
        Py_ssize_t newpos;
        /* startpos for collecting untranslatable chars */
        Py_ssize_t collstart;
        Py_ssize_t collend;
        Py_UCS4 ch;

        ch = PyUnicode_READ(kind, data, i);
        translate = charmaptranslate_output(ch, mapping, &writer);
        if (translate < 0)
            goto onError;

        if (translate != 0) {
            /* it worked => adjust input pointer */
            ++i;
            continue;
        }

        /* untranslatable character */
        collstart = i;
        collend = i+1;

        /* find all untranslatable characters */
        while (collend < size) {
            PyObject *x;
            Py_UCS4 replace;
            ch = PyUnicode_READ(kind, data, collend);
            if (charmaptranslate_lookup(ch, mapping, &x, &replace))
                goto onError;
            Py_XDECREF(x);
            if (x != Py_None)
                break;
            ++collend;
        }

        if (ignore) {
            i = collend;
        }
        else {
            repunicode = unicode_translate_call_errorhandler(errors, &errorHandler,
                                                             reason, input, &exc,
                                                             collstart, collend, &newpos);
            if (repunicode == NULL)
                goto onError;
            if (_PyUnicodeWriter_WriteStr(&writer, repunicode) < 0) {
                Py_DECREF(repunicode);
                goto onError;
            }
            Py_DECREF(repunicode);
            i = newpos;
        }
    }
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return _PyUnicodeWriter_Finish(&writer);

  onError:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return NULL;
}

PyObject *
PyUnicode_Translate(PyObject *str,
                    PyObject *mapping,
                    const char *errors)
{
    if (ensure_unicode(str) < 0)
        return NULL;
    return _PyUnicode_TranslateCharmap(str, mapping, errors);
}

PyObject *
_PyUnicode_TransformDecimalAndSpaceToASCII(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (PyUnicode_IS_ASCII(unicode)) {
        /* If the string is already ASCII, just return the same string */
        return Py_NewRef(unicode);
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    PyObject *result = PyUnicode_New(len, 127);
    if (result == NULL) {
        return NULL;
    }

    Py_UCS1 *out = PyUnicode_1BYTE_DATA(result);
    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    Py_ssize_t i;
    for (i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch < 127) {
            out[i] = ch;
        }
        else if (Py_UNICODE_ISSPACE(ch)) {
            out[i] = ' ';
        }
        else {
            int decimal = Py_UNICODE_TODECIMAL(ch);
            if (decimal < 0) {
                out[i] = '?';
                out[i+1] = '\0';
                _PyUnicode_LENGTH(result) = i + 1;
                break;
            }
            out[i] = '0' + decimal;
        }
    }

    assert(_PyUnicode_CheckConsistency(result, 1));
    return result;
}


#ifdef MS_WINDOWS
int
_PyUnicode_EnableLegacyWindowsFSEncoding(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyConfig *config = (PyConfig *)_PyInterpreterState_GetConfig(interp);

    /* Set the filesystem encoding to mbcs/replace (PEP 529) */
    wchar_t *encoding = _PyMem_RawWcsdup(L"mbcs");
    wchar_t *errors = _PyMem_RawWcsdup(L"replace");
    if (encoding == NULL || errors == NULL) {
        PyMem_RawFree(encoding);
        PyMem_RawFree(errors);
        PyErr_NoMemory();
        return -1;
    }

    PyMem_RawFree(config->filesystem_encoding);
    config->filesystem_encoding = encoding;
    PyMem_RawFree(config->filesystem_errors);
    config->filesystem_errors = errors;

    return init_fs_codec(interp);
}
#endif


static int
encode_wstr_utf8(wchar_t *wstr, char **str, const char *name)
{
    int res;
    res = _Py_EncodeUTF8Ex(wstr, str, NULL, NULL, 1, _Py_ERROR_STRICT);
    if (res == -2) {
        PyErr_Format(PyExc_RuntimeError, "cannot encode %s", name);
        return -1;
    }
    if (res < 0) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}


static int
config_get_codec_name(wchar_t **config_encoding)
{
    char *encoding;
    if (encode_wstr_utf8(*config_encoding, &encoding, "stdio_encoding") < 0) {
        return -1;
    }

    PyObject *name_obj = NULL;
    PyObject *codec = _PyCodec_Lookup(encoding);
    PyMem_RawFree(encoding);

    if (!codec)
        goto error;

    name_obj = PyObject_GetAttrString(codec, "name");
    Py_CLEAR(codec);
    if (!name_obj) {
        goto error;
    }

    wchar_t *wname = PyUnicode_AsWideCharString(name_obj, NULL);
    Py_DECREF(name_obj);
    if (wname == NULL) {
        goto error;
    }

    wchar_t *raw_wname = _PyMem_RawWcsdup(wname);
    if (raw_wname == NULL) {
        PyMem_Free(wname);
        PyErr_NoMemory();
        goto error;
    }

    PyMem_RawFree(*config_encoding);
    *config_encoding = raw_wname;

    PyMem_Free(wname);
    return 0;

error:
    Py_XDECREF(codec);
    Py_XDECREF(name_obj);
    return -1;
}


static PyStatus
init_stdio_encoding(PyInterpreterState *interp)
{
    /* Update the stdio encoding to the normalized Python codec name. */
    PyConfig *config = (PyConfig*)_PyInterpreterState_GetConfig(interp);
    if (config_get_codec_name(&config->stdio_encoding) < 0) {
        return _PyStatus_ERR("failed to get the Python codec name "
                             "of the stdio encoding");
    }
    return _PyStatus_OK();
}


static int
init_fs_codec(PyInterpreterState *interp)
{
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);

    _Py_error_handler error_handler;
    error_handler = get_error_handler_wide(config->filesystem_errors);
    if (error_handler == _Py_ERROR_UNKNOWN) {
        PyErr_SetString(PyExc_RuntimeError, "unknown filesystem error handler");
        return -1;
    }

    char *encoding, *errors;
    if (encode_wstr_utf8(config->filesystem_encoding,
                         &encoding,
                         "filesystem_encoding") < 0) {
        return -1;
    }

    if (encode_wstr_utf8(config->filesystem_errors,
                         &errors,
                         "filesystem_errors") < 0) {
        PyMem_RawFree(encoding);
        return -1;
    }

    struct _Py_unicode_fs_codec *fs_codec = &interp->unicode.fs_codec;
    PyMem_RawFree(fs_codec->encoding);
    fs_codec->encoding = encoding;
    /* encoding has been normalized by init_fs_encoding() */
    fs_codec->utf8 = (strcmp(encoding, "utf-8") == 0);
    PyMem_RawFree(fs_codec->errors);
    fs_codec->errors = errors;
    fs_codec->error_handler = error_handler;

#ifdef _Py_FORCE_UTF8_FS_ENCODING
    assert(fs_codec->utf8 == 1);
#endif

    /* At this point, PyUnicode_EncodeFSDefault() and
       PyUnicode_DecodeFSDefault() can now use the Python codec rather than
       the C implementation of the filesystem encoding. */

    /* Set Py_FileSystemDefaultEncoding and Py_FileSystemDefaultEncodeErrors
       global configuration variables. */
    if (_Py_IsMainInterpreter(interp)) {

        if (_Py_SetFileSystemEncoding(fs_codec->encoding,
                                      fs_codec->errors) < 0) {
            PyErr_NoMemory();
            return -1;
        }
    }
    return 0;
}


static PyStatus
init_fs_encoding(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;

    /* Update the filesystem encoding to the normalized Python codec name.
       For example, replace "ANSI_X3.4-1968" (locale encoding) with "ascii"
       (Python codec name). */
    PyConfig *config = (PyConfig*)_PyInterpreterState_GetConfig(interp);
    if (config_get_codec_name(&config->filesystem_encoding) < 0) {
        _Py_DumpPathConfig(tstate);
        return _PyStatus_ERR("failed to get the Python codec "
                             "of the filesystem encoding");
    }

    if (init_fs_codec(interp) < 0) {
        return _PyStatus_ERR("cannot initialize filesystem codec");
    }
    return _PyStatus_OK();
}


PyStatus
_PyUnicode_InitEncodings(PyThreadState *tstate)
{
    PyStatus status = _PyCodec_InitRegistry(tstate->interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    status = init_fs_encoding(tstate);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return init_stdio_encoding(tstate->interp);
}


void
_PyUnicode_FiniEncodings(struct _Py_unicode_fs_codec *fs_codec)
{
    PyMem_RawFree(fs_codec->encoding);
    fs_codec->encoding = NULL;
    fs_codec->utf8 = 0;
    PyMem_RawFree(fs_codec->errors);
    fs_codec->errors = NULL;
    fs_codec->error_handler = _Py_ERROR_UNKNOWN;
}
