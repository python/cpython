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

/* Math module -- standard C math library functions, pi and e */

#include "allobjects.h"

#include <errno.h>

#include "modsupport.h"

#define getdoublearg(v, a) getargs(v, "d", a)
#define get2doublearg(v, a, b) getargs(v, "(dd)", a, b)

#include <math.h>

#ifdef i860
/* Cray APP has bogus definition of HUGE_VAL in <math.h> */
#undef HUGE_VAL
#endif

#ifndef __STDC__
extern double fmod PROTO((double, double));
#endif

#ifdef HUGE_VAL
#define CHECK(x) if (errno != 0) ; \
	else if (-HUGE_VAL <= (x) && (x) <= HUGE_VAL) ; \
	else errno = ERANGE
#else
#define CHECK(x) /* Don't know how to check */
#endif

static object *
math_error()
{
	if (errno == EDOM)
		err_setstr(ValueError, "math domain error");
	else if (errno == ERANGE)
		err_setstr(OverflowError, "math range error");
	else
		err_errno(ValueError); /* Unexpected math error */
	return NULL;
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
	CHECK(x);
	if (errno != 0)
		return math_error();
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
	CHECK(x);
	if (errno != 0)
		return math_error();
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
FUNC2(math_fmod, fmod)
FUNC1(math_log, log)
FUNC1(math_log10, log10)
#ifdef MPW_3_1 /* This hack is needed for MPW 3.1 but not for 3.2 ... */
FUNC2(math_pow, power)
#else
FUNC2(math_pow, pow)
#endif
FUNC1(math_sin, sin)
FUNC1(math_sinh, sinh)
FUNC1(math_sqrt, sqrt)
FUNC1(math_tan, tan)
FUNC1(math_tanh, tanh)

double	frexp PROTO((double, int *));
double	ldexp PROTO((double, int));
double	modf PROTO((double, double *));

static object *
math_frexp(self, args)
	object *self;
	object *args;
{
	double x;
	int i;
	if (!getdoublearg(args, &x))
		return NULL;
	errno = 0;
	x = frexp(x, &i);
	CHECK(x);
	if (errno != 0)
		return math_error();
	return mkvalue("(di)", x, i);
}

static object *
math_ldexp(self, args)
	object *self;
	object *args;
{
	double x, y;
	/* Cheat -- allow float as second argument */
	if (!get2doublearg(args, &x, &y))
		return NULL;
	errno = 0;
	x = ldexp(x, (int)y);
	CHECK(x);
	if (errno != 0)
		return math_error();
	else
		return newfloatobject(x);
}

static object *
math_modf(self, args)
	object *self;
	object *args;
{
	double x, y;
	if (!getdoublearg(args, &x))
		return NULL;
	errno = 0;
	x = modf(x, &y);
	CHECK(x);
	if (errno != 0)
		return math_error();
	return mkvalue("(dd)", x, y);
}

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
	{"fmod", math_fmod},
	{"frexp", math_frexp},
	{"ldexp", math_ldexp},
	{"log", math_log},
	{"log10", math_log10},
	{"modf", math_modf},
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
