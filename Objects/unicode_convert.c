#include "Python.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_runtime.h"       // runtime->unicode
#include "pycore_unicodeobject.h" // _PyUnicode_GetEmpty()

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
#  include "pycore_fileutils.h"   // _Py_LocaleUsesNonUnicodeWchar()
#endif


/* Compilation of templated routines */

#include "stringlib/asciilib.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs1lib.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs2lib.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs4lib.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"


#define _PyUnicode_LENGTH(op)                           \
    (_PyASCIIObject_CAST(op)->length)

#define _Py_RETURN_UNICODE_EMPTY()   \
    do {                             \
        return _PyUnicode_GetEmpty();\
    } while (0)


static PyObject*
get_latin1_char(Py_UCS1 ch)
{
    PyObject *o = _Py_LATIN1_CHR(ch);
    return o;
}

PyObject*
_PyUnicode_GetLatin1Char(Py_UCS1 ch)
{
    return get_latin1_char(ch);
}


static PyObject*
unicode_char(Py_UCS4 ch)
{
    PyObject *unicode;

    assert(ch <= _Py_MAX_UNICODE);

    if (ch < 256) {
        return get_latin1_char(ch);
    }

    unicode = PyUnicode_New(1, ch);
    if (unicode == NULL)
        return NULL;

    assert(PyUnicode_KIND(unicode) != PyUnicode_1BYTE_KIND);
    if (PyUnicode_KIND(unicode) == PyUnicode_2BYTE_KIND) {
        PyUnicode_2BYTE_DATA(unicode)[0] = (Py_UCS2)ch;
    } else {
        assert(PyUnicode_KIND(unicode) == PyUnicode_4BYTE_KIND);
        PyUnicode_4BYTE_DATA(unicode)[0] = ch;
    }
    assert(_PyUnicode_CheckConsistency(unicode, 1));
    return unicode;
}

PyObject*
_PyUnicode_FromOrdinal(Py_UCS4 ch)
{
    return unicode_char(ch);
}


PyObject *
PyUnicode_FromOrdinal(int ordinal)
{
    if (ordinal < 0 || ordinal > _Py_MAX_UNICODE) {
        PyErr_SetString(PyExc_ValueError,
                        "chr() arg not in range(0x110000)");
        return NULL;
    }

    return unicode_char((Py_UCS4)ordinal);
}


/* Find the maximum code point and count the number of surrogate pairs so a
   correct string length can be computed before converting a string to UCS4.
   This function counts single surrogates as a character and not as a pair.

   Return 0 on success, or -1 on error. */
int
_PyUnicode_FindMaxCharSurrogates(const wchar_t *begin, const wchar_t *end,
                                 Py_UCS4 *maxchar, Py_ssize_t *num_surrogates)
{
    const wchar_t *iter;
    Py_UCS4 ch;

    assert(num_surrogates != NULL && maxchar != NULL);
    *num_surrogates = 0;
    *maxchar = 0;

    for (iter = begin; iter < end; ) {
#if SIZEOF_WCHAR_T == 2
        if (Py_UNICODE_IS_HIGH_SURROGATE(iter[0])
            && (iter+1) < end
            && Py_UNICODE_IS_LOW_SURROGATE(iter[1]))
        {
            ch = Py_UNICODE_JOIN_SURROGATES(iter[0], iter[1]);
            ++(*num_surrogates);
            iter += 2;
        }
        else
#endif
        {
            ch = *iter;
            iter++;
        }
        if (ch > *maxchar) {
            *maxchar = ch;
            if (*maxchar > _Py_MAX_UNICODE) {
                PyErr_Format(PyExc_ValueError,
                             "character U+%x is not in range [U+0000; U+%x]",
                             ch, _Py_MAX_UNICODE);
                return -1;
            }
        }
    }
    return 0;
}


void
_PyUnicode_WriteWideChar(int kind, void *data,
                         const wchar_t *u, Py_ssize_t size,
                         Py_ssize_t num_surrogates)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(wchar_t, unsigned char, u, u + size, data);
        break;

    case PyUnicode_2BYTE_KIND:
#if SIZEOF_WCHAR_T == 2
        memcpy(data, u, size * 2);
#else
        _PyUnicode_CONVERT_BYTES(wchar_t, Py_UCS2, u, u + size, data);
#endif
        break;

    case PyUnicode_4BYTE_KIND:
    {
#if SIZEOF_WCHAR_T == 2
        // Convert a 16-bits wchar_t representation to UCS4, this will decode
        // surrogate pairs.
        const wchar_t *end = u + size;
        Py_UCS4 *ucs4_out = (Py_UCS4*)data;
#  ifndef NDEBUG
        Py_UCS4 *ucs4_end = (Py_UCS4*)data + (size - num_surrogates);
#  endif
        for (const wchar_t *iter = u; iter < end; ) {
            assert(ucs4_out < ucs4_end);
            if (Py_UNICODE_IS_HIGH_SURROGATE(iter[0])
                && (iter+1) < end
                && Py_UNICODE_IS_LOW_SURROGATE(iter[1]))
            {
                *ucs4_out++ = Py_UNICODE_JOIN_SURROGATES(iter[0], iter[1]);
                iter += 2;
            }
            else {
                *ucs4_out++ = *iter;
                iter++;
            }
        }
        assert(ucs4_out == ucs4_end);
#else
        assert(num_surrogates == 0);
        memcpy(data, u, size * 4);
#endif
        break;
    }
    default:
        Py_UNREACHABLE();
    }
}


PyObject *
PyUnicode_FromWideChar(const wchar_t *u, Py_ssize_t size)
{
    PyObject *unicode;
    Py_UCS4 maxchar = 0;
    Py_ssize_t num_surrogates;

    if (u == NULL && size != 0) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (size == -1) {
        size = wcslen(u);
    }

    /* If the Unicode data is known at construction time, we can apply
       some optimizations which share commonly used objects. */

    /* Optimization for empty strings */
    if (size == 0)
        _Py_RETURN_UNICODE_EMPTY();

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
    /* Oracle Solaris uses non-Unicode internal wchar_t form for
       non-Unicode locales and hence needs conversion to UCS-4 first. */
    if (_Py_LocaleUsesNonUnicodeWchar()) {
        wchar_t* converted = _Py_DecodeNonUnicodeWchar(u, size);
        if (!converted) {
            return NULL;
        }
        PyObject *unicode = _PyUnicode_FromUCS4(converted, size);
        PyMem_Free(converted);
        return unicode;
    }
#endif

    /* Single character Unicode objects in the Latin-1 range are
       shared when using this constructor */
    if (size == 1 && (Py_UCS4)*u < 256)
        return get_latin1_char((unsigned char)*u);

    /* If not empty and not single character, copy the Unicode data
       into the new object */
    if (_PyUnicode_FindMaxCharSurrogates(u, u + size,
                                         &maxchar, &num_surrogates) == -1)
        return NULL;

    unicode = PyUnicode_New(size - num_surrogates, maxchar);
    if (!unicode)
        return NULL;

    _PyUnicode_WriteWideChar(PyUnicode_KIND(unicode), PyUnicode_DATA(unicode),
                             u, size, num_surrogates);

    return _PyUnicode_Result(unicode);
}


PyObject *
PyUnicode_FromStringAndSize(const char *u, Py_ssize_t size)
{
    if (size < 0) {
        PyErr_SetString(PyExc_SystemError,
                        "Negative size passed to PyUnicode_FromStringAndSize");
        return NULL;
    }
    if (u != NULL) {
        return PyUnicode_DecodeUTF8Stateful(u, size, NULL, NULL);
    }
    if (size > 0) {
        PyErr_SetString(PyExc_SystemError,
            "NULL string with positive size with NULL passed to PyUnicode_FromStringAndSize");
        return NULL;
    }
    return _PyUnicode_GetEmpty();
}

PyObject *
PyUnicode_FromString(const char *u)
{
    size_t size = strlen(u);
    if (size > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError, "input too long");
        return NULL;
    }
    return PyUnicode_DecodeUTF8Stateful(u, (Py_ssize_t)size, NULL, NULL);
}


PyObject *
_PyUnicode_FromId(_Py_Identifier *id)
{
    PyMutex_Lock((PyMutex *)&id->mutex);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_unicode_ids *ids = &interp->unicode.ids;

    Py_ssize_t index = _Py_atomic_load_ssize(&id->index);
    if (index < 0) {
        struct _Py_unicode_runtime_ids *rt_ids = &interp->runtime->unicode_state.ids;

        PyMutex_Lock(&rt_ids->mutex);
        // Check again to detect concurrent access. Another thread can have
        // initialized the index while this thread waited for the lock.
        index = _Py_atomic_load_ssize(&id->index);
        if (index < 0) {
            assert(rt_ids->next_index < PY_SSIZE_T_MAX);
            index = rt_ids->next_index;
            rt_ids->next_index++;
            _Py_atomic_store_ssize(&id->index, index);
        }
        PyMutex_Unlock(&rt_ids->mutex);
    }
    assert(index >= 0);

    PyObject *obj;
    if (index < ids->size) {
        obj = ids->array[index];
        if (obj) {
            // Return a borrowed reference
            goto end;
        }
    }

    obj = PyUnicode_DecodeUTF8Stateful(id->string, strlen(id->string),
                                       NULL, NULL);
    if (!obj) {
        goto end;
    }
    _PyUnicode_InternImmortal(interp, &obj);

    if (index >= ids->size) {
        // Overallocate to reduce the number of realloc
        Py_ssize_t new_size = Py_MAX(index * 2, 16);
        Py_ssize_t item_size = sizeof(ids->array[0]);
        PyObject **new_array = PyMem_Realloc(ids->array, new_size * item_size);
        if (new_array == NULL) {
            PyErr_NoMemory();
            obj = NULL;
            goto end;
        }
        memset(&new_array[ids->size], 0, (new_size - ids->size) * item_size);
        ids->array = new_array;
        ids->size = new_size;
    }

    // The array stores a strong reference
    ids->array[index] = obj;

end:
    PyMutex_Unlock((PyMutex *)&id->mutex);
    // Return a borrowed reference
    return obj;
}


/* Internal function, doesn't check maximum character */
PyObject*
_PyUnicode_FromASCII(const char *buffer, Py_ssize_t size)
{
    const unsigned char *s = (const unsigned char *)buffer;
    PyObject *unicode;
    if (size == 1) {
#ifdef Py_DEBUG
        assert((unsigned char)s[0] < 128);
#endif
        return get_latin1_char(s[0]);
    }
    unicode = PyUnicode_New(size, 127);
    if (!unicode)
        return NULL;
    memcpy(PyUnicode_1BYTE_DATA(unicode), s, size);
    assert(_PyUnicode_CheckConsistency(unicode, 1));
    return unicode;
}


PyObject*
_PyUnicode_FromUCS1(const Py_UCS1* u, Py_ssize_t size)
{
    PyObject *res;
    unsigned char max_char;

    if (size == 0) {
        _Py_RETURN_UNICODE_EMPTY();
    }
    assert(size > 0);
    if (size == 1) {
        return get_latin1_char(u[0]);
    }

    max_char = ucs1lib_find_max_char(u, u + size);
    res = PyUnicode_New(size, max_char);
    if (!res)
        return NULL;
    memcpy(PyUnicode_1BYTE_DATA(res), u, size);
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}

PyObject*
_PyUnicode_FromUCS2(const Py_UCS2 *u, Py_ssize_t size)
{
    PyObject *res;
    Py_UCS2 max_char;

    if (size == 0)
        _Py_RETURN_UNICODE_EMPTY();
    assert(size > 0);
    if (size == 1)
        return unicode_char(u[0]);

    max_char = ucs2lib_find_max_char(u, u + size);
    res = PyUnicode_New(size, max_char);
    if (!res)
        return NULL;
    if (max_char >= 256)
        memcpy(PyUnicode_2BYTE_DATA(res), u, sizeof(Py_UCS2)*size);
    else {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS2, Py_UCS1, u, u + size, PyUnicode_1BYTE_DATA(res));
    }
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}

PyObject*
_PyUnicode_FromUCS4(const Py_UCS4 *u, Py_ssize_t size)
{
    PyObject *res;
    Py_UCS4 max_char;

    if (size == 0)
        _Py_RETURN_UNICODE_EMPTY();
    assert(size > 0);
    if (size == 1)
        return unicode_char(u[0]);

    max_char = ucs4lib_find_max_char(u, u + size);
    res = PyUnicode_New(size, max_char);
    if (!res)
        return NULL;
    if (max_char < 256)
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS1, u, u + size,
                                 PyUnicode_1BYTE_DATA(res));
    else if (max_char < 0x10000)
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS2, u, u + size,
                                 PyUnicode_2BYTE_DATA(res));
    else
        memcpy(PyUnicode_4BYTE_DATA(res), u, sizeof(Py_UCS4)*size);
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}


PyObject*
PyUnicode_FromKindAndData(int kind, const void *buffer, Py_ssize_t size)
{
    if (size < 0) {
        PyErr_SetString(PyExc_ValueError, "size must be positive");
        return NULL;
    }
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return _PyUnicode_FromUCS1(buffer, size);
    case PyUnicode_2BYTE_KIND:
        return _PyUnicode_FromUCS2(buffer, size);
    case PyUnicode_4BYTE_KIND:
        return _PyUnicode_FromUCS4(buffer, size);
    default:
        PyErr_SetString(PyExc_SystemError, "invalid kind");
        return NULL;
    }
}


static Py_UCS4*
as_ucs4(PyObject *string, Py_UCS4 *target, Py_ssize_t targetsize,
        int copy_null)
{
    int kind;
    const void *data;
    Py_ssize_t len, targetlen;
    kind = PyUnicode_KIND(string);
    data = PyUnicode_DATA(string);
    len = PyUnicode_GET_LENGTH(string);
    targetlen = len;
    if (copy_null)
        targetlen++;
    if (!target) {
        target = PyMem_New(Py_UCS4, targetlen);
        if (!target) {
            PyErr_NoMemory();
            return NULL;
        }
    }
    else {
        if (targetsize < targetlen) {
            PyErr_Format(PyExc_SystemError,
                         "string is longer than the buffer");
            if (copy_null && 0 < targetsize)
                target[0] = 0;
            return NULL;
        }
    }
    if (kind == PyUnicode_1BYTE_KIND) {
        const Py_UCS1 *start = (const Py_UCS1 *) data;
        _PyUnicode_CONVERT_BYTES(Py_UCS1, Py_UCS4, start, start + len, target);
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        const Py_UCS2 *start = (const Py_UCS2 *) data;
        _PyUnicode_CONVERT_BYTES(Py_UCS2, Py_UCS4, start, start + len, target);
    }
    else if (kind == PyUnicode_4BYTE_KIND) {
        memcpy(target, data, len * sizeof(Py_UCS4));
    }
    else {
        Py_UNREACHABLE();
    }
    if (copy_null)
        target[len] = 0;
    return target;
}


Py_UCS4*
PyUnicode_AsUCS4(PyObject *string, Py_UCS4 *target, Py_ssize_t targetsize,
                 int copy_null)
{
    if (target == NULL || targetsize < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return as_ucs4(string, target, targetsize, copy_null);
}

Py_UCS4*
PyUnicode_AsUCS4Copy(PyObject *string)
{
    return as_ucs4(string, NULL, 0, 1);
}


PyObject *
PyUnicode_FromObject(PyObject *obj)
{
    /* XXX Perhaps we should make this API an alias of
       PyObject_Str() instead ?! */
    if (PyUnicode_CheckExact(obj)) {
        return Py_NewRef(obj);
    }
    if (PyUnicode_Check(obj)) {
        /* For a Unicode subtype that's not a Unicode object,
           return a true Unicode object with the same data. */
        return _PyUnicode_Copy(obj);
    }
    PyErr_Format(PyExc_TypeError,
                 "Can't convert '%.100s' object to str implicitly",
                 Py_TYPE(obj)->tp_name);
    return NULL;
}


#ifdef HAVE_WCHAR_H

static Py_ssize_t
unicode_get_widechar_size(PyObject *unicode)
{
    Py_ssize_t res;

    assert(unicode != NULL);
    assert(_PyUnicode_CHECK(unicode));

    res = _PyUnicode_LENGTH(unicode);
#if SIZEOF_WCHAR_T == 2
    if (PyUnicode_KIND(unicode) == PyUnicode_4BYTE_KIND) {
        const Py_UCS4 *s = PyUnicode_4BYTE_DATA(unicode);
        const Py_UCS4 *end = s + res;
        for (; s < end; ++s) {
            if (*s > 0xFFFF) {
                ++res;
            }
        }
    }
#endif
    return res;
}


static void
unicode_copy_as_widechar(PyObject *unicode, wchar_t *w, Py_ssize_t size)
{
    assert(unicode != NULL);
    assert(_PyUnicode_CHECK(unicode));

    if (PyUnicode_KIND(unicode) == sizeof(wchar_t)) {
        memcpy(w, PyUnicode_DATA(unicode), size * sizeof(wchar_t));
        return;
    }

    if (PyUnicode_KIND(unicode) == PyUnicode_1BYTE_KIND) {
        const Py_UCS1 *s = PyUnicode_1BYTE_DATA(unicode);
        for (; size--; ++s, ++w) {
            *w = *s;
        }
    }
    else {
#if SIZEOF_WCHAR_T == 4
        assert(PyUnicode_KIND(unicode) == PyUnicode_2BYTE_KIND);
        const Py_UCS2 *s = PyUnicode_2BYTE_DATA(unicode);
        for (; size--; ++s, ++w) {
            *w = *s;
        }
#else
        assert(PyUnicode_KIND(unicode) == PyUnicode_4BYTE_KIND);
        const Py_UCS4 *s = PyUnicode_4BYTE_DATA(unicode);
        for (; size--; ++s, ++w) {
            Py_UCS4 ch = *s;
            if (ch > 0xFFFF) {
                assert(ch <= _Py_MAX_UNICODE);
                /* encode surrogate pair in this case */
                *w++ = Py_UNICODE_HIGH_SURROGATE(ch);
                if (!size--)
                    break;
                *w = Py_UNICODE_LOW_SURROGATE(ch);
            }
            else {
                *w = ch;
            }
        }
#endif
    }
}


/* Convert a Unicode object to a wide character string.

   - If w is NULL: return the number of wide characters (including the null
     character) required to convert the unicode object. Ignore size argument.

   - Otherwise: return the number of wide characters (excluding the null
     character) written into w. Write at most size wide characters (including
     the null character). */
Py_ssize_t
PyUnicode_AsWideChar(PyObject *unicode,
                     wchar_t *w,
                     Py_ssize_t size)
{
    Py_ssize_t res;

    if (unicode == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return -1;
    }

    res = unicode_get_widechar_size(unicode);
    if (w == NULL) {
        return res + 1;
    }

    if (size > res) {
        size = res + 1;
    }
    else {
        res = size;
    }
    unicode_copy_as_widechar(unicode, w, size);

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
    /* Oracle Solaris uses non-Unicode internal wchar_t form for
       non-Unicode locales and hence needs conversion first. */
    if (_Py_LocaleUsesNonUnicodeWchar()) {
        if (_Py_EncodeNonUnicodeWchar_InPlace(w, size) < 0) {
            return -1;
        }
    }
#endif

    return res;
}

wchar_t*
PyUnicode_AsWideCharString(PyObject *unicode,
                           Py_ssize_t *size)
{
    wchar_t *buffer;
    Py_ssize_t buflen;

    if (unicode == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    buflen = unicode_get_widechar_size(unicode);
    buffer = (wchar_t *) PyMem_New(wchar_t, (buflen + 1));
    if (buffer == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    unicode_copy_as_widechar(unicode, buffer, buflen + 1);

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
    /* Oracle Solaris uses non-Unicode internal wchar_t form for
       non-Unicode locales and hence needs conversion first. */
    if (_Py_LocaleUsesNonUnicodeWchar()) {
        if (_Py_EncodeNonUnicodeWchar_InPlace(buffer, (buflen + 1)) < 0) {
            return NULL;
        }
    }
#endif

    if (size != NULL) {
        *size = buflen;
    }
    else if (wcslen(buffer) != (size_t)buflen) {
        PyMem_Free(buffer);
        PyErr_SetString(PyExc_ValueError,
                        "embedded null character");
        return NULL;
    }
    return buffer;
}

#endif /* HAVE_WCHAR_H */


int
_PyUnicode_WideCharString_Converter(PyObject *obj, void *ptr)
{
    wchar_t **p = (wchar_t **)ptr;
    if (obj == NULL) {
        PyMem_Free(*p);
        *p = NULL;
        return 1;
    }
    if (PyUnicode_Check(obj)) {
        *p = PyUnicode_AsWideCharString(obj, NULL);
        if (*p == NULL) {
            return 0;
        }
        return Py_CLEANUP_SUPPORTED;
    }
    PyErr_Format(PyExc_TypeError,
                 "argument must be str, not %.50s",
                 Py_TYPE(obj)->tp_name);
    return 0;
}

int
_PyUnicode_WideCharString_Opt_Converter(PyObject *obj, void *ptr)
{
    wchar_t **p = (wchar_t **)ptr;
    if (obj == NULL) {
        PyMem_Free(*p);
        *p = NULL;
        return 1;
    }
    if (obj == Py_None) {
        *p = NULL;
        return 1;
    }
    if (PyUnicode_Check(obj)) {
        *p = PyUnicode_AsWideCharString(obj, NULL);
        if (*p == NULL) {
            return 0;
        }
        return Py_CLEANUP_SUPPORTED;
    }
    PyErr_Format(PyExc_TypeError,
                 "argument must be str or None, not %.50s",
                 Py_TYPE(obj)->tp_name);
    return 0;
}


/* Widen Unicode objects to larger buffers. Don't write terminating null
   character. Return NULL on error. */

static void*
unicode_askind(int skind, void const *data, Py_ssize_t len, int kind)
{
    void *result;

    assert(skind < kind);
    switch (kind) {
    case PyUnicode_2BYTE_KIND:
        result = PyMem_New(Py_UCS2, len);
        if (!result)
            return PyErr_NoMemory();
        assert(skind == PyUnicode_1BYTE_KIND);
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS2,
            (const Py_UCS1 *)data,
            ((const Py_UCS1 *)data) + len,
            result);
        return result;
    case PyUnicode_4BYTE_KIND:
        result = PyMem_New(Py_UCS4, len);
        if (!result)
            return PyErr_NoMemory();
        if (skind == PyUnicode_2BYTE_KIND) {
            _PyUnicode_CONVERT_BYTES(
                Py_UCS2, Py_UCS4,
                (const Py_UCS2 *)data,
                ((const Py_UCS2 *)data) + len,
                result);
        }
        else {
            assert(skind == PyUnicode_1BYTE_KIND);
            _PyUnicode_CONVERT_BYTES(
                Py_UCS1, Py_UCS4,
                (const Py_UCS1 *)data,
                ((const Py_UCS1 *)data) + len,
                result);
        }
        return result;
    default:
        Py_UNREACHABLE();
        return NULL;
    }
}


void*
_PyUnicode_AsKind(int skind, void const *data, Py_ssize_t len, int kind)
{
    return unicode_askind(skind, data, len, kind);
}
