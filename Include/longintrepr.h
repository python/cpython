#ifndef Py_LONGINTREPR_H
#define Py_LONGINTREPR_H
#ifdef __cplusplus
extern "C" {
#endif


/* This is published for the benefit of "friend" marshal.c only. */

/* Parameters of the long integer representation.
   These shouldn't have to be changed as C should guarantee that a short
   contains at least 16 bits, but it's made changeable anyway.
   Note: 'digit' should be able to hold 2*MASK+1, and 'twodigits'
   should be able to hold the intermediate results in 'mul'
   (at most (BASE-1)*(2*BASE+1) == MASK*(2*MASK+3)).
   Also, x_sub assumes that 'digit' is an unsigned type, and overflow
   is handled by taking the result mod 2**N for some N > SHIFT.
   And, at some places it is assumed that MASK fits in an int, as well.
   long_pow() requires that SHIFT be divisible by 5. */

typedef unsigned short digit;
typedef unsigned int wdigit; /* digit widened to parameter size */
#define BASE_TWODIGITS_TYPE long
typedef unsigned BASE_TWODIGITS_TYPE twodigits;
typedef BASE_TWODIGITS_TYPE stwodigits; /* signed variant of twodigits */

#define SHIFT	15
#define BASE	((digit)1 << SHIFT)
#define MASK	((int)(BASE - 1))

#if SHIFT % 5 != 0
#error "longobject.c requires that SHIFT be divisible by 5"
#endif

/* Long integer representation.
   The absolute value of a number is equal to
   	SUM(for i=0 through abs(ob_size)-1) ob_digit[i] * 2**(SHIFT*i)
   Negative numbers are represented with ob_size < 0;
   zero is represented by ob_size == 0.
   In a normalized number, ob_digit[abs(ob_size)-1] (the most significant
   digit) is never zero.  Also, in all cases, for all valid i,
   	0 <= ob_digit[i] <= MASK.
   The allocation function takes care of allocating extra memory
   so that ob_digit[0] ... ob_digit[abs(ob_size)-1] are actually available.

   CAUTION:  Generic code manipulating subtypes of PyVarObject has to
   aware that longs abuse  ob_size's sign bit.
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
