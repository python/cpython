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

/* Float object implementation */

/* XXX There should be overflow checks here, but it's hard to check
   for any kind of float exception without losing portability. */

#include "allobjects.h"

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include <ctype.h>
#include <math.h>

#ifndef THINK_C
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
	NEWREF(op);
	op->ob_type = &Floattype;
	op->ob_fval = fval;
	return (object *) op;
}

double
getfloatvalue(op)
	object *op;
{
	if (!is_floatobject(op)) {
		err_badarg();
		return -1;
	}
	else
		return ((floatobject *)op) -> ob_fval;
}

/* Methods */

static void
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

static void
float_print(v, fp, flags)
	floatobject *v;
	FILE *fp;
	int flags;
{
	char buf[100];
	float_buf_repr(buf, v);
	fputs(buf, fp);
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

static object *
float_add(v, w)
	floatobject *v;
	object *w;
{
	if (!is_floatobject(w)) {
		err_badarg();
		return NULL;
	}
	return newfloatobject(v->ob_fval + ((floatobject *)w) -> ob_fval);
}

static object *
float_sub(v, w)
	floatobject *v;
	object *w;
{
	if (!is_floatobject(w)) {
		err_badarg();
		return NULL;
	}
	return newfloatobject(v->ob_fval - ((floatobject *)w) -> ob_fval);
}

static object *
float_mul(v, w)
	floatobject *v;
	object *w;
{
	if (!is_floatobject(w)) {
		err_badarg();
		return NULL;
	}
	return newfloatobject(v->ob_fval * ((floatobject *)w) -> ob_fval);
}

static object *
float_div(v, w)
	floatobject *v;
	object *w;
{
	if (!is_floatobject(w)) {
		err_badarg();
		return NULL;
	}
	if (((floatobject *)w) -> ob_fval == 0) {
		err_setstr(ZeroDivisionError, "float division by zero");
		return NULL;
	}
	return newfloatobject(v->ob_fval / ((floatobject *)w) -> ob_fval);
}

static object *
float_rem(v, w)
	floatobject *v;
	object *w;
{
	double wx;
	if (!is_floatobject(w)) {
		err_badarg();
		return NULL;
	}
	wx = ((floatobject *)w) -> ob_fval;
	if (wx == 0.0) {
		err_setstr(ZeroDivisionError, "float division by zero");
		return NULL;
	}
	return newfloatobject(fmod(v->ob_fval, wx));
}

static object *
float_pow(v, w)
	floatobject *v;
	object *w;
{
	double iv, iw, ix;
	if (!is_floatobject(w)) {
		err_badarg();
		return NULL;
	}
	iv = v->ob_fval;
	iw = ((floatobject *)w)->ob_fval;
	if (iw == 0.0)
		return newfloatobject(1.0); /* x**0 is always 1, even 0**0 */
	errno = 0;
	ix = pow(iv, iw);
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
	return newfloatobject(v->ob_fval);
}

static number_methods float_as_number = {
	float_add,	/*tp_add*/
	float_sub,	/*tp_subtract*/
	float_mul,	/*tp_multiply*/
	float_div,	/*tp_divide*/
	float_rem,	/*tp_remainder*/
	float_pow,	/*tp_power*/
	float_neg,	/*tp_negate*/
	float_pos,	/*tp_plus*/
};

typeobject Floattype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"float",
	sizeof(floatobject),
	0,
	free,			/*tp_dealloc*/
	float_print,		/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	float_compare,		/*tp_compare*/
	float_repr,		/*tp_repr*/
	&float_as_number,	/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};

/*
XXX This is not enough.  Need:
- automatic casts for mixed arithmetic (3.1 * 4)
- mixed comparisons (!)
- look at other uses of ints that could be extended to floats
*/
