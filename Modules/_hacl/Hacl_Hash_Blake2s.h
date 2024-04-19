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


#ifndef __Hacl_Hash_Blake2s_H
#define __Hacl_Hash_Blake2s_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "python_hacl_namespaces.h"
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"
#include "Hacl_Hash_Blake2b.h"

typedef struct Hacl_Hash_Blake2s_block_state_t_s
{
  uint32_t *fst;
  uint32_t *snd;
}
Hacl_Hash_Blake2s_block_state_t;

typedef struct Hacl_Hash_Blake2s_state_t_s
{
  Hacl_Hash_Blake2s_block_state_t block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Hash_Blake2s_state_t;

/**
  State allocation function when there is no key
*/
Hacl_Hash_Blake2s_state_t *Hacl_Hash_Blake2s_malloc(void);

/**
  State allocation function when there are parameters but no key
*/
Hacl_Hash_Blake2s_state_t
*Hacl_Hash_Blake2s_malloc_with_params(Hacl_Hash_Blake2s_blake2_params *key);

/**
  Re-initialization function when there is no key
*/
void Hacl_Hash_Blake2s_reset(Hacl_Hash_Blake2s_state_t *state);

/**
  Re-initialization function when there are parameters but no key
*/
void
Hacl_Hash_Blake2s_reset_with_params(
  Hacl_Hash_Blake2s_state_t *state,
  Hacl_Hash_Blake2s_blake2_params *key
);

/**
  Update function when there is no key; 0 = success, 1 = max length exceeded
*/
Hacl_Streaming_Types_error_code
Hacl_Hash_Blake2s_update(Hacl_Hash_Blake2s_state_t *state, uint8_t *chunk, uint32_t chunk_len);

/**
  Finish function when there is no key
*/
void Hacl_Hash_Blake2s_digest(Hacl_Hash_Blake2s_state_t *state, uint8_t *output);

/**
  Free state function when there is no key
*/
void Hacl_Hash_Blake2s_free(Hacl_Hash_Blake2s_state_t *state);

/**
Write the BLAKE2s digest of message `input` using key `key` into `output`.

@param output Pointer to `output_len` bytes of memory where the digest is written to.
@param output_len Length of the to-be-generated digest with 1 <= `output_len` <= 32.
@param input Pointer to `input_len` bytes of memory where the input message is read from.
@param input_len Length of the input message.
@param key Pointer to `key_len` bytes of memory where the key is read from.
@param key_len Length of the key. Can be 0.
*/
void
Hacl_Hash_Blake2s_hash_with_key(
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

#define __Hacl_Hash_Blake2s_H_DEFINED
#endif
