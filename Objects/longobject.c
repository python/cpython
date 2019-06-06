/* Long (arbitrary precision) integer object implementation */

/* XXX The functional organization of this file is terrible */

#include "Python.h"
#include "longintrepr.h"
#include "structseq.h"

#include <float.h>
#include <ctype.h>
#include <stddef.h>

/* For long multiplication, use the O(N**2) school algorithm unless
 * both operands contain more than KARATSUBA_CUTOFF digits (this
 * being an internal Python long digit, in base PyLong_BASE).
 */
#define KARATSUBA_CUTOFF 70
#define KARATSUBA_SQUARE_CUTOFF (2 * KARATSUBA_CUTOFF)

/* For exponentiation, use the binary left-to-right algorithm
 * unless the exponent contains more than FIVEARY_CUTOFF digits.
 * In that case, do 5 bits at a time.  The potential drawback is that
 * a table of 2**5 intermediate results is computed.
 */
#define FIVEARY_CUTOFF 8

#define ABS(x) ((x) < 0 ? -(x) : (x))

#undef MIN
#undef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

#define SIGCHECK(PyTryBlock)                            \
    do {                                                \
        if (--_Py_Ticker < 0) {                         \
            _Py_Ticker = _Py_CheckInterval;             \
            if (PyErr_CheckSignals()) PyTryBlock        \
                                          }             \
    } while(0)

/* Normalize (remove leading zeros from) a long int object.
   Doesn't attempt to free the storage--in most cases, due to the nature
   of the algorithms used, this could save at most be one word anyway. */

static PyLongObject *
long_normalize(register PyLongObject *v)
{
    Py_ssize_t j = ABS(Py_SIZE(v));
    Py_ssize_t i = j;

    while (i > 0 && v->ob_digit[i-1] == 0)
        --i;
    if (i != j)
        Py_SIZE(v) = (Py_SIZE(v) < 0) ? -(i) : i;
    return v;
}

/* Allocate a new long int object with size digits.
   Return NULL and set exception if we run out of memory. */

#define MAX_LONG_DIGITS \
    ((PY_SSIZE_T_MAX - offsetof(PyLongObject, ob_digit))/sizeof(digit))

PyLongObject *
_PyLong_New(Py_ssize_t size)
{
    if (size > (Py_ssize_t)MAX_LONG_DIGITS) {
        PyErr_SetString(PyExc_OverflowError,
                        "too many digits in integer");
        return NULL;
    }
    /* coverity[ampersand_in_size] */
    /* XXX(nnorwitz): PyObject_NEW_VAR / _PyObject_VAR_SIZE need to detect
       overflow */
    return PyObject_NEW_VAR(PyLongObject, &PyLong_Type, size);
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
    result = _PyLong_New(i);
    if (result != NULL) {
        Py_SIZE(result) = Py_SIZE(src);
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
    unsigned long abs_ival;
    unsigned long t;  /* unsigned so >> doesn't propagate sign bit */
    int ndigits = 0;
    int negative = 0;

    if (ival < 0) {
        /* if LONG_MIN == -LONG_MAX-1 (true on most platforms) then
           ANSI C says that the result of -ival is undefined when ival
           == LONG_MIN.  Hence the following workaround. */
        abs_ival = (unsigned long)(-1-ival) + 1;
        negative = 1;
    }
    else {
        abs_ival = (unsigned long)ival;
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

/* Get a C long int from a Python long or Python int object.
   On overflow, returns -1 and sets *overflow to 1 or -1 depending
   on the sign of the result.  Otherwise *overflow is 0.

   For other errors (e.g., type error), returns -1 and sets an error
   condition.
*/

long
PyLong_AsLongAndOverflow(PyObject *vv, int *overflow)
{
    /* This version by Tim Peters */
    register PyLongObject *v;
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

    if(PyInt_Check(vv))
        return PyInt_AsLong(vv);

    if (!PyLong_Check(vv)) {
        PyNumberMethods *nb;
        nb = vv->ob_type->tp_as_number;
        if (nb == NULL || nb->nb_int == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "an integer is required");
            return -1;
        }
        vv = (*nb->nb_int) (vv);
        if (vv == NULL)
            return -1;
        do_decref = 1;
        if(PyInt_Check(vv)) {
            res = PyInt_AsLong(vv);
            goto exit;
        }
        if (!PyLong_Check(vv)) {
            Py_DECREF(vv);
            PyErr_SetString(PyExc_TypeError,
                            "nb_int should return int object");
            return -1;
        }
    }

    res = -1;
    v = (PyLongObject *)vv;
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
        Py_DECREF(vv);
    }
    return res;
}

/* Get a C long int from a long int object.
   Returns -1 and sets an error condition if overflow occurs. */

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

/* Get a C int from a long int object or any object that has an __int__
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

/* Get a Py_ssize_t from a long int object.
   Returns -1 and sets an error condition if overflow occurs. */

Py_ssize_t
PyLong_AsSsize_t(PyObject *vv) {
    register PyLongObject *v;
    size_t x, prev;
    Py_ssize_t i;
    int sign;

    if (vv == NULL || !PyLong_Check(vv)) {
        PyErr_BadInternalCall();
        return -1;
    }
    v = (PyLongObject *)vv;
    i = Py_SIZE(v);
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
                    "long int too large to convert to int");
    return -1;
}

/* Get a C unsigned long int from a long int object.
   Returns -1 and sets an error condition if overflow occurs. */

unsigned long
PyLong_AsUnsignedLong(PyObject *vv)
{
    register PyLongObject *v;
    unsigned long x, prev;
    Py_ssize_t i;

    if (vv == NULL || !PyLong_Check(vv)) {
        if (vv != NULL && PyInt_Check(vv)) {
            long val = PyInt_AsLong(vv);
            if (val < 0) {
                PyErr_SetString(PyExc_OverflowError,
                                "can't convert negative value "
                                "to unsigned long");
                return (unsigned long) -1;
            }
            return val;
        }
        PyErr_BadInternalCall();
        return (unsigned long) -1;
    }
    v = (PyLongObject *)vv;
    i = Py_SIZE(v);
    x = 0;
    if (i < 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "can't convert negative value to unsigned long");
        return (unsigned long) -1;
    }
    while (--i >= 0) {
        prev = x;
        x = (x << PyLong_SHIFT) | v->ob_digit[i];
        if ((x >> PyLong_SHIFT) != prev) {
            PyErr_SetString(PyExc_OverflowError,
                            "long int too large to convert");
            return (unsigned long) -1;
        }
    }
    return x;
}

/* Get a C unsigned long int from a long int object, ignoring the high bits.
   Returns -1 and sets an error condition if an error occurs. */

unsigned long
PyLong_AsUnsignedLongMask(PyObject *vv)
{
    register PyLongObject *v;
    unsigned long x;
    Py_ssize_t i;
    int sign;

    if (vv == NULL || !PyLong_Check(vv)) {
        if (vv != NULL && PyInt_Check(vv))
            return PyInt_AsUnsignedLongMask(vv);
        PyErr_BadInternalCall();
        return (unsigned long) -1;
    }
    v = (PyLongObject *)vv;
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

int
_PyLong_Sign(PyObject *vv)
{
    PyLongObject *v = (PyLongObject *)vv;

    assert(v != NULL);
    assert(PyLong_Check(v));

    return Py_SIZE(v) == 0 ? 0 : (Py_SIZE(v) < 0 ? -1 : 1);
}

size_t
_PyLong_NumBits(PyObject *vv)
{
    PyLongObject *v = (PyLongObject *)vv;
    size_t result = 0;
    Py_ssize_t ndigits;

    assert(v != NULL);
    assert(PyLong_Check(v));
    ndigits = ABS(Py_SIZE(v));
    assert(ndigits == 0 || v->ob_digit[ndigits - 1] != 0);
    if (ndigits > 0) {
        digit msd = v->ob_digit[ndigits - 1];

        result = (ndigits - 1) * PyLong_SHIFT;
        if (result / PyLong_SHIFT != (size_t)(ndigits - 1))
            goto Overflow;
        do {
            ++result;
            if (result == 0)
                goto Overflow;
            msd >>= 1;
        } while (msd);
    }
    return result;

  Overflow:
    PyErr_SetString(PyExc_OverflowError, "long has too many bits "
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
    Py_ssize_t ndigits;                 /* number of Python long digits */
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

    /* How many Python long digits do we need?  We have
       8*numsignificantbytes bits, and each Python long digit has
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
            accum |= (twodigits)thisbyte << accumbits;
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
                            "can't convert negative long to unsigned");
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
       exactly PyLong_SHIFT bits to the total, so first assert that the long is
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

/* Create a new long (or int) object from a C pointer */

PyObject *
PyLong_FromVoidPtr(void *p)
{
#if SIZEOF_VOID_P <= SIZEOF_LONG
    if ((long)p < 0)
        return PyLong_FromUnsignedLong((unsigned long)p);
    return PyInt_FromLong((long)p);
#else

#ifndef HAVE_LONG_LONG
#   error "PyLong_FromVoidPtr: sizeof(void*) > sizeof(long), but no long long"
#endif
#if SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_FromVoidPtr: sizeof(PY_LONG_LONG) < sizeof(void*)"
#endif
    /* optimize null pointers */
    if (p == NULL)
        return PyInt_FromLong(0);
    return PyLong_FromUnsignedLongLong((unsigned PY_LONG_LONG)p);

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
    else if (PyLong_Check(vv) && _PyLong_Sign(vv) < 0)
        x = PyLong_AsLong(vv);
    else
        x = PyLong_AsUnsignedLong(vv);
#else

#ifndef HAVE_LONG_LONG
#   error "PyLong_AsVoidPtr: sizeof(void*) > sizeof(long), but no long long"
#endif
#if SIZEOF_LONG_LONG < SIZEOF_VOID_P
#   error "PyLong_AsVoidPtr: sizeof(PY_LONG_LONG) < sizeof(void*)"
#endif
    PY_LONG_LONG x;

    if (PyInt_Check(vv))
        x = PyInt_AS_LONG(vv);
    else if (PyLong_Check(vv) && _PyLong_Sign(vv) < 0)
        x = PyLong_AsLongLong(vv);
    else
        x = PyLong_AsUnsignedLongLong(vv);

#endif /* SIZEOF_VOID_P <= SIZEOF_LONG */

    if (x == -1 && PyErr_Occurred())
        return NULL;
    return (void *)x;
}

#ifdef HAVE_LONG_LONG

/* Initial PY_LONG_LONG support by Chris Herborth (chrish@qnx.com), later
 * rewritten to use the newer PyLong_{As,From}ByteArray API.
 */

#define IS_LITTLE_ENDIAN (int)*(unsigned char*)&one
#define PY_ABS_LLONG_MIN (0-(unsigned PY_LONG_LONG)PY_LLONG_MIN)

/* Create a new long int object from a C PY_LONG_LONG int. */

PyObject *
PyLong_FromLongLong(PY_LONG_LONG ival)
{
    PyLongObject *v;
    unsigned PY_LONG_LONG abs_ival;
    unsigned PY_LONG_LONG t;  /* unsigned so >> doesn't propagate sign bit */
    int ndigits = 0;
    int negative = 0;

    if (ival < 0) {
        /* avoid signed overflow on negation;  see comments
           in PyLong_FromLong above. */
        abs_ival = (unsigned PY_LONG_LONG)(-1-ival) + 1;
        negative = 1;
    }
    else {
        abs_ival = (unsigned PY_LONG_LONG)ival;
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

/* Create a new long int object from a C unsigned PY_LONG_LONG int. */

PyObject *
PyLong_FromUnsignedLongLong(unsigned PY_LONG_LONG ival)
{
    PyLongObject *v;
    unsigned PY_LONG_LONG t;
    int ndigits = 0;

    /* Count the number of Python digits. */
    t = (unsigned PY_LONG_LONG)ival;
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

/* Create a new long int object from a C Py_ssize_t. */

PyObject *
PyLong_FromSsize_t(Py_ssize_t ival)
{
    Py_ssize_t bytes = ival;
    int one = 1;
    return _PyLong_FromByteArray((unsigned char *)&bytes,
                                 SIZEOF_SIZE_T, IS_LITTLE_ENDIAN, 1);
}

/* Create a new long int object from a C size_t. */

PyObject *
PyLong_FromSize_t(size_t ival)
{
    size_t bytes = ival;
    int one = 1;
    return _PyLong_FromByteArray((unsigned char *)&bytes,
                                 SIZEOF_SIZE_T, IS_LITTLE_ENDIAN, 0);
}

/* Get a C PY_LONG_LONG int from a long int object.
   Return -1 and set an error if overflow occurs. */

PY_LONG_LONG
PyLong_AsLongLong(PyObject *vv)
{
    PY_LONG_LONG bytes;
    int one = 1;
    int res;

    if (vv == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (!PyLong_Check(vv)) {
        PyNumberMethods *nb;
        PyObject *io;
        if (PyInt_Check(vv))
            return (PY_LONG_LONG)PyInt_AsLong(vv);
        if ((nb = Py_TYPE(vv)->tp_as_number) == NULL ||
            nb->nb_int == NULL) {
            PyErr_SetString(PyExc_TypeError, "an integer is required");
            return -1;
        }
        io = (*nb->nb_int) (vv);
        if (io == NULL)
            return -1;
        if (PyInt_Check(io)) {
            bytes = PyInt_AsLong(io);
            Py_DECREF(io);
            return bytes;
        }
        if (PyLong_Check(io)) {
            bytes = PyLong_AsLongLong(io);
            Py_DECREF(io);
            return bytes;
        }
        Py_DECREF(io);
        PyErr_SetString(PyExc_TypeError, "integer conversion failed");
        return -1;
    }

    res = _PyLong_AsByteArray((PyLongObject *)vv, (unsigned char *)&bytes,
                              SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 1);

    /* Plan 9 can't handle PY_LONG_LONG in ? : expressions */
    if (res < 0)
        return (PY_LONG_LONG)-1;
    else
        return bytes;
}

/* Get a C unsigned PY_LONG_LONG int from a long int object.
   Return -1 and set an error if overflow occurs. */

unsigned PY_LONG_LONG
PyLong_AsUnsignedLongLong(PyObject *vv)
{
    unsigned PY_LONG_LONG bytes;
    int one = 1;
    int res;

    if (vv == NULL || !PyLong_Check(vv)) {
        PyErr_BadInternalCall();
        return (unsigned PY_LONG_LONG)-1;
    }

    res = _PyLong_AsByteArray((PyLongObject *)vv, (unsigned char *)&bytes,
                              SIZEOF_LONG_LONG, IS_LITTLE_ENDIAN, 0);

    /* Plan 9 can't handle PY_LONG_LONG in ? : expressions */
    if (res < 0)
        return (unsigned PY_LONG_LONG)res;
    else
        return bytes;
}

/* Get a C unsigned long int from a long int object, ignoring the high bits.
   Returns -1 and sets an error condition if an error occurs. */

unsigned PY_LONG_LONG
PyLong_AsUnsignedLongLongMask(PyObject *vv)
{
    register PyLongObject *v;
    unsigned PY_LONG_LONG x;
    Py_ssize_t i;
    int sign;

    if (vv == NULL || !PyLong_Check(vv)) {
        PyErr_BadInternalCall();
        return (unsigned PY_LONG_LONG) -1;
    }
    v = (PyLongObject *)vv;
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

/* Get a C long long int from a Python long or Python int object.
   On overflow, returns -1 and sets *overflow to 1 or -1 depending
   on the sign of the result.  Otherwise *overflow is 0.

   For other errors (e.g., type error), returns -1 and sets an error
   condition.
*/

PY_LONG_LONG
PyLong_AsLongLongAndOverflow(PyObject *vv, int *overflow)
{
    /* This version by Tim Peters */
    register PyLongObject *v;
    unsigned PY_LONG_LONG x, prev;
    PY_LONG_LONG res;
    Py_ssize_t i;
    int sign;
    int do_decref = 0; /* if nb_int was called */

    *overflow = 0;
    if (vv == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }

    if (PyInt_Check(vv))
        return PyInt_AsLong(vv);

    if (!PyLong_Check(vv)) {
        PyNumberMethods *nb;
        nb = Py_TYPE(vv)->tp_as_number;
        if (nb == NULL || nb->nb_int == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "an integer is required");
            return -1;
        }
        vv = (*nb->nb_int) (vv);
        if (vv == NULL)
            return -1;
        do_decref = 1;
        if(PyInt_Check(vv)) {
            res = PyInt_AsLong(vv);
            goto exit;
        }
        if (!PyLong_Check(vv)) {
            Py_DECREF(vv);
            PyErr_SetString(PyExc_TypeError,
                            "nb_int should return int object");
            return -1;
        }
    }

    res = -1;
    v = (PyLongObject *)vv;
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
        if (x <= (unsigned PY_LONG_LONG)PY_LLONG_MAX) {
            res = (PY_LONG_LONG)x * sign;
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
        Py_DECREF(vv);
    }
    return res;
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

#define CONVERT_BINOP(v, w, a, b)               \
    do {                                        \
        if (!convert_binop(v, w, a, b)) {       \
            Py_INCREF(Py_NotImplemented);       \
            return Py_NotImplemented;           \
        }                                       \
    } while(0)                                  \

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
   _PyLong_Format, but that should be done with great care since longs are
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

/* Divide a long integer by a digit, returning both the quotient
   (as function result) and the remainder (through *prem).
   The sign of a is ignored; n should not be zero. */

static PyLongObject *
divrem1(PyLongObject *a, digit n, digit *prem)
{
    const Py_ssize_t size = ABS(Py_SIZE(a));
    PyLongObject *z;

    assert(n > 0 && n <= PyLong_MASK);
    z = _PyLong_New(size);
    if (z == NULL)
        return NULL;
    *prem = inplace_divrem1(z->ob_digit, a->ob_digit, size, n);
    return long_normalize(z);
}

/* Convert a long integer to a base 10 string.  Returns a new non-shared
   string.  (Return value is non-shared so that callers can modify the
   returned value if necessary.) */

static PyObject *
long_to_decimal_string(PyObject *aa, int addL)
{
    PyLongObject *scratch, *a;
    PyObject *str;
    Py_ssize_t size, strlen, size_a, i, j;
    digit *pout, *pin, rem, tenpow;
    char *p;
    int negative;

    a = (PyLongObject *)aa;
    if (a == NULL || !PyLong_Check(a)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    size_a = ABS(Py_SIZE(a));
    negative = Py_SIZE(a) < 0;

    /* quick and dirty upper bound for the number of digits
       required to express a in base _PyLong_DECIMAL_BASE:

         #digits = 1 + floor(log2(a) / log2(_PyLong_DECIMAL_BASE))

       But log2(a) < size_a * PyLong_SHIFT, and
       log2(_PyLong_DECIMAL_BASE) = log2(10) * _PyLong_DECIMAL_SHIFT
                                  > 3 * _PyLong_DECIMAL_SHIFT
    */
    if (size_a > PY_SSIZE_T_MAX / PyLong_SHIFT) {
        PyErr_SetString(PyExc_OverflowError,
                        "long is too large to format");
        return NULL;
    }
    /* the expression size_a * PyLong_SHIFT is now safe from overflow */
    size = 1 + size_a * PyLong_SHIFT / (3 * _PyLong_DECIMAL_SHIFT);
    scratch = _PyLong_New(size);
    if (scratch == NULL)
        return NULL;

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
                return NULL;
            });
    }
    /* pout should have at least one digit, so that the case when a = 0
       works correctly */
    if (size == 0)
        pout[size++] = 0;

    /* calculate exact length of output string, and allocate */
    strlen = (addL != 0) + negative +
        1 + (size - 1) * _PyLong_DECIMAL_SHIFT;
    tenpow = 10;
    rem = pout[size-1];
    while (rem >= tenpow) {
        tenpow *= 10;
        strlen++;
    }
    str = PyString_FromStringAndSize(NULL, strlen);
    if (str == NULL) {
        Py_DECREF(scratch);
        return NULL;
    }

    /* fill the string right-to-left */
    p = PyString_AS_STRING(str) + strlen;
    *p = '\0';
    if (addL)
        *--p = 'L';
    /* pout[0] through pout[size-2] contribute exactly
       _PyLong_DECIMAL_SHIFT digits each */
    for (i=0; i < size - 1; i++) {
        rem = pout[i];
        for (j = 0; j < _PyLong_DECIMAL_SHIFT; j++) {
            *--p = '0' + rem % 10;
            rem /= 10;
        }
    }
    /* pout[size-1]: always produce at least one decimal digit */
    rem = pout[i];
    do {
        *--p = '0' + rem % 10;
        rem /= 10;
    } while (rem != 0);

    /* and sign */
    if (negative)
        *--p = '-';

    /* check we've counted correctly */
    assert(p == PyString_AS_STRING(str));
    Py_DECREF(scratch);
    return (PyObject *)str;
}

/* Convert the long to a string object with given base,
   appending a base prefix of 0[box] if base is 2, 8 or 16.
   Add a trailing "L" if addL is non-zero.
   If newstyle is zero, then use the pre-2.6 behavior of octal having
   a leading "0", instead of the prefix "0o" */
PyAPI_FUNC(PyObject *)
_PyLong_Format(PyObject *aa, int base, int addL, int newstyle)
{
    register PyLongObject *a = (PyLongObject *)aa;
    PyStringObject *str;
    Py_ssize_t i, sz;
    Py_ssize_t size_a;
    char *p;
    int bits;
    char sign = '\0';

    if (base == 10)
        return long_to_decimal_string((PyObject *)a, addL);

    if (a == NULL || !PyLong_Check(a)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    assert(base >= 2 && base <= 36);
    size_a = ABS(Py_SIZE(a));

    /* Compute a rough upper bound for the length of the string */
    i = base;
    bits = 0;
    while (i > 1) {
        ++bits;
        i >>= 1;
    }
    i = 5 + (addL ? 1 : 0);
    /* ensure we don't get signed overflow in sz calculation */
    if (size_a > (PY_SSIZE_T_MAX - i) / PyLong_SHIFT) {
        PyErr_SetString(PyExc_OverflowError,
                        "long is too large to format");
        return NULL;
    }
    sz = i + 1 + (size_a * PyLong_SHIFT - 1) / bits;
    assert(sz >= 0);
    str = (PyStringObject *) PyString_FromStringAndSize((char *)0, sz);
    if (str == NULL)
        return NULL;
    p = PyString_AS_STRING(str) + sz;
    *p = '\0';
    if (addL)
        *--p = 'L';
    if (Py_SIZE(a) < 0)
        sign = '-';

    if (Py_SIZE(a) == 0) {
        *--p = '0';
    }
    else if ((base & (base - 1)) == 0) {
        /* JRH: special case for power-of-2 bases */
        twodigits accum = 0;
        int accumbits = 0;              /* # of bits in accum */
        int basebits = 1;               /* # of bits in base-1 */
        i = base;
        while ((i >>= 1) > 1)
            ++basebits;

        for (i = 0; i < size_a; ++i) {
            accum |= (twodigits)a->ob_digit[i] << accumbits;
            accumbits += PyLong_SHIFT;
            assert(accumbits >= basebits);
            do {
                char cdigit = (char)(accum & (base - 1));
                cdigit += (cdigit < 10) ? '0' : 'a'-10;
                assert(p > PyString_AS_STRING(str));
                *--p = cdigit;
                accumbits -= basebits;
                accum >>= basebits;
            } while (i < size_a-1 ? accumbits >= basebits : accum > 0);
        }
    }
    else {
        /* Not 0, and base not a power of 2.  Divide repeatedly by
           base, but for speed use the highest power of base that
           fits in a digit. */
        Py_ssize_t size = size_a;
        digit *pin = a->ob_digit;
        PyLongObject *scratch;
        /* powbasw <- largest power of base that fits in a digit. */
        digit powbase = base;  /* powbase == base ** power */
        int power = 1;
        for (;;) {
            twodigits newpow = powbase * (twodigits)base;
            if (newpow >> PyLong_SHIFT)
                /* doesn't fit in a digit */
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
                });

            /* Break rem into digits. */
            assert(ntostore > 0);
            do {
                digit nextrem = (digit)(rem / base);
                char c = (char)(rem - nextrem * base);
                assert(p > PyString_AS_STRING(str));
                c += (c < 10) ? '0' : 'a'-10;
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

    if (base == 2) {
        *--p = 'b';
        *--p = '0';
    }
    else if (base == 8) {
        if (newstyle) {
            *--p = 'o';
            *--p = '0';
        }
        else
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
                         (Py_ssize_t) (q - PyString_AS_STRING(str)));
    }
    return (PyObject *)str;
}

/* Table of digit values for 8-bit string -> integer conversion.
 * '0' maps to 0, ..., '9' maps to 9.
 * 'a' and 'A' map to 10, ..., 'z' and 'Z' map to 35.
 * All other indices map to 37.
 * Note that when converting a base B string, a char c is a legitimate
 * base B digit iff _PyLong_DigitValue[Py_CHARMASK(c)] < B.
 */
int _PyLong_DigitValue[256] = {
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
 * non-digit (which may be *str!).  A normalized long is returned.
 * The point to this routine is that it takes time linear in the number of
 * string characters.
 */
static PyLongObject *
long_from_binary_base(char **str, int base)
{
    char *p = *str;
    char *start = p;
    int bits_per_char;
    Py_ssize_t n;
    PyLongObject *z;
    twodigits accum;
    int bits_in_accum;
    digit *pdigit;

    assert(base >= 2 && base <= 32 && (base & (base - 1)) == 0);
    n = base;
    for (bits_per_char = -1; n; ++bits_per_char)
        n >>= 1;
    /* n <- total # of bits needed, while setting p to end-of-string */
    while (_PyLong_DigitValue[Py_CHARMASK(*p)] < base)
        ++p;
    *str = p;
    /* n <- # of Python digits needed, = ceiling(n/PyLong_SHIFT). */
    n = (p - start) * bits_per_char + PyLong_SHIFT - 1;
    if (n / bits_per_char < p - start) {
        PyErr_SetString(PyExc_ValueError,
                        "long string too large to convert");
        return NULL;
    }
    n = n / PyLong_SHIFT;
    z = _PyLong_New(n);
    if (z == NULL)
        return NULL;
    /* Read string from right, and fill in long from left; i.e.,
     * from least to most significant in both.
     */
    accum = 0;
    bits_in_accum = 0;
    pdigit = z->ob_digit;
    while (--p >= start) {
        int k = _PyLong_DigitValue[Py_CHARMASK(*p)];
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
    return long_normalize(z);
}

PyObject *
PyLong_FromString(char *str, char **pend, int base)
{
    int sign = 1;
    char *start, *orig_str = str;
    PyLongObject *z;
    PyObject *strobj, *strrepr;
    Py_ssize_t slen;

    if ((base != 0 && base < 2) || base > 36) {
        PyErr_SetString(PyExc_ValueError,
                        "long() base must be >= 2 and <= 36, or 0");
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
        /* No base given.  Deduce the base from the contents
           of the string */
        if (str[0] != '0')
            base = 10;
        else if (str[1] == 'x' || str[1] == 'X')
            base = 16;
        else if (str[1] == 'o' || str[1] == 'O')
            base = 8;
        else if (str[1] == 'b' || str[1] == 'B')
            base = 2;
        else
            /* "old" (C-style) octal literal, still valid in
               2.x, although illegal in 3.x */
            base = 8;
    }
    /* Whether or not we were deducing the base, skip leading chars
       as needed */
    if (str[0] == '0' &&
        ((base == 16 && (str[1] == 'x' || str[1] == 'X')) ||
         (base == 8  && (str[1] == 'o' || str[1] == 'O')) ||
         (base == 2  && (str[1] == 'b' || str[1] == 'B'))))
        str += 2;

    start = str;
    if ((base & (base - 1)) == 0)
        z = long_from_binary_base(&str, base);
    else {
/***
Binary bases can be converted in time linear in the number of digits, because
Python's representation base is binary.  Other bases (including decimal!) use
the simple quadratic-time algorithm below, complicated by some speed tricks.

First some math:  the largest integer that can be expressed in N base-B digits
is B**N-1.  Consequently, if we have an N-digit input in base B, the worst-
case number of Python digits needed to hold it is the smallest integer n s.t.

    PyLong_BASE**n-1 >= B**N-1  [or, adding 1 to both sides]
    PyLong_BASE**n >= B**N      [taking logs to base PyLong_BASE]
    n >= log(B**N)/log(PyLong_BASE) = N * log(B)/log(PyLong_BASE)

The static array log_base_PyLong_BASE[base] == log(base)/log(PyLong_BASE) so
we can compute this quickly.  A Python long with that much space is reserved
near the start, and the result is computed into it.

The input string is actually treated as being in base base**i (i.e., i digits
are processed at a time), where two more static arrays hold:

    convwidth_base[base] = the largest integer i such that
                           base**i <= PyLong_BASE
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

    n >= N * log(B)/log(PyLong_BASE)

where `N` is the number of input digits in base `B`.  This is computed via

    size_z = (Py_ssize_t)((scan - str) * log_base_PyLong_BASE[base]) + 1;

below.  Two numeric concerns are how much space this can waste, and whether
the computed result can be too small.  To be concrete, assume PyLong_BASE =
2**15, which is the default (and it's unlikely anyone changes that).

Waste isn't a problem: provided the first input digit isn't 0, the difference
between the worst-case input with N digits and the smallest input with N
digits is about a factor of B, but B is small compared to PyLong_BASE so at
most one allocated Python digit can remain unused on that count.  If
N*log(B)/log(PyLong_BASE) is mathematically an exact integer, then truncating
that and adding 1 returns a result 1 larger than necessary.  However, that
can't happen: whenever B is a power of 2, long_from_binary_base() is called
instead, and it's impossible for B**i to be an integer power of 2**15 when B
is not a power of 2 (i.e., it's impossible for N*log(B)/log(PyLong_BASE) to be
an exact integer when B is not a power of 2, since B**i has a prime factor
other than 2 in that case, but (2**15)**j's only prime factor is 2).

The computed result can be too small if the true value of
N*log(B)/log(PyLong_BASE) is a little bit larger than an exact integer, but
due to roundoff errors (in computing log(B), log(PyLong_BASE), their quotient,
and/or multiplying that by N) yields a numeric result a little less than that
integer.  Unfortunately, "how close can a transcendental function get to an
integer over some range?"  questions are generally theoretically intractable.
Computer analysis via continued fractions is practical: expand
log(B)/log(PyLong_BASE) via continued fractions, giving a sequence i/j of "the
best" rational approximations.  Then j*log(B)/log(PyLong_BASE) is
approximately equal to (the integer) i.  This shows that we can get very close
to being in trouble, but very rarely.  For example, 76573 is a denominator in
one of the continued-fraction approximations to log(10)/log(2**15), and
indeed:

    >>> log(10)/log(2**15)*76573
    16958.000000654003

is very close to an integer.  If we were working with IEEE single-precision,
rounding errors could kill us.  Finding worst cases in IEEE double-precision
requires better-than-double-precision log() functions, and Tim didn't bother.
Instead the code checks to see whether the allocated space is enough as each
new Python digit is added, and copies the whole thing to a larger long if not.
This should happen extremely rarely, and in fact I don't have a test case
that triggers it(!).  Instead the code was tested by artificially allocating
just 1 digit at the start, so that the copying code was exercised for every
digit beyond the first.
***/
        register twodigits c;           /* current input character */
        Py_ssize_t size_z;
        int i;
        int convwidth;
        twodigits convmultmax, convmult;
        digit *pz, *pzstop;
        char* scan;

        static double log_base_PyLong_BASE[37] = {0.0e0,};
        static int convwidth_base[37] = {0,};
        static twodigits convmultmax_base[37] = {0,};

        if (log_base_PyLong_BASE[base] == 0.0) {
            twodigits convmax = base;
            int i = 1;

            log_base_PyLong_BASE[base] = (log((double)base) /
                                          log((double)PyLong_BASE));
            for (;;) {
                twodigits next = convmax * base;
                if (next > PyLong_BASE)
                    break;
                convmax = next;
                ++i;
            }
            convmultmax_base[base] = convmax;
            assert(i > 0);
            convwidth_base[base] = i;
        }

        /* Find length of the string of numeric characters. */
        scan = str;
        while (_PyLong_DigitValue[Py_CHARMASK(*scan)] < base)
            ++scan;

        /* Create a long object that can contain the largest possible
         * integer with this base and length.  Note that there's no
         * need to initialize z->ob_digit -- no slot is read up before
         * being stored into.
         */
        size_z = (Py_ssize_t)((scan - str) * log_base_PyLong_BASE[base]) + 1;
        /* Uncomment next line to test exceedingly rare copy code */
        /* size_z = 1; */
        assert(size_z > 0);
        z = _PyLong_New(size_z);
        if (z == NULL)
            return NULL;
        Py_SIZE(z) = 0;

        /* `convwidth` consecutive input digits are treated as a single
         * digit in base `convmultmax`.
         */
        convwidth = convwidth_base[base];
        convmultmax = convmultmax_base[base];

        /* Work ;-) */
        while (str < scan) {
            /* grab up to convwidth digits from the input string */
            c = (digit)_PyLong_DigitValue[Py_CHARMASK(*str++)];
            for (i = 1; i < convwidth && str != scan; ++i, ++str) {
                c = (twodigits)(c *  base +
                                _PyLong_DigitValue[Py_CHARMASK(*str)]);
                assert(c < PyLong_BASE);
            }

            convmult = convmultmax;
            /* Calculate the shift only if we couldn't get
             * convwidth digits.
             */
            if (i != convwidth) {
                convmult = base;
                for ( ; i > 1; --i)
                    convmult *= base;
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
    if (z == NULL)
        return NULL;
    if (str == start)
        goto onError;
    if (sign < 0)
        Py_SIZE(z) = -(Py_SIZE(z));
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
    Py_XDECREF(z);
    slen = strlen(orig_str) < 200 ? strlen(orig_str) : 200;
    strobj = PyString_FromStringAndSize(orig_str, slen);
    if (strobj == NULL)
        return NULL;
    strrepr = PyObject_Repr(strobj);
    Py_DECREF(strobj);
    if (strrepr == NULL)
        return NULL;
    PyErr_Format(PyExc_ValueError,
                 "invalid literal for long() with base %d: %s",
                 base, PyString_AS_STRING(strrepr));
    Py_DECREF(strrepr);
    return NULL;
}

#ifdef Py_USING_UNICODE
PyObject *
PyLong_FromUnicode(Py_UNICODE *u, Py_ssize_t length, int base)
{
    PyObject *result;
    char *buffer = (char *)PyMem_MALLOC(length+1);

    if (buffer == NULL)
        return NULL;

    if (PyUnicode_EncodeDecimal(u, length, buffer, NULL)) {
        PyMem_FREE(buffer);
        return NULL;
    }
    result = PyLong_FromString(buffer, NULL, base);
    PyMem_FREE(buffer);
    return result;
}
#endif

/* forward */
static PyLongObject *x_divrem
    (PyLongObject *, PyLongObject *, PyLongObject **);
static PyObject *long_long(PyObject *v);

/* Long division with remainder, top-level routine */

static int
long_divrem(PyLongObject *a, PyLongObject *b,
            PyLongObject **pdiv, PyLongObject **prem)
{
    Py_ssize_t size_a = ABS(Py_SIZE(a)), size_b = ABS(Py_SIZE(b));
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
        if (*pdiv == NULL)
            return -1;
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
    if ((Py_SIZE(a) < 0) != (Py_SIZE(b) < 0))
        Py_SIZE(z) = -(Py_SIZE(z));
    if (Py_SIZE(a) < 0 && Py_SIZE(*prem) != 0)
        Py_SIZE(*prem) = -Py_SIZE(*prem);
    *pdiv = z;
    return 0;
}

/* Unsigned long division with remainder -- the algorithm.  The arguments v1
   and w1 should satisfy 2 <= ABS(Py_SIZE(w1)) <= ABS(Py_SIZE(v1)). */

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
    size_v = ABS(Py_SIZE(v1));
    size_w = ABS(Py_SIZE(w1));
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

    a_size = ABS(Py_SIZE(a));
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
    assert(1 <= x_size &&
           x_size <= (Py_ssize_t)(sizeof(x_digits)/sizeof(digit)));

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

/* Get a C double from a long int object.  Rounds to the nearest double,
   using the round-half-to-even rule in the case of a tie. */

double
PyLong_AsDouble(PyObject *v)
{
    Py_ssize_t exponent;
    double x;

    if (v == NULL || !PyLong_Check(v)) {
        PyErr_BadInternalCall();
        return -1.0;
    }
    x = _PyLong_Frexp((PyLongObject *)v, &exponent);
    if ((x == -1.0 && PyErr_Occurred()) || exponent > DBL_MAX_EXP) {
        PyErr_SetString(PyExc_OverflowError,
                        "long int too large to convert to float");
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

static PyObject *
long_repr(PyObject *v)
{
    return _PyLong_Format(v, 10, 1, 0);
}

static PyObject *
long_str(PyObject *v)
{
    return _PyLong_Format(v, 10, 0, 0);
}

static int
long_compare(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t sign;

    if (Py_SIZE(a) != Py_SIZE(b)) {
        sign = Py_SIZE(a) - Py_SIZE(b);
    }
    else {
        Py_ssize_t i = ABS(Py_SIZE(a));
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

static long
long_hash(PyLongObject *v)
{
    unsigned long x;
    Py_ssize_t i;
    int sign;

    /* This is designed so that Python ints and longs with the
       same value hash to the same value, otherwise comparisons
       of mapping keys will turn out weird */
    i = Py_SIZE(v);
    sign = 1;
    x = 0;
    if (i < 0) {
        sign = -1;
        i = -(i);
    }
    /* The following loop produces a C unsigned long x such that x is
       congruent to the absolute value of v modulo ULONG_MAX.  The
       resulting x is nonzero if and only if v is. */
    while (--i >= 0) {
        /* Force a native long #-bits (32 or 64) circular shift */
        x = (x >> (8*SIZEOF_LONG-PyLong_SHIFT)) | (x << PyLong_SHIFT);
        x += v->ob_digit[i];
        /* If the addition above overflowed we compensate by
           incrementing.  This preserves the value modulo
           ULONG_MAX. */
        if (x < v->ob_digit[i])
            x++;
    }
    x = x * sign;
    if (x == (unsigned long)-1)
        x = (unsigned long)-2;
    return (long)x;
}


/* Add the absolute values of two long integers. */

static PyLongObject *
x_add(PyLongObject *a, PyLongObject *b)
{
    Py_ssize_t size_a = ABS(Py_SIZE(a)), size_b = ABS(Py_SIZE(b));
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
    Py_ssize_t size_a = ABS(Py_SIZE(a)), size_b = ABS(Py_SIZE(b));
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
    if (sign < 0)
        Py_SIZE(z) = -(Py_SIZE(z));
    return long_normalize(z);
}

static PyObject *
long_add(PyLongObject *v, PyLongObject *w)
{
    PyLongObject *a, *b, *z;

    CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

    if (Py_SIZE(a) < 0) {
        if (Py_SIZE(b) < 0) {
            z = x_add(a, b);
            if (z != NULL && Py_SIZE(z) != 0)
                Py_SIZE(z) = -(Py_SIZE(z));
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
    Py_DECREF(a);
    Py_DECREF(b);
    return (PyObject *)z;
}

static PyObject *
long_sub(PyLongObject *v, PyLongObject *w)
{
    PyLongObject *a, *b, *z;

    CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

    if (Py_SIZE(a) < 0) {
        if (Py_SIZE(b) < 0)
            z = x_sub(a, b);
        else
            z = x_add(a, b);
        if (z != NULL && Py_SIZE(z) != 0)
            Py_SIZE(z) = -(Py_SIZE(z));
    }
    else {
        if (Py_SIZE(b) < 0)
            z = x_add(a, b);
        else
            z = x_sub(a, b);
    }
    Py_DECREF(a);
    Py_DECREF(b);
    return (PyObject *)z;
}

/* Grade school multiplication, ignoring the signs.
 * Returns the absolute value of the product, or NULL if error.
 */
static PyLongObject *
x_mul(PyLongObject *a, PyLongObject *b)
{
    PyLongObject *z;
    Py_ssize_t size_a = ABS(Py_SIZE(a));
    Py_ssize_t size_b = ABS(Py_SIZE(b));
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
    else {      /* a is not the same as b -- gradeschool long mult */
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
   Takes a long "n" and an integer "size" representing the place to
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
    const Py_ssize_t size_n = ABS(Py_SIZE(n));

    size_lo = MIN(size_n, size);
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
    Py_ssize_t asize = ABS(Py_SIZE(a));
    Py_ssize_t bsize = ABS(Py_SIZE(b));
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
            return _PyLong_New(0);
        else
            return x_mul(a, b);
    }

    /* If a is small compared to b, splitting on b gives a degenerate
     * case with ah==0, and Karatsuba may be (even much) less efficient
     * than "grade school" then.  However, we can still win, by viewing
     * b as a string of "big digits", each of width Py_SIZE(a).  That
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
     *    PyLong_BASE**(sizea + sizeb), and so long as the *final* result fits,
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
 * of slices, each with Py_SIZE(a) digits, and multiply the slices by a,
 * one at a time.  This gives k_mul balanced inputs to work with, and is
 * also cache-friendly (we compute one double-width slice of the result
 * at a time, then move on, never backtracking except for the helpful
 * single-width slice overlap between successive partial sums).
 */
static PyLongObject *
k_lopsided_mul(PyLongObject *a, PyLongObject *b)
{
    const Py_ssize_t asize = ABS(Py_SIZE(a));
    Py_ssize_t bsize = ABS(Py_SIZE(b));
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
        const Py_ssize_t nbtouse = MIN(bsize, asize);

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
long_mul(PyLongObject *v, PyLongObject *w)
{
    PyLongObject *a, *b, *z;

    if (!convert_binop((PyObject *)v, (PyObject *)w, &a, &b)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    z = k_mul(a, b);
    /* Negate if exactly one of the inputs is negative. */
    if (((Py_SIZE(a) ^ Py_SIZE(b)) < 0) && z)
        Py_SIZE(z) = -(Py_SIZE(z));
    Py_DECREF(a);
    Py_DECREF(b);
    return (PyObject *)z;
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

    if (long_divrem(v, w, &div, &mod) < 0)
        return -1;
    if ((Py_SIZE(mod) < 0 && Py_SIZE(w) > 0) ||
        (Py_SIZE(mod) > 0 && Py_SIZE(w) < 0)) {
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
long_div(PyObject *v, PyObject *w)
{
    PyLongObject *a, *b, *div;

    CONVERT_BINOP(v, w, &a, &b);
    if (l_divmod(a, b, &div, NULL) < 0)
        div = NULL;
    Py_DECREF(a);
    Py_DECREF(b);
    return (PyObject *)div;
}

static PyObject *
long_classic_div(PyObject *v, PyObject *w)
{
    PyLongObject *a, *b, *div;

    CONVERT_BINOP(v, w, &a, &b);
    if (Py_DivisionWarningFlag &&
        PyErr_Warn(PyExc_DeprecationWarning, "classic long division") < 0)
        div = NULL;
    else if (l_divmod(a, b, &div, NULL) < 0)
        div = NULL;
    Py_DECREF(a);
    Py_DECREF(b);
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

    CONVERT_BINOP(v, w, &a, &b);

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
    a_size = ABS(Py_SIZE(a));
    b_size = ABS(Py_SIZE(b));
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
    shift = MAX(diff, DBL_MIN_EXP) - DBL_MANT_DIG - 2;

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
    x_size = ABS(Py_SIZE(x));
    assert(x_size > 0); /* result of division is never zero */
    x_bits = (x_size-1)*PyLong_SHIFT+bits_in_digit(x->ob_digit[x_size-1]);

    /* The number of extra bits that have to be rounded away. */
    extra_bits = MAX(x_bits, DBL_MIN_EXP - shift) - DBL_MANT_DIG;
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
    Py_DECREF(a);
    Py_DECREF(b);
    return PyFloat_FromDouble(negate ? -result : result);

  underflow_or_zero:
    Py_DECREF(a);
    Py_DECREF(b);
    return PyFloat_FromDouble(negate ? -0.0 : 0.0);

  overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "integer division result too large for a float");
  error:
    Py_DECREF(a);
    Py_DECREF(b);
    return NULL;
}

static PyObject *
long_mod(PyObject *v, PyObject *w)
{
    PyLongObject *a, *b, *mod;

    CONVERT_BINOP(v, w, &a, &b);

    if (l_divmod(a, b, NULL, &mod) < 0)
        mod = NULL;
    Py_DECREF(a);
    Py_DECREF(b);
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
    CONVERT_BINOP(v, w, &a, &b);
    if (PyLong_Check(x)) {
        c = (PyLongObject *)x;
        Py_INCREF(x);
    }
    else if (PyInt_Check(x)) {
        c = (PyLongObject *)PyLong_FromLong(PyInt_AS_LONG(x));
        if (c == NULL)
            goto Error;
    }
    else if (x == Py_None)
        c = NULL;
    else {
        Py_DECREF(a);
        Py_DECREF(b);
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    if (Py_SIZE(b) < 0) {  /* if exponent is negative */
        if (c) {
            PyErr_SetString(PyExc_TypeError, "pow() 2nd argument "
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
            Py_SIZE(c) = - Py_SIZE(c);
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
    if (z != NULL) {
        Py_DECREF(z);
        z = NULL;
    }
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
    PyLongObject *w;
    w = (PyLongObject *)PyLong_FromLong(1L);
    if (w == NULL)
        return NULL;
    x = (PyLongObject *) long_add(v, w);
    Py_DECREF(w);
    if (x == NULL)
        return NULL;
    Py_SIZE(x) = -(Py_SIZE(x));
    return (PyObject *)x;
}

static PyObject *
long_neg(PyLongObject *v)
{
    PyLongObject *z;
    if (Py_SIZE(v) == 0 && PyLong_CheckExact(v)) {
        /* -0 == 0 */
        Py_INCREF(v);
        return (PyObject *) v;
    }
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
long_nonzero(PyLongObject *v)
{
    return Py_SIZE(v) != 0;
}

static PyObject *
long_rshift(PyLongObject *v, PyLongObject *w)
{
    PyLongObject *a, *b;
    PyLongObject *z = NULL;
    Py_ssize_t shiftby, newsize, wordshift, loshift, hishift, i, j;
    digit lomask, himask;

    CONVERT_BINOP((PyObject *)v, (PyObject *)w, &a, &b);

    if (Py_SIZE(a) < 0) {
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
        shiftby = PyLong_AsSsize_t((PyObject *)b);
        if (shiftby == -1L && PyErr_Occurred())
            goto rshift_error;
        if (shiftby < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "negative shift count");
            goto rshift_error;
        }
        wordshift = shiftby / PyLong_SHIFT;
        newsize = ABS(Py_SIZE(a)) - wordshift;
        if (newsize <= 0) {
            z = _PyLong_New(0);
            Py_DECREF(a);
            Py_DECREF(b);
            return (PyObject *)z;
        }
        loshift = shiftby % PyLong_SHIFT;
        hishift = PyLong_SHIFT - loshift;
        lomask = ((digit)1 << hishift) - 1;
        himask = PyLong_MASK ^ lomask;
        z = _PyLong_New(newsize);
        if (z == NULL)
            goto rshift_error;
        if (Py_SIZE(a) < 0)
            Py_SIZE(z) = -(Py_SIZE(z));
        for (i = 0, j = wordshift; i < newsize; i++, j++) {
            z->ob_digit[i] = (a->ob_digit[j] >> loshift) & lomask;
            if (i+1 < newsize)
                z->ob_digit[i] |= (a->ob_digit[j+1] << hishift) & himask;
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
    Py_ssize_t shiftby, oldsize, newsize, wordshift, remshift, i, j;
    twodigits accum;

    CONVERT_BINOP(v, w, &a, &b);

    shiftby = PyLong_AsSsize_t((PyObject *)b);
    if (shiftby == -1L && PyErr_Occurred())
        goto out;
    if (shiftby < 0) {
        PyErr_SetString(PyExc_ValueError, "negative shift count");
        goto out;
    }

    if (Py_SIZE(a) == 0) {
        z = (PyLongObject *)PyLong_FromLong(0);
        goto out;
    }

    /* wordshift, remshift = divmod(shiftby, PyLong_SHIFT) */
    wordshift = shiftby / PyLong_SHIFT;
    remshift  = shiftby - wordshift * PyLong_SHIFT;

    oldsize = ABS(Py_SIZE(a));
    newsize = oldsize + wordshift;
    if (remshift)
        ++newsize;
    z = _PyLong_New(newsize);
    if (z == NULL)
        goto out;
    if (Py_SIZE(a) < 0)
        Py_SIZE(z) = -(Py_SIZE(z));
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
  out:
    Py_DECREF(a);
    Py_DECREF(b);
    return (PyObject *) z;
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
             int op,  /* '&', '|', '^' */
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
    size_a = ABS(Py_SIZE(a));
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
    size_b = ABS(Py_SIZE(b));
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
    return (PyObject *)long_normalize(z);
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
        if (*pw == NULL)
            return -1;
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
long_long(PyObject *v)
{
    if (PyLong_CheckExact(v))
        Py_INCREF(v);
    else
        v = _PyLong_Copy((PyLongObject *)v);
    return v;
}

static PyObject *
long_int(PyObject *v)
{
    long x;
    x = PyLong_AsLong(v);
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            PyErr_Clear();
            if (PyLong_CheckExact(v)) {
                Py_INCREF(v);
                return v;
            }
            else
                return _PyLong_Copy((PyLongObject *)v);
        }
        else
            return NULL;
    }
    return PyInt_FromLong(x);
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
    return _PyLong_Format(v, 8, 1, 0);
}

static PyObject *
long_hex(PyObject *v)
{
    return _PyLong_Format(v, 16, 1, 0);
}

static PyObject *
long_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
long_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *x = NULL;
    int base = -909;                         /* unlikely! */
    static char *kwlist[] = {"x", "base", 0};

    if (type != &PyLong_Type)
        return long_subtype_new(type, args, kwds); /* Wimp out */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi:long", kwlist,
                                     &x, &base))
        return NULL;
    if (x == NULL) {
        if (base != -909) {
            PyErr_SetString(PyExc_TypeError,
                            "long() missing string argument");
            return NULL;
        }
        return PyLong_FromLong(0L);
    }
    if (base == -909)
        return PyNumber_Long(x);
    else if (PyString_Check(x)) {
        /* Since PyLong_FromString doesn't have a length parameter,
         * check here for possible NULs in the string. */
        char *string = PyString_AS_STRING(x);
        if (strlen(string) != (size_t)PyString_Size(x)) {
            /* create a repr() of the input string,
             * just like PyLong_FromString does. */
            PyObject *srepr;
            srepr = PyObject_Repr(x);
            if (srepr == NULL)
                return NULL;
            PyErr_Format(PyExc_ValueError,
                         "invalid literal for long() with base %d: %s",
                         base, PyString_AS_STRING(srepr));
            Py_DECREF(srepr);
            return NULL;
        }
        return PyLong_FromString(PyString_AS_STRING(x), NULL, base);
    }
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
    PyLongObject *tmp, *newobj;
    Py_ssize_t i, n;

    assert(PyType_IsSubtype(type, &PyLong_Type));
    tmp = (PyLongObject *)long_new(&PyLong_Type, args, kwds);
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

static PyObject *
long_getnewargs(PyLongObject *v)
{
    return Py_BuildValue("(N)", _PyLong_Copy(v));
}

static PyObject *
long_get0(PyLongObject *v, void *context) {
    return PyLong_FromLong(0L);
}

static PyObject *
long_get1(PyLongObject *v, void *context) {
    return PyLong_FromLong(1L);
}

static PyObject *
long__format__(PyObject *self, PyObject *args)
{
    PyObject *format_spec;

    if (!PyArg_ParseTuple(args, "O:__format__", &format_spec))
        return NULL;
    if (PyBytes_Check(format_spec))
        return _PyLong_FormatAdvanced(self,
                                      PyBytes_AS_STRING(format_spec),
                                      PyBytes_GET_SIZE(format_spec));
    if (PyUnicode_Check(format_spec)) {
        /* Convert format_spec to a str */
        PyObject *result;
        PyObject *str_spec = PyObject_Str(format_spec);

        if (str_spec == NULL)
            return NULL;

        result = _PyLong_FormatAdvanced(self,
                                        PyBytes_AS_STRING(str_spec),
                                        PyBytes_GET_SIZE(str_spec));

        Py_DECREF(str_spec);
        return result;
    }
    PyErr_SetString(PyExc_TypeError, "__format__ requires str or unicode");
    return NULL;
}

static PyObject *
long_sizeof(PyLongObject *v)
{
    Py_ssize_t res;

    res = Py_TYPE(v)->tp_basicsize + ABS(Py_SIZE(v))*sizeof(digit);
    return PyInt_FromSsize_t(res);
}

static PyObject *
long_bit_length(PyLongObject *v)
{
    PyLongObject *result, *x, *y;
    Py_ssize_t ndigits, msd_bits = 0;
    digit msd;

    assert(v != NULL);
    assert(PyLong_Check(v));

    ndigits = ABS(Py_SIZE(v));
    if (ndigits == 0)
        return PyInt_FromLong(0);

    msd = v->ob_digit[ndigits-1];
    while (msd >= 32) {
        msd_bits += 6;
        msd >>= 6;
    }
    msd_bits += (long)(BitLengthTable[msd]);

    if (ndigits <= PY_SSIZE_T_MAX/PyLong_SHIFT)
        return PyInt_FromSsize_t((ndigits-1)*PyLong_SHIFT + msd_bits);

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

PyDoc_STRVAR(long_bit_length_doc,
"long.bit_length() -> int or long\n\
\n\
Number of bits necessary to represent self in binary.\n\
>>> bin(37L)\n\
'0b100101'\n\
>>> (37L).bit_length()\n\
6");

#if 0
static PyObject *
long_is_finite(PyObject *v)
{
    Py_RETURN_TRUE;
}
#endif

static PyMethodDef long_methods[] = {
    {"conjugate",       (PyCFunction)long_long, METH_NOARGS,
     "Returns self, the complex conjugate of any long."},
    {"bit_length",      (PyCFunction)long_bit_length, METH_NOARGS,
     long_bit_length_doc},
#if 0
    {"is_finite",       (PyCFunction)long_is_finite,    METH_NOARGS,
     "Returns always True."},
#endif
    {"__trunc__",       (PyCFunction)long_long, METH_NOARGS,
     "Truncating an Integral returns itself."},
    {"__getnewargs__",          (PyCFunction)long_getnewargs,   METH_NOARGS},
    {"__format__", (PyCFunction)long__format__, METH_VARARGS},
    {"__sizeof__",      (PyCFunction)long_sizeof, METH_NOARGS,
     "Returns size in memory, in bytes"},
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
"long(x=0) -> long\n\
long(x, base=10) -> long\n\
\n\
Convert a number or string to a long integer, or return 0L if no arguments\n\
are given.  If x is floating point, the conversion truncates towards zero.\n\
\n\
If x is not a number or if base is given, then x must be a string or\n\
Unicode object representing an integer literal in the given base.  The\n\
literal can be preceded by '+' or '-' and be surrounded by whitespace.\n\
The base defaults to 10.  Valid bases are 0 and 2-36.  Base 0 means to\n\
interpret the base from the string as an integer literal.\n\
>>> int('0b100', base=0)\n\
4L");

static PyNumberMethods long_as_number = {
    (binaryfunc)long_add,       /*nb_add*/
    (binaryfunc)long_sub,       /*nb_subtract*/
    (binaryfunc)long_mul,       /*nb_multiply*/
    long_classic_div,           /*nb_divide*/
    long_mod,                   /*nb_remainder*/
    long_divmod,                /*nb_divmod*/
    long_pow,                   /*nb_power*/
    (unaryfunc)long_neg,        /*nb_negative*/
    (unaryfunc)long_long,       /*tp_positive*/
    (unaryfunc)long_abs,        /*tp_absolute*/
    (inquiry)long_nonzero,      /*tp_nonzero*/
    (unaryfunc)long_invert,     /*nb_invert*/
    long_lshift,                /*nb_lshift*/
    (binaryfunc)long_rshift,    /*nb_rshift*/
    long_and,                   /*nb_and*/
    long_xor,                   /*nb_xor*/
    long_or,                    /*nb_or*/
    long_coerce,                /*nb_coerce*/
    long_int,                   /*nb_int*/
    long_long,                  /*nb_long*/
    long_float,                 /*nb_float*/
    long_oct,                   /*nb_oct*/
    long_hex,                   /*nb_hex*/
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_divide */
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
    "long",                                     /* tp_name */
    offsetof(PyLongObject, ob_digit),           /* tp_basicsize */
    sizeof(digit),                              /* tp_itemsize */
    long_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    (cmpfunc)long_compare,                      /* tp_compare */
    long_repr,                                  /* tp_repr */
    &long_as_number,                            /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)long_hash,                        /* tp_hash */
    0,                                          /* tp_call */
    long_str,                                   /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_LONG_SUBCLASS, /* tp_flags */
    long_doc,                                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
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

static PyTypeObject Long_InfoType;

PyDoc_STRVAR(long_info__doc__,
"sys.long_info\n\
\n\
A struct sequence that holds information about Python's\n\
internal representation of integers.  The attributes are read only.");

static PyStructSequence_Field long_info_fields[] = {
    {"bits_per_digit", "size of a digit in bits"},
    {"sizeof_digit", "size in bytes of the C type used to represent a digit"},
    {NULL, NULL}
};

static PyStructSequence_Desc long_info_desc = {
    "sys.long_info",   /* name */
    long_info__doc__,  /* doc */
    long_info_fields,  /* fields */
    2                  /* number of fields */
};

PyObject *
PyLong_GetInfo(void)
{
    PyObject* long_info;
    int field = 0;
    long_info = PyStructSequence_New(&Long_InfoType);
    if (long_info == NULL)
        return NULL;
    PyStructSequence_SET_ITEM(long_info, field++,
                              PyInt_FromLong(PyLong_SHIFT));
    PyStructSequence_SET_ITEM(long_info, field++,
                              PyInt_FromLong(sizeof(digit)));
    if (PyErr_Occurred()) {
        Py_CLEAR(long_info);
        return NULL;
    }
    return long_info;
}

int
_PyLong_Init(void)
{
    /* initialize long_info */
    if (Long_InfoType.tp_name == 0)
        PyStructSequence_InitType(&Long_InfoType, &long_info_desc);
    return 1;
}
