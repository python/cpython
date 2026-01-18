/* Format bytes as hexadecimal */

#include "Python.h"
#include "pycore_strhex.h"        // _Py_strhex_with_sep()
#include "pycore_unicodeobject.h" // _PyUnicode_CheckConsistency()

/* SIMD optimization for hexlify.
   x86-64: SSE2 (always available, part of x86-64 baseline)
   ARM64: NEON (always available on AArch64) */
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#  define PY_HEXLIFY_CAN_COMPILE_SSE2 1
#  include <emmintrin.h>
#else
#  define PY_HEXLIFY_CAN_COMPILE_SSE2 0
#endif

#if defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
#  define PY_HEXLIFY_CAN_COMPILE_NEON 1
#  include <arm_neon.h>
#else
#  define PY_HEXLIFY_CAN_COMPILE_NEON 0
#endif

#if PY_HEXLIFY_CAN_COMPILE_SSE2

/* SSE2-accelerated hexlify: converts 16 bytes to 32 hex chars per iteration.
   SSE2 is always available on x86-64 (part of AMD64 baseline). */
static void
_Py_hexlify_sse2(const unsigned char *src, Py_UCS1 *dst, Py_ssize_t len)
{
    const __m128i mask_0f = _mm_set1_epi8(0x0f);
    const __m128i ascii_0 = _mm_set1_epi8('0');
    const __m128i offset = _mm_set1_epi8('a' - '0' - 10);  /* 0x27 */
    const __m128i nine = _mm_set1_epi8(9);

    Py_ssize_t i = 0;

    /* Process 16 bytes at a time */
    for (; i + 16 <= len; i += 16, dst += 32) {
        /* Load 16 input bytes */
        __m128i data = _mm_loadu_si128((const __m128i *)(src + i));

        /* Extract high and low nibbles */
        __m128i hi = _mm_and_si128(_mm_srli_epi16(data, 4), mask_0f);
        __m128i lo = _mm_and_si128(data, mask_0f);

        /* Convert nibbles to hex: add '0', then add 0x27 where nibble > 9 */
        __m128i hi_gt9 = _mm_cmpgt_epi8(hi, nine);
        __m128i lo_gt9 = _mm_cmpgt_epi8(lo, nine);

        hi = _mm_add_epi8(hi, ascii_0);
        lo = _mm_add_epi8(lo, ascii_0);
        hi = _mm_add_epi8(hi, _mm_and_si128(hi_gt9, offset));
        lo = _mm_add_epi8(lo, _mm_and_si128(lo_gt9, offset));

        /* Interleave hi/lo nibbles to get correct output order */
        __m128i result0 = _mm_unpacklo_epi8(hi, lo);  /* First 16 hex chars */
        __m128i result1 = _mm_unpackhi_epi8(hi, lo);  /* Second 16 hex chars */

        /* Store 32 hex characters */
        _mm_storeu_si128((__m128i *)dst, result0);
        _mm_storeu_si128((__m128i *)(dst + 16), result1);
    }

    /* Scalar fallback for remaining 0-15 bytes */
    for (; i < len; i++, dst += 2) {
        unsigned int c = src[i];
        unsigned int hi = c >> 4;
        unsigned int lo = c & 0x0f;
        dst[0] = (Py_UCS1)(hi + '0' + (hi > 9) * ('a' - '0' - 10));
        dst[1] = (Py_UCS1)(lo + '0' + (lo > 9) * ('a' - '0' - 10));
    }
}

#endif /* PY_HEXLIFY_CAN_COMPILE_SSE2 */

#if PY_HEXLIFY_CAN_COMPILE_NEON

/* ARM NEON accelerated hexlify: converts 16 bytes to 32 hex chars per iteration.
   NEON is always available on AArch64, no runtime detection needed. */
static void
_Py_hexlify_neon(const unsigned char *src, Py_UCS1 *dst, Py_ssize_t len)
{
    const uint8x16_t mask_0f = vdupq_n_u8(0x0f);
    const uint8x16_t ascii_0 = vdupq_n_u8('0');
    const uint8x16_t offset = vdupq_n_u8('a' - '0' - 10);  /* 0x27 */
    const uint8x16_t nine = vdupq_n_u8(9);

    Py_ssize_t i = 0;

    /* Process 16 bytes at a time */
    for (; i + 16 <= len; i += 16, dst += 32) {
        /* Load 16 input bytes */
        uint8x16_t data = vld1q_u8(src + i);

        /* Extract high and low nibbles */
        uint8x16_t hi = vandq_u8(vshrq_n_u8(data, 4), mask_0f);
        uint8x16_t lo = vandq_u8(data, mask_0f);

        /* Convert nibbles to hex: add '0', then add 0x27 where nibble > 9 */
        uint8x16_t hi_gt9 = vcgtq_u8(hi, nine);
        uint8x16_t lo_gt9 = vcgtq_u8(lo, nine);

        hi = vaddq_u8(hi, ascii_0);
        lo = vaddq_u8(lo, ascii_0);
        hi = vaddq_u8(hi, vandq_u8(hi_gt9, offset));
        lo = vaddq_u8(lo, vandq_u8(lo_gt9, offset));

        /* Interleave hi/lo nibbles to get correct output order.
           vzip1/vzip2 interleave the low/high halves respectively. */
        uint8x16_t result0 = vzip1q_u8(hi, lo);  /* First 16 hex chars */
        uint8x16_t result1 = vzip2q_u8(hi, lo);  /* Second 16 hex chars */

        /* Store 32 hex characters */
        vst1q_u8(dst, result0);
        vst1q_u8(dst + 16, result1);
    }

    /* Scalar fallback for remaining 0-15 bytes */
    for (; i < len; i++, dst += 2) {
        unsigned int c = src[i];
        unsigned int hi = c >> 4;
        unsigned int lo = c & 0x0f;
        dst[0] = (Py_UCS1)(hi + '0' + (hi > 9) * ('a' - '0' - 10));
        dst[1] = (Py_UCS1)(lo + '0' + (lo > 9) * ('a' - '0' - 10));
    }
}

#endif /* PY_HEXLIFY_CAN_COMPILE_NEON */

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
#if PY_HEXLIFY_CAN_COMPILE_SSE2
        /* Use SSE2 for inputs >= 16 bytes (always available on x86-64) */
        if (arglen >= 16) {
            _Py_hexlify_sse2((const unsigned char *)argbuf, retbuf, arglen);
        }
        else
#elif PY_HEXLIFY_CAN_COMPILE_NEON
        /* Use NEON for inputs >= 16 bytes (always available on AArch64) */
        if (arglen >= 16) {
            _Py_hexlify_neon((const unsigned char *)argbuf, retbuf, arglen);
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
