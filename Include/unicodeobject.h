#ifndef Py_UNICODEOBJECT_H
#define Py_UNICODEOBJECT_H

/*

Unicode implementation based on original code by Fredrik Lundh,
modified by Marc-Andre Lemburg (mal@lemburg.com) according to the
Unicode Integration Proposal (see file Misc/unicode.txt).

Copyright (c) Corporation for National Research Initiatives.


 Original header:
 --------------------------------------------------------------------

 * Yet another Unicode string type for Python.  This type supports the
 * 16-bit Basic Multilingual Plane (BMP) only.
 *
 * Written by Fredrik Lundh, January 1999.
 *
 * Copyright (c) 1999 by Secret Labs AB.
 * Copyright (c) 1999 by Fredrik Lundh.
 *
 * fredrik@pythonware.com
 * http://www.pythonware.com
 *
 * --------------------------------------------------------------------
 * This Unicode String Type is
 * 
 * Copyright (c) 1999 by Secret Labs AB
 * Copyright (c) 1999 by Fredrik Lundh
 * 
 * By obtaining, using, and/or copying this software and/or its
 * associated documentation, you agree that you have read, understood,
 * and will comply with the following terms and conditions:
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * associated documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies, and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Secret Labs
 * AB or the author not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 * 
 * SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS.  IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * -------------------------------------------------------------------- */

#include "ctype.h"

/* === Internal API ======================================================= */

/* --- Internal Unicode Format -------------------------------------------- */

/* Set these flags if the platform has "wchar.h", "wctype.h" and the
   wchar_t type is a 16-bit unsigned type */
/* #define HAVE_WCHAR_H */
/* #define HAVE_USABLE_WCHAR_T */

/* Defaults for various platforms */
#ifndef HAVE_USABLE_WCHAR_T

/* Windows has a usable wchar_t type */
# if defined(MS_WIN32)
#  define HAVE_USABLE_WCHAR_T
# endif

#endif

/* If the compiler provides a wchar_t type we try to support it
   through the interface functions PyUnicode_FromWideChar() and
   PyUnicode_AsWideChar(). */

#ifdef HAVE_USABLE_WCHAR_T
# ifndef HAVE_WCHAR_H
#  define HAVE_WCHAR_H
# endif
#endif

#ifdef HAVE_WCHAR_H
/* Work around a cosmetic bug in BSDI 4.x wchar.h; thanks to Thomas Wouters */
# ifdef _HAVE_BSDI
#  include <time.h>
# endif
# include "wchar.h"
#endif

#ifdef HAVE_USABLE_WCHAR_T

/* If the compiler defines whcar_t as a 16-bit unsigned type we can
   use the compiler type directly.  Works fine with all modern Windows
   platforms. */

typedef wchar_t Py_UNICODE;

#else

/* Use if you have a standard ANSI compiler, without wchar_t support.
   If a short is not 16 bits on your platform, you have to fix the
   typedef below, or the module initialization code will complain. */

typedef unsigned short Py_UNICODE;

#endif

/*
 * Use this typedef when you need to represent a UTF-16 surrogate pair
 * as single unsigned integer.
 */
#if SIZEOF_INT >= 4 
typedef unsigned int Py_UCS4; 
#elif SIZEOF_LONG >= 4
typedef unsigned long Py_UCS4; 
#endif 


/* --- Internal Unicode Operations ---------------------------------------- */

/* If you want Python to use the compiler's wctype.h functions instead
   of the ones supplied with Python, define WANT_WCTYPE_FUNCTIONS or
   configure Python using --with-ctype-functions.  This reduces the
   interpreter's code size. */

#if defined(HAVE_USABLE_WCHAR_T) && defined(WANT_WCTYPE_FUNCTIONS)

#include "wctype.h"

#define Py_UNICODE_ISSPACE(ch) iswspace(ch)

#define Py_UNICODE_ISLOWER(ch) iswlower(ch)
#define Py_UNICODE_ISUPPER(ch) iswupper(ch)
#define Py_UNICODE_ISTITLE(ch) _PyUnicode_IsTitlecase(ch)
#define Py_UNICODE_ISLINEBREAK(ch) _PyUnicode_IsLinebreak(ch)

#define Py_UNICODE_TOLOWER(ch) towlower(ch)
#define Py_UNICODE_TOUPPER(ch) towupper(ch)
#define Py_UNICODE_TOTITLE(ch) _PyUnicode_ToTitlecase(ch)

#define Py_UNICODE_ISDECIMAL(ch) _PyUnicode_IsDecimalDigit(ch)
#define Py_UNICODE_ISDIGIT(ch) _PyUnicode_IsDigit(ch)
#define Py_UNICODE_ISNUMERIC(ch) _PyUnicode_IsNumeric(ch)

#define Py_UNICODE_TODECIMAL(ch) _PyUnicode_ToDecimalDigit(ch)
#define Py_UNICODE_TODIGIT(ch) _PyUnicode_ToDigit(ch)
#define Py_UNICODE_TONUMERIC(ch) _PyUnicode_ToNumeric(ch)

#define Py_UNICODE_ISALPHA(ch) iswalpha(ch)

#else

#define Py_UNICODE_ISSPACE(ch) _PyUnicode_IsWhitespace(ch)

#define Py_UNICODE_ISLOWER(ch) _PyUnicode_IsLowercase(ch)
#define Py_UNICODE_ISUPPER(ch) _PyUnicode_IsUppercase(ch)
#define Py_UNICODE_ISTITLE(ch) _PyUnicode_IsTitlecase(ch)
#define Py_UNICODE_ISLINEBREAK(ch) _PyUnicode_IsLinebreak(ch)

#define Py_UNICODE_TOLOWER(ch) _PyUnicode_ToLowercase(ch)
#define Py_UNICODE_TOUPPER(ch) _PyUnicode_ToUppercase(ch)
#define Py_UNICODE_TOTITLE(ch) _PyUnicode_ToTitlecase(ch)

#define Py_UNICODE_ISDECIMAL(ch) _PyUnicode_IsDecimalDigit(ch)
#define Py_UNICODE_ISDIGIT(ch) _PyUnicode_IsDigit(ch)
#define Py_UNICODE_ISNUMERIC(ch) _PyUnicode_IsNumeric(ch)

#define Py_UNICODE_TODECIMAL(ch) _PyUnicode_ToDecimalDigit(ch)
#define Py_UNICODE_TODIGIT(ch) _PyUnicode_ToDigit(ch)
#define Py_UNICODE_TONUMERIC(ch) _PyUnicode_ToNumeric(ch)

#define Py_UNICODE_ISALPHA(ch) _PyUnicode_IsAlpha(ch)

#endif

#define Py_UNICODE_ISALNUM(ch) \
       (Py_UNICODE_ISALPHA(ch) || \
        Py_UNICODE_ISDECIMAL(ch) || \
        Py_UNICODE_ISDIGIT(ch) || \
        Py_UNICODE_ISNUMERIC(ch))

#define Py_UNICODE_COPY(target, source, length)\
    (memcpy((target), (source), (length)*sizeof(Py_UNICODE)))

#define Py_UNICODE_FILL(target, value, length) do\
    {int i; for (i = 0; i < (length); i++) (target)[i] = (value);}\
    while (0)

#define Py_UNICODE_MATCH(string, offset, substring)\
    ((*((string)->str + (offset)) == *((substring)->str)) &&\
     !memcmp((string)->str + (offset), (substring)->str,\
             (substring)->length*sizeof(Py_UNICODE)))

#ifdef __cplusplus
extern "C" {
#endif

/* --- Unicode Type ------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    int length;			/* Length of raw Unicode data in buffer */
    Py_UNICODE *str;		/* Raw Unicode buffer */
    long hash;			/* Hash value; -1 if not set */
    PyObject *defenc;		/* (Default) Encoded version as Python
				   string, or NULL; this is used for
				   implementing the buffer protocol */
} PyUnicodeObject;

extern DL_IMPORT(PyTypeObject) PyUnicode_Type;

#define PyUnicode_Check(op) (((op)->ob_type == &PyUnicode_Type))

/* Fast access macros */
#define PyUnicode_GET_SIZE(op) \
        (((PyUnicodeObject *)(op))->length)
#define PyUnicode_GET_DATA_SIZE(op) \
        (((PyUnicodeObject *)(op))->length * sizeof(Py_UNICODE))
#define PyUnicode_AS_UNICODE(op) \
        (((PyUnicodeObject *)(op))->str)
#define PyUnicode_AS_DATA(op) \
        ((const char *)((PyUnicodeObject *)(op))->str)

/* --- Constants ---------------------------------------------------------- */

/* This Unicode character will be used as replacement character during
   decoding if the errors argument is set to "replace". Note: the
   Unicode character U+FFFD is the official REPLACEMENT CHARACTER in
   Unicode 3.0. */

#define Py_UNICODE_REPLACEMENT_CHARACTER ((Py_UNICODE) 0xFFFD)

/* === Public API ========================================================= */

/* --- Plain Py_UNICODE --------------------------------------------------- */

/* Create a Unicode Object from the Py_UNICODE buffer u of the given
   size. u may be NULL which causes the contents to be undefined. It
   is the user's responsibility to fill in the needed data.

   The buffer is copied into the new object. */

extern DL_IMPORT(PyObject*) PyUnicode_FromUnicode(
    const Py_UNICODE *u,        /* Unicode buffer */
    int size                    /* size of buffer */
    );

/* Return a read-only pointer to the Unicode object's internal
   Py_UNICODE buffer. */

extern DL_IMPORT(Py_UNICODE *) PyUnicode_AsUnicode(
    PyObject *unicode	 	/* Unicode object */
    );

/* Get the length of the Unicode object. */

extern DL_IMPORT(int) PyUnicode_GetSize(
    PyObject *unicode	 	/* Unicode object */
    );

/* Resize an already allocated Unicode object to the new size length.

   *unicode is modified to point to the new (resized) object and 0
   returned on success.

   This API may only be called by the function which also called the
   Unicode constructor. The refcount on the object must be 1. Otherwise,
   an error is returned.

   Error handling is implemented as follows: an exception is set, -1
   is returned and *unicode left untouched.

*/

extern DL_IMPORT(int) PyUnicode_Resize(
    PyObject **unicode,		/* Pointer to the Unicode object */
    int length			/* New length */
    );

/* Coerce obj to an Unicode object and return a reference with
   *incremented* refcount.

   Coercion is done in the following way:

   1. Unicode objects are passed back as-is with incremented
      refcount.

   2. String and other char buffer compatible objects are decoded
      under the assumptions that they contain data using the current
      default encoding. Decoding is done in "strict" mode.

   3. All other objects raise an exception.

   The API returns NULL in case of an error. The caller is responsible
   for decref'ing the returned objects.

*/

extern DL_IMPORT(PyObject*) PyUnicode_FromEncodedObject(
    register PyObject *obj, 	/* Object */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Shortcut for PyUnicode_FromEncodedObject(obj, NULL, "strict");
   which results in using the default encoding as basis for 
   decoding the object.

   Coerces obj to an Unicode object and return a reference with
   *incremented* refcount.

   The API returns NULL in case of an error. The caller is responsible
   for decref'ing the returned objects.

*/

extern DL_IMPORT(PyObject*) PyUnicode_FromObject(
    register PyObject *obj 	/* Object */
    );

/* --- wchar_t support for platforms which support it --------------------- */

#ifdef HAVE_WCHAR_H

/* Create a Unicode Object from the whcar_t buffer w of the given
   size.

   The buffer is copied into the new object. */

extern DL_IMPORT(PyObject*) PyUnicode_FromWideChar(
    register const wchar_t *w,  /* wchar_t buffer */
    int size                    /* size of buffer */
    );

/* Copies the Unicode Object contents into the whcar_t buffer w.  At
   most size wchar_t characters are copied.

   Returns the number of wchar_t characters copied or -1 in case of an
   error. */

extern DL_IMPORT(int) PyUnicode_AsWideChar(
    PyUnicodeObject *unicode,   /* Unicode object */
    register wchar_t *w,        /* wchar_t buffer */
    int size                    /* size of buffer */
    );

#endif

/* === Builtin Codecs ===================================================== 

   Many of these APIs take two arguments encoding and errors. These
   parameters encoding and errors have the same semantics as the ones
   of the builtin unicode() API. 

   Setting encoding to NULL causes the default encoding to be used.

   Error handling is set by errors which may also be set to NULL
   meaning to use the default handling defined for the codec. Default
   error handling for all builtin codecs is "strict" (ValueErrors are
   raised).

   The codecs all use a similar interface. Only deviation from the
   generic ones are documented.

*/

/* --- Manage the default encoding ---------------------------------------- */

/* Returns the currently active default encoding.

   The default encoding is currently implemented as run-time settable
   process global.  This may change in future versions of the
   interpreter to become a parameter which is managed on a per-thread
   basis.
   
 */

extern DL_IMPORT(const char*) PyUnicode_GetDefaultEncoding(void);

/* Sets the currently active default encoding.

   Returns 0 on success, -1 in case of an error.
   
 */

extern DL_IMPORT(int) PyUnicode_SetDefaultEncoding(
    const char *encoding	/* Encoding name in standard form */
    );

/* --- Generic Codecs ----------------------------------------------------- */

/* Create a Unicode object by decoding the encoded string s of the
   given size. */

extern DL_IMPORT(PyObject*) PyUnicode_Decode(
    const char *s,              /* encoded string */
    int size,                   /* size of buffer */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Encodes a Py_UNICODE buffer of the given size and returns a 
   Python string object. */

extern DL_IMPORT(PyObject*) PyUnicode_Encode(
    const Py_UNICODE *s,        /* Unicode char buffer */
    int size,                   /* number of Py_UNICODE chars to encode */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Encodes a Unicode object and returns the result as Python string
   object. */

extern DL_IMPORT(PyObject*) PyUnicode_AsEncodedString(
    PyObject *unicode,	 	/* Unicode object */
    const char *encoding,	/* encoding */
    const char *errors		/* error handling */
    );

/* --- UTF-8 Codecs ------------------------------------------------------- */

extern DL_IMPORT(PyObject*) PyUnicode_DecodeUTF8(
    const char *string, 	/* UTF-8 encoded string */
    int length,	 		/* size of string */
    const char *errors		/* error handling */
    );

extern DL_IMPORT(PyObject*) PyUnicode_AsUTF8String(
    PyObject *unicode	 	/* Unicode object */
    );

extern DL_IMPORT(PyObject*) PyUnicode_EncodeUTF8(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length,	 		/* number of Py_UNICODE chars to encode */
    const char *errors		/* error handling */
    );

/* --- UTF-16 Codecs ------------------------------------------------------ */

/* Decodes length bytes from a UTF-16 encoded buffer string and returns
   the corresponding Unicode object.

   errors (if non-NULL) defines the error handling. It defaults
   to "strict". 

   If byteorder is non-NULL, the decoder starts decoding using the
   given byte order:

	*byteorder == -1: little endian
	*byteorder == 0:  native order
	*byteorder == 1:  big endian

   and then switches according to all BOM marks it finds in the input
   data. BOM marks are not copied into the resulting Unicode string.
   After completion, *byteorder is set to the current byte order at
   the end of input data.

   If byteorder is NULL, the codec starts in native order mode.

*/

extern DL_IMPORT(PyObject*) PyUnicode_DecodeUTF16(
    const char *string, 	/* UTF-16 encoded string */
    int length,	 		/* size of string */
    const char *errors,		/* error handling */
    int *byteorder		/* pointer to byteorder to use
				   0=native;-1=LE,1=BE; updated on
				   exit */
    );

/* Returns a Python string using the UTF-16 encoding in native byte
   order. The string always starts with a BOM mark.  */

extern DL_IMPORT(PyObject*) PyUnicode_AsUTF16String(
    PyObject *unicode	 	/* Unicode object */
    );

/* Returns a Python string object holding the UTF-16 encoded value of
   the Unicode data.

   If byteorder is not 0, output is written according to the following
   byte order:

   byteorder == -1: little endian
   byteorder == 0:  native byte order (writes a BOM mark)
   byteorder == 1:  big endian

   If byteorder is 0, the output string will always start with the
   Unicode BOM mark (U+FEFF). In the other two modes, no BOM mark is
   prepended.

   Note that Py_UNICODE data is being interpreted as UTF-16 reduced to
   UCS-2. This trick makes it possible to add full UTF-16 capabilities
   at a later point without compromising the APIs.

*/

extern DL_IMPORT(PyObject*) PyUnicode_EncodeUTF16(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length,	 		/* number of Py_UNICODE chars to encode */
    const char *errors,		/* error handling */
    int byteorder		/* byteorder to use 0=BOM+native;-1=LE,1=BE */
    );

/* --- Unicode-Escape Codecs ---------------------------------------------- */

extern DL_IMPORT(PyObject*) PyUnicode_DecodeUnicodeEscape(
    const char *string, 	/* Unicode-Escape encoded string */
    int length,	 		/* size of string */
    const char *errors		/* error handling */
    );

extern DL_IMPORT(PyObject*) PyUnicode_AsUnicodeEscapeString(
    PyObject *unicode	 	/* Unicode object */
    );

extern DL_IMPORT(PyObject*) PyUnicode_EncodeUnicodeEscape(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length	 		/* Number of Py_UNICODE chars to encode */
    );

/* --- Raw-Unicode-Escape Codecs ------------------------------------------ */

extern DL_IMPORT(PyObject*) PyUnicode_DecodeRawUnicodeEscape(
    const char *string, 	/* Raw-Unicode-Escape encoded string */
    int length,	 		/* size of string */
    const char *errors		/* error handling */
    );

extern DL_IMPORT(PyObject*) PyUnicode_AsRawUnicodeEscapeString(
    PyObject *unicode	 	/* Unicode object */
    );

extern DL_IMPORT(PyObject*) PyUnicode_EncodeRawUnicodeEscape(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length	 		/* Number of Py_UNICODE chars to encode */
    );

/* --- Latin-1 Codecs ----------------------------------------------------- 

   Note: Latin-1 corresponds to the first 256 Unicode ordinals.

*/

extern DL_IMPORT(PyObject*) PyUnicode_DecodeLatin1(
    const char *string, 	/* Latin-1 encoded string */
    int length,	 		/* size of string */
    const char *errors		/* error handling */
    );

extern DL_IMPORT(PyObject*) PyUnicode_AsLatin1String(
    PyObject *unicode	 	/* Unicode object */
    );

extern DL_IMPORT(PyObject*) PyUnicode_EncodeLatin1(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length,	 		/* Number of Py_UNICODE chars to encode */
    const char *errors		/* error handling */
    );

/* --- ASCII Codecs ------------------------------------------------------- 

   Only 7-bit ASCII data is excepted. All other codes generate errors.

*/

extern DL_IMPORT(PyObject*) PyUnicode_DecodeASCII(
    const char *string, 	/* ASCII encoded string */
    int length,	 		/* size of string */
    const char *errors		/* error handling */
    );

extern DL_IMPORT(PyObject*) PyUnicode_AsASCIIString(
    PyObject *unicode	 	/* Unicode object */
    );

extern DL_IMPORT(PyObject*) PyUnicode_EncodeASCII(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length,	 		/* Number of Py_UNICODE chars to encode */
    const char *errors		/* error handling */
    );

/* --- Character Map Codecs ----------------------------------------------- 

   This codec uses mappings to encode and decode characters. 

   Decoding mappings must map single string characters to single
   Unicode characters, integers (which are then interpreted as Unicode
   ordinals) or None (meaning "undefined mapping" and causing an
   error).

   Encoding mappings must map single Unicode characters to single
   string characters, integers (which are then interpreted as Latin-1
   ordinals) or None (meaning "undefined mapping" and causing an
   error).

   If a character lookup fails with a LookupError, the character is
   copied as-is meaning that its ordinal value will be interpreted as
   Unicode or Latin-1 ordinal resp. Because of this mappings only need
   to contain those mappings which map characters to different code
   points.

*/

extern DL_IMPORT(PyObject*) PyUnicode_DecodeCharmap(
    const char *string, 	/* Encoded string */
    int length,	 		/* size of string */
    PyObject *mapping,		/* character mapping 
				   (char ordinal -> unicode ordinal) */
    const char *errors		/* error handling */
    );

extern DL_IMPORT(PyObject*) PyUnicode_AsCharmapString(
    PyObject *unicode,	 	/* Unicode object */
    PyObject *mapping		/* character mapping 
				   (unicode ordinal -> char ordinal) */
    );

extern DL_IMPORT(PyObject*) PyUnicode_EncodeCharmap(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length,	 		/* Number of Py_UNICODE chars to encode */
    PyObject *mapping,		/* character mapping 
				   (unicode ordinal -> char ordinal) */
    const char *errors		/* error handling */
    );

/* Translate a Py_UNICODE buffer of the given length by applying a
   character mapping table to it and return the resulting Unicode
   object.

   The mapping table must map Unicode ordinal integers to Unicode
   ordinal integers or None (causing deletion of the character). 

   Mapping tables may be dictionaries or sequences. Unmapped character
   ordinals (ones which cause a LookupError) are left untouched and
   are copied as-is.

*/

extern DL_IMPORT(PyObject *) PyUnicode_TranslateCharmap(
    const Py_UNICODE *data, 	/* Unicode char buffer */
    int length,	 		/* Number of Py_UNICODE chars to encode */
    PyObject *table,		/* Translate table */
    const char *errors		/* error handling */
    );

#ifdef MS_WIN32

/* --- MBCS codecs for Windows -------------------------------------------- */

extern DL_IMPORT(PyObject*) PyUnicode_DecodeMBCS(
    const char *string,         /* MBCS encoded string */
    int length,                 /* size of string */
    const char *errors          /* error handling */
    );

extern DL_IMPORT(PyObject*) PyUnicode_AsMBCSString(
    PyObject *unicode           /* Unicode object */
    );

extern DL_IMPORT(PyObject*) PyUnicode_EncodeMBCS(
    const Py_UNICODE *data,     /* Unicode char buffer */
    int length,                 /* Number of Py_UNICODE chars to encode */
    const char *errors          /* error handling */
    );

#endif /* MS_WIN32 */

/* --- Decimal Encoder ---------------------------------------------------- */

/* Takes a Unicode string holding a decimal value and writes it into
   an output buffer using standard ASCII digit codes.

   The output buffer has to provide at least length+1 bytes of storage
   area. The output string is 0-terminated.

   The encoder converts whitespace to ' ', decimal characters to their
   corresponding ASCII digit and all other Latin-1 characters except
   \0 as-is. Characters outside this range (Unicode ordinals 1-256)
   are treated as errors. This includes embedded NULL bytes.

   Error handling is defined by the errors argument:

      NULL or "strict": raise a ValueError
      "ignore": ignore the wrong characters (these are not copied to the
		output buffer)
      "replace": replaces illegal characters with '?'

   Returns 0 on success, -1 on failure.

*/

extern DL_IMPORT(int) PyUnicode_EncodeDecimal(
    Py_UNICODE *s,		/* Unicode buffer */
    int length,			/* Number of Py_UNICODE chars to encode */
    char *output,		/* Output buffer; must have size >= length */
    const char *errors		/* error handling */
    );

/* --- Methods & Slots ----------------------------------------------------

   These are capable of handling Unicode objects and strings on input
   (we refer to them as strings in the descriptions) and return
   Unicode objects or integers as apporpriate. */

/* Concat two strings giving a new Unicode string. */

extern DL_IMPORT(PyObject*) PyUnicode_Concat(
    PyObject *left,	 	/* Left string */
    PyObject *right	 	/* Right string */
    );

/* Split a string giving a list of Unicode strings.

   If sep is NULL, splitting will be done at all whitespace
   substrings. Otherwise, splits occur at the given separator.

   At most maxsplit splits will be done. If negative, no limit is set.

   Separators are not included in the resulting list.

*/

extern DL_IMPORT(PyObject*) PyUnicode_Split(
    PyObject *s,		/* String to split */
    PyObject *sep,		/* String separator */
    int maxsplit		/* Maxsplit count */
    );		

/* Dito, but split at line breaks.

   CRLF is considered to be one line break. Line breaks are not
   included in the resulting list. */
    
extern DL_IMPORT(PyObject*) PyUnicode_Splitlines(
    PyObject *s,		/* String to split */
    int keepends		/* If true, line end markers are included */
    );		

/* Translate a string by applying a character mapping table to it and
   return the resulting Unicode object.

   The mapping table must map Unicode ordinal integers to Unicode
   ordinal integers or None (causing deletion of the character). 

   Mapping tables may be dictionaries or sequences. Unmapped character
   ordinals (ones which cause a LookupError) are left untouched and
   are copied as-is.

*/

extern DL_IMPORT(PyObject *) PyUnicode_Translate(
    PyObject *str,		/* String */ 
    PyObject *table,		/* Translate table */
    const char *errors		/* error handling */
    );

/* Join a sequence of strings using the given separator and return
   the resulting Unicode string. */
    
extern DL_IMPORT(PyObject*) PyUnicode_Join(
    PyObject *separator, 	/* Separator string */
    PyObject *seq	 	/* Sequence object */
    );

/* Return 1 if substr matches str[start:end] at the given tail end, 0
   otherwise. */

extern DL_IMPORT(int) PyUnicode_Tailmatch(
    PyObject *str,		/* String */ 
    PyObject *substr,		/* Prefix or Suffix string */
    int start,			/* Start index */
    int end,			/* Stop index */
    int direction		/* Tail end: -1 prefix, +1 suffix */
    );

/* Return the first position of substr in str[start:end] using the
   given search direction or -1 if not found. */

extern DL_IMPORT(int) PyUnicode_Find(
    PyObject *str,		/* String */ 
    PyObject *substr,		/* Substring to find */
    int start,			/* Start index */
    int end,			/* Stop index */
    int direction		/* Find direction: +1 forward, -1 backward */
    );

/* Count the number of occurrences of substr in str[start:end]. */

extern DL_IMPORT(int) PyUnicode_Count(
    PyObject *str,		/* String */ 
    PyObject *substr,		/* Substring to count */
    int start,			/* Start index */
    int end			/* Stop index */
    );

/* Replace at most maxcount occurrences of substr in str with replstr
   and return the resulting Unicode object. */

extern DL_IMPORT(PyObject *) PyUnicode_Replace(
    PyObject *str,		/* String */ 
    PyObject *substr,		/* Substring to find */
    PyObject *replstr,		/* Substring to replace */
    int maxcount		/* Max. number of replacements to apply;
				   -1 = all */
    );

/* Compare two strings and return -1, 0, 1 for less than, equal,
   greater than resp. */

extern DL_IMPORT(int) PyUnicode_Compare(
    PyObject *left,		/* Left string */ 
    PyObject *right		/* Right string */
    );

/* Apply a argument tuple or dictionary to a format string and return
   the resulting Unicode string. */

extern DL_IMPORT(PyObject *) PyUnicode_Format(
    PyObject *format,		/* Format string */ 
    PyObject *args		/* Argument tuple or dictionary */
    );

/* Checks whether element is contained in container and return 1/0
   accordingly.

   element has to coerce to an one element Unicode string. -1 is
   returned in case of an error. */

extern DL_IMPORT(int) PyUnicode_Contains(
    PyObject *container,	/* Container string */ 
    PyObject *element		/* Element string */
    );

/* === Characters Type APIs =============================================== */

/* These should not be used directly. Use the Py_UNICODE_IS* and
   Py_UNICODE_TO* macros instead. 

   These APIs are implemented in Objects/unicodectype.c.

*/

extern DL_IMPORT(int) _PyUnicode_IsLowercase(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsUppercase(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsTitlecase(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsWhitespace(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsLinebreak(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(Py_UNICODE) _PyUnicode_ToLowercase(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(Py_UNICODE) _PyUnicode_ToUppercase(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(Py_UNICODE) _PyUnicode_ToTitlecase(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_ToDecimalDigit(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_ToDigit(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(double) _PyUnicode_ToNumeric(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsDecimalDigit(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsDigit(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsNumeric(
    register const Py_UNICODE ch 	/* Unicode character */
    );

extern DL_IMPORT(int) _PyUnicode_IsAlpha(
    register const Py_UNICODE ch 	/* Unicode character */
    );

#ifdef __cplusplus
}
#endif
#endif /* !Py_UNICODEOBJECT_H */
