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

/* Integer object implementation */

#include "allobjects.h"

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
err_ovf()
{
	err_setstr(OverflowError, "integer overflow");
	return NULL;
}

static object *
err_zdiv()
{
	err_setstr(ZeroDivisionError, "integer division by zero");
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

object *
newintobject(ival)
	long ival;
{
	register intobject *v;
	if (free_list == NULL) {
		if ((free_list = fill_free_list()) == NULL)
			return NULL;
	}
	v = free_list;
	free_list = *(intobject **)free_list;
	NEWREF(v);
	v->ob_type = &Inttype;
	v->ob_ival = ival;
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
	if (!is_intobject(op)) {
		err_badcall();
		return -1;
	}
	else
		return ((intobject *)op) -> ob_ival;
}

/* Methods */

static int
int_print(v, fp, flags)
	intobject *v;
	FILE *fp;
	int flags;
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

static object *
int_add(v, w)
	intobject *v;
	register object *w;
{
	register long a, b, x;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	x = a + b;
	if ((x^a) < 0 && (x^b) < 0)
		return err_ovf();
	return newintobject(x);
}

static object *
int_sub(v, w)
	intobject *v;
	register object *w;
{
	register long a, b, x;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	x = a - b;
	if ((x^a) < 0 && (x^~b) < 0)
		return err_ovf();
	return newintobject(x);
}

static object *
int_mul(v, w)
	intobject *v;
	register object *w;
{
	register long a, b;
	double x;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	x = (double)a * (double)b;
	if (x > 0x7fffffff || x < (double) (long) 0x80000000)
		return err_ovf();
	return newintobject(a * b);
}

static object *
int_div(v, w)
	intobject *v;
	register object *w;
{
	register long a, b, x;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	if (((intobject *)w) -> ob_ival == 0)
		return err_zdiv();
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	/* Make sure we always truncate towards zero */
	/* XXX What if a == -0x80000000? */
	if (a < 0) {
		if (b < 0)
			x = -a / -b;
		else
			x = -(-a / b);
	}
	else {
		if (b < 0)
			x = -(a / -b);
		else
			x = a / b;
	}
	return newintobject(x);
}

static object *
int_rem(v, w)
	intobject *v;
	register object *w;
{
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	if (((intobject *)w) -> ob_ival == 0)
		return err_zdiv();
	/* XXX Need to fix this similar to int_div */
	return newintobject(v->ob_ival % ((intobject *)w) -> ob_ival);
}

static object *
int_divmod(x, y)
	intobject *x;
	register object *y;
{
	object *v, *v0, *v1;
	long xi, yi, xdivy, xmody;
	if (!is_intobject(y)) {
		err_badarg();
		return NULL;
	}
	xi = x->ob_ival;
	yi = getintvalue(y);
	if (yi == 0)
		return err_zdiv();
	if (yi < 0) {
		xdivy = -xi / -yi;
	}
	else {
		xdivy = xi / yi;
	}
	xmody = xi - xdivy*yi;
	if (xmody < 0 && yi > 0 || xmody > 0 && yi < 0) {
		xmody += yi;
		xdivy -= 1;
	}
	v = newtupleobject(2);
	v0 = newintobject(xdivy);
	v1 = newintobject(xmody);
	if (v == NULL || v0 == NULL || v1 == NULL ||
		settupleitem(v, 0, v0) != 0 ||
		settupleitem(v, 1, v1) != 0) {
		XDECREF(v);
		XDECREF(v0);
		XDECREF(v1);
		v = NULL;
	}
	return v;
}

static object *
int_pow(v, w)
	intobject *v;
	register object *w;
{
	register long iv, iw, ix;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	iv = v->ob_ival;
	iw = ((intobject *)w)->ob_ival;
	if (iw < 0) {
		err_setstr(RuntimeError, "integer to the negative power");
		return NULL;
	}
	ix = 1;
	while (--iw >= 0) {
		long prev = ix;
		ix = ix * iv;
		if (iv == 0)
			break; /* 0 to some power -- avoid ix / 0 */
		if (ix / iv != prev)
			return err_ovf();
	}
	return newintobject(ix);
}

static object *
int_neg(v)
	intobject *v;
{
	register long a, x;
	a = v->ob_ival;
	x = -a;
	if (a < 0 && x < 0)
		return err_ovf();
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
	register object *w;
{
	register long a, b;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	return newintobject((unsigned long)a << b);
}

static object *
int_rshift(v, w)
	intobject *v;
	register object *w;
{
	register long a, b;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	return newintobject((unsigned long)a >> b);
}

static object *
int_and(v, w)
	intobject *v;
	register object *w;
{
	register long a, b;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	return newintobject(a & b);
}

static object *
int_xor(v, w)
	intobject *v;
	register object *w;
{
	register long a, b;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	return newintobject(a ^ b);
}

static object *
int_or(v, w)
	intobject *v;
	register object *w;
{
	register long a, b;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	a = v->ob_ival;
	b = ((intobject *)w) -> ob_ival;
	return newintobject(a | b);
}

static number_methods int_as_number = {
	int_add,	/*nb_add*/
	int_sub,	/*nb_subtract*/
	int_mul,	/*nb_multiply*/
	int_div,	/*nb_divide*/
	int_rem,	/*nb_remainder*/
	int_divmod,	/*nb_divmod*/
	int_pow,	/*nb_power*/
	int_neg,	/*nb_negative*/
	int_pos,	/*nb_positive*/
	int_abs,	/*nb_absolute*/
	int_nonzero,	/*nb_nonzero*/
	int_invert,	/*nb_invert*/
	int_lshift,	/*nb_lshift*/
	int_rshift,	/*nb_rshift*/
	int_and,	/*nb_and*/
	int_xor,	/*nb_xor*/
	int_or,		/*nb_or*/
};

typeobject Inttype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"int",
	sizeof(intobject),
	0,
	int_dealloc,	/*tp_dealloc*/
	int_print,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	int_compare,	/*tp_compare*/
	int_repr,	/*tp_repr*/
	&int_as_number,	/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};
