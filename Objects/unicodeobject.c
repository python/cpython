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

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "ucnhash.h"

#ifdef MS_WINDOWS
#include <windows.h>
#endif

/* Limit for the Unicode object free list */

#define PyUnicode_MAXFREELIST       1024

/* Limit for the Unicode object free list stay alive optimization.

   The implementation will keep allocated Unicode memory intact for
   all objects on the free list having a size less than this
   limit. This reduces malloc() overhead for small Unicode objects.

   At worst this will result in PyUnicode_MAXFREELIST *
   (sizeof(PyUnicodeObject) + KEEPALIVE_SIZE_LIMIT +
   malloc()-overhead) bytes of unused garbage.

   Setting the limit to 0 effectively turns the feature off.

   Note: This is an experimental feature ! If you get core dumps when
   using Unicode objects, turn this feature off.

*/

#define KEEPALIVE_SIZE_LIMIT       9

/* Endianness switches; defaults to little endian */

#ifdef WORDS_BIGENDIAN
# define BYTEORDER_IS_BIG_ENDIAN
#else
# define BYTEORDER_IS_LITTLE_ENDIAN
#endif

/* --- Globals ------------------------------------------------------------

   The globals are initialized by the _PyUnicode_Init() API and should
   not be used before calling that API.

*/


#ifdef __cplusplus
extern "C" {
#endif

#ifdef Py_DEBUG
#  define _PyUnicode_CHECK(op) _PyUnicode_CheckConsistency(op)
#else
#  define _PyUnicode_CHECK(op) PyUnicode_Check(op)
#endif

#define _PyUnicode_UTF8(op)                             \
    (((PyCompactUnicodeObject*)(op))->utf8)
#define PyUnicode_UTF8(op)                              \
    (assert(_PyUnicode_CHECK(op)),                      \
     assert(PyUnicode_IS_READY(op)),                    \
     PyUnicode_IS_COMPACT_ASCII(op) ?                   \
         ((char*)((PyASCIIObject*)(op) + 1)) :          \
         _PyUnicode_UTF8(op))
#define _PyUnicode_UTF8_LENGTH(op)                      \
    (((PyCompactUnicodeObject*)(op))->utf8_length)
#define PyUnicode_UTF8_LENGTH(op)                       \
    (assert(_PyUnicode_CHECK(op)),                      \
     assert(PyUnicode_IS_READY(op)),                    \
     PyUnicode_IS_COMPACT_ASCII(op) ?                   \
         ((PyASCIIObject*)(op))->length :               \
         _PyUnicode_UTF8_LENGTH(op))
#define _PyUnicode_WSTR(op)                             \
    (((PyASCIIObject*)(op))->wstr)
#define _PyUnicode_WSTR_LENGTH(op)                      \
    (((PyCompactUnicodeObject*)(op))->wstr_length)
#define _PyUnicode_LENGTH(op)                           \
    (((PyASCIIObject *)(op))->length)
#define _PyUnicode_STATE(op)                            \
    (((PyASCIIObject *)(op))->state)
#define _PyUnicode_HASH(op)                             \
    (((PyASCIIObject *)(op))->hash)
#define _PyUnicode_KIND(op)                             \
    (assert(_PyUnicode_CHECK(op)),                      \
     ((PyASCIIObject *)(op))->state.kind)
#define _PyUnicode_GET_LENGTH(op)                       \
    (assert(_PyUnicode_CHECK(op)),                      \
     ((PyASCIIObject *)(op))->length)
#define _PyUnicode_DATA_ANY(op)                         \
    (((PyUnicodeObject*)(op))->data.any)

#undef PyUnicode_READY
#define PyUnicode_READY(op)                             \
    (assert(_PyUnicode_CHECK(op)),                      \
     (PyUnicode_IS_READY(op) ?                          \
      0 :                                               \
      _PyUnicode_Ready((PyObject *)(op))))

#define _PyUnicode_READY_REPLACE(p_obj)                 \
    (assert(_PyUnicode_CHECK(*p_obj)),                  \
     (PyUnicode_IS_READY(*p_obj) ?                      \
      0 : _PyUnicode_ReadyReplace((PyObject **)(p_obj))))

#define _PyUnicode_SHARE_UTF8(op)                       \
    (assert(_PyUnicode_CHECK(op)),                      \
     assert(!PyUnicode_IS_COMPACT_ASCII(op)),           \
     (_PyUnicode_UTF8(op) == PyUnicode_DATA(op)))
#define _PyUnicode_SHARE_WSTR(op)                       \
    (assert(_PyUnicode_CHECK(op)),                      \
     (_PyUnicode_WSTR(unicode) == PyUnicode_DATA(op)))

/* true if the Unicode object has an allocated UTF-8 memory block
   (not shared with other data) */
#define _PyUnicode_HAS_UTF8_MEMORY(op)                  \
    (assert(_PyUnicode_CHECK(op)),                      \
     (!PyUnicode_IS_COMPACT_ASCII(op)                   \
      && _PyUnicode_UTF8(op)                            \
      && _PyUnicode_UTF8(op) != PyUnicode_DATA(op)))

/* true if the Unicode object has an allocated wstr memory block
   (not shared with other data) */
#define _PyUnicode_HAS_WSTR_MEMORY(op)                  \
    (assert(_PyUnicode_CHECK(op)),                      \
     (_PyUnicode_WSTR(op) &&                            \
      (!PyUnicode_IS_READY(op) ||                       \
       _PyUnicode_WSTR(op) != PyUnicode_DATA(op))))

/* Generic helper macro to convert characters of different types.
   from_type and to_type have to be valid type names, begin and end
   are pointers to the source characters which should be of type
   "from_type *".  to is a pointer of type "to_type *" and points to the
   buffer where the result characters are written to. */
#define _PyUnicode_CONVERT_BYTES(from_type, to_type, begin, end, to) \
    do {                                                \
        const from_type *iter_; to_type *to_;           \
        for (iter_ = (begin), to_ = (to_type *)(to);    \
             iter_ < (end);                             \
             ++iter_, ++to_) {                          \
            *to_ = (to_type)*iter_;                     \
        }                                               \
    } while (0)

/* The Unicode string has been modified: reset the hash */
#define _PyUnicode_DIRTY(op) do { _PyUnicode_HASH(op) = -1; } while (0)

/* This dictionary holds all interned unicode strings.  Note that references
   to strings in this dictionary are *not* counted in the string's ob_refcnt.
   When the interned string reaches a refcnt of 0 the string deallocation
   function will delete the reference from this dictionary.

   Another way to look at this is that to say that the actual reference
   count of a string is:  s->ob_refcnt + (s->state ? 2 : 0)
*/
static PyObject *interned;

/* The empty Unicode object is shared to improve performance. */
static PyObject *unicode_empty;

/* Single character Unicode strings in the Latin-1 range are being
   shared as well. */
static PyObject *unicode_latin1[256];

/* Fast detection of the most frequent whitespace characters */
const unsigned char _Py_ascii_whitespace[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
/*     case 0x0009: * CHARACTER TABULATION */
/*     case 0x000A: * LINE FEED */
/*     case 0x000B: * LINE TABULATION */
/*     case 0x000C: * FORM FEED */
/*     case 0x000D: * CARRIAGE RETURN */
    0, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
/*     case 0x001C: * FILE SEPARATOR */
/*     case 0x001D: * GROUP SEPARATOR */
/*     case 0x001E: * RECORD SEPARATOR */
/*     case 0x001F: * UNIT SEPARATOR */
    0, 0, 0, 0, 1, 1, 1, 1,
/*     case 0x0020: * SPACE */
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/* forward */
static PyUnicodeObject *_PyUnicode_New(Py_ssize_t length);
static PyObject* get_latin1_char(unsigned char ch);

static PyObject *
unicode_encode_call_errorhandler(const char *errors,
       PyObject **errorHandler,const char *encoding, const char *reason,
       const Py_UNICODE *unicode, Py_ssize_t size, PyObject **exceptionObject,
       Py_ssize_t startpos, Py_ssize_t endpos, Py_ssize_t *newpos);

static void
raise_encode_exception(PyObject **exceptionObject,
                       const char *encoding,
                       const Py_UNICODE *unicode, Py_ssize_t size,
                       Py_ssize_t startpos, Py_ssize_t endpos,
                       const char *reason);

/* Same for linebreaks */
static unsigned char ascii_linebreak[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
/*         0x000A, * LINE FEED */
/*         0x000B, * LINE TABULATION */
/*         0x000C, * FORM FEED */
/*         0x000D, * CARRIAGE RETURN */
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
/*         0x001C, * FILE SEPARATOR */
/*         0x001D, * GROUP SEPARATOR */
/*         0x001E, * RECORD SEPARATOR */
    0, 0, 0, 0, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/* The max unicode value is always 0x10FFFF while using the PEP-393 API.
   This function is kept for backward compatibility with the old API. */
Py_UNICODE
PyUnicode_GetMax(void)
{
#ifdef Py_UNICODE_WIDE
    return 0x10FFFF;
#else
    /* This is actually an illegal character, so it should
       not be passed to unichr. */
    return 0xFFFF;
#endif
}

#ifdef Py_DEBUG
static int
_PyUnicode_CheckConsistency(void *op)
{
    PyASCIIObject *ascii;
    unsigned int kind;

    assert(PyUnicode_Check(op));

    ascii = (PyASCIIObject *)op;
    kind = ascii->state.kind;

    if (ascii->state.ascii == 1 && ascii->state.compact == 1) {
        assert(kind == PyUnicode_1BYTE_KIND);
        assert(ascii->state.ready == 1);
    }
    else {
        PyCompactUnicodeObject *compact = (PyCompactUnicodeObject *)op;
        void *data;

        if (ascii->state.compact == 1) {
            data = compact + 1;
            assert(kind == PyUnicode_1BYTE_KIND
                   || kind == PyUnicode_2BYTE_KIND
                   || kind == PyUnicode_4BYTE_KIND);
            assert(ascii->state.ascii == 0);
            assert(ascii->state.ready == 1);
            assert (compact->utf8 != data);
        } else {
            PyUnicodeObject *unicode = (PyUnicodeObject *)op;

            data = unicode->data.any;
            if (kind == PyUnicode_WCHAR_KIND) {
                assert(ascii->state.compact == 0);
                assert(ascii->state.ascii == 0);
                assert(ascii->state.ready == 0);
                assert(ascii->wstr != NULL);
                assert(data == NULL);
                assert(compact->utf8 == NULL);
                assert(ascii->state.interned == SSTATE_NOT_INTERNED);
            }
            else {
                assert(kind == PyUnicode_1BYTE_KIND
                       || kind == PyUnicode_2BYTE_KIND
                       || kind == PyUnicode_4BYTE_KIND);
                assert(ascii->state.compact == 0);
                assert(ascii->state.ready == 1);
                assert(data != NULL);
                if (ascii->state.ascii) {
                    assert (compact->utf8 == data);
                    assert (compact->utf8_length == ascii->length);
                }
                else
                    assert (compact->utf8 != data);
            }
        }
        if (kind != PyUnicode_WCHAR_KIND) {
            if (
#if SIZEOF_WCHAR_T == 2
                kind == PyUnicode_2BYTE_KIND
#else
                kind == PyUnicode_4BYTE_KIND
#endif
               )
            {
                assert(ascii->wstr == data);
                assert(compact->wstr_length == ascii->length);
            } else
                assert(ascii->wstr != data);
        }

        if (compact->utf8 == NULL)
            assert(compact->utf8_length == 0);
        if (ascii->wstr == NULL)
            assert(compact->wstr_length == 0);
    }
    return 1;
}
#else
static int
_PyUnicode_CheckConsistency(void *op)
{
    return 1;
}
#endif

/* --- Bloom Filters ----------------------------------------------------- */

/* stuff to implement simple "bloom filters" for Unicode characters.
   to keep things simple, we use a single bitmask, using the least 5
   bits from each unicode characters as the bit index. */

/* the linebreak mask is set up by Unicode_Init below */

#if LONG_BIT >= 128
#define BLOOM_WIDTH 128
#elif LONG_BIT >= 64
#define BLOOM_WIDTH 64
#elif LONG_BIT >= 32
#define BLOOM_WIDTH 32
#else
#error "LONG_BIT is smaller than 32"
#endif

#define BLOOM_MASK unsigned long

static BLOOM_MASK bloom_linebreak;

#define BLOOM_ADD(mask, ch) ((mask |= (1UL << ((ch) & (BLOOM_WIDTH - 1)))))
#define BLOOM(mask, ch)     ((mask &  (1UL << ((ch) & (BLOOM_WIDTH - 1)))))

#define BLOOM_LINEBREAK(ch)                                             \
    ((ch) < 128U ? ascii_linebreak[(ch)] :                              \
     (BLOOM(bloom_linebreak, (ch)) && Py_UNICODE_ISLINEBREAK(ch)))

Py_LOCAL_INLINE(BLOOM_MASK)
make_bloom_mask(int kind, void* ptr, Py_ssize_t len)
{
    /* calculate simple bloom-style bitmask for a given unicode string */

    BLOOM_MASK mask;
    Py_ssize_t i;

    mask = 0;
    for (i = 0; i < len; i++)
        BLOOM_ADD(mask, PyUnicode_READ(kind, ptr, i));

    return mask;
}

#define BLOOM_MEMBER(mask, chr, str) \
    (BLOOM(mask, chr) \
     && (PyUnicode_FindChar(str, chr, 0, PyUnicode_GET_LENGTH(str), 1) >= 0))

/* --- Unicode Object ----------------------------------------------------- */

static PyObject *
fixup(PyUnicodeObject *self, Py_UCS4 (*fixfct)(PyUnicodeObject *s));

Py_LOCAL_INLINE(char *) findchar(void *s, int kind,
                                 Py_ssize_t size, Py_UCS4 ch,
                                 int direction)
{
    /* like wcschr, but doesn't stop at NULL characters */
    Py_ssize_t i;
    if (direction == 1) {
        for(i = 0; i < size; i++)
            if (PyUnicode_READ(kind, s, i) == ch)
                return (char*)s + PyUnicode_KIND_SIZE(kind, i);
    }
    else {
        for(i = size-1; i >= 0; i--)
            if (PyUnicode_READ(kind, s, i) == ch)
                return (char*)s + PyUnicode_KIND_SIZE(kind, i);
    }
    return NULL;
}

static PyObject*
resize_compact(PyObject *unicode, Py_ssize_t length)
{
    Py_ssize_t char_size;
    Py_ssize_t struct_size;
    Py_ssize_t new_size;
    int share_wstr;

    assert(PyUnicode_IS_READY(unicode));
    char_size = PyUnicode_CHARACTER_SIZE(unicode);
    if (PyUnicode_IS_COMPACT_ASCII(unicode))
        struct_size = sizeof(PyASCIIObject);
    else
        struct_size = sizeof(PyCompactUnicodeObject);
    share_wstr = _PyUnicode_SHARE_WSTR(unicode);

    _Py_DEC_REFTOTAL;
    _Py_ForgetReference(unicode);

    if (length > ((PY_SSIZE_T_MAX - struct_size) / char_size - 1)) {
        PyErr_NoMemory();
        return NULL;
    }
    new_size = (struct_size + (length + 1) * char_size);

    unicode = (PyObject *)PyObject_REALLOC((char *)unicode, new_size);
    if (unicode == NULL) {
        PyObject_Del(unicode);
        PyErr_NoMemory();
        return NULL;
    }
    _Py_NewReference(unicode);
    _PyUnicode_LENGTH(unicode) = length;
    if (share_wstr) {
        _PyUnicode_WSTR(unicode) = PyUnicode_DATA(unicode);
        if (!PyUnicode_IS_COMPACT_ASCII(unicode))
            _PyUnicode_WSTR_LENGTH(unicode) = length;
    }
    PyUnicode_WRITE(PyUnicode_KIND(unicode), PyUnicode_DATA(unicode),
                    length, 0);
    return unicode;
}

static int
resize_inplace(PyUnicodeObject *unicode, Py_ssize_t length)
{
    wchar_t *wstr;
    assert(!PyUnicode_IS_COMPACT(unicode));
    assert(Py_REFCNT(unicode) == 1);

    _PyUnicode_DIRTY(unicode);

    if (PyUnicode_IS_READY(unicode)) {
        Py_ssize_t char_size;
        Py_ssize_t new_size;
        int share_wstr, share_utf8;
        void *data;

        data = _PyUnicode_DATA_ANY(unicode);
        assert(data != NULL);
        char_size = PyUnicode_CHARACTER_SIZE(unicode);
        share_wstr = _PyUnicode_SHARE_WSTR(unicode);
        share_utf8 = _PyUnicode_SHARE_UTF8(unicode);
        if (!share_utf8 && _PyUnicode_HAS_UTF8_MEMORY(unicode))
        {
            PyObject_DEL(_PyUnicode_UTF8(unicode));
            _PyUnicode_UTF8(unicode) = NULL;
            _PyUnicode_UTF8_LENGTH(unicode) = 0;
        }

        if (length > (PY_SSIZE_T_MAX / char_size - 1)) {
            PyErr_NoMemory();
            return -1;
        }
        new_size = (length + 1) * char_size;

        data = (PyObject *)PyObject_REALLOC(data, new_size);
        if (data == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        _PyUnicode_DATA_ANY(unicode) = data;
        if (share_wstr) {
            _PyUnicode_WSTR(unicode) = data;
            _PyUnicode_WSTR_LENGTH(unicode) = length;
        }
        if (share_utf8) {
            _PyUnicode_UTF8(unicode) = data;
            _PyUnicode_UTF8_LENGTH(unicode) = length;
        }
        _PyUnicode_LENGTH(unicode) = length;
        PyUnicode_WRITE(PyUnicode_KIND(unicode), data, length, 0);
        if (share_wstr || _PyUnicode_WSTR(unicode) == NULL) {
            _PyUnicode_CheckConsistency(unicode);
            return 0;
        }
    }
    assert(_PyUnicode_WSTR(unicode) != NULL);

    /* check for integer overflow */
    if (length > PY_SSIZE_T_MAX / sizeof(wchar_t) - 1) {
        PyErr_NoMemory();
        return -1;
    }
    wstr =  _PyUnicode_WSTR(unicode);
    wstr = PyObject_REALLOC(wstr, sizeof(wchar_t) * (length + 1));
    if (!wstr) {
        PyErr_NoMemory();
        return -1;
    }
    _PyUnicode_WSTR(unicode) = wstr;
    _PyUnicode_WSTR(unicode)[length] = 0;
    _PyUnicode_WSTR_LENGTH(unicode) = length;
    _PyUnicode_CheckConsistency(unicode);
    return 0;
}

static PyObject*
resize_copy(PyObject *unicode, Py_ssize_t length)
{
    Py_ssize_t copy_length;
    if (PyUnicode_IS_COMPACT(unicode)) {
        PyObject *copy;
        assert(PyUnicode_IS_READY(unicode));

        copy = PyUnicode_New(length, PyUnicode_MAX_CHAR_VALUE(unicode));
        if (copy == NULL)
            return NULL;

        copy_length = Py_MIN(length, PyUnicode_GET_LENGTH(unicode));
        if (PyUnicode_CopyCharacters(copy, 0,
                                     unicode, 0,
                                     copy_length) < 0)
        {
            Py_DECREF(copy);
            return NULL;
        }
        return copy;
    }
    else {
        PyUnicodeObject *w;
        assert(_PyUnicode_WSTR(unicode) != NULL);
        assert(_PyUnicode_DATA_ANY(unicode) == NULL);
        w = _PyUnicode_New(length);
        if (w == NULL)
            return NULL;
        copy_length = _PyUnicode_WSTR_LENGTH(unicode);
        copy_length = Py_MIN(copy_length, length);
        Py_UNICODE_COPY(_PyUnicode_WSTR(w), _PyUnicode_WSTR(unicode),
                        copy_length);
        return (PyObject*)w;
    }
}

/* We allocate one more byte to make sure the string is
   Ux0000 terminated; some code (e.g. new_identifier)
   relies on that.

   XXX This allocator could further be enhanced by assuring that the
   free list never reduces its size below 1.

*/

#ifdef Py_DEBUG
int unicode_old_new_calls = 0;
#endif

static PyUnicodeObject *
_PyUnicode_New(Py_ssize_t length)
{
    register PyUnicodeObject *unicode;
    size_t new_size;

    /* Optimization for empty strings */
    if (length == 0 && unicode_empty != NULL) {
        Py_INCREF(unicode_empty);
        return (PyUnicodeObject*)unicode_empty;
    }

    /* Ensure we won't overflow the size. */
    if (length > ((PY_SSIZE_T_MAX / sizeof(Py_UNICODE)) - 1)) {
        return (PyUnicodeObject *)PyErr_NoMemory();
    }
    if (length < 0) {
        PyErr_SetString(PyExc_SystemError,
                        "Negative size passed to _PyUnicode_New");
        return NULL;
    }

#ifdef Py_DEBUG
    ++unicode_old_new_calls;
#endif

    unicode = PyObject_New(PyUnicodeObject, &PyUnicode_Type);
    if (unicode == NULL)
        return NULL;
    new_size = sizeof(Py_UNICODE) * ((size_t)length + 1);
    _PyUnicode_WSTR(unicode) = (Py_UNICODE*) PyObject_MALLOC(new_size);
    if (!_PyUnicode_WSTR(unicode)) {
        PyErr_NoMemory();
        goto onError;
    }

    /* Initialize the first element to guard against cases where
     * the caller fails before initializing str -- unicode_resize()
     * reads str[0], and the Keep-Alive optimization can keep memory
     * allocated for str alive across a call to unicode_dealloc(unicode).
     * We don't want unicode_resize to read uninitialized memory in
     * that case.
     */
    _PyUnicode_WSTR(unicode)[0] = 0;
    _PyUnicode_WSTR(unicode)[length] = 0;
    _PyUnicode_WSTR_LENGTH(unicode) = length;
    _PyUnicode_HASH(unicode) = -1;
    _PyUnicode_STATE(unicode).interned = 0;
    _PyUnicode_STATE(unicode).kind = 0;
    _PyUnicode_STATE(unicode).compact = 0;
    _PyUnicode_STATE(unicode).ready = 0;
    _PyUnicode_STATE(unicode).ascii = 0;
    _PyUnicode_DATA_ANY(unicode) = NULL;
    _PyUnicode_LENGTH(unicode) = 0;
    _PyUnicode_UTF8(unicode) = NULL;
    _PyUnicode_UTF8_LENGTH(unicode) = 0;
    return unicode;

  onError:
    /* XXX UNREF/NEWREF interface should be more symmetrical */
    _Py_DEC_REFTOTAL;
    _Py_ForgetReference((PyObject *)unicode);
    PyObject_Del(unicode);
    return NULL;
}

static const char*
unicode_kind_name(PyObject *unicode)
{
    /* don't check consistency: unicode_kind_name() is called from
       _PyUnicode_Dump() */
    if (!PyUnicode_IS_COMPACT(unicode))
    {
        if (!PyUnicode_IS_READY(unicode))
            return "wstr";
        switch(PyUnicode_KIND(unicode))
        {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(unicode))
                return "legacy ascii";
            else
                return "legacy latin1";
        case PyUnicode_2BYTE_KIND:
            return "legacy UCS2";
        case PyUnicode_4BYTE_KIND:
            return "legacy UCS4";
        default:
            return "<legacy invalid kind>";
        }
    }
    assert(PyUnicode_IS_READY(unicode));
    switch(PyUnicode_KIND(unicode))
    {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(unicode))
            return "ascii";
        else
            return "latin1";
    case PyUnicode_2BYTE_KIND:
        return "UCS2";
    case PyUnicode_4BYTE_KIND:
        return "UCS4";
    default:
        return "<invalid compact kind>";
    }
}

#ifdef Py_DEBUG
int unicode_new_new_calls = 0;

/* Functions wrapping macros for use in debugger */
char *_PyUnicode_utf8(void *unicode){
    return PyUnicode_UTF8(unicode);
}

void *_PyUnicode_compact_data(void *unicode) {
    return _PyUnicode_COMPACT_DATA(unicode);
}
void *_PyUnicode_data(void *unicode){
    printf("obj %p\n", unicode);
    printf("compact %d\n", PyUnicode_IS_COMPACT(unicode));
    printf("compact ascii %d\n", PyUnicode_IS_COMPACT_ASCII(unicode));
    printf("ascii op %p\n", ((void*)((PyASCIIObject*)(unicode) + 1)));
    printf("compact op %p\n", ((void*)((PyCompactUnicodeObject*)(unicode) + 1)));
    printf("compact data %p\n", _PyUnicode_COMPACT_DATA(unicode));
    return PyUnicode_DATA(unicode);
}

void
_PyUnicode_Dump(PyObject *op)
{
    PyASCIIObject *ascii = (PyASCIIObject *)op;
    PyCompactUnicodeObject *compact = (PyCompactUnicodeObject *)op;
    PyUnicodeObject *unicode = (PyUnicodeObject *)op;
    void *data;
    printf("%s: len=%zu, ",unicode_kind_name(op), ascii->length);
    if (ascii->state.compact)
        data = (compact + 1);
    else
        data = unicode->data.any;
    if (ascii->wstr == data)
        printf("shared ");
    printf("wstr=%p", ascii->wstr);
    if (!(ascii->state.ascii == 1 && ascii->state.compact == 1)) {
        printf(" (%zu), ", compact->wstr_length);
        if (!ascii->state.compact && compact->utf8 == unicode->data.any)
            printf("shared ");
        printf("utf8=%p (%zu)", compact->utf8, compact->utf8_length);
    }
    printf(", data=%p\n", data);
}
#endif

PyObject *
PyUnicode_New(Py_ssize_t size, Py_UCS4 maxchar)
{
    PyObject *obj;
    PyCompactUnicodeObject *unicode;
    void *data;
    int kind_state;
    int is_sharing, is_ascii;
    Py_ssize_t char_size;
    Py_ssize_t struct_size;

    /* Optimization for empty strings */
    if (size == 0 && unicode_empty != NULL) {
        Py_INCREF(unicode_empty);
        return unicode_empty;
    }

#ifdef Py_DEBUG
    ++unicode_new_new_calls;
#endif

    is_ascii = 0;
    is_sharing = 0;
    struct_size = sizeof(PyCompactUnicodeObject);
    if (maxchar < 128) {
        kind_state = PyUnicode_1BYTE_KIND;
        char_size = 1;
        is_ascii = 1;
        struct_size = sizeof(PyASCIIObject);
    }
    else if (maxchar < 256) {
        kind_state = PyUnicode_1BYTE_KIND;
        char_size = 1;
    }
    else if (maxchar < 65536) {
        kind_state = PyUnicode_2BYTE_KIND;
        char_size = 2;
        if (sizeof(wchar_t) == 2)
            is_sharing = 1;
    }
    else {
        kind_state = PyUnicode_4BYTE_KIND;
        char_size = 4;
        if (sizeof(wchar_t) == 4)
            is_sharing = 1;
    }

    /* Ensure we won't overflow the size. */
    if (size < 0) {
        PyErr_SetString(PyExc_SystemError,
                        "Negative size passed to PyUnicode_New");
        return NULL;
    }
    if (size > ((PY_SSIZE_T_MAX - struct_size) / char_size - 1))
        return PyErr_NoMemory();

    /* Duplicated allocation code from _PyObject_New() instead of a call to
     * PyObject_New() so we are able to allocate space for the object and
     * it's data buffer.
     */
    obj = (PyObject *) PyObject_MALLOC(struct_size + (size + 1) * char_size);
    if (obj == NULL)
        return PyErr_NoMemory();
    obj = PyObject_INIT(obj, &PyUnicode_Type);
    if (obj == NULL)
        return NULL;

    unicode = (PyCompactUnicodeObject *)obj;
    if (is_ascii)
        data = ((PyASCIIObject*)obj) + 1;
    else
        data = unicode + 1;
    _PyUnicode_LENGTH(unicode) = size;
    _PyUnicode_HASH(unicode) = -1;
    _PyUnicode_STATE(unicode).interned = 0;
    _PyUnicode_STATE(unicode).kind = kind_state;
    _PyUnicode_STATE(unicode).compact = 1;
    _PyUnicode_STATE(unicode).ready = 1;
    _PyUnicode_STATE(unicode).ascii = is_ascii;
    if (is_ascii) {
        ((char*)data)[size] = 0;
        _PyUnicode_WSTR(unicode) = NULL;
    }
    else if (kind_state == PyUnicode_1BYTE_KIND) {
        ((char*)data)[size] = 0;
        _PyUnicode_WSTR(unicode) = NULL;
        _PyUnicode_WSTR_LENGTH(unicode) = 0;
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
        }
    else {
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
        if (kind_state == PyUnicode_2BYTE_KIND)
            ((Py_UCS2*)data)[size] = 0;
        else /* kind_state == PyUnicode_4BYTE_KIND */
            ((Py_UCS4*)data)[size] = 0;
        if (is_sharing) {
            _PyUnicode_WSTR_LENGTH(unicode) = size;
            _PyUnicode_WSTR(unicode) = (wchar_t *)data;
        }
        else {
            _PyUnicode_WSTR_LENGTH(unicode) = 0;
            _PyUnicode_WSTR(unicode) = NULL;
        }
    }
    return obj;
}

#if SIZEOF_WCHAR_T == 2
/* Helper function to convert a 16-bits wchar_t representation to UCS4, this
   will decode surrogate pairs, the other conversions are implemented as macros
   for efficency.

   This function assumes that unicode can hold one more code point than wstr
   characters for a terminating null character. */
static void
unicode_convert_wchar_to_ucs4(const wchar_t *begin, const wchar_t *end,
                              PyUnicodeObject *unicode)
{
    const wchar_t *iter;
    Py_UCS4 *ucs4_out;

    assert(unicode != NULL);
    assert(_PyUnicode_CHECK(unicode));
    assert(_PyUnicode_KIND(unicode) == PyUnicode_4BYTE_KIND);
    ucs4_out = PyUnicode_4BYTE_DATA(unicode);

    for (iter = begin; iter < end; ) {
        assert(ucs4_out < (PyUnicode_4BYTE_DATA(unicode) +
                           _PyUnicode_GET_LENGTH(unicode)));
        if (*iter >= 0xD800 && *iter <= 0xDBFF
            && (iter+1) < end && iter[1] >= 0xDC00 && iter[1] <= 0xDFFF)
        {
            *ucs4_out++ = (((iter[0] & 0x3FF)<<10) | (iter[1] & 0x3FF)) + 0x10000;
            iter += 2;
        }
        else {
            *ucs4_out++ = *iter;
            iter++;
        }
    }
    assert(ucs4_out == (PyUnicode_4BYTE_DATA(unicode) +
                        _PyUnicode_GET_LENGTH(unicode)));

}
#endif

static int
_PyUnicode_Dirty(PyObject *unicode)
{
    assert(_PyUnicode_CHECK(unicode));
    if (Py_REFCNT(unicode) != 1) {
        PyErr_SetString(PyExc_SystemError,
                        "Cannot modify a string having more than 1 reference");
        return -1;
    }
    _PyUnicode_DIRTY(unicode);
    return 0;
}

Py_ssize_t
PyUnicode_CopyCharacters(PyObject *to, Py_ssize_t to_start,
                         PyObject *from, Py_ssize_t from_start,
                         Py_ssize_t how_many)
{
    unsigned int from_kind, to_kind;
    void *from_data, *to_data;

    if (!PyUnicode_Check(from) || !PyUnicode_Check(to)) {
        PyErr_BadInternalCall();
        return -1;
    }

    if (PyUnicode_READY(from))
        return -1;
    if (PyUnicode_READY(to))
        return -1;

    how_many = Py_MIN(PyUnicode_GET_LENGTH(from), how_many);
    if (to_start + how_many > PyUnicode_GET_LENGTH(to)) {
        PyErr_Format(PyExc_SystemError,
                     "Cannot write %zi characters at %zi "
                     "in a string of %zi characters",
                     how_many, to_start, PyUnicode_GET_LENGTH(to));
        return -1;
    }
    if (how_many == 0)
        return 0;

    if (_PyUnicode_Dirty(to))
        return -1;

    from_kind = PyUnicode_KIND(from);
    from_data = PyUnicode_DATA(from);
    to_kind = PyUnicode_KIND(to);
    to_data = PyUnicode_DATA(to);

    if (from_kind == to_kind
        /* deny latin1 => ascii */
        && PyUnicode_MAX_CHAR_VALUE(to) >= PyUnicode_MAX_CHAR_VALUE(from))
    {
        Py_MEMCPY((char*)to_data
                      + PyUnicode_KIND_SIZE(to_kind, to_start),
                  (char*)from_data
                      + PyUnicode_KIND_SIZE(from_kind, from_start),
                  PyUnicode_KIND_SIZE(to_kind, how_many));
    }
    else if (from_kind == PyUnicode_1BYTE_KIND
             && to_kind == PyUnicode_2BYTE_KIND)
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS2,
            PyUnicode_1BYTE_DATA(from) + from_start,
            PyUnicode_1BYTE_DATA(from) + from_start + how_many,
            PyUnicode_2BYTE_DATA(to) + to_start
            );
    }
    else if (from_kind == PyUnicode_1BYTE_KIND
             && to_kind == PyUnicode_4BYTE_KIND)
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS4,
            PyUnicode_1BYTE_DATA(from) + from_start,
            PyUnicode_1BYTE_DATA(from) + from_start + how_many,
            PyUnicode_4BYTE_DATA(to) + to_start
            );
    }
    else if (from_kind == PyUnicode_2BYTE_KIND
             && to_kind == PyUnicode_4BYTE_KIND)
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS2, Py_UCS4,
            PyUnicode_2BYTE_DATA(from) + from_start,
            PyUnicode_2BYTE_DATA(from) + from_start + how_many,
            PyUnicode_4BYTE_DATA(to) + to_start
            );
    }
    else {
        int invalid_kinds;

        /* check if max_char(from substring) <= max_char(to) */
        if (from_kind > to_kind
                /* latin1 => ascii */
            || (PyUnicode_IS_ASCII(to)
                && to_kind == PyUnicode_1BYTE_KIND
                && !PyUnicode_IS_ASCII(from)))
        {
            /* slow path to check for character overflow */
            const Py_UCS4 to_maxchar = PyUnicode_MAX_CHAR_VALUE(to);
            Py_UCS4 ch, maxchar;
            Py_ssize_t i;

            maxchar = 0;
            invalid_kinds = 0;
            for (i=0; i < how_many; i++) {
                ch = PyUnicode_READ(from_kind, from_data, from_start + i);
                if (ch > maxchar) {
                    maxchar = ch;
                    if (maxchar > to_maxchar) {
                        invalid_kinds = 1;
                        break;
                    }
                }
                PyUnicode_WRITE(to_kind, to_data, to_start + i, ch);
            }
        }
        else
            invalid_kinds = 1;
        if (invalid_kinds) {
            PyErr_Format(PyExc_SystemError,
                         "Cannot copy %s characters "
                         "into a string of %s characters",
                         unicode_kind_name(from),
                         unicode_kind_name(to));
            return -1;
        }
    }
    return how_many;
}

/* Find the maximum code point and count the number of surrogate pairs so a
   correct string length can be computed before converting a string to UCS4.
   This function counts single surrogates as a character and not as a pair.

   Return 0 on success, or -1 on error. */
static int
find_maxchar_surrogates(const wchar_t *begin, const wchar_t *end,
                        Py_UCS4 *maxchar, Py_ssize_t *num_surrogates)
{
    const wchar_t *iter;

    assert(num_surrogates != NULL && maxchar != NULL);
    if (num_surrogates == NULL || maxchar == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "unexpected NULL arguments to "
                        "PyUnicode_FindMaxCharAndNumSurrogatePairs");
        return -1;
    }

    *num_surrogates = 0;
    *maxchar = 0;

    for (iter = begin; iter < end; ) {
        if (*iter > *maxchar)
            *maxchar = *iter;
#if SIZEOF_WCHAR_T == 2
        if (*iter >= 0xD800 && *iter <= 0xDBFF
            && (iter+1) < end && iter[1] >= 0xDC00 && iter[1] <= 0xDFFF)
        {
            Py_UCS4 surrogate_val;
            surrogate_val = (((iter[0] & 0x3FF)<<10)
                             | (iter[1] & 0x3FF)) + 0x10000;
            ++(*num_surrogates);
            if (surrogate_val > *maxchar)
                *maxchar = surrogate_val;
            iter += 2;
        }
        else
            iter++;
#else
        iter++;
#endif
    }
    return 0;
}

#ifdef Py_DEBUG
int unicode_ready_calls = 0;
#endif

static int
unicode_ready(PyObject **p_obj, int replace)
{
    PyUnicodeObject *unicode;
    wchar_t *end;
    Py_UCS4 maxchar = 0;
    Py_ssize_t num_surrogates;
#if SIZEOF_WCHAR_T == 2
    Py_ssize_t length_wo_surrogates;
#endif

    assert(p_obj != NULL);
    unicode = (PyUnicodeObject *)*p_obj;

    /* _PyUnicode_Ready() is only intented for old-style API usage where
       strings were created using _PyObject_New() and where no canonical
       representation (the str field) has been set yet aka strings
       which are not yet ready. */
    assert(_PyUnicode_CHECK(unicode));
    assert(_PyUnicode_KIND(unicode) == PyUnicode_WCHAR_KIND);
    assert(_PyUnicode_WSTR(unicode) != NULL);
    assert(_PyUnicode_DATA_ANY(unicode) == NULL);
    assert(_PyUnicode_UTF8(unicode) == NULL);
    /* Actually, it should neither be interned nor be anything else: */
    assert(_PyUnicode_STATE(unicode).interned == SSTATE_NOT_INTERNED);

#ifdef Py_DEBUG
    ++unicode_ready_calls;
#endif

#ifdef Py_DEBUG
    assert(!replace || Py_REFCNT(unicode) == 1);
#else
    if (replace && Py_REFCNT(unicode) != 1)
        replace = 0;
#endif
    if (replace) {
        Py_ssize_t len = _PyUnicode_WSTR_LENGTH(unicode);
        wchar_t *wstr = _PyUnicode_WSTR(unicode);
        /* Optimization for empty strings */
        if (len == 0) {
            Py_INCREF(unicode_empty);
            Py_DECREF(*p_obj);
            *p_obj = unicode_empty;
            return 0;
        }
        if (len == 1 && wstr[0] < 256) {
            PyObject *latin1_char = get_latin1_char((unsigned char)wstr[0]);
            if (latin1_char == NULL)
                return -1;
            Py_DECREF(*p_obj);
            *p_obj = latin1_char;
            return 0;
        }
    }

    end = _PyUnicode_WSTR(unicode) + _PyUnicode_WSTR_LENGTH(unicode);
    if (find_maxchar_surrogates(_PyUnicode_WSTR(unicode), end,
                                &maxchar, &num_surrogates) == -1)
        return -1;

    if (maxchar < 256) {
        _PyUnicode_DATA_ANY(unicode) = PyObject_MALLOC(_PyUnicode_WSTR_LENGTH(unicode) + 1);
        if (!_PyUnicode_DATA_ANY(unicode)) {
            PyErr_NoMemory();
            return -1;
        }
        _PyUnicode_CONVERT_BYTES(wchar_t, unsigned char,
                                _PyUnicode_WSTR(unicode), end,
                                PyUnicode_1BYTE_DATA(unicode));
        PyUnicode_1BYTE_DATA(unicode)[_PyUnicode_WSTR_LENGTH(unicode)] = '\0';
        _PyUnicode_LENGTH(unicode) = _PyUnicode_WSTR_LENGTH(unicode);
        _PyUnicode_STATE(unicode).kind = PyUnicode_1BYTE_KIND;
        if (maxchar < 128) {
            _PyUnicode_STATE(unicode).ascii = 1;
            _PyUnicode_UTF8(unicode) = _PyUnicode_DATA_ANY(unicode);
            _PyUnicode_UTF8_LENGTH(unicode) = _PyUnicode_WSTR_LENGTH(unicode);
        }
        else {
            _PyUnicode_STATE(unicode).ascii = 0;
            _PyUnicode_UTF8(unicode) = NULL;
            _PyUnicode_UTF8_LENGTH(unicode) = 0;
        }
        PyObject_FREE(_PyUnicode_WSTR(unicode));
        _PyUnicode_WSTR(unicode) = NULL;
        _PyUnicode_WSTR_LENGTH(unicode) = 0;
    }
    /* In this case we might have to convert down from 4-byte native
       wchar_t to 2-byte unicode. */
    else if (maxchar < 65536) {
        assert(num_surrogates == 0 &&
               "FindMaxCharAndNumSurrogatePairs() messed up");

#if SIZEOF_WCHAR_T == 2
        /* We can share representations and are done. */
        _PyUnicode_DATA_ANY(unicode) = _PyUnicode_WSTR(unicode);
        PyUnicode_2BYTE_DATA(unicode)[_PyUnicode_WSTR_LENGTH(unicode)] = '\0';
        _PyUnicode_LENGTH(unicode) = _PyUnicode_WSTR_LENGTH(unicode);
        _PyUnicode_STATE(unicode).kind = PyUnicode_2BYTE_KIND;
        _PyUnicode_UTF8(unicode) = NULL;
        _PyUnicode_UTF8_LENGTH(unicode) = 0;
#else
        /* sizeof(wchar_t) == 4 */
        _PyUnicode_DATA_ANY(unicode) = PyObject_MALLOC(
            2 * (_PyUnicode_WSTR_LENGTH(unicode) + 1));
        if (!_PyUnicode_DATA_ANY(unicode)) {
            PyErr_NoMemory();
            return -1;
        }
        _PyUnicode_CONVERT_BYTES(wchar_t, Py_UCS2,
                                _PyUnicode_WSTR(unicode), end,
                                PyUnicode_2BYTE_DATA(unicode));
        PyUnicode_2BYTE_DATA(unicode)[_PyUnicode_WSTR_LENGTH(unicode)] = '\0';
        _PyUnicode_LENGTH(unicode) = _PyUnicode_WSTR_LENGTH(unicode);
        _PyUnicode_STATE(unicode).kind = PyUnicode_2BYTE_KIND;
        _PyUnicode_UTF8(unicode) = NULL;
        _PyUnicode_UTF8_LENGTH(unicode) = 0;
        PyObject_FREE(_PyUnicode_WSTR(unicode));
        _PyUnicode_WSTR(unicode) = NULL;
        _PyUnicode_WSTR_LENGTH(unicode) = 0;
#endif
    }
    /* maxchar exeeds 16 bit, wee need 4 bytes for unicode characters */
    else {
#if SIZEOF_WCHAR_T == 2
        /* in case the native representation is 2-bytes, we need to allocate a
           new normalized 4-byte version. */
        length_wo_surrogates = _PyUnicode_WSTR_LENGTH(unicode) - num_surrogates;
        _PyUnicode_DATA_ANY(unicode) = PyObject_MALLOC(4 * (length_wo_surrogates + 1));
        if (!_PyUnicode_DATA_ANY(unicode)) {
            PyErr_NoMemory();
            return -1;
        }
        _PyUnicode_LENGTH(unicode) = length_wo_surrogates;
        _PyUnicode_STATE(unicode).kind = PyUnicode_4BYTE_KIND;
        _PyUnicode_UTF8(unicode) = NULL;
        _PyUnicode_UTF8_LENGTH(unicode) = 0;
        /* unicode_convert_wchar_to_ucs4() requires a ready string */
        _PyUnicode_STATE(unicode).ready = 1;
        unicode_convert_wchar_to_ucs4(_PyUnicode_WSTR(unicode), end, unicode);
        PyObject_FREE(_PyUnicode_WSTR(unicode));
        _PyUnicode_WSTR(unicode) = NULL;
        _PyUnicode_WSTR_LENGTH(unicode) = 0;
#else
        assert(num_surrogates == 0);

        _PyUnicode_DATA_ANY(unicode) = _PyUnicode_WSTR(unicode);
        _PyUnicode_LENGTH(unicode) = _PyUnicode_WSTR_LENGTH(unicode);
        _PyUnicode_UTF8(unicode) = NULL;
        _PyUnicode_UTF8_LENGTH(unicode) = 0;
        _PyUnicode_STATE(unicode).kind = PyUnicode_4BYTE_KIND;
#endif
        PyUnicode_4BYTE_DATA(unicode)[_PyUnicode_LENGTH(unicode)] = '\0';
    }
    _PyUnicode_STATE(unicode).ready = 1;
    return 0;
}

int
_PyUnicode_ReadyReplace(PyObject **op)
{
    return unicode_ready(op, 1);
}

int
_PyUnicode_Ready(PyObject *op)
{
    return unicode_ready(&op, 0);
}

static void
unicode_dealloc(register PyUnicodeObject *unicode)
{
    switch (PyUnicode_CHECK_INTERNED(unicode)) {
    case SSTATE_NOT_INTERNED:
        break;

    case SSTATE_INTERNED_MORTAL:
        /* revive dead object temporarily for DelItem */
        Py_REFCNT(unicode) = 3;
        if (PyDict_DelItem(interned, (PyObject *)unicode) != 0)
            Py_FatalError(
                "deletion of interned string failed");
        break;

    case SSTATE_INTERNED_IMMORTAL:
        Py_FatalError("Immortal interned string died.");

    default:
        Py_FatalError("Inconsistent interned string state.");
    }

    if (_PyUnicode_HAS_WSTR_MEMORY(unicode))
        PyObject_DEL(_PyUnicode_WSTR(unicode));
    if (_PyUnicode_HAS_UTF8_MEMORY(unicode))
        PyObject_DEL(_PyUnicode_UTF8(unicode));

    if (PyUnicode_IS_COMPACT(unicode)) {
        Py_TYPE(unicode)->tp_free((PyObject *)unicode);
    }
    else {
        if (_PyUnicode_DATA_ANY(unicode))
            PyObject_DEL(_PyUnicode_DATA_ANY(unicode));
        Py_TYPE(unicode)->tp_free((PyObject *)unicode);
    }
}

static int
unicode_resizable(PyObject *unicode)
{
    if (Py_REFCNT(unicode) != 1)
        return 0;
    if (PyUnicode_CHECK_INTERNED(unicode))
        return 0;
    assert(unicode != unicode_empty);
#ifdef Py_DEBUG
    if (_PyUnicode_KIND(unicode) != PyUnicode_WCHAR_KIND
        && PyUnicode_GET_LENGTH(unicode) == 1)
    {
        Py_UCS4 ch = PyUnicode_READ_CHAR(unicode, 0);
        if (ch < 256 && unicode_latin1[ch] == unicode)
            return 0;
    }
#endif
    return 1;
}

static int
unicode_resize(PyObject **p_unicode, Py_ssize_t length)
{
    PyObject *unicode;
    Py_ssize_t old_length;

    assert(p_unicode != NULL);
    unicode = *p_unicode;

    assert(unicode != NULL);
    assert(PyUnicode_Check(unicode));
    assert(0 <= length);

    if (_PyUnicode_KIND(unicode) == PyUnicode_WCHAR_KIND)
        old_length = PyUnicode_WSTR_LENGTH(unicode);
    else
        old_length = PyUnicode_GET_LENGTH(unicode);
    if (old_length == length)
        return 0;

    if (!unicode_resizable(unicode)) {
        PyObject *copy = resize_copy(unicode, length);
        if (copy == NULL)
            return -1;
        Py_DECREF(*p_unicode);
        *p_unicode = copy;
        return 0;
    }

    if (PyUnicode_IS_COMPACT(unicode)) {
        *p_unicode = resize_compact(unicode, length);
        if (*p_unicode == NULL)
            return -1;
        _PyUnicode_CheckConsistency(*p_unicode);
        return 0;
    }
    return resize_inplace((PyUnicodeObject*)unicode, length);
}

int
PyUnicode_Resize(PyObject **p_unicode, Py_ssize_t length)
{
    PyObject *unicode;
    if (p_unicode == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    unicode = *p_unicode;
    if (unicode == NULL || !PyUnicode_Check(unicode) || length < 0
        || _PyUnicode_KIND(unicode) != PyUnicode_WCHAR_KIND)
    {
        PyErr_BadInternalCall();
        return -1;
    }
    return unicode_resize(p_unicode, length);
}

static PyObject*
get_latin1_char(unsigned char ch)
{
    PyObject *unicode = unicode_latin1[ch];
    if (!unicode) {
        unicode = PyUnicode_New(1, ch);
        if (!unicode)
            return NULL;
        PyUnicode_1BYTE_DATA(unicode)[0] = ch;
        unicode_latin1[ch] = unicode;
    }
    Py_INCREF(unicode);
    return unicode;
}

PyObject *
PyUnicode_FromUnicode(const Py_UNICODE *u, Py_ssize_t size)
{
    PyUnicodeObject *unicode;
    Py_UCS4 maxchar = 0;
    Py_ssize_t num_surrogates;

    if (u == NULL)
        return (PyObject*)_PyUnicode_New(size);

    /* If the Unicode data is known at construction time, we can apply
       some optimizations which share commonly used objects. */

    /* Optimization for empty strings */
    if (size == 0 && unicode_empty != NULL) {
        Py_INCREF(unicode_empty);
        return unicode_empty;
    }

    /* Single character Unicode objects in the Latin-1 range are
       shared when using this constructor */
    if (size == 1 && *u < 256)
        return get_latin1_char((unsigned char)*u);

    /* If not empty and not single character, copy the Unicode data
       into the new object */
    if (find_maxchar_surrogates(u, u + size,
                                &maxchar, &num_surrogates) == -1)
        return NULL;

    unicode = (PyUnicodeObject *) PyUnicode_New(size - num_surrogates,
                                                maxchar);
    if (!unicode)
        return NULL;

    switch (PyUnicode_KIND(unicode)) {
    case PyUnicode_1BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(Py_UNICODE, unsigned char,
                                u, u + size, PyUnicode_1BYTE_DATA(unicode));
        break;
    case PyUnicode_2BYTE_KIND:
#if Py_UNICODE_SIZE == 2
        Py_MEMCPY(PyUnicode_2BYTE_DATA(unicode), u, size * 2);
#else
        _PyUnicode_CONVERT_BYTES(Py_UNICODE, Py_UCS2,
                                u, u + size, PyUnicode_2BYTE_DATA(unicode));
#endif
        break;
    case PyUnicode_4BYTE_KIND:
#if SIZEOF_WCHAR_T == 2
        /* This is the only case which has to process surrogates, thus
           a simple copy loop is not enough and we need a function. */
        unicode_convert_wchar_to_ucs4(u, u + size, unicode);
#else
        assert(num_surrogates == 0);
        Py_MEMCPY(PyUnicode_4BYTE_DATA(unicode), u, size * 4);
#endif
        break;
    default:
        assert(0 && "Impossible state");
    }

    return (PyObject *)unicode;
}

PyObject *
PyUnicode_FromStringAndSize(const char *u, Py_ssize_t size)
{
    PyUnicodeObject *unicode;

    if (size < 0) {
        PyErr_SetString(PyExc_SystemError,
                        "Negative size passed to PyUnicode_FromStringAndSize");
        return NULL;
    }

    /* If the Unicode data is known at construction time, we can apply
       some optimizations which share commonly used objects.
       Also, this means the input must be UTF-8, so fall back to the
       UTF-8 decoder at the end. */
    if (u != NULL) {

        /* Optimization for empty strings */
        if (size == 0 && unicode_empty != NULL) {
            Py_INCREF(unicode_empty);
            return unicode_empty;
        }

        /* Single characters are shared when using this constructor.
           Restrict to ASCII, since the input must be UTF-8. */
        if (size == 1 && Py_CHARMASK(*u) < 128)
            return get_latin1_char(Py_CHARMASK(*u));

        return PyUnicode_DecodeUTF8(u, size, NULL);
    }

    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;

    return (PyObject *)unicode;
}

PyObject *
PyUnicode_FromString(const char *u)
{
    size_t size = strlen(u);
    if (size > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError, "input too long");
        return NULL;
    }

    return PyUnicode_FromStringAndSize(u, size);
}

static PyObject*
_PyUnicode_FromUCS1(const unsigned char* u, Py_ssize_t size)
{
    PyObject *res;
    unsigned char max = 127;
    Py_ssize_t i;
    for (i = 0; i < size; i++) {
        if (u[i] & 0x80) {
            max = 255;
            break;
        }
    }
    res = PyUnicode_New(size, max);
    if (!res)
        return NULL;
    memcpy(PyUnicode_1BYTE_DATA(res), u, size);
    return res;
}

static PyObject*
_PyUnicode_FromUCS2(const Py_UCS2 *u, Py_ssize_t size)
{
    PyObject *res;
    Py_UCS2 max = 0;
    Py_ssize_t i;
    for (i = 0; i < size; i++)
        if (u[i] > max)
            max = u[i];
    res = PyUnicode_New(size, max);
    if (!res)
        return NULL;
    if (max >= 256)
        memcpy(PyUnicode_2BYTE_DATA(res), u, sizeof(Py_UCS2)*size);
    else
        for (i = 0; i < size; i++)
            PyUnicode_1BYTE_DATA(res)[i] = (Py_UCS1)u[i];
    return res;
}

static PyObject*
_PyUnicode_FromUCS4(const Py_UCS4 *u, Py_ssize_t size)
{
    PyObject *res;
    Py_UCS4 max = 0;
    Py_ssize_t i;
    for (i = 0; i < size; i++)
        if (u[i] > max)
            max = u[i];
    res = PyUnicode_New(size, max);
    if (!res)
        return NULL;
    if (max >= 0x10000)
        memcpy(PyUnicode_4BYTE_DATA(res), u, sizeof(Py_UCS4)*size);
    else {
        int kind = PyUnicode_KIND(res);
        void *data = PyUnicode_DATA(res);
        for (i = 0; i < size; i++)
            PyUnicode_WRITE(kind, data, i, u[i]);
    }
    return res;
}

PyObject*
PyUnicode_FromKindAndData(int kind, const void *buffer, Py_ssize_t size)
{
    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        return _PyUnicode_FromUCS1(buffer, size);
    case PyUnicode_2BYTE_KIND:
        return _PyUnicode_FromUCS2(buffer, size);
    case PyUnicode_4BYTE_KIND:
        return _PyUnicode_FromUCS4(buffer, size);
    }
    PyErr_SetString(PyExc_SystemError, "invalid kind");
    return NULL;
}

PyObject*
PyUnicode_Copy(PyObject *unicode)
{
    Py_ssize_t size;
    PyObject *copy;
    void *data;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (PyUnicode_READY(unicode))
        return NULL;

    size = PyUnicode_GET_LENGTH(unicode);
    copy = PyUnicode_New(size, PyUnicode_MAX_CHAR_VALUE(unicode));
    if (!copy)
        return NULL;
    assert(PyUnicode_KIND(copy) == PyUnicode_KIND(unicode));

    data = PyUnicode_DATA(unicode);
    switch (PyUnicode_KIND(unicode))
    {
    case PyUnicode_1BYTE_KIND:
        memcpy(PyUnicode_1BYTE_DATA(copy), data, size);
        break;
    case PyUnicode_2BYTE_KIND:
        memcpy(PyUnicode_2BYTE_DATA(copy), data, sizeof(Py_UCS2) * size);
        break;
    case PyUnicode_4BYTE_KIND:
        memcpy(PyUnicode_4BYTE_DATA(copy), data, sizeof(Py_UCS4) * size);
        break;
    default:
        assert(0);
        break;
    }
    return copy;
}


/* Widen Unicode objects to larger buffers. Don't write terminating null
   character. Return NULL on error. */

void*
_PyUnicode_AsKind(PyObject *s, unsigned int kind)
{
    Py_ssize_t len;
    void *result;
    unsigned int skind;

    if (PyUnicode_READY(s))
        return NULL;

    len = PyUnicode_GET_LENGTH(s);
    skind = PyUnicode_KIND(s);
    if (skind >= kind) {
        PyErr_SetString(PyExc_SystemError, "invalid widening attempt");
        return NULL;
    }
    switch(kind) {
    case PyUnicode_2BYTE_KIND:
        result = PyMem_Malloc(len * sizeof(Py_UCS2));
        if (!result)
            return PyErr_NoMemory();
        assert(skind == PyUnicode_1BYTE_KIND);
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS2,
            PyUnicode_1BYTE_DATA(s),
            PyUnicode_1BYTE_DATA(s) + len,
            result);
        return result;
    case PyUnicode_4BYTE_KIND:
        result = PyMem_Malloc(len * sizeof(Py_UCS4));
        if (!result)
            return PyErr_NoMemory();
        if (skind == PyUnicode_2BYTE_KIND) {
            _PyUnicode_CONVERT_BYTES(
                Py_UCS2, Py_UCS4,
                PyUnicode_2BYTE_DATA(s),
                PyUnicode_2BYTE_DATA(s) + len,
                result);
        }
        else {
            assert(skind == PyUnicode_1BYTE_KIND);
            _PyUnicode_CONVERT_BYTES(
                Py_UCS1, Py_UCS4,
                PyUnicode_1BYTE_DATA(s),
                PyUnicode_1BYTE_DATA(s) + len,
                result);
        }
        return result;
    default:
        break;
    }
    PyErr_SetString(PyExc_SystemError, "invalid kind");
    return NULL;
}

static Py_UCS4*
as_ucs4(PyObject *string, Py_UCS4 *target, Py_ssize_t targetsize,
        int copy_null)
{
    int kind;
    void *data;
    Py_ssize_t len, targetlen;
    if (PyUnicode_READY(string) == -1)
        return NULL;
    kind = PyUnicode_KIND(string);
    data = PyUnicode_DATA(string);
    len = PyUnicode_GET_LENGTH(string);
    targetlen = len;
    if (copy_null)
        targetlen++;
    if (!target) {
        if (PY_SSIZE_T_MAX / sizeof(Py_UCS4) < targetlen) {
            PyErr_NoMemory();
            return NULL;
        }
        target = PyMem_Malloc(targetlen * sizeof(Py_UCS4));
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
    if (kind != PyUnicode_4BYTE_KIND) {
        Py_ssize_t i;
        for (i = 0; i < len; i++)
            target[i] = PyUnicode_READ(kind, data, i);
    }
    else
        Py_MEMCPY(target, data, len * sizeof(Py_UCS4));
    if (copy_null)
        target[len] = 0;
    return target;
}

Py_UCS4*
PyUnicode_AsUCS4(PyObject *string, Py_UCS4 *target, Py_ssize_t targetsize,
                 int copy_null)
{
    if (target == NULL || targetsize < 1) {
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

#ifdef HAVE_WCHAR_H

PyObject *
PyUnicode_FromWideChar(register const wchar_t *w, Py_ssize_t size)
{
    if (w == NULL) {
        if (size == 0)
            return PyUnicode_New(0, 0);
        PyErr_BadInternalCall();
        return NULL;
    }

    if (size == -1) {
        size = wcslen(w);
    }

    return PyUnicode_FromUnicode(w, size);
}

#endif /* HAVE_WCHAR_H */

static void
makefmt(char *fmt, int longflag, int longlongflag, int size_tflag,
        int zeropad, int width, int precision, char c)
{
    *fmt++ = '%';
    if (width) {
        if (zeropad)
            *fmt++ = '0';
        fmt += sprintf(fmt, "%d", width);
    }
    if (precision)
        fmt += sprintf(fmt, ".%d", precision);
    if (longflag)
        *fmt++ = 'l';
    else if (longlongflag) {
        /* longlongflag should only ever be nonzero on machines with
           HAVE_LONG_LONG defined */
#ifdef HAVE_LONG_LONG
        char *f = PY_FORMAT_LONG_LONG;
        while (*f)
            *fmt++ = *f++;
#else
        /* we shouldn't ever get here */
        assert(0);
        *fmt++ = 'l';
#endif
    }
    else if (size_tflag) {
        char *f = PY_FORMAT_SIZE_T;
        while (*f)
            *fmt++ = *f++;
    }
    *fmt++ = c;
    *fmt = '\0';
}

/* helper for PyUnicode_FromFormatV() */

static const char*
parse_format_flags(const char *f,
                   int *p_width, int *p_precision,
                   int *p_longflag, int *p_longlongflag, int *p_size_tflag)
{
    int width, precision, longflag, longlongflag, size_tflag;

    /* parse the width.precision part, e.g. "%2.5s" => width=2, precision=5 */
    f++;
    width = 0;
    while (Py_ISDIGIT((unsigned)*f))
        width = (width*10) + *f++ - '0';
    precision = 0;
    if (*f == '.') {
        f++;
        while (Py_ISDIGIT((unsigned)*f))
            precision = (precision*10) + *f++ - '0';
        if (*f == '%') {
            /* "%.3%s" => f points to "3" */
            f--;
        }
    }
    if (*f == '\0') {
        /* bogus format "%.1" => go backward, f points to "1" */
        f--;
    }
    if (p_width != NULL)
        *p_width = width;
    if (p_precision != NULL)
        *p_precision = precision;

    /* Handle %ld, %lu, %lld and %llu. */
    longflag = 0;
    longlongflag = 0;
    size_tflag = 0;

    if (*f == 'l') {
        if (f[1] == 'd' || f[1] == 'u' || f[1] == 'i') {
            longflag = 1;
            ++f;
        }
#ifdef HAVE_LONG_LONG
        else if (f[1] == 'l' &&
                 (f[2] == 'd' || f[2] == 'u' || f[2] == 'i')) {
            longlongflag = 1;
            f += 2;
        }
#endif
    }
    /* handle the size_t flag. */
    else if (*f == 'z' && (f[1] == 'd' || f[1] == 'u' || f[1] == 'i')) {
        size_tflag = 1;
        ++f;
    }
    if (p_longflag != NULL)
        *p_longflag = longflag;
    if (p_longlongflag != NULL)
        *p_longlongflag = longlongflag;
    if (p_size_tflag != NULL)
        *p_size_tflag = size_tflag;
    return f;
}

/* maximum number of characters required for output of %ld.  21 characters
   allows for 64-bit integers (in decimal) and an optional sign. */
#define MAX_LONG_CHARS 21
/* maximum number of characters required for output of %lld.
   We need at most ceil(log10(256)*SIZEOF_LONG_LONG) digits,
   plus 1 for the sign.  53/22 is an upper bound for log10(256). */
#define MAX_LONG_LONG_CHARS (2 + (SIZEOF_LONG_LONG*53-1) / 22)

PyObject *
PyUnicode_FromFormatV(const char *format, va_list vargs)
{
    va_list count;
    Py_ssize_t callcount = 0;
    PyObject **callresults = NULL;
    PyObject **callresult = NULL;
    Py_ssize_t n = 0;
    int width = 0;
    int precision = 0;
    int zeropad;
    const char* f;
    PyUnicodeObject *string;
    /* used by sprintf */
    char fmt[61]; /* should be enough for %0width.precisionlld */
    Py_UCS4 maxchar = 127; /* result is ASCII by default */
    Py_UCS4 argmaxchar;
    Py_ssize_t numbersize = 0;
    char *numberresults = NULL;
    char *numberresult = NULL;
    Py_ssize_t i;
    int kind;
    void *data;

    Py_VA_COPY(count, vargs);
    /* step 1: count the number of %S/%R/%A/%s format specifications
     * (we call PyObject_Str()/PyObject_Repr()/PyObject_ASCII()/
     * PyUnicode_DecodeUTF8() for these objects once during step 3 and put the
     * result in an array)
     * also esimate a upper bound for all the number formats in the string,
     * numbers will be formated in step 3 and be keept in a '\0'-separated
     * buffer before putting everything together. */
    for (f = format; *f; f++) {
        if (*f == '%') {
            int longlongflag;
            /* skip width or width.precision (eg. "1.2" of "%1.2f") */
            f = parse_format_flags(f, &width, NULL, NULL, &longlongflag, NULL);
            if (*f == 's' || *f=='S' || *f=='R' || *f=='A' || *f=='V')
                ++callcount;

            else if (*f == 'd' || *f=='u' || *f=='i' || *f=='x' || *f=='p') {
#ifdef HAVE_LONG_LONG
                if (longlongflag) {
                    if (width < MAX_LONG_LONG_CHARS)
                        width = MAX_LONG_LONG_CHARS;
                }
                else
#endif
                    /* MAX_LONG_CHARS is enough to hold a 64-bit integer,
                       including sign.  Decimal takes the most space.  This
                       isn't enough for octal.  If a width is specified we
                       need more (which we allocate later). */
                    if (width < MAX_LONG_CHARS)
                        width = MAX_LONG_CHARS;

                /* account for the size + '\0' to separate numbers
                   inside of the numberresults buffer */
                numbersize += (width + 1);
            }
        }
        else if ((unsigned char)*f > 127) {
            PyErr_Format(PyExc_ValueError,
                "PyUnicode_FromFormatV() expects an ASCII-encoded format "
                "string, got a non-ASCII byte: 0x%02x",
                (unsigned char)*f);
            return NULL;
        }
    }
    /* step 2: allocate memory for the results of
     * PyObject_Str()/PyObject_Repr()/PyUnicode_DecodeUTF8() calls */
    if (callcount) {
        callresults = PyObject_Malloc(sizeof(PyObject *) * callcount);
        if (!callresults) {
            PyErr_NoMemory();
            return NULL;
        }
        callresult = callresults;
    }
    /* step 2.5: allocate memory for the results of formating numbers */
    if (numbersize) {
        numberresults = PyObject_Malloc(numbersize);
        if (!numberresults) {
            PyErr_NoMemory();
            goto fail;
        }
        numberresult = numberresults;
    }

    /* step 3: format numbers and figure out how large a buffer we need */
    for (f = format; *f; f++) {
        if (*f == '%') {
            const char* p;
            int longflag;
            int longlongflag;
            int size_tflag;
            int numprinted;

            p = f;
            zeropad = (f[1] == '0');
            f = parse_format_flags(f, &width, &precision,
                                   &longflag, &longlongflag, &size_tflag);
            switch (*f) {
            case 'c':
            {
                Py_UCS4 ordinal = va_arg(count, int);
                maxchar = Py_MAX(maxchar, ordinal);
                n++;
                break;
            }
            case '%':
                n++;
                break;
            case 'i':
            case 'd':
                makefmt(fmt, longflag, longlongflag, size_tflag, zeropad,
                        width, precision, *f);
                if (longflag)
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, long));
#ifdef HAVE_LONG_LONG
                else if (longlongflag)
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, PY_LONG_LONG));
#endif
                else if (size_tflag)
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, Py_ssize_t));
                else
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, int));
                n += numprinted;
                /* advance by +1 to skip over the '\0' */
                numberresult += (numprinted + 1);
                assert(*(numberresult - 1) == '\0');
                assert(*(numberresult - 2) != '\0');
                assert(numprinted >= 0);
                assert(numberresult <= numberresults + numbersize);
                break;
            case 'u':
                makefmt(fmt, longflag, longlongflag, size_tflag, zeropad,
                        width, precision, 'u');
                if (longflag)
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, unsigned long));
#ifdef HAVE_LONG_LONG
                else if (longlongflag)
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, unsigned PY_LONG_LONG));
#endif
                else if (size_tflag)
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, size_t));
                else
                    numprinted = sprintf(numberresult, fmt,
                                         va_arg(count, unsigned int));
                n += numprinted;
                numberresult += (numprinted + 1);
                assert(*(numberresult - 1) == '\0');
                assert(*(numberresult - 2) != '\0');
                assert(numprinted >= 0);
                assert(numberresult <= numberresults + numbersize);
                break;
            case 'x':
                makefmt(fmt, 0, 0, 0, zeropad, width, precision, 'x');
                numprinted = sprintf(numberresult, fmt, va_arg(count, int));
                n += numprinted;
                numberresult += (numprinted + 1);
                assert(*(numberresult - 1) == '\0');
                assert(*(numberresult - 2) != '\0');
                assert(numprinted >= 0);
                assert(numberresult <= numberresults + numbersize);
                break;
            case 'p':
                numprinted = sprintf(numberresult, "%p", va_arg(count, void*));
                /* %p is ill-defined:  ensure leading 0x. */
                if (numberresult[1] == 'X')
                    numberresult[1] = 'x';
                else if (numberresult[1] != 'x') {
                    memmove(numberresult + 2, numberresult,
                            strlen(numberresult) + 1);
                    numberresult[0] = '0';
                    numberresult[1] = 'x';
                    numprinted += 2;
                }
                n += numprinted;
                numberresult += (numprinted + 1);
                assert(*(numberresult - 1) == '\0');
                assert(*(numberresult - 2) != '\0');
                assert(numprinted >= 0);
                assert(numberresult <= numberresults + numbersize);
                break;
            case 's':
            {
                /* UTF-8 */
                const char *s = va_arg(count, const char*);
                PyObject *str = PyUnicode_DecodeUTF8(s, strlen(s), "replace");
                if (!str)
                    goto fail;
                /* since PyUnicode_DecodeUTF8 returns already flexible
                   unicode objects, there is no need to call ready on them */
                argmaxchar = PyUnicode_MAX_CHAR_VALUE(str);
                maxchar = Py_MAX(maxchar, argmaxchar);
                n += PyUnicode_GET_LENGTH(str);
                /* Remember the str and switch to the next slot */
                *callresult++ = str;
                break;
            }
            case 'U':
            {
                PyObject *obj = va_arg(count, PyObject *);
                assert(obj && _PyUnicode_CHECK(obj));
                if (PyUnicode_READY(obj) == -1)
                    goto fail;
                argmaxchar = PyUnicode_MAX_CHAR_VALUE(obj);
                maxchar = Py_MAX(maxchar, argmaxchar);
                n += PyUnicode_GET_LENGTH(obj);
                break;
            }
            case 'V':
            {
                PyObject *obj = va_arg(count, PyObject *);
                const char *str = va_arg(count, const char *);
                PyObject *str_obj;
                assert(obj || str);
                assert(!obj || _PyUnicode_CHECK(obj));
                if (obj) {
                    if (PyUnicode_READY(obj) == -1)
                        goto fail;
                    argmaxchar = PyUnicode_MAX_CHAR_VALUE(obj);
                    maxchar = Py_MAX(maxchar, argmaxchar);
                    n += PyUnicode_GET_LENGTH(obj);
                    *callresult++ = NULL;
                }
                else {
                    str_obj = PyUnicode_DecodeUTF8(str, strlen(str), "replace");
                    if (!str_obj)
                        goto fail;
                    argmaxchar = PyUnicode_MAX_CHAR_VALUE(str_obj);
                    maxchar = Py_MAX(maxchar, argmaxchar);
                    n += PyUnicode_GET_LENGTH(str_obj);
                    *callresult++ = str_obj;
                }
                break;
            }
            case 'S':
            {
                PyObject *obj = va_arg(count, PyObject *);
                PyObject *str;
                assert(obj);
                str = PyObject_Str(obj);
                if (!str || PyUnicode_READY(str) == -1)
                    goto fail;
                argmaxchar = PyUnicode_MAX_CHAR_VALUE(str);
                maxchar = Py_MAX(maxchar, argmaxchar);
                n += PyUnicode_GET_LENGTH(str);
                /* Remember the str and switch to the next slot */
                *callresult++ = str;
                break;
            }
            case 'R':
            {
                PyObject *obj = va_arg(count, PyObject *);
                PyObject *repr;
                assert(obj);
                repr = PyObject_Repr(obj);
                if (!repr || PyUnicode_READY(repr) == -1)
                    goto fail;
                argmaxchar = PyUnicode_MAX_CHAR_VALUE(repr);
                maxchar = Py_MAX(maxchar, argmaxchar);
                n += PyUnicode_GET_LENGTH(repr);
                /* Remember the repr and switch to the next slot */
                *callresult++ = repr;
                break;
            }
            case 'A':
            {
                PyObject *obj = va_arg(count, PyObject *);
                PyObject *ascii;
                assert(obj);
                ascii = PyObject_ASCII(obj);
                if (!ascii || PyUnicode_READY(ascii) == -1)
                    goto fail;
                argmaxchar = PyUnicode_MAX_CHAR_VALUE(ascii);
                maxchar = Py_MAX(maxchar, argmaxchar);
                n += PyUnicode_GET_LENGTH(ascii);
                /* Remember the repr and switch to the next slot */
                *callresult++ = ascii;
                break;
            }
            default:
                /* if we stumble upon an unknown
                   formatting code, copy the rest of
                   the format string to the output
                   string. (we cannot just skip the
                   code, since there's no way to know
                   what's in the argument list) */
                n += strlen(p);
                goto expand;
            }
        } else
            n++;
    }
  expand:
    /* step 4: fill the buffer */
    /* Since we've analyzed how much space we need,
       we don't have to resize the string.
       There can be no errors beyond this point. */
    string = (PyUnicodeObject *)PyUnicode_New(n, maxchar);
    if (!string)
        goto fail;
    kind = PyUnicode_KIND(string);
    data = PyUnicode_DATA(string);
    callresult = callresults;
    numberresult = numberresults;

    for (i = 0, f = format; *f; f++) {
        if (*f == '%') {
            const char* p;

            p = f;
            f = parse_format_flags(f, NULL, NULL, NULL, NULL, NULL);
            /* checking for == because the last argument could be a empty
               string, which causes i to point to end, the assert at the end of
               the loop */
            assert(i <= PyUnicode_GET_LENGTH(string));

            switch (*f) {
            case 'c':
            {
                const int ordinal = va_arg(vargs, int);
                PyUnicode_WRITE(kind, data, i++, ordinal);
                break;
            }
            case 'i':
            case 'd':
            case 'u':
            case 'x':
            case 'p':
                /* unused, since we already have the result */
                if (*f == 'p')
                    (void) va_arg(vargs, void *);
                else
                    (void) va_arg(vargs, int);
                /* extract the result from numberresults and append. */
                for (; *numberresult; ++i, ++numberresult)
                    PyUnicode_WRITE(kind, data, i, *numberresult);
                /* skip over the separating '\0' */
                assert(*numberresult == '\0');
                numberresult++;
                assert(numberresult <= numberresults + numbersize);
                break;
            case 's':
            {
                /* unused, since we already have the result */
                Py_ssize_t size;
                (void) va_arg(vargs, char *);
                size = PyUnicode_GET_LENGTH(*callresult);
                assert(PyUnicode_KIND(*callresult) <= PyUnicode_KIND(string));
                if (PyUnicode_CopyCharacters((PyObject*)string, i,
                                             *callresult, 0,
                                             size) < 0)
                    goto fail;
                i += size;
                /* We're done with the unicode()/repr() => forget it */
                Py_DECREF(*callresult);
                /* switch to next unicode()/repr() result */
                ++callresult;
                break;
            }
            case 'U':
            {
                PyObject *obj = va_arg(vargs, PyObject *);
                Py_ssize_t size;
                assert(PyUnicode_KIND(obj) <= PyUnicode_KIND(string));
                size = PyUnicode_GET_LENGTH(obj);
                if (PyUnicode_CopyCharacters((PyObject*)string, i,
                                             obj, 0,
                                             size) < 0)
                    goto fail;
                i += size;
                break;
            }
            case 'V':
            {
                Py_ssize_t size;
                PyObject *obj = va_arg(vargs, PyObject *);
                va_arg(vargs, const char *);
                if (obj) {
                    size = PyUnicode_GET_LENGTH(obj);
                    assert(PyUnicode_KIND(obj) <= PyUnicode_KIND(string));
                    if (PyUnicode_CopyCharacters((PyObject*)string, i,
                                                 obj, 0,
                                                 size) < 0)
                        goto fail;
                    i += size;
                } else {
                    size = PyUnicode_GET_LENGTH(*callresult);
                    assert(PyUnicode_KIND(*callresult) <=
                           PyUnicode_KIND(string));
                    if (PyUnicode_CopyCharacters((PyObject*)string, i,
                                                 *callresult,
                                                 0, size) < 0)
                        goto fail;
                    i += size;
                    Py_DECREF(*callresult);
                }
                ++callresult;
                break;
            }
            case 'S':
            case 'R':
            case 'A':
            {
                /* unused, since we already have the result */
                (void) va_arg(vargs, PyObject *);
                assert(PyUnicode_KIND(*callresult) <= PyUnicode_KIND(string));
                if (PyUnicode_CopyCharacters((PyObject*)string, i,
                                             *callresult, 0,
                                             PyUnicode_GET_LENGTH(*callresult)) < 0)
                    goto fail;
                i += PyUnicode_GET_LENGTH(*callresult);
                /* We're done with the unicode()/repr() => forget it */
                Py_DECREF(*callresult);
                /* switch to next unicode()/repr() result */
                ++callresult;
                break;
            }
            case '%':
                PyUnicode_WRITE(kind, data, i++, '%');
                break;
            default:
                for (; *p; ++p, ++i)
                    PyUnicode_WRITE(kind, data, i, *p);
                assert(i == PyUnicode_GET_LENGTH(string));
                goto end;
            }
        }
        else {
            assert(i < PyUnicode_GET_LENGTH(string));
            PyUnicode_WRITE(kind, data, i++, *f);
        }
    }
    assert(i == PyUnicode_GET_LENGTH(string));

  end:
    if (callresults)
        PyObject_Free(callresults);
    if (numberresults)
        PyObject_Free(numberresults);
    return (PyObject *)string;
  fail:
    if (callresults) {
        PyObject **callresult2 = callresults;
        while (callresult2 < callresult) {
            Py_XDECREF(*callresult2);
            ++callresult2;
        }
        PyObject_Free(callresults);
    }
    if (numberresults)
        PyObject_Free(numberresults);
    return NULL;
}

PyObject *
PyUnicode_FromFormat(const char *format, ...)
{
    PyObject* ret;
    va_list vargs;

#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    ret = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    return ret;
}

#ifdef HAVE_WCHAR_H

/* Helper function for PyUnicode_AsWideChar() and PyUnicode_AsWideCharString():
   convert a Unicode object to a wide character string.

   - If w is NULL: return the number of wide characters (including the null
     character) required to convert the unicode object. Ignore size argument.

   - Otherwise: return the number of wide characters (excluding the null
     character) written into w. Write at most size wide characters (including
     the null character). */
static Py_ssize_t
unicode_aswidechar(PyUnicodeObject *unicode,
                   wchar_t *w,
                   Py_ssize_t size)
{
    Py_ssize_t res;
    const wchar_t *wstr;

    wstr = PyUnicode_AsUnicodeAndSize((PyObject *)unicode, &res);
    if (wstr == NULL)
        return -1;

    if (w != NULL) {
        if (size > res)
            size = res + 1;
        else
            res = size;
        Py_MEMCPY(w, wstr, size * sizeof(wchar_t));
        return res;
    }
    else
        return res + 1;
}

Py_ssize_t
PyUnicode_AsWideChar(PyObject *unicode,
                     wchar_t *w,
                     Py_ssize_t size)
{
    if (unicode == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    return unicode_aswidechar((PyUnicodeObject*)unicode, w, size);
}

wchar_t*
PyUnicode_AsWideCharString(PyObject *unicode,
                           Py_ssize_t *size)
{
    wchar_t* buffer;
    Py_ssize_t buflen;

    if (unicode == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    buflen = unicode_aswidechar((PyUnicodeObject *)unicode, NULL, 0);
    if (buflen == -1)
        return NULL;
    if (PY_SSIZE_T_MAX / sizeof(wchar_t) < buflen) {
        PyErr_NoMemory();
        return NULL;
    }

    buffer = PyMem_MALLOC(buflen * sizeof(wchar_t));
    if (buffer == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    buflen = unicode_aswidechar((PyUnicodeObject *)unicode, buffer, buflen);
    if (buflen == -1)
        return NULL;
    if (size != NULL)
        *size = buflen;
    return buffer;
}

#endif /* HAVE_WCHAR_H */

PyObject *
PyUnicode_FromOrdinal(int ordinal)
{
    PyObject *v;
    if (ordinal < 0 || ordinal > 0x10ffff) {
        PyErr_SetString(PyExc_ValueError,
                        "chr() arg not in range(0x110000)");
        return NULL;
    }

    if (ordinal < 256)
        return get_latin1_char(ordinal);

    v = PyUnicode_New(1, ordinal);
    if (v == NULL)
        return NULL;
    PyUnicode_WRITE(PyUnicode_KIND(v), PyUnicode_DATA(v), 0, ordinal);
    return v;
}

PyObject *
PyUnicode_FromObject(register PyObject *obj)
{
    /* XXX Perhaps we should make this API an alias of
       PyObject_Str() instead ?! */
    if (PyUnicode_CheckExact(obj)) {
        if (PyUnicode_READY(obj))
            return NULL;
        Py_INCREF(obj);
        return obj;
    }
    if (PyUnicode_Check(obj)) {
        /* For a Unicode subtype that's not a Unicode object,
           return a true Unicode object with the same data. */
        return PyUnicode_Copy(obj);
    }
    PyErr_Format(PyExc_TypeError,
                 "Can't convert '%.100s' object to str implicitly",
                 Py_TYPE(obj)->tp_name);
    return NULL;
}

PyObject *
PyUnicode_FromEncodedObject(register PyObject *obj,
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
            Py_INCREF(unicode_empty);
            v = unicode_empty;
        }
        else {
            v = PyUnicode_Decode(
                    PyBytes_AS_STRING(obj), PyBytes_GET_SIZE(obj),
                    encoding, errors);
        }
        return v;
    }

    if (PyUnicode_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "decoding str is not supported");
        return NULL;
    }

    /* Retrieve a bytes buffer view through the PEP 3118 buffer interface */
    if (PyObject_GetBuffer(obj, &buffer, PyBUF_SIMPLE) < 0) {
        PyErr_Format(PyExc_TypeError,
                     "coercing to str: need bytes, bytearray "
                     "or buffer-like object, %.80s found",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }

    if (buffer.len == 0) {
        Py_INCREF(unicode_empty);
        v = unicode_empty;
    }
    else
        v = PyUnicode_Decode((char*) buffer.buf, buffer.len, encoding, errors);

    PyBuffer_Release(&buffer);
    return v;
}

/* Convert encoding to lower case and replace '_' with '-' in order to
   catch e.g. UTF_8. Return 0 on error (encoding is longer than lower_len-1),
   1 on success. */
static int
normalize_encoding(const char *encoding,
                   char *lower,
                   size_t lower_len)
{
    const char *e;
    char *l;
    char *l_end;

    e = encoding;
    l = lower;
    l_end = &lower[lower_len - 1];
    while (*e) {
        if (l == l_end)
            return 0;
        if (Py_ISUPPER(*e)) {
            *l++ = Py_TOLOWER(*e++);
        }
        else if (*e == '_') {
            *l++ = '-';
            e++;
        }
        else {
            *l++ = *e++;
        }
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
    char lower[11];  /* Enough for any encoding shortcut */

    if (encoding == NULL)
        return PyUnicode_DecodeUTF8(s, size, errors);

    /* Shortcuts for common default encodings */
    if (normalize_encoding(encoding, lower, sizeof(lower))) {
        if ((strcmp(lower, "utf-8") == 0) ||
            (strcmp(lower, "utf8") == 0))
            return PyUnicode_DecodeUTF8(s, size, errors);
        else if ((strcmp(lower, "latin-1") == 0) ||
                 (strcmp(lower, "latin1") == 0) ||
                 (strcmp(lower, "iso-8859-1") == 0))
            return PyUnicode_DecodeLatin1(s, size, errors);
#ifdef HAVE_MBCS
        else if (strcmp(lower, "mbcs") == 0)
            return PyUnicode_DecodeMBCS(s, size, errors);
#endif
        else if (strcmp(lower, "ascii") == 0)
            return PyUnicode_DecodeASCII(s, size, errors);
        else if (strcmp(lower, "utf-16") == 0)
            return PyUnicode_DecodeUTF16(s, size, errors, 0);
        else if (strcmp(lower, "utf-32") == 0)
            return PyUnicode_DecodeUTF32(s, size, errors, 0);
    }

    /* Decode via the codec registry */
    buffer = NULL;
    if (PyBuffer_FillInfo(&info, NULL, (void *)s, size, 1, PyBUF_FULL_RO) < 0)
        goto onError;
    buffer = PyMemoryView_FromBuffer(&info);
    if (buffer == NULL)
        goto onError;
    unicode = PyCodec_Decode(buffer, encoding, errors);
    if (unicode == NULL)
        goto onError;
    if (!PyUnicode_Check(unicode)) {
        PyErr_Format(PyExc_TypeError,
                     "decoder did not return a str object (type=%.400s)",
                     Py_TYPE(unicode)->tp_name);
        Py_DECREF(unicode);
        goto onError;
    }
    Py_DECREF(buffer);
    if (_PyUnicode_READY_REPLACE(&unicode)) {
        Py_DECREF(unicode);
        return NULL;
    }
    return unicode;

  onError:
    Py_XDECREF(buffer);
    return NULL;
}

PyObject *
PyUnicode_AsDecodedObject(PyObject *unicode,
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
    return v;

  onError:
    return NULL;
}

PyObject *
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
                     "decoder did not return a str object (type=%.400s)",
                     Py_TYPE(v)->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return v;

  onError:
    return NULL;
}

PyObject *
PyUnicode_Encode(const Py_UNICODE *s,
                 Py_ssize_t size,
                 const char *encoding,
                 const char *errors)
{
    PyObject *v, *unicode;

    unicode = PyUnicode_FromUnicode(s, size);
    if (unicode == NULL)
        return NULL;
    v = PyUnicode_AsEncodedString(unicode, encoding, errors);
    Py_DECREF(unicode);
    return v;
}

PyObject *
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
PyUnicode_EncodeFSDefault(PyObject *unicode)
{
#ifdef HAVE_MBCS
    return PyUnicode_EncodeMBCS(PyUnicode_AS_UNICODE(unicode),
                                PyUnicode_GET_SIZE(unicode),
                                NULL);
#elif defined(__APPLE__)
    return _PyUnicode_AsUTF8String(unicode, "surrogateescape");
#else
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    /* Bootstrap check: if the filesystem codec is implemented in Python, we
       cannot use it to encode and decode filenames before it is loaded. Load
       the Python codec requires to encode at least its own filename. Use the C
       version of the locale codec until the codec registry is initialized and
       the Python codec is loaded.

       Py_FileSystemDefaultEncoding is shared between all interpreters, we
       cannot only rely on it: check also interp->fscodec_initialized for
       subinterpreters. */
    if (Py_FileSystemDefaultEncoding && interp->fscodec_initialized) {
        return PyUnicode_AsEncodedString(unicode,
                                         Py_FileSystemDefaultEncoding,
                                         "surrogateescape");
    }
    else {
        /* locale encoding with surrogateescape */
        wchar_t *wchar;
        char *bytes;
        PyObject *bytes_obj;
        size_t error_pos;

        wchar = PyUnicode_AsWideCharString(unicode, NULL);
        if (wchar == NULL)
            return NULL;
        bytes = _Py_wchar2char(wchar, &error_pos);
        if (bytes == NULL) {
            if (error_pos != (size_t)-1) {
                char *errmsg = strerror(errno);
                PyObject *exc = NULL;
                if (errmsg == NULL)
                    errmsg = "Py_wchar2char() failed";
                raise_encode_exception(&exc,
                    "filesystemencoding",
                    PyUnicode_AS_UNICODE(unicode), PyUnicode_GET_SIZE(unicode),
                    error_pos, error_pos+1,
                    errmsg);
                Py_XDECREF(exc);
            }
            else
                PyErr_NoMemory();
            PyMem_Free(wchar);
            return NULL;
        }
        PyMem_Free(wchar);

        bytes_obj = PyBytes_FromString(bytes);
        PyMem_Free(bytes);
        return bytes_obj;
    }
#endif
}

PyObject *
PyUnicode_AsEncodedString(PyObject *unicode,
                          const char *encoding,
                          const char *errors)
{
    PyObject *v;
    char lower[11];  /* Enough for any encoding shortcut */

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    if (encoding == NULL) {
        if (errors == NULL || strcmp(errors, "strict") == 0)
            return _PyUnicode_AsUTF8String(unicode, NULL);
        else
            return _PyUnicode_AsUTF8String(unicode, errors);
    }

    /* Shortcuts for common default encodings */
    if (normalize_encoding(encoding, lower, sizeof(lower))) {
        if ((strcmp(lower, "utf-8") == 0) ||
            (strcmp(lower, "utf8") == 0))
        {
            if (errors == NULL || strcmp(errors, "strict") == 0)
                return _PyUnicode_AsUTF8String(unicode, NULL);
            else
                return _PyUnicode_AsUTF8String(unicode, errors);
        }
        else if ((strcmp(lower, "latin-1") == 0) ||
                 (strcmp(lower, "latin1") == 0) ||
                 (strcmp(lower, "iso-8859-1") == 0))
            return _PyUnicode_AsLatin1String(unicode, errors);
#ifdef HAVE_MBCS
        else if (strcmp(lower, "mbcs") == 0)
            return PyUnicode_EncodeMBCS(PyUnicode_AS_UNICODE(unicode),
                                        PyUnicode_GET_SIZE(unicode),
                                        errors);
#endif
        else if (strcmp(lower, "ascii") == 0)
            return _PyUnicode_AsASCIIString(unicode, errors);
    }

    /* Encode via the codec registry */
    v = PyCodec_Encode(unicode, encoding, errors);
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
            "encoder %s returned bytearray instead of bytes",
            encoding);
        if (error) {
            Py_DECREF(v);
            return NULL;
        }

        b = PyBytes_FromStringAndSize(PyByteArray_AS_STRING(v), Py_SIZE(v));
        Py_DECREF(v);
        return b;
    }

    PyErr_Format(PyExc_TypeError,
                 "encoder did not return a bytes object (type=%.400s)",
                 Py_TYPE(v)->tp_name);
    Py_DECREF(v);
    return NULL;
}

PyObject *
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
                     "encoder did not return an str object (type=%.400s)",
                     Py_TYPE(v)->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return v;

  onError:
    return NULL;
}

PyObject*
PyUnicode_DecodeFSDefault(const char *s) {
    Py_ssize_t size = (Py_ssize_t)strlen(s);
    return PyUnicode_DecodeFSDefaultAndSize(s, size);
}

PyObject*
PyUnicode_DecodeFSDefaultAndSize(const char *s, Py_ssize_t size)
{
#ifdef HAVE_MBCS
    return PyUnicode_DecodeMBCS(s, size, NULL);
#elif defined(__APPLE__)
    return PyUnicode_DecodeUTF8(s, size, "surrogateescape");
#else
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    /* Bootstrap check: if the filesystem codec is implemented in Python, we
       cannot use it to encode and decode filenames before it is loaded. Load
       the Python codec requires to encode at least its own filename. Use the C
       version of the locale codec until the codec registry is initialized and
       the Python codec is loaded.

       Py_FileSystemDefaultEncoding is shared between all interpreters, we
       cannot only rely on it: check also interp->fscodec_initialized for
       subinterpreters. */
    if (Py_FileSystemDefaultEncoding && interp->fscodec_initialized) {
        return PyUnicode_Decode(s, size,
                                Py_FileSystemDefaultEncoding,
                                "surrogateescape");
    }
    else {
        /* locale encoding with surrogateescape */
        wchar_t *wchar;
        PyObject *unicode;
        size_t len;

        if (s[size] != '\0' || size != strlen(s)) {
            PyErr_SetString(PyExc_TypeError, "embedded NUL character");
            return NULL;
        }

        wchar = _Py_char2wchar(s, &len);
        if (wchar == NULL)
            return PyErr_NoMemory();

        unicode = PyUnicode_FromWideChar(wchar, len);
        PyMem_Free(wchar);
        return unicode;
    }
#endif
}


int
PyUnicode_FSConverter(PyObject* arg, void* addr)
{
    PyObject *output = NULL;
    Py_ssize_t size;
    void *data;
    if (arg == NULL) {
        Py_DECREF(*(PyObject**)addr);
        return 1;
    }
    if (PyBytes_Check(arg)) {
        output = arg;
        Py_INCREF(output);
    }
    else {
        arg = PyUnicode_FromObject(arg);
        if (!arg)
            return 0;
        output = PyUnicode_EncodeFSDefault(arg);
        Py_DECREF(arg);
        if (!output)
            return 0;
        if (!PyBytes_Check(output)) {
            Py_DECREF(output);
            PyErr_SetString(PyExc_TypeError, "encoder failed to return bytes");
            return 0;
        }
    }
    size = PyBytes_GET_SIZE(output);
    data = PyBytes_AS_STRING(output);
    if (size != strlen(data)) {
        PyErr_SetString(PyExc_TypeError, "embedded NUL character");
        Py_DECREF(output);
        return 0;
    }
    *(PyObject**)addr = output;
    return Py_CLEANUP_SUPPORTED;
}


int
PyUnicode_FSDecoder(PyObject* arg, void* addr)
{
    PyObject *output = NULL;
    if (arg == NULL) {
        Py_DECREF(*(PyObject**)addr);
        return 1;
    }
    if (PyUnicode_Check(arg)) {
        if (PyUnicode_READY(arg))
            return 0;
        output = arg;
        Py_INCREF(output);
    }
    else {
        arg = PyBytes_FromObject(arg);
        if (!arg)
            return 0;
        output = PyUnicode_DecodeFSDefaultAndSize(PyBytes_AS_STRING(arg),
                                                  PyBytes_GET_SIZE(arg));
        Py_DECREF(arg);
        if (!output)
            return 0;
        if (!PyUnicode_Check(output)) {
            Py_DECREF(output);
            PyErr_SetString(PyExc_TypeError, "decoder failed to return unicode");
            return 0;
        }
    }
    if (findchar(PyUnicode_DATA(output), PyUnicode_KIND(output),
                 PyUnicode_GET_LENGTH(output), 0, 1)) {
        PyErr_SetString(PyExc_TypeError, "embedded NUL character");
        Py_DECREF(output);
        return 0;
    }
    *(PyObject**)addr = output;
    return Py_CLEANUP_SUPPORTED;
}


char*
PyUnicode_AsUTF8AndSize(PyObject *unicode, Py_ssize_t *psize)
{
    PyObject *bytes;
    PyUnicodeObject *u = (PyUnicodeObject *)unicode;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (PyUnicode_READY(u) == -1)
        return NULL;

    if (PyUnicode_UTF8(unicode) == NULL) {
        assert(!PyUnicode_IS_COMPACT_ASCII(unicode));
        bytes = _PyUnicode_AsUTF8String(unicode, "strict");
        if (bytes == NULL)
            return NULL;
        _PyUnicode_UTF8(u) = PyObject_MALLOC(PyBytes_GET_SIZE(bytes) + 1);
        if (_PyUnicode_UTF8(u) == NULL) {
            Py_DECREF(bytes);
            return NULL;
        }
        _PyUnicode_UTF8_LENGTH(u) = PyBytes_GET_SIZE(bytes);
        Py_MEMCPY(_PyUnicode_UTF8(u), PyBytes_AS_STRING(bytes), _PyUnicode_UTF8_LENGTH(u) + 1);
        Py_DECREF(bytes);
    }

    if (psize)
        *psize = PyUnicode_UTF8_LENGTH(unicode);
    return PyUnicode_UTF8(unicode);
}

char*
PyUnicode_AsUTF8(PyObject *unicode)
{
    return PyUnicode_AsUTF8AndSize(unicode, NULL);
}

#ifdef Py_DEBUG
int unicode_as_unicode_calls = 0;
#endif


Py_UNICODE *
PyUnicode_AsUnicodeAndSize(PyObject *unicode, Py_ssize_t *size)
{
    PyUnicodeObject *u;
    const unsigned char *one_byte;
#if SIZEOF_WCHAR_T == 4
    const Py_UCS2 *two_bytes;
#else
    const Py_UCS4 *four_bytes;
    const Py_UCS4 *ucs4_end;
    Py_ssize_t num_surrogates;
#endif
    wchar_t *w;
    wchar_t *wchar_end;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    u = (PyUnicodeObject*)unicode;
    if (_PyUnicode_WSTR(u) == NULL) {
        /* Non-ASCII compact unicode object */
        assert(_PyUnicode_KIND(u) != 0);
        assert(PyUnicode_IS_READY(u));

#ifdef Py_DEBUG
        ++unicode_as_unicode_calls;
#endif

        if (PyUnicode_KIND(u) == PyUnicode_4BYTE_KIND) {
#if SIZEOF_WCHAR_T == 2
            four_bytes = PyUnicode_4BYTE_DATA(u);
            ucs4_end = four_bytes + _PyUnicode_LENGTH(u);
            num_surrogates = 0;

            for (; four_bytes < ucs4_end; ++four_bytes) {
                if (*four_bytes > 0xFFFF)
                    ++num_surrogates;
            }

            _PyUnicode_WSTR(u) = (wchar_t *) PyObject_MALLOC(
                    sizeof(wchar_t) * (_PyUnicode_LENGTH(u) + 1 + num_surrogates));
            if (!_PyUnicode_WSTR(u)) {
                PyErr_NoMemory();
                return NULL;
            }
            _PyUnicode_WSTR_LENGTH(u) = _PyUnicode_LENGTH(u) + num_surrogates;

            w = _PyUnicode_WSTR(u);
            wchar_end = w + _PyUnicode_WSTR_LENGTH(u);
            four_bytes = PyUnicode_4BYTE_DATA(u);
            for (; four_bytes < ucs4_end; ++four_bytes, ++w) {
                if (*four_bytes > 0xFFFF) {
                    /* encode surrogate pair in this case */
                    *w++ = 0xD800 | ((*four_bytes - 0x10000) >> 10);
                    *w   = 0xDC00 | ((*four_bytes - 0x10000) & 0x3FF);
                }
                else
                    *w = *four_bytes;

                if (w > wchar_end) {
                    assert(0 && "Miscalculated string end");
                }
            }
            *w = 0;
#else
            /* sizeof(wchar_t) == 4 */
            Py_FatalError("Impossible unicode object state, wstr and str "
                          "should share memory already.");
            return NULL;
#endif
        }
        else {
            _PyUnicode_WSTR(u) = (wchar_t *) PyObject_MALLOC(sizeof(wchar_t) *
                                                  (_PyUnicode_LENGTH(u) + 1));
            if (!_PyUnicode_WSTR(u)) {
                PyErr_NoMemory();
                return NULL;
            }
            if (!PyUnicode_IS_COMPACT_ASCII(u))
                _PyUnicode_WSTR_LENGTH(u) = _PyUnicode_LENGTH(u);
            w = _PyUnicode_WSTR(u);
            wchar_end = w + _PyUnicode_LENGTH(u);

            if (PyUnicode_KIND(u) == PyUnicode_1BYTE_KIND) {
                one_byte = PyUnicode_1BYTE_DATA(u);
                for (; w < wchar_end; ++one_byte, ++w)
                    *w = *one_byte;
                /* null-terminate the wstr */
                *w = 0;
            }
            else if (PyUnicode_KIND(u) == PyUnicode_2BYTE_KIND) {
#if SIZEOF_WCHAR_T == 4
                two_bytes = PyUnicode_2BYTE_DATA(u);
                for (; w < wchar_end; ++two_bytes, ++w)
                    *w = *two_bytes;
                /* null-terminate the wstr */
                *w = 0;
#else
                /* sizeof(wchar_t) == 2 */
                PyObject_FREE(_PyUnicode_WSTR(u));
                _PyUnicode_WSTR(u) = NULL;
                Py_FatalError("Impossible unicode object state, wstr "
                              "and str should share memory already.");
                return NULL;
#endif
            }
            else {
                assert(0 && "This should never happen.");
            }
        }
    }
    if (size != NULL)
        *size = PyUnicode_WSTR_LENGTH(u);
    return _PyUnicode_WSTR(u);
}

Py_UNICODE *
PyUnicode_AsUnicode(PyObject *unicode)
{
    return PyUnicode_AsUnicodeAndSize(unicode, NULL);
}


Py_ssize_t
PyUnicode_GetSize(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        goto onError;
    }
    return PyUnicode_GET_SIZE(unicode);

  onError:
    return -1;
}

Py_ssize_t
PyUnicode_GetLength(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode) || PyUnicode_READY(unicode) == -1) {
        PyErr_BadArgument();
        return -1;
    }

    return PyUnicode_GET_LENGTH(unicode);
}

Py_UCS4
PyUnicode_ReadChar(PyObject *unicode, Py_ssize_t index)
{
    if (!PyUnicode_Check(unicode) || PyUnicode_READY(unicode) == -1) {
        PyErr_BadArgument();
        return (Py_UCS4)-1;
    }
    if (index < 0 || index >= _PyUnicode_LENGTH(unicode)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return (Py_UCS4)-1;
    }
    return PyUnicode_READ_CHAR(unicode, index);
}

int
PyUnicode_WriteChar(PyObject *unicode, Py_ssize_t index, Py_UCS4 ch)
{
    if (!PyUnicode_Check(unicode) || !PyUnicode_IS_COMPACT(unicode)) {
        PyErr_BadArgument();
        return -1;
    }
    if (index < 0 || index >= _PyUnicode_LENGTH(unicode)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if (_PyUnicode_Dirty(unicode))
        return -1;
    PyUnicode_WRITE(PyUnicode_KIND(unicode), PyUnicode_DATA(unicode),
                    index, ch);
    return 0;
}

const char *
PyUnicode_GetDefaultEncoding(void)
{
    return "utf-8";
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
    Py_DECREF(*exceptionObject);
    *exceptionObject = NULL;
}

/* error handling callback helper:
   build arguments, call the callback and check the arguments,
   if no exception occurred, copy the replacement to the output
   and adjust various state variables.
   return 0 on success, -1 on error
*/

static int
unicode_decode_call_errorhandler(const char *errors, PyObject **errorHandler,
                                 const char *encoding, const char *reason,
                                 const char **input, const char **inend, Py_ssize_t *startinpos,
                                 Py_ssize_t *endinpos, PyObject **exceptionObject, const char **inptr,
                                 PyUnicodeObject **output, Py_ssize_t *outpos, Py_UNICODE **outptr)
{
    static char *argparse = "O!n;decoding error handler must return (str, int) tuple";

    PyObject *restuple = NULL;
    PyObject *repunicode = NULL;
    Py_ssize_t outsize = PyUnicode_GET_SIZE(*output);
    Py_ssize_t insize;
    Py_ssize_t requiredsize;
    Py_ssize_t newpos;
    const Py_UNICODE *repptr;
    PyObject *inputobj = NULL;
    Py_ssize_t repsize;
    int res = -1;

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

    restuple = PyObject_CallFunctionObjArgs(*errorHandler, *exceptionObject, NULL);
    if (restuple == NULL)
        goto onError;
    if (!PyTuple_Check(restuple)) {
        PyErr_SetString(PyExc_TypeError, &argparse[4]);
        goto onError;
    }
    if (!PyArg_ParseTuple(restuple, argparse, &PyUnicode_Type, &repunicode, &newpos))
        goto onError;

    /* Copy back the bytes variables, which might have been modified by the
       callback */
    inputobj = PyUnicodeDecodeError_GetObject(*exceptionObject);
    if (!inputobj)
        goto onError;
    if (!PyBytes_Check(inputobj)) {
        PyErr_Format(PyExc_TypeError, "exception attribute object must be bytes");
    }
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

    /* need more space? (at least enough for what we
       have+the replacement+the rest of the string (starting
       at the new input position), so we won't have to check space
       when there are no errors in the rest of the string) */
    repptr = PyUnicode_AS_UNICODE(repunicode);
    repsize = PyUnicode_GET_SIZE(repunicode);
    requiredsize = *outpos + repsize + insize-newpos;
    if (requiredsize > outsize) {
        if (requiredsize<2*outsize)
            requiredsize = 2*outsize;
        if (PyUnicode_Resize((PyObject**)output, requiredsize) < 0)
            goto onError;
        *outptr = PyUnicode_AS_UNICODE(*output) + *outpos;
    }
    *endinpos = newpos;
    *inptr = *input + newpos;
    Py_UNICODE_COPY(*outptr, repptr, repsize);
    *outptr += repsize;
    *outpos += repsize;

    /* we made it! */
    res = 0;

  onError:
    Py_XDECREF(restuple);
    return res;
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
 * on whether we are encoding whitespace as itself.  RFC2152 makes it
 * clear that the answers to these questions vary between
 * applications, so this code needs to be flexible.  */

#define ENCODE_DIRECT(c, directO, directWS)             \
    ((c) < 128 && (c) > 0 &&                            \
     ((utf7_category[(c)] == 0) ||                      \
      (directWS && (utf7_category[(c)] == 2)) ||        \
      (directO && (utf7_category[(c)] == 1))))

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
    Py_ssize_t outpos;
    const char *e;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const char *errmsg = "";
    int inShift = 0;
    Py_UNICODE *shiftOutStart;
    unsigned int base64bits = 0;
    unsigned long base64buffer = 0;
    Py_UNICODE surrogate = 0;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;
    if (size == 0) {
        if (consumed)
            *consumed = 0;
        return (PyObject *)unicode;
    }

    p = PyUnicode_AS_UNICODE(unicode);
    shiftOutStart = p;
    e = s + size;

    while (s < e) {
        Py_UNICODE ch;
      restart:
        ch = (unsigned char) *s;

        if (inShift) { /* in a base-64 section */
            if (IS_BASE64(ch)) { /* consume a base-64 character */
                base64buffer = (base64buffer << 6) | FROM_BASE64(ch);
                base64bits += 6;
                s++;
                if (base64bits >= 16) {
                    /* we have enough bits for a UTF-16 value */
                    Py_UNICODE outCh = (Py_UNICODE)
                                       (base64buffer >> (base64bits-16));
                    base64bits -= 16;
                    base64buffer &= (1 << base64bits) - 1; /* clear high bits */
                    if (surrogate) {
                        /* expecting a second surrogate */
                        if (outCh >= 0xDC00 && outCh <= 0xDFFF) {
#ifdef Py_UNICODE_WIDE
                            *p++ = (((surrogate & 0x3FF)<<10)
                                    | (outCh & 0x3FF)) + 0x10000;
#else
                            *p++ = surrogate;
                            *p++ = outCh;
#endif
                            surrogate = 0;
                        }
                        else {
                            surrogate = 0;
                            errmsg = "second surrogate missing";
                            goto utf7Error;
                        }
                    }
                    else if (outCh >= 0xD800 && outCh <= 0xDBFF) {
                        /* first surrogate */
                        surrogate = outCh;
                    }
                    else if (outCh >= 0xDC00 && outCh <= 0xDFFF) {
                        errmsg = "unexpected second surrogate";
                        goto utf7Error;
                    }
                    else {
                        *p++ = outCh;
                    }
                }
            }
            else { /* now leaving a base-64 section */
                inShift = 0;
                s++;
                if (surrogate) {
                    errmsg = "second surrogate missing at end of shift sequence";
                    goto utf7Error;
                }
                if (base64bits > 0) { /* left-over bits */
                    if (base64bits >= 6) {
                        /* We've seen at least one base-64 character */
                        errmsg = "partial character in shift sequence";
                        goto utf7Error;
                    }
                    else {
                        /* Some bits remain; they should be zero */
                        if (base64buffer != 0) {
                            errmsg = "non-zero padding bits in shift sequence";
                            goto utf7Error;
                        }
                    }
                }
                if (ch != '-') {
                    /* '-' is absorbed; other terminating
                       characters are preserved */
                    *p++ = ch;
                }
            }
        }
        else if ( ch == '+' ) {
            startinpos = s-starts;
            s++; /* consume '+' */
            if (s < e && *s == '-') { /* '+-' encodes '+' */
                s++;
                *p++ = '+';
            }
            else { /* begin base64-encoded section */
                inShift = 1;
                shiftOutStart = p;
                base64bits = 0;
            }
        }
        else if (DECODE_DIRECT(ch)) { /* character decodes as itself */
            *p++ = ch;
            s++;
        }
        else {
            startinpos = s-starts;
            s++;
            errmsg = "unexpected special character";
            goto utf7Error;
        }
        continue;
utf7Error:
        outpos = p-PyUnicode_AS_UNICODE(unicode);
        endinpos = s-starts;
        if (unicode_decode_call_errorhandler(
                errors, &errorHandler,
                "utf7", errmsg,
                &starts, &e, &startinpos, &endinpos, &exc, &s,
                &unicode, &outpos, &p))
            goto onError;
    }

    /* end of string */

    if (inShift && !consumed) { /* in shift sequence, no more to follow */
        /* if we're in an inconsistent state, that's an error */
        if (surrogate ||
                (base64bits >= 6) ||
                (base64bits > 0 && base64buffer != 0)) {
            outpos = p-PyUnicode_AS_UNICODE(unicode);
            endinpos = size;
            if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "utf7", "unterminated shift sequence",
                    &starts, &e, &startinpos, &endinpos, &exc, &s,
                    &unicode, &outpos, &p))
                goto onError;
            if (s < e)
                goto restart;
        }
    }

    /* return state */
    if (consumed) {
        if (inShift) {
            p = shiftOutStart; /* back off output */
            *consumed = startinpos;
        }
        else {
            *consumed = s-starts;
        }
    }

    if (PyUnicode_Resize((PyObject**)&unicode, p - PyUnicode_AS_UNICODE(unicode)) < 0)
        goto onError;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&unicode)) {
        Py_DECREF(unicode);
        return NULL;
    }
    return (PyObject *)unicode;

  onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_DECREF(unicode);
    return NULL;
}


PyObject *
PyUnicode_EncodeUTF7(const Py_UNICODE *s,
                     Py_ssize_t size,
                     int base64SetO,
                     int base64WhiteSpace,
                     const char *errors)
{
    PyObject *v;
    /* It might be possible to tighten this worst case */
    Py_ssize_t allocated = 8 * size;
    int inShift = 0;
    Py_ssize_t i = 0;
    unsigned int base64bits = 0;
    unsigned long base64buffer = 0;
    char * out;
    char * start;

    if (size == 0)
        return PyBytes_FromStringAndSize(NULL, 0);

    if (allocated / 8 != size)
        return PyErr_NoMemory();

    v = PyBytes_FromStringAndSize(NULL, allocated);
    if (v == NULL)
        return NULL;

    start = out = PyBytes_AS_STRING(v);
    for (;i < size; ++i) {
        Py_UNICODE ch = s[i];

        if (inShift) {
            if (ENCODE_DIRECT(ch, !base64SetO, !base64WhiteSpace)) {
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
            else if (ENCODE_DIRECT(ch, !base64SetO, !base64WhiteSpace)) {
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
#ifdef Py_UNICODE_WIDE
        if (ch >= 0x10000) {
            /* code first surrogate */
            base64bits += 16;
            base64buffer = (base64buffer << 16) | 0xd800 | ((ch-0x10000) >> 10);
            while (base64bits >= 6) {
                *out++ = TO_BASE64(base64buffer >> (base64bits-6));
                base64bits -= 6;
            }
            /* prepare second surrogate */
            ch =  0xDC00 | ((ch-0x10000) & 0x3FF);
        }
#endif
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
    if (_PyBytes_Resize(&v, out - start) < 0)
        return NULL;
    return v;
}

#undef IS_BASE64
#undef FROM_BASE64
#undef TO_BASE64
#undef DECODE_DIRECT
#undef ENCODE_DIRECT

/* --- UTF-8 Codec -------------------------------------------------------- */

static
char utf8_code_length[256] = {
    /* Map UTF-8 encoded prefix byte to sequence length.  Zero means
       illegal prefix.  See RFC 3629 for details */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00-0F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 70-7F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80-8F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* B0-BF */
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* C0-C1 + C2-CF */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* D0-DF */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* E0-EF */
    4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* F0-F4 + F5-FF */
};

PyObject *
PyUnicode_DecodeUTF8(const char *s,
                     Py_ssize_t size,
                     const char *errors)
{
    return PyUnicode_DecodeUTF8Stateful(s, size, errors, NULL);
}

/* Mask to check or force alignment of a pointer to C 'long' boundaries */
#define LONG_PTR_MASK (size_t) (SIZEOF_LONG - 1)

/* Mask to quickly check whether a C 'long' contains a
   non-ASCII, UTF8-encoded char. */
#if (SIZEOF_LONG == 8)
# define ASCII_CHAR_MASK 0x8080808080808080L
#elif (SIZEOF_LONG == 4)
# define ASCII_CHAR_MASK 0x80808080L
#else
# error C 'long' size should be either 4 or 8!
#endif

/* Scans a UTF-8 string and returns the maximum character to be expected,
   the size of the decoded unicode string and if any major errors were
   encountered.

   This function does check basic UTF-8 sanity, it does however NOT CHECK
   if the string contains surrogates, and if all continuation bytes are
   within the correct ranges, these checks are performed in
   PyUnicode_DecodeUTF8Stateful.

   If it sets has_errors to 1, it means the value of unicode_size and max_char
   will be bogus and you should not rely on useful information in them.
   */
static Py_UCS4
utf8_max_char_size_and_has_errors(const char *s, Py_ssize_t string_size,
                                  Py_ssize_t *unicode_size, Py_ssize_t* consumed,
                                  int *has_errors)
{
    Py_ssize_t n;
    Py_ssize_t char_count = 0;
    Py_UCS4 max_char = 127, new_max;
    Py_UCS4 upper_bound;
    const unsigned char *p = (const unsigned char *)s;
    const unsigned char *end = p + string_size;
    const unsigned char *aligned_end = (const unsigned char *) ((size_t) end & ~LONG_PTR_MASK);
    int err = 0;

    for (; p < end && !err; ++p, ++char_count) {
        /* Only check value if it's not a ASCII char... */
        if (*p < 0x80) {
            /* Fast path, see below in PyUnicode_DecodeUTF8Stateful for
               an explanation. */
            if (!((size_t) p & LONG_PTR_MASK)) {
                /* Help register allocation */
                register const unsigned char *_p = p;
                while (_p < aligned_end) {
                    unsigned long value = *(unsigned long *) _p;
                    if (value & ASCII_CHAR_MASK)
                        break;
                    _p += SIZEOF_LONG;
                    char_count += SIZEOF_LONG;
                }
                p = _p;
                if (p == end)
                    break;
            }
        }
        if (*p >= 0x80) {
            n = utf8_code_length[*p];
            new_max = max_char;
            switch (n) {
            /* invalid start byte */
            case 0:
                err = 1;
                break;
            case 2:
                /* Code points between 0x00FF and 0x07FF inclusive.
                   Approximate the upper bound of the code point,
                   if this flips over 255 we can be sure it will be more
                   than 255 and the string will need 2 bytes per code coint,
                   if it stays under or equal to 255, we can be sure 1 byte
                   is enough.
                   ((*p & 0b00011111) << 6) | 0b00111111 */
                upper_bound = ((*p & 0x1F) << 6) | 0x3F;
                if (max_char < upper_bound)
                    new_max = upper_bound;
                /* Ensure we track at least that we left ASCII space. */
                if (new_max < 128)
                    new_max = 128;
                break;
            case 3:
                /* Between 0x0FFF and 0xFFFF inclusive, so values are
                   always > 255 and <= 65535 and will always need 2 bytes. */
                if (max_char < 65535)
                    new_max = 65535;
                break;
            case 4:
                /* Code point will be above 0xFFFF for sure in this case. */
                new_max = 65537;
                break;
            /* Internal error, this should be caught by the first if */
            case 1:
            default:
                assert(0 && "Impossible case in utf8_max_char_and_size");
                err = 1;
            }
            /* Instead of number of overall bytes for this code point,
               n containts the number of following bytes: */
            --n;
            /* Check if the follow up chars are all valid continuation bytes */
            if (n >= 1) {
                const unsigned char *cont;
                if ((p + n) >= end) {
                    if (consumed == 0)
                        /* incomplete data, non-incremental decoding */
                        err = 1;
                    break;
                }
                for (cont = p + 1; cont < (p + n); ++cont) {
                    if ((*cont & 0xc0) != 0x80) {
                        err = 1;
                        break;
                    }
                }
                p += n;
            }
            else
                err = 1;
            max_char = new_max;
        }
    }

    if (unicode_size)
        *unicode_size = char_count;
    if (has_errors)
        *has_errors = err;
    return max_char;
}

/* Similar to PyUnicode_WRITE but can also write into wstr field
   of the legacy unicode representation */
#define WRITE_FLEXIBLE_OR_WSTR(kind, buf, index, value) \
    do { \
        const int k_ = (kind); \
        if (k_ == PyUnicode_WCHAR_KIND) \
            ((Py_UNICODE *)(buf))[(index)] = (Py_UNICODE)(value); \
        else if (k_ == PyUnicode_1BYTE_KIND) \
            ((unsigned char *)(buf))[(index)] = (unsigned char)(value); \
        else if (k_ == PyUnicode_2BYTE_KIND) \
            ((Py_UCS2 *)(buf))[(index)] = (Py_UCS2)(value); \
        else \
            ((Py_UCS4 *)(buf))[(index)] = (Py_UCS4)(value); \
    } while (0)

PyObject *
PyUnicode_DecodeUTF8Stateful(const char *s,
                             Py_ssize_t size,
                             const char *errors,
                             Py_ssize_t *consumed)
{
    const char *starts = s;
    int n;
    int k;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    const char *e, *aligned_end;
    PyUnicodeObject *unicode;
    const char *errmsg = "";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    Py_UCS4 maxchar = 0;
    Py_ssize_t unicode_size;
    Py_ssize_t i;
    int kind;
    void *data;
    int has_errors;
    Py_UNICODE *error_outptr;
#if SIZEOF_WCHAR_T == 2
    Py_ssize_t wchar_offset = 0;
#endif

    if (size == 0) {
        if (consumed)
            *consumed = 0;
        return (PyObject *)PyUnicode_New(0, 0);
    }
    maxchar = utf8_max_char_size_and_has_errors(s, size, &unicode_size,
                                                consumed, &has_errors);
    if (has_errors) {
        unicode = _PyUnicode_New(size);
        if (!unicode)
            return NULL;
        kind = PyUnicode_WCHAR_KIND;
        data = PyUnicode_AS_UNICODE(unicode);
        assert(data != NULL);
    }
    else {
        unicode = (PyUnicodeObject *)PyUnicode_New(unicode_size, maxchar);
        if (!unicode)
            return NULL;
        /* When the string is ASCII only, just use memcpy and return.
           unicode_size may be != size if there is an incomplete UTF-8
           sequence at the end of the ASCII block.  */
        if (maxchar < 128 && size == unicode_size) {
            Py_MEMCPY(PyUnicode_1BYTE_DATA(unicode), s, unicode_size);
            return (PyObject *)unicode;
        }
        kind = PyUnicode_KIND(unicode);
        data = PyUnicode_DATA(unicode);
    }
    /* Unpack UTF-8 encoded data */
    i = 0;
    e = s + size;
    aligned_end = (const char *) ((size_t) e & ~LONG_PTR_MASK);

    while (s < e) {
        Py_UCS4 ch = (unsigned char)*s;

        if (ch < 0x80) {
            /* Fast path for runs of ASCII characters. Given that common UTF-8
               input will consist of an overwhelming majority of ASCII
               characters, we try to optimize for this case by checking
               as many characters as a C 'long' can contain.
               First, check if we can do an aligned read, as most CPUs have
               a penalty for unaligned reads.
            */
            if (!((size_t) s & LONG_PTR_MASK)) {
                /* Help register allocation */
                register const char *_s = s;
                register Py_ssize_t _i = i;
                while (_s < aligned_end) {
                    /* Read a whole long at a time (either 4 or 8 bytes),
                       and do a fast unrolled copy if it only contains ASCII
                       characters. */
                    unsigned long value = *(unsigned long *) _s;
                    if (value & ASCII_CHAR_MASK)
                        break;
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+0, _s[0]);
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+1, _s[1]);
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+2, _s[2]);
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+3, _s[3]);
#if (SIZEOF_LONG == 8)
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+4, _s[4]);
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+5, _s[5]);
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+6, _s[6]);
                    WRITE_FLEXIBLE_OR_WSTR(kind, data, _i+7, _s[7]);
#endif
                    _s += SIZEOF_LONG;
                    _i += SIZEOF_LONG;
                }
                s = _s;
                i = _i;
                if (s == e)
                    break;
                ch = (unsigned char)*s;
            }
        }

        if (ch < 0x80) {
            WRITE_FLEXIBLE_OR_WSTR(kind, data, i++, ch);
            s++;
            continue;
        }

        n = utf8_code_length[ch];

        if (s + n > e) {
            if (consumed)
                break;
            else {
                errmsg = "unexpected end of data";
                startinpos = s-starts;
                endinpos = startinpos+1;
                for (k=1; (k < size-startinpos) && ((s[k]&0xC0) == 0x80); k++)
                    endinpos++;
                goto utf8Error;
            }
        }

        switch (n) {

        case 0:
            errmsg = "invalid start byte";
            startinpos = s-starts;
            endinpos = startinpos+1;
            goto utf8Error;

        case 1:
            errmsg = "internal error";
            startinpos = s-starts;
            endinpos = startinpos+1;
            goto utf8Error;

        case 2:
            if ((s[1] & 0xc0) != 0x80) {
                errmsg = "invalid continuation byte";
                startinpos = s-starts;
                endinpos = startinpos + 1;
                goto utf8Error;
            }
            ch = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
            assert ((ch > 0x007F) && (ch <= 0x07FF));
            WRITE_FLEXIBLE_OR_WSTR(kind, data, i++, ch);
            break;

        case 3:
            /* Decoding UTF-8 sequences in range \xed\xa0\x80-\xed\xbf\xbf
               will result in surrogates in range d800-dfff. Surrogates are
               not valid UTF-8 so they are rejected.
               See http://www.unicode.org/versions/Unicode5.2.0/ch03.pdf
               (table 3-7) and http://www.rfc-editor.org/rfc/rfc3629.txt */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                ((unsigned char)s[0] == 0xE0 &&
                 (unsigned char)s[1] < 0xA0) ||
                ((unsigned char)s[0] == 0xED &&
                 (unsigned char)s[1] > 0x9F)) {
                errmsg = "invalid continuation byte";
                startinpos = s-starts;
                endinpos = startinpos + 1;

                /* if s[1] first two bits are 1 and 0, then the invalid
                   continuation byte is s[2], so increment endinpos by 1,
                   if not, s[1] is invalid and endinpos doesn't need to
                   be incremented. */
                if ((s[1] & 0xC0) == 0x80)
                    endinpos++;
                goto utf8Error;
            }
            ch = ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + (s[2] & 0x3f);
            assert ((ch > 0x07FF) && (ch <= 0xFFFF));
            WRITE_FLEXIBLE_OR_WSTR(kind, data, i++, ch);
            break;

        case 4:
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80 ||
                ((unsigned char)s[0] == 0xF0 &&
                 (unsigned char)s[1] < 0x90) ||
                ((unsigned char)s[0] == 0xF4 &&
                 (unsigned char)s[1] > 0x8F)) {
                errmsg = "invalid continuation byte";
                startinpos = s-starts;
                endinpos = startinpos + 1;
                if ((s[1] & 0xC0) == 0x80) {
                    endinpos++;
                    if ((s[2] & 0xC0) == 0x80)
                        endinpos++;
                }
                goto utf8Error;
            }
            ch = ((s[0] & 0x7) << 18) + ((s[1] & 0x3f) << 12) +
                 ((s[2] & 0x3f) << 6) + (s[3] & 0x3f);
            assert ((ch > 0xFFFF) && (ch <= 0x10ffff));

            /* If the string is flexible or we have native UCS-4, write
               directly.. */
            if (sizeof(Py_UNICODE) > 2 || kind != PyUnicode_WCHAR_KIND)
                WRITE_FLEXIBLE_OR_WSTR(kind, data, i++, ch);

            else {
                /* compute and append the two surrogates: */

                /* translate from 10000..10FFFF to 0..FFFF */
                ch -= 0x10000;

                /* high surrogate = top 10 bits added to D800 */
                WRITE_FLEXIBLE_OR_WSTR(kind, data, i++,
                                       (Py_UNICODE)(0xD800 + (ch >> 10)));

                /* low surrogate = bottom 10 bits added to DC00 */
                WRITE_FLEXIBLE_OR_WSTR(kind, data, i++,
                                       (Py_UNICODE)(0xDC00 + (ch & 0x03FF)));
            }
#if SIZEOF_WCHAR_T == 2
            wchar_offset++;
#endif
            break;
        }
        s += n;
        continue;

      utf8Error:
        /* If this is not yet a resizable string, make it one.. */
        if (kind != PyUnicode_WCHAR_KIND) {
            const Py_UNICODE *u;
            PyUnicodeObject *new_unicode = _PyUnicode_New(size);
            if (!new_unicode)
                goto onError;
            u = PyUnicode_AsUnicode((PyObject *)unicode);
            if (!u)
                goto onError;
#if SIZEOF_WCHAR_T == 2
            i += wchar_offset;
#endif
            Py_UNICODE_COPY(PyUnicode_AS_UNICODE(new_unicode), u, i);
            Py_DECREF(unicode);
            unicode = new_unicode;
            kind = 0;
            data = PyUnicode_AS_UNICODE(new_unicode);
            assert(data != NULL);
        }
        error_outptr = PyUnicode_AS_UNICODE(unicode) + i;
        if (unicode_decode_call_errorhandler(
                errors, &errorHandler,
                "utf8", errmsg,
                &starts, &e, &startinpos, &endinpos, &exc, &s,
                &unicode, &i, &error_outptr))
            goto onError;
        /* Update data because unicode_decode_call_errorhandler might have
           re-created or resized the unicode object. */
        data = PyUnicode_AS_UNICODE(unicode);
        aligned_end = (const char *) ((size_t) e & ~LONG_PTR_MASK);
    }
    /* Ensure the unicode_size calculation above was correct: */
    assert(kind == PyUnicode_WCHAR_KIND || i == unicode_size);

    if (consumed)
        *consumed = s-starts;

    /* Adjust length and ready string when it contained errors and
       is of the old resizable kind. */
    if (kind == PyUnicode_WCHAR_KIND) {
        if (PyUnicode_Resize((PyObject**)&unicode, i) < 0)
            goto onError;
    }

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&unicode)) {
        Py_DECREF(unicode);
        return NULL;
    }
    return (PyObject *)unicode;

  onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_DECREF(unicode);
    return NULL;
}

#undef WRITE_FLEXIBLE_OR_WSTR

#ifdef __APPLE__

/* Simplified UTF-8 decoder using surrogateescape error handler,
   used to decode the command line arguments on Mac OS X. */

wchar_t*
_Py_DecodeUTF8_surrogateescape(const char *s, Py_ssize_t size)
{
    int n;
    const char *e;
    wchar_t *unicode, *p;

    /* Note: size will always be longer than the resulting Unicode
       character count */
    if (PY_SSIZE_T_MAX / sizeof(wchar_t) < (size + 1)) {
        PyErr_NoMemory();
        return NULL;
    }
    unicode = PyMem_Malloc((size + 1) * sizeof(wchar_t));
    if (!unicode)
        return NULL;

    /* Unpack UTF-8 encoded data */
    p = unicode;
    e = s + size;
    while (s < e) {
        Py_UCS4 ch = (unsigned char)*s;

        if (ch < 0x80) {
            *p++ = (wchar_t)ch;
            s++;
            continue;
        }

        n = utf8_code_length[ch];
        if (s + n > e) {
            goto surrogateescape;
        }

        switch (n) {
        case 0:
        case 1:
            goto surrogateescape;

        case 2:
            if ((s[1] & 0xc0) != 0x80)
                goto surrogateescape;
            ch = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
            assert ((ch > 0x007F) && (ch <= 0x07FF));
            *p++ = (wchar_t)ch;
            break;

        case 3:
            /* Decoding UTF-8 sequences in range \xed\xa0\x80-\xed\xbf\xbf
               will result in surrogates in range d800-dfff. Surrogates are
               not valid UTF-8 so they are rejected.
               See http://www.unicode.org/versions/Unicode5.2.0/ch03.pdf
               (table 3-7) and http://www.rfc-editor.org/rfc/rfc3629.txt */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                ((unsigned char)s[0] == 0xE0 &&
                 (unsigned char)s[1] < 0xA0) ||
                ((unsigned char)s[0] == 0xED &&
                 (unsigned char)s[1] > 0x9F)) {

                goto surrogateescape;
            }
            ch = ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + (s[2] & 0x3f);
            assert ((ch > 0x07FF) && (ch <= 0xFFFF));
            *p++ = (wchar_t)ch;
            break;

        case 4:
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80 ||
                ((unsigned char)s[0] == 0xF0 &&
                 (unsigned char)s[1] < 0x90) ||
                ((unsigned char)s[0] == 0xF4 &&
                 (unsigned char)s[1] > 0x8F)) {
                goto surrogateescape;
            }
            ch = ((s[0] & 0x7) << 18) + ((s[1] & 0x3f) << 12) +
                 ((s[2] & 0x3f) << 6) + (s[3] & 0x3f);
            assert ((ch > 0xFFFF) && (ch <= 0x10ffff));

#if SIZEOF_WCHAR_T == 4
            *p++ = (wchar_t)ch;
#else
            /*  compute and append the two surrogates: */

            /*  translate from 10000..10FFFF to 0..FFFF */
            ch -= 0x10000;

            /*  high surrogate = top 10 bits added to D800 */
            *p++ = (wchar_t)(0xD800 + (ch >> 10));

            /*  low surrogate = bottom 10 bits added to DC00 */
            *p++ = (wchar_t)(0xDC00 + (ch & 0x03FF));
#endif
            break;
        }
        s += n;
        continue;

      surrogateescape:
        *p++ = 0xDC00 + ch;
        s++;
    }
    *p = L'\0';
    return unicode;
}

#endif /* __APPLE__ */

/* Primary internal function which creates utf8 encoded bytes objects.

   Allocation strategy:  if the string is short, convert into a stack buffer
   and allocate exactly as much space needed at the end.  Else allocate the
   maximum possible needed (4 result bytes per Unicode character), and return
   the excess memory at the end.
*/
PyObject *
_PyUnicode_AsUTF8String(PyObject *obj, const char *errors)
{
#define MAX_SHORT_UNICHARS 300  /* largest size we'll do on the stack */

    Py_ssize_t i;                /* index into s of next input byte */
    PyObject *result;            /* result string object */
    char *p;                     /* next free byte in output buffer */
    Py_ssize_t nallocated;      /* number of result bytes allocated */
    Py_ssize_t nneeded;            /* number of result bytes needed */
    char stackbuf[MAX_SHORT_UNICHARS * 4];
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    int kind;
    void *data;
    Py_ssize_t size;
    PyUnicodeObject *unicode = (PyUnicodeObject *)obj;
#if SIZEOF_WCHAR_T == 2
    Py_ssize_t wchar_offset = 0;
#endif

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    if (PyUnicode_READY(unicode) == -1)
        return NULL;

    if (PyUnicode_UTF8(unicode))
        return PyBytes_FromStringAndSize(PyUnicode_UTF8(unicode),
                                         PyUnicode_UTF8_LENGTH(unicode));

    kind = PyUnicode_KIND(unicode);
    data = PyUnicode_DATA(unicode);
    size = PyUnicode_GET_LENGTH(unicode);

    assert(size >= 0);

    if (size <= MAX_SHORT_UNICHARS) {
        /* Write into the stack buffer; nallocated can't overflow.
         * At the end, we'll allocate exactly as much heap space as it
         * turns out we need.
         */
        nallocated = Py_SAFE_DOWNCAST(sizeof(stackbuf), size_t, int);
        result = NULL;   /* will allocate after we're done */
        p = stackbuf;
    }
    else {
        /* Overallocate on the heap, and give the excess back at the end. */
        nallocated = size * 4;
        if (nallocated / 4 != size)  /* overflow! */
            return PyErr_NoMemory();
        result = PyBytes_FromStringAndSize(NULL, nallocated);
        if (result == NULL)
            return NULL;
        p = PyBytes_AS_STRING(result);
    }

    for (i = 0; i < size;) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i++);

        if (ch < 0x80)
            /* Encode ASCII */
            *p++ = (char) ch;

        else if (ch < 0x0800) {
            /* Encode Latin-1 */
            *p++ = (char)(0xc0 | (ch >> 6));
            *p++ = (char)(0x80 | (ch & 0x3f));
        } else if (0xD800 <= ch && ch <= 0xDFFF) {
            Py_ssize_t newpos;
            PyObject *rep;
            Py_ssize_t repsize, k, startpos;
            startpos = i-1;
#if SIZEOF_WCHAR_T == 2
            startpos += wchar_offset;
#endif
            rep = unicode_encode_call_errorhandler(
                  errors, &errorHandler, "utf-8", "surrogates not allowed",
                  PyUnicode_AS_UNICODE(unicode), PyUnicode_GET_SIZE(unicode),
                  &exc, startpos, startpos+1, &newpos);
            if (!rep)
                goto error;

            if (PyBytes_Check(rep))
                repsize = PyBytes_GET_SIZE(rep);
            else
                repsize = PyUnicode_GET_SIZE(rep);

            if (repsize > 4) {
                Py_ssize_t offset;

                if (result == NULL)
                    offset = p - stackbuf;
                else
                    offset = p - PyBytes_AS_STRING(result);

                if (nallocated > PY_SSIZE_T_MAX - repsize + 4) {
                    /* integer overflow */
                    PyErr_NoMemory();
                    goto error;
                }
                nallocated += repsize - 4;
                if (result != NULL) {
                    if (_PyBytes_Resize(&result, nallocated) < 0)
                        goto error;
                } else {
                    result = PyBytes_FromStringAndSize(NULL, nallocated);
                    if (result == NULL)
                        goto error;
                    Py_MEMCPY(PyBytes_AS_STRING(result), stackbuf, offset);
                }
                p = PyBytes_AS_STRING(result) + offset;
            }

            if (PyBytes_Check(rep)) {
                char *prep = PyBytes_AS_STRING(rep);
                for(k = repsize; k > 0; k--)
                    *p++ = *prep++;
            } else /* rep is unicode */ {
                const Py_UNICODE *prep = PyUnicode_AS_UNICODE(rep);
                Py_UNICODE c;

                for(k=0; k<repsize; k++) {
                    c = prep[k];
                    if (0x80 <= c) {
                        raise_encode_exception(&exc, "utf-8",
                                               PyUnicode_AS_UNICODE(unicode),
                                               size, i-1, i,
                                               "surrogates not allowed");
                        goto error;
                    }
                    *p++ = (char)prep[k];
                }
            }
            Py_DECREF(rep);
        } else if (ch < 0x10000) {
            *p++ = (char)(0xe0 | (ch >> 12));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        } else /* ch >= 0x10000 */ {
            /* Encode UCS4 Unicode ordinals */
            *p++ = (char)(0xf0 | (ch >> 18));
            *p++ = (char)(0x80 | ((ch >> 12) & 0x3f));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
#if SIZEOF_WCHAR_T == 2
            wchar_offset++;
#endif
        }
    }

    if (result == NULL) {
        /* This was stack allocated. */
        nneeded = p - stackbuf;
        assert(nneeded <= nallocated);
        result = PyBytes_FromStringAndSize(stackbuf, nneeded);
    }
    else {
        /* Cut back to size actually needed. */
        nneeded = p - PyBytes_AS_STRING(result);
        assert(nneeded <= nallocated);
        _PyBytes_Resize(&result, nneeded);
    }

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return result;
 error:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_XDECREF(result);
    return NULL;

#undef MAX_SHORT_UNICHARS
}

PyObject *
PyUnicode_EncodeUTF8(const Py_UNICODE *s,
                     Py_ssize_t size,
                     const char *errors)
{
    PyObject *v, *unicode;

    unicode = PyUnicode_FromUnicode(s, size);
    if (unicode == NULL)
        return NULL;
    v = _PyUnicode_AsUTF8String(unicode, errors);
    Py_DECREF(unicode);
    return v;
}

PyObject *
PyUnicode_AsUTF8String(PyObject *unicode)
{
    return _PyUnicode_AsUTF8String(unicode, NULL);
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
    Py_ssize_t outpos;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
#ifndef Py_UNICODE_WIDE
    int pairs = 0;
    const unsigned char *qq;
#else
    const int pairs = 0;
#endif
    const unsigned char *q, *e;
    int bo = 0;       /* assume native ordering by default */
    const char *errmsg = "";
    /* Offsets from q for retrieving bytes in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int iorder[] = {0, 1, 2, 3};
#else
    int iorder[] = {3, 2, 1, 0};
#endif
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    q = (unsigned char *)s;
    e = q + size;

    if (byteorder)
        bo = *byteorder;

    /* Check for BOM marks (U+FEFF) in the input and adjust current
       byte order setting accordingly. In native mode, the leading BOM
       mark is skipped, in all other modes, it is copied to the output
       stream as-is (giving a ZWNBSP character). */
    if (bo == 0) {
        if (size >= 4) {
            const Py_UCS4 bom = (q[iorder[3]] << 24) | (q[iorder[2]] << 16) |
                (q[iorder[1]] << 8) | q[iorder[0]];
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
            if (bom == 0x0000FEFF) {
                q += 4;
                bo = -1;
            }
            else if (bom == 0xFFFE0000) {
                q += 4;
                bo = 1;
            }
#else
            if (bom == 0x0000FEFF) {
                q += 4;
                bo = 1;
            }
            else if (bom == 0xFFFE0000) {
                q += 4;
                bo = -1;
            }
#endif
        }
    }

    if (bo == -1) {
        /* force LE */
        iorder[0] = 0;
        iorder[1] = 1;
        iorder[2] = 2;
        iorder[3] = 3;
    }
    else if (bo == 1) {
        /* force BE */
        iorder[0] = 3;
        iorder[1] = 2;
        iorder[2] = 1;
        iorder[3] = 0;
    }

    /* On narrow builds we split characters outside the BMP into two
       codepoints => count how much extra space we need. */
#ifndef Py_UNICODE_WIDE
    for (qq = q; qq < e; qq += 4)
        if (qq[iorder[2]] != 0 || qq[iorder[3]] != 0)
            pairs++;
#endif

    /* This might be one to much, because of a BOM */
    unicode = _PyUnicode_New((size+3)/4+pairs);
    if (!unicode)
        return NULL;
    if (size == 0)
        return (PyObject *)unicode;

    /* Unpack UTF-32 encoded data */
    p = PyUnicode_AS_UNICODE(unicode);

    while (q < e) {
        Py_UCS4 ch;
        /* remaining bytes at the end? (size should be divisible by 4) */
        if (e-q<4) {
            if (consumed)
                break;
            errmsg = "truncated data";
            startinpos = ((const char *)q)-starts;
            endinpos = ((const char *)e)-starts;
            goto utf32Error;
            /* The remaining input chars are ignored if the callback
               chooses to skip the input */
        }
        ch = (q[iorder[3]] << 24) | (q[iorder[2]] << 16) |
            (q[iorder[1]] << 8) | q[iorder[0]];

        if (ch >= 0x110000)
        {
            errmsg = "codepoint not in range(0x110000)";
            startinpos = ((const char *)q)-starts;
            endinpos = startinpos+4;
            goto utf32Error;
        }
#ifndef Py_UNICODE_WIDE
        if (ch >= 0x10000)
        {
            *p++ = 0xD800 | ((ch-0x10000) >> 10);
            *p++ = 0xDC00 | ((ch-0x10000) & 0x3FF);
        }
        else
#endif
            *p++ = ch;
        q += 4;
        continue;
      utf32Error:
        outpos = p-PyUnicode_AS_UNICODE(unicode);
        if (unicode_decode_call_errorhandler(
                errors, &errorHandler,
                "utf32", errmsg,
                &starts, (const char **)&e, &startinpos, &endinpos, &exc, (const char **)&q,
                &unicode, &outpos, &p))
            goto onError;
    }

    if (byteorder)
        *byteorder = bo;

    if (consumed)
        *consumed = (const char *)q-starts;

    /* Adjust length */
    if (PyUnicode_Resize((PyObject**)&unicode, p - PyUnicode_AS_UNICODE(unicode)) < 0)
        goto onError;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&unicode)) {
        Py_DECREF(unicode);
        return NULL;
    }
    return (PyObject *)unicode;

  onError:
    Py_DECREF(unicode);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *
PyUnicode_EncodeUTF32(const Py_UNICODE *s,
                      Py_ssize_t size,
                      const char *errors,
                      int byteorder)
{
    PyObject *v;
    unsigned char *p;
    Py_ssize_t nsize, bytesize;
#ifndef Py_UNICODE_WIDE
    Py_ssize_t i, pairs;
#else
    const int pairs = 0;
#endif
    /* Offsets from p for storing byte pairs in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int iorder[] = {0, 1, 2, 3};
#else
    int iorder[] = {3, 2, 1, 0};
#endif

#define STORECHAR(CH)                           \
    do {                                        \
        p[iorder[3]] = ((CH) >> 24) & 0xff;     \
        p[iorder[2]] = ((CH) >> 16) & 0xff;     \
        p[iorder[1]] = ((CH) >> 8) & 0xff;      \
        p[iorder[0]] = (CH) & 0xff;             \
        p += 4;                                 \
    } while(0)

    /* In narrow builds we can output surrogate pairs as one codepoint,
       so we need less space. */
#ifndef Py_UNICODE_WIDE
    for (i = pairs = 0; i < size-1; i++)
        if (0xD800 <= s[i] && s[i] <= 0xDBFF &&
            0xDC00 <= s[i+1] && s[i+1] <= 0xDFFF)
            pairs++;
#endif
    nsize = (size - pairs + (byteorder == 0));
    bytesize = nsize * 4;
    if (bytesize / 4 != nsize)
        return PyErr_NoMemory();
    v = PyBytes_FromStringAndSize(NULL, bytesize);
    if (v == NULL)
        return NULL;

    p = (unsigned char *)PyBytes_AS_STRING(v);
    if (byteorder == 0)
        STORECHAR(0xFEFF);
    if (size == 0)
        goto done;

    if (byteorder == -1) {
        /* force LE */
        iorder[0] = 0;
        iorder[1] = 1;
        iorder[2] = 2;
        iorder[3] = 3;
    }
    else if (byteorder == 1) {
        /* force BE */
        iorder[0] = 3;
        iorder[1] = 2;
        iorder[2] = 1;
        iorder[3] = 0;
    }

    while (size-- > 0) {
        Py_UCS4 ch = *s++;
#ifndef Py_UNICODE_WIDE
        if (0xD800 <= ch && ch <= 0xDBFF && size > 0) {
            Py_UCS4 ch2 = *s;
            if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
                ch = (((ch & 0x3FF)<<10) | (ch2 & 0x3FF)) + 0x10000;
                s++;
                size--;
            }
        }
#endif
        STORECHAR(ch);
    }

  done:
    return v;
#undef STORECHAR
}

PyObject *
PyUnicode_AsUTF32String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUTF32(PyUnicode_AS_UNICODE(unicode),
                                 PyUnicode_GET_SIZE(unicode),
                                 NULL,
                                 0);
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

/* Two masks for fast checking of whether a C 'long' may contain
   UTF16-encoded surrogate characters. This is an efficient heuristic,
   assuming that non-surrogate characters with a code point >= 0x8000 are
   rare in most input.
   FAST_CHAR_MASK is used when the input is in native byte ordering,
   SWAPPED_FAST_CHAR_MASK when the input is in byteswapped ordering.
*/
#if (SIZEOF_LONG == 8)
# define FAST_CHAR_MASK         0x8000800080008000L
# define SWAPPED_FAST_CHAR_MASK 0x0080008000800080L
#elif (SIZEOF_LONG == 4)
# define FAST_CHAR_MASK         0x80008000L
# define SWAPPED_FAST_CHAR_MASK 0x00800080L
#else
# error C 'long' size should be either 4 or 8!
#endif

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
    Py_ssize_t outpos;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const unsigned char *q, *e, *aligned_end;
    int bo = 0;       /* assume native ordering by default */
    int native_ordering = 0;
    const char *errmsg = "";
    /* Offsets from q for retrieving byte pairs in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int ihi = 1, ilo = 0;
#else
    int ihi = 0, ilo = 1;
#endif
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    /* Note: size will always be longer than the resulting Unicode
       character count */
    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;
    if (size == 0)
        return (PyObject *)unicode;

    /* Unpack UTF-16 encoded data */
    p = PyUnicode_AS_UNICODE(unicode);
    q = (unsigned char *)s;
    e = q + size - 1;

    if (byteorder)
        bo = *byteorder;

    /* Check for BOM marks (U+FEFF) in the input and adjust current
       byte order setting accordingly. In native mode, the leading BOM
       mark is skipped, in all other modes, it is copied to the output
       stream as-is (giving a ZWNBSP character). */
    if (bo == 0) {
        if (size >= 2) {
            const Py_UNICODE bom = (q[ihi] << 8) | q[ilo];
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
            if (bom == 0xFEFF) {
                q += 2;
                bo = -1;
            }
            else if (bom == 0xFFFE) {
                q += 2;
                bo = 1;
            }
#else
            if (bom == 0xFEFF) {
                q += 2;
                bo = 1;
            }
            else if (bom == 0xFFFE) {
                q += 2;
                bo = -1;
            }
#endif
        }
    }

    if (bo == -1) {
        /* force LE */
        ihi = 1;
        ilo = 0;
    }
    else if (bo == 1) {
        /* force BE */
        ihi = 0;
        ilo = 1;
    }
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    native_ordering = ilo < ihi;
#else
    native_ordering = ilo > ihi;
#endif

    aligned_end = (const unsigned char *) ((size_t) e & ~LONG_PTR_MASK);
    while (q < e) {
        Py_UNICODE ch;
        /* First check for possible aligned read of a C 'long'. Unaligned
           reads are more expensive, better to defer to another iteration. */
        if (!((size_t) q & LONG_PTR_MASK)) {
            /* Fast path for runs of non-surrogate chars. */
            register const unsigned char *_q = q;
            Py_UNICODE *_p = p;
            if (native_ordering) {
                /* Native ordering is simple: as long as the input cannot
                   possibly contain a surrogate char, do an unrolled copy
                   of several 16-bit code points to the target object.
                   The non-surrogate check is done on several input bytes
                   at a time (as many as a C 'long' can contain). */
                while (_q < aligned_end) {
                    unsigned long data = * (unsigned long *) _q;
                    if (data & FAST_CHAR_MASK)
                        break;
                    _p[0] = ((unsigned short *) _q)[0];
                    _p[1] = ((unsigned short *) _q)[1];
#if (SIZEOF_LONG == 8)
                    _p[2] = ((unsigned short *) _q)[2];
                    _p[3] = ((unsigned short *) _q)[3];
#endif
                    _q += SIZEOF_LONG;
                    _p += SIZEOF_LONG / 2;
                }
            }
            else {
                /* Byteswapped ordering is similar, but we must decompose
                   the copy bytewise, and take care of zero'ing out the
                   upper bytes if the target object is in 32-bit units
                   (that is, in UCS-4 builds). */
                while (_q < aligned_end) {
                    unsigned long data = * (unsigned long *) _q;
                    if (data & SWAPPED_FAST_CHAR_MASK)
                        break;
                    /* Zero upper bytes in UCS-4 builds */
#if (Py_UNICODE_SIZE > 2)
                    _p[0] = 0;
                    _p[1] = 0;
#if (SIZEOF_LONG == 8)
                    _p[2] = 0;
                    _p[3] = 0;
#endif
#endif
                    /* Issue #4916; UCS-4 builds on big endian machines must
                       fill the two last bytes of each 4-byte unit. */
#if (!defined(BYTEORDER_IS_LITTLE_ENDIAN) && Py_UNICODE_SIZE > 2)
# define OFF 2
#else
# define OFF 0
#endif
                    ((unsigned char *) _p)[OFF + 1] = _q[0];
                    ((unsigned char *) _p)[OFF + 0] = _q[1];
                    ((unsigned char *) _p)[OFF + 1 + Py_UNICODE_SIZE] = _q[2];
                    ((unsigned char *) _p)[OFF + 0 + Py_UNICODE_SIZE] = _q[3];
#if (SIZEOF_LONG == 8)
                    ((unsigned char *) _p)[OFF + 1 + 2 * Py_UNICODE_SIZE] = _q[4];
                    ((unsigned char *) _p)[OFF + 0 + 2 * Py_UNICODE_SIZE] = _q[5];
                    ((unsigned char *) _p)[OFF + 1 + 3 * Py_UNICODE_SIZE] = _q[6];
                    ((unsigned char *) _p)[OFF + 0 + 3 * Py_UNICODE_SIZE] = _q[7];
#endif
#undef OFF
                    _q += SIZEOF_LONG;
                    _p += SIZEOF_LONG / 2;
                }
            }
            p = _p;
            q = _q;
            if (q >= e)
                break;
        }
        ch = (q[ihi] << 8) | q[ilo];

        q += 2;

        if (ch < 0xD800 || ch > 0xDFFF) {
            *p++ = ch;
            continue;
        }

        /* UTF-16 code pair: */
        if (q > e) {
            errmsg = "unexpected end of data";
            startinpos = (((const char *)q) - 2) - starts;
            endinpos = ((const char *)e) + 1 - starts;
            goto utf16Error;
        }
        if (0xD800 <= ch && ch <= 0xDBFF) {
            Py_UNICODE ch2 = (q[ihi] << 8) | q[ilo];
            q += 2;
            if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
#ifndef Py_UNICODE_WIDE
                *p++ = ch;
                *p++ = ch2;
#else
                *p++ = (((ch & 0x3FF)<<10) | (ch2 & 0x3FF)) + 0x10000;
#endif
                continue;
            }
            else {
                errmsg = "illegal UTF-16 surrogate";
                startinpos = (((const char *)q)-4)-starts;
                endinpos = startinpos+2;
                goto utf16Error;
            }

        }
        errmsg = "illegal encoding";
        startinpos = (((const char *)q)-2)-starts;
        endinpos = startinpos+2;
        /* Fall through to report the error */

      utf16Error:
        outpos = p - PyUnicode_AS_UNICODE(unicode);
        if (unicode_decode_call_errorhandler(
                errors,
                &errorHandler,
                "utf16", errmsg,
                &starts,
                (const char **)&e,
                &startinpos,
                &endinpos,
                &exc,
                (const char **)&q,
                &unicode,
                &outpos,
                &p))
            goto onError;
    }
    /* remaining byte at the end? (size should be even) */
    if (e == q) {
        if (!consumed) {
            errmsg = "truncated data";
            startinpos = ((const char *)q) - starts;
            endinpos = ((const char *)e) + 1 - starts;
            outpos = p - PyUnicode_AS_UNICODE(unicode);
            if (unicode_decode_call_errorhandler(
                    errors,
                    &errorHandler,
                    "utf16", errmsg,
                    &starts,
                    (const char **)&e,
                    &startinpos,
                    &endinpos,
                    &exc,
                    (const char **)&q,
                    &unicode,
                    &outpos,
                    &p))
                goto onError;
            /* The remaining input chars are ignored if the callback
               chooses to skip the input */
        }
    }

    if (byteorder)
        *byteorder = bo;

    if (consumed)
        *consumed = (const char *)q-starts;

    /* Adjust length */
    if (PyUnicode_Resize((PyObject**)&unicode, p - PyUnicode_AS_UNICODE(unicode)) < 0)
        goto onError;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&unicode)) {
        Py_DECREF(unicode);
        return NULL;
    }
    return (PyObject *)unicode;

  onError:
    Py_DECREF(unicode);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

#undef FAST_CHAR_MASK
#undef SWAPPED_FAST_CHAR_MASK

PyObject *
PyUnicode_EncodeUTF16(const Py_UNICODE *s,
                      Py_ssize_t size,
                      const char *errors,
                      int byteorder)
{
    PyObject *v;
    unsigned char *p;
    Py_ssize_t nsize, bytesize;
#ifdef Py_UNICODE_WIDE
    Py_ssize_t i, pairs;
#else
    const int pairs = 0;
#endif
    /* Offsets from p for storing byte pairs in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int ihi = 1, ilo = 0;
#else
    int ihi = 0, ilo = 1;
#endif

#define STORECHAR(CH)                           \
    do {                                        \
        p[ihi] = ((CH) >> 8) & 0xff;            \
        p[ilo] = (CH) & 0xff;                   \
        p += 2;                                 \
    } while(0)

#ifdef Py_UNICODE_WIDE
    for (i = pairs = 0; i < size; i++)
        if (s[i] >= 0x10000)
            pairs++;
#endif
    /* 2 * (size + pairs + (byteorder == 0)) */
    if (size > PY_SSIZE_T_MAX ||
        size > PY_SSIZE_T_MAX - pairs - (byteorder == 0))
        return PyErr_NoMemory();
    nsize = size + pairs + (byteorder == 0);
    bytesize = nsize * 2;
    if (bytesize / 2 != nsize)
        return PyErr_NoMemory();
    v = PyBytes_FromStringAndSize(NULL, bytesize);
    if (v == NULL)
        return NULL;

    p = (unsigned char *)PyBytes_AS_STRING(v);
    if (byteorder == 0)
        STORECHAR(0xFEFF);
    if (size == 0)
        goto done;

    if (byteorder == -1) {
        /* force LE */
        ihi = 1;
        ilo = 0;
    }
    else if (byteorder == 1) {
        /* force BE */
        ihi = 0;
        ilo = 1;
    }

    while (size-- > 0) {
        Py_UNICODE ch = *s++;
        Py_UNICODE ch2 = 0;
#ifdef Py_UNICODE_WIDE
        if (ch >= 0x10000) {
            ch2 = 0xDC00 | ((ch-0x10000) & 0x3FF);
            ch  = 0xD800 | ((ch-0x10000) >> 10);
        }
#endif
        STORECHAR(ch);
        if (ch2)
            STORECHAR(ch2);
    }

  done:
    return v;
#undef STORECHAR
}

PyObject *
PyUnicode_AsUTF16String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(unicode),
                                 PyUnicode_GET_SIZE(unicode),
                                 NULL,
                                 0);
}

/* --- Unicode Escape Codec ----------------------------------------------- */

/* Helper function for PyUnicode_DecodeUnicodeEscape, determines
   if all the escapes in the string make it still a valid ASCII string.
   Returns -1 if any escapes were found which cause the string to
   pop out of ASCII range.  Otherwise returns the length of the
   required buffer to hold the string.
   */
Py_ssize_t
length_of_escaped_ascii_string(const char *s, Py_ssize_t size)
{
    const unsigned char *p = (const unsigned char *)s;
    const unsigned char *end = p + size;
    Py_ssize_t length = 0;

    if (size < 0)
        return -1;

    for (; p < end; ++p) {
        if (*p > 127) {
            /* Non-ASCII */
            return -1;
        }
        else if (*p != '\\') {
            /* Normal character */
            ++length;
        }
        else {
            /* Backslash-escape, check next char */
            ++p;
            /* Escape sequence reaches till end of string or
               non-ASCII follow-up. */
            if (p >= end || *p > 127)
                return -1;
            switch (*p) {
            case '\n':
                /* backslash + \n result in zero characters */
                break;
            case '\\': case '\'': case '\"':
            case 'b': case 'f': case 't':
            case 'n': case 'r': case 'v': case 'a':
                ++length;
                break;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
            case 'x': case 'u': case 'U': case 'N':
                /* these do not guarantee ASCII characters */
                return -1;
            default:
                /* count the backslash + the other character */
                length += 2;
            }
        }
    }
    return length;
}

/* Similar to PyUnicode_WRITE but either write into wstr field
   or treat string as ASCII. */
#define WRITE_ASCII_OR_WSTR(kind, buf, index, value) \
    do { \
        if ((kind) != PyUnicode_WCHAR_KIND) \
            ((unsigned char *)(buf))[(index)] = (unsigned char)(value); \
        else \
            ((Py_UNICODE *)(buf))[(index)] = (Py_UNICODE)(value); \
    } while (0)

#define WRITE_WSTR(buf, index, value) \
    assert(kind == PyUnicode_WCHAR_KIND), \
    ((Py_UNICODE *)(buf))[(index)] = (Py_UNICODE)(value)


static _PyUnicode_Name_CAPI *ucnhash_CAPI = NULL;

PyObject *
PyUnicode_DecodeUnicodeEscape(const char *s,
                              Py_ssize_t size,
                              const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    int j;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    const char *end;
    char* message;
    Py_UCS4 chr = 0xffffffff; /* in case 'getcode' messes up */
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    Py_ssize_t ascii_length;
    Py_ssize_t i;
    int kind;
    void *data;

    ascii_length = length_of_escaped_ascii_string(s, size);

    /* After length_of_escaped_ascii_string() there are two alternatives,
       either the string is pure ASCII with named escapes like \n, etc.
       and we determined it's exact size (common case)
       or it contains \x, \u, ... escape sequences.  then we create a
       legacy wchar string and resize it at the end of this function. */
    if (ascii_length >= 0) {
        v = (PyUnicodeObject *)PyUnicode_New(ascii_length, 127);
        if (!v)
            goto onError;
        assert(PyUnicode_KIND(v) == PyUnicode_1BYTE_KIND);
        kind = PyUnicode_1BYTE_KIND;
        data = PyUnicode_DATA(v);
    }
    else {
        /* Escaped strings will always be longer than the resulting
           Unicode string, so we start with size here and then reduce the
           length after conversion to the true value.
           (but if the error callback returns a long replacement string
           we'll have to allocate more space) */
        v = _PyUnicode_New(size);
        if (!v)
            goto onError;
        kind = PyUnicode_WCHAR_KIND;
        data = PyUnicode_AS_UNICODE(v);
    }

    if (size == 0)
        return (PyObject *)v;
    i = 0;
    end = s + size;

    while (s < end) {
        unsigned char c;
        Py_UNICODE x;
        int digits;

        if (kind == PyUnicode_WCHAR_KIND) {
            assert(i < _PyUnicode_WSTR_LENGTH(v));
        }
        else {
            /* The only case in which i == ascii_length is a backslash
               followed by a newline. */
            assert(i <= ascii_length);
        }

        /* Non-escape characters are interpreted as Unicode ordinals */
        if (*s != '\\') {
            WRITE_ASCII_OR_WSTR(kind, data, i++, (unsigned char) *s++);
            continue;
        }

        startinpos = s-starts;
        /* \ - Escapes */
        s++;
        c = *s++;
        if (s > end)
            c = '\0'; /* Invalid after \ */

        if (kind == PyUnicode_WCHAR_KIND) {
            assert(i < _PyUnicode_WSTR_LENGTH(v));
        }
        else {
            /* The only case in which i == ascii_length is a backslash
               followed by a newline. */
            assert(i < ascii_length || (i == ascii_length && c == '\n'));
        }

        switch (c) {

            /* \x escapes */
        case '\n': break;
        case '\\': WRITE_ASCII_OR_WSTR(kind, data, i++, '\\'); break;
        case '\'': WRITE_ASCII_OR_WSTR(kind, data, i++, '\''); break;
        case '\"': WRITE_ASCII_OR_WSTR(kind, data, i++, '\"'); break;
        case 'b': WRITE_ASCII_OR_WSTR(kind, data, i++, '\b'); break;
        /* FF */
        case 'f': WRITE_ASCII_OR_WSTR(kind, data, i++, '\014'); break;
        case 't': WRITE_ASCII_OR_WSTR(kind, data, i++, '\t'); break;
        case 'n': WRITE_ASCII_OR_WSTR(kind, data, i++, '\n'); break;
        case 'r': WRITE_ASCII_OR_WSTR(kind, data, i++, '\r'); break;
        /* VT */
        case 'v': WRITE_ASCII_OR_WSTR(kind, data, i++, '\013'); break;
        /* BEL, not classic C */
        case 'a': WRITE_ASCII_OR_WSTR(kind, data, i++, '\007'); break;

            /* \OOO (octal) escapes */
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            x = s[-1] - '0';
            if (s < end && '0' <= *s && *s <= '7') {
                x = (x<<3) + *s++ - '0';
                if (s < end && '0' <= *s && *s <= '7')
                    x = (x<<3) + *s++ - '0';
            }
            WRITE_WSTR(data, i++, x);
            break;

            /* hex escapes */
            /* \xXX */
        case 'x':
            digits = 2;
            message = "truncated \\xXX escape";
            goto hexescape;

            /* \uXXXX */
        case 'u':
            digits = 4;
            message = "truncated \\uXXXX escape";
            goto hexescape;

            /* \UXXXXXXXX */
        case 'U':
            digits = 8;
            message = "truncated \\UXXXXXXXX escape";
        hexescape:
            chr = 0;
            p = PyUnicode_AS_UNICODE(v) + i;
            if (s+digits>end) {
                endinpos = size;
                if (unicode_decode_call_errorhandler(
                        errors, &errorHandler,
                        "unicodeescape", "end of string in escape sequence",
                        &starts, &end, &startinpos, &endinpos, &exc, &s,
                        &v, &i, &p))
                    goto onError;
                data = PyUnicode_AS_UNICODE(v);
                goto nextByte;
            }
            for (j = 0; j < digits; ++j) {
                c = (unsigned char) s[j];
                if (!Py_ISXDIGIT(c)) {
                    endinpos = (s+j+1)-starts;
                    p = PyUnicode_AS_UNICODE(v) + i;
                    if (unicode_decode_call_errorhandler(
                            errors, &errorHandler,
                            "unicodeescape", message,
                            &starts, &end, &startinpos, &endinpos, &exc, &s,
                            &v, &i, &p))
                        goto onError;
                    data = PyUnicode_AS_UNICODE(v);
                    goto nextByte;
                }
                chr = (chr<<4) & ~0xF;
                if (c >= '0' && c <= '9')
                    chr += c - '0';
                else if (c >= 'a' && c <= 'f')
                    chr += 10 + c - 'a';
                else
                    chr += 10 + c - 'A';
            }
            s += j;
            if (chr == 0xffffffff && PyErr_Occurred())
                /* _decoding_error will have already written into the
                   target buffer. */
                break;
        store:
            /* when we get here, chr is a 32-bit unicode character */
            if (chr <= 0xffff)
                /* UCS-2 character */
                WRITE_WSTR(data, i++, chr);
            else if (chr <= 0x10ffff) {
                /* UCS-4 character. Either store directly, or as
                   surrogate pair. */
#ifdef Py_UNICODE_WIDE
                WRITE_WSTR(data, i++, chr);
#else
                chr -= 0x10000L;
                WRITE_WSTR(data, i++, 0xD800 + (Py_UNICODE) (chr >> 10));
                WRITE_WSTR(data, i++, 0xDC00 + (Py_UNICODE) (chr & 0x03FF));
#endif
            } else {
                endinpos = s-starts;
                p = PyUnicode_AS_UNICODE(v) + i;
                if (unicode_decode_call_errorhandler(
                        errors, &errorHandler,
                        "unicodeescape", "illegal Unicode character",
                        &starts, &end, &startinpos, &endinpos, &exc, &s,
                        &v, &i, &p))
                    goto onError;
                data = PyUnicode_AS_UNICODE(v);
            }
            break;

            /* \N{name} */
        case 'N':
            message = "malformed \\N character escape";
            if (ucnhash_CAPI == NULL) {
                /* load the unicode data module */
                ucnhash_CAPI = (_PyUnicode_Name_CAPI *)PyCapsule_Import(
                                                PyUnicodeData_CAPSULE_NAME, 1);
                if (ucnhash_CAPI == NULL)
                    goto ucnhashError;
            }
            if (*s == '{') {
                const char *start = s+1;
                /* look for the closing brace */
                while (*s != '}' && s < end)
                    s++;
                if (s > start && s < end && *s == '}') {
                    /* found a name.  look it up in the unicode database */
                    message = "unknown Unicode character name";
                    s++;
                    if (ucnhash_CAPI->getcode(NULL, start, (int)(s-start-1),
                                              &chr))
                        goto store;
                }
            }
            endinpos = s-starts;
            p = PyUnicode_AS_UNICODE(v) + i;
            if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "unicodeescape", message,
                    &starts, &end, &startinpos, &endinpos, &exc, &s,
                    &v, &i, &p))
                goto onError;
            data = PyUnicode_AS_UNICODE(v);
            break;

        default:
            if (s > end) {
                assert(kind == PyUnicode_WCHAR_KIND);
                message = "\\ at end of string";
                s--;
                endinpos = s-starts;
                p = PyUnicode_AS_UNICODE(v) + i;
                if (unicode_decode_call_errorhandler(
                        errors, &errorHandler,
                        "unicodeescape", message,
                        &starts, &end, &startinpos, &endinpos, &exc, &s,
                        &v, &i, &p))
                    goto onError;
                data = PyUnicode_AS_UNICODE(v);
            }
            else {
                WRITE_ASCII_OR_WSTR(kind, data, i++, '\\');
                WRITE_ASCII_OR_WSTR(kind, data, i++, (unsigned char)s[-1]);
            }
            break;
        }
      nextByte:
        ;
    }
    /* Ensure the length prediction worked in case of ASCII strings */
    assert(kind == PyUnicode_WCHAR_KIND || i == ascii_length);

    if (kind == PyUnicode_WCHAR_KIND)
    {
        if (PyUnicode_Resize((PyObject**)&v, i) < 0)
            goto onError;
    }
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&v)) {
        Py_DECREF(v);
        return NULL;
    }
    return (PyObject *)v;

  ucnhashError:
    PyErr_SetString(
        PyExc_UnicodeError,
        "\\N escapes not supported (can't load unicodedata module)"
        );
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;

  onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

#undef WRITE_ASCII_OR_WSTR
#undef WRITE_WSTR

/* Return a Unicode-Escape string version of the Unicode object.

   If quotes is true, the string is enclosed in u"" or u'' quotes as
   appropriate.

*/

static const char *hexdigits = "0123456789abcdef";

PyObject *
PyUnicode_EncodeUnicodeEscape(const Py_UNICODE *s,
                              Py_ssize_t size)
{
    PyObject *repr;
    char *p;

#ifdef Py_UNICODE_WIDE
    const Py_ssize_t expandsize = 10;
#else
    const Py_ssize_t expandsize = 6;
#endif

    /* XXX(nnorwitz): rather than over-allocating, it would be
       better to choose a different scheme.  Perhaps scan the
       first N-chars of the string and allocate based on that size.
    */
    /* Initial allocation is based on the longest-possible unichr
       escape.

       In wide (UTF-32) builds '\U00xxxxxx' is 10 chars per source
       unichr, so in this case it's the longest unichr escape. In
       narrow (UTF-16) builds this is five chars per source unichr
       since there are two unichrs in the surrogate pair, so in narrow
       (UTF-16) builds it's not the longest unichr escape.

       In wide or narrow builds '\uxxxx' is 6 chars per source unichr,
       so in the narrow (UTF-16) build case it's the longest unichr
       escape.
    */

    if (size == 0)
        return PyBytes_FromStringAndSize(NULL, 0);

    if (size > (PY_SSIZE_T_MAX - 2 - 1) / expandsize)
        return PyErr_NoMemory();

    repr = PyBytes_FromStringAndSize(NULL,
                                     2
                                     + expandsize*size
                                     + 1);
    if (repr == NULL)
        return NULL;

    p = PyBytes_AS_STRING(repr);

    while (size-- > 0) {
        Py_UNICODE ch = *s++;

        /* Escape backslashes */
        if (ch == '\\') {
            *p++ = '\\';
            *p++ = (char) ch;
            continue;
        }

#ifdef Py_UNICODE_WIDE
        /* Map 21-bit characters to '\U00xxxxxx' */
        else if (ch >= 0x10000) {
            *p++ = '\\';
            *p++ = 'U';
            *p++ = hexdigits[(ch >> 28) & 0x0000000F];
            *p++ = hexdigits[(ch >> 24) & 0x0000000F];
            *p++ = hexdigits[(ch >> 20) & 0x0000000F];
            *p++ = hexdigits[(ch >> 16) & 0x0000000F];
            *p++ = hexdigits[(ch >> 12) & 0x0000000F];
            *p++ = hexdigits[(ch >> 8) & 0x0000000F];
            *p++ = hexdigits[(ch >> 4) & 0x0000000F];
            *p++ = hexdigits[ch & 0x0000000F];
            continue;
        }
#else
        /* Map UTF-16 surrogate pairs to '\U00xxxxxx' */
        else if (ch >= 0xD800 && ch < 0xDC00) {
            Py_UNICODE ch2;
            Py_UCS4 ucs;

            ch2 = *s++;
            size--;
            if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
                ucs = (((ch & 0x03FF) << 10) | (ch2 & 0x03FF)) + 0x00010000;
                *p++ = '\\';
                *p++ = 'U';
                *p++ = hexdigits[(ucs >> 28) & 0x0000000F];
                *p++ = hexdigits[(ucs >> 24) & 0x0000000F];
                *p++ = hexdigits[(ucs >> 20) & 0x0000000F];
                *p++ = hexdigits[(ucs >> 16) & 0x0000000F];
                *p++ = hexdigits[(ucs >> 12) & 0x0000000F];
                *p++ = hexdigits[(ucs >> 8) & 0x0000000F];
                *p++ = hexdigits[(ucs >> 4) & 0x0000000F];
                *p++ = hexdigits[ucs & 0x0000000F];
                continue;
            }
            /* Fall through: isolated surrogates are copied as-is */
            s--;
            size++;
        }
#endif

        /* Map 16-bit characters to '\uxxxx' */
        if (ch >= 256) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigits[(ch >> 12) & 0x000F];
            *p++ = hexdigits[(ch >> 8) & 0x000F];
            *p++ = hexdigits[(ch >> 4) & 0x000F];
            *p++ = hexdigits[ch & 0x000F];
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

        /* Map non-printable US ASCII to '\xhh' */
        else if (ch < ' ' || ch >= 0x7F) {
            *p++ = '\\';
            *p++ = 'x';
            *p++ = hexdigits[(ch >> 4) & 0x000F];
            *p++ = hexdigits[ch & 0x000F];
        }

        /* Copy everything else as-is */
        else
            *p++ = (char) ch;
    }

    assert(p - PyBytes_AS_STRING(repr) > 0);
    if (_PyBytes_Resize(&repr, p - PyBytes_AS_STRING(repr)) < 0)
        return NULL;
    return repr;
}

PyObject *
PyUnicode_AsUnicodeEscapeString(PyObject *unicode)
{
    PyObject *s;
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    s = PyUnicode_EncodeUnicodeEscape(PyUnicode_AS_UNICODE(unicode),
                                      PyUnicode_GET_SIZE(unicode));
    return s;
}

/* --- Raw Unicode Escape Codec ------------------------------------------- */

PyObject *
PyUnicode_DecodeRawUnicodeEscape(const char *s,
                                 Py_ssize_t size,
                                 const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    const char *end;
    const char *bs;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    /* Escaped strings will always be longer than the resulting
       Unicode string, so we start with size here and then reduce the
       length after conversion to the true value. (But decoding error
       handler might have to resize the string) */
    v = _PyUnicode_New(size);
    if (v == NULL)
        goto onError;
    if (size == 0)
        return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    end = s + size;
    while (s < end) {
        unsigned char c;
        Py_UCS4 x;
        int i;
        int count;

        /* Non-escape characters are interpreted as Unicode ordinals */
        if (*s != '\\') {
            *p++ = (unsigned char)*s++;
            continue;
        }
        startinpos = s-starts;

        /* \u-escapes are only interpreted iff the number of leading
           backslashes if odd */
        bs = s;
        for (;s < end;) {
            if (*s != '\\')
                break;
            *p++ = (unsigned char)*s++;
        }
        if (((s - bs) & 1) == 0 ||
            s >= end ||
            (*s != 'u' && *s != 'U')) {
            continue;
        }
        p--;
        count = *s=='u' ? 4 : 8;
        s++;

        /* \uXXXX with 4 hex digits, \Uxxxxxxxx with 8 */
        outpos = p-PyUnicode_AS_UNICODE(v);
        for (x = 0, i = 0; i < count; ++i, ++s) {
            c = (unsigned char)*s;
            if (!Py_ISXDIGIT(c)) {
                endinpos = s-starts;
                if (unicode_decode_call_errorhandler(
                        errors, &errorHandler,
                        "rawunicodeescape", "truncated \\uXXXX",
                        &starts, &end, &startinpos, &endinpos, &exc, &s,
                        &v, &outpos, &p))
                    goto onError;
                goto nextByte;
            }
            x = (x<<4) & ~0xF;
            if (c >= '0' && c <= '9')
                x += c - '0';
            else if (c >= 'a' && c <= 'f')
                x += 10 + c - 'a';
            else
                x += 10 + c - 'A';
        }
        if (x <= 0xffff)
            /* UCS-2 character */
            *p++ = (Py_UNICODE) x;
        else if (x <= 0x10ffff) {
            /* UCS-4 character. Either store directly, or as
               surrogate pair. */
#ifdef Py_UNICODE_WIDE
            *p++ = (Py_UNICODE) x;
#else
            x -= 0x10000L;
            *p++ = 0xD800 + (Py_UNICODE) (x >> 10);
            *p++ = 0xDC00 + (Py_UNICODE) (x & 0x03FF);
#endif
        } else {
            endinpos = s-starts;
            outpos = p-PyUnicode_AS_UNICODE(v);
            if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "rawunicodeescape", "\\Uxxxxxxxx out of range",
                    &starts, &end, &startinpos, &endinpos, &exc, &s,
                    &v, &outpos, &p))
                goto onError;
        }
      nextByte:
        ;
    }
    if (PyUnicode_Resize((PyObject**)&v, p - PyUnicode_AS_UNICODE(v)) < 0)
        goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&v)) {
        Py_DECREF(v);
        return NULL;
    }
    return (PyObject *)v;

  onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *
PyUnicode_EncodeRawUnicodeEscape(const Py_UNICODE *s,
                                 Py_ssize_t size)
{
    PyObject *repr;
    char *p;
    char *q;

#ifdef Py_UNICODE_WIDE
    const Py_ssize_t expandsize = 10;
#else
    const Py_ssize_t expandsize = 6;
#endif

    if (size > PY_SSIZE_T_MAX / expandsize)
        return PyErr_NoMemory();

    repr = PyBytes_FromStringAndSize(NULL, expandsize * size);
    if (repr == NULL)
        return NULL;
    if (size == 0)
        return repr;

    p = q = PyBytes_AS_STRING(repr);
    while (size-- > 0) {
        Py_UNICODE ch = *s++;
#ifdef Py_UNICODE_WIDE
        /* Map 32-bit characters to '\Uxxxxxxxx' */
        if (ch >= 0x10000) {
            *p++ = '\\';
            *p++ = 'U';
            *p++ = hexdigits[(ch >> 28) & 0xf];
            *p++ = hexdigits[(ch >> 24) & 0xf];
            *p++ = hexdigits[(ch >> 20) & 0xf];
            *p++ = hexdigits[(ch >> 16) & 0xf];
            *p++ = hexdigits[(ch >> 12) & 0xf];
            *p++ = hexdigits[(ch >> 8) & 0xf];
            *p++ = hexdigits[(ch >> 4) & 0xf];
            *p++ = hexdigits[ch & 15];
        }
        else
#else
            /* Map UTF-16 surrogate pairs to '\U00xxxxxx' */
            if (ch >= 0xD800 && ch < 0xDC00) {
                Py_UNICODE ch2;
                Py_UCS4 ucs;

                ch2 = *s++;
                size--;
                if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
                    ucs = (((ch & 0x03FF) << 10) | (ch2 & 0x03FF)) + 0x00010000;
                    *p++ = '\\';
                    *p++ = 'U';
                    *p++ = hexdigits[(ucs >> 28) & 0xf];
                    *p++ = hexdigits[(ucs >> 24) & 0xf];
                    *p++ = hexdigits[(ucs >> 20) & 0xf];
                    *p++ = hexdigits[(ucs >> 16) & 0xf];
                    *p++ = hexdigits[(ucs >> 12) & 0xf];
                    *p++ = hexdigits[(ucs >> 8) & 0xf];
                    *p++ = hexdigits[(ucs >> 4) & 0xf];
                    *p++ = hexdigits[ucs & 0xf];
                    continue;
                }
                /* Fall through: isolated surrogates are copied as-is */
                s--;
                size++;
            }
#endif
        /* Map 16-bit characters to '\uxxxx' */
        if (ch >= 256) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigits[(ch >> 12) & 0xf];
            *p++ = hexdigits[(ch >> 8) & 0xf];
            *p++ = hexdigits[(ch >> 4) & 0xf];
            *p++ = hexdigits[ch & 15];
        }
        /* Copy everything else as-is */
        else
            *p++ = (char) ch;
    }
    size = p - q;

    assert(size > 0);
    if (_PyBytes_Resize(&repr, size) < 0)
        return NULL;
    return repr;
}

PyObject *
PyUnicode_AsRawUnicodeEscapeString(PyObject *unicode)
{
    PyObject *s;
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    s = PyUnicode_EncodeRawUnicodeEscape(PyUnicode_AS_UNICODE(unicode),
                                         PyUnicode_GET_SIZE(unicode));

    return s;
}

/* --- Unicode Internal Codec ------------------------------------------- */

PyObject *
_PyUnicode_DecodeUnicodeInternal(const char *s,
                                 Py_ssize_t size,
                                 const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    const char *end;
    const char *reason;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

#ifdef Py_UNICODE_WIDE
    Py_UNICODE unimax = PyUnicode_GetMax();
#endif

    /* XXX overflow detection missing */
    v = _PyUnicode_New((size+Py_UNICODE_SIZE-1)/ Py_UNICODE_SIZE);
    if (v == NULL)
        goto onError;
    /* Intentionally PyUnicode_GET_SIZE instead of PyUnicode_GET_LENGTH
       as string was created with the old API. */
    if (PyUnicode_GET_SIZE(v) == 0)
        return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    end = s + size;

    while (s < end) {
        memcpy(p, s, sizeof(Py_UNICODE));
        /* We have to sanity check the raw data, otherwise doom looms for
           some malformed UCS-4 data. */
        if (
#ifdef Py_UNICODE_WIDE
            *p > unimax || *p < 0 ||
#endif
            end-s < Py_UNICODE_SIZE
            )
        {
            startinpos = s - starts;
            if (end-s < Py_UNICODE_SIZE) {
                endinpos = end-starts;
                reason = "truncated input";
            }
            else {
                endinpos = s - starts + Py_UNICODE_SIZE;
                reason = "illegal code point (> 0x10FFFF)";
            }
            outpos = p - PyUnicode_AS_UNICODE(v);
            if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "unicode_internal", reason,
                    &starts, &end, &startinpos, &endinpos, &exc, &s,
                    &v, &outpos, &p)) {
                goto onError;
            }
        }
        else {
            p++;
            s += Py_UNICODE_SIZE;
        }
    }

    if (PyUnicode_Resize((PyObject**)&v, p - PyUnicode_AS_UNICODE(v)) < 0)
        goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&v)) {
        Py_DECREF(v);
        return NULL;
    }
    return (PyObject *)v;

  onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

/* --- Latin-1 Codec ------------------------------------------------------ */

PyObject *
PyUnicode_DecodeLatin1(const char *s,
                       Py_ssize_t size,
                       const char *errors)
{
    /* Latin-1 is equivalent to the first 256 ordinals in Unicode. */
    return _PyUnicode_FromUCS1((unsigned char*)s, size);
}

/* create or adjust a UnicodeEncodeError */
static void
make_encode_exception(PyObject **exceptionObject,
                      const char *encoding,
                      const Py_UNICODE *unicode, Py_ssize_t size,
                      Py_ssize_t startpos, Py_ssize_t endpos,
                      const char *reason)
{
    if (*exceptionObject == NULL) {
        *exceptionObject = PyUnicodeEncodeError_Create(
            encoding, unicode, size, startpos, endpos, reason);
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
        Py_DECREF(*exceptionObject);
        *exceptionObject = NULL;
    }
}

/* raises a UnicodeEncodeError */
static void
raise_encode_exception(PyObject **exceptionObject,
                       const char *encoding,
                       const Py_UNICODE *unicode, Py_ssize_t size,
                       Py_ssize_t startpos, Py_ssize_t endpos,
                       const char *reason)
{
    make_encode_exception(exceptionObject,
                          encoding, unicode, size, startpos, endpos, reason);
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
                                 const Py_UNICODE *unicode, Py_ssize_t size, PyObject **exceptionObject,
                                 Py_ssize_t startpos, Py_ssize_t endpos,
                                 Py_ssize_t *newpos)
{
    static char *argparse = "On;encoding error handler must return (str/bytes, int) tuple";

    PyObject *restuple;
    PyObject *resunicode;

    if (*errorHandler == NULL) {
        *errorHandler = PyCodec_LookupError(errors);
        if (*errorHandler == NULL)
            return NULL;
    }

    make_encode_exception(exceptionObject,
                          encoding, unicode, size, startpos, endpos, reason);
    if (*exceptionObject == NULL)
        return NULL;

    restuple = PyObject_CallFunctionObjArgs(
        *errorHandler, *exceptionObject, NULL);
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
        *newpos = size+*newpos;
    if (*newpos<0 || *newpos>size) {
        PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", *newpos);
        Py_DECREF(restuple);
        return NULL;
    }
    Py_INCREF(resunicode);
    Py_DECREF(restuple);
    return resunicode;
}

static PyObject *
unicode_encode_ucs1(const Py_UNICODE *p,
                    Py_ssize_t size,
                    const char *errors,
                    int limit)
{
    /* output object */
    PyObject *res;
    /* pointers to the beginning and end+1 of input */
    const Py_UNICODE *startp = p;
    const Py_UNICODE *endp = p + size;
    /* pointer to the beginning of the unencodable characters */
    /* const Py_UNICODE *badp = NULL; */
    /* pointer into the output */
    char *str;
    /* current output position */
    Py_ssize_t ressize;
    const char *encoding = (limit == 256) ? "latin-1" : "ascii";
    const char *reason = (limit == 256) ? "ordinal not in range(256)" : "ordinal not in range(128)";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace, 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    if (size == 0)
        return PyBytes_FromStringAndSize(NULL, 0);
    res = PyBytes_FromStringAndSize(NULL, size);
    if (res == NULL)
        return NULL;
    str = PyBytes_AS_STRING(res);
    ressize = size;

    while (p<endp) {
        Py_UNICODE c = *p;

        /* can we encode this? */
        if (c<limit) {
            /* no overflow check, because we know that the space is enough */
            *str++ = (char)c;
            ++p;
        }
        else {
            Py_ssize_t unicodepos = p-startp;
            Py_ssize_t requiredsize;
            PyObject *repunicode;
            Py_ssize_t repsize;
            Py_ssize_t newpos;
            Py_ssize_t respos;
            Py_UNICODE *uni2;
            /* startpos for collecting unencodable chars */
            const Py_UNICODE *collstart = p;
            const Py_UNICODE *collend = p;
            /* find all unecodable characters */
            while ((collend < endp) && ((*collend)>=limit))
                ++collend;
            /* cache callback name lookup (if not done yet, i.e. it's the first error) */
            if (known_errorHandler==-1) {
                if ((errors==NULL) || (!strcmp(errors, "strict")))
                    known_errorHandler = 1;
                else if (!strcmp(errors, "replace"))
                    known_errorHandler = 2;
                else if (!strcmp(errors, "ignore"))
                    known_errorHandler = 3;
                else if (!strcmp(errors, "xmlcharrefreplace"))
                    known_errorHandler = 4;
                else
                    known_errorHandler = 0;
            }
            switch (known_errorHandler) {
            case 1: /* strict */
                raise_encode_exception(&exc, encoding, startp, size, collstart-startp, collend-startp, reason);
                goto onError;
            case 2: /* replace */
                while (collstart++<collend)
                    *str++ = '?'; /* fall through */
            case 3: /* ignore */
                p = collend;
                break;
            case 4: /* xmlcharrefreplace */
                respos = str - PyBytes_AS_STRING(res);
                /* determine replacement size (temporarily (mis)uses p) */
                for (p = collstart, repsize = 0; p < collend; ++p) {
                    if (*p<10)
                        repsize += 2+1+1;
                    else if (*p<100)
                        repsize += 2+2+1;
                    else if (*p<1000)
                        repsize += 2+3+1;
                    else if (*p<10000)
                        repsize += 2+4+1;
#ifndef Py_UNICODE_WIDE
                    else
                        repsize += 2+5+1;
#else
                    else if (*p<100000)
                        repsize += 2+5+1;
                    else if (*p<1000000)
                        repsize += 2+6+1;
                    else
                        repsize += 2+7+1;
#endif
                }
                requiredsize = respos+repsize+(endp-collend);
                if (requiredsize > ressize) {
                    if (requiredsize<2*ressize)
                        requiredsize = 2*ressize;
                    if (_PyBytes_Resize(&res, requiredsize))
                        goto onError;
                    str = PyBytes_AS_STRING(res) + respos;
                    ressize = requiredsize;
                }
                /* generate replacement (temporarily (mis)uses p) */
                for (p = collstart; p < collend; ++p) {
                    str += sprintf(str, "&#%d;", (int)*p);
                }
                p = collend;
                break;
            default:
                repunicode = unicode_encode_call_errorhandler(errors, &errorHandler,
                                                              encoding, reason, startp, size, &exc,
                                                              collstart-startp, collend-startp, &newpos);
                if (repunicode == NULL)
                    goto onError;
                if (PyBytes_Check(repunicode)) {
                    /* Directly copy bytes result to output. */
                    repsize = PyBytes_Size(repunicode);
                    if (repsize > 1) {
                        /* Make room for all additional bytes. */
                        respos = str - PyBytes_AS_STRING(res);
                        if (_PyBytes_Resize(&res, ressize+repsize-1)) {
                            Py_DECREF(repunicode);
                            goto onError;
                        }
                        str = PyBytes_AS_STRING(res) + respos;
                        ressize += repsize-1;
                    }
                    memcpy(str, PyBytes_AsString(repunicode), repsize);
                    str += repsize;
                    p = startp + newpos;
                    Py_DECREF(repunicode);
                    break;
                }
                /* need more space? (at least enough for what we
                   have+the replacement+the rest of the string, so
                   we won't have to check space for encodable characters) */
                respos = str - PyBytes_AS_STRING(res);
                repsize = PyUnicode_GET_SIZE(repunicode);
                requiredsize = respos+repsize+(endp-collend);
                if (requiredsize > ressize) {
                    if (requiredsize<2*ressize)
                        requiredsize = 2*ressize;
                    if (_PyBytes_Resize(&res, requiredsize)) {
                        Py_DECREF(repunicode);
                        goto onError;
                    }
                    str = PyBytes_AS_STRING(res) + respos;
                    ressize = requiredsize;
                }
                /* check if there is anything unencodable in the replacement
                   and copy it to the output */
                for (uni2 = PyUnicode_AS_UNICODE(repunicode);repsize-->0; ++uni2, ++str) {
                    c = *uni2;
                    if (c >= limit) {
                        raise_encode_exception(&exc, encoding, startp, size,
                                               unicodepos, unicodepos+1, reason);
                        Py_DECREF(repunicode);
                        goto onError;
                    }
                    *str = (char)c;
                }
                p = startp + newpos;
                Py_DECREF(repunicode);
            }
        }
    }
    /* Resize if we allocated to much */
    size = str - PyBytes_AS_STRING(res);
    if (size < ressize) { /* If this falls res will be NULL */
        assert(size >= 0);
        if (_PyBytes_Resize(&res, size) < 0)
            goto onError;
    }

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return res;

  onError:
    Py_XDECREF(res);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *
PyUnicode_EncodeLatin1(const Py_UNICODE *p,
                       Py_ssize_t size,
                       const char *errors)
{
    return unicode_encode_ucs1(p, size, errors, 256);
}

PyObject *
_PyUnicode_AsLatin1String(PyObject *unicode, const char *errors)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (PyUnicode_READY(unicode) == -1)
        return NULL;
    /* Fast path: if it is a one-byte string, construct
       bytes object directly. */
    if (PyUnicode_KIND(unicode) == PyUnicode_1BYTE_KIND)
        return PyBytes_FromStringAndSize(PyUnicode_DATA(unicode),
                                         PyUnicode_GET_LENGTH(unicode));
    /* Non-Latin-1 characters present. Defer to above function to
       raise the exception. */
    return PyUnicode_EncodeLatin1(PyUnicode_AS_UNICODE(unicode),
                                  PyUnicode_GET_SIZE(unicode),
                                  errors);
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
    PyUnicodeObject *v;
    Py_UNICODE *p;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    const char *e;
    unsigned char* d;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    Py_ssize_t i;

    /* ASCII is equivalent to the first 128 ordinals in Unicode. */
    if (size == 1 && *(unsigned char*)s < 128)
        return PyUnicode_FromOrdinal(*(unsigned char*)s);

    /* Fast path. Assume the input actually *is* ASCII, and allocate
       a single-block Unicode object with that assumption. If there is
       an error, drop the object and start over. */
    v = (PyUnicodeObject*)PyUnicode_New(size, 127);
    if (v == NULL)
        goto onError;
    d = PyUnicode_1BYTE_DATA(v);
    for (i = 0; i < size; i++) {
        unsigned char ch = ((unsigned char*)s)[i];
        if (ch < 128)
            d[i] = ch;
        else
            break;
    }
    if (i == size)
        return (PyObject*)v;
    Py_DECREF(v); /* start over */

    v = _PyUnicode_New(size);
    if (v == NULL)
        goto onError;
    if (size == 0)
        return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    e = s + size;
    while (s < e) {
        register unsigned char c = (unsigned char)*s;
        if (c < 128) {
            *p++ = c;
            ++s;
        }
        else {
            startinpos = s-starts;
            endinpos = startinpos + 1;
            outpos = p - (Py_UNICODE *)PyUnicode_AS_UNICODE(v);
            if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "ascii", "ordinal not in range(128)",
                    &starts, &e, &startinpos, &endinpos, &exc, &s,
                    &v, &outpos, &p))
                goto onError;
        }
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyUnicode_GET_SIZE(v))
        if (PyUnicode_Resize((PyObject**)&v, p - PyUnicode_AS_UNICODE(v)) < 0)
            goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&v)) {
        Py_DECREF(v);
        return NULL;
    }
    return (PyObject *)v;

  onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *
PyUnicode_EncodeASCII(const Py_UNICODE *p,
                      Py_ssize_t size,
                      const char *errors)
{
    return unicode_encode_ucs1(p, size, errors, 128);
}

PyObject *
_PyUnicode_AsASCIIString(PyObject *unicode, const char *errors)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (PyUnicode_READY(unicode) == -1)
        return NULL;
    /* Fast path: if it is an ASCII-only string, construct bytes object
       directly. Else defer to above function to raise the exception. */
    if (PyUnicode_MAX_CHAR_VALUE(unicode) < 128)
        return PyBytes_FromStringAndSize(PyUnicode_DATA(unicode),
                                         PyUnicode_GET_LENGTH(unicode));
    return PyUnicode_EncodeASCII(PyUnicode_AS_UNICODE(unicode),
                                 PyUnicode_GET_SIZE(unicode),
                                 errors);
}

PyObject *
PyUnicode_AsASCIIString(PyObject *unicode)
{
    return _PyUnicode_AsASCIIString(unicode, NULL);
}

#ifdef HAVE_MBCS

/* --- MBCS codecs for Windows -------------------------------------------- */

#if SIZEOF_INT < SIZEOF_SIZE_T
#define NEED_RETRY
#endif

/* XXX This code is limited to "true" double-byte encodings, as
   a) it assumes an incomplete character consists of a single byte, and
   b) IsDBCSLeadByte (probably) does not work for non-DBCS multi-byte
   encodings, see IsDBCSLeadByteEx documentation. */

static int
is_dbcs_lead_byte(const char *s, int offset)
{
    const char *curr = s + offset;

    if (IsDBCSLeadByte(*curr)) {
        const char *prev = CharPrev(s, curr);
        return (prev == curr) || !IsDBCSLeadByte(*prev) || (curr - prev == 2);
    }
    return 0;
}

/*
 * Decode MBCS string into unicode object. If 'final' is set, converts
 * trailing lead-byte too. Returns consumed size if succeed, -1 otherwise.
 */
static int
decode_mbcs(PyUnicodeObject **v,
            const char *s, /* MBCS string */
            int size, /* sizeof MBCS string */
            int final,
            const char *errors)
{
    Py_UNICODE *p;
    Py_ssize_t n;
    DWORD usize;
    DWORD flags;

    assert(size >= 0);

    /* check and handle 'errors' arg */
    if (errors==NULL || strcmp(errors, "strict")==0)
        flags = MB_ERR_INVALID_CHARS;
    else if (strcmp(errors, "ignore")==0)
        flags = 0;
    else {
        PyErr_Format(PyExc_ValueError,
                     "mbcs encoding does not support errors='%s'",
                     errors);
        return -1;
    }

    /* Skip trailing lead-byte unless 'final' is set */
    if (!final && size >= 1 && is_dbcs_lead_byte(s, size - 1))
        --size;

    /* First get the size of the result */
    if (size > 0) {
        usize = MultiByteToWideChar(CP_ACP, flags, s, size, NULL, 0);
        if (usize==0)
            goto mbcs_decode_error;
    } else
        usize = 0;

    if (*v == NULL) {
        /* Create unicode object */
        *v = _PyUnicode_New(usize);
        if (*v == NULL)
            return -1;
        n = 0;
    }
    else {
        /* Extend unicode object */
        n = PyUnicode_GET_SIZE(*v);
        if (PyUnicode_Resize((PyObject**)v, n + usize) < 0)
            return -1;
    }

    /* Do the conversion */
    if (usize > 0) {
        p = PyUnicode_AS_UNICODE(*v) + n;
        if (0 == MultiByteToWideChar(CP_ACP, flags, s, size, p, usize)) {
            goto mbcs_decode_error;
        }
    }
    return size;

mbcs_decode_error:
    /* If the last error was ERROR_NO_UNICODE_TRANSLATION, then
       we raise a UnicodeDecodeError - else it is a 'generic'
       windows error
     */
    if (GetLastError()==ERROR_NO_UNICODE_TRANSLATION) {
        /* Ideally, we should get reason from FormatMessage - this
           is the Windows 2000 English version of the message
        */
        PyObject *exc = NULL;
        const char *reason = "No mapping for the Unicode character exists "
                             "in the target multi-byte code page.";
        make_decode_exception(&exc, "mbcs", s, size, 0, 0, reason);
        if (exc != NULL) {
            PyCodec_StrictErrors(exc);
            Py_DECREF(exc);
        }
    } else {
        PyErr_SetFromWindowsErrWithFilename(0, NULL);
    }
    return -1;
}

PyObject *
PyUnicode_DecodeMBCSStateful(const char *s,
                             Py_ssize_t size,
                             const char *errors,
                             Py_ssize_t *consumed)
{
    PyUnicodeObject *v = NULL;
    int done;

    if (consumed)
        *consumed = 0;

#ifdef NEED_RETRY
  retry:
    if (size > INT_MAX)
        done = decode_mbcs(&v, s, INT_MAX, 0, errors);
    else
#endif
        done = decode_mbcs(&v, s, (int)size, !consumed, errors);

    if (done < 0) {
        Py_XDECREF(v);
        return NULL;
    }

    if (consumed)
        *consumed += done;

#ifdef NEED_RETRY
    if (size > INT_MAX) {
        s += done;
        size -= done;
        goto retry;
    }
#endif
    if (_PyUnicode_READY_REPLACE(&v)) {
        Py_DECREF(v);
        return NULL;
    }
    return (PyObject *)v;
}

PyObject *
PyUnicode_DecodeMBCS(const char *s,
                     Py_ssize_t size,
                     const char *errors)
{
    return PyUnicode_DecodeMBCSStateful(s, size, errors, NULL);
}

/*
 * Convert unicode into string object (MBCS).
 * Returns 0 if succeed, -1 otherwise.
 */
static int
encode_mbcs(PyObject **repr,
            const Py_UNICODE *p, /* unicode */
            int size, /* size of unicode */
            const char* errors)
{
    BOOL usedDefaultChar = FALSE;
    BOOL *pusedDefaultChar;
    int mbcssize;
    Py_ssize_t n;
    PyObject *exc = NULL;
    DWORD flags;

    assert(size >= 0);

    /* check and handle 'errors' arg */
    if (errors==NULL || strcmp(errors, "strict")==0) {
        flags = WC_NO_BEST_FIT_CHARS;
        pusedDefaultChar = &usedDefaultChar;
    } else if (strcmp(errors, "replace")==0) {
        flags = 0;
        pusedDefaultChar = NULL;
    } else {
         PyErr_Format(PyExc_ValueError,
                      "mbcs encoding does not support errors='%s'",
                      errors);
         return -1;
    }

    /* First get the size of the result */
    if (size > 0) {
        mbcssize = WideCharToMultiByte(CP_ACP, flags, p, size, NULL, 0,
                                       NULL, pusedDefaultChar);
        if (mbcssize == 0) {
            PyErr_SetFromWindowsErrWithFilename(0, NULL);
            return -1;
        }
        /* If we used a default char, then we failed! */
        if (pusedDefaultChar && *pusedDefaultChar)
            goto mbcs_encode_error;
    } else {
        mbcssize = 0;
    }

    if (*repr == NULL) {
        /* Create string object */
        *repr = PyBytes_FromStringAndSize(NULL, mbcssize);
        if (*repr == NULL)
            return -1;
        n = 0;
    }
    else {
        /* Extend string object */
        n = PyBytes_Size(*repr);
        if (_PyBytes_Resize(repr, n + mbcssize) < 0)
            return -1;
    }

    /* Do the conversion */
    if (size > 0) {
        char *s = PyBytes_AS_STRING(*repr) + n;
        if (0 == WideCharToMultiByte(CP_ACP, flags, p, size, s, mbcssize,
                                     NULL, pusedDefaultChar)) {
            PyErr_SetFromWindowsErrWithFilename(0, NULL);
            return -1;
        }
        if (pusedDefaultChar && *pusedDefaultChar)
            goto mbcs_encode_error;
    }
    return 0;

mbcs_encode_error:
    raise_encode_exception(&exc, "mbcs", p, size, 0, 0, "invalid character");
    Py_XDECREF(exc);
    return -1;
}

PyObject *
PyUnicode_EncodeMBCS(const Py_UNICODE *p,
                     Py_ssize_t size,
                     const char *errors)
{
    PyObject *repr = NULL;
    int ret;

#ifdef NEED_RETRY
  retry:
    if (size > INT_MAX)
        ret = encode_mbcs(&repr, p, INT_MAX, errors);
    else
#endif
        ret = encode_mbcs(&repr, p, (int)size, errors);

    if (ret < 0) {
        Py_XDECREF(repr);
        return NULL;
    }

#ifdef NEED_RETRY
    if (size > INT_MAX) {
        p += INT_MAX;
        size -= INT_MAX;
        goto retry;
    }
#endif

    return repr;
}

PyObject *
PyUnicode_AsMBCSString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeMBCS(PyUnicode_AS_UNICODE(unicode),
                                PyUnicode_GET_SIZE(unicode),
                                NULL);
}

#undef NEED_RETRY

#endif /* HAVE_MBCS */

/* --- Character Mapping Codec -------------------------------------------- */

PyObject *
PyUnicode_DecodeCharmap(const char *s,
                        Py_ssize_t size,
                        PyObject *mapping,
                        const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    const char *e;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    Py_ssize_t extrachars = 0;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    Py_UNICODE *mapstring = NULL;
    Py_ssize_t maplen = 0;

    /* Default to Latin-1 */
    if (mapping == NULL)
        return PyUnicode_DecodeLatin1(s, size, errors);

    v = _PyUnicode_New(size);
    if (v == NULL)
        goto onError;
    if (size == 0)
        return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    e = s + size;
    if (PyUnicode_CheckExact(mapping)) {
        mapstring = PyUnicode_AS_UNICODE(mapping);
        maplen = PyUnicode_GET_SIZE(mapping);
        while (s < e) {
            unsigned char ch = *s;
            Py_UNICODE x = 0xfffe; /* illegal value */

            if (ch < maplen)
                x = mapstring[ch];

            if (x == 0xfffe) {
                /* undefined mapping */
                outpos = p-PyUnicode_AS_UNICODE(v);
                startinpos = s-starts;
                endinpos = startinpos+1;
                if (unicode_decode_call_errorhandler(
                        errors, &errorHandler,
                        "charmap", "character maps to <undefined>",
                        &starts, &e, &startinpos, &endinpos, &exc, &s,
                        &v, &outpos, &p)) {
                    goto onError;
                }
                continue;
            }
            *p++ = x;
            ++s;
        }
    }
    else {
        while (s < e) {
            unsigned char ch = *s;
            PyObject *w, *x;

            /* Get mapping (char ordinal -> integer, Unicode char or None) */
            w = PyLong_FromLong((long)ch);
            if (w == NULL)
                goto onError;
            x = PyObject_GetItem(mapping, w);
            Py_DECREF(w);
            if (x == NULL) {
                if (PyErr_ExceptionMatches(PyExc_LookupError)) {
                    /* No mapping found means: mapping is undefined. */
                    PyErr_Clear();
                    x = Py_None;
                    Py_INCREF(x);
                } else
                    goto onError;
            }

            /* Apply mapping */
            if (PyLong_Check(x)) {
                long value = PyLong_AS_LONG(x);
                if (value < 0 || value > 65535) {
                    PyErr_SetString(PyExc_TypeError,
                                    "character mapping must be in range(65536)");
                    Py_DECREF(x);
                    goto onError;
                }
                *p++ = (Py_UNICODE)value;
            }
            else if (x == Py_None) {
                /* undefined mapping */
                outpos = p-PyUnicode_AS_UNICODE(v);
                startinpos = s-starts;
                endinpos = startinpos+1;
                if (unicode_decode_call_errorhandler(
                        errors, &errorHandler,
                        "charmap", "character maps to <undefined>",
                        &starts, &e, &startinpos, &endinpos, &exc, &s,
                        &v, &outpos, &p)) {
                    Py_DECREF(x);
                    goto onError;
                }
                Py_DECREF(x);
                continue;
            }
            else if (PyUnicode_Check(x)) {
                Py_ssize_t targetsize = PyUnicode_GET_SIZE(x);

                if (targetsize == 1)
                    /* 1-1 mapping */
                    *p++ = *PyUnicode_AS_UNICODE(x);

                else if (targetsize > 1) {
                    /* 1-n mapping */
                    if (targetsize > extrachars) {
                        /* resize first */
                        Py_ssize_t oldpos = p - PyUnicode_AS_UNICODE(v);
                        Py_ssize_t needed = (targetsize - extrachars) + \
                            (targetsize << 2);
                        extrachars += needed;
                        /* XXX overflow detection missing */
                        if (PyUnicode_Resize((PyObject**)&v,
                                              PyUnicode_GET_SIZE(v) + needed) < 0) {
                            Py_DECREF(x);
                            goto onError;
                        }
                        p = PyUnicode_AS_UNICODE(v) + oldpos;
                    }
                    Py_UNICODE_COPY(p,
                                    PyUnicode_AS_UNICODE(x),
                                    targetsize);
                    p += targetsize;
                    extrachars -= targetsize;
                }
                /* 1-0 mapping: skip the character */
            }
            else {
                /* wrong return value */
                PyErr_SetString(PyExc_TypeError,
                                "character mapping must return integer, None or str");
                Py_DECREF(x);
                goto onError;
            }
            Py_DECREF(x);
            ++s;
        }
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyUnicode_GET_SIZE(v))
        if (PyUnicode_Resize((PyObject**)&v, p - PyUnicode_AS_UNICODE(v)) < 0)
            goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    if (_PyUnicode_READY_REPLACE(&v)) {
        Py_DECREF(v);
        return NULL;
    }
    return (PyObject *)v;

  onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_XDECREF(v);
    return NULL;
}

/* Charmap encoding: the lookup table */

struct encoding_map {
    PyObject_HEAD
    unsigned char level1[32];
    int count2, count3;
    unsigned char level23[1];
};

static PyObject*
encoding_map_size(PyObject *obj, PyObject* args)
{
    struct encoding_map *map = (struct encoding_map*)obj;
    return PyLong_FromLong(sizeof(*map) - 1 + 16*map->count2 +
                           128*map->count3);
}

static PyMethodDef encoding_map_methods[] = {
    {"size", encoding_map_size, METH_NOARGS,
     PyDoc_STR("Return the size (in bytes) of this object") },
    { 0 }
};

static void
encoding_map_dealloc(PyObject* o)
{
    PyObject_FREE(o);
}

static PyTypeObject EncodingMapType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "EncodingMap",          /*tp_name*/
    sizeof(struct encoding_map),   /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    encoding_map_dealloc,   /*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_reserved*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    0,                      /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    encoding_map_methods,   /*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    0,                      /*tp_init*/
    0,                      /*tp_alloc*/
    0,                      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
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
    void *data;
    Py_UCS4 ch;

    if (!PyUnicode_Check(string) || PyUnicode_GET_LENGTH(string) != 256) {
        PyErr_BadArgument();
        return NULL;
    }
    kind = PyUnicode_KIND(string);
    data = PyUnicode_DATA(string);
    memset(level1, 0xFF, sizeof level1);
    memset(level2, 0xFF, sizeof level2);

    /* If there isn't a one-to-one mapping of NULL to \0,
       or if there are non-BMP characters, we need to use
       a mapping dictionary. */
    if (PyUnicode_READ(kind, data, 0) != 0)
        need_dict = 1;
    for (i = 1; i < 256; i++) {
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
        PyObject *key, *value;
        if (!result)
            return NULL;
        for (i = 0; i < 256; i++) {
            key = PyLong_FromLong(PyUnicode_READ(kind, data, i));
            value = PyLong_FromLong(i);
            if (!key || !value)
                goto failed1;
            if (PyDict_SetItem(result, key, value) == -1)
                goto failed1;
            Py_DECREF(key);
            Py_DECREF(value);
        }
        return result;
      failed1:
        Py_XDECREF(key);
        Py_XDECREF(value);
        Py_DECREF(result);
        return NULL;
    }

    /* Create a three-level trie */
    result = PyObject_MALLOC(sizeof(struct encoding_map) +
                             16*count2 + 128*count3 - 1);
    if (!result)
        return PyErr_NoMemory();
    PyObject_Init(result, &EncodingMapType);
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
    for (i = 1; i < 256; i++) {
        int o1, o2, o3, i2, i3;
        if (PyUnicode_READ(kind, data, i) == 0xFFFE)
            /* unmapped character */
            continue;
        o1 = PyUnicode_READ(kind, data, i)>>11;
        o2 = (PyUnicode_READ(kind, data, i)>>7) & 0xF;
        i2 = 16*mlevel1[o1] + o2;
        if (mlevel2[i2] == 0xFF)
            mlevel2[i2] = count3++;
        o3 = PyUnicode_READ(kind, data, i) & 0x7F;
        i3 = 128*mlevel2[i2] + o3;
        mlevel3[i3] = i;
    }
    return result;
}

static int
encoding_map_lookup(Py_UNICODE c, PyObject *mapping)
{
    struct encoding_map *map = (struct encoding_map*)mapping;
    int l1 = c>>11;
    int l2 = (c>>7) & 0xF;
    int l3 = c & 0x7F;
    int i;

#ifdef Py_UNICODE_WIDE
    if (c > 0xFFFF) {
        return -1;
    }
#endif
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

/* Lookup the character ch in the mapping. If the character
   can't be found, Py_None is returned (or NULL, if another
   error occurred). */
static PyObject *
charmapencode_lookup(Py_UNICODE c, PyObject *mapping)
{
    PyObject *w = PyLong_FromLong((long)c);
    PyObject *x;

    if (w == NULL)
        return NULL;
    x = PyObject_GetItem(mapping, w);
    Py_DECREF(w);
    if (x == NULL) {
        if (PyErr_ExceptionMatches(PyExc_LookupError)) {
            /* No mapping found means: mapping is undefined. */
            PyErr_Clear();
            x = Py_None;
            Py_INCREF(x);
            return x;
        } else
            return NULL;
    }
    else if (x == Py_None)
        return x;
    else if (PyLong_Check(x)) {
        long value = PyLong_AS_LONG(x);
        if (value < 0 || value > 255) {
            PyErr_SetString(PyExc_TypeError,
                            "character mapping must be in range(256)");
            Py_DECREF(x);
            return NULL;
        }
        return x;
    }
    else if (PyBytes_Check(x))
        return x;
    else {
        /* wrong return value */
        PyErr_Format(PyExc_TypeError,
                     "character mapping must return integer, bytes or None, not %.400s",
                     x->ob_type->tp_name);
        Py_DECREF(x);
        return NULL;
    }
}

static int
charmapencode_resize(PyObject **outobj, Py_ssize_t *outpos, Py_ssize_t requiredsize)
{
    Py_ssize_t outsize = PyBytes_GET_SIZE(*outobj);
    /* exponentially overallocate to minimize reallocations */
    if (requiredsize < 2*outsize)
        requiredsize = 2*outsize;
    if (_PyBytes_Resize(outobj, requiredsize))
        return -1;
    return 0;
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
charmapencode_output(Py_UNICODE c, PyObject *mapping,
                     PyObject **outobj, Py_ssize_t *outpos)
{
    PyObject *rep;
    char *outstart;
    Py_ssize_t outsize = PyBytes_GET_SIZE(*outobj);

    if (Py_TYPE(mapping) == &EncodingMapType) {
        int res = encoding_map_lookup(c, mapping);
        Py_ssize_t requiredsize = *outpos+1;
        if (res == -1)
            return enc_FAILED;
        if (outsize<requiredsize)
            if (charmapencode_resize(outobj, outpos, requiredsize))
                return enc_EXCEPTION;
        outstart = PyBytes_AS_STRING(*outobj);
        outstart[(*outpos)++] = (char)res;
        return enc_SUCCESS;
    }

    rep = charmapencode_lookup(c, mapping);
    if (rep==NULL)
        return enc_EXCEPTION;
    else if (rep==Py_None) {
        Py_DECREF(rep);
        return enc_FAILED;
    } else {
        if (PyLong_Check(rep)) {
            Py_ssize_t requiredsize = *outpos+1;
            if (outsize<requiredsize)
                if (charmapencode_resize(outobj, outpos, requiredsize)) {
                    Py_DECREF(rep);
                    return enc_EXCEPTION;
                }
            outstart = PyBytes_AS_STRING(*outobj);
            outstart[(*outpos)++] = (char)PyLong_AS_LONG(rep);
        }
        else {
            const char *repchars = PyBytes_AS_STRING(rep);
            Py_ssize_t repsize = PyBytes_GET_SIZE(rep);
            Py_ssize_t requiredsize = *outpos+repsize;
            if (outsize<requiredsize)
                if (charmapencode_resize(outobj, outpos, requiredsize)) {
                    Py_DECREF(rep);
                    return enc_EXCEPTION;
                }
            outstart = PyBytes_AS_STRING(*outobj);
            memcpy(outstart + *outpos, repchars, repsize);
            *outpos += repsize;
        }
    }
    Py_DECREF(rep);
    return enc_SUCCESS;
}

/* handle an error in PyUnicode_EncodeCharmap
   Return 0 on success, -1 on error */
static int
charmap_encoding_error(
    const Py_UNICODE *p, Py_ssize_t size, Py_ssize_t *inpos, PyObject *mapping,
    PyObject **exceptionObject,
    int *known_errorHandler, PyObject **errorHandler, const char *errors,
    PyObject **res, Py_ssize_t *respos)
{
    PyObject *repunicode = NULL; /* initialize to prevent gcc warning */
    Py_ssize_t repsize;
    Py_ssize_t newpos;
    Py_UNICODE *uni2;
    /* startpos for collecting unencodable chars */
    Py_ssize_t collstartpos = *inpos;
    Py_ssize_t collendpos = *inpos+1;
    Py_ssize_t collpos;
    char *encoding = "charmap";
    char *reason = "character maps to <undefined>";
    charmapencode_result x;

    /* find all unencodable characters */
    while (collendpos < size) {
        PyObject *rep;
        if (Py_TYPE(mapping) == &EncodingMapType) {
            int res = encoding_map_lookup(p[collendpos], mapping);
            if (res != -1)
                break;
            ++collendpos;
            continue;
        }

        rep = charmapencode_lookup(p[collendpos], mapping);
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
    if (*known_errorHandler==-1) {
        if ((errors==NULL) || (!strcmp(errors, "strict")))
            *known_errorHandler = 1;
        else if (!strcmp(errors, "replace"))
            *known_errorHandler = 2;
        else if (!strcmp(errors, "ignore"))
            *known_errorHandler = 3;
        else if (!strcmp(errors, "xmlcharrefreplace"))
            *known_errorHandler = 4;
        else
            *known_errorHandler = 0;
    }
    switch (*known_errorHandler) {
    case 1: /* strict */
        raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
        return -1;
    case 2: /* replace */
        for (collpos = collstartpos; collpos<collendpos; ++collpos) {
            x = charmapencode_output('?', mapping, res, respos);
            if (x==enc_EXCEPTION) {
                return -1;
            }
            else if (x==enc_FAILED) {
                raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
                return -1;
            }
        }
        /* fall through */
    case 3: /* ignore */
        *inpos = collendpos;
        break;
    case 4: /* xmlcharrefreplace */
        /* generate replacement (temporarily (mis)uses p) */
        for (collpos = collstartpos; collpos < collendpos; ++collpos) {
            char buffer[2+29+1+1];
            char *cp;
            sprintf(buffer, "&#%d;", (int)p[collpos]);
            for (cp = buffer; *cp; ++cp) {
                x = charmapencode_output(*cp, mapping, res, respos);
                if (x==enc_EXCEPTION)
                    return -1;
                else if (x==enc_FAILED) {
                    raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
                    return -1;
                }
            }
        }
        *inpos = collendpos;
        break;
    default:
        repunicode = unicode_encode_call_errorhandler(errors, errorHandler,
                                                      encoding, reason, p, size, exceptionObject,
                                                      collstartpos, collendpos, &newpos);
        if (repunicode == NULL)
            return -1;
        if (PyBytes_Check(repunicode)) {
            /* Directly copy bytes result to output. */
            Py_ssize_t outsize = PyBytes_Size(*res);
            Py_ssize_t requiredsize;
            repsize = PyBytes_Size(repunicode);
            requiredsize = *respos + repsize;
            if (requiredsize > outsize)
                /* Make room for all additional bytes. */
                if (charmapencode_resize(res, respos, requiredsize)) {
                    Py_DECREF(repunicode);
                    return -1;
                }
            memcpy(PyBytes_AsString(*res) + *respos,
                   PyBytes_AsString(repunicode),  repsize);
            *respos += repsize;
            *inpos = newpos;
            Py_DECREF(repunicode);
            break;
        }
        /* generate replacement  */
        repsize = PyUnicode_GET_SIZE(repunicode);
        for (uni2 = PyUnicode_AS_UNICODE(repunicode); repsize-->0; ++uni2) {
            x = charmapencode_output(*uni2, mapping, res, respos);
            if (x==enc_EXCEPTION) {
                return -1;
            }
            else if (x==enc_FAILED) {
                Py_DECREF(repunicode);
                raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
                return -1;
            }
        }
        *inpos = newpos;
        Py_DECREF(repunicode);
    }
    return 0;
}

PyObject *
PyUnicode_EncodeCharmap(const Py_UNICODE *p,
                        Py_ssize_t size,
                        PyObject *mapping,
                        const char *errors)
{
    /* output object */
    PyObject *res = NULL;
    /* current input position */
    Py_ssize_t inpos = 0;
    /* current output position */
    Py_ssize_t respos = 0;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace,
     * 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    /* Default to Latin-1 */
    if (mapping == NULL)
        return PyUnicode_EncodeLatin1(p, size, errors);

    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    res = PyBytes_FromStringAndSize(NULL, size);
    if (res == NULL)
        goto onError;
    if (size == 0)
        return res;

    while (inpos<size) {
        /* try to encode it */
        charmapencode_result x = charmapencode_output(p[inpos], mapping, &res, &respos);
        if (x==enc_EXCEPTION) /* error */
            goto onError;
        if (x==enc_FAILED) { /* unencodable character */
            if (charmap_encoding_error(p, size, &inpos, mapping,
                                       &exc,
                                       &known_errorHandler, &errorHandler, errors,
                                       &res, &respos)) {
                goto onError;
            }
        }
        else
            /* done with this character => adjust input position */
            ++inpos;
    }

    /* Resize if we allocated to much */
    if (respos<PyBytes_GET_SIZE(res))
        if (_PyBytes_Resize(&res, respos) < 0)
            goto onError;

    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return res;

  onError:
    Py_XDECREF(res);
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
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
    return PyUnicode_EncodeCharmap(PyUnicode_AS_UNICODE(unicode),
                                   PyUnicode_GET_SIZE(unicode),
                                   mapping,
                                   NULL);
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
        Py_DECREF(*exceptionObject);
        *exceptionObject = NULL;
    }
}

/* raises a UnicodeTranslateError */
static void
raise_translate_exception(PyObject **exceptionObject,
                          PyObject *unicode,
                          Py_ssize_t startpos, Py_ssize_t endpos,
                          const char *reason)
{
    make_translate_exception(exceptionObject,
                             unicode, startpos, endpos, reason);
    if (*exceptionObject != NULL)
        PyCodec_StrictErrors(*exceptionObject);
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
    static char *argparse = "O!n;translating error handler must return (str, int) tuple";

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

    restuple = PyObject_CallFunctionObjArgs(
        *errorHandler, *exceptionObject, NULL);
    if (restuple == NULL)
        return NULL;
    if (!PyTuple_Check(restuple)) {
        PyErr_SetString(PyExc_TypeError, &argparse[4]);
        Py_DECREF(restuple);
        return NULL;
    }
    if (!PyArg_ParseTuple(restuple, argparse, &PyUnicode_Type,
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
   Return 0 on success, -1 on error */
static int
charmaptranslate_lookup(Py_UCS4 c, PyObject *mapping, PyObject **result)
{
    PyObject *w = PyLong_FromLong((long)c);
    PyObject *x;

    if (w == NULL)
        return -1;
    x = PyObject_GetItem(mapping, w);
    Py_DECREF(w);
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
        long value = PyLong_AS_LONG(x);
        long max = PyUnicode_GetMax();
        if (value < 0 || value > max) {
            PyErr_Format(PyExc_TypeError,
                         "character mapping must be in range(0x%x)", max+1);
            Py_DECREF(x);
            return -1;
        }
        *result = x;
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
/* ensure that *outobj is at least requiredsize characters long,
   if not reallocate and adjust various state variables.
   Return 0 on success, -1 on error */
static int
charmaptranslate_makespace(Py_UCS4 **outobj, Py_ssize_t *psize,
                               Py_ssize_t requiredsize)
{
    Py_ssize_t oldsize = *psize;
    if (requiredsize > oldsize) {
        /* exponentially overallocate to minimize reallocations */
        if (requiredsize < 2 * oldsize)
            requiredsize = 2 * oldsize;
        *outobj = PyMem_Realloc(*outobj, requiredsize * sizeof(Py_UCS4));
        if (*outobj == 0)
            return -1;
        *psize = requiredsize;
    }
    return 0;
}
/* lookup the character, put the result in the output string and adjust
   various state variables. Return a new reference to the object that
   was put in the output buffer in *result, or Py_None, if the mapping was
   undefined (in which case no character was written).
   The called must decref result.
   Return 0 on success, -1 on error. */
static int
charmaptranslate_output(PyObject *input, Py_ssize_t ipos,
                        PyObject *mapping, Py_UCS4 **output,
                        Py_ssize_t *osize, Py_ssize_t *opos,
                        PyObject **res)
{
    Py_UCS4 curinp = PyUnicode_READ_CHAR(input, ipos);
    if (charmaptranslate_lookup(curinp, mapping, res))
        return -1;
    if (*res==NULL) {
        /* not found => default to 1:1 mapping */
        (*output)[(*opos)++] = curinp;
    }
    else if (*res==Py_None)
        ;
    else if (PyLong_Check(*res)) {
        /* no overflow check, because we know that the space is enough */
        (*output)[(*opos)++] = (Py_UCS4)PyLong_AS_LONG(*res);
    }
    else if (PyUnicode_Check(*res)) {
        Py_ssize_t repsize;
        if (PyUnicode_READY(*res) == -1)
            return -1;
        repsize = PyUnicode_GET_LENGTH(*res);
        if (repsize==1) {
            /* no overflow check, because we know that the space is enough */
            (*output)[(*opos)++] = PyUnicode_READ_CHAR(*res, 0);
        }
        else if (repsize!=0) {
            /* more than one character */
            Py_ssize_t requiredsize = *opos +
                (PyUnicode_GET_LENGTH(input) - ipos) +
                repsize - 1;
            Py_ssize_t i;
            if (charmaptranslate_makespace(output, osize, requiredsize))
                return -1;
            for(i = 0; i < repsize; i++)
                (*output)[(*opos)++] = PyUnicode_READ_CHAR(*res, i);
        }
    }
    else
        return -1;
    return 0;
}

PyObject *
_PyUnicode_TranslateCharmap(PyObject *input,
                            PyObject *mapping,
                            const char *errors)
{
    /* input object */
    char *idata;
    Py_ssize_t size, i;
    int kind;
    /* output buffer */
    Py_UCS4 *output = NULL;
    Py_ssize_t osize;
    PyObject *res;
    /* current output position */
    Py_ssize_t opos;
    char *reason = "character maps to <undefined>";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace,
     * 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    if (mapping == NULL) {
        PyErr_BadArgument();
        return NULL;
    }

    if (PyUnicode_READY(input) == -1)
        return NULL;
    idata = (char*)PyUnicode_DATA(input);
    kind = PyUnicode_KIND(input);
    size = PyUnicode_GET_LENGTH(input);
    i = 0;

    if (size == 0) {
        Py_INCREF(input);
        return input;
    }

    /* allocate enough for a simple 1:1 translation without
       replacements, if we need more, we'll resize */
    osize = size;
    output = PyMem_Malloc(osize * sizeof(Py_UCS4));
    opos = 0;
    if (output == NULL) {
        PyErr_NoMemory();
        goto onError;
    }

    while (i<size) {
        /* try to encode it */
        PyObject *x = NULL;
        if (charmaptranslate_output(input, i, mapping,
                                    &output, &osize, &opos, &x)) {
            Py_XDECREF(x);
            goto onError;
        }
        Py_XDECREF(x);
        if (x!=Py_None) /* it worked => adjust input pointer */
            ++i;
        else { /* untranslatable character */
            PyObject *repunicode = NULL; /* initialize to prevent gcc warning */
            Py_ssize_t repsize;
            Py_ssize_t newpos;
            Py_ssize_t uni2;
            /* startpos for collecting untranslatable chars */
            Py_ssize_t collstart = i;
            Py_ssize_t collend = i+1;
            Py_ssize_t coll;

            /* find all untranslatable characters */
            while (collend < size) {
                if (charmaptranslate_lookup(PyUnicode_READ(kind,idata, collend), mapping, &x))
                    goto onError;
                Py_XDECREF(x);
                if (x!=Py_None)
                    break;
                ++collend;
            }
            /* cache callback name lookup
             * (if not done yet, i.e. it's the first error) */
            if (known_errorHandler==-1) {
                if ((errors==NULL) || (!strcmp(errors, "strict")))
                    known_errorHandler = 1;
                else if (!strcmp(errors, "replace"))
                    known_errorHandler = 2;
                else if (!strcmp(errors, "ignore"))
                    known_errorHandler = 3;
                else if (!strcmp(errors, "xmlcharrefreplace"))
                    known_errorHandler = 4;
                else
                    known_errorHandler = 0;
            }
            switch (known_errorHandler) {
            case 1: /* strict */
                raise_translate_exception(&exc, input, collstart,
                                          collend, reason);
                goto onError;
            case 2: /* replace */
                /* No need to check for space, this is a 1:1 replacement */
                for (coll = collstart; coll<collend; coll++)
                    output[opos++] = '?';
                /* fall through */
            case 3: /* ignore */
                i = collend;
                break;
            case 4: /* xmlcharrefreplace */
                /* generate replacement (temporarily (mis)uses i) */
                for (i = collstart; i < collend; ++i) {
                    char buffer[2+29+1+1];
                    char *cp;
                    sprintf(buffer, "&#%d;", PyUnicode_READ(kind, idata, i));
                    if (charmaptranslate_makespace(&output, &osize,
                                                   opos+strlen(buffer)+(size-collend)))
                        goto onError;
                    for (cp = buffer; *cp; ++cp)
                        output[opos++] = *cp;
                }
                i = collend;
                break;
            default:
                repunicode = unicode_translate_call_errorhandler(errors, &errorHandler,
                                                                 reason, input, &exc,
                                                                 collstart, collend, &newpos);
                if (repunicode == NULL || _PyUnicode_READY_REPLACE(&repunicode))
                    goto onError;
                /* generate replacement  */
                repsize = PyUnicode_GET_LENGTH(repunicode);
                if (charmaptranslate_makespace(&output, &osize,
                                               opos+repsize+(size-collend))) {
                    Py_DECREF(repunicode);
                    goto onError;
                }
                for (uni2 = 0; repsize-->0; ++uni2)
                    output[opos++] = PyUnicode_READ_CHAR(repunicode, uni2);
                i = newpos;
                Py_DECREF(repunicode);
            }
        }
    }
    res = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, output, opos);
    if (!res)
        goto onError;
    PyMem_Free(output);
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return res;

  onError:
    PyMem_Free(output);
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return NULL;
}

/* Deprecated. Use PyUnicode_Translate instead. */
PyObject *
PyUnicode_TranslateCharmap(const Py_UNICODE *p,
                           Py_ssize_t size,
                           PyObject *mapping,
                           const char *errors)
{
    PyObject *unicode = PyUnicode_FromUnicode(p, size);
    if (!unicode)
        return NULL;
    return _PyUnicode_TranslateCharmap(unicode, mapping, errors);
}

PyObject *
PyUnicode_Translate(PyObject *str,
                    PyObject *mapping,
                    const char *errors)
{
    PyObject *result;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
        goto onError;
    result = _PyUnicode_TranslateCharmap(str, mapping, errors);
    Py_DECREF(str);
    return result;

  onError:
    Py_XDECREF(str);
    return NULL;
}

static Py_UCS4
fix_decimal_and_space_to_ascii(PyUnicodeObject *self)
{
    /* No need to call PyUnicode_READY(self) because this function is only
       called as a callback from fixup() which does it already. */
    const Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    const int kind = PyUnicode_KIND(self);
    void *data = PyUnicode_DATA(self);
    Py_UCS4 maxchar = 0, ch, fixed;
    Py_ssize_t i;

    for (i = 0; i < len; ++i) {
        ch = PyUnicode_READ(kind, data, i);
        fixed = 0;
        if (ch > 127) {
            if (Py_UNICODE_ISSPACE(ch))
                fixed = ' ';
            else {
                const int decimal = Py_UNICODE_TODECIMAL(ch);
                if (decimal >= 0)
                    fixed = '0' + decimal;
            }
            if (fixed != 0) {
                if (fixed > maxchar)
                    maxchar = fixed;
                PyUnicode_WRITE(kind, data, i, fixed);
            }
            else if (ch > maxchar)
                maxchar = ch;
        }
        else if (ch > maxchar)
            maxchar = ch;
    }

    return maxchar;
}

PyObject *
_PyUnicode_TransformDecimalAndSpaceToASCII(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (PyUnicode_READY(unicode) == -1)
        return NULL;
    if (PyUnicode_MAX_CHAR_VALUE(unicode) <= 127) {
        /* If the string is already ASCII, just return the same string */
        Py_INCREF(unicode);
        return unicode;
    }
    return fixup((PyUnicodeObject *)unicode, fix_decimal_and_space_to_ascii);
}

PyObject *
PyUnicode_TransformDecimalToASCII(Py_UNICODE *s,
                                  Py_ssize_t length)
{
    PyObject *result;
    Py_UNICODE *p; /* write pointer into result */
    Py_ssize_t i;
    /* Copy to a new string */
    result = (PyObject *)_PyUnicode_New(length);
    Py_UNICODE_COPY(PyUnicode_AS_UNICODE(result), s, length);
    if (result == NULL)
        return result;
    p = PyUnicode_AS_UNICODE(result);
    /* Iterate over code points */
    for (i = 0; i < length; i++) {
        Py_UNICODE ch =s[i];
        if (ch > 127) {
            int decimal = Py_UNICODE_TODECIMAL(ch);
            if (decimal >= 0)
                p[i] = '0' + decimal;
        }
    }
    if (PyUnicode_READY((PyUnicodeObject*)result) == -1) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}
/* --- Decimal Encoder ---------------------------------------------------- */

int
PyUnicode_EncodeDecimal(Py_UNICODE *s,
                        Py_ssize_t length,
                        char *output,
                        const char *errors)
{
    Py_UNICODE *p, *end;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    const char *encoding = "decimal";
    const char *reason = "invalid decimal Unicode string";
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace, 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    if (output == NULL) {
        PyErr_BadArgument();
        return -1;
    }

    p = s;
    end = s + length;
    while (p < end) {
        register Py_UNICODE ch = *p;
        int decimal;
        PyObject *repunicode;
        Py_ssize_t repsize;
        Py_ssize_t newpos;
        Py_UNICODE *uni2;
        Py_UNICODE *collstart;
        Py_UNICODE *collend;

        if (Py_UNICODE_ISSPACE(ch)) {
            *output++ = ' ';
            ++p;
            continue;
        }
        decimal = Py_UNICODE_TODECIMAL(ch);
        if (decimal >= 0) {
            *output++ = '0' + decimal;
            ++p;
            continue;
        }
        if (0 < ch && ch < 256) {
            *output++ = (char)ch;
            ++p;
            continue;
        }
        /* All other characters are considered unencodable */
        collstart = p;
        collend = p+1;
        while (collend < end) {
            if ((0 < *collend && *collend < 256) ||
                !Py_UNICODE_ISSPACE(*collend) ||
                Py_UNICODE_TODECIMAL(*collend))
                break;
        }
        /* cache callback name lookup
         * (if not done yet, i.e. it's the first error) */
        if (known_errorHandler==-1) {
            if ((errors==NULL) || (!strcmp(errors, "strict")))
                known_errorHandler = 1;
            else if (!strcmp(errors, "replace"))
                known_errorHandler = 2;
            else if (!strcmp(errors, "ignore"))
                known_errorHandler = 3;
            else if (!strcmp(errors, "xmlcharrefreplace"))
                known_errorHandler = 4;
            else
                known_errorHandler = 0;
        }
        switch (known_errorHandler) {
        case 1: /* strict */
            raise_encode_exception(&exc, encoding, s, length, collstart-s, collend-s, reason);
            goto onError;
        case 2: /* replace */
            for (p = collstart; p < collend; ++p)
                *output++ = '?';
            /* fall through */
        case 3: /* ignore */
            p = collend;
            break;
        case 4: /* xmlcharrefreplace */
            /* generate replacement (temporarily (mis)uses p) */
            for (p = collstart; p < collend; ++p)
                output += sprintf(output, "&#%d;", (int)*p);
            p = collend;
            break;
        default:
            repunicode = unicode_encode_call_errorhandler(errors, &errorHandler,
                                                          encoding, reason, s, length, &exc,
                                                          collstart-s, collend-s, &newpos);
            if (repunicode == NULL)
                goto onError;
            if (!PyUnicode_Check(repunicode)) {
                /* Byte results not supported, since they have no decimal property. */
                PyErr_SetString(PyExc_TypeError, "error handler should return unicode");
                Py_DECREF(repunicode);
                goto onError;
            }
            /* generate replacement  */
            repsize = PyUnicode_GET_SIZE(repunicode);
            for (uni2 = PyUnicode_AS_UNICODE(repunicode); repsize-->0; ++uni2) {
                Py_UNICODE ch = *uni2;
                if (Py_UNICODE_ISSPACE(ch))
                    *output++ = ' ';
                else {
                    decimal = Py_UNICODE_TODECIMAL(ch);
                    if (decimal >= 0)
                        *output++ = '0' + decimal;
                    else if (0 < ch && ch < 256)
                        *output++ = (char)ch;
                    else {
                        Py_DECREF(repunicode);
                        raise_encode_exception(&exc, encoding,
                                               s, length, collstart-s, collend-s, reason);
                        goto onError;
                    }
                }
            }
            p = s + newpos;
            Py_DECREF(repunicode);
        }
    }
    /* 0-terminate the output string */
    *output++ = '\0';
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return 0;

  onError:
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return -1;
}

/* --- Helpers ------------------------------------------------------------ */

#include "stringlib/ucs1lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/localeutil.h"
#include "stringlib/undef.h"

#include "stringlib/ucs2lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/localeutil.h"
#include "stringlib/undef.h"

#include "stringlib/ucs4lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/localeutil.h"
#include "stringlib/undef.h"

static Py_ssize_t
any_find_slice(Py_ssize_t Py_LOCAL_CALLBACK(ucs1)(const Py_UCS1*, Py_ssize_t,
                                  const Py_UCS1*, Py_ssize_t,
                                  Py_ssize_t, Py_ssize_t),
               Py_ssize_t Py_LOCAL_CALLBACK(ucs2)(const Py_UCS2*, Py_ssize_t,
                                  const Py_UCS2*, Py_ssize_t,
                                  Py_ssize_t, Py_ssize_t),
               Py_ssize_t Py_LOCAL_CALLBACK(ucs4)(const Py_UCS4*, Py_ssize_t,
                                  const Py_UCS4*, Py_ssize_t,
                                  Py_ssize_t, Py_ssize_t),
               PyObject* s1, PyObject* s2,
               Py_ssize_t start,
               Py_ssize_t end)
{
    int kind1, kind2, kind;
    void *buf1, *buf2;
    Py_ssize_t len1, len2, result;

    kind1 = PyUnicode_KIND(s1);
    kind2 = PyUnicode_KIND(s2);
    kind = kind1 > kind2 ? kind1 : kind2;
    buf1 = PyUnicode_DATA(s1);
    buf2 = PyUnicode_DATA(s2);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind(s1, kind);
    if (!buf1)
        return -2;
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind(s2, kind);
    if (!buf2) {
        if (kind1 != kind) PyMem_Free(buf1);
        return -2;
    }
    len1 = PyUnicode_GET_LENGTH(s1);
    len2 = PyUnicode_GET_LENGTH(s2);

    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        result = ucs1(buf1, len1, buf2, len2, start, end);
        break;
    case PyUnicode_2BYTE_KIND:
        result = ucs2(buf1, len1, buf2, len2, start, end);
        break;
    case PyUnicode_4BYTE_KIND:
        result = ucs4(buf1, len1, buf2, len2, start, end);
        break;
    default:
        assert(0); result = -2;
    }

    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);

    return result;
}

Py_ssize_t
_PyUnicode_InsertThousandsGrouping(int kind, void *data,
                                   Py_ssize_t n_buffer,
                                   void *digits, Py_ssize_t n_digits,
                                   Py_ssize_t min_width,
                                   const char *grouping,
                                   const char *thousands_sep)
{
    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        return _PyUnicode_ucs1_InsertThousandsGrouping(
            (Py_UCS1*)data, n_buffer, (Py_UCS1*)digits, n_digits,
            min_width, grouping, thousands_sep);
    case PyUnicode_2BYTE_KIND:
        return _PyUnicode_ucs2_InsertThousandsGrouping(
            (Py_UCS2*)data, n_buffer, (Py_UCS2*)digits, n_digits,
            min_width, grouping, thousands_sep);
    case PyUnicode_4BYTE_KIND:
        return _PyUnicode_ucs4_InsertThousandsGrouping(
            (Py_UCS4*)data, n_buffer, (Py_UCS4*)digits, n_digits,
            min_width, grouping, thousands_sep);
    }
    assert(0);
    return -1;
}


#include "stringlib/unicodedefs.h"
#include "stringlib/fastsearch.h"

#include "stringlib/count.h"
#include "stringlib/find.h"

/* helper macro to fixup start/end slice values */
#define ADJUST_INDICES(start, end, len)         \
    if (end > len)                              \
        end = len;                              \
    else if (end < 0) {                         \
        end += len;                             \
        if (end < 0)                            \
            end = 0;                            \
    }                                           \
    if (start < 0) {                            \
        start += len;                           \
        if (start < 0)                          \
            start = 0;                          \
    }

Py_ssize_t
PyUnicode_Count(PyObject *str,
                PyObject *substr,
                Py_ssize_t start,
                Py_ssize_t end)
{
    Py_ssize_t result;
    PyUnicodeObject* str_obj;
    PyUnicodeObject* sub_obj;
    int kind1, kind2, kind;
    void *buf1 = NULL, *buf2 = NULL;
    Py_ssize_t len1, len2;

    str_obj = (PyUnicodeObject*) PyUnicode_FromObject(str);
    if (!str_obj || PyUnicode_READY(str_obj) == -1)
        return -1;
    sub_obj = (PyUnicodeObject*) PyUnicode_FromObject(substr);
    if (!sub_obj || PyUnicode_READY(sub_obj) == -1) {
        Py_DECREF(str_obj);
        return -1;
    }

    kind1 = PyUnicode_KIND(str_obj);
    kind2 = PyUnicode_KIND(sub_obj);
    kind = kind1 > kind2 ? kind1 : kind2;
    buf1 = PyUnicode_DATA(str_obj);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind((PyObject*)str_obj, kind);
    if (!buf1)
        goto onError;
    buf2 = PyUnicode_DATA(sub_obj);
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind((PyObject*)sub_obj, kind);
    if (!buf2)
        goto onError;
    len1 = PyUnicode_GET_LENGTH(str_obj);
    len2 = PyUnicode_GET_LENGTH(sub_obj);

    ADJUST_INDICES(start, end, len1);
    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        result = ucs1lib_count(
            ((Py_UCS1*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_2BYTE_KIND:
        result = ucs2lib_count(
            ((Py_UCS2*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_4BYTE_KIND:
        result = ucs4lib_count(
            ((Py_UCS4*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    default:
        assert(0); result = 0;
    }

    Py_DECREF(sub_obj);
    Py_DECREF(str_obj);

    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);

    return result;
  onError:
    Py_DECREF(sub_obj);
    Py_DECREF(str_obj);
    if (kind1 != kind && buf1)
        PyMem_Free(buf1);
    if (kind2 != kind && buf2)
        PyMem_Free(buf2);
    return -1;
}

Py_ssize_t
PyUnicode_Find(PyObject *str,
               PyObject *sub,
               Py_ssize_t start,
               Py_ssize_t end,
               int direction)
{
    Py_ssize_t result;

    str = PyUnicode_FromObject(str);
    if (!str || PyUnicode_READY(str) == -1)
        return -2;
    sub = PyUnicode_FromObject(sub);
    if (!sub || PyUnicode_READY(sub) == -1) {
        Py_DECREF(str);
        return -2;
    }

    if (direction > 0)
        result = any_find_slice(
            ucs1lib_find_slice, ucs2lib_find_slice, ucs4lib_find_slice,
            str, sub, start, end
            );
    else
        result = any_find_slice(
            ucs1lib_rfind_slice, ucs2lib_rfind_slice, ucs4lib_rfind_slice,
            str, sub, start, end
            );

    Py_DECREF(str);
    Py_DECREF(sub);

    return result;
}

Py_ssize_t
PyUnicode_FindChar(PyObject *str, Py_UCS4 ch,
                   Py_ssize_t start, Py_ssize_t end,
                   int direction)
{
    char *result;
    int kind;
    if (PyUnicode_READY(str) == -1)
        return -2;
    if (start < 0 || end < 0) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -2;
    }
    if (end > PyUnicode_GET_LENGTH(str))
        end = PyUnicode_GET_LENGTH(str);
    kind = PyUnicode_KIND(str);
    result = findchar(PyUnicode_1BYTE_DATA(str)
                      + PyUnicode_KIND_SIZE(kind, start),
                      kind,
                      end-start, ch, direction);
    if (!result)
        return -1;
    return (result-(char*)PyUnicode_DATA(str)) >> (kind-1);
}

static int
tailmatch(PyUnicodeObject *self,
          PyUnicodeObject *substring,
          Py_ssize_t start,
          Py_ssize_t end,
          int direction)
{
    int kind_self;
    int kind_sub;
    void *data_self;
    void *data_sub;
    Py_ssize_t offset;
    Py_ssize_t i;
    Py_ssize_t end_sub;

    if (PyUnicode_READY(self) == -1 ||
        PyUnicode_READY(substring) == -1)
        return 0;

    if (PyUnicode_GET_LENGTH(substring) == 0)
        return 1;

    ADJUST_INDICES(start, end, PyUnicode_GET_LENGTH(self));
    end -= PyUnicode_GET_LENGTH(substring);
    if (end < start)
        return 0;

    kind_self = PyUnicode_KIND(self);
    data_self = PyUnicode_DATA(self);
    kind_sub = PyUnicode_KIND(substring);
    data_sub = PyUnicode_DATA(substring);
    end_sub = PyUnicode_GET_LENGTH(substring) - 1;

    if (direction > 0)
        offset = end;
    else
        offset = start;

    if (PyUnicode_READ(kind_self, data_self, offset) ==
        PyUnicode_READ(kind_sub, data_sub, 0) &&
        PyUnicode_READ(kind_self, data_self, offset + end_sub) ==
        PyUnicode_READ(kind_sub, data_sub, end_sub)) {
        /* If both are of the same kind, memcmp is sufficient */
        if (kind_self == kind_sub) {
            return ! memcmp((char *)data_self +
                                (offset * PyUnicode_CHARACTER_SIZE(substring)),
                            data_sub,
                            PyUnicode_GET_LENGTH(substring) *
                                PyUnicode_CHARACTER_SIZE(substring));
        }
        /* otherwise we have to compare each character by first accesing it */
        else {
            /* We do not need to compare 0 and len(substring)-1 because
               the if statement above ensured already that they are equal
               when we end up here. */
            // TODO: honor direction and do a forward or backwards search
            for (i = 1; i < end_sub; ++i) {
                if (PyUnicode_READ(kind_self, data_self, offset + i) !=
                    PyUnicode_READ(kind_sub, data_sub, i))
                    return 0;
            }
            return 1;
        }
    }

    return 0;
}

Py_ssize_t
PyUnicode_Tailmatch(PyObject *str,
                    PyObject *substr,
                    Py_ssize_t start,
                    Py_ssize_t end,
                    int direction)
{
    Py_ssize_t result;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
        return -1;
    substr = PyUnicode_FromObject(substr);
    if (substr == NULL) {
        Py_DECREF(str);
        return -1;
    }

    result = tailmatch((PyUnicodeObject *)str,
                       (PyUnicodeObject *)substr,
                       start, end, direction);
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

/* Apply fixfct filter to the Unicode object self and return a
   reference to the modified object */

static PyObject *
fixup(PyUnicodeObject *self,
      Py_UCS4 (*fixfct)(PyUnicodeObject *s))
{
    PyObject *u;
    Py_UCS4 maxchar_old, maxchar_new = 0;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    maxchar_old = PyUnicode_MAX_CHAR_VALUE(self);
    u = PyUnicode_New(PyUnicode_GET_LENGTH(self),
                      maxchar_old);
    if (u == NULL)
        return NULL;

    Py_MEMCPY(PyUnicode_1BYTE_DATA(u), PyUnicode_1BYTE_DATA(self),
              PyUnicode_GET_LENGTH(u) * PyUnicode_CHARACTER_SIZE(u));

    /* fix functions return the new maximum character in a string,
       if the kind of the resulting unicode object does not change,
       everything is fine.  Otherwise we need to change the string kind
       and re-run the fix function. */
    maxchar_new = fixfct((PyUnicodeObject*)u);
    if (maxchar_new == 0)
        /* do nothing, keep maxchar_new at 0 which means no changes. */;
    else if (maxchar_new <= 127)
        maxchar_new = 127;
    else if (maxchar_new <= 255)
        maxchar_new = 255;
    else if (maxchar_new <= 65535)
        maxchar_new = 65535;
    else
        maxchar_new = 1114111; /* 0x10ffff */

    if (!maxchar_new && PyUnicode_CheckExact(self)) {
        /* fixfct should return TRUE if it modified the buffer. If
           FALSE, return a reference to the original buffer instead
           (to save space, not time) */
        Py_INCREF(self);
        Py_DECREF(u);
        return (PyObject*) self;
    }
    else if (maxchar_new == maxchar_old) {
        return u;
    }
    else {
        /* In case the maximum character changed, we need to
           convert the string to the new category. */
        PyObject *v = PyUnicode_New(PyUnicode_GET_LENGTH(self), maxchar_new);
        if (v == NULL) {
            Py_DECREF(u);
            return NULL;
        }
        if (maxchar_new > maxchar_old) {
            /* If the maxchar increased so that the kind changed, not all
               characters are representable anymore and we need to fix the
               string again. This only happens in very few cases. */
            if (PyUnicode_CopyCharacters(v, 0,
                                         (PyObject*)self, 0,
                                         PyUnicode_GET_LENGTH(self)) < 0)
            {
                Py_DECREF(u);
                return NULL;
            }
            maxchar_old = fixfct((PyUnicodeObject*)v);
            assert(maxchar_old > 0 && maxchar_old <= maxchar_new);
        }
        else {
            if (PyUnicode_CopyCharacters(v, 0,
                                         u, 0,
                                         PyUnicode_GET_LENGTH(self)) < 0)
            {
                Py_DECREF(u);
                return NULL;
            }
        }

        Py_DECREF(u);
        return v;
    }
}

static Py_UCS4
fixupper(PyUnicodeObject *self)
{
    /* No need to call PyUnicode_READY(self) because this function is only
       called as a callback from fixup() which does it already. */
    const Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    const int kind = PyUnicode_KIND(self);
    void *data = PyUnicode_DATA(self);
    int touched = 0;
    Py_UCS4 maxchar = 0;
    Py_ssize_t i;

    for (i = 0; i < len; ++i) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        const Py_UCS4 up = Py_UNICODE_TOUPPER(ch);
        if (up != ch) {
            if (up > maxchar)
                maxchar = up;
            PyUnicode_WRITE(kind, data, i, up);
            touched = 1;
        }
        else if (ch > maxchar)
            maxchar = ch;
    }

    if (touched)
        return maxchar;
    else
        return 0;
}

static Py_UCS4
fixlower(PyUnicodeObject *self)
{
    /* No need to call PyUnicode_READY(self) because fixup() which does it. */
    const Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    const int kind = PyUnicode_KIND(self);
    void *data = PyUnicode_DATA(self);
    int touched = 0;
    Py_UCS4 maxchar = 0;
    Py_ssize_t i;

    for(i = 0; i < len; ++i) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        const Py_UCS4 lo = Py_UNICODE_TOLOWER(ch);
        if (lo != ch) {
            if (lo > maxchar)
                maxchar = lo;
            PyUnicode_WRITE(kind, data, i, lo);
            touched = 1;
        }
        else if (ch > maxchar)
            maxchar = ch;
    }

    if (touched)
        return maxchar;
    else
        return 0;
}

static Py_UCS4
fixswapcase(PyUnicodeObject *self)
{
    /* No need to call PyUnicode_READY(self) because fixup() which does it. */
    const Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    const int kind = PyUnicode_KIND(self);
    void *data = PyUnicode_DATA(self);
    int touched = 0;
    Py_UCS4 maxchar = 0;
    Py_ssize_t i;

    for(i = 0; i < len; ++i) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        Py_UCS4 nu = 0;

        if (Py_UNICODE_ISUPPER(ch))
            nu = Py_UNICODE_TOLOWER(ch);
        else if (Py_UNICODE_ISLOWER(ch))
            nu = Py_UNICODE_TOUPPER(ch);

        if (nu != 0) {
            if (nu > maxchar)
                maxchar = nu;
            PyUnicode_WRITE(kind, data, i, nu);
            touched = 1;
        }
        else if (ch > maxchar)
            maxchar = ch;
    }

    if (touched)
        return maxchar;
    else
        return 0;
}

static Py_UCS4
fixcapitalize(PyUnicodeObject *self)
{
    /* No need to call PyUnicode_READY(self) because fixup() which does it. */
    const Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    const int kind = PyUnicode_KIND(self);
    void *data = PyUnicode_DATA(self);
    int touched = 0;
    Py_UCS4 maxchar = 0;
    Py_ssize_t i = 0;
    Py_UCS4 ch;

    if (len == 0)
        return 0;

    ch = PyUnicode_READ(kind, data, i);
    if (!Py_UNICODE_ISUPPER(ch)) {
        maxchar = Py_UNICODE_TOUPPER(ch);
        PyUnicode_WRITE(kind, data, i, maxchar);
        touched = 1;
    }
    ++i;
    for(; i < len; ++i) {
        ch = PyUnicode_READ(kind, data, i);
        if (!Py_UNICODE_ISLOWER(ch)) {
            const Py_UCS4 lo = Py_UNICODE_TOLOWER(ch);
            if (lo > maxchar)
                maxchar = lo;
            PyUnicode_WRITE(kind, data, i, lo);
            touched = 1;
        }
        else if (ch > maxchar)
            maxchar = ch;
    }

    if (touched)
        return maxchar;
    else
        return 0;
}

static Py_UCS4
fixtitle(PyUnicodeObject *self)
{
    /* No need to call PyUnicode_READY(self) because fixup() which does it. */
    const Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    const int kind = PyUnicode_KIND(self);
    void *data = PyUnicode_DATA(self);
    Py_UCS4 maxchar = 0;
    Py_ssize_t i = 0;
    int previous_is_cased;

    /* Shortcut for single character strings */
    if (len == 1) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        const Py_UCS4 ti = Py_UNICODE_TOTITLE(ch);
        if (ti != ch) {
            PyUnicode_WRITE(kind, data, i, ti);
            return ti;
        }
        else
            return 0;
    }
    previous_is_cased = 0;
    for(; i < len; ++i) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        Py_UCS4 nu;

        if (previous_is_cased)
            nu = Py_UNICODE_TOLOWER(ch);
        else
            nu = Py_UNICODE_TOTITLE(ch);

        if (nu > maxchar)
            maxchar = nu;
        PyUnicode_WRITE(kind, data, i, nu);

        if (Py_UNICODE_ISLOWER(ch) ||
            Py_UNICODE_ISUPPER(ch) ||
            Py_UNICODE_ISTITLE(ch))
            previous_is_cased = 1;
        else
            previous_is_cased = 0;
    }
    return maxchar;
}

PyObject *
PyUnicode_Join(PyObject *separator, PyObject *seq)
{
    PyObject *sep = NULL;
    Py_ssize_t seplen = 1;
    PyObject *res = NULL; /* the result */
    PyObject *fseq;          /* PySequence_Fast(seq) */
    Py_ssize_t seqlen;       /* len(fseq) -- number of items in sequence */
    PyObject **items;
    PyObject *item;
    Py_ssize_t sz, i, res_offset;
    Py_UCS4 maxchar = 0;
    Py_UCS4 item_maxchar;

    fseq = PySequence_Fast(seq, "");
    if (fseq == NULL) {
        return NULL;
    }

    /* NOTE: the following code can't call back into Python code,
     * so we are sure that fseq won't be mutated.
     */

    seqlen = PySequence_Fast_GET_SIZE(fseq);
    /* If empty sequence, return u"". */
    if (seqlen == 0) {
        res = PyUnicode_New(0, 0);
        goto Done;
    }
    items = PySequence_Fast_ITEMS(fseq);
    /* If singleton sequence with an exact Unicode, return that. */
    if (seqlen == 1) {
        item = items[0];
        if (PyUnicode_CheckExact(item)) {
            Py_INCREF(item);
            res = item;
            goto Done;
        }
    }
    else {
        /* Set up sep and seplen */
        if (separator == NULL) {
            /* fall back to a blank space separator */
            sep = PyUnicode_FromOrdinal(' ');
            if (!sep)
                goto onError;
        }
        else {
            if (!PyUnicode_Check(separator)) {
                PyErr_Format(PyExc_TypeError,
                             "separator: expected str instance,"
                             " %.80s found",
                             Py_TYPE(separator)->tp_name);
                goto onError;
            }
            if (PyUnicode_READY(separator))
                goto onError;
            sep = separator;
            seplen = PyUnicode_GET_LENGTH(separator);
            maxchar = PyUnicode_MAX_CHAR_VALUE(separator);
            /* inc refcount to keep this code path symetric with the
               above case of a blank separator */
            Py_INCREF(sep);
        }
    }

    /* There are at least two things to join, or else we have a subclass
     * of str in the sequence.
     * Do a pre-pass to figure out the total amount of space we'll
     * need (sz), and see whether all argument are strings.
     */
    sz = 0;
    for (i = 0; i < seqlen; i++) {
        const Py_ssize_t old_sz = sz;
        item = items[i];
        if (!PyUnicode_Check(item)) {
            PyErr_Format(PyExc_TypeError,
                         "sequence item %zd: expected str instance,"
                         " %.80s found",
                         i, Py_TYPE(item)->tp_name);
            goto onError;
        }
        if (PyUnicode_READY(item) == -1)
            goto onError;
        sz += PyUnicode_GET_LENGTH(item);
        item_maxchar = PyUnicode_MAX_CHAR_VALUE(item);
        if (item_maxchar > maxchar)
            maxchar = item_maxchar;
        if (i != 0)
            sz += seplen;
        if (sz < old_sz || sz > PY_SSIZE_T_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "join() result is too long for a Python string");
            goto onError;
        }
    }

    res = PyUnicode_New(sz, maxchar);
    if (res == NULL)
        goto onError;

    /* Catenate everything. */
    for (i = 0, res_offset = 0; i < seqlen; ++i) {
        Py_ssize_t itemlen, copied;
        item = items[i];
        /* Copy item, and maybe the separator. */
        if (i && seplen != 0) {
            copied = PyUnicode_CopyCharacters(res, res_offset,
                                              sep, 0, seplen);
            if (copied < 0)
                goto onError;
#ifdef Py_DEBUG
            res_offset += copied;
#else
            res_offset += seplen;
#endif
        }
        itemlen = PyUnicode_GET_LENGTH(item);
        if (itemlen != 0) {
            copied = PyUnicode_CopyCharacters(res, res_offset,
                                              item, 0, itemlen);
            if (copied < 0)
                goto onError;
#ifdef Py_DEBUG
            res_offset += copied;
#else
            res_offset += itemlen;
#endif
        }
    }
    assert(res_offset == PyUnicode_GET_LENGTH(res));

  Done:
    Py_DECREF(fseq);
    Py_XDECREF(sep);
    return res;

  onError:
    Py_DECREF(fseq);
    Py_XDECREF(sep);
    Py_XDECREF(res);
    return NULL;
}

#define FILL(kind, data, value, start, length) \
    do { \
        Py_ssize_t i_ = 0; \
        assert(kind != PyUnicode_WCHAR_KIND); \
        switch ((kind)) { \
        case PyUnicode_1BYTE_KIND: { \
            unsigned char * to_ = (unsigned char *)((data)) + (start); \
            memset(to_, (unsigned char)value, length); \
            break; \
        } \
        case PyUnicode_2BYTE_KIND: { \
            Py_UCS2 * to_ = (Py_UCS2 *)((data)) + (start); \
            for (; i_ < (length); ++i_, ++to_) *to_ = (value); \
            break; \
        } \
        default: { \
            Py_UCS4 * to_ = (Py_UCS4 *)((data)) + (start); \
            for (; i_ < (length); ++i_, ++to_) *to_ = (value); \
            break; \
        } \
        } \
    } while (0)

static PyUnicodeObject *
pad(PyUnicodeObject *self,
    Py_ssize_t left,
    Py_ssize_t right,
    Py_UCS4 fill)
{
    PyObject *u;
    Py_UCS4 maxchar;
    int kind;
    void *data;

    if (left < 0)
        left = 0;
    if (right < 0)
        right = 0;

    if (left == 0 && right == 0 && PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return self;
    }

    if (left > PY_SSIZE_T_MAX - _PyUnicode_LENGTH(self) ||
        right > PY_SSIZE_T_MAX - (left + _PyUnicode_LENGTH(self))) {
        PyErr_SetString(PyExc_OverflowError, "padded string is too long");
        return NULL;
    }
    maxchar = PyUnicode_MAX_CHAR_VALUE(self);
    if (fill > maxchar)
        maxchar = fill;
    u = PyUnicode_New(left + _PyUnicode_LENGTH(self) + right, maxchar);
    if (!u)
        return NULL;

    kind = PyUnicode_KIND(u);
    data = PyUnicode_DATA(u);
    if (left)
        FILL(kind, data, fill, 0, left);
    if (right)
        FILL(kind, data, fill, left + _PyUnicode_LENGTH(self), right);
    if (PyUnicode_CopyCharacters(u, left,
                                 (PyObject*)self, 0,
                                 _PyUnicode_LENGTH(self)) < 0)
    {
        Py_DECREF(u);
        return NULL;
    }

    return (PyUnicodeObject*)u;
}
#undef FILL

PyObject *
PyUnicode_Splitlines(PyObject *string, int keepends)
{
    PyObject *list;

    string = PyUnicode_FromObject(string);
    if (string == NULL || PyUnicode_READY(string) == -1)
        return NULL;

    switch(PyUnicode_KIND(string)) {
    case PyUnicode_1BYTE_KIND:
        list = ucs1lib_splitlines(
            (PyObject*) string, PyUnicode_1BYTE_DATA(string),
            PyUnicode_GET_LENGTH(string), keepends);
        break;
    case PyUnicode_2BYTE_KIND:
        list = ucs2lib_splitlines(
            (PyObject*) string, PyUnicode_2BYTE_DATA(string),
            PyUnicode_GET_LENGTH(string), keepends);
        break;
    case PyUnicode_4BYTE_KIND:
        list = ucs4lib_splitlines(
            (PyObject*) string, PyUnicode_4BYTE_DATA(string),
            PyUnicode_GET_LENGTH(string), keepends);
        break;
    default:
        assert(0);
        list = 0;
    }
    Py_DECREF(string);
    return list;
}

static PyObject *
split(PyUnicodeObject *self,
      PyUnicodeObject *substring,
      Py_ssize_t maxcount)
{
    int kind1, kind2, kind;
    void *buf1, *buf2;
    Py_ssize_t len1, len2;
    PyObject* out;

    if (maxcount < 0)
        maxcount = PY_SSIZE_T_MAX;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    if (substring == NULL)
        switch(PyUnicode_KIND(self)) {
        case PyUnicode_1BYTE_KIND:
            return ucs1lib_split_whitespace(
                (PyObject*) self,  PyUnicode_1BYTE_DATA(self),
                PyUnicode_GET_LENGTH(self), maxcount
                );
        case PyUnicode_2BYTE_KIND:
            return ucs2lib_split_whitespace(
                (PyObject*) self,  PyUnicode_2BYTE_DATA(self),
                PyUnicode_GET_LENGTH(self), maxcount
                );
        case PyUnicode_4BYTE_KIND:
            return ucs4lib_split_whitespace(
                (PyObject*) self,  PyUnicode_4BYTE_DATA(self),
                PyUnicode_GET_LENGTH(self), maxcount
                );
        default:
            assert(0);
            return NULL;
        }

    if (PyUnicode_READY(substring) == -1)
        return NULL;

    kind1 = PyUnicode_KIND(self);
    kind2 = PyUnicode_KIND(substring);
    kind = kind1 > kind2 ? kind1 : kind2;
    buf1 = PyUnicode_DATA(self);
    buf2 = PyUnicode_DATA(substring);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind((PyObject*)self, kind);
    if (!buf1)
        return NULL;
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind((PyObject*)substring, kind);
    if (!buf2) {
        if (kind1 != kind) PyMem_Free(buf1);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(self);
    len2 = PyUnicode_GET_LENGTH(substring);

    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        out = ucs1lib_split(
            (PyObject*) self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_split(
            (PyObject*) self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_split(
            (PyObject*) self,  buf1, len1, buf2, len2, maxcount);
        break;
    default:
        out = NULL;
    }
    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);
    return out;
}

static PyObject *
rsplit(PyUnicodeObject *self,
       PyUnicodeObject *substring,
       Py_ssize_t maxcount)
{
    int kind1, kind2, kind;
    void *buf1, *buf2;
    Py_ssize_t len1, len2;
    PyObject* out;

    if (maxcount < 0)
        maxcount = PY_SSIZE_T_MAX;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    if (substring == NULL)
        switch(PyUnicode_KIND(self)) {
        case PyUnicode_1BYTE_KIND:
            return ucs1lib_rsplit_whitespace(
                (PyObject*) self,  PyUnicode_1BYTE_DATA(self),
                PyUnicode_GET_LENGTH(self), maxcount
                );
        case PyUnicode_2BYTE_KIND:
            return ucs2lib_rsplit_whitespace(
                (PyObject*) self,  PyUnicode_2BYTE_DATA(self),
                PyUnicode_GET_LENGTH(self), maxcount
                );
        case PyUnicode_4BYTE_KIND:
            return ucs4lib_rsplit_whitespace(
                (PyObject*) self,  PyUnicode_4BYTE_DATA(self),
                PyUnicode_GET_LENGTH(self), maxcount
                );
        default:
            assert(0);
            return NULL;
        }

    if (PyUnicode_READY(substring) == -1)
        return NULL;

    kind1 = PyUnicode_KIND(self);
    kind2 = PyUnicode_KIND(substring);
    kind = kind1 > kind2 ? kind1 : kind2;
    buf1 = PyUnicode_DATA(self);
    buf2 = PyUnicode_DATA(substring);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind((PyObject*)self, kind);
    if (!buf1)
        return NULL;
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind((PyObject*)substring, kind);
    if (!buf2) {
        if (kind1 != kind) PyMem_Free(buf1);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(self);
    len2 = PyUnicode_GET_LENGTH(substring);

    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        out = ucs1lib_rsplit(
            (PyObject*) self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_rsplit(
            (PyObject*) self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_rsplit(
            (PyObject*) self,  buf1, len1, buf2, len2, maxcount);
        break;
    default:
        out = NULL;
    }
    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);
    return out;
}

static Py_ssize_t
anylib_find(int kind, void *buf1, Py_ssize_t len1,
            void *buf2, Py_ssize_t len2, Py_ssize_t offset)
{
    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1lib_find(buf1, len1, buf2, len2, offset);
    case PyUnicode_2BYTE_KIND:
        return ucs2lib_find(buf1, len1, buf2, len2, offset);
    case PyUnicode_4BYTE_KIND:
        return ucs4lib_find(buf1, len1, buf2, len2, offset);
    }
    assert(0);
    return -1;
}

static Py_ssize_t
anylib_count(int kind, void* sbuf, Py_ssize_t slen,
             void *buf1, Py_ssize_t len1, Py_ssize_t maxcount)
{
        switch(kind) {
        case PyUnicode_1BYTE_KIND:
            return ucs1lib_count(sbuf, slen, buf1, len1, maxcount);
        case PyUnicode_2BYTE_KIND:
            return ucs2lib_count(sbuf, slen, buf1, len1, maxcount);
        case PyUnicode_4BYTE_KIND:
            return ucs4lib_count(sbuf, slen, buf1, len1, maxcount);
        }
        assert(0);
        return 0;
}

static PyObject *
replace(PyObject *self, PyObject *str1,
        PyObject *str2, Py_ssize_t maxcount)
{
    PyObject *u;
    char *sbuf = PyUnicode_DATA(self);
    char *buf1 = PyUnicode_DATA(str1);
    char *buf2 = PyUnicode_DATA(str2);
    int srelease = 0, release1 = 0, release2 = 0;
    int skind = PyUnicode_KIND(self);
    int kind1 = PyUnicode_KIND(str1);
    int kind2 = PyUnicode_KIND(str2);
    Py_ssize_t slen = PyUnicode_GET_LENGTH(self);
    Py_ssize_t len1 = PyUnicode_GET_LENGTH(str1);
    Py_ssize_t len2 = PyUnicode_GET_LENGTH(str2);

    if (maxcount < 0)
        maxcount = PY_SSIZE_T_MAX;
    else if (maxcount == 0 || slen == 0)
        goto nothing;

    if (skind < kind1)
        /* substring too wide to be present */
        goto nothing;

    if (len1 == len2) {
        Py_ssize_t i;
        /* same length */
        if (len1 == 0)
            goto nothing;
        if (len1 == 1) {
            /* replace characters */
            Py_UCS4 u1, u2, maxchar;
            int mayshrink, rkind;
            u1 = PyUnicode_READ_CHAR(str1, 0);
            if (!findchar(sbuf, PyUnicode_KIND(self),
                          slen, u1, 1))
                goto nothing;
            u2 = PyUnicode_READ_CHAR(str2, 0);
            maxchar = PyUnicode_MAX_CHAR_VALUE(self);
            /* Replacing u1 with u2 may cause a maxchar reduction in the
               result string. */
            mayshrink = maxchar > 127;
            if (u2 > maxchar) {
                maxchar = u2;
                mayshrink = 0;
            }
            u = PyUnicode_New(slen, maxchar);
            if (!u)
                goto error;
            if (PyUnicode_CopyCharacters(u, 0,
                                         (PyObject*)self, 0, slen) < 0)
            {
                Py_DECREF(u);
                return NULL;
            }
            rkind = PyUnicode_KIND(u);
            for (i = 0; i < PyUnicode_GET_LENGTH(u); i++)
                if (PyUnicode_READ(rkind, PyUnicode_DATA(u), i) == u1) {
                    if (--maxcount < 0)
                        break;
                    PyUnicode_WRITE(rkind, PyUnicode_DATA(u), i, u2);
                }
            if (mayshrink) {
                PyObject *tmp = u;
                u = PyUnicode_FromKindAndData(rkind, PyUnicode_DATA(tmp),
                                              PyUnicode_GET_LENGTH(tmp));
                Py_DECREF(tmp);
            }
        } else {
            int rkind = skind;
            char *res;
            if (kind1 < rkind) {
                /* widen substring */
                buf1 = _PyUnicode_AsKind(str1, rkind);
                if (!buf1) goto error;
                release1 = 1;
            }
            i = anylib_find(rkind, sbuf, slen, buf1, len1, 0);
            if (i < 0)
                goto nothing;
            if (rkind > kind2) {
                /* widen replacement */
                buf2 = _PyUnicode_AsKind(str2, rkind);
                if (!buf2) goto error;
                release2 = 1;
            }
            else if (rkind < kind2) {
                /* widen self and buf1 */
                rkind = kind2;
                if (release1) PyMem_Free(buf1);
                sbuf = _PyUnicode_AsKind(self, rkind);
                if (!sbuf) goto error;
                srelease = 1;
                buf1 = _PyUnicode_AsKind(str1, rkind);
                if (!buf1) goto error;
                release1 = 1;
            }
            res = PyMem_Malloc(PyUnicode_KIND_SIZE(rkind, slen));
            if (!res) {
                PyErr_NoMemory();
                goto error;
            }
            memcpy(res, sbuf, PyUnicode_KIND_SIZE(rkind, slen));
            /* change everything in-place, starting with this one */
            memcpy(res + PyUnicode_KIND_SIZE(rkind, i),
                   buf2,
                   PyUnicode_KIND_SIZE(rkind, len2));
            i += len1;

            while ( --maxcount > 0) {
                i = anylib_find(rkind, sbuf+PyUnicode_KIND_SIZE(rkind, i),
                                slen-i,
                                buf1, len1, i);
                if (i == -1)
                    break;
                memcpy(res + PyUnicode_KIND_SIZE(rkind, i),
                       buf2,
                       PyUnicode_KIND_SIZE(rkind, len2));
                i += len1;
            }

            u = PyUnicode_FromKindAndData(rkind, res, slen);
            PyMem_Free(res);
            if (!u) goto error;
        }
    } else {

        Py_ssize_t n, i, j, ires;
        Py_ssize_t product, new_size;
        int rkind = skind;
        char *res;

        if (kind1 < rkind) {
            buf1 = _PyUnicode_AsKind(str1, rkind);
            if (!buf1) goto error;
            release1 = 1;
        }
        n = anylib_count(rkind, sbuf, slen, buf1, len1, maxcount);
        if (n == 0)
            goto nothing;
        if (kind2 < rkind) {
            buf2 = _PyUnicode_AsKind(str2, rkind);
            if (!buf2) goto error;
            release2 = 1;
        }
        else if (kind2 > rkind) {
            rkind = kind2;
            sbuf = _PyUnicode_AsKind(self, rkind);
            if (!sbuf) goto error;
            srelease = 1;
            if (release1) PyMem_Free(buf1);
            buf1 = _PyUnicode_AsKind(str1, rkind);
            if (!buf1) goto error;
            release1 = 1;
        }
        /* new_size = PyUnicode_GET_LENGTH(self) + n * (PyUnicode_GET_LENGTH(str2) -
           PyUnicode_GET_LENGTH(str1))); */
        product = n * (len2-len1);
        if ((product / (len2-len1)) != n) {
                PyErr_SetString(PyExc_OverflowError,
                                "replace string is too long");
                goto error;
        }
        new_size = slen + product;
        if (new_size < 0 || new_size > (PY_SSIZE_T_MAX >> (rkind-1))) {
            PyErr_SetString(PyExc_OverflowError,
                            "replace string is too long");
            goto error;
        }
        res = PyMem_Malloc(PyUnicode_KIND_SIZE(rkind, new_size));
        if (!res)
            goto error;
        ires = i = 0;
        if (len1 > 0) {
            while (n-- > 0) {
                /* look for next match */
                j = anylib_find(rkind,
                                sbuf + PyUnicode_KIND_SIZE(rkind, i),
                                slen-i, buf1, len1, i);
                if (j == -1)
                    break;
                else if (j > i) {
                    /* copy unchanged part [i:j] */
                    memcpy(res + PyUnicode_KIND_SIZE(rkind, ires),
                           sbuf + PyUnicode_KIND_SIZE(rkind, i),
                           PyUnicode_KIND_SIZE(rkind, j-i));
                    ires += j - i;
                }
                /* copy substitution string */
                if (len2 > 0) {
                    memcpy(res + PyUnicode_KIND_SIZE(rkind, ires),
                           buf2,
                           PyUnicode_KIND_SIZE(rkind, len2));
                    ires += len2;
                }
                i = j + len1;
            }
            if (i < slen)
                /* copy tail [i:] */
                memcpy(res + PyUnicode_KIND_SIZE(rkind, ires),
                       sbuf + PyUnicode_KIND_SIZE(rkind, i),
                       PyUnicode_KIND_SIZE(rkind, slen-i));
        } else {
            /* interleave */
            while (n > 0) {
                memcpy(res + PyUnicode_KIND_SIZE(rkind, ires),
                       buf2,
                       PyUnicode_KIND_SIZE(rkind, len2));
                ires += len2;
                if (--n <= 0)
                    break;
                memcpy(res + PyUnicode_KIND_SIZE(rkind, ires),
                       sbuf + PyUnicode_KIND_SIZE(rkind, i),
                       PyUnicode_KIND_SIZE(rkind, 1));
                ires++;
                i++;
            }
            memcpy(res + PyUnicode_KIND_SIZE(rkind, ires),
                   sbuf + PyUnicode_KIND_SIZE(rkind, i),
                   PyUnicode_KIND_SIZE(rkind, slen-i));
        }
        u = PyUnicode_FromKindAndData(rkind, res, new_size);
        PyMem_Free(res);
    }
    if (srelease)
        PyMem_FREE(sbuf);
    if (release1)
        PyMem_FREE(buf1);
    if (release2)
        PyMem_FREE(buf2);
    return u;

  nothing:
    /* nothing to replace; return original string (when possible) */
    if (srelease)
        PyMem_FREE(sbuf);
    if (release1)
        PyMem_FREE(buf1);
    if (release2)
        PyMem_FREE(buf2);
    if (PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject *) self;
    }
    return PyUnicode_Copy(self);
  error:
    if (srelease && sbuf)
        PyMem_FREE(sbuf);
    if (release1 && buf1)
        PyMem_FREE(buf1);
    if (release2 && buf2)
        PyMem_FREE(buf2);
    return NULL;
}

/* --- Unicode Object Methods --------------------------------------------- */

PyDoc_STRVAR(title__doc__,
             "S.title() -> str\n\
\n\
Return a titlecased version of S, i.e. words start with title case\n\
characters, all remaining cased characters have lower case.");

static PyObject*
unicode_title(PyUnicodeObject *self)
{
    return fixup(self, fixtitle);
}

PyDoc_STRVAR(capitalize__doc__,
             "S.capitalize() -> str\n\
\n\
Return a capitalized version of S, i.e. make the first character\n\
have upper case and the rest lower case.");

static PyObject*
unicode_capitalize(PyUnicodeObject *self)
{
    return fixup(self, fixcapitalize);
}

#if 0
PyDoc_STRVAR(capwords__doc__,
             "S.capwords() -> str\n\
\n\
Apply .capitalize() to all words in S and return the result with\n\
normalized whitespace (all whitespace strings are replaced by ' ').");

static PyObject*
unicode_capwords(PyUnicodeObject *self)
{
    PyObject *list;
    PyObject *item;
    Py_ssize_t i;

    /* Split into words */
    list = split(self, NULL, -1);
    if (!list)
        return NULL;

    /* Capitalize each word */
    for (i = 0; i < PyList_GET_SIZE(list); i++) {
        item = fixup((PyUnicodeObject *)PyList_GET_ITEM(list, i),
                     fixcapitalize);
        if (item == NULL)
            goto onError;
        Py_DECREF(PyList_GET_ITEM(list, i));
        PyList_SET_ITEM(list, i, item);
    }

    /* Join the words to form a new string */
    item = PyUnicode_Join(NULL, list);

  onError:
    Py_DECREF(list);
    return (PyObject *)item;
}
#endif

/* Argument converter.  Coerces to a single unicode character */

static int
convert_uc(PyObject *obj, void *addr)
{
    Py_UCS4 *fillcharloc = (Py_UCS4 *)addr;
    PyObject *uniobj;

    uniobj = PyUnicode_FromObject(obj);
    if (uniobj == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "The fill character cannot be converted to Unicode");
        return 0;
    }
    if (PyUnicode_GET_LENGTH(uniobj) != 1) {
        PyErr_SetString(PyExc_TypeError,
                        "The fill character must be exactly one character long");
        Py_DECREF(uniobj);
        return 0;
    }
    *fillcharloc = PyUnicode_READ_CHAR(uniobj, 0);
    Py_DECREF(uniobj);
    return 1;
}

PyDoc_STRVAR(center__doc__,
             "S.center(width[, fillchar]) -> str\n\
\n\
Return S centered in a string of length width. Padding is\n\
done using the specified fill character (default is a space)");

static PyObject *
unicode_center(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t marg, left;
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|O&:center", &width, convert_uc, &fillchar))
        return NULL;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    if (_PyUnicode_LENGTH(self) >= width && PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    marg = width - _PyUnicode_LENGTH(self);
    left = marg / 2 + (marg & width & 1);

    return (PyObject*) pad(self, left, marg - left, fillchar);
}

#if 0

/* This code should go into some future Unicode collation support
   module. The basic comparison should compare ordinals on a naive
   basis (this is what Java does and thus Jython too). */

/* speedy UTF-16 code point order comparison */
/* gleaned from: */
/* http://www-4.ibm.com/software/developer/library/utf16.html?dwzone=unicode */

static short utf16Fixup[32] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0x2000, -0x800, -0x800, -0x800, -0x800
};

static int
unicode_compare(PyUnicodeObject *str1, PyUnicodeObject *str2)
{
    Py_ssize_t len1, len2;

    Py_UNICODE *s1 = str1->str;
    Py_UNICODE *s2 = str2->str;

    len1 = str1->_base._base.length;
    len2 = str2->_base._base.length;

    while (len1 > 0 && len2 > 0) {
        Py_UNICODE c1, c2;

        c1 = *s1++;
        c2 = *s2++;

        if (c1 > (1<<11) * 26)
            c1 += utf16Fixup[c1>>11];
        if (c2 > (1<<11) * 26)
            c2 += utf16Fixup[c2>>11];
        /* now c1 and c2 are in UTF-32-compatible order */

        if (c1 != c2)
            return (c1 < c2) ? -1 : 1;

        len1--; len2--;
    }

    return (len1 < len2) ? -1 : (len1 != len2);
}

#else

/* This function assumes that str1 and str2 are readied by the caller. */

static int
unicode_compare(PyUnicodeObject *str1, PyUnicodeObject *str2)
{
    int kind1, kind2;
    void *data1, *data2;
    Py_ssize_t len1, len2, i;

    kind1 = PyUnicode_KIND(str1);
    kind2 = PyUnicode_KIND(str2);
    data1 = PyUnicode_DATA(str1);
    data2 = PyUnicode_DATA(str2);
    len1 = PyUnicode_GET_LENGTH(str1);
    len2 = PyUnicode_GET_LENGTH(str2);

    for (i = 0; i < len1 && i < len2; ++i) {
        Py_UCS4 c1, c2;
        c1 = PyUnicode_READ(kind1, data1, i);
        c2 = PyUnicode_READ(kind2, data2, i);

        if (c1 != c2)
            return (c1 < c2) ? -1 : 1;
    }

    return (len1 < len2) ? -1 : (len1 != len2);
}

#endif

int
PyUnicode_Compare(PyObject *left, PyObject *right)
{
    if (PyUnicode_Check(left) && PyUnicode_Check(right)) {
        if (PyUnicode_READY(left) == -1 ||
            PyUnicode_READY(right) == -1)
            return -1;
        return unicode_compare((PyUnicodeObject *)left,
                               (PyUnicodeObject *)right);
    }
    PyErr_Format(PyExc_TypeError,
                 "Can't compare %.100s and %.100s",
                 left->ob_type->tp_name,
                 right->ob_type->tp_name);
    return -1;
}

int
PyUnicode_CompareWithASCIIString(PyObject* uni, const char* str)
{
    Py_ssize_t i;
    int kind;
    void *data;
    Py_UCS4 chr;

    assert(_PyUnicode_CHECK(uni));
    if (PyUnicode_READY(uni) == -1)
        return -1;
    kind = PyUnicode_KIND(uni);
    data = PyUnicode_DATA(uni);
    /* Compare Unicode string and source character set string */
    for (i = 0; (chr = PyUnicode_READ(kind, data, i)) && str[i]; i++)
        if (chr != str[i])
            return (chr < (unsigned char)(str[i])) ? -1 : 1;
    /* This check keeps Python strings that end in '\0' from comparing equal
     to C strings identical up to that point. */
    if (PyUnicode_GET_LENGTH(uni) != i || chr)
        return 1; /* uni is longer */
    if (str[i])
        return -1; /* str is longer */
    return 0;
}


#define TEST_COND(cond)                         \
    ((cond) ? Py_True : Py_False)

PyObject *
PyUnicode_RichCompare(PyObject *left, PyObject *right, int op)
{
    int result;

    if (PyUnicode_Check(left) && PyUnicode_Check(right)) {
        PyObject *v;
        if (PyUnicode_READY(left) == -1 ||
            PyUnicode_READY(right) == -1)
            return NULL;
        if (PyUnicode_GET_LENGTH(left) != PyUnicode_GET_LENGTH(right) ||
            PyUnicode_KIND(left) != PyUnicode_KIND(right)) {
            if (op == Py_EQ) {
                Py_INCREF(Py_False);
                return Py_False;
            }
            if (op == Py_NE) {
                Py_INCREF(Py_True);
                return Py_True;
            }
        }
        if (left == right)
            result = 0;
        else
            result = unicode_compare((PyUnicodeObject *)left,
                                     (PyUnicodeObject *)right);

        /* Convert the return value to a Boolean */
        switch (op) {
        case Py_EQ:
            v = TEST_COND(result == 0);
            break;
        case Py_NE:
            v = TEST_COND(result != 0);
            break;
        case Py_LE:
            v = TEST_COND(result <= 0);
            break;
        case Py_GE:
            v = TEST_COND(result >= 0);
            break;
        case Py_LT:
            v = TEST_COND(result == -1);
            break;
        case Py_GT:
            v = TEST_COND(result == 1);
            break;
        default:
            PyErr_BadArgument();
            return NULL;
        }
        Py_INCREF(v);
        return v;
    }

    Py_RETURN_NOTIMPLEMENTED;
}

int
PyUnicode_Contains(PyObject *container, PyObject *element)
{
    PyObject *str, *sub;
    int kind1, kind2, kind;
    void *buf1, *buf2;
    Py_ssize_t len1, len2;
    int result;

    /* Coerce the two arguments */
    sub = PyUnicode_FromObject(element);
    if (!sub) {
        PyErr_Format(PyExc_TypeError,
                     "'in <string>' requires string as left operand, not %s",
                     element->ob_type->tp_name);
        return -1;
    }
    if (PyUnicode_READY(sub) == -1)
        return -1;

    str = PyUnicode_FromObject(container);
    if (!str || PyUnicode_READY(str) == -1) {
        Py_DECREF(sub);
        return -1;
    }

    kind1 = PyUnicode_KIND(str);
    kind2 = PyUnicode_KIND(sub);
    kind = kind1 > kind2 ? kind1 : kind2;
    buf1 = PyUnicode_DATA(str);
    buf2 = PyUnicode_DATA(sub);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind((PyObject*)str, kind);
    if (!buf1) {
        Py_DECREF(sub);
        return -1;
    }
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind((PyObject*)sub, kind);
    if (!buf2) {
        Py_DECREF(sub);
        if (kind1 != kind) PyMem_Free(buf1);
        return -1;
    }
    len1 = PyUnicode_GET_LENGTH(str);
    len2 = PyUnicode_GET_LENGTH(sub);

    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        result = ucs1lib_find(buf1, len1, buf2, len2, 0) != -1;
        break;
    case PyUnicode_2BYTE_KIND:
        result = ucs2lib_find(buf1, len1, buf2, len2, 0) != -1;
        break;
    case PyUnicode_4BYTE_KIND:
        result = ucs4lib_find(buf1, len1, buf2, len2, 0) != -1;
        break;
    default:
        result = -1;
        assert(0);
    }

    Py_DECREF(str);
    Py_DECREF(sub);

    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);

    return result;
}

/* Concat to string or Unicode object giving a new Unicode object. */

PyObject *
PyUnicode_Concat(PyObject *left, PyObject *right)
{
    PyObject *u = NULL, *v = NULL, *w;
    Py_UCS4 maxchar;

    /* Coerce the two arguments */
    u = PyUnicode_FromObject(left);
    if (u == NULL)
        goto onError;
    v = PyUnicode_FromObject(right);
    if (v == NULL)
        goto onError;

    /* Shortcuts */
    if (v == unicode_empty) {
        Py_DECREF(v);
        return u;
    }
    if (u == unicode_empty) {
        Py_DECREF(u);
        return v;
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(u);
    maxchar = Py_MAX(maxchar, PyUnicode_MAX_CHAR_VALUE(v));

    /* Concat the two Unicode strings */
    w = PyUnicode_New(
        PyUnicode_GET_LENGTH(u) + PyUnicode_GET_LENGTH(v),
        maxchar);
    if (w == NULL)
        goto onError;
    if (PyUnicode_CopyCharacters(w, 0, u, 0, PyUnicode_GET_LENGTH(u)) < 0)
        goto onError;
    if (PyUnicode_CopyCharacters(w, PyUnicode_GET_LENGTH(u),
                                 v, 0,
                                 PyUnicode_GET_LENGTH(v)) < 0)
        goto onError;
    Py_DECREF(u);
    Py_DECREF(v);
    return w;

  onError:
    Py_XDECREF(u);
    Py_XDECREF(v);
    return NULL;
}

static void
unicode_append_inplace(PyObject **p_left, PyObject *right)
{
    Py_ssize_t left_len, right_len, new_len;
#ifdef Py_DEBUG
    Py_ssize_t copied;
#endif

    assert(PyUnicode_IS_READY(*p_left));
    assert(PyUnicode_IS_READY(right));

    left_len = PyUnicode_GET_LENGTH(*p_left);
    right_len = PyUnicode_GET_LENGTH(right);
    if (left_len > PY_SSIZE_T_MAX - right_len) {
        PyErr_SetString(PyExc_OverflowError,
                        "strings are too large to concat");
        goto error;
    }
    new_len = left_len + right_len;

    /* Now we own the last reference to 'left', so we can resize it
     * in-place.
     */
    if (unicode_resize(p_left, new_len) != 0) {
        /* XXX if _PyUnicode_Resize() fails, 'left' has been
         * deallocated so it cannot be put back into
         * 'variable'.  The MemoryError is raised when there
         * is no value in 'variable', which might (very
         * remotely) be a cause of incompatibilities.
         */
        goto error;
    }
    /* copy 'right' into the newly allocated area of 'left' */
#ifdef Py_DEBUG
    copied = PyUnicode_CopyCharacters(*p_left, left_len,
                                      right, 0,
                                      right_len);
    assert(0 <= copied);
#else
    PyUnicode_CopyCharacters(*p_left, left_len, right, 0, right_len);
#endif
    return;

error:
    Py_DECREF(*p_left);
    *p_left = NULL;
}

void
PyUnicode_Append(PyObject **p_left, PyObject *right)
{
    PyObject *left, *res;

    if (p_left == NULL) {
        if (!PyErr_Occurred())
            PyErr_BadInternalCall();
        return;
    }
    left = *p_left;
    if (right == NULL || !PyUnicode_Check(left)) {
        if (!PyErr_Occurred())
            PyErr_BadInternalCall();
        goto error;
    }

    if (PyUnicode_CheckExact(left) && left != unicode_empty
        && PyUnicode_CheckExact(right) && right != unicode_empty
        && unicode_resizable(left)
        && (_PyUnicode_KIND(right) <= _PyUnicode_KIND(left)
            || _PyUnicode_WSTR(left) != NULL))
    {
        if (PyUnicode_READY(left))
            goto error;
        if (PyUnicode_READY(right))
            goto error;

        /* Don't resize for ascii += latin1. Convert ascii to latin1 requires
           to change the structure size, but characters are stored just after
           the structure, and so it requires to move all charactres which is
           not so different than duplicating the string. */
        if (!(PyUnicode_IS_ASCII(left) && !PyUnicode_IS_ASCII(right)))
        {
            unicode_append_inplace(p_left, right);
            return;
        }
    }

    res = PyUnicode_Concat(left, right);
    if (res == NULL)
        goto error;
    Py_DECREF(left);
    *p_left = res;
    return;

error:
    Py_DECREF(*p_left);
    *p_left = NULL;
}

void
PyUnicode_AppendAndDel(PyObject **pleft, PyObject *right)
{
    PyUnicode_Append(pleft, right);
    Py_XDECREF(right);
}

PyDoc_STRVAR(count__doc__,
             "S.count(sub[, start[, end]]) -> int\n\
\n\
Return the number of non-overlapping occurrences of substring sub in\n\
string S[start:end].  Optional arguments start and end are\n\
interpreted as in slice notation.");

static PyObject *
unicode_count(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *result;
    int kind1, kind2, kind;
    void *buf1, *buf2;
    Py_ssize_t len1, len2, iresult;

    if (!stringlib_parse_args_finds_unicode("count", args, &substring,
                                            &start, &end))
        return NULL;

    kind1 = PyUnicode_KIND(self);
    kind2 = PyUnicode_KIND(substring);
    kind = kind1 > kind2 ? kind1 : kind2;
    buf1 = PyUnicode_DATA(self);
    buf2 = PyUnicode_DATA(substring);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind((PyObject*)self, kind);
    if (!buf1) {
        Py_DECREF(substring);
        return NULL;
    }
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind((PyObject*)substring, kind);
    if (!buf2) {
        Py_DECREF(substring);
        if (kind1 != kind) PyMem_Free(buf1);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(self);
    len2 = PyUnicode_GET_LENGTH(substring);

    ADJUST_INDICES(start, end, len1);
    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        iresult = ucs1lib_count(
            ((Py_UCS1*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_2BYTE_KIND:
        iresult = ucs2lib_count(
            ((Py_UCS2*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_4BYTE_KIND:
        iresult = ucs4lib_count(
            ((Py_UCS4*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    default:
        assert(0); iresult = 0;
    }

    result = PyLong_FromSsize_t(iresult);

    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);

    Py_DECREF(substring);

    return result;
}

PyDoc_STRVAR(encode__doc__,
             "S.encode(encoding='utf-8', errors='strict') -> bytes\n\
\n\
Encode S using the codec registered for encoding. Default encoding\n\
is 'utf-8'. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a UnicodeEncodeError. Other possible values are 'ignore', 'replace' and\n\
'xmlcharrefreplace' as well as any other name registered with\n\
codecs.register_error that can handle UnicodeEncodeErrors.");

static PyObject *
unicode_encode(PyUnicodeObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"encoding", "errors", 0};
    char *encoding = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ss:encode",
                                     kwlist, &encoding, &errors))
        return NULL;
    return PyUnicode_AsEncodedString((PyObject *)self, encoding, errors);
}

PyDoc_STRVAR(expandtabs__doc__,
             "S.expandtabs([tabsize]) -> str\n\
\n\
Return a copy of S where all tab characters are expanded using spaces.\n\
If tabsize is not given, a tab size of 8 characters is assumed.");

static PyObject*
unicode_expandtabs(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t i, j, line_pos, src_len, incr;
    Py_UCS4 ch;
    PyObject *u;
    void *src_data, *dest_data;
    int tabsize = 8;
    int kind;
    int found;

    if (!PyArg_ParseTuple(args, "|i:expandtabs", &tabsize))
        return NULL;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    /* First pass: determine size of output string */
    src_len = PyUnicode_GET_LENGTH(self);
    i = j = line_pos = 0;
    kind = PyUnicode_KIND(self);
    src_data = PyUnicode_DATA(self);
    found = 0;
    for (; i < src_len; i++) {
        ch = PyUnicode_READ(kind, src_data, i);
        if (ch == '\t') {
            found = 1;
            if (tabsize > 0) {
                incr = tabsize - (line_pos % tabsize); /* cannot overflow */
                if (j > PY_SSIZE_T_MAX - incr)
                    goto overflow;
                line_pos += incr;
                j += incr;
            }
        }
        else {
            if (j > PY_SSIZE_T_MAX - 1)
                goto overflow;
            line_pos++;
            j++;
            if (ch == '\n' || ch == '\r')
                line_pos = 0;
        }
    }
    if (!found && PyUnicode_CheckExact(self)) {
        Py_INCREF((PyObject *) self);
        return (PyObject *) self;
    }

    /* Second pass: create output string and fill it */
    u = PyUnicode_New(j, PyUnicode_MAX_CHAR_VALUE(self));
    if (!u)
        return NULL;
    dest_data = PyUnicode_DATA(u);

    i = j = line_pos = 0;

    for (; i < src_len; i++) {
        ch = PyUnicode_READ(kind, src_data, i);
        if (ch == '\t') {
            if (tabsize > 0) {
                incr = tabsize - (line_pos % tabsize);
                line_pos += incr;
                while (incr--) {
                    PyUnicode_WRITE(kind, dest_data, j, ' ');
                    j++;
                }
            }
        }
        else {
            line_pos++;
            PyUnicode_WRITE(kind, dest_data, j, ch);
            j++;
            if (ch == '\n' || ch == '\r')
                line_pos = 0;
        }
    }
    assert (j == PyUnicode_GET_LENGTH(u));
    if (PyUnicode_READY(u)) {
        Py_DECREF(u);
        return NULL;
    }
    return (PyObject*) u;

  overflow:
    PyErr_SetString(PyExc_OverflowError, "new string is too long");
    return NULL;
}

PyDoc_STRVAR(find__doc__,
             "S.find(sub[, start[, end]]) -> int\n\
\n\
Return the lowest index in S where substring sub is found,\n\
such that sub is contained within S[start:end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
unicode_find(PyObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;
    Py_ssize_t result;

    if (!stringlib_parse_args_finds_unicode("find", args, &substring,
                                            &start, &end))
        return NULL;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    if (PyUnicode_READY(substring) == -1)
        return NULL;

    result = any_find_slice(
        ucs1lib_find_slice, ucs2lib_find_slice, ucs4lib_find_slice,
        self, (PyObject*)substring, start, end
        );

    Py_DECREF(substring);

    if (result == -2)
        return NULL;

    return PyLong_FromSsize_t(result);
}

static PyObject *
unicode_getitem(PyObject *self, Py_ssize_t index)
{
    Py_UCS4 ch = PyUnicode_ReadChar(self, index);
    if (ch == (Py_UCS4)-1)
        return NULL;
    return PyUnicode_FromOrdinal(ch);
}

/* Believe it or not, this produces the same value for ASCII strings
   as bytes_hash(). */
static Py_hash_t
unicode_hash(PyUnicodeObject *self)
{
    Py_ssize_t len;
    Py_uhash_t x;

    if (_PyUnicode_HASH(self) != -1)
        return _PyUnicode_HASH(self);
    if (PyUnicode_READY(self) == -1)
        return -1;
    len = PyUnicode_GET_LENGTH(self);

    /* The hash function as a macro, gets expanded three times below. */
#define HASH(P) \
    x = (Py_uhash_t)*P << 7; \
    while (--len >= 0) \
        x = (1000003*x) ^ (Py_uhash_t)*P++;

    switch (PyUnicode_KIND(self)) {
    case PyUnicode_1BYTE_KIND: {
        const unsigned char *c = PyUnicode_1BYTE_DATA(self);
        HASH(c);
        break;
    }
    case PyUnicode_2BYTE_KIND: {
        const Py_UCS2 *s = PyUnicode_2BYTE_DATA(self);
        HASH(s);
        break;
    }
    default: {
        Py_UCS4 *l;
        assert(PyUnicode_KIND(self) == PyUnicode_4BYTE_KIND &&
               "Impossible switch case in unicode_hash");
        l = PyUnicode_4BYTE_DATA(self);
        HASH(l);
        break;
    }
    }
    x ^= (Py_uhash_t)PyUnicode_GET_LENGTH(self);

    if (x == -1)
        x = -2;
    _PyUnicode_HASH(self) = x;
    return x;
}
#undef HASH

PyDoc_STRVAR(index__doc__,
             "S.index(sub[, start[, end]]) -> int\n\
\n\
Like S.find() but raise ValueError when the substring is not found.");

static PyObject *
unicode_index(PyObject *self, PyObject *args)
{
    Py_ssize_t result;
    PyUnicodeObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;

    if (!stringlib_parse_args_finds_unicode("index", args, &substring,
                                            &start, &end))
        return NULL;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    if (PyUnicode_READY(substring) == -1)
        return NULL;

    result = any_find_slice(
        ucs1lib_find_slice, ucs2lib_find_slice, ucs4lib_find_slice,
        self, (PyObject*)substring, start, end
        );

    Py_DECREF(substring);

    if (result == -2)
        return NULL;

    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }

    return PyLong_FromSsize_t(result);
}

PyDoc_STRVAR(islower__doc__,
             "S.islower() -> bool\n\
\n\
Return True if all cased characters in S are lowercase and there is\n\
at least one cased character in S, False otherwise.");

static PyObject*
unicode_islower(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;
    int cased;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISLOWER(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    cased = 0;
    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch))
            return PyBool_FromLong(0);
        else if (!cased && Py_UNICODE_ISLOWER(ch))
            cased = 1;
    }
    return PyBool_FromLong(cased);
}

PyDoc_STRVAR(isupper__doc__,
             "S.isupper() -> bool\n\
\n\
Return True if all cased characters in S are uppercase and there is\n\
at least one cased character in S, False otherwise.");

static PyObject*
unicode_isupper(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;
    int cased;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISUPPER(PyUnicode_READ(kind, data, 0)) != 0);

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    cased = 0;
    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        if (Py_UNICODE_ISLOWER(ch) || Py_UNICODE_ISTITLE(ch))
            return PyBool_FromLong(0);
        else if (!cased && Py_UNICODE_ISUPPER(ch))
            cased = 1;
    }
    return PyBool_FromLong(cased);
}

PyDoc_STRVAR(istitle__doc__,
             "S.istitle() -> bool\n\
\n\
Return True if S is a titlecased string and there is at least one\n\
character in S, i.e. upper- and titlecase characters may only\n\
follow uncased characters and lowercase characters only cased ones.\n\
Return False otherwise.");

static PyObject*
unicode_istitle(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;
    int cased, previous_is_cased;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, 0);
        return PyBool_FromLong((Py_UNICODE_ISTITLE(ch) != 0) ||
                               (Py_UNICODE_ISUPPER(ch) != 0));
    }

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    cased = 0;
    previous_is_cased = 0;
    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch)) {
            if (previous_is_cased)
                return PyBool_FromLong(0);
            previous_is_cased = 1;
            cased = 1;
        }
        else if (Py_UNICODE_ISLOWER(ch)) {
            if (!previous_is_cased)
                return PyBool_FromLong(0);
            previous_is_cased = 1;
            cased = 1;
        }
        else
            previous_is_cased = 0;
    }
    return PyBool_FromLong(cased);
}

PyDoc_STRVAR(isspace__doc__,
             "S.isspace() -> bool\n\
\n\
Return True if all characters in S are whitespace\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isspace(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISSPACE(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (!Py_UNICODE_ISSPACE(ch))
            return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isalpha__doc__,
             "S.isalpha() -> bool\n\
\n\
Return True if all characters in S are alphabetic\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isalpha(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISALPHA(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISALPHA(PyUnicode_READ(kind, data, i)))
            return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isalnum__doc__,
             "S.isalnum() -> bool\n\
\n\
Return True if all characters in S are alphanumeric\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isalnum(PyUnicodeObject *self)
{
    int kind;
    void *data;
    Py_ssize_t len, i;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    len = PyUnicode_GET_LENGTH(self);

    /* Shortcut for single character strings */
    if (len == 1) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, 0);
        return PyBool_FromLong(Py_UNICODE_ISALNUM(ch));
    }

    /* Special case for empty strings */
    if (len == 0)
        return PyBool_FromLong(0);

    for (i = 0; i < len; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (!Py_UNICODE_ISALNUM(ch))
            return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isdecimal__doc__,
             "S.isdecimal() -> bool\n\
\n\
Return True if there are only decimal characters in S,\n\
False otherwise.");

static PyObject*
unicode_isdecimal(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISDECIMAL(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISDECIMAL(PyUnicode_READ(kind, data, i)))
            return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isdigit__doc__,
             "S.isdigit() -> bool\n\
\n\
Return True if all characters in S are digits\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isdigit(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, 0);
        return PyBool_FromLong(Py_UNICODE_ISDIGIT(ch));
    }

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISDIGIT(PyUnicode_READ(kind, data, i)))
            return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isnumeric__doc__,
             "S.isnumeric() -> bool\n\
\n\
Return True if there are only numeric characters in S,\n\
False otherwise.");

static PyObject*
unicode_isnumeric(PyUnicodeObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISNUMERIC(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        return PyBool_FromLong(0);

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISNUMERIC(PyUnicode_READ(kind, data, i)))
            return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

int
PyUnicode_IsIdentifier(PyObject *self)
{
    int kind;
    void *data;
    Py_ssize_t i;
    Py_UCS4 first;

    if (PyUnicode_READY(self) == -1) {
        Py_FatalError("identifier not ready");
        return 0;
    }

    /* Special case for empty strings */
    if (PyUnicode_GET_LENGTH(self) == 0)
        return 0;
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* PEP 3131 says that the first character must be in
       XID_Start and subsequent characters in XID_Continue,
       and for the ASCII range, the 2.x rules apply (i.e
       start with letters and underscore, continue with
       letters, digits, underscore). However, given the current
       definition of XID_Start and XID_Continue, it is sufficient
       to check just for these, except that _ must be allowed
       as starting an identifier.  */
    first = PyUnicode_READ(kind, data, 0);
    if (!_PyUnicode_IsXidStart(first) && first != 0x5F /* LOW LINE */)
        return 0;

    for (i = 1; i < PyUnicode_GET_LENGTH(self); i++)
        if (!_PyUnicode_IsXidContinue(PyUnicode_READ(kind, data, i)))
            return 0;
    return 1;
}

PyDoc_STRVAR(isidentifier__doc__,
             "S.isidentifier() -> bool\n\
\n\
Return True if S is a valid identifier according\n\
to the language definition.");

static PyObject*
unicode_isidentifier(PyObject *self)
{
    return PyBool_FromLong(PyUnicode_IsIdentifier(self));
}

PyDoc_STRVAR(isprintable__doc__,
             "S.isprintable() -> bool\n\
\n\
Return True if all characters in S are considered\n\
printable in repr() or S is empty, False otherwise.");

static PyObject*
unicode_isprintable(PyObject *self)
{
    Py_ssize_t i, length;
    int kind;
    void *data;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISPRINTABLE(PyUnicode_READ(kind, data, 0)));

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISPRINTABLE(PyUnicode_READ(kind, data, i))) {
            Py_RETURN_FALSE;
        }
    }
    Py_RETURN_TRUE;
}

PyDoc_STRVAR(join__doc__,
             "S.join(iterable) -> str\n\
\n\
Return a string which is the concatenation of the strings in the\n\
iterable.  The separator between elements is S.");

static PyObject*
unicode_join(PyObject *self, PyObject *data)
{
    return PyUnicode_Join(self, data);
}

static Py_ssize_t
unicode_length(PyUnicodeObject *self)
{
    if (PyUnicode_READY(self) == -1)
        return -1;
    return PyUnicode_GET_LENGTH(self);
}

PyDoc_STRVAR(ljust__doc__,
             "S.ljust(width[, fillchar]) -> str\n\
\n\
Return S left-justified in a Unicode string of length width. Padding is\n\
done using the specified fill character (default is a space).");

static PyObject *
unicode_ljust(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (PyUnicode_READY(self) == -1)
        return NULL;

    if (!PyArg_ParseTuple(args, "n|O&:ljust", &width, convert_uc, &fillchar))
        return NULL;

    if (_PyUnicode_LENGTH(self) >= width && PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, 0, width - _PyUnicode_LENGTH(self), fillchar);
}

PyDoc_STRVAR(lower__doc__,
             "S.lower() -> str\n\
\n\
Return a copy of the string S converted to lowercase.");

static PyObject*
unicode_lower(PyUnicodeObject *self)
{
    return fixup(self, fixlower);
}

#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2

/* Arrays indexed by above */
static const char *stripformat[] = {"|O:lstrip", "|O:rstrip", "|O:strip"};

#define STRIPNAME(i) (stripformat[i]+3)

/* externally visible for str.strip(unicode) */
PyObject *
_PyUnicode_XStrip(PyUnicodeObject *self, int striptype, PyObject *sepobj)
{
    void *data;
    int kind;
    Py_ssize_t i, j, len;
    BLOOM_MASK sepmask;

    if (PyUnicode_READY(self) == -1 || PyUnicode_READY(sepobj) == -1)
        return NULL;

    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    len = PyUnicode_GET_LENGTH(self);
    sepmask = make_bloom_mask(PyUnicode_KIND(sepobj),
                              PyUnicode_DATA(sepobj),
                              PyUnicode_GET_LENGTH(sepobj));

    i = 0;
    if (striptype != RIGHTSTRIP) {
        while (i < len &&
               BLOOM_MEMBER(sepmask, PyUnicode_READ(kind, data, i), sepobj)) {
            i++;
        }
    }

    j = len;
    if (striptype != LEFTSTRIP) {
        do {
            j--;
        } while (j >= i &&
                 BLOOM_MEMBER(sepmask, PyUnicode_READ(kind, data, j), sepobj));
        j++;
    }

    return PyUnicode_Substring((PyObject*)self, i, j);
}

PyObject*
PyUnicode_Substring(PyObject *self, Py_ssize_t start, Py_ssize_t end)
{
    unsigned char *data;
    int kind;
    Py_ssize_t length;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    end = Py_MIN(end, PyUnicode_GET_LENGTH(self));

    if (start == 0 && end == PyUnicode_GET_LENGTH(self))
    {
        if (PyUnicode_CheckExact(self)) {
            Py_INCREF(self);
            return self;
        }
        else
            return PyUnicode_Copy(self);
    }

    length = end - start;
    if (length == 1)
        return unicode_getitem(self, start);

    if (start < 0 || end < 0) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return NULL;
    }

    kind = PyUnicode_KIND(self);
    data = PyUnicode_1BYTE_DATA(self);
    return PyUnicode_FromKindAndData(kind,
                                     data + PyUnicode_KIND_SIZE(kind, start),
                                     length);
}

static PyObject *
do_strip(PyUnicodeObject *self, int striptype)
{
    int kind;
    void *data;
    Py_ssize_t len, i, j;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    len = PyUnicode_GET_LENGTH(self);

    i = 0;
    if (striptype != RIGHTSTRIP) {
        while (i < len && Py_UNICODE_ISSPACE(PyUnicode_READ(kind, data, i))) {
            i++;
        }
    }

    j = len;
    if (striptype != LEFTSTRIP) {
        do {
            j--;
        } while (j >= i && Py_UNICODE_ISSPACE(PyUnicode_READ(kind, data, j)));
        j++;
    }

    return PyUnicode_Substring((PyObject*)self, i, j);
}


static PyObject *
do_argstrip(PyUnicodeObject *self, int striptype, PyObject *args)
{
    PyObject *sep = NULL;

    if (!PyArg_ParseTuple(args, (char *)stripformat[striptype], &sep))
        return NULL;

    if (sep != NULL && sep != Py_None) {
        if (PyUnicode_Check(sep))
            return _PyUnicode_XStrip(self, striptype, sep);
        else {
            PyErr_Format(PyExc_TypeError,
                         "%s arg must be None or str",
                         STRIPNAME(striptype));
            return NULL;
        }
    }

    return do_strip(self, striptype);
}


PyDoc_STRVAR(strip__doc__,
             "S.strip([chars]) -> str\n\
\n\
Return a copy of the string S with leading and trailing\n\
whitespace removed.\n\
If chars is given and not None, remove characters in chars instead.");

static PyObject *
unicode_strip(PyUnicodeObject *self, PyObject *args)
{
    if (PyTuple_GET_SIZE(args) == 0)
        return do_strip(self, BOTHSTRIP); /* Common case */
    else
        return do_argstrip(self, BOTHSTRIP, args);
}


PyDoc_STRVAR(lstrip__doc__,
             "S.lstrip([chars]) -> str\n\
\n\
Return a copy of the string S with leading whitespace removed.\n\
If chars is given and not None, remove characters in chars instead.");

static PyObject *
unicode_lstrip(PyUnicodeObject *self, PyObject *args)
{
    if (PyTuple_GET_SIZE(args) == 0)
        return do_strip(self, LEFTSTRIP); /* Common case */
    else
        return do_argstrip(self, LEFTSTRIP, args);
}


PyDoc_STRVAR(rstrip__doc__,
             "S.rstrip([chars]) -> str\n\
\n\
Return a copy of the string S with trailing whitespace removed.\n\
If chars is given and not None, remove characters in chars instead.");

static PyObject *
unicode_rstrip(PyUnicodeObject *self, PyObject *args)
{
    if (PyTuple_GET_SIZE(args) == 0)
        return do_strip(self, RIGHTSTRIP); /* Common case */
    else
        return do_argstrip(self, RIGHTSTRIP, args);
}


static PyObject*
unicode_repeat(PyUnicodeObject *str, Py_ssize_t len)
{
    PyUnicodeObject *u;
    Py_ssize_t nchars, n;

    if (len < 1) {
        Py_INCREF(unicode_empty);
        return unicode_empty;
    }

    if (len == 1 && PyUnicode_CheckExact(str)) {
        /* no repeat, return original string */
        Py_INCREF(str);
        return (PyObject*) str;
    }

    if (PyUnicode_READY(str) == -1)
        return NULL;

    if (PyUnicode_GET_LENGTH(str) > PY_SSIZE_T_MAX / len) {
        PyErr_SetString(PyExc_OverflowError,
                        "repeated string is too long");
        return NULL;
    }
    nchars = len * PyUnicode_GET_LENGTH(str);

    u = (PyUnicodeObject *)PyUnicode_New(nchars, PyUnicode_MAX_CHAR_VALUE(str));
    if (!u)
        return NULL;
    assert(PyUnicode_KIND(u) == PyUnicode_KIND(str));

    if (PyUnicode_GET_LENGTH(str) == 1) {
        const int kind = PyUnicode_KIND(str);
        const Py_UCS4 fill_char = PyUnicode_READ(kind, PyUnicode_DATA(str), 0);
        void *to = PyUnicode_DATA(u);
        if (kind == PyUnicode_1BYTE_KIND)
            memset(to, (unsigned char)fill_char, len);
        else {
            for (n = 0; n < len; ++n)
                PyUnicode_WRITE(kind, to, n, fill_char);
        }
    }
    else {
        /* number of characters copied this far */
        Py_ssize_t done = PyUnicode_GET_LENGTH(str);
        const Py_ssize_t char_size = PyUnicode_CHARACTER_SIZE(str);
        char *to = (char *) PyUnicode_DATA(u);
        Py_MEMCPY(to, PyUnicode_DATA(str),
                  PyUnicode_GET_LENGTH(str) * char_size);
        while (done < nchars) {
            n = (done <= nchars-done) ? done : nchars-done;
            Py_MEMCPY(to + (done * char_size), to, n * char_size);
            done += n;
        }
    }

    return (PyObject*) u;
}

PyObject *
PyUnicode_Replace(PyObject *obj,
                  PyObject *subobj,
                  PyObject *replobj,
                  Py_ssize_t maxcount)
{
    PyObject *self;
    PyObject *str1;
    PyObject *str2;
    PyObject *result;

    self = PyUnicode_FromObject(obj);
    if (self == NULL || PyUnicode_READY(self) == -1)
        return NULL;
    str1 = PyUnicode_FromObject(subobj);
    if (str1 == NULL || PyUnicode_READY(str1) == -1) {
        Py_DECREF(self);
        return NULL;
    }
    str2 = PyUnicode_FromObject(replobj);
    if (str2 == NULL || PyUnicode_READY(str2)) {
        Py_DECREF(self);
        Py_DECREF(str1);
        return NULL;
    }
    result = replace(self, str1, str2, maxcount);
    Py_DECREF(self);
    Py_DECREF(str1);
    Py_DECREF(str2);
    return result;
}

PyDoc_STRVAR(replace__doc__,
             "S.replace(old, new[, count]) -> str\n\
\n\
Return a copy of S with all occurrences of substring\n\
old replaced by new.  If the optional argument count is\n\
given, only the first count occurrences are replaced.");

static PyObject*
unicode_replace(PyObject *self, PyObject *args)
{
    PyObject *str1;
    PyObject *str2;
    Py_ssize_t maxcount = -1;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "OO|n:replace", &str1, &str2, &maxcount))
        return NULL;
    if (!PyUnicode_READY(self) == -1)
        return NULL;
    str1 = PyUnicode_FromObject(str1);
    if (str1 == NULL || PyUnicode_READY(str1) == -1)
        return NULL;
    str2 = PyUnicode_FromObject(str2);
    if (str2 == NULL || PyUnicode_READY(str2) == -1) {
        Py_DECREF(str1);
        return NULL;
    }

    result = replace(self, str1, str2, maxcount);

    Py_DECREF(str1);
    Py_DECREF(str2);
    return result;
}

static PyObject *
unicode_repr(PyObject *unicode)
{
    PyObject *repr;
    Py_ssize_t isize;
    Py_ssize_t osize, squote, dquote, i, o;
    Py_UCS4 max, quote;
    int ikind, okind;
    void *idata, *odata;

    if (PyUnicode_READY(unicode) == -1)
        return NULL;

    isize = PyUnicode_GET_LENGTH(unicode);
    idata = PyUnicode_DATA(unicode);

    /* Compute length of output, quote characters, and
       maximum character */
    osize = 2; /* quotes */
    max = 127;
    squote = dquote = 0;
    ikind = PyUnicode_KIND(unicode);
    for (i = 0; i < isize; i++) {
        Py_UCS4 ch = PyUnicode_READ(ikind, idata, i);
        switch (ch) {
        case '\'': squote++; osize++; break;
        case '"':  dquote++; osize++; break;
        case '\\': case '\t': case '\r': case '\n':
            osize += 2; break;
        default:
            /* Fast-path ASCII */
            if (ch < ' ' || ch == 0x7f)
                osize += 4; /* \xHH */
            else if (ch < 0x7f)
                osize++;
            else if (Py_UNICODE_ISPRINTABLE(ch)) {
                osize++;
                max = ch > max ? ch : max;
            }
            else if (ch < 0x100)
                osize += 4; /* \xHH */
            else if (ch < 0x10000)
                osize += 6; /* \uHHHH */
            else
                osize += 10; /* \uHHHHHHHH */
        }
    }

    quote = '\'';
    if (squote) {
        if (dquote)
            /* Both squote and dquote present. Use squote,
               and escape them */
            osize += squote;
        else
            quote = '"';
    }

    repr = PyUnicode_New(osize, max);
    if (repr == NULL)
        return NULL;
    okind = PyUnicode_KIND(repr);
    odata = PyUnicode_DATA(repr);

    PyUnicode_WRITE(okind, odata, 0, quote);
    PyUnicode_WRITE(okind, odata, osize-1, quote);

    for (i = 0, o = 1; i < isize; i++) {
        Py_UCS4 ch = PyUnicode_READ(ikind, idata, i);

        /* Escape quotes and backslashes */
        if ((ch == quote) || (ch == '\\')) {
            PyUnicode_WRITE(okind, odata, o++, '\\');
            PyUnicode_WRITE(okind, odata, o++, ch);
            continue;
        }

        /* Map special whitespace to '\t', \n', '\r' */
        if (ch == '\t') {
            PyUnicode_WRITE(okind, odata, o++, '\\');
            PyUnicode_WRITE(okind, odata, o++, 't');
        }
        else if (ch == '\n') {
            PyUnicode_WRITE(okind, odata, o++, '\\');
            PyUnicode_WRITE(okind, odata, o++, 'n');
        }
        else if (ch == '\r') {
            PyUnicode_WRITE(okind, odata, o++, '\\');
            PyUnicode_WRITE(okind, odata, o++, 'r');
        }

        /* Map non-printable US ASCII to '\xhh' */
        else if (ch < ' ' || ch == 0x7F) {
            PyUnicode_WRITE(okind, odata, o++, '\\');
            PyUnicode_WRITE(okind, odata, o++, 'x');
            PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 4) & 0x000F]);
            PyUnicode_WRITE(okind, odata, o++, hexdigits[ch & 0x000F]);
        }

        /* Copy ASCII characters as-is */
        else if (ch < 0x7F) {
            PyUnicode_WRITE(okind, odata, o++, ch);
        }

        /* Non-ASCII characters */
        else {
            /* Map Unicode whitespace and control characters
               (categories Z* and C* except ASCII space)
            */
            if (!Py_UNICODE_ISPRINTABLE(ch)) {
                /* Map 8-bit characters to '\xhh' */
                if (ch <= 0xff) {
                    PyUnicode_WRITE(okind, odata, o++, '\\');
                    PyUnicode_WRITE(okind, odata, o++, 'x');
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 4) & 0x000F]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[ch & 0x000F]);
                }
                /* Map 21-bit characters to '\U00xxxxxx' */
                else if (ch >= 0x10000) {
                    PyUnicode_WRITE(okind, odata, o++, '\\');
                    PyUnicode_WRITE(okind, odata, o++, 'U');
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 28) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 24) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 20) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 16) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 12) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 8) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 4) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[ch & 0xF]);
                }
                /* Map 16-bit characters to '\uxxxx' */
                else {
                    PyUnicode_WRITE(okind, odata, o++, '\\');
                    PyUnicode_WRITE(okind, odata, o++, 'u');
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 12) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 8) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[(ch >> 4) & 0xF]);
                    PyUnicode_WRITE(okind, odata, o++, hexdigits[ch & 0xF]);
                }
            }
            /* Copy characters as-is */
            else {
                PyUnicode_WRITE(okind, odata, o++, ch);
            }
        }
    }
    /* Closing quote already added at the beginning */
    return repr;
}

PyDoc_STRVAR(rfind__doc__,
             "S.rfind(sub[, start[, end]]) -> int\n\
\n\
Return the highest index in S where substring sub is found,\n\
such that sub is contained within S[start:end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
unicode_rfind(PyObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;
    Py_ssize_t result;

    if (!stringlib_parse_args_finds_unicode("rfind", args, &substring,
                                            &start, &end))
        return NULL;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    if (PyUnicode_READY(substring) == -1)
        return NULL;

    result = any_find_slice(
        ucs1lib_rfind_slice, ucs2lib_rfind_slice, ucs4lib_rfind_slice,
        self, (PyObject*)substring, start, end
        );

    Py_DECREF(substring);

    if (result == -2)
        return NULL;

    return PyLong_FromSsize_t(result);
}

PyDoc_STRVAR(rindex__doc__,
             "S.rindex(sub[, start[, end]]) -> int\n\
\n\
Like S.rfind() but raise ValueError when the substring is not found.");

static PyObject *
unicode_rindex(PyObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;
    Py_ssize_t result;

    if (!stringlib_parse_args_finds_unicode("rindex", args, &substring,
                                            &start, &end))
        return NULL;

    if (PyUnicode_READY(self) == -1)
        return NULL;
    if (PyUnicode_READY(substring) == -1)
        return NULL;

    result = any_find_slice(
        ucs1lib_rfind_slice, ucs2lib_rfind_slice, ucs4lib_rfind_slice,
        self, (PyObject*)substring, start, end
        );

    Py_DECREF(substring);

    if (result == -2)
        return NULL;

    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }

    return PyLong_FromSsize_t(result);
}

PyDoc_STRVAR(rjust__doc__,
             "S.rjust(width[, fillchar]) -> str\n\
\n\
Return S right-justified in a string of length width. Padding is\n\
done using the specified fill character (default is a space).");

static PyObject *
unicode_rjust(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|O&:rjust", &width, convert_uc, &fillchar))
        return NULL;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    if (_PyUnicode_LENGTH(self) >= width && PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, width - _PyUnicode_LENGTH(self), 0, fillchar);
}

PyObject *
PyUnicode_Split(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)
{
    PyObject *result;

    s = PyUnicode_FromObject(s);
    if (s == NULL)
        return NULL;
    if (sep != NULL) {
        sep = PyUnicode_FromObject(sep);
        if (sep == NULL) {
            Py_DECREF(s);
            return NULL;
        }
    }

    result = split((PyUnicodeObject *)s, (PyUnicodeObject *)sep, maxsplit);

    Py_DECREF(s);
    Py_XDECREF(sep);
    return result;
}

PyDoc_STRVAR(split__doc__,
             "S.split([sep[, maxsplit]]) -> list of strings\n\
\n\
Return a list of the words in S, using sep as the\n\
delimiter string.  If maxsplit is given, at most maxsplit\n\
splits are done. If sep is not specified or is None, any\n\
whitespace string is a separator and empty strings are\n\
removed from the result.");

static PyObject*
unicode_split(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring = Py_None;
    Py_ssize_t maxcount = -1;

    if (!PyArg_ParseTuple(args, "|On:split", &substring, &maxcount))
        return NULL;

    if (substring == Py_None)
        return split(self, NULL, maxcount);
    else if (PyUnicode_Check(substring))
        return split(self, (PyUnicodeObject *)substring, maxcount);
    else
        return PyUnicode_Split((PyObject *)self, substring, maxcount);
}

PyObject *
PyUnicode_Partition(PyObject *str_in, PyObject *sep_in)
{
    PyObject* str_obj;
    PyObject* sep_obj;
    PyObject* out;
    int kind1, kind2, kind;
    void *buf1 = NULL, *buf2 = NULL;
    Py_ssize_t len1, len2;

    str_obj = PyUnicode_FromObject(str_in);
    if (!str_obj || PyUnicode_READY(str_obj) == -1)
        return NULL;
    sep_obj = PyUnicode_FromObject(sep_in);
    if (!sep_obj || PyUnicode_READY(sep_obj) == -1) {
        Py_DECREF(str_obj);
        return NULL;
    }

    kind1 = PyUnicode_KIND(str_in);
    kind2 = PyUnicode_KIND(sep_obj);
    kind = kind1 > kind2 ? kind1 : kind2;
    buf1 = PyUnicode_DATA(str_in);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind(str_in, kind);
    if (!buf1)
        goto onError;
    buf2 = PyUnicode_DATA(sep_obj);
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind(sep_obj, kind);
    if (!buf2)
        goto onError;
    len1 = PyUnicode_GET_LENGTH(str_obj);
    len2 = PyUnicode_GET_LENGTH(sep_obj);

    switch(PyUnicode_KIND(str_in)) {
    case PyUnicode_1BYTE_KIND:
        out = ucs1lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    default:
        assert(0);
        out = 0;
    }

    Py_DECREF(sep_obj);
    Py_DECREF(str_obj);
    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);

    return out;
  onError:
    Py_DECREF(sep_obj);
    Py_DECREF(str_obj);
    if (kind1 != kind && buf1)
        PyMem_Free(buf1);
    if (kind2 != kind && buf2)
        PyMem_Free(buf2);
    return NULL;
}


PyObject *
PyUnicode_RPartition(PyObject *str_in, PyObject *sep_in)
{
    PyObject* str_obj;
    PyObject* sep_obj;
    PyObject* out;
    int kind1, kind2, kind;
    void *buf1 = NULL, *buf2 = NULL;
    Py_ssize_t len1, len2;

    str_obj = PyUnicode_FromObject(str_in);
    if (!str_obj)
        return NULL;
    sep_obj = PyUnicode_FromObject(sep_in);
    if (!sep_obj) {
        Py_DECREF(str_obj);
        return NULL;
    }

    kind1 = PyUnicode_KIND(str_in);
    kind2 = PyUnicode_KIND(sep_obj);
    kind = Py_MAX(kind1, kind2);
    buf1 = PyUnicode_DATA(str_in);
    if (kind1 != kind)
        buf1 = _PyUnicode_AsKind(str_in, kind);
    if (!buf1)
        goto onError;
    buf2 = PyUnicode_DATA(sep_obj);
    if (kind2 != kind)
        buf2 = _PyUnicode_AsKind(sep_obj, kind);
    if (!buf2)
        goto onError;
    len1 = PyUnicode_GET_LENGTH(str_obj);
    len2 = PyUnicode_GET_LENGTH(sep_obj);

    switch(PyUnicode_KIND(str_in)) {
    case PyUnicode_1BYTE_KIND:
        out = ucs1lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    default:
        assert(0);
        out = 0;
    }

    Py_DECREF(sep_obj);
    Py_DECREF(str_obj);
    if (kind1 != kind)
        PyMem_Free(buf1);
    if (kind2 != kind)
        PyMem_Free(buf2);

    return out;
  onError:
    Py_DECREF(sep_obj);
    Py_DECREF(str_obj);
    if (kind1 != kind && buf1)
        PyMem_Free(buf1);
    if (kind2 != kind && buf2)
        PyMem_Free(buf2);
    return NULL;
}

PyDoc_STRVAR(partition__doc__,
             "S.partition(sep) -> (head, sep, tail)\n\
\n\
Search for the separator sep in S, and return the part before it,\n\
the separator itself, and the part after it.  If the separator is not\n\
found, return S and two empty strings.");

static PyObject*
unicode_partition(PyUnicodeObject *self, PyObject *separator)
{
    return PyUnicode_Partition((PyObject *)self, separator);
}

PyDoc_STRVAR(rpartition__doc__,
             "S.rpartition(sep) -> (head, sep, tail)\n\
\n\
Search for the separator sep in S, starting at the end of S, and return\n\
the part before it, the separator itself, and the part after it.  If the\n\
separator is not found, return two empty strings and S.");

static PyObject*
unicode_rpartition(PyUnicodeObject *self, PyObject *separator)
{
    return PyUnicode_RPartition((PyObject *)self, separator);
}

PyObject *
PyUnicode_RSplit(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)
{
    PyObject *result;

    s = PyUnicode_FromObject(s);
    if (s == NULL)
        return NULL;
    if (sep != NULL) {
        sep = PyUnicode_FromObject(sep);
        if (sep == NULL) {
            Py_DECREF(s);
            return NULL;
        }
    }

    result = rsplit((PyUnicodeObject *)s, (PyUnicodeObject *)sep, maxsplit);

    Py_DECREF(s);
    Py_XDECREF(sep);
    return result;
}

PyDoc_STRVAR(rsplit__doc__,
             "S.rsplit([sep[, maxsplit]]) -> list of strings\n\
\n\
Return a list of the words in S, using sep as the\n\
delimiter string, starting at the end of the string and\n\
working to the front.  If maxsplit is given, at most maxsplit\n\
splits are done. If sep is not specified, any whitespace string\n\
is a separator.");

static PyObject*
unicode_rsplit(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring = Py_None;
    Py_ssize_t maxcount = -1;

    if (!PyArg_ParseTuple(args, "|On:rsplit", &substring, &maxcount))
        return NULL;

    if (substring == Py_None)
        return rsplit(self, NULL, maxcount);
    else if (PyUnicode_Check(substring))
        return rsplit(self, (PyUnicodeObject *)substring, maxcount);
    else
        return PyUnicode_RSplit((PyObject *)self, substring, maxcount);
}

PyDoc_STRVAR(splitlines__doc__,
             "S.splitlines([keepends]) -> list of strings\n\
\n\
Return a list of the lines in S, breaking at line boundaries.\n\
Line breaks are not included in the resulting list unless keepends\n\
is given and true.");

static PyObject*
unicode_splitlines(PyUnicodeObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"keepends", 0};
    int keepends = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:splitlines",
                                     kwlist, &keepends))
        return NULL;

    return PyUnicode_Splitlines((PyObject *)self, keepends);
}

static
PyObject *unicode_str(PyObject *self)
{
    if (PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return self;
    } else
        /* Subtype -- return genuine unicode string with the same value. */
        return PyUnicode_Copy(self);
}

PyDoc_STRVAR(swapcase__doc__,
             "S.swapcase() -> str\n\
\n\
Return a copy of S with uppercase characters converted to lowercase\n\
and vice versa.");

static PyObject*
unicode_swapcase(PyUnicodeObject *self)
{
    return fixup(self, fixswapcase);
}

PyDoc_STRVAR(maketrans__doc__,
             "str.maketrans(x[, y[, z]]) -> dict (static method)\n\
\n\
Return a translation table usable for str.translate().\n\
If there is only one argument, it must be a dictionary mapping Unicode\n\
ordinals (integers) or characters to Unicode ordinals, strings or None.\n\
Character keys will be then converted to ordinals.\n\
If there are two arguments, they must be strings of equal length, and\n\
in the resulting dictionary, each character in x will be mapped to the\n\
character at the same position in y. If there is a third argument, it\n\
must be a string, whose characters will be mapped to None in the result.");

static PyObject*
unicode_maketrans(PyUnicodeObject *null, PyObject *args)
{
    PyObject *x, *y = NULL, *z = NULL;
    PyObject *new = NULL, *key, *value;
    Py_ssize_t i = 0;
    int res;

    if (!PyArg_ParseTuple(args, "O|UU:maketrans", &x, &y, &z))
        return NULL;
    new = PyDict_New();
    if (!new)
        return NULL;
    if (y != NULL) {
        int x_kind, y_kind, z_kind;
        void *x_data, *y_data, *z_data;

        /* x must be a string too, of equal length */
        if (!PyUnicode_Check(x)) {
            PyErr_SetString(PyExc_TypeError, "first maketrans argument must "
                            "be a string if there is a second argument");
            goto err;
        }
        if (PyUnicode_GET_LENGTH(x) != PyUnicode_GET_LENGTH(y)) {
            PyErr_SetString(PyExc_ValueError, "the first two maketrans "
                            "arguments must have equal length");
            goto err;
        }
        /* create entries for translating chars in x to those in y */
        x_kind = PyUnicode_KIND(x);
        y_kind = PyUnicode_KIND(y);
        x_data = PyUnicode_DATA(x);
        y_data = PyUnicode_DATA(y);
        for (i = 0; i < PyUnicode_GET_LENGTH(x); i++) {
            key = PyLong_FromLong(PyUnicode_READ(x_kind, x_data, i));
            value = PyLong_FromLong(PyUnicode_READ(y_kind, y_data, i));
            if (!key || !value)
                goto err;
            res = PyDict_SetItem(new, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            if (res < 0)
                goto err;
        }
        /* create entries for deleting chars in z */
        if (z != NULL) {
            z_kind = PyUnicode_KIND(z);
            z_data = PyUnicode_DATA(z);
            for (i = 0; i < PyUnicode_GET_SIZE(z); i++) {
                key = PyLong_FromLong(PyUnicode_READ(z_kind, z_data, i));
                if (!key)
                    goto err;
                res = PyDict_SetItem(new, key, Py_None);
                Py_DECREF(key);
                if (res < 0)
                    goto err;
            }
        }
    } else {
        int kind;
        void *data;

        /* x must be a dict */
        if (!PyDict_CheckExact(x)) {
            PyErr_SetString(PyExc_TypeError, "if you give only one argument "
                            "to maketrans it must be a dict");
            goto err;
        }
        /* copy entries into the new dict, converting string keys to int keys */
        while (PyDict_Next(x, &i, &key, &value)) {
            if (PyUnicode_Check(key)) {
                /* convert string keys to integer keys */
                PyObject *newkey;
                if (PyUnicode_GET_SIZE(key) != 1) {
                    PyErr_SetString(PyExc_ValueError, "string keys in translate "
                                    "table must be of length 1");
                    goto err;
                }
                kind = PyUnicode_KIND(key);
                data = PyUnicode_DATA(key);
                newkey = PyLong_FromLong(PyUnicode_READ(kind, data, 0));
                if (!newkey)
                    goto err;
                res = PyDict_SetItem(new, newkey, value);
                Py_DECREF(newkey);
                if (res < 0)
                    goto err;
            } else if (PyLong_Check(key)) {
                /* just keep integer keys */
                if (PyDict_SetItem(new, key, value) < 0)
                    goto err;
            } else {
                PyErr_SetString(PyExc_TypeError, "keys in translate table must "
                                "be strings or integers");
                goto err;
            }
        }
    }
    return new;
  err:
    Py_DECREF(new);
    return NULL;
}

PyDoc_STRVAR(translate__doc__,
             "S.translate(table) -> str\n\
\n\
Return a copy of the string S, where all characters have been mapped\n\
through the given translation table, which must be a mapping of\n\
Unicode ordinals to Unicode ordinals, strings, or None.\n\
Unmapped characters are left untouched. Characters mapped to None\n\
are deleted.");

static PyObject*
unicode_translate(PyObject *self, PyObject *table)
{
    return _PyUnicode_TranslateCharmap(self, table, "ignore");
}

PyDoc_STRVAR(upper__doc__,
             "S.upper() -> str\n\
\n\
Return a copy of S converted to uppercase.");

static PyObject*
unicode_upper(PyUnicodeObject *self)
{
    return fixup(self, fixupper);
}

PyDoc_STRVAR(zfill__doc__,
             "S.zfill(width) -> str\n\
\n\
Pad a numeric string S with zeros on the left, to fill a field\n\
of the specified width. The string S is never truncated.");

static PyObject *
unicode_zfill(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t fill;
    PyUnicodeObject *u;
    Py_ssize_t width;
    int kind;
    void *data;
    Py_UCS4 chr;

    if (PyUnicode_READY(self) == -1)
        return NULL;

    if (!PyArg_ParseTuple(args, "n:zfill", &width))
        return NULL;

    if (PyUnicode_GET_LENGTH(self) >= width) {
        if (PyUnicode_CheckExact(self)) {
            Py_INCREF(self);
            return (PyObject*) self;
        }
        else
            return PyUnicode_Copy((PyObject*)self);
    }

    fill = width - _PyUnicode_LENGTH(self);

    u = pad(self, fill, 0, '0');

    if (u == NULL)
        return NULL;

    kind = PyUnicode_KIND(u);
    data = PyUnicode_DATA(u);
    chr = PyUnicode_READ(kind, data, fill);

    if (chr == '+' || chr == '-') {
        /* move sign to beginning of string */
        PyUnicode_WRITE(kind, data, 0, chr);
        PyUnicode_WRITE(kind, data, fill, '0');
    }

    return (PyObject*) u;
}

#if 0
static PyObject *
unicode__decimal2ascii(PyObject *self)
{
    return PyUnicode_TransformDecimalAndSpaceToASCII(self);
}
#endif

PyDoc_STRVAR(startswith__doc__,
             "S.startswith(prefix[, start[, end]]) -> bool\n\
\n\
Return True if S starts with the specified prefix, False otherwise.\n\
With optional start, test S beginning at that position.\n\
With optional end, stop comparing S at that position.\n\
prefix can also be a tuple of strings to try.");

static PyObject *
unicode_startswith(PyUnicodeObject *self,
                   PyObject *args)
{
    PyObject *subobj;
    PyUnicodeObject *substring;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    int result;

    if (!stringlib_parse_args_finds("startswith", args, &subobj, &start, &end))
        return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            substring = (PyUnicodeObject *)PyUnicode_FromObject(
                PyTuple_GET_ITEM(subobj, i));
            if (substring == NULL)
                return NULL;
            result = tailmatch(self, substring, start, end, -1);
            Py_DECREF(substring);
            if (result) {
                Py_RETURN_TRUE;
            }
        }
        /* nothing matched */
        Py_RETURN_FALSE;
    }
    substring = (PyUnicodeObject *)PyUnicode_FromObject(subobj);
    if (substring == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Format(PyExc_TypeError, "startswith first arg must be str or "
                         "a tuple of str, not %s", Py_TYPE(subobj)->tp_name);
        return NULL;
    }
    result = tailmatch(self, substring, start, end, -1);
    Py_DECREF(substring);
    return PyBool_FromLong(result);
}


PyDoc_STRVAR(endswith__doc__,
             "S.endswith(suffix[, start[, end]]) -> bool\n\
\n\
Return True if S ends with the specified suffix, False otherwise.\n\
With optional start, test S beginning at that position.\n\
With optional end, stop comparing S at that position.\n\
suffix can also be a tuple of strings to try.");

static PyObject *
unicode_endswith(PyUnicodeObject *self,
                 PyObject *args)
{
    PyObject *subobj;
    PyUnicodeObject *substring;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    int result;

    if (!stringlib_parse_args_finds("endswith", args, &subobj, &start, &end))
        return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            substring = (PyUnicodeObject *)PyUnicode_FromObject(
                PyTuple_GET_ITEM(subobj, i));
            if (substring == NULL)
                return NULL;
            result = tailmatch(self, substring, start, end, +1);
            Py_DECREF(substring);
            if (result) {
                Py_RETURN_TRUE;
            }
        }
        Py_RETURN_FALSE;
    }
    substring = (PyUnicodeObject *)PyUnicode_FromObject(subobj);
    if (substring == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_Format(PyExc_TypeError, "endswith first arg must be str or "
                         "a tuple of str, not %s", Py_TYPE(subobj)->tp_name);
        return NULL;
    }
    result = tailmatch(self, substring, start, end, +1);
    Py_DECREF(substring);
    return PyBool_FromLong(result);
}

#include "stringlib/unicode_format.h"

PyDoc_STRVAR(format__doc__,
             "S.format(*args, **kwargs) -> str\n\
\n\
Return a formatted version of S, using substitutions from args and kwargs.\n\
The substitutions are identified by braces ('{' and '}').");

PyDoc_STRVAR(format_map__doc__,
             "S.format_map(mapping) -> str\n\
\n\
Return a formatted version of S, using substitutions from mapping.\n\
The substitutions are identified by braces ('{' and '}').");

static PyObject *
unicode__format__(PyObject* self, PyObject* args)
{
    PyObject *format_spec;

    if (!PyArg_ParseTuple(args, "U:__format__", &format_spec))
        return NULL;

    return _PyUnicode_FormatAdvanced(self, format_spec, 0,
                                     PyUnicode_GET_LENGTH(format_spec));
}

PyDoc_STRVAR(p_format__doc__,
             "S.__format__(format_spec) -> str\n\
\n\
Return a formatted version of S as described by format_spec.");

static PyObject *
unicode__sizeof__(PyUnicodeObject *v)
{
    Py_ssize_t size;

    /* If it's a compact object, account for base structure +
       character data. */
    if (PyUnicode_IS_COMPACT_ASCII(v))
        size = sizeof(PyASCIIObject) + PyUnicode_GET_LENGTH(v) + 1;
    else if (PyUnicode_IS_COMPACT(v))
        size = sizeof(PyCompactUnicodeObject) +
            (PyUnicode_GET_LENGTH(v) + 1) * PyUnicode_CHARACTER_SIZE(v);
    else {
        /* If it is a two-block object, account for base object, and
           for character block if present. */
        size = sizeof(PyUnicodeObject);
        if (_PyUnicode_DATA_ANY(v))
            size += (PyUnicode_GET_LENGTH(v) + 1) *
                PyUnicode_CHARACTER_SIZE(v);
    }
    /* If the wstr pointer is present, account for it unless it is shared
       with the data pointer. Check if the data is not shared. */
    if (_PyUnicode_HAS_WSTR_MEMORY(v))
        size += (PyUnicode_WSTR_LENGTH(v) + 1) * sizeof(wchar_t);
    if (_PyUnicode_HAS_UTF8_MEMORY(v))
        size += PyUnicode_UTF8_LENGTH(v) + 1;

    return PyLong_FromSsize_t(size);
}

PyDoc_STRVAR(sizeof__doc__,
             "S.__sizeof__() -> size of S in memory, in bytes");

static PyObject *
unicode_getnewargs(PyObject *v)
{
    PyObject *copy = PyUnicode_Copy(v);
    if (!copy)
        return NULL;
    return Py_BuildValue("(N)", copy);
}

static PyMethodDef unicode_methods[] = {

    /* Order is according to common usage: often used methods should
       appear first, since lookup is done sequentially. */

    {"encode", (PyCFunction) unicode_encode, METH_VARARGS | METH_KEYWORDS, encode__doc__},
    {"replace", (PyCFunction) unicode_replace, METH_VARARGS, replace__doc__},
    {"split", (PyCFunction) unicode_split, METH_VARARGS, split__doc__},
    {"rsplit", (PyCFunction) unicode_rsplit, METH_VARARGS, rsplit__doc__},
    {"join", (PyCFunction) unicode_join, METH_O, join__doc__},
    {"capitalize", (PyCFunction) unicode_capitalize, METH_NOARGS, capitalize__doc__},
    {"title", (PyCFunction) unicode_title, METH_NOARGS, title__doc__},
    {"center", (PyCFunction) unicode_center, METH_VARARGS, center__doc__},
    {"count", (PyCFunction) unicode_count, METH_VARARGS, count__doc__},
    {"expandtabs", (PyCFunction) unicode_expandtabs, METH_VARARGS, expandtabs__doc__},
    {"find", (PyCFunction) unicode_find, METH_VARARGS, find__doc__},
    {"partition", (PyCFunction) unicode_partition, METH_O, partition__doc__},
    {"index", (PyCFunction) unicode_index, METH_VARARGS, index__doc__},
    {"ljust", (PyCFunction) unicode_ljust, METH_VARARGS, ljust__doc__},
    {"lower", (PyCFunction) unicode_lower, METH_NOARGS, lower__doc__},
    {"lstrip", (PyCFunction) unicode_lstrip, METH_VARARGS, lstrip__doc__},
    {"rfind", (PyCFunction) unicode_rfind, METH_VARARGS, rfind__doc__},
    {"rindex", (PyCFunction) unicode_rindex, METH_VARARGS, rindex__doc__},
    {"rjust", (PyCFunction) unicode_rjust, METH_VARARGS, rjust__doc__},
    {"rstrip", (PyCFunction) unicode_rstrip, METH_VARARGS, rstrip__doc__},
    {"rpartition", (PyCFunction) unicode_rpartition, METH_O, rpartition__doc__},
    {"splitlines", (PyCFunction) unicode_splitlines, METH_VARARGS | METH_KEYWORDS, splitlines__doc__},
    {"strip", (PyCFunction) unicode_strip, METH_VARARGS, strip__doc__},
    {"swapcase", (PyCFunction) unicode_swapcase, METH_NOARGS, swapcase__doc__},
    {"translate", (PyCFunction) unicode_translate, METH_O, translate__doc__},
    {"upper", (PyCFunction) unicode_upper, METH_NOARGS, upper__doc__},
    {"startswith", (PyCFunction) unicode_startswith, METH_VARARGS, startswith__doc__},
    {"endswith", (PyCFunction) unicode_endswith, METH_VARARGS, endswith__doc__},
    {"islower", (PyCFunction) unicode_islower, METH_NOARGS, islower__doc__},
    {"isupper", (PyCFunction) unicode_isupper, METH_NOARGS, isupper__doc__},
    {"istitle", (PyCFunction) unicode_istitle, METH_NOARGS, istitle__doc__},
    {"isspace", (PyCFunction) unicode_isspace, METH_NOARGS, isspace__doc__},
    {"isdecimal", (PyCFunction) unicode_isdecimal, METH_NOARGS, isdecimal__doc__},
    {"isdigit", (PyCFunction) unicode_isdigit, METH_NOARGS, isdigit__doc__},
    {"isnumeric", (PyCFunction) unicode_isnumeric, METH_NOARGS, isnumeric__doc__},
    {"isalpha", (PyCFunction) unicode_isalpha, METH_NOARGS, isalpha__doc__},
    {"isalnum", (PyCFunction) unicode_isalnum, METH_NOARGS, isalnum__doc__},
    {"isidentifier", (PyCFunction) unicode_isidentifier, METH_NOARGS, isidentifier__doc__},
    {"isprintable", (PyCFunction) unicode_isprintable, METH_NOARGS, isprintable__doc__},
    {"zfill", (PyCFunction) unicode_zfill, METH_VARARGS, zfill__doc__},
    {"format", (PyCFunction) do_string_format, METH_VARARGS | METH_KEYWORDS, format__doc__},
    {"format_map", (PyCFunction) do_string_format_map, METH_O, format_map__doc__},
    {"__format__", (PyCFunction) unicode__format__, METH_VARARGS, p_format__doc__},
    {"maketrans", (PyCFunction) unicode_maketrans,
     METH_VARARGS | METH_STATIC, maketrans__doc__},
    {"__sizeof__", (PyCFunction) unicode__sizeof__, METH_NOARGS, sizeof__doc__},
#if 0
    {"capwords", (PyCFunction) unicode_capwords, METH_NOARGS, capwords__doc__},
#endif

#if 0
    /* These methods are just used for debugging the implementation. */
    {"_decimal2ascii", (PyCFunction) unicode__decimal2ascii, METH_NOARGS},
#endif

    {"__getnewargs__",  (PyCFunction)unicode_getnewargs, METH_NOARGS},
    {NULL, NULL}
};

static PyObject *
unicode_mod(PyObject *v, PyObject *w)
{
    if (!PyUnicode_Check(v))
        Py_RETURN_NOTIMPLEMENTED;
    return PyUnicode_Format(v, w);
}

static PyNumberMethods unicode_as_number = {
    0,              /*nb_add*/
    0,              /*nb_subtract*/
    0,              /*nb_multiply*/
    unicode_mod,            /*nb_remainder*/
};

static PySequenceMethods unicode_as_sequence = {
    (lenfunc) unicode_length,       /* sq_length */
    PyUnicode_Concat,           /* sq_concat */
    (ssizeargfunc) unicode_repeat,  /* sq_repeat */
    (ssizeargfunc) unicode_getitem,     /* sq_item */
    0,                  /* sq_slice */
    0,                  /* sq_ass_item */
    0,                  /* sq_ass_slice */
    PyUnicode_Contains,         /* sq_contains */
};

static PyObject*
unicode_subscript(PyUnicodeObject* self, PyObject* item)
{
    if (PyUnicode_READY(self) == -1)
        return NULL;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyUnicode_GET_LENGTH(self);
        return unicode_getitem((PyObject*)self, i);
    } else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;
        PyObject *result;
        void *src_data, *dest_data;
        int src_kind, dest_kind;
        Py_UCS4 ch, max_char;

        if (PySlice_GetIndicesEx(item, PyUnicode_GET_LENGTH(self),
                                 &start, &stop, &step, &slicelength) < 0) {
            return NULL;
        }

        if (slicelength <= 0) {
            return PyUnicode_New(0, 0);
        } else if (start == 0 && step == 1 &&
                   slicelength == PyUnicode_GET_LENGTH(self) &&
                   PyUnicode_CheckExact(self)) {
            Py_INCREF(self);
            return (PyObject *)self;
        } else if (step == 1) {
            return PyUnicode_Substring((PyObject*)self,
                                       start, start + slicelength);
        }
        /* General case */
        max_char = 127;
        src_kind = PyUnicode_KIND(self);
        src_data = PyUnicode_DATA(self);
        for (cur = start, i = 0; i < slicelength; cur += step, i++) {
            ch = PyUnicode_READ(src_kind, src_data, cur);
            if (ch > max_char)
                max_char = ch;
        }
        result = PyUnicode_New(slicelength, max_char);
        if (result == NULL)
            return NULL;
        dest_kind = PyUnicode_KIND(result);
        dest_data = PyUnicode_DATA(result);

        for (cur = start, i = 0; i < slicelength; cur += step, i++) {
            Py_UCS4 ch = PyUnicode_READ(src_kind, src_data, cur);
            PyUnicode_WRITE(dest_kind, dest_data, i, ch);
        }
        return result;
    } else {
        PyErr_SetString(PyExc_TypeError, "string indices must be integers");
        return NULL;
    }
}

static PyMappingMethods unicode_as_mapping = {
    (lenfunc)unicode_length,        /* mp_length */
    (binaryfunc)unicode_subscript,  /* mp_subscript */
    (objobjargproc)0,           /* mp_ass_subscript */
};


/* Helpers for PyUnicode_Format() */

static PyObject *
getnextarg(PyObject *args, Py_ssize_t arglen, Py_ssize_t *p_argidx)
{
    Py_ssize_t argidx = *p_argidx;
    if (argidx < arglen) {
        (*p_argidx)++;
        if (arglen < 0)
            return args;
        else
            return PyTuple_GetItem(args, argidx);
    }
    PyErr_SetString(PyExc_TypeError,
                    "not enough arguments for format string");
    return NULL;
}

/* Returns a new reference to a PyUnicode object, or NULL on failure. */

static PyObject *
formatfloat(PyObject *v, int flags, int prec, int type)
{
    char *p;
    PyObject *result;
    double x;

    x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;

    if (prec < 0)
        prec = 6;

    p = PyOS_double_to_string(x, type, prec,
                              (flags & F_ALT) ? Py_DTSF_ALT : 0, NULL);
    if (p == NULL)
        return NULL;
    result = PyUnicode_DecodeASCII(p, strlen(p), NULL);
    PyMem_Free(p);
    return result;
}

static PyObject*
formatlong(PyObject *val, int flags, int prec, int type)
{
    char *buf;
    int len;
    PyObject *str; /* temporary string object. */
    PyObject *result;

    str = _PyBytes_FormatLong(val, flags, prec, type, &buf, &len);
    if (!str)
        return NULL;
    result = PyUnicode_DecodeASCII(buf, len, NULL);
    Py_DECREF(str);
    return result;
}

static int
formatchar(Py_UCS4 *buf,
           size_t buflen,
           PyObject *v)
{
    /* presume that the buffer is at least 3 characters long */
    if (PyUnicode_Check(v)) {
        if (PyUnicode_GET_LENGTH(v) == 1) {
            buf[0] = PyUnicode_READ_CHAR(v, 0);
            buf[1] = '\0';
            return 1;
        }
        goto onError;
    }
    else {
        /* Integer input truncated to a character */
        long x;
        x = PyLong_AsLong(v);
        if (x == -1 && PyErr_Occurred())
            goto onError;

        if (x < 0 || x > 0x10ffff) {
            PyErr_SetString(PyExc_OverflowError,
                            "%c arg not in range(0x110000)");
            return -1;
        }

        buf[0] = (Py_UCS4) x;
        buf[1] = '\0';
        return 1;
    }

  onError:
    PyErr_SetString(PyExc_TypeError,
                    "%c requires int or char");
    return -1;
}

/* fmt%(v1,v2,...) is roughly equivalent to sprintf(fmt, v1, v2, ...)
   FORMATBUFLEN is the length of the buffer in which chars are formatted.
*/
#define FORMATBUFLEN (size_t)10

PyObject *
PyUnicode_Format(PyObject *format, PyObject *args)
{
    void *fmt;
    int fmtkind;
    PyObject *result;
    Py_UCS4 *res, *res0;
    Py_UCS4 max;
    int kind;
    Py_ssize_t fmtcnt, fmtpos, rescnt, reslen, arglen, argidx;
    int args_owned = 0;
    PyObject *dict = NULL;
    PyUnicodeObject *uformat;

    if (format == NULL || args == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    uformat = (PyUnicodeObject*)PyUnicode_FromObject(format);
    if (uformat == NULL || PyUnicode_READY(uformat) == -1)
        return NULL;
    fmt = PyUnicode_DATA(uformat);
    fmtkind = PyUnicode_KIND(uformat);
    fmtcnt = PyUnicode_GET_LENGTH(uformat);
    fmtpos = 0;

    reslen = rescnt = fmtcnt + 100;
    res = res0 = PyMem_Malloc(reslen * sizeof(Py_UCS4));
    if (res0 == NULL) {
        PyErr_NoMemory();
        goto onError;
    }

    if (PyTuple_Check(args)) {
        arglen = PyTuple_Size(args);
        argidx = 0;
    }
    else {
        arglen = -1;
        argidx = -2;
    }
    if (Py_TYPE(args)->tp_as_mapping && !PyTuple_Check(args) &&
        !PyUnicode_Check(args))
        dict = args;

    while (--fmtcnt >= 0) {
        if (PyUnicode_READ(fmtkind, fmt, fmtpos) != '%') {
            if (--rescnt < 0) {
                rescnt = fmtcnt + 100;
                reslen += rescnt;
                res0 = PyMem_Realloc(res0, reslen*sizeof(Py_UCS4));
                if (res0 == NULL){
                    PyErr_NoMemory();
                    goto onError;
                }
                res = res0 + reslen - rescnt;
                --rescnt;
            }
            *res++ = PyUnicode_READ(fmtkind, fmt, fmtpos++);
        }
        else {
            /* Got a format specifier */
            int flags = 0;
            Py_ssize_t width = -1;
            int prec = -1;
            Py_UCS4 c = '\0';
            Py_UCS4 fill;
            int isnumok;
            PyObject *v = NULL;
            PyObject *temp = NULL;
            void *pbuf;
            Py_ssize_t pindex;
            Py_UNICODE sign;
            Py_ssize_t len, len1;
            Py_UCS4 formatbuf[FORMATBUFLEN]; /* For formatchar() */

            fmtpos++;
            if (PyUnicode_READ(fmtkind, fmt, fmtpos) == '(') {
                Py_ssize_t keystart;
                Py_ssize_t keylen;
                PyObject *key;
                int pcount = 1;

                if (dict == NULL) {
                    PyErr_SetString(PyExc_TypeError,
                                    "format requires a mapping");
                    goto onError;
                }
                ++fmtpos;
                --fmtcnt;
                keystart = fmtpos;
                /* Skip over balanced parentheses */
                while (pcount > 0 && --fmtcnt >= 0) {
                    if (PyUnicode_READ(fmtkind, fmt, fmtpos) == ')')
                        --pcount;
                    else if (PyUnicode_READ(fmtkind, fmt, fmtpos) == '(')
                        ++pcount;
                    fmtpos++;
                }
                keylen = fmtpos - keystart - 1;
                if (fmtcnt < 0 || pcount > 0) {
                    PyErr_SetString(PyExc_ValueError,
                                    "incomplete format key");
                    goto onError;
                }
                key = PyUnicode_Substring((PyObject*)uformat,
                                          keystart, keystart + keylen);
                if (key == NULL)
                    goto onError;
                if (args_owned) {
                    Py_DECREF(args);
                    args_owned = 0;
                }
                args = PyObject_GetItem(dict, key);
                Py_DECREF(key);
                if (args == NULL) {
                    goto onError;
                }
                args_owned = 1;
                arglen = -1;
                argidx = -2;
            }
            while (--fmtcnt >= 0) {
                switch (c = PyUnicode_READ(fmtkind, fmt, fmtpos++)) {
                case '-': flags |= F_LJUST; continue;
                case '+': flags |= F_SIGN; continue;
                case ' ': flags |= F_BLANK; continue;
                case '#': flags |= F_ALT; continue;
                case '0': flags |= F_ZERO; continue;
                }
                break;
            }
            if (c == '*') {
                v = getnextarg(args, arglen, &argidx);
                if (v == NULL)
                    goto onError;
                if (!PyLong_Check(v)) {
                    PyErr_SetString(PyExc_TypeError,
                                    "* wants int");
                    goto onError;
                }
                width = PyLong_AsLong(v);
                if (width == -1 && PyErr_Occurred())
                    goto onError;
                if (width < 0) {
                    flags |= F_LJUST;
                    width = -width;
                }
                if (--fmtcnt >= 0)
                    c = PyUnicode_READ(fmtkind, fmt, fmtpos++);
            }
            else if (c >= '0' && c <= '9') {
                width = c - '0';
                while (--fmtcnt >= 0) {
                    c = PyUnicode_READ(fmtkind, fmt, fmtpos++);
                    if (c < '0' || c > '9')
                        break;
                    if ((width*10) / 10 != width) {
                        PyErr_SetString(PyExc_ValueError,
                                        "width too big");
                        goto onError;
                    }
                    width = width*10 + (c - '0');
                }
            }
            if (c == '.') {
                prec = 0;
                if (--fmtcnt >= 0)
                    c = PyUnicode_READ(fmtkind, fmt, fmtpos++);
                if (c == '*') {
                    v = getnextarg(args, arglen, &argidx);
                    if (v == NULL)
                        goto onError;
                    if (!PyLong_Check(v)) {
                        PyErr_SetString(PyExc_TypeError,
                                        "* wants int");
                        goto onError;
                    }
                    prec = PyLong_AsLong(v);
                    if (prec == -1 && PyErr_Occurred())
                        goto onError;
                    if (prec < 0)
                        prec = 0;
                    if (--fmtcnt >= 0)
                        c = PyUnicode_READ(fmtkind, fmt, fmtpos++);
                }
                else if (c >= '0' && c <= '9') {
                    prec = c - '0';
                    while (--fmtcnt >= 0) {
                        c = PyUnicode_READ(fmtkind, fmt, fmtpos++);
                        if (c < '0' || c > '9')
                            break;
                        if ((prec*10) / 10 != prec) {
                            PyErr_SetString(PyExc_ValueError,
                                            "prec too big");
                            goto onError;
                        }
                        prec = prec*10 + (c - '0');
                    }
                }
            } /* prec */
            if (fmtcnt >= 0) {
                if (c == 'h' || c == 'l' || c == 'L') {
                    if (--fmtcnt >= 0)
                        c = PyUnicode_READ(fmtkind, fmt, fmtpos++);
                }
            }
            if (fmtcnt < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "incomplete format");
                goto onError;
            }
            if (c != '%') {
                v = getnextarg(args, arglen, &argidx);
                if (v == NULL)
                    goto onError;
            }
            sign = 0;
            fill = ' ';
            switch (c) {

            case '%':
                pbuf = formatbuf;
                kind = PyUnicode_4BYTE_KIND;
                /* presume that buffer length is at least 1 */
                PyUnicode_WRITE(kind, pbuf, 0, '%');
                len = 1;
                break;

            case 's':
            case 'r':
            case 'a':
                if (PyUnicode_CheckExact(v) && c == 's') {
                    temp = v;
                    Py_INCREF(temp);
                }
                else {
                    if (c == 's')
                        temp = PyObject_Str(v);
                    else if (c == 'r')
                        temp = PyObject_Repr(v);
                    else
                        temp = PyObject_ASCII(v);
                    if (temp == NULL)
                        goto onError;
                    if (PyUnicode_Check(temp))
                        /* nothing to do */;
                    else {
                        Py_DECREF(temp);
                        PyErr_SetString(PyExc_TypeError,
                                        "%s argument has non-string str()");
                        goto onError;
                    }
                }
                if (PyUnicode_READY(temp) == -1) {
                    Py_CLEAR(temp);
                    goto onError;
                }
                pbuf = PyUnicode_DATA(temp);
                kind = PyUnicode_KIND(temp);
                len = PyUnicode_GET_LENGTH(temp);
                if (prec >= 0 && len > prec)
                    len = prec;
                break;

            case 'i':
            case 'd':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
                isnumok = 0;
                if (PyNumber_Check(v)) {
                    PyObject *iobj=NULL;

                    if (PyLong_Check(v)) {
                        iobj = v;
                        Py_INCREF(iobj);
                    }
                    else {
                        iobj = PyNumber_Long(v);
                    }
                    if (iobj!=NULL) {
                        if (PyLong_Check(iobj)) {
                            isnumok = 1;
                            temp = formatlong(iobj, flags, prec, (c == 'i'? 'd': c));
                            Py_DECREF(iobj);
                            if (!temp)
                                goto onError;
                            if (PyUnicode_READY(temp) == -1) {
                                Py_CLEAR(temp);
                                goto onError;
                            }
                            pbuf = PyUnicode_DATA(temp);
                            kind = PyUnicode_KIND(temp);
                            len = PyUnicode_GET_LENGTH(temp);
                            sign = 1;
                        }
                        else {
                            Py_DECREF(iobj);
                        }
                    }
                }
                if (!isnumok) {
                    PyErr_Format(PyExc_TypeError,
                                 "%%%c format: a number is required, "
                                 "not %.200s", (char)c, Py_TYPE(v)->tp_name);
                    goto onError;
                }
                if (flags & F_ZERO)
                    fill = '0';
                break;

            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
                temp = formatfloat(v, flags, prec, c);
                if (!temp)
                    goto onError;
                if (PyUnicode_READY(temp) == -1) {
                    Py_CLEAR(temp);
                    goto onError;
                }
                pbuf = PyUnicode_DATA(temp);
                kind = PyUnicode_KIND(temp);
                len = PyUnicode_GET_LENGTH(temp);
                sign = 1;
                if (flags & F_ZERO)
                    fill = '0';
                break;

            case 'c':
                pbuf = formatbuf;
                kind = PyUnicode_4BYTE_KIND;
                len = formatchar(pbuf, Py_ARRAY_LENGTH(formatbuf), v);
                if (len < 0)
                    goto onError;
                break;

            default:
                PyErr_Format(PyExc_ValueError,
                             "unsupported format character '%c' (0x%x) "
                             "at index %zd",
                             (31<=c && c<=126) ? (char)c : '?',
                             (int)c,
                             fmtpos - 1);
                goto onError;
            }
            /* pbuf is initialized here. */
            pindex = 0;
            if (sign) {
                if (PyUnicode_READ(kind, pbuf, pindex) == '-' ||
                    PyUnicode_READ(kind, pbuf, pindex) == '+') {
                    sign = PyUnicode_READ(kind, pbuf, pindex++);
                    len--;
                }
                else if (flags & F_SIGN)
                    sign = '+';
                else if (flags & F_BLANK)
                    sign = ' ';
                else
                    sign = 0;
            }
            if (width < len)
                width = len;
            if (rescnt - (sign != 0) < width) {
                reslen -= rescnt;
                rescnt = width + fmtcnt + 100;
                reslen += rescnt;
                if (reslen < 0) {
                    Py_XDECREF(temp);
                    PyErr_NoMemory();
                    goto onError;
                }
                res0 = PyMem_Realloc(res0, reslen*sizeof(Py_UCS4));
                if (res0 == 0) {
                    PyErr_NoMemory();
                    Py_XDECREF(temp);
                    goto onError;
                }
                res = res0 + reslen - rescnt;
            }
            if (sign) {
                if (fill != ' ')
                    *res++ = sign;
                rescnt--;
                if (width > len)
                    width--;
            }
            if ((flags & F_ALT) && (c == 'x' || c == 'X' || c == 'o')) {
                assert(PyUnicode_READ(kind, pbuf, pindex) == '0');
                assert(PyUnicode_READ(kind, pbuf, pindex+1) == c);
                if (fill != ' ') {
                    *res++ = PyUnicode_READ(kind, pbuf, pindex++);
                    *res++ = PyUnicode_READ(kind, pbuf, pindex++);
                }
                rescnt -= 2;
                width -= 2;
                if (width < 0)
                    width = 0;
                len -= 2;
            }
            if (width > len && !(flags & F_LJUST)) {
                do {
                    --rescnt;
                    *res++ = fill;
                } while (--width > len);
            }
            if (fill == ' ') {
                if (sign)
                    *res++ = sign;
                if ((flags & F_ALT) && (c == 'x' || c == 'X' || c == 'o')) {
                    assert(PyUnicode_READ(kind, pbuf, pindex) == '0');
                    assert(PyUnicode_READ(kind, pbuf, pindex+1) == c);
                    *res++ = PyUnicode_READ(kind, pbuf, pindex++);
                    *res++ = PyUnicode_READ(kind, pbuf, pindex++);
                }
            }
            /* Copy all characters, preserving len */
            len1 = len;
            while (len1--) {
                *res++ = PyUnicode_READ(kind, pbuf, pindex++);
                rescnt--;
            }
            while (--width >= len) {
                --rescnt;
                *res++ = ' ';
            }
            if (dict && (argidx < arglen) && c != '%') {
                PyErr_SetString(PyExc_TypeError,
                                "not all arguments converted during string formatting");
                Py_XDECREF(temp);
                goto onError;
            }
            Py_XDECREF(temp);
        } /* '%' */
    } /* until end */
    if (argidx < arglen && !dict) {
        PyErr_SetString(PyExc_TypeError,
                        "not all arguments converted during string formatting");
        goto onError;
    }


    for (max=0, res = res0; res < res0+reslen-rescnt; res++)
        if (*res > max)
            max = *res;
    result = PyUnicode_New(reslen - rescnt, max);
    if (!result)
        goto onError;
    kind = PyUnicode_KIND(result);
    for (res = res0; res < res0+reslen-rescnt; res++)
        PyUnicode_WRITE(kind, PyUnicode_DATA(result), res-res0, *res);
    PyMem_Free(res0);
    if (args_owned) {
        Py_DECREF(args);
    }
    Py_DECREF(uformat);
    return (PyObject *)result;

  onError:
    PyMem_Free(res0);
    Py_DECREF(uformat);
    if (args_owned) {
        Py_DECREF(args);
    }
    return NULL;
}

static PyObject *
unicode_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
unicode_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *x = NULL;
    static char *kwlist[] = {"object", "encoding", "errors", 0};
    char *encoding = NULL;
    char *errors = NULL;

    if (type != &PyUnicode_Type)
        return unicode_subtype_new(type, args, kwds);
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oss:str",
                                     kwlist, &x, &encoding, &errors))
        return NULL;
    if (x == NULL)
        return (PyObject *)PyUnicode_New(0, 0);
    if (encoding == NULL && errors == NULL)
        return PyObject_Str(x);
    else
        return PyUnicode_FromEncodedObject(x, encoding, errors);
}

static PyObject *
unicode_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyUnicodeObject *unicode, *self;
    Py_ssize_t length, char_size;
    int share_wstr, share_utf8;
    unsigned int kind;
    void *data;

    assert(PyType_IsSubtype(type, &PyUnicode_Type));

    unicode = (PyUnicodeObject *)unicode_new(&PyUnicode_Type, args, kwds);
    if (unicode == NULL)
        return NULL;
    assert(_PyUnicode_CHECK(unicode));
    if (_PyUnicode_READY_REPLACE(&unicode))
        return NULL;

    self = (PyUnicodeObject *) type->tp_alloc(type, 0);
    if (self == NULL) {
        Py_DECREF(unicode);
        return NULL;
    }
    kind = PyUnicode_KIND(unicode);
    length = PyUnicode_GET_LENGTH(unicode);

    _PyUnicode_LENGTH(self) = length;
    _PyUnicode_HASH(self) = _PyUnicode_HASH(unicode);
    _PyUnicode_STATE(self).interned = 0;
    _PyUnicode_STATE(self).kind = kind;
    _PyUnicode_STATE(self).compact = 0;
    _PyUnicode_STATE(self).ascii = _PyUnicode_STATE(unicode).ascii;
    _PyUnicode_STATE(self).ready = 1;
    _PyUnicode_WSTR(self) = NULL;
    _PyUnicode_UTF8_LENGTH(self) = 0;
    _PyUnicode_UTF8(self) = NULL;
    _PyUnicode_WSTR_LENGTH(self) = 0;
    _PyUnicode_DATA_ANY(self) = NULL;

    share_utf8 = 0;
    share_wstr = 0;
    if (kind == PyUnicode_1BYTE_KIND) {
        char_size = 1;
        if (PyUnicode_MAX_CHAR_VALUE(unicode) < 128)
            share_utf8 = 1;
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        char_size = 2;
        if (sizeof(wchar_t) == 2)
            share_wstr = 1;
    }
    else {
        assert(kind == PyUnicode_4BYTE_KIND);
        char_size = 4;
        if (sizeof(wchar_t) == 4)
            share_wstr = 1;
    }

    /* Ensure we won't overflow the length. */
    if (length > (PY_SSIZE_T_MAX / char_size - 1)) {
        PyErr_NoMemory();
        goto onError;
    }
    data = PyObject_MALLOC((length + 1) * char_size);
    if (data == NULL) {
        PyErr_NoMemory();
        goto onError;
    }

    _PyUnicode_DATA_ANY(self) = data;
    if (share_utf8) {
        _PyUnicode_UTF8_LENGTH(self) = length;
        _PyUnicode_UTF8(self) = data;
    }
    if (share_wstr) {
        _PyUnicode_WSTR_LENGTH(self) = length;
        _PyUnicode_WSTR(self) = (wchar_t *)data;
    }

    Py_MEMCPY(data, PyUnicode_DATA(unicode),
              PyUnicode_KIND_SIZE(kind, length + 1));
    Py_DECREF(unicode);
    return (PyObject *)self;

onError:
    Py_DECREF(unicode);
    Py_DECREF(self);
    return NULL;
}

PyDoc_STRVAR(unicode_doc,
             "str(string[, encoding[, errors]]) -> str\n\
\n\
Create a new string object from the given encoded string.\n\
encoding defaults to the current default string encoding.\n\
errors can be 'strict', 'replace' or 'ignore' and defaults to 'strict'.");

static PyObject *unicode_iter(PyObject *seq);

PyTypeObject PyUnicode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "str",              /* tp_name */
    sizeof(PyUnicodeObject),        /* tp_size */
    0,                  /* tp_itemsize */
    /* Slots */
    (destructor)unicode_dealloc,    /* tp_dealloc */
    0,                  /* tp_print */
    0,                  /* tp_getattr */
    0,                  /* tp_setattr */
    0,                  /* tp_reserved */
    unicode_repr,           /* tp_repr */
    &unicode_as_number,         /* tp_as_number */
    &unicode_as_sequence,       /* tp_as_sequence */
    &unicode_as_mapping,        /* tp_as_mapping */
    (hashfunc) unicode_hash,        /* tp_hash*/
    0,                  /* tp_call*/
    (reprfunc) unicode_str,     /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                  /* tp_setattro */
    0,                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    Py_TPFLAGS_UNICODE_SUBCLASS,    /* tp_flags */
    unicode_doc,            /* tp_doc */
    0,                  /* tp_traverse */
    0,                  /* tp_clear */
    PyUnicode_RichCompare,      /* tp_richcompare */
    0,                  /* tp_weaklistoffset */
    unicode_iter,           /* tp_iter */
    0,                  /* tp_iternext */
    unicode_methods,            /* tp_methods */
    0,                  /* tp_members */
    0,                  /* tp_getset */
    &PyBaseObject_Type,         /* tp_base */
    0,                  /* tp_dict */
    0,                  /* tp_descr_get */
    0,                  /* tp_descr_set */
    0,                  /* tp_dictoffset */
    0,                  /* tp_init */
    0,                  /* tp_alloc */
    unicode_new,            /* tp_new */
    PyObject_Del,           /* tp_free */
};

/* Initialize the Unicode implementation */

void _PyUnicode_Init(void)
{
    int i;

    /* XXX - move this array to unicodectype.c ? */
    Py_UCS2 linebreak[] = {
        0x000A, /* LINE FEED */
        0x000D, /* CARRIAGE RETURN */
        0x001C, /* FILE SEPARATOR */
        0x001D, /* GROUP SEPARATOR */
        0x001E, /* RECORD SEPARATOR */
        0x0085, /* NEXT LINE */
        0x2028, /* LINE SEPARATOR */
        0x2029, /* PARAGRAPH SEPARATOR */
    };

    /* Init the implementation */
    unicode_empty = PyUnicode_New(0, 0);
    if (!unicode_empty)
        Py_FatalError("Can't create empty string");

    for (i = 0; i < 256; i++)
        unicode_latin1[i] = NULL;
    if (PyType_Ready(&PyUnicode_Type) < 0)
        Py_FatalError("Can't initialize 'unicode'");

    /* initialize the linebreak bloom filter */
    bloom_linebreak = make_bloom_mask(
        PyUnicode_2BYTE_KIND, linebreak,
        Py_ARRAY_LENGTH(linebreak));

    PyType_Ready(&EncodingMapType);
}

/* Finalize the Unicode implementation */

int
PyUnicode_ClearFreeList(void)
{
    return 0;
}

void
_PyUnicode_Fini(void)
{
    int i;

    Py_XDECREF(unicode_empty);
    unicode_empty = NULL;

    for (i = 0; i < 256; i++) {
        if (unicode_latin1[i]) {
            Py_DECREF(unicode_latin1[i]);
            unicode_latin1[i] = NULL;
        }
    }
    (void)PyUnicode_ClearFreeList();
}

void
PyUnicode_InternInPlace(PyObject **p)
{
    register PyUnicodeObject *s = (PyUnicodeObject *)(*p);
    PyObject *t;
#ifdef Py_DEBUG
    assert(s != NULL);
    assert(_PyUnicode_CHECK(s));
#else
    if (s == NULL || !PyUnicode_Check(s))
        return;
#endif
    /* If it's a subclass, we don't really know what putting
       it in the interned dict might do. */
    if (!PyUnicode_CheckExact(s))
        return;
    if (PyUnicode_CHECK_INTERNED(s))
        return;
    if (_PyUnicode_READY_REPLACE(p)) {
        assert(0 && "PyUnicode_READY fail in PyUnicode_InternInPlace");
        return;
    }
    s = (PyUnicodeObject *)(*p);
    if (interned == NULL) {
        interned = PyDict_New();
        if (interned == NULL) {
            PyErr_Clear(); /* Don't leave an exception */
            return;
        }
    }
    /* It might be that the GetItem call fails even
       though the key is present in the dictionary,
       namely when this happens during a stack overflow. */
    Py_ALLOW_RECURSION
        t = PyDict_GetItem(interned, (PyObject *)s);
    Py_END_ALLOW_RECURSION

        if (t) {
            Py_INCREF(t);
            Py_DECREF(*p);
            *p = t;
            return;
        }

    PyThreadState_GET()->recursion_critical = 1;
    if (PyDict_SetItem(interned, (PyObject *)s, (PyObject *)s) < 0) {
        PyErr_Clear();
        PyThreadState_GET()->recursion_critical = 0;
        return;
    }
    PyThreadState_GET()->recursion_critical = 0;
    /* The two references in interned are not counted by refcnt.
       The deallocator will take care of this */
    Py_REFCNT(s) -= 2;
    _PyUnicode_STATE(s).interned = SSTATE_INTERNED_MORTAL;
}

void
PyUnicode_InternImmortal(PyObject **p)
{
    PyUnicodeObject *u = (PyUnicodeObject *)*p;

    PyUnicode_InternInPlace(p);
    if (PyUnicode_CHECK_INTERNED(*p) != SSTATE_INTERNED_IMMORTAL) {
        _PyUnicode_STATE(u).interned = SSTATE_INTERNED_IMMORTAL;
        Py_INCREF(*p);
    }
}

PyObject *
PyUnicode_InternFromString(const char *cp)
{
    PyObject *s = PyUnicode_FromString(cp);
    if (s == NULL)
        return NULL;
    PyUnicode_InternInPlace(&s);
    return s;
}

void
_Py_ReleaseInternedUnicodeStrings(void)
{
    PyObject *keys;
    PyUnicodeObject *s;
    Py_ssize_t i, n;
    Py_ssize_t immortal_size = 0, mortal_size = 0;

    if (interned == NULL || !PyDict_Check(interned))
        return;
    keys = PyDict_Keys(interned);
    if (keys == NULL || !PyList_Check(keys)) {
        PyErr_Clear();
        return;
    }

    /* Since _Py_ReleaseInternedUnicodeStrings() is intended to help a leak
       detector, interned unicode strings are not forcibly deallocated;
       rather, we give them their stolen references back, and then clear
       and DECREF the interned dict. */

    n = PyList_GET_SIZE(keys);
    fprintf(stderr, "releasing %" PY_FORMAT_SIZE_T "d interned strings\n",
            n);
    for (i = 0; i < n; i++) {
        s = (PyUnicodeObject *) PyList_GET_ITEM(keys, i);
        if (PyUnicode_READY(s) == -1)
            fprintf(stderr, "could not ready string\n");
        switch (PyUnicode_CHECK_INTERNED(s)) {
        case SSTATE_NOT_INTERNED:
            /* XXX Shouldn't happen */
            break;
        case SSTATE_INTERNED_IMMORTAL:
            Py_REFCNT(s) += 1;
            immortal_size += PyUnicode_GET_LENGTH(s);
            break;
        case SSTATE_INTERNED_MORTAL:
            Py_REFCNT(s) += 2;
            mortal_size += PyUnicode_GET_LENGTH(s);
            break;
        default:
            Py_FatalError("Inconsistent interned string state.");
        }
        _PyUnicode_STATE(s).interned = SSTATE_NOT_INTERNED;
    }
    fprintf(stderr, "total size of all interned strings: "
            "%" PY_FORMAT_SIZE_T "d/%" PY_FORMAT_SIZE_T "d "
            "mortal/immortal\n", mortal_size, immortal_size);
    Py_DECREF(keys);
    PyDict_Clear(interned);
    Py_DECREF(interned);
    interned = NULL;
}


/********************* Unicode Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyUnicodeObject *it_seq; /* Set to NULL when iterator is exhausted */
} unicodeiterobject;

static void
unicodeiter_dealloc(unicodeiterobject *it)
{
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
unicodeiter_traverse(unicodeiterobject *it, visitproc visit, void *arg)
{
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
unicodeiter_next(unicodeiterobject *it)
{
    PyUnicodeObject *seq;
    PyObject *item;

    assert(it != NULL);
    seq = it->it_seq;
    if (seq == NULL)
        return NULL;
    assert(_PyUnicode_CHECK(seq));

    if (it->it_index < PyUnicode_GET_LENGTH(seq)) {
        int kind = PyUnicode_KIND(seq);
        void *data = PyUnicode_DATA(seq);
        Py_UCS4 chr = PyUnicode_READ(kind, data, it->it_index);
        item = PyUnicode_FromOrdinal(chr);
        if (item != NULL)
            ++it->it_index;
        return item;
    }

    Py_DECREF(seq);
    it->it_seq = NULL;
    return NULL;
}

static PyObject *
unicodeiter_len(unicodeiterobject *it)
{
    Py_ssize_t len = 0;
    if (it->it_seq)
        len = PyUnicode_GET_SIZE(it->it_seq) - it->it_index;
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef unicodeiter_methods[] = {
    {"__length_hint__", (PyCFunction)unicodeiter_len, METH_NOARGS,
     length_hint_doc},
    {NULL,      NULL}       /* sentinel */
};

PyTypeObject PyUnicodeIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "str_iterator",         /* tp_name */
    sizeof(unicodeiterobject),      /* tp_basicsize */
    0,                  /* tp_itemsize */
    /* methods */
    (destructor)unicodeiter_dealloc,    /* tp_dealloc */
    0,                  /* tp_print */
    0,                  /* tp_getattr */
    0,                  /* tp_setattr */
    0,                  /* tp_reserved */
    0,                  /* tp_repr */
    0,                  /* tp_as_number */
    0,                  /* tp_as_sequence */
    0,                  /* tp_as_mapping */
    0,                  /* tp_hash */
    0,                  /* tp_call */
    0,                  /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                  /* tp_setattro */
    0,                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                  /* tp_doc */
    (traverseproc)unicodeiter_traverse, /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    0,                  /* tp_weaklistoffset */
    PyObject_SelfIter,          /* tp_iter */
    (iternextfunc)unicodeiter_next,     /* tp_iternext */
    unicodeiter_methods,            /* tp_methods */
    0,
};

static PyObject *
unicode_iter(PyObject *seq)
{
    unicodeiterobject *it;

    if (!PyUnicode_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (PyUnicode_READY(seq) == -1)
        return NULL;
    it = PyObject_GC_New(unicodeiterobject, &PyUnicodeIter_Type);
    if (it == NULL)
        return NULL;
    it->it_index = 0;
    Py_INCREF(seq);
    it->it_seq = (PyUnicodeObject *)seq;
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

#define UNIOP(x) Py_UNICODE_##x
#define UNIOP_t Py_UNICODE
#include "uniops.h"
#undef UNIOP
#undef UNIOP_t
#define UNIOP(x) Py_UCS4_##x
#define UNIOP_t Py_UCS4
#include "uniops.h"
#undef UNIOP
#undef UNIOP_t

Py_UNICODE*
PyUnicode_AsUnicodeCopy(PyObject *object)
{
    PyUnicodeObject *unicode = (PyUnicodeObject *)object;
    Py_UNICODE *copy;
    Py_ssize_t size;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    /* Ensure we won't overflow the size. */
    if (PyUnicode_GET_SIZE(unicode) > ((PY_SSIZE_T_MAX / sizeof(Py_UNICODE)) - 1)) {
        PyErr_NoMemory();
        return NULL;
    }
    size = PyUnicode_GET_SIZE(unicode) + 1; /* copy the nul character */
    size *= sizeof(Py_UNICODE);
    copy = PyMem_Malloc(size);
    if (copy == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(copy, PyUnicode_AS_UNICODE(unicode), size);
    return copy;
}

/* A _string module, to export formatter_parser and formatter_field_name_split
   to the string.Formatter class implemented in Python. */

static PyMethodDef _string_methods[] = {
    {"formatter_field_name_split", (PyCFunction) formatter_field_name_split,
     METH_O, PyDoc_STR("split the argument as a field name")},
    {"formatter_parser", (PyCFunction) formatter_parser,
     METH_O, PyDoc_STR("parse the argument as a format string")},
    {NULL, NULL}
};

static struct PyModuleDef _string_module = {
    PyModuleDef_HEAD_INIT,
    "_string",
    PyDoc_STR("string helper module"),
    0,
    _string_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__string(void)
{
    return PyModule_Create(&_string_module);
}


#ifdef __cplusplus
}
#endif
