/* Complex object implementation */

/* Borrows heavily from floatobject.c */

#ifndef WITHOUT_COMPLEX

#include "allobjects.h"
#include "modsupport.h"

#include <errno.h>
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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef LONG_MAX
#define LONG_MAX 0X7FFFFFFFL
#endif

#ifndef LONG_MIN
#define LONG_MIN (-LONG_MAX-1)
#endif

#ifdef __NeXT__
#ifdef __sparc__
/*
 * This works around a bug in the NS/Sparc 3.3 pre-release
 * limits.h header file.
 * 10-Feb-1995 bwarsaw@cnri.reston.va.us
 */
#undef LONG_MIN
#define LONG_MIN (-LONG_MAX-1)
#endif
#endif

#if !defined(__STDC__) && !defined(macintosh)
extern double fmod PROTO((double, double));
extern double pow PROTO((double, double));
#endif


/* elementary operations on complex numbers */

static int c_error;
static Py_complex c_1 = {1., 0.};

Py_complex c_sum(a,b)
	Py_complex a,b;
{
	Py_complex r;
	r.real = a.real + b.real;
	r.imag = a.imag + b.imag;
	return r;
}

Py_complex c_diff(a,b)
	Py_complex a,b;
{
	Py_complex r;
	r.real = a.real - b.real;
	r.imag = a.imag - b.imag;
	return r;
}

Py_complex c_neg(a)
	Py_complex a;
{
	Py_complex r;
	r.real = -a.real;
	r.imag = -a.imag;
	return r;
}

Py_complex c_prod(a,b)
	Py_complex a,b;
{
	Py_complex r;
	r.real = a.real*b.real - a.imag*b.imag;
	r.imag = a.real*b.imag + a.imag*b.real;
	return r;
}

Py_complex c_quot(a,b)
	Py_complex a,b;
{
	Py_complex r;
	double d = b.real*b.real + b.imag*b.imag;
	if (d == 0.)
		c_error = 1;
	r.real = (a.real*b.real + a.imag*b.imag)/d;
	r.imag = (a.imag*b.real - a.real*b.imag)/d;
	return r;
}

Py_complex c_pow(a,b)
	Py_complex a,b;
{
	Py_complex r;
	double vabs,len,at,phase;
	if (b.real == 0. && b.imag == 0.) {
		r.real = 1.;
		r.imag = 0.;
	}
	else if (a.real == 0. && a.imag == 0.) {
		if (b.imag != 0. || b.real < 0.)
			c_error = 2;
		r.real = 0.;
		r.imag = 0.;
	}
	else {
		vabs = hypot(a.real,a.imag);
		len = pow(vabs,b.real);
		at = atan2(a.imag, a.real);
		phase = at*b.real;
		if (b.imag != 0.0) {
			len /= exp(at*b.imag);
			phase += b.imag*log(vabs);
		}
		r.real = len*cos(phase);
		r.imag = len*sin(phase);
	}
	return r;
}

static Py_complex c_powu(x, n)
	Py_complex x;
	long n;
{
	Py_complex r, p;
	long mask = 1;
	r = c_1;
	p = x;
	while (mask > 0 && n >= mask) {
		if (n & mask)
			r = c_prod(r,p);
		mask <<= 1;
		p = c_prod(p,p);
	}
	return r;
}

static Py_complex c_powi(x, n)
	Py_complex x;
	long n;
{
	Py_complex cn;

	if (n > 100 || n < -100) {
		cn.real = (double) n;
		cn.imag = 0.;
		return c_pow(x,cn);
	}
	else if (n > 0)
		return c_powu(x,n);
	else
		return c_quot(c_1,c_powu(x,-n));

}

PyObject *
PyComplex_FromCComplex(cval)
	Py_complex cval;
{
	register complexobject *op =
		(complexobject *) malloc(sizeof(complexobject));
	if (op == NULL)
		return err_nomem();
	op->ob_type = &Complextype;
	op->cval = cval;
	NEWREF(op);
	return (object *) op;
}

PyObject *
PyComplex_FromDoubles(real, imag)
	double real, imag;
{
	Py_complex c;
	c.real = real;
	c.imag = imag;
	return PyComplex_FromCComplex(c);
}

double
PyComplex_RealAsDouble(op)
	PyObject *op;
{
	if (PyComplex_Check(op)) {
		return ((PyComplexObject *)op)->cval.real;
	} else {
		return PyFloat_AsDouble(op);
	}
}

double
PyComplex_ImagAsDouble(op)
	PyObject *op;
{
	if (PyComplex_Check(op)) {
		return ((PyComplexObject *)op)->cval.imag;
	} else {
		return 0.0;
	}
}

Py_complex
PyComplex_AsCComplex(op)
	PyObject *op;
{
	Py_complex cv;
	if (PyComplex_Check(op)) {
		return ((PyComplexObject *)op)->cval;
	} else {
		cv.real = PyFloat_AsDouble(op);
		cv.imag = 0.;
		return cv;
	}   
}

static void
complex_dealloc(op)
	object *op;
{
	DEL(op);
}


static void
complex_buf_repr(buf, v)
	char *buf;
	complexobject *v;
{
	if (v->cval.real == 0.)
		sprintf(buf, "%.12gj", v->cval.imag);
	else
		sprintf(buf, "(%.12g%+.12gj)", v->cval.real, v->cval.imag);
}

static int
complex_print(v, fp, flags)
	complexobject *v;
	FILE *fp;
	int flags; /* Not used but required by interface */
{
	char buf[100];
	complex_buf_repr(buf, v);
	fputs(buf, fp);
	return 0;
}

static object *
complex_repr(v)
	complexobject *v;
{
	char buf[100];
	complex_buf_repr(buf, v);
	return newstringobject(buf);
}

static int
complex_compare(v, w)
	complexobject *v, *w;
{
/* Note: "greater" and "smaller" have no meaning for complex numbers,
   but Python requires that they be defined nevertheless. */
	Py_complex i, j;
	i = v->cval;
	j = w->cval;
	if (i.real == j.real && i.imag == j.imag)
	   return 0;
	else if (i.real != j.real)
	   return (i.real < j.real) ? -1 : 1;
	else
	   return (i.imag < j.imag) ? -1 : 1;
}

static long
complex_hash(v)
	complexobject *v;
{
	double intpart, fractpart;
	int expo;
	long hipart, x;
	/* This is designed so that Python numbers with the same
	   value hash to the same value, otherwise comparisons
	   of mapping keys will turn out weird */

#ifdef MPW /* MPW C modf expects pointer to extended as second argument */
{
	extended e;
	fractpart = modf(v->cval.real, &e);
	intpart = e;
}
#else
	fractpart = modf(v->cval.real, &intpart);
#endif

	if (fractpart == 0.0 && v->cval.imag == 0.0) {
		if (intpart > 0x7fffffffL || -intpart > 0x7fffffffL) {
			/* Convert to long int and use its hash... */
			object *w = dnewlongobject(v->cval.real);
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
		fractpart = fractpart * 2147483648.0; /* 2**31 */
		hipart = (long)fractpart; /* Take the top 32 bits */
		fractpart = (fractpart - (double)hipart) * 2147483648.0;
						/* Get the next 32 bits */
		x = hipart + (long)fractpart + (long)intpart + (expo << 15);
						/* Combine everything */

		if (v->cval.imag != 0.0) { /* Hash the imaginary part */
			/* XXX Note that this hashes complex(x, y)
			   to the same value as complex(y, x).
			   Still better than it used to be :-) */
#ifdef MPW
			{
				extended e;
				fractpart = modf(v->cval.imag, &e);
				intpart = e;
			}
#else
			fractpart = modf(v->cval.imag, &intpart);
#endif
			fractpart = frexp(fractpart, &expo);
			fractpart = fractpart * 2147483648.0; /* 2**31 */
			hipart = (long)fractpart; /* Take the top 32 bits */
			fractpart =
				(fractpart - (double)hipart) * 2147483648.0;
						/* Get the next 32 bits */
			x ^= hipart + (long)fractpart +
				(long)intpart + (expo << 15);
						/* Combine everything */
		}
	}
	if (x == -1)
		x = -2;
	return x;
}

static object *
complex_add(v, w)
	complexobject *v;
	complexobject *w;
{
	Py_complex result;
	PyFPE_START_PROTECT("complex_add", return 0)
	result = c_sum(v->cval,w->cval);
	PyFPE_END_PROTECT(result)
	return newcomplexobject(result);
}

static object *
complex_sub(v, w)
	complexobject *v;
	complexobject *w;
{
	Py_complex result;
	PyFPE_START_PROTECT("complex_sub", return 0)
	result = c_diff(v->cval,w->cval);
	PyFPE_END_PROTECT(result)
	return newcomplexobject(result);
}

static object *
complex_mul(v, w)
	complexobject *v;
	complexobject *w;
{
	Py_complex result;
	PyFPE_START_PROTECT("complex_mul", return 0)
	result = c_prod(v->cval,w->cval);
	PyFPE_END_PROTECT(result)
	return newcomplexobject(result);
}

static object *
complex_div(v, w)
	complexobject *v;
	complexobject *w;
{
	Py_complex quot;
	PyFPE_START_PROTECT("complex_div", return 0)
	c_error = 0;
	quot = c_quot(v->cval,w->cval);
	PyFPE_END_PROTECT(quot)
	if (c_error == 1) {
		err_setstr(ZeroDivisionError, "complex division");
		return NULL;
	}
	return newcomplexobject(quot);
}

static object *
complex_remainder(v, w)
	complexobject *v;
	complexobject *w;
{
        Py_complex div, mod;
	c_error = 0;
	div = c_quot(v->cval,w->cval); /* The raw divisor value. */
	if (c_error == 1) {
		err_setstr(ZeroDivisionError, "complex remainder");
		return NULL;
	}
	div.real = floor(div.real); /* Use the floor of the real part. */
	div.imag = 0.0;
	mod = c_diff(v->cval, c_prod(w->cval, div));

	return newcomplexobject(mod);
}


static object *
complex_divmod(v, w)
	complexobject *v;
	complexobject *w;
{
        Py_complex div, mod;
	PyObject *d, *m, *z;
	c_error = 0;
	div = c_quot(v->cval,w->cval); /* The raw divisor value. */
	if (c_error == 1) {
		err_setstr(ZeroDivisionError, "complex divmod()");
		return NULL;
	}
	div.real = floor(div.real); /* Use the floor of the real part. */
	div.imag = 0.0;
	mod = c_diff(v->cval, c_prod(w->cval, div));
	d = newcomplexobject(div);
	m = newcomplexobject(mod);
	z = mkvalue("(OO)", d, m);
	Py_XDECREF(d);
	Py_XDECREF(m);
	return z;
}

static object *
complex_pow(v, w, z)
	complexobject *v;
	object *w;
	complexobject *z;
{
	Py_complex p;
	Py_complex exponent;
	long int_exponent;

 	if ((object *)z!=None) {
		err_setstr(ValueError, "complex modulo");
		return NULL;
	}

	PyFPE_START_PROTECT("complex_pow", return 0)
	c_error = 0;
	exponent = ((complexobject*)w)->cval;
	int_exponent = (long)exponent.real;
	if (exponent.imag == 0. && exponent.real == int_exponent)
		p = c_powi(v->cval,int_exponent);
	else
		p = c_pow(v->cval,exponent);

	PyFPE_END_PROTECT(p)
	if (c_error == 2) {
		err_setstr(ValueError, "0.0 to a negative or complex power");
		return NULL;
	}

	return newcomplexobject(p);
}

static object *
complex_neg(v)
	complexobject *v;
{
	Py_complex neg;
	neg.real = -v->cval.real;
	neg.imag = -v->cval.imag;
	return newcomplexobject(neg);
}

static object *
complex_pos(v)
	complexobject *v;
{
	INCREF(v);
	return (object *)v;
}

static object *
complex_abs(v)
	complexobject *v;
{
	double result;
	PyFPE_START_PROTECT("complex_abs", return 0)
	result = hypot(v->cval.real,v->cval.imag);
	PyFPE_END_PROTECT(result)
	return newfloatobject(result);
}

static int
complex_nonzero(v)
	complexobject *v;
{
	return v->cval.real != 0.0 && v->cval.imag != 0.0;
}

static int
complex_coerce(pv, pw)
	object **pv;
	object **pw;
{
	Py_complex cval;
	cval.imag = 0.;
	if (is_intobject(*pw)) {
		cval.real = (double)getintvalue(*pw);
		*pw = newcomplexobject(cval);
		INCREF(*pv);
		return 0;
	}
	else if (is_longobject(*pw)) {
		cval.real = dgetlongvalue(*pw);
		*pw = newcomplexobject(cval);
		INCREF(*pv);
		return 0;
	}
	else if (is_floatobject(*pw)) {
		cval.real = getfloatvalue(*pw);
		*pw = newcomplexobject(cval);
		INCREF(*pv);
		return 0;
	}
	return 1; /* Can't do it */
}

static object *
complex_int(v)
	object *v;
{
	err_setstr(TypeError,
		   "can't convert complex to int; use e.g. int(abs(z))");
	return NULL;
}

static object *
complex_long(v)
	object *v;
{
	err_setstr(TypeError,
		   "can't convert complex to long; use e.g. long(abs(z))");
	return NULL;
}

static object *
complex_float(v)
	object *v;
{
	err_setstr(TypeError,
		   "can't convert complex to float; use e.g. abs(z)");
	return NULL;
}

static object *
complex_conjugate(self)
	object *self;
{
	Py_complex c;
	c = ((complexobject *)self)->cval;
	c.imag = -c.imag;
	return newcomplexobject(c);
}

static PyMethodDef complex_methods[] = {
	{"conjugate",	(PyCFunction)complex_conjugate,	1},
	{NULL,		NULL}		/* sentinel */
};


static object *
complex_getattr(self, name)
	complexobject *self;
	char *name;
{
	Py_complex cval;
	if (strcmp(name, "real") == 0)
		return (object *)newfloatobject(self->cval.real);
	else if (strcmp(name, "imag") == 0)
		return (object *)newfloatobject(self->cval.imag);
	else if (strcmp(name, "__members__") == 0)
		return mkvalue("[ss]", "imag", "real");
	return findmethod(complex_methods, (object *)self, name);
}

static number_methods complex_as_number = {
	(binaryfunc)complex_add, /*nb_add*/
	(binaryfunc)complex_sub, /*nb_subtract*/
	(binaryfunc)complex_mul, /*nb_multiply*/
	(binaryfunc)complex_div, /*nb_divide*/
	(binaryfunc)complex_remainder,	/*nb_remainder*/
	(binaryfunc)complex_divmod,	/*nb_divmod*/
	(ternaryfunc)complex_pow, /*nb_power*/
	(unaryfunc)complex_neg, /*nb_negative*/
	(unaryfunc)complex_pos, /*nb_positive*/
	(unaryfunc)complex_abs, /*nb_absolute*/
	(inquiry)complex_nonzero, /*nb_nonzero*/
	0,		/*nb_invert*/
	0,		/*nb_lshift*/
	0,		/*nb_rshift*/
	0,		/*nb_and*/
	0,		/*nb_xor*/
	0,		/*nb_or*/
	(coercion)complex_coerce, /*nb_coerce*/
	(unaryfunc)complex_int, /*nb_int*/
	(unaryfunc)complex_long, /*nb_long*/
	(unaryfunc)complex_float, /*nb_float*/
	0,		/*nb_oct*/
	0,		/*nb_hex*/
};

typeobject Complextype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"complex",
	sizeof(complexobject),
	0,
	(destructor)complex_dealloc,	/*tp_dealloc*/
	(printfunc)complex_print,	/*tp_print*/
	(getattrfunc)complex_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	(cmpfunc)complex_compare,	/*tp_compare*/
	(reprfunc)complex_repr,		/*tp_repr*/
	&complex_as_number,    		/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	(hashfunc)complex_hash, 	/*tp_hash*/
};

#endif
