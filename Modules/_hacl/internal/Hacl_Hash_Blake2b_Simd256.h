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


#ifndef internal_Hacl_Hash_Blake2b_Simd256_H
#define internal_Hacl_Hash_Blake2b_Simd256_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "../Hacl_Hash_Blake2b_Simd256.h"
#include "libintvector.h"

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

Lib_IntVector_Intrinsics_vec256
*Hacl_Hash_Blake2b_Simd256_malloc_internal_state_with_key(void);

void
Hacl_Hash_Blake2b_Simd256_update_multi_no_inline(
  Lib_IntVector_Intrinsics_vec256 *s,
  FStar_UInt128_uint128 ev,
  uint8_t *blocks,
  uint32_t n
);

void
Hacl_Hash_Blake2b_Simd256_update_last_no_inline(
  Lib_IntVector_Intrinsics_vec256 *s,
  FStar_UInt128_uint128 prev,
  uint8_t *input,
  uint32_t input_len
);

void
Hacl_Hash_Blake2b_Simd256_copy_internal_state(
  Lib_IntVector_Intrinsics_vec256 *src,
  Lib_IntVector_Intrinsics_vec256 *dst
);

typedef struct Hacl_Hash_Blake2b_Simd256_two_2b_256_s
{
  Lib_IntVector_Intrinsics_vec256 *fst;
  Lib_IntVector_Intrinsics_vec256 *snd;
}
Hacl_Hash_Blake2b_Simd256_two_2b_256;

typedef struct Hacl_Hash_Blake2b_Simd256_block_state_t_s
{
  uint8_t fst;
  uint8_t snd;
  bool thd;
  Hacl_Hash_Blake2b_Simd256_two_2b_256 f3;
}
Hacl_Hash_Blake2b_Simd256_block_state_t;

typedef struct Hacl_Hash_Blake2b_Simd256_state_t_s
{
  Hacl_Hash_Blake2b_Simd256_block_state_t block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Hash_Blake2b_Simd256_state_t;

#if defined(__cplusplus)
}
#endif

#define internal_Hacl_Hash_Blake2b_Simd256_H_DEFINED
#endif /* internal_Hacl_Hash_Blake2b_Simd256_H */
