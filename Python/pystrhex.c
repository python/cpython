/* Format bytes as hexadecimal */

#include "Python.h"
#include "pycore_strhex.h"        // _Py_strhex_with_sep()
#include "pycore_unicodeobject.h" // _PyUnicode_CheckConsistency()

/* Scalar hexlify: convert len bytes to 2*len hex characters.
   Uses table lookup via Py_hexdigits for the conversion. */
static inline void
_Py_hexlify_scalar(const unsigned char *src, Py_UCS1 *dst, Py_ssize_t len)
{
    for (Py_ssize_t i = 0; i < len; i++) {
        unsigned char c = src[i];
        *dst++ = Py_hexdigits[c >> 4];
        *dst++ = Py_hexdigits[c & 0x0f];
    }
}

/* Portable SIMD optimization for hexlify using GCC/Clang vector extensions.
   Uses __builtin_shufflevector for portable interleave that compiles to
   native SIMD instructions (SSE2 punpcklbw/punpckhbw on x86-64,
   NEON zip1/zip2 on ARM64, vzip on ARM32).

   Requirements:
   - GCC 12+ or Clang 3.0+ (for __builtin_shufflevector)
   - x86-64, ARM64, or ARM32 with NEON */
#if (defined(__x86_64__) || defined(__aarch64__) || \
     (defined(__arm__) && defined(__ARM_NEON))) && \
    (defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 12))
#  define PY_HEXLIFY_CAN_COMPILE_SIMD 1
#else
#  define PY_HEXLIFY_CAN_COMPILE_SIMD 0
#endif

#if PY_HEXLIFY_CAN_COMPILE_SIMD

/* 128-bit vector of 16 unsigned bytes */
typedef unsigned char v16u8 __attribute__((vector_size(16)));
/* 128-bit vector of 16 signed bytes - for efficient comparison.
   Using signed comparison generates pcmpgtb on x86-64 instead of
   the slower psubusb+pcmpeqb sequence from unsigned comparison. */
typedef signed char v16s8 __attribute__((vector_size(16)));

/* Splat a byte value across all 16 lanes */
static inline v16u8
v16u8_splat(unsigned char x)
{
    return (v16u8){x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x};
}

static inline v16s8
v16s8_splat(signed char x)
{
    return (v16s8){x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x};
}

/* Portable SIMD hexlify: converts 16 bytes to 32 hex chars per iteration.
   Compiles to native SSE2 on x86-64, NEON on ARM64. */
static void
_Py_hexlify_simd(const unsigned char *src, Py_UCS1 *dst, Py_ssize_t len)
{
    const v16u8 mask_0f = v16u8_splat(0x0f);
    const v16u8 ascii_0 = v16u8_splat('0');
    const v16u8 offset = v16u8_splat('a' - '0' - 10);  /* 0x27 */
    const v16s8 nine = v16s8_splat(9);

    Py_ssize_t i = 0;

    /* Process 16 bytes at a time */
    for (; i + 16 <= len; i += 16, dst += 32) {
        /* Load 16 bytes (memcpy for safe unaligned access) */
        v16u8 data;
        memcpy(&data, src + i, 16);

        /* Extract high and low nibbles using vector operators */
        v16u8 hi = (data >> 4) & mask_0f;
        v16u8 lo = data & mask_0f;

        /* Compare > 9 using signed comparison for efficient codegen.
           Nibble values 0-15 are safely in signed byte range.
           This generates pcmpgtb on x86-64, avoiding the slower
           psubusb+pcmpeqb sequence from unsigned comparison. */
        v16u8 hi_gt9 = (v16u8)((v16s8)hi > nine);
        v16u8 lo_gt9 = (v16u8)((v16s8)lo > nine);

        /* Convert nibbles to hex ASCII */
        hi = hi + ascii_0 + (hi_gt9 & offset);
        lo = lo + ascii_0 + (lo_gt9 & offset);

        /* Interleave hi/lo nibbles using portable shufflevector.
           This compiles to punpcklbw/punpckhbw on x86-64, zip1/zip2 on ARM64. */
        v16u8 result0 = __builtin_shufflevector(hi, lo,
            0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23);
        v16u8 result1 = __builtin_shufflevector(hi, lo,
            8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31);

        /* Store 32 hex characters */
        memcpy(dst, &result0, 16);
        memcpy(dst + 16, &result1, 16);
    }

    /* Scalar fallback for remaining 0-15 bytes */
    _Py_hexlify_scalar(src + i, dst, len - i);
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
        /* Use portable SIMD for inputs >= 16 bytes */
        if (arglen >= 16) {
            _Py_hexlify_simd((const unsigned char *)argbuf, retbuf, arglen);
        }
        else
#endif
        {
            _Py_hexlify_scalar((const unsigned char *)argbuf, retbuf, arglen);
        }
    }
    else {
        /* The number of complete chunk+sep periods */
        Py_ssize_t chunks = (arglen - 1) / abs_bytes_per_sep;
        Py_ssize_t chunk;
        unsigned int k;

#if PY_HEXLIFY_CAN_COMPILE_SIMD
        /* SIMD path for separator groups >= 8 bytes.
           SIMD hexlify to output buffer, then shuffle in-place to insert
           separators. Working backwards avoids overlap issues since we're
           expanding (destination index >= source index). */
        if (abs_bytes_per_sep >= 8 && arglen >= 16) {
            /* SIMD hexlify all bytes to start of output buffer */
            _Py_hexlify_simd((const unsigned char *)argbuf, retbuf, arglen);

            /* Shuffle in-place, working backwards */
            Py_ssize_t hex_chunk_size = 2 * (Py_ssize_t)abs_bytes_per_sep;
            Py_ssize_t remainder_bytes = arglen - chunks * (Py_ssize_t)abs_bytes_per_sep;
            Py_ssize_t remainder_hex_len = 2 * remainder_bytes;
            Py_ssize_t hex_pos = 2 * arglen;   /* End of hex data */
            Py_ssize_t out_pos = resultlen;    /* End of output */

            if (bytes_per_sep_group < 0) {
                /* Forward: remainder at end, separators after each chunk */
                if (remainder_hex_len > 0) {
                    hex_pos -= remainder_hex_len;
                    out_pos -= remainder_hex_len;
                    memmove(retbuf + out_pos, retbuf + hex_pos, remainder_hex_len);
                }
                for (Py_ssize_t c = chunks - 1; c >= 0; c--) {
                    retbuf[--out_pos] = sep_char;
                    hex_pos -= hex_chunk_size;
                    out_pos -= hex_chunk_size;
                    memmove(retbuf + out_pos, retbuf + hex_pos, hex_chunk_size);
                }
            }
            else {
                /* Backward: remainder at start, separators before each chunk */
                for (Py_ssize_t c = chunks - 1; c >= 0; c--) {
                    hex_pos -= hex_chunk_size;
                    out_pos -= hex_chunk_size;
                    memmove(retbuf + out_pos, retbuf + hex_pos, hex_chunk_size);
                    retbuf[--out_pos] = sep_char;
                }
                /* Remainder at start stays in place (hex_pos == out_pos == remainder_hex_len) */
            }
            goto done_hexlify;
        }
#endif /* PY_HEXLIFY_CAN_COMPILE_SIMD */

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

#if PY_HEXLIFY_CAN_COMPILE_SIMD
done_hexlify:
#endif

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
