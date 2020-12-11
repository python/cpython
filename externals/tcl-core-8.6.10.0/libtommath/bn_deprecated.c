#include "tommath_private.h"
#ifdef BN_DEPRECATED_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#ifdef BN_MP_GET_BIT_C
int mp_get_bit(const mp_int *a, int b)
{
   if (b < 0) {
      return MP_VAL;
   }
   return (s_mp_get_bit(a, (unsigned int)b) == MP_YES) ? MP_YES : MP_NO;
}
#endif
#ifdef BN_MP_JACOBI_C
mp_err mp_jacobi(const mp_int *a, const mp_int *n, int *c)
{
   if (a->sign == MP_NEG) {
      return MP_VAL;
   }
   if (mp_cmp_d(n, 0uL) != MP_GT) {
      return MP_VAL;
   }
   return mp_kronecker(a, n, c);
}
#endif
#ifdef BN_MP_PRIME_RANDOM_EX_C
mp_err mp_prime_random_ex(mp_int *a, int t, int size, int flags, private_mp_prime_callback cb, void *dat)
{
   return s_mp_prime_random_ex(a, t, size, flags, cb, dat);
}
#endif
#ifdef BN_MP_RAND_DIGIT_C
mp_err mp_rand_digit(mp_digit *r)
{
   mp_err err = s_mp_rand_source(r, sizeof(mp_digit));
   *r &= MP_MASK;
   return err;
}
#endif
#ifdef BN_FAST_MP_INVMOD_C
mp_err fast_mp_invmod(const mp_int *a, const mp_int *b, mp_int *c)
{
   return s_mp_invmod_fast(a, b, c);
}
#endif
#ifdef BN_FAST_MP_MONTGOMERY_REDUCE_C
mp_err fast_mp_montgomery_reduce(mp_int *x, const mp_int *n, mp_digit rho)
{
   return s_mp_montgomery_reduce_fast(x, n, rho);
}
#endif
#ifdef BN_FAST_S_MP_MUL_DIGS_C
mp_err fast_s_mp_mul_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs)
{
   return s_mp_mul_digs_fast(a, b, c, digs);
}
#endif
#ifdef BN_FAST_S_MP_MUL_HIGH_DIGS_C
mp_err fast_s_mp_mul_high_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs)
{
   return s_mp_mul_high_digs_fast(a, b, c, digs);
}
#endif
#ifdef BN_FAST_S_MP_SQR_C
mp_err fast_s_mp_sqr(const mp_int *a, mp_int *b)
{
   return s_mp_sqr_fast(a, b);
}
#endif
#ifdef BN_MP_BALANCE_MUL_C
mp_err mp_balance_mul(const mp_int *a, const mp_int *b, mp_int *c)
{
   return s_mp_balance_mul(a, b, c);
}
#endif
#ifdef BN_MP_EXPTMOD_FAST_C
mp_err mp_exptmod_fast(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y, int redmode)
{
   return s_mp_exptmod_fast(G, X, P, Y, redmode);
}
#endif
#ifdef BN_MP_INVMOD_SLOW_C
mp_err mp_invmod_slow(const mp_int *a, const mp_int *b, mp_int *c)
{
   return s_mp_invmod_slow(a, b, c);
}
#endif
#ifdef BN_MP_KARATSUBA_MUL_C
mp_err mp_karatsuba_mul(const mp_int *a, const mp_int *b, mp_int *c)
{
   return s_mp_karatsuba_mul(a, b, c);
}
#endif
#ifdef BN_MP_KARATSUBA_SQR_C
mp_err mp_karatsuba_sqr(const mp_int *a, mp_int *b)
{
   return s_mp_karatsuba_sqr(a, b);
}
#endif
#ifdef BN_MP_TOOM_MUL_C
mp_err mp_toom_mul(const mp_int *a, const mp_int *b, mp_int *c)
{
   return s_mp_toom_mul(a, b, c);
}
#endif
#ifdef BN_MP_TOOM_SQR_C
mp_err mp_toom_sqr(const mp_int *a, mp_int *b)
{
   return s_mp_toom_sqr(a, b);
}
#endif
#ifdef S_MP_REVERSE_C
void bn_reverse(unsigned char *s, int len)
{
   if (len > 0) {
      s_mp_reverse(s, (size_t)len);
   }
}
#endif
#ifdef BN_MP_TC_AND_C
mp_err mp_tc_and(const mp_int *a, const mp_int *b, mp_int *c)
{
   return mp_and(a, b, c);
}
#endif
#ifdef BN_MP_TC_OR_C
mp_err mp_tc_or(const mp_int *a, const mp_int *b, mp_int *c)
{
   return mp_or(a, b, c);
}
#endif
#ifdef BN_MP_TC_XOR_C
mp_err mp_tc_xor(const mp_int *a, const mp_int *b, mp_int *c)
{
   return mp_xor(a, b, c);
}
#endif
#ifdef BN_MP_TC_DIV_2D_C
mp_err mp_tc_div_2d(const mp_int *a, int b, mp_int *c)
{
   return mp_signed_rsh(a, b, c);
}
#endif
#ifdef BN_MP_INIT_SET_INT_C
mp_err mp_init_set_int(mp_int *a, unsigned long b)
{
   return mp_init_u32(a, (uint32_t)b);
}
#endif
#ifdef BN_MP_SET_INT_C
mp_err mp_set_int(mp_int *a, unsigned long b)
{
   mp_set_u32(a, (uint32_t)b);
   return MP_OKAY;
}
#endif
#ifdef BN_MP_SET_LONG_C
mp_err mp_set_long(mp_int *a, unsigned long b)
{
   mp_set_u64(a, b);
   return MP_OKAY;
}
#endif
#ifdef BN_MP_SET_LONG_LONG_C
mp_err mp_set_long_long(mp_int *a, unsigned long long b)
{
   mp_set_u64(a, b);
   return MP_OKAY;
}
#endif
#ifdef BN_MP_GET_INT_C
unsigned long mp_get_int(const mp_int *a)
{
   return (unsigned long)mp_get_mag_u32(a);
}
#endif
#ifdef BN_MP_GET_LONG_C
unsigned long mp_get_long(const mp_int *a)
{
   return (unsigned long)mp_get_mag_ul(a);
}
#endif
#ifdef BN_MP_GET_LONG_LONG_C
unsigned long long mp_get_long_long(const mp_int *a)
{
   return mp_get_mag_ull(a);
}
#endif
#ifdef BN_MP_PRIME_IS_DIVISIBLE_C
mp_err mp_prime_is_divisible(const mp_int *a, mp_bool *result)
{
   return s_mp_prime_is_divisible(a, result);
}
#endif
#ifdef BN_MP_EXPT_D_EX_C
mp_err mp_expt_d_ex(const mp_int *a, mp_digit b, mp_int *c, int fast)
{
   (void)fast;
   if (b > MP_MIN(MP_DIGIT_MAX, UINT32_MAX)) {
      return MP_VAL;
   }
   return mp_expt_u32(a, (uint32_t)b, c);
}
#endif
#ifdef BN_MP_EXPT_D_C
mp_err mp_expt_d(const mp_int *a, mp_digit b, mp_int *c)
{
   if (b > MP_MIN(MP_DIGIT_MAX, UINT32_MAX)) {
      return MP_VAL;
   }
   return mp_expt_u32(a, (uint32_t)b, c);
}
#endif
#ifdef BN_MP_N_ROOT_EX_C
mp_err mp_n_root_ex(const mp_int *a, mp_digit b, mp_int *c, int fast)
{
   (void)fast;
   if (b > MP_MIN(MP_DIGIT_MAX, UINT32_MAX)) {
      return MP_VAL;
   }
   return mp_root_u32(a, (unsigned int)b, c);
}
#endif
#ifdef BN_MP_N_ROOT_C
mp_err mp_n_root(const mp_int *a, mp_digit b, mp_int *c)
{
   if (b > MP_MIN(MP_DIGIT_MAX, UINT32_MAX)) {
      return MP_VAL;
   }
   return mp_root_u32(a, (unsigned int)b, c);
}
#endif
#ifdef BN_MP_UNSIGNED_BIN_SIZE_C
int mp_unsigned_bin_size(const mp_int *a)
{
   return (int)mp_ubin_size(a);
}
#endif
#ifdef BN_MP_READ_UNSIGNED_BIN_C
mp_err mp_read_unsigned_bin(mp_int *a, const unsigned char *b, int c)
{
   return mp_from_ubin(a, b, (size_t) c);
}
#endif
#ifdef BN_MP_TO_UNSIGNED_BIN_C
mp_err mp_to_unsigned_bin(const mp_int *a, unsigned char *b)
{
   return mp_to_ubin(a, b, SIZE_MAX, NULL);
}
#endif
#ifdef BN_MP_TO_UNSIGNED_BIN_N_C
mp_err mp_to_unsigned_bin_n(const mp_int *a, unsigned char *b, unsigned long *outlen)
{
   size_t n = mp_ubin_size(a);
   if (*outlen < (unsigned long)n) {
      return MP_VAL;
   }
   *outlen = (unsigned long)n;
   return mp_to_ubin(a, b, n, NULL);
}
#endif
#ifdef BN_MP_SIGNED_BIN_SIZE_C
int mp_signed_bin_size(const mp_int *a)
{
   return (int)mp_sbin_size(a);
}
#endif
#ifdef BN_MP_READ_SIGNED_BIN_C
mp_err mp_read_signed_bin(mp_int *a, const unsigned char *b, int c)
{
   return mp_from_sbin(a, b, (size_t) c);
}
#endif
#ifdef BN_MP_TO_SIGNED_BIN_C
mp_err mp_to_signed_bin(const mp_int *a, unsigned char *b)
{
   return mp_to_sbin(a, b, SIZE_MAX, NULL);
}
#endif
#ifdef BN_MP_TO_SIGNED_BIN_N_C
mp_err mp_to_signed_bin_n(const mp_int *a, unsigned char *b, unsigned long *outlen)
{
   size_t n = mp_sbin_size(a);
   if (*outlen < (unsigned long)n) {
      return MP_VAL;
   }
   *outlen = (unsigned long)n;
   return mp_to_sbin(a, b, n, NULL);
}
#endif
#ifdef BN_MP_TORADIX_N_C
mp_err mp_toradix_n(const mp_int *a, char *str, int radix, int maxlen)
{
   if (maxlen < 0) {
      return MP_VAL;
   }
   return mp_to_radix(a, str, (size_t)maxlen, NULL, radix);
}
#endif
#ifdef BN_MP_TORADIX_C
mp_err mp_toradix(const mp_int *a, char *str, int radix)
{
   return mp_to_radix(a, str, SIZE_MAX, NULL, radix);
}
#endif
#ifdef BN_MP_IMPORT_C
mp_err mp_import(mp_int *rop, size_t count, int order, size_t size, int endian, size_t nails,
                 const void *op)
{
   return mp_unpack(rop, count, order, size, endian, nails, op);
}
#endif
#ifdef BN_MP_EXPORT_C
mp_err mp_export(void *rop, size_t *countp, int order, size_t size,
                 int endian, size_t nails, const mp_int *op)
{
   return mp_pack(rop, SIZE_MAX, countp, order, size, endian, nails, op);
}
#endif
#endif
