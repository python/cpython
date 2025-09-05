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


#include "internal/Hacl_Hash_SHA2.h"

#include "Hacl_Streaming_Types.h"

#include "internal/Hacl_Streaming_Types.h"


void Hacl_Hash_SHA2_sha256_init(uint32_t *hash)
{
  KRML_MAYBE_FOR8(i,
    0U,
    8U,
    1U,
    uint32_t *os = hash;
    uint32_t x = Hacl_Hash_SHA2_h256[i];
    os[i] = x;);
}

static inline void sha256_update(uint8_t *b, uint32_t *hash)
{
  uint32_t hash_old[8U] = { 0U };
  uint32_t ws[16U] = { 0U };
  memcpy(hash_old, hash, 8U * sizeof (uint32_t));
  uint8_t *b10 = b;
  uint32_t u = load32_be(b10);
  ws[0U] = u;
  uint32_t u0 = load32_be(b10 + 4U);
  ws[1U] = u0;
  uint32_t u1 = load32_be(b10 + 8U);
  ws[2U] = u1;
  uint32_t u2 = load32_be(b10 + 12U);
  ws[3U] = u2;
  uint32_t u3 = load32_be(b10 + 16U);
  ws[4U] = u3;
  uint32_t u4 = load32_be(b10 + 20U);
  ws[5U] = u4;
  uint32_t u5 = load32_be(b10 + 24U);
  ws[6U] = u5;
  uint32_t u6 = load32_be(b10 + 28U);
  ws[7U] = u6;
  uint32_t u7 = load32_be(b10 + 32U);
  ws[8U] = u7;
  uint32_t u8 = load32_be(b10 + 36U);
  ws[9U] = u8;
  uint32_t u9 = load32_be(b10 + 40U);
  ws[10U] = u9;
  uint32_t u10 = load32_be(b10 + 44U);
  ws[11U] = u10;
  uint32_t u11 = load32_be(b10 + 48U);
  ws[12U] = u11;
  uint32_t u12 = load32_be(b10 + 52U);
  ws[13U] = u12;
  uint32_t u13 = load32_be(b10 + 56U);
  ws[14U] = u13;
  uint32_t u14 = load32_be(b10 + 60U);
  ws[15U] = u14;
  KRML_MAYBE_FOR4(i0,
    0U,
    4U,
    1U,
    KRML_MAYBE_FOR16(i,
      0U,
      16U,
      1U,
      uint32_t k_t = Hacl_Hash_SHA2_k224_256[16U * i0 + i];
      uint32_t ws_t = ws[i];
      uint32_t a0 = hash[0U];
      uint32_t b0 = hash[1U];
      uint32_t c0 = hash[2U];
      uint32_t d0 = hash[3U];
      uint32_t e0 = hash[4U];
      uint32_t f0 = hash[5U];
      uint32_t g0 = hash[6U];
      uint32_t h02 = hash[7U];
      uint32_t k_e_t = k_t;
      uint32_t
      t1 =
        h02 + ((e0 << 26U | e0 >> 6U) ^ ((e0 << 21U | e0 >> 11U) ^ (e0 << 7U | e0 >> 25U))) +
          ((e0 & f0) ^ (~e0 & g0))
        + k_e_t
        + ws_t;
      uint32_t
      t2 =
        ((a0 << 30U | a0 >> 2U) ^ ((a0 << 19U | a0 >> 13U) ^ (a0 << 10U | a0 >> 22U))) +
          ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
      uint32_t a1 = t1 + t2;
      uint32_t b1 = a0;
      uint32_t c1 = b0;
      uint32_t d1 = c0;
      uint32_t e1 = d0 + t1;
      uint32_t f1 = e0;
      uint32_t g1 = f0;
      uint32_t h12 = g0;
      hash[0U] = a1;
      hash[1U] = b1;
      hash[2U] = c1;
      hash[3U] = d1;
      hash[4U] = e1;
      hash[5U] = f1;
      hash[6U] = g1;
      hash[7U] = h12;);
    if (i0 < 3U)
    {
      KRML_MAYBE_FOR16(i,
        0U,
        16U,
        1U,
        uint32_t t16 = ws[i];
        uint32_t t15 = ws[(i + 1U) % 16U];
        uint32_t t7 = ws[(i + 9U) % 16U];
        uint32_t t2 = ws[(i + 14U) % 16U];
        uint32_t s1 = (t2 << 15U | t2 >> 17U) ^ ((t2 << 13U | t2 >> 19U) ^ t2 >> 10U);
        uint32_t s0 = (t15 << 25U | t15 >> 7U) ^ ((t15 << 14U | t15 >> 18U) ^ t15 >> 3U);
        ws[i] = s1 + t7 + s0 + t16;);
    });
  KRML_MAYBE_FOR8(i,
    0U,
    8U,
    1U,
    uint32_t *os = hash;
    uint32_t x = hash[i] + hash_old[i];
    os[i] = x;);
}

void Hacl_Hash_SHA2_sha256_update_nblocks(uint32_t len, uint8_t *b, uint32_t *st)
{
  uint32_t blocks = len / 64U;
  for (uint32_t i = 0U; i < blocks; i++)
  {
    uint8_t *b0 = b;
    uint8_t *mb = b0 + i * 64U;
    sha256_update(mb, st);
  }
}

void
Hacl_Hash_SHA2_sha256_update_last(uint64_t totlen, uint32_t len, uint8_t *b, uint32_t *hash)
{
  uint32_t blocks;
  if (len + 8U + 1U <= 64U)
  {
    blocks = 1U;
  }
  else
  {
    blocks = 2U;
  }
  uint32_t fin = blocks * 64U;
  uint8_t last[128U] = { 0U };
  uint8_t totlen_buf[8U] = { 0U };
  uint64_t total_len_bits = totlen << 3U;
  store64_be(totlen_buf, total_len_bits);
  uint8_t *b0 = b;
  memcpy(last, b0, len * sizeof (uint8_t));
  last[len] = 0x80U;
  memcpy(last + fin - 8U, totlen_buf, 8U * sizeof (uint8_t));
  uint8_t *last00 = last;
  uint8_t *last10 = last + 64U;
  uint8_t *l0 = last00;
  uint8_t *l1 = last10;
  uint8_t *lb0 = l0;
  uint8_t *lb1 = l1;
  uint8_t *last0 = lb0;
  uint8_t *last1 = lb1;
  sha256_update(last0, hash);
  if (blocks > 1U)
  {
    sha256_update(last1, hash);
    return;
  }
}

void Hacl_Hash_SHA2_sha256_finish(uint32_t *st, uint8_t *h)
{
  uint8_t hbuf[32U] = { 0U };
  KRML_MAYBE_FOR8(i, 0U, 8U, 1U, store32_be(hbuf + i * 4U, st[i]););
  memcpy(h, hbuf, 32U * sizeof (uint8_t));
}

void Hacl_Hash_SHA2_sha224_init(uint32_t *hash)
{
  KRML_MAYBE_FOR8(i,
    0U,
    8U,
    1U,
    uint32_t *os = hash;
    uint32_t x = Hacl_Hash_SHA2_h224[i];
    os[i] = x;);
}

void Hacl_Hash_SHA2_sha224_update_nblocks(uint32_t len, uint8_t *b, uint32_t *st)
{
  Hacl_Hash_SHA2_sha256_update_nblocks(len, b, st);
}

void Hacl_Hash_SHA2_sha224_update_last(uint64_t totlen, uint32_t len, uint8_t *b, uint32_t *st)
{
  Hacl_Hash_SHA2_sha256_update_last(totlen, len, b, st);
}

void Hacl_Hash_SHA2_sha224_finish(uint32_t *st, uint8_t *h)
{
  uint8_t hbuf[32U] = { 0U };
  KRML_MAYBE_FOR8(i, 0U, 8U, 1U, store32_be(hbuf + i * 4U, st[i]););
  memcpy(h, hbuf, 28U * sizeof (uint8_t));
}

void Hacl_Hash_SHA2_sha512_init(uint64_t *hash)
{
  KRML_MAYBE_FOR8(i,
    0U,
    8U,
    1U,
    uint64_t *os = hash;
    uint64_t x = Hacl_Hash_SHA2_h512[i];
    os[i] = x;);
}

static inline void sha512_update(uint8_t *b, uint64_t *hash)
{
  uint64_t hash_old[8U] = { 0U };
  uint64_t ws[16U] = { 0U };
  memcpy(hash_old, hash, 8U * sizeof (uint64_t));
  uint8_t *b10 = b;
  uint64_t u = load64_be(b10);
  ws[0U] = u;
  uint64_t u0 = load64_be(b10 + 8U);
  ws[1U] = u0;
  uint64_t u1 = load64_be(b10 + 16U);
  ws[2U] = u1;
  uint64_t u2 = load64_be(b10 + 24U);
  ws[3U] = u2;
  uint64_t u3 = load64_be(b10 + 32U);
  ws[4U] = u3;
  uint64_t u4 = load64_be(b10 + 40U);
  ws[5U] = u4;
  uint64_t u5 = load64_be(b10 + 48U);
  ws[6U] = u5;
  uint64_t u6 = load64_be(b10 + 56U);
  ws[7U] = u6;
  uint64_t u7 = load64_be(b10 + 64U);
  ws[8U] = u7;
  uint64_t u8 = load64_be(b10 + 72U);
  ws[9U] = u8;
  uint64_t u9 = load64_be(b10 + 80U);
  ws[10U] = u9;
  uint64_t u10 = load64_be(b10 + 88U);
  ws[11U] = u10;
  uint64_t u11 = load64_be(b10 + 96U);
  ws[12U] = u11;
  uint64_t u12 = load64_be(b10 + 104U);
  ws[13U] = u12;
  uint64_t u13 = load64_be(b10 + 112U);
  ws[14U] = u13;
  uint64_t u14 = load64_be(b10 + 120U);
  ws[15U] = u14;
  KRML_MAYBE_FOR5(i0,
    0U,
    5U,
    1U,
    KRML_MAYBE_FOR16(i,
      0U,
      16U,
      1U,
      uint64_t k_t = Hacl_Hash_SHA2_k384_512[16U * i0 + i];
      uint64_t ws_t = ws[i];
      uint64_t a0 = hash[0U];
      uint64_t b0 = hash[1U];
      uint64_t c0 = hash[2U];
      uint64_t d0 = hash[3U];
      uint64_t e0 = hash[4U];
      uint64_t f0 = hash[5U];
      uint64_t g0 = hash[6U];
      uint64_t h02 = hash[7U];
      uint64_t k_e_t = k_t;
      uint64_t
      t1 =
        h02 + ((e0 << 50U | e0 >> 14U) ^ ((e0 << 46U | e0 >> 18U) ^ (e0 << 23U | e0 >> 41U))) +
          ((e0 & f0) ^ (~e0 & g0))
        + k_e_t
        + ws_t;
      uint64_t
      t2 =
        ((a0 << 36U | a0 >> 28U) ^ ((a0 << 30U | a0 >> 34U) ^ (a0 << 25U | a0 >> 39U))) +
          ((a0 & b0) ^ ((a0 & c0) ^ (b0 & c0)));
      uint64_t a1 = t1 + t2;
      uint64_t b1 = a0;
      uint64_t c1 = b0;
      uint64_t d1 = c0;
      uint64_t e1 = d0 + t1;
      uint64_t f1 = e0;
      uint64_t g1 = f0;
      uint64_t h12 = g0;
      hash[0U] = a1;
      hash[1U] = b1;
      hash[2U] = c1;
      hash[3U] = d1;
      hash[4U] = e1;
      hash[5U] = f1;
      hash[6U] = g1;
      hash[7U] = h12;);
    if (i0 < 4U)
    {
      KRML_MAYBE_FOR16(i,
        0U,
        16U,
        1U,
        uint64_t t16 = ws[i];
        uint64_t t15 = ws[(i + 1U) % 16U];
        uint64_t t7 = ws[(i + 9U) % 16U];
        uint64_t t2 = ws[(i + 14U) % 16U];
        uint64_t s1 = (t2 << 45U | t2 >> 19U) ^ ((t2 << 3U | t2 >> 61U) ^ t2 >> 6U);
        uint64_t s0 = (t15 << 63U | t15 >> 1U) ^ ((t15 << 56U | t15 >> 8U) ^ t15 >> 7U);
        ws[i] = s1 + t7 + s0 + t16;);
    });
  KRML_MAYBE_FOR8(i,
    0U,
    8U,
    1U,
    uint64_t *os = hash;
    uint64_t x = hash[i] + hash_old[i];
    os[i] = x;);
}

void Hacl_Hash_SHA2_sha512_update_nblocks(uint32_t len, uint8_t *b, uint64_t *st)
{
  uint32_t blocks = len / 128U;
  for (uint32_t i = 0U; i < blocks; i++)
  {
    uint8_t *b0 = b;
    uint8_t *mb = b0 + i * 128U;
    sha512_update(mb, st);
  }
}

void
Hacl_Hash_SHA2_sha512_update_last(
  FStar_UInt128_uint128 totlen,
  uint32_t len,
  uint8_t *b,
  uint64_t *hash
)
{
  uint32_t blocks;
  if (len + 16U + 1U <= 128U)
  {
    blocks = 1U;
  }
  else
  {
    blocks = 2U;
  }
  uint32_t fin = blocks * 128U;
  uint8_t last[256U] = { 0U };
  uint8_t totlen_buf[16U] = { 0U };
  FStar_UInt128_uint128 total_len_bits = FStar_UInt128_shift_left(totlen, 3U);
  store128_be(totlen_buf, total_len_bits);
  uint8_t *b0 = b;
  memcpy(last, b0, len * sizeof (uint8_t));
  last[len] = 0x80U;
  memcpy(last + fin - 16U, totlen_buf, 16U * sizeof (uint8_t));
  uint8_t *last00 = last;
  uint8_t *last10 = last + 128U;
  uint8_t *l0 = last00;
  uint8_t *l1 = last10;
  uint8_t *lb0 = l0;
  uint8_t *lb1 = l1;
  uint8_t *last0 = lb0;
  uint8_t *last1 = lb1;
  sha512_update(last0, hash);
  if (blocks > 1U)
  {
    sha512_update(last1, hash);
    return;
  }
}

void Hacl_Hash_SHA2_sha512_finish(uint64_t *st, uint8_t *h)
{
  uint8_t hbuf[64U] = { 0U };
  KRML_MAYBE_FOR8(i, 0U, 8U, 1U, store64_be(hbuf + i * 8U, st[i]););
  memcpy(h, hbuf, 64U * sizeof (uint8_t));
}

void Hacl_Hash_SHA2_sha384_init(uint64_t *hash)
{
  KRML_MAYBE_FOR8(i,
    0U,
    8U,
    1U,
    uint64_t *os = hash;
    uint64_t x = Hacl_Hash_SHA2_h384[i];
    os[i] = x;);
}

void Hacl_Hash_SHA2_sha384_update_nblocks(uint32_t len, uint8_t *b, uint64_t *st)
{
  Hacl_Hash_SHA2_sha512_update_nblocks(len, b, st);
}

void
Hacl_Hash_SHA2_sha384_update_last(
  FStar_UInt128_uint128 totlen,
  uint32_t len,
  uint8_t *b,
  uint64_t *st
)
{
  Hacl_Hash_SHA2_sha512_update_last(totlen, len, b, st);
}

void Hacl_Hash_SHA2_sha384_finish(uint64_t *st, uint8_t *h)
{
  uint8_t hbuf[64U] = { 0U };
  KRML_MAYBE_FOR8(i, 0U, 8U, 1U, store64_be(hbuf + i * 8U, st[i]););
  memcpy(h, hbuf, 48U * sizeof (uint8_t));
}

/**
Allocate initial state for the SHA2_256 hash. The state is to be freed by
calling `free_256`.
*/
Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA2_malloc_256(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(64U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  uint8_t *buf1 = buf;
  uint32_t *b = (uint32_t *)KRML_HOST_CALLOC(8U, sizeof (uint32_t));
  Hacl_Streaming_Types_optional_32 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf1);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint32_t *block_state1 = block_state.v;
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_32
          s = { .block_state = block_state1, .buf = buf1, .total_len = (uint64_t)0U };
          Hacl_Streaming_MD_state_32
          *p = (Hacl_Streaming_MD_state_32 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_32));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
            KRML_HOST_FREE(buf1);
            return NULL;
          }
          Hacl_Hash_SHA2_sha256_init(block_state1);
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

/**
Copies the state passed as argument into a newly allocated state (deep copy).
The state is to be freed by calling `free_256`. Cloning the state this way is
useful, for instance, if your control-flow diverges and you need to feed
more (different) data into the hash in each branch.
*/
Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA2_copy_256(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint32_t *block_state0 = scrut.block_state;
  uint8_t *buf0 = scrut.buf;
  uint64_t total_len0 = scrut.total_len;
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(64U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, buf0, 64U * sizeof (uint8_t));
  uint32_t *b = (uint32_t *)KRML_HOST_CALLOC(8U, sizeof (uint32_t));
  Hacl_Streaming_Types_optional_32 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint32_t *block_state1 = block_state.v;
    memcpy(block_state1, block_state0, 8U * sizeof (uint32_t));
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_32
          s = { .block_state = block_state1, .buf = buf, .total_len = total_len0 };
          Hacl_Streaming_MD_state_32
          *p = (Hacl_Streaming_MD_state_32 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_32));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
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

/**
Reset an existing state to the initial hash state with empty data.
*/
void Hacl_Hash_SHA2_reset_256(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  Hacl_Hash_SHA2_sha256_init(block_state);
  Hacl_Streaming_MD_state_32
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  state[0U] = tmp;
}

static inline Hacl_Streaming_Types_error_code
update_224_256(Hacl_Streaming_MD_state_32 *state, uint8_t *chunk, uint32_t chunk_len)
{
  Hacl_Streaming_MD_state_32 s = *state;
  uint64_t total_len = s.total_len;
  if ((uint64_t)chunk_len > 2305843009213693951ULL - total_len)
  {
    return Hacl_Streaming_Types_MaximumLengthExceeded;
  }
  uint32_t sz;
  if (total_len % (uint64_t)64U == 0ULL && total_len > 0ULL)
  {
    sz = 64U;
  }
  else
  {
    sz = (uint32_t)(total_len % (uint64_t)64U);
  }
  if (chunk_len <= 64U - sz)
  {
    Hacl_Streaming_MD_state_32 s1 = *state;
    uint32_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)64U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)64U);
    }
    uint8_t *buf2 = buf + sz1;
    memcpy(buf2, chunk, chunk_len * sizeof (uint8_t));
    uint64_t total_len2 = total_len1 + (uint64_t)chunk_len;
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len2
        }
      );
  }
  else if (sz == 0U)
  {
    Hacl_Streaming_MD_state_32 s1 = *state;
    uint32_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)64U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)64U);
    }
    if (!(sz1 == 0U))
    {
      Hacl_Hash_SHA2_sha256_update_nblocks(64U, buf, block_state1);
    }
    uint32_t ite;
    if ((uint64_t)chunk_len % (uint64_t)64U == 0ULL && (uint64_t)chunk_len > 0ULL)
    {
      ite = 64U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)chunk_len % (uint64_t)64U);
    }
    uint32_t n_blocks = (chunk_len - ite) / 64U;
    uint32_t data1_len = n_blocks * 64U;
    uint32_t data2_len = chunk_len - data1_len;
    uint8_t *data1 = chunk;
    uint8_t *data2 = chunk + data1_len;
    Hacl_Hash_SHA2_sha256_update_nblocks(data1_len / 64U * 64U, data1, block_state1);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)chunk_len
        }
      );
  }
  else
  {
    uint32_t diff = 64U - sz;
    uint8_t *chunk1 = chunk;
    uint8_t *chunk2 = chunk + diff;
    Hacl_Streaming_MD_state_32 s1 = *state;
    uint32_t *block_state10 = s1.block_state;
    uint8_t *buf0 = s1.buf;
    uint64_t total_len10 = s1.total_len;
    uint32_t sz10;
    if (total_len10 % (uint64_t)64U == 0ULL && total_len10 > 0ULL)
    {
      sz10 = 64U;
    }
    else
    {
      sz10 = (uint32_t)(total_len10 % (uint64_t)64U);
    }
    uint8_t *buf2 = buf0 + sz10;
    memcpy(buf2, chunk1, diff * sizeof (uint8_t));
    uint64_t total_len2 = total_len10 + (uint64_t)diff;
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state10,
          .buf = buf0,
          .total_len = total_len2
        }
      );
    Hacl_Streaming_MD_state_32 s10 = *state;
    uint32_t *block_state1 = s10.block_state;
    uint8_t *buf = s10.buf;
    uint64_t total_len1 = s10.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)64U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)64U);
    }
    if (!(sz1 == 0U))
    {
      Hacl_Hash_SHA2_sha256_update_nblocks(64U, buf, block_state1);
    }
    uint32_t ite;
    if
    ((uint64_t)(chunk_len - diff) % (uint64_t)64U == 0ULL && (uint64_t)(chunk_len - diff) > 0ULL)
    {
      ite = 64U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)(chunk_len - diff) % (uint64_t)64U);
    }
    uint32_t n_blocks = (chunk_len - diff - ite) / 64U;
    uint32_t data1_len = n_blocks * 64U;
    uint32_t data2_len = chunk_len - diff - data1_len;
    uint8_t *data1 = chunk2;
    uint8_t *data2 = chunk2 + data1_len;
    Hacl_Hash_SHA2_sha256_update_nblocks(data1_len / 64U * 64U, data1, block_state1);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)(chunk_len - diff)
        }
      );
  }
  return Hacl_Streaming_Types_Success;
}

/**
Feed an arbitrary amount of data into the hash. This function returns 0 for
success, or 1 if the combined length of all of the data passed to `update_256`
(since the last call to `reset_256`) exceeds 2^61-1 bytes.

This function is identical to the update function for SHA2_224.
*/
Hacl_Streaming_Types_error_code
Hacl_Hash_SHA2_update_256(
  Hacl_Streaming_MD_state_32 *state,
  uint8_t *input,
  uint32_t input_len
)
{
  return update_224_256(state, input, input_len);
}

/**
Write the resulting hash into `output`, an array of 32 bytes. The state remains
valid after a call to `digest_256`, meaning the user may feed more data into
the hash via `update_256`. (The digest_256 function operates on an internal copy of
the state and therefore does not invalidate the client-held state `p`.)
*/
void Hacl_Hash_SHA2_digest_256(Hacl_Streaming_MD_state_32 *state, uint8_t *output)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint32_t *block_state = scrut.block_state;
  uint8_t *buf_ = scrut.buf;
  uint64_t total_len = scrut.total_len;
  uint32_t r;
  if (total_len % (uint64_t)64U == 0ULL && total_len > 0ULL)
  {
    r = 64U;
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)64U);
  }
  uint8_t *buf_1 = buf_;
  uint32_t tmp_block_state[8U] = { 0U };
  memcpy(tmp_block_state, block_state, 8U * sizeof (uint32_t));
  uint32_t ite;
  if (r % 64U == 0U && r > 0U)
  {
    ite = 64U;
  }
  else
  {
    ite = r % 64U;
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  Hacl_Hash_SHA2_sha256_update_nblocks(0U, buf_multi, tmp_block_state);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  Hacl_Hash_SHA2_sha256_update_last(prev_len_last + (uint64_t)r, r, buf_last, tmp_block_state);
  Hacl_Hash_SHA2_sha256_finish(tmp_block_state, output);
}

/**
Free a state allocated with `malloc_256`.

This function is identical to the free function for SHA2_224.
*/
void Hacl_Hash_SHA2_free_256(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_HOST_FREE(block_state);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(state);
}

/**
Hash `input`, of len `input_len`, into `output`, an array of 32 bytes.
*/
void Hacl_Hash_SHA2_hash_256(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint32_t st[8U] = { 0U };
  Hacl_Hash_SHA2_sha256_init(st);
  uint32_t rem = input_len % 64U;
  uint64_t len_ = (uint64_t)input_len;
  Hacl_Hash_SHA2_sha256_update_nblocks(input_len, ib, st);
  uint32_t rem1 = input_len % 64U;
  uint8_t *b0 = ib;
  uint8_t *lb = b0 + input_len - rem1;
  Hacl_Hash_SHA2_sha256_update_last(len_, rem, lb, st);
  Hacl_Hash_SHA2_sha256_finish(st, rb);
}

Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA2_malloc_224(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(64U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  uint8_t *buf1 = buf;
  uint32_t *b = (uint32_t *)KRML_HOST_CALLOC(8U, sizeof (uint32_t));
  Hacl_Streaming_Types_optional_32 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf1);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint32_t *block_state1 = block_state.v;
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_32
          s = { .block_state = block_state1, .buf = buf1, .total_len = (uint64_t)0U };
          Hacl_Streaming_MD_state_32
          *p = (Hacl_Streaming_MD_state_32 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_32));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
            KRML_HOST_FREE(buf1);
            return NULL;
          }
          Hacl_Hash_SHA2_sha224_init(block_state1);
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

void Hacl_Hash_SHA2_reset_224(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  Hacl_Hash_SHA2_sha224_init(block_state);
  Hacl_Streaming_MD_state_32
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  state[0U] = tmp;
}

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA2_update_224(
  Hacl_Streaming_MD_state_32 *state,
  uint8_t *input,
  uint32_t input_len
)
{
  return update_224_256(state, input, input_len);
}

/**
Write the resulting hash into `output`, an array of 28 bytes. The state remains
valid after a call to `digest_224`, meaning the user may feed more data into
the hash via `update_224`.
*/
void Hacl_Hash_SHA2_digest_224(Hacl_Streaming_MD_state_32 *state, uint8_t *output)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint32_t *block_state = scrut.block_state;
  uint8_t *buf_ = scrut.buf;
  uint64_t total_len = scrut.total_len;
  uint32_t r;
  if (total_len % (uint64_t)64U == 0ULL && total_len > 0ULL)
  {
    r = 64U;
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)64U);
  }
  uint8_t *buf_1 = buf_;
  uint32_t tmp_block_state[8U] = { 0U };
  memcpy(tmp_block_state, block_state, 8U * sizeof (uint32_t));
  uint32_t ite;
  if (r % 64U == 0U && r > 0U)
  {
    ite = 64U;
  }
  else
  {
    ite = r % 64U;
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  Hacl_Hash_SHA2_sha224_update_nblocks(0U, buf_multi, tmp_block_state);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  Hacl_Hash_SHA2_sha224_update_last(prev_len_last + (uint64_t)r, r, buf_last, tmp_block_state);
  Hacl_Hash_SHA2_sha224_finish(tmp_block_state, output);
}

void Hacl_Hash_SHA2_free_224(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Hash_SHA2_free_256(state);
}

/**
Hash `input`, of len `input_len`, into `output`, an array of 28 bytes.
*/
void Hacl_Hash_SHA2_hash_224(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint32_t st[8U] = { 0U };
  Hacl_Hash_SHA2_sha224_init(st);
  uint32_t rem = input_len % 64U;
  uint64_t len_ = (uint64_t)input_len;
  Hacl_Hash_SHA2_sha224_update_nblocks(input_len, ib, st);
  uint32_t rem1 = input_len % 64U;
  uint8_t *b0 = ib;
  uint8_t *lb = b0 + input_len - rem1;
  Hacl_Hash_SHA2_sha224_update_last(len_, rem, lb, st);
  Hacl_Hash_SHA2_sha224_finish(st, rb);
}

Hacl_Streaming_MD_state_64 *Hacl_Hash_SHA2_malloc_512(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(128U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  uint8_t *buf1 = buf;
  uint64_t *b = (uint64_t *)KRML_HOST_CALLOC(8U, sizeof (uint64_t));
  Hacl_Streaming_Types_optional_64 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_64){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_64){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf1);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint64_t *block_state1 = block_state.v;
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_64
          s = { .block_state = block_state1, .buf = buf1, .total_len = (uint64_t)0U };
          Hacl_Streaming_MD_state_64
          *p = (Hacl_Streaming_MD_state_64 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_64));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
            KRML_HOST_FREE(buf1);
            return NULL;
          }
          Hacl_Hash_SHA2_sha512_init(block_state1);
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

/**
Copies the state passed as argument into a newly allocated state (deep copy).
The state is to be freed by calling `free_512`. Cloning the state this way is
useful, for instance, if your control-flow diverges and you need to feed
more (different) data into the hash in each branch.
*/
Hacl_Streaming_MD_state_64 *Hacl_Hash_SHA2_copy_512(Hacl_Streaming_MD_state_64 *state)
{
  Hacl_Streaming_MD_state_64 scrut = *state;
  uint64_t *block_state0 = scrut.block_state;
  uint8_t *buf0 = scrut.buf;
  uint64_t total_len0 = scrut.total_len;
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(128U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, buf0, 128U * sizeof (uint8_t));
  uint64_t *b = (uint64_t *)KRML_HOST_CALLOC(8U, sizeof (uint64_t));
  Hacl_Streaming_Types_optional_64 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_64){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_64){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint64_t *block_state1 = block_state.v;
    memcpy(block_state1, block_state0, 8U * sizeof (uint64_t));
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_64
          s = { .block_state = block_state1, .buf = buf, .total_len = total_len0 };
          Hacl_Streaming_MD_state_64
          *p = (Hacl_Streaming_MD_state_64 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_64));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
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

void Hacl_Hash_SHA2_reset_512(Hacl_Streaming_MD_state_64 *state)
{
  Hacl_Streaming_MD_state_64 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint64_t *block_state = scrut.block_state;
  Hacl_Hash_SHA2_sha512_init(block_state);
  Hacl_Streaming_MD_state_64
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  state[0U] = tmp;
}

static inline Hacl_Streaming_Types_error_code
update_384_512(Hacl_Streaming_MD_state_64 *state, uint8_t *chunk, uint32_t chunk_len)
{
  Hacl_Streaming_MD_state_64 s = *state;
  uint64_t total_len = s.total_len;
  if ((uint64_t)chunk_len > 18446744073709551615ULL - total_len)
  {
    return Hacl_Streaming_Types_MaximumLengthExceeded;
  }
  uint32_t sz;
  if (total_len % (uint64_t)128U == 0ULL && total_len > 0ULL)
  {
    sz = 128U;
  }
  else
  {
    sz = (uint32_t)(total_len % (uint64_t)128U);
  }
  if (chunk_len <= 128U - sz)
  {
    Hacl_Streaming_MD_state_64 s1 = *state;
    uint64_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)128U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 128U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)128U);
    }
    uint8_t *buf2 = buf + sz1;
    memcpy(buf2, chunk, chunk_len * sizeof (uint8_t));
    uint64_t total_len2 = total_len1 + (uint64_t)chunk_len;
    *state =
      (
        (Hacl_Streaming_MD_state_64){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len2
        }
      );
  }
  else if (sz == 0U)
  {
    Hacl_Streaming_MD_state_64 s1 = *state;
    uint64_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)128U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 128U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)128U);
    }
    if (!(sz1 == 0U))
    {
      Hacl_Hash_SHA2_sha512_update_nblocks(128U, buf, block_state1);
    }
    uint32_t ite;
    if ((uint64_t)chunk_len % (uint64_t)128U == 0ULL && (uint64_t)chunk_len > 0ULL)
    {
      ite = 128U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)chunk_len % (uint64_t)128U);
    }
    uint32_t n_blocks = (chunk_len - ite) / 128U;
    uint32_t data1_len = n_blocks * 128U;
    uint32_t data2_len = chunk_len - data1_len;
    uint8_t *data1 = chunk;
    uint8_t *data2 = chunk + data1_len;
    Hacl_Hash_SHA2_sha512_update_nblocks(data1_len / 128U * 128U, data1, block_state1);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Streaming_MD_state_64){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)chunk_len
        }
      );
  }
  else
  {
    uint32_t diff = 128U - sz;
    uint8_t *chunk1 = chunk;
    uint8_t *chunk2 = chunk + diff;
    Hacl_Streaming_MD_state_64 s1 = *state;
    uint64_t *block_state10 = s1.block_state;
    uint8_t *buf0 = s1.buf;
    uint64_t total_len10 = s1.total_len;
    uint32_t sz10;
    if (total_len10 % (uint64_t)128U == 0ULL && total_len10 > 0ULL)
    {
      sz10 = 128U;
    }
    else
    {
      sz10 = (uint32_t)(total_len10 % (uint64_t)128U);
    }
    uint8_t *buf2 = buf0 + sz10;
    memcpy(buf2, chunk1, diff * sizeof (uint8_t));
    uint64_t total_len2 = total_len10 + (uint64_t)diff;
    *state =
      (
        (Hacl_Streaming_MD_state_64){
          .block_state = block_state10,
          .buf = buf0,
          .total_len = total_len2
        }
      );
    Hacl_Streaming_MD_state_64 s10 = *state;
    uint64_t *block_state1 = s10.block_state;
    uint8_t *buf = s10.buf;
    uint64_t total_len1 = s10.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)128U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 128U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)128U);
    }
    if (!(sz1 == 0U))
    {
      Hacl_Hash_SHA2_sha512_update_nblocks(128U, buf, block_state1);
    }
    uint32_t ite;
    if
    ((uint64_t)(chunk_len - diff) % (uint64_t)128U == 0ULL && (uint64_t)(chunk_len - diff) > 0ULL)
    {
      ite = 128U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)(chunk_len - diff) % (uint64_t)128U);
    }
    uint32_t n_blocks = (chunk_len - diff - ite) / 128U;
    uint32_t data1_len = n_blocks * 128U;
    uint32_t data2_len = chunk_len - diff - data1_len;
    uint8_t *data1 = chunk2;
    uint8_t *data2 = chunk2 + data1_len;
    Hacl_Hash_SHA2_sha512_update_nblocks(data1_len / 128U * 128U, data1, block_state1);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Streaming_MD_state_64){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)(chunk_len - diff)
        }
      );
  }
  return Hacl_Streaming_Types_Success;
}

/**
Feed an arbitrary amount of data into the hash. This function returns 0 for
success, or 1 if the combined length of all of the data passed to `update_512`
(since the last call to `reset_512`) exceeds 2^125-1 bytes.

This function is identical to the update function for SHA2_384.
*/
Hacl_Streaming_Types_error_code
Hacl_Hash_SHA2_update_512(
  Hacl_Streaming_MD_state_64 *state,
  uint8_t *input,
  uint32_t input_len
)
{
  return update_384_512(state, input, input_len);
}

/**
Write the resulting hash into `output`, an array of 64 bytes. The state remains
valid after a call to `digest_512`, meaning the user may feed more data into
the hash via `update_512`. (The digest_512 function operates on an internal copy of
the state and therefore does not invalidate the client-held state `p`.)
*/
void Hacl_Hash_SHA2_digest_512(Hacl_Streaming_MD_state_64 *state, uint8_t *output)
{
  Hacl_Streaming_MD_state_64 scrut = *state;
  uint64_t *block_state = scrut.block_state;
  uint8_t *buf_ = scrut.buf;
  uint64_t total_len = scrut.total_len;
  uint32_t r;
  if (total_len % (uint64_t)128U == 0ULL && total_len > 0ULL)
  {
    r = 128U;
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)128U);
  }
  uint8_t *buf_1 = buf_;
  uint64_t tmp_block_state[8U] = { 0U };
  memcpy(tmp_block_state, block_state, 8U * sizeof (uint64_t));
  uint32_t ite;
  if (r % 128U == 0U && r > 0U)
  {
    ite = 128U;
  }
  else
  {
    ite = r % 128U;
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  Hacl_Hash_SHA2_sha512_update_nblocks(0U, buf_multi, tmp_block_state);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  Hacl_Hash_SHA2_sha512_update_last(FStar_UInt128_add(FStar_UInt128_uint64_to_uint128(prev_len_last),
      FStar_UInt128_uint64_to_uint128((uint64_t)r)),
    r,
    buf_last,
    tmp_block_state);
  Hacl_Hash_SHA2_sha512_finish(tmp_block_state, output);
}

/**
Free a state allocated with `malloc_512`.

This function is identical to the free function for SHA2_384.
*/
void Hacl_Hash_SHA2_free_512(Hacl_Streaming_MD_state_64 *state)
{
  Hacl_Streaming_MD_state_64 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint64_t *block_state = scrut.block_state;
  KRML_HOST_FREE(block_state);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(state);
}

/**
Hash `input`, of len `input_len`, into `output`, an array of 64 bytes.
*/
void Hacl_Hash_SHA2_hash_512(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t st[8U] = { 0U };
  Hacl_Hash_SHA2_sha512_init(st);
  uint32_t rem = input_len % 128U;
  FStar_UInt128_uint128 len_ = FStar_UInt128_uint64_to_uint128((uint64_t)input_len);
  Hacl_Hash_SHA2_sha512_update_nblocks(input_len, ib, st);
  uint32_t rem1 = input_len % 128U;
  uint8_t *b0 = ib;
  uint8_t *lb = b0 + input_len - rem1;
  Hacl_Hash_SHA2_sha512_update_last(len_, rem, lb, st);
  Hacl_Hash_SHA2_sha512_finish(st, rb);
}

Hacl_Streaming_MD_state_64 *Hacl_Hash_SHA2_malloc_384(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(128U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  uint8_t *buf1 = buf;
  uint64_t *b = (uint64_t *)KRML_HOST_CALLOC(8U, sizeof (uint64_t));
  Hacl_Streaming_Types_optional_64 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_64){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_64){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf1);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint64_t *block_state1 = block_state.v;
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_64
          s = { .block_state = block_state1, .buf = buf1, .total_len = (uint64_t)0U };
          Hacl_Streaming_MD_state_64
          *p = (Hacl_Streaming_MD_state_64 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_64));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
            KRML_HOST_FREE(buf1);
            return NULL;
          }
          Hacl_Hash_SHA2_sha384_init(block_state1);
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

void Hacl_Hash_SHA2_reset_384(Hacl_Streaming_MD_state_64 *state)
{
  Hacl_Streaming_MD_state_64 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint64_t *block_state = scrut.block_state;
  Hacl_Hash_SHA2_sha384_init(block_state);
  Hacl_Streaming_MD_state_64
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  state[0U] = tmp;
}

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA2_update_384(
  Hacl_Streaming_MD_state_64 *state,
  uint8_t *input,
  uint32_t input_len
)
{
  return update_384_512(state, input, input_len);
}

/**
Write the resulting hash into `output`, an array of 48 bytes. The state remains
valid after a call to `digest_384`, meaning the user may feed more data into
the hash via `update_384`.
*/
void Hacl_Hash_SHA2_digest_384(Hacl_Streaming_MD_state_64 *state, uint8_t *output)
{
  Hacl_Streaming_MD_state_64 scrut = *state;
  uint64_t *block_state = scrut.block_state;
  uint8_t *buf_ = scrut.buf;
  uint64_t total_len = scrut.total_len;
  uint32_t r;
  if (total_len % (uint64_t)128U == 0ULL && total_len > 0ULL)
  {
    r = 128U;
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)128U);
  }
  uint8_t *buf_1 = buf_;
  uint64_t tmp_block_state[8U] = { 0U };
  memcpy(tmp_block_state, block_state, 8U * sizeof (uint64_t));
  uint32_t ite;
  if (r % 128U == 0U && r > 0U)
  {
    ite = 128U;
  }
  else
  {
    ite = r % 128U;
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  Hacl_Hash_SHA2_sha384_update_nblocks(0U, buf_multi, tmp_block_state);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  Hacl_Hash_SHA2_sha384_update_last(FStar_UInt128_add(FStar_UInt128_uint64_to_uint128(prev_len_last),
      FStar_UInt128_uint64_to_uint128((uint64_t)r)),
    r,
    buf_last,
    tmp_block_state);
  Hacl_Hash_SHA2_sha384_finish(tmp_block_state, output);
}

void Hacl_Hash_SHA2_free_384(Hacl_Streaming_MD_state_64 *state)
{
  Hacl_Hash_SHA2_free_512(state);
}

/**
Hash `input`, of len `input_len`, into `output`, an array of 48 bytes.
*/
void Hacl_Hash_SHA2_hash_384(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  uint8_t *ib = input;
  uint8_t *rb = output;
  uint64_t st[8U] = { 0U };
  Hacl_Hash_SHA2_sha384_init(st);
  uint32_t rem = input_len % 128U;
  FStar_UInt128_uint128 len_ = FStar_UInt128_uint64_to_uint128((uint64_t)input_len);
  Hacl_Hash_SHA2_sha384_update_nblocks(input_len, ib, st);
  uint32_t rem1 = input_len % 128U;
  uint8_t *b0 = ib;
  uint8_t *lb = b0 + input_len - rem1;
  Hacl_Hash_SHA2_sha384_update_last(len_, rem, lb, st);
  Hacl_Hash_SHA2_sha384_finish(st, rb);
}

