/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Math module -- standard C math library functions, pi and e */

#include "Python.h"

#include "mymath.h"

#ifndef _MSC_VER
#ifndef __STDC__
extern double fmod Py_PROTO((double, double));
extern double frexp Py_PROTO((double, int *));
extern double ldexp Py_PROTO((double, int));
extern double modf Py_PROTO((double, double *));
#endif /* __STDC__ */
#endif /* _MSC_VER */


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

static PyObject *
math_error()
{
	if (errno == EDOM)
		PyErr_SetString(PyExc_ValueError, "math domain error");
	else if (errno == ERANGE)
		PyErr_SetString(PyExc_OverflowError, "math range error");
	else
                /* Unexpected math error */
		PyErr_SetFromErrno(PyExc_ValueError);
	return NULL;
}

static PyObject *
math_1(args, func)
	PyObject *args;
	double (*func) Py_FPROTO((double));
{
	double x;
	if (!  PyArg_Parse(args, "d", &x))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_1", return 0)
	x = (*func)(x);
	PyFPE_END_PROTECT
	CHECK(x);
	if (errno != 0)
		return math_error();
	else
		return PyFloat_FromDouble(x);
}

static PyObject *
math_2(args, func)
	PyObject *args;
	double (*func) Py_FPROTO((double, double));
{
	double x, y;
	if (! PyArg_Parse(args, "(dd)", &x, &y))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_2", return 0)
	x = (*func)(x, y);
	PyFPE_END_PROTECT
	CHECK(x);
	if (errno != 0)
		return math_error();
	else
		return PyFloat_FromDouble(x);
}

#define FUNC1(stubname, func) \
	static PyObject * stubname(self, args) PyObject *self, *args; { \
		return math_1(args, func); \
	}

#define FUNC2(stubname, func) \
	static PyObject * stubname(self, args) PyObject *self, *args; { \
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
#ifdef __MWERKS__
double myfabs(double x) { return fabs(x); }
FUNC1(math_fabs, myfabs)
#else
FUNC1(math_fabs, fabs)
#endif
FUNC1(math_floor, floor)
FUNC2(math_fmod, fmod)
FUNC2(math_hypot, hypot)
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


static PyObject *
math_frexp(self, args)
	PyObject *self;
	PyObject *args;
{
	double x;
	int i;
	if (! PyArg_Parse(args, "d", &x))
		return NULL;
	errno = 0;
	x = frexp(x, &i);
	CHECK(x);
	if (errno != 0)
		return math_error();
	return Py_BuildValue("(di)", x, i);
}

static PyObject *
math_ldexp(self, args)
	PyObject *self;
	PyObject *args;
{
	double x, y;
	/* Cheat -- allow float as second argument */
        if (! PyArg_Parse(args, "(dd)", &x, &y))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("ldexp", return 0)
	x = ldexp(x, (int)y);
	PyFPE_END_PROTECT
	CHECK(x);
	if (errno != 0)
		return math_error();
	else
		return PyFloat_FromDouble(x);
}

static PyObject *
math_modf(self, args)
	PyObject *self;
	PyObject *args;
{
	double x, y;
	if (! PyArg_Parse(args, "d", &x))
		return NULL;
	errno = 0;
#ifdef MPW /* MPW C modf expects pointer to extended as second argument */
{
	extended e;
	x = modf(x, &e);
	y = e;
}
#else
	x = modf(x, &y);
#endif
	CHECK(x);
	if (errno != 0)
		return math_error();
	return Py_BuildValue("(dd)", x, y);
}

static PyMethodDef math_methods[] = {
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
	{"hypot", math_hypot},
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
	PyObject *m, *d, *v;
	
	m = Py_InitModule("math", math_methods);
	d = PyModule_GetDict(m);

        if (!(v = PyFloat_FromDouble(atan(1.0) * 4.0)))
                goto finally;
	if (PyDict_SetItemString(d, "pi", v) < 0)
                goto finally;
	Py_DECREF(v);

        if (!(v = PyFloat_FromDouble(exp(1.0))))
                goto finally;
	if (PyDict_SetItemString(d, "e", v) < 0)
                goto finally;
	Py_DECREF(v);
	return;

  finally:
        Py_FatalError("can't initialize math module");
}
