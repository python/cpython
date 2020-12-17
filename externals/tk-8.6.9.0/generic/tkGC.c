/*
 * tkGC.c --
 *
 *	This file maintains a database of read-only graphics contexts for the
 *	Tk toolkit, in order to allow GC's to be shared.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * One of the following data structures exists for each GC that is currently
 * active. The structure is indexed with two hash tables, one based on the
 * values in the graphics context and the other based on the display and GC
 * identifier.
 */

typedef struct {
    GC gc;			/* Graphics context. */
    Display *display;		/* Display to which gc belongs. */
    int refCount;		/* Number of active uses of gc. */
    Tcl_HashEntry *valueHashPtr;/* Entry in valueTable (needed when deleting
				 * this structure). */
} TkGC;

typedef struct {
    XGCValues values;		/* Desired values for GC. */
    Display *display;		/* Display for which GC is valid. */
    int screenNum;		/* screen number of display */
    int depth;			/* and depth for which GC is valid. */
} ValueKey;

/*
 * Forward declarations for functions defined in this file:
 */

static void		GCInit(TkDisplay *dispPtr);

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetGC --
 *
 *	Given a desired set of values for a graphics context, find a read-only
 *	graphics context with the desired values.
 *
 * Results:
 *	The return value is the X identifer for the desired graphics context.
 *	The caller should never modify this GC, and should call Tk_FreeGC when
 *	the GC is no longer needed.
 *
 * Side effects:
 *	The GC is added to an internal database with a reference count. For
 *	each call to this function, there should eventually be a call to
 *	Tk_FreeGC, so that the database can be cleaned up when GC's aren't
 *	needed anymore.
 *
 *----------------------------------------------------------------------
 */

GC
Tk_GetGC(
    Tk_Window tkwin,		/* Window in which GC will be used. */
    register unsigned long valueMask,
				/* 1 bits correspond to values specified in
				 * *valuesPtr; other values are set from
				 * defaults. */
    register XGCValues *valuePtr)
				/* Values are specified here for bits set in
				 * valueMask. */
{
    ValueKey valueKey;
    Tcl_HashEntry *valueHashPtr, *idHashPtr;
    register TkGC *gcPtr;
    int isNew;
    Drawable d, freeDrawable;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (dispPtr->gcInit <= 0) {
	GCInit(dispPtr);
    }

    /*
     * Must zero valueKey at start to clear out pad bytes that may be part of
     * structure on some systems.
     */

    memset(&valueKey, 0, sizeof(valueKey));

    /*
     * First, check to see if there's already a GC that will work for this
     * request (exact matches only, sorry).
     */

    if (valueMask & GCFunction) {
	valueKey.values.function = valuePtr->function;
    } else {
	valueKey.values.function = GXcopy;
    }
    if (valueMask & GCPlaneMask) {
	valueKey.values.plane_mask = valuePtr->plane_mask;
    } else {
	valueKey.values.plane_mask = (unsigned) ~0;
    }
    if (valueMask & GCForeground) {
	valueKey.values.foreground = valuePtr->foreground;
    } else {
	valueKey.values.foreground = 0;
    }
    if (valueMask & GCBackground) {
	valueKey.values.background = valuePtr->background;
    } else {
	valueKey.values.background = 1;
    }
    if (valueMask & GCLineWidth) {
	valueKey.values.line_width = valuePtr->line_width;
    } else {
	valueKey.values.line_width = 0;
    }
    if (valueMask & GCLineStyle) {
	valueKey.values.line_style = valuePtr->line_style;
    } else {
	valueKey.values.line_style = LineSolid;
    }
    if (valueMask & GCCapStyle) {
	valueKey.values.cap_style = valuePtr->cap_style;
    } else {
	valueKey.values.cap_style = CapButt;
    }
    if (valueMask & GCJoinStyle) {
	valueKey.values.join_style = valuePtr->join_style;
    } else {
	valueKey.values.join_style = JoinMiter;
    }
    if (valueMask & GCFillStyle) {
	valueKey.values.fill_style = valuePtr->fill_style;
    } else {
	valueKey.values.fill_style = FillSolid;
    }
    if (valueMask & GCFillRule) {
	valueKey.values.fill_rule = valuePtr->fill_rule;
    } else {
	valueKey.values.fill_rule = EvenOddRule;
    }
    if (valueMask & GCArcMode) {
	valueKey.values.arc_mode = valuePtr->arc_mode;
    } else {
	valueKey.values.arc_mode = ArcPieSlice;
    }
    if (valueMask & GCTile) {
	valueKey.values.tile = valuePtr->tile;
    } else {
	valueKey.values.tile = None;
    }
    if (valueMask & GCStipple) {
	valueKey.values.stipple = valuePtr->stipple;
    } else {
	valueKey.values.stipple = None;
    }
    if (valueMask & GCTileStipXOrigin) {
	valueKey.values.ts_x_origin = valuePtr->ts_x_origin;
    } else {
	valueKey.values.ts_x_origin = 0;
    }
    if (valueMask & GCTileStipYOrigin) {
	valueKey.values.ts_y_origin = valuePtr->ts_y_origin;
    } else {
	valueKey.values.ts_y_origin = 0;
    }
    if (valueMask & GCFont) {
	valueKey.values.font = valuePtr->font;
    } else {
	valueKey.values.font = None;
    }
    if (valueMask & GCSubwindowMode) {
	valueKey.values.subwindow_mode = valuePtr->subwindow_mode;
    } else {
	valueKey.values.subwindow_mode = ClipByChildren;
    }
    if (valueMask & GCGraphicsExposures) {
	valueKey.values.graphics_exposures = valuePtr->graphics_exposures;
    } else {
	valueKey.values.graphics_exposures = True;
    }
    if (valueMask & GCClipXOrigin) {
	valueKey.values.clip_x_origin = valuePtr->clip_x_origin;
    } else {
	valueKey.values.clip_x_origin = 0;
    }
    if (valueMask & GCClipYOrigin) {
	valueKey.values.clip_y_origin = valuePtr->clip_y_origin;
    } else {
	valueKey.values.clip_y_origin = 0;
    }
    if (valueMask & GCClipMask) {
	valueKey.values.clip_mask = valuePtr->clip_mask;
    } else {
	valueKey.values.clip_mask = None;
    }
    if (valueMask & GCDashOffset) {
	valueKey.values.dash_offset = valuePtr->dash_offset;
    } else {
	valueKey.values.dash_offset = 0;
    }
    if (valueMask & GCDashList) {
	valueKey.values.dashes = valuePtr->dashes;
    } else {
	valueKey.values.dashes = 4;
    }
    valueKey.display = Tk_Display(tkwin);
    valueKey.screenNum = Tk_ScreenNumber(tkwin);
    valueKey.depth = Tk_Depth(tkwin);
    valueHashPtr = Tcl_CreateHashEntry(&dispPtr->gcValueTable,
	    (char *) &valueKey, &isNew);
    if (!isNew) {
	gcPtr = Tcl_GetHashValue(valueHashPtr);
	gcPtr->refCount++;
	return gcPtr->gc;
    }

    /*
     * No GC is currently available for this set of values. Allocate a new GC
     * and add a new structure to the database.
     */

    gcPtr = ckalloc(sizeof(TkGC));

    /*
     * Find or make a drawable to use to specify the screen and depth of the
     * GC. We may have to make a small pixmap, to avoid doing
     * Tk_MakeWindowExist on the window.
     */

    freeDrawable = None;
    if (Tk_WindowId(tkwin) != None) {
	d = Tk_WindowId(tkwin);
    } else if (valueKey.depth ==
	    DefaultDepth(valueKey.display, valueKey.screenNum)) {
	d = RootWindow(valueKey.display, valueKey.screenNum);
    } else {
	d = Tk_GetPixmap(valueKey.display,
		RootWindow(valueKey.display, valueKey.screenNum),
		1, 1, valueKey.depth);
	freeDrawable = d;
    }

    gcPtr->gc = XCreateGC(valueKey.display, d, valueMask, &valueKey.values);
    gcPtr->display = valueKey.display;
    gcPtr->refCount = 1;
    gcPtr->valueHashPtr = valueHashPtr;
    idHashPtr = Tcl_CreateHashEntry(&dispPtr->gcIdTable,
	    (char *) gcPtr->gc, &isNew);
    if (!isNew) {
	Tcl_Panic("GC already registered in Tk_GetGC");
    }
    Tcl_SetHashValue(valueHashPtr, gcPtr);
    Tcl_SetHashValue(idHashPtr, gcPtr);
    if (freeDrawable != None) {
	Tk_FreePixmap(valueKey.display, freeDrawable);
    }

    return gcPtr->gc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeGC --
 *
 *	This function is called to release a graphics context allocated by
 *	Tk_GetGC.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with gc is decremented, and gc is
 *	officially deallocated if no-one is using it anymore.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeGC(
    Display *display,		/* Display for which gc was allocated. */
    GC gc)			/* Graphics context to be released. */
{
    Tcl_HashEntry *idHashPtr;
    register TkGC *gcPtr;
    TkDisplay *dispPtr = TkGetDisplay(display);

    if (!dispPtr->gcInit) {
	Tcl_Panic("Tk_FreeGC called before Tk_GetGC");
    }
    if (dispPtr->gcInit < 0) {
	/*
	 * The GCCleanup has been called, and remaining GCs have been freed.
	 * This may still get called by other things shutting down, but the
	 * GCs should no longer be in use.
	 */

	return;
    }

    idHashPtr = Tcl_FindHashEntry(&dispPtr->gcIdTable, (char *) gc);
    if (idHashPtr == NULL) {
	Tcl_Panic("Tk_FreeGC received unknown gc argument");
    }
    gcPtr = Tcl_GetHashValue(idHashPtr);
    gcPtr->refCount--;
    if (gcPtr->refCount == 0) {
	XFreeGC(gcPtr->display, gcPtr->gc);
	Tcl_DeleteHashEntry(gcPtr->valueHashPtr);
	Tcl_DeleteHashEntry(idHashPtr);
	ckfree(gcPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkGCCleanup --
 *
 *	Frees the structures used for GC management. We need to have it called
 *	near the end, when other cleanup that calls Tk_FreeGC is all done.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	GC resources are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkGCCleanup(
    TkDisplay *dispPtr)		/* display to clean up resources in */
{
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;
    TkGC *gcPtr;

    for (entryPtr = Tcl_FirstHashEntry(&dispPtr->gcIdTable, &search);
	    entryPtr != NULL; entryPtr = Tcl_NextHashEntry(&search)) {
	gcPtr = Tcl_GetHashValue(entryPtr);

	XFreeGC(gcPtr->display, gcPtr->gc);
	Tcl_DeleteHashEntry(gcPtr->valueHashPtr);
	Tcl_DeleteHashEntry(entryPtr);
	ckfree(gcPtr);
    }
    Tcl_DeleteHashTable(&dispPtr->gcValueTable);
    Tcl_DeleteHashTable(&dispPtr->gcIdTable);
    dispPtr->gcInit = -1;
}

/*
 *----------------------------------------------------------------------
 *
 * GCInit --
 *
 *	Initialize the structures used for GC management.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read the code.
 *
 *----------------------------------------------------------------------
 */

static void
GCInit(
    TkDisplay *dispPtr)
{
    if (dispPtr->gcInit < 0) {
	Tcl_Panic("called GCInit after GCCleanup");
    }
    dispPtr->gcInit = 1;
    Tcl_InitHashTable(&dispPtr->gcValueTable, sizeof(ValueKey)/sizeof(int));
    Tcl_InitHashTable(&dispPtr->gcIdTable, TCL_ONE_WORD_KEYS);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
