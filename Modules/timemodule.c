
/* Time module */

#include "Python.h"

#include <ctype.h>

#ifdef macintosh
#include <time.h>
#include <OSUtils.h>
#ifdef USE_GUSI2
/* GUSI, the I/O library which has the time() function and such uses the
** Mac epoch of 1904. MSL, the C library which has localtime() and so uses
** the ANSI epoch of 1900.
*/
#define GUSI_TO_MSL_EPOCH (4*365*24*60*60)
#endif /* USE_GUSI2 */
#else
#include <sys/types.h>
#endif

#ifdef QUICKWIN
#include <io.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FTIME
#include <sys/timeb.h>
#if !defined(MS_WINDOWS) && !defined(PYOS_OS2)
extern int ftime(struct timeb *);
#endif /* MS_WINDOWS */
#endif /* HAVE_FTIME */

#if defined(__WATCOMC__) && !defined(__QNX__)
#include <i86.h>
#else
#ifdef MS_WINDOWS
#include <windows.h>
#ifdef MS_WIN16
/* These overrides not needed for Win32 */
#define timezone _timezone
#define tzname _tzname
#define daylight _daylight
#define altzone _altzone
#endif /* MS_WIN16 */
#endif /* MS_WINDOWS */
#endif /* !__WATCOMC__ || __QNX__ */

#if defined(MS_WIN32) && !defined(MS_WIN64)
/* Win32 has better clock replacement
   XXX Win64 does not yet, but might when the platform matures. */
#include <largeint.h>
#undef HAVE_CLOCK /* We have our own version down below */
#endif /* MS_WIN32 && !MS_WIN64 */

#if defined(PYCC_VACPP)
#include <sys/time.h>
#endif

#ifdef __BEOS__
#include <time.h>
/* For bigtime_t, snooze(). - [cjh] */
#include <support/SupportDefs.h>
#include <kernel/OS.h>
#endif

/* Forward declarations */
static int floatsleep(double);
static double floattime(void);

/* For Y2K check */
static PyObject *moddict;

#ifdef macintosh
/* Our own timezone. We have enough information to deduce whether
** DST is on currently, but unfortunately we cannot put it to good
** use because we don't know the rules (and that is needed to have
** localtime() return correct tm_isdst values for times other than
** the current time. So, we cop out and only tell the user the current
** timezone.
*/
static long timezone;

static void 
initmactimezone(void)
{
	MachineLocation	loc;
	long		delta;

	ReadLocation(&loc);
	
	if (loc.latitude == 0 && loc.longitude == 0 && loc.u.gmtDelta == 0)
		return;
	
	delta = loc.u.gmtDelta & 0x00FFFFFF;
	
	if (delta & 0x00800000)
		delta |= 0xFF000000;
	
	timezone = -delta;
}
#endif /* macintosh */


static PyObject *
time_time(PyObject *self, PyObject *args)
{
	double secs;
	if (!PyArg_NoArgs(args))
		return NULL;
	secs = floattime();
	if (secs == 0.0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	return PyFloat_FromDouble(secs);
}

static char time_doc[] =
"time() -> floating point number\n\
\n\
Return the current time in seconds since the Epoch.\n\
Fractions of a second may be present if the system clock provides them.";

#ifdef HAVE_CLOCK

#ifndef CLOCKS_PER_SEC
#ifdef CLK_TCK
#define CLOCKS_PER_SEC CLK_TCK
#else
#define CLOCKS_PER_SEC 1000000
#endif
#endif

static PyObject *
time_clock(PyObject *self, PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyFloat_FromDouble(((double)clock()) / CLOCKS_PER_SEC);
}
#endif /* HAVE_CLOCK */

#if defined(MS_WIN32) && !defined(MS_WIN64)
/* Due to Mark Hammond */
static PyObject *
time_clock(PyObject *self, PyObject *args)
{
	static LARGE_INTEGER ctrStart;
	static LARGE_INTEGER divisor = {0,0};
	LARGE_INTEGER now, diff, rem;

	if (!PyArg_NoArgs(args))
		return NULL;

	if (LargeIntegerEqualToZero(divisor)) {
		QueryPerformanceCounter(&ctrStart);
		if (!QueryPerformanceFrequency(&divisor) || 
		    LargeIntegerEqualToZero(divisor)) {
				/* Unlikely to happen - 
				   this works on all intel machines at least! 
				   Revert to clock() */
			return PyFloat_FromDouble(clock());
		}
	}
	QueryPerformanceCounter(&now);
	diff = LargeIntegerSubtract(now, ctrStart);
	diff = LargeIntegerDivide(diff, divisor, &rem);
	/* XXX - we assume both divide results fit in 32 bits.  This is
	   true on Intels.  First person who can afford a machine that 
	   doesnt deserves to fix it :-)
	*/
	return PyFloat_FromDouble((double)diff.LowPart + 
		              ((double)rem.LowPart / (double)divisor.LowPart));
}

#define HAVE_CLOCK /* So it gets included in the methods */
#endif /* MS_WIN32 && !MS_WIN64 */

#ifdef HAVE_CLOCK
static char clock_doc[] =
"clock() -> floating point number\n\
\n\
Return the CPU time or real time since the start of the process or since\n\
the first call to clock().  This has as much precision as the system records.";
#endif

static PyObject *
time_sleep(PyObject *self, PyObject *args)
{
	double secs;
	if (!PyArg_Parse(args, "d", &secs))
		return NULL;
	if (floatsleep(secs) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char sleep_doc[] =
"sleep(seconds)\n\
\n\
Delay execution for a given number of seconds.  The argument may be\n\
a floating point number for subsecond precision.";

static PyObject *
tmtotuple(struct tm *p)
{
	return Py_BuildValue("(iiiiiiiii)",
			     p->tm_year + 1900,
			     p->tm_mon + 1,	   /* Want January == 1 */
			     p->tm_mday,
			     p->tm_hour,
			     p->tm_min,
			     p->tm_sec,
			     (p->tm_wday + 6) % 7, /* Want Monday == 0 */
			     p->tm_yday + 1,	   /* Want January, 1 == 1 */
			     p->tm_isdst);
}

static PyObject *
time_convert(time_t when, struct tm * (*function)(const time_t *))
{
	struct tm *p;
	errno = 0;
#if defined(macintosh) && defined(USE_GUSI204)
	when = when + GUSI_TO_MSL_EPOCH;
#endif
	p = function(&when);
	if (p == NULL) {
#ifdef EINVAL
		if (errno == 0)
			errno = EINVAL;
#endif
		return PyErr_SetFromErrno(PyExc_IOError);
	}
	return tmtotuple(p);
}

static PyObject *
time_gmtime(PyObject *self, PyObject *args)
{
	double when;
	if (!PyArg_Parse(args, "d", &when))
		return NULL;
	return time_convert((time_t)when, gmtime);
}

static char gmtime_doc[] =
"gmtime(seconds) -> tuple\n\
\n\
Convert seconds since the Epoch to a time tuple expressing UTC (a.k.a. GMT).";

static PyObject *
time_localtime(PyObject *self, PyObject *args)
{
	double when;
	if (!PyArg_Parse(args, "d", &when))
		return NULL;
	return time_convert((time_t)when, localtime);
}

static char localtime_doc[] =
"localtime(seconds) -> tuple\n\
Convert seconds since the Epoch to a time tuple expressing local time.";

static int
gettmarg(PyObject *args, struct tm *p)
{
	int y;
	memset((void *) p, '\0', sizeof(struct tm));

	if (!PyArg_Parse(args, "(iiiiiiiii)",
			 &y,
			 &p->tm_mon,
			 &p->tm_mday,
			 &p->tm_hour,
			 &p->tm_min,
			 &p->tm_sec,
			 &p->tm_wday,
			 &p->tm_yday,
			 &p->tm_isdst))
		return 0;
	if (y < 1900) {
		PyObject *accept = PyDict_GetItemString(moddict,
							"accept2dyear");
		if (accept == NULL || !PyInt_Check(accept) ||
		    PyInt_AsLong(accept) == 0) {
			PyErr_SetString(PyExc_ValueError,
					"year >= 1900 required");
			return 0;
		}
		if (69 <= y && y <= 99)
			y += 1900;
		else if (0 <= y && y <= 68)
			y += 2000;
		else {
			PyErr_SetString(PyExc_ValueError,
					"year out of range (00-99, 1900-*)");
			return 0;
		}
	}
	p->tm_year = y - 1900;
	p->tm_mon--;
	p->tm_wday = (p->tm_wday + 1) % 7;
	p->tm_yday--;
	return 1;
}

#ifdef HAVE_STRFTIME
static PyObject *
time_strftime(PyObject *self, PyObject *args)
{
	PyObject *tup;
	struct tm buf;
	const char *fmt;
	size_t fmtlen, buflen;
	char *outbuf = 0;
	size_t i;

	memset((void *) &buf, '\0', sizeof(buf));

	if (!PyArg_ParseTuple(args, "sO:strftime", &fmt, &tup) 
	    || !gettmarg(tup, &buf))
		return NULL;
	fmtlen = strlen(fmt);

	/* I hate these functions that presume you know how big the output
	 * will be ahead of time...
	 */
	for (i = 1024; ; i += i) {
		outbuf = malloc(i);
		if (outbuf == NULL) {
			return PyErr_NoMemory();
		}
		buflen = strftime(outbuf, i, fmt, &buf);
		if (buflen > 0 || i >= 256 * fmtlen) {
			/* If the buffer is 256 times as long as the format,
			   it's probably not failing for lack of room!
			   More likely, the format yields an empty result,
			   e.g. an empty format, or %Z when the timezone
			   is unknown. */
			PyObject *ret;
			ret = PyString_FromStringAndSize(outbuf, buflen);
			free(outbuf);
			return ret;
		}
		free(outbuf);
	}
}

static char strftime_doc[] =
"strftime(format, tuple) -> string\n\
\n\
Convert a time tuple to a string according to a format specification.\n\
See the library reference manual for formatting codes.";
#endif /* HAVE_STRFTIME */

#ifdef HAVE_STRPTIME

#if 0
/* Enable this if it's not declared in <time.h> */
extern char *strptime(const char *, const char *, struct tm *);
#endif

static PyObject *
time_strptime(PyObject *self, PyObject *args)
{
	struct tm tm;
	char *fmt = "%a %b %d %H:%M:%S %Y";
	char *buf;
	char *s;

	if (!PyArg_ParseTuple(args, "s|s:strptime", &buf, &fmt))
	        return NULL;
	memset((void *) &tm, '\0', sizeof(tm));
	s = strptime(buf, fmt, &tm);
	if (s == NULL) {
		PyErr_SetString(PyExc_ValueError, "format mismatch");
		return NULL;
	}
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (*s) {
		PyErr_Format(PyExc_ValueError,
			     "unconverted data remains: '%.400s'", s);
		return NULL;
	}
	return tmtotuple(&tm);
}

static char strptime_doc[] =
"strptime(string, format) -> tuple\n\
Parse a string to a time tuple according to a format specification.\n\
See the library reference manual for formatting codes (same as strftime()).";
#endif /* HAVE_STRPTIME */

static PyObject *
time_asctime(PyObject *self, PyObject *args)
{
	PyObject *tup;
	struct tm buf;
	char *p;
	if (!PyArg_ParseTuple(args, "O:asctime", &tup))
		return NULL;
	if (!gettmarg(tup, &buf))
		return NULL;
	p = asctime(&buf);
	if (p[24] == '\n')
		p[24] = '\0';
	return PyString_FromString(p);
}

static char asctime_doc[] =
"asctime(tuple) -> string\n\
\n\
Convert a time tuple to a string, e.g. 'Sat Jun 06 16:26:11 1998'.";

static PyObject *
time_ctime(PyObject *self, PyObject *args)
{
	double dt;
	time_t tt;
	char *p;
	if (!PyArg_Parse(args, "d", &dt))
		return NULL;
	tt = (time_t)dt;
#if defined(macintosh) && defined(USE_GUSI204)
	tt = tt + GUSI_TO_MSL_EPOCH;
#endif
	p = ctime(&tt);
	if (p == NULL) {
		PyErr_SetString(PyExc_ValueError, "unconvertible time");
		return NULL;
	}
	if (p[24] == '\n')
		p[24] = '\0';
	return PyString_FromString(p);
}

static char ctime_doc[] =
"ctime(seconds) -> string\n\
\n\
Convert a time in seconds since the Epoch to a string in local time.\n\
This is equivalent to asctime(localtime(seconds)).";

#ifdef HAVE_MKTIME
static PyObject *
time_mktime(PyObject *self, PyObject *args)
{
	PyObject *tup;
	struct tm buf;
	time_t tt;
	if (!PyArg_ParseTuple(args, "O:mktime", &tup))
		return NULL;
	tt = time(&tt);
	buf = *localtime(&tt);
	if (!gettmarg(tup, &buf))
		return NULL;
	tt = mktime(&buf);
	if (tt == (time_t)(-1)) {
		PyErr_SetString(PyExc_OverflowError,
                                "mktime argument out of range");
		return NULL;
	}
#if defined(macintosh) && defined(USE_GUSI2)
	tt = tt - GUSI_TO_MSL_EPOCH;
#endif
	return PyFloat_FromDouble((double)tt);
}

static char mktime_doc[] =
"mktime(tuple) -> floating point number\n\
\n\
Convert a time tuple in local time to seconds since the Epoch.";
#endif /* HAVE_MKTIME */

static PyMethodDef time_methods[] = {
	{"time",	time_time, METH_OLDARGS, time_doc},
#ifdef HAVE_CLOCK
	{"clock",	time_clock, METH_OLDARGS, clock_doc},
#endif
	{"sleep",	time_sleep, METH_OLDARGS, sleep_doc},
	{"gmtime",	time_gmtime, METH_OLDARGS, gmtime_doc},
	{"localtime",	time_localtime, METH_OLDARGS, localtime_doc},
	{"asctime",	time_asctime, METH_VARARGS, asctime_doc},
	{"ctime",	time_ctime, METH_OLDARGS, ctime_doc},
#ifdef HAVE_MKTIME
	{"mktime",	time_mktime, METH_VARARGS, mktime_doc},
#endif
#ifdef HAVE_STRFTIME
	{"strftime",	time_strftime, METH_VARARGS, strftime_doc},
#endif
#ifdef HAVE_STRPTIME
	{"strptime",	time_strptime, METH_VARARGS, strptime_doc},
#endif
	{NULL,		NULL}		/* sentinel */
};

static void
ins(PyObject *d, char *name, PyObject *v)
{
	/* Don't worry too much about errors, they'll be caught by the
	 * caller of inittime().
	 */
	if (v)
		PyDict_SetItemString(d, name, v);
	Py_XDECREF(v);
}


static char module_doc[] =
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
";
  

DL_EXPORT(void)
inittime(void)
{
	PyObject *m, *d;
	char *p;
	m = Py_InitModule3("time", time_methods, module_doc);
	d = PyModule_GetDict(m);
	/* Accept 2-digit dates unless PYTHONY2K is set and non-empty */
	p = getenv("PYTHONY2K");
	ins(d, "accept2dyear", PyInt_FromLong((long) (!p || !*p)));
	/* Squirrel away the module's dictionary for the y2k check */
	Py_INCREF(d);
	moddict = d;
#if defined(HAVE_TZNAME) && !defined(__GLIBC__)
	tzset();
#ifdef PYOS_OS2
	ins(d, "timezone", PyInt_FromLong((long)_timezone));
#else /* !PYOS_OS2 */
	ins(d, "timezone", PyInt_FromLong((long)timezone));
#endif /* PYOS_OS2 */
#ifdef HAVE_ALTZONE
	ins(d, "altzone", PyInt_FromLong((long)altzone));
#else
#ifdef PYOS_OS2
	ins(d, "altzone", PyInt_FromLong((long)_timezone-3600));
#else /* !PYOS_OS2 */
	ins(d, "altzone", PyInt_FromLong((long)timezone-3600));
#endif /* PYOS_OS2 */
#endif
	ins(d, "daylight", PyInt_FromLong((long)daylight));
	ins(d, "tzname", Py_BuildValue("(zz)", tzname[0], tzname[1]));
#else /* !HAVE_TZNAME || __GLIBC__ */
#ifdef HAVE_TM_ZONE
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
			ins(d, "timezone", PyInt_FromLong(julyzone));
			ins(d, "altzone", PyInt_FromLong(janzone));
			ins(d, "daylight",
			    PyInt_FromLong((long)(janzone != julyzone)));
			ins(d, "tzname",
			    Py_BuildValue("(zz)", julyname, janname));
		} else {
			ins(d, "timezone", PyInt_FromLong(janzone));
			ins(d, "altzone", PyInt_FromLong(julyzone));
			ins(d, "daylight",
			    PyInt_FromLong((long)(janzone != julyzone)));
			ins(d, "tzname",
			    Py_BuildValue("(zz)", janname, julyname));
		}
	}
#else
#ifdef macintosh
	/* The only thing we can obtain is the current timezone
	** (and whether dst is currently _active_, but that is not what
	** we're looking for:-( )
	*/
	initmactimezone();
	ins(d, "timezone", PyInt_FromLong(timezone));
	ins(d, "altzone", PyInt_FromLong(timezone));
	ins(d, "daylight", PyInt_FromLong((long)0));
	ins(d, "tzname", Py_BuildValue("(zz)", "", ""));
#endif /* macintosh */
#endif /* HAVE_TM_ZONE */
#endif /* !HAVE_TZNAME || __GLIBC__ */
}


/* Implement floattime() for various platforms */

static double
floattime(void)
{
	/* There are three ways to get the time:
	  (1) gettimeofday() -- resolution in microseconds
	  (2) ftime() -- resolution in milliseconds
	  (3) time() -- resolution in seconds
	  In all cases the return value is a float in seconds.
	  Since on some systems (e.g. SCO ODT 3.0) gettimeofday() may
	  fail, so we fall back on ftime() or time().
	  Note: clock resolution does not imply clock accuracy! */
#ifdef HAVE_GETTIMEOFDAY
	{
		struct timeval t;
#ifdef GETTIMEOFDAY_NO_TZ
		if (gettimeofday(&t) == 0)
			return (double)t.tv_sec + t.tv_usec*0.000001;
#else /* !GETTIMEOFDAY_NO_TZ */
		if (gettimeofday(&t, (struct timezone *)NULL) == 0)
			return (double)t.tv_sec + t.tv_usec*0.000001;
#endif /* !GETTIMEOFDAY_NO_TZ */
	}
#endif /* !HAVE_GETTIMEOFDAY */
	{
#if defined(HAVE_FTIME)
		struct timeb t;
		ftime(&t);
		return (double)t.time + (double)t.millitm * (double)0.001;
#else /* !HAVE_FTIME */
		time_t secs;
		time(&secs);
		return (double)secs;
#endif /* !HAVE_FTIME */
	}
}


/* Implement floatsleep() for various platforms.
   When interrupted (or when another error occurs), return -1 and
   set an exception; else return 0. */

static int
floatsleep(double secs)
{
/* XXX Should test for MS_WIN32 first! */
#if defined(HAVE_SELECT) && !defined(__BEOS__)
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
#else /* !HAVE_SELECT || __BEOS__ */
#ifdef macintosh
#define MacTicks	(* (long *)0x16A)
	long deadline;
	deadline = MacTicks + (long)(secs * 60.0);
	while (MacTicks < deadline) {
		/* XXX Should call some yielding function here */
		if (PyErr_CheckSignals())
			return -1;
	}
#else /* !macintosh */
#if defined(__WATCOMC__) && !defined(__QNX__)
	/* XXX Can't interrupt this sleep */
	Py_BEGIN_ALLOW_THREADS
	delay((int)(secs * 1000 + 0.5));  /* delay() uses milliseconds */
	Py_END_ALLOW_THREADS
#else /* !__WATCOMC__ || __QNX__ */
#ifdef MSDOS
	struct timeb t1, t2;
	double frac;
	extern double fmod(double, double);
	extern double floor(double);
	if (secs <= 0.0)
		return;
	frac = fmod(secs, 1.0);
	secs = floor(secs);
	ftime(&t1);
	t2.time = t1.time + (int)secs;
	t2.millitm = t1.millitm + (int)(frac*1000.0);
	while (t2.millitm >= 1000) {
		t2.time++;
		t2.millitm -= 1000;
	}
	for (;;) {
#ifdef QUICKWIN
		Py_BEGIN_ALLOW_THREADS
		_wyield();
		Py_END_ALLOW_THREADS
#endif
		if (PyErr_CheckSignals())
			return -1;
		ftime(&t1);
		if (t1.time > t2.time ||
		    t1.time == t2.time && t1.millitm >= t2.millitm)
			break;
	}
#else /* !MSDOS */
#ifdef MS_WIN32
	{
		double millisecs = secs * 1000.0;
		if (millisecs > (double)ULONG_MAX) {
			PyErr_SetString(PyExc_OverflowError, "sleep length is too large");
			return -1;
		}
		/* XXX Can't interrupt this sleep */
		Py_BEGIN_ALLOW_THREADS
		Sleep((unsigned long)millisecs);
		Py_END_ALLOW_THREADS
	}
#else /* !MS_WIN32 */
#ifdef PYOS_OS2
	/* This Sleep *IS* Interruptable by Exceptions */
	Py_BEGIN_ALLOW_THREADS
	if (DosSleep(secs * 1000) != NO_ERROR) {
		Py_BLOCK_THREADS
		PyErr_SetFromErrno(PyExc_IOError);
		return -1;
	}
	Py_END_ALLOW_THREADS
#else /* !PYOS_OS2 */
#ifdef __BEOS__
	/* This sleep *CAN BE* interrupted. */
	{
		if( secs <= 0.0 ) {
			return;
		}
		
		Py_BEGIN_ALLOW_THREADS
		/* BeOS snooze() is in microseconds... */
		if( snooze( (bigtime_t)( secs * 1000.0 * 1000.0 ) ) == B_INTERRUPTED ) {
			Py_BLOCK_THREADS
			PyErr_SetFromErrno( PyExc_IOError );
			return -1;
		}
		Py_END_ALLOW_THREADS
	}
#else /* !__BEOS__ */
	/* XXX Can't interrupt this sleep */
	Py_BEGIN_ALLOW_THREADS
	sleep((int)secs);
	Py_END_ALLOW_THREADS
#endif /* !__BEOS__ */
#endif /* !PYOS_OS2 */
#endif /* !MS_WIN32 */
#endif /* !MSDOS */
#endif /* !__WATCOMC__ || __QNX__ */
#endif /* !macintosh */
#endif /* !HAVE_SELECT */
	return 0;
}
