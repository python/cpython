/* Complex math module */

/* much code borrowed from mathmodule.c */

#include "Python.h"

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

#ifndef M_PI
#define M_PI (3.141592653589793239)
#endif

/* First, the C functions that do the real work */

/* constants */
static Py_complex c_1 = {1., 0.};
static Py_complex c_half = {0.5, 0.};
static Py_complex c_i = {0., 1.};
static Py_complex c_i2 = {0., 0.5};
#if 0
static Py_complex c_mi = {0., -1.};
static Py_complex c_pi2 = {M_PI/2., 0.};
#endif

/* forward declarations */
staticforward Py_complex c_log(Py_complex);
staticforward Py_complex c_prodi(Py_complex);
staticforward Py_complex c_sqrt(Py_complex);


static Py_complex c_acos(Py_complex x)
{
	return c_neg(c_prodi(c_log(c_sum(x,c_prod(c_i,
		    c_sqrt(c_diff(c_1,c_prod(x,x))))))));
}

static char c_acos_doc [] =
"acos(x)\n\
\n\
Return the arc cosine of x.";


static Py_complex c_acosh(Py_complex x)
{
	Py_complex z;
	z = c_sqrt(c_half);
	z = c_log(c_prod(z, c_sum(c_sqrt(c_sum(x,c_1)),
				  c_sqrt(c_diff(x,c_1)))));
	return c_sum(z, z);
}

static char c_acosh_doc [] =
"acosh(x)\n\
\n\
Return the hyperbolic arccosine of x.";


static Py_complex c_asin(Py_complex x)
{
	Py_complex z;
	z = c_sqrt(c_half);
	z = c_log(c_prod(z, c_sum(c_sqrt(c_sum(x,c_i)),
				  c_sqrt(c_diff(x,c_i)))));
	return c_sum(z, z);
}

static char c_asin_doc [] =
"asin(x)\n\
\n\
Return the arc sine of x.";


static Py_complex c_asinh(Py_complex x)
{
	/* Break up long expression for WATCOM */
	Py_complex z;
	z = c_sum(c_1,c_prod(x, x));
	return c_log(c_sum(c_sqrt(z), x));
}

static char c_asinh_doc [] =
"asinh(x)\n\
\n\
Return the hyperbolic arc sine of x.";


static Py_complex c_atan(Py_complex x)
{
	return c_prod(c_i2,c_log(c_quot(c_sum(c_i,x),c_diff(c_i,x))));
}

static char c_atan_doc [] =
"atan(x)\n\
\n\
Return the arc tangent of x.";


static Py_complex c_atanh(Py_complex x)
{
	return c_prod(c_half,c_log(c_quot(c_sum(c_1,x),c_diff(c_1,x))));
}

static char c_atanh_doc [] =
"atanh(x)\n\
\n\
Return the hyperbolic arc tangent of x.";


static Py_complex c_cos(Py_complex x)
{
	Py_complex r;
	r.real = cos(x.real)*cosh(x.imag);
	r.imag = -sin(x.real)*sinh(x.imag);
	return r;
}

static char c_cos_doc [] =
"cos(x)\n\
\n\
Return the cosine of x.";


static Py_complex c_cosh(Py_complex x)
{
	Py_complex r;
	r.real = cos(x.imag)*cosh(x.real);
	r.imag = sin(x.imag)*sinh(x.real);
	return r;
}

static char c_cosh_doc [] =
"cosh(x)\n\
\n\
Return the hyperbolic cosine of x.";


static Py_complex c_exp(Py_complex x)
{
	Py_complex r;
	double l = exp(x.real);
	r.real = l*cos(x.imag);
	r.imag = l*sin(x.imag);
	return r;
}

static char c_exp_doc [] =
"exp(x)\n\
\n\
Return the exponential value e**x.";


static Py_complex c_log(Py_complex x)
{
	Py_complex r;
	double l = hypot(x.real,x.imag);
	r.imag = atan2(x.imag, x.real);
	r.real = log(l);
	return r;
}

static char c_log_doc [] =
"log(x)\n\
\n\
Return the natural logarithm of x.";


static Py_complex c_log10(Py_complex x)
{
	Py_complex r;
	double l = hypot(x.real,x.imag);
	r.imag = atan2(x.imag, x.real)/log(10.);
	r.real = log10(l);
	return r;
}

static char c_log10_doc [] =
"log10(x)\n\
\n\
Return the base-10 logarithm of x.";


/* internal function not available from Python */
static Py_complex c_prodi(Py_complex x)
{
	Py_complex r;
	r.real = -x.imag;
	r.imag = x.real;
	return r;
}


static Py_complex c_sin(Py_complex x)
{
	Py_complex r;
	r.real = sin(x.real)*cosh(x.imag);
	r.imag = cos(x.real)*sinh(x.imag);
	return r;
}

static char c_sin_doc [] =
"sin(x)\n\
\n\
Return the sine of x.";


static Py_complex c_sinh(Py_complex x)
{
	Py_complex r;
	r.real = cos(x.imag)*sinh(x.real);
	r.imag = sin(x.imag)*cosh(x.real);
	return r;
}

static char c_sinh_doc [] =
"sinh(x)\n\
\n\
Return the hyperbolic sine of x.";


static Py_complex c_sqrt(Py_complex x)
{
	Py_complex r;
	double s,d;
	if (x.real == 0. && x.imag == 0.)
		r = x;
	else {
		s = sqrt(0.5*(fabs(x.real) + hypot(x.real,x.imag)));
		d = 0.5*x.imag/s;
		if (x.real > 0.) {
			r.real = s;
			r.imag = d;
		}
		else if (x.imag >= 0.) {
			r.real = d;
			r.imag = s;
		}
		else {
			r.real = -d;
			r.imag = -s;
		}
	}
	return r;
}

static char c_sqrt_doc [] =
"sqrt(x)\n\
\n\
Return the square root of x.";


static Py_complex c_tan(Py_complex x)
{
	Py_complex r;
	double sr,cr,shi,chi;
	double rs,is,rc,ic;
	double d;
	sr = sin(x.real);
	cr = cos(x.real);
	shi = sinh(x.imag);
	chi = cosh(x.imag);
	rs = sr*chi;
	is = cr*shi;
	rc = cr*chi;
	ic = -sr*shi;
	d = rc*rc + ic*ic;
	r.real = (rs*rc+is*ic)/d;
	r.imag = (is*rc-rs*ic)/d;
	return r;
}

static char c_tan_doc [] =
"tan(x)\n\
\n\
Return the tangent of x.";


static Py_complex c_tanh(Py_complex x)
{
	Py_complex r;
	double si,ci,shr,chr;
	double rs,is,rc,ic;
	double d;
	si = sin(x.imag);
	ci = cos(x.imag);
	shr = sinh(x.real);
	chr = cosh(x.real);
	rs = ci*shr;
	is = si*chr;
	rc = ci*chr;
	ic = si*shr;
	d = rc*rc + ic*ic;
	r.real = (rs*rc+is*ic)/d;
	r.imag = (is*rc-rs*ic)/d;
	return r;
}

static char c_tanh_doc [] =
"tanh(x)\n\
\n\
Return the hyperbolic tangent of x.";


/* And now the glue to make them available from Python: */

static PyObject *
math_error(void)
{
	if (errno == EDOM)
		PyErr_SetString(PyExc_ValueError, "math domain error");
	else if (errno == ERANGE)
		PyErr_SetString(PyExc_OverflowError, "math range error");
	else    /* Unexpected math error */
		PyErr_SetFromErrno(PyExc_ValueError); 
	return NULL;
}

static PyObject *
math_1(PyObject *args, Py_complex (*func)(Py_complex))
{
	Py_complex x;
	if (!PyArg_ParseTuple(args, "D", &x))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("complex function", return 0)
	x = (*func)(x);
	PyFPE_END_PROTECT(x)
	CHECK(x.real);
	CHECK(x.imag);
	if (errno != 0)
		return math_error();
	else
		return PyComplex_FromCComplex(x);
}

#define FUNC1(stubname, func) \
	static PyObject * stubname(PyObject *self, PyObject *args) { \
		return math_1(args, func); \
	}

FUNC1(cmath_acos, c_acos)
FUNC1(cmath_acosh, c_acosh)
FUNC1(cmath_asin, c_asin)
FUNC1(cmath_asinh, c_asinh)
FUNC1(cmath_atan, c_atan)
FUNC1(cmath_atanh, c_atanh)
FUNC1(cmath_cos, c_cos)
FUNC1(cmath_cosh, c_cosh)
FUNC1(cmath_exp, c_exp)
FUNC1(cmath_log, c_log)
FUNC1(cmath_log10, c_log10)
FUNC1(cmath_sin, c_sin)
FUNC1(cmath_sinh, c_sinh)
FUNC1(cmath_sqrt, c_sqrt)
FUNC1(cmath_tan, c_tan)
FUNC1(cmath_tanh, c_tanh)


static char module_doc [] =
"This module is always available. It provides access to mathematical\n\
functions for complex numbers.";


static PyMethodDef cmath_methods[] = {
	{"acos", cmath_acos, 
	 METH_VARARGS, c_acos_doc},
	{"acosh", cmath_acosh, 
	 METH_VARARGS, c_acosh_doc},
	{"asin", cmath_asin, 
	 METH_VARARGS, c_asin_doc},
	{"asinh", cmath_asinh, 
	 METH_VARARGS, c_asinh_doc},
	{"atan", cmath_atan, 
	 METH_VARARGS, c_atan_doc},
	{"atanh", cmath_atanh, 
	 METH_VARARGS, c_atanh_doc},
	{"cos", cmath_cos, 
	 METH_VARARGS, c_cos_doc},
	{"cosh", cmath_cosh, 
	 METH_VARARGS, c_cosh_doc},
	{"exp", cmath_exp, 
	 METH_VARARGS, c_exp_doc},
	{"log", cmath_log, 
	 METH_VARARGS, c_log_doc},
	{"log10", cmath_log10, 
	 METH_VARARGS, c_log10_doc},
	{"sin", cmath_sin, 
	 METH_VARARGS, c_sin_doc},
	{"sinh", cmath_sinh, 
	 METH_VARARGS, c_sinh_doc},
	{"sqrt", cmath_sqrt, 
	 METH_VARARGS, c_sqrt_doc},
	{"tan", cmath_tan, 
	 METH_VARARGS, c_tan_doc},
	{"tanh", cmath_tanh, 
	 METH_VARARGS, c_tanh_doc},
	{NULL,		NULL}		/* sentinel */
};

DL_EXPORT(void)
initcmath(void)
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule3("cmath", cmath_methods, module_doc);
	d = PyModule_GetDict(m);
	PyDict_SetItemString(d, "pi",
			     v = PyFloat_FromDouble(atan(1.0) * 4.0));
	Py_DECREF(v);
	PyDict_SetItemString(d, "e", v = PyFloat_FromDouble(exp(1.0)));
	Py_DECREF(v);
}
