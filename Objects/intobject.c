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

static object *
int_mul(v, w)
	intobject *v;
	intobject *w;
{
	register long a, b;
	double x;
	a = v->ob_ival;
	b = w->ob_ival;
	x = (double)a * (double)b;
	if (x > 0x7fffffff || x < (double) (long) 0x80000000)
		return err_ovf("integer multiplication");
	return newintobject(a * b);
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
	object *v, *v0, *v1;
	long d, m;
	if (i_divmod(x, y, &d, &m) < 0)
		return NULL;
	v = newtupleobject(2);
	v0 = newintobject(d);
	v1 = newintobject(m);
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
	intobject *w;
{
	register long iv, iw, ix;
	iv = v->ob_ival;
	iw = w->ob_ival;
	if (iw < 0) {
		err_setstr(ValueError, "integer to the negative power");
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
	if (b >= 32) {
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
	if (b >= 32) {
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

static number_methods int_as_number = {
	int_add,	/*nb_add*/
	int_sub,	/*nb_subtract*/
	int_mul,	/*nb_multiply*/
	int_div,	/*nb_divide*/
	int_mod,	/*nb_remainder*/
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
