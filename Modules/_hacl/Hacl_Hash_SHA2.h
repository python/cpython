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


#ifndef __Hacl_Hash_SHA2_H
#define __Hacl_Hash_SHA2_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "python_hacl_namespaces.h"
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"

typedef Hacl_Streaming_MD_state_32 Hacl_Hash_SHA2_state_t_224;

typedef Hacl_Streaming_MD_state_32 Hacl_Hash_SHA2_state_t_256;

typedef Hacl_Streaming_MD_state_64 Hacl_Hash_SHA2_state_t_384;

typedef Hacl_Streaming_MD_state_64 Hacl_Hash_SHA2_state_t_512;

/**
Allocate initial state for the SHA2_256 hash. The state is to be freed by
calling `free_256`.
*/
Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA2_malloc_256(void);

/**
Copies the state passed as argument into a newly allocated state (deep copy).
The state is to be freed by calling `free_256`. Cloning the state this way is
useful, for instance, if your control-flow diverges and you need to feed
more (different) data into the hash in each branch.
*/
Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA2_copy_256(Hacl_Streaming_MD_state_32 *state);

/**
Reset an existing state to the initial hash state with empty data.
*/
void Hacl_Hash_SHA2_reset_256(Hacl_Streaming_MD_state_32 *state);

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
);

/**
Write the resulting hash into `output`, an array of 32 bytes. The state remains
valid after a call to `digest_256`, meaning the user may feed more data into
the hash via `update_256`. (The digest_256 function operates on an internal copy of
the state and therefore does not invalidate the client-held state `p`.)
*/
void Hacl_Hash_SHA2_digest_256(Hacl_Streaming_MD_state_32 *state, uint8_t *output);

/**
Free a state allocated with `malloc_256`.

This function is identical to the free function for SHA2_224.
*/
void Hacl_Hash_SHA2_free_256(Hacl_Streaming_MD_state_32 *state);

/**
Hash `input`, of len `input_len`, into `output`, an array of 32 bytes.
*/
void Hacl_Hash_SHA2_hash_256(uint8_t *output, uint8_t *input, uint32_t input_len);

Hacl_Streaming_MD_state_32 *Hacl_Hash_SHA2_malloc_224(void);

void Hacl_Hash_SHA2_reset_224(Hacl_Streaming_MD_state_32 *state);

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA2_update_224(
  Hacl_Streaming_MD_state_32 *state,
  uint8_t *input,
  uint32_t input_len
);

/**
Write the resulting hash into `output`, an array of 28 bytes. The state remains
valid after a call to `digest_224`, meaning the user may feed more data into
the hash via `update_224`.
*/
void Hacl_Hash_SHA2_digest_224(Hacl_Streaming_MD_state_32 *state, uint8_t *output);

void Hacl_Hash_SHA2_free_224(Hacl_Streaming_MD_state_32 *state);

/**
Hash `input`, of len `input_len`, into `output`, an array of 28 bytes.
*/
void Hacl_Hash_SHA2_hash_224(uint8_t *output, uint8_t *input, uint32_t input_len);

Hacl_Streaming_MD_state_64 *Hacl_Hash_SHA2_malloc_512(void);

/**
Copies the state passed as argument into a newly allocated state (deep copy).
The state is to be freed by calling `free_512`. Cloning the state this way is
useful, for instance, if your control-flow diverges and you need to feed
more (different) data into the hash in each branch.
*/
Hacl_Streaming_MD_state_64 *Hacl_Hash_SHA2_copy_512(Hacl_Streaming_MD_state_64 *state);

void Hacl_Hash_SHA2_reset_512(Hacl_Streaming_MD_state_64 *state);

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
);

/**
Write the resulting hash into `output`, an array of 64 bytes. The state remains
valid after a call to `digest_512`, meaning the user may feed more data into
the hash via `update_512`. (The digest_512 function operates on an internal copy of
the state and therefore does not invalidate the client-held state `p`.)
*/
void Hacl_Hash_SHA2_digest_512(Hacl_Streaming_MD_state_64 *state, uint8_t *output);

/**
Free a state allocated with `malloc_512`.

This function is identical to the free function for SHA2_384.
*/
void Hacl_Hash_SHA2_free_512(Hacl_Streaming_MD_state_64 *state);

/**
Hash `input`, of len `input_len`, into `output`, an array of 64 bytes.
*/
void Hacl_Hash_SHA2_hash_512(uint8_t *output, uint8_t *input, uint32_t input_len);

Hacl_Streaming_MD_state_64 *Hacl_Hash_SHA2_malloc_384(void);

void Hacl_Hash_SHA2_reset_384(Hacl_Streaming_MD_state_64 *state);

Hacl_Streaming_Types_error_code
Hacl_Hash_SHA2_update_384(
  Hacl_Streaming_MD_state_64 *state,
  uint8_t *input,
  uint32_t input_len
);

/**
Write the resulting hash into `output`, an array of 48 bytes. The state remains
valid after a call to `digest_384`, meaning the user may feed more data into
the hash via `update_384`.
*/
void Hacl_Hash_SHA2_digest_384(Hacl_Streaming_MD_state_64 *state, uint8_t *output);

void Hacl_Hash_SHA2_free_384(Hacl_Streaming_MD_state_64 *state);

/**
Hash `input`, of len `input_len`, into `output`, an array of 48 bytes.
*/
void Hacl_Hash_SHA2_hash_384(uint8_t *output, uint8_t *input, uint32_t input_len);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Hash_SHA2_H_DEFINED
#endif
