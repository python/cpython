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

/* Check for interrupts */

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


#ifdef THINK_C

/* This is for THINK C 4.0.
   For 3.0, you may have to remove the signal stuff. */

#include <MacHeaders>
#include <signal.h>
#include "sigtype.h"

static int interrupted;

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
	
	/* This is like THINK C 4.0's <console.h>.
	   I'm not sure why FlushEvents must be called from asm{}. */
	for (q = (EvQElPtr)EventQueue.qHead; q; q = (EvQElPtr)q->qLink) {
		if (q->evtQWhat == keyDown &&
				(char)q->evtQMessage == '.' &&
				(q->evtQModifiers & cmdKey) != 0) {
			
			asm {
				moveq	#keyDownMask,d0
				_FlushEvents
			}
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

#endif /* THINK_C */


#ifndef OK

/* Default version -- for real operating systems and for Standard C */

#include <stdio.h>
#include <signal.h>
#include "sigtype.h"

static int interrupted;

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
	if (!interrupted)
		return 0;
	interrupted = 0;
	return 1;
}

#endif /* !OK */
