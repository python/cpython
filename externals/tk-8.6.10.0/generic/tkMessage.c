/*
 * tkMessage.c --
 *
 *	This module implements a message widgets for the Tk toolkit. A message
 *	widget displays a multi-line string in a window according to a
 *	particular aspect ratio.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Ajuba Solutions.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"

/*
 * A data structure of the following type is kept for each message widget
 * managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the message. NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up.*/
    Tk_OptionTable optionTable;	/* Table that defines options available for
				 * this widget. */
    Display *display;		/* Display containing widget. Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with message. */
    Tcl_Command widgetCmd;	/* Token for message's widget command. */

    /*
     * Information used when displaying widget:
     */

    char *string;		/* String displayed in message. */
    int numChars;		/* Number of characters in string, not
				 * including terminating NULL. */
    char *textVarName;		/* Name of variable (malloc'ed) or NULL.
				 * If non-NULL, message displays the contents
				 * of this variable. */
    Tk_3DBorder border;		/* Structure used to draw 3-D border and
				 * background. NULL means a border hasn't been
				 * created yet. */
    int borderWidth;		/* Width of border. */
    int relief;			/* 3-D effect: TK_RELIEF_RAISED, etc. */
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight
				 * area when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    Tk_Font tkfont;		/* Information about text font, or NULL. */
    XColor *fgColorPtr;		/* Foreground color in normal mode. */
    Tcl_Obj *padXPtr, *padYPtr;	/* Tcl_Obj rep's of padX, padY values. */
    int padX, padY;		/* User-requested extra space around text. */
    int width;			/* User-requested width, in pixels. 0 means
				 * compute width using aspect ratio below. */
    int aspect;			/* Desired aspect ratio for window
				 * (100*width/height). */
    int msgWidth;		/* Width in pixels needed to display
				 * message. */
    int msgHeight;		/* Height in pixels needed to display
				 * message. */
    Tk_Anchor anchor;		/* Where to position text within window region
				 * if window is larger or smaller than
				 * needed. */
    Tk_Justify justify;		/* Justification for text. */

    GC textGC;			/* GC for drawing text in normal mode. */
    Tk_TextLayout textLayout;	/* Saved layout information. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option; not used in the
				 * C code, but used by keyboard traversal
				 * scripts. Malloc'ed, but may be NULL. */
    int flags;			/* Various flags; see below for
				 * definitions. */
} Message;

/*
 * Flag bits for messages:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * GOT_FOCUS:			Non-zero means this button currently
 *				has the input focus.
 * MESSAGE_DELETED:		The message has been effectively deleted.
 */

#define REDRAW_PENDING		1
#define GOT_FOCUS		4
#define MESSAGE_DELETED		8

/*
 * Information used for argv parsing.
 */

static const Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MESSAGE_ANCHOR,
	 -1, Tk_Offset(Message, anchor), 0, 0, 0},
    {TK_OPTION_INT, "-aspect", "aspect", "Aspect", DEF_MESSAGE_ASPECT,
	 -1, Tk_Offset(Message, aspect), 0, 0, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	 DEF_MESSAGE_BG_COLOR, -1, Tk_Offset(Message, border), 0,
	 DEF_MESSAGE_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL,
	 0, -1, 0, "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL,
	 0, -1, 0, "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	 DEF_MESSAGE_BORDER_WIDTH, -1,
	 Tk_Offset(Message, borderWidth), 0, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	 DEF_MESSAGE_CURSOR, -1, Tk_Offset(Message, cursor),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL,
	 0, -1, 0, "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_MESSAGE_FONT, -1, Tk_Offset(Message, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_MESSAGE_FG, -1, Tk_Offset(Message, fgColorPtr), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	 "HighlightBackground", DEF_MESSAGE_HIGHLIGHT_BG, -1,
	 Tk_Offset(Message, highlightBgColorPtr), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	 DEF_MESSAGE_HIGHLIGHT, -1, Tk_Offset(Message, highlightColorPtr),
	 0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_MESSAGE_HIGHLIGHT_WIDTH, -1,
	 Tk_Offset(Message, highlightWidth), 0, 0, 0},
    {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
	DEF_MESSAGE_JUSTIFY, -1, Tk_Offset(Message, justify), 0, 0, 0},
    {TK_OPTION_PIXELS, "-padx", "padX", "Pad",
	 DEF_MESSAGE_PADX, Tk_Offset(Message, padXPtr),
	 Tk_Offset(Message, padX), 0, 0, 0},
    {TK_OPTION_PIXELS, "-pady", "padY", "Pad",
	 DEF_MESSAGE_PADY, Tk_Offset(Message, padYPtr),
	 Tk_Offset(Message, padY), 0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_MESSAGE_RELIEF, -1, Tk_Offset(Message, relief), 0, 0, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_MESSAGE_TAKE_FOCUS, -1, Tk_Offset(Message, takeFocus),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-text", "text", "Text",
	DEF_MESSAGE_TEXT, -1, Tk_Offset(Message, string), 0, 0, 0},
    {TK_OPTION_STRING, "-textvariable", "textVariable", "Variable",
	DEF_MESSAGE_TEXT_VARIABLE, -1, Tk_Offset(Message, textVarName),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-width", "width", "Width",
	DEF_MESSAGE_WIDTH, -1, Tk_Offset(Message, width), 0, 0 ,0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

/*
 * Forward declarations for functions defined later in this file:
 */

static void		MessageCmdDeletedProc(ClientData clientData);
static void		MessageEventProc(ClientData clientData,
			    XEvent *eventPtr);
static char *		MessageTextVarProc(ClientData clientData,
			    Tcl_Interp *interp, const char *name1,
			    const char *name2, int flags);
static int		MessageWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		MessageWorldChanged(ClientData instanceData);
static void		ComputeMessageGeometry(Message *msgPtr);
static int		ConfigureMessage(Tcl_Interp *interp, Message *msgPtr,
			    int objc, Tcl_Obj *const objv[], int flags);
static void		DestroyMessage(char *memPtr);
static void		DisplayMessage(ClientData clientData);

/*
 * The structure below defines message class behavior by means of functions
 * that can be invoked from generic window code.
 */

static const Tk_ClassProcs messageClass = {
    sizeof(Tk_ClassProcs),	/* size */
    MessageWorldChanged,	/* worldChangedProc */
    NULL,			/* createProc */
    NULL			/* modalProc */
};

/*
 *--------------------------------------------------------------
 *
 * Tk_MessageObjCmd --
 *
 *	This function is invoked to process the "message" Tcl command. See the
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
Tk_MessageObjCmd(
    ClientData clientData,	/* NULL. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    register Message *msgPtr;
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

    msgPtr = ckalloc(sizeof(Message));
    memset(msgPtr, 0, (size_t) sizeof(Message));

    /*
     * Set values for those fields that don't take a 0 or NULL value.
     */

    msgPtr->tkwin = tkwin;
    msgPtr->display = Tk_Display(tkwin);
    msgPtr->interp = interp;
    msgPtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(msgPtr->tkwin), MessageWidgetObjCmd, msgPtr,
	    MessageCmdDeletedProc);
    msgPtr->optionTable = optionTable;
    msgPtr->relief = TK_RELIEF_FLAT;
    msgPtr->textGC = NULL;
    msgPtr->anchor = TK_ANCHOR_CENTER;
    msgPtr->aspect = 150;
    msgPtr->justify = TK_JUSTIFY_LEFT;
    msgPtr->cursor = NULL;

    Tk_SetClass(msgPtr->tkwin, "Message");
    Tk_SetClassProcs(msgPtr->tkwin, &messageClass, msgPtr);
    Tk_CreateEventHandler(msgPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    MessageEventProc, msgPtr);
    if (Tk_InitOptions(interp, (char *)msgPtr, optionTable, tkwin) != TCL_OK) {
	Tk_DestroyWindow(msgPtr->tkwin);
	return TCL_ERROR;
    }

    if (ConfigureMessage(interp, msgPtr, objc-2, objv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(msgPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, TkNewWindowObj(msgPtr->tkwin));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * MessageWidgetObjCmd --
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
MessageWidgetObjCmd(
    ClientData clientData,	/* Information about message widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    register Message *msgPtr = clientData;
    static const char *const optionStrings[] = { "cget", "configure", NULL };
    enum options { MESSAGE_CGET, MESSAGE_CONFIGURE };
    int index;
    int result = TCL_OK;
    Tcl_Obj *objPtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], optionStrings,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_Preserve(msgPtr);

    switch ((enum options) index) {
    case MESSAGE_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	} else {
	    objPtr = Tk_GetOptionValue(interp, (char *) msgPtr,
		    msgPtr->optionTable, objv[2], msgPtr->tkwin);
	    if (objPtr == NULL) {
		result = TCL_ERROR;
	    } else {
		Tcl_SetObjResult(interp, objPtr);
		result = TCL_OK;
	    }
	}
	break;
    case MESSAGE_CONFIGURE:
	if (objc <= 3) {
	    objPtr = Tk_GetOptionInfo(interp, (char *) msgPtr,
		    msgPtr->optionTable, (objc == 3) ? objv[2] : NULL,
		    msgPtr->tkwin);
	    if (objPtr == NULL) {
		result = TCL_ERROR;
	    } else {
		Tcl_SetObjResult(interp, objPtr);
		result = TCL_OK;
	    }
	} else {
	    result = ConfigureMessage(interp, msgPtr, objc-2, objv+2, 0);
	}
	break;
    }

    Tcl_Release(msgPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMessage --
 *
 *	This function is invoked by Tcl_EventuallyFree or Tcl_Release to clean
 *	up the internal structure of a message at a safe time (when no-one is
 *	using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the message is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMessage(
    char *memPtr)		/* Info about message widget. */
{
    register Message *msgPtr = (Message *) memPtr;

    msgPtr->flags |= MESSAGE_DELETED;

    Tcl_DeleteCommandFromToken(msgPtr->interp, msgPtr->widgetCmd);
    if (msgPtr->flags & REDRAW_PENDING) {
	Tcl_CancelIdleCall(DisplayMessage, msgPtr);
    }

    /*
     * Free up all the stuff that requires special handling, then let
     * Tk_FreeConfigOptions handle all the standard option-related stuff.
     */

    if (msgPtr->textGC != NULL) {
	Tk_FreeGC(msgPtr->display, msgPtr->textGC);
    }
    if (msgPtr->textLayout != NULL) {
	Tk_FreeTextLayout(msgPtr->textLayout);
    }
    if (msgPtr->textVarName != NULL) {
	Tcl_UntraceVar2(msgPtr->interp, msgPtr->textVarName, NULL,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MessageTextVarProc, msgPtr);
    }
    Tk_FreeConfigOptions((char *) msgPtr, msgPtr->optionTable, msgPtr->tkwin);
    msgPtr->tkwin = NULL;
    ckfree(msgPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMessage --
 *
 *	This function is called to process an argv/argc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a message
 *	widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for msgPtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMessage(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register Message *msgPtr,	/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc,			/* Number of valid entries in argv. */
    Tcl_Obj *const objv[],	/* Arguments. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    Tk_SavedOptions savedOptions;

    /*
     * Eliminate any existing trace on a variable monitored by the message.
     */

    if (msgPtr->textVarName != NULL) {
	Tcl_UntraceVar2(interp, msgPtr->textVarName, NULL,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MessageTextVarProc, msgPtr);
    }

    if (Tk_SetOptions(interp, (char *) msgPtr, msgPtr->optionTable, objc, objv,
	    msgPtr->tkwin, &savedOptions, NULL) != TCL_OK) {
	Tk_RestoreSavedOptions(&savedOptions);
	return TCL_ERROR;
    }

    /*
     * If the message is to display the value of a variable, then set up a
     * trace on the variable's value, create the variable if it doesn't exist,
     * and fetch its current value.
     */

    if (msgPtr->textVarName != NULL) {
	const char *value;

	value = Tcl_GetVar2(interp, msgPtr->textVarName, NULL, TCL_GLOBAL_ONLY);
	if (value == NULL) {
	    Tcl_SetVar2(interp, msgPtr->textVarName, NULL, msgPtr->string,
		    TCL_GLOBAL_ONLY);
	} else {
	    if (msgPtr->string != NULL) {
		ckfree(msgPtr->string);
	    }
	    msgPtr->string = strcpy(ckalloc(strlen(value) + 1), value);
	}
	Tcl_TraceVar2(interp, msgPtr->textVarName, NULL,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MessageTextVarProc, msgPtr);
    }

    /*
     * A few other options need special processing, such as setting the
     * background from a 3-D border or handling special defaults that couldn't
     * be specified to Tk_ConfigureWidget.
     */

    msgPtr->numChars = Tcl_NumUtfChars(msgPtr->string, -1);

    if (msgPtr->highlightWidth < 0) {
	msgPtr->highlightWidth = 0;
    }

    Tk_FreeSavedOptions(&savedOptions);
    MessageWorldChanged(msgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MessageWorldChanged --
 *
 *	This function is called when the world has changed in some way and the
 *	widget needs to recompute all its graphics contexts and determine its
 *	new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Message will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
MessageWorldChanged(
    ClientData instanceData)	/* Information about widget. */
{
    XGCValues gcValues;
    GC gc = NULL;
    Tk_FontMetrics fm;
    Message *msgPtr = instanceData;

    if (msgPtr->border != NULL) {
	Tk_SetBackgroundFromBorder(msgPtr->tkwin, msgPtr->border);
    }

    gcValues.font = Tk_FontId(msgPtr->tkfont);
    gcValues.foreground = msgPtr->fgColorPtr->pixel;
    gc = Tk_GetGC(msgPtr->tkwin, GCForeground | GCFont, &gcValues);
    if (msgPtr->textGC != NULL) {
	Tk_FreeGC(msgPtr->display, msgPtr->textGC);
    }
    msgPtr->textGC = gc;

    Tk_GetFontMetrics(msgPtr->tkfont, &fm);
    if (msgPtr->padX < 0) {
	msgPtr->padX = fm.ascent / 2;
    }
    if (msgPtr->padY == -1) {
	msgPtr->padY = fm.ascent / 4;
    }

    /*
     * Recompute the desired geometry for the window, and arrange for the
     * window to be redisplayed.
     */

    ComputeMessageGeometry(msgPtr);
    if ((msgPtr->tkwin != NULL) && Tk_IsMapped(msgPtr->tkwin)
	    && !(msgPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayMessage, msgPtr);
	msgPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeMessageGeometry --
 *
 *	Compute the desired geometry for a message window, taking into account
 *	the desired aspect ratio for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tk_GeometryRequest is called to inform the geometry manager of the
 *	desired geometry for this window.
 *
 *--------------------------------------------------------------
 */

static void
ComputeMessageGeometry(
    register Message *msgPtr)	/* Information about window. */
{
    int width, inc, height;
    int thisWidth, thisHeight, maxWidth;
    int aspect, lowerBound, upperBound, inset;

    Tk_FreeTextLayout(msgPtr->textLayout);

    inset = msgPtr->borderWidth + msgPtr->highlightWidth;

    /*
     * Compute acceptable bounds for the final aspect ratio.
     */

    aspect = msgPtr->aspect/10;
    if (aspect < 5) {
	aspect = 5;
    }
    lowerBound = msgPtr->aspect - aspect;
    upperBound = msgPtr->aspect + aspect;

    /*
     * Do the computation in multiple passes: start off with a very wide
     * window, and compute its height. Then change the width and try again.
     * Reduce the size of the change and iterate until dimensions are found
     * that approximate the desired aspect ratio. Or, if the user gave an
     * explicit width then just use that.
     */

    if (msgPtr->width > 0) {
	width = msgPtr->width;
	inc = 0;
    } else {
	width = WidthOfScreen(Tk_Screen(msgPtr->tkwin))/2;
	inc = width/2;
    }

    for ( ; ; inc /= 2) {
	msgPtr->textLayout = Tk_ComputeTextLayout(msgPtr->tkfont,
		msgPtr->string, msgPtr->numChars, width, msgPtr->justify,
		0, &thisWidth, &thisHeight);
	maxWidth = thisWidth + 2 * (inset + msgPtr->padX);
	height = thisHeight + 2 * (inset + msgPtr->padY);

	if (inc <= 2) {
	    break;
	}
	aspect = (100 * maxWidth) / height;

	if (aspect < lowerBound) {
	    width += inc;
	} else if (aspect > upperBound) {
	    width -= inc;
	} else {
	    break;
	}
	Tk_FreeTextLayout(msgPtr->textLayout);
    }
    msgPtr->msgWidth = thisWidth;
    msgPtr->msgHeight = thisHeight;
    Tk_GeometryRequest(msgPtr->tkwin, maxWidth, height);
    Tk_SetInternalBorder(msgPtr->tkwin, inset);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayMessage --
 *
 *	This function redraws the contents of a message window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayMessage(
    ClientData clientData)	/* Information about window. */
{
    register Message *msgPtr = clientData;
    register Tk_Window tkwin = msgPtr->tkwin;
    int x, y;
    int borderWidth = msgPtr->highlightWidth;

    msgPtr->flags &= ~REDRAW_PENDING;
    if ((msgPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }
    if (msgPtr->border != NULL) {
	borderWidth += msgPtr->borderWidth;
    }
    if (msgPtr->relief == TK_RELIEF_FLAT) {
	borderWidth = msgPtr->highlightWidth;
    }
    Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), msgPtr->border,
	    borderWidth, borderWidth,
	    Tk_Width(tkwin) - 2 * borderWidth,
	    Tk_Height(tkwin) - 2 * borderWidth,
	    0, TK_RELIEF_FLAT);

    /*
     * Compute starting y-location for message based on message size and
     * anchor option.
     */

    TkComputeAnchor(msgPtr->anchor, tkwin, msgPtr->padX, msgPtr->padY,
	    msgPtr->msgWidth, msgPtr->msgHeight, &x, &y);
    Tk_DrawTextLayout(Tk_Display(tkwin), Tk_WindowId(tkwin), msgPtr->textGC,
	    msgPtr->textLayout, x, y, 0, -1);

    if (borderWidth > msgPtr->highlightWidth) {
	Tk_Draw3DRectangle(tkwin, Tk_WindowId(tkwin), msgPtr->border,
		msgPtr->highlightWidth, msgPtr->highlightWidth,
		Tk_Width(tkwin) - 2*msgPtr->highlightWidth,
		Tk_Height(tkwin) - 2*msgPtr->highlightWidth,
		msgPtr->borderWidth, msgPtr->relief);
    }
    if (msgPtr->highlightWidth != 0) {
	GC fgGC, bgGC;

	bgGC = Tk_GCForColor(msgPtr->highlightBgColorPtr, Tk_WindowId(tkwin));
	if (msgPtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(msgPtr->highlightColorPtr,Tk_WindowId(tkwin));
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC, msgPtr->highlightWidth,
		    Tk_WindowId(tkwin));
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC, msgPtr->highlightWidth,
		    Tk_WindowId(tkwin));
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * MessageEventProc --
 *
 *	This function is invoked by the Tk dispatcher for various events on
 *	messages.
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
MessageEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    Message *msgPtr = clientData;

    if (((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0))
	    || (eventPtr->type == ConfigureNotify)) {
	goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
	DestroyMessage(clientData);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    msgPtr->flags |= GOT_FOCUS;
	    if (msgPtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    msgPtr->flags &= ~GOT_FOCUS;
	    if (msgPtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    }
    return;

  redraw:
    if ((msgPtr->tkwin != NULL) && !(msgPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayMessage, msgPtr);
	msgPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MessageCmdDeletedProc --
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
MessageCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    Message *msgPtr = clientData;

    /*
     * This function could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this function destroys the
     * widget.
     */

    if (!(msgPtr->flags & MESSAGE_DELETED)) {
	Tk_DestroyWindow(msgPtr->tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * MessageTextVarProc --
 *
 *	This function is invoked when someone changes the variable whose
 *	contents are to be displayed in a message.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The text displayed in the message will change to match the variable.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
MessageTextVarProc(
    ClientData clientData,	/* Information about message. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Name of variable. */
    const char *name2,		/* Second part of variable name. */
    int flags)			/* Information about what happened. */
{
    register Message *msgPtr = clientData;
    const char *value;

    /*
     * If the variable is unset, then immediately recreate it unless the whole
     * interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
        if (!Tcl_InterpDeleted(interp) && msgPtr->textVarName) {
            ClientData probe = NULL;

            do {
                probe = Tcl_VarTraceInfo(interp,
                        msgPtr->textVarName,
                        TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
                        MessageTextVarProc, probe);
                if (probe == (ClientData)msgPtr) {
                    break;
                }
            } while (probe);
            if (probe) {
                /*
                 * We were able to fetch the unset trace for our
                 * textVarName, which means it is not unset and not
                 * the cause of this unset trace. Instead some outdated
                 * former variable must be, and we should ignore it.
                 */
                return NULL;
            }
	    Tcl_SetVar2(interp, msgPtr->textVarName, NULL, msgPtr->string,
		    TCL_GLOBAL_ONLY);
	    Tcl_TraceVar2(interp, msgPtr->textVarName, NULL,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    MessageTextVarProc, clientData);
	}
	return NULL;
    }

    value = Tcl_GetVar2(interp, msgPtr->textVarName, NULL, TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    if (msgPtr->string != NULL) {
	ckfree(msgPtr->string);
    }
    msgPtr->numChars = Tcl_NumUtfChars(value, -1);
    msgPtr->string = ckalloc(strlen(value) + 1);
    strcpy(msgPtr->string, value);
    ComputeMessageGeometry(msgPtr);

    if ((msgPtr->tkwin != NULL) && Tk_IsMapped(msgPtr->tkwin)
	    && !(msgPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayMessage, msgPtr);
	msgPtr->flags |= REDRAW_PENDING;
    }
    return NULL;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
