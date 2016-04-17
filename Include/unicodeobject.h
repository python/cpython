#ifndef Py_UNICODEOBJECT_H
#define Py_UNICODEOBJECT_H

#include <stdarg.h>

/*

Unicode implementation based on original code by Fredrik Lundh,
modified by Marc-Andre Lemburg (mal@lemburg.com) according to the
Unicode Integration Proposal. (See
http://www.egenix.com/files/python/unicode-proposal.txt).

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

#include <ctype.h>

/* === Internal API ======================================================= */

/* --- Internal Unicode Format -------------------------------------------- */

/* Python 3.x requires unicode */
#define Py_USING_UNICODE

#ifndef SIZEOF_WCHAR_T
#error Must define SIZEOF_WCHAR_T
#endif

#define Py_UNICODE_SIZE SIZEOF_WCHAR_T

/* If wchar_t can be used for UCS-4 storage, set Py_UNICODE_WIDE.
   Otherwise, Unicode strings are stored as UCS-2 (with limited support
   for UTF-16) */

#if Py_UNICODE_SIZE >= 4
#define Py_UNICODE_WIDE
#endif

/* Set these flags if the platform has "wchar.h" and the
   wchar_t type is a 16-bit unsigned type */
/* #define HAVE_WCHAR_H */
/* #define HAVE_USABLE_WCHAR_T */

/* Py_UNICODE was the native Unicode storage format (code unit) used by
   Python and represents a single Unicode element in the Unicode type.
   With PEP 393, Py_UNICODE is deprecated and replaced with a
   typedef to wchar_t. */

#ifndef Py_LIMITED_API
#define PY_UNICODE_TYPE wchar_t
typedef wchar_t Py_UNICODE;
#endif

/* If the compiler provides a wchar_t type we try to support it
   through the interface functions PyUnicode_FromWideChar(),
   PyUnicode_AsWideChar() and PyUnicode_AsWideCharString(). */

#ifdef HAVE_USABLE_WCHAR_T
# ifndef HAVE_WCHAR_H
#  define HAVE_WCHAR_H
# endif
#endif

#if defined(MS_WINDOWS)
#  define HAVE_MBCS
#endif

#ifdef HAVE_WCHAR_H
/* Work around a cosmetic bug in BSDI 4.x wchar.h; thanks to Thomas Wouters */
# ifdef _HAVE_BSDI
#  include <time.h>
# endif
#  include <wchar.h>
#endif

/* Py_UCS4 and Py_UCS2 are typedefs for the respective
   unicode representations. */
#if SIZEOF_INT == 4
typedef unsigned int Py_UCS4;
#elif SIZEOF_LONG == 4
typedef unsigned long Py_UCS4;
#else
#error "Could not find a proper typedef for Py_UCS4"
#endif

#if SIZEOF_SHORT == 2
typedef unsigned short Py_UCS2;
#else
#error "Could not find a proper typedef for Py_UCS2"
#endif

typedef unsigned char Py_UCS1;

/* --- Internal Unicode Operations ---------------------------------------- */

/* Since splitting on whitespace is an important use case, and
   whitespace in most situations is solely ASCII whitespace, we
   optimize for the common case by using a quick look-up table
   _Py_ascii_whitespace (see below) with an inlined check.

 */
#ifndef Py_LIMITED_API
#define Py_UNICODE_ISSPACE(ch) \
    ((ch) < 128U ? _Py_ascii_whitespace[(ch)] : _PyUnicode_IsWhitespace(ch))

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
#define Py_UNICODE_ISPRINTABLE(ch) _PyUnicode_IsPrintable(ch)

#define Py_UNICODE_TODECIMAL(ch) _PyUnicode_ToDecimalDigit(ch)
#define Py_UNICODE_TODIGIT(ch) _PyUnicode_ToDigit(ch)
#define Py_UNICODE_TONUMERIC(ch) _PyUnicode_ToNumeric(ch)

#define Py_UNICODE_ISALPHA(ch) _PyUnicode_IsAlpha(ch)

#define Py_UNICODE_ISALNUM(ch) \
       (Py_UNICODE_ISALPHA(ch) || \
    Py_UNICODE_ISDECIMAL(ch) || \
    Py_UNICODE_ISDIGIT(ch) || \
    Py_UNICODE_ISNUMERIC(ch))

#define Py_UNICODE_COPY(target, source, length) \
    Py_MEMCPY((target), (source), (length)*sizeof(Py_UNICODE))

#define Py_UNICODE_FILL(target, value, length) \
    do {Py_ssize_t i_; Py_UNICODE *t_ = (target); Py_UNICODE v_ = (value);\
    for (i_ = 0; i_ < (length); i_++) t_[i_] = v_;\
    } while (0)

/* macros to work with surrogates */
#define Py_UNICODE_IS_SURROGATE(ch) (0xD800 <= (ch) && (ch) <= 0xDFFF)
#define Py_UNICODE_IS_HIGH_SURROGATE(ch) (0xD800 <= (ch) && (ch) <= 0xDBFF)
#define Py_UNICODE_IS_LOW_SURROGATE(ch) (0xDC00 <= (ch) && (ch) <= 0xDFFF)
/* Join two surrogate characters and return a single Py_UCS4 value. */
#define Py_UNICODE_JOIN_SURROGATES(high, low)  \
    (((((Py_UCS4)(high) & 0x03FF) << 10) |      \
      ((Py_UCS4)(low) & 0x03FF)) + 0x10000)
/* high surrogate = top 10 bits added to D800 */
#define Py_UNICODE_HIGH_SURROGATE(ch) (0xD800 - (0x10000 >> 10) + ((ch) >> 10))
/* low surrogate = bottom 10 bits added to DC00 */
#define Py_UNICODE_LOW_SURROGATE(ch) (0xDC00 + ((ch) & 0x3FF))

/* Check if substring matches at given offset.  The offset must be
   valid, and the substring must not be empty. */

#define Py_UNICODE_MATCH(string, offset, substring) \
    ((*((string)->wstr + (offset)) == *((substring)->wstr)) && \
     ((*((string)->wstr + (offset) + (substring)->wstr_length-1) == *((substring)->wstr + (substring)->wstr_length-1))) && \
     !memcmp((string)->wstr + (offset), (substring)->wstr, (substring)->wstr_length*sizeof(Py_UNICODE)))

#endif /* Py_LIMITED_API */

#ifdef __cplusplus
extern "C" {
#endif

/* --- Unicode Type ------------------------------------------------------- */

#ifndef Py_LIMITED_API

/* ASCII-only strings created through PyUnicode_New use the PyASCIIObject
   structure. state.ascii and state.compact are set, and the data
   immediately follow the structure. utf8_length and wstr_length can be found
   in the length field; the utf8 pointer is equal to the data pointer. */
typedef struct {
    /* There are 4 forms of Unicode strings:

       - compact ascii:

         * structure = PyASCIIObject
         * test: PyUnicode_IS_COMPACT_ASCII(op)
         * kind = PyUnicode_1BYTE_KIND
         * compact = 1
         * ascii = 1
         * ready = 1
         * (length is the length of the utf8 and wstr strings)
         * (data starts just after the structure)
         * (since ASCII is decoded from UTF-8, the utf8 string are the data)

       - compact:

         * structure = PyCompactUnicodeObject
         * test: PyUnicode_IS_COMPACT(op) && !PyUnicode_IS_ASCII(op)
         * kind = PyUnicode_1BYTE_KIND, PyUnicode_2BYTE_KIND or
           PyUnicode_4BYTE_KIND
         * compact = 1
         * ready = 1
         * ascii = 0
         * utf8 is not shared with data
         * utf8_length = 0 if utf8 is NULL
         * wstr is shared with data and wstr_length=length
           if kind=PyUnicode_2BYTE_KIND and sizeof(wchar_t)=2
           or if kind=PyUnicode_4BYTE_KIND and sizeof(wchar_t)=4
         * wstr_length = 0 if wstr is NULL
         * (data starts just after the structure)

       - legacy string, not ready:

         * structure = PyUnicodeObject
         * test: kind == PyUnicode_WCHAR_KIND
         * length = 0 (use wstr_length)
         * hash = -1
         * kind = PyUnicode_WCHAR_KIND
         * compact = 0
         * ascii = 0
         * ready = 0
         * interned = SSTATE_NOT_INTERNED
         * wstr is not NULL
         * data.any is NULL
         * utf8 is NULL
         * utf8_length = 0

       - legacy string, ready:

         * structure = PyUnicodeObject structure
         * test: !PyUnicode_IS_COMPACT(op) && kind != PyUnicode_WCHAR_KIND
         * kind = PyUnicode_1BYTE_KIND, PyUnicode_2BYTE_KIND or
           PyUnicode_4BYTE_KIND
         * compact = 0
         * ready = 1
         * data.any is not NULL
         * utf8 is shared and utf8_length = length with data.any if ascii = 1
         * utf8_length = 0 if utf8 is NULL
         * wstr is shared with data.any and wstr_length = length
           if kind=PyUnicode_2BYTE_KIND and sizeof(wchar_t)=2
           or if kind=PyUnicode_4BYTE_KIND and sizeof(wchar_4)=4
         * wstr_length = 0 if wstr is NULL

       Compact strings use only one memory block (structure + characters),
       whereas legacy strings use one block for the structure and one block
       for characters.

       Legacy strings are created by PyUnicode_FromUnicode() and
       PyUnicode_FromStringAndSize(NULL, size) functions. They become ready
       when PyUnicode_READY() is called.

       See also _PyUnicode_CheckConsistency().
    */
    PyObject_HEAD
    Py_ssize_t length;          /* Number of code points in the string */
    Py_hash_t hash;             /* Hash value; -1 if not set */
    struct {
        /*
           SSTATE_NOT_INTERNED (0)
           SSTATE_INTERNED_MORTAL (1)
           SSTATE_INTERNED_IMMORTAL (2)

           If interned != SSTATE_NOT_INTERNED, the two references from the
           dictionary to this object are *not* counted in ob_refcnt.
         */
        unsigned int interned:2;
        /* Character size:

           - PyUnicode_WCHAR_KIND (0):

             * character type = wchar_t (16 or 32 bits, depending on the
               platform)

           - PyUnicode_1BYTE_KIND (1):

             * character type = Py_UCS1 (8 bits, unsigned)
             * all characters are in the range U+0000-U+00FF (latin1)
             * if ascii is set, all characters are in the range U+0000-U+007F
               (ASCII), otherwise at least one character is in the range
               U+0080-U+00FF

           - PyUnicode_2BYTE_KIND (2):

             * character type = Py_UCS2 (16 bits, unsigned)
             * all characters are in the range U+0000-U+FFFF (BMP)
             * at least one character is in the range U+0100-U+FFFF

           - PyUnicode_4BYTE_KIND (4):

             * character type = Py_UCS4 (32 bits, unsigned)
             * all characters are in the range U+0000-U+10FFFF
             * at least one character is in the range U+10000-U+10FFFF
         */
        unsigned int kind:3;
        /* Compact is with respect to the allocation scheme. Compact unicode
           objects only require one memory block while non-compact objects use
           one block for the PyUnicodeObject struct and another for its data
           buffer. */
        unsigned int compact:1;
        /* The string only contains characters in the range U+0000-U+007F (ASCII)
           and the kind is PyUnicode_1BYTE_KIND. If ascii is set and compact is
           set, use the PyASCIIObject structure. */
        unsigned int ascii:1;
        /* The ready flag indicates whether the object layout is initialized
           completely. This means that this is either a compact object, or
           the data pointer is filled out. The bit is redundant, and helps
           to minimize the test in PyUnicode_IS_READY(). */
        unsigned int ready:1;
        /* Padding to ensure that PyUnicode_DATA() is always aligned to
           4 bytes (see issue #19537 on m68k). */
        unsigned int :24;
    } state;
    wchar_t *wstr;              /* wchar_t representation (null-terminated) */
} PyASCIIObject;

/* Non-ASCII strings allocated through PyUnicode_New use the
   PyCompactUnicodeObject structure. state.compact is set, and the data
   immediately follow the structure. */
typedef struct {
    PyASCIIObject _base;
    Py_ssize_t utf8_length;     /* Number of bytes in utf8, excluding the
                                 * terminating \0. */
    char *utf8;                 /* UTF-8 representation (null-terminated) */
    Py_ssize_t wstr_length;     /* Number of code points in wstr, possible
                                 * surrogates count as two code points. */
} PyCompactUnicodeObject;

/* Strings allocated through PyUnicode_FromUnicode(NULL, len) use the
   PyUnicodeObject structure. The actual string data is initially in the wstr
   block, and copied into the data block using _PyUnicode_Ready. */
typedef struct {
    PyCompactUnicodeObject _base;
    union {
        void *any;
        Py_UCS1 *latin1;
        Py_UCS2 *ucs2;
        Py_UCS4 *ucs4;
    } data;                     /* Canonical, smallest-form Unicode buffer */
} PyUnicodeObject;
#endif

PyAPI_DATA(PyTypeObject) PyUnicode_Type;
PyAPI_DATA(PyTypeObject) PyUnicodeIter_Type;

#define PyUnicode_Check(op) \
                 PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_UNICODE_SUBCLASS)
#define PyUnicode_CheckExact(op) (Py_TYPE(op) == &PyUnicode_Type)

/* Fast access macros */
#ifndef Py_LIMITED_API

#define PyUnicode_WSTR_LENGTH(op) \
    (PyUnicode_IS_COMPACT_ASCII(op) ?                  \
     ((PyASCIIObject*)op)->length :                    \
     ((PyCompactUnicodeObject*)op)->wstr_length)

/* Returns the deprecated Py_UNICODE representation's size in code units
   (this includes surrogate pairs as 2 units).
   If the Py_UNICODE representation is not available, it will be computed
   on request.  Use PyUnicode_GET_LENGTH() for the length in code points. */

#define PyUnicode_GET_SIZE(op)                       \
    (assert(PyUnicode_Check(op)),                    \
     (((PyASCIIObject *)(op))->wstr) ?               \
      PyUnicode_WSTR_LENGTH(op) :                    \
      ((void)PyUnicode_AsUnicode((PyObject *)(op)),  \
       assert(((PyASCIIObject *)(op))->wstr),        \
       PyUnicode_WSTR_LENGTH(op)))

#define PyUnicode_GET_DATA_SIZE(op) \
    (PyUnicode_GET_SIZE(op) * Py_UNICODE_SIZE)

/* Alias for PyUnicode_AsUnicode().  This will create a wchar_t/Py_UNICODE
   representation on demand.  Using this macro is very inefficient now,
   try to port your code to use the new PyUnicode_*BYTE_DATA() macros or
   use PyUnicode_WRITE() and PyUnicode_READ(). */

#define PyUnicode_AS_UNICODE(op) \
    (assert(PyUnicode_Check(op)), \
     (((PyASCIIObject *)(op))->wstr) ? (((PyASCIIObject *)(op))->wstr) : \
      PyUnicode_AsUnicode((PyObject *)(op)))

#define PyUnicode_AS_DATA(op) \
    ((const char *)(PyUnicode_AS_UNICODE(op)))


/* --- Flexible String Representation Helper Macros (PEP 393) -------------- */

/* Values for PyASCIIObject.state: */

/* Interning state. */
#define SSTATE_NOT_INTERNED 0
#define SSTATE_INTERNED_MORTAL 1
#define SSTATE_INTERNED_IMMORTAL 2

/* Return true if the string contains only ASCII characters, or 0 if not. The
   string may be compact (PyUnicode_IS_COMPACT_ASCII) or not, but must be
   ready. */
#define PyUnicode_IS_ASCII(op)                   \
    (assert(PyUnicode_Check(op)),                \
     assert(PyUnicode_IS_READY(op)),             \
     ((PyASCIIObject*)op)->state.ascii)

/* Return true if the string is compact or 0 if not.
   No type checks or Ready calls are performed. */
#define PyUnicode_IS_COMPACT(op) \
    (((PyASCIIObject*)(op))->state.compact)

/* Return true if the string is a compact ASCII string (use PyASCIIObject
   structure), or 0 if not.  No type checks or Ready calls are performed. */
#define PyUnicode_IS_COMPACT_ASCII(op)                 \
    (((PyASCIIObject*)op)->state.ascii && PyUnicode_IS_COMPACT(op))

enum PyUnicode_Kind {
/* String contains only wstr byte characters.  This is only possible
   when the string was created with a legacy API and _PyUnicode_Ready()
   has not been called yet.  */
    PyUnicode_WCHAR_KIND = 0,
/* Return values of the PyUnicode_KIND() macro: */
    PyUnicode_1BYTE_KIND = 1,
    PyUnicode_2BYTE_KIND = 2,
    PyUnicode_4BYTE_KIND = 4
};

/* Return pointers to the canonical representation cast to unsigned char,
   Py_UCS2, or Py_UCS4 for direct character access.
   No checks are performed, use PyUnicode_KIND() before to ensure
   these will work correctly. */

#define PyUnicode_1BYTE_DATA(op) ((Py_UCS1*)PyUnicode_DATA(op))
#define PyUnicode_2BYTE_DATA(op) ((Py_UCS2*)PyUnicode_DATA(op))
#define PyUnicode_4BYTE_DATA(op) ((Py_UCS4*)PyUnicode_DATA(op))

/* Return one of the PyUnicode_*_KIND values defined above. */
#define PyUnicode_KIND(op) \
    (assert(PyUnicode_Check(op)), \
     assert(PyUnicode_IS_READY(op)),            \
     ((PyASCIIObject *)(op))->state.kind)

/* Return a void pointer to the raw unicode buffer. */
#define _PyUnicode_COMPACT_DATA(op)                     \
    (PyUnicode_IS_ASCII(op) ?                   \
     ((void*)((PyASCIIObject*)(op) + 1)) :              \
     ((void*)((PyCompactUnicodeObject*)(op) + 1)))

#define _PyUnicode_NONCOMPACT_DATA(op)                  \
    (assert(((PyUnicodeObject*)(op))->data.any),        \
     ((((PyUnicodeObject *)(op))->data.any)))

#define PyUnicode_DATA(op) \
    (assert(PyUnicode_Check(op)), \
     PyUnicode_IS_COMPACT(op) ? _PyUnicode_COMPACT_DATA(op) :   \
     _PyUnicode_NONCOMPACT_DATA(op))

/* In the access macros below, "kind" may be evaluated more than once.
   All other macro parameters are evaluated exactly once, so it is safe
   to put side effects into them (such as increasing the index). */

/* Write into the canonical representation, this macro does not do any sanity
   checks and is intended for usage in loops.  The caller should cache the
   kind and data pointers obtained from other macro calls.
   index is the index in the string (starts at 0) and value is the new
   code point value which should be written to that location. */
#define PyUnicode_WRITE(kind, data, index, value) \
    do { \
        switch ((kind)) { \
        case PyUnicode_1BYTE_KIND: { \
            ((Py_UCS1 *)(data))[(index)] = (Py_UCS1)(value); \
            break; \
        } \
        case PyUnicode_2BYTE_KIND: { \
            ((Py_UCS2 *)(data))[(index)] = (Py_UCS2)(value); \
            break; \
        } \
        default: { \
            assert((kind) == PyUnicode_4BYTE_KIND); \
            ((Py_UCS4 *)(data))[(index)] = (Py_UCS4)(value); \
        } \
        } \
    } while (0)

/* Read a code point from the string's canonical representation.  No checks
   or ready calls are performed. */
#define PyUnicode_READ(kind, data, index) \
    ((Py_UCS4) \
    ((kind) == PyUnicode_1BYTE_KIND ? \
        ((const Py_UCS1 *)(data))[(index)] : \
        ((kind) == PyUnicode_2BYTE_KIND ? \
            ((const Py_UCS2 *)(data))[(index)] : \
            ((const Py_UCS4 *)(data))[(index)] \
        ) \
    ))

/* PyUnicode_READ_CHAR() is less efficient than PyUnicode_READ() because it
   calls PyUnicode_KIND() and might call it twice.  For single reads, use
   PyUnicode_READ_CHAR, for multiple consecutive reads callers should
   cache kind and use PyUnicode_READ instead. */
#define PyUnicode_READ_CHAR(unicode, index) \
    (assert(PyUnicode_Check(unicode)),          \
     assert(PyUnicode_IS_READY(unicode)),       \
     (Py_UCS4)                                  \
        (PyUnicode_KIND((unicode)) == PyUnicode_1BYTE_KIND ? \
            ((const Py_UCS1 *)(PyUnicode_DATA((unicode))))[(index)] : \
            (PyUnicode_KIND((unicode)) == PyUnicode_2BYTE_KIND ? \
                ((const Py_UCS2 *)(PyUnicode_DATA((unicode))))[(index)] : \
                ((const Py_UCS4 *)(PyUnicode_DATA((unicode))))[(index)] \
            ) \
        ))

/* Returns the length of the unicode string. The caller has to make sure that
   the string has it's canonical representation set before calling
   this macro.  Call PyUnicode_(FAST_)Ready to ensure that. */
#define PyUnicode_GET_LENGTH(op)                \
    (assert(PyUnicode_Check(op)),               \
     assert(PyUnicode_IS_READY(op)),            \
     ((PyASCIIObject *)(op))->length)


/* Fast check to determine whether an object is ready. Equivalent to
   PyUnicode_IS_COMPACT(op) || ((PyUnicodeObject*)(op))->data.any) */

#define PyUnicode_IS_READY(op) (((PyASCIIObject*)op)->state.ready)

/* PyUnicode_READY() does less work than _PyUnicode_Ready() in the best
   case.  If the canonical representation is not yet set, it will still call
   _PyUnicode_Ready().
   Returns 0 on success and -1 on errors. */
#define PyUnicode_READY(op)                        \
    (assert(PyUnicode_Check(op)),                       \
     (PyUnicode_IS_READY(op) ?                          \
      0 : _PyUnicode_Ready((PyObject *)(op))))

/* Return a maximum character value which is suitable for creating another
   string based on op.  This is always an approximation but more efficient
   than iterating over the string. */
#define PyUnicode_MAX_CHAR_VALUE(op) \
    (assert(PyUnicode_IS_READY(op)),                                    \
     (PyUnicode_IS_ASCII(op) ?                                          \
      (0x7f) :                                                          \
      (PyUnicode_KIND(op) == PyUnicode_1BYTE_KIND ?                     \
       (0xffU) :                                                        \
       (PyUnicode_KIND(op) == PyUnicode_2BYTE_KIND ?                    \
        (0xffffU) :                                                     \
        (0x10ffffU)))))

#endif

/* --- Constants ---------------------------------------------------------- */

/* This Unicode character will be used as replacement character during
   decoding if the errors argument is set to "replace". Note: the
   Unicode character U+FFFD is the official REPLACEMENT CHARACTER in
   Unicode 3.0. */

#define Py_UNICODE_REPLACEMENT_CHARACTER ((Py_UCS4) 0xFFFD)

/* === Public API ========================================================= */

/* --- Plain Py_UNICODE --------------------------------------------------- */

/* With PEP 393, this is the recommended way to allocate a new unicode object.
   This function will allocate the object and its buffer in a single memory
   block.  Objects created using this function are not resizable. */
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_New(
    Py_ssize_t size,            /* Number of code points in the new string */
    Py_UCS4 maxchar             /* maximum code point value in the string */
    );
#endif

/* Initializes the canonical string representation from the deprecated
   wstr/Py_UNICODE representation. This function is used to convert Unicode
   objects which were created using the old API to the new flexible format
   introduced with PEP 393.

   Don't call this function directly, use the public PyUnicode_READY() macro
   instead. */
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyUnicode_Ready(
    PyObject *unicode           /* Unicode object */
    );
#endif

/* Get a copy of a Unicode string. */
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) _PyUnicode_Copy(
    PyObject *unicode
    );
#endif

/* Copy character from one unicode object into another, this function performs
   character conversion when necessary and falls back to memcpy() if possible.

   Fail if to is too small (smaller than *how_many* or smaller than
   len(from)-from_start), or if kind(from[from_start:from_start+how_many]) >
   kind(to), or if *to* has more than 1 reference.

   Return the number of written character, or return -1 and raise an exception
   on error.

   Pseudo-code:

       how_many = min(how_many, len(from) - from_start)
       to[to_start:to_start+how_many] = from[from_start:from_start+how_many]
       return how_many

   Note: The function doesn't write a terminating null character.
   */
#ifndef Py_LIMITED_API
PyAPI_FUNC(Py_ssize_t) PyUnicode_CopyCharacters(
    PyObject *to,
    Py_ssize_t to_start,
    PyObject *from,
    Py_ssize_t from_start,
    Py_ssize_t how_many
    );

/* Unsafe version of PyUnicode_CopyCharacters(): don't check arguments and so
   may crash if parameters are invalid (e.g. if the output string
   is too short). */
PyAPI_FUNC(void) _PyUnicode_FastCopyCharacters(
    PyObject *to,
    Py_ssize_t to_start,
    PyObject *from,
    Py_ssize_t from_start,
    Py_ssize_t how_many
    );
#endif

#ifndef Py_LIMITED_API
/* Fill a string with a character: write fill_char into
   unicode[start:start+length].

   Fail if fill_char is bigger than the string maximum character, or if the
   string has more than 1 reference.

   Return the number of written character, or return -1 and raise an exception
   on error. */
PyAPI_FUNC(Py_ssize_t) PyUnicode_Fill(
    PyObject *unicode,
    Py_ssize_t start,
    Py_ssize_t length,
    Py_UCS4 fill_char
    );

/* Unsafe version of PyUnicode_Fill(): don't check arguments and so may crash
   if parameters are invalid (e.g. if length is longer than the string). */
PyAPI_FUNC(void) _PyUnicode_FastFill(
    PyObject *unicode,
    Py_ssize_t start,
    Py_ssize_t length,
    Py_UCS4 fill_char
    );
#endif

/* Create a Unicode Object from the Py_UNICODE buffer u of the given
   size.

   u may be NULL which causes the contents to be undefined. It is the
   user's responsibility to fill in the needed data afterwards. Note
   that modifying the Unicode object contents after construction is
   only allowed if u was set to NULL.

   The buffer is copied into the new object. */

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_FromUnicode(
    const Py_UNICODE *u,        /* Unicode buffer */
    Py_ssize_t size             /* size of buffer */
    );
#endif

/* Similar to PyUnicode_FromUnicode(), but u points to UTF-8 encoded bytes */
PyAPI_FUNC(PyObject*) PyUnicode_FromStringAndSize(
    const char *u,             /* UTF-8 encoded string */
    Py_ssize_t size            /* size of buffer */
    );

/* Similar to PyUnicode_FromUnicode(), but u points to null-terminated
   UTF-8 encoded bytes.  The size is determined with strlen(). */
PyAPI_FUNC(PyObject*) PyUnicode_FromString(
    const char *u              /* UTF-8 encoded string */
    );

#ifndef Py_LIMITED_API
/* Create a new string from a buffer of Py_UCS1, Py_UCS2 or Py_UCS4 characters.
   Scan the string to find the maximum character. */
PyAPI_FUNC(PyObject*) PyUnicode_FromKindAndData(
    int kind,
    const void *buffer,
    Py_ssize_t size);

/* Create a new string from a buffer of ASCII characters.
   WARNING: Don't check if the string contains any non-ASCII character. */
PyAPI_FUNC(PyObject*) _PyUnicode_FromASCII(
    const char *buffer,
    Py_ssize_t size);
#endif

PyAPI_FUNC(PyObject*) PyUnicode_Substring(
    PyObject *str,
    Py_ssize_t start,
    Py_ssize_t end);

#ifndef Py_LIMITED_API
/* Compute the maximum character of the substring unicode[start:end].
   Return 127 for an empty string. */
PyAPI_FUNC(Py_UCS4) _PyUnicode_FindMaxChar (
    PyObject *unicode,
    Py_ssize_t start,
    Py_ssize_t end);
#endif

/* Copy the string into a UCS4 buffer including the null character if copy_null
   is set. Return NULL and raise an exception on error. Raise a ValueError if
   the buffer is smaller than the string. Return buffer on success.

   buflen is the length of the buffer in (Py_UCS4) characters. */
PyAPI_FUNC(Py_UCS4*) PyUnicode_AsUCS4(
    PyObject *unicode,
    Py_UCS4* buffer,
    Py_ssize_t buflen,
    int copy_null);

/* Copy the string into a UCS4 buffer. A new buffer is allocated using
 * PyMem_Malloc; if this fails, NULL is returned with a memory error
   exception set. */
PyAPI_FUNC(Py_UCS4*) PyUnicode_AsUCS4Copy(PyObject *unicode);

/* Return a read-only pointer to the Unicode object's internal
   Py_UNICODE buffer.
   If the wchar_t/Py_UNICODE representation is not yet available, this
   function will calculate it. */

#ifndef Py_LIMITED_API
PyAPI_FUNC(Py_UNICODE *) PyUnicode_AsUnicode(
    PyObject *unicode           /* Unicode object */
    );
#endif

/* Return a read-only pointer to the Unicode object's internal
   Py_UNICODE buffer and save the length at size.
   If the wchar_t/Py_UNICODE representation is not yet available, this
   function will calculate it. */

#ifndef Py_LIMITED_API
PyAPI_FUNC(Py_UNICODE *) PyUnicode_AsUnicodeAndSize(
    PyObject *unicode,          /* Unicode object */
    Py_ssize_t *size            /* location where to save the length */
    );
#endif

/* Get the length of the Unicode object. */

PyAPI_FUNC(Py_ssize_t) PyUnicode_GetLength(
    PyObject *unicode
);

/* Get the number of Py_UNICODE units in the
   string representation. */

PyAPI_FUNC(Py_ssize_t) PyUnicode_GetSize(
    PyObject *unicode           /* Unicode object */
    );

/* Read a character from the string. */

PyAPI_FUNC(Py_UCS4) PyUnicode_ReadChar(
    PyObject *unicode,
    Py_ssize_t index
    );

/* Write a character to the string. The string must have been created through
   PyUnicode_New, must not be shared, and must not have been hashed yet.

   Return 0 on success, -1 on error. */

PyAPI_FUNC(int) PyUnicode_WriteChar(
    PyObject *unicode,
    Py_ssize_t index,
    Py_UCS4 character
    );

#ifndef Py_LIMITED_API
/* Get the maximum ordinal for a Unicode character. */
PyAPI_FUNC(Py_UNICODE) PyUnicode_GetMax(void);
#endif

/* Resize a Unicode object. The length is the number of characters, except
   if the kind of the string is PyUnicode_WCHAR_KIND: in this case, the length
   is the number of Py_UNICODE characters.

   *unicode is modified to point to the new (resized) object and 0
   returned on success.

   Try to resize the string in place (which is usually faster than allocating
   a new string and copy characters), or create a new string.

   Error handling is implemented as follows: an exception is set, -1
   is returned and *unicode left untouched.

   WARNING: The function doesn't check string content, the result may not be a
            string in canonical representation. */

PyAPI_FUNC(int) PyUnicode_Resize(
    PyObject **unicode,         /* Pointer to the Unicode object */
    Py_ssize_t length           /* New length */
    );

/* Decode obj to a Unicode object.

   bytes, bytearray and other bytes-like objects are decoded according to the
   given encoding and error handler. The encoding and error handler can be
   NULL to have the interface use UTF-8 and "strict".

   All other objects (including Unicode objects) raise an exception.

   The API returns NULL in case of an error. The caller is responsible
   for decref'ing the returned objects.

*/

PyAPI_FUNC(PyObject*) PyUnicode_FromEncodedObject(
    PyObject *obj,              /* Object */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Copy an instance of a Unicode subtype to a new true Unicode object if
   necessary. If obj is already a true Unicode object (not a subtype), return
   the reference with *incremented* refcount.

   The API returns NULL in case of an error. The caller is responsible
   for decref'ing the returned objects.

*/

PyAPI_FUNC(PyObject*) PyUnicode_FromObject(
    PyObject *obj      /* Object */
    );

PyAPI_FUNC(PyObject *) PyUnicode_FromFormatV(
    const char *format,   /* ASCII-encoded string  */
    va_list vargs
    );
PyAPI_FUNC(PyObject *) PyUnicode_FromFormat(
    const char *format,   /* ASCII-encoded string  */
    ...
    );

#ifndef Py_LIMITED_API
typedef struct {
    PyObject *buffer;
    void *data;
    enum PyUnicode_Kind kind;
    Py_UCS4 maxchar;
    Py_ssize_t size;
    Py_ssize_t pos;

    /* minimum number of allocated characters (default: 0) */
    Py_ssize_t min_length;

    /* minimum character (default: 127, ASCII) */
    Py_UCS4 min_char;

    /* If non-zero, overallocate the buffer by 25% (default: 0). */
    unsigned char overallocate;

    /* If readonly is 1, buffer is a shared string (cannot be modified)
       and size is set to 0. */
    unsigned char readonly;
} _PyUnicodeWriter ;

/* Initialize a Unicode writer.
 *
 * By default, the minimum buffer size is 0 character and overallocation is
 * disabled. Set min_length, min_char and overallocate attributes to control
 * the allocation of the buffer. */
PyAPI_FUNC(void)
_PyUnicodeWriter_Init(_PyUnicodeWriter *writer);

/* Prepare the buffer to write 'length' characters
   with the specified maximum character.

   Return 0 on success, raise an exception and return -1 on error. */
#define _PyUnicodeWriter_Prepare(WRITER, LENGTH, MAXCHAR)             \
    (((MAXCHAR) <= (WRITER)->maxchar                                  \
      && (LENGTH) <= (WRITER)->size - (WRITER)->pos)                  \
     ? 0                                                              \
     : (((LENGTH) == 0)                                               \
        ? 0                                                           \
        : _PyUnicodeWriter_PrepareInternal((WRITER), (LENGTH), (MAXCHAR))))

/* Don't call this function directly, use the _PyUnicodeWriter_Prepare() macro
   instead. */
PyAPI_FUNC(int)
_PyUnicodeWriter_PrepareInternal(_PyUnicodeWriter *writer,
                                 Py_ssize_t length, Py_UCS4 maxchar);

/* Append a Unicode character.
   Return 0 on success, raise an exception and return -1 on error. */
PyAPI_FUNC(int)
_PyUnicodeWriter_WriteChar(_PyUnicodeWriter *writer,
    Py_UCS4 ch
    );

/* Append a Unicode string.
   Return 0 on success, raise an exception and return -1 on error. */
PyAPI_FUNC(int)
_PyUnicodeWriter_WriteStr(_PyUnicodeWriter *writer,
    PyObject *str               /* Unicode string */
    );

/* Append a substring of a Unicode string.
   Return 0 on success, raise an exception and return -1 on error. */
PyAPI_FUNC(int)
_PyUnicodeWriter_WriteSubstring(_PyUnicodeWriter *writer,
    PyObject *str,              /* Unicode string */
    Py_ssize_t start,
    Py_ssize_t end
    );

/* Append an ASCII-encoded byte string.
   Return 0 on success, raise an exception and return -1 on error. */
PyAPI_FUNC(int)
_PyUnicodeWriter_WriteASCIIString(_PyUnicodeWriter *writer,
    const char *str,           /* ASCII-encoded byte string */
    Py_ssize_t len             /* number of bytes, or -1 if unknown */
    );

/* Append a latin1-encoded byte string.
   Return 0 on success, raise an exception and return -1 on error. */
PyAPI_FUNC(int)
_PyUnicodeWriter_WriteLatin1String(_PyUnicodeWriter *writer,
    const char *str,           /* latin1-encoded byte string */
    Py_ssize_t len             /* length in bytes */
    );

/* Get the value of the writer as a Unicode string. Clear the
   buffer of the writer. Raise an exception and return NULL
   on error. */
PyAPI_FUNC(PyObject *)
_PyUnicodeWriter_Finish(_PyUnicodeWriter *writer);

/* Deallocate memory of a writer (clear its internal buffer). */
PyAPI_FUNC(void)
_PyUnicodeWriter_Dealloc(_PyUnicodeWriter *writer);
#endif

#ifndef Py_LIMITED_API
/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
PyAPI_FUNC(int) _PyUnicode_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);
#endif

PyAPI_FUNC(void) PyUnicode_InternInPlace(PyObject **);
PyAPI_FUNC(void) PyUnicode_InternImmortal(PyObject **);
PyAPI_FUNC(PyObject *) PyUnicode_InternFromString(
    const char *u              /* UTF-8 encoded string */
    );
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _Py_ReleaseInternedUnicodeStrings(void);
#endif

/* Use only if you know it's a string */
#define PyUnicode_CHECK_INTERNED(op) \
    (((PyASCIIObject *)(op))->state.interned)

/* --- wchar_t support for platforms which support it --------------------- */

#ifdef HAVE_WCHAR_H

/* Create a Unicode Object from the wchar_t buffer w of the given
   size.

   The buffer is copied into the new object. */

PyAPI_FUNC(PyObject*) PyUnicode_FromWideChar(
    const wchar_t *w,           /* wchar_t buffer */
    Py_ssize_t size             /* size of buffer */
    );

/* Copies the Unicode Object contents into the wchar_t buffer w.  At
   most size wchar_t characters are copied.

   Note that the resulting wchar_t string may or may not be
   0-terminated.  It is the responsibility of the caller to make sure
   that the wchar_t string is 0-terminated in case this is required by
   the application.

   Returns the number of wchar_t characters copied (excluding a
   possibly trailing 0-termination character) or -1 in case of an
   error. */

PyAPI_FUNC(Py_ssize_t) PyUnicode_AsWideChar(
    PyObject *unicode,          /* Unicode object */
    wchar_t *w,                 /* wchar_t buffer */
    Py_ssize_t size             /* size of buffer */
    );

/* Convert the Unicode object to a wide character string. The output string
   always ends with a nul character. If size is not NULL, write the number of
   wide characters (excluding the null character) into *size.

   Returns a buffer allocated by PyMem_Malloc() (use PyMem_Free() to free it)
   on success. On error, returns NULL, *size is undefined and raises a
   MemoryError. */

PyAPI_FUNC(wchar_t*) PyUnicode_AsWideCharString(
    PyObject *unicode,          /* Unicode object */
    Py_ssize_t *size            /* number of characters of the result */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(void*) _PyUnicode_AsKind(PyObject *s, unsigned int kind);
#endif

#endif

/* --- Unicode ordinals --------------------------------------------------- */

/* Create a Unicode Object from the given Unicode code point ordinal.

   The ordinal must be in range(0x110000). A ValueError is
   raised in case it is not.

*/

PyAPI_FUNC(PyObject*) PyUnicode_FromOrdinal(int ordinal);

/* --- Free-list management ----------------------------------------------- */

/* Clear the free list used by the Unicode implementation.

   This can be used to release memory used for objects on the free
   list back to the Python memory allocator.

*/

PyAPI_FUNC(int) PyUnicode_ClearFreeList(void);

/* === Builtin Codecs =====================================================

   Many of these APIs take two arguments encoding and errors. These
   parameters encoding and errors have the same semantics as the ones
   of the builtin str() API.

   Setting encoding to NULL causes the default encoding (UTF-8) to be used.

   Error handling is set by errors which may also be set to NULL
   meaning to use the default handling defined for the codec. Default
   error handling for all builtin codecs is "strict" (ValueErrors are
   raised).

   The codecs all use a similar interface. Only deviation from the
   generic ones are documented.

*/

/* --- Manage the default encoding ---------------------------------------- */

/* Returns a pointer to the default encoding (UTF-8) of the
   Unicode object unicode and the size of the encoded representation
   in bytes stored in *size.

   In case of an error, no *size is set.

   This function caches the UTF-8 encoded string in the unicodeobject
   and subsequent calls will return the same string.  The memory is released
   when the unicodeobject is deallocated.

   _PyUnicode_AsStringAndSize is a #define for PyUnicode_AsUTF8AndSize to
   support the previous internal function with the same behaviour.

   *** This API is for interpreter INTERNAL USE ONLY and will likely
   *** be removed or changed in the future.

   *** If you need to access the Unicode object as UTF-8 bytes string,
   *** please use PyUnicode_AsUTF8String() instead.
*/

#ifndef Py_LIMITED_API
PyAPI_FUNC(char *) PyUnicode_AsUTF8AndSize(
    PyObject *unicode,
    Py_ssize_t *size);
#define _PyUnicode_AsStringAndSize PyUnicode_AsUTF8AndSize
#endif

/* Returns a pointer to the default encoding (UTF-8) of the
   Unicode object unicode.

   Like PyUnicode_AsUTF8AndSize(), this also caches the UTF-8 representation
   in the unicodeobject.

   _PyUnicode_AsString is a #define for PyUnicode_AsUTF8 to
   support the previous internal function with the same behaviour.

   Use of this API is DEPRECATED since no size information can be
   extracted from the returned data.

   *** This API is for interpreter INTERNAL USE ONLY and will likely
   *** be removed or changed for Python 3.1.

   *** If you need to access the Unicode object as UTF-8 bytes string,
   *** please use PyUnicode_AsUTF8String() instead.

*/

#ifndef Py_LIMITED_API
PyAPI_FUNC(char *) PyUnicode_AsUTF8(PyObject *unicode);
#define _PyUnicode_AsString PyUnicode_AsUTF8
#endif

/* Returns "utf-8".  */

PyAPI_FUNC(const char*) PyUnicode_GetDefaultEncoding(void);

/* --- Generic Codecs ----------------------------------------------------- */

/* Create a Unicode object by decoding the encoded string s of the
   given size. */

PyAPI_FUNC(PyObject*) PyUnicode_Decode(
    const char *s,              /* encoded string */
    Py_ssize_t size,            /* size of buffer */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Decode a Unicode object unicode and return the result as Python
   object. */

PyAPI_FUNC(PyObject*) PyUnicode_AsDecodedObject(
    PyObject *unicode,          /* Unicode object */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Decode a Unicode object unicode and return the result as Unicode
   object. */

PyAPI_FUNC(PyObject*) PyUnicode_AsDecodedUnicode(
    PyObject *unicode,          /* Unicode object */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Encodes a Py_UNICODE buffer of the given size and returns a
   Python string object. */

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_Encode(
    const Py_UNICODE *s,        /* Unicode char buffer */
    Py_ssize_t size,            /* number of Py_UNICODE chars to encode */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );
#endif

/* Encodes a Unicode object and returns the result as Python
   object. */

PyAPI_FUNC(PyObject*) PyUnicode_AsEncodedObject(
    PyObject *unicode,          /* Unicode object */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Encodes a Unicode object and returns the result as Python string
   object. */

PyAPI_FUNC(PyObject*) PyUnicode_AsEncodedString(
    PyObject *unicode,          /* Unicode object */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Encodes a Unicode object and returns the result as Unicode
   object. */

PyAPI_FUNC(PyObject*) PyUnicode_AsEncodedUnicode(
    PyObject *unicode,          /* Unicode object */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Build an encoding map. */

PyAPI_FUNC(PyObject*) PyUnicode_BuildEncodingMap(
    PyObject* string            /* 256 character map */
   );

/* --- UTF-7 Codecs ------------------------------------------------------- */

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF7(
    const char *string,         /* UTF-7 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF7Stateful(
    const char *string,         /* UTF-7 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    Py_ssize_t *consumed        /* bytes consumed */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_EncodeUTF7(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* number of Py_UNICODE chars to encode */
    int base64SetO,             /* Encode RFC2152 Set O characters in base64 */
    int base64WhiteSpace,       /* Encode whitespace (sp, ht, nl, cr) in base64 */
    const char *errors          /* error handling */
    );
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeUTF7(
    PyObject *unicode,          /* Unicode object */
    int base64SetO,             /* Encode RFC2152 Set O characters in base64 */
    int base64WhiteSpace,       /* Encode whitespace (sp, ht, nl, cr) in base64 */
    const char *errors          /* error handling */
    );
#endif

/* --- UTF-8 Codecs ------------------------------------------------------- */

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF8(
    const char *string,         /* UTF-8 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF8Stateful(
    const char *string,         /* UTF-8 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    Py_ssize_t *consumed        /* bytes consumed */
    );

PyAPI_FUNC(PyObject*) PyUnicode_AsUTF8String(
    PyObject *unicode           /* Unicode object */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) _PyUnicode_AsUTF8String(
    PyObject *unicode,
    const char *errors);

PyAPI_FUNC(PyObject*) PyUnicode_EncodeUTF8(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* number of Py_UNICODE chars to encode */
    const char *errors          /* error handling */
    );
#endif

/* --- UTF-32 Codecs ------------------------------------------------------ */

/* Decodes length bytes from a UTF-32 encoded buffer string and returns
   the corresponding Unicode object.

   errors (if non-NULL) defines the error handling. It defaults
   to "strict".

   If byteorder is non-NULL, the decoder starts decoding using the
   given byte order:

    *byteorder == -1: little endian
    *byteorder == 0:  native order
    *byteorder == 1:  big endian

   In native mode, the first four bytes of the stream are checked for a
   BOM mark. If found, the BOM mark is analysed, the byte order
   adjusted and the BOM skipped.  In the other modes, no BOM mark
   interpretation is done. After completion, *byteorder is set to the
   current byte order at the end of input data.

   If byteorder is NULL, the codec starts in native order mode.

*/

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF32(
    const char *string,         /* UTF-32 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    int *byteorder              /* pointer to byteorder to use
                                   0=native;-1=LE,1=BE; updated on
                                   exit */
    );

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF32Stateful(
    const char *string,         /* UTF-32 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    int *byteorder,             /* pointer to byteorder to use
                                   0=native;-1=LE,1=BE; updated on
                                   exit */
    Py_ssize_t *consumed        /* bytes consumed */
    );

/* Returns a Python string using the UTF-32 encoding in native byte
   order. The string always starts with a BOM mark.  */

PyAPI_FUNC(PyObject*) PyUnicode_AsUTF32String(
    PyObject *unicode           /* Unicode object */
    );

/* Returns a Python string object holding the UTF-32 encoded value of
   the Unicode data.

   If byteorder is not 0, output is written according to the following
   byte order:

   byteorder == -1: little endian
   byteorder == 0:  native byte order (writes a BOM mark)
   byteorder == 1:  big endian

   If byteorder is 0, the output string will always start with the
   Unicode BOM mark (U+FEFF). In the other two modes, no BOM mark is
   prepended.

*/

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_EncodeUTF32(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* number of Py_UNICODE chars to encode */
    const char *errors,         /* error handling */
    int byteorder               /* byteorder to use 0=BOM+native;-1=LE,1=BE */
    );
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeUTF32(
    PyObject *object,           /* Unicode object */
    const char *errors,         /* error handling */
    int byteorder               /* byteorder to use 0=BOM+native;-1=LE,1=BE */
    );
#endif

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

   In native mode, the first two bytes of the stream are checked for a
   BOM mark. If found, the BOM mark is analysed, the byte order
   adjusted and the BOM skipped.  In the other modes, no BOM mark
   interpretation is done. After completion, *byteorder is set to the
   current byte order at the end of input data.

   If byteorder is NULL, the codec starts in native order mode.

*/

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF16(
    const char *string,         /* UTF-16 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    int *byteorder              /* pointer to byteorder to use
                                   0=native;-1=LE,1=BE; updated on
                                   exit */
    );

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF16Stateful(
    const char *string,         /* UTF-16 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    int *byteorder,             /* pointer to byteorder to use
                                   0=native;-1=LE,1=BE; updated on
                                   exit */
    Py_ssize_t *consumed        /* bytes consumed */
    );

/* Returns a Python string using the UTF-16 encoding in native byte
   order. The string always starts with a BOM mark.  */

PyAPI_FUNC(PyObject*) PyUnicode_AsUTF16String(
    PyObject *unicode           /* Unicode object */
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

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_EncodeUTF16(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* number of Py_UNICODE chars to encode */
    const char *errors,         /* error handling */
    int byteorder               /* byteorder to use 0=BOM+native;-1=LE,1=BE */
    );
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeUTF16(
    PyObject* unicode,          /* Unicode object */
    const char *errors,         /* error handling */
    int byteorder               /* byteorder to use 0=BOM+native;-1=LE,1=BE */
    );
#endif

/* --- Unicode-Escape Codecs ---------------------------------------------- */

PyAPI_FUNC(PyObject*) PyUnicode_DecodeUnicodeEscape(
    const char *string,         /* Unicode-Escape encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_AsUnicodeEscapeString(
    PyObject *unicode           /* Unicode object */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_EncodeUnicodeEscape(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length           /* Number of Py_UNICODE chars to encode */
    );
#endif

/* --- Raw-Unicode-Escape Codecs ------------------------------------------ */

PyAPI_FUNC(PyObject*) PyUnicode_DecodeRawUnicodeEscape(
    const char *string,         /* Raw-Unicode-Escape encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_AsRawUnicodeEscapeString(
    PyObject *unicode           /* Unicode object */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_EncodeRawUnicodeEscape(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length           /* Number of Py_UNICODE chars to encode */
    );
#endif

/* --- Unicode Internal Codec ---------------------------------------------

    Only for internal use in _codecsmodule.c */

#ifndef Py_LIMITED_API
PyObject *_PyUnicode_DecodeUnicodeInternal(
    const char *string,
    Py_ssize_t length,
    const char *errors
    );
#endif

/* --- Latin-1 Codecs -----------------------------------------------------

   Note: Latin-1 corresponds to the first 256 Unicode ordinals.

*/

PyAPI_FUNC(PyObject*) PyUnicode_DecodeLatin1(
    const char *string,         /* Latin-1 encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_AsLatin1String(
    PyObject *unicode           /* Unicode object */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) _PyUnicode_AsLatin1String(
    PyObject* unicode,
    const char* errors);

PyAPI_FUNC(PyObject*) PyUnicode_EncodeLatin1(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* Number of Py_UNICODE chars to encode */
    const char *errors          /* error handling */
    );
#endif

/* --- ASCII Codecs -------------------------------------------------------

   Only 7-bit ASCII data is excepted. All other codes generate errors.

*/

PyAPI_FUNC(PyObject*) PyUnicode_DecodeASCII(
    const char *string,         /* ASCII encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_AsASCIIString(
    PyObject *unicode           /* Unicode object */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) _PyUnicode_AsASCIIString(
    PyObject* unicode,
    const char* errors);

PyAPI_FUNC(PyObject*) PyUnicode_EncodeASCII(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* Number of Py_UNICODE chars to encode */
    const char *errors          /* error handling */
    );
#endif

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

PyAPI_FUNC(PyObject*) PyUnicode_DecodeCharmap(
    const char *string,         /* Encoded string */
    Py_ssize_t length,          /* size of string */
    PyObject *mapping,          /* character mapping
                                   (char ordinal -> unicode ordinal) */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_AsCharmapString(
    PyObject *unicode,          /* Unicode object */
    PyObject *mapping           /* character mapping
                                   (unicode ordinal -> char ordinal) */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_EncodeCharmap(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* Number of Py_UNICODE chars to encode */
    PyObject *mapping,          /* character mapping
                                   (unicode ordinal -> char ordinal) */
    const char *errors          /* error handling */
    );
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeCharmap(
    PyObject *unicode,          /* Unicode object */
    PyObject *mapping,          /* character mapping
                                   (unicode ordinal -> char ordinal) */
    const char *errors          /* error handling */
    );
#endif

/* Translate a Py_UNICODE buffer of the given length by applying a
   character mapping table to it and return the resulting Unicode
   object.

   The mapping table must map Unicode ordinal integers to Unicode
   ordinal integers or None (causing deletion of the character).

   Mapping tables may be dictionaries or sequences. Unmapped character
   ordinals (ones which cause a LookupError) are left untouched and
   are copied as-is.

*/

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) PyUnicode_TranslateCharmap(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* Number of Py_UNICODE chars to encode */
    PyObject *table,            /* Translate table */
    const char *errors          /* error handling */
    );
#endif

#ifdef HAVE_MBCS

/* --- MBCS codecs for Windows -------------------------------------------- */

PyAPI_FUNC(PyObject*) PyUnicode_DecodeMBCS(
    const char *string,         /* MBCS encoded string */
    Py_ssize_t length,              /* size of string */
    const char *errors          /* error handling */
    );

PyAPI_FUNC(PyObject*) PyUnicode_DecodeMBCSStateful(
    const char *string,         /* MBCS encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    Py_ssize_t *consumed        /* bytes consumed */
    );

PyAPI_FUNC(PyObject*) PyUnicode_DecodeCodePageStateful(
    int code_page,              /* code page number */
    const char *string,         /* encoded string */
    Py_ssize_t length,          /* size of string */
    const char *errors,         /* error handling */
    Py_ssize_t *consumed        /* bytes consumed */
    );

PyAPI_FUNC(PyObject*) PyUnicode_AsMBCSString(
    PyObject *unicode           /* Unicode object */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_EncodeMBCS(
    const Py_UNICODE *data,     /* Unicode char buffer */
    Py_ssize_t length,          /* number of Py_UNICODE chars to encode */
    const char *errors          /* error handling */
    );
#endif

PyAPI_FUNC(PyObject*) PyUnicode_EncodeCodePage(
    int code_page,              /* code page number */
    PyObject *unicode,          /* Unicode object */
    const char *errors          /* error handling */
    );

#endif /* HAVE_MBCS */

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

#ifndef Py_LIMITED_API
PyAPI_FUNC(int) PyUnicode_EncodeDecimal(
    Py_UNICODE *s,              /* Unicode buffer */
    Py_ssize_t length,          /* Number of Py_UNICODE chars to encode */
    char *output,               /* Output buffer; must have size >= length */
    const char *errors          /* error handling */
    );
#endif

/* Transforms code points that have decimal digit property to the
   corresponding ASCII digit code points.

   Returns a new Unicode string on success, NULL on failure.
*/

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) PyUnicode_TransformDecimalToASCII(
    Py_UNICODE *s,              /* Unicode buffer */
    Py_ssize_t length           /* Number of Py_UNICODE chars to transform */
    );
#endif

/* Similar to PyUnicode_TransformDecimalToASCII(), but takes a PyObject
   as argument instead of a raw buffer and length.  This function additionally
   transforms spaces to ASCII because this is what the callers in longobject,
   floatobject, and complexobject did anyways. */

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) _PyUnicode_TransformDecimalAndSpaceToASCII(
    PyObject *unicode           /* Unicode object */
    );
#endif

/* --- Locale encoding --------------------------------------------------- */

/* Decode a string from the current locale encoding. The decoder is strict if
   *surrogateescape* is equal to zero, otherwise it uses the 'surrogateescape'
   error handler (PEP 383) to escape undecodable bytes. If a byte sequence can
   be decoded as a surrogate character and *surrogateescape* is not equal to
   zero, the byte sequence is escaped using the 'surrogateescape' error handler
   instead of being decoded. *str* must end with a null character but cannot
   contain embedded null characters. */

PyAPI_FUNC(PyObject*) PyUnicode_DecodeLocaleAndSize(
    const char *str,
    Py_ssize_t len,
    const char *errors);

/* Similar to PyUnicode_DecodeLocaleAndSize(), but compute the string
   length using strlen(). */

PyAPI_FUNC(PyObject*) PyUnicode_DecodeLocale(
    const char *str,
    const char *errors);

/* Encode a Unicode object to the current locale encoding. The encoder is
   strict is *surrogateescape* is equal to zero, otherwise the
   "surrogateescape" error handler is used. Return a bytes object. The string
   cannot contain embedded null characters. */

PyAPI_FUNC(PyObject*) PyUnicode_EncodeLocale(
    PyObject *unicode,
    const char *errors
    );

/* --- File system encoding ---------------------------------------------- */

/* ParseTuple converter: encode str objects to bytes using
   PyUnicode_EncodeFSDefault(); bytes objects are output as-is. */

PyAPI_FUNC(int) PyUnicode_FSConverter(PyObject*, void*);

/* ParseTuple converter: decode bytes objects to unicode using
   PyUnicode_DecodeFSDefaultAndSize(); str objects are output as-is. */

PyAPI_FUNC(int) PyUnicode_FSDecoder(PyObject*, void*);

/* Decode a null-terminated string using Py_FileSystemDefaultEncoding
   and the "surrogateescape" error handler.

   If Py_FileSystemDefaultEncoding is not set, fall back to the locale
   encoding.

   Use PyUnicode_DecodeFSDefaultAndSize() if the string length is known.
*/

PyAPI_FUNC(PyObject*) PyUnicode_DecodeFSDefault(
    const char *s               /* encoded string */
    );

/* Decode a string using Py_FileSystemDefaultEncoding
   and the "surrogateescape" error handler.

   If Py_FileSystemDefaultEncoding is not set, fall back to the locale
   encoding.
*/

PyAPI_FUNC(PyObject*) PyUnicode_DecodeFSDefaultAndSize(
    const char *s,               /* encoded string */
    Py_ssize_t size              /* size */
    );

/* Encode a Unicode object to Py_FileSystemDefaultEncoding with the
   "surrogateescape" error handler, and return bytes.

   If Py_FileSystemDefaultEncoding is not set, fall back to the locale
   encoding.
*/

PyAPI_FUNC(PyObject*) PyUnicode_EncodeFSDefault(
    PyObject *unicode
    );

/* --- Methods & Slots ----------------------------------------------------

   These are capable of handling Unicode objects and strings on input
   (we refer to them as strings in the descriptions) and return
   Unicode objects or integers as appropriate. */

/* Concat two strings giving a new Unicode string. */

PyAPI_FUNC(PyObject*) PyUnicode_Concat(
    PyObject *left,             /* Left string */
    PyObject *right             /* Right string */
    );

/* Concat two strings and put the result in *pleft
   (sets *pleft to NULL on error) */

PyAPI_FUNC(void) PyUnicode_Append(
    PyObject **pleft,           /* Pointer to left string */
    PyObject *right             /* Right string */
    );

/* Concat two strings, put the result in *pleft and drop the right object
   (sets *pleft to NULL on error) */

PyAPI_FUNC(void) PyUnicode_AppendAndDel(
    PyObject **pleft,           /* Pointer to left string */
    PyObject *right             /* Right string */
    );

/* Split a string giving a list of Unicode strings.

   If sep is NULL, splitting will be done at all whitespace
   substrings. Otherwise, splits occur at the given separator.

   At most maxsplit splits will be done. If negative, no limit is set.

   Separators are not included in the resulting list.

*/

PyAPI_FUNC(PyObject*) PyUnicode_Split(
    PyObject *s,                /* String to split */
    PyObject *sep,              /* String separator */
    Py_ssize_t maxsplit         /* Maxsplit count */
    );

/* Dito, but split at line breaks.

   CRLF is considered to be one line break. Line breaks are not
   included in the resulting list. */

PyAPI_FUNC(PyObject*) PyUnicode_Splitlines(
    PyObject *s,                /* String to split */
    int keepends                /* If true, line end markers are included */
    );

/* Partition a string using a given separator. */

PyAPI_FUNC(PyObject*) PyUnicode_Partition(
    PyObject *s,                /* String to partition */
    PyObject *sep               /* String separator */
    );

/* Partition a string using a given separator, searching from the end of the
   string. */

PyAPI_FUNC(PyObject*) PyUnicode_RPartition(
    PyObject *s,                /* String to partition */
    PyObject *sep               /* String separator */
    );

/* Split a string giving a list of Unicode strings.

   If sep is NULL, splitting will be done at all whitespace
   substrings. Otherwise, splits occur at the given separator.

   At most maxsplit splits will be done. But unlike PyUnicode_Split
   PyUnicode_RSplit splits from the end of the string. If negative,
   no limit is set.

   Separators are not included in the resulting list.

*/

PyAPI_FUNC(PyObject*) PyUnicode_RSplit(
    PyObject *s,                /* String to split */
    PyObject *sep,              /* String separator */
    Py_ssize_t maxsplit         /* Maxsplit count */
    );

/* Translate a string by applying a character mapping table to it and
   return the resulting Unicode object.

   The mapping table must map Unicode ordinal integers to Unicode
   ordinal integers or None (causing deletion of the character).

   Mapping tables may be dictionaries or sequences. Unmapped character
   ordinals (ones which cause a LookupError) are left untouched and
   are copied as-is.

*/

PyAPI_FUNC(PyObject *) PyUnicode_Translate(
    PyObject *str,              /* String */
    PyObject *table,            /* Translate table */
    const char *errors          /* error handling */
    );

/* Join a sequence of strings using the given separator and return
   the resulting Unicode string. */

PyAPI_FUNC(PyObject*) PyUnicode_Join(
    PyObject *separator,        /* Separator string */
    PyObject *seq               /* Sequence object */
    );

/* Return 1 if substr matches str[start:end] at the given tail end, 0
   otherwise. */

PyAPI_FUNC(Py_ssize_t) PyUnicode_Tailmatch(
    PyObject *str,              /* String */
    PyObject *substr,           /* Prefix or Suffix string */
    Py_ssize_t start,           /* Start index */
    Py_ssize_t end,             /* Stop index */
    int direction               /* Tail end: -1 prefix, +1 suffix */
    );

/* Return the first position of substr in str[start:end] using the
   given search direction or -1 if not found. -2 is returned in case
   an error occurred and an exception is set. */

PyAPI_FUNC(Py_ssize_t) PyUnicode_Find(
    PyObject *str,              /* String */
    PyObject *substr,           /* Substring to find */
    Py_ssize_t start,           /* Start index */
    Py_ssize_t end,             /* Stop index */
    int direction               /* Find direction: +1 forward, -1 backward */
    );

/* Like PyUnicode_Find, but search for single character only. */
PyAPI_FUNC(Py_ssize_t) PyUnicode_FindChar(
    PyObject *str,
    Py_UCS4 ch,
    Py_ssize_t start,
    Py_ssize_t end,
    int direction
    );

/* Count the number of occurrences of substr in str[start:end]. */

PyAPI_FUNC(Py_ssize_t) PyUnicode_Count(
    PyObject *str,              /* String */
    PyObject *substr,           /* Substring to count */
    Py_ssize_t start,           /* Start index */
    Py_ssize_t end              /* Stop index */
    );

/* Replace at most maxcount occurrences of substr in str with replstr
   and return the resulting Unicode object. */

PyAPI_FUNC(PyObject *) PyUnicode_Replace(
    PyObject *str,              /* String */
    PyObject *substr,           /* Substring to find */
    PyObject *replstr,          /* Substring to replace */
    Py_ssize_t maxcount         /* Max. number of replacements to apply;
                                   -1 = all */
    );

/* Compare two strings and return -1, 0, 1 for less than, equal,
   greater than resp.
   Raise an exception and return -1 on error. */

PyAPI_FUNC(int) PyUnicode_Compare(
    PyObject *left,             /* Left string */
    PyObject *right             /* Right string */
    );

#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _PyUnicode_CompareWithId(
    PyObject *left,             /* Left string */
    _Py_Identifier *right       /* Right identifier */
    );
#endif

PyAPI_FUNC(int) PyUnicode_CompareWithASCIIString(
    PyObject *left,
    const char *right           /* ASCII-encoded string */
    );

/* Rich compare two strings and return one of the following:

   - NULL in case an exception was raised
   - Py_True or Py_False for successfully comparisons
   - Py_NotImplemented in case the type combination is unknown

   Note that Py_EQ and Py_NE comparisons can cause a UnicodeWarning in
   case the conversion of the arguments to Unicode fails with a
   UnicodeDecodeError.

   Possible values for op:

     Py_GT, Py_GE, Py_EQ, Py_NE, Py_LT, Py_LE

*/

PyAPI_FUNC(PyObject *) PyUnicode_RichCompare(
    PyObject *left,             /* Left string */
    PyObject *right,            /* Right string */
    int op                      /* Operation: Py_EQ, Py_NE, Py_GT, etc. */
    );

/* Apply an argument tuple or dictionary to a format string and return
   the resulting Unicode string. */

PyAPI_FUNC(PyObject *) PyUnicode_Format(
    PyObject *format,           /* Format string */
    PyObject *args              /* Argument tuple or dictionary */
    );

/* Checks whether element is contained in container and return 1/0
   accordingly.

   element has to coerce to a one element Unicode string. -1 is
   returned in case of an error. */

PyAPI_FUNC(int) PyUnicode_Contains(
    PyObject *container,        /* Container string */
    PyObject *element           /* Element string */
    );

/* Checks whether argument is a valid identifier. */

PyAPI_FUNC(int) PyUnicode_IsIdentifier(PyObject *s);

#ifndef Py_LIMITED_API
/* Externally visible for str.strip(unicode) */
PyAPI_FUNC(PyObject *) _PyUnicode_XStrip(
    PyObject *self,
    int striptype,
    PyObject *sepobj
    );
#endif

/* Using explicit passed-in values, insert the thousands grouping
   into the string pointed to by buffer.  For the argument descriptions,
   see Objects/stringlib/localeutil.h */
#ifndef Py_LIMITED_API
PyAPI_FUNC(Py_ssize_t) _PyUnicode_InsertThousandsGrouping(
    PyObject *unicode,
    Py_ssize_t index,
    Py_ssize_t n_buffer,
    void *digits,
    Py_ssize_t n_digits,
    Py_ssize_t min_width,
    const char *grouping,
    PyObject *thousands_sep,
    Py_UCS4 *maxchar);
#endif
/* === Characters Type APIs =============================================== */

/* Helper array used by Py_UNICODE_ISSPACE(). */

#ifndef Py_LIMITED_API
PyAPI_DATA(const unsigned char) _Py_ascii_whitespace[];

/* These should not be used directly. Use the Py_UNICODE_IS* and
   Py_UNICODE_TO* macros instead.

   These APIs are implemented in Objects/unicodectype.c.

*/

PyAPI_FUNC(int) _PyUnicode_IsLowercase(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsUppercase(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsTitlecase(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsXidStart(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsXidContinue(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsWhitespace(
    const Py_UCS4 ch         /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsLinebreak(
    const Py_UCS4 ch         /* Unicode character */
    );

PyAPI_FUNC(Py_UCS4) _PyUnicode_ToLowercase(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(Py_UCS4) _PyUnicode_ToUppercase(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(Py_UCS4) _PyUnicode_ToTitlecase(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_ToLowerFull(
    Py_UCS4 ch,       /* Unicode character */
    Py_UCS4 *res
    );

PyAPI_FUNC(int) _PyUnicode_ToTitleFull(
    Py_UCS4 ch,       /* Unicode character */
    Py_UCS4 *res
    );

PyAPI_FUNC(int) _PyUnicode_ToUpperFull(
    Py_UCS4 ch,       /* Unicode character */
    Py_UCS4 *res
    );

PyAPI_FUNC(int) _PyUnicode_ToFoldedFull(
    Py_UCS4 ch,       /* Unicode character */
    Py_UCS4 *res
    );

PyAPI_FUNC(int) _PyUnicode_IsCaseIgnorable(
    Py_UCS4 ch         /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsCased(
    Py_UCS4 ch         /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_ToDecimalDigit(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_ToDigit(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(double) _PyUnicode_ToNumeric(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsDecimalDigit(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsDigit(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsNumeric(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsPrintable(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(int) _PyUnicode_IsAlpha(
    Py_UCS4 ch       /* Unicode character */
    );

PyAPI_FUNC(size_t) Py_UNICODE_strlen(
    const Py_UNICODE *u
    );

PyAPI_FUNC(Py_UNICODE*) Py_UNICODE_strcpy(
    Py_UNICODE *s1,
    const Py_UNICODE *s2);

PyAPI_FUNC(Py_UNICODE*) Py_UNICODE_strcat(
    Py_UNICODE *s1, const Py_UNICODE *s2);

PyAPI_FUNC(Py_UNICODE*) Py_UNICODE_strncpy(
    Py_UNICODE *s1,
    const Py_UNICODE *s2,
    size_t n);

PyAPI_FUNC(int) Py_UNICODE_strcmp(
    const Py_UNICODE *s1,
    const Py_UNICODE *s2
    );

PyAPI_FUNC(int) Py_UNICODE_strncmp(
    const Py_UNICODE *s1,
    const Py_UNICODE *s2,
    size_t n
    );

PyAPI_FUNC(Py_UNICODE*) Py_UNICODE_strchr(
    const Py_UNICODE *s,
    Py_UNICODE c
    );

PyAPI_FUNC(Py_UNICODE*) Py_UNICODE_strrchr(
    const Py_UNICODE *s,
    Py_UNICODE c
    );

PyAPI_FUNC(PyObject*) _PyUnicode_FormatLong(PyObject *, int, int, int);

/* Create a copy of a unicode string ending with a nul character. Return NULL
   and raise a MemoryError exception on memory allocation failure, otherwise
   return a new allocated buffer (use PyMem_Free() to free the buffer). */

PyAPI_FUNC(Py_UNICODE*) PyUnicode_AsUnicodeCopy(
    PyObject *unicode
    );
#endif /* Py_LIMITED_API */

#if defined(Py_DEBUG) && !defined(Py_LIMITED_API)
PyAPI_FUNC(int) _PyUnicode_CheckConsistency(
    PyObject *op,
    int check_content);
#endif

/* Return an interned Unicode object for an Identifier; may fail if there is no memory.*/
PyAPI_FUNC(PyObject*) _PyUnicode_FromId(_Py_Identifier*);
/* Clear all static strings. */
PyAPI_FUNC(void) _PyUnicode_ClearStaticStrings(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_UNICODEOBJECT_H */
