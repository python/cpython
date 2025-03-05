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


#ifndef __Hacl_Hash_Blake2b_Simd256_H
#define __Hacl_Hash_Blake2b_Simd256_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "python_hacl_namespaces.h"

#include "Hacl_Streaming_Types.h"

#include "Hacl_Hash_Blake2b.h"

/**
 General-purpose allocation function that gives control over all
Blake2 parameters, including the key. Further resettings of the state SHALL be
done with `reset_with_params_and_key`, and SHALL feature the exact same values
for the `key_length` and `digest_length` fields as passed here. In other words,
once you commit to a digest and key length, the only way to change these
parameters is to allocate a new object.

The caller must satisfy the following requirements.
- The length of the key k MUST match the value of the field key_length in the
  parameters.
- The key_length must not exceed 256 for S, 64 for B.
- The digest_length must not exceed 256 for S, 64 for B.

*/
void
*Hacl_Hash_Blake2b_Simd256_malloc_with_params_and_key(
  Hacl_Hash_Blake2b_blake2_params *p,
  bool last_node,
  uint8_t *k
);

/**
  Update function; 0 = success, 1 = max length exceeded
*/
Hacl_Streaming_Types_error_code
Hacl_Hash_Blake2b_Simd256_update(
  void *state,
  uint8_t *chunk,
  uint32_t chunk_len
);

/**
 Digest function. This function expects the `output` array to hold
at least `digest_length` bytes, where `digest_length` was determined by your
choice of `malloc` function. Concretely, if you used `malloc` or
`malloc_with_key`, then the expected length is 256 for S, or 64 for B (default
digest length). If you used `malloc_with_params_and_key`, then the expected
length is whatever you chose for the `digest_length` field of your parameters.
For convenience, this function returns `digest_length`. When in doubt, callers
can pass an array of size HACL_BLAKE2B_256_OUT_BYTES, then use the return value
to see how many bytes were actually written.
*/
uint8_t Hacl_Hash_Blake2b_Simd256_digest(void *s, uint8_t *dst);

Hacl_Hash_Blake2b_index Hacl_Hash_Blake2b_Simd256_info(void *s);

/**
  Free state function when there is no key
*/
void Hacl_Hash_Blake2b_Simd256_free(void *state);

/**
  Copying. This preserves all parameters.
*/
void
*Hacl_Hash_Blake2b_Simd256_copy(void *state);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Hash_Blake2b_Simd256_H_DEFINED
#endif
