#include "Python.h"
#ifdef MS_WINDOWS
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <mach/mach_time.h>   /* mach_absolute_time(), mach_timebase_info() */
#endif

#ifdef MS_WINDOWS
static OSVERSIONINFOEX winver;
#endif

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
    tp->tv_sec = microseconds / 1000000;
    tp->tv_usec = microseconds % 1000000;
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
    tp->tv_usec = ts.tv_nsec / 1000;

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
    assert(0 <= tp->tv_usec && tp->tv_usec < 1000 * 1000);
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
    static _PyTime_timeval last = {-1, -1};
#endif
#if defined(MS_WINDOWS)
    static ULONGLONG (*GetTickCount64) (void) = NULL;
    static ULONGLONG (CALLBACK *Py_GetTickCount64)(void);
    static int has_gettickcount64 = -1;
    ULONGLONG result;

    assert(info == NULL || raise);

    if (has_gettickcount64 == -1) {
        /* GetTickCount64() was added to Windows Vista */
        has_gettickcount64 = (winver.dwMajorVersion >= 6);
        if (has_gettickcount64) {
            HINSTANCE hKernel32;
            hKernel32 = GetModuleHandleW(L"KERNEL32");
            *(FARPROC*)&Py_GetTickCount64 = GetProcAddress(hKernel32,
                                                           "GetTickCount64");
            assert(Py_GetTickCount64 != NULL);
        }
    }

    if (has_gettickcount64) {
        result = Py_GetTickCount64();
    }
    else {
        static DWORD last_ticks = 0;
        static DWORD n_overflow = 0;
        DWORD ticks;

        ticks = GetTickCount();
        if (ticks < last_ticks)
            n_overflow++;
        last_ticks = ticks;

        result = (ULONGLONG)n_overflow << 32;
        result += ticks;
    }

    tp->tv_sec = result / 1000;
    tp->tv_usec = (result % 1000) * 1000;

    if (info) {
        DWORD timeAdjustment, timeIncrement;
        BOOL isTimeAdjustmentDisabled, ok;
        if (has_gettickcount64)
            info->implementation = "GetTickCount64()";
        else
            info->implementation = "GetTickCount()";
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
    time /= 1000;
    /* apply timebase factor */
    time *= timebase.numer;
    time /= timebase.denom;
    tp->tv_sec = time / (1000 * 1000);
    tp->tv_usec = time % (1000 * 1000);

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
    tp->tv_usec = ts.tv_nsec / 1000;
#endif
    assert(0 <= tp->tv_usec && tp->tv_usec < 1000 * 1000);
#ifdef Py_DEBUG
    /* monotonic clock cannot go backward */
    assert(tp->tv_sec > last.tv_sec
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

int
_PyTime_Init(void)
{
    _PyTime_timeval tv;

#ifdef MS_WINDOWS
    winver.dwOSVersionInfoSize = sizeof(winver);
    if (!GetVersionEx((OSVERSIONINFO*)&winver)) {
        PyErr_SetFromWindowsErr(0);
        return -1;
    }
#endif

    /* ensure that the system clock works */
    if (_PyTime_gettimeofday_info(&tv, NULL) < 0)
        return -1;

    /* ensure that the operating system provides a monotonic clock */
    if (_PyTime_monotonic_info(&tv, NULL) < 0)
        return -1;
    return 0;
}
