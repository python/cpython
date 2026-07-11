/* Fast unicode equal function optimized for dictobject.c and setobject.c */

/* Return 1 if two unicode objects are equal, 0 if not.
 * unicode_eq() is called when the hash of two unicode objects is equal.
 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#  include <emmintrin.h>
#  define USE_SSE2 1
#endif

static inline int
unicode_memeq(const void *p1, const void *p2, size_t len) {
    if (len == 0) return 1;

    const uint8_t *u1 = (const uint8_t *)p1;
    const uint8_t *u2 = (const uint8_t *)p2;

    switch (len) {
        case 1:
            return *u1 == *u2;
        case 2:
            return *(const uint16_t *)u1 == *(const uint16_t *)u2;
        case 3:
            return (*(const uint16_t *)u1 == *(const uint16_t *)u2) &&
                   (*(const uint16_t *)(u1 + 1) == *(const uint16_t *)(u2 + 1));
        case 4:
            return *(const uint32_t *)u1 == *(const uint32_t *)u2;
        case 5:
        case 6:
        case 7:
            return (*(const uint32_t *)u1 == *(const uint32_t *)u2) &&
                   (*(const uint32_t *)(u1 + len - 4) == *(const uint32_t *)(u2 + len - 4));
        case 8:
            return *(const uint64_t *)u1 == *(const uint64_t *)u2;
        default:
            if (len <= 16) {
                return (*(const uint64_t *)u1 == *(const uint64_t *)u2) &&
                       (*(const uint64_t *)(u1 + len - 8) == *(const uint64_t *)(u2 + len - 8));
            }
#ifdef USE_SSE2
            if (len <= 32) {
                __m128i v1_a = _mm_loadu_si128((const __m128i *)u1);
                __m128i v2_a = _mm_loadu_si128((const __m128i *)u2);
                __m128i cmp_a = _mm_cmpeq_epi8(v1_a, v2_a);
                int mask_a = _mm_movemask_epi8(cmp_a);

                __m128i v1_b = _mm_loadu_si128((const __m128i *)(u1 + len - 16));
                __m128i v2_b = _mm_loadu_si128((const __m128i *)(u2 + len - 16));
                __m128i cmp_b = _mm_cmpeq_epi8(v1_b, v2_b);
                int mask_b = _mm_movemask_epi8(cmp_b);

                return (mask_a == 0xFFFF) && (mask_b == 0xFFFF);
            }
#endif
            return memcmp(p1, p2, len) == 0;
    }
}

/* Return 1 if two unicode objects are equal, 0 if not.
 * unicode_eq() is called when the hash of two unicode objects is equal.
 */
Py_LOCAL_INLINE(int)
unicode_eq(PyObject *str1, PyObject *str2)
{
    Py_ssize_t len = PyUnicode_GET_LENGTH(str1);
    if (PyUnicode_GET_LENGTH(str2) != len) {
        return 0;
    }

    int kind = PyUnicode_KIND(str1);
    if (PyUnicode_KIND(str2) != kind) {
        return 0;
    }

    const void *data1 = PyUnicode_DATA(str1);
    const void *data2 = PyUnicode_DATA(str2);
    return unicode_memeq(data1, data2, len * kind);
}
