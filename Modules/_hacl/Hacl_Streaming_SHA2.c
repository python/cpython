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


#include "Hacl_Streaming_SHA2.h"

#include "internal/Hacl_SHA2_Generic.h"


static inline void sha256_init(uint32_t *hash)
{
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = hash;
    uint32_t x = Hacl_Impl_SHA2_Generic_h256[i];
    os[i] = x;);
}

static inline void sha256_update0(uint8_t *b, uint32_t *hash)
{
  uint32_t hash_old[8U] = { 0U };
  uint32_t ws[16U] = { 0U };
  memcpy(hash_old, hash, (uint32_t)8U * sizeof (uint32_t));
  uint8_t *b10 = b;
  uint32_t u = load32_be(b10);
  ws[0U] = u;
  uint32_t u0 = load32_be(b10 + (uint32_t)4U);
  ws[1U] = u0;
  uint32_t u1 = load32_be(b10 + (uint32_t)8U);
  ws[2U] = u1;
  uint32_t u2 = load32_be(b10 + (uint32_t)12U);
  ws[3U] = u2;
  uint32_t u3 = load32_be(b10 + (uint32_t)16U);
  ws[4U] = u3;
  uint32_t u4 = load32_be(b10 + (uint32_t)20U);
  ws[5U] = u4;
  uint32_t u5 = load32_be(b10 + (uint32_t)24U);
  ws[6U] = u5;
  uint32_t u6 = load32_be(b10 + (uint32_t)28U);
  ws[7U] = u6;
  uint32_t u7 = load32_be(b10 + (uint32_t)32U);
  ws[8U] = u7;
  uint32_t u8 = load32_be(b10 + (uint32_t)36U);
  ws[9U] = u8;
  uint32_t u9 = load32_be(b10 + (uint32_t)40U);
  ws[10U] = u9;
  uint32_t u10 = load32_be(b10 + (uint32_t)44U);
  ws[11U] = u10;
  uint32_t u11 = load32_be(b10 + (uint32_t)48U);
  ws[12U] = u11;
  uint32_t u12 = load32_be(b10 + (uint32_t)52U);
  ws[13U] = u12;
  uint32_t u13 = load32_be(b10 + (uint32_t)56U);
  ws[14U] = u13;
  uint32_t u14 = load32_be(b10 + (uint32_t)60U);
  ws[15U] = u14;
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
        h02
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
      hash[0U] = a1;
      hash[1U] = b1;
      hash[2U] = c1;
      hash[3U] = d1;
      hash[4U] = e1;
      hash[5U] = f1;
      hash[6U] = g1;
      hash[7U] = h12;);
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
    uint32_t *os = hash;
    uint32_t x = hash[i] + hash_old[i];
    os[i] = x;);
}

static inline void sha256_update_nblocks(uint32_t len, uint8_t *b, uint32_t *st)
{
  uint32_t blocks = len / (uint32_t)64U;
  for (uint32_t i = (uint32_t)0U; i < blocks; i++)
  {
    uint8_t *b0 = b;
    uint8_t *mb = b0 + i * (uint32_t)64U;
    sha256_update0(mb, st);
  }
}

static inline void
sha256_update_last(uint64_t totlen, uint32_t len, uint8_t *b, uint32_t *hash)
{
  uint32_t blocks;
  if (len + (uint32_t)8U + (uint32_t)1U <= (uint32_t)64U)
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
  uint64_t total_len_bits = totlen << (uint32_t)3U;
  store64_be(totlen_buf, total_len_bits);
  uint8_t *b0 = b;
  memcpy(last, b0, len * sizeof (uint8_t));
  last[len] = (uint8_t)0x80U;
  memcpy(last + fin - (uint32_t)8U, totlen_buf, (uint32_t)8U * sizeof (uint8_t));
  uint8_t *last00 = last;
  uint8_t *last10 = last + (uint32_t)64U;
  uint8_t *l0 = last00;
  uint8_t *l1 = last10;
  uint8_t *lb0 = l0;
  uint8_t *lb1 = l1;
  uint8_t *last0 = lb0;
  uint8_t *last1 = lb1;
  sha256_update0(last0, hash);
  if (blocks > (uint32_t)1U)
  {
    sha256_update0(last1, hash);
    return;
  }
}

static inline void sha256_finish(uint32_t *st, uint8_t *h)
{
  uint8_t hbuf[32U] = { 0U };
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    store32_be(hbuf + i * (uint32_t)4U, st[i]););
  memcpy(h, hbuf, (uint32_t)32U * sizeof (uint8_t));
}

static inline void sha224_init(uint32_t *hash)
{
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    uint32_t *os = hash;
    uint32_t x = Hacl_Impl_SHA2_Generic_h224[i];
    os[i] = x;);
}

static inline void sha224_update_nblocks(uint32_t len, uint8_t *b, uint32_t *st)
{
  sha256_update_nblocks(len, b, st);
}

static void sha224_update_last(uint64_t totlen, uint32_t len, uint8_t *b, uint32_t *st)
{
  sha256_update_last(totlen, len, b, st);
}

static inline void sha224_finish(uint32_t *st, uint8_t *h)
{
  uint8_t hbuf[32U] = { 0U };
  KRML_MAYBE_FOR8(i,
    (uint32_t)0U,
    (uint32_t)8U,
    (uint32_t)1U,
    store32_be(hbuf + i * (uint32_t)4U, st[i]););
  memcpy(h, hbuf, (uint32_t)28U * sizeof (uint8_t));
}

/**
Allocate initial state for the SHA2_256 hash. The state is to be freed by
calling `free_256`.
*/
Hacl_Streaming_SHA2_state_sha2_224 *Hacl_Streaming_SHA2_create_in_256(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC((uint32_t)64U, sizeof (uint8_t));
  uint32_t *block_state = (uint32_t *)KRML_HOST_CALLOC((uint32_t)8U, sizeof (uint32_t));
  Hacl_Streaming_SHA2_state_sha2_224
  s = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)(uint32_t)0U };
  Hacl_Streaming_SHA2_state_sha2_224
  *p =
    (Hacl_Streaming_SHA2_state_sha2_224 *)KRML_HOST_MALLOC(sizeof (
        Hacl_Streaming_SHA2_state_sha2_224
      ));
  p[0U] = s;
  sha256_init(block_state);
  return p;
}

/**
Copies the state passed as argument into a newly allocated state (deep copy).
The state is to be freed by calling `free_256`. Cloning the state this way is
useful, for instance, if your control-flow diverges and you need to feed
more (different) data into the hash in each branch.
*/
Hacl_Streaming_SHA2_state_sha2_224
*Hacl_Streaming_SHA2_copy_256(Hacl_Streaming_SHA2_state_sha2_224 *s0)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s0;
  uint32_t *block_state0 = scrut.block_state;
  uint8_t *buf0 = scrut.buf;
  uint64_t total_len0 = scrut.total_len;
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC((uint32_t)64U, sizeof (uint8_t));
  memcpy(buf, buf0, (uint32_t)64U * sizeof (uint8_t));
  uint32_t *block_state = (uint32_t *)KRML_HOST_CALLOC((uint32_t)8U, sizeof (uint32_t));
  memcpy(block_state, block_state0, (uint32_t)8U * sizeof (uint32_t));
  Hacl_Streaming_SHA2_state_sha2_224
  s = { .block_state = block_state, .buf = buf, .total_len = total_len0 };
  Hacl_Streaming_SHA2_state_sha2_224
  *p =
    (Hacl_Streaming_SHA2_state_sha2_224 *)KRML_HOST_MALLOC(sizeof (
        Hacl_Streaming_SHA2_state_sha2_224
      ));
  p[0U] = s;
  return p;
}

/**
Reset an existing state to the initial hash state with empty data.
*/
void Hacl_Streaming_SHA2_init_256(Hacl_Streaming_SHA2_state_sha2_224 *s)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  sha256_init(block_state);
  Hacl_Streaming_SHA2_state_sha2_224
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)(uint32_t)0U };
  s[0U] = tmp;
}

static inline uint32_t
update_224_256(Hacl_Streaming_SHA2_state_sha2_224 *p, uint8_t *data, uint32_t len)
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
      sha256_update_nblocks((uint32_t)64U, buf, block_state1);
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
    sha256_update_nblocks(data1_len, data1, block_state1);
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
      sha256_update_nblocks((uint32_t)64U, buf, block_state1);
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
    sha256_update_nblocks(data1_len, data11, block_state1);
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

/**
Feed an arbitrary amount of data into the hash. This function returns 0 for
success, or 1 if the combined length of all of the data passed to `update_256`
(since the last call to `init_256`) exceeds 2^61-1 bytes.

This function is identical to the update function for SHA2_224.
*/
uint32_t
Hacl_Streaming_SHA2_update_256(
  Hacl_Streaming_SHA2_state_sha2_224 *p,
  uint8_t *input,
  uint32_t input_len
)
{
  return update_224_256(p, input, input_len);
}

/**
Write the resulting hash into `dst`, an array of 32 bytes. The state remains
valid after a call to `finish_256`, meaning the user may feed more data into
the hash via `update_256`. (The finish_256 function operates on an internal copy of
the state and therefore does not invalidate the client-held state `p`.)
*/
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
  sha256_update_nblocks((uint32_t)0U, buf_multi, tmp_block_state);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  sha256_update_last(prev_len_last + (uint64_t)r, r, buf_last, tmp_block_state);
  sha256_finish(tmp_block_state, dst);
}

/**
Free a state allocated with `create_in_256`.

This function is identical to the free function for SHA2_224.
*/
void Hacl_Streaming_SHA2_free_256(Hacl_Streaming_SHA2_state_sha2_224 *s)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_HOST_FREE(block_state);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(s);
}

/**
Hash `input`, of len `input_len`, into `dst`, an array of 32 bytes.
*/
void Hacl_Streaming_SHA2_sha256(uint8_t *input, uint32_t input_len, uint8_t *dst)
{
  uint8_t *ib = input;
  uint8_t *rb = dst;
  uint32_t st[8U] = { 0U };
  sha256_init(st);
  uint32_t rem = input_len % (uint32_t)64U;
  uint64_t len_ = (uint64_t)input_len;
  sha256_update_nblocks(input_len, ib, st);
  uint32_t rem1 = input_len % (uint32_t)64U;
  uint8_t *b0 = ib;
  uint8_t *lb = b0 + input_len - rem1;
  sha256_update_last(len_, rem, lb, st);
  sha256_finish(st, rb);
}

Hacl_Streaming_SHA2_state_sha2_224 *Hacl_Streaming_SHA2_create_in_224(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC((uint32_t)64U, sizeof (uint8_t));
  uint32_t *block_state = (uint32_t *)KRML_HOST_CALLOC((uint32_t)8U, sizeof (uint32_t));
  Hacl_Streaming_SHA2_state_sha2_224
  s = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)(uint32_t)0U };
  Hacl_Streaming_SHA2_state_sha2_224
  *p =
    (Hacl_Streaming_SHA2_state_sha2_224 *)KRML_HOST_MALLOC(sizeof (
        Hacl_Streaming_SHA2_state_sha2_224
      ));
  p[0U] = s;
  sha224_init(block_state);
  return p;
}

void Hacl_Streaming_SHA2_init_224(Hacl_Streaming_SHA2_state_sha2_224 *s)
{
  Hacl_Streaming_SHA2_state_sha2_224 scrut = *s;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  sha224_init(block_state);
  Hacl_Streaming_SHA2_state_sha2_224
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)(uint32_t)0U };
  s[0U] = tmp;
}

uint32_t
Hacl_Streaming_SHA2_update_224(
  Hacl_Streaming_SHA2_state_sha2_224 *p,
  uint8_t *input,
  uint32_t input_len
)
{
  return update_224_256(p, input, input_len);
}

/**
Write the resulting hash into `dst`, an array of 28 bytes. The state remains
valid after a call to `finish_224`, meaning the user may feed more data into
the hash via `update_224`.
*/
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
  sha224_update_nblocks((uint32_t)0U, buf_multi, tmp_block_state);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  sha224_update_last(prev_len_last + (uint64_t)r, r, buf_last, tmp_block_state);
  sha224_finish(tmp_block_state, dst);
}

void Hacl_Streaming_SHA2_free_224(Hacl_Streaming_SHA2_state_sha2_224 *p)
{
  Hacl_Streaming_SHA2_free_256(p);
}

/**
Hash `input`, of len `input_len`, into `dst`, an array of 28 bytes.
*/
void Hacl_Streaming_SHA2_sha224(uint8_t *input, uint32_t input_len, uint8_t *dst)
{
  uint8_t *ib = input;
  uint8_t *rb = dst;
  uint32_t st[8U] = { 0U };
  sha224_init(st);
  uint32_t rem = input_len % (uint32_t)64U;
  uint64_t len_ = (uint64_t)input_len;
  sha224_update_nblocks(input_len, ib, st);
  uint32_t rem1 = input_len % (uint32_t)64U;
  uint8_t *b0 = ib;
  uint8_t *lb = b0 + input_len - rem1;
  sha224_update_last(len_, rem, lb, st);
  sha224_finish(st, rb);
}

