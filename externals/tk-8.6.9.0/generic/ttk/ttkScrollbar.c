/*
 * Copyright (c) 2003, Joe English
 *
 * ttk::scrollbar widget.
 */

#include <tk.h>

#include "ttkTheme.h"
#include "ttkWidget.h"

/*------------------------------------------------------------------------
 * +++ Scrollbar widget record.
 */
typedef struct
{
    Tcl_Obj	*commandObj;

    int 	orient;
    Tcl_Obj	*orientObj;

    double	first;			/* top fraction */
    double	last;			/* bottom fraction */

    Ttk_Box	troughBox;		/* trough parcel */ 
    int 	minSize;		/* minimum size of thumb */
} ScrollbarPart;

typedef struct
{
    WidgetCore core;
    ScrollbarPart scrollbar;
} Scrollbar;

static Tk_OptionSpec ScrollbarOptionSpecs[] =
{
    {TK_OPTION_STRING, "-command", "command", "Command", "",
	Tk_Offset(Scrollbar,scrollbar.commandObj), -1, 0,0,0},

    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient", "vertical",
	Tk_Offset(Scrollbar,scrollbar.orientObj),
	Tk_Offset(Scrollbar,scrollbar.orient),
	0,(ClientData)ttkOrientStrings,STYLE_CHANGED },

    WIDGET_TAKEFOCUS_FALSE,
    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

/*------------------------------------------------------------------------
 * +++ Widget hooks.
 */

static void 
ScrollbarInitialize(Tcl_Interp *interp, void *recordPtr)
{
    Scrollbar *sb = recordPtr;
    sb->scrollbar.first = 0.0;
    sb->scrollbar.last = 1.0;

    TtkTrackElementState(&sb->core);
}

static Ttk_Layout ScrollbarGetLayout(
    Tcl_Interp *interp, Ttk_Theme theme, void *recordPtr)
{
    Scrollbar *sb = recordPtr;
    return TtkWidgetGetOrientedLayout(
	interp, theme, recordPtr, sb->scrollbar.orientObj);
}

/*
 * ScrollbarDoLayout --
 * 	Layout hook.  Adjusts the position of the scrollbar thumb.
 *
 * Side effects:
 * 	Sets sb->troughBox and sb->minSize.
 */
static void ScrollbarDoLayout(void *recordPtr)
{
    Scrollbar *sb = recordPtr;
    WidgetCore *corePtr = &sb->core;
    Ttk_Element thumb;
    Ttk_Box thumbBox;
    int thumbWidth, thumbHeight;
    double first, last, size;
    int minSize;

    /*
     * Use generic layout manager to compute initial layout:
     */
    Ttk_PlaceLayout(corePtr->layout,corePtr->state,Ttk_WinBox(corePtr->tkwin));

    /*
     * Locate thumb element, extract parcel and requested minimum size:
     */
    thumb = Ttk_FindElement(corePtr->layout, "thumb");
    if (!thumb)	/* Something has gone wrong -- bail */
	return;

    sb->scrollbar.troughBox = thumbBox = Ttk_ElementParcel(thumb);
    Ttk_LayoutNodeReqSize(
	corePtr->layout, thumb, &thumbWidth,&thumbHeight);

    /*
     * Adjust thumb element parcel:
     */
    first = sb->scrollbar.first;
    last  = sb->scrollbar.last;

    if (sb->scrollbar.orient == TTK_ORIENT_VERTICAL) {
	minSize = thumbHeight;
	size = thumbBox.height - minSize;
	thumbBox.y += (int)(size * first);
	thumbBox.height = (int)(size * last) + minSize - (int)(size * first);
    } else {
	minSize = thumbWidth;
	size = thumbBox.width - minSize;
	thumbBox.x += (int)(size * first);
	thumbBox.width = (int)(size * last) + minSize - (int)(size * first);
    }
    sb->scrollbar.minSize = minSize;
    Ttk_PlaceElement(corePtr->layout, thumb, thumbBox);
}

/*------------------------------------------------------------------------
 * +++ Widget commands.
 */

/* $sb set $first $last --
 * 	Set the position of the scrollbar.
 */
static int
ScrollbarSetCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Scrollbar *scrollbar = recordPtr;
    Tcl_Obj *firstObj, *lastObj;
    double first, last;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "first last");
	return TCL_ERROR;
    }

    firstObj = objv[2];
    lastObj = objv[3];
    if (Tcl_GetDoubleFromObj(interp, firstObj, &first) != TCL_OK
	|| Tcl_GetDoubleFromObj(interp, lastObj, &last) != TCL_OK)
	return TCL_ERROR;

    /* Range-checks:
     */
    if (first < 0.0) {
	first = 0.0;
    } else if (first > 1.0) {
	first = 1.0;
    }

    if (last < first) {
	last = first;
    } else if (last > 1.0) {
	last = 1.0;
    }

    /* ASSERT: 0.0 <= first <= last <= 1.0 */

    scrollbar->scrollbar.first = first;
    scrollbar->scrollbar.last = last;
    if (first <= 0.0 && last >= 1.0) {
	scrollbar->core.state |= TTK_STATE_DISABLED;
    } else {
	scrollbar->core.state &= ~TTK_STATE_DISABLED;
    }

    TtkRedisplayWidget(&scrollbar->core);

    return TCL_OK;
}

/* $sb get --
 * 	Returns the last thing passed to 'set'.
 */
static int
ScrollbarGetCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Scrollbar *scrollbar = recordPtr;
    Tcl_Obj *result[2];

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "");
	return TCL_ERROR;
    }

    result[0] = Tcl_NewDoubleObj(scrollbar->scrollbar.first);
    result[1] = Tcl_NewDoubleObj(scrollbar->scrollbar.last);
    Tcl_SetObjResult(interp, Tcl_NewListObj(2, result));

    return TCL_OK;
}

/* $sb delta $dx $dy --
 * 	Returns the percentage change corresponding to a mouse movement
 * 	of $dx, $dy.
 */
static int
ScrollbarDeltaCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Scrollbar *sb = recordPtr;
    double dx, dy;
    double delta = 0.0;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "dx dy");
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[2], &dx) != TCL_OK
	|| Tcl_GetDoubleFromObj(interp, objv[3], &dy) != TCL_OK)
    {
	return TCL_ERROR;
    }

    delta = 0.0;
    if (sb->scrollbar.orient == TTK_ORIENT_VERTICAL) {
	int size = sb->scrollbar.troughBox.height - sb->scrollbar.minSize;
	if (size > 0) {
	    delta = (double)dy / (double)size;
	}
    } else {
	int size = sb->scrollbar.troughBox.width - sb->scrollbar.minSize;
	if (size > 0) {
	    delta = (double)dx / (double)size;
	}
    }

    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(delta));
    return TCL_OK;
}

/* $sb fraction $x $y --
 * 	Returns a real number between 0 and 1 indicating  where  the
 * 	point given by x and y lies in the trough area of the scrollbar. 
 */
static int
ScrollbarFractionCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Scrollbar *sb = recordPtr;
    Ttk_Box b = sb->scrollbar.troughBox;
    int minSize = sb->scrollbar.minSize;
    double x, y;
    double fraction = 0.0;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "x y");
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK
	|| Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK)
    {
	return TCL_ERROR;
    }

    fraction = 0.0;
    if (sb->scrollbar.orient == TTK_ORIENT_VERTICAL) {
	if (b.height > minSize) {
	    fraction = (double)(y - b.y) / (double)(b.height - minSize);
	}
    } else {
	if (b.width > minSize) {
	    fraction = (double)(x - b.x) / (double)(b.width - minSize);
	}
    }

    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(fraction));
    return TCL_OK;
}

static const Ttk_Ensemble ScrollbarCommands[] = {
    { "configure",	TtkWidgetConfigureCommand,0 },
    { "cget",		TtkWidgetCgetCommand,0 },
    { "delta",    	ScrollbarDeltaCommand,0 },
    { "fraction",    	ScrollbarFractionCommand,0 },
    { "get",    	ScrollbarGetCommand,0 },
    { "identify",	TtkWidgetIdentifyCommand,0 },
    { "instate",	TtkWidgetInstateCommand,0 },
    { "set",  		ScrollbarSetCommand,0 },
    { "state",  	TtkWidgetStateCommand,0 },
    { 0,0,0 }
};

/*------------------------------------------------------------------------
 * +++ Widget specification.
 */
static WidgetSpec ScrollbarWidgetSpec =
{
    "TScrollbar",		/* className */
    sizeof(Scrollbar),		/* recordSize */
    ScrollbarOptionSpecs,	/* optionSpecs */
    ScrollbarCommands,		/* subcommands */
    ScrollbarInitialize,	/* initializeProc */
    TtkNullCleanup,		/* cleanupProc */
    TtkCoreConfigure,		/* configureProc */
    TtkNullPostConfigure,	/* postConfigureProc */
    ScrollbarGetLayout,		/* getLayoutProc */
    TtkWidgetSize, 		/* sizeProc */
    ScrollbarDoLayout,		/* layoutProc */
    TtkWidgetDisplay		/* displayProc */
};

TTK_BEGIN_LAYOUT(VerticalScrollbarLayout)
    TTK_GROUP("Vertical.Scrollbar.trough", TTK_FILL_Y,
	TTK_NODE("Vertical.Scrollbar.uparrow", TTK_PACK_TOP)
	TTK_NODE("Vertical.Scrollbar.downarrow", TTK_PACK_BOTTOM)
	TTK_NODE(
	    "Vertical.Scrollbar.thumb", TTK_PACK_TOP|TTK_EXPAND|TTK_FILL_BOTH))
TTK_END_LAYOUT

TTK_BEGIN_LAYOUT(HorizontalScrollbarLayout)
    TTK_GROUP("Horizontal.Scrollbar.trough", TTK_FILL_X,
	TTK_NODE("Horizontal.Scrollbar.leftarrow", TTK_PACK_LEFT)
	TTK_NODE("Horizontal.Scrollbar.rightarrow", TTK_PACK_RIGHT)
	TTK_NODE(
	"Horizontal.Scrollbar.thumb", TTK_PACK_LEFT|TTK_EXPAND|TTK_FILL_BOTH))
TTK_END_LAYOUT

/*------------------------------------------------------------------------
 * +++ Initialization.
 */

MODULE_SCOPE
void TtkScrollbar_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme = Ttk_GetDefaultTheme(interp);

    Ttk_RegisterLayout(theme,"Vertical.TScrollbar",VerticalScrollbarLayout);
    Ttk_RegisterLayout(theme,"Horizontal.TScrollbar",HorizontalScrollbarLayout);

    RegisterWidget(interp, "ttk::scrollbar", &ScrollbarWidgetSpec);
}

/*EOF*/
