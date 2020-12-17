/*
 * tkUnixFocus.c --
 *
 *	This file contains platform specific functions that manage focus for
 *	Tk.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"


/*
 *----------------------------------------------------------------------
 *
 * TkpChangeFocus --
 *
 *	This function is invoked to move the official X focus from one window
 *	to another.
 *
 * Results:
 *	The return value is the serial number of the command that changed the
 *	focus. It may be needed by the caller to filter out focus change
 *	events that were queued before the command. If the function doesn't
 *	actually change the focus then it returns 0.
 *
 * Side effects:
 *	The official X focus window changes; the application's focus window
 *	isn't changed by this function.
 *
 *----------------------------------------------------------------------
 */

int
TkpChangeFocus(
    TkWindow *winPtr,		/* Window that is to receive the X focus. */
    int force)			/* Non-zero means claim the focus even if it
				 * didn't originally belong to topLevelPtr's
				 * application. */
{
    TkDisplay *dispPtr = winPtr->dispPtr;
    Tk_ErrorHandler errHandler;
    Window window, root, parent, *children;
    unsigned int numChildren, serial;
    TkWindow *winPtr2;
    int dummy;

    /*
     * Don't set the X focus to a window that's marked override-redirect.
     * This is a hack to avoid problems with menus under olvwm: if we move
     * the focus then the focus can get lost during keyboard traversal.
     * Fortunately, we don't really need to move the focus for menus: events
     * will still find their way to the focus window, and menus aren't
     * decorated anyway so the window manager doesn't need to hear about the
     * focus change in order to redecorate the menu.
     */

    serial = 0;
    if (winPtr->atts.override_redirect) {
	return serial;
    }

    /*
     * Check to make sure that the focus is still in one of the windows of
     * this application or one of their descendants. Furthermore, grab the
     * server to make sure that the focus doesn't change in the middle of this
     * operation.
     */

    XGrabServer(dispPtr->display);
    if (!force) {
	/*
	 * Find the focus window, then see if it or one of its ancestors is a
	 * window in our application (it's possible that the focus window is
	 * in an embedded application, which may or may not be in the same
	 * process.
	 */

	XGetInputFocus(dispPtr->display, &window, &dummy);
	while (1) {
	    winPtr2 = (TkWindow *) Tk_IdToWindow(dispPtr->display, window);
	    if ((winPtr2 != NULL) && (winPtr2->mainPtr == winPtr->mainPtr)) {
		break;
	    }
	    if ((window == PointerRoot) || (window == None)) {
		goto done;
	    }
	    XQueryTree(dispPtr->display, window, &root, &parent, &children,
		    &numChildren);
	    if (children != NULL) {
		XFree((void *) children);
	    }
	    if (parent == root) {
		goto done;
	    }
	    window = parent;
	}
    }

    /*
     * Tell X to change the focus. Ignore errors that occur when changing the
     * focus: it is still possible that the window we're focussing to could
     * have gotten unmapped, which will generate an error.
     */

    errHandler = Tk_CreateErrorHandler(dispPtr->display, -1,-1,-1, NULL,NULL);
    if (winPtr->window == None) {
	Tcl_Panic("ChangeXFocus got null X window");
    }
    XSetInputFocus(dispPtr->display, winPtr->window, RevertToParent,
	    CurrentTime);
    Tk_DeleteErrorHandler(errHandler);

    /*
     * Remember the current serial number for the X server and issue a dummy
     * server request. This marks the position at which we changed the focus,
     * so we can distinguish FocusIn and FocusOut events on either side of the
     * mark.
     */

    serial = NextRequest(winPtr->display);
    XNoOp(winPtr->display);

  done:
    XUngrabServer(dispPtr->display);

    /*
     * After ungrabbing the server, it's important to flush the output
     * immediately so that the server sees the ungrab command. Otherwise we
     * might do something else that needs to communicate with the server (such
     * as invoking a subprocess that needs to do I/O to the screen); if the
     * ungrab command is still sitting in our output buffer, we could
     * deadlock.
     */

    XFlush(dispPtr->display);
    return serial;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
