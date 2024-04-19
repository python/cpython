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


#ifndef __Hacl_Hash_Blake2b_H
#define __Hacl_Hash_Blake2b_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "python_hacl_namespaces.h"
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"


typedef struct Hacl_Hash_Blake2s_blake2_params_s
{
  uint8_t digest_length;
  uint8_t key_length;
  uint8_t fanout;
  uint8_t depth;
  uint32_t leaf_length;
  uint64_t node_offset;
  uint8_t node_depth;
  uint8_t inner_length;
  uint8_t *salt;
  uint8_t *personal;
}
Hacl_Hash_Blake2s_blake2_params;

typedef struct Hacl_Hash_Blake2b_block_state_t_s
{
  uint64_t *fst;
  uint64_t *snd;
}
Hacl_Hash_Blake2b_block_state_t;

typedef struct Hacl_Hash_Blake2b_state_t_s
{
  Hacl_Hash_Blake2b_block_state_t block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Hash_Blake2b_state_t;

/**
  State allocation function when there is no key
*/
Hacl_Hash_Blake2b_state_t *Hacl_Hash_Blake2b_malloc(void);

/**
  State allocation function when there are parameters but no key
*/
Hacl_Hash_Blake2b_state_t
*Hacl_Hash_Blake2b_malloc_with_params(Hacl_Hash_Blake2s_blake2_params *key);

/**
  Re-initialization function when there is no key
*/
void Hacl_Hash_Blake2b_reset(Hacl_Hash_Blake2b_state_t *state);

/**
  Re-initialization function when there are parameters but no key
*/
void
Hacl_Hash_Blake2b_reset_with_params(
  Hacl_Hash_Blake2b_state_t *state,
  Hacl_Hash_Blake2s_blake2_params *key
);

/**
  Update function when there is no key; 0 = success, 1 = max length exceeded
*/
Hacl_Streaming_Types_error_code
Hacl_Hash_Blake2b_update(Hacl_Hash_Blake2b_state_t *state, uint8_t *chunk, uint32_t chunk_len);

/**
  Finish function when there is no key
*/
void Hacl_Hash_Blake2b_digest(Hacl_Hash_Blake2b_state_t *state, uint8_t *output);

/**
  Free state function when there is no key
*/
void Hacl_Hash_Blake2b_free(Hacl_Hash_Blake2b_state_t *state);

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
Hacl_Hash_Blake2b_hash_with_key(
  uint8_t *output,
  uint32_t output_len,
  uint8_t *input,
  uint32_t input_len,
  uint8_t *key,
  uint32_t key_len
);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Hash_Blake2b_H_DEFINED
#endif
