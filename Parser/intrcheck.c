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

/* Check for interrupts */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "myproto.h"
#include "intrcheck.h"

#ifdef macintosh
#ifdef THINK_C
#include <OSEvents.h>
#endif
#include <Events.h>
#endif



#ifdef QUICKWIN

#include <io.h>

void
initintr()
{
}

int
intrcheck()
{
	_wyield();
}

#define OK

#endif /* QUICKWIN */

#ifdef _M_IX86
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
initintr()
{
	_go32_want_ctrl_break(1 /* TRUE */);
}

int
intrcheck()
{
	return _go32_was_ctrl_break_hit();
}

#else /* !__GNUC__ */

/* This might work for MS-DOS (untested though): */

void
initintr()
{
}

int
intrcheck()
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

#ifdef applec /* MPW */
#include <OSEvents.h>
#include <SysEqu.h>
#endif /* applec */

#include <signal.h>

static int interrupted;

static RETSIGTYPE intcatcher PROTO((int));
static RETSIGTYPE
intcatcher(sig)
	int sig;
{
	interrupted = 1;
	signal(SIGINT, intcatcher);
}

void
initintr()
{
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, intcatcher);
}

int
intrcheck()
{
	register EvQElPtr q;
	
	q = (EvQElPtr) GetEvQHdr()->qHead;
	
	for (; q; q = (EvQElPtr)q->qLink) {
		if (q->evtQWhat == keyDown &&
				(char)q->evtQMessage == '.' &&
				(q->evtQModifiers & cmdKey) != 0) {
			FlushEvents(keyDownMask, 0);
			interrupted = 1;
			break;
		}
	}
	if (interrupted) {
		interrupted = 0;
		return 1;
	}
	return 0;
}

/* intrpeek() is like intrcheck(), but it doesn't flush the events. The
** idea is that you call intrpeek() somewhere in a busy-wait loop, and return
** None as soon as it returns 1. The mainloop will then pick up the cmd-. soon
** thereafter.
*/
int
intrpeek()
{
	register EvQElPtr q;
	
	q = (EvQElPtr) GetEvQHdr()->qHead;
	
	for (; q; q = (EvQElPtr)q->qLink) {
		if (q->evtQWhat == keyDown &&
				(char)q->evtQMessage == '.' &&
				(q->evtQModifiers & cmdKey) != 0) {
			return 1;
		}
	}
	return 0;
}

#define OK

#endif /* macintosh */


#ifndef OK

/* Default version -- for real operating systems and for Standard C */

#include <stdio.h>
#include <string.h>
#include <signal.h>

static int interrupted;

/* ARGSUSED */
static RETSIGTYPE
#ifdef _M_IX86
intcatcher(int sig)	/* So the C compiler shuts up */
#else /* _M_IX86 */
intcatcher(sig)
	int sig; /* Not used by required by interface */
#endif /* _M_IX86 */
{
	extern void goaway PROTO((int));
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
		goaway(1);
		break;
	}
	signal(SIGINT, intcatcher);
}

void
initintr()
{
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
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

int
intrcheck()
{
	if (!interrupted)
		return 0;
	interrupted = 0;
	return 1;
}

#endif /* !OK */
