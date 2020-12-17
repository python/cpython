/*
 * tkCanvWind.c --
 *
 *	This file implements window items for canvas widgets.
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
 * The structure below defines the record for each window item.
 */

typedef struct WindowItem  {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types.  MUST BE FIRST IN STRUCTURE. */
    double x, y;		/* Coordinates of positioning point for
				 * window. */
    Tk_Window tkwin;		/* Window associated with item.  NULL means
				 * window has been destroyed. */
    int width;			/* Width to use for window (<= 0 means use
				 * window's requested width). */
    int height;			/* Width to use for window (<= 0 means use
				 * window's requested width). */
    Tk_Anchor anchor;		/* Where to anchor window relative to
				 * (x,y). */
    Tk_Canvas canvas;		/* Canvas containing this item. */
} WindowItem;

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
    {TK_CONFIG_ANCHOR, "-anchor", NULL, NULL,
	"center", Tk_Offset(WindowItem, anchor), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_PIXELS, "-height", NULL, NULL,
	"0", Tk_Offset(WindowItem, height), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_CUSTOM, "-state", NULL, NULL,
	NULL, Tk_Offset(Tk_Item, state), TK_CONFIG_NULL_OK, &stateOption},
    {TK_CONFIG_CUSTOM, "-tags", NULL, NULL,
	NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_PIXELS, "-width", NULL, NULL,
	"0", Tk_Offset(WindowItem, width), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_WINDOW, "-window", NULL, NULL,
	NULL, Tk_Offset(WindowItem, tkwin), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};

/*
 * Prototypes for functions defined in this file:
 */

static void		ComputeWindowBbox(Tk_Canvas canvas,
			    WindowItem *winItemPtr);
static int		ConfigureWinItem(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *const objv[], int flags);
static int		CreateWinItem(Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int objc, Tcl_Obj *const objv[]);
static void		DeleteWinItem(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display);
static void		DisplayWinItem(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height);
static void		ScaleWinItem(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY);
static void		TranslateWinItem(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double deltaX, double deltaY);
static int		WinItemCoords(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *const objv[]);
static void		WinItemLostSlaveProc(ClientData clientData,
			    Tk_Window tkwin);
static void		WinItemRequestProc(ClientData clientData,
			    Tk_Window tkwin);
static void		WinItemStructureProc(ClientData clientData,
			    XEvent *eventPtr);
static int		WinItemToArea(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *rectPtr);
static int		WinItemToPostscript(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass);
static double		WinItemToPoint(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *pointPtr);
#ifdef X_GetImage
static int		xerrorhandler(ClientData clientData, XErrorEvent *e);
#endif
static int		CanvasPsWindow(Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_Canvas canvas, double x,
			    double y, int width, int height);

/*
 * The structure below defines the window item type by means of functions
 * that can be invoked by generic item code.
 */

Tk_ItemType tkWindowType = {
    "window",			/* name */
    sizeof(WindowItem),		/* itemSize */
    CreateWinItem,		/* createProc */
    configSpecs,		/* configSpecs */
    ConfigureWinItem,		/* configureProc */
    WinItemCoords,		/* coordProc */
    DeleteWinItem,		/* deleteProc */
    DisplayWinItem,		/* displayProc */
    1|TK_CONFIG_OBJS,		/* flags */
    WinItemToPoint,		/* pointProc */
    WinItemToArea,		/* areaProc */
    WinItemToPostscript,	/* postscriptProc */
    ScaleWinItem,		/* scaleProc */
    TranslateWinItem,		/* translateProc */
    NULL,			/* indexProc */
    NULL,			/* cursorProc */
    NULL,			/* selectionProc */
    NULL,			/* insertProc */
    NULL,			/* dTextProc */
    NULL,			/* nextPtr */
    NULL, 0, NULL, NULL
};

/*
 * The structure below defines the official type record for the canvas (as
 * geometry manager):
 */

static const Tk_GeomMgr canvasGeomType = {
    "canvas",				/* name */
    WinItemRequestProc,			/* requestProc */
    WinItemLostSlaveProc,		/* lostSlaveProc */
};

/*
 *--------------------------------------------------------------
 *
 * CreateWinItem --
 *
 *	This function is invoked to create a new window item in a canvas.
 *
 * Results:
 *	A standard Tcl return value. If an error occurred in creating the
 *	item, then an error message is left in the interp's result; in this
 *	case itemPtr is left uninitialized, so it can be safely freed by the
 *	caller.
 *
 * Side effects:
 *	A new window item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateWinItem(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Canvas canvas,		/* Canvas to hold new item. */
    Tk_Item *itemPtr,		/* Record to hold new item; header has been
				 * initialized by caller. */
    int objc,			/* Number of arguments in objv. */
    Tcl_Obj *const objv[])	/* Arguments describing window. */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;
    int i;

    if (objc == 0) {
	Tcl_Panic("canvas did not pass any coords");
    }

    /*
     * Initialize item's record.
     */

    winItemPtr->tkwin = NULL;
    winItemPtr->width = 0;
    winItemPtr->height = 0;
    winItemPtr->anchor = TK_ANCHOR_CENTER;
    winItemPtr->canvas = canvas;

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
    if (WinItemCoords(interp, canvas, itemPtr, i, objv) != TCL_OK) {
	goto error;
    }
    if (ConfigureWinItem(interp, canvas, itemPtr, objc-i, objv+i, 0)
	    == TCL_OK) {
	return TCL_OK;
    }

  error:
    DeleteWinItem(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * WinItemCoords --
 *
 *	This function is invoked to process the "coords" widget command on
 *	window items. See the user documentation for details on what it does.
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
WinItemCoords(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item whose coordinates are to be read or
				 * modified. */
    int objc,			/* Number of coordinates supplied in objv. */
    Tcl_Obj *const objv[])	/* Array of coordinates: x1, y1, x2, y2, ... */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;

    if (objc == 0) {
	Tcl_Obj *objs[2];

	objs[0] = Tcl_NewDoubleObj(winItemPtr->x);
	objs[1] = Tcl_NewDoubleObj(winItemPtr->y);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, objs));
    } else if (objc < 3) {
	if (objc==1) {
	    if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		    (Tcl_Obj ***) &objv) != TCL_OK) {
		return TCL_ERROR;
	    } else if (objc != 2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"wrong # coordinates: expected 2, got %d", objc));
		Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "WINDOW",
			NULL);
		return TCL_ERROR;
	    }
	}
	if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0], &winItemPtr->x)
		!= TCL_OK) || (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
		&winItemPtr->y) != TCL_OK)) {
	    return TCL_ERROR;
	}
	ComputeWindowBbox(canvas, winItemPtr);
    } else {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"wrong # coordinates: expected 0 or 2, got %d", objc));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "WINDOW", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureWinItem --
 *
 *	This function is invoked to configure various aspects of a window
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
ConfigureWinItem(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr,		/* Window item to reconfigure. */
    int objc,			/* Number of elements in objv.  */
    Tcl_Obj *const objv[],	/* Arguments describing things to configure. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;
    Tk_Window oldWindow;
    Tk_Window canvasTkwin;

    oldWindow = winItemPtr->tkwin;
    canvasTkwin = Tk_CanvasTkwin(canvas);
    if (TCL_OK != Tk_ConfigureWidget(interp, canvasTkwin, configSpecs, objc,
	    (const char **) objv, (char *) winItemPtr, flags|TK_CONFIG_OBJS)) {
	return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing.
     */

    if (oldWindow != winItemPtr->tkwin) {
	if (oldWindow != NULL) {
	    Tk_DeleteEventHandler(oldWindow, StructureNotifyMask,
		    WinItemStructureProc, winItemPtr);
	    Tk_ManageGeometry(oldWindow, NULL, NULL);
	    Tk_UnmaintainGeometry(oldWindow, canvasTkwin);
	    Tk_UnmapWindow(oldWindow);
	}
	if (winItemPtr->tkwin != NULL) {
	    Tk_Window ancestor, parent;

	    /*
	     * Make sure that the canvas is either the parent of the window
	     * associated with the item or a descendant of that parent. Also,
	     * don't allow a top-of-hierarchy window to be managed inside a
	     * canvas.
	     */

	    parent = Tk_Parent(winItemPtr->tkwin);
	    for (ancestor = canvasTkwin ;; ancestor = Tk_Parent(ancestor)) {
		if (ancestor == parent) {
		    break;
		}
		if (((Tk_FakeWin *) ancestor)->flags & TK_TOP_HIERARCHY) {
		    goto badWindow;
		}
	    }
	    if (((Tk_FakeWin *) winItemPtr->tkwin)->flags & TK_TOP_HIERARCHY){
		goto badWindow;
	    }
	    if (winItemPtr->tkwin == canvasTkwin) {
		goto badWindow;
	    }
	    Tk_CreateEventHandler(winItemPtr->tkwin, StructureNotifyMask,
		    WinItemStructureProc, winItemPtr);
	    Tk_ManageGeometry(winItemPtr->tkwin, &canvasGeomType, winItemPtr);
	}
    }
    if ((winItemPtr->tkwin != NULL)
	    && (itemPtr->state == TK_STATE_HIDDEN)) {
	if (canvasTkwin == Tk_Parent(winItemPtr->tkwin)) {
	    Tk_UnmapWindow(winItemPtr->tkwin);
	} else {
	    Tk_UnmaintainGeometry(winItemPtr->tkwin, canvasTkwin);
	}
    }

    ComputeWindowBbox(canvas, winItemPtr);
    return TCL_OK;

  badWindow:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "can't use %s in a window item of this canvas",
	    Tk_PathName(winItemPtr->tkwin)));
    Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "HIERARCHY", NULL);
    winItemPtr->tkwin = NULL;
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteWinItem --
 *
 *	This function is called to clean up the data structure associated with
 *	a window item.
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
DeleteWinItem(
    Tk_Canvas canvas,		/* Overall info about widget. */
    Tk_Item *itemPtr,		/* Item that is being deleted. */
    Display *display)		/* Display containing window for canvas. */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;
    Tk_Window canvasTkwin = Tk_CanvasTkwin(canvas);

    if (winItemPtr->tkwin != NULL) {
	Tk_DeleteEventHandler(winItemPtr->tkwin, StructureNotifyMask,
		WinItemStructureProc, winItemPtr);
	Tk_ManageGeometry(winItemPtr->tkwin, NULL, NULL);
	if (canvasTkwin != Tk_Parent(winItemPtr->tkwin)) {
	    Tk_UnmaintainGeometry(winItemPtr->tkwin, canvasTkwin);
	}
	Tk_UnmapWindow(winItemPtr->tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeWindowBbox --
 *
 *	This function is invoked to compute the bounding box of all the pixels
 *	that may be drawn as part of a window item. This function is where the
 *	child window's placement is computed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header for itemPtr.
 *
 *--------------------------------------------------------------
 */

static void
ComputeWindowBbox(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    WindowItem *winItemPtr)	/* Item whose bbox is to be recomputed. */
{
    int width, height, x, y;
    Tk_State state = winItemPtr->header.state;

    x = (int) (winItemPtr->x + ((winItemPtr->x >= 0) ? 0.5 : - 0.5));
    y = (int) (winItemPtr->y + ((winItemPtr->y >= 0) ? 0.5 : - 0.5));

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    if ((winItemPtr->tkwin == NULL) || (state == TK_STATE_HIDDEN)) {
	/*
	 * There is no window for this item yet. Just give it a 1x1 bounding
	 * box. Don't give it a 0x0 bounding box; there are strange cases
	 * where this bounding box might be used as the dimensions of the
	 * window, and 0x0 causes problems under X.
	 */

	winItemPtr->header.x1 = x;
	winItemPtr->header.x2 = winItemPtr->header.x1 + 1;
	winItemPtr->header.y1 = y;
	winItemPtr->header.y2 = winItemPtr->header.y1 + 1;
	return;
    }

    /*
     * Compute dimensions of window.
     */

    width = winItemPtr->width;
    if (width <= 0) {
	width = Tk_ReqWidth(winItemPtr->tkwin);
	if (width <= 0) {
	    width = 1;
	}
    }
    height = winItemPtr->height;
    if (height <= 0) {
	height = Tk_ReqHeight(winItemPtr->tkwin);
	if (height <= 0) {
	    height = 1;
	}
    }

    /*
     * Compute location of window, using anchor information.
     */

    switch (winItemPtr->anchor) {
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

    winItemPtr->header.x1 = x;
    winItemPtr->header.y1 = y;
    winItemPtr->header.x2 = x + width;
    winItemPtr->header.y2 = y + height;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayWinItem --
 *
 *	This function is invoked to "draw" a window item in a given drawable.
 *	Since the window draws itself, we needn't do any actual redisplay
 *	here. However, this function takes care of actually repositioning the
 *	child window so that it occupies the correct screen position.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The child window's position may get changed. Note: this function gets
 *	called both when a window needs to be displayed and when it ceases to
 *	be visible on the screen (e.g. it was scrolled or moved off-screen or
 *	the enclosing canvas is unmapped).
 *
 *--------------------------------------------------------------
 */

static void
DisplayWinItem(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    Tk_Item *itemPtr,		/* Item to be displayed. */
    Display *display,		/* Display on which to draw item. */
    Drawable drawable,		/* Pixmap or window in which to draw item. */
    int regionX, int regionY, int regionWidth, int regionHeight)
				/* Describes region of canvas that must be
				 * redisplayed (not used). */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;
    int width, height;
    short x, y;
    Tk_Window canvasTkwin = Tk_CanvasTkwin(canvas);
    Tk_State state = itemPtr->state;

    if (winItemPtr->tkwin == NULL) {
	return;
    }
    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }

    /*
     * A drawable of None is used by the canvas UnmapNotify handler
     * to indicate that we should no longer display ourselves.
     */
    if (state == TK_STATE_HIDDEN || drawable == None) {
	if (canvasTkwin == Tk_Parent(winItemPtr->tkwin)) {
	    Tk_UnmapWindow(winItemPtr->tkwin);
	} else {
	    Tk_UnmaintainGeometry(winItemPtr->tkwin, canvasTkwin);
	}
	return;
    }
    Tk_CanvasWindowCoords(canvas, (double) winItemPtr->header.x1,
	    (double) winItemPtr->header.y1, &x, &y);
    width = winItemPtr->header.x2 - winItemPtr->header.x1;
    height = winItemPtr->header.y2 - winItemPtr->header.y1;

    /*
     * If the window is completely out of the visible area of the canvas then
     * unmap it. This code used not to be present (why unmap the window if it
     * isn't visible anyway?) but this could cause the window to suddenly
     * reappear if the canvas window got resized.
     */

    if (((x + width) <= 0) || ((y + height) <= 0)
	    || (x >= Tk_Width(canvasTkwin)) || (y >= Tk_Height(canvasTkwin))) {
	if (canvasTkwin == Tk_Parent(winItemPtr->tkwin)) {
	    Tk_UnmapWindow(winItemPtr->tkwin);
	} else {
	    Tk_UnmaintainGeometry(winItemPtr->tkwin, canvasTkwin);
	}
	return;
    }

    /*
     * Reposition and map the window (but in different ways depending on
     * whether the canvas is the window's parent).
     */

    if (canvasTkwin == Tk_Parent(winItemPtr->tkwin)) {
	if ((x != Tk_X(winItemPtr->tkwin)) || (y != Tk_Y(winItemPtr->tkwin))
		|| (width != Tk_Width(winItemPtr->tkwin))
		|| (height != Tk_Height(winItemPtr->tkwin))) {
	    Tk_MoveResizeWindow(winItemPtr->tkwin, x, y, width, height);
	}
	Tk_MapWindow(winItemPtr->tkwin);
    } else {
	Tk_MaintainGeometry(winItemPtr->tkwin, canvasTkwin, x, y,
		width, height);
    }
}

/*
 *--------------------------------------------------------------
 *
 * WinItemToPoint --
 *
 *	Computes the distance from a given point to a given window, in canvas
 *	units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	coordPtr[0] and coordPtr[1] is inside the window. If the point isn't
 *	inside the window then the return value is the distance from the point
 *	to the window.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static double
WinItemToPoint(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item to check against point. */
    double *pointPtr)		/* Pointer to x and y coordinates. */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;
    double x1, x2, y1, y2, xDiff, yDiff;

    x1 = winItemPtr->header.x1;
    y1 = winItemPtr->header.y1;
    x2 = winItemPtr->header.x2;
    y2 = winItemPtr->header.y2;

    /*
     * Point is outside window.
     */

    if (pointPtr[0] < x1) {
	xDiff = x1 - pointPtr[0];
    } else if (pointPtr[0] >= x2)  {
	xDiff = pointPtr[0] + 1 - x2;
    } else {
	xDiff = 0;
    }

    if (pointPtr[1] < y1) {
	yDiff = y1 - pointPtr[1];
    } else if (pointPtr[1] >= y2)  {
	yDiff = pointPtr[1] + 1 - y2;
    } else {
	yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 *--------------------------------------------------------------
 *
 * WinItemToArea --
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

static int
WinItemToArea(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item to check against rectangle. */
    double *rectPtr)		/* Pointer to array of four coordinates
				 * (x1,y1,x2,y2) describing rectangular
				 * area.  */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;

    if ((rectPtr[2] <= winItemPtr->header.x1)
	    || (rectPtr[0] >= winItemPtr->header.x2)
	    || (rectPtr[3] <= winItemPtr->header.y1)
	    || (rectPtr[1] >= winItemPtr->header.y2)) {
	return -1;
    }
    if ((rectPtr[0] <= winItemPtr->header.x1)
	    && (rectPtr[1] <= winItemPtr->header.y1)
	    && (rectPtr[2] >= winItemPtr->header.x2)
	    && (rectPtr[3] >= winItemPtr->header.y2)) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * xerrorhandler --
 *
 *	This is a dummy function to catch X11 errors during an attempt to
 *	print a canvas window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

#ifdef X_GetImage
static int
xerrorhandler(
    ClientData clientData,
    XErrorEvent *e)
{
    return 0;
}
#endif /* X_GetImage */

/*
 *--------------------------------------------------------------
 *
 * WinItemToPostscript --
 *
 *	This function is called to generate Postscript for window items.
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs in
 *	generating Postscript then an error message is left in interp->result,
 *	replacing whatever used to be there. If no error occurs, then
 *	Postscript for the item is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
WinItemToPostscript(
    Tcl_Interp *interp,		/* Leave Postscript or error message here. */
    Tk_Canvas canvas,		/* Information about overall canvas. */
    Tk_Item *itemPtr,		/* Item for which Postscript is wanted. */
    int prepass)		/* 1 means this is a prepass to collect font
				 * information; 0 means final Postscript is
				 * being created. */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;
    double x, y;
    int width, height;
    Tk_Window tkwin = winItemPtr->tkwin;

    if (prepass || winItemPtr->tkwin == NULL) {
        return TCL_OK;
    }

    width = Tk_Width(tkwin);
    height = Tk_Height(tkwin);

    /*
     * Compute the coordinates of the lower-left corner of the window, taking
     * into account the anchor position for the window.
     */

    x = winItemPtr->x;
    y = Tk_CanvasPsY(canvas, winItemPtr->y);

    switch (winItemPtr->anchor) {
    case TK_ANCHOR_NW:			    y -= height;	    break;
    case TK_ANCHOR_N:	    x -= width/2.0; y -= height;	    break;
    case TK_ANCHOR_NE:	    x -= width;	    y -= height;	    break;
    case TK_ANCHOR_E:	    x -= width;	    y -= height/2.0;	    break;
    case TK_ANCHOR_SE:	    x -= width;				    break;
    case TK_ANCHOR_S:	    x -= width/2.0;			    break;
    case TK_ANCHOR_SW:						    break;
    case TK_ANCHOR_W:			    y -= height/2.0;	    break;
    case TK_ANCHOR_CENTER:  x -= width/2.0; y -= height/2.0;	    break;
    }

    return CanvasPsWindow(interp, tkwin, canvas, x, y, width, height);
}

static int
CanvasPsWindow(
    Tcl_Interp *interp,		/* Leave Postscript or error message here. */
    Tk_Window tkwin,		/* window to be printed */
    Tk_Canvas canvas,		/* Information about overall canvas. */
    double x, double y,		/* origin of window. */
    int width, int height)	/* width/height of window. */
{
    XImage *ximage;
    int result;
#ifdef X_GetImage
    Tk_ErrorHandler handle;
#endif
    Tcl_Obj *cmdObj, *psObj;
    Tcl_InterpState interpState = Tcl_SaveInterpState(interp, TCL_OK);

    /*
     * Locate the subwindow within the wider window.
     */

    psObj = Tcl_ObjPrintf(
	    "\n%%%% %s item (%s, %d x %d)\n"	/* Comment */
	    "%.15g %.15g translate\n",		/* Position */
	    Tk_Class(tkwin), Tk_PathName(tkwin), width, height, x, y);

    /*
     * First try if the widget has its own "postscript" command. If it exists,
     * this will produce much better postscript than when a pixmap is used.
     */

    Tcl_ResetResult(interp);
    cmdObj = Tcl_ObjPrintf("%s postscript -prolog 0", Tk_PathName(tkwin));
    Tcl_IncrRefCount(cmdObj);
    result = Tcl_EvalObjEx(interp, cmdObj, 0);
    Tcl_DecrRefCount(cmdObj);

    if (result == TCL_OK) {
	Tcl_AppendPrintfToObj(psObj,
		"50 dict begin\nsave\ngsave\n"
		"0 %d moveto %d 0 rlineto 0 -%d rlineto -%d 0 rlineto closepath\n"
		"1.000 1.000 1.000 setrgbcolor AdjustColor\nfill\ngrestore\n",
		height, width, height, width);
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
	Tcl_AppendToObj(psObj, "\nrestore\nend\n\n\n", -1);
	goto done;
    }

    /*
     * If the window is off the screen it will generate a BadMatch/XError. We
     * catch any BadMatch errors here
     */

#ifdef X_GetImage
    handle = Tk_CreateErrorHandler(Tk_Display(tkwin), BadMatch,
	    X_GetImage, -1, xerrorhandler, tkwin);
#endif

    /*
     * Generate an XImage from the window. We can then read pixel values out
     * of the XImage.
     */

    ximage = XGetImage(Tk_Display(tkwin), Tk_WindowId(tkwin), 0, 0,
	    (unsigned) width, (unsigned) height, AllPlanes, ZPixmap);

#ifdef X_GetImage
    Tk_DeleteErrorHandler(handle);
#endif

    if (ximage == NULL) {
	result = TCL_OK;
    } else {
	Tcl_ResetResult(interp);
	result = TkPostscriptImage(interp, tkwin, Canvas(canvas)->psInfo,
		ximage, 0, 0, width, height);
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
	XDestroyImage(ximage);
    }

    /*
     * Plug the accumulated postscript back into the result.
     */

  done:
    if (result == TCL_OK) {
	(void) Tcl_RestoreInterpState(interp, interpState);
	Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
    } else {
	Tcl_DiscardInterpState(interpState);
    }
    Tcl_DecrRefCount(psObj);
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleWinItem --
 *
 *	This function is invoked to rescale a window item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window referred to by itemPtr is rescaled so that the following
 *	transformation is applied to all point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleWinItem(
    Tk_Canvas canvas,		/* Canvas containing window. */
    Tk_Item *itemPtr,		/* Window to be scaled. */
    double originX, double originY,
				/* Origin about which to scale window. */
    double scaleX,		/* Amount to scale in X direction. */
    double scaleY)		/* Amount to scale in Y direction. */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;

    winItemPtr->x = originX + scaleX*(winItemPtr->x - originX);
    winItemPtr->y = originY + scaleY*(winItemPtr->y - originY);
    if (winItemPtr->width > 0) {
	winItemPtr->width = (int) (scaleX*winItemPtr->width);
    }
    if (winItemPtr->height > 0) {
	winItemPtr->height = (int) (scaleY*winItemPtr->height);
    }
    ComputeWindowBbox(canvas, winItemPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateWinItem --
 *
 *	This function is called to move a window by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the window is offset by (xDelta, yDelta), and the
 *	bounding box is updated in the generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateWinItem(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item that is being moved. */
    double deltaX, double deltaY)
				/* Amount by which item is to be moved. */
{
    WindowItem *winItemPtr = (WindowItem *) itemPtr;

    winItemPtr->x += deltaX;
    winItemPtr->y += deltaY;
    ComputeWindowBbox(canvas, winItemPtr);
}

/*
 *--------------------------------------------------------------
 *
 * WinItemStructureProc --
 *
 *	This function is invoked whenever StructureNotify events occur for a
 *	window that's managed as part of a canvas window item. This function's
 *	only purpose is to clean up when windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is disassociated from the window item when it is deleted.
 *
 *--------------------------------------------------------------
 */

static void
WinItemStructureProc(
    ClientData clientData,	/* Pointer to record describing window item. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    WindowItem *winItemPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	winItemPtr->tkwin = NULL;
    }
}

/*
 *--------------------------------------------------------------
 *
 * WinItemRequestProc --
 *
 *	This function is invoked whenever a window that's associated with a
 *	window canvas item changes its requested dimensions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size and location on the screen of the window may change,
 *	depending on the options specified for the window item.
 *
 *--------------------------------------------------------------
 */

static void
WinItemRequestProc(
    ClientData clientData,	/* Pointer to record for window item. */
    Tk_Window tkwin)		/* Window that changed its desired size. */
{
    WindowItem *winItemPtr = clientData;

    ComputeWindowBbox(winItemPtr->canvas, winItemPtr);

    /*
     * A drawable argument of None to DisplayWinItem is used by the canvas
     * UnmapNotify handler to indicate that we should no longer display
     * ourselves, so need to pass a (bogus) non-zero drawable value here.
     */
    DisplayWinItem(winItemPtr->canvas, (Tk_Item *) winItemPtr, NULL,
	    (Drawable) -1, 0, 0, 0, 0);
}

/*
 *--------------------------------------------------------------
 *
 * WinItemLostSlaveProc --
 *
 *	This function is invoked by Tk whenever some other geometry claims
 *	control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all canvas-related information about the slave.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
WinItemLostSlaveProc(
    ClientData clientData,	/* WindowItem structure for slave window that
				 * was stolen away. */
    Tk_Window tkwin)		/* Tk's handle for the slave window. */
{
    WindowItem *winItemPtr = clientData;
    Tk_Window canvasTkwin = Tk_CanvasTkwin(winItemPtr->canvas);

    Tk_DeleteEventHandler(winItemPtr->tkwin, StructureNotifyMask,
	    WinItemStructureProc, winItemPtr);
    if (canvasTkwin != Tk_Parent(winItemPtr->tkwin)) {
	Tk_UnmaintainGeometry(winItemPtr->tkwin, canvasTkwin);
    }
    Tk_UnmapWindow(winItemPtr->tkwin);
    winItemPtr->tkwin = NULL;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
