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


#include "internal/Hacl_Hash_SHA1.h"

static uint32_t _h0[5U] = { 0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U };

void Hacl_Hash_SHA1_init(uint32_t *s)
{
  KRML_MAYBE_FOR5(i, 0U, 5U, 1U, s[i] = _h0[i];);
}

static void update(uint32_t *h, uint8_t *l)
{
  uint32_t ha = h[0U];
  uint32_t hb = h[1U];
  uint32_t hc = h[2U];
  uint32_t hd = h[3U];
  uint32_t he = h[4U];
  uint32_t _w[80U] = { 0U };
  for (uint32_t i = 0U; i < 80U; i++)
  {
    uint32_t v;
    if (i < 16U)
    {
      uint8_t *b = l + i * 4U;
      uint32_t u = load32_be(b);
      v = u;
    }
    else
    {
      uint32_t wmit3 = _w[i - 3U];
      uint32_t wmit8 = _w[i - 8U];
      uint32_t wmit14 = _w[i - 14U];
      uint32_t wmit16 = _w[i - 16U];
      v = (wmit3 ^ (wmit8 ^ (wmit14 ^ wmit16))) << 1U | (wmit3 ^ (wmit8 ^ (wmit14 ^ wmit16))) >> 31U;
    }
    _w[i] = v;
  }
  for (uint32_t i = 0U; i < 80U; i++)
  {
    uint32_t _a = h[0U];
    uint32_t _b = h[1U];
    uint32_t _c = h[2U];
    uint32_t _d = h[3U];
    uint32_t _e = h[4U];
    uint32_t wmit = _w[i];
    uint32_t ite0;
    if (i < 20U)
    {
      ite0 = (_b & _c) ^ (~_b & _d);
    }
    else if (39U < i && i < 60U)
    {
      ite0 = (_b & _c) ^ ((_b & _d) ^ (_c & _d));
    }
    else
    {
      ite0 = _b ^ (_c ^ _d);
    }
    uint32_t ite;
    if (i < 20U)
    {
      ite = 0x5a827999U;
    }
    else if (i < 40U)
    {
      ite = 0x6ed9eba1U;
    }
    else if (i < 60U)
    {
      ite = 0x8f1bbcdcU;
    }
    else
    {
      ite = 0xca62c1d6U;
    }
    uint32_t _T = (_a << 5U | _a >> 27U) + ite0 + _e + ite + wmit;
    h[0U] = _T;
    h[1U] = _a;
    h[2U] = _b << 30U | _b >> 2U;
    h[3U] = _c;
    h[4U] = _d;
  }
  for (uint32_t i = 0U; i < 80U; i++)
  {
    _w[i] = 0U;
  }
  uint32_t sta = h[0U];
  uint32_t stb = h[1U];
  uint32_t stc = h[2U];
  uint32_t std = h[3U];
  uint32_t ste = h[4U];
  h[0U] = sta + ha;
  h[1U] = stb + hb;
  h[2U] = stc + hc;
  h[3U] = std + hd;
  h[4U] = ste + he;
}

static void pad(uint64_t len, uint8_t *dst)
{
  uint8_t *dst1 = dst;
  dst1[0U] = 0x80U;
  uint8_t *dst2 = dst + 1U;
  for (uint32_t i = 0U; i < (128U - (9U + (uint32_t)(len % (uint64_t)64U))) % 64U; i++)
  {
    dst2[i] = 0U;
  }
  uint8_t *dst3 = dst + 1U + (128U - (9U + (uint32_t)(len % (uint64_t)64U))) % 64U;
  store64_be(dst3, len << 3U);
}

void Hacl_Hash_SHA1_finish(uint32_t *s, uint8_t *dst)
{
  KRML_MAYBE_FOR5(i, 0U, 5U, 1U, store32_be(dst + i * 4U, s[i]););
}

void Hacl_Hash_SHA1_update_multi(uint32_t *s, uint8_t *blocks, uint32_t n_blocks)
{
  for (uint32_t i = 0U; i < n_blocks; i++)
  {
    uint32_t sz = 64U;
    uint8_t *block = blocks + sz * i;
    update(s, block);
  }
}

void
Hacl_Hash_SHA1_update_last(uint32_t *s, uint64_t prev_len, uint8_t *input, uint32_t input_len)
{
  uint32_t blocks_n = input_len / 64U;
  uint32_t blocks_len = blocks_n * 64U;
  uint8_t *blocks = input;
  uint32_t rest_len = input_len - blocks_len;
  uint8_t *rest = input + blocks_len;
  Hacl_Hash_SHA1_update_multi(s, blocks, blocks_n);
  uint64_t total_input_len = prev_len + (uint64_t)input_len;
  uint32_t pad_len = 1U + (128U - (9U + (uint32_t)(total_input_len % (uint64_t)64U))) % 64U + 8U;
  uint32_t tmp_len = rest_len + pad_len;
  uint8_t tmp_twoblocks[128U] = { 0U };
  uint8_t *tmp = tmp_twoblocks;
  uint8_t *tmp_rest = tmp;
  uint8_t *tmp_pad = tmp + rest_len;
  memcpy(tmp_rest, rest, rest_len * sizeof (uint8_t));
  pad(total_input_len, tmp_pad);
  Hacl_Hash_SHA1_update_multi(s, tmp, tmp_len / 64U);
}

void Hacl_Hash_SHA1_hash_oneshot(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  uint32_t s[5U] = { 0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U };
  uint32_t blocks_n0 = input_len / 64U;
  uint32_t blocks_n1;
  if (input_len % 64U == 0U && blocks_n0 > 0U)
  {
    blocks_n1 = blocks_n0 - 1U;
  }
  else
  {
    blocks_n1 = blocks_n0;
  }
  uint32_t blocks_len0 = blocks_n1 * 64U;
  uint8_t *blocks0 = input;
  uint32_t rest_len0 = input_len - blocks_len0;
  uint8_t *rest0 = input + blocks_len0;
  uint32_t blocks_n = blocks_n1;
  uint32_t blocks_len = blocks_len0;
  uint8_t *blocks = blocks0;
  uint32_t rest_len = rest_len0;
  uint8_t *rest = rest0;
  Hacl_Hash_SHA1_update_multi(s, blocks, blocks_n);
  Hacl_Hash_SHA1_update_last(s, (uint64_t)blocks_len, rest, rest_len);
  Hacl_Hash_SHA1_finish(s, output);
}

Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA1_malloc(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(64U, sizeof (uint8_t));
  uint32_t *block_state = (uint32_t *)KRML_HOST_CALLOC(5U, sizeof (uint32_t));
  Hacl_Streaming_MD_state_32
  s = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  Hacl_Streaming_MD_state_32
  *p = (Hacl_Streaming_MD_state_32 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_32));
  p[0U] = s;
  Hacl_Hash_SHA1_init(block_state);
  return p;
}

void Hacl_Hash_SHA1_reset(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  Hacl_Hash_SHA1_init(block_state);
  Hacl_Streaming_MD_state_32
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  state[0U] = tmp;
}

/**
0 = success, 1 = max length exceeded
*/
Hacl_Streaming_Types_error_code
Hacl_Hash_SHA1_update(Hacl_Streaming_MD_state_32 *state, uint8_t *chunk, uint32_t chunk_len)
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
    *state
    =
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
      Hacl_Hash_SHA1_update_multi(block_state1, buf, 1U);
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
    Hacl_Hash_SHA1_update_multi(block_state1, data1, data1_len / 64U);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state
    =
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
    *state
    =
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
      Hacl_Hash_SHA1_update_multi(block_state1, buf, 1U);
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
    Hacl_Hash_SHA1_update_multi(block_state1, data1, data1_len / 64U);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state
    =
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

void Hacl_Hash_SHA1_digest(Hacl_Streaming_MD_state_32 *state, uint8_t *output)
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
  uint32_t tmp_block_state[5U] = { 0U };
  memcpy(tmp_block_state, block_state, 5U * sizeof (uint32_t));
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
  Hacl_Hash_SHA1_update_multi(tmp_block_state, buf_multi, 0U);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  Hacl_Hash_SHA1_update_last(tmp_block_state, prev_len_last, buf_last, r);
  Hacl_Hash_SHA1_finish(tmp_block_state, output);
}

void Hacl_Hash_SHA1_free(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_HOST_FREE(block_state);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(state);
}

Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA1_copy(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint32_t *block_state0 = scrut.block_state;
  uint8_t *buf0 = scrut.buf;
  uint64_t total_len0 = scrut.total_len;
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(64U, sizeof (uint8_t));
  memcpy(buf, buf0, 64U * sizeof (uint8_t));
  uint32_t *block_state = (uint32_t *)KRML_HOST_CALLOC(5U, sizeof (uint32_t));
  memcpy(block_state, block_state0, 5U * sizeof (uint32_t));
  Hacl_Streaming_MD_state_32
  s = { .block_state = block_state, .buf = buf, .total_len = total_len0 };
  Hacl_Streaming_MD_state_32
  *p = (Hacl_Streaming_MD_state_32 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_32));
  p[0U] = s;
  return p;
}

void Hacl_Hash_SHA1_hash(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  Hacl_Hash_SHA1_hash_oneshot(output, input, input_len);
}

