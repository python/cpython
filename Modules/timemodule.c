/* Time module */

#include "Python.h"
#include "_time.h"

#define TZNAME_ENCODING "utf-8"

#include <ctype.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef QUICKWIN
#include <io.h>
#endif

#if defined(__WATCOMC__) && !defined(__QNX__)
#include <i86.h>
#else
#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "pythread.h"

/* helper to allow us to interrupt sleep() on Windows*/
static HANDLE hInterruptEvent = NULL;
static BOOL WINAPI PyCtrlHandler(DWORD dwCtrlType)
{
    SetEvent(hInterruptEvent);
    /* allow other default handlers to be called.
       Default Python handler will setup the
       KeyboardInterrupt exception.
    */
    return FALSE;
}
static long main_thread;

#if defined(__BORLANDC__)
/* These overrides not needed for Win32 */
#define timezone _timezone
#define tzname _tzname
#define daylight _daylight
#endif /* __BORLANDC__ */
#endif /* MS_WINDOWS */
#endif /* !__WATCOMC__ || __QNX__ */

#if defined(MS_WINDOWS) && !defined(__BORLANDC__)
/* Win32 has better clock replacement; we have our own version below. */
#undef HAVE_CLOCK
#undef TZNAME_ENCODING
#define TZNAME_ENCODING "mbcs"
#endif /* MS_WINDOWS && !defined(__BORLANDC__) */

#if defined(PYOS_OS2)
#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>
#endif

#if defined(PYCC_VACPP)
#include <sys/time.h>
#endif

/* Forward declarations */
static int floatsleep(double);
static double floattime(void);

/* For Y2K check */
static PyObject *moddict;

static PyObject *
time_time(PyObject *self, PyObject *unused)
{
    double secs;
    secs = floattime();
    if (secs == 0.0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return PyFloat_FromDouble(secs);
}

PyDoc_STRVAR(time_doc,
"time() -> floating point number\n\
\n\
Return the current time in seconds since the Epoch.\n\
Fractions of a second may be present if the system clock provides them.");

#ifdef HAVE_CLOCK

#ifndef CLOCKS_PER_SEC
#ifdef CLK_TCK
#define CLOCKS_PER_SEC CLK_TCK
#else
#define CLOCKS_PER_SEC 1000000
#endif
#endif

static PyObject *
time_clock(PyObject *self, PyObject *unused)
{
    return PyFloat_FromDouble(((double)clock()) / CLOCKS_PER_SEC);
}
#endif /* HAVE_CLOCK */

#if defined(MS_WINDOWS) && !defined(__BORLANDC__)
/* Due to Mark Hammond and Tim Peters */
static PyObject *
time_clock(PyObject *self, PyObject *unused)
{
    static LARGE_INTEGER ctrStart;
    static double divisor = 0.0;
    LARGE_INTEGER now;
    double diff;

    if (divisor == 0.0) {
        LARGE_INTEGER freq;
        QueryPerformanceCounter(&ctrStart);
        if (!QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) {
            /* Unlikely to happen - this works on all intel
               machines at least!  Revert to clock() */
            return PyFloat_FromDouble(((double)clock()) /
                                      CLOCKS_PER_SEC);
        }
        divisor = (double)freq.QuadPart;
    }
    QueryPerformanceCounter(&now);
    diff = (double)(now.QuadPart - ctrStart.QuadPart);
    return PyFloat_FromDouble(diff / divisor);
}

#define HAVE_CLOCK /* So it gets included in the methods */
#endif /* MS_WINDOWS && !defined(__BORLANDC__) */

#ifdef HAVE_CLOCK
PyDoc_STRVAR(clock_doc,
"clock() -> floating point number\n\
\n\
Return the CPU time or real time since the start of the process or since\n\
the first call to clock().  This has as much precision as the system\n\
records.");
#endif

static PyObject *
time_sleep(PyObject *self, PyObject *args)
{
    double secs;
    if (!PyArg_ParseTuple(args, "d:sleep", &secs))
        return NULL;
    if (floatsleep(secs) != 0)
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(sleep_doc,
"sleep(seconds)\n\
\n\
Delay execution for a given number of seconds.  The argument may be\n\
a floating point number for subsecond precision.");

static PyStructSequence_Field struct_time_type_fields[] = {
    {"tm_year", "year, for example, 1993"},
    {"tm_mon", "month of year, range [1, 12]"},
    {"tm_mday", "day of month, range [1, 31]"},
    {"tm_hour", "hours, range [0, 23]"},
    {"tm_min", "minutes, range [0, 59]"},
    {"tm_sec", "seconds, range [0, 61])"},
    {"tm_wday", "day of week, range [0, 6], Monday is 0"},
    {"tm_yday", "day of year, range [1, 366]"},
    {"tm_isdst", "1 if summer time is in effect, 0 if not, and -1 if unknown"},
    {0}
};

static PyStructSequence_Desc struct_time_type_desc = {
    "time.struct_time",
    "The time value as returned by gmtime(), localtime(), and strptime(), and\n"
    " accepted by asctime(), mktime() and strftime().  May be considered as a\n"
    " sequence of 9 integers.\n\n"
    " Note that several fields' values are not the same as those defined by\n"
    " the C language standard for struct tm.  For example, the value of the\n"
    " field tm_year is the actual year, not year - 1900.  See individual\n"
    " fields' descriptions for details.",
    struct_time_type_fields,
    9,
};

static int initialized;
static PyTypeObject StructTimeType;

static PyObject *
tmtotuple(struct tm *p)
{
    PyObject *v = PyStructSequence_New(&StructTimeType);
    if (v == NULL)
        return NULL;

#define SET(i,val) PyStructSequence_SET_ITEM(v, i, PyLong_FromLong((long) val))

    SET(0, p->tm_year + 1900);
    SET(1, p->tm_mon + 1);         /* Want January == 1 */
    SET(2, p->tm_mday);
    SET(3, p->tm_hour);
    SET(4, p->tm_min);
    SET(5, p->tm_sec);
    SET(6, (p->tm_wday + 6) % 7); /* Want Monday == 0 */
    SET(7, p->tm_yday + 1);        /* Want January, 1 == 1 */
    SET(8, p->tm_isdst);
#undef SET
    if (PyErr_Occurred()) {
        Py_XDECREF(v);
        return NULL;
    }

    return v;
}

static PyObject *
time_convert(double when, struct tm * (*function)(const time_t *))
{
    struct tm *p;
    time_t whent = _PyTime_DoubleToTimet(when);

    if (whent == (time_t)-1 && PyErr_Occurred())
        return NULL;
    errno = 0;
    p = function(&whent);
    if (p == NULL) {
#ifdef EINVAL
        if (errno == 0)
            errno = EINVAL;
#endif
        return PyErr_SetFromErrno(PyExc_ValueError);
    }
    return tmtotuple(p);
}

/* Parse arg tuple that can contain an optional float-or-None value;
   format needs to be "|O:name".
   Returns non-zero on success (parallels PyArg_ParseTuple).
*/
static int
parse_time_double_args(PyObject *args, char *format, double *pwhen)
{
    PyObject *ot = NULL;

    if (!PyArg_ParseTuple(args, format, &ot))
        return 0;
    if (ot == NULL || ot == Py_None)
        *pwhen = floattime();
    else {
        double when = PyFloat_AsDouble(ot);
        if (PyErr_Occurred())
            return 0;
        *pwhen = when;
    }
    return 1;
}

static PyObject *
time_gmtime(PyObject *self, PyObject *args)
{
    double when;
    if (!parse_time_double_args(args, "|O:gmtime", &when))
        return NULL;
    return time_convert(when, gmtime);
}

PyDoc_STRVAR(gmtime_doc,
"gmtime([seconds]) -> (tm_year, tm_mon, tm_mday, tm_hour, tm_min,\n\
                       tm_sec, tm_wday, tm_yday, tm_isdst)\n\
\n\
Convert seconds since the Epoch to a time tuple expressing UTC (a.k.a.\n\
GMT).  When 'seconds' is not passed in, convert the current time instead.");

static PyObject *
time_localtime(PyObject *self, PyObject *args)
{
    double when;
    if (!parse_time_double_args(args, "|O:localtime", &when))
        return NULL;
    return time_convert(when, localtime);
}

PyDoc_STRVAR(localtime_doc,
"localtime([seconds]) -> (tm_year,tm_mon,tm_mday,tm_hour,tm_min,\n\
                          tm_sec,tm_wday,tm_yday,tm_isdst)\n\
\n\
Convert seconds since the Epoch to a time tuple expressing local time.\n\
When 'seconds' is not passed in, convert the current time instead.");

/* Convert 9-item tuple to tm structure.  Return 1 on success, set
 * an exception and return 0 on error.
 */
static int
gettmarg(PyObject *args, struct tm *p)
{
    int y;

    memset((void *) p, '\0', sizeof(struct tm));

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError,
                        "Tuple or struct_time argument required");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "iiiiiiiii",
                          &y, &p->tm_mon, &p->tm_mday,
                          &p->tm_hour, &p->tm_min, &p->tm_sec,
                          &p->tm_wday, &p->tm_yday, &p->tm_isdst))
        return 0;

    /* If year is specified with less than 4 digits, its interpretation
     * depends on the accept2dyear value.
     *
     * If accept2dyear is true (default), a backward compatibility behavior is
     * invoked as follows:
     *
     *   - for 2-digit year, century is guessed according to POSIX rules for
     *      %y strptime format: 21st century for y < 69, 20th century
     *      otherwise.  A deprecation warning is issued when century
     *      information is guessed in this way.
     *
     *   - for 3-digit or negative year, a ValueError exception is raised.
     *
     * If accept2dyear is false (set by the program or as a result of a
     * non-empty value assigned to PYTHONY2K environment variable) all year
     * values are interpreted as given.
     */
    if (y < 1000) {
        PyObject *accept = PyDict_GetItemString(moddict,
                                                "accept2dyear");
	if (accept != NULL) {
            int acceptval =  PyObject_IsTrue(accept);
            if (acceptval == -1)
                return 0;
            if (acceptval) {
                if (0 <= y && y < 69)
                    y += 2000;
                else if (69 <= y && y < 100)
                    y += 1900;
                else {
                    PyErr_SetString(PyExc_ValueError,
                                    "year out of range");
                    return 0;
                }
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                           "Century info guessed for a 2-digit year.", 1) != 0)
                    return 0;
            }
        }
        else
            return 0;
    }
    p->tm_year = y - 1900;
    p->tm_mon--;
    p->tm_wday = (p->tm_wday + 1) % 7;
    p->tm_yday--;
    return 1;
}

/* Check values of the struct tm fields before it is passed to strftime() and
 * asctime().  Return 1 if all values are valid, otherwise set an exception
 * and returns 0.
 */
static int
checktm(struct tm* buf)
{
    /* Checks added to make sure strftime() and asctime() does not crash Python by
       indexing blindly into some array for a textual representation
       by some bad index (fixes bug #897625 and #6608).

       Also support values of zero from Python code for arguments in which
       that is out of range by forcing that value to the lowest value that
       is valid (fixed bug #1520914).

       Valid ranges based on what is allowed in struct tm:

       - tm_year: [0, max(int)] (1)
       - tm_mon: [0, 11] (2)
       - tm_mday: [1, 31]
       - tm_hour: [0, 23]
       - tm_min: [0, 59]
       - tm_sec: [0, 60]
       - tm_wday: [0, 6] (1)
       - tm_yday: [0, 365] (2)
       - tm_isdst: [-max(int), max(int)]

       (1) gettmarg() handles bounds-checking.
       (2) Python's acceptable range is one greater than the range in C,
       thus need to check against automatic decrement by gettmarg().
    */
    if (buf->tm_mon == -1)
        buf->tm_mon = 0;
    else if (buf->tm_mon < 0 || buf->tm_mon > 11) {
        PyErr_SetString(PyExc_ValueError, "month out of range");
        return 0;
    }
    if (buf->tm_mday == 0)
        buf->tm_mday = 1;
    else if (buf->tm_mday < 0 || buf->tm_mday > 31) {
        PyErr_SetString(PyExc_ValueError, "day of month out of range");
        return 0;
    }
    if (buf->tm_hour < 0 || buf->tm_hour > 23) {
        PyErr_SetString(PyExc_ValueError, "hour out of range");
        return 0;
    }
    if (buf->tm_min < 0 || buf->tm_min > 59) {
        PyErr_SetString(PyExc_ValueError, "minute out of range");
        return 0;
    }
    if (buf->tm_sec < 0 || buf->tm_sec > 61) {
        PyErr_SetString(PyExc_ValueError, "seconds out of range");
        return 0;
    }
    /* tm_wday does not need checking of its upper-bound since taking
    ``% 7`` in gettmarg() automatically restricts the range. */
    if (buf->tm_wday < 0) {
        PyErr_SetString(PyExc_ValueError, "day of week out of range");
        return 0;
    }
    if (buf->tm_yday == -1)
        buf->tm_yday = 0;
    else if (buf->tm_yday < 0 || buf->tm_yday > 365) {
        PyErr_SetString(PyExc_ValueError, "day of year out of range");
        return 0;
    }
    return 1;
}

#ifdef HAVE_STRFTIME
#ifdef HAVE_WCSFTIME
#define time_char wchar_t
#define format_time wcsftime
#define time_strlen wcslen
#else
#define time_char char
#define format_time strftime
#define time_strlen strlen
#endif

static PyObject *
time_strftime(PyObject *self, PyObject *args)
{
    PyObject *tup = NULL;
    struct tm buf;
    const time_char *fmt;
#ifdef HAVE_WCSFTIME
    wchar_t *format;
#else
    PyObject *format;
#endif
    PyObject *format_arg;
    size_t fmtlen, buflen;
    time_char *outbuf = NULL;
    size_t i;
    PyObject *ret = NULL;

    memset((void *) &buf, '\0', sizeof(buf));

    /* Will always expect a unicode string to be passed as format.
       Given that there's no str type anymore in py3k this seems safe.
    */
    if (!PyArg_ParseTuple(args, "U|O:strftime", &format_arg, &tup))
        return NULL;

    if (tup == NULL) {
        time_t tt = time(NULL);
        buf = *localtime(&tt);
    }
    else if (!gettmarg(tup, &buf) || !checktm(&buf))
        return NULL;

#ifdef _MSC_VER
    if (buf.tm_year + 1900 < 1 || 9999 < buf.tm_year + 1900) {
        PyErr_Format(PyExc_ValueError,
                     "strftime() requires year in [1; 9999]",
                     buf.tm_year + 1900);
        return NULL;
    }
#else
    if (buf.tm_year + 1900 < 1) {
        PyErr_Format(PyExc_ValueError,
                     "strftime() requires year >= 1",
                     buf.tm_year + 1900);
        return NULL;
    }
#endif

    /* Normalize tm_isdst just in case someone foolishly implements %Z
       based on the assumption that tm_isdst falls within the range of
       [-1, 1] */
    if (buf.tm_isdst < -1)
        buf.tm_isdst = -1;
    else if (buf.tm_isdst > 1)
        buf.tm_isdst = 1;

#ifdef HAVE_WCSFTIME
    format = PyUnicode_AsWideCharString(format_arg, NULL);
    if (format == NULL)
        return NULL;
    fmt = format;
#else
    /* Convert the unicode string to an ascii one */
    format = PyUnicode_AsEncodedString(format_arg, TZNAME_ENCODING, NULL);
    if (format == NULL)
        return NULL;
    fmt = PyBytes_AS_STRING(format);
#endif

#if defined(MS_WINDOWS) && defined(HAVE_WCSFTIME)
    /* check that the format string contains only valid directives */
    for(outbuf = wcschr(fmt, L'%');
        outbuf != NULL;
        outbuf = wcschr(outbuf+2, L'%'))
    {
        if (outbuf[1]=='#')
            ++outbuf; /* not documented by python, */
        if (outbuf[1]=='\0' ||
            !wcschr(L"aAbBcdfHIjmMpSUwWxXyYzZ%", outbuf[1]))
        {
            PyErr_SetString(PyExc_ValueError, "Invalid format string");
            return 0;
        }
    }
#endif

    fmtlen = time_strlen(fmt);

    /* I hate these functions that presume you know how big the output
     * will be ahead of time...
     */
    for (i = 1024; ; i += i) {
        outbuf = (time_char *)PyMem_Malloc(i*sizeof(time_char));
        if (outbuf == NULL) {
            PyErr_NoMemory();
            break;
        }
        buflen = format_time(outbuf, i, fmt, &buf);
        if (buflen > 0 || i >= 256 * fmtlen) {
            /* If the buffer is 256 times as long as the format,
               it's probably not failing for lack of room!
               More likely, the format yields an empty result,
               e.g. an empty format, or %Z when the timezone
               is unknown. */
#ifdef HAVE_WCSFTIME
            ret = PyUnicode_FromWideChar(outbuf, buflen);
#else
            ret = PyUnicode_Decode(outbuf, buflen,
                                   TZNAME_ENCODING, NULL);
#endif
            PyMem_Free(outbuf);
            break;
        }
        PyMem_Free(outbuf);
#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
        /* VisualStudio .NET 2005 does this properly */
        if (buflen == 0 && errno == EINVAL) {
            PyErr_SetString(PyExc_ValueError, "Invalid format string");
            break;
        }
#endif
    }
#ifdef HAVE_WCSFTIME
    PyMem_Free(format);
#else
    Py_DECREF(format);
#endif
    return ret;
}

#undef time_char
#undef format_time

PyDoc_STRVAR(strftime_doc,
"strftime(format[, tuple]) -> string\n\
\n\
Convert a time tuple to a string according to a format specification.\n\
See the library reference manual for formatting codes. When the time tuple\n\
is not present, current time as returned by localtime() is used.");
#endif /* HAVE_STRFTIME */

static PyObject *
time_strptime(PyObject *self, PyObject *args)
{
    PyObject *strptime_module = PyImport_ImportModuleNoBlock("_strptime");
    PyObject *strptime_result;

    if (!strptime_module)
        return NULL;
    strptime_result = PyObject_CallMethod(strptime_module,
                                            "_strptime_time", "O", args);
    Py_DECREF(strptime_module);
    return strptime_result;
}


PyDoc_STRVAR(strptime_doc,
"strptime(string, format) -> struct_time\n\
\n\
Parse a string to a time tuple according to a format specification.\n\
See the library reference manual for formatting codes (same as strftime()).");

static PyObject *
_asctime(struct tm *timeptr)
{
    /* Inspired by Open Group reference implementation available at
     * http://pubs.opengroup.org/onlinepubs/009695399/functions/asctime.html */
    static char wday_name[7][3] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static char mon_name[12][3] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    char buf[20]; /* 'Sun Sep 16 01:03:52\0' */
    int n;

    n = PyOS_snprintf(buf, sizeof(buf), "%.3s %.3s%3d %.2d:%.2d:%.2d",
                      wday_name[timeptr->tm_wday],
                      mon_name[timeptr->tm_mon],
                      timeptr->tm_mday, timeptr->tm_hour,
                      timeptr->tm_min, timeptr->tm_sec);
    /* XXX: since the fields used by snprintf above are validated in checktm,
     * the following condition should never trigger. We keep the check because
     * historically fixed size buffer used in asctime was the source of
     * crashes. */
    if (n + 1 != sizeof(buf)) {
        PyErr_SetString(PyExc_ValueError, "unconvertible time");
        return NULL;
    }

    return PyUnicode_FromFormat("%s %d", buf, 1900 + timeptr->tm_year);
}

static PyObject *
time_asctime(PyObject *self, PyObject *args)
{
    PyObject *tup = NULL;
    struct tm buf;

    if (!PyArg_UnpackTuple(args, "asctime", 0, 1, &tup))
        return NULL;
    if (tup == NULL) {
        time_t tt = time(NULL);
        buf = *localtime(&tt);
    } else if (!gettmarg(tup, &buf) || !checktm(&buf))
        return NULL;
    return _asctime(&buf);
}

PyDoc_STRVAR(asctime_doc,
"asctime([tuple]) -> string\n\
\n\
Convert a time tuple to a string, e.g. 'Sat Jun 06 16:26:11 1998'.\n\
When the time tuple is not present, current time as returned by localtime()\n\
is used.");

static PyObject *
time_ctime(PyObject *self, PyObject *args)
{
    PyObject *ot = NULL;
    time_t tt;
    struct tm *timeptr;

    if (!PyArg_UnpackTuple(args, "ctime", 0, 1, &ot))
        return NULL;
    if (ot == NULL || ot == Py_None)
        tt = time(NULL);
    else {
        double dt = PyFloat_AsDouble(ot);
        if (PyErr_Occurred())
            return NULL;
        tt = _PyTime_DoubleToTimet(dt);
        if (tt == (time_t)-1 && PyErr_Occurred())
            return NULL;
    }
    timeptr = localtime(&tt);
    if (timeptr == NULL) {
        PyErr_SetString(PyExc_ValueError, "unconvertible time");
        return NULL;
    }
    return _asctime(timeptr);
}

PyDoc_STRVAR(ctime_doc,
"ctime(seconds) -> string\n\
\n\
Convert a time in seconds since the Epoch to a string in local time.\n\
This is equivalent to asctime(localtime(seconds)). When the time tuple is\n\
not present, current time as returned by localtime() is used.");

#ifdef HAVE_MKTIME
static PyObject *
time_mktime(PyObject *self, PyObject *tup)
{
    struct tm buf;
    time_t tt;
    if (!gettmarg(tup, &buf))
        return NULL;
    tt = mktime(&buf);
    if (tt == (time_t)(-1)) {
        PyErr_SetString(PyExc_OverflowError,
                        "mktime argument out of range");
        return NULL;
    }
    return PyFloat_FromDouble((double)tt);
}

PyDoc_STRVAR(mktime_doc,
"mktime(tuple) -> floating point number\n\
\n\
Convert a time tuple in local time to seconds since the Epoch.");
#endif /* HAVE_MKTIME */

#ifdef HAVE_WORKING_TZSET
static void PyInit_timezone(PyObject *module);

static PyObject *
time_tzset(PyObject *self, PyObject *unused)
{
    PyObject* m;

    m = PyImport_ImportModuleNoBlock("time");
    if (m == NULL) {
        return NULL;
    }

    tzset();

    /* Reset timezone, altzone, daylight and tzname */
    PyInit_timezone(m);
    Py_DECREF(m);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(tzset_doc,
"tzset()\n\
\n\
Initialize, or reinitialize, the local timezone to the value stored in\n\
os.environ['TZ']. The TZ environment variable should be specified in\n\
standard Unix timezone format as documented in the tzset man page\n\
(eg. 'US/Eastern', 'Europe/Amsterdam'). Unknown timezones will silently\n\
fall back to UTC. If the TZ environment variable is not set, the local\n\
timezone is set to the systems best guess of wallclock time.\n\
Changing the TZ environment variable without calling tzset *may* change\n\
the local timezone used by methods such as localtime, but this behaviour\n\
should not be relied on.");
#endif /* HAVE_WORKING_TZSET */

static void
PyInit_timezone(PyObject *m) {
    /* This code moved from PyInit_time wholesale to allow calling it from
    time_tzset. In the future, some parts of it can be moved back
    (for platforms that don't HAVE_WORKING_TZSET, when we know what they
    are), and the extraneous calls to tzset(3) should be removed.
    I haven't done this yet, as I don't want to change this code as
    little as possible when introducing the time.tzset and time.tzsetwall
    methods. This should simply be a method of doing the following once,
    at the top of this function and removing the call to tzset() from
    time_tzset():

        #ifdef HAVE_TZSET
        tzset()
        #endif

    And I'm lazy and hate C so nyer.
     */
#if defined(HAVE_TZNAME) && !defined(__GLIBC__) && !defined(__CYGWIN__)
    PyObject *otz0, *otz1;
    tzset();
#ifdef PYOS_OS2
    PyModule_AddIntConstant(m, "timezone", _timezone);
#else /* !PYOS_OS2 */
    PyModule_AddIntConstant(m, "timezone", timezone);
#endif /* PYOS_OS2 */
#ifdef HAVE_ALTZONE
    PyModule_AddIntConstant(m, "altzone", altzone);
#else
#ifdef PYOS_OS2
    PyModule_AddIntConstant(m, "altzone", _timezone-3600);
#else /* !PYOS_OS2 */
    PyModule_AddIntConstant(m, "altzone", timezone-3600);
#endif /* PYOS_OS2 */
#endif
    PyModule_AddIntConstant(m, "daylight", daylight);
    otz0 = PyUnicode_Decode(tzname[0], strlen(tzname[0]), TZNAME_ENCODING, NULL);
    otz1 = PyUnicode_Decode(tzname[1], strlen(tzname[1]), TZNAME_ENCODING, NULL);
    PyModule_AddObject(m, "tzname", Py_BuildValue("(NN)", otz0, otz1));
#else /* !HAVE_TZNAME || __GLIBC__ || __CYGWIN__*/
#ifdef HAVE_STRUCT_TM_TM_ZONE
    {
#define YEAR ((time_t)((365 * 24 + 6) * 3600))
        time_t t;
        struct tm *p;
        long janzone, julyzone;
        char janname[10], julyname[10];
        t = (time((time_t *)0) / YEAR) * YEAR;
        p = localtime(&t);
        janzone = -p->tm_gmtoff;
        strncpy(janname, p->tm_zone ? p->tm_zone : "   ", 9);
        janname[9] = '\0';
        t += YEAR/2;
        p = localtime(&t);
        julyzone = -p->tm_gmtoff;
        strncpy(julyname, p->tm_zone ? p->tm_zone : "   ", 9);
        julyname[9] = '\0';

        if( janzone < julyzone ) {
            /* DST is reversed in the southern hemisphere */
            PyModule_AddIntConstant(m, "timezone", julyzone);
            PyModule_AddIntConstant(m, "altzone", janzone);
            PyModule_AddIntConstant(m, "daylight",
                                    janzone != julyzone);
            PyModule_AddObject(m, "tzname",
                               Py_BuildValue("(zz)",
                                             julyname, janname));
        } else {
            PyModule_AddIntConstant(m, "timezone", janzone);
            PyModule_AddIntConstant(m, "altzone", julyzone);
            PyModule_AddIntConstant(m, "daylight",
                                    janzone != julyzone);
            PyModule_AddObject(m, "tzname",
                               Py_BuildValue("(zz)",
                                             janname, julyname));
        }
    }
#else
#endif /* HAVE_STRUCT_TM_TM_ZONE */
#ifdef __CYGWIN__
    tzset();
    PyModule_AddIntConstant(m, "timezone", _timezone);
    PyModule_AddIntConstant(m, "altzone", _timezone-3600);
    PyModule_AddIntConstant(m, "daylight", _daylight);
    PyModule_AddObject(m, "tzname",
                       Py_BuildValue("(zz)", _tzname[0], _tzname[1]));
#endif /* __CYGWIN__ */
#endif /* !HAVE_TZNAME || __GLIBC__ || __CYGWIN__*/
}


static PyMethodDef time_methods[] = {
    {"time",            time_time, METH_NOARGS, time_doc},
#ifdef HAVE_CLOCK
    {"clock",           time_clock, METH_NOARGS, clock_doc},
#endif
    {"sleep",           time_sleep, METH_VARARGS, sleep_doc},
    {"gmtime",          time_gmtime, METH_VARARGS, gmtime_doc},
    {"localtime",       time_localtime, METH_VARARGS, localtime_doc},
    {"asctime",         time_asctime, METH_VARARGS, asctime_doc},
    {"ctime",           time_ctime, METH_VARARGS, ctime_doc},
#ifdef HAVE_MKTIME
    {"mktime",          time_mktime, METH_O, mktime_doc},
#endif
#ifdef HAVE_STRFTIME
    {"strftime",        time_strftime, METH_VARARGS, strftime_doc},
#endif
    {"strptime",        time_strptime, METH_VARARGS, strptime_doc},
#ifdef HAVE_WORKING_TZSET
    {"tzset",           time_tzset, METH_NOARGS, tzset_doc},
#endif
    {NULL,              NULL}           /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module provides various functions to manipulate time values.\n\
\n\
There are two standard representations of time.  One is the number\n\
of seconds since the Epoch, in UTC (a.k.a. GMT).  It may be an integer\n\
or a floating point number (to represent fractions of seconds).\n\
The Epoch is system-defined; on Unix, it is generally January 1st, 1970.\n\
The actual value can be retrieved by calling gmtime(0).\n\
\n\
The other representation is a tuple of 9 integers giving local time.\n\
The tuple items are:\n\
  year (four digits, e.g. 1998)\n\
  month (1-12)\n\
  day (1-31)\n\
  hours (0-23)\n\
  minutes (0-59)\n\
  seconds (0-59)\n\
  weekday (0-6, Monday is 0)\n\
  Julian day (day in the year, 1-366)\n\
  DST (Daylight Savings Time) flag (-1, 0 or 1)\n\
If the DST flag is 0, the time is given in the regular time zone;\n\
if it is 1, the time is given in the DST time zone;\n\
if it is -1, mktime() should guess based on the date and time.\n\
\n\
Variables:\n\
\n\
timezone -- difference in seconds between UTC and local standard time\n\
altzone -- difference in  seconds between UTC and local DST time\n\
daylight -- whether local time should reflect DST\n\
tzname -- tuple of (standard time zone name, DST time zone name)\n\
\n\
Functions:\n\
\n\
time() -- return current time in seconds since the Epoch as a float\n\
clock() -- return CPU time since process start as a float\n\
sleep() -- delay for a number of seconds given as a float\n\
gmtime() -- convert seconds since Epoch to UTC tuple\n\
localtime() -- convert seconds since Epoch to local time tuple\n\
asctime() -- convert time tuple to string\n\
ctime() -- convert time in seconds to string\n\
mktime() -- convert local time tuple to seconds since Epoch\n\
strftime() -- convert time tuple to string according to format specification\n\
strptime() -- parse string to time tuple according to format specification\n\
tzset() -- change the local timezone");



static struct PyModuleDef timemodule = {
    PyModuleDef_HEAD_INIT,
    "time",
    module_doc,
    -1,
    time_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_time(void)
{
    PyObject *m;
    char *p;
    m = PyModule_Create(&timemodule);
    if (m == NULL)
        return NULL;

    /* Accept 2-digit dates unless PYTHONY2K is set and non-empty */
    p = Py_GETENV("PYTHONY2K");
    PyModule_AddIntConstant(m, "accept2dyear", (long) (!p || !*p));
    /* Squirrel away the module's dictionary for the y2k check */
    moddict = PyModule_GetDict(m);
    Py_INCREF(moddict);

    /* Set, or reset, module variables like time.timezone */
    PyInit_timezone(m);

#ifdef MS_WINDOWS
    /* Helper to allow interrupts for Windows.
       If Ctrl+C event delivered while not sleeping
       it will be ignored.
    */
    main_thread = PyThread_get_thread_ident();
    hInterruptEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler( PyCtrlHandler, TRUE);
#endif /* MS_WINDOWS */
    if (!initialized) {
        PyStructSequence_InitType(&StructTimeType,
                                  &struct_time_type_desc);
    }
    Py_INCREF(&StructTimeType);
    PyModule_AddObject(m, "struct_time", (PyObject*) &StructTimeType);
    initialized = 1;
    return m;
}

static double
floattime(void)
{
    _PyTime_timeval t;
    _PyTime_gettimeofday(&t);
    return (double)t.tv_sec + t.tv_usec*0.000001;
}


/* Implement floatsleep() for various platforms.
   When interrupted (or when another error occurs), return -1 and
   set an exception; else return 0. */

static int
floatsleep(double secs)
{
/* XXX Should test for MS_WINDOWS first! */
#if defined(HAVE_SELECT) && !defined(__EMX__)
    struct timeval t;
    double frac;
    frac = fmod(secs, 1.0);
    secs = floor(secs);
    t.tv_sec = (long)secs;
    t.tv_usec = (long)(frac*1000000.0);
    Py_BEGIN_ALLOW_THREADS
    if (select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t) != 0) {
#ifdef EINTR
        if (errno != EINTR) {
#else
        if (1) {
#endif
            Py_BLOCK_THREADS
            PyErr_SetFromErrno(PyExc_IOError);
            return -1;
        }
    }
    Py_END_ALLOW_THREADS
#elif defined(__WATCOMC__) && !defined(__QNX__)
    /* XXX Can't interrupt this sleep */
    Py_BEGIN_ALLOW_THREADS
    delay((int)(secs * 1000 + 0.5));  /* delay() uses milliseconds */
    Py_END_ALLOW_THREADS
#elif defined(MS_WINDOWS)
    {
        double millisecs = secs * 1000.0;
        unsigned long ul_millis;

        if (millisecs > (double)ULONG_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "sleep length is too large");
            return -1;
        }
        Py_BEGIN_ALLOW_THREADS
        /* Allow sleep(0) to maintain win32 semantics, and as decreed
         * by Guido, only the main thread can be interrupted.
         */
        ul_millis = (unsigned long)millisecs;
        if (ul_millis == 0 ||
            main_thread != PyThread_get_thread_ident())
            Sleep(ul_millis);
        else {
            DWORD rc;
            ResetEvent(hInterruptEvent);
            rc = WaitForSingleObject(hInterruptEvent, ul_millis);
            if (rc == WAIT_OBJECT_0) {
                /* Yield to make sure real Python signal
                 * handler called.
                 */
                Sleep(1);
                Py_BLOCK_THREADS
                errno = EINTR;
                PyErr_SetFromErrno(PyExc_IOError);
                return -1;
            }
        }
        Py_END_ALLOW_THREADS
    }
#elif defined(PYOS_OS2)
    /* This Sleep *IS* Interruptable by Exceptions */
    Py_BEGIN_ALLOW_THREADS
    if (DosSleep(secs * 1000) != NO_ERROR) {
        Py_BLOCK_THREADS
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    Py_END_ALLOW_THREADS
#else
    /* XXX Can't interrupt this sleep */
    Py_BEGIN_ALLOW_THREADS
    sleep((int)secs);
    Py_END_ALLOW_THREADS
#endif

    return 0;
}
