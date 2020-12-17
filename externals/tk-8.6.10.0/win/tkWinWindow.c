/*
 * tkWinWindow.c --
 *
 *	Xlib emulation routines for Windows related to creating, displaying
 *	and destroying windows.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"
#include "tkBusy.h"

typedef struct {
    int initialized;		/* 0 means table below needs initializing. */
    Tcl_HashTable windowTable;  /* The windowTable maps from HWND to Tk_Window
				 * handles. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations for functions defined in this file:
 */

static void		NotifyVisibility(XEvent *eventPtr, TkWindow *winPtr);

/*
 *----------------------------------------------------------------------
 *
 * Tk_AttachHWND --
 *
 *	This function binds an HWND and a reflection function to the specified
 *	Tk_Window.
 *
 * Results:
 *	Returns an X Window that encapsulates the HWND.
 *
 * Side effects:
 *	May allocate a new X Window. Also enters the HWND into the global
 *	window table.
 *
 *----------------------------------------------------------------------
 */

Window
Tk_AttachHWND(
    Tk_Window tkwin,
    HWND hwnd)
{
    int new;
    Tcl_HashEntry *entryPtr;
    TkWinDrawable *twdPtr = (TkWinDrawable *) Tk_WindowId(tkwin);
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	Tcl_InitHashTable(&tsdPtr->windowTable, TCL_ONE_WORD_KEYS);
	tsdPtr->initialized = 1;
    }

    /*
     * Allocate a new drawable if necessary. Otherwise, remove the previous
     * HWND from from the window table.
     */

    if (twdPtr == NULL) {
	twdPtr = ckalloc(sizeof(TkWinDrawable));
	twdPtr->type = TWD_WINDOW;
	twdPtr->window.winPtr = (TkWindow *) tkwin;
    } else if (twdPtr->window.handle != NULL) {
	entryPtr = Tcl_FindHashEntry(&tsdPtr->windowTable,
		(char *)twdPtr->window.handle);
	Tcl_DeleteHashEntry(entryPtr);
    }

    /*
     * Insert the new HWND into the window table.
     */

    twdPtr->window.handle = hwnd;
    entryPtr = Tcl_CreateHashEntry(&tsdPtr->windowTable, (char *)hwnd, &new);
    Tcl_SetHashValue(entryPtr, tkwin);

    return (Window)twdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_HWNDToWindow --
 *
 *	This function retrieves a Tk_Window from the window table given an
 *	HWND.
 *
 * Results:
 *	Returns the matching Tk_Window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_HWNDToWindow(
    HWND hwnd)
{
    Tcl_HashEntry *entryPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	Tcl_InitHashTable(&tsdPtr->windowTable, TCL_ONE_WORD_KEYS);
	tsdPtr->initialized = 1;
    }
    entryPtr = Tcl_FindHashEntry(&tsdPtr->windowTable, (char *) hwnd);
    if (entryPtr != NULL) {
	return (Tk_Window) Tcl_GetHashValue(entryPtr);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetHWND --
 *
 *	This function extracts the HWND from an X Window.
 *
 * Results:
 *	Returns the HWND associated with the Window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HWND
Tk_GetHWND(
    Window window)
{
    return ((TkWinDrawable *) window)->window.handle;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpPrintWindowId --
 *
 *	This routine stores the string representation of the platform
 *	dependent window handle for an X Window in the given buffer.
 *
 * Results:
 *	Returns the result in the specified buffer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpPrintWindowId(
    char *buf,			/* Pointer to string large enough to hold the
				 * hex representation of a pointer. */
    Window window)		/* Window to be printed into buffer. */
{
    HWND hwnd = (window) ? Tk_GetHWND(window) : 0;

    sprintf(buf, "0x%" TCL_Z_MODIFIER "x", (size_t)hwnd);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpScanWindowId --
 *
 *	Given a string which represents the platform dependent window handle,
 *	produce the X Window id for the window.
 *
 * Results:
 *	The return value is normally TCL_OK; in this case *idPtr will be set
 *	to the X Window id equivalent to string. If string is improperly
 *	formed then TCL_ERROR is returned and an error message will be left in
 *	the interp's result. If the number does not correspond to a Tk Window,
 *	then *idPtr will be set to None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpScanWindowId(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    const char *string,		/* String containing a (possibly signed)
				 * integer in a form acceptable to strtol. */
    Window *idPtr)		/* Place to store converted result. */
{
    Tk_Window tkwin;
    union {
	HWND hwnd;
	int number;
    } win;

    /*
     * We want sscanf for the 64-bit check, but if that doesn't work, then
     * Tcl_GetInt manages the error correctly.
     */

    if (
#ifdef _WIN64
	    (sscanf(string, "0x%p", &win.hwnd) != 1) &&
#endif
	    Tcl_GetInt(interp, string, &win.number) != TCL_OK) {
	return TCL_ERROR;
    }

    tkwin = Tk_HWNDToWindow(win.hwnd);
    if (tkwin) {
	*idPtr = Tk_WindowId(tkwin);
    } else {
	*idPtr = None;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeWindow --
 *
 *	Creates a Windows window object based on the current attributes of the
 *	specified TkWindow.
 *
 * Results:
 *	Returns a pointer to a new TkWinDrawable cast to a Window.
 *
 * Side effects:
 *	Creates a new window.
 *
 *----------------------------------------------------------------------
 */

Window
TkpMakeWindow(
    TkWindow *winPtr,
    Window parent)
{
    HWND parentWin;
    int style;
    HWND hwnd;

    if (parent != None) {
	parentWin = Tk_GetHWND(parent);
	style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    } else {
	parentWin = NULL;
	style = WS_POPUP | WS_CLIPCHILDREN;
    }

    /*
     * Create the window, then ensure that it is at the top of the stacking
     * order.
     */

    hwnd = CreateWindowExW(WS_EX_NOPARENTNOTIFY, TK_WIN_CHILD_CLASS_NAME, NULL,
	    (DWORD) style, Tk_X(winPtr), Tk_Y(winPtr), Tk_Width(winPtr),
	    Tk_Height(winPtr), parentWin, NULL, Tk_GetHINSTANCE(), NULL);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
	    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    return Tk_AttachHWND((Tk_Window)winPtr, hwnd);
}

/*
 *----------------------------------------------------------------------
 *
 * XDestroyWindow --
 *
 *	Destroys the given window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sends the WM_DESTROY message to the window and then destroys it the
 *	Win32 resources associated with the window.
 *
 *----------------------------------------------------------------------
 */

int
XDestroyWindow(
    Display *display,
    Window w)
{
    Tcl_HashEntry *entryPtr;
    TkWinDrawable *twdPtr = (TkWinDrawable *)w;
    TkWindow *winPtr = TkWinGetWinPtr(w);
    HWND hwnd = Tk_GetHWND(w);
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    display->request++;

    /*
     * Remove references to the window in the pointer module then release the
     * drawable.
     */

    TkPointerDeadWindow(winPtr);

    entryPtr = Tcl_FindHashEntry(&tsdPtr->windowTable, (char*)hwnd);
    if (entryPtr != NULL) {
	Tcl_DeleteHashEntry(entryPtr);
    }

    ckfree(twdPtr);

    /*
     * Don't bother destroying the window if we are going to destroy the
     * parent later.
     */

    if (hwnd != NULL && !(winPtr->flags & TK_DONT_DESTROY_WINDOW)) {
	DestroyWindow(hwnd);
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XMapWindow --
 *
 *	Cause the given window to become visible.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Causes the window state to change, and generates a MapNotify event.
 *
 *----------------------------------------------------------------------
 */

int
XMapWindow(
    Display *display,
    Window w)
{
    XEvent event;
    TkWindow *parentPtr;
    TkWindow *winPtr = TkWinGetWinPtr(w);

    display->request++;

    ShowWindow(Tk_GetHWND(w), SW_SHOWNORMAL);
    winPtr->flags |= TK_MAPPED;

    /*
     * Check to see if this window is visible now. If all of the parent
     * windows up to the first toplevel are mapped, then this window and its
     * mapped children have just become visible.
     */

    if (!(winPtr->flags & TK_TOP_HIERARCHY)) {
	for (parentPtr = winPtr->parentPtr; ;
		parentPtr = parentPtr->parentPtr) {
	    if ((parentPtr == NULL) || !(parentPtr->flags & TK_MAPPED)) {
		return Success;
	    }
	    if (parentPtr->flags & TK_TOP_HIERARCHY) {
		break;
	    }
	}
    } else {
	event.type = MapNotify;
	event.xmap.serial = display->request;
	event.xmap.send_event = False;
	event.xmap.display = display;
	event.xmap.event = winPtr->window;
	event.xmap.window = winPtr->window;
	event.xmap.override_redirect = winPtr->atts.override_redirect;
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    }

    /*
     * Generate VisibilityNotify events for this window and its mapped
     * children.
     */

    event.type = VisibilityNotify;
    event.xvisibility.serial = display->request;
    event.xvisibility.send_event = False;
    event.xvisibility.display = display;
    event.xvisibility.window = winPtr->window;
    event.xvisibility.state = VisibilityUnobscured;
    NotifyVisibility(&event, winPtr);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * NotifyVisibility --
 *
 *	This function recursively notifies the mapped children of the
 *	specified window of a change in visibility. Note that we don't
 *	properly report the visibility state, since Windows does not provide
 *	that info. The eventPtr argument must point to an event that has been
 *	completely initialized except for the window slot.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates lots of events.
 *
 *----------------------------------------------------------------------
 */

static void
NotifyVisibility(
    XEvent *eventPtr,		/* Initialized VisibilityNotify event. */
    TkWindow *winPtr)		/* Window to notify. */
{
    if (winPtr->atts.event_mask & VisibilityChangeMask) {
	eventPtr->xvisibility.window = winPtr->window;
	Tk_QueueWindowEvent(eventPtr, TCL_QUEUE_TAIL);
    }
    for (winPtr = winPtr->childList; winPtr != NULL;
	    winPtr = winPtr->nextPtr) {
	if (winPtr->flags & TK_MAPPED) {
	    NotifyVisibility(eventPtr, winPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XUnmapWindow --
 *
 *	Cause the given window to become invisible.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Causes the window state to change, and generates an UnmapNotify event.
 *
 *----------------------------------------------------------------------
 */

int
XUnmapWindow(
    Display *display,
    Window w)
{
    XEvent event;
    TkWindow *winPtr = TkWinGetWinPtr(w);

    display->request++;

    /*
     * Bug fix: Don't short circuit this routine based on TK_MAPPED because it
     * will be cleared before XUnmapWindow is called.
     */

    ShowWindow(Tk_GetHWND(w), SW_HIDE);
    winPtr->flags &= ~TK_MAPPED;

    if (winPtr->flags & TK_WIN_MANAGED) {
	event.type = UnmapNotify;
	event.xunmap.serial = display->request;
	event.xunmap.send_event = False;
	event.xunmap.display = display;
	event.xunmap.event = winPtr->window;
	event.xunmap.window = winPtr->window;
	event.xunmap.from_configure = False;
	Tk_HandleEvent(&event);
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XMoveResizeWindow --
 *
 *	Move and resize a window relative to its parent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Repositions and resizes the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XMoveResizeWindow(
    Display *display,
    Window w,
    int x, int y,		/* Position relative to parent. */
    unsigned int width, unsigned int height)
{
    display->request++;
    MoveWindow(Tk_GetHWND(w), x, y, (int) width, (int) height, TRUE);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XMoveWindow --
 *
 *	Move a window relative to its parent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Repositions the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XMoveWindow(
    Display *display,
    Window w,
    int x, int y)		/* Position relative to parent */
{
    TkWindow *winPtr = TkWinGetWinPtr(w);

    display->request++;

    MoveWindow(Tk_GetHWND(w), x, y, winPtr->changes.width,
	    winPtr->changes.height, TRUE);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XResizeWindow --
 *
 *	Resize a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resizes the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XResizeWindow(
    Display *display,
    Window w,
    unsigned int width, unsigned int height)
{
    TkWindow *winPtr = TkWinGetWinPtr(w);

    display->request++;

    MoveWindow(Tk_GetHWND(w), winPtr->changes.x, winPtr->changes.y, (int)width,
	    (int)height, TRUE);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XRaiseWindow, XLowerWindow --
 *
 *	Change the stacking order of a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the stacking order of the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XRaiseWindow(
    Display *display,
    Window w)
{
    HWND window = Tk_GetHWND(w);

    display->request++;
    SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    return Success;
}

int
XLowerWindow(
    Display *display,
    Window w)
{
    HWND window = Tk_GetHWND(w);

    display->request++;
    SetWindowPos(window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XConfigureWindow --
 *
 *	Change the size, position, stacking, or border of the specified
 *	window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the attributes of the specified window. Note that we ignore
 *	the passed in values and use the values stored in the TkWindow data
 *	structure.
 *
 *----------------------------------------------------------------------
 */

int
XConfigureWindow(
    Display *display,
    Window w,
    unsigned int valueMask,
    XWindowChanges *values)
{
    TkWindow *winPtr = TkWinGetWinPtr(w);
    HWND hwnd = Tk_GetHWND(w);

    display->request++;

    /*
     * Change the shape and/or position of the window.
     */

    if (valueMask & (CWX|CWY|CWWidth|CWHeight)) {
	MoveWindow(hwnd, winPtr->changes.x, winPtr->changes.y,
		winPtr->changes.width, winPtr->changes.height, TRUE);
    }

    /*
     * Change the stacking order of the window.
     */

    if (valueMask & CWStackMode) {
	HWND sibling;

	if ((valueMask & CWSibling) && (values->sibling != None)) {
	    sibling = Tk_GetHWND(values->sibling);
	} else {
	    sibling = NULL;
	}
	TkWinSetWindowPos(hwnd, sibling, values->stack_mode);
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XClearWindow --
 *
 *	Clears the entire window to the current background color.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Erases the current contents of the window.
 *
 *----------------------------------------------------------------------
 */

int
XClearWindow(
    Display *display,
    Window w)
{
    RECT rc;
    HBRUSH brush;
    HPALETTE oldPalette, palette;
    TkWindow *winPtr;
    HWND hwnd = Tk_GetHWND(w);
    HDC dc = GetDC(hwnd);

    palette = TkWinGetPalette(display->screens[0].cmap);
    oldPalette = SelectPalette(dc, palette, FALSE);

    display->request++;

    winPtr = TkWinGetWinPtr(w);
    brush = CreateSolidBrush(winPtr->atts.background_pixel);
    GetWindowRect(hwnd, &rc);
    rc.right = rc.right - rc.left;
    rc.bottom = rc.bottom - rc.top;
    rc.left = rc.top = 0;
    FillRect(dc, &rc, brush);

    DeleteObject(brush);
    SelectPalette(dc, oldPalette, TRUE);
    ReleaseDC(hwnd, dc);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XChangeWindowAttributes --
 *
 *	This function is called when the attributes on a window are updated.
 *	Since Tk maintains all of the window state, the only relevant value is
 *	the cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May cause the mouse position to be updated.
 *
 *----------------------------------------------------------------------
 */

int
XChangeWindowAttributes(
    Display *display,
    Window w,
    unsigned long valueMask,
    XSetWindowAttributes* attributes)
{
    if (valueMask & CWCursor) {
	XDefineCursor(display, w, attributes->cursor);
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XReparentWindow --
 *
 *	TODO: currently placeholder to satisfy Xlib stubs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TODO.
 *
 *----------------------------------------------------------------------
 */

int
XReparentWindow(
    Display *display,
    Window w,
    Window parent,
    int x,
    int y)
{
    return BadWindow;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinSetWindowPos --
 *
 *	Adjust the stacking order of a window relative to a second window (or
 *	NULL).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves the specified window in the stacking order.
 *
 *----------------------------------------------------------------------
 */

void
TkWinSetWindowPos(
    HWND hwnd,			/* Window to restack. */
    HWND siblingHwnd,		/* Sibling window. */
    int pos)			/* One of Above or Below. */
{
    HWND temp;

    /*
     * Since Windows does not support Above mode, we place the specified
     * window below the sibling and then swap them.
     */

    if (siblingHwnd) {
	if (pos == Above) {
	    SetWindowPos(hwnd, siblingHwnd, 0, 0, 0, 0,
		    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	    temp = hwnd;
	    hwnd = siblingHwnd;
	    siblingHwnd = temp;
	}
    } else {
	siblingHwnd = (pos == Above) ? HWND_TOP : HWND_BOTTOM;
    }

    SetWindowPos(hwnd, siblingHwnd, 0, 0, 0, 0,
	    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpShowBusyWindow --
 *
 *	Makes a busy window "appear".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the busy window to start intercepting events and the
 *	cursor to change to the configured "hey, I'm busy!" setting.
 *
 *----------------------------------------------------------------------
 */

void
TkpShowBusyWindow(
    TkBusy busy)
{
    Busy *busyPtr = (Busy *) busy;
    HWND hWnd;
    POINT point;
    Display *display;
    Window window;

    if (busyPtr->tkBusy != NULL) {
	Tk_MapWindow(busyPtr->tkBusy);
	window = Tk_WindowId(busyPtr->tkBusy);
	display = Tk_Display(busyPtr->tkBusy);
	hWnd = Tk_GetHWND(window);
	display->request++;
	SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    /*
     * Under Win32, cursors aren't associated with windows. Tk fakes this by
     * watching Motion events on its windows. So Tk will automatically change
     * the cursor when the pointer enters the Busy window. But Windows does
     * not immediately change the cursor; it waits for the cursor position to
     * change or a system call. We need to change the cursor before the
     * application starts processing, so set the cursor position redundantly
     * back to the current position.
     */

    GetCursorPos(&point);
    TkSetCursorPos(point.x, point.y);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpHideBusyWindow --
 *
 *	Makes a busy window "disappear".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the busy window to stop intercepting events, and the
 *	cursor to change back to its normal setting.
 *
 *----------------------------------------------------------------------
 */

void
TkpHideBusyWindow(
    TkBusy busy)
{
    Busy *busyPtr = (Busy *) busy;
    POINT point;

    if (busyPtr->tkBusy != NULL) {
	Tk_UnmapWindow(busyPtr->tkBusy);
    }

    /*
     * Under Win32, cursors aren't associated with windows. Tk fakes this by
     * watching Motion events on its windows. So Tk will automatically change
     * the cursor when the pointer enters the Busy window. But Windows does
     * not immediately change the cursor: it waits for the cursor position to
     * change or a system call. We need to change the cursor before the
     * application starts processing, so set the cursor position redundantly
     * back to the current position.
     */

    GetCursorPos(&point);
    TkSetCursorPos(point.x, point.y);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeTransparentWindowExist --
 *
 *	Construct the platform-specific resources for a transparent window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves the specified window in the stacking order.
 *
 *----------------------------------------------------------------------
 */

void
TkpMakeTransparentWindowExist(
    Tk_Window tkwin,		/* Token for window. */
    Window parent)		/* Parent window. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    HWND hParent = (HWND) parent, hWnd;
    int style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    DWORD exStyle = WS_EX_TRANSPARENT | WS_EX_TOPMOST;

    hWnd = CreateWindowExW(exStyle, TK_WIN_CHILD_CLASS_NAME, NULL, style,
	    Tk_X(tkwin), Tk_Y(tkwin), Tk_Width(tkwin), Tk_Height(tkwin),
	    hParent, NULL, Tk_GetHINSTANCE(), NULL);
    winPtr->window = Tk_AttachHWND(tkwin, hWnd);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateBusy --
 *
 *	Construct the platform-specific parts of a busy window. Note that this
 *	postpones the actual creation of the window resource until later.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up part of the busy window structure.
 *
 *----------------------------------------------------------------------
 */

void
TkpCreateBusy(
    Tk_FakeWin *winPtr,
    Tk_Window tkRef,
    Window *parentPtr,
    Tk_Window tkParent,
    TkBusy busy)
{
    Busy *busyPtr = (Busy *) busy;

    if (winPtr->flags & TK_REPARENTED) {
	/*
	 * This works around a bug in the implementation of menubars for
	 * non-Macintosh window systems (Win32 and X11). Tk doesn't reset the
	 * pointers to the parent window when the menu is reparented
	 * (winPtr->parentPtr points to the wrong window). We get around this
	 * by determining the parent via the native API calls.
	 */

	HWND hWnd = GetParent(Tk_GetHWND(Tk_WindowId(tkRef)));
	RECT rect;

	if (GetWindowRect(hWnd, &rect)) {
	    busyPtr->width = rect.right - rect.left;
	    busyPtr->height = rect.bottom - rect.top;
	}
    } else {
	*parentPtr = Tk_WindowId(tkParent);
	*parentPtr = (Window) Tk_GetHWND(*parentPtr);
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
