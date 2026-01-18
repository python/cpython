/* Format bytes as hexadecimal */

#include "Python.h"
#include "pycore_strhex.h"        // _Py_strhex_with_sep()
#include "pycore_unicodeobject.h" // _PyUnicode_CheckConsistency()

/* AVX2 SIMD optimization for hexlify.
   Only available on x86-64 with GCC/Clang. */
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#  define PY_HEXLIFY_CAN_COMPILE_AVX2 1
#  include <cpuid.h>
#  include <immintrin.h>
#else
#  define PY_HEXLIFY_CAN_COMPILE_AVX2 0
#endif

#if PY_HEXLIFY_CAN_COMPILE_AVX2

/* Runtime CPU feature detection (lazy initialization) */
static int _Py_hexlify_avx2_available = -1;  /* -1 = not checked yet */

static void
_Py_hexlify_detect_cpu_features(void)
{
    unsigned int eax, ebx, ecx, edx;

    /* Check for AVX2 support: CPUID.7H:EBX bit 5 */
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        _Py_hexlify_avx2_available = (ebx & (1 << 5)) != 0;
    } else {
        _Py_hexlify_avx2_available = 0;
    }
}

static inline int
_Py_hexlify_can_use_avx2(void)
{
    if (_Py_hexlify_avx2_available < 0) {
        _Py_hexlify_detect_cpu_features();
    }
    return _Py_hexlify_avx2_available;
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

#endif /* PY_HEXLIFY_CAN_COMPILE_AVX2 */

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
#if PY_HEXLIFY_CAN_COMPILE_AVX2
        /* Use AVX2 for inputs >= 32 bytes when available */
        if (arglen >= 32 && _Py_hexlify_can_use_avx2()) {
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
