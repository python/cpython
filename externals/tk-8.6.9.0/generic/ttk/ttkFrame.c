/*
 * Copyright (c) 2004, Joe English
 *
 * ttk::frame and ttk::labelframe widgets.
 */

#include <tk.h>

#include "ttkTheme.h"
#include "ttkWidget.h"
#include "ttkManager.h"

/* ======================================================================
 * +++ Frame widget:
 */

typedef struct {
    Tcl_Obj	*borderWidthObj;
    Tcl_Obj	*paddingObj;
    Tcl_Obj	*reliefObj;
    Tcl_Obj 	*widthObj;
    Tcl_Obj 	*heightObj;
} FramePart;

typedef struct {
    WidgetCore	core;
    FramePart	frame;
} Frame;

static Tk_OptionSpec FrameOptionSpecs[] = {
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth", NULL,
	Tk_Offset(Frame,frame.borderWidthObj), -1,
	TK_OPTION_NULL_OK,0,GEOMETRY_CHANGED },
    {TK_OPTION_STRING, "-padding", "padding", "Pad", NULL,
	Tk_Offset(Frame,frame.paddingObj), -1,
	TK_OPTION_NULL_OK,0,GEOMETRY_CHANGED },
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief", NULL,
	Tk_Offset(Frame,frame.reliefObj), -1,
	TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_PIXELS, "-width", "width", "Width", "0",
	Tk_Offset(Frame,frame.widthObj), -1,
	0,0,GEOMETRY_CHANGED },
    {TK_OPTION_PIXELS, "-height", "height", "Height", "0",
	Tk_Offset(Frame,frame.heightObj), -1,
	0,0,GEOMETRY_CHANGED },

    WIDGET_TAKEFOCUS_FALSE,
    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

static const Ttk_Ensemble FrameCommands[] = {
    { "configure",	TtkWidgetConfigureCommand,0 },
    { "cget",   	TtkWidgetCgetCommand,0 },
    { "instate",	TtkWidgetInstateCommand,0 },
    { "state",  	TtkWidgetStateCommand,0 },
    { "identify",	TtkWidgetIdentifyCommand,0 },
    { 0,0,0 }
};

/*
 * FrameMargins --
 * 	Compute internal margins for a frame widget.
 * 	This includes the -borderWidth, plus any additional -padding.
 */
static Ttk_Padding FrameMargins(Frame *framePtr)
{
    Ttk_Padding margins = Ttk_UniformPadding(0);

    /* Check -padding:
     */
    if (framePtr->frame.paddingObj) {
	Ttk_GetPaddingFromObj(NULL,
	    framePtr->core.tkwin, framePtr->frame.paddingObj, &margins);
    }

    /* Add padding for border:
     */
    if (framePtr->frame.borderWidthObj) {
	int border = 0;
	Tk_GetPixelsFromObj(NULL,
	    framePtr->core.tkwin, framePtr->frame.borderWidthObj, &border);
	margins = Ttk_AddPadding(margins, Ttk_UniformPadding((short)border));
    }

    return margins;
}

/* FrameSize procedure --
 * 	The frame doesn't request a size of its own by default,
 * 	but it does have an internal border.  See also <<NOTE-SIZE>>
 */
static int FrameSize(void *recordPtr, int *widthPtr, int *heightPtr)
{
    Frame *framePtr = recordPtr;
    Ttk_SetMargins(framePtr->core.tkwin, FrameMargins(framePtr));
    return 0;
}

/*
 * FrameConfigure -- configure hook.
 *	<<NOTE-SIZE>> Usually the size of a frame is controlled by
 *	a geometry manager (pack, grid); the -width and -height
 *	options are only effective if geometry propagation is turned
 *	off or if the [place] GM is used for child widgets.
 *
 *	To avoid geometry blinking, we issue a geometry request
 *	in the Configure hook instead of the Size hook, and only
 *	if -width and/or -height is nonzero and one of them
 *	or the other size-related options (-borderwidth, -padding)
 *	has been changed.
 */

static int FrameConfigure(Tcl_Interp *interp, void *recordPtr, int mask)
{
    Frame *framePtr = recordPtr;
    int width, height;

    /*
     * Make sure -padding resource, if present, is correct:
     */
    if (framePtr->frame.paddingObj) {
	Ttk_Padding unused;
	if (Ttk_GetPaddingFromObj(interp,
		    	framePtr->core.tkwin,
			framePtr->frame.paddingObj,
			&unused) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    /* See <<NOTE-SIZE>>
     */
    if (  TCL_OK != Tk_GetPixelsFromObj(
	    interp,framePtr->core.tkwin,framePtr->frame.widthObj,&width)
       || TCL_OK != Tk_GetPixelsFromObj(
	    interp,framePtr->core.tkwin,framePtr->frame.heightObj,&height)
       )
    {
	return TCL_ERROR;
    }

    if ((width > 0 || height > 0) && (mask & GEOMETRY_CHANGED)) {
	Tk_GeometryRequest(framePtr->core.tkwin, width, height);
    }

    return TtkCoreConfigure(interp, recordPtr, mask);
}

static WidgetSpec FrameWidgetSpec = {
    "TFrame",			/* className */
    sizeof(Frame),		/* recordSize */
    FrameOptionSpecs,		/* optionSpecs */
    FrameCommands,		/* subcommands */
    TtkNullInitialize,		/* initializeProc */
    TtkNullCleanup,		/* cleanupProc */
    FrameConfigure,		/* configureProc */
    TtkNullPostConfigure,	/* postConfigureProc */
    TtkWidgetGetLayout, 	/* getLayoutProc */
    FrameSize,			/* sizeProc */
    TtkWidgetDoLayout,		/* layoutProc */
    TtkWidgetDisplay		/* displayProc */
};

TTK_BEGIN_LAYOUT(FrameLayout)
    TTK_NODE("Frame.border", TTK_FILL_BOTH)
TTK_END_LAYOUT

/* ======================================================================
 * +++ Labelframe widget:
 */

#define DEFAULT_LABELINSET	8
#define DEFAULT_BORDERWIDTH	2

int TtkGetLabelAnchorFromObj(
    Tcl_Interp *interp, Tcl_Obj *objPtr, Ttk_PositionSpec *anchorPtr)
{
    const char *string = Tcl_GetString(objPtr);
    char c = *string++;
    Ttk_PositionSpec flags = 0;

    /* First character determines side:
     */
    switch (c) {
	case 'w' : flags = TTK_PACK_LEFT; 	break;
	case 'e' : flags = TTK_PACK_RIGHT; 	break;
	case 'n' : flags = TTK_PACK_TOP; 	break;
	case 's' : flags = TTK_PACK_BOTTOM; 	break;
	default  : goto error;
    }

    /* Remaining characters are as per -sticky:
     */
    while ((c = *string++) != '\0') {
	switch (c) {
	    case 'w' : flags |= TTK_STICK_W; break;
	    case 'e' : flags |= TTK_STICK_E; break;
	    case 'n' : flags |= TTK_STICK_N; break;
	    case 's' : flags |= TTK_STICK_S; break;
	    default  : goto error;
	}
    }

    *anchorPtr = flags;
    return TCL_OK;

error:
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Bad label anchor specification %s", Tcl_GetString(objPtr)));
	Tcl_SetErrorCode(interp, "TTK", "LABEL", "ANCHOR", NULL);
    }
    return TCL_ERROR;
}

/* LabelAnchorSide --
 * 	Returns the side corresponding to a LabelAnchor value.
 */
static Ttk_Side LabelAnchorSide(Ttk_PositionSpec flags)
{
    if (flags & TTK_PACK_LEFT) 		return TTK_SIDE_LEFT;
    else if (flags & TTK_PACK_RIGHT)	return TTK_SIDE_RIGHT;
    else if (flags & TTK_PACK_TOP)	return TTK_SIDE_TOP;
    else if (flags & TTK_PACK_BOTTOM)	return TTK_SIDE_BOTTOM;
    /*NOTREACHED*/
    return TTK_SIDE_TOP;
}

/*
 * Labelframe widget record:
 */
typedef struct {
    Tcl_Obj 	*labelAnchorObj;
    Tcl_Obj	*textObj;
    Tcl_Obj 	*underlineObj;
    Tk_Window	labelWidget;

    Ttk_Manager	*mgr;
    Ttk_Layout	labelLayout;	/* Sublayout for label */
    Ttk_Box	labelParcel;	/* Set in layoutProc */
} LabelframePart;

typedef struct {
    WidgetCore  	core;
    FramePart   	frame;
    LabelframePart	label;
} Labelframe;

#define LABELWIDGET_CHANGED 0x100

static Tk_OptionSpec LabelframeOptionSpecs[] = {
    {TK_OPTION_STRING, "-labelanchor", "labelAnchor", "LabelAnchor",
	"nw", Tk_Offset(Labelframe, label.labelAnchorObj),-1,
        0,0,GEOMETRY_CHANGED},
    {TK_OPTION_STRING, "-text", "text", "Text", "",
	Tk_Offset(Labelframe,label.textObj), -1,
	0,0,GEOMETRY_CHANGED },
    {TK_OPTION_INT, "-underline", "underline", "Underline",
	"-1", Tk_Offset(Labelframe,label.underlineObj), -1,
	0,0,0 },
    {TK_OPTION_WINDOW, "-labelwidget", "labelWidget", "LabelWidget", NULL,
	-1, Tk_Offset(Labelframe,label.labelWidget),
	TK_OPTION_NULL_OK,0,LABELWIDGET_CHANGED|GEOMETRY_CHANGED },

    WIDGET_INHERIT_OPTIONS(FrameOptionSpecs)
};

/*
 * Labelframe style parameters:
 */
typedef struct {
    int 		borderWidth;	/* border width */
    Ttk_Padding 	padding;	/* internal padding */
    Ttk_PositionSpec	labelAnchor;	/* corner/side to place label */
    Ttk_Padding		labelMargins;	/* extra space around label */
    int 		labelOutside;	/* true=>place label outside border */
} LabelframeStyle;

static void LabelframeStyleOptions(Labelframe *lf, LabelframeStyle *style)
{
    Ttk_Layout layout = lf->core.layout;
    Tcl_Obj *objPtr;

    style->borderWidth = DEFAULT_BORDERWIDTH;
    style->padding = Ttk_UniformPadding(0);
    style->labelAnchor = TTK_PACK_TOP | TTK_STICK_W;
    style->labelOutside = 0;

    if ((objPtr = Ttk_QueryOption(layout, "-borderwidth", 0)) != NULL) {
	Tk_GetPixelsFromObj(NULL, lf->core.tkwin, objPtr, &style->borderWidth);
    }
    if ((objPtr = Ttk_QueryOption(layout, "-padding", 0)) != NULL) {
	Ttk_GetPaddingFromObj(NULL, lf->core.tkwin, objPtr, &style->padding);
    }
    if ((objPtr = Ttk_QueryOption(layout,"-labelanchor", 0)) != NULL) {
	TtkGetLabelAnchorFromObj(NULL, objPtr, &style->labelAnchor);
    }
    if ((objPtr = Ttk_QueryOption(layout,"-labelmargins", 0)) != NULL) {
	Ttk_GetBorderFromObj(NULL, objPtr, &style->labelMargins);
    } else {
	if (style->labelAnchor & (TTK_PACK_TOP|TTK_PACK_BOTTOM)) {
	    style->labelMargins =
		Ttk_MakePadding(DEFAULT_LABELINSET,0,DEFAULT_LABELINSET,0);
	} else {
	    style->labelMargins =
		Ttk_MakePadding(0,DEFAULT_LABELINSET,0,DEFAULT_LABELINSET);
	}
    }
    if ((objPtr = Ttk_QueryOption(layout,"-labeloutside", 0)) != NULL) {
	Tcl_GetBooleanFromObj(NULL, objPtr, &style->labelOutside);
    }

    return;
}

/* LabelframeLabelSize --
 * 	Extract the requested width and height of the labelframe's label:
 * 	taken from the label widget if specified, otherwise the text label.
 */
static void
LabelframeLabelSize(Labelframe *lframePtr, int *widthPtr, int *heightPtr)
{
    Tk_Window labelWidget = lframePtr->label.labelWidget;
    Ttk_Layout labelLayout = lframePtr->label.labelLayout;

    if (labelWidget) {
	*widthPtr = Tk_ReqWidth(labelWidget);
	*heightPtr = Tk_ReqHeight(labelWidget);
    } else if (labelLayout) {
	Ttk_LayoutSize(labelLayout, 0, widthPtr, heightPtr);
    } else {
	*widthPtr = *heightPtr = 0;
    }
}

/*
 * LabelframeSize --
 * 	Like the frame, this doesn't request a size of its own
 * 	but it does have internal padding and a minimum size.
 */
static int LabelframeSize(void *recordPtr, int *widthPtr, int *heightPtr)
{
    Labelframe *lframePtr = recordPtr;
    WidgetCore *corePtr = &lframePtr->core;
    Ttk_Padding margins;
    LabelframeStyle style;
    int labelWidth, labelHeight;

    LabelframeStyleOptions(lframePtr, &style);

    /* Compute base margins (See also: FrameMargins)
     */
    margins = Ttk_AddPadding(
		style.padding, Ttk_UniformPadding((short)style.borderWidth));

    /* Adjust margins based on label size and position:
     */
    LabelframeLabelSize(lframePtr, &labelWidth, &labelHeight);
    labelWidth += Ttk_PaddingWidth(style.labelMargins);
    labelHeight += Ttk_PaddingHeight(style.labelMargins);

    switch (LabelAnchorSide(style.labelAnchor)) {
	case TTK_SIDE_LEFT:	margins.left   += labelWidth;	break;
	case TTK_SIDE_RIGHT:	margins.right  += labelWidth;	break;
	case TTK_SIDE_TOP:	margins.top    += labelHeight;	break;
	case TTK_SIDE_BOTTOM:	margins.bottom += labelHeight;	break;
    }

    Ttk_SetMargins(corePtr->tkwin,margins);

    /* Request minimum size based on border width and label size:
     */
    Tk_SetMinimumRequestSize(corePtr->tkwin,
	    labelWidth + 2*style.borderWidth,
	    labelHeight + 2*style.borderWidth);

    return 0;
}

/*
 * LabelframeGetLayout --
 * 	Getlayout widget hook.
 */

static Ttk_Layout LabelframeGetLayout(
    Tcl_Interp *interp, Ttk_Theme theme, void *recordPtr)
{
    Labelframe *lf = recordPtr;
    Ttk_Layout frameLayout = TtkWidgetGetLayout(interp, theme, recordPtr);
    Ttk_Layout labelLayout;

    if (!frameLayout) {
	return NULL;
    }

    labelLayout = Ttk_CreateSublayout(
	interp, theme, frameLayout, ".Label", lf->core.optionTable);

    if (labelLayout) {
	if (lf->label.labelLayout) {
	    Ttk_FreeLayout(lf->label.labelLayout);
	}
	Ttk_RebindSublayout(labelLayout, recordPtr);
	lf->label.labelLayout = labelLayout;
    }

    return frameLayout;
}

/*
 * LabelframeDoLayout --
 * 	Labelframe layout hook.
 *
 * Side effects: Computes labelParcel.
 */

static void LabelframeDoLayout(void *recordPtr)
{
    Labelframe *lframePtr = recordPtr;
    WidgetCore *corePtr = &lframePtr->core;
    int lw, lh;			/* Label width and height */
    LabelframeStyle style;
    Ttk_Box borderParcel = Ttk_WinBox(lframePtr->core.tkwin);
    Ttk_Box labelParcel;

    /*
     * Compute label parcel:
     */
    LabelframeStyleOptions(lframePtr, &style);
    LabelframeLabelSize(lframePtr, &lw, &lh);
    lw += Ttk_PaddingWidth(style.labelMargins);
    lh += Ttk_PaddingHeight(style.labelMargins);

    labelParcel = Ttk_PadBox(
	Ttk_PositionBox(&borderParcel, lw, lh, style.labelAnchor),
	style.labelMargins);

    if (!style.labelOutside) {
	/* Move border edge so it's over label:
	*/
	switch (LabelAnchorSide(style.labelAnchor)) {
	    case TTK_SIDE_LEFT: 	borderParcel.x -= lw / 2;
	    case TTK_SIDE_RIGHT:	borderParcel.width += lw/2; 	break;
	    case TTK_SIDE_TOP:  	borderParcel.y -= lh / 2;
	    case TTK_SIDE_BOTTOM:	borderParcel.height += lh / 2;	break;
	}
    }

    /*
     * Place border and label:
     */
    Ttk_PlaceLayout(corePtr->layout, corePtr->state, borderParcel);
    if (lframePtr->label.labelLayout) {
	Ttk_PlaceLayout(
	    lframePtr->label.labelLayout, corePtr->state, labelParcel);
    }
    /* labelWidget placed in LabelframePlaceSlaves GM hook */
    lframePtr->label.labelParcel = labelParcel;
}

static void LabelframeDisplay(void *recordPtr, Drawable d)
{
    Labelframe *lframePtr = recordPtr;
    Ttk_DrawLayout(lframePtr->core.layout, lframePtr->core.state, d);
    if (lframePtr->label.labelLayout) {
	Ttk_DrawLayout(lframePtr->label.labelLayout, lframePtr->core.state, d);
    }
}

/* +++ Labelframe geometry manager hooks.
 */

/* LabelframePlaceSlaves --
 * 	Sets the position and size of the labelwidget.
 */
static void LabelframePlaceSlaves(void *recordPtr)
{
    Labelframe *lframe = recordPtr;

    if (Ttk_NumberSlaves(lframe->label.mgr) == 1) {
	Ttk_Box b;
	LabelframeDoLayout(recordPtr);
	b = lframe->label.labelParcel;
	/* ASSERT: slave #0 is lframe->label.labelWidget */
	Ttk_PlaceSlave(lframe->label.mgr, 0, b.x,b.y,b.width,b.height);
    }
}

static int LabelRequest(void *managerData, int index, int width, int height)
{
    return 1;
}

/* LabelRemoved --
 * 	Unset the -labelwidget option.
 *
 * <<NOTE-LABELREMOVED>>:
 * 	This routine is also called when the widget voluntarily forgets
 * 	the slave in LabelframeConfigure.
 */
static void LabelRemoved(void *managerData, int slaveIndex)
{
    Labelframe *lframe = managerData;
    lframe->label.labelWidget = 0;
}

static Ttk_ManagerSpec LabelframeManagerSpec = {
    { "labelframe", Ttk_GeometryRequestProc, Ttk_LostSlaveProc },
    LabelframeSize,
    LabelframePlaceSlaves,
    LabelRequest,
    LabelRemoved
};

/* LabelframeInitialize --
 * 	Initialization hook.
 */
static void LabelframeInitialize(Tcl_Interp *interp, void *recordPtr)
{
    Labelframe *lframe = recordPtr;

    lframe->label.mgr = Ttk_CreateManager(
	&LabelframeManagerSpec, lframe, lframe->core.tkwin);
    lframe->label.labelWidget = 0;
    lframe->label.labelLayout = 0;
    lframe->label.labelParcel = Ttk_MakeBox(-1,-1,-1,-1);
}

/* LabelframeCleanup --
 * 	Cleanup hook.
 */
static void LabelframeCleanup(void *recordPtr)
{
    Labelframe *lframe = recordPtr;
    Ttk_DeleteManager(lframe->label.mgr);
    if (lframe->label.labelLayout) {
	Ttk_FreeLayout(lframe->label.labelLayout);
    }
}

/* RaiseLabelWidget --
 * 	Raise the -labelwidget to ensure that the labelframe doesn't
 * 	obscure it (if it's not a direct child), or bring it to
 * 	the top of the stacking order (if it is).
 */
static void RaiseLabelWidget(Labelframe *lframe)
{
    Tk_Window parent = Tk_Parent(lframe->label.labelWidget);
    Tk_Window sibling = NULL;
    Tk_Window w = lframe->core.tkwin;

    while (w && w != parent) {
	sibling = w;
	w = Tk_Parent(w);
    }

    Tk_RestackWindow(lframe->label.labelWidget, Above, sibling);
}

/* LabelframeConfigure --
 * 	Configuration hook.
 */
static int LabelframeConfigure(Tcl_Interp *interp,void *recordPtr,int mask)
{
    Labelframe *lframePtr = recordPtr;
    Tk_Window labelWidget = lframePtr->label.labelWidget;
    Ttk_PositionSpec unused;

    /* Validate options:
     */
    if (mask & LABELWIDGET_CHANGED && labelWidget != NULL) {
	if (!Ttk_Maintainable(interp, labelWidget, lframePtr->core.tkwin)) {
	    return TCL_ERROR;
	}
    }

    if (TtkGetLabelAnchorFromObj(
    	interp, lframePtr->label.labelAnchorObj, &unused) != TCL_OK)
    {
	return TCL_ERROR;
    }

    /* Base class configuration:
     */
    if (FrameConfigure(interp, recordPtr, mask) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Update -labelwidget changes, if any:
     */
    if (mask & LABELWIDGET_CHANGED) {
	if (Ttk_NumberSlaves(lframePtr->label.mgr) == 1) {
	    Ttk_ForgetSlave(lframePtr->label.mgr, 0);
	    /* Restore labelWidget field (see <<NOTE-LABELREMOVED>>)
	     */
	    lframePtr->label.labelWidget = labelWidget;
	}

	if (labelWidget) {
	    Ttk_InsertSlave(lframePtr->label.mgr, 0, labelWidget, NULL);
	    RaiseLabelWidget(lframePtr);
	}
    }

    if (mask & GEOMETRY_CHANGED) {
	Ttk_ManagerSizeChanged(lframePtr->label.mgr);
	Ttk_ManagerLayoutChanged(lframePtr->label.mgr);
    }

    return TCL_OK;
}

static WidgetSpec LabelframeWidgetSpec = {
    "TLabelframe",		/* className */
    sizeof(Labelframe),		/* recordSize */
    LabelframeOptionSpecs, 	/* optionSpecs */
    FrameCommands,		/* subcommands */
    LabelframeInitialize,	/* initializeProc */
    LabelframeCleanup,		/* cleanupProc */
    LabelframeConfigure,	/* configureProc */
    TtkNullPostConfigure,  	/* postConfigureProc */
    LabelframeGetLayout, 	/* getLayoutProc */
    LabelframeSize,		/* sizeProc */
    LabelframeDoLayout,		/* layoutProc */
    LabelframeDisplay		/* displayProc */
};

TTK_BEGIN_LAYOUT(LabelframeLayout)
    TTK_NODE("Labelframe.border", TTK_FILL_BOTH)
TTK_END_LAYOUT

TTK_BEGIN_LAYOUT(LabelSublayout)
    TTK_GROUP("Label.fill", TTK_FILL_BOTH,
	TTK_NODE("Label.text", TTK_FILL_BOTH))
TTK_END_LAYOUT

/* ======================================================================
 * +++ Initialization.
 */

MODULE_SCOPE
void TtkFrame_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme =  Ttk_GetDefaultTheme(interp);

    Ttk_RegisterLayout(theme, "TFrame", FrameLayout);
    Ttk_RegisterLayout(theme, "TLabelframe", LabelframeLayout);
    Ttk_RegisterLayout(theme, "Label", LabelSublayout);

    RegisterWidget(interp, "ttk::frame", &FrameWidgetSpec);
    RegisterWidget(interp, "ttk::labelframe", &LabelframeWidgetSpec);
}

