/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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
#include "longintrepr.h"
#include <assert.h>

static int ticker;	/* XXX Could be shared with ceval? */

#define INTRCHECK(block) \
	if (--ticker < 0) { \
		ticker = 100; \
		if (intrcheck()) { block; } \
	}

/* Normalize (remove leading zeros from) a long int object.
   Doesn't attempt to free the storage--in most cases, due to the nature
   of the algorithms used, this could save at most be one word anyway. */

longobject *
long_normalize(v)
	register longobject *v;
{
	int j = ABS(v->ob_size);
	register int i = j;
	
	while (i > 0 && v->ob_digit[i-1] == 0)
		--i;
	if (i != j)
		v->ob_size = (v->ob_size < 0) ? -i : i;
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
			v->ob_size = -v->ob_size;
		}
		v->ob_digit[0] = ival & MASK;
		v->ob_digit[1] = (ival >> SHIFT) & MASK;
		v->ob_digit[2] = (ival >> (2*SHIFT)) & MASK;
		v = long_normalize(v);
	}
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
		i = -i;
	}
	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev) {
			err_setstr(RuntimeError,
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
		i = -i;
	}
	while (--i >= 0) {
		x = x*multiplier + v->ob_digit[i];
	}
	return x * sign;
}

/* Multiply by a single digit, ignoring the sign. */

longobject *
mul1(a, n)
	longobject *a;
	wdigit n;
{
	return muladd1(a, n, (digit)0);
}

/* Multiply by a single digit and add a single digit, ignoring the sign. */

longobject *
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

longobject *
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
   Return a string object. */

stringobject *
long_format(a, base)
	longobject *a;
	int base;
{
	stringobject *str;
	int i;
	int size_a = ABS(a->ob_size);
	char *p;
	int bits;
	char sign = '\0';
	
	assert(base >= 2 && base <= 36);
	
	/* Compute a rough upper bound for the length of the string */
	i = base;
	bits = 0;
	while (i > 1) {
		++bits;
		i >>= 1;
	}
	i = 1 + (size_a*SHIFT + bits-1) / bits;
	str = (stringobject *) newsizedstringobject((char *)0, i);
	if (str == NULL)
		return NULL;
	p = GETSTRINGVALUE(str) + i;
	*p = '\0';
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
	} while (a->ob_size != 0);
	DECREF(a);
	if (sign)
		*--p = sign;
	if (p != GETSTRINGVALUE(str)) {
		char *q = GETSTRINGVALUE(str);
		assert(p > q);
		do {
		} while ((*q++ = *p++) != '\0');
		resizestring((object **)&str, (int) (q - GETSTRINGVALUE(str)));
	}
	return str;
}

/* Convert a string to a long int object, in a given base.
   Base zero implies a default depending on the number. */

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
	if (z != NULL)
		z->ob_size *= sign;
	return (object *) z;
}

static longobject *x_divrem PROTO((longobject *, longobject *, longobject **));

/* Long division with remainder, top-level routine */

longobject *
long_divrem(a, b, prem)
	longobject *a, *b;
	longobject **prem;
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	longobject *z;
	
	if (size_b == 0) {
		if (prem != NULL)
			*prem = NULL;
		err_setstr(RuntimeError, "long division by zero");
		return NULL;
	}
	if (size_a < size_b ||
			size_a == size_b &&
			a->ob_digit[size_a-1] < b->ob_digit[size_b-1]) {
		/* |a| < |b|. */
		if (prem != NULL) {
			INCREF(a);
			*prem = a;
		}
		return alloclongobject(0);
	}
	if (size_b == 1) {
		digit rem = 0;
		z = divrem1(a, b->ob_digit[0], &rem);
		if (prem != NULL) {
			if (z == NULL)
				*prem = NULL;
			else
				*prem = (longobject *)
					newlongobject((long)rem);
		}
	}
	else
		z = x_divrem(a, b, prem);
	/* Set the signs.
	   The quotient z has the sign of a*b;
	   the remainder r has the sign of a,
	   so a = b*z + r. */
	if (z != NULL) {
		if ((a->ob_size < 0) != (b->ob_size < 0))
			z->ob_size = - z->ob_size;
		if (prem != NULL && *prem != NULL && a->ob_size < 0)
			(*prem)->ob_size = - (*prem)->ob_size;
	}
	return z;
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
		if (prem != NULL)
			*prem = NULL;
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
	
	if (a != NULL)
		a = long_normalize(a);
	if (prem != 0) {
		if (a == NULL)
			*prem = NULL;
		else
			*prem = divrem1(v, d, &d);
		/* Using d as a dummy to receive the - unused - remainder */
	}
	DECREF(v);
	DECREF(w);
	return a;
}

/* Methods */

static void
long_dealloc(v)
	longobject *v;
{
	DEL(v);
}

static void
long_print(v, fp, flags)
	longobject *v;
	FILE *fp;
	int flags;
{
	stringobject *str = long_format(v, 10);
	if (str == NULL) {
		err_clear();
		fprintf(fp, "[err]");
	}
	else {
		fprintf(fp, "%sL", GETSTRINGVALUE(str));
		DECREF(str);
	}
}

static object *
long_repr(v)
	longobject *v;
{
	stringobject *str = long_format(v, 10);
	if (str != NULL) {
		int len = getstringsize((object *)str);
		resizestring((object **)&str, len + 1);
		if (str != NULL)
			GETSTRINGVALUE(str)[len] = 'L';
	}
	return (object *)str;
}

static int
long_compare(a, b)
	longobject *a, *b;
{
	int sign;
	
	if (a->ob_size != b->ob_size)
		sign = a->ob_size - b->ob_size;
	else {
		int i = ABS(a->ob_size);
		while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
			;
		if (i < 0)
			sign = 0;
		else
			sign = (int)a->ob_digit[i] - (int)b->ob_digit[i];
	}
	return sign;
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
	z->ob_size *= sign;
	return long_normalize(z);
}

static object *
long_add(a, w)
	longobject *a;
	object *w;
{
	longobject *b;
	longobject *z;
	
	if (!is_longobject(w)) {
		err_badarg();
		return NULL;
	}
	b = (longobject *)w;
	
	if (a->ob_size < 0) {
		if (b->ob_size < 0) {
			z = x_add(a, b);
			if (z != NULL)
				z->ob_size = -z->ob_size;
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
long_sub(a, w)
	longobject *a;
	object *w;
{
	longobject *b;
	longobject *z;
	
	if (!is_longobject(w)) {
		err_badarg();
		return NULL;
	}
	b = (longobject *)w;
	
	if (a->ob_size < 0) {
		if (b->ob_size < 0)
			z = x_sub(a, b);
		else
			z = x_add(a, b);
		if (z != NULL)
			z->ob_size = -z->ob_size;
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
long_mul(a, w)
	longobject *a;
	object *w;
{
	longobject *b;
	int size_a;
	int size_b;
	longobject *z;
	int i;
	
	if (!is_longobject(w)) {
		err_badarg();
		return NULL;
	}
	b = (longobject *)w;
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
		z->ob_size = -z->ob_size;
	if (b->ob_size < 0)
		z->ob_size = -z->ob_size;
	return (object *) long_normalize(z);
}

static object *
long_div(v, w)
	longobject *v;
	register object *w;
{
	if (!is_longobject(w)) {
		err_badarg();
		return NULL;
	}
	return (object *) long_divrem(v, (longobject *)w, (longobject **)0);
}

static object *
long_rem(v, w)
	longobject *v;
	register object *w;
{
	longobject *div, *rem = NULL;
	if (!is_longobject(w)) {
		err_badarg();
		return NULL;
	}
	div = long_divrem(v, (longobject *)w, &rem);
	if (div == NULL) {
		XDECREF(rem);
		rem = NULL;
	}
	else {
		DECREF(div);
	}
	return (object *) rem;
}

/* The expression a mod b has the value a - b*floor(a/b).
   The divrem function gives the remainder after division of
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

static object *
long_divmod(v, w)
	longobject *v;
	register object *w;
{
	object *z;
	longobject *div, *rem;
	if (!is_longobject(w)) {
		err_badarg();
		return NULL;
	}
	div = long_divrem(v, (longobject *)w, &rem);
	if (div == NULL) {
		XDECREF(rem);
		return NULL;
	}
	if ((v->ob_size < 0) != (((longobject *)w)->ob_size < 0)) {
		longobject *temp;
		longobject *one;
		temp = (longobject *) long_add(rem, w);
		DECREF(rem);
		rem = temp;
		if (rem == NULL) {
			DECREF(div);
			return NULL;
		}
		one = (longobject *) newlongobject(1L);
		if (one == NULL ||
		    (temp = (longobject *) long_sub(div, one)) == NULL) {
			DECREF(rem);
			DECREF(div);
			return NULL;
		}
		DECREF(div);
		div = temp;
	}
	z = newtupleobject(2);
	if (z != NULL) {
		settupleitem(z, 0, (object *) div);
		settupleitem(z, 1, (object *) rem);
	}
	else {
		DECREF(div);
		DECREF(rem);
	}
	return z;
}

static object *
long_pow(v, w)
	longobject *v;
	register object *w;
{
	if (!is_longobject(w)) {
		err_badarg();
		return NULL;
	}
	err_setstr(SystemError, "long power not implemented");
	return NULL;
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
	int i = v->ob_size;
	if (i == 0)
		return long_pos(v);
	i = ABS(i);
	z = alloclongobject(i);
	if (z != NULL) {
		z->ob_size = - v->ob_size;
		while (--i >= 0)
			z->ob_digit[i] = v->ob_digit[i];
	}
	return (object *)z;
}

static object *
long_abs(v)
	longobject *v;
{
	if (v->ob_size < 0)
		return long_neg(v);
	else
		return long_pos(v);
}

static int
long_nonzero(v)
	longobject *v;
{
	return v->ob_size != 0;
}

static number_methods long_as_number = {
	long_add,	/*nb_add*/
	long_sub,	/*nb_subtract*/
	long_mul,	/*nb_multiply*/
	long_div,	/*nb_divide*/
	long_rem,	/*nb_remainder*/
	long_divmod,	/*nb_divmod*/
	long_pow,	/*nb_power*/
	long_neg,	/*nb_negative*/
	long_pos,	/*tp_positive*/
	long_abs,	/*tp_absolute*/
	long_nonzero,	/*tp_nonzero*/
};

typeobject Longtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"long int",
	sizeof(longobject) - sizeof(digit),
	sizeof(digit),
	long_dealloc,	/*tp_dealloc*/
	long_print,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	long_compare,	/*tp_compare*/
	long_repr,	/*tp_repr*/
	&long_as_number,/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};
