/*
 * tkCanvArc.c --
 *
 *	This file implements arc items for canvas widgets.
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
 * The structure below defines the record for each arc item.
 */

typedef enum {
    PIESLICE_STYLE, CHORD_STYLE, ARC_STYLE
} Style;

typedef struct ArcItem {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types. MUST BE FIRST IN STRUCTURE. */
    Tk_Outline outline;		/* Outline structure */
    double bbox[4];		/* Coordinates (x1, y1, x2, y2) of bounding
				 * box for oval of which arc is a piece. */
    double start;		/* Angle at which arc begins, in degrees
				 * between 0 and 360. */
    double extent;		/* Extent of arc (angular distance from start
				 * to end of arc) in degrees between -360 and
				 * 360. */
    double *outlinePtr;		/* Points to (x,y) coordinates for points that
				 * define one or two closed polygons
				 * representing the portion of the outline
				 * that isn't part of the arc (the V-shape for
				 * a pie slice or a line-like segment for a
				 * chord). Malloc'ed. */
    int numOutlinePoints;	/* Number of points at outlinePtr. Zero means
				 * no space allocated. */
    Tk_TSOffset tsoffset;
    XColor *fillColor;		/* Color for filling arc (used for drawing
				 * outline too when style is "arc"). NULL
				 * means don't fill arc. */
    XColor *activeFillColor;	/* Color for filling arc (used for drawing
				 * outline too when style is "arc" and state
				 * is "active"). NULL means use fillColor. */
    XColor *disabledFillColor;	/* Color for filling arc (used for drawing
				 * outline too when style is "arc" and state
				 * is "disabled". NULL means use fillColor */
    Pixmap fillStipple;		/* Stipple bitmap for filling item. */
    Pixmap activeFillStipple;	/* Stipple bitmap for filling item if state is
				 * active. */
    Pixmap disabledFillStipple;	/* Stipple bitmap for filling item if state is
				 * disabled. */
    Style style;		/* How to draw arc: arc, chord, or
				 * pieslice. */
    GC fillGC;			/* Graphics context for filling item. */
    double center1[2];		/* Coordinates of center of arc outline at
				 * start (see ComputeArcOutline). */
    double center2[2];		/* Coordinates of center of arc outline at
				 * start+extent (see ComputeArcOutline). */
} ArcItem;

/*
 * The definitions below define the sizes of the polygons used to display
 * outline information for various styles of arcs:
 */

#define CHORD_OUTLINE_PTS	7
#define PIE_OUTLINE1_PTS	6
#define PIE_OUTLINE2_PTS	7

/*
 * Information used for parsing configuration specs:
 */

static int	StyleParseProc(ClientData clientData, Tcl_Interp *interp,
		    Tk_Window tkwin, const char *value,
		    char *widgRec, int offset);
static const char * StylePrintProc(ClientData clientData, Tk_Window tkwin,
		    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr);

static const Tk_CustomOption stateOption = {
    TkStateParseProc, TkStatePrintProc, INT2PTR(2)
};
static const Tk_CustomOption styleOption = {
    StyleParseProc, StylePrintProc, NULL
};
static const Tk_CustomOption tagsOption = {
    Tk_CanvasTagsParseProc, Tk_CanvasTagsPrintProc, NULL
};
static const Tk_CustomOption dashOption = {
    TkCanvasDashParseProc, TkCanvasDashPrintProc, NULL
};
static const Tk_CustomOption offsetOption = {
    TkOffsetParseProc, TkOffsetPrintProc, INT2PTR(TK_OFFSET_RELATIVE)
};
static const Tk_CustomOption pixelOption = {
    TkPixelParseProc, TkPixelPrintProc, NULL
};

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_CUSTOM, "-activedash", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.activeDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-activefill", NULL, NULL,
	NULL, Tk_Offset(ArcItem, activeFillColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-activeoutline", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.activeColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-activeoutlinestipple", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.activeStipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-activestipple", NULL, NULL,
	NULL, Tk_Offset(ArcItem, activeFillStipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_CUSTOM, "-activewidth", NULL, NULL,
	"0.0", Tk_Offset(ArcItem, outline.activeWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_CUSTOM, "-dash", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.dash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_PIXELS, "-dashoffset", NULL, NULL,
	"0", Tk_Offset(ArcItem, outline.offset), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_CUSTOM, "-disableddash", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.disabledDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-disabledfill", NULL, NULL,
	NULL, Tk_Offset(ArcItem, disabledFillColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-disabledoutline", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.disabledColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-disabledoutlinestipple", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.disabledStipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-disabledstipple", NULL, NULL,
	NULL, Tk_Offset(ArcItem, disabledFillStipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_CUSTOM, "-disabledwidth", NULL, NULL,
	"0.0", Tk_Offset(ArcItem, outline.disabledWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_DOUBLE, "-extent", NULL, NULL,
	"90", Tk_Offset(ArcItem, extent), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_COLOR, "-fill", NULL, NULL,
	NULL, Tk_Offset(ArcItem, fillColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_CUSTOM, "-offset", NULL, NULL,
	"0,0", Tk_Offset(ArcItem, tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_COLOR, "-outline", NULL, NULL,
	"black", Tk_Offset(ArcItem, outline.color), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_CUSTOM, "-outlineoffset", NULL, NULL,
	"0,0", Tk_Offset(ArcItem, outline.tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_BITMAP, "-outlinestipple", NULL, NULL,
	NULL, Tk_Offset(ArcItem, outline.stipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_DOUBLE, "-start", NULL, NULL,
	"0", Tk_Offset(ArcItem, start), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_CUSTOM, "-state", NULL, NULL,
	NULL, Tk_Offset(Tk_Item, state), TK_CONFIG_NULL_OK, &stateOption},
    {TK_CONFIG_BITMAP, "-stipple", NULL, NULL,
	NULL, Tk_Offset(ArcItem, fillStipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_CUSTOM, "-style", NULL, NULL,
	NULL, Tk_Offset(ArcItem, style), TK_CONFIG_DONT_SET_DEFAULT,
	&styleOption},
    {TK_CONFIG_CUSTOM, "-tags", NULL, NULL,
	NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_CUSTOM, "-width", NULL, NULL,
	"1.0", Tk_Offset(ArcItem, outline.width), TK_CONFIG_DONT_SET_DEFAULT,
	&pixelOption},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};

/*
 * Prototypes for functions defined in this file:
 */

static void		ComputeArcBbox(Tk_Canvas canvas, ArcItem *arcPtr);
static int		ConfigureArc(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *const objv[], int flags);
static int		CreateArc(Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int objc, Tcl_Obj *const objv[]);
static void		DeleteArc(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display);
static void		DisplayArc(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height);
static int		ArcCoords(Tcl_Interp *interp, Tk_Canvas canvas,
			    Tk_Item *itemPtr, int objc, Tcl_Obj *const objv[]);
static int		ArcToArea(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *rectPtr);
static double		ArcToPoint(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *coordPtr);
static int		ArcToPostscript(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass);
static void		ScaleArc(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY);
static void		TranslateArc(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double deltaX, double deltaY);
static int		AngleInRange(double x, double y,
			    double start, double extent);
static void		ComputeArcOutline(Tk_Canvas canvas, ArcItem *arcPtr);
static int		HorizLineToArc(double x1, double x2,
			    double y, double rx, double ry,
			    double start, double extent);
static int		VertLineToArc(double x, double y1,
			    double y2, double rx, double ry,
			    double start, double extent);

/*
 * The structures below defines the arc item types by means of functions that
 * can be invoked by generic item code.
 */

Tk_ItemType tkArcType = {
    "arc",			/* name */
    sizeof(ArcItem),		/* itemSize */
    CreateArc,			/* createProc */
    configSpecs,		/* configSpecs */
    ConfigureArc,		/* configureProc */
    ArcCoords,			/* coordProc */
    DeleteArc,			/* deleteProc */
    DisplayArc,			/* displayProc */
    TK_CONFIG_OBJS,		/* flags */
    ArcToPoint,			/* pointProc */
    ArcToArea,			/* areaProc */
    ArcToPostscript,		/* postscriptProc */
    ScaleArc,			/* scaleProc */
    TranslateArc,		/* translateProc */
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
 * CreateArc --
 *
 *	This function is invoked to create a new arc item in a canvas.
 *
 * Results:
 *	A standard Tcl return value. If an error occurred in creating the
 *	item, then an error message is left in the interp's result; in this
 *	case itemPtr is left uninitialized, so it can be safely freed by the
 *	caller.
 *
 * Side effects:
 *	A new arc item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateArc(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Canvas canvas,		/* Canvas to hold new item. */
    Tk_Item *itemPtr,		/* Record to hold new item; header has been
				 * initialized by caller. */
    int objc,			/* Number of arguments in objv. */
    Tcl_Obj *const objv[])	/* Arguments describing arc. */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;
    int i;

    if (objc == 0) {
	Tcl_Panic("canvas did not pass any coords");
    }

    /*
     * Carry out initialization that is needed in order to clean up after
     * errors during the the remainder of this function.
     */

    Tk_CreateOutline(&(arcPtr->outline));
    arcPtr->start = 0;
    arcPtr->extent = 90;
    arcPtr->outlinePtr = NULL;
    arcPtr->numOutlinePoints = 0;
    arcPtr->tsoffset.flags = 0;
    arcPtr->tsoffset.xoffset = 0;
    arcPtr->tsoffset.yoffset = 0;
    arcPtr->fillColor = NULL;
    arcPtr->activeFillColor = NULL;
    arcPtr->disabledFillColor = NULL;
    arcPtr->fillStipple = None;
    arcPtr->activeFillStipple = None;
    arcPtr->disabledFillStipple = None;
    arcPtr->style = PIESLICE_STYLE;
    arcPtr->fillGC = None;

    /*
     * Process the arguments to fill in the item record.
     */

    for (i = 1; i < objc; i++) {
	const char *arg = Tcl_GetString(objv[i]);

	if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    break;
	}
    }
    if (ArcCoords(interp, canvas, itemPtr, i, objv) != TCL_OK) {
	goto error;
    }
    if (ConfigureArc(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
	return TCL_OK;
    }

  error:
    DeleteArc(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * ArcCoords --
 *
 *	This function is invoked to process the "coords" widget command on
 *	arcs. See the user documentation for details on what it does.
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
ArcCoords(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item whose coordinates are to be read or
				 * modified. */
    int objc,			/* Number of coordinates supplied in objv. */
    Tcl_Obj *const objv[])	/* Array of coordinates: x1, y1, x2, y2, ... */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;

    if (objc == 0) {
	Tcl_Obj *objs[4];

	objs[0] = Tcl_NewDoubleObj(arcPtr->bbox[0]);
	objs[1] = Tcl_NewDoubleObj(arcPtr->bbox[1]);
	objs[2] = Tcl_NewDoubleObj(arcPtr->bbox[2]);
	objs[3] = Tcl_NewDoubleObj(arcPtr->bbox[3]);
	Tcl_SetObjResult(interp, Tcl_NewListObj(4, objs));
    } else if ((objc == 1)||(objc == 4)) {
	if (objc==1) {
	    if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		    (Tcl_Obj ***) &objv) != TCL_OK) {
		return TCL_ERROR;
	    } else if (objc != 4) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"wrong # coordinates: expected 4, got %d", objc));
		Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "ARC",
			NULL);
		return TCL_ERROR;
	    }
	}
	if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0],
 		    &arcPtr->bbox[0]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
		    &arcPtr->bbox[1]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[2],
			&arcPtr->bbox[2]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[3],
			&arcPtr->bbox[3]) != TCL_OK)) {
	    return TCL_ERROR;
	}
	ComputeArcBbox(canvas, arcPtr);
    } else {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"wrong # coordinates: expected 0 or 4, got %d", objc));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "ARC", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureArc --
 *
 *	This function is invoked to configure various aspects of a arc item,
 *	such as its outline and fill colors.
 *
 * Results:
 *	A standard Tcl result code. If an error occurs, then an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	Configuration information, such as colors and stipple patterns, may be
 *	set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigureArc(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr,		/* Arc item to reconfigure. */
    int objc,			/* Number of elements in objv. */
    Tcl_Obj *const objv[],	/* Arguments describing things to configure. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    int i;
    Tk_Window tkwin;
    Tk_TSOffset *tsoffset;
    XColor *color;
    Pixmap stipple;
    Tk_State state;

    tkwin = Tk_CanvasTkwin(canvas);
    if (TCL_OK != Tk_ConfigureWidget(interp, tkwin, configSpecs, objc,
	    (const char **) objv, (char *) arcPtr, flags|TK_CONFIG_OBJS)) {
	return TCL_ERROR;
    }

    state = itemPtr->state;

    /*
     * A few of the options require additional processing, such as style and
     * graphics contexts.
     */

    if (arcPtr->outline.activeWidth > arcPtr->outline.width ||
	    arcPtr->outline.activeDash.number != 0 ||
	    arcPtr->outline.activeColor != NULL ||
	    arcPtr->outline.activeStipple != None ||
	    arcPtr->activeFillColor != NULL ||
	    arcPtr->activeFillStipple != None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }

    tsoffset = &arcPtr->outline.tsoffset;
    flags = tsoffset->flags;
    if (flags & TK_OFFSET_LEFT) {
	tsoffset->xoffset = (int) (arcPtr->bbox[0] + 0.5);
    } else if (flags & TK_OFFSET_CENTER) {
	tsoffset->xoffset = (int) ((arcPtr->bbox[0]+arcPtr->bbox[2]+1)/2);
    } else if (flags & TK_OFFSET_RIGHT) {
	tsoffset->xoffset = (int) (arcPtr->bbox[2] + 0.5);
    }
    if (flags & TK_OFFSET_TOP) {
	tsoffset->yoffset = (int) (arcPtr->bbox[1] + 0.5);
    } else if (flags & TK_OFFSET_MIDDLE) {
	tsoffset->yoffset = (int) ((arcPtr->bbox[1]+arcPtr->bbox[3]+1)/2);
    } else if (flags & TK_OFFSET_BOTTOM) {
	tsoffset->yoffset = (int) (arcPtr->bbox[2] + 0.5);
    }

    i = (int) (arcPtr->start/360.0);
    arcPtr->start -= i*360.0;
    if (arcPtr->start < 0) {
	arcPtr->start += 360.0;
    }
    i = (int) (arcPtr->extent/360.0);
    arcPtr->extent -= i*360.0;

    mask = Tk_ConfigOutlineGC(&gcValues, canvas, itemPtr, &(arcPtr->outline));
    if (mask) {
	gcValues.cap_style = CapButt;
	mask |= GCCapStyle;
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    } else {
	newGC = None;
    }
    if (arcPtr->outline.gc != None) {
	Tk_FreeGC(Tk_Display(tkwin), arcPtr->outline.gc);
    }
    arcPtr->outline.gc = newGC;

    if(state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    if (state==TK_STATE_HIDDEN) {
	ComputeArcBbox(canvas, arcPtr);
	return TCL_OK;
    }

    color = arcPtr->fillColor;
    stipple = arcPtr->fillStipple;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (arcPtr->activeFillColor!=NULL) {
	    color = arcPtr->activeFillColor;
	}
	if (arcPtr->activeFillStipple!=None) {
	    stipple = arcPtr->activeFillStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (arcPtr->disabledFillColor!=NULL) {
	    color = arcPtr->disabledFillColor;
	}
	if (arcPtr->disabledFillStipple!=None) {
	    stipple = arcPtr->disabledFillStipple;
	}
      }

    if (arcPtr->style == ARC_STYLE) {
	newGC = None;
    } else if (color == NULL) {
	newGC = None;
    } else {
	gcValues.foreground = color->pixel;
	if (arcPtr->style == CHORD_STYLE) {
	    gcValues.arc_mode = ArcChord;
	} else {
	    gcValues.arc_mode = ArcPieSlice;
	}
	mask = GCForeground|GCArcMode;
	if (stipple != None) {
	    gcValues.stipple = stipple;
	    gcValues.fill_style = FillStippled;
	    mask |= GCStipple|GCFillStyle;
	}
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    }
    if (arcPtr->fillGC != None) {
	Tk_FreeGC(Tk_Display(tkwin), arcPtr->fillGC);
    }
    arcPtr->fillGC = newGC;

    tsoffset = &arcPtr->tsoffset;
    flags = tsoffset->flags;
    if (flags & TK_OFFSET_LEFT) {
	tsoffset->xoffset = (int) (arcPtr->bbox[0] + 0.5);
    } else if (flags & TK_OFFSET_CENTER) {
	tsoffset->xoffset = (int) ((arcPtr->bbox[0]+arcPtr->bbox[2]+1)/2);
    } else if (flags & TK_OFFSET_RIGHT) {
	tsoffset->xoffset = (int) (arcPtr->bbox[2] + 0.5);
    }
    if (flags & TK_OFFSET_TOP) {
	tsoffset->yoffset = (int) (arcPtr->bbox[1] + 0.5);
    } else if (flags & TK_OFFSET_MIDDLE) {
	tsoffset->yoffset = (int) ((arcPtr->bbox[1]+arcPtr->bbox[3]+1)/2);
    } else if (flags & TK_OFFSET_BOTTOM) {
	tsoffset->yoffset = (int) (arcPtr->bbox[3] + 0.5);
    }

    ComputeArcBbox(canvas, arcPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteArc --
 *
 *	This function is called to clean up the data structure associated with
 *	an arc item.
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
DeleteArc(
    Tk_Canvas canvas,		/* Info about overall canvas. */
    Tk_Item *itemPtr,		/* Item that is being deleted. */
    Display *display)		/* Display containing window for canvas. */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;

    Tk_DeleteOutline(display, &(arcPtr->outline));
    if (arcPtr->numOutlinePoints != 0) {
	ckfree(arcPtr->outlinePtr);
    }
    if (arcPtr->fillColor != NULL) {
	Tk_FreeColor(arcPtr->fillColor);
    }
    if (arcPtr->activeFillColor != NULL) {
	Tk_FreeColor(arcPtr->activeFillColor);
    }
    if (arcPtr->disabledFillColor != NULL) {
	Tk_FreeColor(arcPtr->disabledFillColor);
    }
    if (arcPtr->fillStipple != None) {
	Tk_FreeBitmap(display, arcPtr->fillStipple);
    }
    if (arcPtr->activeFillStipple != None) {
	Tk_FreeBitmap(display, arcPtr->activeFillStipple);
    }
    if (arcPtr->disabledFillStipple != None) {
	Tk_FreeBitmap(display, arcPtr->disabledFillStipple);
    }
    if (arcPtr->fillGC != None) {
	Tk_FreeGC(display, arcPtr->fillGC);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeArcBbox --
 *
 *	This function is invoked to compute the bounding box of all the pixels
 *	that may be drawn as part of an arc.
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
ComputeArcBbox(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    ArcItem *arcPtr)		/* Item whose bbox is to be recomputed. */
{
    double tmp, center[2], point[2];
    double width;
    Tk_State state = arcPtr->header.state;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }

    width = arcPtr->outline.width;
    if (width < 1.0) {
	width = 1.0;
    }
    if (state==TK_STATE_HIDDEN) {
	arcPtr->header.x1 = arcPtr->header.x2 =
	arcPtr->header.y1 = arcPtr->header.y2 = -1;
	return;
    } else if (Canvas(canvas)->currentItemPtr == (Tk_Item *) arcPtr) {
	if (arcPtr->outline.activeWidth>width) {
	    width = arcPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (arcPtr->outline.disabledWidth>0) {
	    width = arcPtr->outline.disabledWidth;
	}
    }

    /*
     * Make sure that the first coordinates are the lowest ones.
     */

    if (arcPtr->bbox[1] > arcPtr->bbox[3]) {
	double tmp = arcPtr->bbox[3];

	arcPtr->bbox[3] = arcPtr->bbox[1];
	arcPtr->bbox[1] = tmp;
    }
    if (arcPtr->bbox[0] > arcPtr->bbox[2]) {
	double tmp = arcPtr->bbox[2];

	arcPtr->bbox[2] = arcPtr->bbox[0];
	arcPtr->bbox[0] = tmp;
    }

    ComputeArcOutline(canvas,arcPtr);

    /*
     * To compute the bounding box, start with the the bbox formed by the two
     * endpoints of the arc. Then add in the center of the arc's oval (if
     * relevant) and the 3-o'clock, 6-o'clock, 9-o'clock, and 12-o'clock
     * positions, if they are relevant.
     */

    arcPtr->header.x1 = arcPtr->header.x2 = (int) arcPtr->center1[0];
    arcPtr->header.y1 = arcPtr->header.y2 = (int) arcPtr->center1[1];
    TkIncludePoint((Tk_Item *) arcPtr, arcPtr->center2);
    center[0] = (arcPtr->bbox[0] + arcPtr->bbox[2])/2;
    center[1] = (arcPtr->bbox[1] + arcPtr->bbox[3])/2;
    if (arcPtr->style == PIESLICE_STYLE) {
	TkIncludePoint((Tk_Item *) arcPtr, center);
    }

    tmp = -arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	point[0] = arcPtr->bbox[2];
	point[1] = center[1];
	TkIncludePoint((Tk_Item *) arcPtr, point);
    }
    tmp = 90.0 - arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	point[0] = center[0];
	point[1] = arcPtr->bbox[1];
	TkIncludePoint((Tk_Item *) arcPtr, point);
    }
    tmp = 180.0 - arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	point[0] = arcPtr->bbox[0];
	point[1] = center[1];
	TkIncludePoint((Tk_Item *) arcPtr, point);
    }
    tmp = 270.0 - arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	point[0] = center[0];
	point[1] = arcPtr->bbox[3];
	TkIncludePoint((Tk_Item *) arcPtr, point);
    }

    /*
     * Lastly, expand by the width of the arc (if the arc's outline is being
     * drawn) and add one extra pixel just for safety.
     */

    if (arcPtr->outline.gc == None) {
	tmp = 1;
    } else {
	tmp = (int) ((width + 1.0)/2.0 + 1);
    }
    arcPtr->header.x1 -= (int) tmp;
    arcPtr->header.y1 -= (int) tmp;
    arcPtr->header.x2 += (int) tmp;
    arcPtr->header.y2 += (int) tmp;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayArc --
 *
 *	This function is invoked to draw an arc item in a given drawable.
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
DisplayArc(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    Tk_Item *itemPtr,		/* Item to be displayed. */
    Display *display,		/* Display on which to draw item. */
    Drawable drawable,		/* Pixmap or window in which to draw item. */
    int x, int y,		/* Describes region of canvas that must be */
    int width, int height)	/* redisplayed (not used). */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;
    short x1, y1, x2, y2;
    int start, extent, dashnumber;
    double lineWidth;
    Tk_State state = itemPtr->state;
    Pixmap stipple;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    lineWidth = arcPtr->outline.width;
    if (lineWidth < 1.0) {
	lineWidth = 1.0;
    }
    dashnumber = arcPtr->outline.dash.number;
    stipple = arcPtr->fillStipple;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (arcPtr->outline.activeWidth>lineWidth) {
	    lineWidth = arcPtr->outline.activeWidth;
	}
	if (arcPtr->outline.activeDash.number != 0) {
	    dashnumber = arcPtr->outline.activeDash.number;
	}
	if (arcPtr->activeFillStipple != None) {
	    stipple = arcPtr->activeFillStipple;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (arcPtr->outline.disabledWidth > 0) {
	    lineWidth = arcPtr->outline.disabledWidth;
	}
	if (arcPtr->outline.disabledDash.number != 0) {
	    dashnumber = arcPtr->outline.disabledDash.number;
	}
	if (arcPtr->disabledFillStipple != None) {
	    stipple = arcPtr->disabledFillStipple;
	}
    }

    /*
     * Compute the screen coordinates of the bounding box for the item, plus
     * integer values for the angles.
     */

    Tk_CanvasDrawableCoords(canvas, arcPtr->bbox[0], arcPtr->bbox[1],
	    &x1, &y1);
    Tk_CanvasDrawableCoords(canvas, arcPtr->bbox[2], arcPtr->bbox[3],
	    &x2, &y2);
    if (x2 <= x1) {
	x2 = x1+1;
    }
    if (y2 <= y1) {
	y2 = y1+1;
    }
    start = (int) ((64*arcPtr->start) + 0.5);
    extent = (int) ((64*arcPtr->extent) + 0.5);

    /*
     * Display filled arc first (if wanted), then outline. If the extent is
     * zero then don't invoke XFillArc or XDrawArc, since this causes some
     * window servers to crash and should be a no-op anyway.
     */

    if ((arcPtr->fillGC != None) && (extent != 0)) {
	if (stipple != None) {
	    int w = 0;
	    int h = 0;
	    Tk_TSOffset *tsoffset = &arcPtr->tsoffset;
	    int flags = tsoffset->flags;

	    if (flags & (TK_OFFSET_CENTER|TK_OFFSET_MIDDLE)) {
		Tk_SizeOfBitmap(display, stipple, &w, &h);
		if (flags & TK_OFFSET_CENTER) {
		    w /= 2;
		} else {
		    w = 0;
		}
		if (flags & TK_OFFSET_MIDDLE) {
		    h /= 2;
		} else {
		    h = 0;
		}
	    }
	    tsoffset->xoffset -= w;
	    tsoffset->yoffset -= h;
	    Tk_CanvasSetOffset(canvas, arcPtr->fillGC, tsoffset);
	    if (tsoffset) {
		tsoffset->xoffset += w;
		tsoffset->yoffset += h;
	    }
	}
	XFillArc(display, drawable, arcPtr->fillGC, x1, y1, (unsigned) (x2-x1),
		(unsigned) (y2-y1), start, extent);
	if (stipple != None) {
	    XSetTSOrigin(display, arcPtr->fillGC, 0, 0);
	}
    }
    if (arcPtr->outline.gc != None) {
	Tk_ChangeOutlineGC(canvas, itemPtr, &(arcPtr->outline));

	if (extent != 0) {
	    XDrawArc(display, drawable, arcPtr->outline.gc, x1, y1,
		    (unsigned) (x2-x1), (unsigned) (y2-y1), start, extent);
	}

	/*
	 * If the outline width is very thin, don't use polygons to draw the
	 * linear parts of the outline (this often results in nothing being
	 * displayed); just draw lines instead. The same is done if the
	 * outline is dashed, because then polygons don't work.
	 */

	if (lineWidth < 1.5 || dashnumber != 0) {
	    Tk_CanvasDrawableCoords(canvas, arcPtr->center1[0],
		    arcPtr->center1[1], &x1, &y1);
	    Tk_CanvasDrawableCoords(canvas, arcPtr->center2[0],
		    arcPtr->center2[1], &x2, &y2);

	    if (arcPtr->style == CHORD_STYLE) {
		XDrawLine(display, drawable, arcPtr->outline.gc,
			x1, y1, x2, y2);
	    } else if (arcPtr->style == PIESLICE_STYLE) {
		short cx, cy;

		Tk_CanvasDrawableCoords(canvas,
			(arcPtr->bbox[0] + arcPtr->bbox[2])/2.0,
			(arcPtr->bbox[1] + arcPtr->bbox[3])/2.0, &cx, &cy);
		XDrawLine(display, drawable, arcPtr->outline.gc,
			cx, cy, x1, y1);
		XDrawLine(display, drawable, arcPtr->outline.gc,
			cx, cy, x2, y2);
	    }
	} else {
	    if (arcPtr->style == CHORD_STYLE) {
		TkFillPolygon(canvas, arcPtr->outlinePtr, CHORD_OUTLINE_PTS,
			display, drawable, arcPtr->outline.gc, None);
	    } else if (arcPtr->style == PIESLICE_STYLE) {
		TkFillPolygon(canvas, arcPtr->outlinePtr, PIE_OUTLINE1_PTS,
			display, drawable, arcPtr->outline.gc, None);
		TkFillPolygon(canvas, arcPtr->outlinePtr + 2*PIE_OUTLINE1_PTS,
			PIE_OUTLINE2_PTS, display, drawable,
			arcPtr->outline.gc, None);
	    }
	}

	Tk_ResetOutlineGC(canvas, itemPtr, &(arcPtr->outline));
    }
}

/*
 *--------------------------------------------------------------
 *
 * ArcToPoint --
 *
 *	Computes the distance from a given point to a given arc, in canvas
 *	units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	coordPtr[0] and coordPtr[1] is inside the arc. If the point isn't
 *	inside the arc then the return value is the distance from the point to
 *	the arc. If itemPtr is filled, then anywhere in the interior is
 *	considered "inside"; if itemPtr isn't filled, then "inside" means only
 *	the area occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
ArcToPoint(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item to check against point. */
    double *pointPtr)		/* Pointer to x and y coordinates. */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;
    double vertex[2], pointAngle, diff, dist, newDist;
    double poly[8], polyDist, width, t1, t2;
    int filled, angleInRange;
    Tk_State state = itemPtr->state;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }

    width = (double) arcPtr->outline.width;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (arcPtr->outline.activeWidth>width) {
	    width = (double) arcPtr->outline.activeWidth;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (arcPtr->outline.disabledWidth>0) {
	    width = (double) arcPtr->outline.disabledWidth;
	}
    }

    /*
     * See if the point is within the angular range of the arc. Remember, X
     * angles are backwards from the way we'd normally think of them. Also,
     * compensate for any eccentricity of the oval.
     */

    vertex[0] = (arcPtr->bbox[0] + arcPtr->bbox[2])/2.0;
    vertex[1] = (arcPtr->bbox[1] + arcPtr->bbox[3])/2.0;
    t1 = arcPtr->bbox[3] - arcPtr->bbox[1];
    if (t1 != 0.0) {
	t1 = (pointPtr[1] - vertex[1]) / t1;
    }
    t2 = arcPtr->bbox[2] - arcPtr->bbox[0];
    if (t2 != 0.0) {
	t2 = (pointPtr[0] - vertex[0]) / t2;
    }
    if ((t1 == 0.0) && (t2 == 0.0)) {
	pointAngle = 0;
    } else {
	pointAngle = -atan2(t1, t2)*180/PI;
    }
    diff = pointAngle - arcPtr->start;
    diff -= ((int) (diff/360.0) * 360.0);
    if (diff < 0) {
	diff += 360.0;
    }
    angleInRange = (diff <= arcPtr->extent) ||
	    ((arcPtr->extent < 0) && ((diff - 360.0) >= arcPtr->extent));

    /*
     * Now perform different tests depending on what kind of arc we're dealing
     * with.
     */

    if (arcPtr->style == ARC_STYLE) {
	if (angleInRange) {
	    return TkOvalToPoint(arcPtr->bbox, width, 0, pointPtr);
	}
	dist = hypot(pointPtr[0] - arcPtr->center1[0],
		pointPtr[1] - arcPtr->center1[1]);
	newDist = hypot(pointPtr[0] - arcPtr->center2[0],
		pointPtr[1] - arcPtr->center2[1]);
	if (newDist < dist) {
	    return newDist;
	}
	return dist;
    }

    if ((arcPtr->fillGC != None) || (arcPtr->outline.gc == None)) {
	filled = 1;
    } else {
	filled = 0;
    }
    if (arcPtr->outline.gc == None) {
	width = 0.0;
    }

    if (arcPtr->style == PIESLICE_STYLE) {
	if (width > 1.0) {
	    dist = TkPolygonToPoint(arcPtr->outlinePtr, PIE_OUTLINE1_PTS,
		    pointPtr);
	    newDist = TkPolygonToPoint(arcPtr->outlinePtr + 2*PIE_OUTLINE1_PTS,
		    PIE_OUTLINE2_PTS, pointPtr);
	} else {
	    dist = TkLineToPoint(vertex, arcPtr->center1, pointPtr);
	    newDist = TkLineToPoint(vertex, arcPtr->center2, pointPtr);
	}
	if (newDist < dist) {
	    dist = newDist;
	}
	if (angleInRange) {
	    newDist = TkOvalToPoint(arcPtr->bbox, width, filled, pointPtr);
	    if (newDist < dist) {
		dist = newDist;
	    }
	}
	return dist;
    }

    /*
     * This is a chord-style arc. We have to deal specially with the
     * triangular piece that represents the difference between a chord-style
     * arc and a pie-slice arc (for small angles this piece is excluded here
     * where it would be included for pie slices; for large angles the piece
     * is included here but would be excluded for pie slices).
     */

    if (width > 1.0) {
	dist = TkPolygonToPoint(arcPtr->outlinePtr, CHORD_OUTLINE_PTS,
		pointPtr);
    } else {
	dist = TkLineToPoint(arcPtr->center1, arcPtr->center2, pointPtr);
    }
    poly[0] = poly[6] = vertex[0];
    poly[1] = poly[7] = vertex[1];
    poly[2] = arcPtr->center1[0];
    poly[3] = arcPtr->center1[1];
    poly[4] = arcPtr->center2[0];
    poly[5] = arcPtr->center2[1];
    polyDist = TkPolygonToPoint(poly, 4, pointPtr);
    if (angleInRange) {
	if ((arcPtr->extent < -180.0) || (arcPtr->extent > 180.0)
		|| (polyDist > 0.0)) {
	    newDist = TkOvalToPoint(arcPtr->bbox, width, filled, pointPtr);
	    if (newDist < dist) {
		dist = newDist;
	    }
	}
    } else {
	if ((arcPtr->extent < -180.0) || (arcPtr->extent > 180.0)) {
	    if (filled && (polyDist < dist)) {
		dist = polyDist;
	    }
	}
    }
    return dist;
}

/*
 *--------------------------------------------------------------
 *
 * ArcToArea --
 *
 *	This function is called to determine whether an item lies entirely
 *	inside, entirely outside, or overlapping a given area.
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
ArcToArea(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item to check against arc. */
    double *rectPtr)		/* Pointer to array of four coordinates (x1,
				 * y1, x2, y2) describing rectangular area. */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;
    double rx, ry;		/* Radii for transformed oval: these define an
				 * oval centered at the origin. */
    double tRect[4];		/* Transformed version of x1, y1, x2, y2, for
				 * coord. system where arc is centered on the
				 * origin. */
    double center[2], width, angle, tmp;
    double points[20], *pointPtr;
    int numPoints, filled;
    int inside;			/* Non-zero means every test so far suggests
				 * that arc is inside rectangle. 0 means every
				 * test so far shows arc to be outside of
				 * rectangle. */
    int newInside;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    width = (double) arcPtr->outline.width;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (arcPtr->outline.activeWidth>width) {
	    width = (double) arcPtr->outline.activeWidth;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (arcPtr->outline.disabledWidth>0) {
	    width = (double) arcPtr->outline.disabledWidth;
	}
    }

    if ((arcPtr->fillGC != None) || (arcPtr->outline.gc == None)) {
	filled = 1;
    } else {
	filled = 0;
    }
    if (arcPtr->outline.gc == None) {
	width = 0.0;
    }

    /*
     * Transform both the arc and the rectangle so that the arc's oval is
     * centered on the origin.
     */

    center[0] = (arcPtr->bbox[0] + arcPtr->bbox[2])/2.0;
    center[1] = (arcPtr->bbox[1] + arcPtr->bbox[3])/2.0;
    tRect[0] = rectPtr[0] - center[0];
    tRect[1] = rectPtr[1] - center[1];
    tRect[2] = rectPtr[2] - center[0];
    tRect[3] = rectPtr[3] - center[1];
    rx = arcPtr->bbox[2] - center[0] + width/2.0;
    ry = arcPtr->bbox[3] - center[1] + width/2.0;

    /*
     * Find the extreme points of the arc and see whether these are all inside
     * the rectangle (in which case we're done), partly in and partly out (in
     * which case we're done), or all outside (in which case we have more work
     * to do). The extreme points include the following, which are checked in
     * order:
     *
     * 1. The outside points of the arc, corresponding to start and extent.
     * 2. The center of the arc (but only in pie-slice mode).
     * 3. The 12, 3, 6, and 9-o'clock positions (but only if the arc includes
     *	  those angles).
     */

    pointPtr = points;
    angle = -arcPtr->start*(PI/180.0);
    pointPtr[0] = rx*cos(angle);
    pointPtr[1] = ry*sin(angle);
    angle += -arcPtr->extent*(PI/180.0);
    pointPtr[2] = rx*cos(angle);
    pointPtr[3] = ry*sin(angle);
    numPoints = 2;
    pointPtr += 4;

    if ((arcPtr->style == PIESLICE_STYLE) && (arcPtr->extent < 180.0)) {
	pointPtr[0] = 0.0;
	pointPtr[1] = 0.0;
	numPoints++;
	pointPtr += 2;
    }

    tmp = -arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	pointPtr[0] = rx;
	pointPtr[1] = 0.0;
	numPoints++;
	pointPtr += 2;
    }
    tmp = 90.0 - arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	pointPtr[0] = 0.0;
	pointPtr[1] = -ry;
	numPoints++;
	pointPtr += 2;
    }
    tmp = 180.0 - arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	pointPtr[0] = -rx;
	pointPtr[1] = 0.0;
	numPoints++;
	pointPtr += 2;
    }
    tmp = 270.0 - arcPtr->start;
    if (tmp < 0) {
	tmp += 360.0;
    }
    if ((tmp < arcPtr->extent) || ((tmp-360) > arcPtr->extent)) {
	pointPtr[0] = 0.0;
	pointPtr[1] = ry;
	numPoints++;
    }

    /*
     * Now that we've located the extreme points, loop through them all to see
     * which are inside the rectangle.
     */

    inside = (points[0] > tRect[0]) && (points[0] < tRect[2])
	    && (points[1] > tRect[1]) && (points[1] < tRect[3]);
    for (pointPtr = points+2; numPoints > 1; pointPtr += 2, numPoints--) {
	newInside = (pointPtr[0] > tRect[0]) && (pointPtr[0] < tRect[2])
		&& (pointPtr[1] > tRect[1]) && (pointPtr[1] < tRect[3]);
	if (newInside != inside) {
	    return 0;
	}
    }

    if (inside) {
	return 1;
    }

    /*
     * So far, oval appears to be outside rectangle, but can't yet tell for
     * sure. Next, test each of the four sides of the rectangle against the
     * bounding region for the arc. If any intersections are found, then
     * return "overlapping". First, test against the polygon(s) forming the
     * sides of a chord or pie-slice.
     */

    if (arcPtr->style == PIESLICE_STYLE) {
	if (width >= 1.0) {
	    if (TkPolygonToArea(arcPtr->outlinePtr, PIE_OUTLINE1_PTS,
		    rectPtr) != -1) {
		return 0;
	    }
	    if (TkPolygonToArea(arcPtr->outlinePtr + 2*PIE_OUTLINE1_PTS,
		    PIE_OUTLINE2_PTS, rectPtr) != -1) {
		return 0;
	    }
	} else {
	    if ((TkLineToArea(center, arcPtr->center1, rectPtr) != -1) ||
		    (TkLineToArea(center, arcPtr->center2, rectPtr) != -1)) {
		return 0;
	    }
	}
    } else if (arcPtr->style == CHORD_STYLE) {
	if (width >= 1.0) {
	    if (TkPolygonToArea(arcPtr->outlinePtr, CHORD_OUTLINE_PTS,
		    rectPtr) != -1) {
		return 0;
	    }
	} else {
	    if (TkLineToArea(arcPtr->center1, arcPtr->center2,
		    rectPtr) != -1) {
		return 0;
	    }
	}
    }

    /*
     * Next check for overlap between each of the four sides and the outer
     * perimiter of the arc. If the arc isn't filled, then also check the
     * inner perimeter of the arc.
     */

    if (HorizLineToArc(tRect[0], tRect[2], tRect[1], rx, ry, arcPtr->start,
		arcPtr->extent)
	    || HorizLineToArc(tRect[0], tRect[2], tRect[3], rx, ry,
		arcPtr->start, arcPtr->extent)
	    || VertLineToArc(tRect[0], tRect[1], tRect[3], rx, ry,
		arcPtr->start, arcPtr->extent)
	    || VertLineToArc(tRect[2], tRect[1], tRect[3], rx, ry,
		arcPtr->start, arcPtr->extent)) {
	return 0;
    }
    if ((width > 1.0) && !filled) {
	rx -= width;
	ry -= width;
	if (HorizLineToArc(tRect[0], tRect[2], tRect[1], rx, ry, arcPtr->start,
		    arcPtr->extent)
		|| HorizLineToArc(tRect[0], tRect[2], tRect[3], rx, ry,
		    arcPtr->start, arcPtr->extent)
		|| VertLineToArc(tRect[0], tRect[1], tRect[3], rx, ry,
		    arcPtr->start, arcPtr->extent)
		|| VertLineToArc(tRect[2], tRect[1], tRect[3], rx, ry,
		    arcPtr->start, arcPtr->extent)) {
	    return 0;
	}
    }

    /*
     * The arc still appears to be totally disjoint from the rectangle, but
     * it's also possible that the rectangle is totally inside the arc. Do one
     * last check, which is to check one point of the rectangle to see if it's
     * inside the arc. If it is, we've got overlap. If it isn't, the arc's
     * really outside the rectangle.
     */

    if (ArcToPoint(canvas, itemPtr, rectPtr) == 0.0) {
	return 0;
    }
    return -1;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleArc --
 *
 *	This function is invoked to rescale an arc item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The arc referred to by itemPtr is rescaled so that the following
 *	transformation is applied to all point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleArc(
    Tk_Canvas canvas,		/* Canvas containing arc. */
    Tk_Item *itemPtr,		/* Arc to be scaled. */
    double originX,		/* Origin about which to scale rect. */
    double originY,
    double scaleX,		/* Amount to scale in X direction. */
    double scaleY)		/* Amount to scale in Y direction. */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;

    arcPtr->bbox[0] = originX + scaleX*(arcPtr->bbox[0] - originX);
    arcPtr->bbox[1] = originY + scaleY*(arcPtr->bbox[1] - originY);
    arcPtr->bbox[2] = originX + scaleX*(arcPtr->bbox[2] - originX);
    arcPtr->bbox[3] = originY + scaleY*(arcPtr->bbox[3] - originY);
    ComputeArcBbox(canvas, arcPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateArc --
 *
 *	This function is called to move an arc by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the arc is offset by (xDelta, yDelta), and the
 *	bounding box is updated in the generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateArc(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item that is being moved. */
    double deltaX,		/* Amount by which item is to be moved. */
    double deltaY)
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;

    arcPtr->bbox[0] += deltaX;
    arcPtr->bbox[1] += deltaY;
    arcPtr->bbox[2] += deltaX;
    arcPtr->bbox[3] += deltaY;
    ComputeArcBbox(canvas, arcPtr);
}

/*
 *--------------------------------------------------------------
 *
 * ComputeArcOutline --
 *
 *	This function creates a polygon describing everything in the outline
 *	for an arc except what's in the curved part. For a "pie slice" arc
 *	this is a V-shaped chunk, and for a "chord" arc this is a linear chunk
 *	(with cutaway corners). For "arc" arcs, this stuff isn't relevant.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The information at arcPtr->outlinePtr gets modified, and storage for
 *	arcPtr->outlinePtr may be allocated or freed.
 *
 *--------------------------------------------------------------
 */

static void
ComputeArcOutline(
    Tk_Canvas canvas,		/* Information about overall canvas. */
    ArcItem *arcPtr)		/* Information about arc. */
{
    double sin1, cos1, sin2, cos2, angle, width, halfWidth;
    double boxWidth, boxHeight;
    double vertex[2], corner1[2], corner2[2];
    double *outlinePtr;
    Tk_State state = arcPtr->header.state;

    /*
     * Make sure that the outlinePtr array is large enough to hold either a
     * chord or pie-slice outline.
     */

    if (arcPtr->numOutlinePoints == 0) {
	arcPtr->outlinePtr = ckalloc(26 * sizeof(double));
	arcPtr->numOutlinePoints = 22;
    }
    outlinePtr = arcPtr->outlinePtr;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }

    /*
     * First compute the two points that lie at the centers of the ends of the
     * curved arc segment, which are marked with X's in the figure below:
     *
     *
     *				  * * *
     *			      *          *
     *			   *      * *      *
     *			 *    *         *    *
     *			*   *             *   *
     *			 X *               * X
     *
     * The code is tricky because the arc can be ovular in shape. It computes
     * the position for a unit circle, and then scales to fit the shape of the
     * arc's bounding box.
     *
     * Also, watch out because angles go counter-clockwise like you might
     * expect, but the y-coordinate system is inverted. To handle this, just
     * negate the angles in all the computations.
     */

    boxWidth = arcPtr->bbox[2] - arcPtr->bbox[0];
    boxHeight = arcPtr->bbox[3] - arcPtr->bbox[1];
    angle = -arcPtr->start*PI/180.0;
    sin1 = sin(angle);
    cos1 = cos(angle);
    angle -= arcPtr->extent*PI/180.0;
    sin2 = sin(angle);
    cos2 = cos(angle);
    vertex[0] = (arcPtr->bbox[0] + arcPtr->bbox[2])/2.0;
    vertex[1] = (arcPtr->bbox[1] + arcPtr->bbox[3])/2.0;
    arcPtr->center1[0] = vertex[0] + cos1*boxWidth/2.0;
    arcPtr->center1[1] = vertex[1] + sin1*boxHeight/2.0;
    arcPtr->center2[0] = vertex[0] + cos2*boxWidth/2.0;
    arcPtr->center2[1] = vertex[1] + sin2*boxHeight/2.0;

    /*
     * Next compute the "outermost corners" of the arc, which are marked with
     * X's in the figure below:
     *
     *				  * * *
     *			      *          *
     *			   *      * *      *
     *			 *    *         *    *
     *			X   *             *   X
     *			   *               *
     *
     * The code below is tricky because it has to handle eccentricity in the
     * shape of the oval. The key in the code below is to realize that the
     * slope of the line from arcPtr->center1 to corner1 is (boxWidth*sin1)
     * divided by (boxHeight*cos1), and similarly for arcPtr->center2 and
     * corner2. These formulas can be computed from the formula for the oval.
     */

    width = arcPtr->outline.width;
    if (Canvas(canvas)->currentItemPtr == (Tk_Item *) arcPtr) {
	if (arcPtr->outline.activeWidth>arcPtr->outline.width) {
	    width = arcPtr->outline.activeWidth;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (arcPtr->outline.disabledWidth>arcPtr->outline.width) {
	    width = arcPtr->outline.disabledWidth;
	}
    }
    halfWidth = width/2.0;

    if (((boxWidth*sin1) == 0.0) && ((boxHeight*cos1) == 0.0)) {
	angle = 0.0;
    } else {
	angle = atan2(boxWidth*sin1, boxHeight*cos1);
    }
    corner1[0] = arcPtr->center1[0] + cos(angle)*halfWidth;
    corner1[1] = arcPtr->center1[1] + sin(angle)*halfWidth;
    if (((boxWidth*sin2) == 0.0) && ((boxHeight*cos2) == 0.0)) {
	angle = 0.0;
    } else {
	angle = atan2(boxWidth*sin2, boxHeight*cos2);
    }
    corner2[0] = arcPtr->center2[0] + cos(angle)*halfWidth;
    corner2[1] = arcPtr->center2[1] + sin(angle)*halfWidth;

    /*
     * For a chord outline, generate a six-sided polygon with three points for
     * each end of the chord. The first and third points for each end are butt
     * points generated on either side of the center point. The second point
     * is the corner point.
     */

    if (arcPtr->style == CHORD_STYLE) {
	outlinePtr[0] = outlinePtr[12] = corner1[0];
	outlinePtr[1] = outlinePtr[13] = corner1[1];
	TkGetButtPoints(arcPtr->center2, arcPtr->center1,
		width, 0, outlinePtr+10, outlinePtr+2);
	outlinePtr[4] = arcPtr->center2[0] + outlinePtr[2]
		- arcPtr->center1[0];
	outlinePtr[5] = arcPtr->center2[1] + outlinePtr[3]
		- arcPtr->center1[1];
	outlinePtr[6] = corner2[0];
	outlinePtr[7] = corner2[1];
	outlinePtr[8] = arcPtr->center2[0] + outlinePtr[10]
		- arcPtr->center1[0];
	outlinePtr[9] = arcPtr->center2[1] + outlinePtr[11]
		- arcPtr->center1[1];
    } else if (arcPtr->style == PIESLICE_STYLE) {
	/*
	 * For pie slices, generate two polygons, one for each side of the pie
	 * slice. The first arm has a shape like this, where the center of the
	 * oval is X, arcPtr->center1 is at Y, and corner1 is at Z:
	 *
	 *	 _____________________
	 *	|		      \
	 *	|		       \
	 *	X		     Y  Z
	 *	|		       /
	 *	|_____________________/
	 */

	TkGetButtPoints(arcPtr->center1, vertex, width, 0,
		outlinePtr, outlinePtr+2);
	outlinePtr[4] = arcPtr->center1[0] + outlinePtr[2] - vertex[0];
	outlinePtr[5] = arcPtr->center1[1] + outlinePtr[3] - vertex[1];
	outlinePtr[6] = corner1[0];
	outlinePtr[7] = corner1[1];
	outlinePtr[8] = arcPtr->center1[0] + outlinePtr[0] - vertex[0];
	outlinePtr[9] = arcPtr->center1[1] + outlinePtr[1] - vertex[1];
	outlinePtr[10] = outlinePtr[0];
	outlinePtr[11] = outlinePtr[1];

	/*
	 * The second arm has a shape like this:
	 *
	 *	   ______________________
	 *	  /			  \
	 *	 /			   \
	 *	Z  Y			X  /
	 *	 \			  /
	 *	  \______________________/
	 *
	 * Similar to above X is the center of the oval/circle, Y is
	 * arcPtr->center2, and Z is corner2. The extra jog out to the left of
	 * X is needed in or to produce a butted joint with the first arm; the
	 * corner to the right of X is one of the first two points of the
	 * first arm, depending on extent.
	 */

	TkGetButtPoints(arcPtr->center2, vertex, width, 0,
		outlinePtr+12, outlinePtr+16);
	if ((arcPtr->extent > 180) ||
		((arcPtr->extent < 0) && (arcPtr->extent > -180))) {
	    outlinePtr[14] = outlinePtr[0];
	    outlinePtr[15] = outlinePtr[1];
	} else {
	    outlinePtr[14] = outlinePtr[2];
	    outlinePtr[15] = outlinePtr[3];
	}
	outlinePtr[18] = arcPtr->center2[0] + outlinePtr[16] - vertex[0];
	outlinePtr[19] = arcPtr->center2[1] + outlinePtr[17] - vertex[1];
	outlinePtr[20] = corner2[0];
	outlinePtr[21] = corner2[1];
	outlinePtr[22] = arcPtr->center2[0] + outlinePtr[12] - vertex[0];
	outlinePtr[23] = arcPtr->center2[1] + outlinePtr[13] - vertex[1];
	outlinePtr[24] = outlinePtr[12];
	outlinePtr[25] = outlinePtr[13];
    }
}

/*
 *--------------------------------------------------------------
 *
 * HorizLineToArc --
 *
 *	Determines whether a horizontal line segment intersects a given arc.
 *
 * Results:
 *	The return value is 1 if the given line intersects the infinitely-thin
 *	arc section defined by rx, ry, start, and extent, and 0 otherwise.
 *	Only the perimeter of the arc is checked: interior areas (e.g. chord
 *	or pie-slice) are not checked.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
HorizLineToArc(
    double x1, double x2,	/* X-coords of endpoints of line segment. X1
				 * must be <= x2. */
    double y,			/* Y-coordinate of line segment. */
    double rx, double ry,	/* These x- and y-radii define an oval
				 * centered at the origin. */
    double start, double extent)/* Angles that define extent of arc, in the
				 * standard fashion for this module. */
{
    double tmp, x;
    double tx, ty;		/* Coordinates of intersection point in
				 * transformed coordinate system. */

    /*
     * Compute the x-coordinate of one possible intersection point between the
     * arc and the line. Use a transformed coordinate system where the oval is
     * a unit circle centered at the origin. Then scale back to get actual
     * x-coordinate.
     */

    ty = y/ry;
    tmp = 1 - ty*ty;
    if (tmp < 0) {
	return 0;
    }
    tx = sqrt(tmp);
    x = tx*rx;

    /*
     * Test both intersection points.
     */

    if ((x >= x1) && (x <= x2) && AngleInRange(tx, ty, start, extent)) {
	return 1;
    }
    if ((-x >= x1) && (-x <= x2) && AngleInRange(-tx, ty, start, extent)) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * VertLineToArc --
 *
 *	Determines whether a vertical line segment intersects a given arc.
 *
 * Results:
 *	The return value is 1 if the given line intersects the infinitely-thin
 *	arc section defined by rx, ry, start, and extent, and 0 otherwise.
 *	Only the perimeter of the arc is checked: interior areas (e.g. chord
 *	or pie-slice) are not checked.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
VertLineToArc(
    double x,			/* X-coordinate of line segment. */
    double y1, double y2,	/* Y-coords of endpoints of line segment. Y1
				 * must be <= y2. */
    double rx, double ry,	/* These x- and y-radii define an oval
				 * centered at the origin. */
    double start, double extent)/* Angles that define extent of arc, in the
				 * standard fashion for this module. */
{
    double tmp, y;
    double tx, ty;		/* Coordinates of intersection point in
				 * transformed coordinate system. */

    /*
     * Compute the y-coordinate of one possible intersection point between the
     * arc and the line. Use a transformed coordinate system where the oval is
     * a unit circle centered at the origin. Then scale back to get actual
     * y-coordinate.
     */

    tx = x/rx;
    tmp = 1 - tx*tx;
    if (tmp < 0) {
	return 0;
    }
    ty = sqrt(tmp);
    y = ty*ry;

    /*
     * Test both intersection points.
     */

    if ((y > y1) && (y < y2) && AngleInRange(tx, ty, start, extent)) {
	return 1;
    }
    if ((-y > y1) && (-y < y2) && AngleInRange(tx, -ty, start, extent)) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * AngleInRange --
 *
 *	Determine whether the angle from the origin to a given point is within
 *	a given range.
 *
 * Results:
 *	The return value is 1 if the angle from (0,0) to (x,y) is in the range
 *	given by start and extent, where angles are interpreted in the
 *	standard way for ovals (meaning backwards from normal interpretation).
 *	Otherwise the return value is 0.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
AngleInRange(
    double x, double y,		/* Coordinate of point; angle measured from
				 * origin to here, relative to x-axis. */
    double start,		/* First angle, degrees, >=0, <=360. */
    double extent)		/* Size of arc in degrees >=-360, <=360. */
{
    double diff;

    if ((x == 0.0) && (y == 0.0)) {
	return 1;
    }
    diff = -atan2(y, x);
    diff = diff*(180.0/PI) - start;
    while (diff > 360.0) {
	diff -= 360.0;
    }
    while (diff < 0.0) {
	diff += 360.0;
    }
    if (extent >= 0) {
	return diff <= extent;
    }
    return (diff-360.0) >= extent;
}

/*
 *--------------------------------------------------------------
 *
 * ArcToPostscript --
 *
 *	This function is called to generate Postscript for arc items.
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
ArcToPostscript(
    Tcl_Interp *interp,		/* Leave Postscript or error message here. */
    Tk_Canvas canvas,		/* Information about overall canvas. */
    Tk_Item *itemPtr,		/* Item for which Postscript is wanted. */
    int prepass)		/* 1 means this is a prepass to collect font
				 * information; 0 means final Postscript is
				 * being created. */
{
    ArcItem *arcPtr = (ArcItem *) itemPtr;
    double y1, y2, ang1, ang2;
    XColor *color;
    Pixmap stipple;
    XColor *fillColor;
    Pixmap fillStipple;
    Tk_State state = itemPtr->state;
    Tcl_Obj *psObj;
    Tcl_InterpState interpState;

    y1 = Tk_CanvasPsY(canvas, arcPtr->bbox[1]);
    y2 = Tk_CanvasPsY(canvas, arcPtr->bbox[3]);
    ang1 = arcPtr->start;
    ang2 = ang1 + arcPtr->extent;
    if (ang2 < ang1) {
	ang1 = ang2;
	ang2 = arcPtr->start;
    }

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    color = arcPtr->outline.color;
    stipple = arcPtr->outline.stipple;
    fillColor = arcPtr->fillColor;
    fillStipple = arcPtr->fillStipple;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (arcPtr->outline.activeColor!=NULL) {
	    color = arcPtr->outline.activeColor;
	}
	if (arcPtr->outline.activeStipple!=None) {
	    stipple = arcPtr->outline.activeStipple;
	}
	if (arcPtr->activeFillColor!=NULL) {
	    fillColor = arcPtr->activeFillColor;
	}
	if (arcPtr->activeFillStipple!=None) {
	    fillStipple = arcPtr->activeFillStipple;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (arcPtr->outline.disabledColor!=NULL) {
	    color = arcPtr->outline.disabledColor;
	}
	if (arcPtr->outline.disabledStipple!=None) {
	    stipple = arcPtr->outline.disabledStipple;
	}
	if (arcPtr->disabledFillColor!=NULL) {
	    fillColor = arcPtr->disabledFillColor;
	}
	if (arcPtr->disabledFillStipple!=None) {
	    fillStipple = arcPtr->disabledFillStipple;
	}
    }

    /*
     * Make our working space.
     */

    psObj = Tcl_NewObj();
    interpState = Tcl_SaveInterpState(interp, TCL_OK);

    /*
     * If the arc is filled, output Postscript for the interior region of the
     * arc.
     */

    if (arcPtr->fillGC != None) {
	Tcl_AppendPrintfToObj(psObj,
		"matrix currentmatrix\n"
		"%.15g %.15g translate %.15g %.15g scale\n",
		(arcPtr->bbox[0] + arcPtr->bbox[2])/2, (y1 + y2)/2,
		(arcPtr->bbox[2] - arcPtr->bbox[0])/2, (y1 - y2)/2);

	if (arcPtr->style != CHORD_STYLE) {
	    Tcl_AppendToObj(psObj, "0 0 moveto ", -1);
	}
	Tcl_AppendPrintfToObj(psObj,
		"0 0 1 %.15g %.15g arc closepath\nsetmatrix\n",
		ang1, ang2);

	Tcl_ResetResult(interp);
	if (Tk_CanvasPsColor(interp, canvas, fillColor) != TCL_OK) {
	    goto error;
	}
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	if (fillStipple != None) {
	    Tcl_AppendToObj(psObj, "clip ", -1);

	    Tcl_ResetResult(interp);
	    if (Tk_CanvasPsStipple(interp, canvas, fillStipple) != TCL_OK) {
		goto error;
	    }
	    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	    if (arcPtr->outline.gc != None) {
		Tcl_AppendToObj(psObj, "grestore gsave\n", -1);
	    }
	} else {
	    Tcl_AppendToObj(psObj, "fill\n", -1);
	}
    }

    /*
     * If there's an outline for the arc, draw it.
     */

    if (arcPtr->outline.gc != None) {
	Tcl_AppendPrintfToObj(psObj,
		"matrix currentmatrix\n"
		"%.15g %.15g translate %.15g %.15g scale\n",
		(arcPtr->bbox[0] + arcPtr->bbox[2])/2, (y1 + y2)/2,
		(arcPtr->bbox[2] - arcPtr->bbox[0])/2, (y1 - y2)/2);
	Tcl_AppendPrintfToObj(psObj,
		"0 0 1 %.15g %.15g arc\nsetmatrix\n0 setlinecap\n",
		ang1, ang2);

	Tcl_ResetResult(interp);
	if (Tk_CanvasPsOutline(canvas, itemPtr, &arcPtr->outline) != TCL_OK) {
	    goto error;
	}
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	if (arcPtr->style != ARC_STYLE) {
	    Tcl_AppendToObj(psObj, "grestore gsave\n", -1);

	    Tcl_ResetResult(interp);
	    if (arcPtr->style == CHORD_STYLE) {
		Tk_CanvasPsPath(interp, canvas, arcPtr->outlinePtr,
			CHORD_OUTLINE_PTS);
	    } else {
		Tk_CanvasPsPath(interp, canvas, arcPtr->outlinePtr,
			PIE_OUTLINE1_PTS);
		if (Tk_CanvasPsColor(interp, canvas, color) != TCL_OK) {
		    goto error;
		}
		Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

		if (stipple != None) {
		    Tcl_AppendToObj(psObj, "clip ", -1);

		    Tcl_ResetResult(interp);
		    if (Tk_CanvasPsStipple(interp, canvas, stipple) !=TCL_OK){
			goto error;
		    }
		    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
		} else {
		    Tcl_AppendToObj(psObj, "fill\n", -1);
		}
		Tcl_AppendToObj(psObj, "grestore gsave\n", -1);

		Tcl_ResetResult(interp);
		Tk_CanvasPsPath(interp, canvas,
			arcPtr->outlinePtr + 2*PIE_OUTLINE1_PTS,
			PIE_OUTLINE2_PTS);
	    }
	    if (Tk_CanvasPsColor(interp, canvas, color) != TCL_OK) {
		goto error;
	    }
	    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	    if (stipple != None) {
		Tcl_AppendToObj(psObj, "clip ", -1);

		Tcl_ResetResult(interp);
		if (Tk_CanvasPsStipple(interp, canvas, stipple) != TCL_OK) {
		    goto error;
		}
		Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
	    } else {
		Tcl_AppendToObj(psObj, "fill\n", -1);
	    }
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
 *--------------------------------------------------------------
 *
 * StyleParseProc --
 *
 *	This function is invoked during option processing to handle the
 *	"-style" option.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The state for a given item gets replaced by the state indicated in the
 *	value argument.
 *
 *--------------------------------------------------------------
 */

static int
StyleParseProc(
    ClientData clientData,	/* some flags.*/
    Tcl_Interp *interp,		/* Used for reporting errors. */
    Tk_Window tkwin,		/* Window containing canvas widget. */
    const char *value,		/* Value of option. */
    char *widgRec,		/* Pointer to record for item. */
    int offset)			/* Offset into item. */
{
    int c;
    size_t length;

    register Style *stylePtr = (Style *) (widgRec + offset);

    if (value == NULL || *value == 0) {
	*stylePtr = PIESLICE_STYLE;
	return TCL_OK;
    }

    c = value[0];
    length = strlen(value);

    if ((c == 'a') && (strncmp(value, "arc", length) == 0)) {
	*stylePtr = ARC_STYLE;
	return TCL_OK;
    }
    if ((c == 'c') && (strncmp(value, "chord", length) == 0)) {
	*stylePtr = CHORD_STYLE;
	return TCL_OK;
    }
    if ((c == 'p') && (strncmp(value, "pieslice", length) == 0)) {
	*stylePtr = PIESLICE_STYLE;
	return TCL_OK;
    }

    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "bad -style option \"%s\": must be arc, chord, or pieslice",
	    value));
    Tcl_SetErrorCode(interp, "TK", "CANVAS", "ARC_STYLE", NULL);
    *stylePtr = PIESLICE_STYLE;
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * StylePrintProc --
 *
 *	This function is invoked by the Tk configuration code to produce a
 *	printable string for the "-style" configuration option.
 *
 * Results:
 *	The return value is a string describing the state for the item
 *	referred to by "widgRec". In addition, *freeProcPtr is filled in with
 *	the address of a function to call to free the result string when it's
 *	no longer needed (or NULL to indicate that the string doesn't need to
 *	be freed).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static const char *
StylePrintProc(
    ClientData clientData,	/* Ignored. */
    Tk_Window tkwin,		/* Ignored. */
    char *widgRec,		/* Pointer to record for item. */
    int offset,			/* Offset into item. */
    Tcl_FreeProc **freeProcPtr)	/* Pointer to variable to fill in with
				 * information about how to reclaim storage
				 * for return string. */
{
    register Style *stylePtr = (Style *) (widgRec + offset);

    if (*stylePtr == ARC_STYLE) {
	return "arc";
    } else if (*stylePtr == CHORD_STYLE) {
	return "chord";
    } else {
	return "pieslice";
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
