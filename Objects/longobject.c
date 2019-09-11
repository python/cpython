/* Long (arbitrary precision) integer object implementation */

/* XXX The functional organization of this file is terrible */

#include "Python.h"
#include "longintrepr.h"

#include <float.h>
#include <ctype.h>
#include <stddef.h>

#include "clinic/longobject.c.h"
/*[clinic input]
class int "PyObject *" "&PyLong_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ec0275e3422a36e3]*/

#ifndef NSMALLPOSINTS
#define NSMALLPOSINTS           257
#endif
#ifndef NSMALLNEGINTS
#define NSMALLNEGINTS           5
#endif

_Py_IDENTIFIER(little);
_Py_IDENTIFIER(big);

/* convert a PyLong of size 1, 0 or -1 to an sdigit */
#define MEDIUM_VALUE(x) (assert(-1 <= Py_SIZE(x) && Py_SIZE(x) <= 1),   \
         Py_SIZE(x) < 0 ? -(sdigit)(x)->ob_digit[0] :   \
             (Py_SIZE(x) == 0 ? (sdigit)0 :                             \
              (sdigit)(x)->ob_digit[0]))

PyObject *_PyLong_Zero = NULL;
PyObject *_PyLong_One = NULL;

#if NSMALLNEGINTS + NSMALLPOSINTS > 0
/* Small integers are preallocated in this array so that they
   can be shared.
   The integers that are preallocated are those in the range
   -NSMALLNEGINTS (inclusive) to NSMALLPOSINTS (not inclusive).
*/
static PyLongObject small_ints[NSMALLNEGINTS + NSMALLPOSINTS];
#ifdef COUNT_ALLOCS
Py_ssize_t quick_int_allocs, quick_neg_int_allocs;
#endif

static PyObject *
get_small_int(sdigit ival)
{
    PyObject *v;
    assert(-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS);
    v = (PyObject *)&small_ints[ival + NSMALLNEGINTS];
    Py_INCREF(v);
#ifdef COUNT_ALLOCS
    if (ival >= 0)
        quick_int_allocs++;
    else
        quick_neg_int_allocs++;
#endif
    return v;
}
#define CHECK_SMALL_INT(ival) \
    do if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS) { \
        return get_small_int((sdigit)ival); \
    } while(0)

static PyLongObject *
maybe_small_long(PyLongObject *v)
{
    if (v && Py_ABS(Py_SIZE(v)) <= 1) {
        sdigit ival = MEDIUM_VALUE(v);
        if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS) {
            Py_DECREF(v);
            return (PyLongObject *)get_small_int(ival);
        }
    }
    return v;
}
#else
#define CHECK_SMALL_INT(ival)
#define maybe_small_long(val) (val)
#endif

/* If a freshly-allocated int is already shared, it must
   be a small integer, so negating it must go to PyLong_FromLong */
Py_LOCAL_INLINE(void)
_PyLong_Negate(PyLongObject **x_p)
{
    PyLongObject *x;

    x = (PyLongObject *)*x_p;
    if (Py_REFCNT(x) == 1) {
        Py_SIZE(x) = -Py_SIZE(x);
        return;
    }

    *x_p = (PyLongObject *)PyLong_FromLong(-MEDIUM_VALUE(x));
    Py_DECREF(x);
}

/* For int multiplication, use the O(N**2) school algorithm unless
 * both operands contain more than KARATSUBA_CUTOFF digits (this
 * being an internal Python int digit, in base BASE).
 */
#define KARATSUBA_CUTOFF 70
#define KARATSUBA_SQUARE_CUTOFF (2 * KARATSUBA_CUTOFF)

/* For exponentiation, use the binary left-to-right algorithm
 * unless the exponent contains more than FIVEARY_CUTOFF digits.
 * In that case, do 5 bits at a time.  The potential drawback is that
 * a table of 2**5 intermediate results is computed.
 */
#define FIVEARY_CUTOFF 8

#define SIGCHECK(PyTryBlock)                    \
    do {                                        \
        if (PyErr_CheckSignals()) PyTryBlock    \
    } while(0)

/* Normalize (remove leading zeros from) an int object.
   Doesn't attempt to free the storage--in most cases, due to the nature
   of the algorithms used, this could save at most be one word anyway. */

static PyLongObject *
long_normalize(PyLongObject *v)
{
    Py_ssize_t j = Py_ABS(Py_SIZE(v));
    Py_ssize_t i = j;

    while (i > 0 && v->ob_digit[i-1] == 0)
        --i;
    if (i != j)
        Py_SIZE(v) = (Py_SIZE(v) < 0) ? -(i) : i;
    return v;
}

/* _PyLong_FromNbInt: Convert the given object to a PyLongObject
   using the nb_int slot, if available.  Raise TypeError if either the
   nb_int slot is not available or the result of the call to nb_int
   returns something not of type int.
*/
PyLongObject *
_PyLong_FromNbInt(PyObject *integral)
{
    PyNumberMethods *nb;
    PyObject *result;

    /* Fast path for the case that we already have an int. */
    if (PyLong_CheckExact(integral)) {
        Py_INCREF(integral);
        return (PyLongObject *)integral;
    }

    nb = Py_TYPE(integral)->tp_as_number;
    if (nb == NULL || nb->nb_int == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "an integer is required (got type %.200s)",
                     Py_TYPE(integral)->tp_name);
        return NULL;
    }

    /* Convert using the nb_int slot, which should return something
       of exact type int. */
    result = nb->nb_int(integral);
    if (!result || PyLong_CheckExact(result))
        return (PyLongObject *)result;
    if (!PyLong_Check(result)) {
        PyErr_Format(PyExc_TypeError,
                     "__int__ returned non-int (type %.200s)",
                     result->ob_type->tp_name);
        Py_DECREF(result);
        return NULL;
    }
    /* Issue #17576: warn if 'result' not of exact type int. */
    if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
            "__int__ returned non-int (type %.200s).  "
            "The ability to return an instance of a strict subclass of int "
            "is deprecated, and may be removed in a future version of Python.",
            result->ob_type->tp_name)) {
        Py_DECREF(result);
        return NULL;
    }
    return (PyLongObject *)result;
}


/* Allocate a new int object with size digits.
   Return NULL and set exception if we run out of memory. */

#define MAX_LONG_DIGITS \
    ((PY_SSIZE_T_MAX - offsetof(PyLongObject, ob_digit))/sizeof(digit))

PyLongObject *
_PyLong_New(Py_ssize_t size)
{
    PyLongObject *result;
    /* Number of bytes needed is: offsetof(PyLongObject, ob_digit) +
       sizeof(digit)*size.  Previous incarnations of this code used
       sizeof(PyVarObject) instead of the offsetof, but this risks being
       incorrect in the presence of padding between the PyVarObject header
       and the digits. */
    if (size > (Py_ssize_t)MAX_LONG_DIGITS) {
        PyErr_SetString(PyExc_OverflowError,
                        "too many digits in integer");
        return NULL;
    }
    result = PyObject_MALLOC(offsetof(PyLongObject, ob_digit) +
                             size*sizeof(digit));
    if (!result) {
        PyErr_NoMemory();
        return NULL;
    }
    return (PyLongObject*)PyObject_INIT_VAR(result, &PyLong_Type, size);
}

PyObject *
_PyLong_Copy(PyLongObject *src)
{
    PyLongObject *result;
    Py_ssize_t i;

    assert(src != NULL);
    i = Py_SIZE(src);
    if (i < 0)
        i = -(i);
    if (i < 2) {
        sdigit ival = MEDIUM_VALUE(src);
        CHECK_SMALL_INT(ival);
    }
    result = _PyLong_New(i);
    if (result != NULL) {
        Py_SIZE(result) = Py_SIZE(src);
        while (--i >= 0)
            result->ob_digit[i] = src->ob_digit[i];
    }
    return (PyObject *)result;
}

/* Create a new int object from a C long int */

PyObject *
PyLong_FromLong(long ival)
{
    PyLongObject *v;
    unsigned long abs_ival;
    unsigned long t;  /* unsigned so >> doesn't propagate sign bit */
    int ndigits = 0;
    int sign;

    CHECK_SMALL_INT(ival);

    if (ival < 0) {
        /* negate: can't write this as abs_ival = -ival since that
           invokes undefined behaviour when ival is LONG_MIN */
        abs_ival = 0U-(unsigned long)ival;
        sign = -1;
    }
    else {
        abs_ival = (unsigned long)ival;
        sign = ival == 0 ? 0 : 1;
    }

    /* Fast path for single-digit ints */
    if (!(abs_ival >> PyLong_SHIFT)) {
        v = _PyLong_New(1);
        if (v) {
            Py_SIZE(v) = sign;
            v->ob_digit[0] = Py_SAFE_DOWNCAST(
                abs_ival, unsigned long, digit);
        }
        return (PyObject*)v;
    }

#if PyLong_SHIFT==15
    /* 2 digits */
    if (!(abs_ival >> 2*PyLong_SHIFT)) {
        v = _PyLong_New(2);
        if (v) {
            Py_SIZE(v) = 2*sign;
            v->ob_digit[0] = Py_SAFE_DOWNCAST(
                abs_ival & PyLong_MASK, unsigned long, digit);
            v->ob_digit[1] = Py_SAFE_DOWNCAST(
                  abs_ival >> PyLong_SHIFT, unsigned long, digit);
        }
        return (PyObject*)v;
    }
#endif

    /* Larger numbers: loop to determine number of digits */
    t = abs_ival;
    while (t) {
        ++ndigits;
        t >>= PyLong_SHIFT;
    }
    v = _PyLong_New(ndigits);
    if (v != NULL) {
        digit *p = v->ob_digit;
        Py_SIZE(v) = ndigits*sign;
        t = abs_ival;
        while (t) {
            *p++ = Py_SAFE_DOWNCAST(
                t & PyLong_MASK, unsigned long, digit);
            t >>= PyLong_SHIFT;
        }
    }
    return (PyObject *)v;
}

/* Create a new int object from a C unsigned long int */

PyObject *
PyLong_FromUnsignedLong(unsigned long ival)
{
    PyLongObject *v;
    unsigned long t;
    int ndigits = 0;

    if (ival < PyLong_BASE)
        return PyLong_FromLong(ival);
    /* Count the number of Python digits. */
    t = ival;
    while (t) {
        ++ndigits;
        t >>= PyLong_SHIFT;
    }
    v = _PyLong_New(ndigits);
    if (v != NULL) {
        digit *p = v->ob_digit;
        while (ival) {
            *p++ = (digit)(ival & PyLong_MASK);
            ival >>= PyLong_SHIFT;
        }
    }
    return (PyObject *)v;
}

/* Create a new int object from a C double */

PyObject *
PyLong_FromDouble(double dval)
{
    PyLongObject *v;
    double frac;
    int i, ndig, expo, neg;
    neg = 0;
    if (Py_IS_INFINITY(dval)) {
        PyErr_SetString(PyExc_OverflowError,
                        "cannot convert float infinity to integer");
        return NULL;
    }
    if (Py_IS_NAN(dval)) {
        PyErr_SetString(PyExc_ValueError,
                        "cannot convert float NaN to integer");
        return NULL;
    }
    if (dval < 0.0) {
        neg = 1;
        dval = -dval;
    }
    frac = frexp(dval, &expo); /* dval = frac*2**expo; 0.0 <= frac < 1.0 */
    if (expo <= 0)
        return PyLong_FromLong(0L);
    ndig = (expo-1) / PyLong_SHIFT + 1; /* Number of 'digits' in result */
    v = _PyLong_New(ndig);
    if (v == NULL)
        return NULL;
    frac = ldexp(frac, (expo-1) % PyLong_SHIFT + 1);
    for (i = ndig; --i >= 0; ) {
        digit bits = (digit)frac;
        v->ob_digit[i] = bits;
        frac = frac - (double)bits;
        frac = ldexp(frac, PyLong_SHIFT);
    }
    if (neg)
        Py_SIZE(v) = -(Py_SIZE(v));
    return (PyObject *)v;
}

/* Checking for overflow in PyLong_AsLong is a PITA since C doesn't define
 * anything about what happens when a signed integer operation overflows,
 * and some compilers think they're doing you a favor by being "clever"
 * then.  The bit pattern for the largest positive signed long is
 * (unsigned long)LONG_MAX, and for the smallest negative signed long
 * it is abs(LONG_MIN), which we could write -(unsigned long)LONG_MIN.
 * However, some other compilers warn about applying unary minus to an
 * unsigned operand.  Hence the weird "0-".
 */
#define PY_ABS_LONG_MIN         (0-(unsigned long)LONG_MIN)
#define PY_ABS_SSIZE_T_MIN      (0-(size_t)PY_SSIZE_T_MIN)

/* Get a C long int from an int object or any object that has an __int__
   method.

   On overflow, return -1 and set *overflow to 1 or -1 depending on the sign of
   the result.  Otherwise *overflow is 0.

   For other errors (e.g., TypeError), return -1 and set an error condition.
   In this case *overflow will be 0.
*/

long
PyLong_AsLongAndOverflow(PyObject *vv, int *overflow)
{
    /* This version by Tim Peters */
    PyLongObject *v;
    unsigned long x, prev;
    long res;
    Py_ssize_t i;
    int sign;
    int do_decref = 0; /* if nb_int was called */

    *overflow = 0;
    if (vv == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }

    if (PyLong_Check(vv)) {
        v = (PyLongObject *)vv;
    }
    else {
        v = _PyLong_FromNbInt(vv);
        if (v == NULL)
            return -1;
        do_decref = 1;
    }

    res = -1;
    i = Py_SIZE(v);

    switch (i) {
    case -1:
        res = -(sdigit)v->ob_digit[0];
        break;
    case 0:
        res = 0;
        break;
    case 1:
        res = v->ob_digit[0];
        break;
    default:
        sign = 1;
        x = 0;
        if (i < 0) {
            sign = -1;
            i = -(i);
        }
        while (--i >= 0) {
            prev = x;
            x = (x << PyLong_SHIFT) | v->ob_digit[i];
            if ((x >> PyLong_SHIFT) != prev) {
                *overflow = sign;
                goto exit;
            }
        }
        /* Haven't lost any bits, but casting to long requires extra
         * care (see comment above).
         */
        if (x <= (unsigned long)LONG_MAX) {
            res = (long)x * sign;
        }
        else if (sign < 0 && x == PY_ABS_LONG_MIN) {
            res = LONG_MIN;
        }
        else {
            *overflow = sign;
            /* res is already set to -1 */
        }
    }
  exit:
    if (do_decref) {
        Py_DECREF(v);
    }
    return res;
}

/* Get a C long int from an int object or any object that has an __int__
   method.  Return -1 and set an error if overflow occurs. */

long
PyLong_AsLong(PyObject *obj)
{
    int overflow;
    long result = PyLong_AsLongAndOverflow(obj, &overflow);
    if (overflow) {
        /* XXX: could be cute and give a different
           message for overflow == -1 */
        PyErr_SetString(PyExc_OverflowError,
                        "Python int too large to convert to C long");
    }
    return result;
}

/* Get a C int from an int object or any object that has an __int__
   method.  Return -1 and set an error if overflow occurs. */

int
_PyLong_AsInt(PyObject *obj)
{
    int overflow;
    long result = PyLong_AsLongAndOverflow(obj, &overflow);
    if (overflow || result > INT_MAX || result < INT_MIN) {
        /* XXX: could be cute and give a different
           message for overflow == -1 */
        PyErr_SetString(PyExc_OverflowError,
                        "Python int too large to convert to C int");
        return -1;
    }
    return (int)result;
}

/* Get a Py_ssize_t from an int object.
   Returns -1 and sets an error condition if overflow occurs. */

Py_ssize_t
PyLong_AsSsize_t(PyObject *vv) {
    PyLongObject *v;
    size_t x, prev;
    Py_ssize_t i;
    int sign;

    if (vv == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (!PyLong_Check(vv)) {
        PyErr_SetString(PyExc_TypeError, "an integer is required");
        return -1;
    }

    v = (PyLongObject *)vv;
    i = Py_SIZE(v);
    switch (i) {
    case -1: return -(sdigit)v->ob_digit[0];
    case 0: return 0;
    case 1: return v->ob_digit[0];
    }
    sign = 1;
    x = 0;
    if (i < 0) {
        sign = -1;
        i = -(i);
    }
    while (--i >= 0) {
        prev = x;
        x = (x << PyLong_SHIFT) | v->ob_digit[i];
        if ((x >> PyLong_SHIFT) != prev)
            goto overflow;
    }
    /* Haven't lost any bits, but casting to a signed type requires
     * extra care (see comment above).
     */
    if (x <= (size_t)PY_SSIZE_T_MAX) {
        return (Py_ssize_t)x * sign;
    }
    else if (sign < 0 && x == PY_ABS_SSIZE_T_MIN) {
        return PY_SSIZE_T_MIN;
    }
    /* else overflow */

  overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "Python int too large to convert to C ssize_t");
    return -1;
}

/* Get a C unsigned long int from an int object.
   Returns -1 and sets an error condition if overflow occurs. */

unsigned long
PyLong_AsUnsignedLong(PyObject *vv)
{
    PyLongObject *v;
    unsigned long x, prev;
    Py_ssize_t i;

    if (vv == NULL) {
        PyErr_BadInternalCall();
        return (unsigned long)-1;
    }
    if (!PyLong_Check(vv)) {
        PyErr_SetString(PyExc_TypeError, "an integer is required");
        return (unsigned long)-1;
    }

    v = (PyLongObject *)vv;
    i = Py_SIZE(v);
    x = 0;
    if (i < 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "can't convert negative value to unsigned int");
        return (unsigned long) -1;
    }
    switch (i) {
    case 0: return 0;
    case 1: return v->ob_digit[0];
    }
    while (--i >= 0) {
        prev = x;
        x = (x << PyLong_SHIFT) | v->ob_digit[i];
        if ((x >> PyLong_SHIFT) != prev) {
            PyErr_SetString(PyExc_OverflowError,
                            "Python int too large to convert "
                            "to C unsigned long");
            return (unsigned long) -1;
        }
    }
    return x;
}

/* Get a C size_t from an int object. Returns (size_t)-1 and sets
   an error condition if overflow occurs. */

size_t
PyLong_AsSize_t(PyObject *vv)
{
    PyLongObject *v;
    size_t x, prev;
    Py_ssize_t i;

    if (vv == NULL) {
        PyErr_BadInternalCall();
        return (size_t) -1;
    }
    if (!PyLong_Check(vv)) {
        PyErr_SetString(PyExc_TypeError, "an integer is required");
        return (size_t)-1;
    }

    v = (PyLongObject *)vv;
    i = Py_SIZE(v);
    x = 0;
    if (i < 0) {
        PyErr_SetString(PyExc_OverflowError,
                   "can't convert negative value to size_t");
        return (size_t) -1;
    }
    switch (i) {
    case 0: return 0;
    case 1: return v->ob_digit[0];
    }
    while (--i >= 0) {
        prev = x;
        x = (x << PyLong_SHIFT) | v->ob_digit[i];
        if ((x >> PyLong_SHIFT) != prev) {
            PyErr_SetString(PyExc_OverflowError,
                "Python int too large to convert to C size_t");
            return (size_t) -1;
        }
    }
    return x;
}

/* Get a C unsigned long int from an int object, ignoring the high bits.
   Returns -1 and sets an error condition if an error occurs. */

static unsigned long
_PyLong_AsUnsignedLongMask(PyObject *vv)
{
    PyLongObject *v;
    unsigned long x;
    Py_ssize_t i;
    int sign;

    if (vv == NULL || !PyLong_Check(vv)) {
        PyErr_BadInternalCall();
        return (unsigned long) -1;
    }
    v = (PyLongObject *)vv;
    i = Py_SIZE(v);
    switch (i) {
    case 0: return 0;
    case 1: return v->ob_digit[0];
    }
    sign = 1;
    x = 0;
    if (i < 0) {
        sign = -1;
        i = -i;
    }
    while (--i >= 0) {
        x = (x << PyLong_SHIFT) | v->ob_digit[i];
    }
    return x * sign;
}

unsigned long
PyLong_AsUnsignedLongMask(PyObject *op)
{
    PyLongObject *lo;
    unsigned long val;

    if (op == NULL) {
        PyErr_BadInternalCall();
        return (unsigned long)-1;
    }

    if (PyLong_Check(op)) {
        return _PyLong_AsUnsignedLongMask(op);
    }

    lo = _PyLong_FromNbInt(op);
    if (lo == NULL)
        return (unsigned long)-1;

    val = _PyLong_AsUnsignedLongMask((PyObject *)lo);
    Py_DECREF(lo);
    return val;
}

int
_PyLong_Sign(PyObject *vv)
{
    PyLongObject *v = (PyLongObject *)vv;

    assert(v != NULL);
    assert(PyLong_Check(v));

    return Py_SIZE(v) == 0 ? 0 : (Py_SIZE(v) < 0 ? -1 : 1);
}

/* bits_in_digit(d) returns the unique integer k such that 2**(k-1) <= d <
   2**k if d is nonzero, else 0. */

static const unsigned char BitLengthTable[32] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

static int
bits_in_digit(digit d)
{
    int d_bits = 0;
    while (d >= 32) {
        d_bits += 6;
        d >>= 6;
    }
    d_bits += (int)BitLengthTable[d];
    return d_bits;
}

size_t
_PyLong_NumBits(PyObject *vv)
{
    PyLongObject *v = (PyLongObject *)vv;
    size_t result = 0;
    Py_ssize_t ndigits;
    int msd_bits;

    assert(v != NULL);
    assert(PyLong_Check(v));
    ndigits = Py_ABS(Py_SIZE(v));
    assert(ndigits == 0 || v->ob_digit[ndigits - 1] != 0);
    if (ndigits > 0) {
        digit msd = v->ob_digit[ndigits - 1];
        if ((size_t)(ndigits - 1) > SIZE_MAX / (size_t)PyLong_SHIFT)
            goto Overflow;
        result = (size_t)(ndigits - 1) * (size_t)PyLong_SHIFT;
        msd_bits = bits_in_digit(msd);
        if (SIZE_MAX - msd_bits < result)
            goto Overflow;
        result += msd_bits;
    }
    return result;

  Overflow:
    PyErr_SetString(PyExc_OverflowError, "int has too many bits "
                    "to express in a platform size_t");
    return (size_t)-1;
}

PyObject *
_PyLong_FromByteArray(const unsigned char* bytes, size_t n,
                      int little_endian, int is_signed)
{
    const unsigned char* pstartbyte;    /* LSB of bytes */
    int incr;                           /* direction to move pstartbyte */
    const unsigned char* pendbyte;      /* MSB of bytes */
    size_t numsignificantbytes;         /* number of bytes that matter */
    Py_ssize_t ndigits;                 /* number of Python int digits */
    PyLongObject* v;                    /* result */
    Py_ssize_t idigit = 0;              /* next free index in v->ob_digit */

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
       significant byte.  Leading 0 bytes are insignificant if the number
       is positive, and leading 0xff bytes if negative. */
    {
        size_t i;
        const unsigned char* p = pendbyte;
        const int pincr = -incr;  /* search MSB to LSB */
        const unsigned char insignificant = is_signed ? 0xff : 0x00;

        for (i = 0; i < n; ++i, p += pincr) {
            if (*p != insignificant)
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

    /* How many Python int digits do we need?  We have
       8*numsignificantbytes bits, and each Python int digit has
       PyLong_SHIFT bits, so it's the ceiling of the quotient. */
    /* catch overflow before it happens */
    if (numsignificantbytes > (PY_SSIZE_T_MAX - PyLong_SHIFT) / 8) {
        PyErr_SetString(PyExc_OverflowError,
                        "byte array too long to convert to int");
        return NULL;
    }
    ndigits = (numsignificantbytes * 8 + PyLong_SHIFT - 1) / PyLong_SHIFT;
    v = _PyLong_New(ndigits);
    if (v == NULL)
        return NULL;

    /* Copy the bits over.  The tricky parts are computing 2's-comp on
       the fly for signed numbers, and dealing with the mismatch between
       8-bit bytes and (probably) 15-bit Python digits.*/
    {
        size_t i;
        twodigits carry = 1;                    /* for 2's-comp calculation */
        twodigits accum = 0;                    /* sliding register */
        unsigned int accumbits = 0;             /* number of bits in accum */
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
            if (accumbits >= PyLong_SHIFT) {
                /* There's enough to fill a Python digit. */
                assert(idigit < ndigits);
                v->ob_digit[idigit] = (digit)(accum & PyLong_MASK);
                ++idigit;
                accum >>= PyLong_SHIFT;
                accumbits -= PyLong_SHIFT;
                assert(accumbits < PyLong_SHIFT);
            }
        }
        assert(accumbits < PyLong_SHIFT);
        if (accumbits) {
            assert(idigit < ndigits);
            v->ob_digit[idigit] = (digit)accum;
            ++idigit;
        }
    }

    Py_SIZE(v) = is_signed ? -idigit : idigit;
    return (PyObject *)long_normalize(v);
}

int
_PyLong_AsByteArray(PyLongObject* v,
                    unsigned char* bytes, size_t n,
                    int little_endian, int is_signed)
{
    Py_ssize_t i;               /* index into v->ob_digit */
    Py_ssize_t ndigits;         /* |v->ob_size| */
    twodigits accum;            /* sliding register */
    unsigned int accumbits;     /* # bits in accum */
    int do_twos_comp;           /* store 2's-comp?  is_signed and v < 0 */
    digit carry;                /* for computing 2's-comp */
    size_t j;                   /* # bytes filled */
    unsigned char* p;           /* pointer to next byte in bytes */
    int pincr;                  /* direction to move p */

    assert(v != NULL && PyLong_Check(v));

    if (Py_SIZE(v) < 0) {
        ndigits = -(Py_SIZE(v));
        if (!is_signed) {
            PyErr_SetString(PyExc_OverflowError,
                            "can't convert negative int to unsigned");
            return -1;
        }
        do_twos_comp = 1;
    }
    else {
        ndigits = Py_SIZE(v);
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
       exactly PyLong_SHIFT bits to the total, so first assert that the int is
       normalized. */
    assert(ndigits == 0 || v->ob_digit[ndigits - 1] != 0);
    j = 0;
    accum = 0;
    accumbits = 0;
    carry = do_twos_comp ? 1 : 0;
    for (i = 0; i < ndigits; ++i) {
        digit thisdigit = v->ob_digit[i];
        if (do_twos_comp) {
            thisdigit = (thisdigit ^ PyLong_MASK) + carry;
            carry = thisdigit >> PyLong_SHIFT;
            thisdigit &= PyLong_MASK;
        }
        /* Because we're going LSB to MSB, thisdigit is more
           significant than what's already in accum, so needs to be
           prepended to accum. */
        accum |= (twodigits)thisdigit << accumbits;

        /* The most-significant digit may be (probably is) at least
           partly empty. */
        if (i == ndigits - 1) {
            /* Count # of sign bits -- they needn't be stored,
             * although for signed conversion we need later to
             * make sure at least one sign bit gets stored. */
            digit s = do_twos_comp ? thisdigit ^ PyLong_MASK : thisdigit;
            while (s != 0) {
                s >>= 1;
                accumbits++;
            }
        }
        else
            accumbits += PyLong_SHIFT;

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
               (appropriately pretending that the int had an
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
    PyErr_SetString(PyExc_OverflowError, "int too big to convert");
    return -1;

}

/* Create a new int object from a C pointer */

PyObject *
PyLong_FromVoidPtr(void *p)
{
#if SIZEOF_VOID_P <= SIZEOF_LONG
    return PyLong_FromUnsignedLong((unsigned long)(uintptr_t)p);
#else

#if SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_FromVoidPtr: sizeof(long long) < sizeof(void*)"
#endif
    return PyLong_FromUnsignedLongLong((unsigned long long)(uintptr_t)p);
#endif /* SIZEOF_VOID_P <= SIZEOF_LONG */

}

/* Get a C pointer from an int object. */

void *
PyLong_AsVoidPtr(PyObject *vv)
{
#if SIZEOF_VOID_P <= SIZEOF_LONG
    long x;

    if (PyLong_Check(vv) && _PyLong_Sign(vv) < 0)
        x = PyLong_AsLong(vv);
    else
        x = PyLong_AsUnsignedLong(vv);
#else

#if SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_AsVoidPtr: sizeof(long long) < sizeof(void*)"
#endif
    long long x;

    if (PyLong_Check(vv) && _PyLong_Sign(vv) < 0)
        x = PyLong_AsLongLong(vv);
    else
        x = PyLong_AsUnsignedLongLong(vv);

#endif /* SIZEOF_VOID_P <= SIZEOF_LONG */

    if (x == -1 && PyErr_Occurred())
        return NULL;
    return (void *)x;
}

/* Initial long long support by Chris Herborth (chrish@qnx.com), later
 * rewritten to use the newer PyLong_{As,From}ByteArray API.
 */

#define PY_ABS_LLONG_MIN (0-(unsigned long long)PY_LLONG_MIN)

/* Create a new int object from a C long long int. */

PyObject *
PyLong_FromLongLong(long long ival)
{
    PyLongObject *v;
    unsigned long long abs_ival;
    unsigned long long t;  /* unsigned so >> doesn't propagate sign bit */
    int ndigits = 0;
    int negative = 0;

    CHECK_SMALL_INT(ival);
    if (ival < 0) {
        /* avoid signed overflow on negation;  see comments
           in PyLong_FromLong above. */
        abs_ival = (unsigned long long)(-1-ival) + 1;
        negative = 1;
    }
    else {
        abs_ival = (unsigned long long)ival;
    }

    /* Count the number of Python digits.
       We used to pick 5 ("big enough for anything"), but that's a
       waste of time and space given that 5*15 = 75 bits are rarely
       needed. */
    t = abs_ival;
    while (t) {
        ++ndigits;
        t >>= PyLong_SHIFT;
    }
    v = _PyLong_New(ndigits);
    if (v != NULL) {
        digit *p = v->ob_digit;
        Py_SIZE(v) = negative ? -ndigits : ndigits;
        t = abs_ival;
        while (t) {
            *p++ = (digit)(t & PyLong_MASK);
            t >>= PyLong_SHIFT;
        }
    }
    return (PyObject *)v;
}

/* Create a new int object from a C unsigned long long int. */

PyObject *
PyLong_FromUnsignedLongLong(unsigned long long ival)
{
    PyLongObject *v;
    unsigned long long t;
    int ndigits = 0;

    if (ival < PyLong_BASE)
        return PyLong_FromLong((long)ival);
    /* Count the number of Python digits. */
    t = ival;
    while (t) {
        ++ndigits;
        t >>= PyLong_SHIFT;
    }
    v = _PyLong_New(ndigits);
    if (v != NULL) {
        digit *p = v->ob_digit;
        while (ival) {
            *p++ = (digit)(ival & PyLong_MASK);
            ival >>= PyLong_SHIFT;
        }
    }
    return (PyObject *)v;
}

/* Create a new int object from a C Py_ssize_t. */

PyObject *
PyLong_FromSsize_t(Py_ssize_t ival)
{
    PyLongObject *v;
    size_t abs_ival;
    size_t t;  /* unsigned so >> doesn't propagate sign bit */
    int ndigits = 0;
    int negative = 0;

    CHECK_SMALL_INT(ival);
    if (ival < 0) {
        /* avoid signed overflow when ival = SIZE_T_MIN */
        abs_ival = (size_t)(-1-ival)+1;
        negative = 1;
    }
    else {
        abs_ival = (size_t)ival;
    }

    /* Count the number of Python digits. */
    t = abs_ival;
    while (t) {
        ++ndigits;
        t >>= PyLong_SHIFT;
    }
    v = _PyLong_New(ndigits);
    if (v != NULL) {
        digit *p = v->ob_digit;
        Py_SIZE(v) = negative ? -ndigits : ndigits;
        t = abs_ival;
        while (t) {
            *p++ = (digit)(t & PyLong_MASK);
            t >>= PyLong_SHIFT;
        }
    }
    return (PyObject *)v;
}

/* Create a new int object from a C size_t. */

PyObject *
PyLong_FromSize_t(size_t ival)
{
    PyLongObject *v;
    size_t t;
    int ndigits = 0;

    if (ival < PyLong_BASE)
        return PyLong_FromLong((long)ival);
    /* Count the number of Python digits. */
    t = ival;
    while (t) {
        ++ndigits;
        t >>= PyLong_SHIFT;
    }
    v = _PyLong_New(ndigits);
    if (v != NULL) {
        digit *p = v->ob_digit;
        Py_SIZE(v) = ndigits;
        while (ival) {
            *p++ = (digit)(ival & PyLong_MASK);
            ival >>= PyLong_SHIFT;
        }
    }
    return (PyObject *)v;
}

/* Get a C long long int from an int object or any object that has an
   __int__ method.  Return -1 and set an error if overflow occurs. */

long long
PyLong_AsLongLong(PyObject *vv)
{
    PyLongObject *v;
    long long bytes;
    int res;
    int do_decref = 0; /* if nb_int was called */

    if (vv == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }

    if (PyLong_Check(vv)) {
        v = (PyLongObject *)vv;
    }
    else {
        v = _PyLong_FromNbInt(vv);
        if (v == NULL)
            return -1;
        do_decref = 1;
    }

    res = 0;
    switch(Py_SIZE(v)) {
    case -1:
        bytes = -(sdigit)v->ob_digit[0];
        break;
    case 0:
        bytes = 0;
        break;
    case 1:
        bytes = v->ob_digit[0];
        break;
    default:
        res = _PyLong_AsByteArray((PyLongObject *)v, (unsigned char *)&bytes,
                                  SIZEOF_LONG_LONG, PY_LITTLE_ENDIAN, 1);
    }
    if (do_decref) {
        Py_DECREF(v);
    }

    /* Plan 9 can't handle long long in ? : expressions */
    if (res < 0)
        return (long long)-1;
    else
        return bytes;
}

/* Get a C unsigned long long int from an int object.
   Return -1 and set an error if overflow occurs. */

unsigned long long
PyLong_AsUnsignedLongLong(PyObject *vv)
{
    PyLongObject *v;
    unsigned long long bytes;
    int res;

    if (vv == NULL) {
        PyErr_BadInternalCall();
        return (unsigned long long)-1;
    }
    if (!PyLong_Check(vv)) {
        PyErr_SetString(PyExc_TypeError, "an integer is required");
        return (unsigned long long)-1;
    }

    v = (PyLongObject*)vv;
    switch(Py_SIZE(v)) {
    case 0: return 0;
    case 1: return v->ob_digit[0];
    }

    res = _PyLong_AsByteArray((PyLongObject *)vv, (unsigned char *)&bytes,
                              SIZEOF_LONG_LONG, PY_LITTLE_ENDIAN, 0);

    /* Plan 9 can't handle long long in ? : expressions */
    if (res < 0)
        return (unsigned long long)res;
    else
        return bytes;
}

/* Get a C unsigned long int from an int object, ignoring the high bits.
   Returns -1 and sets an error condition if an error occurs. */

static unsigned long long
_PyLong_AsUnsignedLongLongMask(PyObject *vv)
{
    PyLongObject *v;
    unsigned long long x;
    Py_ssize_t i;
    int sign;

    if (vv == NULL || !PyLong_Check(vv)) {
        PyErr_BadInternalCall();
        return (unsigned long long) -1;
    }
    v = (PyLongObject *)vv;
    switch(Py_SIZE(v)) {
    case 0: return 0;
    case 1: return v->ob_digit[0];
    }
    i = Py_SIZE(v);
    sign = 1;
    x = 0;
    if (i < 0) {
        sign = -1;
        i = -i;
    }
    while (--i >= 0) {
        x = (x << PyLong_SHIFT) | v->ob_digit[i];
    }
    return x * sign;
}

unsigned long long
PyLong_AsUnsignedLongLongMask(PyObject *op)
{
    PyLongObject *lo;
    unsigned long long val;

    if (op == NULL) {
        PyErr_BadInternalCall();
        return (unsigned long long)-1;
    }

    if (PyLong_Check(op)) {
        return _PyLong_AsUnsignedLongLongMask(op);
    }

    lo = _PyLong_FromNbInt(op);
    if (lo == NULL)
        return (unsigned long long)-1;

    val = _PyLong_AsUnsignedLongLongMask((PyObject *)lo);
    Py_DECREF(lo);
    return val;
}

/* Get a C long long int from an int object or any object that has an
   __int__ method.

   On overflow, return -1 and set *overflow to 1 or -1 depending on the sign of
   the result.  Otherwise *overflow is 0.

   For other errors (e.g., TypeError), return -1 and set an error condition.
   In this case *overflow will be 0.
*/

long long
PyLong_AsLongLongAndOverflow(PyObject *vv, int *overflow)
{
    /* This version by Tim Peters */
    PyLongObject *v;
    unsigned long long x, prev;
    long long res;
    Py_ssize_t i;
    int sign;
    int do_decref = 0; /* if nb_int was called */

    *overflow = 0;
    if (vv == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }

    if (PyLong_Check(vv)) {
        v = (PyLongObject *)vv;
    }
    else {
        v = _PyLong_FromNbInt(vv);
        if (v == NULL)
            return -1;
        do_decref = 1;
    }

    res = -1;
    i = Py_SIZE(v);

    switch (i) {
    case -1:
        res = -(sdigit)v->ob_digit[0];
        break;
    case 0:
        res = 0;
        break;
    case 1:
        res = v->ob_digit[0];
        break;
    default:
        sign = 1;
        x = 0;
        if (i < 0) {
            sign = -1;
            i = -(i);
        }
        while (--i >= 0) {
            prev = x;
            x = (x << PyLong_SHIFT) + v->ob_digit[i];
            if ((x >> PyLong_SHIFT) != prev) {
                *overflow = sign;
                goto exit;
            }
        }
        /* Haven't lost any bits, but casting to long requires extra
         * care (see comment above).
         */
        if (x <= (unsigned long long)PY_LLONG_MAX) {
            res = (long long)x * sign;
        }
        else if (sign < 0 && x == PY_ABS_LLONG_MIN) {
            res = PY_LLONG_MIN;
        }
        else {
            *overflow = sign;
            /* res is already set to -1 */
        }
    }
  exit:
    if (do_decref) {
        Py_DECREF(v);
    }
    return res;
}

#define CHECK_BINOP(v,w)                                \
    do {                                                \
        if (!PyLong_Check(v) || !PyLong_Check(w))       \
            Py_RETURN_NOTIMPLEMENTED;                   \
    } while(0)

/* x[0:m] and y[0:n] are digit vectors, LSD first, m >= n required.  x[0:n]
 * is modified in place, by adding y to it.  Carries are propagated as far as
 * x[m-1], and the remaining carry (0 or 1) is returned.
 */
static digit
v_iadd(digit *x, Py_ssize_t m, digit *y, Py_ssize_t n)
{
    Py_ssize_t i;
    digit carry = 0;

    assert(m >= n);
    for (i = 0; i < n; ++i) {
        carry += x[i] + y[i];
        x[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
        assert((carry & 1) == carry);
    }
    for (; carry && i < m; ++i) {
        carry += x[i];
        x[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
        assert((carry & 1) == carry);
    }
    return carry;
}

/* x[0:m] and y[0:n] are digit vectors, LSD first, m >= n required.  x[0:n]
 * is modified in place, by subtracting y from it.  Borrows are propagated as
 * far as x[m-1], and the remaining borrow (0 or 1) is returned.
 */
static digit
v_isub(digit *x, Py_ssize_t m, digit *y, Py_ssize_t n)
{
    Py_ssize_t i;
    digit borrow = 0;

    assert(m >= n);
    for (i = 0; i < n; ++i) {
        borrow = x[i] - y[i] - borrow;
        x[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1;            /* keep only 1 sign bit */
    }
    for (; borrow && i < m; ++i) {
        borrow = x[i] - borrow;
        x[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1;
    }
    return borrow;
}

/* Shift digit vector a[0:m] d bits left, with 0 <= d < PyLong_SHIFT.  Put
 * result in z[0:m], and return the d bits shifted out of the top.
 */
static digit
v_lshift(digit *z, digit *a, Py_ssize_t m, int d)
{
    Py_ssize_t i;
    digit carry = 0;

    assert(0 <= d && d < PyLong_SHIFT);
    for (i=0; i < m; i++) {
        twodigits acc = (twodigits)a[i] << d | carry;
        z[i] = (digit)acc & PyLong_MASK;
        carry = (digit)(acc >> PyLong_SHIFT);
    }
    return carry;
}

/* Shift digit vector a[0:m] d bits right, with 0 <= d < PyLong_SHIFT.  Put
 * result in z[0:m], and return the d bits shifted out of the bottom.
 */
static digit
v_rshift(digit *z, digit *a, Py_ssize_t m, int d)
{
    Py_ssize_t i;
    digit carry = 0;
    digit mask = ((digit)1 << d) - 1U;

    assert(0 <= d && d < PyLong_SHIFT);
    for (i=m; i-- > 0;) {
        twodigits acc = (twodigits)carry << PyLong_SHIFT | a[i];
        carry = (digit)acc & mask;
        z[i] = (digit)(acc >> d);
    }
    return carry;
}

/* Divide long pin, w/ size digits, by non-zero digit n, storing quotient
   in pout, and returning the remainder.  pin and pout point at the LSD.
   It's OK for pin == pout on entry, which saves oodles of mallocs/frees in
   _PyLong_Format, but that should be done with great care since ints are
   immutable. */

static digit
inplace_divrem1(digit *pout, digit *pin, Py_ssize_t size, digit n)
{
    twodigits rem = 0;

    assert(n > 0 && n <= PyLong_MASK);
    pin += size;
    pout += size;
    while (--size >= 0) {
        digit hi;
        rem = (rem << PyLong_SHIFT) | *--pin;
        *--pout = hi = (digit)(rem / n);
        rem -= (twodigits)hi * n;
    }
    return (digit)rem;
}

/* Divide an integer by a digit, returning both the quotient
   (as function result) and the remainder (through *prem).
   The sign of a is ignored; n should not be zero. */

static PyLongObject *
divrem1(PyLongObject *a, digit n, digit *prem)
{
    const Py_ssize_t size = Py_ABS(Py_SIZE(a));
    PyLongObject *z;

    assert(n > 0 && n <= PyLong_MASK);
    z = _PyLong_New(size);
    if (z == NULL)
        return NULL;
    *prem = inplace_divrem1(z->ob_digit, a->ob_digit, size, n);
    return long_normalize(z);
}

/* Convert an integer to a base 10 string.  Returns a new non-shared
   string.  (Return value is non-shared so that callers can modify the
   returned value if necessary.) */

static int
long_to_decimal_string_internal(PyObject *aa,
                                PyObject **p_output,
                                _PyUnicodeWriter *writer,
                                _PyBytesWriter *bytes_writer,
                                char **bytes_str)
{
    PyLongObject *scratch, *a;
    PyObject *str = NULL;
    Py_ssize_t size, strlen, size_a, i, j;
    digit *pout, *pin, rem, tenpow;
    int negative;
    int d;
    enum PyUnicode_Kind kind;

    a = (PyLongObject *)aa;
    if (a == NULL || !PyLong_Check(a)) {
        PyErr_BadInternalCall();
        return -1;
    }
    size_a = Py_ABS(Py_SIZE(a));
    negative = Py_SIZE(a) < 0;

    /* quick and dirty upper bound for the number of digits
       required to express a in base _PyLong_DECIMAL_BASE:

         #digits = 1 + floor(log2(a) / log2(_PyLong_DECIMAL_BASE))

       But log2(a) < size_a * PyLong_SHIFT, and
       log2(_PyLong_DECIMAL_BASE) = log2(10) * _PyLong_DECIMAL_SHIFT
                                  > 3.3 * _PyLong_DECIMAL_SHIFT

         size_a * PyLong_SHIFT / (3.3 * _PyLong_DECIMAL_SHIFT) =
             size_a + size_a / d < size_a + size_a / floor(d),
       where d = (3.3 * _PyLong_DECIMAL_SHIFT) /
                 (PyLong_SHIFT - 3.3 * _PyLong_DECIMAL_SHIFT)
    */
    d = (33 * _PyLong_DECIMAL_SHIFT) /
        (10 * PyLong_SHIFT - 33 * _PyLong_DECIMAL_SHIFT);
    assert(size_a < PY_SSIZE_T_MAX/2);
    size = 1 + size_a + size_a / d;
    scratch = _PyLong_New(size);
    if (scratch == NULL)
        return -1;

    /* convert array of base _PyLong_BASE digits in pin to an array of
       base _PyLong_DECIMAL_BASE digits in pout, following Knuth (TAOCP,
       Volume 2 (3rd edn), section 4.4, Method 1b). */
    pin = a->ob_digit;
    pout = scratch->ob_digit;
    size = 0;
    for (i = size_a; --i >= 0; ) {
        digit hi = pin[i];
        for (j = 0; j < size; j++) {
            twodigits z = (twodigits)pout[j] << PyLong_SHIFT | hi;
            hi = (digit)(z / _PyLong_DECIMAL_BASE);
            pout[j] = (digit)(z - (twodigits)hi *
                              _PyLong_DECIMAL_BASE);
        }
        while (hi) {
            pout[size++] = hi % _PyLong_DECIMAL_BASE;
            hi /= _PyLong_DECIMAL_BASE;
        }
        /* check for keyboard interrupt */
        SIGCHECK({
                Py_DECREF(scratch);
                return -1;
            });
    }
    /* pout should have at least one digit, so that the case when a = 0
       works correctly */
    if (size == 0)
        pout[size++] = 0;

    /* calculate exact length of output string, and allocate */
    strlen = negative + 1 + (size - 1) * _PyLong_DECIMAL_SHIFT;
    tenpow = 10;
    rem = pout[size-1];
    while (rem >= tenpow) {
        tenpow *= 10;
        strlen++;
    }
    if (writer) {
        if (_PyUnicodeWriter_Prepare(writer, strlen, '9') == -1) {
            Py_DECREF(scratch);
            return -1;
        }
        kind = writer->kind;
    }
    else if (bytes_writer) {
        *bytes_str = _PyBytesWriter_Prepare(bytes_writer, *bytes_str, strlen);
        if (*bytes_str == NULL) {
            Py_DECREF(scratch);
            return -1;
        }
    }
    else {
        str = PyUnicode_New(strlen, '9');
        if (str == NULL) {
            Py_DECREF(scratch);
            return -1;
        }
        kind = PyUnicode_KIND(str);
    }

#define WRITE_DIGITS(p)                                               \
    do {                                                              \
        /* pout[0] through pout[size-2] contribute exactly            \
           _PyLong_DECIMAL_SHIFT digits each */                       \
        for (i=0; i < size - 1; i++) {                                \
            rem = pout[i];                                            \
            for (j = 0; j < _PyLong_DECIMAL_SHIFT; j++) {             \
                *--p = '0' + rem % 10;                                \
                rem /= 10;                                            \
            }                                                         \
        }                                                             \
        /* pout[size-1]: always produce at least one decimal digit */ \
        rem = pout[i];                                                \
        do {                                                          \
            *--p = '0' + rem % 10;                                    \
            rem /= 10;                                                \
        } while (rem != 0);                                           \
                                                                      \
        /* and sign */                                                \
        if (negative)                                                 \
            *--p = '-';                                               \
    } while (0)

#define WRITE_UNICODE_DIGITS(TYPE)                                    \
    do {                                                              \
        if (writer)                                                   \
            p = (TYPE*)PyUnicode_DATA(writer->buffer) + writer->pos + strlen; \
        else                                                          \
            p = (TYPE*)PyUnicode_DATA(str) + strlen;                  \
                                                                      \
        WRITE_DIGITS(p);                                              \
                                                                      \
        /* check we've counted correctly */                           \
        if (writer)                                                   \
            assert(p == ((TYPE*)PyUnicode_DATA(writer->buffer) + writer->pos)); \
        else                                                          \
            assert(p == (TYPE*)PyUnicode_DATA(str));                  \
    } while (0)

    /* fill the string right-to-left */
    if (bytes_writer) {
        char *p = *bytes_str + strlen;
        WRITE_DIGITS(p);
        assert(p == *bytes_str);
    }
    else if (kind == PyUnicode_1BYTE_KIND) {
        Py_UCS1 *p;
        WRITE_UNICODE_DIGITS(Py_UCS1);
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        Py_UCS2 *p;
        WRITE_UNICODE_DIGITS(Py_UCS2);
    }
    else {
        Py_UCS4 *p;
        assert (kind == PyUnicode_4BYTE_KIND);
        WRITE_UNICODE_DIGITS(Py_UCS4);
    }
#undef WRITE_DIGITS
#undef WRITE_UNICODE_DIGITS

    Py_DECREF(scratch);
    if (writer) {
        writer->pos += strlen;
    }
    else if (bytes_writer) {
        (*bytes_str) += strlen;
    }
    else {
        assert(_PyUnicode_CheckConsistency(str, 1));
        *p_output = (PyObject *)str;
    }
    return 0;
}

static PyObject *
long_to_decimal_string(PyObject *aa)
{
    PyObject *v;
    if (long_to_decimal_string_internal(aa, &v, NULL, NULL, NULL) == -1)
        return NULL;
    return v;
}

/* Convert an int object to a string, using a given conversion base,
   which should be one of 2, 8 or 16.  Return a string object.
   If base is 2, 8 or 16, add the proper prefix '0b', '0o' or '0x'
   if alternate is nonzero. */

static int
long_format_binary(PyObject *aa, int base, int alternate,
                   PyObject **p_output, _PyUnicodeWriter *writer,
                   _PyBytesWriter *bytes_writer, char **bytes_str)
{
    PyLongObject *a = (PyLongObject *)aa;
    PyObject *v = NULL;
    Py_ssize_t sz;
    Py_ssize_t size_a;
    enum PyUnicode_Kind kind;
    int negative;
    int bits;

    assert(base == 2 || base == 8 || base == 16);
    if (a == NULL || !PyLong_Check(a)) {
        PyErr_BadInternalCall();
        return -1;
    }
    size_a = Py_ABS(Py_SIZE(a));
    negative = Py_SIZE(a) < 0;

    /* Compute a rough upper bound for the length of the string */
    switch (base) {
    case 16:
        bits = 4;
        break;
    case 8:
        bits = 3;
        break;
    case 2:
        bits = 1;
        break;
    default:
        Py_UNREACHABLE();
    }

    /* Compute exact length 'sz' of output string. */
    if (size_a == 0) {
        sz = 1;
    }
    else {
        Py_ssize_t size_a_in_bits;
        /* Ensure overflow doesn't occur during computation of sz. */
        if (size_a > (PY_SSIZE_T_MAX - 3) / PyLong_SHIFT) {
            PyErr_SetString(PyExc_OverflowError,
                            "int too large to format");
            return -1;
        }
        size_a_in_bits = (size_a - 1) * PyLong_SHIFT +
                         bits_in_digit(a->ob_digit[size_a - 1]);
        /* Allow 1 character for a '-' sign. */
        sz = negative + (size_a_in_bits + (bits - 1)) / bits;
    }
    if (alternate) {
        /* 2 characters for prefix  */
        sz += 2;
    }

    if (writer) {
        if (_PyUnicodeWriter_Prepare(writer, sz, 'x') == -1)
            return -1;
        kind = writer->kind;
    }
    else if (bytes_writer) {
        *bytes_str = _PyBytesWriter_Prepare(bytes_writer, *bytes_str, sz);
        if (*bytes_str == NULL)
            return -1;
    }
    else {
        v = PyUnicode_New(sz, 'x');
        if (v == NULL)
            return -1;
        kind = PyUnicode_KIND(v);
    }

#define WRITE_DIGITS(p)                                                 \
    do {                                                                \
        if (size_a == 0) {                                              \
            *--p = '0';                                                 \
        }                                                               \
        else {                                                          \
            /* JRH: special case for power-of-2 bases */                \
            twodigits accum = 0;                                        \
            int accumbits = 0;   /* # of bits in accum */               \
            Py_ssize_t i;                                               \
            for (i = 0; i < size_a; ++i) {                              \
                accum |= (twodigits)a->ob_digit[i] << accumbits;        \
                accumbits += PyLong_SHIFT;                              \
                assert(accumbits >= bits);                              \
                do {                                                    \
                    char cdigit;                                        \
                    cdigit = (char)(accum & (base - 1));                \
                    cdigit += (cdigit < 10) ? '0' : 'a'-10;             \
                    *--p = cdigit;                                      \
                    accumbits -= bits;                                  \
                    accum >>= bits;                                     \
                } while (i < size_a-1 ? accumbits >= bits : accum > 0); \
            }                                                           \
        }                                                               \
                                                                        \
        if (alternate) {                                                \
            if (base == 16)                                             \
                *--p = 'x';                                             \
            else if (base == 8)                                         \
                *--p = 'o';                                             \
            else /* (base == 2) */                                      \
                *--p = 'b';                                             \
            *--p = '0';                                                 \
        }                                                               \
        if (negative)                                                   \
            *--p = '-';                                                 \
    } while (0)

#define WRITE_UNICODE_DIGITS(TYPE)                                      \
    do {                                                                \
        if (writer)                                                     \
            p = (TYPE*)PyUnicode_DATA(writer->buffer) + writer->pos + sz; \
        else                                                            \
            p = (TYPE*)PyUnicode_DATA(v) + sz;                          \
                                                                        \
        WRITE_DIGITS(p);                                                \
                                                                        \
        if (writer)                                                     \
            assert(p == ((TYPE*)PyUnicode_DATA(writer->buffer) + writer->pos)); \
        else                                                            \
            assert(p == (TYPE*)PyUnicode_DATA(v));                      \
    } while (0)

    if (bytes_writer) {
        char *p = *bytes_str + sz;
        WRITE_DIGITS(p);
        assert(p == *bytes_str);
    }
    else if (kind == PyUnicode_1BYTE_KIND) {
        Py_UCS1 *p;
        WRITE_UNICODE_DIGITS(Py_UCS1);
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        Py_UCS2 *p;
        WRITE_UNICODE_DIGITS(Py_UCS2);
    }
    else {
        Py_UCS4 *p;
        assert (kind == PyUnicode_4BYTE_KIND);
        WRITE_UNICODE_DIGITS(Py_UCS4);
    }
#undef WRITE_DIGITS
#undef WRITE_UNICODE_DIGITS

    if (writer) {
        writer->pos += sz;
    }
    else if (bytes_writer) {
        (*bytes_str) += sz;
    }
    else {
        assert(_PyUnicode_CheckConsistency(v, 1));
        *p_output = v;
    }
    return 0;
}

PyObject *
_PyLong_Format(PyObject *obj, int base)
{
    PyObject *str;
    int err;
    if (base == 10)
        err = long_to_decimal_string_internal(obj, &str, NULL, NULL, NULL);
    else
        err = long_format_binary(obj, base, 1, &str, NULL, NULL, NULL);
    if (err == -1)
        return NULL;
    return str;
}

int
_PyLong_FormatWriter(_PyUnicodeWriter *writer,
                     PyObject *obj,
                     int base, int alternate)
{
    if (base == 10)
        return long_to_decimal_string_internal(obj, NULL, writer,
                                               NULL, NULL);
    else
        return long_format_binary(obj, base, alternate, NULL, writer,
                                  NULL, NULL);
}

char*
_PyLong_FormatBytesWriter(_PyBytesWriter *writer, char *str,
                          PyObject *obj,
                          int base, int alternate)
{
    char *str2;
    int res;
    str2 = str;
    if (base == 10)
        res = long_to_decimal_string_internal(obj, NULL, NULL,
                                              writer, &str2);
    else
        res = long_format_binary(obj, base, alternate, NULL, NULL,
                                 writer, &str2);
    if (res < 0)
        return NULL;
    assert(str2 != NULL);
    return str2;
}

/* Table of digit values for 8-bit string -> integer conversion.
 * '0' maps to 0, ..., '9' maps to 9.
 * 'a' and 'A' map to 10, ..., 'z' and 'Z' map to 35.
 * All other indices map to 37.
 * Note that when converting a base B string, a char c is a legitimate
 * base B digit iff _PyLong_DigitValue[Py_CHARPyLong_MASK(c)] < B.
 */
unsigned char _PyLong_DigitValue[256] = {
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  37, 37, 37, 37, 37, 37,
    37, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 37, 37, 37, 37,
    37, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
};

/* *str points to the first digit in a string of base `base` digits.  base
 * is a power of 2 (2, 4, 8, 16, or 32).  *str is set to point to the first
 * non-digit (which may be *str!).  A normalized int is returned.
 * The point to this routine is that it takes time linear in the number of
 * string characters.
 *
 * Return values:
 *   -1 on syntax error (exception needs to be set, *res is untouched)
 *   0 else (exception may be set, in that case *res is set to NULL)
 */
static int
long_from_binary_base(const char **str, int base, PyLongObject **res)
{
    const char *p = *str;
    const char *start = p;
    char prev = 0;
    Py_ssize_t digits = 0;
    int bits_per_char;
    Py_ssize_t n;
    PyLongObject *z;
    twodigits accum;
    int bits_in_accum;
    digit *pdigit;

    assert(base >= 2 && base <= 32 && (base & (base - 1)) == 0);
    n = base;
    for (bits_per_char = -1; n; ++bits_per_char) {
        n >>= 1;
    }
    /* count digits and set p to end-of-string */
    while (_PyLong_DigitValue[Py_CHARMASK(*p)] < base || *p == '_') {
        if (*p == '_') {
            if (prev == '_') {
                *str = p - 1;
                return -1;
            }
        } else {
            ++digits;
        }
        prev = *p;
        ++p;
    }
    if (prev == '_') {
        /* Trailing underscore not allowed. */
        *str = p - 1;
        return -1;
    }

    *str = p;
    /* n <- the number of Python digits needed,
            = ceiling((digits * bits_per_char) / PyLong_SHIFT). */
    if (digits > (PY_SSIZE_T_MAX - (PyLong_SHIFT - 1)) / bits_per_char) {
        PyErr_SetString(PyExc_ValueError,
                        "int string too large to convert");
        *res = NULL;
        return 0;
    }
    n = (digits * bits_per_char + PyLong_SHIFT - 1) / PyLong_SHIFT;
    z = _PyLong_New(n);
    if (z == NULL) {
        *res = NULL;
        return 0;
    }
    /* Read string from right, and fill in int from left; i.e.,
     * from least to most significant in both.
     */
    accum = 0;
    bits_in_accum = 0;
    pdigit = z->ob_digit;
    while (--p >= start) {
        int k;
        if (*p == '_') {
            continue;
        }
        k = (int)_PyLong_DigitValue[Py_CHARMASK(*p)];
        assert(k >= 0 && k < base);
        accum |= (twodigits)k << bits_in_accum;
        bits_in_accum += bits_per_char;
        if (bits_in_accum >= PyLong_SHIFT) {
            *pdigit++ = (digit)(accum & PyLong_MASK);
            assert(pdigit - z->ob_digit <= n);
            accum >>= PyLong_SHIFT;
            bits_in_accum -= PyLong_SHIFT;
            assert(bits_in_accum < PyLong_SHIFT);
        }
    }
    if (bits_in_accum) {
        assert(bits_in_accum <= PyLong_SHIFT);
        *pdigit++ = (digit)accum;
        assert(pdigit - z->ob_digit <= n);
    }
    while (pdigit - z->ob_digit < n)
        *pdigit++ = 0;
    *res = long_normalize(z);
    return 0;
}

/* Parses an int from a bytestring. Leading and trailing whitespace will be
 * ignored.
 *
 * If successful, a PyLong object will be returned and 'pend' will be pointing
 * to the first unused byte unless it's NULL.
 *
 * If unsuccessful, NULL will be returned.
 */
PyObject *
PyLong_FromString(const char *str, char **pend, int base)
{
    int sign = 1, error_if_nonzero = 0;
    const char *start, *orig_str = str;
    PyLongObject *z = NULL;
    PyObject *strobj;
    Py_ssize_t slen;

    if ((base != 0 && base < 2) || base > 36) {
        PyErr_SetString(PyExc_ValueError,
                        "int() arg 2 must be >= 2 and <= 36");
        return NULL;
    }
    while (*str != '\0' && Py_ISSPACE(Py_CHARMASK(*str))) {
        str++;
    }
    if (*str == '+') {
        ++str;
    }
    else if (*str == '-') {
        ++str;
        sign = -1;
    }
    if (base == 0) {
        if (str[0] != '0') {
            base = 10;
        }
        else if (str[1] == 'x' || str[1] == 'X') {
            base = 16;
        }
        else if (str[1] == 'o' || str[1] == 'O') {
            base = 8;
        }
        else if (str[1] == 'b' || str[1] == 'B') {
            base = 2;
        }
        else {
            /* "old" (C-style) octal literal, now invalid.
               it might still be zero though */
            error_if_nonzero = 1;
            base = 10;
        }
    }
    if (str[0] == '0' &&
        ((base == 16 && (str[1] == 'x' || str[1] == 'X')) ||
         (base == 8  && (str[1] == 'o' || str[1] == 'O')) ||
         (base == 2  && (str[1] == 'b' || str[1] == 'B')))) {
        str += 2;
        /* One underscore allowed here. */
        if (*str == '_') {
            ++str;
        }
    }
    if (str[0] == '_') {
        /* May not start with underscores. */
        goto onError;
    }

    start = str;
    if ((base & (base - 1)) == 0) {
        int res = long_from_binary_base(&str, base, &z);
        if (res < 0) {
            /* Syntax error. */
            goto onError;
        }
    }
    else {
/***
Binary bases can be converted in time linear in the number of digits, because
Python's representation base is binary.  Other bases (including decimal!) use
the simple quadratic-time algorithm below, complicated by some speed tricks.

First some math:  the largest integer that can be expressed in N base-B digits
is B**N-1.  Consequently, if we have an N-digit input in base B, the worst-
case number of Python digits needed to hold it is the smallest integer n s.t.

    BASE**n-1 >= B**N-1  [or, adding 1 to both sides]
    BASE**n >= B**N      [taking logs to base BASE]
    n >= log(B**N)/log(BASE) = N * log(B)/log(BASE)

The static array log_base_BASE[base] == log(base)/log(BASE) so we can compute
this quickly.  A Python int with that much space is reserved near the start,
and the result is computed into it.

The input string is actually treated as being in base base**i (i.e., i digits
are processed at a time), where two more static arrays hold:

    convwidth_base[base] = the largest integer i such that base**i <= BASE
    convmultmax_base[base] = base ** convwidth_base[base]

The first of these is the largest i such that i consecutive input digits
must fit in a single Python digit.  The second is effectively the input
base we're really using.

Viewing the input as a sequence <c0, c1, ..., c_n-1> of digits in base
convmultmax_base[base], the result is "simply"

   (((c0*B + c1)*B + c2)*B + c3)*B + ... ))) + c_n-1

where B = convmultmax_base[base].

Error analysis:  as above, the number of Python digits `n` needed is worst-
case

    n >= N * log(B)/log(BASE)

where `N` is the number of input digits in base `B`.  This is computed via

    size_z = (Py_ssize_t)((scan - str) * log_base_BASE[base]) + 1;

below.  Two numeric concerns are how much space this can waste, and whether
the computed result can be too small.  To be concrete, assume BASE = 2**15,
which is the default (and it's unlikely anyone changes that).

Waste isn't a problem:  provided the first input digit isn't 0, the difference
between the worst-case input with N digits and the smallest input with N
digits is about a factor of B, but B is small compared to BASE so at most
one allocated Python digit can remain unused on that count.  If
N*log(B)/log(BASE) is mathematically an exact integer, then truncating that
and adding 1 returns a result 1 larger than necessary.  However, that can't
happen:  whenever B is a power of 2, long_from_binary_base() is called
instead, and it's impossible for B**i to be an integer power of 2**15 when
B is not a power of 2 (i.e., it's impossible for N*log(B)/log(BASE) to be
an exact integer when B is not a power of 2, since B**i has a prime factor
other than 2 in that case, but (2**15)**j's only prime factor is 2).

The computed result can be too small if the true value of N*log(B)/log(BASE)
is a little bit larger than an exact integer, but due to roundoff errors (in
computing log(B), log(BASE), their quotient, and/or multiplying that by N)
yields a numeric result a little less than that integer.  Unfortunately, "how
close can a transcendental function get to an integer over some range?"
questions are generally theoretically intractable.  Computer analysis via
continued fractions is practical:  expand log(B)/log(BASE) via continued
fractions, giving a sequence i/j of "the best" rational approximations.  Then
j*log(B)/log(BASE) is approximately equal to (the integer) i.  This shows that
we can get very close to being in trouble, but very rarely.  For example,
76573 is a denominator in one of the continued-fraction approximations to
log(10)/log(2**15), and indeed:

    >>> log(10)/log(2**15)*76573
    16958.000000654003

is very close to an integer.  If we were working with IEEE single-precision,
rounding errors could kill us.  Finding worst cases in IEEE double-precision
requires better-than-double-precision log() functions, and Tim didn't bother.
Instead the code checks to see whether the allocated space is enough as each
new Python digit is added, and copies the whole thing to a larger int if not.
This should happen extremely rarely, and in fact I don't have a test case
that triggers it(!).  Instead the code was tested by artificially allocating
just 1 digit at the start, so that the copying code was exercised for every
digit beyond the first.
***/
        twodigits c;           /* current input character */
        Py_ssize_t size_z;
        Py_ssize_t digits = 0;
        int i;
        int convwidth;
        twodigits convmultmax, convmult;
        digit *pz, *pzstop;
        const char *scan, *lastdigit;
        char prev = 0;

        static double log_base_BASE[37] = {0.0e0,};
        static int convwidth_base[37] = {0,};
        static twodigits convmultmax_base[37] = {0,};

        if (log_base_BASE[base] == 0.0) {
            twodigits convmax = base;
            int i = 1;

            log_base_BASE[base] = (log((double)base) /
                                   log((double)PyLong_BASE));
            for (;;) {
                twodigits next = convmax * base;
                if (next > PyLong_BASE) {
                    break;
                }
                convmax = next;
                ++i;
            }
            convmultmax_base[base] = convmax;
            assert(i > 0);
            convwidth_base[base] = i;
        }

        /* Find length of the string of numeric characters. */
        scan = str;
        lastdigit = str;

        while (_PyLong_DigitValue[Py_CHARMASK(*scan)] < base || *scan == '_') {
            if (*scan == '_') {
                if (prev == '_') {
                    /* Only one underscore allowed. */
                    str = lastdigit + 1;
                    goto onError;
                }
            }
            else {
                ++digits;
                lastdigit = scan;
            }
            prev = *scan;
            ++scan;
        }
        if (prev == '_') {
            /* Trailing underscore not allowed. */
            /* Set error pointer to first underscore. */
            str = lastdigit + 1;
            goto onError;
        }

        /* Create an int object that can contain the largest possible
         * integer with this base and length.  Note that there's no
         * need to initialize z->ob_digit -- no slot is read up before
         * being stored into.
         */
        double fsize_z = (double)digits * log_base_BASE[base] + 1.0;
        if (fsize_z > (double)MAX_LONG_DIGITS) {
            /* The same exception as in _PyLong_New(). */
            PyErr_SetString(PyExc_OverflowError,
                            "too many digits in integer");
            return NULL;
        }
        size_z = (Py_ssize_t)fsize_z;
        /* Uncomment next line to test exceedingly rare copy code */
        /* size_z = 1; */
        assert(size_z > 0);
        z = _PyLong_New(size_z);
        if (z == NULL) {
            return NULL;
        }
        Py_SIZE(z) = 0;

        /* `convwidth` consecutive input digits are treated as a single
         * digit in base `convmultmax`.
         */
        convwidth = convwidth_base[base];
        convmultmax = convmultmax_base[base];

        /* Work ;-) */
        while (str < scan) {
            if (*str == '_') {
                str++;
                continue;
            }
            /* grab up to convwidth digits from the input string */
            c = (digit)_PyLong_DigitValue[Py_CHARMASK(*str++)];
            for (i = 1; i < convwidth && str != scan; ++str) {
                if (*str == '_') {
                    continue;
                }
                i++;
                c = (twodigits)(c *  base +
                                (int)_PyLong_DigitValue[Py_CHARMASK(*str)]);
                assert(c < PyLong_BASE);
            }

            convmult = convmultmax;
            /* Calculate the shift only if we couldn't get
             * convwidth digits.
             */
            if (i != convwidth) {
                convmult = base;
                for ( ; i > 1; --i) {
                    convmult *= base;
                }
            }

            /* Multiply z by convmult, and add c. */
            pz = z->ob_digit;
            pzstop = pz + Py_SIZE(z);
            for (; pz < pzstop; ++pz) {
                c += (twodigits)*pz * convmult;
                *pz = (digit)(c & PyLong_MASK);
                c >>= PyLong_SHIFT;
            }
            /* carry off the current end? */
            if (c) {
                assert(c < PyLong_BASE);
                if (Py_SIZE(z) < size_z) {
                    *pz = (digit)c;
                    ++Py_SIZE(z);
                }
                else {
                    PyLongObject *tmp;
                    /* Extremely rare.  Get more space. */
                    assert(Py_SIZE(z) == size_z);
                    tmp = _PyLong_New(size_z + 1);
                    if (tmp == NULL) {
                        Py_DECREF(z);
                        return NULL;
                    }
                    memcpy(tmp->ob_digit,
                           z->ob_digit,
                           sizeof(digit) * size_z);
                    Py_DECREF(z);
                    z = tmp;
                    z->ob_digit[size_z] = (digit)c;
                    ++size_z;
                }
            }
        }
    }
    if (z == NULL) {
        return NULL;
    }
    if (error_if_nonzero) {
        /* reset the base to 0, else the exception message
           doesn't make too much sense */
        base = 0;
        if (Py_SIZE(z) != 0) {
            goto onError;
        }
        /* there might still be other problems, therefore base
           remains zero here for the same reason */
    }
    if (str == start) {
        goto onError;
    }
    if (sign < 0) {
        Py_SIZE(z) = -(Py_SIZE(z));
    }
    while (*str && Py_ISSPACE(Py_CHARMASK(*str))) {
        str++;
    }
    if (*str != '\0') {
        goto onError;
    }
    long_normalize(z);
    z = maybe_small_long(z);
    if (z == NULL) {
        return NULL;
    }
    if (pend != NULL) {
        *pend = (char *)str;
    }
    return (PyObject *) z;

  onError:
    if (pend != NULL) {
        *pend = (char *)str;
    }
    Py_XDECREF(z);
    slen = strlen(orig_str) < 200 ? strlen(orig_str) : 200;
    strobj = PyUnicode_FromStringAndSize(orig_str, slen);
    if (strobj == NULL) {
        return NULL;
    }
    PyErr_Format(PyExc_ValueError,
                 "invalid literal for int() with base %d: %.200R",
                 base, strobj);
    Py_DECREF(strobj);
    return NULL;
}

/* Since PyLong_FromString doesn't have a length parameter,
 * check here for possible NULs in the string.
 *
 * Reports an invalid literal as a bytes object.
 */
PyObject *
_PyLong_FromBytes(const char *s, Py_ssize_t len, int base)
{
    PyObject *result, *strobj;
    char *end = NULL;

    result = PyLong_FromString(s, &end, base);
    if (end == NULL || (result != NULL && end == s + len))
        return result;
    Py_XDECREF(result);
    strobj = PyBytes_FromStringAndSize(s, Py_MIN(len, 200));
    if (strobj != NULL) {
        PyErr_Format(PyExc_ValueError,
                     "invalid literal for int() with base %d: %.200R",
                     base, strobj);
        Py_DECREF(strobj);
    }
    return NULL;
}

PyObject *
PyLong_FromUnicode(Py_UNICODE *u, Py_ssize_t length, int base)
{
    PyObject *v, *unicode = PyUnicode_FromWideChar(u, length);
    if (unicode == NULL)
        return NULL;
    v = PyLong_FromUnicodeObject(unicode, base);
    Py_DECREF(unicode);
    return v;
}

PyObject *
PyLong_FromUnicodeObject(PyObject *u, int base)
{
    PyObject *result, *asciidig;
    const char *buffer;
    char *end = NULL;
    Py_ssize_t buflen;

    asciidig = _PyUnicode_TransformDecimalAndSpaceToASCII(u);
    if (asciidig == NULL)
        return NULL;
    assert(PyUnicode_IS_ASCII(asciidig));
    /* Simply get a pointer to existing ASCII characters. */
    buffer = PyUnicode_AsUTF8AndSize(asciidig, &buflen);
    assert(buffer != NULL);

    result = PyLong_FromString(buffer, &end, base);
    if (end == NULL || (result != NULL && end == buffer + buflen)) {
        Py_DECREF(asciidig);
        return result;
    }
    Py_DECREF(asciidig);
    Py_XDECREF(result);
    PyErr_Format(PyExc_ValueError,
                 "invalid literal for int() with base %d: %.200R",
                 base, u);
    return NULL;
}

/* forward */
static PyLongObject *x_divrem
    (PyLongObject *, PyLongObject *, PyLongObject **);
static PyObject *long_long(PyObject *v);

/* Int division with remainder, top-level routine */

static int
long_divrem(PyLongObject *a, PyLongObject *b,
            PyLongObject **pdiv, PyLongObject **prem)
{
    Py_ssize_t size_a = Py_ABS(Py_SIZE(a)), size_b = Py_ABS(Py_SIZE(b));
    PyLongObject *z;

    if (size_b == 0) {
        PyErr_SetString(PyExc_ZeroDivisionError,
                        "integer division or modulo by zero");
        return -1;
    }
    if (size_a < size_b ||
        (size_a == size_b &&
         a->ob_digit[size_a-1] < b->ob_digit[size_b-1])) {
        /* |a| < |b|. */
        *prem = (PyLongObject *)long_long((PyObject *)a);
        if (*prem == NULL) {
            return -1;
        }
        Py_INCREF(_PyLong_Zero);
        *pdiv = (PyLongObject*)_PyLong_Zero;
        return 0;
    }
    if (size_b == 1) {
        digit rem = 0;
        z = divrem1(a, b->ob_digit[0], &rem);
        if (z == NULL)
            return -1;
        *prem = (PyLongObject *) PyLong_FromLong((long)rem);
        if (*prem == NULL) {
            Py_DECREF(z);
            return -1;
        }
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
    if ((Py_SIZE(a) < 0) != (Py_SIZE(b) < 0)) {
        _PyLong_Negate(&z);
        if (z == NULL) {
            Py_CLEAR(*prem);
            return -1;
        }
    }
    if (Py_SIZE(a) < 0 && Py_SIZE(*prem) != 0) {
        _PyLong_Negate(prem);
        if (*prem == NULL) {
            Py_DECREF(z);
            Py_CLEAR(*prem);
            return -1;
        }
    }
    *pdiv = maybe_small_long(z);
    return 0;
}

/* Unsigned int division with remainder -- the algorithm.  The arguments v1
   and w1 should satisfy 2 <= Py_ABS(Py_SIZE(w1)) <= Py_ABS(Py_SIZE(v1)). */

static PyLongObject *
x_divrem(PyLongObject *v1, PyLongObject *w1, PyLongObject **prem)
{
    PyLongObject *v, *w, *a;
    Py_ssize_t i, k, size_v, size_w;
    int d;
    digit wm1, wm2, carry, q, r, vtop, *v0, *vk, *w0, *ak;
    twodigits vv;
    sdigit zhi;
    stwodigits z;

    /* We follow Knuth [The Art of Computer Programming, Vol. 2 (3rd
       edn.), section 4.3.1, Algorithm D], except that we don't explicitly
       handle the special case when the initial estimate q for a quotient
       digit is >= PyLong_BASE: the max value for q is PyLong_BASE+1, and
       that won't overflow a digit. */

    /* allocate space; w will also be used to hold the final remainder */
    size_v = Py_ABS(Py_SIZE(v1));
    size_w = Py_ABS(Py_SIZE(w1));
    assert(size_v >= size_w && size_w >= 2); /* Assert checks by div() */
    v = _PyLong_New(size_v+1);
    if (v == NULL) {
        *prem = NULL;
        return NULL;
    }
    w = _PyLong_New(size_w);
    if (w == NULL) {
        Py_DECREF(v);
        *prem = NULL;
        return NULL;
    }

    /* normalize: shift w1 left so that its top digit is >= PyLong_BASE/2.
       shift v1 left by the same amount.  Results go into w and v. */
    d = PyLong_SHIFT - bits_in_digit(w1->ob_digit[size_w-1]);
    carry = v_lshift(w->ob_digit, w1->ob_digit, size_w, d);
    assert(carry == 0);
    carry = v_lshift(v->ob_digit, v1->ob_digit, size_v, d);
    if (carry != 0 || v->ob_digit[size_v-1] >= w->ob_digit[size_w-1]) {
        v->ob_digit[size_v] = carry;
        size_v++;
    }

    /* Now v->ob_digit[size_v-1] < w->ob_digit[size_w-1], so quotient has
       at most (and usually exactly) k = size_v - size_w digits. */
    k = size_v - size_w;
    assert(k >= 0);
    a = _PyLong_New(k);
    if (a == NULL) {
        Py_DECREF(w);
        Py_DECREF(v);
        *prem = NULL;
        return NULL;
    }
    v0 = v->ob_digit;
    w0 = w->ob_digit;
    wm1 = w0[size_w-1];
    wm2 = w0[size_w-2];
    for (vk = v0+k, ak = a->ob_digit + k; vk-- > v0;) {
        /* inner loop: divide vk[0:size_w+1] by w0[0:size_w], giving
           single-digit quotient q, remainder in vk[0:size_w]. */

        SIGCHECK({
                Py_DECREF(a);
                Py_DECREF(w);
                Py_DECREF(v);
                *prem = NULL;
                return NULL;
            });

        /* estimate quotient digit q; may overestimate by 1 (rare) */
        vtop = vk[size_w];
        assert(vtop <= wm1);
        vv = ((twodigits)vtop << PyLong_SHIFT) | vk[size_w-1];
        q = (digit)(vv / wm1);
        r = (digit)(vv - (twodigits)wm1 * q); /* r = vv % wm1 */
        while ((twodigits)wm2 * q > (((twodigits)r << PyLong_SHIFT)
                                     | vk[size_w-2])) {
            --q;
            r += wm1;
            if (r >= PyLong_BASE)
                break;
        }
        assert(q <= PyLong_BASE);

        /* subtract q*w0[0:size_w] from vk[0:size_w+1] */
        zhi = 0;
        for (i = 0; i < size_w; ++i) {
            /* invariants: -PyLong_BASE <= -q <= zhi <= 0;
               -PyLong_BASE * q <= z < PyLong_BASE */
            z = (sdigit)vk[i] + zhi -
                (stwodigits)q * (stwodigits)w0[i];
            vk[i] = (digit)z & PyLong_MASK;
            zhi = (sdigit)Py_ARITHMETIC_RIGHT_SHIFT(stwodigits,
                                                    z, PyLong_SHIFT);
        }

        /* add w back if q was too large (this branch taken rarely) */
        assert((sdigit)vtop + zhi == -1 || (sdigit)vtop + zhi == 0);
        if ((sdigit)vtop + zhi < 0) {
            carry = 0;
            for (i = 0; i < size_w; ++i) {
                carry += vk[i] + w0[i];
                vk[i] = carry & PyLong_MASK;
                carry >>= PyLong_SHIFT;
            }
            --q;
        }

        /* store quotient digit */
        assert(q < PyLong_BASE);
        *--ak = q;
    }

    /* unshift remainder; we reuse w to store the result */
    carry = v_rshift(w0, v0, size_w, d);
    assert(carry==0);
    Py_DECREF(v);

    *prem = long_normalize(w);
    return long_normalize(a);
}

/* For a nonzero PyLong a, express a in the form x * 2**e, with 0.5 <=
   abs(x) < 1.0 and e >= 0; return x and put e in *e.  Here x is
   rounded to DBL_MANT_DIG significant bits using round-half-to-even.
   If a == 0, return 0.0 and set *e = 0.  If the resulting exponent
   e is larger than PY_SSIZE_T_MAX, raise OverflowError and return
   -1.0. */

/* attempt to define 2.0**DBL_MANT_DIG as a compile-time constant */
#if DBL_MANT_DIG == 53
#define EXP2_DBL_MANT_DIG 9007199254740992.0
#else
#define EXP2_DBL_MANT_DIG (ldexp(1.0, DBL_MANT_DIG))
#endif

double
_PyLong_Frexp(PyLongObject *a, Py_ssize_t *e)
{
    Py_ssize_t a_size, a_bits, shift_digits, shift_bits, x_size;
    /* See below for why x_digits is always large enough. */
    digit rem, x_digits[2 + (DBL_MANT_DIG + 1) / PyLong_SHIFT];
    double dx;
    /* Correction term for round-half-to-even rounding.  For a digit x,
       "x + half_even_correction[x & 7]" gives x rounded to the nearest
       multiple of 4, rounding ties to a multiple of 8. */
    static const int half_even_correction[8] = {0, -1, -2, 1, 0, -1, 2, 1};

    a_size = Py_ABS(Py_SIZE(a));
    if (a_size == 0) {
        /* Special case for 0: significand 0.0, exponent 0. */
        *e = 0;
        return 0.0;
    }
    a_bits = bits_in_digit(a->ob_digit[a_size-1]);
    /* The following is an overflow-free version of the check
       "if ((a_size - 1) * PyLong_SHIFT + a_bits > PY_SSIZE_T_MAX) ..." */
    if (a_size >= (PY_SSIZE_T_MAX - 1) / PyLong_SHIFT + 1 &&
        (a_size > (PY_SSIZE_T_MAX - 1) / PyLong_SHIFT + 1 ||
         a_bits > (PY_SSIZE_T_MAX - 1) % PyLong_SHIFT + 1))
        goto overflow;
    a_bits = (a_size - 1) * PyLong_SHIFT + a_bits;

    /* Shift the first DBL_MANT_DIG + 2 bits of a into x_digits[0:x_size]
       (shifting left if a_bits <= DBL_MANT_DIG + 2).

       Number of digits needed for result: write // for floor division.
       Then if shifting left, we end up using

         1 + a_size + (DBL_MANT_DIG + 2 - a_bits) // PyLong_SHIFT

       digits.  If shifting right, we use

         a_size - (a_bits - DBL_MANT_DIG - 2) // PyLong_SHIFT

       digits.  Using a_size = 1 + (a_bits - 1) // PyLong_SHIFT along with
       the inequalities

         m // PyLong_SHIFT + n // PyLong_SHIFT <= (m + n) // PyLong_SHIFT
         m // PyLong_SHIFT - n // PyLong_SHIFT <=
                                          1 + (m - n - 1) // PyLong_SHIFT,

       valid for any integers m and n, we find that x_size satisfies

         x_size <= 2 + (DBL_MANT_DIG + 1) // PyLong_SHIFT

       in both cases.
    */
    if (a_bits <= DBL_MANT_DIG + 2) {
        shift_digits = (DBL_MANT_DIG + 2 - a_bits) / PyLong_SHIFT;
        shift_bits = (DBL_MANT_DIG + 2 - a_bits) % PyLong_SHIFT;
        x_size = 0;
        while (x_size < shift_digits)
            x_digits[x_size++] = 0;
        rem = v_lshift(x_digits + x_size, a->ob_digit, a_size,
                       (int)shift_bits);
        x_size += a_size;
        x_digits[x_size++] = rem;
    }
    else {
        shift_digits = (a_bits - DBL_MANT_DIG - 2) / PyLong_SHIFT;
        shift_bits = (a_bits - DBL_MANT_DIG - 2) % PyLong_SHIFT;
        rem = v_rshift(x_digits, a->ob_digit + shift_digits,
                       a_size - shift_digits, (int)shift_bits);
        x_size = a_size - shift_digits;
        /* For correct rounding below, we need the least significant
           bit of x to be 'sticky' for this shift: if any of the bits
           shifted out was nonzero, we set the least significant bit
           of x. */
        if (rem)
            x_digits[0] |= 1;
        else
            while (shift_digits > 0)
                if (a->ob_digit[--shift_digits]) {
                    x_digits[0] |= 1;
                    break;
                }
    }
    assert(1 <= x_size && x_size <= (Py_ssize_t)Py_ARRAY_LENGTH(x_digits));

    /* Round, and convert to double. */
    x_digits[0] += half_even_correction[x_digits[0] & 7];
    dx = x_digits[--x_size];
    while (x_size > 0)
        dx = dx * PyLong_BASE + x_digits[--x_size];

    /* Rescale;  make correction if result is 1.0. */
    dx /= 4.0 * EXP2_DBL_MANT_DIG;
    if (dx == 1.0) {
        if (a_bits == PY_SSIZE_T_MAX)
            goto overflow;
        dx = 0.5;
        a_bits += 1;
    }

    *e = a_bits;
    return Py_SIZE(a) < 0 ? -dx : dx;

  overflow:
    /* exponent > PY_SSIZE_T_MAX */
    PyErr_SetString(PyExc_OverflowError,
                    "huge integer: number of bits overflows a Py_ssize_t");
    *e = 0;
    return -1.0;
}

/* Get a C double from an int object.  Rounds to the nearest double,
   using the round-half-to-even rule in the case of a tie. */

double
PyLong_AsDouble(PyObject *v)
{
    Py_ssize_t exponent;
    double x;

    if (v == NULL) {
        PyErr_BadInternalCall();
        return -1.0;
    }
    if (!PyLong_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "an integer is required");
        return -1.0;
    }
    if (Py_ABS(Py_SIZE(v)) <= 1) {
        /* Fast path; single digit long (31 bits) will cast safely
           to double.  This improves performance of FP/long operations
           by 20%.
        */
        return (double)MEDIUM_VALUE((PyLongObject *)v);
    }
    x = _PyLong_Frexp((PyLongObject *)v, &exponent);
    if ((x == -1.0 && PyErr_Occurred()) || exponent > DBL_MAX_EXP) {
        PyErr_SetString(PyExc_OverflowError,
                        "int too large to convert to float");
        return -1.0;
    }
    return ldexp(x, (int)exponent);
}

/* Methods */

static void
long_dealloc(PyObject *v)
{
    Py_TYPE(v)->tp_free(v);
}

static int
long_compare(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t sign;

    if (Py_SIZE(a) != Py_SIZE(b)) {
        sign = Py_SIZE(a) - Py_SIZE(b);
    }
    else {
        Py_ssize_t i = Py_ABS(Py_SIZE(a));
        while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
            ;
        if (i < 0)
            sign = 0;
        else {
            sign = (sdigit)a->ob_digit[i] - (sdigit)b->ob_digit[i];
            if (Py_SIZE(a) < 0)
                sign = -sign;
        }
    }
    return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}

static PyObject *
long_richcompare(PyObject *self, PyObject *other, int op)
{
    int result;
    CHECK_BINOP(self, other);
    if (self == other)
        result = 0;
    else
        result = long_compare((PyLongObject*)self, (PyLongObject*)other);
    Py_RETURN_RICHCOMPARE(result, 0, op);
}

static Py_hash_t
long_hash(PyLongObject *v)
{
    Py_uhash_t x;
    Py_ssize_t i;
    int sign;

    i = Py_SIZE(v);
    switch(i) {
    case -1: return v->ob_digit[0]==1 ? -2 : -(sdigit)v->ob_digit[0];
    case 0: return 0;
    case 1: return v->ob_digit[0];
    }
    sign = 1;
    x = 0;
    if (i < 0) {
        sign = -1;
        i = -(i);
    }
    while (--i >= 0) {
        /* Here x is a quantity in the range [0, _PyHASH_MODULUS); we
           want to compute x * 2**PyLong_SHIFT + v->ob_digit[i] modulo
           _PyHASH_MODULUS.

           The computation of x * 2**PyLong_SHIFT % _PyHASH_MODULUS
           amounts to a rotation of the bits of x.  To see this, write

             x * 2**PyLong_SHIFT = y * 2**_PyHASH_BITS + z

           where y = x >> (_PyHASH_BITS - PyLong_SHIFT) gives the top
           PyLong_SHIFT bits of x (those that are shifted out of the
           original _PyHASH_BITS bits, and z = (x << PyLong_SHIFT) &
           _PyHASH_MODULUS gives the bottom _PyHASH_BITS - PyLong_SHIFT
           bits of x, shifted up.  Then since 2**_PyHASH_BITS is
           congruent to 1 modulo _PyHASH_MODULUS, y*2**_PyHASH_BITS is
           congruent to y modulo _PyHASH_MODULUS.  So

             x * 2**PyLong_SHIFT = y + z (mod _PyHASH_MODULUS).

           The right-hand side is just the result of rotating the
           _PyHASH_BITS bits of x left by PyLong_SHIFT places; since
           not all _PyHASH_BITS bits of x are 1s, the same is true
           after rotation, so 0 <= y+z < _PyHASH_MODULUS and y + z is
           the reduction of x*2**PyLong_SHIFT modulo
           _PyHASH_MODULUS. */
        x = ((x << PyLong_SHIFT) & _PyHASH_MODULUS) |
            (x >> (_PyHASH_BITS - PyLong_SHIFT));
        x += v->ob_digit[i];
        if (x >= _PyHASH_MODULUS)
            x -= _PyHASH_MODULUS;
    }
    x = x * sign;
    if (x == (Py_uhash_t)-1)
        x = (Py_uhash_t)-2;
    return (Py_hash_t)x;
}


/* Add the absolute values of two integers. */

static PyLongObject *
x_add(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t size_a = Py_ABS(Py_SIZE(a)), size_b = Py_ABS(Py_SIZE(b));
    PyLongObject *z;
    Py_ssize_t i;
    digit carry = 0;

    /* Ensure a is the larger of the two: */
    if (size_a < size_b) {
        { PyLongObject *temp = a; a = b; b = temp; }
        { Py_ssize_t size_temp = size_a;
            size_a = size_b;
            size_b = size_temp; }
    }
    z = _PyLong_New(size_a+1);
    if (z == NULL)
        return NULL;
    for (i = 0; i < size_b; ++i) {
        carry += a->ob_digit[i] + b->ob_digit[i];
        z->ob_digit[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
    }
    for (; i < size_a; ++i) {
        carry += a->ob_digit[i];
        z->ob_digit[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
    }
    z->ob_digit[i] = carry;
    return long_normalize(z);
}

/* Subtract the absolute values of two integers. */

static PyLongObject *
x_sub(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t size_a = Py_ABS(Py_SIZE(a)), size_b = Py_ABS(Py_SIZE(b));
    PyLongObject *z;
    Py_ssize_t i;
    int sign = 1;
    digit borrow = 0;

    /* Ensure a is the larger of the two: */
    if (size_a < size_b) {
        sign = -1;
        { PyLongObject *temp = a; a = b; b = temp; }
        { Py_ssize_t size_temp = size_a;
            size_a = size_b;
            size_b = size_temp; }
    }
    else if (size_a == size_b) {
        /* Find highest digit where a and b differ: */
        i = size_a;
        while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
            ;
        if (i < 0)
            return (PyLongObject *)PyLong_FromLong(0);
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
           works module 2**N for some N>PyLong_SHIFT. */
        borrow = a->ob_digit[i] - b->ob_digit[i] - borrow;
        z->ob_digit[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1; /* Keep only one sign bit */
    }
    for (; i < size_a; ++i) {
        borrow = a->ob_digit[i] - borrow;
        z->ob_digit[i] = borrow & PyLong_MASK;
        borrow >>= PyLong_SHIFT;
        borrow &= 1; /* Keep only one sign bit */
    }
    assert(borrow == 0);
    if (sign < 0) {
        Py_SIZE(z) = -Py_SIZE(z);
    }
    return long_normalize(z);
}

static PyObject *
long_add(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z;

    CHECK_BINOP(a, b);

    if (Py_ABS(Py_SIZE(a)) <= 1 && Py_ABS(Py_SIZE(b)) <= 1) {
        return PyLong_FromLong(MEDIUM_VALUE(a) + MEDIUM_VALUE(b));
    }
    if (Py_SIZE(a) < 0) {
        if (Py_SIZE(b) < 0) {
            z = x_add(a, b);
            if (z != NULL) {
                /* x_add received at least one multiple-digit int,
                   and thus z must be a multiple-digit int.
                   That also means z is not an element of
                   small_ints, so negating it in-place is safe. */
                assert(Py_REFCNT(z) == 1);
                Py_SIZE(z) = -(Py_SIZE(z));
            }
        }
        else
            z = x_sub(b, a);
    }
    else {
        if (Py_SIZE(b) < 0)
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

    CHECK_BINOP(a, b);

    if (Py_ABS(Py_SIZE(a)) <= 1 && Py_ABS(Py_SIZE(b)) <= 1) {
        return PyLong_FromLong(MEDIUM_VALUE(a) - MEDIUM_VALUE(b));
    }
    if (Py_SIZE(a) < 0) {
        if (Py_SIZE(b) < 0)
            z = x_sub(a, b);
        else
            z = x_add(a, b);
        if (z != NULL) {
            assert(Py_SIZE(z) == 0 || Py_REFCNT(z) == 1);
            Py_SIZE(z) = -(Py_SIZE(z));
        }
    }
    else {
        if (Py_SIZE(b) < 0)
            z = x_add(a, b);
        else
            z = x_sub(a, b);
    }
    return (PyObject *)z;
}

/* Grade school multiplication, ignoring the signs.
 * Returns the absolute value of the product, or NULL if error.
 */
static PyLongObject *
x_mul(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z;
    Py_ssize_t size_a = Py_ABS(Py_SIZE(a));
    Py_ssize_t size_b = Py_ABS(Py_SIZE(b));
    Py_ssize_t i;

    z = _PyLong_New(size_a + size_b);
    if (z == NULL)
        return NULL;

    memset(z->ob_digit, 0, Py_SIZE(z) * sizeof(digit));
    if (a == b) {
        /* Efficient squaring per HAC, Algorithm 14.16:
         * http://www.cacr.math.uwaterloo.ca/hac/about/chap14.pdf
         * Gives slightly less than a 2x speedup when a == b,
         * via exploiting that each entry in the multiplication
         * pyramid appears twice (except for the size_a squares).
         */
        for (i = 0; i < size_a; ++i) {
            twodigits carry;
            twodigits f = a->ob_digit[i];
            digit *pz = z->ob_digit + (i << 1);
            digit *pa = a->ob_digit + i + 1;
            digit *paend = a->ob_digit + size_a;

            SIGCHECK({
                    Py_DECREF(z);
                    return NULL;
                });

            carry = *pz + f * f;
            *pz++ = (digit)(carry & PyLong_MASK);
            carry >>= PyLong_SHIFT;
            assert(carry <= PyLong_MASK);

            /* Now f is added in twice in each column of the
             * pyramid it appears.  Same as adding f<<1 once.
             */
            f <<= 1;
            while (pa < paend) {
                carry += *pz + *pa++ * f;
                *pz++ = (digit)(carry & PyLong_MASK);
                carry >>= PyLong_SHIFT;
                assert(carry <= (PyLong_MASK << 1));
            }
            if (carry) {
                carry += *pz;
                *pz++ = (digit)(carry & PyLong_MASK);
                carry >>= PyLong_SHIFT;
            }
            if (carry)
                *pz += (digit)(carry & PyLong_MASK);
            assert((carry >> PyLong_SHIFT) == 0);
        }
    }
    else {      /* a is not the same as b -- gradeschool int mult */
        for (i = 0; i < size_a; ++i) {
            twodigits carry = 0;
            twodigits f = a->ob_digit[i];
            digit *pz = z->ob_digit + i;
            digit *pb = b->ob_digit;
            digit *pbend = b->ob_digit + size_b;

            SIGCHECK({
                    Py_DECREF(z);
                    return NULL;
                });

            while (pb < pbend) {
                carry += *pz + *pb++ * f;
                *pz++ = (digit)(carry & PyLong_MASK);
                carry >>= PyLong_SHIFT;
                assert(carry <= PyLong_MASK);
            }
            if (carry)
                *pz += (digit)(carry & PyLong_MASK);
            assert((carry >> PyLong_SHIFT) == 0);
        }
    }
    return long_normalize(z);
}

/* A helper for Karatsuba multiplication (k_mul).
   Takes an int "n" and an integer "size" representing the place to
   split, and sets low and high such that abs(n) == (high << size) + low,
   viewing the shift as being by digits.  The sign bit is ignored, and
   the return values are >= 0.
   Returns 0 on success, -1 on failure.
*/
static int
kmul_split(PyLongObject *n,
           Py_ssize_t size,
           PyLongObject **high,
           PyLongObject **low)
{
    PyLongObject *hi, *lo;
    Py_ssize_t size_lo, size_hi;
    const Py_ssize_t size_n = Py_ABS(Py_SIZE(n));

    size_lo = Py_MIN(size_n, size);
    size_hi = size_n - size_lo;

    if ((hi = _PyLong_New(size_hi)) == NULL)
        return -1;
    if ((lo = _PyLong_New(size_lo)) == NULL) {
        Py_DECREF(hi);
        return -1;
    }

    memcpy(lo->ob_digit, n->ob_digit, size_lo * sizeof(digit));
    memcpy(hi->ob_digit, n->ob_digit + size_lo, size_hi * sizeof(digit));

    *high = long_normalize(hi);
    *low = long_normalize(lo);
    return 0;
}

static PyLongObject *k_lopsided_mul(PyLongObject *a, PyLongObject *b);

/* Karatsuba multiplication.  Ignores the input signs, and returns the
 * absolute value of the product (or NULL if error).
 * See Knuth Vol. 2 Chapter 4.3.3 (Pp. 294-295).
 */
static PyLongObject *
k_mul(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t asize = Py_ABS(Py_SIZE(a));
    Py_ssize_t bsize = Py_ABS(Py_SIZE(b));
    PyLongObject *ah = NULL;
    PyLongObject *al = NULL;
    PyLongObject *bh = NULL;
    PyLongObject *bl = NULL;
    PyLongObject *ret = NULL;
    PyLongObject *t1, *t2, *t3;
    Py_ssize_t shift;           /* the number of digits we split off */
    Py_ssize_t i;

    /* (ah*X+al)(bh*X+bl) = ah*bh*X*X + (ah*bl + al*bh)*X + al*bl
     * Let k = (ah+al)*(bh+bl) = ah*bl + al*bh  + ah*bh + al*bl
     * Then the original product is
     *     ah*bh*X*X + (k - ah*bh - al*bl)*X + al*bl
     * By picking X to be a power of 2, "*X" is just shifting, and it's
     * been reduced to 3 multiplies on numbers half the size.
     */

    /* We want to split based on the larger number; fiddle so that b
     * is largest.
     */
    if (asize > bsize) {
        t1 = a;
        a = b;
        b = t1;

        i = asize;
        asize = bsize;
        bsize = i;
    }

    /* Use gradeschool math when either number is too small. */
    i = a == b ? KARATSUBA_SQUARE_CUTOFF : KARATSUBA_CUTOFF;
    if (asize <= i) {
        if (asize == 0)
            return (PyLongObject *)PyLong_FromLong(0);
        else
            return x_mul(a, b);
    }

    /* If a is small compared to b, splitting on b gives a degenerate
     * case with ah==0, and Karatsuba may be (even much) less efficient
     * than "grade school" then.  However, we can still win, by viewing
     * b as a string of "big digits", each of width a->ob_size.  That
     * leads to a sequence of balanced calls to k_mul.
     */
    if (2 * asize <= bsize)
        return k_lopsided_mul(a, b);

    /* Split a & b into hi & lo pieces. */
    shift = bsize >> 1;
    if (kmul_split(a, shift, &ah, &al) < 0) goto fail;
    assert(Py_SIZE(ah) > 0);            /* the split isn't degenerate */

    if (a == b) {
        bh = ah;
        bl = al;
        Py_INCREF(bh);
        Py_INCREF(bl);
    }
    else if (kmul_split(b, shift, &bh, &bl) < 0) goto fail;

    /* The plan:
     * 1. Allocate result space (asize + bsize digits:  that's always
     *    enough).
     * 2. Compute ah*bh, and copy into result at 2*shift.
     * 3. Compute al*bl, and copy into result at 0.  Note that this
     *    can't overlap with #2.
     * 4. Subtract al*bl from the result, starting at shift.  This may
     *    underflow (borrow out of the high digit), but we don't care:
     *    we're effectively doing unsigned arithmetic mod
     *    BASE**(sizea + sizeb), and so long as the *final* result fits,
     *    borrows and carries out of the high digit can be ignored.
     * 5. Subtract ah*bh from the result, starting at shift.
     * 6. Compute (ah+al)*(bh+bl), and add it into the result starting
     *    at shift.
     */

    /* 1. Allocate result space. */
    ret = _PyLong_New(asize + bsize);
    if (ret == NULL) goto fail;
#ifdef Py_DEBUG
    /* Fill with trash, to catch reference to uninitialized digits. */
    memset(ret->ob_digit, 0xDF, Py_SIZE(ret) * sizeof(digit));
#endif

    /* 2. t1 <- ah*bh, and copy into high digits of result. */
    if ((t1 = k_mul(ah, bh)) == NULL) goto fail;
    assert(Py_SIZE(t1) >= 0);
    assert(2*shift + Py_SIZE(t1) <= Py_SIZE(ret));
    memcpy(ret->ob_digit + 2*shift, t1->ob_digit,
           Py_SIZE(t1) * sizeof(digit));

    /* Zero-out the digits higher than the ah*bh copy. */
    i = Py_SIZE(ret) - 2*shift - Py_SIZE(t1);
    if (i)
        memset(ret->ob_digit + 2*shift + Py_SIZE(t1), 0,
               i * sizeof(digit));

    /* 3. t2 <- al*bl, and copy into the low digits. */
    if ((t2 = k_mul(al, bl)) == NULL) {
        Py_DECREF(t1);
        goto fail;
    }
    assert(Py_SIZE(t2) >= 0);
    assert(Py_SIZE(t2) <= 2*shift); /* no overlap with high digits */
    memcpy(ret->ob_digit, t2->ob_digit, Py_SIZE(t2) * sizeof(digit));

    /* Zero out remaining digits. */
    i = 2*shift - Py_SIZE(t2);          /* number of uninitialized digits */
    if (i)
        memset(ret->ob_digit + Py_SIZE(t2), 0, i * sizeof(digit));

    /* 4 & 5. Subtract ah*bh (t1) and al*bl (t2).  We do al*bl first
     * because it's fresher in cache.
     */
    i = Py_SIZE(ret) - shift;  /* # digits after shift */
    (void)v_isub(ret->ob_digit + shift, i, t2->ob_digit, Py_SIZE(t2));
    Py_DECREF(t2);

    (void)v_isub(ret->ob_digit + shift, i, t1->ob_digit, Py_SIZE(t1));
    Py_DECREF(t1);

    /* 6. t3 <- (ah+al)(bh+bl), and add into result. */
    if ((t1 = x_add(ah, al)) == NULL) goto fail;
    Py_DECREF(ah);
    Py_DECREF(al);
    ah = al = NULL;

    if (a == b) {
        t2 = t1;
        Py_INCREF(t2);
    }
    else if ((t2 = x_add(bh, bl)) == NULL) {
        Py_DECREF(t1);
        goto fail;
    }
    Py_DECREF(bh);
    Py_DECREF(bl);
    bh = bl = NULL;

    t3 = k_mul(t1, t2);
    Py_DECREF(t1);
    Py_DECREF(t2);
    if (t3 == NULL) goto fail;
    assert(Py_SIZE(t3) >= 0);

    /* Add t3.  It's not obvious why we can't run out of room here.
     * See the (*) comment after this function.
     */
    (void)v_iadd(ret->ob_digit + shift, i, t3->ob_digit, Py_SIZE(t3));
    Py_DECREF(t3);

    return long_normalize(ret);

  fail:
    Py_XDECREF(ret);
    Py_XDECREF(ah);
    Py_XDECREF(al);
    Py_XDECREF(bh);
    Py_XDECREF(bl);
    return NULL;
}

/* (*) Why adding t3 can't "run out of room" above.

Let f(x) mean the floor of x and c(x) mean the ceiling of x.  Some facts
to start with:

1. For any integer i, i = c(i/2) + f(i/2).  In particular,
   bsize = c(bsize/2) + f(bsize/2).
2. shift = f(bsize/2)
3. asize <= bsize
4. Since we call k_lopsided_mul if asize*2 <= bsize, asize*2 > bsize in this
   routine, so asize > bsize/2 >= f(bsize/2) in this routine.

We allocated asize + bsize result digits, and add t3 into them at an offset
of shift.  This leaves asize+bsize-shift allocated digit positions for t3
to fit into, = (by #1 and #2) asize + f(bsize/2) + c(bsize/2) - f(bsize/2) =
asize + c(bsize/2) available digit positions.

bh has c(bsize/2) digits, and bl at most f(size/2) digits.  So bh+hl has
at most c(bsize/2) digits + 1 bit.

If asize == bsize, ah has c(bsize/2) digits, else ah has at most f(bsize/2)
digits, and al has at most f(bsize/2) digits in any case.  So ah+al has at
most (asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 1 bit.

The product (ah+al)*(bh+bl) therefore has at most

    c(bsize/2) + (asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 2 bits

and we have asize + c(bsize/2) available digit positions.  We need to show
this is always enough.  An instance of c(bsize/2) cancels out in both, so
the question reduces to whether asize digits is enough to hold
(asize == bsize ? c(bsize/2) : f(bsize/2)) digits + 2 bits.  If asize < bsize,
then we're asking whether asize digits >= f(bsize/2) digits + 2 bits.  By #4,
asize is at least f(bsize/2)+1 digits, so this in turn reduces to whether 1
digit is enough to hold 2 bits.  This is so since PyLong_SHIFT=15 >= 2.  If
asize == bsize, then we're asking whether bsize digits is enough to hold
c(bsize/2) digits + 2 bits, or equivalently (by #1) whether f(bsize/2) digits
is enough to hold 2 bits.  This is so if bsize >= 2, which holds because
bsize >= KARATSUBA_CUTOFF >= 2.

Note that since there's always enough room for (ah+al)*(bh+bl), and that's
clearly >= each of ah*bh and al*bl, there's always enough room to subtract
ah*bh and al*bl too.
*/

/* b has at least twice the digits of a, and a is big enough that Karatsuba
 * would pay off *if* the inputs had balanced sizes.  View b as a sequence
 * of slices, each with a->ob_size digits, and multiply the slices by a,
 * one at a time.  This gives k_mul balanced inputs to work with, and is
 * also cache-friendly (we compute one double-width slice of the result
 * at a time, then move on, never backtracking except for the helpful
 * single-width slice overlap between successive partial sums).
 */
static PyLongObject *
k_lopsided_mul(PyLongObject *a, PyLongObject *b)
{
    const Py_ssize_t asize = Py_ABS(Py_SIZE(a));
    Py_ssize_t bsize = Py_ABS(Py_SIZE(b));
    Py_ssize_t nbdone;          /* # of b digits already multiplied */
    PyLongObject *ret;
    PyLongObject *bslice = NULL;

    assert(asize > KARATSUBA_CUTOFF);
    assert(2 * asize <= bsize);

    /* Allocate result space, and zero it out. */
    ret = _PyLong_New(asize + bsize);
    if (ret == NULL)
        return NULL;
    memset(ret->ob_digit, 0, Py_SIZE(ret) * sizeof(digit));

    /* Successive slices of b are copied into bslice. */
    bslice = _PyLong_New(asize);
    if (bslice == NULL)
        goto fail;

    nbdone = 0;
    while (bsize > 0) {
        PyLongObject *product;
        const Py_ssize_t nbtouse = Py_MIN(bsize, asize);

        /* Multiply the next slice of b by a. */
        memcpy(bslice->ob_digit, b->ob_digit + nbdone,
               nbtouse * sizeof(digit));
        Py_SIZE(bslice) = nbtouse;
        product = k_mul(a, bslice);
        if (product == NULL)
            goto fail;

        /* Add into result. */
        (void)v_iadd(ret->ob_digit + nbdone, Py_SIZE(ret) - nbdone,
                     product->ob_digit, Py_SIZE(product));
        Py_DECREF(product);

        bsize -= nbtouse;
        nbdone += nbtouse;
    }

    Py_DECREF(bslice);
    return long_normalize(ret);

  fail:
    Py_DECREF(ret);
    Py_XDECREF(bslice);
    return NULL;
}

static PyObject *
long_mul(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z;

    CHECK_BINOP(a, b);

    /* fast path for single-digit multiplication */
    if (Py_ABS(Py_SIZE(a)) <= 1 && Py_ABS(Py_SIZE(b)) <= 1) {
        stwodigits v = (stwodigits)(MEDIUM_VALUE(a)) * MEDIUM_VALUE(b);
        return PyLong_FromLongLong((long long)v);
    }

    z = k_mul(a, b);
    /* Negate if exactly one of the inputs is negative. */
    if (((Py_SIZE(a) ^ Py_SIZE(b)) < 0) && z) {
        _PyLong_Negate(&z);
        if (z == NULL)
            return NULL;
    }
    return (PyObject *)z;
}

/* Fast modulo division for single-digit longs. */
static PyObject *
fast_mod(PyLongObject *a, PyLongObject *b)
{
    sdigit left = a->ob_digit[0];
    sdigit right = b->ob_digit[0];
    sdigit mod;

    assert(Py_ABS(Py_SIZE(a)) == 1);
    assert(Py_ABS(Py_SIZE(b)) == 1);

    if (Py_SIZE(a) == Py_SIZE(b)) {
        /* 'a' and 'b' have the same sign. */
        mod = left % right;
    }
    else {
        /* Either 'a' or 'b' is negative. */
        mod = right - 1 - (left - 1) % right;
    }

    return PyLong_FromLong(mod * (sdigit)Py_SIZE(b));
}

/* Fast floor division for single-digit longs. */
static PyObject *
fast_floor_div(PyLongObject *a, PyLongObject *b)
{
    sdigit left = a->ob_digit[0];
    sdigit right = b->ob_digit[0];
    sdigit div;

    assert(Py_ABS(Py_SIZE(a)) == 1);
    assert(Py_ABS(Py_SIZE(b)) == 1);

    if (Py_SIZE(a) == Py_SIZE(b)) {
        /* 'a' and 'b' have the same sign. */
        div = left / right;
    }
    else {
        /* Either 'a' or 'b' is negative. */
        div = -1 - (left - 1) / right;
    }

    return PyLong_FromLong(div);
}

/* The / and % operators are now defined in terms of divmod().
   The expression a mod b has the value a - b*floor(a/b).
   The long_divrem function gives the remainder after division of
   |a| by |b|, with the sign of a.  This is also expressed
   as a - b*trunc(a/b), if trunc truncates towards zero.
   Some examples:
     a           b      a rem b         a mod b
     13          10      3               3
    -13          10     -3               7
     13         -10      3              -7
    -13         -10     -3              -3
   So, to get from rem to mod, we have to add b if a and b
   have different signs.  We then subtract one from the 'div'
   part of the outcome to keep the invariant intact. */

/* Compute
 *     *pdiv, *pmod = divmod(v, w)
 * NULL can be passed for pdiv or pmod, in which case that part of
 * the result is simply thrown away.  The caller owns a reference to
 * each of these it requests (does not pass NULL for).
 */
static int
l_divmod(PyLongObject *v, PyLongObject *w,
         PyLongObject **pdiv, PyLongObject **pmod)
{
    PyLongObject *div, *mod;

    if (Py_ABS(Py_SIZE(v)) == 1 && Py_ABS(Py_SIZE(w)) == 1) {
        /* Fast path for single-digit longs */
        div = NULL;
        if (pdiv != NULL) {
            div = (PyLongObject *)fast_floor_div(v, w);
            if (div == NULL) {
                return -1;
            }
        }
        if (pmod != NULL) {
            mod = (PyLongObject *)fast_mod(v, w);
            if (mod == NULL) {
                Py_XDECREF(div);
                return -1;
            }
            *pmod = mod;
        }
        if (pdiv != NULL) {
            /* We only want to set `*pdiv` when `*pmod` is
               set successfully. */
            *pdiv = div;
        }
        return 0;
    }
    if (long_divrem(v, w, &div, &mod) < 0)
        return -1;
    if ((Py_SIZE(mod) < 0 && Py_SIZE(w) > 0) ||
        (Py_SIZE(mod) > 0 && Py_SIZE(w) < 0)) {
        PyLongObject *temp;
        temp = (PyLongObject *) long_add(mod, w);
        Py_DECREF(mod);
        mod = temp;
        if (mod == NULL) {
            Py_DECREF(div);
            return -1;
        }
        temp = (PyLongObject *) long_sub(div, (PyLongObject *)_PyLong_One);
        if (temp == NULL) {
            Py_DECREF(mod);
            Py_DECREF(div);
            return -1;
        }
        Py_DECREF(div);
        div = temp;
    }
    if (pdiv != NULL)
        *pdiv = div;
    else
        Py_DECREF(div);

    if (pmod != NULL)
        *pmod = mod;
    else
        Py_DECREF(mod);

    return 0;
}

static PyObject *
long_div(PyObject *a, PyObject *b)
{
    PyLongObject *div;

    CHECK_BINOP(a, b);

    if (Py_ABS(Py_SIZE(a)) == 1 && Py_ABS(Py_SIZE(b)) == 1) {
        return fast_floor_div((PyLongObject*)a, (PyLongObject*)b);
    }

    if (l_divmod((PyLongObject*)a, (PyLongObject*)b, &div, NULL) < 0)
        div = NULL;
    return (PyObject *)div;
}

/* PyLong/PyLong -> float, with correctly rounded result. */

#define MANT_DIG_DIGITS (DBL_MANT_DIG / PyLong_SHIFT)
#define MANT_DIG_BITS (DBL_MANT_DIG % PyLong_SHIFT)

static PyObject *
long_true_divide(PyObject *v, PyObject *w)
{
    PyLongObject *a, *b, *x;
    Py_ssize_t a_size, b_size, shift, extra_bits, diff, x_size, x_bits;
    digit mask, low;
    int inexact, negate, a_is_small, b_is_small;
    double dx, result;

    CHECK_BINOP(v, w);
    a = (PyLongObject *)v;
    b = (PyLongObject *)w;

    /*
       Method in a nutshell:

         0. reduce to case a, b > 0; filter out obvious underflow/overflow
         1. choose a suitable integer 'shift'
         2. use integer arithmetic to compute x = floor(2**-shift*a/b)
         3. adjust x for correct rounding
         4. convert x to a double dx with the same value
         5. return ldexp(dx, shift).

       In more detail:

       0. For any a, a/0 raises ZeroDivisionError; for nonzero b, 0/b
       returns either 0.0 or -0.0, depending on the sign of b.  For a and
       b both nonzero, ignore signs of a and b, and add the sign back in
       at the end.  Now write a_bits and b_bits for the bit lengths of a
       and b respectively (that is, a_bits = 1 + floor(log_2(a)); likewise
       for b).  Then

          2**(a_bits - b_bits - 1) < a/b < 2**(a_bits - b_bits + 1).

       So if a_bits - b_bits > DBL_MAX_EXP then a/b > 2**DBL_MAX_EXP and
       so overflows.  Similarly, if a_bits - b_bits < DBL_MIN_EXP -
       DBL_MANT_DIG - 1 then a/b underflows to 0.  With these cases out of
       the way, we can assume that

          DBL_MIN_EXP - DBL_MANT_DIG - 1 <= a_bits - b_bits <= DBL_MAX_EXP.

       1. The integer 'shift' is chosen so that x has the right number of
       bits for a double, plus two or three extra bits that will be used
       in the rounding decisions.  Writing a_bits and b_bits for the
       number of significant bits in a and b respectively, a
       straightforward formula for shift is:

          shift = a_bits - b_bits - DBL_MANT_DIG - 2

       This is fine in the usual case, but if a/b is smaller than the
       smallest normal float then it can lead to double rounding on an
       IEEE 754 platform, giving incorrectly rounded results.  So we
       adjust the formula slightly.  The actual formula used is:

           shift = MAX(a_bits - b_bits, DBL_MIN_EXP) - DBL_MANT_DIG - 2

       2. The quantity x is computed by first shifting a (left -shift bits
       if shift <= 0, right shift bits if shift > 0) and then dividing by
       b.  For both the shift and the division, we keep track of whether
       the result is inexact, in a flag 'inexact'; this information is
       needed at the rounding stage.

       With the choice of shift above, together with our assumption that
       a_bits - b_bits >= DBL_MIN_EXP - DBL_MANT_DIG - 1, it follows
       that x >= 1.

       3. Now x * 2**shift <= a/b < (x+1) * 2**shift.  We want to replace
       this with an exactly representable float of the form

          round(x/2**extra_bits) * 2**(extra_bits+shift).

       For float representability, we need x/2**extra_bits <
       2**DBL_MANT_DIG and extra_bits + shift >= DBL_MIN_EXP -
       DBL_MANT_DIG.  This translates to the condition:

          extra_bits >= MAX(x_bits, DBL_MIN_EXP - shift) - DBL_MANT_DIG

       To round, we just modify the bottom digit of x in-place; this can
       end up giving a digit with value > PyLONG_MASK, but that's not a
       problem since digits can hold values up to 2*PyLONG_MASK+1.

       With the original choices for shift above, extra_bits will always
       be 2 or 3.  Then rounding under the round-half-to-even rule, we
       round up iff the most significant of the extra bits is 1, and
       either: (a) the computation of x in step 2 had an inexact result,
       or (b) at least one other of the extra bits is 1, or (c) the least
       significant bit of x (above those to be rounded) is 1.

       4. Conversion to a double is straightforward; all floating-point
       operations involved in the conversion are exact, so there's no
       danger of rounding errors.

       5. Use ldexp(x, shift) to compute x*2**shift, the final result.
       The result will always be exactly representable as a double, except
       in the case that it overflows.  To avoid dependence on the exact
       behaviour of ldexp on overflow, we check for overflow before
       applying ldexp.  The result of ldexp is adjusted for sign before
       returning.
    */

    /* Reduce to case where a and b are both positive. */
    a_size = Py_ABS(Py_SIZE(a));
    b_size = Py_ABS(Py_SIZE(b));
    negate = (Py_SIZE(a) < 0) ^ (Py_SIZE(b) < 0);
    if (b_size == 0) {
        PyErr_SetString(PyExc_ZeroDivisionError,
                        "division by zero");
        goto error;
    }
    if (a_size == 0)
        goto underflow_or_zero;

    /* Fast path for a and b small (exactly representable in a double).
       Relies on floating-point division being correctly rounded; results
       may be subject to double rounding on x86 machines that operate with
       the x87 FPU set to 64-bit precision. */
    a_is_small = a_size <= MANT_DIG_DIGITS ||
        (a_size == MANT_DIG_DIGITS+1 &&
         a->ob_digit[MANT_DIG_DIGITS] >> MANT_DIG_BITS == 0);
    b_is_small = b_size <= MANT_DIG_DIGITS ||
        (b_size == MANT_DIG_DIGITS+1 &&
         b->ob_digit[MANT_DIG_DIGITS] >> MANT_DIG_BITS == 0);
    if (a_is_small && b_is_small) {
        double da, db;
        da = a->ob_digit[--a_size];
        while (a_size > 0)
            da = da * PyLong_BASE + a->ob_digit[--a_size];
        db = b->ob_digit[--b_size];
        while (b_size > 0)
            db = db * PyLong_BASE + b->ob_digit[--b_size];
        result = da / db;
        goto success;
    }

    /* Catch obvious cases of underflow and overflow */
    diff = a_size - b_size;
    if (diff > PY_SSIZE_T_MAX/PyLong_SHIFT - 1)
        /* Extreme overflow */
        goto overflow;
    else if (diff < 1 - PY_SSIZE_T_MAX/PyLong_SHIFT)
        /* Extreme underflow */
        goto underflow_or_zero;
    /* Next line is now safe from overflowing a Py_ssize_t */
    diff = diff * PyLong_SHIFT + bits_in_digit(a->ob_digit[a_size - 1]) -
        bits_in_digit(b->ob_digit[b_size - 1]);
    /* Now diff = a_bits - b_bits. */
    if (diff > DBL_MAX_EXP)
        goto overflow;
    else if (diff < DBL_MIN_EXP - DBL_MANT_DIG - 1)
        goto underflow_or_zero;

    /* Choose value for shift; see comments for step 1 above. */
    shift = Py_MAX(diff, DBL_MIN_EXP) - DBL_MANT_DIG - 2;

    inexact = 0;

    /* x = abs(a * 2**-shift) */
    if (shift <= 0) {
        Py_ssize_t i, shift_digits = -shift / PyLong_SHIFT;
        digit rem;
        /* x = a << -shift */
        if (a_size >= PY_SSIZE_T_MAX - 1 - shift_digits) {
            /* In practice, it's probably impossible to end up
               here.  Both a and b would have to be enormous,
               using close to SIZE_T_MAX bytes of memory each. */
            PyErr_SetString(PyExc_OverflowError,
                            "intermediate overflow during division");
            goto error;
        }
        x = _PyLong_New(a_size + shift_digits + 1);
        if (x == NULL)
            goto error;
        for (i = 0; i < shift_digits; i++)
            x->ob_digit[i] = 0;
        rem = v_lshift(x->ob_digit + shift_digits, a->ob_digit,
                       a_size, -shift % PyLong_SHIFT);
        x->ob_digit[a_size + shift_digits] = rem;
    }
    else {
        Py_ssize_t shift_digits = shift / PyLong_SHIFT;
        digit rem;
        /* x = a >> shift */
        assert(a_size >= shift_digits);
        x = _PyLong_New(a_size - shift_digits);
        if (x == NULL)
            goto error;
        rem = v_rshift(x->ob_digit, a->ob_digit + shift_digits,
                       a_size - shift_digits, shift % PyLong_SHIFT);
        /* set inexact if any of the bits shifted out is nonzero */
        if (rem)
            inexact = 1;
        while (!inexact && shift_digits > 0)
            if (a->ob_digit[--shift_digits])
                inexact = 1;
    }
    long_normalize(x);
    x_size = Py_SIZE(x);

    /* x //= b. If the remainder is nonzero, set inexact.  We own the only
       reference to x, so it's safe to modify it in-place. */
    if (b_size == 1) {
        digit rem = inplace_divrem1(x->ob_digit, x->ob_digit, x_size,
                              b->ob_digit[0]);
        long_normalize(x);
        if (rem)
            inexact = 1;
    }
    else {
        PyLongObject *div, *rem;
        div = x_divrem(x, b, &rem);
        Py_DECREF(x);
        x = div;
        if (x == NULL)
            goto error;
        if (Py_SIZE(rem))
            inexact = 1;
        Py_DECREF(rem);
    }
    x_size = Py_ABS(Py_SIZE(x));
    assert(x_size > 0); /* result of division is never zero */
    x_bits = (x_size-1)*PyLong_SHIFT+bits_in_digit(x->ob_digit[x_size-1]);

    /* The number of extra bits that have to be rounded away. */
    extra_bits = Py_MAX(x_bits, DBL_MIN_EXP - shift) - DBL_MANT_DIG;
    assert(extra_bits == 2 || extra_bits == 3);

    /* Round by directly modifying the low digit of x. */
    mask = (digit)1 << (extra_bits - 1);
    low = x->ob_digit[0] | inexact;
    if ((low & mask) && (low & (3U*mask-1U)))
        low += mask;
    x->ob_digit[0] = low & ~(2U*mask-1U);

    /* Convert x to a double dx; the conversion is exact. */
    dx = x->ob_digit[--x_size];
    while (x_size > 0)
        dx = dx * PyLong_BASE + x->ob_digit[--x_size];
    Py_DECREF(x);

    /* Check whether ldexp result will overflow a double. */
    if (shift + x_bits >= DBL_MAX_EXP &&
        (shift + x_bits > DBL_MAX_EXP || dx == ldexp(1.0, (int)x_bits)))
        goto overflow;
    result = ldexp(dx, (int)shift);

  success:
    return PyFloat_FromDouble(negate ? -result : result);

  underflow_or_zero:
    return PyFloat_FromDouble(negate ? -0.0 : 0.0);

  overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "integer division result too large for a float");
  error:
    return NULL;
}

static PyObject *
long_mod(PyObject *a, PyObject *b)
{
    PyLongObject *mod;

    CHECK_BINOP(a, b);

    if (Py_ABS(Py_SIZE(a)) == 1 && Py_ABS(Py_SIZE(b)) == 1) {
        return fast_mod((PyLongObject*)a, (PyLongObject*)b);
    }

    if (l_divmod((PyLongObject*)a, (PyLongObject*)b, NULL, &mod) < 0)
        mod = NULL;
    return (PyObject *)mod;
}

static PyObject *
long_divmod(PyObject *a, PyObject *b)
{
    PyLongObject *div, *mod;
    PyObject *z;

    CHECK_BINOP(a, b);

    if (l_divmod((PyLongObject*)a, (PyLongObject*)b, &div, &mod) < 0) {
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
    return z;
}

/* pow(v, w, x) */
static PyObject *
long_pow(PyObject *v, PyObject *w, PyObject *x)
{
    PyLongObject *a, *b, *c; /* a,b,c = v,w,x */
    int negativeOutput = 0;  /* if x<0 return negative output */

    PyLongObject *z = NULL;  /* accumulated result */
    Py_ssize_t i, j, k;             /* counters */
    PyLongObject *temp = NULL;

    /* 5-ary values.  If the exponent is large enough, table is
     * precomputed so that table[i] == a**i % c for i in range(32).
     */
    PyLongObject *table[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    /* a, b, c = v, w, x */
    CHECK_BINOP(v, w);
    a = (PyLongObject*)v; Py_INCREF(a);
    b = (PyLongObject*)w; Py_INCREF(b);
    if (PyLong_Check(x)) {
        c = (PyLongObject *)x;
        Py_INCREF(x);
    }
    else if (x == Py_None)
        c = NULL;
    else {
        Py_DECREF(a);
        Py_DECREF(b);
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (Py_SIZE(b) < 0) {  /* if exponent is negative */
        if (c) {
            PyErr_SetString(PyExc_ValueError, "pow() 2nd argument "
                            "cannot be negative when 3rd argument specified");
            goto Error;
        }
        else {
            /* else return a float.  This works because we know
               that this calls float_pow() which converts its
               arguments to double. */
            Py_DECREF(a);
            Py_DECREF(b);
            return PyFloat_Type.tp_as_number->nb_power(v, w, x);
        }
    }

    if (c) {
        /* if modulus == 0:
               raise ValueError() */
        if (Py_SIZE(c) == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "pow() 3rd argument cannot be 0");
            goto Error;
        }

        /* if modulus < 0:
               negativeOutput = True
               modulus = -modulus */
        if (Py_SIZE(c) < 0) {
            negativeOutput = 1;
            temp = (PyLongObject *)_PyLong_Copy(c);
            if (temp == NULL)
                goto Error;
            Py_DECREF(c);
            c = temp;
            temp = NULL;
            _PyLong_Negate(&c);
            if (c == NULL)
                goto Error;
        }

        /* if modulus == 1:
               return 0 */
        if ((Py_SIZE(c) == 1) && (c->ob_digit[0] == 1)) {
            z = (PyLongObject *)PyLong_FromLong(0L);
            goto Done;
        }

        /* Reduce base by modulus in some cases:
           1. If base < 0.  Forcing the base non-negative makes things easier.
           2. If base is obviously larger than the modulus.  The "small
              exponent" case later can multiply directly by base repeatedly,
              while the "large exponent" case multiplies directly by base 31
              times.  It can be unboundedly faster to multiply by
              base % modulus instead.
           We could _always_ do this reduction, but l_divmod() isn't cheap,
           so we only do it when it buys something. */
        if (Py_SIZE(a) < 0 || Py_SIZE(a) > Py_SIZE(c)) {
            if (l_divmod(a, c, NULL, &temp) < 0)
                goto Error;
            Py_DECREF(a);
            a = temp;
            temp = NULL;
        }
    }

    /* At this point a, b, and c are guaranteed non-negative UNLESS
       c is NULL, in which case a may be negative. */

    z = (PyLongObject *)PyLong_FromLong(1L);
    if (z == NULL)
        goto Error;

    /* Perform a modular reduction, X = X % c, but leave X alone if c
     * is NULL.
     */
#define REDUCE(X)                                       \
    do {                                                \
        if (c != NULL) {                                \
            if (l_divmod(X, c, NULL, &temp) < 0)        \
                goto Error;                             \
            Py_XDECREF(X);                              \
            X = temp;                                   \
            temp = NULL;                                \
        }                                               \
    } while(0)

    /* Multiply two values, then reduce the result:
       result = X*Y % c.  If c is NULL, skip the mod. */
#define MULT(X, Y, result)                      \
    do {                                        \
        temp = (PyLongObject *)long_mul(X, Y);  \
        if (temp == NULL)                       \
            goto Error;                         \
        Py_XDECREF(result);                     \
        result = temp;                          \
        temp = NULL;                            \
        REDUCE(result);                         \
    } while(0)

    if (Py_SIZE(b) <= FIVEARY_CUTOFF) {
        /* Left-to-right binary exponentiation (HAC Algorithm 14.79) */
        /* http://www.cacr.math.uwaterloo.ca/hac/about/chap14.pdf    */
        for (i = Py_SIZE(b) - 1; i >= 0; --i) {
            digit bi = b->ob_digit[i];

            for (j = (digit)1 << (PyLong_SHIFT-1); j != 0; j >>= 1) {
                MULT(z, z, z);
                if (bi & j)
                    MULT(z, a, z);
            }
        }
    }
    else {
        /* Left-to-right 5-ary exponentiation (HAC Algorithm 14.82) */
        Py_INCREF(z);           /* still holds 1L */
        table[0] = z;
        for (i = 1; i < 32; ++i)
            MULT(table[i-1], a, table[i]);

        for (i = Py_SIZE(b) - 1; i >= 0; --i) {
            const digit bi = b->ob_digit[i];

            for (j = PyLong_SHIFT - 5; j >= 0; j -= 5) {
                const int index = (bi >> j) & 0x1f;
                for (k = 0; k < 5; ++k)
                    MULT(z, z, z);
                if (index)
                    MULT(z, table[index], z);
            }
        }
    }

    if (negativeOutput && (Py_SIZE(z) != 0)) {
        temp = (PyLongObject *)long_sub(z, c);
        if (temp == NULL)
            goto Error;
        Py_DECREF(z);
        z = temp;
        temp = NULL;
    }
    goto Done;

  Error:
    Py_CLEAR(z);
    /* fall through */
  Done:
    if (Py_SIZE(b) > FIVEARY_CUTOFF) {
        for (i = 0; i < 32; ++i)
            Py_XDECREF(table[i]);
    }
    Py_DECREF(a);
    Py_DECREF(b);
    Py_XDECREF(c);
    Py_XDECREF(temp);
    return (PyObject *)z;
}

static PyObject *
long_invert(PyLongObject *v)
{
    /* Implement ~x as -(x+1) */
    PyLongObject *x;
    if (Py_ABS(Py_SIZE(v)) <=1)
        return PyLong_FromLong(-(MEDIUM_VALUE(v)+1));
    x = (PyLongObject *) long_add(v, (PyLongObject *)_PyLong_One);
    if (x == NULL)
        return NULL;
    _PyLong_Negate(&x);
    /* No need for maybe_small_long here, since any small
       longs will have been caught in the Py_SIZE <= 1 fast path. */
    return (PyObject *)x;
}

static PyObject *
long_neg(PyLongObject *v)
{
    PyLongObject *z;
    if (Py_ABS(Py_SIZE(v)) <= 1)
        return PyLong_FromLong(-MEDIUM_VALUE(v));
    z = (PyLongObject *)_PyLong_Copy(v);
    if (z != NULL)
        Py_SIZE(z) = -(Py_SIZE(v));
    return (PyObject *)z;
}

static PyObject *
long_abs(PyLongObject *v)
{
    if (Py_SIZE(v) < 0)
        return long_neg(v);
    else
        return long_long((PyObject *)v);
}

static int
long_bool(PyLongObject *v)
{
    return Py_SIZE(v) != 0;
}

/* wordshift, remshift = divmod(shiftby, PyLong_SHIFT) */
static int
divmod_shift(PyLongObject *shiftby, Py_ssize_t *wordshift, digit *remshift)
{
    assert(PyLong_Check((PyObject *)shiftby));
    assert(Py_SIZE(shiftby) >= 0);
    Py_ssize_t lshiftby = PyLong_AsSsize_t((PyObject *)shiftby);
    if (lshiftby >= 0) {
        *wordshift = lshiftby / PyLong_SHIFT;
        *remshift = lshiftby % PyLong_SHIFT;
        return 0;
    }
    /* PyLong_Check(shiftby) is true and Py_SIZE(shiftby) >= 0, so it must
       be that PyLong_AsSsize_t raised an OverflowError. */
    assert(PyErr_ExceptionMatches(PyExc_OverflowError));
    PyErr_Clear();
    PyLongObject *wordshift_obj = divrem1(shiftby, PyLong_SHIFT, remshift);
    if (wordshift_obj == NULL) {
        return -1;
    }
    *wordshift = PyLong_AsSsize_t((PyObject *)wordshift_obj);
    Py_DECREF(wordshift_obj);
    if (*wordshift >= 0 && *wordshift < PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(digit)) {
        return 0;
    }
    PyErr_Clear();
    /* Clip the value.  With such large wordshift the right shift
       returns 0 and the left shift raises an error in _PyLong_New(). */
    *wordshift = PY_SSIZE_T_MAX / sizeof(digit);
    *remshift = 0;
    return 0;
}

static PyObject *
long_rshift(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z = NULL;
    Py_ssize_t newsize, wordshift, hishift, i, j;
    digit loshift, lomask, himask;

    CHECK_BINOP(a, b);

    if (Py_SIZE(b) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "negative shift count");
        return NULL;
    }

    if (Py_SIZE(a) < 0) {
        /* Right shifting negative numbers is harder */
        PyLongObject *a1, *a2;
        a1 = (PyLongObject *) long_invert(a);
        if (a1 == NULL)
            return NULL;
        a2 = (PyLongObject *) long_rshift(a1, b);
        Py_DECREF(a1);
        if (a2 == NULL)
            return NULL;
        z = (PyLongObject *) long_invert(a2);
        Py_DECREF(a2);
    }
    else {
        if (divmod_shift(b, &wordshift, &loshift) < 0)
            return NULL;
        newsize = Py_SIZE(a) - wordshift;
        if (newsize <= 0)
            return PyLong_FromLong(0);
        hishift = PyLong_SHIFT - loshift;
        lomask = ((digit)1 << hishift) - 1;
        himask = PyLong_MASK ^ lomask;
        z = _PyLong_New(newsize);
        if (z == NULL)
            return NULL;
        for (i = 0, j = wordshift; i < newsize; i++, j++) {
            z->ob_digit[i] = (a->ob_digit[j] >> loshift) & lomask;
            if (i+1 < newsize)
                z->ob_digit[i] |= (a->ob_digit[j+1] << hishift) & himask;
        }
        z = maybe_small_long(long_normalize(z));
    }
    return (PyObject *)z;
}

static PyObject *
long_lshift(PyObject *v, PyObject *w)
{
    /* This version due to Tim Peters */
    PyLongObject *a = (PyLongObject*)v;
    PyLongObject *b = (PyLongObject*)w;
    PyLongObject *z = NULL;
    Py_ssize_t oldsize, newsize, wordshift, i, j;
    digit remshift;
    twodigits accum;

    CHECK_BINOP(a, b);

    if (Py_SIZE(b) < 0) {
        PyErr_SetString(PyExc_ValueError, "negative shift count");
        return NULL;
    }
    if (Py_SIZE(a) == 0) {
        return PyLong_FromLong(0);
    }

    if (divmod_shift(b, &wordshift, &remshift) < 0)
        return NULL;
    oldsize = Py_ABS(Py_SIZE(a));
    newsize = oldsize + wordshift;
    if (remshift)
        ++newsize;
    z = _PyLong_New(newsize);
    if (z == NULL)
        return NULL;
    if (Py_SIZE(a) < 0) {
        assert(Py_REFCNT(z) == 1);
        Py_SIZE(z) = -Py_SIZE(z);
    }
    for (i = 0; i < wordshift; i++)
        z->ob_digit[i] = 0;
    accum = 0;
    for (i = wordshift, j = 0; j < oldsize; i++, j++) {
        accum |= (twodigits)a->ob_digit[j] << remshift;
        z->ob_digit[i] = (digit)(accum & PyLong_MASK);
        accum >>= PyLong_SHIFT;
    }
    if (remshift)
        z->ob_digit[newsize-1] = (digit)accum;
    else
        assert(!accum);
    z = long_normalize(z);
    return (PyObject *) maybe_small_long(z);
}

/* Compute two's complement of digit vector a[0:m], writing result to
   z[0:m].  The digit vector a need not be normalized, but should not
   be entirely zero.  a and z may point to the same digit vector. */

static void
v_complement(digit *z, digit *a, Py_ssize_t m)
{
    Py_ssize_t i;
    digit carry = 1;
    for (i = 0; i < m; ++i) {
        carry += a[i] ^ PyLong_MASK;
        z[i] = carry & PyLong_MASK;
        carry >>= PyLong_SHIFT;
    }
    assert(carry == 0);
}

/* Bitwise and/xor/or operations */

static PyObject *
long_bitwise(PyLongObject *a,
             char op,  /* '&', '|', '^' */
             PyLongObject *b)
{
    int nega, negb, negz;
    Py_ssize_t size_a, size_b, size_z, i;
    PyLongObject *z;

    /* Bitwise operations for negative numbers operate as though
       on a two's complement representation.  So convert arguments
       from sign-magnitude to two's complement, and convert the
       result back to sign-magnitude at the end. */

    /* If a is negative, replace it by its two's complement. */
    size_a = Py_ABS(Py_SIZE(a));
    nega = Py_SIZE(a) < 0;
    if (nega) {
        z = _PyLong_New(size_a);
        if (z == NULL)
            return NULL;
        v_complement(z->ob_digit, a->ob_digit, size_a);
        a = z;
    }
    else
        /* Keep reference count consistent. */
        Py_INCREF(a);

    /* Same for b. */
    size_b = Py_ABS(Py_SIZE(b));
    negb = Py_SIZE(b) < 0;
    if (negb) {
        z = _PyLong_New(size_b);
        if (z == NULL) {
            Py_DECREF(a);
            return NULL;
        }
        v_complement(z->ob_digit, b->ob_digit, size_b);
        b = z;
    }
    else
        Py_INCREF(b);

    /* Swap a and b if necessary to ensure size_a >= size_b. */
    if (size_a < size_b) {
        z = a; a = b; b = z;
        size_z = size_a; size_a = size_b; size_b = size_z;
        negz = nega; nega = negb; negb = negz;
    }

    /* JRH: The original logic here was to allocate the result value (z)
       as the longer of the two operands.  However, there are some cases
       where the result is guaranteed to be shorter than that: AND of two
       positives, OR of two negatives: use the shorter number.  AND with
       mixed signs: use the positive number.  OR with mixed signs: use the
       negative number.
    */
    switch (op) {
    case '^':
        negz = nega ^ negb;
        size_z = size_a;
        break;
    case '&':
        negz = nega & negb;
        size_z = negb ? size_a : size_b;
        break;
    case '|':
        negz = nega | negb;
        size_z = negb ? size_b : size_a;
        break;
    default:
        PyErr_BadArgument();
        return NULL;
    }

    /* We allow an extra digit if z is negative, to make sure that
       the final two's complement of z doesn't overflow. */
    z = _PyLong_New(size_z + negz);
    if (z == NULL) {
        Py_DECREF(a);
        Py_DECREF(b);
        return NULL;
    }

    /* Compute digits for overlap of a and b. */
    switch(op) {
    case '&':
        for (i = 0; i < size_b; ++i)
            z->ob_digit[i] = a->ob_digit[i] & b->ob_digit[i];
        break;
    case '|':
        for (i = 0; i < size_b; ++i)
            z->ob_digit[i] = a->ob_digit[i] | b->ob_digit[i];
        break;
    case '^':
        for (i = 0; i < size_b; ++i)
            z->ob_digit[i] = a->ob_digit[i] ^ b->ob_digit[i];
        break;
    default:
        PyErr_BadArgument();
        return NULL;
    }

    /* Copy any remaining digits of a, inverting if necessary. */
    if (op == '^' && negb)
        for (; i < size_z; ++i)
            z->ob_digit[i] = a->ob_digit[i] ^ PyLong_MASK;
    else if (i < size_z)
        memcpy(&z->ob_digit[i], &a->ob_digit[i],
               (size_z-i)*sizeof(digit));

    /* Complement result if negative. */
    if (negz) {
        Py_SIZE(z) = -(Py_SIZE(z));
        z->ob_digit[size_z] = PyLong_MASK;
        v_complement(z->ob_digit, z->ob_digit, size_z+1);
    }

    Py_DECREF(a);
    Py_DECREF(b);
    return (PyObject *)maybe_small_long(long_normalize(z));
}

static PyObject *
long_and(PyObject *a, PyObject *b)
{
    PyObject *c;
    CHECK_BINOP(a, b);
    c = long_bitwise((PyLongObject*)a, '&', (PyLongObject*)b);
    return c;
}

static PyObject *
long_xor(PyObject *a, PyObject *b)
{
    PyObject *c;
    CHECK_BINOP(a, b);
    c = long_bitwise((PyLongObject*)a, '^', (PyLongObject*)b);
    return c;
}

static PyObject *
long_or(PyObject *a, PyObject *b)
{
    PyObject *c;
    CHECK_BINOP(a, b);
    c = long_bitwise((PyLongObject*)a, '|', (PyLongObject*)b);
    return c;
}

static PyObject *
long_long(PyObject *v)
{
    if (PyLong_CheckExact(v))
        Py_INCREF(v);
    else
        v = _PyLong_Copy((PyLongObject *)v);
    return v;
}

PyObject *
_PyLong_GCD(PyObject *aarg, PyObject *barg)
{
    PyLongObject *a, *b, *c = NULL, *d = NULL, *r;
    stwodigits x, y, q, s, t, c_carry, d_carry;
    stwodigits A, B, C, D, T;
    int nbits, k;
    Py_ssize_t size_a, size_b, alloc_a, alloc_b;
    digit *a_digit, *b_digit, *c_digit, *d_digit, *a_end, *b_end;

    a = (PyLongObject *)aarg;
    b = (PyLongObject *)barg;
    size_a = Py_SIZE(a);
    size_b = Py_SIZE(b);
    if (-2 <= size_a && size_a <= 2 && -2 <= size_b && size_b <= 2) {
        Py_INCREF(a);
        Py_INCREF(b);
        goto simple;
    }

    /* Initial reduction: make sure that 0 <= b <= a. */
    a = (PyLongObject *)long_abs(a);
    if (a == NULL)
        return NULL;
    b = (PyLongObject *)long_abs(b);
    if (b == NULL) {
        Py_DECREF(a);
        return NULL;
    }
    if (long_compare(a, b) < 0) {
        r = a;
        a = b;
        b = r;
    }
    /* We now own references to a and b */

    alloc_a = Py_SIZE(a);
    alloc_b = Py_SIZE(b);
    /* reduce until a fits into 2 digits */
    while ((size_a = Py_SIZE(a)) > 2) {
        nbits = bits_in_digit(a->ob_digit[size_a-1]);
        /* extract top 2*PyLong_SHIFT bits of a into x, along with
           corresponding bits of b into y */
        size_b = Py_SIZE(b);
        assert(size_b <= size_a);
        if (size_b == 0) {
            if (size_a < alloc_a) {
                r = (PyLongObject *)_PyLong_Copy(a);
                Py_DECREF(a);
            }
            else
                r = a;
            Py_DECREF(b);
            Py_XDECREF(c);
            Py_XDECREF(d);
            return (PyObject *)r;
        }
        x = (((twodigits)a->ob_digit[size_a-1] << (2*PyLong_SHIFT-nbits)) |
             ((twodigits)a->ob_digit[size_a-2] << (PyLong_SHIFT-nbits)) |
             (a->ob_digit[size_a-3] >> nbits));

        y = ((size_b >= size_a - 2 ? b->ob_digit[size_a-3] >> nbits : 0) |
             (size_b >= size_a - 1 ? (twodigits)b->ob_digit[size_a-2] << (PyLong_SHIFT-nbits) : 0) |
             (size_b >= size_a ? (twodigits)b->ob_digit[size_a-1] << (2*PyLong_SHIFT-nbits) : 0));

        /* inner loop of Lehmer's algorithm; A, B, C, D never grow
           larger than PyLong_MASK during the algorithm. */
        A = 1; B = 0; C = 0; D = 1;
        for (k=0;; k++) {
            if (y-C == 0)
                break;
            q = (x+(A-1))/(y-C);
            s = B+q*D;
            t = x-q*y;
            if (s > t)
                break;
            x = y; y = t;
            t = A+q*C; A = D; B = C; C = s; D = t;
        }

        if (k == 0) {
            /* no progress; do a Euclidean step */
            if (l_divmod(a, b, NULL, &r) < 0)
                goto error;
            Py_DECREF(a);
            a = b;
            b = r;
            alloc_a = alloc_b;
            alloc_b = Py_SIZE(b);
            continue;
        }

        /*
          a, b = A*b-B*a, D*a-C*b if k is odd
          a, b = A*a-B*b, D*b-C*a if k is even
        */
        if (k&1) {
            T = -A; A = -B; B = T;
            T = -C; C = -D; D = T;
        }
        if (c != NULL)
            Py_SIZE(c) = size_a;
        else if (Py_REFCNT(a) == 1) {
            Py_INCREF(a);
            c = a;
        }
        else {
            alloc_a = size_a;
            c = _PyLong_New(size_a);
            if (c == NULL)
                goto error;
        }

        if (d != NULL)
            Py_SIZE(d) = size_a;
        else if (Py_REFCNT(b) == 1 && size_a <= alloc_b) {
            Py_INCREF(b);
            d = b;
            Py_SIZE(d) = size_a;
        }
        else {
            alloc_b = size_a;
            d = _PyLong_New(size_a);
            if (d == NULL)
                goto error;
        }
        a_end = a->ob_digit + size_a;
        b_end = b->ob_digit + size_b;

        /* compute new a and new b in parallel */
        a_digit = a->ob_digit;
        b_digit = b->ob_digit;
        c_digit = c->ob_digit;
        d_digit = d->ob_digit;
        c_carry = 0;
        d_carry = 0;
        while (b_digit < b_end) {
            c_carry += (A * *a_digit) - (B * *b_digit);
            d_carry += (D * *b_digit++) - (C * *a_digit++);
            *c_digit++ = (digit)(c_carry & PyLong_MASK);
            *d_digit++ = (digit)(d_carry & PyLong_MASK);
            c_carry >>= PyLong_SHIFT;
            d_carry >>= PyLong_SHIFT;
        }
        while (a_digit < a_end) {
            c_carry += A * *a_digit;
            d_carry -= C * *a_digit++;
            *c_digit++ = (digit)(c_carry & PyLong_MASK);
            *d_digit++ = (digit)(d_carry & PyLong_MASK);
            c_carry >>= PyLong_SHIFT;
            d_carry >>= PyLong_SHIFT;
        }
        assert(c_carry == 0);
        assert(d_carry == 0);

        Py_INCREF(c);
        Py_INCREF(d);
        Py_DECREF(a);
        Py_DECREF(b);
        a = long_normalize(c);
        b = long_normalize(d);
    }
    Py_XDECREF(c);
    Py_XDECREF(d);

simple:
    assert(Py_REFCNT(a) > 0);
    assert(Py_REFCNT(b) > 0);
/* Issue #24999: use two shifts instead of ">> 2*PyLong_SHIFT" to avoid
   undefined behaviour when LONG_MAX type is smaller than 60 bits */
#if LONG_MAX >> PyLong_SHIFT >> PyLong_SHIFT
    /* a fits into a long, so b must too */
    x = PyLong_AsLong((PyObject *)a);
    y = PyLong_AsLong((PyObject *)b);
#elif PY_LLONG_MAX >> PyLong_SHIFT >> PyLong_SHIFT
    x = PyLong_AsLongLong((PyObject *)a);
    y = PyLong_AsLongLong((PyObject *)b);
#else
# error "_PyLong_GCD"
#endif
    x = Py_ABS(x);
    y = Py_ABS(y);
    Py_DECREF(a);
    Py_DECREF(b);

    /* usual Euclidean algorithm for longs */
    while (y != 0) {
        t = y;
        y = x % y;
        x = t;
    }
#if LONG_MAX >> PyLong_SHIFT >> PyLong_SHIFT
    return PyLong_FromLong(x);
#elif PY_LLONG_MAX >> PyLong_SHIFT >> PyLong_SHIFT
    return PyLong_FromLongLong(x);
#else
# error "_PyLong_GCD"
#endif

error:
    Py_DECREF(a);
    Py_DECREF(b);
    Py_XDECREF(c);
    Py_XDECREF(d);
    return NULL;
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
long_subtype_new(PyTypeObject *type, PyObject *x, PyObject *obase);

/*[clinic input]
@classmethod
int.__new__ as long_new
    x: object(c_default="NULL") = 0
    /
    base as obase: object(c_default="NULL") = 10
[clinic start generated code]*/

static PyObject *
long_new_impl(PyTypeObject *type, PyObject *x, PyObject *obase)
/*[clinic end generated code: output=e47cfe777ab0f24c input=81c98f418af9eb6f]*/
{
    Py_ssize_t base;

    if (type != &PyLong_Type)
        return long_subtype_new(type, x, obase); /* Wimp out */
    if (x == NULL) {
        if (obase != NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "int() missing string argument");
            return NULL;
        }
        return PyLong_FromLong(0L);
    }
    if (obase == NULL)
        return PyNumber_Long(x);

    base = PyNumber_AsSsize_t(obase, NULL);
    if (base == -1 && PyErr_Occurred())
        return NULL;
    if ((base != 0 && base < 2) || base > 36) {
        PyErr_SetString(PyExc_ValueError,
                        "int() base must be >= 2 and <= 36, or 0");
        return NULL;
    }

    if (PyUnicode_Check(x))
        return PyLong_FromUnicodeObject(x, (int)base);
    else if (PyByteArray_Check(x) || PyBytes_Check(x)) {
        char *string;
        if (PyByteArray_Check(x))
            string = PyByteArray_AS_STRING(x);
        else
            string = PyBytes_AS_STRING(x);
        return _PyLong_FromBytes(string, Py_SIZE(x), (int)base);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "int() can't convert non-string with explicit base");
        return NULL;
    }
}

/* Wimpy, slow approach to tp_new calls for subtypes of int:
   first create a regular int from whatever arguments we got,
   then allocate a subtype instance and initialize it from
   the regular int.  The regular int is then thrown away.
*/
static PyObject *
long_subtype_new(PyTypeObject *type, PyObject *x, PyObject *obase)
{
    PyLongObject *tmp, *newobj;
    Py_ssize_t i, n;

    assert(PyType_IsSubtype(type, &PyLong_Type));
    tmp = (PyLongObject *)long_new_impl(&PyLong_Type, x, obase);
    if (tmp == NULL)
        return NULL;
    assert(PyLong_Check(tmp));
    n = Py_SIZE(tmp);
    if (n < 0)
        n = -n;
    newobj = (PyLongObject *)type->tp_alloc(type, n);
    if (newobj == NULL) {
        Py_DECREF(tmp);
        return NULL;
    }
    assert(PyLong_Check(newobj));
    Py_SIZE(newobj) = Py_SIZE(tmp);
    for (i = 0; i < n; i++)
        newobj->ob_digit[i] = tmp->ob_digit[i];
    Py_DECREF(tmp);
    return (PyObject *)newobj;
}

/*[clinic input]
int.__getnewargs__
[clinic start generated code]*/

static PyObject *
int___getnewargs___impl(PyObject *self)
/*[clinic end generated code: output=839a49de3f00b61b input=5904770ab1fb8c75]*/
{
    return Py_BuildValue("(N)", _PyLong_Copy((PyLongObject *)self));
}

static PyObject *
long_get0(PyLongObject *v, void *context) {
    return PyLong_FromLong(0L);
}

static PyObject *
long_get1(PyLongObject *v, void *context) {
    return PyLong_FromLong(1L);
}

/*[clinic input]
int.__format__

    format_spec: unicode
    /
[clinic start generated code]*/

static PyObject *
int___format___impl(PyObject *self, PyObject *format_spec)
/*[clinic end generated code: output=b4929dee9ae18689 input=e31944a9b3e428b7]*/
{
    _PyUnicodeWriter writer;
    int ret;

    _PyUnicodeWriter_Init(&writer);
    ret = _PyLong_FormatAdvancedWriter(
        &writer,
        self,
        format_spec, 0, PyUnicode_GET_LENGTH(format_spec));
    if (ret == -1) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyUnicodeWriter_Finish(&writer);
}

/* Return a pair (q, r) such that a = b * q + r, and
   abs(r) <= abs(b)/2, with equality possible only if q is even.
   In other words, q == a / b, rounded to the nearest integer using
   round-half-to-even. */

PyObject *
_PyLong_DivmodNear(PyObject *a, PyObject *b)
{
    PyLongObject *quo = NULL, *rem = NULL;
    PyObject *twice_rem, *result, *temp;
    int cmp, quo_is_odd, quo_is_neg;

    /* Equivalent Python code:

       def divmod_near(a, b):
           q, r = divmod(a, b)
           # round up if either r / b > 0.5, or r / b == 0.5 and q is odd.
           # The expression r / b > 0.5 is equivalent to 2 * r > b if b is
           # positive, 2 * r < b if b negative.
           greater_than_half = 2*r > b if b > 0 else 2*r < b
           exactly_half = 2*r == b
           if greater_than_half or exactly_half and q % 2 == 1:
               q += 1
               r -= b
           return q, r

    */
    if (!PyLong_Check(a) || !PyLong_Check(b)) {
        PyErr_SetString(PyExc_TypeError,
                        "non-integer arguments in division");
        return NULL;
    }

    /* Do a and b have different signs?  If so, quotient is negative. */
    quo_is_neg = (Py_SIZE(a) < 0) != (Py_SIZE(b) < 0);

    if (long_divrem((PyLongObject*)a, (PyLongObject*)b, &quo, &rem) < 0)
        goto error;

    /* compare twice the remainder with the divisor, to see
       if we need to adjust the quotient and remainder */
    twice_rem = long_lshift((PyObject *)rem, _PyLong_One);
    if (twice_rem == NULL)
        goto error;
    if (quo_is_neg) {
        temp = long_neg((PyLongObject*)twice_rem);
        Py_DECREF(twice_rem);
        twice_rem = temp;
        if (twice_rem == NULL)
            goto error;
    }
    cmp = long_compare((PyLongObject *)twice_rem, (PyLongObject *)b);
    Py_DECREF(twice_rem);

    quo_is_odd = Py_SIZE(quo) != 0 && ((quo->ob_digit[0] & 1) != 0);
    if ((Py_SIZE(b) < 0 ? cmp < 0 : cmp > 0) || (cmp == 0 && quo_is_odd)) {
        /* fix up quotient */
        if (quo_is_neg)
            temp = long_sub(quo, (PyLongObject *)_PyLong_One);
        else
            temp = long_add(quo, (PyLongObject *)_PyLong_One);
        Py_DECREF(quo);
        quo = (PyLongObject *)temp;
        if (quo == NULL)
            goto error;
        /* and remainder */
        if (quo_is_neg)
            temp = long_add(rem, (PyLongObject *)b);
        else
            temp = long_sub(rem, (PyLongObject *)b);
        Py_DECREF(rem);
        rem = (PyLongObject *)temp;
        if (rem == NULL)
            goto error;
    }

    result = PyTuple_New(2);
    if (result == NULL)
        goto error;

    /* PyTuple_SET_ITEM steals references */
    PyTuple_SET_ITEM(result, 0, (PyObject *)quo);
    PyTuple_SET_ITEM(result, 1, (PyObject *)rem);
    return result;

  error:
    Py_XDECREF(quo);
    Py_XDECREF(rem);
    return NULL;
}

static PyObject *
long_round(PyObject *self, PyObject *args)
{
    PyObject *o_ndigits=NULL, *temp, *result, *ndigits;

    /* To round an integer m to the nearest 10**n (n positive), we make use of
     * the divmod_near operation, defined by:
     *
     *   divmod_near(a, b) = (q, r)
     *
     * where q is the nearest integer to the quotient a / b (the
     * nearest even integer in the case of a tie) and r == a - q * b.
     * Hence q * b = a - r is the nearest multiple of b to a,
     * preferring even multiples in the case of a tie.
     *
     * So the nearest multiple of 10**n to m is:
     *
     *   m - divmod_near(m, 10**n)[1].
     */
    if (!PyArg_ParseTuple(args, "|O", &o_ndigits))
        return NULL;
    if (o_ndigits == NULL)
        return long_long(self);

    ndigits = PyNumber_Index(o_ndigits);
    if (ndigits == NULL)
        return NULL;

    /* if ndigits >= 0 then no rounding is necessary; return self unchanged */
    if (Py_SIZE(ndigits) >= 0) {
        Py_DECREF(ndigits);
        return long_long(self);
    }

    /* result = self - divmod_near(self, 10 ** -ndigits)[1] */
    temp = long_neg((PyLongObject*)ndigits);
    Py_DECREF(ndigits);
    ndigits = temp;
    if (ndigits == NULL)
        return NULL;

    result = PyLong_FromLong(10L);
    if (result == NULL) {
        Py_DECREF(ndigits);
        return NULL;
    }

    temp = long_pow(result, ndigits, Py_None);
    Py_DECREF(ndigits);
    Py_DECREF(result);
    result = temp;
    if (result == NULL)
        return NULL;

    temp = _PyLong_DivmodNear(self, result);
    Py_DECREF(result);
    result = temp;
    if (result == NULL)
        return NULL;

    temp = long_sub((PyLongObject *)self,
                    (PyLongObject *)PyTuple_GET_ITEM(result, 1));
    Py_DECREF(result);
    result = temp;

    return result;
}

/*[clinic input]
int.__sizeof__ -> Py_ssize_t

Returns size in memory, in bytes.
[clinic start generated code]*/

static Py_ssize_t
int___sizeof___impl(PyObject *self)
/*[clinic end generated code: output=3303f008eaa6a0a5 input=9b51620c76fc4507]*/
{
    Py_ssize_t res;

    res = offsetof(PyLongObject, ob_digit) + Py_ABS(Py_SIZE(self))*sizeof(digit);
    return res;
}

/*[clinic input]
int.bit_length

Number of bits necessary to represent self in binary.

>>> bin(37)
'0b100101'
>>> (37).bit_length()
6
[clinic start generated code]*/

static PyObject *
int_bit_length_impl(PyObject *self)
/*[clinic end generated code: output=fc1977c9353d6a59 input=e4eb7a587e849a32]*/
{
    PyLongObject *result, *x, *y;
    Py_ssize_t ndigits;
    int msd_bits;
    digit msd;

    assert(self != NULL);
    assert(PyLong_Check(self));

    ndigits = Py_ABS(Py_SIZE(self));
    if (ndigits == 0)
        return PyLong_FromLong(0);

    msd = ((PyLongObject *)self)->ob_digit[ndigits-1];
    msd_bits = bits_in_digit(msd);

    if (ndigits <= PY_SSIZE_T_MAX/PyLong_SHIFT)
        return PyLong_FromSsize_t((ndigits-1)*PyLong_SHIFT + msd_bits);

    /* expression above may overflow; use Python integers instead */
    result = (PyLongObject *)PyLong_FromSsize_t(ndigits - 1);
    if (result == NULL)
        return NULL;
    x = (PyLongObject *)PyLong_FromLong(PyLong_SHIFT);
    if (x == NULL)
        goto error;
    y = (PyLongObject *)long_mul(result, x);
    Py_DECREF(x);
    if (y == NULL)
        goto error;
    Py_DECREF(result);
    result = y;

    x = (PyLongObject *)PyLong_FromLong((long)msd_bits);
    if (x == NULL)
        goto error;
    y = (PyLongObject *)long_add(result, x);
    Py_DECREF(x);
    if (y == NULL)
        goto error;
    Py_DECREF(result);
    result = y;

    return (PyObject *)result;

  error:
    Py_DECREF(result);
    return NULL;
}

#if 0
static PyObject *
long_is_finite(PyObject *v)
{
    Py_RETURN_TRUE;
}
#endif

/*[clinic input]
int.to_bytes

    length: Py_ssize_t
        Length of bytes object to use.  An OverflowError is raised if the
        integer is not representable with the given number of bytes.
    byteorder: unicode
        The byte order used to represent the integer.  If byteorder is 'big',
        the most significant byte is at the beginning of the byte array.  If
        byteorder is 'little', the most significant byte is at the end of the
        byte array.  To request the native byte order of the host system, use
        `sys.byteorder' as the byte order value.
    *
    signed as is_signed: bool = False
        Determines whether two's complement is used to represent the integer.
        If signed is False and a negative integer is given, an OverflowError
        is raised.

Return an array of bytes representing an integer.
[clinic start generated code]*/

static PyObject *
int_to_bytes_impl(PyObject *self, Py_ssize_t length, PyObject *byteorder,
                  int is_signed)
/*[clinic end generated code: output=89c801df114050a3 input=ddac63f4c7bf414c]*/
{
    int little_endian;
    PyObject *bytes;

    if (_PyUnicode_EqualToASCIIId(byteorder, &PyId_little))
        little_endian = 1;
    else if (_PyUnicode_EqualToASCIIId(byteorder, &PyId_big))
        little_endian = 0;
    else {
        PyErr_SetString(PyExc_ValueError,
            "byteorder must be either 'little' or 'big'");
        return NULL;
    }

    if (length < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "length argument must be non-negative");
        return NULL;
    }

    bytes = PyBytes_FromStringAndSize(NULL, length);
    if (bytes == NULL)
        return NULL;

    if (_PyLong_AsByteArray((PyLongObject *)self,
                            (unsigned char *)PyBytes_AS_STRING(bytes),
                            length, little_endian, is_signed) < 0) {
        Py_DECREF(bytes);
        return NULL;
    }

    return bytes;
}

/*[clinic input]
@classmethod
int.from_bytes

    bytes as bytes_obj: object
        Holds the array of bytes to convert.  The argument must either
        support the buffer protocol or be an iterable object producing bytes.
        Bytes and bytearray are examples of built-in objects that support the
        buffer protocol.
    byteorder: unicode
        The byte order used to represent the integer.  If byteorder is 'big',
        the most significant byte is at the beginning of the byte array.  If
        byteorder is 'little', the most significant byte is at the end of the
        byte array.  To request the native byte order of the host system, use
        `sys.byteorder' as the byte order value.
    *
    signed as is_signed: bool = False
        Indicates whether two's complement is used to represent the integer.

Return the integer represented by the given array of bytes.
[clinic start generated code]*/

static PyObject *
int_from_bytes_impl(PyTypeObject *type, PyObject *bytes_obj,
                    PyObject *byteorder, int is_signed)
/*[clinic end generated code: output=efc5d68e31f9314f input=cdf98332b6a821b0]*/
{
    int little_endian;
    PyObject *long_obj, *bytes;

    if (_PyUnicode_EqualToASCIIId(byteorder, &PyId_little))
        little_endian = 1;
    else if (_PyUnicode_EqualToASCIIId(byteorder, &PyId_big))
        little_endian = 0;
    else {
        PyErr_SetString(PyExc_ValueError,
            "byteorder must be either 'little' or 'big'");
        return NULL;
    }

    bytes = PyObject_Bytes(bytes_obj);
    if (bytes == NULL)
        return NULL;

    long_obj = _PyLong_FromByteArray(
        (unsigned char *)PyBytes_AS_STRING(bytes), Py_SIZE(bytes),
        little_endian, is_signed);
    Py_DECREF(bytes);

    if (long_obj != NULL && type != &PyLong_Type) {
        Py_SETREF(long_obj, PyObject_CallFunctionObjArgs((PyObject *)type,
                                                         long_obj, NULL));
    }

    return long_obj;
}

static PyMethodDef long_methods[] = {
    {"conjugate",       (PyCFunction)long_long, METH_NOARGS,
     "Returns self, the complex conjugate of any int."},
    INT_BIT_LENGTH_METHODDEF
#if 0
    {"is_finite",       (PyCFunction)long_is_finite,    METH_NOARGS,
     "Returns always True."},
#endif
    INT_TO_BYTES_METHODDEF
    INT_FROM_BYTES_METHODDEF
    {"__trunc__",       (PyCFunction)long_long, METH_NOARGS,
     "Truncating an Integral returns itself."},
    {"__floor__",       (PyCFunction)long_long, METH_NOARGS,
     "Flooring an Integral returns itself."},
    {"__ceil__",        (PyCFunction)long_long, METH_NOARGS,
     "Ceiling of an Integral returns itself."},
    {"__round__",       (PyCFunction)long_round, METH_VARARGS,
     "Rounding an Integral returns itself.\n"
     "Rounding with an ndigits argument also returns an integer."},
    INT___GETNEWARGS___METHODDEF
    INT___FORMAT___METHODDEF
    INT___SIZEOF___METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static PyGetSetDef long_getset[] = {
    {"real",
     (getter)long_long, (setter)NULL,
     "the real part of a complex number",
     NULL},
    {"imag",
     (getter)long_get0, (setter)NULL,
     "the imaginary part of a complex number",
     NULL},
    {"numerator",
     (getter)long_long, (setter)NULL,
     "the numerator of a rational number in lowest terms",
     NULL},
    {"denominator",
     (getter)long_get1, (setter)NULL,
     "the denominator of a rational number in lowest terms",
     NULL},
    {NULL}  /* Sentinel */
};

PyDoc_STRVAR(long_doc,
"int([x]) -> integer\n\
int(x, base=10) -> integer\n\
\n\
Convert a number or string to an integer, or return 0 if no arguments\n\
are given.  If x is a number, return x.__int__().  For floating point\n\
numbers, this truncates towards zero.\n\
\n\
If x is not a number or if base is given, then x must be a string,\n\
bytes, or bytearray instance representing an integer literal in the\n\
given base.  The literal can be preceded by '+' or '-' and be surrounded\n\
by whitespace.  The base defaults to 10.  Valid bases are 0 and 2-36.\n\
Base 0 means to interpret the base from the string as an integer literal.\n\
>>> int('0b100', base=0)\n\
4");

static PyNumberMethods long_as_number = {
    (binaryfunc)long_add,       /*nb_add*/
    (binaryfunc)long_sub,       /*nb_subtract*/
    (binaryfunc)long_mul,       /*nb_multiply*/
    long_mod,                   /*nb_remainder*/
    long_divmod,                /*nb_divmod*/
    long_pow,                   /*nb_power*/
    (unaryfunc)long_neg,        /*nb_negative*/
    (unaryfunc)long_long,       /*tp_positive*/
    (unaryfunc)long_abs,        /*tp_absolute*/
    (inquiry)long_bool,         /*tp_bool*/
    (unaryfunc)long_invert,     /*nb_invert*/
    long_lshift,                /*nb_lshift*/
    (binaryfunc)long_rshift,    /*nb_rshift*/
    long_and,                   /*nb_and*/
    long_xor,                   /*nb_xor*/
    long_or,                    /*nb_or*/
    long_long,                  /*nb_int*/
    0,                          /*nb_reserved*/
    long_float,                 /*nb_float*/
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    long_div,                   /* nb_floor_divide */
    long_true_divide,           /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    long_long,                  /* nb_index */
};

PyTypeObject PyLong_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "int",                                      /* tp_name */
    offsetof(PyLongObject, ob_digit),           /* tp_basicsize */
    sizeof(digit),                              /* tp_itemsize */
    long_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    long_to_decimal_string,                     /* tp_repr */
    &long_as_number,                            /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)long_hash,                        /* tp_hash */
    0,                                          /* tp_call */
    long_to_decimal_string,                     /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_LONG_SUBCLASS,               /* tp_flags */
    long_doc,                                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    long_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    long_methods,                               /* tp_methods */
    0,                                          /* tp_members */
    long_getset,                                /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    long_new,                                   /* tp_new */
    PyObject_Del,                               /* tp_free */
};

static PyTypeObject Int_InfoType;

PyDoc_STRVAR(int_info__doc__,
"sys.int_info\n\
\n\
A named tuple that holds information about Python's\n\
internal representation of integers.  The attributes are read only.");

static PyStructSequence_Field int_info_fields[] = {
    {"bits_per_digit", "size of a digit in bits"},
    {"sizeof_digit", "size in bytes of the C type used to represent a digit"},
    {NULL, NULL}
};

static PyStructSequence_Desc int_info_desc = {
    "sys.int_info",   /* name */
    int_info__doc__,  /* doc */
    int_info_fields,  /* fields */
    2                 /* number of fields */
};

PyObject *
PyLong_GetInfo(void)
{
    PyObject* int_info;
    int field = 0;
    int_info = PyStructSequence_New(&Int_InfoType);
    if (int_info == NULL)
        return NULL;
    PyStructSequence_SET_ITEM(int_info, field++,
                              PyLong_FromLong(PyLong_SHIFT));
    PyStructSequence_SET_ITEM(int_info, field++,
                              PyLong_FromLong(sizeof(digit)));
    if (PyErr_Occurred()) {
        Py_CLEAR(int_info);
        return NULL;
    }
    return int_info;
}

int
_PyLong_Init(void)
{
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
    int ival, size;
    PyLongObject *v = small_ints;

    for (ival = -NSMALLNEGINTS; ival <  NSMALLPOSINTS; ival++, v++) {
        size = (ival < 0) ? -1 : ((ival == 0) ? 0 : 1);
        if (Py_TYPE(v) == &PyLong_Type) {
            /* The element is already initialized, most likely
             * the Python interpreter was initialized before.
             */
            Py_ssize_t refcnt;
            PyObject* op = (PyObject*)v;

            refcnt = Py_REFCNT(op) < 0 ? 0 : Py_REFCNT(op);
            _Py_NewReference(op);
            /* _Py_NewReference sets the ref count to 1 but
             * the ref count might be larger. Set the refcnt
             * to the original refcnt + 1 */
            Py_REFCNT(op) = refcnt + 1;
            assert(Py_SIZE(op) == size);
            assert(v->ob_digit[0] == (digit)abs(ival));
        }
        else {
            (void)PyObject_INIT(v, &PyLong_Type);
        }
        Py_SIZE(v) = size;
        v->ob_digit[0] = (digit)abs(ival);
    }
#endif
    _PyLong_Zero = PyLong_FromLong(0);
    if (_PyLong_Zero == NULL)
        return 0;
    _PyLong_One = PyLong_FromLong(1);
    if (_PyLong_One == NULL)
        return 0;

    /* initialize int_info */
    if (Int_InfoType.tp_name == NULL) {
        if (PyStructSequence_InitType2(&Int_InfoType, &int_info_desc) < 0)
            return 0;
    }

    return 1;
}

void
PyLong_Fini(void)
{
    /* Integers are currently statically allocated. Py_DECREF is not
       needed, but Python must forget about the reference or multiple
       reinitializations will fail. */
    Py_CLEAR(_PyLong_One);
    Py_CLEAR(_PyLong_Zero);
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
    int i;
    PyLongObject *v = small_ints;
    for (i = 0; i < NSMALLNEGINTS + NSMALLPOSINTS; i++, v++) {
        _Py_DEC_REFTOTAL;
        _Py_ForgetReference((PyObject*)v);
    }
#endif
}
