/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
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

#ifdef __STDC__
#include <time.h>
#else /* !__STDC__ */
typedef unsigned long time_t;
extern time_t time();
#endif /* !__STDC__ */


/* Time methods */

static object *
time_time(self, args)
	object *self;
	object *args;
{
	long secs;
	if (!getnoarg(args))
		return NULL;
	secs = time((time_t *)NULL);
	return newintobject(secs);
}

static jmp_buf sleep_intr;

static void
sleep_catcher(sig)
	int sig;
{
	longjmp(sleep_intr, 1);
}

static object *
time_sleep(self, args)
	object *self;
	object *args;
{
	int secs;
	SIGTYPE (*sigsave)();
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

#ifdef THINK_C
#define DO_MILLI
#endif /* THINK_C */

#ifdef AMOEBA
#define DO_MILLI
extern long sys_milli();
#define millitimer sys_milli
#endif /* AMOEBA */

#ifdef BSD_TIME
#define DO_MILLI
#endif /* BSD_TIME */

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


static struct methodlist time_methods[] = {
#ifdef DO_MILLI
	{"millisleep",	time_millisleep},
	{"millitimer",	time_millitimer},
#endif /* DO_MILLI */
	{"sleep",	time_sleep},
	{"time",	time_time},
	{NULL,		NULL}		/* sentinel */
};


void
inittime()
{
	initmodule("time", time_methods);
}


#ifdef THINK_C

#define MacTicks	(* (long *)0x16A)

static
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

static
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

static long
millitimer()
{
	return MacTicks * 50 / 3; /* MacTicks * 1000 / 60 */
}

#endif /* THINK_C */


#ifdef BSD_TIME

#include <sys/types.h>
#include <sys/time.h>

static long
millitimer()
{
	struct timeval t;
	struct timezone tz;
	if (gettimeofday(&t, &tz) != 0)
		return -1;
	return t.tv_sec*1000 + t.tv_usec/1000;
	
}

static
millisleep(msecs)
	long msecs;
{
	struct timeval t;
	t.tv_sec = msecs/1000;
	t.tv_usec = (msecs%1000)*1000;
	(void) select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t);
}

#endif /* BSD_TIME */

