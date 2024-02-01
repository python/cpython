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


#ifndef __Hacl_Hash_SHA3_H
#define __Hacl_Hash_SHA3_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"

typedef struct Hacl_Streaming_Keccak_hash_buf_s
{
  Spec_Hash_Definitions_hash_alg fst;
  uint64_t *snd;
}
Hacl_Streaming_Keccak_hash_buf;

typedef struct Hacl_Streaming_Keccak_state_s
{
  Hacl_Streaming_Keccak_hash_buf block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Streaming_Keccak_state;

Spec_Hash_Definitions_hash_alg Hacl_Streaming_Keccak_get_alg(Hacl_Streaming_Keccak_state *s);

Hacl_Streaming_Keccak_state *Hacl_Streaming_Keccak_malloc(Spec_Hash_Definitions_hash_alg a);

void Hacl_Streaming_Keccak_free(Hacl_Streaming_Keccak_state *s);

Hacl_Streaming_Keccak_state *Hacl_Streaming_Keccak_copy(Hacl_Streaming_Keccak_state *s0);

void Hacl_Streaming_Keccak_reset(Hacl_Streaming_Keccak_state *s);

Hacl_Streaming_Types_error_code
Hacl_Streaming_Keccak_update(Hacl_Streaming_Keccak_state *p, uint8_t *data, uint32_t len);

Hacl_Streaming_Types_error_code
Hacl_Streaming_Keccak_finish(Hacl_Streaming_Keccak_state *s, uint8_t *dst);

Hacl_Streaming_Types_error_code
Hacl_Streaming_Keccak_squeeze(Hacl_Streaming_Keccak_state *s, uint8_t *dst, uint32_t l);

uint32_t Hacl_Streaming_Keccak_block_len(Hacl_Streaming_Keccak_state *s);

uint32_t Hacl_Streaming_Keccak_hash_len(Hacl_Streaming_Keccak_state *s);

bool Hacl_Streaming_Keccak_is_shake(Hacl_Streaming_Keccak_state *s);

void
Hacl_SHA3_shake128_hacl(
  uint32_t inputByteLen,
  uint8_t *input,
  uint32_t outputByteLen,
  uint8_t *output
);

void
Hacl_SHA3_shake256_hacl(
  uint32_t inputByteLen,
  uint8_t *input,
  uint32_t outputByteLen,
  uint8_t *output
);

void Hacl_SHA3_sha3_224(uint32_t inputByteLen, uint8_t *input, uint8_t *output);

void Hacl_SHA3_sha3_256(uint32_t inputByteLen, uint8_t *input, uint8_t *output);

void Hacl_SHA3_sha3_384(uint32_t inputByteLen, uint8_t *input, uint8_t *output);

void Hacl_SHA3_sha3_512(uint32_t inputByteLen, uint8_t *input, uint8_t *output);

void Hacl_Impl_SHA3_absorb_inner(uint32_t rateInBytes, uint8_t *block, uint64_t *s);

void
Hacl_Impl_SHA3_squeeze(
  uint64_t *s,
  uint32_t rateInBytes,
  uint32_t outputByteLen,
  uint8_t *output
);

void
Hacl_Impl_SHA3_keccak(
  uint32_t rate,
  uint32_t capacity,
  uint32_t inputByteLen,
  uint8_t *input,
  uint8_t delimitedSuffix,
  uint32_t outputByteLen,
  uint8_t *output
);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Hash_SHA3_H_DEFINED
#endif
