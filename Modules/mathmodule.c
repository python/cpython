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
	PyFPE_END_PROTECT(x)
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
	PyFPE_END_PROTECT(x)
	CHECK(x);
	if (errno != 0)
		return math_error();
	else
		return PyFloat_FromDouble(x);
}

#define FUNC1(stubname, func, docstring_name, docstring) \
	static PyObject * stubname(self, args) PyObject *self, *args; { \
		return math_1(args, func); \
	}\
        static char docstring_name [] = docstring;

#define FUNC2(stubname, func, docstring_name, docstring) \
	static PyObject * stubname(self, args) PyObject *self, *args; { \
		return math_2(args, func); \
	}\
        static char docstring_name [] = docstring;

FUNC1(math_acos, acos, math_acos_doc,
      "acos(x)\n\nReturn the arc cosine of x.")
FUNC1(math_asin, asin, math_asin_doc,
      "asin(x)\n\nReturn the arc sine of x.")
FUNC1(math_atan, atan, math_atan_doc,
      "atan(x)\n\nReturn the arc tangent of x.")
FUNC2(math_atan2, atan2, math_atan2_doc,
      "atan2(x)\n\nReturn atan(x /y).")
FUNC1(math_ceil, ceil, math_ceil_doc,
      "ceil(x)\n\nReturn the ceiling of x as a real.")
FUNC1(math_cos, cos, math_cos_doc,
      "cos(x)\n\nReturn the cosine of x.")
FUNC1(math_cosh, cosh, math_cosh_doc,
      "cosh(x)\n\nReturn the hyperbolic cosine of x.")
FUNC1(math_exp, exp, math_exp_doc,
      "exp(x)\n\nReturn e raised to the power of x.")
FUNC1(math_fabs, fabs, math_fabs_doc,
      "fabs(x)\n\nReturn the absolute value of the real x.")
FUNC1(math_floor, floor, math_floor_doc,
      "floor(x)\n\nReturn the floor of x as a real.")
FUNC2(math_fmod, fmod, math_fmod_doc,
      "fmod(x,y)\n\nReturn x % y.")
FUNC2(math_hypot, hypot, math_hypot_doc,
      "hypot(x,y)\n\nReturn the Euclidean distance, sqrt(x*x + y*y).")
FUNC1(math_log, log, math_log_doc,
      "log(x)\n\nReturn the natural logarithm of x.")
FUNC1(math_log10, log10, math_log10_doc,
      "log10(x)\n\nReturn the base-10 logarithm of x.")
#ifdef MPW_3_1 /* This hack is needed for MPW 3.1 but not for 3.2 ... */
FUNC2(math_pow, power, math_pow_doc,
      "power(x,y)\n\nReturn x**y.")
#else
FUNC2(math_pow, pow, math_pow_doc,
      "pow(x,y)\n\nReturn x**y.")
#endif
FUNC1(math_sin, sin, math_sin_doc,
      "sin(x)\n\nReturn the sine of x.")
FUNC1(math_sinh, sinh, math_sinh_doc,
      "sinh(x)\n\nReturn the hyperbolic sine of x.")
FUNC1(math_sqrt, sqrt, math_sqrt_doc,
      "sqrt(x)\n\nReturn the square root of x.")
FUNC1(math_tan, tan, math_tan_doc,
      "tan(x)\n\nReturn the tangent of x.")
FUNC1(math_tanh, tanh, math_tanh_doc,
      "tanh(x)\n\nReturn the hyperbolic tangent of x.")


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

static char math_frexp_doc [] =
"frexp(x)\n\
\n\
Return the matissa and exponent for x. The mantissa is positive.";


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
	PyFPE_END_PROTECT(x)
	CHECK(x);
	if (errno != 0)
		return math_error();
	else
		return PyFloat_FromDouble(x);
}

static char math_ldexp_doc [] = 
"ldexp_doc(x, i)\n\
\n\
Return x * (2**i).";


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

static char math_modf_doc [] =
"modf(x)\n\
\n\
Return the fractional and integer parts of x. Both results carry the sign\n\
of x.  The integer part is returned as a real.";


static PyMethodDef math_methods[] = {
	{"acos",	math_acos,	0,	math_acos_doc},
	{"asin",	math_asin,	0,	math_asin_doc},
	{"atan",	math_atan,	0,	math_atan_doc},
	{"atan2",	math_atan2,	0,	math_atan2_doc},
	{"ceil",	math_ceil,	0,	math_ceil_doc},
	{"cos",		math_cos,	0,	math_cos_doc},
	{"cosh",	math_cosh,	0,	math_cosh_doc},
	{"exp",		math_exp,	0,	math_exp_doc},
	{"fabs",	math_fabs,	0,	math_fabs_doc},
	{"floor",	math_floor,	0,	math_floor_doc},
	{"fmod",	math_fmod,	0,	math_fmod_doc},
	{"frexp",	math_frexp,	0,	math_frexp_doc},
	{"hypot",	math_hypot,	0,	math_hypot_doc},
	{"ldexp",	math_ldexp,	0,	math_ldexp_doc},
	{"log",		math_log,	0,	math_log_doc},
	{"log10",	math_log10,	0,	math_log10_doc},
	{"modf",	math_modf,	0,	math_modf_doc},
	{"pow",		math_pow,	0,	math_pow_doc},
	{"sin",		math_sin,	0,	math_sin_doc},
	{"sinh",	math_sinh,	0,	math_sinh_doc},
	{"sqrt",	math_sqrt,	0,	math_sqrt_doc},
	{"tan",		math_tan,	0,	math_tan_doc},
	{"tanh",	math_tanh,	0,	math_tanh_doc},
	{NULL,		NULL}		/* sentinel */
};


static char module_doc [] =
"This module is always available.  It provides access to the\n\
mathematical functions defined by the C standard.";

DL_EXPORT(void)
initmath()
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule3("math", math_methods, module_doc);
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
