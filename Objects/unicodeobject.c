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
#include "pycore_codecs.h"        // _PyCodec_Lookup()
#include "pycore_critical_section.h" // Py_*_CRITICAL_SECTION_SEQUENCE_FAST
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // PyInterpreterState.fs_codec
#include "pycore_object.h"        // _Py_FatalRefcountError()
#include "pycore_pyhash.h"        // _Py_HashSecret_Initialized
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_tuple.h"         // _PyTuple_FromArray()
#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI
#include "pycore_unicodeobject.h" // struct _Py_unicode_state

#ifdef MS_WINDOWS
#  include <windows.h>
#endif


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

#define MAX_UNICODE _Py_MAX_UNICODE

static inline char* _PyUnicode_UTF8(PyObject *op)
{
    return FT_ATOMIC_LOAD_PTR_ACQUIRE(_PyCompactUnicodeObject_CAST(op)->utf8);
}

char* PyUnicode_UTF8(PyObject *op)
{
    assert(_PyUnicode_CHECK(op));
    if (PyUnicode_IS_COMPACT_ASCII(op)) {
        return ((char*)(_PyASCIIObject_CAST(op) + 1));
    }
    else {
         return _PyUnicode_UTF8(op);
    }
}

void _PyUnicode_SET_UTF8(PyObject *op, char *utf8)
{
    FT_ATOMIC_STORE_PTR_RELEASE(_PyCompactUnicodeObject_CAST(op)->utf8, utf8);
}

Py_ssize_t PyUnicode_UTF8_LENGTH(PyObject *op)
{
    assert(_PyUnicode_CHECK(op));
    if (PyUnicode_IS_COMPACT_ASCII(op)) {
         return _PyASCIIObject_CAST(op)->length;
    }
    else {
         return _PyCompactUnicodeObject_CAST(op)->utf8_length;
    }
}

void _PyUnicode_SET_UTF8_LENGTH(PyObject *op, Py_ssize_t length)
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


#define LATIN1 _Py_LATIN1_CHR

/* Forward declaration */
#ifdef Py_DEBUG
static inline int unicode_is_finalizing(void);
static int unicode_is_singleton(PyObject *unicode);
#endif
static int unicode_modifiable(PyObject *unicode);


// Return a reference to the immortal empty string singleton.
static inline PyObject* unicode_get_empty(void)
{
    _Py_DECLARE_STR(empty, "");
    return &_Py_STR(empty);
}

PyObject*
_PyUnicode_GetEmpty(void)
{
    return unicode_get_empty();
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

#define _Py_RETURN_UNICODE_EMPTY()   \
    do {                             \
        return unicode_get_empty();  \
    } while (0)

static int convert_uc(PyObject *obj, void *addr);

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


_Py_error_handler
_Py_GetErrorHandlerWide(const wchar_t *errors)
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


int
_PyUnicode_CheckEncodingErrors(const char *encoding, const char *errors)
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

PyObject*
_PyUnicode_Result(PyObject *unicode)
{
    return unicode_result(unicode);
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

PyObject*
_PyUnicode_ResultUnchanged(PyObject *unicode)
{
    return unicode_result_unchanged(unicode);
}

/* --- Unicode Object ----------------------------------------------------- */

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
        _PyUnicode_SET_UTF8_LENGTH(unicode, 0);
        _PyUnicode_SET_UTF8(unicode, NULL);
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

PyObject*
_PyUnicode_ResizeCompact(PyObject *unicode, Py_ssize_t length)
{
    return resize_compact(unicode, length);
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
        _PyUnicode_SET_UTF8_LENGTH(unicode, 0);
        _PyUnicode_SET_UTF8(unicode, NULL);
    }

    data = (PyObject *)PyObject_Realloc(data, new_size);
    if (data == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    _PyUnicode_DATA_ANY(unicode) = data;
    if (share_utf8) {
        _PyUnicode_SET_UTF8_LENGTH(unicode, length);
        _PyUnicode_SET_UTF8(unicode, data);
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

int
_PyUnicode_CheckModifiable(PyObject *unicode)
{
    if (!unicode_modifiable(unicode)) {
        PyErr_SetString(PyExc_SystemError,
                        "Cannot modify a string currently used");
        return -1;
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

int
_PyUnicode_IsModifiable(PyObject *unicode)
{
    return unicode_modifiable(unicode);
}

int
_PyUnicode_Resize(PyObject **p_unicode, Py_ssize_t length)
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
    return _PyUnicode_Resize(p_unicode, length);
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

/* --- Helpers ------------------------------------------------------------ */

#define ADJUST_INDICES _Py_ADJUST_INDICES
#define ensure_unicode _PyUnicode_Ensure

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

/* --- Unicode Object Methods --------------------------------------------- */

/*[clinic input]
@permit_long_docstring_body
str.title as unicode_title

Return a version of the string where each word is titlecased.

More specifically, words start with uppercased characters and all remaining
cased characters have lower case.
[clinic start generated code]*/

static PyObject *
unicode_title_impl(PyObject *self)
/*[clinic end generated code: output=c75ae03809574902 input=533ce0eb6a7f5d1b]*/
{
    return case_operation(self, do_title);
}

/*[clinic input]
@permit_long_docstring_body
str.capitalize as unicode_capitalize

Return a capitalized version of the string.

More specifically, make the first character have upper case and the rest lower
case.
[clinic start generated code]*/

static PyObject *
unicode_capitalize_impl(PyObject *self)
/*[clinic end generated code: output=e49a4c333cdb7667 input=a4a15ade41f6f9e9]*/
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

    return _PyUnicode_Pad(self, left, marg - left, fillchar);
}

/*[clinic input]
@permit_long_summary
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
/*[clinic end generated code: output=8fcc3aef0b18edbf input=8590716ee228b935]*/
{
    return _PyUnicode_Count(str, substr, start, end);
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
    return _PyUnicode_Expandtabs(self, tabsize);
}

/*[clinic input]
@permit_long_summary
str.find as unicode_find = str.count

Return the lowest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Return -1 on failure.
[clinic start generated code]*/

static Py_ssize_t
unicode_find_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                  Py_ssize_t end)
/*[clinic end generated code: output=51dbe6255712e278 input=3a9d650fe4c24695]*/
{
    Py_ssize_t result = _PyUnicode_AnylibFindSlice(str, substr, start, end, 1);
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
    return _PyUnicode_FromOrdinal(ch);
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

Py_hash_t
_PyUnicode_Hash(PyObject *self)
{
    return unicode_hash(self);
}

/*[clinic input]
@permit_long_summary
str.index as unicode_index = str.count

Return the lowest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Raises ValueError when the substring is not found.
[clinic start generated code]*/

static Py_ssize_t
unicode_index_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end)
/*[clinic end generated code: output=77558288837cdf40 input=ae5e48f69ed75b06]*/
{
    Py_ssize_t result = _PyUnicode_AnylibFindSlice(str, substr, start, end, 1);
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
@permit_long_docstring_body
str.islower as unicode_islower

Return True if the string is a lowercase string, False otherwise.

A string is lowercase if all cased characters in the string are lowercase and
there is at least one cased character in the string.
[clinic start generated code]*/

static PyObject *
unicode_islower_impl(PyObject *self)
/*[clinic end generated code: output=dbd41995bd005b81 input=c6fc0295241a1aaa]*/
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
@permit_long_docstring_body
str.isupper as unicode_isupper

Return True if the string is an uppercase string, False otherwise.

A string is uppercase if all cased characters in the string are uppercase and
there is at least one cased character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isupper_impl(PyObject *self)
/*[clinic end generated code: output=049209c8e7f15f59 input=8d5cb33e67efde72]*/
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
@permit_long_docstring_body
str.isspace as unicode_isspace

Return True if the string is a whitespace string, False otherwise.

A string is whitespace if all characters in the string are whitespace and there
is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isspace_impl(PyObject *self)
/*[clinic end generated code: output=163a63bfa08ac2b9 input=44fe05e248c6e159]*/
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
@permit_long_docstring_body
str.isalpha as unicode_isalpha

Return True if the string is an alphabetic string, False otherwise.

A string is alphabetic if all characters in the string are alphabetic and there
is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isalpha_impl(PyObject *self)
/*[clinic end generated code: output=cc81b9ac3883ec4f input=c233000624a56e0d]*/
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
@permit_long_docstring_body
str.isalnum as unicode_isalnum

Return True if the string is an alpha-numeric string, False otherwise.

A string is alpha-numeric if all characters in the string are alpha-numeric and
there is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isalnum_impl(PyObject *self)
/*[clinic end generated code: output=a5a23490ffc3660c input=5d63ba9c9bafdb6b]*/
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
@permit_long_docstring_body
str.isdecimal as unicode_isdecimal

Return True if the string is a decimal string, False otherwise.

A string is a decimal string if all characters in the string are decimal and
there is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isdecimal_impl(PyObject *self)
/*[clinic end generated code: output=fb2dcdb62d3fc548 input=8e84a58b414935a3]*/
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
@permit_long_docstring_body
str.isdigit as unicode_isdigit

Return True if the string is a digit string, False otherwise.

A string is a digit string if all characters in the string are digits and there
is at least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isdigit_impl(PyObject *self)
/*[clinic end generated code: output=10a6985311da6858 input=99e284affb54d4a0]*/
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
@permit_long_docstring_body
str.isnumeric as unicode_isnumeric

Return True if the string is a numeric string, False otherwise.

A string is numeric if all characters in the string are numeric and there is at
least one character in the string.
[clinic start generated code]*/

static PyObject *
unicode_isnumeric_impl(PyObject *self)
/*[clinic end generated code: output=9172a32d9013051a input=e9f5b6b8b29b0ee6]*/
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
@permit_long_docstring_body
str.isidentifier as unicode_isidentifier

Return True if the string is a valid Python identifier, False otherwise.

Call keyword.iskeyword(s) to test whether string s is a reserved identifier,
such as "def" or "class".
[clinic start generated code]*/

static PyObject *
unicode_isidentifier_impl(PyObject *self)
/*[clinic end generated code: output=fe585a9666572905 input=86315dd889d7bd04]*/
{
    return PyBool_FromLong(PyUnicode_IsIdentifier(self));
}

/*[clinic input]
@permit_long_summary
str.isprintable as unicode_isprintable

Return True if all characters in the string are printable, False otherwise.

A character is printable if repr() may use it in its output.
[clinic start generated code]*/

static PyObject *
unicode_isprintable_impl(PyObject *self)
/*[clinic end generated code: output=3ab9626cd32dd1a0 input=18345ba847084ec5]*/
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
@permit_long_docstring_body
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
/*[clinic end generated code: output=6857e7cecfe7bf98 input=bac724ed412ef3f8]*/
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

    return _PyUnicode_Pad(self, 0, width - PyUnicode_GET_LENGTH(self), fillchar);
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

/* Arrays indexed by above */
static const char *stripfuncnames[] = {"lstrip", "rstrip", "strip"};

#define STRIPNAME(i) (stripfuncnames[i])

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

    return _PyUnicode_do_strip(self, striptype);
}


/*[clinic input]
@permit_long_summary
str.strip as unicode_strip

    chars: object = None
    /

Return a copy of the string with leading and trailing whitespace removed.

If chars is given and not None, remove characters in chars instead.
[clinic start generated code]*/

static PyObject *
unicode_strip_impl(PyObject *self, PyObject *chars)
/*[clinic end generated code: output=ca19018454345d57 input=8bc6353450345fbd]*/
{
    return do_argstrip(self, _Py_BOTHSTRIP, chars);
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
    return do_argstrip(self, _Py_LEFTSTRIP, chars);
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
    return do_argstrip(self, _Py_RIGHTSTRIP, chars);
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
    return _PyUnicode_Replace(str, substr, replstr, maxcount);
}

/*[clinic input]
@permit_long_docstring_body
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
/*[clinic end generated code: output=b63f1a8b5eebf448 input=f27ca92ac46b65a1]*/
{
    return _PyUnicode_Replace(self, old, new, count);
}

/*[clinic input]
@permit_long_docstring_body
str.removeprefix as unicode_removeprefix

    prefix: unicode
    /

Return a str with the given prefix string removed if present.

If the string starts with the prefix string, return string[len(prefix):].
Otherwise, return a copy of the original string.
[clinic start generated code]*/

static PyObject *
unicode_removeprefix_impl(PyObject *self, PyObject *prefix)
/*[clinic end generated code: output=f1e5945e9763bcb9 input=1989a856dbb813f1]*/
{
    int match = _PyUnicode_Tailmatch(self, prefix, 0, PY_SSIZE_T_MAX, -1);
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
    int match = _PyUnicode_Tailmatch(self, suffix, 0, PY_SSIZE_T_MAX, +1);
    if (match == -1) {
        return NULL;
    }
    if (match) {
        return PyUnicode_Substring(self, 0, PyUnicode_GET_LENGTH(self)
                                            - PyUnicode_GET_LENGTH(suffix));
    }
    return unicode_result_unchanged(self);
}

/*[clinic input]
@permit_long_summary
str.rfind as unicode_rfind = str.count

Return the highest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Return -1 on failure.
[clinic start generated code]*/

static Py_ssize_t
unicode_rfind_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end)
/*[clinic end generated code: output=880b29f01dd014c8 input=7f7e97d5cd3299a2]*/
{
    Py_ssize_t result = _PyUnicode_AnylibFindSlice(str, substr, start, end, -1);
    if (result < 0) {
        return -1;
    }
    return result;
}

/*[clinic input]
@permit_long_summary
str.rindex as unicode_rindex = str.count

Return the highest index in S where substring sub is found, such that sub is contained within S[start:end].

Optional arguments start and end are interpreted as in slice notation.
Raises ValueError when the substring is not found.
[clinic start generated code]*/

static Py_ssize_t
unicode_rindex_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                    Py_ssize_t end)
/*[clinic end generated code: output=5f3aef124c867fe1 input=0363a324740b3e62]*/
{
    Py_ssize_t result = _PyUnicode_AnylibFindSlice(str, substr, start, end, -1);
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

    return _PyUnicode_Pad(self, width - PyUnicode_GET_LENGTH(self), 0, fillchar);
}

PyObject *
PyUnicode_Split(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)
{
    if (ensure_unicode(s) < 0 || (sep != NULL && ensure_unicode(sep) < 0))
        return NULL;

    return _PyUnicode_Split(s, sep, maxsplit);
}

/*[clinic input]
@permit_long_summary
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
/*[clinic end generated code: output=3a65b1db356948dc input=2c1fd08a78e038b8]*/
{
    if (sep == Py_None)
        return _PyUnicode_Split(self, NULL, maxsplit);
    if (PyUnicode_Check(sep))
        return _PyUnicode_Split(self, sep, maxsplit);

    PyErr_Format(PyExc_TypeError,
                 "must be str or None, not %.100s",
                 Py_TYPE(sep)->tp_name);
    return NULL;
}

/*[clinic input]
@permit_long_docstring_body
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
/*[clinic end generated code: output=e4ced7bd253ca3c4 input=4d854b520d7b0e97]*/
{
    return PyUnicode_Partition(self, sep);
}

/*[clinic input]
@permit_long_docstring_body
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
/*[clinic end generated code: output=1aa13cf1156572aa input=a6adabe91e75b486]*/
{
    return PyUnicode_RPartition(self, sep);
}

PyObject *
PyUnicode_RSplit(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)
{
    if (ensure_unicode(s) < 0 || (sep != NULL && ensure_unicode(sep) < 0))
        return NULL;

    return _PyUnicode_RSplit(s, sep, maxsplit);
}

/*[clinic input]
@permit_long_summary
str.rsplit as unicode_rsplit = str.split

Return a list of the substrings in the string, using sep as the separator string.

Splitting starts at the end of the string and works to the front.
[clinic start generated code]*/

static PyObject *
unicode_rsplit_impl(PyObject *self, PyObject *sep, Py_ssize_t maxsplit)
/*[clinic end generated code: output=c2b815c63bcabffc input=0f762e30d267fa83]*/
{
    if (sep == Py_None)
        return _PyUnicode_RSplit(self, NULL, maxsplit);
    if (PyUnicode_Check(sep))
        return _PyUnicode_RSplit(self, sep, maxsplit);

    PyErr_Format(PyExc_TypeError,
                 "must be str or None, not %.100s",
                 Py_TYPE(sep)->tp_name);
    return NULL;
}

/*[clinic input]
@permit_long_docstring_body
str.splitlines as unicode_splitlines

    keepends: bool = False

Return a list of the lines in the string, breaking at line boundaries.

Line breaks are not included in the resulting list unless keepends is given and
true.
[clinic start generated code]*/

static PyObject *
unicode_splitlines_impl(PyObject *self, int keepends)
/*[clinic end generated code: output=f664dcdad153ec40 input=39eeafbfef61c827]*/
{
    return PyUnicode_Splitlines(self, keepends);
}

static
PyObject *unicode_str(PyObject *self)
{
    return unicode_result_unchanged(self);
}

/*[clinic input]
@permit_long_summary
str.swapcase as unicode_swapcase

Convert uppercase characters to lowercase and lowercase characters to uppercase.
[clinic start generated code]*/

static PyObject *
unicode_swapcase_impl(PyObject *self)
/*[clinic end generated code: output=5d28966bf6d7b2af input=85bc39a9b4e8ee91]*/
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
    return _PyUnicode_Maketrans(x, y, z);
}

/*[clinic input]
@permit_long_docstring_body
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
/*[clinic end generated code: output=3cb448ff2fd96bf3 input=699e5fa0ebf9f5e9]*/
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
@permit_long_summary
str.zfill as unicode_zfill

    width: Py_ssize_t
    /

Pad a numeric string with zeros on the left, to fill a field of the given width.

The string is never truncated.
[clinic start generated code]*/

static PyObject *
unicode_zfill_impl(PyObject *self, Py_ssize_t width)
/*[clinic end generated code: output=e13fb6bdf8e3b9df input=25a4ee0ea3e58ce0]*/
{
    Py_ssize_t fill;
    PyObject *u;
    int kind;
    const void *data;
    Py_UCS4 chr;

    if (PyUnicode_GET_LENGTH(self) >= width)
        return unicode_result_unchanged(self);

    fill = width - PyUnicode_GET_LENGTH(self);

    u = _PyUnicode_Pad(self, fill, 0, '0');

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
@permit_long_summary
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
/*[clinic end generated code: output=4bd7cfd0803051d4 input=766bdbd33df251dc]*/
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
            int result = _PyUnicode_Tailmatch(self, substring, start, end, -1);
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
    int result = _PyUnicode_Tailmatch(self, subobj, start, end, -1);
    if (result < 0) {
        return NULL;
    }
    return PyBool_FromLong(result);
}


/*[clinic input]
@permit_long_summary
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
/*[clinic end generated code: output=cce6f8ceb0102ca9 input=b66bf6d5547ba1aa]*/
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
            int result = _PyUnicode_Tailmatch(self, substring, start, end, +1);
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
    int result = _PyUnicode_Tailmatch(self, subobj, start, end, +1);
    if (result < 0) {
        return NULL;
    }
    return PyBool_FromLong(result);
}


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
    {"format", _PyCFunction_CAST(_PyUnicode_do_string_format), METH_VARARGS | METH_KEYWORDS, format__doc__},
    {"format_map", _PyUnicode_do_string_format_map, METH_O, format_map__doc__},
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
    _PyUnicode_SET_UTF8_LENGTH(self, 0);
    _PyUnicode_SET_UTF8(self, NULL);
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
        _PyUnicode_SET_UTF8_LENGTH(self, length);
        _PyUnicode_SET_UTF8(self, data);
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
    _PyUnicode_Repr,              /* tp_repr */
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
    _PyUnicode_Iter,              /* tp_iter */
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

void
_PyUnicode_InitState(PyInterpreterState *interp)
{
    if (!_Py_IsMainInterpreter(interp)) {
        return;
    }
    _PyUnicode_InitGlobalState();
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
#ifdef Py_GIL_DISABLED
#  define INTERN_MUTEX &_Py_INTERP_CACHED_OBJECT(interp, interned_mutex)
#endif
    FT_MUTEX_LOCK(INTERN_MUTEX);
    PyObject *t;
    {
        int res = PyDict_SetDefaultRef(interned, s, s, &t);
        if (res < 0) {
            PyErr_Clear();
            FT_MUTEX_UNLOCK(INTERN_MUTEX);
            return s;
        }
        else if (res == 1) {
            // value was already present (not inserted)
            Py_DECREF(s);
            if (immortalize &&
                    PyUnicode_CHECK_INTERNED(t) == SSTATE_INTERNED_MORTAL) {
                immortalize_interned(t);
            }
            FT_MUTEX_UNLOCK(INTERN_MUTEX);
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

    FT_MUTEX_UNLOCK(INTERN_MUTEX);
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


#ifdef Py_DEBUG
static inline int
unicode_is_finalizing(void)
{
    return (get_interned_dict(_PyInterpreterState_Main()) == NULL);
}
#endif


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
