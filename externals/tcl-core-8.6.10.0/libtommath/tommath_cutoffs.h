/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */
/*
   Current values evaluated on an AMD A8-6600K (64-bit).
   Type "make tune" to optimize them for your machine but
   be aware that it may take a long time. It took 2:30 minutes
   on the aforementioned machine for example.
 */

#define MP_DEFAULT_KARATSUBA_MUL_CUTOFF 80
#define MP_DEFAULT_KARATSUBA_SQR_CUTOFF 120
#define MP_DEFAULT_TOOM_MUL_CUTOFF      350
#define MP_DEFAULT_TOOM_SQR_CUTOFF      400
