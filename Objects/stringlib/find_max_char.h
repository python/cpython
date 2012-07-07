/* Finding the optimal width of unicode characters in a buffer */

#if STRINGLIB_IS_UNICODE

/* Mask to check or force alignment of a pointer to C 'long' boundaries */
#define LONG_PTR_MASK (size_t) (SIZEOF_LONG - 1)

/* Mask to quickly check whether a C 'long' contains a
   non-ASCII, UTF8-encoded char. */
#if (SIZEOF_LONG == 8)
# define UCS1_ASCII_CHAR_MASK 0x8080808080808080UL
#elif (SIZEOF_LONG == 4)
# define UCS1_ASCII_CHAR_MASK 0x80808080UL
#else
# error C 'long' size should be either 4 or 8!
#endif

#if STRINGLIB_SIZEOF_CHAR == 1

Py_LOCAL_INLINE(Py_UCS4)
STRINGLIB(find_max_char)(const STRINGLIB_CHAR *begin, const STRINGLIB_CHAR *end)
{
    const unsigned char *p = (const unsigned char *) begin;
    const unsigned char *aligned_end = (const unsigned char *) ((size_t) end & ~LONG_PTR_MASK);

    while (p < end) {
        if (!((size_t) p & LONG_PTR_MASK)) {
            /* Help register allocation */
            register const unsigned char *_p = p;
            while (_p < aligned_end) {
                unsigned long value = *(unsigned long *) _p;
                if (value & UCS1_ASCII_CHAR_MASK)
                    return 255;
                _p += SIZEOF_LONG;
            }
            p = _p;
            if (p == end)
                break;
        }
        if (*p++ & 0x80)
            return 255;
    }
    return 127;
}

#undef LONG_PTR_MASK
#undef ASCII_CHAR_MASK

#else /* STRINGLIB_SIZEOF_CHAR == 1 */

#define MASK_ASCII 0xFFFFFF80
#define MASK_UCS1 0xFFFFFF00
#define MASK_UCS2 0xFFFF0000

#define MAX_CHAR_ASCII 0x7f
#define MAX_CHAR_UCS1  0xff
#define MAX_CHAR_UCS2  0xffff
#define MAX_CHAR_UCS4  0x10ffff

Py_LOCAL_INLINE(Py_UCS4)
STRINGLIB(find_max_char)(const STRINGLIB_CHAR *begin, const STRINGLIB_CHAR *end)
{
#if STRINGLIB_SIZEOF_CHAR == 2
    const Py_UCS4 mask_limit = MASK_UCS1;
    const Py_UCS4 max_char_limit = MAX_CHAR_UCS2;
#elif STRINGLIB_SIZEOF_CHAR == 4
    const Py_UCS4 mask_limit = MASK_UCS2;
    const Py_UCS4 max_char_limit = MAX_CHAR_UCS4;
#else
#error Invalid STRINGLIB_SIZEOF_CHAR (must be 1, 2 or 4)
#endif
    register Py_UCS4 mask;
    Py_ssize_t n = end - begin;
    const STRINGLIB_CHAR *p = begin;
    const STRINGLIB_CHAR *unrolled_end = begin + (n & ~ (Py_ssize_t) 3);
    Py_UCS4 max_char;

    max_char = MAX_CHAR_ASCII;
    mask = MASK_ASCII;
    while (p < unrolled_end) {
        STRINGLIB_CHAR bits = p[0] | p[1] | p[2] | p[3];
        if (bits & mask) {
            if (mask == mask_limit) {
                /* Limit reached */
                return max_char_limit;
            }
            if (mask == MASK_ASCII) {
                max_char = MAX_CHAR_UCS1;
                mask = MASK_UCS1;
            }
            else {
                /* mask can't be MASK_UCS2 because of mask_limit above */
                assert(mask == MASK_UCS1);
                max_char = MAX_CHAR_UCS2;
                mask = MASK_UCS2;
            }
            /* We check the new mask on the same chars in the next iteration */
            continue;
        }
        p += 4;
    }
    while (p < end) {
        if (p[0] & mask) {
            if (mask == mask_limit) {
                /* Limit reached */
                return max_char_limit;
            }
            if (mask == MASK_ASCII) {
                max_char = MAX_CHAR_UCS1;
                mask = MASK_UCS1;
            }
            else {
                /* mask can't be MASK_UCS2 because of mask_limit above */
                assert(mask == MASK_UCS1);
                max_char = MAX_CHAR_UCS2;
                mask = MASK_UCS2;
            }
            /* We check the new mask on the same chars in the next iteration */
            continue;
        }
        p++;
    }
    return max_char;
}

#undef MASK_ASCII
#undef MASK_UCS1
#undef MASK_UCS2
#undef MAX_CHAR_ASCII
#undef MAX_CHAR_UCS1
#undef MAX_CHAR_UCS2
#undef MAX_CHAR_UCS4

#endif /* STRINGLIB_SIZEOF_CHAR == 1 */
#endif /* STRINGLIB_IS_UNICODE */

