/* Integer object implementation */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "intobject.h"
#include "stringobject.h"
#include "objimpl.h"
#include "errors.h"

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

object *
newintobject(ival)
	long ival;
{
	/* For efficiency, this code is copied from newobject() */
	register intobject *op = (intobject *) malloc(sizeof(intobject));
	if (op == NULL) {
		err_nomem();
	}
	else {
		NEWREF(op);
		op->ob_type = &Inttype;
		op->ob_ival = ival;
	}
	return (object *) op;
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

static void
intprint(v, fp, flags)
	intobject *v;
	FILE *fp;
	int flags;
{
	fprintf(fp, "%ld", v->ob_ival);
}

static object *
intrepr(v)
	intobject *v;
{
	char buf[20];
	sprintf(buf, "%ld", v->ob_ival);
	return newstringobject(buf);
}

static int
intcompare(v, w)
	intobject *v, *w;
{
	register long i = v->ob_ival;
	register long j = w->ob_ival;
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static object *
intadd(v, w)
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
intsub(v, w)
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
intmul(v, w)
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
intdiv(v, w)
	intobject *v;
	register object *w;
{
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	if (((intobject *)w) -> ob_ival == 0)
		err_zdiv();
	return newintobject(v->ob_ival / ((intobject *)w) -> ob_ival);
}

static object *
intrem(v, w)
	intobject *v;
	register object *w;
{
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	if (((intobject *)w) -> ob_ival == 0)
		err_zdiv();
	return newintobject(v->ob_ival % ((intobject *)w) -> ob_ival);
}

static object *
intpow(v, w)
	intobject *v;
	register object *w;
{
	register long iv, iw, ix;
	register int neg;
	if (!is_intobject(w)) {
		err_badarg();
		return NULL;
	}
	iv = v->ob_ival;
	iw = ((intobject *)w)->ob_ival;
	neg = 0;
	if (iw < 0)
		neg = 1, iw = -iw;
	ix = 1;
	for (; iw > 0; iw--)
		ix = ix * iv;
	if (neg) {
		if (ix == 0)
			err_zdiv();
		ix = 1/ix;
	}
	/* XXX How to check for overflow? */
	return newintobject(ix);
}

static object *
intneg(v)
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
intpos(v)
	intobject *v;
{
	INCREF(v);
	return (object *)v;
}

static number_methods int_as_number = {
	intadd,	/*tp_add*/
	intsub,	/*tp_subtract*/
	intmul,	/*tp_multiply*/
	intdiv,	/*tp_divide*/
	intrem,	/*tp_remainder*/
	intpow,	/*tp_power*/
	intneg,	/*tp_negate*/
	intpos,	/*tp_plus*/
};

typeobject Inttype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"int",
	sizeof(intobject),
	0,
	free,		/*tp_dealloc*/
	intprint,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	intcompare,	/*tp_compare*/
	intrepr,	/*tp_repr*/
	&int_as_number,	/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};
