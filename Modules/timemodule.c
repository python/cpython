/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Time module */

#include "Python.h"

#ifdef HAVE_SELECT
#include "mymath.h"
#endif

#ifdef macintosh
#include <time.h>
#else
#include <sys/types.h>
#endif

#ifdef QUICKWIN
#include <io.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SELECT
#include "myselect.h"
#else
#include "mytime.h"
#endif

#ifdef HAVE_FTIME
#include <sys/timeb.h>
#if !defined(MS_WINDOWS) && !defined(PYOS_OS2)
extern int ftime();
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

#ifdef MS_WIN32
/* Win32 has better clock replacement */
#include <largeint.h>
#undef HAVE_CLOCK /* We have our own version down below */
#endif /* MS_WIN32 */

#if defined(PYOS_OS2)
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_NOPMAPI
#include <os2.h>
#endif

#if defined(PYCC_VACPP)
#include <time.h>
#define timezone _timezone
#endif

/* Forward declarations */
static int floatsleep Py_PROTO((double));
static double floattime Py_PROTO(());

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
initmactimezone()
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
time_time(self, args)
	PyObject *self;
	PyObject *args;
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

#ifdef HAVE_CLOCK

#ifndef CLOCKS_PER_SEC
#ifdef CLK_TCK
#define CLOCKS_PER_SEC CLK_TCK
#else
#define CLOCKS_PER_SEC 1000000
#endif
#endif

static PyObject *
time_clock(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyFloat_FromDouble(((double)clock()) / CLOCKS_PER_SEC);
}
#endif /* HAVE_CLOCK */

#ifdef MS_WIN32
/* Due to Mark Hammond */
static PyObject *
time_clock(self, args)
	PyObject *self;
	PyObject *args;
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
#endif /* MS_WIN32 */

static PyObject *
time_sleep(self, args)
	PyObject *self;
	PyObject *args;
{
	double secs;
	if (!PyArg_Parse(args, "d", &secs))
		return NULL;
	if (floatsleep(secs) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
time_convert(when, function)
	time_t when;
	struct tm * (*function) Py_PROTO((const time_t *));
{
	struct tm *p;
	errno = 0;
	p = function(&when);
	if (p == NULL) {
#ifdef EINVAL
		if (errno == 0)
			errno = EINVAL;
#endif
		return PyErr_SetFromErrno(PyExc_IOError);
	}
	return Py_BuildValue("(iiiiiiiii)",
			     p->tm_year + 1900,
			     p->tm_mon + 1,        /* Want January == 1 */
			     p->tm_mday,
			     p->tm_hour,
			     p->tm_min,
			     p->tm_sec,
			     (p->tm_wday + 6) % 7, /* Want Monday == 0 */
			     p->tm_yday + 1,       /* Want January, 1 == 1 */
			     p->tm_isdst);
}

static PyObject *
time_gmtime(self, args)
	PyObject *self;
	PyObject *args;
{
	double when;
	if (!PyArg_Parse(args, "d", &when))
		return NULL;
	return time_convert((time_t)when, gmtime);
}

static PyObject *
time_localtime(self, args)
	PyObject *self;
	PyObject *args;
{
	double when;
	if (!PyArg_Parse(args, "d", &when))
		return NULL;
	return time_convert((time_t)when, localtime);
}

static int
gettmarg(args, p)
	PyObject *args;
	struct tm *p;
{
	if (!PyArg_Parse(args, "(iiiiiiiii)",
			 &p->tm_year,
			 &p->tm_mon,
			 &p->tm_mday,
			 &p->tm_hour,
			 &p->tm_min,
			 &p->tm_sec,
			 &p->tm_wday,
			 &p->tm_yday,
			 &p->tm_isdst))
		return 0;
	if (p->tm_year >= 1900)
		p->tm_year -= 1900;
	p->tm_mon--;
	p->tm_wday = (p->tm_wday + 1) % 7;
	p->tm_yday--;
	return 1;
}

#ifdef HAVE_STRFTIME
static PyObject *
time_strftime(self, args)
	PyObject *self;
	PyObject *args;
{
	struct tm buf;
	const char *fmt;
	char *outbuf = 0;
	int i;

	if (!PyArg_ParseTuple(args, "s(iiiiiiiii)",
			      &fmt,
			      &(buf.tm_year),
			      &(buf.tm_mon),
			      &(buf.tm_mday),
			      &(buf.tm_hour),
			      &(buf.tm_min),
			      &(buf.tm_sec),
			      &(buf.tm_wday),
			      &(buf.tm_yday),
			      &(buf.tm_isdst)))
		return NULL;
	if (buf.tm_year >= 1900)
		buf.tm_year -= 1900;
	buf.tm_mon--;
	buf.tm_wday = (buf.tm_wday + 1) % 7;
	buf.tm_yday--;
	/* I hate these functions that presume you know how big the output
	 * will be ahead of time...
	 */
	for (i = 1024 ; i <= 8192 ; i += 1024) {
		outbuf = malloc(i);
		if (outbuf == NULL) {
			return PyErr_NoMemory();
		}
		if (strftime(outbuf, i-1, fmt, &buf) != 0) {
			PyObject *ret;
			ret = PyString_FromString(outbuf);
			free(outbuf);
			return ret;
		}
		free(outbuf);
	}
	PyErr_SetString(PyExc_ValueError,
			"bad strftime format or result too big");
	return NULL;
}
#endif /* HAVE_STRFTIME */

static PyObject *
time_asctime(self, args)
	PyObject *self;
	PyObject *args;
{
	struct tm buf;
	char *p;
	if (!gettmarg(args, &buf))
		return NULL;
	p = asctime(&buf);
	if (p[24] == '\n')
		p[24] = '\0';
	return PyString_FromString(p);
}

static PyObject *
time_ctime(self, args)
	PyObject *self;
	PyObject *args;
{
	double dt;
	time_t tt;
	char *p;
	if (!PyArg_Parse(args, "d", &dt))
		return NULL;
	tt = (time_t)dt;
	p = ctime(&tt);
	if (p[24] == '\n')
		p[24] = '\0';
	return PyString_FromString(p);
}

static PyObject *
time_mktime(self, args)
	PyObject *self;
	PyObject *args;
{
	struct tm buf;
	time_t tt;
	tt = time(&tt);
	buf = *localtime(&tt);
	if (!gettmarg(args, &buf))
		return NULL;
	tt = mktime(&buf);
	if (tt == (time_t)(-1)) {
		PyErr_SetString(PyExc_OverflowError,
                                "mktime argument out of range");
		return NULL;
	}
	return PyFloat_FromDouble((double)tt);
}

static PyMethodDef time_methods[] = {
	{"time",	time_time},
#ifdef HAVE_CLOCK
	{"clock",	time_clock},
#endif
	{"sleep",	time_sleep},
	{"gmtime",	time_gmtime},
	{"localtime",	time_localtime},
	{"asctime",	time_asctime},
	{"ctime",	time_ctime},
	{"mktime",	time_mktime},
#ifdef HAVE_STRFTIME
	{"strftime",	time_strftime, 1},
#endif
	{NULL,		NULL}		/* sentinel */
};

static void
ins(d, name, v)
	PyObject *d;
	char *name;
	PyObject *v;
{
	if (v == NULL)
		Py_FatalError("Can't initialize time module -- NULL value");
	if (PyDict_SetItemString(d, name, v) != 0)
		Py_FatalError(
		"Can't initialize time module -- PyDict_SetItemString failed");
	Py_DECREF(v);
}

void
inittime()
{
	PyObject *m, *d;
	m = Py_InitModule("time", time_methods);
	d = PyModule_GetDict(m);
#ifdef HAVE_TZNAME
	tzset();
	ins(d, "timezone", PyInt_FromLong((long)timezone));
#ifdef HAVE_ALTZONE
	ins(d, "altzone", PyInt_FromLong((long)altzone));
#else
	ins(d, "altzone", PyInt_FromLong((long)timezone-3600));
#endif
	ins(d, "daylight", PyInt_FromLong((long)daylight));
	ins(d, "tzname", Py_BuildValue("(zz)", tzname[0], tzname[1]));
#else /* !HAVE_TZNAME */
#if HAVE_TM_ZONE
	{
#define YEAR ((time_t)((365 * 24 + 6) * 3600))
		time_t t;
		struct tm *p;
		long winterzone, summerzone;
		char wintername[10], summername[10];
		/* XXX This won't work on the southern hemisphere.
		  XXX Anybody got a better idea? */
		t = (time((time_t *)0) / YEAR) * YEAR;
		p = localtime(&t);
		winterzone = -p->tm_gmtoff;
		strncpy(wintername, p->tm_zone ? p->tm_zone : "   ", 9);
		wintername[9] = '\0';
		t += YEAR/2;
		p = localtime(&t);
		summerzone = -p->tm_gmtoff;
		strncpy(summername, p->tm_zone ? p->tm_zone : "   ", 9);
		summername[9] = '\0';
		ins(d, "timezone", PyInt_FromLong(winterzone));
		ins(d, "altzone", PyInt_FromLong(summerzone));
		ins(d, "daylight",
                    PyInt_FromLong((long)(winterzone != summerzone)));
		ins(d, "tzname",
                    Py_BuildValue("(zz)", wintername, summername));
	}
#else
#ifdef macintosh
	initmactimezone();
	ins(d, "timezone", PyInt_FromLong(timezone));
#endif /* macintosh */
#endif /* HAVE_TM_ZONE */
#endif /* !HAVE_TZNAME */
	if (PyErr_Occurred())
		Py_FatalError("Can't initialize time module");
}


/* Implement floattime() for various platforms */

static double
floattime()
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
#ifdef HAVE_FTIME
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
#ifdef MPW
floatsleep(double secs)
#else
	floatsleep(secs)
	double secs;
#endif /* MPW */
{
/* XXX Should test for MS_WIN32 first! */
#ifdef HAVE_SELECT
	struct timeval t;
	double frac;
	frac = fmod(secs, 1.0);
	secs = floor(secs);
	t.tv_sec = (long)secs;
	t.tv_usec = (long)(frac*1000000.0);
	Py_BEGIN_ALLOW_THREADS
	if (select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t) != 0) {
		Py_BLOCK_THREADS
		PyErr_SetFromErrno(PyExc_IOError);
		return -1;
	}
	Py_END_ALLOW_THREADS
#else /* !HAVE_SELECT */
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
	extern double fmod Py_PROTO((double, double));
	extern double floor Py_PROTO((double));
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
	/* XXX Can't interrupt this sleep */
	Py_BEGIN_ALLOW_THREADS
	Sleep((int)(secs*1000));
	Py_END_ALLOW_THREADS
#else /* !MS_WIN32 */
#ifdef PYOS_OS2
	/* This Sleep *IS* Interruptable by Exceptions */
	if (DosSleep(secs * 1000) != NO_ERROR) {
		PyErr_SetFromErrno(PyExc_IOError);
		return -1;
	}
#else /* !PYOS_OS2 */
	/* XXX Can't interrupt this sleep */
	Py_BEGIN_ALLOW_THREADS
	sleep((int)secs);
	Py_END_ALLOW_THREADS
#endif /* !PYOS_OS2 */
#endif /* !MS_WIN32 */
#endif /* !MSDOS */
#endif /* !__WATCOMC__ || __QNX__ */
#endif /* !macintosh */
#endif /* !HAVE_SELECT */
	return 0;
}
