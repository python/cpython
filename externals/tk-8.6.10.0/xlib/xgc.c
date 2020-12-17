/*
 * xgc.c --
 *
 *	This file contains generic routines for manipulating X graphics
 *	contexts.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * Copyright (c) 2002-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2008-2009, Apple Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

#if !defined(MAC_OSX_TK)
#   include <X11/Xlib.h>
#   define gcCacheSize 0
#   define TkpInitGCCache(gc)
#   define TkpFreeGCCache(gc)
#   define TkpGetGCCache(gc)
#else
#   include <tkMacOSXInt.h>
#   include <X11/Xlib.h>
#   include <X11/X.h>
#   define Cursor XCursor
#   define Region XRegion
#   define gcCacheSize sizeof(TkpGCCache)
#endif

#undef TkSetRegion

/*
 *----------------------------------------------------------------------
 *
 * AllocClipMask --
 *
 *	Static helper proc to allocate new or clear existing TkpClipMask.
 *
 * Results:
 *	Returns ptr to the new/cleared TkpClipMask.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TkpClipMask *AllocClipMask(GC gc) {
    TkpClipMask *clip_mask = (TkpClipMask*) gc->clip_mask;

    if (clip_mask == NULL) {
	clip_mask = ckalloc(sizeof(TkpClipMask));
	gc->clip_mask = (Pixmap) clip_mask;
#ifdef MAC_OSX_TK
    } else if (clip_mask->type == TKP_CLIP_REGION) {
	TkpReleaseRegion(clip_mask->value.region);
#endif
    }
    return clip_mask;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeClipMask --
 *
 *	Static helper proc to free TkpClipMask.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void FreeClipMask(GC gc) {
    if (gc->clip_mask != None) {
#ifdef MAC_OSX_TK
	if (((TkpClipMask*) gc->clip_mask)->type == TKP_CLIP_REGION) {
	    TkpReleaseRegion(((TkpClipMask*) gc->clip_mask)->value.region);
	}
#endif
	ckfree(gc->clip_mask);
	gc->clip_mask = None;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XCreateGC --
 *
 *	Allocate a new GC, and initialize the specified fields.
 *
 * Results:
 *	Returns a newly allocated GC.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

GC
XCreateGC(
    Display *display,
    Drawable d,
    unsigned long mask,
    XGCValues *values)
{
    GC gp;

    /*
     * In order to have room for a dash list, MAX_DASH_LIST_SIZE extra chars
     * are defined, which is invisible from the outside. The list is assumed
     * to end with a 0-char, so this must be set explicitly during
     * initialization.
     */

#define MAX_DASH_LIST_SIZE 10

    gp = ckalloc(sizeof(XGCValues) + MAX_DASH_LIST_SIZE + gcCacheSize);
    if (!gp) {
	return NULL;
    }

#define InitField(name,maskbit,default) \
	(gp->name = (mask & (maskbit)) ? values->name : (default))

    InitField(function,		  GCFunction,		GXcopy);
    InitField(plane_mask,	  GCPlaneMask,		(unsigned long)(~0));
    InitField(foreground,	  GCForeground,
	    BlackPixelOfScreen(DefaultScreenOfDisplay(display)));
    InitField(background,	  GCBackground,
	    WhitePixelOfScreen(DefaultScreenOfDisplay(display)));
    InitField(line_width,	  GCLineWidth,		1);
    InitField(line_style,	  GCLineStyle,		LineSolid);
    InitField(cap_style,	  GCCapStyle,		0);
    InitField(join_style,	  GCJoinStyle,		0);
    InitField(fill_style,	  GCFillStyle,		FillSolid);
    InitField(fill_rule,	  GCFillRule,		WindingRule);
    InitField(arc_mode,		  GCArcMode,		ArcPieSlice);
    InitField(tile,		  GCTile,		None);
    InitField(stipple,		  GCStipple,		None);
    InitField(ts_x_origin,	  GCTileStipXOrigin,	0);
    InitField(ts_y_origin,	  GCTileStipYOrigin,	0);
    InitField(font,		  GCFont,		None);
    InitField(subwindow_mode,	  GCSubwindowMode,	ClipByChildren);
    InitField(graphics_exposures, GCGraphicsExposures,	True);
    InitField(clip_x_origin,	  GCClipXOrigin,	0);
    InitField(clip_y_origin,	  GCClipYOrigin,	0);
    InitField(dash_offset,	  GCDashOffset,		0);
    InitField(dashes,		  GCDashList,		4);
    (&(gp->dashes))[1] = 0;

    gp->clip_mask = None;
    if (mask & GCClipMask) {
	TkpClipMask *clip_mask = AllocClipMask(gp);

	clip_mask->type = TKP_CLIP_PIXMAP;
	clip_mask->value.pixmap = values->clip_mask;
    }
    TkpInitGCCache(gp);

    return gp;
}

#ifdef MAC_OSX_TK
/*
 *----------------------------------------------------------------------
 *
 * TkpGetGCCache --
 *
 * Results:
 *	Pointer to the TkpGCCache at the end of the GC.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkpGCCache*
TkpGetGCCache(GC gc) {
    return (gc ? (TkpGCCache*)(((char*) gc) + sizeof(XGCValues) +
	    MAX_DASH_LIST_SIZE) : NULL);
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * XChangeGC --
 *
 *	Changes the GC components specified by valuemask for the specified GC.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the specified GC.
 *
 *----------------------------------------------------------------------
 */

int
XChangeGC(
    Display *d,
    GC gc,
    unsigned long mask,
    XGCValues *values)
{
#define ModifyField(name,maskbit) \
	if (mask & (maskbit)) { gc->name = values->name; }

    ModifyField(function, GCFunction);
    ModifyField(plane_mask, GCPlaneMask);
    ModifyField(foreground, GCForeground);
    ModifyField(background, GCBackground);
    ModifyField(line_width, GCLineWidth);
    ModifyField(line_style, GCLineStyle);
    ModifyField(cap_style, GCCapStyle);
    ModifyField(join_style, GCJoinStyle);
    ModifyField(fill_style, GCFillStyle);
    ModifyField(fill_rule, GCFillRule);
    ModifyField(arc_mode, GCArcMode);
    ModifyField(tile, GCTile);
    ModifyField(stipple, GCStipple);
    ModifyField(ts_x_origin, GCTileStipXOrigin);
    ModifyField(ts_y_origin, GCTileStipYOrigin);
    ModifyField(font, GCFont);
    ModifyField(subwindow_mode, GCSubwindowMode);
    ModifyField(graphics_exposures, GCGraphicsExposures);
    ModifyField(clip_x_origin, GCClipXOrigin);
    ModifyField(clip_y_origin, GCClipYOrigin);
    ModifyField(dash_offset, GCDashOffset);
    if (mask & GCClipMask) {
	XSetClipMask(d, gc, values->clip_mask);
    }
    if (mask & GCDashList) {
	gc->dashes = values->dashes;
	(&(gc->dashes))[1] = 0;
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XFreeGC --
 *
 *	Deallocates the specified graphics context.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int XFreeGC(
    Display *d,
    GC gc)
{
    if (gc != NULL) {
	FreeClipMask(gc);
	TkpFreeGCCache(gc);
	ckfree(gc);
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XSetForeground, etc. --
 *
 *	The following functions are simply accessor functions for the GC
 *	slots.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Each function sets some slot in the GC.
 *
 *----------------------------------------------------------------------
 */

int
XSetForeground(
    Display *display,
    GC gc,
    unsigned long foreground)
{
    gc->foreground = foreground;
    return Success;
}

int
XSetBackground(
    Display *display,
    GC gc,
    unsigned long background)
{
    gc->background = background;
    return Success;
}

int
XSetDashes(
    Display *display,
    GC gc,
    int dash_offset,
    _Xconst char *dash_list,
    int n)
{
    char *p = &(gc->dashes);

#ifdef TkWinDeleteBrush
    TkWinDeleteBrush(gc->fgBrush);
    TkWinDeletePen(gc->fgPen);
    TkWinDeleteBrush(gc->bgBrush);
    TkWinDeletePen(gc->fgExtPen);
#endif
    gc->dash_offset = dash_offset;
    if (n > MAX_DASH_LIST_SIZE) n = MAX_DASH_LIST_SIZE;
    while (n-- > 0) {
	*p++ = *dash_list++;
    }
    *p = 0;
    return Success;
}

int
XSetFunction(
    Display *display,
    GC gc,
    int function)
{
    gc->function = function;
    return Success;
}

int
XSetFillRule(
    Display *display,
    GC gc,
    int fill_rule)
{
    gc->fill_rule = fill_rule;
    return Success;
}

int
XSetFillStyle(
    Display *display,
    GC gc,
    int fill_style)
{
    gc->fill_style = fill_style;
    return Success;
}

int
XSetTSOrigin(
    Display *display,
    GC gc,
    int x, int y)
{
    gc->ts_x_origin = x;
    gc->ts_y_origin = y;
    return Success;
}

int
XSetFont(
    Display *display,
    GC gc,
    Font font)
{
    gc->font = font;
    return Success;
}

int
XSetArcMode(
    Display *display,
    GC gc,
    int arc_mode)
{
    gc->arc_mode = arc_mode;
    return Success;
}

int
XSetStipple(
    Display *display,
    GC gc,
    Pixmap stipple)
{
    gc->stipple = stipple;
    return Success;
}

int
XSetLineAttributes(
    Display *display,
    GC gc,
    unsigned int line_width,
    int line_style,
    int cap_style,
    int join_style)
{
    gc->line_width = line_width;
    gc->line_style = line_style;
    gc->cap_style = cap_style;
    gc->join_style = join_style;
    return Success;
}

int
XSetClipOrigin(
    Display *display,
    GC gc,
    int clip_x_origin,
    int clip_y_origin)
{
    gc->clip_x_origin = clip_x_origin;
    gc->clip_y_origin = clip_y_origin;
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetRegion, XSetClipMask --
 *
 *	Sets the clipping region/pixmap for a GC.
 *
 *	Note that unlike the Xlib equivalent, it is not safe to delete the
 *	region after setting it into the GC (except on Mac OS X). The only
 *	uses of TkSetRegion are currently in DisplayFrame and in
 *	ImgPhotoDisplay, which use the GC immediately.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates or deallocates a TkpClipMask.
 *
 *----------------------------------------------------------------------
 */

int
TkSetRegion(
    Display *display,
    GC gc,
    TkRegion r)
{
    if (r == NULL) {
	Tcl_Panic("must not pass NULL to TkSetRegion for compatibility with X11; use XSetClipMask instead");
    } else {
	TkpClipMask *clip_mask = AllocClipMask(gc);

	clip_mask->type = TKP_CLIP_REGION;
	clip_mask->value.region = r;
#ifdef MAC_OSX_TK
	TkpRetainRegion(r);
#endif
    }
    return Success;
}

int
XSetClipMask(
    Display *display,
    GC gc,
    Pixmap pixmap)
{
    if (pixmap == None) {
	FreeClipMask(gc);
    } else {
	TkpClipMask *clip_mask = AllocClipMask(gc);

	clip_mask->type = TKP_CLIP_PIXMAP;
	clip_mask->value.pixmap = pixmap;
    }
    return Success;
}

/*
 * Some additional dummy functions (hopefully implemented soon).
 */

#if 0
Cursor
XCreateFontCursor(
    Display *display,
    unsigned int shape)
{
    return (Cursor) 0;
}

void
XDrawImageString(
    Display *display,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length)
{
}
#endif

int
XDrawPoint(
    Display *display,
    Drawable d,
    GC gc,
    int x,
    int y)
{
    return XDrawLine(display, d, gc, x, y, x, y);
}

int
XDrawPoints(
    Display *display,
    Drawable d,
    GC gc,
    XPoint *points,
    int npoints,
    int mode)
{
    int res = Success;

    while (npoints-- > 0) {
	res = XDrawLine(display, d, gc,
		points[0].x, points[0].y, points[0].x, points[0].y);
	if (res != Success) break;
	++points;
    }
    return res;
}

#if !defined(MAC_OSX_TK)
int
XDrawSegments(
    Display *display,
    Drawable d,
    GC gc,
    XSegment *segments,
    int nsegments)
{
    return BadDrawable;
}
#endif

#if 0
char *
XFetchBuffer(
    Display *display,
    int *nbytes_return,
    int buffer)
{
    return (char *) 0;
}

Status
XFetchName(
    Display *display,
    Window w,
    char **window_name_return)
{
    return (Status) 0;
}

Atom *
XListProperties(
    Display* display,
    Window w,
    int *num_prop_return)
{
    return (Atom *) 0;
}

void
XMapRaised(
    Display *display,
    Window w)
{
}

void
XPutImage(
    Display *display,
    Drawable d,
    GC gc,
    XImage *image,
    int src_x,
    int src_y,
    int dest_x,
    int dest_y,
    unsigned int width,
    unsigned int height)
{
}

void
XQueryTextExtents(
    Display *display,
    XID font_ID,
    _Xconst char *string,
    int nchars,
    int *direction_return,
    int *font_ascent_return,
    int *font_descent_return,
    XCharStruct *overall_return)
{
}

void
XReparentWindow(
    Display *display,
    Window w,
    Window parent,
    int x,
    int y)
{
}

void
XRotateBuffers(
    Display *display,
    int rotate)
{
}

void
XStoreBuffer(
    Display *display,
    _Xconst char *bytes,
    int nbytes,
    int buffer)
{
}

void
XUndefineCursor(
    Display *display,
    Window w)
{
}
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
