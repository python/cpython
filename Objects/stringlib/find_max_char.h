/* Finding the optimal width of unicode characters in a buffer */

/* find_max_char for one-byte will work for bytes objects as well. */
#if !STRINGLIB_IS_UNICODE && STRINGLIB_SIZEOF_CHAR > 1
# error "find_max_char.h is specific to Unicode"
#endif

#define MAX_CHAR_ASCII 0x7f
#define MAX_CHAR_UCS1  0xff
#define MAX_CHAR_UCS2  0xffff
#define MAX_CHAR_UCS4  0x10ffff

/* Masks selecting the bits that are set in a C 'size_t' holding several
   characters when at least one of them is wider than ASCII, UCS1 or UCS2. */
#if STRINGLIB_SIZEOF_CHAR == 1
#  if SIZEOF_SIZE_T == 8
#    define MASK_ASCII 0x8080808080808080ULL
#  elif SIZEOF_SIZE_T == 4
#    define MASK_ASCII 0x80808080U
#  else
#    error C 'size_t' size should be either 4 or 8!
#  endif
#  define MASK_MAX_CHAR MASK_ASCII
#  define MAX_CHAR MAX_CHAR_UCS1
#elif STRINGLIB_SIZEOF_CHAR == 2
#  if SIZEOF_SIZE_T == 8
#    define MASK_ASCII 0xFF80FF80FF80FF80ULL
#    define MASK_UCS1 0xFF00FF00FF00FF00ULL
#  elif SIZEOF_SIZE_T == 4
#    define MASK_ASCII 0xFF80FF80U
#    define MASK_UCS1 0xFF00FF00U
#  else
#    error C 'size_t' size should be either 4 or 8!
#  endif
#  define MASK_MAX_CHAR MASK_UCS1
#  define MAX_CHAR MAX_CHAR_UCS2
#elif STRINGLIB_SIZEOF_CHAR == 4
#  if SIZEOF_SIZE_T == 8
#    define MASK_ASCII 0xFFFFFF80FFFFFF80ULL
#    define MASK_UCS1 0xFFFFFF00FFFFFF00ULL
#    define MASK_UCS2 0xFFFF0000FFFF0000ULL
#  elif SIZEOF_SIZE_T == 4
#    define MASK_ASCII 0xFFFFFF80U
#    define MASK_UCS1 0xFFFFFF00U
#    define MASK_UCS2 0xFFFF0000U
#  else
#    error C 'size_t' size should be either 4 or 8!
#  endif
#  define MASK_MAX_CHAR MASK_UCS2
#  define MAX_CHAR MAX_CHAR_UCS4
#else
#  error Invalid STRINGLIB_SIZEOF_CHAR (must be 1, 2 or 4)
#endif

/* OR all characters into a single size_t accumulator and test the high bits
   at the end, instead of comparing each character. The accumulation loop has
   no branches, so compilers vectorize it. The words are loaded with memcpy,
   so the data does not need to be aligned, and the tail re-reads the last
   word of the buffer, which may overlap data already seen. Every 16 words,
   check whether the widest kind is already known and exit early. */
Py_LOCAL_INLINE(Py_UCS4)
STRINGLIB(find_max_char)(const STRINGLIB_CHAR *begin, const STRINGLIB_CHAR *end)
{
    const unsigned char *p = (const unsigned char *)begin;
    const unsigned char *_end = (const unsigned char *)end;
    size_t value = 0;

    while (p + SIZEOF_SIZE_T * 16 <= _end) {
        /* Four independent accumulators so the loop does not serialize on
           one register when the compiler does not vectorize it. */
        size_t v0 = 0, v1 = 0, v2 = 0, v3 = 0;
        for (int i = 0; i < 16 * SIZEOF_SIZE_T; i += 4 * SIZEOF_SIZE_T) {
            size_t w0, w1, w2, w3;
            memcpy(&w0, p + i, SIZEOF_SIZE_T);
            memcpy(&w1, p + i + SIZEOF_SIZE_T, SIZEOF_SIZE_T);
            memcpy(&w2, p + i + 2*SIZEOF_SIZE_T, SIZEOF_SIZE_T);
            memcpy(&w3, p + i + 3*SIZEOF_SIZE_T, SIZEOF_SIZE_T);
            v0 |= w0;
            v1 |= w1;
            v2 |= w2;
            v3 |= w3;
        }
        value |= v0 | v1 | v2 | v3;
        if (value & MASK_MAX_CHAR) {
            return MAX_CHAR;
        }
        p += SIZEOF_SIZE_T * 16;
    }

    while (p + SIZEOF_SIZE_T <= _end) {
        size_t a;
        memcpy(&a, p, SIZEOF_SIZE_T);
        value |= a;
        p += SIZEOF_SIZE_T;
    }

    if (p < _end) {
        size_t n = (size_t)(_end - (const unsigned char *)begin);
        if (n >= SIZEOF_SIZE_T) {
            size_t a;
            memcpy(&a, _end - SIZEOF_SIZE_T, SIZEOF_SIZE_T);
            value |= a;
        }
#if SIZEOF_SIZE_T == 8
        else if (n >= 4) {
            uint32_t a, b;
            memcpy(&a, p, 4);
            memcpy(&b, _end - 4, 4);
            value |= a | b;
        }
#endif
#if STRINGLIB_SIZEOF_CHAR <= 2
        else if (n >= 2) {
            uint16_t a, b;
            memcpy(&a, p, 2);
            memcpy(&b, _end - 2, 2);
            value |= a | b;
        }
#endif
#if STRINGLIB_SIZEOF_CHAR == 1
        else {
            value |= *p;
        }
#endif
    }

#ifdef MASK_UCS2
    if (value & MASK_UCS2) {
        return MAX_CHAR_UCS4;
    }
#endif
#ifdef MASK_UCS1
    if (value & MASK_UCS1) {
        return MAX_CHAR_UCS2;
    }
#endif
    if (value & MASK_ASCII) {
        return MAX_CHAR_UCS1;
    }
    return MAX_CHAR_ASCII;
}

#undef MASK_MAX_CHAR
#undef MAX_CHAR
#undef MASK_ASCII
#undef MASK_UCS1
#undef MASK_UCS2
#undef MAX_CHAR_ASCII
#undef MAX_CHAR_UCS1
#undef MAX_CHAR_UCS2
#undef MAX_CHAR_UCS4
