#ifndef Py_INTERNAL_PYMATH_H
#define Py_INTERNAL_PYMATH_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* _Py_ADJUST_ERANGE1(x)
 * _Py_ADJUST_ERANGE2(x, y)
 * Set errno to 0 before calling a libm function, and invoke one of these
 * macros after, passing the function result(s) (_Py_ADJUST_ERANGE2 is useful
 * for functions returning complex results).  This makes two kinds of
 * adjustments to errno:  (A) If it looks like the platform libm set
 * errno=ERANGE due to underflow, clear errno. (B) If it looks like the
 * platform libm overflowed but didn't set errno, force errno to ERANGE.  In
 * effect, we're trying to force a useful implementation of C89 errno
 * behavior.
 * Caution:
 *    This isn't reliable.  See Py_OVERFLOWED comments.
 *    X and Y may be evaluated more than once.
 */
static inline void _Py_ADJUST_ERANGE1(double x)
{
    if (errno == 0) {
        if (x == Py_HUGE_VAL || x == -Py_HUGE_VAL) {
            errno = ERANGE;
        }
    }
    else if (errno == ERANGE && x == 0.0) {
        errno = 0;
    }
}

static inline void _Py_ADJUST_ERANGE2(double x, double y)
{
    if (x == Py_HUGE_VAL || x == -Py_HUGE_VAL ||
        y == Py_HUGE_VAL || y == -Py_HUGE_VAL)
    {
        if (errno == 0) {
            errno = ERANGE;
        }
    }
    else if (errno == ERANGE) {
        errno = 0;
    }
}

// Return whether integral type *type* is signed or not.
#define _Py_IntegralTypeSigned(type) \
    ((type)(-1) < 0)

// Return the maximum value of integral type *type*.
#define _Py_IntegralTypeMax(type) \
    ((_Py_IntegralTypeSigned(type)) ? (((((type)1 << (sizeof(type)*CHAR_BIT - 2)) - 1) << 1) + 1) : ~(type)0)

// Return the minimum value of integral type *type*.
#define _Py_IntegralTypeMin(type) \
    ((_Py_IntegralTypeSigned(type)) ? -_Py_IntegralTypeMax(type) - 1 : 0)

// Check whether *v* is in the range of integral type *type*. This is most
// useful if *v* is floating-point, since demoting a floating-point *v* to an
// integral type that cannot represent *v*'s integral part is undefined
// behavior.
#define _Py_InIntegralTypeRange(type, v) \
    (_Py_IntegralTypeMin(type) <= v && v <= _Py_IntegralTypeMax(type))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYMATH_H */
