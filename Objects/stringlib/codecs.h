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

#endif /* STRINGLIB_IS_UNICODE */
