#ifndef Py_INTERNAL_UNICODEOBJECT_H
#define Py_INTERNAL_UNICODEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"          // PyMutex
#include "pycore_fileutils.h"     // _Py_error_handler
#include "pycore_identifier.h"    // _Py_Identifier
#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI
#include "pycore_global_objects.h"  // _Py_SINGLETON

/* --- Characters Type APIs ----------------------------------------------- */

extern int _PyUnicode_IsXidStart(Py_UCS4 ch);
extern int _PyUnicode_IsXidContinue(Py_UCS4 ch);
extern int _PyUnicode_ToLowerFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_ToTitleFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_ToUpperFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_ToFoldedFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_IsCaseIgnorable(Py_UCS4 ch);
extern int _PyUnicode_IsCased(Py_UCS4 ch);

/* --- Unicode API -------------------------------------------------------- */

// Export for '_json' shared extension
PyAPI_FUNC(int) _PyUnicode_CheckConsistency(
    PyObject *op,
    int check_content);

PyAPI_FUNC(void) _PyUnicode_ExactDealloc(PyObject *op);
extern Py_ssize_t _PyUnicode_InternedSize(void);
extern Py_ssize_t _PyUnicode_InternedSize_Immortal(void);

// Get a copy of a Unicode string.
// Export for '_datetime' shared extension.
PyAPI_FUNC(PyObject*) _PyUnicode_Copy(
    PyObject *unicode);

/* Unsafe version of PyUnicode_Fill(): don't check arguments and so may crash
   if parameters are invalid (e.g. if length is longer than the string). */
extern void _PyUnicode_FastFill(
    PyObject *unicode,
    Py_ssize_t start,
    Py_ssize_t length,
    Py_UCS4 fill_char
    );

/* Unsafe version of PyUnicode_CopyCharacters(): don't check arguments and so
   may crash if parameters are invalid (e.g. if the output string
   is too short). */
extern void _PyUnicode_FastCopyCharacters(
    PyObject *to,
    Py_ssize_t to_start,
    PyObject *from,
    Py_ssize_t from_start,
    Py_ssize_t how_many
    );

/* Create a new string from a buffer of ASCII characters.
   WARNING: Don't check if the string contains any non-ASCII character. */
extern PyObject* _PyUnicode_FromASCII(
    const char *buffer,
    Py_ssize_t size);

/* Compute the maximum character of the substring unicode[start:end].
   Return 127 for an empty string. */
extern Py_UCS4 _PyUnicode_FindMaxChar (
    PyObject *unicode,
    Py_ssize_t start,
    Py_ssize_t end);

/* --- _PyUnicodeWriter API ----------------------------------------------- */

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyUnicode_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

/* --- UTF-7 Codecs ------------------------------------------------------- */

extern PyObject* _PyUnicode_EncodeUTF7(
    PyObject *unicode,          /* Unicode object */
    int base64SetO,             /* Encode RFC2152 Set O characters in base64 */
    int base64WhiteSpace,       /* Encode whitespace (sp, ht, nl, cr) in base64 */
    const char *errors);        /* error handling */

/* --- UTF-8 Codecs ------------------------------------------------------- */

// Export for '_tkinter' shared extension.
PyAPI_FUNC(PyObject*) _PyUnicode_AsUTF8String(
    PyObject *unicode,
    const char *errors);

/* --- UTF-32 Codecs ------------------------------------------------------ */

// Export for '_tkinter' shared extension
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeUTF32(
    PyObject *object,           /* Unicode object */
    const char *errors,         /* error handling */
    int byteorder);             /* byteorder to use 0=BOM+native;-1=LE,1=BE */

/* --- UTF-16 Codecs ------------------------------------------------------ */

// Returns a Python string object holding the UTF-16 encoded value of
// the Unicode data.
//
// If byteorder is not 0, output is written according to the following
// byte order:
//
// byteorder == -1: little endian
// byteorder == 0:  native byte order (writes a BOM mark)
// byteorder == 1:  big endian
//
// If byteorder is 0, the output string will always start with the
// Unicode BOM mark (U+FEFF). In the other two modes, no BOM mark is
// prepended.
//
// Export for '_tkinter' shared extension
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeUTF16(
    PyObject* unicode,          /* Unicode object */
    const char *errors,         /* error handling */
    int byteorder);             /* byteorder to use 0=BOM+native;-1=LE,1=BE */

/* --- Unicode-Escape Codecs ---------------------------------------------- */

/* Variant of PyUnicode_DecodeUnicodeEscape that supports partial decoding. */
extern PyObject* _PyUnicode_DecodeUnicodeEscapeStateful(
    const char *string,     /* Unicode-Escape encoded string */
    Py_ssize_t length,      /* size of string */
    const char *errors,     /* error handling */
    Py_ssize_t *consumed);  /* bytes consumed */

// Helper for PyUnicode_DecodeUnicodeEscape that detects invalid escape
// chars.
// Export for test_peg_generator.
PyAPI_FUNC(PyObject*) _PyUnicode_DecodeUnicodeEscapeInternal2(
    const char *string,     /* Unicode-Escape encoded string */
    Py_ssize_t length,      /* size of string */
    const char *errors,     /* error handling */
    Py_ssize_t *consumed,   /* bytes consumed */
    int *first_invalid_escape_char, /* on return, if not -1, contain the first
                                       invalid escaped char (<= 0xff) or invalid
                                       octal escape (> 0xff) in string. */
    const char **first_invalid_escape_ptr); /* on return, if not NULL, may
                                        point to the first invalid escaped
                                        char in string.
                                        May be NULL if errors is not NULL. */
// Export for binary compatibility.
PyAPI_FUNC(PyObject*) _PyUnicode_DecodeUnicodeEscapeInternal(
    const char *string,     /* Unicode-Escape encoded string */
    Py_ssize_t length,      /* size of string */
    const char *errors,     /* error handling */
    Py_ssize_t *consumed,   /* bytes consumed */
    const char **first_invalid_escape); /* on return, points to first
                                           invalid escaped char in
                                           string. */

/* --- Raw-Unicode-Escape Codecs ---------------------------------------------- */

/* Variant of PyUnicode_DecodeRawUnicodeEscape that supports partial decoding. */
extern PyObject* _PyUnicode_DecodeRawUnicodeEscapeStateful(
    const char *string,     /* Unicode-Escape encoded string */
    Py_ssize_t length,      /* size of string */
    const char *errors,     /* error handling */
    Py_ssize_t *consumed);  /* bytes consumed */

/* --- Latin-1 Codecs ----------------------------------------------------- */

extern PyObject* _PyUnicode_AsLatin1String(
    PyObject* unicode,
    const char* errors);

/* --- ASCII Codecs ------------------------------------------------------- */

extern PyObject* _PyUnicode_AsASCIIString(
    PyObject* unicode,
    const char* errors);

/* --- Character Map Codecs ----------------------------------------------- */

/* Translate an Unicode object by applying a character mapping table to
   it and return the resulting Unicode object.

   The mapping table must map Unicode ordinal integers to Unicode strings,
   Unicode ordinal integers or None (causing deletion of the character).

   Mapping tables may be dictionaries or sequences. Unmapped character
   ordinals (ones which cause a LookupError) are left untouched and
   are copied as-is.
*/
extern PyObject* _PyUnicode_EncodeCharmap(
    PyObject *unicode,          /* Unicode object */
    PyObject *mapping,          /* encoding mapping */
    const char *errors);        /* error handling */

/* --- Decimal Encoder ---------------------------------------------------- */

// Coverts a Unicode object holding a decimal value to an ASCII string
// for using in int, float and complex parsers.
// Transforms code points that have decimal digit property to the
// corresponding ASCII digit code points.  Transforms spaces to ASCII.
// Transforms code points starting from the first non-ASCII code point that
// is neither a decimal digit nor a space to the end into '?'.
//
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(PyObject*) _PyUnicode_TransformDecimalAndSpaceToASCII(
    PyObject *unicode);         /* Unicode object */

/* --- Methods & Slots ---------------------------------------------------- */

PyAPI_FUNC(PyObject*) _PyUnicode_JoinArray(
    PyObject *separator,
    PyObject *const *items,
    Py_ssize_t seqlen
    );

/* Test whether a unicode is equal to ASCII identifier.  Return 1 if true,
   0 otherwise.  The right argument must be ASCII identifier.
   Any error occurs inside will be cleared before return. */
extern int _PyUnicode_EqualToASCIIId(
    PyObject *left,             /* Left string */
    _Py_Identifier *right       /* Right identifier */
    );

// Test whether a unicode is equal to ASCII string.  Return 1 if true,
// 0 otherwise.  The right argument must be ASCII-encoded string.
// Any error occurs inside will be cleared before return.
// Export for '_ctypes' shared extension
PyAPI_FUNC(int) _PyUnicode_EqualToASCIIString(
    PyObject *left,
    const char *right           /* ASCII-encoded string */
    );

/* Externally visible for str.strip(unicode) */
extern PyObject* _PyUnicode_XStrip(
    PyObject *self,
    int striptype,
    PyObject *sepobj
    );


/* Using explicit passed-in values, insert the thousands grouping
   into the string pointed to by buffer.  For the argument descriptions,
   see Objects/stringlib/localeutil.h */
extern Py_ssize_t _PyUnicode_InsertThousandsGrouping(
    _PyUnicodeWriter *writer,
    Py_ssize_t n_buffer,
    PyObject *digits,
    Py_ssize_t d_pos,
    Py_ssize_t n_digits,
    Py_ssize_t min_width,
    const char *grouping,
    PyObject *thousands_sep,
    Py_UCS4 *maxchar);

/* --- Misc functions ----------------------------------------------------- */

extern PyObject* _PyUnicode_FormatLong(PyObject *, int, int, int);

/* Fast equality check when the inputs are known to be exact unicode types
   and where the hash values are equal (i.e. a very probable match) */
extern int _PyUnicode_EQ(PyObject *, PyObject *);

// Equality check.
// Export for '_pickle' shared extension.
PyAPI_FUNC(int) _PyUnicode_Equal(PyObject *, PyObject *);

extern int _PyUnicode_WideCharString_Converter(PyObject *, void *);
extern int _PyUnicode_WideCharString_Opt_Converter(PyObject *, void *);

// Export for test_peg_generator
PyAPI_FUNC(Py_ssize_t) _PyUnicode_ScanIdentifier(PyObject *);

/* --- Runtime lifecycle -------------------------------------------------- */

extern void _PyUnicode_InitState(PyInterpreterState *);
extern PyStatus _PyUnicode_InitGlobalObjects(PyInterpreterState *);
extern PyStatus _PyUnicode_InitTypes(PyInterpreterState *);
extern void _PyUnicode_Fini(PyInterpreterState *);
extern void _PyUnicode_FiniTypes(PyInterpreterState *);

extern PyTypeObject _PyUnicodeASCIIIter_Type;

/* --- Interning ---------------------------------------------------------- */

// All these are "ref-neutral", like the public PyUnicode_InternInPlace.

// Explicit interning routines:
PyAPI_FUNC(void) _PyUnicode_InternMortal(PyInterpreterState *interp, PyObject **);
PyAPI_FUNC(void) _PyUnicode_InternImmortal(PyInterpreterState *interp, PyObject **);
// Left here to help backporting:
PyAPI_FUNC(void) _PyUnicode_InternInPlace(PyInterpreterState *interp, PyObject **p);
// Only for singletons in the _PyRuntime struct:
extern void _PyUnicode_InternStatic(PyInterpreterState *interp, PyObject **);

/* --- Other API ---------------------------------------------------------- */

struct _Py_unicode_runtime_ids {
    PyMutex mutex;
    // next_index value must be preserved when Py_Initialize()/Py_Finalize()
    // is called multiple times: see _PyUnicode_FromId() implementation.
    Py_ssize_t next_index;
};

struct _Py_unicode_runtime_state {
    struct _Py_unicode_runtime_ids ids;
};

/* fs_codec.encoding is initialized to NULL.
   Later, it is set to a non-NULL string by _PyUnicode_InitEncodings(). */
struct _Py_unicode_fs_codec {
    char *encoding;   // Filesystem encoding (encoded to UTF-8)
    int utf8;         // encoding=="utf-8"?
    char *errors;     // Filesystem errors (encoded to UTF-8)
    _Py_error_handler error_handler;
};

struct _Py_unicode_ids {
    Py_ssize_t size;
    PyObject **array;
};

struct _Py_unicode_state {
    struct _Py_unicode_fs_codec fs_codec;

    _PyUnicode_Name_CAPI *ucnhash_capi;

    // Unicode identifiers (_Py_Identifier): see _PyUnicode_FromId()
    struct _Py_unicode_ids ids;
};

extern void _PyUnicode_ClearInterned(PyInterpreterState *interp);

// Like PyUnicode_AsUTF8(), but check for embedded null characters.
// Export for '_sqlite3' shared extension.
PyAPI_FUNC(const char *) _PyUnicode_AsUTF8NoNUL(PyObject *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODEOBJECT_H */
