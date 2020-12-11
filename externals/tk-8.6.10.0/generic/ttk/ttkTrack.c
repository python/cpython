/*
 * Copyright (c) 2004, Joe English
 *
 * TtkTrackElementState() -- helper routine for widgets
 * like scrollbars in which individual elements may
 * be active or pressed instead of the widget as a whole.
 *
 * Usage:
 * 	TtkTrackElementState(&recordPtr->core);
 *
 * Registers an event handler on the widget that tracks pointer
 * events and updates the state of the element under the
 * mouse cursor.
 *
 * The "active" element is the one under the mouse cursor,
 * and is normally set to the ACTIVE state unless another element
 * is currently being pressed.
 *
 * The active element becomes "pressed" on <ButtonPress> events,
 * and remains "active" and "pressed" until the corresponding
 * <ButtonRelease> event.
 *
 * TODO: Handle "chords" properly (e.g., <B1-ButtonPress-2>)
 */

#include <tk.h>
#include "ttkTheme.h"
#include "ttkWidget.h"

typedef struct {
    WidgetCore		*corePtr;	/* widget to track */
    Ttk_Layout		tracking;	/* current layout being tracked */
    Ttk_Element 	activeElement;	/* element under the mouse cursor */
    Ttk_Element 	pressedElement; /* currently pressed element */
} ElementStateTracker;

/*
 * ActivateElement(es, node) --
 * 	Make 'node' the active element if non-NULL.
 * 	Deactivates the currently active element if different.
 *
 * 	The active element has TTK_STATE_ACTIVE set _unless_
 * 	another element is 'pressed'
 */
static void ActivateElement(ElementStateTracker *es, Ttk_Element element)
{
    if (es->activeElement == element) {
	/* No change */
	return;
    }

    if (!es->pressedElement) {
	if (es->activeElement) {
	    /* Deactivate old element */
	    Ttk_ChangeElementState(es->activeElement, 0,TTK_STATE_ACTIVE);
	}
	if (element) {
	    /* Activate new element */
	    Ttk_ChangeElementState(element, TTK_STATE_ACTIVE,0);
	}
	TtkRedisplayWidget(es->corePtr);
    }

    es->activeElement = element;
}

/* ReleaseElement --
 * 	Releases the currently pressed element, if any.
 */
static void ReleaseElement(ElementStateTracker *es)
{
    if (!es->pressedElement)
	return;

    Ttk_ChangeElementState(
	es->pressedElement, 0,TTK_STATE_PRESSED|TTK_STATE_ACTIVE);
    es->pressedElement = 0;

    /* Reactivate element under the mouse cursor:
     */
    if (es->activeElement)
	Ttk_ChangeElementState(es->activeElement, TTK_STATE_ACTIVE,0);

    TtkRedisplayWidget(es->corePtr);
}

/* PressElement --
 * 	Presses the specified element.
 */
static void PressElement(ElementStateTracker *es, Ttk_Element element)
{
    if (es->pressedElement) {
	ReleaseElement(es);
    }

    if (element) {
	Ttk_ChangeElementState(
	    element, TTK_STATE_PRESSED|TTK_STATE_ACTIVE, 0);
    }

    es->pressedElement = element;
    TtkRedisplayWidget(es->corePtr);
}

/* ElementStateEventProc --
 * 	Event handler for tracking element states.
 */

static const unsigned ElementStateMask =
      ButtonPressMask
    | ButtonReleaseMask
    | PointerMotionMask
    | LeaveWindowMask
    | EnterWindowMask
    | StructureNotifyMask
    ;

static void
ElementStateEventProc(ClientData clientData, XEvent *ev)
{
    ElementStateTracker *es = clientData;
    Ttk_Layout layout = es->corePtr->layout;
    Ttk_Element element;

    /* Guard against dangling pointers [#2431428]
     */
    if (es->tracking != layout) {
	es->pressedElement = es->activeElement = 0;
	es->tracking = layout;
    }

    switch (ev->type)
    {
	case MotionNotify :
	    element = Ttk_IdentifyElement(
		layout, ev->xmotion.x, ev->xmotion.y);
	    ActivateElement(es, element);
	    break;
	case LeaveNotify:
	    ActivateElement(es, 0);
	    if (ev->xcrossing.mode == NotifyGrab)
		PressElement(es, 0);
	    break;
	case EnterNotify:
	    element = Ttk_IdentifyElement(
		layout, ev->xcrossing.x, ev->xcrossing.y);
	    ActivateElement(es, element);
	    break;
	case ButtonPress:
	    element = Ttk_IdentifyElement(
		layout, ev->xbutton.x, ev->xbutton.y);
	    if (element)
		PressElement(es, element);
	    break;
	case ButtonRelease:
	    ReleaseElement(es);
	    break;
	case DestroyNotify:
	    /* Unregister this event handler and free client data.
	     */
	    Tk_DeleteEventHandler(es->corePtr->tkwin,
		    ElementStateMask, ElementStateEventProc, es);
	    ckfree(clientData);
	    break;
    }
}

/*
 * TtkTrackElementState --
 * 	Register an event handler to manage the 'pressed'
 * 	and 'active' states of individual widget elements.
 */

void TtkTrackElementState(WidgetCore *corePtr)
{
    ElementStateTracker *es = ckalloc(sizeof(*es));
    es->corePtr = corePtr;
    es->tracking = 0;
    es->activeElement = es->pressedElement = 0;
    Tk_CreateEventHandler(corePtr->tkwin,
	    ElementStateMask,ElementStateEventProc,es);
}

