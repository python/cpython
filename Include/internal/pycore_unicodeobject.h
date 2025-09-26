#ifndef Py_INTERNAL_UNICODEOBJECT_H
#define Py_INTERNAL_UNICODEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_fileutils.h"     // _Py_error_handler
#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI

// Maximum code point of Unicode 6.0: 0x10ffff (1,114,111).
// The value must be the same in fileutils.c.
#define _Py_MAX_UNICODE 0x10ffff

#define _Py_LEFTSTRIP 0
#define _Py_RIGHTSTRIP 1
#define _Py_BOTHSTRIP 2

extern int _PyUnicode_CheckEncodingErrors(
    const char *encoding,
    const char *errors);
extern PyObject* _PyUnicode_GetEmpty(void);
extern PyObject* _PyUnicode_Result(PyObject *unicode);
extern PyObject* _PyUnicode_ResultUnchanged(PyObject *unicode);
extern Py_ssize_t _PyUnicode_FindChar(
    const void *s,
    int kind,
    Py_ssize_t size,
    Py_UCS4 ch,
    int direction);
extern PyObject* _PyUnicode_GetLatin1Char(Py_UCS1 ch);
extern char* PyUnicode_UTF8(PyObject *op);
extern Py_ssize_t PyUnicode_UTF8_LENGTH(PyObject *op);
extern void _PyUnicode_SET_UTF8(PyObject *op, char *utf8);
extern void _PyUnicode_SET_UTF8_LENGTH(PyObject *op, Py_ssize_t length);
extern PyObject* _PyUnicode_FromUCS1(const Py_UCS1* u, Py_ssize_t size);
extern PyObject* _PyUnicode_TranslateCharmap(
    PyObject *input,
    PyObject *mapping,
    const char *errors);
extern int _PyUnicode_FillUTF8(PyObject *unicode);
extern int _PyUnicode_DecodeUTF8Writer(
    _PyUnicodeWriter *writer,
    const char *s,
    Py_ssize_t size,
    _Py_error_handler error_handler,
    const char *errors,
    Py_ssize_t *consumed);
extern int _Py_normalize_encoding(const char *, char *, size_t);
extern void* _PyUnicode_AsKind(
    int skind,
    void const *data,
    Py_ssize_t len,
    int kind);
extern int _PyUnicode_Tailmatch(
    PyObject *self,
    PyObject *substring,
    Py_ssize_t start,
    Py_ssize_t end,
    int direction);
extern Py_ssize_t _PyUnicode_Count(
    PyObject *str,
    PyObject *substr,
    Py_ssize_t start,
    Py_ssize_t end);
extern PyObject * _PyUnicode_Replace(
    PyObject *self,
    PyObject *str1,
    PyObject *str2,
    Py_ssize_t maxcount);
extern Py_ssize_t _PyUnicode_AnylibFindSlice(
    PyObject* s1,
    PyObject* s2,
    Py_ssize_t start,
    Py_ssize_t end,
    int direction);
extern int _PyUnicode_FindMaxCharSurrogates(
    const wchar_t *begin,
    const wchar_t *end,
    Py_UCS4 *maxchar,
    Py_ssize_t *num_surrogates);
extern void _PyUnicode_WriteWideChar(
    int kind,
    void *data,
    const wchar_t *u,
    Py_ssize_t size,
    Py_ssize_t num_surrogates);
extern PyObject* _PyUnicode_FromUCS2(const Py_UCS2 *u, Py_ssize_t size);
extern PyObject* _PyUnicode_FromUCS4(const Py_UCS4 *u, Py_ssize_t size);
extern PyObject* _PyUnicode_FromOrdinal(Py_UCS4 ordinal);
extern PyObject* _PyUnicode_do_string_format(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs);
extern PyObject* _PyUnicode_do_string_format_map(
    PyObject *self,
    PyObject *obj);
extern Py_hash_t _PyUnicode_Hash(PyObject *self);
extern PyObject* _PyUnicode_Iter(PyObject *seq);
extern int _PyUnicode_IsModifiable(PyObject *unicode);
extern void _PyUnicode_Fill(
    int kind,
    void *data,
    Py_UCS4 value,
    Py_ssize_t start,
    Py_ssize_t length);
extern PyObject* _PyUnicode_ResizeCompact(
    PyObject *unicode,
    Py_ssize_t length);
extern int _PyUnicode_CheckModifiable(PyObject *unicode);
extern PyObject* _PyUnicode_Repr(PyObject *unicode);
extern PyObject* _PyUnicode_Pad(
    PyObject *self,
    Py_ssize_t left,
    Py_ssize_t right,
    Py_UCS4 fill);
extern int _PyUnicode_Resize(PyObject **p_unicode, Py_ssize_t length);
extern PyObject* _PyUnicode_EncodeUTF8(
    PyObject *unicode,
    _Py_error_handler error_handler,
    const char *errors);
extern PyObject* _PyUnicode_DecodeUTF8(
    const char *s,
    Py_ssize_t size,
    _Py_error_handler error_handler,
    const char *errors,
    Py_ssize_t *consumed);
extern char* _PyUnicode_Backslashreplace(
    PyBytesWriter *writer,
    char *str,
    PyObject *unicode,
    Py_ssize_t collstart,
    Py_ssize_t collend);
extern char* _PyUnicode_Xmlcharrefreplace(
    PyBytesWriter *writer,
    char *str,
    PyObject *unicode,
    Py_ssize_t collstart,
    Py_ssize_t collend);
extern PyObject* _PyUnicode_EncodeCallErrorHandler(
    const char *errors,
    PyObject **errorHandler,
    const char *encoding,
    const char *reason,
    PyObject *unicode,
    PyObject **exceptionObject,
    Py_ssize_t startpos,
    Py_ssize_t endpos,
    Py_ssize_t *newpos);
extern void _PyUnicode_RaiseEncodeException(
    PyObject **exceptionObject,
    const char *encoding,
    PyObject *unicode,
    Py_ssize_t startpos,
    Py_ssize_t endpos,
    const char *reason);
extern int _PyUnicode_DecodeCallErrorHandlerWriter(
    const char *errors,
    PyObject **errorHandler,
    const char *encoding,
    const char *reason,
    const char **input,
    const char **inend,
    Py_ssize_t *startinpos,
    Py_ssize_t *endinpos,
    PyObject **exceptionObject,
    const char **inptr,
    _PyUnicodeWriter *writer);
extern PyObject* _PyUnicode_EncodeUCS1(
    PyObject *unicode,
    const char *errors,
    const Py_UCS4 limit);
extern void _PyUnicode_InitGlobalState(void);
extern PyObject* _PyUnicode_do_strip(PyObject *self, int striptype);
extern PyObject* _PyUnicode_Split(
    PyObject *self,
    PyObject *substring,
    Py_ssize_t maxcount);
extern PyObject* _PyUnicode_RSplit(
    PyObject *self,
    PyObject *substring,
    Py_ssize_t maxcount);
extern PyObject* _PyUnicode_Maketrans(
    PyObject *x,
    PyObject *y,
    PyObject *z);
extern PyObject* _PyUnicode_Expandtabs(
    PyObject *self,
    int tabsize);
void _PyUnicode_MakeDecodeException(
    PyObject **exceptionObject,
    const char *encoding,
    const char *input, Py_ssize_t length,
    Py_ssize_t startpos, Py_ssize_t endpos,
    const char *reason);

extern PyTypeObject _Py_EncodingMapType;
extern PyTypeObject _Py_FieldNameIter_Type;
extern PyTypeObject _Py_FormatterIter_Type;

/* helper macro to fixup start/end slice values */
#define _Py_ADJUST_INDICES(start, end, len) \
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

#ifdef Py_DEBUG
#  define _PyUnicode_CHECK(op) _PyUnicode_CheckConsistency(op, 0)
#else
#  define _PyUnicode_CHECK(op) PyUnicode_Check(op)
#endif

static inline int
_PyUnicode_Ensure(PyObject *obj)
{
    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "must be str, not %T", obj);
        return -1;
    }
    return 0;
}


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

static inline int
_PyUnicodeWriter_WriteCharInline(_PyUnicodeWriter *writer, Py_UCS4 ch)
{
    assert(ch <= _Py_MAX_UNICODE);
    if (_PyUnicodeWriter_Prepare(writer, 1, ch) < 0)
        return -1;
    PyUnicode_WRITE(writer->kind, writer->data, writer->pos, ch);
    writer->pos++;
    return 0;
}

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyUnicode_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

/* PyUnicodeWriter_Format, with va_list instead of `...` */
extern int _PyUnicodeWriter_FormatV(
    PyUnicodeWriter *writer,
    const char *format,
    va_list vargs);

extern void _PyUnicodeWriter_InitWithBuffer(
    _PyUnicodeWriter *writer,
    PyObject *buffer);

/* --- UTF-7 Codecs ------------------------------------------------------- */

extern PyObject* _PyUnicode_EncodeUTF7(
    PyObject *unicode,          /* Unicode object */
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

// Converts a Unicode object holding a decimal value to an ASCII string
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
    Py_UCS4 *maxchar,
    int forward);

/* Dedent a string.
   Behaviour is expected to be an exact match of `textwrap.dedent`.
   Return a new reference on success, NULL with exception set on error.
   */
extern PyObject* _PyUnicode_Dedent(PyObject *unicode);

/* --- Misc functions ----------------------------------------------------- */

extern PyObject* _PyUnicode_FormatLong(PyObject *, int, int, int);

// Fast equality check when the inputs are known to be exact unicode types.
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

extern void _PyUnicode_ClearInterned(PyInterpreterState *interp);

// Like PyUnicode_AsUTF8(), but check for embedded null characters.
// Export for '_sqlite3' shared extension.
PyAPI_FUNC(const char *) _PyUnicode_AsUTF8NoNUL(PyObject *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODEOBJECT_H */
