/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991, 2000, 2001 by Lucent Technologies.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/****************************************************************
 * This is dtoa.c by David M. Gay, downloaded from
 * http://www.netlib.org/fp/dtoa.c on April 15, 2009 and modified for
 * inclusion into the Python core by Mark E. T. Dickinson and Eric V. Smith.
 * The major modifications are as follows:
 *
 *  0. The original code has been specialized to Python's needs by removing
 *     many of the #ifdef'd sections.  In particular, code to support VAX and
 *     IBM floating-point formats, hex NaNs, hex floats, locale-aware
 *     treatment of the decimal point, and setting of the inexact flag have
 *     been removed.
 *
 *  1. We use PyMem_Malloc and PyMem_Free in place of malloc and free.
 *
 *  2. The public functions strtod, dtoa and freedtoa all now have
 *     a _Py_dg_ prefix.
 *
 *  3. Instead of assuming that PyMem_Malloc always succeeds, we thread
 *     PyMem_Malloc failures through the code.  The functions
 *
 *       Balloc, multadd, s2b, i2b, mult, pow5mult, lshift, diff, d2b
 *
 *     of return type *Bigint all return NULL to indicate a malloc failure.
 *     Similarly, rv_alloc and nrv_alloc (return type char *) return NULL on
 *     failure.  bigcomp now has return type int (it used to be void) and
 *     returns -1 on failure and 0 otherwise.  _Py_dg_dtoa returns NULL
 *     on failure.  _Py_dg_strtod indicates failure due to malloc failure
 *     by returning -1.0, setting errno=ENOMEM and *se to s00.
 *
 *  4. The static variable dtoa_result has been removed.  Callers of
 *     _Py_dg_dtoa are expected to call _Py_dg_freedtoa to free
 *     the memory allocated by _Py_dg_dtoa.
 *
 *  5. The code has been reformatted to better fit with Python's
 *     C style guide (PEP 7).
 *
 ***************************************************************/

/* Please send bug reports for the original dtoa.c code to David M. Gay (dmg
 * at acm dot org, with " at " changed at "@" and " dot " changed to ".").
 * Please report bugs for this modified version using the Python issue tracker
 * (http://bugs.python.org). */

/* On a machine with IEEE extended-precision registers, it is
 * necessary to specify double-precision (53-bit) rounding precision
 * before invoking strtod or dtoa.  If the machine uses (the equivalent
 * of) Intel 80x87 arithmetic, the call
 *      _control87(PC_53, MCW_PC);
 * does this with many compilers.  Whether this or another call is
 * appropriate depends on the compiler; for this to work, it may be
 * necessary to #include "float.h" or another system-dependent header
 * file.
 */

/* strtod for IEEE-, VAX-, and IBM-arithmetic machines.
 *
 * This strtod returns a nearest machine number to the input decimal
 * string (or sets errno to ERANGE).  With IEEE arithmetic, ties are
 * broken by the IEEE round-even rule.  Otherwise ties are broken by
 * biased rounding (add half and chop).
 *
 * Inspired loosely by William D. Clinger's paper "How to Read Floating
 * Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
 *
 * Modifications:
 *
 *      1. We only require IEEE, IBM, or VAX double-precision
 *              arithmetic (not IEEE double-extended).
 *      2. We get by with floating-point arithmetic in a case that
 *              Clinger missed -- when we're computing d * 10^n
 *              for a small integer d and the integer n is not too
 *              much larger than 22 (the maximum integer k for which
 *              we can represent 10^k exactly), we may be able to
 *              compute (d*10^k) * 10^(e-k) with just one roundoff.
 *      3. Rather than a bit-at-a-time adjustment of the binary
 *              result in the hard case, we use floating-point
 *              arithmetic to determine the adjustment to within
 *              one bit; only in really hard cases do we need to
 *              compute a second residual.
 *      4. Because of 3., we don't need a large table of powers of 10
 *              for ten-to-e (just some small tables, e.g. of 10^k
 *              for 0 <= k <= 22).
 */

/* Linking of Python's #defines to Gay's #defines starts here. */

#include "Python.h"

/* if PY_NO_SHORT_FLOAT_REPR is defined, then don't even try to compile
   the following code */
#ifndef PY_NO_SHORT_FLOAT_REPR

#include "float.h"

#define MALLOC PyMem_Malloc
#define FREE PyMem_Free

/* This code should also work for ARM mixed-endian format on little-endian
   machines, where doubles have byte order 45670123 (in increasing address
   order, 0 being the least significant byte). */
#ifdef DOUBLE_IS_LITTLE_ENDIAN_IEEE754
#  define IEEE_8087
#endif
#if defined(DOUBLE_IS_BIG_ENDIAN_IEEE754) ||  \
  defined(DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754)
#  define IEEE_MC68k
#endif
#if defined(IEEE_8087) + defined(IEEE_MC68k) != 1
#error "Exactly one of IEEE_8087 or IEEE_MC68k should be defined."
#endif

/* The code below assumes that the endianness of integers matches the
   endianness of the two 32-bit words of a double.  Check this. */
#if defined(WORDS_BIGENDIAN) && (defined(DOUBLE_IS_LITTLE_ENDIAN_IEEE754) || \
                                 defined(DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754))
#error "doubles and ints have incompatible endianness"
#endif

#if !defined(WORDS_BIGENDIAN) && defined(DOUBLE_IS_BIG_ENDIAN_IEEE754)
#error "doubles and ints have incompatible endianness"
#endif


#if defined(HAVE_UINT32_T) && defined(HAVE_INT32_T)
typedef PY_UINT32_T ULong;
typedef PY_INT32_T Long;
#else
#error "Failed to find an exact-width 32-bit integer type"
#endif

#if defined(HAVE_UINT64_T)
#define ULLong PY_UINT64_T
#else
#undef ULLong
#endif

#undef DEBUG
#ifdef Py_DEBUG
#define DEBUG
#endif

/* End Python #define linking */

#ifdef DEBUG
#define Bug(x) {fprintf(stderr, "%s\n", x); exit(1);}
#endif

#ifndef PRIVATE_MEM
#define PRIVATE_MEM 2304
#endif
#define PRIVATE_mem ((PRIVATE_MEM+sizeof(double)-1)/sizeof(double))
static double private_mem[PRIVATE_mem], *pmem_next = private_mem;

#ifdef __cplusplus
extern "C" {
#endif

typedef union { double d; ULong L[2]; } U;

#ifdef IEEE_8087
#define word0(x) (x)->L[1]
#define word1(x) (x)->L[0]
#else
#define word0(x) (x)->L[0]
#define word1(x) (x)->L[1]
#endif
#define dval(x) (x)->d

#ifndef STRTOD_DIGLIM
#define STRTOD_DIGLIM 40
#endif

#ifdef DIGLIM_DEBUG
extern int strtod_diglim;
#else
#define strtod_diglim STRTOD_DIGLIM
#endif

/* The following definition of Storeinc is appropriate for MIPS processors.
 * An alternative that might be better on some machines is
 * #define Storeinc(a,b,c) (*a++ = b << 16 | c & 0xffff)
 */
#if defined(IEEE_8087)
#define Storeinc(a,b,c) (((unsigned short *)a)[1] = (unsigned short)b,  \
                         ((unsigned short *)a)[0] = (unsigned short)c, a++)
#else
#define Storeinc(a,b,c) (((unsigned short *)a)[0] = (unsigned short)b,  \
                         ((unsigned short *)a)[1] = (unsigned short)c, a++)
#endif

/* #define P DBL_MANT_DIG */
/* Ten_pmax = floor(P*log(2)/log(5)) */
/* Bletch = (highest power of 2 < DBL_MAX_10_EXP) / 16 */
/* Quick_max = floor((P-1)*log(FLT_RADIX)/log(10) - 1) */
/* Int_max = floor(P*log(FLT_RADIX)/log(10) - 1) */

#define Exp_shift  20
#define Exp_shift1 20
#define Exp_msk1    0x100000
#define Exp_msk11   0x100000
#define Exp_mask  0x7ff00000
#define P 53
#define Nbits 53
#define Bias 1023
#define Emax 1023
#define Emin (-1022)
#define Exp_1  0x3ff00000
#define Exp_11 0x3ff00000
#define Ebits 11
#define Frac_mask  0xfffff
#define Frac_mask1 0xfffff
#define Ten_pmax 22
#define Bletch 0x10
#define Bndry_mask  0xfffff
#define Bndry_mask1 0xfffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 1
#define Tiny0 0
#define Tiny1 1
#define Quick_max 14
#define Int_max 14

#ifndef Flt_Rounds
#ifdef FLT_ROUNDS
#define Flt_Rounds FLT_ROUNDS
#else
#define Flt_Rounds 1
#endif
#endif /*Flt_Rounds*/

#define Rounding Flt_Rounds

#define Big0 (Frac_mask1 | Exp_msk1*(DBL_MAX_EXP+Bias-1))
#define Big1 0xffffffff

#ifndef NAN_WORD0
#define NAN_WORD0 0x7ff80000
#endif

#ifndef NAN_WORD1
#define NAN_WORD1 0
#endif


/* struct BCinfo is used to pass information from _Py_dg_strtod to bigcomp */

typedef struct BCinfo BCinfo;
struct
BCinfo {
    int dp0, dp1, dplen, dsign, e0, inexact;
    int nd, nd0, rounding, scale, uflchk;
};

#define FFFFFFFF 0xffffffffUL

#define Kmax 7

/* struct Bigint is used to represent arbitrary-precision integers.  These
   integers are stored in sign-magnitude format, with the magnitude stored as
   an array of base 2**32 digits.  Bigints are always normalized: if x is a
   Bigint then x->wds >= 1, and either x->wds == 1 or x[wds-1] is nonzero.

   The Bigint fields are as follows:

     - next is a header used by Balloc and Bfree to keep track of lists
         of freed Bigints;  it's also used for the linked list of
         powers of 5 of the form 5**2**i used by pow5mult.
     - k indicates which pool this Bigint was allocated from
     - maxwds is the maximum number of words space was allocated for
       (usually maxwds == 2**k)
     - sign is 1 for negative Bigints, 0 for positive.  The sign is unused
       (ignored on inputs, set to 0 on outputs) in almost all operations
       involving Bigints: a notable exception is the diff function, which
       ignores signs on inputs but sets the sign of the output correctly.
     - wds is the actual number of significant words
     - x contains the vector of words (digits) for this Bigint, from least
       significant (x[0]) to most significant (x[wds-1]).
*/

struct
Bigint {
    struct Bigint *next;
    int k, maxwds, sign, wds;
    ULong x[1];
};

typedef struct Bigint Bigint;

/* Memory management: memory is allocated from, and returned to, Kmax+1 pools
   of memory, where pool k (0 <= k <= Kmax) is for Bigints b with b->maxwds ==
   1 << k.  These pools are maintained as linked lists, with freelist[k]
   pointing to the head of the list for pool k.

   On allocation, if there's no free slot in the appropriate pool, MALLOC is
   called to get more memory.  This memory is not returned to the system until
   Python quits.  There's also a private memory pool that's allocated from
   in preference to using MALLOC.

   For Bigints with more than (1 << Kmax) digits (which implies at least 1233
   decimal digits), memory is directly allocated using MALLOC, and freed using
   FREE.

   XXX: it would be easy to bypass this memory-management system and
   translate each call to Balloc into a call to PyMem_Malloc, and each
   Bfree to PyMem_Free.  Investigate whether this has any significant
   performance on impact. */

static Bigint *freelist[Kmax+1];

/* Allocate space for a Bigint with up to 1<<k digits */

static Bigint *
Balloc(int k)
{
    int x;
    Bigint *rv;
    unsigned int len;

    if (k <= Kmax && (rv = freelist[k]))
        freelist[k] = rv->next;
    else {
        x = 1 << k;
        len = (sizeof(Bigint) + (x-1)*sizeof(ULong) + sizeof(double) - 1)
            /sizeof(double);
        if (pmem_next - private_mem + len <= PRIVATE_mem) {
            rv = (Bigint*)pmem_next;
            pmem_next += len;
        }
        else {
            rv = (Bigint*)MALLOC(len*sizeof(double));
            if (rv == NULL)
                return NULL;
        }
        rv->k = k;
        rv->maxwds = x;
    }
    rv->sign = rv->wds = 0;
    return rv;
}

/* Free a Bigint allocated with Balloc */

static void
Bfree(Bigint *v)
{
    if (v) {
        if (v->k > Kmax)
            FREE((void*)v);
        else {
            v->next = freelist[v->k];
            freelist[v->k] = v;
        }
    }
}

#define Bcopy(x,y) memcpy((char *)&x->sign, (char *)&y->sign,   \
                          y->wds*sizeof(Long) + 2*sizeof(int))

/* Multiply a Bigint b by m and add a.  Either modifies b in place and returns
   a pointer to the modified b, or Bfrees b and returns a pointer to a copy.
   On failure, return NULL.  In this case, b will have been already freed. */

static Bigint *
multadd(Bigint *b, int m, int a)       /* multiply by m and add a */
{
    int i, wds;
#ifdef ULLong
    ULong *x;
    ULLong carry, y;
#else
    ULong carry, *x, y;
    ULong xi, z;
#endif
    Bigint *b1;

    wds = b->wds;
    x = b->x;
    i = 0;
    carry = a;
    do {
#ifdef ULLong
        y = *x * (ULLong)m + carry;
        carry = y >> 32;
        *x++ = y & FFFFFFFF;
#else
        xi = *x;
        y = (xi & 0xffff) * m + carry;
        z = (xi >> 16) * m + (y >> 16);
        carry = z >> 16;
        *x++ = (z << 16) + (y & 0xffff);
#endif
    }
    while(++i < wds);
    if (carry) {
        if (wds >= b->maxwds) {
            b1 = Balloc(b->k+1);
            if (b1 == NULL){
                Bfree(b);
                return NULL;
            }
            Bcopy(b1, b);
            Bfree(b);
            b = b1;
        }
        b->x[wds++] = (ULong)carry;
        b->wds = wds;
    }
    return b;
}

/* convert a string s containing nd decimal digits (possibly containing a
   decimal separator at position nd0, which is ignored) to a Bigint.  This
   function carries on where the parsing code in _Py_dg_strtod leaves off: on
   entry, y9 contains the result of converting the first 9 digits.  Returns
   NULL on failure. */

static Bigint *
s2b(const char *s, int nd0, int nd, ULong y9, int dplen)
{
    Bigint *b;
    int i, k;
    Long x, y;

    x = (nd + 8) / 9;
    for(k = 0, y = 1; x > y; y <<= 1, k++) ;
    b = Balloc(k);
    if (b == NULL)
        return NULL;
    b->x[0] = y9;
    b->wds = 1;

    i = 9;
    if (9 < nd0) {
        s += 9;
        do {
            b = multadd(b, 10, *s++ - '0');
            if (b == NULL)
                return NULL;
        } while(++i < nd0);
        s += dplen;
    }
    else
        s += dplen + 9;
    for(; i < nd; i++) {
        b = multadd(b, 10, *s++ - '0');
        if (b == NULL)
            return NULL;
    }
    return b;
}

/* count leading 0 bits in the 32-bit integer x. */

static int
hi0bits(ULong x)
{
    int k = 0;

    if (!(x & 0xffff0000)) {
        k = 16;
        x <<= 16;
    }
    if (!(x & 0xff000000)) {
        k += 8;
        x <<= 8;
    }
    if (!(x & 0xf0000000)) {
        k += 4;
        x <<= 4;
    }
    if (!(x & 0xc0000000)) {
        k += 2;
        x <<= 2;
    }
    if (!(x & 0x80000000)) {
        k++;
        if (!(x & 0x40000000))
            return 32;
    }
    return k;
}

/* count trailing 0 bits in the 32-bit integer y, and shift y right by that
   number of bits. */

static int
lo0bits(ULong *y)
{
    int k;
    ULong x = *y;

    if (x & 7) {
        if (x & 1)
            return 0;
        if (x & 2) {
            *y = x >> 1;
            return 1;
        }
        *y = x >> 2;
        return 2;
    }
    k = 0;
    if (!(x & 0xffff)) {
        k = 16;
        x >>= 16;
    }
    if (!(x & 0xff)) {
        k += 8;
        x >>= 8;
    }
    if (!(x & 0xf)) {
        k += 4;
        x >>= 4;
    }
    if (!(x & 0x3)) {
        k += 2;
        x >>= 2;
    }
    if (!(x & 1)) {
        k++;
        x >>= 1;
        if (!x)
            return 32;
    }
    *y = x;
    return k;
}

/* convert a small nonnegative integer to a Bigint */

static Bigint *
i2b(int i)
{
    Bigint *b;

    b = Balloc(1);
    if (b == NULL)
        return NULL;
    b->x[0] = i;
    b->wds = 1;
    return b;
}

/* multiply two Bigints.  Returns a new Bigint, or NULL on failure.  Ignores
   the signs of a and b. */

static Bigint *
mult(Bigint *a, Bigint *b)
{
    Bigint *c;
    int k, wa, wb, wc;
    ULong *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
    ULong y;
#ifdef ULLong
    ULLong carry, z;
#else
    ULong carry, z;
    ULong z2;
#endif

    if (a->wds < b->wds) {
        c = a;
        a = b;
        b = c;
    }
    k = a->k;
    wa = a->wds;
    wb = b->wds;
    wc = wa + wb;
    if (wc > a->maxwds)
        k++;
    c = Balloc(k);
    if (c == NULL)
        return NULL;
    for(x = c->x, xa = x + wc; x < xa; x++)
        *x = 0;
    xa = a->x;
    xae = xa + wa;
    xb = b->x;
    xbe = xb + wb;
    xc0 = c->x;
#ifdef ULLong
    for(; xb < xbe; xc0++) {
        if ((y = *xb++)) {
            x = xa;
            xc = xc0;
            carry = 0;
            do {
                z = *x++ * (ULLong)y + *xc + carry;
                carry = z >> 32;
                *xc++ = z & FFFFFFFF;
            }
            while(x < xae);
            *xc = (ULong)carry;
        }
    }
#else
    for(; xb < xbe; xb++, xc0++) {
        if (y = *xb & 0xffff) {
            x = xa;
            xc = xc0;
            carry = 0;
            do {
                z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
                carry = z >> 16;
                z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
                carry = z2 >> 16;
                Storeinc(xc, z2, z);
            }
            while(x < xae);
            *xc = carry;
        }
        if (y = *xb >> 16) {
            x = xa;
            xc = xc0;
            carry = 0;
            z2 = *xc;
            do {
                z = (*x & 0xffff) * y + (*xc >> 16) + carry;
                carry = z >> 16;
                Storeinc(xc, z, z2);
                z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
                carry = z2 >> 16;
            }
            while(x < xae);
            *xc = z2;
        }
    }
#endif
    for(xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) ;
    c->wds = wc;
    return c;
}

/* p5s is a linked list of powers of 5 of the form 5**(2**i), i >= 2 */

static Bigint *p5s;

/* multiply the Bigint b by 5**k.  Returns a pointer to the result, or NULL on
   failure; if the returned pointer is distinct from b then the original
   Bigint b will have been Bfree'd.   Ignores the sign of b. */

static Bigint *
pow5mult(Bigint *b, int k)
{
    Bigint *b1, *p5, *p51;
    int i;
    static int p05[3] = { 5, 25, 125 };

    if ((i = k & 3)) {
        b = multadd(b, p05[i-1], 0);
        if (b == NULL)
            return NULL;
    }

    if (!(k >>= 2))
        return b;
    p5 = p5s;
    if (!p5) {
        /* first time */
        p5 = i2b(625);
        if (p5 == NULL) {
            Bfree(b);
            return NULL;
        }
        p5s = p5;
        p5->next = 0;
    }
    for(;;) {
        if (k & 1) {
            b1 = mult(b, p5);
            Bfree(b);
            b = b1;
            if (b == NULL)
                return NULL;
        }
        if (!(k >>= 1))
            break;
        p51 = p5->next;
        if (!p51) {
            p51 = mult(p5,p5);
            if (p51 == NULL) {
                Bfree(b);
                return NULL;
            }
            p51->next = 0;
            p5->next = p51;
        }
        p5 = p51;
    }
    return b;
}

/* shift a Bigint b left by k bits.  Return a pointer to the shifted result,
   or NULL on failure.  If the returned pointer is distinct from b then the
   original b will have been Bfree'd.   Ignores the sign of b. */

static Bigint *
lshift(Bigint *b, int k)
{
    int i, k1, n, n1;
    Bigint *b1;
    ULong *x, *x1, *xe, z;

    n = k >> 5;
    k1 = b->k;
    n1 = n + b->wds + 1;
    for(i = b->maxwds; n1 > i; i <<= 1)
        k1++;
    b1 = Balloc(k1);
    if (b1 == NULL) {
        Bfree(b);
        return NULL;
    }
    x1 = b1->x;
    for(i = 0; i < n; i++)
        *x1++ = 0;
    x = b->x;
    xe = x + b->wds;
    if (k &= 0x1f) {
        k1 = 32 - k;
        z = 0;
        do {
            *x1++ = *x << k | z;
            z = *x++ >> k1;
        }
        while(x < xe);
        if ((*x1 = z))
            ++n1;
    }
    else do
             *x1++ = *x++;
        while(x < xe);
    b1->wds = n1 - 1;
    Bfree(b);
    return b1;
}

/* Do a three-way compare of a and b, returning -1 if a < b, 0 if a == b and
   1 if a > b.  Ignores signs of a and b. */

static int
cmp(Bigint *a, Bigint *b)
{
    ULong *xa, *xa0, *xb, *xb0;
    int i, j;

    i = a->wds;
    j = b->wds;
#ifdef DEBUG
    if (i > 1 && !a->x[i-1])
        Bug("cmp called with a->x[a->wds-1] == 0");
    if (j > 1 && !b->x[j-1])
        Bug("cmp called with b->x[b->wds-1] == 0");
#endif
    if (i -= j)
        return i;
    xa0 = a->x;
    xa = xa0 + j;
    xb0 = b->x;
    xb = xb0 + j;
    for(;;) {
        if (*--xa != *--xb)
            return *xa < *xb ? -1 : 1;
        if (xa <= xa0)
            break;
    }
    return 0;
}

/* Take the difference of Bigints a and b, returning a new Bigint.  Returns
   NULL on failure.  The signs of a and b are ignored, but the sign of the
   result is set appropriately. */

static Bigint *
diff(Bigint *a, Bigint *b)
{
    Bigint *c;
    int i, wa, wb;
    ULong *xa, *xae, *xb, *xbe, *xc;
#ifdef ULLong
    ULLong borrow, y;
#else
    ULong borrow, y;
    ULong z;
#endif

    i = cmp(a,b);
    if (!i) {
        c = Balloc(0);
        if (c == NULL)
            return NULL;
        c->wds = 1;
        c->x[0] = 0;
        return c;
    }
    if (i < 0) {
        c = a;
        a = b;
        b = c;
        i = 1;
    }
    else
        i = 0;
    c = Balloc(a->k);
    if (c == NULL)
        return NULL;
    c->sign = i;
    wa = a->wds;
    xa = a->x;
    xae = xa + wa;
    wb = b->wds;
    xb = b->x;
    xbe = xb + wb;
    xc = c->x;
    borrow = 0;
#ifdef ULLong
    do {
        y = (ULLong)*xa++ - *xb++ - borrow;
        borrow = y >> 32 & (ULong)1;
        *xc++ = y & FFFFFFFF;
    }
    while(xb < xbe);
    while(xa < xae) {
        y = *xa++ - borrow;
        borrow = y >> 32 & (ULong)1;
        *xc++ = y & FFFFFFFF;
    }
#else
    do {
        y = (*xa & 0xffff) - (*xb & 0xffff) - borrow;
        borrow = (y & 0x10000) >> 16;
        z = (*xa++ >> 16) - (*xb++ >> 16) - borrow;
        borrow = (z & 0x10000) >> 16;
        Storeinc(xc, z, y);
    }
    while(xb < xbe);
    while(xa < xae) {
        y = (*xa & 0xffff) - borrow;
        borrow = (y & 0x10000) >> 16;
        z = (*xa++ >> 16) - borrow;
        borrow = (z & 0x10000) >> 16;
        Storeinc(xc, z, y);
    }
#endif
    while(!*--xc)
        wa--;
    c->wds = wa;
    return c;
}

/* Given a positive normal double x, return the difference between x and the next
   double up.  Doesn't give correct results for subnormals. */

static double
ulp(U *x)
{
    Long L;
    U u;

    L = (word0(x) & Exp_mask) - (P-1)*Exp_msk1;
    word0(&u) = L;
    word1(&u) = 0;
    return dval(&u);
}

/* Convert a Bigint to a double plus an exponent */

static double
b2d(Bigint *a, int *e)
{
    ULong *xa, *xa0, w, y, z;
    int k;
    U d;

    xa0 = a->x;
    xa = xa0 + a->wds;
    y = *--xa;
#ifdef DEBUG
    if (!y) Bug("zero y in b2d");
#endif
    k = hi0bits(y);
    *e = 32 - k;
    if (k < Ebits) {
        word0(&d) = Exp_1 | y >> (Ebits - k);
        w = xa > xa0 ? *--xa : 0;
        word1(&d) = y << ((32-Ebits) + k) | w >> (Ebits - k);
        goto ret_d;
    }
    z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits) {
        word0(&d) = Exp_1 | y << k | z >> (32 - k);
        y = xa > xa0 ? *--xa : 0;
        word1(&d) = z << k | y >> (32 - k);
    }
    else {
        word0(&d) = Exp_1 | y;
        word1(&d) = z;
    }
  ret_d:
    return dval(&d);
}

/* Convert a double to a Bigint plus an exponent.  Return NULL on failure.

   Given a finite nonzero double d, return an odd Bigint b and exponent *e
   such that fabs(d) = b * 2**e.  On return, *bbits gives the number of
   significant bits of e; that is, 2**(*bbits-1) <= b < 2**(*bbits).

   If d is zero, then b == 0, *e == -1010, *bbits = 0.
 */


static Bigint *
d2b(U *d, int *e, int *bits)
{
    Bigint *b;
    int de, k;
    ULong *x, y, z;
    int i;

    b = Balloc(1);
    if (b == NULL)
        return NULL;
    x = b->x;

    z = word0(d) & Frac_mask;
    word0(d) &= 0x7fffffff;   /* clear sign bit, which we ignore */
    if ((de = (int)(word0(d) >> Exp_shift)))
        z |= Exp_msk1;
    if ((y = word1(d))) {
        if ((k = lo0bits(&y))) {
            x[0] = y | z << (32 - k);
            z >>= k;
        }
        else
            x[0] = y;
        i =
            b->wds = (x[1] = z) ? 2 : 1;
    }
    else {
        k = lo0bits(&z);
        x[0] = z;
        i =
            b->wds = 1;
        k += 32;
    }
    if (de) {
        *e = de - Bias - (P-1) + k;
        *bits = P - k;
    }
    else {
        *e = de - Bias - (P-1) + 1 + k;
        *bits = 32*i - hi0bits(x[i-1]);
    }
    return b;
}

/* Compute the ratio of two Bigints, as a double.  The result may have an
   error of up to 2.5 ulps. */

static double
ratio(Bigint *a, Bigint *b)
{
    U da, db;
    int k, ka, kb;

    dval(&da) = b2d(a, &ka);
    dval(&db) = b2d(b, &kb);
    k = ka - kb + 32*(a->wds - b->wds);
    if (k > 0)
        word0(&da) += k*Exp_msk1;
    else {
        k = -k;
        word0(&db) += k*Exp_msk1;
    }
    return dval(&da) / dval(&db);
}

static const double
tens[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
    1e20, 1e21, 1e22
};

static const double
bigtens[] = { 1e16, 1e32, 1e64, 1e128, 1e256 };
static const double tinytens[] = { 1e-16, 1e-32, 1e-64, 1e-128,
                                   9007199254740992.*9007199254740992.e-256
                                   /* = 2^106 * 1e-256 */
};
/* The factor of 2^53 in tinytens[4] helps us avoid setting the underflow */
/* flag unnecessarily.  It leads to a song and dance at the end of strtod. */
#define Scale_Bit 0x10
#define n_bigtens 5

/* case insensitive string match, for recognising 'inf[inity]' and
   'nan' strings. */

static int
match(const char **sp, char *t)
{
    int c, d;
    const char *s = *sp;

    while((d = *t++)) {
        if ((c = *++s) >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        if (c != d)
            return 0;
    }
    *sp = s + 1;
    return 1;
}

#define ULbits 32
#define kshift 5
#define kmask 31


static int
dshift(Bigint *b, int p2)
{
    int rv = hi0bits(b->x[b->wds-1]) - 4;
    if (p2 > 0)
        rv -= p2;
    return rv & kmask;
}

/* special case of Bigint division.  The quotient is always in the range 0 <=
   quotient < 10, and on entry the divisor S is normalized so that its top 4
   bits (28--31) are zero and bit 27 is set. */

static int
quorem(Bigint *b, Bigint *S)
{
    int n;
    ULong *bx, *bxe, q, *sx, *sxe;
#ifdef ULLong
    ULLong borrow, carry, y, ys;
#else
    ULong borrow, carry, y, ys;
    ULong si, z, zs;
#endif

    n = S->wds;
#ifdef DEBUG
    /*debug*/ if (b->wds > n)
        /*debug*/       Bug("oversize b in quorem");
#endif
    if (b->wds < n)
        return 0;
    sx = S->x;
    sxe = sx + --n;
    bx = b->x;
    bxe = bx + n;
    q = *bxe / (*sxe + 1);      /* ensure q <= true quotient */
#ifdef DEBUG
    /*debug*/ if (q > 9)
        /*debug*/       Bug("oversized quotient in quorem");
#endif
    if (q) {
        borrow = 0;
        carry = 0;
        do {
#ifdef ULLong
            ys = *sx++ * (ULLong)q + carry;
            carry = ys >> 32;
            y = *bx - (ys & FFFFFFFF) - borrow;
            borrow = y >> 32 & (ULong)1;
            *bx++ = y & FFFFFFFF;
#else
            si = *sx++;
            ys = (si & 0xffff) * q + carry;
            zs = (si >> 16) * q + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) - borrow;
            borrow = (y & 0x10000) >> 16;
            z = (*bx >> 16) - (zs & 0xffff) - borrow;
            borrow = (z & 0x10000) >> 16;
            Storeinc(bx, z, y);
#endif
        }
        while(sx <= sxe);
        if (!*bxe) {
            bx = b->x;
            while(--bxe > bx && !*bxe)
                --n;
            b->wds = n;
        }
    }
    if (cmp(b, S) >= 0) {
        q++;
        borrow = 0;
        carry = 0;
        bx = b->x;
        sx = S->x;
        do {
#ifdef ULLong
            ys = *sx++ + carry;
            carry = ys >> 32;
            y = *bx - (ys & FFFFFFFF) - borrow;
            borrow = y >> 32 & (ULong)1;
            *bx++ = y & FFFFFFFF;
#else
            si = *sx++;
            ys = (si & 0xffff) + carry;
            zs = (si >> 16) + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) - borrow;
            borrow = (y & 0x10000) >> 16;
            z = (*bx >> 16) - (zs & 0xffff) - borrow;
            borrow = (z & 0x10000) >> 16;
            Storeinc(bx, z, y);
#endif
        }
        while(sx <= sxe);
        bx = b->x;
        bxe = bx + n;
        if (!*bxe) {
            while(--bxe > bx && !*bxe)
                --n;
            b->wds = n;
        }
    }
    return q;
}


/* return 0 on success, -1 on failure */

static int
bigcomp(U *rv, const char *s0, BCinfo *bc)
{
    Bigint *b, *d;
    int b2, bbits, d2, dd, dig, dsign, i, j, nd, nd0, p2, p5, speccase;

    dsign = bc->dsign;
    nd = bc->nd;
    nd0 = bc->nd0;
    p5 = nd + bc->e0 - 1;
    speccase = 0;
    if (rv->d == 0.) {  /* special case: value near underflow-to-zero */
        /* threshold was rounded to zero */
        b = i2b(1);
        if (b == NULL)
            return -1;
        p2 = Emin - P + 1;
        bbits = 1;
        word0(rv) = (P+2) << Exp_shift;
        i = 0;
        {
            speccase = 1;
            --p2;
            dsign = 0;
            goto have_i;
        }
    }
    else
    {
        b = d2b(rv, &p2, &bbits);
        if (b == NULL)
            return -1;
    }
    p2 -= bc->scale;
    /* floor(log2(rv)) == bbits - 1 + p2 */
    /* Check for denormal case. */
    i = P - bbits;
    if (i > (j = P - Emin - 1 + p2)) {
        i = j;
    }
    {
        b = lshift(b, ++i);
        if (b == NULL)
            return -1;
        b->x[0] |= 1;
    }
  have_i:
    p2 -= p5 + i;
    d = i2b(1);
    if (d == NULL) {
        Bfree(b);
        return -1;
    }
    /* Arrange for convenient computation of quotients:
     * shift left if necessary so divisor has 4 leading 0 bits.
     */
    if (p5 > 0) {
        d = pow5mult(d, p5);
        if (d == NULL) {
            Bfree(b);
            return -1;
        }
    }
    else if (p5 < 0) {
        b = pow5mult(b, -p5);
        if (b == NULL) {
            Bfree(d);
            return -1;
        }
    }
    if (p2 > 0) {
        b2 = p2;
        d2 = 0;
    }
    else {
        b2 = 0;
        d2 = -p2;
    }
    i = dshift(d, d2);
    if ((b2 += i) > 0) {
        b = lshift(b, b2);
        if (b == NULL) {
            Bfree(d);
            return -1;
        }
    }
    if ((d2 += i) > 0) {
        d = lshift(d, d2);
        if (d == NULL) {
            Bfree(b);
            return -1;
        }
    }

    /* Now b/d = exactly half-way between the two floating-point values */
    /* on either side of the input string.  Compute first digit of b/d. */

    if (!(dig = quorem(b,d))) {
        b = multadd(b, 10, 0);  /* very unlikely */
        if (b == NULL) {
            Bfree(d);
            return -1;
        }
        dig = quorem(b,d);
    }

    /* Compare b/d with s0 */

    assert(nd > 0);
    dd = 9999;  /* silence gcc compiler warning */
    for(i = 0; i < nd0; ) {
        if ((dd = s0[i++] - '0' - dig))
            goto ret;
        if (!b->x[0] && b->wds == 1) {
            if (i < nd)
                dd = 1;
            goto ret;
        }
        b = multadd(b, 10, 0);
        if (b == NULL) {
            Bfree(d);
            return -1;
        }
        dig = quorem(b,d);
    }
    for(j = bc->dp1; i++ < nd;) {
        if ((dd = s0[j++] - '0' - dig))
            goto ret;
        if (!b->x[0] && b->wds == 1) {
            if (i < nd)
                dd = 1;
            goto ret;
        }
        b = multadd(b, 10, 0);
        if (b == NULL) {
            Bfree(d);
            return -1;
        }
        dig = quorem(b,d);
    }
    if (b->x[0] || b->wds > 1)
        dd = -1;
  ret:
    Bfree(b);
    Bfree(d);
    if (speccase) {
        if (dd <= 0)
            rv->d = 0.;
    }
    else if (dd < 0) {
        if (!dsign)     /* does not happen for round-near */
          retlow1:
            dval(rv) -= ulp(rv);
    }
    else if (dd > 0) {
        if (dsign) {
          rethi1:
            dval(rv) += ulp(rv);
        }
    }
    else {
        /* Exact half-way case:  apply round-even rule. */
        if (word1(rv) & 1) {
            if (dsign)
                goto rethi1;
            goto retlow1;
        }
    }

    return 0;
}

double
_Py_dg_strtod(const char *s00, char **se)
{
    int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, e, e1, error;
    int esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
    const char *s, *s0, *s1;
    double aadj, aadj1;
    Long L;
    U aadj2, adj, rv, rv0;
    ULong y, z;
    BCinfo bc;
    Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;

    sign = nz0 = nz = bc.dplen = bc.uflchk = 0;
    dval(&rv) = 0.;
    for(s = s00;;s++) switch(*s) {
        case '-':
            sign = 1;
            /* no break */
        case '+':
            if (*++s)
                goto break2;
            /* no break */
        case 0:
            goto ret0;
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r':
        case ' ':
            continue;
        default:
            goto break2;
        }
  break2:
    if (*s == '0') {
        nz0 = 1;
        while(*++s == '0') ;
        if (!*s)
            goto ret;
    }
    s0 = s;
    y = z = 0;
    for(nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
        if (nd < 9)
            y = 10*y + c - '0';
        else if (nd < 16)
            z = 10*z + c - '0';
    nd0 = nd;
    bc.dp0 = bc.dp1 = s - s0;
    if (c == '.') {
        c = *++s;
        bc.dp1 = s - s0;
        bc.dplen = bc.dp1 - bc.dp0;
        if (!nd) {
            for(; c == '0'; c = *++s)
                nz++;
            if (c > '0' && c <= '9') {
                s0 = s;
                nf += nz;
                nz = 0;
                goto have_dig;
            }
            goto dig_done;
        }
        for(; c >= '0' && c <= '9'; c = *++s) {
          have_dig:
            nz++;
            if (c -= '0') {
                nf += nz;
                for(i = 1; i < nz; i++)
                    if (nd++ < 9)
                        y *= 10;
                    else if (nd <= DBL_DIG + 1)
                        z *= 10;
                if (nd++ < 9)
                    y = 10*y + c;
                else if (nd <= DBL_DIG + 1)
                    z = 10*z + c;
                nz = 0;
            }
        }
    }
  dig_done:
    e = 0;
    if (c == 'e' || c == 'E') {
        if (!nd && !nz && !nz0) {
            goto ret0;
        }
        s00 = s;
        esign = 0;
        switch(c = *++s) {
        case '-':
            esign = 1;
        case '+':
            c = *++s;
        }
        if (c >= '0' && c <= '9') {
            while(c == '0')
                c = *++s;
            if (c > '0' && c <= '9') {
                L = c - '0';
                s1 = s;
                while((c = *++s) >= '0' && c <= '9')
                    L = 10*L + c - '0';
                if (s - s1 > 8 || L > 19999)
                    /* Avoid confusion from exponents
                     * so large that e might overflow.
                     */
                    e = 19999; /* safe for 16 bit ints */
                else
                    e = (int)L;
                if (esign)
                    e = -e;
            }
            else
                e = 0;
        }
        else
            s = s00;
    }
    if (!nd) {
        if (!nz && !nz0) {
            /* Check for Nan and Infinity */
            if (!bc.dplen)
                switch(c) {
                case 'i':
                case 'I':
                    if (match(&s,"nf")) {
                        --s;
                        if (!match(&s,"inity"))
                            ++s;
                        word0(&rv) = 0x7ff00000;
                        word1(&rv) = 0;
                        goto ret;
                    }
                    break;
                case 'n':
                case 'N':
                    if (match(&s, "an")) {
                        word0(&rv) = NAN_WORD0;
                        word1(&rv) = NAN_WORD1;
                        goto ret;
                    }
                }
          ret0:
            s = s00;
            sign = 0;
        }
        goto ret;
    }
    bc.e0 = e1 = e -= nf;

    /* Now we have nd0 digits, starting at s0, followed by a
     * decimal point, followed by nd-nd0 digits.  The number we're
     * after is the integer represented by those digits times
     * 10**e */

    if (!nd0)
        nd0 = nd;
    k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
    dval(&rv) = y;
    if (k > 9) {
        dval(&rv) = tens[k - 9] * dval(&rv) + z;
    }
    bd0 = 0;
    if (nd <= DBL_DIG
        && Flt_Rounds == 1
        ) {
        if (!e)
            goto ret;
        if (e > 0) {
            if (e <= Ten_pmax) {
                dval(&rv) *= tens[e];
                goto ret;
            }
            i = DBL_DIG - nd;
            if (e <= Ten_pmax + i) {
                /* A fancier test would sometimes let us do
                 * this for larger i values.
                 */
                e -= i;
                dval(&rv) *= tens[i];
                dval(&rv) *= tens[e];
                goto ret;
            }
        }
        else if (e >= -Ten_pmax) {
            dval(&rv) /= tens[-e];
            goto ret;
        }
    }
    e1 += nd - k;

    bc.scale = 0;

    /* Get starting approximation = rv * 10**e1 */

    if (e1 > 0) {
        if ((i = e1 & 15))
            dval(&rv) *= tens[i];
        if (e1 &= ~15) {
            if (e1 > DBL_MAX_10_EXP) {
              ovfl:
                errno = ERANGE;
                /* Can't trust HUGE_VAL */
                word0(&rv) = Exp_mask;
                word1(&rv) = 0;
                goto ret;
            }
            e1 >>= 4;
            for(j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    dval(&rv) *= bigtens[j];
            /* The last multiplication could overflow. */
            word0(&rv) -= P*Exp_msk1;
            dval(&rv) *= bigtens[j];
            if ((z = word0(&rv) & Exp_mask)
                > Exp_msk1*(DBL_MAX_EXP+Bias-P))
                goto ovfl;
            if (z > Exp_msk1*(DBL_MAX_EXP+Bias-1-P)) {
                /* set to largest number */
                /* (Can't trust DBL_MAX) */
                word0(&rv) = Big0;
                word1(&rv) = Big1;
            }
            else
                word0(&rv) += P*Exp_msk1;
        }
    }
    else if (e1 < 0) {
        e1 = -e1;
        if ((i = e1 & 15))
            dval(&rv) /= tens[i];
        if (e1 >>= 4) {
            if (e1 >= 1 << n_bigtens)
                goto undfl;
            if (e1 & Scale_Bit)
                bc.scale = 2*P;
            for(j = 0; e1 > 0; j++, e1 >>= 1)
                if (e1 & 1)
                    dval(&rv) *= tinytens[j];
            if (bc.scale && (j = 2*P + 1 - ((word0(&rv) & Exp_mask)
                                            >> Exp_shift)) > 0) {
                /* scaled rv is denormal; clear j low bits */
                if (j >= 32) {
                    word1(&rv) = 0;
                    if (j >= 53)
                        word0(&rv) = (P+2)*Exp_msk1;
                    else
                        word0(&rv) &= 0xffffffff << (j-32);
                }
                else
                    word1(&rv) &= 0xffffffff << j;
            }
            if (!dval(&rv)) {
              undfl:
                dval(&rv) = 0.;
                errno = ERANGE;
                goto ret;
            }
        }
    }

    /* Now the hard part -- adjusting rv to the correct value.*/

    /* Put digits into bd: true value = bd * 10^e */

    bc.nd = nd;
    bc.nd0 = nd0;       /* Only needed if nd > strtod_diglim, but done here */
                        /* to silence an erroneous warning about bc.nd0 */
                        /* possibly not being initialized. */
    if (nd > strtod_diglim) {
        /* ASSERT(strtod_diglim >= 18); 18 == one more than the */
        /* minimum number of decimal digits to distinguish double values */
        /* in IEEE arithmetic. */
        i = j = 18;
        if (i > nd0)
            j += bc.dplen;
        for(;;) {
            if (--j <= bc.dp1 && j >= bc.dp0)
                j = bc.dp0 - 1;
            if (s0[j] != '0')
                break;
            --i;
        }
        e += nd - i;
        nd = i;
        if (nd0 > nd)
            nd0 = nd;
        if (nd < 9) { /* must recompute y */
            y = 0;
            for(i = 0; i < nd0; ++i)
                y = 10*y + s0[i] - '0';
            for(j = bc.dp1; i < nd; ++i)
                y = 10*y + s0[j++] - '0';
        }
    }
    bd0 = s2b(s0, nd0, nd, y, bc.dplen);
    if (bd0 == NULL)
        goto failed_malloc;

    for(;;) {
        bd = Balloc(bd0->k);
        if (bd == NULL) {
            Bfree(bd0);
            goto failed_malloc;
        }
        Bcopy(bd, bd0);
        bb = d2b(&rv, &bbe, &bbbits);   /* rv = bb * 2^bbe */
        if (bb == NULL) {
            Bfree(bd);
            Bfree(bd0);
            goto failed_malloc;
        }
        bs = i2b(1);
        if (bs == NULL) {
            Bfree(bb);
            Bfree(bd);
            Bfree(bd0);
            goto failed_malloc;
        }

        if (e >= 0) {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
        }
        else {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
        }
        if (bbe >= 0)
            bb2 += bbe;
        else
            bd2 -= bbe;
        bs2 = bb2;
        j = bbe - bc.scale;
        i = j + bbbits - 1;     /* logb(rv) */
        if (i < Emin)   /* denormal */
            j += P - Emin;
        else
            j = P + 1 - bbbits;
        bb2 += j;
        bd2 += j;
        bd2 += bc.scale;
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2)
            i = bs2;
        if (i > 0) {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
        }
        if (bb5 > 0) {
            bs = pow5mult(bs, bb5);
            if (bs == NULL) {
                Bfree(bb);
                Bfree(bd);
                Bfree(bd0);
                goto failed_malloc;
            }
            bb1 = mult(bs, bb);
            Bfree(bb);
            bb = bb1;
            if (bb == NULL) {
                Bfree(bs);
                Bfree(bd);
                Bfree(bd0);
                goto failed_malloc;
            }
        }
        if (bb2 > 0) {
            bb = lshift(bb, bb2);
            if (bb == NULL) {
                Bfree(bs);
                Bfree(bd);
                Bfree(bd0);
                goto failed_malloc;
            }
        }
        if (bd5 > 0) {
            bd = pow5mult(bd, bd5);
            if (bd == NULL) {
                Bfree(bb);
                Bfree(bs);
                Bfree(bd0);
                goto failed_malloc;
            }
        }
        if (bd2 > 0) {
            bd = lshift(bd, bd2);
            if (bd == NULL) {
                Bfree(bb);
                Bfree(bs);
                Bfree(bd0);
                goto failed_malloc;
            }
        }
        if (bs2 > 0) {
            bs = lshift(bs, bs2);
            if (bs == NULL) {
                Bfree(bb);
                Bfree(bd);
                Bfree(bd0);
                goto failed_malloc;
            }
        }
        delta = diff(bb, bd);
        if (delta == NULL) {
            Bfree(bb);
            Bfree(bs);
            Bfree(bd);
            Bfree(bd0);
            goto failed_malloc;
        }
        bc.dsign = delta->sign;
        delta->sign = 0;
        i = cmp(delta, bs);
        if (bc.nd > nd && i <= 0) {
            if (bc.dsign)
                break;  /* Must use bigcomp(). */
            {
                bc.nd = nd;
                i = -1; /* Discarded digits make delta smaller. */
            }
        }

        if (i < 0) {
            /* Error is less than half an ulp -- check for
             * special case of mantissa a power of two.
             */
            if (bc.dsign || word1(&rv) || word0(&rv) & Bndry_mask
                || (word0(&rv) & Exp_mask) <= (2*P+1)*Exp_msk1
                ) {
                break;
            }
            if (!delta->x[0] && delta->wds <= 1) {
                /* exact result */
                break;
            }
            delta = lshift(delta,Log2P);
            if (delta == NULL) {
                Bfree(bb);
                Bfree(bs);
                Bfree(bd);
                Bfree(bd0);
                goto failed_malloc;
            }
            if (cmp(delta, bs) > 0)
                goto drop_down;
            break;
        }
        if (i == 0) {
            /* exactly half-way between */
            if (bc.dsign) {
                if ((word0(&rv) & Bndry_mask1) == Bndry_mask1
                    &&  word1(&rv) == (
                        (bc.scale &&
                         (y = word0(&rv) & Exp_mask) <= 2*P*Exp_msk1) ?
                        (0xffffffff & (0xffffffff << (2*P+1-(y>>Exp_shift)))) :
                        0xffffffff)) {
                    /*boundary case -- increment exponent*/
                    word0(&rv) = (word0(&rv) & Exp_mask)
                        + Exp_msk1
                        ;
                    word1(&rv) = 0;
                    bc.dsign = 0;
                    break;
                }
            }
            else if (!(word0(&rv) & Bndry_mask) && !word1(&rv)) {
              drop_down:
                /* boundary case -- decrement exponent */
                if (bc.scale) {
                    L = word0(&rv) & Exp_mask;
                    if (L <= (2*P+1)*Exp_msk1) {
                        if (L > (P+2)*Exp_msk1)
                            /* round even ==> */
                            /* accept rv */
                            break;
                        /* rv = smallest denormal */
                        if (bc.nd >nd) {
                            bc.uflchk = 1;
                            break;
                        }
                        goto undfl;
                    }
                }
                L = (word0(&rv) & Exp_mask) - Exp_msk1;
                word0(&rv) = L | Bndry_mask1;
                word1(&rv) = 0xffffffff;
                break;
            }
            if (!(word1(&rv) & LSB))
                break;
            if (bc.dsign)
                dval(&rv) += ulp(&rv);
            else {
                dval(&rv) -= ulp(&rv);
                if (!dval(&rv)) {
                    if (bc.nd >nd) {
                        bc.uflchk = 1;
                        break;
                    }
                    goto undfl;
                }
            }
            bc.dsign = 1 - bc.dsign;
            break;
        }
        if ((aadj = ratio(delta, bs)) <= 2.) {
            if (bc.dsign)
                aadj = aadj1 = 1.;
            else if (word1(&rv) || word0(&rv) & Bndry_mask) {
                if (word1(&rv) == Tiny1 && !word0(&rv)) {
                    if (bc.nd >nd) {
                        bc.uflchk = 1;
                        break;
                    }
                    goto undfl;
                }
                aadj = 1.;
                aadj1 = -1.;
            }
            else {
                /* special case -- power of FLT_RADIX to be */
                /* rounded down... */

                if (aadj < 2./FLT_RADIX)
                    aadj = 1./FLT_RADIX;
                else
                    aadj *= 0.5;
                aadj1 = -aadj;
            }
        }
        else {
            aadj *= 0.5;
            aadj1 = bc.dsign ? aadj : -aadj;
            if (Flt_Rounds == 0)
                aadj1 += 0.5;
        }
        y = word0(&rv) & Exp_mask;

        /* Check for overflow */

        if (y == Exp_msk1*(DBL_MAX_EXP+Bias-1)) {
            dval(&rv0) = dval(&rv);
            word0(&rv) -= P*Exp_msk1;
            adj.d = aadj1 * ulp(&rv);
            dval(&rv) += adj.d;
            if ((word0(&rv) & Exp_mask) >=
                Exp_msk1*(DBL_MAX_EXP+Bias-P)) {
                if (word0(&rv0) == Big0 && word1(&rv0) == Big1)
                    goto ovfl;
                word0(&rv) = Big0;
                word1(&rv) = Big1;
                goto cont;
            }
            else
                word0(&rv) += P*Exp_msk1;
        }
        else {
            if (bc.scale && y <= 2*P*Exp_msk1) {
                if (aadj <= 0x7fffffff) {
                    if ((z = (ULong)aadj) <= 0)
                        z = 1;
                    aadj = z;
                    aadj1 = bc.dsign ? aadj : -aadj;
                }
                dval(&aadj2) = aadj1;
                word0(&aadj2) += (2*P+1)*Exp_msk1 - y;
                aadj1 = dval(&aadj2);
            }
            adj.d = aadj1 * ulp(&rv);
            dval(&rv) += adj.d;
        }
        z = word0(&rv) & Exp_mask;
        if (bc.nd == nd) {
            if (!bc.scale)
                if (y == z) {
                    /* Can we stop now? */
                    L = (Long)aadj;
                    aadj -= L;
                    /* The tolerances below are conservative. */
                    if (bc.dsign || word1(&rv) || word0(&rv) & Bndry_mask) {
                        if (aadj < .4999999 || aadj > .5000001)
                            break;
                    }
                    else if (aadj < .4999999/FLT_RADIX)
                        break;
                }
        }
      cont:
        Bfree(bb);
        Bfree(bd);
        Bfree(bs);
        Bfree(delta);
    }
    Bfree(bb);
    Bfree(bd);
    Bfree(bs);
    Bfree(bd0);
    Bfree(delta);
    if (bc.nd > nd) {
        error = bigcomp(&rv, s0, &bc);
        if (error)
            goto failed_malloc;
    }

    if (bc.scale) {
        word0(&rv0) = Exp_1 - 2*P*Exp_msk1;
        word1(&rv0) = 0;
        dval(&rv) *= dval(&rv0);
        /* try to avoid the bug of testing an 8087 register value */
        if (!(word0(&rv) & Exp_mask))
            errno = ERANGE;
    }
  ret:
    if (se)
        *se = (char *)s;
    return sign ? -dval(&rv) : dval(&rv);

  failed_malloc:
    if (se)
        *se = (char *)s00;
    errno = ENOMEM;
    return -1.0;
}

static char *
rv_alloc(int i)
{
    int j, k, *r;

    j = sizeof(ULong);
    for(k = 0;
        sizeof(Bigint) - sizeof(ULong) - sizeof(int) + j <= (unsigned)i;
        j <<= 1)
        k++;
    r = (int*)Balloc(k);
    if (r == NULL)
        return NULL;
    *r = k;
    return (char *)(r+1);
}

static char *
nrv_alloc(char *s, char **rve, int n)
{
    char *rv, *t;

    rv = rv_alloc(n);
    if (rv == NULL)
        return NULL;
    t = rv;
    while((*t = *s++)) t++;
    if (rve)
        *rve = t;
    return rv;
}

/* freedtoa(s) must be used to free values s returned by dtoa
 * when MULTIPLE_THREADS is #defined.  It should be used in all cases,
 * but for consistency with earlier versions of dtoa, it is optional
 * when MULTIPLE_THREADS is not defined.
 */

void
_Py_dg_freedtoa(char *s)
{
    Bigint *b = (Bigint *)((int *)s - 1);
    b->maxwds = 1 << (b->k = *(int*)b);
    Bfree(b);
}

/* dtoa for IEEE arithmetic (dmg): convert double to ASCII string.
 *
 * Inspired by "How to Print Floating-Point Numbers Accurately" by
 * Guy L. Steele, Jr. and Jon L. White [Proc. ACM SIGPLAN '90, pp. 112-126].
 *
 * Modifications:
 *      1. Rather than iterating, we use a simple numeric overestimate
 *         to determine k = floor(log10(d)).  We scale relevant
 *         quantities using O(log2(k)) rather than O(k) multiplications.
 *      2. For some modes > 2 (corresponding to ecvt and fcvt), we don't
 *         try to generate digits strictly left to right.  Instead, we
 *         compute with fewer bits and propagate the carry if necessary
 *         when rounding the final digit up.  This is often faster.
 *      3. Under the assumption that input will be rounded nearest,
 *         mode 0 renders 1e23 as 1e23 rather than 9.999999999999999e22.
 *         That is, we allow equality in stopping tests when the
 *         round-nearest rule will give the same floating-point value
 *         as would satisfaction of the stopping test with strict
 *         inequality.
 *      4. We remove common factors of powers of 2 from relevant
 *         quantities.
 *      5. When converting floating-point integers less than 1e16,
 *         we use floating-point arithmetic rather than resorting
 *         to multiple-precision integers.
 *      6. When asked to produce fewer than 15 digits, we first try
 *         to get by with floating-point arithmetic; we resort to
 *         multiple-precision integer arithmetic only if we cannot
 *         guarantee that the floating-point calculation has given
 *         the correctly rounded result.  For k requested digits and
 *         "uniformly" distributed input, the probability is
 *         something like 10^(k-15) that we must resort to the Long
 *         calculation.
 */

/* Additional notes (METD): (1) returns NULL on failure.  (2) to avoid memory
   leakage, a successful call to _Py_dg_dtoa should always be matched by a
   call to _Py_dg_freedtoa. */

char *
_Py_dg_dtoa(double dd, int mode, int ndigits,
            int *decpt, int *sign, char **rve)
{
    /*  Arguments ndigits, decpt, sign are similar to those
        of ecvt and fcvt; trailing zeros are suppressed from
        the returned string.  If not null, *rve is set to point
        to the end of the return value.  If d is +-Infinity or NaN,
        then *decpt is set to 9999.

        mode:
        0 ==> shortest string that yields d when read in
        and rounded to nearest.
        1 ==> like 0, but with Steele & White stopping rule;
        e.g. with IEEE P754 arithmetic , mode 0 gives
        1e23 whereas mode 1 gives 9.999999999999999e22.
        2 ==> max(1,ndigits) significant digits.  This gives a
        return value similar to that of ecvt, except
        that trailing zeros are suppressed.
        3 ==> through ndigits past the decimal point.  This
        gives a return value similar to that from fcvt,
        except that trailing zeros are suppressed, and
        ndigits can be negative.
        4,5 ==> similar to 2 and 3, respectively, but (in
        round-nearest mode) with the tests of mode 0 to
        possibly return a shorter string that rounds to d.
        With IEEE arithmetic and compilation with
        -DHonor_FLT_ROUNDS, modes 4 and 5 behave the same
        as modes 2 and 3 when FLT_ROUNDS != 1.
        6-9 ==> Debugging modes similar to mode - 4:  don't try
        fast floating-point estimate (if applicable).

        Values of mode other than 0-9 are treated as mode 0.

        Sufficient space is allocated to the return value
        to hold the suppressed trailing zeros.
    */

    int bbits, b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1,
        j, j1, k, k0, k_check, leftright, m2, m5, s2, s5,
        spec_case, try_quick;
    Long L;
    int denorm;
    ULong x;
    Bigint *b, *b1, *delta, *mlo, *mhi, *S;
    U d2, eps, u;
    double ds;
    char *s, *s0;

    /* set pointers to NULL, to silence gcc compiler warnings and make
       cleanup easier on error */
    mlo = mhi = b = S = 0;
    s0 = 0;

    u.d = dd;
    if (word0(&u) & Sign_bit) {
        /* set sign for everything, including 0's and NaNs */
        *sign = 1;
        word0(&u) &= ~Sign_bit; /* clear sign bit */
    }
    else
        *sign = 0;

    /* quick return for Infinities, NaNs and zeros */
    if ((word0(&u) & Exp_mask) == Exp_mask)
    {
        /* Infinity or NaN */
        *decpt = 9999;
        if (!word1(&u) && !(word0(&u) & 0xfffff))
            return nrv_alloc("Infinity", rve, 8);
        return nrv_alloc("NaN", rve, 3);
    }
    if (!dval(&u)) {
        *decpt = 1;
        return nrv_alloc("0", rve, 1);
    }

    /* compute k = floor(log10(d)).  The computation may leave k
       one too large, but should never leave k too small. */
    b = d2b(&u, &be, &bbits);
    if (b == NULL)
        goto failed_malloc;
    if ((i = (int)(word0(&u) >> Exp_shift1 & (Exp_mask>>Exp_shift1)))) {
        dval(&d2) = dval(&u);
        word0(&d2) &= Frac_mask1;
        word0(&d2) |= Exp_11;

        /* log(x)       ~=~ log(1.5) + (x-1.5)/1.5
         * log10(x)      =  log(x) / log(10)
         *              ~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
         * log10(d) = (i-Bias)*log(2)/log(10) + log10(d2)
         *
         * This suggests computing an approximation k to log10(d) by
         *
         * k = (i - Bias)*0.301029995663981
         *      + ( (d2-1.5)*0.289529654602168 + 0.176091259055681 );
         *
         * We want k to be too large rather than too small.
         * The error in the first-order Taylor series approximation
         * is in our favor, so we just round up the constant enough
         * to compensate for any error in the multiplication of
         * (i - Bias) by 0.301029995663981; since |i - Bias| <= 1077,
         * and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
         * adding 1e-13 to the constant term more than suffices.
         * Hence we adjust the constant term to 0.1760912590558.
         * (We could get a more accurate k by invoking log10,
         *  but this is probably not worthwhile.)
         */

        i -= Bias;
        denorm = 0;
    }
    else {
        /* d is denormalized */

        i = bbits + be + (Bias + (P-1) - 1);
        x = i > 32  ? word0(&u) << (64 - i) | word1(&u) >> (i - 32)
            : word1(&u) << (32 - i);
        dval(&d2) = x;
        word0(&d2) -= 31*Exp_msk1; /* adjust exponent */
        i -= (Bias + (P-1) - 1) + 1;
        denorm = 1;
    }
    ds = (dval(&d2)-1.5)*0.289529654602168 + 0.1760912590558 +
        i*0.301029995663981;
    k = (int)ds;
    if (ds < 0. && ds != k)
        k--;    /* want k = floor(ds) */
    k_check = 1;
    if (k >= 0 && k <= Ten_pmax) {
        if (dval(&u) < tens[k])
            k--;
        k_check = 0;
    }
    j = bbits - i - 1;
    if (j >= 0) {
        b2 = 0;
        s2 = j;
    }
    else {
        b2 = -j;
        s2 = 0;
    }
    if (k >= 0) {
        b5 = 0;
        s5 = k;
        s2 += k;
    }
    else {
        b2 -= k;
        b5 = -k;
        s5 = 0;
    }
    if (mode < 0 || mode > 9)
        mode = 0;

    try_quick = 1;

    if (mode > 5) {
        mode -= 4;
        try_quick = 0;
    }
    leftright = 1;
    ilim = ilim1 = -1;  /* Values for cases 0 and 1; done here to */
    /* silence erroneous "gcc -Wall" warning. */
    switch(mode) {
    case 0:
    case 1:
        i = 18;
        ndigits = 0;
        break;
    case 2:
        leftright = 0;
        /* no break */
    case 4:
        if (ndigits <= 0)
            ndigits = 1;
        ilim = ilim1 = i = ndigits;
        break;
    case 3:
        leftright = 0;
        /* no break */
    case 5:
        i = ndigits + k + 1;
        ilim = i;
        ilim1 = i - 1;
        if (i <= 0)
            i = 1;
    }
    s0 = rv_alloc(i);
    if (s0 == NULL)
        goto failed_malloc;
    s = s0;


    if (ilim >= 0 && ilim <= Quick_max && try_quick) {

        /* Try to get by with floating-point arithmetic. */

        i = 0;
        dval(&d2) = dval(&u);
        k0 = k;
        ilim0 = ilim;
        ieps = 2; /* conservative */
        if (k > 0) {
            ds = tens[k&0xf];
            j = k >> 4;
            if (j & Bletch) {
                /* prevent overflows */
                j &= Bletch - 1;
                dval(&u) /= bigtens[n_bigtens-1];
                ieps++;
            }
            for(; j; j >>= 1, i++)
                if (j & 1) {
                    ieps++;
                    ds *= bigtens[i];
                }
            dval(&u) /= ds;
        }
        else if ((j1 = -k)) {
            dval(&u) *= tens[j1 & 0xf];
            for(j = j1 >> 4; j; j >>= 1, i++)
                if (j & 1) {
                    ieps++;
                    dval(&u) *= bigtens[i];
                }
        }
        if (k_check && dval(&u) < 1. && ilim > 0) {
            if (ilim1 <= 0)
                goto fast_failed;
            ilim = ilim1;
            k--;
            dval(&u) *= 10.;
            ieps++;
        }
        dval(&eps) = ieps*dval(&u) + 7.;
        word0(&eps) -= (P-1)*Exp_msk1;
        if (ilim == 0) {
            S = mhi = 0;
            dval(&u) -= 5.;
            if (dval(&u) > dval(&eps))
                goto one_digit;
            if (dval(&u) < -dval(&eps))
                goto no_digits;
            goto fast_failed;
        }
        if (leftright) {
            /* Use Steele & White method of only
             * generating digits needed.
             */
            dval(&eps) = 0.5/tens[ilim-1] - dval(&eps);
            for(i = 0;;) {
                L = (Long)dval(&u);
                dval(&u) -= L;
                *s++ = '0' + (int)L;
                if (dval(&u) < dval(&eps))
                    goto ret1;
                if (1. - dval(&u) < dval(&eps))
                    goto bump_up;
                if (++i >= ilim)
                    break;
                dval(&eps) *= 10.;
                dval(&u) *= 10.;
            }
        }
        else {
            /* Generate ilim digits, then fix them up. */
            dval(&eps) *= tens[ilim-1];
            for(i = 1;; i++, dval(&u) *= 10.) {
                L = (Long)(dval(&u));
                if (!(dval(&u) -= L))
                    ilim = i;
                *s++ = '0' + (int)L;
                if (i == ilim) {
                    if (dval(&u) > 0.5 + dval(&eps))
                        goto bump_up;
                    else if (dval(&u) < 0.5 - dval(&eps)) {
                        while(*--s == '0');
                        s++;
                        goto ret1;
                    }
                    break;
                }
            }
        }
      fast_failed:
        s = s0;
        dval(&u) = dval(&d2);
        k = k0;
        ilim = ilim0;
    }

    /* Do we have a "small" integer? */

    if (be >= 0 && k <= Int_max) {
        /* Yes. */
        ds = tens[k];
        if (ndigits < 0 && ilim <= 0) {
            S = mhi = 0;
            if (ilim < 0 || dval(&u) <= 5*ds)
                goto no_digits;
            goto one_digit;
        }
        for(i = 1;; i++, dval(&u) *= 10.) {
            L = (Long)(dval(&u) / ds);
            dval(&u) -= L*ds;
            *s++ = '0' + (int)L;
            if (!dval(&u)) {
                break;
            }
            if (i == ilim) {
                dval(&u) += dval(&u);
                if (dval(&u) > ds || (dval(&u) == ds && L & 1)) {
                  bump_up:
                    while(*--s == '9')
                        if (s == s0) {
                            k++;
                            *s = '0';
                            break;
                        }
                    ++*s++;
                }
                break;
            }
        }
        goto ret1;
    }

    m2 = b2;
    m5 = b5;
    if (leftright) {
        i =
            denorm ? be + (Bias + (P-1) - 1 + 1) :
            1 + P - bbits;
        b2 += i;
        s2 += i;
        mhi = i2b(1);
        if (mhi == NULL)
            goto failed_malloc;
    }
    if (m2 > 0 && s2 > 0) {
        i = m2 < s2 ? m2 : s2;
        b2 -= i;
        m2 -= i;
        s2 -= i;
    }
    if (b5 > 0) {
        if (leftright) {
            if (m5 > 0) {
                mhi = pow5mult(mhi, m5);
                if (mhi == NULL)
                    goto failed_malloc;
                b1 = mult(mhi, b);
                Bfree(b);
                b = b1;
                if (b == NULL)
                    goto failed_malloc;
            }
            if ((j = b5 - m5)) {
                b = pow5mult(b, j);
                if (b == NULL)
                    goto failed_malloc;
            }
        }
        else {
            b = pow5mult(b, b5);
            if (b == NULL)
                goto failed_malloc;
        }
    }
    S = i2b(1);
    if (S == NULL)
        goto failed_malloc;
    if (s5 > 0) {
        S = pow5mult(S, s5);
        if (S == NULL)
            goto failed_malloc;
    }

    /* Check for special case that d is a normalized power of 2. */

    spec_case = 0;
    if ((mode < 2 || leftright)
        ) {
        if (!word1(&u) && !(word0(&u) & Bndry_mask)
            && word0(&u) & (Exp_mask & ~Exp_msk1)
            ) {
            /* The special case */
            b2 += Log2P;
            s2 += Log2P;
            spec_case = 1;
        }
    }

    /* Arrange for convenient computation of quotients:
     * shift left if necessary so divisor has 4 leading 0 bits.
     *
     * Perhaps we should just compute leading 28 bits of S once
     * and for all and pass them and a shift to quorem, so it
     * can do shifts and ors to compute the numerator for q.
     */
    if ((i = ((s5 ? 32 - hi0bits(S->x[S->wds-1]) : 1) + s2) & 0x1f))
        i = 32 - i;
#define iInc 28
    i = dshift(S, s2);
    b2 += i;
    m2 += i;
    s2 += i;
    if (b2 > 0) {
        b = lshift(b, b2);
        if (b == NULL)
            goto failed_malloc;
    }
    if (s2 > 0) {
        S = lshift(S, s2);
        if (S == NULL)
            goto failed_malloc;
    }
    if (k_check) {
        if (cmp(b,S) < 0) {
            k--;
            b = multadd(b, 10, 0);      /* we botched the k estimate */
            if (b == NULL)
                goto failed_malloc;
            if (leftright) {
                mhi = multadd(mhi, 10, 0);
                if (mhi == NULL)
                    goto failed_malloc;
            }
            ilim = ilim1;
        }
    }
    if (ilim <= 0 && (mode == 3 || mode == 5)) {
        if (ilim < 0) {
            /* no digits, fcvt style */
          no_digits:
            k = -1 - ndigits;
            goto ret;
        }
        else {
            S = multadd(S, 5, 0);
            if (S == NULL)
                goto failed_malloc;
            if (cmp(b, S) <= 0)
                goto no_digits;
        }
      one_digit:
        *s++ = '1';
        k++;
        goto ret;
    }
    if (leftright) {
        if (m2 > 0) {
            mhi = lshift(mhi, m2);
            if (mhi == NULL)
                goto failed_malloc;
        }

        /* Compute mlo -- check for special case
         * that d is a normalized power of 2.
         */

        mlo = mhi;
        if (spec_case) {
            mhi = Balloc(mhi->k);
            if (mhi == NULL)
                goto failed_malloc;
            Bcopy(mhi, mlo);
            mhi = lshift(mhi, Log2P);
            if (mhi == NULL)
                goto failed_malloc;
        }

        for(i = 1;;i++) {
            dig = quorem(b,S) + '0';
            /* Do we yet have the shortest decimal string
             * that will round to d?
             */
            j = cmp(b, mlo);
            delta = diff(S, mhi);
            if (delta == NULL)
                goto failed_malloc;
            j1 = delta->sign ? 1 : cmp(b, delta);
            Bfree(delta);
            if (j1 == 0 && mode != 1 && !(word1(&u) & 1)
                ) {
                if (dig == '9')
                    goto round_9_up;
                if (j > 0)
                    dig++;
                *s++ = dig;
                goto ret;
            }
            if (j < 0 || (j == 0 && mode != 1
                          && !(word1(&u) & 1)
                    )) {
                if (!b->x[0] && b->wds <= 1) {
                    goto accept_dig;
                }
                if (j1 > 0) {
                    b = lshift(b, 1);
                    if (b == NULL)
                        goto failed_malloc;
                    j1 = cmp(b, S);
                    if ((j1 > 0 || (j1 == 0 && dig & 1))
                        && dig++ == '9')
                        goto round_9_up;
                }
              accept_dig:
                *s++ = dig;
                goto ret;
            }
            if (j1 > 0) {
                if (dig == '9') { /* possible if i == 1 */
                  round_9_up:
                    *s++ = '9';
                    goto roundoff;
                }
                *s++ = dig + 1;
                goto ret;
            }
            *s++ = dig;
            if (i == ilim)
                break;
            b = multadd(b, 10, 0);
            if (b == NULL)
                goto failed_malloc;
            if (mlo == mhi) {
                mlo = mhi = multadd(mhi, 10, 0);
                if (mlo == NULL)
                    goto failed_malloc;
            }
            else {
                mlo = multadd(mlo, 10, 0);
                if (mlo == NULL)
                    goto failed_malloc;
                mhi = multadd(mhi, 10, 0);
                if (mhi == NULL)
                    goto failed_malloc;
            }
        }
    }
    else
        for(i = 1;; i++) {
            *s++ = dig = quorem(b,S) + '0';
            if (!b->x[0] && b->wds <= 1) {
                goto ret;
            }
            if (i >= ilim)
                break;
            b = multadd(b, 10, 0);
            if (b == NULL)
                goto failed_malloc;
        }

    /* Round off last digit */

    b = lshift(b, 1);
    if (b == NULL)
        goto failed_malloc;
    j = cmp(b, S);
    if (j > 0 || (j == 0 && dig & 1)) {
      roundoff:
        while(*--s == '9')
            if (s == s0) {
                k++;
                *s++ = '1';
                goto ret;
            }
        ++*s++;
    }
    else {
        while(*--s == '0');
        s++;
    }
  ret:
    Bfree(S);
    if (mhi) {
        if (mlo && mlo != mhi)
            Bfree(mlo);
        Bfree(mhi);
    }
  ret1:
    Bfree(b);
    *s = 0;
    *decpt = k + 1;
    if (rve)
        *rve = s;
    return s0;
  failed_malloc:
    if (S)
        Bfree(S);
    if (mlo && mlo != mhi)
        Bfree(mlo);
    if (mhi)
        Bfree(mhi);
    if (b)
        Bfree(b);
    if (s0)
        _Py_dg_freedtoa(s0);
    return NULL;
}
#ifdef __cplusplus
}
#endif

#endif  /* PY_NO_SHORT_FLOAT_REPR */
