/*
 * tkCanvBmap.c --
 *
 *	This file implements bitmap items for canvas widgets.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkCanvas.h"

/*
 * The structure below defines the record for each bitmap item.
 */

typedef struct BitmapItem  {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types. MUST BE FIRST IN STRUCTURE. */
    double x, y;		/* Coordinates of positioning point for
				 * bitmap. */
    Tk_Anchor anchor;		/* Where to anchor bitmap relative to (x,y) */
    Pixmap bitmap;		/* Bitmap to display in window. */
    Pixmap activeBitmap;	/* Bitmap to display in window. */
    Pixmap disabledBitmap;	/* Bitmap to display in window. */
    XColor *fgColor;		/* Foreground color to use for bitmap. */
    XColor *activeFgColor;	/* Foreground color to use for bitmap. */
    XColor *disabledFgColor;	/* Foreground color to use for bitmap. */
    XColor *bgColor;		/* Background color to use for bitmap. */
    XColor *activeBgColor;	/* Background color to use for bitmap. */
    XColor *disabledBgColor;	/* Background color to use for bitmap. */
    GC gc;			/* Graphics context to use for drawing bitmap
				 * on screen. */
} BitmapItem;

/*
 * Information used for parsing configuration specs:
 */

static const Tk_CustomOption stateOption = {
    TkStateParseProc, TkStatePrintProc, INT2PTR(2)
};
static const Tk_CustomOption tagsOption = {
    Tk_CanvasTagsParseProc, Tk_CanvasTagsPrintProc, NULL
};

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-activebackground", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, activeBgColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-activebitmap", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, activeBitmap), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-activeforeground", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, activeFgColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_ANCHOR, "-anchor", NULL, NULL,
	"center", Tk_Offset(BitmapItem, anchor), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_COLOR, "-background", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, bgColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-bitmap", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, bitmap), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-disabledbackground", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, disabledBgColor),
	TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-disabledbitmap", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, disabledBitmap),
	TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-disabledforeground", NULL, NULL,
	NULL, Tk_Offset(BitmapItem, disabledFgColor),
	TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-foreground", NULL, NULL,
	"black", Tk_Offset(BitmapItem, fgColor), 0, NULL},
    {TK_CONFIG_CUSTOM, "-state", NULL, NULL,
	NULL, Tk_Offset(Tk_Item, state), TK_CONFIG_NULL_OK,
	&stateOption},
    {TK_CONFIG_CUSTOM, "-tags", NULL, NULL,
	NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};

/*
 * Prototypes for functions defined in this file:
 */

static int		BitmapCoords(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *const objv[]);
static int		BitmapToArea(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *rectPtr);
static double		BitmapToPoint(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *coordPtr);
static int		BitmapToPostscript(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass);
static void		ComputeBitmapBbox(Tk_Canvas canvas,
			    BitmapItem *bmapPtr);
static int		ConfigureBitmap(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *const objv[], int flags);
static int		TkcCreateBitmap(Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int objc, Tcl_Obj *const objv[]);
static void		DeleteBitmap(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display);
static void		DisplayBitmap(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height);
static void		ScaleBitmap(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY);
static void		TranslateBitmap(Tk_Canvas canvas, Tk_Item *itemPtr,
			    double deltaX, double deltaY);

/*
 * The structures below defines the bitmap item type in terms of functions
 * that can be invoked by generic item code.
 */

Tk_ItemType tkBitmapType = {
    "bitmap",			/* name */
    sizeof(BitmapItem),		/* itemSize */
    TkcCreateBitmap,		/* createProc */
    configSpecs,		/* configSpecs */
    ConfigureBitmap,		/* configureProc */
    BitmapCoords,		/* coordProc */
    DeleteBitmap,		/* deleteProc */
    DisplayBitmap,		/* displayProc */
    TK_CONFIG_OBJS,		/* flags */
    BitmapToPoint,		/* pointProc */
    BitmapToArea,		/* areaProc */
    BitmapToPostscript,		/* postscriptProc */
    ScaleBitmap,		/* scaleProc */
    TranslateBitmap,		/* translateProc */
    NULL,			/* indexProc */
    NULL,			/* icursorProc */
    NULL,			/* selectionProc */
    NULL,			/* insertProc */
    NULL,			/* dTextProc */
    NULL,			/* nextPtr */
    NULL, 0, NULL, NULL
};

/*
 *--------------------------------------------------------------
 *
 * TkcCreateBitmap --
 *
 *	This function is invoked to create a new bitmap item in a canvas.
 *
 * Results:
 *	A standard Tcl return value. If an error occurred in creating the
 *	item, then an error message is left in the interp's result; in this
 *	case itemPtr is left uninitialized, so it can be safely freed by the
 *	caller.
 *
 * Side effects:
 *	A new bitmap item is created.
 *
 *--------------------------------------------------------------
 */

static int
TkcCreateBitmap(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Canvas canvas,		/* Canvas to hold new item. */
    Tk_Item *itemPtr,		/* Record to hold new item; header has been
				 * initialized by caller. */
    int objc,			/* Number of arguments in objv. */
    Tcl_Obj *const objv[])	/* Arguments describing rectangle. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;
    int i;

    if (objc == 0) {
	Tcl_Panic("canvas did not pass any coords");
    }

    /*
     * Initialize item's record.
     */

    bmapPtr->anchor = TK_ANCHOR_CENTER;
    bmapPtr->bitmap = None;
    bmapPtr->activeBitmap = None;
    bmapPtr->disabledBitmap = None;
    bmapPtr->fgColor = NULL;
    bmapPtr->activeFgColor = NULL;
    bmapPtr->disabledFgColor = NULL;
    bmapPtr->bgColor = NULL;
    bmapPtr->activeBgColor = NULL;
    bmapPtr->disabledBgColor = NULL;
    bmapPtr->gc = None;

    /*
     * Process the arguments to fill in the item record. Only 1 (list) or 2 (x
     * y) coords are allowed.
     */

    if (objc == 1) {
	i = 1;
    } else {
	const char *arg = Tcl_GetString(objv[1]);
	i = 2;
	if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    i = 1;
	}
    }
    if (BitmapCoords(interp, canvas, itemPtr, i, objv) != TCL_OK) {
	goto error;
    }
    if (ConfigureBitmap(interp, canvas, itemPtr, objc-i, objv+i, 0)
	    == TCL_OK) {
	return TCL_OK;
    }

  error:
    DeleteBitmap(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * BitmapCoords --
 *
 *	This function is invoked to process the "coords" widget command on
 *	bitmap items. See the user documentation for details on what it does.
 *
 * Results:
 *	Returns TCL_OK or TCL_ERROR, and sets the interp's result.
 *
 * Side effects:
 *	The coordinates for the given item may be changed.
 *
 *--------------------------------------------------------------
 */

static int
BitmapCoords(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item whose coordinates are to be read or
				 * modified. */
    int objc,			/* Number of coordinates supplied in objv. */
    Tcl_Obj *const objv[])	/* Array of coordinates: x1, y1, x2, y2, ... */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;

    if (objc == 0) {
	Tcl_Obj *obj = Tcl_NewObj();

	Tcl_ListObjAppendElement(NULL, obj, Tcl_NewDoubleObj(bmapPtr->x));
	Tcl_ListObjAppendElement(NULL, obj, Tcl_NewDoubleObj(bmapPtr->y));
	Tcl_SetObjResult(interp, obj);
    } else if (objc < 3) {
	if (objc == 1) {
	    if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		    (Tcl_Obj ***) &objv) != TCL_OK) {
		return TCL_ERROR;
	    } else if (objc != 2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"wrong # coordinates: expected 2, got %d", objc));
		Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "BITMAP",
			NULL);
		return TCL_ERROR;
	    }
	}
	if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0],
		&bmapPtr->x) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
			&bmapPtr->y) != TCL_OK)) {
	    return TCL_ERROR;
	}
	ComputeBitmapBbox(canvas, bmapPtr);
    } else {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"wrong # coordinates: expected 0 or 2, got %d", objc));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "BITMAP", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureBitmap --
 *
 *	This function is invoked to configure various aspects of a bitmap
 *	item, such as its anchor position.
 *
 * Results:
 *	A standard Tcl result code. If an error occurs, then an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	Configuration information may be set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigureBitmap(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr,		/* Bitmap item to reconfigure. */
    int objc,			/* Number of elements in objv.  */
    Tcl_Obj *const objv[],	/* Arguments describing things to configure. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    Tk_Window tkwin;
    unsigned long mask;
    XColor *fgColor;
    XColor *bgColor;
    Pixmap bitmap;
    Tk_State state;

    tkwin = Tk_CanvasTkwin(canvas);
    if (TCL_OK != Tk_ConfigureWidget(interp, tkwin, configSpecs, objc,
	    (const char **) objv, (char *) bmapPtr, flags|TK_CONFIG_OBJS)) {
	return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing, such as those that
     * determine the graphics context.
     */

    state = itemPtr->state;

    if (bmapPtr->activeFgColor!=NULL ||
	    bmapPtr->activeBgColor!=NULL ||
	    bmapPtr->activeBitmap!=None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    if (state == TK_STATE_HIDDEN) {
	ComputeBitmapBbox(canvas, bmapPtr);
	return TCL_OK;
    }
    fgColor = bmapPtr->fgColor;
    bgColor = bmapPtr->bgColor;
    bitmap = bmapPtr->bitmap;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (bmapPtr->activeFgColor!=NULL) {
	    fgColor = bmapPtr->activeFgColor;
	}
	if (bmapPtr->activeBgColor!=NULL) {
	    bgColor = bmapPtr->activeBgColor;
	}
	if (bmapPtr->activeBitmap!=None) {
	    bitmap = bmapPtr->activeBitmap;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (bmapPtr->disabledFgColor!=NULL) {
	    fgColor = bmapPtr->disabledFgColor;
	}
	if (bmapPtr->disabledBgColor!=NULL) {
	    bgColor = bmapPtr->disabledBgColor;
	}
	if (bmapPtr->disabledBitmap!=None) {
	    bitmap = bmapPtr->disabledBitmap;
	}
    }

    if (bitmap == None) {
	newGC = None;
    } else {
	gcValues.foreground = fgColor->pixel;
	mask = GCForeground;
	if (bgColor != NULL) {
	    gcValues.background = bgColor->pixel;
	    mask |= GCBackground;
	} else {
	    gcValues.clip_mask = bitmap;
	    mask |= GCClipMask;
	}
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    }
    if (bmapPtr->gc != None) {
	Tk_FreeGC(Tk_Display(tkwin), bmapPtr->gc);
    }
    bmapPtr->gc = newGC;

    ComputeBitmapBbox(canvas, bmapPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteBitmap --
 *
 *	This function is called to clean up the data structure associated with
 *	a bitmap item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with itemPtr are released.
 *
 *--------------------------------------------------------------
 */

static void
DeleteBitmap(
    Tk_Canvas canvas,		/* Info about overall canvas widget. */
    Tk_Item *itemPtr,		/* Item that is being deleted. */
    Display *display)		/* Display containing window for canvas. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;

    if (bmapPtr->bitmap != None) {
	Tk_FreeBitmap(display, bmapPtr->bitmap);
    }
    if (bmapPtr->activeBitmap != None) {
	Tk_FreeBitmap(display, bmapPtr->activeBitmap);
    }
    if (bmapPtr->disabledBitmap != None) {
	Tk_FreeBitmap(display, bmapPtr->disabledBitmap);
    }
    if (bmapPtr->fgColor != NULL) {
	Tk_FreeColor(bmapPtr->fgColor);
    }
    if (bmapPtr->activeFgColor != NULL) {
	Tk_FreeColor(bmapPtr->activeFgColor);
    }
    if (bmapPtr->disabledFgColor != NULL) {
	Tk_FreeColor(bmapPtr->disabledFgColor);
    }
    if (bmapPtr->bgColor != NULL) {
	Tk_FreeColor(bmapPtr->bgColor);
    }
    if (bmapPtr->activeBgColor != NULL) {
	Tk_FreeColor(bmapPtr->activeBgColor);
    }
    if (bmapPtr->disabledBgColor != NULL) {
	Tk_FreeColor(bmapPtr->disabledBgColor);
    }
    if (bmapPtr->gc != NULL) {
	Tk_FreeGC(display, bmapPtr->gc);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeBitmapBbox --
 *
 *	This function is invoked to compute the bounding box of all the pixels
 *	that may be drawn as part of a bitmap item. This function is where the
 *	child bitmap's placement is computed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header for itemPtr.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
ComputeBitmapBbox(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    BitmapItem *bmapPtr)	/* Item whose bbox is to be recomputed. */
{
    int width, height;
    int x, y;
    Pixmap bitmap;
    Tk_State state = bmapPtr->header.state;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    bitmap = bmapPtr->bitmap;
    if (Canvas(canvas)->currentItemPtr == (Tk_Item *)bmapPtr) {
	if (bmapPtr->activeBitmap!=None) {
	    bitmap = bmapPtr->activeBitmap;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (bmapPtr->disabledBitmap!=None) {
	    bitmap = bmapPtr->disabledBitmap;
	}
    }

    x = (int) (bmapPtr->x + ((bmapPtr->x >= 0) ? 0.5 : - 0.5));
    y = (int) (bmapPtr->y + ((bmapPtr->y >= 0) ? 0.5 : - 0.5));

    if (state==TK_STATE_HIDDEN || bitmap == None) {
	bmapPtr->header.x1 = bmapPtr->header.x2 = x;
	bmapPtr->header.y1 = bmapPtr->header.y2 = y;
	return;
    }

    /*
     * Compute location and size of bitmap, using anchor information.
     */

    Tk_SizeOfBitmap(Tk_Display(Tk_CanvasTkwin(canvas)), bitmap,
	    &width, &height);
    switch (bmapPtr->anchor) {
    case TK_ANCHOR_N:
	x -= width/2;
	break;
    case TK_ANCHOR_NE:
	x -= width;
	break;
    case TK_ANCHOR_E:
	x -= width;
	y -= height/2;
	break;
    case TK_ANCHOR_SE:
	x -= width;
	y -= height;
	break;
    case TK_ANCHOR_S:
	x -= width/2;
	y -= height;
	break;
    case TK_ANCHOR_SW:
	y -= height;
	break;
    case TK_ANCHOR_W:
	y -= height/2;
	break;
    case TK_ANCHOR_NW:
	break;
    case TK_ANCHOR_CENTER:
	x -= width/2;
	y -= height/2;
	break;
    }

    /*
     * Store the information in the item header.
     */

    bmapPtr->header.x1 = x;
    bmapPtr->header.y1 = y;
    bmapPtr->header.x2 = x + width;
    bmapPtr->header.y2 = y + height;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayBitmap --
 *
 *	This function is invoked to draw a bitmap item in a given drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation information in
 *	canvas.
 *
 *--------------------------------------------------------------
 */

static void
DisplayBitmap(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    Tk_Item *itemPtr,		/* Item to be displayed. */
    Display *display,		/* Display on which to draw item. */
    Drawable drawable,		/* Pixmap or window in which to draw item. */
    int x, int y, int width, int height)
				/* Describes region of canvas that must be
				 * redisplayed (not used). */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;
    int bmapX, bmapY, bmapWidth, bmapHeight;
    short drawableX, drawableY;
    Pixmap bitmap;
    Tk_State state = itemPtr->state;

    /*
     * If the area being displayed doesn't cover the whole bitmap, then only
     * redisplay the part of the bitmap that needs redisplay.
     */

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    bitmap = bmapPtr->bitmap;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (bmapPtr->activeBitmap!=None) {
	    bitmap = bmapPtr->activeBitmap;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (bmapPtr->disabledBitmap!=None) {
	    bitmap = bmapPtr->disabledBitmap;
	}
    }

    if (bitmap != None) {
	if (x > bmapPtr->header.x1) {
	    bmapX = x - bmapPtr->header.x1;
	    bmapWidth = bmapPtr->header.x2 - x;
	} else {
	    bmapX = 0;
	    if ((x+width) < bmapPtr->header.x2) {
		bmapWidth = x + width - bmapPtr->header.x1;
	    } else {
		bmapWidth = bmapPtr->header.x2 - bmapPtr->header.x1;
	    }
	}
	if (y > bmapPtr->header.y1) {
	    bmapY = y - bmapPtr->header.y1;
	    bmapHeight = bmapPtr->header.y2 - y;
	} else {
	    bmapY = 0;
	    if ((y+height) < bmapPtr->header.y2) {
		bmapHeight = y + height - bmapPtr->header.y1;
	    } else {
		bmapHeight = bmapPtr->header.y2 - bmapPtr->header.y1;
	    }
	}
	Tk_CanvasDrawableCoords(canvas,
		(double) (bmapPtr->header.x1 + bmapX),
		(double) (bmapPtr->header.y1 + bmapY),
		&drawableX, &drawableY);

	/*
	 * Must modify the mask origin within the graphics context to line up
	 * with the bitmap's origin (in order to make bitmaps with
	 * "-background {}" work right).
	 */

	XSetClipOrigin(display, bmapPtr->gc, drawableX - bmapX,
		drawableY - bmapY);
	XCopyPlane(display, bitmap, drawable,
		bmapPtr->gc, bmapX, bmapY, (unsigned int) bmapWidth,
		(unsigned int) bmapHeight, drawableX, drawableY, 1);
	XSetClipOrigin(display, bmapPtr->gc, 0, 0);
    }
}

/*
 *--------------------------------------------------------------
 *
 * BitmapToPoint --
 *
 *	Computes the distance from a given point to a given rectangle, in
 *	canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	coordPtr[0] and coordPtr[1] is inside the bitmap. If the point isn't
 *	inside the bitmap then the return value is the distance from the point
 *	to the bitmap.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
BitmapToPoint(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item to check against point. */
    double *coordPtr)		/* Pointer to x and y coordinates. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;
    double x1, x2, y1, y2, xDiff, yDiff;

    x1 = bmapPtr->header.x1;
    y1 = bmapPtr->header.y1;
    x2 = bmapPtr->header.x2;
    y2 = bmapPtr->header.y2;

    /*
     * Point is outside rectangle.
     */

    if (coordPtr[0] < x1) {
	xDiff = x1 - coordPtr[0];
    } else if (coordPtr[0] > x2)  {
	xDiff = coordPtr[0] - x2;
    } else {
	xDiff = 0;
    }

    if (coordPtr[1] < y1) {
	yDiff = y1 - coordPtr[1];
    } else if (coordPtr[1] > y2)  {
	yDiff = coordPtr[1] - y2;
    } else {
	yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 *--------------------------------------------------------------
 *
 * BitmapToArea --
 *
 *	This function is called to determine whether an item lies entirely
 *	inside, entirely outside, or overlapping a given rectangle.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area given by
 *	rectPtr, 0 if it overlaps, and 1 if it is entirely inside the given
 *	area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
BitmapToArea(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item to check against rectangle. */
    double *rectPtr)		/* Pointer to array of four coordinates
				 * (x1,y1,x2,y2) describing rectangular
				 * area. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;

    if ((rectPtr[2] <= bmapPtr->header.x1)
	    || (rectPtr[0] >= bmapPtr->header.x2)
	    || (rectPtr[3] <= bmapPtr->header.y1)
	    || (rectPtr[1] >= bmapPtr->header.y2)) {
	return -1;
    }
    if ((rectPtr[0] <= bmapPtr->header.x1)
	    && (rectPtr[1] <= bmapPtr->header.y1)
	    && (rectPtr[2] >= bmapPtr->header.x2)
	    && (rectPtr[3] >= bmapPtr->header.y2)) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleBitmap --
 *
 *	This function is invoked to rescale a bitmap item in a canvas. It is
 *	one of the standard item functions for bitmap items, and is invoked by
 *	the generic canvas code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The item referred to by itemPtr is rescaled so that the following
 *	transformation is applied to all point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleBitmap(
    Tk_Canvas canvas,		/* Canvas containing rectangle. */
    Tk_Item *itemPtr,		/* Rectangle to be scaled. */
    double originX, double originY,
				/* Origin about which to scale item. */
    double scaleX,		/* Amount to scale in X direction. */
    double scaleY)		/* Amount to scale in Y direction. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;

    bmapPtr->x = originX + scaleX*(bmapPtr->x - originX);
    bmapPtr->y = originY + scaleY*(bmapPtr->y - originY);
    ComputeBitmapBbox(canvas, bmapPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateBitmap --
 *
 *	This function is called to move an item by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the item is offset by (xDelta, yDelta), and the
 *	bounding box is updated in the generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateBitmap(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item that is being moved. */
    double deltaX, double deltaY)
				/* Amount by which item is to be moved. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;

    bmapPtr->x += deltaX;
    bmapPtr->y += deltaY;
    ComputeBitmapBbox(canvas, bmapPtr);
}

/*
 *--------------------------------------------------------------
 *
 * BitmapToPostscript --
 *
 *	This function is called to generate Postscript for bitmap items.
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs in
 *	generating Postscript then an error message is left in the interp's
 *	result, replacing whatever used to be there. If no error occurs, then
 *	Postscript for the item is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
BitmapToPostscript(
    Tcl_Interp *interp,		/* Leave Postscript or error message here. */
    Tk_Canvas canvas,		/* Information about overall canvas. */
    Tk_Item *itemPtr,		/* Item for which Postscript is wanted. */
    int prepass)		/* 1 means this is a prepass to collect font
				 * information; 0 means final Postscript is
				 * being created. */
{
    BitmapItem *bmapPtr = (BitmapItem *) itemPtr;
    double x, y;
    int width, height, rowsAtOnce, rowsThisTime;
    int curRow;
    XColor *fgColor;
    XColor *bgColor;
    Pixmap bitmap;
    Tk_State state = itemPtr->state;
    Tcl_Obj *psObj;
    Tcl_InterpState interpState;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    fgColor = bmapPtr->fgColor;
    bgColor = bmapPtr->bgColor;
    bitmap = bmapPtr->bitmap;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (bmapPtr->activeFgColor!=NULL) {
	    fgColor = bmapPtr->activeFgColor;
	}
	if (bmapPtr->activeBgColor!=NULL) {
	    bgColor = bmapPtr->activeBgColor;
	}
	if (bmapPtr->activeBitmap!=None) {
	    bitmap = bmapPtr->activeBitmap;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (bmapPtr->disabledFgColor!=NULL) {
	    fgColor = bmapPtr->disabledFgColor;
	}
	if (bmapPtr->disabledBgColor!=NULL) {
	    bgColor = bmapPtr->disabledBgColor;
	}
	if (bmapPtr->disabledBitmap!=None) {
	    bitmap = bmapPtr->disabledBitmap;
	}
    }

    if (bitmap == None) {
	return TCL_OK;
    }

    /*
     * Compute the coordinates of the lower-left corner of the bitmap, taking
     * into account the anchor position for the bitmp.
     */

    x = bmapPtr->x;
    y = Tk_CanvasPsY(canvas, bmapPtr->y);
    Tk_SizeOfBitmap(Tk_Display(Tk_CanvasTkwin(canvas)), bitmap,
	    &width, &height);
    switch (bmapPtr->anchor) {
    case TK_ANCHOR_NW:			   y -= height;		break;
    case TK_ANCHOR_N:	   x -= width/2.0; y -= height;		break;
    case TK_ANCHOR_NE:	   x -= width;	   y -= height;		break;
    case TK_ANCHOR_E:	   x -= width;	   y -= height/2.0;	break;
    case TK_ANCHOR_SE:	   x -= width;				break;
    case TK_ANCHOR_S:	   x -= width/2.0;			break;
    case TK_ANCHOR_SW:						break;
    case TK_ANCHOR_W:			   y -= height/2.0;	break;
    case TK_ANCHOR_CENTER: x -= width/2.0; y -= height/2.0;	break;
    }

    /*
     * Make our working space.
     */

    psObj = Tcl_NewObj();
    interpState = Tcl_SaveInterpState(interp, TCL_OK);

    /*
     * Color the background, if there is one.
     */

    if (bgColor != NULL) {
	Tcl_AppendPrintfToObj(psObj,
		"%.15g %.15g moveto %d 0 rlineto 0 %d rlineto "
		"%d 0 rlineto closepath\n",
		x, y, width, height, -width);

	Tcl_ResetResult(interp);
	if (Tk_CanvasPsColor(interp, canvas, bgColor) != TCL_OK) {
	    goto error;
	}
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	Tcl_AppendToObj(psObj, "fill\n", -1);
    }

    /*
     * Draw the bitmap, if there is a foreground color. If the bitmap is very
     * large, then chop it up into multiple bitmaps, each consisting of one or
     * more rows. This is needed because Postscript can't handle single
     * strings longer than 64 KBytes long.
     */

    if (fgColor != NULL) {
	Tcl_ResetResult(interp);
	if (Tk_CanvasPsColor(interp, canvas, fgColor) != TCL_OK) {
	    goto error;
	}
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	if (width > 60000) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't generate Postscript for bitmaps more than 60000"
		    " pixels wide", -1));
	    Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "MEMLIMIT", NULL);
	    goto error;
	}

	rowsAtOnce = 60000/width;
	if (rowsAtOnce < 1) {
	    rowsAtOnce = 1;
	}

	Tcl_AppendPrintfToObj(psObj, "%.15g %.15g translate\n", x, y+height);

	for (curRow = 0; curRow < height; curRow += rowsAtOnce) {
	    rowsThisTime = rowsAtOnce;
	    if (rowsThisTime > (height - curRow)) {
		rowsThisTime = height - curRow;
	    }

	    Tcl_AppendPrintfToObj(psObj,
		    "0 -%.15g translate\n%d %d true matrix {\n",
		    (double) rowsThisTime, width, rowsThisTime);

	    Tcl_ResetResult(interp);
	    if (Tk_CanvasPsBitmap(interp, canvas, bitmap,
		    0, curRow, width, rowsThisTime) != TCL_OK) {
		goto error;
	    }
	    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	    Tcl_AppendToObj(psObj, "\n} imagemask\n", -1);
	}
    }

    /*
     * Plug the accumulated postscript back into the result.
     */

    (void) Tcl_RestoreInterpState(interp, interpState);
    Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
    Tcl_DecrRefCount(psObj);
    return TCL_OK;

  error:
    Tcl_DiscardInterpState(interpState);
    Tcl_DecrRefCount(psObj);
    return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
