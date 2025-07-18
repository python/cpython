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
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_bytes_methods.h" // _Py_bytes_lower()
#include "pycore_bytesobject.h"   // _PyBytes_Repeat()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_codecs.h"        // _PyCodec_Lookup()
#include "pycore_critical_section.h" // Py_*_CRITICAL_SECTION_SEQUENCE_FAST
#include "pycore_format.h"        // F_LJUST
#include "pycore_freelist.h"      // _Py_FREELIST_FREE(), _Py_FREELIST_POP()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // PyInterpreterState.fs_codec
#include "pycore_long.h"          // _PyLong_FormatWriter()
#include "pycore_object.h"        // _PyObject_GC_TRACK(), _Py_FatalRefcountError()
#include "pycore_pathconfig.h"    // _Py_DumpPathConfig()
#include "pycore_pyerrors.h"      // _PyUnicodeTranslateError_Create()
#include "pycore_pyhash.h"        // _Py_HashSecret_t
#include "pycore_pylifecycle.h"   // _Py_SetFileSystemEncoding()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_template.h"      // _PyTemplate_Concat()
#include "pycore_tuple.h"         // _PyTuple_FromArray()
#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI
#include "pycore_unicodeobject.h" // struct _Py_unicode_state
#include "pycore_unicodeobject_generated.h"  // _PyUnicode_InitStaticStrings()

#include "stringlib/eq.h"         // unicode_eq()
#include <stddef.h>               // ptrdiff_t

#ifdef MS_WINDOWS
#include <windows.h>
#endif

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
#  include "pycore_fileutils.h"   // _Py_LocaleUsesNonUnicodeWchar()
#endif

/* Uncomment to display statistics on interned strings at exit
   in _PyUnicode_ClearInterned(). */
/* #define INTERNED_STATS 1 */


/*[clinic input]
class str "PyObject *" "&PyUnicode_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4884c934de622cf6]*/

/*[python input]
class Py_UCS4_converter(CConverter):
    type = 'Py_UCS4'
    converter = 'convert_uc'

    def converter_init(self):
        if self.default is not unspecified:
            self.c_default = ascii(self.default)
            if len(self.c_default) > 4 or self.c_default[0] != "'":
                self.c_default = hex(ord(self.default))

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=88f5dd06cd8e7a61]*/

/* --- Globals ------------------------------------------------------------

NOTE: In the interpreter's initialization phase, some globals are currently
      initialized dynamically as needed. In the process Unicode objects may
      be created before the Unicode type is ready.

*/

// Maximum code point of Unicode 6.0: 0x10ffff (1,114,111).
// The value must be the same in fileutils.c.
#define MAX_UNICODE 0x10ffff

#ifdef Py_DEBUG
#  define _PyUnicode_CHECK(op) _PyUnicode_CheckConsistency(op, 0)
#else
#  define _PyUnicode_CHECK(op) PyUnicode_Check(op)
#endif

#ifdef Py_GIL_DISABLED
#  define LOCK_INTERNED(interp) PyMutex_Lock(&_Py_INTERP_CACHED_OBJECT(interp, interned_mutex))
#  define UNLOCK_INTERNED(interp) PyMutex_Unlock(&_Py_INTERP_CACHED_OBJECT(interp, interned_mutex))
#else
#  define LOCK_INTERNED(interp)
#  define UNLOCK_INTERNED(interp)
#endif

static inline char* _PyUnicode_UTF8(PyObject *op)
{
    return FT_ATOMIC_LOAD_PTR_ACQUIRE(_PyCompactUnicodeObject_CAST(op)->utf8);
}

static inline char* PyUnicode_UTF8(PyObject *op)
{
    assert(_PyUnicode_CHECK(op));
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
    assert(_PyUnicode_CHECK(op));
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

#define _PyUnicode_LENGTH(op)                           \
    (_PyASCIIObject_CAST(op)->length)
#define _PyUnicode_STATE(op)                            \
    (_PyASCIIObject_CAST(op)->state)
#define _PyUnicode_HASH(op)                             \
    (_PyASCIIObject_CAST(op)->hash)

#define PyUnicode_HASH PyUnstable_Unicode_GET_CACHED_HASH

static inline void PyUnicode_SET_HASH(PyObject *op, Py_hash_t hash)
{
    FT_ATOMIC_STORE_SSIZE_RELAXED(_PyASCIIObject_CAST(op)->hash, hash);
}

#define _PyUnicode_DATA_ANY(op)                         \
    (_PyUnicodeObject_CAST(op)->data.any)

static inline int _PyUnicode_SHARE_UTF8(PyObject *op)
{
    assert(_PyUnicode_CHECK(op));
    assert(!PyUnicode_IS_COMPACT_ASCII(op));
    return (_PyUnicode_UTF8(op) == PyUnicode_DATA(op));
}

/* true if the Unicode object has an allocated UTF-8 memory block
   (not shared with other data) */
static inline int _PyUnicode_HAS_UTF8_MEMORY(PyObject *op)
{
    return (!PyUnicode_IS_COMPACT_ASCII(op)
            && _PyUnicode_UTF8(op) != NULL
            && _PyUnicode_UTF8(op) != PyUnicode_DATA(op));
}


/* Generic helper macro to convert characters of different types.
   from_type and to_type have to be valid type names, begin and end
   are pointers to the source characters which should be of type
   "from_type *".  to is a pointer of type "to_type *" and points to the
   buffer where the result characters are written to. */
#define _PyUnicode_CONVERT_BYTES(from_type, to_type, begin, end, to) \
    do {                                                \
        to_type *_to = (to_type *)(to);                 \
        const from_type *_iter = (const from_type *)(begin);\
        const from_type *_end = (const from_type *)(end);\
        Py_ssize_t n = (_end) - (_iter);                \
        const from_type *_unrolled_end =                \
            _iter + _Py_SIZE_ROUND_DOWN(n, 4);          \
        while (_iter < (_unrolled_end)) {               \
            _to[0] = (to_type) _iter[0];                \
            _to[1] = (to_type) _iter[1];                \
            _to[2] = (to_type) _iter[2];                \
            _to[3] = (to_type) _iter[3];                \
            _iter += 4; _to += 4;                       \
        }                                               \
        while (_iter < (_end))                          \
            *_to++ = (to_type) *_iter++;                \
    } while (0)

#define LATIN1 _Py_LATIN1_CHR

#ifdef MS_WINDOWS
   /* On Windows, overallocate by 50% is the best factor */
#  define OVERALLOCATE_FACTOR 2
#else
   /* On Linux, overallocate by 25% is the best factor */
#  define OVERALLOCATE_FACTOR 4
#endif

/* Forward declaration */
static inline int
_PyUnicodeWriter_WriteCharInline(_PyUnicodeWriter *writer, Py_UCS4 ch);
static inline void
_PyUnicodeWriter_InitWithBuffer(_PyUnicodeWriter *writer, PyObject *buffer);
static PyObject *
unicode_encode_utf8(PyObject *unicode, _Py_error_handler error_handler,
                    const char *errors);
static PyObject *
unicode_decode_utf8(const char *s, Py_ssize_t size,
                    _Py_error_handler error_handler, const char *errors,
                    Py_ssize_t *consumed);
static int
unicode_decode_utf8_writer(_PyUnicodeWriter *writer,
                           const char *s, Py_ssize_t size,
                           _Py_error_handler error_handler, const char *errors,
                           Py_ssize_t *consumed);
#ifdef Py_DEBUG
static inline int unicode_is_finalizing(void);
static int unicode_is_singleton(PyObject *unicode);
#endif


// Return a reference to the immortal empty string singleton.
static inline PyObject* unicode_get_empty(void)
{
    _Py_DECLARE_STR(empty, "");
    return &_Py_STR(empty);
}

/* This dictionary holds per-interpreter interned strings.
 * See InternalDocs/string_interning.md for details.
 */
static inline PyObject *get_interned_dict(PyInterpreterState *interp)
{
    return _Py_INTERP_CACHED_OBJECT(interp, interned_strings);
}

/* This hashtable holds statically allocated interned strings.
 * See InternalDocs/string_interning.md for details.
 */
#define INTERNED_STRINGS _PyRuntime.cached_objects.interned_strings

/* Get number of all interned strings for the current interpreter. */
Py_ssize_t
_PyUnicode_InternedSize(void)
{
    PyObject *dict = get_interned_dict(_PyInterpreterState_GET());
    return _Py_hashtable_len(INTERNED_STRINGS) + PyDict_GET_SIZE(dict);
}

/* Get number of immortal interned strings for the current interpreter. */
Py_ssize_t
_PyUnicode_InternedSize_Immortal(void)
{
    PyObject *dict = get_interned_dict(_PyInterpreterState_GET());
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    Py_ssize_t count = 0;

    // It's tempting to keep a count and avoid a loop here. But, this function
    // is intended for refleak tests. It spends extra work to report the true
    // value, to help detect bugs in optimizations.

    while (PyDict_Next(dict, &pos, &key, &value)) {
        assert(PyUnicode_CHECK_INTERNED(key) != SSTATE_INTERNED_IMMORTAL_STATIC);
        if (PyUnicode_CHECK_INTERNED(key) == SSTATE_INTERNED_IMMORTAL) {
           count++;
       }
    }
    return _Py_hashtable_len(INTERNED_STRINGS) + count;
}

static Py_hash_t unicode_hash(PyObject *);

static Py_uhash_t
hashtable_unicode_hash(const void *key)
{
    return unicode_hash((PyObject *)key);
}

static int
hashtable_unicode_compare(const void *key1, const void *key2)
{
    PyObject *obj1 = (PyObject *)key1;
    PyObject *obj2 = (PyObject *)key2;
    if (obj1 != NULL && obj2 != NULL) {
        return unicode_eq(obj1, obj2);
    }
    else {
        return obj1 == obj2;
    }
}

/* Return true if this interpreter should share the main interpreter's
   intern_dict.  That's important for interpreters which load basic
   single-phase init extension modules (m_size == -1).  There could be interned
   immortal strings that are shared between interpreters, due to the
   PyDict_Update(mdict, m_copy) call in import_find_extension().

   It's not safe to deallocate those strings until all interpreters that
   potentially use them are freed.  By storing them in the main interpreter, we
   ensure they get freed after all other interpreters are freed.
*/
static bool
has_shared_intern_dict(PyInterpreterState *interp)
{
    PyInterpreterState *main_interp = _PyInterpreterState_Main();
    return interp != main_interp  && interp->feature_flags & Py_RTFLAGS_USE_MAIN_OBMALLOC;
}

static int
init_interned_dict(PyInterpreterState *interp)
{
    assert(get_interned_dict(interp) == NULL);
    PyObject *interned;
    if (has_shared_intern_dict(interp)) {
        interned = get_interned_dict(_PyInterpreterState_Main());
        Py_INCREF(interned);
    }
    else {
        interned = PyDict_New();
        if (interned == NULL) {
            return -1;
        }
    }
    _Py_INTERP_CACHED_OBJECT(interp, interned_strings) = interned;
    return 0;
}

static void
clear_interned_dict(PyInterpreterState *interp)
{
    PyObject *interned = get_interned_dict(interp);
    if (interned != NULL) {
        if (!has_shared_intern_dict(interp)) {
            // only clear if the dict belongs to this interpreter
            PyDict_Clear(interned);
        }
        Py_DECREF(interned);
        _Py_INTERP_CACHED_OBJECT(interp, interned_strings) = NULL;
    }
}

static PyStatus
init_global_interned_strings(PyInterpreterState *interp)
{
    assert(INTERNED_STRINGS == NULL);
    _Py_hashtable_allocator_t hashtable_alloc = {PyMem_RawMalloc, PyMem_RawFree};

    INTERNED_STRINGS = _Py_hashtable_new_full(
        hashtable_unicode_hash,
        hashtable_unicode_compare,
        // Objects stored here are immortal and statically allocated,
        // so we don't need key_destroy_func & value_destroy_func:
        NULL,
        NULL,
        &hashtable_alloc
    );
    if (INTERNED_STRINGS == NULL) {
        PyErr_Clear();
        return _PyStatus_ERR("failed to create global interned dict");
    }

    /* Intern statically allocated string identifiers, deepfreeze strings,
        * and one-byte latin-1 strings.
        * This must be done before any module initialization so that statically
        * allocated string identifiers are used instead of heap allocated strings.
        * Deepfreeze uses the interned identifiers if present to save space
        * else generates them and they are interned to speed up dict lookups.
    */
    _PyUnicode_InitStaticStrings(interp);

    for (int i = 0; i < 256; i++) {
        PyObject *s = LATIN1(i);
        _PyUnicode_InternStatic(interp, &s);
        assert(s == LATIN1(i));
    }
#ifdef Py_DEBUG
    assert(_PyUnicode_CheckConsistency(&_Py_STR(empty), 1));

    for (int i = 0; i < 256; i++) {
        assert(_PyUnicode_CheckConsistency(LATIN1(i), 1));
    }
#endif
    return _PyStatus_OK();
}

static void clear_global_interned_strings(void)
{
    if (INTERNED_STRINGS != NULL) {
        _Py_hashtable_destroy(INTERNED_STRINGS);
        INTERNED_STRINGS = NULL;
    }
}

#define _Py_RETURN_UNICODE_EMPTY()   \
    do {                             \
        return unicode_get_empty();  \
    } while (0)

static inline void
unicode_fill(int kind, void *data, Py_UCS4 value,
             Py_ssize_t start, Py_ssize_t length)
{
    assert(0 <= start);
    switch (kind) {
    case PyUnicode_1BYTE_KIND: {
        assert(value <= 0xff);
        Py_UCS1 ch = (unsigned char)value;
        Py_UCS1 *to = (Py_UCS1 *)data + start;
        memset(to, ch, length);
        break;
    }
    case PyUnicode_2BYTE_KIND: {
        assert(value <= 0xffff);
        Py_UCS2 ch = (Py_UCS2)value;
        Py_UCS2 *to = (Py_UCS2 *)data + start;
        const Py_UCS2 *end = to + length;
        for (; to < end; ++to) *to = ch;
        break;
    }
    case PyUnicode_4BYTE_KIND: {
        assert(value <= MAX_UNICODE);
        Py_UCS4 ch = value;
        Py_UCS4 * to = (Py_UCS4 *)data + start;
        const Py_UCS4 *end = to + length;
        for (; to < end; ++to) *to = ch;
        break;
    }
    default: Py_UNREACHABLE();
    }
}


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
static PyObject* get_latin1_char(unsigned char ch);
static int unicode_modifiable(PyObject *unicode);


static PyObject *
_PyUnicode_FromUCS1(const Py_UCS1 *s, Py_ssize_t size);
static PyObject *
_PyUnicode_FromUCS2(const Py_UCS2 *s, Py_ssize_t size);
static PyObject *
_PyUnicode_FromUCS4(const Py_UCS4 *s, Py_ssize_t size);

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

/* Same for linebreaks */
static const unsigned char ascii_linebreak[] = {
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

static int convert_uc(PyObject *obj, void *addr);

struct encoding_map;
#include "clinic/unicodeobject.c.h"

_Py_error_handler
_Py_GetErrorHandler(const char *errors)
{
    if (errors == NULL || strcmp(errors, "strict") == 0) {
        return _Py_ERROR_STRICT;
    }
    if (strcmp(errors, "surrogateescape") == 0) {
        return _Py_ERROR_SURROGATEESCAPE;
    }
    if (strcmp(errors, "replace") == 0) {
        return _Py_ERROR_REPLACE;
    }
    if (strcmp(errors, "ignore") == 0) {
        return _Py_ERROR_IGNORE;
    }
    if (strcmp(errors, "backslashreplace") == 0) {
        return _Py_ERROR_BACKSLASHREPLACE;
    }
    if (strcmp(errors, "surrogatepass") == 0) {
        return _Py_ERROR_SURROGATEPASS;
    }
    if (strcmp(errors, "xmlcharrefreplace") == 0) {
        return _Py_ERROR_XMLCHARREFREPLACE;
    }
    return _Py_ERROR_OTHER;
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


int
_PyUnicode_CheckConsistency(PyObject *op, int check_content)
{
#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG(op, Py_STRINGIFY(expr)); } } while (0)

    assert(op != NULL);
    CHECK(PyUnicode_Check(op));

    PyASCIIObject *ascii = _PyASCIIObject_CAST(op);
    int kind = ascii->state.kind;

    if (ascii->state.ascii == 1 && ascii->state.compact == 1) {
        CHECK(kind == PyUnicode_1BYTE_KIND);
    }
    else {
        PyCompactUnicodeObject *compact = _PyCompactUnicodeObject_CAST(op);
        void *data;

        if (ascii->state.compact == 1) {
            data = compact + 1;
            CHECK(kind == PyUnicode_1BYTE_KIND
                                 || kind == PyUnicode_2BYTE_KIND
                                 || kind == PyUnicode_4BYTE_KIND);
            CHECK(ascii->state.ascii == 0);
            CHECK(_PyUnicode_UTF8(op) != data);
        }
        else {
            PyUnicodeObject *unicode = _PyUnicodeObject_CAST(op);

            data = unicode->data.any;
            CHECK(kind == PyUnicode_1BYTE_KIND
                     || kind == PyUnicode_2BYTE_KIND
                     || kind == PyUnicode_4BYTE_KIND);
            CHECK(ascii->state.compact == 0);
            CHECK(data != NULL);
            if (ascii->state.ascii) {
                CHECK(_PyUnicode_UTF8(op) == data);
                CHECK(compact->utf8_length == ascii->length);
            }
            else {
                CHECK(_PyUnicode_UTF8(op) != data);
            }
        }
#ifndef Py_GIL_DISABLED
        if (_PyUnicode_UTF8(op) == NULL)
            CHECK(compact->utf8_length == 0);
#endif
    }

    /* check that the best kind is used: O(n) operation */
    if (check_content) {
        Py_ssize_t i;
        Py_UCS4 maxchar = 0;
        const void *data;
        Py_UCS4 ch;

        data = PyUnicode_DATA(ascii);
        for (i=0; i < ascii->length; i++)
        {
            ch = PyUnicode_READ(kind, data, i);
            if (ch > maxchar)
                maxchar = ch;
        }
        if (kind == PyUnicode_1BYTE_KIND) {
            if (ascii->state.ascii == 0) {
                CHECK(maxchar >= 128);
                CHECK(maxchar <= 255);
            }
            else
                CHECK(maxchar < 128);
        }
        else if (kind == PyUnicode_2BYTE_KIND) {
            CHECK(maxchar >= 0x100);
            CHECK(maxchar <= 0xFFFF);
        }
        else {
            CHECK(maxchar >= 0x10000);
            CHECK(maxchar <= MAX_UNICODE);
        }
        CHECK(PyUnicode_READ(kind, data, ascii->length) == 0);
    }

    /* Check interning state */
#ifdef Py_DEBUG
    // Note that we do not check `_Py_IsImmortal(op)`, since stable ABI
    // extensions can make immortal strings mortal (but with a high enough
    // refcount).
    // The other way is extremely unlikely (worth a potential failed assertion
    // in a debug build), so we do check `!_Py_IsImmortal(op)`.
    switch (PyUnicode_CHECK_INTERNED(op)) {
        case SSTATE_NOT_INTERNED:
            if (ascii->state.statically_allocated) {
                // This state is for two exceptions:
                // - strings are currently checked before they're interned
                // - the 256 one-latin1-character strings
                //   are static but use SSTATE_NOT_INTERNED
            }
            else {
                CHECK(!_Py_IsImmortal(op));
            }
            break;
        case SSTATE_INTERNED_MORTAL:
            CHECK(!ascii->state.statically_allocated);
            CHECK(!_Py_IsImmortal(op));
            break;
        case SSTATE_INTERNED_IMMORTAL:
            CHECK(!ascii->state.statically_allocated);
            break;
        case SSTATE_INTERNED_IMMORTAL_STATIC:
            CHECK(ascii->state.statically_allocated);
            break;
        default:
            Py_UNREACHABLE();
    }
#endif

    return 1;

#undef CHECK
}

static PyObject*
unicode_result(PyObject *unicode)
{
    assert(_PyUnicode_CHECK(unicode));

    Py_ssize_t length = PyUnicode_GET_LENGTH(unicode);
    if (length == 0) {
        PyObject *empty = unicode_get_empty();
        if (unicode != empty) {
            Py_DECREF(unicode);
        }
        return empty;
    }

    if (length == 1) {
        int kind = PyUnicode_KIND(unicode);
        if (kind == PyUnicode_1BYTE_KIND) {
            const Py_UCS1 *data = PyUnicode_1BYTE_DATA(unicode);
            Py_UCS1 ch = data[0];
            PyObject *latin1_char = LATIN1(ch);
            if (unicode != latin1_char) {
                Py_DECREF(unicode);
            }
            return latin1_char;
        }
    }

    assert(_PyUnicode_CheckConsistency(unicode, 1));
    return unicode;
}

static PyObject*
unicode_result_unchanged(PyObject *unicode)
{
    if (PyUnicode_CheckExact(unicode)) {
        return Py_NewRef(unicode);
    }
    else
        /* Subtype -- return genuine unicode string with the same value. */
        return _PyUnicode_Copy(unicode);
}

/* Implementation of the "backslashreplace" error handler for 8-bit encodings:
   ASCII, Latin1, UTF-8, etc. */
static char*
backslashreplace(_PyBytesWriter *writer, char *str,
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

    str = _PyBytesWriter_Prepare(writer, str, size);
    if (str == NULL)
        return NULL;

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
xmlcharrefreplace(_PyBytesWriter *writer, char *str,
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

    str = _PyBytesWriter_Prepare(writer, str, size);
    if (str == NULL)
        return NULL;

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

/* --- Bloom Filters ----------------------------------------------------- */

/* stuff to implement simple "bloom filters" for Unicode characters.
   to keep things simple, we use a single bitmask, using the least 5
   bits from each unicode characters as the bit index. */

/* the linebreak mask is set up by _PyUnicode_Init() below */

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

static BLOOM_MASK bloom_linebreak = ~(BLOOM_MASK)0;

#define BLOOM(mask, ch)     ((mask &  (1UL << ((ch) & (BLOOM_WIDTH - 1)))))

#define BLOOM_LINEBREAK(ch)                                             \
    ((ch) < 128U ? ascii_linebreak[(ch)] :                              \
     (BLOOM(bloom_linebreak, (ch)) && Py_UNICODE_ISLINEBREAK(ch)))

static inline BLOOM_MASK
make_bloom_mask(int kind, const void* ptr, Py_ssize_t len)
{
#define BLOOM_UPDATE(TYPE, MASK, PTR, LEN)             \
    do {                                               \
        TYPE *data = (TYPE *)PTR;                      \
        TYPE *end = data + LEN;                        \
        Py_UCS4 ch;                                    \
        for (; data != end; data++) {                  \
            ch = *data;                                \
            MASK |= (1UL << (ch & (BLOOM_WIDTH - 1))); \
        }                                              \
        break;                                         \
    } while (0)

    /* calculate simple bloom-style bitmask for a given unicode string */

    BLOOM_MASK mask;

    mask = 0;
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        BLOOM_UPDATE(Py_UCS1, mask, ptr, len);
        break;
    case PyUnicode_2BYTE_KIND:
        BLOOM_UPDATE(Py_UCS2, mask, ptr, len);
        break;
    case PyUnicode_4BYTE_KIND:
        BLOOM_UPDATE(Py_UCS4, mask, ptr, len);
        break;
    default:
        Py_UNREACHABLE();
    }
    return mask;

#undef BLOOM_UPDATE
}

static int
ensure_unicode(PyObject *obj)
{
    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
                     "must be str, not %.100s",
                     Py_TYPE(obj)->tp_name);
        return -1;
    }
    return 0;
}

/* Compilation of templated routines */

#define STRINGLIB_GET_EMPTY() unicode_get_empty()

#include "stringlib/asciilib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs1lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/replace.h"
#include "stringlib/repr.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs2lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/replace.h"
#include "stringlib/repr.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#include "stringlib/ucs4lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/replace.h"
#include "stringlib/repr.h"
#include "stringlib/find_max_char.h"
#include "stringlib/undef.h"

#undef STRINGLIB_GET_EMPTY

/* --- Unicode Object ----------------------------------------------------- */

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

#ifdef Py_DEBUG
/* Fill the data of a Unicode string with invalid characters to detect bugs
   earlier.

   _PyUnicode_CheckConsistency(str, 1) detects invalid characters, at least for
   ASCII and UCS-4 strings. U+00FF is invalid in ASCII and U+FFFFFFFF is an
   invalid character in Unicode 6.0. */
static void
unicode_fill_invalid(PyObject *unicode, Py_ssize_t old_length)
{
    int kind = PyUnicode_KIND(unicode);
    Py_UCS1 *data = PyUnicode_1BYTE_DATA(unicode);
    Py_ssize_t length = _PyUnicode_LENGTH(unicode);
    if (length <= old_length)
        return;
    memset(data + old_length * kind, 0xff, (length - old_length) * kind);
}
#endif

static PyObject*
resize_copy(PyObject *unicode, Py_ssize_t length)
{
    Py_ssize_t copy_length;
    PyObject *copy;

    copy = PyUnicode_New(length, PyUnicode_MAX_CHAR_VALUE(unicode));
    if (copy == NULL)
        return NULL;

    copy_length = Py_MIN(length, PyUnicode_GET_LENGTH(unicode));
    _PyUnicode_FastCopyCharacters(copy, 0, unicode, 0, copy_length);
    return copy;
}

static PyObject*
resize_compact(PyObject *unicode, Py_ssize_t length)
{
    Py_ssize_t char_size;
    Py_ssize_t struct_size;
    Py_ssize_t new_size;
    PyObject *new_unicode;
#ifdef Py_DEBUG
    Py_ssize_t old_length = _PyUnicode_LENGTH(unicode);
#endif

    if (!unicode_modifiable(unicode)) {
        PyObject *copy = resize_copy(unicode, length);
        if (copy == NULL) {
            return NULL;
        }
        Py_DECREF(unicode);
        return copy;
    }
    assert(PyUnicode_IS_COMPACT(unicode));

    char_size = PyUnicode_KIND(unicode);
    if (PyUnicode_IS_ASCII(unicode))
        struct_size = sizeof(PyASCIIObject);
    else
        struct_size = sizeof(PyCompactUnicodeObject);

    if (length > ((PY_SSIZE_T_MAX - struct_size) / char_size - 1)) {
        PyErr_NoMemory();
        return NULL;
    }
    new_size = (struct_size + (length + 1) * char_size);

    if (_PyUnicode_HAS_UTF8_MEMORY(unicode)) {
        PyMem_Free(_PyUnicode_UTF8(unicode));
        PyUnicode_SET_UTF8_LENGTH(unicode, 0);
        PyUnicode_SET_UTF8(unicode, NULL);
    }
#ifdef Py_TRACE_REFS
    _Py_ForgetReference(unicode);
#endif
    _PyReftracerTrack(unicode, PyRefTracer_DESTROY);

    new_unicode = (PyObject *)PyObject_Realloc(unicode, new_size);
    if (new_unicode == NULL) {
        _Py_NewReferenceNoTotal(unicode);
        PyErr_NoMemory();
        return NULL;
    }
    unicode = new_unicode;
    _Py_NewReferenceNoTotal(unicode);

    _PyUnicode_LENGTH(unicode) = length;
#ifdef Py_DEBUG
    unicode_fill_invalid(unicode, old_length);
#endif
    PyUnicode_WRITE(PyUnicode_KIND(unicode), PyUnicode_DATA(unicode),
                    length, 0);
    assert(_PyUnicode_CheckConsistency(unicode, 0));
    return unicode;
}

static int
resize_inplace(PyObject *unicode, Py_ssize_t length)
{
    assert(!PyUnicode_IS_COMPACT(unicode));
    assert(Py_REFCNT(unicode) == 1);

    Py_ssize_t new_size;
    Py_ssize_t char_size;
    int share_utf8;
    void *data;
#ifdef Py_DEBUG
    Py_ssize_t old_length = _PyUnicode_LENGTH(unicode);
#endif

    data = _PyUnicode_DATA_ANY(unicode);
    char_size = PyUnicode_KIND(unicode);
    share_utf8 = _PyUnicode_SHARE_UTF8(unicode);

    if (length > (PY_SSIZE_T_MAX / char_size - 1)) {
        PyErr_NoMemory();
        return -1;
    }
    new_size = (length + 1) * char_size;

    if (!share_utf8 && _PyUnicode_HAS_UTF8_MEMORY(unicode))
    {
        PyMem_Free(_PyUnicode_UTF8(unicode));
        PyUnicode_SET_UTF8_LENGTH(unicode, 0);
        PyUnicode_SET_UTF8(unicode, NULL);
    }

    data = (PyObject *)PyObject_Realloc(data, new_size);
    if (data == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    _PyUnicode_DATA_ANY(unicode) = data;
    if (share_utf8) {
        PyUnicode_SET_UTF8_LENGTH(unicode, length);
        PyUnicode_SET_UTF8(unicode, data);
    }
    _PyUnicode_LENGTH(unicode) = length;
    PyUnicode_WRITE(PyUnicode_KIND(unicode), data, length, 0);
#ifdef Py_DEBUG
    unicode_fill_invalid(unicode, old_length);
#endif

    /* check for integer overflow */
    if (length > PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(wchar_t) - 1) {
        PyErr_NoMemory();
        return -1;
    }
    assert(_PyUnicode_CheckConsistency(unicode, 0));
    return 0;
}

static const char*
unicode_kind_name(PyObject *unicode)
{
    /* don't check consistency: unicode_kind_name() is called from
       _PyUnicode_Dump() */
    if (!PyUnicode_IS_COMPACT(unicode))
    {
        switch (PyUnicode_KIND(unicode))
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
    switch (PyUnicode_KIND(unicode)) {
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
/* Functions wrapping macros for use in debugger */
const char *_PyUnicode_utf8(void *unicode_raw){
    PyObject *unicode = _PyObject_CAST(unicode_raw);
    return PyUnicode_UTF8(unicode);
}

const void *_PyUnicode_compact_data(void *unicode_raw) {
    PyObject *unicode = _PyObject_CAST(unicode_raw);
    return _PyUnicode_COMPACT_DATA(unicode);
}
const void *_PyUnicode_data(void *unicode_raw) {
    PyObject *unicode = _PyObject_CAST(unicode_raw);
    printf("obj %p\n", (void*)unicode);
    printf("compact %d\n", PyUnicode_IS_COMPACT(unicode));
    printf("compact ascii %d\n", PyUnicode_IS_COMPACT_ASCII(unicode));
    printf("ascii op %p\n", (void*)(_PyASCIIObject_CAST(unicode) + 1));
    printf("compact op %p\n", (void*)(_PyCompactUnicodeObject_CAST(unicode) + 1));
    printf("compact data %p\n", _PyUnicode_COMPACT_DATA(unicode));
    return PyUnicode_DATA(unicode);
}

void
_PyUnicode_Dump(PyObject *op)
{
    PyASCIIObject *ascii = _PyASCIIObject_CAST(op);
    PyCompactUnicodeObject *compact = _PyCompactUnicodeObject_CAST(op);
    PyUnicodeObject *unicode = _PyUnicodeObject_CAST(op);
    const void *data;

    if (ascii->state.compact)
    {
        if (ascii->state.ascii)
            data = (ascii + 1);
        else
            data = (compact + 1);
    }
    else
        data = unicode->data.any;
    printf("%s: len=%zu, ", unicode_kind_name(op), ascii->length);

    if (!ascii->state.ascii) {
        printf("utf8=%p (%zu)", (void *)compact->utf8, compact->utf8_length);
    }
    printf(", data=%p\n", data);
}
#endif


PyObject *
PyUnicode_New(Py_ssize_t size, Py_UCS4 maxchar)
{
    /* Optimization for empty strings */
    if (size == 0) {
        return unicode_get_empty();
    }

    PyObject *obj;
    PyCompactUnicodeObject *unicode;
    void *data;
    int kind;
    int is_ascii;
    Py_ssize_t char_size;
    Py_ssize_t struct_size;

    is_ascii = 0;
    struct_size = sizeof(PyCompactUnicodeObject);
    if (maxchar < 128) {
        kind = PyUnicode_1BYTE_KIND;
        char_size = 1;
        is_ascii = 1;
        struct_size = sizeof(PyASCIIObject);
    }
    else if (maxchar < 256) {
        kind = PyUnicode_1BYTE_KIND;
        char_size = 1;
    }
    else if (maxchar < 65536) {
        kind = PyUnicode_2BYTE_KIND;
        char_size = 2;
    }
    else {
        if (maxchar > MAX_UNICODE) {
            PyErr_SetString(PyExc_SystemError,
                            "invalid maximum character passed to PyUnicode_New");
            return NULL;
        }
        kind = PyUnicode_4BYTE_KIND;
        char_size = 4;
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
    obj = (PyObject *) PyObject_Malloc(struct_size + (size + 1) * char_size);
    if (obj == NULL) {
        return PyErr_NoMemory();
    }
    _PyObject_Init(obj, &PyUnicode_Type);

    unicode = (PyCompactUnicodeObject *)obj;
    if (is_ascii)
        data = ((PyASCIIObject*)obj) + 1;
    else
        data = unicode + 1;
    _PyUnicode_LENGTH(unicode) = size;
    _PyUnicode_HASH(unicode) = -1;
    _PyUnicode_STATE(unicode).interned = 0;
    _PyUnicode_STATE(unicode).kind = kind;
    _PyUnicode_STATE(unicode).compact = 1;
    _PyUnicode_STATE(unicode).ascii = is_ascii;
    _PyUnicode_STATE(unicode).statically_allocated = 0;
    if (is_ascii) {
        ((char*)data)[size] = 0;
    }
    else if (kind == PyUnicode_1BYTE_KIND) {
        ((char*)data)[size] = 0;
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
    }
    else {
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
        if (kind == PyUnicode_2BYTE_KIND)
            ((Py_UCS2*)data)[size] = 0;
        else /* kind == PyUnicode_4BYTE_KIND */
            ((Py_UCS4*)data)[size] = 0;
    }
#ifdef Py_DEBUG
    unicode_fill_invalid((PyObject*)unicode, 0);
#endif
    assert(_PyUnicode_CheckConsistency((PyObject*)unicode, 0));
    return obj;
}

static int
unicode_check_modifiable(PyObject *unicode)
{
    if (!unicode_modifiable(unicode)) {
        PyErr_SetString(PyExc_SystemError,
                        "Cannot modify a string currently used");
        return -1;
    }
    return 0;
}

static int
_copy_characters(PyObject *to, Py_ssize_t to_start,
                 PyObject *from, Py_ssize_t from_start,
                 Py_ssize_t how_many, int check_maxchar)
{
    int from_kind, to_kind;
    const void *from_data;
    void *to_data;

    assert(0 <= how_many);
    assert(0 <= from_start);
    assert(0 <= to_start);
    assert(PyUnicode_Check(from));
    assert(from_start + how_many <= PyUnicode_GET_LENGTH(from));

    assert(to == NULL || PyUnicode_Check(to));

    if (how_many == 0) {
        return 0;
    }

    assert(to != NULL);
    assert(to_start + how_many <= PyUnicode_GET_LENGTH(to));

    from_kind = PyUnicode_KIND(from);
    from_data = PyUnicode_DATA(from);
    to_kind = PyUnicode_KIND(to);
    to_data = PyUnicode_DATA(to);

#ifdef Py_DEBUG
    if (!check_maxchar
        && PyUnicode_MAX_CHAR_VALUE(from) > PyUnicode_MAX_CHAR_VALUE(to))
    {
        Py_UCS4 to_maxchar = PyUnicode_MAX_CHAR_VALUE(to);
        Py_UCS4 ch;
        Py_ssize_t i;
        for (i=0; i < how_many; i++) {
            ch = PyUnicode_READ(from_kind, from_data, from_start + i);
            assert(ch <= to_maxchar);
        }
    }
#endif

    if (from_kind == to_kind) {
        if (check_maxchar
            && !PyUnicode_IS_ASCII(from) && PyUnicode_IS_ASCII(to))
        {
            /* Writing Latin-1 characters into an ASCII string requires to
               check that all written characters are pure ASCII */
            Py_UCS4 max_char;
            max_char = ucs1lib_find_max_char(from_data,
                                             (const Py_UCS1*)from_data + how_many);
            if (max_char >= 128)
                return -1;
        }
        memcpy((char*)to_data + to_kind * to_start,
                  (const char*)from_data + from_kind * from_start,
                  to_kind * how_many);
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
        assert (PyUnicode_MAX_CHAR_VALUE(from) > PyUnicode_MAX_CHAR_VALUE(to));

        if (!check_maxchar) {
            if (from_kind == PyUnicode_2BYTE_KIND
                && to_kind == PyUnicode_1BYTE_KIND)
            {
                _PyUnicode_CONVERT_BYTES(
                    Py_UCS2, Py_UCS1,
                    PyUnicode_2BYTE_DATA(from) + from_start,
                    PyUnicode_2BYTE_DATA(from) + from_start + how_many,
                    PyUnicode_1BYTE_DATA(to) + to_start
                    );
            }
            else if (from_kind == PyUnicode_4BYTE_KIND
                     && to_kind == PyUnicode_1BYTE_KIND)
            {
                _PyUnicode_CONVERT_BYTES(
                    Py_UCS4, Py_UCS1,
                    PyUnicode_4BYTE_DATA(from) + from_start,
                    PyUnicode_4BYTE_DATA(from) + from_start + how_many,
                    PyUnicode_1BYTE_DATA(to) + to_start
                    );
            }
            else if (from_kind == PyUnicode_4BYTE_KIND
                     && to_kind == PyUnicode_2BYTE_KIND)
            {
                _PyUnicode_CONVERT_BYTES(
                    Py_UCS4, Py_UCS2,
                    PyUnicode_4BYTE_DATA(from) + from_start,
                    PyUnicode_4BYTE_DATA(from) + from_start + how_many,
                    PyUnicode_2BYTE_DATA(to) + to_start
                    );
            }
            else {
                Py_UNREACHABLE();
            }
        }
        else {
            const Py_UCS4 to_maxchar = PyUnicode_MAX_CHAR_VALUE(to);
            Py_UCS4 ch;
            Py_ssize_t i;

            for (i=0; i < how_many; i++) {
                ch = PyUnicode_READ(from_kind, from_data, from_start + i);
                if (ch > to_maxchar)
                    return -1;
                PyUnicode_WRITE(to_kind, to_data, to_start + i, ch);
            }
        }
    }
    return 0;
}

void
_PyUnicode_FastCopyCharacters(
    PyObject *to, Py_ssize_t to_start,
    PyObject *from, Py_ssize_t from_start, Py_ssize_t how_many)
{
    (void)_copy_characters(to, to_start, from, from_start, how_many, 0);
}

Py_ssize_t
PyUnicode_CopyCharacters(PyObject *to, Py_ssize_t to_start,
                         PyObject *from, Py_ssize_t from_start,
                         Py_ssize_t how_many)
{
    int err;

    if (!PyUnicode_Check(from) || !PyUnicode_Check(to)) {
        PyErr_BadInternalCall();
        return -1;
    }

    if ((size_t)from_start > (size_t)PyUnicode_GET_LENGTH(from)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if ((size_t)to_start > (size_t)PyUnicode_GET_LENGTH(to)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if (how_many < 0) {
        PyErr_SetString(PyExc_SystemError, "how_many cannot be negative");
        return -1;
    }
    how_many = Py_MIN(PyUnicode_GET_LENGTH(from)-from_start, how_many);
    if (to_start + how_many > PyUnicode_GET_LENGTH(to)) {
        PyErr_Format(PyExc_SystemError,
                     "Cannot write %zi characters at %zi "
                     "in a string of %zi characters",
                     how_many, to_start, PyUnicode_GET_LENGTH(to));
        return -1;
    }

    if (how_many == 0)
        return 0;

    if (unicode_check_modifiable(to))
        return -1;

    err = _copy_characters(to, to_start, from, from_start, how_many, 1);
    if (err) {
        PyErr_Format(PyExc_SystemError,
                     "Cannot copy %s characters "
                     "into a string of %s characters",
                     unicode_kind_name(from),
                     unicode_kind_name(to));
        return -1;
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
            if (*maxchar > MAX_UNICODE) {
                PyErr_Format(PyExc_ValueError,
                             "character U+%x is not in range [U+0000; U+%x]",
                             ch, MAX_UNICODE);
                return -1;
            }
        }
    }
    return 0;
}

static void
unicode_dealloc(PyObject *unicode)
{
#ifdef Py_DEBUG
    if (!unicode_is_finalizing() && unicode_is_singleton(unicode)) {
        _Py_FatalRefcountError("deallocating an Unicode singleton");
    }
#endif
    if (_PyUnicode_STATE(unicode).statically_allocated) {
        /* This should never get called, but we also don't want to SEGV if
        * we accidentally decref an immortal string out of existence. Since
        * the string is an immortal object, just re-set the reference count.
        */
#ifdef Py_DEBUG
        Py_UNREACHABLE();
#endif
        _Py_SetImmortal(unicode);
        return;
    }
    switch (_PyUnicode_STATE(unicode).interned) {
        case SSTATE_NOT_INTERNED:
            break;
        case SSTATE_INTERNED_MORTAL:
            /* Remove the object from the intern dict.
             * Before doing so, we set the refcount to 2: the key and value
             * in the interned_dict.
             */
            assert(Py_REFCNT(unicode) == 0);
            Py_SET_REFCNT(unicode, 2);
#ifdef Py_REF_DEBUG
            /* let's be pedantic with the ref total */
            _Py_IncRefTotal(_PyThreadState_GET());
            _Py_IncRefTotal(_PyThreadState_GET());
#endif
            PyInterpreterState *interp = _PyInterpreterState_GET();
            PyObject *interned = get_interned_dict(interp);
            assert(interned != NULL);
            PyObject *popped;
            int r = PyDict_Pop(interned, unicode, &popped);
            if (r == -1) {
                PyErr_FormatUnraisable("Exception ignored while "
                                       "removing an interned string %R",
                                       unicode);
                // We don't know what happened to the string. It's probably
                // best to leak it:
                // - if it was popped, there are no more references to it
                //   so it can't cause trouble (except wasted memory)
                // - if it wasn't popped, it'll remain interned
                _Py_SetImmortal(unicode);
                _PyUnicode_STATE(unicode).interned = SSTATE_INTERNED_IMMORTAL;
                return;
            }
            if (r == 0) {
                // The interned string was not found in the interned_dict.
#ifdef Py_DEBUG
                Py_UNREACHABLE();
#endif
                _Py_SetImmortal(unicode);
                return;
            }
            // Successfully popped.
            assert(popped == unicode);
            // Only our `popped` reference should be left; remove it too.
            assert(Py_REFCNT(unicode) == 1);
            Py_SET_REFCNT(unicode, 0);
#ifdef Py_REF_DEBUG
            /* let's be pedantic with the ref total */
            _Py_DecRefTotal(_PyThreadState_GET());
#endif
            break;
        default:
            // As with `statically_allocated` above.
#ifdef Py_REF_DEBUG
            Py_UNREACHABLE();
#endif
            _Py_SetImmortal(unicode);
            return;
    }
    if (_PyUnicode_HAS_UTF8_MEMORY(unicode)) {
        PyMem_Free(_PyUnicode_UTF8(unicode));
    }
    if (!PyUnicode_IS_COMPACT(unicode) && _PyUnicode_DATA_ANY(unicode)) {
        PyMem_Free(_PyUnicode_DATA_ANY(unicode));
    }

    Py_TYPE(unicode)->tp_free(unicode);
}

#ifdef Py_DEBUG
static int
unicode_is_singleton(PyObject *unicode)
{
    if (unicode == &_Py_STR(empty)) {
        return 1;
    }

    PyASCIIObject *ascii = _PyASCIIObject_CAST(unicode);
    if (ascii->length == 1) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(unicode, 0);
        if (ch < 256 && LATIN1(ch) == unicode) {
            return 1;
        }
    }
    return 0;
}
#endif

static int
unicode_modifiable(PyObject *unicode)
{
    assert(_PyUnicode_CHECK(unicode));
    if (!_PyObject_IsUniquelyReferenced(unicode))
        return 0;
    if (PyUnicode_HASH(unicode) != -1)
        return 0;
    if (PyUnicode_CHECK_INTERNED(unicode))
        return 0;
    if (!PyUnicode_CheckExact(unicode))
        return 0;
#ifdef Py_DEBUG
    /* singleton refcount is greater than 1 */
    assert(!unicode_is_singleton(unicode));
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

    old_length = PyUnicode_GET_LENGTH(unicode);
    if (old_length == length)
        return 0;

    if (length == 0) {
        PyObject *empty = unicode_get_empty();
        Py_SETREF(*p_unicode, empty);
        return 0;
    }

    if (!unicode_modifiable(unicode)) {
        PyObject *copy = resize_copy(unicode, length);
        if (copy == NULL)
            return -1;
        Py_SETREF(*p_unicode, copy);
        return 0;
    }

    if (PyUnicode_IS_COMPACT(unicode)) {
        PyObject *new_unicode = resize_compact(unicode, length);
        if (new_unicode == NULL)
            return -1;
        *p_unicode = new_unicode;
        return 0;
    }
    return resize_inplace(unicode, length);
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
    if (unicode == NULL || !PyUnicode_Check(unicode) || length < 0)
    {
        PyErr_BadInternalCall();
        return -1;
    }
    return unicode_resize(p_unicode, length);
}

/* Copy an ASCII or latin1 char* string into a Python Unicode string.

   WARNING: The function doesn't copy the terminating null character and
   doesn't check the maximum character (may write a latin1 character in an
   ASCII string). */
static void
unicode_write_cstr(PyObject *unicode, Py_ssize_t index,
                   const char *str, Py_ssize_t len)
{
    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    const char *end = str + len;

    assert(index + len <= PyUnicode_GET_LENGTH(unicode));
    switch (kind) {
    case PyUnicode_1BYTE_KIND: {
#ifdef Py_DEBUG
        if (PyUnicode_IS_ASCII(unicode)) {
            Py_UCS4 maxchar = ucs1lib_find_max_char(
                (const Py_UCS1*)str,
                (const Py_UCS1*)str + len);
            assert(maxchar < 128);
        }
#endif
        memcpy((char *) data + index, str, len);
        break;
    }
    case PyUnicode_2BYTE_KIND: {
        Py_UCS2 *start = (Py_UCS2 *)data + index;
        Py_UCS2 *ucs2 = start;

        for (; str < end; ++ucs2, ++str)
            *ucs2 = (Py_UCS2)*str;

        assert((ucs2 - start) <= PyUnicode_GET_LENGTH(unicode));
        break;
    }
    case PyUnicode_4BYTE_KIND: {
        Py_UCS4 *start = (Py_UCS4 *)data + index;
        Py_UCS4 *ucs4 = start;

        for (; str < end; ++ucs4, ++str)
            *ucs4 = (Py_UCS4)*str;

        assert((ucs4 - start) <= PyUnicode_GET_LENGTH(unicode));
        break;
    }
    default:
        Py_UNREACHABLE();
    }
}

static PyObject*
get_latin1_char(Py_UCS1 ch)
{
    PyObject *o = LATIN1(ch);
    return o;
}

static PyObject*
unicode_char(Py_UCS4 ch)
{
    PyObject *unicode;

    assert(ch <= MAX_UNICODE);

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


static inline void
unicode_write_widechar(int kind, void *data,
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
    if (find_maxchar_surrogates(u, u + size,
                                &maxchar, &num_surrogates) == -1)
        return NULL;

    unicode = PyUnicode_New(size - num_surrogates, maxchar);
    if (!unicode)
        return NULL;

    unicode_write_widechar(PyUnicode_KIND(unicode), PyUnicode_DATA(unicode),
                           u, size, num_surrogates);

    return unicode_result(unicode);
}


int
PyUnicodeWriter_WriteWideChar(PyUnicodeWriter *pub_writer,
                              const wchar_t *str,
                              Py_ssize_t size)
{
    _PyUnicodeWriter *writer = (_PyUnicodeWriter *)pub_writer;

    if (size < 0) {
        size = wcslen(str);
    }

    if (size == 0) {
        return 0;
    }

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
    /* Oracle Solaris uses non-Unicode internal wchar_t form for
       non-Unicode locales and hence needs conversion to UCS-4 first. */
    if (_Py_LocaleUsesNonUnicodeWchar()) {
        wchar_t* converted = _Py_DecodeNonUnicodeWchar(str, size);
        if (!converted) {
            return -1;
        }

        int res = PyUnicodeWriter_WriteUCS4(pub_writer, converted, size);
        PyMem_Free(converted);
        return res;
    }
#endif

    Py_UCS4 maxchar = 0;
    Py_ssize_t num_surrogates;
    if (find_maxchar_surrogates(str, str + size,
                                &maxchar, &num_surrogates) == -1) {
        return -1;
    }

    if (_PyUnicodeWriter_Prepare(writer, size - num_surrogates, maxchar) < 0) {
        return -1;
    }

    int kind = writer->kind;
    void *data = (Py_UCS1*)writer->data + writer->pos * kind;
    unicode_write_widechar(kind, data, str, size, num_surrogates);

    writer->pos += size - num_surrogates;
    return 0;
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
    return unicode_get_empty();
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


static void
unicode_clear_identifiers(struct _Py_unicode_state *state)
{
    struct _Py_unicode_ids *ids = &state->ids;
    for (Py_ssize_t i=0; i < ids->size; i++) {
        Py_XDECREF(ids->array[i]);
    }
    ids->size = 0;
    PyMem_Free(ids->array);
    ids->array = NULL;
    // Don't reset _PyRuntime next_index: _Py_Identifier.id remains valid
    // after Py_Finalize().
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

static Py_UCS4
kind_maxchar_limit(int kind)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return 0x80;
    case PyUnicode_2BYTE_KIND:
        return 0x100;
    case PyUnicode_4BYTE_KIND:
        return 0x10000;
    default:
        Py_UNREACHABLE();
    }
}

static PyObject*
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

static PyObject*
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

static PyObject*
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


int
PyUnicodeWriter_WriteUCS4(PyUnicodeWriter *pub_writer,
                          Py_UCS4 *str,
                          Py_ssize_t size)
{
    _PyUnicodeWriter *writer = (_PyUnicodeWriter*)pub_writer;

    if (size < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "size must be positive");
        return -1;
    }

    if (size == 0) {
        return 0;
    }

    Py_UCS4 max_char = ucs4lib_find_max_char(str, str + size);

    if (_PyUnicodeWriter_Prepare(writer, size, max_char) < 0) {
        return -1;
    }

    int kind = writer->kind;
    void *data = (Py_UCS1*)writer->data + writer->pos * kind;
    if (kind == PyUnicode_1BYTE_KIND) {
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS1,
                                 str, str + size,
                                 data);
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS2,
                                 str, str + size,
                                 data);
    }
    else {
        memcpy(data, str, size * sizeof(Py_UCS4));
    }
    writer->pos += size;

    return 0;
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

Py_UCS4
_PyUnicode_FindMaxChar(PyObject *unicode, Py_ssize_t start, Py_ssize_t end)
{
    int kind;
    const void *startptr, *endptr;

    assert(0 <= start);
    assert(end <= PyUnicode_GET_LENGTH(unicode));
    assert(start <= end);

    if (start == 0 && end == PyUnicode_GET_LENGTH(unicode))
        return PyUnicode_MAX_CHAR_VALUE(unicode);

    if (start == end)
        return 127;

    if (PyUnicode_IS_ASCII(unicode))
        return 127;

    kind = PyUnicode_KIND(unicode);
    startptr = PyUnicode_DATA(unicode);
    endptr = (char *)startptr + end * kind;
    startptr = (char *)startptr + start * kind;
    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1lib_find_max_char(startptr, endptr);
    case PyUnicode_2BYTE_KIND:
        return ucs2lib_find_max_char(startptr, endptr);
    case PyUnicode_4BYTE_KIND:
        return ucs4lib_find_max_char(startptr, endptr);
    default:
        Py_UNREACHABLE();
    }
}

/* Ensure that a string uses the most efficient storage, if it is not the
   case: create a new string with of the right kind. Write NULL into *p_unicode
   on error. */
static void
unicode_adjust_maxchar(PyObject **p_unicode)
{
    PyObject *unicode, *copy;
    Py_UCS4 max_char;
    Py_ssize_t len;
    int kind;

    assert(p_unicode != NULL);
    unicode = *p_unicode;
    if (PyUnicode_IS_ASCII(unicode))
        return;

    len = PyUnicode_GET_LENGTH(unicode);
    kind = PyUnicode_KIND(unicode);
    if (kind == PyUnicode_1BYTE_KIND) {
        const Py_UCS1 *u = PyUnicode_1BYTE_DATA(unicode);
        max_char = ucs1lib_find_max_char(u, u + len);
        if (max_char >= 128)
            return;
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        const Py_UCS2 *u = PyUnicode_2BYTE_DATA(unicode);
        max_char = ucs2lib_find_max_char(u, u + len);
        if (max_char >= 256)
            return;
    }
    else if (kind == PyUnicode_4BYTE_KIND) {
        const Py_UCS4 *u = PyUnicode_4BYTE_DATA(unicode);
        max_char = ucs4lib_find_max_char(u, u + len);
        if (max_char >= 0x10000)
            return;
    }
    else
        Py_UNREACHABLE();

    copy = PyUnicode_New(len, max_char);
    if (copy != NULL)
        _PyUnicode_FastCopyCharacters(copy, 0, unicode, 0, len);
    Py_DECREF(unicode);
    *p_unicode = copy;
}

PyObject*
_PyUnicode_Copy(PyObject *unicode)
{
    Py_ssize_t length;
    PyObject *copy;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    length = PyUnicode_GET_LENGTH(unicode);
    copy = PyUnicode_New(length, PyUnicode_MAX_CHAR_VALUE(unicode));
    if (!copy)
        return NULL;
    assert(PyUnicode_KIND(copy) == PyUnicode_KIND(unicode));

    memcpy(PyUnicode_DATA(copy), PyUnicode_DATA(unicode),
              length * PyUnicode_KIND(unicode));
    assert(_PyUnicode_CheckConsistency(copy, 1));
    return copy;
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
        return unicode_decode_utf8_writer(writer, str, length,
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
        if (ordinal < 0 || ordinal > MAX_UNICODE) {
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
PyUnicodeWriter_Format(PyUnicodeWriter *writer, const char *format, ...)
{
    _PyUnicodeWriter *_writer = (_PyUnicodeWriter*)writer;
    Py_ssize_t old_pos = _writer->pos;

    va_list vargs;
    va_start(vargs, format);
    int res = unicode_from_format(_writer, format, vargs);
    va_end(vargs);

    if (res < 0) {
        _writer->pos = old_pos;
    }
    return res;
}

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
                assert(ch <= MAX_UNICODE);
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

#ifdef HAVE_WCHAR_H

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

PyObject *
PyUnicode_FromOrdinal(int ordinal)
{
    if (ordinal < 0 || ordinal > MAX_UNICODE) {
        PyErr_SetString(PyExc_ValueError,
                        "chr() arg not in range(0x110000)");
        return NULL;
    }

    return unicode_char((Py_UCS4)ordinal);
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

/* Normalize an encoding name: similar to encodings.normalize_encoding(), but
   also convert to lowercase. Return 1 on success, or 0 on error (encoding is
   longer than lower_len-1). */
int
_Py_normalize_encoding(const char *encoding,
                       char *lower,
                       size_t lower_len)
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
            *l++ = Py_TOLOWER(c);
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
    if (_Py_normalize_encoding(encoding, buflower, sizeof(buflower))) {
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
    if (_Py_normalize_encoding(encoding, buflower, sizeof(buflower))) {
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


static int unicode_fill_utf8(PyObject *unicode);


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

/*
PyUnicode_GetSize() has been deprecated since Python 3.3
because it returned length of Py_UNICODE.

But this function is part of stable abi, because it doesn't
include Py_UNICODE in signature and it was not excluded from
stable ABI in PEP 384.
*/
PyAPI_FUNC(Py_ssize_t)
PyUnicode_GetSize(PyObject *unicode)
{
    PyErr_SetString(PyExc_RuntimeError,
                    "PyUnicode_GetSize has been removed.");
    return -1;
}

Py_ssize_t
PyUnicode_GetLength(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return -1;
    }
    return PyUnicode_GET_LENGTH(unicode);
}

Py_UCS4
PyUnicode_ReadChar(PyObject *unicode, Py_ssize_t index)
{
    const void *data;
    int kind;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return (Py_UCS4)-1;
    }
    if (index < 0 || index >= PyUnicode_GET_LENGTH(unicode)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return (Py_UCS4)-1;
    }
    data = PyUnicode_DATA(unicode);
    kind = PyUnicode_KIND(unicode);
    return PyUnicode_READ(kind, data, index);
}

int
PyUnicode_WriteChar(PyObject *unicode, Py_ssize_t index, Py_UCS4 ch)
{
    if (!PyUnicode_Check(unicode) || !PyUnicode_IS_COMPACT(unicode)) {
        PyErr_BadArgument();
        return -1;
    }
    if (index < 0 || index >= PyUnicode_GET_LENGTH(unicode)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if (unicode_check_modifiable(unicode))
        return -1;
    if (ch > PyUnicode_MAX_CHAR_VALUE(unicode)) {
        PyErr_SetString(PyExc_ValueError, "character out of range");
        return -1;
    }
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
                      int base64SetO,
                      int base64WhiteSpace,
                      const char *errors)
{
    int kind;
    const void *data;
    Py_ssize_t len;
    PyObject *v;
    int inShift = 0;
    Py_ssize_t i;
    unsigned int base64bits = 0;
    unsigned long base64buffer = 0;
    char * out;
    const char * start;

    kind = PyUnicode_KIND(str);
    data = PyUnicode_DATA(str);
    len = PyUnicode_GET_LENGTH(str);

    if (len == 0)
        return PyBytes_FromStringAndSize(NULL, 0);

    /* It might be possible to tighten this worst case */
    if (len > PY_SSIZE_T_MAX / 8)
        return PyErr_NoMemory();
    v = PyBytes_FromStringAndSize(NULL, len * 8);
    if (v == NULL)
        return NULL;

    start = out = PyBytes_AS_STRING(v);
    for (i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);

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
        size -= pos;
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
static int
unicode_decode_utf8_writer(_PyUnicodeWriter *writer,
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
        size -= decoded;
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

    _PyBytesWriter writer;
    char *end;

    switch (kind) {
    default:
        Py_UNREACHABLE();
    case PyUnicode_1BYTE_KIND:
        /* the string cannot be ASCII, or PyUnicode_UTF8() would be set */
        assert(!PyUnicode_IS_ASCII(unicode));
        end = ucs1lib_utf8_encoder(&writer, unicode, data, size, error_handler, errors);
        break;
    case PyUnicode_2BYTE_KIND:
        end = ucs2lib_utf8_encoder(&writer, unicode, data, size, error_handler, errors);
        break;
    case PyUnicode_4BYTE_KIND:
        end = ucs4lib_utf8_encoder(&writer, unicode, data, size, error_handler, errors);
        break;
    }

    if (end == NULL) {
        _PyBytesWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyBytesWriter_Finish(&writer, end);
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

    _PyBytesWriter writer;
    char *end;

    switch (kind) {
    default:
        Py_UNREACHABLE();
    case PyUnicode_1BYTE_KIND:
        end = ucs1lib_utf8_encoder(&writer, unicode, data, size,
                                   _Py_ERROR_STRICT, NULL);
        break;
    case PyUnicode_2BYTE_KIND:
        end = ucs2lib_utf8_encoder(&writer, unicode, data, size,
                                   _Py_ERROR_STRICT, NULL);
        break;
    case PyUnicode_4BYTE_KIND:
        end = ucs4lib_utf8_encoder(&writer, unicode, data, size,
                                   _Py_ERROR_STRICT, NULL);
        break;
    }
    if (end == NULL) {
        _PyBytesWriter_Dealloc(&writer);
        return -1;
    }

    const char *start = writer.use_small_buffer ? writer.small_buffer :
                    PyBytes_AS_STRING(writer.buffer);
    Py_ssize_t len = end - start;

    char *cache = PyMem_Malloc(len + 1);
    if (cache == NULL) {
        _PyBytesWriter_Dealloc(&writer);
        PyErr_NoMemory();
        return -1;
    }
    memcpy(cache, start, len);
    cache[len] = '\0';
    PyUnicode_SET_UTF8_LENGTH(unicode, len);
    PyUnicode_SET_UTF8(unicode, cache);
    _PyBytesWriter_Dealloc(&writer);
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
    int kind;
    const void *data;
    Py_ssize_t len;
    PyObject *v;
    uint32_t *out;
#if PY_LITTLE_ENDIAN
    int native_ordering = byteorder <= 0;
#else
    int native_ordering = byteorder >= 0;
#endif
    const char *encoding;
    Py_ssize_t nsize, pos;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    PyObject *rep = NULL;

    if (!PyUnicode_Check(str)) {
        PyErr_BadArgument();
        return NULL;
    }
    kind = PyUnicode_KIND(str);
    data = PyUnicode_DATA(str);
    len = PyUnicode_GET_LENGTH(str);

    if (len > PY_SSIZE_T_MAX / 4 - (byteorder == 0))
        return PyErr_NoMemory();
    nsize = len + (byteorder == 0);
    v = PyBytes_FromStringAndSize(NULL, nsize * 4);
    if (v == NULL)
        return NULL;

    /* output buffer is 4-bytes aligned */
    assert(_Py_IS_ALIGNED(PyBytes_AS_STRING(v), 4));
    out = (uint32_t *)PyBytes_AS_STRING(v);
    if (byteorder == 0)
        *out++ = 0xFEFF;
    if (len == 0)
        goto done;

    if (byteorder == -1)
        encoding = "utf-32-le";
    else if (byteorder == 1)
        encoding = "utf-32-be";
    else
        encoding = "utf-32";

    if (kind == PyUnicode_1BYTE_KIND) {
        ucs1lib_utf32_encode((const Py_UCS1 *)data, len, &out, native_ordering);
        goto done;
    }

    pos = 0;
    while (pos < len) {
        Py_ssize_t newpos, repsize, moreunits;

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

        rep = unicode_encode_call_errorhandler(
                errors, &errorHandler,
                encoding, "surrogates not allowed",
                str, &exc, pos, pos + 1, &newpos);
        if (!rep)
            goto error;

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
            Py_ssize_t outpos = out - (uint32_t*) PyBytes_AS_STRING(v);
            if (moreunits >= (PY_SSIZE_T_MAX - PyBytes_GET_SIZE(v)) / 4) {
                /* integer overflow */
                PyErr_NoMemory();
                goto error;
            }
            if (_PyBytes_Resize(&v, PyBytes_GET_SIZE(v) + 4 * moreunits) < 0)
                goto error;
            out = (uint32_t*) PyBytes_AS_STRING(v) + outpos;
        }

        if (PyBytes_Check(rep)) {
            memcpy(out, PyBytes_AS_STRING(rep), repsize);
            out += repsize / 4;
        } else /* rep is unicode */ {
            assert(PyUnicode_KIND(rep) == PyUnicode_1BYTE_KIND);
            ucs1lib_utf32_encode(PyUnicode_1BYTE_DATA(rep), repsize,
                                 &out, native_ordering);
        }

        Py_CLEAR(rep);
    }

    /* Cut back to size actually needed. This is necessary for, for example,
       encoding of a string containing isolated surrogates and the 'ignore'
       handler is used. */
    nsize = (unsigned char*) out - (unsigned char*) PyBytes_AS_STRING(v);
    if (nsize != PyBytes_GET_SIZE(v))
      _PyBytes_Resize(&v, nsize);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
  done:
    return v;
  error:
    Py_XDECREF(rep);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_XDECREF(v);
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
    int kind;
    const void *data;
    Py_ssize_t len;
    PyObject *v;
    unsigned short *out;
    Py_ssize_t pairs;
#if PY_BIG_ENDIAN
    int native_ordering = byteorder >= 0;
#else
    int native_ordering = byteorder <= 0;
#endif
    const char *encoding;
    Py_ssize_t nsize, pos;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    PyObject *rep = NULL;

    if (!PyUnicode_Check(str)) {
        PyErr_BadArgument();
        return NULL;
    }
    kind = PyUnicode_KIND(str);
    data = PyUnicode_DATA(str);
    len = PyUnicode_GET_LENGTH(str);

    pairs = 0;
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
    nsize = len + pairs + (byteorder == 0);
    v = PyBytes_FromStringAndSize(NULL, nsize * 2);
    if (v == NULL) {
        return NULL;
    }

    /* output buffer is 2-bytes aligned */
    assert(_Py_IS_ALIGNED(PyBytes_AS_STRING(v), 2));
    out = (unsigned short *)PyBytes_AS_STRING(v);
    if (byteorder == 0) {
        *out++ = 0xFEFF;
    }
    if (len == 0) {
        goto done;
    }

    if (kind == PyUnicode_1BYTE_KIND) {
        ucs1lib_utf16_encode((const Py_UCS1 *)data, len, &out, native_ordering);
        goto done;
    }

    if (byteorder < 0) {
        encoding = "utf-16-le";
    }
    else if (byteorder > 0) {
        encoding = "utf-16-be";
    }
    else {
        encoding = "utf-16";
    }

    pos = 0;
    while (pos < len) {
        Py_ssize_t newpos, repsize, moreunits;

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

        rep = unicode_encode_call_errorhandler(
                errors, &errorHandler,
                encoding, "surrogates not allowed",
                str, &exc, pos, pos + 1, &newpos);
        if (!rep)
            goto error;

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
            Py_ssize_t outpos = out - (unsigned short*) PyBytes_AS_STRING(v);
            if (moreunits >= (PY_SSIZE_T_MAX - PyBytes_GET_SIZE(v)) / 2) {
                /* integer overflow */
                PyErr_NoMemory();
                goto error;
            }
            if (_PyBytes_Resize(&v, PyBytes_GET_SIZE(v) + 2 * moreunits) < 0)
                goto error;
            out = (unsigned short*) PyBytes_AS_STRING(v) + outpos;
        }

        if (PyBytes_Check(rep)) {
            memcpy(out, PyBytes_AS_STRING(rep), repsize);
            out += repsize / 2;
        } else /* rep is unicode */ {
            assert(PyUnicode_KIND(rep) == PyUnicode_1BYTE_KIND);
            ucs1lib_utf16_encode(PyUnicode_1BYTE_DATA(rep), repsize,
                                 &out, native_ordering);
        }

        Py_CLEAR(rep);
    }

    /* Cut back to size actually needed. This is necessary for, for example,
    encoding of a string containing isolated surrogates and the 'ignore' handler
    is used. */
    nsize = (unsigned char*) out - (unsigned char*) PyBytes_AS_STRING(v);
    if (nsize != PyBytes_GET_SIZE(v))
      _PyBytes_Resize(&v, nsize);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
  done:
    return v;
  error:
    Py_XDECREF(rep);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_XDECREF(v);
    return NULL;
#undef STORECHAR
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
    Py_ssize_t i, len;
    PyObject *repr;
    char *p;
    int kind;
    const void *data;
    Py_ssize_t expandsize;

    /* Initial allocation is based on the longest-possible character
       escape.

       For UCS1 strings it's '\xxx', 4 bytes per source character.
       For UCS2 strings it's '\uxxxx', 6 bytes per source character.
       For UCS4 strings it's '\U00xxxxxx', 10 bytes per source character.
    */

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }

    len = PyUnicode_GET_LENGTH(unicode);
    if (len == 0) {
        return PyBytes_FromStringAndSize(NULL, 0);
    }

    kind = PyUnicode_KIND(unicode);
    data = PyUnicode_DATA(unicode);
    /* 4 byte characters can take up 10 bytes, 2 byte characters can take up 6
       bytes, and 1 byte characters 4. */
    expandsize = kind * 2 + 2;
    if (len > PY_SSIZE_T_MAX / expandsize) {
        return PyErr_NoMemory();
    }
    repr = PyBytes_FromStringAndSize(NULL, expandsize * len);
    if (repr == NULL) {
        return NULL;
    }

    p = PyBytes_AS_STRING(repr);
    for (i = 0; i < len; i++) {
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

    assert(p - PyBytes_AS_STRING(repr) > 0);
    if (_PyBytes_Resize(&repr, p - PyBytes_AS_STRING(repr)) < 0) {
        return NULL;
    }
    return repr;
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
    PyObject *repr;
    char *p;
    Py_ssize_t expandsize, pos;
    int kind;
    const void *data;
    Py_ssize_t len;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    kind = PyUnicode_KIND(unicode);
    data = PyUnicode_DATA(unicode);
    len = PyUnicode_GET_LENGTH(unicode);
    if (kind == PyUnicode_1BYTE_KIND) {
        return PyBytes_FromStringAndSize(data, len);
    }

    /* 4 byte characters can take up 10 bytes, 2 byte characters can take up 6
       bytes, and 1 byte characters 4. */
    expandsize = kind * 2 + 2;

    if (len > PY_SSIZE_T_MAX / expandsize) {
        return PyErr_NoMemory();
    }
    repr = PyBytes_FromStringAndSize(NULL, expandsize * len);
    if (repr == NULL) {
        return NULL;
    }
    if (len == 0) {
        return repr;
    }

    p = PyBytes_AS_STRING(repr);
    for (pos = 0; pos < len; pos++) {
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

    assert(p > PyBytes_AS_STRING(repr));
    if (_PyBytes_Resize(&repr, p - PyBytes_AS_STRING(repr)) < 0) {
        return NULL;
    }
    return repr;
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
    /* pointer into the output */
    char *str;
    const char *encoding = (limit == 256) ? "latin-1" : "ascii";
    const char *reason = (limit == 256) ? "ordinal not in range(256)" : "ordinal not in range(128)";
    PyObject *error_handler_obj = NULL;
    PyObject *exc = NULL;
    _Py_error_handler error_handler = _Py_ERROR_UNKNOWN;
    PyObject *rep = NULL;
    /* output object */
    _PyBytesWriter writer;

    size = PyUnicode_GET_LENGTH(unicode);
    kind = PyUnicode_KIND(unicode);
    data = PyUnicode_DATA(unicode);
    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    if (size == 0)
        return PyBytes_FromStringAndSize(NULL, 0);

    _PyBytesWriter_Init(&writer);
    str = _PyBytesWriter_Alloc(&writer, size);
    if (str == NULL)
        return NULL;

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
            writer.overallocate = (collend < size);

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
                writer.min_size -= (collend - collstart);
                str = backslashreplace(&writer, str,
                                       unicode, collstart, collend);
                if (str == NULL)
                    goto onError;
                pos = collend;
                break;

            case _Py_ERROR_XMLCHARREFREPLACE:
                /* subtract preallocated bytes */
                writer.min_size -= (collend - collstart);
                str = xmlcharrefreplace(&writer, str,
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
                    writer.overallocate = 1;
                    str = _PyBytesWriter_Prepare(&writer, str,
                                                 collstart - newpos);
                    if (str == NULL)
                        goto onError;
                }
                else {
                    /* subtract preallocated bytes */
                    writer.min_size -= newpos - collstart;
                    /* Only overallocate the buffer if it's not the last write */
                    writer.overallocate = (newpos < size);
                }

                if (PyBytes_Check(rep)) {
                    /* Directly copy bytes result to output. */
                    str = _PyBytesWriter_WriteBytes(&writer, str,
                                                    PyBytes_AS_STRING(rep),
                                                    PyBytes_GET_SIZE(rep));
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
                    str = _PyBytesWriter_WriteBytes(&writer, str,
                                                    PyUnicode_DATA(rep),
                                                    PyUnicode_GET_LENGTH(rep));
                }
                if (str == NULL)
                    goto onError;

                pos = newpos;
                Py_CLEAR(rep);
            }

            /* If overallocation was disabled, ensure that it was the last
               write. Otherwise, we missed an optimization */
            assert(writer.overallocate || pos == size);
        }
    }

    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return _PyBytesWriter_Finish(&writer, str);

  onError:
    Py_XDECREF(rep);
    _PyBytesWriter_Dealloc(&writer);
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
    if (code_page == CP_UTF7)
        return "CP_UTF7";
    if (code_page == CP_UTF8)
        return "CP_UTF8";

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
encode_code_page_strict(UINT code_page, PyObject **outbytes,
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

    if (*outbytes == NULL) {
        /* Create string object */
        *outbytes = PyBytes_FromStringAndSize(NULL, outsize);
        if (*outbytes == NULL) {
            goto done;
        }
        out = PyBytes_AS_STRING(*outbytes);
    }
    else {
        /* Extend string object */
        const Py_ssize_t n = PyBytes_Size(*outbytes);
        if (outsize > PY_SSIZE_T_MAX - n) {
            PyErr_NoMemory();
            goto done;
        }
        if (_PyBytes_Resize(outbytes, n + outsize) < 0) {
            goto done;
        }
        out = PyBytes_AS_STRING(*outbytes) + n;
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
encode_code_page_errors(UINT code_page, PyObject **outbytes,
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
    Py_ssize_t newpos, newoutsize;
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

    if (*outbytes == NULL) {
        /* Create string object */
        *outbytes = PyBytes_FromStringAndSize(NULL, outsize);
        if (*outbytes == NULL)
            goto error;
        out = PyBytes_AS_STRING(*outbytes);
    }
    else {
        /* Extend string object */
        Py_ssize_t n = PyBytes_Size(*outbytes);
        if (n > PY_SSIZE_T_MAX - outsize) {
            PyErr_NoMemory();
            goto error;
        }
        if (_PyBytes_Resize(outbytes, n + outsize) < 0)
            goto error;
        out = PyBytes_AS_STRING(*outbytes) + n;
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
                Py_ssize_t offset = out - PyBytes_AS_STRING(*outbytes);
                newoutsize = PyBytes_GET_SIZE(*outbytes) + morebytes;
                if (_PyBytes_Resize(outbytes, newoutsize) < 0) {
                    Py_DECREF(rep);
                    goto error;
                }
                out = PyBytes_AS_STRING(*outbytes) + offset;
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
                Py_ssize_t offset = out - PyBytes_AS_STRING(*outbytes);
                newoutsize = PyBytes_GET_SIZE(*outbytes) + morebytes;
                if (_PyBytes_Resize(outbytes, newoutsize) < 0) {
                    Py_DECREF(rep);
                    goto error;
                }
                out = PyBytes_AS_STRING(*outbytes) + offset;
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
    outsize = out - PyBytes_AS_STRING(*outbytes);
    assert(outsize <= PyBytes_GET_SIZE(*outbytes));
    if (_PyBytes_Resize(outbytes, outsize) < 0)
        goto error;
    ret = 0;

error:
    Py_XDECREF(encoding_obj);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return ret;
}

static PyObject *
encode_code_page(int code_page,
                 PyObject *unicode,
                 const char *errors)
{
    Py_ssize_t len;
    PyObject *outbytes = NULL;
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
        return PyBytes_FromStringAndSize(NULL, 0);

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

        ret = encode_code_page_strict(code_page, &outbytes,
                                      unicode, offset, chunk_len,
                                      errors);
        if (ret == -2)
            ret = encode_code_page_errors(code_page, &outbytes,
                                          unicode, offset,
                                          chunk_len, errors);
        if (ret < 0) {
            Py_XDECREF(outbytes);
            return NULL;
        }

        offset += chunk_len;
        len -= chunk_len;
    } while (!done);

    return outbytes;
}

PyObject *
PyUnicode_EncodeCodePage(int code_page,
                         PyObject *unicode,
                         const char *errors)
{
    return encode_code_page(code_page, unicode, errors);
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
class EncodingMap "struct encoding_map *" "&EncodingMapType"
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

static PyTypeObject EncodingMapType = {
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

    _PyObject_Init(result, &EncodingMapType);
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
charmapencode_output(Py_UCS4 c, PyObject *mapping,
                     PyObject **outobj, Py_ssize_t *outpos)
{
    PyObject *rep;
    unsigned char replace;
    char *outstart;
    Py_ssize_t outsize = PyBytes_GET_SIZE(*outobj);

    if (Py_IS_TYPE(mapping, &EncodingMapType)) {
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
                if (charmapencode_resize(outobj, outpos, requiredsize)) {
                    Py_DECREF(rep);
                    return enc_EXCEPTION;
                }
            outstart = PyBytes_AS_STRING(*outobj);
            outstart[(*outpos)++] = (char)replace;
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
    PyObject *unicode, Py_ssize_t *inpos, PyObject *mapping,
    PyObject **exceptionObject,
    _Py_error_handler *error_handler, PyObject **error_handler_obj, const char *errors,
    PyObject **res, Py_ssize_t *respos)
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
        if (Py_IS_TYPE(mapping, &EncodingMapType)) {
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
            x = charmapencode_output('?', mapping, res, respos);
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
                x = charmapencode_output(*cp, mapping, res, respos);
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
        repsize = PyUnicode_GET_LENGTH(repunicode);
        data = PyUnicode_DATA(repunicode);
        kind = PyUnicode_KIND(repunicode);
        for (index = 0; index < repsize; index++) {
            Py_UCS4 repch = PyUnicode_READ(kind, data, index);
            x = charmapencode_output(repch, mapping, res, respos);
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
    /* output object */
    PyObject *res = NULL;
    /* current input position */
    Py_ssize_t inpos = 0;
    Py_ssize_t size;
    /* current output position */
    Py_ssize_t respos = 0;
    PyObject *error_handler_obj = NULL;
    PyObject *exc = NULL;
    _Py_error_handler error_handler = _Py_ERROR_UNKNOWN;
    const void *data;
    int kind;

    size = PyUnicode_GET_LENGTH(unicode);
    data = PyUnicode_DATA(unicode);
    kind = PyUnicode_KIND(unicode);

    /* Default to Latin-1 */
    if (mapping == NULL)
        return unicode_encode_ucs1(unicode, errors, 256);

    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    res = PyBytes_FromStringAndSize(NULL, size);
    if (res == NULL)
        goto onError;
    if (size == 0)
        return res;

    while (inpos<size) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, inpos);
        /* try to encode it */
        charmapencode_result x = charmapencode_output(ch, mapping, &res, &respos);
        if (x==enc_EXCEPTION) /* error */
            goto onError;
        if (x==enc_FAILED) { /* unencodable character */
            if (charmap_encoding_error(unicode, &inpos, mapping,
                                       &exc,
                                       &error_handler, &error_handler_obj, errors,
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
    Py_XDECREF(error_handler_obj);
    return res;

  onError:
    Py_XDECREF(res);
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

static PyObject *
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

/* --- Helpers ------------------------------------------------------------ */

/* helper macro to fixup start/end slice values */
#define ADJUST_INDICES(start, end, len) \
    do {                                \
        if (end > len) {                \
            end = len;                  \
        }                               \
        else if (end < 0) {             \
            end += len;                 \
            if (end < 0) {              \
                end = 0;                \
            }                           \
        }                               \
        if (start < 0) {                \
            start += len;               \
            if (start < 0) {            \
                start = 0;              \
            }                           \
        }                               \
    } while (0)

static Py_ssize_t
any_find_slice(PyObject* s1, PyObject* s2,
               Py_ssize_t start,
               Py_ssize_t end,
               int direction)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2, result;

    kind1 = PyUnicode_KIND(s1);
    kind2 = PyUnicode_KIND(s2);
    if (kind1 < kind2)
        return -1;

    len1 = PyUnicode_GET_LENGTH(s1);
    len2 = PyUnicode_GET_LENGTH(s2);
    ADJUST_INDICES(start, end, len1);
    if (end - start < len2)
        return -1;

    buf1 = PyUnicode_DATA(s1);
    buf2 = PyUnicode_DATA(s2);
    if (len2 == 1) {
        Py_UCS4 ch = PyUnicode_READ(kind2, buf2, 0);
        result = findchar((const char *)buf1 + kind1*start,
                          kind1, end - start, ch, direction);
        if (result == -1)
            return -1;
        else
            return start + result;
    }

    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            return -2;
    }

    if (direction > 0) {
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(s1) && PyUnicode_IS_ASCII(s2))
                result = asciilib_find_slice(buf1, len1, buf2, len2, start, end);
            else
                result = ucs1lib_find_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_2BYTE_KIND:
            result = ucs2lib_find_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_4BYTE_KIND:
            result = ucs4lib_find_slice(buf1, len1, buf2, len2, start, end);
            break;
        default:
            Py_UNREACHABLE();
        }
    }
    else {
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(s1) && PyUnicode_IS_ASCII(s2))
                result = asciilib_rfind_slice(buf1, len1, buf2, len2, start, end);
            else
                result = ucs1lib_rfind_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_2BYTE_KIND:
            result = ucs2lib_rfind_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_4BYTE_KIND:
            result = ucs4lib_rfind_slice(buf1, len1, buf2, len2, start, end);
            break;
        default:
            Py_UNREACHABLE();
        }
    }

    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(s2)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return result;
}

/* _PyUnicode_InsertThousandsGrouping() helper functions */
#include "stringlib/localeutil.h"

/**
 * InsertThousandsGrouping:
 * @writer: Unicode writer.
 * @n_buffer: Number of characters in @buffer.
 * @digits: Digits we're reading from. If count is non-NULL, this is unused.
 * @d_pos: Start of digits string.
 * @n_digits: The number of digits in the string, in which we want
 *            to put the grouping chars.
 * @min_width: The minimum width of the digits in the output string.
 *             Output will be zero-padded on the left to fill.
 * @grouping: see definition in localeconv().
 * @thousands_sep: see definition in localeconv().
 *
 * There are 2 modes: counting and filling. If @writer is NULL,
 *  we are in counting mode, else filling mode.
 * If counting, the required buffer size is returned.
 * If filling, we know the buffer will be large enough, so we don't
 *  need to pass in the buffer size.
 * Inserts thousand grouping characters (as defined by grouping and
 *  thousands_sep) into @writer.
 *
 * Return value: -1 on error, number of characters otherwise.
 **/
Py_ssize_t
_PyUnicode_InsertThousandsGrouping(
    _PyUnicodeWriter *writer,
    Py_ssize_t n_buffer,
    PyObject *digits,
    Py_ssize_t d_pos,
    Py_ssize_t n_digits,
    Py_ssize_t min_width,
    const char *grouping,
    PyObject *thousands_sep,
    Py_UCS4 *maxchar,
    int forward)
{
    min_width = Py_MAX(0, min_width);
    if (writer) {
        assert(digits != NULL);
        assert(maxchar == NULL);
    }
    else {
        assert(digits == NULL);
        assert(maxchar != NULL);
    }
    assert(0 <= d_pos);
    assert(0 <= n_digits);
    assert(grouping != NULL);

    Py_ssize_t count = 0;
    Py_ssize_t n_zeros;
    int loop_broken = 0;
    int use_separator = 0; /* First time through, don't append the
                              separator. They only go between
                              groups. */
    Py_ssize_t buffer_pos;
    Py_ssize_t digits_pos;
    Py_ssize_t len;
    Py_ssize_t n_chars;
    Py_ssize_t remaining = n_digits; /* Number of chars remaining to
                                        be looked at */
    /* A generator that returns all of the grouping widths, until it
       returns 0. */
    GroupGenerator groupgen;
    GroupGenerator_init(&groupgen, grouping);
    const Py_ssize_t thousands_sep_len = PyUnicode_GET_LENGTH(thousands_sep);

    /* if digits are not grouped, thousands separator
       should be an empty string */
    assert(!(grouping[0] == CHAR_MAX && thousands_sep_len != 0));

    digits_pos = d_pos + (forward ? 0 : n_digits);
    if (writer) {
        buffer_pos = writer->pos + (forward ? 0 : n_buffer);
        assert(buffer_pos <= PyUnicode_GET_LENGTH(writer->buffer));
        assert(digits_pos <= PyUnicode_GET_LENGTH(digits));
    }
    else {
        buffer_pos = forward ? 0 : n_buffer;
    }

    if (!writer) {
        *maxchar = 127;
    }

    while ((len = GroupGenerator_next(&groupgen)) > 0) {
        len = Py_MIN(len, Py_MAX(Py_MAX(remaining, min_width), 1));
        n_zeros = Py_MAX(0, len - remaining);
        n_chars = Py_MAX(0, Py_MIN(remaining, len));

        /* Use n_zero zero's and n_chars chars */

        /* Count only, don't do anything. */
        count += (use_separator ? thousands_sep_len : 0) + n_zeros + n_chars;

        /* Copy into the writer. */
        InsertThousandsGrouping_fill(writer, &buffer_pos,
                                     digits, &digits_pos,
                                     n_chars, n_zeros,
                                     use_separator ? thousands_sep : NULL,
                                     thousands_sep_len, maxchar, forward);

        /* Use a separator next time. */
        use_separator = 1;

        remaining -= n_chars;
        min_width -= len;

        if (remaining <= 0 && min_width <= 0) {
            loop_broken = 1;
            break;
        }
        min_width -= thousands_sep_len;
    }
    if (!loop_broken) {
        /* We left the loop without using a break statement. */

        len = Py_MAX(Py_MAX(remaining, min_width), 1);
        n_zeros = Py_MAX(0, len - remaining);
        n_chars = Py_MAX(0, Py_MIN(remaining, len));

        /* Use n_zero zero's and n_chars chars */
        count += (use_separator ? thousands_sep_len : 0) + n_zeros + n_chars;

        /* Copy into the writer. */
        InsertThousandsGrouping_fill(writer, &buffer_pos,
                                     digits, &digits_pos,
                                     n_chars, n_zeros,
                                     use_separator ? thousands_sep : NULL,
                                     thousands_sep_len, maxchar, forward);
    }
    return count;
}

Py_ssize_t
PyUnicode_Count(PyObject *str,
                PyObject *substr,
                Py_ssize_t start,
                Py_ssize_t end)
{
    if (ensure_unicode(str) < 0 || ensure_unicode(substr) < 0)
        return -1;

    return unicode_count_impl(str, substr, start, end);
}

Py_ssize_t
PyUnicode_Find(PyObject *str,
               PyObject *substr,
               Py_ssize_t start,
               Py_ssize_t end,
               int direction)
{
    if (ensure_unicode(str) < 0 || ensure_unicode(substr) < 0)
        return -2;

    return any_find_slice(str, substr, start, end, direction);
}

Py_ssize_t
PyUnicode_FindChar(PyObject *str, Py_UCS4 ch,
                   Py_ssize_t start, Py_ssize_t end,
                   int direction)
{
    int kind;
    Py_ssize_t len, result;
    len = PyUnicode_GET_LENGTH(str);
    ADJUST_INDICES(start, end, len);
    if (end - start < 1)
        return -1;
    kind = PyUnicode_KIND(str);
    result = findchar(PyUnicode_1BYTE_DATA(str) + kind*start,
                      kind, end-start, ch, direction);
    if (result == -1)
        return -1;
    else
        return start + result;
}

static int
tailmatch(PyObject *self,
          PyObject *substring,
          Py_ssize_t start,
          Py_ssize_t end,
          int direction)
{
    int kind_self;
    int kind_sub;
    const void *data_self;
    const void *data_sub;
    Py_ssize_t offset;
    Py_ssize_t i;
    Py_ssize_t end_sub;

    ADJUST_INDICES(start, end, PyUnicode_GET_LENGTH(self));
    end -= PyUnicode_GET_LENGTH(substring);
    if (end < start)
        return 0;

    if (PyUnicode_GET_LENGTH(substring) == 0)
        return 1;

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
                                (offset * PyUnicode_KIND(substring)),
                            data_sub,
                            PyUnicode_GET_LENGTH(substring) *
                                PyUnicode_KIND(substring));
        }
        /* otherwise we have to compare each character by first accessing it */
        else {
            /* We do not need to compare 0 and len(substring)-1 because
               the if statement above ensured already that they are equal
               when we end up here. */
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
    if (ensure_unicode(str) < 0 || ensure_unicode(substr) < 0)
        return -1;

    return tailmatch(str, substr, start, end, direction);
}

static PyObject *
ascii_upper_or_lower(PyObject *self, int lower)
{
    Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    const char *data = PyUnicode_DATA(self);
    char *resdata;
    PyObject *res;

    res = PyUnicode_New(len, 127);
    if (res == NULL)
        return NULL;
    resdata = PyUnicode_DATA(res);
    if (lower)
        _Py_bytes_lower(resdata, data, len);
    else
        _Py_bytes_upper(resdata, data, len);
    return res;
}

static Py_UCS4
handle_capital_sigma(int kind, const void *data, Py_ssize_t length, Py_ssize_t i)
{
    Py_ssize_t j;
    int final_sigma;
    Py_UCS4 c = 0;   /* initialize to prevent gcc warning */
    /* U+03A3 is in the Final_Sigma context when, it is found like this:

     \p{cased}\p{case-ignorable}*U+03A3!(\p{case-ignorable}*\p{cased})

    where ! is a negation and \p{xxx} is a character with property xxx.
    */
    for (j = i - 1; j >= 0; j--) {
        c = PyUnicode_READ(kind, data, j);
        if (!_PyUnicode_IsCaseIgnorable(c))
            break;
    }
    final_sigma = j >= 0 && _PyUnicode_IsCased(c);
    if (final_sigma) {
        for (j = i + 1; j < length; j++) {
            c = PyUnicode_READ(kind, data, j);
            if (!_PyUnicode_IsCaseIgnorable(c))
                break;
        }
        final_sigma = j == length || !_PyUnicode_IsCased(c);
    }
    return (final_sigma) ? 0x3C2 : 0x3C3;
}

static int
lower_ucs4(int kind, const void *data, Py_ssize_t length, Py_ssize_t i,
           Py_UCS4 c, Py_UCS4 *mapped)
{
    /* Obscure special case. */
    if (c == 0x3A3) {
        mapped[0] = handle_capital_sigma(kind, data, length, i);
        return 1;
    }
    return _PyUnicode_ToLowerFull(c, mapped);
}

static Py_ssize_t
do_capitalize(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res, Py_UCS4 *maxchar)
{
    Py_ssize_t i, k = 0;
    int n_res, j;
    Py_UCS4 c, mapped[3];

    c = PyUnicode_READ(kind, data, 0);
    n_res = _PyUnicode_ToTitleFull(c, mapped);
    for (j = 0; j < n_res; j++) {
        *maxchar = Py_MAX(*maxchar, mapped[j]);
        res[k++] = mapped[j];
    }
    for (i = 1; i < length; i++) {
        c = PyUnicode_READ(kind, data, i);
        n_res = lower_ucs4(kind, data, length, i, c, mapped);
        for (j = 0; j < n_res; j++) {
            *maxchar = Py_MAX(*maxchar, mapped[j]);
            res[k++] = mapped[j];
        }
    }
    return k;
}

static Py_ssize_t
do_swapcase(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res, Py_UCS4 *maxchar) {
    Py_ssize_t i, k = 0;

    for (i = 0; i < length; i++) {
        Py_UCS4 c = PyUnicode_READ(kind, data, i), mapped[3];
        int n_res, j;
        if (Py_UNICODE_ISUPPER(c)) {
            n_res = lower_ucs4(kind, data, length, i, c, mapped);
        }
        else if (Py_UNICODE_ISLOWER(c)) {
            n_res = _PyUnicode_ToUpperFull(c, mapped);
        }
        else {
            n_res = 1;
            mapped[0] = c;
        }
        for (j = 0; j < n_res; j++) {
            *maxchar = Py_MAX(*maxchar, mapped[j]);
            res[k++] = mapped[j];
        }
    }
    return k;
}

static Py_ssize_t
do_upper_or_lower(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res,
                  Py_UCS4 *maxchar, int lower)
{
    Py_ssize_t i, k = 0;

    for (i = 0; i < length; i++) {
        Py_UCS4 c = PyUnicode_READ(kind, data, i), mapped[3];
        int n_res, j;
        if (lower)
            n_res = lower_ucs4(kind, data, length, i, c, mapped);
        else
            n_res = _PyUnicode_ToUpperFull(c, mapped);
        for (j = 0; j < n_res; j++) {
            *maxchar = Py_MAX(*maxchar, mapped[j]);
            res[k++] = mapped[j];
        }
    }
    return k;
}

static Py_ssize_t
do_upper(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res, Py_UCS4 *maxchar)
{
    return do_upper_or_lower(kind, data, length, res, maxchar, 0);
}

static Py_ssize_t
do_lower(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res, Py_UCS4 *maxchar)
{
    return do_upper_or_lower(kind, data, length, res, maxchar, 1);
}

static Py_ssize_t
do_casefold(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res, Py_UCS4 *maxchar)
{
    Py_ssize_t i, k = 0;

    for (i = 0; i < length; i++) {
        Py_UCS4 c = PyUnicode_READ(kind, data, i);
        Py_UCS4 mapped[3];
        int j, n_res = _PyUnicode_ToFoldedFull(c, mapped);
        for (j = 0; j < n_res; j++) {
            *maxchar = Py_MAX(*maxchar, mapped[j]);
            res[k++] = mapped[j];
        }
    }
    return k;
}

static Py_ssize_t
do_title(int kind, const void *data, Py_ssize_t length, Py_UCS4 *res, Py_UCS4 *maxchar)
{
    Py_ssize_t i, k = 0;
    int previous_is_cased;

    previous_is_cased = 0;
    for (i = 0; i < length; i++) {
        const Py_UCS4 c = PyUnicode_READ(kind, data, i);
        Py_UCS4 mapped[3];
        int n_res, j;

        if (previous_is_cased)
            n_res = lower_ucs4(kind, data, length, i, c, mapped);
        else
            n_res = _PyUnicode_ToTitleFull(c, mapped);

        for (j = 0; j < n_res; j++) {
            *maxchar = Py_MAX(*maxchar, mapped[j]);
            res[k++] = mapped[j];
        }

        previous_is_cased = _PyUnicode_IsCased(c);
    }
    return k;
}

static PyObject *
case_operation(PyObject *self,
               Py_ssize_t (*perform)(int, const void *, Py_ssize_t, Py_UCS4 *, Py_UCS4 *))
{
    PyObject *res = NULL;
    Py_ssize_t length, newlength = 0;
    int kind, outkind;
    const void *data;
    void *outdata;
    Py_UCS4 maxchar = 0, *tmp, *tmpend;

    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    length = PyUnicode_GET_LENGTH(self);
    if ((size_t) length > PY_SSIZE_T_MAX / (3 * sizeof(Py_UCS4))) {
        PyErr_SetString(PyExc_OverflowError, "string is too long");
        return NULL;
    }
    tmp = PyMem_Malloc(sizeof(Py_UCS4) * 3 * length);
    if (tmp == NULL)
        return PyErr_NoMemory();
    newlength = perform(kind, data, length, tmp, &maxchar);
    res = PyUnicode_New(newlength, maxchar);
    if (res == NULL)
        goto leave;
    tmpend = tmp + newlength;
    outdata = PyUnicode_DATA(res);
    outkind = PyUnicode_KIND(res);
    switch (outkind) {
    case PyUnicode_1BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS1, tmp, tmpend, outdata);
        break;
    case PyUnicode_2BYTE_KIND:
        _PyUnicode_CONVERT_BYTES(Py_UCS4, Py_UCS2, tmp, tmpend, outdata);
        break;
    case PyUnicode_4BYTE_KIND:
        memcpy(outdata, tmp, sizeof(Py_UCS4) * newlength);
        break;
    default:
        Py_UNREACHABLE();
    }
  leave:
    PyMem_Free(tmp);
    return res;
}

PyObject *
PyUnicode_Join(PyObject *separator, PyObject *seq)
{
    PyObject *res;
    PyObject *fseq;
    Py_ssize_t seqlen;
    PyObject **items;

    fseq = PySequence_Fast(seq, "can only join an iterable");
    if (fseq == NULL) {
        return NULL;
    }

    Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(seq);

    items = PySequence_Fast_ITEMS(fseq);
    seqlen = PySequence_Fast_GET_SIZE(fseq);
    res = _PyUnicode_JoinArray(separator, items, seqlen);

    Py_END_CRITICAL_SECTION_SEQUENCE_FAST();

    Py_DECREF(fseq);
    return res;
}

PyObject *
_PyUnicode_JoinArray(PyObject *separator, PyObject *const *items, Py_ssize_t seqlen)
{
    PyObject *res = NULL; /* the result */
    PyObject *sep = NULL;
    Py_ssize_t seplen;
    PyObject *item;
    Py_ssize_t sz, i, res_offset;
    Py_UCS4 maxchar;
    Py_UCS4 item_maxchar;
    int use_memcpy;
    unsigned char *res_data = NULL, *sep_data = NULL;
    PyObject *last_obj;
    int kind = 0;

    /* If empty sequence, return u"". */
    if (seqlen == 0) {
        _Py_RETURN_UNICODE_EMPTY();
    }

    /* If singleton sequence with an exact Unicode, return that. */
    last_obj = NULL;
    if (seqlen == 1) {
        if (PyUnicode_CheckExact(items[0])) {
            res = items[0];
            return Py_NewRef(res);
        }
        seplen = 0;
        maxchar = 0;
    }
    else {
        /* Set up sep and seplen */
        if (separator == NULL) {
            /* fall back to a blank space separator */
            sep = PyUnicode_FromOrdinal(' ');
            if (!sep)
                goto onError;
            seplen = 1;
            maxchar = 32;
        }
        else {
            if (!PyUnicode_Check(separator)) {
                PyErr_Format(PyExc_TypeError,
                             "separator: expected str instance,"
                             " %.80s found",
                             Py_TYPE(separator)->tp_name);
                goto onError;
            }
            sep = separator;
            seplen = PyUnicode_GET_LENGTH(separator);
            maxchar = PyUnicode_MAX_CHAR_VALUE(separator);
            /* inc refcount to keep this code path symmetric with the
               above case of a blank separator */
            Py_INCREF(sep);
        }
        last_obj = sep;
    }

    /* There are at least two things to join, or else we have a subclass
     * of str in the sequence.
     * Do a pre-pass to figure out the total amount of space we'll
     * need (sz), and see whether all argument are strings.
     */
    sz = 0;
#ifdef Py_DEBUG
    use_memcpy = 0;
#else
    use_memcpy = 1;
#endif
    for (i = 0; i < seqlen; i++) {
        size_t add_sz;
        item = items[i];
        if (!PyUnicode_Check(item)) {
            PyErr_Format(PyExc_TypeError,
                         "sequence item %zd: expected str instance,"
                         " %.80s found",
                         i, Py_TYPE(item)->tp_name);
            goto onError;
        }
        add_sz = PyUnicode_GET_LENGTH(item);
        item_maxchar = PyUnicode_MAX_CHAR_VALUE(item);
        maxchar = Py_MAX(maxchar, item_maxchar);
        if (i != 0) {
            add_sz += seplen;
        }
        if (add_sz > (size_t)(PY_SSIZE_T_MAX - sz)) {
            PyErr_SetString(PyExc_OverflowError,
                            "join() result is too long for a Python string");
            goto onError;
        }
        sz += add_sz;
        if (use_memcpy && last_obj != NULL) {
            if (PyUnicode_KIND(last_obj) != PyUnicode_KIND(item))
                use_memcpy = 0;
        }
        last_obj = item;
    }

    res = PyUnicode_New(sz, maxchar);
    if (res == NULL)
        goto onError;

    /* Catenate everything. */
#ifdef Py_DEBUG
    use_memcpy = 0;
#else
    if (use_memcpy) {
        res_data = PyUnicode_1BYTE_DATA(res);
        kind = PyUnicode_KIND(res);
        if (seplen != 0)
            sep_data = PyUnicode_1BYTE_DATA(sep);
    }
#endif
    if (use_memcpy) {
        for (i = 0; i < seqlen; ++i) {
            Py_ssize_t itemlen;
            item = items[i];

            /* Copy item, and maybe the separator. */
            if (i && seplen != 0) {
                memcpy(res_data,
                          sep_data,
                          kind * seplen);
                res_data += kind * seplen;
            }

            itemlen = PyUnicode_GET_LENGTH(item);
            if (itemlen != 0) {
                memcpy(res_data,
                          PyUnicode_DATA(item),
                          kind * itemlen);
                res_data += kind * itemlen;
            }
        }
        assert(res_data == PyUnicode_1BYTE_DATA(res)
                           + kind * PyUnicode_GET_LENGTH(res));
    }
    else {
        for (i = 0, res_offset = 0; i < seqlen; ++i) {
            Py_ssize_t itemlen;
            item = items[i];

            /* Copy item, and maybe the separator. */
            if (i && seplen != 0) {
                _PyUnicode_FastCopyCharacters(res, res_offset, sep, 0, seplen);
                res_offset += seplen;
            }

            itemlen = PyUnicode_GET_LENGTH(item);
            if (itemlen != 0) {
                _PyUnicode_FastCopyCharacters(res, res_offset, item, 0, itemlen);
                res_offset += itemlen;
            }
        }
        assert(res_offset == PyUnicode_GET_LENGTH(res));
    }

    Py_XDECREF(sep);
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;

  onError:
    Py_XDECREF(sep);
    Py_XDECREF(res);
    return NULL;
}

void
_PyUnicode_FastFill(PyObject *unicode, Py_ssize_t start, Py_ssize_t length,
                    Py_UCS4 fill_char)
{
    const int kind = PyUnicode_KIND(unicode);
    void *data = PyUnicode_DATA(unicode);
    assert(unicode_modifiable(unicode));
    assert(fill_char <= PyUnicode_MAX_CHAR_VALUE(unicode));
    assert(start >= 0);
    assert(start + length <= PyUnicode_GET_LENGTH(unicode));
    unicode_fill(kind, data, fill_char, start, length);
}

Py_ssize_t
PyUnicode_Fill(PyObject *unicode, Py_ssize_t start, Py_ssize_t length,
               Py_UCS4 fill_char)
{
    Py_ssize_t maxlen;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (unicode_check_modifiable(unicode))
        return -1;

    if (start < 0) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if (fill_char > PyUnicode_MAX_CHAR_VALUE(unicode)) {
        PyErr_SetString(PyExc_ValueError,
                         "fill character is bigger than "
                         "the string maximum character");
        return -1;
    }

    maxlen = PyUnicode_GET_LENGTH(unicode) - start;
    length = Py_MIN(maxlen, length);
    if (length <= 0)
        return 0;

    _PyUnicode_FastFill(unicode, start, length, fill_char);
    return length;
}

static PyObject *
pad(PyObject *self,
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

    if (left == 0 && right == 0)
        return unicode_result_unchanged(self);

    if (left > PY_SSIZE_T_MAX - _PyUnicode_LENGTH(self) ||
        right > PY_SSIZE_T_MAX - (left + _PyUnicode_LENGTH(self))) {
        PyErr_SetString(PyExc_OverflowError, "padded string is too long");
        return NULL;
    }
    maxchar = PyUnicode_MAX_CHAR_VALUE(self);
    maxchar = Py_MAX(maxchar, fill);
    u = PyUnicode_New(left + _PyUnicode_LENGTH(self) + right, maxchar);
    if (!u)
        return NULL;

    kind = PyUnicode_KIND(u);
    data = PyUnicode_DATA(u);
    if (left)
        unicode_fill(kind, data, fill, 0, left);
    if (right)
        unicode_fill(kind, data, fill, left + _PyUnicode_LENGTH(self), right);
    _PyUnicode_FastCopyCharacters(u, left, self, 0, _PyUnicode_LENGTH(self));
    assert(_PyUnicode_CheckConsistency(u, 1));
    return u;
}

PyObject *
PyUnicode_Splitlines(PyObject *string, int keepends)
{
    PyObject *list;

    if (ensure_unicode(string) < 0)
        return NULL;

    switch (PyUnicode_KIND(string)) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(string))
            list = asciilib_splitlines(
                string, PyUnicode_1BYTE_DATA(string),
                PyUnicode_GET_LENGTH(string), keepends);
        else
            list = ucs1lib_splitlines(
                string, PyUnicode_1BYTE_DATA(string),
                PyUnicode_GET_LENGTH(string), keepends);
        break;
    case PyUnicode_2BYTE_KIND:
        list = ucs2lib_splitlines(
            string, PyUnicode_2BYTE_DATA(string),
            PyUnicode_GET_LENGTH(string), keepends);
        break;
    case PyUnicode_4BYTE_KIND:
        list = ucs4lib_splitlines(
            string, PyUnicode_4BYTE_DATA(string),
            PyUnicode_GET_LENGTH(string), keepends);
        break;
    default:
        Py_UNREACHABLE();
    }
    return list;
}

static PyObject *
split(PyObject *self,
      PyObject *substring,
      Py_ssize_t maxcount)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;
    PyObject* out;
    len1 = PyUnicode_GET_LENGTH(self);
    kind1 = PyUnicode_KIND(self);

    if (substring == NULL) {
        if (maxcount < 0) {
            maxcount = (len1 - 1) / 2 + 1;
        }
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(self))
                return asciilib_split_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
            else
                return ucs1lib_split_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
        case PyUnicode_2BYTE_KIND:
            return ucs2lib_split_whitespace(
                self,  PyUnicode_2BYTE_DATA(self),
                len1, maxcount
                );
        case PyUnicode_4BYTE_KIND:
            return ucs4lib_split_whitespace(
                self,  PyUnicode_4BYTE_DATA(self),
                len1, maxcount
                );
        default:
            Py_UNREACHABLE();
        }
    }

    kind2 = PyUnicode_KIND(substring);
    len2 = PyUnicode_GET_LENGTH(substring);
    if (maxcount < 0) {
        // if len2 == 0, it will raise ValueError.
        maxcount = len2 == 0 ? 0 : (len1 / len2) + 1;
        // handle expected overflow case: (Py_SSIZE_T_MAX / 1) + 1
        maxcount = maxcount < 0 ? len1 : maxcount;
    }
    if (kind1 < kind2 || len1 < len2) {
        out = PyList_New(1);
        if (out == NULL)
            return NULL;
        PyList_SET_ITEM(out, 0, Py_NewRef(self));
        return out;
    }
    buf1 = PyUnicode_DATA(self);
    buf2 = PyUnicode_DATA(substring);
    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(self) && PyUnicode_IS_ASCII(substring))
            out = asciilib_split(
                self,  buf1, len1, buf2, len2, maxcount);
        else
            out = ucs1lib_split(
                self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_split(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_split(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    default:
        out = NULL;
    }
    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substring)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);
    return out;
}

static PyObject *
rsplit(PyObject *self,
       PyObject *substring,
       Py_ssize_t maxcount)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;
    PyObject* out;

    len1 = PyUnicode_GET_LENGTH(self);
    kind1 = PyUnicode_KIND(self);

    if (substring == NULL) {
        if (maxcount < 0) {
            maxcount = (len1 - 1) / 2 + 1;
        }
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(self))
                return asciilib_rsplit_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
            else
                return ucs1lib_rsplit_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
        case PyUnicode_2BYTE_KIND:
            return ucs2lib_rsplit_whitespace(
                self,  PyUnicode_2BYTE_DATA(self),
                len1, maxcount
                );
        case PyUnicode_4BYTE_KIND:
            return ucs4lib_rsplit_whitespace(
                self,  PyUnicode_4BYTE_DATA(self),
                len1, maxcount
                );
        default:
            Py_UNREACHABLE();
        }
    }
    kind2 = PyUnicode_KIND(substring);
    len2 = PyUnicode_GET_LENGTH(substring);
    if (maxcount < 0) {
        // if len2 == 0, it will raise ValueError.
        maxcount = len2 == 0 ? 0 : (len1 / len2) + 1;
        // handle expected overflow case: (Py_SSIZE_T_MAX / 1) + 1
        maxcount = maxcount < 0 ? len1 : maxcount;
    }
    if (kind1 < kind2 || len1 < len2) {
        out = PyList_New(1);
        if (out == NULL)
            return NULL;
        PyList_SET_ITEM(out, 0, Py_NewRef(self));
        return out;
    }
    buf1 = PyUnicode_DATA(self);
    buf2 = PyUnicode_DATA(substring);
    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(self) && PyUnicode_IS_ASCII(substring))
            out = asciilib_rsplit(
                self,  buf1, len1, buf2, len2, maxcount);
        else
            out = ucs1lib_rsplit(
                self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_rsplit(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_rsplit(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    default:
        out = NULL;
    }
    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substring)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);
    return out;
}

static Py_ssize_t
anylib_find(int kind, PyObject *str1, const void *buf1, Py_ssize_t len1,
            PyObject *str2, const void *buf2, Py_ssize_t len2, Py_ssize_t offset)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(str1) && PyUnicode_IS_ASCII(str2))
            return asciilib_find(buf1, len1, buf2, len2, offset);
        else
            return ucs1lib_find(buf1, len1, buf2, len2, offset);
    case PyUnicode_2BYTE_KIND:
        return ucs2lib_find(buf1, len1, buf2, len2, offset);
    case PyUnicode_4BYTE_KIND:
        return ucs4lib_find(buf1, len1, buf2, len2, offset);
    }
    Py_UNREACHABLE();
}

static Py_ssize_t
anylib_count(int kind, PyObject *sstr, const void* sbuf, Py_ssize_t slen,
             PyObject *str1, const void *buf1, Py_ssize_t len1, Py_ssize_t maxcount)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1lib_count(sbuf, slen, buf1, len1, maxcount);
    case PyUnicode_2BYTE_KIND:
        return ucs2lib_count(sbuf, slen, buf1, len1, maxcount);
    case PyUnicode_4BYTE_KIND:
        return ucs4lib_count(sbuf, slen, buf1, len1, maxcount);
    }
    Py_UNREACHABLE();
}

static void
replace_1char_inplace(PyObject *u, Py_ssize_t pos,
                      Py_UCS4 u1, Py_UCS4 u2, Py_ssize_t maxcount)
{
    int kind = PyUnicode_KIND(u);
    void *data = PyUnicode_DATA(u);
    Py_ssize_t len = PyUnicode_GET_LENGTH(u);
    if (kind == PyUnicode_1BYTE_KIND) {
        ucs1lib_replace_1char_inplace((Py_UCS1 *)data + pos,
                                      (Py_UCS1 *)data + len,
                                      u1, u2, maxcount);
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        ucs2lib_replace_1char_inplace((Py_UCS2 *)data + pos,
                                      (Py_UCS2 *)data + len,
                                      u1, u2, maxcount);
    }
    else {
        assert(kind == PyUnicode_4BYTE_KIND);
        ucs4lib_replace_1char_inplace((Py_UCS4 *)data + pos,
                                      (Py_UCS4 *)data + len,
                                      u1, u2, maxcount);
    }
}

static PyObject *
replace(PyObject *self, PyObject *str1,
        PyObject *str2, Py_ssize_t maxcount)
{
    PyObject *u;
    const char *sbuf = PyUnicode_DATA(self);
    const void *buf1 = PyUnicode_DATA(str1);
    const void *buf2 = PyUnicode_DATA(str2);
    int srelease = 0, release1 = 0, release2 = 0;
    int skind = PyUnicode_KIND(self);
    int kind1 = PyUnicode_KIND(str1);
    int kind2 = PyUnicode_KIND(str2);
    Py_ssize_t slen = PyUnicode_GET_LENGTH(self);
    Py_ssize_t len1 = PyUnicode_GET_LENGTH(str1);
    Py_ssize_t len2 = PyUnicode_GET_LENGTH(str2);
    int mayshrink;
    Py_UCS4 maxchar, maxchar_str1, maxchar_str2;

    if (slen < len1)
        goto nothing;

    if (maxcount < 0)
        maxcount = PY_SSIZE_T_MAX;
    else if (maxcount == 0)
        goto nothing;

    if (str1 == str2)
        goto nothing;

    maxchar = PyUnicode_MAX_CHAR_VALUE(self);
    maxchar_str1 = PyUnicode_MAX_CHAR_VALUE(str1);
    if (maxchar < maxchar_str1)
        /* substring too wide to be present */
        goto nothing;
    maxchar_str2 = PyUnicode_MAX_CHAR_VALUE(str2);
    /* Replacing str1 with str2 may cause a maxchar reduction in the
       result string. */
    mayshrink = (maxchar_str2 < maxchar_str1) && (maxchar == maxchar_str1);
    maxchar = Py_MAX(maxchar, maxchar_str2);

    if (len1 == len2) {
        /* same length */
        if (len1 == 0)
            goto nothing;
        if (len1 == 1) {
            /* replace characters */
            Py_UCS4 u1, u2;
            Py_ssize_t pos;

            u1 = PyUnicode_READ(kind1, buf1, 0);
            pos = findchar(sbuf, skind, slen, u1, 1);
            if (pos < 0)
                goto nothing;
            u2 = PyUnicode_READ(kind2, buf2, 0);
            u = PyUnicode_New(slen, maxchar);
            if (!u)
                goto error;

            _PyUnicode_FastCopyCharacters(u, 0, self, 0, slen);
            replace_1char_inplace(u, pos, u1, u2, maxcount);
        }
        else {
            int rkind = skind;
            char *res;
            Py_ssize_t i;

            if (kind1 < rkind) {
                /* widen substring */
                buf1 = unicode_askind(kind1, buf1, len1, rkind);
                if (!buf1) goto error;
                release1 = 1;
            }
            i = anylib_find(rkind, self, sbuf, slen, str1, buf1, len1, 0);
            if (i < 0)
                goto nothing;
            if (rkind > kind2) {
                /* widen replacement */
                buf2 = unicode_askind(kind2, buf2, len2, rkind);
                if (!buf2) goto error;
                release2 = 1;
            }
            else if (rkind < kind2) {
                /* widen self and buf1 */
                rkind = kind2;
                if (release1) {
                    assert(buf1 != PyUnicode_DATA(str1));
                    PyMem_Free((void *)buf1);
                    buf1 = PyUnicode_DATA(str1);
                    release1 = 0;
                }
                sbuf = unicode_askind(skind, sbuf, slen, rkind);
                if (!sbuf) goto error;
                srelease = 1;
                buf1 = unicode_askind(kind1, buf1, len1, rkind);
                if (!buf1) goto error;
                release1 = 1;
            }
            u = PyUnicode_New(slen, maxchar);
            if (!u)
                goto error;
            assert(PyUnicode_KIND(u) == rkind);
            res = PyUnicode_DATA(u);

            memcpy(res, sbuf, rkind * slen);
            /* change everything in-place, starting with this one */
            memcpy(res + rkind * i,
                   buf2,
                   rkind * len2);
            i += len1;

            while ( --maxcount > 0) {
                i = anylib_find(rkind, self,
                                sbuf+rkind*i, slen-i,
                                str1, buf1, len1, i);
                if (i == -1)
                    break;
                memcpy(res + rkind * i,
                       buf2,
                       rkind * len2);
                i += len1;
            }
        }
    }
    else {
        Py_ssize_t n, i, j, ires;
        Py_ssize_t new_size;
        int rkind = skind;
        char *res;

        if (kind1 < rkind) {
            /* widen substring */
            buf1 = unicode_askind(kind1, buf1, len1, rkind);
            if (!buf1) goto error;
            release1 = 1;
        }
        n = anylib_count(rkind, self, sbuf, slen, str1, buf1, len1, maxcount);
        if (n == 0)
            goto nothing;
        if (kind2 < rkind) {
            /* widen replacement */
            buf2 = unicode_askind(kind2, buf2, len2, rkind);
            if (!buf2) goto error;
            release2 = 1;
        }
        else if (kind2 > rkind) {
            /* widen self and buf1 */
            rkind = kind2;
            sbuf = unicode_askind(skind, sbuf, slen, rkind);
            if (!sbuf) goto error;
            srelease = 1;
            if (release1) {
                assert(buf1 != PyUnicode_DATA(str1));
                PyMem_Free((void *)buf1);
                buf1 = PyUnicode_DATA(str1);
                release1 = 0;
            }
            buf1 = unicode_askind(kind1, buf1, len1, rkind);
            if (!buf1) goto error;
            release1 = 1;
        }
        /* new_size = PyUnicode_GET_LENGTH(self) + n * (PyUnicode_GET_LENGTH(str2) -
           PyUnicode_GET_LENGTH(str1)); */
        if (len1 < len2 && len2 - len1 > (PY_SSIZE_T_MAX - slen) / n) {
                PyErr_SetString(PyExc_OverflowError,
                                "replace string is too long");
                goto error;
        }
        new_size = slen + n * (len2 - len1);
        if (new_size == 0) {
            u = unicode_get_empty();
            goto done;
        }
        if (new_size > (PY_SSIZE_T_MAX / rkind)) {
            PyErr_SetString(PyExc_OverflowError,
                            "replace string is too long");
            goto error;
        }
        u = PyUnicode_New(new_size, maxchar);
        if (!u)
            goto error;
        assert(PyUnicode_KIND(u) == rkind);
        res = PyUnicode_DATA(u);
        ires = i = 0;
        if (len1 > 0) {
            while (n-- > 0) {
                /* look for next match */
                j = anylib_find(rkind, self,
                                sbuf + rkind * i, slen-i,
                                str1, buf1, len1, i);
                if (j == -1)
                    break;
                else if (j > i) {
                    /* copy unchanged part [i:j] */
                    memcpy(res + rkind * ires,
                           sbuf + rkind * i,
                           rkind * (j-i));
                    ires += j - i;
                }
                /* copy substitution string */
                if (len2 > 0) {
                    memcpy(res + rkind * ires,
                           buf2,
                           rkind * len2);
                    ires += len2;
                }
                i = j + len1;
            }
            if (i < slen)
                /* copy tail [i:] */
                memcpy(res + rkind * ires,
                       sbuf + rkind * i,
                       rkind * (slen-i));
        }
        else {
            /* interleave */
            while (n > 0) {
                memcpy(res + rkind * ires,
                       buf2,
                       rkind * len2);
                ires += len2;
                if (--n <= 0)
                    break;
                memcpy(res + rkind * ires,
                       sbuf + rkind * i,
                       rkind);
                ires++;
                i++;
            }
            memcpy(res + rkind * ires,
                   sbuf + rkind * i,
                   rkind * (slen-i));
        }
    }

    if (mayshrink) {
        unicode_adjust_maxchar(&u);
        if (u == NULL)
            goto error;
    }

  done:
    assert(srelease == (sbuf != PyUnicode_DATA(self)));
    assert(release1 == (buf1 != PyUnicode_DATA(str1)));
    assert(release2 == (buf2 != PyUnicode_DATA(str2)));
    if (srelease)
        PyMem_Free((void *)sbuf);
    if (release1)
        PyMem_Free((void *)buf1);
    if (release2)
        PyMem_Free((void *)buf2);
    assert(_PyUnicode_CheckConsistency(u, 1));
    return u;

  nothing:
    /* nothing to replace; return original string (when possible) */
    assert(srelease == (sbuf != PyUnicode_DATA(self)));
    assert(release1 == (buf1 != PyUnicode_DATA(str1)));
    assert(release2 == (buf2 != PyUnicode_DATA(str2)));
    if (srelease)
        PyMem_Free((void *)sbuf);
    if (release1)
        PyMem_Free((void *)buf1);
    if (release2)
        PyMem_Free((void *)buf2);
    return unicode_result_unchanged(self);

  error:
    assert(srelease == (sbuf != PyUnicode_DATA(self)));
    assert(release1 == (buf1 != PyUnicode_DATA(str1)));
    assert(release2 == (buf2 != PyUnicode_DATA(str2)));
    if (srelease)
        PyMem_Free((void *)sbuf);
    if (release1)
        PyMem_Free((void *)buf1);
    if (release2)
        PyMem_Free((void *)buf2);
    return NULL;
}

/* --- Unicode Object Methods --------------------------------------------- */

/*[clinic input]
str.title as unicode_title

Return a version of the string where each word is titlecased.

More specifically, words start with uppercased characters and all remaining
cased characters have lower case.
[clinic start generated code]*/

static PyObject *
unicode_title_impl(PyObject *self)
/*[clinic end generated code: output=c75ae03809574902 input=fa945d669b26e683]*/
{
    return case_operation(self, do_title);
}

/*[clinic input]
str.capitalize as unicode_capitalize

Return a capitalized version of the string.

More specifically, make the first character have upper case and the rest lower
case.
[clinic start generated code]*/

static PyObject *
unicode_capitalize_impl(PyObject *self)
/*[clinic end generated code: output=e49a4c333cdb7667 input=f4cbf1016938da6d]*/
{
    if (PyUnicode_GET_LENGTH(self) == 0)
        return unicode_result_unchanged(self);
    return case_operation(self, do_capitalize);
}

/*[clinic input]
str.casefold as unicode_casefold

Return a version of the string suitable for caseless comparisons.
[clinic start generated code]*/

static PyObject *
unicode_casefold_impl(PyObject *self)
/*[clinic end generated code: output=0120daf657ca40af input=384d66cc2ae30daf]*/
{
    if (PyUnicode_IS_ASCII(self))
        return ascii_upper_or_lower(self, 1);
    return case_operation(self, do_casefold);
}


/* Argument converter. Accepts a single Unicode character. */

static int
convert_uc(PyObject *obj, void *addr)
{
    Py_UCS4 *fillcharloc = (Py_UCS4 *)addr;

    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
                     "The fill character must be a unicode character, "
                     "not %.100s", Py_TYPE(obj)->tp_name);
        return 0;
    }
    if (PyUnicode_GET_LENGTH(obj) != 1) {
        PyErr_SetString(PyExc_TypeError,
                        "The fill character must be exactly one character long");
        return 0;
    }
    *fillcharloc = PyUnicode_READ_CHAR(obj, 0);
    return 1;
}

/*[clinic input]
str.center as unicode_center

    width: Py_ssize_t
    fillchar: Py_UCS4 = ' '
    /

Return a centered string of length width.

Padding is done using the specified fill character (default is a space).
[clinic start generated code]*/

static PyObject *
unicode_center_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar)
/*[clinic end generated code: output=420c8859effc7c0c input=b42b247eb26e6519]*/
{
    Py_ssize_t marg, left;

    if (PyUnicode_GET_LENGTH(self) >= width)
        return unicode_result_unchanged(self);

    marg = width - PyUnicode_GET_LENGTH(self);
    left = marg / 2 + (marg & width & 1);

    return pad(self, left, marg - left, fillchar);
}

/* This function assumes that str1 and str2 are readied by the caller. */

static int
unicode_compare(PyObject *str1, PyObject *str2)
{
#define COMPARE(TYPE1, TYPE2) \
    do { \
        TYPE1* p1 = (TYPE1 *)data1; \
        TYPE2* p2 = (TYPE2 *)data2; \
        TYPE1* end = p1 + len; \
        Py_UCS4 c1, c2; \
        for (; p1 != end; p1++, p2++) { \
            c1 = *p1; \
            c2 = *p2; \
            if (c1 != c2) \
                return (c1 < c2) ? -1 : 1; \
        } \
    } \
    while (0)

    int kind1, kind2;
    const void *data1, *data2;
    Py_ssize_t len1, len2, len;

    kind1 = PyUnicode_KIND(str1);
    kind2 = PyUnicode_KIND(str2);
    data1 = PyUnicode_DATA(str1);
    data2 = PyUnicode_DATA(str2);
    len1 = PyUnicode_GET_LENGTH(str1);
    len2 = PyUnicode_GET_LENGTH(str2);
    len = Py_MIN(len1, len2);

    switch(kind1) {
    case PyUnicode_1BYTE_KIND:
    {
        switch(kind2) {
        case PyUnicode_1BYTE_KIND:
        {
            int cmp = memcmp(data1, data2, len);
            /* normalize result of memcmp() into the range [-1; 1] */
            if (cmp < 0)
                return -1;
            if (cmp > 0)
                return 1;
            break;
        }
        case PyUnicode_2BYTE_KIND:
            COMPARE(Py_UCS1, Py_UCS2);
            break;
        case PyUnicode_4BYTE_KIND:
            COMPARE(Py_UCS1, Py_UCS4);
            break;
        default:
            Py_UNREACHABLE();
        }
        break;
    }
    case PyUnicode_2BYTE_KIND:
    {
        switch(kind2) {
        case PyUnicode_1BYTE_KIND:
            COMPARE(Py_UCS2, Py_UCS1);
            break;
        case PyUnicode_2BYTE_KIND:
        {
            COMPARE(Py_UCS2, Py_UCS2);
            break;
        }
        case PyUnicode_4BYTE_KIND:
            COMPARE(Py_UCS2, Py_UCS4);
            break;
        default:
            Py_UNREACHABLE();
        }
        break;
    }
    case PyUnicode_4BYTE_KIND:
    {
        switch(kind2) {
        case PyUnicode_1BYTE_KIND:
            COMPARE(Py_UCS4, Py_UCS1);
            break;
        case PyUnicode_2BYTE_KIND:
            COMPARE(Py_UCS4, Py_UCS2);
            break;
        case PyUnicode_4BYTE_KIND:
        {
#if defined(HAVE_WMEMCMP) && SIZEOF_WCHAR_T == 4
            int cmp = wmemcmp((wchar_t *)data1, (wchar_t *)data2, len);
            /* normalize result of wmemcmp() into the range [-1; 1] */
            if (cmp < 0)
                return -1;
            if (cmp > 0)
                return 1;
#else
            COMPARE(Py_UCS4, Py_UCS4);
#endif
            break;
        }
        default:
            Py_UNREACHABLE();
        }
        break;
    }
    default:
        Py_UNREACHABLE();
    }

    if (len1 == len2)
        return 0;
    if (len1 < len2)
        return -1;
    else
        return 1;

#undef COMPARE
}


int
_PyUnicode_Equal(PyObject *str1, PyObject *str2)
{
    assert(PyUnicode_Check(str1));
    assert(PyUnicode_Check(str2));
    if (str1 == str2) {
        return 1;
    }
    return unicode_eq(str1, str2);
}


int
PyUnicode_Equal(PyObject *str1, PyObject *str2)
{
    if (!PyUnicode_Check(str1)) {
        PyErr_Format(PyExc_TypeError,
                     "first argument must be str, not %T", str1);
        return -1;
    }
    if (!PyUnicode_Check(str2)) {
        PyErr_Format(PyExc_TypeError,
                     "second argument must be str, not %T", str2);
        return -1;
    }

    return _PyUnicode_Equal(str1, str2);
}


int
PyUnicode_Compare(PyObject *left, PyObject *right)
{
    if (PyUnicode_Check(left) && PyUnicode_Check(right)) {
        /* a string is equal to itself */
        if (left == right)
            return 0;

        return unicode_compare(left, right);
    }
    PyErr_Format(PyExc_TypeError,
                 "Can't compare %.100s and %.100s",
                 Py_TYPE(left)->tp_name,
                 Py_TYPE(right)->tp_name);
    return -1;
}

int
PyUnicode_CompareWithASCIIString(PyObject* uni, const char* str)
{
    Py_ssize_t i;
    int kind;
    Py_UCS4 chr;

    assert(_PyUnicode_CHECK(uni));
    kind = PyUnicode_KIND(uni);
    if (kind == PyUnicode_1BYTE_KIND) {
        const void *data = PyUnicode_1BYTE_DATA(uni);
        size_t len1 = (size_t)PyUnicode_GET_LENGTH(uni);
        size_t len, len2 = strlen(str);
        int cmp;

        len = Py_MIN(len1, len2);
        cmp = memcmp(data, str, len);
        if (cmp != 0) {
            if (cmp < 0)
                return -1;
            else
                return 1;
        }
        if (len1 > len2)
            return 1; /* uni is longer */
        if (len1 < len2)
            return -1; /* str is longer */
        return 0;
    }
    else {
        const void *data = PyUnicode_DATA(uni);
        /* Compare Unicode string and source character set string */
        for (i = 0; (chr = PyUnicode_READ(kind, data, i)) && str[i]; i++)
            if (chr != (unsigned char)str[i])
                return (chr < (unsigned char)(str[i])) ? -1 : 1;
        /* This check keeps Python strings that end in '\0' from comparing equal
         to C strings identical up to that point. */
        if (PyUnicode_GET_LENGTH(uni) != i || chr)
            return 1; /* uni is longer */
        if (str[i])
            return -1; /* str is longer */
        return 0;
    }
}

int
PyUnicode_EqualToUTF8(PyObject *unicode, const char *str)
{
    return PyUnicode_EqualToUTF8AndSize(unicode, str, strlen(str));
}

int
PyUnicode_EqualToUTF8AndSize(PyObject *unicode, const char *str, Py_ssize_t size)
{
    assert(_PyUnicode_CHECK(unicode));
    assert(str);

    if (PyUnicode_IS_ASCII(unicode)) {
        Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
        return size == len &&
            memcmp(PyUnicode_1BYTE_DATA(unicode), str, len) == 0;
    }
    if (PyUnicode_UTF8(unicode) != NULL) {
        Py_ssize_t len = PyUnicode_UTF8_LENGTH(unicode);
        return size == len &&
            memcmp(PyUnicode_UTF8(unicode), str, len) == 0;
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    if ((size_t)len >= (size_t)size || (size_t)len < (size_t)size / 4) {
        return 0;
    }
    const unsigned char *s = (const unsigned char *)str;
    const unsigned char *ends = s + (size_t)size;
    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    /* Compare Unicode string and UTF-8 string */
    for (Py_ssize_t i = 0; i < len; i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch < 0x80) {
            if (ends == s || s[0] != ch) {
                return 0;
            }
            s += 1;
        }
        else if (ch < 0x800) {
            if ((ends - s) < 2 ||
                s[0] != (0xc0 | (ch >> 6)) ||
                s[1] != (0x80 | (ch & 0x3f)))
            {
                return 0;
            }
            s += 2;
        }
        else if (ch < 0x10000) {
            if (Py_UNICODE_IS_SURROGATE(ch) ||
                (ends - s) < 3 ||
                s[0] != (0xe0 | (ch >> 12)) ||
                s[1] != (0x80 | ((ch >> 6) & 0x3f)) ||
                s[2] != (0x80 | (ch & 0x3f)))
            {
                return 0;
            }
            s += 3;
        }
        else {
            assert(ch <= MAX_UNICODE);
            if ((ends - s) < 4 ||
                s[0] != (0xf0 | (ch >> 18)) ||
                s[1] != (0x80 | ((ch >> 12) & 0x3f)) ||
                s[2] != (0x80 | ((ch >> 6) & 0x3f)) ||
                s[3] != (0x80 | (ch & 0x3f)))
            {
                return 0;
            }
            s += 4;
        }
    }
    return s == ends;
}

int
_PyUnicode_EqualToASCIIString(PyObject *unicode, const char *str)
{
    size_t len;
    assert(_PyUnicode_CHECK(unicode));
    assert(str);
#ifndef NDEBUG
    for (const char *p = str; *p; p++) {
        assert((unsigned char)*p < 128);
    }
#endif
    if (!PyUnicode_IS_ASCII(unicode))
        return 0;
    len = (size_t)PyUnicode_GET_LENGTH(unicode);
    return strlen(str) == len &&
           memcmp(PyUnicode_1BYTE_DATA(unicode), str, len) == 0;
}

int
_PyUnicode_EqualToASCIIId(PyObject *left, _Py_Identifier *right)
{
    PyObject *right_uni;

    assert(_PyUnicode_CHECK(left));
    assert(right->string);
#ifndef NDEBUG
    for (const char *p = right->string; *p; p++) {
        assert((unsigned char)*p < 128);
    }
#endif

    if (!PyUnicode_IS_ASCII(left))
        return 0;

    right_uni = _PyUnicode_FromId(right);       /* borrowed */
    if (right_uni == NULL) {
        /* memory error or bad data */
        PyErr_Clear();
        return _PyUnicode_EqualToASCIIString(left, right->string);
    }

    if (left == right_uni)
        return 1;

    assert(PyUnicode_CHECK_INTERNED(right_uni));
    if (PyUnicode_CHECK_INTERNED(left)) {
        return 0;
    }

    Py_hash_t right_hash = PyUnicode_HASH(right_uni);
    assert(right_hash != -1);
    Py_hash_t hash = PyUnicode_HASH(left);
    if (hash != -1 && hash != right_hash) {
        return 0;
    }

    return unicode_eq(left, right_uni);
}

PyObject *
PyUnicode_RichCompare(PyObject *left, PyObject *right, int op)
{
    int result;

    if (!PyUnicode_Check(left) || !PyUnicode_Check(right))
        Py_RETURN_NOTIMPLEMENTED;

    if (left == right) {
        switch (op) {
        case Py_EQ:
        case Py_LE:
        case Py_GE:
            /* a string is equal to itself */
            Py_RETURN_TRUE;
        case Py_NE:
        case Py_LT:
        case Py_GT:
            Py_RETURN_FALSE;
        default:
            PyErr_BadArgument();
            return NULL;
        }
    }
    else if (op == Py_EQ || op == Py_NE) {
        result = unicode_eq(left, right);
        result ^= (op == Py_NE);
        return PyBool_FromLong(result);
    }
    else {
        result = unicode_compare(left, right);
        Py_RETURN_RICHCOMPARE(result, 0, op);
    }
}

int
PyUnicode_Contains(PyObject *str, PyObject *substr)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;
    int result;

    if (!PyUnicode_Check(substr)) {
        PyErr_Format(PyExc_TypeError,
                     "'in <string>' requires string as left operand, not %.100s",
                     Py_TYPE(substr)->tp_name);
        return -1;
    }
    if (ensure_unicode(str) < 0)
        return -1;

    kind1 = PyUnicode_KIND(str);
    kind2 = PyUnicode_KIND(substr);
    if (kind1 < kind2)
        return 0;
    len1 = PyUnicode_GET_LENGTH(str);
    len2 = PyUnicode_GET_LENGTH(substr);
    if (len1 < len2)
        return 0;
    buf1 = PyUnicode_DATA(str);
    buf2 = PyUnicode_DATA(substr);
    if (len2 == 1) {
        Py_UCS4 ch = PyUnicode_READ(kind2, buf2, 0);
        result = findchar((const char *)buf1, kind1, len1, ch, 1) != -1;
        return result;
    }
    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            return -1;
    }

    switch (kind1) {
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
        Py_UNREACHABLE();
    }

    assert((kind2 == kind1) == (buf2 == PyUnicode_DATA(substr)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return result;
}

/* Concat to string or Unicode object giving a new Unicode object. */

PyObject *
PyUnicode_Concat(PyObject *left, PyObject *right)
{
    PyObject *result;
    Py_UCS4 maxchar, maxchar2;
    Py_ssize_t left_len, right_len, new_len;

    if (ensure_unicode(left) < 0)
        return NULL;

    if (!PyUnicode_Check(right)) {
        if (_PyTemplate_CheckExact(right)) {
            // str + tstring is implemented in the tstring type
            return _PyTemplate_Concat(left, right);
        }
        else {
            PyErr_Format(PyExc_TypeError,
                "can only concatenate str (not \"%.200s\") to str",
                Py_TYPE(right)->tp_name);
            return NULL;
        }
    }

    /* Shortcuts */
    PyObject *empty = unicode_get_empty();  // Borrowed reference
    if (left == empty) {
        return PyUnicode_FromObject(right);
    }
    if (right == empty) {
        return PyUnicode_FromObject(left);
    }

    left_len = PyUnicode_GET_LENGTH(left);
    right_len = PyUnicode_GET_LENGTH(right);
    if (left_len > PY_SSIZE_T_MAX - right_len) {
        PyErr_SetString(PyExc_OverflowError,
                        "strings are too large to concat");
        return NULL;
    }
    new_len = left_len + right_len;

    maxchar = PyUnicode_MAX_CHAR_VALUE(left);
    maxchar2 = PyUnicode_MAX_CHAR_VALUE(right);
    maxchar = Py_MAX(maxchar, maxchar2);

    /* Concat the two Unicode strings */
    result = PyUnicode_New(new_len, maxchar);
    if (result == NULL)
        return NULL;
    _PyUnicode_FastCopyCharacters(result, 0, left, 0, left_len);
    _PyUnicode_FastCopyCharacters(result, left_len, right, 0, right_len);
    assert(_PyUnicode_CheckConsistency(result, 1));
    return result;
}

void
PyUnicode_Append(PyObject **p_left, PyObject *right)
{
    PyObject *left, *res;
    Py_UCS4 maxchar, maxchar2;
    Py_ssize_t left_len, right_len, new_len;

    if (p_left == NULL) {
        if (!PyErr_Occurred())
            PyErr_BadInternalCall();
        return;
    }
    left = *p_left;
    if (right == NULL || left == NULL
        || !PyUnicode_Check(left) || !PyUnicode_Check(right)) {
        if (!PyErr_Occurred())
            PyErr_BadInternalCall();
        goto error;
    }

    /* Shortcuts */
    PyObject *empty = unicode_get_empty();  // Borrowed reference
    if (left == empty) {
        Py_DECREF(left);
        *p_left = Py_NewRef(right);
        return;
    }
    if (right == empty) {
        return;
    }

    left_len = PyUnicode_GET_LENGTH(left);
    right_len = PyUnicode_GET_LENGTH(right);
    if (left_len > PY_SSIZE_T_MAX - right_len) {
        PyErr_SetString(PyExc_OverflowError,
                        "strings are too large to concat");
        goto error;
    }
    new_len = left_len + right_len;

    if (unicode_modifiable(left)
        && PyUnicode_CheckExact(right)
        && PyUnicode_KIND(right) <= PyUnicode_KIND(left)
        /* Don't resize for ascii += latin1. Convert ascii to latin1 requires
           to change the structure size, but characters are stored just after
           the structure, and so it requires to move all characters which is
           not so different than duplicating the string. */
        && !(PyUnicode_IS_ASCII(left) && !PyUnicode_IS_ASCII(right)))
    {
        /* append inplace */
        if (unicode_resize(p_left, new_len) != 0)
            goto error;

        /* copy 'right' into the newly allocated area of 'left' */
        _PyUnicode_FastCopyCharacters(*p_left, left_len, right, 0, right_len);
    }
    else {
        maxchar = PyUnicode_MAX_CHAR_VALUE(left);
        maxchar2 = PyUnicode_MAX_CHAR_VALUE(right);
        maxchar = Py_MAX(maxchar, maxchar2);

        /* Concat the two Unicode strings */
        res = PyUnicode_New(new_len, maxchar);
        if (res == NULL)
            goto error;
        _PyUnicode_FastCopyCharacters(res, 0, left, 0, left_len);
        _PyUnicode_FastCopyCharacters(res, left_len, right, 0, right_len);
        Py_DECREF(left);
        *p_left = res;
    }
    assert(_PyUnicode_CheckConsistency(*p_left, 1));
    return;

error:
    Py_CLEAR(*p_left);
}

void
PyUnicode_AppendAndDel(PyObject **pleft, PyObject *right)
{
    PyUnicode_Append(pleft, right);
    Py_XDECREF(right);
}

/*[clinic input]
@text_signature "($self, sub[, start[, end]], /)"
str.count as unicode_count -> Py_ssize_t

    self as str: self
    sub as substr: unicode
    start: slice_index(accept={int, NoneType}, c_default='0') = None
    end: slice_index(accept={int, NoneType}, c_default='PY_SSIZE_T_MAX') = None
    /

Return the number of non-overlapping occurrences of substring sub in string S[start:end].

Optional arguments start and end are interpreted as in slice notation.
[clinic start generated code]*/

static Py_ssize_t
unicode_count_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end)
/*[clinic end generated code: output=8fcc3aef0b18edbf input=6f168ffd94be8785]*/
{
    assert(PyUnicode_Check(str));
    assert(PyUnicode_Check(substr));

    Py_ssize_t result;
    int kind1, kind2;
    const void *buf1 = NULL, *buf2 = NULL;
    Py_ssize_t len1, len2;

    kind1 = PyUnicode_KIND(str);
    kind2 = PyUnicode_KIND(substr);
    if (kind1 < kind2)
        return 0;

    len1 = PyUnicode_GET_LENGTH(str);
    len2 = PyUnicode_GET_LENGTH(substr);
    ADJUST_INDICES(start, end, len1);
    if (end - start < len2)
        return 0;

    buf1 = PyUnicode_DATA(str);
    buf2 = PyUnicode_DATA(substr);
    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            goto onError;
    }

    // We don't reuse `anylib_count` here because of the explicit casts.
    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        result = ucs1lib_count(
            ((const Py_UCS1*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_2BYTE_KIND:
        result = ucs2lib_count(
            ((const Py_UCS2*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_4BYTE_KIND:
        result = ucs4lib_count(
            ((const Py_UCS4*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    default:
        Py_UNREACHABLE();
    }

    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substr)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return result;
  onError:
    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substr)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);
    return -1;
}

/*[clinic input]
str.encode as unicode_encode

    encoding: str(c_default="NULL") = 'utf-8'
        The encoding in which to encode the string.
    errors: str(c_default="NULL") = 'strict'
        The error handling scheme to use for encoding errors.
        The default is 'strict' meaning that encoding errors raise a
        UnicodeEncodeError.  Other possible values are 'ignore', 'replace' and
        'xmlcharrefreplace' as well as any other name registered with
        codecs.register_error that can handle UnicodeEncodeErrors.

Encode the string using the codec registered for encoding.
[clinic start generated code]*/

static PyObject *
unicode_encode_impl(PyObject *self, const char *encoding, const char *errors)
/*[clinic end generated code: output=bf78b6e2a9470e3c input=f0a9eb293d08fe02]*/
{
    return PyUnicode_AsEncodedString(self, encoding, errors);
}

/*[clinic input]
str.expandtabs as unicode_expandtabs

    tabsize: int = 8

Return a copy where all tab characters are expanded using spaces.

If tabsize is not given, a tab size of 8 characters is assumed.
[clinic start generated code]*/

static PyObject *
unicode_expandtabs_impl(PyObject *self, int tabsize)
/*[clinic end generated code: output=3457c5dcee26928f input=8a01914034af4c85]*/
{
    Py_ssize_t i, j, line_pos, src_len, incr;
    Py_UCS4 ch;
    PyObject *u;
    const void *src_data;
    void *dest_data;
    int kind;
    int found;

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
    if (!found)
        return unicode_result_unchanged(self);

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
                unicode_fill(kind, dest_data, ' ', j, incr);
                j += incr;
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
    return unicode_result(u);

  overflow:
    PyErr_SetString(PyExc_OverflowError, "new string is too long");
    return NULL;
}

/*[clinic input]
str.find as unicode_find = str.count

Return the lowest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Return -1 on failure.
[clinic start generated code]*/

static Py_ssize_t
unicode_find_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                  Py_ssize_t end)
/*[clinic end generated code: output=51dbe6255712e278 input=4a89d2d68ef57256]*/
{
    Py_ssize_t result = any_find_slice(str, substr, start, end, 1);
    if (result < 0) {
        return -1;
    }
    return result;
}

static PyObject *
unicode_getitem(PyObject *self, Py_ssize_t index)
{
    const void *data;
    int kind;
    Py_UCS4 ch;

    if (!PyUnicode_Check(self)) {
        PyErr_BadArgument();
        return NULL;
    }
    if (index < 0 || index >= PyUnicode_GET_LENGTH(self)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return NULL;
    }
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    ch = PyUnicode_READ(kind, data, index);
    return unicode_char(ch);
}

/* Believe it or not, this produces the same value for ASCII strings
   as bytes_hash(). */
static Py_hash_t
unicode_hash(PyObject *self)
{
    Py_uhash_t x;  /* Unsigned for defined overflow behavior. */

#ifdef Py_DEBUG
    assert(_Py_HashSecret_Initialized);
#endif
    Py_hash_t hash = PyUnicode_HASH(self);
    if (hash != -1) {
        return hash;
    }
    x = Py_HashBuffer(PyUnicode_DATA(self),
                      PyUnicode_GET_LENGTH(self) * PyUnicode_KIND(self));

    PyUnicode_SET_HASH(self, x);
    return x;
}

/*[clinic input]
str.index as unicode_index = str.count

Return the lowest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Raises ValueError when the substring is not found.
[clinic start generated code]*/

static Py_ssize_t
unicode_index_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end)
/*[clinic end generated code: output=77558288837cdf40 input=d986aeac0be14a1c]*/
{
    Py_ssize_t result = any_find_slice(str, substr, start, end, 1);
    if (result == -1) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
    }
    else if (result < 0) {
        return -1;
    }
    return result;
}

/*[clinic input]
str.isascii as unicode_isascii

Return True if all characters in the string are ASCII, False otherwise.

ASCII characters have code points in the range U+0000-U+007F.
Empty string is ASCII too.
[clinic start generated code]*/

static PyObject *
unicode_isascii_impl(PyObject *self)
/*[clinic end generated code: output=c5910d64b5a8003f input=5a43cbc6399621d5]*/
{
    return PyBool_FromLong(PyUnicode_IS_ASCII(self));
}

/*[clinic input]
str.islower as unicode_islower

Return True if the string is a lowercase string, False otherwise.

A string is lowercase if all cased characters in the string are lowercase and
there is at least one cased character in the string.
[clinic start generated code]*/

static PyObject *
unicode_islower_impl(PyObject *self)
/*[clinic end generated code: output=dbd41995bd005b81 input=acec65ac6821ae47]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;
    int cased;

    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISLOWER(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        Py_RETURN_FALSE;

    cased = 0;
    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch))
            Py_RETURN_FALSE;
        else if (!cased && Py_UNICODE_ISLOWER(ch))
            cased = 1;
    }
    return PyBool_FromLong(cased);
}

/*[clinic input]
str.isupper as unicode_isupper

Return True if the string is an uppercase string, False otherwise.

A string is uppercase if all cased characters in the string are uppercase and
there is at least one cased character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isupper_impl(PyObject *self)
/*[clinic end generated code: output=049209c8e7f15f59 input=e9b1feda5d17f2d3]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;
    int cased;

    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISUPPER(PyUnicode_READ(kind, data, 0)) != 0);

    /* Special case for empty strings */
    if (length == 0)
        Py_RETURN_FALSE;

    cased = 0;
    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        if (Py_UNICODE_ISLOWER(ch) || Py_UNICODE_ISTITLE(ch))
            Py_RETURN_FALSE;
        else if (!cased && Py_UNICODE_ISUPPER(ch))
            cased = 1;
    }
    return PyBool_FromLong(cased);
}

/*[clinic input]
str.istitle as unicode_istitle

Return True if the string is a title-cased string, False otherwise.

In a title-cased string, upper- and title-case characters may only
follow uncased characters and lowercase characters only cased ones.
[clinic start generated code]*/

static PyObject *
unicode_istitle_impl(PyObject *self)
/*[clinic end generated code: output=e9bf6eb91f5d3f0e input=98d32bd2e1f06f8c]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;
    int cased, previous_is_cased;

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
        Py_RETURN_FALSE;

    cased = 0;
    previous_is_cased = 0;
    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);

        if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch)) {
            if (previous_is_cased)
                Py_RETURN_FALSE;
            previous_is_cased = 1;
            cased = 1;
        }
        else if (Py_UNICODE_ISLOWER(ch)) {
            if (!previous_is_cased)
                Py_RETURN_FALSE;
            previous_is_cased = 1;
            cased = 1;
        }
        else
            previous_is_cased = 0;
    }
    return PyBool_FromLong(cased);
}

/*[clinic input]
str.isspace as unicode_isspace

Return True if the string is a whitespace string, False otherwise.

A string is whitespace if all characters in the string are whitespace and there
is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isspace_impl(PyObject *self)
/*[clinic end generated code: output=163a63bfa08ac2b9 input=fe462cb74f8437d8]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;

    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISSPACE(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        Py_RETURN_FALSE;

    for (i = 0; i < length; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (!Py_UNICODE_ISSPACE(ch))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/*[clinic input]
str.isalpha as unicode_isalpha

Return True if the string is an alphabetic string, False otherwise.

A string is alphabetic if all characters in the string are alphabetic and there
is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isalpha_impl(PyObject *self)
/*[clinic end generated code: output=cc81b9ac3883ec4f input=d0fd18a96cbca5eb]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;

    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISALPHA(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        Py_RETURN_FALSE;

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISALPHA(PyUnicode_READ(kind, data, i)))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/*[clinic input]
str.isalnum as unicode_isalnum

Return True if the string is an alpha-numeric string, False otherwise.

A string is alpha-numeric if all characters in the string are alpha-numeric and
there is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isalnum_impl(PyObject *self)
/*[clinic end generated code: output=a5a23490ffc3660c input=5c6579bf2e04758c]*/
{
    int kind;
    const void *data;
    Py_ssize_t len, i;

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
        Py_RETURN_FALSE;

    for (i = 0; i < len; i++) {
        const Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (!Py_UNICODE_ISALNUM(ch))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/*[clinic input]
str.isdecimal as unicode_isdecimal

Return True if the string is a decimal string, False otherwise.

A string is a decimal string if all characters in the string are decimal and
there is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isdecimal_impl(PyObject *self)
/*[clinic end generated code: output=fb2dcdb62d3fc548 input=336bc97ab4c8268f]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;

    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISDECIMAL(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        Py_RETURN_FALSE;

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISDECIMAL(PyUnicode_READ(kind, data, i)))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/*[clinic input]
str.isdigit as unicode_isdigit

Return True if the string is a digit string, False otherwise.

A string is a digit string if all characters in the string are digits and there
is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isdigit_impl(PyObject *self)
/*[clinic end generated code: output=10a6985311da6858 input=901116c31deeea4c]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;

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
        Py_RETURN_FALSE;

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISDIGIT(PyUnicode_READ(kind, data, i)))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/*[clinic input]
str.isnumeric as unicode_isnumeric

Return True if the string is a numeric string, False otherwise.

A string is numeric if all characters in the string are numeric and there is at
least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isnumeric_impl(PyObject *self)
/*[clinic end generated code: output=9172a32d9013051a input=722507db976f826c]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;

    length = PyUnicode_GET_LENGTH(self);
    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);

    /* Shortcut for single character strings */
    if (length == 1)
        return PyBool_FromLong(
            Py_UNICODE_ISNUMERIC(PyUnicode_READ(kind, data, 0)));

    /* Special case for empty strings */
    if (length == 0)
        Py_RETURN_FALSE;

    for (i = 0; i < length; i++) {
        if (!Py_UNICODE_ISNUMERIC(PyUnicode_READ(kind, data, i)))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

Py_ssize_t
_PyUnicode_ScanIdentifier(PyObject *self)
{
    Py_ssize_t i;
    Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    if (len == 0) {
        /* an empty string is not a valid identifier */
        return 0;
    }

    int kind = PyUnicode_KIND(self);
    const void *data = PyUnicode_DATA(self);
    Py_UCS4 ch = PyUnicode_READ(kind, data, 0);
    /* PEP 3131 says that the first character must be in
       XID_Start and subsequent characters in XID_Continue,
       and for the ASCII range, the 2.x rules apply (i.e
       start with letters and underscore, continue with
       letters, digits, underscore). However, given the current
       definition of XID_Start and XID_Continue, it is sufficient
       to check just for these, except that _ must be allowed
       as starting an identifier.  */
    if (!_PyUnicode_IsXidStart(ch) && ch != 0x5F /* LOW LINE */) {
        return 0;
    }

    for (i = 1; i < len; i++) {
        ch = PyUnicode_READ(kind, data, i);
        if (!_PyUnicode_IsXidContinue(ch)) {
            return i;
        }
    }
    return i;
}

int
PyUnicode_IsIdentifier(PyObject *self)
{
    Py_ssize_t i = _PyUnicode_ScanIdentifier(self);
    Py_ssize_t len = PyUnicode_GET_LENGTH(self);
    /* an empty string is not a valid identifier */
    return len && i == len;
}

/*[clinic input]
str.isidentifier as unicode_isidentifier

Return True if the string is a valid Python identifier, False otherwise.

Call keyword.iskeyword(s) to test whether string s is a reserved identifier,
such as "def" or "class".
[clinic start generated code]*/

static PyObject *
unicode_isidentifier_impl(PyObject *self)
/*[clinic end generated code: output=fe585a9666572905 input=2d807a104f21c0c5]*/
{
    return PyBool_FromLong(PyUnicode_IsIdentifier(self));
}

/*[clinic input]
str.isprintable as unicode_isprintable

Return True if all characters in the string are printable, False otherwise.

A character is printable if repr() may use it in its output.
[clinic start generated code]*/

static PyObject *
unicode_isprintable_impl(PyObject *self)
/*[clinic end generated code: output=3ab9626cd32dd1a0 input=4e56bcc6b06ca18c]*/
{
    Py_ssize_t i, length;
    int kind;
    const void *data;

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

/*[clinic input]
str.join as unicode_join

    iterable: object
    /

Concatenate any number of strings.

The string whose method is called is inserted in between each given string.
The result is returned as a new string.

Example: '.'.join(['ab', 'pq', 'rs']) -> 'ab.pq.rs'
[clinic start generated code]*/

static PyObject *
unicode_join(PyObject *self, PyObject *iterable)
/*[clinic end generated code: output=6857e7cecfe7bf98 input=2f70422bfb8fa189]*/
{
    return PyUnicode_Join(self, iterable);
}

static Py_ssize_t
unicode_length(PyObject *self)
{
    return PyUnicode_GET_LENGTH(self);
}

/*[clinic input]
str.ljust as unicode_ljust

    width: Py_ssize_t
    fillchar: Py_UCS4 = ' '
    /

Return a left-justified string of length width.

Padding is done using the specified fill character (default is a space).
[clinic start generated code]*/

static PyObject *
unicode_ljust_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar)
/*[clinic end generated code: output=1cce0e0e0a0b84b3 input=3ab599e335e60a32]*/
{
    if (PyUnicode_GET_LENGTH(self) >= width)
        return unicode_result_unchanged(self);

    return pad(self, 0, width - PyUnicode_GET_LENGTH(self), fillchar);
}

/*[clinic input]
str.lower as unicode_lower

Return a copy of the string converted to lowercase.
[clinic start generated code]*/

static PyObject *
unicode_lower_impl(PyObject *self)
/*[clinic end generated code: output=84ef9ed42efad663 input=60a2984b8beff23a]*/
{
    if (PyUnicode_IS_ASCII(self))
        return ascii_upper_or_lower(self, 1);
    return case_operation(self, do_lower);
}

#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2

/* Arrays indexed by above */
static const char *stripfuncnames[] = {"lstrip", "rstrip", "strip"};

#define STRIPNAME(i) (stripfuncnames[i])

/* externally visible for str.strip(unicode) */
PyObject *
_PyUnicode_XStrip(PyObject *self, int striptype, PyObject *sepobj)
{
    const void *data;
    int kind;
    Py_ssize_t i, j, len;
    BLOOM_MASK sepmask;
    Py_ssize_t seplen;

    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    len = PyUnicode_GET_LENGTH(self);
    seplen = PyUnicode_GET_LENGTH(sepobj);
    sepmask = make_bloom_mask(PyUnicode_KIND(sepobj),
                              PyUnicode_DATA(sepobj),
                              seplen);

    i = 0;
    if (striptype != RIGHTSTRIP) {
        while (i < len) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, i);
            if (!BLOOM(sepmask, ch))
                break;
            if (PyUnicode_FindChar(sepobj, ch, 0, seplen, 1) < 0)
                break;
            i++;
        }
    }

    j = len;
    if (striptype != LEFTSTRIP) {
        j--;
        while (j >= i) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, j);
            if (!BLOOM(sepmask, ch))
                break;
            if (PyUnicode_FindChar(sepobj, ch, 0, seplen, 1) < 0)
                break;
            j--;
        }

        j++;
    }

    return PyUnicode_Substring(self, i, j);
}

PyObject*
PyUnicode_Substring(PyObject *self, Py_ssize_t start, Py_ssize_t end)
{
    const unsigned char *data;
    int kind;
    Py_ssize_t length;

    length = PyUnicode_GET_LENGTH(self);
    end = Py_MIN(end, length);

    if (start == 0 && end == length)
        return unicode_result_unchanged(self);

    if (start < 0 || end < 0) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return NULL;
    }
    if (start >= length || end < start)
        _Py_RETURN_UNICODE_EMPTY();

    length = end - start;
    if (PyUnicode_IS_ASCII(self)) {
        data = PyUnicode_1BYTE_DATA(self);
        return _PyUnicode_FromASCII((const char*)(data + start), length);
    }
    else {
        kind = PyUnicode_KIND(self);
        data = PyUnicode_1BYTE_DATA(self);
        return PyUnicode_FromKindAndData(kind,
                                         data + kind * start,
                                         length);
    }
}

static PyObject *
do_strip(PyObject *self, int striptype)
{
    Py_ssize_t len, i, j;

    len = PyUnicode_GET_LENGTH(self);

    if (PyUnicode_IS_ASCII(self)) {
        const Py_UCS1 *data = PyUnicode_1BYTE_DATA(self);

        i = 0;
        if (striptype != RIGHTSTRIP) {
            while (i < len) {
                Py_UCS1 ch = data[i];
                if (!_Py_ascii_whitespace[ch])
                    break;
                i++;
            }
        }

        j = len;
        if (striptype != LEFTSTRIP) {
            j--;
            while (j >= i) {
                Py_UCS1 ch = data[j];
                if (!_Py_ascii_whitespace[ch])
                    break;
                j--;
            }
            j++;
        }
    }
    else {
        int kind = PyUnicode_KIND(self);
        const void *data = PyUnicode_DATA(self);

        i = 0;
        if (striptype != RIGHTSTRIP) {
            while (i < len) {
                Py_UCS4 ch = PyUnicode_READ(kind, data, i);
                if (!Py_UNICODE_ISSPACE(ch))
                    break;
                i++;
            }
        }

        j = len;
        if (striptype != LEFTSTRIP) {
            j--;
            while (j >= i) {
                Py_UCS4 ch = PyUnicode_READ(kind, data, j);
                if (!Py_UNICODE_ISSPACE(ch))
                    break;
                j--;
            }
            j++;
        }
    }

    return PyUnicode_Substring(self, i, j);
}


static PyObject *
do_argstrip(PyObject *self, int striptype, PyObject *sep)
{
    if (sep != Py_None) {
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


/*[clinic input]
str.strip as unicode_strip

    chars: object = None
    /

Return a copy of the string with leading and trailing whitespace removed.

If chars is given and not None, remove characters in chars instead.
[clinic start generated code]*/

static PyObject *
unicode_strip_impl(PyObject *self, PyObject *chars)
/*[clinic end generated code: output=ca19018454345d57 input=385289c6f423b954]*/
{
    return do_argstrip(self, BOTHSTRIP, chars);
}


/*[clinic input]
str.lstrip as unicode_lstrip

    chars: object = None
    /

Return a copy of the string with leading whitespace removed.

If chars is given and not None, remove characters in chars instead.
[clinic start generated code]*/

static PyObject *
unicode_lstrip_impl(PyObject *self, PyObject *chars)
/*[clinic end generated code: output=3b43683251f79ca7 input=529f9f3834448671]*/
{
    return do_argstrip(self, LEFTSTRIP, chars);
}


/*[clinic input]
str.rstrip as unicode_rstrip

    chars: object = None
    /

Return a copy of the string with trailing whitespace removed.

If chars is given and not None, remove characters in chars instead.
[clinic start generated code]*/

static PyObject *
unicode_rstrip_impl(PyObject *self, PyObject *chars)
/*[clinic end generated code: output=4a59230017cc3b7a input=62566c627916557f]*/
{
    return do_argstrip(self, RIGHTSTRIP, chars);
}


static PyObject*
unicode_repeat(PyObject *str, Py_ssize_t len)
{
    PyObject *u;
    Py_ssize_t nchars, n;

    if (len < 1)
        _Py_RETURN_UNICODE_EMPTY();

    /* no repeat, return original string */
    if (len == 1)
        return unicode_result_unchanged(str);

    if (PyUnicode_GET_LENGTH(str) > PY_SSIZE_T_MAX / len) {
        PyErr_SetString(PyExc_OverflowError,
                        "repeated string is too long");
        return NULL;
    }
    nchars = len * PyUnicode_GET_LENGTH(str);

    u = PyUnicode_New(nchars, PyUnicode_MAX_CHAR_VALUE(str));
    if (!u)
        return NULL;
    assert(PyUnicode_KIND(u) == PyUnicode_KIND(str));

    if (PyUnicode_GET_LENGTH(str) == 1) {
        int kind = PyUnicode_KIND(str);
        Py_UCS4 fill_char = PyUnicode_READ(kind, PyUnicode_DATA(str), 0);
        if (kind == PyUnicode_1BYTE_KIND) {
            void *to = PyUnicode_DATA(u);
            memset(to, (unsigned char)fill_char, len);
        }
        else if (kind == PyUnicode_2BYTE_KIND) {
            Py_UCS2 *ucs2 = PyUnicode_2BYTE_DATA(u);
            for (n = 0; n < len; ++n)
                ucs2[n] = fill_char;
        } else {
            Py_UCS4 *ucs4 = PyUnicode_4BYTE_DATA(u);
            assert(kind == PyUnicode_4BYTE_KIND);
            for (n = 0; n < len; ++n)
                ucs4[n] = fill_char;
        }
    }
    else {
        Py_ssize_t char_size = PyUnicode_KIND(str);
        char *to = (char *) PyUnicode_DATA(u);
        _PyBytes_Repeat(to, nchars * char_size, PyUnicode_DATA(str),
            PyUnicode_GET_LENGTH(str) * char_size);
    }

    assert(_PyUnicode_CheckConsistency(u, 1));
    return u;
}

PyObject *
PyUnicode_Replace(PyObject *str,
                  PyObject *substr,
                  PyObject *replstr,
                  Py_ssize_t maxcount)
{
    if (ensure_unicode(str) < 0 || ensure_unicode(substr) < 0 ||
            ensure_unicode(replstr) < 0)
        return NULL;
    return replace(str, substr, replstr, maxcount);
}

/*[clinic input]
str.replace as unicode_replace

    old: unicode
    new: unicode
    /
    count: Py_ssize_t = -1
        Maximum number of occurrences to replace.
        -1 (the default value) means replace all occurrences.

Return a copy with all occurrences of substring old replaced by new.

If the optional argument count is given, only the first count occurrences are
replaced.
[clinic start generated code]*/

static PyObject *
unicode_replace_impl(PyObject *self, PyObject *old, PyObject *new,
                     Py_ssize_t count)
/*[clinic end generated code: output=b63f1a8b5eebf448 input=3345c455d60a5499]*/
{
    return replace(self, old, new, count);
}

/*[clinic input]
str.removeprefix as unicode_removeprefix

    prefix: unicode
    /

Return a str with the given prefix string removed if present.

If the string starts with the prefix string, return string[len(prefix):].
Otherwise, return a copy of the original string.
[clinic start generated code]*/

static PyObject *
unicode_removeprefix_impl(PyObject *self, PyObject *prefix)
/*[clinic end generated code: output=f1e5945e9763bcb9 input=27ec40b99a37eb88]*/
{
    int match = tailmatch(self, prefix, 0, PY_SSIZE_T_MAX, -1);
    if (match == -1) {
        return NULL;
    }
    if (match) {
        return PyUnicode_Substring(self, PyUnicode_GET_LENGTH(prefix),
                                   PyUnicode_GET_LENGTH(self));
    }
    return unicode_result_unchanged(self);
}

/*[clinic input]
str.removesuffix as unicode_removesuffix

    suffix: unicode
    /

Return a str with the given suffix string removed if present.

If the string ends with the suffix string and that suffix is not empty,
return string[:-len(suffix)]. Otherwise, return a copy of the original
string.
[clinic start generated code]*/

static PyObject *
unicode_removesuffix_impl(PyObject *self, PyObject *suffix)
/*[clinic end generated code: output=d36629e227636822 input=12cc32561e769be4]*/
{
    int match = tailmatch(self, suffix, 0, PY_SSIZE_T_MAX, +1);
    if (match == -1) {
        return NULL;
    }
    if (match) {
        return PyUnicode_Substring(self, 0, PyUnicode_GET_LENGTH(self)
                                            - PyUnicode_GET_LENGTH(suffix));
    }
    return unicode_result_unchanged(self);
}

static PyObject *
unicode_repr(PyObject *unicode)
{
    Py_ssize_t isize = PyUnicode_GET_LENGTH(unicode);
    const void *idata = PyUnicode_DATA(unicode);

    /* Compute length of output, quote characters, and
       maximum character */
    Py_ssize_t osize = 0;
    Py_UCS4 maxch = 127;
    Py_ssize_t squote = 0;
    Py_ssize_t dquote = 0;
    int ikind = PyUnicode_KIND(unicode);
    for (Py_ssize_t i = 0; i < isize; i++) {
        Py_UCS4 ch = PyUnicode_READ(ikind, idata, i);
        Py_ssize_t incr = 1;
        switch (ch) {
        case '\'': squote++; break;
        case '"':  dquote++; break;
        case '\\': case '\t': case '\r': case '\n':
            incr = 2;
            break;
        default:
            /* Fast-path ASCII */
            if (ch < ' ' || ch == 0x7f)
                incr = 4; /* \xHH */
            else if (ch < 0x7f)
                ;
            else if (Py_UNICODE_ISPRINTABLE(ch))
                maxch = (ch > maxch) ? ch : maxch;
            else if (ch < 0x100)
                incr = 4; /* \xHH */
            else if (ch < 0x10000)
                incr = 6; /* \uHHHH */
            else
                incr = 10; /* \uHHHHHHHH */
        }
        if (osize > PY_SSIZE_T_MAX - incr) {
            PyErr_SetString(PyExc_OverflowError,
                            "string is too long to generate repr");
            return NULL;
        }
        osize += incr;
    }

    Py_UCS4 quote = '\'';
    int changed = (osize != isize);
    if (squote) {
        changed = 1;
        if (dquote)
            /* Both squote and dquote present. Use squote,
               and escape them */
            osize += squote;
        else
            quote = '"';
    }
    osize += 2;   /* quotes */

    PyObject *repr = PyUnicode_New(osize, maxch);
    if (repr == NULL)
        return NULL;
    int okind = PyUnicode_KIND(repr);
    void *odata = PyUnicode_DATA(repr);

    if (!changed) {
        PyUnicode_WRITE(okind, odata, 0, quote);

        _PyUnicode_FastCopyCharacters(repr, 1,
                                      unicode, 0,
                                      isize);

        PyUnicode_WRITE(okind, odata, osize-1, quote);
    }
    else {
        switch (okind) {
        case PyUnicode_1BYTE_KIND:
            ucs1lib_repr(unicode, quote, odata);
            break;
        case PyUnicode_2BYTE_KIND:
            ucs2lib_repr(unicode, quote, odata);
            break;
        default:
            assert(okind == PyUnicode_4BYTE_KIND);
            ucs4lib_repr(unicode, quote, odata);
        }
    }

    assert(_PyUnicode_CheckConsistency(repr, 1));
    return repr;
}

/*[clinic input]
str.rfind as unicode_rfind = str.count

Return the highest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Return -1 on failure.
[clinic start generated code]*/

static Py_ssize_t
unicode_rfind_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end)
/*[clinic end generated code: output=880b29f01dd014c8 input=898361fb71f59294]*/
{
    Py_ssize_t result = any_find_slice(str, substr, start, end, -1);
    if (result < 0) {
        return -1;
    }
    return result;
}

/*[clinic input]
str.rindex as unicode_rindex = str.count

Return the highest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Raises ValueError when the substring is not found.
[clinic start generated code]*/

static Py_ssize_t
unicode_rindex_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                    Py_ssize_t end)
/*[clinic end generated code: output=5f3aef124c867fe1 input=35943dead6c1ea9d]*/
{
    Py_ssize_t result = any_find_slice(str, substr, start, end, -1);
    if (result == -1) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
    }
    else if (result < 0) {
        return -1;
    }
    return result;
}

/*[clinic input]
str.rjust as unicode_rjust

    width: Py_ssize_t
    fillchar: Py_UCS4 = ' '
    /

Return a right-justified string of length width.

Padding is done using the specified fill character (default is a space).
[clinic start generated code]*/

static PyObject *
unicode_rjust_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar)
/*[clinic end generated code: output=804a1a57fbe8d5cf input=d05f550b5beb1f72]*/
{
    if (PyUnicode_GET_LENGTH(self) >= width)
        return unicode_result_unchanged(self);

    return pad(self, width - PyUnicode_GET_LENGTH(self), 0, fillchar);
}

PyObject *
PyUnicode_Split(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)
{
    if (ensure_unicode(s) < 0 || (sep != NULL && ensure_unicode(sep) < 0))
        return NULL;

    return split(s, sep, maxsplit);
}

/*[clinic input]
str.split as unicode_split

    sep: object = None
        The separator used to split the string.

        When set to None (the default value), will split on any whitespace
        character (including \n \r \t \f and spaces) and will discard
        empty strings from the result.
    maxsplit: Py_ssize_t = -1
        Maximum number of splits.
        -1 (the default value) means no limit.

Return a list of the substrings in the string, using sep as the separator string.

Splitting starts at the front of the string and works to the end.

Note, str.split() is mainly useful for data that has been intentionally
delimited.  With natural text that includes punctuation, consider using
the regular expression module.

[clinic start generated code]*/

static PyObject *
unicode_split_impl(PyObject *self, PyObject *sep, Py_ssize_t maxsplit)
/*[clinic end generated code: output=3a65b1db356948dc input=a29bcc0c7a5af0eb]*/
{
    if (sep == Py_None)
        return split(self, NULL, maxsplit);
    if (PyUnicode_Check(sep))
        return split(self, sep, maxsplit);

    PyErr_Format(PyExc_TypeError,
                 "must be str or None, not %.100s",
                 Py_TYPE(sep)->tp_name);
    return NULL;
}

PyObject *
PyUnicode_Partition(PyObject *str_obj, PyObject *sep_obj)
{
    PyObject* out;
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;

    if (ensure_unicode(str_obj) < 0 || ensure_unicode(sep_obj) < 0)
        return NULL;

    kind1 = PyUnicode_KIND(str_obj);
    kind2 = PyUnicode_KIND(sep_obj);
    len1 = PyUnicode_GET_LENGTH(str_obj);
    len2 = PyUnicode_GET_LENGTH(sep_obj);
    if (kind1 < kind2 || len1 < len2) {
        PyObject *empty = unicode_get_empty();  // Borrowed reference
        return PyTuple_Pack(3, str_obj, empty, empty);
    }
    buf1 = PyUnicode_DATA(str_obj);
    buf2 = PyUnicode_DATA(sep_obj);
    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(str_obj) && PyUnicode_IS_ASCII(sep_obj))
            out = asciilib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        else
            out = ucs1lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    default:
        Py_UNREACHABLE();
    }

    assert((kind2 == kind1) == (buf2 == PyUnicode_DATA(sep_obj)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return out;
}


PyObject *
PyUnicode_RPartition(PyObject *str_obj, PyObject *sep_obj)
{
    PyObject* out;
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;

    if (ensure_unicode(str_obj) < 0 || ensure_unicode(sep_obj) < 0)
        return NULL;

    kind1 = PyUnicode_KIND(str_obj);
    kind2 = PyUnicode_KIND(sep_obj);
    len1 = PyUnicode_GET_LENGTH(str_obj);
    len2 = PyUnicode_GET_LENGTH(sep_obj);
    if (kind1 < kind2 || len1 < len2) {
        PyObject *empty = unicode_get_empty();  // Borrowed reference
        return PyTuple_Pack(3, empty, empty, str_obj);
    }
    buf1 = PyUnicode_DATA(str_obj);
    buf2 = PyUnicode_DATA(sep_obj);
    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(str_obj) && PyUnicode_IS_ASCII(sep_obj))
            out = asciilib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        else
            out = ucs1lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    default:
        Py_UNREACHABLE();
    }

    assert((kind2 == kind1) == (buf2 == PyUnicode_DATA(sep_obj)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return out;
}

/*[clinic input]
str.partition as unicode_partition

    sep: object
    /

Partition the string into three parts using the given separator.

This will search for the separator in the string.  If the separator is found,
returns a 3-tuple containing the part before the separator, the separator
itself, and the part after it.

If the separator is not found, returns a 3-tuple containing the original string
and two empty strings.
[clinic start generated code]*/

static PyObject *
unicode_partition(PyObject *self, PyObject *sep)
/*[clinic end generated code: output=e4ced7bd253ca3c4 input=f29b8d06c63e50be]*/
{
    return PyUnicode_Partition(self, sep);
}

/*[clinic input]
str.rpartition as unicode_rpartition = str.partition

Partition the string into three parts using the given separator.

This will search for the separator in the string, starting at the end. If
the separator is found, returns a 3-tuple containing the part before the
separator, the separator itself, and the part after it.

If the separator is not found, returns a 3-tuple containing two empty strings
and the original string.
[clinic start generated code]*/

static PyObject *
unicode_rpartition(PyObject *self, PyObject *sep)
/*[clinic end generated code: output=1aa13cf1156572aa input=c4b7db3ef5cf336a]*/
{
    return PyUnicode_RPartition(self, sep);
}

PyObject *
PyUnicode_RSplit(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)
{
    if (ensure_unicode(s) < 0 || (sep != NULL && ensure_unicode(sep) < 0))
        return NULL;

    return rsplit(s, sep, maxsplit);
}

/*[clinic input]
str.rsplit as unicode_rsplit = str.split

Return a list of the substrings in the string, using sep as the separator string.

Splitting starts at the end of the string and works to the front.
[clinic start generated code]*/

static PyObject *
unicode_rsplit_impl(PyObject *self, PyObject *sep, Py_ssize_t maxsplit)
/*[clinic end generated code: output=c2b815c63bcabffc input=ea78406060fce33c]*/
{
    if (sep == Py_None)
        return rsplit(self, NULL, maxsplit);
    if (PyUnicode_Check(sep))
        return rsplit(self, sep, maxsplit);

    PyErr_Format(PyExc_TypeError,
                 "must be str or None, not %.100s",
                 Py_TYPE(sep)->tp_name);
    return NULL;
}

/*[clinic input]
str.splitlines as unicode_splitlines

    keepends: bool = False

Return a list of the lines in the string, breaking at line boundaries.

Line breaks are not included in the resulting list unless keepends is given and
true.
[clinic start generated code]*/

static PyObject *
unicode_splitlines_impl(PyObject *self, int keepends)
/*[clinic end generated code: output=f664dcdad153ec40 input=ba6ad05ee85d2b55]*/
{
    return PyUnicode_Splitlines(self, keepends);
}

static
PyObject *unicode_str(PyObject *self)
{
    return unicode_result_unchanged(self);
}

/*[clinic input]
str.swapcase as unicode_swapcase

Convert uppercase characters to lowercase and lowercase characters to uppercase.
[clinic start generated code]*/

static PyObject *
unicode_swapcase_impl(PyObject *self)
/*[clinic end generated code: output=5d28966bf6d7b2af input=3f3ef96d5798a7bb]*/
{
    return case_operation(self, do_swapcase);
}

/*[clinic input]

@staticmethod
str.maketrans as unicode_maketrans

  x: object

  y: unicode=NULL

  z: unicode=NULL

  /

Return a translation table usable for str.translate().

If there is only one argument, it must be a dictionary mapping Unicode
ordinals (integers) or characters to Unicode ordinals, strings or None.
Character keys will be then converted to ordinals.
If there are two arguments, they must be strings of equal length, and
in the resulting dictionary, each character in x will be mapped to the
character at the same position in y. If there is a third argument, it
must be a string, whose characters will be mapped to None in the result.
[clinic start generated code]*/

static PyObject *
unicode_maketrans_impl(PyObject *x, PyObject *y, PyObject *z)
/*[clinic end generated code: output=a925c89452bd5881 input=7bfbf529a293c6c5]*/
{
    PyObject *new = NULL, *key, *value;
    Py_ssize_t i = 0;
    int res;

    new = PyDict_New();
    if (!new)
        return NULL;
    if (y != NULL) {
        int x_kind, y_kind, z_kind;
        const void *x_data, *y_data, *z_data;

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
            if (!key)
                goto err;
            value = PyLong_FromLong(PyUnicode_READ(y_kind, y_data, i));
            if (!value) {
                Py_DECREF(key);
                goto err;
            }
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
            for (i = 0; i < PyUnicode_GET_LENGTH(z); i++) {
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
        const void *data;

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
                if (PyUnicode_GET_LENGTH(key) != 1) {
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

/*[clinic input]
str.translate as unicode_translate

    table: object
        Translation table, which must be a mapping of Unicode ordinals to
        Unicode ordinals, strings, or None.
    /

Replace each character in the string using the given translation table.

The table must implement lookup/indexing via __getitem__, for instance a
dictionary or list.  If this operation raises LookupError, the character is
left untouched.  Characters mapped to None are deleted.
[clinic start generated code]*/

static PyObject *
unicode_translate(PyObject *self, PyObject *table)
/*[clinic end generated code: output=3cb448ff2fd96bf3 input=6d38343db63d8eb0]*/
{
    return _PyUnicode_TranslateCharmap(self, table, "ignore");
}

/*[clinic input]
str.upper as unicode_upper

Return a copy of the string converted to uppercase.
[clinic start generated code]*/

static PyObject *
unicode_upper_impl(PyObject *self)
/*[clinic end generated code: output=1b7ddd16bbcdc092 input=db3d55682dfe2e6c]*/
{
    if (PyUnicode_IS_ASCII(self))
        return ascii_upper_or_lower(self, 0);
    return case_operation(self, do_upper);
}

/*[clinic input]
str.zfill as unicode_zfill

    width: Py_ssize_t
    /

Pad a numeric string with zeros on the left, to fill a field of the given width.

The string is never truncated.
[clinic start generated code]*/

static PyObject *
unicode_zfill_impl(PyObject *self, Py_ssize_t width)
/*[clinic end generated code: output=e13fb6bdf8e3b9df input=c6b2f772c6f27799]*/
{
    Py_ssize_t fill;
    PyObject *u;
    int kind;
    const void *data;
    Py_UCS4 chr;

    if (PyUnicode_GET_LENGTH(self) >= width)
        return unicode_result_unchanged(self);

    fill = width - PyUnicode_GET_LENGTH(self);

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

    assert(_PyUnicode_CheckConsistency(u, 1));
    return u;
}

/*[clinic input]
@text_signature "($self, prefix[, start[, end]], /)"
str.startswith as unicode_startswith

    prefix as subobj: object
        A string or a tuple of strings to try.
    start: slice_index(accept={int, NoneType}, c_default='0') = None
        Optional start position. Default: start of the string.
    end: slice_index(accept={int, NoneType}, c_default='PY_SSIZE_T_MAX') = None
        Optional stop position. Default: end of the string.
    /

Return True if the string starts with the specified prefix, False otherwise.
[clinic start generated code]*/

static PyObject *
unicode_startswith_impl(PyObject *self, PyObject *subobj, Py_ssize_t start,
                        Py_ssize_t end)
/*[clinic end generated code: output=4bd7cfd0803051d4 input=5f918b5f5f89d856]*/
{
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            PyObject *substring = PyTuple_GET_ITEM(subobj, i);
            if (!PyUnicode_Check(substring)) {
                PyErr_Format(PyExc_TypeError,
                             "tuple for startswith must only contain str, "
                             "not %.100s",
                             Py_TYPE(substring)->tp_name);
                return NULL;
            }
            int result = tailmatch(self, substring, start, end, -1);
            if (result < 0) {
                return NULL;
            }
            if (result) {
                Py_RETURN_TRUE;
            }
        }
        /* nothing matched */
        Py_RETURN_FALSE;
    }
    if (!PyUnicode_Check(subobj)) {
        PyErr_Format(PyExc_TypeError,
                     "startswith first arg must be str or "
                     "a tuple of str, not %.100s", Py_TYPE(subobj)->tp_name);
        return NULL;
    }
    int result = tailmatch(self, subobj, start, end, -1);
    if (result < 0) {
        return NULL;
    }
    return PyBool_FromLong(result);
}


/*[clinic input]
@text_signature "($self, suffix[, start[, end]], /)"
str.endswith as unicode_endswith

    suffix as subobj: object
        A string or a tuple of strings to try.
    start: slice_index(accept={int, NoneType}, c_default='0') = None
        Optional start position. Default: start of the string.
    end: slice_index(accept={int, NoneType}, c_default='PY_SSIZE_T_MAX') = None
        Optional stop position. Default: end of the string.
    /

Return True if the string ends with the specified suffix, False otherwise.
[clinic start generated code]*/

static PyObject *
unicode_endswith_impl(PyObject *self, PyObject *subobj, Py_ssize_t start,
                      Py_ssize_t end)
/*[clinic end generated code: output=cce6f8ceb0102ca9 input=00fbdc774a7d4d71]*/
{
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            PyObject *substring = PyTuple_GET_ITEM(subobj, i);
            if (!PyUnicode_Check(substring)) {
                PyErr_Format(PyExc_TypeError,
                             "tuple for endswith must only contain str, "
                             "not %.100s",
                             Py_TYPE(substring)->tp_name);
                return NULL;
            }
            int result = tailmatch(self, substring, start, end, +1);
            if (result < 0) {
                return NULL;
            }
            if (result) {
                Py_RETURN_TRUE;
            }
        }
        Py_RETURN_FALSE;
    }
    if (!PyUnicode_Check(subobj)) {
        PyErr_Format(PyExc_TypeError,
                     "endswith first arg must be str or "
                     "a tuple of str, not %.100s", Py_TYPE(subobj)->tp_name);
        return NULL;
    }
    int result = tailmatch(self, subobj, start, end, +1);
    if (result < 0) {
        return NULL;
    }
    return PyBool_FromLong(result);
}


static inline void
_PyUnicodeWriter_Update(_PyUnicodeWriter *writer)
{
    writer->maxchar = PyUnicode_MAX_CHAR_VALUE(writer->buffer);
    writer->data = PyUnicode_DATA(writer->buffer);

    if (!writer->readonly) {
        writer->kind = PyUnicode_KIND(writer->buffer);
        writer->size = PyUnicode_GET_LENGTH(writer->buffer);
    }
    else {
        /* use a value smaller than PyUnicode_1BYTE_KIND() so
           _PyUnicodeWriter_PrepareKind() will copy the buffer. */
        writer->kind = 0;
        assert(writer->kind <= PyUnicode_1BYTE_KIND);

        /* Copy-on-write mode: set buffer size to 0 so
         * _PyUnicodeWriter_Prepare() will copy (and enlarge) the buffer on
         * next write. */
        writer->size = 0;
    }
}


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
    if (_PyUnicodeWriter_Prepare(writer, length, 127) < 0) {
        PyUnicodeWriter_Discard(pub_writer);
        return NULL;
    }
    writer->overallocate = 1;

    return pub_writer;
}


void PyUnicodeWriter_Discard(PyUnicodeWriter *writer)
{
    if (writer == NULL) {
        return;
    }
    _PyUnicodeWriter_Dealloc((_PyUnicodeWriter*)writer);
    _Py_FREELIST_FREE(unicode_writers, writer, PyMem_Free);
}


// Initialize _PyUnicodeWriter with initial buffer
static inline void
_PyUnicodeWriter_InitWithBuffer(_PyUnicodeWriter *writer, PyObject *buffer)
{
    memset(writer, 0, sizeof(*writer));
    writer->buffer = buffer;
    _PyUnicodeWriter_Update(writer);
    writer->min_length = writer->size;
}


int
_PyUnicodeWriter_PrepareInternal(_PyUnicodeWriter *writer,
                                 Py_ssize_t length, Py_UCS4 maxchar)
{
    Py_ssize_t newlen;
    PyObject *newbuffer;

    assert(length >= 0);
    assert(maxchar <= MAX_UNICODE);

    /* ensure that the _PyUnicodeWriter_Prepare macro was used */
    assert((maxchar > writer->maxchar && length >= 0)
           || length > 0);

    if (length > PY_SSIZE_T_MAX - writer->pos) {
        PyErr_NoMemory();
        return -1;
    }
    newlen = writer->pos + length;

    maxchar = Py_MAX(maxchar, writer->min_char);

    if (writer->buffer == NULL) {
        assert(!writer->readonly);
        if (writer->overallocate
            && newlen <= (PY_SSIZE_T_MAX - newlen / OVERALLOCATE_FACTOR)) {
            /* overallocate to limit the number of realloc() */
            newlen += newlen / OVERALLOCATE_FACTOR;
        }
        if (newlen < writer->min_length)
            newlen = writer->min_length;

        writer->buffer = PyUnicode_New(newlen, maxchar);
        if (writer->buffer == NULL)
            return -1;
    }
    else if (newlen > writer->size) {
        if (writer->overallocate
            && newlen <= (PY_SSIZE_T_MAX - newlen / OVERALLOCATE_FACTOR)) {
            /* overallocate to limit the number of realloc() */
            newlen += newlen / OVERALLOCATE_FACTOR;
        }
        if (newlen < writer->min_length)
            newlen = writer->min_length;

        if (maxchar > writer->maxchar || writer->readonly) {
            /* resize + widen */
            maxchar = Py_MAX(maxchar, writer->maxchar);
            newbuffer = PyUnicode_New(newlen, maxchar);
            if (newbuffer == NULL)
                return -1;
            _PyUnicode_FastCopyCharacters(newbuffer, 0,
                                          writer->buffer, 0, writer->pos);
            Py_DECREF(writer->buffer);
            writer->readonly = 0;
        }
        else {
            newbuffer = resize_compact(writer->buffer, newlen);
            if (newbuffer == NULL)
                return -1;
        }
        writer->buffer = newbuffer;
    }
    else if (maxchar > writer->maxchar) {
        assert(!writer->readonly);
        newbuffer = PyUnicode_New(writer->size, maxchar);
        if (newbuffer == NULL)
            return -1;
        _PyUnicode_FastCopyCharacters(newbuffer, 0,
                                      writer->buffer, 0, writer->pos);
        Py_SETREF(writer->buffer, newbuffer);
    }
    _PyUnicodeWriter_Update(writer);
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
    case PyUnicode_4BYTE_KIND: maxchar = MAX_UNICODE; break;
    default:
        Py_UNREACHABLE();
    }

    return _PyUnicodeWriter_PrepareInternal(writer, 0, maxchar);
}

static inline int
_PyUnicodeWriter_WriteCharInline(_PyUnicodeWriter *writer, Py_UCS4 ch)
{
    assert(ch <= MAX_UNICODE);
    if (_PyUnicodeWriter_Prepare(writer, 1, ch) < 0)
        return -1;
    PyUnicode_WRITE(writer->kind, writer->data, writer->pos, ch);
    writer->pos++;
    return 0;
}

int
_PyUnicodeWriter_WriteChar(_PyUnicodeWriter *writer, Py_UCS4 ch)
{
    return _PyUnicodeWriter_WriteCharInline(writer, ch);
}

int
PyUnicodeWriter_WriteChar(PyUnicodeWriter *writer, Py_UCS4 ch)
{
    if (ch > MAX_UNICODE) {
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

    Py_UCS4 maxchar;
    Py_ssize_t len;

    len = PyUnicode_GET_LENGTH(str);
    if (len == 0)
        return 0;
    maxchar = PyUnicode_MAX_CHAR_VALUE(str);
    if (maxchar > writer->maxchar || len > writer->size - writer->pos) {
        if (writer->buffer == NULL && !writer->overallocate) {
            assert(_PyUnicode_CheckConsistency(str, 1));
            writer->readonly = 1;
            writer->buffer = Py_NewRef(str);
            _PyUnicodeWriter_Update(writer);
            writer->pos += len;
            return 0;
        }
        if (_PyUnicodeWriter_PrepareInternal(writer, len, maxchar) == -1)
            return -1;
    }
    _PyUnicode_FastCopyCharacters(writer->buffer, writer->pos,
                                  str, 0, len);
    writer->pos += len;
    return 0;
}

int
PyUnicodeWriter_WriteStr(PyUnicodeWriter *writer, PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (type == &PyUnicode_Type) {
        return _PyUnicodeWriter_WriteStr((_PyUnicodeWriter*)writer, obj);
    }

    if (type == &PyLong_Type) {
        return _PyLong_FormatWriter((_PyUnicodeWriter*)writer, obj, 10, 0);
    }

    PyObject *str = PyObject_Str(obj);
    if (str == NULL) {
        return -1;
    }

    int res = _PyUnicodeWriter_WriteStr((_PyUnicodeWriter*)writer, str);
    Py_DECREF(str);
    return res;
}


int
PyUnicodeWriter_WriteRepr(PyUnicodeWriter *writer, PyObject *obj)
{
    if (Py_TYPE(obj) == &PyLong_Type) {
        return _PyLong_FormatWriter((_PyUnicodeWriter*)writer, obj, 10, 0);
    }

    PyObject *repr = PyObject_Repr(obj);
    if (repr == NULL) {
        return -1;
    }

    int res = _PyUnicodeWriter_WriteStr((_PyUnicodeWriter*)writer, repr);
    Py_DECREF(repr);
    return res;
}


int
_PyUnicodeWriter_WriteSubstring(_PyUnicodeWriter *writer, PyObject *str,
                                Py_ssize_t start, Py_ssize_t end)
{
    assert(0 <= start);
    assert(end <= PyUnicode_GET_LENGTH(str));
    assert(start <= end);

    if (start == 0 && end == PyUnicode_GET_LENGTH(str))
        return _PyUnicodeWriter_WriteStr(writer, str);

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

    assert(ucs1lib_find_max_char((const Py_UCS1*)ascii, (const Py_UCS1*)ascii + len) < 128);

    if (writer->buffer == NULL && !writer->overallocate) {
        PyObject *str;

        str = _PyUnicode_FromASCII(ascii, len);
        if (str == NULL)
            return -1;

        writer->readonly = 1;
        writer->buffer = str;
        _PyUnicodeWriter_Update(writer);
        writer->pos += len;
        return 0;
    }

    if (_PyUnicodeWriter_Prepare(writer, len, 127) == -1)
        return -1;

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

    _PyUnicodeWriter *priv_writer = (_PyUnicodeWriter*)writer;
    return _PyUnicodeWriter_WriteASCIIString(priv_writer, str, size);
}


int
PyUnicodeWriter_WriteUTF8(PyUnicodeWriter *writer,
                          const char *str,
                          Py_ssize_t size)
{
    if (size < 0) {
        size = strlen(str);
    }

    _PyUnicodeWriter *_writer = (_PyUnicodeWriter*)writer;
    Py_ssize_t old_pos = _writer->pos;
    int res = unicode_decode_utf8_writer(_writer, str, size,
                                         _Py_ERROR_STRICT, NULL, NULL);
    if (res < 0) {
        _writer->pos = old_pos;
    }
    return res;
}


int
PyUnicodeWriter_DecodeUTF8Stateful(PyUnicodeWriter *writer,
                                   const char *string,
                                   Py_ssize_t length,
                                   const char *errors,
                                   Py_ssize_t *consumed)
{
    if (length < 0) {
        length = strlen(string);
    }

    _PyUnicodeWriter *_writer = (_PyUnicodeWriter*)writer;
    Py_ssize_t old_pos = _writer->pos;
    int res = unicode_decode_utf8_writer(_writer, string, length,
                                         _Py_ERROR_UNKNOWN, errors, consumed);
    if (res < 0) {
        _writer->pos = old_pos;
        if (consumed) {
            *consumed = 0;
        }
    }
    return res;
}


int
_PyUnicodeWriter_WriteLatin1String(_PyUnicodeWriter *writer,
                                   const char *str, Py_ssize_t len)
{
    Py_UCS4 maxchar;

    maxchar = ucs1lib_find_max_char((const Py_UCS1*)str, (const Py_UCS1*)str + len);
    if (_PyUnicodeWriter_Prepare(writer, len, maxchar) == -1)
        return -1;
    unicode_write_cstr(writer->buffer, writer->pos, str, len);
    writer->pos += len;
    return 0;
}

PyObject *
_PyUnicodeWriter_Finish(_PyUnicodeWriter *writer)
{
    PyObject *str;

    if (writer->pos == 0) {
        Py_CLEAR(writer->buffer);
        _Py_RETURN_UNICODE_EMPTY();
    }

    str = writer->buffer;
    writer->buffer = NULL;

    if (writer->readonly) {
        assert(PyUnicode_GET_LENGTH(str) == writer->pos);
        return str;
    }

    if (PyUnicode_GET_LENGTH(str) != writer->pos) {
        PyObject *str2;
        str2 = resize_compact(str, writer->pos);
        if (str2 == NULL) {
            Py_DECREF(str);
            return NULL;
        }
        str = str2;
    }

    assert(_PyUnicode_CheckConsistency(str, 1));
    return unicode_result(str);
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

#include "stringlib/unicode_format.h"

PyDoc_STRVAR(format__doc__,
             "format($self, /, *args, **kwargs)\n\
--\n\
\n\
Return a formatted version of the string, using substitutions from args and kwargs.\n\
The substitutions are identified by braces ('{' and '}').");

PyDoc_STRVAR(format_map__doc__,
             "format_map($self, mapping, /)\n\
--\n\
\n\
Return a formatted version of the string, using substitutions from mapping.\n\
The substitutions are identified by braces ('{' and '}').");

/*[clinic input]
str.__format__ as unicode___format__

    format_spec: unicode
    /

Return a formatted version of the string as described by format_spec.
[clinic start generated code]*/

static PyObject *
unicode___format___impl(PyObject *self, PyObject *format_spec)
/*[clinic end generated code: output=45fceaca6d2ba4c8 input=5e135645d167a214]*/
{
    _PyUnicodeWriter writer;
    int ret;

    _PyUnicodeWriter_Init(&writer);
    ret = _PyUnicode_FormatAdvancedWriter(&writer,
                                          self, format_spec, 0,
                                          PyUnicode_GET_LENGTH(format_spec));
    if (ret == -1) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyUnicodeWriter_Finish(&writer);
}

/*[clinic input]
str.__sizeof__ as unicode_sizeof

Return the size of the string in memory, in bytes.
[clinic start generated code]*/

static PyObject *
unicode_sizeof_impl(PyObject *self)
/*[clinic end generated code: output=6dbc2f5a408b6d4f input=6dd011c108e33fb0]*/
{
    Py_ssize_t size;

    /* If it's a compact object, account for base structure +
       character data. */
    if (PyUnicode_IS_COMPACT_ASCII(self)) {
        size = sizeof(PyASCIIObject) + PyUnicode_GET_LENGTH(self) + 1;
    }
    else if (PyUnicode_IS_COMPACT(self)) {
        size = sizeof(PyCompactUnicodeObject) +
            (PyUnicode_GET_LENGTH(self) + 1) * PyUnicode_KIND(self);
    }
    else {
        /* If it is a two-block object, account for base object, and
           for character block if present. */
        size = sizeof(PyUnicodeObject);
        if (_PyUnicode_DATA_ANY(self))
            size += (PyUnicode_GET_LENGTH(self) + 1) *
                PyUnicode_KIND(self);
    }
    if (_PyUnicode_HAS_UTF8_MEMORY(self))
        size += PyUnicode_UTF8_LENGTH(self) + 1;

    return PyLong_FromSsize_t(size);
}

static PyObject *
unicode_getnewargs(PyObject *v, PyObject *Py_UNUSED(ignored))
{
    PyObject *copy = _PyUnicode_Copy(v);
    if (!copy)
        return NULL;
    return Py_BuildValue("(N)", copy);
}

/*
This function searchs the longest common leading whitespace
of all lines in the [src, end).
It returns the length of the common leading whitespace and sets `output` to
point to the beginning of the common leading whitespace if length > 0.
*/
static Py_ssize_t
search_longest_common_leading_whitespace(
    const char *const src,
    const char *const end,
    const char **output)
{
    // [_start, _start + _len)
    // describes the current longest common leading whitespace
    const char *_start = NULL;
    Py_ssize_t _len = 0;

    for (const char *iter = src; iter < end; ++iter) {
        const char *line_start = iter;
        const char *leading_whitespace_end = NULL;

        // scan the whole line
        while (iter < end && *iter != '\n') {
            if (!leading_whitespace_end && *iter != ' ' && *iter != '\t') {
                /* `iter` points to the first non-whitespace character
                   in this line */
                if (iter == line_start) {
                    // some line has no indent, fast exit!
                    return 0;
                }
                leading_whitespace_end = iter;
            }
            ++iter;
        }

        // if this line has all white space, skip it
        if (!leading_whitespace_end) {
            continue;
        }

        if (!_start) {
            // update the first leading whitespace
            _start = line_start;
            _len = leading_whitespace_end - line_start;
            assert(_len > 0);
        }
        else {
            /* We then compare with the current longest leading whitespace.

               [line_start, leading_whitespace_end) is the leading
               whitespace of this line,

               [_start, _start + _len) is the leading whitespace of the
               current longest leading whitespace. */
            Py_ssize_t new_len = 0;
            const char *_iter = _start, *line_iter = line_start;

            while (_iter < _start + _len && line_iter < leading_whitespace_end
                   && *_iter == *line_iter)
            {
                ++_iter;
                ++line_iter;
                ++new_len;
            }

            _len = new_len;
            if (_len == 0) {
                // No common things now, fast exit!
                return 0;
            }
        }
    }

    assert(_len >= 0);
    if (_len > 0) {
        *output = _start;
    }
    return _len;
}

/* Dedent a string.
   Behaviour is expected to be an exact match of `textwrap.dedent`.
   Return a new reference on success, NULL with exception set on error.
   */
PyObject *
_PyUnicode_Dedent(PyObject *unicode)
{
    Py_ssize_t src_len = 0;
    const char *src = PyUnicode_AsUTF8AndSize(unicode, &src_len);
    if (!src) {
        return NULL;
    }
    assert(src_len >= 0);
    if (src_len == 0) {
        return Py_NewRef(unicode);
    }

    const char *const end = src + src_len;

    // [whitespace_start, whitespace_start + whitespace_len)
    // describes the current longest common leading whitespace
    const char *whitespace_start = NULL;
    Py_ssize_t whitespace_len = search_longest_common_leading_whitespace(
        src, end, &whitespace_start);

    if (whitespace_len == 0) {
        return Py_NewRef(unicode);
    }

    // now we should trigger a dedent
    char *dest = PyMem_Malloc(src_len);
    if (!dest) {
        PyErr_NoMemory();
        return NULL;
    }
    char *dest_iter = dest;

    for (const char *iter = src; iter < end; ++iter) {
        const char *line_start = iter;
        bool in_leading_space = true;

        // iterate over a line to find the end of a line
        while (iter < end && *iter != '\n') {
            if (in_leading_space && *iter != ' ' && *iter != '\t') {
                in_leading_space = false;
            }
            ++iter;
        }

        // invariant: *iter == '\n' or iter == end
        bool append_newline = iter < end;

        // if this line has all white space, write '\n' and continue
        if (in_leading_space && append_newline) {
            *dest_iter++ = '\n';
            continue;
        }

        /* copy [new_line_start + whitespace_len, iter) to buffer, then
            conditionally append '\n' */

        Py_ssize_t new_line_len = iter - line_start - whitespace_len;
        assert(new_line_len >= 0);
        memcpy(dest_iter, line_start + whitespace_len, new_line_len);

        dest_iter += new_line_len;

        if (append_newline) {
            *dest_iter++ = '\n';
        }
    }

    PyObject *res = PyUnicode_FromStringAndSize(dest, dest_iter - dest);
    PyMem_Free(dest);
    return res;
}

static PyMethodDef unicode_methods[] = {
    UNICODE_ENCODE_METHODDEF
    UNICODE_REPLACE_METHODDEF
    UNICODE_SPLIT_METHODDEF
    UNICODE_RSPLIT_METHODDEF
    UNICODE_JOIN_METHODDEF
    UNICODE_CAPITALIZE_METHODDEF
    UNICODE_CASEFOLD_METHODDEF
    UNICODE_TITLE_METHODDEF
    UNICODE_CENTER_METHODDEF
    UNICODE_COUNT_METHODDEF
    UNICODE_EXPANDTABS_METHODDEF
    UNICODE_FIND_METHODDEF
    UNICODE_PARTITION_METHODDEF
    UNICODE_INDEX_METHODDEF
    UNICODE_LJUST_METHODDEF
    UNICODE_LOWER_METHODDEF
    UNICODE_LSTRIP_METHODDEF
    UNICODE_RFIND_METHODDEF
    UNICODE_RINDEX_METHODDEF
    UNICODE_RJUST_METHODDEF
    UNICODE_RSTRIP_METHODDEF
    UNICODE_RPARTITION_METHODDEF
    UNICODE_SPLITLINES_METHODDEF
    UNICODE_STRIP_METHODDEF
    UNICODE_SWAPCASE_METHODDEF
    UNICODE_TRANSLATE_METHODDEF
    UNICODE_UPPER_METHODDEF
    UNICODE_STARTSWITH_METHODDEF
    UNICODE_ENDSWITH_METHODDEF
    UNICODE_REMOVEPREFIX_METHODDEF
    UNICODE_REMOVESUFFIX_METHODDEF
    UNICODE_ISASCII_METHODDEF
    UNICODE_ISLOWER_METHODDEF
    UNICODE_ISUPPER_METHODDEF
    UNICODE_ISTITLE_METHODDEF
    UNICODE_ISSPACE_METHODDEF
    UNICODE_ISDECIMAL_METHODDEF
    UNICODE_ISDIGIT_METHODDEF
    UNICODE_ISNUMERIC_METHODDEF
    UNICODE_ISALPHA_METHODDEF
    UNICODE_ISALNUM_METHODDEF
    UNICODE_ISIDENTIFIER_METHODDEF
    UNICODE_ISPRINTABLE_METHODDEF
    UNICODE_ZFILL_METHODDEF
    {"format", _PyCFunction_CAST(do_string_format), METH_VARARGS | METH_KEYWORDS, format__doc__},
    {"format_map", do_string_format_map, METH_O, format_map__doc__},
    UNICODE___FORMAT___METHODDEF
    UNICODE_MAKETRANS_METHODDEF
    UNICODE_SIZEOF_METHODDEF
    {"__getnewargs__",  unicode_getnewargs, METH_NOARGS},
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
    unicode_length,     /* sq_length */
    PyUnicode_Concat,   /* sq_concat */
    unicode_repeat,     /* sq_repeat */
    unicode_getitem,    /* sq_item */
    0,                  /* sq_slice */
    0,                  /* sq_ass_item */
    0,                  /* sq_ass_slice */
    PyUnicode_Contains, /* sq_contains */
};

static PyObject*
unicode_subscript(PyObject* self, PyObject* item)
{
    if (_PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyUnicode_GET_LENGTH(self);
        return unicode_getitem(self, i);
    } else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, i;
        size_t cur;
        PyObject *result;
        const void *src_data;
        void *dest_data;
        int src_kind, dest_kind;
        Py_UCS4 ch, max_char, kind_limit;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(PyUnicode_GET_LENGTH(self),
                                            &start, &stop, step);

        if (slicelength <= 0) {
            _Py_RETURN_UNICODE_EMPTY();
        } else if (start == 0 && step == 1 &&
                   slicelength == PyUnicode_GET_LENGTH(self)) {
            return unicode_result_unchanged(self);
        } else if (step == 1) {
            return PyUnicode_Substring(self,
                                       start, start + slicelength);
        }
        /* General case */
        src_kind = PyUnicode_KIND(self);
        src_data = PyUnicode_DATA(self);
        if (!PyUnicode_IS_ASCII(self)) {
            kind_limit = kind_maxchar_limit(src_kind);
            max_char = 0;
            for (cur = start, i = 0; i < slicelength; cur += step, i++) {
                ch = PyUnicode_READ(src_kind, src_data, cur);
                if (ch > max_char) {
                    max_char = ch;
                    if (max_char >= kind_limit)
                        break;
                }
            }
        }
        else
            max_char = 127;
        result = PyUnicode_New(slicelength, max_char);
        if (result == NULL)
            return NULL;
        dest_kind = PyUnicode_KIND(result);
        dest_data = PyUnicode_DATA(result);

        for (cur = start, i = 0; i < slicelength; cur += step, i++) {
            Py_UCS4 ch = PyUnicode_READ(src_kind, src_data, cur);
            PyUnicode_WRITE(dest_kind, dest_data, i, ch);
        }
        assert(_PyUnicode_CheckConsistency(result, 1));
        return result;
    } else {
        PyErr_Format(PyExc_TypeError, "string indices must be integers, not '%.200s'",
                     Py_TYPE(item)->tp_name);
        return NULL;
    }
}

static PyMappingMethods unicode_as_mapping = {
    unicode_length,     /* mp_length */
    unicode_subscript,  /* mp_subscript */
    0,                  /* mp_ass_subscript */
};


/* Helpers for PyUnicode_Format() */

struct unicode_formatter_t {
    PyObject *args;
    int args_owned;
    Py_ssize_t arglen, argidx;
    PyObject *dict;

    int fmtkind;
    Py_ssize_t fmtcnt, fmtpos;
    const void *fmtdata;
    PyObject *fmtstr;

    _PyUnicodeWriter writer;
};

struct unicode_format_arg_t {
    Py_UCS4 ch;
    int flags;
    Py_ssize_t width;
    int prec;
    int sign;
};

static PyObject *
unicode_format_getnextarg(struct unicode_formatter_t *ctx)
{
    Py_ssize_t argidx = ctx->argidx;

    if (argidx < ctx->arglen) {
        ctx->argidx++;
        if (ctx->arglen < 0)
            return ctx->args;
        else
            return PyTuple_GetItem(ctx->args, argidx);
    }
    PyErr_SetString(PyExc_TypeError,
                    "not enough arguments for format string");
    return NULL;
}

/* Returns a new reference to a PyUnicode object, or NULL on failure. */

/* Format a float into the writer if the writer is not NULL, or into *p_output
   otherwise.

   Return 0 on success, raise an exception and return -1 on error. */
static int
formatfloat(PyObject *v, struct unicode_format_arg_t *arg,
            PyObject **p_output,
            _PyUnicodeWriter *writer)
{
    char *p;
    double x;
    Py_ssize_t len;
    int prec;
    int dtoa_flags = 0;

    x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred())
        return -1;

    prec = arg->prec;
    if (prec < 0)
        prec = 6;

    if (arg->flags & F_ALT)
        dtoa_flags |= Py_DTSF_ALT;
    p = PyOS_double_to_string(x, arg->ch, prec, dtoa_flags, NULL);
    if (p == NULL)
        return -1;
    len = strlen(p);
    if (writer) {
        if (_PyUnicodeWriter_WriteASCIIString(writer, p, len) < 0) {
            PyMem_Free(p);
            return -1;
        }
    }
    else
        *p_output = _PyUnicode_FromASCII(p, len);
    PyMem_Free(p);
    return 0;
}

/* formatlong() emulates the format codes d, u, o, x and X, and
 * the F_ALT flag, for Python's long (unbounded) ints.  It's not used for
 * Python's regular ints.
 * Return value:  a new PyUnicodeObject*, or NULL if error.
 *     The output string is of the form
 *         "-"? ("0x" | "0X")? digit+
 *     "0x"/"0X" are present only for x and X conversions, with F_ALT
 *         set in flags.  The case of hex digits will be correct,
 *     There will be at least prec digits, zero-filled on the left if
 *         necessary to get that many.
 * val          object to be converted
 * flags        bitmask of format flags; only F_ALT is looked at
 * prec         minimum number of digits; 0-fill on left if needed
 * type         a character in [duoxX]; u acts the same as d
 *
 * CAUTION:  o, x and X conversions on regular ints can never
 * produce a '-' sign, but can for Python's unbounded ints.
 */
PyObject *
_PyUnicode_FormatLong(PyObject *val, int alt, int prec, int type)
{
    PyObject *result = NULL;
    char *buf;
    Py_ssize_t i;
    int sign;           /* 1 if '-', else 0 */
    int len;            /* number of characters */
    Py_ssize_t llen;
    int numdigits;      /* len == numnondigits + numdigits */
    int numnondigits = 0;

    /* Avoid exceeding SSIZE_T_MAX */
    if (prec > INT_MAX-3) {
        PyErr_SetString(PyExc_OverflowError,
                        "precision too large");
        return NULL;
    }

    assert(PyLong_Check(val));

    switch (type) {
    default:
        Py_UNREACHABLE();
    case 'd':
    case 'i':
    case 'u':
        /* int and int subclasses should print numerically when a numeric */
        /* format code is used (see issue18780) */
        result = PyNumber_ToBase(val, 10);
        break;
    case 'o':
        numnondigits = 2;
        result = PyNumber_ToBase(val, 8);
        break;
    case 'x':
    case 'X':
        numnondigits = 2;
        result = PyNumber_ToBase(val, 16);
        break;
    }
    if (!result)
        return NULL;

    assert(unicode_modifiable(result));
    assert(PyUnicode_IS_ASCII(result));

    /* To modify the string in-place, there can only be one reference. */
    if (!_PyObject_IsUniquelyReferenced(result)) {
        Py_DECREF(result);
        PyErr_BadInternalCall();
        return NULL;
    }
    buf = PyUnicode_DATA(result);
    llen = PyUnicode_GET_LENGTH(result);
    if (llen > INT_MAX) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_ValueError,
                        "string too large in _PyUnicode_FormatLong");
        return NULL;
    }
    len = (int)llen;
    sign = buf[0] == '-';
    numnondigits += sign;
    numdigits = len - numnondigits;
    assert(numdigits > 0);

    /* Get rid of base marker unless F_ALT */
    if (((alt) == 0 &&
        (type == 'o' || type == 'x' || type == 'X'))) {
        assert(buf[sign] == '0');
        assert(buf[sign+1] == 'x' || buf[sign+1] == 'X' ||
               buf[sign+1] == 'o');
        numnondigits -= 2;
        buf += 2;
        len -= 2;
        if (sign)
            buf[0] = '-';
        assert(len == numnondigits + numdigits);
        assert(numdigits > 0);
    }

    /* Fill with leading zeroes to meet minimum width. */
    if (prec > numdigits) {
        PyObject *r1 = PyBytes_FromStringAndSize(NULL,
                                numnondigits + prec);
        char *b1;
        if (!r1) {
            Py_DECREF(result);
            return NULL;
        }
        b1 = PyBytes_AS_STRING(r1);
        for (i = 0; i < numnondigits; ++i)
            *b1++ = *buf++;
        for (i = 0; i < prec - numdigits; i++)
            *b1++ = '0';
        for (i = 0; i < numdigits; i++)
            *b1++ = *buf++;
        *b1 = '\0';
        Py_SETREF(result, r1);
        buf = PyBytes_AS_STRING(result);
        len = numnondigits + prec;
    }

    /* Fix up case for hex conversions. */
    if (type == 'X') {
        /* Need to convert all lower case letters to upper case.
           and need to convert 0x to 0X (and -0x to -0X). */
        for (i = 0; i < len; i++)
            if (buf[i] >= 'a' && buf[i] <= 'x')
                buf[i] -= 'a'-'A';
    }
    if (!PyUnicode_Check(result)
        || buf != PyUnicode_DATA(result)) {
        PyObject *unicode;
        unicode = _PyUnicode_FromASCII(buf, len);
        Py_SETREF(result, unicode);
    }
    else if (len != PyUnicode_GET_LENGTH(result)) {
        if (PyUnicode_Resize(&result, len) < 0)
            Py_CLEAR(result);
    }
    return result;
}

/* Format an integer or a float as an integer.
 * Return 1 if the number has been formatted into the writer,
 *        0 if the number has been formatted into *p_output
 *       -1 and raise an exception on error */
static int
mainformatlong(PyObject *v,
               struct unicode_format_arg_t *arg,
               PyObject **p_output,
               _PyUnicodeWriter *writer)
{
    PyObject *iobj, *res;
    char type = (char)arg->ch;

    if (!PyNumber_Check(v))
        goto wrongtype;

    /* make sure number is a type of integer for o, x, and X */
    if (!PyLong_Check(v)) {
        if (type == 'o' || type == 'x' || type == 'X') {
            iobj = _PyNumber_Index(v);
        }
        else {
            iobj = PyNumber_Long(v);
        }
        if (iobj == NULL ) {
            if (PyErr_ExceptionMatches(PyExc_TypeError))
                goto wrongtype;
            return -1;
        }
        assert(PyLong_Check(iobj));
    }
    else {
        iobj = Py_NewRef(v);
    }

    if (PyLong_CheckExact(v)
        && arg->width == -1 && arg->prec == -1
        && !(arg->flags & (F_SIGN | F_BLANK))
        && type != 'X')
    {
        /* Fast path */
        int alternate = arg->flags & F_ALT;
        int base;

        switch(type)
        {
            default:
                Py_UNREACHABLE();
            case 'd':
            case 'i':
            case 'u':
                base = 10;
                break;
            case 'o':
                base = 8;
                break;
            case 'x':
            case 'X':
                base = 16;
                break;
        }

        if (_PyLong_FormatWriter(writer, v, base, alternate) == -1) {
            Py_DECREF(iobj);
            return -1;
        }
        Py_DECREF(iobj);
        return 1;
    }

    res = _PyUnicode_FormatLong(iobj, arg->flags & F_ALT, arg->prec, type);
    Py_DECREF(iobj);
    if (res == NULL)
        return -1;
    *p_output = res;
    return 0;

wrongtype:
    switch(type)
    {
        case 'o':
        case 'x':
        case 'X':
            PyErr_Format(PyExc_TypeError,
                    "%%%c format: an integer is required, "
                    "not %.200s",
                    type, Py_TYPE(v)->tp_name);
            break;
        default:
            PyErr_Format(PyExc_TypeError,
                    "%%%c format: a real number is required, "
                    "not %.200s",
                    type, Py_TYPE(v)->tp_name);
            break;
    }
    return -1;
}

static Py_UCS4
formatchar(PyObject *v)
{
    /* presume that the buffer is at least 3 characters long */
    if (PyUnicode_Check(v)) {
        if (PyUnicode_GET_LENGTH(v) == 1) {
            return PyUnicode_READ_CHAR(v, 0);
        }
        PyErr_Format(PyExc_TypeError,
                     "%%c requires an int or a unicode character, "
                     "not a string of length %zd",
                     PyUnicode_GET_LENGTH(v));
        return (Py_UCS4) -1;
    }
    else {
        int overflow;
        long x = PyLong_AsLongAndOverflow(v, &overflow);
        if (x == -1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_Format(PyExc_TypeError,
                             "%%c requires an int or a unicode character, not %T",
                             v);
                return (Py_UCS4) -1;
            }
            return (Py_UCS4) -1;
        }

        if (x < 0 || x > MAX_UNICODE) {
            /* this includes an overflow in converting to C long */
            PyErr_SetString(PyExc_OverflowError,
                            "%c arg not in range(0x110000)");
            return (Py_UCS4) -1;
        }

        return (Py_UCS4) x;
    }
}

/* Parse options of an argument: flags, width, precision.
   Handle also "%(name)" syntax.

   Return 0 if the argument has been formatted into arg->str.
   Return 1 if the argument has been written into ctx->writer,
   Raise an exception and return -1 on error. */
static int
unicode_format_arg_parse(struct unicode_formatter_t *ctx,
                         struct unicode_format_arg_t *arg)
{
#define FORMAT_READ(ctx) \
        PyUnicode_READ((ctx)->fmtkind, (ctx)->fmtdata, (ctx)->fmtpos)

    PyObject *v;

    if (arg->ch == '(') {
        /* Get argument value from a dictionary. Example: "%(name)s". */
        Py_ssize_t keystart;
        Py_ssize_t keylen;
        PyObject *key;
        int pcount = 1;

        if (ctx->dict == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "format requires a mapping");
            return -1;
        }
        ++ctx->fmtpos;
        --ctx->fmtcnt;
        keystart = ctx->fmtpos;
        /* Skip over balanced parentheses */
        while (pcount > 0 && --ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            if (arg->ch == ')')
                --pcount;
            else if (arg->ch == '(')
                ++pcount;
            ctx->fmtpos++;
        }
        keylen = ctx->fmtpos - keystart - 1;
        if (ctx->fmtcnt < 0 || pcount > 0) {
            PyErr_SetString(PyExc_ValueError,
                            "incomplete format key");
            return -1;
        }
        key = PyUnicode_Substring(ctx->fmtstr,
                                  keystart, keystart + keylen);
        if (key == NULL)
            return -1;
        if (ctx->args_owned) {
            ctx->args_owned = 0;
            Py_DECREF(ctx->args);
        }
        ctx->args = PyObject_GetItem(ctx->dict, key);
        Py_DECREF(key);
        if (ctx->args == NULL)
            return -1;
        ctx->args_owned = 1;
        ctx->arglen = -1;
        ctx->argidx = -2;
    }

    /* Parse flags. Example: "%+i" => flags=F_SIGN. */
    while (--ctx->fmtcnt >= 0) {
        arg->ch = FORMAT_READ(ctx);
        ctx->fmtpos++;
        switch (arg->ch) {
        case '-': arg->flags |= F_LJUST; continue;
        case '+': arg->flags |= F_SIGN; continue;
        case ' ': arg->flags |= F_BLANK; continue;
        case '#': arg->flags |= F_ALT; continue;
        case '0': arg->flags |= F_ZERO; continue;
        }
        break;
    }

    /* Parse width. Example: "%10s" => width=10 */
    if (arg->ch == '*') {
        v = unicode_format_getnextarg(ctx);
        if (v == NULL)
            return -1;
        if (!PyLong_Check(v)) {
            PyErr_SetString(PyExc_TypeError,
                            "* wants int");
            return -1;
        }
        arg->width = PyLong_AsSsize_t(v);
        if (arg->width == -1 && PyErr_Occurred())
            return -1;
        if (arg->width < 0) {
            arg->flags |= F_LJUST;
            arg->width = -arg->width;
        }
        if (--ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            ctx->fmtpos++;
        }
    }
    else if (arg->ch >= '0' && arg->ch <= '9') {
        arg->width = arg->ch - '0';
        while (--ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            ctx->fmtpos++;
            if (arg->ch < '0' || arg->ch > '9')
                break;
            /* Since arg->ch is unsigned, the RHS would end up as unsigned,
               mixing signed and unsigned comparison. Since arg->ch is between
               '0' and '9', casting to int is safe. */
            if (arg->width > (PY_SSIZE_T_MAX - ((int)arg->ch - '0')) / 10) {
                PyErr_SetString(PyExc_ValueError,
                                "width too big");
                return -1;
            }
            arg->width = arg->width*10 + (arg->ch - '0');
        }
    }

    /* Parse precision. Example: "%.3f" => prec=3 */
    if (arg->ch == '.') {
        arg->prec = 0;
        if (--ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            ctx->fmtpos++;
        }
        if (arg->ch == '*') {
            v = unicode_format_getnextarg(ctx);
            if (v == NULL)
                return -1;
            if (!PyLong_Check(v)) {
                PyErr_SetString(PyExc_TypeError,
                                "* wants int");
                return -1;
            }
            arg->prec = PyLong_AsInt(v);
            if (arg->prec == -1 && PyErr_Occurred())
                return -1;
            if (arg->prec < 0)
                arg->prec = 0;
            if (--ctx->fmtcnt >= 0) {
                arg->ch = FORMAT_READ(ctx);
                ctx->fmtpos++;
            }
        }
        else if (arg->ch >= '0' && arg->ch <= '9') {
            arg->prec = arg->ch - '0';
            while (--ctx->fmtcnt >= 0) {
                arg->ch = FORMAT_READ(ctx);
                ctx->fmtpos++;
                if (arg->ch < '0' || arg->ch > '9')
                    break;
                if (arg->prec > (INT_MAX - ((int)arg->ch - '0')) / 10) {
                    PyErr_SetString(PyExc_ValueError,
                                    "precision too big");
                    return -1;
                }
                arg->prec = arg->prec*10 + (arg->ch - '0');
            }
        }
    }

    /* Ignore "h", "l" and "L" format prefix (ex: "%hi" or "%ls") */
    if (ctx->fmtcnt >= 0) {
        if (arg->ch == 'h' || arg->ch == 'l' || arg->ch == 'L') {
            if (--ctx->fmtcnt >= 0) {
                arg->ch = FORMAT_READ(ctx);
                ctx->fmtpos++;
            }
        }
    }
    if (ctx->fmtcnt < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "incomplete format");
        return -1;
    }
    return 0;

#undef FORMAT_READ
}

/* Format one argument. Supported conversion specifiers:

   - "s", "r", "a": any type
   - "i", "d", "u": int or float
   - "o", "x", "X": int
   - "e", "E", "f", "F", "g", "G": float
   - "c": int or str (1 character)

   When possible, the output is written directly into the Unicode writer
   (ctx->writer). A string is created when padding is required.

   Return 0 if the argument has been formatted into *p_str,
          1 if the argument has been written into ctx->writer,
         -1 on error. */
static int
unicode_format_arg_format(struct unicode_formatter_t *ctx,
                          struct unicode_format_arg_t *arg,
                          PyObject **p_str)
{
    PyObject *v;
    _PyUnicodeWriter *writer = &ctx->writer;

    if (ctx->fmtcnt == 0)
        ctx->writer.overallocate = 0;

    v = unicode_format_getnextarg(ctx);
    if (v == NULL)
        return -1;


    switch (arg->ch) {
    case 's':
    case 'r':
    case 'a':
        if (PyLong_CheckExact(v) && arg->width == -1 && arg->prec == -1) {
            /* Fast path */
            if (_PyLong_FormatWriter(writer, v, 10, arg->flags & F_ALT) == -1)
                return -1;
            return 1;
        }

        if (PyUnicode_CheckExact(v) && arg->ch == 's') {
            *p_str = Py_NewRef(v);
        }
        else {
            if (arg->ch == 's')
                *p_str = PyObject_Str(v);
            else if (arg->ch == 'r')
                *p_str = PyObject_Repr(v);
            else
                *p_str = PyObject_ASCII(v);
        }
        break;

    case 'i':
    case 'd':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
    {
        int ret = mainformatlong(v, arg, p_str, writer);
        if (ret != 0)
            return ret;
        arg->sign = 1;
        break;
    }

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
        if (arg->width == -1 && arg->prec == -1
            && !(arg->flags & (F_SIGN | F_BLANK)))
        {
            /* Fast path */
            if (formatfloat(v, arg, NULL, writer) == -1)
                return -1;
            return 1;
        }

        arg->sign = 1;
        if (formatfloat(v, arg, p_str, NULL) == -1)
            return -1;
        break;

    case 'c':
    {
        Py_UCS4 ch = formatchar(v);
        if (ch == (Py_UCS4) -1)
            return -1;
        if (arg->width == -1 && arg->prec == -1) {
            /* Fast path */
            if (_PyUnicodeWriter_WriteCharInline(writer, ch) < 0)
                return -1;
            return 1;
        }
        *p_str = PyUnicode_FromOrdinal(ch);
        break;
    }

    default:
        PyErr_Format(PyExc_ValueError,
                     "unsupported format character '%c' (0x%x) "
                     "at index %zd",
                     (31<=arg->ch && arg->ch<=126) ? (char)arg->ch : '?',
                     (int)arg->ch,
                     ctx->fmtpos - 1);
        return -1;
    }
    if (*p_str == NULL)
        return -1;
    assert (PyUnicode_Check(*p_str));
    return 0;
}

static int
unicode_format_arg_output(struct unicode_formatter_t *ctx,
                          struct unicode_format_arg_t *arg,
                          PyObject *str)
{
    Py_ssize_t len;
    int kind;
    const void *pbuf;
    Py_ssize_t pindex;
    Py_UCS4 signchar;
    Py_ssize_t buflen;
    Py_UCS4 maxchar;
    Py_ssize_t sublen;
    _PyUnicodeWriter *writer = &ctx->writer;
    Py_UCS4 fill;

    fill = ' ';
    if (arg->sign && arg->flags & F_ZERO)
        fill = '0';

    len = PyUnicode_GET_LENGTH(str);
    if ((arg->width == -1 || arg->width <= len)
        && (arg->prec == -1 || arg->prec >= len)
        && !(arg->flags & (F_SIGN | F_BLANK)))
    {
        /* Fast path */
        if (_PyUnicodeWriter_WriteStr(writer, str) == -1)
            return -1;
        return 0;
    }

    /* Truncate the string for "s", "r" and "a" formats
       if the precision is set */
    if (arg->ch == 's' || arg->ch == 'r' || arg->ch == 'a') {
        if (arg->prec >= 0 && len > arg->prec)
            len = arg->prec;
    }

    /* Adjust sign and width */
    kind = PyUnicode_KIND(str);
    pbuf = PyUnicode_DATA(str);
    pindex = 0;
    signchar = '\0';
    if (arg->sign) {
        Py_UCS4 ch = PyUnicode_READ(kind, pbuf, pindex);
        if (ch == '-' || ch == '+') {
            signchar = ch;
            len--;
            pindex++;
        }
        else if (arg->flags & F_SIGN)
            signchar = '+';
        else if (arg->flags & F_BLANK)
            signchar = ' ';
        else
            arg->sign = 0;
    }
    if (arg->width < len)
        arg->width = len;

    /* Prepare the writer */
    maxchar = writer->maxchar;
    if (!(arg->flags & F_LJUST)) {
        if (arg->sign) {
            if ((arg->width-1) > len)
                maxchar = Py_MAX(maxchar, fill);
        }
        else {
            if (arg->width > len)
                maxchar = Py_MAX(maxchar, fill);
        }
    }
    if (PyUnicode_MAX_CHAR_VALUE(str) > maxchar) {
        Py_UCS4 strmaxchar = _PyUnicode_FindMaxChar(str, 0, pindex+len);
        maxchar = Py_MAX(maxchar, strmaxchar);
    }

    buflen = arg->width;
    if (arg->sign && len == arg->width)
        buflen++;
    if (_PyUnicodeWriter_Prepare(writer, buflen, maxchar) == -1)
        return -1;

    /* Write the sign if needed */
    if (arg->sign) {
        if (fill != ' ') {
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, signchar);
            writer->pos += 1;
        }
        if (arg->width > len)
            arg->width--;
    }

    /* Write the numeric prefix for "x", "X" and "o" formats
       if the alternate form is used.
       For example, write "0x" for the "%#x" format. */
    if ((arg->flags & F_ALT) && (arg->ch == 'x' || arg->ch == 'X' || arg->ch == 'o')) {
        assert(PyUnicode_READ(kind, pbuf, pindex) == '0');
        assert(PyUnicode_READ(kind, pbuf, pindex + 1) == arg->ch);
        if (fill != ' ') {
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, '0');
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos+1, arg->ch);
            writer->pos += 2;
            pindex += 2;
        }
        arg->width -= 2;
        if (arg->width < 0)
            arg->width = 0;
        len -= 2;
    }

    /* Pad left with the fill character if needed */
    if (arg->width > len && !(arg->flags & F_LJUST)) {
        sublen = arg->width - len;
        unicode_fill(writer->kind, writer->data, fill, writer->pos, sublen);
        writer->pos += sublen;
        arg->width = len;
    }

    /* If padding with spaces: write sign if needed and/or numeric prefix if
       the alternate form is used */
    if (fill == ' ') {
        if (arg->sign) {
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, signchar);
            writer->pos += 1;
        }
        if ((arg->flags & F_ALT) && (arg->ch == 'x' || arg->ch == 'X' || arg->ch == 'o')) {
            assert(PyUnicode_READ(kind, pbuf, pindex) == '0');
            assert(PyUnicode_READ(kind, pbuf, pindex+1) == arg->ch);
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, '0');
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos+1, arg->ch);
            writer->pos += 2;
            pindex += 2;
        }
    }

    /* Write characters */
    if (len) {
        _PyUnicode_FastCopyCharacters(writer->buffer, writer->pos,
                                      str, pindex, len);
        writer->pos += len;
    }

    /* Pad right with the fill character if needed */
    if (arg->width > len) {
        sublen = arg->width - len;
        unicode_fill(writer->kind, writer->data, ' ', writer->pos, sublen);
        writer->pos += sublen;
    }
    return 0;
}

/* Helper of PyUnicode_Format(): format one arg.
   Return 0 on success, raise an exception and return -1 on error. */
static int
unicode_format_arg(struct unicode_formatter_t *ctx)
{
    struct unicode_format_arg_t arg;
    PyObject *str;
    int ret;

    arg.ch = PyUnicode_READ(ctx->fmtkind, ctx->fmtdata, ctx->fmtpos);
    if (arg.ch == '%') {
        ctx->fmtpos++;
        ctx->fmtcnt--;
        if (_PyUnicodeWriter_WriteCharInline(&ctx->writer, '%') < 0)
            return -1;
        return 0;
    }
    arg.flags = 0;
    arg.width = -1;
    arg.prec = -1;
    arg.sign = 0;
    str = NULL;

    ret = unicode_format_arg_parse(ctx, &arg);
    if (ret == -1)
        return -1;

    ret = unicode_format_arg_format(ctx, &arg, &str);
    if (ret == -1)
        return -1;

    if (ret != 1) {
        ret = unicode_format_arg_output(ctx, &arg, str);
        Py_DECREF(str);
        if (ret == -1)
            return -1;
    }

    if (ctx->dict && (ctx->argidx < ctx->arglen)) {
        PyErr_SetString(PyExc_TypeError,
                        "not all arguments converted during string formatting");
        return -1;
    }
    return 0;
}

PyObject *
PyUnicode_Format(PyObject *format, PyObject *args)
{
    struct unicode_formatter_t ctx;

    if (format == NULL || args == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (ensure_unicode(format) < 0)
        return NULL;

    ctx.fmtstr = format;
    ctx.fmtdata = PyUnicode_DATA(ctx.fmtstr);
    ctx.fmtkind = PyUnicode_KIND(ctx.fmtstr);
    ctx.fmtcnt = PyUnicode_GET_LENGTH(ctx.fmtstr);
    ctx.fmtpos = 0;

    _PyUnicodeWriter_Init(&ctx.writer);
    ctx.writer.min_length = ctx.fmtcnt + 100;
    ctx.writer.overallocate = 1;

    if (PyTuple_Check(args)) {
        ctx.arglen = PyTuple_Size(args);
        ctx.argidx = 0;
    }
    else {
        ctx.arglen = -1;
        ctx.argidx = -2;
    }
    ctx.args_owned = 0;
    if (PyMapping_Check(args) && !PyTuple_Check(args) && !PyUnicode_Check(args))
        ctx.dict = args;
    else
        ctx.dict = NULL;
    ctx.args = args;

    while (--ctx.fmtcnt >= 0) {
        if (PyUnicode_READ(ctx.fmtkind, ctx.fmtdata, ctx.fmtpos) != '%') {
            Py_ssize_t nonfmtpos;

            nonfmtpos = ctx.fmtpos++;
            while (ctx.fmtcnt >= 0 &&
                   PyUnicode_READ(ctx.fmtkind, ctx.fmtdata, ctx.fmtpos) != '%') {
                ctx.fmtpos++;
                ctx.fmtcnt--;
            }
            if (ctx.fmtcnt < 0) {
                ctx.fmtpos--;
                ctx.writer.overallocate = 0;
            }

            if (_PyUnicodeWriter_WriteSubstring(&ctx.writer, ctx.fmtstr,
                                                nonfmtpos, ctx.fmtpos) < 0)
                goto onError;
        }
        else {
            ctx.fmtpos++;
            if (unicode_format_arg(&ctx) == -1)
                goto onError;
        }
    }

    if (ctx.argidx < ctx.arglen && !ctx.dict) {
        PyErr_SetString(PyExc_TypeError,
                        "not all arguments converted during string formatting");
        goto onError;
    }

    if (ctx.args_owned) {
        Py_DECREF(ctx.args);
    }
    return _PyUnicodeWriter_Finish(&ctx.writer);

  onError:
    _PyUnicodeWriter_Dealloc(&ctx.writer);
    if (ctx.args_owned) {
        Py_DECREF(ctx.args);
    }
    return NULL;
}

static PyObject *
unicode_subtype_new(PyTypeObject *type, PyObject *unicode);

/*[clinic input]
@classmethod
str.__new__ as unicode_new

    object as x: object = NULL
    encoding: str = NULL
    errors: str = NULL

[clinic start generated code]*/

static PyObject *
unicode_new_impl(PyTypeObject *type, PyObject *x, const char *encoding,
                 const char *errors)
/*[clinic end generated code: output=fc72d4878b0b57e9 input=e81255e5676d174e]*/
{
    PyObject *unicode;
    if (x == NULL) {
        unicode = unicode_get_empty();
    }
    else if (encoding == NULL && errors == NULL) {
        unicode = PyObject_Str(x);
    }
    else {
        unicode = PyUnicode_FromEncodedObject(x, encoding, errors);
    }

    if (unicode != NULL && type != &PyUnicode_Type) {
        Py_SETREF(unicode, unicode_subtype_new(type, unicode));
    }
    return unicode;
}

static const char *
arg_as_utf8(PyObject *obj, const char *name)
{
    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError,
                     "str() argument '%s' must be str, not %T",
                     name, obj);
        return NULL;
    }
    return _PyUnicode_AsUTF8NoNUL(obj);
}

static PyObject *
unicode_vectorcall(PyObject *type, PyObject *const *args,
                   size_t nargsf, PyObject *kwnames)
{
    assert(Py_Is(_PyType_CAST(type), &PyUnicode_Type));

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (kwnames != NULL && PyTuple_GET_SIZE(kwnames) != 0) {
        // Fallback to unicode_new()
        PyObject *tuple = _PyTuple_FromArray(args, nargs);
        if (tuple == NULL) {
            return NULL;
        }
        PyObject *dict = _PyStack_AsDict(args + nargs, kwnames);
        if (dict == NULL) {
            Py_DECREF(tuple);
            return NULL;
        }
        PyObject *ret = unicode_new(_PyType_CAST(type), tuple, dict);
        Py_DECREF(tuple);
        Py_DECREF(dict);
        return ret;
    }
    if (!_PyArg_CheckPositional("str", nargs, 0, 3)) {
        return NULL;
    }
    if (nargs == 0) {
        return unicode_get_empty();
    }
    PyObject *object = args[0];
    if (nargs == 1) {
        return PyObject_Str(object);
    }
    const char *encoding = arg_as_utf8(args[1], "encoding");
    if (encoding == NULL) {
        return NULL;
    }
    const char *errors = NULL;
    if (nargs == 3) {
        errors = arg_as_utf8(args[2], "errors");
        if (errors == NULL) {
            return NULL;
        }
    }
    return PyUnicode_FromEncodedObject(object, encoding, errors);
}

static PyObject *
unicode_subtype_new(PyTypeObject *type, PyObject *unicode)
{
    PyObject *self;
    Py_ssize_t length, char_size;
    int share_utf8;
    int kind;
    void *data;

    assert(PyType_IsSubtype(type, &PyUnicode_Type));
    assert(_PyUnicode_CHECK(unicode));

    self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    kind = PyUnicode_KIND(unicode);
    length = PyUnicode_GET_LENGTH(unicode);

    _PyUnicode_LENGTH(self) = length;
#ifdef Py_DEBUG
    _PyUnicode_HASH(self) = -1;
#else
    _PyUnicode_HASH(self) = _PyUnicode_HASH(unicode);
#endif
    _PyUnicode_STATE(self).interned = 0;
    _PyUnicode_STATE(self).kind = kind;
    _PyUnicode_STATE(self).compact = 0;
    _PyUnicode_STATE(self).ascii = _PyUnicode_STATE(unicode).ascii;
    _PyUnicode_STATE(self).statically_allocated = 0;
    PyUnicode_SET_UTF8_LENGTH(self, 0);
    PyUnicode_SET_UTF8(self, NULL);
    _PyUnicode_DATA_ANY(self) = NULL;

    share_utf8 = 0;
    if (kind == PyUnicode_1BYTE_KIND) {
        char_size = 1;
        if (PyUnicode_MAX_CHAR_VALUE(unicode) < 128)
            share_utf8 = 1;
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        char_size = 2;
    }
    else {
        assert(kind == PyUnicode_4BYTE_KIND);
        char_size = 4;
    }

    /* Ensure we won't overflow the length. */
    if (length > (PY_SSIZE_T_MAX / char_size - 1)) {
        PyErr_NoMemory();
        goto onError;
    }
    data = PyMem_Malloc((length + 1) * char_size);
    if (data == NULL) {
        PyErr_NoMemory();
        goto onError;
    }

    _PyUnicode_DATA_ANY(self) = data;
    if (share_utf8) {
        PyUnicode_SET_UTF8_LENGTH(self, length);
        PyUnicode_SET_UTF8(self, data);
    }

    memcpy(data, PyUnicode_DATA(unicode), kind * (length + 1));
    assert(_PyUnicode_CheckConsistency(self, 1));
#ifdef Py_DEBUG
    _PyUnicode_HASH(self) = _PyUnicode_HASH(unicode);
#endif
    return self;

onError:
    Py_DECREF(self);
    return NULL;
}

void
_PyUnicode_ExactDealloc(PyObject *op)
{
    assert(PyUnicode_CheckExact(op));
    unicode_dealloc(op);
}

PyDoc_STRVAR(unicode_doc,
"str(object='') -> str\n\
str(bytes_or_buffer[, encoding[, errors]]) -> str\n\
\n\
Create a new string object from the given object. If encoding or\n\
errors is specified, then the object must expose a data buffer\n\
that will be decoded using the given encoding and error handler.\n\
Otherwise, returns the result of object.__str__() (if defined)\n\
or repr(object).\n\
encoding defaults to 'utf-8'.\n\
errors defaults to 'strict'.");

static PyObject *unicode_iter(PyObject *seq);

PyTypeObject PyUnicode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "str",                        /* tp_name */
    sizeof(PyUnicodeObject),      /* tp_basicsize */
    0,                            /* tp_itemsize */
    /* Slots */
    unicode_dealloc,              /* tp_dealloc */
    0,                            /* tp_vectorcall_offset */
    0,                            /* tp_getattr */
    0,                            /* tp_setattr */
    0,                            /* tp_as_async */
    unicode_repr,                 /* tp_repr */
    &unicode_as_number,           /* tp_as_number */
    &unicode_as_sequence,         /* tp_as_sequence */
    &unicode_as_mapping,          /* tp_as_mapping */
    unicode_hash,                 /* tp_hash*/
    0,                            /* tp_call*/
    unicode_str,                  /* tp_str */
    PyObject_GenericGetAttr,      /* tp_getattro */
    0,                            /* tp_setattro */
    0,                            /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_UNICODE_SUBCLASS |
        _Py_TPFLAGS_MATCH_SELF, /* tp_flags */
    unicode_doc,                  /* tp_doc */
    0,                            /* tp_traverse */
    0,                            /* tp_clear */
    PyUnicode_RichCompare,        /* tp_richcompare */
    0,                            /* tp_weaklistoffset */
    unicode_iter,                 /* tp_iter */
    0,                            /* tp_iternext */
    unicode_methods,              /* tp_methods */
    0,                            /* tp_members */
    0,                            /* tp_getset */
    0,                            /* tp_base */
    0,                            /* tp_dict */
    0,                            /* tp_descr_get */
    0,                            /* tp_descr_set */
    0,                            /* tp_dictoffset */
    0,                            /* tp_init */
    0,                            /* tp_alloc */
    unicode_new,                  /* tp_new */
    PyObject_Free,                /* tp_free */
    .tp_vectorcall = unicode_vectorcall,
};

/* Initialize the Unicode implementation */

static void
_init_global_state(void)
{
    static int initialized = 0;
    if (initialized) {
        return;
    }
    initialized = 1;

    /* initialize the linebreak bloom filter */
    const Py_UCS2 linebreak[] = {
        0x000A, /* LINE FEED */
        0x000D, /* CARRIAGE RETURN */
        0x001C, /* FILE SEPARATOR */
        0x001D, /* GROUP SEPARATOR */
        0x001E, /* RECORD SEPARATOR */
        0x0085, /* NEXT LINE */
        0x2028, /* LINE SEPARATOR */
        0x2029, /* PARAGRAPH SEPARATOR */
    };
    bloom_linebreak = make_bloom_mask(
        PyUnicode_2BYTE_KIND, linebreak,
        Py_ARRAY_LENGTH(linebreak));
}

void
_PyUnicode_InitState(PyInterpreterState *interp)
{
    if (!_Py_IsMainInterpreter(interp)) {
        return;
    }
    _init_global_state();
}


PyStatus
_PyUnicode_InitGlobalObjects(PyInterpreterState *interp)
{
    if (_Py_IsMainInterpreter(interp)) {
        PyStatus status = init_global_interned_strings(interp);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }
    assert(INTERNED_STRINGS);

    if (init_interned_dict(interp)) {
        PyErr_Clear();
        return _PyStatus_ERR("failed to create interned dict");
    }

    return _PyStatus_OK();
}


PyStatus
_PyUnicode_InitTypes(PyInterpreterState *interp)
{
    if (_PyStaticType_InitBuiltin(interp, &EncodingMapType) < 0) {
        goto error;
    }
    if (_PyStaticType_InitBuiltin(interp, &PyFieldNameIter_Type) < 0) {
        goto error;
    }
    if (_PyStaticType_InitBuiltin(interp, &PyFormatterIter_Type) < 0) {
        goto error;
    }
    return _PyStatus_OK();

error:
    return _PyStatus_ERR("Can't initialize unicode types");
}

static /* non-null */ PyObject*
intern_static(PyInterpreterState *interp, PyObject *s /* stolen */)
{
    // Note that this steals a reference to `s`, but in many cases that
    // stolen ref is returned, requiring no decref/incref.

    assert(s != NULL);
    assert(_PyUnicode_CHECK(s));
    assert(_PyUnicode_STATE(s).statically_allocated);
    assert(!PyUnicode_CHECK_INTERNED(s));

#ifdef Py_DEBUG
    /* We must not add process-global interned string if there's already a
     * per-interpreter interned_dict, which might contain duplicates.
     */
    PyObject *interned = get_interned_dict(interp);
    assert(interned == NULL);
#endif

    /* Look in the global cache first. */
    PyObject *r = (PyObject *)_Py_hashtable_get(INTERNED_STRINGS, s);
    /* We should only init each string once */
    assert(r == NULL);
    /* but just in case (for the non-debug build), handle this */
    if (r != NULL && r != s) {
        assert(_PyUnicode_STATE(r).interned == SSTATE_INTERNED_IMMORTAL_STATIC);
        assert(_PyUnicode_CHECK(r));
        Py_DECREF(s);
        return Py_NewRef(r);
    }

    if (_Py_hashtable_set(INTERNED_STRINGS, s, s) < -1) {
        Py_FatalError("failed to intern static string");
    }

    _PyUnicode_STATE(s).interned = SSTATE_INTERNED_IMMORTAL_STATIC;
    return s;
}

void
_PyUnicode_InternStatic(PyInterpreterState *interp, PyObject **p)
{
    // This should only be called as part of runtime initialization
    assert(!Py_IsInitialized());

    *p = intern_static(interp, *p);
    assert(*p);
}

static void
immortalize_interned(PyObject *s)
{
    assert(PyUnicode_CHECK_INTERNED(s) == SSTATE_INTERNED_MORTAL);
    assert(!_Py_IsImmortal(s));
#ifdef Py_REF_DEBUG
    /* The reference count value should be excluded from the RefTotal.
       The decrements to these objects will not be registered so they
       need to be accounted for in here. */
    for (Py_ssize_t i = 0; i < Py_REFCNT(s); i++) {
        _Py_DecRefTotal(_PyThreadState_GET());
    }
#endif
    FT_ATOMIC_STORE_UINT8_RELAXED(_PyUnicode_STATE(s).interned, SSTATE_INTERNED_IMMORTAL);
    _Py_SetImmortal(s);
}

static /* non-null */ PyObject*
intern_common(PyInterpreterState *interp, PyObject *s /* stolen */,
              bool immortalize)
{
    // Note that this steals a reference to `s`, but in many cases that
    // stolen ref is returned, requiring no decref/incref.

#ifdef Py_DEBUG
    assert(s != NULL);
    assert(_PyUnicode_CHECK(s));
#else
    if (s == NULL || !PyUnicode_Check(s)) {
        return s;
    }
#endif

    /* If it's a subclass, we don't really know what putting
       it in the interned dict might do. */
    if (!PyUnicode_CheckExact(s)) {
        return s;
    }

    /* Is it already interned? */
    switch (PyUnicode_CHECK_INTERNED(s)) {
        case SSTATE_NOT_INTERNED:
            // no, go on
            break;
        case SSTATE_INTERNED_MORTAL:
            // yes but we might need to make it immortal
            if (immortalize) {
                immortalize_interned(s);
            }
            return s;
        default:
            // all done
            return s;
    }

    /* Statically allocated strings must be already interned. */
    assert(!_PyUnicode_STATE(s).statically_allocated);

#if Py_GIL_DISABLED
    /* In the free-threaded build, all interned strings are immortal */
    immortalize = 1;
#endif

    /* If it's already immortal, intern it as such */
    if (_Py_IsImmortal(s)) {
        immortalize = 1;
    }

    /* if it's a short string, get the singleton */
    if (PyUnicode_GET_LENGTH(s) == 1 &&
                PyUnicode_KIND(s) == PyUnicode_1BYTE_KIND) {
        PyObject *r = LATIN1(*(unsigned char*)PyUnicode_DATA(s));
        assert(PyUnicode_CHECK_INTERNED(r));
        Py_DECREF(s);
        return r;
    }
#ifdef Py_DEBUG
    assert(!unicode_is_singleton(s));
#endif

    /* Look in the global cache now. */
    {
        PyObject *r = (PyObject *)_Py_hashtable_get(INTERNED_STRINGS, s);
        if (r != NULL) {
            assert(_PyUnicode_STATE(r).statically_allocated);
            assert(r != s);  // r must be statically_allocated; s is not
            Py_DECREF(s);
            return Py_NewRef(r);
        }
    }

    /* Do a setdefault on the per-interpreter cache. */
    PyObject *interned = get_interned_dict(interp);
    assert(interned != NULL);

    LOCK_INTERNED(interp);
    PyObject *t;
    {
        int res = PyDict_SetDefaultRef(interned, s, s, &t);
        if (res < 0) {
            PyErr_Clear();
            UNLOCK_INTERNED(interp);
            return s;
        }
        else if (res == 1) {
            // value was already present (not inserted)
            Py_DECREF(s);
            if (immortalize &&
                    PyUnicode_CHECK_INTERNED(t) == SSTATE_INTERNED_MORTAL) {
                immortalize_interned(t);
            }
            UNLOCK_INTERNED(interp);
            return t;
        }
        else {
            // value was newly inserted
            assert (s == t);
            Py_DECREF(t);
        }
    }

    /* NOT_INTERNED -> INTERNED_MORTAL */

    assert(_PyUnicode_STATE(s).interned == SSTATE_NOT_INTERNED);

    if (!_Py_IsImmortal(s)) {
        /* The two references in interned dict (key and value) are not counted.
        unicode_dealloc() and _PyUnicode_ClearInterned() take care of this. */
        Py_DECREF(s);
        Py_DECREF(s);
    }
    FT_ATOMIC_STORE_UINT8_RELAXED(_PyUnicode_STATE(s).interned, SSTATE_INTERNED_MORTAL);

    /* INTERNED_MORTAL -> INTERNED_IMMORTAL (if needed) */

#ifdef Py_DEBUG
    if (_Py_IsImmortal(s)) {
        assert(immortalize);
    }
#endif
    if (immortalize) {
        immortalize_interned(s);
    }

    UNLOCK_INTERNED(interp);
    return s;
}

void
_PyUnicode_InternImmortal(PyInterpreterState *interp, PyObject **p)
{
    *p = intern_common(interp, *p, 1);
    assert(*p);
}

void
_PyUnicode_InternMortal(PyInterpreterState *interp, PyObject **p)
{
    *p = intern_common(interp, *p, 0);
    assert(*p);
}


void
_PyUnicode_InternInPlace(PyInterpreterState *interp, PyObject **p)
{
    _PyUnicode_InternImmortal(interp, p);
    return;
}

void
PyUnicode_InternInPlace(PyObject **p)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_InternMortal(interp, p);
}

// Public-looking name kept for the stable ABI; user should not call this:
PyAPI_FUNC(void) PyUnicode_InternImmortal(PyObject **);
void
PyUnicode_InternImmortal(PyObject **p)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_InternImmortal(interp, p);
}

PyObject *
PyUnicode_InternFromString(const char *cp)
{
    PyObject *s = PyUnicode_FromString(cp);
    if (s == NULL) {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_InternMortal(interp, &s);
    return s;
}


void
_PyUnicode_ClearInterned(PyInterpreterState *interp)
{
    PyObject *interned = get_interned_dict(interp);
    if (interned == NULL) {
        return;
    }
    assert(PyDict_CheckExact(interned));

    if (has_shared_intern_dict(interp)) {
        // the dict doesn't belong to this interpreter, skip the debug
        // checks on it and just clear the pointer to it
        clear_interned_dict(interp);
        return;
    }

#ifdef INTERNED_STATS
    fprintf(stderr, "releasing %zd interned strings\n",
            PyDict_GET_SIZE(interned));

    Py_ssize_t total_length = 0;
#endif
    Py_ssize_t pos = 0;
    PyObject *s, *ignored_value;
    while (PyDict_Next(interned, &pos, &s, &ignored_value)) {
        int shared = 0;
        switch (PyUnicode_CHECK_INTERNED(s)) {
        case SSTATE_INTERNED_IMMORTAL:
            /* Make immortal interned strings mortal again. */
            // Skip the Immortal Instance check and restore
            // the two references (key and value) ignored
            // by PyUnicode_InternInPlace().
            _Py_SetMortal(s, 2);
#ifdef Py_REF_DEBUG
            /* let's be pedantic with the ref total */
            _Py_IncRefTotal(_PyThreadState_GET());
            _Py_IncRefTotal(_PyThreadState_GET());
#endif
#ifdef INTERNED_STATS
            total_length += PyUnicode_GET_LENGTH(s);
#endif
            break;
        case SSTATE_INTERNED_IMMORTAL_STATIC:
            /* It is shared between interpreters, so we should unmark it
               only when this is the last interpreter in which it's
               interned.  We immortalize all the statically initialized
               strings during startup, so we can rely on the
               main interpreter to be the last one. */
            if (!_Py_IsMainInterpreter(interp)) {
                shared = 1;
            }
            break;
        case SSTATE_INTERNED_MORTAL:
            // Restore 2 references held by the interned dict; these will
            // be decref'd by clear_interned_dict's PyDict_Clear.
            _Py_RefcntAdd(s, 2);
#ifdef Py_REF_DEBUG
            /* let's be pedantic with the ref total */
            _Py_IncRefTotal(_PyThreadState_GET());
            _Py_IncRefTotal(_PyThreadState_GET());
#endif
            break;
        case SSTATE_NOT_INTERNED:
            _Py_FALLTHROUGH;
        default:
            Py_UNREACHABLE();
        }
        if (!shared) {
            FT_ATOMIC_STORE_UINT8_RELAXED(_PyUnicode_STATE(s).interned, SSTATE_NOT_INTERNED);
        }
    }
#ifdef INTERNED_STATS
    fprintf(stderr,
            "total length of all interned strings: %zd characters\n",
            total_length);
#endif

    struct _Py_unicode_state *state = &interp->unicode;
    struct _Py_unicode_ids *ids = &state->ids;
    for (Py_ssize_t i=0; i < ids->size; i++) {
        Py_XINCREF(ids->array[i]);
    }
    clear_interned_dict(interp);
    if (_Py_IsMainInterpreter(interp)) {
        clear_global_interned_strings();
    }
}


/********************* Unicode Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyObject *it_seq;    /* Set to NULL when iterator is exhausted */
} unicodeiterobject;

static void
unicodeiter_dealloc(PyObject *op)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
unicodeiter_traverse(PyObject *op, visitproc visit, void *arg)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
unicodeiter_next(PyObject *op)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    PyObject *seq;

    assert(it != NULL);
    seq = it->it_seq;
    if (seq == NULL)
        return NULL;
    assert(_PyUnicode_CHECK(seq));

    if (it->it_index < PyUnicode_GET_LENGTH(seq)) {
        int kind = PyUnicode_KIND(seq);
        const void *data = PyUnicode_DATA(seq);
        Py_UCS4 chr = PyUnicode_READ(kind, data, it->it_index);
        it->it_index++;
        return unicode_char(chr);
    }

    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
unicode_ascii_iter_next(PyObject *op)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    assert(it != NULL);
    PyObject *seq = it->it_seq;
    if (seq == NULL) {
        return NULL;
    }
    assert(_PyUnicode_CHECK(seq));
    assert(PyUnicode_IS_COMPACT_ASCII(seq));
    if (it->it_index < PyUnicode_GET_LENGTH(seq)) {
        const void *data = ((void*)(_PyASCIIObject_CAST(seq) + 1));
        Py_UCS1 chr = (Py_UCS1)PyUnicode_READ(PyUnicode_1BYTE_KIND,
                                              data, it->it_index);
        it->it_index++;
        return (PyObject*)&_Py_SINGLETON(strings).ascii[chr];
    }
    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
unicodeiter_len(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    Py_ssize_t len = 0;
    if (it->it_seq)
        len = PyUnicode_GET_LENGTH(it->it_seq) - it->it_index;
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
unicodeiter_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */

    if (it->it_seq != NULL) {
        return Py_BuildValue("N(O)n", iter, it->it_seq, it->it_index);
    } else {
        PyObject *u = unicode_get_empty();
        if (u == NULL) {
            Py_XDECREF(iter);
            return NULL;
        }
        return Py_BuildValue("N(N)", iter, u);
    }
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
unicodeiter_setstate(PyObject *op, PyObject *state)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyUnicode_GET_LENGTH(it->it_seq))
            index = PyUnicode_GET_LENGTH(it->it_seq); /* iterator truncated */
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef unicodeiter_methods[] = {
    {"__length_hint__", unicodeiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__",      unicodeiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__",    unicodeiter_setstate, METH_O, setstate_doc},
    {NULL,      NULL}       /* sentinel */
};

PyTypeObject PyUnicodeIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "str_iterator",         /* tp_name */
    sizeof(unicodeiterobject),      /* tp_basicsize */
    0,                  /* tp_itemsize */
    /* methods */
    unicodeiter_dealloc,/* tp_dealloc */
    0,                  /* tp_vectorcall_offset */
    0,                  /* tp_getattr */
    0,                  /* tp_setattr */
    0,                  /* tp_as_async */
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
    unicodeiter_traverse, /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    0,                  /* tp_weaklistoffset */
    PyObject_SelfIter,          /* tp_iter */
    unicodeiter_next,   /* tp_iternext */
    unicodeiter_methods,            /* tp_methods */
    0,
};

PyTypeObject _PyUnicodeASCIIIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "str_ascii_iterator",
    .tp_basicsize = sizeof(unicodeiterobject),
    .tp_dealloc = unicodeiter_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = unicodeiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = unicode_ascii_iter_next,
    .tp_methods = unicodeiter_methods,
};

static PyObject *
unicode_iter(PyObject *seq)
{
    unicodeiterobject *it;

    if (!PyUnicode_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (PyUnicode_IS_COMPACT_ASCII(seq)) {
        it = PyObject_GC_New(unicodeiterobject, &_PyUnicodeASCIIIter_Type);
    }
    else {
        it = PyObject_GC_New(unicodeiterobject, &PyUnicodeIter_Type);
    }
    if (it == NULL)
        return NULL;
    it->it_index = 0;
    it->it_seq = Py_NewRef(seq);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

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


static void
_PyUnicode_FiniEncodings(struct _Py_unicode_fs_codec *fs_codec)
{
    PyMem_RawFree(fs_codec->encoding);
    fs_codec->encoding = NULL;
    fs_codec->utf8 = 0;
    PyMem_RawFree(fs_codec->errors);
    fs_codec->errors = NULL;
    fs_codec->error_handler = _Py_ERROR_UNKNOWN;
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


#ifdef Py_DEBUG
static inline int
unicode_is_finalizing(void)
{
    return (get_interned_dict(_PyInterpreterState_Main()) == NULL);
}
#endif


void
_PyUnicode_FiniTypes(PyInterpreterState *interp)
{
    _PyStaticType_FiniBuiltin(interp, &EncodingMapType);
    _PyStaticType_FiniBuiltin(interp, &PyFieldNameIter_Type);
    _PyStaticType_FiniBuiltin(interp, &PyFormatterIter_Type);
}


void
_PyUnicode_Fini(PyInterpreterState *interp)
{
    struct _Py_unicode_state *state = &interp->unicode;

    if (!has_shared_intern_dict(interp)) {
        // _PyUnicode_ClearInterned() must be called before _PyUnicode_Fini()
        assert(get_interned_dict(interp) == NULL);
    }

    _PyUnicode_FiniEncodings(&state->fs_codec);

    // bpo-47182: force a unicodedata CAPI capsule re-import on
    // subsequent initialization of interpreter.
    interp->unicode.ucnhash_capi = NULL;

    unicode_clear_identifiers(state);
}

/* A _string module, to export formatter_parser and formatter_field_name_split
   to the string.Formatter class implemented in Python. */

static PyMethodDef _string_methods[] = {
    {"formatter_field_name_split", formatter_field_name_split,
     METH_O, PyDoc_STR("split the argument as a field name")},
    {"formatter_parser", formatter_parser,
     METH_O, PyDoc_STR("parse the argument as a format string")},
    {NULL, NULL}
};

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _string_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_string",
    .m_doc = PyDoc_STR("string helper module"),
    .m_size = 0,
    .m_methods = _string_methods,
    .m_slots = module_slots,
};

PyMODINIT_FUNC
PyInit__string(void)
{
    return PyModuleDef_Init(&_string_module);
}


#undef PyUnicode_KIND
int PyUnicode_KIND(PyObject *op)
{
    if (!PyUnicode_Check(op)) {
        PyErr_Format(PyExc_TypeError, "expect str, got %T", op);
        return -1;
    }
    return _PyASCIIObject_CAST(op)->state.kind;
}

#undef PyUnicode_DATA
void* PyUnicode_DATA(PyObject *op)
{
    if (!PyUnicode_Check(op)) {
        PyErr_Format(PyExc_TypeError, "expect str, got %T", op);
        return NULL;
    }
    return _PyUnicode_DATA(op);
}
