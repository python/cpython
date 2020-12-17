#include "tommath_private.h"
#ifdef BN_S_MP_EXPTMOD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#ifdef MP_LOW_MEM
#   define TAB_SIZE 32
#   define MAX_WINSIZE 5
#else
#   define TAB_SIZE 256
#   define MAX_WINSIZE 0
#endif

mp_err s_mp_exptmod(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y, int redmode)
{
   mp_int  M[TAB_SIZE], res, mu;
   mp_digit buf;
   mp_err   err;
   int      bitbuf, bitcpy, bitcnt, mode, digidx, x, y, winsize;
   mp_err(*redux)(mp_int *x, const mp_int *m, const mp_int *mu);

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
   if ((err = mp_init(&M[1])) != MP_OKAY) {
      return err;
   }

   /* now init the second half of the array */
   for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
      if ((err = mp_init(&M[x])) != MP_OKAY) {
         for (y = 1<<(winsize-1); y < x; y++) {
            mp_clear(&M[y]);
         }
         mp_clear(&M[1]);
         return err;
      }
   }

   /* create mu, used for Barrett reduction */
   if ((err = mp_init(&mu)) != MP_OKAY)                           goto LBL_M;

   if (redmode == 0) {
      if ((err = mp_reduce_setup(&mu, P)) != MP_OKAY)             goto LBL_MU;
      redux = mp_reduce;
   } else {
      if ((err = mp_reduce_2k_setup_l(P, &mu)) != MP_OKAY)        goto LBL_MU;
      redux = mp_reduce_2k_l;
   }

   /* create M table
    *
    * The M table contains powers of the base,
    * e.g. M[x] = G**x mod P
    *
    * The first half of the table is not
    * computed though accept for M[0] and M[1]
    */
   if ((err = mp_mod(G, P, &M[1])) != MP_OKAY)                    goto LBL_MU;

   /* compute the value at M[1<<(winsize-1)] by squaring
    * M[1] (winsize-1) times
    */
   if ((err = mp_copy(&M[1], &M[(size_t)1 << (winsize - 1)])) != MP_OKAY) goto LBL_MU;

   for (x = 0; x < (winsize - 1); x++) {
      /* square it */
      if ((err = mp_sqr(&M[(size_t)1 << (winsize - 1)],
                        &M[(size_t)1 << (winsize - 1)])) != MP_OKAY) goto LBL_MU;

      /* reduce modulo P */
      if ((err = redux(&M[(size_t)1 << (winsize - 1)], P, &mu)) != MP_OKAY) goto LBL_MU;
   }

   /* create upper table, that is M[x] = M[x-1] * M[1] (mod P)
    * for x = (2**(winsize - 1) + 1) to (2**winsize - 1)
    */
   for (x = (1 << (winsize - 1)) + 1; x < (1 << winsize); x++) {
      if ((err = mp_mul(&M[x - 1], &M[1], &M[x])) != MP_OKAY)     goto LBL_MU;
      if ((err = redux(&M[x], P, &mu)) != MP_OKAY)                goto LBL_MU;
   }

   /* setup result */
   if ((err = mp_init(&res)) != MP_OKAY)                          goto LBL_MU;
   mp_set(&res, 1uL);

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
         /* if digidx == -1 we are out of digits */
         if (digidx == -1) {
            break;
         }
         /* read next digit and reset the bitcnt */
         buf    = X->dp[digidx--];
         bitcnt = (int)MP_DIGIT_BIT;
      }

      /* grab the next msb from the exponent */
      y     = (buf >> (mp_digit)(MP_DIGIT_BIT - 1)) & 1uL;
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
         if ((err = redux(&res, P, &mu)) != MP_OKAY)              goto LBL_RES;
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
            if ((err = redux(&res, P, &mu)) != MP_OKAY)           goto LBL_RES;
         }

         /* then multiply */
         if ((err = mp_mul(&res, &M[bitbuf], &res)) != MP_OKAY)  goto LBL_RES;
         if ((err = redux(&res, P, &mu)) != MP_OKAY)             goto LBL_RES;

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
         if ((err = redux(&res, P, &mu)) != MP_OKAY)              goto LBL_RES;

         bitbuf <<= 1;
         if ((bitbuf & (1 << winsize)) != 0) {
            /* then multiply */
            if ((err = mp_mul(&res, &M[1], &res)) != MP_OKAY)     goto LBL_RES;
            if ((err = redux(&res, P, &mu)) != MP_OKAY)           goto LBL_RES;
         }
      }
   }

   mp_exch(&res, Y);
   err = MP_OKAY;
LBL_RES:
   mp_clear(&res);
LBL_MU:
   mp_clear(&mu);
LBL_M:
   mp_clear(&M[1]);
   for (x = 1<<(winsize-1); x < (1 << winsize); x++) {
      mp_clear(&M[x]);
   }
   return err;
}
#endif
