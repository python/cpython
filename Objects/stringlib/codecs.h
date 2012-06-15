/* stringlib: codec implementations */

#if STRINGLIB_IS_UNICODE

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

Py_LOCAL_INLINE(Py_UCS4)
STRINGLIB(utf8_decode)(const char **inptr, const char *end,
                       STRINGLIB_CHAR *dest,
                       Py_ssize_t *outpos)
{
    Py_UCS4 ch;
    const char *s = *inptr;
    const char *aligned_end = (const char *) ((size_t) end & ~LONG_PTR_MASK);
    STRINGLIB_CHAR *p = dest + *outpos;

    while (s < end) {
        ch = (unsigned char)*s;

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
                register STRINGLIB_CHAR *_p = p;
                while (_s < aligned_end) {
                    /* Read a whole long at a time (either 4 or 8 bytes),
                       and do a fast unrolled copy if it only contains ASCII
                       characters. */
                    unsigned long value = *(unsigned long *) _s;
                    if (value & ASCII_CHAR_MASK)
                        break;
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
                    _p[0] = (STRINGLIB_CHAR)(value & 0xFFu);
                    _p[1] = (STRINGLIB_CHAR)((value >> 8) & 0xFFu);
                    _p[2] = (STRINGLIB_CHAR)((value >> 16) & 0xFFu);
                    _p[3] = (STRINGLIB_CHAR)((value >> 24) & 0xFFu);
# if SIZEOF_LONG == 8
                    _p[4] = (STRINGLIB_CHAR)((value >> 32) & 0xFFu);
                    _p[5] = (STRINGLIB_CHAR)((value >> 40) & 0xFFu);
                    _p[6] = (STRINGLIB_CHAR)((value >> 48) & 0xFFu);
                    _p[7] = (STRINGLIB_CHAR)((value >> 56) & 0xFFu);
# endif
#else
# if SIZEOF_LONG == 8
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
                    _s += SIZEOF_LONG;
                    _p += SIZEOF_LONG;
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

        if (ch < 0xC2) {
            /* invalid sequence
               \x80-\xBF -- continuation byte
               \xC0-\xC1 -- fake 0000-007F */
            goto InvalidStart;
        }

        if (ch < 0xE0) {
            /* \xC2\x80-\xDF\xBF -- 0080-07FF */
            Py_UCS4 ch2;
            if (end - s < 2) {
                /* unexpected end of data: the caller will decide whether
                   it's an error or not */
                break;
            }
            ch2 = (unsigned char)s[1];
            if ((ch2 & 0xC0) != 0x80)
                /* invalid continuation byte */
                goto InvalidContinuation;
            ch = (ch << 6) + ch2 -
                 ((0xC0 << 6) + 0x80);
            assert ((ch > 0x007F) && (ch <= 0x07FF));
            s += 2;
            if (STRINGLIB_MAX_CHAR <= 0x007F ||
                (STRINGLIB_MAX_CHAR < 0x07FF && ch > STRINGLIB_MAX_CHAR))
                goto Overflow;
            *p++ = ch;
            continue;
        }

        if (ch < 0xF0) {
            /* \xE0\xA0\x80-\xEF\xBF\xBF -- 0800-FFFF */
            Py_UCS4 ch2, ch3;
            if (end - s < 3) {
                /* unexpected end of data: the caller will decide whether
                   it's an error or not */
                break;
            }
            ch2 = (unsigned char)s[1];
            ch3 = (unsigned char)s[2];
            if ((ch2 & 0xC0) != 0x80 ||
                (ch3 & 0xC0) != 0x80) {
                /* invalid continuation byte */
                goto InvalidContinuation;
            }
            if (ch == 0xE0) {
                if (ch2 < 0xA0)
                    /* invalid sequence
                       \xE0\x80\x80-\xE0\x9F\xBF -- fake 0000-0800 */
                    goto InvalidContinuation;
            }
            else if (ch == 0xED && ch2 > 0x9F) {
                /* Decoding UTF-8 sequences in range \xED\xA0\x80-\xED\xBF\xBF
                   will result in surrogates in range D800-DFFF. Surrogates are
                   not valid UTF-8 so they are rejected.
                   See http://www.unicode.org/versions/Unicode5.2.0/ch03.pdf
                   (table 3-7) and http://www.rfc-editor.org/rfc/rfc3629.txt */
                goto InvalidContinuation;
            }
            ch = (ch << 12) + (ch2 << 6) + ch3 -
                 ((0xE0 << 12) + (0x80 << 6) + 0x80);
            assert ((ch > 0x07FF) && (ch <= 0xFFFF));
            s += 3;
            if (STRINGLIB_MAX_CHAR <= 0x07FF ||
                (STRINGLIB_MAX_CHAR < 0xFFFF && ch > STRINGLIB_MAX_CHAR))
                goto Overflow;
            *p++ = ch;
            continue;
        }

        if (ch < 0xF5) {
            /* \xF0\x90\x80\x80-\xF4\x8F\xBF\xBF -- 10000-10FFFF */
            Py_UCS4 ch2, ch3, ch4;
            if (end - s < 4) {
                /* unexpected end of data: the caller will decide whether
                   it's an error or not */
                break;
            }
            ch2 = (unsigned char)s[1];
            ch3 = (unsigned char)s[2];
            ch4 = (unsigned char)s[3];
            if ((ch2 & 0xC0) != 0x80 ||
                (ch3 & 0xC0) != 0x80 ||
                (ch4 & 0xC0) != 0x80) {
                /* invalid continuation byte */
                goto InvalidContinuation;
            }
            if (ch == 0xF0) {
                if (ch2 < 0x90)
                    /* invalid sequence
                       \xF0\x80\x80\x80-\xF0\x80\xBF\xBF -- fake 0000-FFFF */
                    goto InvalidContinuation;
            }
            else if (ch == 0xF4 && ch2 > 0x8F) {
                /* invalid sequence
                   \xF4\x90\x80\80- -- 110000- overflow */
                goto InvalidContinuation;
            }
            ch = (ch << 18) + (ch2 << 12) + (ch3 << 6) + ch4 -
                 ((0xF0 << 18) + (0x80 << 12) + (0x80 << 6) + 0x80);
            assert ((ch > 0xFFFF) && (ch <= 0x10FFFF));
            s += 4;
            if (STRINGLIB_MAX_CHAR <= 0xFFFF ||
                (STRINGLIB_MAX_CHAR < 0x10FFFF && ch > STRINGLIB_MAX_CHAR))
                goto Overflow;
            *p++ = ch;
            continue;
        }
        goto InvalidStart;
    }
    ch = 0;
Overflow:
Return:
    *inptr = s;
    *outpos = p - dest;
    return ch;
InvalidStart:
    ch = 1;
    goto Return;
InvalidContinuation:
    ch = 2;
    goto Return;
}

#undef ASCII_CHAR_MASK


/* UTF-8 encoder specialized for a Unicode kind to avoid the slow
   PyUnicode_READ() macro. Delete some parts of the code depending on the kind:
   UCS-1 strings don't need to handle surrogates for example. */
Py_LOCAL_INLINE(PyObject *)
STRINGLIB(utf8_encoder)(PyObject *unicode,
                        STRINGLIB_CHAR *data,
                        Py_ssize_t size,
                        const char *errors)
{
#define MAX_SHORT_UNICHARS 300  /* largest size we'll do on the stack */

    Py_ssize_t i;                /* index into s of next input byte */
    PyObject *result;            /* result string object */
    char *p;                     /* next free byte in output buffer */
    Py_ssize_t nallocated;      /* number of result bytes allocated */
    Py_ssize_t nneeded;            /* number of result bytes needed */
#if STRINGLIB_SIZEOF_CHAR > 1
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    PyObject *rep = NULL;
#endif
#if STRINGLIB_SIZEOF_CHAR == 1
    const Py_ssize_t max_char_size = 2;
    char stackbuf[MAX_SHORT_UNICHARS * 2];
#elif STRINGLIB_SIZEOF_CHAR == 2
    const Py_ssize_t max_char_size = 3;
    char stackbuf[MAX_SHORT_UNICHARS * 3];
#else /*  STRINGLIB_SIZEOF_CHAR == 4 */
    const Py_ssize_t max_char_size = 4;
    char stackbuf[MAX_SHORT_UNICHARS * 4];
#endif

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
        if (size > PY_SSIZE_T_MAX / max_char_size) {
            /* integer overflow */
            return PyErr_NoMemory();
        }
        /* Overallocate on the heap, and give the excess back at the end. */
        nallocated = size * max_char_size;
        result = PyBytes_FromStringAndSize(NULL, nallocated);
        if (result == NULL)
            return NULL;
        p = PyBytes_AS_STRING(result);
    }

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
            Py_ssize_t newpos;
            Py_ssize_t repsize, k, startpos;
            startpos = i-1;
            rep = unicode_encode_call_errorhandler(
                  errors, &errorHandler, "utf-8", "surrogates not allowed",
                  unicode, &exc, startpos, startpos+1, &newpos);
            if (!rep)
                goto error;

            if (PyBytes_Check(rep))
                repsize = PyBytes_GET_SIZE(rep);
            else
                repsize = PyUnicode_GET_LENGTH(rep);

            if (repsize > max_char_size) {
                Py_ssize_t offset;

                if (result == NULL)
                    offset = p - stackbuf;
                else
                    offset = p - PyBytes_AS_STRING(result);

                if (nallocated > PY_SSIZE_T_MAX - repsize + max_char_size) {
                    /* integer overflow */
                    PyErr_NoMemory();
                    goto error;
                }
                nallocated += repsize - max_char_size;
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
                enum PyUnicode_Kind repkind;
                void *repdata;

                if (PyUnicode_READY(rep) < 0)
                    goto error;
                repkind = PyUnicode_KIND(rep);
                repdata = PyUnicode_DATA(rep);

                for(k=0; k<repsize; k++) {
                    Py_UCS4 c = PyUnicode_READ(repkind, repdata, k);
                    if (0x80 <= c) {
                        raise_encode_exception(&exc, "utf-8",
                                               unicode,
                                               i-1, i,
                                               "surrogates not allowed");
                        goto error;
                    }
                    *p++ = (char)c;
                }
            }
            Py_CLEAR(rep);
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

#if STRINGLIB_SIZEOF_CHAR > 1
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
#endif
    return result;

#if STRINGLIB_SIZEOF_CHAR > 1
 error:
    Py_XDECREF(rep);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_XDECREF(result);
    return NULL;
#endif

#undef MAX_SHORT_UNICHARS
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
    const unsigned char *aligned_end =
            (const unsigned char *) ((size_t) e & ~LONG_PTR_MASK);
    const unsigned char *q = *inptr;
    STRINGLIB_CHAR *p = dest + *outpos;
    /* Offsets from q for retrieving byte pairs in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int ihi = !!native_ordering, ilo = !native_ordering;
#else
    int ihi = !native_ordering, ilo = !!native_ordering;
#endif
    --e;

    while (q < e) {
        Py_UCS4 ch2;
        /* First check for possible aligned read of a C 'long'. Unaligned
           reads are more expensive, better to defer to another iteration. */
        if (!((size_t) q & LONG_PTR_MASK)) {
            /* Fast path for runs of in-range non-surrogate chars. */
            register const unsigned char *_q = q;
            while (_q < aligned_end) {
                unsigned long block = * (unsigned long *) _q;
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
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
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
        if (q >= e)
            goto UnexpectedEnd;
        if (!Py_UNICODE_IS_HIGH_SURROGATE(ch))
            goto IllegalEncoding;
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
#undef LONG_PTR_MASK


Py_LOCAL_INLINE(void)
STRINGLIB(utf16_encode)(unsigned short *out,
                        const STRINGLIB_CHAR *in,
                        Py_ssize_t len,
                        int native_ordering)
{
    const STRINGLIB_CHAR *end = in + len;
#if STRINGLIB_SIZEOF_CHAR == 1
# define SWAB2(CH)  ((CH) << 8)
#else
# define SWAB2(CH)  (((CH) << 8) | ((CH) >> 8))
#endif
#if STRINGLIB_MAX_CHAR < 0x10000
    if (native_ordering) {
# if STRINGLIB_SIZEOF_CHAR == 2
        Py_MEMCPY(out, in, 2 * len);
# else
        _PyUnicode_CONVERT_BYTES(STRINGLIB_CHAR, unsigned short, in, end, out);
# endif
    } else {
        const STRINGLIB_CHAR *unrolled_end = in + (len & ~ (Py_ssize_t) 3);
        while (in < unrolled_end) {
            out[0] = SWAB2(in[0]);
            out[1] = SWAB2(in[1]);
            out[2] = SWAB2(in[2]);
            out[3] = SWAB2(in[3]);
            in += 4; out += 4;
        }
        while (in < end) {
            *out++ = SWAB2(*in);
            ++in;
        }
    }
#else
    if (native_ordering) {
        while (in < end) {
            Py_UCS4 ch = *in++;
            if (ch < 0x10000)
                *out++ = ch;
            else {
                out[0] = Py_UNICODE_HIGH_SURROGATE(ch);
                out[1] = Py_UNICODE_LOW_SURROGATE(ch);
                out += 2;
            }
        }
    } else {
        while (in < end) {
            Py_UCS4 ch = *in++;
            if (ch < 0x10000)
                *out++ = SWAB2((Py_UCS2)ch);
            else {
                Py_UCS2 ch1 = Py_UNICODE_HIGH_SURROGATE(ch);
                Py_UCS2 ch2 = Py_UNICODE_LOW_SURROGATE(ch);
                out[0] = SWAB2(ch1);
                out[1] = SWAB2(ch2);
                out += 2;
            }
        }
    }
#endif
#undef SWAB2
}
#endif /* STRINGLIB_IS_UNICODE */
