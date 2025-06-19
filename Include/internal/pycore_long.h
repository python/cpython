#ifndef Py_INTERNAL_LONG_H
#define Py_INTERNAL_LONG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_bytesobject.h"   // _PyBytesWriter
#include "pycore_runtime.h"       // _Py_SINGLETON()

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

PyAPI_FUNC(void) _PyLong_ExactDealloc(PyObject *self);

#define _PyLong_SMALL_INTS _Py_SINGLETON(small_ints)

// _PyLong_GetZero() and _PyLong_GetOne() must always be available
// _PyLong_FromUnsignedChar must always be available
#if _PY_NSMALLPOSINTS < 257
#  error "_PY_NSMALLPOSINTS must be greater than or equal to 257"
#endif

#define _PY_IS_SMALL_INT(val) ((val) >= 0 && (val) < 256 && (val) < _PY_NSMALLPOSINTS)

// Return a reference to the immortal zero singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetZero(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS]; }

// Return a reference to the immortal one singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetOne(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+1]; }

static inline PyObject* _PyLong_FromUnsignedChar(unsigned char i)
{
    return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+i];
}

// _PyLong_Frexp returns a double x and an exponent e such that the
// true value is approximately equal to x * 2**e.  x is
// 0.0 if and only if the input is 0 (in which case, e and x are both
// zeroes); otherwise, 0.5 <= abs(x) < 1.0.
// Always successful.
//
// Export for 'math' shared extension
PyAPI_DATA(double) _PyLong_Frexp(PyLongObject *a, int64_t *e);

extern PyObject* _PyLong_FromBytes(const char *, Py_ssize_t, int);

// _PyLong_DivmodNear.  Given integers a and b, compute the nearest
// integer q to the exact quotient a / b, rounding to the nearest even integer
// in the case of a tie.  Return (q, r), where r = a - q*b.  The remainder r
// will satisfy abs(r) <= abs(b)/2, with equality possible only if q is
// even.
//
// Export for '_datetime' shared extension.
PyAPI_DATA(PyObject*) _PyLong_DivmodNear(PyObject *, PyObject *);

// _PyLong_Format: Convert the long to a string object with given base,
// appending a base prefix of 0[box] if base is 2, 8 or 16.
// Export for '_tkinter' shared extension.
PyAPI_DATA(PyObject*) _PyLong_Format(PyObject *obj, int base);

// Export for 'math' shared extension
PyAPI_DATA(PyObject*) _PyLong_Rshift(PyObject *, int64_t);

// Export for 'math' shared extension
PyAPI_DATA(PyObject*) _PyLong_Lshift(PyObject *, int64_t);

PyAPI_FUNC(_PyStackRef) _PyCompactLong_Add(PyLongObject *left, PyLongObject *right);
PyAPI_FUNC(_PyStackRef) _PyCompactLong_Multiply(PyLongObject *left, PyLongObject *right);
PyAPI_FUNC(_PyStackRef) _PyCompactLong_Subtract(PyLongObject *left, PyLongObject *right);

// Export for 'binascii' shared extension.
PyAPI_DATA(unsigned char) _PyLong_DigitValue[256];

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyLong_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

extern int _PyLong_FormatWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    int base,
    int alternate);

extern char* _PyLong_FormatBytesWriter(
    _PyBytesWriter *writer,
    char *str,
    PyObject *obj,
    int base,
    int alternate);

// Argument converters used by Argument Clinic

// Export for 'select' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedShort_Converter(PyObject *, void *);

// Export for '_testclinic' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedInt_Converter(PyObject *, void *);

// Export for '_blake2' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedLong_Converter(PyObject *, void *);

// Export for '_blake2' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedLongLong_Converter(PyObject *, void *);

// Export for '_testclinic' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_Size_t_Converter(PyObject *, void *);

PyAPI_FUNC(int) _PyLong_UInt8_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UInt16_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UInt32_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UInt64_Converter(PyObject *, void *);

/* Long value tag bits:
 * 0-1: Sign bits value = (1-sign), ie. negative=2, positive=0, zero=1.
 * 2: Set to 1 for the small ints
 * 3+ Unsigned digit count
 */
#define SIGN_MASK 3
#define SIGN_ZERO 1
#define SIGN_NEGATIVE 2
#define NON_SIZE_BITS 3
#define IMMORTALITY_BIT_MASK (1 << 2)

/* The functions _PyLong_IsCompact and _PyLong_CompactValue are defined
 * in Include/cpython/longobject.h, since they need to be inline.
 *
 * "Compact" values have at least one bit to spare,
 * so that addition and subtraction can be performed on the values
 * without risk of overflow.
 *
 * The inline functions need tag bits.
 * For readability, rather than do `#define SIGN_MASK _PyLong_SIGN_MASK`
 * we define them to the numbers in both places and then assert that
 * they're the same.
 */
#if SIGN_MASK != _PyLong_SIGN_MASK
#  error "SIGN_MASK does not match _PyLong_SIGN_MASK"
#endif
#if NON_SIZE_BITS != _PyLong_NON_SIZE_BITS
#  error "NON_SIZE_BITS does not match _PyLong_NON_SIZE_BITS"
#endif

/* All *compact" values are guaranteed to fit into
 * a Py_ssize_t with at least one bit to spare.
 * In other words, for 64 bit machines, compact
 * will be signed 63 (or fewer) bit values
 */

/* Return 1 if the argument is compact int */
static inline int
_PyLong_IsNonNegativeCompact(const PyLongObject* op) {
    assert(PyLong_Check(op));
    return ((op->long_value.lv_tag & ~IMMORTALITY_BIT_MASK) <= (1 << NON_SIZE_BITS));
}


static inline int
_PyLong_BothAreCompact(const PyLongObject* a, const PyLongObject* b) {
    assert(PyLong_Check(a));
    assert(PyLong_Check(b));
    return (a->long_value.lv_tag | b->long_value.lv_tag) < (2 << NON_SIZE_BITS);
}
static inline bool
_PyLong_IsZero(const PyLongObject *op)
{
    return (op->long_value.lv_tag & SIGN_MASK) == SIGN_ZERO;
}

static inline bool
_PyLong_IsNegative(const PyLongObject *op)
{
    return (op->long_value.lv_tag & SIGN_MASK) == SIGN_NEGATIVE;
}

static inline bool
_PyLong_IsPositive(const PyLongObject *op)
{
    return (op->long_value.lv_tag & SIGN_MASK) == 0;
}

static inline Py_ssize_t
_PyLong_DigitCount(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    return (Py_ssize_t)(op->long_value.lv_tag >> NON_SIZE_BITS);
}

/* Equivalent to _PyLong_DigitCount(op) * _PyLong_NonCompactSign(op) */
static inline Py_ssize_t
_PyLong_SignedDigitCount(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    Py_ssize_t sign = 1 - (op->long_value.lv_tag & SIGN_MASK);
    return sign * (Py_ssize_t)(op->long_value.lv_tag >> NON_SIZE_BITS);
}

static inline int
_PyLong_CompactSign(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    assert(_PyLong_IsCompact((PyLongObject *)op));
    return 1 - (op->long_value.lv_tag & SIGN_MASK);
}

static inline int
_PyLong_NonCompactSign(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    assert(!_PyLong_IsCompact((PyLongObject *)op));
    return 1 - (op->long_value.lv_tag & SIGN_MASK);
}

/* Do a and b have the same sign? */
static inline int
_PyLong_SameSign(const PyLongObject *a, const PyLongObject *b)
{
    return (a->long_value.lv_tag & SIGN_MASK) == (b->long_value.lv_tag & SIGN_MASK);
}

#define TAG_FROM_SIGN_AND_SIZE(sign, size) \
    ((uintptr_t)(1 - (sign)) | ((uintptr_t)(size) << NON_SIZE_BITS))

static inline void
_PyLong_SetSignAndDigitCount(PyLongObject *op, int sign, Py_ssize_t size)
{
    assert(size >= 0);
    assert(-1 <= sign && sign <= 1);
    assert(sign != 0 || size == 0);
    op->long_value.lv_tag = TAG_FROM_SIGN_AND_SIZE(sign, size);
}

static inline void
_PyLong_SetDigitCount(PyLongObject *op, Py_ssize_t size)
{
    assert(size >= 0);
    op->long_value.lv_tag = (((size_t)size) << NON_SIZE_BITS) | (op->long_value.lv_tag & SIGN_MASK);
}

#define NON_SIZE_MASK ~(uintptr_t)((1 << NON_SIZE_BITS) - 1)

static inline void
_PyLong_FlipSign(PyLongObject *op) {
    unsigned int flipped_sign = 2 - (op->long_value.lv_tag & SIGN_MASK);
    op->long_value.lv_tag &= NON_SIZE_MASK;
    op->long_value.lv_tag |= flipped_sign;
}

#define _PyLong_DIGIT_INIT(val) \
    { \
        .ob_base = _PyObject_HEAD_INIT(&PyLong_Type), \
        .long_value  = { \
            .lv_tag = TAG_FROM_SIGN_AND_SIZE( \
                (val) == 0 ? 0 : ((val) < 0 ? -1 : 1), \
                (val) == 0 ? 0 : 1) | IMMORTALITY_BIT_MASK, \
            { ((val) >= 0 ? (val) : -(val)) }, \
        } \
    }

#define _PyLong_FALSE_TAG TAG_FROM_SIGN_AND_SIZE(0, 0)
#define _PyLong_TRUE_TAG TAG_FROM_SIGN_AND_SIZE(1, 1)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_H */
