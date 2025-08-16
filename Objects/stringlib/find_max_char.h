/* Finding the optimal width of unicode characters in a buffer */

/* find_max_char for one-byte will work for bytes objects as well. */
#if !STRINGLIB_IS_UNICODE && STRINGLIB_SIZEOF_CHAR > 1
# error "find_max_char.h is specific to Unicode"
#endif

#define MAX_CHAR_ASCII 0x7f
#define MAX_CHAR_UCS1  0xff
#define MAX_CHAR_UCS2  0xffff
#define MAX_CHAR_UCS4  0x10ffff

/* Mask to quickly check whether a C 'size_t' contains a
   non-ASCII, UTF8-encoded char. */

#if STRINGLIB_SIZEOF_CHAR == 1
# if SIZEOF_SIZE_T == 8
#  define MASK_ASCII 0x8080808080808080ULL
# elif SIZEOF_SIZE_T == 4
#  define MASK_ASCII 0x80808080U
# else
#  error C 'size_t' size should be either 4 or 8!
# endif
# define MASK_MAX_CHAR MASK_ASCII
# define MAX_CHAR MAX_CHAR_UCS1
#elif STRINGLIB_SIZEOF_CHAR == 2
# if SIZEOF_SIZE_T == 8
#  define MASK_ASCII 0xFF80FF80FF80FF80ULL
#  define MASK_UCS1 0xFF00FF00FF00FF00ULL
# elif SIZEOF_SIZE_T == 4
#  define MASK_ASCII 0xFF80FF80U
#  define MASK_UCS1 0xFF00FF00U
# else
#  error C 'size_t' size should be either 4 or 8!
# endif
# define MASK_MAX_CHAR MASK_UCS1
# define MAX_CHAR MAX_CHAR_UCS2
#elif STRINGLIB_SIZEOF_CHAR == 4
# if SIZEOF_SIZE_T == 8
#  define MASK_ASCII 0xFFFFFF80FFFFFF80ULL
#  define MASK_UCS1 0xFFFFFF00FFFFFF00ULL
#  define MASK_UCS2 0xFFFF0000FFFF0000ULL
# elif SIZEOF_SIZE_T == 4
#  define MASK_ASCII 0xFFFFFF80U
#  define MASK_UCS1 0xFFFFFF00U
#  define MASK_UCS2 0xFFFF0000U
# else
#  error C 'size_t' size should be either 4 or 8!
# endif
# define MASK_MAX_CHAR MASK_UCS2
# define MAX_CHAR MAX_CHAR_UCS4
#else
#error Invalid STRINGLIB_SIZEOF_CHAR (must be 1, 2 or 4)
#endif

Py_LOCAL_INLINE(Py_UCS4)
STRINGLIB(find_max_char)(const STRINGLIB_CHAR *begin, const STRINGLIB_CHAR *end)
{
    const unsigned char *p = (const unsigned char *)begin;
    const unsigned char *_end = (const unsigned char *)end;

    size_t value = 0;

    if (!_Py_IS_ALIGNED(p, ALIGNOF_SIZE_T)) {
#if STRINGLIB_SIZEOF_CHAR <= 1
        if (!_Py_IS_ALIGNED(p, 1) && p + 1 <= _end) {
            value |= *p++;
        }
#endif
#if STRINGLIB_SIZEOF_CHAR <= 2
        if (!_Py_IS_ALIGNED(p, sizeof(uint16_t)) && p + sizeof(uint16_t) <= _end) {
            value |= *(const uint16_t*)p;
            p += sizeof(uint16_t);
        }
#endif
#if SIZEOF_SIZE_T == 8
        if (!_Py_IS_ALIGNED(p, sizeof(uint32_t)) && p + sizeof(uint32_t) <= _end) {
            value |= *(const uint32_t*)p;
            p += sizeof(uint32_t);
        }
#endif
    }

    while (p + SIZEOF_SIZE_T * 32 <= _end) {
        const size_t *pp = (const size_t *)p;
        for(int i=0; i<32; i++) {
            value |= pp[i];
        }
        if (value & MASK_MAX_CHAR) {
            return MAX_CHAR;
        }
        p += SIZEOF_SIZE_T * 32;
    }

    while (p + SIZEOF_SIZE_T <= _end) {
        value |= *(const size_t *)p;
        p += SIZEOF_SIZE_T;
    }
#if SIZEOF_SIZE_T == 8
    if (p + sizeof(uint32_t) <= _end) {
        value |= *(const uint32_t*)p;
        p += sizeof(uint32_t);
    }
#endif
#if STRINGLIB_SIZEOF_CHAR <= 2
    if (p + sizeof(uint16_t) <= _end) {
        value |= *(const uint16_t*)p;
        p += sizeof(uint16_t);
    }
#endif
#if STRINGLIB_SIZEOF_CHAR <= 1
    if (p + 1 <= _end) {
        value |= *p++;
    }
#endif

#ifdef MASK_UCS2
    if (value & MASK_UCS2)
        return MAX_CHAR_UCS4;
#endif
#ifdef MASK_UCS1
    if (value & MASK_UCS1)
        return MAX_CHAR_UCS2;
#endif
    if (value & MASK_ASCII)
        return MAX_CHAR_UCS1;
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
