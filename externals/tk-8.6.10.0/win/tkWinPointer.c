/*
 * tkWinPointer.c --
 *
 *	Windows specific mouse tracking code.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"

/*
 * Check for enter/leave events every MOUSE_TIMER_INTERVAL milliseconds.
 */

#define MOUSE_TIMER_INTERVAL 250

/*
 * Declarations of static variables used in this file.
 */

static int captured = 0;		/* 1 if mouse is currently captured. */
static TkWindow *keyboardWinPtr = NULL; /* Current keyboard grab window. */
static Tcl_TimerToken mouseTimer;	/* Handle to the latest mouse timer. */
static int mouseTimerSet = 0;		/* 1 if the mouse timer is active. */

/*
 * Forward declarations of procedures used in this file.
 */

static void		MouseTimerProc(ClientData clientData);

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetModifierState --
 *
 *	Return the modifier state as of the last message.
 *
 * Results:
 *	Returns the X modifier mask.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkWinGetModifierState(void)
{
    int state = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000) {
	state |= ShiftMask;
    }
    if (GetKeyState(VK_CONTROL) & 0x8000) {
	state |= ControlMask;
    }
    if (GetKeyState(VK_MENU) & 0x8000) {
	state |= ALT_MASK;
    }
    if (GetKeyState(VK_CAPITAL) & 0x0001) {
	state |= LockMask;
    }
    if (GetKeyState(VK_NUMLOCK) & 0x0001) {
	state |= Mod1Mask;
    }
    if (GetKeyState(VK_SCROLL) & 0x0001) {
	state |= Mod3Mask;
    }
    if (GetKeyState(VK_LBUTTON) & 0x8000) {
	state |= Button1Mask;
    }
    if (GetKeyState(VK_MBUTTON) & 0x8000) {
	state |= Button2Mask;
    }
    if (GetKeyState(VK_RBUTTON) & 0x8000) {
	state |= Button3Mask;
    }
    if (GetKeyState(VK_XBUTTON1) & 0x8000) {
	state |= Button4Mask;
    }
    if (GetKeyState(VK_XBUTTON2) & 0x8000) {
	state |= Button5Mask;
    }
    return state;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PointerEvent --
 *
 *	This procedure is called for each pointer-related event. It converts
 *	the position to root coords and updates the global pointer state
 *	machine. It also ensures that the mouse timer is scheduled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May queue events and change the grab state.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PointerEvent(
    HWND hwnd,			/* Window for coords, or NULL for the root
				 * window. */
    int x, int y)		/* Coords relative to hwnd, or screen if hwnd
				 * is NULL. */
{
    POINT pos;
    int state;
    Tk_Window tkwin;

    pos.x = x;
    pos.y = y;

    /*
     * Convert client coords to root coords if we were given a window.
     */

    if (hwnd) {
	ClientToScreen(hwnd, &pos);
    }

    /*
     * If the mouse is captured, Windows will report all pointer events to the
     * capture window. So, we need to determine which window the mouse is
     * really over and change the event. Note that the computed hwnd may point
     * to a window not owned by Tk, or a toplevel decorative frame, so tkwin
     * can be NULL.
     */

    if (captured || hwnd == NULL) {
	hwnd = WindowFromPoint(pos);
    }
    tkwin = Tk_HWNDToWindow(hwnd);

    state = TkWinGetModifierState();

    Tk_UpdatePointer(tkwin, pos.x, pos.y, state);

    if ((captured || tkwin) && !mouseTimerSet) {
	mouseTimerSet = 1;
	mouseTimer = Tcl_CreateTimerHandler(MOUSE_TIMER_INTERVAL,
		MouseTimerProc, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XGrabKeyboard --
 *
 *	Simulates a keyboard grab by setting the focus.
 *
 * Results:
 *	Always returns GrabSuccess.
 *
 * Side effects:
 *	Sets the keyboard focus to the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XGrabKeyboard(
    Display *display,
    Window grab_window,
    Bool owner_events,
    int pointer_mode,
    int keyboard_mode,
    Time time)
{
    keyboardWinPtr = TkWinGetWinPtr(grab_window);
    return GrabSuccess;
}

/*
 *----------------------------------------------------------------------
 *
 * XUngrabKeyboard --
 *
 *	Releases the simulated keyboard grab.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the keyboard focus back to the value before the grab.
 *
 *----------------------------------------------------------------------
 */

int
XUngrabKeyboard(
    Display *display,
    Time time)
{
    keyboardWinPtr = NULL;
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * MouseTimerProc --
 *
 *	Check the current mouse position and look for enter/leave events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May schedule a new timer and/or generate enter/leave events.
 *
 *----------------------------------------------------------------------
 */

void
MouseTimerProc(
    ClientData clientData)
{
    POINT pos;

    mouseTimerSet = 0;

    /*
     * Get the current mouse position and window. Don't do anything if the
     * mouse hasn't moved since the last time we looked.
     */

    GetCursorPos(&pos);
    Tk_PointerEvent(NULL, pos.x, pos.y);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinCancelMouseTimer --
 *
 *    If the mouse timer is set, cancel it.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    May cancel the mouse timer.
 *
 *----------------------------------------------------------------------
 */

void
TkWinCancelMouseTimer(void)
{
    if (mouseTimerSet) {
	Tcl_DeleteTimerHandler(mouseTimer);
	mouseTimerSet = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetPointerCoords --
 *
 *	Fetch the position of the mouse pointer.
 *
 * Results:
 *	*xPtr and *yPtr are filled in with the root coordinates of the mouse
 *	pointer for the display.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkGetPointerCoords(
    Tk_Window tkwin,		/* Window that identifies screen on which
				 * lookup is to be done. */
    int *xPtr, int *yPtr)	/* Store pointer coordinates here. */
{
    POINT point;

    GetCursorPos(&point);
    *xPtr = point.x;
    *yPtr = point.y;
}

/*
 *----------------------------------------------------------------------
 *
 * XQueryPointer --
 *
 *	Check the current state of the mouse. This is not a complete
 *	implementation of this function. It only computes the root coordinates
 *	and the current mask.
 *
 * Results:
 *	Sets root_x_return, root_y_return, and mask_return. Returns true on
 *	success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Bool
XQueryPointer(
    Display *display,
    Window w,
    Window *root_return,
    Window *child_return,
    int *root_x_return,
    int *root_y_return,
    int *win_x_return,
    int *win_y_return,
    unsigned int *mask_return)
{
    display->request++;
    TkGetPointerCoords(NULL, root_x_return, root_y_return);
    *mask_return = TkWinGetModifierState();
    return True;
}

/*
 *----------------------------------------------------------------------
 *
 * XWarpPointer, TkpWarpPointer --
 *
 *	Move pointer to new location. Note that implementation of XWarpPointer
 *	is incomplete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Mouse pointer changes position on screen.
 *
 *----------------------------------------------------------------------
 */

/*
 * TkSetCursorPos is a helper function replacing SetCursorPos since this
 * latter Windows function appears to have been broken by Microsoft
 * since Win10 Falls Creator Update - See ticket [69b48f427e] along with
 * several other Internet reports about this breakage.
 */

void TkSetCursorPos(
    int x,
    int y)
{
    INPUT input;
    int xscreen = (int)(GetSystemMetrics(SM_CXSCREEN) - 1);
    int yscreen = (int)(GetSystemMetrics(SM_CYSCREEN) - 1);

    input.type = INPUT_MOUSE;
    input.mi.dx = (x * 65535 + xscreen/2) / xscreen;
    input.mi.dy = (y * 65535 + yscreen/2) / yscreen;
    input.mi.mouseData = 0;
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(input));
}

int
XWarpPointer(
    Display *display,
    Window src_w,
    Window dest_w,
    int src_x,
    int src_y,
    unsigned int src_width,
    unsigned int src_height,
    int dest_x,
    int dest_y)
{
    RECT r;

    GetWindowRect(Tk_GetHWND(dest_w), &r);
    TkSetCursorPos(r.left+dest_x, r.top+dest_y);
    return Success;
}

void
TkpWarpPointer(
    TkDisplay *dispPtr)
{
    if (dispPtr->warpWindow) {
	RECT r;

	GetWindowRect(Tk_GetHWND(Tk_WindowId(dispPtr->warpWindow)), &r);
	TkSetCursorPos(r.left + dispPtr->warpX, r.top + dispPtr->warpY);
    } else {
	TkSetCursorPos(dispPtr->warpX, dispPtr->warpY);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XGetInputFocus --
 *
 *	Retrieves the current keyboard focus window.
 *
 * Results:
 *	Returns the current focus window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
XGetInputFocus(
    Display *display,
    Window *focus_return,
    int *revert_to_return)
{
    Tk_Window tkwin = Tk_HWNDToWindow(GetFocus());

    *focus_return = tkwin ? Tk_WindowId(tkwin) : None;
    *revert_to_return = RevertToParent;
    display->request++;
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XSetInputFocus --
 *
 *	Set the current focus window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the keyboard focus and causes the selected window to
 *	be activated.
 *
 *----------------------------------------------------------------------
 */

int
XSetInputFocus(
    Display *display,
    Window focus,
    int revert_to,
    Time time)
{
    display->request++;
    if (focus != None) {
	SetFocus(Tk_GetHWND(focus));
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpChangeFocus --
 *
 *	This procedure is invoked to move the system focus from one window to
 *	another.
 *
 * Results:
 *	The return value is the serial number of the command that changed the
 *	focus. It may be needed by the caller to filter out focus change
 *	events that were queued before the command. If the procedure doesn't
 *	actually change the focus then it returns 0.
 *
 * Side effects:
 *	The official Windows focus window changes; the application's focus
 *	window isn't changed by this procedure.
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
    Window focusWindow;
    int dummy, serial;
    TkWindow *winPtr2;

    if (!force) {
	XGetInputFocus(dispPtr->display, &focusWindow, &dummy);
	winPtr2 = (TkWindow *) Tk_IdToWindow(dispPtr->display, focusWindow);
	if ((winPtr2 == NULL) || (winPtr2->mainPtr != winPtr->mainPtr)) {
	    return 0;
	}
    }

    if (winPtr->window == None) {
	Tcl_Panic("ChangeXFocus got null X window");
    }

    /*
     * Change the foreground window so the focus window is raised to the top
     * of the system stacking order and gets the keyboard focus.
     */

    if (force) {
	TkWinSetForegroundWindow(winPtr);
    }
    XSetInputFocus(dispPtr->display, winPtr->window, RevertToParent,
	    CurrentTime);

    /*
     * Remember the current serial number for the X server and issue a dummy
     * server request. This marks the position at which we changed the focus,
     * so we can distinguish FocusIn and FocusOut events on either side of the
     * mark.
     */

    serial = NextRequest(winPtr->display);
    XNoOp(winPtr->display);
    return serial;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetCapture --
 *
 *	This function captures the mouse so that all future events will be
 *	reported to this window, even if the mouse is outside the window. If
 *	the specified window is NULL, then the mouse is released.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the capture flag and captures the mouse.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetCapture(
    TkWindow *winPtr)		/* Capture window, or NULL. */
{
    if (winPtr) {
	SetCapture(Tk_GetHWND(Tk_WindowId(winPtr)));
	captured = 1;
    } else {
	captured = 0;
	ReleaseCapture();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetCapture --
 *
 *	This function requests which window is capturing the mouse.
 *
 * Results:
 *	The return value is a pointer to the capture window, if there is
 *      one, otherwise it is NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
TkpGetCapture(void)
{
    return Tk_HWNDToWindow(GetCapture());
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
