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


#include "internal/Hacl_Streaming_HMAC.h"

#include "Hacl_Streaming_Types.h"

#include "Hacl_Hash_SHA3.h"
#include "Hacl_Hash_SHA2.h"
#include "Hacl_Hash_Blake2s_Simd128.h"
#include "Hacl_Hash_Blake2s.h"
#include "Hacl_Hash_Blake2b_Simd256.h"
#include "Hacl_Hash_Blake2b.h"
#include "internal/Hacl_Streaming_Types.h"

#include "internal/Hacl_Hash_SHA3.h"
#include "internal/Hacl_Hash_SHA2.h"
#include "internal/Hacl_Hash_SHA1.h"
#include "internal/Hacl_Hash_MD5.h"
#include "internal/Hacl_Hash_Blake2s_Simd128.h"
#include "internal/Hacl_Hash_Blake2s.h"
#include "internal/Hacl_Hash_Blake2b_Simd256.h"
#include "internal/Hacl_Hash_Blake2b.h"

static Spec_Hash_Definitions_hash_alg alg_of_impl(Hacl_Agile_Hash_impl i)
{
  switch (i)
  {
    case Hacl_Agile_Hash_MD5:
      {
        return Spec_Hash_Definitions_MD5;
      }
    case Hacl_Agile_Hash_SHA1:
      {
        return Spec_Hash_Definitions_SHA1;
      }
    case Hacl_Agile_Hash_SHA2_224:
      {
        return Spec_Hash_Definitions_SHA2_224;
      }
    case Hacl_Agile_Hash_SHA2_256:
      {
        return Spec_Hash_Definitions_SHA2_256;
      }
    case Hacl_Agile_Hash_SHA2_384:
      {
        return Spec_Hash_Definitions_SHA2_384;
      }
    case Hacl_Agile_Hash_SHA2_512:
      {
        return Spec_Hash_Definitions_SHA2_512;
      }
    case Hacl_Agile_Hash_SHA3_224:
      {
        return Spec_Hash_Definitions_SHA3_224;
      }
    case Hacl_Agile_Hash_SHA3_256:
      {
        return Spec_Hash_Definitions_SHA3_256;
      }
    case Hacl_Agile_Hash_SHA3_384:
      {
        return Spec_Hash_Definitions_SHA3_384;
      }
    case Hacl_Agile_Hash_SHA3_512:
      {
        return Spec_Hash_Definitions_SHA3_512;
      }
    case Hacl_Agile_Hash_Blake2S_32:
      {
        return Spec_Hash_Definitions_Blake2S;
      }
    case Hacl_Agile_Hash_Blake2S_128:
      {
        return Spec_Hash_Definitions_Blake2S;
      }
    case Hacl_Agile_Hash_Blake2B_32:
      {
        return Spec_Hash_Definitions_Blake2B;
      }
    case Hacl_Agile_Hash_Blake2B_256:
      {
        return Spec_Hash_Definitions_Blake2B;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static Hacl_Agile_Hash_impl impl_of_state_s(Hacl_Agile_Hash_state_s s)
{
  if (s.tag == Hacl_Agile_Hash_MD5_a)
  {
    return Hacl_Agile_Hash_MD5;
  }
  if (s.tag == Hacl_Agile_Hash_SHA1_a)
  {
    return Hacl_Agile_Hash_SHA1;
  }
  if (s.tag == Hacl_Agile_Hash_SHA2_224_a)
  {
    return Hacl_Agile_Hash_SHA2_224;
  }
  if (s.tag == Hacl_Agile_Hash_SHA2_256_a)
  {
    return Hacl_Agile_Hash_SHA2_256;
  }
  if (s.tag == Hacl_Agile_Hash_SHA2_384_a)
  {
    return Hacl_Agile_Hash_SHA2_384;
  }
  if (s.tag == Hacl_Agile_Hash_SHA2_512_a)
  {
    return Hacl_Agile_Hash_SHA2_512;
  }
  if (s.tag == Hacl_Agile_Hash_SHA3_224_a)
  {
    return Hacl_Agile_Hash_SHA3_224;
  }
  if (s.tag == Hacl_Agile_Hash_SHA3_256_a)
  {
    return Hacl_Agile_Hash_SHA3_256;
  }
  if (s.tag == Hacl_Agile_Hash_SHA3_384_a)
  {
    return Hacl_Agile_Hash_SHA3_384;
  }
  if (s.tag == Hacl_Agile_Hash_SHA3_512_a)
  {
    return Hacl_Agile_Hash_SHA3_512;
  }
  if (s.tag == Hacl_Agile_Hash_Blake2S_a)
  {
    return Hacl_Agile_Hash_Blake2S_32;
  }
  if (s.tag == Hacl_Agile_Hash_Blake2S_128_a)
  {
    return Hacl_Agile_Hash_Blake2S_128;
  }
  if (s.tag == Hacl_Agile_Hash_Blake2B_a)
  {
    return Hacl_Agile_Hash_Blake2B_32;
  }
  if (s.tag == Hacl_Agile_Hash_Blake2B_256_a)
  {
    return Hacl_Agile_Hash_Blake2B_256;
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

static Hacl_Agile_Hash_impl impl_of_state(Hacl_Agile_Hash_state_s *s)
{
  return impl_of_state_s(*s);
}

static Hacl_Agile_Hash_state_s *malloc_(Hacl_Agile_Hash_impl a)
{
  switch (a)
  {
    case Hacl_Agile_Hash_MD5:
      {
        uint32_t *s = (uint32_t *)KRML_HOST_CALLOC(4U, sizeof (uint32_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint32_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          = ((Hacl_Agile_Hash_state_s){ .tag = Hacl_Agile_Hash_MD5_a, { .case_MD5_a = s1 } });
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA1:
      {
        uint32_t *s = (uint32_t *)KRML_HOST_CALLOC(5U, sizeof (uint32_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint32_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          = ((Hacl_Agile_Hash_state_s){ .tag = Hacl_Agile_Hash_SHA1_a, { .case_SHA1_a = s1 } });
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA2_224:
      {
        uint32_t *s = (uint32_t *)KRML_HOST_CALLOC(8U, sizeof (uint32_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint32_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA2_224_a,
                { .case_SHA2_224_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA2_256:
      {
        uint32_t *s = (uint32_t *)KRML_HOST_CALLOC(8U, sizeof (uint32_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint32_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA2_256_a,
                { .case_SHA2_256_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA2_384:
      {
        uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(8U, sizeof (uint64_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint64_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA2_384_a,
                { .case_SHA2_384_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA2_512:
      {
        uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(8U, sizeof (uint64_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint64_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA2_512_a,
                { .case_SHA2_512_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA3_224:
      {
        uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint64_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA3_224_a,
                { .case_SHA3_224_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA3_256:
      {
        uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint64_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA3_256_a,
                { .case_SHA3_256_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA3_384:
      {
        uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint64_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA3_384_a,
                { .case_SHA3_384_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_SHA3_512:
      {
        uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(25U, sizeof (uint64_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint64_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_SHA3_512_a,
                { .case_SHA3_512_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_Blake2S_32:
      {
        uint32_t *s = (uint32_t *)KRML_HOST_CALLOC(16U, sizeof (uint32_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint32_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_Blake2S_a,
                { .case_Blake2S_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_Blake2S_128:
      {
        #if HACL_CAN_COMPILE_VEC128
        Lib_IntVector_Intrinsics_vec128
        *s = Hacl_Hash_Blake2s_Simd128_malloc_internal_state_with_key();
        if (s == NULL)
        {
          return NULL;
        }
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_Blake2S_128_a,
                { .case_Blake2S_128_a = s }
              }
            );
        }
        if (st == NULL)
        {
          KRML_ALIGNED_FREE(s);
          return NULL;
        }
        return st;
        #else
        KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
          __FILE__,
          __LINE__,
          "provably unreachable code: did an unverified caller violate a precondition\?");
        KRML_HOST_EXIT(255U);
        #endif
        break;
      }
    case Hacl_Agile_Hash_Blake2B_32:
      {
        uint64_t *s = (uint64_t *)KRML_HOST_CALLOC(16U, sizeof (uint64_t));
        if (s == NULL)
        {
          return NULL;
        }
        uint64_t *s1 = s;
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_Blake2B_a,
                { .case_Blake2B_a = s1 }
              }
            );
        }
        if (st == NULL)
        {
          KRML_HOST_FREE(s1);
          return NULL;
        }
        return st;
      }
    case Hacl_Agile_Hash_Blake2B_256:
      {
        #if HACL_CAN_COMPILE_VEC256
        Lib_IntVector_Intrinsics_vec256
        *s = Hacl_Hash_Blake2b_Simd256_malloc_internal_state_with_key();
        if (s == NULL)
        {
          return NULL;
        }
        Hacl_Agile_Hash_state_s
        *st = (Hacl_Agile_Hash_state_s *)KRML_HOST_MALLOC(sizeof (Hacl_Agile_Hash_state_s));
        if (st != NULL)
        {
          st[0U]
          =
            (
              (Hacl_Agile_Hash_state_s){
                .tag = Hacl_Agile_Hash_Blake2B_256_a,
                { .case_Blake2B_256_a = s }
              }
            );
        }
        if (st == NULL)
        {
          KRML_ALIGNED_FREE(s);
          return NULL;
        }
        return st;
        #else
        KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
          __FILE__,
          __LINE__,
          "provably unreachable code: did an unverified caller violate a precondition\?");
        KRML_HOST_EXIT(255U);
        #endif
        break;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static void init(Hacl_Agile_Hash_state_s *s)
{
  Hacl_Agile_Hash_state_s scrut = *s;
  if (scrut.tag == Hacl_Agile_Hash_MD5_a)
  {
    uint32_t *p1 = scrut.case_MD5_a;
    Hacl_Hash_MD5_init(p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA1_a)
  {
    uint32_t *p1 = scrut.case_SHA1_a;
    Hacl_Hash_SHA1_init(p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_224_a)
  {
    uint32_t *p1 = scrut.case_SHA2_224_a;
    Hacl_Hash_SHA2_sha224_init(p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_256_a)
  {
    uint32_t *p1 = scrut.case_SHA2_256_a;
    Hacl_Hash_SHA2_sha256_init(p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_384_a)
  {
    uint64_t *p1 = scrut.case_SHA2_384_a;
    Hacl_Hash_SHA2_sha384_init(p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_512_a)
  {
    uint64_t *p1 = scrut.case_SHA2_512_a;
    Hacl_Hash_SHA2_sha512_init(p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_224_a)
  {
    uint64_t *p1 = scrut.case_SHA3_224_a;
    Hacl_Hash_SHA3_init_(Spec_Hash_Definitions_SHA3_224, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_256_a)
  {
    uint64_t *p1 = scrut.case_SHA3_256_a;
    Hacl_Hash_SHA3_init_(Spec_Hash_Definitions_SHA3_256, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_384_a)
  {
    uint64_t *p1 = scrut.case_SHA3_384_a;
    Hacl_Hash_SHA3_init_(Spec_Hash_Definitions_SHA3_384, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_512_a)
  {
    uint64_t *p1 = scrut.case_SHA3_512_a;
    Hacl_Hash_SHA3_init_(Spec_Hash_Definitions_SHA3_512, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_a)
  {
    uint32_t *p1 = scrut.case_Blake2S_a;
    Hacl_Hash_Blake2s_init(p1, 0U, 32U);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_128_a)
  {
    Lib_IntVector_Intrinsics_vec128 *p1 = scrut.case_Blake2S_128_a;
    #if HACL_CAN_COMPILE_VEC128
    Hacl_Hash_Blake2s_Simd128_init(p1, 0U, 32U);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_a)
  {
    uint64_t *p1 = scrut.case_Blake2B_a;
    Hacl_Hash_Blake2b_init(p1, 0U, 64U);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_256_a)
  {
    Lib_IntVector_Intrinsics_vec256 *p1 = scrut.case_Blake2B_256_a;
    #if HACL_CAN_COMPILE_VEC256
    Hacl_Hash_Blake2b_Simd256_init(p1, 0U, 64U);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

static void
update_multi(Hacl_Agile_Hash_state_s *s, uint64_t prevlen, uint8_t *blocks, uint32_t len)
{
  Hacl_Agile_Hash_state_s scrut = *s;
  if (scrut.tag == Hacl_Agile_Hash_MD5_a)
  {
    uint32_t *p1 = scrut.case_MD5_a;
    uint32_t n = len / 64U;
    Hacl_Hash_MD5_update_multi(p1, blocks, n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA1_a)
  {
    uint32_t *p1 = scrut.case_SHA1_a;
    uint32_t n = len / 64U;
    Hacl_Hash_SHA1_update_multi(p1, blocks, n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_224_a)
  {
    uint32_t *p1 = scrut.case_SHA2_224_a;
    uint32_t n = len / 64U;
    Hacl_Hash_SHA2_sha224_update_nblocks(n * 64U, blocks, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_256_a)
  {
    uint32_t *p1 = scrut.case_SHA2_256_a;
    uint32_t n = len / 64U;
    Hacl_Hash_SHA2_sha256_update_nblocks(n * 64U, blocks, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_384_a)
  {
    uint64_t *p1 = scrut.case_SHA2_384_a;
    uint32_t n = len / 128U;
    Hacl_Hash_SHA2_sha384_update_nblocks(n * 128U, blocks, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_512_a)
  {
    uint64_t *p1 = scrut.case_SHA2_512_a;
    uint32_t n = len / 128U;
    Hacl_Hash_SHA2_sha512_update_nblocks(n * 128U, blocks, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_224_a)
  {
    uint64_t *p1 = scrut.case_SHA3_224_a;
    uint32_t n = len / 144U;
    Hacl_Hash_SHA3_update_multi_sha3(Spec_Hash_Definitions_SHA3_224, p1, blocks, n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_256_a)
  {
    uint64_t *p1 = scrut.case_SHA3_256_a;
    uint32_t n = len / 136U;
    Hacl_Hash_SHA3_update_multi_sha3(Spec_Hash_Definitions_SHA3_256, p1, blocks, n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_384_a)
  {
    uint64_t *p1 = scrut.case_SHA3_384_a;
    uint32_t n = len / 104U;
    Hacl_Hash_SHA3_update_multi_sha3(Spec_Hash_Definitions_SHA3_384, p1, blocks, n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_512_a)
  {
    uint64_t *p1 = scrut.case_SHA3_512_a;
    uint32_t n = len / 72U;
    Hacl_Hash_SHA3_update_multi_sha3(Spec_Hash_Definitions_SHA3_512, p1, blocks, n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_a)
  {
    uint32_t *p1 = scrut.case_Blake2S_a;
    uint32_t n = len / 64U;
    uint32_t wv[16U] = { 0U };
    Hacl_Hash_Blake2s_update_multi(n * 64U, wv, p1, prevlen, blocks, n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_128_a)
  {
    Lib_IntVector_Intrinsics_vec128 *p1 = scrut.case_Blake2S_128_a;
    #if HACL_CAN_COMPILE_VEC128
    uint32_t n = len / 64U;
    Hacl_Hash_Blake2s_Simd128_update_multi_no_inline(p1, prevlen, blocks, n);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_a)
  {
    uint64_t *p1 = scrut.case_Blake2B_a;
    uint32_t n = len / 128U;
    uint64_t wv[16U] = { 0U };
    Hacl_Hash_Blake2b_update_multi(n * 128U,
      wv,
      p1,
      FStar_UInt128_uint64_to_uint128(prevlen),
      blocks,
      n);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_256_a)
  {
    Lib_IntVector_Intrinsics_vec256 *p1 = scrut.case_Blake2B_256_a;
    #if HACL_CAN_COMPILE_VEC256
    uint32_t n = len / 128U;
    Hacl_Hash_Blake2b_Simd256_update_multi_no_inline(p1,
      FStar_UInt128_uint64_to_uint128(prevlen),
      blocks,
      n);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

static void
update_last(Hacl_Agile_Hash_state_s *s, uint64_t prev_len, uint8_t *last, uint32_t last_len)
{
  Hacl_Agile_Hash_state_s scrut = *s;
  if (scrut.tag == Hacl_Agile_Hash_MD5_a)
  {
    uint32_t *p1 = scrut.case_MD5_a;
    Hacl_Hash_MD5_update_last(p1, prev_len, last, last_len);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA1_a)
  {
    uint32_t *p1 = scrut.case_SHA1_a;
    Hacl_Hash_SHA1_update_last(p1, prev_len, last, last_len);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_224_a)
  {
    uint32_t *p1 = scrut.case_SHA2_224_a;
    Hacl_Hash_SHA2_sha224_update_last(prev_len + (uint64_t)last_len, last_len, last, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_256_a)
  {
    uint32_t *p1 = scrut.case_SHA2_256_a;
    Hacl_Hash_SHA2_sha256_update_last(prev_len + (uint64_t)last_len, last_len, last, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_384_a)
  {
    uint64_t *p1 = scrut.case_SHA2_384_a;
    Hacl_Hash_SHA2_sha384_update_last(FStar_UInt128_add(FStar_UInt128_uint64_to_uint128(prev_len),
        FStar_UInt128_uint64_to_uint128((uint64_t)last_len)),
      last_len,
      last,
      p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_512_a)
  {
    uint64_t *p1 = scrut.case_SHA2_512_a;
    Hacl_Hash_SHA2_sha512_update_last(FStar_UInt128_add(FStar_UInt128_uint64_to_uint128(prev_len),
        FStar_UInt128_uint64_to_uint128((uint64_t)last_len)),
      last_len,
      last,
      p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_224_a)
  {
    uint64_t *p1 = scrut.case_SHA3_224_a;
    Hacl_Hash_SHA3_update_last_sha3(Spec_Hash_Definitions_SHA3_224, p1, last, last_len);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_256_a)
  {
    uint64_t *p1 = scrut.case_SHA3_256_a;
    Hacl_Hash_SHA3_update_last_sha3(Spec_Hash_Definitions_SHA3_256, p1, last, last_len);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_384_a)
  {
    uint64_t *p1 = scrut.case_SHA3_384_a;
    Hacl_Hash_SHA3_update_last_sha3(Spec_Hash_Definitions_SHA3_384, p1, last, last_len);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_512_a)
  {
    uint64_t *p1 = scrut.case_SHA3_512_a;
    Hacl_Hash_SHA3_update_last_sha3(Spec_Hash_Definitions_SHA3_512, p1, last, last_len);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_a)
  {
    uint32_t *p1 = scrut.case_Blake2S_a;
    uint32_t wv[16U] = { 0U };
    Hacl_Hash_Blake2s_update_last(last_len, wv, p1, false, prev_len, last_len, last);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_128_a)
  {
    Lib_IntVector_Intrinsics_vec128 *p1 = scrut.case_Blake2S_128_a;
    #if HACL_CAN_COMPILE_VEC128
    Hacl_Hash_Blake2s_Simd128_update_last_no_inline(p1, prev_len, last, last_len);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_a)
  {
    uint64_t *p1 = scrut.case_Blake2B_a;
    uint64_t wv[16U] = { 0U };
    Hacl_Hash_Blake2b_update_last(last_len,
      wv,
      p1,
      false,
      FStar_UInt128_uint64_to_uint128(prev_len),
      last_len,
      last);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_256_a)
  {
    Lib_IntVector_Intrinsics_vec256 *p1 = scrut.case_Blake2B_256_a;
    #if HACL_CAN_COMPILE_VEC256
    Hacl_Hash_Blake2b_Simd256_update_last_no_inline(p1,
      FStar_UInt128_uint64_to_uint128(prev_len),
      last,
      last_len);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

static void finish(Hacl_Agile_Hash_state_s *s, uint8_t *dst)
{
  Hacl_Agile_Hash_state_s scrut = *s;
  if (scrut.tag == Hacl_Agile_Hash_MD5_a)
  {
    uint32_t *p1 = scrut.case_MD5_a;
    Hacl_Hash_MD5_finish(p1, dst);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA1_a)
  {
    uint32_t *p1 = scrut.case_SHA1_a;
    Hacl_Hash_SHA1_finish(p1, dst);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_224_a)
  {
    uint32_t *p1 = scrut.case_SHA2_224_a;
    Hacl_Hash_SHA2_sha224_finish(p1, dst);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_256_a)
  {
    uint32_t *p1 = scrut.case_SHA2_256_a;
    Hacl_Hash_SHA2_sha256_finish(p1, dst);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_384_a)
  {
    uint64_t *p1 = scrut.case_SHA2_384_a;
    Hacl_Hash_SHA2_sha384_finish(p1, dst);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_512_a)
  {
    uint64_t *p1 = scrut.case_SHA2_512_a;
    Hacl_Hash_SHA2_sha512_finish(p1, dst);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_224_a)
  {
    uint64_t *p1 = scrut.case_SHA3_224_a;
    uint32_t remOut = 28U;
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, p1, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    memcpy(dst + 28U - remOut, hbuf, remOut * sizeof (uint8_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_256_a)
  {
    uint64_t *p1 = scrut.case_SHA3_256_a;
    uint32_t remOut = 32U;
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, p1, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    memcpy(dst + 32U - remOut, hbuf, remOut * sizeof (uint8_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_384_a)
  {
    uint64_t *p1 = scrut.case_SHA3_384_a;
    uint32_t remOut = 48U;
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, p1, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    memcpy(dst + 48U - remOut, hbuf, remOut * sizeof (uint8_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_512_a)
  {
    uint64_t *p1 = scrut.case_SHA3_512_a;
    uint32_t remOut = 64U;
    uint8_t hbuf[256U] = { 0U };
    uint64_t ws[32U] = { 0U };
    memcpy(ws, p1, 25U * sizeof (uint64_t));
    for (uint32_t i = 0U; i < 32U; i++)
    {
      store64_le(hbuf + i * 8U, ws[i]);
    }
    memcpy(dst + 64U - remOut, hbuf, remOut * sizeof (uint8_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_a)
  {
    uint32_t *p1 = scrut.case_Blake2S_a;
    Hacl_Hash_Blake2s_finish(32U, dst, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_128_a)
  {
    Lib_IntVector_Intrinsics_vec128 *p1 = scrut.case_Blake2S_128_a;
    #if HACL_CAN_COMPILE_VEC128
    Hacl_Hash_Blake2s_Simd128_finish(32U, dst, p1);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_a)
  {
    uint64_t *p1 = scrut.case_Blake2B_a;
    Hacl_Hash_Blake2b_finish(64U, dst, p1);
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_256_a)
  {
    Lib_IntVector_Intrinsics_vec256 *p1 = scrut.case_Blake2B_256_a;
    #if HACL_CAN_COMPILE_VEC256
    Hacl_Hash_Blake2b_Simd256_finish(64U, dst, p1);
    return;
    #else
    KRML_MAYBE_UNUSED_VAR(p1);
    return;
    #endif
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

static void free_(Hacl_Agile_Hash_state_s *s)
{
  Hacl_Agile_Hash_state_s scrut = *s;
  if (scrut.tag == Hacl_Agile_Hash_MD5_a)
  {
    uint32_t *p1 = scrut.case_MD5_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA1_a)
  {
    uint32_t *p1 = scrut.case_SHA1_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA2_224_a)
  {
    uint32_t *p1 = scrut.case_SHA2_224_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA2_256_a)
  {
    uint32_t *p1 = scrut.case_SHA2_256_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA2_384_a)
  {
    uint64_t *p1 = scrut.case_SHA2_384_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA2_512_a)
  {
    uint64_t *p1 = scrut.case_SHA2_512_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA3_224_a)
  {
    uint64_t *p1 = scrut.case_SHA3_224_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA3_256_a)
  {
    uint64_t *p1 = scrut.case_SHA3_256_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA3_384_a)
  {
    uint64_t *p1 = scrut.case_SHA3_384_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_SHA3_512_a)
  {
    uint64_t *p1 = scrut.case_SHA3_512_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_Blake2S_a)
  {
    uint32_t *p1 = scrut.case_Blake2S_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_Blake2S_128_a)
  {
    Lib_IntVector_Intrinsics_vec128 *p1 = scrut.case_Blake2S_128_a;
    KRML_ALIGNED_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_Blake2B_a)
  {
    uint64_t *p1 = scrut.case_Blake2B_a;
    KRML_HOST_FREE(p1);
  }
  else if (scrut.tag == Hacl_Agile_Hash_Blake2B_256_a)
  {
    Lib_IntVector_Intrinsics_vec256 *p1 = scrut.case_Blake2B_256_a;
    KRML_ALIGNED_FREE(p1);
  }
  else
  {
    KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
      __FILE__,
      __LINE__,
      "unreachable (pattern matches are exhaustive in F*)");
    KRML_HOST_EXIT(255U);
  }
  KRML_HOST_FREE(s);
}

static void copy(Hacl_Agile_Hash_state_s *s_src, Hacl_Agile_Hash_state_s *s_dst)
{
  Hacl_Agile_Hash_state_s scrut = *s_src;
  if (scrut.tag == Hacl_Agile_Hash_MD5_a)
  {
    uint32_t *p_src = scrut.case_MD5_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint32_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_MD5_a)
    {
      p_dst = x1.case_MD5_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint32_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 4U * sizeof (uint32_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA1_a)
  {
    uint32_t *p_src = scrut.case_SHA1_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint32_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA1_a)
    {
      p_dst = x1.case_SHA1_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint32_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 5U * sizeof (uint32_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_224_a)
  {
    uint32_t *p_src = scrut.case_SHA2_224_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint32_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA2_224_a)
    {
      p_dst = x1.case_SHA2_224_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint32_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 8U * sizeof (uint32_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_256_a)
  {
    uint32_t *p_src = scrut.case_SHA2_256_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint32_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA2_256_a)
    {
      p_dst = x1.case_SHA2_256_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint32_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 8U * sizeof (uint32_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_384_a)
  {
    uint64_t *p_src = scrut.case_SHA2_384_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint64_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA2_384_a)
    {
      p_dst = x1.case_SHA2_384_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint64_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 8U * sizeof (uint64_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA2_512_a)
  {
    uint64_t *p_src = scrut.case_SHA2_512_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint64_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA2_512_a)
    {
      p_dst = x1.case_SHA2_512_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint64_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 8U * sizeof (uint64_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_224_a)
  {
    uint64_t *p_src = scrut.case_SHA3_224_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint64_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA3_224_a)
    {
      p_dst = x1.case_SHA3_224_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint64_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 25U * sizeof (uint64_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_256_a)
  {
    uint64_t *p_src = scrut.case_SHA3_256_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint64_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA3_256_a)
    {
      p_dst = x1.case_SHA3_256_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint64_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 25U * sizeof (uint64_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_384_a)
  {
    uint64_t *p_src = scrut.case_SHA3_384_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint64_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA3_384_a)
    {
      p_dst = x1.case_SHA3_384_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint64_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 25U * sizeof (uint64_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_SHA3_512_a)
  {
    uint64_t *p_src = scrut.case_SHA3_512_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint64_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_SHA3_512_a)
    {
      p_dst = x1.case_SHA3_512_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint64_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 25U * sizeof (uint64_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_a)
  {
    uint32_t *p_src = scrut.case_Blake2S_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint32_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_Blake2S_a)
    {
      p_dst = x1.case_Blake2S_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint32_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 16U * sizeof (uint32_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2S_128_a)
  {
    Lib_IntVector_Intrinsics_vec128 *p_src = scrut.case_Blake2S_128_a;
    KRML_MAYBE_UNUSED_VAR(p_src);
    #if HACL_CAN_COMPILE_VEC128
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    Lib_IntVector_Intrinsics_vec128 *p_dst;
    if (x1.tag == Hacl_Agile_Hash_Blake2S_128_a)
    {
      p_dst = x1.case_Blake2S_128_a;
    }
    else
    {
      p_dst =
        KRML_EABORT(Lib_IntVector_Intrinsics_vec128 *,
          "unreachable (pattern matches are exhaustive in F*)");
    }
    Hacl_Hash_Blake2s_Simd128_copy_internal_state(p_src, p_dst);
    return;
    #else
    return;
    #endif
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_a)
  {
    uint64_t *p_src = scrut.case_Blake2B_a;
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    uint64_t *p_dst;
    if (x1.tag == Hacl_Agile_Hash_Blake2B_a)
    {
      p_dst = x1.case_Blake2B_a;
    }
    else
    {
      p_dst = KRML_EABORT(uint64_t *, "unreachable (pattern matches are exhaustive in F*)");
    }
    memcpy(p_dst, p_src, 16U * sizeof (uint64_t));
    return;
  }
  if (scrut.tag == Hacl_Agile_Hash_Blake2B_256_a)
  {
    Lib_IntVector_Intrinsics_vec256 *p_src = scrut.case_Blake2B_256_a;
    KRML_MAYBE_UNUSED_VAR(p_src);
    #if HACL_CAN_COMPILE_VEC256
    Hacl_Agile_Hash_state_s x1 = *s_dst;
    Lib_IntVector_Intrinsics_vec256 *p_dst;
    if (x1.tag == Hacl_Agile_Hash_Blake2B_256_a)
    {
      p_dst = x1.case_Blake2B_256_a;
    }
    else
    {
      p_dst =
        KRML_EABORT(Lib_IntVector_Intrinsics_vec256 *,
          "unreachable (pattern matches are exhaustive in F*)");
    }
    Hacl_Hash_Blake2b_Simd256_copy_internal_state(p_src, p_dst);
    return;
    #else
    return;
    #endif
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

static void hash(Hacl_Agile_Hash_impl i, uint8_t *dst, uint8_t *input, uint32_t input_len)
{
  switch (i)
  {
    case Hacl_Agile_Hash_MD5:
      {
        Hacl_Hash_MD5_hash_oneshot(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA1:
      {
        Hacl_Hash_SHA1_hash_oneshot(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA2_224:
      {
        Hacl_Hash_SHA2_hash_224(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA2_256:
      {
        Hacl_Hash_SHA2_hash_256(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA2_384:
      {
        Hacl_Hash_SHA2_hash_384(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA2_512:
      {
        Hacl_Hash_SHA2_hash_512(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA3_224:
      {
        Hacl_Hash_SHA3_sha3_224(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA3_256:
      {
        Hacl_Hash_SHA3_sha3_256(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA3_384:
      {
        Hacl_Hash_SHA3_sha3_384(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_SHA3_512:
      {
        Hacl_Hash_SHA3_sha3_512(dst, input, input_len);
        break;
      }
    case Hacl_Agile_Hash_Blake2S_32:
      {
        Hacl_Hash_Blake2s_hash_with_key(dst, 32U, input, input_len, NULL, 0U);
        break;
      }
    case Hacl_Agile_Hash_Blake2S_128:
      {
        #if HACL_CAN_COMPILE_VEC128
        Hacl_Hash_Blake2s_Simd128_hash_with_key(dst, 32U, input, input_len, NULL, 0U);
        #endif
        break;
      }
    case Hacl_Agile_Hash_Blake2B_32:
      {
        Hacl_Hash_Blake2b_hash_with_key(dst, 64U, input, input_len, NULL, 0U);
        break;
      }
    case Hacl_Agile_Hash_Blake2B_256:
      {
        #if HACL_CAN_COMPILE_VEC256
        Hacl_Hash_Blake2b_Simd256_hash_with_key(dst, 64U, input, input_len, NULL, 0U);
        #endif
        break;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static uint32_t hash_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_MD5:
      {
        return 16U;
      }
    case Spec_Hash_Definitions_SHA1:
      {
        return 20U;
      }
    case Spec_Hash_Definitions_SHA2_224:
      {
        return 28U;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        return 32U;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        return 48U;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_Blake2S:
      {
        return 32U;
      }
    case Spec_Hash_Definitions_Blake2B:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA3_224:
      {
        return 28U;
      }
    case Spec_Hash_Definitions_SHA3_256:
      {
        return 32U;
      }
    case Spec_Hash_Definitions_SHA3_384:
      {
        return 48U;
      }
    case Spec_Hash_Definitions_SHA3_512:
      {
        return 64U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static uint32_t block_len(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_MD5:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA1:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA2_224:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        return 128U;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        return 128U;
      }
    case Spec_Hash_Definitions_SHA3_224:
      {
        return 144U;
      }
    case Spec_Hash_Definitions_SHA3_256:
      {
        return 136U;
      }
    case Spec_Hash_Definitions_SHA3_384:
      {
        return 104U;
      }
    case Spec_Hash_Definitions_SHA3_512:
      {
        return 72U;
      }
    case Spec_Hash_Definitions_Shake128:
      {
        return 168U;
      }
    case Spec_Hash_Definitions_Shake256:
      {
        return 136U;
      }
    case Spec_Hash_Definitions_Blake2S:
      {
        return 64U;
      }
    case Spec_Hash_Definitions_Blake2B:
      {
        return 128U;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static uint64_t max_input_len64(Spec_Hash_Definitions_hash_alg a)
{
  switch (a)
  {
    case Spec_Hash_Definitions_MD5:
      {
        return 2305843009213693951ULL;
      }
    case Spec_Hash_Definitions_SHA1:
      {
        return 2305843009213693951ULL;
      }
    case Spec_Hash_Definitions_SHA2_224:
      {
        return 2305843009213693951ULL;
      }
    case Spec_Hash_Definitions_SHA2_256:
      {
        return 2305843009213693951ULL;
      }
    case Spec_Hash_Definitions_SHA2_384:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_SHA2_512:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_Blake2S:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_Blake2B:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_SHA3_224:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_SHA3_256:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_SHA3_384:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_SHA3_512:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_Shake128:
      {
        return 18446744073709551615ULL;
      }
    case Spec_Hash_Definitions_Shake256:
      {
        return 18446744073709551615ULL;
      }
    default:
      {
        KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
        KRML_HOST_EXIT(253U);
      }
  }
}

static void wrap_key(Hacl_Agile_Hash_impl impl, uint8_t *output, uint8_t *key, uint32_t len)
{
  uint8_t *nkey = output;
  uint32_t ite;
  if (len <= block_len(alg_of_impl(impl)))
  {
    ite = len;
  }
  else
  {
    ite = hash_len(alg_of_impl(impl));
  }
  uint8_t *zeroes = output + ite;
  KRML_MAYBE_UNUSED_VAR(zeroes);
  if (len <= block_len(alg_of_impl(impl)))
  {
    if (len > 0U)
    {
      memcpy(nkey, key, len * sizeof (uint8_t));
      return;
    }
    return;
  }
  hash(impl, nkey, key, len);
}

static void init0(uint8_t *k, uint8_t *buf, Hacl_Streaming_HMAC_Definitions_two_state s)
{
  uint32_t k_len = s.fst;
  Hacl_Agile_Hash_state_s *s1 = s.snd;
  Hacl_Agile_Hash_state_s *s2 = s.thd;
  init(s1);
  init(s2);
  Hacl_Agile_Hash_impl i1 = impl_of_state(s1);
  Spec_Hash_Definitions_hash_alg a = alg_of_impl(i1);
  uint8_t b0[168U] = { 0U };
  uint8_t *block = b0;
  wrap_key(i1, block, k, k_len);
  uint8_t b1[168U];
  memset(b1, 0x36U, 168U * sizeof (uint8_t));
  uint8_t *ipad = b1;
  uint8_t b[168U];
  memset(b, 0x5cU, 168U * sizeof (uint8_t));
  uint8_t *opad = b;
  for (uint32_t i = 0U; i < block_len(a); i++)
  {
    uint8_t xi = ipad[i];
    uint8_t yi = block[i];
    buf[i] = (uint32_t)xi ^ (uint32_t)yi;
  }
  for (uint32_t i = 0U; i < block_len(a); i++)
  {
    uint8_t xi = opad[i];
    uint8_t yi = block[i];
    opad[i] = (uint32_t)xi ^ (uint32_t)yi;
  }
  update_multi(s2, 0ULL, opad, block_len(a));
}

static void finish0(Hacl_Streaming_HMAC_Definitions_two_state s, uint8_t *dst)
{
  Hacl_Agile_Hash_state_s *s2 = s.thd;
  Hacl_Agile_Hash_state_s *s1 = s.snd;
  Hacl_Agile_Hash_impl i1 = impl_of_state(s1);
  Spec_Hash_Definitions_hash_alg a = alg_of_impl(i1);
  finish(s1, dst);
  update_last(s2, (uint64_t)block_len(a), dst, hash_len(a));
  finish(s2, dst);
}

Hacl_Agile_Hash_state_s
*Hacl_Streaming_HMAC_s1(
  Hacl_Streaming_HMAC_Definitions_index i,
  Hacl_Streaming_HMAC_Definitions_two_state s
)
{
  KRML_MAYBE_UNUSED_VAR(i);
  return s.snd;
}

Hacl_Agile_Hash_state_s
*Hacl_Streaming_HMAC_s2(
  Hacl_Streaming_HMAC_Definitions_index i,
  Hacl_Streaming_HMAC_Definitions_two_state s
)
{
  KRML_MAYBE_UNUSED_VAR(i);
  return s.thd;
}

Hacl_Streaming_HMAC_Definitions_index
Hacl_Streaming_HMAC_index_of_state(Hacl_Streaming_HMAC_Definitions_two_state s)
{
  Hacl_Agile_Hash_state_s *s11 = s.snd;
  uint32_t kl = s.fst;
  Hacl_Agile_Hash_impl i1 = impl_of_state(s11);
  return ((Hacl_Streaming_HMAC_Definitions_index){ .fst = i1, .snd = kl });
}

static Hacl_Agile_Hash_impl
__proj__Mkdtuple2__item___1__Hacl_Agile_Hash_impl_uint32_t(
  Hacl_Streaming_HMAC_Definitions_index pair
)
{
  return pair.fst;
}

static Hacl_Agile_Hash_impl
dfst__Hacl_Agile_Hash_impl_uint32_t(Hacl_Streaming_HMAC_Definitions_index t)
{
  return __proj__Mkdtuple2__item___1__Hacl_Agile_Hash_impl_uint32_t(t);
}

static uint32_t
__proj__Mkdtuple2__item___2__Hacl_Agile_Hash_impl_uint32_t(
  Hacl_Streaming_HMAC_Definitions_index pair
)
{
  return pair.snd;
}

static uint32_t dsnd__Hacl_Agile_Hash_impl_uint32_t(Hacl_Streaming_HMAC_Definitions_index t)
{
  return __proj__Mkdtuple2__item___2__Hacl_Agile_Hash_impl_uint32_t(t);
}

typedef struct option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s___s
{
  Hacl_Streaming_Types_optional tag;
  Hacl_Streaming_HMAC_Definitions_two_state v;
}
option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__;

KRML_MAYBE_UNUSED static Hacl_Streaming_HMAC_agile_state
*malloc_internal(Hacl_Streaming_HMAC_Definitions_index i, uint8_t *key)
{
  KRML_CHECK_SIZE(sizeof (uint8_t),
    block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i))));
  uint8_t
  *buf =
    (uint8_t *)KRML_HOST_CALLOC(block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i))),
      sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  uint8_t *buf1 = buf;
  Hacl_Agile_Hash_state_s *s110 = malloc_(dfst__Hacl_Agile_Hash_impl_uint32_t(i));
  option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__ block_state;
  if (s110 == NULL)
  {
    block_state =
      (
        (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
          .tag = Hacl_Streaming_Types_None
        }
      );
  }
  else
  {
    Hacl_Agile_Hash_state_s *s21 = malloc_(dfst__Hacl_Agile_Hash_impl_uint32_t(i));
    if (s21 == NULL)
    {
      KRML_HOST_FREE(s110);
      block_state =
        (
          (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
            .tag = Hacl_Streaming_Types_None
          }
        );
    }
    else
    {
      block_state =
        (
          (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
            .tag = Hacl_Streaming_Types_Some,
            .v = { .fst = dsnd__Hacl_Agile_Hash_impl_uint32_t(i), .snd = s110, .thd = s21 }
          }
        );
    }
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf1);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    Hacl_Streaming_HMAC_Definitions_two_state block_state1 = block_state.v;
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_HMAC_agile_state
          s =
            {
              .block_state = block_state1,
              .buf = buf1,
              .total_len = (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i)))
            };
          Hacl_Streaming_HMAC_agile_state
          *p =
            (Hacl_Streaming_HMAC_agile_state *)KRML_HOST_MALLOC(sizeof (
                Hacl_Streaming_HMAC_agile_state
              ));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            Hacl_Agile_Hash_state_s *s21 = block_state1.thd;
            Hacl_Agile_Hash_state_s *s11 = block_state1.snd;
            free_(s11);
            free_(s21);
            KRML_HOST_FREE(buf1);
            return NULL;
          }
          init0(key, buf1, block_state1);
          return p;
        }
      default:
        {
          KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
          KRML_HOST_EXIT(253U);
        }
    }
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

KRML_MAYBE_UNUSED static bool is_blake2b_256(Hacl_Agile_Hash_impl uu___)
{
  switch (uu___)
  {
    case Hacl_Agile_Hash_Blake2B_256:
      {
        return true;
      }
    default:
      {
        return false;
      }
  }
}

KRML_MAYBE_UNUSED static bool is_blake2s_128(Hacl_Agile_Hash_impl uu___)
{
  switch (uu___)
  {
    case Hacl_Agile_Hash_Blake2S_128:
      {
        return true;
      }
    default:
      {
        return false;
      }
  }
}

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_malloc_(
  Hacl_Agile_Hash_impl impl,
  uint8_t *key,
  uint32_t key_length,
  Hacl_Streaming_HMAC_agile_state **dst
)
{
  KRML_MAYBE_UNUSED_VAR(key);
  KRML_MAYBE_UNUSED_VAR(key_length);
  KRML_MAYBE_UNUSED_VAR(dst);
  #if !HACL_CAN_COMPILE_VEC256
  if (is_blake2b_256(impl))
  {
    return Hacl_Streaming_Types_InvalidAlgorithm;
  }
  #endif
  #if !HACL_CAN_COMPILE_VEC128
  if (is_blake2s_128(impl))
  {
    return Hacl_Streaming_Types_InvalidAlgorithm;
  }
  #endif
  Hacl_Streaming_HMAC_agile_state
  *st =
    malloc_internal(((Hacl_Streaming_HMAC_Definitions_index){ .fst = impl, .snd = key_length }),
      key);
  if (st == NULL)
  {
    return Hacl_Streaming_Types_OutOfMemory;
  }
  *dst = st;
  return Hacl_Streaming_Types_Success;
}

Hacl_Streaming_HMAC_Definitions_index
Hacl_Streaming_HMAC_get_impl(Hacl_Streaming_HMAC_agile_state *s)
{
  Hacl_Streaming_HMAC_Definitions_two_state block_state = (*s).block_state;
  return Hacl_Streaming_HMAC_index_of_state(block_state);
}

static void reset_internal(Hacl_Streaming_HMAC_agile_state *state, uint8_t *key)
{
  Hacl_Streaming_HMAC_agile_state scrut = *state;
  uint8_t *buf = scrut.buf;
  Hacl_Streaming_HMAC_Definitions_two_state block_state = scrut.block_state;
  Hacl_Streaming_HMAC_Definitions_index i1 = Hacl_Streaming_HMAC_index_of_state(block_state);
  KRML_MAYBE_UNUSED_VAR(i1);
  init0(key, buf, block_state);
  Hacl_Streaming_HMAC_agile_state
  tmp =
    {
      .block_state = block_state,
      .buf = buf,
      .total_len = (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
    };
  state[0U] = tmp;
}

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_reset(
  Hacl_Streaming_HMAC_agile_state *state,
  uint8_t *key,
  uint32_t key_length
)
{
  uint32_t k_len = Hacl_Streaming_HMAC_get_impl(state).snd;
  if (key_length != k_len)
  {
    return Hacl_Streaming_Types_InvalidLength;
  }
  reset_internal(state, key);
  return Hacl_Streaming_Types_Success;
}

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_update(
  Hacl_Streaming_HMAC_agile_state *state,
  uint8_t *chunk,
  uint32_t chunk_len
)
{
  Hacl_Streaming_HMAC_agile_state s = *state;
  Hacl_Streaming_HMAC_Definitions_two_state block_state = s.block_state;
  uint64_t total_len = s.total_len;
  Hacl_Streaming_HMAC_Definitions_index i1 = Hacl_Streaming_HMAC_index_of_state(block_state);
  if
  (
    (uint64_t)chunk_len
    > max_input_len64(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))) - total_len
  )
  {
    return Hacl_Streaming_Types_MaximumLengthExceeded;
  }
  uint32_t sz;
  if
  (
    total_len
    % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
    == 0ULL
    && total_len > 0ULL
  )
  {
    sz = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
  }
  else
  {
    sz =
      (uint32_t)(total_len
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
  }
  if (chunk_len <= block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))) - sz)
  {
    Hacl_Streaming_HMAC_agile_state s3 = *state;
    Hacl_Streaming_HMAC_Definitions_two_state block_state1 = s3.block_state;
    uint8_t *buf = s3.buf;
    uint64_t total_len1 = s3.total_len;
    uint32_t sz1;
    if
    (
      total_len1
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
      == 0ULL
      && total_len1 > 0ULL
    )
    {
      sz1 = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    else
    {
      sz1 =
        (uint32_t)(total_len1
        % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    uint8_t *buf2 = buf + sz1;
    memcpy(buf2, chunk, chunk_len * sizeof (uint8_t));
    uint64_t total_len2 = total_len1 + (uint64_t)chunk_len;
    *state
    =
      (
        (Hacl_Streaming_HMAC_agile_state){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len2
        }
      );
  }
  else if (sz == 0U)
  {
    Hacl_Streaming_HMAC_agile_state s3 = *state;
    Hacl_Streaming_HMAC_Definitions_two_state block_state1 = s3.block_state;
    uint8_t *buf = s3.buf;
    uint64_t total_len1 = s3.total_len;
    uint32_t sz1;
    if
    (
      total_len1
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
      == 0ULL
      && total_len1 > 0ULL
    )
    {
      sz1 = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    else
    {
      sz1 =
        (uint32_t)(total_len1
        % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    if (!(sz1 == 0U))
    {
      uint64_t prevlen = total_len1 - (uint64_t)sz1;
      Hacl_Agile_Hash_state_s *s11 = block_state1.snd;
      update_multi(s11,
        prevlen,
        buf,
        block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    uint32_t ite;
    if
    (
      (uint64_t)chunk_len
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
      == 0ULL
      && (uint64_t)chunk_len > 0ULL
    )
    {
      ite = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    else
    {
      ite =
        (uint32_t)((uint64_t)chunk_len
        % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    uint32_t
    n_blocks = (chunk_len - ite) / block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    uint32_t
    data1_len = n_blocks * block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    uint32_t data2_len = chunk_len - data1_len;
    uint8_t *data1 = chunk;
    uint8_t *data2 = chunk + data1_len;
    Hacl_Agile_Hash_state_s *s11 = block_state1.snd;
    update_multi(s11, total_len1, data1, data1_len);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state
    =
      (
        (Hacl_Streaming_HMAC_agile_state){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)chunk_len
        }
      );
  }
  else
  {
    uint32_t diff = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))) - sz;
    uint8_t *chunk1 = chunk;
    uint8_t *chunk2 = chunk + diff;
    Hacl_Streaming_HMAC_agile_state s3 = *state;
    Hacl_Streaming_HMAC_Definitions_two_state block_state10 = s3.block_state;
    uint8_t *buf0 = s3.buf;
    uint64_t total_len10 = s3.total_len;
    uint32_t sz10;
    if
    (
      total_len10
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
      == 0ULL
      && total_len10 > 0ULL
    )
    {
      sz10 = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    else
    {
      sz10 =
        (uint32_t)(total_len10
        % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    uint8_t *buf2 = buf0 + sz10;
    memcpy(buf2, chunk1, diff * sizeof (uint8_t));
    uint64_t total_len2 = total_len10 + (uint64_t)diff;
    *state
    =
      (
        (Hacl_Streaming_HMAC_agile_state){
          .block_state = block_state10,
          .buf = buf0,
          .total_len = total_len2
        }
      );
    Hacl_Streaming_HMAC_agile_state s30 = *state;
    Hacl_Streaming_HMAC_Definitions_two_state block_state1 = s30.block_state;
    uint8_t *buf = s30.buf;
    uint64_t total_len1 = s30.total_len;
    uint32_t sz1;
    if
    (
      total_len1
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
      == 0ULL
      && total_len1 > 0ULL
    )
    {
      sz1 = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    else
    {
      sz1 =
        (uint32_t)(total_len1
        % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    if (!(sz1 == 0U))
    {
      uint64_t prevlen = total_len1 - (uint64_t)sz1;
      Hacl_Agile_Hash_state_s *s11 = block_state1.snd;
      update_multi(s11,
        prevlen,
        buf,
        block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    uint32_t ite;
    if
    (
      (uint64_t)(chunk_len - diff)
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
      == 0ULL
      && (uint64_t)(chunk_len - diff) > 0ULL
    )
    {
      ite = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    else
    {
      ite =
        (uint32_t)((uint64_t)(chunk_len - diff)
        % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
    }
    uint32_t
    n_blocks =
      (chunk_len - diff - ite)
      / block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    uint32_t
    data1_len = n_blocks * block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    uint32_t data2_len = chunk_len - diff - data1_len;
    uint8_t *data1 = chunk2;
    uint8_t *data2 = chunk2 + data1_len;
    Hacl_Agile_Hash_state_s *s11 = block_state1.snd;
    update_multi(s11, total_len1, data1, data1_len);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state
    =
      (
        (Hacl_Streaming_HMAC_agile_state){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)(chunk_len - diff)
        }
      );
  }
  return Hacl_Streaming_Types_Success;
}

typedef struct
___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s____uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s___s
{
  Hacl_Streaming_HMAC_Definitions_two_state fst;
  Hacl_Streaming_HMAC_Definitions_two_state snd;
}
___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s____uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__;

Hacl_Streaming_Types_error_code
Hacl_Streaming_HMAC_digest(
  Hacl_Streaming_HMAC_agile_state *state,
  uint8_t *output,
  uint32_t digest_length
)
{
  KRML_MAYBE_UNUSED_VAR(digest_length);
  Hacl_Streaming_HMAC_Definitions_two_state block_state0 = (*state).block_state;
  Hacl_Streaming_HMAC_Definitions_index i1 = Hacl_Streaming_HMAC_index_of_state(block_state0);
  Hacl_Streaming_HMAC_agile_state scrut0 = *state;
  Hacl_Streaming_HMAC_Definitions_two_state block_state = scrut0.block_state;
  uint8_t *buf_ = scrut0.buf;
  uint64_t total_len = scrut0.total_len;
  uint32_t r;
  if
  (
    total_len
    % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)))
    == 0ULL
    && total_len > 0ULL
  )
  {
    r = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
  }
  else
  {
    r =
      (uint32_t)(total_len
      % (uint64_t)block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
  }
  uint8_t *buf_1 = buf_;
  Hacl_Agile_Hash_state_s *s110 = malloc_(dfst__Hacl_Agile_Hash_impl_uint32_t(i1));
  option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__ tmp_block_state;
  if (s110 == NULL)
  {
    tmp_block_state =
      (
        (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
          .tag = Hacl_Streaming_Types_None
        }
      );
  }
  else
  {
    Hacl_Agile_Hash_state_s *s21 = malloc_(dfst__Hacl_Agile_Hash_impl_uint32_t(i1));
    if (s21 == NULL)
    {
      KRML_HOST_FREE(s110);
      tmp_block_state =
        (
          (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
            .tag = Hacl_Streaming_Types_None
          }
        );
    }
    else
    {
      tmp_block_state =
        (
          (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
            .tag = Hacl_Streaming_Types_Some,
            .v = { .fst = dsnd__Hacl_Agile_Hash_impl_uint32_t(i1), .snd = s110, .thd = s21 }
          }
        );
    }
  }
  if (tmp_block_state.tag == Hacl_Streaming_Types_None)
  {
    return Hacl_Streaming_Types_OutOfMemory;
  }
  if (tmp_block_state.tag == Hacl_Streaming_Types_Some)
  {
    Hacl_Streaming_HMAC_Definitions_two_state tmp_block_state1 = tmp_block_state.v;
    ___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s____uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__
    scrut = { .fst = block_state, .snd = tmp_block_state1 };
    Hacl_Agile_Hash_state_s *s2_ = scrut.snd.thd;
    Hacl_Agile_Hash_state_s *s1_ = scrut.snd.snd;
    Hacl_Agile_Hash_state_s *s21 = scrut.fst.thd;
    Hacl_Agile_Hash_state_s *s111 = scrut.fst.snd;
    copy(s111, s1_);
    copy(s21, s2_);
    uint64_t prev_len = total_len - (uint64_t)r;
    uint32_t ite;
    if (r % block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))) == 0U && r > 0U)
    {
      ite = block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    else
    {
      ite = r % block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1)));
    }
    uint8_t *buf_last = buf_1 + r - ite;
    uint8_t *buf_multi = buf_1;
    Hacl_Agile_Hash_state_s *s112 = tmp_block_state1.snd;
    update_multi(s112, prev_len, buf_multi, 0U);
    uint64_t prev_len_last = total_len - (uint64_t)r;
    Hacl_Agile_Hash_state_s *s11 = tmp_block_state1.snd;
    update_last(s11, prev_len_last, buf_last, r);
    finish0(tmp_block_state1, output);
    return Hacl_Streaming_Types_Success;
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

void Hacl_Streaming_HMAC_free(Hacl_Streaming_HMAC_agile_state *state)
{
  Hacl_Streaming_HMAC_Definitions_two_state block_state0 = (*state).block_state;
  Hacl_Streaming_HMAC_Definitions_index i1 = Hacl_Streaming_HMAC_index_of_state(block_state0);
  KRML_MAYBE_UNUSED_VAR(i1);
  Hacl_Streaming_HMAC_agile_state scrut = *state;
  uint8_t *buf = scrut.buf;
  Hacl_Streaming_HMAC_Definitions_two_state block_state = scrut.block_state;
  Hacl_Agile_Hash_state_s *s21 = block_state.thd;
  Hacl_Agile_Hash_state_s *s11 = block_state.snd;
  free_(s11);
  free_(s21);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(state);
}

Hacl_Streaming_HMAC_agile_state
*Hacl_Streaming_HMAC_copy(Hacl_Streaming_HMAC_agile_state *state)
{
  Hacl_Streaming_HMAC_agile_state scrut0 = *state;
  Hacl_Streaming_HMAC_Definitions_two_state block_state0 = scrut0.block_state;
  uint8_t *buf0 = scrut0.buf;
  uint64_t total_len0 = scrut0.total_len;
  Hacl_Streaming_HMAC_Definitions_index i1 = Hacl_Streaming_HMAC_index_of_state(block_state0);
  KRML_CHECK_SIZE(sizeof (uint8_t),
    block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))));
  uint8_t
  *buf =
    (uint8_t *)KRML_HOST_CALLOC(block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))),
      sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf,
    buf0,
    block_len(alg_of_impl(dfst__Hacl_Agile_Hash_impl_uint32_t(i1))) * sizeof (uint8_t));
  Hacl_Agile_Hash_state_s *s110 = malloc_(dfst__Hacl_Agile_Hash_impl_uint32_t(i1));
  option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__ block_state;
  if (s110 == NULL)
  {
    block_state =
      (
        (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
          .tag = Hacl_Streaming_Types_None
        }
      );
  }
  else
  {
    Hacl_Agile_Hash_state_s *s21 = malloc_(dfst__Hacl_Agile_Hash_impl_uint32_t(i1));
    if (s21 == NULL)
    {
      KRML_HOST_FREE(s110);
      block_state =
        (
          (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
            .tag = Hacl_Streaming_Types_None
          }
        );
    }
    else
    {
      block_state =
        (
          (option___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__){
            .tag = Hacl_Streaming_Types_Some,
            .v = { .fst = dsnd__Hacl_Agile_Hash_impl_uint32_t(i1), .snd = s110, .thd = s21 }
          }
        );
    }
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    Hacl_Streaming_HMAC_Definitions_two_state block_state1 = block_state.v;
    ___uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s____uint32_t____Hacl_Agile_Hash_state_s_____Hacl_Agile_Hash_state_s__
    scrut = { .fst = block_state0, .snd = block_state1 };
    Hacl_Agile_Hash_state_s *s2_ = scrut.snd.thd;
    Hacl_Agile_Hash_state_s *s1_ = scrut.snd.snd;
    Hacl_Agile_Hash_state_s *s21 = scrut.fst.thd;
    Hacl_Agile_Hash_state_s *s111 = scrut.fst.snd;
    copy(s111, s1_);
    copy(s21, s2_);
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_HMAC_agile_state
          s = { .block_state = block_state1, .buf = buf, .total_len = total_len0 };
          Hacl_Streaming_HMAC_agile_state
          *p =
            (Hacl_Streaming_HMAC_agile_state *)KRML_HOST_MALLOC(sizeof (
                Hacl_Streaming_HMAC_agile_state
              ));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            Hacl_Agile_Hash_state_s *s210 = block_state1.thd;
            Hacl_Agile_Hash_state_s *s11 = block_state1.snd;
            free_(s11);
            free_(s210);
            KRML_HOST_FREE(buf);
            return NULL;
          }
          return p;
        }
      default:
        {
          KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
          KRML_HOST_EXIT(253U);
        }
    }
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

