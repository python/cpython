#include "Python.h"
#include "pycore_initconfig.h"    // _PyStatus_ERR
#include "pycore_pystate.h"       // _Py_AssertHoldsTstate()
#include "pycore_runtime.h"       // _PyRuntime
#include "pycore_time.h"          // PyTime_t

#include <time.h>                 // gmtime_r()
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>           // gettimeofday()
#endif
#ifdef MS_WINDOWS
#  include <winsock2.h>           // struct timeval
#endif

#if defined(__APPLE__)
#  include <mach/mach_time.h>     // mach_absolute_time(), mach_timebase_info()

#if defined(__APPLE__) && defined(__has_builtin)
#  if __has_builtin(__builtin_available)
#    define HAVE_CLOCK_GETTIME_RUNTIME __builtin_available(macOS 10.12, iOS 10.0, tvOS 10.0, watchOS 3.0, *)
#  endif
#endif
#endif

/* To millisecond (10^-3) */
#define SEC_TO_MS 1000

/* To microseconds (10^-6) */
#define MS_TO_US 1000
#define SEC_TO_US (SEC_TO_MS * MS_TO_US)

/* To nanoseconds (10^-9) */
#define US_TO_NS 1000
#define MS_TO_NS (MS_TO_US * US_TO_NS)
#define SEC_TO_NS (SEC_TO_MS * MS_TO_NS)

/* Conversion from nanoseconds */
#define NS_TO_MS (1000 * 1000)
#define NS_TO_US (1000)
#define NS_TO_100NS (100)

#if SIZEOF_TIME_T == SIZEOF_LONG_LONG
#  define PY_TIME_T_MAX LLONG_MAX
#  define PY_TIME_T_MIN LLONG_MIN
#elif SIZEOF_TIME_T == SIZEOF_LONG
#  define PY_TIME_T_MAX LONG_MAX
#  define PY_TIME_T_MIN LONG_MIN
#else
#  error "unsupported time_t size"
#endif

#if PY_TIME_T_MAX + PY_TIME_T_MIN != -1
#  error "time_t is not a two's complement integer type"
#endif

#if PyTime_MIN + PyTime_MAX != -1
#  error "PyTime_t is not a two's complement integer type"
#endif


static PyTime_t
_PyTime_GCD(PyTime_t x, PyTime_t y)
{
    // Euclidean algorithm
    assert(x >= 1);
    assert(y >= 1);
    while (y != 0) {
        PyTime_t tmp = y;
        y = x % y;
        x = tmp;
    }
    assert(x >= 1);
    return x;
}


int
_PyTimeFraction_Set(_PyTimeFraction *frac, PyTime_t numer, PyTime_t denom)
{
    if (numer < 1 || denom < 1) {
        return -1;
    }

    PyTime_t gcd = _PyTime_GCD(numer, denom);
    frac->numer = numer / gcd;
    frac->denom = denom / gcd;
    return 0;
}


double
_PyTimeFraction_Resolution(const _PyTimeFraction *frac)
{
    return (double)frac->numer / (double)frac->denom / 1e9;
}


static void
pytime_time_t_overflow(void)
{
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp out of range for platform time_t");
}


static void
pytime_overflow(void)
{
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp too large to convert to C PyTime_t");
}


// Compute t1 + t2. Clamp to [PyTime_MIN; PyTime_MAX] on overflow.
static inline int
pytime_add(PyTime_t *t1, PyTime_t t2)
{
    if (t2 > 0 && *t1 > PyTime_MAX - t2) {
        *t1 = PyTime_MAX;
        return -1;
    }
    else if (t2 < 0 && *t1 < PyTime_MIN - t2) {
        *t1 = PyTime_MIN;
        return -1;
    }
    else {
        *t1 += t2;
        return 0;
    }
}


PyTime_t
_PyTime_Add(PyTime_t t1, PyTime_t t2)
{
    (void)pytime_add(&t1, t2);
    return t1;
}


static inline int
pytime_mul_check_overflow(PyTime_t a, PyTime_t b)
{
    if (b != 0) {
        assert(b > 0);
        return ((a < PyTime_MIN / b) || (PyTime_MAX / b < a));
    }
    else {
        return 0;
    }
}


// Compute t * k. Clamp to [PyTime_MIN; PyTime_MAX] on overflow.
static inline int
pytime_mul(PyTime_t *t, PyTime_t k)
{
    assert(k >= 0);
    if (pytime_mul_check_overflow(*t, k)) {
        *t = (*t >= 0) ? PyTime_MAX : PyTime_MIN;
        return -1;
    }
    else {
        *t *= k;
        return 0;
    }
}


// Compute t * k. Clamp to [PyTime_MIN; PyTime_MAX] on overflow.
static inline PyTime_t
_PyTime_Mul(PyTime_t t, PyTime_t k)
{
    (void)pytime_mul(&t, k);
    return t;
}


PyTime_t
_PyTimeFraction_Mul(PyTime_t ticks, const _PyTimeFraction *frac)
{
    const PyTime_t mul = frac->numer;
    const PyTime_t div = frac->denom;

    if (div == 1) {
        // Fast-path taken by mach_absolute_time() with 1/1 time base.
        return _PyTime_Mul(ticks, mul);
    }

    /* Compute (ticks * mul / div) in two parts to reduce the risk of integer
       overflow: compute the integer part, and then the remaining part.

       (ticks * mul) / div == (ticks / div) * mul + (ticks % div) * mul / div
    */
    PyTime_t intpart, remaining;
    intpart = ticks / div;
    ticks %= div;
    remaining = _PyTime_Mul(ticks, mul) / div;
    // intpart * mul + remaining
    return _PyTime_Add(_PyTime_Mul(intpart, mul), remaining);
}


time_t
_PyLong_AsTime_t(PyObject *obj)
{
#if SIZEOF_TIME_T == SIZEOF_LONG_LONG
    long long val = PyLong_AsLongLong(obj);
#elif SIZEOF_TIME_T <= SIZEOF_LONG
    long val = PyLong_AsLong(obj);
#else
#   error "unsupported time_t size"
#endif
    if (val == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            pytime_time_t_overflow();
        }
        return -1;
    }
    return (time_t)val;
}


PyObject *
_PyLong_FromTime_t(time_t t)
{
#if SIZEOF_TIME_T == SIZEOF_LONG_LONG
    return PyLong_FromLongLong((long long)t);
#elif SIZEOF_TIME_T <= SIZEOF_LONG
    return PyLong_FromLong((long)t);
#else
#   error "unsupported time_t size"
#endif
}


// Convert PyTime_t to time_t.
// Return 0 on success. Return -1 and clamp the value on overflow.
static int
_PyTime_AsTime_t(PyTime_t t, time_t *t2)
{
#if SIZEOF_TIME_T < _SIZEOF_PYTIME_T
    if ((PyTime_t)PY_TIME_T_MAX < t) {
        *t2 = PY_TIME_T_MAX;
        return -1;
    }
    if (t < (PyTime_t)PY_TIME_T_MIN) {
        *t2 = PY_TIME_T_MIN;
        return -1;
    }
#endif
    *t2 = (time_t)t;
    return 0;
}


#ifdef MS_WINDOWS
// Convert PyTime_t to long.
// Return 0 on success. Return -1 and clamp the value on overflow.
static int
_PyTime_AsCLong(PyTime_t t, long *t2)
{
#if SIZEOF_LONG < _SIZEOF_PYTIME_T
    if ((PyTime_t)LONG_MAX < t) {
        *t2 = LONG_MAX;
        return -1;
    }
    if (t < (PyTime_t)LONG_MIN) {
        *t2 = LONG_MIN;
        return -1;
    }
#endif
    *t2 = (long)t;
    return 0;
}
#endif


/* Round to nearest with ties going to nearest even integer
   (_PyTime_ROUND_HALF_EVEN) */
static double
pytime_round_half_even(double x)
{
    double rounded = round(x);
    if (fabs(x-rounded) == 0.5) {
        /* halfway case: round to even */
        rounded = 2.0 * round(x / 2.0);
    }
    return rounded;
}


static double
pytime_round(double x, _PyTime_round_t round)
{
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double d;

    d = x;
    if (round == _PyTime_ROUND_HALF_EVEN) {
        d = pytime_round_half_even(d);
    }
    else if (round == _PyTime_ROUND_CEILING) {
        d = ceil(d);
    }
    else if (round == _PyTime_ROUND_FLOOR) {
        d = floor(d);
    }
    else {
        assert(round == _PyTime_ROUND_UP);
        d = (d >= 0.0) ? ceil(d) : floor(d);
    }
    return d;
}


static int
pytime_double_to_denominator(double d, time_t *sec, long *numerator,
                             long idenominator, _PyTime_round_t round)
{
    double denominator = (double)idenominator;
    double intpart;
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double floatpart;

    floatpart = modf(d, &intpart);

    floatpart *= denominator;
    floatpart = pytime_round(floatpart, round);
    if (floatpart >= denominator) {
        floatpart -= denominator;
        intpart += 1.0;
    }
    else if (floatpart < 0) {
        floatpart += denominator;
        intpart -= 1.0;
    }
    assert(0.0 <= floatpart && floatpart < denominator);

    /*
       Conversion of an out-of-range value to time_t gives undefined behaviour
       (C99 §6.3.1.4p1), so we must guard against it. However, checking that
       `intpart` is in range is delicate: the obvious expression `intpart <=
       PY_TIME_T_MAX` will first convert the value `PY_TIME_T_MAX` to a double,
       potentially changing its value and leading to us failing to catch some
       UB-inducing values. The code below works correctly under the mild
       assumption that time_t is a two's complement integer type with no trap
       representation, and that `PY_TIME_T_MIN` is within the representable
       range of a C double.

       Note: we want the `if` condition below to be true for NaNs; therefore,
       resist any temptation to simplify by applying De Morgan's laws.
    */
    if (!((double)PY_TIME_T_MIN <= intpart && intpart < -(double)PY_TIME_T_MIN)) {
        pytime_time_t_overflow();
        return -1;
    }
    *sec = (time_t)intpart;
    *numerator = (long)floatpart;
    assert(0 <= *numerator && *numerator < idenominator);
    return 0;
}


static int
pytime_object_to_denominator(PyObject *obj, time_t *sec, long *numerator,
                             long denominator, _PyTime_round_t round)
{
    assert(denominator >= 1);

    if (PyFloat_Check(obj)) {
        double d = PyFloat_AsDouble(obj);
        if (isnan(d)) {
            *numerator = 0;
            PyErr_SetString(PyExc_ValueError, "Invalid value NaN (not a number)");
            return -1;
        }
        return pytime_double_to_denominator(d, sec, numerator,
                                            denominator, round);
    }
    else {
        *sec = _PyLong_AsTime_t(obj);
        *numerator = 0;
        if (*sec == (time_t)-1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_Format(PyExc_TypeError,
                             "argument must be int or float, not %T", obj);
            }
            return -1;
        }
        return 0;
    }
}


int
_PyTime_ObjectToTime_t(PyObject *obj, time_t *sec, _PyTime_round_t round)
{
    if (PyFloat_Check(obj)) {
        double intpart;
        /* volatile avoids optimization changing how numbers are rounded */
        volatile double d;

        d = PyFloat_AsDouble(obj);
        if (isnan(d)) {
            PyErr_SetString(PyExc_ValueError, "Invalid value NaN (not a number)");
            return -1;
        }

        d = pytime_round(d, round);
        (void)modf(d, &intpart);

        /* See comments in pytime_double_to_denominator */
        if (!((double)PY_TIME_T_MIN <= intpart && intpart < -(double)PY_TIME_T_MIN)) {
            pytime_time_t_overflow();
            return -1;
        }
        *sec = (time_t)intpart;
        return 0;
    }
    else {
        *sec = _PyLong_AsTime_t(obj);
        if (*sec == (time_t)-1 && PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }
}


int
_PyTime_ObjectToTimespec(PyObject *obj, time_t *sec, long *nsec,
                         _PyTime_round_t round)
{
    return pytime_object_to_denominator(obj, sec, nsec, SEC_TO_NS, round);
}


int
_PyTime_ObjectToTimeval(PyObject *obj, time_t *sec, long *usec,
                        _PyTime_round_t round)
{
    return pytime_object_to_denominator(obj, sec, usec, SEC_TO_US, round);
}


PyTime_t
_PyTime_FromSeconds(int seconds)
{
    /* ensure that integer overflow cannot happen, int type should have 32
       bits, whereas PyTime_t type has at least 64 bits (SEC_TO_NS takes 30
       bits). */
    static_assert(INT_MAX <= PyTime_MAX / SEC_TO_NS, "PyTime_t overflow");
    static_assert(INT_MIN >= PyTime_MIN / SEC_TO_NS, "PyTime_t underflow");

    PyTime_t t = (PyTime_t)seconds;
    assert((t >= 0 && t <= PyTime_MAX / SEC_TO_NS)
           || (t < 0 && t >= PyTime_MIN / SEC_TO_NS));
    t *= SEC_TO_NS;
    return t;
}


PyTime_t
_PyTime_FromMicrosecondsClamp(PyTime_t us)
{
    PyTime_t ns = _PyTime_Mul(us, US_TO_NS);
    return ns;
}


int
_PyTime_FromLong(PyTime_t *tp, PyObject *obj)
{
    if (!PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "expect int, got %s",
                     Py_TYPE(obj)->tp_name);
        return -1;
    }

    static_assert(sizeof(long long) == sizeof(PyTime_t),
                  "PyTime_t is not long long");
    long long nsec = PyLong_AsLongLong(obj);
    if (nsec == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            pytime_overflow();
        }
        return -1;
    }

    PyTime_t t = (PyTime_t)nsec;
    *tp = t;
    return 0;
}


#ifdef HAVE_CLOCK_GETTIME
static int
pytime_fromtimespec(PyTime_t *tp, const struct timespec *ts, int raise_exc)
{
    PyTime_t t, tv_nsec;

    static_assert(sizeof(ts->tv_sec) <= sizeof(PyTime_t),
                  "timespec.tv_sec is larger than PyTime_t");
    t = (PyTime_t)ts->tv_sec;

    int res1 = pytime_mul(&t, SEC_TO_NS);

    tv_nsec = ts->tv_nsec;
    int res2 = pytime_add(&t, tv_nsec);

    *tp = t;

    if (raise_exc && (res1 < 0 || res2 < 0)) {
        pytime_overflow();
        return -1;
    }
    return 0;
}

int
_PyTime_FromTimespec(PyTime_t *tp, const struct timespec *ts)
{
    return pytime_fromtimespec(tp, ts, 1);
}
#endif


#ifndef MS_WINDOWS
static int
pytime_fromtimeval(PyTime_t *tp, struct timeval *tv, int raise_exc)
{
    static_assert(sizeof(tv->tv_sec) <= sizeof(PyTime_t),
                  "timeval.tv_sec is larger than PyTime_t");
    PyTime_t t = (PyTime_t)tv->tv_sec;

    int res1 = pytime_mul(&t, SEC_TO_NS);

    PyTime_t usec = (PyTime_t)tv->tv_usec * US_TO_NS;
    int res2 = pytime_add(&t, usec);

    *tp = t;

    if (raise_exc && (res1 < 0 || res2 < 0)) {
        pytime_overflow();
        return -1;
    }
    return 0;
}


int
_PyTime_FromTimeval(PyTime_t *tp, struct timeval *tv)
{
    return pytime_fromtimeval(tp, tv, 1);
}
#endif


static int
pytime_from_double(PyTime_t *tp, double value, _PyTime_round_t round,
                   long unit_to_ns)
{
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double d;

    /* convert to a number of nanoseconds */
    d = value;
    d *= (double)unit_to_ns;
    d = pytime_round(d, round);

    /* See comments in pytime_double_to_denominator */
    if (!((double)PyTime_MIN <= d && d < -(double)PyTime_MIN)) {
        pytime_time_t_overflow();
        *tp = 0;
        return -1;
    }
    PyTime_t ns = (PyTime_t)d;

    *tp = ns;
    return 0;
}


static int
pytime_from_object(PyTime_t *tp, PyObject *obj, _PyTime_round_t round,
                   long unit_to_ns)
{
    if (PyFloat_Check(obj)) {
        double d;
        d = PyFloat_AsDouble(obj);
        if (isnan(d)) {
            PyErr_SetString(PyExc_ValueError, "Invalid value NaN (not a number)");
            return -1;
        }
        return pytime_from_double(tp, d, round, unit_to_ns);
    }

    long long sec = PyLong_AsLongLong(obj);
    if (sec == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            pytime_overflow();
        }
        else if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Format(PyExc_TypeError,
                         "'%T' object cannot be interpreted as an integer or float",
                         obj);
        }
        return -1;
    }

    static_assert(sizeof(long long) <= sizeof(PyTime_t),
                  "PyTime_t is smaller than long long");
    PyTime_t ns = (PyTime_t)sec;
    if (pytime_mul(&ns, unit_to_ns) < 0) {
        pytime_overflow();
        return -1;
    }

    *tp = ns;
    return 0;
}


int
_PyTime_FromSecondsObject(PyTime_t *tp, PyObject *obj, _PyTime_round_t round)
{
    return pytime_from_object(tp, obj, round, SEC_TO_NS);
}


int
_PyTime_FromMillisecondsObject(PyTime_t *tp, PyObject *obj, _PyTime_round_t round)
{
    return pytime_from_object(tp, obj, round, MS_TO_NS);
}


double
PyTime_AsSecondsDouble(PyTime_t ns)
{
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double d;

    if (ns % SEC_TO_NS == 0) {
        /* Divide using integers to avoid rounding issues on the integer part.
           1e-9 cannot be stored exactly in IEEE 64-bit. */
        PyTime_t secs = ns / SEC_TO_NS;
        d = (double)secs;
    }
    else {
        d = (double)ns;
        d /= 1e9;
    }
    return d;
}


PyObject *
_PyTime_AsLong(PyTime_t ns)
{
    static_assert(sizeof(long long) >= sizeof(PyTime_t),
                  "PyTime_t is larger than long long");
    return PyLong_FromLongLong((long long)ns);
}

int
_PyTime_FromSecondsDouble(double seconds, _PyTime_round_t round, PyTime_t *result)
{
    return pytime_from_double(result, seconds, round, SEC_TO_NS);
}


static PyTime_t
pytime_divide_round_up(const PyTime_t t, const PyTime_t k)
{
    assert(k > 1);
    if (t >= 0) {
        // Don't use (t + k - 1) / k to avoid integer overflow
        // if t is equal to PyTime_MAX
        PyTime_t q = t / k;
        if (t % k) {
            q += 1;
        }
        return q;
    }
    else {
        // Don't use (t - (k - 1)) / k to avoid integer overflow
        // if t is equals to PyTime_MIN.
        PyTime_t q = t / k;
        if (t % k) {
            q -= 1;
        }
        return q;
    }
}


static PyTime_t
pytime_divide(const PyTime_t t, const PyTime_t k,
              const _PyTime_round_t round)
{
    assert(k > 1);
    if (round == _PyTime_ROUND_HALF_EVEN) {
        PyTime_t x = t / k;
        PyTime_t r = t % k;
        PyTime_t abs_r = Py_ABS(r);
        if (abs_r > k / 2 || (abs_r == k / 2 && (Py_ABS(x) & 1))) {
            if (t >= 0) {
                x++;
            }
            else {
                x--;
            }
        }
        return x;
    }
    else if (round == _PyTime_ROUND_CEILING) {
        if (t >= 0) {
            return pytime_divide_round_up(t, k);
        }
        else {
            return t / k;
        }
    }
    else if (round == _PyTime_ROUND_FLOOR){
        if (t >= 0) {
            return t / k;
        }
        else {
            return pytime_divide_round_up(t, k);
        }
    }
    else {
        assert(round == _PyTime_ROUND_UP);
        return pytime_divide_round_up(t, k);
    }
}


// Compute (t / k, t % k) in (pq, pr).
// Make sure that 0 <= pr < k.
// Return 0 on success.
// Return -1 on underflow and store (PyTime_MIN, 0) in (pq, pr).
static int
pytime_divmod(const PyTime_t t, const PyTime_t k,
              PyTime_t *pq, PyTime_t *pr)
{
    assert(k > 1);
    PyTime_t q = t / k;
    PyTime_t r = t % k;
    if (r < 0) {
        if (q == PyTime_MIN) {
            *pq = PyTime_MIN;
            *pr = 0;
            return -1;
        }
        r += k;
        q -= 1;
    }
    assert(0 <= r && r < k);

    *pq = q;
    *pr = r;
    return 0;
}


#ifdef MS_WINDOWS
PyTime_t
_PyTime_As100Nanoseconds(PyTime_t ns, _PyTime_round_t round)
{
    return pytime_divide(ns, NS_TO_100NS, round);
}
#endif


PyTime_t
_PyTime_AsMicroseconds(PyTime_t ns, _PyTime_round_t round)
{
    return pytime_divide(ns, NS_TO_US, round);
}


PyTime_t
_PyTime_AsMilliseconds(PyTime_t ns, _PyTime_round_t round)
{
    return pytime_divide(ns, NS_TO_MS, round);
}


static int
pytime_as_timeval(PyTime_t ns, PyTime_t *ptv_sec, int *ptv_usec,
                  _PyTime_round_t round)
{
    PyTime_t us = pytime_divide(ns, US_TO_NS, round);

    PyTime_t tv_sec, tv_usec;
    int res = pytime_divmod(us, SEC_TO_US, &tv_sec, &tv_usec);
    *ptv_sec = tv_sec;
    *ptv_usec = (int)tv_usec;
    return res;
}


static int
pytime_as_timeval_struct(PyTime_t t, struct timeval *tv,
                         _PyTime_round_t round, int raise_exc)
{
    PyTime_t tv_sec;
    int tv_usec;
    int res = pytime_as_timeval(t, &tv_sec, &tv_usec, round);
    int res2;
#ifdef MS_WINDOWS
    // On Windows, timeval.tv_sec type is long
    res2 = _PyTime_AsCLong(tv_sec, &tv->tv_sec);
#else
    res2 = _PyTime_AsTime_t(tv_sec, &tv->tv_sec);
#endif
    if (res2 < 0) {
        tv_usec = 0;
    }
    tv->tv_usec = tv_usec;

    if (raise_exc && (res < 0 || res2 < 0)) {
        pytime_time_t_overflow();
        return -1;
    }
    return 0;
}


int
_PyTime_AsTimeval(PyTime_t t, struct timeval *tv, _PyTime_round_t round)
{
    return pytime_as_timeval_struct(t, tv, round, 1);
}


void
_PyTime_AsTimeval_clamp(PyTime_t t, struct timeval *tv, _PyTime_round_t round)
{
    (void)pytime_as_timeval_struct(t, tv, round, 0);
}


int
_PyTime_AsTimevalTime_t(PyTime_t t, time_t *p_secs, int *us,
                        _PyTime_round_t round)
{
    PyTime_t secs;
    if (pytime_as_timeval(t, &secs, us, round) < 0) {
        pytime_time_t_overflow();
        return -1;
    }

    if (_PyTime_AsTime_t(secs, p_secs) < 0) {
        pytime_time_t_overflow();
        return -1;
    }
    return 0;
}


#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_KQUEUE)
static int
pytime_as_timespec(PyTime_t ns, struct timespec *ts, int raise_exc)
{
    PyTime_t tv_sec, tv_nsec;
    int res = pytime_divmod(ns, SEC_TO_NS, &tv_sec, &tv_nsec);

    int res2 = _PyTime_AsTime_t(tv_sec, &ts->tv_sec);
    if (res2 < 0) {
        tv_nsec = 0;
    }
    ts->tv_nsec = tv_nsec;

    if (raise_exc && (res < 0 || res2 < 0)) {
        pytime_time_t_overflow();
        return -1;
    }
    return 0;
}

void
_PyTime_AsTimespec_clamp(PyTime_t t, struct timespec *ts)
{
    (void)pytime_as_timespec(t, ts, 0);
}

int
_PyTime_AsTimespec(PyTime_t t, struct timespec *ts)
{
    return pytime_as_timespec(t, ts, 1);
}
#endif


// N.B. If raise_exc=0, this may be called without a thread state.
static int
py_get_system_clock(PyTime_t *tp, _Py_clock_info_t *info, int raise_exc)
{
    assert(info == NULL || raise_exc);
    if (raise_exc) {
        // raise_exc requires to hold a thread state
        _Py_AssertHoldsTstate();
    }

#ifdef MS_WINDOWS
    FILETIME system_time;
    ULARGE_INTEGER large;

    GetSystemTimePreciseAsFileTime(&system_time);
    large.u.LowPart = system_time.dwLowDateTime;
    large.u.HighPart = system_time.dwHighDateTime;
    /* 11,644,473,600,000,000,000: number of nanoseconds between
       the 1st january 1601 and the 1st january 1970 (369 years + 89 leap
       days). */
    PyTime_t ns = (large.QuadPart - 116444736000000000) * 100;
    *tp = ns;
    if (info) {
        // GetSystemTimePreciseAsFileTime() is implemented using
        // QueryPerformanceCounter() internally.
        info->implementation = "GetSystemTimePreciseAsFileTime()";
        info->monotonic = 0;
        info->resolution = _PyTimeFraction_Resolution(&_PyRuntime.time.base);
        info->adjustable = 1;
    }

#else   /* MS_WINDOWS */
    int err;
#if defined(HAVE_CLOCK_GETTIME)
    struct timespec ts;
#endif

#if !defined(HAVE_CLOCK_GETTIME) || defined(__APPLE__)
    struct timeval tv;
#endif

#ifdef HAVE_CLOCK_GETTIME

#ifdef HAVE_CLOCK_GETTIME_RUNTIME
    if (HAVE_CLOCK_GETTIME_RUNTIME) {
#endif

    err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err) {
        if (raise_exc) {
            PyErr_SetFromErrno(PyExc_OSError);
        }
        return -1;
    }
    if (pytime_fromtimespec(tp, &ts, raise_exc) < 0) {
        return -1;
    }

    if (info) {
        struct timespec res;
        info->implementation = "clock_gettime(CLOCK_REALTIME)";
        info->monotonic = 0;
        info->adjustable = 1;
        if (clock_getres(CLOCK_REALTIME, &res) == 0) {
            info->resolution = (double)res.tv_sec + (double)res.tv_nsec * 1e-9;
        }
        else {
            info->resolution = 1e-9;
        }
    }

#ifdef HAVE_CLOCK_GETTIME_RUNTIME
    }
    else {
#endif

#endif

#if !defined(HAVE_CLOCK_GETTIME) || defined(HAVE_CLOCK_GETTIME_RUNTIME)

     /* test gettimeofday() */
    err = gettimeofday(&tv, (struct timezone *)NULL);
    if (err) {
        if (raise_exc) {
            PyErr_SetFromErrno(PyExc_OSError);
        }
        return -1;
    }
    if (pytime_fromtimeval(tp, &tv, raise_exc) < 0) {
        return -1;
    }

    if (info) {
        info->implementation = "gettimeofday()";
        info->resolution = 1e-6;
        info->monotonic = 0;
        info->adjustable = 1;
    }

#if defined(HAVE_CLOCK_GETTIME_RUNTIME) && defined(HAVE_CLOCK_GETTIME)
    } /* end of availability block */
#endif

#endif   /* !HAVE_CLOCK_GETTIME */
#endif   /* !MS_WINDOWS */
    return 0;
}


int
PyTime_Time(PyTime_t *result)
{
    if (py_get_system_clock(result, NULL, 1) < 0) {
        *result = 0;
        return -1;
    }
    return 0;
}


int
PyTime_TimeRaw(PyTime_t *result)
{
    if (py_get_system_clock(result, NULL, 0) < 0) {
        *result = 0;
        return -1;
    }
    return 0;
}


int
_PyTime_TimeWithInfo(PyTime_t *t, _Py_clock_info_t *info)
{
    return py_get_system_clock(t, info, 1);
}


#ifdef MS_WINDOWS
static PyStatus
py_win_perf_counter_frequency(_PyTimeFraction *base)
{
    LARGE_INTEGER freq;
    // Since Windows XP, the function cannot fail.
    (void)QueryPerformanceFrequency(&freq);
    LONGLONG frequency = freq.QuadPart;

    // Since Windows XP, frequency cannot be zero.
    assert(frequency >= 1);

    Py_BUILD_ASSERT(sizeof(PyTime_t) == sizeof(frequency));
    PyTime_t denom = (PyTime_t)frequency;

    // Known QueryPerformanceFrequency() values:
    //
    // * 10,000,000 (10 MHz): 100 ns resolution
    // * 3,579,545 Hz (3.6 MHz): 279 ns resolution
    if (_PyTimeFraction_Set(base, SEC_TO_NS, denom) < 0) {
        return _PyStatus_ERR("invalid QueryPerformanceFrequency");
    }
    return PyStatus_Ok();
}


// N.B. If raise_exc=0, this may be called without the GIL.
static int
py_get_win_perf_counter(PyTime_t *tp, _Py_clock_info_t *info, int raise_exc)
{
    assert(info == NULL || raise_exc);

    if (info) {
        info->implementation = "QueryPerformanceCounter()";
        info->resolution = _PyTimeFraction_Resolution(&_PyRuntime.time.base);
        info->monotonic = 1;
        info->adjustable = 0;
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    LONGLONG ticksll = now.QuadPart;

    /* Make sure that casting LONGLONG to PyTime_t cannot overflow,
       both types are signed */
    PyTime_t ticks;
    static_assert(sizeof(ticksll) <= sizeof(ticks),
                  "LONGLONG is larger than PyTime_t");
    ticks = (PyTime_t)ticksll;

    *tp = _PyTimeFraction_Mul(ticks, &_PyRuntime.time.base);
    return 0;
}
#endif  // MS_WINDOWS


#ifdef __APPLE__
static PyStatus
py_mach_timebase_info(_PyTimeFraction *base)
{
    mach_timebase_info_data_t timebase;
    // According to the Technical Q&A QA1398, mach_timebase_info() cannot
    // fail: https://developer.apple.com/library/mac/#qa/qa1398/
    (void)mach_timebase_info(&timebase);

    // Check that timebase.numer and timebase.denom can be casted to
    // PyTime_t. In practice, timebase uses uint32_t, so casting cannot
    // overflow. At the end, only make sure that the type is uint32_t
    // (PyTime_t is 64-bit long).
    Py_BUILD_ASSERT(sizeof(timebase.numer) <= sizeof(PyTime_t));
    Py_BUILD_ASSERT(sizeof(timebase.denom) <= sizeof(PyTime_t));
    PyTime_t numer = (PyTime_t)timebase.numer;
    PyTime_t denom = (PyTime_t)timebase.denom;

    // Known time bases:
    //
    // * (1, 1) on Intel: 1 ns
    // * (1000000000, 33333335) on PowerPC: ~30 ns
    // * (1000000000, 25000000) on PowerPC: 40 ns
    if (_PyTimeFraction_Set(base, numer, denom) < 0) {
        return _PyStatus_ERR("invalid mach_timebase_info");
    }
    return PyStatus_Ok();
}
#endif

PyStatus
_PyTime_Init(struct _Py_time_runtime_state *state)
{
#ifdef MS_WINDOWS
    return py_win_perf_counter_frequency(&state->base);
#elif defined(__APPLE__)
    return py_mach_timebase_info(&state->base);
#else
    return PyStatus_Ok();
#endif
}

// N.B. If raise_exc=0, this may be called without a thread state.
static int
py_get_monotonic_clock(PyTime_t *tp, _Py_clock_info_t *info, int raise_exc)
{
    assert(info == NULL || raise_exc);
    if (raise_exc) {
        // raise_exc requires to hold a thread state
        _Py_AssertHoldsTstate();
    }

#if defined(MS_WINDOWS)
    if (py_get_win_perf_counter(tp, info, raise_exc) < 0) {
        return -1;
    }
#elif defined(__APPLE__)
    if (info) {
        info->implementation = "mach_absolute_time()";
        info->resolution = _PyTimeFraction_Resolution(&_PyRuntime.time.base);
        info->monotonic = 1;
        info->adjustable = 0;
    }

    uint64_t uticks = mach_absolute_time();
    // unsigned => signed
    assert(uticks <= (uint64_t)PyTime_MAX);
    PyTime_t ticks = (PyTime_t)uticks;

    PyTime_t ns = _PyTimeFraction_Mul(ticks, &_PyRuntime.time.base);
    *tp = ns;

#elif defined(__hpux)
    hrtime_t time = gethrtime();
    if (time == -1) {
        if (raise_exc) {
            PyErr_SetFromErrno(PyExc_OSError);
        }
        return -1;
    }

    *tp = time;

    if (info) {
        info->implementation = "gethrtime()";
        info->resolution = 1e-9;
        info->monotonic = 1;
        info->adjustable = 0;
    }

#else

#ifdef CLOCK_HIGHRES
    const clockid_t clk_id = CLOCK_HIGHRES;
    const char *implementation = "clock_gettime(CLOCK_HIGHRES)";
#else
    const clockid_t clk_id = CLOCK_MONOTONIC;
    const char *implementation = "clock_gettime(CLOCK_MONOTONIC)";
#endif

    struct timespec ts;
    if (clock_gettime(clk_id, &ts) != 0) {
        if (raise_exc) {
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }
        return -1;
    }

    if (pytime_fromtimespec(tp, &ts, raise_exc) < 0) {
        return -1;
    }

    if (info) {
        info->monotonic = 1;
        info->implementation = implementation;
        info->adjustable = 0;
        struct timespec res;
        if (clock_getres(clk_id, &res) != 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }
        info->resolution = res.tv_sec + res.tv_nsec * 1e-9;
    }
#endif
    return 0;
}


int
PyTime_Monotonic(PyTime_t *result)
{
    if (py_get_monotonic_clock(result, NULL, 1) < 0) {
        *result = 0;
        return -1;
    }
    return 0;
}


int
PyTime_MonotonicRaw(PyTime_t *result)
{
    if (py_get_monotonic_clock(result, NULL, 0) < 0) {
        *result = 0;
        return -1;
    }
    return 0;
}


int
_PyTime_MonotonicWithInfo(PyTime_t *tp, _Py_clock_info_t *info)
{
    return py_get_monotonic_clock(tp, info, 1);
}


int
_PyTime_PerfCounterWithInfo(PyTime_t *t, _Py_clock_info_t *info)
{
    return _PyTime_MonotonicWithInfo(t, info);
}


int
PyTime_PerfCounter(PyTime_t *result)
{
    return PyTime_Monotonic(result);
}


int
PyTime_PerfCounterRaw(PyTime_t *result)
{
    return PyTime_MonotonicRaw(result);
}


int
_PyTime_localtime(time_t t, struct tm *tm)
{
#ifdef MS_WINDOWS
    int error;

    error = localtime_s(tm, &t);
    if (error != 0) {
        errno = error;
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;
#else /* !MS_WINDOWS */

#if defined(_AIX) && (SIZEOF_TIME_T < 8)
    /* bpo-34373: AIX does not return NULL if t is too small or too large */
    if (t < -2145916800 /* 1902-01-01 */
       || t > 2145916800 /* 2038-01-01 */) {
        errno = EINVAL;
        PyErr_SetString(PyExc_OverflowError,
                        "localtime argument out of range");
        return -1;
    }
#endif

    errno = 0;
    if (localtime_r(&t, tm) == NULL) {
        if (errno == 0) {
            errno = EINVAL;
        }
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;
#endif /* MS_WINDOWS */
}


int
_PyTime_gmtime(time_t t, struct tm *tm)
{
#ifdef MS_WINDOWS
    int error;

    error = gmtime_s(tm, &t);
    if (error != 0) {
        errno = error;
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;
#else /* !MS_WINDOWS */
    if (gmtime_r(&t, tm) == NULL) {
#ifdef EINVAL
        if (errno == 0) {
            errno = EINVAL;
        }
#endif
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;
#endif /* MS_WINDOWS */
}


PyTime_t
_PyDeadline_Init(PyTime_t timeout)
{
    PyTime_t now;
    // silently ignore error: cannot report error to the caller
    (void)PyTime_MonotonicRaw(&now);
    return _PyTime_Add(now, timeout);
}


PyTime_t
_PyDeadline_Get(PyTime_t deadline)
{
    PyTime_t now;
    // silently ignore error: cannot report error to the caller
    (void)PyTime_MonotonicRaw(&now);
    return deadline - now;
}
