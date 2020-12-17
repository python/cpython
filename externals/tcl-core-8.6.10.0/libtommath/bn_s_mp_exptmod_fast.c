#include "tommath_private.h"
#ifdef BN_S_MP_EXPTMOD_FAST_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* computes Y == G**X mod P, HAC pp.616, Algorithm 14.85
 *
 * Uses a left-to-right k-ary sliding window to compute the modular exponentiation.
 * The value of k changes based on the size of the exponent.
 *
 * Uses Montgomery or Diminished Radix reduction [whichever appropriate]
 */

#ifdef MP_LOW_MEM
#   define TAB_SIZE 32
#   define MAX_WINSIZE 5
#else
#   define TAB_SIZE 256
#   define MAX_WINSIZE 0
#endif

mp_err s_mp_exptmod_fast(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y, int redmode)
{
   mp_int  M[TAB_SIZE], res;
   mp_digit buf, mp;
   int     bitbuf, bitcpy, bitcnt, mode, digidx, x, y, winsize;
   mp_err   err;

   /* use a pointer to the reduction algorithm.  This allows us to use
    * one of many reduction algorithms without modding the guts of
    * the code with if statements everywhere.
    */
   mp_err(*redux)(mp_int *x, const mp_int *n, mp_digit rho);

   /* find window size */
   x = mp_count_bits(X);
   if (x <= 7) {
      winsize = 2;
   } else if (x <= 36) {
      winsize = 3;
   } else if (x <= 140) {
      winsize = 4;
   } else if (x <= 450) {
      winsize = 5;
   } else if (x <= 1303) {
      winsize = 6;
   } else if (x <= 3529) {
      winsize = 7;
   } else {
      winsize = 8;
   }

   winsize = MAX_WINSIZE ? MP_MIN(MAX_WINSIZE, winsize) : winsize;

   /* init M array */
   /* init first cell */
   if ((err = mp_init_size(&M[1], P->alloc)) != MP_OKAY) {
      return err;
   }

   /* now init the second half of the array */
   for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
      if ((err = mp_init_size(&M[x], P->alloc)) != MP_OKAY) {
         for (y = 1<<(winsize-1); y < x; y++) {
            mp_clear(&M[y]);
         }
         mp_clear(&M[1]);
         return err;
      }
   }

   /* determine and setup reduction code */
   if (redmode == 0) {
      if (MP_HAS(MP_MONTGOMERY_SETUP)) {
         /* now setup montgomery  */
         if ((err = mp_montgomery_setup(P, &mp)) != MP_OKAY)      goto LBL_M;
      } else {
         err = MP_VAL;
         goto LBL_M;
      }

      /* automatically pick the comba one if available (saves quite a few calls/ifs) */
      if (MP_HAS(S_MP_MONTGOMERY_REDUCE_FAST) &&
          (((P->used * 2) + 1) < MP_WARRAY) &&
          (P->used < MP_MAXFAST)) {
         redux = s_mp_montgomery_reduce_fast;
      } else if (MP_HAS(MP_MONTGOMERY_REDUCE)) {
         /* use slower baseline Montgomery method */
         redux = mp_montgomery_reduce;
      } else {
         err = MP_VAL;
         goto LBL_M;
      }
   } else if (redmode == 1) {
      if (MP_HAS(MP_DR_SETUP) && MP_HAS(MP_DR_REDUCE)) {
         /* setup DR reduction for moduli of the form B**k - b */
         mp_dr_setup(P, &mp);
         redux = mp_dr_reduce;
      } else {
         err = MP_VAL;
         goto LBL_M;
      }
   } else if (MP_HAS(MP_REDUCE_2K_SETUP) && MP_HAS(MP_REDUCE_2K)) {
      /* setup DR reduction for moduli of the form 2**k - b */
      if ((err = mp_reduce_2k_setup(P, &mp)) != MP_OKAY)          goto LBL_M;
      redux = mp_reduce_2k;
   } else {
      err = MP_VAL;
      goto LBL_M;
   }

   /* setup result */
   if ((err = mp_init_size(&res, P->alloc)) != MP_OKAY)           goto LBL_M;

   /* create M table
    *

    *
    * The first half of the table is not computed though accept for M[0] and M[1]
    */

   if (redmode == 0) {
      if (MP_HAS(MP_MONTGOMERY_CALC_NORMALIZATION)) {
         /* now we need R mod m */
         if ((err = mp_montgomery_calc_normalization(&res, P)) != MP_OKAY) goto LBL_RES;

         /* now set M[1] to G * R mod m */
         if ((err = mp_mulmod(G, &res, P, &M[1])) != MP_OKAY)     goto LBL_RES;
      } else {
         err = MP_VAL;
         goto LBL_RES;
      }
   } else {
      mp_set(&res, 1uL);
      if ((err = mp_mod(G, P, &M[1])) != MP_OKAY)                 goto LBL_RES;
   }

   /* compute the value at M[1<<(winsize-1)] by squaring M[1] (winsize-1) times */
   if ((err = mp_copy(&M[1], &M[(size_t)1 << (winsize - 1)])) != MP_OKAY) goto LBL_RES;

   for (x = 0; x < (winsize - 1); x++) {
      if ((err = mp_sqr(&M[(size_t)1 << (winsize - 1)], &M[(size_t)1 << (winsize - 1)])) != MP_OKAY) goto LBL_RES;
      if ((err = redux(&M[(size_t)1 << (winsize - 1)], P, mp)) != MP_OKAY) goto LBL_RES;
   }

   /* create upper table */
   for (x = (1 << (winsize - 1)) + 1; x < (1 << winsize); x++) {
      if ((err = mp_mul(&M[x - 1], &M[1], &M[x])) != MP_OKAY)     goto LBL_RES;
      if ((err = redux(&M[x], P, mp)) != MP_OKAY)                 goto LBL_RES;
   }

   /* set initial mode and bit cnt */
   mode   = 0;
   bitcnt = 1;
   buf    = 0;
   digidx = X->used - 1;
   bitcpy = 0;
   bitbuf = 0;

   for (;;) {
      /* grab next digit as required */
      if (--bitcnt == 0) {
         /* if digidx == -1 we are out of digits so break */
         if (digidx == -1) {
            break;
         }
         /* read next digit and reset bitcnt */
         buf    = X->dp[digidx--];
         bitcnt = (int)MP_DIGIT_BIT;
      }

      /* grab the next msb from the exponent */
      y     = (mp_digit)(buf >> (MP_DIGIT_BIT - 1)) & 1uL;
      buf <<= (mp_digit)1;

      /* if the bit is zero and mode == 0 then we ignore it
       * These represent the leading zero bits before the first 1 bit
       * in the exponent.  Technically this opt is not required but it
       * does lower the # of trivial squaring/reductions used
       */
      if ((mode == 0) && (y == 0)) {
         continue;
      }

      /* if the bit is zero and mode == 1 then we square */
      if ((mode == 1) && (y == 0)) {
         if ((err = mp_sqr(&res, &res)) != MP_OKAY)               goto LBL_RES;
         if ((err = redux(&res, P, mp)) != MP_OKAY)               goto LBL_RES;
         continue;
      }

      /* else we add it to the window */
      bitbuf |= (y << (winsize - ++bitcpy));
      mode    = 2;

      if (bitcpy == winsize) {
         /* ok window is filled so square as required and multiply  */
         /* square first */
         for (x = 0; x < winsize; x++) {
            if ((err = mp_sqr(&res, &res)) != MP_OKAY)            goto LBL_RES;
            if ((err = redux(&res, P, mp)) != MP_OKAY)            goto LBL_RES;
         }

         /* then multiply */
         if ((err = mp_mul(&res, &M[bitbuf], &res)) != MP_OKAY)   goto LBL_RES;
         if ((err = redux(&res, P, mp)) != MP_OKAY)               goto LBL_RES;

         /* empty window and reset */
         bitcpy = 0;
         bitbuf = 0;
         mode   = 1;
      }
   }

   /* if bits remain then square/multiply */
   if ((mode == 2) && (bitcpy > 0)) {
      /* square then multiply if the bit is set */
      for (x = 0; x < bitcpy; x++) {
         if ((err = mp_sqr(&res, &res)) != MP_OKAY)               goto LBL_RES;
         if ((err = redux(&res, P, mp)) != MP_OKAY)               goto LBL_RES;

         /* get next bit of the window */
         bitbuf <<= 1;
         if ((bitbuf & (1 << winsize)) != 0) {
            /* then multiply */
            if ((err = mp_mul(&res, &M[1], &res)) != MP_OKAY)     goto LBL_RES;
            if ((err = redux(&res, P, mp)) != MP_OKAY)            goto LBL_RES;
         }
      }
   }

   if (redmode == 0) {
      /* fixup result if Montgomery reduction is used
       * recall that any value in a Montgomery system is
       * actually multiplied by R mod n.  So we have
       * to reduce one more time to cancel out the factor
       * of R.
       */
      if ((err = redux(&res, P, mp)) != MP_OKAY)                  goto LBL_RES;
   }

   /* swap res with Y */
   mp_exch(&res, Y);
   err = MP_OKAY;
LBL_RES:
   mp_clear(&res);
LBL_M:
   mp_clear(&M[1]);
   for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
      mp_clear(&M[x]);
   }
   return err;
}
#endif
