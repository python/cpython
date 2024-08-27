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


#ifndef __internal_Hacl_Hash_Blake2s_Simd128_H
#define __internal_Hacl_Hash_Blake2s_Simd128_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "internal/Hacl_Impl_Blake2_Constants.h"
#include "internal/Hacl_Hash_Blake2b.h"
#include "../Hacl_Hash_Blake2s_Simd128.h"
#include "libintvector.h"

void
Hacl_Hash_Blake2s_Simd128_init(Lib_IntVector_Intrinsics_vec128 *hash, uint32_t kk, uint32_t nn);

void
Hacl_Hash_Blake2s_Simd128_update_multi(
  uint32_t len,
  Lib_IntVector_Intrinsics_vec128 *wv,
  Lib_IntVector_Intrinsics_vec128 *hash,
  uint64_t prev,
  uint8_t *blocks,
  uint32_t nb
);

void
Hacl_Hash_Blake2s_Simd128_update_last(
  uint32_t len,
  Lib_IntVector_Intrinsics_vec128 *wv,
  Lib_IntVector_Intrinsics_vec128 *hash,
  bool last_node,
  uint64_t prev,
  uint32_t rem,
  uint8_t *d
);

void
Hacl_Hash_Blake2s_Simd128_finish(
  uint32_t nn,
  uint8_t *output,
  Lib_IntVector_Intrinsics_vec128 *hash
);

void
Hacl_Hash_Blake2s_Simd128_store_state128s_to_state32(
  uint32_t *st32,
  Lib_IntVector_Intrinsics_vec128 *st
);

void
Hacl_Hash_Blake2s_Simd128_load_state128s_from_state32(
  Lib_IntVector_Intrinsics_vec128 *st,
  uint32_t *st32
);

Lib_IntVector_Intrinsics_vec128 *Hacl_Hash_Blake2s_Simd128_malloc_with_key(void);

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_Hash_Blake2s_Simd128_H_DEFINED
#endif
