#include "Python.h"
#ifdef MS_WINDOWS
#include <winsock2.h>         /* struct timeval */
#endif

#if defined(__APPLE__)
#include <mach/mach_time.h>   /* mach_absolute_time(), mach_timebase_info() */

#if defined(__APPLE__) && defined(__has_builtin)
#  if __has_builtin(__builtin_available)
#    define HAVE_CLOCK_GETTIME_RUNTIME __builtin_available(macOS 10.12, iOS 10.0, tvOS 10.0, watchOS 3.0, *)
#  endif
#endif
#endif

#define _PyTime_check_mul_overflow(a, b) \
    (assert(b > 0), \
     (_PyTime_t)(a) < _PyTime_MIN / (_PyTime_t)(b) \
     || _PyTime_MAX / (_PyTime_t)(b) < (_PyTime_t)(a))

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

static void
error_time_t_overflow(void)
{
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp out of range for platform time_t");
}

static void
_PyTime_overflow(void)
{
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp too large to convert to C _PyTime_t");
}


_PyTime_t
_PyTime_MulDiv(_PyTime_t ticks, _PyTime_t mul, _PyTime_t div)
{
    _PyTime_t intpart, remaining;
    /* Compute (ticks * mul / div) in two parts to prevent integer overflow:
       compute integer part, and then the remaining part.

       (ticks * mul) / div == (ticks / div) * mul + (ticks % div) * mul / div

       The caller must ensure that "(div - 1) * mul" cannot overflow. */
    intpart = ticks / div;
    ticks %= div;
    remaining = ticks * mul;
    remaining /= div;
    return intpart * mul + remaining;
}


time_t
_PyLong_AsTime_t(PyObject *obj)
{
#if SIZEOF_TIME_T == SIZEOF_LONG_LONG
    long long val;
    val = PyLong_AsLongLong(obj);
#else
    long val;
    Py_BUILD_ASSERT(sizeof(time_t) <= sizeof(long));
    val = PyLong_AsLong(obj);
#endif
    if (val == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            error_time_t_overflow();
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
#else
    Py_BUILD_ASSERT(sizeof(time_t) <= sizeof(long));
    return PyLong_FromLong((long)t);
#endif
}

/* Round to nearest with ties going to nearest even integer
   (_PyTime_ROUND_HALF_EVEN) */
static double
_PyTime_RoundHalfEven(double x)
{
    double rounded = round(x);
    if (fabs(x-rounded) == 0.5) {
        /* halfway case: round to even */
        rounded = 2.0*round(x/2.0);
    }
    return rounded;
}

static double
_PyTime_Round(double x, _PyTime_round_t round)
{
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double d;

    d = x;
    if (round == _PyTime_ROUND_HALF_EVEN) {
        d = _PyTime_RoundHalfEven(d);
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
_PyTime_DoubleToDenominator(double d, time_t *sec, long *numerator,
                            long idenominator, _PyTime_round_t round)
{
    double denominator = (double)idenominator;
    double intpart;
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double floatpart;

    floatpart = modf(d, &intpart);

    floatpart *= denominator;
    floatpart = _PyTime_Round(floatpart, round);
    if (floatpart >= denominator) {
        floatpart -= denominator;
        intpart += 1.0;
    }
    else if (floatpart < 0) {
        floatpart += denominator;
        intpart -= 1.0;
    }
    assert(0.0 <= floatpart && floatpart < denominator);

    if (!_Py_InIntegralTypeRange(time_t, intpart)) {
        error_time_t_overflow();
        return -1;
    }
    *sec = (time_t)intpart;
    *numerator = (long)floatpart;
    assert(0 <= *numerator && *numerator < idenominator);
    return 0;
}

static int
_PyTime_ObjectToDenominator(PyObject *obj, time_t *sec, long *numerator,
                            long denominator, _PyTime_round_t round)
{
    assert(denominator >= 1);

    if (PyFloat_Check(obj)) {
        double d = PyFloat_AsDouble(obj);
        if (Py_IS_NAN(d)) {
            *numerator = 0;
            PyErr_SetString(PyExc_ValueError, "Invalid value NaN (not a number)");
            return -1;
        }
        return _PyTime_DoubleToDenominator(d, sec, numerator,
                                           denominator, round);
    }
    else {
        *sec = _PyLong_AsTime_t(obj);
        *numerator = 0;
        if (*sec == (time_t)-1 && PyErr_Occurred()) {
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
        if (Py_IS_NAN(d)) {
            PyErr_SetString(PyExc_ValueError, "Invalid value NaN (not a number)");
            return -1;
        }

        d = _PyTime_Round(d, round);
        (void)modf(d, &intpart);

        if (!_Py_InIntegralTypeRange(time_t, intpart)) {
            error_time_t_overflow();
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
    return _PyTime_ObjectToDenominator(obj, sec, nsec, SEC_TO_NS, round);
}

int
_PyTime_ObjectToTimeval(PyObject *obj, time_t *sec, long *usec,
                        _PyTime_round_t round)
{
    return _PyTime_ObjectToDenominator(obj, sec, usec, SEC_TO_US, round);
}

_PyTime_t
_PyTime_FromSeconds(int seconds)
{
    _PyTime_t t;
    /* ensure that integer overflow cannot happen, int type should have 32
       bits, whereas _PyTime_t type has at least 64 bits (SEC_TO_MS takes 30
       bits). */
    Py_BUILD_ASSERT(INT_MAX <= _PyTime_MAX / SEC_TO_NS);
    Py_BUILD_ASSERT(INT_MIN >= _PyTime_MIN / SEC_TO_NS);

    t = (_PyTime_t)seconds;
    assert((t >= 0 && t <= _PyTime_MAX / SEC_TO_NS)
           || (t < 0 && t >= _PyTime_MIN / SEC_TO_NS));
    t *= SEC_TO_NS;
    return t;
}

_PyTime_t
_PyTime_FromNanoseconds(_PyTime_t ns)
{
    /* _PyTime_t already uses nanosecond resolution, no conversion needed */
    return ns;
}

int
_PyTime_FromNanosecondsObject(_PyTime_t *tp, PyObject *obj)
{
    long long nsec;
    _PyTime_t t;

    if (!PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "expect int, got %s",
                     Py_TYPE(obj)->tp_name);
        return -1;
    }

    Py_BUILD_ASSERT(sizeof(long long) == sizeof(_PyTime_t));
    nsec = PyLong_AsLongLong(obj);
    if (nsec == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            _PyTime_overflow();
        }
        return -1;
    }

    /* _PyTime_t already uses nanosecond resolution, no conversion needed */
    t = (_PyTime_t)nsec;
    *tp = t;
    return 0;
}

#ifdef HAVE_CLOCK_GETTIME
static int
pytime_fromtimespec(_PyTime_t *tp, struct timespec *ts, int raise)
{
    _PyTime_t t, nsec;
    int res = 0;

    Py_BUILD_ASSERT(sizeof(ts->tv_sec) <= sizeof(_PyTime_t));
    t = (_PyTime_t)ts->tv_sec;

    if (_PyTime_check_mul_overflow(t, SEC_TO_NS)) {
        if (raise) {
            _PyTime_overflow();
        }
        res = -1;
        t = (t > 0) ? _PyTime_MAX : _PyTime_MIN;
    }
    else {
        t = t * SEC_TO_NS;
    }

    nsec = ts->tv_nsec;
    /* The following test is written for positive only nsec */
    assert(nsec >= 0);
    if (t > _PyTime_MAX - nsec) {
        if (raise) {
            _PyTime_overflow();
        }
        res = -1;
        t = _PyTime_MAX;
    }
    else {
        t += nsec;
    }

    *tp = t;
    return res;
}

int
_PyTime_FromTimespec(_PyTime_t *tp, struct timespec *ts)
{
    return pytime_fromtimespec(tp, ts, 1);
}
#endif

#if !defined(MS_WINDOWS)
static int
pytime_fromtimeval(_PyTime_t *tp, struct timeval *tv, int raise)
{
    _PyTime_t t, usec;
    int res = 0;

    Py_BUILD_ASSERT(sizeof(tv->tv_sec) <= sizeof(_PyTime_t));
    t = (_PyTime_t)tv->tv_sec;

    if (_PyTime_check_mul_overflow(t, SEC_TO_NS)) {
        if (raise) {
            _PyTime_overflow();
        }
        res = -1;
        t = (t > 0) ? _PyTime_MAX : _PyTime_MIN;
    }
    else {
        t = t * SEC_TO_NS;
    }

    usec = (_PyTime_t)tv->tv_usec * US_TO_NS;
    /* The following test is written for positive only usec */
    assert(usec >= 0);
    if (t > _PyTime_MAX - usec) {
        if (raise) {
            _PyTime_overflow();
        }
        res = -1;
        t = _PyTime_MAX;
    }
    else {
        t += usec;
    }

    *tp = t;
    return res;
}

int
_PyTime_FromTimeval(_PyTime_t *tp, struct timeval *tv)
{
    return pytime_fromtimeval(tp, tv, 1);
}
#endif

static int
_PyTime_FromDouble(_PyTime_t *t, double value, _PyTime_round_t round,
                   long unit_to_ns)
{
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double d;

    /* convert to a number of nanoseconds */
    d = value;
    d *= (double)unit_to_ns;
    d = _PyTime_Round(d, round);

    if (!_Py_InIntegralTypeRange(_PyTime_t, d)) {
        _PyTime_overflow();
        return -1;
    }
    *t = (_PyTime_t)d;
    return 0;
}

static int
_PyTime_FromObject(_PyTime_t *t, PyObject *obj, _PyTime_round_t round,
                   long unit_to_ns)
{
    if (PyFloat_Check(obj)) {
        double d;
        d = PyFloat_AsDouble(obj);
        if (Py_IS_NAN(d)) {
            PyErr_SetString(PyExc_ValueError, "Invalid value NaN (not a number)");
            return -1;
        }
        return _PyTime_FromDouble(t, d, round, unit_to_ns);
    }
    else {
        long long sec;
        Py_BUILD_ASSERT(sizeof(long long) <= sizeof(_PyTime_t));

        sec = PyLong_AsLongLong(obj);
        if (sec == -1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                _PyTime_overflow();
            }
            return -1;
        }

        if (_PyTime_check_mul_overflow(sec, unit_to_ns)) {
            _PyTime_overflow();
            return -1;
        }
        *t = sec * unit_to_ns;
        return 0;
    }
}

int
_PyTime_FromSecondsObject(_PyTime_t *t, PyObject *obj, _PyTime_round_t round)
{
    return _PyTime_FromObject(t, obj, round, SEC_TO_NS);
}

int
_PyTime_FromMillisecondsObject(_PyTime_t *t, PyObject *obj, _PyTime_round_t round)
{
    return _PyTime_FromObject(t, obj, round, MS_TO_NS);
}

double
_PyTime_AsSecondsDouble(_PyTime_t t)
{
    /* volatile avoids optimization changing how numbers are rounded */
    volatile double d;

    if (t % SEC_TO_NS == 0) {
        _PyTime_t secs;
        /* Divide using integers to avoid rounding issues on the integer part.
           1e-9 cannot be stored exactly in IEEE 64-bit. */
        secs = t / SEC_TO_NS;
        d = (double)secs;
    }
    else {
        d = (double)t;
        d /= 1e9;
    }
    return d;
}

PyObject *
_PyTime_AsNanosecondsObject(_PyTime_t t)
{
    Py_BUILD_ASSERT(sizeof(long long) >= sizeof(_PyTime_t));
    return PyLong_FromLongLong((long long)t);
}

static _PyTime_t
_PyTime_Divide(const _PyTime_t t, const _PyTime_t k,
               const _PyTime_round_t round)
{
    assert(k > 1);
    if (round == _PyTime_ROUND_HALF_EVEN) {
        _PyTime_t x, r, abs_r;
        x = t / k;
        r = t % k;
        abs_r = Py_ABS(r);
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
            return (t + k - 1) / k;
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
            return (t - (k - 1)) / k;
        }
    }
    else {
        assert(round == _PyTime_ROUND_UP);
        if (t >= 0) {
            return (t + k - 1) / k;
        }
        else {
            return (t - (k - 1)) / k;
        }
    }
}

_PyTime_t
_PyTime_AsMilliseconds(_PyTime_t t, _PyTime_round_t round)
{
    return _PyTime_Divide(t, NS_TO_MS, round);
}

_PyTime_t
_PyTime_AsMicroseconds(_PyTime_t t, _PyTime_round_t round)
{
    return _PyTime_Divide(t, NS_TO_US, round);
}

static int
_PyTime_AsTimeval_impl(_PyTime_t t, _PyTime_t *p_secs, int *p_us,
                       _PyTime_round_t round)
{
    _PyTime_t secs, ns;
    int usec;
    int res = 0;

    secs = t / SEC_TO_NS;
    ns = t % SEC_TO_NS;

    usec = (int)_PyTime_Divide(ns, US_TO_NS, round);
    if (usec < 0) {
        usec += SEC_TO_US;
        if (secs != _PyTime_MIN) {
            secs -= 1;
        }
        else {
            res = -1;
        }
    }
    else if (usec >= SEC_TO_US) {
        usec -= SEC_TO_US;
        if (secs != _PyTime_MAX) {
            secs += 1;
        }
        else {
            res = -1;
        }
    }
    assert(0 <= usec && usec < SEC_TO_US);

    *p_secs = secs;
    *p_us = usec;

    return res;
}

static int
_PyTime_AsTimevalStruct_impl(_PyTime_t t, struct timeval *tv,
                             _PyTime_round_t round, int raise)
{
    _PyTime_t secs, secs2;
    int us;
    int res;

    res = _PyTime_AsTimeval_impl(t, &secs, &us, round);

#ifdef MS_WINDOWS
    tv->tv_sec = (long)secs;
#else
    tv->tv_sec = secs;
#endif
    tv->tv_usec = us;

    secs2 = (_PyTime_t)tv->tv_sec;
    if (res < 0 || secs2 != secs) {
        if (raise) {
            error_time_t_overflow();
        }
        return -1;
    }
    return 0;
}

int
_PyTime_AsTimeval(_PyTime_t t, struct timeval *tv, _PyTime_round_t round)
{
    return _PyTime_AsTimevalStruct_impl(t, tv, round, 1);
}

int
_PyTime_AsTimeval_noraise(_PyTime_t t, struct timeval *tv, _PyTime_round_t round)
{
    return _PyTime_AsTimevalStruct_impl(t, tv, round, 0);
}

int
_PyTime_AsTimevalTime_t(_PyTime_t t, time_t *p_secs, int *us,
                        _PyTime_round_t round)
{
    _PyTime_t secs;
    int res;

    res = _PyTime_AsTimeval_impl(t, &secs, us, round);

    *p_secs = secs;

    if (res < 0 || (_PyTime_t)*p_secs != secs) {
        error_time_t_overflow();
        return -1;
    }
    return 0;
}


#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_KQUEUE)
int
_PyTime_AsTimespec(_PyTime_t t, struct timespec *ts)
{
    _PyTime_t secs, nsec;

    secs = t / SEC_TO_NS;
    nsec = t % SEC_TO_NS;
    if (nsec < 0) {
        nsec += SEC_TO_NS;
        secs -= 1;
    }
    ts->tv_sec = (time_t)secs;
    assert(0 <= nsec && nsec < SEC_TO_NS);
    ts->tv_nsec = nsec;

    if ((_PyTime_t)ts->tv_sec != secs) {
        error_time_t_overflow();
        return -1;
    }
    return 0;
}
#endif

static int
pygettimeofday(_PyTime_t *tp, _Py_clock_info_t *info, int raise)
{
#ifdef MS_WINDOWS
    FILETIME system_time;
    ULARGE_INTEGER large;

    assert(info == NULL || raise);

    GetSystemTimeAsFileTime(&system_time);
    large.u.LowPart = system_time.dwLowDateTime;
    large.u.HighPart = system_time.dwHighDateTime;
    /* 11,644,473,600,000,000,000: number of nanoseconds between
       the 1st january 1601 and the 1st january 1970 (369 years + 89 leap
       days). */
    *tp = large.QuadPart * 100 - 11644473600000000000;
    if (info) {
        DWORD timeAdjustment, timeIncrement;
        BOOL isTimeAdjustmentDisabled, ok;

        info->implementation = "GetSystemTimeAsFileTime()";
        info->monotonic = 0;
        ok = GetSystemTimeAdjustment(&timeAdjustment, &timeIncrement,
                                     &isTimeAdjustmentDisabled);
        if (!ok) {
            PyErr_SetFromWindowsErr(0);
            return -1;
        }
        info->resolution = timeIncrement * 1e-7;
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

    assert(info == NULL || raise);

#ifdef HAVE_CLOCK_GETTIME

#ifdef HAVE_CLOCK_GETTIME_RUNTIME
    if (HAVE_CLOCK_GETTIME_RUNTIME) {
#endif

    err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err) {
        if (raise) {
            PyErr_SetFromErrno(PyExc_OSError);
        }
        return -1;
    }
    if (pytime_fromtimespec(tp, &ts, raise) < 0) {
        return -1;
    }

    if (info) {
        struct timespec res;
        info->implementation = "clock_gettime(CLOCK_REALTIME)";
        info->monotonic = 0;
        info->adjustable = 1;
        if (clock_getres(CLOCK_REALTIME, &res) == 0) {
            info->resolution = res.tv_sec + res.tv_nsec * 1e-9;
        }
        else {
            info->resolution = 1e-9;
        }
    }

#ifdef HAVE_CLOCK_GETTIME_RUNTIME
    } else {
#endif

#endif

#if !defined(HAVE_CLOCK_GETTIME) || defined(HAVE_CLOCK_GETTIME_RUNTIME)

     /* test gettimeofday() */
    err = gettimeofday(&tv, (struct timezone *)NULL);
    if (err) {
        if (raise) {
            PyErr_SetFromErrno(PyExc_OSError);
        }
        return -1;
    }
    if (pytime_fromtimeval(tp, &tv, raise) < 0) {
        return -1;
    }

    if (info) {
        info->implementation = "gettimeofday()";
        info->resolution = 1e-6;
        info->monotonic = 0;
        info->adjustable = 1;
    }

#if defined(HAVE_CLOCK_GETTIME_RUNTIME) && defined(HAVE_CLOCK_GETTIME)
    } /* end of availibity block */
#endif

#endif   /* !HAVE_CLOCK_GETTIME */
#endif   /* !MS_WINDOWS */
    return 0;
}

_PyTime_t
_PyTime_GetSystemClock(void)
{
    _PyTime_t t;
    if (pygettimeofday(&t, NULL, 0) < 0) {
        /* should not happen, _PyTime_Init() checked the clock at startup */
        Py_FatalError("pygettimeofday() failed");
    }
    return t;
}

int
_PyTime_GetSystemClockWithInfo(_PyTime_t *t, _Py_clock_info_t *info)
{
    return pygettimeofday(t, info, 1);
}

static int
pymonotonic(_PyTime_t *tp, _Py_clock_info_t *info, int raise)
{
#if defined(MS_WINDOWS)
    ULONGLONG ticks;
    _PyTime_t t;

    assert(info == NULL || raise);

    ticks = GetTickCount64();
    Py_BUILD_ASSERT(sizeof(ticks) <= sizeof(_PyTime_t));
    t = (_PyTime_t)ticks;

    if (_PyTime_check_mul_overflow(t, MS_TO_NS)) {
        if (raise) {
            _PyTime_overflow();
            return -1;
        }
        /* Hello, time traveler! */
        Py_FatalError("pymonotonic: integer overflow");
    }
    *tp = t * MS_TO_NS;

    if (info) {
        DWORD timeAdjustment, timeIncrement;
        BOOL isTimeAdjustmentDisabled, ok;
        info->implementation = "GetTickCount64()";
        info->monotonic = 1;
        ok = GetSystemTimeAdjustment(&timeAdjustment, &timeIncrement,
                                     &isTimeAdjustmentDisabled);
        if (!ok) {
            PyErr_SetFromWindowsErr(0);
            return -1;
        }
        info->resolution = timeIncrement * 1e-7;
        info->adjustable = 0;
    }

#elif defined(__APPLE__)
    static mach_timebase_info_data_t timebase;
    static uint64_t t0 = 0;
    uint64_t ticks;

    if (timebase.denom == 0) {
        /* According to the Technical Q&A QA1398, mach_timebase_info() cannot
           fail: https://developer.apple.com/library/mac/#qa/qa1398/ */
        (void)mach_timebase_info(&timebase);

        /* Sanity check: should never occur in practice */
        if (timebase.numer < 1 || timebase.denom < 1) {
            PyErr_SetString(PyExc_RuntimeError,
                            "invalid mach_timebase_info");
            return -1;
        }

        /* Check that timebase.numer and timebase.denom can be casted to
           _PyTime_t. In practice, timebase uses uint32_t, so casting cannot
           overflow. At the end, only make sure that the type is uint32_t
           (_PyTime_t is 64-bit long). */
        assert(sizeof(timebase.numer) < sizeof(_PyTime_t));
        assert(sizeof(timebase.denom) < sizeof(_PyTime_t));

        /* Make sure that (ticks * timebase.numer) cannot overflow in
           _PyTime_MulDiv(), with ticks < timebase.denom.

           Known time bases:

           * always (1, 1) on Intel
           * (1000000000, 33333335) or (1000000000, 25000000) on PowerPC

           None of these time bases can overflow with 64-bit _PyTime_t, but
           check for overflow, just in case. */
        if ((_PyTime_t)timebase.numer > _PyTime_MAX / (_PyTime_t)timebase.denom) {
            PyErr_SetString(PyExc_OverflowError,
                            "mach_timebase_info is too large");
            return -1;
        }

        t0 = mach_absolute_time();
    }

    if (info) {
        info->implementation = "mach_absolute_time()";
        info->resolution = (double)timebase.numer / (double)timebase.denom * 1e-9;
        info->monotonic = 1;
        info->adjustable = 0;
    }

    ticks = mach_absolute_time();
    /* Use a "time zero" to reduce precision loss when converting time
       to floatting point number, as in time.monotonic(). */
    ticks -= t0;
    *tp = _PyTime_MulDiv(ticks,
                         (_PyTime_t)timebase.numer,
                         (_PyTime_t)timebase.denom);

#elif defined(__hpux)
    hrtime_t time;

    time = gethrtime();
    if (time == -1) {
        if (raise) {
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
    struct timespec ts;
#ifdef CLOCK_HIGHRES
    const clockid_t clk_id = CLOCK_HIGHRES;
    const char *implementation = "clock_gettime(CLOCK_HIGHRES)";
#else
    const clockid_t clk_id = CLOCK_MONOTONIC;
    const char *implementation = "clock_gettime(CLOCK_MONOTONIC)";
#endif

    assert(info == NULL || raise);

    if (clock_gettime(clk_id, &ts) != 0) {
        if (raise) {
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }
        return -1;
    }

    if (info) {
        struct timespec res;
        info->monotonic = 1;
        info->implementation = implementation;
        info->adjustable = 0;
        if (clock_getres(clk_id, &res) != 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }
        info->resolution = res.tv_sec + res.tv_nsec * 1e-9;
    }
    if (pytime_fromtimespec(tp, &ts, raise) < 0) {
        return -1;
    }
#endif
    return 0;
}

_PyTime_t
_PyTime_GetMonotonicClock(void)
{
    _PyTime_t t;
    if (pymonotonic(&t, NULL, 0) < 0) {
        /* should not happen, _PyTime_Init() checked that monotonic clock at
           startup */
        Py_FatalError("pymonotonic() failed");
    }
    return t;
}

int
_PyTime_GetMonotonicClockWithInfo(_PyTime_t *tp, _Py_clock_info_t *info)
{
    return pymonotonic(tp, info, 1);
}


#ifdef MS_WINDOWS
static int
win_perf_counter(_PyTime_t *tp, _Py_clock_info_t *info)
{
    static LONGLONG frequency = 0;
    static LONGLONG t0 = 0;
    LARGE_INTEGER now;
    LONGLONG ticksll;
    _PyTime_t ticks;

    if (frequency == 0) {
        LARGE_INTEGER freq;
        if (!QueryPerformanceFrequency(&freq)) {
            PyErr_SetFromWindowsErr(0);
            return -1;
        }
        frequency = freq.QuadPart;

        /* Sanity check: should never occur in practice */
        if (frequency < 1) {
            PyErr_SetString(PyExc_RuntimeError,
                            "invalid QueryPerformanceFrequency");
            return -1;
        }

        /* Check that frequency can be casted to _PyTime_t.

           Make also sure that (ticks * SEC_TO_NS) cannot overflow in
           _PyTime_MulDiv(), with ticks < frequency.

           Known QueryPerformanceFrequency() values:

           * 10,000,000 (10 MHz): 100 ns resolution
           * 3,579,545 Hz (3.6 MHz): 279 ns resolution

           None of these frequencies can overflow with 64-bit _PyTime_t, but
           check for overflow, just in case. */
        if (frequency > _PyTime_MAX
            || frequency > (LONGLONG)_PyTime_MAX / (LONGLONG)SEC_TO_NS) {
            PyErr_SetString(PyExc_OverflowError,
                            "QueryPerformanceFrequency is too large");
            return -1;
        }

        QueryPerformanceCounter(&now);
        t0 = now.QuadPart;
    }

    if (info) {
        info->implementation = "QueryPerformanceCounter()";
        info->resolution = 1.0 / (double)frequency;
        info->monotonic = 1;
        info->adjustable = 0;
    }

    QueryPerformanceCounter(&now);
    ticksll = now.QuadPart;

    /* Use a "time zero" to reduce precision loss when converting time
       to floatting point number, as in time.perf_counter(). */
    ticksll -= t0;

    /* Make sure that casting LONGLONG to _PyTime_t cannot overflow,
       both types are signed */
    Py_BUILD_ASSERT(sizeof(ticksll) <= sizeof(ticks));
    ticks = (_PyTime_t)ticksll;

    *tp = _PyTime_MulDiv(ticks, SEC_TO_NS, (_PyTime_t)frequency);
    return 0;
}
#endif


int
_PyTime_GetPerfCounterWithInfo(_PyTime_t *t, _Py_clock_info_t *info)
{
#ifdef MS_WINDOWS
    return win_perf_counter(t, info);
#else
    return _PyTime_GetMonotonicClockWithInfo(t, info);
#endif
}


_PyTime_t
_PyTime_GetPerfCounter(void)
{
    _PyTime_t t;
    if (_PyTime_GetPerfCounterWithInfo(&t, NULL)) {
        Py_FatalError("_PyTime_GetPerfCounterWithInfo() failed");
    }
    return t;
}


int
_PyTime_Init(void)
{
    /* check that time.time(), time.monotonic() and time.perf_counter() clocks
       are working properly to not have to check for exceptions at runtime. If
       a clock works once, it cannot fail in next calls. */
    _PyTime_t t;
    if (_PyTime_GetSystemClockWithInfo(&t, NULL) < 0) {
        return -1;
    }
    if (_PyTime_GetMonotonicClockWithInfo(&t, NULL) < 0) {
        return -1;
    }
    if (_PyTime_GetPerfCounterWithInfo(&t, NULL) < 0) {
        return -1;
    }
    return 0;
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
