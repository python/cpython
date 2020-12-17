/*
 * tkGrab.c --
 *
 *	This file provides functions that implement grabs for Tk.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

#ifdef _WIN32
#include "tkWinInt.h"
#elif !defined(MAC_OSX_TK)
#include "tkUnixInt.h"
#endif

/*
 * The grab state machine has four states: ungrabbed, button pressed, grabbed,
 * and button pressed while grabbed. In addition, there are three pieces of
 * grab state information: the current grab window, the current restrict
 * window, and whether the mouse is captured.
 *
 * The current grab window specifies the point in the Tk window heirarchy
 * above which pointer events will not be reported. Any window within the
 * subtree below the grab window will continue to receive events as normal.
 * Events outside of the grab tree will be reported to the grab window.
 *
 * If the current restrict window is set, then all pointer events will be
 * reported only to the restrict window. The restrict window is normally set
 * during an automatic button grab.
 *
 * The mouse capture state specifies whether the window system will report
 * mouse events outside of any Tk toplevels. This is set during a global grab
 * or an automatic button grab.
 *
 * The transitions between different states is given in the following table:
 *
 * Event\State	U	B	G	GB
 * -----------	--	--	--	--
 * FirstPress	B	B	GB	GB
 * Press	B	B	G	GB
 * Release	U	B	G	GB
 * LastRelease	U	U	G	G
 * Grab		G	G	G	G
 * Ungrab	U	B	U	U
 *
 * Note: U=Ungrabbed, B=Button, G=Grabbed, GB=Grab and Button
 *
 * In addition, the following conditions are always true:
 *
 * State\Variable	Grab	     Restrict	     Capture
 * --------------	----	     --------	     -------
 * Ungrabbed		 0		0		0
 * Button		 0		1		1
 * Grabbed		 1		0		b/g
 * Grab and Button	 1		1		1
 *
 * Note: 0 means variable is set to NULL, 1 means variable is set to some
 * window, b/g means the variable is set to a window if a button is currently
 * down or a global grab is in effect.
 *
 * The final complication to all of this is enter and leave events. In order
 * to correctly handle all of the various cases, Tk cannot rely on X
 * enter/leave events in all situations. The following describes the correct
 * sequence of enter and leave events that should be observed by Tk scripts:
 *
 * Event(state)		Enter/Leave From -> To
 * ------------		----------------------
 * LastRelease(B | GB): restrict window -> anc(grab window, event window)
 * Grab(U | B): 	event window -> anc(grab window, event window)
 * Grab(G):		anc(old grab window, event window) ->
 * 				anc(new grab window, event window)
 * Grab(GB):		restrict window -> anc(new grab window, event window)
 * Ungrab(G):		anc(grab window, event window) -> event window
 * Ungrab(GB):		restrict window -> event window
 *
 * Note: anc(x,y) returns the least ancestor of y that is in the tree of x,
 * terminating at toplevels.
 */

/*
 * The following structure is used to pass information to GrabRestrictProc
 * from EatGrabEvents.
 */

typedef struct {
    Display *display;		/* Display from which to discard events. */
    unsigned int serial;	/* Serial number with which to compare. */
} GrabInfo;

/*
 * Bit definitions for grabFlags field of TkDisplay structures:
 *
 * GRAB_GLOBAL			1 means this is a global grab (we grabbed via
 *				the server so all applications are locked out).
 *				0 means this is a local grab that affects only
 *				this application.
 * GRAB_TEMP_GLOBAL		1 means we've temporarily grabbed via the
 *				server because a button is down and we want to
 *				make sure that we get the button-up event. The
 *				grab will be released when the last mouse
 *				button goes up.
 */

#define GRAB_GLOBAL		1
#define GRAB_TEMP_GLOBAL	4

/*
 * The following structure is a Tcl_Event that triggers a change in the
 * grabWinPtr field of a display. This event guarantees that the change occurs
 * in the proper order relative to enter and leave events.
 */

typedef struct NewGrabWinEvent {
    Tcl_Event header;		/* Standard information for all Tcl events. */
    TkDisplay *dispPtr;		/* Display whose grab window is to change. */
    Window grabWindow;		/* New grab window for display. This is
				 * recorded instead of a (TkWindow *) because
				 * it will allow us to detect cases where the
				 * window is destroyed before this event is
				 * processed. */
} NewGrabWinEvent;

/*
 * The following magic value is stored in the "send_event" field of
 * EnterNotify and LeaveNotify events that are generated in this file. This
 * allows us to separate "real" events coming from the server from those that
 * we generated.
 */

#define GENERATED_GRAB_EVENT_MAGIC ((Bool) 0x147321ac)

/*
 * Forward declarations for functions declared later in this file:
 */

static void		EatGrabEvents(TkDisplay *dispPtr, unsigned int serial);
static TkWindow *	FindCommonAncestor(TkWindow *winPtr1,
			    TkWindow *winPtr2, int *countPtr1, int *countPtr2);
static Tk_RestrictProc GrabRestrictProc;
static int		GrabWinEventProc(Tcl_Event *evPtr, int flags);
static void		MovePointer2(TkWindow *sourcePtr, TkWindow *destPtr,
			    int mode, int leaveEvents, int EnterEvents);
static void		QueueGrabWindowChange(TkDisplay *dispPtr,
			    TkWindow *grabWinPtr);
static void		ReleaseButtonGrab(TkDisplay *dispPtr);

/*
 *----------------------------------------------------------------------
 *
 * Tk_GrabObjCmd --
 *
 *	This function is invoked to process the "grab" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_GrabObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int globalGrab;
    Tk_Window tkwin;
    TkDisplay *dispPtr;
    const char *arg;
    int index;
    int len;
    static const char *const optionStrings[] = {
	"current", "release", "set", "status", NULL
    };
    static const char *const flagStrings[] = {
	"-global", NULL
    };
    enum options {
	GRABCMD_CURRENT, GRABCMD_RELEASE, GRABCMD_SET, GRABCMD_STATUS
    };

    if (objc < 2) {
	/*
	 * Can't use Tcl_WrongNumArgs here because we want the message to
	 * read:
	 * wrong # args: should be "cmd ?-global? window" or "cmd option
	 *    ?arg ...?"
	 * We can fake it with Tcl_WrongNumArgs if we assume the command name
	 * is "grab", but if it has been aliased, the message will be
	 * incorrect.
	 */

	Tcl_WrongNumArgs(interp, 1, objv, "?-global? window");
	Tcl_AppendResult(interp, " or \"", Tcl_GetString(objv[0]),
		" option ?arg ...?\"", NULL);
	/* This API not exposed:
	 *
	((Interp *) interp)->flags |= INTERP_ALTERNATE_WRONG_ARGS;
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	 */
	return TCL_ERROR;
    }

    /*
     * First check for a window name or "-global" as the first argument.
     */

    arg = Tcl_GetStringFromObj(objv[1], &len);
    if (arg[0] == '.') {
	/* [grab window] */
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 1, objv, "?-global? window");
	    return TCL_ERROR;
	}
	tkwin = Tk_NameToWindow(interp, arg, clientData);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	return Tk_Grab(interp, tkwin, 0);
    } else if (arg[0] == '-' && len > 1) {
	if (Tcl_GetIndexFromObj(interp, objv[1], flagStrings, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}

	/* [grab -global window] */
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "?-global? window");
	    return TCL_ERROR;
	}
	tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), clientData);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	return Tk_Grab(interp, tkwin, 1);
    }

    /*
     * First argument is not a window name and not "-global", find out which
     * option it is.
     */

    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case GRABCMD_CURRENT:
	/* [grab current ?window?] */
	if (objc > 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "current ?window?");
	    return TCL_ERROR;
	}
	if (objc == 3) {
	    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]),
		    clientData);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    dispPtr = ((TkWindow *) tkwin)->dispPtr;
	    if (dispPtr->eventualGrabWinPtr != NULL) {
		Tcl_SetObjResult(interp, TkNewWindowObj((Tk_Window)
			dispPtr->eventualGrabWinPtr));
	    }
	} else {
	    Tcl_Obj *resultObj = Tcl_NewObj();

	    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
		    dispPtr = dispPtr->nextPtr) {
		if (dispPtr->eventualGrabWinPtr != NULL) {
		    Tcl_ListObjAppendElement(NULL, resultObj, TkNewWindowObj(
			    (Tk_Window) dispPtr->eventualGrabWinPtr));
		}
	    }
	    Tcl_SetObjResult(interp, resultObj);
	}
	return TCL_OK;

    case GRABCMD_RELEASE:
	/* [grab release window] */
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "release window");
	    return TCL_ERROR;
	}
	tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), clientData);
	if (tkwin == NULL) {
	    Tcl_ResetResult(interp);
	} else {
	    Tk_Ungrab(tkwin);
	}
	break;

    case GRABCMD_SET:
	/* [grab set ?-global? window] */
	if ((objc != 3) && (objc != 4)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "set ?-global? window");
	    return TCL_ERROR;
	}
	if (objc == 3) {
	    globalGrab = 0;
	    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]),
		    clientData);
	} else {
	    globalGrab = 1;

	    /*
	     * We could just test the argument by hand instead of using
	     * Tcl_GetIndexFromObj; the benefit of using the function is that
	     * it sets up the error message for us, so we are certain to be
	     * consistant with the rest of Tcl.
	     */

	    if (Tcl_GetIndexFromObj(interp, objv[2], flagStrings, "option",
		    0, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[3]),
		    clientData);
	}
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	return Tk_Grab(interp, tkwin, globalGrab);

    case GRABCMD_STATUS: {
	/* [grab status window] */
	TkWindow *winPtr;
	const char *statusString;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "status window");
	    return TCL_ERROR;
	}
	winPtr = (TkWindow *) Tk_NameToWindow(interp, Tcl_GetString(objv[2]),
		clientData);
	if (winPtr == NULL) {
	    return TCL_ERROR;
	}
	dispPtr = winPtr->dispPtr;
	if (dispPtr->eventualGrabWinPtr != winPtr) {
	    statusString = "none";
	} else if (dispPtr->grabFlags & GRAB_GLOBAL) {
	    statusString = "global";
	} else {
	    statusString = "local";
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(statusString, -1));
	break;
    }
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_Grab --
 *
 *	Grabs the pointer and keyboard, so that mouse-related events are only
 *	reported relative to a given window and its descendants.
 *
 * Results:
 *	A standard Tcl result is returned. TCL_OK is the normal return value;
 *	if the grab could not be set then TCL_ERROR is returned and the
 *	interp's result will hold an error message.
 *
 * Side effects:
 *	Once this call completes successfully, no window outside the tree
 *	rooted at tkwin will receive pointer- or keyboard-related events until
 *	the next call to Tk_Ungrab. If a previous grab was in effect within
 *	this application, then it is replaced with a new one.
 *
 *----------------------------------------------------------------------
 */

int
Tk_Grab(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Window tkwin,		/* Window on whose behalf the pointer is to be
				 * grabbed. */
    int grabGlobal)		/* Non-zero means issue a grab to the server
				 * so that no other application gets mouse or
				 * keyboard events. Zero means the grab only
				 * applies within this application. */
{
    int grabResult, numTries;
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkWindow *winPtr2;
    unsigned int serial;

    ReleaseButtonGrab(dispPtr);
    if (dispPtr->eventualGrabWinPtr != NULL) {
	if ((dispPtr->eventualGrabWinPtr == winPtr)
		&& (grabGlobal == ((dispPtr->grabFlags & GRAB_GLOBAL) != 0))) {
	    return TCL_OK;
	}
	if (dispPtr->eventualGrabWinPtr->mainPtr != winPtr->mainPtr) {
	    goto alreadyGrabbed;
	}
	Tk_Ungrab((Tk_Window) dispPtr->eventualGrabWinPtr);
    }

    Tk_MakeWindowExist(tkwin);
    if (!grabGlobal) {
	Window dummy1, dummy2;
	int dummy3, dummy4, dummy5, dummy6;
	unsigned int state;

	/*
	 * Local grab. However, if any mouse buttons are down, turn it into a
	 * global grab temporarily, until the last button goes up. This does
	 * two things: (a) it makes sure that we see the button-up event; and
	 * (b) it allows us to track mouse motion among all of the windows of
	 * this application.
	 */

	dispPtr->grabFlags &= ~(GRAB_GLOBAL|GRAB_TEMP_GLOBAL);
	XQueryPointer(dispPtr->display, winPtr->window, &dummy1,
		&dummy2, &dummy3, &dummy4, &dummy5, &dummy6, &state);
	if (state & ALL_BUTTONS) {
	    dispPtr->grabFlags |= GRAB_TEMP_GLOBAL;
	    goto setGlobalGrab;
	}
    } else {
	dispPtr->grabFlags |= GRAB_GLOBAL;
    setGlobalGrab:

	/*
	 * Tricky point: must ungrab before grabbing. This is needed in case
	 * there is a button auto-grab already in effect. If there is, and the
	 * mouse has moved to a different window, X won't generate enter and
	 * leave events to move the mouse if we grab without ungrabbing.
	 */

	XUngrabPointer(dispPtr->display, CurrentTime);
	serial = NextRequest(dispPtr->display);

	/*
	 * Another tricky point: there are races with some window managers
	 * that can cause grabs to fail because the window manager hasn't
	 * released its grab quickly enough. To work around this problem,
	 * retry a few times after AlreadyGrabbed errors to give the grab
	 * release enough time to register with the server.
	 */

	grabResult = 0;			/* Needed only to prevent gcc compiler
					 * warnings. */
	for (numTries = 0; numTries < 10; numTries++) {
	    grabResult = XGrabPointer(dispPtr->display, winPtr->window,
		    True, ButtonPressMask|ButtonReleaseMask|ButtonMotionMask
		    |PointerMotionMask, GrabModeAsync, GrabModeAsync, None,
		    None, CurrentTime);
	    if (grabResult != AlreadyGrabbed) {
		break;
	    }
	    Tcl_Sleep(100);
	}
	if (grabResult != 0) {
	    goto grabError;
	}
	grabResult = XGrabKeyboard(dispPtr->display, Tk_WindowId(tkwin),
		False, GrabModeAsync, GrabModeAsync, CurrentTime);
	if (grabResult != 0) {
	    XUngrabPointer(dispPtr->display, CurrentTime);
	    goto grabError;
	}

	/*
	 * Eat up any grab-related events generated by the server for the
	 * grab. There are several reasons for doing this:
	 *
	 * 1. We have to synthesize the events for local grabs anyway, since
	 *    the server doesn't participate in them.
	 * 2. The server doesn't always generate the right events for global
	 *    grabs (e.g. it generates events even if the current window is in
	 *    the grab tree, which we don't want).
	 * 3. We want all the grab-related events to be processed immediately
	 *    (before other events that are already queued); events coming
	 *    from the server will be in the wrong place, but events we
	 *    synthesize here will go to the front of the queue.
	 */

	EatGrabEvents(dispPtr, serial);
    }

    /*
     * Synthesize leave events to move the pointer from its current window up
     * to the lowest ancestor that it has in common with the grab window.
     * However, only do this if the pointer is outside the grab window's
     * subtree but inside the grab window's application.
     */

    if ((dispPtr->serverWinPtr != NULL)
	    && (dispPtr->serverWinPtr->mainPtr == winPtr->mainPtr)) {
	for (winPtr2 = dispPtr->serverWinPtr; ; winPtr2 = winPtr2->parentPtr) {
	    if (winPtr2 == winPtr) {
		break;
	    }
	    if (winPtr2 == NULL) {
		MovePointer2(dispPtr->serverWinPtr, winPtr, NotifyGrab, 1, 0);
		break;
	    }
	}
    }
    QueueGrabWindowChange(dispPtr, winPtr);
    return TCL_OK;

  grabError:
    if (grabResult == GrabNotViewable) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"grab failed: window not viewable", -1));
	Tcl_SetErrorCode(interp, "TK", "GRAB", "UNVIEWABLE", NULL);
    } else if (grabResult == AlreadyGrabbed) {
    alreadyGrabbed:
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"grab failed: another application has grab", -1));
	Tcl_SetErrorCode(interp, "TK", "GRAB", "GRABBED", NULL);
    } else if (grabResult == GrabFrozen) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"grab failed: keyboard or pointer frozen", -1));
	Tcl_SetErrorCode(interp, "TK", "GRAB", "FROZEN", NULL);
    } else if (grabResult == GrabInvalidTime) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"grab failed: invalid time", -1));
	Tcl_SetErrorCode(interp, "TK", "GRAB", "BAD_TIME", NULL);
    } else {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"grab failed for unknown reason (code %d)", grabResult));
	Tcl_SetErrorCode(interp, "TK", "GRAB", "UNKNOWN", NULL);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_Ungrab --
 *
 *	Releases a grab on the mouse pointer and keyboard, if there is one set
 *	on the specified window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Pointer and keyboard events will start being delivered to other
 *	windows again.
 *
 *----------------------------------------------------------------------
 */

void
Tk_Ungrab(
    Tk_Window tkwin)		/* Window whose grab should be released. */
{
    TkDisplay *dispPtr;
    TkWindow *grabWinPtr, *winPtr;
    unsigned int serial;

    grabWinPtr = (TkWindow *) tkwin;
    dispPtr = grabWinPtr->dispPtr;
    if (grabWinPtr != dispPtr->eventualGrabWinPtr) {
	return;
    }
    ReleaseButtonGrab(dispPtr);
    QueueGrabWindowChange(dispPtr, NULL);
    if (dispPtr->grabFlags & (GRAB_GLOBAL|GRAB_TEMP_GLOBAL)) {
	dispPtr->grabFlags &= ~(GRAB_GLOBAL|GRAB_TEMP_GLOBAL);
	serial = NextRequest(dispPtr->display);
	XUngrabPointer(dispPtr->display, CurrentTime);
	XUngrabKeyboard(dispPtr->display, CurrentTime);
	EatGrabEvents(dispPtr, serial);
    }

    /*
     * Generate events to move the pointer back to the window where it really
     * is. Some notes:
     * 1. As with grabs, only do this if the "real" window is not a descendant
     *    of the grab window, since in this case the pointer is already where
     *    it's supposed to be.
     * 2. If the "real" window is in some other application then don't
     *    generate any events at all, since everything's already been reported
     *    correctly.
     * 3. Only generate enter events. Don't generate leave events, because we
     *    never told the lower-level windows that they had the pointer in the
     *    first place.
     */

    for (winPtr = dispPtr->serverWinPtr; ; winPtr = winPtr->parentPtr) {
	if (winPtr == grabWinPtr) {
	    break;
	}
	if (winPtr == NULL) {
	    if ((dispPtr->serverWinPtr == NULL) ||
		    (dispPtr->serverWinPtr->mainPtr == grabWinPtr->mainPtr)) {
		MovePointer2(grabWinPtr, dispPtr->serverWinPtr,
			NotifyUngrab, 0, 1);
	    }
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseButtonGrab --
 *
 *	This function is called to release a simulated button grab, if there
 *	is one in effect. A button grab is present whenever
 *	dispPtr->buttonWinPtr is non-NULL or when the GRAB_TEMP_GLOBAL flag is
 *	set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	DispPtr->buttonWinPtr is reset to NULL, and enter and leave events are
 *	generated if necessary to move the pointer from the button grab window
 *	to its current window.
 *
 *----------------------------------------------------------------------
 */

static void
ReleaseButtonGrab(
    register TkDisplay *dispPtr)/* Display whose button grab is to be
				 * released. */
{
    unsigned int serial;

    if (dispPtr->buttonWinPtr != NULL) {
	if (dispPtr->buttonWinPtr != dispPtr->serverWinPtr) {
	    MovePointer2(dispPtr->buttonWinPtr, dispPtr->serverWinPtr,
		    NotifyUngrab, 1, 1);
	}
	dispPtr->buttonWinPtr = NULL;
    }
    if (dispPtr->grabFlags & GRAB_TEMP_GLOBAL) {
	dispPtr->grabFlags &= ~GRAB_TEMP_GLOBAL;
	serial = NextRequest(dispPtr->display);
	XUngrabPointer(dispPtr->display, CurrentTime);
	XUngrabKeyboard(dispPtr->display, CurrentTime);
	EatGrabEvents(dispPtr, serial);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkPointerEvent --
 *
 *	This function is called for each pointer-related event, before the
 *	event has been processed. It does various things to make grabs work
 *	correctly.
 *
 * Results:
 *	If the return value is 1 it means the event should be processed (event
 *	handlers should be invoked). If the return value is 0 it means the
 *	event should be ignored in order to make grabs work correctly. In some
 *	cases this function modifies the event.
 *
 * Side effects:
 *	Grab state information may be updated. New events may also be pushed
 *	back onto the event queue to replace or augment the one passed in
 *	here.
 *
 *----------------------------------------------------------------------
 */

int
TkPointerEvent(
    register XEvent *eventPtr,	/* Pointer to the event. */
    TkWindow *winPtr)		/* Tk's information for window where event was
				 * reported. */
{
    register TkWindow *winPtr2;
    TkDisplay *dispPtr = winPtr->dispPtr;
    unsigned int serial;
    int outsideGrabTree = 0;
    int ancestorOfGrab = 0;
    int appGrabbed = 0;		/* Non-zero means event is being reported to
				 * an application that is affected by the
				 * grab. */

    /*
     * Collect information about the grab (if any).
     */

    switch (TkGrabState(winPtr)) {
    case TK_GRAB_IN_TREE:
	appGrabbed = 1;
	break;
    case TK_GRAB_ANCESTOR:
	appGrabbed = 1;
	outsideGrabTree = 1;
	ancestorOfGrab = 1;
	break;
    case TK_GRAB_EXCLUDED:
	appGrabbed = 1;
	outsideGrabTree = 1;
	break;
    }

    if ((eventPtr->type == EnterNotify) || (eventPtr->type == LeaveNotify)) {
	/*
	 * Keep track of what window the mouse is *really* over. Any events
	 * that we generate have a special send_event value, which is detected
	 * below and used to ignore the event for purposes of setting
	 * serverWinPtr.
	 */

	if (eventPtr->xcrossing.send_event != GENERATED_GRAB_EVENT_MAGIC) {
	    if ((eventPtr->type == LeaveNotify) &&
		    (winPtr->flags & TK_TOP_HIERARCHY)) {
		dispPtr->serverWinPtr = NULL;
	    } else {
		dispPtr->serverWinPtr = winPtr;
	    }
	}

	/*
	 * When a grab is active, X continues to report enter and leave events
	 * for windows outside the tree of the grab window:
	 * 1. Detect these events and ignore them except for windows above the
	 *    grab window.
	 * 2. Allow Enter and Leave events to pass through the windows above
	 *    the grab window, but never let them end up with the pointer *in*
	 *    one of those windows.
	 */

	if (dispPtr->grabWinPtr != NULL) {
	    if (outsideGrabTree && appGrabbed) {
		if (!ancestorOfGrab) {
		    return 0;
		}
		switch (eventPtr->xcrossing.detail) {
		case NotifyInferior:
		    return 0;
		case NotifyAncestor:
		    eventPtr->xcrossing.detail = NotifyVirtual;
		    break;
		case NotifyNonlinear:
		    eventPtr->xcrossing.detail = NotifyNonlinearVirtual;
		    break;
		}
	    }

	    /*
	     * Make buttons have the same grab-like behavior inside a grab as
	     * they do outside a grab: do this by ignoring enter and leave
	     * events except for the window in which the button was pressed.
	     */

	    if ((dispPtr->buttonWinPtr != NULL)
		    && (winPtr != dispPtr->buttonWinPtr)) {
		return 0;
	    }
	}
	return 1;
    }

    if (!appGrabbed) {
	return 1;
    }

    if (eventPtr->type == MotionNotify) {
	/*
	 * When grabs are active, X reports motion events relative to the
	 * window under the pointer. Instead, it should report the events
	 * relative to the window the button went down in, if there is a
	 * button down. Otherwise, if the pointer window is outside the
	 * subtree of the grab window, the events should be reported relative
	 * to the grab window. Otherwise, the event should be reported to the
	 * pointer window.
	 */

	winPtr2 = winPtr;
	if (dispPtr->buttonWinPtr != NULL) {
	    winPtr2 = dispPtr->buttonWinPtr;
	} else if (outsideGrabTree || (dispPtr->serverWinPtr == NULL)) {
	    winPtr2 = dispPtr->grabWinPtr;
	}
	if (winPtr2 != winPtr) {
	    TkChangeEventWindow(eventPtr, winPtr2);
	    Tk_QueueWindowEvent(eventPtr, TCL_QUEUE_HEAD);
	    return 0;
	}
	return 1;
    }

    /*
     * Process ButtonPress and ButtonRelease events:
     * 1. Keep track of whether a button is down and what window it went down
     *    in.
     * 2. If the first button goes down outside the grab tree, pretend it went
     *    down in the grab window. Note: it's important to redirect events to
     *    the grab window like this in order to make things like menus work,
     *    where button presses outside the grabbed menu need to be seen. An
     *    application can always ignore the events if they occur outside its
     *    window.
     * 3. If a button press or release occurs outside the window where the
     *    first button was pressed, retarget the event so it's reported to the
     *    window where the first button was pressed.
     * 4. If the last button is released in a window different than where the
     *    first button was pressed, generate Enter/Leave events to move the
     *    mouse from the button window to its current window.
     * 5. If the grab is set at a time when a button is already down, or if
     *    the window where the button was pressed was deleted, then
     *    dispPtr->buttonWinPtr will stay NULL. Just forget about the
     *    auto-grab for the button press; events will go to whatever window
     *    contains the pointer. If this window isn't in the grab tree then
     *    redirect events to the grab window.
     * 6. When a button is pressed during a local grab, the X server sets a
     *    grab of its own, since it doesn't even know about our local grab.
     *    This causes enter and leave events no longer to be generated in the
     *    same way as for global grabs. To eliminate this problem, set a
     *    temporary global grab when the first button goes down and release it
     *    when the last button comes up.
     */

    if ((eventPtr->type == ButtonPress) || (eventPtr->type == ButtonRelease)) {
	winPtr2 = dispPtr->buttonWinPtr;
	if (winPtr2 == NULL) {
	    if (outsideGrabTree) {
		winPtr2 = dispPtr->grabWinPtr;			/* Note 5. */
	    } else {
		winPtr2 = winPtr;				/* Note 5. */
	    }
	}
	if (eventPtr->type == ButtonPress) {
	    if (!(eventPtr->xbutton.state & ALL_BUTTONS)) {
		if (outsideGrabTree) {
		    TkChangeEventWindow(eventPtr, dispPtr->grabWinPtr);
		    Tk_QueueWindowEvent(eventPtr, TCL_QUEUE_HEAD);
		    return 0;					/* Note 2. */
		}
		if (!(dispPtr->grabFlags & GRAB_GLOBAL)) {	/* Note 6. */
		    serial = NextRequest(dispPtr->display);
		    if (XGrabPointer(dispPtr->display,
			    dispPtr->grabWinPtr->window, True,
			    ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
			    GrabModeAsync, GrabModeAsync, None, None,
			    CurrentTime) == 0) {
			EatGrabEvents(dispPtr, serial);
			if (XGrabKeyboard(dispPtr->display, winPtr->window,
				False, GrabModeAsync, GrabModeAsync,
				CurrentTime) == 0) {
			    dispPtr->grabFlags |= GRAB_TEMP_GLOBAL;
			} else {
			    XUngrabPointer(dispPtr->display, CurrentTime);
			}
		    }
		}
		dispPtr->buttonWinPtr = winPtr;
		return 1;
	    }
	} else {
	    if (eventPtr->xbutton.button != AnyButton &&
		    ((eventPtr->xbutton.state & ALL_BUTTONS)
		    == (unsigned int)TkGetButtonMask(eventPtr->xbutton.button))) {
		ReleaseButtonGrab(dispPtr);			/* Note 4. */
	    }
	}
	if (winPtr2 != winPtr) {
	    TkChangeEventWindow(eventPtr, winPtr2);
	    Tk_QueueWindowEvent(eventPtr, TCL_QUEUE_HEAD);
	    return 0;						/* Note 3. */
	}
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkChangeEventWindow --
 *
 *	Given an event and a new window to which the event should be
 *	retargeted, modify fields of the event so that the event is properly
 *	retargeted to the new window.
 *
 * Results:
 *	The following fields of eventPtr are modified: window, subwindow, x,
 *	y, same_screen.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkChangeEventWindow(
    register XEvent *eventPtr,	/* Event to retarget. Must have type
				 * ButtonPress, ButtonRelease, KeyPress,
				 * KeyRelease, MotionNotify, EnterNotify, or
				 * LeaveNotify. */
    TkWindow *winPtr)		/* New target window for event. */
{
    int x, y, sameScreen, bd;
    register TkWindow *childPtr;

    eventPtr->xmotion.window = Tk_WindowId(winPtr);
    if (eventPtr->xmotion.root ==
	    RootWindow(winPtr->display, winPtr->screenNum)) {
	Tk_GetRootCoords((Tk_Window) winPtr, &x, &y);
	eventPtr->xmotion.x = eventPtr->xmotion.x_root - x;
	eventPtr->xmotion.y = eventPtr->xmotion.y_root - y;
	eventPtr->xmotion.subwindow = None;
	for (childPtr = winPtr->childList; childPtr != NULL;
		childPtr = childPtr->nextPtr) {
	    if (childPtr->flags & TK_TOP_HIERARCHY) {
		continue;
	    }
	    x = eventPtr->xmotion.x - childPtr->changes.x;
	    y = eventPtr->xmotion.y - childPtr->changes.y;
	    bd = childPtr->changes.border_width;
	    if ((x >= -bd) && (y >= -bd)
		    && (x < (childPtr->changes.width + bd))
		    && (y < (childPtr->changes.height + bd))) {
		eventPtr->xmotion.subwindow = childPtr->window;
	    }
	}
	sameScreen = 1;
    } else {
	eventPtr->xmotion.x = 0;
	eventPtr->xmotion.y = 0;
	eventPtr->xmotion.subwindow = None;
	sameScreen = 0;
    }
    if (eventPtr->type == MotionNotify) {
	eventPtr->xmotion.same_screen = sameScreen;
    } else {
	eventPtr->xbutton.same_screen = sameScreen;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkInOutEvents --
 *
 *	This function synthesizes EnterNotify and LeaveNotify events to
 *	correctly transfer the pointer from one window to another. It can also
 *	be used to generate FocusIn and FocusOut events to move the input
 *	focus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Synthesized events may be pushed back onto the event queue. The event
 *	pointed to by eventPtr is modified.
 *
 *----------------------------------------------------------------------
 */

void
TkInOutEvents(
    XEvent *eventPtr,		/* A template X event. Must have all fields
				 * properly set except for type, window,
				 * subwindow, x, y, detail, and same_screen.
				 * (Not all of these fields are valid for
				 * FocusIn/FocusOut events; x_root and y_root
				 * must be valid for Enter/Leave events, even
				 * though x and y needn't be valid). */
    TkWindow *sourcePtr,	/* Window that used to have the pointer or
				 * focus (NULL means it was not in a window
				 * managed by this process). */
    TkWindow *destPtr,		/* Window that is to end up with the pointer
				 * or focus (NULL means it's not one managed
				 * by this process). */
    int leaveType,		/* Type of events to generate for windows
				 * being left (LeaveNotify or FocusOut). 0
				 * means don't generate leave events. */
    int enterType,		/* Type of events to generate for windows
				 * being entered (EnterNotify or FocusIn). 0
				 * means don't generate enter events. */
    Tcl_QueuePosition position)	/* Position at which events are added to the
				 * system event queue. */
{
    register TkWindow *winPtr;
    int upLevels, downLevels, i, j, focus;

    /*
     * There are four possible cases to deal with:
     *
     * 1. SourcePtr and destPtr are the same. There's nothing to do in this
     *    case.
     * 2. SourcePtr is an ancestor of destPtr in the same top-level window.
     *    Must generate events down the window tree from source to dest.
     * 3. DestPtr is an ancestor of sourcePtr in the same top-level window.
     *    Must generate events up the window tree from sourcePtr to destPtr.
     * 4. All other cases. Must first generate events up the window tree from
     *    sourcePtr to its top-level, then down from destPtr's top-level to
     *    destPtr. This form is called "non-linear."
     *
     * The call to FindCommonAncestor separates these four cases and decides
     * how many levels up and down events have to be generated for.
     */

    if (sourcePtr == destPtr) {
	return;
    }
    if ((leaveType == FocusOut) || (enterType == FocusIn)) {
	focus = 1;
    } else {
	focus = 0;
    }
    FindCommonAncestor(sourcePtr, destPtr, &upLevels, &downLevels);

    /*
     * Generate enter/leave events and add them to the grab event queue.
     */

#define QUEUE(w, t, d)					\
    if (w->window != None) {				\
	eventPtr->type = t;				\
	if (focus) {					\
	    eventPtr->xfocus.window = w->window;	\
	    eventPtr->xfocus.detail = d;		\
	} else {					\
	    eventPtr->xcrossing.detail = d;		\
	    TkChangeEventWindow(eventPtr, w);		\
	}						\
	Tk_QueueWindowEvent(eventPtr, position);	\
    }

    if (downLevels == 0) {
	/*
	 * SourcePtr is an inferior of destPtr.
	 */

	if (leaveType != 0) {
	    QUEUE(sourcePtr, leaveType, NotifyAncestor);
	    for (winPtr = sourcePtr->parentPtr, i = upLevels-1; i > 0;
		    winPtr = winPtr->parentPtr, i--) {
		QUEUE(winPtr, leaveType, NotifyVirtual);
	    }
	}
	if ((enterType != 0) && (destPtr != NULL)) {
	    QUEUE(destPtr, enterType, NotifyInferior);
	}
    } else if (upLevels == 0) {
	/*
	 * DestPtr is an inferior of sourcePtr.
	 */

	if ((leaveType != 0) && (sourcePtr != NULL)) {
	    QUEUE(sourcePtr, leaveType, NotifyInferior);
	}
	if (enterType != 0) {
	    for (i = downLevels-1; i > 0; i--) {
		for (winPtr = destPtr->parentPtr, j = 1; j < i;
			winPtr = winPtr->parentPtr, j++) {
		    /* empty */
		}
		QUEUE(winPtr, enterType, NotifyVirtual);
	    }
	    if (destPtr != NULL) {
		QUEUE(destPtr, enterType, NotifyAncestor);
	    }
	}
    } else {
	/*
	 * Non-linear: neither window is an inferior of the other.
	 */

	if (leaveType != 0) {
	    QUEUE(sourcePtr, leaveType, NotifyNonlinear);
	    for (winPtr = sourcePtr->parentPtr, i = upLevels-1; i > 0;
		    winPtr = winPtr->parentPtr, i--) {
		QUEUE(winPtr, leaveType, NotifyNonlinearVirtual);
	    }
	}
	if (enterType != 0) {
	    for (i = downLevels-1; i > 0; i--) {
		for (winPtr = destPtr->parentPtr, j = 1; j < i;
			winPtr = winPtr->parentPtr, j++) {
		}
		QUEUE(winPtr, enterType, NotifyNonlinearVirtual);
	    }
	    if (destPtr != NULL) {
		QUEUE(destPtr, enterType, NotifyNonlinear);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MovePointer2 --
 *
 *	This function synthesizes EnterNotify and LeaveNotify events to
 *	correctly transfer the pointer from one window to another. It is
 *	different from TkInOutEvents in that no template X event needs to be
 *	supplied; this function generates the template event and calls
 *	TkInOutEvents.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Synthesized events may be pushed back onto the event queue.
 *
 *----------------------------------------------------------------------
 */

static void
MovePointer2(
    TkWindow *sourcePtr,	/* Window currently containing pointer (NULL
				 * means it's not one managed by this
				 * process). */
    TkWindow *destPtr,		/* Window that is to end up containing the
				 * pointer (NULL means it's not one managed by
				 * this process). */
    int mode,			/* Mode for enter/leave events, such as
				 * NotifyNormal or NotifyUngrab. */
    int leaveEvents,		/* Non-zero means generate leave events for
				 * the windows being left. Zero means don't
				 * generate leave events. */
    int enterEvents)		/* Non-zero means generate enter events for
				 * the windows being entered. Zero means don't
				 * generate enter events. */
{
    XEvent event;
    Window dummy1, dummy2;
    int dummy3, dummy4;
    TkWindow *winPtr;

    winPtr = sourcePtr;
    if ((winPtr == NULL) || (winPtr->window == None)) {
	winPtr = destPtr;
	if ((winPtr == NULL) || (winPtr->window == None)) {
	    return;
	}
    }

    event.xcrossing.serial = LastKnownRequestProcessed(winPtr->display);
    event.xcrossing.send_event = GENERATED_GRAB_EVENT_MAGIC;
    event.xcrossing.display = winPtr->display;
    event.xcrossing.root = RootWindow(winPtr->display, winPtr->screenNum);
    event.xcrossing.time = TkCurrentTime(winPtr->dispPtr);
    XQueryPointer(winPtr->display, winPtr->window, &dummy1, &dummy2,
	    &event.xcrossing.x_root, &event.xcrossing.y_root,
	    &dummy3, &dummy4, &event.xcrossing.state);
    event.xcrossing.mode = mode;
    event.xcrossing.focus = False;
    TkInOutEvents(&event, sourcePtr, destPtr, (leaveEvents) ? LeaveNotify : 0,
	    (enterEvents) ? EnterNotify : 0, TCL_QUEUE_MARK);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGrabDeadWindow --
 *
 *	This function is invoked whenever a window is deleted, so that
 *	grab-related cleanup can be performed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Various cleanups happen, such as generating events to move the pointer
 *	back to its "natural" window as if an ungrab had been done. See the
 *	code.
 *
 *----------------------------------------------------------------------
 */

void
TkGrabDeadWindow(
    register TkWindow *winPtr)	/* Window that is in the process of being
				 * deleted. */
{
    TkDisplay *dispPtr = winPtr->dispPtr;

    if (dispPtr->eventualGrabWinPtr == winPtr) {
	/*
	 * Grab window was deleted. Release the grab.
	 */

	Tk_Ungrab((Tk_Window) dispPtr->eventualGrabWinPtr);
    } else if (dispPtr->buttonWinPtr == winPtr) {
	ReleaseButtonGrab(dispPtr);
    }
    if (dispPtr->serverWinPtr == winPtr) {
	if (winPtr->flags & TK_TOP_HIERARCHY) {
	    dispPtr->serverWinPtr = NULL;
	} else {
	    dispPtr->serverWinPtr = winPtr->parentPtr;
	}
    }
    if (dispPtr->grabWinPtr == winPtr) {
	dispPtr->grabWinPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EatGrabEvents --
 *
 *	This function is called to eliminate any Enter, Leave, FocusIn, or
 *	FocusOut events in the event queue for a display that have mode
 *	NotifyGrab or NotifyUngrab and have a serial number no less than a
 *	given value and are not generated by the grab module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	DispPtr's display gets sync-ed, and some of the events get removed
 *	from the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static void
EatGrabEvents(
    TkDisplay *dispPtr,		/* Display from which to consume events. */
    unsigned int serial)	/* Only discard events that have a serial
				 * number at least this great. */
{
    Tk_RestrictProc *prevProc;
    GrabInfo info;
    ClientData prevArg;

    info.display = dispPtr->display;
    info.serial = serial;
    TkpSync(info.display);
    prevProc = Tk_RestrictEvents(GrabRestrictProc, &info, &prevArg);
    while (Tcl_ServiceEvent(TCL_WINDOW_EVENTS)) {
	/* EMPTY */
    }
    Tk_RestrictEvents(prevProc, prevArg, &prevArg);
}

/*
 *----------------------------------------------------------------------
 *
 * GrabRestrictProc --
 *
 *	A Tk_RestrictProc used by EatGrabEvents to eliminate any Enter, Leave,
 *	FocusIn, or FocusOut events in the event queue for a display that has
 *	mode NotifyGrab or NotifyUngrab and have a serial number no less than
 *	a given value.
 *
 * Results:
 *	Returns either TK_DISCARD_EVENT or TK_DEFER_EVENT.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tk_RestrictAction
GrabRestrictProc(
    ClientData arg,
    XEvent *eventPtr)
{
    GrabInfo *info = arg;
    int mode, diff;

    /*
     * The diff caculation is trickier than it may seem. Don't forget that
     * serial numbers can wrap around, so can't compare the two serial numbers
     * directly.
     */

    diff = eventPtr->xany.serial - info->serial;
    if ((eventPtr->type == EnterNotify)
	    || (eventPtr->type == LeaveNotify)) {
	mode = eventPtr->xcrossing.mode;
    } else if ((eventPtr->type == FocusIn)
	    || (eventPtr->type == FocusOut)) {
	mode = eventPtr->xfocus.mode;
    } else {
	mode = NotifyNormal;
    }
    if ((info->display != eventPtr->xany.display) || (mode == NotifyNormal)
	    || (diff < 0)) {
	return TK_DEFER_EVENT;
    } else {
	return TK_DISCARD_EVENT;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * QueueGrabWindowChange --
 *
 *	This function queues a special event in the Tcl event queue, which
 *	will cause the "grabWinPtr" field for the display to get modified when
 *	the event is processed. This is needed to make sure that the grab
 *	window changes at the proper time relative to grab-related enter and
 *	leave events that are also in the queue. In particular, this approach
 *	works even when multiple grabs and ungrabs happen back-to-back.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	DispPtr->grabWinPtr will be modified later (by GrabWinEventProc) when
 *	the event is removed from the grab event queue.
 *
 *----------------------------------------------------------------------
 */

static void
QueueGrabWindowChange(
    TkDisplay *dispPtr,		/* Display on which to change the grab
				 * window. */
    TkWindow *grabWinPtr)	/* Window that is to become the new grab
				 * window (may be NULL). */
{
    NewGrabWinEvent *grabEvPtr;

    grabEvPtr = ckalloc(sizeof(NewGrabWinEvent));
    grabEvPtr->header.proc = GrabWinEventProc;
    grabEvPtr->dispPtr = dispPtr;
    if (grabWinPtr == NULL) {
	grabEvPtr->grabWindow = None;
    } else {
	grabEvPtr->grabWindow = grabWinPtr->window;
    }
    Tcl_QueueEvent(&grabEvPtr->header, TCL_QUEUE_MARK);
    dispPtr->eventualGrabWinPtr = grabWinPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GrabWinEventProc --
 *
 *	This function is invoked as a handler for Tcl_Events of type
 *	NewGrabWinEvent. It updates the current grab window field in a
 *	display.
 *
 * Results:
 *	Returns 1 if the event was processed, 0 if it should be deferred for
 *	processing later.
 *
 * Side effects:
 *	The grabWinPtr field is modified in the display associated with the
 *	event.
 *
 *----------------------------------------------------------------------
 */

static int
GrabWinEventProc(
    Tcl_Event *evPtr,		/* Event of type NewGrabWinEvent. */
    int flags)			/* Flags argument to Tcl_DoOneEvent: indicates
				 * what kinds of events are being processed
				 * right now. */
{
    NewGrabWinEvent *grabEvPtr = (NewGrabWinEvent *) evPtr;

    grabEvPtr->dispPtr->grabWinPtr = (TkWindow *) Tk_IdToWindow(
	    grabEvPtr->dispPtr->display, grabEvPtr->grabWindow);
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * FindCommonAncestor --
 *
 *	Given two windows, this function finds their least common ancestor and
 *	also computes how many levels up this ancestor is from each of the
 *	original windows.
 *
 * Results:
 *	If the windows are in different applications or top-level windows,
 *	then NULL is returned and *countPtr1 and *countPtr2 are set to the
 *	depths of the two windows in their respective top-level windows (1
 *	means the window is a top-level, 2 means its parent is a top-level,
 *	and so on). Otherwise, the return value is a pointer to the common
 *	ancestor and the counts are set to the distance of winPtr1 and winPtr2
 *	from this ancestor (1 means they're children, 2 means grand-children,
 *	etc.).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TkWindow *
FindCommonAncestor(
    TkWindow *winPtr1,		/* First window. May be NULL. */
    TkWindow *winPtr2,		/* Second window. May be NULL. */
    int *countPtr1,		/* Store nesting level of winPtr1 within
				 * common ancestor here. */
    int *countPtr2)		/* Store nesting level of winPtr2 within
				 * common ancestor here. */
{
    register TkWindow *winPtr;
    TkWindow *ancestorPtr;
    int count1, count2, i;

    /*
     * Mark winPtr1 and all of its ancestors with a special flag bit.
     */

    if (winPtr1 != NULL) {
	for (winPtr = winPtr1; winPtr != NULL; winPtr = winPtr->parentPtr) {
	    winPtr->flags |= TK_GRAB_FLAG;
	    if (winPtr->flags & TK_TOP_HIERARCHY) {
		break;
	    }
	}
    }

    /*
     * Search upwards from winPtr2 until an ancestor of winPtr1 is found or a
     * top-level window is reached.
     */

    winPtr = winPtr2;
    count2 = 0;
    ancestorPtr = NULL;
    if (winPtr2 != NULL) {
	for (; winPtr != NULL; count2++, winPtr = winPtr->parentPtr) {
	    if (winPtr->flags & TK_GRAB_FLAG) {
		ancestorPtr = winPtr;
		break;
	    }
	    if (winPtr->flags & TK_TOP_HIERARCHY)  {
		count2++;
		break;
	    }
	}
    }

    /*
     * Search upwards from winPtr1 again, clearing the flag bits and
     * remembering how many levels up we had to go.
     */

    if (winPtr1 == NULL) {
	count1 = 0;
    } else {
	count1 = -1;
	for (i = 0, winPtr = winPtr1; winPtr != NULL;
		i++, winPtr = winPtr->parentPtr) {
	    winPtr->flags &= ~TK_GRAB_FLAG;
	    if (winPtr == ancestorPtr) {
		count1 = i;
	    }
	    if (winPtr->flags & TK_TOP_HIERARCHY) {
		if (count1 == -1) {
		    count1 = i+1;
		}
		break;
	    }
	}
    }

    *countPtr1 = count1;
    *countPtr2 = count2;
    return ancestorPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPositionInTree --
 *
 *	Compute where the given window is relative to a particular subtree of
 *	the window hierarchy.
 *
 * Results:
 *	Returns TK_GRAB_IN_TREE if the window is contained in the subtree.
 *	Returns TK_GRAB_ANCESTOR if the window is an ancestor of the subtree,
 *	in the same toplevel. Otherwise it returns TK_GRAB_EXCLUDED.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkPositionInTree(
    TkWindow *winPtr,		/* Window to be checked. */
    TkWindow *treePtr)		/* Root of tree to compare against. */
{
    TkWindow *winPtr2;

    for (winPtr2 = winPtr; winPtr2 != treePtr;
	   winPtr2 = winPtr2->parentPtr) {
	if (winPtr2 == NULL) {
	    for (winPtr2 = treePtr; winPtr2 != NULL;
		    winPtr2 = winPtr2->parentPtr) {
		if (winPtr2 == winPtr) {
		    return TK_GRAB_ANCESTOR;
		}
		if (winPtr2->flags & TK_TOP_HIERARCHY) {
		    break;
		}
	    }
	    return TK_GRAB_EXCLUDED;
	}
    }
    return TK_GRAB_IN_TREE;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGrabState --
 *
 *	Given a window, this function returns a value that indicates the grab
 *	state of the application relative to the window.
 *
 * Results:
 *	The return value is one of three things:
 *	    TK_GRAB_NONE -	no grab is in effect.
 *	    TK_GRAB_IN_TREE -   there is a grab in effect, and winPtr is in
 *				the grabbed subtree.
 *	    TK_GRAB_ANCESTOR -  there is a grab in effect; winPtr is an
 *				ancestor of the grabbed window, in the same
 *				toplevel.
 *	    TK_GRAB_EXCLUDED -	there is a grab in effect; winPtr is outside
 *				the tree of the grab and is not an ancestor of
 *				the grabbed window in the same toplevel.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkGrabState(
    TkWindow *winPtr)		/* Window for which grab information is
				 * needed. */
{
    TkWindow *grabWinPtr = winPtr->dispPtr->grabWinPtr;

    if (grabWinPtr == NULL) {
	return TK_GRAB_NONE;
    }
    if ((winPtr->mainPtr != grabWinPtr->mainPtr)
	    && !(winPtr->dispPtr->grabFlags & GRAB_GLOBAL)) {
	return TK_GRAB_NONE;
    }

    return TkPositionInTree(winPtr, grabWinPtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
