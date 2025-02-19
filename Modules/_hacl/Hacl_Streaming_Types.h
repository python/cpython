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


#ifndef __Hacl_Streaming_Types_H
#define __Hacl_Streaming_Types_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "libintvector.h"

#define Spec_Hash_Definitions_SHA2_224 0
#define Spec_Hash_Definitions_SHA2_256 1
#define Spec_Hash_Definitions_SHA2_384 2
#define Spec_Hash_Definitions_SHA2_512 3
#define Spec_Hash_Definitions_SHA1 4
#define Spec_Hash_Definitions_MD5 5
#define Spec_Hash_Definitions_Blake2S 6
#define Spec_Hash_Definitions_Blake2B 7
#define Spec_Hash_Definitions_SHA3_256 8
#define Spec_Hash_Definitions_SHA3_224 9
#define Spec_Hash_Definitions_SHA3_384 10
#define Spec_Hash_Definitions_SHA3_512 11
#define Spec_Hash_Definitions_Shake128 12
#define Spec_Hash_Definitions_Shake256 13

typedef uint8_t Spec_Hash_Definitions_hash_alg;

typedef struct Hacl_Streaming_MD_state_32_s
{
  uint32_t *block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Streaming_MD_state_32;

typedef struct Hacl_Streaming_MD_state_64_s
{
  uint64_t *block_state;
  uint8_t *buf;
  uint64_t total_len;
}
Hacl_Streaming_MD_state_64;

typedef struct K____uint64_t___uint64_t__s
{
  uint64_t *fst;
  uint64_t *snd;
}
K____uint64_t___uint64_t_;

typedef struct Hacl_Streaming_Blake2_Types_block_state_blake2b_32_s
{
  uint8_t fst;
  uint8_t snd;
  bool thd;
  K____uint64_t___uint64_t_ f3;
}
Hacl_Streaming_Blake2_Types_block_state_blake2b_32;

#define Hacl_Streaming_Blake2_Types_None 0
#define Hacl_Streaming_Blake2_Types_Some 1

typedef uint8_t Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_32_tags;

typedef struct Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_32_s
{
  Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_32_tags tag;
  Hacl_Streaming_Blake2_Types_block_state_blake2b_32 v;
}
Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_32;

typedef struct K____Lib_IntVector_Intrinsics_vec256___Lib_IntVector_Intrinsics_vec256__s
{
  Lib_IntVector_Intrinsics_vec256 *fst;
  Lib_IntVector_Intrinsics_vec256 *snd;
}
K____Lib_IntVector_Intrinsics_vec256___Lib_IntVector_Intrinsics_vec256_;

typedef struct Hacl_Streaming_Blake2_Types_block_state_blake2b_256_s
{
  uint8_t fst;
  uint8_t snd;
  bool thd;
  K____Lib_IntVector_Intrinsics_vec256___Lib_IntVector_Intrinsics_vec256_ f3;
}
Hacl_Streaming_Blake2_Types_block_state_blake2b_256;

typedef struct Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_256_s
{
  Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_32_tags tag;
  Hacl_Streaming_Blake2_Types_block_state_blake2b_256 v;
}
Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_256;

typedef struct K____uint32_t___uint32_t__s
{
  uint32_t *fst;
  uint32_t *snd;
}
K____uint32_t___uint32_t_;

typedef struct Hacl_Streaming_Blake2_Types_block_state_blake2s_32_s
{
  uint8_t fst;
  uint8_t snd;
  bool thd;
  K____uint32_t___uint32_t_ f3;
}
Hacl_Streaming_Blake2_Types_block_state_blake2s_32;

typedef struct Hacl_Streaming_Blake2_Types_optional_block_state_blake2s_32_s
{
  Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_32_tags tag;
  Hacl_Streaming_Blake2_Types_block_state_blake2s_32 v;
}
Hacl_Streaming_Blake2_Types_optional_block_state_blake2s_32;

typedef struct K____Lib_IntVector_Intrinsics_vec128___Lib_IntVector_Intrinsics_vec128__s
{
  Lib_IntVector_Intrinsics_vec128 *fst;
  Lib_IntVector_Intrinsics_vec128 *snd;
}
K____Lib_IntVector_Intrinsics_vec128___Lib_IntVector_Intrinsics_vec128_;

typedef struct Hacl_Streaming_Blake2_Types_block_state_blake2s_128_s
{
  uint8_t fst;
  uint8_t snd;
  bool thd;
  K____Lib_IntVector_Intrinsics_vec128___Lib_IntVector_Intrinsics_vec128_ f3;
}
Hacl_Streaming_Blake2_Types_block_state_blake2s_128;

typedef struct Hacl_Streaming_Blake2_Types_optional_block_state_blake2s_128_s
{
  Hacl_Streaming_Blake2_Types_optional_block_state_blake2b_32_tags tag;
  Hacl_Streaming_Blake2_Types_block_state_blake2s_128 v;
}
Hacl_Streaming_Blake2_Types_optional_block_state_blake2s_128;

#define Hacl_Streaming_Types_Success 0
#define Hacl_Streaming_Types_InvalidAlgorithm 1
#define Hacl_Streaming_Types_InvalidLength 2
#define Hacl_Streaming_Types_MaximumLengthExceeded 3
#define Hacl_Streaming_Types_OutOfMemory 4

typedef uint8_t Hacl_Streaming_Types_error_code;

typedef K____uint64_t___uint64_t_ Hacl_Streaming_Types_two_pointers;

#if defined(__cplusplus)
}
#endif

#define __Hacl_Streaming_Types_H_DEFINED
#endif
