/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
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

/* Check for interrupts */

#ifdef THINK_C
/* This is for THINK C 4.0.
   For 3.0, you may have to remove the signal stuff. */
#include <MacHeaders>
#define macintosh
#endif

#include "PROTO.h"
#include "intrcheck.h"


#ifdef MSDOS

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

#define OK

#endif


#ifdef macintosh

#ifdef applec /* MPW */
#include <OSEvents.h>
#include <SysEqu.h>
#endif

#include <signal.h>
#include "sigtype.h"

static int interrupted;

static SIGTYPE intcatcher PROTO((int));
static SIGTYPE
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

#define OK

#endif /* macintosh */


#ifndef OK

/* Default version -- for real operating systems and for Standard C */

#include <stdio.h>
#include <signal.h>
#include "sigtype.h"

static int interrupted;

/* ARGSUSED */
static SIGTYPE
intcatcher(sig)
	int sig; /* Not used by required by interface */
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
#ifdef SV_INTERRUPT
	/* This is for SunOS and other modern BSD derivatives.
	   It means that system calls (like read()) are not restarted
	   after an interrupt.  This is necessary so interrupting a
	   read() or readline() call works as expected.
	   XXX On old BSD (pure 4.2 or older) you may have to do this
	   differently! */
	siginterrupt(SIGINT, 1);
#endif
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
