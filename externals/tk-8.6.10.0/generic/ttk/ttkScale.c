/*
 * Copyright (C) 2004 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 * ttk::scale widget.
 */

#include <tk.h>
#include <string.h>
#include <stdio.h>
#include "ttkTheme.h"
#include "ttkWidget.h"

#define DEF_SCALE_LENGTH "100"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* Bit fields for OptionSpec mask field:
 */
#define STATE_CHANGED	 	(0x100)		/* -state option changed */

/*
 * Scale widget record
 */
typedef struct
{
    /* slider element options */
    Tcl_Obj *fromObj;         /* minimum value */
    Tcl_Obj *toObj;           /* maximum value */
    Tcl_Obj *valueObj;        /* current value */
    Tcl_Obj *lengthObj;       /* length of the long axis of the scale */
    Tcl_Obj *orientObj;       /* widget orientation */
    int orient;

    /* widget options */
    Tcl_Obj *commandObj;
    Tcl_Obj *variableObj;

    /* internal state */
    Ttk_TraceHandle *variableTrace;

    /*
     * Compatibility/legacy options:
     */
    Tcl_Obj *stateObj;

} ScalePart;

typedef struct
{
    WidgetCore core;
    ScalePart  scale;
} Scale;

static Tk_OptionSpec ScaleOptionSpecs[] =
{
    {TK_OPTION_STRING, "-command", "command", "Command", "",
	Tk_Offset(Scale,scale.commandObj), -1, 
	TK_OPTION_NULL_OK,0,0},
    {TK_OPTION_STRING, "-variable", "variable", "Variable", "",
	Tk_Offset(Scale,scale.variableObj), -1, 
	0,0,0},
    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient", "horizontal",
	Tk_Offset(Scale,scale.orientObj),
	Tk_Offset(Scale,scale.orient), 0, 
	(ClientData)ttkOrientStrings, STYLE_CHANGED },

    {TK_OPTION_DOUBLE, "-from", "from", "From", "0",
	Tk_Offset(Scale,scale.fromObj), -1, 0, 0, 0},
    {TK_OPTION_DOUBLE, "-to", "to", "To", "1.0",
	Tk_Offset(Scale,scale.toObj), -1, 0, 0, 0},
    {TK_OPTION_DOUBLE, "-value", "value", "Value", "0",
	Tk_Offset(Scale,scale.valueObj), -1, 0, 0, 0},
    {TK_OPTION_PIXELS, "-length", "length", "Length",
	DEF_SCALE_LENGTH, Tk_Offset(Scale,scale.lengthObj), -1, 0, 0, 
    	GEOMETRY_CHANGED},

    {TK_OPTION_STRING, "-state", "state", "State",
	"normal", Tk_Offset(Scale,scale.stateObj), -1,
        0,0,STATE_CHANGED},

    WIDGET_TAKEFOCUS_TRUE,
    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

static XPoint ValueToPoint(Scale *scalePtr, double value);
static double PointToValue(Scale *scalePtr, int x, int y);

/* ScaleVariableChanged --
 * 	Variable trace procedure for scale -variable;
 * 	Updates the scale's value.
 * 	If the linked variable is not a valid double, 
 * 	sets the 'invalid' state.
 */
static void ScaleVariableChanged(void *recordPtr, const char *value)
{
    Scale *scale = recordPtr;
    double v;

    if (value == NULL || Tcl_GetDouble(0, value, &v) != TCL_OK) {
	TtkWidgetChangeState(&scale->core, TTK_STATE_INVALID, 0);
    } else {
	Tcl_Obj *valueObj = Tcl_NewDoubleObj(v);
	Tcl_IncrRefCount(valueObj);
	Tcl_DecrRefCount(scale->scale.valueObj);
	scale->scale.valueObj = valueObj;
	TtkWidgetChangeState(&scale->core, 0, TTK_STATE_INVALID);
    }
    TtkRedisplayWidget(&scale->core);
}

/* ScaleInitialize --
 * 	Scale widget initialization hook.
 */
static void ScaleInitialize(Tcl_Interp *interp, void *recordPtr)
{
    Scale *scalePtr = recordPtr;
    TtkTrackElementState(&scalePtr->core);
}

static void ScaleCleanup(void *recordPtr)
{
    Scale *scale = recordPtr;

    if (scale->scale.variableTrace) {
	Ttk_UntraceVariable(scale->scale.variableTrace);
	scale->scale.variableTrace = 0;
    }
}

/* ScaleConfigure --
 * 	Configuration hook.
 */
static int ScaleConfigure(Tcl_Interp *interp, void *recordPtr, int mask)
{
    Scale *scale = recordPtr;
    Tcl_Obj *varName = scale->scale.variableObj;
    Ttk_TraceHandle *vt = 0;

    if (varName != NULL && *Tcl_GetString(varName) != '\0') {
	vt = Ttk_TraceVariable(interp,varName, ScaleVariableChanged,recordPtr);
	if (!vt) return TCL_ERROR;
    }

    if (TtkCoreConfigure(interp, recordPtr, mask) != TCL_OK) {
	if (vt) Ttk_UntraceVariable(vt);
	return TCL_ERROR;
    }

    if (scale->scale.variableTrace) {
	Ttk_UntraceVariable(scale->scale.variableTrace);
    }
    scale->scale.variableTrace = vt;

    if (mask & STATE_CHANGED) {
	TtkCheckStateOption(&scale->core, scale->scale.stateObj);
    }

    return TCL_OK;
}

/* ScalePostConfigure --
 * 	Post-configuration hook.
 */
static int ScalePostConfigure(
    Tcl_Interp *interp, void *recordPtr, int mask)
{
    Scale *scale = recordPtr;
    int status = TCL_OK;

    if (scale->scale.variableTrace) {
	status = Ttk_FireTrace(scale->scale.variableTrace);
	if (WidgetDestroyed(&scale->core)) {
	    return TCL_ERROR;
	}
	if (status != TCL_OK) {
	    /* Unset -variable: */
	    Ttk_UntraceVariable(scale->scale.variableTrace);
	    Tcl_DecrRefCount(scale->scale.variableObj);
	    scale->scale.variableTrace = 0;
	    scale->scale.variableObj = NULL;
	    status = TCL_ERROR;
	}
    }

    return status;
}

/* ScaleGetLayout --
 *	getLayout hook.
 */
static Ttk_Layout 
ScaleGetLayout(Tcl_Interp *interp, Ttk_Theme theme, void *recordPtr)
{
    Scale *scalePtr = recordPtr;
    return TtkWidgetGetOrientedLayout(
	interp, theme, recordPtr, scalePtr->scale.orientObj);
}

/*
 * TroughBox --
 * 	Returns the inner area of the trough element.
 */
static Ttk_Box TroughBox(Scale *scalePtr)
{
    return Ttk_ClientRegion(scalePtr->core.layout, "trough");
}

/*
 * TroughRange --
 * 	Return the value area of the trough element, adjusted
 * 	for slider size.
 */
static Ttk_Box TroughRange(Scale *scalePtr)
{
    Ttk_Box troughBox = TroughBox(scalePtr);
    Ttk_Element slider = Ttk_FindElement(scalePtr->core.layout,"slider");

    /*
     * If this is a scale widget, adjust range for slider:
     */
    if (slider) {
	Ttk_Box sliderBox = Ttk_ElementParcel(slider);
	if (scalePtr->scale.orient == TTK_ORIENT_HORIZONTAL) {
	    troughBox.x += sliderBox.width / 2;
	    troughBox.width -= sliderBox.width;
	} else {
	    troughBox.y += sliderBox.height / 2;
	    troughBox.height -= sliderBox.height;
	}
    }

    return troughBox;
}

/*
 * ScaleFraction --
 */
static double ScaleFraction(Scale *scalePtr, double value)
{
    double from = 0, to = 1, fraction;

    Tcl_GetDoubleFromObj(NULL, scalePtr->scale.fromObj, &from);
    Tcl_GetDoubleFromObj(NULL, scalePtr->scale.toObj, &to);

    if (from == to) {
	return 1.0;
    }

    fraction = (value - from) / (to - from);

    return fraction < 0 ? 0 : fraction > 1 ? 1 : fraction;
}

/* $scale get ?x y? --
 * 	Returns the current value of the scale widget, or if $x and 
 * 	$y are specified, the value represented by point @x,y.
 */
static int
ScaleGetCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Scale *scalePtr = recordPtr;
    int x, y, r = TCL_OK;
    double value = 0;

    if ((objc != 2) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "get ?x y?");
	return TCL_ERROR;
    }
    if (objc == 2) {
	Tcl_SetObjResult(interp, scalePtr->scale.valueObj);
    } else {
	r = Tcl_GetIntFromObj(interp, objv[2], &x);
	if (r == TCL_OK)
	    r = Tcl_GetIntFromObj(interp, objv[3], &y);
	if (r == TCL_OK) {
	    value = PointToValue(scalePtr, x, y);
	    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(value));
	}
    }
    return r;
}

/* $scale set $newValue
 */
static int
ScaleSetCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Scale *scalePtr = recordPtr;
    double from = 0.0, to = 1.0, value;
    int result = TCL_OK;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "set value");
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[2], &value) != TCL_OK) {
	return TCL_ERROR;
    }

    if (scalePtr->core.state & TTK_STATE_DISABLED) {
	return TCL_OK;
    }

    /* ASSERT: fromObj and toObj are valid doubles.
     */
    Tcl_GetDoubleFromObj(interp, scalePtr->scale.fromObj, &from);
    Tcl_GetDoubleFromObj(interp, scalePtr->scale.toObj, &to);

    /* Limit new value to between 'from' and 'to':
     */
    if (from < to) {
	value = value < from ? from : value > to ? to : value;
    } else {
	value = value < to ? to : value > from ? from : value;
    }

    /*
     * Set value:
     */
    Tcl_DecrRefCount(scalePtr->scale.valueObj);
    scalePtr->scale.valueObj = Tcl_NewDoubleObj(value);
    Tcl_IncrRefCount(scalePtr->scale.valueObj);
    TtkRedisplayWidget(&scalePtr->core);

    /*
     * Set attached variable, if any:
     */
    if (scalePtr->scale.variableObj != NULL) {
	Tcl_ObjSetVar2(interp, scalePtr->scale.variableObj, NULL,
	    scalePtr->scale.valueObj, TCL_GLOBAL_ONLY);
    }
    if (WidgetDestroyed(&scalePtr->core)) {
	return TCL_ERROR;
    }

    /*
     * Invoke -command, if any:
     */
    if (scalePtr->scale.commandObj != NULL) {
	Tcl_Obj *cmdObj = Tcl_DuplicateObj(scalePtr->scale.commandObj);
	Tcl_IncrRefCount(cmdObj);
	Tcl_AppendToObj(cmdObj, " ", 1);
	Tcl_AppendObjToObj(cmdObj, scalePtr->scale.valueObj);
	result = Tcl_EvalObjEx(interp, cmdObj, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdObj);
    }

    return result;
}

static int
ScaleCoordsCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Scale *scalePtr = recordPtr;
    double value;
    int r = TCL_OK;

    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "coords ?value?");
	return TCL_ERROR;
    }

    if (objc == 3) {
	r = Tcl_GetDoubleFromObj(interp, objv[2], &value);
    } else {
	r = Tcl_GetDoubleFromObj(interp, scalePtr->scale.valueObj, &value);
    }

    if (r == TCL_OK) {
	Tcl_Obj *point[2];
	XPoint pt = ValueToPoint(scalePtr, value);
	point[0] = Tcl_NewIntObj(pt.x);
	point[1] = Tcl_NewIntObj(pt.y);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, point));
    }
    return r;
}

static void ScaleDoLayout(void *clientData)
{
    WidgetCore *corePtr = clientData;
    Ttk_Element slider = Ttk_FindElement(corePtr->layout, "slider");

    Ttk_PlaceLayout(corePtr->layout,corePtr->state,Ttk_WinBox(corePtr->tkwin));

    /* Adjust the slider position:
     */
    if (slider) {
	Scale *scalePtr = clientData;
	Ttk_Box troughBox = TroughBox(scalePtr);
	Ttk_Box sliderBox = Ttk_ElementParcel(slider);
	double value = 0.0;
	double fraction;
	int range;

	Tcl_GetDoubleFromObj(NULL, scalePtr->scale.valueObj, &value);
	fraction = ScaleFraction(scalePtr, value);

	if (scalePtr->scale.orient == TTK_ORIENT_HORIZONTAL) {
	    range = troughBox.width - sliderBox.width;
	    sliderBox.x += (int)(fraction * range);
	} else {
	    range = troughBox.height - sliderBox.height;
	    sliderBox.y += (int)(fraction * range);
	}
	Ttk_PlaceElement(corePtr->layout, slider, sliderBox);
    }
}

/*
 * ScaleSize --
 * 	Compute requested size of scale.
 */
static int ScaleSize(void *clientData, int *widthPtr, int *heightPtr)
{
    WidgetCore *corePtr = clientData;
    Scale *scalePtr = clientData;
    int length;

    Ttk_LayoutSize(corePtr->layout, corePtr->state, widthPtr, heightPtr);

    /* Assert the -length configuration option */
    Tk_GetPixelsFromObj(NULL, corePtr->tkwin,
	    scalePtr->scale.lengthObj, &length);
    if (scalePtr->scale.orient == TTK_ORIENT_VERTICAL) {
	*heightPtr = MAX(*heightPtr, length);
    } else {
	*widthPtr = MAX(*widthPtr, length);
    }

    return 1;
}

static double
PointToValue(Scale *scalePtr, int x, int y)
{
    Ttk_Box troughBox = TroughRange(scalePtr);
    double from = 0, to = 1, fraction;

    Tcl_GetDoubleFromObj(NULL, scalePtr->scale.fromObj, &from);
    Tcl_GetDoubleFromObj(NULL, scalePtr->scale.toObj, &to);

    if (scalePtr->scale.orient == TTK_ORIENT_HORIZONTAL) {
	fraction = (double)(x - troughBox.x) / (double)troughBox.width;
    } else {
	fraction = (double)(y - troughBox.y) / (double)troughBox.height;
    }

    fraction = fraction < 0 ? 0 : fraction > 1 ? 1 : fraction;

    return from + fraction * (to-from);
}

/*
 * Return the center point in the widget corresponding to the given
 * value. This point can be used to center the slider.
 */

static XPoint
ValueToPoint(Scale *scalePtr, double value)
{
    Ttk_Box troughBox = TroughRange(scalePtr);
    double fraction = ScaleFraction(scalePtr, value);
    XPoint pt = {0, 0};

    if (scalePtr->scale.orient == TTK_ORIENT_HORIZONTAL) {
	pt.x = troughBox.x + (int)(fraction * troughBox.width);
	pt.y = troughBox.y + troughBox.height / 2;
    } else {
	pt.x = troughBox.x + troughBox.width / 2;
	pt.y = troughBox.y + (int)(fraction * troughBox.height);
    }
    return pt;
}

static const Ttk_Ensemble ScaleCommands[] = {
    { "configure",   TtkWidgetConfigureCommand,0 },
    { "cget",        TtkWidgetCgetCommand,0 },
    { "state",       TtkWidgetStateCommand,0 },
    { "instate",     TtkWidgetInstateCommand,0 },
    { "identify",    TtkWidgetIdentifyCommand,0 },
    { "set",         ScaleSetCommand,0 },
    { "get",         ScaleGetCommand,0 },
    { "coords",      ScaleCoordsCommand,0 },
    { 0,0,0 }
};

static WidgetSpec ScaleWidgetSpec =
{
    "TScale",			/* Class name */
    sizeof(Scale),		/* record size */
    ScaleOptionSpecs,		/* option specs */
    ScaleCommands,		/* widget commands */
    ScaleInitialize,		/* initialization proc */
    ScaleCleanup,		/* cleanup proc */
    ScaleConfigure,		/* configure proc */
    ScalePostConfigure,		/* postConfigure */
    ScaleGetLayout, 		/* getLayoutProc */
    ScaleSize,			/* sizeProc */
    ScaleDoLayout,		/* layoutProc */
    TtkWidgetDisplay		/* displayProc */
};

TTK_BEGIN_LAYOUT(VerticalScaleLayout)
    TTK_GROUP("Vertical.Scale.trough", TTK_FILL_BOTH,
	TTK_NODE("Vertical.Scale.slider", TTK_PACK_TOP) )
TTK_END_LAYOUT

TTK_BEGIN_LAYOUT(HorizontalScaleLayout)
    TTK_GROUP("Horizontal.Scale.trough", TTK_FILL_BOTH,
	TTK_NODE("Horizontal.Scale.slider", TTK_PACK_LEFT) )
TTK_END_LAYOUT

/*
 * Initialization.
 */
MODULE_SCOPE
void TtkScale_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme = Ttk_GetDefaultTheme(interp);

    Ttk_RegisterLayout(theme, "Vertical.TScale", VerticalScaleLayout);
    Ttk_RegisterLayout(theme, "Horizontal.TScale", HorizontalScaleLayout);

    RegisterWidget(interp, "ttk::scale", &ScaleWidgetSpec);
}

