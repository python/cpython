/* Check for interrupts */

#ifdef MSDOS

/* This might work for MS-DOS: */

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

#include <MacHeaders>

void
initintr()
{
}

int
intrcheck()
{
	/* Static to make it faster(?) only */
	static EventRecord e;
	
	/* XXX This fails if the user first types ahead and then
	   decides to interrupt -- repeating Command-. until the
	   event queue overflows may work though. */
	if (EventAvail(keyDownMask|autoKeyMask, &e) &&
				(e.modifiers & cmdKey) &&
				(e.message & charCodeMask) == '.') {
		(void) GetNextEvent(keyDownMask|autoKeyMask, &e);
		return 1;
	}
	return 0;
}

#define OK

#endif /* THINK_C */


#ifndef OK

/* Default version -- should work for Unix and Standard C */

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
