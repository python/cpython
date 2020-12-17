/*
 * tkUnixDraw.c --
 *
 *	This file contains X specific drawing routines.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

#ifndef _WIN32
#include "tkUnixInt.h"
#endif

/*
 * The following structure is used to pass information to ScrollRestrictProc
 * from TkScrollWindow.
 */

typedef struct ScrollInfo {
    int done;			/* Flag is 0 until filtering is done. */
    Display *display;		/* Display to filter. */
    Window window;		/* Window to filter. */
    TkRegion region;		/* Region into which damage is accumulated. */
    int dx, dy;			/* Amount by which window was shifted. */
} ScrollInfo;

/*
 * Forward declarations for functions declared later in this file:
 */

static Tk_RestrictProc ScrollRestrictProc;

/*
 *----------------------------------------------------------------------
 *
 * TkScrollWindow --
 *
 *	Scroll a rectangle of the specified window and accumulate damage
 *	information in the specified Region.
 *
 * Results:
 *	Returns 0 if no damage additional damage was generated. Sets damageRgn
 *	to contain the damaged areas and returns 1 if GraphicsExpose events
 *	were detected.
 *
 * Side effects:
 *	Scrolls the bits in the window and enters the event loop looking for
 *	damage events.
 *
 *----------------------------------------------------------------------
 */

int
TkScrollWindow(
    Tk_Window tkwin,		/* The window to be scrolled. */
    GC gc,			/* GC for window to be scrolled. */
    int x, int y, int width, int height,
				/* Position rectangle to be scrolled. */
    int dx, int dy,		/* Distance rectangle should be moved. */
    TkRegion damageRgn)		/* Region to accumulate damage in. */
{
    Tk_RestrictProc *prevProc;
    ClientData prevArg;
    ScrollInfo info;

    XCopyArea(Tk_Display(tkwin), Tk_WindowId(tkwin), Tk_WindowId(tkwin), gc,
	    x, y, (unsigned) width, (unsigned) height, x+dx, y+dy);

    info.done = 0;
    info.window = Tk_WindowId(tkwin);
    info.display = Tk_Display(tkwin);
    info.region = damageRgn;
    info.dx = dx;
    info.dy = dy;

    /*
     * Sync the event stream so all of the expose events will be on the Tk
     * event queue before we start filtering. This avoids busy waiting while
     * we filter events.
     */

    TkpSync(info.display);
    prevProc = Tk_RestrictEvents(ScrollRestrictProc, &info, &prevArg);
    while (!info.done) {
	Tcl_ServiceEvent(TCL_WINDOW_EVENTS);
    }
    Tk_RestrictEvents(prevProc, prevArg, &prevArg);

    if (XEmptyRegion((Region) damageRgn)) {
	return 0;
    } else {
	return 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ScrollRestrictProc --
 *
 *	A Tk_RestrictProc used by TkScrollWindow to gather up Expose
 *	information into a single damage region. It accumulates damage events
 *	on the specified window until a NoExpose or the last GraphicsExpose
 *	event is detected.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Discards Expose events after accumulating damage information
 *	for a particular window.
 *
 *----------------------------------------------------------------------
 */

static Tk_RestrictAction
ScrollRestrictProc(
    ClientData arg,
    XEvent *eventPtr)
{
    ScrollInfo *info = (ScrollInfo *) arg;
    XRectangle rect;

    /*
     * Defer events which aren't for the specified window.
     */

    if (info->done || (eventPtr->xany.display != info->display)
	    || (eventPtr->xany.window != info->window)) {
	return TK_DEFER_EVENT;
    }

    if (eventPtr->type == NoExpose) {
	info->done = 1;
    } else if (eventPtr->type == GraphicsExpose) {
	rect.x = eventPtr->xgraphicsexpose.x;
	rect.y = eventPtr->xgraphicsexpose.y;
	rect.width = eventPtr->xgraphicsexpose.width;
	rect.height = eventPtr->xgraphicsexpose.height;
	XUnionRectWithRegion(&rect, (Region) info->region,
		(Region) info->region);

	if (eventPtr->xgraphicsexpose.count == 0) {
	    info->done = 1;
	}
    } else if (eventPtr->type == Expose) {
	/*
	 * This case is tricky. This event was already queued before the
	 * XCopyArea was issued. If this area overlaps the area being copied,
	 * then some of the copied area may be invalid. The easiest way to
	 * handle this case is to mark both the original area and the shifted
	 * area as damaged.
	 */

	rect.x = eventPtr->xexpose.x;
	rect.y = eventPtr->xexpose.y;
	rect.width = eventPtr->xexpose.width;
	rect.height = eventPtr->xexpose.height;
	XUnionRectWithRegion(&rect, (Region) info->region,
		(Region) info->region);
	rect.x += info->dx;
	rect.y += info->dy;
	XUnionRectWithRegion(&rect, (Region) info->region,
		(Region) info->region);
    } else {
	return TK_DEFER_EVENT;
    }
    return TK_DISCARD_EVENT;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDrawHighlightBorder --
 *
 *	This function draws a rectangular ring around the outside of a widget
 *	to indicate that it has received the input focus.
 *
 *      On Unix, we just draw the simple inset ring. On other sytems, e.g. the
 *      Mac, the focus ring is a little more complicated, so we need this
 *      abstraction.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A rectangle "width" pixels wide is drawn in "drawable", corresponding
 *	to the outer area of "tkwin".
 *
 *----------------------------------------------------------------------
 */

void
TkpDrawHighlightBorder(
    Tk_Window tkwin,
    GC fgGC,
    GC bgGC,
    int highlightWidth,
    Drawable drawable)
{
    TkDrawInsetFocusHighlight(tkwin, fgGC, highlightWidth, drawable, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDrawFrame --
 *
 *	This function draws the rectangular frame area.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws inside the tkwin area.
 *
 *----------------------------------------------------------------------
 */

void
TkpDrawFrame(
    Tk_Window tkwin,
    Tk_3DBorder border,
    int highlightWidth,
    int borderWidth,
    int relief)
{
    Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), border, highlightWidth,
	    highlightWidth, Tk_Width(tkwin) - 2*highlightWidth,
	    Tk_Height(tkwin) - 2*highlightWidth, borderWidth, relief);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
