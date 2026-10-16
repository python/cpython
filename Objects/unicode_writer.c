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
#include "pycore_freelist.h"      // _Py_FREELIST_FREE()
#include "pycore_long.h"          // _PyLong_FormatWriter()
#include "pycore_unicodeobject.h" // _PyUnicode_Result()


#ifdef MS_WINDOWS
   /* On Windows, overallocate by 50% is the best factor */
#  define OVERALLOCATE_FACTOR 2
#else
   /* On Linux, overallocate by 25% is the best factor */
#  define OVERALLOCATE_FACTOR 4
#endif


/* Compilation of templated routines */

#define STRINGLIB_GET_EMPTY() _PyUnicode_GetEmpty()

#include "stringlib/ucs1lib.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"


void
_PyUnicodeWriter_Init(_PyUnicodeWriter *writer)
{
    memset(writer, 0, sizeof(*writer));

    /* ASCII is the bare minimum */
    writer->min_char = 127;

    /* use a kind value smaller than PyUnicode_1BYTE_KIND so
       _PyUnicodeWriter_PrepareKind() will copy the buffer. */
    assert(writer->kind == 0);
    assert(writer->kind < PyUnicode_1BYTE_KIND);
}


PyUnicodeWriter*
PyUnicodeWriter_Create(Py_ssize_t length)
{
    if (length < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "length must be positive");
        return NULL;
    }

    const size_t size = sizeof(_PyUnicodeWriter);
    PyUnicodeWriter *pub_writer;
    pub_writer = _Py_FREELIST_POP_MEM(unicode_writers);
    if (pub_writer == NULL) {
        pub_writer = (PyUnicodeWriter *)PyMem_Malloc(size);
        if (pub_writer == NULL) {
            return (PyUnicodeWriter *)PyErr_NoMemory();
        }
    }
    _PyUnicodeWriter *writer = (_PyUnicodeWriter *)pub_writer;

    _PyUnicodeWriter_Init(writer);
    // The buffer is created lazily at the first write, except if
    // the read-only optimization is used.
    writer->min_length = length;
    writer->overallocate = 1;

    return pub_writer;
}


void
PyUnicodeWriter_Discard(PyUnicodeWriter *writer)
{
    if (writer == NULL) {
        return;
    }
    _PyUnicodeWriter_Dealloc((_PyUnicodeWriter*)writer);
    _Py_FREELIST_FREE(unicode_writers, writer, PyMem_Free);
}


// Initialize _PyUnicodeWriter with initial buffer
void
_PyUnicodeWriter_InitWithBuffer(_PyUnicodeWriter *writer, PyObject *buffer)
{
    assert(PyUnstable_Object_IsUniquelyReferenced(buffer));

    memset(writer, 0, sizeof(*writer));
    _PyUnicodeWriter_SetBuffer(writer, buffer);
    writer->min_length = writer->size;
    assert(_PyUnicodeWriter_CanWrite(writer));
}


int
_PyUnicodeWriter_PrepareInternal(_PyUnicodeWriter *writer,
                                 Py_ssize_t length, Py_UCS4 maxchar)
{
    assert(length >= 0);
    assert(maxchar <= _Py_MAX_UNICODE);

    // Check that _PyUnicodeWriter_Prepare() or _PyUnicodeWriter_PrepareKind()
    // was used
    assert(maxchar > writer->maxchar
           || (length > (writer->size - writer->pos) && length >= 1));

    if (length > PY_SSIZE_T_MAX - writer->pos) {
        PyErr_NoMemory();
        return -1;
    }
    Py_ssize_t alloc = writer->pos + length;

    maxchar = Py_MAX(maxchar, writer->min_char);

    PyObject *new_buffer;
    if (writer->buffer == NULL) {
        assert(!writer->readonly);

        // Do not overallocate at the first allocation, but use min_length
        if (alloc < writer->min_length) {
            alloc = writer->min_length;
        }

        new_buffer = PyUnicode_New(alloc, maxchar);
        if (new_buffer == NULL) {
            return -1;
        }
    }
    else if (alloc > writer->size) {
        // Do not overallocate at the first allocation, but use min_length
        int overallocate = (writer->overallocate && !writer->readonly);
        if (overallocate
            && alloc <= (PY_SSIZE_T_MAX - alloc / OVERALLOCATE_FACTOR)) {
            /* overallocate to limit the number of realloc() */
            alloc += alloc / OVERALLOCATE_FACTOR;
        }
        if (alloc < writer->min_length) {
            alloc = writer->min_length;
        }

        if (maxchar > writer->maxchar || writer->readonly) {
            /* resize + widen */
            maxchar = Py_MAX(maxchar, writer->maxchar);
            new_buffer = PyUnicode_New(alloc, maxchar);
            if (new_buffer == NULL) {
                return -1;
            }
            _PyUnicode_FastCopyCharacters(new_buffer, 0,
                                          writer->buffer, 0, writer->pos);
        }
        else {
            new_buffer = _PyUnicode_ResizeCompact(writer->buffer, alloc);
            if (new_buffer == NULL) {
                return -1;
            }
            // Do not DECREF the old buffer
            writer->buffer = NULL;
        }
    }
    else {
        assert(maxchar > writer->maxchar);
        assert(!writer->readonly);

        new_buffer = PyUnicode_New(writer->size, maxchar);
        if (new_buffer == NULL) {
            return -1;
        }
        _PyUnicode_FastCopyCharacters(new_buffer, 0,
                                      writer->buffer, 0, writer->pos);
    }

    _PyUnicodeWriter_SetBuffer(writer, new_buffer);
    return 0;

#undef OVERALLOCATE_FACTOR
}

int
_PyUnicodeWriter_PrepareKindInternal(_PyUnicodeWriter *writer,
                                     int kind)
{
    Py_UCS4 maxchar;

    /* ensure that the _PyUnicodeWriter_PrepareKind macro was used */
    assert(writer->kind < kind);

    switch (kind)
    {
    case PyUnicode_1BYTE_KIND: maxchar = 0xff; break;
    case PyUnicode_2BYTE_KIND: maxchar = 0xffff; break;
    case PyUnicode_4BYTE_KIND: maxchar = _Py_MAX_UNICODE; break;
    default:
        Py_UNREACHABLE();
    }

    return _PyUnicodeWriter_PrepareInternal(writer, 0, maxchar);
}


int
_PyUnicodeWriter_WriteChar(_PyUnicodeWriter *writer, Py_UCS4 ch)
{
    return _PyUnicodeWriter_WriteCharInline(writer, ch);
}


int
PyUnicodeWriter_WriteChar(PyUnicodeWriter *writer, Py_UCS4 ch)
{
    if (ch > _Py_MAX_UNICODE) {
        PyErr_SetString(PyExc_ValueError,
                        "character must be in range(0x110000)");
        return -1;
    }

    return _PyUnicodeWriter_WriteChar((_PyUnicodeWriter*)writer, ch);
}


int
_PyUnicodeWriter_WriteStr(_PyUnicodeWriter *writer, PyObject *str)
{
    assert(PyUnicode_Check(str));

    Py_ssize_t len = PyUnicode_GET_LENGTH(str);
    if (len == 0) {
        return 0;
    }
    Py_UCS4 maxchar = PyUnicode_MAX_CHAR_VALUE(str);

    if (maxchar > writer->maxchar || len > writer->size - writer->pos) {
        if (writer->buffer == NULL && PyUnicode_CheckExact(str)) {
            assert(_PyUnicode_CheckConsistency(str, 1));
            _PyUnicodeWriter_SetReadOnly(writer, Py_NewRef(str), len);
            return 0;
        }
        if (_PyUnicodeWriter_PrepareInternal(writer, len, maxchar) == -1)
            return -1;
    }

    assert(_PyUnicodeWriter_CanWrite(writer));
    _PyUnicode_FastCopyCharacters(writer->buffer, writer->pos,
                                  str, 0, len);
    writer->pos += len;
    return 0;
}


int
PyUnicodeWriter_WriteStr(PyUnicodeWriter *pub_writer, PyObject *obj)
{
    _PyUnicodeWriter *writer = (_PyUnicodeWriter*)pub_writer;
    PyTypeObject *type = Py_TYPE(obj);
    if (type == &PyUnicode_Type) {
        return _PyUnicodeWriter_WriteStr(writer, obj);
    }

    if (type == &PyLong_Type) {
        return _PyLong_FormatWriter(writer, obj, 10, 0);
    }

    PyObject *str = PyObject_Str(obj);
    if (str == NULL) {
        return -1;
    }

    int res = _PyUnicodeWriter_WriteStr(writer, str);
    Py_DECREF(str);
    return res;
}


int
PyUnicodeWriter_WriteRepr(PyUnicodeWriter *pub_writer, PyObject *obj)
{
    _PyUnicodeWriter *writer = (_PyUnicodeWriter*)pub_writer;
    if (obj == NULL) {
        return _PyUnicodeWriter_WriteASCIIString(writer, "<NULL>", 6);
    }

    if (Py_TYPE(obj) == &PyLong_Type) {
        return _PyLong_FormatWriter(writer, obj, 10, 0);
    }

    PyObject *repr = PyObject_Repr(obj);
    if (repr == NULL) {
        return -1;
    }

    int res = _PyUnicodeWriter_WriteStr(writer, repr);
    Py_DECREF(repr);
    return res;
}


int
_PyUnicodeWriter_WriteSubstring(_PyUnicodeWriter *writer, PyObject *str,
                                Py_ssize_t start, Py_ssize_t end)
{
    assert(0 <= start);
    assert(start <= end);
    assert(end <= PyUnicode_GET_LENGTH(str));

    if (start == 0 && end == PyUnicode_GET_LENGTH(str)) {
        return _PyUnicodeWriter_WriteStr(writer, str);
    }

    Py_ssize_t len = end - start;
    if (len == 0) {
        return 0;
    }

    Py_UCS4 maxchar;
    if (PyUnicode_MAX_CHAR_VALUE(str) > writer->maxchar) {
        maxchar = _PyUnicode_FindMaxChar(str, start, end);
    }
    else {
        maxchar = writer->maxchar;
    }
    if (_PyUnicodeWriter_Prepare(writer, len, maxchar) < 0) {
        return -1;
    }
    assert(_PyUnicodeWriter_CanWrite(writer));

    _PyUnicode_FastCopyCharacters(writer->buffer, writer->pos,
                                  str, start, len);
    writer->pos += len;
    return 0;
}


int
PyUnicodeWriter_WriteSubstring(PyUnicodeWriter *writer, PyObject *str,
                               Py_ssize_t start, Py_ssize_t end)
{
    if (!PyUnicode_Check(str)) {
        PyErr_Format(PyExc_TypeError, "expect str, not %T", str);
        return -1;
    }
    if (start < 0 || start > end) {
        PyErr_Format(PyExc_ValueError, "invalid start argument");
        return -1;
    }
    if (end > PyUnicode_GET_LENGTH(str)) {
        PyErr_Format(PyExc_ValueError, "invalid end argument");
        return -1;
    }

    return _PyUnicodeWriter_WriteSubstring((_PyUnicodeWriter*)writer, str,
                                           start, end);
}


int
_PyUnicodeWriter_WriteASCIIString(_PyUnicodeWriter *writer,
                                  const char *ascii, Py_ssize_t len)
{
    if (len == -1)
        len = strlen(ascii);

    if (len == 0) {
        return 0;
    }

    assert(ucs1lib_find_max_char((const Py_UCS1*)ascii, (const Py_UCS1*)ascii + len) <= 127);

    if (_PyUnicodeWriter_Prepare(writer, len, 127) == -1) {
        return -1;
    }
    assert(_PyUnicodeWriter_CanWrite(writer));

    switch (writer->kind)
    {
    case PyUnicode_1BYTE_KIND:
    {
        const Py_UCS1 *str = (const Py_UCS1 *)ascii;
        Py_UCS1 *data = writer->data;

        memcpy(data + writer->pos, str, len);
        break;
    }
    case PyUnicode_2BYTE_KIND:
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS2,
            ascii, ascii + len,
            (Py_UCS2 *)writer->data + writer->pos);
        break;
    }
    case PyUnicode_4BYTE_KIND:
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS4,
            ascii, ascii + len,
            (Py_UCS4 *)writer->data + writer->pos);
        break;
    }
    default:
        Py_UNREACHABLE();
    }

    writer->pos += len;
    return 0;
}


int
PyUnicodeWriter_WriteASCII(PyUnicodeWriter *writer,
                           const char *str,
                           Py_ssize_t size)
{
    assert(writer != NULL);
    _Py_AssertHoldsTstate();

    return _PyUnicodeWriter_WriteASCIIString((_PyUnicodeWriter*)writer, str, size);
}


int
PyUnicodeWriter_WriteUTF8(PyUnicodeWriter *writer,
                          const char *str,
                          Py_ssize_t size)
{
    if (size < 0) {
        size = strlen(str);
    }

    return _PyUnicode_DecodeUTF8Writer((_PyUnicodeWriter*)writer, str, size,
                                       _Py_ERROR_STRICT, NULL, NULL);
}


int
PyUnicodeWriter_DecodeUTF8Stateful(PyUnicodeWriter *writer,
                                   const char *str,
                                   Py_ssize_t size,
                                   const char *errors,
                                   Py_ssize_t *consumed)
{
    if (size < 0) {
        size = strlen(str);
    }

    return _PyUnicode_DecodeUTF8Writer((_PyUnicodeWriter*)writer, str, size,
                                       _Py_ERROR_UNKNOWN, errors,
                                       consumed);
}


int
_PyUnicodeWriter_WriteLatin1String(_PyUnicodeWriter *writer,
                                   const char *str, Py_ssize_t len)
{
    if (len == 0) {
        return 0;
    }

    const Py_UCS1 *ucs1 = (const Py_UCS1 *)str;
    Py_UCS4 maxchar = ucs1lib_find_max_char(ucs1, ucs1 + len);
    if (_PyUnicodeWriter_Prepare(writer, len, maxchar) < 0) {
        return -1;
    }
    assert(_PyUnicodeWriter_CanWrite(writer));

    Py_ssize_t index = writer->pos;
    switch (writer->kind) {
    case PyUnicode_1BYTE_KIND: {
        memcpy((Py_UCS1 *)writer->data + index, ucs1, len);
        break;
    }
    case PyUnicode_2BYTE_KIND: {
        Py_UCS2 *ucs2 = (Py_UCS2 *)writer->data + index;
        const Py_UCS1 *end = ucs1 + len;
        for (; ucs1 < end; ++ucs2, ++ucs1) {
            *ucs2 = (Py_UCS2)*ucs1;
        }
        break;
    }
    case PyUnicode_4BYTE_KIND: {
        Py_UCS4 *ucs4 = (Py_UCS4 *)writer->data + index;
        const Py_UCS1 *end = ucs1 + len;
        for (; ucs1 < end; ++ucs4, ++ucs1) {
            *ucs4 = (Py_UCS4)*ucs1;
        }
        break;
    }
    default:
        Py_UNREACHABLE();
    }

    writer->pos += len;
    return 0;
}


PyObject *
_PyUnicodeWriter_Finish(_PyUnicodeWriter *writer)
{
#ifdef Py_DEBUG
    // Check for buffer overflow
    if (writer->buffer != NULL) {
        Py_ssize_t pos = PyUnicode_GET_LENGTH(writer->buffer);
        Py_UCS4 ch = PyUnicode_READ_CHAR(writer->buffer, pos);
        if (ch != 0) {
            _Py_FatalErrorFormat(__func__,
                                 "Buffer overflow detected in "
                                 "PyUnicodeWriter %p at position %zd",
                                 writer, pos);
        }
    }
#endif

    PyObject *str = writer->buffer;
    writer->buffer = NULL;

    Py_ssize_t final_size = writer->pos;
    if (final_size == 0) {
        // Get the empty string singleton
        PyObject *empty = _PyUnicode_GetEmpty();
        Py_XDECREF(str);  // writer->buffer can be NULL if the position is 0
        return empty;
    }

    if (writer->readonly) {
        assert(final_size == PyUnicode_GET_LENGTH(str));
        goto done;
    }
    assert(final_size <= PyUnicode_GET_LENGTH(str));

    if (final_size == 1 && PyUnicode_KIND(str) == PyUnicode_1BYTE_KIND) {
        // Get the single character singleton
        assert(PyUnicode_GET_LENGTH(str) >= 1);
        const Py_UCS1 *data = PyUnicode_1BYTE_DATA(str);
        Py_UCS1 ch = data[0];
        Py_DECREF(str);
        str = _Py_LATIN1_CHR(ch);
        goto done;
    }

    if (writer->recheck_maxchar) {
        Py_UCS4 maxchar = _PyUnicode_FindMaxChar(str, 0, final_size);
        if (maxchar != writer->maxchar) {
            // Adjust the string kind
            PyObject *str2 = PyUnicode_New(final_size, maxchar);
            if (str2 == NULL) {
                Py_DECREF(str);
                return NULL;
            }
            _PyUnicode_FastCopyCharacters(str2, 0, str, 0, final_size);
            Py_SETREF(str, str2);
            goto done;
        }
    }

    if (PyUnicode_GET_LENGTH(str) != final_size) {
        // Truncate the string
        PyObject *str2 = _PyUnicode_ResizeCompact(str, final_size);
        if (str2 == NULL) {
            Py_DECREF(str);
            return NULL;
        }
        str = str2;
        goto done;
    }

done:
    assert(_PyUnicode_CheckConsistency(str, 1));
    return str;
}


PyObject *
_PyUnicodeWriter_FinishWithSize(_PyUnicodeWriter *writer, Py_ssize_t size)
{
    assert(0 <= size);
    if (writer->buffer != NULL) {
        assert(size <= writer->pos);
        assert(size <= PyUnicode_GET_LENGTH(writer->buffer));
        if (size < writer->pos) {
            // Truncate the string: we may need to adjust the string kind
            writer->recheck_maxchar = 1;
        }
    }
    else {
        assert(size == 0);
    }
    writer->pos = size;
    return _PyUnicodeWriter_Finish(writer);
}


PyObject*
PyUnicodeWriter_Finish(PyUnicodeWriter *writer)
{
    PyObject *str = _PyUnicodeWriter_Finish((_PyUnicodeWriter*)writer);
    assert(((_PyUnicodeWriter*)writer)->buffer == NULL);
    _Py_FREELIST_FREE(unicode_writers, writer, PyMem_Free);
    return str;
}


void
_PyUnicodeWriter_Dealloc(_PyUnicodeWriter *writer)
{
    Py_CLEAR(writer->buffer);
}
