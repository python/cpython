
/* Long (arbitrary precision) integer object implementation */

/* XXX The functional organization of this file is terrible */

#include "Python.h"
#include "longintrepr.h"

#include <ctype.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* Forward */
static PyLongObject *long_normalize(PyLongObject *);
static PyLongObject *mul1(PyLongObject *, wdigit);
static PyLongObject *muladd1(PyLongObject *, wdigit, wdigit);
static PyLongObject *divrem1(PyLongObject *, digit, digit *);
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

PyObject *
_PyLong_Copy(PyLongObject *src)
{
	PyLongObject *result;
	int i;

	assert(src != NULL);
	i = src->ob_size;
	if (i < 0)
		i = -(i);
	result = _PyLong_New(i);
	if (result != NULL) {
		result->ob_size = src->ob_size;
		while (--i >= 0)
			result->ob_digit[i] = src->ob_digit[i];
	}
	return (PyObject *)result;
}

/* Create a new long int object from a C long int */

PyObject *
PyLong_FromLong(long ival)
{
	PyLongObject *v;
	unsigned long t;  /* unsigned so >> doesn't propagate sign bit */
	int ndigits = 0;
	int negative = 0;

	if (ival < 0) {
		ival = -ival;
		negative = 1;
	}

	/* Count the number of Python digits.
	   We used to pick 5 ("big enough for anything"), but that's a
	   waste of time and space given that 5*15 = 75 bits are rarely
	   needed. */
	t = (unsigned long)ival;
	while (t) {
		++ndigits;
		t >>= SHIFT;
	}
	v = _PyLong_New(ndigits);
	if (v != NULL) {
		digit *p = v->ob_digit;
		v->ob_size = negative ? -ndigits : ndigits;
		t = (unsigned long)ival;
		while (t) {
			*p++ = (digit)(t & MASK);
			t >>= SHIFT;
		}
	}
	return (PyObject *)v;
}

/* Create a new long int object from a C unsigned long int */

PyObject *
PyLong_FromUnsignedLong(unsigned long ival)
{
	PyLongObject *v;
	unsigned long t;
	int ndigits = 0;

	/* Count the number of Python digits. */
	t = (unsigned long)ival;
	while (t) {
		++ndigits;
		t >>= SHIFT;
	}
	v = _PyLong_New(ndigits);
	if (v != NULL) {
		digit *p = v->ob_digit;
		v->ob_size = ndigits;
		while (ival) {
			*p++ = (digit)(ival & MASK);
			ival >>= SHIFT;
		}
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
		if (vv != NULL && PyInt_Check(vv))
			return PyInt_AsLong(vv);
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
			"long int too large to convert to int");
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
				"long int too large to convert");
			return (unsigned long) -1;
		}
	}
	return x;
}

PyObject *
_PyLong_FromByteArray(const unsigned char* bytes, size_t n,
		      int little_endian, int is_signed)
{
	const unsigned char* pstartbyte;/* LSB of bytes */
	int incr;			/* direction to move pstartbyte */
	const unsigned char* pendbyte;	/* MSB of bytes */
	size_t numsignificantbytes;	/* number of bytes that matter */
	size_t ndigits;			/* number of Python long digits */
	PyLongObject* v;		/* result */
	int idigit = 0;  		/* next free index in v->ob_digit */

	if (n == 0)
		return PyLong_FromLong(0L);

	if (little_endian) {
		pstartbyte = bytes;
		pendbyte = bytes + n - 1;
		incr = 1;
	}
	else {
		pstartbyte = bytes + n - 1;
		pendbyte = bytes;
		incr = -1;
	}

	if (is_signed)
		is_signed = *pendbyte >= 0x80;

	/* Compute numsignificantbytes.  This consists of finding the most
	   significant byte.  Leading 0 bytes are insignficant if the number
	   is positive, and leading 0xff bytes if negative. */
	{
		size_t i;
		const unsigned char* p = pendbyte;
		const int pincr = -incr;  /* search MSB to LSB */
		const unsigned char insignficant = is_signed ? 0xff : 0x00;

		for (i = 0; i < n; ++i, p += pincr) {
			if (*p != insignficant)
				break;
		}
		numsignificantbytes = n - i;
		/* 2's-comp is a bit tricky here, e.g. 0xff00 == -0x0100, so
		   actually has 2 significant bytes.  OTOH, 0xff0001 ==
		   -0x00ffff, so we wouldn't *need* to bump it there; but we
		   do for 0xffff = -0x0001.  To be safe without bothering to
		   check every case, bump it regardless. */
		if (is_signed && numsignificantbytes < n)
			++numsignificantbytes;
	}

	/* How many Python long digits do we need?  We have
	   8*numsignificantbytes bits, and each Python long digit has SHIFT
	   bits, so it's the ceiling of the quotient. */
	ndigits = (numsignificantbytes * 8 + SHIFT - 1) / SHIFT;
	if (ndigits > (size_t)INT_MAX)
		return PyErr_NoMemory();
	v = _PyLong_New((int)ndigits);
	if (v == NULL)
		return NULL;

	/* Copy the bits over.  The tricky parts are computing 2's-comp on
	   the fly for signed numbers, and dealing with the mismatch between
	   8-bit bytes and (probably) 15-bit Python digits.*/
	{
		size_t i;
		twodigits carry = 1;		/* for 2's-comp calculation */
		twodigits accum = 0;		/* sliding register */
		unsigned int accumbits = 0; 	/* number of bits in accum */
		const unsigned char* p = pstartbyte;

		for (i = 0; i < numsignificantbytes; ++i, p += incr) {
			twodigits thisbyte = *p;
			/* Compute correction for 2's comp, if needed. */
			if (is_signed) {
				thisbyte = (0xff ^ thisbyte) + carry;
				carry = thisbyte >> 8;
				thisbyte &= 0xff;
			}
			/* Because we're going LSB to MSB, thisbyte is
			   more significant than what's already in accum,
			   so needs to be prepended to accum. */
			accum |= thisbyte << accumbits;
			accumbits += 8;
			if (accumbits >= SHIFT) {
				/* There's enough to fill a Python digit. */
				assert(idigit < (int)ndigits);
				v->ob_digit[idigit] = (digit)(accum & MASK);
				++idigit;
				accum >>= SHIFT;
				accumbits -= SHIFT;
				assert(accumbits < SHIFT);
			}
		}
		assert(accumbits < SHIFT);
		if (accumbits) {
			assert(idigit < (int)ndigits);
			v->ob_digit[idigit] = (digit)accum;
			++idigit;
		}
	}

	v->ob_size = is_signed ? -idigit : idigit;
	return (PyObject *)long_normalize(v);
}

int
_PyLong_AsByteArray(PyLongObject* v,
		    unsigned char* bytes, size_t n,
		    int little_endian, int is_signed)
{
	int i;			/* index into v->ob_digit */
	int ndigits;		/* |v->ob_size| */
	twodigits accum;	/* sliding register */
	unsigned int accumbits; /* # bits in accum */
	int do_twos_comp;	/* store 2's-comp?  is_signed and v < 0 */
	twodigits carry;	/* for computing 2's-comp */
	size_t j;		/* # bytes filled */
	unsigned char* p;	/* pointer to next byte in bytes */
	int pincr;		/* direction to move p */

	assert(v != NULL && PyLong_Check(v));

	if (v->ob_size < 0) {
		ndigits = -(v->ob_size);
		if (!is_signed) {
			PyErr_SetString(PyExc_TypeError,
				"can't convert negative long to unsigned");
			return -1;
		}
		do_twos_comp = 1;
	}
	else {
		ndigits = v->ob_size;
		do_twos_comp = 0;
	}

	if (little_endian) {
		p = bytes;
		pincr = 1;
	}
	else {
		p = bytes + n - 1;
		pincr = -1;
	}

	/* Copy over all the Python digits.
	   It's crucial that every Python digit except for the MSD contribute
	   exactly SHIFT bits to the total, so first assert that the long is
	   normalized. */
	assert(ndigits == 0 || v->ob_digit[ndigits - 1] != 0);
	j = 0;
	accum = 0;
	accumbits = 0;
	carry = do_twos_comp ? 1 : 0;
	for (i = 0; i < ndigits; ++i) {
		twodigits thisdigit = v->ob_digit[i];
		if (do_twos_comp) {
			thisdigit = (thisdigit ^ MASK) + carry;
			carry = thisdigit >> SHIFT;
			thisdigit &= MASK;
		}
		/* Because we're going LSB to MSB, thisdigit is more
		   significant than what's already in accum, so needs to be
		   prepended to accum. */
		accum |= thisdigit << accumbits;
		accumbits += SHIFT;

		/* The most-significant digit may be (probably is) at least
		   partly empty. */
		if (i == ndigits - 1) {
			/* Count # of sign bits -- they needn't be stored,
			 * although for signed conversion we need later to
			 * make sure at least one sign bit gets stored.
			 * First shift conceptual sign bit to real sign bit.
			 */
			stwodigits s = (stwodigits)(thisdigit <<
				(8*sizeof(stwodigits) - SHIFT));
			unsigned int nsignbits = 0;
			while ((s < 0) == do_twos_comp && nsignbits < SHIFT) {
				++nsignbits;
				s <<= 1;
			}
			accumbits -= nsignbits;
		}

		/* Store as many bytes as possible. */
		while (accumbits >= 8) {
			if (j >= n)
				goto Overflow;
			++j;
			*p = (unsigned char)(accum & 0xff);
			p += pincr;
			accumbits -= 8;
			accum >>= 8;
		}
	}

	/* Store the straggler (if any). */
	assert(accumbits < 8);
	assert(carry == 0);  /* else do_twos_comp and *every* digit was 0 */
	if (accumbits > 0) {
		if (j >= n)
			goto Overflow;
		++j;
		if (do_twos_comp) {
			/* Fill leading bits of the byte with sign bits
			   (appropriately pretending that the long had an
			   infinite supply of sign bits). */
			accum |= (~(twodigits)0) << accumbits;
		}
		*p = (unsigned char)(accum & 0xff);
		p += pincr;
	}
	else if (j == n && n > 0 && is_signed) {
		/* The main loop filled the byte array exactly, so the code
		   just above didn't get to ensure there's a sign bit, and the
		   loop below wouldn't add one either.  Make sure a sign bit
		   exists. */
		unsigned char msb = *(p - pincr);
		int sign_bit_set = msb >= 0x80;
		assert(accumbits == 0);
		if (sign_bit_set == do_twos_comp)
			return 0;
		else
			goto Overflow;
	}

	/* Fill remaining bytes with copies of the sign bit. */
	{
		unsigned char signbyte = do_twos_comp ? 0xffU : 0U;
		for ( ; j < n; ++j, p += pincr)
			*p = signbyte;
	}

	return 0;

Overflow:
	PyErr_SetString(PyExc_OverflowError, "long too big to convert");
	return -1;
	
}

double
_PyLong_AsScaledDouble(PyObject *vv, int *exponent)
{
/* NBITS_WANTED should be > the number of bits in a double's precision,
   but small enough so that 2**NBITS_WANTED is within the normal double
   range.  nbitsneeded is set to 1 less than that because the most-significant
   Python digit contains at least 1 significant bit, but we don't want to
   bother counting them (catering to the worst case cheaply).

   57 is one more than VAX-D double precision; I (Tim) don't know of a double
   format with more precision than that; it's 1 larger so that we add in at
   least one round bit to stand in for the ignored least-significant bits.
*/
#define NBITS_WANTED 57
	PyLongObject *v;
	double x;
	const double multiplier = (double)(1L << SHIFT);
	int i, sign;
	int nbitsneeded;

	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return -1;
	}
	v = (PyLongObject *)vv;
	i = v->ob_size;
	sign = 1;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
	else if (i == 0) {
		*exponent = 0;
		return 0.0;
	}
	--i;
	x = (double)v->ob_digit[i];
	nbitsneeded = NBITS_WANTED - 1;
	/* Invariant:  i Python digits remain unaccounted for. */
	while (i > 0 && nbitsneeded > 0) {
		--i;
		x = x * multiplier + (double)v->ob_digit[i];
		nbitsneeded -= SHIFT;
	}
	/* There are i digits we didn't shift in.  Pretending they're all
	   zeroes, the true value is x * 2**(i*SHIFT). */
	*exponent = i;
	assert(x > 0.0);
	return x * sign;
#undef NBITS_WANTED
}

/* Get a C double from a long int object. */

double
PyLong_AsDouble(PyObject *vv)
{
	int e;
	double x;

	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return -1;
	}
	x = _PyLong_AsScaledDouble(vv, &e);
	if (x == -1.0 && PyErr_Occurred())
		return -1.0;
	if (e > INT_MAX / SHIFT)
		goto overflow;
	errno = 0;
	x = ldexp(x, e * SHIFT);
	if (Py_OVERFLOWED(x))
		goto overflow;
	return x;

overflow:
	PyErr_SetString(PyExc_OverflowError,
		"long int too large to convert to float");
	return -1.0;
}

/* Create a new long (or int) object from a C pointer */

PyObject *
PyLong_FromVoidPtr(void *p)
{
#if SIZEOF_VOID_P <= SIZEOF_LONG
	return PyInt_FromLong((long)p);
#else

#ifndef HAVE_LONG_LONG
#   error "PyLong_FromVoidPtr: sizeof(void*) > sizeof(long), but no long long"
#endif
#if SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_FromVoidPtr: sizeof(LONG_LONG) < sizeof(void*)"
#endif
	/* optimize null pointers */
	if (p == NULL)
		return PyInt_FromLong(0);
	return PyLong_FromLongLong((LONG_LONG)p);

#endif /* SIZEOF_VOID_P <= SIZEOF_LONG */
}

/* Get a C pointer from a long object (or an int object in some cases) */

void *
PyLong_AsVoidPtr(PyObject *vv)
{
	/* This function will allow int or long objects. If vv is neither,
	   then the PyLong_AsLong*() functions will raise the exception:
	   PyExc_SystemError, "bad argument to internal function"
	*/
#if SIZEOF_VOID_P <= SIZEOF_LONG
	long x;

	if (PyInt_Check(vv))
		x = PyInt_AS_LONG(vv);
	else
		x = PyLong_AsLong(vv);
#else

#ifndef HAVE_LONG_LONG
#   error "PyLong_AsVoidPtr: sizeof(void*) > sizeof(long), but no long long"
#endif
#if SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_AsVoidPtr: sizeof(LONG_LONG) < sizeof(void*)"
#endif
	LONG_LONG x;

	if (PyInt_Check(vv))
		x = PyInt_AS_LONG(vv);
	else
		x = PyLong_AsLongLong(vv);

#endif /* SIZEOF_VOID_P <= SIZEOF_LONG */

	if (x == -1 && PyErr_Occurred())
		return NULL;
	return (void *)x;
}

#ifdef HAVE_LONG_LONG

/* Initial LONG_LONG support by Chris Herborth (chrish@qnx.com), later
 * rewritten to use the newer PyLong_{As,From}ByteArray API.
 */

#define IS_LITTLE_ENDIAN (int)*(unsigned char*)&one

/* Create a new long int object from a C LONG_LONG int. */

PyObject *
PyLong_FromLongLong(LONG_LONG ival)
{
	LONG_LONG bytes = ival;
	int one = 1;
	return _PyLong_FromByteArray(
			(unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 1);
}

/* Create a new long int object from a C unsigned LONG_LONG int. */

PyObject *
PyLong_FromUnsignedLongLong(unsigned LONG_LONG ival)
{
	unsigned LONG_LONG bytes = ival;
	int one = 1;
	return _PyLong_FromByteArray(
			(unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 0);
}

/* Get a C LONG_LONG int from a long int object.
   Return -1 and set an error if overflow occurs. */

LONG_LONG
PyLong_AsLongLong(PyObject *vv)
{
	LONG_LONG bytes;
	int one = 1;
	int res;

	if (vv == NULL) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (!PyLong_Check(vv)) {
		if (PyInt_Check(vv))
			return (LONG_LONG)PyInt_AsLong(vv);
		PyErr_BadInternalCall();
		return -1;
	}

	res = _PyLong_AsByteArray(
			(PyLongObject *)vv, (unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 1);

	/* Plan 9 can't handle LONG_LONG in ? : expressions */
	if (res < 0)
		return (LONG_LONG)res;
	else
		return bytes;
}

/* Get a C unsigned LONG_LONG int from a long int object.
   Return -1 and set an error if overflow occurs. */

unsigned LONG_LONG
PyLong_AsUnsignedLongLong(PyObject *vv)
{
	unsigned LONG_LONG bytes;
	int one = 1;
	int res;

	if (vv == NULL || !PyLong_Check(vv)) {
		PyErr_BadInternalCall();
		return -1;
	}

	res = _PyLong_AsByteArray(
			(PyLongObject *)vv, (unsigned char *)&bytes,
			SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 0);

	/* Plan 9 can't handle LONG_LONG in ? : expressions */
	if (res < 0)
		return (unsigned LONG_LONG)res;
	else
		return bytes;
}

#undef IS_LITTLE_ENDIAN

#endif /* HAVE_LONG_LONG */


static int
convert_binop(PyObject *v, PyObject *w, PyLongObject **a, PyLongObject **b) {
	if (PyLong_Check(v)) { 
		*a = (PyLongObject *) v;
		Py_INCREF(v);
	}
	else if (PyInt_Check(v)) {
		*a = (PyLongObject *) PyLong_FromLong(PyInt_AS_LONG(v));
	}
	else {
		return 0;
	}
	if (PyLong_Check(w)) { 
		*b = (PyLongObject *) w;
		Py_INCREF(w);
	}
	else if (PyInt_Check(w)) {
		*b = (PyLongObject *) PyLong_FromLong(PyInt_AS_LONG(w));
	}
	else {
		Py_DECREF(*a);
		return 0;
	}
	return 1;
}

#define CONVERT_BINOP(v, w, a, b) \
	if (!convert_binop(v, w, a, b)) { \
		Py_INCREF(Py_NotImplemented); \
		return Py_NotImplemented; \
	}


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

/* Divide long pin, w/ size digits, by non-zero digit n, storing quotient
   in pout, and returning the remainder.  pin and pout point at the LSD.
   It's OK for pin == pout on entry, which saves oodles of mallocs/frees in
   long_format, but that should be done with great care since longs are
   immutable. */

static digit
inplace_divrem1(digit *pout, digit *pin, int size, digit n)
{
	twodigits rem = 0;

	assert(n > 0 && n <= MASK);
	pin += size;
	pout += size;
	while (--size >= 0) {
		digit hi;
		rem = (rem << SHIFT) + *--pin;
		*--pout = hi = (digit)(rem / n);
		rem -= hi * n;
	}
	return (digit)rem;
}

/* Divide a long integer by a digit, returning both the quotient
   (as function result) and the remainder (through *prem).
   The sign of a is ignored; n should not be zero. */

static PyLongObject *
divrem1(PyLongObject *a, digit n, digit *prem)
{
	const int size = ABS(a->ob_size);
	PyLongObject *z;
	
	assert(n > 0 && n <= MASK);
	z = _PyLong_New(size);
	if (z == NULL)
		return NULL;
	*prem = inplace_divrem1(z->ob_digit, a->ob_digit, size, n);
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
	const int size_a = ABS(a->ob_size);
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
		twodigits accum = 0;
		int accumbits = 0;	/* # of bits in accum */
		int basebits = 1;	/* # of bits in base-1 */
		i = base;
		while ((i >>= 1) > 1)
			++basebits;

		for (i = 0; i < size_a; ++i) {
			accum |= a->ob_digit[i] << accumbits;
			accumbits += SHIFT;
			assert(accumbits >= basebits);
			do {
				char cdigit = (char)(accum & (base - 1));
				cdigit += (cdigit < 10) ? '0' : 'A'-10;
				assert(p > PyString_AS_STRING(str));
				*--p = cdigit;
				accumbits -= basebits;
				accum >>= basebits;
			} while (i < size_a-1 ? accumbits >= basebits :
					 	accum > 0);
		}
	}
	else {
		/* Not 0, and base not a power of 2.  Divide repeatedly by
		   base, but for speed use the highest power of base that
		   fits in a digit. */
		int size = size_a;
		digit *pin = a->ob_digit;
		PyLongObject *scratch;
		/* powbasw <- largest power of base that fits in a digit. */
		digit powbase = base;  /* powbase == base ** power */
		int power = 1;
		for (;;) {
			unsigned long newpow = powbase * (unsigned long)base;
			if (newpow >> SHIFT)  /* doesn't fit in a digit */
				break;
			powbase = (digit)newpow;
			++power;
		}

		/* Get a scratch area for repeated division. */
		scratch = _PyLong_New(size);
		if (scratch == NULL) {
			Py_DECREF(str);
			return NULL;
		}

		/* Repeatedly divide by powbase. */
		do {
			int ntostore = power;
			digit rem = inplace_divrem1(scratch->ob_digit,
						     pin, size, powbase);
			pin = scratch->ob_digit; /* no need to use a again */
			if (pin[size - 1] == 0)
				--size;
			SIGCHECK({
				Py_DECREF(scratch);
				Py_DECREF(str);
				return NULL;
			})

			/* Break rem into digits. */
			assert(ntostore > 0);
			do {
				digit nextrem = (digit)(rem / base);
				char c = (char)(rem - nextrem * base);
				assert(p > PyString_AS_STRING(str));
				c += (c < 10) ? '0' : 'A'-10;
				*--p = c;
				rem = nextrem;
				--ntostore;
				/* Termination is a bit delicate:  must not
				   store leading zeroes, so must get out if
				   remaining quotient and rem are both 0. */
			} while (ntostore && (size || rem));
		} while (size != 0);
		Py_DECREF(scratch);
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
				"long() arg 2 must be >= 2 and <= 36");
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

#ifdef Py_USING_UNICODE
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
#endif

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
				"long division or modulo by zero");
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
	v->ob_type->tp_free(v);
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
long_add(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b, *z;

	CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

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
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *)z;
}

static PyObject *
long_sub(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b, *z;
	
	CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

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
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *)z;
}

static PyObject *
long_repeat(PyObject *v, PyLongObject *w)
{
	/* sequence * long */
	long n = PyLong_AsLong((PyObject *) w);
	if (n == -1 && PyErr_Occurred())
		return NULL;
	else
		return (*v->ob_type->tp_as_sequence->sq_repeat)(v, n);
}

static PyObject *
long_mul(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b, *z;
	int size_a;
	int size_b;
	int i;

	if (!convert_binop((PyObject *)v, (PyObject *)w, &a, &b)) {
		if (!PyLong_Check(v) &&
		    v->ob_type->tp_as_sequence &&
		    v->ob_type->tp_as_sequence->sq_repeat)
			return long_repeat((PyObject *)v, w);
		if (!PyLong_Check(w) &&
			 w->ob_type->tp_as_sequence &&
			 w->ob_type->tp_as_sequence->sq_repeat)
			return long_repeat((PyObject *)w, v);
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

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
	if (z == NULL) {
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	for (i = 0; i < z->ob_size; ++i)
		z->ob_digit[i] = 0;
	for (i = 0; i < size_a; ++i) {
		twodigits carry = 0;
		twodigits f = a->ob_digit[i];
		int j;
		
		SIGCHECK({
			Py_DECREF(a);
			Py_DECREF(b);
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
	Py_DECREF(a);
	Py_DECREF(b);
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
long_div(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(a, b, &div, &mod) < 0) {
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	Py_DECREF(a);
	Py_DECREF(b);
	Py_DECREF(mod);
	return (PyObject *)div;
}

static PyObject *
long_classic_div(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (Py_DivisionWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, "classic long division") < 0)
		div = NULL;
	else if (l_divmod(a, b, &div, &mod) < 0)
		div = NULL;
	else
		Py_DECREF(mod);

	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *)div;
}

static PyObject *
long_true_divide(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	double ad, bd;
	int aexp, bexp, failed;

	CONVERT_BINOP(v, w, &a, &b);
	ad = _PyLong_AsScaledDouble((PyObject *)a, &aexp);
	bd = _PyLong_AsScaledDouble((PyObject *)b, &bexp);
	failed = (ad == -1.0 || bd == -1.0) && PyErr_Occurred();
	Py_DECREF(a);
	Py_DECREF(b);
	if (failed)
		return NULL;

	if (bd == 0.0) {
		PyErr_SetString(PyExc_ZeroDivisionError,
			"long division or modulo by zero");
		return NULL;
	}

	/* True value is very close to ad/bd * 2**(SHIFT*(aexp-bexp)) */
	ad /= bd;	/* overflow/underflow impossible here */
	aexp -= bexp;
	if (aexp > INT_MAX / SHIFT)
		goto overflow;
	else if (aexp < -(INT_MAX / SHIFT))
		return PyFloat_FromDouble(0.0);	/* underflow to 0 */
	errno = 0;
	ad = ldexp(ad, aexp * SHIFT);
	if (Py_OVERFLOWED(ad)) /* ignore underflow to 0.0 */
		goto overflow;
	return PyFloat_FromDouble(ad);

overflow:
	PyErr_SetString(PyExc_OverflowError,
		"long/long too large for a float");
	return NULL;
	
}

static PyObject *
long_mod(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(a, b, &div, &mod) < 0) {
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	Py_DECREF(a);
	Py_DECREF(b);
	Py_DECREF(div);
	return (PyObject *)mod;
}

static PyObject *
long_divmod(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b, *div, *mod;
	PyObject *z;

	CONVERT_BINOP(v, w, &a, &b);

	if (l_divmod(a, b, &div, &mod) < 0) {
		Py_DECREF(a);
		Py_DECREF(b);
		return NULL;
	}
	z = PyTuple_New(2);
	if (z != NULL) {
		PyTuple_SetItem(z, 0, (PyObject *) div);
		PyTuple_SetItem(z, 1, (PyObject *) mod);
	}
	else {
		Py_DECREF(div);
		Py_DECREF(mod);
	}
	Py_DECREF(a);
	Py_DECREF(b);
	return z;
}

static PyObject *
long_pow(PyObject *v, PyObject *w, PyObject *x)
{
	PyLongObject *a, *b;
	PyObject *c;
	PyLongObject *z, *div, *mod;
	int size_b, i;

	CONVERT_BINOP(v, w, &a, &b);
	if (PyLong_Check(x) || Py_None == x) { 
		c = x;
		Py_INCREF(x);
	}
	else if (PyInt_Check(x)) {
		c = PyLong_FromLong(PyInt_AS_LONG(x));
	}
	else {
		Py_DECREF(a);
		Py_DECREF(b);
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	if (c != Py_None && ((PyLongObject *)c)->ob_size == 0) {
		PyErr_SetString(PyExc_ValueError,
				"pow() 3rd argument cannot be 0");
		z = NULL;
		goto error;
	}

	size_b = b->ob_size;
	if (size_b < 0) {
		Py_DECREF(a);
		Py_DECREF(b);
		Py_DECREF(c);
		if (x != Py_None) {
			PyErr_SetString(PyExc_TypeError, "pow() 2nd argument "
			     "cannot be negative when 3rd argument specified");
			return NULL;
		}
		/* Return a float.  This works because we know that
		   this calls float_pow() which converts its
		   arguments to double. */
		return PyFloat_Type.tp_as_number->nb_power(v, w, x);
	}
	z = (PyLongObject *)PyLong_FromLong(1L);
	for (i = 0; i < size_b; ++i) {
		digit bi = b->ob_digit[i];
		int j;
	
		for (j = 0; j < SHIFT; ++j) {
			PyLongObject *temp;
		
			if (bi & 1) {
				temp = (PyLongObject *)long_mul(z, a);
				Py_DECREF(z);
			 	if (c!=Py_None && temp!=NULL) {
			 		if (l_divmod(temp,(PyLongObject *)c,
							&div,&mod) < 0) {
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
		 	if (c!=Py_None && temp!=NULL) {
			 	if (l_divmod(temp, (PyLongObject *)c, &div,
							&mod) < 0) {
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
	if (c!=Py_None && z!=NULL) {
		if (l_divmod(z, (PyLongObject *)c, &div, &mod) < 0) {
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
	Py_XDECREF(a);
	Py_DECREF(b);
	Py_DECREF(c);
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
	x->ob_size = -(x->ob_size);
	return (PyObject *)x;
}

static PyObject *
long_pos(PyLongObject *v)
{
	if (PyLong_CheckExact(v)) {
		Py_INCREF(v);
		return (PyObject *)v;
	}
	else
		return _PyLong_Copy(v);
}

static PyObject *
long_neg(PyLongObject *v)
{
	PyLongObject *z;
	if (v->ob_size == 0 && PyLong_CheckExact(v)) {
		/* -0 == 0 */
		Py_INCREF(v);
		return (PyObject *) v;
	}
	z = (PyLongObject *)_PyLong_Copy(v);
	if (z != NULL)
		z->ob_size = -(v->ob_size);
	return (PyObject *)z;
}

static PyObject *
long_abs(PyLongObject *v)
{
	if (v->ob_size < 0)
		return long_neg(v);
	else
		return long_pos(v);
}

static int
long_nonzero(PyLongObject *v)
{
	return ABS(v->ob_size) != 0;
}

static PyObject *
long_rshift(PyLongObject *v, PyLongObject *w)
{
	PyLongObject *a, *b;
	PyLongObject *z = NULL;
	long shiftby;
	int newsize, wordshift, loshift, hishift, i, j;
	digit lomask, himask;
	
	CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

	if (a->ob_size < 0) {
		/* Right shifting negative numbers is harder */
		PyLongObject *a1, *a2;
		a1 = (PyLongObject *) long_invert(a);
		if (a1 == NULL)
			goto rshift_error;
		a2 = (PyLongObject *) long_rshift(a1, b);
		Py_DECREF(a1);
		if (a2 == NULL)
			goto rshift_error;
		z = (PyLongObject *) long_invert(a2);
		Py_DECREF(a2);
	}
	else {
		
		shiftby = PyLong_AsLong((PyObject *)b);
		if (shiftby == -1L && PyErr_Occurred())
			goto rshift_error;
		if (shiftby < 0) {
			PyErr_SetString(PyExc_ValueError,
					"negative shift count");
			goto rshift_error;
		}
		wordshift = shiftby / SHIFT;
		newsize = ABS(a->ob_size) - wordshift;
		if (newsize <= 0) {
			z = _PyLong_New(0);
			Py_DECREF(a);
			Py_DECREF(b);
			return (PyObject *)z;
		}
		loshift = shiftby % SHIFT;
		hishift = SHIFT - loshift;
		lomask = ((digit)1 << hishift) - 1;
		himask = MASK ^ lomask;
		z = _PyLong_New(newsize);
		if (z == NULL)
			goto rshift_error;
		if (a->ob_size < 0)
			z->ob_size = -(z->ob_size);
		for (i = 0, j = wordshift; i < newsize; i++, j++) {
			z->ob_digit[i] = (a->ob_digit[j] >> loshift) & lomask;
			if (i+1 < newsize)
				z->ob_digit[i] |=
				  (a->ob_digit[j+1] << hishift) & himask;
		}
		z = long_normalize(z);
	}
rshift_error:
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *) z;

}

static PyObject *
long_lshift(PyObject *v, PyObject *w)
{
	/* This version due to Tim Peters */
	PyLongObject *a, *b;
	PyLongObject *z = NULL;
	long shiftby;
	int oldsize, newsize, wordshift, remshift, i, j;
	twodigits accum;
	
	CONVERT_BINOP(v, w, &a, &b);

	shiftby = PyLong_AsLong((PyObject *)b);
	if (shiftby == -1L && PyErr_Occurred())
		goto lshift_error;
	if (shiftby < 0) {
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		goto lshift_error;
	}
	if ((long)(int)shiftby != shiftby) {
		PyErr_SetString(PyExc_ValueError,
				"outrageous left shift count");
		goto lshift_error;
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
		goto lshift_error;
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
	z = long_normalize(z);
lshift_error:
	Py_DECREF(a);
	Py_DECREF(b);
	return (PyObject *) z;
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
long_and(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	PyObject *c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(a, '&', b);
	Py_DECREF(a);
	Py_DECREF(b);
	return c;
}

static PyObject *
long_xor(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	PyObject *c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(a, '^', b);
	Py_DECREF(a);
	Py_DECREF(b);
	return c;
}

static PyObject *
long_or(PyObject *v, PyObject *w)
{
	PyLongObject *a, *b;
	PyObject *c;
	CONVERT_BINOP(v, w, &a, &b);
	c = long_bitwise(a, '|', b);
	Py_DECREF(a);
	Py_DECREF(b);
	return c;
}

static int
long_coerce(PyObject **pv, PyObject **pw)
{
	if (PyInt_Check(*pw)) {
		*pw = PyLong_FromLong(PyInt_AS_LONG(*pw));
		Py_INCREF(*pv);
		return 0;
	}
	else if (PyLong_Check(*pw)) {
		Py_INCREF(*pv);
		Py_INCREF(*pw);
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
	result = PyLong_AsDouble(v);
	if (result == -1.0 && PyErr_Occurred())
		return NULL;
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
staticforward PyObject *
long_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
long_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *x = NULL;
	int base = -909;		     /* unlikely! */
	static char *kwlist[] = {"x", "base", 0};

	if (type != &PyLong_Type)
		return long_subtype_new(type, args, kwds); /* Wimp out */
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi:long", kwlist,
					 &x, &base))
		return NULL;
	if (x == NULL)
		return PyLong_FromLong(0L);
	if (base == -909)
		return PyNumber_Long(x);
	else if (PyString_Check(x))
		return PyLong_FromString(PyString_AS_STRING(x), NULL, base);
#ifdef Py_USING_UNICODE
	else if (PyUnicode_Check(x))
		return PyLong_FromUnicode(PyUnicode_AS_UNICODE(x),
					  PyUnicode_GET_SIZE(x),
					  base);
#endif
	else {
		PyErr_SetString(PyExc_TypeError,
			"long() can't convert non-string with explicit base");
		return NULL;
	}
}

/* Wimpy, slow approach to tp_new calls for subtypes of long:
   first create a regular long from whatever arguments we got,
   then allocate a subtype instance and initialize it from
   the regular long.  The regular long is then thrown away.
*/
static PyObject *
long_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyLongObject *tmp, *new;
	int i, n;

	assert(PyType_IsSubtype(type, &PyLong_Type));
	tmp = (PyLongObject *)long_new(&PyLong_Type, args, kwds);
	if (tmp == NULL)
		return NULL;
	assert(PyLong_CheckExact(tmp));
	n = tmp->ob_size;
	if (n < 0)
		n = -n;
	new = (PyLongObject *)type->tp_alloc(type, n);
	if (new == NULL)
		return NULL;
	assert(PyLong_Check(new));
	new->ob_size = tmp->ob_size;
	for (i = 0; i < n; i++)
		new->ob_digit[i] = tmp->ob_digit[i];
	Py_DECREF(tmp);
	return (PyObject *)new;
}

static char long_doc[] =
"long(x[, base]) -> integer\n\
\n\
Convert a string or number to a long integer, if possible.  A floating\n\
point argument will be truncated towards zero (this does not include a\n\
string representation of a floating point number!)  When converting a\n\
string, use the optional base.  It is an error to supply a base when\n\
converting a non-string.";

static PyNumberMethods long_as_number = {
	(binaryfunc)	long_add,	/*nb_add*/
	(binaryfunc)	long_sub,	/*nb_subtract*/
	(binaryfunc)	long_mul,	/*nb_multiply*/
	(binaryfunc)	long_classic_div, /*nb_divide*/
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
	0,				/* nb_inplace_add */
	0,				/* nb_inplace_subtract */
	0,				/* nb_inplace_multiply */
	0,				/* nb_inplace_divide */
	0,				/* nb_inplace_remainder */
	0,				/* nb_inplace_power */
	0,				/* nb_inplace_lshift */
	0,				/* nb_inplace_rshift */
	0,				/* nb_inplace_and */
	0,				/* nb_inplace_xor */
	0,				/* nb_inplace_or */
	(binaryfunc)long_div,		/* nb_floor_divide */
	long_true_divide,		/* nb_true_divide */
	0,				/* nb_inplace_floor_divide */
	0,				/* nb_inplace_true_divide */
};

PyTypeObject PyLong_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"long",					/* tp_name */
	sizeof(PyLongObject) - sizeof(digit),	/* tp_basicsize */
	sizeof(digit),				/* tp_itemsize */
	(destructor)long_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	(cmpfunc)long_compare,			/* tp_compare */
	(reprfunc)long_repr,			/* tp_repr */
	&long_as_number,			/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)long_hash,			/* tp_hash */
        0,              			/* tp_call */
        (reprfunc)long_str,			/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE,		/* tp_flags */
	long_doc,				/* tp_doc */
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
	long_new,				/* tp_new */
	_PyObject_Del,				/* tp_free */
};
