/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Long (arbitrary precision) integer object implementation */

/* XXX The functional organization of this file is terrible */

#include "allobjects.h"
#include "intrcheck.h"
#include "longintrepr.h"
#include <math.h>
#include <assert.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* Forward */
static longobject *long_normalize PROTO((longobject *));
static longobject *mul1 PROTO((longobject *, wdigit));
static longobject *muladd1 PROTO((longobject *, wdigit, wdigit));
static longobject *divrem1 PROTO((longobject *, wdigit, digit *));

static int ticker;	/* XXX Could be shared with ceval? */

#define INTRCHECK(block) \
	if (--ticker < 0) { \
		ticker = 100; \
		if (intrcheck()) { block; } \
	}

/* Normalize (remove leading zeros from) a long int object.
   Doesn't attempt to free the storage--in most cases, due to the nature
   of the algorithms used, this could save at most be one word anyway. */

static longobject *
long_normalize(v)
	register longobject *v;
{
	int j = ABS(v->ob_size);
	register int i = j;
	
	while (i > 0 && v->ob_digit[i-1] == 0)
		--i;
	if (i != j)
		v->ob_size = (v->ob_size < 0) ? -(i) : i;
	return v;
}

/* Allocate a new long int object with size digits.
   Return NULL and set exception if we run out of memory. */

longobject *
alloclongobject(size)
	int size;
{
	return NEWVAROBJ(longobject, &Longtype, size);
}

/* Create a new long int object from a C long int */

object *
newlongobject(ival)
	long ival;
{
	/* Assume a C long fits in at most 3 'digits' */
	longobject *v = alloclongobject(3);
	if (v != NULL) {
		if (ival < 0) {
			ival = -ival;
			v->ob_size = -(v->ob_size);
		}
		v->ob_digit[0] = ival & MASK;
		v->ob_digit[1] = (ival >> SHIFT) & MASK;
		v->ob_digit[2] = (ival >> (2*SHIFT)) & MASK;
		v = long_normalize(v);
	}
	return (object *)v;
}

/* Create a new long int object from a C double */

object *
dnewlongobject(dval)
	double dval;
{
	longobject *v;
	double frac;
	int i, ndig, expo, neg;
	neg = 0;
	if (dval < 0.0) {
		neg = 1;
		dval = -dval;
	}
	frac = frexp(dval, &expo); /* dval = frac*2**expo; 0.0 <= frac < 1.0 */
	if (expo <= 0)
		return newlongobject(0L);
	ndig = (expo-1) / SHIFT + 1; /* Number of 'digits' in result */
	v = alloclongobject(ndig);
	if (v == NULL)
		return NULL;
	frac = ldexp(frac, (expo-1) % SHIFT + 1);
	for (i = ndig; --i >= 0; ) {
		long bits = (long)frac;
		v->ob_digit[i] = bits;
		frac = frac - (double)bits;
		frac = ldexp(frac, SHIFT);
	}
	if (neg)
		v->ob_size = -(v->ob_size);
	return (object *)v;
}

/* Get a C long int from a long int object.
   Returns -1 and sets an error condition if overflow occurs. */

long
getlongvalue(vv)
	object *vv;
{
	register longobject *v;
	long x, prev;
	int i, sign;
	
	if (vv == NULL || !is_longobject(vv)) {
		err_badcall();
		return -1;
	}
	v = (longobject *)vv;
	i = v->ob_size;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev) {
			err_setstr(ValueError,
				"long int too long to convert");
			return -1;
		}
	}
	return x * sign;
}

/* Get a C double from a long int object.  No overflow check. */

double
dgetlongvalue(vv)
	object *vv;
{
	register longobject *v;
	double x;
	double multiplier = (double) (1L << SHIFT);
	int i, sign;
	
	if (vv == NULL || !is_longobject(vv)) {
		err_badcall();
		return -1;
	}
	v = (longobject *)vv;
	i = v->ob_size;
	sign = 1;
	x = 0.0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		x = x*multiplier + v->ob_digit[i];
	}
	return x * sign;
}

/* Multiply by a single digit, ignoring the sign. */

static longobject *
mul1(a, n)
	longobject *a;
	wdigit n;
{
	return muladd1(a, n, (digit)0);
}

/* Multiply by a single digit and add a single digit, ignoring the sign. */

static longobject *
muladd1(a, n, extra)
	longobject *a;
	wdigit n;
	wdigit extra;
{
	int size_a = ABS(a->ob_size);
	longobject *z = alloclongobject(size_a+1);
	twodigits carry = extra;
	int i;
	
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_a; ++i) {
		carry += (twodigits)a->ob_digit[i] * n;
		z->ob_digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	z->ob_digit[i] = carry;
	return long_normalize(z);
}

/* Divide a long integer by a digit, returning both the quotient
   (as function result) and the remainder (through *prem).
   The sign of a is ignored; n should not be zero. */

static longobject *
divrem1(a, n, prem)
	longobject *a;
	wdigit n;
	digit *prem;
{
	int size = ABS(a->ob_size);
	longobject *z;
	int i;
	twodigits rem = 0;
	
	assert(n > 0 && n <= MASK);
	z = alloclongobject(size);
	if (z == NULL)
		return NULL;
	for (i = size; --i >= 0; ) {
		rem = (rem << SHIFT) + a->ob_digit[i];
		z->ob_digit[i] = rem/n;
		rem %= n;
	}
	*prem = rem;
	return long_normalize(z);
}

/* Convert a long int object to a string, using a given conversion base.
   Return a string object.
   If base is 8 or 16, add the proper prefix '0' or '0x'.
   External linkage: used in bltinmodule.c by hex() and oct(). */

object *
long_format(aa, base)
	object *aa;
	int base;
{
	register longobject *a = (longobject *)aa;
	stringobject *str;
	int i;
	int size_a = ABS(a->ob_size);
	char *p;
	int bits;
	char sign = '\0';

	if (a == NULL || !is_longobject(a)) {
		err_badcall();
		return NULL;
	}
	assert(base >= 2 && base <= 36);
	
	/* Compute a rough upper bound for the length of the string */
	i = base;
	bits = 0;
	while (i > 1) {
		++bits;
		i >>= 1;
	}
	i = 6 + (size_a*SHIFT + bits-1) / bits;
	str = (stringobject *) newsizedstringobject((char *)0, i);
	if (str == NULL)
		return NULL;
	p = GETSTRINGVALUE(str) + i;
	*p = '\0';
	*--p = 'L';
	if (a->ob_size < 0)
		sign = '-';
	
	INCREF(a);
	do {
		digit rem;
		longobject *temp = divrem1(a, (digit)base, &rem);
		if (temp == NULL) {
			DECREF(a);
			DECREF(str);
			return NULL;
		}
		if (rem < 10)
			rem += '0';
		else
			rem += 'A'-10;
		assert(p > GETSTRINGVALUE(str));
		*--p = rem;
		DECREF(a);
		a = temp;
		INTRCHECK({
			DECREF(a);
			DECREF(str);
			err_set(KeyboardInterrupt);
			return NULL;
		})
	} while (ABS(a->ob_size) != 0);
	DECREF(a);
	if (base == 8) {
		if (size_a != 0)
			*--p = '0';
	}
	else if (base == 16) {
		*--p = 'x';
		*--p = '0';
	}
	else if (base != 10) {
		*--p = '#';
		*--p = '0' + base%10;
		if (base > 10)
			*--p = '0' + base/10;
	}
	if (sign)
		*--p = sign;
	if (p != GETSTRINGVALUE(str)) {
		char *q = GETSTRINGVALUE(str);
		assert(p > q);
		do {
		} while ((*q++ = *p++) != '\0');
		q--;
		resizestring((object **)&str, (int) (q - GETSTRINGVALUE(str)));
	}
	return (object *)str;
}

/* Convert a string to a long int object, in a given base.
   Base zero implies a default depending on the number.
   External linkage: used in compile.c for literals. */

object *
long_scan(str, base)
	char *str;
	int base;
{
	int sign = 1;
	longobject *z;
	
	assert(base == 0 || base >= 2 && base <= 36);
	if (*str == '+')
		++str;
	else if (*str == '-') {
		++str;
		sign = -1;
	}
	if (base == 0) {
		if (str[0] != '0')
			base = 10;
		else if (str[1] == 'x' || str[1] == 'X')
			base = 16;
		else
			base = 8;
	}
	if (base == 16 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		str += 2;
	z = alloclongobject(0);
	for ( ; z != NULL; ++str) {
		int k = -1;
		longobject *temp;
		
		if (*str <= '9')
			k = *str - '0';
		else if (*str >= 'a')
			k = *str - 'a' + 10;
		else if (*str >= 'A')
			k = *str - 'A' + 10;
		if (k < 0 || k >= base)
			break;
		temp = muladd1(z, (digit)base, (digit)k);
		DECREF(z);
		z = temp;
	}
	if (sign < 0 && z != NULL && z->ob_size != 0)
		z->ob_size = -(z->ob_size);
	return (object *) z;
}

static longobject *x_divrem PROTO((longobject *, longobject *, longobject **));
static object *long_pos PROTO((longobject *));
static long_divrem PROTO((longobject *, longobject *,
	longobject **, longobject **));

/* Long division with remainder, top-level routine */

static int
long_divrem(a, b, pdiv, prem)
	longobject *a, *b;
	longobject **pdiv;
	longobject **prem;
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	longobject *z;
	
	if (size_b == 0) {
		err_setstr(ZeroDivisionError, "long division or modulo");
		return -1;
	}
	if (size_a < size_b ||
			size_a == size_b &&
			a->ob_digit[size_a-1] < b->ob_digit[size_b-1]) {
		/* |a| < |b|. */
		*pdiv = alloclongobject(0);
		INCREF(a);
		*prem = (longobject *) a;
		return 0;
	}
	if (size_b == 1) {
		digit rem = 0;
		z = divrem1(a, b->ob_digit[0], &rem);
		if (z == NULL)
			return -1;
		*prem = (longobject *) newlongobject((long)rem);
	}
	else {
		z = x_divrem(a, b, prem);
		if (z == NULL)
			return -1;
	}
	/* Set the signs.
	   The quotient z has the sign of a*b;
	   the remainder r has the sign of a,
	   so a = b*z + r. */
	if ((a->ob_size < 0) != (b->ob_size < 0))
		z->ob_size = -(z->ob_size);
	if (a->ob_size < 0 && (*prem)->ob_size != 0)
		(*prem)->ob_size = -((*prem)->ob_size);
	*pdiv = z;
	return 0;
}

/* Unsigned long division with remainder -- the algorithm */

static longobject *
x_divrem(v1, w1, prem)
	longobject *v1, *w1;
	longobject **prem;
{
	int size_v = ABS(v1->ob_size), size_w = ABS(w1->ob_size);
	digit d = (twodigits)BASE / (w1->ob_digit[size_w-1] + 1);
	longobject *v = mul1(v1, d);
	longobject *w = mul1(w1, d);
	longobject *a;
	int j, k;
	
	if (v == NULL || w == NULL) {
		XDECREF(v);
		XDECREF(w);
		return NULL;
	}
	
	assert(size_v >= size_w && size_w > 1); /* Assert checks by div() */
	assert(v->ob_refcnt == 1); /* Since v will be used as accumulator! */
	assert(size_w == ABS(w->ob_size)); /* That's how d was calculated */
	
	size_v = ABS(v->ob_size);
	a = alloclongobject(size_v - size_w + 1);
	
	for (j = size_v, k = a->ob_size-1; a != NULL && k >= 0; --j, --k) {
		digit vj = (j >= size_v) ? 0 : v->ob_digit[j];
		twodigits q;
		stwodigits carry = 0;
		int i;
		
		INTRCHECK({
			DECREF(a);
			a = NULL;
			err_set(KeyboardInterrupt);
			break;
		})
		if (vj == w->ob_digit[size_w-1])
			q = MASK;
		else
			q = (((twodigits)vj << SHIFT) + v->ob_digit[j-1]) /
				w->ob_digit[size_w-1];
		
		while (w->ob_digit[size_w-2]*q >
				((
					((twodigits)vj << SHIFT)
					+ v->ob_digit[j-1]
					- q*w->ob_digit[size_w-1]
								) << SHIFT)
				+ v->ob_digit[j-2])
			--q;
		
		for (i = 0; i < size_w && i+k < size_v; ++i) {
			twodigits z = w->ob_digit[i] * q;
			digit zz = z >> SHIFT;
			carry += v->ob_digit[i+k] - z + ((twodigits)zz << SHIFT);
			v->ob_digit[i+k] = carry & MASK;
			carry = (carry >> SHIFT) - zz;
		}
		
		if (i+k < size_v) {
			carry += v->ob_digit[i+k];
			v->ob_digit[i+k] = 0;
		}
		
		if (carry == 0)
			a->ob_digit[k] = q;
		else {
			assert(carry == -1);
			a->ob_digit[k] = q-1;
			carry = 0;
			for (i = 0; i < size_w && i+k < size_v; ++i) {
				carry += v->ob_digit[i+k] + w->ob_digit[i];
				v->ob_digit[i+k] = carry & MASK;
				carry >>= SHIFT;
			}
		}
	} /* for j, k */
	
	if (a != NULL) {
		a = long_normalize(a);
		*prem = divrem1(v, d, &d);
		/* d receives the (unused) remainder */
		if (*prem == NULL) {
			DECREF(a);
			a = NULL;
		}
	}
	DECREF(v);
	DECREF(w);
	return a;
}

/* Methods */

/* Forward */
static void long_dealloc PROTO((object *));
static int long_print PROTO((object *, FILE *, int));
static object *long_repr PROTO((object *));
static int long_compare PROTO((longobject *, longobject *));
static long long_hash PROTO((longobject *));

static object *long_add PROTO((longobject *, longobject *));
static object *long_sub PROTO((longobject *, longobject *));
static object *long_mul PROTO((longobject *, longobject *));
static object *long_div PROTO((longobject *, longobject *));
static object *long_mod PROTO((longobject *, longobject *));
static object *long_divmod PROTO((longobject *, longobject *));
static object *long_pow PROTO((longobject *, longobject *));
static object *long_neg PROTO((longobject *));
static object *long_pos PROTO((longobject *));
static object *long_abs PROTO((longobject *));
static int long_nonzero PROTO((longobject *));
static object *long_invert PROTO((longobject *));
static object *long_lshift PROTO((longobject *, longobject *));
static object *long_rshift PROTO((longobject *, longobject *));
static object *long_and PROTO((longobject *, longobject *));
static object *long_xor PROTO((longobject *, longobject *));
static object *long_or PROTO((longobject *, longobject *));

static void
long_dealloc(v)
	object *v;
{
	DEL(v);
}

static object *
long_repr(v)
	object *v;
{
	return long_format(v, 10);
}

static int
long_compare(a, b)
	longobject *a, *b;
{
	int sign;
	
	if (a->ob_size != b->ob_size) {
		if (ABS(a->ob_size) == 0 && ABS(b->ob_size) == 0)
			sign = 0;
		else
			sign = a->ob_size - b->ob_size;
	}
	else {
		int i = ABS(a->ob_size);
		while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
			;
		if (i < 0)
			sign = 0;
		else {
			sign = (int)a->ob_digit[i] - (int)b->ob_digit[i];
			if (a->ob_size < 0)
				sign = -sign;
		}
	}
	return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}

static long
long_hash(v)
	longobject *v;
{
	long x;
	int i, sign;

	/* This is designed so that Python ints and longs with the
	   same value hash to the same value, otherwise comparisons
	   of mapping keys will turn out weird */
	i = v->ob_size;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		/* Force a 32-bit circular shift */
		x = ((x << SHIFT) & ~MASK) | ((x >> (32-SHIFT)) & MASK);
		x += v->ob_digit[i];
	}
	x = x * sign;
	if (x == -1)
		x = -2;
	return x;
}


/* Add the absolute values of two long integers. */

static longobject *x_add PROTO((longobject *, longobject *));
static longobject *
x_add(a, b)
	longobject *a, *b;
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	longobject *z;
	int i;
	digit carry = 0;
	
	/* Ensure a is the larger of the two: */
	if (size_a < size_b) {
		{ longobject *temp = a; a = b; b = temp; }
		{ int size_temp = size_a; size_a = size_b; size_b = size_temp; }
	}
	z = alloclongobject(size_a+1);
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_b; ++i) {
		carry += a->ob_digit[i] + b->ob_digit[i];
		z->ob_digit[i] = carry & MASK;
		/* The following assumes unsigned shifts don't
		   propagate the sign bit. */
		carry >>= SHIFT;
	}
	for (; i < size_a; ++i) {
		carry += a->ob_digit[i];
		z->ob_digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	z->ob_digit[i] = carry;
	return long_normalize(z);
}

/* Subtract the absolute values of two integers. */

static longobject *x_sub PROTO((longobject *, longobject *));
static longobject *
x_sub(a, b)
	longobject *a, *b;
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	longobject *z;
	int i;
	int sign = 1;
	digit borrow = 0;
	
	/* Ensure a is the larger of the two: */
	if (size_a < size_b) {
		sign = -1;
		{ longobject *temp = a; a = b; b = temp; }
		{ int size_temp = size_a; size_a = size_b; size_b = size_temp; }
	}
	else if (size_a == size_b) {
		/* Find highest digit where a and b differ: */
		i = size_a;
		while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
			;
		if (i < 0)
			return alloclongobject(0);
		if (a->ob_digit[i] < b->ob_digit[i]) {
			sign = -1;
			{ longobject *temp = a; a = b; b = temp; }
		}
		size_a = size_b = i+1;
	}
	z = alloclongobject(size_a);
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_b; ++i) {
		/* The following assumes unsigned arithmetic
		   works module 2**N for some N>SHIFT. */
		borrow = a->ob_digit[i] - b->ob_digit[i] - borrow; 
		z->ob_digit[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1; /* Keep only one sign bit */
	}
	for (; i < size_a; ++i) {
		borrow = a->ob_digit[i] - borrow;
		z->ob_digit[i] = borrow & MASK;
		borrow >>= SHIFT;
	}
	assert(borrow == 0);
	if (sign < 0)
		z->ob_size = -(z->ob_size);
	return long_normalize(z);
}

static object *
long_add(a, b)
	longobject *a;
	longobject *b;
{
	longobject *z;
	
	if (a->ob_size < 0) {
		if (b->ob_size < 0) {
			z = x_add(a, b);
			if (z != NULL && z->ob_size != 0)
				z->ob_size = -(z->ob_size);
		}
		else
			z = x_sub(b, a);
	}
	else {
		if (b->ob_size < 0)
			z = x_sub(a, b);
		else
			z = x_add(a, b);
	}
	return (object *)z;
}

static object *
long_sub(a, b)
	longobject *a;
	longobject *b;
{
	longobject *z;
	
	if (a->ob_size < 0) {
		if (b->ob_size < 0)
			z = x_sub(a, b);
		else
			z = x_add(a, b);
		if (z != NULL && z->ob_size != 0)
			z->ob_size = -(z->ob_size);
	}
	else {
		if (b->ob_size < 0)
			z = x_add(a, b);
		else
			z = x_sub(a, b);
	}
	return (object *)z;
}

static object *
long_mul(a, b)
	longobject *a;
	longobject *b;
{
	int size_a;
	int size_b;
	longobject *z;
	int i;
	
	size_a = ABS(a->ob_size);
	size_b = ABS(b->ob_size);
	z = alloclongobject(size_a + size_b);
	if (z == NULL)
		return NULL;
	for (i = 0; i < z->ob_size; ++i)
		z->ob_digit[i] = 0;
	for (i = 0; i < size_a; ++i) {
		twodigits carry = 0;
		twodigits f = a->ob_digit[i];
		int j;
		
		INTRCHECK({
			DECREF(z);
			err_set(KeyboardInterrupt);
			return NULL;
		})
		for (j = 0; j < size_b; ++j) {
			carry += z->ob_digit[i+j] + b->ob_digit[j] * f;
			z->ob_digit[i+j] = carry & MASK;
			carry >>= SHIFT;
		}
		for (; carry != 0; ++j) {
			assert(i+j < z->ob_size);
			carry += z->ob_digit[i+j];
			z->ob_digit[i+j] = carry & MASK;
			carry >>= SHIFT;
		}
	}
	if (a->ob_size < 0)
		z->ob_size = -(z->ob_size);
	if (b->ob_size < 0)
		z->ob_size = -(z->ob_size);
	return (object *) long_normalize(z);
}

/* The / and % operators are now defined in terms of divmod().
   The expression a mod b has the value a - b*floor(a/b).
   The long_divrem function gives the remainder after division of
   |a| by |b|, with the sign of a.  This is also expressed
   as a - b*trunc(a/b), if trunc truncates towards zero.
   Some examples:
   	 a	 b	a rem b		a mod b
   	 13	 10	 3		 3
   	-13	 10	-3		 7
   	 13	-10	 3		-7
   	-13	-10	-3		-3
   So, to get from rem to mod, we have to add b if a and b
   have different signs.  We then subtract one from the 'div'
   part of the outcome to keep the invariant intact. */

static int l_divmod PROTO((longobject *, longobject *,
	longobject **, longobject **));
static int
l_divmod(v, w, pdiv, pmod)
	longobject *v;
	longobject *w;
	longobject **pdiv;
	longobject **pmod;
{
	longobject *div, *mod;
	
	if (long_divrem(v, w, &div, &mod) < 0)
		return -1;
	if (mod->ob_size < 0 && w->ob_size > 0 ||
				mod->ob_size > 0 && w->ob_size < 0) {
		longobject *temp;
		longobject *one;
		temp = (longobject *) long_add(mod, w);
		DECREF(mod);
		mod = temp;
		if (mod == NULL) {
			DECREF(div);
			return -1;
		}
		one = (longobject *) newlongobject(1L);
		if (one == NULL ||
		    (temp = (longobject *) long_sub(div, one)) == NULL) {
			DECREF(mod);
			DECREF(div);
			XDECREF(one);
			return -1;
		}
		DECREF(one);
		DECREF(div);
		div = temp;
	}
	*pdiv = div;
	*pmod = mod;
	return 0;
}

static object *
long_div(v, w)
	longobject *v;
	longobject *w;
{
	longobject *div, *mod;
	if (l_divmod(v, w, &div, &mod) < 0)
		return NULL;
	DECREF(mod);
	return (object *)div;
}

static object *
long_mod(v, w)
	longobject *v;
	longobject *w;
{
	longobject *div, *mod;
	if (l_divmod(v, w, &div, &mod) < 0)
		return NULL;
	DECREF(div);
	return (object *)mod;
}

static object *
long_divmod(v, w)
	longobject *v;
	longobject *w;
{
	object *z;
	longobject *div, *mod;
	if (l_divmod(v, w, &div, &mod) < 0)
		return NULL;
	z = newtupleobject(2);
	if (z != NULL) {
		settupleitem(z, 0, (object *) div);
		settupleitem(z, 1, (object *) mod);
	}
	else {
		DECREF(div);
		DECREF(mod);
	}
	return z;
}

static object *
long_pow(a, b)
	longobject *a;
	longobject *b;
{
	longobject *z;
	int size_b, i;
	
	size_b = b->ob_size;
	if (size_b < 0) {
		err_setstr(ValueError, "long integer to the negative power");
		return NULL;
	}

	z = (longobject *)newlongobject(1L);
	
	INCREF(a);
	for (i = 0; i < size_b; ++i) {
		digit bi = b->ob_digit[i];
		int j;
		
		for (j = 0; j < SHIFT; ++j) {
			longobject *temp;
			
			if (bi & 1) {
				temp = (longobject *)long_mul(z, a);
				DECREF(z);
				z = temp;
				if (z == NULL)
					break;
			}
			bi >>= 1;
			if (bi == 0 && i+1 == size_b)
				break;
			temp = (longobject *)long_mul(a, a);
			DECREF(a);
			a = temp;
			if (a == NULL) {
				DECREF(z);
				z = NULL;
				break;
			}
		}
		if (a == NULL)
			break;
	}
	XDECREF(a);
	return (object *)z;
}

static object *
long_invert(v)
	longobject *v;
{
	/* Implement ~x as -(x+1) */
	longobject *x;
	longobject *w;
	w = (longobject *)newlongobject(1L);
	if (w == NULL)
		return NULL;
	x = (longobject *) long_add(v, w);
	DECREF(w);
	if (x == NULL)
		return NULL;
	if (x->ob_size != 0)
		x->ob_size = -(x->ob_size);
	return (object *)x;
}

static object *
long_pos(v)
	longobject *v;
{
	INCREF(v);
	return (object *)v;
}

static object *
long_neg(v)
	longobject *v;
{
	longobject *z;
	int i, n;
	n = ABS(v->ob_size);
	if (n == 0) {
		/* -0 == 0 */
		INCREF(v);
		return (object *) v;
	}
	z = alloclongobject(ABS(n));
	if (z == NULL)
		return NULL;
	for (i = 0; i < n; i++)
		z->ob_digit[i] = v->ob_digit[i];
	z->ob_size = -(v->ob_size);
	return (object *)z;
}

static object *
long_abs(v)
	longobject *v;
{
	if (v->ob_size < 0)
		return long_neg(v);
	else {
		INCREF(v);
		return (object *)v;
	}
}

static int
long_nonzero(v)
	longobject *v;
{
	return ABS(v->ob_size) != 0;
}

static object *
long_rshift(a, b)
	longobject *a;
	longobject *b;
{
	longobject *z;
	long shiftby;
	int newsize, wordshift, loshift, hishift, i, j;
	digit lomask, himask;
	
	if (a->ob_size < 0) {
		/* Right shifting negative numbers is harder */
		longobject *a1, *a2, *a3;
		a1 = (longobject *) long_invert(a);
		if (a1 == NULL) return NULL;
		a2 = (longobject *) long_rshift(a1, b);
		DECREF(a1);
		if (a2 == NULL) return NULL;
		a3 = (longobject *) long_invert(a2);
		DECREF(a2);
		return (object *) a3;
	}
	
	shiftby = getlongvalue((object *)b);
	if (shiftby == -1L && err_occurred())
		return NULL;
	if (shiftby < 0) {
		err_setstr(ValueError, "negative shift count");
		return NULL;
	}
	wordshift = shiftby / SHIFT;
	newsize = ABS(a->ob_size) - wordshift;
	if (newsize <= 0) {
		z = alloclongobject(0);
		return (object *)z;
	}
	loshift = shiftby % SHIFT;
	hishift = SHIFT - loshift;
	lomask = ((digit)1 << hishift) - 1;
	himask = MASK ^ lomask;
	z = alloclongobject(newsize);
	if (z == NULL)
		return NULL;
	if (a->ob_size < 0)
		z->ob_size = -(z->ob_size);
	for (i = 0, j = wordshift; i < newsize; i++, j++) {
		z->ob_digit[i] = (a->ob_digit[j] >> loshift) & lomask;
		if (i+1 < newsize)
			z->ob_digit[i] |=
			  (a->ob_digit[j+1] << hishift) & himask;
	}
	return (object *) long_normalize(z);
}

static object *
long_lshift(a, b)
	longobject *a;
	longobject *b;
{
	longobject *z;
	long shiftby;
	int newsize, wordshift, loshift, hishift, i, j;
	digit lomask, himask;
	
	shiftby = getlongvalue((object *)b);
	if (shiftby == -1L && err_occurred())
		return NULL;
	if (shiftby < 0) {
		err_setstr(ValueError, "negative shift count");
		return NULL;
	}
	if (shiftby > MASK) {
		err_setstr(ValueError, "outrageous left shift count");
		return NULL;
	}
	if (shiftby % SHIFT == 0) {
		wordshift = shiftby / SHIFT;
		loshift = 0;
		hishift = SHIFT;
		newsize = ABS(a->ob_size) + wordshift;
		lomask = MASK;
		himask = 0;
	}
	else {
		wordshift = shiftby / SHIFT + 1;
		loshift = SHIFT - shiftby%SHIFT;
		hishift = shiftby % SHIFT;
		newsize = ABS(a->ob_size) + wordshift;
		lomask = ((digit)1 << hishift) - 1;
		himask = MASK ^ lomask;
	}
	z = alloclongobject(newsize);
	if (z == NULL)
		return NULL;
	if (a->ob_size < 0)
		z->ob_size = -(z->ob_size);
	for (i = 0; i < wordshift; i++)
		z->ob_digit[i] = 0;
	for (i = wordshift, j = 0; i < newsize; i++, j++) {
		if (i > 0)
			z->ob_digit[i-1] |=
				(a->ob_digit[j] << hishift) & himask;
		z->ob_digit[i] =
			(a->ob_digit[j] >> loshift) & lomask;
	}
	return (object *) long_normalize(z);
}


/* Bitwise and/xor/or operations */

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

static object *long_bitwise PROTO((longobject *, int, longobject *));
static object *
long_bitwise(a, op, b)
	longobject *a;
	int op; /* '&', '|', '^' */
	longobject *b;
{
	digit maska, maskb; /* 0 or MASK */
	int negz;
	int size_a, size_b, size_z;
	longobject *z;
	int i;
	digit diga, digb;
	object *v;
	
	if (a->ob_size < 0) {
		a = (longobject *) long_invert(a);
		maska = MASK;
	}
	else {
		INCREF(a);
		maska = 0;
	}
	if (b->ob_size < 0) {
		b = (longobject *) long_invert(b);
		maskb = MASK;
	}
	else {
		INCREF(b);
		maskb = 0;
	}
	
	size_a = a->ob_size;
	size_b = b->ob_size;
	size_z = MAX(size_a, size_b);
	z = alloclongobject(size_z);
	if (a == NULL || b == NULL || z == NULL) {
		XDECREF(a);
		XDECREF(b);
		XDECREF(z);
		return NULL;
	}
	
	negz = 0;
	switch (op) {
	case '^':
		if (maska != maskb) {
			maska ^= MASK;
			negz = -1;
		}
		break;
	case '&': 
		if (maska && maskb) {
			op = '|';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	case '|':
		if (maska || maskb) {
			op = '&';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	}
	
	for (i = 0; i < size_z; ++i) {
		diga = (i < size_a ? a->ob_digit[i] : 0) ^ maska;
		digb = (i < size_b ? b->ob_digit[i] : 0) ^ maskb;
		switch (op) {
		case '&': z->ob_digit[i] = diga & digb; break;
		case '|': z->ob_digit[i] = diga | digb; break;
		case '^': z->ob_digit[i] = diga ^ digb; break;
		}
	}
	
	DECREF(a);
	DECREF(b);
	z = long_normalize(z);
	if (negz == 0)
		return (object *) z;
	v = long_invert(z);
	DECREF(z);
	return v;
}

static object *
long_and(a, b)
	longobject *a;
	longobject *b;
{
	return long_bitwise(a, '&', b);
}

static object *
long_xor(a, b)
	longobject *a;
	longobject *b;
{
	return long_bitwise(a, '^', b);
}

static object *
long_or(a, b)
	longobject *a;
	longobject *b;
{
	return long_bitwise(a, '|', b);
}

int
long_coerce(pv, pw)
	object **pv;
	object **pw;
{
	if (is_intobject(*pw)) {
		*pw = newlongobject(getintvalue(*pw));
		INCREF(*pv);
		return 0;
	}
	return 1; /* Can't do it */
}

static object *
long_int(v)
	object *v;
{
	long x;
	x = getlongvalue(v);
	if (err_occurred())
		return NULL;
	return newintobject(x);
}

static object *
long_long(v)
	object *v;
{
	INCREF(v);
	return v;
}

static object *
long_float(v)
	object *v;
{
	return newfloatobject(dgetlongvalue(v));
}

static object *
long_oct(v)
	object *v;
{
	return long_format(v, 8);
}

static object *
long_hex(v)
	object *v;
{
	return long_format(v, 16);
}


#define UF (object* (*) FPROTO((object *))) /* Unary function */
#define BF (object* (*) FPROTO((object *, object *))) /* Binary function */
#define IF (int (*) FPROTO((object *))) /* Int function */

static number_methods long_as_number = {
	BF long_add,	/*nb_add*/
	BF long_sub,	/*nb_subtract*/
	BF long_mul,	/*nb_multiply*/
	BF long_div,	/*nb_divide*/
	BF long_mod,	/*nb_remainder*/
	BF long_divmod,	/*nb_divmod*/
	BF long_pow,	/*nb_power*/
	UF long_neg,	/*nb_negative*/
	UF long_pos,	/*tp_positive*/
	UF long_abs,	/*tp_absolute*/
	IF long_nonzero,/*tp_nonzero*/
	UF long_invert,	/*nb_invert*/
	BF long_lshift,	/*nb_lshift*/
	BF long_rshift,	/*nb_rshift*/
	BF long_and,	/*nb_and*/
	BF long_xor,	/*nb_xor*/
	BF long_or,	/*nb_or*/
	(int (*) FPROTO((object **, object **)))
	long_coerce,	/*nb_coerce*/
	UF long_int,	/*nb_int*/
	UF long_long,	/*nb_long*/
	UF long_float,	/*nb_float*/
	UF long_oct,	/*nb_oct*/
	UF long_hex,	/*nb_hex*/
};

typeobject Longtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"long int",
	sizeof(longobject) - sizeof(digit),
	sizeof(digit),
	long_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(int (*) FPROTO((object *, object *)))
	long_compare,	/*tp_compare*/
	long_repr,	/*tp_repr*/
	&long_as_number,/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(long (*) FPROTO((object *)))
	long_hash,	/*tp_hash*/
};
