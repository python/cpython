
/* Float object implementation */

/* XXX There should be overflow checks here, but it's hard to check
   for any kind of float exception without losing portability. */

#include "Python.h"

#include <ctype.h>

#if !defined(__STDC__) && !defined(macintosh)
extern double fmod(double, double);
extern double pow(double, double);
#endif

#if defined(sun) && !defined(__SVR4)
/* On SunOS4.1 only libm.a exists. Make sure that references to all
   needed math functions exist in the executable, so that dynamic
   loading of mathmodule does not fail. */
double (*_Py_math_funcs_hack[])() = {
	acos, asin, atan, atan2, ceil, cos, cosh, exp, fabs, floor,
	fmod, log, log10, pow, sin, sinh, sqrt, tan, tanh
};
#endif

/* Special free list -- see comments for same code in intobject.c. */
#define BLOCK_SIZE	1000	/* 1K less typical malloc overhead */
#define BHEAD_SIZE	8	/* Enough for a 64-bit pointer */
#define N_FLOATOBJECTS	((BLOCK_SIZE - BHEAD_SIZE) / sizeof(PyFloatObject))

struct _floatblock {
	struct _floatblock *next;
	PyFloatObject objects[N_FLOATOBJECTS];
};

typedef struct _floatblock PyFloatBlock;

static PyFloatBlock *block_list = NULL;
static PyFloatObject *free_list = NULL;

static PyFloatObject *
fill_free_list(void)
{
	PyFloatObject *p, *q;
	/* XXX Float blocks escape the object heap. Use PyObject_MALLOC ??? */
	p = (PyFloatObject *) PyMem_MALLOC(sizeof(PyFloatBlock));
	if (p == NULL)
		return (PyFloatObject *) PyErr_NoMemory();
	((PyFloatBlock *)p)->next = block_list;
	block_list = (PyFloatBlock *)p;
	p = &((PyFloatBlock *)p)->objects[0];
	q = p + N_FLOATOBJECTS;
	while (--q > p)
		q->ob_type = (struct _typeobject *)(q-1);
	q->ob_type = NULL;
	return p + N_FLOATOBJECTS - 1;
}

PyObject *
PyFloat_FromDouble(double fval)
{
	register PyFloatObject *op;
	if (free_list == NULL) {
		if ((free_list = fill_free_list()) == NULL)
			return NULL;
	}
	/* PyObject_New is inlined */
	op = free_list;
	free_list = (PyFloatObject *)op->ob_type;
	PyObject_INIT(op, &PyFloat_Type);
	op->ob_fval = fval;
	return (PyObject *) op;
}

/**************************************************************************
RED_FLAG 22-Sep-2000 tim
PyFloat_FromString's pend argument is braindead.  Prior to this RED_FLAG,

1.  If v was a regular string, *pend was set to point to its terminating
    null byte.  That's useless (the caller can find that without any
    help from this function!).

2.  If v was a Unicode string, or an object convertible to a character
    buffer, *pend was set to point into stack trash (the auto temp
    vector holding the character buffer).  That was downright dangerous.

Since we can't change the interface of a public API function, pend is
still supported but now *officially* useless:  if pend is not NULL,
*pend is set to NULL.
**************************************************************************/
PyObject *
PyFloat_FromString(PyObject *v, char **pend)
{
	const char *s, *last, *end;
	double x;
	char buffer[256]; /* for errors */
#ifdef Py_USING_UNICODE
	char s_buffer[256]; /* for objects convertible to a char buffer */
#endif
	int len;

	if (pend)
		*pend = NULL;
	if (PyString_Check(v)) {
		s = PyString_AS_STRING(v);
		len = PyString_GET_SIZE(v);
	}
#ifdef Py_USING_UNICODE
	else if (PyUnicode_Check(v)) {
		if (PyUnicode_GET_SIZE(v) >= sizeof(s_buffer)) {
			PyErr_SetString(PyExc_ValueError,
				"Unicode float() literal too long to convert");
			return NULL;
		}
		if (PyUnicode_EncodeDecimal(PyUnicode_AS_UNICODE(v),
					    PyUnicode_GET_SIZE(v),
					    s_buffer,
					    NULL))
			return NULL;
		s = s_buffer;
		len = (int)strlen(s);
	}
#endif
	else if (PyObject_AsCharBuffer(v, &s, &len)) {
		PyErr_SetString(PyExc_TypeError,
				"float() argument must be a string or a number");
		return NULL;
	}

	last = s + len;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (*s == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for float()");
		return NULL;
	}
	/* We don't care about overflow or underflow.  If the platform supports
	 * them, infinities and signed zeroes (on underflow) are fine.
	 * However, strtod can return 0 for denormalized numbers, where atof
	 * does not.  So (alas!) we special-case a zero result.  Note that
	 * whether strtod sets errno on underflow is not defined, so we can't
	 * key off errno.
         */
	PyFPE_START_PROTECT("strtod", return NULL)
	x = strtod(s, (char **)&end);
	PyFPE_END_PROTECT(x)
	errno = 0;
	/* Believe it or not, Solaris 2.6 can move end *beyond* the null
	   byte at the end of the string, when the input is inf(inity). */
	if (end > last)
		end = last;
	if (end == s) {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for float(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	/* Since end != s, the platform made *some* kind of sense out
	   of the input.  Trust it. */
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for float(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (end != last) {
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for float()");
		return NULL;
	}
	if (x == 0.0) {
		/* See above -- may have been strtod being anal
		   about denorms. */
		PyFPE_START_PROTECT("atof", return NULL)
		x = atof(s);
		PyFPE_END_PROTECT(x)
		errno = 0;    /* whether atof ever set errno is undefined */
	}
	return PyFloat_FromDouble(x);
}

static void
float_dealloc(PyFloatObject *op)
{
	if (PyFloat_CheckExact(op)) {
		op->ob_type = (struct _typeobject *)free_list;
		free_list = op;
	}
	else
		op->ob_type->tp_free((PyObject *)op);
}

double
PyFloat_AsDouble(PyObject *op)
{
	PyNumberMethods *nb;
	PyFloatObject *fo;
	double val;

	if (op && PyFloat_Check(op))
		return PyFloat_AS_DOUBLE((PyFloatObject*) op);

	if (op == NULL || (nb = op->ob_type->tp_as_number) == NULL ||
	    nb->nb_float == NULL) {
		PyErr_BadArgument();
		return -1;
	}

	fo = (PyFloatObject*) (*nb->nb_float) (op);
	if (fo == NULL)
		return -1;
	if (!PyFloat_Check(fo)) {
		PyErr_SetString(PyExc_TypeError,
				"nb_float should return float object");
		return -1;
	}

	val = PyFloat_AS_DOUBLE(fo);
	Py_DECREF(fo);

	return val;
}

/* Methods */

static void
format_float(char *buf, size_t buflen, PyFloatObject *v, int precision)
{
	register char *cp;
	/* Subroutine for float_repr and float_print.
	   We want float numbers to be recognizable as such,
	   i.e., they should contain a decimal point or an exponent.
	   However, %g may print the number as an integer;
	   in such cases, we append ".0" to the string. */

	assert(PyFloat_Check(v));
	PyOS_snprintf(buf, buflen, "%.*g", precision, v->ob_fval);
	cp = buf;
	if (*cp == '-')
		cp++;
	for (; *cp != '\0'; cp++) {
		/* Any non-digit means it's not an integer;
		   this takes care of NAN and INF as well. */
		if (!isdigit(Py_CHARMASK(*cp)))
			break;
	}
	if (*cp == '\0') {
		*cp++ = '.';
		*cp++ = '0';
		*cp++ = '\0';
	}
}

/* XXX PyFloat_AsStringEx should not be a public API function (for one
   XXX thing, its signature passes a buffer without a length; for another,
   XXX it isn't useful outside this file).
*/
void
PyFloat_AsStringEx(char *buf, PyFloatObject *v, int precision)
{
	format_float(buf, 100, v, precision);
}

/* Macro and helper that convert PyObject obj to a C double and store
   the value in dbl; this replaces the functionality of the coercion
   slot function.  If conversion to double raises an exception, obj is
   set to NULL, and the function invoking this macro returns NULL.  If
   obj is not of float, int or long type, Py_NotImplemented is incref'ed,
   stored in obj, and returned from the function invoking this macro.
*/
#define CONVERT_TO_DOUBLE(obj, dbl)			\
	if (PyFloat_Check(obj))				\
		dbl = PyFloat_AS_DOUBLE(obj);		\
	else if (convert_to_double(&(obj), &(dbl)) < 0)	\
		return obj;

static int
convert_to_double(PyObject **v, double *dbl)
{
	register PyObject *obj = *v;

	if (PyInt_Check(obj)) {
		*dbl = (double)PyInt_AS_LONG(obj);
	}
	else if (PyLong_Check(obj)) {
		*dbl = PyLong_AsDouble(obj);
		if (*dbl == -1.0 && PyErr_Occurred()) {
			*v = NULL;
			return -1;
		}
	}
	else {
		Py_INCREF(Py_NotImplemented);
		*v = Py_NotImplemented;
		return -1;
	}
	return 0;
}

/* Precisions used by repr() and str(), respectively.

   The repr() precision (17 significant decimal digits) is the minimal number
   that is guaranteed to have enough precision so that if the number is read
   back in the exact same binary value is recreated.  This is true for IEEE
   floating point by design, and also happens to work for all other modern
   hardware.

   The str() precision is chosen so that in most cases, the rounding noise
   created by various operations is suppressed, while giving plenty of
   precision for practical use.

*/

#define PREC_REPR	17
#define PREC_STR	12

/* XXX PyFloat_AsString and PyFloat_AsReprString should be deprecated:
   XXX they pass a char buffer without passing a length.
*/
void
PyFloat_AsString(char *buf, PyFloatObject *v)
{
	format_float(buf, 100, v, PREC_STR);
}

void
PyFloat_AsReprString(char *buf, PyFloatObject *v)
{
	format_float(buf, 100, v, PREC_REPR);
}

/* ARGSUSED */
static int
float_print(PyFloatObject *v, FILE *fp, int flags)
{
	char buf[100];
	format_float(buf, sizeof(buf), v,
		     (flags & Py_PRINT_RAW) ? PREC_STR : PREC_REPR);
	fputs(buf, fp);
	return 0;
}

static PyObject *
float_repr(PyFloatObject *v)
{
	char buf[100];
	format_float(buf, sizeof(buf), v, PREC_REPR);
	return PyString_FromString(buf);
}

static PyObject *
float_str(PyFloatObject *v)
{
	char buf[100];
	format_float(buf, sizeof(buf), v, PREC_STR);
	return PyString_FromString(buf);
}

static int
float_compare(PyFloatObject *v, PyFloatObject *w)
{
	double i = v->ob_fval;
	double j = w->ob_fval;
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long
float_hash(PyFloatObject *v)
{
	return _Py_HashDouble(v->ob_fval);
}

static PyObject *
float_add(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	PyFPE_START_PROTECT("add", return 0)
	a = a + b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *
float_sub(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	PyFPE_START_PROTECT("subtract", return 0)
	a = a - b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *
float_mul(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	PyFPE_START_PROTECT("multiply", return 0)
	a = a * b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *
float_div(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	if (b == 0.0) {
		PyErr_SetString(PyExc_ZeroDivisionError, "float division");
		return NULL;
	}
	PyFPE_START_PROTECT("divide", return 0)
	a = a / b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *
float_classic_div(PyObject *v, PyObject *w)
{
	double a,b;
	CONVERT_TO_DOUBLE(v, a);
	CONVERT_TO_DOUBLE(w, b);
	if (Py_DivisionWarningFlag >= 2 &&
	    PyErr_Warn(PyExc_DeprecationWarning, "classic float division") < 0)
		return NULL;
	if (b == 0.0) {
		PyErr_SetString(PyExc_ZeroDivisionError, "float division");
		return NULL;
	}
	PyFPE_START_PROTECT("divide", return 0)
	a = a / b;
	PyFPE_END_PROTECT(a)
	return PyFloat_FromDouble(a);
}

static PyObject *
float_rem(PyObject *v, PyObject *w)
{
	double vx, wx;
	double mod;
 	CONVERT_TO_DOUBLE(v, vx);
 	CONVERT_TO_DOUBLE(w, wx);
	if (wx == 0.0) {
		PyErr_SetString(PyExc_ZeroDivisionError, "float modulo");
		return NULL;
	}
	PyFPE_START_PROTECT("modulo", return 0)
	mod = fmod(vx, wx);
	/* note: checking mod*wx < 0 is incorrect -- underflows to
	   0 if wx < sqrt(smallest nonzero double) */
	if (mod && ((wx < 0) != (mod < 0))) {
		mod += wx;
	}
	PyFPE_END_PROTECT(mod)
	return PyFloat_FromDouble(mod);
}

static PyObject *
float_divmod(PyObject *v, PyObject *w)
{
	double vx, wx;
	double div, mod, floordiv;
 	CONVERT_TO_DOUBLE(v, vx);
 	CONVERT_TO_DOUBLE(w, wx);
	if (wx == 0.0) {
		PyErr_SetString(PyExc_ZeroDivisionError, "float divmod()");
		return NULL;
	}
	PyFPE_START_PROTECT("divmod", return 0)
	mod = fmod(vx, wx);
	/* fmod is typically exact, so vx-mod is *mathematically* an
	   exact multiple of wx.  But this is fp arithmetic, and fp
	   vx - mod is an approximation; the result is that div may
	   not be an exact integral value after the division, although
	   it will always be very close to one.
	*/
	div = (vx - mod) / wx;
	if (mod) {
		/* ensure the remainder has the same sign as the denominator */
		if ((wx < 0) != (mod < 0)) {
			mod += wx;
			div -= 1.0;
		}
	}
	else {
		/* the remainder is zero, and in the presence of signed zeroes
		   fmod returns different results across platforms; ensure
		   it has the same sign as the denominator; we'd like to do
		   "mod = wx * 0.0", but that may get optimized away */
		mod *= mod;  /* hide "mod = +0" from optimizer */
		if (wx < 0.0)
			mod = -mod;
	}
	/* snap quotient to nearest integral value */
	if (div) {
		floordiv = floor(div);
		if (div - floordiv > 0.5)
			floordiv += 1.0;
	}
	else {
		/* div is zero - get the same sign as the true quotient */
		div *= div;	/* hide "div = +0" from optimizers */
		floordiv = div * vx / wx; /* zero w/ sign of vx/wx */
	}
	PyFPE_END_PROTECT(floordiv)
	return Py_BuildValue("(dd)", floordiv, mod);
}

static PyObject *
float_floor_div(PyObject *v, PyObject *w)
{
	PyObject *t, *r;

	t = float_divmod(v, w);
	if (t == NULL || t == Py_NotImplemented)
		return t;
	assert(PyTuple_CheckExact(t));
	r = PyTuple_GET_ITEM(t, 0);
	Py_INCREF(r);
	Py_DECREF(t);
	return r;
}

static PyObject *
float_pow(PyObject *v, PyObject *w, PyObject *z)
{
	double iv, iw, ix;

	if ((PyObject *)z != Py_None) {
		PyErr_SetString(PyExc_TypeError, "pow() 3rd argument not "
			"allowed unless all arguments are integers");
		return NULL;
	}

	CONVERT_TO_DOUBLE(v, iv);
	CONVERT_TO_DOUBLE(w, iw);

	/* Sort out special cases here instead of relying on pow() */
	if (iw == 0) { 		/* v**0 is 1, even 0**0 */
		PyFPE_START_PROTECT("pow", return NULL)
		if ((PyObject *)z != Py_None) {
			double iz;
			CONVERT_TO_DOUBLE(z, iz);
			ix = fmod(1.0, iz);
			if (ix != 0 && iz < 0)
				ix += iz;
		}
		else
			ix = 1.0;
		PyFPE_END_PROTECT(ix)
		return PyFloat_FromDouble(ix);
	}
	if (iv == 0.0) {  /* 0**w is error if w<0, else 1 */
		if (iw < 0.0) {
			PyErr_SetString(PyExc_ZeroDivisionError,
					"0.0 cannot be raised to a negative power");
			return NULL;
		}
		return PyFloat_FromDouble(0.0);
	}
	if (iv < 0.0 && iw != floor(iw)) {
		PyErr_SetString(PyExc_ValueError,
				"negative number cannot be raised to a fractional power");
		return NULL;
	}
	errno = 0;
	PyFPE_START_PROTECT("pow", return NULL)
	ix = pow(iv, iw);
	PyFPE_END_PROTECT(ix)
	Py_ADJUST_ERANGE1(ix);
	if (errno != 0) {
		assert(errno == ERANGE);
		PyErr_SetFromErrno(PyExc_OverflowError);
		return NULL;
	}
	return PyFloat_FromDouble(ix);
}

static PyObject *
float_neg(PyFloatObject *v)
{
	return PyFloat_FromDouble(-v->ob_fval);
}

static PyObject *
float_pos(PyFloatObject *v)
{
	if (PyFloat_CheckExact(v)) {
		Py_INCREF(v);
		return (PyObject *)v;
	}
	else
		return PyFloat_FromDouble(v->ob_fval);
}

static PyObject *
float_abs(PyFloatObject *v)
{
	return PyFloat_FromDouble(fabs(v->ob_fval));
}

static int
float_nonzero(PyFloatObject *v)
{
	return v->ob_fval != 0.0;
}

static int
float_coerce(PyObject **pv, PyObject **pw)
{
	if (PyInt_Check(*pw)) {
		long x = PyInt_AsLong(*pw);
		*pw = PyFloat_FromDouble((double)x);
		Py_INCREF(*pv);
		return 0;
	}
	else if (PyLong_Check(*pw)) {
		*pw = PyFloat_FromDouble(PyLong_AsDouble(*pw));
		Py_INCREF(*pv);
		return 0;
	}
	else if (PyFloat_Check(*pw)) {
		Py_INCREF(*pv);
		Py_INCREF(*pw);
		return 0;
	}
	return 1; /* Can't do it */
}

static PyObject *
float_int(PyObject *v)
{
	double x = PyFloat_AsDouble(v);
	double wholepart;	/* integral portion of x, rounded toward 0 */
	long aslong;		/* (long)wholepart */

	(void)modf(x, &wholepart);
#ifdef RISCOS
	/* conversion from floating to integral type would raise exception */
	if (wholepart>LONG_MAX || wholepart<LONG_MIN) {
		PyErr_SetString(PyExc_OverflowError, "float too large to convert");
		return NULL;
	}
#endif
	/* doubles may have more bits than longs, or vice versa; and casting
	   to long may yield gibberish in either case.  What really matters
	   is whether converting back to double again reproduces what we
	   started with. */
	aslong = (long)wholepart;
	if ((double)aslong == wholepart)
		return PyInt_FromLong(aslong);
	PyErr_SetString(PyExc_OverflowError, "float too large to convert");
	return NULL;
}

static PyObject *
float_long(PyObject *v)
{
	double x = PyFloat_AsDouble(v);
	return PyLong_FromDouble(x);
}

static PyObject *
float_float(PyObject *v)
{
	Py_INCREF(v);
	return v;
}


staticforward PyObject *
float_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
float_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *x = Py_False; /* Integer zero */
	static char *kwlist[] = {"x", 0};

	if (type != &PyFloat_Type)
		return float_subtype_new(type, args, kwds); /* Wimp out */
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:float", kwlist, &x))
		return NULL;
	if (PyString_Check(x))
		return PyFloat_FromString(x, NULL);
	return PyNumber_Float(x);
}

/* Wimpy, slow approach to tp_new calls for subtypes of float:
   first create a regular float from whatever arguments we got,
   then allocate a subtype instance and initialize its ob_fval
   from the regular float.  The regular float is then thrown away.
*/
static PyObject *
float_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *tmp, *new;

	assert(PyType_IsSubtype(type, &PyFloat_Type));
	tmp = float_new(&PyFloat_Type, args, kwds);
	if (tmp == NULL)
		return NULL;
	assert(PyFloat_CheckExact(tmp));
	new = type->tp_alloc(type, 0);
	if (new == NULL)
		return NULL;
	((PyFloatObject *)new)->ob_fval = ((PyFloatObject *)tmp)->ob_fval;
	Py_DECREF(tmp);
	return new;
}

PyDoc_STRVAR(float_doc,
"float(x) -> floating point number\n\
\n\
Convert a string or number to a floating point number, if possible.");


static PyNumberMethods float_as_number = {
	(binaryfunc)float_add, /*nb_add*/
	(binaryfunc)float_sub, /*nb_subtract*/
	(binaryfunc)float_mul, /*nb_multiply*/
	(binaryfunc)float_classic_div, /*nb_divide*/
	(binaryfunc)float_rem, /*nb_remainder*/
	(binaryfunc)float_divmod, /*nb_divmod*/
	(ternaryfunc)float_pow, /*nb_power*/
	(unaryfunc)float_neg, /*nb_negative*/
	(unaryfunc)float_pos, /*nb_positive*/
	(unaryfunc)float_abs, /*nb_absolute*/
	(inquiry)float_nonzero, /*nb_nonzero*/
	0,		/*nb_invert*/
	0,		/*nb_lshift*/
	0,		/*nb_rshift*/
	0,		/*nb_and*/
	0,		/*nb_xor*/
	0,		/*nb_or*/
	(coercion)float_coerce, /*nb_coerce*/
	(unaryfunc)float_int, /*nb_int*/
	(unaryfunc)float_long, /*nb_long*/
	(unaryfunc)float_float, /*nb_float*/
	0,		/* nb_oct */
	0,		/* nb_hex */
	0,		/* nb_inplace_add */
	0,		/* nb_inplace_subtract */
	0,		/* nb_inplace_multiply */
	0,		/* nb_inplace_divide */
	0,		/* nb_inplace_remainder */
	0, 		/* nb_inplace_power */
	0,		/* nb_inplace_lshift */
	0,		/* nb_inplace_rshift */
	0,		/* nb_inplace_and */
	0,		/* nb_inplace_xor */
	0,		/* nb_inplace_or */
	float_floor_div, /* nb_floor_divide */
	float_div,	/* nb_true_divide */
	0,		/* nb_inplace_floor_divide */
	0,		/* nb_inplace_true_divide */
};

PyTypeObject PyFloat_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"float",
	sizeof(PyFloatObject),
	0,
	(destructor)float_dealloc,		/* tp_dealloc */
	(printfunc)float_print, 		/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	(cmpfunc)float_compare, 		/* tp_compare */
	(reprfunc)float_repr,			/* tp_repr */
	&float_as_number,			/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)float_hash,			/* tp_hash */
	0,					/* tp_call */
	(reprfunc)float_str,			/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE,		/* tp_flags */
	float_doc,				/* tp_doc */
 	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	float_new,				/* tp_new */
};

void
PyFloat_Fini(void)
{
	PyFloatObject *p;
	PyFloatBlock *list, *next;
	int i;
	int bc, bf;	/* block count, number of freed blocks */
	int frem, fsum;	/* remaining unfreed floats per block, total */

	bc = 0;
	bf = 0;
	fsum = 0;
	list = block_list;
	block_list = NULL;
	free_list = NULL;
	while (list != NULL) {
		bc++;
		frem = 0;
		for (i = 0, p = &list->objects[0];
		     i < N_FLOATOBJECTS;
		     i++, p++) {
			if (PyFloat_CheckExact(p) && p->ob_refcnt != 0)
				frem++;
		}
		next = list->next;
		if (frem) {
			list->next = block_list;
			block_list = list;
			for (i = 0, p = &list->objects[0];
			     i < N_FLOATOBJECTS;
			     i++, p++) {
				if (!PyFloat_CheckExact(p) ||
				    p->ob_refcnt == 0) {
					p->ob_type = (struct _typeobject *)
						free_list;
					free_list = p;
				}
			}
		}
		else {
			PyMem_FREE(list); /* XXX PyObject_FREE ??? */
			bf++;
		}
		fsum += frem;
		list = next;
	}
	if (!Py_VerboseFlag)
		return;
	fprintf(stderr, "# cleanup floats");
	if (!fsum) {
		fprintf(stderr, "\n");
	}
	else {
		fprintf(stderr,
			": %d unfreed float%s in %d out of %d block%s\n",
			fsum, fsum == 1 ? "" : "s",
			bc - bf, bc, bc == 1 ? "" : "s");
	}
	if (Py_VerboseFlag > 1) {
		list = block_list;
		while (list != NULL) {
			for (i = 0, p = &list->objects[0];
			     i < N_FLOATOBJECTS;
			     i++, p++) {
				if (PyFloat_CheckExact(p) &&
				    p->ob_refcnt != 0) {
					char buf[100];
					PyFloat_AsString(buf, p);
					fprintf(stderr,
			     "#   <float at %p, refcnt=%d, val=%s>\n",
						p, p->ob_refcnt, buf);
				}
			}
			list = list->next;
		}
	}
}
