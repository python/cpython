/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

/* Signal module -- many thanks to Lance Ellinghouse */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"
#include "intrcheck.h"

#include <signal.h>
#include <errno.h>

struct signalhandler_list {
	int	tripped;
	object *func;
};

static struct signalhandler_list sig_list[NSIG];
static int tripped = 0;		/* Speed up sigcheck() when none tripped */

static object *default_sig_object;
static object *default_ignore_object;
static object *default_int_object;

static object *
default_int_handler(self, arg)
     object *self;
     object *arg;
{
	err_set(KeyboardInterrupt);
	return NULL;
}

static RETSIGTYPE
signal_handler(sig_num)
     int sig_num;
{
	tripped++;
	sig_list[sig_num].tripped = 1;
	(void *)signal(sig_num, &signal_handler);
}
 

static object *
signal_signal(self, args)
	object *self; /* Not used */
	object *args;
{
        object *obj;
	int sig_num;
	object *old_handler;
	RETSIGTYPE (*func)();
	if (!getargs(args, "(iO)", &sig_num, &obj))
		return NULL;
	if (sig_num < 1 || sig_num >= NSIG) {
		err_setstr(ValueError, "signal number out of range");
		return NULL;
	}
	if (obj == default_ignore_object)
		func = SIG_IGN;
	else if (obj == default_sig_object)
		func = SIG_DFL;
	else if (!is_methodobject(obj) &&
		 !is_funcobject(obj) &&
		 !is_instancemethodobject(obj)) {
		err_setstr(TypeError,
"signal handler must be signal.SIG_IGN, signal.SIG_DFL, or a callable object");
		return NULL;
	}
	else
		func = signal_handler;
	old_handler = sig_list[sig_num].func;
	/* XXX is it always correct to clear tripped? */
	sig_list[sig_num].tripped = 0;
	INCREF(obj);
	sig_list[sig_num].func = obj;
	signal(sig_num, func);
	return old_handler;
}

static object *
signal_getsignal(self, args)
	object *self; /* Not used */
	object *args;
{
	int sig_num;
	object *old_handler;
	if (!getargs(args, "i", &sig_num))
		return NULL;
	if (sig_num < 1 || sig_num >= NSIG) {
		err_setstr(ValueError, "signal number out of range");
		return NULL;
	}
	old_handler = sig_list[sig_num].func;
	INCREF(old_handler);
	return old_handler;
}


/* List of functions defined in the module */

static struct methodlist signal_methods[] = {
        {"signal",      signal_signal},
        {"getsignal",      signal_getsignal},
	{NULL,		NULL}		/* sentinel */
};

void
initsignal()
{
	object *m, *d, *x;
	object *b_dict;
	int i;
	/* Create the module and add the functions */
	m = initmodule("signal", signal_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);

	default_sig_object = newintobject((long)SIG_DFL);
	dictinsert(d, "SIG_IGN", default_sig_object);
	default_ignore_object = newintobject((long)SIG_IGN);
	dictinsert(d, "SIG_DFL", default_ignore_object);
	default_int_object = newmethodobject("default_int_handler",
					     default_int_handler,
					     (object *)NULL,
					     0);
	dictinsert(d, "default_int_handler", default_int_object);

	sig_list[0].tripped = 0;
	for (i = 1; i < NSIG; i++) {
		RETSIGTYPE (*t)();
		t = signal(i, SIG_IGN);
		signal(i, t);
		sig_list[i].tripped = 0;
		if (t == SIG_DFL)
			sig_list[i].func = default_sig_object;
		else if (t == SIG_IGN)
			sig_list[i].func = default_ignore_object;
		else
			sig_list[i].func = None; /* None of our business */
		INCREF(sig_list[i].func);
	}
	if (sig_list[SIGINT].func == default_sig_object) {
		/* Install default int handler */
		DECREF(sig_list[SIGINT].func);
		sig_list[SIGINT].func = default_int_object;
		INCREF(default_int_object);
		signal(SIGINT, &signal_handler);
	}

#ifdef SIGHUP
	x = newintobject(SIGHUP);
	dictinsert(d, "SIGHUP", x);
#endif
#ifdef SIGINT
	x = newintobject(SIGINT);
	dictinsert(d, "SIGINT", x);
#endif
#ifdef SIGQUIT
	x = newintobject(SIGQUIT);
	dictinsert(d, "SIGQUIT", x);
#endif
#ifdef SIGILL
	x = newintobject(SIGILL);
	dictinsert(d, "SIGILL", x);
#endif
#ifdef SIGTRAP
	x = newintobject(SIGTRAP);
	dictinsert(d, "SIGTRAP", x);
#endif
#ifdef SIGIOT
	x = newintobject(SIGIOT);
	dictinsert(d, "SIGIOT", x);
#endif
#ifdef SIGABRT
	x = newintobject(SIGABRT);
	dictinsert(d, "SIGABRT", x);
#endif
#ifdef SIGEMT
	x = newintobject(SIGEMT);
	dictinsert(d, "SIGEMT", x);
#endif
#ifdef SIGFPE
	x = newintobject(SIGFPE);
	dictinsert(d, "SIGFPE", x);
#endif
#ifdef SIGKILL
	x = newintobject(SIGKILL);
	dictinsert(d, "SIGKILL", x);
#endif
#ifdef SIGBUS
	x = newintobject(SIGBUS);
	dictinsert(d, "SIGBUS", x);
#endif
#ifdef SIGSEGV
	x = newintobject(SIGSEGV);
	dictinsert(d, "SIGSEGV", x);
#endif
#ifdef SIGSYS
	x = newintobject(SIGSYS);
	dictinsert(d, "SIGSYS", x);
#endif
#ifdef SIGPIPE
	x = newintobject(SIGPIPE);
	dictinsert(d, "SIGPIPE", x);
#endif
#ifdef SIGALRM
	x = newintobject(SIGALRM);
	dictinsert(d, "SIGALRM", x);
#endif
#ifdef SIGTERM
	x = newintobject(SIGTERM);
	dictinsert(d, "SIGTERM", x);
#endif
#ifdef SIGUSR1
	x = newintobject(SIGUSR1);
	dictinsert(d, "SIGUSR1", x);
#endif
#ifdef SIGUSR2
	x = newintobject(SIGUSR2);
	dictinsert(d, "SIGUSR2", x);
#endif
#ifdef SIGCLD
	x = newintobject(SIGCLD);
	dictinsert(d, "SIGCLD", x);
#endif
#ifdef SIGCHLD
	x = newintobject(SIGCHLD);
	dictinsert(d, "SIGCHLD", x);
#endif
#ifdef SIGPWR
	x = newintobject(SIGPWR);
	dictinsert(d, "SIGPWR", x);
#endif
#ifdef SIGIO
	x = newintobject(SIGIO);
	dictinsert(d, "SIGIO", x);
#endif
#ifdef SIGURG
	x = newintobject(SIGURG);
	dictinsert(d, "SIGURG", x);
#endif
#ifdef SIGWINCH
	x = newintobject(SIGWINCH);
	dictinsert(d, "SIGWINCH", x);
#endif
#ifdef SIGPOLL
	x = newintobject(SIGPOLL);
	dictinsert(d, "SIGPOLL", x);
#endif
#ifdef SIGSTOP
	x = newintobject(SIGSTOP);
	dictinsert(d, "SIGSTOP", x);
#endif
#ifdef SIGTSTP
	x = newintobject(SIGTSTP);
	dictinsert(d, "SIGTSTP", x);
#endif
#ifdef SIGCONT
	x = newintobject(SIGCONT);
	dictinsert(d, "SIGCONT", x);
#endif
#ifdef SIGTTIN
	x = newintobject(SIGTTIN);
	dictinsert(d, "SIGTTIN", x);
#endif
#ifdef SIGTTOU
	x = newintobject(SIGTTOU);
	dictinsert(d, "SIGTTOU", x);
#endif
#ifdef SIGVTALRM
	x = newintobject(SIGVTALRM);
	dictinsert(d, "SIGVTALRM", x);
#endif
#ifdef SIGPROF
	x = newintobject(SIGPROF);
	dictinsert(d, "SIGPROF", x);
#endif
#ifdef SIGCPU
	x = newintobject(SIGCPU);
	dictinsert(d, "SIGCPU", x);
#endif
#ifdef SIGFSZ
	x = newintobject(SIGFSZ);
	dictinsert(d, "SIGFSZ", x);
#endif
	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module signal");
}

int
sigcheck()
{
	int i;
	object *f;
	if (!tripped)
		return 0;
	f = getframe();
	if (f == NULL)
		f = None;
	for (i = 1; i < NSIG; i++) {
		if (sig_list[i].tripped) {
			object *arglist, *result;
			sig_list[i].tripped = 0;
			arglist = mkvalue("(iO)", i, f);
			if (arglist == NULL)
				result = NULL;
			else {
				result = call_object(sig_list[i].func,arglist);
				DECREF(arglist);
			}
			if (result == NULL) {
				return 1;
			} else {
				DECREF(result);
			}
		}
	}
	tripped = 0;
	return 0;
}

/* Replacement for intrcheck.c functionality */

void
initintr()
{
	initsignal();
}

int
intrcheck()
{
	if (sig_list[SIGINT].tripped) {
		sig_list[SIGINT].tripped = 0;
		return 1;
	}
	return 0;
}
