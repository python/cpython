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

/* Math module -- standard C math library functions, pi and e */

#include "allobjects.h"

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "modsupport.h"

#include <math.h>

static int
getdoublearg(args, px)
	register object *args;
	double *px;
{
	if (args == NULL)
		return err_badarg();
	if (is_floatobject(args)) {
		*px = getfloatvalue(args);
		return 1;
	}
	if (is_intobject(args)) {
		*px = getintvalue(args);
		return 1;
	}
	return err_badarg();
}

static int
get2doublearg(args, px, py)
	register object *args;
	double *px, *py;
{
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2)
		return err_badarg();
	return getdoublearg(gettupleitem(args, 0), px) &&
		getdoublearg(gettupleitem(args, 1), py);
}

static object *
math_1(args, func)
	object *args;
	double (*func) FPROTO((double));
{
	double x;
	if (!getdoublearg(args, &x))
		return NULL;
	errno = 0;
	x = (*func)(x);
	if (errno != 0)
		return NULL;
	else
		return newfloatobject(x);
}

static object *
math_2(args, func)
	object *args;
	double (*func) FPROTO((double, double));
{
	double x, y;
	if (!get2doublearg(args, &x, &y))
		return NULL;
	errno = 0;
	x = (*func)(x, y);
	if (errno != 0)
		return NULL;
	else
		return newfloatobject(x);
}

#define FUNC1(stubname, func) \
	static object * stubname(self, args) object *self, *args; { \
		return math_1(args, func); \
	}

#define FUNC2(stubname, func) \
	static object * stubname(self, args) object *self, *args; { \
		return math_2(args, func); \
	}

FUNC1(math_acos, acos)
FUNC1(math_asin, asin)
FUNC1(math_atan, atan)
FUNC2(math_atan2, atan2)
FUNC1(math_ceil, ceil)
FUNC1(math_cos, cos)
FUNC1(math_cosh, cosh)
FUNC1(math_exp, exp)
FUNC1(math_fabs, fabs)
FUNC1(math_floor, floor)
#if 0
/* XXX This one is not in the Amoeba library yet, so what the heck... */
FUNC2(math_fmod, fmod)
#endif
FUNC1(math_log, log)
FUNC1(math_log10, log10)
FUNC2(math_pow, pow)
FUNC1(math_sin, sin)
FUNC1(math_sinh, sinh)
FUNC1(math_sqrt, sqrt)
FUNC1(math_tan, tan)
FUNC1(math_tanh, tanh)

#if 0
/* What about these? */
double	frexp(double x, int *i);
double	ldexp(double x, int n);
double	modf(double x, double *i);
#endif

static struct methodlist math_methods[] = {
	{"acos", math_acos},
	{"asin", math_asin},
	{"atan", math_atan},
	{"atan2", math_atan2},
	{"ceil", math_ceil},
	{"cos", math_cos},
	{"cosh", math_cosh},
	{"exp", math_exp},
	{"fabs", math_fabs},
	{"floor", math_floor},
#if 0
	{"fmod", math_fmod},
	{"frexp", math_freqp},
	{"ldexp", math_ldexp},
#endif
	{"log", math_log},
	{"log10", math_log10},
#if 0
	{"modf", math_modf},
#endif
	{"pow", math_pow},
	{"sin", math_sin},
	{"sinh", math_sinh},
	{"sqrt", math_sqrt},
	{"tan", math_tan},
	{"tanh", math_tanh},
	{NULL,		NULL}		/* sentinel */
};

void
initmath()
{
	object *m, *d, *v;
	
	m = initmodule("math", math_methods);
	d = getmoduledict(m);
	dictinsert(d, "pi", v = newfloatobject(atan(1.0) * 4.0));
	DECREF(v);
	dictinsert(d, "e", v = newfloatobject(exp(1.0)));
	DECREF(v);
}
