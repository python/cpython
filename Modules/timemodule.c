/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Time module */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

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
#endif

#ifdef _M_IX86
#include <windows.h>
#define timezone _timezone
#define tzname _tzname
#define daylight _daylight
#define altzone _altzone
#endif

/* Forward declarations */
static int floatsleep PROTO((double));
static double floattime PROTO(());

static object *
time_time(self, args)
	object *self;
	object *args;
{
	double secs;
	if (!getnoarg(args))
		return NULL;
	secs = floattime();
	if (secs == 0.0) {
		err_errno(IOError);
		return NULL;
	}
	return newfloatobject(secs);
}

#ifdef HAVE_CLOCK

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000
#endif

static object *
time_clock(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newfloatobject(((double)clock()) / CLOCKS_PER_SEC);
}
#endif /* HAVE_CLOCK */

static object *
time_sleep(self, args)
	object *self;
	object *args;
{
	double secs;
	if (!getargs(args, "d", &secs))
		return NULL;
	BGN_SAVE
	if (floatsleep(secs) != 0) {
		RET_SAVE
		return NULL;
	}
	END_SAVE
	INCREF(None);
	return None;
}

static object *
time_convert(when, function)
	time_t when;
	struct tm * (*function) PROTO((const time_t *));
{
	struct tm *p = function(&when);
	return mkvalue("(iiiiiiiii)",
		       p->tm_year + 1900,
		       p->tm_mon + 1, /* Want January == 1 */
		       p->tm_mday,
		       p->tm_hour,
		       p->tm_min,
		       p->tm_sec,
		       (p->tm_wday + 6) % 7, /* Want Monday == 0 */
		       p->tm_yday + 1, /* Want January, 1 == 1 */
		       p->tm_isdst);
}

static object *
time_gmtime(self, args)
	object *self;
	object *args;
{
	double when;
	if (!getargs(args, "d", &when))
		return NULL;
	return time_convert((time_t)when, gmtime);
}

static object *
time_localtime(self, args)
	object *self;
	object *args;
{
	double when;
	if (!getargs(args, "d", &when))
		return NULL;
	return time_convert((time_t)when, localtime);
}

static int
gettmarg(args, p)
	object *args;
	struct tm *p;
{
	if (!getargs(args, "(iiiiiiiii)",
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
static object *
time_strftime(self, args)
	object *self;
	object *args;
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
	/* I hate these functions that presume you know how big the output */
	/* will be ahead of time... */
	for (i = 1024 ; i < 8192 ; i += 1024) {
		outbuf = malloc(i);
		if (outbuf == NULL) {
			return err_nomem();
		}
		if (strftime(outbuf, i-1, fmt, &buf) != 0) {
			object *ret;
			ret = newstringobject(outbuf);
			free(outbuf);
			return ret;
		}
		free(outbuf);
	}
	return err_nomem();
}
#endif /* HAVE_STRFTIME */

static object *
time_asctime(self, args)
	object *self;
	object *args;
{
	struct tm buf;
	char *p;
	if (!gettmarg(args, &buf))
		return NULL;
	p = asctime(&buf);
	if (p[24] == '\n')
		p[24] = '\0';
	return newstringobject(p);
}

static object *
time_ctime(self, args)
	object *self;
	object *args;
{
	double dt;
	time_t tt;
	char *p;
	if (!getargs(args, "d", &dt))
		return NULL;
	tt = dt;
	p = ctime(&tt);
	if (p[24] == '\n')
		p[24] = '\0';
	return newstringobject(p);
}

static object *
time_mktime(self, args)
	object *self;
	object *args;
{
	struct tm buf;
	if (!gettmarg(args, &buf))
		return NULL;
	return newintobject((long)mktime(&buf));
}

static struct methodlist time_methods[] = {
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
	{"strftime",	time_strftime},
#endif
	{NULL,		NULL}		/* sentinel */
};

static void
ins(d, name, v)
	object *d;
	char *name;
	object *v;
{
	if (v == NULL)
		fatal("Can't initialize time module -- NULL value");
	if (dictinsert(d, name, v) != 0)
		fatal("Can't initialize time module -- dictinsert failed");
	DECREF(v);
}

void
inittime()
{
	object *m, *d, *v;
	m = initmodule("time", time_methods);
	d = getmoduledict(m);
#ifdef HAVE_TZNAME
	tzset();
	ins(d, "timezone", newintobject((long)timezone));
#ifdef HAVE_ALTZONE
	ins(d, "altzone", newintobject((long)altzone));
#else
	ins(d, "altzone", newintobject((long)timezone-3600));
#endif
	ins(d, "daylight", newintobject((long)daylight));
	ins(d, "tzname", mkvalue("(zz)", tzname[0], tzname[1]));
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
		ins(d, "timezone", newintobject(winterzone));
		ins(d, "altzone", newintobject(summerzone));
		ins(d, "daylight", newintobject((long)(winterzone != summerzone)));
		ins(d, "tzname",  mkvalue("(zz)", wintername, summername));
	}
#endif /* HAVE_TM_ZONE */
#endif /* !HAVE_TZNAME */
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
#ifdef HAVE_SELECT
	struct timeval t;
	double frac;
	extern double fmod PROTO((double, double));
	extern double floor PROTO((double));
	frac = fmod(secs, 1.0);
	secs = floor(secs);
	t.tv_sec = (long)secs;
	t.tv_usec = (long)(frac*1000000.0);
	if (select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t) != 0) {
		err_errno(IOError);
		return -1;
	}
#else /* !HAVE_SELECT */
#ifdef macintosh
#define MacTicks	(* (long *)0x16A)
	long deadline;
	deadline = MacTicks + (long)(secs * 60.0);
	while (MacTicks < deadline) {
		if (sigcheck())
			return -1;
	}
#else /* !macintosh */
#ifdef MSDOS
	struct timeb t1, t2;
	double frac;
	extern double fmod PROTO((double, double));
	extern double floor PROTO((double));
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
		_wyield();
#endif
		if (sigcheck())
			return -1;
		ftime(&t1);
		if (t1.time > t2.time ||
		    t1.time == t2.time && t1.millitm >= t2.millitm)
			break;
	}
#else /* !MSDOS */
#ifdef _M_IX86
	/* XXX Can't interrupt this sleep */
	Sleep((int)(secs*1000));
#else /* _M_IX86 */
	/* XXX Can't interrupt this sleep */
	sleep((int)secs);
#endif /* _M_IX86 */
#endif /* !MSDOS */
#endif /* !macintosh */
#endif /* !HAVE_SELECT */
	return 0;
}
