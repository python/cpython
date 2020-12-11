/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#if !(defined(LTM1) && defined(LTM2) && defined(LTM3))
#define LTM_INSIDE
#if defined(LTM2)
#   define LTM3
#endif
#if defined(LTM1)
#   define LTM2
#endif
#define LTM1
#if defined(LTM_ALL)
#   define BN_CUTOFFS_C
#   define BN_DEPRECATED_C
#   define BN_MP_2EXPT_C
#   define BN_MP_ABS_C
#   define BN_MP_ADD_C
#   define BN_MP_ADD_D_C
#   define BN_MP_ADDMOD_C
#   define BN_MP_AND_C
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_CNT_LSB_C
#   define BN_MP_COMPLEMENT_C
#   define BN_MP_COPY_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DECR_C
#   define BN_MP_DIV_C
#   define BN_MP_DIV_2_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_DIV_3_C
#   define BN_MP_DIV_D_C
#   define BN_MP_DR_IS_MODULUS_C
#   define BN_MP_DR_REDUCE_C
#   define BN_MP_DR_SETUP_C
#   define BN_MP_ERROR_TO_STRING_C
#   define BN_MP_EXCH_C
#   define BN_MP_EXPT_U32_C
#   define BN_MP_EXPTMOD_C
#   define BN_MP_EXTEUCLID_C
#   define BN_MP_FREAD_C
#   define BN_MP_FROM_SBIN_C
#   define BN_MP_FROM_UBIN_C
#   define BN_MP_FWRITE_C
#   define BN_MP_GCD_C
#   define BN_MP_GET_DOUBLE_C
#   define BN_MP_GET_I32_C
#   define BN_MP_GET_I64_C
#   define BN_MP_GET_L_C
#   define BN_MP_GET_LL_C
#   define BN_MP_GET_MAG_U32_C
#   define BN_MP_GET_MAG_U64_C
#   define BN_MP_GET_MAG_UL_C
#   define BN_MP_GET_MAG_ULL_C
#   define BN_MP_GROW_C
#   define BN_MP_INCR_C
#   define BN_MP_INIT_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_INIT_I32_C
#   define BN_MP_INIT_I64_C
#   define BN_MP_INIT_L_C
#   define BN_MP_INIT_LL_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_INIT_SET_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_INIT_U32_C
#   define BN_MP_INIT_U64_C
#   define BN_MP_INIT_UL_C
#   define BN_MP_INIT_ULL_C
#   define BN_MP_INVMOD_C
#   define BN_MP_IS_SQUARE_C
#   define BN_MP_ISEVEN_C
#   define BN_MP_ISODD_C
#   define BN_MP_KRONECKER_C
#   define BN_MP_LCM_C
#   define BN_MP_LOG_U32_C
#   define BN_MP_LSHD_C
#   define BN_MP_MOD_C
#   define BN_MP_MOD_2D_C
#   define BN_MP_MOD_D_C
#   define BN_MP_MONTGOMERY_CALC_NORMALIZATION_C
#   define BN_MP_MONTGOMERY_REDUCE_C
#   define BN_MP_MONTGOMERY_SETUP_C
#   define BN_MP_MUL_C
#   define BN_MP_MUL_2_C
#   define BN_MP_MUL_2D_C
#   define BN_MP_MUL_D_C
#   define BN_MP_MULMOD_C
#   define BN_MP_NEG_C
#   define BN_MP_OR_C
#   define BN_MP_PACK_C
#   define BN_MP_PACK_COUNT_C
#   define BN_MP_PRIME_FERMAT_C
#   define BN_MP_PRIME_FROBENIUS_UNDERWOOD_C
#   define BN_MP_PRIME_IS_PRIME_C
#   define BN_MP_PRIME_MILLER_RABIN_C
#   define BN_MP_PRIME_NEXT_PRIME_C
#   define BN_MP_PRIME_RABIN_MILLER_TRIALS_C
#   define BN_MP_PRIME_RAND_C
#   define BN_MP_PRIME_STRONG_LUCAS_SELFRIDGE_C
#   define BN_MP_RADIX_SIZE_C
#   define BN_MP_RADIX_SMAP_C
#   define BN_MP_RAND_C
#   define BN_MP_READ_RADIX_C
#   define BN_MP_REDUCE_C
#   define BN_MP_REDUCE_2K_C
#   define BN_MP_REDUCE_2K_L_C
#   define BN_MP_REDUCE_2K_SETUP_C
#   define BN_MP_REDUCE_2K_SETUP_L_C
#   define BN_MP_REDUCE_IS_2K_C
#   define BN_MP_REDUCE_IS_2K_L_C
#   define BN_MP_REDUCE_SETUP_C
#   define BN_MP_ROOT_U32_C
#   define BN_MP_RSHD_C
#   define BN_MP_SBIN_SIZE_C
#   define BN_MP_SET_C
#   define BN_MP_SET_DOUBLE_C
#   define BN_MP_SET_I32_C
#   define BN_MP_SET_I64_C
#   define BN_MP_SET_L_C
#   define BN_MP_SET_LL_C
#   define BN_MP_SET_U32_C
#   define BN_MP_SET_U64_C
#   define BN_MP_SET_UL_C
#   define BN_MP_SET_ULL_C
#   define BN_MP_SHRINK_C
#   define BN_MP_SIGNED_RSH_C
#   define BN_MP_SQR_C
#   define BN_MP_SQRMOD_C
#   define BN_MP_SQRT_C
#   define BN_MP_SQRTMOD_PRIME_C
#   define BN_MP_SUB_C
#   define BN_MP_SUB_D_C
#   define BN_MP_SUBMOD_C
#   define BN_MP_TO_RADIX_C
#   define BN_MP_TO_SBIN_C
#   define BN_MP_TO_UBIN_C
#   define BN_MP_UBIN_SIZE_C
#   define BN_MP_UNPACK_C
#   define BN_MP_XOR_C
#   define BN_MP_ZERO_C
#   define BN_PRIME_TAB_C
#   define BN_S_MP_ADD_C
#   define BN_S_MP_BALANCE_MUL_C
#   define BN_S_MP_EXPTMOD_C
#   define BN_S_MP_EXPTMOD_FAST_C
#   define BN_S_MP_GET_BIT_C
#   define BN_S_MP_INVMOD_FAST_C
#   define BN_S_MP_INVMOD_SLOW_C
#   define BN_S_MP_KARATSUBA_MUL_C
#   define BN_S_MP_KARATSUBA_SQR_C
#   define BN_S_MP_MONTGOMERY_REDUCE_FAST_C
#   define BN_S_MP_MUL_DIGS_C
#   define BN_S_MP_MUL_DIGS_FAST_C
#   define BN_S_MP_MUL_HIGH_DIGS_C
#   define BN_S_MP_MUL_HIGH_DIGS_FAST_C
#   define BN_S_MP_PRIME_IS_DIVISIBLE_C
#   define BN_S_MP_RAND_JENKINS_C
#   define BN_S_MP_RAND_PLATFORM_C
#   define BN_S_MP_REVERSE_C
#   define BN_S_MP_SQR_C
#   define BN_S_MP_SQR_FAST_C
#   define BN_S_MP_SUB_C
#   define BN_S_MP_TOOM_MUL_C
#   define BN_S_MP_TOOM_SQR_C
#endif
#endif
#if defined(BN_CUTOFFS_C)
#endif

#if defined(BN_DEPRECATED_C)
#   define BN_FAST_MP_INVMOD_C
#   define BN_FAST_MP_MONTGOMERY_REDUCE_C
#   define BN_FAST_S_MP_MUL_DIGS_C
#   define BN_FAST_S_MP_MUL_HIGH_DIGS_C
#   define BN_FAST_S_MP_SQR_C
#   define BN_MP_AND_C
#   define BN_MP_BALANCE_MUL_C
#   define BN_MP_CMP_D_C
#   define BN_MP_EXPORT_C
#   define BN_MP_EXPTMOD_FAST_C
#   define BN_MP_EXPT_D_C
#   define BN_MP_EXPT_D_EX_C
#   define BN_MP_EXPT_U32_C
#   define BN_MP_FROM_SBIN_C
#   define BN_MP_FROM_UBIN_C
#   define BN_MP_GET_BIT_C
#   define BN_MP_GET_INT_C
#   define BN_MP_GET_LONG_C
#   define BN_MP_GET_LONG_LONG_C
#   define BN_MP_GET_MAG_U32_C
#   define BN_MP_GET_MAG_ULL_C
#   define BN_MP_GET_MAG_UL_C
#   define BN_MP_IMPORT_C
#   define BN_MP_INIT_SET_INT_C
#   define BN_MP_INIT_U32_C
#   define BN_MP_INVMOD_SLOW_C
#   define BN_MP_JACOBI_C
#   define BN_MP_KARATSUBA_MUL_C
#   define BN_MP_KARATSUBA_SQR_C
#   define BN_MP_KRONECKER_C
#   define BN_MP_N_ROOT_C
#   define BN_MP_N_ROOT_EX_C
#   define BN_MP_OR_C
#   define BN_MP_PACK_C
#   define BN_MP_PRIME_IS_DIVISIBLE_C
#   define BN_MP_PRIME_RANDOM_EX_C
#   define BN_MP_RAND_DIGIT_C
#   define BN_MP_READ_SIGNED_BIN_C
#   define BN_MP_READ_UNSIGNED_BIN_C
#   define BN_MP_ROOT_U32_C
#   define BN_MP_SBIN_SIZE_C
#   define BN_MP_SET_INT_C
#   define BN_MP_SET_LONG_C
#   define BN_MP_SET_LONG_LONG_C
#   define BN_MP_SET_U32_C
#   define BN_MP_SET_U64_C
#   define BN_MP_SIGNED_BIN_SIZE_C
#   define BN_MP_SIGNED_RSH_C
#   define BN_MP_TC_AND_C
#   define BN_MP_TC_DIV_2D_C
#   define BN_MP_TC_OR_C
#   define BN_MP_TC_XOR_C
#   define BN_MP_TOOM_MUL_C
#   define BN_MP_TOOM_SQR_C
#   define BN_MP_TORADIX_C
#   define BN_MP_TORADIX_N_C
#   define BN_MP_TO_RADIX_C
#   define BN_MP_TO_SBIN_C
#   define BN_MP_TO_SIGNED_BIN_C
#   define BN_MP_TO_SIGNED_BIN_N_C
#   define BN_MP_TO_UBIN_C
#   define BN_MP_TO_UNSIGNED_BIN_C
#   define BN_MP_TO_UNSIGNED_BIN_N_C
#   define BN_MP_UBIN_SIZE_C
#   define BN_MP_UNPACK_C
#   define BN_MP_UNSIGNED_BIN_SIZE_C
#   define BN_MP_XOR_C
#   define BN_S_MP_BALANCE_MUL_C
#   define BN_S_MP_EXPTMOD_FAST_C
#   define BN_S_MP_GET_BIT_C
#   define BN_S_MP_INVMOD_FAST_C
#   define BN_S_MP_INVMOD_SLOW_C
#   define BN_S_MP_KARATSUBA_MUL_C
#   define BN_S_MP_KARATSUBA_SQR_C
#   define BN_S_MP_MONTGOMERY_REDUCE_FAST_C
#   define BN_S_MP_MUL_DIGS_FAST_C
#   define BN_S_MP_MUL_HIGH_DIGS_FAST_C
#   define BN_S_MP_PRIME_IS_DIVISIBLE_C
#   define BN_S_MP_PRIME_RANDOM_EX_C
#   define BN_S_MP_RAND_SOURCE_C
#   define BN_S_MP_REVERSE_C
#   define BN_S_MP_SQR_FAST_C
#   define BN_S_MP_TOOM_MUL_C
#   define BN_S_MP_TOOM_SQR_C
#endif

#if defined(BN_MP_2EXPT_C)
#   define BN_MP_GROW_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_ABS_C)
#   define BN_MP_COPY_C
#endif

#if defined(BN_MP_ADD_C)
#   define BN_MP_CMP_MAG_C
#   define BN_S_MP_ADD_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_ADD_D_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#   define BN_MP_SUB_D_C
#endif

#if defined(BN_MP_ADDMOD_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_C
#   define BN_MP_MOD_C
#endif

#if defined(BN_MP_AND_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_CLAMP_C)
#endif

#if defined(BN_MP_CLEAR_C)
#endif

#if defined(BN_MP_CLEAR_MULTI_C)
#   define BN_MP_CLEAR_C
#endif

#if defined(BN_MP_CMP_C)
#   define BN_MP_CMP_MAG_C
#endif

#if defined(BN_MP_CMP_D_C)
#endif

#if defined(BN_MP_CMP_MAG_C)
#endif

#if defined(BN_MP_CNT_LSB_C)
#endif

#if defined(BN_MP_COMPLEMENT_C)
#   define BN_MP_NEG_C
#   define BN_MP_SUB_D_C
#endif

#if defined(BN_MP_COPY_C)
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_COUNT_BITS_C)
#endif

#if defined(BN_MP_DECR_C)
#   define BN_MP_INCR_C
#   define BN_MP_SET_C
#   define BN_MP_SUB_D_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_DIV_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_COPY_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_LSHD_C
#   define BN_MP_MUL_2D_C
#   define BN_MP_MUL_D_C
#   define BN_MP_RSHD_C
#   define BN_MP_SUB_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_DIV_2_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_DIV_2D_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_COPY_C
#   define BN_MP_MOD_2D_C
#   define BN_MP_RSHD_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_DIV_3_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_SIZE_C
#endif

#if defined(BN_MP_DIV_D_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_COPY_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_DIV_3_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_SIZE_C
#endif

#if defined(BN_MP_DR_IS_MODULUS_C)
#endif

#if defined(BN_MP_DR_REDUCE_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_GROW_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_DR_SETUP_C)
#endif

#if defined(BN_MP_ERROR_TO_STRING_C)
#endif

#if defined(BN_MP_EXCH_C)
#endif

#if defined(BN_MP_EXPT_U32_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_MUL_C
#   define BN_MP_SET_C
#   define BN_MP_SQR_C
#endif

#if defined(BN_MP_EXPTMOD_C)
#   define BN_MP_ABS_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_DR_IS_MODULUS_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_INVMOD_C
#   define BN_MP_REDUCE_IS_2K_C
#   define BN_MP_REDUCE_IS_2K_L_C
#   define BN_S_MP_EXPTMOD_C
#   define BN_S_MP_EXPTMOD_FAST_C
#endif

#if defined(BN_MP_EXTEUCLID_C)
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_COPY_C
#   define BN_MP_DIV_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_MUL_C
#   define BN_MP_NEG_C
#   define BN_MP_SET_C
#   define BN_MP_SUB_C
#endif

#if defined(BN_MP_FREAD_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_MUL_D_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_FROM_SBIN_C)
#   define BN_MP_FROM_UBIN_C
#endif

#if defined(BN_MP_FROM_UBIN_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#   define BN_MP_MUL_2D_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_FWRITE_C)
#   define BN_MP_RADIX_SIZE_C
#   define BN_MP_TO_RADIX_C
#endif

#if defined(BN_MP_GCD_C)
#   define BN_MP_ABS_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_CNT_LSB_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_MUL_2D_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_GET_DOUBLE_C)
#endif

#if defined(BN_MP_GET_I32_C)
#   define BN_MP_GET_MAG_U32_C
#endif

#if defined(BN_MP_GET_I64_C)
#   define BN_MP_GET_MAG_U64_C
#endif

#if defined(BN_MP_GET_L_C)
#   define BN_MP_GET_MAG_UL_C
#endif

#if defined(BN_MP_GET_LL_C)
#   define BN_MP_GET_MAG_ULL_C
#endif

#if defined(BN_MP_GET_MAG_U32_C)
#endif

#if defined(BN_MP_GET_MAG_U64_C)
#endif

#if defined(BN_MP_GET_MAG_UL_C)
#endif

#if defined(BN_MP_GET_MAG_ULL_C)
#endif

#if defined(BN_MP_GROW_C)
#endif

#if defined(BN_MP_INCR_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_DECR_C
#   define BN_MP_SET_C
#endif

#if defined(BN_MP_INIT_C)
#endif

#if defined(BN_MP_INIT_COPY_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_COPY_C
#   define BN_MP_INIT_SIZE_C
#endif

#if defined(BN_MP_INIT_I32_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_I32_C
#endif

#if defined(BN_MP_INIT_I64_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_I64_C
#endif

#if defined(BN_MP_INIT_L_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_L_C
#endif

#if defined(BN_MP_INIT_LL_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_LL_C
#endif

#if defined(BN_MP_INIT_MULTI_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_C
#endif

#if defined(BN_MP_INIT_SET_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_C
#endif

#if defined(BN_MP_INIT_SIZE_C)
#endif

#if defined(BN_MP_INIT_U32_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_U32_C
#endif

#if defined(BN_MP_INIT_U64_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_U64_C
#endif

#if defined(BN_MP_INIT_UL_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_UL_C
#endif

#if defined(BN_MP_INIT_ULL_C)
#   define BN_MP_INIT_C
#   define BN_MP_SET_ULL_C
#endif

#if defined(BN_MP_INVMOD_C)
#   define BN_MP_CMP_D_C
#   define BN_S_MP_INVMOD_FAST_C
#   define BN_S_MP_INVMOD_SLOW_C
#endif

#if defined(BN_MP_IS_SQUARE_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_GET_I32_C
#   define BN_MP_INIT_U32_C
#   define BN_MP_MOD_C
#   define BN_MP_MOD_D_C
#   define BN_MP_SQRT_C
#   define BN_MP_SQR_C
#endif

#if defined(BN_MP_ISEVEN_C)
#endif

#if defined(BN_MP_ISODD_C)
#endif

#if defined(BN_MP_KRONECKER_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_D_C
#   define BN_MP_CNT_LSB_C
#   define BN_MP_COPY_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_INIT_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_MOD_C
#endif

#if defined(BN_MP_LCM_C)
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_DIV_C
#   define BN_MP_GCD_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_MUL_C
#endif

#if defined(BN_MP_LOG_U32_C)
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_COPY_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_EXCH_C
#   define BN_MP_EXPT_U32_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_MUL_C
#   define BN_MP_SET_C
#   define BN_MP_SQR_C
#endif

#if defined(BN_MP_LSHD_C)
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_MOD_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLEAR_C
#   define BN_MP_DIV_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_SIZE_C
#endif

#if defined(BN_MP_MOD_2D_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_COPY_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_MOD_D_C)
#   define BN_MP_DIV_D_C
#endif

#if defined(BN_MP_MONTGOMERY_CALC_NORMALIZATION_C)
#   define BN_MP_2EXPT_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_MUL_2_C
#   define BN_MP_SET_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_MONTGOMERY_REDUCE_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_GROW_C
#   define BN_MP_RSHD_C
#   define BN_S_MP_MONTGOMERY_REDUCE_FAST_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_MONTGOMERY_SETUP_C)
#endif

#if defined(BN_MP_MUL_C)
#   define BN_S_MP_BALANCE_MUL_C
#   define BN_S_MP_KARATSUBA_MUL_C
#   define BN_S_MP_MUL_DIGS_C
#   define BN_S_MP_MUL_DIGS_FAST_C
#   define BN_S_MP_TOOM_MUL_C
#endif

#if defined(BN_MP_MUL_2_C)
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_MUL_2D_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_COPY_C
#   define BN_MP_GROW_C
#   define BN_MP_LSHD_C
#endif

#if defined(BN_MP_MUL_D_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_MULMOD_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_MOD_C
#   define BN_MP_MUL_C
#endif

#if defined(BN_MP_NEG_C)
#   define BN_MP_COPY_C
#endif

#if defined(BN_MP_OR_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_PACK_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_PACK_COUNT_C
#endif

#if defined(BN_MP_PACK_COUNT_C)
#   define BN_MP_COUNT_BITS_C
#endif

#if defined(BN_MP_PRIME_FERMAT_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_EXPTMOD_C
#   define BN_MP_INIT_C
#endif

#if defined(BN_MP_PRIME_FROBENIUS_UNDERWOOD_C)
#   define BN_MP_ADD_C
#   define BN_MP_ADD_D_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_EXCH_C
#   define BN_MP_GCD_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_KRONECKER_C
#   define BN_MP_MOD_C
#   define BN_MP_MUL_2_C
#   define BN_MP_MUL_C
#   define BN_MP_MUL_D_C
#   define BN_MP_SET_C
#   define BN_MP_SET_U32_C
#   define BN_MP_SQR_C
#   define BN_MP_SUB_C
#   define BN_MP_SUB_D_C
#   define BN_S_MP_GET_BIT_C
#endif

#if defined(BN_MP_PRIME_IS_PRIME_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_INIT_SET_C
#   define BN_MP_IS_SQUARE_C
#   define BN_MP_PRIME_MILLER_RABIN_C
#   define BN_MP_PRIME_STRONG_LUCAS_SELFRIDGE_C
#   define BN_MP_RAND_C
#   define BN_MP_READ_RADIX_C
#   define BN_MP_SET_C
#   define BN_S_MP_PRIME_IS_DIVISIBLE_C
#endif

#if defined(BN_MP_PRIME_MILLER_RABIN_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_CNT_LSB_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_EXPTMOD_C
#   define BN_MP_INIT_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_SQRMOD_C
#   define BN_MP_SUB_D_C
#endif

#if defined(BN_MP_PRIME_NEXT_PRIME_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_D_C
#   define BN_MP_INIT_C
#   define BN_MP_MOD_D_C
#   define BN_MP_PRIME_IS_PRIME_C
#   define BN_MP_SET_C
#   define BN_MP_SUB_D_C
#endif

#if defined(BN_MP_PRIME_RABIN_MILLER_TRIALS_C)
#endif

#if defined(BN_MP_PRIME_RAND_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_DIV_2_C
#   define BN_MP_FROM_UBIN_C
#   define BN_MP_MUL_2_C
#   define BN_MP_PRIME_IS_PRIME_C
#   define BN_MP_SUB_D_C
#   define BN_S_MP_PRIME_RANDOM_EX_C
#   define BN_S_MP_RAND_CB_C
#   define BN_S_MP_RAND_SOURCE_C
#endif

#if defined(BN_MP_PRIME_STRONG_LUCAS_SELFRIDGE_C)
#   define BN_MP_ADD_C
#   define BN_MP_ADD_D_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_CNT_LSB_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_DIV_2_C
#   define BN_MP_GCD_C
#   define BN_MP_INIT_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_KRONECKER_C
#   define BN_MP_MOD_C
#   define BN_MP_MUL_2_C
#   define BN_MP_MUL_C
#   define BN_MP_SET_C
#   define BN_MP_SET_I32_C
#   define BN_MP_SET_U32_C
#   define BN_MP_SQR_C
#   define BN_MP_SUB_C
#   define BN_MP_SUB_D_C
#   define BN_S_MP_GET_BIT_C
#   define BN_S_MP_MUL_SI_C
#endif

#if defined(BN_MP_RADIX_SIZE_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DIV_D_C
#   define BN_MP_INIT_COPY_C
#endif

#if defined(BN_MP_RADIX_SMAP_C)
#endif

#if defined(BN_MP_RAND_C)
#   define BN_MP_GROW_C
#   define BN_MP_RAND_SOURCE_C
#   define BN_MP_ZERO_C
#   define BN_S_MP_RAND_PLATFORM_C
#   define BN_S_MP_RAND_SOURCE_C
#endif

#if defined(BN_MP_READ_RADIX_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_MUL_D_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_REDUCE_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_LSHD_C
#   define BN_MP_MOD_2D_C
#   define BN_MP_MUL_C
#   define BN_MP_RSHD_C
#   define BN_MP_SET_C
#   define BN_MP_SUB_C
#   define BN_S_MP_MUL_DIGS_C
#   define BN_S_MP_MUL_HIGH_DIGS_C
#   define BN_S_MP_MUL_HIGH_DIGS_FAST_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_REDUCE_2K_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_INIT_C
#   define BN_MP_MUL_D_C
#   define BN_S_MP_ADD_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_REDUCE_2K_L_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_INIT_C
#   define BN_MP_MUL_C
#   define BN_S_MP_ADD_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_REDUCE_2K_SETUP_C)
#   define BN_MP_2EXPT_C
#   define BN_MP_CLEAR_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_INIT_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_REDUCE_2K_SETUP_L_C)
#   define BN_MP_2EXPT_C
#   define BN_MP_CLEAR_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_INIT_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_REDUCE_IS_2K_C)
#   define BN_MP_COUNT_BITS_C
#endif

#if defined(BN_MP_REDUCE_IS_2K_L_C)
#endif

#if defined(BN_MP_REDUCE_SETUP_C)
#   define BN_MP_2EXPT_C
#   define BN_MP_DIV_C
#endif

#if defined(BN_MP_ROOT_U32_C)
#   define BN_MP_2EXPT_C
#   define BN_MP_ADD_D_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_C
#   define BN_MP_COPY_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DIV_C
#   define BN_MP_EXCH_C
#   define BN_MP_EXPT_U32_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_MUL_C
#   define BN_MP_MUL_D_C
#   define BN_MP_SET_C
#   define BN_MP_SUB_C
#   define BN_MP_SUB_D_C
#endif

#if defined(BN_MP_RSHD_C)
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_SBIN_SIZE_C)
#   define BN_MP_UBIN_SIZE_C
#endif

#if defined(BN_MP_SET_C)
#endif

#if defined(BN_MP_SET_DOUBLE_C)
#   define BN_MP_DIV_2D_C
#   define BN_MP_MUL_2D_C
#   define BN_MP_SET_U64_C
#endif

#if defined(BN_MP_SET_I32_C)
#   define BN_MP_SET_U32_C
#endif

#if defined(BN_MP_SET_I64_C)
#   define BN_MP_SET_U64_C
#endif

#if defined(BN_MP_SET_L_C)
#   define BN_MP_SET_UL_C
#endif

#if defined(BN_MP_SET_LL_C)
#   define BN_MP_SET_ULL_C
#endif

#if defined(BN_MP_SET_U32_C)
#endif

#if defined(BN_MP_SET_U64_C)
#endif

#if defined(BN_MP_SET_UL_C)
#endif

#if defined(BN_MP_SET_ULL_C)
#endif

#if defined(BN_MP_SHRINK_C)
#endif

#if defined(BN_MP_SIGNED_RSH_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_SUB_D_C
#endif

#if defined(BN_MP_SQR_C)
#   define BN_S_MP_KARATSUBA_SQR_C
#   define BN_S_MP_SQR_C
#   define BN_S_MP_SQR_FAST_C
#   define BN_S_MP_TOOM_SQR_C
#endif

#if defined(BN_MP_SQRMOD_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_C
#   define BN_MP_MOD_C
#   define BN_MP_SQR_C
#endif

#if defined(BN_MP_SQRT_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_DIV_2_C
#   define BN_MP_DIV_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_RSHD_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_SQRTMOD_PRIME_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_D_C
#   define BN_MP_COPY_C
#   define BN_MP_DIV_2_C
#   define BN_MP_EXPTMOD_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_KRONECKER_C
#   define BN_MP_MOD_D_C
#   define BN_MP_MULMOD_C
#   define BN_MP_SET_C
#   define BN_MP_SET_U32_C
#   define BN_MP_SQRMOD_C
#   define BN_MP_SUB_D_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_SUB_C)
#   define BN_MP_CMP_MAG_C
#   define BN_S_MP_ADD_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_MP_SUB_D_C)
#   define BN_MP_ADD_D_C
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_SUBMOD_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_C
#   define BN_MP_MOD_C
#   define BN_MP_SUB_C
#endif

#if defined(BN_MP_TO_RADIX_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_DIV_D_C
#   define BN_MP_INIT_COPY_C
#   define BN_S_MP_REVERSE_C
#endif

#if defined(BN_MP_TO_SBIN_C)
#   define BN_MP_TO_UBIN_C
#endif

#if defined(BN_MP_TO_UBIN_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_DIV_2D_C
#   define BN_MP_INIT_COPY_C
#   define BN_MP_UBIN_SIZE_C
#endif

#if defined(BN_MP_UBIN_SIZE_C)
#   define BN_MP_COUNT_BITS_C
#endif

#if defined(BN_MP_UNPACK_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_MUL_2D_C
#   define BN_MP_ZERO_C
#endif

#if defined(BN_MP_XOR_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_MP_ZERO_C)
#endif

#if defined(BN_PRIME_TAB_C)
#endif

#if defined(BN_S_MP_ADD_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_S_MP_BALANCE_MUL_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_LSHD_C
#   define BN_MP_MUL_C
#endif

#if defined(BN_S_MP_EXPTMOD_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_COPY_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_C
#   define BN_MP_MOD_C
#   define BN_MP_MUL_C
#   define BN_MP_REDUCE_2K_L_C
#   define BN_MP_REDUCE_2K_SETUP_L_C
#   define BN_MP_REDUCE_C
#   define BN_MP_REDUCE_SETUP_C
#   define BN_MP_SET_C
#   define BN_MP_SQR_C
#endif

#if defined(BN_S_MP_EXPTMOD_FAST_C)
#   define BN_MP_CLEAR_C
#   define BN_MP_COPY_C
#   define BN_MP_COUNT_BITS_C
#   define BN_MP_DR_REDUCE_C
#   define BN_MP_DR_SETUP_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_MOD_C
#   define BN_MP_MONTGOMERY_CALC_NORMALIZATION_C
#   define BN_MP_MONTGOMERY_REDUCE_C
#   define BN_MP_MONTGOMERY_SETUP_C
#   define BN_MP_MULMOD_C
#   define BN_MP_MUL_C
#   define BN_MP_REDUCE_2K_C
#   define BN_MP_REDUCE_2K_SETUP_C
#   define BN_MP_SET_C
#   define BN_MP_SQR_C
#   define BN_S_MP_MONTGOMERY_REDUCE_FAST_C
#endif

#if defined(BN_S_MP_GET_BIT_C)
#endif

#if defined(BN_S_MP_INVMOD_FAST_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_COPY_C
#   define BN_MP_DIV_2_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_MOD_C
#   define BN_MP_SET_C
#   define BN_MP_SUB_C
#endif

#if defined(BN_S_MP_INVMOD_SLOW_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_CMP_C
#   define BN_MP_CMP_D_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_COPY_C
#   define BN_MP_DIV_2_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_MOD_C
#   define BN_MP_SET_C
#   define BN_MP_SUB_C
#endif

#if defined(BN_S_MP_KARATSUBA_MUL_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_LSHD_C
#   define BN_MP_MUL_C
#   define BN_S_MP_ADD_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_S_MP_KARATSUBA_SQR_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_LSHD_C
#   define BN_MP_SQR_C
#   define BN_S_MP_ADD_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_S_MP_MONTGOMERY_REDUCE_FAST_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CMP_MAG_C
#   define BN_MP_GROW_C
#   define BN_S_MP_SUB_C
#endif

#if defined(BN_S_MP_MUL_DIGS_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_SIZE_C
#   define BN_S_MP_MUL_DIGS_FAST_C
#endif

#if defined(BN_S_MP_MUL_DIGS_FAST_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_S_MP_MUL_HIGH_DIGS_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_SIZE_C
#   define BN_S_MP_MUL_HIGH_DIGS_FAST_C
#endif

#if defined(BN_S_MP_MUL_HIGH_DIGS_FAST_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_S_MP_PRIME_IS_DIVISIBLE_C)
#   define BN_MP_MOD_D_C
#endif

#if defined(BN_S_MP_RAND_JENKINS_C)
#   define BN_S_MP_RAND_JENKINS_INIT_C
#endif

#if defined(BN_S_MP_RAND_PLATFORM_C)
#endif

#if defined(BN_S_MP_REVERSE_C)
#endif

#if defined(BN_S_MP_SQR_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_EXCH_C
#   define BN_MP_INIT_SIZE_C
#endif

#if defined(BN_S_MP_SQR_FAST_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_S_MP_SUB_C)
#   define BN_MP_CLAMP_C
#   define BN_MP_GROW_C
#endif

#if defined(BN_S_MP_TOOM_MUL_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_DIV_2_C
#   define BN_MP_DIV_3_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_LSHD_C
#   define BN_MP_MUL_2_C
#   define BN_MP_MUL_C
#   define BN_MP_SUB_C
#endif

#if defined(BN_S_MP_TOOM_SQR_C)
#   define BN_MP_ADD_C
#   define BN_MP_CLAMP_C
#   define BN_MP_CLEAR_C
#   define BN_MP_DIV_2_C
#   define BN_MP_INIT_C
#   define BN_MP_INIT_SIZE_C
#   define BN_MP_LSHD_C
#   define BN_MP_MUL_2_C
#   define BN_MP_MUL_C
#   define BN_MP_SQR_C
#   define BN_MP_SUB_C
#endif

#ifdef LTM_INSIDE
#undef LTM_INSIDE
#ifdef LTM3
#   define LTM_LAST
#endif

#include "tommath_superclass.h"
#include "tommath_class.h"
#else
#   define LTM_LAST
#endif
