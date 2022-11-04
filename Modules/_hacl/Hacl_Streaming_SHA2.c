/* MIT License
 *
 * Copyright (c) 2016-2020 INRIA, CMU and Microsoft Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "Hacl_Streaming_SHA2.h"

#include "internal/Hacl_SHA2_Types.h"

Hacl_Streaming_SHA2_state_sha2_224 *Hacl_Streaming_SHA2_create_in_224()
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC((uint32_t)64U, sizeof (uint8_t));
  uint32_t *block_state = (uint32_t *)KRML_HOST_CALLOC((uint32_t)8U, sizeof (uint32_t));
  Hacl_Streaming_SHA2_state_sha2_224
  s = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  KRML_CHECK_SIZE(sizeof (Hacl_Streaming_SHA2_state_sha2_224), (uint32_t)1U);
  Hacl_Streaming_SHA2_state_sha2_224
  *p =
    (Hacl_Streaming_SHA2_state_sha2_224 *)KRML_HOST_MALLOC(sizeof (
        Hacl_Streaming_SHA2_state_sha2_224
      ));
  p[0U] = s;
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = block_state;
    uint32_t x = Hacl_Impl_SHA2_Generic_h224[i];
    os[i] = x;);
  return p;
}

void Hacl_Streaming_SHA2_init_224(Hacl_Streaming_SHA2_state_sha2_224 *s)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = block_state;
    uint32_t x = Hacl_Impl_SHA2_Generic_h224[i];
    os[i] = x;);
  s[0U] =
    (
      (Hacl_Streaming_SHA2_state_sha2_224){
        .block_state = block_state,
        .buf = buf,
        .total_len = (uint64_t)0U
      }
    );
}

void Hacl_Streaming_SHA2_finish_224(Hacl_Streaming_SHA2_state_sha2_224 *p, uint8_t *dst)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *p;
  uint32_t *block_state = scrut.block_state;
  uint8_t *buf_ = scrut.buf;
  uint64_t total_len = scrut.total_len;
  uint32_t r;
  if (total_len % (uint64_t)(uint32_t)64U == (uint64_t)0U && total_len > (uint64_t)0U)
  {
    r = (uint32_t)64U;
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)(uint32_t)64U);
  }
  uint8_t *buf_1 = buf_;
  uint32_t tmp_block_state[8U] = { 0U };
  memcpy(tmp_block_state, block_state, (uint32_t)8U * sizeof (uint32_t));
  uint32_t ite;
  if (r % (uint32_t)64U == (uint32_t)0U && r > (uint32_t)0U)
  {
    ite = (uint32_t)64U;
  }
  else
  {
    ite = r % (uint32_t)64U;
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  uint32_t blocks0 = (uint32_t)0U;
  for (uint32_t i0 = (uint32_t)0U; i0 < blocks0; i0++)
  {
    uint8_t *b00 = buf_multi;
    uint8_t *mb = b00 + i0 * (uint32_t)64U;
    uint32_t hash_old[8U] = { 0U };
    uint32_t ws[16U] = { 0U };
    memcpy(hash_old, tmp_block_state, (uint32_t)8U * sizeof (uint32_t));
    uint8_t *b = mb;
    uint32_t u = load32_be(b);
    ws[0U] = u;
    uint32_t u0 = load32_be(b + (uint32_t)4U);
    ws[1U] = u0;
    uint32_t u1 = load32_be(b + (uint32_t)8U);
    ws[2U] = u1;
    uint32_t u2 = load32_be(b + (uint32_t)12U);
    ws[3U] = u2;
    uint32_t u3 = load32_be(b + (uint32_t)16U);
    ws[4U] = u3;
    uint32_t u4 = load32_be(b + (uint32_t)20U);
    ws[5U] = u4;
    uint32_t u5 = load32_be(b + (uint32_t)24U);
    ws[6U] = u5;
    uint32_t u6 = load32_be(b + (uint32_t)28U);
    ws[7U] = u6;
    uint32_t u7 = load32_be(b + (uint32_t)32U);
    ws[8U] = u7;
    uint32_t u8 = load32_be(b + (uint32_t)36U);
    ws[9U] = u8;
    uint32_t u9 = load32_be(b + (uint32_t)40U);
    ws[10U] = u9;
    uint32_t u10 = load32_be(b + (uint32_t)44U);
    ws[11U] = u10;
    uint32_t u11 = load32_be(b + (uint32_t)48U);
    ws[12U] = u11;
    uint32_t u12 = load32_be(b + (uint32_t)52U);
    ws[13U] = u12;
    uint32_t u13 = load32_be(b + (uint32_t)56U);
    ws[14U] = u13;
    uint32_t u14 = load32_be(b + (uint32_t)60U);
    ws[15U] = u14;
    KRML_MAYBE_FOR4(i1,
      (uint32_t)0U,
      (uint32_t)4U,
      (uint32_t)1U,
      KRML_MAYBE_FOR16(i,
        (uint32_t)0U,
        (uint32_t)16U,
        (uint32_t)1U,
        uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i1 + i];
        uint32_t ws_t = ws[i];
        uint32_t a0 = tmp_block_state[0U];
        uint32_t b0 = tmp_block_state[1U];
        uint32_t c0 = tmp_block_state[2U];
        uint32_t d0 = tmp_block_state[3U];
        uint32_t e0 = tmp_block_state[4U];
        uint32_t f0 = tmp_block_state[5U];
        uint32_t g0 = tmp_block_state[6U];
        uint32_t h05 = tmp_block_state[7U];
        uint32_t k_e_t = k_t;
        uint32_t
        t1 =
          h05
          +
            ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
            ^
              ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
              ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
          + ((e0 & f0) ^ (~e0 & g0))
          + k_e_t
          + ws_t;
        uint32_t
        t2 =
          ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
          ^
            ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
            ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
          + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
        uint32_t a1 = t1 + t2;
        uint32_t b1 = a0;
        uint32_t c1 = b0;
        uint32_t d1 = c0;
        uint32_t e1 = d0 + t1;
        uint32_t f1 = e0;
        uint32_t g1 = f0;
        uint32_t h13 = g0;
        tmp_block_state[0U] = a1;
        tmp_block_state[1U] = b1;
        tmp_block_state[2U] = c1;
        tmp_block_state[3U] = d1;
        tmp_block_state[4U] = e1;
        tmp_block_state[5U] = f1;
        tmp_block_state[6U] = g1;
        tmp_block_state[7U] = h13;);
      if (i1 < (uint32_t)3U)
      {
        KRML_MAYBE_FOR16(i,
          (uint32_t)0U,
          (uint32_t)16U,
          (uint32_t)1U,
          uint32_t t16 = ws[i];
          uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
          uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
          uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
          uint32_t
          s1 =
            (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
            ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
          uint32_t
          s0 =
            (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
            ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
          ws[i] = s1 + t7 + s0 + t16;);
      });
    KRML_MAYBE_FOR8(i,
      (uint32_t)0U,
      (uint32_t)8U,
      (uint32_t)1U,
      uint32_t *os = tmp_block_state;
      uint32_t x = tmp_block_state[i] + hash_old[i];
      os[i] = x;);
  }
  uint64_t prev_len_last = total_len - (uint64_t)r;
  uint32_t blocks;
  if (r + (uint32_t)8U + (uint32_t)1U <= (uint32_t)64U)
  {
    blocks = (uint32_t)1U;
  }
  else
  {
    blocks = (uint32_t)2U;
  }
  uint32_t fin = blocks * (uint32_t)64U;
  uint8_t last[128U] = { 0U };
  uint8_t totlen_buf[8U] = { 0U };
  uint64_t total_len_bits = (prev_len_last + (uint64_t)r) << (uint32_t)3U;
  store64_be(totlen_buf, total_len_bits);
  uint8_t *b00 = buf_last;
  memcpy(last, b00, r * sizeof (uint8_t));
  last[r] = (uint8_t)0x80U;
  memcpy(last + fin - (uint32_t)8U, totlen_buf, (uint32_t)8U * sizeof (uint8_t));
  uint8_t *last00 = last;
  uint8_t *last10 = last + (uint32_t)64U;
  Hacl_Impl_SHA2_Types_uint8_2p scrut0 = { .fst = last00, .snd = last10 };
  uint8_t *l0 = scrut0.fst;
  uint8_t *l1 = scrut0.snd;
  uint8_t *lb0 = l0;
  uint8_t *lb1 = l1;
  Hacl_Impl_SHA2_Types_uint8_2p scrut1 = { .fst = lb0, .snd = lb1 };
  uint8_t *last0 = scrut1.fst;
  uint8_t *last1 = scrut1.snd;
  uint32_t hash_old[8U] = { 0U };
  uint32_t ws0[16U] = { 0U };
  memcpy(hash_old, tmp_block_state, (uint32_t)8U * sizeof (uint32_t));
  uint8_t *b2 = last0;
  uint32_t u0 = load32_be(b2);
  ws0[0U] = u0;
  uint32_t u1 = load32_be(b2 + (uint32_t)4U);
  ws0[1U] = u1;
  uint32_t u2 = load32_be(b2 + (uint32_t)8U);
  ws0[2U] = u2;
  uint32_t u3 = load32_be(b2 + (uint32_t)12U);
  ws0[3U] = u3;
  uint32_t u4 = load32_be(b2 + (uint32_t)16U);
  ws0[4U] = u4;
  uint32_t u5 = load32_be(b2 + (uint32_t)20U);
  ws0[5U] = u5;
  uint32_t u6 = load32_be(b2 + (uint32_t)24U);
  ws0[6U] = u6;
  uint32_t u7 = load32_be(b2 + (uint32_t)28U);
  ws0[7U] = u7;
  uint32_t u8 = load32_be(b2 + (uint32_t)32U);
  ws0[8U] = u8;
  uint32_t u9 = load32_be(b2 + (uint32_t)36U);
  ws0[9U] = u9;
  uint32_t u10 = load32_be(b2 + (uint32_t)40U);
  ws0[10U] = u10;
  uint32_t u11 = load32_be(b2 + (uint32_t)44U);
  ws0[11U] = u11;
  uint32_t u12 = load32_be(b2 + (uint32_t)48U);
  ws0[12U] = u12;
  uint32_t u13 = load32_be(b2 + (uint32_t)52U);
  ws0[13U] = u13;
  uint32_t u14 = load32_be(b2 + (uint32_t)56U);
  ws0[14U] = u14;
  uint32_t u15 = load32_be(b2 + (uint32_t)60U);
  ws0[15U] = u15;
  KRML_MAYBE_FOR4(i0,
    (uint32_t)0U,
    (uint32_t)4U,
    (uint32_t)1U,
    KRML_MAYBE_FOR16(i,
      (uint32_t)0U,
      (uint32_t)16U,
      (uint32_t)1U,
      uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i0 + i];
      uint32_t ws_t = ws0[i];
      uint32_t a0 = tmp_block_state[0U];
      uint32_t b0 = tmp_block_state[1U];
      uint32_t c0 = tmp_block_state[2U];
      uint32_t d0 = tmp_block_state[3U];
      uint32_t e0 = tmp_block_state[4U];
      uint32_t f0 = tmp_block_state[5U];
      uint32_t g0 = tmp_block_state[6U];
      uint32_t h05 = tmp_block_state[7U];
      uint32_t k_e_t = k_t;
      uint32_t
      t1 =
        h05
        +
          ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
          ^
            ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
            ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
        + ((e0 & f0) ^ (~e0 & g0))
        + k_e_t
        + ws_t;
      uint32_t
      t2 =
        ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
        ^
          ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
          ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
        + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
      uint32_t a1 = t1 + t2;
      uint32_t b1 = a0;
      uint32_t c1 = b0;
      uint32_t d1 = c0;
      uint32_t e1 = d0 + t1;
      uint32_t f1 = e0;
      uint32_t g1 = f0;
      uint32_t h14 = g0;
      tmp_block_state[0U] = a1;
      tmp_block_state[1U] = b1;
      tmp_block_state[2U] = c1;
      tmp_block_state[3U] = d1;
      tmp_block_state[4U] = e1;
      tmp_block_state[5U] = f1;
      tmp_block_state[6U] = g1;
      tmp_block_state[7U] = h14;);
    if (i0 < (uint32_t)3U)
    {
      KRML_MAYBE_FOR16(i,
        (uint32_t)0U,
        (uint32_t)16U,
        (uint32_t)1U,
        uint32_t t16 = ws0[i];
        uint32_t t15 = ws0[(i + (uint32_t)1U) % (uint32_t)16U];
        uint32_t t7 = ws0[(i + (uint32_t)9U) % (uint32_t)16U];
        uint32_t t2 = ws0[(i + (uint32_t)14U) % (uint32_t)16U];
        uint32_t
        s1 =
          (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
          ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
        uint32_t
        s0 =
          (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
          ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
        ws0[i] = s1 + t7 + s0 + t16;);
    });
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = tmp_block_state;
    uint32_t x = tmp_block_state[i] + hash_old[i];
    os[i] = x;);
  if (blocks > (uint32_t)1U)
  {
    uint32_t hash_old0[8U] = { 0U };
    uint32_t ws[16U] = { 0U };
    memcpy(hash_old0, tmp_block_state, (uint32_t)8U * sizeof (uint32_t));
    uint8_t *b = last1;
    uint32_t u = load32_be(b);
    ws[0U] = u;
    uint32_t u16 = load32_be(b + (uint32_t)4U);
    ws[1U] = u16;
    uint32_t u17 = load32_be(b + (uint32_t)8U);
    ws[2U] = u17;
    uint32_t u18 = load32_be(b + (uint32_t)12U);
    ws[3U] = u18;
    uint32_t u19 = load32_be(b + (uint32_t)16U);
    ws[4U] = u19;
    uint32_t u20 = load32_be(b + (uint32_t)20U);
    ws[5U] = u20;
    uint32_t u21 = load32_be(b + (uint32_t)24U);
    ws[6U] = u21;
    uint32_t u22 = load32_be(b + (uint32_t)28U);
    ws[7U] = u22;
    uint32_t u23 = load32_be(b + (uint32_t)32U);
    ws[8U] = u23;
    uint32_t u24 = load32_be(b + (uint32_t)36U);
    ws[9U] = u24;
    uint32_t u25 = load32_be(b + (uint32_t)40U);
    ws[10U] = u25;
    uint32_t u26 = load32_be(b + (uint32_t)44U);
    ws[11U] = u26;
    uint32_t u27 = load32_be(b + (uint32_t)48U);
    ws[12U] = u27;
    uint32_t u28 = load32_be(b + (uint32_t)52U);
    ws[13U] = u28;
    uint32_t u29 = load32_be(b + (uint32_t)56U);
    ws[14U] = u29;
    uint32_t u30 = load32_be(b + (uint32_t)60U);
    ws[15U] = u30;
    KRML_MAYBE_FOR4(i0,
      (uint32_t)0U,
      (uint32_t)4U,
      (uint32_t)1U,
      KRML_MAYBE_FOR16(i,
        (uint32_t)0U,
        (uint32_t)16U,
        (uint32_t)1U,
        uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i0 + i];
        uint32_t ws_t = ws[i];
        uint32_t a0 = tmp_block_state[0U];
        uint32_t b0 = tmp_block_state[1U];
        uint32_t c0 = tmp_block_state[2U];
        uint32_t d0 = tmp_block_state[3U];
        uint32_t e0 = tmp_block_state[4U];
        uint32_t f0 = tmp_block_state[5U];
        uint32_t g0 = tmp_block_state[6U];
        uint32_t h05 = tmp_block_state[7U];
        uint32_t k_e_t = k_t;
        uint32_t
        t1 =
          h05
          +
            ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
            ^
              ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
              ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
          + ((e0 & f0) ^ (~e0 & g0))
          + k_e_t
          + ws_t;
        uint32_t
        t2 =
          ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
          ^
            ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
            ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
          + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
        uint32_t a1 = t1 + t2;
        uint32_t b1 = a0;
        uint32_t c1 = b0;
        uint32_t d1 = c0;
        uint32_t e1 = d0 + t1;
        uint32_t f1 = e0;
        uint32_t g1 = f0;
        uint32_t h14 = g0;
        tmp_block_state[0U] = a1;
        tmp_block_state[1U] = b1;
        tmp_block_state[2U] = c1;
        tmp_block_state[3U] = d1;
        tmp_block_state[4U] = e1;
        tmp_block_state[5U] = f1;
        tmp_block_state[6U] = g1;
        tmp_block_state[7U] = h14;);
      if (i0 < (uint32_t)3U)
      {
        KRML_MAYBE_FOR16(i,
          (uint32_t)0U,
          (uint32_t)16U,
          (uint32_t)1U,
          uint32_t t16 = ws[i];
          uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
          uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
          uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
          uint32_t
          s1 =
            (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
            ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
          uint32_t
          s0 =
            (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
            ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
          ws[i] = s1 + t7 + s0 + t16;);
      });
    KRML_MAYBE_FOR8(i,
      (uint32_t)0U,
      (uint32_t)8U,
      (uint32_t)1U,
      uint32_t *os = tmp_block_state;
      uint32_t x = tmp_block_state[i] + hash_old0[i];
      os[i] = x;);
  }
  uint8_t hbuf[32U] = { 0U };
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    store32_be(hbuf + i * (uint32_t)4U, tmp_block_state[i]););
  memcpy(dst, hbuf, (uint32_t)28U * sizeof (uint8_t));
}

void Hacl_Streaming_SHA2_free_224(Hacl_Streaming_SHA2_state_sha2_224 *s)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_HOST_FREE(block_state);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(s);
}

Hacl_Streaming_SHA2_state_sha2_224 *Hacl_Streaming_SHA2_create_in_256()
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC((uint32_t)64U, sizeof (uint8_t));
  uint32_t *block_state = (uint32_t *)KRML_HOST_CALLOC((uint32_t)8U, sizeof (uint32_t));
  Hacl_Streaming_SHA2_state_sha2_224
  s = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  KRML_CHECK_SIZE(sizeof (Hacl_Streaming_SHA2_state_sha2_224), (uint32_t)1U);
  Hacl_Streaming_SHA2_state_sha2_224
  *p =
    (Hacl_Streaming_SHA2_state_sha2_224 *)KRML_HOST_MALLOC(sizeof (
        Hacl_Streaming_SHA2_state_sha2_224
      ));
  p[0U] = s;
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = block_state;
    uint32_t x = Hacl_Impl_SHA2_Generic_h256[i];
    os[i] = x;);
  return p;
}

void Hacl_Streaming_SHA2_init_256(Hacl_Streaming_SHA2_state_sha2_224 *s)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = block_state;
    uint32_t x = Hacl_Impl_SHA2_Generic_h256[i];
    os[i] = x;);
  s[0U] =
    (
      (Hacl_Streaming_SHA2_state_sha2_224){
        .block_state = block_state,
        .buf = buf,
        .total_len = (uint64_t)0U
      }
    );
}

/**
0 = success, 1 = max length exceeded
*/
uint32_t
Hacl_Streaming_SHA2_update_256(
  Hacl_Streaming_SHA2_state_sha2_224 *p,
  uint8_t *data,
  uint32_t len
)
{
  Hacl_Streaming_SHA2_state_sha2_224 s = *p;
  uint64_t total_len = s.total_len;
  if ((uint64_t)len > (uint64_t)2305843009213693951U - total_len)
  {
    return (uint32_t)1U;
  }
  uint32_t sz;
  if (total_len % (uint64_t)(uint32_t)64U == (uint64_t)0U && total_len > (uint64_t)0U)
  {
    sz = (uint32_t)64U;
  }
  else
  {
    sz = (uint32_t)(total_len % (uint64_t)(uint32_t)64U);
  }
  if (len <= (uint32_t)64U - sz)
  {
    Hacl_Streaming_SHA2_state_sha2_224 s1 = *p;
    uint32_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)(uint32_t)64U == (uint64_t)0U && total_len1 > (uint64_t)0U)
    {
      sz1 = (uint32_t)64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)(uint32_t)64U);
    }
    uint8_t *buf2 = buf + sz1;
    memcpy(buf2, data, len * sizeof (uint8_t));
    uint64_t total_len2 = total_len1 + (uint64_t)len;
    *p
    =
      (
        (Hacl_Streaming_SHA2_state_sha2_224){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len2
        }
      );
  }
  else if (sz == (uint32_t)0U)
  {
    Hacl_Streaming_SHA2_state_sha2_224 s1 = *p;
    uint32_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)(uint32_t)64U == (uint64_t)0U && total_len1 > (uint64_t)0U)
    {
      sz1 = (uint32_t)64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)(uint32_t)64U);
    }
    if (!(sz1 == (uint32_t)0U))
    {
      uint32_t blocks = (uint32_t)1U;
      for (uint32_t i0 = (uint32_t)0U; i0 < blocks; i0++)
      {
        uint8_t *b00 = buf;
        uint8_t *mb = b00 + i0 * (uint32_t)64U;
        uint32_t hash_old[8U] = { 0U };
        uint32_t ws[16U] = { 0U };
        memcpy(hash_old, block_state1, (uint32_t)8U * sizeof (uint32_t));
        uint8_t *b = mb;
        uint32_t u = load32_be(b);
        ws[0U] = u;
        uint32_t u0 = load32_be(b + (uint32_t)4U);
        ws[1U] = u0;
        uint32_t u1 = load32_be(b + (uint32_t)8U);
        ws[2U] = u1;
        uint32_t u2 = load32_be(b + (uint32_t)12U);
        ws[3U] = u2;
        uint32_t u3 = load32_be(b + (uint32_t)16U);
        ws[4U] = u3;
        uint32_t u4 = load32_be(b + (uint32_t)20U);
        ws[5U] = u4;
        uint32_t u5 = load32_be(b + (uint32_t)24U);
        ws[6U] = u5;
        uint32_t u6 = load32_be(b + (uint32_t)28U);
        ws[7U] = u6;
        uint32_t u7 = load32_be(b + (uint32_t)32U);
        ws[8U] = u7;
        uint32_t u8 = load32_be(b + (uint32_t)36U);
        ws[9U] = u8;
        uint32_t u9 = load32_be(b + (uint32_t)40U);
        ws[10U] = u9;
        uint32_t u10 = load32_be(b + (uint32_t)44U);
        ws[11U] = u10;
        uint32_t u11 = load32_be(b + (uint32_t)48U);
        ws[12U] = u11;
        uint32_t u12 = load32_be(b + (uint32_t)52U);
        ws[13U] = u12;
        uint32_t u13 = load32_be(b + (uint32_t)56U);
        ws[14U] = u13;
        uint32_t u14 = load32_be(b + (uint32_t)60U);
        ws[15U] = u14;
        KRML_MAYBE_FOR4(i1,
          (uint32_t)0U,
          (uint32_t)4U,
          (uint32_t)1U,
          KRML_MAYBE_FOR16(i,
            (uint32_t)0U,
            (uint32_t)16U,
            (uint32_t)1U,
            uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i1 + i];
            uint32_t ws_t = ws[i];
            uint32_t a0 = block_state1[0U];
            uint32_t b0 = block_state1[1U];
            uint32_t c0 = block_state1[2U];
            uint32_t d0 = block_state1[3U];
            uint32_t e0 = block_state1[4U];
            uint32_t f0 = block_state1[5U];
            uint32_t g0 = block_state1[6U];
            uint32_t h05 = block_state1[7U];
            uint32_t k_e_t = k_t;
            uint32_t
            t1 =
              h05
              +
                ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
                ^
                  ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
                  ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
              + ((e0 & f0) ^ (~e0 & g0))
              + k_e_t
              + ws_t;
            uint32_t
            t2 =
              ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
              ^
                ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
                ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
              + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
            uint32_t a1 = t1 + t2;
            uint32_t b1 = a0;
            uint32_t c1 = b0;
            uint32_t d1 = c0;
            uint32_t e1 = d0 + t1;
            uint32_t f1 = e0;
            uint32_t g1 = f0;
            uint32_t h12 = g0;
            block_state1[0U] = a1;
            block_state1[1U] = b1;
            block_state1[2U] = c1;
            block_state1[3U] = d1;
            block_state1[4U] = e1;
            block_state1[5U] = f1;
            block_state1[6U] = g1;
            block_state1[7U] = h12;);
          if (i1 < (uint32_t)3U)
          {
            KRML_MAYBE_FOR16(i,
              (uint32_t)0U,
              (uint32_t)16U,
              (uint32_t)1U,
              uint32_t t16 = ws[i];
              uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
              uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
              uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
              uint32_t
              s11 =
                (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
                ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
              uint32_t
              s0 =
                (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
                ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
              ws[i] = s11 + t7 + s0 + t16;);
          });
        KRML_MAYBE_FOR8(i,
          (uint32_t)0U,
          (uint32_t)8U,
          (uint32_t)1U,
          uint32_t *os = block_state1;
          uint32_t x = block_state1[i] + hash_old[i];
          os[i] = x;);
      }
    }
    uint32_t ite;
    if ((uint64_t)len % (uint64_t)(uint32_t)64U == (uint64_t)0U && (uint64_t)len > (uint64_t)0U)
    {
      ite = (uint32_t)64U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)len % (uint64_t)(uint32_t)64U);
    }
    uint32_t n_blocks = (len - ite) / (uint32_t)64U;
    uint32_t data1_len = n_blocks * (uint32_t)64U;
    uint32_t data2_len = len - data1_len;
    uint8_t *data1 = data;
    uint8_t *data2 = data + data1_len;
    uint32_t blocks = data1_len / (uint32_t)64U;
    for (uint32_t i0 = (uint32_t)0U; i0 < blocks; i0++)
    {
      uint8_t *b00 = data1;
      uint8_t *mb = b00 + i0 * (uint32_t)64U;
      uint32_t hash_old[8U] = { 0U };
      uint32_t ws[16U] = { 0U };
      memcpy(hash_old, block_state1, (uint32_t)8U * sizeof (uint32_t));
      uint8_t *b = mb;
      uint32_t u = load32_be(b);
      ws[0U] = u;
      uint32_t u0 = load32_be(b + (uint32_t)4U);
      ws[1U] = u0;
      uint32_t u1 = load32_be(b + (uint32_t)8U);
      ws[2U] = u1;
      uint32_t u2 = load32_be(b + (uint32_t)12U);
      ws[3U] = u2;
      uint32_t u3 = load32_be(b + (uint32_t)16U);
      ws[4U] = u3;
      uint32_t u4 = load32_be(b + (uint32_t)20U);
      ws[5U] = u4;
      uint32_t u5 = load32_be(b + (uint32_t)24U);
      ws[6U] = u5;
      uint32_t u6 = load32_be(b + (uint32_t)28U);
      ws[7U] = u6;
      uint32_t u7 = load32_be(b + (uint32_t)32U);
      ws[8U] = u7;
      uint32_t u8 = load32_be(b + (uint32_t)36U);
      ws[9U] = u8;
      uint32_t u9 = load32_be(b + (uint32_t)40U);
      ws[10U] = u9;
      uint32_t u10 = load32_be(b + (uint32_t)44U);
      ws[11U] = u10;
      uint32_t u11 = load32_be(b + (uint32_t)48U);
      ws[12U] = u11;
      uint32_t u12 = load32_be(b + (uint32_t)52U);
      ws[13U] = u12;
      uint32_t u13 = load32_be(b + (uint32_t)56U);
      ws[14U] = u13;
      uint32_t u14 = load32_be(b + (uint32_t)60U);
      ws[15U] = u14;
      KRML_MAYBE_FOR4(i1,
        (uint32_t)0U,
        (uint32_t)4U,
        (uint32_t)1U,
        KRML_MAYBE_FOR16(i,
          (uint32_t)0U,
          (uint32_t)16U,
          (uint32_t)1U,
          uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i1 + i];
          uint32_t ws_t = ws[i];
          uint32_t a0 = block_state1[0U];
          uint32_t b0 = block_state1[1U];
          uint32_t c0 = block_state1[2U];
          uint32_t d0 = block_state1[3U];
          uint32_t e0 = block_state1[4U];
          uint32_t f0 = block_state1[5U];
          uint32_t g0 = block_state1[6U];
          uint32_t h05 = block_state1[7U];
          uint32_t k_e_t = k_t;
          uint32_t
          t1 =
            h05
            +
              ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
              ^
                ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
                ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
            + ((e0 & f0) ^ (~e0 & g0))
            + k_e_t
            + ws_t;
          uint32_t
          t2 =
            ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
            ^
              ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
              ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
            + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
          uint32_t a1 = t1 + t2;
          uint32_t b1 = a0;
          uint32_t c1 = b0;
          uint32_t d1 = c0;
          uint32_t e1 = d0 + t1;
          uint32_t f1 = e0;
          uint32_t g1 = f0;
          uint32_t h13 = g0;
          block_state1[0U] = a1;
          block_state1[1U] = b1;
          block_state1[2U] = c1;
          block_state1[3U] = d1;
          block_state1[4U] = e1;
          block_state1[5U] = f1;
          block_state1[6U] = g1;
          block_state1[7U] = h13;);
        if (i1 < (uint32_t)3U)
        {
          KRML_MAYBE_FOR16(i,
            (uint32_t)0U,
            (uint32_t)16U,
            (uint32_t)1U,
            uint32_t t16 = ws[i];
            uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
            uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
            uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
            uint32_t
            s11 =
              (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
              ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
            uint32_t
            s0 =
              (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
              ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
            ws[i] = s11 + t7 + s0 + t16;);
        });
      KRML_MAYBE_FOR8(i,
        (uint32_t)0U,
        (uint32_t)8U,
        (uint32_t)1U,
        uint32_t *os = block_state1;
        uint32_t x = block_state1[i] + hash_old[i];
        os[i] = x;);
    }
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *p
    =
      (
        (Hacl_Streaming_SHA2_state_sha2_224){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)len
        }
      );
  }
  else
  {
    uint32_t diff = (uint32_t)64U - sz;
    uint8_t *data1 = data;
    uint8_t *data2 = data + diff;
    Hacl_Streaming_SHA2_state_sha2_224 s1 = *p;
    uint32_t *block_state10 = s1.block_state;
    uint8_t *buf0 = s1.buf;
    uint64_t total_len10 = s1.total_len;
    uint32_t sz10;
    if (total_len10 % (uint64_t)(uint32_t)64U == (uint64_t)0U && total_len10 > (uint64_t)0U)
    {
      sz10 = (uint32_t)64U;
    }
    else
    {
      sz10 = (uint32_t)(total_len10 % (uint64_t)(uint32_t)64U);
    }
    uint8_t *buf2 = buf0 + sz10;
    memcpy(buf2, data1, diff * sizeof (uint8_t));
    uint64_t total_len2 = total_len10 + (uint64_t)diff;
    *p
    =
      (
        (Hacl_Streaming_SHA2_state_sha2_224){
          .block_state = block_state10,
          .buf = buf0,
          .total_len = total_len2
        }
      );
    Hacl_Streaming_SHA2_state_sha2_224 s10 = *p;
    uint32_t *block_state1 = s10.block_state;
    uint8_t *buf = s10.buf;
    uint64_t total_len1 = s10.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)(uint32_t)64U == (uint64_t)0U && total_len1 > (uint64_t)0U)
    {
      sz1 = (uint32_t)64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)(uint32_t)64U);
    }
    if (!(sz1 == (uint32_t)0U))
    {
      uint32_t blocks = (uint32_t)1U;
      for (uint32_t i0 = (uint32_t)0U; i0 < blocks; i0++)
      {
        uint8_t *b00 = buf;
        uint8_t *mb = b00 + i0 * (uint32_t)64U;
        uint32_t hash_old[8U] = { 0U };
        uint32_t ws[16U] = { 0U };
        memcpy(hash_old, block_state1, (uint32_t)8U * sizeof (uint32_t));
        uint8_t *b = mb;
        uint32_t u = load32_be(b);
        ws[0U] = u;
        uint32_t u0 = load32_be(b + (uint32_t)4U);
        ws[1U] = u0;
        uint32_t u1 = load32_be(b + (uint32_t)8U);
        ws[2U] = u1;
        uint32_t u2 = load32_be(b + (uint32_t)12U);
        ws[3U] = u2;
        uint32_t u3 = load32_be(b + (uint32_t)16U);
        ws[4U] = u3;
        uint32_t u4 = load32_be(b + (uint32_t)20U);
        ws[5U] = u4;
        uint32_t u5 = load32_be(b + (uint32_t)24U);
        ws[6U] = u5;
        uint32_t u6 = load32_be(b + (uint32_t)28U);
        ws[7U] = u6;
        uint32_t u7 = load32_be(b + (uint32_t)32U);
        ws[8U] = u7;
        uint32_t u8 = load32_be(b + (uint32_t)36U);
        ws[9U] = u8;
        uint32_t u9 = load32_be(b + (uint32_t)40U);
        ws[10U] = u9;
        uint32_t u10 = load32_be(b + (uint32_t)44U);
        ws[11U] = u10;
        uint32_t u11 = load32_be(b + (uint32_t)48U);
        ws[12U] = u11;
        uint32_t u12 = load32_be(b + (uint32_t)52U);
        ws[13U] = u12;
        uint32_t u13 = load32_be(b + (uint32_t)56U);
        ws[14U] = u13;
        uint32_t u14 = load32_be(b + (uint32_t)60U);
        ws[15U] = u14;
        KRML_MAYBE_FOR4(i1,
          (uint32_t)0U,
          (uint32_t)4U,
          (uint32_t)1U,
          KRML_MAYBE_FOR16(i,
            (uint32_t)0U,
            (uint32_t)16U,
            (uint32_t)1U,
            uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i1 + i];
            uint32_t ws_t = ws[i];
            uint32_t a0 = block_state1[0U];
            uint32_t b0 = block_state1[1U];
            uint32_t c0 = block_state1[2U];
            uint32_t d0 = block_state1[3U];
            uint32_t e0 = block_state1[4U];
            uint32_t f0 = block_state1[5U];
            uint32_t g0 = block_state1[6U];
            uint32_t h06 = block_state1[7U];
            uint32_t k_e_t = k_t;
            uint32_t
            t1 =
              h06
              +
                ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
                ^
                  ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
                  ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
              + ((e0 & f0) ^ (~e0 & g0))
              + k_e_t
              + ws_t;
            uint32_t
            t2 =
              ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
              ^
                ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
                ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
              + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
            uint32_t a1 = t1 + t2;
            uint32_t b1 = a0;
            uint32_t c1 = b0;
            uint32_t d1 = c0;
            uint32_t e1 = d0 + t1;
            uint32_t f1 = e0;
            uint32_t g1 = f0;
            uint32_t h13 = g0;
            block_state1[0U] = a1;
            block_state1[1U] = b1;
            block_state1[2U] = c1;
            block_state1[3U] = d1;
            block_state1[4U] = e1;
            block_state1[5U] = f1;
            block_state1[6U] = g1;
            block_state1[7U] = h13;);
          if (i1 < (uint32_t)3U)
          {
            KRML_MAYBE_FOR16(i,
              (uint32_t)0U,
              (uint32_t)16U,
              (uint32_t)1U,
              uint32_t t16 = ws[i];
              uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
              uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
              uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
              uint32_t
              s11 =
                (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
                ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
              uint32_t
              s0 =
                (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
                ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
              ws[i] = s11 + t7 + s0 + t16;);
          });
        KRML_MAYBE_FOR8(i,
          (uint32_t)0U,
          (uint32_t)8U,
          (uint32_t)1U,
          uint32_t *os = block_state1;
          uint32_t x = block_state1[i] + hash_old[i];
          os[i] = x;);
      }
    }
    uint32_t ite;
    if
    (
      (uint64_t)(len - diff)
      % (uint64_t)(uint32_t)64U
      == (uint64_t)0U
      && (uint64_t)(len - diff) > (uint64_t)0U
    )
    {
      ite = (uint32_t)64U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)(len - diff) % (uint64_t)(uint32_t)64U);
    }
    uint32_t n_blocks = (len - diff - ite) / (uint32_t)64U;
    uint32_t data1_len = n_blocks * (uint32_t)64U;
    uint32_t data2_len = len - diff - data1_len;
    uint8_t *data11 = data2;
    uint8_t *data21 = data2 + data1_len;
    uint32_t blocks = data1_len / (uint32_t)64U;
    for (uint32_t i0 = (uint32_t)0U; i0 < blocks; i0++)
    {
      uint8_t *b00 = data11;
      uint8_t *mb = b00 + i0 * (uint32_t)64U;
      uint32_t hash_old[8U] = { 0U };
      uint32_t ws[16U] = { 0U };
      memcpy(hash_old, block_state1, (uint32_t)8U * sizeof (uint32_t));
      uint8_t *b = mb;
      uint32_t u = load32_be(b);
      ws[0U] = u;
      uint32_t u0 = load32_be(b + (uint32_t)4U);
      ws[1U] = u0;
      uint32_t u1 = load32_be(b + (uint32_t)8U);
      ws[2U] = u1;
      uint32_t u2 = load32_be(b + (uint32_t)12U);
      ws[3U] = u2;
      uint32_t u3 = load32_be(b + (uint32_t)16U);
      ws[4U] = u3;
      uint32_t u4 = load32_be(b + (uint32_t)20U);
      ws[5U] = u4;
      uint32_t u5 = load32_be(b + (uint32_t)24U);
      ws[6U] = u5;
      uint32_t u6 = load32_be(b + (uint32_t)28U);
      ws[7U] = u6;
      uint32_t u7 = load32_be(b + (uint32_t)32U);
      ws[8U] = u7;
      uint32_t u8 = load32_be(b + (uint32_t)36U);
      ws[9U] = u8;
      uint32_t u9 = load32_be(b + (uint32_t)40U);
      ws[10U] = u9;
      uint32_t u10 = load32_be(b + (uint32_t)44U);
      ws[11U] = u10;
      uint32_t u11 = load32_be(b + (uint32_t)48U);
      ws[12U] = u11;
      uint32_t u12 = load32_be(b + (uint32_t)52U);
      ws[13U] = u12;
      uint32_t u13 = load32_be(b + (uint32_t)56U);
      ws[14U] = u13;
      uint32_t u14 = load32_be(b + (uint32_t)60U);
      ws[15U] = u14;
      KRML_MAYBE_FOR4(i1,
        (uint32_t)0U,
        (uint32_t)4U,
        (uint32_t)1U,
        KRML_MAYBE_FOR16(i,
          (uint32_t)0U,
          (uint32_t)16U,
          (uint32_t)1U,
          uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i1 + i];
          uint32_t ws_t = ws[i];
          uint32_t a0 = block_state1[0U];
          uint32_t b0 = block_state1[1U];
          uint32_t c0 = block_state1[2U];
          uint32_t d0 = block_state1[3U];
          uint32_t e0 = block_state1[4U];
          uint32_t f0 = block_state1[5U];
          uint32_t g0 = block_state1[6U];
          uint32_t h06 = block_state1[7U];
          uint32_t k_e_t = k_t;
          uint32_t
          t1 =
            h06
            +
              ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
              ^
                ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
                ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
            + ((e0 & f0) ^ (~e0 & g0))
            + k_e_t
            + ws_t;
          uint32_t
          t2 =
            ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
            ^
              ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
              ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
            + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
          uint32_t a1 = t1 + t2;
          uint32_t b1 = a0;
          uint32_t c1 = b0;
          uint32_t d1 = c0;
          uint32_t e1 = d0 + t1;
          uint32_t f1 = e0;
          uint32_t g1 = f0;
          uint32_t h14 = g0;
          block_state1[0U] = a1;
          block_state1[1U] = b1;
          block_state1[2U] = c1;
          block_state1[3U] = d1;
          block_state1[4U] = e1;
          block_state1[5U] = f1;
          block_state1[6U] = g1;
          block_state1[7U] = h14;);
        if (i1 < (uint32_t)3U)
        {
          KRML_MAYBE_FOR16(i,
            (uint32_t)0U,
            (uint32_t)16U,
            (uint32_t)1U,
            uint32_t t16 = ws[i];
            uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
            uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
            uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
            uint32_t
            s11 =
              (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
              ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
            uint32_t
            s0 =
              (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
              ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
            ws[i] = s11 + t7 + s0 + t16;);
        });
      KRML_MAYBE_FOR8(i,
        (uint32_t)0U,
        (uint32_t)8U,
        (uint32_t)1U,
        uint32_t *os = block_state1;
        uint32_t x = block_state1[i] + hash_old[i];
        os[i] = x;);
    }
    uint8_t *dst = buf;
    memcpy(dst, data21, data2_len * sizeof (uint8_t));
    *p
    =
      (
        (Hacl_Streaming_SHA2_state_sha2_224){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)(len - diff)
        }
      );
  }
  return (uint32_t)0U;
}

void Hacl_Streaming_SHA2_finish_256(Hacl_Streaming_SHA2_state_sha2_224 *p, uint8_t *dst)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *p;
  uint32_t *block_state = scrut.block_state;
  uint8_t *buf_ = scrut.buf;
  uint64_t total_len = scrut.total_len;
  uint32_t r;
  if (total_len % (uint64_t)(uint32_t)64U == (uint64_t)0U && total_len > (uint64_t)0U)
  {
    r = (uint32_t)64U;
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)(uint32_t)64U);
  }
  uint8_t *buf_1 = buf_;
  uint32_t tmp_block_state[8U] = { 0U };
  memcpy(tmp_block_state, block_state, (uint32_t)8U * sizeof (uint32_t));
  uint32_t ite;
  if (r % (uint32_t)64U == (uint32_t)0U && r > (uint32_t)0U)
  {
    ite = (uint32_t)64U;
  }
  else
  {
    ite = r % (uint32_t)64U;
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  uint32_t blocks0 = (uint32_t)0U;
  for (uint32_t i0 = (uint32_t)0U; i0 < blocks0; i0++)
  {
    uint8_t *b00 = buf_multi;
    uint8_t *mb = b00 + i0 * (uint32_t)64U;
    uint32_t hash_old[8U] = { 0U };
    uint32_t ws[16U] = { 0U };
    memcpy(hash_old, tmp_block_state, (uint32_t)8U * sizeof (uint32_t));
    uint8_t *b = mb;
    uint32_t u = load32_be(b);
    ws[0U] = u;
    uint32_t u0 = load32_be(b + (uint32_t)4U);
    ws[1U] = u0;
    uint32_t u1 = load32_be(b + (uint32_t)8U);
    ws[2U] = u1;
    uint32_t u2 = load32_be(b + (uint32_t)12U);
    ws[3U] = u2;
    uint32_t u3 = load32_be(b + (uint32_t)16U);
    ws[4U] = u3;
    uint32_t u4 = load32_be(b + (uint32_t)20U);
    ws[5U] = u4;
    uint32_t u5 = load32_be(b + (uint32_t)24U);
    ws[6U] = u5;
    uint32_t u6 = load32_be(b + (uint32_t)28U);
    ws[7U] = u6;
    uint32_t u7 = load32_be(b + (uint32_t)32U);
    ws[8U] = u7;
    uint32_t u8 = load32_be(b + (uint32_t)36U);
    ws[9U] = u8;
    uint32_t u9 = load32_be(b + (uint32_t)40U);
    ws[10U] = u9;
    uint32_t u10 = load32_be(b + (uint32_t)44U);
    ws[11U] = u10;
    uint32_t u11 = load32_be(b + (uint32_t)48U);
    ws[12U] = u11;
    uint32_t u12 = load32_be(b + (uint32_t)52U);
    ws[13U] = u12;
    uint32_t u13 = load32_be(b + (uint32_t)56U);
    ws[14U] = u13;
    uint32_t u14 = load32_be(b + (uint32_t)60U);
    ws[15U] = u14;
    KRML_MAYBE_FOR4(i1,
      (uint32_t)0U,
      (uint32_t)4U,
      (uint32_t)1U,
      KRML_MAYBE_FOR16(i,
        (uint32_t)0U,
        (uint32_t)16U,
        (uint32_t)1U,
        uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i1 + i];
        uint32_t ws_t = ws[i];
        uint32_t a0 = tmp_block_state[0U];
        uint32_t b0 = tmp_block_state[1U];
        uint32_t c0 = tmp_block_state[2U];
        uint32_t d0 = tmp_block_state[3U];
        uint32_t e0 = tmp_block_state[4U];
        uint32_t f0 = tmp_block_state[5U];
        uint32_t g0 = tmp_block_state[6U];
        uint32_t h05 = tmp_block_state[7U];
        uint32_t k_e_t = k_t;
        uint32_t
        t1 =
          h05
          +
            ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
            ^
              ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
              ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
          + ((e0 & f0) ^ (~e0 & g0))
          + k_e_t
          + ws_t;
        uint32_t
        t2 =
          ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
          ^
            ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
            ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
          + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
        uint32_t a1 = t1 + t2;
        uint32_t b1 = a0;
        uint32_t c1 = b0;
        uint32_t d1 = c0;
        uint32_t e1 = d0 + t1;
        uint32_t f1 = e0;
        uint32_t g1 = f0;
        uint32_t h13 = g0;
        tmp_block_state[0U] = a1;
        tmp_block_state[1U] = b1;
        tmp_block_state[2U] = c1;
        tmp_block_state[3U] = d1;
        tmp_block_state[4U] = e1;
        tmp_block_state[5U] = f1;
        tmp_block_state[6U] = g1;
        tmp_block_state[7U] = h13;);
      if (i1 < (uint32_t)3U)
      {
        KRML_MAYBE_FOR16(i,
          (uint32_t)0U,
          (uint32_t)16U,
          (uint32_t)1U,
          uint32_t t16 = ws[i];
          uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
          uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
          uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
          uint32_t
          s1 =
            (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
            ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
          uint32_t
          s0 =
            (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
            ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
          ws[i] = s1 + t7 + s0 + t16;);
      });
    KRML_MAYBE_FOR8(i,
      (uint32_t)0U,
      (uint32_t)8U,
      (uint32_t)1U,
      uint32_t *os = tmp_block_state;
      uint32_t x = tmp_block_state[i] + hash_old[i];
      os[i] = x;);
  }
  uint64_t prev_len_last = total_len - (uint64_t)r;
  uint32_t blocks;
  if (r + (uint32_t)8U + (uint32_t)1U <= (uint32_t)64U)
  {
    blocks = (uint32_t)1U;
  }
  else
  {
    blocks = (uint32_t)2U;
  }
  uint32_t fin = blocks * (uint32_t)64U;
  uint8_t last[128U] = { 0U };
  uint8_t totlen_buf[8U] = { 0U };
  uint64_t total_len_bits = (prev_len_last + (uint64_t)r) << (uint32_t)3U;
  store64_be(totlen_buf, total_len_bits);
  uint8_t *b00 = buf_last;
  memcpy(last, b00, r * sizeof (uint8_t));
  last[r] = (uint8_t)0x80U;
  memcpy(last + fin - (uint32_t)8U, totlen_buf, (uint32_t)8U * sizeof (uint8_t));
  uint8_t *last00 = last;
  uint8_t *last10 = last + (uint32_t)64U;
  Hacl_Impl_SHA2_Types_uint8_2p scrut0 = { .fst = last00, .snd = last10 };
  uint8_t *l0 = scrut0.fst;
  uint8_t *l1 = scrut0.snd;
  uint8_t *lb0 = l0;
  uint8_t *lb1 = l1;
  Hacl_Impl_SHA2_Types_uint8_2p scrut1 = { .fst = lb0, .snd = lb1 };
  uint8_t *last0 = scrut1.fst;
  uint8_t *last1 = scrut1.snd;
  uint32_t hash_old[8U] = { 0U };
  uint32_t ws0[16U] = { 0U };
  memcpy(hash_old, tmp_block_state, (uint32_t)8U * sizeof (uint32_t));
  uint8_t *b2 = last0;
  uint32_t u0 = load32_be(b2);
  ws0[0U] = u0;
  uint32_t u1 = load32_be(b2 + (uint32_t)4U);
  ws0[1U] = u1;
  uint32_t u2 = load32_be(b2 + (uint32_t)8U);
  ws0[2U] = u2;
  uint32_t u3 = load32_be(b2 + (uint32_t)12U);
  ws0[3U] = u3;
  uint32_t u4 = load32_be(b2 + (uint32_t)16U);
  ws0[4U] = u4;
  uint32_t u5 = load32_be(b2 + (uint32_t)20U);
  ws0[5U] = u5;
  uint32_t u6 = load32_be(b2 + (uint32_t)24U);
  ws0[6U] = u6;
  uint32_t u7 = load32_be(b2 + (uint32_t)28U);
  ws0[7U] = u7;
  uint32_t u8 = load32_be(b2 + (uint32_t)32U);
  ws0[8U] = u8;
  uint32_t u9 = load32_be(b2 + (uint32_t)36U);
  ws0[9U] = u9;
  uint32_t u10 = load32_be(b2 + (uint32_t)40U);
  ws0[10U] = u10;
  uint32_t u11 = load32_be(b2 + (uint32_t)44U);
  ws0[11U] = u11;
  uint32_t u12 = load32_be(b2 + (uint32_t)48U);
  ws0[12U] = u12;
  uint32_t u13 = load32_be(b2 + (uint32_t)52U);
  ws0[13U] = u13;
  uint32_t u14 = load32_be(b2 + (uint32_t)56U);
  ws0[14U] = u14;
  uint32_t u15 = load32_be(b2 + (uint32_t)60U);
  ws0[15U] = u15;
  KRML_MAYBE_FOR4(i0,
    (uint32_t)0U,
    (uint32_t)4U,
    (uint32_t)1U,
    KRML_MAYBE_FOR16(i,
      (uint32_t)0U,
      (uint32_t)16U,
      (uint32_t)1U,
      uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i0 + i];
      uint32_t ws_t = ws0[i];
      uint32_t a0 = tmp_block_state[0U];
      uint32_t b0 = tmp_block_state[1U];
      uint32_t c0 = tmp_block_state[2U];
      uint32_t d0 = tmp_block_state[3U];
      uint32_t e0 = tmp_block_state[4U];
      uint32_t f0 = tmp_block_state[5U];
      uint32_t g0 = tmp_block_state[6U];
      uint32_t h05 = tmp_block_state[7U];
      uint32_t k_e_t = k_t;
      uint32_t
      t1 =
        h05
        +
          ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
          ^
            ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
            ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
        + ((e0 & f0) ^ (~e0 & g0))
        + k_e_t
        + ws_t;
      uint32_t
      t2 =
        ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
        ^
          ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
          ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
        + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
      uint32_t a1 = t1 + t2;
      uint32_t b1 = a0;
      uint32_t c1 = b0;
      uint32_t d1 = c0;
      uint32_t e1 = d0 + t1;
      uint32_t f1 = e0;
      uint32_t g1 = f0;
      uint32_t h14 = g0;
      tmp_block_state[0U] = a1;
      tmp_block_state[1U] = b1;
      tmp_block_state[2U] = c1;
      tmp_block_state[3U] = d1;
      tmp_block_state[4U] = e1;
      tmp_block_state[5U] = f1;
      tmp_block_state[6U] = g1;
      tmp_block_state[7U] = h14;);
    if (i0 < (uint32_t)3U)
    {
      KRML_MAYBE_FOR16(i,
        (uint32_t)0U,
        (uint32_t)16U,
        (uint32_t)1U,
        uint32_t t16 = ws0[i];
        uint32_t t15 = ws0[(i + (uint32_t)1U) % (uint32_t)16U];
        uint32_t t7 = ws0[(i + (uint32_t)9U) % (uint32_t)16U];
        uint32_t t2 = ws0[(i + (uint32_t)14U) % (uint32_t)16U];
        uint32_t
        s1 =
          (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
          ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
        uint32_t
        s0 =
          (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
          ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
        ws0[i] = s1 + t7 + s0 + t16;);
    });
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = tmp_block_state;
    uint32_t x = tmp_block_state[i] + hash_old[i];
    os[i] = x;);
  if (blocks > (uint32_t)1U)
  {
    uint32_t hash_old0[8U] = { 0U };
    uint32_t ws[16U] = { 0U };
    memcpy(hash_old0, tmp_block_state, (uint32_t)8U * sizeof (uint32_t));
    uint8_t *b = last1;
    uint32_t u = load32_be(b);
    ws[0U] = u;
    uint32_t u16 = load32_be(b + (uint32_t)4U);
    ws[1U] = u16;
    uint32_t u17 = load32_be(b + (uint32_t)8U);
    ws[2U] = u17;
    uint32_t u18 = load32_be(b + (uint32_t)12U);
    ws[3U] = u18;
    uint32_t u19 = load32_be(b + (uint32_t)16U);
    ws[4U] = u19;
    uint32_t u20 = load32_be(b + (uint32_t)20U);
    ws[5U] = u20;
    uint32_t u21 = load32_be(b + (uint32_t)24U);
    ws[6U] = u21;
    uint32_t u22 = load32_be(b + (uint32_t)28U);
    ws[7U] = u22;
    uint32_t u23 = load32_be(b + (uint32_t)32U);
    ws[8U] = u23;
    uint32_t u24 = load32_be(b + (uint32_t)36U);
    ws[9U] = u24;
    uint32_t u25 = load32_be(b + (uint32_t)40U);
    ws[10U] = u25;
    uint32_t u26 = load32_be(b + (uint32_t)44U);
    ws[11U] = u26;
    uint32_t u27 = load32_be(b + (uint32_t)48U);
    ws[12U] = u27;
    uint32_t u28 = load32_be(b + (uint32_t)52U);
    ws[13U] = u28;
    uint32_t u29 = load32_be(b + (uint32_t)56U);
    ws[14U] = u29;
    uint32_t u30 = load32_be(b + (uint32_t)60U);
    ws[15U] = u30;
    KRML_MAYBE_FOR4(i0,
      (uint32_t)0U,
      (uint32_t)4U,
      (uint32_t)1U,
      KRML_MAYBE_FOR16(i,
        (uint32_t)0U,
        (uint32_t)16U,
        (uint32_t)1U,
        uint32_t k_t = Hacl_Impl_SHA2_Generic_k224_256[(uint32_t)16U * i0 + i];
        uint32_t ws_t = ws[i];
        uint32_t a0 = tmp_block_state[0U];
        uint32_t b0 = tmp_block_state[1U];
        uint32_t c0 = tmp_block_state[2U];
        uint32_t d0 = tmp_block_state[3U];
        uint32_t e0 = tmp_block_state[4U];
        uint32_t f0 = tmp_block_state[5U];
        uint32_t g0 = tmp_block_state[6U];
        uint32_t h05 = tmp_block_state[7U];
        uint32_t k_e_t = k_t;
        uint32_t
        t1 =
          h05
          +
            ((e0 << (uint32_t)26U | e0 >> (uint32_t)6U)
            ^
              ((e0 << (uint32_t)21U | e0 >> (uint32_t)11U)
              ^ (e0 << (uint32_t)7U | e0 >> (uint32_t)25U)))
          + ((e0 & f0) ^ (~e0 & g0))
          + k_e_t
          + ws_t;
        uint32_t
        t2 =
          ((a0 << (uint32_t)30U | a0 >> (uint32_t)2U)
          ^
            ((a0 << (uint32_t)19U | a0 >> (uint32_t)13U)
            ^ (a0 << (uint32_t)10U | a0 >> (uint32_t)22U)))
          + ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
        uint32_t a1 = t1 + t2;
        uint32_t b1 = a0;
        uint32_t c1 = b0;
        uint32_t d1 = c0;
        uint32_t e1 = d0 + t1;
        uint32_t f1 = e0;
        uint32_t g1 = f0;
        uint32_t h14 = g0;
        tmp_block_state[0U] = a1;
        tmp_block_state[1U] = b1;
        tmp_block_state[2U] = c1;
        tmp_block_state[3U] = d1;
        tmp_block_state[4U] = e1;
        tmp_block_state[5U] = f1;
        tmp_block_state[6U] = g1;
        tmp_block_state[7U] = h14;);
      if (i0 < (uint32_t)3U)
      {
        KRML_MAYBE_FOR16(i,
          (uint32_t)0U,
          (uint32_t)16U,
          (uint32_t)1U,
          uint32_t t16 = ws[i];
          uint32_t t15 = ws[(i + (uint32_t)1U) % (uint32_t)16U];
          uint32_t t7 = ws[(i + (uint32_t)9U) % (uint32_t)16U];
          uint32_t t2 = ws[(i + (uint32_t)14U) % (uint32_t)16U];
          uint32_t
          s1 =
            (t2 << (uint32_t)15U | t2 >> (uint32_t)17U)
            ^ ((t2 << (uint32_t)13U | t2 >> (uint32_t)19U) ^ t2 >> (uint32_t)10U);
          uint32_t
          s0 =
            (t15 << (uint32_t)25U | t15 >> (uint32_t)7U)
            ^ ((t15 << (uint32_t)14U | t15 >> (uint32_t)18U) ^ t15 >> (uint32_t)3U);
          ws[i] = s1 + t7 + s0 + t16;);
      });
    KRML_MAYBE_FOR8(i,
      (uint32_t)0U,
      (uint32_t)8U,
      (uint32_t)1U,
      uint32_t *os = tmp_block_state;
      uint32_t x = tmp_block_state[i] + hash_old0[i];
      os[i] = x;);
  }
  uint8_t hbuf[32U] = { 0U };
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    store32_be(hbuf + i * (uint32_t)4U, tmp_block_state[i]););
  memcpy(dst, hbuf, (uint32_t)32U * sizeof (uint8_t));
}

void Hacl_Streaming_SHA2_free_256(Hacl_Streaming_SHA2_state_sha2_224 *s)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_HOST_FREE(block_state);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(s);
}
