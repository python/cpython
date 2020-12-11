/*
 * tkUnixScrollbar.c --
 *
 *	This file implements the Unix specific portion of the scrollbar
 *	widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkScrollbar.h"

/*
 * Minimum slider length, in pixels (designed to make sure that the slider is
 * always easy to grab with the mouse).
 */

#define MIN_SLIDER_LENGTH	5

/*
 * Declaration of Unix specific scrollbar structure.
 */

typedef struct UnixScrollbar {
    TkScrollbar info;		/* Generic scrollbar info. */
    GC troughGC;		/* For drawing trough. */
    GC copyGC;			/* Used for copying from pixmap onto screen. */
} UnixScrollbar;

/*
 * The class procedure table for the scrollbar widget. All fields except size
 * are left initialized to NULL, which should happen automatically since the
 * variable is declared at this scope.
 */

const Tk_ClassProcs tkpScrollbarProcs = {
    sizeof(Tk_ClassProcs),	/* size */
    NULL,					/* worldChangedProc */
    NULL,					/* createProc */
    NULL					/* modalProc */
};

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateScrollbar --
 *
 *	Allocate a new TkScrollbar structure.
 *
 * Results:
 *	Returns a newly allocated TkScrollbar structure.
 *
 * Side effects:
 *	Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkScrollbar *
TkpCreateScrollbar(
    Tk_Window tkwin)
{
    UnixScrollbar *scrollPtr = ckalloc(sizeof(UnixScrollbar));

    scrollPtr->troughGC = NULL;
    scrollPtr->copyGC = NULL;

    Tk_CreateEventHandler(tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    TkScrollbarEventProc, scrollPtr);

    return (TkScrollbar *) scrollPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkpDisplayScrollbar --
 *
 *	This procedure redraws the contents of a scrollbar window. It is
 *	invoked as a do-when-idle handler, so it only runs when there's
 *	nothing else for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

void
TkpDisplayScrollbar(
    ClientData clientData)	/* Information about window. */
{
    register TkScrollbar *scrollPtr = (TkScrollbar *) clientData;
    register Tk_Window tkwin = scrollPtr->tkwin;
    XPoint points[7];
    Tk_3DBorder border;
    int relief, width, elementBorderWidth;
    Pixmap pixmap;

    if ((scrollPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	goto done;
    }

    if (scrollPtr->vertical) {
	width = Tk_Width(tkwin) - 2*scrollPtr->inset;
    } else {
	width = Tk_Height(tkwin) - 2*scrollPtr->inset;
    }
    elementBorderWidth = scrollPtr->elementBorderWidth;
    if (elementBorderWidth < 0) {
	elementBorderWidth = scrollPtr->borderWidth;
    }

    /*
     * In order to avoid screen flashes, this procedure redraws the scrollbar
     * in a pixmap, then copies the pixmap to the screen in a single
     * operation. This means that there's no point in time where the on-sreen
     * image has been cleared.
     */

    pixmap = Tk_GetPixmap(scrollPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));

    if (scrollPtr->highlightWidth != 0) {
	GC gc;

	if (scrollPtr->flags & GOT_FOCUS) {
	    gc = Tk_GCForColor(scrollPtr->highlightColorPtr, pixmap);
	} else {
	    gc = Tk_GCForColor(scrollPtr->highlightBgColorPtr, pixmap);
	}
	Tk_DrawFocusHighlight(tkwin, gc, scrollPtr->highlightWidth, pixmap);
    }
    Tk_Draw3DRectangle(tkwin, pixmap, scrollPtr->bgBorder,
	    scrollPtr->highlightWidth, scrollPtr->highlightWidth,
	    Tk_Width(tkwin) - 2*scrollPtr->highlightWidth,
	    Tk_Height(tkwin) - 2*scrollPtr->highlightWidth,
	    scrollPtr->borderWidth, scrollPtr->relief);
    XFillRectangle(scrollPtr->display, pixmap,
	    ((UnixScrollbar*)scrollPtr)->troughGC,
	    scrollPtr->inset, scrollPtr->inset,
	    (unsigned) (Tk_Width(tkwin) - 2*scrollPtr->inset),
	    (unsigned) (Tk_Height(tkwin) - 2*scrollPtr->inset));

    /*
     * Draw the top or left arrow. The coordinates of the polygon points
     * probably seem odd, but they were carefully chosen with respect to X's
     * rules for filling polygons. These point choices cause the arrows to
     * just fill the narrow dimension of the scrollbar and be properly
     * centered.
     */

    if (scrollPtr->activeField == TOP_ARROW) {
	border = scrollPtr->activeBorder;
	relief = scrollPtr->activeField == TOP_ARROW ? scrollPtr->activeRelief
		: TK_RELIEF_RAISED;
    } else {
	border = scrollPtr->bgBorder;
	relief = TK_RELIEF_RAISED;
    }
    if (scrollPtr->vertical) {
	points[0].x = scrollPtr->inset - 1;
	points[0].y = scrollPtr->arrowLength + scrollPtr->inset - 1;
	points[1].x = width + scrollPtr->inset;
	points[1].y = points[0].y;
	points[2].x = width/2 + scrollPtr->inset;
	points[2].y = scrollPtr->inset - 1;
	Tk_Fill3DPolygon(tkwin, pixmap, border, points, 3,
		elementBorderWidth, relief);
    } else {
	points[0].x = scrollPtr->arrowLength + scrollPtr->inset - 1;
	points[0].y = scrollPtr->inset - 1;
	points[1].x = scrollPtr->inset;
	points[1].y = width/2 + scrollPtr->inset;
	points[2].x = points[0].x;
	points[2].y = width + scrollPtr->inset;
	Tk_Fill3DPolygon(tkwin, pixmap, border, points, 3,
		elementBorderWidth, relief);
    }

    /*
     * Display the bottom or right arrow.
     */

    if (scrollPtr->activeField == BOTTOM_ARROW) {
	border = scrollPtr->activeBorder;
	relief = scrollPtr->activeField == BOTTOM_ARROW
		? scrollPtr->activeRelief : TK_RELIEF_RAISED;
    } else {
	border = scrollPtr->bgBorder;
	relief = TK_RELIEF_RAISED;
    }
    if (scrollPtr->vertical) {
	points[0].x = scrollPtr->inset;
	points[0].y = Tk_Height(tkwin) - scrollPtr->arrowLength
		- scrollPtr->inset + 1;
	points[1].x = width/2 + scrollPtr->inset;
	points[1].y = Tk_Height(tkwin) - scrollPtr->inset;
	points[2].x = width + scrollPtr->inset;
	points[2].y = points[0].y;
	Tk_Fill3DPolygon(tkwin, pixmap, border,
		points, 3, elementBorderWidth, relief);
    } else {
	points[0].x = Tk_Width(tkwin) - scrollPtr->arrowLength
		- scrollPtr->inset + 1;
	points[0].y = scrollPtr->inset - 1;
	points[1].x = points[0].x;
	points[1].y = width + scrollPtr->inset;
	points[2].x = Tk_Width(tkwin) - scrollPtr->inset;
	points[2].y = width/2 + scrollPtr->inset;
	Tk_Fill3DPolygon(tkwin, pixmap, border,
		points, 3, elementBorderWidth, relief);
    }

    /*
     * Display the slider.
     */

    if (scrollPtr->activeField == SLIDER) {
	border = scrollPtr->activeBorder;
	relief = scrollPtr->activeField == SLIDER ? scrollPtr->activeRelief
		: TK_RELIEF_RAISED;
    } else {
	border = scrollPtr->bgBorder;
	relief = TK_RELIEF_RAISED;
    }
    if (scrollPtr->vertical) {
	Tk_Fill3DRectangle(tkwin, pixmap, border,
		scrollPtr->inset, scrollPtr->sliderFirst,
		width, scrollPtr->sliderLast - scrollPtr->sliderFirst,
		elementBorderWidth, relief);
    } else {
	Tk_Fill3DRectangle(tkwin, pixmap, border,
		scrollPtr->sliderFirst, scrollPtr->inset,
		scrollPtr->sliderLast - scrollPtr->sliderFirst, width,
		elementBorderWidth, relief);
    }

    /*
     * Copy the information from the off-screen pixmap onto the screen, then
     * delete the pixmap.
     */

    XCopyArea(scrollPtr->display, pixmap, Tk_WindowId(tkwin),
	    ((UnixScrollbar*)scrollPtr)->copyGC, 0, 0,
	    (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(scrollPtr->display, pixmap);

  done:
    scrollPtr->flags &= ~REDRAW_PENDING;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeScrollbarGeometry --
 *
 *	After changes in a scrollbar's size or configuration, this procedure
 *	recomputes various geometry information used in displaying the
 *	scrollbar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The scrollbar will be displayed differently.
 *
 *----------------------------------------------------------------------
 */

extern void
TkpComputeScrollbarGeometry(
    register TkScrollbar *scrollPtr)
				/* Scrollbar whose geometry may have
				 * changed. */
{
    int width, fieldLength;

    if (scrollPtr->highlightWidth < 0) {
	scrollPtr->highlightWidth = 0;
    }
    scrollPtr->inset = scrollPtr->highlightWidth + scrollPtr->borderWidth;
    width = (scrollPtr->vertical) ? Tk_Width(scrollPtr->tkwin)
	    : Tk_Height(scrollPtr->tkwin);

    /*
     * Next line assumes that the arrow area is a square.
     */

    scrollPtr->arrowLength = width - 2*scrollPtr->inset + 1;
    fieldLength = (scrollPtr->vertical ? Tk_Height(scrollPtr->tkwin)
	    : Tk_Width(scrollPtr->tkwin))
	    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
    if (fieldLength < 0) {
	fieldLength = 0;
    }
    scrollPtr->sliderFirst = fieldLength*scrollPtr->firstFraction;
    scrollPtr->sliderLast = fieldLength*scrollPtr->lastFraction;

    /*
     * Adjust the slider so that some piece of it is always displayed in the
     * scrollbar and so that it has at least a minimal width (so it can be
     * grabbed with the mouse).
     */

    if (scrollPtr->sliderFirst > fieldLength - MIN_SLIDER_LENGTH) {
	scrollPtr->sliderFirst = fieldLength - MIN_SLIDER_LENGTH;
    }
    if (scrollPtr->sliderFirst < 0) {
	scrollPtr->sliderFirst = 0;
    }
    if (scrollPtr->sliderLast < scrollPtr->sliderFirst + MIN_SLIDER_LENGTH) {
	scrollPtr->sliderLast = scrollPtr->sliderFirst + MIN_SLIDER_LENGTH;
    }
    if (scrollPtr->sliderLast > fieldLength) {
	scrollPtr->sliderLast = fieldLength;
    }
    scrollPtr->sliderFirst += scrollPtr->arrowLength + scrollPtr->inset;
    scrollPtr->sliderLast += scrollPtr->arrowLength + scrollPtr->inset;

    /*
     * Register the desired geometry for the window (leave enough space for
     * the two arrows plus a minimum-size slider, plus border around the whole
     * window, if any). Then arrange for the window to be redisplayed.
     */

    if (scrollPtr->vertical) {
	Tk_GeometryRequest(scrollPtr->tkwin,
		scrollPtr->width + 2*scrollPtr->inset,
		2*(scrollPtr->arrowLength + scrollPtr->borderWidth
		+ scrollPtr->inset));
    } else {
	Tk_GeometryRequest(scrollPtr->tkwin,
		2*(scrollPtr->arrowLength + scrollPtr->borderWidth
		+ scrollPtr->inset), scrollPtr->width + 2*scrollPtr->inset);
    }
    Tk_SetInternalBorder(scrollPtr->tkwin, scrollPtr->inset);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyScrollbar --
 *
 *	Free data structures associated with the scrollbar control.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the GCs associated with the scrollbar.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyScrollbar(
    TkScrollbar *scrollPtr)
{
    UnixScrollbar *unixScrollPtr = (UnixScrollbar *)scrollPtr;

    if (unixScrollPtr->troughGC != NULL) {
	Tk_FreeGC(scrollPtr->display, unixScrollPtr->troughGC);
    }
    if (unixScrollPtr->copyGC != NULL) {
	Tk_FreeGC(scrollPtr->display, unixScrollPtr->copyGC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpConfigureScrollbar --
 *
 *	This procedure is called after the generic code has finished
 *	processing configuration options, in order to configure platform
 *	specific options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Configuration info may get changed.
 *
 *----------------------------------------------------------------------
 */

void
TkpConfigureScrollbar(
    register TkScrollbar *scrollPtr)
				/* Information about widget; may or may not
				 * already have values for some fields. */
{
    XGCValues gcValues;
    GC new;
    UnixScrollbar *unixScrollPtr = (UnixScrollbar *) scrollPtr;

    Tk_SetBackgroundFromBorder(scrollPtr->tkwin, scrollPtr->bgBorder);

    gcValues.foreground = scrollPtr->troughColorPtr->pixel;
    new = Tk_GetGC(scrollPtr->tkwin, GCForeground, &gcValues);
    if (unixScrollPtr->troughGC != NULL) {
	Tk_FreeGC(scrollPtr->display, unixScrollPtr->troughGC);
    }
    unixScrollPtr->troughGC = new;
    if (unixScrollPtr->copyGC == NULL) {
	gcValues.graphics_exposures = False;
	unixScrollPtr->copyGC = Tk_GetGC(scrollPtr->tkwin,
		GCGraphicsExposures, &gcValues);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpScrollbarPosition --
 *
 *	Determine the scrollbar element corresponding to a given position.
 *
 * Results:
 *	One of TOP_ARROW, TOP_GAP, etc., indicating which element of the
 *	scrollbar covers the position given by (x, y). If (x,y) is outside the
 *	scrollbar entirely, then OUTSIDE is returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkpScrollbarPosition(
    register TkScrollbar *scrollPtr,
				/* Scrollbar widget record. */
    int x, int y)		/* Coordinates within scrollPtr's window. */
{
    int length, width, tmp;
    register const int inset = scrollPtr->inset;

    if (scrollPtr->vertical) {
	length = Tk_Height(scrollPtr->tkwin);
	width = Tk_Width(scrollPtr->tkwin);
    } else {
	tmp = x;
	x = y;
	y = tmp;
	length = Tk_Width(scrollPtr->tkwin);
	width = Tk_Height(scrollPtr->tkwin);
    }

    if (x<inset || x>=width-inset || y<inset || y>=length-inset) {
	return OUTSIDE;
    }

    /*
     * All of the calculations in this procedure mirror those in
     * TkpDisplayScrollbar. Be sure to keep the two consistent.
     */

    if (y < inset + scrollPtr->arrowLength) {
	return TOP_ARROW;
    }
    if (y < scrollPtr->sliderFirst) {
	return TOP_GAP;
    }
    if (y < scrollPtr->sliderLast) {
	return SLIDER;
    }
    if (y >= length - (scrollPtr->arrowLength + inset)) {
	return BOTTOM_ARROW;
    }
    return BOTTOM_GAP;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
