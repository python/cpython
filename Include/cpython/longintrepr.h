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

/* Long integer representation.

   Long integers are made up of a number of 30- or 15-bit digits, depending on
   the platform. The number of digits (ndigits) is stored in the high bits of
   the lv_tag field (lvtag >> _PyLong_NON_SIZE_BITS).

   The absolute value of a number is equal to
        SUM(for i=0 through ndigits-1) ob_digit[i] * 2**(PyLong_SHIFT*i)

   The sign of the value is stored in the lower 2 bits of lv_tag.

    - 0: Positive
    - 1: Zero
    - 2: Negative

   The third lowest bit of lv_tag is
   set to 1 for the small ints.

   In a normalized number, ob_digit[ndigits-1] (the most significant
   digit) is never zero.  Also, in all cases, for all valid i,
        0 <= ob_digit[i] <= PyLong_MASK.

   The allocation function takes care of allocating extra memory
   so that ob_digit[0] ... ob_digit[ndigits-1] are actually available.
   We always allocate memory for at least one digit, so accessing ob_digit[0]
   is always safe. However, in the case ndigits == 0, the contents of
   ob_digit[0] may be undefined.
*/

typedef struct _PyLongValue {
    uintptr_t lv_tag; /* Number of digits, sign and flags */
    digit ob_digit[1];
} _PyLongValue;

struct _longobject {
    PyObject_HEAD
    _PyLongValue long_value;
};

Py_DEPRECATED(3.14) PyAPI_FUNC(PyLongObject*) _PyLong_New(Py_ssize_t);

// Return a copy of src.
PyAPI_FUNC(PyObject*) _PyLong_Copy(PyLongObject *src);

Py_DEPRECATED(3.14) PyAPI_FUNC(PyLongObject*) _PyLong_FromDigits(
    int negative,
    Py_ssize_t digit_count,
    digit *digits);


/* Inline some internals for speed. These should be in pycore_long.h
 * if user code didn't need them inlined. */

#define _PyLong_SIGN_MASK 3
#define _PyLong_NON_SIZE_BITS 3


static inline int
_PyLong_IsCompact(const PyLongObject* op) {
    assert(PyType_HasFeature(op->ob_base.ob_type, Py_TPFLAGS_LONG_SUBCLASS));
    return op->long_value.lv_tag < (2 << _PyLong_NON_SIZE_BITS);
}

#define PyUnstable_Long_IsCompact _PyLong_IsCompact

static inline Py_ssize_t
_PyLong_CompactValue(const PyLongObject *op)
{
    Py_ssize_t sign;
    assert(PyType_HasFeature(op->ob_base.ob_type, Py_TPFLAGS_LONG_SUBCLASS));
    assert(PyUnstable_Long_IsCompact(op));
    sign = 1 - (op->long_value.lv_tag & _PyLong_SIGN_MASK);
    return sign * (Py_ssize_t)op->long_value.ob_digit[0];
}

#define PyUnstable_Long_CompactValue _PyLong_CompactValue


/* --- Import/Export API -------------------------------------------------- */

typedef struct PyLongLayout {
    uint8_t bits_per_digit;
    uint8_t digit_size;
    int8_t digits_order;
    int8_t digit_endianness;
} PyLongLayout;

PyAPI_FUNC(const PyLongLayout*) PyLong_GetNativeLayout(void);

typedef struct PyLongExport {
    int64_t value;
    uint8_t negative;
    Py_ssize_t ndigits;
    const void *digits;
    // Member used internally, must not be used for other purpose.
    Py_uintptr_t _reserved;
} PyLongExport;

PyAPI_FUNC(int) PyLong_Export(
    PyObject *obj,
    PyLongExport *export_long);
PyAPI_FUNC(void) PyLong_FreeExport(
    PyLongExport *export_long);


/* --- PyLongWriter API --------------------------------------------------- */

typedef struct PyLongWriter PyLongWriter;

PyAPI_FUNC(PyLongWriter*) PyLongWriter_Create(
    int negative,
    Py_ssize_t ndigits,
    void **digits);
PyAPI_FUNC(PyObject*) PyLongWriter_Finish(PyLongWriter *writer);
PyAPI_FUNC(void) PyLongWriter_Discard(PyLongWriter *writer);

#ifdef __cplusplus
}
#endif
#endif /* !Py_LONGINTREPR_H */
#endif /* Py_LIMITED_API */
