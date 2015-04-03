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

/* Conversion from nanoseconds */
#define NS_TO_MS (1000 * 1000)
#define NS_TO_US (1000)

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
        if (round == _PyTime_ROUND_CEILING) {
            floatpart = ceil(floatpart);
            if (floatpart >= denominator) {
                floatpart = 0.0;
                intpart += 1.0;
            }
        }
        else {
            floatpart = floor(floatpart);
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
        if (round == _PyTime_ROUND_CEILING)
            d = ceil(d);
        else
            d = floor(d);
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

static void
_PyTime_overflow(void)
{
    PyErr_SetString(PyExc_OverflowError,
                    "timestamp too large to convert to C _PyTime_t");
}

_PyTime_t
_PyTime_FromSeconds(int seconds)
{
    _PyTime_t t;
    /* ensure that integer overflow cannot happen, int type should have 32
       bits, whereas _PyTime_t type has at least 64 bits (SEC_TO_MS takes 30
       bits). */
    assert((seconds >= 0 && seconds <= _PyTime_MAX / SEC_TO_NS)
           || (seconds < 0 && seconds >= _PyTime_MIN / SEC_TO_NS));
    t = (_PyTime_t)seconds * SEC_TO_NS;
    return t;
}

_PyTime_t
_PyTime_FromNanoseconds(PY_LONG_LONG ns)
{
    _PyTime_t t;
    assert(sizeof(PY_LONG_LONG) <= sizeof(_PyTime_t));
    t = Py_SAFE_DOWNCAST(ns, PY_LONG_LONG, _PyTime_t);
    return t;
}

#ifdef HAVE_CLOCK_GETTIME
static int
_PyTime_FromTimespec(_PyTime_t *tp, struct timespec *ts, int raise)
{
    _PyTime_t t;
    int res = 0;

    t = (_PyTime_t)ts->tv_sec * SEC_TO_NS;
    if (t / SEC_TO_NS != ts->tv_sec) {
        if (raise)
            _PyTime_overflow();
        res = -1;
    }

    t += ts->tv_nsec;

    *tp = t;
    return res;
}
#elif !defined(MS_WINDOWS)
static int
_PyTime_FromTimeval(_PyTime_t *tp, struct timeval *tv, int raise)
{
    _PyTime_t t;
    int res = 0;

    t = (_PyTime_t)tv->tv_sec * SEC_TO_NS;
    if (t / SEC_TO_NS != tv->tv_sec) {
        if (raise)
            _PyTime_overflow();
        res = -1;
    }

    t += (_PyTime_t)tv->tv_usec * US_TO_NS;

    *tp = t;
    return res;
}
#endif

static int
_PyTime_FromObject(_PyTime_t *t, PyObject *obj, _PyTime_round_t round,
                   long to_nanoseconds)
{
    if (PyFloat_Check(obj)) {
        /* volatile avoids unsafe optimization on float enabled by gcc -O3 */
        volatile double d, err;

        /* convert to a number of nanoseconds */
        d = PyFloat_AsDouble(obj);
        d *= to_nanoseconds;

        if (round == _PyTime_ROUND_CEILING)
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
        *t = sec * to_nanoseconds;
        if (*t / to_nanoseconds != sec) {
            _PyTime_overflow();
            return -1;
        }
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
_PyTime_Divide(_PyTime_t t, _PyTime_t k, _PyTime_round_t round)
{
    assert(k > 1);
    if (round == _PyTime_ROUND_CEILING) {
        if (t >= 0)
            return (t + k - 1) / k;
        else
            return (t - (k - 1)) / k;
    }
    else
        return t / k;
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
_PyTime_AsTimeval_impl(_PyTime_t t, struct timeval *tv, _PyTime_round_t round,
                       int raise)
{
    _PyTime_t secs, ns;
    int res = 0;

    secs = t / SEC_TO_NS;
    ns = t % SEC_TO_NS;
    if (ns < 0) {
        ns += SEC_TO_NS;
        secs -= 1;
    }

#ifdef MS_WINDOWS
    /* On Windows, timeval.tv_sec is a long (32 bit),
       whereas time_t can be 64-bit. */
    assert(sizeof(tv->tv_sec) == sizeof(long));
#if SIZEOF_TIME_T > SIZEOF_LONG
    if (secs > LONG_MAX) {
        secs = LONG_MAX;
        res = -1;
    }
    else if (secs < LONG_MIN) {
        secs = LONG_MIN;
        res = -1;
    }
#endif
    tv->tv_sec = (long)secs;
#else
    /* On OpenBSD 5.4, timeval.tv_sec is a long.
       Example: long is 64-bit, whereas time_t is 32-bit. */
    tv->tv_sec = secs;
    if ((_PyTime_t)tv->tv_sec != secs)
        res = -1;
#endif

    if (round == _PyTime_ROUND_CEILING)
        tv->tv_usec = (int)((ns + US_TO_NS - 1) / US_TO_NS);
    else
        tv->tv_usec = (int)(ns / US_TO_NS);

    if (tv->tv_usec >= SEC_TO_US) {
        tv->tv_usec -= SEC_TO_US;
        tv->tv_sec += 1;
    }

    if (res && raise)
        _PyTime_overflow();

    assert(0 <= tv->tv_usec && tv->tv_usec <= 999999);
    return res;
}

int
_PyTime_AsTimeval(_PyTime_t t, struct timeval *tv, _PyTime_round_t round)
{
    return _PyTime_AsTimeval_impl(t, tv, round, 1);
}

int
_PyTime_AsTimeval_noraise(_PyTime_t t, struct timeval *tv, _PyTime_round_t round)
{
    return _PyTime_AsTimeval_impl(t, tv, round, 0);
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
    if ((_PyTime_t)ts->tv_sec != secs) {
        _PyTime_overflow();
        return -1;
    }
    ts->tv_nsec = nsec;

    assert(0 <= ts->tv_nsec && ts->tv_nsec <= 999999999);
    return 0;
}
#endif

static int
pygettimeofday_new(_PyTime_t *tp, _Py_clock_info_t *info, int raise)
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
#ifdef HAVE_CLOCK_GETTIME
    struct timespec ts;
#else
    struct timeval tv;
#endif

    assert(info == NULL || raise);

#ifdef HAVE_CLOCK_GETTIME
    err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    if (_PyTime_FromTimespec(tp, &ts, raise) < 0)
        return -1;

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
    err = gettimeofday(&tv);
#else
    err = gettimeofday(&tv, (struct timezone *)NULL);
#endif
    if (err) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    if (_PyTime_FromTimeval(tp, &tv, raise) < 0)
        return -1;

    if (info) {
        info->implementation = "gettimeofday()";
        info->resolution = 1e-6;
        info->monotonic = 0;
        info->adjustable = 1;
    }
#endif   /* !HAVE_CLOCK_GETTIME */
#endif   /* !MS_WINDOWS */
    return 0;
}

_PyTime_t
_PyTime_GetSystemClock(void)
{
    _PyTime_t t;
    if (pygettimeofday_new(&t, NULL, 0) < 0) {
        /* should not happen, _PyTime_Init() checked the clock at startup */
        assert(0);

        /* use a fixed value instead of a random value from the stack */
        t = 0;
    }
    return t;
}

int
_PyTime_GetSystemClockWithInfo(_PyTime_t *t, _Py_clock_info_t *info)
{
    return pygettimeofday_new(t, info, 1);
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
    if (_PyTime_FromTimespec(tp, &ts, raise) < 0)
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
        /* should not happen, _PyTime_Init() checked that monotonic clock at
           startup */
        assert(0);

        /* use a fixed value instead of a random value from the stack */
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
    _PyTime_t t;

    /* ensure that the system clock works */
    if (_PyTime_GetSystemClockWithInfo(&t, NULL) < 0)
        return -1;

    /* ensure that the operating system provides a monotonic clock */
    if (_PyTime_GetMonotonicClockWithInfo(&t, NULL) < 0)
        return -1;

    /* check that _PyTime_FromSeconds() cannot overflow */
    assert(INT_MAX <= _PyTime_MAX / SEC_TO_NS);
    assert(INT_MIN >= _PyTime_MIN / SEC_TO_NS);
    return 0;
}
