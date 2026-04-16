/* MIT License
 *
 * Copyright (c) 2016-2022 INRIA, CMU and Microsoft Corporation
 * Copyright (c) 2022-2023 HACL* Contributors
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


#include "internal/Hacl_Hash_SHA3.h"

#include "Hacl_Streaming_Types.h"
#include "internal/Hacl_Streaming_Types.h"

const
uint32_t
Hacl_Hash_SHA3_keccak_rotc[24U] =
  {
    1U, 3U, 6U, 10U, 15U, 21U, 28U, 36U, 45U, 55U, 2U, 14U, 27U, 41U, 56U, 8U, 25U, 43U, 62U, 18U,
    39U, 61U, 20U, 44U
  };

const
uint32_t
Hacl_Hash_SHA3_keccak_piln[24U] =
  {
    10U, 7U, 11U, 17U, 18U, 3U, 5U, 16U, 8U, 21U, 24U, 4U, 15U, 23U, 19U, 13U, 12U, 2U, 20U, 14U,
    22U, 9U, 6U, 1U
  };

const
uint64_t
Hacl_Hash_SHA3_keccak_rndc[24U] =
  {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
  };

static void absorb_inner_32(uint8_t *b, uint64_t *s)
{
  uint64_t ws[32U] = { 0U };
  uint8_t *b1 = b;
  uint64_t u = load64_le(b1);
  ws[0U] = u;
  uint64_t u0 = load64_le(b1 + 8U);
  ws[1U] = u0;
  uint64_t u1 = load64_le(b1 + 16U);
  ws[2U] = u1;
  uint64_t u2 = load64_le(b1 + 24U);
  ws[3U] = u2;
  uint64_t u3 = load64_le(b1 + 32U);
  ws[4U] = u3;
  uint64_t u4 = load64_le(b1 + 40U);
  ws[5U] = u4;
  uint64_t u5 = load64_le(b1 + 48U);
  ws[6U] = u5;
  uint64_t u6 = load64_le(b1 + 56U);
  ws[7U] = u6;
  uint64_t u7 = load64_le(b1 + 64U);
  ws[8U] = u7;
  uint64_t u8 = load64_le(b1 + 72U);
  ws[9U] = u8;
  uint64_t u9 = load64_le(b1 + 80U);
  ws[10U] = u9;
  uint64_t u10 = load64_le(b1 + 88U);
  ws[11U] = u10;
  uint64_t u11 = load64_le(b1 + 96U);
  ws[12U] = u11;
  uint64_t u12 = load64_le(b1 + 104U);
  ws[13U] = u12;
  uint64_t u13 = load64_le(b1 + 112U);
  ws[14U] = u13;
  uint64_t u14 = load64_le(b1 + 120U);
  ws[15U] = u14;
  uint64_t u15 = load64_le(b1 + 128U);
  ws[16U] = u15;
  uint64_t u16 = load64_le(b1 + 136U);
  ws[17U] = u16;
  uint64_t u17 = load64_le(b1 + 144U);
  ws[18U] = u17;
  uint64_t u18 = load64_le(b1 + 152U);
  ws[19U] = u18;
  uint64_t u19 = load64_le(b1 + 160U);
  ws[20U] = u19;
  uint64_t u20 = load64_le(b1 + 168U);
  ws[21U] = u20;
  uint64_t u21 = load64_le(b1 + 176U);
  ws[22U] = u21;
  uint64_t u22 = load64_le(b1 + 184U);
  ws[23U] = u22;
  uint64_t u23 = load64_le(b1 + 192U);
  ws[24U] = u23;
  uint64_t u24 = load64_le(b1 + 200U);
  ws[25U] = u24;
  uint64_t u25 = load64_le(b1 + 208U);
  ws[26U] = u25;
  uint64_t u26 = load64_le(b1 + 216U);
  ws[27U] = u26;
  uint64_t u27 = load64_le(b1 + 224U);
  ws[28U] = u27;
  uint64_t u28 = load64_le(b1 + 232U);
  ws[29U] = u28;
  uint64_t u29 = load64_le(b1 + 240U);
  ws[30U] = u29;
  uint64_t u30 = load64_le(b1 + 248U);
  ws[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws[i];
  }
  for (uint32_t i0 = 0U; i0 < 24U; i0++)
  {
    uint64_t _C[5U] = { 0U };
    KRML_MAYBE_FOR5(i,
      0U,
      5U,
      1U,
      _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
    KRML_MAYBE_FOR5(i1,
      0U,
      5U,
      1U,
      uint64_t uu____0 = _C[(i1 + 1U) % 5U];
      uint64_t _D = _C[(i1 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
      KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i1 + 5U * i] = s[i1 + 5U * i] ^ _D;););
    uint64_t x = s[1U];
    uint64_t current = x;
    for (uint32_t i = 0U; i < 24U; i++)
    {
      uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
      uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
      uint64_t temp = s[_Y];
      uint64_t uu____1 = current;
      s[_Y] = uu____1 << r | uu____1 >> (64U - r);
      current = temp;
    }
    KRML_MAYBE_FOR5(i,
      0U,
      5U,
      1U,
      uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
      uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
      uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
      uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
      uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
      s[0U + 5U * i] = v0;
      s[1U + 5U * i] = v1;
      s[2U + 5U * i] = v2;
      s[3U + 5U * i] = v3;
      s[4U + 5U * i] = v4;);
    uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i0];
    s[0U] = s[0U] ^ c;
  }
}

static uint32_t block_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_SHA3_224:
      {
        return 144U;
      }
    case Spec_Hash_Definitions_SHA3_256:
      {
        return 136U;
      }
    case Spec_Hash_Definitions_SHA3_384:
      {
        return 104U;
      }
    case Spec_Hash_Definitions_SHA3_512:
      {
        return 72U;
      }
    case Spec_Hash_Definitions_Shake128:
      {
        return 168U;
      }
    case Spec_Hash_Definitions_Shake256:
      {
        return 136U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static uint32_t hash_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_SHA3_224:
      {
        return 28U;
      }
    case Spec_Hash_Definitions_SHA3_256:
      {
        return 32U;
      }
    case Spec_Hash_Definitions_SHA3_384:
      {
        return 48U;
      }
    case Spec_Hash_Definitions_SHA3_512:
      {
        return 64U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

void Hacl_Hash_SHA3_init_(Spec_Hash_Definitions_hash_alg a, uint64_t *s)
{
  KRML_MAYBE_UNUSED_VAR(a);
  memset(s, 0U, 25U * sizeof (uint64_t));
}

void
Hacl_Hash_SHA3_update_multi_sha3(
  Spec_Hash_Definitions_hash_alg a,
  uint64_t *s,
  uint8_t *blocks,
  uint32_t n_blocks
)
{
  uint32_t l = block_len(a) * n_blocks;
  for (uint32_t i = 0U; i < l / block_len(a); i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = blocks;
    uint8_t *bl0 = b_;
    uint8_t *uu____0 = b0 + i * block_len(a);
    memcpy(bl0, uu____0, block_len(a) * sizeof (uint8_t));
    uint32_t unused = block_len(a);
    KRML_MAYBE_UNUSED_VAR(unused);
    absorb_inner_32(b_, s);
  }
}

void
Hacl_Hash_SHA3_update_last_sha3(
  Spec_Hash_Definitions_hash_alg a,
  uint64_t *s,
  uint8_t *input,
  uint32_t input_len
)
{
  uint8_t suffix;
  if (a == Spec_Hash_Definitions_Shake128 || a == Spec_Hash_Definitions_Shake256)
  {
    suffix = 0x1fU;
  }
  else
  {
    suffix = 0x06U;
  }
  uint32_t len = block_len(a);
  if (input_len == len)
  {
    uint8_t b1[256U] = { 0U };
    uint8_t *b_ = b1;
    uint8_t *b00 = input;
    uint8_t *bl00 = b_;
    memcpy(bl00, b00 + 0U * len, len * sizeof (uint8_t));
    absorb_inner_32(b_, s);
    uint8_t b2[256U] = { 0U };
    uint8_t *b_0 = b2;
    uint32_t rem = 0U % len;
    uint8_t *b01 = input + input_len;
    uint8_t *bl0 = b_0;
    memcpy(bl0, b01 + 0U - rem, rem * sizeof (uint8_t));
    uint8_t *b02 = b_0;
    b02[0U % len] = suffix;
    uint64_t ws[32U] = { 0U };
    uint8_t *b = b_0;
    uint64_t u = load64_le(b);
    ws[0U] = u;
    uint64_t u0 = load64_le(b + 8U);
    ws[1U] = u0;
    uint64_t u1 = load64_le(b + 16U);
    ws[2U] = u1;
    uint64_t u2 = load64_le(b + 24U);
    ws[3U] = u2;
    uint64_t u3 = load64_le(b + 32U);
    ws[4U] = u3;
    uint64_t u4 = load64_le(b + 40U);
    ws[5U] = u4;
    uint64_t u5 = load64_le(b + 48U);
    ws[6U] = u5;
    uint64_t u6 = load64_le(b + 56U);
    ws[7U] = u6;
    uint64_t u7 = load64_le(b + 64U);
    ws[8U] = u7;
    uint64_t u8 = load64_le(b + 72U);
    ws[9U] = u8;
    uint64_t u9 = load64_le(b + 80U);
    ws[10U] = u9;
    uint64_t u10 = load64_le(b + 88U);
    ws[11U] = u10;
    uint64_t u11 = load64_le(b + 96U);
    ws[12U] = u11;
    uint64_t u12 = load64_le(b + 104U);
    ws[13U] = u12;
    uint64_t u13 = load64_le(b + 112U);
    ws[14U] = u13;
    uint64_t u14 = load64_le(b + 120U);
    ws[15U] = u14;
    uint64_t u15 = load64_le(b + 128U);
    ws[16U] = u15;
    uint64_t u16 = load64_le(b + 136U);
    ws[17U] = u16;
    uint64_t u17 = load64_le(b + 144U);
    ws[18U] = u17;
    uint64_t u18 = load64_le(b + 152U);
    ws[19U] = u18;
    uint64_t u19 = load64_le(b + 160U);
    ws[20U] = u19;
    uint64_t u20 = load64_le(b + 168U);
    ws[21U] = u20;
    uint64_t u21 = load64_le(b + 176U);
    ws[22U] = u21;
    uint64_t u22 = load64_le(b + 184U);
    ws[23U] = u22;
    uint64_t u23 = load64_le(b + 192U);
    ws[24U] = u23;
    uint64_t u24 = load64_le(b + 200U);
    ws[25U] = u24;
    uint64_t u25 = load64_le(b + 208U);
    ws[26U] = u25;
    uint64_t u26 = load64_le(b + 216U);
    ws[27U] = u26;
    uint64_t u27 = load64_le(b + 224U);
    ws[28U] = u27;
    uint64_t u28 = load64_le(b + 232U);
    ws[29U] = u28;
    uint64_t u29 = load64_le(b + 240U);
    ws[30U] = u29;
    uint64_t u30 = load64_le(b + 248U);
    ws[31U] = u30;
    for (uint32_t i = 0U; i < 25U; i++)
    {
      s[i] = s[i] ^ ws[i];
    }
    if (!(((uint32_t)suffix & 0x80U) == 0U) && 0U % len == len - 1U)
    {
      for (uint32_t i0 = 0U; i0 < 24U; i0++)
      {
        uint64_t _C[5U] = { 0U };
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
        KRML_MAYBE_FOR5(i1,
          0U,
          5U,
          1U,
          uint64_t uu____0 = _C[(i1 + 1U) % 5U];
          uint64_t _D = _C[(i1 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
          KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i1 + 5U * i] = s[i1 + 5U * i] ^ _D;););
        uint64_t x = s[1U];
        uint64_t current = x;
        for (uint32_t i = 0U; i < 24U; i++)
        {
          uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
          uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
          uint64_t temp = s[_Y];
          uint64_t uu____1 = current;
          s[_Y] = uu____1 << r | uu____1 >> (64U - r);
          current = temp;
        }
        KRML_MAYBE_FOR5(i,
          0U,
          5U,
          1U,
          uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
          uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
          uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
          uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
          uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
          s[0U + 5U * i] = v0;
          s[1U + 5U * i] = v1;
          s[2U + 5U * i] = v2;
          s[3U + 5U * i] = v3;
          s[4U + 5U * i] = v4;);
        uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i0];
        s[0U] = s[0U] ^ c;
      }
    }
    uint8_t b3[256U] = { 0U };
    uint8_t *b4 = b3;
    uint8_t *b0 = b4;
    b0[len - 1U] = 0x80U;
    absorb_inner_32(b4, s);
    return;
  }
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = input_len % len;
  uint8_t *b00 = input;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + input_len - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[input_len % len] = suffix;
  uint64_t ws[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws[i];
  }
  if (!(((uint32_t)suffix & 0x80U) == 0U) && input_len % len == len - 1U)
  {
    for (uint32_t i0 = 0U; i0 < 24U; i0++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i1,
        0U,
        5U,
        1U,
        uint64_t uu____2 = _C[(i1 + 1U) % 5U];
        uint64_t _D = _C[(i1 + 4U) % 5U] ^ (uu____2 << 1U | uu____2 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i1 + 5U * i] = s[i1 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____3 = current;
        s[_Y] = uu____3 << r | uu____3 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i0];
      s[0U] = s[0U] ^ c;
    }
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[len - 1U] = 0x80U;
  absorb_inner_32(b3, s);
}

static void squeeze(uint64_t *s, uint32_t rateInBytes, uint32_t outputByteLen, uint8_t *b)
{
  for (uint32_t i0 = 0U; i0 < outputByteLen / rateInBytes; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, s, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b0 = b;
    memcpy(b0 + i0 * rateInBytes, hbuf, rateInBytes * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i2 + 5U * i] = s[i2 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____1 = current;
        s[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      s[0U] = s[0U] ^ c;
    }
  }
  uint32_t remOut = outputByteLen % rateInBytes;
  uint8_t hbuf[256U] = { 0U };
  uint64_t ws[32U] = { 0U };
  memcpy(ws, s, 25U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < 32U; i++)
  {
    store64_le(hbuf + i * 8U, ws[i]);
  }
  memcpy(b + outputByteLen - remOut, hbuf, remOut * sizeof (uint8_t));
}

typedef struct hash_buf2_s
{
  Hacl_Hash_SHA3_hash_buf fst;
  Hacl_Hash_SHA3_hash_buf snd;
}
hash_buf2;

Spec_Hash_Definitions_hash_alg Hacl_Hash_SHA3_get_alg(Hacl_Hash_SHA3_state_t *s)
{
  Hacl_Hash_SHA3_hash_buf block_state = (*s).block_state;
  return block_state.fst;
}

typedef struct option___Spec_Hash_Definitions_hash_alg____uint64_t___s
{
  Hacl_Streaming_Types_optional tag;
  Hacl_Hash_SHA3_hash_buf v;
}
option___Spec_Hash_Definitions_hash_alg____uint64_t__;

Hacl_Hash_SHA3_state_t *Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_hash_alg a)
{
  KRML_CHECK_SIZE(sizeof (uint8_t), block_len(a));
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(block_len(a), sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  uint8_t *buf1 = buf;
  uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
  option___Spec_Hash_Definitions_hash_alg____uint64_t__ block_state;
  if (s == NULL)
  {
    block_state =
      ((option___Spec_Hash_Definitions_hash_alg____uint64_t__){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state =
      (
        (option___Spec_Hash_Definitions_hash_alg____uint64_t__){
          .tag = Hacl_Streaming_Types_Some,
          .v = { .fst = a, .snd = s }
        }
      );
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf1);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    Hacl_Hash_SHA3_hash_buf block_state1 = block_state.v;
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Hash_SHA3_state_t
          s0 = { .block_state = block_state1, .buf = buf1, .total_len = (uint64_t)0U };
          Hacl_Hash_SHA3_state_t
          *p = (Hacl_Hash_SHA3_state_t *)KRML_HOST_MALLOC(sizeof (Hacl_Hash_SHA3_state_t));
          if (p != NULL)
          {
            p[0U] = s0;
          }
          if (p == NULL)
          {
            uint64_t *s1 = block_state1.snd;
            KRML_HOST_FREE(s1);
            KRML_HOST_FREE(buf1);
            return NULL;
          }
          Spec_Hash_Definitions_hash_alg a1 = block_state1.fst;
          uint64_t *s1 = block_state1.snd;
          Hacl_Hash_SHA3_init_(a1, s1);
          return p;
        }
      default:
        {
          KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
          KRML_HOST_EXIT(253U);
        }
    }
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

void Hacl_Hash_SHA3_free(Hacl_Hash_SHA3_state_t *state)
{
  Hacl_Hash_SHA3_state_t scrut = *state;
  uint8_t *buf = scrut.buf;
  Hacl_Hash_SHA3_hash_buf block_state = scrut.block_state;
  uint64_t *s = block_state.snd;
  KRML_HOST_FREE(s);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(state);
}

Hacl_Hash_SHA3_state_t *Hacl_Hash_SHA3_copy(Hacl_Hash_SHA3_state_t *state)
{
  Hacl_Hash_SHA3_state_t scrut0 = *state;
  Hacl_Hash_SHA3_hash_buf block_state0 = scrut0.block_state;
  uint8_t *buf0 = scrut0.buf;
  uint64_t total_len0 = scrut0.total_len;
  Spec_Hash_Definitions_hash_alg i = block_state0.fst;
  KRML_CHECK_SIZE(sizeof (uint8_t), block_len(i));
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(block_len(i), sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, buf0, block_len(i) * sizeof (uint8_t));
  uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
  option___Spec_Hash_Definitions_hash_alg____uint64_t__ block_state;
  if (s == NULL)
  {
    block_state =
      ((option___Spec_Hash_Definitions_hash_alg____uint64_t__){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state =
      (
        (option___Spec_Hash_Definitions_hash_alg____uint64_t__){
          .tag = Hacl_Streaming_Types_Some,
          .v = { .fst = i, .snd = s }
        }
      );
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    Hacl_Hash_SHA3_hash_buf block_state1 = block_state.v;
    hash_buf2 scrut = { .fst = block_state0, .snd = block_state1 };
    uint64_t *s_dst = scrut.snd.snd;
    uint64_t *s_src = scrut.fst.snd;
    memcpy(s_dst, s_src, 25U * sizeof (uint64_t));
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Hash_SHA3_state_t
          s0 = { .block_state = block_state1, .buf = buf, .total_len = total_len0 };
          Hacl_Hash_SHA3_state_t
          *p = (Hacl_Hash_SHA3_state_t *)KRML_HOST_MALLOC(sizeof (Hacl_Hash_SHA3_state_t));
          if (p != NULL)
          {
            p[0U] = s0;
          }
          if (p == NULL)
          {
            uint64_t *s1 = block_state1.snd;
            KRML_HOST_FREE(s1);
            KRML_HOST_FREE(buf);
            return NULL;
          }
          return p;
        }
      default:
        {
          KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
          KRML_HOST_EXIT(253U);
        }
    }
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

void Hacl_Hash_SHA3_reset(Hacl_Hash_SHA3_state_t *state)
{
  Hacl_Hash_SHA3_state_t scrut = *state;
  uint8_t *buf = scrut.buf;
  Hacl_Hash_SHA3_hash_buf block_state = scrut.block_state;
  Spec_Hash_Definitions_hash_alg i = block_state.fst;
  KRML_MAYBE_UNUSED_VAR(i);
  Spec_Hash_Definitions_hash_alg a1 = block_state.fst;
  uint64_t *s = block_state.snd;
  Hacl_Hash_SHA3_init_(a1, s);
  Hacl_Hash_SHA3_state_t
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  state[0U] = tmp;
}

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA3_update(Hacl_Hash_SHA3_state_t *state, uint8_t *chunk, uint32_t chunk_len)
{
  Hacl_Hash_SHA3_state_t s = *state;
  Hacl_Hash_SHA3_hash_buf block_state = s.block_state;
  uint64_t total_len = s.total_len;
  Spec_Hash_Definitions_hash_alg i = block_state.fst;
  if ((uint64_t)chunk_len > 0xFFFFFFFFFFFFFFFFULL - total_len)
  {
    return Hacl_Streaming_Types_MaximumLengthExceeded;
  }
  uint32_t sz;
  if (total_len % (uint64_t)block_len(i) == 0ULL && total_len > 0ULL)
  {
    sz = block_len(i);
  }
  else
  {
    sz = (uint32_t)(total_len % (uint64_t)block_len(i));
  }
  if (chunk_len <= block_len(i) - sz)
  {
    Hacl_Hash_SHA3_state_t s1 = *state;
    Hacl_Hash_SHA3_hash_buf block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)block_len(i) == 0ULL && total_len1 > 0ULL)
    {
      sz1 = block_len(i);
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)block_len(i));
    }
    uint8_t *buf2 = buf + sz1;
    memcpy(buf2, chunk, chunk_len * sizeof (uint8_t));
    uint64_t total_len2 = total_len1 + (uint64_t)chunk_len;
    *state =
      ((Hacl_Hash_SHA3_state_t){ .block_state = block_state1, .buf = buf, .total_len = total_len2 });
  }
  else if (sz == 0U)
  {
    Hacl_Hash_SHA3_state_t s1 = *state;
    Hacl_Hash_SHA3_hash_buf block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)block_len(i) == 0ULL && total_len1 > 0ULL)
    {
      sz1 = block_len(i);
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)block_len(i));
    }
    if (!(sz1 == 0U))
    {
      Spec_Hash_Definitions_hash_alg a1 = block_state1.fst;
      uint64_t *s2 = block_state1.snd;
      Hacl_Hash_SHA3_update_multi_sha3(a1, s2, buf, block_len(i) / block_len(a1));
    }
    uint32_t ite;
    if ((uint64_t)chunk_len % (uint64_t)block_len(i) == 0ULL && (uint64_t)chunk_len > 0ULL)
    {
      ite = block_len(i);
    }
    else
    {
      ite = (uint32_t)((uint64_t)chunk_len % (uint64_t)block_len(i));
    }
    uint32_t n_blocks = (chunk_len - ite) / block_len(i);
    uint32_t data1_len = n_blocks * block_len(i);
    uint32_t data2_len = chunk_len - data1_len;
    uint8_t *data1 = chunk;
    uint8_t *data2 = chunk + data1_len;
    Spec_Hash_Definitions_hash_alg a1 = block_state1.fst;
    uint64_t *s2 = block_state1.snd;
    Hacl_Hash_SHA3_update_multi_sha3(a1, s2, data1, data1_len / block_len(a1));
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Hash_SHA3_state_t){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)chunk_len
        }
      );
  }
  else
  {
    uint32_t diff = block_len(i) - sz;
    uint8_t *chunk1 = chunk;
    uint8_t *chunk2 = chunk + diff;
    Hacl_Hash_SHA3_state_t s1 = *state;
    Hacl_Hash_SHA3_hash_buf block_state10 = s1.block_state;
    uint8_t *buf0 = s1.buf;
    uint64_t total_len10 = s1.total_len;
    uint32_t sz10;
    if (total_len10 % (uint64_t)block_len(i) == 0ULL && total_len10 > 0ULL)
    {
      sz10 = block_len(i);
    }
    else
    {
      sz10 = (uint32_t)(total_len10 % (uint64_t)block_len(i));
    }
    uint8_t *buf2 = buf0 + sz10;
    memcpy(buf2, chunk1, diff * sizeof (uint8_t));
    uint64_t total_len2 = total_len10 + (uint64_t)diff;
    *state =
      (
        (Hacl_Hash_SHA3_state_t){
          .block_state = block_state10,
          .buf = buf0,
          .total_len = total_len2
        }
      );
    Hacl_Hash_SHA3_state_t s10 = *state;
    Hacl_Hash_SHA3_hash_buf block_state1 = s10.block_state;
    uint8_t *buf = s10.buf;
    uint64_t total_len1 = s10.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)block_len(i) == 0ULL && total_len1 > 0ULL)
    {
      sz1 = block_len(i);
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)block_len(i));
    }
    if (!(sz1 == 0U))
    {
      Spec_Hash_Definitions_hash_alg a1 = block_state1.fst;
      uint64_t *s2 = block_state1.snd;
      Hacl_Hash_SHA3_update_multi_sha3(a1, s2, buf, block_len(i) / block_len(a1));
    }
    uint32_t ite;
    if
    (
      (uint64_t)(chunk_len - diff) % (uint64_t)block_len(i) == 0ULL &&
        (uint64_t)(chunk_len - diff) > 0ULL
    )
    {
      ite = block_len(i);
    }
    else
    {
      ite = (uint32_t)((uint64_t)(chunk_len - diff) % (uint64_t)block_len(i));
    }
    uint32_t n_blocks = (chunk_len - diff - ite) / block_len(i);
    uint32_t data1_len = n_blocks * block_len(i);
    uint32_t data2_len = chunk_len - diff - data1_len;
    uint8_t *data1 = chunk2;
    uint8_t *data2 = chunk2 + data1_len;
    Spec_Hash_Definitions_hash_alg a1 = block_state1.fst;
    uint64_t *s2 = block_state1.snd;
    Hacl_Hash_SHA3_update_multi_sha3(a1, s2, data1, data1_len / block_len(a1));
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Hash_SHA3_state_t){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)(chunk_len - diff)
        }
      );
  }
  return Hacl_Streaming_Types_Success;
}

static void
digest_(
  Spec_Hash_Definitions_hash_alg a,
  Hacl_Hash_SHA3_state_t *state,
  uint8_t *output,
  uint32_t l
)
{
  Hacl_Hash_SHA3_state_t scrut0 = *state;
  Hacl_Hash_SHA3_hash_buf block_state = scrut0.block_state;
  uint8_t *buf_ = scrut0.buf;
  uint64_t total_len = scrut0.total_len;
  uint32_t r;
  if (total_len % (uint64_t)block_len(a) == 0ULL && total_len > 0ULL)
  {
    r = block_len(a);
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)block_len(a));
  }
  uint8_t *buf_1 = buf_;
  uint64_t buf[25U] = { 0U };
  Hacl_Hash_SHA3_hash_buf tmp_block_state = { .fst = a, .snd = buf };
  hash_buf2 scrut = { .fst = block_state, .snd = tmp_block_state };
  uint64_t *s_dst = scrut.snd.snd;
  uint64_t *s_src = scrut.fst.snd;
  memcpy(s_dst, s_src, 25U * sizeof (uint64_t));
  uint32_t ite;
  if (r % block_len(a) == 0U && r > 0U)
  {
    ite = block_len(a);
  }
  else
  {
    ite = r % block_len(a);
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  Spec_Hash_Definitions_hash_alg a1 = tmp_block_state.fst;
  uint64_t *s0 = tmp_block_state.snd;
  Hacl_Hash_SHA3_update_multi_sha3(a1, s0, buf_multi, 0U / block_len(a1));
  Spec_Hash_Definitions_hash_alg a10 = tmp_block_state.fst;
  uint64_t *s1 = tmp_block_state.snd;
  Hacl_Hash_SHA3_update_last_sha3(a10, s1, buf_last, r);
  Spec_Hash_Definitions_hash_alg a11 = tmp_block_state.fst;
  uint64_t *s = tmp_block_state.snd;
  bool sw;
  switch (a11)
  {
    case Spec_Hash_Definitions_Shake128:
      {
        sw = true;
        break;
      }
    case Spec_Hash_Definitions_Shake256:
      {
        sw = true;
        break;
      }
    default:
      {
        sw = false;
      }
  }
  if (sw)
  {
    squeeze(s, block_len(a11), l, output);
    return;
  }
  squeeze(s, block_len(a11), hash_len(a11), output);
}

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA3_digest(Hacl_Hash_SHA3_state_t *state, uint8_t *output)
{
  Spec_Hash_Definitions_hash_alg a1 = Hacl_Hash_SHA3_get_alg(state);
  if (a1 == Spec_Hash_Definitions_Shake128 || a1 == Spec_Hash_Definitions_Shake256)
  {
    return Hacl_Streaming_Types_InvalidAlgorithm;
  }
  digest_(a1, state, output, hash_len(a1));
  return Hacl_Streaming_Types_Success;
}

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA3_squeeze(Hacl_Hash_SHA3_state_t *s, uint8_t *dst, uint32_t l)
{
  Spec_Hash_Definitions_hash_alg a1 = Hacl_Hash_SHA3_get_alg(s);
  if (!(a1 == Spec_Hash_Definitions_Shake128 || a1 == Spec_Hash_Definitions_Shake256))
  {
    return Hacl_Streaming_Types_InvalidAlgorithm;
  }
  if (l == 0U)
  {
    return Hacl_Streaming_Types_InvalidLength;
  }
  digest_(a1, s, dst, l);
  return Hacl_Streaming_Types_Success;
}

uint32_t Hacl_Hash_SHA3_block_len(Hacl_Hash_SHA3_state_t *s)
{
  Spec_Hash_Definitions_hash_alg a1 = Hacl_Hash_SHA3_get_alg(s);
  return block_len(a1);
}

uint32_t Hacl_Hash_SHA3_hash_len(Hacl_Hash_SHA3_state_t *s)
{
  Spec_Hash_Definitions_hash_alg a1 = Hacl_Hash_SHA3_get_alg(s);
  return hash_len(a1);
}

bool Hacl_Hash_SHA3_is_shake(Hacl_Hash_SHA3_state_t *s)
{
  Spec_Hash_Definitions_hash_alg uu____0 = Hacl_Hash_SHA3_get_alg(s);
  return uu____0 == Spec_Hash_Definitions_Shake128 || uu____0 == Spec_Hash_Definitions_Shake256;
}

void Hacl_Hash_SHA3_absorb_inner_32(uint32_t rateInBytes, uint8_t *b, uint64_t *s)
{
  KRML_MAYBE_UNUSED_VAR(rateInBytes);
  uint64_t ws[32U] = { 0U };
  uint8_t *b1 = b;
  uint64_t u = load64_le(b1);
  ws[0U] = u;
  uint64_t u0 = load64_le(b1 + 8U);
  ws[1U] = u0;
  uint64_t u1 = load64_le(b1 + 16U);
  ws[2U] = u1;
  uint64_t u2 = load64_le(b1 + 24U);
  ws[3U] = u2;
  uint64_t u3 = load64_le(b1 + 32U);
  ws[4U] = u3;
  uint64_t u4 = load64_le(b1 + 40U);
  ws[5U] = u4;
  uint64_t u5 = load64_le(b1 + 48U);
  ws[6U] = u5;
  uint64_t u6 = load64_le(b1 + 56U);
  ws[7U] = u6;
  uint64_t u7 = load64_le(b1 + 64U);
  ws[8U] = u7;
  uint64_t u8 = load64_le(b1 + 72U);
  ws[9U] = u8;
  uint64_t u9 = load64_le(b1 + 80U);
  ws[10U] = u9;
  uint64_t u10 = load64_le(b1 + 88U);
  ws[11U] = u10;
  uint64_t u11 = load64_le(b1 + 96U);
  ws[12U] = u11;
  uint64_t u12 = load64_le(b1 + 104U);
  ws[13U] = u12;
  uint64_t u13 = load64_le(b1 + 112U);
  ws[14U] = u13;
  uint64_t u14 = load64_le(b1 + 120U);
  ws[15U] = u14;
  uint64_t u15 = load64_le(b1 + 128U);
  ws[16U] = u15;
  uint64_t u16 = load64_le(b1 + 136U);
  ws[17U] = u16;
  uint64_t u17 = load64_le(b1 + 144U);
  ws[18U] = u17;
  uint64_t u18 = load64_le(b1 + 152U);
  ws[19U] = u18;
  uint64_t u19 = load64_le(b1 + 160U);
  ws[20U] = u19;
  uint64_t u20 = load64_le(b1 + 168U);
  ws[21U] = u20;
  uint64_t u21 = load64_le(b1 + 176U);
  ws[22U] = u21;
  uint64_t u22 = load64_le(b1 + 184U);
  ws[23U] = u22;
  uint64_t u23 = load64_le(b1 + 192U);
  ws[24U] = u23;
  uint64_t u24 = load64_le(b1 + 200U);
  ws[25U] = u24;
  uint64_t u25 = load64_le(b1 + 208U);
  ws[26U] = u25;
  uint64_t u26 = load64_le(b1 + 216U);
  ws[27U] = u26;
  uint64_t u27 = load64_le(b1 + 224U);
  ws[28U] = u27;
  uint64_t u28 = load64_le(b1 + 232U);
  ws[29U] = u28;
  uint64_t u29 = load64_le(b1 + 240U);
  ws[30U] = u29;
  uint64_t u30 = load64_le(b1 + 248U);
  ws[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws[i];
  }
  for (uint32_t i0 = 0U; i0 < 24U; i0++)
  {
    uint64_t _C[5U] = { 0U };
    KRML_MAYBE_FOR5(i,
      0U,
      5U,
      1U,
      _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
    KRML_MAYBE_FOR5(i1,
      0U,
      5U,
      1U,
      uint64_t uu____0 = _C[(i1 + 1U) % 5U];
      uint64_t _D = _C[(i1 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
      KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i1 + 5U * i] = s[i1 + 5U * i] ^ _D;););
    uint64_t x = s[1U];
    uint64_t current = x;
    for (uint32_t i = 0U; i < 24U; i++)
    {
      uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
      uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
      uint64_t temp = s[_Y];
      uint64_t uu____1 = current;
      s[_Y] = uu____1 << r | uu____1 >> (64U - r);
      current = temp;
    }
    KRML_MAYBE_FOR5(i,
      0U,
      5U,
      1U,
      uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
      uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
      uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
      uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
      uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
      s[0U + 5U * i] = v0;
      s[1U + 5U * i] = v1;
      s[2U + 5U * i] = v2;
      s[3U + 5U * i] = v3;
      s[4U + 5U * i] = v4;);
    uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i0];
    s[0U] = s[0U] ^ c;
  }
}

void
Hacl_Hash_SHA3_shake128(
  uint8_t *output,
  uint32_t outputByteLen,
  uint8_t *input,
  uint32_t inputByteLen
)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t s[25U] = { 0U };
  uint32_t rateInBytes1 = 168U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = ib;
    uint8_t *bl0 = b_;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b_, s);
  }
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b00 = ib;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[inputByteLen % rateInBytes1] = 0x1FU;
  uint64_t ws0[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws0[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws0[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws0[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws0[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws0[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws0[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws0[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws0[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws0[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws0[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws0[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws0[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws0[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws0[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws0[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws0[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws0[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws0[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws0[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws0[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws0[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws0[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws0[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws0[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws0[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws0[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws0[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws0[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws0[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws0[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws0[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws0[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws0[i];
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b3, s);
  for (uint32_t i0 = 0U; i0 < outputByteLen / rateInBytes1; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, s, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b02 = rb;
    memcpy(b02 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i2 + 5U * i] = s[i2 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____1 = current;
        s[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      s[0U] = s[0U] ^ c;
    }
  }
  uint32_t remOut = outputByteLen % rateInBytes1;
  uint8_t hbuf[256U] = { 0U };
  uint64_t ws[32U] = { 0U };
  memcpy(ws, s, 25U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < 32U; i++)
  {
    store64_le(hbuf + i * 8U, ws[i]);
  }
  memcpy(rb + outputByteLen - remOut, hbuf, remOut * sizeof (uint8_t));
}

void
Hacl_Hash_SHA3_shake256(
  uint8_t *output,
  uint32_t outputByteLen,
  uint8_t *input,
  uint32_t inputByteLen
)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t s[25U] = { 0U };
  uint32_t rateInBytes1 = 136U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = ib;
    uint8_t *bl0 = b_;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b_, s);
  }
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b00 = ib;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[inputByteLen % rateInBytes1] = 0x1FU;
  uint64_t ws0[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws0[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws0[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws0[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws0[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws0[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws0[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws0[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws0[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws0[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws0[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws0[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws0[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws0[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws0[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws0[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws0[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws0[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws0[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws0[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws0[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws0[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws0[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws0[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws0[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws0[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws0[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws0[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws0[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws0[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws0[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws0[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws0[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws0[i];
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b3, s);
  for (uint32_t i0 = 0U; i0 < outputByteLen / rateInBytes1; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, s, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b02 = rb;
    memcpy(b02 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i2 + 5U * i] = s[i2 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____1 = current;
        s[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      s[0U] = s[0U] ^ c;
    }
  }
  uint32_t remOut = outputByteLen % rateInBytes1;
  uint8_t hbuf[256U] = { 0U };
  uint64_t ws[32U] = { 0U };
  memcpy(ws, s, 25U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < 32U; i++)
  {
    store64_le(hbuf + i * 8U, ws[i]);
  }
  memcpy(rb + outputByteLen - remOut, hbuf, remOut * sizeof (uint8_t));
}

void Hacl_Hash_SHA3_sha3_224(uint8_t *output, uint8_t *input, uint32_t inputByteLen)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t s[25U] = { 0U };
  uint32_t rateInBytes1 = 144U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = ib;
    uint8_t *bl0 = b_;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b_, s);
  }
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b00 = ib;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[inputByteLen % rateInBytes1] = 0x06U;
  uint64_t ws0[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws0[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws0[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws0[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws0[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws0[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws0[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws0[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws0[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws0[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws0[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws0[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws0[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws0[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws0[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws0[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws0[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws0[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws0[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws0[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws0[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws0[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws0[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws0[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws0[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws0[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws0[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws0[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws0[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws0[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws0[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws0[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws0[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws0[i];
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b3, s);
  for (uint32_t i0 = 0U; i0 < 28U / rateInBytes1; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, s, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b02 = rb;
    memcpy(b02 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i2 + 5U * i] = s[i2 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____1 = current;
        s[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      s[0U] = s[0U] ^ c;
    }
  }
  uint32_t remOut = 28U % rateInBytes1;
  uint8_t hbuf[256U] = { 0U };
  uint64_t ws[32U] = { 0U };
  memcpy(ws, s, 25U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < 32U; i++)
  {
    store64_le(hbuf + i * 8U, ws[i]);
  }
  memcpy(rb + 28U - remOut, hbuf, remOut * sizeof (uint8_t));
}

void Hacl_Hash_SHA3_sha3_256(uint8_t *output, uint8_t *input, uint32_t inputByteLen)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t s[25U] = { 0U };
  uint32_t rateInBytes1 = 136U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = ib;
    uint8_t *bl0 = b_;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b_, s);
  }
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b00 = ib;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[inputByteLen % rateInBytes1] = 0x06U;
  uint64_t ws0[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws0[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws0[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws0[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws0[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws0[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws0[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws0[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws0[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws0[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws0[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws0[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws0[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws0[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws0[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws0[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws0[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws0[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws0[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws0[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws0[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws0[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws0[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws0[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws0[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws0[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws0[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws0[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws0[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws0[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws0[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws0[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws0[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws0[i];
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b3, s);
  for (uint32_t i0 = 0U; i0 < 32U / rateInBytes1; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, s, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b02 = rb;
    memcpy(b02 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i2 + 5U * i] = s[i2 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____1 = current;
        s[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      s[0U] = s[0U] ^ c;
    }
  }
  uint32_t remOut = 32U % rateInBytes1;
  uint8_t hbuf[256U] = { 0U };
  uint64_t ws[32U] = { 0U };
  memcpy(ws, s, 25U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < 32U; i++)
  {
    store64_le(hbuf + i * 8U, ws[i]);
  }
  memcpy(rb + 32U - remOut, hbuf, remOut * sizeof (uint8_t));
}

void Hacl_Hash_SHA3_sha3_384(uint8_t *output, uint8_t *input, uint32_t inputByteLen)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t s[25U] = { 0U };
  uint32_t rateInBytes1 = 104U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = ib;
    uint8_t *bl0 = b_;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b_, s);
  }
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b00 = ib;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[inputByteLen % rateInBytes1] = 0x06U;
  uint64_t ws0[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws0[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws0[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws0[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws0[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws0[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws0[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws0[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws0[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws0[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws0[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws0[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws0[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws0[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws0[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws0[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws0[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws0[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws0[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws0[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws0[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws0[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws0[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws0[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws0[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws0[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws0[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws0[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws0[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws0[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws0[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws0[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws0[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws0[i];
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b3, s);
  for (uint32_t i0 = 0U; i0 < 48U / rateInBytes1; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, s, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b02 = rb;
    memcpy(b02 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i2 + 5U * i] = s[i2 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____1 = current;
        s[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      s[0U] = s[0U] ^ c;
    }
  }
  uint32_t remOut = 48U % rateInBytes1;
  uint8_t hbuf[256U] = { 0U };
  uint64_t ws[32U] = { 0U };
  memcpy(ws, s, 25U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < 32U; i++)
  {
    store64_le(hbuf + i * 8U, ws[i]);
  }
  memcpy(rb + 48U - remOut, hbuf, remOut * sizeof (uint8_t));
}

void Hacl_Hash_SHA3_sha3_512(uint8_t *output, uint8_t *input, uint32_t inputByteLen)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t s[25U] = { 0U };
  uint32_t rateInBytes1 = 72U;
  for (uint32_t i = 0U; i < inputByteLen / rateInBytes1; i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = ib;
    uint8_t *bl0 = b_;
    memcpy(bl0, b0 + i * rateInBytes1, rateInBytes1 * sizeof (uint8_t));
    Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b_, s);
  }
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = inputByteLen % rateInBytes1;
  uint8_t *b00 = ib;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[inputByteLen % rateInBytes1] = 0x06U;
  uint64_t ws0[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws0[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws0[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws0[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws0[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws0[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws0[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws0[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws0[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws0[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws0[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws0[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws0[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws0[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws0[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws0[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws0[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws0[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws0[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws0[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws0[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws0[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws0[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws0[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws0[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws0[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws0[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws0[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws0[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws0[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws0[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws0[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws0[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    s[i] = s[i] ^ ws0[i];
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[rateInBytes1 - 1U] = 0x80U;
  Hacl_Hash_SHA3_absorb_inner_32(rateInBytes1, b3, s);
  for (uint32_t i0 = 0U; i0 < 64U / rateInBytes1; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, s, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b02 = rb;
    memcpy(b02 + i0 * rateInBytes1, hbuf, rateInBytes1 * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] = s[i + 0U] ^ (s[i + 5U] ^ (s[i + 10U] ^ (s[i + 15U] ^ s[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i2 + 5U * i] = s[i2 + 5U * i] ^ _D;););
      uint64_t x = s[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = s[_Y];
        uint64_t uu____1 = current;
        s[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = s[0U + 5U * i] ^ (~s[1U + 5U * i] & s[2U + 5U * i]);
        uint64_t v1 = s[1U + 5U * i] ^ (~s[2U + 5U * i] & s[3U + 5U * i]);
        uint64_t v2 = s[2U + 5U * i] ^ (~s[3U + 5U * i] & s[4U + 5U * i]);
        uint64_t v3 = s[3U + 5U * i] ^ (~s[4U + 5U * i] & s[0U + 5U * i]);
        uint64_t v4 = s[4U + 5U * i] ^ (~s[0U + 5U * i] & s[1U + 5U * i]);
        s[0U + 5U * i] = v0;
        s[1U + 5U * i] = v1;
        s[2U + 5U * i] = v2;
        s[3U + 5U * i] = v3;
        s[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      s[0U] = s[0U] ^ c;
    }
  }
  uint32_t remOut = 64U % rateInBytes1;
  uint8_t hbuf[256U] = { 0U };
  uint64_t ws[32U] = { 0U };
  memcpy(ws, s, 25U * sizeof (uint64_t));
  for (uint32_t i = 0U; i < 32U; i++)
  {
    store64_le(hbuf + i * 8U, ws[i]);
  }
  memcpy(rb + 64U - remOut, hbuf, remOut * sizeof (uint8_t));
}

/**
Allocate state buffer of 200-bytes
*/
uint64_t *Hacl_Hash_SHA3_state_malloc(void)
{
  uint64_t *buf = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
  return buf;
}

/**
Free state buffer
*/
void Hacl_Hash_SHA3_state_free(uint64_t *s)
{
  KRML_HOST_FREE(s);
}

/**
Absorb number of input blocks and write the output state

  This function is intended to receive a hash state and input buffer.
  It processes an input of multiple of 168-bytes (SHAKE128 block size),
  any additional bytes of final partial block are ignored.

  The argument `state` (IN/OUT) points to hash state, i.e., uint64_t[25]
  The argument `input` (IN) points to `inputByteLen` bytes of valid memory,
  i.e., uint8_t[inputByteLen]
*/
void
Hacl_Hash_SHA3_shake128_absorb_nblocks(uint64_t *state, uint8_t *input, uint32_t inputByteLen)
{
  for (uint32_t i = 0U; i < inputByteLen / 168U; i++)
  {
    uint8_t b[256U] = { 0U };
    uint8_t *b_ = b;
    uint8_t *b0 = input;
    uint8_t *bl0 = b_;
    memcpy(bl0, b0 + i * 168U, 168U * sizeof (uint8_t));
    Hacl_Hash_SHA3_absorb_inner_32(168U, b_, state);
  }
}

/**
Absorb a final partial block of input and write the output state

  This function is intended to receive a hash state and input buffer.
  It processes a sequence of bytes at end of input buffer that is less
  than 168-bytes (SHAKE128 block size),
  any bytes of full blocks at start of input buffer are ignored.

  The argument `state` (IN/OUT) points to hash state, i.e., uint64_t[25]
  The argument `input` (IN) points to `inputByteLen` bytes of valid memory,
  i.e., uint8_t[inputByteLen]

  Note: Full size of input buffer must be passed to `inputByteLen` including
  the number of full-block bytes at start of input buffer that are ignored
*/
void
Hacl_Hash_SHA3_shake128_absorb_final(uint64_t *state, uint8_t *input, uint32_t inputByteLen)
{
  uint8_t b1[256U] = { 0U };
  uint8_t *b_ = b1;
  uint32_t rem = inputByteLen % 168U;
  uint8_t *b00 = input;
  uint8_t *bl0 = b_;
  memcpy(bl0, b00 + inputByteLen - rem, rem * sizeof (uint8_t));
  uint8_t *b01 = b_;
  b01[inputByteLen % 168U] = 0x1FU;
  uint64_t ws[32U] = { 0U };
  uint8_t *b = b_;
  uint64_t u = load64_le(b);
  ws[0U] = u;
  uint64_t u0 = load64_le(b + 8U);
  ws[1U] = u0;
  uint64_t u1 = load64_le(b + 16U);
  ws[2U] = u1;
  uint64_t u2 = load64_le(b + 24U);
  ws[3U] = u2;
  uint64_t u3 = load64_le(b + 32U);
  ws[4U] = u3;
  uint64_t u4 = load64_le(b + 40U);
  ws[5U] = u4;
  uint64_t u5 = load64_le(b + 48U);
  ws[6U] = u5;
  uint64_t u6 = load64_le(b + 56U);
  ws[7U] = u6;
  uint64_t u7 = load64_le(b + 64U);
  ws[8U] = u7;
  uint64_t u8 = load64_le(b + 72U);
  ws[9U] = u8;
  uint64_t u9 = load64_le(b + 80U);
  ws[10U] = u9;
  uint64_t u10 = load64_le(b + 88U);
  ws[11U] = u10;
  uint64_t u11 = load64_le(b + 96U);
  ws[12U] = u11;
  uint64_t u12 = load64_le(b + 104U);
  ws[13U] = u12;
  uint64_t u13 = load64_le(b + 112U);
  ws[14U] = u13;
  uint64_t u14 = load64_le(b + 120U);
  ws[15U] = u14;
  uint64_t u15 = load64_le(b + 128U);
  ws[16U] = u15;
  uint64_t u16 = load64_le(b + 136U);
  ws[17U] = u16;
  uint64_t u17 = load64_le(b + 144U);
  ws[18U] = u17;
  uint64_t u18 = load64_le(b + 152U);
  ws[19U] = u18;
  uint64_t u19 = load64_le(b + 160U);
  ws[20U] = u19;
  uint64_t u20 = load64_le(b + 168U);
  ws[21U] = u20;
  uint64_t u21 = load64_le(b + 176U);
  ws[22U] = u21;
  uint64_t u22 = load64_le(b + 184U);
  ws[23U] = u22;
  uint64_t u23 = load64_le(b + 192U);
  ws[24U] = u23;
  uint64_t u24 = load64_le(b + 200U);
  ws[25U] = u24;
  uint64_t u25 = load64_le(b + 208U);
  ws[26U] = u25;
  uint64_t u26 = load64_le(b + 216U);
  ws[27U] = u26;
  uint64_t u27 = load64_le(b + 224U);
  ws[28U] = u27;
  uint64_t u28 = load64_le(b + 232U);
  ws[29U] = u28;
  uint64_t u29 = load64_le(b + 240U);
  ws[30U] = u29;
  uint64_t u30 = load64_le(b + 248U);
  ws[31U] = u30;
  for (uint32_t i = 0U; i < 25U; i++)
  {
    state[i] = state[i] ^ ws[i];
  }
  uint8_t b2[256U] = { 0U };
  uint8_t *b3 = b2;
  uint8_t *b0 = b3;
  b0[167U] = 0x80U;
  Hacl_Hash_SHA3_absorb_inner_32(168U, b3, state);
}

/**
Squeeze a hash state to output buffer

  This function is intended to receive a hash state and output buffer.
  It produces an output of multiple of 168-bytes (SHAKE128 block size),
  any additional bytes of final partial block are ignored.

  The argument `state` (IN) points to hash state, i.e., uint64_t[25]
  The argument `output` (OUT) points to `outputByteLen` bytes of valid memory,
  i.e., uint8_t[outputByteLen]
*/
void
Hacl_Hash_SHA3_shake128_squeeze_nblocks(
  uint64_t *state,
  uint8_t *output,
  uint32_t outputByteLen
)
{
  for (uint32_t i0 = 0U; i0 < outputByteLen / 168U; i0++)
  {
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, state, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    uint8_t *b0 = output;
    memcpy(b0 + i0 * 168U, hbuf, 168U * sizeof (uint8_t));
    for (uint32_t i1 = 0U; i1 < 24U; i1++)
    {
      uint64_t _C[5U] = { 0U };
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        _C[i] =
          state[i + 0U] ^ (state[i + 5U] ^ (state[i + 10U] ^ (state[i + 15U] ^ state[i + 20U]))););
      KRML_MAYBE_FOR5(i2,
        0U,
        5U,
        1U,
        uint64_t uu____0 = _C[(i2 + 1U) % 5U];
        uint64_t _D = _C[(i2 + 4U) % 5U] ^ (uu____0 << 1U | uu____0 >> 63U);
        KRML_MAYBE_FOR5(i, 0U, 5U, 1U, state[i2 + 5U * i] = state[i2 + 5U * i] ^ _D;););
      uint64_t x = state[1U];
      uint64_t current = x;
      for (uint32_t i = 0U; i < 24U; i++)
      {
        uint32_t _Y = Hacl_Hash_SHA3_keccak_piln[i];
        uint32_t r = Hacl_Hash_SHA3_keccak_rotc[i];
        uint64_t temp = state[_Y];
        uint64_t uu____1 = current;
        state[_Y] = uu____1 << r | uu____1 >> (64U - r);
        current = temp;
      }
      KRML_MAYBE_FOR5(i,
        0U,
        5U,
        1U,
        uint64_t v0 = state[0U + 5U * i] ^ (~state[1U + 5U * i] & state[2U + 5U * i]);
        uint64_t v1 = state[1U + 5U * i] ^ (~state[2U + 5U * i] & state[3U + 5U * i]);
        uint64_t v2 = state[2U + 5U * i] ^ (~state[3U + 5U * i] & state[4U + 5U * i]);
        uint64_t v3 = state[3U + 5U * i] ^ (~state[4U + 5U * i] & state[0U + 5U * i]);
        uint64_t v4 = state[4U + 5U * i] ^ (~state[0U + 5U * i] & state[1U + 5U * i]);
        state[0U + 5U * i] = v0;
        state[1U + 5U * i] = v1;
        state[2U + 5U * i] = v2;
        state[3U + 5U * i] = v3;
        state[4U + 5U * i] = v4;);
      uint64_t c = Hacl_Hash_SHA3_keccak_rndc[i1];
      state[0U] = state[0U] ^ c;
    }
  }
}

