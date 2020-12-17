/*
 * tkUnixScale.c --
 *
 *	This file implements the X specific portion of the scrollbar widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkScale.h"

#if defined(_WIN32)
#define snprintf _snprintf
#endif

/*
 * Forward declarations for functions defined later in this file:
 */

static void		DisplayHorizontalScale(TkScale *scalePtr,
			    Drawable drawable, XRectangle *drawnAreaPtr);
static void		DisplayHorizontalValue(TkScale *scalePtr,
			    Drawable drawable, double value, int top,
			    const char *format);
static void		DisplayVerticalScale(TkScale *scalePtr,
			    Drawable drawable, XRectangle *drawnAreaPtr);
static void		DisplayVerticalValue(TkScale *scalePtr,
			    Drawable drawable, double value, int rightEdge,
			    const char *format);

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
    return ckalloc(sizeof(TkScale));
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyScale --
 *
 *	Destroy a TkScale structure. It's necessary to do this with
 *	Tcl_EventuallyFree to allow the Tcl_Preserve(scalePtr) to work as
 *	expected in TkpDisplayScale. (hobbs)
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory is freed.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyScale(
    TkScale *scalePtr)
{
    Tcl_EventuallyFree(scalePtr, TCL_DYNAMIC);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayVerticalScale --
 *
 *	This function redraws the contents of a vertical scale window. It is
 *	invoked as a do-when-idle handler, so it only runs when there's
 *	nothing else for the application to do.
 *
 * Results:
 *	There is no return value. If only a part of the scale needs to be
 *	redrawn, then drawnAreaPtr is modified to reflect the area that was
 *	actually modified.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayVerticalScale(
    TkScale *scalePtr,		/* Widget record for scale. */
    Drawable drawable,		/* Where to display scale (window or
				 * pixmap). */
    XRectangle *drawnAreaPtr)	/* Initally contains area of window; if only a
				 * part of the scale is redrawn, gets modified
				 * to reflect the part of the window that was
				 * redrawn. */
{
    Tk_Window tkwin = scalePtr->tkwin;
    int x, y, width, height, shadowWidth;
    double tickValue, tickInterval = scalePtr->tickInterval;
    Tk_3DBorder sliderBorder;

    /*
     * Display the information from left to right across the window.
     */

    if (!(scalePtr->flags & REDRAW_OTHER)) {
	drawnAreaPtr->x = scalePtr->vertTickRightX;
	drawnAreaPtr->y = scalePtr->inset;
	drawnAreaPtr->width = scalePtr->vertTroughX + scalePtr->width
		+ 2*scalePtr->borderWidth - scalePtr->vertTickRightX;
	drawnAreaPtr->height -= 2*scalePtr->inset;
    }
    Tk_Fill3DRectangle(tkwin, drawable, scalePtr->bgBorder,
	    drawnAreaPtr->x, drawnAreaPtr->y, drawnAreaPtr->width,
	    drawnAreaPtr->height, 0, TK_RELIEF_FLAT);
    if (scalePtr->flags & REDRAW_OTHER) {
	/*
	 * Display the tick marks.
	 */

	if (tickInterval != 0) {
	    double ticks, maxTicks;

	    /*
	     * Ensure that we will only draw enough of the tick values such
	     * that they don't overlap
	     */

	    ticks = fabs((scalePtr->toValue - scalePtr->fromValue)
		    / tickInterval);
	    maxTicks = (double) Tk_Height(tkwin)
		    / (double) scalePtr->fontHeight;
	    if (ticks > maxTicks) {
		tickInterval *= (ticks / maxTicks);
	    }
	    for (tickValue = scalePtr->fromValue; ;
		    tickValue += tickInterval) {
		/*
		 * The TkRoundValueToResolution call gets rid of accumulated
		 * round-off errors, if any.
		 */

		tickValue = TkRoundValueToResolution(scalePtr, tickValue);
		if (scalePtr->toValue >= scalePtr->fromValue) {
		    if (tickValue > scalePtr->toValue) {
			break;
		    }
		} else {
		    if (tickValue < scalePtr->toValue) {
			break;
		    }
		}
		DisplayVerticalValue(scalePtr, drawable, tickValue,
			scalePtr->vertTickRightX, scalePtr->tickFormat);
	    }
	}
    }

    /*
     * Display the value, if it is desired.
     */

    if (scalePtr->showValue) {
	DisplayVerticalValue(scalePtr, drawable, scalePtr->value,
		scalePtr->vertValueRightX, scalePtr->valueFormat);
    }

    /*
     * Display the trough and the slider.
     */

    Tk_Draw3DRectangle(tkwin, drawable,
	    scalePtr->bgBorder, scalePtr->vertTroughX, scalePtr->inset,
	    scalePtr->width + 2*scalePtr->borderWidth,
	    Tk_Height(tkwin) - 2*scalePtr->inset, scalePtr->borderWidth,
	    TK_RELIEF_SUNKEN);
    XFillRectangle(scalePtr->display, drawable, scalePtr->troughGC,
	    scalePtr->vertTroughX + scalePtr->borderWidth,
	    scalePtr->inset + scalePtr->borderWidth,
	    (unsigned) scalePtr->width,
	    (unsigned) (Tk_Height(tkwin) - 2*scalePtr->inset
		- 2*scalePtr->borderWidth));
    if (scalePtr->state == STATE_ACTIVE) {
	sliderBorder = scalePtr->activeBorder;
    } else {
	sliderBorder = scalePtr->bgBorder;
    }
    width = scalePtr->width;
    height = scalePtr->sliderLength/2;
    x = scalePtr->vertTroughX + scalePtr->borderWidth;
    y = TkScaleValueToPixel(scalePtr, scalePtr->value) - height;
    shadowWidth = scalePtr->borderWidth/2;
    if (shadowWidth == 0) {
	shadowWidth = 1;
    }
    Tk_Draw3DRectangle(tkwin, drawable, sliderBorder, x, y, width,
	    2*height, shadowWidth, scalePtr->sliderRelief);
    x += shadowWidth;
    y += shadowWidth;
    width -= 2*shadowWidth;
    height -= shadowWidth;
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x, y, width,
	    height, shadowWidth, scalePtr->sliderRelief);
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x, y+height,
	    width, height, shadowWidth, scalePtr->sliderRelief);

    /*
     * Draw the label to the right of the scale.
     */

    if ((scalePtr->flags & REDRAW_OTHER) && (scalePtr->labelLength != 0)) {
	Tk_FontMetrics fm;

	Tk_GetFontMetrics(scalePtr->tkfont, &fm);
	Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
		scalePtr->tkfont, scalePtr->label,
		scalePtr->labelLength, scalePtr->vertLabelX,
		scalePtr->inset + (3 * fm.ascent) / 2);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayVerticalValue --
 *
 *	This function is called to display values (scale readings) for
 *	vertically-oriented scales.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The numerical value corresponding to value is displayed with its right
 *	edge at "rightEdge", and at a vertical position in the scale that
 *	corresponds to "value".
 *
 *----------------------------------------------------------------------
 */

static void
DisplayVerticalValue(
    register TkScale *scalePtr,	/* Information about widget in which to
				 * display value. */
    Drawable drawable,		/* Pixmap or window in which to draw the
				 * value. */
    double value,		/* Y-coordinate of number to display,
				 * specified in application coords, not in
				 * pixels (we'll compute pixels). */
    int rightEdge,		/* X-coordinate of right edge of text,
				 * specified in pixels. */
    const char *format)		/* Format string to use for the value */
{
    register Tk_Window tkwin = scalePtr->tkwin;
    int y, width, length;
    char valueString[TCL_DOUBLE_SPACE];
    Tk_FontMetrics fm;

    Tk_GetFontMetrics(scalePtr->tkfont, &fm);
    y = TkScaleValueToPixel(scalePtr, value) + fm.ascent/2;
    if (snprintf(valueString, TCL_DOUBLE_SPACE, format, value) < 0) {
	valueString[TCL_DOUBLE_SPACE - 1] = '\0';
    }
    length = (int) strlen(valueString);
    width = Tk_TextWidth(scalePtr->tkfont, valueString, length);

    /*
     * Adjust the y-coordinate if necessary to keep the text entirely inside
     * the window.
     */

    if (y - fm.ascent < scalePtr->inset + SPACING) {
	y = scalePtr->inset + SPACING + fm.ascent;
    }
    if (y + fm.descent > Tk_Height(tkwin) - scalePtr->inset - SPACING) {
	y = Tk_Height(tkwin) - scalePtr->inset - SPACING - fm.descent;
    }
    Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
	    scalePtr->tkfont, valueString, length, rightEdge - width, y);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayHorizontalScale --
 *
 *	This function redraws the contents of a horizontal scale window. It is
 *	invoked as a do-when-idle handler, so it only runs when there's
 *	nothing else for the application to do.
 *
 * Results:
 *	There is no return value. If only a part of the scale needs to be
 *	redrawn, then drawnAreaPtr is modified to reflect the area that was
 *	actually modified.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayHorizontalScale(
    TkScale *scalePtr,		/* Widget record for scale. */
    Drawable drawable,		/* Where to display scale (window or
				 * pixmap). */
    XRectangle *drawnAreaPtr)	/* Initally contains area of window; if only a
				 * part of the scale is redrawn, gets modified
				 * to reflect the part of the window that was
				 * redrawn. */
{
    register Tk_Window tkwin = scalePtr->tkwin;
    int x, y, width, height, shadowWidth;
    double tickInterval = scalePtr->tickInterval;
    Tk_3DBorder sliderBorder;

    /*
     * Display the information from bottom to top across the window.
     */

    if (!(scalePtr->flags & REDRAW_OTHER)) {
	drawnAreaPtr->x = scalePtr->inset;
	drawnAreaPtr->y = scalePtr->horizValueY;
	drawnAreaPtr->width -= 2*scalePtr->inset;
	drawnAreaPtr->height = scalePtr->horizTroughY + scalePtr->width
		+ 2*scalePtr->borderWidth - scalePtr->horizValueY;
    }
    Tk_Fill3DRectangle(tkwin, drawable, scalePtr->bgBorder,
	    drawnAreaPtr->x, drawnAreaPtr->y, drawnAreaPtr->width,
	    drawnAreaPtr->height, 0, TK_RELIEF_FLAT);
    if (scalePtr->flags & REDRAW_OTHER) {
	/*
	 * Display the tick marks.
	 */

	if (tickInterval != 0) {
	    char valueString[TCL_DOUBLE_SPACE];
	    double ticks, maxTicks, tickValue;

	    /*
	     * Ensure that we will only draw enough of the tick values such
	     * that they don't overlap. We base this off the width that
	     * fromValue would take. Not exact, but better than no constraint.
	     */

	    ticks = fabs((scalePtr->toValue - scalePtr->fromValue)
		    / tickInterval);
	    if (snprintf(valueString, TCL_DOUBLE_SPACE, scalePtr->tickFormat,
		    scalePtr->fromValue) < 0) {
		valueString[TCL_DOUBLE_SPACE - 1] = '\0';
	    }
	    maxTicks = (double) Tk_Width(tkwin)
		    / (double) Tk_TextWidth(scalePtr->tkfont, valueString, -1);
	    if (ticks > maxTicks) {
		tickInterval *= ticks / maxTicks;
	    }
	    tickValue = scalePtr->fromValue;
	    while (1) {
		/*
		 * The TkRoundValueToResolution call gets rid of accumulated
		 * round-off errors, if any.
		 */

		tickValue = TkRoundValueToResolution(scalePtr, tickValue);
		if (scalePtr->toValue >= scalePtr->fromValue) {
		    if (tickValue > scalePtr->toValue) {
			break;
		    }
		} else {
		    if (tickValue < scalePtr->toValue) {
			break;
		    }
		}
		DisplayHorizontalValue(scalePtr, drawable, tickValue,
			scalePtr->horizTickY, scalePtr->tickFormat);
		tickValue += tickInterval;
	    }
	}
    }

    /*
     * Display the value, if it is desired.
     */

    if (scalePtr->showValue) {
	DisplayHorizontalValue(scalePtr, drawable, scalePtr->value,
		scalePtr->horizValueY, scalePtr->valueFormat);
    }

    /*
     * Display the trough and the slider.
     */

    y = scalePtr->horizTroughY;
    Tk_Draw3DRectangle(tkwin, drawable,
	    scalePtr->bgBorder, scalePtr->inset, y,
	    Tk_Width(tkwin) - 2*scalePtr->inset,
	    scalePtr->width + 2*scalePtr->borderWidth,
	    scalePtr->borderWidth, TK_RELIEF_SUNKEN);
    XFillRectangle(scalePtr->display, drawable, scalePtr->troughGC,
	    scalePtr->inset + scalePtr->borderWidth,
	    y + scalePtr->borderWidth,
	    (unsigned) (Tk_Width(tkwin) - 2*scalePtr->inset
		- 2*scalePtr->borderWidth),
	    (unsigned) scalePtr->width);
    if (scalePtr->state == STATE_ACTIVE) {
	sliderBorder = scalePtr->activeBorder;
    } else {
	sliderBorder = scalePtr->bgBorder;
    }
    width = scalePtr->sliderLength/2;
    height = scalePtr->width;
    x = TkScaleValueToPixel(scalePtr, scalePtr->value) - width;
    y += scalePtr->borderWidth;
    shadowWidth = scalePtr->borderWidth/2;
    if (shadowWidth == 0) {
	shadowWidth = 1;
    }
    Tk_Draw3DRectangle(tkwin, drawable, sliderBorder,
	    x, y, 2*width, height, shadowWidth, scalePtr->sliderRelief);
    x += shadowWidth;
    y += shadowWidth;
    width -= shadowWidth;
    height -= 2*shadowWidth;
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x, y, width, height,
	    shadowWidth, scalePtr->sliderRelief);
    Tk_Fill3DRectangle(tkwin, drawable, sliderBorder, x+width, y,
	    width, height, shadowWidth, scalePtr->sliderRelief);

    /*
     * Draw the label at the top of the scale.
     */

    if ((scalePtr->flags & REDRAW_OTHER) && (scalePtr->labelLength != 0)) {
	Tk_FontMetrics fm;

	Tk_GetFontMetrics(scalePtr->tkfont, &fm);
	Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
		scalePtr->tkfont, scalePtr->label,
		scalePtr->labelLength, scalePtr->inset + fm.ascent/2,
		scalePtr->horizLabelY + fm.ascent);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayHorizontalValue --
 *
 *	This function is called to display values (scale readings) for
 *	horizontally-oriented scales.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The numerical value corresponding to value is displayed with its
 *	bottom edge at "bottom", and at a horizontal position in the scale
 *	that corresponds to "value".
 *
 *----------------------------------------------------------------------
 */

static void
DisplayHorizontalValue(
    register TkScale *scalePtr,	/* Information about widget in which to
				 * display value. */
    Drawable drawable,		/* Pixmap or window in which to draw the
				 * value. */
    double value,		/* X-coordinate of number to display,
				 * specified in application coords, not in
				 * pixels (we'll compute pixels). */
    int top,			/* Y-coordinate of top edge of text, specified
				 * in pixels. */
    const char *format)		/* Format string to use for the value */
{
    register Tk_Window tkwin = scalePtr->tkwin;
    int x, y, length, width;
    char valueString[TCL_DOUBLE_SPACE];
    Tk_FontMetrics fm;

    x = TkScaleValueToPixel(scalePtr, value);
    Tk_GetFontMetrics(scalePtr->tkfont, &fm);
    y = top + fm.ascent;
    if (snprintf(valueString, TCL_DOUBLE_SPACE, format, value) < 0) {
	valueString[TCL_DOUBLE_SPACE - 1] = '\0';
    }
    length = (int) strlen(valueString);
    width = Tk_TextWidth(scalePtr->tkfont, valueString, length);

    /*
     * Adjust the x-coordinate if necessary to keep the text entirely inside
     * the window.
     */

    x -= width / 2;
    if (x < scalePtr->inset + SPACING) {
	x = scalePtr->inset + SPACING;
    }

    /*
     * Check the right border so use starting point +text width for the check.
     */

    if (x + width >= (Tk_Width(tkwin) - scalePtr->inset)) {
	x = Tk_Width(tkwin) - scalePtr->inset - SPACING - width;
    }
    Tk_DrawChars(scalePtr->display, drawable, scalePtr->textGC,
	    scalePtr->tkfont, valueString, length, x, y);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayScale --
 *
 *	This function is invoked as an idle handler to redisplay the contents
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
    TkScale *scalePtr = (TkScale *) clientData;
    Tk_Window tkwin = scalePtr->tkwin;
    Tcl_Interp *interp = scalePtr->interp;
    Pixmap pixmap;
    int result;
    char string[TCL_DOUBLE_SPACE];
    XRectangle drawnArea;
    Tcl_DString buf;

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
	if (snprintf(string, TCL_DOUBLE_SPACE, scalePtr->valueFormat,
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

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * In order to avoid screen flashes, this function redraws the scale in a
     * pixmap, then copies the pixmap to the screen in a single operation.
     * This means that there's no point in time where the on-sreen image has
     * been cleared.
     */

    pixmap = Tk_GetPixmap(scalePtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
#else
    pixmap = Tk_WindowId(tkwin);
#endif /* TK_NO_DOUBLE_BUFFERING */
    drawnArea.x = 0;
    drawnArea.y = 0;
    drawnArea.width = Tk_Width(tkwin);
    drawnArea.height = Tk_Height(tkwin);

    /*
     * Much of the redisplay is done totally differently for horizontal and
     * vertical scales. Handle the part that's different.
     */

    if (scalePtr->orient == ORIENT_VERTICAL) {
	DisplayVerticalScale(scalePtr, pixmap, &drawnArea);
    } else {
	DisplayHorizontalScale(scalePtr, pixmap, &drawnArea);
    }

    /*
     * Now handle the part of redisplay that is the same for horizontal and
     * vertical scales: border and traversal highlight.
     */

    if (scalePtr->flags & REDRAW_OTHER) {
	if (scalePtr->relief != TK_RELIEF_FLAT) {
	    Tk_Draw3DRectangle(tkwin, pixmap, scalePtr->bgBorder,
		    scalePtr->highlightWidth, scalePtr->highlightWidth,
		    Tk_Width(tkwin) - 2*scalePtr->highlightWidth,
		    Tk_Height(tkwin) - 2*scalePtr->highlightWidth,
		    scalePtr->borderWidth, scalePtr->relief);
	}
	if (scalePtr->highlightWidth != 0) {
	    GC gc;

	    if (scalePtr->flags & GOT_FOCUS) {
		gc = Tk_GCForColor(scalePtr->highlightColorPtr, pixmap);
	    } else {
		gc = Tk_GCForColor(
			Tk_3DBorderColor(scalePtr->highlightBorder), pixmap);
	    }
	    Tk_DrawFocusHighlight(tkwin, gc, scalePtr->highlightWidth, pixmap);
	}
    }

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Copy the information from the off-screen pixmap onto the screen, then
     * delete the pixmap.
     */

    XCopyArea(scalePtr->display, pixmap, Tk_WindowId(tkwin),
	    scalePtr->copyGC, drawnArea.x, drawnArea.y, drawnArea.width,
	    drawnArea.height, drawnArea.x, drawnArea.y);
    Tk_FreePixmap(scalePtr->display, pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */

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
 *	depending on which of the scale's active elements (if any) is under
 *	the point at (x,y).
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
    int sliderFirst;

    if (scalePtr->orient == ORIENT_VERTICAL) {
	if ((x < scalePtr->vertTroughX)
		|| (x >= (scalePtr->vertTroughX + 2*scalePtr->borderWidth +
		scalePtr->width))) {
	    return OTHER;
	}
	if ((y < scalePtr->inset)
		|| (y >= (Tk_Height(scalePtr->tkwin) - scalePtr->inset))) {
	    return OTHER;
	}
	sliderFirst = TkScaleValueToPixel(scalePtr, scalePtr->value)
		- scalePtr->sliderLength/2;
	if (y < sliderFirst) {
	    return TROUGH1;
	}
	if (y < sliderFirst + scalePtr->sliderLength) {
	    return SLIDER;
	}
	return TROUGH2;
    }

    if ((y < scalePtr->horizTroughY)
	    || (y >= (scalePtr->horizTroughY + 2*scalePtr->borderWidth +
	    scalePtr->width))) {
	return OTHER;
    }
    if ((x < scalePtr->inset)
	    || (x >= (Tk_Width(scalePtr->tkwin) - scalePtr->inset))) {
	return OTHER;
    }
    sliderFirst = TkScaleValueToPixel(scalePtr, scalePtr->value)
	    - scalePtr->sliderLength/2;
    if (x < sliderFirst) {
	return TROUGH1;
    }
    if (x < sliderFirst + scalePtr->sliderLength) {
	return SLIDER;
    }
    return TROUGH2;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
