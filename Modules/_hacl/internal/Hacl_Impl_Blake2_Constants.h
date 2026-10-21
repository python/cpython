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


#ifndef internal_Hacl_Impl_Blake2_Constants_H
#define internal_Hacl_Impl_Blake2_Constants_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include "krml/internal/types.h"
#include "krml/lowstar_endianness.h"
#include "krml/internal/target.h"

static const
uint32_t
Hacl_Hash_Blake2b_sigmaTable[160U] =
  {
    0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U, 13U, 14U, 15U, 14U, 10U, 4U, 8U, 9U, 15U,
    13U, 6U, 1U, 12U, 0U, 2U, 11U, 7U, 5U, 3U, 11U, 8U, 12U, 0U, 5U, 2U, 15U, 13U, 10U, 14U, 3U, 6U,
    7U, 1U, 9U, 4U, 7U, 9U, 3U, 1U, 13U, 12U, 11U, 14U, 2U, 6U, 5U, 10U, 4U, 0U, 15U, 8U, 9U, 0U,
    5U, 7U, 2U, 4U, 10U, 15U, 14U, 1U, 11U, 12U, 6U, 8U, 3U, 13U, 2U, 12U, 6U, 10U, 0U, 11U, 8U, 3U,
    4U, 13U, 7U, 5U, 15U, 14U, 1U, 9U, 12U, 5U, 1U, 15U, 14U, 13U, 4U, 10U, 0U, 7U, 6U, 3U, 9U, 2U,
    8U, 11U, 13U, 11U, 7U, 14U, 12U, 1U, 3U, 9U, 5U, 0U, 15U, 4U, 8U, 6U, 2U, 10U, 6U, 15U, 14U, 9U,
    11U, 3U, 0U, 8U, 12U, 2U, 13U, 7U, 1U, 4U, 10U, 5U, 10U, 2U, 8U, 4U, 7U, 6U, 1U, 5U, 15U, 11U,
    9U, 14U, 3U, 12U, 13U
  };

static const
uint32_t
Hacl_Hash_Blake2b_ivTable_S[8U] =
  {
    0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU, 0x510E527FU, 0x9B05688CU, 0x1F83D9ABU,
    0x5BE0CD19U
  };

static const
uint64_t
Hacl_Hash_Blake2b_ivTable_B[8U] =
  {
    0x6A09E667F3BCC908ULL, 0xBB67AE8584CAA73BULL, 0x3C6EF372FE94F82BULL, 0xA54FF53A5F1D36F1ULL,
    0x510E527FADE682D1ULL, 0x9B05688C2B3E6C1FULL, 0x1F83D9ABFB41BD6BULL, 0x5BE0CD19137E2179ULL
  };

#if defined(__cplusplus)
}
#endif

#define internal_Hacl_Impl_Blake2_Constants_H_DEFINED
#endif /* internal_Hacl_Impl_Blake2_Constants_H */
