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

void
Hacl_Hash_SHA3_update_multi_sha3(
  Spec_Hash_Definitions_hash_alg a,
  uint64_t *s,
  uint8_t *blocks,
  uint32_t n_blocks
)
{
  for (uint32_t i = 0U; i < n_blocks; i++)
  {
    uint8_t *block = blocks + i * block_len(a);
    Hacl_Hash_SHA3_absorb_inner(block_len(a), block, s);
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
    Hacl_Hash_SHA3_absorb_inner(len, input, s);
    uint8_t lastBlock_[200U] = { 0U };
    uint8_t *lastBlock = lastBlock_;
    memcpy(lastBlock, input + input_len, 0U * sizeof (uint8_t));
    lastBlock[0U] = suffix;
    Hacl_Hash_SHA3_loadState(len, lastBlock, s);
    if (!(((uint32_t)suffix & 0x80U) == 0U) && 0U == len - 1U)
    {
      Hacl_Hash_SHA3_state_permute(s);
    }
    uint8_t nextBlock_[200U] = { 0U };
    uint8_t *nextBlock = nextBlock_;
    nextBlock[len - 1U] = 0x80U;
    Hacl_Hash_SHA3_loadState(len, nextBlock, s);
    Hacl_Hash_SHA3_state_permute(s);
    return;
  }
  uint8_t lastBlock_[200U] = { 0U };
  uint8_t *lastBlock = lastBlock_;
  memcpy(lastBlock, input, input_len * sizeof (uint8_t));
  lastBlock[input_len] = suffix;
  Hacl_Hash_SHA3_loadState(len, lastBlock, s);
  if (!(((uint32_t)suffix & 0x80U) == 0U) && input_len == len - 1U)
  {
    Hacl_Hash_SHA3_state_permute(s);
  }
  uint8_t nextBlock_[200U] = { 0U };
  uint8_t *nextBlock = nextBlock_;
  nextBlock[len - 1U] = 0x80U;
  Hacl_Hash_SHA3_loadState(len, nextBlock, s);
  Hacl_Hash_SHA3_state_permute(s);
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

Hacl_Hash_SHA3_state_t *Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_hash_alg a)
{
  KRML_CHECK_SIZE(sizeof (uint8_t), block_len(a));
  uint8_t *buf0 = (uint8_t *)KRML_HOST_CALLOC(block_len(a), sizeof (uint8_t));
  uint64_t *buf = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
  Hacl_Hash_SHA3_hash_buf block_state = { .fst = a, .snd = buf };
  Hacl_Hash_SHA3_state_t
  s = { .block_state = block_state, .buf = buf0, .total_len = (uint64_t)0U };
  Hacl_Hash_SHA3_state_t
  *p = (Hacl_Hash_SHA3_state_t *)KRML_HOST_MALLOC(sizeof (Hacl_Hash_SHA3_state_t));
  p[0U] = s;
  uint64_t *s1 = block_state.snd;
  memset(s1, 0U, 25U * sizeof (uint64_t));
  return p;
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
  uint8_t *buf1 = (uint8_t *)KRML_HOST_CALLOC(block_len(i), sizeof (uint8_t));
  memcpy(buf1, buf0, block_len(i) * sizeof (uint8_t));
  uint64_t *buf = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
  Hacl_Hash_SHA3_hash_buf block_state = { .fst = i, .snd = buf };
  hash_buf2 scrut = { .fst = block_state0, .snd = block_state };
  uint64_t *s_dst = scrut.snd.snd;
  uint64_t *s_src = scrut.fst.snd;
  memcpy(s_dst, s_src, 25U * sizeof (uint64_t));
  Hacl_Hash_SHA3_state_t
  s = { .block_state = block_state, .buf = buf1, .total_len = total_len0 };
  Hacl_Hash_SHA3_state_t
  *p = (Hacl_Hash_SHA3_state_t *)KRML_HOST_MALLOC(sizeof (Hacl_Hash_SHA3_state_t));
  p[0U] = s;
  return p;
}

void Hacl_Hash_SHA3_reset(Hacl_Hash_SHA3_state_t *state)
{
  Hacl_Hash_SHA3_state_t scrut = *state;
  uint8_t *buf = scrut.buf;
  Hacl_Hash_SHA3_hash_buf block_state = scrut.block_state;
  Spec_Hash_Definitions_hash_alg i = block_state.fst;
  KRML_MAYBE_UNUSED_VAR(i);
  uint64_t *s = block_state.snd;
  memset(s, 0U, 25U * sizeof (uint64_t));
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
    *state
    =
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
    *state
    =
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
    *state
    =
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
      (uint64_t)(chunk_len - diff)
      % (uint64_t)block_len(i)
      == 0ULL
      && (uint64_t)(chunk_len - diff) > 0ULL
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
    *state
    =
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
  if (a11 == Spec_Hash_Definitions_Shake128 || a11 == Spec_Hash_Definitions_Shake256)
  {
    Hacl_Hash_SHA3_squeeze0(s, block_len(a11), l, output);
    return;
  }
  Hacl_Hash_SHA3_squeeze0(s, block_len(a11), hash_len(a11), output);
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

void
Hacl_Hash_SHA3_shake128_hacl(
  uint32_t inputByteLen,
  uint8_t *input,
  uint32_t outputByteLen,
  uint8_t *output
)
{
  Hacl_Hash_SHA3_keccak(1344U, 256U, inputByteLen, input, 0x1FU, outputByteLen, output);
}

void
Hacl_Hash_SHA3_shake256_hacl(
  uint32_t inputByteLen,
  uint8_t *input,
  uint32_t outputByteLen,
  uint8_t *output
)
{
  Hacl_Hash_SHA3_keccak(1088U, 512U, inputByteLen, input, 0x1FU, outputByteLen, output);
}

void Hacl_Hash_SHA3_sha3_224(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  Hacl_Hash_SHA3_keccak(1152U, 448U, input_len, input, 0x06U, 28U, output);
}

void Hacl_Hash_SHA3_sha3_256(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  Hacl_Hash_SHA3_keccak(1088U, 512U, input_len, input, 0x06U, 32U, output);
}

void Hacl_Hash_SHA3_sha3_384(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  Hacl_Hash_SHA3_keccak(832U, 768U, input_len, input, 0x06U, 48U, output);
}

void Hacl_Hash_SHA3_sha3_512(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  Hacl_Hash_SHA3_keccak(576U, 1024U, input_len, input, 0x06U, 64U, output);
}

static const
uint32_t
keccak_rotc[24U] =
  {
    1U, 3U, 6U, 10U, 15U, 21U, 28U, 36U, 45U, 55U, 2U, 14U, 27U, 41U, 56U, 8U, 25U, 43U, 62U, 18U,
    39U, 61U, 20U, 44U
  };

static const
uint32_t
keccak_piln[24U] =
  {
    10U, 7U, 11U, 17U, 18U, 3U, 5U, 16U, 8U, 21U, 24U, 4U, 15U, 23U, 19U, 13U, 12U, 2U, 20U, 14U,
    22U, 9U, 6U, 1U
  };

static const
uint64_t
keccak_rndc[24U] =
  {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
  };

void Hacl_Hash_SHA3_state_permute(uint64_t *s)
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
      uint32_t _Y = keccak_piln[i];
      uint32_t r = keccak_rotc[i];
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
    uint64_t c = keccak_rndc[i0];
    s[0U] = s[0U] ^ c;
  }
}

void Hacl_Hash_SHA3_loadState(uint32_t rateInBytes, uint8_t *input, uint64_t *s)
{
  uint8_t block[200U] = { 0U };
  memcpy(block, input, rateInBytes * sizeof (uint8_t));
  for (uint32_t i = 0U; i < 25U; i++)
  {
    uint64_t u = load64_le(block + i * 8U);
    uint64_t x = u;
    s[i] = s[i] ^ x;
  }
}

static void storeState(uint32_t rateInBytes, uint64_t *s, uint8_t *res)
{
  uint8_t block[200U] = { 0U };
  for (uint32_t i = 0U; i < 25U; i++)
  {
    uint64_t sj = s[i];
    store64_le(block + i * 8U, sj);
  }
  memcpy(res, block, rateInBytes * sizeof (uint8_t));
}

void Hacl_Hash_SHA3_absorb_inner(uint32_t rateInBytes, uint8_t *block, uint64_t *s)
{
  Hacl_Hash_SHA3_loadState(rateInBytes, block, s);
  Hacl_Hash_SHA3_state_permute(s);
}

static void
absorb(
  uint64_t *s,
  uint32_t rateInBytes,
  uint32_t inputByteLen,
  uint8_t *input,
  uint8_t delimitedSuffix
)
{
  uint32_t n_blocks = inputByteLen / rateInBytes;
  uint32_t rem = inputByteLen % rateInBytes;
  for (uint32_t i = 0U; i < n_blocks; i++)
  {
    uint8_t *block = input + i * rateInBytes;
    Hacl_Hash_SHA3_absorb_inner(rateInBytes, block, s);
  }
  uint8_t *last = input + n_blocks * rateInBytes;
  uint8_t lastBlock_[200U] = { 0U };
  uint8_t *lastBlock = lastBlock_;
  memcpy(lastBlock, last, rem * sizeof (uint8_t));
  lastBlock[rem] = delimitedSuffix;
  Hacl_Hash_SHA3_loadState(rateInBytes, lastBlock, s);
  if (!(((uint32_t)delimitedSuffix & 0x80U) == 0U) && rem == rateInBytes - 1U)
  {
    Hacl_Hash_SHA3_state_permute(s);
  }
  uint8_t nextBlock_[200U] = { 0U };
  uint8_t *nextBlock = nextBlock_;
  nextBlock[rateInBytes - 1U] = 0x80U;
  Hacl_Hash_SHA3_loadState(rateInBytes, nextBlock, s);
  Hacl_Hash_SHA3_state_permute(s);
}

void
Hacl_Hash_SHA3_squeeze0(
  uint64_t *s,
  uint32_t rateInBytes,
  uint32_t outputByteLen,
  uint8_t *output
)
{
  uint32_t outBlocks = outputByteLen / rateInBytes;
  uint32_t remOut = outputByteLen % rateInBytes;
  uint8_t *last = output + outputByteLen - remOut;
  uint8_t *blocks = output;
  for (uint32_t i = 0U; i < outBlocks; i++)
  {
    storeState(rateInBytes, s, blocks + i * rateInBytes);
    Hacl_Hash_SHA3_state_permute(s);
  }
  storeState(remOut, s, last);
}

void
Hacl_Hash_SHA3_keccak(
  uint32_t rate,
  uint32_t capacity,
  uint32_t inputByteLen,
  uint8_t *input,
  uint8_t delimitedSuffix,
  uint32_t outputByteLen,
  uint8_t *output
)
{
  KRML_MAYBE_UNUSED_VAR(capacity);
  uint32_t rateInBytes = rate / 8U;
  uint64_t s[25U] = { 0U };
  absorb(s, rateInBytes, inputByteLen, input, delimitedSuffix);
  Hacl_Hash_SHA3_squeeze0(s, rateInBytes, outputByteLen, output);
}

