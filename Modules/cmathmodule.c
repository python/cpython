/* Complex math module */

/* much code borrowed from mathmodule.c */

#include "Python.h"

#ifndef M_PI
#define M_PI (3.141592653589793239)
#endif

/* First, the C functions that do the real work */

/* constants */
static Py_complex c_one = {1., 0.};
static Py_complex c_half = {0.5, 0.};
static Py_complex c_i = {0., 1.};
static Py_complex c_halfi = {0., 0.5};

/* forward declarations */
static Py_complex c_log(Py_complex);
static Py_complex c_prodi(Py_complex);
static Py_complex c_sqrt(Py_complex);
static PyObject * math_error(void);


static Py_complex
c_acos(Py_complex x)
{
	return c_neg(c_prodi(c_log(c_sum(x,c_prod(c_i,
		    c_sqrt(c_diff(c_one,c_prod(x,x))))))));
}

PyDoc_STRVAR(c_acos_doc,
"acos(x)\n"
"\n"
"Return the arc cosine of x.");


static Py_complex
c_acosh(Py_complex x)
{
	Py_complex z;
	z = c_sqrt(c_half);
	z = c_log(c_prod(z, c_sum(c_sqrt(c_sum(x,c_one)),
				  c_sqrt(c_diff(x,c_one)))));
	return c_sum(z, z);
}

PyDoc_STRVAR(c_acosh_doc,
"acosh(x)\n"
"\n"
"Return the hyperbolic arccosine of x.");


static Py_complex
c_asin(Py_complex x)
{
	/* -i * log[(sqrt(1-x**2) + i*x] */
	const Py_complex squared = c_prod(x, x);
	const Py_complex sqrt_1_minus_x_sq = c_sqrt(c_diff(c_one, squared));
        return c_neg(c_prodi(c_log(
        		c_sum(sqrt_1_minus_x_sq, c_prodi(x))
		    )       )     );
}

PyDoc_STRVAR(c_asin_doc,
"asin(x)\n"
"\n"
"Return the arc sine of x.");


static Py_complex
c_asinh(Py_complex x)
{
	Py_complex z;
	z = c_sqrt(c_half);
	z = c_log(c_prod(z, c_sum(c_sqrt(c_sum(x, c_i)),
				  c_sqrt(c_diff(x, c_i)))));
	return c_sum(z, z);
}

PyDoc_STRVAR(c_asinh_doc,
"asinh(x)\n"
"\n"
"Return the hyperbolic arc sine of x.");


static Py_complex
c_atan(Py_complex x)
{
	return c_prod(c_halfi,c_log(c_quot(c_sum(c_i,x),c_diff(c_i,x))));
}

PyDoc_STRVAR(c_atan_doc,
"atan(x)\n"
"\n"
"Return the arc tangent of x.");


static Py_complex
c_atanh(Py_complex x)
{
	return c_prod(c_half,c_log(c_quot(c_sum(c_one,x),c_diff(c_one,x))));
}

PyDoc_STRVAR(c_atanh_doc,
"atanh(x)\n"
"\n"
"Return the hyperbolic arc tangent of x.");


static Py_complex
c_cos(Py_complex x)
{
	Py_complex r;
	r.real = cos(x.real)*cosh(x.imag);
	r.imag = -sin(x.real)*sinh(x.imag);
	return r;
}

PyDoc_STRVAR(c_cos_doc,
"cos(x)\n"
"n"
"Return the cosine of x.");


static Py_complex
c_cosh(Py_complex x)
{
	Py_complex r;
	r.real = cos(x.imag)*cosh(x.real);
	r.imag = sin(x.imag)*sinh(x.real);
	return r;
}

PyDoc_STRVAR(c_cosh_doc,
"cosh(x)\n"
"n"
"Return the hyperbolic cosine of x.");


static Py_complex
c_exp(Py_complex x)
{
	Py_complex r;
	double l = exp(x.real);
	r.real = l*cos(x.imag);
	r.imag = l*sin(x.imag);
	return r;
}

PyDoc_STRVAR(c_exp_doc,
"exp(x)\n"
"\n"
"Return the exponential value e**x.");


static Py_complex
c_log(Py_complex x)
{
	Py_complex r;
	double l = hypot(x.real,x.imag);
	r.imag = atan2(x.imag, x.real);
	r.real = log(l);
	return r;
}


static Py_complex
c_log10(Py_complex x)
{
	Py_complex r;
	double l = hypot(x.real,x.imag);
	r.imag = atan2(x.imag, x.real)/log(10.);
	r.real = log10(l);
	return r;
}

PyDoc_STRVAR(c_log10_doc,
"log10(x)\n"
"\n"
"Return the base-10 logarithm of x.");


/* internal function not available from Python */
static Py_complex
c_prodi(Py_complex x)
{
	Py_complex r;
	r.real = -x.imag;
	r.imag = x.real;
	return r;
}


static Py_complex
c_sin(Py_complex x)
{
	Py_complex r;
	r.real = sin(x.real) * cosh(x.imag);
	r.imag = cos(x.real) * sinh(x.imag);
	return r;
}

PyDoc_STRVAR(c_sin_doc,
"sin(x)\n"
"\n"
"Return the sine of x.");


static Py_complex
c_sinh(Py_complex x)
{
	Py_complex r;
	r.real = cos(x.imag) * sinh(x.real);
	r.imag = sin(x.imag) * cosh(x.real);
	return r;
}

PyDoc_STRVAR(c_sinh_doc,
"sinh(x)\n"
"\n"
"Return the hyperbolic sine of x.");


static Py_complex
c_sqrt(Py_complex x)
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

PyDoc_STRVAR(c_sqrt_doc,
"sqrt(x)\n"
"\n"
"Return the square root of x.");


static Py_complex
c_tan(Py_complex x)
{
	Py_complex r;
	double sr,cr,shi,chi;
	double rs,is,rc,ic;
	double d;
	sr = sin(x.real);
	cr = cos(x.real);
	shi = sinh(x.imag);
	chi = cosh(x.imag);
	rs = sr * chi;
	is = cr * shi;
	rc = cr * chi;
	ic = -sr * shi;
	d = rc*rc + ic * ic;
	r.real = (rs*rc + is*ic) / d;
	r.imag = (is*rc - rs*ic) / d;
	return r;
}

PyDoc_STRVAR(c_tan_doc,
"tan(x)\n"
"\n"
"Return the tangent of x.");


static Py_complex
c_tanh(Py_complex x)
{
	Py_complex r;
	double si,ci,shr,chr;
	double rs,is,rc,ic;
	double d;
	si = sin(x.imag);
	ci = cos(x.imag);
	shr = sinh(x.real);
	chr = cosh(x.real);
	rs = ci * shr;
	is = si * chr;
	rc = ci * chr;
	ic = si * shr;
	d = rc*rc + ic*ic;
	r.real = (rs*rc + is*ic) / d;
	r.imag = (is*rc - rs*ic) / d;
	return r;
}

PyDoc_STRVAR(c_tanh_doc,
"tanh(x)\n"
"\n"
"Return the hyperbolic tangent of x.");

static PyObject *
cmath_log(PyObject *self, PyObject *args)
{
	Py_complex x;
	Py_complex y;

	if (!PyArg_ParseTuple(args, "D|D", &x, &y))
		return NULL;

	errno = 0;
	PyFPE_START_PROTECT("complex function", return 0)
	x = c_log(x);
	if (PyTuple_GET_SIZE(args) == 2)
		x = c_quot(x, c_log(y));
	PyFPE_END_PROTECT(x)
	if (errno != 0)
		return math_error();
	Py_ADJUST_ERANGE2(x.real, x.imag);
	return PyComplex_FromCComplex(x);
}

PyDoc_STRVAR(cmath_log_doc,
"log(x[, base]) -> the logarithm of x to the given base.\n\
If the base not specified, returns the natural logarithm (base e) of x.");


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
	Py_ADJUST_ERANGE2(x.real, x.imag);
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
FUNC1(cmath_log10, c_log10)
FUNC1(cmath_sin, c_sin)
FUNC1(cmath_sinh, c_sinh)
FUNC1(cmath_sqrt, c_sqrt)
FUNC1(cmath_tan, c_tan)
FUNC1(cmath_tanh, c_tanh)


PyDoc_STRVAR(module_doc,
"This module is always available. It provides access to mathematical\n"
"functions for complex numbers.");

static PyMethodDef cmath_methods[] = {
	{"acos",   cmath_acos,  METH_VARARGS, c_acos_doc},
	{"acosh",  cmath_acosh, METH_VARARGS, c_acosh_doc},
	{"asin",   cmath_asin,  METH_VARARGS, c_asin_doc},
	{"asinh",  cmath_asinh, METH_VARARGS, c_asinh_doc},
	{"atan",   cmath_atan,  METH_VARARGS, c_atan_doc},
	{"atanh",  cmath_atanh, METH_VARARGS, c_atanh_doc},
	{"cos",    cmath_cos,   METH_VARARGS, c_cos_doc},
	{"cosh",   cmath_cosh,  METH_VARARGS, c_cosh_doc},
	{"exp",    cmath_exp,   METH_VARARGS, c_exp_doc},
	{"log",    cmath_log,   METH_VARARGS, cmath_log_doc},
	{"log10",  cmath_log10, METH_VARARGS, c_log10_doc},
	{"sin",    cmath_sin,   METH_VARARGS, c_sin_doc},
	{"sinh",   cmath_sinh,  METH_VARARGS, c_sinh_doc},
	{"sqrt",   cmath_sqrt,  METH_VARARGS, c_sqrt_doc},
	{"tan",    cmath_tan,   METH_VARARGS, c_tan_doc},
	{"tanh",   cmath_tanh,  METH_VARARGS, c_tanh_doc},
	{NULL,		NULL}		/* sentinel */
};

PyMODINIT_FUNC
initcmath(void)
{
	PyObject *m;

	m = Py_InitModule3("cmath", cmath_methods, module_doc);
	if (m == NULL)
		return;

	PyModule_AddObject(m, "pi",
                           PyFloat_FromDouble(atan(1.0) * 4.0));
	PyModule_AddObject(m, "e", PyFloat_FromDouble(exp(1.0)));
}
