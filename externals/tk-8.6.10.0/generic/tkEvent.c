/*
 * tkEvent.c --
 *
 *	This file provides basic low-level facilities for managing X events in
 *	Tk.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Ajuba Solutions.
 * Copyright (c) 2004 George Peter Staplin
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * There's a potential problem if a handler is deleted while it's current
 * (i.e. its function is executing), since Tk_HandleEvent will need to read
 * the handler's "nextPtr" field when the function returns. To handle this
 * problem, structures of the type below indicate the next handler to be
 * processed for any (recursively nested) dispatches in progress. The
 * nextHandler fields get updated if the handlers pointed to are deleted.
 * Tk_HandleEvent also needs to know if the entire window gets deleted; the
 * winPtr field is set to zero if that particular window gets deleted.
 */

typedef struct InProgress {
    XEvent *eventPtr;		/* Event currently being handled. */
    TkWindow *winPtr;		/* Window for event. Gets set to None if
				 * window is deleted while event is being
				 * handled. */
    TkEventHandler *nextHandler;/* Next handler in search. */
    struct InProgress *nextPtr;	/* Next higher nested search. */
} InProgress;

/*
 * For each call to Tk_CreateGenericHandler or Tk_CreateClientMessageHandler,
 * an instance of the following structure will be created. All of the active
 * handlers are linked into a list.
 */

typedef struct GenericHandler {
    Tk_GenericProc *proc;	/* Function to dispatch on all X events. */
    ClientData clientData;	/* Client data to pass to function. */
    int deleteFlag;		/* Flag to set when this handler is
				 * deleted. */
    struct GenericHandler *nextPtr;
				/* Next handler in list of all generic
				 * handlers, or NULL for end of list. */
} GenericHandler;

/*
 * There's a potential problem if Tk_HandleEvent is entered recursively. A
 * handler cannot be deleted physically until we have returned from calling
 * it. Otherwise, we're looking at unallocated memory in advancing to its
 * `next' entry. We deal with the problem by using the `delete flag' and
 * deleting handlers only when it's known that there's no handler active.
 */

/*
 * The following structure is used for queueing X-style events on the Tcl
 * event queue.
 */

typedef struct TkWindowEvent {
    Tcl_Event header;		/* Standard information for all events. */
    XEvent event;		/* The X event. */
} TkWindowEvent;

/*
 * Array of event masks corresponding to each X event:
 */

static const unsigned long realEventMasks[MappingNotify+1] = {
    0,
    0,
    KeyPressMask,			/* KeyPress */
    KeyReleaseMask,			/* KeyRelease */
    ButtonPressMask,			/* ButtonPress */
    ButtonReleaseMask,			/* ButtonRelease */
    PointerMotionMask|PointerMotionHintMask|ButtonMotionMask
	    |Button1MotionMask|Button2MotionMask|Button3MotionMask
	    |Button4MotionMask|Button5MotionMask,
					/* MotionNotify */
    EnterWindowMask,			/* EnterNotify */
    LeaveWindowMask,			/* LeaveNotify */
    FocusChangeMask,			/* FocusIn */
    FocusChangeMask,			/* FocusOut */
    KeymapStateMask,			/* KeymapNotify */
    ExposureMask,			/* Expose */
    ExposureMask,			/* GraphicsExpose */
    ExposureMask,			/* NoExpose */
    VisibilityChangeMask,		/* VisibilityNotify */
    SubstructureNotifyMask,		/* CreateNotify */
    StructureNotifyMask,		/* DestroyNotify */
    StructureNotifyMask,		/* UnmapNotify */
    StructureNotifyMask,		/* MapNotify */
    SubstructureRedirectMask,		/* MapRequest */
    StructureNotifyMask,		/* ReparentNotify */
    StructureNotifyMask,		/* ConfigureNotify */
    SubstructureRedirectMask,		/* ConfigureRequest */
    StructureNotifyMask,		/* GravityNotify */
    ResizeRedirectMask,			/* ResizeRequest */
    StructureNotifyMask,		/* CirculateNotify */
    SubstructureRedirectMask,		/* CirculateRequest */
    PropertyChangeMask,			/* PropertyNotify */
    0,					/* SelectionClear */
    0,					/* SelectionRequest */
    0,					/* SelectionNotify */
    ColormapChangeMask,			/* ColormapNotify */
    0,					/* ClientMessage */
    0					/* Mapping Notify */
};

static const unsigned long virtualEventMasks[TK_LASTEVENT-VirtualEvent] = {
    VirtualEventMask,			/* VirtualEvents */
    ActivateMask,			/* ActivateNotify */
    ActivateMask,			/* DeactivateNotify */
    MouseWheelMask			/* MouseWheelEvent */
};

/*
 * For each exit handler created with a call to TkCreateExitHandler or
 * TkCreateThreadExitHandler there is a structure of the following type:
 */

typedef struct ExitHandler {
    Tcl_ExitProc *proc;		/* Function to call when process exits. */
    ClientData clientData;	/* One word of information to pass to proc. */
    struct ExitHandler *nextPtr;/* Next in list of all exit handlers for this
				 * application, or NULL for end of list. */
} ExitHandler;

/*
 * The structure below is used to store Data for the Event module that must be
 * kept thread-local. The "dataKey" is used to fetch the thread-specific
 * storage for the current thread.
 */

typedef struct {
    int handlersActive;		/* The following variable has a non-zero value
				 * when a handler is active. */
    InProgress *pendingPtr;	/* Topmost search in progress, or NULL if
				 * none. */

    /*
     * List of generic handler records.
     */

    GenericHandler *genericList;/* First handler in the list, or NULL. */
    GenericHandler *lastGenericPtr;
				/* Last handler in list. */

    /*
     * List of client message handler records.
     */

    GenericHandler *cmList;	/* First handler in the list, or NULL. */
    GenericHandler *lastCmPtr;	/* Last handler in list. */

    /*
     * If someone has called Tk_RestrictEvents, the information below keeps
     * track of it.
     */

    Tk_RestrictProc *restrictProc;
				/* Function to call. NULL means no
				 * restrictProc is currently in effect. */
    ClientData restrictArg;	/* Argument to pass to restrictProc. */
    ExitHandler *firstExitPtr;	/* First in list of all exit handlers for this
				 * thread. */
    int inExit;			/* True when this thread is exiting. This is
				 * used as a hack to decide to close the
				 * standard channels. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * There are both per-process and per-thread exit handlers. The first list is
 * controlled by a mutex. The other is in thread local storage.
 */

static ExitHandler *firstExitPtr = NULL;
				/* First in list of all exit handlers for
				 * application. */
TCL_DECLARE_MUTEX(exitMutex)

/*
 * Prototypes for functions that are only referenced locally within this file.
 */

static void		CleanUpTkEvent(XEvent *eventPtr);
static void		DelayedMotionProc(ClientData clientData);
static unsigned long    GetEventMaskFromXEvent(XEvent *eventPtr);
static TkWindow *	GetTkWindowFromXEvent(XEvent *eventPtr);
static void		InvokeClientMessageHandlers(ThreadSpecificData *tsdPtr,
			    Tk_Window tkwin, XEvent *eventPtr);
static int		InvokeFocusHandlers(TkWindow **winPtrPtr,
			    unsigned long mask, XEvent *eventPtr);
static int		InvokeGenericHandlers(ThreadSpecificData *tsdPtr,
			    XEvent *eventPtr);
static int		InvokeMouseHandlers(TkWindow *winPtr,
			    unsigned long mask, XEvent *eventPtr);
static Window		ParentXId(Display *display, Window w);
static int		RefreshKeyboardMappingIfNeeded(XEvent *eventPtr);
static int		TkXErrorHandler(ClientData clientData,
			    XErrorEvent *errEventPtr);
static int		WindowEventProc(Tcl_Event *evPtr, int flags);
#ifdef TK_USE_INPUT_METHODS
static void		CreateXIC(TkWindow *winPtr);
#endif /* TK_USE_INPUT_METHODS */

/*
 *----------------------------------------------------------------------
 *
 * InvokeFocusHandlers --
 *
 *	Call focus-related code to look at FocusIn, FocusOut, Enter, and Leave
 *	events; depending on its return value, ignore the event.
 *
 * Results:
 *	0 further processing can be done on the event.
 *	1 we are done with the event passed.
 *
 * Side effects:
 *	The *winPtrPtr in the caller may be changed to the TkWindow for the
 *	window with focus.
 *
 *----------------------------------------------------------------------
 */

static int
InvokeFocusHandlers(
    TkWindow **winPtrPtr,
    unsigned long mask,
    XEvent *eventPtr)
{
    if ((mask & (FocusChangeMask|EnterWindowMask|LeaveWindowMask))
	    && (TkFocusFilterEvent(*winPtrPtr, eventPtr) == 0)) {
	return 1;
    }

    /*
     * Only key-related events are directed according to the focus.
     */

    if (mask & (KeyPressMask|KeyReleaseMask)) {
	(*winPtrPtr)->dispPtr->lastEventTime = eventPtr->xkey.time;
	*winPtrPtr = TkFocusKeyEvent(*winPtrPtr, eventPtr);
	if (*winPtrPtr == NULL) {
	    return 1;
	}
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * InvokeMouseHandlers --
 *
 *	Call a grab-related function to do special processing on pointer
 *	events.
 *
 * Results:
 *	0 further processing can be done on the event.
 *	1 we are done with the event passed.
 *
 * Side effects:
 *	New events may be queued from TkPointerEvent and grabs may be added
 *	and/or removed. The eventPtr may be changed by TkPointerEvent in some
 *	cases.
 *
 *----------------------------------------------------------------------
 */

static int
InvokeMouseHandlers(
    TkWindow *winPtr,
    unsigned long mask,
    XEvent *eventPtr)
{
    if (mask & (ButtonPressMask|ButtonReleaseMask|PointerMotionMask
	    |EnterWindowMask|LeaveWindowMask)) {

	if (mask & (ButtonPressMask|ButtonReleaseMask)) {
	    winPtr->dispPtr->lastEventTime = eventPtr->xbutton.time;
	} else if (mask & PointerMotionMask) {
	    winPtr->dispPtr->lastEventTime = eventPtr->xmotion.time;
	} else {
	    winPtr->dispPtr->lastEventTime = eventPtr->xcrossing.time;
	}

	if (TkPointerEvent(eventPtr, winPtr) == 0) {
	    /*
	     * The event should be ignored to make grab work correctly (as the
	     * comment for TkPointerEvent states).
	     */

	    return 1;
	}
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateXIC --
 *
 *	Create the X input context for our winPtr.
 *	XIM is only ever enabled on Unix.
 *
 *----------------------------------------------------------------------
 */

#ifdef TK_USE_INPUT_METHODS
static void
CreateXIC(
    TkWindow *winPtr)
{
    TkDisplay *dispPtr = winPtr->dispPtr;
    long im_event_mask = 0L;
    const char *preedit_attname = NULL;
    XVaNestedList preedit_attlist = NULL;

    if (dispPtr->inputStyle & XIMPreeditPosition) {
	XPoint spot = {0, 0};

	preedit_attname = XNPreeditAttributes;
	preedit_attlist = XVaCreateNestedList(0,
		XNSpotLocation, &spot,
		XNFontSet, dispPtr->inputXfs,
		NULL);
    }

    winPtr->inputContext = XCreateIC(dispPtr->inputMethod,
	    XNInputStyle, dispPtr->inputStyle,
	    XNClientWindow, winPtr->window,
	    XNFocusWindow, winPtr->window,
	    preedit_attname, preedit_attlist,
	    NULL);

    if (preedit_attlist) {
	XFree(preedit_attlist);
    }


    if (winPtr->inputContext == NULL) {
	/* XCreateIC failed. */
	return;
    }
    winPtr->ximGeneration = dispPtr->ximGeneration;

    /*
     * Adjust the window's event mask if the IM requires it.
     */
    XGetICValues(winPtr->inputContext, XNFilterEvents, &im_event_mask, NULL);
    if ((winPtr->atts.event_mask & im_event_mask) != im_event_mask) {
	winPtr->atts.event_mask |= im_event_mask;
	XSelectInput(winPtr->display, winPtr->window, winPtr->atts.event_mask);
    }
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * GetTkWindowFromXEvent --
 *
 *	Attempt to find which TkWindow is associated with an event. If it
 *	fails we attempt to get the TkWindow from the parent for a property
 *	notification.
 *
 * Results:
 *	The TkWindow associated with the event or NULL.
 *
 * Side effects:
 *	TkSelPropProc may influence selection on windows not known to Tk.
 *
 *----------------------------------------------------------------------
 */

static TkWindow *
GetTkWindowFromXEvent(
    XEvent *eventPtr)
{
    TkWindow *winPtr;
    Window parentXId, handlerWindow = eventPtr->xany.window;

    if ((eventPtr->xany.type == StructureNotifyMask)
	    && (eventPtr->xmap.event != eventPtr->xmap.window)) {
	handlerWindow = eventPtr->xmap.event;
    }

    winPtr = (TkWindow *) Tk_IdToWindow(eventPtr->xany.display, handlerWindow);

    if (winPtr == NULL) {
	/*
	 * There isn't a TkWindow structure for this window. However, if the
	 * event is a PropertyNotify event then call the selection manager (it
	 * deals beneath-the-table with certain properties). Also, if the
	 * window's parent is a Tk window that has the TK_PROP_PROPCHANGE flag
	 * set, then we must propagate the PropertyNotify event up to the
	 * parent.
	 */

	if (eventPtr->type != PropertyNotify) {
	    return NULL;
	}
	TkSelPropProc(eventPtr);
	parentXId = ParentXId(eventPtr->xany.display, handlerWindow);
	if (parentXId == None) {
	    return NULL;
	}
	winPtr = (TkWindow *) Tk_IdToWindow(eventPtr->xany.display, parentXId);
	if (winPtr == NULL) {
	    return NULL;
	}
	if (!(winPtr->flags & TK_PROP_PROPCHANGE)) {
	    return NULL;
	}
    }
    return winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetEventMaskFromXEvent --
 *
 *	The event type is looked up in our eventMasks tables, and may be
 *	changed to a different mask depending on the state of the event and
 *	window members.
 *
 * Results:
 *	The mask for the event.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned long
GetEventMaskFromXEvent(
    XEvent *eventPtr)
{
    unsigned long mask;

    /*
     * Get the event mask from the correct table. Note that there are two
     * tables here because that means we no longer need this code to rely on
     * the exact value of VirtualEvent, which has caused us problems in the
     * past when X11 changed the value of LASTEvent. [Bug ???]
     */

    if (eventPtr->xany.type <= MappingNotify) {
	mask = realEventMasks[eventPtr->xany.type];
    } else if (eventPtr->xany.type >= VirtualEvent
	    && eventPtr->xany.type<TK_LASTEVENT) {
	mask = virtualEventMasks[eventPtr->xany.type - VirtualEvent];
    } else {
	mask = 0;
    }

    /*
     * Events selected by StructureNotify require special handling. They look
     * the same as those selected by SubstructureNotify. The only difference
     * is whether the "event" and "window" fields are the same. Compare the
     * two fields and convert StructureNotify to SubstructureNotify if
     * necessary.
     */

    if (mask == StructureNotifyMask) {
	if (eventPtr->xmap.event != eventPtr->xmap.window) {
	    mask = SubstructureNotifyMask;
	}
    }
    return mask;
}

/*
 *----------------------------------------------------------------------
 *
 * RefreshKeyboardMappingIfNeeded --
 *
 *	If the event is a MappingNotify event, find its display and refresh
 *	the keyboard mapping information for the display.
 *
 * Results:
 *	0 if the event was not a MappingNotify event
 *	1 if the event was a MappingNotify event
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
RefreshKeyboardMappingIfNeeded(
    XEvent *eventPtr)
{
    TkDisplay *dispPtr;

    if (eventPtr->type == MappingNotify) {
	dispPtr = TkGetDisplay(eventPtr->xmapping.display);
	if (dispPtr != NULL) {
	    XRefreshKeyboardMapping(&eventPtr->xmapping);
	    dispPtr->bindInfoStale = 1;
	}
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetButtonMask --
 *
 *	Return the proper Button${n}Mask for the button.
 *
 * Results:
 *	A button mask.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static const unsigned long buttonMasks[] = {
    0, Button1Mask, Button2Mask, Button3Mask, Button4Mask, Button5Mask
};

unsigned long
TkGetButtonMask(
    unsigned int button)
{
    return (button > Button5) ? 0 : buttonMasks[button];
}

/*
 *----------------------------------------------------------------------
 *
 * InvokeClientMessageHandlers --
 *
 *	Iterate the list of handlers and invoke the function pointer for each.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Handlers may be deleted and events may be sent to handlers.
 *
 *----------------------------------------------------------------------
 */

static void
InvokeClientMessageHandlers(
    ThreadSpecificData *tsdPtr,
    Tk_Window tkwin,
    XEvent *eventPtr)
{
    GenericHandler *prevPtr, *tmpPtr, *curPtr = tsdPtr->cmList;

    for (prevPtr = NULL; curPtr != NULL; ) {
	if (curPtr->deleteFlag) {
	    if (!tsdPtr->handlersActive) {
		/*
		 * This handler needs to be deleted and there are no calls
		 * pending through any handlers, so now is a safe time to
		 * delete it.
		 */

		tmpPtr = curPtr->nextPtr;
		if (prevPtr == NULL) {
		    tsdPtr->cmList = tmpPtr;
		} else {
		    prevPtr->nextPtr = tmpPtr;
		}
		if (tmpPtr == NULL) {
		    tsdPtr->lastCmPtr = prevPtr;
		}
		ckfree(curPtr);
		curPtr = tmpPtr;
		continue;
	    }
	} else {
	    int done;

	    tsdPtr->handlersActive++;
	    done = (*(Tk_ClientMessageProc *)curPtr->proc)(tkwin, eventPtr);
	    tsdPtr->handlersActive--;
	    if (done) {
		break;
	    }
	}
	prevPtr = curPtr;
	curPtr = curPtr->nextPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InvokeGenericHandlers --
 *
 *	Iterate the list of handlers and invoke the function pointer for each.
 *	If the handler invoked returns a non-zero value then we are done.
 *
 * Results:
 *	0 when the event wasn't handled by a handler. Non-zero when it was
 *	processed and handled by a handler.
 *
 * Side effects:
 *	Handlers may be deleted and events may be sent to handlers.
 *
 *----------------------------------------------------------------------
 */

static int
InvokeGenericHandlers(
    ThreadSpecificData *tsdPtr,
    XEvent *eventPtr)
{
    GenericHandler *prevPtr, *tmpPtr, *curPtr = tsdPtr->genericList;

    for (prevPtr = NULL; curPtr != NULL; ) {
	if (curPtr->deleteFlag) {
	    if (!tsdPtr->handlersActive) {
		/*
		 * This handler needs to be deleted and there are no calls
		 * pending through the handler, so now is a safe time to
		 * delete it.
		 */

		tmpPtr = curPtr->nextPtr;
		if (prevPtr == NULL) {
		    tsdPtr->genericList = tmpPtr;
		} else {
		    prevPtr->nextPtr = tmpPtr;
		}
		if (tmpPtr == NULL) {
		    tsdPtr->lastGenericPtr = prevPtr;
		}
		ckfree(curPtr);
		curPtr = tmpPtr;
		continue;
	    }
	} else {
	    int done;

	    tsdPtr->handlersActive++;
	    done = curPtr->proc(curPtr->clientData, eventPtr);
	    tsdPtr->handlersActive--;
	    if (done) {
		return done;
	    }
	}
	prevPtr = curPtr;
	curPtr = curPtr->nextPtr;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateEventHandler --
 *
 *	Arrange for a given function to be invoked whenever events from a
 *	given class occur in a given window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, whenever an event of the type given by mask occurs for
 *	token and is processed by Tk_HandleEvent, proc will be called. See the
 *	manual entry for details of the calling sequence and return value for
 *	proc.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CreateEventHandler(
    Tk_Window token,		/* Token for window in which to create
				 * handler. */
    unsigned long mask,		/* Events for which proc should be called. */
    Tk_EventProc *proc,		/* Function to call for each selected event */
    ClientData clientData)	/* Arbitrary data to pass to proc. */
{
    register TkEventHandler *handlerPtr;
    register TkWindow *winPtr = (TkWindow *) token;

    /*
     * Skim through the list of existing handlers to (a) compute the overall
     * event mask for the window (so we can pass this new value to the X
     * system) and (b) see if there's already a handler declared with the same
     * callback and clientData (if so, just change the mask). If no existing
     * handler matches, then create a new handler.
     */

    if (winPtr->handlerList == NULL) {
	/*
	 * No event handlers defined at all, so must create.
	 */

	handlerPtr = ckalloc(sizeof(TkEventHandler));
	winPtr->handlerList = handlerPtr;
    } else {
	int found = 0;

	for (handlerPtr = winPtr->handlerList; ;
		handlerPtr = handlerPtr->nextPtr) {
	    if ((handlerPtr->proc == proc)
		    && (handlerPtr->clientData == clientData)) {
		handlerPtr->mask = mask;
		found = 1;
	    }
	    if (handlerPtr->nextPtr == NULL) {
		break;
	    }
	}

	/*
	 * If we found anything, we're done because we do not need to use
	 * XSelectInput; Tk always selects on all events anyway in order to
	 * support binding on classes, 'all' and other bind-tags.
	 */

	if (found) {
	    return;
	}

	/*
	 * No event handler matched, so create a new one.
	 */

	handlerPtr->nextPtr = ckalloc(sizeof(TkEventHandler));
	handlerPtr = handlerPtr->nextPtr;
    }

    /*
     * Initialize the new event handler.
     */

    handlerPtr->mask = mask;
    handlerPtr->proc = proc;
    handlerPtr->clientData = clientData;
    handlerPtr->nextPtr = NULL;

    /*
     * No need to call XSelectInput: Tk always selects on all events for all
     * windows (needed to support bindings on classes and "all").
     */
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DeleteEventHandler --
 *
 *	Delete a previously-created handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there existed a handler as described by the parameters, the handler
 *	is deleted so that proc will not be invoked again.
 *
 *----------------------------------------------------------------------
 */

void
Tk_DeleteEventHandler(
    Tk_Window token,		/* Same as corresponding arguments passed */
    unsigned long mask,		/* previously to Tk_CreateEventHandler. */
    Tk_EventProc *proc,
    ClientData clientData)
{
    register TkEventHandler *handlerPtr;
    register InProgress *ipPtr;
    TkEventHandler *prevPtr;
    register TkWindow *winPtr = (TkWindow *) token;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Find the event handler to be deleted, or return immediately if it
     * doesn't exist.
     */

    for (handlerPtr = winPtr->handlerList, prevPtr = NULL; ;
	    prevPtr = handlerPtr, handlerPtr = handlerPtr->nextPtr) {
	if (handlerPtr == NULL) {
	    return;
	}
	if ((handlerPtr->mask == mask) && (handlerPtr->proc == proc)
		&& (handlerPtr->clientData == clientData)) {
	    break;
	}
    }

    /*
     * If Tk_HandleEvent is about to process this handler, tell it to process
     * the next one instead.
     */

    for (ipPtr = tsdPtr->pendingPtr; ipPtr != NULL; ipPtr = ipPtr->nextPtr) {
	if (ipPtr->nextHandler == handlerPtr) {
	    ipPtr->nextHandler = handlerPtr->nextPtr;
	}
    }

    /*
     * Free resources associated with the handler.
     */

    if (prevPtr == NULL) {
	winPtr->handlerList = handlerPtr->nextPtr;
    } else {
	prevPtr->nextPtr = handlerPtr->nextPtr;
    }
    ckfree(handlerPtr);

    /*
     * No need to call XSelectInput: Tk always selects on all events for all
     * windows (needed to support bindings on classes and "all").
     */
}

/*----------------------------------------------------------------------
 *
 * Tk_CreateGenericHandler --
 *
 *	Register a function to be called on each X event, regardless of
 *	display or window. Generic handlers are useful for capturing events
 *	that aren't associated with windows, or events for windows not managed
 *	by Tk.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	From now on, whenever an X event is given to Tk_HandleEvent, invoke
 *	proc, giving it clientData and the event as arguments.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CreateGenericHandler(
    Tk_GenericProc *proc,	/* Function to call on every event. */
    ClientData clientData)	/* One-word value to pass to proc. */
{
    GenericHandler *handlerPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    handlerPtr = ckalloc(sizeof(GenericHandler));

    handlerPtr->proc		= proc;
    handlerPtr->clientData	= clientData;
    handlerPtr->deleteFlag	= 0;
    handlerPtr->nextPtr		= NULL;
    if (tsdPtr->genericList == NULL) {
	tsdPtr->genericList	= handlerPtr;
    } else {
	tsdPtr->lastGenericPtr->nextPtr = handlerPtr;
    }
    tsdPtr->lastGenericPtr	= handlerPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DeleteGenericHandler --
 *
 *	Delete a previously-created generic handler.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If there existed a handler as described by the parameters, that
 *	handler is logically deleted so that proc will not be invoked again.
 *	The physical deletion happens in the event loop in Tk_HandleEvent.
 *
 *----------------------------------------------------------------------
 */

void
Tk_DeleteGenericHandler(
    Tk_GenericProc *proc,
    ClientData clientData)
{
    GenericHandler * handler;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (handler=tsdPtr->genericList ; handler ; handler=handler->nextPtr) {
	if ((handler->proc == proc) && (handler->clientData == clientData)) {
	    handler->deleteFlag = 1;
	}
    }
}

/*----------------------------------------------------------------------
 *
 * Tk_CreateClientMessageHandler --
 *
 *	Register a function to be called on each ClientMessage event.
 *	ClientMessage handlers are useful for Drag&Drop extensions.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	From now on, whenever a ClientMessage event is received that isn't a
 *	WM_PROTOCOL event or SelectionEvent, invoke proc, giving it tkwin and
 *	the event as arguments.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CreateClientMessageHandler(
    Tk_ClientMessageProc *proc)	/* Function to call on event. */
{
    GenericHandler *handlerPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * We use a GenericHandler struct, because it's basically the same, except
     * with an extra clientData field we'll never use.
     */

    handlerPtr = ckalloc(sizeof(GenericHandler));

    handlerPtr->proc = (Tk_GenericProc *) proc;
    handlerPtr->clientData = NULL;	/* never used */
    handlerPtr->deleteFlag = 0;
    handlerPtr->nextPtr = NULL;
    if (tsdPtr->cmList == NULL) {
	tsdPtr->cmList = handlerPtr;
    } else {
	tsdPtr->lastCmPtr->nextPtr = handlerPtr;
    }
    tsdPtr->lastCmPtr = handlerPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DeleteClientMessageHandler --
 *
 *	Delete a previously-created ClientMessage handler.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If there existed a handler as described by the parameters, that
 *	handler is logically deleted so that proc will not be invoked again.
 *	The physical deletion happens in the event loop in
 *	TkClientMessageEventProc.
 *
 *----------------------------------------------------------------------
 */

void
Tk_DeleteClientMessageHandler(
    Tk_ClientMessageProc *proc)
{
    GenericHandler * handler;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (handler=tsdPtr->cmList ; handler!=NULL ; handler=handler->nextPtr) {
	if (handler->proc == (Tk_GenericProc *) proc) {
	    handler->deleteFlag = 1;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkEventInit --
 *
 *	This functions initializes all the event module structures used by the
 *	current thread. It must be called before any other function in this
 *	file is called.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkEventInit(void)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->handlersActive	= 0;
    tsdPtr->pendingPtr		= NULL;
    tsdPtr->genericList		= NULL;
    tsdPtr->lastGenericPtr	= NULL;
    tsdPtr->cmList		= NULL;
    tsdPtr->lastCmPtr		= NULL;
    tsdPtr->restrictProc	= NULL;
    tsdPtr->restrictArg		= NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkXErrorHandler --
 *
 *	TkXErrorHandler is an error handler, to be installed via
 *	Tk_CreateErrorHandler, that will set a flag if an X error occurred.
 *
 * Results:
 *	Always returns 0, indicating that the X error was handled.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TkXErrorHandler(
    ClientData clientData,	/* Pointer to flag we set. */
    XErrorEvent *errEventPtr)	/* X error info. */
{
    int *error = clientData;

    *error = 1;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ParentXId --
 *
 *	Returns the parent of the given window, or "None" if the window
 *	doesn't exist.
 *
 * Results:
 *	Returns an X window ID.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Window
ParentXId(
    Display *display,
    Window w)
{
    Tk_ErrorHandler handler;
    int gotXError;
    Status status;
    Window parent;
    Window root;
    Window *childList;
    unsigned int nChildren;

    /*
     * Handle errors ourselves.
     */

    gotXError = 0;
    handler = Tk_CreateErrorHandler(display, -1, -1, -1,
	    TkXErrorHandler, &gotXError);

    /*
     * Get the parent window.
     */

    status = XQueryTree(display, w, &root, &parent, &childList, &nChildren);

    /*
     * Do some cleanup; gotta return "None" if we got an error.
     */

    Tk_DeleteErrorHandler(handler);
    XSync(display, False);
    if (status != 0 && childList != NULL) {
	XFree(childList);
    }
    if (status == 0) {
	parent = None;
    }

    return parent;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_HandleEvent --
 *
 *	Given an event, invoke all the handlers that have been registered for
 *	the event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the handlers.
 *
 *----------------------------------------------------------------------
 */

void
Tk_HandleEvent(
    XEvent *eventPtr)	/* Event to dispatch. */
{
    register TkEventHandler *handlerPtr;
    TkWindow *winPtr;
    unsigned long mask;
    InProgress ip;
    Tcl_Interp *interp = NULL;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

#if !defined(MAC_OSX_TK) && !defined(_WIN32)
    if (((eventPtr->type == ButtonPress) || (eventPtr->type == ButtonRelease))
	    && ((eventPtr->xbutton.button - 6) < 2)) {
	eventPtr->xbutton.button -= 2;
	eventPtr->xbutton.state ^= ShiftMask;
    }
#endif

    /*
     * If the generic handler processed this event we are done and can return.
     */

    if (InvokeGenericHandlers(tsdPtr, eventPtr)) {
	goto releaseEventResources;
    }

    if (RefreshKeyboardMappingIfNeeded(eventPtr)) {
	/*
	 * We are done with a MappingNotify event.
	 */

	goto releaseEventResources;
    }

    mask = GetEventMaskFromXEvent(eventPtr);
    winPtr = GetTkWindowFromXEvent(eventPtr);

    if (winPtr == NULL) {
	goto releaseEventResources;
    }

    /*
     * Once a window has started getting deleted, don't process any more
     * events for it except for the DestroyNotify event. This check is needed
     * because a DestroyNotify handler could re-invoke the event loop, causing
     * other pending events to be handled for the window (the window doesn't
     * get totally expunged from our tables until after the DestroyNotify
     * event has been completely handled).
     */

    if ((winPtr->flags & TK_ALREADY_DEAD)
	    && (eventPtr->type != DestroyNotify)) {
	goto releaseEventResources;
    }

    if (winPtr->mainPtr != NULL) {
	int result;

	interp = winPtr->mainPtr->interp;

	/*
	 * Protect interpreter for this window from possible deletion while we
	 * are dealing with the event for this window. Thus, widget writers do
	 * not have to worry about protecting the interpreter in their own
	 * code.
	 */

	Tcl_Preserve(interp);

	result = ((InvokeFocusHandlers(&winPtr, mask, eventPtr))
		|| (InvokeMouseHandlers(winPtr, mask, eventPtr)));

	if (result) {
	    goto releaseInterpreter;
	}
    }

    /*
     * Create the input context for the window if it hasn't already been done
     * (XFilterEvent needs this context). When the event is a FocusIn event,
     * set the input context focus to the receiving window. This code is only
     * ever active for X11.
     */

#ifdef TK_USE_INPUT_METHODS
    /*
     * If the XIC has been invalidated, it must be recreated.
     */
    if (winPtr->dispPtr->ximGeneration != winPtr->ximGeneration) {
	winPtr->flags &= ~TK_CHECKED_IC;
	winPtr->inputContext = NULL;
    }

    if ((winPtr->dispPtr->flags & TK_DISPLAY_USE_IM)) {
	if (!(winPtr->flags & (TK_CHECKED_IC|TK_ALREADY_DEAD))) {
	    winPtr->flags |= TK_CHECKED_IC;
	    if (winPtr->dispPtr->inputMethod != NULL) {
		CreateXIC(winPtr);
	    }
	}
	if ((eventPtr->type == FocusIn) &&
		(winPtr->dispPtr->inputMethod != NULL) &&
		(winPtr->inputContext != NULL)) {
	    XSetICFocus(winPtr->inputContext);
	}
    }
#endif /*TK_USE_INPUT_METHODS*/

    /*
     * For events where it hasn't already been done, update the current time
     * in the display.
     */

    if (eventPtr->type == PropertyNotify) {
	winPtr->dispPtr->lastEventTime = eventPtr->xproperty.time;
    }

    /*
     * There's a potential interaction here with Tk_DeleteEventHandler. Read
     * the documentation for pendingPtr.
     */

    ip.eventPtr = eventPtr;
    ip.winPtr = winPtr;
    ip.nextHandler = NULL;
    ip.nextPtr = tsdPtr->pendingPtr;
    tsdPtr->pendingPtr = &ip;
    if (mask == 0) {
	if ((eventPtr->type == SelectionClear)
		|| (eventPtr->type == SelectionRequest)
		|| (eventPtr->type == SelectionNotify)) {
	    TkSelEventProc((Tk_Window) winPtr, eventPtr);
	} else if (eventPtr->type == ClientMessage) {
	    if (eventPtr->xclient.message_type ==
		    Tk_InternAtom((Tk_Window) winPtr, "WM_PROTOCOLS")) {
		TkWmProtocolEventProc(winPtr, eventPtr);
	    } else {
		InvokeClientMessageHandlers(tsdPtr, (Tk_Window) winPtr,
			eventPtr);
	    }
	}
    } else {
	for (handlerPtr = winPtr->handlerList; handlerPtr != NULL; ) {
	    if (handlerPtr->mask & mask) {
		ip.nextHandler = handlerPtr->nextPtr;
		handlerPtr->proc(handlerPtr->clientData, eventPtr);
		handlerPtr = ip.nextHandler;
	    } else {
		handlerPtr = handlerPtr->nextPtr;
	    }
	}

	/*
	 * Pass the event to the "bind" command mechanism. But, don't do this
	 * for SubstructureNotify events. The "bind" command doesn't support
	 * them anyway, and it's easier to filter out these events here than
	 * in the lower-level functions.
	 */

	/*
	 * ...well, except when we use the tkwm patches, in which case we DO
	 * handle CreateNotify events, so we gotta pass 'em through.
	 */

	if ((ip.winPtr != NULL)
		&& ((mask != SubstructureNotifyMask)
		|| (eventPtr->type == CreateNotify))) {
	    TkBindEventProc(winPtr, eventPtr);
	}
    }
    tsdPtr->pendingPtr = ip.nextPtr;

    /*
     * Release the interpreter for this window so that it can be potentially
     * deleted if requested.
     */

  releaseInterpreter:
    if (interp != NULL) {
	Tcl_Release(interp);
    }

    /*
     * Release the user_data from the event (if it is a virtual event and the
     * field was non-NULL in the first place.) Note that this is done using a
     * Tcl_Obj interface, and we set the field back to NULL afterwards out of
     * paranoia. Also clean up any cached %A substitutions from key events.
     */

  releaseEventResources:
    CleanUpTkEvent(eventPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkEventDeadWindow --
 *
 *	This function is invoked when it is determined that a window is dead.
 *	It cleans up event-related information about the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Various things get cleaned up and recycled.
 *
 *----------------------------------------------------------------------
 */

void
TkEventDeadWindow(
    TkWindow *winPtr)		/* Information about the window that is being
				 * deleted. */
{
    register TkEventHandler *handlerPtr;
    register InProgress *ipPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * While deleting all the handlers, be careful to check for Tk_HandleEvent
     * being about to process one of the deleted handlers. If it is, tell it
     * to quit (all of the handlers are being deleted).
     */

    while (winPtr->handlerList != NULL) {
	handlerPtr = winPtr->handlerList;
	winPtr->handlerList = handlerPtr->nextPtr;
	for (ipPtr = tsdPtr->pendingPtr; ipPtr != NULL;
		ipPtr = ipPtr->nextPtr) {
	    if (ipPtr->nextHandler == handlerPtr) {
		ipPtr->nextHandler = NULL;
	    }
	    if (ipPtr->winPtr == winPtr) {
		ipPtr->winPtr = NULL;
	    }
	}
	ckfree(handlerPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkCurrentTime --
 *
 *	Try to deduce the current time. "Current time" means the time of the
 *	event that led to the current code being executed, which means the
 *	time in the most recently-nested invocation of Tk_HandleEvent.
 *
 * Results:
 *	The return value is the time from the current event, or CurrentTime if
 *	there is no current event or if the current event contains no time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Time
TkCurrentTime(
    TkDisplay *dispPtr)		/* Display for which the time is desired. */
{
    register XEvent *eventPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->pendingPtr == NULL) {
	return dispPtr->lastEventTime;
    }
    eventPtr = tsdPtr->pendingPtr->eventPtr;
    switch (eventPtr->type) {
    case ButtonPress:
    case ButtonRelease:
	return eventPtr->xbutton.time;
    case KeyPress:
    case KeyRelease:
	return eventPtr->xkey.time;
    case MotionNotify:
	return eventPtr->xmotion.time;
    case EnterNotify:
    case LeaveNotify:
	return eventPtr->xcrossing.time;
    case PropertyNotify:
	return eventPtr->xproperty.time;
    }
    return dispPtr->lastEventTime;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_RestrictEvents --
 *
 *	This function is used to globally restrict the set of events that will
 *	be dispatched. The restriction is done by filtering all incoming X
 *	events through a function that determines whether they are to be
 *	processed immediately, deferred, or discarded.
 *
 * Results:
 *	The return value is the previous restriction function in effect, if
 *	there was one, or NULL if there wasn't.
 *
 * Side effects:
 *	From now on, proc will be called to determine whether to process,
 *	defer or discard each incoming X event.
 *
 *----------------------------------------------------------------------
 */

Tk_RestrictProc *
Tk_RestrictEvents(
    Tk_RestrictProc *proc,	/* Function to call for each incoming event */
    ClientData arg,		/* Arbitrary argument to pass to proc. */
    ClientData *prevArgPtr)	/* Place to store information about previous
				 * argument. */
{
    Tk_RestrictProc *prev;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    prev = tsdPtr->restrictProc;
    *prevArgPtr = tsdPtr->restrictArg;
    tsdPtr->restrictProc = proc;
    tsdPtr->restrictArg = arg;
    return prev;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CollapseMotionEvents --
 *
 *	This function controls whether we collapse motion events in a
 *	particular display or not.
 *
 * Results:
 *	The return value is the previous collapse value in effect.
 *
 * Side effects:
 *	Filtering of motion events may be changed after calling this.
 *
 *----------------------------------------------------------------------
 */

int
Tk_CollapseMotionEvents(
    Display *display,		/* Display handling these events. */
    int collapse)		/* Boolean value that specifies whether motion
				 * events should be collapsed. */
{
    TkDisplay *dispPtr = (TkDisplay *) display;
    int prev = (dispPtr->flags & TK_DISPLAY_COLLAPSE_MOTION_EVENTS);

    if (collapse) {
	dispPtr->flags |= TK_DISPLAY_COLLAPSE_MOTION_EVENTS;
    } else {
	dispPtr->flags &= ~TK_DISPLAY_COLLAPSE_MOTION_EVENTS;
    }
    return prev;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_QueueWindowEvent --
 *
 *	Given an X-style window event, this function adds it to the Tcl event
 *	queue at the given position. This function also performs mouse motion
 *	event collapsing if possible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds stuff to the event queue, which will eventually be processed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_QueueWindowEvent(
    XEvent *eventPtr,		/* Event to add to queue. This function copies
				 * it before adding it to the queue. */
    Tcl_QueuePosition position)	/* Where to put it on the queue:
				 * TCL_QUEUE_TAIL, TCL_QUEUE_HEAD, or
				 * TCL_QUEUE_MARK. */
{
    TkWindowEvent *wevPtr;
    TkDisplay *dispPtr;

    /*
     * Find our display structure for the event's display.
     */

    for (dispPtr = TkGetDisplayList(); ; dispPtr = dispPtr->nextPtr) {
	if (dispPtr == NULL) {
	    return;
	}
	if (dispPtr->display == eventPtr->xany.display) {
	    break;
	}
    }

    /*
     * Don't filter motion events if the user defaulting to true (1), which
     * could be set to false (0) when the user wishes to receive all the
     * motion data)
     */

    if (!(dispPtr->flags & TK_DISPLAY_COLLAPSE_MOTION_EVENTS)) {
	wevPtr = ckalloc(sizeof(TkWindowEvent));
	wevPtr->header.proc = WindowEventProc;
	wevPtr->event = *eventPtr;
	Tcl_QueueEvent(&wevPtr->header, position);
	return;
    }

    if ((dispPtr->delayedMotionPtr != NULL) && (position == TCL_QUEUE_TAIL)) {
	if ((eventPtr->type == MotionNotify) && (eventPtr->xmotion.window
		== dispPtr->delayedMotionPtr->event.xmotion.window)) {
	    /*
	     * The new event is a motion event in the same window as the saved
	     * motion event. Just replace the saved event with the new one.
	     */

	    dispPtr->delayedMotionPtr->event = *eventPtr;
	    return;
	} else if ((eventPtr->type != GraphicsExpose)
		&& (eventPtr->type != NoExpose)
		&& (eventPtr->type != Expose)) {
	    /*
	     * The new event may conflict with the saved motion event. Queue
	     * the saved motion event now so that it will be processed before
	     * the new event.
	     */

	    Tcl_QueueEvent(&dispPtr->delayedMotionPtr->header, position);
	    dispPtr->delayedMotionPtr = NULL;
	    Tcl_CancelIdleCall(DelayedMotionProc, dispPtr);
	}
    }

    wevPtr = ckalloc(sizeof(TkWindowEvent));
    wevPtr->header.proc = WindowEventProc;
    wevPtr->event = *eventPtr;
    if ((eventPtr->type == MotionNotify) && (position == TCL_QUEUE_TAIL)) {
	/*
	 * The new event is a motion event so don't queue it immediately; save
	 * it around in case another motion event arrives that it can be
	 * collapsed with.
	 */

	if (dispPtr->delayedMotionPtr != NULL) {
	    Tcl_Panic("Tk_QueueWindowEvent found unexpected delayed motion event");
	}
	dispPtr->delayedMotionPtr = wevPtr;
	Tcl_DoWhenIdle(DelayedMotionProc, dispPtr);
    } else {
	Tcl_QueueEvent(&wevPtr->header, position);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkQueueEventForAllChildren --
 *
 *	Given an XEvent, recursively queue the event for this window and all
 *	non-toplevel children of the given window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Events queued.
 *
 *----------------------------------------------------------------------
 */

void
TkQueueEventForAllChildren(
    TkWindow *winPtr,	    /* Window to which event is sent. */
    XEvent *eventPtr)	    /* The event to be sent. */
{
    TkWindow *childPtr;

    if (!Tk_IsMapped(winPtr)) {
	return;
    }

    eventPtr->xany.window = winPtr->window;
    Tk_QueueWindowEvent(eventPtr, TCL_QUEUE_TAIL);

    childPtr = winPtr->childList;
    while (childPtr != NULL) {
	if (!Tk_TopWinHierarchy(childPtr)) {
	    TkQueueEventForAllChildren(childPtr, eventPtr);
	}
	childPtr = childPtr->nextPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WindowEventProc --
 *
 *	This function is called by Tcl_DoOneEvent when a window event reaches
 *	the front of the event queue. This function is responsible for
 *	actually handling the event.
 *
 * Results:
 *	Returns 1 if the event was handled, meaning it should be removed from
 *	the queue. Returns 0 if the event was not handled, meaning it should
 *	stay on the queue. The event isn't handled if the TCL_WINDOW_EVENTS
 *	bit isn't set in flags, if a restrict proc prevents the event from
 *	being handled.
 *
 * Side effects:
 *	Whatever the event handlers for the event do.
 *
 *----------------------------------------------------------------------
 */

static int
WindowEventProc(
    Tcl_Event *evPtr,		/* Event to service. */
    int flags)			/* Flags that indicate what events to handle,
				 * such as TCL_WINDOW_EVENTS. */
{
    TkWindowEvent *wevPtr = (TkWindowEvent *) evPtr;
    Tk_RestrictAction result;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!(flags & TCL_WINDOW_EVENTS)) {
	return 0;
    }
    if (tsdPtr->restrictProc != NULL) {
	result = tsdPtr->restrictProc(tsdPtr->restrictArg, &wevPtr->event);
	if (result != TK_PROCESS_EVENT) {
	    if (result == TK_DEFER_EVENT) {
		return 0;
	    } else {
		/*
		 * TK_DELETE_EVENT: return and say we processed the event,
		 * even though we didn't do anything at all.
		 */

		CleanUpTkEvent(&wevPtr->event);
		return 1;
	    }
	}
    }
    Tk_HandleEvent(&wevPtr->event);
    CleanUpTkEvent(&wevPtr->event);
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * CleanUpTkEvent --
 *
 *	This function is called to remove and deallocate any information in
 *	the event which is not directly in the event structure itself. It may
 *	be called multiple times per event, so it takes care to set the
 *	cleared pointer fields to NULL afterwards.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Makes the event no longer have any external resources.
 *
 *----------------------------------------------------------------------
 */

static void
CleanUpTkEvent(
    XEvent *eventPtr)
{
    switch (eventPtr->type) {
    case KeyPress:
    case KeyRelease: {
	TkKeyEvent *kePtr = (TkKeyEvent *) eventPtr;

	if (kePtr->charValuePtr != NULL) {
	    ckfree(kePtr->charValuePtr);
	    kePtr->charValuePtr = NULL;
	    kePtr->charValueLen = 0;
	}
	break;
    }

    case VirtualEvent: {
	XVirtualEvent *vePtr = (XVirtualEvent *) eventPtr;

	if (vePtr->user_data != NULL) {
	    Tcl_DecrRefCount(vePtr->user_data);
	    vePtr->user_data = NULL;
	}
	break;
    }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DelayedMotionProc --
 *
 *	This function is invoked as an idle handler when a mouse motion event
 *	has been delayed. It queues the delayed event so that it will finally
 *	be serviced.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The delayed mouse motion event gets added to the Tcl event queue for
 *	servicing.
 *
 *----------------------------------------------------------------------
 */

static void
DelayedMotionProc(
    ClientData clientData)	/* Pointer to display containing a delayed
				 * motion event to be serviced. */
{
    TkDisplay *dispPtr = clientData;

    if (dispPtr->delayedMotionPtr == NULL) {
	Tcl_Panic("DelayedMotionProc found no delayed mouse motion event");
    }
    Tcl_QueueEvent(&dispPtr->delayedMotionPtr->header, TCL_QUEUE_TAIL);
    dispPtr->delayedMotionPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkCreateExitHandler --
 *
 *	Same as Tcl_CreateExitHandler, but private to Tk.
 *
 * Results:
 *	None.
 *
 * Side effects.
 *	Sets a handler with Tcl_CreateExitHandler if this is the first call.
 *
 *----------------------------------------------------------------------
 */

void
TkCreateExitHandler(
    Tcl_ExitProc *proc,		/* Function to invoke. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr;

    exitPtr = ckalloc(sizeof(ExitHandler));
    exitPtr->proc = proc;
    exitPtr->clientData = clientData;
    Tcl_MutexLock(&exitMutex);

    /*
     * The call to TclInExit() is disabled here. That's a private Tcl routine,
     * and calling it is causing some trouble with portability of building Tk.
     * We should avoid private Tcl routines generally.
     *
     * In this case, the TclInExit() call is being used only to prevent a
     * Tcl_CreateExitHandler() call when Tcl finalization is in progress.
     * That's a situation that shouldn't happen anyway. Recent changes within
     * Tcl_Finalize now cause a Tcl_Panic() to happen if exit handlers get
     * added after exit handling is complete. By disabling the guard here,
     * that panic will serve to help us find the buggy conditions and correct
     * them.
     *
     * We can restore this guard if we find we must (hopefully getting public
     * access to TclInExit() if we discover extensions really do need this),
     * but during alpha development, this is a good time to dig in and find
     * the root causes of finalization bugs.
     */

    if (firstExitPtr == NULL/* && !TclInExit()*/) {
	Tcl_CreateExitHandler(TkFinalize, NULL);
    }
    exitPtr->nextPtr = firstExitPtr;
    firstExitPtr = exitPtr;
    Tcl_MutexUnlock(&exitMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TkDeleteExitHandler --
 *
 *	Same as Tcl_DeleteExitHandler, but private to Tk.
 *
 * Results:
 *	None.
 *
 * Side effects.
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkDeleteExitHandler(
    Tcl_ExitProc *proc,		/* Function that was previously registered. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr, *prevPtr;

    Tcl_MutexLock(&exitMutex);
    for (prevPtr = NULL, exitPtr = firstExitPtr; exitPtr != NULL;
	    prevPtr = exitPtr, exitPtr = exitPtr->nextPtr) {
	if ((exitPtr->proc == proc)
		&& (exitPtr->clientData == clientData)) {
	    if (prevPtr == NULL) {
		firstExitPtr = exitPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = exitPtr->nextPtr;
	    }
	    ckfree(exitPtr);
	    break;
	}
    }
    Tcl_MutexUnlock(&exitMutex);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TkCreateThreadExitHandler --
 *
 *	Same as Tcl_CreateThreadExitHandler, but private to Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc will be invoked with clientData as argument when the application
 *	exits.
 *
 *----------------------------------------------------------------------
 */

void
TkCreateThreadExitHandler(
    Tcl_ExitProc *proc,		/* Function to invoke. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    exitPtr = ckalloc(sizeof(ExitHandler));
    exitPtr->proc = proc;
    exitPtr->clientData = clientData;

    /*
     * See comments in TkCreateExitHandler().
     */

    if (tsdPtr->firstExitPtr == NULL/* && !TclInExit()*/) {
	Tcl_CreateThreadExitHandler(TkFinalizeThread, NULL);
    }
    exitPtr->nextPtr = tsdPtr->firstExitPtr;
    tsdPtr->firstExitPtr = exitPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDeleteThreadExitHandler --
 *
 *	Same as Tcl_DeleteThreadExitHandler, but private to Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is an exit handler corresponding to proc and clientData then
 *	it is cancelled; if no such handler exists then nothing happens.
 *
 *----------------------------------------------------------------------
 */

void
TkDeleteThreadExitHandler(
    Tcl_ExitProc *proc,		/* Function that was previously registered. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr, *prevPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (prevPtr = NULL, exitPtr = tsdPtr->firstExitPtr; exitPtr != NULL;
	    prevPtr = exitPtr, exitPtr = exitPtr->nextPtr) {
	if ((exitPtr->proc == proc)
		&& (exitPtr->clientData == clientData)) {
	    if (prevPtr == NULL) {
		tsdPtr->firstExitPtr = exitPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = exitPtr->nextPtr;
	    }
	    ckfree(exitPtr);
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkFinalize --
 *
 *	Runs our private exit handlers and removes itself from Tcl. This is
 *	benificial should we want to protect from dangling pointers should the
 *	Tk shared library be unloaded prior to Tcl which can happen on windows
 *	should the process be forcefully exiting from an exception handler.
 *
 * Results:
 *	None.
 *
 * Side effects.
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkFinalize(
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr;

#if defined(_WIN32) && !defined(STATIC_BUILD)
    if (!tclStubsPtr) {
	return;
    }
#endif

    Tcl_DeleteExitHandler(TkFinalize, NULL);

    Tcl_MutexLock(&exitMutex);
    for (exitPtr = firstExitPtr; exitPtr != NULL; exitPtr = firstExitPtr) {
	/*
	 * Be careful to remove the handler from the list before invoking its
	 * callback. This protects us against double-freeing if the callback
	 * should call TkDeleteExitHandler on itself.
	 */

	firstExitPtr = exitPtr->nextPtr;
	Tcl_MutexUnlock(&exitMutex);
	exitPtr->proc(exitPtr->clientData);
	ckfree(exitPtr);
	Tcl_MutexLock(&exitMutex);
    }
    firstExitPtr = NULL;
    Tcl_MutexUnlock(&exitMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TkFinalizeThread --
 *
 *	Runs our private thread exit handlers and removes itself from Tcl.
 *	This is beneficial should we want to protect from dangling pointers
 *	should the Tk shared library be unloaded prior to Tcl which can happen
 *	on Windows should the process be forcefully exiting from an exception
 *	handler.
 *
 * Results:
 *	None.
 *
 * Side effects.
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkFinalizeThread(
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    Tcl_DeleteThreadExitHandler(TkFinalizeThread, NULL);

    if (tsdPtr != NULL) {
	tsdPtr->inExit = 1;

	for (exitPtr = tsdPtr->firstExitPtr; exitPtr != NULL;
		exitPtr = tsdPtr->firstExitPtr) {
	    /*
	     * Be careful to remove the handler from the list before invoking
	     * its callback. This protects us against double-freeing if the
	     * callback should call TkDeleteThreadExitHandler on itself.
	     */

	    tsdPtr->firstExitPtr = exitPtr->nextPtr;
	    exitPtr->proc(exitPtr->clientData);
	    ckfree(exitPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MainLoop --
 *
 *	Call Tcl_DoOneEvent over and over again in an infinite loop as long as
 *	there exist any main windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arbitrary; depends on handlers for events.
 *
 *----------------------------------------------------------------------
 */

void
Tk_MainLoop(void)
{
    while (Tk_GetNumMainWindows() > 0) {
	Tcl_DoOneEvent(0);
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
