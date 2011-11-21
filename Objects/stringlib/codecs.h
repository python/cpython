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

Py_LOCAL_INLINE(int)
STRINGLIB(utf8_try_decode)(const char *start, const char *end,
                           STRINGLIB_CHAR *dest,
                           const char **src_pos, Py_ssize_t *dest_index)
{
    int ret;
    Py_ssize_t n;
    const char *s = start;
    const char *aligned_end = (const char *) ((size_t) end & ~LONG_PTR_MASK);
    STRINGLIB_CHAR *p = dest;

    while (s < end) {
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
                register STRINGLIB_CHAR *_p = p;
                while (_s < aligned_end) {
                    /* Read a whole long at a time (either 4 or 8 bytes),
                       and do a fast unrolled copy if it only contains ASCII
                       characters. */
                    unsigned long value = *(unsigned long *) _s;
                    if (value & ASCII_CHAR_MASK)
                        break;
                    _p[0] = _s[0];
                    _p[1] = _s[1];
                    _p[2] = _s[2];
                    _p[3] = _s[3];
#if (SIZEOF_LONG == 8)
                    _p[4] = _s[4];
                    _p[5] = _s[5];
                    _p[6] = _s[6];
                    _p[7] = _s[7];
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
        }

        if (ch < 0x80) {
            s++;
            *p++ = ch;
            continue;
        }

        n = utf8_code_length[ch];

        if (s + n > end) {
            /* unexpected end of data: the caller will decide whether
               it's an error or not */
            goto _error;
        }

        switch (n) {
        case 0:
            /* invalid start byte */
            goto _error;
        case 1:
            /* internal error */
            goto _error;
        case 2:
            if ((s[1] & 0xc0) != 0x80)
                /* invalid continuation byte */
                goto _error;
            ch = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
            assert ((ch > 0x007F) && (ch <= 0x07FF));
            s += 2;
            *p++ = ch;
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
                /* invalid continuation byte */
                goto _error;
            }
            ch = ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + (s[2] & 0x3f);
            assert ((ch > 0x07FF) && (ch <= 0xFFFF));
            s += 3;
            *p++ = ch;
            break;

        case 4:
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80 ||
                ((unsigned char)s[0] == 0xF0 &&
                 (unsigned char)s[1] < 0x90) ||
                ((unsigned char)s[0] == 0xF4 &&
                 (unsigned char)s[1] > 0x8F)) {
                /* invalid continuation byte */
                goto _error;
            }
            ch = ((s[0] & 0x7) << 18) + ((s[1] & 0x3f) << 12) +
                 ((s[2] & 0x3f) << 6) + (s[3] & 0x3f);
            assert ((ch > 0xFFFF) && (ch <= 0x10ffff));
            s += 4;
            *p++ = ch;
            break;
        }
    }
    ret = 0;
    goto _ok;
_error:
    ret = -1;
_ok:
    *src_pos = s;
    *dest_index = p - dest;
    return ret;
}

#undef LONG_PTR_MASK
#undef ASCII_CHAR_MASK

#endif /* STRINGLIB_IS_UNICODE */
