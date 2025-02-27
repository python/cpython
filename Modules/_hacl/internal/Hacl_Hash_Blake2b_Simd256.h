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


#ifndef __internal_Hacl_Hash_Blake2b_Simd256_H
#define __internal_Hacl_Hash_Blake2b_Simd256_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "internal/Hacl_Impl_Blake2_Constants.h"
#include "internal/Hacl_Hash_Blake2b.h"
#include "../Hacl_Hash_Blake2b_Simd256.h"
#include "libintvector.h"

#define HACL_HASH_BLAKE2B_SIMD256_BLOCK_BYTES (128U)

#define HACL_HASH_BLAKE2B_SIMD256_OUT_BYTES (64U)

#define HACL_HASH_BLAKE2B_SIMD256_KEY_BYTES (64U)

#define HACL_HASH_BLAKE2B_SIMD256_SALT_BYTES (16U)

#define HACL_HASH_BLAKE2B_SIMD256_PERSONAL_BYTES (16U)

typedef struct K____Lib_IntVector_Intrinsics_vec256___Lib_IntVector_Intrinsics_vec256__s
{
  Lib_IntVector_Intrinsics_vec256 *fst;
  Lib_IntVector_Intrinsics_vec256 *snd;
}
K____Lib_IntVector_Intrinsics_vec256___Lib_IntVector_Intrinsics_vec256_;

typedef struct Hacl_Hash_Blake2b_Simd256_block_state_t_s
{
  uint8_t fst;
  uint8_t snd;
  bool thd;
  K____Lib_IntVector_Intrinsics_vec256___Lib_IntVector_Intrinsics_vec256_ f3;
}
Hacl_Hash_Blake2b_Simd256_block_state_t;

typedef struct Hacl_Hash_Blake2b_Simd256_state_t_s
{
  Hacl_Hash_Blake2b_Simd256_block_state_t block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Hash_Blake2b_Simd256_state_t;

/**
 Specialized allocation function that picks default values for all
parameters, except for the key_length. Further resettings of the state SHALL be
done with `reset_with_key`, and SHALL feature the exact same key length `kk` as
passed here. In other words, once you commit to a key length, the only way to
change this parameter is to allocate a new object.

The caller must satisfy the following requirements.
- The key_length must not exceed 256 for S, 64 for B.

*/
Hacl_Hash_Blake2b_Simd256_state_t
*Hacl_Hash_Blake2b_Simd256_malloc_with_key0(uint8_t *k, uint8_t kk);

/**
 Specialized allocation function that picks default values for all
parameters, and has no key. Effectively, this is what you want if you intend to
use Blake2 as a hash function. Further resettings of the state SHALL be done with `reset`.
*/
Hacl_Hash_Blake2b_Simd256_state_t *Hacl_Hash_Blake2b_Simd256_malloc(void);

/**
 General-purpose re-initialization function with parameters and
key. You cannot change digest_length, key_length, or last_node, meaning those values in
the parameters object must be the same as originally decided via one of the
malloc functions. All other values of the parameter can be changed. The behavior
is unspecified if you violate this precondition.
*/
void
Hacl_Hash_Blake2b_Simd256_reset_with_key_and_params(
  Hacl_Hash_Blake2b_Simd256_state_t *s,
  Hacl_Hash_Blake2b_blake2_params *p,
  uint8_t *k
);

/**
 Specialized-purpose re-initialization function with no parameters,
and a key. The key length must be the same as originally decided via your choice
of malloc function. All other parameters are reset to their default values. The
original call to malloc MUST have set digest_length to the default value. The
behavior is unspecified if you violate this precondition.
*/
void
Hacl_Hash_Blake2b_Simd256_reset_with_key(Hacl_Hash_Blake2b_Simd256_state_t *s, uint8_t *k);

/**
 Specialized-purpose re-initialization function with no parameters
and no key. This is what you want if you intend to use Blake2 as a hash
function. The key length and digest length must have been set to their
respective default values via your choice of malloc function (always true if you
used `malloc`). All other parameters are reset to their default values. The
behavior is unspecified if you violate this precondition.
*/
void Hacl_Hash_Blake2b_Simd256_reset(Hacl_Hash_Blake2b_Simd256_state_t *s);

/**
Write the BLAKE2b digest of message `input` using key `key` into `output`.

@param output Pointer to `output_len` bytes of memory where the digest is written to.
@param output_len Length of the to-be-generated digest with 1 <= `output_len` <= 64.
@param input Pointer to `input_len` bytes of memory where the input message is read from.
@param input_len Length of the input message.
@param key Pointer to `key_len` bytes of memory where the key is read from.
@param key_len Length of the key. Can be 0.
*/
void
Hacl_Hash_Blake2b_Simd256_hash_with_key(
  uint8_t *output,
  uint32_t output_len,
  uint8_t *input,
  uint32_t input_len,
  uint8_t *key,
  uint32_t key_len
);

/**
Write the BLAKE2b digest of message `input` using key `key` and
parameters `params` into `output`. The `key` array must be of length
`params.key_length`. The `output` array must be of length
`params.digest_length`.
*/
void
Hacl_Hash_Blake2b_Simd256_hash_with_key_and_params(
  uint8_t *output,
  uint8_t *input,
  uint32_t input_len,
  Hacl_Hash_Blake2b_blake2_params params,
  uint8_t *key
);

void
Hacl_Hash_Blake2b_Simd256_init(Lib_IntVector_Intrinsics_vec256 *hash, uint32_t kk, uint32_t nn);

void
Hacl_Hash_Blake2b_Simd256_update_multi(
  uint32_t len,
  Lib_IntVector_Intrinsics_vec256 *wv,
  Lib_IntVector_Intrinsics_vec256 *hash,
  FStar_UInt128_uint128 prev,
  uint8_t *blocks,
  uint32_t nb
);

void
Hacl_Hash_Blake2b_Simd256_update_last(
  uint32_t len,
  Lib_IntVector_Intrinsics_vec256 *wv,
  Lib_IntVector_Intrinsics_vec256 *hash,
  bool last_node,
  FStar_UInt128_uint128 prev,
  uint32_t rem,
  uint8_t *d
);

void
Hacl_Hash_Blake2b_Simd256_finish(
  uint32_t nn,
  uint8_t *output,
  Lib_IntVector_Intrinsics_vec256 *hash
);

void
Hacl_Hash_Blake2b_Simd256_load_state256b_from_state32(
  Lib_IntVector_Intrinsics_vec256 *st,
  uint64_t *st32
);

void
Hacl_Hash_Blake2b_Simd256_store_state256b_to_state32(
  uint64_t *st32,
  Lib_IntVector_Intrinsics_vec256 *st
);

Lib_IntVector_Intrinsics_vec256 *Hacl_Hash_Blake2b_Simd256_malloc_with_key(void);

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_Hash_Blake2b_Simd256_H_DEFINED
#endif
