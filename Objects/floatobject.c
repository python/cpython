
/* Float object implementation */

/* XXX There should be overflow checks here, but it's hard to check
   for any kind of float exception without losing portability. */

#include "Python.h"

#include "formatter_unicode.h"

#include <ctype.h>

#if !defined(__STDC__)
extern double fmod(double, double);
extern double pow(double, double);
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
		Py_Type(q) = (struct _typeobject *)(q-1);
	Py_Type(q) = NULL;
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
	/* Inline PyObject_New */
	op = free_list;
	free_list = (PyFloatObject *)Py_Type(op);
	PyObject_INIT(op, &PyFloat_Type);
	op->ob_fval = fval;
	return (PyObject *) op;
}

PyObject *
PyFloat_FromString(PyObject *v)
{
	const char *s, *last, *end;
	double x;
	char buffer[256]; /* for errors */
	char *s_buffer = NULL;
	Py_ssize_t len;
	PyObject *result = NULL;

	if (PyUnicode_Check(v)) {
		s_buffer = (char *)PyMem_MALLOC(PyUnicode_GET_SIZE(v)+1);
		if (s_buffer == NULL)
			return PyErr_NoMemory();
		if (PyUnicode_EncodeDecimal(PyUnicode_AS_UNICODE(v),
					    PyUnicode_GET_SIZE(v),
					    s_buffer,
					    NULL))
			goto error;
		s = s_buffer;
		len = strlen(s);
	}
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
		goto error;
	}
	/* We don't care about overflow or underflow.  If the platform supports
	 * them, infinities and signed zeroes (on underflow) are fine.
	 * However, strtod can return 0 for denormalized numbers, where atof
	 * does not.  So (alas!) we special-case a zero result.  Note that
	 * whether strtod sets errno on underflow is not defined, so we can't
	 * key off errno.
         */
	PyFPE_START_PROTECT("strtod", goto error)
	x = PyOS_ascii_strtod(s, (char **)&end);
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
		goto error;
	}
	/* Since end != s, the platform made *some* kind of sense out
	   of the input.  Trust it. */
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "invalid literal for float(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		goto error;
	}
	else if (end != last) {
		PyErr_SetString(PyExc_ValueError,
				"null byte in argument for float()");
		goto error;
	}
	if (x == 0.0) {
		/* See above -- may have been strtod being anal
		   about denorms. */
		PyFPE_START_PROTECT("atof", goto error)
		x = PyOS_ascii_atof(s);
		PyFPE_END_PROTECT(x)
		errno = 0;    /* whether atof ever set errno is undefined */
	}
	result = PyFloat_FromDouble(x);
  error:
	if (s_buffer)
		PyMem_FREE(s_buffer);
	return result;
}

static void
float_dealloc(PyFloatObject *op)
{
	if (PyFloat_CheckExact(op)) {
		Py_Type(op) = (struct _typeobject *)free_list;
		free_list = op;
	}
	else
		Py_Type(op)->tp_free((PyObject *)op);
}

double
PyFloat_AsDouble(PyObject *op)
{
	PyNumberMethods *nb;
	PyFloatObject *fo;
	double val;

	if (op && PyFloat_Check(op))
		return PyFloat_AS_DOUBLE((PyFloatObject*) op);

	if (op == NULL) {
		PyErr_BadArgument();
		return -1;
	}

	if ((nb = Py_Type(op)->tp_as_number) == NULL || nb->nb_float == NULL) {
		PyErr_SetString(PyExc_TypeError, "a float is required");
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
format_double(char *buf, size_t buflen, double ob_fval, int precision)
{
	register char *cp;
	char format[32];
	/* Subroutine for float_repr, float_str, and others.
	   We want float numbers to be recognizable as such,
	   i.e., they should contain a decimal point or an exponent.
	   However, %g may print the number as an integer;
	   in such cases, we append ".0" to the string. */

	PyOS_snprintf(format, 32, "%%.%ig", precision);
	PyOS_ascii_formatd(buf, buflen, format, ob_fval);
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

static void
format_float(char *buf, size_t buflen, PyFloatObject *v, int precision)
{
	assert(PyFloat_Check(v));
	format_double(buf, buflen, PyFloat_AS_DOUBLE(v), precision);
}

/* Macro and helper that convert PyObject obj to a C double and store
   the value in dbl.  If conversion to double raises an exception, obj is
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

	if (PyLong_Check(obj)) {
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

static PyObject *
float_repr(PyFloatObject *v)
{
	char buf[100];
	format_float(buf, sizeof(buf), v, PREC_REPR);
	return PyUnicode_FromString(buf);
}

static PyObject *
float_str(PyFloatObject *v)
{
	char buf[100];
	format_float(buf, sizeof(buf), v, PREC_STR);
	return PyUnicode_FromString(buf);
}

/* Comparison is pretty much a nightmare.  When comparing float to float,
 * we do it as straightforwardly (and long-windedly) as conceivable, so
 * that, e.g., Python x == y delivers the same result as the platform
 * C x == y when x and/or y is a NaN.
 * When mixing float with an integer type, there's no good *uniform* approach.
 * Converting the double to an integer obviously doesn't work, since we
 * may lose info from fractional bits.  Converting the integer to a double
 * also has two failure modes:  (1) a long int may trigger overflow (too
 * large to fit in the dynamic range of a C double); (2) even a C long may have
 * more bits than fit in a C double (e.g., on a a 64-bit box long may have
 * 63 bits of precision, but a C double probably has only 53), and then
 * we can falsely claim equality when low-order integer bits are lost by
 * coercion to double.  So this part is painful too.
 */

static PyObject*
float_richcompare(PyObject *v, PyObject *w, int op)
{
	double i, j;
	int r = 0;

	assert(PyFloat_Check(v));
	i = PyFloat_AS_DOUBLE(v);

	/* Switch on the type of w.  Set i and j to doubles to be compared,
	 * and op to the richcomp to use.
	 */
	if (PyFloat_Check(w))
		j = PyFloat_AS_DOUBLE(w);

	else if (!Py_IS_FINITE(i)) {
		if (PyLong_Check(w))
			/* If i is an infinity, its magnitude exceeds any
			 * finite integer, so it doesn't matter which int we
			 * compare i with.  If i is a NaN, similarly.
			 */
			j = 0.0;
		else
			goto Unimplemented;
	}

	else if (PyLong_Check(w)) {
		int vsign = i == 0.0 ? 0 : i < 0.0 ? -1 : 1;
		int wsign = _PyLong_Sign(w);
		size_t nbits;
		int exponent;

		if (vsign != wsign) {
			/* Magnitudes are irrelevant -- the signs alone
			 * determine the outcome.
			 */
			i = (double)vsign;
			j = (double)wsign;
			goto Compare;
		}
		/* The signs are the same. */
		/* Convert w to a double if it fits.  In particular, 0 fits. */
		nbits = _PyLong_NumBits(w);
		if (nbits == (size_t)-1 && PyErr_Occurred()) {
			/* This long is so large that size_t isn't big enough
			 * to hold the # of bits.  Replace with little doubles
			 * that give the same outcome -- w is so large that
			 * its magnitude must exceed the magnitude of any
			 * finite float.
			 */
			PyErr_Clear();
			i = (double)vsign;
			assert(wsign != 0);
			j = wsign * 2.0;
			goto Compare;
		}
		if (nbits <= 48) {
			j = PyLong_AsDouble(w);
			/* It's impossible that <= 48 bits overflowed. */
			assert(j != -1.0 || ! PyErr_Occurred());
			goto Compare;
		}
		assert(wsign != 0); /* else nbits was 0 */
		assert(vsign != 0); /* if vsign were 0, then since wsign is
		                     * not 0, we would have taken the
		                     * vsign != wsign branch at the start */
		/* We want to work with non-negative numbers. */
		if (vsign < 0) {
			/* "Multiply both sides" by -1; this also swaps the
			 * comparator.
			 */
			i = -i;
			op = _Py_SwappedOp[op];
		}
		assert(i > 0.0);
		(void) frexp(i, &exponent);
		/* exponent is the # of bits in v before the radix point;
		 * we know that nbits (the # of bits in w) > 48 at this point
		 */
		if (exponent < 0 || (size_t)exponent < nbits) {
			i = 1.0;
			j = 2.0;
			goto Compare;
		}
		if ((size_t)exponent > nbits) {
			i = 2.0;
			j = 1.0;
			goto Compare;
		}
		/* v and w have the same number of bits before the radix
		 * point.  Construct two longs that have the same comparison
		 * outcome.
		 */
		{
			double fracpart;
			double intpart;
			PyObject *result = NULL;
			PyObject *one = NULL;
			PyObject *vv = NULL;
			PyObject *ww = w;

			if (wsign < 0) {
				ww = PyNumber_Negative(w);
				if (ww == NULL)
					goto Error;
			}
			else
				Py_INCREF(ww);

			fracpart = modf(i, &intpart);
			vv = PyLong_FromDouble(intpart);
			if (vv == NULL)
				goto Error;

			if (fracpart != 0.0) {
				/* Shift left, and or a 1 bit into vv
				 * to represent the lost fraction.
				 */
				PyObject *temp;

				one = PyInt_FromLong(1);
				if (one == NULL)
					goto Error;

				temp = PyNumber_Lshift(ww, one);
				if (temp == NULL)
					goto Error;
				Py_DECREF(ww);
				ww = temp;

				temp = PyNumber_Lshift(vv, one);
				if (temp == NULL)
					goto Error;
				Py_DECREF(vv);
				vv = temp;

				temp = PyNumber_Or(vv, one);
				if (temp == NULL)
					goto Error;
				Py_DECREF(vv);
				vv = temp;
			}

			r = PyObject_RichCompareBool(vv, ww, op);
			if (r < 0)
				goto Error;
			result = PyBool_FromLong(r);
 		 Error:
 		 	Py_XDECREF(vv);
 		 	Py_XDECREF(ww);
 		 	Py_XDECREF(one);
 		 	return result;
		}
	} /* else if (PyLong_Check(w)) */

	else	/* w isn't float, int, or long */
		goto Unimplemented;

 Compare:
	PyFPE_START_PROTECT("richcompare", return NULL)
	switch (op) {
	case Py_EQ:
		r = i == j;
		break;
	case Py_NE:
		r = i != j;
		break;
	case Py_LE:
		r = i <= j;
		break;
	case Py_GE:
		r = i >= j;
		break;
	case Py_LT:
		r = i < j;
		break;
	case Py_GT:
		r = i > j;
		break;
	}
	PyFPE_END_PROTECT(r)
	return PyBool_FromLong(r);

 Unimplemented:
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
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
		return PyFloat_FromDouble(1.0);
	}
	if (iv == 0.0) {  /* 0**w is error if w<0, else 1 */
		if (iw < 0.0) {
			PyErr_SetString(PyExc_ZeroDivisionError,
					"0.0 cannot be raised to a negative power");
			return NULL;
		}
		return PyFloat_FromDouble(0.0);
	}
	if (iv < 0.0) {
		/* Whether this is an error is a mess, and bumps into libm
		 * bugs so we have to figure it out ourselves.
		 */
		if (iw != floor(iw)) {
			/* Negative numbers raised to fractional powers
			 * become complex.
			 */
			return PyComplex_Type.tp_as_number->nb_power(v, w, z);
		}
		/* iw is an exact integer, albeit perhaps a very large one.
		 * -1 raised to an exact integer should never be exceptional.
		 * Alas, some libms (chiefly glibc as of early 2003) return
		 * NaN and set EDOM on pow(-1, large_int) if the int doesn't
		 * happen to be representable in a *C* integer.  That's a
		 * bug; we let that slide in math.pow() (which currently
		 * reflects all platform accidents), but not for Python's **.
		 */
		 if (iv == -1.0 && Py_IS_FINITE(iw)) {
		 	/* Return 1 if iw is even, -1 if iw is odd; there's
		 	 * no guarantee that any C integral type is big
		 	 * enough to hold iw, so we have to check this
		 	 * indirectly.
		 	 */
		 	ix = floor(iw * 0.5) * 2.0;
			return PyFloat_FromDouble(ix == iw ? 1.0 : -1.0);
		}
		/* Else iv != -1.0, and overflow or underflow are possible.
		 * Unless we're to write pow() ourselves, we have to trust
		 * the platform to do this correctly.
		 */
	}
	errno = 0;
	PyFPE_START_PROTECT("pow", return NULL)
	ix = pow(iv, iw);
	PyFPE_END_PROTECT(ix)
	Py_ADJUST_ERANGE1(ix);
	if (errno != 0) {
		/* We don't expect any errno value other than ERANGE, but
		 * the range of libm bugs appears unbounded.
		 */
		PyErr_SetFromErrno(errno == ERANGE ? PyExc_OverflowError :
						     PyExc_ValueError);
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
float_abs(PyFloatObject *v)
{
	return PyFloat_FromDouble(fabs(v->ob_fval));
}

static int
float_bool(PyFloatObject *v)
{
	return v->ob_fval != 0.0;
}

static PyObject *
float_trunc(PyObject *v)
{
	double x = PyFloat_AsDouble(v);
	double wholepart;	/* integral portion of x, rounded toward 0 */

	(void)modf(x, &wholepart);
	/* Try to get out cheap if this fits in a Python int.  The attempt
	 * to cast to long must be protected, as C doesn't define what
	 * happens if the double is too big to fit in a long.  Some rare
	 * systems raise an exception then (RISCOS was mentioned as one,
	 * and someone using a non-default option on Sun also bumped into
	 * that).  Note that checking for >= and <= LONG_{MIN,MAX} would
	 * still be vulnerable:  if a long has more bits of precision than
	 * a double, casting MIN/MAX to double may yield an approximation,
	 * and if that's rounded up, then, e.g., wholepart=LONG_MAX+1 would
	 * yield true from the C expression wholepart<=LONG_MAX, despite
	 * that wholepart is actually greater than LONG_MAX.
	 */
	if (LONG_MIN < wholepart && wholepart < LONG_MAX) {
		const long aslong = (long)wholepart;
		return PyInt_FromLong(aslong);
	}
	return PyLong_FromDouble(wholepart);
}

static PyObject *
float_round(PyObject *v, PyObject *args)
{
#define UNDEF_NDIGITS (-0x7fffffff) /* Unlikely ndigits value */
	double x;
	double f = 1.0;
	double flr, cil;
	double rounded;
	int ndigits = UNDEF_NDIGITS;

	if (!PyArg_ParseTuple(args, "|i", &ndigits))
		return NULL;

	x = PyFloat_AsDouble(v);

	if (ndigits != UNDEF_NDIGITS) {
		f = pow(10.0, ndigits);
		x *= f;
	}

	flr = floor(x);
	cil = ceil(x);

	if (x-flr > 0.5)
		rounded = cil;
	else if (x-flr == 0.5)
		rounded = fmod(flr, 2) == 0 ? flr : cil;
	else
		rounded = flr;

	if (ndigits != UNDEF_NDIGITS) {
		rounded /= f;
		return PyFloat_FromDouble(rounded);
	}

	return PyLong_FromDouble(rounded);
#undef UNDEF_NDIGITS
}

static PyObject *
float_float(PyObject *v)
{
	if (PyFloat_CheckExact(v))
		Py_INCREF(v);
	else
		v = PyFloat_FromDouble(((PyFloatObject *)v)->ob_fval);
	return v;
}


static PyObject *
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
	if (PyUnicode_Check(x))
		return PyFloat_FromString(x);
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
	PyObject *tmp, *newobj;

	assert(PyType_IsSubtype(type, &PyFloat_Type));
	tmp = float_new(&PyFloat_Type, args, kwds);
	if (tmp == NULL)
		return NULL;
	assert(PyFloat_CheckExact(tmp));
	newobj = type->tp_alloc(type, 0);
	if (newobj == NULL) {
		Py_DECREF(tmp);
		return NULL;
	}
	((PyFloatObject *)newobj)->ob_fval = ((PyFloatObject *)tmp)->ob_fval;
	Py_DECREF(tmp);
	return newobj;
}

static PyObject *
float_getnewargs(PyFloatObject *v)
{
	return Py_BuildValue("(d)", v->ob_fval);
}

/* this is for the benefit of the pack/unpack routines below */

typedef enum {
	unknown_format, ieee_big_endian_format, ieee_little_endian_format
} float_format_type;

static float_format_type double_format, float_format;
static float_format_type detected_double_format, detected_float_format;

static PyObject *
float_getformat(PyTypeObject *v, PyObject* arg)
{
	char* s;
	float_format_type r;

	if (!PyUnicode_Check(arg)) {
		PyErr_Format(PyExc_TypeError,
	     "__getformat__() argument must be string, not %.500s",
			     Py_Type(arg)->tp_name);
		return NULL;
	}
	s = PyUnicode_AsString(arg);
	if (s == NULL)
		return NULL;
	if (strcmp(s, "double") == 0) {
		r = double_format;
	}
	else if (strcmp(s, "float") == 0) {
		r = float_format;
	}
	else {
		PyErr_SetString(PyExc_ValueError,
				"__getformat__() argument 1 must be "
				"'double' or 'float'");
		return NULL;
	}
	
	switch (r) {
	case unknown_format:
		return PyUnicode_FromString("unknown");
	case ieee_little_endian_format:
		return PyUnicode_FromString("IEEE, little-endian");
	case ieee_big_endian_format:
		return PyUnicode_FromString("IEEE, big-endian");
	default:
		Py_FatalError("insane float_format or double_format");
		return NULL;
	}
}

PyDoc_STRVAR(float_getformat_doc,
"float.__getformat__(typestr) -> string\n"
"\n"
"You probably don't want to use this function.  It exists mainly to be\n"
"used in Python's test suite.\n"
"\n"
"typestr must be 'double' or 'float'.  This function returns whichever of\n"
"'unknown', 'IEEE, big-endian' or 'IEEE, little-endian' best describes the\n"
"format of floating point numbers used by the C type named by typestr.");

static PyObject *
float_setformat(PyTypeObject *v, PyObject* args)
{
	char* typestr;
	char* format;
	float_format_type f;
	float_format_type detected;
	float_format_type *p;

	if (!PyArg_ParseTuple(args, "ss:__setformat__", &typestr, &format))
		return NULL;

	if (strcmp(typestr, "double") == 0) {
		p = &double_format;
		detected = detected_double_format;
	}
	else if (strcmp(typestr, "float") == 0) {
		p = &float_format;
		detected = detected_float_format;
	}
	else {
		PyErr_SetString(PyExc_ValueError,
				"__setformat__() argument 1 must "
				"be 'double' or 'float'");
		return NULL;
	}
	
	if (strcmp(format, "unknown") == 0) {
		f = unknown_format;
	}
	else if (strcmp(format, "IEEE, little-endian") == 0) {
		f = ieee_little_endian_format;
	}
	else if (strcmp(format, "IEEE, big-endian") == 0) {
		f = ieee_big_endian_format;
	}
	else {
		PyErr_SetString(PyExc_ValueError,
				"__setformat__() argument 2 must be "
				"'unknown', 'IEEE, little-endian' or "
				"'IEEE, big-endian'");
		return NULL;

	}

	if (f != unknown_format && f != detected) {
		PyErr_Format(PyExc_ValueError,
			     "can only set %s format to 'unknown' or the "
			     "detected platform value", typestr);
		return NULL;
	}

	*p = f;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(float_setformat_doc,
"float.__setformat__(typestr, fmt) -> None\n"
"\n"
"You probably don't want to use this function.  It exists mainly to be\n"
"used in Python's test suite.\n"
"\n"
"typestr must be 'double' or 'float'.  fmt must be one of 'unknown',\n"
"'IEEE, big-endian' or 'IEEE, little-endian', and in addition can only be\n"
"one of the latter two if it appears to match the underlying C reality.\n"
"\n"
"Overrides the automatic determination of C-level floating point type.\n"
"This affects how floats are converted to and from binary strings.");

static PyObject *
float_getzero(PyObject *v, void *closure)
{
	return PyFloat_FromDouble(0.0);
}

static PyObject *
float__format__(PyObject *self, PyObject *args)
{
    /* when back porting this to 2.6, check type of the format_spec
       and call either unicode_long__format__ or
       string_long__format__ */
    return unicode_float__format__(self, args);
}

PyDoc_STRVAR(float__format__doc,
"float.__format__(format_spec) -> string\n"
"\n"
"Formats the float according to format_spec.");


static PyMethodDef float_methods[] = {
  	{"conjugate",	(PyCFunction)float_float,	METH_NOARGS,
	 "Returns self, the complex conjugate of any float."},
	{"__trunc__",	(PyCFunction)float_trunc, METH_NOARGS,
         "Returns the Integral closest to x between 0 and x."},
	{"__round__",	(PyCFunction)float_round, METH_VARARGS,
         "Returns the Integral closest to x, rounding half toward even.\n"
         "When an argument is passed, works like built-in round(x, ndigits)."},
	{"__getnewargs__",	(PyCFunction)float_getnewargs,	METH_NOARGS},
	{"__getformat__",	(PyCFunction)float_getformat,	
	 METH_O|METH_CLASS,		float_getformat_doc},
	{"__setformat__",	(PyCFunction)float_setformat,	
	 METH_VARARGS|METH_CLASS,	float_setformat_doc},
        {"__format__",          (PyCFunction)float__format__,
         METH_VARARGS,                  float__format__doc},
	{NULL,		NULL}		/* sentinel */
};

static PyGetSetDef float_getset[] = {
    {"real", 
     (getter)float_float, (setter)NULL,
     "the real part of a complex number",
     NULL},
    {"imag", 
     (getter)float_getzero, (setter)NULL,
     "the imaginary part of a complex number",
     NULL},
    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(float_doc,
"float(x) -> floating point number\n\
\n\
Convert a string or number to a floating point number, if possible.");


static PyNumberMethods float_as_number = {
	float_add, 	/*nb_add*/
	float_sub, 	/*nb_subtract*/
	float_mul, 	/*nb_multiply*/
	float_rem, 	/*nb_remainder*/
	float_divmod, 	/*nb_divmod*/
	float_pow, 	/*nb_power*/
	(unaryfunc)float_neg, /*nb_negative*/
	(unaryfunc)float_float, /*nb_positive*/
	(unaryfunc)float_abs, /*nb_absolute*/
	(inquiry)float_bool, /*nb_bool*/
	0,		/*nb_invert*/
	0,		/*nb_lshift*/
	0,		/*nb_rshift*/
	0,		/*nb_and*/
	0,		/*nb_xor*/
	0,		/*nb_or*/
	0,		/*nb_reserved*/
	float_trunc,	/*nb_int*/
	float_trunc,	/*nb_long*/
	float_float,	/*nb_float*/
	0,		/* nb_oct */
	0,		/* nb_hex */
	0,		/* nb_inplace_add */
	0,		/* nb_inplace_subtract */
	0,		/* nb_inplace_multiply */
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
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"float",
	sizeof(PyFloatObject),
	0,
	(destructor)float_dealloc,		/* tp_dealloc */
	0,			 		/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,			 		/* tp_compare */
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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	float_doc,				/* tp_doc */
 	0,					/* tp_traverse */
	0,					/* tp_clear */
	float_richcompare,			/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	float_methods,				/* tp_methods */
	0,					/* tp_members */
	float_getset,				/* tp_getset */
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
_PyFloat_Init(void)
{
	/* We attempt to determine if this machine is using IEEE
	   floating point formats by peering at the bits of some
	   carefully chosen values.  If it looks like we are on an
	   IEEE platform, the float packing/unpacking routines can
	   just copy bits, if not they resort to arithmetic & shifts
	   and masks.  The shifts & masks approach works on all finite
	   values, but what happens to infinities, NaNs and signed
	   zeroes on packing is an accident, and attempting to unpack
	   a NaN or an infinity will raise an exception.

	   Note that if we're on some whacked-out platform which uses
	   IEEE formats but isn't strictly little-endian or big-
	   endian, we will fall back to the portable shifts & masks
	   method. */

#if SIZEOF_DOUBLE == 8
	{
		double x = 9006104071832581.0;
		if (memcmp(&x, "\x43\x3f\xff\x01\x02\x03\x04\x05", 8) == 0)
			detected_double_format = ieee_big_endian_format;
		else if (memcmp(&x, "\x05\x04\x03\x02\x01\xff\x3f\x43", 8) == 0)
			detected_double_format = ieee_little_endian_format;
		else 
			detected_double_format = unknown_format;
	}
#else
	detected_double_format = unknown_format;
#endif

#if SIZEOF_FLOAT == 4
	{
		float y = 16711938.0;
		if (memcmp(&y, "\x4b\x7f\x01\x02", 4) == 0)
			detected_float_format = ieee_big_endian_format;
		else if (memcmp(&y, "\x02\x01\x7f\x4b", 4) == 0)
			detected_float_format = ieee_little_endian_format;
		else 
			detected_float_format = unknown_format;
	}
#else
	detected_float_format = unknown_format;
#endif

	double_format = detected_double_format;
	float_format = detected_float_format;
}

void
PyFloat_Fini(void)
{
	PyFloatObject *p;
	PyFloatBlock *list, *next;
	unsigned i;
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
			if (PyFloat_CheckExact(p) && Py_Refcnt(p) != 0)
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
				    Py_Refcnt(p) == 0) {
					Py_Type(p) = (struct _typeobject *)
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
				    Py_Refcnt(p) != 0) {
					char buf[100];
					format_float(buf, sizeof(buf), p, PREC_STR);
					/* XXX(twouters) cast refcount to
					   long until %zd is universally
					   available
					 */
					fprintf(stderr,
			     "#   <float at %p, refcnt=%ld, val=%s>\n",
						p, (long)Py_Refcnt(p), buf);
				}
			}
			list = list->next;
		}
	}
}

/*----------------------------------------------------------------------------
 * _PyFloat_{Pack,Unpack}{4,8}.  See floatobject.h.
 *
 * TODO:  On platforms that use the standard IEEE-754 single and double
 * formats natively, these routines could simply copy the bytes.
 */
int
_PyFloat_Pack4(double x, unsigned char *p, int le)
{
	if (float_format == unknown_format) {
		unsigned char sign;
		int e;
		double f;
		unsigned int fbits;
		int incr = 1;

		if (le) {
			p += 3;
			incr = -1;
		}

		if (x < 0) {
			sign = 1;
			x = -x;
		}
		else
			sign = 0;

		f = frexp(x, &e);

		/* Normalize f to be in the range [1.0, 2.0) */
		if (0.5 <= f && f < 1.0) {
			f *= 2.0;
			e--;
		}
		else if (f == 0.0)
			e = 0;
		else {
			PyErr_SetString(PyExc_SystemError,
					"frexp() result out of range");
			return -1;
		}

		if (e >= 128)
			goto Overflow;
		else if (e < -126) {
			/* Gradual underflow */
			f = ldexp(f, 126 + e);
			e = 0;
		}
		else if (!(e == 0 && f == 0.0)) {
			e += 127;
			f -= 1.0; /* Get rid of leading 1 */
		}

		f *= 8388608.0; /* 2**23 */
		fbits = (unsigned int)(f + 0.5); /* Round */
		assert(fbits <= 8388608);
		if (fbits >> 23) {
			/* The carry propagated out of a string of 23 1 bits. */
			fbits = 0;
			++e;
			if (e >= 255)
				goto Overflow;
		}

		/* First byte */
		*p = (sign << 7) | (e >> 1);
		p += incr;

		/* Second byte */
		*p = (char) (((e & 1) << 7) | (fbits >> 16));
		p += incr;

		/* Third byte */
		*p = (fbits >> 8) & 0xFF;
		p += incr;

		/* Fourth byte */
		*p = fbits & 0xFF;

		/* Done */
		return 0;

	  Overflow:
		PyErr_SetString(PyExc_OverflowError,
				"float too large to pack with f format");
		return -1;
	}
	else {
		float y = (float)x;
		const char *s = (char*)&y;
		int i, incr = 1;

		if ((float_format == ieee_little_endian_format && !le)
		    || (float_format == ieee_big_endian_format && le)) {
			p += 3;
			incr = -1;
		}
		
		for (i = 0; i < 4; i++) {
			*p = *s++;
			p += incr;
		}
		return 0;
	}
}

int
_PyFloat_Pack8(double x, unsigned char *p, int le)
{
	if (double_format == unknown_format) {
		unsigned char sign;
		int e;
		double f;
		unsigned int fhi, flo;
		int incr = 1;

		if (le) {
			p += 7;
			incr = -1;
		}

		if (x < 0) {
			sign = 1;
			x = -x;
		}
		else
			sign = 0;

		f = frexp(x, &e);

		/* Normalize f to be in the range [1.0, 2.0) */
		if (0.5 <= f && f < 1.0) {
			f *= 2.0;
			e--;
		}
		else if (f == 0.0)
			e = 0;
		else {
			PyErr_SetString(PyExc_SystemError,
					"frexp() result out of range");
			return -1;
		}

		if (e >= 1024)
			goto Overflow;
		else if (e < -1022) {
			/* Gradual underflow */
			f = ldexp(f, 1022 + e);
			e = 0;
		}
		else if (!(e == 0 && f == 0.0)) {
			e += 1023;
			f -= 1.0; /* Get rid of leading 1 */
		}

		/* fhi receives the high 28 bits; flo the low 24 bits (== 52 bits) */
		f *= 268435456.0; /* 2**28 */
		fhi = (unsigned int)f; /* Truncate */
		assert(fhi < 268435456);

		f -= (double)fhi;
		f *= 16777216.0; /* 2**24 */
		flo = (unsigned int)(f + 0.5); /* Round */
		assert(flo <= 16777216);
		if (flo >> 24) {
			/* The carry propagated out of a string of 24 1 bits. */
			flo = 0;
			++fhi;
			if (fhi >> 28) {
				/* And it also progagated out of the next 28 bits. */
				fhi = 0;
				++e;
				if (e >= 2047)
					goto Overflow;
			}
		}

		/* First byte */
		*p = (sign << 7) | (e >> 4);
		p += incr;

		/* Second byte */
		*p = (unsigned char) (((e & 0xF) << 4) | (fhi >> 24));
		p += incr;

		/* Third byte */
		*p = (fhi >> 16) & 0xFF;
		p += incr;

		/* Fourth byte */
		*p = (fhi >> 8) & 0xFF;
		p += incr;

		/* Fifth byte */
		*p = fhi & 0xFF;
		p += incr;

		/* Sixth byte */
		*p = (flo >> 16) & 0xFF;
		p += incr;

		/* Seventh byte */
		*p = (flo >> 8) & 0xFF;
		p += incr;

		/* Eighth byte */
		*p = flo & 0xFF;
		p += incr;

		/* Done */
		return 0;

	  Overflow:
		PyErr_SetString(PyExc_OverflowError,
				"float too large to pack with d format");
		return -1;
	}
	else {
		const char *s = (char*)&x;
		int i, incr = 1;

		if ((double_format == ieee_little_endian_format && !le)
		    || (double_format == ieee_big_endian_format && le)) {
			p += 7;
			incr = -1;
		}
		
		for (i = 0; i < 8; i++) {
			*p = *s++;
			p += incr;
		}
		return 0;
	}
}

/* Should only be used by marshal. */
int
_PyFloat_Repr(double x, char *p, size_t len)
{
	format_double(p, len, x, PREC_REPR);
	return (int)strlen(p);
}

double
_PyFloat_Unpack4(const unsigned char *p, int le)
{
	if (float_format == unknown_format) {
		unsigned char sign;
		int e;
		unsigned int f;
		double x;
		int incr = 1;

		if (le) {
			p += 3;
			incr = -1;
		}

		/* First byte */
		sign = (*p >> 7) & 1;
		e = (*p & 0x7F) << 1;
		p += incr;

		/* Second byte */
		e |= (*p >> 7) & 1;
		f = (*p & 0x7F) << 16;
		p += incr;

		if (e == 255) {
			PyErr_SetString(
				PyExc_ValueError,
				"can't unpack IEEE 754 special value "
				"on non-IEEE platform");
			return -1;
		}

		/* Third byte */
		f |= *p << 8;
		p += incr;

		/* Fourth byte */
		f |= *p;

		x = (double)f / 8388608.0;

		/* XXX This sadly ignores Inf/NaN issues */
		if (e == 0)
			e = -126;
		else {
			x += 1.0;
			e -= 127;
		}
		x = ldexp(x, e);

		if (sign)
			x = -x;

		return x;
	}
	else {
		float x;

		if ((float_format == ieee_little_endian_format && !le)
		    || (float_format == ieee_big_endian_format && le)) {
			char buf[4];
			char *d = &buf[3];
			int i;

			for (i = 0; i < 4; i++) {
				*d-- = *p++;
			}
			memcpy(&x, buf, 4);
		}
		else {
			memcpy(&x, p, 4);
		}

		return x;
	}		
}

double
_PyFloat_Unpack8(const unsigned char *p, int le)
{
	if (double_format == unknown_format) {
		unsigned char sign;
		int e;
		unsigned int fhi, flo;
		double x;
		int incr = 1;

		if (le) {
			p += 7;
			incr = -1;
		}

		/* First byte */
		sign = (*p >> 7) & 1;
		e = (*p & 0x7F) << 4;
		
		p += incr;

		/* Second byte */
		e |= (*p >> 4) & 0xF;
		fhi = (*p & 0xF) << 24;
		p += incr;

		if (e == 2047) {
			PyErr_SetString(
				PyExc_ValueError,
				"can't unpack IEEE 754 special value "
				"on non-IEEE platform");
			return -1.0;
		}

		/* Third byte */
		fhi |= *p << 16;
		p += incr;

		/* Fourth byte */
		fhi |= *p  << 8;
		p += incr;

		/* Fifth byte */
		fhi |= *p;
		p += incr;

		/* Sixth byte */
		flo = *p << 16;
		p += incr;

		/* Seventh byte */
		flo |= *p << 8;
		p += incr;

		/* Eighth byte */
		flo |= *p;

		x = (double)fhi + (double)flo / 16777216.0; /* 2**24 */
		x /= 268435456.0; /* 2**28 */

		if (e == 0)
			e = -1022;
		else {
			x += 1.0;
			e -= 1023;
		}
		x = ldexp(x, e);

		if (sign)
			x = -x;

		return x;
	}
	else {
		double x;

		if ((double_format == ieee_little_endian_format && !le)
		    || (double_format == ieee_big_endian_format && le)) {
			char buf[8];
			char *d = &buf[7];
			int i;
			
			for (i = 0; i < 8; i++) {
				*d-- = *p++;
			}
			memcpy(&x, buf, 8);
		}
		else {
			memcpy(&x, p, 8);
		}

		return x;
	}
}
