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


#ifndef __Hacl_Streaming_HMAC_H
#define __Hacl_Streaming_HMAC_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "python_hacl_namespaces.h"
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"

#define Hacl_Agile_Hash_MD5 0
#define Hacl_Agile_Hash_SHA1 1
#define Hacl_Agile_Hash_SHA2_224 2
#define Hacl_Agile_Hash_SHA2_256 3
#define Hacl_Agile_Hash_SHA2_384 4
#define Hacl_Agile_Hash_SHA2_512 5
#define Hacl_Agile_Hash_SHA3_224 6
#define Hacl_Agile_Hash_SHA3_256 7
#define Hacl_Agile_Hash_SHA3_384 8
#define Hacl_Agile_Hash_SHA3_512 9
#define Hacl_Agile_Hash_Blake2S_32 10
#define Hacl_Agile_Hash_Blake2S_128 11
#define Hacl_Agile_Hash_Blake2B_32 12
#define Hacl_Agile_Hash_Blake2B_256 13

typedef uint8_t Hacl_Agile_Hash_impl;

typedef struct Hacl_Agile_Hash_state_s_s Hacl_Agile_Hash_state_s;

typedef struct Hacl_Streaming_HMAC_Definitions_index_s
{
  Hacl_Agile_Hash_impl fst;
  uint32_t snd;
}
Hacl_Streaming_HMAC_Definitions_index;

typedef struct Hacl_Streaming_HMAC_Definitions_two_state_s
{
  uint32_t fst;
  Hacl_Agile_Hash_state_s *snd;
  Hacl_Agile_Hash_state_s *thd;
}
Hacl_Streaming_HMAC_Definitions_two_state;

Hacl_Agile_Hash_state_s
*Hacl_Streaming_HMAC_s1(
  Hacl_Streaming_HMAC_Definitions_index i,
  Hacl_Streaming_HMAC_Definitions_two_state s
);

Hacl_Agile_Hash_state_s
*Hacl_Streaming_HMAC_s2(
  Hacl_Streaming_HMAC_Definitions_index i,
  Hacl_Streaming_HMAC_Definitions_two_state s
);

Hacl_Streaming_HMAC_Definitions_index
Hacl_Streaming_HMAC_index_of_state(Hacl_Streaming_HMAC_Definitions_two_state s);

typedef struct Hacl_Streaming_HMAC_agile_state_s Hacl_Streaming_HMAC_agile_state;

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_malloc_(
  Hacl_Agile_Hash_impl impl,
  uint8_t *key,
  uint32_t key_length,
  Hacl_Streaming_HMAC_agile_state **dst
);

Hacl_Streaming_HMAC_Definitions_index
Hacl_Streaming_HMAC_get_impl(Hacl_Streaming_HMAC_agile_state *s);

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_reset(
  Hacl_Streaming_HMAC_agile_state *state,
  uint8_t *key,
  uint32_t key_length
);

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_update(
  Hacl_Streaming_HMAC_agile_state *state,
  uint8_t *chunk,
  uint32_t chunk_len
);

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_digest(
  Hacl_Streaming_HMAC_agile_state *state,
  uint8_t *output,
  uint32_t digest_length
);

void Hacl_Streaming_HMAC_free(Hacl_Streaming_HMAC_agile_state *state);

Hacl_Streaming_HMAC_agile_state
*Hacl_Streaming_HMAC_copy(Hacl_Streaming_HMAC_agile_state *state);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Streaming_HMAC_H_DEFINED
#endif
