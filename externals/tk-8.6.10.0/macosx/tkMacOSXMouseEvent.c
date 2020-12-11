/*
 * tkMacOSXMouseEvent.c --
 *
 *	This file implements functions that decode & handle mouse events on
 *	MacOS X.
 *
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXWm.h"
#include "tkMacOSXEvent.h"
#include "tkMacOSXDebug.h"
#include "tkMacOSXConstants.h"

typedef struct {
    unsigned int state;
    long delta;
    Window window;
    Point global;
    Point local;
} MouseEventData;
static Tk_Window captureWinPtr = NULL;	/* Current capture window; may be
					 * NULL. */

static int		GenerateButtonEvent(MouseEventData *medPtr);
static unsigned int	ButtonModifiers2State(UInt32 buttonState,
			    UInt32 keyModifiers);

#pragma mark TKApplication(TKMouseEvent)

enum {
    NSWindowWillMoveEventType = 20
};

/*
 * In OS X 10.6 an NSEvent of type NSMouseMoved would always have a non-Nil
 * window attribute pointing to the active window.  As of 10.8 this behavior
 * had changed.  The new behavior was that if the mouse were ever moved outside
 * of a window, all subsequent NSMouseMoved NSEvents would have a Nil window
 * attribute.  To work around this the TKApplication remembers the last non-Nil
 * window that it received in a mouse event. If it receives an NSEvent with a
 * Nil window attribute then the saved window is used.
 */

@implementation TKApplication(TKMouseEvent)
- (NSEvent *) tkProcessMouseEvent: (NSEvent *) theEvent
{
    NSWindow *eventWindow = [theEvent window];
    NSEventType eventType = [theEvent type];
    TkWindow *winPtr = NULL, *grabWinPtr;
    Tk_Window tkwin;
    NSPoint local, global;
#if 0
    NSTrackingArea *trackingArea = nil;
    NSInteger eventNumber, clickCount, buttonNumber;
#endif
    [NSEvent stopPeriodicEvents];

#ifdef TK_MAC_DEBUG_EVENTS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, theEvent);
#endif
    switch (eventType) {
    case NSMouseEntered:
    case NSMouseExited:
    case NSCursorUpdate:
    case NSLeftMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseDown:
    case NSRightMouseUp:
    case NSOtherMouseDown:
    case NSOtherMouseUp:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged:
    case NSMouseMoved:
    case NSTabletPoint:
    case NSTabletProximity:
    case NSScrollWheel:
	break;
    default: /* Unrecognized mouse event. */
	return theEvent;
    }

    /*
     * Compute the mouse position in Tk screen coordinates (global) and in the
     * Tk coordinates of its containing Tk Window (local). If a grab is in effect,
     * the local coordinates should be relative to the grab window.
     */

    if (eventWindow) {
	local = [theEvent locationInWindow];
	global = [eventWindow tkConvertPointToScreen: local];
	tkwin = TkMacOSXGetCapture();
	if (tkwin) {
	    winPtr = (TkWindow *) tkwin;
	    eventWindow = TkMacOSXDrawableWindow(winPtr->window);
	    if (eventWindow) {
		local = [eventWindow tkConvertPointFromScreen: global];
	    } else {
		return theEvent;
	    }
	}
	local.y = [eventWindow frame].size.height - local.y;
	global.y = TkMacOSXZeroScreenHeight() - global.y;
    } else {

	/*
	 * If the event has no NSWindow, the location is in screen coordinates.
	 */

	global = [theEvent locationInWindow];
	tkwin = TkMacOSXGetCapture();
	if (tkwin) {
	    winPtr = (TkWindow *) tkwin;
	    eventWindow = TkMacOSXDrawableWindow(winPtr->window);
	} else {
	    eventWindow = [NSApp mainWindow];
	}
	if (!eventWindow) {
	    return theEvent;
	}
	local = [eventWindow tkConvertPointFromScreen: global];
	local.y = [eventWindow frame].size.height - local.y;
	global.y = TkMacOSXZeroScreenHeight() - global.y;
    }

    /*
     * If we still don't have a window, try using the toplevel that
     * manages the NSWindow.
     */

    if (!tkwin) {
	winPtr = TkMacOSXGetTkWindow(eventWindow);
	tkwin = (Tk_Window) winPtr;
    }
    if (!tkwin) {

	/*
	 * We can't find a window for this event.  We have to ignore it.
	 */

#ifdef TK_MAC_DEBUG_EVENTS
	TkMacOSXDbgMsg("tkwin == NULL");
#endif
	return theEvent;
    }

    /*
     * Ignore the event if a local grab is in effect and the Tk event window is
     * not in the grabber's subtree.
     */

    grabWinPtr = winPtr->dispPtr->grabWinPtr;
    if (grabWinPtr && /* There is a grab in effect ... */
	!winPtr->dispPtr->grabFlags && /* and it is a local grab ... */
	grabWinPtr->mainPtr == winPtr->mainPtr){ /* in the same application. */
	Tk_Window tkwin2, tkEventWindow = Tk_CoordsToWindow(global.x, global.y, tkwin);
	if (!tkEventWindow) {
	    return theEvent;
	}
	for (tkwin2 = tkEventWindow;
	     !Tk_IsTopLevel(tkwin2);
	     tkwin2 = Tk_Parent(tkwin2)) {
	    if (tkwin2 == (Tk_Window) grabWinPtr) {
		break;
	    }
	}
	if (tkwin2 != (Tk_Window) grabWinPtr) {
	    return theEvent;
	}
    }

    /*
     * Convert local from NSWindow flipped coordinates to the toplevel's
     * coordinates.
     */

    if (Tk_IsEmbedded(winPtr)) {
	TkWindow *contPtr = TkpGetOtherWindow(winPtr);
	if (Tk_IsTopLevel(contPtr)) {
	    local.x -= contPtr->wmInfoPtr->xInParent;
	    local.y -= contPtr->wmInfoPtr->yInParent;
	} else {
	    TkWindow *topPtr = TkMacOSXGetHostToplevel(winPtr)->winPtr;
	    local.x -= (topPtr->wmInfoPtr->xInParent + contPtr->changes.x);
	    local.y -= (topPtr->wmInfoPtr->yInParent + contPtr->changes.y);
	}
    } else {
	local.x -= winPtr->wmInfoPtr->xInParent;
	local.y -= winPtr->wmInfoPtr->yInParent;
    }

    /*
     * Use the toplevel coordinates to find the containing Tk window.  Then
     * convert local into the coordinates of that window.  (The converted
     * local coordinates are only needed for scrollwheel events.)
     */

    int win_x, win_y;
    tkwin = Tk_TopCoordsToWindow(tkwin, local.x, local.y, &win_x, &win_y);
    local.x = win_x;
    local.y = win_y;

    /*
     *  Generate an XEvent for this mouse event.
     */

    unsigned int state = 0;
    int button = [theEvent buttonNumber] + Button1;
    EventRef eventRef = (EventRef)[theEvent eventRef];
    UInt32 buttons;
    OSStatus err = GetEventParameter(eventRef, kEventParamMouseChord,
	    typeUInt32, NULL, sizeof(UInt32), NULL, &buttons);

    if (err == noErr) {
	state |= (buttons & 0x1F) * Button1Mask;
    } else if (button <= Button5) {
	switch (eventType) {
	case NSLeftMouseDown:
	case NSRightMouseDown:
	case NSLeftMouseDragged:
	case NSRightMouseDragged:
	case NSOtherMouseDown:
	    state |= TkGetButtonMask(button);
	    break;
	default:
	    break;
	}
    }

    NSUInteger modifiers = [theEvent modifierFlags];

    if (modifiers & NSAlphaShiftKeyMask) {
	state |= LockMask;
    }
    if (modifiers & NSShiftKeyMask) {
	state |= ShiftMask;
    }
    if (modifiers & NSControlKeyMask) {
	state |= ControlMask;
    }
    if (modifiers & NSCommandKeyMask) {
	state |= Mod1Mask;		/* command key */
    }
    if (modifiers & NSAlternateKeyMask) {
	state |= Mod2Mask;		/* option key */
    }
    if (modifiers & NSNumericPadKeyMask) {
	state |= Mod3Mask;
    }
    if (modifiers & NSFunctionKeyMask) {
	state |= Mod4Mask;
    }

    if (eventType != NSScrollWheel) {

	/*
	 * For normal mouse events, Tk_UpdatePointer will send the XEvent.
	 */

#ifdef TK_MAC_DEBUG_EVENTS
	TKLog(@"UpdatePointer %p x %f.0 y %f.0 %d",
		tkwin, global.x, global.y, state);
#endif
	Tk_UpdatePointer(tkwin, global.x, global.y, state);
    } else {

	/*
	 * For scroll wheel events we need to send the XEvent here.
	 */

	CGFloat delta;
	int coarseDelta;
	XEvent xEvent;

	xEvent.type = MouseWheelEvent;
	xEvent.xbutton.x = local.x;
	xEvent.xbutton.y = local.y;
	xEvent.xbutton.x_root = global.x;
	xEvent.xbutton.y_root = global.y;
	xEvent.xany.send_event = false;
	xEvent.xany.display = Tk_Display(tkwin);
	xEvent.xany.window = Tk_WindowId(tkwin);

	delta = [theEvent deltaY];
	if (delta != 0.0) {
	    coarseDelta = (delta > -1.0 && delta < 1.0) ?
		    (signbit(delta) ? -1 : 1) : lround(delta);
	    xEvent.xbutton.state = state;
	    xEvent.xkey.keycode = coarseDelta;
	    xEvent.xany.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
	    Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);
	}
	delta = [theEvent deltaX];
	if (delta != 0.0) {
	    coarseDelta = (delta > -1.0 && delta < 1.0) ?
		    (signbit(delta) ? -1 : 1) : lround(delta);
	    xEvent.xbutton.state = state | ShiftMask;
	    xEvent.xkey.keycode = coarseDelta;
	    xEvent.xany.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
	    Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);
	}
    }
    return theEvent;
}
@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXKeyModifiers --
 *
 *	Returns the current state of the modifier keys.
 *
 * Results:
 *	An OS Modifier state.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

EventModifiers
TkMacOSXModifierState(void)
{
    UInt32 keyModifiers;
    int isFrontProcess = (GetCurrentEvent() && Tk_MacOSXIsAppInFront());

    keyModifiers = isFrontProcess ? GetCurrentEventKeyModifiers() :
	    GetCurrentKeyModifiers();

    return (EventModifiers) (keyModifiers & USHRT_MAX);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXButtonKeyState --
 *
 *	Returns the current state of the button & modifier keys.
 *
 * Results:
 *	A bitwise inclusive OR of a subset of the following: Button1Mask,
 *	ShiftMask, LockMask, ControlMask, Mod*Mask.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int
TkMacOSXButtonKeyState(void)
{
    UInt32 buttonState = 0, keyModifiers;
    int isFrontProcess = (GetCurrentEvent() && Tk_MacOSXIsAppInFront());

    buttonState = isFrontProcess ? GetCurrentEventButtonState() :
	    GetCurrentButtonState();
    keyModifiers = isFrontProcess ? GetCurrentEventKeyModifiers() :
	    GetCurrentKeyModifiers();

    return ButtonModifiers2State(buttonState, keyModifiers);
}

/*
 *----------------------------------------------------------------------
 *
 * ButtonModifiers2State --
 *
 *	Converts Carbon mouse button state and modifier values into a Tk
 *	button/modifier state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
ButtonModifiers2State(
    UInt32 buttonState,
    UInt32 keyModifiers)
{
    unsigned int state;

    /*
     * Tk on OSX supports at most 5 buttons.
     */

    state = (buttonState & 0x1F) * Button1Mask;

    if (keyModifiers & alphaLock) {
	state |= LockMask;
    }
    if (keyModifiers & shiftKey) {
	state |= ShiftMask;
    }
    if (keyModifiers & controlKey) {
	state |= ControlMask;
    }
    if (keyModifiers & cmdKey) {
	state |= Mod1Mask;		/* command key */
    }
    if (keyModifiers & optionKey) {
	state |= Mod2Mask;		/* option key */
    }
    if (keyModifiers & kEventKeyModifierNumLockMask) {
	state |= Mod3Mask;
    }
    if (keyModifiers & kEventKeyModifierFnMask) {
	state |= Mod4Mask;
    }

    return state;
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
    int getGlobal = (root_x_return && root_y_return);
    int getLocal = (win_x_return && win_y_return && w != None);

    if (getGlobal || getLocal) {
	NSPoint global = [NSEvent mouseLocation];

	if (getLocal) {
	    MacDrawable *macWin = (MacDrawable *) w;
	    NSWindow *win = TkMacOSXDrawableWindow(w);

	    if (win) {
		NSPoint local;

		local = [win tkConvertPointFromScreen:global];
		local.y = [win frame].size.height - local.y;
		if (macWin->winPtr && macWin->winPtr->wmInfoPtr) {
		    local.x -= macWin->winPtr->wmInfoPtr->xInParent;
		    local.y -= macWin->winPtr->wmInfoPtr->yInParent;
		}
		*win_x_return = local.x;
		*win_y_return = local.y;
	    }
	}
	if (getGlobal) {
	    *root_x_return = global.x;
	    *root_y_return = TkMacOSXZeroScreenHeight() - global.y;
	}
    }
    if (mask_return) {
	*mask_return = TkMacOSXButtonKeyState();
    }
    return True;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenerateButtonEventForXPointer --
 *
 *	This procedure generates an X button event for the current pointer
 *	state as reported by XQueryPointer().
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue. Grab state may
 *	also change.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE int
TkGenerateButtonEventForXPointer(
    Window window)		/* X Window containing button event. */
{
    MouseEventData med;
    int global_x, global_y, local_x, local_y;

    bzero(&med, sizeof(MouseEventData));
    XQueryPointer(NULL, window, NULL, NULL, &global_x, &global_y,
	    &local_x, &local_y, &med.state);
    med.global.h = global_x;
    med.global.v = global_y;
    med.local.h = local_x;
    med.local.v = local_y;
    med.window = window;

    return GenerateButtonEvent(&med);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenerateButtonEvent --
 *
 *	Given a global x & y position and the button key status this procedure
 *	generates the appropriate X button event. It also handles the state
 *	changes needed to implement implicit grabs.
 *
 * Results:
 *	True if event(s) are generated, false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue. Grab state may
 *	also change.
 *
 *----------------------------------------------------------------------
 */

int
TkGenerateButtonEvent(
    int x,			/* X location of mouse, */
    int y,			/* Y location of mouse. */
    Window window,		/* X Window containing button event. */
    unsigned int state)		/* Button Key state suitable for X event. */
{
    MacDrawable *macWin = (MacDrawable *) window;
    NSWindow *win = TkMacOSXDrawableWindow(window);
    MouseEventData med;

    bzero(&med, sizeof(MouseEventData));
    med.state = state;
    med.window = window;
    med.global.h = x;
    med.global.v = y;
    med.local = med.global;

    if (win) {
	NSPoint local = NSMakePoint(x, TkMacOSXZeroScreenHeight() - y);

	local = [win tkConvertPointFromScreen:local];
	local.y = [win frame].size.height - local.y;
	if (macWin->winPtr && macWin->winPtr->wmInfoPtr) {
	    local.x -= macWin->winPtr->wmInfoPtr->xInParent;
	    local.y -= macWin->winPtr->wmInfoPtr->yInParent;
	}
	med.local.h = local.x;
	med.local.v = TkMacOSXZeroScreenHeight() - local.y;
    }

    return GenerateButtonEvent(&med);
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateButtonEvent --
 *
 *	Generate an X button event from a MouseEventData structure. Handles
 *	the state changes needed to implement implicit grabs.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue. Grab state may
 *	also change.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateButtonEvent(
    MouseEventData *medPtr)
{
    Tk_Window tkwin;
    int dummy;
    TkDisplay *dispPtr;

#if UNUSED

    /*
     * ButtonDown events will always occur in the front window. ButtonUp
     * events, however, may occur anywhere on the screen. ButtonUp events
     * should only be sent to Tk if in the front window or during an implicit
     * grab.
     */

    if ((medPtr->activeNonFloating == NULL)
	    || ((!(TkpIsWindowFloating(medPtr->whichWin))
	    && (medPtr->activeNonFloating != medPtr->whichWin))
	    && TkMacOSXGetCapture() == NULL)) {
	return false;
    }
#endif

    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, medPtr->window);

    if (tkwin != NULL) {
	tkwin = Tk_TopCoordsToWindow(tkwin, medPtr->local.h, medPtr->local.v,
		&dummy, &dummy);
    }

    Tk_UpdatePointer(tkwin, medPtr->global.h, medPtr->global.v, medPtr->state);
    return true;
}

void
TkpWarpPointer(
    TkDisplay *dispPtr)
{
    CGPoint pt;
    NSPoint loc;
    int wNum;

    if (dispPtr->warpWindow) {
	int x, y;
	TkWindow *winPtr = (TkWindow *) dispPtr->warpWindow;
	TkWindow *topPtr = winPtr->privatePtr->toplevel->winPtr;
	NSWindow *w = TkMacOSXDrawableWindow(winPtr->window);
	wNum = [w windowNumber];
	Tk_GetRootCoords(dispPtr->warpWindow, &x, &y);
	pt.x = x + dispPtr->warpX;
	pt.y = y + dispPtr->warpY;
	loc.x = dispPtr->warpX;
	loc.y = Tk_Height(topPtr) - dispPtr->warpY;
    } else {
	wNum = 0;
	pt.x = loc.x = dispPtr->warpX;
	pt.y = dispPtr->warpY;
	loc.y = TkMacOSXZeroScreenHeight() - pt.y;
    }

    /*
     * Generate an NSEvent of type NSMouseMoved.
     *
     * It is not clear why this is necessary.  For example, calling
     *     event generate $w <Motion> -warp 1 -x $X -y $Y
     * will cause two <Motion> events to be added to the Tcl queue.
     */

    CGWarpMouseCursorPosition(pt);
    NSEvent *warpEvent = [NSEvent mouseEventWithType:NSMouseMoved
	location:loc
	modifierFlags:0
	timestamp:GetCurrentEventTime()
	windowNumber:wNum
	context:nil
	eventNumber:0
	clickCount:1
	pressure:0.0];
    [NSApp postEvent:warpEvent atStart:NO];
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
    while (winPtr && !Tk_IsTopLevel(winPtr)) {
	winPtr = winPtr->parentPtr;
    }
    [NSEvent stopPeriodicEvents];
    captureWinPtr = (Tk_Window) winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetCapture --
 *
 * Results:
 *	Returns the current grab window
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
TkMacOSXGetCapture(void)
{
    return captureWinPtr;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
