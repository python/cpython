#ifndef __Vec_Intrin_H
#define __Vec_Intrin_H

#include <sys/types.h>

#ifndef HACL_INTRINSICS_SHIMMED

/* We include config.h here to ensure that the various feature-flags are
 * properly brought into scope. Users can either run the configure script, or
 * write a config.h themselves and put it under version control. */
#if defined(__has_include)
#if __has_include("config.h")
#include "config.h"
#endif
#endif

/* # DEBUGGING:
 * ============
 * It is possible to debug the current definitions by using libintvector_debug.h
 * See the include at the bottom of the file. */

#define Lib_IntVector_Intrinsics_bit_mask64(x) -((x) & 1)

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)

#if defined(HACL_CAN_COMPILE_VEC128)

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

typedef __m128i Lib_IntVector_Intrinsics_vec128;

#define Lib_IntVector_Intrinsics_ni_aes_enc(x0, x1) \
  (_mm_aesenc_si128(x0, x1))

#define Lib_IntVector_Intrinsics_ni_aes_enc_last(x0, x1) \
  (_mm_aesenclast_si128(x0, x1))

#define Lib_IntVector_Intrinsics_ni_aes_keygen_assist(x0, x1) \
  (_mm_aeskeygenassist_si128(x0, x1))

#define Lib_IntVector_Intrinsics_ni_clmul(x0, x1, x2)		\
  (_mm_clmulepi64_si128(x0, x1, x2))


#define Lib_IntVector_Intrinsics_vec128_xor(x0, x1) \
  (_mm_xor_si128(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_eq64(x0, x1) \
  (_mm_cmpeq_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_eq32(x0, x1) \
  (_mm_cmpeq_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_gt64(x0, x1) \
  (_mm_cmpgt_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_gt32(x0, x1) \
  (_mm_cmpgt_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_or(x0, x1) \
  (_mm_or_si128(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_and(x0, x1) \
  (_mm_and_si128(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_lognot(x0) \
  (_mm_xor_si128(x0, _mm_set1_epi32(-1)))


#define Lib_IntVector_Intrinsics_vec128_shift_left(x0, x1) \
  (_mm_slli_si128(x0, (x1)/8))

#define Lib_IntVector_Intrinsics_vec128_shift_right(x0, x1) \
  (_mm_srli_si128(x0, (x1)/8))

#define Lib_IntVector_Intrinsics_vec128_shift_left64(x0, x1) \
  (_mm_slli_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_shift_right64(x0, x1) \
  (_mm_srli_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_shift_left32(x0, x1) \
  (_mm_slli_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_shift_right32(x0, x1) \
  (_mm_srli_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32_8(x0) \
  (_mm_shuffle_epi8(x0, _mm_set_epi8(14,13,12,15,10,9,8,11,6,5,4,7,2,1,0,3)))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32_16(x0) \
  (_mm_shuffle_epi8(x0, _mm_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2)))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32_24(x0) \
  (_mm_shuffle_epi8(x0, _mm_set_epi8(12,15,14,13,8,11,10,9,4,7,6,5,0,3,2,1)))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32(x0,x1)	\
  (((x1) == 8? Lib_IntVector_Intrinsics_vec128_rotate_left32_8(x0) : \
   ((x1) == 16? Lib_IntVector_Intrinsics_vec128_rotate_left32_16(x0) : \
   ((x1) == 24? Lib_IntVector_Intrinsics_vec128_rotate_left32_24(x0) : \
    _mm_xor_si128(_mm_slli_epi32(x0,x1),_mm_srli_epi32(x0,32-(x1)))))))

#define Lib_IntVector_Intrinsics_vec128_rotate_right32(x0,x1)	\
  (Lib_IntVector_Intrinsics_vec128_rotate_left32(x0,32-(x1)))

#define Lib_IntVector_Intrinsics_vec128_shuffle32(x0, x1, x2, x3, x4)	\
  (_mm_shuffle_epi32(x0, _MM_SHUFFLE(x4,x3,x2,x1)))

#define Lib_IntVector_Intrinsics_vec128_shuffle64(x0, x1, x2) \
  (_mm_shuffle_epi32(x0, _MM_SHUFFLE(2*x1+1,2*x1,2*x2+1,2*x2)))

#define Lib_IntVector_Intrinsics_vec128_rotate_right_lanes32(x0, x1)	\
  (_mm_shuffle_epi32(x0, _MM_SHUFFLE((x1+3)%4,(x1+2)%4,(x1+1)%4,x1%4)))

#define Lib_IntVector_Intrinsics_vec128_rotate_right_lanes64(x0, x1)	\
  (_mm_shuffle_epi32(x0, _MM_SHUFFLE((2*x1+3)%4,(2*x1+2)%4,(2*x1+1)%4,(2*x1)%4)))

#define Lib_IntVector_Intrinsics_vec128_load32_le(x0) \
  (_mm_loadu_si128((__m128i*)(x0)))

#define Lib_IntVector_Intrinsics_vec128_load64_le(x0) \
  (_mm_loadu_si128((__m128i*)(x0)))

#define Lib_IntVector_Intrinsics_vec128_store32_le(x0, x1) \
  (_mm_storeu_si128((__m128i*)(x0), x1))

#define Lib_IntVector_Intrinsics_vec128_store64_le(x0, x1) \
  (_mm_storeu_si128((__m128i*)(x0), x1))

#define Lib_IntVector_Intrinsics_vec128_load_be(x0)		\
  (_mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(x0)), _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)))

#define Lib_IntVector_Intrinsics_vec128_load32_be(x0)		\
  (_mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(x0)), _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3)))

#define Lib_IntVector_Intrinsics_vec128_load64_be(x0)		\
  (_mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(x0)), _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7)))

#define Lib_IntVector_Intrinsics_vec128_store_be(x0, x1)	\
  (_mm_storeu_si128((__m128i*)(x0), _mm_shuffle_epi8(x1, _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15))))


#define Lib_IntVector_Intrinsics_vec128_store32_be(x0, x1)	\
  (_mm_storeu_si128((__m128i*)(x0), _mm_shuffle_epi8(x1, _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3))))

#define Lib_IntVector_Intrinsics_vec128_store64_be(x0, x1)	\
  (_mm_storeu_si128((__m128i*)(x0), _mm_shuffle_epi8(x1,  _mm_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7))))



#define Lib_IntVector_Intrinsics_vec128_insert8(x0, x1, x2)	\
  (_mm_insert_epi8(x0, x1, x2))

#define Lib_IntVector_Intrinsics_vec128_insert32(x0, x1, x2)	\
  (_mm_insert_epi32(x0, x1, x2))

#define Lib_IntVector_Intrinsics_vec128_insert64(x0, x1, x2)	\
  (_mm_insert_epi64(x0, x1, x2))

#define Lib_IntVector_Intrinsics_vec128_extract8(x0, x1)	\
  (_mm_extract_epi8(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_extract32(x0, x1)	\
  (_mm_extract_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_extract64(x0, x1)	\
  (_mm_extract_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_zero  \
  (_mm_setzero_si128())


#define Lib_IntVector_Intrinsics_vec128_add64(x0, x1) \
  (_mm_add_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_sub64(x0, x1)		\
  (_mm_sub_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_mul64(x0, x1) \
  (_mm_mul_epu32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_smul64(x0, x1) \
  (_mm_mul_epu32(x0, _mm_set1_epi64x(x1)))

#define Lib_IntVector_Intrinsics_vec128_add32(x0, x1) \
  (_mm_add_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_sub32(x0, x1)		\
  (_mm_sub_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_mul32(x0, x1) \
  (_mm_mullo_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_smul32(x0, x1) \
  (_mm_mullo_epi32(x0, _mm_set1_epi32(x1)))

#define Lib_IntVector_Intrinsics_vec128_load128(x) \
  ((__m128i)x)

#define Lib_IntVector_Intrinsics_vec128_load64(x) \
  (_mm_set1_epi64x(x)) /* hi lo */

#define Lib_IntVector_Intrinsics_vec128_load64s(x0, x1) \
  (_mm_set_epi64x(x1, x0)) /* hi lo */

#define Lib_IntVector_Intrinsics_vec128_load32(x) \
  (_mm_set1_epi32(x))

#define Lib_IntVector_Intrinsics_vec128_load32s(x0, x1, x2, x3) \
  (_mm_set_epi32(x3, x2, x1, x0)) /* hi lo */

#define Lib_IntVector_Intrinsics_vec128_interleave_low32(x1, x2) \
  (_mm_unpacklo_epi32(x1, x2))

#define Lib_IntVector_Intrinsics_vec128_interleave_high32(x1, x2) \
  (_mm_unpackhi_epi32(x1, x2))

#define Lib_IntVector_Intrinsics_vec128_interleave_low64(x1, x2) \
  (_mm_unpacklo_epi64(x1, x2))

#define Lib_IntVector_Intrinsics_vec128_interleave_high64(x1, x2) \
  (_mm_unpackhi_epi64(x1, x2))

#endif /* HACL_CAN_COMPILE_VEC128 */

#if defined(HACL_CAN_COMPILE_VEC256)

#include <immintrin.h>

typedef __m256i Lib_IntVector_Intrinsics_vec256;


#define Lib_IntVector_Intrinsics_vec256_eq64(x0, x1) \
  (_mm256_cmpeq_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_eq32(x0, x1) \
  (_mm256_cmpeq_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_gt64(x0, x1) \
  (_mm256_cmpgt_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_gt32(x0, x1) \
  (_mm256_cmpgt_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_xor(x0, x1) \
  (_mm256_xor_si256(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_or(x0, x1) \
  (_mm256_or_si256(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_and(x0, x1) \
  (_mm256_and_si256(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_lognot(x0) \
  (_mm256_xor_si256(x0, _mm256_set1_epi32(-1)))

#define Lib_IntVector_Intrinsics_vec256_shift_left(x0, x1) \
  (_mm256_slli_si256(x0, (x1)/8))

#define Lib_IntVector_Intrinsics_vec256_shift_right(x0, x1) \
  (_mm256_srli_si256(x0, (x1)/8))

#define Lib_IntVector_Intrinsics_vec256_shift_left64(x0, x1) \
  (_mm256_slli_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_shift_right64(x0, x1) \
  (_mm256_srli_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_shift_left32(x0, x1) \
  (_mm256_slli_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_shift_right32(x0, x1) \
  (_mm256_srli_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_rotate_left32_8(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(14,13,12,15,10,9,8,11,6,5,4,7,2,1,0,3,14,13,12,15,10,9,8,11,6,5,4,7,2,1,0,3)))

#define Lib_IntVector_Intrinsics_vec256_rotate_left32_16(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2,13,12,15,14,9,8,11,10,5,4,7,6,1,0,3,2)))

#define Lib_IntVector_Intrinsics_vec256_rotate_left32_24(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(12,15,14,13,8,11,10,9,4,7,6,5,0,3,2,1,12,15,14,13,8,11,10,9,4,7,6,5,0,3,2,1)))

#define Lib_IntVector_Intrinsics_vec256_rotate_left32(x0,x1)	\
  ((x1 == 8? Lib_IntVector_Intrinsics_vec256_rotate_left32_8(x0) : \
   (x1 == 16? Lib_IntVector_Intrinsics_vec256_rotate_left32_16(x0) : \
   (x1 == 24? Lib_IntVector_Intrinsics_vec256_rotate_left32_24(x0) : \
   _mm256_or_si256(_mm256_slli_epi32(x0,x1),_mm256_srli_epi32(x0,32-(x1)))))))

#define Lib_IntVector_Intrinsics_vec256_rotate_right32(x0,x1)	\
  (Lib_IntVector_Intrinsics_vec256_rotate_left32(x0,32-(x1)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64_8(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(8,15,14,13,12,11,10,9,0,7,6,5,4,3,2,1,8,15,14,13,12,11,10,9,0,7,6,5,4,3,2,1)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64_16(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(9,8,15,14,13,12,11,10,1,0,7,6,5,4,3,2,9,8,15,14,13,12,11,10,1,0,7,6,5,4,3,2)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64_24(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(10,9,8,15,14,13,12,11,2,1,0,7,6,5,4,3,10,9,8,15,14,13,12,11,2,1,0,7,6,5,4,3)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64_32(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(11,10,9,8,15,14,13,12,3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,3,2,1,0,7,6,5,4)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64_40(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(12,11,10,9,8,15,14,13,4,3,2,1,0,7,6,5,12,11,10,9,8,15,14,13,4,3,2,1,0,7,6,5)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64_48(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(13,12,11,10,9,8,15,14,5,4,3,2,1,0,7,6,13,12,11,10,9,8,15,14,5,4,3,2,1,0,7,6)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64_56(x0) \
  (_mm256_shuffle_epi8(x0, _mm256_set_epi8(14,13,12,11,10,9,8,15,6,5,4,3,2,1,0,7,14,13,12,11,10,9,8,15,6,5,4,3,2,1,0,7)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right64(x0,x1)	\
  ((x1 == 8? Lib_IntVector_Intrinsics_vec256_rotate_right64_8(x0) : \
   (x1 == 16? Lib_IntVector_Intrinsics_vec256_rotate_right64_16(x0) : \
   (x1 == 24? Lib_IntVector_Intrinsics_vec256_rotate_right64_24(x0) : \
   (x1 == 32? Lib_IntVector_Intrinsics_vec256_rotate_right64_32(x0) : \
   (x1 == 40? Lib_IntVector_Intrinsics_vec256_rotate_right64_40(x0) : \
   (x1 == 48? Lib_IntVector_Intrinsics_vec256_rotate_right64_48(x0) : \
   (x1 == 56? Lib_IntVector_Intrinsics_vec256_rotate_right64_56(x0) : \
   _mm256_xor_si256(_mm256_srli_epi64((x0),(x1)),_mm256_slli_epi64((x0),(64-(x1))))))))))))

#define Lib_IntVector_Intrinsics_vec256_rotate_left64(x0,x1)	\
  (Lib_IntVector_Intrinsics_vec256_rotate_right64(x0,64-(x1)))

#define Lib_IntVector_Intrinsics_vec256_shuffle64(x0,  x1, x2, x3, x4)	\
  (_mm256_permute4x64_epi64(x0, _MM_SHUFFLE(x4,x3,x2,x1)))

#define Lib_IntVector_Intrinsics_vec256_shuffle32(x0, x1, x2, x3, x4, x5, x6, x7, x8)	\
  (_mm256_permutevar8x32_epi32(x0, _mm256_set_epi32(x8,x7,x6,x5,x4,x3,x2,x1)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right_lanes32(x0, x1)	\
  (_mm256_permutevar8x32_epi32(x0, _mm256_set_epi32((x1+7)%8,(x1+6)%8,(x1+5)%8,(x1+4)%8,(x1+3%8),(x1+2)%8,(x1+1)%8,x1%8)))

#define Lib_IntVector_Intrinsics_vec256_rotate_right_lanes64(x0, x1)	\
  (_mm256_permute4x64_epi64(x0, _MM_SHUFFLE((x1+3)%4,(x1+2)%4,(x1+1)%4,x1%4)))

#define Lib_IntVector_Intrinsics_vec256_load32_le(x0) \
  (_mm256_loadu_si256((__m256i*)(x0)))

#define Lib_IntVector_Intrinsics_vec256_load64_le(x0) \
  (_mm256_loadu_si256((__m256i*)(x0)))

#define Lib_IntVector_Intrinsics_vec256_load32_be(x0)		\
  (_mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*)(x0)), _mm256_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3)))

#define Lib_IntVector_Intrinsics_vec256_load64_be(x0)		\
  (_mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*)(x0)), _mm256_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7)))


#define Lib_IntVector_Intrinsics_vec256_store32_le(x0, x1) \
  (_mm256_storeu_si256((__m256i*)(x0), x1))

#define Lib_IntVector_Intrinsics_vec256_store64_le(x0, x1) \
  (_mm256_storeu_si256((__m256i*)(x0), x1))

#define Lib_IntVector_Intrinsics_vec256_store32_be(x0, x1)	\
  (_mm256_storeu_si256((__m256i*)(x0), _mm256_shuffle_epi8(x1, _mm256_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3))))

#define Lib_IntVector_Intrinsics_vec256_store64_be(x0, x1)	\
  (_mm256_storeu_si256((__m256i*)(x0), _mm256_shuffle_epi8(x1, _mm256_set_epi8(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7))))


#define Lib_IntVector_Intrinsics_vec256_insert8(x0, x1, x2)	\
  (_mm256_insert_epi8(x0, x1, x2))

#define Lib_IntVector_Intrinsics_vec256_insert32(x0, x1, x2)	\
  (_mm256_insert_epi32(x0, x1, x2))

#define Lib_IntVector_Intrinsics_vec256_insert64(x0, x1, x2)	\
  (_mm256_insert_epi64(x0, x1, x2))

#define Lib_IntVector_Intrinsics_vec256_extract8(x0, x1)	\
  (_mm256_extract_epi8(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_extract32(x0, x1)	\
  (_mm256_extract_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_extract64(x0, x1)	\
  (_mm256_extract_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_zero  \
  (_mm256_setzero_si256())

#define Lib_IntVector_Intrinsics_vec256_add64(x0, x1) \
  (_mm256_add_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_sub64(x0, x1)		\
  (_mm256_sub_epi64(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_mul64(x0, x1) \
  (_mm256_mul_epu32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_smul64(x0, x1) \
  (_mm256_mul_epu32(x0, _mm256_set1_epi64x(x1)))


#define Lib_IntVector_Intrinsics_vec256_add32(x0, x1) \
  (_mm256_add_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_sub32(x0, x1)		\
  (_mm256_sub_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_mul32(x0, x1) \
  (_mm256_mullo_epi32(x0, x1))

#define Lib_IntVector_Intrinsics_vec256_smul32(x0, x1) \
  (_mm256_mullo_epi32(x0, _mm256_set1_epi32(x1)))


#define Lib_IntVector_Intrinsics_vec256_load64(x1) \
  (_mm256_set1_epi64x(x1)) /* hi lo */

#define Lib_IntVector_Intrinsics_vec256_load64s(x0, x1, x2, x3) \
  (_mm256_set_epi64x(x3,x2,x1,x0)) /* hi lo */

#define Lib_IntVector_Intrinsics_vec256_load32(x) \
  (_mm256_set1_epi32(x))

#define Lib_IntVector_Intrinsics_vec256_load32s(x0,x1,x2,x3,x4, x5, x6, x7) \
  (_mm256_set_epi32(x7, x6, x5, x4, x3, x2, x1, x0)) /* hi lo */

#define Lib_IntVector_Intrinsics_vec256_load128(x) \
  (_mm256_set_m128i((__m128i)x))

#define Lib_IntVector_Intrinsics_vec256_load128s(x0,x1) \
  (_mm256_set_m128i((__m128i)x1,(__m128i)x0))

#define Lib_IntVector_Intrinsics_vec256_interleave_low32(x1, x2) \
  (_mm256_unpacklo_epi32(x1, x2))

#define Lib_IntVector_Intrinsics_vec256_interleave_high32(x1, x2) \
  (_mm256_unpackhi_epi32(x1, x2))

#define Lib_IntVector_Intrinsics_vec256_interleave_low64(x1, x2) \
  (_mm256_unpacklo_epi64(x1, x2))

#define Lib_IntVector_Intrinsics_vec256_interleave_high64(x1, x2) \
  (_mm256_unpackhi_epi64(x1, x2))

#define Lib_IntVector_Intrinsics_vec256_interleave_low128(x1, x2) \
  (_mm256_permute2x128_si256(x1, x2, 0x20))

#define Lib_IntVector_Intrinsics_vec256_interleave_high128(x1, x2) \
  (_mm256_permute2x128_si256(x1, x2, 0x31))

#endif /* HACL_CAN_COMPILE_VEC256 */

#elif (defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)) \
      && !defined(__ARM_32BIT_STATE)

#if defined(HACL_CAN_COMPILE_VEC128)

#include <arm_neon.h>

typedef uint32x4_t Lib_IntVector_Intrinsics_vec128;

#define Lib_IntVector_Intrinsics_vec128_xor(x0, x1) \
  (veorq_u32(x0,x1))

#define Lib_IntVector_Intrinsics_vec128_eq64(x0, x1) \
  (vceqq_u32(x0,x1))

#define Lib_IntVector_Intrinsics_vec128_eq32(x0, x1) \
  (vceqq_u32(x0,x1))

#define Lib_IntVector_Intrinsics_vec128_gt32(x0, x1) \
  (vcgtq_u32(x0, x1))

#define high32(x0) \
  (vmovn_u64(vshrq_n_u64(vreinterpretq_u64_u32(x0),32)))

#define low32(x0) \
  (vmovn_u64(vreinterpretq_u64_u32(x0)))

#define Lib_IntVector_Intrinsics_vec128_gt64(x0, x1) \
  (vreinterpretq_u32_u64(vmovl_u32(vorr_u32(vcgt_u32(high32(x0),high32(x1)),vand_u32(vceq_u32(high32(x0),high32(x1)),vcgt_u32(low32(x0),low32(x1)))))))

#define Lib_IntVector_Intrinsics_vec128_or(x0, x1) \
  (vorrq_u32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_and(x0, x1) \
  (vandq_u32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_lognot(x0) \
  (vmvnq_u32(x0))


#define Lib_IntVector_Intrinsics_vec128_shift_left(x0, x1) \
  (vextq_u32(x0, vdupq_n_u8(0), 16-(x1)/8))

#define Lib_IntVector_Intrinsics_vec128_shift_right(x0, x1) \
  (vextq_u32(x0, vdupq_n_u8(0), (x1)/8))

#define Lib_IntVector_Intrinsics_vec128_shift_left64(x0, x1) \
  (vreinterpretq_u32_u64(vshlq_n_u64(vreinterpretq_u64_u32(x0), x1)))

#define Lib_IntVector_Intrinsics_vec128_shift_right64(x0, x1) \
  (vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(x0), x1)))

#define Lib_IntVector_Intrinsics_vec128_shift_left32(x0, x1) \
  (vshlq_n_u32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_shift_right32(x0, x1) \
  (vshrq_n_u32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32_16(x1)	\
  (vreinterpretq_u32_u16(vrev32q_u16(vreinterpretq_u16_u32(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32(x0,x1)	\
  (((x1) == 16? Lib_IntVector_Intrinsics_vec128_rotate_left32_16(x0) : \
                vsriq_n_u32(vshlq_n_u32((x0),(x1)),(x0),32-(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_right32_16(x1)	\
  (vreinterpretq_u32_u16(vrev32q_u16(vreinterpretq_u16_u32(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_right32(x0,x1)	\
  (((x1) == 16? Lib_IntVector_Intrinsics_vec128_rotate_right32_16(x0) : \
                vsriq_n_u32(vshlq_n_u32((x0),32-(x1)),(x0),(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_right_lanes32(x0, x1)	\
  (vextq_u32(x0,x0,x1))

#define Lib_IntVector_Intrinsics_vec128_rotate_right_lanes64(x0, x1)	\
  (vextq_u64(x0,x0,x1))


/*
#define Lib_IntVector_Intrinsics_vec128_shuffle32(x0, x1, x2, x3, x4)	\
  (_mm_shuffle_epi32(x0, _MM_SHUFFLE(x1,x2,x3,x4)))

#define Lib_IntVector_Intrinsics_vec128_shuffle64(x0, x1, x2) \
  (_mm_shuffle_epi32(x0, _MM_SHUFFLE(2*x1+1,2*x1,2*x2+1,2*x2)))
*/

#define Lib_IntVector_Intrinsics_vec128_load32_le(x0) \
  (vld1q_u32((const uint32_t*) (x0)))

#define Lib_IntVector_Intrinsics_vec128_load64_le(x0) \
  (vld1q_u32((const uint32_t*) (x0)))

#define Lib_IntVector_Intrinsics_vec128_store32_le(x0, x1) \
  (vst1q_u32((uint32_t*)(x0),(x1)))

#define Lib_IntVector_Intrinsics_vec128_store64_le(x0, x1) \
  (vst1q_u32((uint32_t*)(x0),(x1)))

/*
#define Lib_IntVector_Intrinsics_vec128_load_be(x0)		\
  (     Lib_IntVector_Intrinsics_vec128 l = vrev64q_u8(vld1q_u32((uint32_t*)(x0)));

*/

#define Lib_IntVector_Intrinsics_vec128_load32_be(x0)		\
  (vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(vld1q_u32((const uint32_t*)(x0))))))

#define Lib_IntVector_Intrinsics_vec128_load64_be(x0)		\
  (vreinterpretq_u32_u8(vrev64q_u8(vreinterpretq_u8_u32(vld1q_u32((const uint32_t*)(x0))))))

/*
#define Lib_IntVector_Intrinsics_vec128_store_be(x0, x1)	\
  (_mm_storeu_si128((__m128i*)(x0), _mm_shuffle_epi8(x1, _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15))))
*/

#define Lib_IntVector_Intrinsics_vec128_store32_be(x0, x1)	\
  (vst1q_u32((uint32_t*)(x0),(vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(x1))))))

#define Lib_IntVector_Intrinsics_vec128_store64_be(x0, x1)	\
  (vst1q_u32((uint32_t*)(x0),(vreinterpretq_u32_u8(vrev64q_u8(vreinterpretq_u8_u32(x1))))))

#define Lib_IntVector_Intrinsics_vec128_insert8(x0, x1, x2)	\
  (vsetq_lane_u8(x1,x0,x2))

#define Lib_IntVector_Intrinsics_vec128_insert32(x0, x1, x2)	\
  (vsetq_lane_u32(x1,x0,x2))

#define Lib_IntVector_Intrinsics_vec128_insert64(x0, x1, x2)	\
  (vreinterpretq_u32_u64(vsetq_lane_u64(x1,vreinterpretq_u64_u32(x0),x2)))

#define Lib_IntVector_Intrinsics_vec128_extract8(x0, x1)	\
  (vgetq_lane_u8(x0,x1))

#define Lib_IntVector_Intrinsics_vec128_extract32(x0, x1)	\
  (vgetq_lane_u32(x0,x1))

#define Lib_IntVector_Intrinsics_vec128_extract64(x0, x1)	\
  (vgetq_lane_u64(vreinterpretq_u64_u32(x0),x1))

#define Lib_IntVector_Intrinsics_vec128_zero  \
  (vdupq_n_u32(0))

#define Lib_IntVector_Intrinsics_vec128_add64(x0, x1) \
  (vreinterpretq_u32_u64(vaddq_u64(vreinterpretq_u64_u32(x0), vreinterpretq_u64_u32(x1))))

#define Lib_IntVector_Intrinsics_vec128_sub64(x0, x1)		\
  (vreinterpretq_u32_u64(vsubq_u64(vreinterpretq_u64_u32(x0), vreinterpretq_u64_u32(x1))))

#define Lib_IntVector_Intrinsics_vec128_mul64(x0, x1) \
  (vreinterpretq_u32_u64(vmull_u32(vmovn_u64(vreinterpretq_u64_u32(x0)), vmovn_u64(vreinterpretq_u64_u32(x1)))))

#define Lib_IntVector_Intrinsics_vec128_smul64(x0, x1) \
  (vreinterpretq_u32_u64(vmull_n_u32(vmovn_u64(vreinterpretq_u64_u32(x0)), (uint32_t)x1)))

#define Lib_IntVector_Intrinsics_vec128_add32(x0, x1) \
  (vaddq_u32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_sub32(x0, x1)		\
  (vsubq_u32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_mul32(x0, x1) \
  (vmulq_lane_u32(x0, x1))

#define Lib_IntVector_Intrinsics_vec128_smul32(x0, x1) \
  (vmulq_lane_u32(x0, vdupq_n_u32(x1)))

#define Lib_IntVector_Intrinsics_vec128_load128(x) \
  ((uint32x4_t)(x))

#define Lib_IntVector_Intrinsics_vec128_load64(x) \
  (vreinterpretq_u32_u64(vdupq_n_u64(x))) /* hi lo */

#define Lib_IntVector_Intrinsics_vec128_load32(x) \
  (vdupq_n_u32(x)) /* hi lo */

static inline Lib_IntVector_Intrinsics_vec128 Lib_IntVector_Intrinsics_vec128_load64s(uint64_t x1, uint64_t x2){
  const uint64_t a[2] = {x1,x2};
  return vreinterpretq_u32_u64(vld1q_u64(a));
}

static inline Lib_IntVector_Intrinsics_vec128 Lib_IntVector_Intrinsics_vec128_load32s(uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4){
  const uint32_t a[4] = {x1,x2,x3,x4};
  return vld1q_u32(a);
}

#define Lib_IntVector_Intrinsics_vec128_interleave_low32(x1, x2) \
  (vzip1q_u32(x1,x2))

#define Lib_IntVector_Intrinsics_vec128_interleave_high32(x1, x2) \
  (vzip2q_u32(x1,x2))

#define Lib_IntVector_Intrinsics_vec128_interleave_low64(x1,x2) \
  (vreinterpretq_u32_u64(vzip1q_u64(vreinterpretq_u64_u32(x1),vreinterpretq_u64_u32(x2))))

#define Lib_IntVector_Intrinsics_vec128_interleave_high64(x1,x2) \
  (vreinterpretq_u32_u64(vzip2q_u64(vreinterpretq_u64_u32(x1),vreinterpretq_u64_u32(x2))))

#endif /* HACL_CAN_COMPILE_VEC128 */

/* IBM z architecture */
#elif defined(__s390x__) /* this flag is for GCC only */

#if defined(HACL_CAN_COMPILE_VEC128)

#include <stdint.h>
#include <vecintrin.h>

/* The main vector 128 type
 * We can't use uint8_t, uint32_t, uint64_t... instead of unsigned char,
 * unsigned int, unsigned long long: the compiler complains that the parameter
 * combination is invalid. */
typedef unsigned char vector128_8 __attribute__ ((vector_size(16)));
typedef unsigned int vector128_32 __attribute__ ((vector_size(16)));
typedef unsigned long long vector128_64 __attribute__ ((vector_size(16)));

typedef vector128_8 Lib_IntVector_Intrinsics_vec128;
typedef vector128_8 vector128;

#define Lib_IntVector_Intrinsics_vec128_load32_le(x)                 \
  (vector128) ((vector128_32) vec_revb(*((vector128_32*) (const uint8_t*)(x))))

#define Lib_IntVector_Intrinsics_vec128_load32_be(x)                 \
  (vector128) (*((vector128_32*) (const uint8_t*)(x)))

#define Lib_IntVector_Intrinsics_vec128_load64_le(x)                 \
  (vector128) ((vector128_64) vec_revb(*((vector128_64*) (const uint8_t*)(x))))

static inline
void Lib_IntVector_Intrinsics_vec128_store32_le(const uint8_t *x0, vector128 x1) {
  *((vector128_32*)x0) = vec_revb((vector128_32) x1);
}

static inline
void Lib_IntVector_Intrinsics_vec128_store32_be(const uint8_t *x0, vector128 x1) {
  *((vector128_32*)x0) = (vector128_32) x1;
}

static inline
void Lib_IntVector_Intrinsics_vec128_store64_le(const uint8_t *x0, vector128 x1) {
  *((vector128_64*)x0) = vec_revb((vector128_64) x1);
}

#define Lib_IntVector_Intrinsics_vec128_add32(x0,x1)            \
  ((vector128)((vector128_32)(((vector128_32)(x0)) + ((vector128_32)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_add64(x0, x1)           \
  ((vector128)((vector128_64)(((vector128_64)(x0)) + ((vector128_64)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_and(x0, x1)             \
  ((vector128)(vec_and((vector128)(x0),(vector128)(x1))))

#define Lib_IntVector_Intrinsics_vec128_eq32(x0, x1)            \
  ((vector128)(vec_cmpeq(((vector128_32)(x0)),((vector128_32)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_eq64(x0, x1)            \
  ((vector128)(vec_cmpeq(((vector128_64)(x0)),((vector128_64)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_extract32(x0, x1)       \
  ((unsigned int)(vec_extract((vector128_32)(x0), x1)))

#define Lib_IntVector_Intrinsics_vec128_extract64(x0, x1)       \
  ((unsigned long long)(vec_extract((vector128_64)(x0), x1)))

#define Lib_IntVector_Intrinsics_vec128_gt32(x0, x1)                   \
  ((vector128)((vector128_32)(((vector128_32)(x0)) > ((vector128_32)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_gt64(x0, x1)                   \
  ((vector128)((vector128_64)(((vector128_64)(x0)) > ((vector128_64)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_insert32(x0, x1, x2)           \
  ((vector128)((vector128_32)vec_insert((unsigned int)(x1), (vector128_32)(x0), x2)))

#define Lib_IntVector_Intrinsics_vec128_insert64(x0, x1, x2)           \
  ((vector128)((vector128_64)vec_insert((unsigned long long)(x1), (vector128_64)(x0), x2)))

#define Lib_IntVector_Intrinsics_vec128_interleave_high32(x0, x1)      \
  ((vector128)((vector128_32)vec_mergel((vector128_32)(x0), (vector128_32)(x1))))

#define Lib_IntVector_Intrinsics_vec128_interleave_high64(x0, x1)      \
  ((vector128)((vector128_64)vec_mergel((vector128_64)(x0), (vector128_64)(x1))))

#define Lib_IntVector_Intrinsics_vec128_interleave_low32(x0, x1)       \
  ((vector128)((vector128_32)vec_mergeh((vector128_32)(x0), (vector128_32)(x1))))

#define Lib_IntVector_Intrinsics_vec128_interleave_low64(x0, x1)       \
  ((vector128)((vector128_64)vec_mergeh((vector128_64)(x0), (vector128_64)(x1))))

#define Lib_IntVector_Intrinsics_vec128_load32(x)                      \
  ((vector128)((vector128_32){(unsigned int)(x), (unsigned int)(x),     \
        (unsigned int)(x), (unsigned int)(x)}))

#define Lib_IntVector_Intrinsics_vec128_load32s(x0, x1, x2, x3) \
  ((vector128)((vector128_32){(unsigned int)(x0),(unsigned int)(x1),(unsigned int)(x2),(unsigned int)(x3)}))

#define Lib_IntVector_Intrinsics_vec128_load64(x)                      \
  ((vector128)((vector128_64)vec_load_pair((unsigned long long)(x),(unsigned long long)(x))))

#define Lib_IntVector_Intrinsics_vec128_lognot(x0)                     \
  ((vector128)(vec_xor((vector128)(x0), (vector128)vec_splat_u32(-1))))

#define Lib_IntVector_Intrinsics_vec128_mul64(x0, x1)                  \
  ((vector128)(vec_mulo((vector128_32)(x0), \
                        (vector128_32)(x1))))

#define Lib_IntVector_Intrinsics_vec128_or(x0, x1)              \
  ((vector128)(vec_or((vector128)(x0),(vector128)(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32(x0, x1)           \
  ((vector128)(vec_rli((vector128_32)(x0), (unsigned long)(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_right32(x0, x1)          \
  (Lib_IntVector_Intrinsics_vec128_rotate_left32(x0,(uint32_t)(32-(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_right_lanes32(x0, x1)    \
  ((vector128)(vec_sld((vector128)(x0), (vector128)(x0), (x1%4)*4)))

#define Lib_IntVector_Intrinsics_vec128_shift_left64(x0, x1)            \
  (((vector128)((vector128_64)vec_rli((vector128_64)(x0), (unsigned long)(x1)))) & \
   ((vector128)((vector128_64){0xffffffffffffffff << (x1), 0xffffffffffffffff << (x1)})))

#define Lib_IntVector_Intrinsics_vec128_shift_right64(x0, x1)         \
  (((vector128)((vector128_64)vec_rli((vector128_64)(x0), (unsigned long)(64-(x1))))) & \
   ((vector128)((vector128_64){0xffffffffffffffff >> (x1), 0xffffffffffffffff >> (x1)})))

#define Lib_IntVector_Intrinsics_vec128_shift_right32(x0, x1)         \
  (((vector128)((vector128_32)vec_rli((vector128_32)(x0), (unsigned int)(32-(x1))))) & \
   ((vector128)((vector128_32){0xffffffff >> (x1), 0xffffffff >> (x1), \
                               0xffffffff >> (x1), 0xffffffff >> (x1)})))

/* Doesn't work with vec_splat_u64 */
#define Lib_IntVector_Intrinsics_vec128_smul64(x0, x1)          \
  ((vector128)(Lib_IntVector_Intrinsics_vec128_mul64(x0,((vector128_64){(unsigned long long)(x1),(unsigned long long)(x1)}))))

#define Lib_IntVector_Intrinsics_vec128_sub64(x0, x1)   \
  ((vector128)((vector128_64)(x0) - (vector128_64)(x1)))

static inline
vector128 Lib_IntVector_Intrinsics_vec128_xor(vector128 x0, vector128 x1) {
  return ((vector128)(vec_xor((vector128)(x0), (vector128)(x1))));
}


#define Lib_IntVector_Intrinsics_vec128_zero \
  ((vector128){})

#endif /* HACL_CAN_COMPILE_VEC128 */

#elif defined(__powerpc64__) // PowerPC 64 - this flag is for GCC only

#if defined(HACL_CAN_COMPILE_VEC128)

#include <altivec.h>
#include <string.h> // for memcpy
#include <stdint.h>

// The main vector 128 type
// We can't use uint8_t, uint32_t, uint64_t... instead of unsigned char,
// unsigned int, unsigned long long: the compiler complains that the parameter
// combination is invalid.
typedef vector unsigned char vector128_8;
typedef vector unsigned int vector128_32;
typedef vector unsigned long long vector128_64;

typedef vector128_8 Lib_IntVector_Intrinsics_vec128;
typedef vector128_8 vector128;

#define Lib_IntVector_Intrinsics_vec128_load32_le(x) \
  ((vector128)((vector128_32)(vec_xl(0, (const unsigned int*) ((const uint8_t*)(x))))))

#define Lib_IntVector_Intrinsics_vec128_load64_le(x) \
  ((vector128)((vector128_64)(vec_xl(0, (const unsigned long long*) ((const uint8_t*)(x))))))

#define Lib_IntVector_Intrinsics_vec128_store32_le(x0, x1) \
  (vec_xst((vector128_32)(x1), 0, (unsigned int*) ((uint8_t*)(x0))))

#define Lib_IntVector_Intrinsics_vec128_store64_le(x0, x1) \
  (vec_xst((vector128_64)(x1), 0, (unsigned long long*) ((uint8_t*)(x0))))

#define Lib_IntVector_Intrinsics_vec128_add32(x0,x1)            \
  ((vector128)((vector128_32)(((vector128_32)(x0)) + ((vector128_32)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_add64(x0, x1)           \
  ((vector128)((vector128_64)(((vector128_64)(x0)) + ((vector128_64)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_and(x0, x1)             \
  ((vector128)(vec_and((vector128)(x0),(vector128)(x1))))

#define Lib_IntVector_Intrinsics_vec128_eq32(x0, x1)            \
  ((vector128)(vec_cmpeq(((vector128_32)(x0)),((vector128_32)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_eq64(x0, x1)            \
  ((vector128)(vec_cmpeq(((vector128_64)(x0)),((vector128_64)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_extract32(x0, x1)       \
  ((unsigned int)(vec_extract((vector128_32)(x0), x1)))

#define Lib_IntVector_Intrinsics_vec128_extract64(x0, x1)       \
  ((unsigned long long)(vec_extract((vector128_64)(x0), x1)))

#define Lib_IntVector_Intrinsics_vec128_gt32(x0, x1)                   \
  ((vector128)((vector128_32)(((vector128_32)(x0)) > ((vector128_32)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_gt64(x0, x1)                   \
  ((vector128)((vector128_64)(((vector128_64)(x0)) > ((vector128_64)(x1)))))

#define Lib_IntVector_Intrinsics_vec128_insert32(x0, x1, x2)           \
  ((vector128)((vector128_32)vec_insert((unsigned int)(x1), (vector128_32)(x0), x2)))

#define Lib_IntVector_Intrinsics_vec128_insert64(x0, x1, x2)           \
  ((vector128)((vector128_64)vec_insert((unsigned long long)(x1), (vector128_64)(x0), x2)))

#define Lib_IntVector_Intrinsics_vec128_interleave_high32(x0, x1)      \
  ((vector128)((vector128_32)vec_mergel((vector128_32)(x0), (vector128_32)(x1))))

#define Lib_IntVector_Intrinsics_vec128_interleave_high64(x0, x1)      \
  ((vector128)((vector128_64)vec_mergel((vector128_64)(x0), (vector128_64)(x1))))

#define Lib_IntVector_Intrinsics_vec128_interleave_low32(x0, x1)       \
  ((vector128)((vector128_32)vec_mergeh((vector128_32)(x0), (vector128_32)(x1))))

#define Lib_IntVector_Intrinsics_vec128_interleave_low64(x0, x1)       \
  ((vector128)((vector128_64)vec_mergeh((vector128_64)(x0), (vector128_64)(x1))))

#define Lib_IntVector_Intrinsics_vec128_load32(x)                      \
  ((vector128)((vector128_32){(unsigned int)(x), (unsigned int)(x),     \
        (unsigned int)(x), (unsigned int)(x)}))

#define Lib_IntVector_Intrinsics_vec128_load32s(x0, x1, x2, x3) \
  ((vector128)((vector128_32){(unsigned int)(x0),(unsigned int)(x1),(unsigned int)(x2),(unsigned int)(x3)}))

#define Lib_IntVector_Intrinsics_vec128_load64(x)                      \
  ((vector128)((vector128_64){(unsigned long long)(x),(unsigned long long)(x)}))

#define Lib_IntVector_Intrinsics_vec128_lognot(x0)                     \
  ((vector128)(vec_xor((vector128)(x0), (vector128)vec_splat_u32(-1))))

#define Lib_IntVector_Intrinsics_vec128_mul64(x0, x1)                  \
    ((vector128)(vec_mule((vector128_32)(x0),                          \
                          (vector128_32)(x1))))

#define Lib_IntVector_Intrinsics_vec128_or(x0, x1)              \
  ((vector128)(vec_or((vector128)(x0),(vector128)(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_left32(x0, x1)           \
  ((vector128)(vec_rl((vector128_32)(x0), (vector128_32){(unsigned int)(x1),(unsigned int)(x1),(unsigned int)(x1),(unsigned int)(x1)})))

#define Lib_IntVector_Intrinsics_vec128_rotate_right32(x0, x1)          \
  (Lib_IntVector_Intrinsics_vec128_rotate_left32(x0,(uint32_t)(32-(x1))))

#define Lib_IntVector_Intrinsics_vec128_rotate_right_lanes32(x0, x1)    \
  ((vector128)(vec_sld((vector128)(x0), (vector128)(x0), ((4-(x1))%4)*4)))

#define Lib_IntVector_Intrinsics_vec128_shift_left64(x0, x1)            \
  ((vector128)((vector128_64)vec_sl((vector128_64)(x0), (vector128_64){(unsigned long)(x1),(unsigned long)(x1)})))

#define Lib_IntVector_Intrinsics_vec128_shift_right64(x0, x1)         \
  ((vector128)((vector128_64)vec_sr((vector128_64)(x0), (vector128_64){(unsigned long)(x1),(unsigned long)(x1)})))

// Doesn't work with vec_splat_u64
#define Lib_IntVector_Intrinsics_vec128_smul64(x0, x1)          \
  ((vector128)(Lib_IntVector_Intrinsics_vec128_mul64(x0,((vector128_64){(unsigned long long)(x1),(unsigned long long)(x1)}))))

#define Lib_IntVector_Intrinsics_vec128_sub64(x0, x1)   \
  ((vector128)((vector128_64)(x0) - (vector128_64)(x1)))

#define Lib_IntVector_Intrinsics_vec128_xor(x0, x1)  \
  ((vector128)(vec_xor((vector128)(x0), (vector128)(x1))))

#define Lib_IntVector_Intrinsics_vec128_zero \
  ((vector128){})

#endif /* HACL_CAN_COMPILE_VEC128 */

#endif // PowerPC64

// DEBUGGING:
// If libintvector_debug.h exists, use it to debug the current implementations.
// Note that some flags must be enabled for the debugging to be effective:
// see libintvector_debug.h for more details.
#if defined(__has_include)
#if __has_include("libintvector_debug.h")
#include "libintvector_debug.h"
#endif
#endif

#endif // HACL_INTRINSICS_SHIMMED

#endif // __Vec_Intrin_H
