/* Float object implementation */

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include "PROTO.h"
#include "object.h"
#include "floatobject.h"
#include "stringobject.h"
#include "objimpl.h"

object *
newfloatobject(fval)
	double fval;
{
	/* For efficiency, this code is copied from newobject() */
	register floatobject *op = (floatobject *) malloc(sizeof(floatobject));
	if (op == NULL) {
		errno = ENOMEM;
	}
	else {
		NEWREF(op);
		op->ob_type = &Floattype;
		op->ob_fval = fval;
	}
	return (object *) op;
}

double
getfloatvalue(op)
	object *op;
{
	if (!is_floatobject(op)) {
		errno = EBADF;
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
		errno = EINVAL;
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
		errno = EINVAL;
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
		errno = EINVAL;
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
		errno = EINVAL;
		return NULL;
	}
	if (((floatobject *)w) -> ob_fval == 0) {
		errno = EDOM;
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
	extern double fmod();
	if (!is_floatobject(w)) {
		errno = EINVAL;
		return NULL;
	}
	wx = ((floatobject *)w) -> ob_fval;
	if (wx == 0.0) {
		errno = EDOM;
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
	extern double pow();
	if (!is_floatobject(w)) {
		errno = EINVAL;
		return NULL;
	}
	iv = v->ob_fval;
	iw = ((floatobject *)w)->ob_fval;
	errno = 0;
	ix = pow(iv, iw);
	if (errno != 0)
		return NULL;
	else
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
