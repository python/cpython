/* Complex math module */

/* much code borrowed from mathmodule.c */

#include "Python.h"

#include "mymath.h"

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
staticforward Py_complex c_log();
staticforward Py_complex c_prodi();
staticforward Py_complex c_sqrt();


static Py_complex c_acos(x)
	Py_complex x;
{
	return c_neg(c_prodi(c_log(c_sum(x,c_prod(c_i,
		    c_sqrt(c_diff(c_1,c_prod(x,x))))))));
}

static Py_complex c_acosh(x)
	Py_complex x;
{
	return c_log(c_sum(x,c_prod(c_i,
		    c_sqrt(c_diff(c_1,c_prod(x,x))))));
}

static Py_complex c_asin(x)
	Py_complex x;
{
	return c_neg(c_prodi(c_log(c_sum(c_prod(c_i,x),
		    c_sqrt(c_diff(c_1,c_prod(x,x)))))));
}

static Py_complex c_asinh(x)
	Py_complex x;
{
	return c_neg(c_log(c_diff(c_sqrt(c_sum(c_1,c_prod(x,x))),x)));
}

static Py_complex c_atan(x)
	Py_complex x;
{
	return c_prod(c_i2,c_log(c_quot(c_sum(c_i,x),c_diff(c_i,x))));
}

static Py_complex c_atanh(x)
	Py_complex x;
{
	return c_prod(c_half,c_log(c_quot(c_sum(c_1,x),c_diff(c_1,x))));
}

static Py_complex c_cos(x)
	Py_complex x;
{
	Py_complex r;
	r.real = cos(x.real)*cosh(x.imag);
	r.imag = -sin(x.real)*sinh(x.imag);
	return r;
}

static Py_complex c_cosh(x)
	Py_complex x;
{
	Py_complex r;
	r.real = cos(x.imag)*cosh(x.real);
	r.imag = sin(x.imag)*sinh(x.real);
	return r;
}

static Py_complex c_exp(x)
	Py_complex x;
{
	Py_complex r;
	double l = exp(x.real);
	r.real = l*cos(x.imag);
	r.imag = l*sin(x.imag);
	return r;
}

static Py_complex c_log(x)
	Py_complex x;
{
	Py_complex r;
	double l = hypot(x.real,x.imag);
	r.imag = atan2(x.imag, x.real);
	r.real = log(l);
	return r;
}

static Py_complex c_log10(x)
	Py_complex x;
{
	Py_complex r;
	double l = hypot(x.real,x.imag);
	r.imag = atan2(x.imag, x.real)/log(10.);
	r.real = log10(l);
	return r;
}

static Py_complex c_prodi(x)
     Py_complex x;
{
	Py_complex r;
	r.real = -x.imag;
	r.imag = x.real;
	return r;
}

static Py_complex c_sin(x)
	Py_complex x;
{
	Py_complex r;
	r.real = sin(x.real)*cosh(x.imag);
	r.imag = cos(x.real)*sinh(x.imag);
	return r;
}

static Py_complex c_sinh(x)
	Py_complex x;
{
	Py_complex r;
	r.real = cos(x.imag)*sinh(x.real);
	r.imag = sin(x.imag)*cosh(x.real);
	return r;
}

static Py_complex c_sqrt(x)
	Py_complex x;
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

static Py_complex c_tan(x)
	Py_complex x;
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

static Py_complex c_tanh(x)
	Py_complex x;
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


/* And now the glue to make them available from Python: */

static PyObject *
math_error()
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
math_1(args, func)
	PyObject *args;
	Py_complex (*func) Py_FPROTO((Py_complex));
{
	Py_complex x;
	if (!PyArg_ParseTuple(args, "D", &x))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("complex function", return 0)
	x = (*func)(x);
	PyFPE_END_PROTECT
	CHECK(x.real);
	CHECK(x.imag);
	if (errno != 0)
		return math_error();
	else
		return PyComplex_FromCComplex(x);
}

#define FUNC1(stubname, func) \
	static PyObject * stubname(self, args) PyObject *self, *args; { \
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


static PyMethodDef cmath_methods[] = {
	{"acos", cmath_acos, 1},
	{"acosh", cmath_acosh, 1},
	{"asin", cmath_asin, 1},
	{"asinh", cmath_asinh, 1},
	{"atan", cmath_atan, 1},
	{"atanh", cmath_atanh, 1},
	{"cos", cmath_cos, 1},
	{"cosh", cmath_cosh, 1},
	{"exp", cmath_exp, 1},
	{"log", cmath_log, 1},
	{"log10", cmath_log10, 1},
	{"sin", cmath_sin, 1},
	{"sinh", cmath_sinh, 1},
	{"sqrt", cmath_sqrt, 1},
	{"tan", cmath_tan, 1},
	{"tanh", cmath_tanh, 1},
	{NULL,		NULL}		/* sentinel */
};

void
initcmath()
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule("cmath", cmath_methods);
	d = PyModule_GetDict(m);
	PyDict_SetItemString(d, "pi",
			     v = PyFloat_FromDouble(atan(1.0) * 4.0));
	Py_DECREF(v);
	PyDict_SetItemString(d, "e", v = PyFloat_FromDouble(exp(1.0)));
	Py_DECREF(v);
}
