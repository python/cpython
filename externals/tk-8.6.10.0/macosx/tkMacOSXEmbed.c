/*
 * tkMacOSXEmbed.c --
 *
 *	This file contains platform-specific procedures for theMac to provide
 *	basic operations needed for application embedding (where one
 *	application can use as its main window an internal window from some
 *	other application). Currently only Toplevel embedding within the same
 *	Tk application is allowed on the Macintosh.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkBusy.h"

/*
 * One of the following structures exists for each container in this
 * application. It keeps track of the container window and its associated
 * embedded window.
 */

typedef struct Container {
    Window parent;		/* The Mac Drawable for the parent of the pair
				 * (the container). */
    TkWindow *parentPtr;	/* Tk's information about the container, or
				 * NULL if the container isn't in this
				 * process. */
    Window embedded;		/* The MacDrawable for the embedded window.
				 * Starts off as None, but gets filled in when
				 * the window is eventually created. */
    TkWindow *embeddedPtr;	/* Tk's information about the embedded window,
				 * or NULL if the embedded application isn't
				 * in this process. */
    struct Container *nextPtr;	/* Next in list of all containers in this
				 * process. */
} Container;

static Container *firstContainerPtr = NULL;
				/* First in list of all containers managed by
				 * this process. */
/*
 * Globals defined in this file:
 */

TkMacOSXEmbedHandler *tkMacOSXEmbedHandler = NULL;

/*
 * Prototypes for static procedures defined in this file:
 */

static void	ContainerEventProc(ClientData clientData, XEvent *eventPtr);
static void	EmbeddedEventProc(ClientData clientData, XEvent *eventPtr);
static void	EmbedActivateProc(ClientData clientData, XEvent *eventPtr);
static void	EmbedFocusProc(ClientData clientData, XEvent *eventPtr);
static void	EmbedGeometryRequest(Container *containerPtr, int width,
		    int height);
static void	EmbedSendConfigure(Container *containerPtr);
static void	EmbedStructureProc(ClientData clientData, XEvent *eventPtr);
static void	EmbedWindowDeleted(TkWindow *winPtr);

/*
 *----------------------------------------------------------------------
 *
 * Tk_MacOSXSetEmbedHandler --
 *
 *	Registers a handler for an in process form of embedding, like Netscape
 *	plugins, where Tk is loaded into the process, but does not control the
 *	main window
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The embed handler is set.
 *
 *----------------------------------------------------------------------
 */

void
Tk_MacOSXSetEmbedHandler(
    Tk_MacOSXEmbedRegisterWinProc *registerWinProc,
    Tk_MacOSXEmbedGetGrafPortProc *getPortProc,
    Tk_MacOSXEmbedMakeContainerExistProc *containerExistProc,
    Tk_MacOSXEmbedGetClipProc *getClipProc,
    Tk_MacOSXEmbedGetOffsetInParentProc *getOffsetProc)
{
    if (tkMacOSXEmbedHandler == NULL) {
	tkMacOSXEmbedHandler = ckalloc(sizeof(TkMacOSXEmbedHandler));
    }
    tkMacOSXEmbedHandler->registerWinProc = registerWinProc;
    tkMacOSXEmbedHandler->getPortProc = getPortProc;
    tkMacOSXEmbedHandler->containerExistProc = containerExistProc;
    tkMacOSXEmbedHandler->getClipProc = getClipProc;
    tkMacOSXEmbedHandler->getOffsetProc = getOffsetProc;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeWindow --
 *
 *	Creates an X Window (Mac subwindow).
 *
 * Results:
 *	The window id is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Window
TkpMakeWindow(
    TkWindow *winPtr,
    Window parent)
{
    MacDrawable *macWin;

    /*
     * If this window is marked as embedded then the window structure should
     * have already been created in the TkpUseWindow function.
     */

    if (Tk_IsEmbedded(winPtr)) {
	macWin = winPtr->privatePtr;
    } else {
	/*
	 * Allocate sub window
	 */

	macWin = ckalloc(sizeof(MacDrawable));
	if (macWin == NULL) {
	    winPtr->privatePtr = NULL;
	    return None;
	}
	macWin->winPtr = winPtr;
	winPtr->privatePtr = macWin;
	macWin->visRgn = NULL;
	macWin->aboveVisRgn = NULL;
	macWin->drawRgn = NULL;
	macWin->referenceCount = 0;
	macWin->flags = TK_CLIP_INVALID;
	macWin->view = nil;
	macWin->context = NULL;
	macWin->size = CGSizeZero;
	if (Tk_IsTopLevel(macWin->winPtr)) {
	    /*
	     * This will be set when we are mapped.
	     */

	    macWin->xOff = 0;
	    macWin->yOff = 0;
	    macWin->toplevel = macWin;
	} else if (winPtr->parentPtr) {
	    macWin->xOff = winPtr->parentPtr->privatePtr->xOff +
		    winPtr->parentPtr->changes.border_width +
		    winPtr->changes.x;
	    macWin->yOff = winPtr->parentPtr->privatePtr->yOff +
		    winPtr->parentPtr->changes.border_width +
		    winPtr->changes.y;
	    macWin->toplevel = winPtr->parentPtr->privatePtr->toplevel;
	}
	macWin->toplevel->referenceCount++;
    }
    return (Window) macWin;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpScanWindowId --
 *
 *	Given a string, produce the corresponding Window Id.
 *
 * Results:
 *      The return value is normally TCL_OK; in this case *idPtr will be set
 *      to the Window value equivalent to string. If string is improperly
 *      formed then TCL_ERROR is returned and an error message will be left in
 *      the interp's result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
TkpScanWindowId(
    Tcl_Interp *interp,
    const char * string,
    Window *idPtr)
{
    int code;
    Tcl_Obj obj;

    obj.refCount = 1;
    obj.bytes = (char *) string;	/* DANGER?! */
    obj.length = strlen(string);
    obj.typePtr = NULL;

    code = Tcl_GetLongFromObj(interp, &obj, (long *)idPtr);

    if (obj.refCount > 1) {
	Tcl_Panic("invalid sharing of Tcl_Obj on C stack");
    }
    if (obj.typePtr && obj.typePtr->freeIntRepProc) {
	obj.typePtr->freeIntRepProc(&obj);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpUseWindow --
 *
 *	This procedure causes a Tk window to use a given X window as its
 *	parent window, rather than the root window for the screen. It is
 *	invoked by an embedded application to specify the window in which it
 *	is embedded.
 *
 * Results:
 *	The return value is normally TCL_OK. If an error occurs (such as
 *	string not being a valid window spec), then the return value is
 *	TCL_ERROR and an error message is left in the interp's result if
 *	interp is non-NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpUseWindow(
    Tcl_Interp *interp,		/* If not NULL, used for error reporting if
				 * string is bogus. */
    Tk_Window tkwin,		/* Tk window that does not yet have an
				 * associated X window. */
    const char *string)		/* String identifying an X window to use for
				 * tkwin; must be an integer value. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkWindow *usePtr;
    MacDrawable *parent, *macWin;
    Container *containerPtr;

    if (winPtr->window != None) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"can't modify container after widget is created", -1));
	Tcl_SetErrorCode(interp, "TK", "EMBED", "POST_CREATE", NULL);
	return TCL_ERROR;
    }

    /*
     * Decode the container window ID, and look for it among the list of
     * available containers.
     *
     * N.B. For now, we are limiting the containers to be in the same Tk
     * application as tkwin, since otherwise they would not be in our list of
     * containers.
     */

    if (TkpScanWindowId(interp, string, (Window *)&parent) != TCL_OK) {
	return TCL_ERROR;
    }

    usePtr = (TkWindow *) Tk_IdToWindow(winPtr->display, (Window) parent);
    if (usePtr == NULL) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"couldn't create child of window \"%s\"", string));
	    Tcl_SetErrorCode(interp, "TK", "EMBED", "NO_TARGET", NULL);
	}
	return TCL_ERROR;
    } else if (!(usePtr->flags & TK_CONTAINER)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"window \"%s\" doesn't have -container option set",
		usePtr->pathName));
	Tcl_SetErrorCode(interp, "TK", "EMBED", "CONTAINER", NULL);
	return TCL_ERROR;
    }

    /*
     * Since we do not allow embedding into windows belonging to a different
     * process, we know that a container will exist showing the parent window
     * as the parent.  This loop finds that container.
     */

    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->parent == (Window) parent) {
	    winPtr->flags |= TK_BOTH_HALVES;
	    containerPtr->parentPtr->flags |= TK_BOTH_HALVES;
	    break;
	}
    }

    /*
     * Make the embedded window.
     */

    macWin = ckalloc(sizeof(MacDrawable));
    if (macWin == NULL) {
	winPtr->privatePtr = NULL;
	return TCL_ERROR;
    }

    macWin->winPtr = winPtr;
    macWin->view = nil;
    macWin->context = NULL;
    macWin->size = CGSizeZero;
    macWin->visRgn = NULL;
    macWin->aboveVisRgn = NULL;
    macWin->drawRgn = NULL;
    macWin->referenceCount = 0;
    macWin->flags = TK_CLIP_INVALID;
    macWin->toplevel = macWin;
    macWin->toplevel->referenceCount++;

    winPtr->privatePtr = macWin;
    winPtr->flags |= TK_EMBEDDED;

    /*
     * Make a copy of the TK_EMBEDDED flag, since sometimes we need this to
     * get the port after the TkWindow structure has been freed.
     */

    macWin->flags |= TK_EMBEDDED;
    macWin->xOff = parent->winPtr->privatePtr->xOff +
	    parent->winPtr->changes.border_width +
	    winPtr->changes.x;
    macWin->yOff = parent->winPtr->privatePtr->yOff +
	    parent->winPtr->changes.border_width +
	    winPtr->changes.y;

    /*
     * Finish filling up the container structure with the embedded window's
     * information.
     */

    containerPtr->embedded = (Window) macWin;
    containerPtr->embeddedPtr = macWin->winPtr;

    /*
     * Create an event handler to clean up the Container structure when
     * tkwin is eventually deleted.
     */

    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	    EmbeddedEventProc, winPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeContainer --
 *
 *	This procedure is called to indicate that a particular window will be
 *	a container for an embedded application. This changes certain aspects
 *	of the window's behavior, such as whether it will receive events
 *	anymore.
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
TkpMakeContainer(
    Tk_Window tkwin)		/* Token for a window that is about to become
				 * a container. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    Container *containerPtr;

    /*
     * Register the window as a container so that, for example, we can make
     * sure the argument to -use is valid.
     */

    Tk_MakeWindowExist(tkwin);
    containerPtr = ckalloc(sizeof(Container));
    containerPtr->parent = Tk_WindowId(tkwin);
    containerPtr->parentPtr = winPtr;
    containerPtr->embedded = None;
    containerPtr->embeddedPtr = NULL;
    containerPtr->nextPtr = firstContainerPtr;
    firstContainerPtr = containerPtr;
    winPtr->flags |= TK_CONTAINER;

    /*
     * Request SubstructureNotify events so that we can find out when the
     * embedded application creates its window or attempts to resize it. Also
     * watch Configure events on the container so that we can resize the child
     * to match. Also, pass activate events from the container down to the
     * embedded toplevel.
     */

    Tk_CreateEventHandler(tkwin,
	    SubstructureNotifyMask|SubstructureRedirectMask,
	    ContainerEventProc, winPtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, EmbedStructureProc,
	    containerPtr);
    Tk_CreateEventHandler(tkwin, ActivateMask, EmbedActivateProc,
	    containerPtr);
    Tk_CreateEventHandler(tkwin, FocusChangeMask, EmbedFocusProc,
	    containerPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXContainerId --
 *
 *	Given an embedded window, this procedure returns the MacDrawable
 *	identifier for the associated container window.
 *
 * Results:
 *	The return value is the MacDrawable for winPtr's container window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MacDrawable *
TkMacOSXContainerId(
    TkWindow *winPtr)		/* Tk's structure for an embedded window. */
{
    Container *containerPtr;

    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->embeddedPtr == winPtr) {
	    return (MacDrawable *) containerPtr->parent;
	}
    }
    Tcl_Panic("TkMacOSXContainerId couldn't find window");
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetHostToplevel --
 *
 *	Given the TkWindow, return the MacDrawable for the outermost toplevel
 *	containing it. This will be a real Macintosh window.
 *
 * Results:
 *	Returns a MacDrawable corresponding to a Macintosh Toplevel
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MacDrawable *
TkMacOSXGetHostToplevel(
    TkWindow *winPtr)		/* Tk's structure for a window. */
{
    TkWindow *contWinPtr, *topWinPtr;

    topWinPtr = winPtr->privatePtr->toplevel->winPtr;
    if (!Tk_IsEmbedded(topWinPtr)) {
	return winPtr->privatePtr->toplevel;
    }
    contWinPtr = TkpGetOtherWindow(topWinPtr);

    /*
     * TODO: Here we should handle out of process embedding.
     */

    if (!contWinPtr) {
	return NULL;
    }
    return TkMacOSXGetHostToplevel(contWinPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpClaimFocus --
 *
 *	This procedure is invoked when someone asks for the input focus to be
 *	put on a window in an embedded application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The input focus may change.
 *
 *----------------------------------------------------------------------
 */

void
TkpClaimFocus(
    TkWindow *topLevelPtr,	/* Top-level window containing desired focus
				 * window; should be embedded. */
    int force)			/* One means that the container should claim
				 * the focus if it doesn't currently have
				 * it. */
{
    XEvent event;
    Container *containerPtr;

    if (!(topLevelPtr->flags & TK_EMBEDDED)) {
	return;
    }

    for (containerPtr = firstContainerPtr;
	    containerPtr->embeddedPtr != topLevelPtr;
	    containerPtr = containerPtr->nextPtr) {
	/* Empty loop body. */
    }

    event.xfocus.type = FocusIn;
    event.xfocus.serial = LastKnownRequestProcessed(topLevelPtr->display);
    event.xfocus.send_event = 1;
    event.xfocus.display = topLevelPtr->display;
    event.xfocus.window = containerPtr->parent;
    event.xfocus.mode = EMBEDDED_APP_WANTS_FOCUS;
    event.xfocus.detail = force;
    Tk_HandleEvent(&event);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpTestembedCmd --
 *
 *	This procedure implements the "testembed" command. It returns some or
 *	all of the information in the list pointed to by firstContainerPtr.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpTestembedCmd(
    ClientData clientData,	/* Main window for application. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument strings. */
{
    int all;
    Container *containerPtr;
    Tcl_DString dString;
    char buffer[50];
    Tcl_Interp *embeddedInterp = NULL, *parentInterp = NULL;

    if ((objc > 1) && (strcmp(Tcl_GetString(objv[1]), "all") == 0)) {
	all = 1;
    } else {
	all = 0;
    }
    Tcl_DStringInit(&dString);
    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->embeddedPtr != NULL) {
	    embeddedInterp = containerPtr->embeddedPtr->mainPtr->interp;
	}
	if (containerPtr->parentPtr != NULL) {
	    parentInterp = containerPtr->parentPtr->mainPtr->interp;
	}
	if (embeddedInterp != interp && parentInterp != interp) {
	    continue;
	}
	Tcl_DStringStartSublist(&dString);

	/*
	 * Parent id
	 */

	if (containerPtr->parent == None) {
	    Tcl_DStringAppendElement(&dString, "");
	} else if (all) {
	    sprintf(buffer, "0x%lx", containerPtr->parent);
	    Tcl_DStringAppendElement(&dString, buffer);
	} else {
	    Tcl_DStringAppendElement(&dString, "XXX");
	}

	/*
	 * Parent pathName
	 */

	if (containerPtr->parentPtr == NULL ||
	    parentInterp != interp) {
	    Tcl_DStringAppendElement(&dString, "");
	} else {
	    Tcl_DStringAppendElement(&dString,
		    containerPtr->parentPtr->pathName);
	}

	/*
	 * On X11 embedded is a wrapper, which does not exist on macOS.
	 */

	Tcl_DStringAppendElement(&dString, "");

	/*
	 * Embedded window pathName
	 */

	if (containerPtr->embeddedPtr == NULL ||
	    embeddedInterp != interp) {
	    Tcl_DStringAppendElement(&dString, "");
	} else {
	    Tcl_DStringAppendElement(&dString,
		    containerPtr->embeddedPtr->pathName);
	}
	Tcl_DStringEndSublist(&dString);
    }
    Tcl_DStringResult(interp, &dString);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpRedirectKeyEvent --
 *
 *	This procedure is invoked when a key press or release event arrives
 *	for an application that does not believe it owns the input focus. This
 *	can happen because of embedding; for example, X can send an event to
 *	an embedded application when the real focus window is in the container
 *	application and is an ancestor of the container. This procedure's job
 *	is to forward the event back to the application where it really
 *	belongs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The event may get sent to a different application.
 *
 *----------------------------------------------------------------------
 */

void
TkpRedirectKeyEvent(
    TkWindow *winPtr,		/* Window to which the event was originally
				 * reported. */
    XEvent *eventPtr)		/* X event to redirect (should be KeyPress or
				 * KeyRelease). */
{
    /* TODO: Implement this or decide it definitely needs no implementation */
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetOtherWindow --
 *
 *	If both the container and embedded window are in the same process,
 *	this procedure will return either one, given the other.
 *
 * Results:
 *	If winPtr is a container, the return value is the token for the
 *	embedded window, and vice versa. If the "other" window isn't in this
 *	process, NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkpGetOtherWindow(
    TkWindow *winPtr)		/* Tk's structure for a container or embedded
				 * window. */
{
    Container *containerPtr;

    /*
     * TkpGetOtherWindow returns NULL if both windows are not in the same
     * process...
     */

    if (!(winPtr->flags & TK_BOTH_HALVES)) {
	return NULL;
    }

    for (containerPtr = firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->embeddedPtr == winPtr) {
	    return containerPtr->parentPtr;
	} else if (containerPtr->parentPtr == winPtr) {
	    return containerPtr->embeddedPtr;
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * EmbeddedEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when various
 *	useful events are received for a window that is embedded in
 *	another application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Our internal state gets cleaned up when an embedded window is
 *	destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
EmbeddedEventProc(
    ClientData clientData,	/* Token for container window. */
    XEvent *eventPtr)		/* ResizeRequest event. */
{
    TkWindow *winPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	EmbedWindowDeleted(winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ContainerEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when various
 *	useful events are received for the children of a container window. It
 *	forwards relevant information, such as geometry requests, from the
 *	events into the container's application.
 *
 *	NOTE: on the Mac, only the DestroyNotify branch is ever taken. We
 *	don't synthesize the other events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the event. For example, when ConfigureRequest events occur,
 *	geometry information gets set for the container window.
 *
 *----------------------------------------------------------------------
 */

static void
ContainerEventProc(
    ClientData clientData,	/* Token for container window. */
    XEvent *eventPtr)		/* ResizeRequest event. */
{
    TkWindow *winPtr = clientData;
    Container *containerPtr;
    Tk_ErrorHandler errHandler;

    if (!firstContainerPtr) {
	/*
	 * When the interpreter is being dismantled this can be nil.
	 */

	return;
    }

    /*
     * Ignore any X protocol errors that happen in this procedure (almost any
     * operation could fail, for example, if the embedded application has
     * deleted its window).
     */

    errHandler = Tk_CreateErrorHandler(eventPtr->xfocus.display, -1,
	    -1, -1, NULL, NULL);

    /*
     * Find the Container structure associated with the parent window.
     */

    for (containerPtr = firstContainerPtr;
	    containerPtr->parent != eventPtr->xmaprequest.parent;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr == NULL) {
	    Tcl_Panic("ContainerEventProc couldn't find Container record");
	}
    }

    if (eventPtr->type == CreateNotify) {
	/*
	 * A new child window has been created in the container. Record its id
	 * in the Container structure (if more than one child is created, just
	 * remember the last one and ignore the earlier ones).
	 */

	containerPtr->embedded = eventPtr->xcreatewindow.window;
    } else if (eventPtr->type == ConfigureRequest) {
	if ((eventPtr->xconfigurerequest.x != 0)
		|| (eventPtr->xconfigurerequest.y != 0)) {
	    /*
	     * The embedded application is trying to move itself, which isn't
	     * legal. At this point, the window hasn't actually moved, but we
	     * need to send it a ConfigureNotify event to let it know that its
	     * request has been denied. If the embedded application was also
	     * trying to resize itself, a ConfigureNotify will be sent by the
	     * geometry management code below, so we don't need to do
	     * anything. Otherwise, generate a synthetic event.
	     */

	    if ((eventPtr->xconfigurerequest.width == winPtr->changes.width)
		    && (eventPtr->xconfigurerequest.height
		    == winPtr->changes.height)) {
		EmbedSendConfigure(containerPtr);
	    }
	}
	EmbedGeometryRequest(containerPtr,
		eventPtr->xconfigurerequest.width,
		eventPtr->xconfigurerequest.height);
    } else if (eventPtr->type == MapRequest) {
	/*
	 * The embedded application's map request was ignored and simply
	 * passed on to us, so we have to map the window for it to appear on
	 * the screen.
	 */

	XMapWindow(eventPtr->xmaprequest.display,
		eventPtr->xmaprequest.window);
    } else if (eventPtr->type == DestroyNotify) {
	/*
	 * It is not clear whether the container should be destroyed
	 * when an embedded window is destroyed.  See ticket [67384bce7d].
	 * Here we are following unix, by destroying the container.
	 */

	Tk_DestroyWindow((Tk_Window) winPtr);
    }
    Tk_DeleteErrorHandler(errHandler);
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedStructureProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when a container
 *	window owned by this application gets resized (and also at several
 *	other times that we don't care about). This procedure reflects the
 *	size change in the embedded window that corresponds to the container.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The embedded window gets resized to match the container.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedStructureProc(
    ClientData clientData,	/* Token for container window. */
    XEvent *eventPtr)		/* ResizeRequest event. */
{
    Container *containerPtr = clientData;
    Tk_ErrorHandler errHandler;

    if (eventPtr->type == ConfigureNotify) {

	/*
         * Send a ConfigureNotify  to the embedded application.
         */

        if (containerPtr->embeddedPtr != None) {
            TkDoConfigureNotify(containerPtr->embeddedPtr);
        }
	if (containerPtr->embedded != None) {
	    /*
	     * Ignore errors, since the embedded application could have
	     * deleted its window.
	     */

	    errHandler = Tk_CreateErrorHandler(eventPtr->xfocus.display, -1,
		    -1, -1, NULL, NULL);
	    Tk_MoveResizeWindow((Tk_Window) containerPtr->embeddedPtr, 0, 0,
		    (unsigned) Tk_Width((Tk_Window) containerPtr->parentPtr),
		    (unsigned) Tk_Height((Tk_Window)containerPtr->parentPtr));
	    Tk_DeleteErrorHandler(errHandler);
	}
    } else if (eventPtr->type == DestroyNotify) {
	EmbedWindowDeleted(containerPtr->parentPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedActivateProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when Activate and
 *	Deactivate events occur for a container window owned by this
 *	application. It is responsible for forwarding an activate event down
 *	into the embedded toplevel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The X focus may change.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedActivateProc(
    ClientData clientData,	/* Token for container window. */
    XEvent *eventPtr)		/* ResizeRequest event. */
{
    Container *containerPtr = clientData;

    if (containerPtr->embeddedPtr != NULL) {
	if (eventPtr->type == ActivateNotify) {
	    TkGenerateActivateEvents(containerPtr->embeddedPtr,1);
	} else if (eventPtr->type == DeactivateNotify) {
	    TkGenerateActivateEvents(containerPtr->embeddedPtr,0);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedFocusProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when FocusIn and
 *	FocusOut events occur for a container window owned by this
 *	application. It is responsible for moving the focus back and forth
 *	between a container application and an embedded application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The X focus may change.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedFocusProc(
    ClientData clientData,	/* Token for container window. */
    XEvent *eventPtr)		/* ResizeRequest event. */
{
    Container *containerPtr = clientData;
    Display *display;
    XEvent event;

    if (containerPtr->embeddedPtr != NULL) {
	display = Tk_Display(containerPtr->parentPtr);
	event.xfocus.serial = LastKnownRequestProcessed(display);
	event.xfocus.send_event = false;
	event.xfocus.display = display;
	event.xfocus.mode = NotifyNormal;
	event.xfocus.window = containerPtr->embedded;

	if (eventPtr->type == FocusIn) {
	    /*
	     * The focus just arrived at the container. Change the X focus to
	     * move it to the embedded application, if there is one. Ignore X
	     * errors that occur during this operation (it's possible that the
	     * new focus window isn't mapped).
	     */

	    event.xfocus.detail = NotifyNonlinear;
	    event.xfocus.type = FocusIn;
	} else if (eventPtr->type == FocusOut) {
	    /*
	     * When the container gets a FocusOut event, it has to tell the
	     * embedded app that it has lost the focus.
	     */

	    event.xfocus.type = FocusOut;
	    event.xfocus.detail = NotifyNonlinear;
	}

	Tk_QueueWindowEvent(&event, TCL_QUEUE_MARK);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedGeometryRequest --
 *
 *	This procedure is invoked when an embedded application requests a
 *	particular size. It processes the request (which may or may not
 *	actually honor the request) and reflects the results back to the
 *	embedded application.
 *
 *	NOTE: On the Mac, this is a stub, since we don't synthesize
 *	ConfigureRequest events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If we deny the child's size change request, a Configure event is
 *	synthesized to let the child know how big it ought to be. Events get
 *	processed while we're waiting for the geometry managers to do their
 *	thing.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedGeometryRequest(
    Container *containerPtr,	/* Information about the embedding. */
    int width, int height)	/* Size that the child has requested. */
{
    TkWindow *winPtr = containerPtr->parentPtr;

    /*
     * Forward the requested size into our geometry management hierarchy via
     * the container window. We need to send a Configure event back to the
     * embedded application if we decide not to honor its request; to make this
     * happen, process all idle event handlers synchronously here (so that the
     * geometry managers have had a chance to do whatever they want to do), and
     * if the window's size didn't change then generate a configure event.
     */

    Tk_GeometryRequest((Tk_Window) winPtr, width, height);
    while (Tcl_DoOneEvent(TCL_IDLE_EVENTS)) {
	/* Empty loop body. */
    }
    if ((winPtr->changes.width != width)
	    || (winPtr->changes.height != height)) {
	EmbedSendConfigure(containerPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedSendConfigure --
 *
 *	This is currently a stub. It is called to notify an embedded
 *	application of its current size and location. This procedure is called
 *	when the embedded application made a geometry request that we did not
 *	grant, so that the embedded application knows that its geometry didn't
 *	change after all. It is a response to ConfigureRequest events, which we
 *	do not currently synthesize on the Mac
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
EmbedSendConfigure(
    Container *containerPtr)	/* Information about the embedding. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedWindowDeleted --
 *
 *	This procedure is invoked when a window involved in embedding (as
 *	either the container or the embedded application) is destroyed. It
 *	cleans up the Container structure for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Container structure may be freed.
 *
 *----------------------------------------------------------------------
 */

static void
EmbedWindowDeleted(
    TkWindow *winPtr)		/* Tk's information about window that
				 * was deleted. */
{
    Container *containerPtr, *prevPtr;

    /*
     * Find the Container structure for this window. Delete the information
     * about the embedded application and free the container's record.
     */

    prevPtr = NULL;
    containerPtr = firstContainerPtr;
    while (1) {
	if (containerPtr->embeddedPtr == winPtr) {
	    /*
	     * We also have to destroy our parent, to clean up the container.
	     * Fabricate an event to do this.
	     */

	    if (containerPtr->parentPtr != NULL &&
		    containerPtr->parentPtr->flags & TK_BOTH_HALVES) {
		XEvent event;

		event.xany.serial = LastKnownRequestProcessed(
			Tk_Display(containerPtr->parentPtr));
		event.xany.send_event = False;
		event.xany.display = Tk_Display(containerPtr->parentPtr);

		event.xany.type = DestroyNotify;
		event.xany.window = containerPtr->parent;
		event.xdestroywindow.event = containerPtr->parent;
		Tk_QueueWindowEvent(&event, TCL_QUEUE_HEAD);
	    }

	    containerPtr->embedded = None;
	    containerPtr->embeddedPtr = NULL;
	    break;
	}
	if (containerPtr->parentPtr == winPtr) {
	    containerPtr->parentPtr = NULL;
	    break;
	}
	prevPtr = containerPtr;
	containerPtr = containerPtr->nextPtr;
    }
    if ((containerPtr->embeddedPtr == NULL)
	    && (containerPtr->parentPtr == NULL)) {
	if (prevPtr == NULL) {
	    firstContainerPtr = containerPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = containerPtr->nextPtr;
	}
	ckfree(containerPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpShowBusyWindow, TkpHideBusyWindow, TkpMakeTransparentWindowExist,
 * TkpCreateBusy --
 *
 *	Portability layer for busy windows. Holds platform-specific gunk for
 *	the [tk busy] command, which is currently a dummy implementation for
 *	OSX/Aqua. The individual functions are supposed to do the following:
 *
 * TkpShowBusyWindow --
 *	Make the busy window appear.
 *
 * TkpHideBusyWindow --
 *	Make the busy window go away.
 *
 * TkpMakeTransparentWindowExist --
 *	Actually make a transparent window.
 *
 * TkpCreateBusy --
 *	Creates the platform-specific part of a busy window structure.
 *
 *----------------------------------------------------------------------
 */

void
TkpShowBusyWindow(
    TkBusy busy)
{
}

void
TkpHideBusyWindow(
    TkBusy busy)
{
}

void
TkpMakeTransparentWindowExist(
    Tk_Window tkwin,		/* Token for window. */
    Window parent)		/* Parent window. */
{
}

void
TkpCreateBusy(
    Tk_FakeWin *winPtr,
    Tk_Window tkRef,
    Window* parentPtr,
    Tk_Window tkParent,
    TkBusy busy)
{
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
