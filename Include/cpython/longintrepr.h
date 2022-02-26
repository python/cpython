#ifndef Py_LIMITED_API
#ifndef Py_LONGINTREPR_H
#define Py_LONGINTREPR_H
#ifdef __cplusplus
extern "C" {
#endif


/* This is published for the benefit of "friends" marshal.c and _decimal.c. */

/* Parameters of the integer representation.  There are two different
   sets of parameters: one set for 30-bit digits, stored in an unsigned 32-bit
   integer type, and one set for 15-bit digits with each digit stored in an
   unsigned short.  The value of PYLONG_BITS_IN_DIGIT, defined either at
   configure time or in pyport.h, is used to decide which digit size to use.

   Type 'digit' should be able to hold 2*PyLong_BASE-1, and type 'twodigits'
   should be an unsigned integer type able to hold all integers up to
   PyLong_BASE*PyLong_BASE-1.  x_sub assumes that 'digit' is an unsigned type,
   and that overflow is handled by taking the result modulo 2**N for some N >
   PyLong_SHIFT.  The majority of the code doesn't care about the precise
   value of PyLong_SHIFT, but there are some notable exceptions:

   - PyLong_{As,From}ByteArray require that PyLong_SHIFT be at least 8

   - long_hash() requires that PyLong_SHIFT is *strictly* less than the number
     of bits in an unsigned long, as do the PyLong <-> long (or unsigned long)
     conversion functions

   - the Python int <-> size_t/Py_ssize_t conversion functions expect that
     PyLong_SHIFT is strictly less than the number of bits in a size_t

   - the marshal code currently expects that PyLong_SHIFT is a multiple of 15

   - NSMALLNEGINTS and NSMALLPOSINTS should be small enough to fit in a single
     digit; with the current values this forces PyLong_SHIFT >= 9

  The values 15 and 30 should fit all of the above requirements, on any
  platform.
*/

#if PYLONG_BITS_IN_DIGIT == 30
typedef uint32_t digit;
typedef int32_t sdigit; /* signed variant of digit */
typedef uint64_t twodigits;
typedef int64_t stwodigits; /* signed variant of twodigits */
#define PyLong_SHIFT    30
#define _PyLong_DECIMAL_SHIFT   9 /* max(e such that 10**e fits in a digit) */
#define _PyLong_DECIMAL_BASE    ((digit)1000000000) /* 10 ** DECIMAL_SHIFT */
#elif PYLONG_BITS_IN_DIGIT == 15
typedef unsigned short digit;
typedef short sdigit; /* signed variant of digit */
typedef unsigned long twodigits;
typedef long stwodigits; /* signed variant of twodigits */
#define PyLong_SHIFT    15
#define _PyLong_DECIMAL_SHIFT   4 /* max(e such that 10**e fits in a digit) */
#define _PyLong_DECIMAL_BASE    ((digit)10000) /* 10 ** DECIMAL_SHIFT */
#else
#error "PYLONG_BITS_IN_DIGIT should be 15 or 30"
#endif
#define PyLong_BASE     ((digit)1 << PyLong_SHIFT)
#define PyLong_MASK     ((digit)(PyLong_BASE - 1))

/* Long Integer Representation
   ---------------------------

   There are two representations of long objects: the inlined
   representation, where the sign and value is stored within the ob_size
   field, and the bignum implementation, where the ob_size stores both the sign
   and number of digits in the ob_digits field.

   To distinguish between either representation, one looks at the least significant
   bit of the ob_size field; if it's set, the value is inlined in that field; if it's
   unset, then it should be treated as the number of digits in ob_digit.

   For inlined longs, their value can be obtained with this expression:

        ob_size >> 1

   For inlined longs:
       * These integers have a capacity of 62bits on 64-bit architectures:
         one bit for the "is inlined" flag, and one sign bit.  This is 30 bits on
         32-bit architectures (for the same reasons).
       * Inlined longs are always normalized, as they use the machine
         representation for integers.
       * Allocation functions won't allocate the space for the ob_digit buffer,
         because these are never used with this representation.
       * As a consequence of the previous point, the width of a digit for
         long longs can be either 15 or 30, and this doesn't affect the
         representation of inlined longs.

   For bignum longs, their absolute value can be obtained with this expression:

        SUM(for i=0 through abs(ob_size >> 1)-1) ob_digit[i] * 2**(SHIFT*i)

   In this representation:
       * These numbers can also be normalized.  In a normalized number,
         ob_digit[abs(ob_size)-1] (the most significant digit) is never zero.
       * Also, in all cases, for all valid i, 0 <= ob_digit[i] <= MASK.
       * The allocation function takes care of allocating extra memory
         so that ob_digit[0] ... ob_digit[abs(ob_size)-1] are actually available.

   In either case:
       * Negative numbers are represented by (ob_size >> 1) < 0
       * Zero is represented by (ob_size >> 1) == 0

   CAUTION:  Generic code manipulating subtypes of PyVarObject has to
   aware that ints abuse  ob_size's sign bit and its least significant
   bit.
*/

struct _longobject {
    PyObject_VAR_HEAD
    digit ob_digit[1];
};

PyAPI_FUNC(PyLongObject *) _PyLong_New(Py_ssize_t);

/* Return a copy of src. */
PyAPI_FUNC(PyObject *) _PyLong_Copy(PyLongObject *src);

#ifdef __cplusplus
}
#endif
#endif /* !Py_LONGINTREPR_H */
#endif /* Py_LIMITED_API */
