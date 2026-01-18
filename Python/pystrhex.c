/* Format bytes as hexadecimal */

#include "Python.h"
#include "pycore_strhex.h"        // _Py_strhex_with_sep()
#include "pycore_unicodeobject.h" // _PyUnicode_CheckConsistency()

/* SIMD optimization for hexlify.
   Only available on x86-64 with GCC/Clang. */
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#  define PY_HEXLIFY_CAN_COMPILE_SIMD 1
#  include <cpuid.h>
#  include <immintrin.h>
#else
#  define PY_HEXLIFY_CAN_COMPILE_SIMD 0
#endif

#if PY_HEXLIFY_CAN_COMPILE_SIMD

/* Runtime CPU feature detection (lazy initialization)
   -1 = not checked, 0 = no SIMD, 1 = AVX2, 2 = AVX-512 */
static int _Py_hexlify_simd_level = -1;

#define PY_HEXLIFY_SIMD_NONE   0
#define PY_HEXLIFY_SIMD_AVX2   1
#define PY_HEXLIFY_SIMD_AVX512 2

static void
_Py_hexlify_detect_cpu_features(void)
{
    unsigned int eax, ebx, ecx, edx;

    _Py_hexlify_simd_level = PY_HEXLIFY_SIMD_NONE;

    if (!__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        return;
    }

    /* Check for AVX2: CPUID.7H:EBX bit 5 */
    int has_avx2 = (ebx & (1 << 5)) != 0;

    /* Check for AVX-512F + AVX-512BW + AVX-512VBMI:
       CPUID.7H:EBX bits 16 and 30, ECX bit 1 */
    int has_avx512f = (ebx & (1 << 16)) != 0;
    int has_avx512bw = (ebx & (1 << 30)) != 0;
    int has_avx512vbmi = (ecx & (1 << 1)) != 0;

    if (has_avx512f && has_avx512bw && has_avx512vbmi) {
        _Py_hexlify_simd_level = PY_HEXLIFY_SIMD_AVX512;
    } else if (has_avx2) {
        _Py_hexlify_simd_level = PY_HEXLIFY_SIMD_AVX2;
    }
}

static inline int
_Py_hexlify_get_simd_level(void)
{
    if (_Py_hexlify_simd_level < 0) {
        _Py_hexlify_detect_cpu_features();
    }
    return _Py_hexlify_simd_level;
}

/* AVX2-accelerated hexlify: converts 32 bytes to 64 hex chars per iteration.
   Uses arithmetic nibble-to-hex conversion instead of table lookup. */
__attribute__((target("avx2")))
static void
_Py_hexlify_avx2(const unsigned char *src, Py_UCS1 *dst, Py_ssize_t len)
{
    const __m256i mask_0f = _mm256_set1_epi8(0x0f);
    const __m256i ascii_0 = _mm256_set1_epi8('0');
    const __m256i offset = _mm256_set1_epi8('a' - '0' - 10);  /* 0x27 */
    const __m256i nine = _mm256_set1_epi8(9);

    Py_ssize_t i = 0;

    /* Process 32 bytes at a time */
    for (; i + 32 <= len; i += 32, dst += 64) {
        /* Load 32 input bytes */
        __m256i data = _mm256_loadu_si256((const __m256i *)(src + i));

        /* Extract high and low nibbles */
        __m256i hi = _mm256_and_si256(_mm256_srli_epi16(data, 4), mask_0f);
        __m256i lo = _mm256_and_si256(data, mask_0f);

        /* Convert nibbles to hex: add '0', then add 0x27 where nibble > 9 */
        __m256i hi_gt9 = _mm256_cmpgt_epi8(hi, nine);
        __m256i lo_gt9 = _mm256_cmpgt_epi8(lo, nine);

        hi = _mm256_add_epi8(hi, ascii_0);
        lo = _mm256_add_epi8(lo, ascii_0);
        hi = _mm256_add_epi8(hi, _mm256_and_si256(hi_gt9, offset));
        lo = _mm256_add_epi8(lo, _mm256_and_si256(lo_gt9, offset));

        /* Interleave hi/lo nibbles to get correct output order.
           unpacklo/hi work within 128-bit lanes, so we need permute to fix. */
        __m256i mixed_lo = _mm256_unpacklo_epi8(hi, lo);
        __m256i mixed_hi = _mm256_unpackhi_epi8(hi, lo);

        /* Fix cross-lane ordering */
        __m256i result0 = _mm256_permute2x128_si256(mixed_lo, mixed_hi, 0x20);
        __m256i result1 = _mm256_permute2x128_si256(mixed_lo, mixed_hi, 0x31);

        /* Store 64 hex characters */
        _mm256_storeu_si256((__m256i *)dst, result0);
        _mm256_storeu_si256((__m256i *)(dst + 32), result1);
    }

    /* Scalar fallback for remaining 0-31 bytes */
    for (; i < len; i++, dst += 2) {
        unsigned int c = src[i];
        unsigned int hi = c >> 4;
        unsigned int lo = c & 0x0f;
        dst[0] = (Py_UCS1)(hi + '0' + (hi > 9) * ('a' - '0' - 10));
        dst[1] = (Py_UCS1)(lo + '0' + (lo > 9) * ('a' - '0' - 10));
    }
}

/* AVX-512 accelerated hexlify: converts 64 bytes to 128 hex chars per iteration.
   Requires AVX-512F, AVX-512BW, and AVX-512VBMI for byte-level permutation. */
__attribute__((target("avx512f,avx512bw,avx512vbmi")))
static void
_Py_hexlify_avx512(const unsigned char *src, Py_UCS1 *dst, Py_ssize_t len)
{
    const __m512i mask_0f = _mm512_set1_epi8(0x0f);
    const __m512i ascii_0 = _mm512_set1_epi8('0');
    const __m512i ascii_a = _mm512_set1_epi8('a' - 10);
    const __m512i nine = _mm512_set1_epi8(9);

    /* Permutation indices for interleaving hi/lo nibbles.
       We need to transform:
         hi: H0 H1 H2 ... H63
         lo: L0 L1 L2 ... L63
       into:
         out0: H0 L0 H1 L1 ... H31 L31
         out1: H32 L32 H33 L33 ... H63 L63
    */
    const __m512i interleave_lo = _mm512_set_epi8(
        39, 103, 38, 102, 37, 101, 36, 100, 35, 99, 34, 98, 33, 97, 32, 96,
        47, 111, 46, 110, 45, 109, 44, 108, 43, 107, 42, 106, 41, 105, 40, 104,
        55, 119, 54, 118, 53, 117, 52, 116, 51, 115, 50, 114, 49, 113, 48, 112,
        63, 127, 62, 126, 61, 125, 60, 124, 59, 123, 58, 122, 57, 121, 56, 120
    );
    const __m512i interleave_hi = _mm512_set_epi8(
        7, 71, 6, 70, 5, 69, 4, 68, 3, 67, 2, 66, 1, 65, 0, 64,
        15, 79, 14, 78, 13, 77, 12, 76, 11, 75, 10, 74, 9, 73, 8, 72,
        23, 87, 22, 86, 21, 85, 20, 84, 19, 83, 18, 82, 17, 81, 16, 80,
        31, 95, 30, 94, 29, 93, 28, 92, 27, 91, 26, 90, 25, 89, 24, 88
    );

    Py_ssize_t i = 0;

    /* Process 64 bytes at a time */
    for (; i + 64 <= len; i += 64, dst += 128) {
        /* Load 64 input bytes */
        __m512i data = _mm512_loadu_si512((const __m512i *)(src + i));

        /* Extract high and low nibbles */
        __m512i hi = _mm512_and_si512(_mm512_srli_epi16(data, 4), mask_0f);
        __m512i lo = _mm512_and_si512(data, mask_0f);

        /* Convert nibbles to hex using masked blend:
           if nibble > 9: use 'a' + (nibble - 10) = nibble + ('a' - 10)
           else: use '0' + nibble */
        __mmask64 hi_alpha = _mm512_cmpgt_epi8_mask(hi, nine);
        __mmask64 lo_alpha = _mm512_cmpgt_epi8_mask(lo, nine);

        __m512i hi_digit = _mm512_add_epi8(hi, ascii_0);
        __m512i hi_letter = _mm512_add_epi8(hi, ascii_a);
        hi = _mm512_mask_blend_epi8(hi_alpha, hi_digit, hi_letter);

        __m512i lo_digit = _mm512_add_epi8(lo, ascii_0);
        __m512i lo_letter = _mm512_add_epi8(lo, ascii_a);
        lo = _mm512_mask_blend_epi8(lo_alpha, lo_digit, lo_letter);

        /* Interleave hi/lo to get correct output order using permutex2var */
        __m512i result0 = _mm512_permutex2var_epi8(hi, interleave_hi, lo);
        __m512i result1 = _mm512_permutex2var_epi8(hi, interleave_lo, lo);

        /* Store 128 hex characters */
        _mm512_storeu_si512((__m512i *)dst, result0);
        _mm512_storeu_si512((__m512i *)(dst + 64), result1);
    }

    /* Use AVX2 for remaining 32-63 bytes */
    if (i + 32 <= len) {
        const __m256i mask_0f_256 = _mm256_set1_epi8(0x0f);
        const __m256i ascii_0_256 = _mm256_set1_epi8('0');
        const __m256i offset_256 = _mm256_set1_epi8('a' - '0' - 10);
        const __m256i nine_256 = _mm256_set1_epi8(9);

        __m256i data = _mm256_loadu_si256((const __m256i *)(src + i));
        __m256i hi = _mm256_and_si256(_mm256_srli_epi16(data, 4), mask_0f_256);
        __m256i lo = _mm256_and_si256(data, mask_0f_256);

        __m256i hi_gt9 = _mm256_cmpgt_epi8(hi, nine_256);
        __m256i lo_gt9 = _mm256_cmpgt_epi8(lo, nine_256);

        hi = _mm256_add_epi8(hi, ascii_0_256);
        lo = _mm256_add_epi8(lo, ascii_0_256);
        hi = _mm256_add_epi8(hi, _mm256_and_si256(hi_gt9, offset_256));
        lo = _mm256_add_epi8(lo, _mm256_and_si256(lo_gt9, offset_256));

        __m256i mixed_lo = _mm256_unpacklo_epi8(hi, lo);
        __m256i mixed_hi = _mm256_unpackhi_epi8(hi, lo);

        __m256i r0 = _mm256_permute2x128_si256(mixed_lo, mixed_hi, 0x20);
        __m256i r1 = _mm256_permute2x128_si256(mixed_lo, mixed_hi, 0x31);

        _mm256_storeu_si256((__m256i *)dst, r0);
        _mm256_storeu_si256((__m256i *)(dst + 32), r1);

        i += 32;
        dst += 64;
    }

    /* Scalar fallback for remaining 0-31 bytes */
    for (; i < len; i++, dst += 2) {
        unsigned int c = src[i];
        unsigned int hi = c >> 4;
        unsigned int lo = c & 0x0f;
        dst[0] = (Py_UCS1)(hi + '0' + (hi > 9) * ('a' - '0' - 10));
        dst[1] = (Py_UCS1)(lo + '0' + (lo > 9) * ('a' - '0' - 10));
    }
}

#endif /* PY_HEXLIFY_CAN_COMPILE_SIMD */

static PyObject *_Py_strhex_impl(const char* argbuf, const Py_ssize_t arglen,
                                 PyObject* sep, int bytes_per_sep_group,
                                 const int return_bytes)
{
    assert(arglen >= 0);

    Py_UCS1 sep_char = 0;
    if (sep) {
        Py_ssize_t seplen = PyObject_Length((PyObject*)sep);
        if (seplen < 0) {
            return NULL;
        }
        if (seplen != 1) {
            PyErr_SetString(PyExc_ValueError, "sep must be length 1.");
            return NULL;
        }
        if (PyUnicode_Check(sep)) {
            if (PyUnicode_KIND(sep) != PyUnicode_1BYTE_KIND) {
                PyErr_SetString(PyExc_ValueError, "sep must be ASCII.");
                return NULL;
            }
            sep_char = PyUnicode_READ_CHAR(sep, 0);
        }
        else if (PyBytes_Check(sep)) {
            sep_char = PyBytes_AS_STRING(sep)[0];
        }
        else {
            PyErr_SetString(PyExc_TypeError, "sep must be str or bytes.");
            return NULL;
        }
        if (sep_char > 127 && !return_bytes) {
            PyErr_SetString(PyExc_ValueError, "sep must be ASCII.");
            return NULL;
        }
    }
    else {
        bytes_per_sep_group = 0;
    }
    unsigned int abs_bytes_per_sep = _Py_ABS_CAST(unsigned int, bytes_per_sep_group);
    Py_ssize_t resultlen = 0;
    if (bytes_per_sep_group && arglen > 0) {
        /* How many sep characters we'll be inserting. */
        resultlen = (arglen - 1) / abs_bytes_per_sep;
    }
    /* Bounds checking for our Py_ssize_t indices. */
    if (arglen >= PY_SSIZE_T_MAX / 2 - resultlen) {
        return PyErr_NoMemory();
    }
    resultlen += arglen * 2;

    if ((size_t)abs_bytes_per_sep >= (size_t)arglen) {
        bytes_per_sep_group = 0;
        abs_bytes_per_sep = 0;
    }

    PyObject *retval;
    Py_UCS1 *retbuf;
    if (return_bytes) {
        /* If _PyBytes_FromSize() were public we could avoid malloc+copy. */
        retval = PyBytes_FromStringAndSize(NULL, resultlen);
        if (!retval) {
            return NULL;
        }
        retbuf = (Py_UCS1 *)PyBytes_AS_STRING(retval);
    }
    else {
        retval = PyUnicode_New(resultlen, 127);
        if (!retval) {
            return NULL;
        }
        retbuf = PyUnicode_1BYTE_DATA(retval);
    }

    /* Hexlify */
    Py_ssize_t i, j;
    unsigned char c;

    if (bytes_per_sep_group == 0) {
#if PY_HEXLIFY_CAN_COMPILE_SIMD
        int simd_level = _Py_hexlify_get_simd_level();
        /* Use AVX-512 for inputs >= 64 bytes, AVX2 for >= 32 bytes */
        if (arglen >= 64 && simd_level >= PY_HEXLIFY_SIMD_AVX512) {
            _Py_hexlify_avx512((const unsigned char *)argbuf, retbuf, arglen);
        }
        else if (arglen >= 32 && simd_level >= PY_HEXLIFY_SIMD_AVX2) {
            _Py_hexlify_avx2((const unsigned char *)argbuf, retbuf, arglen);
        }
        else
#endif
        {
            for (i = j = 0; i < arglen; ++i) {
                c = argbuf[i];
                retbuf[j++] = Py_hexdigits[c >> 4];
                retbuf[j++] = Py_hexdigits[c & 0x0f];
            }
        }
    }
    else {
        /* The number of complete chunk+sep periods */
        Py_ssize_t chunks = (arglen - 1) / abs_bytes_per_sep;
        Py_ssize_t chunk;
        unsigned int k;

        if (bytes_per_sep_group < 0) {
            i = j = 0;
            for (chunk = 0; chunk < chunks; chunk++) {
                for (k = 0; k < abs_bytes_per_sep; k++) {
                    c = argbuf[i++];
                    retbuf[j++] = Py_hexdigits[c >> 4];
                    retbuf[j++] = Py_hexdigits[c & 0x0f];
                }
                retbuf[j++] = sep_char;
            }
            while (i < arglen) {
                c = argbuf[i++];
                retbuf[j++] = Py_hexdigits[c >> 4];
                retbuf[j++] = Py_hexdigits[c & 0x0f];
            }
            assert(j == resultlen);
        }
        else {
            i = arglen - 1;
            j = resultlen - 1;
            for (chunk = 0; chunk < chunks; chunk++) {
                for (k = 0; k < abs_bytes_per_sep; k++) {
                    c = argbuf[i--];
                    retbuf[j--] = Py_hexdigits[c & 0x0f];
                    retbuf[j--] = Py_hexdigits[c >> 4];
                }
                retbuf[j--] = sep_char;
            }
            while (i >= 0) {
                c = argbuf[i--];
                retbuf[j--] = Py_hexdigits[c & 0x0f];
                retbuf[j--] = Py_hexdigits[c >> 4];
            }
            assert(j == -1);
        }
    }

#ifdef Py_DEBUG
    if (!return_bytes) {
        assert(_PyUnicode_CheckConsistency(retval, 1));
    }
#endif

    return retval;
}

PyObject * _Py_strhex(const char* argbuf, const Py_ssize_t arglen)
{
    return _Py_strhex_impl(argbuf, arglen, NULL, 0, 0);
}

/* Same as above but returns a bytes() instead of str() to avoid the
 * need to decode the str() when bytes are needed. */
PyObject* _Py_strhex_bytes(const char* argbuf, const Py_ssize_t arglen)
{
    return _Py_strhex_impl(argbuf, arglen, NULL, 0, 1);
}

/* These variants include support for a separator between every N bytes: */

PyObject* _Py_strhex_with_sep(const char* argbuf, const Py_ssize_t arglen,
                              PyObject* sep, const int bytes_per_group)
{
    return _Py_strhex_impl(argbuf, arglen, sep, bytes_per_group, 0);
}

/* Same as above but returns a bytes() instead of str() to avoid the
 * need to decode the str() when bytes are needed. */
PyObject* _Py_strhex_bytes_with_sep(const char* argbuf, const Py_ssize_t arglen,
                                    PyObject* sep, const int bytes_per_group)
{
    return _Py_strhex_impl(argbuf, arglen, sep, bytes_per_group, 1);
}
