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


#ifndef __Hacl_Hash_SHA1_H
#define __Hacl_Hash_SHA1_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "Hacl_Streaming_Types.h"

typedef Hacl_Streaming_MD_state_32 Hacl_Streaming_SHA1_state;

Hacl_Streaming_MD_state_32 *Hacl_Streaming_SHA1_legacy_create_in(void);

void Hacl_Streaming_SHA1_legacy_init(Hacl_Streaming_MD_state_32 *s);

/**
0 = success, 1 = max length exceeded
*/
Hacl_Streaming_Types_error_code
Hacl_Streaming_SHA1_legacy_update(Hacl_Streaming_MD_state_32 *p, uint8_t *data, uint32_t len);

void Hacl_Streaming_SHA1_legacy_finish(Hacl_Streaming_MD_state_32 *p, uint8_t *dst);

void Hacl_Streaming_SHA1_legacy_free(Hacl_Streaming_MD_state_32 *s);

Hacl_Streaming_MD_state_32 *Hacl_Streaming_SHA1_legacy_copy(Hacl_Streaming_MD_state_32 *s0);

void Hacl_Streaming_SHA1_legacy_hash(uint8_t *input, uint32_t input_len, uint8_t *dst);

#if defined(__cplusplus)
}
#endif

#define __Hacl_Hash_SHA1_H_DEFINED
#endif
