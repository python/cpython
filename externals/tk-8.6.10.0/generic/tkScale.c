/*
 * tkScale.c --
 *
 *	This module implements a scale widgets for the Tk toolkit. A scale
 *	displays a slider that can be adjusted to change a value; it also
 *	displays numeric labels and a textual label, if desired.
 *
 *	The modifications to use floating-point values are based on an
 *	implementation by Paul Mackerras. The -variable option is due to
 *	Henning Schulzrinne. All of these are used with permission.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"
#include "tkScale.h"

#if defined(_WIN32)
#define snprintf _snprintf
#endif

/*
 * The following table defines the legal values for the -orient option. It is
 * used together with the "enum orient" declaration in tkScale.h.
 */

static const char *const orientStrings[] = {
    "horizontal", "vertical", NULL
};

/*
 * The following table defines the legal values for the -state option. It is
 * used together with the "enum state" declaration in tkScale.h.
 */

static const char *const stateStrings[] = {
    "active", "disabled", "normal", NULL
};

static const Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_SCALE_ACTIVE_BG_COLOR, -1, Tk_Offset(TkScale, activeBorder),
	0, DEF_SCALE_ACTIVE_BG_MONO, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_SCALE_BG_COLOR, -1, Tk_Offset(TkScale, bgBorder),
	0, DEF_SCALE_BG_MONO, 0},
    {TK_OPTION_DOUBLE, "-bigincrement", "bigIncrement", "BigIncrement",
	DEF_SCALE_BIG_INCREMENT, -1, Tk_Offset(TkScale, bigIncrement),
	0, 0, 0},
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	NULL, 0, -1, 0, "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
	NULL, 0, -1, 0, "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_SCALE_BORDER_WIDTH, -1, Tk_Offset(TkScale, borderWidth),
	0, 0, 0},
    {TK_OPTION_STRING, "-command", "command", "Command",
	DEF_SCALE_COMMAND, -1, Tk_Offset(TkScale, command),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_SCALE_CURSOR, -1, Tk_Offset(TkScale, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_INT, "-digits", "digits", "Digits",
	DEF_SCALE_DIGITS, -1, Tk_Offset(TkScale, digits),
	0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", NULL,
	NULL, 0, -1, 0, "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_SCALE_FONT, -1, Tk_Offset(TkScale, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_SCALE_FG_COLOR, -1, Tk_Offset(TkScale, textColorPtr), 0,
	(ClientData) DEF_SCALE_FG_MONO, 0},
    {TK_OPTION_DOUBLE, "-from", "from", "From", DEF_SCALE_FROM, -1,
	Tk_Offset(TkScale, fromValue), 0, 0, 0},
    {TK_OPTION_BORDER, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_SCALE_HIGHLIGHT_BG_COLOR,
	-1, Tk_Offset(TkScale, highlightBorder),
	0, DEF_SCALE_HIGHLIGHT_BG_MONO, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_SCALE_HIGHLIGHT, -1, Tk_Offset(TkScale, highlightColorPtr),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_SCALE_HIGHLIGHT_WIDTH, -1,
	Tk_Offset(TkScale, highlightWidth), 0, 0, 0},
    {TK_OPTION_STRING, "-label", "label", "Label",
	DEF_SCALE_LABEL, -1, Tk_Offset(TkScale, label),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-length", "length", "Length",
	DEF_SCALE_LENGTH, -1, Tk_Offset(TkScale, length), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient",
	DEF_SCALE_ORIENT, -1, Tk_Offset(TkScale, orient),
	0, orientStrings, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_SCALE_RELIEF, -1, Tk_Offset(TkScale, relief), 0, 0, 0},
    {TK_OPTION_INT, "-repeatdelay", "repeatDelay", "RepeatDelay",
	DEF_SCALE_REPEAT_DELAY, -1, Tk_Offset(TkScale, repeatDelay),
	0, 0, 0},
    {TK_OPTION_INT, "-repeatinterval", "repeatInterval", "RepeatInterval",
	DEF_SCALE_REPEAT_INTERVAL, -1, Tk_Offset(TkScale, repeatInterval),
	0, 0, 0},
    {TK_OPTION_DOUBLE, "-resolution", "resolution", "Resolution",
	DEF_SCALE_RESOLUTION, -1, Tk_Offset(TkScale, resolution),
	0, 0, 0},
    {TK_OPTION_BOOLEAN, "-showvalue", "showValue", "ShowValue",
	DEF_SCALE_SHOW_VALUE, -1, Tk_Offset(TkScale, showValue),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-sliderlength", "sliderLength", "SliderLength",
	DEF_SCALE_SLIDER_LENGTH, -1, Tk_Offset(TkScale, sliderLength),
	0, 0, 0},
    {TK_OPTION_RELIEF, "-sliderrelief", "sliderRelief", "SliderRelief",
	DEF_SCALE_SLIDER_RELIEF, -1, Tk_Offset(TkScale, sliderRelief),
	0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
	DEF_SCALE_STATE, -1, Tk_Offset(TkScale, state),
	0, stateStrings, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_SCALE_TAKE_FOCUS, Tk_Offset(TkScale, takeFocusPtr), -1,
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-tickinterval", "tickInterval", "TickInterval",
	DEF_SCALE_TICK_INTERVAL, -1, Tk_Offset(TkScale, tickInterval),
	0, 0, 0},
    {TK_OPTION_DOUBLE, "-to", "to", "To",
	DEF_SCALE_TO, -1, Tk_Offset(TkScale, toValue), 0, 0, 0},
    {TK_OPTION_COLOR, "-troughcolor", "troughColor", "Background",
	DEF_SCALE_TROUGH_COLOR, -1, Tk_Offset(TkScale, troughColorPtr),
	0, DEF_SCALE_TROUGH_MONO, 0},
    {TK_OPTION_STRING, "-variable", "variable", "Variable",
	DEF_SCALE_VARIABLE, Tk_Offset(TkScale, varNamePtr), -1,
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-width", "width", "Width",
	DEF_SCALE_WIDTH, -1, Tk_Offset(TkScale, width), 0, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, -1, 0, 0, 0}
};

/*
 * The following tables define the scale widget commands and map the indexes
 * into the string tables into a single enumerated type used to dispatch the
 * scale widget command.
 */

static const char *const commandNames[] = {
    "cget", "configure", "coords", "get", "identify", "set", NULL
};

enum command {
    COMMAND_CGET, COMMAND_CONFIGURE, COMMAND_COORDS, COMMAND_GET,
    COMMAND_IDENTIFY, COMMAND_SET
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		ComputeFormat(TkScale *scalePtr, int forTicks);
static void		ComputeScaleGeometry(TkScale *scalePtr);
static int		ConfigureScale(Tcl_Interp *interp, TkScale *scalePtr,
			    int objc, Tcl_Obj *const objv[]);
static void		DestroyScale(char *memPtr);
static double		MaxTickRoundingError(TkScale *scalePtr,
			    double tickResolution);
static void		ScaleCmdDeletedProc(ClientData clientData);
static void		ScaleEventProc(ClientData clientData,
			    XEvent *eventPtr);
static char *		ScaleVarProc(ClientData clientData,
			    Tcl_Interp *interp, const char *name1,
			    const char *name2, int flags);
static int		ScaleWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		ScaleWorldChanged(ClientData instanceData);
static void		ScaleSetVariable(TkScale *scalePtr);

/*
 * The structure below defines scale class behavior by means of procedures
 * that can be invoked from generic window code.
 */

static const Tk_ClassProcs scaleClass = {
    sizeof(Tk_ClassProcs),	/* size */
    ScaleWorldChanged,		/* worldChangedProc */
    NULL,			/* createProc */
    NULL			/* modalProc */
};

/*
 *--------------------------------------------------------------
 *
 * ScaleDigit, ScaleMax, ScaleMin, ScaleRound --
 *
 *	Simple math helper functions, designed to be automatically inlined by
 *	the compiler most of the time.
 *
 *--------------------------------------------------------------
 */

static inline int
ScaleDigit(
    double value)
{
    return (int) floor(log10(fabs(value)));
}

static inline double
ScaleMax(
    double a,
    double b)
{
    return (a > b) ? a : b;
}

static inline double
ScaleMin(
    double a,
    double b)
{
    return (a < b) ? a : b;
}

static inline int
ScaleRound(
    double value)
{
    return (int) floor(value + 0.5);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_ScaleObjCmd --
 *
 *	This procedure is invoked to process the "scale" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_ScaleObjCmd(
    ClientData clientData,	/* NULL. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument values. */
{
    register TkScale *scalePtr;
    Tk_OptionTable optionTable;
    Tk_Window tkwin;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?-option value ...?");
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp),
	    Tcl_GetString(objv[1]), NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    /*
     * Create the option table for this widget class. If it has already been
     * created, the cached pointer will be returned.
     */

    optionTable = Tk_CreateOptionTable(interp, optionSpecs);

    Tk_SetClass(tkwin, "Scale");
    scalePtr = TkpCreateScale(tkwin);

    /*
     * Initialize fields that won't be initialized by ConfigureScale, or which
     * ConfigureScale expects to have reasonable values (e.g. resource
     * pointers).
     */

    scalePtr->tkwin		= tkwin;
    scalePtr->display		= Tk_Display(tkwin);
    scalePtr->interp		= interp;
    scalePtr->widgetCmd		= Tcl_CreateObjCommand(interp,
	    Tk_PathName(scalePtr->tkwin), ScaleWidgetObjCmd,
	    scalePtr, ScaleCmdDeletedProc);
    scalePtr->optionTable	= optionTable;
    scalePtr->orient		= ORIENT_VERTICAL;
    scalePtr->width		= 0;
    scalePtr->length		= 0;
    scalePtr->value		= 0.0;
    scalePtr->varNamePtr	= NULL;
    scalePtr->fromValue		= 0.0;
    scalePtr->toValue		= 0.0;
    scalePtr->tickInterval	= 0.0;
    scalePtr->resolution	= 1.0;
    scalePtr->digits		= 0;
    scalePtr->bigIncrement	= 0.0;
    scalePtr->command		= NULL;
    scalePtr->repeatDelay	= 0;
    scalePtr->repeatInterval	= 0;
    scalePtr->label		= NULL;
    scalePtr->labelLength	= 0;
    scalePtr->state		= STATE_NORMAL;
    scalePtr->borderWidth	= 0;
    scalePtr->bgBorder		= NULL;
    scalePtr->activeBorder	= NULL;
    scalePtr->sliderRelief	= TK_RELIEF_RAISED;
    scalePtr->troughColorPtr	= NULL;
    scalePtr->troughGC		= NULL;
    scalePtr->copyGC		= NULL;
    scalePtr->tkfont		= NULL;
    scalePtr->textColorPtr	= NULL;
    scalePtr->textGC		= NULL;
    scalePtr->relief		= TK_RELIEF_FLAT;
    scalePtr->highlightWidth	= 0;
    scalePtr->highlightBorder	= NULL;
    scalePtr->highlightColorPtr	= NULL;
    scalePtr->inset		= 0;
    scalePtr->sliderLength	= 0;
    scalePtr->showValue		= 0;
    scalePtr->horizLabelY	= 0;
    scalePtr->horizValueY	= 0;
    scalePtr->horizTroughY	= 0;
    scalePtr->horizTickY	= 0;
    scalePtr->vertTickRightX	= 0;
    scalePtr->vertValueRightX	= 0;
    scalePtr->vertTroughX	= 0;
    scalePtr->vertLabelX	= 0;
    scalePtr->fontHeight	= 0;
    scalePtr->cursor		= NULL;
    scalePtr->takeFocusPtr	= NULL;
    scalePtr->flags		= NEVER_SET;

    Tk_SetClassProcs(scalePtr->tkwin, &scaleClass, scalePtr);
    Tk_CreateEventHandler(scalePtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    ScaleEventProc, scalePtr);

    if ((Tk_InitOptions(interp, (char *) scalePtr, optionTable, tkwin)
	    != TCL_OK) ||
	    (ConfigureScale(interp, scalePtr, objc - 2, objv + 2) != TCL_OK)) {
	Tk_DestroyWindow(scalePtr->tkwin);
	return TCL_ERROR;
    }

    /*
     * The widget was just created, no command callback must be invoked.
     */

    scalePtr->flags &= ~INVOKE_COMMAND;

    Tcl_SetObjResult(interp, TkNewWindowObj(scalePtr->tkwin));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleWidgetObjCmd --
 *
 *	This procedure is invoked to process the Tcl command that corresponds
 *	to a widget managed by this module. See the user documentation for
 *	details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
ScaleWidgetObjCmd(
    ClientData clientData,	/* Information about scale widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    TkScale *scalePtr = clientData;
    Tcl_Obj *objPtr;
    int index, result;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    result = Tcl_GetIndexFromObjStruct(interp, objv[1], commandNames,
	    sizeof(char *), "option", 0, &index);
    if (result != TCL_OK) {
	return result;
    }
    Tcl_Preserve(scalePtr);

    switch (index) {
    case COMMAND_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "cget option");
	    goto error;
	}
	objPtr = Tk_GetOptionValue(interp, (char *) scalePtr,
		scalePtr->optionTable, objv[2], scalePtr->tkwin);
	if (objPtr == NULL) {
	    goto error;
	}
	Tcl_SetObjResult(interp, objPtr);
	break;
    case COMMAND_CONFIGURE:
	if (objc <= 3) {
	    objPtr = Tk_GetOptionInfo(interp, (char *) scalePtr,
		    scalePtr->optionTable,
		    (objc == 3) ? objv[2] : NULL, scalePtr->tkwin);
	    if (objPtr == NULL) {
		goto error;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	} else {
	    result = ConfigureScale(interp, scalePtr, objc-2, objv+2);
	}
	break;
    case COMMAND_COORDS: {
	int x, y;
	double value;
	Tcl_Obj *coords[2];

	if ((objc != 2) && (objc != 3)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "coords ?value?");
	    goto error;
	}
	if (objc == 3) {
	    if (Tcl_GetDoubleFromObj(interp, objv[2], &value) != TCL_OK) {
		goto error;
	    }
	} else {
	    value = scalePtr->value;
	}
	if (scalePtr->orient == ORIENT_VERTICAL) {
	    x = scalePtr->vertTroughX + scalePtr->width/2
		    + scalePtr->borderWidth;
	    y = TkScaleValueToPixel(scalePtr, value);
	} else {
	    x = TkScaleValueToPixel(scalePtr, value);
	    y = scalePtr->horizTroughY + scalePtr->width/2
		    + scalePtr->borderWidth;
	}
	coords[0] = Tcl_NewIntObj(x);
	coords[1] = Tcl_NewIntObj(y);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, coords));
	break;
    }
    case COMMAND_GET: {
	double value;
	int x, y;

	if ((objc != 2) && (objc != 4)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "get ?x y?");
	    goto error;
	}
	if (objc == 2) {
	    value = scalePtr->value;
	} else {
	    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
		    (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
		goto error;
	    }
	    value = TkScalePixelToValue(scalePtr, x, y);
	}
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(scalePtr->valueFormat, value));
	break;
    }
    case COMMAND_IDENTIFY: {
	int x, y;
	const char *zone = "";

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 1, objv, "identify x y");
	    goto error;
	}
	if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	    goto error;
	}
	switch (TkpScaleElement(scalePtr, x, y)) {
	case TROUGH1:	zone = "trough1"; break;
	case SLIDER:	zone = "slider";  break;
	case TROUGH2:	zone = "trough2"; break;
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(zone, -1));
	break;
    }
    case COMMAND_SET: {
	double value;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 1, objv, "set value");
	    goto error;
	}
	if (Tcl_GetDoubleFromObj(interp, objv[2], &value) != TCL_OK) {
	    goto error;
	}
	if (scalePtr->state != STATE_DISABLED) {
	    TkScaleSetValue(scalePtr, value, 1, 1);
	}
	break;
    }
    }
    Tcl_Release(scalePtr);
    return result;

  error:
    Tcl_Release(scalePtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyScale --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *	clean up the internal structure of a button at a safe time (when
 *	no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the scale is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyScale(
    char *memPtr)	/* Info about scale widget. */
{
    register TkScale *scalePtr = (TkScale *) memPtr;

    scalePtr->flags |= SCALE_DELETED;

    Tcl_DeleteCommandFromToken(scalePtr->interp, scalePtr->widgetCmd);
    if (scalePtr->flags & REDRAW_PENDING) {
	Tcl_CancelIdleCall(TkpDisplayScale, scalePtr);
    }

    /*
     * Free up all the stuff that requires special handling, then let
     * Tk_FreeOptions handle all the standard option-related stuff.
     */

    if (scalePtr->varNamePtr != NULL) {
	Tcl_UntraceVar2(scalePtr->interp, Tcl_GetString(scalePtr->varNamePtr),
		NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		ScaleVarProc, scalePtr);
    }
    if (scalePtr->troughGC != NULL) {
	Tk_FreeGC(scalePtr->display, scalePtr->troughGC);
    }
    if (scalePtr->copyGC != NULL) {
	Tk_FreeGC(scalePtr->display, scalePtr->copyGC);
    }
    if (scalePtr->textGC != NULL) {
	Tk_FreeGC(scalePtr->display, scalePtr->textGC);
    }
    Tk_FreeConfigOptions((char *) scalePtr, scalePtr->optionTable,
	    scalePtr->tkwin);
    scalePtr->tkwin = NULL;
    TkpDestroyScale(scalePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureScale --
 *
 *	This procedure is called to process an argv/argc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a scale
 *	widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width, etc. get set
 *	for scalePtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureScale(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register TkScale *scalePtr,	/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc,			/* Number of valid entries in objv. */
    Tcl_Obj *const objv[])	/* Argument values. */
{
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error;
    double varValue;

    /*
     * Eliminate any existing trace on a variable monitored by the scale.
     */

    if (scalePtr->varNamePtr != NULL) {
	Tcl_UntraceVar2(interp, Tcl_GetString(scalePtr->varNamePtr),
		NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		ScaleVarProc, scalePtr);
    }

    for (error = 0; error <= 1; error++) {
	if (!error) {
	    /*
	     * First pass: set options to new values.
	     */

	    if (Tk_SetOptions(interp, (char *) scalePtr,
		    scalePtr->optionTable, objc, objv, scalePtr->tkwin,
		    &savedOptions, NULL) != TCL_OK) {
		continue;
	    }
	} else {
	    /*
	     * Second pass: restore options to old values.
	     */

	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}

	/*
	 * If the scale is tied to the value of a variable, then set the
	 * scale's value from the value of the variable, if it exists and it
	 * holds a valid double value.
	 */

	if (scalePtr->varNamePtr != NULL) {
	    double value;
	    Tcl_Obj *valuePtr;

	    valuePtr = Tcl_ObjGetVar2(interp, scalePtr->varNamePtr, NULL,
		    TCL_GLOBAL_ONLY);
	    if ((valuePtr != NULL) &&
		    (Tcl_GetDoubleFromObj(NULL, valuePtr, &value) == TCL_OK)) {
		scalePtr->value = TkRoundValueToResolution(scalePtr, value);
	    }
	}

	/*
	 * Several options need special processing, such as parsing the
	 * orientation and creating GCs.
	 */

	scalePtr->fromValue = TkRoundValueToResolution(scalePtr,
		scalePtr->fromValue);
	scalePtr->toValue = TkRoundValueToResolution(scalePtr, scalePtr->toValue);
	scalePtr->tickInterval = TkRoundIntervalToResolution(scalePtr,
		scalePtr->tickInterval);

	/*
	 * Make sure that the tick interval has the right sign so that
	 * addition moves from fromValue to toValue.
	 */

	if ((scalePtr->tickInterval < 0)
		^ ((scalePtr->toValue - scalePtr->fromValue) < 0)) {
	    scalePtr->tickInterval = -scalePtr->tickInterval;
	}

	ComputeFormat(scalePtr, 0);
	ComputeFormat(scalePtr, 1);

	scalePtr->labelLength = scalePtr->label ? (int)strlen(scalePtr->label) : 0;

	Tk_SetBackgroundFromBorder(scalePtr->tkwin, scalePtr->bgBorder);

	if (scalePtr->highlightWidth < 0) {
	    scalePtr->highlightWidth = 0;
	}
	scalePtr->inset = scalePtr->highlightWidth + scalePtr->borderWidth;
	break;
    }
    if (!error) {
	Tk_FreeSavedOptions(&savedOptions);
    }

    /*
     * Set the scale value to itself; all this does is to make sure that the
     * scale's value is within the new acceptable range for the scale. We
     * don't set the var here because we need to make special checks for
     * possibly changed varNamePtr.
     */

    TkScaleSetValue(scalePtr, scalePtr->value, 0, 1);

    /*
     * Reestablish the variable trace, if it is needed.
     */

    if (scalePtr->varNamePtr != NULL) {
	Tcl_Obj *valuePtr;

	/*
	 * Set the associated variable only when the new value differs from
	 * the current value, or the variable doesn't yet exist.
	 */

	valuePtr = Tcl_ObjGetVar2(interp, scalePtr->varNamePtr, NULL,
		TCL_GLOBAL_ONLY);
	if ((valuePtr == NULL) || (Tcl_GetDoubleFromObj(NULL,
		valuePtr, &varValue) != TCL_OK)) {
	    ScaleSetVariable(scalePtr);
	} else {
	    char varString[TCL_DOUBLE_SPACE], scaleString[TCL_DOUBLE_SPACE];

            Tcl_PrintDouble(NULL, varValue, varString);
            Tcl_PrintDouble(NULL, scalePtr->value, scaleString);
            if (strcmp(varString, scaleString)) {
		ScaleSetVariable(scalePtr);
	    }
	}
	Tcl_TraceVar2(interp, Tcl_GetString(scalePtr->varNamePtr),
		NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		ScaleVarProc, scalePtr);
    }

    ScaleWorldChanged(scalePtr);
    if (error) {
	Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScaleWorldChanged --
 *
 *	This procedure is called when the world has changed in some way and
 *	the widget needs to recompute all its graphics contexts and determine
 *	its new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Scale will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
ScaleWorldChanged(
    ClientData instanceData)	/* Information about widget. */
{
    XGCValues gcValues;
    GC gc;
    TkScale *scalePtr = instanceData;

    gcValues.foreground = scalePtr->troughColorPtr->pixel;
    gc = Tk_GetGC(scalePtr->tkwin, GCForeground, &gcValues);
    if (scalePtr->troughGC != NULL) {
	Tk_FreeGC(scalePtr->display, scalePtr->troughGC);
    }
    scalePtr->troughGC = gc;

    gcValues.font = Tk_FontId(scalePtr->tkfont);
    gcValues.foreground = scalePtr->textColorPtr->pixel;
    gc = Tk_GetGC(scalePtr->tkwin, GCForeground | GCFont, &gcValues);
    if (scalePtr->textGC != NULL) {
	Tk_FreeGC(scalePtr->display, scalePtr->textGC);
    }
    scalePtr->textGC = gc;

    if (scalePtr->copyGC == NULL) {
	gcValues.graphics_exposures = False;
	scalePtr->copyGC = Tk_GetGC(scalePtr->tkwin, GCGraphicsExposures,
		&gcValues);
    }
    scalePtr->inset = scalePtr->highlightWidth + scalePtr->borderWidth;

    /*
     * Recompute display-related information, and let the geometry manager
     * know how much space is needed now.
     */

    ComputeScaleGeometry(scalePtr);

    TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
}

 /*
  *----------------------------------------------------------------------
  *
  * MaxTickRoundingError --
  *
  *      Given the separation between values that can be displayed on ticks,
  *      this calculates the maximum magnitude of error for the displayed
  *      value. Tries to be clever by working out the increment in error
  *      between ticks rather than testing all of them, so may overestimate
  *      error if it is greater than 0.25 x the value separation.
  *
  * Results:
  *      Maximum error magnitude of tick numbers.
  *
  * Side effects:
  *      None.
  *
  *----------------------------------------------------------------------
  */

static double
MaxTickRoundingError(
    TkScale *scalePtr,		/* Information about scale widget. */
    double tickResolution)      /* Separation between displayable values. */
{
    double tickPosn, firstTickError, lastTickError, intervalError;
    int tickCount;

    /*
     * Compute the error for each tick-related measure.
     */

    tickPosn = scalePtr->fromValue / tickResolution;
    firstTickError = tickPosn - ScaleRound(tickPosn);

    tickPosn = scalePtr->tickInterval / tickResolution;
    intervalError = tickPosn - ScaleRound(tickPosn);

    tickCount = (int) ((scalePtr->toValue - scalePtr->fromValue) /
	    scalePtr->tickInterval);	/* not including first */
    lastTickError = ScaleMin(0.5,
	    fabs(firstTickError + tickCount * intervalError));

    /*
     * Compute the maximum cumulative rounding error.
     */

    return ScaleMax(fabs(firstTickError), lastTickError) * tickResolution;
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeFormat --
 *
 *	This procedure is invoked to recompute the "valueFormat" or
 *	"tickFormat" field of a scale's widget record, which determines how
 *	the value of the scale or one of its ticks is converted to a string.
 *
 * Results:
 *	None.
 *
 * Side effects: The valueFormat or tickFormat field of scalePtr is modified.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeFormat(
    TkScale *scalePtr,		/* Information about scale widget. */
    int forTicks)               /* Do for ticks rather than value */
{
    double maxValue, x;
    int mostSigDigit, numDigits, leastSigDigit, afterDecimal;
    int eDigits, fDigits;

    /*
     * Compute the displacement from the decimal of the most significant digit
     * required for any number in the scale's range.
     */

    maxValue = fabs(scalePtr->fromValue);
    x = fabs(scalePtr->toValue);
    if (x > maxValue) {
	maxValue = x;
    }
    if (maxValue == 0) {
	maxValue = 1;
    }
    mostSigDigit = ScaleDigit(maxValue);

    if (forTicks) {
	/*
	 * Display only enough digits to ensure adjacent ticks have different
	 * values.
	 */

	if (scalePtr->tickInterval != 0) {
	    leastSigDigit = ScaleDigit(scalePtr->tickInterval);

	    /*
	     * Now add more digits until max error is less than
	     * TICK_VALUES_DISPLAY_ACCURACY intervals
	     */

	    while (MaxTickRoundingError(scalePtr, pow(10, leastSigDigit))
		    > fabs(TICK_VALUES_DISPLAY_ACCURACY * scalePtr->tickInterval)) {
		--leastSigDigit;
	    }
	    numDigits = 1 + mostSigDigit - leastSigDigit;
	} else {
	    numDigits = 1;
	}
    } else {
	/*
	 * If the number of significant digits wasn't specified explicitly,
	 * compute it. It's the difference between the most significant digit
	 * needed to represent any number on the scale and the most
	 * significant digit of the smallest difference between numbers on the
	 * scale. In other words, display enough digits so that at least one
	 * digit will be different between any two adjacent positions of the
	 * scale.
	 */

	numDigits = scalePtr->digits;
	if (numDigits > TCL_MAX_PREC) {
	    numDigits = 0;
	}
	if (numDigits <= 0) {
	    if (scalePtr->resolution > 0) {
		/*
		 * A resolution was specified for the scale, so just use it.
		 */

		leastSigDigit = ScaleDigit(scalePtr->resolution);
	    } else {
		/*
		 * No resolution was specified, so compute the difference in
		 * value between adjacent pixels and use it for the least
		 * significant digit.
		 */

		x = fabs(scalePtr->fromValue - scalePtr->toValue);
		if (scalePtr->length > 0) {
		    x /= scalePtr->length;
		}
		if (x > 0) {
		    leastSigDigit = ScaleDigit(x);
		} else {
		    leastSigDigit = 0;
		}
	    }
	    numDigits = mostSigDigit - leastSigDigit + 1;
	    if (numDigits < 1) {
		numDigits = 1;
	    }
	}
    }

    /*
     * Compute the number of characters required using "e" format and "f"
     * format, and then choose whichever one takes fewer characters.
     */

    eDigits = numDigits + 4;
    if (numDigits > 1) {
	eDigits++;			/* Decimal point. */
    }
    afterDecimal = numDigits - mostSigDigit - 1;
    if (afterDecimal < 0) {
	afterDecimal = 0;
    }
    fDigits = (mostSigDigit >= 0) ? mostSigDigit + afterDecimal : afterDecimal;
    if (afterDecimal > 0) {
	fDigits++;			/* Decimal point. */
    }
    if (mostSigDigit < 0) {
	fDigits++;			/* Zero to left of decimal point. */
    }

    if (forTicks) {
	if (fDigits <= eDigits) {
	    sprintf(scalePtr->tickFormat, "%%.%df", afterDecimal);
	} else {
	    sprintf(scalePtr->tickFormat, "%%.%de", numDigits - 1);
	}
    } else {
	if (fDigits <= eDigits) {
	    sprintf(scalePtr->valueFormat, "%%.%df", afterDecimal);
	} else {
	    sprintf(scalePtr->valueFormat, "%%.%de", numDigits - 1);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeScaleGeometry --
 *
 *	This procedure is called to compute various geometrical information
 *	for a scale, such as where various things get displayed. It's called
 *	when the window is reconfigured.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Display-related numbers get changed in *scalePtr. The geometry manager
 *	gets told about the window's preferred size.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeScaleGeometry(
    register TkScale *scalePtr)	/* Information about widget. */
{
    char valueString[TCL_DOUBLE_SPACE];
    int tmp, valuePixels, tickPixels, x, y, extraSpace;
    Tk_FontMetrics fm;

    Tk_GetFontMetrics(scalePtr->tkfont, &fm);
    scalePtr->fontHeight = fm.linespace + SPACING;

    /*
     * Horizontal scales are simpler than vertical ones because all sizes are
     * the same (the height of a line of text); handle them first and then
     * quit.
     */

    if (scalePtr->orient == ORIENT_HORIZONTAL) {
	y = scalePtr->inset;
	extraSpace = 0;
	if (scalePtr->labelLength != 0) {
	    scalePtr->horizLabelY = y + SPACING;
	    y += scalePtr->fontHeight;
	    extraSpace = SPACING;
	}
	if (scalePtr->showValue) {
	    scalePtr->horizValueY = y + SPACING;
	    y += scalePtr->fontHeight;
	    extraSpace = SPACING;
	} else {
	    scalePtr->horizValueY = y;
	}
	y += extraSpace;
	scalePtr->horizTroughY = y;
	y += scalePtr->width + 2*scalePtr->borderWidth;
	if (scalePtr->tickInterval != 0) {
	    scalePtr->horizTickY = y + SPACING;
	    y += scalePtr->fontHeight + SPACING;
	}
	Tk_GeometryRequest(scalePtr->tkwin,
		scalePtr->length + 2*scalePtr->inset, y + scalePtr->inset);
	Tk_SetInternalBorder(scalePtr->tkwin, scalePtr->inset);
	return;
    }

    /*
     * Vertical scale: compute the amount of space needed to display the
     * scales value by formatting strings for the two end points; use
     * whichever length is longer.
     */

    if (snprintf(valueString, TCL_DOUBLE_SPACE, scalePtr->valueFormat,
            scalePtr->fromValue) < 0) {
        valueString[TCL_DOUBLE_SPACE - 1] = '\0';
    }
    valuePixels = Tk_TextWidth(scalePtr->tkfont, valueString, -1);

    if (snprintf(valueString, TCL_DOUBLE_SPACE, scalePtr->valueFormat,
            scalePtr->toValue) < 0) {
        valueString[TCL_DOUBLE_SPACE - 1] = '\0';
    }
    tmp = Tk_TextWidth(scalePtr->tkfont, valueString, -1);
    if (valuePixels < tmp) {
	valuePixels = tmp;
    }

    /*
     * Now do the same thing for the tick values
     */

    if (snprintf(valueString, TCL_DOUBLE_SPACE, scalePtr->tickFormat,
            scalePtr->fromValue) < 0) {
        valueString[TCL_DOUBLE_SPACE - 1] = '\0';
    }
    tickPixels = Tk_TextWidth(scalePtr->tkfont, valueString, -1);

    if (snprintf(valueString, TCL_DOUBLE_SPACE, scalePtr->tickFormat,
            scalePtr->toValue) < 0) {
        valueString[TCL_DOUBLE_SPACE - 1] = '\0';
    }
    tmp = Tk_TextWidth(scalePtr->tkfont, valueString, -1);
    if (tickPixels < tmp) {
	tickPixels = tmp;
    }

    /*
     * Assign x-locations to the elements of the scale, working from left to
     * right.
     */

    x = scalePtr->inset;
    if ((scalePtr->tickInterval != 0) && (scalePtr->showValue)) {
	scalePtr->vertTickRightX = x + SPACING + tickPixels;
	scalePtr->vertValueRightX = scalePtr->vertTickRightX + valuePixels
		+ fm.ascent/2;
	x = scalePtr->vertValueRightX + SPACING;
    } else if (scalePtr->tickInterval != 0) {
	scalePtr->vertTickRightX = x + SPACING + tickPixels;
	scalePtr->vertValueRightX = scalePtr->vertTickRightX;
	x = scalePtr->vertTickRightX + SPACING;
    } else if (scalePtr->showValue) {
	scalePtr->vertTickRightX = x;
	scalePtr->vertValueRightX = x + SPACING + valuePixels;
	x = scalePtr->vertValueRightX + SPACING;
    } else {
	scalePtr->vertTickRightX = x;
	scalePtr->vertValueRightX = x;
    }
    scalePtr->vertTroughX = x;
    x += 2*scalePtr->borderWidth + scalePtr->width;
    if (scalePtr->labelLength == 0) {
	scalePtr->vertLabelX = 0;
    } else {
	scalePtr->vertLabelX = x + fm.ascent/2;
	x = scalePtr->vertLabelX + fm.ascent/2
	    + Tk_TextWidth(scalePtr->tkfont, scalePtr->label,
		    scalePtr->labelLength);
    }
    Tk_GeometryRequest(scalePtr->tkwin, x + scalePtr->inset,
	    scalePtr->length + 2*scalePtr->inset);
    Tk_SetInternalBorder(scalePtr->tkwin, scalePtr->inset);
}

/*
 *--------------------------------------------------------------
 *
 * ScaleEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various events on
 *	scales.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up.
 *	When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ScaleEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkScale *scalePtr = clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
    } else if (eventPtr->type == DestroyNotify) {
	DestroyScale(clientData);
    } else if (eventPtr->type == ConfigureNotify) {
	ComputeScaleGeometry(scalePtr);
	TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scalePtr->flags |= GOT_FOCUS;
	    if (scalePtr->highlightWidth > 0) {
		TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scalePtr->flags &= ~GOT_FOCUS;
	    if (scalePtr->highlightWidth > 0) {
		TkEventuallyRedrawScale(scalePtr, REDRAW_ALL);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ScaleCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted. If the
 *	widget isn't already in the process of being destroyed, this command
 *	destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
ScaleCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    TkScale *scalePtr = clientData;
    Tk_Window tkwin = scalePtr->tkwin;

    /*
     * This procedure could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this procedure destroys the
     * widget.
     */

    if (!(scalePtr->flags & SCALE_DELETED)) {
	scalePtr->flags |= SCALE_DELETED;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkEventuallyRedrawScale --
 *
 *	Arrange for part or all of a scale widget to redrawn at the next
 *	convenient time in the future.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If "what" is REDRAW_SLIDER then just the slider and the value readout
 *	will be redrawn; if "what" is REDRAW_ALL then the entire widget will
 *	be redrawn.
 *
 *--------------------------------------------------------------
 */

void
TkEventuallyRedrawScale(
    register TkScale *scalePtr,	/* Information about widget. */
    int what)			/* What to redraw: REDRAW_SLIDER or
				 * REDRAW_ALL. */
{
    if ((what == 0) || (scalePtr->tkwin == NULL)
	    || !Tk_IsMapped(scalePtr->tkwin)) {
	return;
    }
    if (!(scalePtr->flags & REDRAW_PENDING)) {
	scalePtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(TkpDisplayScale, scalePtr);
    }
    scalePtr->flags |= what;
}

/*
 *--------------------------------------------------------------
 *
 * TkRoundValueToResolution, TkRoundIntervalToResolution --
 *
 *	Round a given floating-point value to the nearest multiple of the
 *	scale's resolution.
 *	TkRoundValueToResolution rounds an absolute value based on the from
 *	value as a reference.
 *	TkRoundIntervalToResolution rounds a relative value without
 *	reference, i.e.	it rounds an interval.
 *
 * Results:
 *	The return value is the rounded result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
TkRoundValueToResolution(
    TkScale *scalePtr,		/* Information about scale widget. */
    double value)		/* Value to round. */
{
    return TkRoundIntervalToResolution(scalePtr, value - scalePtr->fromValue)
            + scalePtr->fromValue;
}

double
TkRoundIntervalToResolution(
    TkScale *scalePtr,		/* Information about scale widget. */
    double value)		/* Value to round. */
{
    double rem, rounded, tick;

    if (scalePtr->resolution <= 0) {
	return value;
    }
    tick = floor(value/scalePtr->resolution);
    rounded = scalePtr->resolution * tick;
    rem = value - rounded;
    if (rem < 0) {
        if (rem <= -scalePtr->resolution/2) {
            rounded = (tick - 1.0) * scalePtr->resolution;
        }
    } else {
        if (rem >= scalePtr->resolution/2) {
            rounded = (tick + 1.0) * scalePtr->resolution;
        }
    }
    return rounded;
}

/*
 *----------------------------------------------------------------------
 *
 * ScaleVarProc --
 *
 *	This procedure is invoked by Tcl whenever someone modifies a variable
 *	associated with a scale widget.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The value displayed in the scale will change to match the variable's
 *	new value. If the variable has a bogus value then it is reset to the
 *	value of the scale.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static char *
ScaleVarProc(
    ClientData clientData,	/* Information about button. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Name of variable. */
    const char *name2,		/* Second part of variable name. */
    int flags)			/* Information about what happened. */
{
    register TkScale *scalePtr = clientData;
    const char *resultStr;
    double value;
    Tcl_Obj *valuePtr;
    int result;

    /*
     * If the variable is unset, then immediately recreate it unless the whole
     * interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
        if (!Tcl_InterpDeleted(interp) && scalePtr->varNamePtr) {
            ClientData probe = NULL;

            do {
                probe = Tcl_VarTraceInfo(interp,
                        Tcl_GetString(scalePtr->varNamePtr),
                        TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
                        ScaleVarProc, probe);
                if (probe == (ClientData)scalePtr) {
                    break;
                }
            } while (probe);
            if (probe) {
                /*
                 * We were able to fetch the unset trace for our
                 * varNamePtr, which means it is not unset and not
                 * the cause of this unset trace. Instead some outdated
                 * former variable must be, and we should ignore it.
                 */
                return NULL;
            }
	    Tcl_TraceVar2(interp, Tcl_GetString(scalePtr->varNamePtr),
		    NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    ScaleVarProc, clientData);
	    scalePtr->flags |= NEVER_SET;
	    TkScaleSetValue(scalePtr, scalePtr->value, 1, 0);
	}
	return NULL;
    }

    /*
     * If we came here because we updated the variable (in TkScaleSetValue),
     * then ignore the trace. Otherwise update the scale with the value of the
     * variable.
     */

    if (scalePtr->flags & SETTING_VAR) {
	return NULL;
    }
    resultStr = NULL;
    valuePtr = Tcl_ObjGetVar2(interp, scalePtr->varNamePtr, NULL,
	    TCL_GLOBAL_ONLY);
    result = Tcl_GetDoubleFromObj(interp, valuePtr, &value);
    if (result != TCL_OK) {
	resultStr = "can't assign non-numeric value to scale variable";
	ScaleSetVariable(scalePtr);
    } else {
	scalePtr->value = TkRoundValueToResolution(scalePtr, value);

	/*
	 * This code is a bit tricky because it sets the scale's value before
	 * calling TkScaleSetValue. This way, TkScaleSetValue won't bother to
	 * set the variable again or to invoke the -command. However, it also
	 * won't redisplay the scale, so we have to ask for that explicitly.
	 */

	TkScaleSetValue(scalePtr, scalePtr->value, 1, 0);
    }
    TkEventuallyRedrawScale(scalePtr, REDRAW_SLIDER);

    return (char *) resultStr;
}

/*
 *--------------------------------------------------------------
 *
 * TkScaleSetValue --
 *
 *	This procedure changes the value of a scale and invokes a Tcl command
 *	to reflect the current position of a scale
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional error-processing command
 *	may also be invoked. The scale's slider is redrawn.
 *
 *--------------------------------------------------------------
 */

void
TkScaleSetValue(
    register TkScale *scalePtr,	/* Info about widget. */
    double value,		/* New value for scale. Gets adjusted if it's
				 * off the scale. */
    int setVar,			/* Non-zero means reflect new value through to
				 * associated variable, if any. */
    int invokeCommand)		/* Non-zero means invoked -command option to
				 * notify of new value, 0 means don't. */
{
    value = TkRoundValueToResolution(scalePtr, value);
    if ((value < scalePtr->fromValue)
	    ^ (scalePtr->toValue < scalePtr->fromValue)) {
	value = scalePtr->fromValue;
    }
    if ((value > scalePtr->toValue)
	    ^ (scalePtr->toValue < scalePtr->fromValue)) {
	value = scalePtr->toValue;
    }
    if (scalePtr->flags & NEVER_SET) {
	scalePtr->flags &= ~NEVER_SET;
    } else if (scalePtr->value == value) {
	return;
    }
    scalePtr->value = value;

    /*
     * Schedule command callback invocation only if there is such a command
     * already registered, otherwise the callback would trigger later when
     * configuring the widget -command option even if the value did not change.
     */

    if ((invokeCommand) && (scalePtr->command != NULL)) {
	scalePtr->flags |= INVOKE_COMMAND;
    }
    TkEventuallyRedrawScale(scalePtr, REDRAW_SLIDER);

    if (setVar && scalePtr->varNamePtr) {
	ScaleSetVariable(scalePtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ScaleSetVariable --
 *
 *	This procedure sets the variable associated with a scale, if any.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Other write traces on the variable will trigger.
 *
 *--------------------------------------------------------------
 */

static void
ScaleSetVariable(
    register TkScale *scalePtr)	/* Info about widget. */
{
    if (scalePtr->varNamePtr != NULL) {
	char string[TCL_DOUBLE_SPACE];

        if (snprintf(string, TCL_DOUBLE_SPACE, scalePtr->valueFormat,
                scalePtr->value) < 0) {
            string[TCL_DOUBLE_SPACE - 1] = '\0';
        }
	scalePtr->flags |= SETTING_VAR;
	Tcl_ObjSetVar2(scalePtr->interp, scalePtr->varNamePtr, NULL,
		Tcl_NewStringObj(string, -1), TCL_GLOBAL_ONLY);
	scalePtr->flags &= ~SETTING_VAR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkScalePixelToValue --
 *
 *	Given a pixel within a scale window, return the scale reading
 *	corresponding to that pixel.
 *
 * Results:
 *	A double-precision scale reading. If the value is outside the legal
 *	range for the scale then it's rounded to the nearest end of the scale.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double
TkScalePixelToValue(
    register TkScale *scalePtr,	/* Information about widget. */
    int x, int y)		/* Coordinates of point within window. */
{
    double value, pixelRange;

    if (scalePtr->orient == ORIENT_VERTICAL) {
	pixelRange = Tk_Height(scalePtr->tkwin) - scalePtr->sliderLength
		- 2*scalePtr->inset - 2*scalePtr->borderWidth;
	value = y;
    } else {
	pixelRange = Tk_Width(scalePtr->tkwin) - scalePtr->sliderLength
		- 2*scalePtr->inset - 2*scalePtr->borderWidth;
	value = x;
    }

    if (pixelRange <= 0) {
	/*
	 * Not enough room for the slider to actually slide: just return the
	 * scale's current value.
	 */

	return scalePtr->value;
    }
    value -= scalePtr->sliderLength/2 + scalePtr->inset
	    + scalePtr->borderWidth;
    value /= pixelRange;
    if (value < 0) {
	value = 0;
    }
    if (value > 1) {
	value = 1;
    }
    value = scalePtr->fromValue +
		value * (scalePtr->toValue - scalePtr->fromValue);
    return TkRoundValueToResolution(scalePtr, value);
}

/*
 *----------------------------------------------------------------------
 *
 * TkScaleValueToPixel --
 *
 *	Given a reading of the scale, return the x-coordinate or y-coordinate
 *	corresponding to that reading, depending on whether the scale is
 *	vertical or horizontal, respectively.
 *
 * Results:
 *	An integer value giving the pixel location corresponding to reading.
 *	The value is restricted to lie within the defined range for the scale.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkScaleValueToPixel(
    register TkScale *scalePtr,	/* Information about widget. */
    double value)		/* Reading of the widget. */
{
    int y, pixelRange;
    double valueRange;

    valueRange = scalePtr->toValue - scalePtr->fromValue;
    pixelRange = ((scalePtr->orient == ORIENT_VERTICAL)
	    ? Tk_Height(scalePtr->tkwin) : Tk_Width(scalePtr->tkwin))
	- scalePtr->sliderLength - 2*scalePtr->inset - 2*scalePtr->borderWidth;
    if (valueRange == 0) {
	y = 0;
    } else {
	y = ScaleRound((value - scalePtr->fromValue) * pixelRange
		/ valueRange);
	if (y < 0) {
	    y = 0;
	} else if (y > pixelRange) {
	    y = pixelRange;
	}
    }
    y += scalePtr->sliderLength/2 + scalePtr->inset + scalePtr->borderWidth;
    return y;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
