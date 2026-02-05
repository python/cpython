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


#ifndef internal_Hacl_Hash_SHA3_H
#define internal_Hacl_Hash_SHA3_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"
#include "../Hacl_Hash_SHA3.h"

extern const uint32_t Hacl_Hash_SHA3_keccak_rotc[24U];

extern const uint32_t Hacl_Hash_SHA3_keccak_piln[24U];

extern const uint64_t Hacl_Hash_SHA3_keccak_rndc[24U];

void Hacl_Hash_SHA3_init_(Spec_Hash_Definitions_hash_alg a, uint64_t *s);

void
Hacl_Hash_SHA3_update_multi_sha3(
  Spec_Hash_Definitions_hash_alg a,
  uint64_t *s,
  uint8_t *blocks,
  uint32_t n_blocks
);

void
Hacl_Hash_SHA3_update_last_sha3(
  Spec_Hash_Definitions_hash_alg a,
  uint64_t *s,
  uint8_t *input,
  uint32_t input_len
);

typedef struct Hacl_Hash_SHA3_hash_buf_s
{
  Spec_Hash_Definitions_hash_alg fst;
  uint64_t *snd;
}
Hacl_Hash_SHA3_hash_buf;

typedef struct Hacl_Hash_SHA3_state_t_s
{
  Hacl_Hash_SHA3_hash_buf block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Hash_SHA3_state_t;

#if defined(__cplusplus)
}
#endif

#define internal_Hacl_Hash_SHA3_H_DEFINED
#endif /* internal_Hacl_Hash_SHA3_H */
