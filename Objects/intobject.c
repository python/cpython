/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

/* Integer object implementation */

#include "allobjects.h"
#include "modsupport.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef LONG_MAX
#define LONG_MAX 0X7FFFFFFFL
#endif

#ifndef LONG_MIN
#define LONG_MIN (-LONG_MAX-1)
#endif

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef LONG_BIT
#define LONG_BIT (CHAR_BIT * sizeof(long))
#endif

long
getmaxint()
{
	return LONG_MAX;	/* To initialize sys.maxint */
}

/* Standard Booleans */

intobject FalseObject = {
	OB_HEAD_INIT(&Inttype)
	0
};

intobject TrueObject = {
	OB_HEAD_INIT(&Inttype)
	1
};

static object *
err_ovf(msg)
	char *msg;
{
	err_setstr(OverflowError, msg);
	return NULL;
}

/* Integers are quite normal objects, to make object handling uniform.
   (Using odd pointers to represent integers would save much space
   but require extra checks for this special case throughout the code.)
   Since, a typical Python program spends much of its time allocating
   and deallocating integers, these operations should be very fast.
   Therefore we use a dedicated allocation scheme with a much lower
   overhead (in space and time) than straight malloc(): a simple
   dedicated free list, filled when necessary with memory from malloc().
*/

#define BLOCK_SIZE	1000	/* 1K less typical malloc overhead */
#define N_INTOBJECTS	(BLOCK_SIZE / sizeof(intobject))

static intobject *
fill_free_list()
{
	intobject *p, *q;
	p = NEW(intobject, N_INTOBJECTS);
	if (p == NULL)
		return (intobject *)err_nomem();
	q = p + N_INTOBJECTS;
	while (--q > p)
		*(intobject **)q = q-1;
	*(intobject **)q = NULL;
	return p + N_INTOBJECTS - 1;
}

static intobject *free_list = NULL;
#ifndef NSMALLPOSINTS
#define NSMALLPOSINTS		100
#endif
#ifndef NSMALLNEGINTS
#define NSMALLNEGINTS		1
#endif
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
/* References to small integers are saved in this array so that they
   can be shared.
   The integers that are saved are those in the range
   -NSMALLNEGINTS (inclusive) to NSMALLPOSINTS (not inclusive).
*/
static intobject *small_ints[NSMALLNEGINTS + NSMALLPOSINTS];
#endif
#ifdef COUNT_ALLOCS
int quick_int_allocs, quick_neg_int_allocs;
#endif

object *
newintobject(ival)
	long ival;
{
	register intobject *v;
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
	if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS &&
	    (v = small_ints[ival + NSMALLNEGINTS]) != NULL) {
		INCREF(v);
#ifdef COUNT_ALLOCS
		if (ival >= 0)
			quick_int_allocs++;
		else
			quick_neg_int_allocs++;
#endif
		return (object *) v;
	}
#endif
	if (free_list == NULL) {
		if ((free_list = fill_free_list()) == NULL)
			return NULL;
	}
	v = free_list;
	free_list = *(intobject **)free_list;
	v->ob_type = &Inttype;
	v->ob_ival = ival;
	NEWREF(v);
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
	if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS) {
		/* save this one for a following allocation */
		INCREF(v);
		small_ints[ival + NSMALLNEGINTS] = v;
	}
#endif
	return (object *) v;
}

static void
int_dealloc(v)
	intobject *v;
{
	*(intobject **)v = free_list;
	free_list = v;
}

long
getintvalue(op)
	register object *op;
{
	number_methods *nb;
	intobject *io;
	long val;
	
	if (op && is_intobject(op))
		return GETINTVALUE((intobject*) op);
	
	if (op == NULL || (nb = op->ob_type->tp_as_number) == NULL ||
	    nb->nb_int == NULL) {
		err_badarg();
		return -1;
	}
	
	io = (intobject*) (*nb->nb_int) (op);
	if (io == NULL)
		return -1;
	if (!is_intobject(io)) {
		err_setstr(TypeError, "nb_int should return int object");
		return -1;
	}
	
	val = GETINTVALUE(io);
	DECREF(io);
	
	return val;
}

/* Methods */

/* ARGSUSED */
static int
int_print(v, fp, flags)
	intobject *v;
	FILE *fp;
	int flags; /* Not used but required by interface */
{
	fprintf(fp, "%ld", v->ob_ival);
	return 0;
}

static object *
int_repr(v)
	intobject *v;
{
	char buf[20];
	sprintf(buf, "%ld", v->ob_ival);
	return newstringobject(buf);
}

static int
int_compare(v, w)
	intobject *v, *w;
{
	register long i = v->ob_ival;
	register long j = w->ob_ival;
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long
int_hash(v)
	intobject *v;
{
	long x = v -> ob_ival;
	if (x == -1)
		x = -2;
	return x;
}

static object *
int_add(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b, x;
	a = v->ob_ival;
	b = w->ob_ival;
	x = a + b;
	if ((x^a) < 0 && (x^b) < 0)
		return err_ovf("integer addition");
	return newintobject(x);
}

static object *
int_sub(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b, x;
	a = v->ob_ival;
	b = w->ob_ival;
	x = a - b;
	if ((x^a) < 0 && (x^~b) < 0)
		return err_ovf("integer subtraction");
	return newintobject(x);
}

/*
Integer overflow checking used to be done using a double, but on 64
bit machines (where both long and double are 64 bit) this fails
because the double doesn't have enouvg precision.  John Tromp suggests
the following algorithm:

Suppose again we normalize a and b to be nonnegative.
Let ah and al (bh and bl) be the high and low 32 bits of a (b, resp.).
Now we test ah and bh against zero and get essentially 3 possible outcomes.

1) both ah and bh > 0 : then report overflow

2) both ah and bh = 0 : then compute a*b and report overflow if it comes out
                        negative

3) ah > 0 and bh = 0  : compute ah*bl and report overflow if it's >= 2^31
                        compute al*bl and report overflow if it's negative
                        add (ah*bl)<<32 to al*bl and report overflow if
                        it's negative

In case of no overflow the result is then negated if necessary.

The majority of cases will be 2), in which case this method is the same as
what I suggested before. If multiplication is expensive enough, then the
other method is faster on case 3), but also more work to program, so I
guess the above is the preferred solution.

*/

static object *
int_mul(v, w)
	intobject *v;
	intobject *w;
{
	long a, b, ah, bh, x, y;
	int s = 1;

	a = v->ob_ival;
	b = w->ob_ival;
	ah = a >> (LONG_BIT/2);
	bh = b >> (LONG_BIT/2);

	/* Quick test for common case: two small positive ints */

	if (ah == 0 && bh == 0) {
		x = a*b;
		if (x < 0)
			goto bad;
		return newintobject(x);
	}

	/* Arrange that a >= b >= 0 */

	if (a < 0) {
		a = -a;
		if (a < 0) {
			/* Largest negative */
			if (b == 0 || b == 1) {
				x = a*b;
				goto ok;
			}
			else
				goto bad;
		}
		s = -s;
		ah = a >> (LONG_BIT/2);
	}
	if (b < 0) {
		b = -b;
		if (b < 0) {
			/* Largest negative */
			if (a == 0 || a == 1 && s == 1) {
				x = a*b;
				goto ok;
			}
			else
				goto bad;
		}
		s = -s;
		bh = b >> (LONG_BIT/2);
	}

	/* 1) both ah and bh > 0 : then report overflow */

	if (ah != 0 && bh != 0)
		goto bad;

	/* 2) both ah and bh = 0 : then compute a*b and report
				   overflow if it comes out negative */

	if (ah == 0 && bh == 0) {
		x = a*b;
		if (x < 0)
			goto bad;
		return newintobject(x*s);
	}

	if (a < b) {
		/* Swap */
		x = a;
		a = b;
		b = x;
		ah = bh;
		/* bh not used beyond this point */
	}

	/* 3) ah > 0 and bh = 0  : compute ah*bl and report overflow if
				   it's >= 2^31
                        compute al*bl and report overflow if it's negative
                        add (ah*bl)<<32 to al*bl and report overflow if
                        it's negative
			(NB b == bl in this case, and we make a = al) */

	y = ah*b;
	if (y >= (1L << (LONG_BIT/2)))
		goto bad;
	a &= (1L << (LONG_BIT/2)) - 1;
	x = a*b;
	if (x < 0)
		goto bad;
	x += y << LONG_BIT/2;
	if (x < 0)
		goto bad;
 ok:
	return newintobject(x * s);

 bad:
	return err_ovf("integer multiplication");
}

static int
i_divmod(x, y, p_xdivy, p_xmody)
	register intobject *x, *y;
	long *p_xdivy, *p_xmody;
{
	long xi = x->ob_ival;
	long yi = y->ob_ival;
	long xdivy, xmody;
	
	if (yi == 0) {
		err_setstr(ZeroDivisionError, "integer division or modulo");
		return -1;
	}
	if (yi < 0) {
		if (xi < 0)
			xdivy = -xi / -yi;
		else
			xdivy = - (xi / -yi);
	}
	else {
		if (xi < 0)
			xdivy = - (-xi / yi);
		else
			xdivy = xi / yi;
	}
	xmody = xi - xdivy*yi;
	if (xmody < 0 && yi > 0 || xmody > 0 && yi < 0) {
		xmody += yi;
		xdivy -= 1;
	}
	*p_xdivy = xdivy;
	*p_xmody = xmody;
	return 0;
}

static object *
int_div(x, y)
	intobject *x;
	intobject *y;
{
	long d, m;
	if (i_divmod(x, y, &d, &m) < 0)
		return NULL;
	return newintobject(d);
}

static object *
int_mod(x, y)
	intobject *x;
	intobject *y;
{
	long d, m;
	if (i_divmod(x, y, &d, &m) < 0)
		return NULL;
	return newintobject(m);
}

static object *
int_divmod(x, y)
	intobject *x;
	intobject *y;
{
	long d, m;
	if (i_divmod(x, y, &d, &m) < 0)
		return NULL;
	return mkvalue("(ll)", d, m);
}

static object *
int_pow(v, w, z)
	intobject *v;
	intobject *w;
	intobject *z;
{
#if 1
	register long iv, iw, iz, ix, temp, prev;
 	int zset = 0;
	iv = v->ob_ival;
	iw = w->ob_ival;
	if (iw < 0) {
		err_setstr(ValueError, "integer to the negative power");
		return NULL;
	}
 	if ((object *)z != None) {
		iz = z->ob_ival;
	 	zset = 1;
	}
	/*
	 * XXX: The original exponentiation code stopped looping
	 * when temp hit zero; this code will continue onwards
	 * unnecessarily, but at least it won't cause any errors.
	 * Hopefully the speed improvement from the fast exponentiation
	 * will compensate for the slight inefficiency.
	 * XXX: Better handling of overflows is desperately needed.
	 */
 	temp = iv;
	ix = 1;
	while (iw > 0) {
	 	prev = ix;	/* Save value for overflow check */
	 	if (iw & 1) {	
		 	ix = ix*temp;
			if (temp == 0)
				break; /* Avoid ix / 0 */
			if (ix / temp != prev)
				return err_ovf("integer pow()");
		}
	 	iw >>= 1;	/* Shift exponent down by 1 bit */
	        if (iw==0) break;
	 	prev = temp;
	 	temp *= temp;	/* Square the value of temp */
	 	if (prev!=0 && temp/prev!=prev)
			return err_ovf("integer pow()");
	 	if (zset) {
			/* If we did a multiplication, perform a modulo */
		 	ix = ix % iz;
		 	temp = temp % iz;
		}
	}
	if (zset) {
	 	object *t1, *t2;
	 	long int div, mod;
	 	t1=newintobject(ix); 
		t2=newintobject(iz);
	 	if (t1==NULL || t2==NULL ||
	 		i_divmod((intobject *)t1, (intobject *)t2, &div, &mod)<0) {
		 	XDECREF(t1);
		 	XDECREF(t2);
			return(NULL);
		}
		DECREF(t1);
		DECREF(t2);
	 	ix=mod;
	}
	return newintobject(ix);
#else
	register long iv, iw, ix;
	iv = v->ob_ival;
	iw = w->ob_ival;
	if (iw < 0) {
		err_setstr(ValueError, "integer to the negative power");
		return NULL;
	}
	if ((object *)z != None) {
		err_setstr(TypeError, "pow(int, int, int) not yet supported");
		return NULL;
	}
	ix = 1;
	while (--iw >= 0) {
		long prev = ix;
		ix = ix * iv;
		if (iv == 0)
			break; /* 0 to some power -- avoid ix / 0 */
		if (ix / iv != prev)
			return err_ovf("integer pow()");
	}
	return newintobject(ix);
#endif
}				

static object *
int_neg(v)
	intobject *v;
{
	register long a, x;
	a = v->ob_ival;
	x = -a;
	if (a < 0 && x < 0)
		return err_ovf("integer negation");
	return newintobject(x);
}

static object *
int_pos(v)
	intobject *v;
{
	INCREF(v);
	return (object *)v;
}

static object *
int_abs(v)
	intobject *v;
{
	if (v->ob_ival >= 0)
		return int_pos(v);
	else
		return int_neg(v);
}

static int
int_nonzero(v)
	intobject *v;
{
	return v->ob_ival != 0;
}

static object *
int_invert(v)
	intobject *v;
{
	return newintobject(~v->ob_ival);
}

static object *
int_lshift(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	if (b < 0) {
		err_setstr(ValueError, "negative shift count");
		return NULL;
	}
	if (a == 0 || b == 0) {
		INCREF(v);
		return (object *) v;
	}
	if (b >= LONG_BIT) {
		return newintobject(0L);
	}
	a = (unsigned long)a << b;
	return newintobject(a);
}

static object *
int_rshift(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	if (b < 0) {
		err_setstr(ValueError, "negative shift count");
		return NULL;
	}
	if (a == 0 || b == 0) {
		INCREF(v);
		return (object *) v;
	}
	if (b >= LONG_BIT) {
		if (a < 0)
			a = -1;
		else
			a = 0;
	}
	else {
		if (a < 0)
			a = ~( ~(unsigned long)a >> b );
		else
			a = (unsigned long)a >> b;
	}
	return newintobject(a);
}

static object *
int_and(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	return newintobject(a & b);
}

static object *
int_xor(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	return newintobject(a ^ b);
}

static object *
int_or(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	return newintobject(a | b);
}

static object *
int_int(v)
	intobject *v;
{
	INCREF(v);
	return (object *)v;
}

static object *
int_long(v)
	intobject *v;
{
	return newlongobject((v -> ob_ival));
}

static object *
int_float(v)
	intobject *v;
{
	return newfloatobject((double)(v -> ob_ival));
}

static object *
int_oct(v)
	intobject *v;
{
	char buf[20];
	long x = v -> ob_ival;
	if (x == 0)
		strcpy(buf, "0");
	else if (x > 0)
		sprintf(buf, "0%lo", x);
	else
		sprintf(buf, "-0%lo", -x);
	return newstringobject(buf);
}

static object *
int_hex(v)
	intobject *v;
{
	char buf[20];
	long x = v -> ob_ival;
	if (x >= 0)
		sprintf(buf, "0x%lx", x);
	else
		sprintf(buf, "-0x%lx", -x);
	return newstringobject(buf);
}

static number_methods int_as_number = {
	(binaryfunc)int_add, /*nb_add*/
	(binaryfunc)int_sub, /*nb_subtract*/
	(binaryfunc)int_mul, /*nb_multiply*/
	(binaryfunc)int_div, /*nb_divide*/
	(binaryfunc)int_mod, /*nb_remainder*/
	(binaryfunc)int_divmod, /*nb_divmod*/
	(ternaryfunc)int_pow, /*nb_power*/
	(unaryfunc)int_neg, /*nb_negative*/
	(unaryfunc)int_pos, /*nb_positive*/
	(unaryfunc)int_abs, /*nb_absolute*/
	(inquiry)int_nonzero, /*nb_nonzero*/
	(unaryfunc)int_invert, /*nb_invert*/
	(binaryfunc)int_lshift, /*nb_lshift*/
	(binaryfunc)int_rshift, /*nb_rshift*/
	(binaryfunc)int_and, /*nb_and*/
	(binaryfunc)int_xor, /*nb_xor*/
	(binaryfunc)int_or, /*nb_or*/
	0,		/*nb_coerce*/
	(unaryfunc)int_int, /*nb_int*/
	(unaryfunc)int_long, /*nb_long*/
	(unaryfunc)int_float, /*nb_float*/
	(unaryfunc)int_oct, /*nb_oct*/
	(unaryfunc)int_hex, /*nb_hex*/
};

typeobject Inttype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"int",
	sizeof(intobject),
	0,
	(destructor)int_dealloc, /*tp_dealloc*/
	(printfunc)int_print, /*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)int_compare, /*tp_compare*/
	(reprfunc)int_repr, /*tp_repr*/
	&int_as_number,	/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)int_hash, /*tp_hash*/
};
