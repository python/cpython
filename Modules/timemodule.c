/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

#include "sigtype.h"

#include <signal.h>
#include <setjmp.h>

#ifndef NO_UNISTD
#include <unistd.h>
#endif

/* What happens here is not trivial.
   The BSD_TIME code needs <sys/time.h> (for struct timeval).
   The rest of the code needs only time_t, except some MS-DOS
   code which needs clock_t as well.
   Standard C says that time_t is defined in <time.h>, and
   does not have <sys/types.h>; THINK C agrees (MS-DOS too?).
   What's worse, in pure 4.3 BSD, older SunOS versions, and
   probably everything derived from BSD, you can't #include
   both <time.h> and <sys/time.h> in the same file, since
   <sys/time.h> includes <time.h> without any protection,
   and <time.h> contains a typedef, which can't be parsed twice!
   So on traditional UNIX systems we include <sys/types.h>
   and <sys/time.h> and hope this implies <time.h> and time_t,
   while on other systems, including conforming Standard C
   systems (where 'unix' can't be defined), we rely on <time.h>.
   Still one problem: BSD_TIME won't work with strict Standard C...
*/

#ifdef unix
#include <sys/types.h>
#include <sys/time.h> /* Implies <time.h> everywhere, as far as I know */
#else /* !unix */
#include <time.h>
#endif /* !unix */

#ifdef DO_TIMES
#include <sys/times.h>
#include <sys/param.h>
#include <errno.h>
#endif

#ifdef _SEQUENT_
#include <sys/select.h>
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#endif

/* Time methods */

static object *
time_time(self, args)
	object *self;
	object *args;
{
	time_t secs;
	if (!getnoarg(args))
		return NULL;
	time(&secs);
#ifdef macintosh
/* The Mac epoch is 1904, while UNIX uses 1970; Python prefers 1970 */
/* Moreover, the Mac returns local time.  This we cannot fix... */
#define TIMEDIFF ((time_t) \
	(((1970-1904)*365L + (1970-1904)/4) * 24 * 3600))
	secs -= TIMEDIFF;
#endif
	return newintobject((long)secs);
}

static jmp_buf sleep_intr;

/* ARGSUSED */
static void
sleep_catcher(sig)
	int sig; /* Not used but required by interface */
{
	longjmp(sleep_intr, 1);
}

static object *
time_sleep(self, args)
	object *self;
	object *args;
{
	int secs;
	SIGTYPE (*sigsave)() = 0; /* Initialized to shut lint up */
	if (!getintarg(args, &secs))
		return NULL;
	if (setjmp(sleep_intr)) {
		signal(SIGINT, sigsave);
		err_set(KeyboardInterrupt);
		return NULL;
	}
	sigsave = signal(SIGINT, SIG_IGN);
	if (sigsave != (SIGTYPE (*)()) SIG_IGN)
		signal(SIGINT, sleep_catcher);
	sleep(secs);
	signal(SIGINT, sigsave);
	INCREF(None);
	return None;
}

#ifdef macintosh
#define DO_MILLI
#endif

#ifdef AMOEBA
#define DO_MILLI
extern long sys_milli();
#define millitimer sys_milli
#endif /* AMOEBA */

#ifdef BSD_TIME
#define DO_MILLI
#endif /* BSD_TIME */

#ifdef TURBO_C
#define DO_MILLI
#endif

#ifdef DO_MILLI

static object *
time_millisleep(self, args)
	object *self;
	object *args;
{
	long msecs;
	SIGTYPE (*sigsave)();
	if (!getlongarg(args, &msecs))
		return NULL;
	if (setjmp(sleep_intr)) {
		signal(SIGINT, sigsave);
		err_set(KeyboardInterrupt);
		return NULL;
	}
	sigsave = signal(SIGINT, SIG_IGN);
	if (sigsave != (SIGTYPE (*)()) SIG_IGN)
		signal(SIGINT, sleep_catcher);
	millisleep(msecs);
	signal(SIGINT, sigsave);
	INCREF(None);
	return None;
}

static object *
time_millitimer(self, args)
	object *self;
	object *args;
{
	long msecs;
	extern long millitimer();
	if (!getnoarg(args))
		return NULL;
	msecs = millitimer();
	return newintobject(msecs);
}

#endif /* DO_MILLI */

#ifdef DO_TIMES

static object *
time_times(self, args)
	object *self;
	object *args;
{
	struct tms t;
	clock_t c;
	object *tuple;
	if (!getnoarg(args))
		return NULL;
	errno = 0;
	c = times(&t);
	if (c == (clock_t) -1) {
		err_errno(IOError);
		return NULL;
	}
	tuple = newtupleobject(4);
	if (tuple == NULL)
		return NULL;
	settupleitem(tuple, 0, newfloatobject((double)t.tms_utime / HZ));
	settupleitem(tuple, 1, newfloatobject((double)t.tms_stime / HZ));
	settupleitem(tuple, 2, newfloatobject((double)t.tms_cutime / HZ));
	settupleitem(tuple, 3, newfloatobject((double)t.tms_cstime / HZ));
	if (err_occurred()) {
		DECREF(tuple);
		return NULL;
	}
	return tuple;
}

#endif


static struct methodlist time_methods[] = {
#ifdef DO_MILLI
	{"millisleep",	time_millisleep},
	{"millitimer",	time_millitimer},
#endif /* DO_MILLI */
#ifdef DO_TIMES
	{"times",	time_times},
#endif
	{"sleep",	time_sleep},
	{"time",	time_time},
	{NULL,		NULL}		/* sentinel */
};


void
inittime()
{
	initmodule("time", time_methods);
}


#ifdef macintosh

#define MacTicks	(* (long *)0x16A)

#ifdef THINK_C_3_0
sleep(msecs)
	int msecs;
{
	register long deadline;
	
	deadline = MacTicks + msecs * 60;
	while (MacTicks < deadline) {
		if (intrcheck())
			sleep_catcher(SIGINT);
	}
}
#endif

millisleep(msecs)
	long msecs;
{
	register long deadline;
	
	deadline = MacTicks + msecs * 3 / 50; /* msecs * 60 / 1000 */
	while (MacTicks < deadline) {
		if (intrcheck())
			sleep_catcher(SIGINT);
	}
}

long
millitimer()
{
	return MacTicks * 50 / 3; /* MacTicks * 1000 / 60 */
}

#endif /* macintosh */


#ifdef BSD_TIME

#ifdef _AIX /* I *think* this works */
/* AIX defines fd_set in a separate file.  Sigh... */
#include <sys/select.h>
#endif

long
millitimer()
{
	struct timeval t;
	struct timezone tz;
	if (gettimeofday(&t, &tz) != 0)
		return -1;
	return t.tv_sec*1000 + t.tv_usec/1000;
	
}

millisleep(msecs)
	long msecs;
{
	struct timeval t;
	t.tv_sec = msecs/1000;
	t.tv_usec = (msecs%1000)*1000;
	(void) select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t);
}

#endif /* BSD_TIME */


#ifdef TURBO_C /* Maybe also for MS-DOS? */

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 55	/* 54.945 msec per tick (18.2 HZ clock) */
#endif

static
millisleep(msecs)
	long msecs;
{
	delay(msecs);
}

static long
millitimer()
{
	clock_t ticks;

	ticks = clock();	/* ticks since program start */
	return ticks * CLOCKS_PER_SEC;
}

#endif /* TURBO_C */
