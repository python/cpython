/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* super class file for PK algos */

/* default ... include all MPI */
#ifndef LTM_NOTHING
#define LTM_ALL
#endif

/* RSA only (does not support DH/DSA/ECC) */
/* #define SC_RSA_1 */
/* #define SC_RSA_1_WITH_TESTS */

/* For reference.... On an Athlon64 optimizing for speed...

   LTM's mpi.o with all functions [striped] is 142KiB in size.

*/

#ifdef SC_RSA_1_WITH_TESTS
#   define BN_MP_ERROR_TO_STRING_C
#   define BN_MP_FREAD_C
#   define BN_MP_FWRITE_C
#   define BN_MP_INCR_C
#   define BN_MP_ISEVEN_C
#   define BN_MP_ISODD_C
#   define BN_MP_NEG_C
#   define BN_MP_PRIME_FROBENIUS_UNDERWOOD_C
#   define BN_MP_RADIX_SIZE_C
#   define BN_MP_RAND_C
#   define BN_MP_REDUCE_C
#   define BN_MP_REDUCE_2K_L_C
#   define BN_MP_FROM_SBIN_C
#   define BN_MP_ROOT_U32_C
#   define BN_MP_SET_L_C
#   define BN_MP_SET_UL_C
#   define BN_MP_SBIN_SIZE_C
#   define BN_MP_TO_RADIX_C
#   define BN_MP_TO_SBIN_C
#   define BN_S_MP_RAND_JENKINS_C
#   define BN_S_MP_RAND_PLATFORM_C
#endif

/* Works for RSA only, mpi.o is 68KiB */
#if defined(SC_RSA_1) || defined (SC_RSA_1_WITH_TESTS)
#   define BN_CUTOFFS_C
#   define BN_MP_ADDMOD_C
#   define BN_MP_CLEAR_MULTI_C
#   define BN_MP_EXPTMOD_C
#   define BN_MP_GCD_C
#   define BN_MP_INIT_MULTI_C
#   define BN_MP_INVMOD_C
#   define BN_MP_LCM_C
#   define BN_MP_MOD_C
#   define BN_MP_MOD_D_C
#   define BN_MP_MULMOD_C
#   define BN_MP_PRIME_IS_PRIME_C
#   define BN_MP_PRIME_RABIN_MILLER_TRIALS_C
#   define BN_MP_PRIME_RAND_C
#   define BN_MP_RADIX_SMAP_C
#   define BN_MP_SET_INT_C
#   define BN_MP_SHRINK_C
#   define BN_MP_TO_UNSIGNED_BIN_C
#   define BN_MP_UNSIGNED_BIN_SIZE_C
#   define BN_PRIME_TAB_C
#   define BN_S_MP_REVERSE_C

/* other modifiers */
#   define BN_MP_DIV_SMALL                    /* Slower division, not critical */


/* here we are on the last pass so we turn things off.  The functions classes are still there
 * but we remove them specifically from the build.  This also invokes tweaks in functions
 * like removing support for even moduli, etc...
 */
#   ifdef LTM_LAST
#      undef BN_MP_DR_IS_MODULUS_C
#      undef BN_MP_DR_SETUP_C
#      undef BN_MP_DR_REDUCE_C
#      undef BN_MP_DIV_3_C
#      undef BN_MP_REDUCE_2K_SETUP_C
#      undef BN_MP_REDUCE_2K_C
#      undef BN_MP_REDUCE_IS_2K_C
#      undef BN_MP_REDUCE_SETUP_C
#      undef BN_S_MP_BALANCE_MUL_C
#      undef BN_S_MP_EXPTMOD_C
#      undef BN_S_MP_INVMOD_FAST_C
#      undef BN_S_MP_KARATSUBA_MUL_C
#      undef BN_S_MP_KARATSUBA_SQR_C
#      undef BN_S_MP_MUL_HIGH_DIGS_C
#      undef BN_S_MP_MUL_HIGH_DIGS_FAST_C
#      undef BN_S_MP_TOOM_MUL_C
#      undef BN_S_MP_TOOM_SQR_C

#      ifndef SC_RSA_1_WITH_TESTS
#         undef BN_MP_REDUCE_C
#      endif

/* To safely undefine these you have to make sure your RSA key won't exceed the Comba threshold
 * which is roughly 255 digits [7140 bits for 32-bit machines, 15300 bits for 64-bit machines]
 * which means roughly speaking you can handle upto 2536-bit RSA keys with these defined without
 * trouble.
 */
#      undef BN_MP_MONTGOMERY_REDUCE_C
#      undef BN_S_MP_MUL_DIGS_C
#      undef BN_S_MP_SQR_C
#   endif

#endif
