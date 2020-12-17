/*
 * tkCanvText.c --
 *
 *	This file implements text items for canvas widgets.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkCanvas.h"
#include "default.h"

/*
 * The structure below defines the record for each text item.
 */

typedef struct TextItem {
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types. MUST BE FIRST IN STRUCTURE. */
    Tk_CanvasTextInfo *textInfoPtr;
				/* Pointer to a structure containing
				 * information about the selection and
				 * insertion cursor. The structure is owned by
				 * (and shared with) the generic canvas
				 * code. */
    /*
     * Fields that are set by widget commands other than "configure".
     */

    double x, y;		/* Positioning point for text. */
    int insertPos;		/* Character index of character just before
				 * which the insertion cursor is displayed. */

    /*
     * Configuration settings that are updated by Tk_ConfigureWidget.
     */

    Tk_Anchor anchor;		/* Where to anchor text relative to (x,y). */
    Tk_TSOffset tsoffset;
    XColor *color;		/* Color for text. */
    XColor *activeColor;	/* Color for text. */
    XColor *disabledColor;	/* Color for text. */
    Tk_Font tkfont;		/* Font for drawing text. */
    Tk_Justify justify;		/* Justification mode for text. */
    Pixmap stipple;		/* Stipple bitmap for text, or None. */
    Pixmap activeStipple;	/* Stipple bitmap for text, or None. */
    Pixmap disabledStipple;	/* Stipple bitmap for text, or None. */
    char *text;			/* Text for item (malloc-ed). */
    int width;			/* Width of lines for word-wrap, pixels. Zero
				 * means no word-wrap. */
    int underline;		/* Index of character to put underline beneath
				 * or -1 for no underlining. */
    double angle;		/* What angle, in degrees, to draw the text
				 * at. */

    /*
     * Fields whose values are derived from the current values of the
     * configuration settings above.
     */

    int numChars;		/* Length of text in characters. */
    int numBytes;		/* Length of text in bytes. */
    Tk_TextLayout textLayout;	/* Cached text layout information. */
    int actualWidth;		/* Width of text as computed. Used to make
				 * selections of wrapped text display
				 * right. */
    double drawOrigin[2];	/* Where we start drawing from. */
    GC gc;			/* Graphics context for drawing text. */
    GC selTextGC;		/* Graphics context for selected text. */
    GC cursorOffGC;		/* If not None, this gives a graphics context
				 * to use to draw the insertion cursor when
				 * it's off. Used if the selection and
				 * insertion cursor colors are the same. */
    double sine;		/* Sine of angle field. */
    double cosine;		/* Cosine of angle field. */
} TextItem;

/*
 * Information used for parsing configuration specs:
 */

static const Tk_CustomOption stateOption = {
    TkStateParseProc, TkStatePrintProc, INT2PTR(2)
};
static const Tk_CustomOption tagsOption = {
    Tk_CanvasTagsParseProc, Tk_CanvasTagsPrintProc, NULL
};
static const Tk_CustomOption offsetOption = {
    TkOffsetParseProc, TkOffsetPrintProc, INT2PTR(TK_OFFSET_RELATIVE)
};

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-activefill", NULL, NULL,
	NULL, Tk_Offset(TextItem, activeColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-activestipple", NULL, NULL,
	NULL, Tk_Offset(TextItem, activeStipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_ANCHOR, "-anchor", NULL, NULL,
	"center", Tk_Offset(TextItem, anchor), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_DOUBLE, "-angle", NULL, NULL,
	"0.0", Tk_Offset(TextItem, angle), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_COLOR, "-disabledfill", NULL, NULL,
	NULL, Tk_Offset(TextItem, disabledColor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_BITMAP, "-disabledstipple", NULL, NULL,
	NULL, Tk_Offset(TextItem, disabledStipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-fill", NULL, NULL,
	DEF_CANVITEM_OUTLINE, Tk_Offset(TextItem, color), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_FONT, "-font", NULL, NULL,
	DEF_CANVTEXT_FONT, Tk_Offset(TextItem, tkfont), 0, NULL},
    {TK_CONFIG_JUSTIFY, "-justify", NULL, NULL,
	"left", Tk_Offset(TextItem, justify), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_CUSTOM, "-offset", NULL, NULL,
	"0,0", Tk_Offset(TextItem, tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_CUSTOM, "-state", NULL, NULL,
	NULL, Tk_Offset(Tk_Item, state), TK_CONFIG_NULL_OK, &stateOption},
    {TK_CONFIG_BITMAP, "-stipple", NULL, NULL,
	NULL, Tk_Offset(TextItem, stipple), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_CUSTOM, "-tags", NULL, NULL,
	NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_STRING, "-text", NULL, NULL,
	"", Tk_Offset(TextItem, text), 0, NULL},
    {TK_CONFIG_INT, "-underline", NULL, NULL,
	"-1", Tk_Offset(TextItem, underline), 0, NULL},
    {TK_CONFIG_PIXELS, "-width", NULL, NULL,
	"0", Tk_Offset(TextItem, width), TK_CONFIG_DONT_SET_DEFAULT, NULL},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};

/*
 * Prototypes for functions defined in this file:
 */

static void		ComputeTextBbox(Tk_Canvas canvas, TextItem *textPtr);
static int		ConfigureText(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int argc,
			    Tcl_Obj *const objv[], int flags);
static int		CreateText(Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int argc, Tcl_Obj *const objv[]);
static void		DeleteText(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display);
static void		DisplayCanvText(Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height);
static int		GetSelText(Tk_Canvas canvas,
			    Tk_Item *itemPtr, int offset, char *buffer,
			    int maxBytes);
static int		GetTextIndex(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr,
			    Tcl_Obj *obj, int *indexPtr);
static void		ScaleText(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY);
static void		SetTextCursor(Tk_Canvas canvas,
			    Tk_Item *itemPtr, int index);
static int		TextCoords(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr,
			    int argc, Tcl_Obj *const objv[]);
static void		TextDeleteChars(Tk_Canvas canvas,
			    Tk_Item *itemPtr, int first, int last);
static void		TextInsert(Tk_Canvas canvas,
			    Tk_Item *itemPtr, int beforeThis, Tcl_Obj *obj);
static int		TextToArea(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *rectPtr);
static double		TextToPoint(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *pointPtr);
static int		TextToPostscript(Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass);
static void		TranslateText(Tk_Canvas canvas,
			    Tk_Item *itemPtr, double deltaX, double deltaY);

/*
 * The structures below defines the rectangle and oval item types by means of
 * functions that can be invoked by generic item code.
 */

Tk_ItemType tkTextType = {
    "text",			/* name */
    sizeof(TextItem),		/* itemSize */
    CreateText,			/* createProc */
    configSpecs,		/* configSpecs */
    ConfigureText,		/* configureProc */
    TextCoords,			/* coordProc */
    DeleteText,			/* deleteProc */
    DisplayCanvText,		/* displayProc */
    TK_CONFIG_OBJS,		/* flags */
    TextToPoint,		/* pointProc */
    TextToArea,			/* areaProc */
    TextToPostscript,		/* postscriptProc */
    ScaleText,			/* scaleProc */
    TranslateText,		/* translateProc */
    GetTextIndex,		/* indexProc */
    SetTextCursor,		/* icursorProc */
    GetSelText,			/* selectionProc */
    TextInsert,			/* insertProc */
    TextDeleteChars,		/* dTextProc */
    NULL,			/* nextPtr */
    NULL, 0, NULL, NULL
};

#define ROUND(d) ((int) floor((d) + 0.5))

/*
 *--------------------------------------------------------------
 *
 * CreateText --
 *
 *	This function is invoked to create a new text item in a canvas.
 *
 * Results:
 *	A standard Tcl return value. If an error occurred in creating the item
 *	then an error message is left in the interp's result; in this case
 *	itemPtr is left uninitialized so it can be safely freed by the caller.
 *
 * Side effects:
 *	A new text item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateText(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Canvas canvas,		/* Canvas to hold new item. */
    Tk_Item *itemPtr,		/* Record to hold new item; header has been
				 * initialized by caller. */
    int objc,			/* Number of arguments in objv. */
    Tcl_Obj *const objv[])	/* Arguments describing rectangle. */
{
    TextItem *textPtr = (TextItem *) itemPtr;
    int i;

    if (objc == 0) {
	Tcl_Panic("canvas did not pass any coords");
    }

    /*
     * Carry out initialization that is needed in order to clean up after
     * errors during the the remainder of this function.
     */

    textPtr->textInfoPtr = Tk_CanvasGetTextInfo(canvas);

    textPtr->insertPos	= 0;

    textPtr->anchor	= TK_ANCHOR_CENTER;
    textPtr->tsoffset.flags = 0;
    textPtr->tsoffset.xoffset = 0;
    textPtr->tsoffset.yoffset = 0;
    textPtr->color	= NULL;
    textPtr->activeColor = NULL;
    textPtr->disabledColor = NULL;
    textPtr->tkfont	= NULL;
    textPtr->justify	= TK_JUSTIFY_LEFT;
    textPtr->stipple	= None;
    textPtr->activeStipple = None;
    textPtr->disabledStipple = None;
    textPtr->text	= NULL;
    textPtr->width	= 0;
    textPtr->underline	= -1;
    textPtr->angle	= 0.0;

    textPtr->numChars	= 0;
    textPtr->numBytes	= 0;
    textPtr->textLayout = NULL;
    textPtr->actualWidth = 0;
    textPtr->drawOrigin[0] = textPtr->drawOrigin[1] = 0.0;
    textPtr->gc		= NULL;
    textPtr->selTextGC	= NULL;
    textPtr->cursorOffGC = NULL;
    textPtr->sine	= 0.0;
    textPtr->cosine	= 1.0;

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
    if ((TextCoords(interp, canvas, itemPtr, i, objv) != TCL_OK)) {
	goto error;
    }
    if (ConfigureText(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
	return TCL_OK;
    }

  error:
    DeleteText(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * TextCoords --
 *
 *	This function is invoked to process the "coords" widget command on
 *	text items. See the user documentation for details on what it does.
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
TextCoords(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item whose coordinates are to be read or
				 * modified. */
    int objc,			/* Number of coordinates supplied in objv. */
    Tcl_Obj *const objv[])	/* Array of coordinates: x1, y1, x2, y2, ... */
{
    TextItem *textPtr = (TextItem *) itemPtr;

    if (objc == 0) {
	Tcl_Obj *obj = Tcl_NewObj();
	Tcl_Obj *subobj = Tcl_NewDoubleObj(textPtr->x);

	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(textPtr->y);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	Tcl_SetObjResult(interp, obj);
	return TCL_OK;
    } else if (objc > 2) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"wrong # coordinates: expected 0 or 2, got %d", objc));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "TEXT", NULL);
	return TCL_ERROR;
    }

    if (objc == 1) {
	if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		(Tcl_Obj ***) &objv) != TCL_OK) {
	    return TCL_ERROR;
	} else if (objc != 2) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "wrong # coordinates: expected 2, got %d", objc));
	    Tcl_SetErrorCode(interp, "TK", "CANVAS", "COORDS", "TEXT", NULL);
	    return TCL_ERROR;
	}
    }
    if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0],
		&textPtr->x) != TCL_OK)
	    || (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
		&textPtr->y) != TCL_OK)) {
	return TCL_ERROR;
    }
    ComputeTextBbox(canvas, textPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureText --
 *
 *	This function is invoked to configure various aspects of a text item,
 *	such as its border and background colors.
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
ConfigureText(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr,		/* Rectangle item to reconfigure. */
    int objc,			/* Number of elements in objv. */
    Tcl_Obj *const objv[],	/* Arguments describing things to configure. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    TextItem *textPtr = (TextItem *) itemPtr;
    XGCValues gcValues;
    GC newGC, newSelGC;
    unsigned long mask;
    Tk_Window tkwin;
    Tk_CanvasTextInfo *textInfoPtr = textPtr->textInfoPtr;
    XColor *selBgColorPtr;
    XColor *color;
    Pixmap stipple;
    Tk_State state;

    tkwin = Tk_CanvasTkwin(canvas);
    if (TCL_OK != Tk_ConfigureWidget(interp, tkwin, configSpecs, objc,
	    (const char **) objv, (char *) textPtr, flags|TK_CONFIG_OBJS)) {
	return TCL_ERROR;
    }

    /*
     * A few of the options require additional processing, such as graphics
     * contexts.
     */

    state = itemPtr->state;

    if (textPtr->activeColor != NULL || textPtr->activeStipple != None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }

    color = textPtr->color;
    stipple = textPtr->stipple;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (textPtr->activeColor != NULL) {
	    color = textPtr->activeColor;
	}
	if (textPtr->activeStipple != None) {
	    stipple = textPtr->activeStipple;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (textPtr->disabledColor != NULL) {
	    color = textPtr->disabledColor;
	}
	if (textPtr->disabledStipple != None) {
	    stipple = textPtr->disabledStipple;
	}
    }

    newGC = newSelGC = NULL;
    if (textPtr->tkfont != NULL) {
	gcValues.font = Tk_FontId(textPtr->tkfont);
	mask = GCFont;
	if (color != NULL) {
	    gcValues.foreground = color->pixel;
	    mask |= GCForeground;
	    if (stipple != None) {
		gcValues.stipple = stipple;
		gcValues.fill_style = FillStippled;
		mask |= GCStipple|GCFillStyle;
	    }
	    newGC = Tk_GetGC(tkwin, mask, &gcValues);
	}
	mask &= ~(GCTile|GCFillStyle|GCStipple);
	if (stipple != None) {
	    gcValues.stipple = stipple;
	    gcValues.fill_style = FillStippled;
	    mask |= GCStipple|GCFillStyle;
	}
	if (textInfoPtr->selFgColorPtr != NULL) {
	    gcValues.foreground = textInfoPtr->selFgColorPtr->pixel;
	}
	newSelGC = Tk_GetGC(tkwin, mask|GCForeground, &gcValues);
    }
    if (textPtr->gc != NULL) {
	Tk_FreeGC(Tk_Display(tkwin), textPtr->gc);
    }
    textPtr->gc = newGC;
    if (textPtr->selTextGC != NULL) {
	Tk_FreeGC(Tk_Display(tkwin), textPtr->selTextGC);
    }
    textPtr->selTextGC = newSelGC;

    selBgColorPtr = Tk_3DBorderColor(textInfoPtr->selBorder);
    if (Tk_3DBorderColor(textInfoPtr->insertBorder)->pixel
	    == selBgColorPtr->pixel) {
	if (selBgColorPtr->pixel == BlackPixelOfScreen(Tk_Screen(tkwin))) {
	    gcValues.foreground = WhitePixelOfScreen(Tk_Screen(tkwin));
	} else {
	    gcValues.foreground = BlackPixelOfScreen(Tk_Screen(tkwin));
	}
	newGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
    } else {
	newGC = NULL;
    }
    if (textPtr->cursorOffGC != NULL) {
	Tk_FreeGC(Tk_Display(tkwin), textPtr->cursorOffGC);
    }
    textPtr->cursorOffGC = newGC;

    /*
     * If the text was changed, move the selection and insertion indices to
     * keep them inside the item.
     */

    textPtr->numBytes = strlen(textPtr->text);
    textPtr->numChars = Tcl_NumUtfChars(textPtr->text, textPtr->numBytes);
    if (textInfoPtr->selItemPtr == itemPtr) {

	if (textInfoPtr->selectFirst >= textPtr->numChars) {
	    textInfoPtr->selItemPtr = NULL;
	} else {
	    if (textInfoPtr->selectLast >= textPtr->numChars) {
		textInfoPtr->selectLast = textPtr->numChars - 1;
	    }
	    if ((textInfoPtr->anchorItemPtr == itemPtr)
		    && (textInfoPtr->selectAnchor >= textPtr->numChars)) {
		textInfoPtr->selectAnchor = textPtr->numChars - 1;
	    }
	}
    }
    if (textPtr->insertPos >= textPtr->numChars) {
	textPtr->insertPos = textPtr->numChars;
    }

    /*
     * Restrict so that 0.0 <= angle < 360.0, and then recompute the cached
     * sine and cosine of the angle. Note that fmod() can produce negative
     * results, and we try to avoid negative zero as well.
     */

    textPtr->angle = fmod(textPtr->angle, 360.0);
    if (textPtr->angle < 0.0) {
	textPtr->angle += 360.0;
    }
    if (textPtr->angle == 0.0) {
	textPtr->angle = 0.0;
    }
    textPtr->sine = sin(textPtr->angle * PI/180.0);
    textPtr->cosine = cos(textPtr->angle * PI/180.0);

    ComputeTextBbox(canvas, textPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteText --
 *
 *	This function is called to clean up the data structure associated with
 *	a text item.
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
DeleteText(
    Tk_Canvas canvas,		/* Info about overall canvas widget. */
    Tk_Item *itemPtr,		/* Item that is being deleted. */
    Display *display)		/* Display containing window for canvas. */
{
    TextItem *textPtr = (TextItem *) itemPtr;

    if (textPtr->color != NULL) {
	Tk_FreeColor(textPtr->color);
    }
    if (textPtr->activeColor != NULL) {
	Tk_FreeColor(textPtr->activeColor);
    }
    if (textPtr->disabledColor != NULL) {
	Tk_FreeColor(textPtr->disabledColor);
    }
    Tk_FreeFont(textPtr->tkfont);
    if (textPtr->stipple != None) {
	Tk_FreeBitmap(display, textPtr->stipple);
    }
    if (textPtr->activeStipple != None) {
	Tk_FreeBitmap(display, textPtr->activeStipple);
    }
    if (textPtr->disabledStipple != None) {
	Tk_FreeBitmap(display, textPtr->disabledStipple);
    }
    if (textPtr->text != NULL) {
	ckfree(textPtr->text);
    }

    Tk_FreeTextLayout(textPtr->textLayout);
    if (textPtr->gc != NULL) {
	Tk_FreeGC(display, textPtr->gc);
    }
    if (textPtr->selTextGC != NULL) {
	Tk_FreeGC(display, textPtr->selTextGC);
    }
    if (textPtr->cursorOffGC != NULL) {
	Tk_FreeGC(display, textPtr->cursorOffGC);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeTextBbox --
 *
 *	This function is invoked to compute the bounding box of all the pixels
 *	that may be drawn as part of a text item. In addition, it recomputes
 *	all of the geometry information used to display a text item or check
 *	for mouse hits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header for itemPtr,
 *	and the linePtr structure is regenerated for itemPtr.
 *
 *--------------------------------------------------------------
 */

static void
ComputeTextBbox(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    TextItem *textPtr)		/* Item whose bbox is to be recomputed. */
{
    Tk_CanvasTextInfo *textInfoPtr;
    int leftX, topY, width, height, fudge, i;
    Tk_State state = textPtr->header.state;
    double x[4], y[4], dx[4], dy[4], sinA, cosA, tmp;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }

    Tk_FreeTextLayout(textPtr->textLayout);
    textPtr->textLayout = Tk_ComputeTextLayout(textPtr->tkfont,
	    textPtr->text, textPtr->numChars, textPtr->width,
	    textPtr->justify, 0, &width, &height);

    if (state == TK_STATE_HIDDEN || textPtr->color == NULL) {
	width = height = 0;
    }

    /*
     * Use overall geometry information to compute the top-left corner of the
     * bounding box for the text item.
     */

    leftX = ROUND(textPtr->x);
    topY = ROUND(textPtr->y);
    for (i=0 ; i<4 ; i++) {
	dx[i] = dy[i] = 0.0;
    }
    switch (textPtr->anchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_N:
    case TK_ANCHOR_NE:
	break;

    case TK_ANCHOR_W:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_E:
	topY -= height / 2;
	for (i=0 ; i<4 ; i++) {
	    dy[i] = -height / 2;
	}
	break;

    case TK_ANCHOR_SW:
    case TK_ANCHOR_S:
    case TK_ANCHOR_SE:
	topY -= height;
	for (i=0 ; i<4 ; i++) {
	    dy[i] = -height;
	}
	break;
    }
    switch (textPtr->anchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_W:
    case TK_ANCHOR_SW:
	break;

    case TK_ANCHOR_N:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_S:
	leftX -= width / 2;
	for (i=0 ; i<4 ; i++) {
	    dx[i] = -width / 2;
	}
	break;

    case TK_ANCHOR_NE:
    case TK_ANCHOR_E:
    case TK_ANCHOR_SE:
	leftX -= width;
	for (i=0 ; i<4 ; i++) {
	    dx[i] = -width;
	}
	break;
    }

    textPtr->actualWidth = width;

    sinA = textPtr->sine;
    cosA = textPtr->cosine;
    textPtr->drawOrigin[0] = textPtr->x + dx[0]*cosA + dy[0]*sinA;
    textPtr->drawOrigin[1] = textPtr->y + dy[0]*cosA - dx[0]*sinA;

    /*
     * Last of all, update the bounding box for the item. The item's bounding
     * box includes the bounding box of all its lines, plus an extra fudge
     * factor for the cursor border (which could potentially be quite large).
     */

    textInfoPtr = textPtr->textInfoPtr;
    fudge = (textInfoPtr->insertWidth + 1) / 2;
    if (textInfoPtr->selBorderWidth > fudge) {
	fudge = textInfoPtr->selBorderWidth;
    }

    /*
     * Apply the rotation before computing the bounding box.
     */

    dx[0] -= fudge;
    dx[1] += width + fudge;
    dx[2] += width + fudge;
    dy[2] += height;
    dx[3] -= fudge;
    dy[3] += height;
    for (i=0 ; i<4 ; i++) {
	x[i] = textPtr->x + dx[i] * cosA + dy[i] * sinA;
	y[i] = textPtr->y + dy[i] * cosA - dx[i] * sinA;
    }

    /*
     * Convert to a rectilinear bounding box.
     */

    for (i=1,tmp=x[0] ; i<4 ; i++) {
	if (x[i] < tmp) {
	    tmp = x[i];
	}
    }
    textPtr->header.x1 = ROUND(tmp);
    for (i=1,tmp=y[0] ; i<4 ; i++) {
	if (y[i] < tmp) {
	    tmp = y[i];
	}
    }
    textPtr->header.y1 = ROUND(tmp);
    for (i=1,tmp=x[0] ; i<4 ; i++) {
	if (x[i] > tmp) {
	    tmp = x[i];
	}
    }
    textPtr->header.x2 = ROUND(tmp);
    for (i=1,tmp=y[0] ; i<4 ; i++) {
	if (y[i] > tmp) {
	    tmp = y[i];
	}
    }
    textPtr->header.y2 = ROUND(tmp);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayCanvText --
 *
 *	This function is invoked to draw a text item in a given drawable.
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
DisplayCanvText(
    Tk_Canvas canvas,		/* Canvas that contains item. */
    Tk_Item *itemPtr,		/* Item to be displayed. */
    Display *display,		/* Display on which to draw item. */
    Drawable drawable,		/* Pixmap or window in which to draw item. */
    int x, int y, int width, int height)
				/* Describes region of canvas that must be
				 * redisplayed (not used). */
{
    TextItem *textPtr;
    Tk_CanvasTextInfo *textInfoPtr;
    int selFirstChar, selLastChar;
    short drawableX, drawableY;
    Pixmap stipple;
    Tk_State state = itemPtr->state;

    textPtr = (TextItem *) itemPtr;
    textInfoPtr = textPtr->textInfoPtr;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    stipple = textPtr->stipple;
    if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (textPtr->activeStipple != None) {
	    stipple = textPtr->activeStipple;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (textPtr->disabledStipple != None) {
	    stipple = textPtr->disabledStipple;
	}
    }

    if (textPtr->gc == NULL) {
	return;
    }

    /*
     * If we're stippling, then modify the stipple offset in the GC. Be sure
     * to reset the offset when done, since the GC is supposed to be
     * read-only.
     */

    if (stipple != None) {
	Tk_CanvasSetOffset(canvas, textPtr->gc, &textPtr->tsoffset);
    }

    selFirstChar = -1;
    selLastChar = 0;		/* lint. */
    Tk_CanvasDrawableCoords(canvas, textPtr->drawOrigin[0],
	    textPtr->drawOrigin[1], &drawableX, &drawableY);

    if (textInfoPtr->selItemPtr == itemPtr) {
	selFirstChar = textInfoPtr->selectFirst;
	selLastChar = textInfoPtr->selectLast;
	if (selLastChar > textPtr->numChars) {
	    selLastChar = textPtr->numChars - 1;
	}
	if ((selFirstChar >= 0) && (selFirstChar <= selLastChar)) {
	    int xFirst, yFirst, hFirst;
	    int xLast, yLast, wLast;

	    /*
	     * Draw a special background under the selection.
	     */

	    Tk_CharBbox(textPtr->textLayout, selFirstChar, &xFirst, &yFirst,
		    NULL, &hFirst);
	    Tk_CharBbox(textPtr->textLayout, selLastChar, &xLast, &yLast,
		    &wLast, NULL);

	    /*
	     * If the selection spans the end of this line, then display
	     * selection background all the way to the end of the line.
	     * However, for the last line we only want to display up to the
	     * last character, not the end of the line.
	     */

	    x = xFirst;
	    height = hFirst;
	    for (y = yFirst ; y <= yLast; y += height) {
		int dx1, dy1, dx2, dy2;
		double s = textPtr->sine, c = textPtr->cosine;
		XPoint points[4];

		if (y == yLast) {
		    width = xLast + wLast - x;
		} else {
		    width = textPtr->actualWidth - x;
		}
		dx1 = x - textInfoPtr->selBorderWidth;
		dy1 = y;
		dx2 = width + 2 * textInfoPtr->selBorderWidth;
		dy2 = height;
		points[0].x = (short)(drawableX + dx1*c + dy1*s);
		points[0].y = (short)(drawableY + dy1*c - dx1*s);
		points[1].x = (short)(drawableX + (dx1+dx2)*c + dy1*s);
		points[1].y = (short)(drawableY + dy1*c - (dx1+dx2)*s);
		points[2].x = (short)(drawableX + (dx1+dx2)*c + (dy1+dy2)*s);
		points[2].y = (short)(drawableY + (dy1+dy2)*c - (dx1+dx2)*s);
		points[3].x = (short)(drawableX + dx1*c + (dy1+dy2)*s);
		points[3].y = (short)(drawableY + (dy1+dy2)*c - dx1*s);
		Tk_Fill3DPolygon(Tk_CanvasTkwin(canvas), drawable,
			textInfoPtr->selBorder, points, 4,
			textInfoPtr->selBorderWidth, TK_RELIEF_RAISED);
		x = 0;
	    }
	}
    }

    /*
     * If the insertion point should be displayed, then draw a special
     * background for the cursor before drawing the text. Note: if we're the
     * cursor item but the cursor is turned off, then redraw background over
     * the area of the cursor. This guarantees that the selection won't make
     * the cursor invisible on mono displays, where both are drawn in the same
     * color.
     */

    if ((textInfoPtr->focusItemPtr == itemPtr) && (textInfoPtr->gotFocus)) {
	if (Tk_CharBbox(textPtr->textLayout, textPtr->insertPos,
		&x, &y, NULL, &height)) {
	    int dx1, dy1, dx2, dy2;
	    double s = textPtr->sine, c = textPtr->cosine;
	    XPoint points[4];

	    dx1 = x - (textInfoPtr->insertWidth / 2);
	    dy1 = y;
	    dx2 = textInfoPtr->insertWidth;
	    dy2 = height;
	    points[0].x = (short)(drawableX + dx1*c + dy1*s);
	    points[0].y = (short)(drawableY + dy1*c - dx1*s);
	    points[1].x = (short)(drawableX + (dx1+dx2)*c + dy1*s);
	    points[1].y = (short)(drawableY + dy1*c - (dx1+dx2)*s);
	    points[2].x = (short)(drawableX + (dx1+dx2)*c + (dy1+dy2)*s);
	    points[2].y = (short)(drawableY + (dy1+dy2)*c - (dx1+dx2)*s);
	    points[3].x = (short)(drawableX + dx1*c + (dy1+dy2)*s);
	    points[3].y = (short)(drawableY + (dy1+dy2)*c - dx1*s);

	    Tk_SetCaretPos(Tk_CanvasTkwin(canvas), points[0].x, points[0].y,
		    height);
	    if (textInfoPtr->cursorOn) {
		Tk_Fill3DPolygon(Tk_CanvasTkwin(canvas), drawable,
			textInfoPtr->insertBorder, points, 4,
			textInfoPtr->insertBorderWidth, TK_RELIEF_RAISED);
	    } else if (textPtr->cursorOffGC != NULL) {
		/*
		 * Redraw the background over the area of the cursor, even
		 * though the cursor is turned off. This guarantees that the
		 * selection won't make the cursor invisible on mono displays,
		 * where both may be drawn in the same color.
		 */

		XFillPolygon(display, drawable, textPtr->cursorOffGC,
			points, 4, Convex, CoordModeOrigin);
	    }
	}
    }

    /*
     * If there is no selected text or the selected text foreground is the
     * same as the regular text foreground, then draw one text string. If
     * there is selected text and the foregrounds differ, draw the regular
     * text up to the selection, draw the selection, then draw the rest of the
     * regular text. Drawing the regular text and then the selected text over
     * it would causes problems with anti-aliased text because the two
     * anti-aliasing colors would blend together.
     */

    if ((selFirstChar >= 0) && (textPtr->selTextGC != textPtr->gc)) {
	TkDrawAngledTextLayout(display, drawable, textPtr->gc,
		textPtr->textLayout, drawableX, drawableY, textPtr->angle,
		0, selFirstChar);
	TkDrawAngledTextLayout(display, drawable, textPtr->selTextGC,
		textPtr->textLayout, drawableX, drawableY, textPtr->angle,
		selFirstChar, selLastChar + 1);
	TkDrawAngledTextLayout(display, drawable, textPtr->gc,
		textPtr->textLayout, drawableX, drawableY, textPtr->angle,
		selLastChar + 1, -1);
    } else {
	TkDrawAngledTextLayout(display, drawable, textPtr->gc,
		textPtr->textLayout, drawableX, drawableY, textPtr->angle,
		0, -1);
    }
    TkUnderlineAngledTextLayout(display, drawable, textPtr->gc,
	    textPtr->textLayout, drawableX, drawableY, textPtr->angle,
	    textPtr->underline);

    if (stipple != None) {
	XSetTSOrigin(display, textPtr->gc, 0, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TextInsert --
 *
 *	Insert characters into a text item at a given position.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The text in the given item is modified. The cursor and selection
 *	positions are also modified to reflect the insertion.
 *
 *----------------------------------------------------------------------
 */

static void
TextInsert(
    Tk_Canvas canvas,		/* Canvas containing text item. */
    Tk_Item *itemPtr,		/* Text item to be modified. */
    int index,			/* Character index before which string is to
				 * be inserted. */
    Tcl_Obj *obj)		/* New characters to be inserted. */
{
    TextItem *textPtr = (TextItem *) itemPtr;
    int byteIndex, byteCount, charsAdded;
    char *newStr, *text;
    const char *string;
    Tk_CanvasTextInfo *textInfoPtr = textPtr->textInfoPtr;

    string = Tcl_GetStringFromObj(obj, &byteCount);

    text = textPtr->text;

    if (index < 0) {
	index = 0;
    }
    if (index > textPtr->numChars) {
	index = textPtr->numChars;
    }
    byteIndex = Tcl_UtfAtIndex(text, index) - text;
    byteCount = strlen(string);
    if (byteCount == 0) {
	return;
    }

    newStr = ckalloc(textPtr->numBytes + byteCount + 1);
    memcpy(newStr, text, (size_t) byteIndex);
    strcpy(newStr + byteIndex, string);
    strcpy(newStr + byteIndex + byteCount, text + byteIndex);

    ckfree(text);
    textPtr->text = newStr;
    charsAdded = Tcl_NumUtfChars(string, byteCount);
    textPtr->numChars += charsAdded;
    textPtr->numBytes += byteCount;

    /*
     * Inserting characters invalidates indices such as those for the
     * selection and cursor. Update the indices appropriately.
     */

    if (textInfoPtr->selItemPtr == itemPtr) {
	if (textInfoPtr->selectFirst >= index) {
	    textInfoPtr->selectFirst += charsAdded;
	}
	if (textInfoPtr->selectLast >= index) {
	    textInfoPtr->selectLast += charsAdded;
	}
	if ((textInfoPtr->anchorItemPtr == itemPtr)
		&& (textInfoPtr->selectAnchor >= index)) {
	    textInfoPtr->selectAnchor += charsAdded;
	}
    }
    if (textPtr->insertPos >= index) {
	textPtr->insertPos += charsAdded;
    }
    ComputeTextBbox(canvas, textPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TextDeleteChars --
 *
 *	Delete one or more characters from a text item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters between "first" and "last", inclusive, get deleted from
 *	itemPtr, and things like the selection position get updated.
 *
 *--------------------------------------------------------------
 */

static void
TextDeleteChars(
    Tk_Canvas canvas,		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr,		/* Item in which to delete characters. */
    int first,			/* Character index of first character to
				 * delete. */
    int last)			/* Character index of last character to delete
				 * (inclusive). */
{
    TextItem *textPtr = (TextItem *) itemPtr;
    int byteIndex, byteCount, charsRemoved;
    char *newStr, *text;
    Tk_CanvasTextInfo *textInfoPtr = textPtr->textInfoPtr;

    text = textPtr->text;
    if (first < 0) {
	first = 0;
    }
    if (last >= textPtr->numChars) {
	last = textPtr->numChars - 1;
    }
    if (first > last) {
	return;
    }
    charsRemoved = last + 1 - first;

    byteIndex = Tcl_UtfAtIndex(text, first) - text;
    byteCount = Tcl_UtfAtIndex(text + byteIndex, charsRemoved)
	- (text + byteIndex);

    newStr = ckalloc(textPtr->numBytes + 1 - byteCount);
    memcpy(newStr, text, (size_t) byteIndex);
    strcpy(newStr + byteIndex, text + byteIndex + byteCount);

    ckfree(text);
    textPtr->text = newStr;
    textPtr->numChars -= charsRemoved;
    textPtr->numBytes -= byteCount;

    /*
     * Update indexes for the selection and cursor to reflect the renumbering
     * of the remaining characters.
     */

    if (textInfoPtr->selItemPtr == itemPtr) {
	if (textInfoPtr->selectFirst > first) {
	    textInfoPtr->selectFirst -= charsRemoved;
	    if (textInfoPtr->selectFirst < first) {
		textInfoPtr->selectFirst = first;
	    }
	}
	if (textInfoPtr->selectLast >= first) {
	    textInfoPtr->selectLast -= charsRemoved;
	    if (textInfoPtr->selectLast < first - 1) {
		textInfoPtr->selectLast = first - 1;
	    }
	}
	if (textInfoPtr->selectFirst > textInfoPtr->selectLast) {
	    textInfoPtr->selItemPtr = NULL;
	}
	if ((textInfoPtr->anchorItemPtr == itemPtr)
		&& (textInfoPtr->selectAnchor > first)) {
	    textInfoPtr->selectAnchor -= charsRemoved;
	    if (textInfoPtr->selectAnchor < first) {
		textInfoPtr->selectAnchor = first;
	    }
	}
    }
    if (textPtr->insertPos > first) {
	textPtr->insertPos -= charsRemoved;
	if (textPtr->insertPos < first) {
	    textPtr->insertPos = first;
	}
    }
    ComputeTextBbox(canvas, textPtr);
    return;
}

/*
 *--------------------------------------------------------------
 *
 * TextToPoint --
 *
 *	Computes the distance from a given point to a given text item, in
 *	canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	pointPtr[0] and pointPtr[1] is inside the text item. If the point
 *	isn't inside the text item then the return value is the distance from
 *	the point to the text item.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static double
TextToPoint(
    Tk_Canvas canvas,		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr,		/* Item to check against point. */
    double *pointPtr)		/* Pointer to x and y coordinates. */
{
    TextItem *textPtr;
    Tk_State state = itemPtr->state;
    double value, px, py;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    textPtr = (TextItem *) itemPtr;
    px = pointPtr[0] - textPtr->drawOrigin[0];
    py = pointPtr[1] - textPtr->drawOrigin[1];
    value = (double) Tk_DistanceToTextLayout(textPtr->textLayout,
	    (int) (px*textPtr->cosine - py*textPtr->sine),
	    (int) (py*textPtr->cosine + px*textPtr->sine));

    if ((state == TK_STATE_HIDDEN) || (textPtr->color == NULL) ||
	    (textPtr->text == NULL) || (*textPtr->text == 0)) {
	value = 1.0e36;
    }
    return value;
}

/*
 *--------------------------------------------------------------
 *
 * TextToArea --
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
TextToArea(
    Tk_Canvas canvas,		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr,		/* Item to check against rectangle. */
    double *rectPtr)		/* Pointer to array of four coordinates
				 * (x1,y1,x2,y2) describing rectangular
				 * area. */
{
    TextItem *textPtr;
    Tk_State state = itemPtr->state;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }

    textPtr = (TextItem *) itemPtr;
    return TkIntersectAngledTextLayout(textPtr->textLayout,
	    (int) ((rectPtr[0] + 0.5) - textPtr->drawOrigin[0]),
	    (int) ((rectPtr[1] + 0.5) - textPtr->drawOrigin[1]),
	    (int) (rectPtr[2] - rectPtr[0] + 0.5),
	    (int) (rectPtr[3] - rectPtr[1] + 0.5),
	    textPtr->angle);
}

/*
 *--------------------------------------------------------------
 *
 * ScaleText --
 *
 *	This function is invoked to rescale a text item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Scales the position of the text, but not the size of the font for the
 *	text.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
ScaleText(
    Tk_Canvas canvas,		/* Canvas containing rectangle. */
    Tk_Item *itemPtr,		/* Rectangle to be scaled. */
    double originX, double originY,
				/* Origin about which to scale rect. */
    double scaleX,		/* Amount to scale in X direction. */
    double scaleY)		/* Amount to scale in Y direction. */
{
    TextItem *textPtr = (TextItem *) itemPtr;

    textPtr->x = originX + scaleX*(textPtr->x - originX);
    textPtr->y = originY + scaleY*(textPtr->y - originY);
    ComputeTextBbox(canvas, textPtr);
    return;
}

/*
 *--------------------------------------------------------------
 *
 * TranslateText --
 *
 *	This function is called to move a text item by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the text item is offset by (xDelta, yDelta), and the
 *	bounding box is updated in the generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateText(
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item that is being moved. */
    double deltaX, double deltaY)
				/* Amount by which item is to be moved. */
{
    TextItem *textPtr = (TextItem *) itemPtr;

    textPtr->x += deltaX;
    textPtr->y += deltaY;
    ComputeTextBbox(canvas, textPtr);
}

/*
 *--------------------------------------------------------------
 *
 * GetTextIndex --
 *
 *	Parse an index into a text item and return either its value or an
 *	error.
 *
 * Results:
 *	A standard Tcl result. If all went well, then *indexPtr is filled in
 *	with the index (into itemPtr) corresponding to string. Otherwise an
 *	error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetTextIndex(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Canvas canvas,		/* Canvas containing item. */
    Tk_Item *itemPtr,		/* Item for which the index is being
				 * specified. */
    Tcl_Obj *obj,		/* Specification of a particular character in
				 * itemPtr's text. */
    int *indexPtr)		/* Where to store converted character
				 * index. */
{
    TextItem *textPtr = (TextItem *) itemPtr;
    int length;
    int c;
    TkCanvas *canvasPtr = (TkCanvas *) canvas;
    Tk_CanvasTextInfo *textInfoPtr = textPtr->textInfoPtr;
    const char *string = Tcl_GetStringFromObj(obj, &length);

    c = string[0];

    if ((c == 'e') && (strncmp(string, "end", (unsigned) length) == 0)) {
	*indexPtr = textPtr->numChars;
    } else if ((c == 'i')
	    && (strncmp(string, "insert", (unsigned) length) == 0)) {
	*indexPtr = textPtr->insertPos;
    } else if ((c == 's') && (length >= 5)
	    && (strncmp(string, "sel.first", (unsigned) length) == 0)) {
	if (textInfoPtr->selItemPtr != itemPtr) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "selection isn't in item", -1));
	    Tcl_SetErrorCode(interp, "TK", "CANVAS", "UNSELECTED", NULL);
	    return TCL_ERROR;
	}
	*indexPtr = textInfoPtr->selectFirst;
    } else if ((c == 's') && (length >= 5)
	    && (strncmp(string, "sel.last", (unsigned) length) == 0)) {
	if (textInfoPtr->selItemPtr != itemPtr) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "selection isn't in item", -1));
	    Tcl_SetErrorCode(interp, "TK", "CANVAS", "UNSELECTED", NULL);
	    return TCL_ERROR;
	}
	*indexPtr = textInfoPtr->selectLast;
    } else if (c == '@') {
	int x, y;
	double tmp, c = textPtr->cosine, s = textPtr->sine;
	char *end;
	const char *p;

	p = string+1;
	tmp = strtod(p, &end);
	if ((end == p) || (*end != ',')) {
	    goto badIndex;
	}
	x = (int) ((tmp < 0) ? tmp - 0.5 : tmp + 0.5);
	p = end+1;
	tmp = strtod(p, &end);
	if ((end == p) || (*end != 0)) {
	    goto badIndex;
	}
	y = (int) ((tmp < 0) ? tmp - 0.5 : tmp + 0.5);
	x += canvasPtr->scrollX1 - (int) textPtr->drawOrigin[0];
	y += canvasPtr->scrollY1 - (int) textPtr->drawOrigin[1];
	*indexPtr = Tk_PointToChar(textPtr->textLayout,
		(int) (x*c - y*s), (int) (y*c + x*s));
    } else if (Tcl_GetIntFromObj(NULL, obj, indexPtr) == TCL_OK) {
	if (*indexPtr < 0) {
	    *indexPtr = 0;
	} else if (*indexPtr > textPtr->numChars) {
	    *indexPtr = textPtr->numChars;
	}
    } else {
	/*
	 * Some of the paths here leave messages in the interp's result, so we
	 * have to clear it out before storing our own message.
	 */

    badIndex:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad index \"%s\"", string));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "ITEM_INDEX", "TEXT", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * SetTextCursor --
 *
 *	Set the position of the insertion cursor in this item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor position will change.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
SetTextCursor(
    Tk_Canvas canvas,		/* Record describing canvas widget. */
    Tk_Item *itemPtr,		/* Text item in which cursor position is to be
				 * set. */
    int index)			/* Character index of character just before
				 * which cursor is to be positioned. */
{
    TextItem *textPtr = (TextItem *) itemPtr;

    if (index < 0) {
	textPtr->insertPos = 0;
    } else if (index > textPtr->numChars) {
	textPtr->insertPos = textPtr->numChars;
    } else {
	textPtr->insertPos = index;
    }
}

/*
 *--------------------------------------------------------------
 *
 * GetSelText --
 *
 *	This function is invoked to return the selected portion of a text
 *	item. It is only called when this item has the selection.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored at buffer.
 *	Buffer is filled (or partially filled) with a NULL-terminated string
 *	containing part or all of the selection, as given by offset and
 *	maxBytes.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetSelText(
    Tk_Canvas canvas,		/* Canvas containing selection. */
    Tk_Item *itemPtr,		/* Text item containing selection. */
    int offset,			/* Byte offset within selection of first
				 * character to be returned. */
    char *buffer,		/* Location in which to place selection. */
    int maxBytes)		/* Maximum number of bytes to place at buffer,
				 * not including terminating NULL
				 * character. */
{
    TextItem *textPtr = (TextItem *) itemPtr;
    int byteCount;
    char *text;
    const char *selStart, *selEnd;
    Tk_CanvasTextInfo *textInfoPtr = textPtr->textInfoPtr;

    if ((textInfoPtr->selectFirst < 0) ||
	    (textInfoPtr->selectFirst > textInfoPtr->selectLast)) {
	return 0;
    }
    text = textPtr->text;
    selStart = Tcl_UtfAtIndex(text, textInfoPtr->selectFirst);
    selEnd = Tcl_UtfAtIndex(selStart,
	    textInfoPtr->selectLast + 1 - textInfoPtr->selectFirst);
    byteCount = selEnd - selStart - offset;
    if (byteCount > maxBytes) {
	byteCount = maxBytes;
    }
    if (byteCount <= 0) {
	return 0;
    }
    memcpy(buffer, selStart + offset, (size_t) byteCount);
    buffer[byteCount] = '\0';
    return byteCount;
}

/*
 *--------------------------------------------------------------
 *
 * TextToPostscript --
 *
 *	This function is called to generate Postscript for text items.
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
TextToPostscript(
    Tcl_Interp *interp,		/* Leave Postscript or error message here. */
    Tk_Canvas canvas,		/* Information about overall canvas. */
    Tk_Item *itemPtr,		/* Item for which Postscript is wanted. */
    int prepass)		/* 1 means this is a prepass to collect font
				 * information; 0 means final Postscript is
				 * being created. */
{
    TextItem *textPtr = (TextItem *) itemPtr;
    double x, y;
    Tk_FontMetrics fm;
    const char *justify;
    XColor *color;
    Pixmap stipple;
    Tk_State state = itemPtr->state;
    Tcl_Obj *psObj;
    Tcl_InterpState interpState;

    if (state == TK_STATE_NULL) {
	state = Canvas(canvas)->canvas_state;
    }
    color = textPtr->color;
    stipple = textPtr->stipple;
    if (state == TK_STATE_HIDDEN || textPtr->color == NULL ||
	    textPtr->text == NULL || *textPtr->text == 0) {
	return TCL_OK;
    } else if (Canvas(canvas)->currentItemPtr == itemPtr) {
	if (textPtr->activeColor != NULL) {
	    color = textPtr->activeColor;
	}
	if (textPtr->activeStipple != None) {
	    stipple = textPtr->activeStipple;
	}
    } else if (state == TK_STATE_DISABLED) {
	if (textPtr->disabledColor != NULL) {
	    color = textPtr->disabledColor;
	}
	if (textPtr->disabledStipple != None) {
	    stipple = textPtr->disabledStipple;
	}
    }

    /*
     * Make our working space.
     */

    psObj = Tcl_NewObj();
    interpState = Tcl_SaveInterpState(interp, TCL_OK);

    /*
     * Generate postscript.
     */

    Tcl_ResetResult(interp);
    if (Tk_CanvasPsFont(interp, canvas, textPtr->tkfont) != TCL_OK) {
	goto error;
    }
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

    if (prepass != 0) {
	goto done;
    }

    Tcl_ResetResult(interp);
    if (Tk_CanvasPsColor(interp, canvas, color) != TCL_OK) {
	goto error;
    }
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

    if (stipple != None) {
	Tcl_ResetResult(interp);
	Tk_CanvasPsStipple(interp, canvas, stipple);
	Tcl_AppendPrintfToObj(psObj, "/StippleText {\n    %s} bind def\n",
		Tcl_GetString(Tcl_GetObjResult(interp)));
    }

    x = 0;  y = 0;  justify = NULL;	/* lint. */
    switch (textPtr->anchor) {
    case TK_ANCHOR_NW:	   x = 0; y = 0; break;
    case TK_ANCHOR_N:	   x = 1; y = 0; break;
    case TK_ANCHOR_NE:	   x = 2; y = 0; break;
    case TK_ANCHOR_E:	   x = 2; y = 1; break;
    case TK_ANCHOR_SE:	   x = 2; y = 2; break;
    case TK_ANCHOR_S:	   x = 1; y = 2; break;
    case TK_ANCHOR_SW:	   x = 0; y = 2; break;
    case TK_ANCHOR_W:	   x = 0; y = 1; break;
    case TK_ANCHOR_CENTER: x = 1; y = 1; break;
    }
    switch (textPtr->justify) {
    case TK_JUSTIFY_LEFT:   justify = "0";   break;
    case TK_JUSTIFY_CENTER: justify = "0.5"; break;
    case TK_JUSTIFY_RIGHT:  justify = "1";   break;
    }

    Tk_GetFontMetrics(textPtr->tkfont, &fm);

    Tcl_AppendPrintfToObj(psObj, "%.15g %.15g %.15g [\n",
	    textPtr->angle, textPtr->x, Tk_CanvasPsY(canvas, textPtr->y));
    Tcl_ResetResult(interp);
    Tk_TextLayoutToPostscript(interp, textPtr->textLayout);
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
    Tcl_AppendPrintfToObj(psObj,
	    "] %d %g %g %s %s DrawText\n",
	    fm.linespace, x / -2.0, y / 2.0, justify,
	    ((stipple == None) ? "false" : "true"));

    /*
     * Plug the accumulated postscript back into the result.
     */

  done:
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
