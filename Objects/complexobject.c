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
static complex c_1 = {1., 0.};

complex c_sum(a,b)
	complex a,b;
{
	complex r;
	r.real = a.real + b.real;
	r.imag = a.imag + b.imag;
	return r;
}

complex c_diff(a,b)
	complex a,b;
{
	complex r;
	r.real = a.real - b.real;
	r.imag = a.imag - b.imag;
	return r;
}

complex c_neg(a)
	complex a;
{
	complex r;
	r.real = -a.real;
	r.imag = -a.imag;
	return r;
}

complex c_prod(a,b)
	complex a,b;
{
	complex r;
	r.real = a.real*b.real - a.imag*b.imag;
	r.imag = a.real*b.imag + a.imag*b.real;
	return r;
}

complex c_quot(a,b)
	complex a,b;
{
	complex r;
	double d = b.real*b.real + b.imag*b.imag;
	if (d == 0.)
		c_error = 1;
	r.real = (a.real*b.real + a.imag*b.imag)/d;
	r.imag = (a.imag*b.real - a.real*b.imag)/d;
	return r;
}

complex c_pow(a,b)
	complex a,b;
{
	complex r;
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

static complex c_powu(x, n)
	complex x;
	long n;
{
	complex r = c_1;
	complex p = x;
	long mask = 1;
	while (mask > 0 && n >= mask) {
		if (n & mask)
			r = c_prod(r,p);
		mask <<= 1;
		p = c_prod(p,p);
	}
	return r;
}

static complex c_powi(x, n)
	complex x;
	long n;
{
	complex cn;

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
PyComplex_FromCComplex(complex cval)
{
	register complexobject *op = (complexobject *) malloc(sizeof(complexobject));
	if (op == NULL)
		return err_nomem();
	op->ob_type = &Complextype;
	op->cval = cval;
	NEWREF(op);
	return (object *) op;
}

PyObject *
PyComplex_FromDoubles(double real, double imag) {
  complex c;
  c.real = real;
  c.imag = imag;
  return PyComplex_FromCComplex(c);
}

double
PyComplex_RealAsDouble(PyObject *op) {
  if (PyComplex_Check(op)) {
    return ((PyComplexObject *)op)->cval.real;
  } else {
    return PyFloat_AsDouble(op);
  }
}

double
PyComplex_ImagAsDouble(PyObject *op) {
  if (PyComplex_Check(op)) {
    return ((PyComplexObject *)op)->cval.imag;
  } else {
    return 0.0;
  }
}

complex
PyComplex_AsCComplex(PyObject *op) {
	complex cv;
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
	complex i = v->cval;
	complex j = w->cval;
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
	long x;
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

	if (fractpart == 0.0) {
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
		fractpart = fractpart*2147483648.0; /* 2**31 */
		x = (long) (intpart + fractpart) ^ expo; /* Rather arbitrary */
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
	return newcomplexobject(c_sum(v->cval,w->cval));
}

static object *
complex_sub(v, w)
	complexobject *v;
	complexobject *w;
{
	return newcomplexobject(c_diff(v->cval,w->cval));
}

static object *
complex_mul(v, w)
	complexobject *v;
	complexobject *w;
{
	return newcomplexobject(c_prod(v->cval,w->cval));
}

static object *
complex_div(v, w)
	complexobject *v;
	complexobject *w;
{
	complex quot;
	c_error = 0;
	quot = c_quot(v->cval,w->cval);
	if (c_error == 1) {
		err_setstr(ZeroDivisionError, "float division");
		return NULL;
	}
	return newcomplexobject(quot);
}


static object *
complex_pow(v, w, z)
	complexobject *v;
	object *w;
	complexobject *z;
{
	complex p;
	complex exponent;
	long int_exponent;

 	if ((object *)z!=None) {
		err_setstr(ValueError, "complex modulo");
		return NULL;
	}

	c_error = 0;
	exponent = ((complexobject*)w)->cval;
	int_exponent = (long)exponent.real;
	if (exponent.imag == 0. && exponent.real == int_exponent)
		p = c_powi(v->cval,int_exponent);
	else
		p = c_pow(v->cval,exponent);

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
	complex neg;
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
	return newfloatobject(hypot(v->cval.real,v->cval.imag));
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
	complex cval;
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
	double x = ((complexobject *)v)->cval.real;
	if (x < 0 ? (x = ceil(x)) < (double)LONG_MIN
	          : (x = floor(x)) > (double)LONG_MAX) {
		err_setstr(OverflowError, "float too large to convert");
		return NULL;
	}
	return newintobject((long)x);
}

static object *
complex_long(v)
	object *v;
{
	double x = ((complexobject *)v)->cval.real;
	return dnewlongobject(x);
}

static object *
complex_float(v)
	object *v;
{
	double x = ((complexobject *)v)->cval.real;
	return newfloatobject(x);
}


static object *
complex_new(self, args)
	object *self;
	object *args;
{
	complex cval;

	cval.imag = 0.;
	if (!PyArg_ParseTuple(args, "d|d", &cval.real, &cval.imag))
		return NULL;
	return newcomplexobject(cval);
}

static object *
complex_conjugate(self)
	object *self;
{
	complex c = ((complexobject *)self)->cval;
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
	complex cval;
	if (strcmp(name, "real") == 0)
		return (object *)newfloatobject(self->cval.real);
	else if (strcmp(name, "imag") == 0)
		return (object *)newfloatobject(self->cval.imag);
	else if (strcmp(name, "conj") == 0) {
		cval.real = self->cval.real;
		cval.imag = -self->cval.imag;
		return (object *)newcomplexobject(cval);
	}
	return findmethod(complex_methods, (object *)self, name);
}

static number_methods complex_as_number = {
	(binaryfunc)complex_add, /*nb_add*/
	(binaryfunc)complex_sub, /*nb_subtract*/
	(binaryfunc)complex_mul, /*nb_multiply*/
	(binaryfunc)complex_div, /*nb_divide*/
	0,		/*nb_remainder*/
	0,		/*nb_divmod*/
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
