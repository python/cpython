
/* Check for interrupts */

#include "Python.h"

#ifdef QUICKWIN

#include <io.h>

void
PyOS_InitInterrupts(void)
{
}

void
PyOS_FiniInterrupts(void)
{
}

int
PyOS_InterruptOccurred(void)
{
	_wyield();
}

#define OK

#endif /* QUICKWIN */

#if defined(_M_IX86) && !defined(__QNX__)
#include <io.h>
#endif

#if defined(MSDOS) && !defined(QUICKWIN)

#ifdef __GNUC__

/* This is for DJGPP's GO32 extender.  I don't know how to trap
 * control-C  (There's no API for ctrl-C, and I don't want to mess with
 * the interrupt vectors.)  However, this DOES catch control-break.
 * --Amrit
 */

#include <go32.h>

void
PyOS_InitInterrupts(void)
{
	_go32_want_ctrl_break(1 /* TRUE */);
}

void
PyOS_FiniInterrupts(void)
{
}

int
PyOS_InterruptOccurred(void)
{
	return _go32_was_ctrl_break_hit();
}

#else /* !__GNUC__ */

/* This might work for MS-DOS (untested though): */

void
PyOS_InitInterrupts(void)
{
}

void
PyOS_FiniInterrupts(void)
{
}

int
PyOS_InterruptOccurred(void)
{
	int interrupted = 0;
	while (kbhit()) {
		if (getch() == '\003')
			interrupted = 1;
	}
	return interrupted;
}

#endif /* __GNUC__ */

#define OK

#endif /* MSDOS && !QUICKWIN */


#ifdef macintosh

/* The Mac interrupt code has moved to macglue.c */
#define OK

#endif /* macintosh */


#ifndef OK

/* Default version -- for real operating systems and for Standard C */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static int interrupted;

void
PyErr_SetInterrupt(void)
{
	interrupted = 1;
}

extern int PyErr_CheckSignals(void);

static int
checksignals_witharg(void * arg)
{
	return PyErr_CheckSignals();
}

static void
intcatcher(int sig)
{
	extern void Py_Exit(int);
	static char message[] =
"python: to interrupt a truly hanging Python program, interrupt once more.\n";
	switch (interrupted++) {
	case 0:
		break;
	case 1:
		write(2, message, strlen(message));
		break;
	case 2:
		interrupted = 0;
		Py_Exit(1);
		break;
	}
	signal(SIGINT, intcatcher);
	Py_AddPendingCall(checksignals_witharg, NULL);
}

static void (*old_siginthandler)(int) = SIG_DFL;

void
PyOS_InitInterrupts(void)
{
	if ((old_siginthandler = signal(SIGINT, SIG_IGN)) != SIG_IGN)
		signal(SIGINT, intcatcher);
#ifdef HAVE_SIGINTERRUPT
	/* This is for SunOS and other modern BSD derivatives.
	   It means that system calls (like read()) are not restarted
	   after an interrupt.  This is necessary so interrupting a
	   read() or readline() call works as expected.
	   XXX On old BSD (pure 4.2 or older) you may have to do this
	   differently! */
	siginterrupt(SIGINT, 1);
#endif /* HAVE_SIGINTERRUPT */
}

void
PyOS_FiniInterrupts(void)
{
	signal(SIGINT, old_siginthandler);
}

int
PyOS_InterruptOccurred(void)
{
	if (!interrupted)
		return 0;
	interrupted = 0;
	return 1;
}

#endif /* !OK */

void
PyOS_AfterFork(void)
{
#ifdef WITH_THREAD
	PyEval_ReInitThreads();
#endif
}
