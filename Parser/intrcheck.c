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

/* Check for interrupts */

#include "config.h"
#include "myproto.h"
#include "mymalloc.h" /* For ANY */
#include "intrcheck.h"

/* Copied here from ceval.h -- can't include that file. */
int Py_AddPendingCall Py_PROTO((int (*func) Py_PROTO((ANY *)), ANY *arg));


#ifdef QUICKWIN

#include <io.h>

void
PyOS_InitInterrupts()
{
}

void
PyOS_FiniInterrupts()
{
}

int
PyOS_InterruptOccurred()
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
PyOS_InitInterrupts()
{
	_go32_want_ctrl_break(1 /* TRUE */);
}

void
PyOS_FiniInterrupts()
{
}

int
PyOS_InterruptOccurred()
{
	return _go32_was_ctrl_break_hit();
}

#else /* !__GNUC__ */

/* This might work for MS-DOS (untested though): */

void
PyOS_InitInterrupts()
{
}

void
PyOS_FiniInterrupts()
{
}

int
PyOS_InterruptOccurred()
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
PyErr_SetInterrupt()
{
	interrupted = 1;
}

extern int PyErr_CheckSignals();

/* ARGSUSED */
static RETSIGTYPE
#if defined(_M_IX86) && !defined(__QNX__)
intcatcher(int sig)	/* So the C compiler shuts up */
#else /* _M_IX86 */
intcatcher(sig)
	int sig; /* Not used by required by interface */
#endif /* _M_IX86 */
{
	extern void Py_Exit Py_PROTO((int));
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
	Py_AddPendingCall(PyErr_CheckSignals, NULL);
}

static RETSIGTYPE (*old_siginthandler)() = SIG_DFL;

void
PyOS_InitInterrupts()
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
PyOS_FiniInterrupts()
{
	signal(SIGINT, old_siginthandler);
}

int
PyOS_InterruptOccurred()
{
	if (!interrupted)
		return 0;
	interrupted = 0;
	return 1;
}

#endif /* !OK */

void
PyOS_AfterFork()
{
}
