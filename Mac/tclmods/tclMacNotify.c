/* 
 * tclMacNotify.c --
 *
 *	This file contains Macintosh-specific procedures for the notifier,
 *	which is the lowest-level part of the Tcl event loop.  This file
 *	works together with ../generic/tclNotify.c.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tclMacNotify.c 1.36 97/05/07 19:09:29
 */

#ifdef USE_GUSI
/* Move this include up otherwise tclPort.h tried to redefine signals */
#include <sys/signal.h>
#endif
#include "tclInt.h"
#include "tclPort.h"
#include "tclMac.h"
#include "tclMacInt.h"
#include <signal.h>
#include <Events.h>
#include <LowMem.h>
#include <Processes.h>
#include <Timer.h>


/* 
 * This is necessary to work around a bug in Apple's Universal header files
 * for the CFM68K libraries.
 */

#ifdef __CFM68K__
#undef GetEventQueue
extern pascal QHdrPtr GetEventQueue(void)
 THREEWORDINLINE(0x2EBC, 0x0000, 0x014A);
#pragma import list GetEventQueue
#define GetEvQHdr() GetEventQueue()
#endif

/*
 * The follwing static indicates whether this module has been initialized.
 */

static int initialized = 0;

/*
 * The following structure contains the state information for the
 * notifier module.
 */

static struct {
    int timerActive;		/* 1 if timer is running. */
    Tcl_Time timer;		/* Time when next timer event is expected. */
    int flags;			/* OR'ed set of flags defined below. */
    Point lastMousePosition;	/* Last known mouse location. */
    RgnHandle utilityRgn;	/* Region used as the mouse region for
				 * WaitNextEvent and the update region when
				 * checking for events. */   
    Tcl_MacConvertEventPtr eventProcPtr;
				/* This pointer holds the address of the
				 * function that will handle all incoming
				 * Macintosh events. */
} notifier;

/*
 * The following defines are used in the flags field of the notifier struct.
 */

#define NOTIFY_IDLE	(1<<1)	/* Tcl_ServiceIdle should be called. */
#define NOTIFY_TIMER	(1<<2)	/* Tcl_ServiceTimer should be called. */

/*
 * Prototypes for procedures that are referenced only in this file:
 */

static int		HandleMacEvents _ANSI_ARGS_((void));
static void		InitNotifier _ANSI_ARGS_((void));
static void		NotifierExitHandler _ANSI_ARGS_((
			    ClientData clientData));

/*
 *----------------------------------------------------------------------
 *
 * InitNotifier --
 *
 *	Initializes the notifier structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a new exit handler.
 *
 *----------------------------------------------------------------------
 */

static void
InitNotifier(void)
{
    initialized = 1;
    memset(&notifier, 0, sizeof(notifier));
    Tcl_CreateExitHandler(NotifierExitHandler, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * NotifierExitHandler --
 *
 *	This function is called to cleanup the notifier state before
 *	Tcl is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
NotifierExitHandler(
    ClientData clientData)	/* Not used. */
{
    initialized = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * HandleMacEvents --
 *
 *	This function checks for events from the Macintosh event queue.
 *
 * Results:
 *	Returns 1 if event found, 0 otherwise.
 *
 * Side effects:
 *	Pulls events off of the Mac event queue and then calls
 *	convertEventProc.
 *
 *----------------------------------------------------------------------
 */

static int
HandleMacEvents(void)
{
    EventRecord theEvent;
    int eventFound = 0, needsUpdate = 0;
    Point currentMouse;
    WindowRef windowRef;
    Rect mouseRect;

    /*
     * Check for mouse moved events.  These events aren't placed on the
     * system event queue unless we call WaitNextEvent.
     */

    GetGlobalMouse(&currentMouse);
    if ((notifier.eventProcPtr != NULL) &&
	    !EqualPt(currentMouse, notifier.lastMousePosition)) {
	notifier.lastMousePosition = currentMouse;
	theEvent.what = nullEvent;
	if ((*notifier.eventProcPtr)(&theEvent) == true) {
	    eventFound = 1;
	}
    }

    /*
     * Check for update events.  Since update events aren't generated
     * until we call GetNextEvent, we may need to force a call to
     * GetNextEvent, even if the queue is empty.
     */

    for (windowRef = FrontWindow(); windowRef != NULL;
	    windowRef = GetNextWindow(windowRef)) {
	GetWindowUpdateRgn(windowRef, notifier.utilityRgn);
	if (!EmptyRgn(notifier.utilityRgn)) {
	    needsUpdate = 1;
	    break;
	}
    }
    
    /*
     * Process events from the OS event queue.
     */

    while (needsUpdate || (GetEvQHdr()->qHead != NULL)) {
	GetGlobalMouse(&currentMouse);
	SetRect(&mouseRect, currentMouse.h, currentMouse.v,
		currentMouse.h + 1, currentMouse.v + 1);
	RectRgn(notifier.utilityRgn, &mouseRect);
	
	WaitNextEvent(everyEvent, &theEvent, 5, notifier.utilityRgn);
	needsUpdate = 0;
	if ((notifier.eventProcPtr != NULL)
		&& ((*notifier.eventProcPtr)(&theEvent) == true)) {
	    eventFound = 1;
	}
    }
    
    return eventFound;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetTimer --
 *
 *	This procedure sets the current notifier timer value.  The
 *	notifier will ensure that Tcl_ServiceAll() is called after
 *	the specified interval, even if no events have occurred.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Replaces any previous timer.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetTimer(
    Tcl_Time *timePtr)		/* New value for interval timer. */
{
    if (!timePtr) {
	notifier.timerActive = 0;
    } else {
	/*
	 * Compute when the timer should fire.
	 */
	
	TclpGetTime(&notifier.timer);
	notifier.timer.sec += timePtr->sec;
	notifier.timer.usec += timePtr->usec;
	if (notifier.timer.usec >= 1000000) {
	    notifier.timer.usec -= 1000000;
	    notifier.timer.sec += 1;
	}
	notifier.timerActive = 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WaitForEvent --
 *
 *	This function is called by Tcl_DoOneEvent to wait for new
 *	events on the message queue.  If the block time is 0, then
 *	Tcl_WaitForEvent just polls the event queue without blocking.
 *
 * Results:
 *	Always returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WaitForEvent(
    Tcl_Time *timePtr)		/* Maximum block time. */
{
    int found;
    EventRecord macEvent;
    long sleepTime = 5;
    long ms;
    Point currentMouse;
    void * timerToken;
    Rect mouseRect;

    /*
     * Compute the next timeout value.
     */

    if (!timePtr) {
	ms = INT_MAX;
    } else {
	ms = (timePtr->sec * 1000) + (timePtr->usec / 1000);
    }
    timerToken = TclMacStartTimer((long) ms);
   
    /*
     * Poll the Mac event sources.  This loop repeats until something
     * happens: a timeout, a socket event, mouse motion, or some other
     * window event.  Note that we don't call WaitNextEvent if another
     * event is found to avoid context switches.  This effectively gives
     * events coming in via WaitNextEvent a slightly lower priority.
     */

    found = 0;
    if (notifier.utilityRgn == NULL) {
	notifier.utilityRgn = NewRgn();
    }

    while (!found) {
	/*
	 * Check for generated and queued events.
	 */

	if (HandleMacEvents()) {
	    found = 1;
	}

	/*
	 * Check for time out.
	 */

	if (!found && TclMacTimerExpired(timerToken)) {
	    found = 1;
	}
	
	/*
	 * Mod by Jack: poll for select() events. Code is in TclSelectNotify.c
	 */
	{
	    int Tcl_PollSelectEvent(void);
	    if (!found && Tcl_PollSelectEvent())
		found = 1;
	}

	/*
	 * Check for window events.  We may receive a NULL event for
	 * various reasons. 1) the timer has expired, 2) a mouse moved
	 * event is occuring or 3) the os is giving us time for idle
	 * events.  Note that we aren't sharing the processor very
	 * well here.  We really ought to do a better job of calling
	 * WaitNextEvent for time slicing purposes.
	 */

	if (!found) {
	    /*
	     * Set up mouse region so we will wake if the mouse is moved.
	     * We do this by defining the smallest possible region around
	     * the current mouse position.
	     */

	    GetGlobalMouse(&currentMouse);
	    SetRect(&mouseRect, currentMouse.h, currentMouse.v,
		    currentMouse.h + 1, currentMouse.v + 1);
	    RectRgn(notifier.utilityRgn, &mouseRect);
	
	    WaitNextEvent(everyEvent, &macEvent, sleepTime,
		    notifier.utilityRgn);

	    if (notifier.eventProcPtr != NULL) {
		if ((*notifier.eventProcPtr)(&macEvent) == true) {
		    found = 1;
		}
	    }
	}
    }
    TclMacRemoveTimer(timerToken);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Sleep --
 *
 *	Delay execution for the specified number of milliseconds.  This
 *	is not a very good call to make.  It will block the system -
 *	you will not even be able to switch applications.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Time passes.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Sleep(
    int ms)			/* Number of milliseconds to sleep. */
{
    EventRecord dummy;
    void *timerToken;
    
    if (ms <= 0) {
	return;
    }
    
    timerToken = TclMacStartTimer((long) ms);
    while (1) {
	WaitNextEvent(0, &dummy, (ms / 16.66) + 1, NULL);
	
	if (TclMacTimerExpired(timerToken)) {
	    break;
	}
    }
    TclMacRemoveTimer(timerToken);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MacSetEventProc --
 *
 *	This function sets the event handling procedure for the 
 *	application.  This function will be passed all incoming Mac
 *	events.  This function usually controls the console or some
 *	other entity like Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the event handling function.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_MacSetEventProc(
    Tcl_MacConvertEventPtr procPtr)
{
    notifier.eventProcPtr = procPtr;
}
