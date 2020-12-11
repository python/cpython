/*
 * tclNotify.c --
 *
 *	This file implements the generic portion of the Tcl notifier. The
 *	notifier is lowest-level part of the event system. It manages an event
 *	queue that holds Tcl_Event structures. The platform specific portion
 *	of the notifier is defined in the tcl*Notify.c files in each platform
 *	directory.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998 by Scriptics Corporation.
 * Copyright (c) 2003 by Kevin B. Kenny.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Module-scope struct of notifier hooks that are checked in the default
 * notifier functions (for overriding via Tcl_SetNotifier).
 */

Tcl_NotifierProcs tclNotifierHooks = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/*
 * For each event source (created with Tcl_CreateEventSource) there is a
 * structure of the following type:
 */

typedef struct EventSource {
    Tcl_EventSetupProc *setupProc;
    Tcl_EventCheckProc *checkProc;
    ClientData clientData;
    struct EventSource *nextPtr;
} EventSource;

/*
 * The following structure keeps track of the state of the notifier on a
 * per-thread basis. The first three elements keep track of the event queue.
 * In addition to the first (next to be serviced) and last events in the
 * queue, we keep track of a "marker" event. This provides a simple priority
 * mechanism whereby events can be inserted at the front of the queue but
 * behind all other high-priority events already in the queue (this is used
 * for things like a sequence of Enter and Leave events generated during a
 * grab in Tk). These elements are protected by the queueMutex so that any
 * thread can queue an event on any notifier. Note that all of the values in
 * this structure will be initialized to 0.
 */

typedef struct ThreadSpecificData {
    Tcl_Event *firstEventPtr;	/* First pending event, or NULL if none. */
    Tcl_Event *lastEventPtr;	/* Last pending event, or NULL if none. */
    Tcl_Event *markerEventPtr;	/* Last high-priority event in queue, or NULL
				 * if none. */
    Tcl_Mutex queueMutex;	/* Mutex to protect access to the previous
				 * three fields. */
    int serviceMode;		/* One of TCL_SERVICE_NONE or
				 * TCL_SERVICE_ALL. */
    int blockTimeSet;		/* 0 means there is no maximum block time:
				 * block forever. */
    Tcl_Time blockTime;		/* If blockTimeSet is 1, gives the maximum
				 * elapsed time for the next block. */
    int inTraversal;		/* 1 if Tcl_SetMaxBlockTime is being called
				 * during an event source traversal. */
    EventSource *firstEventSourcePtr;
				/* Pointer to first event source in list of
				 * event sources for this thread. */
    Tcl_ThreadId threadId;	/* Thread that owns this notifier instance. */
    ClientData clientData;	/* Opaque handle for platform specific
				 * notifier. */
    int initialized;		/* 1 if notifier has been initialized. */
    struct ThreadSpecificData *nextPtr;
				/* Next notifier in global list of notifiers.
				 * Access is controlled by the listLock global
				 * mutex. */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * Global list of notifiers. Access to this list is controlled by the listLock
 * mutex. If this becomes a performance bottleneck, this could be replaced
 * with a hashtable.
 */

static ThreadSpecificData *firstNotifierPtr = NULL;
TCL_DECLARE_MUTEX(listLock)

/*
 * Declarations for routines used only in this file.
 */

static void		QueueEvent(ThreadSpecificData *tsdPtr,
			    Tcl_Event *evPtr, Tcl_QueuePosition position);

/*
 *----------------------------------------------------------------------
 *
 * TclInitNotifier --
 *
 *	Initialize the thread local data structures for the notifier
 *	subsystem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds the current thread to the global list of notifiers.
 *
 *----------------------------------------------------------------------
 */

void
TclInitNotifier(void)
{
    ThreadSpecificData *tsdPtr;
    Tcl_ThreadId threadId = Tcl_GetCurrentThread();

    Tcl_MutexLock(&listLock);
    for (tsdPtr = firstNotifierPtr; tsdPtr && tsdPtr->threadId != threadId;
	    tsdPtr = tsdPtr->nextPtr) {
	/* Empty loop body. */
    }

    if (NULL == tsdPtr) {
	/*
	 * Notifier not yet initialized in this thread.
	 */

	tsdPtr = TCL_TSD_INIT(&dataKey);
	tsdPtr->threadId = threadId;
	tsdPtr->clientData = Tcl_InitNotifier();
	tsdPtr->initialized = 1;
	tsdPtr->nextPtr = firstNotifierPtr;
	firstNotifierPtr = tsdPtr;
    }
    Tcl_MutexUnlock(&listLock);
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeNotifier --
 *
 *	Finalize the thread local data structures for the notifier subsystem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the notifier associated with the current thread from the
 *	global notifier list. This is done only if the notifier was
 *	initialized for this thread by call to TclInitNotifier(). This is
 *	always true for threads which have been seeded with an Tcl
 *	interpreter, since the call to Tcl_CreateInterp will, among other
 *	things, call TclInitializeSubsystems() and this one will, in turn,
 *	call the TclInitNotifier() for the thread. For threads created without
 *	the Tcl interpreter, though, nobody is explicitly nor implicitly
 *	calling the TclInitNotifier hence, TclFinalizeNotifier should not be
 *	performed at all.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeNotifier(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ThreadSpecificData **prevPtrPtr;
    Tcl_Event *evPtr, *hold;

    if (!tsdPtr->initialized) {
	return;		/* Notifier not initialized for the current thread */
    }

    Tcl_MutexLock(&(tsdPtr->queueMutex));
    for (evPtr = tsdPtr->firstEventPtr; evPtr != NULL; ) {
	hold = evPtr;
	evPtr = evPtr->nextPtr;
	ckfree(hold);
    }
    tsdPtr->firstEventPtr = NULL;
    tsdPtr->lastEventPtr = NULL;
    Tcl_MutexUnlock(&(tsdPtr->queueMutex));

    Tcl_MutexLock(&listLock);

    Tcl_FinalizeNotifier(tsdPtr->clientData);
    Tcl_MutexFinalize(&(tsdPtr->queueMutex));
    for (prevPtrPtr = &firstNotifierPtr; *prevPtrPtr != NULL;
	    prevPtrPtr = &((*prevPtrPtr)->nextPtr)) {
	if (*prevPtrPtr == tsdPtr) {
	    *prevPtrPtr = tsdPtr->nextPtr;
	    break;
	}
    }
    tsdPtr->initialized = 0;

    Tcl_MutexUnlock(&listLock);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetNotifier --
 *
 *	Install a set of alternate functions for use with the notifier. In
 *	particular, this can be used to install the Xt-based notifier for use
 *	with the Browser plugin.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the tclNotifierHooks global, which is checked in the default
 *	notifier functions.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetNotifier(
    Tcl_NotifierProcs *notifierProcPtr)
{
    tclNotifierHooks = *notifierProcPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateEventSource --
 *
 *	This function is invoked to create a new source of events. The source
 *	is identified by a function that gets invoked during Tcl_DoOneEvent to
 *	check for events on that source and queue them.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	SetupProc and checkProc will be invoked each time that Tcl_DoOneEvent
 *	runs out of things to do. SetupProc will be invoked before
 *	Tcl_DoOneEvent calls select or whatever else it uses to wait for
 *	events. SetupProc typically calls functions like Tcl_SetMaxBlockTime
 *	to indicate what to wait for.
 *
 *	CheckProc is called after select or whatever operation was actually
 *	used to wait. It figures out whether anything interesting actually
 *	happened (e.g. by calling Tcl_AsyncReady), and then calls
 *	Tcl_QueueEvent to queue any events that are ready.
 *
 *	Each of these functions is passed two arguments, e.g.
 *		(*checkProc)(ClientData clientData, int flags));
 *	ClientData is the same as the clientData argument here, and flags is a
 *	combination of things like TCL_FILE_EVENTS that indicates what events
 *	are of interest: setupProc and checkProc use flags to figure out
 *	whether their events are relevant or not.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateEventSource(
    Tcl_EventSetupProc *setupProc,
				/* Function to invoke to figure out what to
				 * wait for. */
    Tcl_EventCheckProc *checkProc,
				/* Function to call after waiting to see what
				 * happened. */
    ClientData clientData)	/* One-word argument to pass to setupProc and
				 * checkProc. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    EventSource *sourcePtr = ckalloc(sizeof(EventSource));

    sourcePtr->setupProc = setupProc;
    sourcePtr->checkProc = checkProc;
    sourcePtr->clientData = clientData;
    sourcePtr->nextPtr = tsdPtr->firstEventSourcePtr;
    tsdPtr->firstEventSourcePtr = sourcePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteEventSource --
 *
 *	This function is invoked to delete the source of events given by proc
 *	and clientData.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given event source is canceled, so its function will never again
 *	be called. If no such source exists, nothing happens.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteEventSource(
    Tcl_EventSetupProc *setupProc,
				/* Function to invoke to figure out what to
				 * wait for. */
    Tcl_EventCheckProc *checkProc,
				/* Function to call after waiting to see what
				 * happened. */
    ClientData clientData)	/* One-word argument to pass to setupProc and
				 * checkProc. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    EventSource *sourcePtr, *prevPtr;

    for (sourcePtr = tsdPtr->firstEventSourcePtr, prevPtr = NULL;
	    sourcePtr != NULL;
	    prevPtr = sourcePtr, sourcePtr = sourcePtr->nextPtr) {
	if ((sourcePtr->setupProc != setupProc)
		|| (sourcePtr->checkProc != checkProc)
		|| (sourcePtr->clientData != clientData)) {
	    continue;
	}
	if (prevPtr == NULL) {
	    tsdPtr->firstEventSourcePtr = sourcePtr->nextPtr;
	} else {
	    prevPtr->nextPtr = sourcePtr->nextPtr;
	}
	ckfree(sourcePtr);
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_QueueEvent --
 *
 *	Queue an event on the event queue associated with the current thread.
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
Tcl_QueueEvent(
    Tcl_Event *evPtr,		/* Event to add to queue. The storage space
				 * must have been allocated the caller with
				 * malloc (ckalloc), and it becomes the
				 * property of the event queue. It will be
				 * freed after the event has been handled. */
    Tcl_QueuePosition position)	/* One of TCL_QUEUE_TAIL, TCL_QUEUE_HEAD,
				 * TCL_QUEUE_MARK. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    QueueEvent(tsdPtr, evPtr, position);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ThreadQueueEvent --
 *
 *	Queue an event on the specified thread's event queue.
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
Tcl_ThreadQueueEvent(
    Tcl_ThreadId threadId,	/* Identifier for thread to use. */
    Tcl_Event *evPtr,		/* Event to add to queue. The storage space
				 * must have been allocated the caller with
				 * malloc (ckalloc), and it becomes the
				 * property of the event queue. It will be
				 * freed after the event has been handled. */
    Tcl_QueuePosition position)	/* One of TCL_QUEUE_TAIL, TCL_QUEUE_HEAD,
				 * TCL_QUEUE_MARK. */
{
    ThreadSpecificData *tsdPtr;

    /*
     * Find the notifier associated with the specified thread.
     */

    Tcl_MutexLock(&listLock);
    for (tsdPtr = firstNotifierPtr; tsdPtr && tsdPtr->threadId != threadId;
	    tsdPtr = tsdPtr->nextPtr) {
	/* Empty loop body. */
    }

    /*
     * Queue the event if there was a notifier associated with the thread.
     */

    if (tsdPtr) {
	QueueEvent(tsdPtr, evPtr, position);
    } else {
	ckfree(evPtr);
    }
    Tcl_MutexUnlock(&listLock);
}

/*
 *----------------------------------------------------------------------
 *
 * QueueEvent --
 *
 *	Insert an event into the specified thread's event queue at one of
 *	three positions: the head, the tail, or before a floating marker.
 *	Events inserted before the marker will be processed in first-in-
 *	first-out order, but before any events inserted at the tail of the
 *	queue. Events inserted at the head of the queue will be processed in
 *	last-in-first-out order.
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
QueueEvent(
    ThreadSpecificData *tsdPtr,	/* Handle to thread local data that indicates
				 * which event queue to use. */
    Tcl_Event *evPtr,		/* Event to add to queue.  The storage space
				 * must have been allocated the caller with
				 * malloc (ckalloc), and it becomes the
				 * property of the event queue. It will be
				 * freed after the event has been handled. */
    Tcl_QueuePosition position)	/* One of TCL_QUEUE_TAIL, TCL_QUEUE_HEAD,
				 * TCL_QUEUE_MARK. */
{
    Tcl_MutexLock(&(tsdPtr->queueMutex));
    if (position == TCL_QUEUE_TAIL) {
	/*
	 * Append the event on the end of the queue.
	 */

	evPtr->nextPtr = NULL;
	if (tsdPtr->firstEventPtr == NULL) {
	    tsdPtr->firstEventPtr = evPtr;
	} else {
	    tsdPtr->lastEventPtr->nextPtr = evPtr;
	}
	tsdPtr->lastEventPtr = evPtr;
    } else if (position == TCL_QUEUE_HEAD) {
	/*
	 * Push the event on the head of the queue.
	 */

	evPtr->nextPtr = tsdPtr->firstEventPtr;
	if (tsdPtr->firstEventPtr == NULL) {
	    tsdPtr->lastEventPtr = evPtr;
	}
	tsdPtr->firstEventPtr = evPtr;
    } else if (position == TCL_QUEUE_MARK) {
	/*
	 * Insert the event after the current marker event and advance the
	 * marker to the new event.
	 */

	if (tsdPtr->markerEventPtr == NULL) {
	    evPtr->nextPtr = tsdPtr->firstEventPtr;
	    tsdPtr->firstEventPtr = evPtr;
	} else {
	    evPtr->nextPtr = tsdPtr->markerEventPtr->nextPtr;
	    tsdPtr->markerEventPtr->nextPtr = evPtr;
	}
	tsdPtr->markerEventPtr = evPtr;
	if (evPtr->nextPtr == NULL) {
	    tsdPtr->lastEventPtr = evPtr;
	}
    }
    Tcl_MutexUnlock(&(tsdPtr->queueMutex));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteEvents --
 *
 *	Calls a function for each event in the queue and deletes those for
 *	which the function returns 1. Events for which the function returns 0
 *	are left in the queue. Operates on the queue associated with the
 *	current thread.
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
Tcl_DeleteEvents(
    Tcl_EventDeleteProc *proc,	/* The function to call. */
    ClientData clientData)	/* The type-specific data. */
{
    Tcl_Event *evPtr;		/* Pointer to the event being examined */
    Tcl_Event *prevPtr;		/* Pointer to evPtr's predecessor, or NULL if
				 * evPtr designates the first event in the
				 * queue for the thread. */
    Tcl_Event *hold;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    Tcl_MutexLock(&(tsdPtr->queueMutex));

    /*
     * Walk the queue of events for the thread, applying 'proc' to each to
     * decide whether to eliminate the event.
     */

    prevPtr = NULL;
    evPtr = tsdPtr->firstEventPtr;
    while (evPtr != NULL) {
	if (proc(evPtr, clientData) == 1) {
	    /*
	     * This event should be deleted. Unlink it.
	     */

	    if (prevPtr == NULL) {
		tsdPtr->firstEventPtr = evPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = evPtr->nextPtr;
	    }

	    /*
	     * Update 'last' and 'marker' events if either has been deleted.
	     */

	    if (evPtr->nextPtr == NULL) {
		tsdPtr->lastEventPtr = prevPtr;
	    }
	    if (tsdPtr->markerEventPtr == evPtr) {
		tsdPtr->markerEventPtr = prevPtr;
	    }

	    /*
	     * Delete the event data structure.
	     */

	    hold = evPtr;
	    evPtr = evPtr->nextPtr;
	    ckfree(hold);
	} else {
	    /*
	     * Event is to be retained.
	     */

	    prevPtr = evPtr;
	    evPtr = evPtr->nextPtr;
	}
    }
    Tcl_MutexUnlock(&(tsdPtr->queueMutex));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ServiceEvent --
 *
 *	Process one event from the event queue, or invoke an asynchronous
 *	event handler. Operates on event queue for current thread.
 *
 * Results:
 *	The return value is 1 if the function actually found an event to
 *	process. If no processing occurred, then 0 is returned.
 *
 * Side effects:
 *	Invokes all of the event handlers for the highest priority event in
 *	the event queue. May collapse some events into a single event or
 *	discard stale events.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ServiceEvent(
    int flags)			/* Indicates what events should be processed.
				 * May be any combination of TCL_WINDOW_EVENTS
				 * TCL_FILE_EVENTS, TCL_TIMER_EVENTS, or other
				 * flags defined elsewhere. Events not
				 * matching this will be skipped for
				 * processing later. */
{
    Tcl_Event *evPtr, *prevPtr;
    Tcl_EventProc *proc;
    int result;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Asynchronous event handlers are considered to be the highest priority
     * events, and so must be invoked before we process events on the event
     * queue.
     */

    if (Tcl_AsyncReady()) {
	(void) Tcl_AsyncInvoke(NULL, 0);
	return 1;
    }

    /*
     * No event flags is equivalent to TCL_ALL_EVENTS.
     */

    if ((flags & TCL_ALL_EVENTS) == 0) {
	flags |= TCL_ALL_EVENTS;
    }

    /*
     * Loop through all the events in the queue until we find one that can
     * actually be handled.
     */

    Tcl_MutexLock(&(tsdPtr->queueMutex));
    for (evPtr = tsdPtr->firstEventPtr; evPtr != NULL;
	    evPtr = evPtr->nextPtr) {
	/*
	 * Call the handler for the event. If it actually handles the event
	 * then free the storage for the event. There are two tricky things
	 * here, both stemming from the fact that the event code may be
	 * re-entered while servicing the event:
	 *
	 * 1. Set the "proc" field to NULL.  This is a signal to ourselves
	 *    that we shouldn't reexecute the handler if the event loop is
	 *    re-entered.
	 * 2. When freeing the event, must search the queue again from the
	 *    front to find it. This is because the event queue could change
	 *    almost arbitrarily while handling the event, so we can't depend
	 *    on pointers found now still being valid when the handler
	 *    returns.
	 */

	proc = evPtr->proc;
	if (proc == NULL) {
	    continue;
	}
	evPtr->proc = NULL;

	/*
	 * Release the lock before calling the event function. This allows
	 * other threads to post events if we enter a recursive event loop in
	 * this thread. Note that we are making the assumption that if the
	 * proc returns 0, the event is still in the list.
	 */

	Tcl_MutexUnlock(&(tsdPtr->queueMutex));
	result = proc(evPtr, flags);
	Tcl_MutexLock(&(tsdPtr->queueMutex));

	if (result) {
	    /*
	     * The event was processed, so remove it from the queue.
	     */

	    if (tsdPtr->firstEventPtr == evPtr) {
		tsdPtr->firstEventPtr = evPtr->nextPtr;
		if (evPtr->nextPtr == NULL) {
		    tsdPtr->lastEventPtr = NULL;
		}
		if (tsdPtr->markerEventPtr == evPtr) {
		    tsdPtr->markerEventPtr = NULL;
		}
	    } else {
		for (prevPtr = tsdPtr->firstEventPtr;
			prevPtr && prevPtr->nextPtr != evPtr;
			prevPtr = prevPtr->nextPtr) {
		    /* Empty loop body. */
		}
		if (prevPtr) {
		    prevPtr->nextPtr = evPtr->nextPtr;
		    if (evPtr->nextPtr == NULL) {
			tsdPtr->lastEventPtr = prevPtr;
		    }
		    if (tsdPtr->markerEventPtr == evPtr) {
			tsdPtr->markerEventPtr = prevPtr;
		    }
		} else {
		    evPtr = NULL;
		}
	    }
	    if (evPtr) {
		ckfree(evPtr);
	    }
	    Tcl_MutexUnlock(&(tsdPtr->queueMutex));
	    return 1;
	} else {
	    /*
	     * The event wasn't actually handled, so we have to restore the
	     * proc field to allow the event to be attempted again.
	     */

	    evPtr->proc = proc;
	}
    }
    Tcl_MutexUnlock(&(tsdPtr->queueMutex));
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
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    return tsdPtr->serviceMode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetServiceMode --
 *
 *	This routine sets the current service mode of the tsdPtr->
 *
 * Results:
 *	Returns the previous service mode.
 *
 * Side effects:
 *	Invokes the notifier service mode hook function.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetServiceMode(
    int mode)			/* New service mode: TCL_SERVICE_ALL or
				 * TCL_SERVICE_NONE */
{
    int oldMode;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    oldMode = tsdPtr->serviceMode;
    tsdPtr->serviceMode = mode;
    Tcl_ServiceModeHook(mode);
    return oldMode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetMaxBlockTime --
 *
 *	This function is invoked by event sources to tell the notifier how
 *	long it may block the next time it blocks. The timePtr argument gives
 *	a maximum time; the actual time may be less if some other event source
 *	requested a smaller time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May reduce the length of the next sleep in the tsdPtr->
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetMaxBlockTime(
    const Tcl_Time *timePtr)		/* Specifies a maximum elapsed time for the
				 * next blocking operation in the event
				 * tsdPtr-> */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!tsdPtr->blockTimeSet || (timePtr->sec < tsdPtr->blockTime.sec)
	    || ((timePtr->sec == tsdPtr->blockTime.sec)
	    && (timePtr->usec < tsdPtr->blockTime.usec))) {
	tsdPtr->blockTime = *timePtr;
	tsdPtr->blockTimeSet = 1;
    }

    /*
     * If we are called outside an event source traversal, set the timeout
     * immediately.
     */

    if (!tsdPtr->inTraversal) {
	Tcl_SetTimer(&tsdPtr->blockTime);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DoOneEvent --
 *
 *	Process a single event of some sort. If there's no work to do, wait
 *	for an event to occur, then process it.
 *
 * Results:
 *	The return value is 1 if the function actually found an event to
 *	process. If no processing occurred, then 0 is returned (this can
 *	happen if the TCL_DONT_WAIT flag is set or if there are no event
 *	handlers to wait for in the set specified by flags).
 *
 * Side effects:
 *	May delay execution of process while waiting for an event, unless
 *	TCL_DONT_WAIT is set in the flags argument. Event sources are invoked
 *	to check for and queue events. Event handlers may produce arbitrary
 *	side effects.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_DoOneEvent(
    int flags)			/* Miscellaneous flag values: may be any
				 * combination of TCL_DONT_WAIT,
				 * TCL_WINDOW_EVENTS, TCL_FILE_EVENTS,
				 * TCL_TIMER_EVENTS, TCL_IDLE_EVENTS, or
				 * others defined by event sources. */
{
    int result = 0, oldMode;
    EventSource *sourcePtr;
    Tcl_Time *timePtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * The first thing we do is to service any asynchronous event handlers.
     */

    if (Tcl_AsyncReady()) {
	(void) Tcl_AsyncInvoke(NULL, 0);
	return 1;
    }

    /*
     * No event flags is equivalent to TCL_ALL_EVENTS.
     */

    if ((flags & TCL_ALL_EVENTS) == 0) {
	flags |= TCL_ALL_EVENTS;
    }

    /*
     * Set the service mode to none so notifier event routines won't try to
     * service events recursively.
     */

    oldMode = tsdPtr->serviceMode;
    tsdPtr->serviceMode = TCL_SERVICE_NONE;

    /*
     * The core of this function is an infinite loop, even though we only
     * service one event. The reason for this is that we may be processing
     * events that don't do anything inside of Tcl.
     */

    while (1) {
	/*
	 * If idle events are the only things to service, skip the main part
	 * of the loop and go directly to handle idle events (i.e. don't wait
	 * even if TCL_DONT_WAIT isn't set).
	 */

	if ((flags & TCL_ALL_EVENTS) == TCL_IDLE_EVENTS) {
	    flags = TCL_IDLE_EVENTS | TCL_DONT_WAIT;
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
	 * If TCL_DONT_WAIT is set, be sure to poll rather than blocking,
	 * otherwise reset the block time to infinity.
	 */

	if (flags & TCL_DONT_WAIT) {
	    tsdPtr->blockTime.sec = 0;
	    tsdPtr->blockTime.usec = 0;
	    tsdPtr->blockTimeSet = 1;
	} else {
	    tsdPtr->blockTimeSet = 0;
	}

	/*
	 * Set up all the event sources for new events. This will cause the
	 * block time to be updated if necessary.
	 */

	tsdPtr->inTraversal = 1;
	for (sourcePtr = tsdPtr->firstEventSourcePtr; sourcePtr != NULL;
		sourcePtr = sourcePtr->nextPtr) {
	    if (sourcePtr->setupProc) {
		sourcePtr->setupProc(sourcePtr->clientData, flags);
	    }
	}
	tsdPtr->inTraversal = 0;

	if ((flags & TCL_DONT_WAIT) || tsdPtr->blockTimeSet) {
	    timePtr = &tsdPtr->blockTime;
	} else {
	    timePtr = NULL;
	}

	/*
	 * Wait for a new event or a timeout. If Tcl_WaitForEvent returns -1,
	 * we should abort Tcl_DoOneEvent.
	 */

	result = Tcl_WaitForEvent(timePtr);
	if (result < 0) {
	    result = 0;
	    break;
	}

	/*
	 * Check all the event sources for new events.
	 */

	for (sourcePtr = tsdPtr->firstEventSourcePtr; sourcePtr != NULL;
		sourcePtr = sourcePtr->nextPtr) {
	    if (sourcePtr->checkProc) {
		sourcePtr->checkProc(sourcePtr->clientData, flags);
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
	 * We've tried everything at this point, but nobody we know about had
	 * anything to do. Check for idle events. If none, either quit or go
	 * back to the top and try again.
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

	/*
	 * If Tcl_WaitForEvent has returned 1, indicating that one system
	 * event has been dispatched (and thus that some Tcl code might have
	 * been indirectly executed), we break out of the loop. We do this to
	 * give VwaitCmd for instance a chance to check if that system event
	 * had the side effect of changing the variable (so the vwait can
	 * return and unwind properly).
	 *
	 * NB: We will process idle events if any first, because otherwise we
	 *     might never do the idle events if the notifier always gets
	 *     system events.
	 */

	if (result) {
	    break;
	}
    }

    tsdPtr->serviceMode = oldMode;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ServiceAll --
 *
 *	This routine checks all of the event sources, processes events that
 *	are on the Tcl event queue, and then calls the any idle handlers.
 *	Platform specific notifier callbacks that generate events should call
 *	this routine before returning to the system in order to ensure that
 *	Tcl gets a chance to process the new events.
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
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->serviceMode == TCL_SERVICE_NONE) {
	return result;
    }

    /*
     * We need to turn off event servicing like we to in Tcl_DoOneEvent, to
     * avoid recursive calls.
     */

    tsdPtr->serviceMode = TCL_SERVICE_NONE;

    /*
     * Check async handlers first.
     */

    if (Tcl_AsyncReady()) {
	(void) Tcl_AsyncInvoke(NULL, 0);
    }

    /*
     * Make a single pass through all event sources, queued events, and idle
     * handlers. Note that we wait to update the notifier timer until the end
     * so we can avoid multiple changes.
     */

    tsdPtr->inTraversal = 1;
    tsdPtr->blockTimeSet = 0;

    for (sourcePtr = tsdPtr->firstEventSourcePtr; sourcePtr != NULL;
	    sourcePtr = sourcePtr->nextPtr) {
	if (sourcePtr->setupProc) {
	    sourcePtr->setupProc(sourcePtr->clientData, TCL_ALL_EVENTS);
	}
    }
    for (sourcePtr = tsdPtr->firstEventSourcePtr; sourcePtr != NULL;
	    sourcePtr = sourcePtr->nextPtr) {
	if (sourcePtr->checkProc) {
	    sourcePtr->checkProc(sourcePtr->clientData, TCL_ALL_EVENTS);
	}
    }

    while (Tcl_ServiceEvent(0)) {
	result = 1;
    }
    if (TclServiceIdle()) {
	result = 1;
    }

    if (!tsdPtr->blockTimeSet) {
	Tcl_SetTimer(NULL);
    } else {
	Tcl_SetTimer(&tsdPtr->blockTime);
    }
    tsdPtr->inTraversal = 0;
    tsdPtr->serviceMode = TCL_SERVICE_ALL;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ThreadAlert --
 *
 *	This function wakes up the notifier associated with the specified
 *	thread (if there is one).
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
Tcl_ThreadAlert(
    Tcl_ThreadId threadId)	/* Identifier for thread to use. */
{
    ThreadSpecificData *tsdPtr;

    /*
     * Find the notifier associated with the specified thread. Note that we
     * need to hold the listLock while calling Tcl_AlertNotifier to avoid a
     * race condition where the specified thread might destroy its notifier.
     */

    Tcl_MutexLock(&listLock);
    for (tsdPtr = firstNotifierPtr; tsdPtr; tsdPtr = tsdPtr->nextPtr) {
	if (tsdPtr->threadId == threadId) {
	    Tcl_AlertNotifier(tsdPtr->clientData);
	    break;
	}
    }
    Tcl_MutexUnlock(&listLock);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
