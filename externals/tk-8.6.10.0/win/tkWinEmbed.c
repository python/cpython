/*
 * tkWinEmbed.c --
 *
 *	This file contains platform specific procedures for Windows platforms
 *	to provide basic operations needed for application embedding (where
 *	one application can use as its main window an internal window from
 *	another application).
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"

/*
 * One of the following structures exists for each container in this
 * application. It keeps track of the container window and its associated
 * embedded window.
 */

typedef struct Container {
    HWND parentHWnd;		/* Windows HWND to the parent window */
    TkWindow *parentPtr;	/* Tk's information about the container or
				 * NULL if the container isn't in this
				 * process. */
    HWND embeddedHWnd;		/* Windows HWND to the embedded window. */
    TkWindow *embeddedPtr;	/* Tk's information about the embedded window,
				 * or NULL if the embedded application isn't
				 * in this process. */
    HWND embeddedMenuHWnd;	/* Tk's embedded menu window handler. */
    struct Container *nextPtr;	/* Next in list of all containers in this
				 * process. */
} Container;

typedef struct {
    Container *firstContainerPtr;
				/* First in list of all containers managed by
				 * this process. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

static void		ContainerEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		EmbedGeometryRequest(Container *containerPtr,
			    int width, int height);
static void		EmbedWindowDeleted(TkWindow *winPtr);
static void		Tk_MapEmbeddedWindow(TkWindow* winPtr);
HWND			Tk_GetEmbeddedHWnd(TkWindow* winPtr);

/*
 *----------------------------------------------------------------------
 *
 * TkWinCleanupContainerList --
 *
 *	Finalizes the list of containers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases memory occupied by containers of embedded windows.
 *
 *----------------------------------------------------------------------
 */

void
TkWinCleanupContainerList(void)
{
    Container *nextPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (; tsdPtr->firstContainerPtr != NULL;
	    tsdPtr->firstContainerPtr = nextPtr) {
	nextPtr = tsdPtr->firstContainerPtr->nextPtr;
	ckfree(tsdPtr->firstContainerPtr);
    }
    tsdPtr->firstContainerPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpTestembedCmd --
 *
 *	Test command for the embedding facility.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	Currently it does not do anything.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkpTestembedCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DetachEmbeddedWindow --
 *
 *	This function detaches an embedded window
 *
 * Results:
 *	No return value. Detach the embedded window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static
void
Tk_DetachEmbeddedWindow(
    TkWindow *winPtr,		/* an embedded window */
    BOOL detachFlag)		/* a flag of truely detaching */
{
    TkpWinToplevelDetachWindow(winPtr);
    if(detachFlag) {
	TkpWinToplevelOverrideRedirect(winPtr, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MapEmbeddedWindow --
 *
 *	This function is required for mapping an embedded window during idle.
 *	The input winPtr must be preserved using Tcl_Preserve before call this
 *	function and will be released by this function.
 *
 * Results:
 *	No return value. Map the embedded window if it is not dead.
 *
 * Side effects:
 *	The embedded window may change its state as the container's.
 *
 *----------------------------------------------------------------------
 */

static
void Tk_MapEmbeddedWindow(
    TkWindow *winPtr)		/* Top-level window that's about to be
				 * mapped. */
{
    if(!(winPtr->flags & TK_ALREADY_DEAD)) {
	HWND hwnd = (HWND)winPtr->privatePtr;
	int state = SendMessageW(hwnd, TK_STATE, -1, -1) - 1;

	if (state < 0 || state > 3) {
	    state = NormalState;
	}

	while (Tcl_DoOneEvent(TCL_IDLE_EVENTS)) {
	    /* empty body */
	}

	TkpWmSetState(winPtr, state);
	TkWmMapWindow(winPtr);
    }
    Tcl_Release((ClientData)winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpUseWindow --
 *
 *	This procedure causes a Tk window to use a given Windows handle for a
 *	window as its underlying window, rather than a new Windows window
 *	being created automatically. It is invoked by an embedded application
 *	to specify the window in which the application is embedded.
 *
 *	This procedure uses a simple attachment protocol by sending TK_INFO
 *	messages to the window to use with two sub messages:
 *
 *	    TK_CONTAINER_VERIFY - if a window handles this message, it should
 *		return either a (long)hwnd for a container or a -(long)hwnd
 *		for a non-container.
 *
 *	    TK_CONTAINER_ISAVAILABLE - a container window should return either
 *		a TRUE (non-zero) if it is available for use or a FALSE (zero)
 *		othersize.
 *
 *	The TK_INFO messages are required in order to verify if the window to
 *	use is a valid container. Without an id verification, an invalid
 *	window attachment may cause unexpected crashes/panics (bug 1096074).
 *	Additional sub messages may be defined/used in future for other
 *	needs.
 *
 *	We do not enforce the above protocol for the reason of backward
 *	compatibility. If the window to use is unable to handle TK_INFO
 *	messages (e.g., legacy Tk container applications before 8.5), a dialog
 *	box with a warning message pops up and the user is asked to confirm if
 *	the attachment should proceed. However, we may have to enforce it in
 *	future.
 *
 * Results:
 *	The return value is normally TCL_OK. If an error occurred (such as if
 *	the argument does not identify a legal Windows window handle or it is
 *	already in use or a cancel button is pressed by a user in confirming
 *	the use window as a Tk container) the return value is TCL_ERROR and an
 *	error message is left in the the interp's result if interp is not
 *	NULL.
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
    int id;
    HWND hwnd;
/*
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
*/

/*
    if (winPtr->window != None) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"can't modify container after widget is created", -1));
	Tcl_SetErrorCode(interp, "TK", "EMBED", "POST_CREATE", NULL);
	return TCL_ERROR;
    }
*/

    if (strcmp(string, "") == 0) {
	if (winPtr->flags & TK_EMBEDDED) {
	    Tk_DetachEmbeddedWindow(winPtr, TRUE);
	}
	return TCL_OK;
    }

    if (
#ifdef _WIN64
	    (sscanf(string, "0x%p", &hwnd) != 1) &&
#endif
	    Tcl_GetInt(interp, string, (int *) &hwnd) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((HWND)winPtr->privatePtr == hwnd) {
	return TCL_OK;
    }

    /*
     * Check if the window is a valid handle. If it is invalid, return
     * TCL_ERROR and potentially leave an error message in the interp's
     * result.
     */

    if (!IsWindow(hwnd)) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" doesn't exist", string));
	    Tcl_SetErrorCode(interp, "TK", "EMBED", "EXIST", NULL);
	}
	return TCL_ERROR;
    }

    id = SendMessageW(hwnd, TK_INFO, TK_CONTAINER_VERIFY, 0);
    if (id == PTR2INT(hwnd)) {
	if (!SendMessageW(hwnd, TK_INFO, TK_CONTAINER_ISAVAILABLE, 0)) {
    	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "The container is already in use", -1));
	    Tcl_SetErrorCode(interp, "TK", "EMBED", "IN_USE", NULL);
	    return TCL_ERROR;
	}
    } else if (id == -PTR2INT(hwnd)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"the window to use is not a Tk container", -1));
	Tcl_SetErrorCode(interp, "TK", "EMBED", "CONTAINER", NULL);
	return TCL_ERROR;
    } else {
	/*
	 * Proceed if the user decide to do so because it can be a legacy
	 * container application. However we may have to return a TCL_ERROR in
	 * order to avoid bug 1096074 in future.
	 */

	WCHAR msg[256];

	wsprintfW(msg, L"Unable to get information of window \"%.40hs\".  Attach to this\nwindow may have unpredictable results if it is not a valid container.\n\nPress Ok to proceed or Cancel to abort attaching.", string);
	if (IDCANCEL == MessageBoxW(hwnd, msg, L"Tk Warning",
		MB_OKCANCEL | MB_ICONWARNING)) {
    	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "Operation has been canceled", -1));
	    Tcl_SetErrorCode(interp, "TK", "EMBED", "CANCEL", NULL);
	    return TCL_ERROR;
	}
    }

    Tk_DetachEmbeddedWindow(winPtr, FALSE);

    /*
     * Store the parent window in the platform private data slot so
     * TkWmMapWindow can use it when creating the wrapper window.
     */

    winPtr->privatePtr = (struct TkWindowPrivate*) hwnd;
    winPtr->flags |= TK_EMBEDDED;
    winPtr->flags &= ~(TK_MAPPED);

    /*
     * Preserve the winPtr and create an idle handler to map the embedded
     * window.
     */

    Tcl_Preserve((ClientData) winPtr);
    Tcl_DoWhenIdle((Tcl_IdleProc*) Tk_MapEmbeddedWindow, (ClientData) winPtr);

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
    Tk_Window tkwin)
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Register the window as a container so that, for example, we can find
     * out later if the embedded app. is in the same process.
     */

    Tk_MakeWindowExist(tkwin);
    containerPtr = ckalloc(sizeof(Container));
    containerPtr->parentPtr = winPtr;
    containerPtr->parentHWnd = Tk_GetHWND(Tk_WindowId(tkwin));
    containerPtr->embeddedHWnd = NULL;
    containerPtr->embeddedPtr = NULL;
    containerPtr->embeddedMenuHWnd = NULL;
    containerPtr->nextPtr = tsdPtr->firstContainerPtr;
    tsdPtr->firstContainerPtr = containerPtr;
    winPtr->flags |= TK_CONTAINER;

    /*
     * Unlike in tkUnixEmbed.c, we don't make any requests for events in the
     * embedded window here. Now we just allow the embedding of another TK
     * application into TK windows. When the embedded window makes a request,
     * that will be done by sending to the container window a WM_USER message,
     * which will be intercepted by TkWinContainerProc.
     *
     * We need to get structure events of the container itself, though.
     */

    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	    ContainerEventProc, (ClientData) containerPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinEmbeddedEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when various
 *	useful events are received for the *children* of a container window.
 *	It forwards relevant information, such as geometry requests, from the
 *	events into the container's application.
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

LRESULT
TkWinEmbeddedEventProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    int result = 1;
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Find the Container structure associated with the parent window.
     */

    for (containerPtr = tsdPtr->firstContainerPtr;
	    containerPtr && containerPtr->parentHWnd != hwnd;
	    containerPtr = containerPtr->nextPtr) {
	/* empty loop body */
    }

    if (containerPtr) {
	TkWindow *topwinPtr = NULL;
	if(Tk_IsTopLevel(containerPtr->parentPtr)) {
	    topwinPtr = containerPtr->parentPtr;
	}
	switch (message) {
	case TK_INFO:
	    /*
	     * An embedded window may send this message for container
	     * verification and availability before attach.
	     *
	     * wParam - a sub message
	     *
	     *	    TK_CONTAINER_ISAVAILABLE - if the container is available
	     *		for use?
	     *		result = 1 for yes and 0 for no;
	     *
	     *	    TK_CONTAINER_VERIFY - request the container to verify its
	     *		identification
	     *		result =  (long)hwnd if this window is a container
	     *			 -(long)hwnd otherwise
	     *
	     * lParam - N/A
	     */

	    switch(wParam) {
	    case TK_CONTAINER_ISAVAILABLE:
		result = containerPtr->embeddedHWnd == NULL? 1:0;
		break;
	    case TK_CONTAINER_VERIFY:
		result = PTR2INT(containerPtr->parentHWnd);
		break;
	    default:
		result = 0;
	    }
	    break;

	case TK_ATTACHWINDOW:
	    /*
	     * An embedded window (either from this application or from
	     * another application) is trying to attach to this container. We
	     * attach it only if this container is not yet containing any
	     * window.
	     *
	     * wParam - a handle of an embedded window
	     * lParam - N/A
	     *
	     * An embedded window may send this message with a wParam of NULL
	     * to test if a window is able to provide embedding service. The
	     * container returns its window handle for accepting the
	     * attachment and identifying itself or a zero for being already
	     * in use.
	     *
	     * Return value:
	     * 0    - the container is unable to be used.
	     * hwnd - the container is ready to be used.
	     */
	    if (containerPtr->embeddedHWnd == NULL) {
		if (wParam) {
		    TkWindow *winPtr = (TkWindow *)
			    Tk_HWNDToWindow((HWND) wParam);
		    if (winPtr) {
			winPtr->flags |= TK_BOTH_HALVES;
			containerPtr->embeddedPtr = winPtr;
			containerPtr->parentPtr->flags |= TK_BOTH_HALVES;
		    }
		    containerPtr->embeddedHWnd = (HWND)wParam;
		}
		result = PTR2INT(containerPtr->parentHWnd);
	    } else {
		result = 0;
	    }
	    break;

	case TK_DETACHWINDOW:
	    /*
	     * An embedded window notifies the container that it is detached.
	     * The container should clearn the related variables and redraw
	     * its window.
	     *
	     * wParam - N/A
	     * lParam - N/A
	     *
	     * Return value:
	     * 0	- the message is not processed.
	     * others	- the message is processed.
	     */

	    containerPtr->embeddedMenuHWnd = NULL;
	    containerPtr->embeddedHWnd = NULL;
	    containerPtr->parentPtr->flags &= ~TK_BOTH_HALVES;
	    if (topwinPtr) {
		TkWinSetMenu((Tk_Window) topwinPtr, 0);
	    }
	    InvalidateRect(hwnd, NULL, TRUE);
	    break;

	case TK_GEOMETRYREQ:
	    /*
	     * An embedded window requests a window size change.
	     *
	     * wParam - window width
	     * lParam - window height
	     *
	     * Return value:
	     * 0	- the message is not processed.
	     * others	- the message is processed.
	     */

	    EmbedGeometryRequest(containerPtr, (int)wParam, lParam);
	    break;

	case TK_RAISEWINDOW:
	    /*
	     * An embedded window requests to change its Z-order.
	     *
	     * wParam - a window handle as a z-order stack reference
	     * lParam - a flag of above-below: 0 - above; 1 or others: - below
	     *
	     * Return value:
	     * 0	- the message is not processed.
	     * others	- the message is processed.
	     */

	    TkWinSetWindowPos(GetParent(containerPtr->parentHWnd),
		    (HWND)wParam, (int)lParam);
	    break;

	case TK_GETFRAMEWID:
	    /*
	     * An embedded window requests to get the frame window's id.
	     *
	     * wParam - N/A
	     * lParam - N/A
	     *
	     * Return vlaue:
	     *
	     * A handle of the frame window. If it is not availble, a zero is
	     * returned.
	     */
	    if (topwinPtr) {
		result = PTR2INT(GetParent(containerPtr->parentHWnd));
	    } else {
		topwinPtr = containerPtr->parentPtr;
		while (!(topwinPtr->flags & TK_TOP_HIERARCHY)) {
		    topwinPtr = topwinPtr->parentPtr;
		}
		if (topwinPtr && topwinPtr->window) {
		    result = PTR2INT(GetParent(Tk_GetHWND(topwinPtr->window)));
		} else {
		    result = 0;
		}
	    }
	    break;

	case TK_CLAIMFOCUS:
	    /*
	     * An embedded window requests a focus.
	     *
	     * wParam - a flag of forcing focus
	     * lParam - N/A
	     *
	     * Return value:
	     * 0    - the message is not processed
	     * 1    - the message is processed
	     */

	    if (!SetFocus(containerPtr->embeddedHWnd) && wParam) {
		/*
		 * forcing focus TBD
		 */
	    }
	    break;

	case TK_WITHDRAW:
	    /*
	     * An embedded window requests withdraw.
	     *
	     * wParam	- N/A
	     * lParam	- N/A
	     *
	     * Return value
	     * 0    - the message is not processed
	     * 1    - the message is processed
	     */

	    if (topwinPtr) {
		TkpWinToplevelWithDraw(topwinPtr);
	    } else {
		result = 0;
	    }
	    break;

	case TK_ICONIFY:
	    /*
	     * An embedded window requests iconification.
	     *
	     * wParam	- N/A
	     * lParam	- N/A
	     *
	     * Return value
	     * 0    - the message is not processed
	     * 1    - the message is processed
	     */

	    if (topwinPtr) {
		TkpWinToplevelIconify(topwinPtr);
	    } else {
		result = 0;
	    }
	    break;

	case TK_DEICONIFY:
	    /*
	     * An embedded window requests deiconification.
	     *
	     * wParam	- N/A
	     * lParam	- N/A
	     *
	     * Return value
	     * 0    - the message is not processed
	     * 1    - the message is processed
	     */
	    if (topwinPtr) {
		TkpWinToplevelDeiconify(topwinPtr);
	    } else {
		result = 0;
	    }
	    break;

	case TK_MOVEWINDOW:
	    /*
	     * An embedded window requests to move position if both wParam and
	     * lParam are greater or equal to 0.
	     *	    wParam - x value of the frame's upper left
	     *	    lParam - y value of the frame's upper left
	     *
	     * Otherwise an embedded window requests the current position
	     *
	     * Return value: an encoded window position in a 32bit long, i.e,
	     * ((x << 16) & 0xffff0000) | (y & 0xffff)
	     *
	     * Only a toplevel container may move the embedded.
	     */

	    result = TkpWinToplevelMove(containerPtr->parentPtr,
		    wParam, lParam);
	    break;

	case TK_OVERRIDEREDIRECT:
	    /*
	     * An embedded window request overrideredirect.
	     *
	     * wParam
	     *	0	- add a frame if there is no one
	     *  1	- remove the frame if there is a one
	     *  < 0	- query the current overrideredirect value
	     *
	     * lParam	- N/A
	     *
	     * Return value:
	     * 1 + the current value of overrideredirect if the container is a
	     * toplevel. Otherwise 0.
	     */
	    if (topwinPtr) {
		result = 1 + TkpWinToplevelOverrideRedirect(topwinPtr, wParam);
	    } else {
		result = 0;
	    }
	    break;

	case TK_SETMENU:
	    /*
	     * An embedded requests to set a menu.
	     *
	     * wParam	- a menu handle
	     * lParam	- a menu window handle
	     *
	     * Return value:
	     * 1    - the message is processed
	     * 0    - the message is not processed
	     */
	    if (topwinPtr) {
		containerPtr->embeddedMenuHWnd = (HWND)lParam;
		TkWinSetMenu((Tk_Window)topwinPtr, (HMENU)wParam);
	    } else {
		result = 0;
	    }
	    break;

	case TK_STATE:
	    /*
	     * An embedded window request set/get state services.
	     *
	     * wParam	- service directive
	     *	    0 - 3 for setting state
	     *		0 - withdrawn state
	     *		1 - normal state
	     *		2 - zoom state
	     *		3 - icon state
	     * others for gettting state
	     *
	     * lParam	- N/A
	     *
	     * Return value
	     * 1 + the current state or 0 if the container is not a toplevel
	     */

	    if (topwinPtr) {
		if (wParam <= 3) {
		    TkpWmSetState(topwinPtr, wParam);
		}
		result = 1+TkpWmGetState(topwinPtr);
	    } else {
		result = 0;
	    }
	    break;

	    /*
	     * Return 0 since the current Tk container implementation is
	     * unable to provide following services.
	     */
	default:
	    result = 0;
	    break;
	}
    } else {
	if ((message == TK_INFO) && (wParam == TK_CONTAINER_VERIFY)) {
	    /*
	     * Reply the message sender: this is not a Tk container
	     */

	    return -PTR2INT(hwnd);
	} else {
	    result = 0;
	}
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * EmbedGeometryRequest --
 *
 *	This procedure is invoked when an embedded application requests a
 *	particular size. It processes the request (which may or may not
 *	actually resize the window) and reflects the results back to the
 *	embedded application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If we deny the child's size change request, a Configure event is
 *	synthesized to let the child know that the size is the same as it used
 *	to be. Events get processed while we're waiting for the geometry
 *	managers to do their thing.
 *
 *----------------------------------------------------------------------
 */

void
EmbedGeometryRequest(
    Container *containerPtr,	/* Information about the container window. */
    int width, int height)	/* Size that the child has requested. */
{
    TkWindow *winPtr = containerPtr->parentPtr;

    /*
     * Forward the requested size into our geometry management hierarchy via
     * the container window. We need to send a Configure event back to the
     * embedded application even if we decide not to resize the window; to
     * make this happen, process all idle event handlers synchronously here
     * (so that the geometry managers have had a chance to do whatever they
     * want to do), and if the window's size didn't change then generate a
     * configure event.
     */

    Tk_GeometryRequest((Tk_Window)winPtr, width, height);

    if (containerPtr->embeddedHWnd != NULL) {
	while (Tcl_DoOneEvent(TCL_IDLE_EVENTS)) {
	    /* Empty loop body. */
	}

	SetWindowPos(containerPtr->embeddedHWnd, NULL, 0, 0,
		winPtr->changes.width, winPtr->changes.height, SWP_NOZORDER);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ContainerEventProc --
 *
 *	This procedure is invoked by the Tk event dispatcher when various
 *	useful events are received for the container window.
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
    Container *containerPtr = (Container *)clientData;
    Tk_Window tkwin = (Tk_Window)containerPtr->parentPtr;

    if (eventPtr->type == ConfigureNotify) {

	/*
         * Send a ConfigureNotify  to the embedded application.
         */

        if (containerPtr->embeddedPtr != NULL) {
            TkDoConfigureNotify(containerPtr->embeddedPtr);
        }

	/*
	 * Resize the embedded window, if there is any.
	 */

	if (containerPtr->embeddedHWnd) {
	    SetWindowPos(containerPtr->embeddedHWnd, NULL, 0, 0,
		    Tk_Width(tkwin), Tk_Height(tkwin), SWP_NOZORDER);
	}
    } else if (eventPtr->type == DestroyNotify) {
	/*
	 * The container is gone, remove it from the list.
	 */

	EmbedWindowDeleted(containerPtr->parentPtr);
    }
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
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (containerPtr = tsdPtr->firstContainerPtr; containerPtr != NULL;
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
 * Tk_GetEmbeddedHWnd --
 *
 *	This function returns the embedded window id.
 *
 * Results:
 *	If winPtr is a container, the return value is the HWND for the
 *	embedded window. Otherwise it returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HWND
Tk_GetEmbeddedHWnd(
    TkWindow *winPtr)
{
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (containerPtr = tsdPtr->firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->parentPtr == winPtr) {
	    return containerPtr->embeddedHWnd;
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetEmbeddedMenuHWND --
 *
 *	This function returns the embedded menu window id.
 *
 * Results:
 *	If winPtr is a container, the return value is the HWND for the
 *	embedded menu window. Otherwise it returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HWND
Tk_GetEmbeddedMenuHWND(
    Tk_Window tkwin)
{
    TkWindow *winPtr = (TkWindow*)tkwin;
    Container *containerPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (containerPtr = tsdPtr->firstContainerPtr; containerPtr != NULL;
	    containerPtr = containerPtr->nextPtr) {
	if (containerPtr->parentPtr == winPtr) {
	    return containerPtr->embeddedMenuHWnd;
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpClaimFocus --
 *
 *	This procedure is invoked when someone asks or the input focus to be
 *	put on a window in an embedded application, but the application
 *	doesn't currently have the focus. It requests the input focus from the
 *	container application.
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
    HWND hwnd = GetParent(Tk_GetHWND(topLevelPtr->window));
    SendMessageW(hwnd, TK_CLAIMFOCUS, (WPARAM) force, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpRedirectKeyEvent --
 *
 *	This procedure is invoked when a key press or release event arrives
 *	for an application that does not believe it owns the input focus.
 *	This can happen because of embedding; for example, X can send an event
 *	to an embedded application when the real focus window is in the
 *	container application and is an ancestor of the container. This
 *	procedure's job is to forward the event back to the application where
 *	it really belongs.
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
    /* not implemented */
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
    TkWindow *winPtr)		/* Tk's information about window that was
				 * deleted. */
{
    Container *containerPtr, *prevPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Find the Container structure for this window work. Delete the
     * information about the embedded application and free the container's
     * record. The main container may be null. [Bug #476176]
     */

    prevPtr = NULL;
    containerPtr = tsdPtr->firstContainerPtr;
    if (containerPtr == NULL) return;
    while (1) {
	if (containerPtr->embeddedPtr == winPtr) {
	    containerPtr->embeddedHWnd = NULL;
	    containerPtr->embeddedPtr = NULL;
	    break;
	}
	if (containerPtr->parentPtr == winPtr) {
	    SendMessageW(containerPtr->embeddedHWnd, WM_CLOSE, 0, 0);
	    containerPtr->parentPtr = NULL;
	    containerPtr->embeddedPtr = NULL;
	    break;
	}
	prevPtr = containerPtr;
	containerPtr = containerPtr->nextPtr;
	if (containerPtr == NULL) {
	    return;
	}
    }
    if ((containerPtr->embeddedPtr == NULL)
	    && (containerPtr->parentPtr == NULL)) {
	if (prevPtr == NULL) {
	    tsdPtr->firstContainerPtr = containerPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = containerPtr->nextPtr;
	}
	ckfree(containerPtr);
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
