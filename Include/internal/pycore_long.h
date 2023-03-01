#ifndef Py_INTERNAL_LONG_H
#define Py_INTERNAL_LONG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_global_objects.h"  // _PY_NSMALLNEGINTS
#include "pycore_runtime.h"       // _PyRuntime

/*
 * Default int base conversion size limitation: Denial of Service prevention.
 *
 * Chosen such that this isn't wildly slow on modern hardware and so that
 * everyone's existing deployed numpy test suite passes before
 * https://github.com/numpy/numpy/issues/22098 is widely available.
 *
 * $ python -m timeit -s 's = "1"*4300' 'int(s)'
 * 2000 loops, best of 5: 125 usec per loop
 * $ python -m timeit -s 's = "1"*4300; v = int(s)' 'str(v)'
 * 1000 loops, best of 5: 311 usec per loop
 * (zen2 cloud VM)
 *
 * 4300 decimal digits fits a ~14284 bit number.
 */
#define _PY_LONG_DEFAULT_MAX_STR_DIGITS 4300
/*
 * Threshold for max digits check.  For performance reasons int() and
 * int.__str__() don't checks values that are smaller than this
 * threshold.  Acts as a guaranteed minimum size limit for bignums that
 * applications can expect from CPython.
 *
 * % python -m timeit -s 's = "1"*640; v = int(s)' 'str(int(s))'
 * 20000 loops, best of 5: 12 usec per loop
 *
 * "640 digits should be enough for anyone." - gps
 * fits a ~2126 bit decimal number.
 */
#define _PY_LONG_MAX_STR_DIGITS_THRESHOLD 640

#if ((_PY_LONG_DEFAULT_MAX_STR_DIGITS != 0) && \
   (_PY_LONG_DEFAULT_MAX_STR_DIGITS < _PY_LONG_MAX_STR_DIGITS_THRESHOLD))
# error "_PY_LONG_DEFAULT_MAX_STR_DIGITS smaller than threshold."
#endif


/* runtime lifecycle */

extern PyStatus _PyLong_InitTypes(PyInterpreterState *);
extern void _PyLong_FiniTypes(PyInterpreterState *interp);


/* other API */

#define _PyLong_SMALL_INTS _Py_SINGLETON(small_ints)

// _PyLong_GetZero() and _PyLong_GetOne() must always be available
// _PyLong_FromUnsignedChar must always be available
#if _PY_NSMALLPOSINTS < 257
#  error "_PY_NSMALLPOSINTS must be greater than or equal to 257"
#endif

// Return a borrowed reference to the zero singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetZero(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS]; }

// Return a borrowed reference to the one singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetOne(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+1]; }

static inline PyObject* _PyLong_FromUnsignedChar(unsigned char i)
{
    return Py_NewRef((PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+i]);
}

PyObject *_PyLong_Add(PyLongObject *left, PyLongObject *right);
PyObject *_PyLong_Multiply(PyLongObject *left, PyLongObject *right);
PyObject *_PyLong_Subtract(PyLongObject *left, PyLongObject *right);

/* Used by Python/mystrtoul.c, _PyBytes_FromHex(),
   _PyBytes_DecodeEscape(), etc. */
PyAPI_DATA(unsigned char) _PyLong_DigitValue[256];

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
PyAPI_FUNC(int) _PyLong_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

PyAPI_FUNC(int) _PyLong_FormatWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    int base,
    int alternate);

PyAPI_FUNC(char*) _PyLong_FormatBytesWriter(
    _PyBytesWriter *writer,
    char *str,
    PyObject *obj,
    int base,
    int alternate);


/* Return 1 if the argument is positive single digit int */
static inline int
_PyLong_IsNonNegativeSingleDigit(const PyLongObject* op) {
    /*  We perform a fast check using a single comparison by casting from int
        to uint which casts negative numbers to large positive numbers.
        For details see Section 14.2 "Bounds Checking" in the Agner Fog
        optimization manual found at:
        https://www.agner.org/optimize/optimizing_cpp.pdf

        The function is not affected by -fwrapv, -fno-wrapv and -ftrapv
        compiler options of GCC and clang
    */
    assert(PyLong_Check(op));
    Py_ssize_t signed_size = op->long_value.ob_size;
    return ((size_t)signed_size) <= 1;
}


static inline int
_PyLong_IsSingleDigit(const PyLongObject* op) {
    assert(PyLong_Check(op));
    Py_ssize_t signed_size = op->long_value.ob_size;
    return ((size_t)(signed_size+1)) <= 2;
}

static inline Py_ssize_t
_PyLong_SingleDigitValue(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    assert(op->long_value.ob_size >= -1 && op->long_value.ob_size <= 1);
    return op->long_value.ob_size * op->long_value.ob_digit[0];
}

static inline bool
_PyLong_IsPositive(const PyLongObject *op)
{
    return op->long_value.ob_size > 0;
}

static inline bool
_PyLong_IsNegative(const PyLongObject *op)
{
    return op->long_value.ob_size < 0;
}

static inline Py_ssize_t
_PyLong_DigitCount(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    return Py_ABS(op->long_value.ob_size);
}

/* Equivalent to _PyLong_DigitCount(op) * _PyLong_NonZeroSign(op) */
static inline Py_ssize_t
_PyLong_SignedDigitCount(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    return op->long_value.ob_size;
}

/* Like _PyLong_DigitCount but asserts that op is non-negative */
static inline Py_ssize_t
_PyLong_UnsignedDigitCount(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    assert(!_PyLong_IsNegative(op));
    return op->long_value.ob_size;
}

static inline bool
_PyLong_IsZero(const PyLongObject *op)
{
    return op->long_value.ob_size == 0;
}

static inline int
_PyLong_NonZeroSign(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    assert(!_PyLong_IsZero(op));
    return ((op->long_value.ob_size > 0) << 1) - 1;
}

/* Do a and b have the same sign? Zero counts as positive. */
static inline int
_PyLong_SameSign(const PyLongObject *a, const PyLongObject *b)
{
    return (a->long_value.ob_size ^ b->long_value.ob_size) >= 0;
}

static inline void
_PyLong_SetSignAndSize(PyLongObject *op, bool negative, Py_ssize_t size)
{
    int sign = 1-negative*2;
    assert(sign == -1 || sign == 1);
    op->long_value.ob_size = sign * size;
}

static inline void
_PyLong_FlipSign(PyLongObject *op) {
    op->long_value.ob_size = -op->long_value.ob_size;
}

#define _PyLong_DIGIT_INIT(val) \
    { \
        .ob_base = _PyObject_IMMORTAL_INIT(&PyLong_Type), \
        .long_value  = { \
            .ob_size = (val) == 0 ? 0 : ((val) < 0 ? -1 : 1), \
            { ((val) >= 0 ? (val) : -(val)) }, \
        } \
    }

#define TAG_FROM_SIGN_AND_SIZE(neg, size) (neg ? -(size) : (size))

#define _PyLong_FALSE_TAG 0
#define _PyLong_TRUE_TAG 1


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_H */
