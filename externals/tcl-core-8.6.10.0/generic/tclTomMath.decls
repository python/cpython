# tclTomMath.decls --
#
#	This file contains the declarations for the functions in 'libtommath'
#	that are contained within the Tcl library.  This file is used to
#	generate the 'tclTomMathDecls.h' and 'tclStubInit.c' files.
#
# If you edit this file, advance the revision number (and the epoch
# if the new stubs are not backward compatible) in tclTomMathDecls.h
#
# Copyright (c) 2005 by Kevin B. Kenny.  All rights reserved.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

library tcl

# Define the unsupported generic interfaces.

interface tclTomMath
# hooks {tclTomMathInt}
scspec EXTERN

# Declare each of the functions in the Tcl tommath interface

declare 0 {
    int TclBN_epoch(void)
}
declare 1 {
    int TclBN_revision(void)
}

declare 2 {
    mp_err TclBN_mp_add(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 3 {
    mp_err TclBN_mp_add_d(const mp_int *a, mp_digit b, mp_int *c)
}
declare 4 {
    mp_err TclBN_mp_and(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 5 {
    void TclBN_mp_clamp(mp_int *a)
}
declare 6 {
    void TclBN_mp_clear(mp_int *a)
}
declare 7 {
    void TclBN_mp_clear_multi(mp_int *a, ...)
}
declare 8 {
    mp_ord TclBN_mp_cmp(const mp_int *a, const mp_int *b)
}
declare 9 {
    mp_ord TclBN_mp_cmp_d(const mp_int *a, mp_digit b)
}
declare 10 {
    mp_ord TclBN_mp_cmp_mag(const mp_int *a, const mp_int *b)
}
declare 11 {
    mp_err TclBN_mp_copy(const mp_int *a, mp_int *b)
}
declare 12 {
    int TclBN_mp_count_bits(const mp_int *a)
}
declare 13 {
    mp_err TclBN_mp_div(const mp_int *a, const mp_int *b, mp_int *q, mp_int *r)
}
declare 14 {
    mp_err TclBN_mp_div_d(const mp_int *a, mp_digit b, mp_int *q, mp_digit *r)
}
declare 15 {
    mp_err TclBN_mp_div_2(const mp_int *a, mp_int *q)
}
declare 16 {
    mp_err TclBN_mp_div_2d(const mp_int *a, int b, mp_int *q, mp_int *r)
}
declare 17 {
    mp_err TclBN_mp_div_3(const mp_int *a, mp_int *q, mp_digit *r)
}
declare 18 {
    void TclBN_mp_exch(mp_int *a, mp_int *b)
}
declare 19 {
    mp_err TclBN_mp_expt_d(const mp_int *a, unsigned int b, mp_int *c)
}
declare 20 {
    mp_err TclBN_mp_grow(mp_int *a, int size)
}
declare 21 {
    mp_err TclBN_mp_init(mp_int *a)
}
declare 22 {
    mp_err TclBN_mp_init_copy(mp_int *a, const mp_int *b)
}
declare 23 {
    mp_err TclBN_mp_init_multi(mp_int *a, ...)
}
declare 24 {
    mp_err TclBN_mp_init_set(mp_int *a, mp_digit b)
}
declare 25 {
    mp_err TclBN_mp_init_size(mp_int *a, int size)
}
declare 26 {
    mp_err TclBN_mp_lshd(mp_int *a, int shift)
}
declare 27 {
    mp_err TclBN_mp_mod(const mp_int *a, const mp_int *b, mp_int *r)
}
declare 28 {
    mp_err TclBN_mp_mod_2d(const mp_int *a, int b, mp_int *r)
}
declare 29 {
    mp_err TclBN_mp_mul(const mp_int *a, const mp_int *b, mp_int *p)
}
declare 30 {
    mp_err TclBN_mp_mul_d(const mp_int *a, mp_digit b, mp_int *p)
}
declare 31 {
    mp_err TclBN_mp_mul_2(const mp_int *a, mp_int *p)
}
declare 32 {
    mp_err TclBN_mp_mul_2d(const mp_int *a, int d, mp_int *p)
}
declare 33 {
    mp_err TclBN_mp_neg(const mp_int *a, mp_int *b)
}
declare 34 {
    mp_err TclBN_mp_or(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 35 {
    mp_err TclBN_mp_radix_size(const mp_int *a, int radix, int *size)
}
declare 36 {
    mp_err TclBN_mp_read_radix(mp_int *a, const char *str, int radix)
}
declare 37 {
    void TclBN_mp_rshd(mp_int *a, int shift)
}
declare 38 {
    mp_err TclBN_mp_shrink(mp_int *a)
}
declare 39 {
    void TclBN_mp_set(mp_int *a, mp_digit b)
}
declare 40 {
    mp_err TclBN_mp_sqr(const mp_int *a, mp_int *b)
}
declare 41 {
    mp_err TclBN_mp_sqrt(const mp_int *a, mp_int *b)
}
declare 42 {
    mp_err TclBN_mp_sub(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 43 {
    mp_err TclBN_mp_sub_d(const mp_int *a, mp_digit b, mp_int *c)
}
declare 44 {
    mp_err TclBN_mp_to_unsigned_bin(const mp_int *a, unsigned char *b)
}
declare 45 {
    mp_err TclBN_mp_to_unsigned_bin_n(const mp_int *a, unsigned char *b,
	    unsigned long *outlen)
}
declare 46 {
    mp_err TclBN_mp_toradix_n(const mp_int *a, char *str, int radix, int maxlen)
}
declare 47 {
    size_t TclBN_mp_unsigned_bin_size(const mp_int *a)
}
declare 48 {
    mp_err TclBN_mp_xor(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 49 {
    void TclBN_mp_zero(mp_int *a)
}

# internal routines to libtommath - should not be called but must be
# exported to accommodate the "tommath" extension

declare 50 {
    void TclBN_reverse(unsigned char *s, int len)
}
declare 51 {
    mp_err TclBN_fast_s_mp_mul_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs)
}
declare 52 {
    mp_err TclBN_fast_s_mp_sqr(const mp_int *a, mp_int *b)
}
declare 53 {
    mp_err TclBN_mp_karatsuba_mul(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 54 {
    mp_err TclBN_mp_karatsuba_sqr(const mp_int *a, mp_int *b)
}
declare 55 {
    mp_err TclBN_mp_toom_mul(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 56 {
    mp_err TclBN_mp_toom_sqr(const mp_int *a, mp_int *b)
}
declare 57 {
    mp_err TclBN_s_mp_add(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 58 {
    mp_err TclBN_s_mp_mul_digs(const mp_int *a, const mp_int *b, mp_int *c, int digs)
}
declare 59 {
    mp_err TclBN_s_mp_sqr(const mp_int *a, mp_int *b)
}
declare 60 {
    mp_err TclBN_s_mp_sub(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 61 {
    mp_err TclBN_mp_init_set_int(mp_int *a, unsigned long i)
}
declare 62 {
    mp_err TclBN_mp_set_int(mp_int *a, unsigned long i)
}
declare 63 {
    int TclBN_mp_cnt_lsb(const mp_int *a)
}

# Formerly internal API to allow initialisation of bignums without knowing the
# typedefs of how a bignum works internally.
declare 64 {
    int TclBNInitBignumFromLong(mp_int *bignum, long initVal)
}
declare 65 {
    int TclBNInitBignumFromWideInt(mp_int *bignum, Tcl_WideInt initVal)
}
declare 66 {
    int TclBNInitBignumFromWideUInt(mp_int *bignum, Tcl_WideUInt initVal)
}

# Added in libtommath 1.0
declare 67 {
    mp_err TclBN_mp_expt_d_ex(const mp_int *a, mp_digit b, mp_int *c, int fast)
}
# Added in libtommath 1.0.1
declare 68 {
    void TclBN_mp_set_ull(mp_int *a, Tcl_WideUInt i)
}
# Added in libtommath 1.1.0
declare 73 {
    mp_err TclBN_mp_tc_and(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 74 {
    mp_err TclBN_mp_tc_or(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 75 {
    mp_err TclBN_mp_tc_xor(const mp_int *a, const mp_int *b, mp_int *c)
}
declare 76 {
    mp_err TclBN_mp_signed_rsh(const mp_int *a, int b, mp_int *c)
}

# Added in libtommath 1.2.0
declare 78 {
    int TclBN_mp_to_ubin(const mp_int *a, unsigned char *buf, size_t maxlen, size_t *written)
}
declare 80 {
    int TclBN_mp_to_radix(const mp_int *a, char *str, size_t maxlen, size_t *written, int radix)
}


# Local Variables:
# mode: tcl
# End:
