
/*	$Id: tixInputO.c,v 1.4 2008/02/28 04:05:29 hobbs Exp $	*/

/* 
 * tixInputO.c --
 *
 *	This module implements "InputOnly" widgets.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *
 */

#include <tkInt.h>
#include <tixPort.h>
#include <tix.h>

#ifndef MAC_OSX_TK
/*
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */

typedef struct Tix_InputOnlyStruct {
    Tk_Window tkwin;		/* Window that embodies the widget.  NULL
				 * means window has been deleted but
				 * widget record hasn't been cleaned up yet. */
    Tcl_Command widgetCmd;	/* Token for button's widget command. */
    Display *display;		/* X's token for the window's display. */
    Tcl_Interp *interp;		/* Interpreter associated with widget. */

    /*
     * Information used when displaying widget:
     */
    int width;
    int height;

    /* Cursor */
    Cursor cursor;		/* Current cursor for window, or None. */
    int changed;
} Tix_InputOnly;

typedef Tix_InputOnly   WidgetRecord;
typedef Tix_InputOnly * WidgetPtr;

/*
 * hint:: Place these into a default.f file
 */
#define DEF_INPUTONLY_CURSOR		""
#define DEF_INPUTONLY_WIDTH		"0"
#define DEF_INPUTONLY_HEIGHT		"0"

/*
 * Information used for argv parsing.
 */
static Tk_ConfigSpec configSpecs[] = {

    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
       DEF_INPUTONLY_CURSOR, Tk_Offset(WidgetRecord, cursor),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_PIXELS, "-height", "height", "Height",
       DEF_INPUTONLY_HEIGHT, Tk_Offset(WidgetRecord, height), 0},

    {TK_CONFIG_PIXELS, "-width", "width", "Width",
       DEF_INPUTONLY_WIDTH, Tk_Offset(WidgetRecord, width), 0},


    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		WidgetCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static int		WidgetConfigure _ANSI_ARGS_((Tcl_Interp *interp,
			    WidgetPtr wPtr, int argc, CONST84 char **argv,
			    int flags));
static void		WidgetDestroy _ANSI_ARGS_((ClientData clientData));
static void		WidgetEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		WidgetCommand _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *, int argc, CONST84 char **argv));
static void		Tix_MakeInputOnlyWindowExist _ANSI_ARGS_((
			    WidgetPtr wPtr));


#define INPUT_ONLY_EVENTS_MASK \
  KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask| \
  EnterWindowMask|LeaveWindowMask|PointerMotionMask| \
  VisibilityChangeMask|SubstructureNotifyMask| \
  FocusChangeMask|PropertyChangeMask

static XSetWindowAttributes inputOnlyAtts = {
    None,			/* background_pixmap */
    0,				/* background_pixel */
    None,			/* border_pixmap */
    0,				/* border_pixel */
    ForgetGravity,		/* bit_gravity */
    NorthWestGravity,		/* win_gravity */
    NotUseful,			/* backing_store */
    (unsigned) ~0,		/* backing_planes */
    0,				/* backing_pixel */
    False,			/* save_under */
    INPUT_ONLY_EVENTS_MASK,	/* event_mask */
    0,				/* do_not_propagate_mask */
    False,			/* override_redirect */
    None,			/* colormap */
    None			/* cursor */
};


static
void Tix_MakeInputOnlyWindowExist(wPtr)
    WidgetPtr wPtr;
{
    TkWindow* winPtr;
    Tcl_HashEntry *hPtr;
    int new;
    Window parent;

    winPtr = (TkWindow*) wPtr->tkwin;
    inputOnlyAtts.cursor = winPtr->atts.cursor;


    if (winPtr->flags & TK_TOP_LEVEL) {
	parent = XRootWindow(winPtr->display, winPtr->screenNum);
    } else {
	if (winPtr->parentPtr->window == None) {
	    Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
	}
	parent = winPtr->parentPtr->window;
    }

    winPtr->window = XCreateWindow(winPtr->display, 
	parent,
	winPtr->changes.x, winPtr->changes.y,
	(unsigned) winPtr->changes.width,
	(unsigned) winPtr->changes.height,
	0, 0,
	InputOnly,
	CopyFromParent,
	CWEventMask|CWCursor,
	&inputOnlyAtts);

    hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable,
	(char *) winPtr->window, &new);
    Tcl_SetHashValue(hPtr, winPtr);

    winPtr->dirtyAtts = 0;
    winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
    winPtr->inputContext = NULL;
#endif /* TK_USE_INPUT_METHODS */
}

/*
 *--------------------------------------------------------------
 *
 * Tix_InputOnlyCmd --
 *
 *	This procedure is invoked to process the "inputOnly" Tcl
 *	command.  It creates a new "InputOnly" widget.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	A new widget is created and configured.
 *
 *--------------------------------------------------------------
 */
int
Tix_InputOnlyCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window mainwin = (Tk_Window) clientData;
    WidgetPtr wPtr;
    Tk_Window tkwin;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args:  should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, mainwin, argv[1], (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    /*
     * Allocate and initialize the widget record.
     */
    wPtr = (WidgetPtr) ckalloc(sizeof(WidgetRecord));
    wPtr->tkwin 	= tkwin;
    wPtr->display 	= Tk_Display(tkwin);
    wPtr->interp 	= interp;
    wPtr->width 	= 0;
    wPtr->height 	= 0;
    wPtr->cursor 	= None;
    wPtr->changed 	= 0;

    Tk_SetClass(tkwin, "TixInputOnly");

    Tix_MakeInputOnlyWindowExist(wPtr);

    Tk_CreateEventHandler(wPtr->tkwin, StructureNotifyMask,
	WidgetEventProc, (ClientData) wPtr);
    wPtr->widgetCmd = Tcl_CreateCommand(interp, Tk_PathName(wPtr->tkwin),
	WidgetCommand, (ClientData) wPtr, WidgetCmdDeletedProc);
    if (WidgetConfigure(interp, wPtr, argc-2, argv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(wPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetResult(interp, Tk_PathName(wPtr->tkwin), TCL_STATIC);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * WidgetCommand --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
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
WidgetCommand(clientData, interp, argc, argv)
    ClientData clientData;		/* Information about the widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST84 char **argv;		/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int result = TCL_OK;
    size_t length;
    char c;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }

    Tk_Preserve((ClientData) wPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, wPtr->tkwin, configSpecs,
		    (char *) wPtr, (char *) NULL, 0);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, wPtr->tkwin, configSpecs,
		    (char *) wPtr, argv[2], 0);
	} else {
	    result = WidgetConfigure(interp, wPtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    }
    else if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)) {
	if (argc == 3) {
	    return Tk_ConfigureValue(interp, wPtr->tkwin, configSpecs,
		(char *)wPtr, argv[2], 0);
	} else {
	    return Tix_ArgcError(interp, argc, argv, 2, "option");
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\":  must be cget or configure", (char *) NULL);
	goto error;
    }

    Tk_Release((ClientData) wPtr);
    return result;

    error:
    Tk_Release((ClientData) wPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetConfigure --
 *
 *	This procedure is called to process an argv/argc list in
 *	conjunction with the Tk option database to configure (or
 *	reconfigure) a InputOnly widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for wPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */
static int
WidgetConfigure(interp, wPtr, argc, argv, flags)
    Tcl_Interp *interp;			/* Used for error reporting. */
    WidgetPtr wPtr;			/* Information about widget. */
    int argc;				/* Number of valid entries in argv. */
    CONST84 char **argv;		/* Arguments. */
    int flags;				/* Flags to pass to
					 * Tk_ConfigureWidget. */
{
    if (Tk_ConfigureWidget(interp, wPtr->tkwin, configSpecs,
	argc, argv, (char *) wPtr, flags) != TCL_OK) {

	return TCL_ERROR;
    }

    Tk_GeometryRequest(wPtr->tkwin, wPtr->width, wPtr->height);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * WidgetEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on InputOnlys.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
WidgetEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    switch (eventPtr->type) {
      case DestroyNotify:
	if (wPtr->tkwin != NULL) {
	    wPtr->tkwin = NULL;
	    Tcl_DeleteCommand(wPtr->interp, 
	        Tcl_GetCommandName(wPtr->interp, wPtr->widgetCmd));
	}
	Tk_EventuallyFree((ClientData) wPtr, (Tix_FreeProc*)WidgetDestroy);
	break;

      case MapNotify:
      case ConfigureNotify:
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetDestroy --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a InputOnly at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the InputOnly is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
WidgetDestroy(clientData)
    ClientData clientData;	/* Info about my widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    Tk_FreeOptions(configSpecs, (char *) wPtr, wPtr->display, 0);
    ckfree((char *) wPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
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
WidgetCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */
    if (wPtr->tkwin != NULL) {
	Tk_Window tkwin = wPtr->tkwin;
	wPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}
#endif
