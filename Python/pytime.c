#include "Python.h"
#ifdef MS_WINDOWS
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <mach/mach_time.h>   /* mach_absolute_time(), mach_timebase_info() */
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

static int
pygettimeofday(_PyTime_timeval *tp, _Py_clock_info_t *info, int raise)
{
#ifdef MS_WINDOWS
    FILETIME system_time;
    ULARGE_INTEGER large;
    ULONGLONG microseconds;

    assert(info == NULL || raise);

    GetSystemTimeAsFileTime(&system_time);
    large.u.LowPart = system_time.dwLowDateTime;
    large.u.HighPart = system_time.dwHighDateTime;
    /* 11,644,473,600,000,000: number of microseconds between
       the 1st january 1601 and the 1st january 1970 (369 years + 89 leap
       days). */
    microseconds = large.QuadPart / 10 - 11644473600000000;
    tp->tv_sec = microseconds / SEC_TO_US;
    tp->tv_usec = microseconds % SEC_TO_US;
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
#ifdef HAVE_CLOCK_GETTIME
    struct timespec ts;
#endif

    assert(info == NULL || raise);

#ifdef HAVE_CLOCK_GETTIME
    err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    tp->tv_sec = ts.tv_sec;
    tp->tv_usec = ts.tv_nsec / US_TO_NS;

    if (info) {
        struct timespec res;
        info->implementation = "clock_gettime(CLOCK_REALTIME)";
        info->monotonic = 0;
        info->adjustable = 1;
        if (clock_getres(CLOCK_REALTIME, &res) == 0)
            info->resolution = res.tv_sec + res.tv_nsec * 1e-9;
        else
            info->resolution = 1e-9;
    }
#else   /* HAVE_CLOCK_GETTIME */

     /* test gettimeofday() */
#ifdef GETTIMEOFDAY_NO_TZ
    err = gettimeofday(tp);
#else
    err = gettimeofday(tp, (struct timezone *)NULL);
#endif
    if (err) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (info) {
        info->implementation = "gettimeofday()";
        info->resolution = 1e-6;
        info->monotonic = 0;
        info->adjustable = 1;
    }
#endif   /* !HAVE_CLOCK_GETTIME */
#endif   /* !MS_WINDOWS */
    assert(0 <= tp->tv_usec && tp->tv_usec < SEC_TO_US);
    return 0;
}

void
_PyTime_gettimeofday(_PyTime_timeval *tp)
{
    if (pygettimeofday(tp, NULL, 0) < 0) {
        /* cannot happen, _PyTime_Init() checks that pygettimeofday() works */
        assert(0);
        tp->tv_sec = 0;
        tp->tv_usec = 0;
    }
}

int
_PyTime_gettimeofday_info(_PyTime_timeval *tp, _Py_clock_info_t *info)
{
    return pygettimeofday(tp, info, 1);
}

static int
pymonotonic(_PyTime_timeval *tp, _Py_clock_info_t *info, int raise)
{
#ifdef Py_DEBUG
    static _PyTime_timeval last = {0, -1};
#endif
#if defined(MS_WINDOWS)
    ULONGLONG result;

    assert(info == NULL || raise);

    result = GetTickCount64();

    tp->tv_sec = result / SEC_TO_MS;
    tp->tv_usec = (result % SEC_TO_MS) * MS_TO_US;

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
    uint64_t time;

    if (timebase.denom == 0) {
        /* According to the Technical Q&A QA1398, mach_timebase_info() cannot
           fail: https://developer.apple.com/library/mac/#qa/qa1398/ */
        (void)mach_timebase_info(&timebase);
    }

    time = mach_absolute_time();

    /* nanoseconds => microseconds */
    time /= US_TO_NS;
    /* apply timebase factor */
    time *= timebase.numer;
    time /= timebase.denom;
    tp->tv_sec = time / SEC_TO_US;
    tp->tv_usec = time % SEC_TO_US;

    if (info) {
        info->implementation = "mach_absolute_time()";
        info->resolution = (double)timebase.numer / timebase.denom * 1e-9;
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
        tp->tv_sec = 0;
        tp->tv_usec = 0;
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
    tp->tv_sec = ts.tv_sec;
    tp->tv_usec = ts.tv_nsec / US_TO_NS;
#endif
    assert(0 <= tp->tv_usec && tp->tv_usec < SEC_TO_US);
#ifdef Py_DEBUG
    /* monotonic clock cannot go backward */
    assert(last.tv_usec == -1
           || tp->tv_sec > last.tv_sec
           || (tp->tv_sec == last.tv_sec && tp->tv_usec >= last.tv_usec));
    last = *tp;
#endif
    return 0;
}

void
_PyTime_monotonic(_PyTime_timeval *tp)
{
    if (pymonotonic(tp, NULL, 0) < 0) {
        /* cannot happen, _PyTime_Init() checks that pymonotonic() works */
        assert(0);
        tp->tv_sec = 0;
        tp->tv_usec = 0;
    }
}

int
_PyTime_monotonic_info(_PyTime_timeval *tp, _Py_clock_info_t *info)
{
    return pymonotonic(tp, info, 1);
}

static void
error_time_t_overflow(void)
{
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp out of range for platform time_t");
}

time_t
_PyLong_AsTime_t(PyObject *obj)
{
#if defined(HAVE_LONG_LONG) && SIZEOF_TIME_T == SIZEOF_LONG_LONG
    PY_LONG_LONG val;
    val = PyLong_AsLongLong(obj);
#else
    long val;
    assert(sizeof(time_t) <= sizeof(long));
    val = PyLong_AsLong(obj);
#endif
    if (val == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            error_time_t_overflow();
        return -1;
    }
    return (time_t)val;
}

PyObject *
_PyLong_FromTime_t(time_t t)
{
#if defined(HAVE_LONG_LONG) && SIZEOF_TIME_T == SIZEOF_LONG_LONG
    return PyLong_FromLongLong((PY_LONG_LONG)t);
#else
    assert(sizeof(time_t) <= sizeof(long));
    return PyLong_FromLong((long)t);
#endif
}

static int
_PyTime_ObjectToDenominator(PyObject *obj, time_t *sec, long *numerator,
                            double denominator, _PyTime_round_t round)
{
    assert(denominator <= LONG_MAX);
    if (PyFloat_Check(obj)) {
        double d, intpart, err;
        /* volatile avoids unsafe optimization on float enabled by gcc -O3 */
        volatile double floatpart;

        d = PyFloat_AsDouble(obj);
        floatpart = modf(d, &intpart);
        if (floatpart < 0) {
            floatpart = 1.0 + floatpart;
            intpart -= 1.0;
        }

        floatpart *= denominator;
        if (round == _PyTime_ROUND_UP) {
            if (intpart >= 0) {
                floatpart = ceil(floatpart);
                if (floatpart >= denominator) {
                    floatpart = 0.0;
                    intpart += 1.0;
                }
            }
            else {
                floatpart = floor(floatpart);
            }
        }

        *sec = (time_t)intpart;
        err = intpart - (double)*sec;
        if (err <= -1.0 || err >= 1.0) {
            error_time_t_overflow();
            return -1;
        }

        *numerator = (long)floatpart;
        return 0;
    }
    else {
        *sec = _PyLong_AsTime_t(obj);
        if (*sec == (time_t)-1 && PyErr_Occurred())
            return -1;
        *numerator = 0;
        return 0;
    }
}

int
_PyTime_ObjectToTime_t(PyObject *obj, time_t *sec, _PyTime_round_t round)
{
    if (PyFloat_Check(obj)) {
        double d, intpart, err;

        d = PyFloat_AsDouble(obj);
        if (round == _PyTime_ROUND_UP) {
            if (d >= 0)
                d = ceil(d);
            else
                d = floor(d);
        }
        (void)modf(d, &intpart);

        *sec = (time_t)intpart;
        err = intpart - (double)*sec;
        if (err <= -1.0 || err >= 1.0) {
            error_time_t_overflow();
            return -1;
        }
        return 0;
    }
    else {
        *sec = _PyLong_AsTime_t(obj);
        if (*sec == (time_t)-1 && PyErr_Occurred())
            return -1;
        return 0;
    }
}

int
_PyTime_ObjectToTimespec(PyObject *obj, time_t *sec, long *nsec,
                         _PyTime_round_t round)
{
    return _PyTime_ObjectToDenominator(obj, sec, nsec, 1e9, round);
}

int
_PyTime_ObjectToTimeval(PyObject *obj, time_t *sec, long *usec,
                        _PyTime_round_t round)
{
    return _PyTime_ObjectToDenominator(obj, sec, usec, 1e6, round);
}

void
_PyTime_AddDouble(_PyTime_timeval *tv, double interval, _PyTime_round_t round)
{
    _PyTime_timeval tv2;
    double frac;

    frac = fmod(interval, 1.0);
    interval = floor(interval);
    tv2.tv_sec = (long)interval;
    tv2.tv_usec = (long)(frac*1e6);

    tv->tv_sec += tv2.tv_sec;
    tv->tv_usec += tv2.tv_usec;
    tv->tv_sec += (time_t)(tv->tv_usec / SEC_TO_US);
    tv->tv_usec %= SEC_TO_US;
}

/****************** NEW _PyTime_t API **********************/

static void
_PyTime_overflow(void)
{
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp too large to convert to C _PyTime_t");
}

_PyTime_t
_PyTime_FromNanoseconds(PY_LONG_LONG ns)
{
    _PyTime_t t;
    assert(sizeof(PY_LONG_LONG) <= sizeof(_PyTime_t));
    t = Py_SAFE_DOWNCAST(ns, PY_LONG_LONG, _PyTime_t);
    return t;
}

#if !defined(MS_WINDOWS) && !defined(__APPLE__)
static int
_PyTime_FromTimespec(_PyTime_t *tp, struct timespec *ts)
{
    _PyTime_t t;
    t = (_PyTime_t)ts->tv_sec * SEC_TO_NS;
    if (t / SEC_TO_NS != ts->tv_sec) {
        _PyTime_overflow();
        return -1;
    }

    t += ts->tv_nsec;

    *tp = t;
    return 0;
}
#endif

int
_PyTime_FromSecondsObject(_PyTime_t *t, PyObject *obj, _PyTime_round_t round)
{
    if (PyFloat_Check(obj)) {
        double d, err;

        /* convert to a number of nanoseconds */
        d = PyFloat_AsDouble(obj);
        d *= 1e9;

        if ((round == _PyTime_ROUND_UP) ^ (d < 0))
            d = ceil(d);
        else
            d = floor(d);

        *t = (_PyTime_t)d;
        err = d - (double)*t;
        if (fabs(err) >= 1.0) {
            _PyTime_overflow();
            return -1;
        }
        return 0;
    }
    else {
#ifdef HAVE_LONG_LONG
        PY_LONG_LONG sec;
        sec = PyLong_AsLongLong(obj);
        assert(sizeof(PY_LONG_LONG) <= sizeof(_PyTime_t));
#else
        long sec;
        sec = PyLong_AsLong(obj);
        assert(sizeof(PY_LONG_LONG) <= sizeof(_PyTime_t));
#endif
        if (sec == -1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError))
                _PyTime_overflow();
            return -1;
        }
        *t = sec * SEC_TO_NS;
        if (*t / SEC_TO_NS != sec) {
            _PyTime_overflow();
            return -1;
        }
        return 0;
    }
}

double
_PyTime_AsSecondsDouble(_PyTime_t t)
{
    _PyTime_t sec, ns;
    /* Divide using integers to avoid rounding issues on the integer part.
       1e-9 cannot be stored exactly in IEEE 64-bit. */
    sec = t / SEC_TO_NS;
    ns = t % SEC_TO_NS;
    return (double)sec + (double)ns * 1e-9;
}

PyObject *
_PyTime_AsNanosecondsObject(_PyTime_t t)
{
#ifdef HAVE_LONG_LONG
    assert(sizeof(PY_LONG_LONG) >= sizeof(_PyTime_t));
    return PyLong_FromLongLong((PY_LONG_LONG)t);
#else
    assert(sizeof(long) >= sizeof(_PyTime_t));
    return PyLong_FromLong((long)t);
#endif
}

static _PyTime_t
_PyTime_Multiply(_PyTime_t t, unsigned int multiply, _PyTime_round_t round)
{
    _PyTime_t k;
    if (multiply < SEC_TO_NS) {
        k = SEC_TO_NS / multiply;
        if (round == _PyTime_ROUND_UP)
            return (t + k - 1) / k;
        else
            return t / k;
    }
    else {
        k = multiply / SEC_TO_NS;
        return t * k;
    }
}

_PyTime_t
_PyTime_AsMilliseconds(_PyTime_t t, _PyTime_round_t round)
{
    return _PyTime_Multiply(t, 1000, round);
}

int
_PyTime_AsTimeval(_PyTime_t t, struct timeval *tv, _PyTime_round_t round)
{
    _PyTime_t secs, ns;

    secs = t / SEC_TO_NS;
    ns = t % SEC_TO_NS;

#ifdef MS_WINDOWS
    /* On Windows, timeval.tv_sec is a long (32 bit),
       whereas time_t can be 64-bit. */
    assert(sizeof(tv->tv_sec) == sizeof(long));
#if SIZEOF_TIME_T > SIZEOF_LONG
    if (secs > LONG_MAX) {
        _PyTime_overflow();
        return -1;
    }
#endif
    tv->tv_sec = (long)secs;
#else
    /* On OpenBSD 5.4, timeval.tv_sec is a long.
       Example: long is 64-bit, whereas time_t is 32-bit. */
    tv->tv_sec = secs;
    if ((_PyTime_t)tv->tv_sec != secs) {
        _PyTime_overflow();
        return -1;
    }
#endif

    if (round == _PyTime_ROUND_UP)
        tv->tv_usec = (int)((ns + US_TO_NS - 1) / US_TO_NS);
    else
        tv->tv_usec = (int)(ns / US_TO_NS);
    return 0;
}

static int
pymonotonic_new(_PyTime_t *tp, _Py_clock_info_t *info, int raise)
{
#ifdef Py_DEBUG
    static int last_set = 0;
    static _PyTime_t last = 0;
#endif
#if defined(MS_WINDOWS)
    ULONGLONG result;

    assert(info == NULL || raise);

    result = GetTickCount64();

    *tp = result * MS_TO_NS;
    if (*tp / MS_TO_NS != result) {
        if (raise) {
            _PyTime_overflow();
            return -1;
        }
        /* Hello, time traveler! */
        assert(0);
    }

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
    uint64_t time;

    if (timebase.denom == 0) {
        /* According to the Technical Q&A QA1398, mach_timebase_info() cannot
           fail: https://developer.apple.com/library/mac/#qa/qa1398/ */
        (void)mach_timebase_info(&timebase);
    }

    time = mach_absolute_time();

    /* apply timebase factor */
    time *= timebase.numer;
    time /= timebase.denom;

    *tp = time;

    if (info) {
        info->implementation = "mach_absolute_time()";
        info->resolution = (double)timebase.numer / timebase.denom * 1e-9;
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
    if (_PyTime_FromTimespec(tp, &ts) < 0)
        return -1;
#endif
#ifdef Py_DEBUG
    /* monotonic clock cannot go backward */
    assert(!last_set || last <= *tp);
    last = *tp;
    last_set = 1;
#endif
    return 0;
}

_PyTime_t
_PyTime_GetMonotonicClock(void)
{
    _PyTime_t t;
    if (pymonotonic_new(&t, NULL, 0) < 0) {
        /* cannot happen, _PyTime_Init() checks that pymonotonic_new() works */
        assert(0);
        t = 0;
    }
    return t;
}

int
_PyTime_GetMonotonicClockWithInfo(_PyTime_t *tp, _Py_clock_info_t *info)
{
    return pymonotonic_new(tp, info, 1);
}

int
_PyTime_Init(void)
{
    _PyTime_timeval tv;
    _PyTime_t t;

    /* ensure that the system clock works */
    if (_PyTime_gettimeofday_info(&tv, NULL) < 0)
        return -1;

    /* ensure that the operating system provides a monotonic clock */
    if (_PyTime_monotonic_info(&tv, NULL) < 0)
        return -1;

    /* ensure that the operating system provides a monotonic clock */
    if (pymonotonic_new(&t, NULL, 1) < 0)
        return -1;
    return 0;
}
