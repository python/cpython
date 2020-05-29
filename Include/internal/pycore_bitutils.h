/* Bit and bytes utilities.

   Bytes swap functions, reverse order of bytes:

   - _Py_bswap16(uint16_t)
   - _Py_bswap32(uint32_t)
   - _Py_bswap64(uint64_t)
*/

#ifndef Py_INTERNAL_BSWAP_H
#define Py_INTERNAL_BSWAP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#if defined(__clang__) || \
    (defined(__GNUC__) && \
     ((__GNUC__ >= 5) || (__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)))
   /* __builtin_bswap16() is available since GCC 4.8,
      __builtin_bswap32() is available since GCC 4.3,
      __builtin_bswap64() is available since GCC 4.3. */
#  define _PY_HAVE_BUILTIN_BSWAP
#endif

#ifdef _MSC_VER
   /* Get _byteswap_ushort(), _byteswap_ulong(), _byteswap_uint64() */
#  include <intrin.h>
#endif

static inline uint16_t
_Py_bswap16(uint16_t word)
{
#ifdef _PY_HAVE_BUILTIN_BSWAP
    return __builtin_bswap16(word);
#elif defined(_MSC_VER)
    Py_BUILD_ASSERT(sizeof(word) == sizeof(unsigned short));
    return _byteswap_ushort(word);
#else
    // Portable implementation which doesn't rely on circular bit shift
    return ( ((word & UINT16_C(0x00FF)) << 8)
           | ((word & UINT16_C(0xFF00)) >> 8));
#endif
}

static inline uint32_t
_Py_bswap32(uint32_t word)
{
#ifdef _PY_HAVE_BUILTIN_BSWAP
    return __builtin_bswap32(word);
#elif defined(_MSC_VER)
    Py_BUILD_ASSERT(sizeof(word) == sizeof(unsigned long));
    return _byteswap_ulong(word);
#else
    // Portable implementation which doesn't rely on circular bit shift
    return ( ((word & UINT32_C(0x000000FF)) << 24)
           | ((word & UINT32_C(0x0000FF00)) <<  8)
           | ((word & UINT32_C(0x00FF0000)) >>  8)
           | ((word & UINT32_C(0xFF000000)) >> 24));
#endif
}

static inline uint64_t
_Py_bswap64(uint64_t word)
{
#ifdef _PY_HAVE_BUILTIN_BSWAP
    return __builtin_bswap64(word);
#elif defined(_MSC_VER)
    return _byteswap_uint64(word);
#else
    // Portable implementation which doesn't rely on circular bit shift
    return ( ((word & UINT64_C(0x00000000000000FF)) << 56)
           | ((word & UINT64_C(0x000000000000FF00)) << 40)
           | ((word & UINT64_C(0x0000000000FF0000)) << 24)
           | ((word & UINT64_C(0x00000000FF000000)) <<  8)
           | ((word & UINT64_C(0x000000FF00000000)) >>  8)
           | ((word & UINT64_C(0x0000FF0000000000)) >> 24)
           | ((word & UINT64_C(0x00FF000000000000)) >> 40)
           | ((word & UINT64_C(0xFF00000000000000)) >> 56));
#endif
}


// Population count: count the number of 1's in 'u'
// (number of bits set to 1)
static inline int
_Py_popcount32(uint32_t u)
{
#if defined(__clang__) || defined(__GNUC__)

#if SIZEOF_INT == 4
    return __builtin_popcount(u);
#elif SIZEOF_LONG == 4
    return __builtin_popcountl(u);
#else
#  error "unsupported configuration"
#endif

#else
    /* 32-bit SWAR (SIMD Within A Register) popcount */
    u -= (u >> 1) & UINT32_C(0x55555555);
    u = (u & UINT32_C(0x33333333)) + ((u >> 2) & UINT32_C(0x33333333));
    u = (u + (u >> 4)) & UINT32_C(0x0f0f0f0f);
    return (uint32_t)(u * UINT32_C(0x01010101)) >> 24;
#endif
}


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BSWAP_H */

