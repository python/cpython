/* stringlib: codec implementations */

#if !STRINGLIB_IS_UNICODE
# error "codecs.h is specific to Unicode"
#endif

#include "pycore_bitutils.h"      // _Py_bswap32()

/* Mask to quickly check whether a C 'size_t' contains a
   non-ASCII, UTF8-encoded char. */
#if (SIZEOF_SIZE_T == 8)
# define ASCII_CHAR_MASK 0x8080808080808080ULL
#elif (SIZEOF_SIZE_T == 4)
# define ASCII_CHAR_MASK 0x80808080U
#else
# error C 'size_t' size should be either 4 or 8!
#endif

/* 10xxxxxx */
#define IS_CONTINUATION_BYTE(ch) ((ch) >= 0x80 && (ch) < 0xC0)

Py_LOCAL_INLINE(Py_UCS4)
STRINGLIB(utf8_decode)(const char **inptr, const char *end,
                       STRINGLIB_CHAR *dest,
                       Py_ssize_t *outpos)
{
    Py_UCS4 ch;
    const char *s = *inptr;
    STRINGLIB_CHAR *p = dest + *outpos;

    while (s < end) {
        ch = (unsigned char)*s;

        if (ch < 0x80) {
            /* Fast path for runs of ASCII characters. Given that common UTF-8
               input will consist of an overwhelming majority of ASCII
               characters, we try to optimize for this case by checking
               as many characters as a C 'size_t' can contain.
               First, check if we can do an aligned read, as most CPUs have
               a penalty for unaligned reads.
            */
            if (_Py_IS_ALIGNED(s, ALIGNOF_SIZE_T)) {
                /* Help register allocation */
                const char *_s = s;
                STRINGLIB_CHAR *_p = p;
                while (_s + SIZEOF_SIZE_T <= end) {
                    /* Read a whole size_t at a time (either 4 or 8 bytes),
                       and do a fast unrolled copy if it only contains ASCII
                       characters. */
                    size_t value = *(const size_t *) _s;
                    if (value & ASCII_CHAR_MASK)
                        break;
#if PY_LITTLE_ENDIAN
                    _p[0] = (STRINGLIB_CHAR)(value & 0xFFu);
                    _p[1] = (STRINGLIB_CHAR)((value >> 8) & 0xFFu);
                    _p[2] = (STRINGLIB_CHAR)((value >> 16) & 0xFFu);
                    _p[3] = (STRINGLIB_CHAR)((value >> 24) & 0xFFu);
# if SIZEOF_SIZE_T == 8
                    _p[4] = (STRINGLIB_CHAR)((value >> 32) & 0xFFu);
                    _p[5] = (STRINGLIB_CHAR)((value >> 40) & 0xFFu);
                    _p[6] = (STRINGLIB_CHAR)((value >> 48) & 0xFFu);
                    _p[7] = (STRINGLIB_CHAR)((value >> 56) & 0xFFu);
# endif
#else
# if SIZEOF_SIZE_T == 8
                    _p[0] = (STRINGLIB_CHAR)((value >> 56) & 0xFFu);
                    _p[1] = (STRINGLIB_CHAR)((value >> 48) & 0xFFu);
                    _p[2] = (STRINGLIB_CHAR)((value >> 40) & 0xFFu);
                    _p[3] = (STRINGLIB_CHAR)((value >> 32) & 0xFFu);
                    _p[4] = (STRINGLIB_CHAR)((value >> 24) & 0xFFu);
                    _p[5] = (STRINGLIB_CHAR)((value >> 16) & 0xFFu);
                    _p[6] = (STRINGLIB_CHAR)((value >> 8) & 0xFFu);
                    _p[7] = (STRINGLIB_CHAR)(value & 0xFFu);
# else
                    _p[0] = (STRINGLIB_CHAR)((value >> 24) & 0xFFu);
                    _p[1] = (STRINGLIB_CHAR)((value >> 16) & 0xFFu);
                    _p[2] = (STRINGLIB_CHAR)((value >> 8) & 0xFFu);
                    _p[3] = (STRINGLIB_CHAR)(value & 0xFFu);
# endif
#endif
                    _s += SIZEOF_SIZE_T;
                    _p += SIZEOF_SIZE_T;
                }
                s = _s;
                p = _p;
                if (s == end)
                    break;
                ch = (unsigned char)*s;
            }
            if (ch < 0x80) {
                s++;
                *p++ = ch;
                continue;
            }
        }

        if (ch < 0xE0) {
            /* \xC2\x80-\xDF\xBF -- 0080-07FF */
            Py_UCS4 ch2;
            if (ch < 0xC2) {
                /* invalid sequence
                \x80-\xBF -- continuation byte
                \xC0-\xC1 -- fake 0000-007F */
                goto InvalidStart;
            }
            if (end - s < 2) {
                /* unexpected end of data: the caller will decide whether
                   it's an error or not */
                break;
            }
            ch2 = (unsigned char)s[1];
            if (!IS_CONTINUATION_BYTE(ch2))
                /* invalid continuation byte */
                goto InvalidContinuation1;
            ch = (ch << 6) + ch2 -
                 ((0xC0 << 6) + 0x80);
            assert ((ch > 0x007F) && (ch <= 0x07FF));
            s += 2;
            if (STRINGLIB_MAX_CHAR <= 0x007F ||
                (STRINGLIB_MAX_CHAR < 0x07FF && ch > STRINGLIB_MAX_CHAR))
                /* Out-of-range */
                goto Return;
            *p++ = ch;
            continue;
        }

        if (ch < 0xF0) {
            /* \xE0\xA0\x80-\xEF\xBF\xBF -- 0800-FFFF */
            Py_UCS4 ch2, ch3;
            if (end - s < 3) {
                /* unexpected end of data: the caller will decide whether
                   it's an error or not */
                if (end - s < 2)
                    break;
                ch2 = (unsigned char)s[1];
                if (!IS_CONTINUATION_BYTE(ch2) ||
                    (ch2 < 0xA0 ? ch == 0xE0 : ch == 0xED))
                    /* for clarification see comments below */
                    goto InvalidContinuation1;
                break;
            }
            ch2 = (unsigned char)s[1];
            ch3 = (unsigned char)s[2];
            if (!IS_CONTINUATION_BYTE(ch2)) {
                /* invalid continuation byte */
                goto InvalidContinuation1;
            }
            if (ch == 0xE0) {
                if (ch2 < 0xA0)
                    /* invalid sequence
                       \xE0\x80\x80-\xE0\x9F\xBF -- fake 0000-0800 */
                    goto InvalidContinuation1;
            } else if (ch == 0xED && ch2 >= 0xA0) {
                /* Decoding UTF-8 sequences in range \xED\xA0\x80-\xED\xBF\xBF
                   will result in surrogates in range D800-DFFF. Surrogates are
                   not valid UTF-8 so they are rejected.
                   See https://www.unicode.org/versions/Unicode5.2.0/ch03.pdf
                   (table 3-7) and http://www.rfc-editor.org/rfc/rfc3629.txt */
                goto InvalidContinuation1;
            }
            if (!IS_CONTINUATION_BYTE(ch3)) {
                /* invalid continuation byte */
                goto InvalidContinuation2;
            }
            ch = (ch << 12) + (ch2 << 6) + ch3 -
                 ((0xE0 << 12) + (0x80 << 6) + 0x80);
            assert ((ch > 0x07FF) && (ch <= 0xFFFF));
            s += 3;
            if (STRINGLIB_MAX_CHAR <= 0x07FF ||
                (STRINGLIB_MAX_CHAR < 0xFFFF && ch > STRINGLIB_MAX_CHAR))
                /* Out-of-range */
                goto Return;
            *p++ = ch;
            continue;
        }

        if (ch < 0xF5) {
            /* \xF0\x90\x80\x80-\xF4\x8F\xBF\xBF -- 10000-10FFFF */
            Py_UCS4 ch2, ch3, ch4;
            if (end - s < 4) {
                /* unexpected end of data: the caller will decide whether
                   it's an error or not */
                if (end - s < 2)
                    break;
                ch2 = (unsigned char)s[1];
                if (!IS_CONTINUATION_BYTE(ch2) ||
                    (ch2 < 0x90 ? ch == 0xF0 : ch == 0xF4))
                    /* for clarification see comments below */
                    goto InvalidContinuation1;
                if (end - s < 3)
                    break;
                ch3 = (unsigned char)s[2];
                if (!IS_CONTINUATION_BYTE(ch3))
                    goto InvalidContinuation2;
                break;
            }
            ch2 = (unsigned char)s[1];
            ch3 = (unsigned char)s[2];
            ch4 = (unsigned char)s[3];
            if (!IS_CONTINUATION_BYTE(ch2)) {
                /* invalid continuation byte */
                goto InvalidContinuation1;
            }
            if (ch == 0xF0) {
                if (ch2 < 0x90)
                    /* invalid sequence
                       \xF0\x80\x80\x80-\xF0\x8F\xBF\xBF -- fake 0000-FFFF */
                    goto InvalidContinuation1;
            } else if (ch == 0xF4 && ch2 >= 0x90) {
                /* invalid sequence
                   \xF4\x90\x80\x80- -- 110000- overflow */
                goto InvalidContinuation1;
            }
            if (!IS_CONTINUATION_BYTE(ch3)) {
                /* invalid continuation byte */
                goto InvalidContinuation2;
            }
            if (!IS_CONTINUATION_BYTE(ch4)) {
                /* invalid continuation byte */
                goto InvalidContinuation3;
            }
            ch = (ch << 18) + (ch2 << 12) + (ch3 << 6) + ch4 -
                 ((0xF0 << 18) + (0x80 << 12) + (0x80 << 6) + 0x80);
            assert ((ch > 0xFFFF) && (ch <= 0x10FFFF));
            s += 4;
            if (STRINGLIB_MAX_CHAR <= 0xFFFF ||
                (STRINGLIB_MAX_CHAR < 0x10FFFF && ch > STRINGLIB_MAX_CHAR))
                /* Out-of-range */
                goto Return;
            *p++ = ch;
            continue;
        }
        goto InvalidStart;
    }
    ch = 0;
Return:
    *inptr = s;
    *outpos = p - dest;
    return ch;
InvalidStart:
    ch = 1;
    goto Return;
InvalidContinuation1:
    ch = 2;
    goto Return;
InvalidContinuation2:
    ch = 3;
    goto Return;
InvalidContinuation3:
    ch = 4;
    goto Return;
}

#undef ASCII_CHAR_MASK


/* UTF-8 encoder specialized for a Unicode kind to avoid the slow
   PyUnicode_READ() macro. Delete some parts of the code depending on the kind:
   UCS-1 strings don't need to handle surrogates for example. */
Py_LOCAL_INLINE(char *)
STRINGLIB(utf8_encoder)(_PyBytesWriter *writer,
                        PyObject *unicode,
                        const STRINGLIB_CHAR *data,
                        Py_ssize_t size,
                        _Py_error_handler error_handler,
                        const char *errors)
{
    Py_ssize_t i;                /* index into data of next input character */
    char *p;                     /* next free byte in output buffer */
#if STRINGLIB_SIZEOF_CHAR > 1
    PyObject *error_handler_obj = NULL;
    PyObject *exc = NULL;
    PyObject *rep = NULL;
#endif
#if STRINGLIB_SIZEOF_CHAR == 1
    const Py_ssize_t max_char_size = 2;
#elif STRINGLIB_SIZEOF_CHAR == 2
    const Py_ssize_t max_char_size = 3;
#else /*  STRINGLIB_SIZEOF_CHAR == 4 */
    const Py_ssize_t max_char_size = 4;
#endif

    assert(size >= 0);
    if (size > PY_SSIZE_T_MAX / max_char_size) {
        /* integer overflow */
        PyErr_NoMemory();
        return NULL;
    }

    _PyBytesWriter_Init(writer);
    p = _PyBytesWriter_Alloc(writer, size * max_char_size);
    if (p == NULL)
        return NULL;

    for (i = 0; i < size;) {
        Py_UCS4 ch = data[i++];

        if (ch < 0x80) {
            /* Encode ASCII */
            *p++ = (char) ch;

        }
        else
#if STRINGLIB_SIZEOF_CHAR > 1
        if (ch < 0x0800)
#endif
        {
            /* Encode Latin-1 */
            *p++ = (char)(0xc0 | (ch >> 6));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
#if STRINGLIB_SIZEOF_CHAR > 1
        else if (Py_UNICODE_IS_SURROGATE(ch)) {
            Py_ssize_t startpos, endpos, newpos;
            Py_ssize_t k;
            if (error_handler == _Py_ERROR_UNKNOWN) {
                error_handler = _Py_GetErrorHandler(errors);
            }

            startpos = i-1;
            endpos = startpos+1;

            while ((endpos < size) && Py_UNICODE_IS_SURROGATE(data[endpos]))
                endpos++;

            /* Only overallocate the buffer if it's not the last write */
            writer->overallocate = (endpos < size);

            switch (error_handler)
            {
            case _Py_ERROR_REPLACE:
                memset(p, '?', endpos - startpos);
                p += (endpos - startpos);
                /* fall through */
            case _Py_ERROR_IGNORE:
                i += (endpos - startpos - 1);
                break;

            case _Py_ERROR_SURROGATEPASS:
                for (k=startpos; k<endpos; k++) {
                    ch = data[k];
                    *p++ = (char)(0xe0 | (ch >> 12));
                    *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
                    *p++ = (char)(0x80 | (ch & 0x3f));
                }
                i += (endpos - startpos - 1);
                break;

            case _Py_ERROR_BACKSLASHREPLACE:
                /* subtract preallocated bytes */
                writer->min_size -= max_char_size * (endpos - startpos);
                p = backslashreplace(writer, p,
                                     unicode, startpos, endpos);
                if (p == NULL)
                    goto error;
                i += (endpos - startpos - 1);
                break;

            case _Py_ERROR_XMLCHARREFREPLACE:
                /* subtract preallocated bytes */
                writer->min_size -= max_char_size * (endpos - startpos);
                p = xmlcharrefreplace(writer, p,
                                      unicode, startpos, endpos);
                if (p == NULL)
                    goto error;
                i += (endpos - startpos - 1);
                break;

            case _Py_ERROR_SURROGATEESCAPE:
                for (k=startpos; k<endpos; k++) {
                    ch = data[k];
                    if (!(0xDC80 <= ch && ch <= 0xDCFF))
                        break;
                    *p++ = (char)(ch & 0xff);
                }
                if (k >= endpos) {
                    i += (endpos - startpos - 1);
                    break;
                }
                startpos = k;
                assert(startpos < endpos);
                /* fall through */
            default:
                rep = unicode_encode_call_errorhandler(
                      errors, &error_handler_obj, "utf-8", "surrogates not allowed",
                      unicode, &exc, startpos, endpos, &newpos);
                if (!rep)
                    goto error;

                if (newpos < startpos) {
                    writer->overallocate = 1;
                    p = _PyBytesWriter_Prepare(writer, p,
                                               max_char_size * (startpos - newpos));
                    if (p == NULL)
                        goto error;
                }
                else {
                    /* subtract preallocated bytes */
                    writer->min_size -= max_char_size * (newpos - startpos);
                    /* Only overallocate the buffer if it's not the last write */
                    writer->overallocate = (newpos < size);
                }

                if (PyBytes_Check(rep)) {
                    p = _PyBytesWriter_WriteBytes(writer, p,
                                                  PyBytes_AS_STRING(rep),
                                                  PyBytes_GET_SIZE(rep));
                }
                else {
                    /* rep is unicode */
                    if (PyUnicode_READY(rep) < 0)
                        goto error;

                    if (!PyUnicode_IS_ASCII(rep)) {
                        raise_encode_exception(&exc, "utf-8", unicode,
                                               startpos, endpos,
                                               "surrogates not allowed");
                        goto error;
                    }

                    p = _PyBytesWriter_WriteBytes(writer, p,
                                                  PyUnicode_DATA(rep),
                                                  PyUnicode_GET_LENGTH(rep));
                }

                if (p == NULL)
                    goto error;
                Py_CLEAR(rep);

                i = newpos;
            }

            /* If overallocation was disabled, ensure that it was the last
               write. Otherwise, we missed an optimization */
            assert(writer->overallocate || i == size);
        }
        else
#if STRINGLIB_SIZEOF_CHAR > 2
        if (ch < 0x10000)
#endif
        {
            *p++ = (char)(0xe0 | (ch >> 12));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
#if STRINGLIB_SIZEOF_CHAR > 2
        else /* ch >= 0x10000 */
        {
            assert(ch <= MAX_UNICODE);
            /* Encode UCS4 Unicode ordinals */
            *p++ = (char)(0xf0 | (ch >> 18));
            *p++ = (char)(0x80 | ((ch >> 12) & 0x3f));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
#endif /* STRINGLIB_SIZEOF_CHAR > 2 */
#endif /* STRINGLIB_SIZEOF_CHAR > 1 */
    }

#if STRINGLIB_SIZEOF_CHAR > 1
    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
#endif
    return p;

#if STRINGLIB_SIZEOF_CHAR > 1
 error:
    Py_XDECREF(rep);
    Py_XDECREF(error_handler_obj);
    Py_XDECREF(exc);
    return NULL;
#endif
}

/* The pattern for constructing UCS2-repeated masks. */
#if SIZEOF_LONG == 8
# define UCS2_REPEAT_MASK 0x0001000100010001ul
#elif SIZEOF_LONG == 4
# define UCS2_REPEAT_MASK 0x00010001ul
#else
# error C 'long' size should be either 4 or 8!
#endif

/* The mask for fast checking. */
#if STRINGLIB_SIZEOF_CHAR == 1
/* The mask for fast checking of whether a C 'long' contains a
   non-ASCII or non-Latin1 UTF16-encoded characters. */
# define FAST_CHAR_MASK         (UCS2_REPEAT_MASK * (0xFFFFu & ~STRINGLIB_MAX_CHAR))
#else
/* The mask for fast checking of whether a C 'long' may contain
   UTF16-encoded surrogate characters. This is an efficient heuristic,
   assuming that non-surrogate characters with a code point >= 0x8000 are
   rare in most input.
*/
# define FAST_CHAR_MASK         (UCS2_REPEAT_MASK * 0x8000u)
#endif
/* The mask for fast byte-swapping. */
#define STRIPPED_MASK           (UCS2_REPEAT_MASK * 0x00FFu)
/* Swap bytes. */
#define SWAB(value)             ((((value) >> 8) & STRIPPED_MASK) | \
                                 (((value) & STRIPPED_MASK) << 8))

Py_LOCAL_INLINE(Py_UCS4)
STRINGLIB(utf16_decode)(const unsigned char **inptr, const unsigned char *e,
                        STRINGLIB_CHAR *dest, Py_ssize_t *outpos,
                        int native_ordering)
{
    Py_UCS4 ch;
    const unsigned char *q = *inptr;
    STRINGLIB_CHAR *p = dest + *outpos;
    /* Offsets from q for retrieving byte pairs in the right order. */
#if PY_LITTLE_ENDIAN
    int ihi = !!native_ordering, ilo = !native_ordering;
#else
    int ihi = !native_ordering, ilo = !!native_ordering;
#endif
    --e;

    while (q < e) {
        Py_UCS4 ch2;
        /* First check for possible aligned read of a C 'long'. Unaligned
           reads are more expensive, better to defer to another iteration. */
        if (_Py_IS_ALIGNED(q, ALIGNOF_LONG)) {
            /* Fast path for runs of in-range non-surrogate chars. */
            const unsigned char *_q = q;
            while (_q + SIZEOF_LONG <= e) {
                unsigned long block = * (const unsigned long *) _q;
                if (native_ordering) {
                    /* Can use buffer directly */
                    if (block & FAST_CHAR_MASK)
                        break;
                }
                else {
                    /* Need to byte-swap */
                    if (block & SWAB(FAST_CHAR_MASK))
                        break;
#if STRINGLIB_SIZEOF_CHAR == 1
                    block >>= 8;
#else
                    block = SWAB(block);
#endif
                }
#if PY_LITTLE_ENDIAN
# if SIZEOF_LONG == 4
                p[0] = (STRINGLIB_CHAR)(block & 0xFFFFu);
                p[1] = (STRINGLIB_CHAR)(block >> 16);
# elif SIZEOF_LONG == 8
                p[0] = (STRINGLIB_CHAR)(block & 0xFFFFu);
                p[1] = (STRINGLIB_CHAR)((block >> 16) & 0xFFFFu);
                p[2] = (STRINGLIB_CHAR)((block >> 32) & 0xFFFFu);
                p[3] = (STRINGLIB_CHAR)(block >> 48);
# endif
#else
# if SIZEOF_LONG == 4
                p[0] = (STRINGLIB_CHAR)(block >> 16);
                p[1] = (STRINGLIB_CHAR)(block & 0xFFFFu);
# elif SIZEOF_LONG == 8
                p[0] = (STRINGLIB_CHAR)(block >> 48);
                p[1] = (STRINGLIB_CHAR)((block >> 32) & 0xFFFFu);
                p[2] = (STRINGLIB_CHAR)((block >> 16) & 0xFFFFu);
                p[3] = (STRINGLIB_CHAR)(block & 0xFFFFu);
# endif
#endif
                _q += SIZEOF_LONG;
                p += SIZEOF_LONG / 2;
            }
            q = _q;
            if (q >= e)
                break;
        }

        ch = (q[ihi] << 8) | q[ilo];
        q += 2;
        if (!Py_UNICODE_IS_SURROGATE(ch)) {
#if STRINGLIB_SIZEOF_CHAR < 2
            if (ch > STRINGLIB_MAX_CHAR)
                /* Out-of-range */
                goto Return;
#endif
            *p++ = (STRINGLIB_CHAR)ch;
            continue;
        }

        /* UTF-16 code pair: */
        if (!Py_UNICODE_IS_HIGH_SURROGATE(ch))
            goto IllegalEncoding;
        if (q >= e)
            goto UnexpectedEnd;
        ch2 = (q[ihi] << 8) | q[ilo];
        q += 2;
        if (!Py_UNICODE_IS_LOW_SURROGATE(ch2))
            goto IllegalSurrogate;
        ch = Py_UNICODE_JOIN_SURROGATES(ch, ch2);
#if STRINGLIB_SIZEOF_CHAR < 4
        /* Out-of-range */
        goto Return;
#else
        *p++ = (STRINGLIB_CHAR)ch;
#endif
    }
    ch = 0;
Return:
    *inptr = q;
    *outpos = p - dest;
    return ch;
UnexpectedEnd:
    ch = 1;
    goto Return;
IllegalEncoding:
    ch = 2;
    goto Return;
IllegalSurrogate:
    ch = 3;
    goto Return;
}
#undef UCS2_REPEAT_MASK
#undef FAST_CHAR_MASK
#undef STRIPPED_MASK
#undef SWAB


#if STRINGLIB_MAX_CHAR >= 0x80
Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(utf16_encode)(const STRINGLIB_CHAR *in,
                        Py_ssize_t len,
                        unsigned short **outptr,
                        int native_ordering)
{
    unsigned short *out = *outptr;
    const STRINGLIB_CHAR *end = in + len;
#if STRINGLIB_SIZEOF_CHAR == 1
    if (native_ordering) {
        const STRINGLIB_CHAR *unrolled_end = in + _Py_SIZE_ROUND_DOWN(len, 4);
        while (in < unrolled_end) {
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
            out[3] = in[3];
            in += 4; out += 4;
        }
        while (in < end) {
            *out++ = *in++;
        }
    } else {
# define SWAB2(CH)  ((CH) << 8) /* high byte is zero */
        const STRINGLIB_CHAR *unrolled_end = in + _Py_SIZE_ROUND_DOWN(len, 4);
        while (in < unrolled_end) {
            out[0] = SWAB2(in[0]);
            out[1] = SWAB2(in[1]);
            out[2] = SWAB2(in[2]);
            out[3] = SWAB2(in[3]);
            in += 4; out += 4;
        }
        while (in < end) {
            Py_UCS4 ch = *in++;
            *out++ = SWAB2((Py_UCS2)ch);
        }
#undef SWAB2
    }
    *outptr = out;
    return len;
#else
    if (native_ordering) {
#if STRINGLIB_MAX_CHAR < 0x10000
        const STRINGLIB_CHAR *unrolled_end = in + _Py_SIZE_ROUND_DOWN(len, 4);
        while (in < unrolled_end) {
            /* check if any character is a surrogate character */
            if (((in[0] ^ 0xd800) &
                 (in[1] ^ 0xd800) &
                 (in[2] ^ 0xd800) &
                 (in[3] ^ 0xd800) & 0xf800) == 0)
                break;
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
            out[3] = in[3];
            in += 4; out += 4;
        }
#endif
        while (in < end) {
            Py_UCS4 ch;
            ch = *in++;
            if (ch < 0xd800)
                *out++ = ch;
            else if (ch < 0xe000)
                /* reject surrogate characters (U+D800-U+DFFF) */
                goto fail;
#if STRINGLIB_MAX_CHAR >= 0x10000
            else if (ch >= 0x10000) {
                out[0] = Py_UNICODE_HIGH_SURROGATE(ch);
                out[1] = Py_UNICODE_LOW_SURROGATE(ch);
                out += 2;
            }
#endif
            else
                *out++ = ch;
        }
    } else {
#define SWAB2(CH)  (((CH) << 8) | ((CH) >> 8))
#if STRINGLIB_MAX_CHAR < 0x10000
        const STRINGLIB_CHAR *unrolled_end = in + _Py_SIZE_ROUND_DOWN(len, 4);
        while (in < unrolled_end) {
            /* check if any character is a surrogate character */
            if (((in[0] ^ 0xd800) &
                 (in[1] ^ 0xd800) &
                 (in[2] ^ 0xd800) &
                 (in[3] ^ 0xd800) & 0xf800) == 0)
                break;
            out[0] = SWAB2(in[0]);
            out[1] = SWAB2(in[1]);
            out[2] = SWAB2(in[2]);
            out[3] = SWAB2(in[3]);
            in += 4; out += 4;
        }
#endif
        while (in < end) {
            Py_UCS4 ch = *in++;
            if (ch < 0xd800)
                *out++ = SWAB2((Py_UCS2)ch);
            else if (ch < 0xe000)
                /* reject surrogate characters (U+D800-U+DFFF) */
                goto fail;
#if STRINGLIB_MAX_CHAR >= 0x10000
            else if (ch >= 0x10000) {
                Py_UCS2 ch1 = Py_UNICODE_HIGH_SURROGATE(ch);
                Py_UCS2 ch2 = Py_UNICODE_LOW_SURROGATE(ch);
                out[0] = SWAB2(ch1);
                out[1] = SWAB2(ch2);
                out += 2;
            }
#endif
            else
                *out++ = SWAB2((Py_UCS2)ch);
        }
#undef SWAB2
    }
    *outptr = out;
    return len;
  fail:
    *outptr = out;
    return len - (end - in + 1);
#endif
}

static inline uint32_t
STRINGLIB(SWAB4)(STRINGLIB_CHAR ch)
{
    uint32_t word = ch;
#if STRINGLIB_SIZEOF_CHAR == 1
    /* high bytes are zero */
    return (word << 24);
#elif STRINGLIB_SIZEOF_CHAR == 2
    /* high bytes are zero */
    return ((word & 0x00FFu) << 24) | ((word & 0xFF00u) << 8);
#else
    return _Py_bswap32(word);
#endif
}

Py_LOCAL_INLINE(Py_ssize_t)
STRINGLIB(utf32_encode)(const STRINGLIB_CHAR *in,
                        Py_ssize_t len,
                        uint32_t **outptr,
                        int native_ordering)
{
    uint32_t *out = *outptr;
    const STRINGLIB_CHAR *end = in + len;
    if (native_ordering) {
        const STRINGLIB_CHAR *unrolled_end = in + _Py_SIZE_ROUND_DOWN(len, 4);
        while (in < unrolled_end) {
#if STRINGLIB_SIZEOF_CHAR > 1
            /* check if any character is a surrogate character */
            if (((in[0] ^ 0xd800) &
                 (in[1] ^ 0xd800) &
                 (in[2] ^ 0xd800) &
                 (in[3] ^ 0xd800) & 0xf800) == 0)
                break;
#endif
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
            out[3] = in[3];
            in += 4; out += 4;
        }
        while (in < end) {
            Py_UCS4 ch;
            ch = *in++;
#if STRINGLIB_SIZEOF_CHAR > 1
            if (Py_UNICODE_IS_SURROGATE(ch)) {
                /* reject surrogate characters (U+D800-U+DFFF) */
                goto fail;
            }
#endif
            *out++ = ch;
        }
    } else {
        const STRINGLIB_CHAR *unrolled_end = in + _Py_SIZE_ROUND_DOWN(len, 4);
        while (in < unrolled_end) {
#if STRINGLIB_SIZEOF_CHAR > 1
            /* check if any character is a surrogate character */
            if (((in[0] ^ 0xd800) &
                 (in[1] ^ 0xd800) &
                 (in[2] ^ 0xd800) &
                 (in[3] ^ 0xd800) & 0xf800) == 0)
                break;
#endif
            out[0] = STRINGLIB(SWAB4)(in[0]);
            out[1] = STRINGLIB(SWAB4)(in[1]);
            out[2] = STRINGLIB(SWAB4)(in[2]);
            out[3] = STRINGLIB(SWAB4)(in[3]);
            in += 4; out += 4;
        }
        while (in < end) {
            Py_UCS4 ch = *in++;
#if STRINGLIB_SIZEOF_CHAR > 1
            if (Py_UNICODE_IS_SURROGATE(ch)) {
                /* reject surrogate characters (U+D800-U+DFFF) */
                goto fail;
            }
#endif
            *out++ = STRINGLIB(SWAB4)(ch);
        }
    }
    *outptr = out;
    return len;
#if STRINGLIB_SIZEOF_CHAR > 1
  fail:
    *outptr = out;
    return len - (end - in + 1);
#endif
}

#endif
