/*
 * tkFocus.c --
 *
 *	This file contains functions that manage the input focus for Tk.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * For each top-level window that has ever received the focus, there is a
 * record of the following type:
 */

typedef struct TkToplevelFocusInfo {
    TkWindow *topLevelPtr;	/* Information about top-level window. */
    TkWindow *focusWinPtr;	/* The next time the focus comes to this
				 * top-level, it will be given to this
				 * window. */
    struct TkToplevelFocusInfo *nextPtr;
				/* Next in list of all toplevel focus records
				 * for a given application. */
} ToplevelFocusInfo;

/*
 * One of the following structures exists for each display used by each
 * application. These are linked together from the TkMainInfo structure.
 * These structures are needed because it isn't sufficient to store a single
 * piece of focus information in each display or in each application: we need
 * the cross-product. There needs to be separate information for each display,
 * because it's possible to have multiple focus windows active simultaneously
 * on different displays. There also needs to be separate information for each
 * application, because of embedding: if an embedded application has the
 * focus, its container application also has the focus. Thus we keep a list of
 * structures for each application: the same display can appear in structures
 * for several applications at once.
 */

typedef struct TkDisplayFocusInfo {
    TkDisplay *dispPtr;		/* Display that this information pertains
				 * to. */
    struct TkWindow *focusWinPtr;
				/* Window that currently has the focus for
				 * this application on this display, or NULL
				 * if none. */
    struct TkWindow *focusOnMapPtr;
				/* This points to a toplevel window that is
				 * supposed to receive the X input focus as
				 * soon as it is mapped (needed to handle the
				 * fact that X won't allow the focus on an
				 * unmapped window). NULL means no delayed
				 * focus op in progress for this display. */
    int forceFocus;		/* Associated with focusOnMapPtr: non-zero
				 * means claim the focus even if some other
				 * application currently has it. */
    unsigned long focusSerial;	/* Serial number of last request this
				 * application made to change the focus on
				 * this display. Used to identify stale focus
				 * notifications coming from the X server. */
    struct TkDisplayFocusInfo *nextPtr;
				/* Next in list of all display focus records
				 * for a given application. */
} DisplayFocusInfo;

/*
 * Debugging support...
 */

#define DEBUG(dispPtr, arguments) \
    if ((dispPtr)->focusDebug) { \
	printf arguments; \
    }

/*
 * Forward declarations for functions defined in this file:
 */

static DisplayFocusInfo*FindDisplayFocusInfo(TkMainInfo *mainPtr,
			    TkDisplay *dispPtr);
static void		FocusMapProc(ClientData clientData, XEvent *eventPtr);
static void		GenerateFocusEvents(TkWindow *sourcePtr,
			    TkWindow *destPtr);

/*
 *--------------------------------------------------------------
 *
 * Tk_FocusObjCmd --
 *
 *	This function is invoked to process the "focus" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_FocusObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const focusOptions[] = {
	"-displayof", "-force", "-lastfor", NULL
    };
    Tk_Window tkwin = clientData;
    TkWindow *winPtr = clientData;
    TkWindow *newPtr, *topLevelPtr;
    ToplevelFocusInfo *tlFocusPtr;
    const char *windowName;
    int index;

    /*
     * If invoked with no arguments, just return the current focus window.
     */

    if (objc == 1) {
	Tk_Window focusWin = (Tk_Window) TkGetFocusWin(winPtr);

	if (focusWin != NULL) {
	    Tcl_SetObjResult(interp, TkNewWindowObj(focusWin));
	}
	return TCL_OK;
    }

    /*
     * If invoked with a single argument beginning with "." then focus on that
     * window.
     */

    if (objc == 2) {
	windowName = Tcl_GetString(objv[1]);

	/*
	 * The empty string case exists for backwards compatibility.
	 */

	if (windowName[0] == '\0') {
	    return TCL_OK;
	}
	if (windowName[0] == '.') {
	    newPtr = (TkWindow *) Tk_NameToWindow(interp, windowName, tkwin);
	    if (newPtr == NULL) {
		return TCL_ERROR;
	    }
	    TkSetFocusWin(newPtr, 0);
	    return TCL_OK;
	}
    }

    /*
     * We have a subcommand to parse and act upon.
     */

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], focusOptions,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
    	return TCL_ERROR;
    }
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    switch (index) {
    case 0:			/* -displayof */
	windowName = Tcl_GetString(objv[2]);
	newPtr = (TkWindow *) Tk_NameToWindow(interp, windowName, tkwin);
	if (newPtr == NULL) {
	    return TCL_ERROR;
	}
	newPtr = TkGetFocusWin(newPtr);
	if (newPtr != NULL) {
	    Tcl_SetObjResult(interp, TkNewWindowObj((Tk_Window) newPtr));
	}
	break;
    case 1:			/* -force */
	windowName = Tcl_GetString(objv[2]);

	/*
	 * The empty string case exists for backwards compatibility.
	 */

	if (windowName[0] == '\0') {
	    return TCL_OK;
	}
	newPtr = (TkWindow *) Tk_NameToWindow(interp, windowName, tkwin);
	if (newPtr == NULL) {
	    return TCL_ERROR;
	}
	TkSetFocusWin(newPtr, 1);
	break;
    case 2:			/* -lastfor */
	windowName = Tcl_GetString(objv[2]);
	newPtr = (TkWindow *) Tk_NameToWindow(interp, windowName, tkwin);
	if (newPtr == NULL) {
	    return TCL_ERROR;
	}
	for (topLevelPtr = newPtr; topLevelPtr != NULL;
		topLevelPtr = topLevelPtr->parentPtr) {
	    if (!(topLevelPtr->flags & TK_TOP_HIERARCHY)) {
		continue;
	    }
	    for (tlFocusPtr = newPtr->mainPtr->tlFocusPtr; tlFocusPtr != NULL;
		    tlFocusPtr = tlFocusPtr->nextPtr) {
		if (tlFocusPtr->topLevelPtr == topLevelPtr) {
		    Tcl_SetObjResult(interp, TkNewWindowObj((Tk_Window)
			    tlFocusPtr->focusWinPtr));
		    return TCL_OK;
		}
	    }
	    Tcl_SetObjResult(interp, TkNewWindowObj((Tk_Window) topLevelPtr));
	    return TCL_OK;
	}
	break;
    default:
	Tcl_Panic("bad const entries to focusOptions in focus command");
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkFocusFilterEvent --
 *
 *	This function is invoked by Tk_HandleEvent when it encounters a
 *	FocusIn, FocusOut, Enter, or Leave event.
 *
 * Results:
 *	A return value of 1 means that Tk_HandleEvent should process the event
 *	normally (i.e. event handlers should be invoked). A return value of 0
 *	means that this event should be ignored.
 *
 * Side effects:
 *	Additional events may be generated, and the focus may switch.
 *
 *--------------------------------------------------------------
 */

int
TkFocusFilterEvent(
    TkWindow *winPtr,		/* Window that focus event is directed to. */
    XEvent *eventPtr)		/* FocusIn, FocusOut, Enter, or Leave
				 * event. */
{
    /*
     * Design notes: the window manager and X server work together to transfer
     * the focus among top-level windows. This function takes care of
     * transferring the focus from a top-level or wrapper window to the actual
     * window within that top-level that has the focus. We do this by
     * synthesizing X events to move the focus around. None of the FocusIn and
     * FocusOut events generated by X are ever used outside of this function;
     * only the synthesized events get through to the rest of the application.
     * At one point (e.g. Tk4.0b1) Tk used to call X to move the focus from a
     * top-level to one of its descendants, then just pass through the events
     * generated by X. This approach didn't work very well, for a variety of
     * reasons. For example, if X generates the events they go at the back of
     * the event queue, which could cause problems if other things have
     * already happened, such as moving the focus to yet another window.
     */

    ToplevelFocusInfo *tlFocusPtr;
    DisplayFocusInfo *displayFocusPtr;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkWindow *newFocusPtr;
    int retValue, delta;

    /*
     * If this was a generated event, just turn off the generated flag and
     * pass the event through to Tk bindings.
     */

    if (eventPtr->xfocus.send_event == GENERATED_FOCUS_EVENT_MAGIC) {
	eventPtr->xfocus.send_event = 0;
	return 1;
    }

    /*
     * Check for special events generated by embedded applications to request
     * the input focus. If this is one of those events, make the change in
     * focus and return without any additional processing of the event (note:
     * the "detail" field of the event indicates whether to claim the focus
     * even if we don't already have it).
     */

    if ((eventPtr->xfocus.mode == EMBEDDED_APP_WANTS_FOCUS)
	    && (eventPtr->type == FocusIn)) {
	TkSetFocusWin(winPtr, eventPtr->xfocus.detail);
	return 0;
    }

    /*
     * This was not a generated event. We'll return 1 (so that the event will
     * be processed) if it's an Enter or Leave event, and 0 (so that the event
     * won't be processed) if it's a FocusIn or FocusOut event.
     */

    retValue = 0;
    displayFocusPtr = FindDisplayFocusInfo(winPtr->mainPtr, winPtr->dispPtr);
    if (eventPtr->type == FocusIn) {
	/*
	 * Skip FocusIn events that cause confusion
	 * NotifyVirtual and NotifyNonlinearVirtual - Virtual events occur on
	 *	windows in between the origin and destination of the focus
	 *	change. For FocusIn we may see this when focus goes into an
	 *	embedded child. We don't care about this, although we may end
	 *	up getting a NotifyPointer later.
	 * NotifyInferior - focus is coming to us from an embedded child. When
	 *	focus is on an embeded focus, we still think we have the
	 *	focus, too, so this message doesn't change our state.
	 * NotifyPointerRoot - should never happen because this is sent to the
	 *	root window.
	 *
	 * Interesting FocusIn events are
	 * NotifyAncestor - focus is coming from our parent, probably the root.
	 * NotifyNonlinear - focus is coming from a different branch, probably
	 *	another toplevel.
	 * NotifyPointer - implicit focus because of the mouse position. This
	 *	is only interesting on toplevels, when it means that the focus
	 *	has been set to the root window but the mouse is over this
	 *	toplevel. We take the focus implicitly (probably no window
	 *	manager)
	 */

	if ((eventPtr->xfocus.detail == NotifyVirtual)
		|| (eventPtr->xfocus.detail == NotifyNonlinearVirtual)
		|| (eventPtr->xfocus.detail == NotifyPointerRoot)
		|| (eventPtr->xfocus.detail == NotifyInferior)) {
	    return retValue;
	}
    } else if (eventPtr->type == FocusOut) {
	/*
	 * Skip FocusOut events that cause confusion.
	 * NotifyPointer - the pointer is in us or a child, and we are losing
	 *	focus because of an XSetInputFocus. Other focus events will
	 *	set our state properly.
	 * NotifyPointerRoot - should never happen because this is sent to the
	 *	root window.
	 * NotifyInferior - focus leaving us for an embedded child. We retain
	 *	a notion of focus when an embedded child has focus.
	 *
	 * Interesting events are:
	 * NotifyAncestor - focus is going to root.
	 * NotifyNonlinear - focus is going to another branch, probably
	 *	another toplevel.
	 * NotifyVirtual, NotifyNonlinearVirtual - focus is passing through,
	 *	and we need to make sure we track this.
	 */

	if ((eventPtr->xfocus.detail == NotifyPointer)
		|| (eventPtr->xfocus.detail == NotifyPointerRoot)
		|| (eventPtr->xfocus.detail == NotifyInferior)) {
	    return retValue;
	}
    } else {
	retValue = 1;
	if (eventPtr->xcrossing.detail == NotifyInferior) {
	    return retValue;
	}
    }

    /*
     * If winPtr isn't a top-level window than just ignore the event.
     */

    winPtr = TkWmFocusToplevel(winPtr);
    if (winPtr == NULL) {
	return retValue;
    }

    /*
     * If there is a grab in effect and this window is outside the grabbed
     * tree, then ignore the event.
     */

    if (TkGrabState(winPtr) == TK_GRAB_EXCLUDED) {
	return retValue;
    }

    /*
     * It is possible that there were outstanding FocusIn and FocusOut events
     * on their way to us at the time the focus was changed internally with
     * the "focus" command. If so, these events could potentially cause us to
     * lose the focus (switch it to the window of the last FocusIn event) even
     * though the focus change occurred after those events. The following code
     * detects this and ignores the stale events.
     *
     * Note: the focusSerial is only generated by TkpChangeFocus, whereas in
     * Tk 4.2 there was always a nop marker generated.
     */

    delta = eventPtr->xfocus.serial - displayFocusPtr->focusSerial;
    if (delta < 0) {
	return retValue;
    }

    /*
     * Find the ToplevelFocusInfo structure for the window, and make a new one
     * if there isn't one already.
     */

    for (tlFocusPtr = winPtr->mainPtr->tlFocusPtr; tlFocusPtr != NULL;
	    tlFocusPtr = tlFocusPtr->nextPtr) {
	if (tlFocusPtr->topLevelPtr == winPtr) {
	    break;
	}
    }
    if (tlFocusPtr == NULL) {
	tlFocusPtr = ckalloc(sizeof(ToplevelFocusInfo));
	tlFocusPtr->topLevelPtr = tlFocusPtr->focusWinPtr = winPtr;
	tlFocusPtr->nextPtr = winPtr->mainPtr->tlFocusPtr;
	winPtr->mainPtr->tlFocusPtr = tlFocusPtr;
    }
    newFocusPtr = tlFocusPtr->focusWinPtr;

    /*
     * Ignore event if newFocus window is already dead!
     */

    if (newFocusPtr->flags & TK_ALREADY_DEAD) {
	return retValue;
    }

    if (eventPtr->type == FocusIn) {
	GenerateFocusEvents(displayFocusPtr->focusWinPtr, newFocusPtr);
	displayFocusPtr->focusWinPtr = newFocusPtr;
	dispPtr->focusPtr = newFocusPtr;

	/*
	 * NotifyPointer gets set when the focus has been set to the root
	 * window but we have the pointer. We'll treat this like an implicit
	 * focus in event so that upon Leave events we release focus.
	 */

	if (!(winPtr->flags & TK_EMBEDDED)) {
	    if (eventPtr->xfocus.detail == NotifyPointer) {
		dispPtr->implicitWinPtr = winPtr;
	    } else {
		dispPtr->implicitWinPtr = NULL;
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	GenerateFocusEvents(displayFocusPtr->focusWinPtr, NULL);

	/*
	 * Reset dispPtr->focusPtr, but only if it currently is the same as
	 * this application's focusWinPtr: this check is needed to handle
	 * embedded applications in the same process.
	 */

	if (dispPtr->focusPtr == displayFocusPtr->focusWinPtr) {
	    dispPtr->focusPtr = NULL;
	}
	displayFocusPtr->focusWinPtr = NULL;
    } else if (eventPtr->type == EnterNotify) {
	/*
	 * If there is no window manager, or if the window manager isn't
	 * moving the focus around (e.g. the disgusting "NoTitleFocus" option
	 * has been selected in twm), then we won't get FocusIn or FocusOut
	 * events. Instead, the "focus" field will be set in an Enter event to
	 * indicate that we've already got the focus when the mouse enters the
	 * window (even though we didn't get a FocusIn event). Watch for this
	 * and grab the focus when it happens. Note: if this is an embedded
	 * application then don't accept the focus implicitly like this; the
	 * container application will give us the focus explicitly if it wants
	 * us to have it.
	 */

	if (eventPtr->xcrossing.focus &&
		(displayFocusPtr->focusWinPtr == NULL)
		&& !(winPtr->flags & TK_EMBEDDED)) {
	    DEBUG(dispPtr,
		    ("Focussed implicitly on %s\n", newFocusPtr->pathName));

	    GenerateFocusEvents(displayFocusPtr->focusWinPtr, newFocusPtr);
	    displayFocusPtr->focusWinPtr = newFocusPtr;
	    dispPtr->implicitWinPtr = winPtr;
	    dispPtr->focusPtr = newFocusPtr;
	}
    } else if (eventPtr->type == LeaveNotify) {
	/*
	 * If the pointer just left a window for which we automatically
	 * claimed the focus on enter, move the focus back to the root window,
	 * where it was before we claimed it above. Note:
	 * dispPtr->implicitWinPtr may not be the same as
	 * displayFocusPtr->focusWinPtr (e.g. because the "focus" command was
	 * used to redirect the focus after it arrived at
	 * dispPtr->implicitWinPtr)!! In addition, we generate events because
	 * the window manager won't give us a FocusOut event when we focus on
	 * the root.
	 */

	if ((dispPtr->implicitWinPtr != NULL)
		&& !(winPtr->flags & TK_EMBEDDED)) {
	    DEBUG(dispPtr, ("Defocussed implicit Async\n"));
	    GenerateFocusEvents(displayFocusPtr->focusWinPtr, NULL);
	    XSetInputFocus(dispPtr->display, PointerRoot, RevertToPointerRoot,
		    CurrentTime);
	    displayFocusPtr->focusWinPtr = NULL;
	    dispPtr->implicitWinPtr = NULL;
	}
    }
    return retValue;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetFocusWin --
 *
 *	This function is invoked to change the focus window for a given
 *	display in a given application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Event handlers may be invoked to process the change of
 *	focus.
 *
 *----------------------------------------------------------------------
 */

void
TkSetFocusWin(
    TkWindow *winPtr,		/* Window that is to be the new focus for its
				 * display and application. */
    int force)			/* If non-zero, set the X focus to this window
				 * even if the application doesn't currently
				 * have the X focus. */
{
    ToplevelFocusInfo *tlFocusPtr;
    DisplayFocusInfo *displayFocusPtr;
    TkWindow *topLevelPtr;
    int allMapped, serial;

    /*
     * Don't set focus if window is already dead. [Bug 3574708]
     */

    if (winPtr->flags & TK_ALREADY_DEAD) {
	return;
    }

    displayFocusPtr = FindDisplayFocusInfo(winPtr->mainPtr, winPtr->dispPtr);

    /*
     * If force is set, we should make sure we grab the focus regardless of
     * the current focus window since under Windows, we may need to take
     * control away from another application.
     */

    if (winPtr == displayFocusPtr->focusWinPtr && !force) {
	return;
    }

    /*
     * Find the top-level window for winPtr, then find (or create) a record
     * for the top-level. Also see whether winPtr and all its ancestors are
     * mapped.
     */

    allMapped = 1;
    for (topLevelPtr = winPtr; ; topLevelPtr = topLevelPtr->parentPtr) {
	if (topLevelPtr == NULL) {
	    /*
	     * The window is being deleted. No point in worrying about giving
	     * it the focus.
	     */

	    return;
	}
	if (!(topLevelPtr->flags & TK_MAPPED)) {
	    allMapped = 0;
	}
	if (topLevelPtr->flags & TK_TOP_HIERARCHY) {
	    break;
	}
    }

    /*
     * If the new focus window isn't mapped, then we can't focus on it (X will
     * generate an error, for example). Instead, create an event handler that
     * will set the focus to this window once it gets mapped. At the same
     * time, delete any old handler that might be around; it's no longer
     * relevant.
     */

    if (displayFocusPtr->focusOnMapPtr != NULL) {
	Tk_DeleteEventHandler((Tk_Window) displayFocusPtr->focusOnMapPtr,
		StructureNotifyMask, FocusMapProc,
		displayFocusPtr->focusOnMapPtr);
	displayFocusPtr->focusOnMapPtr = NULL;
    }
    if (!allMapped) {
	Tk_CreateEventHandler((Tk_Window) winPtr, VisibilityChangeMask,
		FocusMapProc, winPtr);
	displayFocusPtr->focusOnMapPtr = winPtr;
	displayFocusPtr->forceFocus = force;
	return;
    }

    for (tlFocusPtr = winPtr->mainPtr->tlFocusPtr; tlFocusPtr != NULL;
	    tlFocusPtr = tlFocusPtr->nextPtr) {
	if (tlFocusPtr->topLevelPtr == topLevelPtr) {
	    break;
	}
    }
    if (tlFocusPtr == NULL) {
	tlFocusPtr = ckalloc(sizeof(ToplevelFocusInfo));
	tlFocusPtr->topLevelPtr = topLevelPtr;
	tlFocusPtr->nextPtr = winPtr->mainPtr->tlFocusPtr;
	winPtr->mainPtr->tlFocusPtr = tlFocusPtr;
    }
    tlFocusPtr->focusWinPtr = winPtr;

    /*
     * Reset the window system's focus window and generate focus events, with
     * two special cases:
     *
     * 1. If the application is embedded and doesn't currently have the focus,
     *    don't set the focus directly. Instead, see if the embedding code can
     *    claim the focus from the enclosing container.
     * 2. Otherwise, if the application doesn't currently have the focus,
     *    don't change the window system's focus unless it was already in this
     *    application or "force" was specified.
     */

    if ((topLevelPtr->flags & TK_EMBEDDED)
	    && (displayFocusPtr->focusWinPtr == NULL)) {
	TkpClaimFocus(topLevelPtr, force);
    } else if ((displayFocusPtr->focusWinPtr != NULL) || force) {
	/*
	 * Generate events to shift focus between Tk windows. We do this
	 * regardless of what TkpChangeFocus does with the real X focus so
	 * that Tk widgets track focus commands when there is no window
	 * manager. GenerateFocusEvents will set up a serial number marker so
	 * we discard focus events that are triggered by the ChangeFocus.
	 */

	serial = TkpChangeFocus(TkpGetWrapperWindow(topLevelPtr), force);
	if (serial != 0) {
	    displayFocusPtr->focusSerial = serial;
	}
	GenerateFocusEvents(displayFocusPtr->focusWinPtr, winPtr);
	displayFocusPtr->focusWinPtr = winPtr;
	winPtr->dispPtr->focusPtr = winPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetFocusWin --
 *
 *	Given a window, this function returns the current focus window for its
 *	application and display.
 *
 * Results:
 *	The return value is a pointer to the window that currently has the
 *	input focus for the specified application and display, or NULL if
 *	none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkGetFocusWin(
    TkWindow *winPtr)		/* Window that selects an application and a
				 * display. */
{
    DisplayFocusInfo *displayFocusPtr;

    if (winPtr == NULL) {
	return NULL;
    }

    displayFocusPtr = FindDisplayFocusInfo(winPtr->mainPtr, winPtr->dispPtr);
    return displayFocusPtr->focusWinPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFocusKeyEvent --
 *
 *	Given a window and a key press or release event that arrived for the
 *	window, use information about the keyboard focus to compute which
 *	window should really get the event. In addition, update the event to
 *	refer to its new window.
 *
 * Results:
 *	The return value is a pointer to the window that has the input focus
 *	in winPtr's application, or NULL if winPtr's application doesn't have
 *	the input focus. If a non-NULL value is returned, eventPtr will be
 *	updated to refer properly to the focus window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkFocusKeyEvent(
    TkWindow *winPtr,		/* Window that selects an application and a
				 * display. */
    XEvent *eventPtr)		/* X event to redirect (should be KeyPress or
				 * KeyRelease). */
{
    DisplayFocusInfo *displayFocusPtr;
    TkWindow *focusWinPtr;
    int focusX, focusY;

    displayFocusPtr = FindDisplayFocusInfo(winPtr->mainPtr, winPtr->dispPtr);
    focusWinPtr = displayFocusPtr->focusWinPtr;

    /*
     * The code below is a debugging aid to make sure that dispPtr->focusPtr
     * is kept properly in sync with the "truth", which is the value in
     * displayFocusPtr->focusWinPtr.
     */

#ifdef TCL_MEM_DEBUG
    if (focusWinPtr != winPtr->dispPtr->focusPtr) {
	printf("TkFocusKeyEvent found dispPtr->focusPtr out of sync:\n");
	printf("expected %s, got %s\n",
		(focusWinPtr != NULL) ? focusWinPtr->pathName : "??",
		(winPtr->dispPtr->focusPtr != NULL) ?
		winPtr->dispPtr->focusPtr->pathName : "??");
    }
#endif

    if ((focusWinPtr != NULL) && (focusWinPtr->mainPtr == winPtr->mainPtr)) {
	/*
	 * Map the x and y coordinates to make sense in the context of the
	 * focus window, if possible (make both -1 if the map-from and map-to
	 * windows don't share the same screen).
	 */

	if ((focusWinPtr->display != winPtr->display)
		|| (focusWinPtr->screenNum != winPtr->screenNum)) {
	    eventPtr->xkey.x = -1;
	    eventPtr->xkey.y = -1;
	} else {
	    Tk_GetRootCoords((Tk_Window) focusWinPtr, &focusX, &focusY);
	    eventPtr->xkey.x = eventPtr->xkey.x_root - focusX;
	    eventPtr->xkey.y = eventPtr->xkey.y_root - focusY;
	}
	eventPtr->xkey.window = focusWinPtr->window;
	return focusWinPtr;
    }

    /*
     * The event doesn't belong to us. Perhaps, due to embedding, it really
     * belongs to someone else. Give the embedding code a chance to redirect
     * the event.
     */

    TkpRedirectKeyEvent(winPtr, eventPtr);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFocusDeadWindow --
 *
 *	This function is invoked when it is determined that a window is dead.
 *	It cleans up focus-related information about the window.
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
TkFocusDeadWindow(
    register TkWindow *winPtr)	/* Information about the window that is being
				 * deleted. */
{
    ToplevelFocusInfo *tlFocusPtr, *prevPtr;
    DisplayFocusInfo *displayFocusPtr;
    TkDisplay *dispPtr = winPtr->dispPtr;

    /*
     * Certain special windows like those used for send and clipboard have no
     * mainPtr.
     */

    if (winPtr->mainPtr == NULL) {
	return;
    }

    /*
     * Search for focus records that refer to this window either as the
     * top-level window or the current focus window.
     */

    displayFocusPtr = FindDisplayFocusInfo(winPtr->mainPtr, winPtr->dispPtr);
    for (prevPtr = NULL, tlFocusPtr = winPtr->mainPtr->tlFocusPtr;
	    tlFocusPtr != NULL;
	    prevPtr = tlFocusPtr, tlFocusPtr = tlFocusPtr->nextPtr) {
	if (winPtr == tlFocusPtr->topLevelPtr) {
	    /*
	     * The top-level window is the one being deleted: free the focus
	     * record and release the focus back to PointerRoot if we acquired
	     * it implicitly.
	     */

	    if (dispPtr->implicitWinPtr == winPtr) {
		DEBUG(dispPtr, ("releasing focus to root after %s died\n",
			tlFocusPtr->topLevelPtr->pathName));
		dispPtr->implicitWinPtr = NULL;
		displayFocusPtr->focusWinPtr = NULL;
		dispPtr->focusPtr = NULL;
	    }
	    if (displayFocusPtr->focusWinPtr == tlFocusPtr->focusWinPtr) {
		displayFocusPtr->focusWinPtr = NULL;
		dispPtr->focusPtr = NULL;
	    }
	    if (prevPtr == NULL) {
		winPtr->mainPtr->tlFocusPtr = tlFocusPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = tlFocusPtr->nextPtr;
	    }
	    ckfree(tlFocusPtr);
	    break;
	} else if (winPtr == tlFocusPtr->focusWinPtr) {
	    /*
	     * The deleted window had the focus for its top-level: move the
	     * focus to the top-level itself.
	     */

	    tlFocusPtr->focusWinPtr = tlFocusPtr->topLevelPtr;
	    if ((displayFocusPtr->focusWinPtr == winPtr)
		    && !(tlFocusPtr->topLevelPtr->flags & TK_ALREADY_DEAD)) {
		DEBUG(dispPtr, ("forwarding focus to %s after %s died\n",
			tlFocusPtr->topLevelPtr->pathName, winPtr->pathName));
		GenerateFocusEvents(displayFocusPtr->focusWinPtr,
			tlFocusPtr->topLevelPtr);
		displayFocusPtr->focusWinPtr = tlFocusPtr->topLevelPtr;
		dispPtr->focusPtr = tlFocusPtr->topLevelPtr;
	    }
	    break;
	}
    }

    /*
     * Occasionally, things can become unsynchronized. Move them back into
     * synch now. [Bug 2496114]
     */

    if (displayFocusPtr->focusWinPtr == winPtr) {
	DEBUG(dispPtr, ("focus cleared after %s died\n", winPtr->pathName));
	displayFocusPtr->focusWinPtr = NULL;
    }

    if (displayFocusPtr->focusOnMapPtr == winPtr) {
	displayFocusPtr->focusOnMapPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateFocusEvents --
 *
 *	This function is called to create FocusIn and FocusOut events to move
 *	the input focus from one window to another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	FocusIn and FocusOut events are generated.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateFocusEvents(
    TkWindow *sourcePtr,	/* Window that used to have the focus (may be
				 * NULL). */
    TkWindow *destPtr)		/* New window to have the focus (may be
				 * NULL). */
{
    XEvent event;
    TkWindow *winPtr;

    winPtr = sourcePtr;
    if (winPtr == NULL) {
	winPtr = destPtr;
	if (winPtr == NULL) {
	    return;
	}
    }

    event.xfocus.serial = LastKnownRequestProcessed(winPtr->display);
    event.xfocus.send_event = GENERATED_FOCUS_EVENT_MAGIC;
    event.xfocus.display = winPtr->display;
    event.xfocus.mode = NotifyNormal;
    TkInOutEvents(&event, sourcePtr, destPtr, FocusOut, FocusIn,
	    TCL_QUEUE_MARK);
}

/*
 *----------------------------------------------------------------------
 *
 * FocusMapProc --
 *
 *	This function is called as an event handler for VisibilityNotify
 *	events, if a window receives the focus at a time when its toplevel
 *	isn't mapped. The function is needed because X won't allow the focus
 *	to be set to an unmapped window; we detect when the toplevel is mapped
 *	and set the focus to it then.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If this is a map event, the focus gets set to the toplevel given by
 *	clientData.
 *
 *----------------------------------------------------------------------
 */

static void
FocusMapProc(
    ClientData clientData,	/* Toplevel window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkWindow *winPtr = clientData;
    DisplayFocusInfo *displayFocusPtr;

    if (eventPtr->type == VisibilityNotify) {
	displayFocusPtr = FindDisplayFocusInfo(winPtr->mainPtr,
		winPtr->dispPtr);
	DEBUG(winPtr->dispPtr, ("auto-focussing on %s, force %d\n",
		winPtr->pathName, displayFocusPtr->forceFocus));
	Tk_DeleteEventHandler((Tk_Window) winPtr, VisibilityChangeMask,
		FocusMapProc, clientData);
	displayFocusPtr->focusOnMapPtr = NULL;
	TkSetFocusWin(winPtr, displayFocusPtr->forceFocus);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FindDisplayFocusInfo --
 *
 *	Given an application and a display, this function locate the focus
 *	record for that combination. If no such record exists, it creates a
 *	new record and initializes it.
 *
 * Results:
 *	The return value is a pointer to the record.
 *
 * Side effects:
 *	A new record will be allocated if there wasn't one already.
 *
 *----------------------------------------------------------------------
 */

static DisplayFocusInfo *
FindDisplayFocusInfo(
    TkMainInfo *mainPtr,	/* Record that identifies a particular
				 * application. */
    TkDisplay *dispPtr)		/* Display whose focus information is
				 * needed. */
{
    DisplayFocusInfo *displayFocusPtr;

    for (displayFocusPtr = mainPtr->displayFocusPtr;
	    displayFocusPtr != NULL;
	    displayFocusPtr = displayFocusPtr->nextPtr) {
	if (displayFocusPtr->dispPtr == dispPtr) {
	    return displayFocusPtr;
	}
    }

    /*
     * The record doesn't exist yet. Make a new one.
     */

    displayFocusPtr = ckalloc(sizeof(DisplayFocusInfo));
    displayFocusPtr->dispPtr = dispPtr;
    displayFocusPtr->focusWinPtr = NULL;
    displayFocusPtr->focusOnMapPtr = NULL;
    displayFocusPtr->forceFocus = 0;
    displayFocusPtr->focusSerial = 0;
    displayFocusPtr->nextPtr = mainPtr->displayFocusPtr;
    mainPtr->displayFocusPtr = displayFocusPtr;
    return displayFocusPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFocusFree --
 *
 *	Free resources associated with maintaining the focus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This mainPtr should no long access focus information.
 *
 *----------------------------------------------------------------------
 */

void
TkFocusFree(
    TkMainInfo *mainPtr)	/* Record that identifies a particular
				 * application. */
{
    while (mainPtr->displayFocusPtr != NULL) {
	DisplayFocusInfo *displayFocusPtr = mainPtr->displayFocusPtr;

	mainPtr->displayFocusPtr = mainPtr->displayFocusPtr->nextPtr;
	ckfree(displayFocusPtr);
    }
    while (mainPtr->tlFocusPtr != NULL) {
	ToplevelFocusInfo *tlFocusPtr = mainPtr->tlFocusPtr;

	mainPtr->tlFocusPtr = mainPtr->tlFocusPtr->nextPtr;
	ckfree(tlFocusPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkFocusSplit --
 *
 *	Adjust focus window for a newly managed toplevel, thus splitting the
 *	toplevel into two toplevels.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new record is allocated for the new toplevel window.
 *
 *----------------------------------------------------------------------
 */

void
TkFocusSplit(
    TkWindow *winPtr)		/* Window is the new toplevel. Any focus point
				 * at or below window must be moved to this
				 * new toplevel. */
{
    ToplevelFocusInfo *tlFocusPtr;
    TkWindow *topLevelPtr, *subWinPtr;

    FindDisplayFocusInfo(winPtr->mainPtr, winPtr->dispPtr);

    /*
     * Find the top-level window for winPtr, then find (or create) a record
     * for the top-level. Also see whether winPtr and all its ancestors are
     * mapped.
     */

    for (topLevelPtr = winPtr; ; topLevelPtr = topLevelPtr->parentPtr) {
	if (topLevelPtr == NULL) {
	    /*
	     * The window is being deleted. No point in worrying about giving
	     * it the focus.
	     */

	    return;
	}
	if (topLevelPtr->flags & TK_TOP_HIERARCHY) {
	    break;
	}
    }

    /*
     * Search all focus records to find child windows of winPtr.
     */

    for (tlFocusPtr = winPtr->mainPtr->tlFocusPtr; tlFocusPtr != NULL;
	    tlFocusPtr = tlFocusPtr->nextPtr) {
	if (tlFocusPtr->topLevelPtr == topLevelPtr) {
	    break;
	}
    }

    if (tlFocusPtr == NULL) {
	/*
	 * No focus record for this toplevel, nothing to do.
	 */

	return;
    }

    /*
     * See if current focusWin is child of the new toplevel.
     */

    for (subWinPtr = tlFocusPtr->focusWinPtr;
	    subWinPtr && subWinPtr != winPtr && subWinPtr != topLevelPtr;
	    subWinPtr = subWinPtr->parentPtr) {
	/* EMPTY */
    }

    if (subWinPtr == winPtr) {
	/*
	 * Move focus to new toplevel.
	 */

	ToplevelFocusInfo *newTlFocusPtr = ckalloc(sizeof(ToplevelFocusInfo));

	newTlFocusPtr->topLevelPtr = winPtr;
	newTlFocusPtr->focusWinPtr = tlFocusPtr->focusWinPtr;
	newTlFocusPtr->nextPtr = winPtr->mainPtr->tlFocusPtr;
	winPtr->mainPtr->tlFocusPtr = newTlFocusPtr;

	/*
	 * Move old toplevel's focus to the toplevel itself.
	 */

	tlFocusPtr->focusWinPtr = topLevelPtr;
    }

    /*
     * If it's not, then let focus progress naturally.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkFocusJoin --
 *
 *	Remove the focus record for this window that is nolonger managed
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A tlFocusPtr record is removed
 *
 *----------------------------------------------------------------------
 */

void
TkFocusJoin(
    TkWindow *winPtr)		/* Window is no longer a toplevel. */
{
    ToplevelFocusInfo *tlFocusPtr, *tmpPtr;

    /*
     * Remove old toplevel record
     */

    if (winPtr && winPtr->mainPtr && winPtr->mainPtr->tlFocusPtr
	    && winPtr->mainPtr->tlFocusPtr->topLevelPtr == winPtr) {
	tmpPtr = winPtr->mainPtr->tlFocusPtr;
	winPtr->mainPtr->tlFocusPtr = tmpPtr->nextPtr;
	ckfree(tmpPtr);
    } else if (winPtr && winPtr->mainPtr) {
	for (tlFocusPtr = winPtr->mainPtr->tlFocusPtr; tlFocusPtr != NULL;
		tlFocusPtr = tlFocusPtr->nextPtr) {
	    if (tlFocusPtr->nextPtr &&
		    tlFocusPtr->nextPtr->topLevelPtr == winPtr) {
		tmpPtr = tlFocusPtr->nextPtr;
		tlFocusPtr->nextPtr = tmpPtr->nextPtr;
		ckfree(tmpPtr);
		break;
	    }
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
