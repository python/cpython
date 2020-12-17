/* 
 * tixNBFrame.c --
 *
 *	This module implements "tixNoteBookFrame" widgets.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000      Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixNBFrame.c,v 1.9 2008/02/28 22:41:11 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>

#define NUM_TAB_POINTS          6
#define ANCHOR_TABPAD           2
#define MIN_TABPADX             3
#define MIN_TABPADY             3
#define ACTIVE_TAB_ELEVATION    2
#define MAX_BORDER_WIDTH        4

/*
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */

typedef struct NoteBookFrameStruct {
    Tk_Window tkwin;		/* Window that embodies the widget.  NULL
				 * means window has been deleted but
				 * widget record hasn't been cleaned up yet. */
    Display *display;		/* X's token for the window's display. */
    Tcl_Interp *interp;		/* Interpreter associated with widget. */
    Tcl_Command widgetCmd;	/* Token for button's widget command. */

    /*
     * Information used when displaying widget:
     */
    int desiredWidth;		/* Desired narrow dimension of scrollbar,
				 * in pixels. */
    int width;			/* total width of the widget */
    int height;			/* total width of the widget */

    /*
     * Information used when displaying widget:
     */

    /* Border and general drawing */

    int borderWidth;		/* Width of 3-D borders. */
    Tk_3DBorder bgBorder;	/* Used for drawing background. */
    Tk_3DBorder focusBorder;	/* background of the "focus" tab. */
    Tk_3DBorder inactiveBorder;	/* background of the "inactive" tab(s) */
    XColor * backPageColorPtr;	/* the color used as the "back page" */
    GC backPageGC;		/* GC for drawing text in normal mode. */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    int tabPadx;                /* Horiz-paddings around the tab label/image */
    int tabPady;                /* Vert-paddings around the tab label/image */

    int isSlave;		/* if is in Slave mode, do not request for
				 * germetry */
    /* Text drawing */
    TixFont font;		/* Font used by the active (selected)
                                 * notebook tab. May be NULL */
    XColor *textColorPtr;	/* Color for drawing text. */
    XColor *disabledFg;		/* Foreground color when disabled.  NULL
				 * means use normalFg with a 50% stipple
				 * instead. */
    GC activeGC;		/* GC for drawing text on active tab. */
    GC disabledGC;		/* GC for drawing text on disabled tabs */
    GC anchorGC;		/* GC for drawing dotted anchor highlight. */
    GC inactiveAnchorGC;	/* GC for drawing dotted anchor highlight
                                 * inside inactive tabs.*/
    Pixmap gray;		/* Pixmap for displaying disabled text if
				 * disabledFg is NULL. */

    Cursor cursor;		/* Current cursor for window, or None. */

    struct _Tab * tabHead;
    struct _Tab * tabTail;
    struct _Tab * active;
    struct _Tab * focus;

    int tabsWidth;		/* total width  of the tabs */
    int tabsHeight;		/* total height of the tabs */

    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */

    unsigned int redrawing : 1;
    unsigned int gotFocus : 1;

} NoteBookFrame;

typedef struct _Tab {
    struct _Tab * next;

    NoteBookFrame * wPtr;
    char * name;

    Tk_Uid state;		/* State of Tab's for display purposes:
				 * normal or disabled. */
    Tk_Anchor anchor;

    char * text;
    int width, height;
    Tk_Justify justify;		/* Justification to use for multi-line text. */
    int wrapLength;
    int underline;		/* Index of character to underline.  < 0 means
				 * don't underline anything. */

    Tk_Image image;
    char * imageString;

    Pixmap bitmap;
} Tab;

typedef NoteBookFrame   WidgetRecord;
typedef NoteBookFrame * WidgetPtr;

/*
 * Information used for argv parsing.
 */
static Tk_ConfigSpec configSpecs[] = {

    {TK_CONFIG_BORDER, "-background", "background", "Background",
       DEF_NOTEBOOKFRAME_BG_COLOR, Tk_Offset(WidgetRecord, bgBorder),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-background", "background", "Background",
       DEF_NOTEBOOKFRAME_BG_MONO, Tk_Offset(WidgetRecord, bgBorder),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_COLOR, "-backpagecolor", "backPageColor", "BackPageColor",
       DEF_NOTEBOOKFRAME_BACKPAGE_COLOR,
       Tk_Offset(WidgetRecord, backPageColorPtr),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-backpagecolor", "backPageColor", "BackPageColor",
       DEF_NOTEBOOKFRAME_BACKPAGE_MONO,
       Tk_Offset(WidgetRecord, backPageColorPtr),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
       DEF_NOTEBOOKFRAME_BORDER_WIDTH, Tk_Offset(WidgetRecord, borderWidth),
       0},

    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
       DEF_NOTEBOOKFRAME_CURSOR, Tk_Offset(WidgetRecord, cursor),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
       "DisabledForeground", DEF_NOTEBOOKFRAME_DISABLED_FG_COLOR,
       Tk_Offset(WidgetRecord, disabledFg),
       TK_CONFIG_COLOR_ONLY|TK_CONFIG_NULL_OK},

    {TK_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
       "DisabledForeground", DEF_NOTEBOOKFRAME_DISABLED_FG_MONO,
       Tk_Offset(WidgetRecord, disabledFg),
       TK_CONFIG_MONO_ONLY|TK_CONFIG_NULL_OK},

    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_BORDER, "-focuscolor", "focusColor", "FocusColor",
       DEF_NOTEBOOKFRAME_FOCUS_COLOR, Tk_Offset(WidgetRecord, focusBorder),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-focuscolor", "focusColor", "FocusColor",
       DEF_NOTEBOOKFRAME_FOCUS_MONO, Tk_Offset(WidgetRecord, focusBorder),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_FONT, "-font", "font", "Font",
       DEF_NOTEBOOKFRAME_FONT, Tk_Offset(WidgetRecord, font), 0},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_NOTEBOOKFRAME_FG_COLOR, Tk_Offset(WidgetRecord, textColorPtr),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_NOTEBOOKFRAME_FG_MONO, Tk_Offset(WidgetRecord, textColorPtr),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_BORDER, "-inactivebackground", "inactiveBackground",
       "Background",
       DEF_NOTEBOOKFRAME_INACTIVE_BG_COLOR,
       Tk_Offset(WidgetRecord, inactiveBorder),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-inactivebackground", "inactiveBackground",
       "Background",
       DEF_NOTEBOOKFRAME_INACTIVE_BG_MONO,
       Tk_Offset(WidgetRecord, inactiveBorder),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
       DEF_NOTEBOOKFRAME_RELIEF, Tk_Offset(WidgetRecord, relief), 0},

    {TK_CONFIG_BOOLEAN, "-slave", "slave", "Slave",
       DEF_NOTEBOOKFRAME_SLAVE, Tk_Offset(WidgetRecord, isSlave), 0},

    {TK_CONFIG_PIXELS, "-tabpadx", "tabPadX", "Pad",
       DEF_NOTEBOOKFRAME_TABPADX, Tk_Offset(WidgetRecord, tabPadx), 0},

    {TK_CONFIG_PIXELS, "-tabpady", "tabPadY", "Pad",
       DEF_NOTEBOOKFRAME_TABPADY, Tk_Offset(WidgetRecord, tabPady), 0},

    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_NOTEBOOKFRAME_TAKE_FOCUS, Tk_Offset(WidgetRecord, takeFocus),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_PIXELS, "-width", "width", "Width",
       DEF_NOTEBOOKFRAME_WIDTH, Tk_Offset(WidgetRecord, desiredWidth), 0},
    
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

#define DEF_NBF_TAB_ANCHOR		"c"
#define DEF_NBF_TAB_BITMAP		""
#define DEF_NBF_TAB_IMAGE		""
#define DEF_NBF_TAB_JUSTIFY		"center"
#define DEF_NBF_TAB_TEXT		""
#define DEF_NBF_TAB_STATE		"normal"
#define DEF_NBF_TAB_UNDERLINE		"-1"
#define DEF_NBF_TAB_WRAPLENGTH		"0"

static Tk_ConfigSpec tabConfigSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", (char*)NULL, (char*)NULL,
       DEF_NBF_TAB_ANCHOR, Tk_Offset(Tab, anchor), 0},

    {TK_CONFIG_BITMAP, "-bitmap", (char*)NULL, (char*)NULL,
       DEF_NBF_TAB_BITMAP, Tk_Offset(Tab, bitmap), TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-image", (char*)NULL, (char*)NULL,
       DEF_NBF_TAB_IMAGE, Tk_Offset(Tab, imageString),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_JUSTIFY, "-justify", (char*)NULL, (char*)NULL,
	DEF_NBF_TAB_JUSTIFY, Tk_Offset(Tab, justify), 0},

    {TK_CONFIG_STRING, "-label", (char*)NULL, (char*)NULL,
       DEF_NBF_TAB_TEXT, Tk_Offset(Tab, text), TK_CONFIG_NULL_OK},

    {TK_CONFIG_UID, "-state", (char*)NULL, (char*)NULL,
       DEF_NBF_TAB_STATE, Tk_Offset(Tab, state), 0},

    {TK_CONFIG_INT, "-underline", (char*)NULL, (char*)NULL,
	DEF_NBF_TAB_UNDERLINE, Tk_Offset(Tab, underline), 0},

    {TK_CONFIG_PIXELS, "-wraplength", (char*)NULL, (char*)NULL,
       DEF_NBF_TAB_WRAPLENGTH, Tk_Offset(Tab, wrapLength), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

/* These are standard procedures for TK widgets implemeted in C
 */
static void		WidgetCmdDeletedProc(
			    ClientData clientData);
static int		WidgetConfigure(Tcl_Interp *interp,
			    WidgetPtr wPtr, int argc, CONST84 char **argv,
			    int flags);
static int		WidgetCommand(ClientData clientData,
			    Tcl_Interp *, int argc, CONST84 char **argv);
static void		WidgetComputeGeometry(WidgetPtr wPtr);
static void		WidgetDestroy(ClientData clientData);
static void		WidgetDisplay(ClientData clientData);
static void		WidgetEventProc(ClientData clientData,
			    XEvent *eventPtr);

	/* Extra procedures for this widget
	 */
static int		AddTab(WidgetPtr wPtr, CONST84 char * name,
			    CONST84 char ** argv, int argc);
static void		DeleteTab(Tab * tPtr);
static void		CancelRedrawWhenIdle(WidgetPtr wPtr);
static void		ComputeGeometry(WidgetPtr wPtr);
static void		DrawTab(WidgetPtr wPtr,
			    Tab * tPtr, int x, int isActive, int isFocused,
			    Drawable drawable);
static Tab * 		FindTab(Tcl_Interp *interp,
			    WidgetPtr wPtr, CONST84 char * name);
static void		GetTabPoints(
			    WidgetPtr wPtr, Tab * tPtr,
			    int x, XPoint *points, int isActive);
static void		ImageProc(ClientData clientData,
			    int x, int y, int width, int height,
			    int imgWidth, int imgHeight);
static void		RedrawWhenIdle(WidgetPtr wPtr);
static int		TabConfigure(WidgetPtr wPtr,
			    Tab *tPtr, CONST84 char ** argv, int argc);


/*
 *--------------------------------------------------------------
 *
 * Tix_NoteBookFrameCmd --
 *
 *	This procedure is invoked to process the "tixNoteBookFrame" Tcl
 *	command.  It creates a new "TixNoteBookFrame" widget.
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
Tix_NoteBookFrameCmd(clientData, interp, argc, argv)
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

    Tk_SetClass(tkwin, "TixNoteBookFrame");

    /*
     * Allocate and initialize the widget record.
     */
    wPtr = (WidgetPtr) ckalloc(sizeof(WidgetRecord));
    wPtr->tkwin 	 	= tkwin;
    wPtr->display 	 	= Tk_Display(tkwin);
    wPtr->interp 	 	= interp;
    wPtr->isSlave 	 	= 1;
    wPtr->desiredWidth  	= 0;
    wPtr->interp 	 	= interp;
    wPtr->width 	 	= 0;
    wPtr->borderWidth 	 	= 0;
    wPtr->bgBorder 	 	= NULL;
    wPtr->backPageGC	 	= None;
    wPtr->backPageColorPtr 	= NULL;
    wPtr->disabledFg 		= NULL;
    wPtr->gray	 		= None;
    wPtr->disabledGC 		= None;
    wPtr->inactiveBorder 	= NULL;
    wPtr->focusBorder	 	= NULL;
    wPtr->font	 	        = NULL;
    wPtr->textColorPtr	 	= NULL;
    wPtr->activeGC	 	= None;
    wPtr->anchorGC		= None;
    wPtr->inactiveAnchorGC	= None;
    wPtr->relief 	 	= TK_RELIEF_FLAT;
    wPtr->cursor 	 	= None;

    wPtr->tabHead 	 	= 0;
    wPtr->tabTail	 	= 0;
    wPtr->tabPadx	 	= 0;
    wPtr->tabPady	 	= 0;
    wPtr->active	 	= 0;
    wPtr->focus		 	= 0;
    wPtr->takeFocus	 	= 0;
    wPtr->redrawing	 	= 0;
    wPtr->gotFocus	 	= 0;

    Tk_CreateEventHandler(wPtr->tkwin, 
	ExposureMask|StructureNotifyMask|FocusChangeMask,
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
    unsigned int length;
    char c;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tk_Preserve((ClientData) wPtr);
    c = argv[1][0];
    length = strlen(argv[1]);

    if (((c == 'a') && (strncmp(argv[1], "activate", length) == 0))||
	((c == 'f') && (strncmp(argv[1], "focus", length) == 0))){
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " ", argv[1], " name\"", (char *) NULL);
	    goto error;
	}
	else {
	    if (strcmp(argv[2], "")==0) {
		if (c == 'a') {
		    wPtr->active = 0;
		    wPtr->focus  = 0;
		}
		else {
		    wPtr->focus  = 0;
		}
		RedrawWhenIdle(wPtr);
	    }
	    else {
		Tab * tPtr;

		for (tPtr=wPtr->tabHead; tPtr; tPtr=tPtr->next) {
		    if (strcmp(argv[2], tPtr->name) == 0) {
			if (c == 'a') {
			    wPtr->active = tPtr;
			    wPtr->focus  = tPtr;
			}
			else {
			    wPtr->focus  = tPtr;
			}
			RedrawWhenIdle(wPtr);
			goto done;
		    }
		}

		Tcl_AppendResult(interp, "unknown tab \"",
		    argv[0], "\"", (char *) NULL);
		goto error;
	    }
	}
    }
    else if ((c == 'a') && (strncmp(argv[1], "add", length) == 0)) {
	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be ",
		argv[0], " add name ?options?", (char *) NULL);
	    goto error;
	}
	else {
	    if (AddTab(wPtr, argv[2], argv+3, argc-3)!= TCL_OK) {
		goto error;
	    } else {
		WidgetComputeGeometry(wPtr);
		RedrawWhenIdle(wPtr);
	    }
	}
    }
    else if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)) {
	if (argc == 3) {
	    result = Tk_ConfigureValue(interp, wPtr->tkwin, configSpecs,
		(char *)wPtr, argv[2], 0);
	} else {
	    result = Tix_ArgcError(interp, argc, argv, 2, "option");
	}
    }
    else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)) {
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
    else if ((c == 'd') && (strncmp(argv[1], "delete", length) == 0)) {
	Tab * tPtr, * prev ;

	if (argc != 3) {
	    Tix_ArgcError(interp, argc, argv, 2, "page");
	    goto error;
	}

	/* Find the tab from the list */
	for (prev=tPtr=wPtr->tabHead; tPtr; prev=tPtr,tPtr=tPtr->next) {
	    if (strcmp(tPtr->name, argv[2])==0) {
		break;
	    }
	}
	if (tPtr == NULL) {
	    Tcl_AppendResult(wPtr->interp, 
		"Unknown tab \"", argv[2], "\"", (char*) NULL);
	    goto error;
	}
	if (tPtr == prev) {
	    if (wPtr->tabHead == wPtr->tabTail) {
		wPtr->tabHead = wPtr->tabTail = NULL;
	    } else {
		wPtr->tabHead = tPtr->next;
	    }
	} else {
	    if (tPtr == wPtr->tabTail) {
		wPtr->tabTail = prev;
	    }
	    prev->next = tPtr->next;
	}

	DeleteTab(tPtr);
	ComputeGeometry(wPtr);
	RedrawWhenIdle(wPtr);
    }
    else if ((c == 'g') && (strncmp(argv[1], "geometryinfo", length) == 0)) {
	char buff[20];

	ComputeGeometry(wPtr);
	sprintf(buff, "%d %d", wPtr->width, wPtr->height);

	Tcl_AppendResult(interp, buff, NULL);
    }
    else if ((c == 'i') && (strncmp(argv[1], "identify", length) == 0)) {
	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " identify x y\"", (char *) NULL);
	    goto error;
	}
	else {
	    int x, y, left, right;
	    Tab * tPtr;

	    if (Tcl_GetInt(interp, argv[2], &x) != TCL_OK) {
		goto error;
	    }
	    if (Tcl_GetInt(interp, argv[3], &y) != TCL_OK) {
		goto error;
	    }

	    if (y < wPtr->tabsHeight) {
		left = 0;
		for (tPtr=wPtr->tabHead; tPtr; tPtr=tPtr->next) {
		    right = left + (wPtr->borderWidth +  wPtr->tabPadx) * 2
		      + tPtr->width;

		    if (x >= left && x <= right && tPtr->state ==tixNormalUid){
			Tcl_AppendResult(interp, tPtr->name, NULL);
			goto done;
		    }
		    left = right;
		}
	    }

	    /*
	     * An empty string is returned to indicate "nothing selected"
	     */
	    Tcl_ResetResult(interp);
	}
    }
    else if ((c == 'i') && (strncmp(argv[1], "info", length) == 0)) {
	Tcl_ResetResult(interp);

	if (argc == 3 && strcmp(argv[2], "pages")==0 ) {
	    Tab * tPtr;

	    for (tPtr=wPtr->tabHead; tPtr; tPtr=tPtr->next) {
		Tcl_AppendElement(interp, tPtr->name);
	    }
	}
	else if (argc == 3 && strcmp(argv[2], "active")==0 ) {
	    if (wPtr->active) {
		Tcl_AppendResult(interp, wPtr->active->name, NULL);
	    }
	}
	else if (argc == 3 && strcmp(argv[2], "focus")==0 ) {
	    if (wPtr->focus) {
		Tcl_AppendResult(interp, wPtr->focus->name, NULL);
	    }
	}
	else if (argc == 3 && strcmp(argv[2], "focusnext")==0 ) {
	    Tab * next;
	    if (wPtr->focus) {
		if (wPtr->focus->next) {
		    next = wPtr->focus->next;
		} else {
		    next = wPtr->tabHead;
		}
		Tcl_AppendResult(interp, next->name, NULL);
	    }
	}
	else if (argc == 3 && strcmp(argv[2], "focusprev")==0 ) {
	    Tab * prev, *tPtr;

	    if (wPtr->focus==wPtr->tabHead) {
		prev = wPtr->tabTail;
	    }
	    else {
		for (prev=tPtr=wPtr->tabHead;tPtr; prev=tPtr,tPtr=tPtr->next) {
		    if (tPtr == wPtr->focus) {
			break;
		    }
		}
	    }

	    if (prev) {
		Tcl_AppendResult(interp, prev->name, NULL);
	    }
	}
	else {
	    Tcl_AppendResult(interp, "wrong number of arguments or ",
		"unknown option", NULL);
	    goto error;
	}
    }
    else if ((c == 'm') && (strncmp(argv[1], "move", length) == 0)) {

    }
    else if ((c == 'p') && (strncmp(argv[1], "pagecget", length) == 0)) {
	Tab * tPtr;

	if (argc != 4) {
	    Tix_ArgcError(interp, argc, argv, 2, "option");
	    goto error;
	}
	if ((tPtr=FindTab(interp, wPtr, argv[2])) == NULL) {
	    goto error;
	}
	result = Tk_ConfigureValue(interp, wPtr->tkwin, tabConfigSpecs,
	    (char *)tPtr, argv[3], 0);
    }
    else if ((c == 'p') && (strncmp(argv[1], "pageconfigure", length) == 0)) {
	Tab * tPtr;

	if (argc < 3) {
	    Tix_ArgcError(interp, argc, argv, 2, 
		"page ?option value ...?");
	    goto error;
	}
	if ((tPtr=FindTab(interp, wPtr, argv[2])) == NULL) {
	    goto error;
	}
	if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, wPtr->tkwin, tabConfigSpecs,
		(char *)tPtr, (char *) NULL, 0);
	} else if (argc == 4) {
	    result = Tk_ConfigureInfo(interp, wPtr->tkwin, tabConfigSpecs,
		(char *)tPtr, argv[3], 0);
	} else {
	    result = TabConfigure(wPtr, tPtr, argv+3, argc-3);
	}
    }
    else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
	    "\":  must be activate, add, configure, delete, ",
	    "geometryinfo, identify, move, pagecget or ",
	    "pageconfigure", (char *) NULL);
	goto error;
    }

  done:
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
 *	reconfigure) a Notebookframe widget.
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
    XGCValues gcValues;
    GC newGC;
    unsigned int mask;

    if (Tk_ConfigureWidget(interp, wPtr->tkwin, configSpecs,
	    argc, argv, (char *) wPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Tab paddings cannot be too small, or else we can't draw the
     * focus highlight nicely.
     */
    if (wPtr->tabPadx < MIN_TABPADX) {
	wPtr->tabPadx = MIN_TABPADX;
    } 
    if (wPtr->tabPady < MIN_TABPADY) {
	wPtr->tabPady = MIN_TABPADY;
    } 

    if (wPtr->borderWidth > MAX_BORDER_WIDTH) {
        wPtr->borderWidth = MAX_BORDER_WIDTH;
    }

    Tk_SetBackgroundFromBorder(wPtr->tkwin, wPtr->bgBorder);

    /*
     * Get the back page GC
     */
    gcValues.foreground = wPtr->backPageColorPtr->pixel;
    gcValues.graphics_exposures = False;
    newGC = Tk_GetGC(wPtr->tkwin, GCForeground|GCGraphicsExposures,
	    &gcValues);
    if (wPtr->backPageGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->backPageGC);
    }
    wPtr->backPageGC = newGC;

    /*
     * Get the active GC
     */
    gcValues.foreground = wPtr->textColorPtr->pixel;
    gcValues.background = Tk_3DBorderColor(wPtr->bgBorder)->pixel;
    gcValues.font = TixFontId(wPtr->font);
    gcValues.graphics_exposures = False;
    newGC = Tk_GetGC(wPtr->tkwin,
	 GCBackground|GCForeground|GCFont|GCGraphicsExposures,
	 &gcValues);
    if (wPtr->activeGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->activeGC);
    }
    wPtr->activeGC = newGC;

    /*
     * Get the disabled GC
     */
    if (wPtr->disabledFg != NULL) {
	gcValues.foreground = wPtr->disabledFg->pixel;
	gcValues.background = Tk_3DBorderColor(wPtr->bgBorder)->pixel;
	mask = GCForeground|GCBackground|GCFont;
    } else {
	gcValues.foreground = Tk_3DBorderColor(wPtr->bgBorder)->pixel;
	if (wPtr->gray == None) {
	    wPtr->gray = Tk_GetBitmap(interp, wPtr->tkwin,
			Tk_GetUid("gray50"));
	    if (wPtr->gray == None) {
		return TCL_ERROR;
	    }
	}
	gcValues.fill_style = FillStippled;
	gcValues.stipple = wPtr->gray;
	mask = GCForeground|GCFillStyle|GCFont|GCStipple;
    }
    gcValues.font = TixFontId(wPtr->font);
    newGC = Tk_GetGC(wPtr->tkwin, mask, &gcValues);
    if (wPtr->disabledGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->disabledGC);
    }
    wPtr->disabledGC = newGC;

    /*
     * GC for the dotted anchor lines drawn around the focused tab's
     * label/image.
     */
    newGC = Tix_GetAnchorGC(wPtr->tkwin, Tk_3DBorderColor(wPtr->bgBorder));
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->anchorGC);
    }
    wPtr->anchorGC = newGC;

    /*
     * GC for the dotted anchor lines drawn around the focused tab's
     * label/image.
     */
    newGC = Tix_GetAnchorGC(wPtr->tkwin,
            Tk_3DBorderColor(wPtr->inactiveBorder));
    if (wPtr->inactiveAnchorGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->inactiveAnchorGC);
    }
    wPtr->inactiveAnchorGC = newGC;

    WidgetComputeGeometry(wPtr);
    RedrawWhenIdle(wPtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * WidgetEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on Notebookframes.
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

    switch (eventPtr->type ) {
      case DestroyNotify:
	if (wPtr->tkwin != NULL) {
	    wPtr->tkwin = NULL;
	    Tcl_DeleteCommand(wPtr->interp, 
	        Tcl_GetCommandName(wPtr->interp, wPtr->widgetCmd));
	}
	CancelRedrawWhenIdle(wPtr);
	Tk_EventuallyFree((ClientData) wPtr, (Tix_FreeProc*)WidgetDestroy);
	break;

      case Expose:
      case ConfigureNotify:
	RedrawWhenIdle(wPtr);
	break;

      case FocusIn:
	if (eventPtr->xfocus.detail != NotifyVirtual) {
	    wPtr->gotFocus = 1;
	    if (wPtr->focus == NULL) {
		wPtr->focus = wPtr->active;
	    }
	    RedrawWhenIdle(wPtr);
	}
	break;

      case FocusOut:
	if (eventPtr->xfocus.detail != NotifyVirtual) {
	    wPtr->gotFocus = 0;
	    RedrawWhenIdle(wPtr);
	}
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetDestroy --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a Notebookframe at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the Notebookframe is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
WidgetDestroy(clientData)
    ClientData clientData;	/* Info about my widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    Tab * tPtr;

    for (tPtr=wPtr->tabHead; tPtr;) {
	Tab * toDelete;

	toDelete = tPtr;
	tPtr=tPtr->next;

	DeleteTab(toDelete);
    }

    if (wPtr->backPageGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->backPageGC);
    }
    if (wPtr->activeGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->activeGC);
    }
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->anchorGC);
    }
    if (wPtr->inactiveAnchorGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->inactiveAnchorGC);
    }
    if (wPtr->gray != None) {
	Tk_FreeBitmap(wPtr->display, wPtr->gray);
    }
    if (wPtr->disabledGC != None) {
	Tk_FreeGC(wPtr->display, wPtr->disabledGC);
    }
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

/*
 *----------------------------------------------------------------------
 *
 * FindTab --
 *
 *	Seraches for the Tab is the widget's tab list
 *----------------------------------------------------------------------
 */
static Tab * FindTab(interp, wPtr, name)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    CONST84 char * name;
{
    Tab *tPtr;
 
    for (tPtr=wPtr->tabHead; tPtr; tPtr=tPtr->next) {
	if (strcmp(tPtr->name, name) == 0) {
	    return tPtr;
	}
    }

    Tcl_AppendResult(interp, "Unknown tab \"", name, "\"", (char*) NULL);
    return NULL;
}


static int TabConfigure(wPtr, tPtr, argv, argc)
    WidgetPtr wPtr;
    Tab *tPtr;
    CONST84 char ** argv;
    int argc;
{
    if (Tk_ConfigureWidget(wPtr->interp, wPtr->tkwin, tabConfigSpecs,
	argc, argv, (char *)tPtr, TK_CONFIG_ARGV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Free the old images for the widget, if there were any.
     */
    if (tPtr->image != NULL) {
	Tk_FreeImage(tPtr->image);
	tPtr->image = NULL;
    }

    if (tPtr->imageString != NULL) {
	tPtr->image = Tk_GetImage(wPtr->interp, wPtr->tkwin,
	    tPtr->imageString, ImageProc, (ClientData) tPtr);
	if (tPtr->image == NULL) {
	    return TCL_ERROR;
	}
    }

    /*
     * Recalculate the size of the tab (minus the padding and borders)
     */

    if (tPtr->text != NULL) {
        TixComputeTextGeometry(wPtr->font, tPtr->text, -1,
                tPtr->wrapLength, &tPtr->width, &tPtr->height);
    }
    else if (tPtr->image != NULL) {
	Tk_SizeOfImage(tPtr->image, &tPtr->width, &tPtr->height);
    }
    else if (tPtr->bitmap != None) {
	Tk_SizeOfBitmap(wPtr->display, tPtr->bitmap, &tPtr->width,
	    &tPtr->height);
    }
    else {
	tPtr->width = 0;
	tPtr->height = 0;
    }

    WidgetComputeGeometry(wPtr);
    RedrawWhenIdle(wPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AddTab --
 *
 *	Adds a new tab into the list of tabs.
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
AddTab(wPtr, name, argv, argc)
    WidgetPtr wPtr;			/* Information about widget. */
    CONST84 char * name;		/* Arguments. */
    CONST84 char ** argv;
    int argc;
{
    Tab * tPtr;

    tPtr = (Tab*)ckalloc(sizeof(Tab));

    tPtr->next = NULL;
    tPtr->wPtr = wPtr;
    tPtr->name = tixStrDup(name);
    tPtr->state = tixNormalUid;
    tPtr->text = NULL;
    tPtr->width = 0;
    tPtr->height = 0;
    tPtr->justify = TK_JUSTIFY_CENTER;
    tPtr->wrapLength = 0;
    tPtr->underline = -1;
    tPtr->image = NULL;
    tPtr->imageString = NULL;
    tPtr->bitmap = None;
    tPtr->anchor = TK_ANCHOR_CENTER;

    if (TabConfigure(wPtr, tPtr, argv, argc) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Append the tab to the end of the list */

    if (wPtr->tabHead == 0) {
	wPtr->tabHead = wPtr->tabTail = tPtr;
    }
    else {
	/* Insert right after the tail */
	wPtr->tabTail->next = tPtr;
	wPtr->tabTail = tPtr;
    }

    return TCL_OK;
}

static void DeleteTab(tPtr)
    Tab * tPtr;
{
    if (tPtr->wPtr->focus == tPtr) {
	tPtr->wPtr->focus = 0;
    }
    if (tPtr->wPtr->active == tPtr) {
	tPtr->wPtr->active = 0;
    }
    if (tPtr->name) {
	ckfree(tPtr->name);
    }
    if (tPtr->image) {
	Tk_FreeImage(tPtr->image);
    }

    if (tPtr->wPtr->tkwin) {
	Tk_FreeOptions(tabConfigSpecs, (char *)tPtr, 
	    Tk_Display(tPtr->wPtr->tkwin), 0);
    }
    ckfree((char*)tPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RedrawWhenIdle --
 *
 *	Redraw this widget when idle
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void
RedrawWhenIdle(wPtr)
    WidgetPtr wPtr;			/* Information about widget. */
{
    if (! wPtr->redrawing && Tk_IsMapped(wPtr->tkwin)) {
	wPtr->redrawing = 1;
	Tk_DoWhenIdle(WidgetDisplay, (ClientData)wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CancelRedrawWhenIdle --
 *
 *	Redraw this widget when idle
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void
CancelRedrawWhenIdle(wPtr)
    WidgetPtr wPtr;			/* Information about widget. */
{
    if (wPtr->redrawing) {
	wPtr->redrawing = 0;
	Tk_CancelIdleCall(WidgetDisplay, (ClientData)wPtr);
    }
}

static void
GetTabPoints(wPtr, tPtr, x, points, isActive)
    WidgetPtr wPtr;
    Tab * tPtr;
    int x;
    XPoint *points;
    int isActive;
{
    /*
     * Starting clock-wise from points[0] (the lower-left corner of the tab)
     */

    points[0].x = x + wPtr->borderWidth;
    points[0].y = wPtr->tabsHeight;
    points[1].x = points[0].x;
    points[1].y = wPtr->borderWidth * 2;
    points[2].x = x + wPtr->borderWidth * 2;
    points[2].y = wPtr->borderWidth;

    points[3].x = x + tPtr->width + wPtr->tabPadx*2;
    points[3].y = points[2].y;
    points[4].x = points[3].x + wPtr->borderWidth;
    points[4].y = points[1].y;
    points[5].x = points[4].x;
    points[5].y = points[0].y;

    if (!isActive) {
        points[1].y += ACTIVE_TAB_ELEVATION;
        points[2].y += ACTIVE_TAB_ELEVATION;
        points[3].y += ACTIVE_TAB_ELEVATION;
        points[4].y += ACTIVE_TAB_ELEVATION;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawTab --
 *
 *	Draws one tab according to its position and text
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static void
DrawTab(wPtr, tPtr, x, isActive, isFocused, drawable)
    WidgetPtr wPtr;             /* The widget to draw */
    Tab * tPtr;                 /* The tab to draw. */
    int x;                      /* The x coordinate of the left edge
                                 * of the tab */
    int isActive;               /* Is this an active tab */
    int isFocused;              /* Should this tab be focused */
    Drawable drawable;          /* The drawable to render the widget
                                 * in */
{
    Tk_3DBorder border;
    XPoint points[NUM_TAB_POINTS];
    int drawX, drawY, extraH;
    GC gc, anchorGC;
    int tabHeight = wPtr->tabsHeight - ACTIVE_TAB_ELEVATION;

    /*
     * (1) Choose the right GC/border for drawing the tab
     */

    if (tPtr->state == tixNormalUid) {
        gc = wPtr->activeGC;
        if (isActive) {
            anchorGC = wPtr->anchorGC;
        } else {
            anchorGC = wPtr->inactiveAnchorGC;
        }
    } else {
	anchorGC = NULL;
        gc = wPtr->disabledGC;
    }

    if (isActive) {
	border = wPtr->bgBorder;
    } else {
	border = wPtr->inactiveBorder;
        tabHeight -= ACTIVE_TAB_ELEVATION;
    }

    /*
     * (2) Draw the tab border
     */

    GetTabPoints(wPtr, tPtr, x, points, isActive);
    drawX = x + wPtr->borderWidth + wPtr->tabPadx;
    drawY = wPtr->borderWidth + wPtr->tabPady;
    extraH = tabHeight - tPtr->height - wPtr->borderWidth - wPtr->tabPady *2;

    if (extraH > 0) {
        /*
         * Center the content vertically -- we may have contents
         * of different height in the tabs of this notebook frame.
         */

	switch (tPtr->anchor) {
	  case TK_ANCHOR_SW: case TK_ANCHOR_S: case TK_ANCHOR_SE:
	    drawY += extraH;
	    break;
	  case TK_ANCHOR_W: case TK_ANCHOR_CENTER: case TK_ANCHOR_E:
	    drawY += extraH/2;
	    break;
	  case TK_ANCHOR_N: case TK_ANCHOR_NE: case TK_ANCHOR_NW:
	    /*
	     * Do nothing.
	     */
	    break;
	}
    }

    if (!isActive) {
        drawY += ACTIVE_TAB_ELEVATION;
    }

#ifdef __WIN32__
    if (wPtr->borderWidth == 2) {
        /*
         * If the borderWidth is the standard value of 2, try to make
         * the 3D page tab look like native Win32 tabs.
         */

        int x = points[0].x - wPtr->borderWidth;
        int y = points[2].y - wPtr->borderWidth;
        int w = points[4].x - points[0].x + wPtr->borderWidth * 2;
        int h = points[0].y - points[2].y + wPtr->borderWidth * 2;

        if (tPtr != wPtr->tabHead && isActive) {
            /*
             * Make this active tab look more prominent
             */

            x -= 2;
            w += 2;
        }

        Tk_Fill3DRectangle(wPtr->tkwin, drawable,
	        border, x, y, w, h,
	        wPtr->borderWidth, TK_RELIEF_RAISED);

        /*
         * Round the edges of the 3D rect
         */

        XFillRectangle(wPtr->display, drawable, wPtr->backPageGC,
            x, y, 2, 1);
        XFillRectangle(wPtr->display, drawable, wPtr->backPageGC,
            x, y, 1, 2);
        XFillRectangle(wPtr->display, drawable, wPtr->backPageGC,
            x+w-2, y, 2, 1);
        XFillRectangle(wPtr->display, drawable, wPtr->backPageGC,
            x+w-1, y, 1, 2);

        /*
         * Fill in one missing pixel
         */

        XFillRectangle(wPtr->display, drawable,
                Tk_3DBorderGC(wPtr->tkwin, border, TK_3D_LIGHT_GC),
                x+1, y+1, 1, 1);
    } else {
        Tk_Fill3DPolygon(wPtr->tkwin, drawable,
                border, points, NUM_TAB_POINTS,
                wPtr->borderWidth, TK_RELIEF_SUNKEN);
    }
#else
    Tk_Fill3DPolygon(wPtr->tkwin, drawable,
            border, points, NUM_TAB_POINTS,
            wPtr->borderWidth, TK_RELIEF_SUNKEN);
#endif

    /*
     * (3) Draw the tab content
     */
    if (tPtr->text != NULL) {
        TixDisplayText(wPtr->display, drawable, wPtr->font,
	        tPtr->text, -1,
	    	drawX, drawY,
	        tPtr->wrapLength,
	        tPtr->justify,
	        tPtr->underline,
	        gc);
    } else if (tPtr->image != NULL) {
	Tk_RedrawImage(tPtr->image, 0, 0, tPtr->width, tPtr->height,
	    drawable, drawX, drawY);
    } else if (tPtr->bitmap != None) {
	XSetClipOrigin(wPtr->display, gc, drawX, drawY);
	XCopyPlane(wPtr->display, tPtr->bitmap, drawable,
		gc, 0, 0, (unsigned) tPtr->width, (unsigned) tPtr->height,
		drawX, drawY, 1);
	XSetClipOrigin(wPtr->display, gc, 0, 0);
    }

    if (isFocused && (anchorGC != NULL)) {
        int ancX = drawX - ANCHOR_TABPAD;
        int ancY = drawY - ANCHOR_TABPAD;
        int ancW = tPtr->width  + 2*ANCHOR_TABPAD;
        int ancH = tPtr->height + 2*ANCHOR_TABPAD;

	Tix_DrawAnchorLines(wPtr->display, drawable, anchorGC,
                ancX, ancY, ancW, ancH);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetDisplay --
 *
 *	Redraw this widget
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void
WidgetDisplay(clientData)
    ClientData clientData;			/* Information about widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    Tab * tPtr;
    int width, height;
    Drawable buffer;

    /* Now let's redraw */
    if (wPtr->tabHead == NULL) {
	/*
	 * no tabs to redraw: just draw the border
	 */
	if ((wPtr->bgBorder != NULL) && (wPtr->relief != TK_RELIEF_FLAT)) {
	    Tk_Fill3DRectangle(wPtr->tkwin, Tk_WindowId(wPtr->tkwin),
		wPtr->bgBorder, 0, 0,
		Tk_Width(wPtr->tkwin), Tk_Height(wPtr->tkwin),
		wPtr->borderWidth, wPtr->relief);
	}
    }
    else {
	int x, y, activex = 0;

	buffer = Tix_GetRenderBuffer(wPtr->display, Tk_WindowId(wPtr->tkwin),
	    Tk_Width(wPtr->tkwin), Tk_Height(wPtr->tkwin),
	    Tk_Depth(wPtr->tkwin));
	XFillRectangle(wPtr->display, buffer, wPtr->backPageGC, 0, 0,
		(unsigned) Tk_Width(wPtr->tkwin),
		(unsigned) Tk_Height(wPtr->tkwin));

	Tk_Fill3DRectangle(wPtr->tkwin, buffer,
		wPtr->bgBorder, 0, wPtr->tabsHeight,
		Tk_Width(wPtr->tkwin), 
		Tk_Height(wPtr->tkwin) - wPtr->tabsHeight,
		wPtr->borderWidth, wPtr->relief);

	/* Draw the tabs */
	x = 0;
	for (tPtr=wPtr->tabHead; tPtr; tPtr=tPtr->next) {
            int isActive = 0, isFocused = 0;

	    if (tPtr == wPtr->active) {
		activex = x;
                isActive = 1;
	    }
	    if (tPtr == wPtr->focus && wPtr->gotFocus) {
                isFocused = 1;
	    }

            DrawTab(wPtr, tPtr, x, isActive, isFocused, buffer);

	    x += (wPtr->borderWidth +  wPtr->tabPadx) * 2;
	    x += tPtr->width;
	}

	/* Draw the box */
	Tk_Draw3DRectangle(wPtr->tkwin, buffer,
		wPtr->bgBorder, 0, wPtr->tabsHeight,
		Tk_Width(wPtr->tkwin), 
		Tk_Height(wPtr->tkwin) - wPtr->tabsHeight,
		wPtr->borderWidth, wPtr->relief);

	if (wPtr->active != NULL) {
	    /*
	     * Fill up the gap between the active tab and the box
	     */
	    x = activex + wPtr->borderWidth;
	    y = wPtr->tabsHeight;
	    height = wPtr->borderWidth;
	    width = wPtr->active->width + wPtr->tabPadx*2;

#ifdef __WIN32__
            if (wPtr->borderWidth == 2) {
                x -= 1;
                width += 2;
                if (wPtr->active != wPtr->tabHead) {
                    x -= 2;
                    width += 2;
                }
            }
#endif
	    XFillRectangle(wPtr->display, buffer,
		Tk_3DBorderGC(wPtr->tkwin, wPtr->bgBorder, TK_3D_FLAT_GC),
		x, y, (unsigned) width, (unsigned) height);
	}

	if (buffer != Tk_WindowId(wPtr->tkwin)) {
	    XCopyArea(wPtr->display, buffer, Tk_WindowId(wPtr->tkwin),
	        wPtr->activeGC, 0, 0, (unsigned) Tk_Width(wPtr->tkwin),
		(unsigned) Tk_Height(wPtr->tkwin), 0, 0);
	    Tk_FreePixmap(wPtr->display, buffer);
	}
    }

    wPtr->redrawing = 0;
}

/*
 *--------------------------------------------------------------
 *
 * ComputeGeometry --
 *
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */
static void
ComputeGeometry(wPtr)
    WidgetPtr wPtr;
{
    Tab * tPtr;

    /* Calculate the requested size of the widget */
    if (wPtr->tabHead == NULL) {
	wPtr->width  = wPtr->borderWidth * 2;
	wPtr->height = wPtr->borderWidth * 2;

	wPtr->tabsWidth  = 0;
	wPtr->tabsHeight = 0;
    } else {
	wPtr->tabsWidth  = 0;
	wPtr->tabsHeight = 0;
	for (tPtr=wPtr->tabHead; tPtr; tPtr=tPtr->next) {
	    wPtr->tabsWidth += (wPtr->borderWidth + wPtr->tabPadx) * 2;
	    wPtr->tabsWidth += tPtr->width;

	    if (wPtr->tabsHeight < tPtr->height) {
		wPtr->tabsHeight = tPtr->height;
	    }
	}
	wPtr->tabsHeight += wPtr->tabPady*2 + wPtr->borderWidth;
        wPtr->tabsHeight += ACTIVE_TAB_ELEVATION;

	wPtr->width  = wPtr->tabsWidth;
	wPtr->height = wPtr->tabsHeight + wPtr->borderWidth*2;
    }
}

/*
 *--------------------------------------------------------------
 *
 * WidgetComputeGeometry --
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
static void
WidgetComputeGeometry(wPtr)
    WidgetPtr wPtr;
{
    int width;

    ComputeGeometry(wPtr);

    if (!wPtr->isSlave) {
	if (wPtr->desiredWidth > 0) {
	    width = wPtr->desiredWidth;
	} else {
	    width = wPtr->width;
	}
	Tk_GeometryRequest(wPtr->tkwin, width, wPtr->height);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ImageProc --
 *
 *	This procedure is invoked by the image code whenever the manager
 *	for an image does something that affects the size of contents
 *	of an image displayed in this widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the HList to get redisplayed.
 *
 *----------------------------------------------------------------------
 */
static void
ImageProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;		/* Pointer to widget record. */
    int x, y;				/* Upper left pixel (within image)
					 * that must be redisplayed. */
    int width, height;			/* Dimensions of area to redisplay
					 * (may be <= 0). */
    int imgWidth, imgHeight;		/* New dimensions of image. */
{
    Tab * tPtr;

    tPtr = (Tab *)clientData;

    WidgetComputeGeometry(tPtr->wPtr);
    RedrawWhenIdle(tPtr->wPtr);
}
