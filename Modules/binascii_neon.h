/* AArch64 NEON helpers for the base64 fast paths in binascii.c.
 *
 * Both functions process a prefix of the input and return how much they
 * consumed.  base64_encode_fast() and base64_decode_fast() continue from
 * there with the scalar loops, which remain the reference for behaviour:
 * the tail, padding, invalid characters and error reporting are never
 * handled here.
 *
 * Neither function reads outside [in, in + in_len) or writes more than the
 * scalar code would for the consumed prefix.
 */
#ifndef BINASCII_NEON_H
#define BINASCII_NEON_H

#if defined(__aarch64__) || defined(_M_ARM64)
#define BINASCII_HAVE_NEON 1

#include <arm_neon.h>
#include <stdint.h>

/* Encoding works for any 64-byte alphabet: the 6-bit indices are looked up
 * in the table with TBL, which takes four 16-byte registers.
 *
 * 48 input bytes -> 64 output characters. */
static inline void
b64neon_encode48(const unsigned char *in, unsigned char *out,
                 uint8x16x4_t tbl)
{
    uint8x16x3_t s = vld3q_u8(in);
    uint8x16x4_t d;
    uint8x16_t m = vdupq_n_u8(0x3f);

    d.val[0] = vshrq_n_u8(s.val[0], 2);
    d.val[1] = vandq_u8(vsliq_n_u8(vshrq_n_u8(s.val[1], 4), s.val[0], 4), m);
    d.val[2] = vandq_u8(vsliq_n_u8(vshrq_n_u8(s.val[2], 6), s.val[1], 2), m);
    d.val[3] = vandq_u8(s.val[2], m);

    d.val[0] = vqtbl4q_u8(tbl, d.val[0]);
    d.val[1] = vqtbl4q_u8(tbl, d.val[1]);
    d.val[2] = vqtbl4q_u8(tbl, d.val[2]);
    d.val[3] = vqtbl4q_u8(tbl, d.val[3]);
    vst4q_u8(out, d);
}

/* 24 input bytes -> 32 output characters. */
static inline void
b64neon_encode24(const unsigned char *in, unsigned char *out,
                 uint8x16x4_t tbl)
{
    uint8x8x3_t s = vld3_u8(in);
    uint8x8x4_t d;
    uint8x8_t m = vdup_n_u8(0x3f);

    d.val[0] = vshr_n_u8(s.val[0], 2);
    d.val[1] = vand_u8(vsli_n_u8(vshr_n_u8(s.val[1], 4), s.val[0], 4), m);
    d.val[2] = vand_u8(vsli_n_u8(vshr_n_u8(s.val[2], 6), s.val[1], 2), m);
    d.val[3] = vand_u8(s.val[2], m);

    d.val[0] = vqtbl4_u8(tbl, d.val[0]);
    d.val[1] = vqtbl4_u8(tbl, d.val[1]);
    d.val[2] = vqtbl4_u8(tbl, d.val[2]);
    d.val[3] = vqtbl4_u8(tbl, d.val[3]);
    vst4_u8(out, d);
}

/* Encode all complete 3-byte groups.  Requires in_len >= 24.
 * Returns in_len rounded down to a multiple of 3.
 *
 * A length that is not a multiple of the block size is finished by encoding
 * one more block that overlaps the previous one.  The overlapping part is
 * rewritten with the same characters, since block boundaries stay on 3-byte
 * group boundaries. */
static inline Py_ssize_t
base64_encode_neon(const unsigned char *in, Py_ssize_t in_len,
                   unsigned char *out, const unsigned char *table)
{
    Py_ssize_t n = in_len / 3 * 3;
    Py_ssize_t i = 0;
    uint8x16x4_t tbl = vld1q_u8_x4(table);

    assert(n >= 24);
    if (n < 48) {
        b64neon_encode24(in, out, tbl);
        if (n > 24) {
            b64neon_encode24(in + n - 24, out + (n - 24) / 3 * 4, tbl);
        }
        return n;
    }

    for (; i + 96 <= n; i += 96) {
        b64neon_encode48(in + i, out + i / 3 * 4, tbl);
        b64neon_encode48(in + i + 48, out + i / 3 * 4 + 64, tbl);
    }
    if (i + 48 <= n) {
        b64neon_encode48(in + i, out + i / 3 * 4, tbl);
        i += 48;
    }
    if (i < n) {
        b64neon_encode48(in + n - 48, out + (n - 48) / 3 * 4, tbl);
    }
    return n;
}

/* Decoding with the standard alphabet validates and translates with nibble
 * lookups instead of the 256-byte table.  The technique is described in
 * W. Mula and D. Lemire, "Faster Base64 Encoding and Decoding using AVX2
 * Instructions", ACM TOW 12(3), 2018.
 *
 * For each input byte, err is nonzero iff the byte is not in the alphabet. */
static inline uint8x16_t
b64neon_translate_std(uint8x16_t x, uint8x16_t *err)
{
    static const uint8_t lut_lo[16] = {
        0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A,
    };
    static const uint8_t lut_hi[16] = {
        0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
        0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    };
    static const uint8_t lut_roll[16] = {
        0, 16, 19, 4, (uint8_t)-65, (uint8_t)-65, (uint8_t)-71, (uint8_t)-71,
        0, 0, 0, 0, 0, 0, 0, 0,
    };
    uint8x16_t hi = vshrq_n_u8(x, 4);
    uint8x16_t lo = vandq_u8(x, vdupq_n_u8(0x0f));
    uint8x16_t e = vandq_u8(vqtbl1q_u8(vld1q_u8(lut_lo), lo),
                            vqtbl1q_u8(vld1q_u8(lut_hi), hi));
    /* '/' shares its high nibble with '+' but needs a different offset;
     * vceqq_u8() gives -1 for it, which selects lut_roll[1]. */
    uint8x16_t idx = vaddq_u8(hi, vceqq_u8(x, vdupq_n_u8('/')));
    *err = vorrq_u8(*err, e);
    return vaddq_u8(x, vqtbl1q_u8(vld1q_u8(lut_roll), idx));
}

typedef struct {
    uint8x16x4_t t[4];
    int high;       /* nonzero if some byte >= 0x80 maps to a valid value */
} b64neon_table;

/* Other alphabets use the 256-byte reverse table directly, 64 bytes per
 * TBL/TBX.  The upper half is skipped when the table rejects every byte
 * >= 0x80, which is the case for every ASCII alphabet.
 *
 * The scalar code rejects a value with either of the top two bits set;
 * a valid value is <= 63, so any rejection makes the OR of err exceed 63. */
static inline uint8x16_t
b64neon_translate_table(uint8x16_t x, uint8x16_t *err,
                        const b64neon_table *T)
{
    uint8x16_t v = vqtbl4q_u8(T->t[0], x);
    v = vqtbx4q_u8(v, T->t[1], veorq_u8(x, vdupq_n_u8(0x40)));
    if (T->high) {
        v = vqtbx4q_u8(v, T->t[2], veorq_u8(x, vdupq_n_u8(0x80)));
        v = vqtbx4q_u8(v, T->t[3], veorq_u8(x, vdupq_n_u8(0xc0)));
        *err = vorrq_u8(*err, v);
    }
    else {
        *err = vorrq_u8(*err, vorrq_u8(v, vandq_u8(x, vdupq_n_u8(0x80))));
    }
    return v;
}

/* Pack 16 6-bit values (4 quads) into 12 bytes and store exactly 12. */
static inline void
b64neon_pack16(uint8x16_t v, unsigned char *out)
{
    static const uint8_t shuf[16] = {
        2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, 0xff, 0xff, 0xff, 0xff,
    };
    uint16x8_t w16 = vreinterpretq_u16_u8(v);
    w16 = vandq_u16(vsliq_n_u16(vshrq_n_u16(w16, 8), w16, 6),
                    vdupq_n_u16(0x0fff));
    uint32x4_t w32 = vreinterpretq_u32_u16(w16);
    w32 = vsliq_n_u32(vshrq_n_u32(w32, 16), w32, 12);
    uint8x16_t r = vqtbl1q_u8(vreinterpretq_u8_u32(w32), vld1q_u8(shuf));
    vst1_u8(out, vget_low_u8(r));
    vst1q_lane_u32((uint32_t *)(void *)(out + 8), vreinterpretq_u32_u8(r), 2);
}

#define B64NEON_TRANSLATE(x, errp) \
    (is_std ? b64neon_translate_std((x), (errp)) \
            : b64neon_translate_table((x), (errp), &T))

/* Decode blocks of 64, then 16, characters.  Stops before the first block
 * containing a character the table rejects; the scalar loop then finds the
 * exact position.  Returns the number of characters consumed, a multiple
 * of 4.  is_std must be a constant so that each caller is specialized. */
static inline Py_ssize_t
b64neon_decode_blocks(const unsigned char *in, Py_ssize_t in_len,
                      unsigned char *out, const unsigned char *table,
                      const int is_std)
{
    const unsigned char *const start = in;
    b64neon_table T = {0};

    if (!is_std) {
        for (int i = 0; i < 4; i++) {
            T.t[i] = vld1q_u8_x4(table + 64 * i);
        }
        for (int i = 0x80; i < 0x100; i++) {
            if (!(table[i] & 0xc0)) {
                T.high = 1;
                break;
            }
        }
    }

    for (; in_len >= 64; in += 64, out += 48, in_len -= 64) {
        uint8x16x4_t s = vld4q_u8(in);
        uint8x16_t e0 = vdupq_n_u8(0), e1 = vdupq_n_u8(0);
        uint8x16_t a = B64NEON_TRANSLATE(s.val[0], &e0);
        uint8x16_t b = B64NEON_TRANSLATE(s.val[1], &e1);
        uint8x16_t c = B64NEON_TRANSLATE(s.val[2], &e0);
        uint8x16_t q = B64NEON_TRANSLATE(s.val[3], &e1);
        uint8_t e = vmaxvq_u8(vorrq_u8(e0, e1));
        if (is_std ? e != 0 : e > 63) {
            break;
        }
        uint8x16x3_t d;
        d.val[0] = vsliq_n_u8(vshrq_n_u8(b, 4), a, 2);
        d.val[1] = vsliq_n_u8(vshrq_n_u8(c, 2), b, 4);
        d.val[2] = vsliq_n_u8(q, c, 6);
        vst3q_u8(out, d);
    }
    for (; in_len >= 16; in += 16, out += 12, in_len -= 16) {
        uint8x16_t e0 = vdupq_n_u8(0);
        uint8x16_t v = B64NEON_TRANSLATE(vld1q_u8(in), &e0);
        uint8_t e = vmaxvq_u8(e0);
        if (is_std ? e != 0 : e > 63) {
            break;
        }
        b64neon_pack16(v, out);
    }
    return in - start;
}

#undef B64NEON_TRANSLATE

static inline Py_ssize_t
base64_decode_neon(const unsigned char *in, Py_ssize_t in_len,
                   unsigned char *out, const unsigned char *table,
                   int is_std)
{
    if (is_std) {
        return b64neon_decode_blocks(in, in_len, out, table, 1);
    }
    return b64neon_decode_blocks(in, in_len, out, table, 0);
}

#endif /* __aarch64__ || _M_ARM64 */
#endif /* BINASCII_NEON_H */
