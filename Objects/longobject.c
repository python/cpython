
/* Long (arbitrary precision) integer object implementation */

/* XXX The functional organization of this file is terrible */

#include "Python.h"
#include "longintrepr.h"

#include <assert.h>
#include <ctype.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* Forward */
static PyLongObject *long_normalize(PyLongObject *);
static PyLongObject *mul1(PyLongObject *, wdigit);
static PyLongObject *muladd1(PyLongObject *, wdigit, wdigit);
static PyLongObject *divrem1(PyLongObject *, wdigit, digit *);
static PyObject *long_format(PyObject *aa, int base, int addL);

static int ticker;	/* XXX Could be shared with ceval? */

#define SIGCHECK(PyTryBlock) \
	if (--ticker < 0) { \
		ticker = 100; \
		if (PyErr_CheckSignals()) { PyTryBlock; } \
	}

/* Normalize (remove leading zeros from) a long int object.
   Doesn't attempt to free the storage--in most cases, due to the nature
   of the algorithms used, this could save at most be one word anyway. */

static PyLongObject *
long_normalize(register PyLongObject *v)
{
	int j = ABS(v->ob_size);
	register int i = j;
	
	while (i > 0 && v->ob_digit[i-1] == 0)
		--i;
	if (i != j)
		v->ob_size = (v->ob_size < 0) ? -(i) : i;
	return v;
}

/* Allocate a new long int object with size digits.
   Return NULL and set exception if we run out of memory. */

PyLongObject *
_PyLong_New(int size)
{
	return PyObject_NEW_VAR(PyLongObject, &PyLong_Type, size);
}

/* Create a new long int object from a C long int */

PyObject *
PyLong_FromLong(long ival)
{
	/* Assume a C long fits in at most 5 'digits' */
	/* Works on both 32- and 64-bit machines */
	PyLongObject *v = _PyLong_New(5);
	if (v != NULL) {
		unsigned long t = ival;
		int i;
		if (ival < 0) {
			t = -ival;
			v->ob_size = -(v->ob_size);
  		}
		for (i = 0; i < 5; i++) {
			v->ob_digit[i] = (digit) (t & MASK);
			t >>= SHIFT;
		}
		v = long_normalize(v);
	}
	return (PyObject *)v;
}

/* Create a new long int object from a C unsigned long int */

PyObject *
PyLong_FromUnsignedLong(unsigned long ival)
{
	/* Assume a C long fits in at most 5 'digits' */
	/* Works on both 32- and 64-bit machines */
	PyLongObject *v = _PyLong_New(5);
	if (v != NULL) {
		unsigned long t = ival;
		int i;
		for (i = 0; i < 5; i++) {
			v->ob_digit[i] = (digit) (t & MASK);
			t >>= SHIFT;
		}
		v = long_normalize(v);
	}
	return (PyObject *)v;
}

/* Create a new long int object from a C double */

PyObject *
PyLong_FromDouble(double dval)
{
	PyLongObject *v;
	double frac;
	int i, ndig, expo, neg;
	neg = 0;
	if (Py_IS_INFINITY(dval)) {
		PyErr_SetString(PyExc_OverflowError,
			"cannot convert float infinity to long");
		return NULL;
	}
	if (dval < 0.0) {
		neg = 1;
		dval = -dval;
	}
	frac = frexp(dval, &expo); /* dval = frac*2**expo; 0.0 <= frac < 1.0 */
	if (expo <= 0)
		return PyLong_FromLong(0L);
	ndig = (expo-1) / SHIFT + 1; /* Number of 'digits' in result */
	v = _PyLong_New(ndig);
	if (v == NULL)
		return NULL;
	frac = ldexp(frac, (expo-1) % SHIFT + 1);
	for (i = ndig; --i >= 0; ) {
		long bits = (long)frac;
		v->ob_digit[i] = (digit) bits;
		frac = frac - (double)bits;
		frac = ldexp(frac, SHIFT);
	}
	if (neg)
		v->ob_size = -(v->ob_size);
	return (PyObject *)v;
}

/* Get a C long int from a long int object.
   Returns -1 and sets an error condition if overflow occurs. */

long
PyLong_AsLong(PyObject *vv)
{
	/* This version by Tim Peters */
	register PyLongObject *v;
	unsigned long x, prev;
	int i, sign;

	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return -1;
	}
	v = (PyLongObject *)vv;
	i = v->ob_size;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev)
			goto overflow;
	}
	/* Haven't lost any bits, but if the sign bit is set we're in
	 * trouble *unless* this is the min negative number.  So,
	 * trouble iff sign bit set && (positive || some bit set other
	 * than the sign bit).
	 */
	if ((long)x < 0 && (sign > 0 || (x << 1) != 0))
		goto overflow;
	return (long)x * sign;

 overflow:
	PyErr_SetString(PyExc_OverflowError,
			"long int too long to convert");
	return -1;
}

/* Get a C long int from a long int object.
   Returns -1 and sets an error condition if overflow occurs. */

unsigned long
PyLong_AsUnsignedLong(PyObject *vv)
{
	register PyLongObject *v;
	unsigned long x, prev;
	int i;
	
	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return (unsigned long) -1;
	}
	v = (PyLongObject *)vv;
	i = v->ob_size;
	x = 0;
	if (i < 0) {
		PyErr_SetString(PyExc_OverflowError,
			   "can't convert negative value to unsigned long");
		return (unsigned long) -1;
	}
	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev) {
			PyErr_SetString(PyExc_OverflowError,
				"long int too long to convert");
			return (unsigned long) -1;
		}
	}
	return x;
}

/* Get a C double from a long int object. */

double
PyLong_AsDouble(PyObject *vv)
{
	register PyLongObject *v;
	double x;
	double multiplier = (double) (1L << SHIFT);
	int i, sign;
	
	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return -1;
	}
	v = (PyLongObject *)vv;
	i = v->ob_size;
	sign = 1;
	x = 0.0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		x = x*multiplier + (double)v->ob_digit[i];
	}
	return x * sign;
}

/* Create a new long (or int) object from a C pointer */

PyObject *
PyLong_FromVoidPtr(void *p)
{
#if SIZEOF_VOID_P == SIZEOF_LONG
	return PyInt_FromLong((long)p);
#else
	/* optimize null pointers */
	if ( p == NULL )
		return PyInt_FromLong(0);

	/* we can assume that HAVE_LONG_LONG is true. if not, then the
	   configuration process should have bailed (having big pointers
	   without long longs seems non-sensical) */
	return PyLong_FromLongLong((LONG_LONG)p);
#endif /* SIZEOF_VOID_P == SIZEOF_LONG */
}

/* Get a C pointer from a long object (or an int object in some cases) */

void *
PyLong_AsVoidPtr(PyObject *vv)
{
	/* This function will allow int or long objects. If vv is neither,
	   then the PyLong_AsLong*() functions will raise the exception:
	   PyExc_SystemError, "bad argument to internal function"
	*/

#if SIZEOF_VOID_P == SIZEOF_LONG
	long x;

	if ( PyInt_Check(vv) )
		x = PyInt_AS_LONG(vv);
	else
		x = PyLong_AsLong(vv);
#else
	/* we can assume that HAVE_LONG_LONG is true. if not, then the
	   configuration process should have bailed (having big pointers
	   without long longs seems non-sensical) */
	LONG_LONG x;

	if ( PyInt_Check(vv) )
		x = PyInt_AS_LONG(vv);
	else
		x = PyLong_AsLongLong(vv);
#endif /* SIZEOF_VOID_P == SIZEOF_LONG */

	if (x == -1 && PyErr_Occurred())
		return NULL;
	return (void *)x;
}

#ifdef HAVE_LONG_LONG
/*
 * LONG_LONG support by Chris Herborth (chrish@qnx.com)
 *
 * For better or worse :-), I tried to follow the coding style already
 * here.
 */

/* Create a new long int object from a C LONG_LONG int */

PyObject *
PyLong_FromLongLong(LONG_LONG ival)
{
#if SIZEOF_LONG_LONG == SIZEOF_LONG
	/* In case the compiler is faking it. */
	return PyLong_FromLong( (long)ival );
#else
	if ((LONG_LONG)LONG_MIN <= ival && ival <= (LONG_LONG)LONG_MAX) {
		return PyLong_FromLong( (long)ival );
	}
	else if (0 <= ival && ival <= (unsigned LONG_LONG)ULONG_MAX) {
		return PyLong_FromUnsignedLong( (unsigned long)ival );
	}
	else {
		/* Assume a C LONG_LONG fits in at most 10 'digits'.
		 * Should be OK if we're assuming long fits in 5.
		 */
		PyLongObject *v = _PyLong_New(10);

		if (v != NULL) {
			unsigned LONG_LONG t = ival;
			int i;
			if (ival < 0) {
				t = -ival;
				v->ob_size = -(v->ob_size);
  			}

			for (i = 0; i < 10; i++) {
				v->ob_digit[i] = (digit) (t & MASK);
				t >>= SHIFT;
			}

			v = long_normalize(v);
		}

		return (PyObject *)v;
	}
#endif
}

/* Create a new long int object from a C unsigned LONG_LONG int */
PyObject *
PyLong_FromUnsignedLongLong(unsigned LONG_LONG ival)
{
#if SIZEOF_LONG_LONG == SIZEOF_LONG
	/* In case the compiler is faking it. */
	return PyLong_FromUnsignedLong( (unsigned long)ival );
#else
	if( ival <= (unsigned LONG_LONG)ULONG_MAX ) {
		return PyLong_FromUnsignedLong( (unsigned long)ival );
	}
	else {
		/* Assume a C long fits in at most 10 'digits'. */
		PyLongObject *v = _PyLong_New(10);

		if (v != NULL) {
			unsigned LONG_LONG t = ival;
			int i;
			for (i = 0; i < 10; i++) {
				v->ob_digit[i] = (digit) (t & MASK);
				t >>= SHIFT;
			}

			v = long_normalize(v);
		}

		return (PyObject *)v;
	}
#endif
}

/* Get a C LONG_LONG int from a long int object.
   Returns -1 and sets an error condition if overflow occurs. */

LONG_LONG
PyLong_AsLongLong(PyObject *vv)
{
#if SIZEOF_LONG_LONG == SIZEOF_LONG
	/* In case the compiler is faking it. */
	return (LONG_LONG)PyLong_AsLong( vv );
#else
	register PyLongObject *v;
	LONG_LONG x, prev;
	int i, sign;
	
	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return -1;
	}

	v = (PyLongObject *)vv;
	i = v->ob_size;
	sign = 1;
	x = 0;

	if (i < 0) {
		sign = -1;
		i = -(i);
	}

	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev) {
			PyErr_SetString(PyExc_OverflowError,
				"long int too long to convert");
			return -1;
		}
	}

	return x * sign;
#endif
}

unsigned LONG_LONG
PyLong_AsUnsignedLongLong(PyObject *vv)
{
#if SIZEOF_LONG_LONG == 4
	/* In case the compiler is faking it. */
	return (unsigned LONG_LONG)PyLong_AsUnsignedLong( vv );
#else
	register PyLongObject *v;
	unsigned LONG_LONG x, prev;
	int i;
	
	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return (unsigned LONG_LONG) -1;
	}

	v = (PyLongObject *)vv;
	i = v->ob_size;
	x = 0;

	if (i < 0) {
		PyErr_SetString(PyExc_OverflowError,
			   "can't convert negative value to unsigned long");
		return (unsigned LONG_LONG) -1;
	}

	while (--i >= 0) {
		prev = x;
		x = (x << SHIFT) + v->ob_digit[i];
		if ((x >> SHIFT) != prev) {
			PyErr_SetString(PyExc_OverflowError,
				"long int too long to convert");
			return (unsigned LONG_LONG) -1;
		}
	}

	return x;
#endif
}
#endif /* HAVE_LONG_LONG */

/* Multiply by a single digit, ignoring the sign. */

static PyLongObject *
mul1(PyLongObject *a, wdigit n)
{
	return muladd1(a, n, (digit)0);
}

/* Multiply by a single digit and add a single digit, ignoring the sign. */

static PyLongObject *
muladd1(PyLongObject *a, wdigit n, wdigit extra)
{
	int size_a = ABS(a->ob_size);
	PyLongObject *z = _PyLong_New(size_a+1);
	twodigits carry = extra;
	int i;
	
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_a; ++i) {
		carry += (twodigits)a->ob_digit[i] * n;
		z->ob_digit[i] = (digit) (carry & MASK);
		carry >>= SHIFT;
	}
	z->ob_digit[i] = (digit) carry;
	return long_normalize(z);
}

/* Divide a long integer by a digit, returning both the quotient
   (as function result) and the remainder (through *prem).
   The sign of a is ignored; n should not be zero. */

static PyLongObject *
divrem1(PyLongObject *a, wdigit n, digit *prem)
{
	int size = ABS(a->ob_size);
	PyLongObject *z;
	int i;
	twodigits rem = 0;
	
	assert(n > 0 && n <= MASK);
	z = _PyLong_New(size);
	if (z == NULL)
		return NULL;
	for (i = size; --i >= 0; ) {
		rem = (rem << SHIFT) + a->ob_digit[i];
		z->ob_digit[i] = (digit) (rem/n);
		rem %= n;
	}
	*prem = (digit) rem;
	return long_normalize(z);
}

/* Convert a long int object to a string, using a given conversion base.
   Return a string object.
   If base is 8 or 16, add the proper prefix '0' or '0x'. */

static PyObject *
long_format(PyObject *aa, int base, int addL)
{
	register PyLongObject *a = (PyLongObject *)aa;
	PyStringObject *str;
	int i;
	int size_a = ABS(a->ob_size);
	char *p;
	int bits;
	char sign = '\0';

	if (a == NULL || !PyLong_Check(a)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	assert(base >= 2 && base <= 36);
	
	/* Compute a rough upper bound for the length of the string */
	i = base;
	bits = 0;
	while (i > 1) {
		++bits;
		i >>= 1;
	}
	i = 5 + (addL ? 1 : 0) + (size_a*SHIFT + bits-1) / bits;
	str = (PyStringObject *) PyString_FromStringAndSize((char *)0, i);
	if (str == NULL)
		return NULL;
	p = PyString_AS_STRING(str) + i;
	*p = '\0';
        if (addL)
                *--p = 'L';
	if (a->ob_size < 0)
		sign = '-';
	
	if (a->ob_size == 0) {
		*--p = '0';
	}
	else if ((base & (base - 1)) == 0) {
		/* JRH: special case for power-of-2 bases */
		twodigits temp = a->ob_digit[0];
		int bitsleft = SHIFT;
		int rem;
		int last = abs(a->ob_size);
		int basebits = 1;
		i = base;
		while ((i >>= 1) > 1)
			++basebits;
		
		i = 0;
		for (;;) {
			while (bitsleft >= basebits) {
				if ((temp == 0) && (i >= last - 1)) break;
				rem = temp & (base - 1);
				if (rem < 10)
					rem += '0';
				else
					rem += 'A' - 10;
				assert(p > PyString_AS_STRING(str));
				*--p = (char) rem;
				bitsleft -= basebits;
				temp >>= basebits;
			}
			if (++i >= last) {
				if (temp == 0) break;
				bitsleft = 99;
				/* loop again to pick up final digits */
			}
			else {
				temp = (a->ob_digit[i] << bitsleft) | temp;
				bitsleft += SHIFT;
			}
		}
	}
	else {
		Py_INCREF(a);
		do {
			digit rem;
			PyLongObject *temp = divrem1(a, (digit)base, &rem);
			if (temp == NULL) {
				Py_DECREF(a);
				Py_DECREF(str);
				return NULL;
			}
			if (rem < 10)
				rem += '0';
			else
				rem += 'A'-10;
			assert(p > PyString_AS_STRING(str));
			*--p = (char) rem;
			Py_DECREF(a);
			a = temp;
			SIGCHECK({
				Py_DECREF(a);
				Py_DECREF(str);
				return NULL;
			})
		} while (ABS(a->ob_size) != 0);
		Py_DECREF(a);
	}

	if (base == 8) {
		if (size_a != 0)
			*--p = '0';
	}
	else if (base == 16) {
		*--p = 'x';
		*--p = '0';
	}
	else if (base != 10) {
		*--p = '#';
		*--p = '0' + base%10;
		if (base > 10)
			*--p = '0' + base/10;
	}
	if (sign)
		*--p = sign;
	if (p != PyString_AS_STRING(str)) {
		char *q = PyString_AS_STRING(str);
		assert(p > q);
		do {
		} while ((*q++ = *p++) != '\0');
		q--;
		_PyString_Resize((PyObject **)&str,
				 (int) (q - PyString_AS_STRING(str)));
	}
	return (PyObject *)str;
}

PyObject *
PyLong_FromString(char *str, char **pend, int base)
{
	int sign = 1;
	char *start, *orig_str = str;
	PyLongObject *z;
	
	if ((base != 0 && base < 2) || base > 36) {
		PyErr_SetString(PyExc_ValueError,
				"invalid base for long literal");
		return NULL;
	}
	while (*str != '\0' && isspace(Py_CHARMASK(*str)))
		str++;
	if (*str == '+')
		++str;
	else if (*str == '-') {
		++str;
		sign = -1;
	}
	while (*str != '\0' && isspace(Py_CHARMASK(*str)))
		str++;
	if (base == 0) {
		if (str[0] != '0')
			base = 10;
		else if (str[1] == 'x' || str[1] == 'X')
			base = 16;
		else
			base = 8;
	}
	if (base == 16 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
		str += 2;
	z = _PyLong_New(0);
	start = str;
	for ( ; z != NULL; ++str) {
		int k = -1;
		PyLongObject *temp;
		
		if (*str <= '9')
			k = *str - '0';
		else if (*str >= 'a')
			k = *str - 'a' + 10;
		else if (*str >= 'A')
			k = *str - 'A' + 10;
		if (k < 0 || k >= base)
			break;
		temp = muladd1(z, (digit)base, (digit)k);
		Py_DECREF(z);
		z = temp;
	}
	if (z == NULL)
		return NULL;
	if (str == start)
		goto onError;
	if (sign < 0 && z != NULL && z->ob_size != 0)
		z->ob_size = -(z->ob_size);
	if (*str == 'L' || *str == 'l')
		str++;
	while (*str && isspace(Py_CHARMASK(*str)))
		str++;
	if (*str != '\0')
		goto onError;
	if (pend)
		*pend = str;
	return (PyObject *) z;

 onError:
	PyErr_Format(PyExc_ValueError, 
		     "invalid literal for long(): %.200s", orig_str);
	Py_XDECREF(z);
	return NULL;
}

PyObject *
PyLong_FromUnicode(Py_UNICODE *u, int length, int base)
{
	char buffer[256];

	if (length >= sizeof(buffer)) {
		PyErr_SetString(PyExc_ValueError,
				"long() literal too large to convert");
		return NULL;
	}
	if (PyUnicode_EncodeDecimal(u, length, buffer, NULL))
		return NULL;

	return PyLong_FromString(buffer, NULL, base);
}

/* forward */
static PyLongObject *x_divrem
	(PyLongObject *, PyLongObject *, PyLongObject **);
static PyObject *long_pos(PyLongObject *);
static int long_divrem(PyLongObject *, PyLongObject *,
	PyLongObject **, PyLongObject **);

/* Long division with remainder, top-level routine */

static int
long_divrem(PyLongObject *a, PyLongObject *b,
	    PyLongObject **pdiv, PyLongObject **prem)
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	PyLongObject *z;
	
	if (size_b == 0) {
		PyErr_SetString(PyExc_ZeroDivisionError,
				"long division or modulo");
		return -1;
	}
	if (size_a < size_b ||
	    (size_a == size_b &&
	     a->ob_digit[size_a-1] < b->ob_digit[size_b-1])) {
		/* |a| < |b|. */
		*pdiv = _PyLong_New(0);
		Py_INCREF(a);
		*prem = (PyLongObject *) a;
		return 0;
	}
	if (size_b == 1) {
		digit rem = 0;
		z = divrem1(a, b->ob_digit[0], &rem);
		if (z == NULL)
			return -1;
		*prem = (PyLongObject *) PyLong_FromLong((long)rem);
	}
	else {
		z = x_divrem(a, b, prem);
		if (z == NULL)
			return -1;
	}
	/* Set the signs.
	   The quotient z has the sign of a*b;
	   the remainder r has the sign of a,
	   so a = b*z + r. */
	if ((a->ob_size < 0) != (b->ob_size < 0))
		z->ob_size = -(z->ob_size);
	if (a->ob_size < 0 && (*prem)->ob_size != 0)
		(*prem)->ob_size = -((*prem)->ob_size);
	*pdiv = z;
	return 0;
}

/* Unsigned long division with remainder -- the algorithm */

static PyLongObject *
x_divrem(PyLongObject *v1, PyLongObject *w1, PyLongObject **prem)
{
	int size_v = ABS(v1->ob_size), size_w = ABS(w1->ob_size);
	digit d = (digit) ((twodigits)BASE / (w1->ob_digit[size_w-1] + 1));
	PyLongObject *v = mul1(v1, d);
	PyLongObject *w = mul1(w1, d);
	PyLongObject *a;
	int j, k;
	
	if (v == NULL || w == NULL) {
		Py_XDECREF(v);
		Py_XDECREF(w);
		return NULL;
	}
	
	assert(size_v >= size_w && size_w > 1); /* Assert checks by div() */
	assert(v->ob_refcnt == 1); /* Since v will be used as accumulator! */
	assert(size_w == ABS(w->ob_size)); /* That's how d was calculated */
	
	size_v = ABS(v->ob_size);
	a = _PyLong_New(size_v - size_w + 1);
	
	for (j = size_v, k = a->ob_size-1; a != NULL && k >= 0; --j, --k) {
		digit vj = (j >= size_v) ? 0 : v->ob_digit[j];
		twodigits q;
		stwodigits carry = 0;
		int i;
		
		SIGCHECK({
			Py_DECREF(a);
			a = NULL;
			break;
		})
		if (vj == w->ob_digit[size_w-1])
			q = MASK;
		else
			q = (((twodigits)vj << SHIFT) + v->ob_digit[j-1]) /
				w->ob_digit[size_w-1];
		
		while (w->ob_digit[size_w-2]*q >
				((
					((twodigits)vj << SHIFT)
					+ v->ob_digit[j-1]
					- q*w->ob_digit[size_w-1]
								) << SHIFT)
				+ v->ob_digit[j-2])
			--q;
		
		for (i = 0; i < size_w && i+k < size_v; ++i) {
			twodigits z = w->ob_digit[i] * q;
			digit zz = (digit) (z >> SHIFT);
			carry += v->ob_digit[i+k] - z
				+ ((twodigits)zz << SHIFT);
			v->ob_digit[i+k] = carry & MASK;
			carry = Py_ARITHMETIC_RIGHT_SHIFT(BASE_TWODIGITS_TYPE,
							  carry, SHIFT);
			carry -= zz;
		}
		
		if (i+k < size_v) {
			carry += v->ob_digit[i+k];
			v->ob_digit[i+k] = 0;
		}
		
		if (carry == 0)
			a->ob_digit[k] = (digit) q;
		else {
			assert(carry == -1);
			a->ob_digit[k] = (digit) q-1;
			carry = 0;
			for (i = 0; i < size_w && i+k < size_v; ++i) {
				carry += v->ob_digit[i+k] + w->ob_digit[i];
				v->ob_digit[i+k] = carry & MASK;
				carry = Py_ARITHMETIC_RIGHT_SHIFT(
						BASE_TWODIGITS_TYPE,
						carry, SHIFT);
			}
		}
	} /* for j, k */
	
	if (a == NULL)
		*prem = NULL;
	else {
		a = long_normalize(a);
		*prem = divrem1(v, d, &d);
		/* d receives the (unused) remainder */
		if (*prem == NULL) {
			Py_DECREF(a);
			a = NULL;
		}
	}
	Py_DECREF(v);
	Py_DECREF(w);
	return a;
}

/* Methods */

static void
long_dealloc(PyObject *v)
{
	PyObject_DEL(v);
}

static PyObject *
long_repr(PyObject *v)
{
	return long_format(v, 10, 1);
}

static PyObject *
long_str(PyObject *v)
{
	return long_format(v, 10, 0);
}

static int
long_compare(PyLongObject *a, PyLongObject *b)
{
	int sign;
	
	if (a->ob_size != b->ob_size) {
		if (ABS(a->ob_size) == 0 && ABS(b->ob_size) == 0)
			sign = 0;
		else
			sign = a->ob_size - b->ob_size;
	}
	else {
		int i = ABS(a->ob_size);
		while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
			;
		if (i < 0)
			sign = 0;
		else {
			sign = (int)a->ob_digit[i] - (int)b->ob_digit[i];
			if (a->ob_size < 0)
				sign = -sign;
		}
	}
	return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}

static long
long_hash(PyLongObject *v)
{
	long x;
	int i, sign;

	/* This is designed so that Python ints and longs with the
	   same value hash to the same value, otherwise comparisons
	   of mapping keys will turn out weird */
	i = v->ob_size;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	while (--i >= 0) {
		/* Force a 32-bit circular shift */
		x = ((x << SHIFT) & ~MASK) | ((x >> (32-SHIFT)) & MASK);
		x += v->ob_digit[i];
	}
	x = x * sign;
	if (x == -1)
		x = -2;
	return x;
}


/* Add the absolute values of two long integers. */

static PyLongObject *
x_add(PyLongObject *a, PyLongObject *b)
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	PyLongObject *z;
	int i;
	digit carry = 0;
	
	/* Ensure a is the larger of the two: */
	if (size_a < size_b) {
		{ PyLongObject *temp = a; a = b; b = temp; }
		{ int size_temp = size_a;
		  size_a = size_b;
		  size_b = size_temp; }
	}
	z = _PyLong_New(size_a+1);
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_b; ++i) {
		carry += a->ob_digit[i] + b->ob_digit[i];
		z->ob_digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	for (; i < size_a; ++i) {
		carry += a->ob_digit[i];
		z->ob_digit[i] = carry & MASK;
		carry >>= SHIFT;
	}
	z->ob_digit[i] = carry;
	return long_normalize(z);
}

/* Subtract the absolute values of two integers. */

static PyLongObject *
x_sub(PyLongObject *a, PyLongObject *b)
{
	int size_a = ABS(a->ob_size), size_b = ABS(b->ob_size);
	PyLongObject *z;
	int i;
	int sign = 1;
	digit borrow = 0;
	
	/* Ensure a is the larger of the two: */
	if (size_a < size_b) {
		sign = -1;
		{ PyLongObject *temp = a; a = b; b = temp; }
		{ int size_temp = size_a;
		  size_a = size_b;
		  size_b = size_temp; }
	}
	else if (size_a == size_b) {
		/* Find highest digit where a and b differ: */
		i = size_a;
		while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
			;
		if (i < 0)
			return _PyLong_New(0);
		if (a->ob_digit[i] < b->ob_digit[i]) {
			sign = -1;
			{ PyLongObject *temp = a; a = b; b = temp; }
		}
		size_a = size_b = i+1;
	}
	z = _PyLong_New(size_a);
	if (z == NULL)
		return NULL;
	for (i = 0; i < size_b; ++i) {
		/* The following assumes unsigned arithmetic
		   works module 2**N for some N>SHIFT. */
		borrow = a->ob_digit[i] - b->ob_digit[i] - borrow;
		z->ob_digit[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1; /* Keep only one sign bit */
	}
	for (; i < size_a; ++i) {
		borrow = a->ob_digit[i] - borrow;
		z->ob_digit[i] = borrow & MASK;
		borrow >>= SHIFT;
		borrow &= 1; /* Keep only one sign bit */
	}
	assert(borrow == 0);
	if (sign < 0)
		z->ob_size = -(z->ob_size);
	return long_normalize(z);
}

static PyObject *
long_add(PyLongObject *a, PyLongObject *b)
{
	PyLongObject *z;
	
	if (a->ob_size < 0) {
		if (b->ob_size < 0) {
			z = x_add(a, b);
			if (z != NULL && z->ob_size != 0)
				z->ob_size = -(z->ob_size);
		}
		else
			z = x_sub(b, a);
	}
	else {
		if (b->ob_size < 0)
			z = x_sub(a, b);
		else
			z = x_add(a, b);
	}
	return (PyObject *)z;
}

static PyObject *
long_sub(PyLongObject *a, PyLongObject *b)
{
	PyLongObject *z;
	
	if (a->ob_size < 0) {
		if (b->ob_size < 0)
			z = x_sub(a, b);
		else
			z = x_add(a, b);
		if (z != NULL && z->ob_size != 0)
			z->ob_size = -(z->ob_size);
	}
	else {
		if (b->ob_size < 0)
			z = x_add(a, b);
		else
			z = x_sub(a, b);
	}
	return (PyObject *)z;
}

static PyObject *
long_mul(PyLongObject *a, PyLongObject *b)
{
	int size_a;
	int size_b;
	PyLongObject *z;
	int i;
	
	size_a = ABS(a->ob_size);
	size_b = ABS(b->ob_size);
	if (size_a > size_b) {
		/* we are faster with the small object on the left */
		int hold_sa = size_a;
		PyLongObject *hold_a = a;
		size_a = size_b;
		size_b = hold_sa;
		a = b;
		b = hold_a;
	}
	z = _PyLong_New(size_a + size_b);
	if (z == NULL)
		return NULL;
	for (i = 0; i < z->ob_size; ++i)
		z->ob_digit[i] = 0;
	for (i = 0; i < size_a; ++i) {
		twodigits carry = 0;
		twodigits f = a->ob_digit[i];
		int j;
		
		SIGCHECK({
			Py_DECREF(z);
			return NULL;
		})
		for (j = 0; j < size_b; ++j) {
			carry += z->ob_digit[i+j] + b->ob_digit[j] * f;
			z->ob_digit[i+j] = (digit) (carry & MASK);
			carry >>= SHIFT;
		}
		for (; carry != 0; ++j) {
			assert(i+j < z->ob_size);
			carry += z->ob_digit[i+j];
			z->ob_digit[i+j] = (digit) (carry & MASK);
			carry >>= SHIFT;
		}
	}
	if (a->ob_size < 0)
		z->ob_size = -(z->ob_size);
	if (b->ob_size < 0)
		z->ob_size = -(z->ob_size);
	return (PyObject *) long_normalize(z);
}

/* The / and % operators are now defined in terms of divmod().
   The expression a mod b has the value a - b*floor(a/b).
   The long_divrem function gives the remainder after division of
   |a| by |b|, with the sign of a.  This is also expressed
   as a - b*trunc(a/b), if trunc truncates towards zero.
   Some examples:
   	 a	 b	a rem b		a mod b
   	 13	 10	 3		 3
   	-13	 10	-3		 7
   	 13	-10	 3		-7
   	-13	-10	-3		-3
   So, to get from rem to mod, we have to add b if a and b
   have different signs.  We then subtract one from the 'div'
   part of the outcome to keep the invariant intact. */

static int
l_divmod(PyLongObject *v, PyLongObject *w, 
	 PyLongObject **pdiv, PyLongObject **pmod)
{
	PyLongObject *div, *mod;
	
	if (long_divrem(v, w, &div, &mod) < 0)
		return -1;
	if ((mod->ob_size < 0 && w->ob_size > 0) ||
	    (mod->ob_size > 0 && w->ob_size < 0)) {
		PyLongObject *temp;
		PyLongObject *one;
		temp = (PyLongObject *) long_add(mod, w);
		Py_DECREF(mod);
		mod = temp;
		if (mod == NULL) {
			Py_DECREF(div);
			return -1;
		}
		one = (PyLongObject *) PyLong_FromLong(1L);
		if (one == NULL ||
		    (temp = (PyLongObject *) long_sub(div, one)) == NULL) {
			Py_DECREF(mod);
			Py_DECREF(div);
			Py_XDECREF(one);
			return -1;
		}
		Py_DECREF(one);
		Py_DECREF(div);
		div = temp;
	}
	*pdiv = div;
	*pmod = mod;
	return 0;
}

static PyObject *
long_div(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *div, *mod;
	if (l_divmod(v, w, &div, &mod) < 0)
		return NULL;
	Py_DECREF(mod);
	return (PyObject *)div;
}

static PyObject *
long_mod(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *div, *mod;
	if (l_divmod(v, w, &div, &mod) < 0)
		return NULL;
	Py_DECREF(div);
	return (PyObject *)mod;
}

static PyObject *
long_divmod(PyLongObject *v, PyLongObject *w)
{
	PyObject *z;
	PyLongObject *div, *mod;
	if (l_divmod(v, w, &div, &mod) < 0)
		return NULL;
	z = PyTuple_New(2);
	if (z != NULL) {
		PyTuple_SetItem(z, 0, (PyObject *) div);
		PyTuple_SetItem(z, 1, (PyObject *) mod);
	}
	else {
		Py_DECREF(div);
		Py_DECREF(mod);
	}
	return z;
}

static PyObject *
long_pow(PyLongObject *a, PyLongObject *b, PyLongObject *c)
{
	PyLongObject *z, *div, *mod;
	int size_b, i;
	
	size_b = b->ob_size;
	if (size_b < 0) {
		if (a->ob_size)
			PyErr_SetString(PyExc_ValueError,
					"long integer to a negative power");
		else
			PyErr_SetString(PyExc_ZeroDivisionError,
					"zero to a negative power");
		return NULL;
	}
	z = (PyLongObject *)PyLong_FromLong(1L);
	Py_INCREF(a);
	for (i = 0; i < size_b; ++i) {
		digit bi = b->ob_digit[i];
		int j;
	
		for (j = 0; j < SHIFT; ++j) {
			PyLongObject *temp;
		
			if (bi & 1) {
				temp = (PyLongObject *)long_mul(z, a);
				Py_DECREF(z);
			 	if ((PyObject*)c!=Py_None && temp!=NULL) {
			 		if (l_divmod(temp,c,&div,&mod) < 0) {
						Py_DECREF(temp);
						z = NULL;
						goto error;
					}
				 	Py_XDECREF(div);
				 	Py_DECREF(temp);
				 	temp = mod;
				}
			 	z = temp;
				if (z == NULL)
					break;
			}
			bi >>= 1;
			if (bi == 0 && i+1 == size_b)
				break;
			temp = (PyLongObject *)long_mul(a, a);
			Py_DECREF(a);
		 	if ((PyObject*)c!=Py_None && temp!=NULL) {
			 	if (l_divmod(temp, c, &div, &mod) < 0) {
					Py_DECREF(temp);
					z = NULL;
					goto error;
				}
			 	Py_XDECREF(div);
			 	Py_DECREF(temp);
			 	temp = mod;
			}
			a = temp;
			if (a == NULL) {
				Py_DECREF(z);
				z = NULL;
				break;
			}
		}
		if (a == NULL || z == NULL)
			break;
	}
	Py_XDECREF(a);
	if ((PyObject*)c!=Py_None && z!=NULL) {
		if (l_divmod(z, c, &div, &mod) < 0) {
			Py_DECREF(z);
			z = NULL;
		}
		else {
			Py_XDECREF(div);
			Py_DECREF(z);
			z = mod;
		}
	}
  error:
	return (PyObject *)z;
}

static PyObject *
long_invert(PyLongObject *v)
{
	/* Implement ~x as -(x+1) */
	PyLongObject *x;
	PyLongObject *w;
	w = (PyLongObject *)PyLong_FromLong(1L);
	if (w == NULL)
		return NULL;
	x = (PyLongObject *) long_add(v, w);
	Py_DECREF(w);
	if (x == NULL)
		return NULL;
	if (x->ob_size != 0)
		x->ob_size = -(x->ob_size);
	return (PyObject *)x;
}

static PyObject *
long_pos(PyLongObject *v)
{
	Py_INCREF(v);
	return (PyObject *)v;
}

static PyObject *
long_neg(PyLongObject *v)
{
	PyLongObject *z;
	int i, n;
	n = ABS(v->ob_size);
	if (n == 0) {
		/* -0 == 0 */
		Py_INCREF(v);
		return (PyObject *) v;
	}
	z = _PyLong_New(ABS(n));
	if (z == NULL)
		return NULL;
	for (i = 0; i < n; i++)
		z->ob_digit[i] = v->ob_digit[i];
	z->ob_size = -(v->ob_size);
	return (PyObject *)z;
}

static PyObject *
long_abs(PyLongObject *v)
{
	if (v->ob_size < 0)
		return long_neg(v);
	else {
		Py_INCREF(v);
		return (PyObject *)v;
	}
}

static int
long_nonzero(PyLongObject *v)
{
	return ABS(v->ob_size) != 0;
}

static PyObject *
long_rshift(PyLongObject *a, PyLongObject *b)
{
	PyLongObject *z;
	long shiftby;
	int newsize, wordshift, loshift, hishift, i, j;
	digit lomask, himask;
	
	if (a->ob_size < 0) {
		/* Right shifting negative numbers is harder */
		PyLongObject *a1, *a2, *a3;
		a1 = (PyLongObject *) long_invert(a);
		if (a1 == NULL) return NULL;
		a2 = (PyLongObject *) long_rshift(a1, b);
		Py_DECREF(a1);
		if (a2 == NULL) return NULL;
		a3 = (PyLongObject *) long_invert(a2);
		Py_DECREF(a2);
		return (PyObject *) a3;
	}
	
	shiftby = PyLong_AsLong((PyObject *)b);
	if (shiftby == -1L && PyErr_Occurred())
		return NULL;
	if (shiftby < 0) {
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return NULL;
	}
	wordshift = shiftby / SHIFT;
	newsize = ABS(a->ob_size) - wordshift;
	if (newsize <= 0) {
		z = _PyLong_New(0);
		return (PyObject *)z;
	}
	loshift = shiftby % SHIFT;
	hishift = SHIFT - loshift;
	lomask = ((digit)1 << hishift) - 1;
	himask = MASK ^ lomask;
	z = _PyLong_New(newsize);
	if (z == NULL)
		return NULL;
	if (a->ob_size < 0)
		z->ob_size = -(z->ob_size);
	for (i = 0, j = wordshift; i < newsize; i++, j++) {
		z->ob_digit[i] = (a->ob_digit[j] >> loshift) & lomask;
		if (i+1 < newsize)
			z->ob_digit[i] |=
			  (a->ob_digit[j+1] << hishift) & himask;
	}
	return (PyObject *) long_normalize(z);
}

static PyObject *
long_lshift(PyLongObject *a, PyLongObject *b)
{
	/* This version due to Tim Peters */
	PyLongObject *z;
	long shiftby;
	int oldsize, newsize, wordshift, remshift, i, j;
	twodigits accum;
	
	shiftby = PyLong_AsLong((PyObject *)b);
	if (shiftby == -1L && PyErr_Occurred())
		return NULL;
	if (shiftby < 0) {
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return NULL;
	}
	if ((long)(int)shiftby != shiftby) {
		PyErr_SetString(PyExc_ValueError,
				"outrageous left shift count");
		return NULL;
	}
	/* wordshift, remshift = divmod(shiftby, SHIFT) */
	wordshift = (int)shiftby / SHIFT;
	remshift  = (int)shiftby - wordshift * SHIFT;

	oldsize = ABS(a->ob_size);
	newsize = oldsize + wordshift;
	if (remshift)
		++newsize;
	z = _PyLong_New(newsize);
	if (z == NULL)
		return NULL;
	if (a->ob_size < 0)
		z->ob_size = -(z->ob_size);
	for (i = 0; i < wordshift; i++)
		z->ob_digit[i] = 0;
	accum = 0;	
	for (i = wordshift, j = 0; j < oldsize; i++, j++) {
		accum |= a->ob_digit[j] << remshift;
		z->ob_digit[i] = (digit)(accum & MASK);
		accum >>= SHIFT;
	}
	if (remshift)
		z->ob_digit[newsize-1] = (digit)accum;
	else	
		assert(!accum);
	return (PyObject *) long_normalize(z);
}


/* Bitwise and/xor/or operations */

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

static PyObject *
long_bitwise(PyLongObject *a,
	     int op,  /* '&', '|', '^' */
	     PyLongObject *b)
{
	digit maska, maskb; /* 0 or MASK */
	int negz;
	int size_a, size_b, size_z;
	PyLongObject *z;
	int i;
	digit diga, digb;
	PyObject *v;
	
	if (a->ob_size < 0) {
		a = (PyLongObject *) long_invert(a);
		maska = MASK;
	}
	else {
		Py_INCREF(a);
		maska = 0;
	}
	if (b->ob_size < 0) {
		b = (PyLongObject *) long_invert(b);
		maskb = MASK;
	}
	else {
		Py_INCREF(b);
		maskb = 0;
	}
	
	negz = 0;
	switch (op) {
	case '^':
		if (maska != maskb) {
			maska ^= MASK;
			negz = -1;
		}
		break;
	case '&':
		if (maska && maskb) {
			op = '|';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	case '|':
		if (maska || maskb) {
			op = '&';
			maska ^= MASK;
			maskb ^= MASK;
			negz = -1;
		}
		break;
	}
	
	/* JRH: The original logic here was to allocate the result value (z)
	   as the longer of the two operands.  However, there are some cases
	   where the result is guaranteed to be shorter than that: AND of two
	   positives, OR of two negatives: use the shorter number.  AND with
	   mixed signs: use the positive number.  OR with mixed signs: use the
	   negative number.  After the transformations above, op will be '&'
	   iff one of these cases applies, and mask will be non-0 for operands
	   whose length should be ignored.
	*/

	size_a = a->ob_size;
	size_b = b->ob_size;
	size_z = op == '&'
		? (maska
		   ? size_b
		   : (maskb ? size_a : MIN(size_a, size_b)))
		: MAX(size_a, size_b);
	z = _PyLong_New(size_z);
	if (a == NULL || b == NULL || z == NULL) {
		Py_XDECREF(a);
		Py_XDECREF(b);
		Py_XDECREF(z);
		return NULL;
	}
	
	for (i = 0; i < size_z; ++i) {
		diga = (i < size_a ? a->ob_digit[i] : 0) ^ maska;
		digb = (i < size_b ? b->ob_digit[i] : 0) ^ maskb;
		switch (op) {
		case '&': z->ob_digit[i] = diga & digb; break;
		case '|': z->ob_digit[i] = diga | digb; break;
		case '^': z->ob_digit[i] = diga ^ digb; break;
		}
	}
	
	Py_DECREF(a);
	Py_DECREF(b);
	z = long_normalize(z);
	if (negz == 0)
		return (PyObject *) z;
	v = long_invert(z);
	Py_DECREF(z);
	return v;
}

static PyObject *
long_and(PyLongObject *a, PyLongObject *b)
{
	return long_bitwise(a, '&', b);
}

static PyObject *
long_xor(PyLongObject *a, PyLongObject *b)
{
	return long_bitwise(a, '^', b);
}

static PyObject *
long_or(PyLongObject *a, PyLongObject *b)
{
	return long_bitwise(a, '|', b);
}

static int
long_coerce(PyObject **pv, PyObject **pw)
{
	if (PyInt_Check(*pw)) {
		*pw = PyLong_FromLong(PyInt_AsLong(*pw));
		Py_INCREF(*pv);
		return 0;
	}
	return 1; /* Can't do it */
}

static PyObject *
long_int(PyObject *v)
{
	long x;
	x = PyLong_AsLong(v);
	if (PyErr_Occurred())
		return NULL;
	return PyInt_FromLong(x);
}

static PyObject *
long_long(PyObject *v)
{
	Py_INCREF(v);
	return v;
}

static PyObject *
long_float(PyObject *v)
{
	double result;
	PyFPE_START_PROTECT("long_float", return 0)
	result = PyLong_AsDouble(v);
	PyFPE_END_PROTECT(result)
	return PyFloat_FromDouble(result);
}

static PyObject *
long_oct(PyObject *v)
{
	return long_format(v, 8, 1);
}

static PyObject *
long_hex(PyObject *v)
{
	return long_format(v, 16, 1);
}

static PyNumberMethods long_as_number = {
	(binaryfunc)	long_add,	/*nb_add*/
	(binaryfunc)	long_sub,	/*nb_subtract*/
	(binaryfunc)	long_mul,	/*nb_multiply*/
	(binaryfunc)	long_div,	/*nb_divide*/
	(binaryfunc)	long_mod,	/*nb_remainder*/
	(binaryfunc)	long_divmod,	/*nb_divmod*/
	(ternaryfunc)	long_pow,	/*nb_power*/
	(unaryfunc) 	long_neg,	/*nb_negative*/
	(unaryfunc) 	long_pos,	/*tp_positive*/
	(unaryfunc) 	long_abs,	/*tp_absolute*/
	(inquiry)	long_nonzero,	/*tp_nonzero*/
	(unaryfunc)	long_invert,	/*nb_invert*/
	(binaryfunc)	long_lshift,	/*nb_lshift*/
	(binaryfunc)	long_rshift,	/*nb_rshift*/
	(binaryfunc)	long_and,	/*nb_and*/
	(binaryfunc)	long_xor,	/*nb_xor*/
	(binaryfunc)	long_or,	/*nb_or*/
	(coercion)	long_coerce,	/*nb_coerce*/
	(unaryfunc)	long_int,	/*nb_int*/
	(unaryfunc)	long_long,	/*nb_long*/
	(unaryfunc)	long_float,	/*nb_float*/
	(unaryfunc)	long_oct,	/*nb_oct*/
	(unaryfunc)	long_hex,	/*nb_hex*/
};

PyTypeObject PyLong_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"long int",
	sizeof(PyLongObject) - sizeof(digit),
	sizeof(digit),
	(destructor)long_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	0,				/*tp_getattr*/
	0,				/*tp_setattr*/
	(cmpfunc)long_compare,		/*tp_compare*/
	(reprfunc)long_repr,		/*tp_repr*/
	&long_as_number,		/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	(hashfunc)long_hash,		/*tp_hash*/
        0,              		/*tp_call*/
        (reprfunc)long_str,		/*tp_str*/
};
