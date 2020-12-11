/*
 * tkScrollbar.c --
 *
 *	This module implements a scrollbar widgets for the Tk toolkit. A
 *	scrollbar displays a slider and two arrows; mouse clicks on features
 *	within the scrollbar cause scrolling commands to be invoked.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkScrollbar.h"
#include "default.h"

/*
 * Custom option for handling "-orient"
 */

static const Tk_CustomOption orientOption = {
    TkOrientParseProc, TkOrientPrintProc, NULL
};

/* non-const space for "-width" default value for scrollbars */
char tkDefScrollbarWidth[TCL_INTEGER_SPACE] = DEF_SCROLLBAR_WIDTH;

/*
 * Information used for argv parsing.
 */

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_SCROLLBAR_ACTIVE_BG_COLOR, Tk_Offset(TkScrollbar, activeBorder),
	TK_CONFIG_COLOR_ONLY, NULL},
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_SCROLLBAR_ACTIVE_BG_MONO, Tk_Offset(TkScrollbar, activeBorder),
	TK_CONFIG_MONO_ONLY, NULL},
    {TK_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
	DEF_SCROLLBAR_ACTIVE_RELIEF, Tk_Offset(TkScrollbar, activeRelief), 0, NULL},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_SCROLLBAR_BG_COLOR, Tk_Offset(TkScrollbar, bgBorder),
	TK_CONFIG_COLOR_ONLY, NULL},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_SCROLLBAR_BG_MONO, Tk_Offset(TkScrollbar, bgBorder),
	TK_CONFIG_MONO_ONLY, NULL},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", NULL, NULL, 0, 0, NULL},
    {TK_CONFIG_SYNONYM, "-bg", "background", NULL, NULL, 0, 0, NULL},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_SCROLLBAR_BORDER_WIDTH, Tk_Offset(TkScrollbar, borderWidth), 0, NULL},
    {TK_CONFIG_STRING, "-command", "command", "Command",
	DEF_SCROLLBAR_COMMAND, Tk_Offset(TkScrollbar, command),
	TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_SCROLLBAR_CURSOR, Tk_Offset(TkScrollbar, cursor), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_PIXELS, "-elementborderwidth", "elementBorderWidth",
	"BorderWidth", DEF_SCROLLBAR_EL_BORDER_WIDTH,
	Tk_Offset(TkScrollbar, elementBorderWidth), 0, NULL},
    {TK_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_SCROLLBAR_HIGHLIGHT_BG,
	Tk_Offset(TkScrollbar, highlightBgColorPtr), 0, NULL},
    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_SCROLLBAR_HIGHLIGHT,
	Tk_Offset(TkScrollbar, highlightColorPtr), 0, NULL},
    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness",
	DEF_SCROLLBAR_HIGHLIGHT_WIDTH, Tk_Offset(TkScrollbar, highlightWidth), 0, NULL},
    {TK_CONFIG_BOOLEAN, "-jump", "jump", "Jump",
	DEF_SCROLLBAR_JUMP, Tk_Offset(TkScrollbar, jump), 0, NULL},
    {TK_CONFIG_CUSTOM, "-orient", "orient", "Orient",
	DEF_SCROLLBAR_ORIENT, Tk_Offset(TkScrollbar, vertical), 0,
	&orientOption},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_SCROLLBAR_RELIEF, Tk_Offset(TkScrollbar, relief), 0, NULL},
    {TK_CONFIG_INT, "-repeatdelay", "repeatDelay", "RepeatDelay",
	DEF_SCROLLBAR_REPEAT_DELAY, Tk_Offset(TkScrollbar, repeatDelay), 0, NULL},
    {TK_CONFIG_INT, "-repeatinterval", "repeatInterval", "RepeatInterval",
	DEF_SCROLLBAR_REPEAT_INTERVAL, Tk_Offset(TkScrollbar, repeatInterval), 0, NULL},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_SCROLLBAR_TAKE_FOCUS, Tk_Offset(TkScrollbar, takeFocus),
	TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_COLOR, "-troughcolor", "troughColor", "Background",
	DEF_SCROLLBAR_TROUGH_COLOR, Tk_Offset(TkScrollbar, troughColorPtr),
	TK_CONFIG_COLOR_ONLY, NULL},
    {TK_CONFIG_COLOR, "-troughcolor", "troughColor", "Background",
	DEF_SCROLLBAR_TROUGH_MONO, Tk_Offset(TkScrollbar, troughColorPtr),
	TK_CONFIG_MONO_ONLY, NULL},
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
	tkDefScrollbarWidth, Tk_Offset(TkScrollbar, width), 0, NULL},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};

/*
 * Forward declarations for functions defined later in this file:
 */

static int		ConfigureScrollbar(Tcl_Interp *interp,
			    TkScrollbar *scrollPtr, int objc,
			    Tcl_Obj *const objv[], int flags);
static void		ScrollbarCmdDeletedProc(ClientData clientData);
static int		ScrollbarWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *, int objc, Tcl_Obj *const objv[]);

/*
 *--------------------------------------------------------------
 *
 * Tk_ScrollbarObjCmd --
 *
 *	This function is invoked to process the "scrollbar" Tcl command. See
 *	the user documentation for details on what it does.
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
Tk_ScrollbarObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument strings. */
{
    Tk_Window tkwin = clientData;
    register TkScrollbar *scrollPtr;
    Tk_Window newWin;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?-option value ...?");
	return TCL_ERROR;
    }

    newWin = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(objv[1]), NULL);
    if (newWin == NULL) {
	return TCL_ERROR;
    }

    Tk_SetClass(newWin, "Scrollbar");
    scrollPtr = TkpCreateScrollbar(newWin);

    Tk_SetClassProcs(newWin, &tkpScrollbarProcs, scrollPtr);

    /*
     * Initialize fields that won't be initialized by ConfigureScrollbar, or
     * which ConfigureScrollbar expects to have reasonable values (e.g.
     * resource pointers).
     */

    scrollPtr->tkwin = newWin;
    scrollPtr->display = Tk_Display(newWin);
    scrollPtr->interp = interp;
    scrollPtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(scrollPtr->tkwin), ScrollbarWidgetObjCmd,
	    scrollPtr, ScrollbarCmdDeletedProc);
    scrollPtr->vertical = 0;
    scrollPtr->width = 0;
    scrollPtr->command = NULL;
    scrollPtr->commandSize = 0;
    scrollPtr->repeatDelay = 0;
    scrollPtr->repeatInterval = 0;
    scrollPtr->borderWidth = 0;
    scrollPtr->bgBorder = NULL;
    scrollPtr->activeBorder = NULL;
    scrollPtr->troughColorPtr = NULL;
    scrollPtr->relief = TK_RELIEF_FLAT;
    scrollPtr->highlightWidth = 0;
    scrollPtr->highlightBgColorPtr = NULL;
    scrollPtr->highlightColorPtr = NULL;
    scrollPtr->inset = 0;
    scrollPtr->elementBorderWidth = -1;
    scrollPtr->arrowLength = 0;
    scrollPtr->sliderFirst = 0;
    scrollPtr->sliderLast = 0;
    scrollPtr->activeField = 0;
    scrollPtr->activeRelief = TK_RELIEF_RAISED;
    scrollPtr->totalUnits = 0;
    scrollPtr->windowUnits = 0;
    scrollPtr->firstUnit = 0;
    scrollPtr->lastUnit = 0;
    scrollPtr->firstFraction = 0.0;
    scrollPtr->lastFraction = 0.0;
    scrollPtr->cursor = NULL;
    scrollPtr->takeFocus = NULL;
    scrollPtr->flags = 0;

    if (ConfigureScrollbar(interp, scrollPtr, objc-2, objv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(scrollPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, TkNewWindowObj(scrollPtr->tkwin));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarWidgetObjCmd --
 *
 *	This function is invoked to process the Tcl command that corresponds
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
ScrollbarWidgetObjCmd(
    ClientData clientData,	/* Information about scrollbar widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument strings. */
{
    register TkScrollbar *scrollPtr = clientData;
    int result = TCL_OK;
    int length, cmdIndex;
    static const char *const commandNames[] = {
        "activate", "cget", "configure", "delta", "fraction",
        "get", "identify", "set", NULL
    };
    enum command {
        COMMAND_ACTIVATE, COMMAND_CGET, COMMAND_CONFIGURE, COMMAND_DELTA,
        COMMAND_FRACTION, COMMAND_GET, COMMAND_IDENTIFY, COMMAND_SET
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    /*
     * Parse the command by looking up the second argument in the list of
     * valid subcommand names
     */

    result = Tcl_GetIndexFromObj(interp, objv[1], commandNames,
	    "option", 0, &cmdIndex);
    if (result != TCL_OK) {
	return result;
    }
    Tcl_Preserve(scrollPtr);
    switch (cmdIndex) {
    case COMMAND_ACTIVATE: {
	int oldActiveField, c;

	if (objc == 2) {
	    const char *zone = "";

	    switch (scrollPtr->activeField) {
	    case TOP_ARROW:	zone = "arrow1"; break;
	    case SLIDER:	zone = "slider"; break;
	    case BOTTOM_ARROW:	zone = "arrow2"; break;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(zone, -1));
	    goto done;
	}
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "activate element");
	    goto error;
	}
	c = Tcl_GetStringFromObj(objv[2], &length)[0];
	oldActiveField = scrollPtr->activeField;
	if ((c == 'a') && (strcmp(Tcl_GetString(objv[2]), "arrow1") == 0)) {
	    scrollPtr->activeField = TOP_ARROW;
	} else if ((c == 'a') && (strcmp(Tcl_GetString(objv[2]), "arrow2") == 0)) {
	    scrollPtr->activeField = BOTTOM_ARROW;
	} else if ((c == 's') && (strncmp(Tcl_GetString(objv[2]), "slider", length) == 0)) {
	    scrollPtr->activeField = SLIDER;
	} else {
	    scrollPtr->activeField = OUTSIDE;
	}
	if (oldActiveField != scrollPtr->activeField) {
	    TkScrollbarEventuallyRedraw(scrollPtr);
	}
	break;
    }
    case COMMAND_CGET: {
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "cget option");
	    goto error;
	}
	result = Tk_ConfigureValue(interp, scrollPtr->tkwin,
		configSpecs, (char *) scrollPtr, Tcl_GetString(objv[2]), 0);
	break;
    }
    case COMMAND_CONFIGURE: {
	if (objc == 2) {
	    result = Tk_ConfigureInfo(interp, scrollPtr->tkwin,
		    configSpecs, (char *) scrollPtr, NULL, 0);
	} else if (objc == 3) {
	    result = Tk_ConfigureInfo(interp, scrollPtr->tkwin,
		    configSpecs, (char *) scrollPtr, Tcl_GetString(objv[2]), 0);
	} else {
	    result = ConfigureScrollbar(interp, scrollPtr, objc-2,
		    objv+2, TK_CONFIG_ARGV_ONLY);
	}
	break;
    }
    case COMMAND_DELTA: {
	int xDelta, yDelta, pixels, length;
	double fraction;

	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "delta xDelta yDelta");
	    goto error;
	}
	if ((Tcl_GetIntFromObj(interp, objv[2], &xDelta) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[3], &yDelta) != TCL_OK)) {
	    goto error;
	}
	if (scrollPtr->vertical) {
	    pixels = yDelta;
	    length = Tk_Height(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	} else {
	    pixels = xDelta;
	    length = Tk_Width(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	}
	if (length == 0) {
	    fraction = 0.0;
	} else {
	    fraction = ((double) pixels / (double) length);
	}
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(fraction));
	break;
    }
    case COMMAND_FRACTION: {
	int x, y, pos, length;
	double fraction;

	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "fraction x y");
	    goto error;
	}
	if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	    goto error;
	}
	if (scrollPtr->vertical) {
	    pos = y - (scrollPtr->arrowLength + scrollPtr->inset);
	    length = Tk_Height(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	} else {
	    pos = x - (scrollPtr->arrowLength + scrollPtr->inset);
	    length = Tk_Width(scrollPtr->tkwin) - 1
		    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
	}
	if (length == 0) {
	    fraction = 0.0;
	} else {
	    fraction = ((double) pos / (double) length);
	}
	if (fraction < 0) {
	    fraction = 0;
	} else if (fraction > 1.0) {
	    fraction = 1.0;
	}
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(fraction));
	break;
    }
    case COMMAND_GET: {
	Tcl_Obj *resObjs[4];

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "get");
	    goto error;
	}
	if (scrollPtr->flags & NEW_STYLE_COMMANDS) {
	    resObjs[0] = Tcl_NewDoubleObj(scrollPtr->firstFraction);
	    resObjs[1] = Tcl_NewDoubleObj(scrollPtr->lastFraction);
	    Tcl_SetObjResult(interp, Tcl_NewListObj(2, resObjs));
	} else {
	    resObjs[0] = Tcl_NewIntObj(scrollPtr->totalUnits);
	    resObjs[1] = Tcl_NewIntObj(scrollPtr->windowUnits);
	    resObjs[2] = Tcl_NewIntObj(scrollPtr->firstUnit);
	    resObjs[3] = Tcl_NewIntObj(scrollPtr->lastUnit);
	    Tcl_SetObjResult(interp, Tcl_NewListObj(4, resObjs));
	}
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
	switch (TkpScrollbarPosition(scrollPtr, x, y)) {
	case TOP_ARROW:		zone = "arrow1";  break;
	case TOP_GAP:		zone = "trough1"; break;
	case SLIDER:		zone = "slider";  break;
	case BOTTOM_GAP:	zone = "trough2"; break;
	case BOTTOM_ARROW:	zone = "arrow2";  break;
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(zone, -1));
	break;
    }
    case COMMAND_SET: {
	int totalUnits, windowUnits, firstUnit, lastUnit;

	if (objc == 4) {
	    double first, last;

	    if (Tcl_GetDoubleFromObj(interp, objv[2], &first) != TCL_OK) {
		goto error;
	    }
	    if (Tcl_GetDoubleFromObj(interp, objv[3], &last) != TCL_OK) {
		goto error;
	    }
	    if (first < 0) {
		scrollPtr->firstFraction = 0;
	    } else if (first > 1.0) {
		scrollPtr->firstFraction = 1.0;
	    } else {
		scrollPtr->firstFraction = first;
	    }
	    if (last < scrollPtr->firstFraction) {
		scrollPtr->lastFraction = scrollPtr->firstFraction;
	    } else if (last > 1.0) {
		scrollPtr->lastFraction = 1.0;
	    } else {
		scrollPtr->lastFraction = last;
	    }
	    scrollPtr->flags |= NEW_STYLE_COMMANDS;
	} else if (objc == 6) {
	    if (Tcl_GetIntFromObj(interp, objv[2], &totalUnits) != TCL_OK) {
		goto error;
	    }
	    if (totalUnits < 0) {
		totalUnits = 0;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[3], &windowUnits) != TCL_OK) {
		goto error;
	    }
	    if (windowUnits < 0) {
		windowUnits = 0;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[4], &firstUnit) != TCL_OK) {
		goto error;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[5], &lastUnit) != TCL_OK) {
		goto error;
	    }
	    if (totalUnits > 0) {
		if (lastUnit < firstUnit) {
		    lastUnit = firstUnit;
		}
	    } else {
		firstUnit = lastUnit = 0;
	    }
	    scrollPtr->totalUnits = totalUnits;
	    scrollPtr->windowUnits = windowUnits;
	    scrollPtr->firstUnit = firstUnit;
	    scrollPtr->lastUnit = lastUnit;
	    if (scrollPtr->totalUnits == 0) {
		scrollPtr->firstFraction = 0.0;
		scrollPtr->lastFraction = 1.0;
	    } else {
		scrollPtr->firstFraction = ((double) firstUnit)/totalUnits;
		scrollPtr->lastFraction = ((double) (lastUnit+1))/totalUnits;
	    }
	    scrollPtr->flags &= ~NEW_STYLE_COMMANDS;
	} else {
		Tcl_WrongNumArgs(interp, 1, objv, "set firstFraction lastFraction");
		Tcl_AppendResult(interp, " or \"", Tcl_GetString(objv[0]),
			" set totalUnits windowUnits firstUnit lastUnit\"", NULL);
	    goto error;
	}
	TkpComputeScrollbarGeometry(scrollPtr);
	TkScrollbarEventuallyRedraw(scrollPtr);
	break;
    }
    }

  done:
    Tcl_Release(scrollPtr);
    return result;

  error:
    Tcl_Release(scrollPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureScrollbar --
 *
 *	This function is called to process an argv/argc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a scrollbar
 *	widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width, etc. get set
 *	for scrollPtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureScrollbar(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register TkScrollbar *scrollPtr,
				/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc,			/* Number of valid entries in argv. */
    Tcl_Obj *const objv[],		/* Arguments. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    if (Tk_ConfigureWidget(interp, scrollPtr->tkwin, configSpecs, objc,
	    (const char **)objv, (char *) scrollPtr, flags|TK_CONFIG_OBJS) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * A few options need special processing, such as setting the background
     * from a 3-D border.
     */

    if (scrollPtr->command != NULL) {
        scrollPtr->commandSize = (int) strlen(scrollPtr->command);
    } else {
	scrollPtr->commandSize = 0;
    }

    /*
     * Configure platform specific options.
     */

    TkpConfigureScrollbar(scrollPtr);

    /*
     * Register the desired geometry for the window (leave enough space for
     * the two arrows plus a minimum-size slider, plus border around the whole
     * window, if any). Then arrange for the window to be redisplayed.
     */

    TkpComputeScrollbarGeometry(scrollPtr);
    TkScrollbarEventuallyRedraw(scrollPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkScrollbarEventProc --
 *
 *	This function is invoked by the Tk dispatcher for various events on
 *	scrollbars.
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

void
TkScrollbarEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkScrollbar *scrollPtr = clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	TkScrollbarEventuallyRedraw(scrollPtr);
    } else if (eventPtr->type == DestroyNotify) {
	TkpDestroyScrollbar(scrollPtr);
	if (scrollPtr->tkwin != NULL) {
	    scrollPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(scrollPtr->interp,
		    scrollPtr->widgetCmd);
	}
	if (scrollPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(TkpDisplayScrollbar, scrollPtr);
	}
	/*
	 * Free up all the stuff that requires special handling, then let
	 * Tk_FreeOptions handle all the standard option-related stuff.
	 */

	Tk_FreeOptions(configSpecs, (char*) scrollPtr, scrollPtr->display, 0);
	Tcl_EventuallyFree(scrollPtr, TCL_DYNAMIC);
    } else if (eventPtr->type == ConfigureNotify) {
	TkpComputeScrollbarGeometry(scrollPtr);
	TkScrollbarEventuallyRedraw(scrollPtr);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scrollPtr->flags |= GOT_FOCUS;
	    if (scrollPtr->highlightWidth > 0) {
		TkScrollbarEventuallyRedraw(scrollPtr);
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    scrollPtr->flags &= ~GOT_FOCUS;
	    if (scrollPtr->highlightWidth > 0) {
		TkScrollbarEventuallyRedraw(scrollPtr);
	    }
	}
    } else if (eventPtr->type == MapNotify) {
	TkScrollbarEventuallyRedraw(scrollPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ScrollbarCmdDeletedProc --
 *
 *	This function is invoked when a widget command is deleted. If the
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
ScrollbarCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    TkScrollbar *scrollPtr = clientData;
    Tk_Window tkwin = scrollPtr->tkwin;

    /*
     * This function could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this function destroys the
     * widget.
     */

    if (tkwin != NULL) {
	scrollPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkScrollbarEventuallyRedraw --
 *
 *	Arrange for one or more of the fields of a scrollbar to be redrawn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkScrollbarEventuallyRedraw(
    TkScrollbar *scrollPtr)	/* Information about widget. */
{
    if ((scrollPtr->tkwin == NULL) || !Tk_IsMapped(scrollPtr->tkwin)) {
	return;
    }
    if (!(scrollPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(TkpDisplayScrollbar, scrollPtr);
	scrollPtr->flags |= REDRAW_PENDING;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
