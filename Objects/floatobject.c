/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* Float object implementation */

/* XXX There should be overflow checks here, but it's hard to check
   for any kind of float exception without losing portability. */

#include "allobjects.h"
#include "modsupport.h"

#include <errno.h>
#include <ctype.h>
#include <math.h>

#ifdef i860
/* Cray APP has bogus definition of HUGE_VAL in <math.h> */
#undef HUGE_VAL
#endif

#ifdef HUGE_VAL
#define CHECK(x) if (errno != 0) ; \
	else if (-HUGE_VAL <= (x) && (x) <= HUGE_VAL) ; \
	else errno = ERANGE
#else
#define CHECK(x) /* Don't know how to check */
#endif

#ifndef macintosh
extern double fmod PROTO((double, double));
extern double pow PROTO((double, double));
#endif

object *
newfloatobject(fval)
	double fval;
{
	/* For efficiency, this code is copied from newobject() */
	register floatobject *op = (floatobject *) malloc(sizeof(floatobject));
	if (op == NULL)
		return err_nomem();
	op->ob_type = &Floattype;
	op->ob_fval = fval;
	NEWREF(op);
	return (object *) op;
}

static void
float_dealloc(op)
	object *op;
{
	DEL(op);
}

double
getfloatvalue(op)
	object *op;
{
	number_methods *nb;
	floatobject *fo;
	double val;
	
	if (op && is_floatobject(op))
		return GETFLOATVALUE((floatobject*) op);
	
	if (op == NULL || (nb = op->ob_type->tp_as_number) == NULL ||
	    nb->nb_float == NULL) {
		err_badarg();
		return -1;
	}
	
	fo = (floatobject*) (*nb->nb_float) (op);
	if (fo == NULL)
		return -1;
	if (!is_floatobject(fo)) {
		err_setstr(TypeError, "nb_float should return float object");
		return -1;
	}
	
	val = GETFLOATVALUE(fo);
	DECREF(fo);
	
	return val;
}

/* Methods */

void
float_buf_repr(buf, v)
	char *buf;
	floatobject *v;
{
	register char *cp;
	/* Subroutine for float_repr and float_print.
	   We want float numbers to be recognizable as such,
	   i.e., they should contain a decimal point or an exponent.
	   However, %g may print the number as an integer;
	   in such cases, we append ".0" to the string. */
	sprintf(buf, "%.12g", v->ob_fval);
	cp = buf;
	if (*cp == '-')
		cp++;
	for (; *cp != '\0'; cp++) {
		/* Any non-digit means it's not an integer;
		   this takes care of NAN and INF as well. */
		if (!isdigit(*cp))
			break;
	}
	if (*cp == '\0') {
		*cp++ = '.';
		*cp++ = '0';
		*cp++ = '\0';
	}
}

/* ARGSUSED */
static int
float_print(v, fp, flags)
	floatobject *v;
	FILE *fp;
	int flags; /* Not used but required by interface */
{
	char buf[100];
	float_buf_repr(buf, v);
	fputs(buf, fp);
	return 0;
}

static object *
float_repr(v)
	floatobject *v;
{
	char buf[100];
	float_buf_repr(buf, v);
	return newstringobject(buf);
}

static int
float_compare(v, w)
	floatobject *v, *w;
{
	double i = v->ob_fval;
	double j = w->ob_fval;
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long
float_hash(v)
	floatobject *v;
{
	double intpart, fractpart;
	int expo;
	long x;
	/* This is designed so that Python numbers with the same
	   value hash to the same value, otherwise comparisons
	   of mapping keys will turn out weird */

#ifdef MPW /* MPW C modf expects pointer to extended as second argument */
{
	extended e;
	fractpart = modf(v->ob_fval, &e);
	intpart = e;
}
#else
	fractpart = modf(v->ob_fval, &intpart);
#endif

	if (fractpart == 0.0) {
		if (intpart > 0x7fffffffL || -intpart > 0x7fffffffL) {
			/* Convert to long int and use its hash... */
			object *w = dnewlongobject(v->ob_fval);
			if (w == NULL)
				return -1;
			x = hashobject(w);
			DECREF(w);
			return x;
		}
		x = (long)intpart;
	}
	else {
		fractpart = frexp(fractpart, &expo);
		fractpart = fractpart*4294967296.0; /* 2**32 */
		x = (long) (intpart + fractpart) ^ expo; /* Rather arbitrary */
	}
	if (x == -1)
		x = -2;
	return x;
}

static object *
float_add(v, w)
	floatobject *v;
	floatobject *w;
{
	return newfloatobject(v->ob_fval + w->ob_fval);
}

static object *
float_sub(v, w)
	floatobject *v;
	floatobject *w;
{
	return newfloatobject(v->ob_fval - w->ob_fval);
}

static object *
float_mul(v, w)
	floatobject *v;
	floatobject *w;
{
	return newfloatobject(v->ob_fval * w->ob_fval);
}

static object *
float_div(v, w)
	floatobject *v;
	floatobject *w;
{
	if (w->ob_fval == 0) {
		err_setstr(ZeroDivisionError, "float division");
		return NULL;
	}
	return newfloatobject(v->ob_fval / w->ob_fval);
}

static object *
float_rem(v, w)
	floatobject *v;
	floatobject *w;
{
	double vx, wx;
	double /* div, */ mod;
	wx = w->ob_fval;
	if (wx == 0.0) {
		err_setstr(ZeroDivisionError, "float modulo");
		return NULL;
	}
	vx = v->ob_fval;
	mod = fmod(vx, wx);
	/* div = (vx - mod) / wx; */
	if (wx*mod < 0) {
		mod += wx;
		/* div -= 1.0; */
	}
	return newfloatobject(mod);
}

static object *
float_divmod(v, w)
	floatobject *v;
	floatobject *w;
{
	double vx, wx;
	double div, mod;
	object *t;
	wx = w->ob_fval;
	if (wx == 0.0) {
		err_setstr(ZeroDivisionError, "float divmod()");
		return NULL;
	}
	vx = v->ob_fval;
	mod = fmod(vx, wx);
	div = (vx - mod) / wx;
	if (wx*mod < 0) {
		mod += wx;
		div -= 1.0;
	}
	return mkvalue("(dd)", div, mod);
}

static object *
float_pow(v, w)
	floatobject *v;
	floatobject *w;
{
	double iv, iw, ix;
	iv = v->ob_fval;
	iw = w->ob_fval;
	/* Sort out special cases here instead of relying on pow() */
	if (iw == 0.0)
		return newfloatobject(1.0); /* x**0 is 1, even 0**0 */
	if (iv == 0.0) {
		if (iw < 0.0) {
			err_setstr(ValueError, "0.0 to the negative power");
			return NULL;
		}
		return newfloatobject(0.0);
	}
	if (iv < 0.0) {
		err_setstr(ValueError, "negative float to float power");
		return NULL;
	}
	errno = 0;
	ix = pow(iv, iw);
	CHECK(ix);
	if (errno != 0) {
		/* XXX could it be another type of error? */
		err_errno(OverflowError);
		return NULL;
	}
	return newfloatobject(ix);
}

static object *
float_neg(v)
	floatobject *v;
{
	return newfloatobject(-v->ob_fval);
}

static object *
float_pos(v)
	floatobject *v;
{
	INCREF(v);
	return (object *)v;
}

static object *
float_abs(v)
	floatobject *v;
{
	if (v->ob_fval < 0)
		return float_neg(v);
	else
		return float_pos(v);
}

static int
float_nonzero(v)
	floatobject *v;
{
	return v->ob_fval != 0.0;
}

static int
float_coerce(pv, pw)
	object **pv;
	object **pw;
{
	if (is_intobject(*pw)) {
		long x = getintvalue(*pw);
		*pw = newfloatobject((double)x);
		INCREF(*pv);
		return 0;
	}
	else if (is_longobject(*pw)) {
		*pw = newfloatobject(dgetlongvalue(*pw));
		INCREF(*pv);
		return 0;
	}
	return 1; /* Can't do it */
}

static object *
float_int(v)
	object *v;
{
	double x = getfloatvalue(v);
	/* XXX should check for overflow */
	/* XXX should define how we round */
	return newintobject((long)x);
}

static object *
float_long(v)
	object *v;
{
	double x = getfloatvalue(v);
	return dnewlongobject(x);
}

static object *
float_float(v)
	object *v;
{
	INCREF(v);
	return v;
}


static number_methods float_as_number = {
	(binaryfunc)float_add, /*nb_add*/
	(binaryfunc)float_sub, /*nb_subtract*/
	(binaryfunc)float_mul, /*nb_multiply*/
	(binaryfunc)float_div, /*nb_divide*/
	(binaryfunc)float_rem, /*nb_remainder*/
	(binaryfunc)float_divmod, /*nb_divmod*/
	(binaryfunc)float_pow, /*nb_power*/
	(unaryfunc)float_neg, /*nb_negative*/
	(unaryfunc)float_pos, /*nb_positive*/
	(unaryfunc)float_abs, /*nb_absolute*/
	(inquiry)float_nonzero, /*nb_nonzero*/
	0,		/*nb_invert*/
	0,		/*nb_lshift*/
	0,		/*nb_rshift*/
	0,		/*nb_and*/
	0,		/*nb_xor*/
	0,		/*nb_or*/
	(coercion)float_coerce, /*nb_coerce*/
	(unaryfunc)float_int, /*nb_int*/
	(unaryfunc)float_long, /*nb_long*/
	(unaryfunc)float_float, /*nb_float*/
	0,		/*nb_oct*/
	0,		/*nb_hex*/
};

typeobject Floattype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"float",
	sizeof(floatobject),
	0,
	(destructor)float_dealloc, /*tp_dealloc*/
	(printfunc)float_print, /*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)float_compare, /*tp_compare*/
	(reprfunc)float_repr, /*tp_repr*/
	&float_as_number,	/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc)float_hash, /*tp_hash*/
};
