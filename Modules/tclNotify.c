/*
 * This is a modified version of tclNotify.c from Sun's Tcl 8.0
 * distribution.  The purpose of the modification is to provide an
 * interface to the internals of the notifier that make it possible to
 * write safe multi-threaded Python programs that use Tkinter.
 *
 * Original comments follow.  The file license.terms from the Tcl 8.0
 * distribution is contained in this directory, as required.
 */

/* 
 * tclNotify.c --
 *
 *	This file implements the generic portion of the Tcl notifier.
 *	The notifier is lowest-level part of the event system.  It
 *	manages an event queue that holds Tcl_Event structures.  The
 *	platform specific portion of the notifier is defined in the
 *	tcl*Notify.c files in each platform directory.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tclNotify.c 1.15 97/06/18 17:14:04
 */

#include "tclInt.h"
#include "tclPort.h"

/*
 * The following static indicates whether this module has been initialized.
 */

static int initialized = 0;

/*
 * For each event source (created with Tcl_CreateEventSource) there
 * is a structure of the following type:
 */

typedef struct EventSource {
    Tcl_EventSetupProc *setupProc;
    Tcl_EventCheckProc *checkProc;
    ClientData clientData;
    struct EventSource *nextPtr;
} EventSource;

/*
 * The following structure keeps track of the state of the notifier.
 * The first three elements keep track of the event queue.  In addition to
 * the first (next to be serviced) and last events in the queue, we keep
 * track of a "marker" event.  This provides a simple priority mechanism
 * whereby events can be inserted at the front of the queue but behind all
 * other high-priority events already in the queue (this is used for things
 * like a sequence of Enter and Leave events generated during a grab in
 * Tk).
 */

static struct {
    Tcl_Event *firstEventPtr;	/* First pending event, or NULL if none. */
    Tcl_Event *lastEventPtr;	/* Last pending event, or NULL if none. */
    Tcl_Event *markerEventPtr;	/* Last high-priority event in queue, or
				 * NULL if none. */
    int serviceMode;		/* One of TCL_SERVICE_NONE or
				 * TCL_SERVICE_ALL. */
    int blockTimeSet;		/* 0 means there is no maximum block
				 * time:  block forever. */
    Tcl_Time blockTime;		/* If blockTimeSet is 1, gives the
				 * maximum elapsed time for the next block. */
    int inTraversal;		/* 1 if Tcl_SetMaxBlockTime is being
				 * called during an event source traversal. */
    EventSource *firstEventSourcePtr;
				/* Pointer to first event source in
				 * global list of event sources. */
} notifier;

/*
 * Declarations for functions used in this file.
 */

static void	InitNotifier _ANSI_ARGS_((void));
static void	NotifierExitHandler _ANSI_ARGS_((ClientData clientData));


/*
 *----------------------------------------------------------------------
 *
 * InitNotifier --
 *
 *	This routine is called to initialize the notifier module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates an exit handler and initializes static data.
 *
 *----------------------------------------------------------------------
 */

static void
InitNotifier(void)
{
    initialized = 1;
    memset(&notifier, 0, sizeof(notifier));
    notifier.serviceMode = TCL_SERVICE_NONE;
    Tcl_CreateExitHandler(NotifierExitHandler, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * NotifierExitHandler --
 *
 *	This routine is called during Tcl finalization.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the notifier initialization flag.
 *
 *----------------------------------------------------------------------
 */

static void
NotifierExitHandler(clientData)
    ClientData clientData;  /* Not used. */
{
    initialized = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateEventSource --
 *
 *	This procedure is invoked to create a new source of events.
 *	The source is identified by a procedure that gets invoked
 *	during Tcl_DoOneEvent to check for events on that source
 *	and queue them.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	SetupProc and checkProc will be invoked each time that Tcl_DoOneEvent
 *	runs out of things to do.  SetupProc will be invoked before
 *	Tcl_DoOneEvent calls select or whatever else it uses to wait
 *	for events.  SetupProc typically calls functions like Tcl_WatchFile
 *	or Tcl_SetMaxBlockTime to indicate what to wait for.
 *
 *	CheckProc is called after select or whatever operation was actually
 *	used to wait.  It figures out whether anything interesting actually
 *	happened (e.g. by calling Tcl_FileReady), and then calls
 *	Tcl_QueueEvent to queue any events that are ready.
 *
 *	Each of these procedures is passed two arguments, e.g.
 *		(*checkProc)(ClientData clientData, int flags));
 *	ClientData is the same as the clientData argument here, and flags
 *	is a combination of things like TCL_FILE_EVENTS that indicates
 *	what events are of interest:  setupProc and checkProc use flags
 *	to figure out whether their events are relevant or not.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateEventSource(setupProc, checkProc, clientData)
    Tcl_EventSetupProc *setupProc;	/* Procedure to invoke to figure out
					 * what to wait for. */
    Tcl_EventCheckProc *checkProc;	/* Procedure to call after waiting
					 * to see what happened. */
    ClientData clientData;		/* One-word argument to pass to
					 * setupProc and checkProc. */
{
    EventSource *sourcePtr;

    if (!initialized) {
	InitNotifier();
    }

    sourcePtr = (EventSource *) ckalloc(sizeof(EventSource));
    sourcePtr->setupProc = setupProc;
    sourcePtr->checkProc = checkProc;
    sourcePtr->clientData = clientData;
    sourcePtr->nextPtr = notifier.firstEventSourcePtr;
    notifier.firstEventSourcePtr = sourcePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteEventSource --
 *
 *	This procedure is invoked to delete the source of events
 *	given by proc and clientData.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given event source is canceled, so its procedure will
 *	never again be called.  If no such source exists, nothing
 *	happens.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteEventSource(setupProc, checkProc, clientData)
    Tcl_EventSetupProc *setupProc;	/* Procedure to invoke to figure out
					 * what to wait for. */
    Tcl_EventCheckProc *checkProc;	/* Procedure to call after waiting
					 * to see what happened. */
    ClientData clientData;		/* One-word argument to pass to
					 * setupProc and checkProc. */
{
    EventSource *sourcePtr, *prevPtr;

    for (sourcePtr = notifier.firstEventSourcePtr, prevPtr = NULL;
	    sourcePtr != NULL;
	    prevPtr = sourcePtr, sourcePtr = sourcePtr->nextPtr) {
	if ((sourcePtr->setupProc != setupProc)
		|| (sourcePtr->checkProc != checkProc)
		|| (sourcePtr->clientData != clientData)) {
	    continue;
	}
	if (prevPtr == NULL) {
	    notifier.firstEventSourcePtr = sourcePtr->nextPtr;
	} else {
	    prevPtr->nextPtr = sourcePtr->nextPtr;
	}
	ckfree((char *) sourcePtr);
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_QueueEvent --
 *
 *	Insert an event into the Tk event queue at one of three
 *	positions: the head, the tail, or before a floating marker.
 *	Events inserted before the marker will be processed in
 *	first-in-first-out order, but before any events inserted at
 *	the tail of the queue.  Events inserted at the head of the
 *	queue will be processed in last-in-first-out order.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_QueueEvent(evPtr, position)
    Tcl_Event* evPtr;		/* Event to add to queue.  The storage
				 * space must have been allocated the caller
				 * with malloc (ckalloc), and it becomes
				 * the property of the event queue.  It
				 * will be freed after the event has been
				 * handled. */
    Tcl_QueuePosition position;	/* One of TCL_QUEUE_TAIL, TCL_QUEUE_HEAD,
				 * TCL_QUEUE_MARK. */
{
    if (!initialized) {
	InitNotifier();
    }

    if (position == TCL_QUEUE_TAIL) {
	/*
	 * Append the event on the end of the queue.
	 */

	evPtr->nextPtr = NULL;
	if (notifier.firstEventPtr == NULL) {
	    notifier.firstEventPtr = evPtr;
	} else {
	    notifier.lastEventPtr->nextPtr = evPtr;
	}
	notifier.lastEventPtr = evPtr;
    } else if (position == TCL_QUEUE_HEAD) {
	/*
	 * Push the event on the head of the queue.
	 */

	evPtr->nextPtr = notifier.firstEventPtr;
	if (notifier.firstEventPtr == NULL) {
	    notifier.lastEventPtr = evPtr;
	}	    
	notifier.firstEventPtr = evPtr;
    } else if (position == TCL_QUEUE_MARK) {
	/*
	 * Insert the event after the current marker event and advance
	 * the marker to the new event.
	 */

	if (notifier.markerEventPtr == NULL) {
	    evPtr->nextPtr = notifier.firstEventPtr;
	    notifier.firstEventPtr = evPtr;
	} else {
	    evPtr->nextPtr = notifier.markerEventPtr->nextPtr;
	    notifier.markerEventPtr->nextPtr = evPtr;
	}
	notifier.markerEventPtr = evPtr;
	if (evPtr->nextPtr == NULL) {
	    notifier.lastEventPtr = evPtr;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteEvents --
 *
 *	Calls a procedure for each event in the queue and deletes those
 *	for which the procedure returns 1. Events for which the
 *	procedure returns 0 are left in the queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Potentially removes one or more events from the event queue.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteEvents(proc, clientData)
    Tcl_EventDeleteProc *proc;		/* The procedure to call. */
    ClientData clientData;    		/* type-specific data. */
{
    Tcl_Event *evPtr, *prevPtr, *hold;

    if (!initialized) {
	InitNotifier();
    }

    for (prevPtr = (Tcl_Event *) NULL, evPtr = notifier.firstEventPtr;
             evPtr != (Tcl_Event *) NULL;
             ) {
        if ((*proc) (evPtr, clientData) == 1) {
            if (notifier.firstEventPtr == evPtr) {
                notifier.firstEventPtr = evPtr->nextPtr;
                if (evPtr->nextPtr == (Tcl_Event *) NULL) {
                    notifier.lastEventPtr = (Tcl_Event *) NULL;
                }
            } else {
                prevPtr->nextPtr = evPtr->nextPtr;
            }
            hold = evPtr;
            evPtr = evPtr->nextPtr;
            ckfree((char *) hold);
        } else {
            prevPtr = evPtr;
            evPtr = evPtr->nextPtr;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ServiceEvent --
 *
 *	Process one event from the event queue, or invoke an
 *	asynchronous event handler.
 *
 * Results:
 *	The return value is 1 if the procedure actually found an event
 *	to process.  If no processing occurred, then 0 is returned.
 *
 * Side effects:
 *	Invokes all of the event handlers for the highest priority
 *	event in the event queue.  May collapse some events into a
 *	single event or discard stale events.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ServiceEvent(flags)
    int flags;			/* Indicates what events should be processed.
				 * May be any combination of TCL_WINDOW_EVENTS
				 * TCL_FILE_EVENTS, TCL_TIMER_EVENTS, or other
				 * flags defined elsewhere.  Events not
				 * matching this will be skipped for processing
				 * later. */
{
    Tcl_Event *evPtr, *prevPtr;
    Tcl_EventProc *proc;

    if (!initialized) {
	InitNotifier();
    }

    /*
     * Asynchronous event handlers are considered to be the highest
     * priority events, and so must be invoked before we process events
     * on the event queue.
     */
    
    if (Tcl_AsyncReady()) {
	(void) Tcl_AsyncInvoke((Tcl_Interp *) NULL, 0);
	return 1;
    }

    /*
     * No event flags is equivalent to TCL_ALL_EVENTS.
     */
    
    if ((flags & TCL_ALL_EVENTS) == 0) {
	flags |= TCL_ALL_EVENTS;
    }

    /*
     * Loop through all the events in the queue until we find one
     * that can actually be handled.
     */

    for (evPtr = notifier.firstEventPtr; evPtr != NULL;
	 evPtr = evPtr->nextPtr) {
	/*
	 * Call the handler for the event.  If it actually handles the
	 * event then free the storage for the event.  There are two
	 * tricky things here, but stemming from the fact that the event
	 * code may be re-entered while servicing the event:
	 *
	 * 1. Set the "proc" field to NULL.  This is a signal to ourselves
	 *    that we shouldn't reexecute the handler if the event loop
	 *    is re-entered.
	 * 2. When freeing the event, must search the queue again from the
	 *    front to find it.  This is because the event queue could
	 *    change almost arbitrarily while handling the event, so we
	 *    can't depend on pointers found now still being valid when
	 *    the handler returns.
	 */

	proc = evPtr->proc;
	evPtr->proc = NULL;
	if ((proc != NULL) && (*proc)(evPtr, flags)) {
	    if (notifier.firstEventPtr == evPtr) {
		notifier.firstEventPtr = evPtr->nextPtr;
		if (evPtr->nextPtr == NULL) {
		    notifier.lastEventPtr = NULL;
		}
		if (notifier.markerEventPtr == evPtr) {
		    notifier.markerEventPtr = NULL;
		}
	    } else {
		for (prevPtr = notifier.firstEventPtr;
		     prevPtr->nextPtr != evPtr; prevPtr = prevPtr->nextPtr) {
		    /* Empty loop body. */
		}
		prevPtr->nextPtr = evPtr->nextPtr;
		if (evPtr->nextPtr == NULL) {
		    notifier.lastEventPtr = prevPtr;
		}
		if (notifier.markerEventPtr == evPtr) {
		    notifier.markerEventPtr = prevPtr;
		}
	    }
	    ckfree((char *) evPtr);
	    return 1;
	} else {
	    /*
	     * The event wasn't actually handled, so we have to restore
	     * the proc field to allow the event to be attempted again.
	     */

	    evPtr->proc = proc;
	}

	/*
	 * The handler for this event asked to defer it.  Just go on to
	 * the next event.
	 */

	continue;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetServiceMode --
 *
 *	This routine returns the current service mode of the notifier.
 *
 * Results:
 *	Returns either TCL_SERVICE_ALL or TCL_SERVICE_NONE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetServiceMode(void)
{
    if (!initialized) {
	InitNotifier();
    }

    return notifier.serviceMode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetServiceMode --
 *
 *	This routine sets the current service mode of the notifier.
 *
 * Results:
 *	Returns the previous service mode.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetServiceMode(mode)
    int mode;			/* New service mode: TCL_SERVICE_ALL or
				 * TCL_SERVICE_NONE */
{
    int oldMode;

    if (!initialized) {
	InitNotifier();
    }

    oldMode = notifier.serviceMode;
    notifier.serviceMode = mode;
    return oldMode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetMaxBlockTime --
 *
 *	This procedure is invoked by event sources to tell the notifier
 *	how long it may block the next time it blocks.  The timePtr
 *	argument gives a maximum time;  the actual time may be less if
 *	some other event source requested a smaller time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May reduce the length of the next sleep in the notifier.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetMaxBlockTime(timePtr)
    Tcl_Time *timePtr;		/* Specifies a maximum elapsed time for
				 * the next blocking operation in the
				 * event notifier. */
{
    if (!initialized) {
	InitNotifier();
    }

    if (!notifier.blockTimeSet || (timePtr->sec < notifier.blockTime.sec)
	    || ((timePtr->sec == notifier.blockTime.sec)
	    && (timePtr->usec < notifier.blockTime.usec))) {
	notifier.blockTime = *timePtr;
	notifier.blockTimeSet = 1;
    }

    /*
     * If we are called outside an event source traversal, set the
     * timeout immediately.
     */

    if (!notifier.inTraversal) {
	if (notifier.blockTimeSet) {
	    Tcl_SetTimer(&notifier.blockTime);
	} else {
	    Tcl_SetTimer(NULL);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DoOneEvent --
 *
 *	Process a single event of some sort.  If there's no work to
 *	do, wait for an event to occur, then process it.
 *
 * Results:
 *	The return value is 1 if the procedure actually found an event
 *	to process.  If no processing occurred, then 0 is returned (this
 *	can happen if the TCL_DONT_WAIT flag is set or if there are no
 *	event handlers to wait for in the set specified by flags).
 *
 * Side effects:
 *	May delay execution of process while waiting for an event,
 *	unless TCL_DONT_WAIT is set in the flags argument.  Event
 *	sources are invoked to check for and queue events.  Event
 *	handlers may produce arbitrary side effects.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_DoOneEvent(flags)
    int flags;			/* Miscellaneous flag values:  may be any
				 * combination of TCL_DONT_WAIT,
				 * TCL_WINDOW_EVENTS, TCL_FILE_EVENTS,
				 * TCL_TIMER_EVENTS, TCL_IDLE_EVENTS, or
				 * others defined by event sources. */
{
    int result = 0, oldMode;
    EventSource *sourcePtr;
    Tcl_Time *timePtr;

    if (!initialized) {
	InitNotifier();
    }

    /*
     * The first thing we do is to service any asynchronous event
     * handlers.
     */
    
    if (Tcl_AsyncReady()) {
	(void) Tcl_AsyncInvoke((Tcl_Interp *) NULL, 0);
	return 1;
    }

    /*
     * No event flags is equivalent to TCL_ALL_EVENTS.
     */
    
    if ((flags & TCL_ALL_EVENTS) == 0) {
	flags |= TCL_ALL_EVENTS;
    }

    /*
     * Set the service mode to none so notifier event routines won't
     * try to service events recursively.
     */

    oldMode = notifier.serviceMode;
    notifier.serviceMode = TCL_SERVICE_NONE;

    /*
     * The core of this procedure is an infinite loop, even though
     * we only service one event.  The reason for this is that we
     * may be processing events that don't do anything inside of Tcl.
     */

    while (1) {

	/*
	 * If idle events are the only things to service, skip the
	 * main part of the loop and go directly to handle idle
	 * events (i.e. don't wait even if TCL_DONT_WAIT isn't set).
	 */

	if ((flags & TCL_ALL_EVENTS) == TCL_IDLE_EVENTS) {
	    flags = TCL_IDLE_EVENTS|TCL_DONT_WAIT;
	    goto idleEvents;
	}

	/*
	 * Ask Tcl to service a queued event, if there are any.
	 */

	if (Tcl_ServiceEvent(flags)) {
	    result = 1;	    
	    break;
	}

	/*
	 * If TCL_DONT_WAIT is set, be sure to poll rather than
	 * blocking, otherwise reset the block time to infinity.
	 */

	if (flags & TCL_DONT_WAIT) {
	    notifier.blockTime.sec = 0;
	    notifier.blockTime.usec = 0;
	    notifier.blockTimeSet = 1;
	} else {
	    notifier.blockTimeSet = 0;
	}

	/*
	 * Set up all the event sources for new events.  This will
	 * cause the block time to be updated if necessary.
	 */

	notifier.inTraversal = 1;
	for (sourcePtr = notifier.firstEventSourcePtr; sourcePtr != NULL;
	     sourcePtr = sourcePtr->nextPtr) {
	    if (sourcePtr->setupProc) {
		(sourcePtr->setupProc)(sourcePtr->clientData, flags);
	    }
	}
	notifier.inTraversal = 0;

	if ((flags & TCL_DONT_WAIT) || notifier.blockTimeSet) {
	    timePtr = &notifier.blockTime;
	} else {
	    timePtr = NULL;
	}

	/*
	 * Wait for a new event or a timeout.  If Tcl_WaitForEvent
	 * returns -1, we should abort Tcl_DoOneEvent.
	 */

	result = Tcl_WaitForEvent(timePtr);
	if (result < 0) {
	    result = 0;
	    break;
	}

	/*
	 * Check all the event sources for new events.
	 */

	for (sourcePtr = notifier.firstEventSourcePtr; sourcePtr != NULL;
	     sourcePtr = sourcePtr->nextPtr) {
	    if (sourcePtr->checkProc) {
		(sourcePtr->checkProc)(sourcePtr->clientData, flags);
	    }
	}

	/*
	 * Check for events queued by the notifier or event sources.
	 */

	if (Tcl_ServiceEvent(flags)) {
	    result = 1;
	    break;
	}

	/*
	 * We've tried everything at this point, but nobody we know
	 * about had anything to do.  Check for idle events.  If none,
	 * either quit or go back to the top and try again.
	 */

	idleEvents:
	if (flags & TCL_IDLE_EVENTS) {
	    if (TclServiceIdle()) {
		result = 1;
		break;
	    }
	}
	if (flags & TCL_DONT_WAIT) {
	    break;
	}
    }

    notifier.serviceMode = oldMode;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ServiceAll --
 *
 *	This routine checks all of the event sources, processes
 *	events that are on the Tcl event queue, and then calls the
 *	any idle handlers.  Platform specific notifier callbacks that
 *	generate events should call this routine before returning to
 *	the system in order to ensure that Tcl gets a chance to
 *	process the new events.
 *
 * Results:
 *	Returns 1 if an event or idle handler was invoked, else 0.
 *
 * Side effects:
 *	Anything that an event or idle handler may do.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ServiceAll(void)
{
    int result = 0;
    EventSource *sourcePtr;

    if (!initialized) {
	InitNotifier();
    }

    if (notifier.serviceMode == TCL_SERVICE_NONE) {
	return result;
    }

    /*
     * We need to turn off event servicing like we to in Tcl_DoOneEvent,
     * to avoid recursive calls.
     */
    
    notifier.serviceMode = TCL_SERVICE_NONE;

    /*
     * Check async handlers first.
     */

    if (Tcl_AsyncReady()) {
	(void) Tcl_AsyncInvoke((Tcl_Interp *) NULL, 0);
    }

    /*
     * Make a single pass through all event sources, queued events,
     * and idle handlers.  Note that we wait to update the notifier
     * timer until the end so we can avoid multiple changes.
     */

    notifier.inTraversal = 1;
    notifier.blockTimeSet = 0;

    for (sourcePtr = notifier.firstEventSourcePtr; sourcePtr != NULL;
	 sourcePtr = sourcePtr->nextPtr) {
	if (sourcePtr->setupProc) {
	    (sourcePtr->setupProc)(sourcePtr->clientData, TCL_ALL_EVENTS);
	}
    }
    for (sourcePtr = notifier.firstEventSourcePtr; sourcePtr != NULL;
	 sourcePtr = sourcePtr->nextPtr) {
	if (sourcePtr->checkProc) {
	    (sourcePtr->checkProc)(sourcePtr->clientData, TCL_ALL_EVENTS);
	}
    }

    while (Tcl_ServiceEvent(0)) {
	result = 1;
    }
    if (TclServiceIdle()) {
	result = 1;
    }

    if (!notifier.blockTimeSet) {
	Tcl_SetTimer(NULL);
    } else {
	Tcl_SetTimer(&notifier.blockTime);
    }
    notifier.inTraversal = 0;
    notifier.serviceMode = TCL_SERVICE_ALL;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * PyTcl_WaitUntilEvent --
 *
 *	New function to wait until a Tcl event is ready without
 *	actually handling the event.  This is different than
 *	TclWaitForEvent(): that function doesn't call the event
 *	check routines, which is necessary for our purpose.
 *	We also can't use Tcl_DoOneEvent(TCL_DONT_WAIT), since that
 *	does too much: it handles the event.  We want the *handling*
 *	of the event to be done with the Python lock held, but the
 *	*waiting* with the lock released.
 *
 *	Since the event administration is not exported, our only
 *	choice is to use a modified copy of the file tclNotify.c,
 *	containing this additional function that makes the desired
 *	functionality available.  It is mostly a stripped down version
 *	of the code in Tcl_DoOneEvent().
 *
 *	This requires that you link with a static version of the Tcl
 *	library.  On Windows/Mac, a custom compilation of Tcl may be
 *	required (I haven't tried this yet).
 *
 *----------------------------------------------------------------------
 */

int
PyTcl_WaitUntilEvent(void)
{
    int flags = TCL_ALL_EVENTS;
    int result = 0, oldMode;
    EventSource *sourcePtr;
    Tcl_Time *timePtr;

    if (!initialized) {
	InitNotifier();
    }

    /*
     * The first thing we do is to service any asynchronous event
     * handlers.
     */
    
    if (Tcl_AsyncReady())
	return 1;

    /*
     * Set the service mode to none so notifier event routines won't
     * try to service events recursively.
     */

    oldMode = notifier.serviceMode;
    notifier.serviceMode = TCL_SERVICE_NONE;

    notifier.blockTimeSet = 0;

    /*
     * Set up all the event sources for new events.  This will
     * cause the block time to be updated if necessary.
     */

    notifier.inTraversal = 1;
    for (sourcePtr = notifier.firstEventSourcePtr; sourcePtr != NULL;
	 sourcePtr = sourcePtr->nextPtr) {
	if (sourcePtr->setupProc) {
	    (sourcePtr->setupProc)(sourcePtr->clientData, flags);
	}
    }
    notifier.inTraversal = 0;

    timePtr = NULL;

    /*
     * Wait for a new event or a timeout.  If Tcl_WaitForEvent
     * returns -1, we should abort Tcl_DoOneEvent.
     */

    result = Tcl_WaitForEvent(timePtr);
    if (result < 0)
	return 0;

    /*
     * Check all the event sources for new events.
     */

    for (sourcePtr = notifier.firstEventSourcePtr; sourcePtr != NULL;
	 sourcePtr = sourcePtr->nextPtr) {
	if (sourcePtr->checkProc) {
	    (sourcePtr->checkProc)(sourcePtr->clientData, flags);
	}
    }

    notifier.serviceMode = oldMode;
    return result;
}
