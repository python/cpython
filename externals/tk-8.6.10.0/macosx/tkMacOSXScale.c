/*
 * tkMacOSXScale.c --
 *
 *	This file implements the Macintosh specific portion of the
 *	scale widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2008-2009, Apple Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkScale.h"

#ifdef MAC_OSX_TK_TODO
/*
#ifdef TK_MAC_DEBUG
#define TK_MAC_DEBUG_SCALE
#endif
*/

/*
 * Defines used in this file.
 */

#define slider		1110
#define inSlider	1
#define inInc		2
#define inDecr		3

/*
 * Declaration of Macintosh specific scale structure.
 */

typedef struct MacScale {
    TkScale info;		/* Generic scale info. */
    int flags;			/* Flags. */
    ControlRef scaleHandle;	/* Handle to the Scale control struct. */
} MacScale;

/*
 * Globals uses locally in this file.
 */
static ControlActionUPP scaleActionProc = NULL; /* Pointer to func. */

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		MacScaleEventProc(ClientData clientData,
			    XEvent *eventPtr);
static pascal void	ScaleActionProc(ControlRef theControl,
			    ControlPartCode partCode);

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateScale --
 *
 *	Allocate a new TkScale structure.
 *
 * Results:
 *	Returns a newly allocated TkScale structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkScale *
TkpCreateScale(
    Tk_Window tkwin)
{
    MacScale *macScalePtr = ckalloc(sizeof(MacScale));

    macScalePtr->scaleHandle = NULL;
    if (scaleActionProc == NULL) {
	scaleActionProc = NewControlActionUPP(ScaleActionProc);
    }

    Tk_CreateEventHandler(tkwin, ButtonPressMask,
	    MacScaleEventProc, macScalePtr);

    return (TkScale *) macScalePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyScale --
 *
 *	Free Macintosh specific resources.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The slider control is destroyed.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyScale(
    TkScale *scalePtr)
{
    MacScale *macScalePtr = (MacScale *) scalePtr;

    /*
     * Free Macintosh control.
     */

    if (macScalePtr->scaleHandle != NULL) {
	DisposeControl(macScalePtr->scaleHandle);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayScale --
 *
 *	This procedure is invoked as an idle handler to redisplay the contents
 *	of a scale widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The scale gets redisplayed.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayScale(
    ClientData clientData)	/* Widget record for scale. */
{
    TkScale *scalePtr = clientData;
    Tk_Window tkwin = scalePtr->tkwin;
    Tcl_Interp *interp = scalePtr->interp;
    int result;
    char string[TCL_DOUBLE_SPACE];
    MacScale *macScalePtr = clientData;
    Rect r;
    WindowRef windowRef;
    CGrafPtr destPort, savePort;
    Boolean portChanged;
    MacDrawable *macDraw;
    SInt32 initialValue, minValue, maxValue;
    UInt16 numTicks;
    Tcl_DString buf;

#ifdef TK_MAC_DEBUG_SCALE
    TkMacOSXDbgMsg("TkpDisplayScale");
#endif
    scalePtr->flags &= ~REDRAW_PENDING;
    if ((scalePtr->tkwin == NULL) || !Tk_IsMapped(scalePtr->tkwin)) {
	goto done;
    }

    /*
     * Invoke the scale's command if needed.
     */

    Tcl_Preserve(scalePtr);
    if ((scalePtr->flags & INVOKE_COMMAND) && (scalePtr->command != NULL)) {
	Tcl_Preserve(interp);
        if (snprintf(string, TCL_DOUBLE_SPACE, scalePtr->format,
                scalePtr->value) < 0) {
            string[TCL_DOUBLE_SPACE - 1] = '\0';
        }
	Tcl_DStringInit(&buf);
	Tcl_DStringAppend(&buf, scalePtr->command, -1);
	Tcl_DStringAppend(&buf, " ", -1);
	Tcl_DStringAppend(&buf, string, -1);
	result = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
	Tcl_DStringFree(&buf);
	if (result != TCL_OK) {
	    Tcl_AddErrorInfo(interp, "\n    (command executed by scale)");
	    Tcl_BackgroundException(interp, result);
	}
	Tcl_Release(interp);
    }
    scalePtr->flags &= ~INVOKE_COMMAND;
    if (scalePtr->flags & SCALE_DELETED) {
	Tcl_Release(scalePtr);
	return;
    }
    Tcl_Release(scalePtr);

    /*
     * Now handle the part of redisplay that is the same for horizontal and
     * vertical scales: border and traversal highlight.
     */

    if (scalePtr->highlightWidth != 0) {
	GC gc = Tk_GCForColor(scalePtr->highlightColorPtr, Tk_WindowId(tkwin));

	Tk_DrawFocusHighlight(tkwin, gc, scalePtr->highlightWidth,
		Tk_WindowId(tkwin));
    }
    Tk_Draw3DRectangle(tkwin, Tk_WindowId(tkwin), scalePtr->bgBorder,
	    scalePtr->highlightWidth, scalePtr->highlightWidth,
	    Tk_Width(tkwin) - 2*scalePtr->highlightWidth,
	    Tk_Height(tkwin) - 2*scalePtr->highlightWidth,
	    scalePtr->borderWidth, scalePtr->relief);

    /*
     * Set up port for drawing Macintosh control.
     */

    macDraw = (MacDrawable *) Tk_WindowId(tkwin);
    destPort = TkMacOSXGetDrawablePort(Tk_WindowId(tkwin));
    windowRef = TkMacOSXDrawableWindow(Tk_WindowId(tkwin));
    portChanged = QDSwapPort(destPort, &savePort);
    TkMacOSXSetUpClippingRgn(Tk_WindowId(tkwin));

    /*
     * Create Macintosh control.
     */

#define MAC_OSX_SCROLL_WIDTH 10

    if (scalePtr->orient == ORIENT_HORIZONTAL) {
	int offset = (Tk_Height(tkwin) - MAC_OSX_SCROLL_WIDTH) / 2;

	if (offset < 0) {
	    offset = 0;
	}

	r.left = macDraw->xOff + scalePtr->inset;
	r.top = macDraw->yOff + offset;
	r.right = macDraw->xOff+Tk_Width(tkwin) - scalePtr->inset;
	r.bottom = macDraw->yOff + offset + MAC_OSX_SCROLL_WIDTH/2;
    } else {
	int offset = (Tk_Width(tkwin) - MAC_OSX_SCROLL_WIDTH) / 2;

	if (offset < 0) {
	    offset = 0;
	}

	r.left = macDraw->xOff + offset;
	r.top = macDraw->yOff + scalePtr->inset;
	r.right = macDraw->xOff + offset + MAC_OSX_SCROLL_WIDTH/2;
	r.bottom = macDraw->yOff + Tk_Height(tkwin) - scalePtr->inset;
    }

    if (macScalePtr->scaleHandle == NULL) {
#ifdef TK_MAC_DEBUG_SCALE
	TkMacOSXDbgMsg("Initialising scale");
#endif
	initialValue = scalePtr->value;
	if (scalePtr->orient == ORIENT_HORIZONTAL) {
	    minValue = scalePtr->fromValue;
	    maxValue = scalePtr->toValue;
	} else {
	    minValue = scalePtr->fromValue;
	    maxValue = scalePtr->toValue;
	}

	if (scalePtr->tickInterval == 0) {
	    numTicks = 0;
	} else {
	    numTicks = (maxValue - minValue)/scalePtr->tickInterval;
	}

	CreateSliderControl(windowRef, &r, initialValue, minValue, maxValue,
		kControlSliderPointsDownOrRight, numTicks, 1, scaleActionProc,
		&macScalePtr->scaleHandle);
	SetControlReference(macScalePtr->scaleHandle, (UInt32) scalePtr);

	if (IsWindowActive(windowRef)) {
	    macScalePtr->flags |= ACTIVE;
	}
    } else {
	SetControlBounds(macScalePtr->scaleHandle, &r);
	SetControl32BitValue(macScalePtr->scaleHandle, scalePtr->value);
	SetControl32BitMinimum(macScalePtr->scaleHandle, scalePtr->fromValue);
	SetControl32BitMaximum(macScalePtr->scaleHandle, scalePtr->toValue);
    }

    /*
     * Finally draw the control.
     */

    SetControlVisibility(macScalePtr->scaleHandle, true, true);
    HiliteControl(macScalePtr->scaleHandle, 0);
    Draw1Control(macScalePtr->scaleHandle);

    if (portChanged) {
	QDSwapPort(savePort, NULL);
    }
done:
    scalePtr->flags &= ~REDRAW_ALL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpScaleElement --
 *
 *	Determine which part of a scale widget lies under a given point.
 *
 * Results:
 *	The return value is either TROUGH1, SLIDER, TROUGH2, or OTHER,
 *	depending on which of the scale's active elements (if any) is under the
 *	point at (x,y).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpScaleElement(
    TkScale *scalePtr,		/* Widget record for scale. */
    int x, int y)		/* Coordinates within scalePtr's window. */
{
    MacScale *macScalePtr = (MacScale *) scalePtr;
    ControlPartCode part;
    Point where;
    Rect bounds;
    CGrafPtr destPort, savePort;
    Boolean portChanged;

#ifdef TK_MAC_DEBUG_SCALE
    TkMacOSXDbgMsg("TkpScaleElement");
#endif
    destPort = TkMacOSXGetDrawablePort(Tk_WindowId(scalePtr->tkwin));
    portChanged = QDSwapPort(destPort, &savePort);

    /*
     * All of the calculations in this procedure mirror those in
     * DisplayScrollbar. Be sure to keep the two consistent.
     */

    TkMacOSXWinBounds((TkWindow *) scalePtr->tkwin, &bounds);
    where.h = x + bounds.left;
    where.v = y + bounds.top;
    part = TestControl(macScalePtr->scaleHandle, where);

    if (portChanged) {
	QDSwapPort(savePort, NULL);
    }

#ifdef TK_MAC_DEBUG_SCALE
    fprintf (stderr,"ScalePart %d, pos ( %d %d )\n", part, where.h, where.v );
#endif

    switch (part) {
    case inSlider:
	return SLIDER;
    case inInc:
	if (scalePtr->orient == ORIENT_VERTICAL) {
	    return TROUGH1;
	} else {
	    return TROUGH2;
	}
    case inDecr:
	if (scalePtr->orient == ORIENT_VERTICAL) {
	    return TROUGH2;
	} else {
	    return TROUGH1;
	}
    default:
	return OTHER;
    }
}

/*
 *--------------------------------------------------------------
 *
 * MacScaleEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for ButtonPress events
 *	on scales.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up. When
 *	it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
MacScaleEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    MacScale *macScalePtr = (MacScale *) clientData;
    Point where;
    Rect bounds;
    int part;
    CGrafPtr destPort, savePort;
    Boolean portChanged;

#ifdef TK_MAC_DEBUG_SCALE
    fprintf(stderr,"MacScaleEventProc\n" );
#endif

    /*
     * To call Macintosh control routines we must have the port set to the
     * window containing the control. We will then test which part of the
     * control was hit and act accordingly.
     */

    destPort = TkMacOSXGetDrawablePort(Tk_WindowId(macScalePtr->info.tkwin));
    portChanged = QDSwapPort(destPort, &savePort);
    TkMacOSXSetUpClippingRgn(Tk_WindowId(macScalePtr->info.tkwin));

    TkMacOSXWinBounds((TkWindow *) macScalePtr->info.tkwin, &bounds);
    where.h = eventPtr->xbutton.x + bounds.left;
    where.v = eventPtr->xbutton.y + bounds.top;
#ifdef TK_MAC_DEBUG_SCALE
    TkMacOSXDbgMsg("calling TestControl");
#endif
    part = TestControl(macScalePtr->scaleHandle, where);
    if (part == 0) {
	return;
    }

    TkMacOSXTrackingLoop(1);
    part = HandleControlClick(macScalePtr->scaleHandle, where,
	    TkMacOSXModifierState(), scaleActionProc);
    TkMacOSXTrackingLoop(0);

    /*
     * Update the value for the widget.
     */

    macScalePtr->info.value = GetControlValue(macScalePtr->scaleHandle);
    /* TkScaleSetValue(&macScalePtr->info, macScalePtr->info.value, 1, 0); */

    /*
     * The HandleControlClick call will "eat" the ButtonUp event. We now
     * generate a ButtonUp event so Tk will unset implicit grabs etc.
     */

    TkGenerateButtonEventForXPointer(Tk_WindowId(macScalePtr->info.tkwin));

    if (portChanged) {
	QDSwapPort(savePort, NULL);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ScaleActionProc --
 *
 *	Callback procedure used by the Macintosh toolbox call
 *	HandleControlClick. This call will update the display while the
 *	scrollbar is being manipulated by the user.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May change the display.
 *
 *--------------------------------------------------------------
 */

static pascal void
ScaleActionProc(
    ControlRef theControl,	/* Handle to scrollbat control */
    ControlPartCode partCode)	/* Part of scrollbar that was "hit" */
{
    int value;
    TkScale *scalePtr = (TkScale *) GetControlReference(theControl);

#ifdef TK_MAC_DEBUG_SCALE
    TkMacOSXDbgMsg("ScaleActionProc");
#endif
    value = GetControlValue(theControl);
    TkScaleSetValue(scalePtr, value, 1, 1);
    Tcl_Preserve(scalePtr);
    TkMacOSXRunTclEventLoop();
    Tcl_Release(scalePtr);
}
#endif

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
