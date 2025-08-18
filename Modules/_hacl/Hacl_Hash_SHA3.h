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
#include "python_hacl_namespaces.h"
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"

typedef struct Hacl_Hash_SHA3_state_t_s Hacl_Hash_SHA3_state_t;

Spec_Hash_Definitions_hash_alg Hacl_Hash_SHA3_get_alg(Hacl_Hash_SHA3_state_t *s);

Hacl_Hash_SHA3_state_t *Hacl_Hash_SHA3_malloc(Spec_Hash_Definitions_hash_alg a);

void Hacl_Hash_SHA3_free(Hacl_Hash_SHA3_state_t *state);

Hacl_Hash_SHA3_state_t *Hacl_Hash_SHA3_copy(Hacl_Hash_SHA3_state_t *state);

void Hacl_Hash_SHA3_reset(Hacl_Hash_SHA3_state_t *state);

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA3_update(Hacl_Hash_SHA3_state_t *state, uint8_t *chunk, uint32_t chunk_len);

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA3_digest(Hacl_Hash_SHA3_state_t *state, uint8_t *output);

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA3_squeeze(Hacl_Hash_SHA3_state_t *s, uint8_t *dst, uint32_t l);

uint32_t Hacl_Hash_SHA3_block_len(Hacl_Hash_SHA3_state_t *s);

uint32_t Hacl_Hash_SHA3_hash_len(Hacl_Hash_SHA3_state_t *s);

bool Hacl_Hash_SHA3_is_shake(Hacl_Hash_SHA3_state_t *s);

void Hacl_Hash_SHA3_absorb_inner_32(uint32_t rateInBytes, uint8_t *b, uint64_t *s);

void
Hacl_Hash_SHA3_shake128(
  uint8_t *output,
  uint32_t outputByteLen,
  uint8_t *input,
  uint32_t inputByteLen
);

void
Hacl_Hash_SHA3_shake256(
  uint8_t *output,
  uint32_t outputByteLen,
  uint8_t *input,
  uint32_t inputByteLen
);

void Hacl_Hash_SHA3_sha3_224(uint8_t *output, uint8_t *input, uint32_t inputByteLen);

void Hacl_Hash_SHA3_sha3_256(uint8_t *output, uint8_t *input, uint32_t inputByteLen);

void Hacl_Hash_SHA3_sha3_384(uint8_t *output, uint8_t *input, uint32_t inputByteLen);

void Hacl_Hash_SHA3_sha3_512(uint8_t *output, uint8_t *input, uint32_t inputByteLen);

/**
Allocate state buffer of 200-bytes
*/
uint64_t *Hacl_Hash_SHA3_state_malloc(void);

/**
Free state buffer
*/
void Hacl_Hash_SHA3_state_free(uint64_t *s);

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
Hacl_Hash_SHA3_shake128_absorb_nblocks(uint64_t *state, uint8_t *input, uint32_t inputByteLen);

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
Hacl_Hash_SHA3_shake128_absorb_final(uint64_t *state, uint8_t *input, uint32_t inputByteLen);

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
);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Hash_SHA3_H_DEFINED
#endif
