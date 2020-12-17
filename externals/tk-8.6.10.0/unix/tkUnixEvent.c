/*
 * tkUnixEvent.c --
 *
 *	This file implements an event source for X displays for the UNIX
 *	version of Tk.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"
#include <signal.h>
#ifdef HAVE_XKBKEYCODETOKEYSYM
#  include <X11/XKBlib.h>
#else
#  define XkbOpenDisplay(D,V,E,M,m,R) ((V),(E),(M),(m),(R),(NULL))
#endif

/*
 * The following static indicates whether this module has been initialized in
 * the current thread.
 */

typedef struct {
    int initialized;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Prototypes for functions that are referenced only in this file:
 */

static void		DisplayCheckProc(ClientData clientData, int flags);
static void		DisplayExitHandler(ClientData clientData);
static void		DisplayFileProc(ClientData clientData, int flags);
static void		DisplaySetupProc(ClientData clientData, int flags);
static void		TransferXEventsToTcl(Display *display);
#ifdef TK_USE_INPUT_METHODS
static void		InstantiateIMCallback(Display *, XPointer client_data, XPointer call_data);
static void		DestroyIMCallback(XIM im, XPointer client_data, XPointer call_data);
static void		OpenIM(TkDisplay *dispPtr);
#endif

/*
 *----------------------------------------------------------------------
 *
 * TkCreateXEventSource --
 *
 *	This function is called during Tk initialization to create the event
 *	source for X Window events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new event source is created.
 *
 *----------------------------------------------------------------------
 */

void
TkCreateXEventSource(void)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	Tcl_CreateEventSource(DisplaySetupProc, DisplayCheckProc, NULL);
	TkCreateExitHandler(DisplayExitHandler, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayExitHandler --
 *
 *	This function is called during finalization to clean up the display
 *	module.
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
DisplayExitHandler(
    ClientData clientData)	/* Not used. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    Tcl_DeleteEventSource(DisplaySetupProc, DisplayCheckProc, NULL);
    tsdPtr->initialized = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpOpenDisplay --
 *
 *	Allocates a new TkDisplay, opens the X display, and establishes the
 *	file handler for the connection.
 *
 * Results:
 *	A pointer to a Tk display structure.
 *
 * Side effects:
 *	Opens a display.
 *
 *----------------------------------------------------------------------
 */

TkDisplay *
TkpOpenDisplay(
    const char *displayNameStr)
{
    TkDisplay *dispPtr;
    Display *display;
    int event = 0;
    int error = 0;
    int major = 1;
    int minor = 0;
    int reason = 0;
    unsigned int use_xkb = 0;
    /* Disabled, until we have a better test. See [Bug 3613668] */
#if 0 && defined(XKEYCODETOKEYSYM_IS_DEPRECATED) && defined(TCL_THREADS)
    static int xinited = 0;
    static Tcl_Mutex xinitMutex = NULL;

    if (!xinited) {
	Tcl_MutexLock(&xinitMutex);
	if (!xinited) {
	    /* Necessary for threaded apps, of no consequence otherwise  */
	    /* need only be called once, but must be called before *any* */
	    /* Xlib call is made. If xinitMutex is still NULL after the  */
	    /* Tcl_MutexLock call, Tcl was compiled without threads so   */
	    /* we cannot use XInitThreads() either.                      */
	    if (xinitMutex != NULL){
		XInitThreads();
	    }
	    xinited = 1;
	}
	Tcl_MutexUnlock(&xinitMutex);
    }
#endif

    /*
    ** Bug [3607830]: Before using Xkb, it must be initialized and confirmed
    **                that the serve supports it.  The XkbOpenDisplay call
    **                will perform this check and return NULL if the extension
    **                is not supported.
    **
    ** Work around un-const-ified Xkb headers using (char *) cast.
    */
    display = XkbOpenDisplay((char *)displayNameStr, &event, &error, &major,
	    &minor, &reason);

    if (display == NULL) {
	/*fprintf(stderr,"event=%d error=%d major=%d minor=%d reason=%d\nDisabling xkb\n",
	event, error, major, minor, reason);*/
	display = XOpenDisplay(displayNameStr);
    } else {
	use_xkb = TK_DISPLAY_USE_XKB;
	/*fprintf(stderr, "Using xkb %d.%d\n", major, minor);*/
    }

    if (display == NULL) {
	return NULL;
    }
    dispPtr = ckalloc(sizeof(TkDisplay));
    memset(dispPtr, 0, sizeof(TkDisplay));
    dispPtr->display = display;
    dispPtr->flags |= use_xkb;
#ifdef TK_USE_INPUT_METHODS
    OpenIM(dispPtr);
    XRegisterIMInstantiateCallback(dispPtr->display, NULL, NULL, NULL,
	    InstantiateIMCallback, (XPointer) dispPtr);
#endif
    Tcl_CreateFileHandler(ConnectionNumber(display), TCL_READABLE,
	    DisplayFileProc, dispPtr);

    /*
     * Observed weird WidthMMOfScreen() in X on Wayland on a
     * Fedora 30/i386 running in a VM. Fallback to 75 dpi,
     * otherwise many other strange things may happen later.
     * See: [https://core.tcl-lang.org/tk/tktview?name=a01b6f7227]
     */
    if (WidthMMOfScreen(DefaultScreenOfDisplay(display)) <= 0) {
	int mm;

	mm = WidthOfScreen(DefaultScreenOfDisplay(display)) * (25.4 / 75.0);
	WidthMMOfScreen(DefaultScreenOfDisplay(display)) = mm;
    }
    if (HeightMMOfScreen(DefaultScreenOfDisplay(display)) <= 0) {
	int mm;

	mm = HeightOfScreen(DefaultScreenOfDisplay(display)) * (25.4 / 75.0);
	HeightMMOfScreen(DefaultScreenOfDisplay(display)) = mm;
    }

    /*
     * Key map info must be available immediately, because of "send event".
     */
    TkpInitKeymapInfo(dispPtr);

    return dispPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCloseDisplay --
 *
 *	Cancels notifier callbacks and closes a display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates the displayPtr and unix-specific resources.
 *
 *----------------------------------------------------------------------
 */

void
TkpCloseDisplay(
    TkDisplay *dispPtr)
{
    TkSendCleanup(dispPtr);

    TkWmCleanup(dispPtr);

#ifdef TK_USE_INPUT_METHODS
    if (dispPtr->inputXfs) {
	XFreeFontSet(dispPtr->display, dispPtr->inputXfs);
    }
    if (dispPtr->inputMethod) {
	XCloseIM(dispPtr->inputMethod);
    }
#endif

    if (dispPtr->display != 0) {
	Tcl_DeleteFileHandler(ConnectionNumber(dispPtr->display));
	(void) XSync(dispPtr->display, False);
	(void) XCloseDisplay(dispPtr->display);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkClipCleanup --
 *
 *	This function is called to cleanup resources associated with claiming
 *	clipboard ownership and for receiving selection get results. This
 *	function is called in tkWindow.c. This has to be called by the display
 *	cleanup function because we still need the access display elements.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources are freed - the clipboard may no longer be used.
 *
 *----------------------------------------------------------------------
 */

void
TkClipCleanup(
    TkDisplay *dispPtr)		/* Display associated with clipboard */
{
    if (dispPtr->clipWindow != NULL) {
	Tk_DeleteSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
		dispPtr->applicationAtom);
	Tk_DeleteSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
		dispPtr->windowAtom);

	Tk_DestroyWindow(dispPtr->clipWindow);
	Tcl_Release(dispPtr->clipWindow);
	dispPtr->clipWindow = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplaySetupProc --
 *
 *	This function implements the setup part of the UNIX X display event
 *	source. It is invoked by Tcl_DoOneEvent before entering the notifier
 *	to check for events on all displays.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If data is queued on a display inside Xlib, then the maximum block
 *	time will be set to 0 to ensure that the notifier returns control to
 *	Tcl even if there is no more data on the X connection.
 *
 *----------------------------------------------------------------------
 */

static void
DisplaySetupProc(
    ClientData clientData,	/* Not used. */
    int flags)
{
    TkDisplay *dispPtr;
    static Tcl_Time blockTime = { 0, 0 };

    if (!(flags & TCL_WINDOW_EVENTS)) {
	return;
    }

    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	    dispPtr = dispPtr->nextPtr) {
	/*
	 * Flush the display. If data is pending on the X queue, set the block
	 * time to zero. This ensures that we won't block in the notifier if
	 * there is data in the X queue, but not on the server socket.
	 */

	XFlush(dispPtr->display);
	if (QLength(dispPtr->display) > 0) {
	    Tcl_SetMaxBlockTime(&blockTime);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TransferXEventsToTcl --
 *
 *	Transfer events from the X event queue to the Tk event queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves queued X events onto the Tcl event queue.
 *
 *----------------------------------------------------------------------
 */

static void
TransferXEventsToTcl(
    Display *display)
{
    union {
	int type;
	XEvent x;
	TkKeyEvent k;
#ifdef GenericEvent
	xGenericEvent xge;
#endif
    } event;
    Window w;
    TkDisplay *dispPtr = NULL;

    /*
     * Transfer events from the X event queue to the Tk event queue after XIM
     * event filtering. KeyPress and KeyRelease events need special treatment
     * so that they get directed according to Tk's focus rules during XIM
     * handling. Theoretically they can go to the wrong place still (if
     * there's a focus change in the queue) but if we push the handling off
     * until Tk_HandleEvent then many input methods actually cease to work
     * correctly. Most of the time, Tk processes its event queue fast enough
     * for this to not be an issue anyway. [Bug 1924761]
     */

    while (QLength(display) > 0) {
	XNextEvent(display, &event.x);
#ifdef GenericEvent
	if (event.type == GenericEvent) {
	    Tcl_Panic("Wild GenericEvent; panic! (extension=%d,evtype=%d)",
		    event.xge.extension, event.xge.evtype);
	}
#endif
	w = None;
	if (event.type == KeyPress || event.type == KeyRelease) {
	    for (dispPtr = TkGetDisplayList(); ; dispPtr = dispPtr->nextPtr) {
		if (dispPtr == NULL) {
		    break;
		} else if (dispPtr->display == event.x.xany.display) {
		    if (dispPtr->focusPtr != NULL) {
			w = dispPtr->focusPtr->window;
		    }
		    break;
		}
	    }
	}
	if (XFilterEvent(&event.x, w)) {
	    continue;
	}
	if (event.type == KeyPress || event.type == KeyRelease) {
	    event.k.charValuePtr = NULL;
	    event.k.charValueLen = 0;
	    event.k.keysym = NoSymbol;

	    /*
	     * Force the calling of the input method engine now. The results
	     * from it will be cached in the event so that they don't get lost
	     * (to a race condition with other XIM-handled key events) between
	     * entering the event queue and getting serviced. [Bug 1924761]
	     */

#ifdef TK_USE_INPUT_METHODS
	    if (event.type == KeyPress && dispPtr &&
		    (dispPtr->flags & TK_DISPLAY_USE_IM)) {
		if (dispPtr->focusPtr && dispPtr->focusPtr->inputContext) {
		    Tcl_DString ds;

		    Tcl_DStringInit(&ds);
		    (void) TkpGetString(dispPtr->focusPtr, &event.x, &ds);
		    Tcl_DStringFree(&ds);
		}
	    }
#endif
	}
	Tk_QueueWindowEvent(&event.x, TCL_QUEUE_TAIL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayCheckProc --
 *
 *	This function checks for events sitting in the X event queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves queued events onto the Tcl event queue.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayCheckProc(
    ClientData clientData,	/* Not used. */
    int flags)
{
    TkDisplay *dispPtr;

    if (!(flags & TCL_WINDOW_EVENTS)) {
	return;
    }

    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	    dispPtr = dispPtr->nextPtr) {
	XFlush(dispPtr->display);
	TransferXEventsToTcl(dispPtr->display);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayFileProc --
 *
 *	This function implements the file handler for the X connection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Makes entries on the Tcl event queue for all the events available from
 *	all the displays.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayFileProc(
    ClientData clientData,	/* The display pointer. */
    int flags)			/* Should be TCL_READABLE. */
{
    TkDisplay *dispPtr = (TkDisplay *) clientData;
    Display *display = dispPtr->display;
    int numFound;

    XFlush(display);
    numFound = XEventsQueued(display, QueuedAfterReading);
    if (numFound == 0) {
	/*
	 * Things are very tricky if there aren't any events readable at this
	 * point (after all, there was supposedly data available on the
	 * connection). A couple of things could have occurred:
	 *
	 * One possibility is that there were only error events in the input
	 * from the server. If this happens, we should return (we don't want
	 * to go to sleep in XNextEvent below, since this would block out
	 * other sources of input to the process).
	 *
	 * Another possibility is that our connection to the server has been
	 * closed. This will not necessarily be detected in XEventsQueued (!!)
	 * so if we just return then there will be an infinite loop. To detect
	 * such an error, generate a NoOp protocol request to exercise the
	 * connection to the server, then return. However, must disable
	 * SIGPIPE while sending the request, or else the process will die
	 * from the signal and won't invoke the X error function to print a
	 * nice (?!) message.
	 */

	void (*oldHandler)();

	oldHandler = (void (*)()) signal(SIGPIPE, SIG_IGN);
	XNoOp(display);
	XFlush(display);
	(void) signal(SIGPIPE, oldHandler);
    }

    TransferXEventsToTcl(display);
}

/*
 *----------------------------------------------------------------------
 *
 * TkUnixDoOneXEvent --
 *
 *	This routine waits for an X event to be processed or for a timeout to
 *	occur. The timeout is specified as an absolute time. This routine is
 *	called when Tk needs to wait for a particular X event without letting
 *	arbitrary events be processed. The caller will typically call
 *	Tk_RestrictEvents to set up an event filter before calling this
 *	routine. This routine will service at most one event per invocation.
 *
 * Results:
 *	Returns 0 if the timeout has expired, otherwise returns 1.
 *
 * Side effects:
 *	Can invoke arbitrary Tcl scripts.
 *
 *----------------------------------------------------------------------
 */

int
TkUnixDoOneXEvent(
    Tcl_Time *timePtr)		/* Specifies the absolute time when the call
				 * should time out. */
{
    TkDisplay *dispPtr;
    static fd_mask readMask[MASK_SIZE];
    struct timeval blockTime, *timeoutPtr;
    Tcl_Time now;
    int fd, index, numFound, numFdBits = 0;
    fd_mask bit, *readMaskPtr = readMask;

    /*
     * Look for queued events first.
     */

    if (Tcl_ServiceEvent(TCL_WINDOW_EVENTS)) {
	return 1;
    }

    /*
     * Compute the next block time and check to see if we have timed out. Note
     * that HP-UX defines tv_sec to be unsigned so we have to be careful in
     * our arithmetic.
     */

    if (timePtr) {
	Tcl_GetTime(&now);
	blockTime.tv_sec = timePtr->sec;
	blockTime.tv_usec = timePtr->usec - now.usec;
	if (blockTime.tv_usec < 0) {
	    now.sec += 1;
	    blockTime.tv_usec += 1000000;
	}
	if (blockTime.tv_sec < now.sec) {
	    blockTime.tv_sec = 0;
	    blockTime.tv_usec = 0;
	} else {
	    blockTime.tv_sec -= now.sec;
	}
	timeoutPtr = &blockTime;
    } else {
	timeoutPtr = NULL;
    }

    /*
     * Set up the select mask for all of the displays. If a display has data
     * pending, then we want to poll instead of blocking.
     */

    memset(readMask, 0, MASK_SIZE*sizeof(fd_mask));
    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	    dispPtr = dispPtr->nextPtr) {
	XFlush(dispPtr->display);
	if (QLength(dispPtr->display) > 0) {
	    blockTime.tv_sec = 0;
	    blockTime.tv_usec = 0;
	}
	fd = ConnectionNumber(dispPtr->display);
	index = fd/(NBBY*sizeof(fd_mask));
	bit = ((fd_mask)1) << (fd%(NBBY*sizeof(fd_mask)));
	readMask[index] |= bit;
	if (numFdBits <= fd) {
	    numFdBits = fd+1;
	}
    }

    numFound = select(numFdBits, (SELECT_MASK *) readMaskPtr, NULL, NULL,
	    timeoutPtr);
    if (numFound <= 0) {
	/*
	 * Some systems don't clear the masks after an error, so we have to do
	 * it here.
	 */

	memset(readMask, 0, MASK_SIZE*sizeof(fd_mask));
    }

    /*
     * Process any new events on the display connections.
     */

    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	    dispPtr = dispPtr->nextPtr) {
	fd = ConnectionNumber(dispPtr->display);
	index = fd/(NBBY*sizeof(fd_mask));
	bit = ((fd_mask)1) << (fd%(NBBY*sizeof(fd_mask)));
	if ((readMask[index] & bit) || (QLength(dispPtr->display) > 0)) {
	    DisplayFileProc(dispPtr, TCL_READABLE);
	}
    }
    if (Tcl_ServiceEvent(TCL_WINDOW_EVENTS)) {
	return 1;
    }

    /*
     * Check to see if we timed out.
     */

    if (timePtr) {
	Tcl_GetTime(&now);
	if ((now.sec > timePtr->sec) || ((now.sec == timePtr->sec)
		&& (now.usec > timePtr->usec))) {
	    return 0;
	}
    }

    /*
     * We had an event but we did not generate a Tcl event from it. Behave as
     * though we dealt with it. (JYL&SS)
     */

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSync --
 *
 *	This routine ensures that all pending X requests have been seen by the
 *	server, and that any pending X events have been moved onto the Tk
 *	event queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Places new events on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

void
TkpSync(
    Display *display)		/* Display to sync. */
{
    XSync(display, False);

    /*
     * Transfer events from the X event queue to the Tk event queue.
     */

    TransferXEventsToTcl(display);
}
#ifdef TK_USE_INPUT_METHODS

static void
InstantiateIMCallback(
    Display      *display,
    XPointer     client_data,
    XPointer     call_data)
{
    TkDisplay    *dispPtr;

    dispPtr = (TkDisplay *) client_data;
    OpenIM(dispPtr);
    XUnregisterIMInstantiateCallback(dispPtr->display, NULL, NULL, NULL,
	    InstantiateIMCallback, (XPointer) dispPtr);
}

static void
DestroyIMCallback(
    XIM         im,
    XPointer    client_data,
    XPointer    call_data)
{
    TkDisplay   *dispPtr;

    dispPtr = (TkDisplay *) client_data;
    dispPtr->inputMethod = NULL;
    ++dispPtr->ximGeneration;
    XRegisterIMInstantiateCallback(dispPtr->display, NULL, NULL, NULL,
	    InstantiateIMCallback, (XPointer) dispPtr);
}

/*
 *--------------------------------------------------------------
 *
 * OpenIM --
 *
 *	Tries to open an X input method associated with the given display.
 *
 * Results:
 *	Stores the input method in dispPtr->inputMethod; if there isn't a
 *	suitable input method, then NULL is stored in dispPtr->inputMethod.
 *
 * Side effects:
 *	An input method gets opened.
 *
 *--------------------------------------------------------------
 */

static void
OpenIM(
    TkDisplay *dispPtr)		/* Tk's structure for the display. */
{
    int i;
    XIMStyles *stylePtr;
    XIMStyle bestStyle = 0;

    if (XSetLocaleModifiers("") == NULL) {
	return;
    }

    ++dispPtr->ximGeneration;
    dispPtr->inputMethod = XOpenIM(dispPtr->display, NULL, NULL, NULL);
    if (dispPtr->inputMethod == NULL) {
	return;
    }

    /* Require X11R6 */
    {
	XIMCallback destroy_cb;

	destroy_cb.callback = DestroyIMCallback;
	destroy_cb.client_data = (XPointer) dispPtr;
	if (XSetIMValues(dispPtr->inputMethod, XNDestroyCallback,
		&destroy_cb, NULL))
	    goto error;
    }

    if ((XGetIMValues(dispPtr->inputMethod, XNQueryInputStyle, &stylePtr,
	    NULL) != NULL) || (stylePtr == NULL)) {
	goto error;
    }

    /*
     * Select the best input style supported by both the IM and Tk.
     */
    for (i = 0; i < stylePtr->count_styles; i++) {
	XIMStyle thisStyle = stylePtr->supported_styles[i];
	if (thisStyle == (XIMPreeditPosition | XIMStatusNothing)) {
	    bestStyle = thisStyle;
	    break;
	} else if (thisStyle == (XIMPreeditNothing | XIMStatusNothing)) {
	    bestStyle = thisStyle;
	}
    }
    XFree(stylePtr);
    if (bestStyle == 0) {
	goto error;
    }

    dispPtr->inputStyle = bestStyle;

    /*
     * Create an XFontSet for preedit area.
     */
    if (dispPtr->inputStyle & XIMPreeditPosition) {
	char **missing_list;
	int missing_count;
	char *def_string;

	dispPtr->inputXfs = XCreateFontSet(dispPtr->display,
		"-*-*-*-R-Normal--14-130-75-75-*-*",
		&missing_list, &missing_count, &def_string);
	if (missing_count > 0) {
	    XFreeStringList(missing_list);
	}
    }

    return;

error:
    if (dispPtr->inputMethod) {
	XCloseIM(dispPtr->inputMethod);
	dispPtr->inputMethod = NULL;
	++dispPtr->ximGeneration;
    }
}
#endif /* TK_USE_INPUT_METHODS */

void
TkpWarpPointer(
    TkDisplay *dispPtr)
{
    Window w;			/* Which window to warp relative to. */

    if (dispPtr->warpWindow != NULL) {
	w = Tk_WindowId(dispPtr->warpWindow);
    } else {
	w = RootWindow(dispPtr->display,
		Tk_ScreenNumber(dispPtr->warpMainwin));
    }
    XWarpPointer(dispPtr->display, None, w, 0, 0, 0, 0,
	    (int) dispPtr->warpX, (int) dispPtr->warpY);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
