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
#include "pycore_bytesobject.h"   // PyBytesWriter members
#include "pycore_critical_section.h" // _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED()
#include "pycore_interp_structs.h" // _Py_error_handler
#include "pycore_unicodeobject.h" // struct _Py_unicode_state
#include "pycore_runtime.h"       // _Py_LATIN1_CHR()


static PyObject*
get_latin1_char(Py_UCS1 ch)
{
    return _Py_LATIN1_CHR(ch);
}


#define _Py_RETURN_UNICODE_EMPTY()   \
    do {                             \
        return _PyUnicode_GetEmpty();\
    } while (0)


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
        if (_PyUnicode_DecodeCallErrorHandler(
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
            if (_PyUnicode_DecodeCallErrorHandler(
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
            assert(ch <= _Py_MAX_UNICODE);

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

Py_ssize_t
_PyUnicode_DecodeASCII(const char *start, const char *end, Py_UCS1 *dest)
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
            if (_PyUnicode_DecodeCallErrorHandler(
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


PyObject *
_PyUnicode_DecodeUTF8(const char *s, Py_ssize_t size,
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
        decoded = _PyUnicode_DecodeASCII(s, end, dest);
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
    return _PyUnicode_DecodeUTF8(s, size,
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
            assert(ch > 0xFFFF && ch <= _Py_MAX_UNICODE);
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
            assert(ch <= _Py_MAX_UNICODE);
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
PyObject *
_PyUnicode_EncodeUTF8(PyObject *unicode, _Py_error_handler error_handler,
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
    return _PyUnicode_EncodeUTF8(unicode, _Py_ERROR_UNKNOWN, errors);
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
        if (_PyUnicode_DecodeCallErrorHandler(
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
        rep = _PyUnicode_EncodeCallErrorHandler(
                errors, &errorHandler,
                encoding, "surrogates not allowed",
                str, &exc, pos, pos + 1, &newpos);
        if (!rep)
            goto error;

        Py_ssize_t repsize, moreunits;
        if (PyBytes_Check(rep)) {
            repsize = PyBytes_GET_SIZE(rep);
            if (repsize & 3) {
                _PyUnicode_RaiseEncodeException(
                    &exc, encoding, str,
                    pos, pos + 1, "surrogates not allowed");
                goto error;
            }
            moreunits = repsize / 4;
        }
        else {
            assert(PyUnicode_Check(rep));
            moreunits = repsize = PyUnicode_GET_LENGTH(rep);
            if (!PyUnicode_IS_ASCII(rep)) {
                _PyUnicode_RaiseEncodeException(
                    &exc, encoding, str,
                    pos, pos + 1, "surrogates not allowed");
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

        if (_PyUnicode_DecodeCallErrorHandler(
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
        rep = _PyUnicode_EncodeCallErrorHandler(
                errors, &errorHandler,
                encoding, "surrogates not allowed",
                str, &exc, pos, pos + 1, &newpos);
        if (!rep)
            goto error;

        Py_ssize_t repsize, moreunits;
        if (PyBytes_Check(rep)) {
            repsize = PyBytes_GET_SIZE(rep);
            if (repsize & 1) {
                _PyUnicode_RaiseEncodeException(
                    &exc, encoding, str,
                    pos, pos + 1, "surrogates not allowed");
                goto error;
            }
            moreunits = repsize / 2;
        }
        else {
            assert(PyUnicode_Check(rep));
            moreunits = repsize = PyUnicode_GET_LENGTH(rep);
            if (!PyUnicode_IS_ASCII(rep)) {
                _PyUnicode_RaiseEncodeException(
                    &exc, encoding, str,
                    pos, pos + 1, "surrogates not allowed");
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
