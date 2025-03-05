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


#ifndef __internal_Hacl_Streaming_Types_H
#define __internal_Hacl_Streaming_Types_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

#include "../Hacl_Streaming_Types.h"
#include "libintvector.h"

#define Hacl_Streaming_Types_None 0
#define Hacl_Streaming_Types_Some 1

typedef uint8_t Hacl_Streaming_Types_optional_32_tags;

typedef struct Hacl_Streaming_Types_optional_32_s
{
  Hacl_Streaming_Types_optional_32_tags tag;
  uint32_t *v;
}
Hacl_Streaming_Types_optional_32;

typedef struct Hacl_Streaming_Types_optional_64_s
{
  Hacl_Streaming_Types_optional_32_tags tag;
  uint64_t *v;
}
Hacl_Streaming_Types_optional_64;

typedef Hacl_Streaming_Types_optional_32_tags Hacl_Streaming_Types_optional_unit;

#if defined(__cplusplus)
}
#endif

#define __internal_Hacl_Streaming_Types_H_DEFINED
#endif
