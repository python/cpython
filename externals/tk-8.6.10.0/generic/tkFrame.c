/*
 * tkFrame.c --
 *
 *	This module implements "frame", "labelframe" and "toplevel" widgets
 *	for the Tk toolkit. Frames are windows with a background color and
 *	possibly a 3-D effect, but not much else in the way of attributes.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"

/*
 * The following enum is used to define the type of the frame.
 */

enum FrameType {
    TYPE_FRAME, TYPE_TOPLEVEL, TYPE_LABELFRAME
};

/*
 * A data structure of the following type is kept for each
 * frame that currently exists for this process:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the frame. NULL means
				 * that the window has been destroyed but the
				 * data structures haven't yet been cleaned
				 * up. */
    Display *display;		/* Display containing widget. Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with widget. Used to
				 * delete widget command. */
    Tcl_Command widgetCmd;	/* Token for frame's widget command. */
    Tk_OptionTable optionTable;	/* Table that defines configuration options
				 * available for this widget. */
    char *className;		/* Class name for widget (from configuration
				 * option). Malloc-ed. */
    enum FrameType type;	/* Type of widget, such as TYPE_FRAME. */
    char *screenName;		/* Screen on which widget is created. Non-null
				 * only for top-levels. Malloc-ed, may be
				 * NULL. */
    char *visualName;		/* Textual description of visual for window,
				 * from -visual option. Malloc-ed, may be
				 * NULL. */
    char *colormapName;		/* Textual description of colormap for window,
				 * from -colormap option. Malloc-ed, may be
				 * NULL. */
    char *menuName;		/* Textual description of menu to use for
				 * menubar. Malloc-ed, may be NULL. */
    Colormap colormap;		/* If not None, identifies a colormap
				 * allocated for this window, which must be
				 * freed when the window is deleted. */
    Tk_3DBorder border;		/* Structure used to draw 3-D border and
				 * background. NULL means no background or
				 * border. */
    int borderWidth;		/* Width of 3-D border (if any). */
    int relief;			/* 3-d effect: TK_RELIEF_RAISED etc. */
    int highlightWidth;		/* Width in pixels of highlight to draw around
				 * widget when it has the focus. 0 means don't
				 * draw a highlight. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight area
				 * when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int width;			/* Width to request for window. <= 0 means
				 * don't request any size. */
    int height;			/* Height to request for window. <= 0 means
				 * don't request any size. */
    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option; not used in the
				 * C code, but used by keyboard traversal
				 * scripts. Malloc'ed, but may be NULL. */
    int isContainer;		/* 1 means this window is a container, 0 means
				 * that it isn't. */
    char *useThis;		/* If the window is embedded, this points to
				 * the name of the window in which it is
				 * embedded (malloc'ed). For non-embedded
				 * windows this is NULL. */
    int flags;			/* Various flags; see below for
				 * definitions. */
    Tcl_Obj *padXPtr;		/* Value of -padx option: specifies how many
				 * pixels of extra space to leave on left and
				 * right of child area. */
    int padX;			/* Integer value corresponding to padXPtr. */
    Tcl_Obj *padYPtr;		/* Value of -padx option: specifies how many
				 * pixels of extra space to leave above and
				 * below child area. */
    int padY;			/* Integer value corresponding to padYPtr. */
} Frame;

/*
 * A data structure of the following type is kept for each labelframe widget
 * managed by this file:
 */

typedef struct {
    Frame frame;		/* A pointer to the generic frame structure.
				 * This must be the first element of the
				 * Labelframe. */
    /*
     * Labelframe specific configuration settings.
     */
    Tcl_Obj *textPtr;		/* Value of -text option: specifies text to
				 * display in button. */
    Tk_Font tkfont;		/* Value of -font option: specifies font to
				 * use for display text. */
    XColor *textColorPtr;	/* Value of -fg option: specifies foreground
				 * color in normal mode. */
    int labelAnchor;		/* Value of -labelanchor option: specifies
				 * where to place the label. */
    Tk_Window labelWin;		/* Value of -labelwidget option: Window to use
				 * as label for the frame. */
    /*
     * Labelframe specific fields for use with configuration settings above.
     */
    GC textGC;			/* GC for drawing text in normal mode. */
    Tk_TextLayout textLayout;	/* Stored text layout information. */
    XRectangle labelBox;	/* The label's actual size and position. */
    int labelReqWidth;		/* The label's requested width. */
    int labelReqHeight;		/* The label's requested height. */
    int labelTextX, labelTextY;	/* Position of the text to be drawn. */
} Labelframe;

/*
 * The following macros define how many extra pixels to leave around a label's
 * text.
 */

#define LABELSPACING 1
#define LABELMARGIN 4

/*
 * Flag bits for frames:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler has
 *				already been queued to redraw this window.
 * GOT_FOCUS:			Non-zero means this widget currently has the
 *				input focus.
 */

#define REDRAW_PENDING		1
#define GOT_FOCUS		4

/*
 * The following enum is used to define a type for the -labelanchor option of
 * the Labelframe widget. These values are used as indices into the string
 * table below.
 */

enum labelanchor {
    LABELANCHOR_E, LABELANCHOR_EN, LABELANCHOR_ES,
    LABELANCHOR_N, LABELANCHOR_NE, LABELANCHOR_NW,
    LABELANCHOR_S, LABELANCHOR_SE, LABELANCHOR_SW,
    LABELANCHOR_W, LABELANCHOR_WN, LABELANCHOR_WS
};

static const char *const labelAnchorStrings[] = {
    "e", "en", "es", "n", "ne", "nw", "s", "se", "sw", "w", "wn", "ws",
    NULL
};

/*
 * Information used for parsing configuration options. There are one common
 * table used by all and one table for each widget class.
 */

static const Tk_OptionSpec commonOptSpec[] = {
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_FRAME_BG_COLOR, -1, Tk_Offset(Frame, border),
	TK_OPTION_NULL_OK, DEF_FRAME_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
	NULL, 0, -1, 0, "-background", 0},
    {TK_OPTION_STRING, "-colormap", "colormap", "Colormap",
	DEF_FRAME_COLORMAP, -1, Tk_Offset(Frame, colormapName),
	TK_OPTION_NULL_OK, 0, 0},
    /*
     * Having -container is useless in a labelframe since a container has
     * no border. It should be deprecated.
     */
    {TK_OPTION_BOOLEAN, "-container", "container", "Container",
	DEF_FRAME_CONTAINER, -1, Tk_Offset(Frame, isContainer), 0, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_FRAME_CURSOR, -1, Tk_Offset(Frame, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-height", "height", "Height",
	DEF_FRAME_HEIGHT, -1, Tk_Offset(Frame, height), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_FRAME_HIGHLIGHT_BG, -1,
	Tk_Offset(Frame, highlightBgColorPtr), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_FRAME_HIGHLIGHT, -1, Tk_Offset(Frame, highlightColorPtr),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_FRAME_HIGHLIGHT_WIDTH, -1,
	Tk_Offset(Frame, highlightWidth), 0, 0, 0},
    {TK_OPTION_PIXELS, "-padx", "padX", "Pad",
	DEF_FRAME_PADX, Tk_Offset(Frame, padXPtr),
	Tk_Offset(Frame, padX), 0, 0, 0},
    {TK_OPTION_PIXELS, "-pady", "padY", "Pad",
	DEF_FRAME_PADY, Tk_Offset(Frame, padYPtr),
	Tk_Offset(Frame, padY), 0, 0, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_FRAME_TAKE_FOCUS, -1, Tk_Offset(Frame, takeFocus),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-visual", "visual", "Visual",
	DEF_FRAME_VISUAL, -1, Tk_Offset(Frame, visualName),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-width", "width", "Width",
	DEF_FRAME_WIDTH, -1, Tk_Offset(Frame, width), 0, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

static const Tk_OptionSpec frameOptSpec[] = {
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	NULL, 0, -1, 0, "-borderwidth", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_FRAME_BORDER_WIDTH, -1, Tk_Offset(Frame, borderWidth), 0, 0, 0},
    {TK_OPTION_STRING, "-class", "class", "Class",
	DEF_FRAME_CLASS, -1, Tk_Offset(Frame, className), 0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_FRAME_RELIEF, -1, Tk_Offset(Frame, relief), 0, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL,
	NULL, 0, 0, 0, commonOptSpec, 0}
};

static const Tk_OptionSpec toplevelOptSpec[] = {
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	NULL, 0, -1, 0, "-borderwidth", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_FRAME_BORDER_WIDTH, -1, Tk_Offset(Frame, borderWidth), 0, 0, 0},
    {TK_OPTION_STRING, "-class", "class", "Class",
	DEF_TOPLEVEL_CLASS, -1, Tk_Offset(Frame, className), 0, 0, 0},
    {TK_OPTION_STRING, "-menu", "menu", "Menu",
	DEF_TOPLEVEL_MENU, -1, Tk_Offset(Frame, menuName),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_FRAME_RELIEF, -1, Tk_Offset(Frame, relief), 0, 0, 0},
    {TK_OPTION_STRING, "-screen", "screen", "Screen",
	DEF_TOPLEVEL_SCREEN, -1, Tk_Offset(Frame, screenName),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-use", "use", "Use",
	DEF_TOPLEVEL_USE, -1, Tk_Offset(Frame, useThis),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL,
	NULL, 0, 0, 0, commonOptSpec, 0}
};

static const Tk_OptionSpec labelframeOptSpec[] = {
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	NULL, 0, -1, 0, "-borderwidth", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_LABELFRAME_BORDER_WIDTH, -1, Tk_Offset(Frame, borderWidth),
	0, 0, 0},
    {TK_OPTION_STRING, "-class", "class", "Class",
	DEF_LABELFRAME_CLASS, -1, Tk_Offset(Frame, className), 0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", NULL,
	NULL, 0, -1, 0, "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_LABELFRAME_FONT, -1, Tk_Offset(Labelframe, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_LABELFRAME_FG, -1, Tk_Offset(Labelframe, textColorPtr), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-labelanchor", "labelAnchor", "LabelAnchor",
	DEF_LABELFRAME_LABELANCHOR, -1, Tk_Offset(Labelframe, labelAnchor),
	0, labelAnchorStrings, 0},
    {TK_OPTION_WINDOW, "-labelwidget", "labelWidget", "LabelWidget",
	NULL, -1, Tk_Offset(Labelframe, labelWin), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_LABELFRAME_RELIEF, -1, Tk_Offset(Frame, relief), 0, 0, 0},
    {TK_OPTION_STRING, "-text", "text", "Text",
	DEF_LABELFRAME_TEXT, Tk_Offset(Labelframe, textPtr), -1,
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL,
	NULL, 0, 0, 0, commonOptSpec, 0}
};

/*
 * Class names for widgets, indexed by FrameType.
 */

static const char *const classNames[] = {"Frame", "Toplevel", "Labelframe"};

/*
 * The following table maps from FrameType to the option template for that
 * class of widgets.
 */

static const Tk_OptionSpec *const optionSpecs[] = {
    frameOptSpec,
    toplevelOptSpec,
    labelframeOptSpec,
};

/*
 * Forward declarations for functions defined later in this file:
 */

static void		ComputeFrameGeometry(Frame *framePtr);
static int		ConfigureFrame(Tcl_Interp *interp, Frame *framePtr,
			    int objc, Tcl_Obj *const objv[]);
static int		CreateFrame(ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const argv[],
			    enum FrameType type, const char *appName);
static void		DestroyFrame(void *memPtr);
static void		DestroyFramePartly(Frame *framePtr);
static void		DisplayFrame(ClientData clientData);
static void		FrameCmdDeletedProc(ClientData clientData);
static void		FrameEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		FrameLostSlaveProc(ClientData clientData,
			    Tk_Window tkwin);
static void		FrameRequestProc(ClientData clientData,
			    Tk_Window tkwin);
static void		FrameStructureProc(ClientData clientData,
			    XEvent *eventPtr);
static int		FrameWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		FrameWorldChanged(ClientData instanceData);
static void		MapFrame(ClientData clientData);

/*
 * The structure below defines frame class behavior by means of functions that
 * can be invoked from generic window code.
 */

static const Tk_ClassProcs frameClass = {
    sizeof(Tk_ClassProcs),	/* size */
    FrameWorldChanged,		/* worldChangedProc */
    NULL,			/* createProc */
    NULL			/* modalProc */
};

/*
 * The structure below defines the official type record for the labelframe's
 * geometry manager:
 */

static const Tk_GeomMgr frameGeomType = {
    "labelframe",		/* name */
    FrameRequestProc,		/* requestProc */
    FrameLostSlaveProc		/* lostSlaveProc */
};

/*
 *--------------------------------------------------------------
 *
 * Tk_FrameObjCmd, Tk_ToplevelObjCmd, Tk_LabelframeObjCmd --
 *
 *	These functions are invoked to process the "frame", "toplevel" and
 *	"labelframe" Tcl commands. See the user documentation for details on
 *	what they do.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation. These functions are just wrappers; they
 *	call CreateFrame to do all of the real work.
 *
 *--------------------------------------------------------------
 */

int
Tk_FrameObjCmd(
    ClientData clientData,	/* Either NULL or pointer to option table. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return CreateFrame(clientData, interp, objc, objv, TYPE_FRAME, NULL);
}

int
Tk_ToplevelObjCmd(
    ClientData clientData,	/* Either NULL or pointer to option table. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return CreateFrame(clientData, interp, objc, objv, TYPE_TOPLEVEL, NULL);
}

int
Tk_LabelframeObjCmd(
    ClientData clientData,	/* Either NULL or pointer to option table. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return CreateFrame(clientData, interp, objc, objv, TYPE_LABELFRAME, NULL);
}

/*
 *--------------------------------------------------------------
 *
 * TkCreateFrame --
 *
 *	This function is the old command function for the "frame" and
 *	"toplevel" commands. Now it is used directly by Tk_Init to create a
 *	new main window. See the user documentation for the "frame" and
 *	"toplevel" commands for details on what it does.
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
TkCreateFrame(
    ClientData clientData,	/* Either NULL or pointer to option table. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char *const *argv,	/* Argument strings. */
    int toplevel,		/* Non-zero means create a toplevel window,
				 * zero means create a frame. */
    const char *appName)	/* Should only be non-NULL if there is no main
				 * window associated with the interpreter.
				 * Gives the base name to use for the new
				 * application. */
{
    int result, i;
    Tcl_Obj **objv = ckalloc((argc+1) * sizeof(Tcl_Obj **));

    for (i=0; i<argc; i++) {
	objv[i] = Tcl_NewStringObj(argv[i], -1);
	Tcl_IncrRefCount(objv[i]);
    }
    objv[argc] = NULL;
    result = CreateFrame(clientData, interp, argc, objv,
	    toplevel ? TYPE_TOPLEVEL : TYPE_FRAME, appName);
    for (i=0; i<argc; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
    ckfree(objv);
    return result;
}

int
TkListCreateFrame(
    ClientData clientData,	/* Either NULL or pointer to option table. */
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *listObj,		/* List of arguments. */
    int toplevel,		/* Non-zero means create a toplevel window,
				 * zero means create a frame. */
    Tcl_Obj *nameObj)		/* Should only be non-NULL if there is no main
				 * window associated with the interpreter.
				 * Gives the base name to use for the new
				 * application. */
{
    int objc;
    Tcl_Obj **objv;

    if (TCL_OK != Tcl_ListObjGetElements(interp, listObj, &objc, &objv)) {
	return TCL_ERROR;
    }
    return CreateFrame(clientData, interp, objc, objv,
	    toplevel ? TYPE_TOPLEVEL : TYPE_FRAME,
	    nameObj ? Tcl_GetString(nameObj) : NULL);
}

static int
CreateFrame(
    ClientData clientData,	/* NULL. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects. */
    enum FrameType type,	/* What widget type to create. */
    const char *appName)	/* Should only be non-NULL if there are no
				 * Main window associated with the
				 * interpreter. Gives the base name to use for
				 * the new application. */
{
    Tk_Window tkwin;
    Frame *framePtr;
    Tk_OptionTable optionTable;
    Tk_Window newWin;
    const char *className, *screenName, *visualName, *colormapName;
    const char *arg, *useOption;
    int i, length, depth;
    unsigned int mask;
    Colormap colormap;
    Visual *visual;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?-option value ...?");
	return TCL_ERROR;
    }

    /*
     * Create the option table for this widget class. If it has already been
     * created, the cached pointer will be returned.
     */

    optionTable = Tk_CreateOptionTable(interp, optionSpecs[type]);

    /*
     * Pre-process the argument list. Scan through it to find any "-class",
     * "-screen", "-visual", and "-colormap" options. These arguments need to
     * be processed specially, before the window is configured using the usual
     * Tk mechanisms.
     */

    className = colormapName = screenName = visualName = useOption = NULL;
    colormap = None;
    for (i = 2; i < objc; i += 2) {
	arg = Tcl_GetStringFromObj(objv[i], &length);
	if (length < 2) {
	    continue;
	}
	if ((arg[1] == 'c') && (length >= 3)
		&& (strncmp(arg, "-class", (unsigned) length) == 0)) {
	    className = Tcl_GetString(objv[i+1]);
	} else if ((arg[1] == 'c') && (length >= 3)
		&& (strncmp(arg, "-colormap", (unsigned) length) == 0)) {
	    colormapName = Tcl_GetString(objv[i+1]);
	} else if ((arg[1] == 's') && (type == TYPE_TOPLEVEL)
		&& (strncmp(arg, "-screen", (unsigned) length) == 0)) {
	    screenName = Tcl_GetString(objv[i+1]);
	} else if ((arg[1] == 'u') && (type == TYPE_TOPLEVEL)
		&& (strncmp(arg, "-use", (unsigned) length) == 0)) {
	    useOption = Tcl_GetString(objv[i+1]);
	} else if ((arg[1] == 'v')
		&& (strncmp(arg, "-visual", (unsigned) length) == 0)) {
	    visualName = Tcl_GetString(objv[i+1]);
	}
    }

    /*
     * Create the window, and deal with the special options -use, -classname,
     * -colormap, -screenname, and -visual. These options must be handle
     * before calling ConfigureFrame below, and they must also be processed in
     * a particular order, for the following reasons:
     * 1. Must set the window's class before calling ConfigureFrame, so that
     *	  unspecified options are looked up in the option database using the
     *	  correct class.
     * 2. Must set visual information before calling ConfigureFrame so that
     *	  colors are allocated in a proper colormap.
     * 3. Must call TkpUseWindow before setting non-default visual
     *	  information, since TkpUseWindow changes the defaults.
     */

    if (screenName == NULL) {
	screenName = (type == TYPE_TOPLEVEL) ? "" : NULL;
    }

    /*
     * Main window associated with interpreter. If we're called by Tk_Init to
     * create a new application, then this is NULL.
     */

    tkwin = Tk_MainWindow(interp);
    if (tkwin != NULL) {
	newWin = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(objv[1]),
		screenName);
    } else if (appName == NULL) {
	/*
	 * This occurs when someone tried to create a frame/toplevel while we
	 * are being destroyed. Let an error be thrown.
	 */

	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unable to create widget \"%s\"", Tcl_GetString(objv[1])));
	Tcl_SetErrorCode(interp, "TK", "APPLICATION_GONE", NULL);
	return TCL_ERROR;
    } else {
	/*
	 * We were called from Tk_Init; create a new application.
	 */

	newWin = TkCreateMainWindow(interp, screenName, appName);
    }
    if (newWin == NULL) {
	goto error;
    }

    /*
     * Mark Tk frames as suitable candidates for [wm manage].
     */

    ((TkWindow *) newWin)->flags |= TK_WM_MANAGEABLE;

    if (className == NULL) {
	className = Tk_GetOption(newWin, "class", "Class");
	if (className == NULL) {
	    className = classNames[type];
	}
    }
    Tk_SetClass(newWin, className);
    if (useOption == NULL) {
	useOption = Tk_GetOption(newWin, "use", "Use");
    }
    if ((useOption != NULL) && (*useOption != 0)
	    && (TkpUseWindow(interp, newWin, useOption) != TCL_OK)) {
	goto error;
    }
    if (visualName == NULL) {
	visualName = Tk_GetOption(newWin, "visual", "Visual");
    }
    if (colormapName == NULL) {
	colormapName = Tk_GetOption(newWin, "colormap", "Colormap");
    }
    if ((colormapName != NULL) && (*colormapName == 0)) {
	colormapName = NULL;
    }
    if (visualName != NULL) {
	visual = Tk_GetVisual(interp, newWin, visualName, &depth,
		(colormapName == NULL) ? &colormap : NULL);
	if (visual == NULL) {
	    goto error;
	}
	Tk_SetWindowVisual(newWin, visual, depth, colormap);
    }
    if (colormapName != NULL) {
	colormap = Tk_GetColormap(interp, newWin, colormapName);
	if (colormap == None) {
	    goto error;
	}
	Tk_SetWindowColormap(newWin, colormap);
    }

    /*
     * For top-level windows, provide an initial geometry request of 200x200,
     * just so the window looks nicer on the screen if it doesn't request a
     * size for itself.
     */

    if (type == TYPE_TOPLEVEL) {
	Tk_GeometryRequest(newWin, 200, 200);
    }

    /*
     * Create the widget record, process configuration options, and create
     * event handlers. Then fill in a few additional fields in the widget
     * record from the special options.
     */

    if (type == TYPE_LABELFRAME) {
	framePtr = ckalloc(sizeof(Labelframe));
	memset(framePtr, 0, sizeof(Labelframe));
    } else {
	framePtr = ckalloc(sizeof(Frame));
	memset(framePtr, 0, sizeof(Frame));
    }
    framePtr->tkwin = newWin;
    framePtr->display = Tk_Display(newWin);
    framePtr->interp = interp;
    framePtr->widgetCmd	= Tcl_CreateObjCommand(interp, Tk_PathName(newWin),
	    FrameWidgetObjCmd, framePtr, FrameCmdDeletedProc);
    framePtr->optionTable = optionTable;
    framePtr->type = type;
    framePtr->colormap = colormap;
    framePtr->relief = TK_RELIEF_FLAT;
    framePtr->cursor = NULL;

    if (framePtr->type == TYPE_LABELFRAME) {
	Labelframe *labelframePtr = (Labelframe *) framePtr;

	labelframePtr->labelAnchor = LABELANCHOR_NW;
	labelframePtr->textGC = NULL;
    }

    /*
     * Store backreference to frame widget in window structure.
     */

    Tk_SetClassProcs(newWin, &frameClass, framePtr);

    mask = ExposureMask | StructureNotifyMask | FocusChangeMask;
    if (type == TYPE_TOPLEVEL) {
	mask |= ActivateMask;
    }
    Tk_CreateEventHandler(newWin, mask, FrameEventProc, framePtr);
    if ((Tk_InitOptions(interp, (char *) framePtr, optionTable, newWin)
	    != TCL_OK) ||
	    (ConfigureFrame(interp, framePtr, objc-2, objv+2) != TCL_OK)) {
	goto error;
    }
    if (framePtr->isContainer) {
	if (framePtr->useThis != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "windows cannot have both the -use and the -container"
		    " option set", -1));
	    Tcl_SetErrorCode(interp, "TK", "FRAME", "CONTAINMENT", NULL);
	    goto error;
	}
	TkpMakeContainer(framePtr->tkwin);
    }
    if (type == TYPE_TOPLEVEL) {
	Tcl_DoWhenIdle(MapFrame, framePtr);
    }
    Tcl_SetObjResult(interp, TkNewWindowObj(newWin));
    return TCL_OK;

  error:
    if (newWin != NULL) {
	Tk_DestroyWindow(newWin);
    }
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * FrameWidgetObjCmd --
 *
 *	This function is invoked to process the Tcl command that corresponds
 *	to a frame widget. See the user documentation for details on what it
 *	does.
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
FrameWidgetObjCmd(
    ClientData clientData,	/* Information about frame widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const frameOptions[] = {
	"cget", "configure", NULL
    };
    enum options {
	FRAME_CGET, FRAME_CONFIGURE
    };
    register Frame *framePtr = clientData;
    int result = TCL_OK, index;
    int c, i, length;
    Tcl_Obj *objPtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], frameOptions,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_Preserve(framePtr);
    switch ((enum options) index) {
    case FRAME_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	    goto done;
	}
	objPtr = Tk_GetOptionValue(interp, (char *) framePtr,
		framePtr->optionTable, objv[2], framePtr->tkwin);
	if (objPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	Tcl_SetObjResult(interp, objPtr);
	break;
    case FRAME_CONFIGURE:
	if (objc <= 3) {
	    objPtr = Tk_GetOptionInfo(interp, (char *) framePtr,
		    framePtr->optionTable, (objc == 3) ? objv[2] : NULL,
		    framePtr->tkwin);
	    if (objPtr == NULL) {
		result = TCL_ERROR;
		goto done;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	} else {
	    /*
	     * Don't allow the options -class, -colormap, -container, -screen,
	     * -use, or -visual to be changed.
	     */

	    for (i = 2; i < objc; i++) {
		const char *arg = Tcl_GetStringFromObj(objv[i], &length);

		if (length < 2) {
		    continue;
		}
		c = arg[1];
		if (((c == 'c') && (length >= 2)
			&& (strncmp(arg, "-class", (unsigned)length) == 0))
		    || ((c == 'c') && (length >= 3)
			&& (strncmp(arg, "-colormap", (unsigned)length) == 0))
		    || ((c == 'c') && (length >= 3)
			&& (strncmp(arg, "-container", (unsigned)length) == 0))
		    || ((c == 's') && (framePtr->type == TYPE_TOPLEVEL)
			&& (strncmp(arg, "-screen", (unsigned)length) == 0))
		    || ((c == 'u') && (framePtr->type == TYPE_TOPLEVEL)
			&& (strncmp(arg, "-use", (unsigned)length) == 0))
		    || ((c == 'v')
			&& (strncmp(arg, "-visual", (unsigned)length) == 0))) {

#ifdef SUPPORT_CONFIG_EMBEDDED
		    if (c == 'u') {
			const char *string = Tcl_GetString(objv[i+1]);

			if (TkpUseWindow(interp, framePtr->tkwin,
				string) != TCL_OK) {
			    result = TCL_ERROR;
			    goto done;
			}
			continue;
		    }
#endif
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "can't modify %s option after widget is created",
			    arg));
		    Tcl_SetErrorCode(interp, "TK", "FRAME", "CREATE_ONLY",
			    NULL);
		    result = TCL_ERROR;
		    goto done;
		}
	    }
	    result = ConfigureFrame(interp, framePtr, objc-2, objv+2);
	}
	break;
    }

  done:
    Tcl_Release(framePtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyFrame --
 *
 *	This function is invoked by Tcl_EventuallyFree or Tcl_Release to clean
 *	up the internal structure of a frame at a safe time (when no-one is
 *	using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the frame is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyFrame(
    void *memPtr)		/* Info about frame widget. */
{
    register Frame *framePtr = memPtr;
    register Labelframe *labelframePtr = memPtr;

    if (framePtr->type == TYPE_LABELFRAME) {
	Tk_FreeTextLayout(labelframePtr->textLayout);
	if (labelframePtr->textGC != NULL) {
	    Tk_FreeGC(framePtr->display, labelframePtr->textGC);
	}
    }
    if (framePtr->colormap != None) {
	Tk_FreeColormap(framePtr->display, framePtr->colormap);
    }
    ckfree(framePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyFramePartly --
 *
 *	This function is invoked to clean up everything that needs tkwin to be
 *	defined when deleted. During the destruction process tkwin is always
 *	set to NULL and this function must be called before that happens.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Some things associated with the frame are freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyFramePartly(
    Frame *framePtr)		/* Info about frame widget. */
{
    register Labelframe *labelframePtr = (Labelframe *) framePtr;

    if (framePtr->type == TYPE_LABELFRAME && labelframePtr->labelWin != NULL) {
	Tk_DeleteEventHandler(labelframePtr->labelWin, StructureNotifyMask,
		FrameStructureProc, framePtr);
	Tk_ManageGeometry(labelframePtr->labelWin, NULL, NULL);
	if (framePtr->tkwin != Tk_Parent(labelframePtr->labelWin)) {
	    Tk_UnmaintainGeometry(labelframePtr->labelWin, framePtr->tkwin);
	}
	Tk_UnmapWindow(labelframePtr->labelWin);
	labelframePtr->labelWin = NULL;
    }

    Tk_FreeConfigOptions((char *) framePtr, framePtr->optionTable,
	    framePtr->tkwin);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureFrame --
 *
 *	This function is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a frame
 *	widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for framePtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureFrame(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register Frame *framePtr,	/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc,			/* Number of valid entries in objv. */
    Tcl_Obj *const objv[])	/* Arguments. */
{
    Tk_SavedOptions savedOptions;
    char *oldMenuName;
    Tk_Window oldWindow = NULL;
    Labelframe *labelframePtr = (Labelframe *) framePtr;

    /*
     * Need the old menubar name for the menu code to delete it.
     */

    if (framePtr->menuName == NULL) {
    	oldMenuName = NULL;
    } else {
    	oldMenuName = ckalloc(strlen(framePtr->menuName) + 1);
    	strcpy(oldMenuName, framePtr->menuName);
    }

    if (framePtr->type == TYPE_LABELFRAME) {
	oldWindow = labelframePtr->labelWin;
    }
    if (Tk_SetOptions(interp, (char *) framePtr,
	    framePtr->optionTable, objc, objv,
	    framePtr->tkwin, &savedOptions, NULL) != TCL_OK) {
	if (oldMenuName != NULL) {
	    ckfree(oldMenuName);
	}
	return TCL_ERROR;
    }
    Tk_FreeSavedOptions(&savedOptions);

    /*
     * A few of the options require additional processing.
     */

    if ((((oldMenuName == NULL) && (framePtr->menuName != NULL))
	    || ((oldMenuName != NULL) && (framePtr->menuName == NULL))
	    || ((oldMenuName != NULL) && (framePtr->menuName != NULL)
	    && strcmp(oldMenuName, framePtr->menuName) != 0))
	    && framePtr->type == TYPE_TOPLEVEL) {
	TkSetWindowMenuBar(interp, framePtr->tkwin, oldMenuName,
		framePtr->menuName);
    }

    if (oldMenuName != NULL) {
    	ckfree(oldMenuName);
    }

    if (framePtr->border != NULL) {
	Tk_SetBackgroundFromBorder(framePtr->tkwin, framePtr->border);
    } else {
	Tk_SetWindowBackgroundPixmap(framePtr->tkwin, None);
    }

    if (framePtr->highlightWidth < 0) {
	framePtr->highlightWidth = 0;
    }
    if (framePtr->padX < 0) {
	framePtr->padX = 0;
    }
    if (framePtr->padY < 0) {
	framePtr->padY = 0;
    }

    /*
     * If a -labelwidget is specified, check that it is valid and set up
     * geometry management for it.
     */

    if (framePtr->type == TYPE_LABELFRAME) {
	if (oldWindow != labelframePtr->labelWin) {
	    if (oldWindow != NULL) {
		Tk_DeleteEventHandler(oldWindow, StructureNotifyMask,
			FrameStructureProc, framePtr);
		Tk_ManageGeometry(oldWindow, NULL, NULL);
		Tk_UnmaintainGeometry(oldWindow, framePtr->tkwin);
		Tk_UnmapWindow(oldWindow);
	    }
	    if (labelframePtr->labelWin != NULL) {
		Tk_Window ancestor, parent, sibling = NULL;

		/*
		 * Make sure that the frame is either the parent of the window
		 * used as label or a descendant of that parent. Also, don't
		 * allow a top-level window to be managed inside the frame.
		 */

		parent = Tk_Parent(labelframePtr->labelWin);
		for (ancestor = framePtr->tkwin; ;
		     ancestor = Tk_Parent(ancestor)) {
		    if (ancestor == parent) {
			break;
		    }
		    sibling = ancestor;
		    if (Tk_IsTopLevel(ancestor)) {
			goto badLabelWindow;
		    }
		}
		if (Tk_IsTopLevel(labelframePtr->labelWin)) {
		    goto badLabelWindow;
		}
		if (labelframePtr->labelWin == framePtr->tkwin) {
		    goto badLabelWindow;
		}
		Tk_CreateEventHandler(labelframePtr->labelWin,
			StructureNotifyMask, FrameStructureProc, framePtr);
		Tk_ManageGeometry(labelframePtr->labelWin, &frameGeomType,
			framePtr);

		/*
		 * If the frame is not parent to the label, make sure the
		 * label is above its sibling in the stacking order.
		 */

		if (sibling != NULL) {
		    Tk_RestackWindow(labelframePtr->labelWin, Above, sibling);
		}
	    }
	}
    }

    FrameWorldChanged(framePtr);
    return TCL_OK;

  badLabelWindow:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "can't use %s as label in this frame",
	    Tk_PathName(labelframePtr->labelWin)));
    Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "HIERARCHY", NULL);
    labelframePtr->labelWin = NULL;
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameWorldChanged --
 *
 *	This function is called when the world has changed in some way and the
 *	widget needs to recompute all its graphics contexts and determine its
 *	new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frame will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
FrameWorldChanged(
    ClientData instanceData)	/* Information about widget. */
{
    Frame *framePtr = instanceData;
    Labelframe *labelframePtr = instanceData;
    Tk_Window tkwin = framePtr->tkwin;
    XGCValues gcValues;
    GC gc;
    int anyTextLabel, anyWindowLabel;
    int bWidthLeft, bWidthRight, bWidthTop, bWidthBottom;
    const char *labelText;

    anyTextLabel = (framePtr->type == TYPE_LABELFRAME) &&
	    (labelframePtr->textPtr != NULL) &&
	    (labelframePtr->labelWin == NULL);
    anyWindowLabel = (framePtr->type == TYPE_LABELFRAME) &&
	    (labelframePtr->labelWin != NULL);

    if (framePtr->type == TYPE_LABELFRAME) {
	/*
	 * The textGC is needed even in the labelWin case, so it's always
	 * created for a labelframe.
	 */

	gcValues.font = Tk_FontId(labelframePtr->tkfont);
	gcValues.foreground = labelframePtr->textColorPtr->pixel;
	gcValues.graphics_exposures = False;
	gc = Tk_GetGC(tkwin, GCForeground | GCFont | GCGraphicsExposures,
		&gcValues);
	if (labelframePtr->textGC != NULL) {
	    Tk_FreeGC(framePtr->display, labelframePtr->textGC);
	}
	labelframePtr->textGC = gc;

	/*
	 * Calculate label size.
	 */

	labelframePtr->labelReqWidth = labelframePtr->labelReqHeight = 0;

	if (anyTextLabel) {
	    labelText = Tcl_GetString(labelframePtr->textPtr);
	    Tk_FreeTextLayout(labelframePtr->textLayout);
	    labelframePtr->textLayout =
		    Tk_ComputeTextLayout(labelframePtr->tkfont,
		    labelText, -1, 0, TK_JUSTIFY_CENTER, 0,
		    &labelframePtr->labelReqWidth,
		    &labelframePtr->labelReqHeight);
	    labelframePtr->labelReqWidth += 2 * LABELSPACING;
	    labelframePtr->labelReqHeight += 2 * LABELSPACING;
	} else if (anyWindowLabel) {
	    labelframePtr->labelReqWidth = Tk_ReqWidth(labelframePtr->labelWin);
	    labelframePtr->labelReqHeight =
		    Tk_ReqHeight(labelframePtr->labelWin);
	}

	/*
	 * Make sure label size is at least as big as the border. This
	 * simplifies later calculations and gives a better appearance with
	 * thick borders.
	 */

	if ((labelframePtr->labelAnchor >= LABELANCHOR_N) &&
		(labelframePtr->labelAnchor <= LABELANCHOR_SW)) {
	    if (labelframePtr->labelReqHeight < framePtr->borderWidth) {
		labelframePtr->labelReqHeight = framePtr->borderWidth;
	    }
	} else {
	    if (labelframePtr->labelReqWidth < framePtr->borderWidth) {
		labelframePtr->labelReqWidth = framePtr->borderWidth;
	    }
	}
    }

    /*
     * Calculate individual border widths.
     */

    bWidthBottom = bWidthTop = bWidthRight = bWidthLeft =
	    framePtr->borderWidth + framePtr->highlightWidth;

    bWidthLeft   += framePtr->padX;
    bWidthRight  += framePtr->padX;
    bWidthTop    += framePtr->padY;
    bWidthBottom += framePtr->padY;

    if (anyTextLabel || anyWindowLabel) {
	switch (labelframePtr->labelAnchor) {
	case LABELANCHOR_E:
	case LABELANCHOR_EN:
	case LABELANCHOR_ES:
	    bWidthRight += labelframePtr->labelReqWidth -
		    framePtr->borderWidth;
	    break;
	case LABELANCHOR_N:
	case LABELANCHOR_NE:
	case LABELANCHOR_NW:
	    bWidthTop += labelframePtr->labelReqHeight - framePtr->borderWidth;
	    break;
	case LABELANCHOR_S:
	case LABELANCHOR_SE:
	case LABELANCHOR_SW:
	    bWidthBottom += labelframePtr->labelReqHeight -
		    framePtr->borderWidth;
	    break;
	default:
	    bWidthLeft += labelframePtr->labelReqWidth - framePtr->borderWidth;
	    break;
	}
    }

    Tk_SetInternalBorderEx(tkwin, bWidthLeft, bWidthRight, bWidthTop,
	    bWidthBottom);

    ComputeFrameGeometry(framePtr);

    /*
     * A labelframe should request size for its label.
     */

    if (framePtr->type == TYPE_LABELFRAME) {
	int minwidth = labelframePtr->labelReqWidth;
	int minheight = labelframePtr->labelReqHeight;
	int padding = framePtr->highlightWidth;

	if (framePtr->borderWidth > 0) {
	    padding += framePtr->borderWidth + LABELMARGIN;
	}
	padding *= 2;
	if ((labelframePtr->labelAnchor >= LABELANCHOR_N) &&
		(labelframePtr->labelAnchor <= LABELANCHOR_SW)) {
	    minwidth += padding;
	    minheight += framePtr->borderWidth + framePtr->highlightWidth;
	} else {
	    minheight += padding;
	    minwidth += framePtr->borderWidth + framePtr->highlightWidth;
	}
	Tk_SetMinimumRequestSize(tkwin, minwidth, minheight);
    }

    if ((framePtr->width > 0) || (framePtr->height > 0)) {
	Tk_GeometryRequest(tkwin, framePtr->width, framePtr->height);
    }

    if (Tk_IsMapped(tkwin)) {
	if (!(framePtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayFrame, framePtr);
	}
	framePtr->flags |= REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeFrameGeometry --
 *
 *	This function is called to compute various geometrical information for
 *	a frame, such as where various things get displayed. It's called when
 *	the window is reconfigured.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Display-related numbers get changed in *framePtr.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeFrameGeometry(
    register Frame *framePtr)	/* Information about widget. */
{
    int otherWidth, otherHeight, otherWidthT, otherHeightT, padding;
    int maxWidth, maxHeight;
    Tk_Window tkwin;
    Labelframe *labelframePtr = (Labelframe *) framePtr;

    /*
     * We have nothing to do here unless there is a label.
     */

    if (framePtr->type != TYPE_LABELFRAME) {
	return;
    }
    if (labelframePtr->textPtr == NULL && labelframePtr->labelWin == NULL) {
	return;
    }

    tkwin = framePtr->tkwin;

    /*
     * Calculate the available size for the label
     */

    labelframePtr->labelBox.width = labelframePtr->labelReqWidth;
    labelframePtr->labelBox.height = labelframePtr->labelReqHeight;

    padding = framePtr->highlightWidth;
    if (framePtr->borderWidth > 0) {
	padding += framePtr->borderWidth + LABELMARGIN;
    }
    padding *= 2;

    maxHeight = Tk_Height(tkwin);
    maxWidth  = Tk_Width(tkwin);

    if ((labelframePtr->labelAnchor >= LABELANCHOR_N) &&
	    (labelframePtr->labelAnchor <= LABELANCHOR_SW)) {
	maxWidth -= padding;
	if (maxWidth < 1) {
	    maxWidth = 1;
	}
    } else {
	maxHeight -= padding;
	if (maxHeight < 1) {
	    maxHeight = 1;
	}
    }
    if (labelframePtr->labelBox.width > maxWidth) {
	labelframePtr->labelBox.width = maxWidth;
    }
    if (labelframePtr->labelBox.height > maxHeight) {
	labelframePtr->labelBox.height = maxHeight;
    }

    /*
     * Calculate label and text position. The text's position is based on the
     * requested size (= the text's real size) to get proper alignment if the
     * text does not fit.
     */

    otherWidth   = Tk_Width(tkwin)  - labelframePtr->labelBox.width;
    otherHeight  = Tk_Height(tkwin) - labelframePtr->labelBox.height;
    otherWidthT  = Tk_Width(tkwin)  - labelframePtr->labelReqWidth;
    otherHeightT = Tk_Height(tkwin) - labelframePtr->labelReqHeight;
    padding = framePtr->highlightWidth;

    switch (labelframePtr->labelAnchor) {
    case LABELANCHOR_E:
    case LABELANCHOR_EN:
    case LABELANCHOR_ES:
	labelframePtr->labelTextX = otherWidthT - padding;
	labelframePtr->labelBox.x = otherWidth - padding;
	break;
    case LABELANCHOR_N:
    case LABELANCHOR_NE:
    case LABELANCHOR_NW:
	labelframePtr->labelTextY = padding;
	labelframePtr->labelBox.y = padding;
	break;
    case LABELANCHOR_S:
    case LABELANCHOR_SE:
    case LABELANCHOR_SW:
	labelframePtr->labelTextY = otherHeightT - padding;
	labelframePtr->labelBox.y = otherHeight - padding;
	break;
    default:
	labelframePtr->labelTextX = padding;
	labelframePtr->labelBox.x = padding;
	break;
    }

    if (framePtr->borderWidth > 0) {
	padding += framePtr->borderWidth + LABELMARGIN;
    }

    switch (labelframePtr->labelAnchor) {
    case LABELANCHOR_NW:
    case LABELANCHOR_SW:
	labelframePtr->labelTextX = padding;
	labelframePtr->labelBox.x = padding;
	break;
    case LABELANCHOR_N:
    case LABELANCHOR_S:
	labelframePtr->labelTextX = otherWidthT / 2;
	labelframePtr->labelBox.x = otherWidth / 2;
	break;
    case LABELANCHOR_NE:
    case LABELANCHOR_SE:
	labelframePtr->labelTextX = otherWidthT - padding;
	labelframePtr->labelBox.x = otherWidth - padding;
	break;
    case LABELANCHOR_EN:
    case LABELANCHOR_WN:
	labelframePtr->labelTextY = padding;
	labelframePtr->labelBox.y = padding;
	break;
    case LABELANCHOR_E:
    case LABELANCHOR_W:
	labelframePtr->labelTextY = otherHeightT / 2;
	labelframePtr->labelBox.y = otherHeight / 2;
	break;
    default:
	labelframePtr->labelTextY = otherHeightT - padding;
	labelframePtr->labelBox.y = otherHeight - padding;
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayFrame --
 *
 *	This function is invoked to display a frame widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the frame in its current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayFrame(
    ClientData clientData)	/* Information about widget. */
{
    register Frame *framePtr = clientData;
    register Tk_Window tkwin = framePtr->tkwin;
    int bdX1, bdY1, bdX2, bdY2, hlWidth;
    Pixmap pixmap;
    TkRegion clipRegion = NULL;

    framePtr->flags &= ~REDRAW_PENDING;
    if ((framePtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    /*
     * Highlight shall always be drawn if it exists, so do that first.
     */

    hlWidth = framePtr->highlightWidth;

    if (hlWidth != 0) {
	GC fgGC, bgGC;

	bgGC = Tk_GCForColor(framePtr->highlightBgColorPtr,
		Tk_WindowId(tkwin));
	if (framePtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(framePtr->highlightColorPtr,
		    Tk_WindowId(tkwin));
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC, hlWidth,
		    Tk_WindowId(tkwin));
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC, hlWidth,
		    Tk_WindowId(tkwin));
	}
    }

    /*
     * If -background is set to "", no interior is drawn.
     */

    if (framePtr->border == NULL) {
	return;
    }

    if (framePtr->type != TYPE_LABELFRAME) {
	/*
	 * Pass to platform specific draw function. In general, it just draws
	 * a simple rectangle, but it may "theme" the background.
	 */

    noLabel:
	TkpDrawFrame(tkwin, framePtr->border, hlWidth,
		framePtr->borderWidth, framePtr->relief);
    } else {
	Labelframe *labelframePtr = (Labelframe *) framePtr;

	if ((labelframePtr->textPtr == NULL) &&
		(labelframePtr->labelWin == NULL)) {
	    goto noLabel;
	}

#ifndef TK_NO_DOUBLE_BUFFERING
	/*
	 * In order to avoid screen flashes, this function redraws the frame
	 * into off-screen memory, then copies it back on-screen in a single
	 * operation. This means there's no point in time where the on-screen
	 * image has been cleared.
	 */

	pixmap = Tk_GetPixmap(framePtr->display, Tk_WindowId(tkwin),
		Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
#else
	pixmap = Tk_WindowId(tkwin);
#endif /* TK_NO_DOUBLE_BUFFERING */

	/*
	 * Clear the pixmap.
	 */

	Tk_Fill3DRectangle(tkwin, pixmap, framePtr->border, 0, 0,
		Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

	/*
	 * Calculate how the label affects the border's position.
	 */

	bdX1 = bdY1 = hlWidth;
	bdX2 = Tk_Width(tkwin) - hlWidth;
	bdY2 = Tk_Height(tkwin) - hlWidth;

	switch (labelframePtr->labelAnchor) {
	case LABELANCHOR_E:
	case LABELANCHOR_EN:
	case LABELANCHOR_ES:
	    bdX2 -= (labelframePtr->labelBox.width-framePtr->borderWidth) / 2;
	    break;
	case LABELANCHOR_N:
	case LABELANCHOR_NE:
	case LABELANCHOR_NW:
	    /*
	     * Since the glyphs of the text tend to be in the lower part we
	     * favor a lower border position by rounding up.
	     */

	    bdY1 += (labelframePtr->labelBox.height-framePtr->borderWidth+1)/2;
	    break;
	case LABELANCHOR_S:
	case LABELANCHOR_SE:
	case LABELANCHOR_SW:
	    bdY2 -= (labelframePtr->labelBox.height-framePtr->borderWidth) / 2;
	    break;
	default:
	    bdX1 += (labelframePtr->labelBox.width-framePtr->borderWidth) / 2;
	    break;
	}

	/*
	 * Draw border
	 */

	Tk_Draw3DRectangle(tkwin, pixmap, framePtr->border, bdX1, bdY1,
		bdX2 - bdX1, bdY2 - bdY1, framePtr->borderWidth,
		framePtr->relief);

	if (labelframePtr->labelWin == NULL) {
	    /*
	     * Clear behind the label
	     */

	    Tk_Fill3DRectangle(tkwin, pixmap,
		    framePtr->border, labelframePtr->labelBox.x,
		    labelframePtr->labelBox.y, labelframePtr->labelBox.width,
		    labelframePtr->labelBox.height, 0, TK_RELIEF_FLAT);

	    /*
	     * Draw label. If there is not room for the entire label, use
	     * clipping to get a nice appearance.
	     */

	    if ((labelframePtr->labelBox.width < labelframePtr->labelReqWidth)
		    || (labelframePtr->labelBox.height <
			    labelframePtr->labelReqHeight)) {
		clipRegion = TkCreateRegion();
		TkUnionRectWithRegion(&labelframePtr->labelBox, clipRegion,
			clipRegion);
		TkSetRegion(framePtr->display, labelframePtr->textGC,
			clipRegion);
	    }

	    Tk_DrawTextLayout(framePtr->display, pixmap,
		    labelframePtr->textGC, labelframePtr->textLayout,
		    labelframePtr->labelTextX + LABELSPACING,
		    labelframePtr->labelTextY + LABELSPACING, 0, -1);

	    if (clipRegion != NULL) {
		XSetClipMask(framePtr->display, labelframePtr->textGC, None);
		TkDestroyRegion(clipRegion);
	    }
	} else {
	    /*
	     * Reposition and map the window (but in different ways depending
	     * on whether the frame is the window's parent).
	     */

	    if (framePtr->tkwin == Tk_Parent(labelframePtr->labelWin)) {
		if ((labelframePtr->labelBox.x != Tk_X(labelframePtr->labelWin))
			|| (labelframePtr->labelBox.y !=
				Tk_Y(labelframePtr->labelWin))
			|| (labelframePtr->labelBox.width !=
				Tk_Width(labelframePtr->labelWin))
			|| (labelframePtr->labelBox.height !=
				Tk_Height(labelframePtr->labelWin))) {
		    Tk_MoveResizeWindow(labelframePtr->labelWin,
			    labelframePtr->labelBox.x,
			    labelframePtr->labelBox.y,
			    labelframePtr->labelBox.width,
			    labelframePtr->labelBox.height);
		}
		Tk_MapWindow(labelframePtr->labelWin);
	    } else {
		Tk_MaintainGeometry(labelframePtr->labelWin, framePtr->tkwin,
			labelframePtr->labelBox.x, labelframePtr->labelBox.y,
			labelframePtr->labelBox.width,
			labelframePtr->labelBox.height);
	    }
	}

#ifndef TK_NO_DOUBLE_BUFFERING
	/*
	 * Everything's been redisplayed; now copy the pixmap onto the screen
	 * and free up the pixmap.
	 */

	XCopyArea(framePtr->display, pixmap, Tk_WindowId(tkwin),
		labelframePtr->textGC, hlWidth, hlWidth,
		(unsigned) (Tk_Width(tkwin) - 2 * hlWidth),
		(unsigned) (Tk_Height(tkwin) - 2 * hlWidth),
		hlWidth, hlWidth);
	Tk_FreePixmap(framePtr->display, pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */
    }

}

/*
 *--------------------------------------------------------------
 *
 * FrameEventProc --
 *
 *	This function is invoked by the Tk dispatcher on structure changes to
 *	a frame. For frames with 3D borders, this function is also invoked for
 *	exposures.
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
FrameEventProc(
    ClientData clientData,	/* Information about window. */
    register XEvent *eventPtr)	/* Information about event. */
{
    register Frame *framePtr = clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	goto redraw;
    } else if (eventPtr->type == ConfigureNotify) {
	ComputeFrameGeometry(framePtr);
	goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
	if (framePtr->menuName != NULL) {
	    TkSetWindowMenuBar(framePtr->interp, framePtr->tkwin,
		    framePtr->menuName, NULL);
	    ckfree(framePtr->menuName);
	    framePtr->menuName = NULL;
	}
	if (framePtr->tkwin != NULL) {
	    /*
	     * If this window is a container, then this event could be coming
	     * from the embedded application, in which case Tk_DestroyWindow
	     * hasn't been called yet. When Tk_DestroyWindow is called later,
	     * then another destroy event will be generated. We need to be
	     * sure we ignore the second event, since the frame could be gone
	     * by then. To do so, delete the event handler explicitly
	     * (normally it's done implicitly by Tk_DestroyWindow).
	     */

	    /*
	     * Since the tkwin pointer will be gone when we reach
	     * DestroyFrame, we must free all options now.
	     */

	    DestroyFramePartly(framePtr);

	    Tk_DeleteEventHandler(framePtr->tkwin,
		    ExposureMask|StructureNotifyMask|FocusChangeMask,
		    FrameEventProc, framePtr);
	    framePtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(framePtr->interp, framePtr->widgetCmd);
	}
	if (framePtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayFrame, framePtr);
	}
	Tcl_CancelIdleCall(MapFrame, framePtr);
	Tcl_EventuallyFree(framePtr, (Tcl_FreeProc *) DestroyFrame);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    framePtr->flags |= GOT_FOCUS;
	    if (framePtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    framePtr->flags &= ~GOT_FOCUS;
	    if (framePtr->highlightWidth > 0) {
		goto redraw;
	    }
	}
    } else if (eventPtr->type == ActivateNotify) {
    	TkpSetMainMenubar(framePtr->interp, framePtr->tkwin,
    		framePtr->menuName);
    }
    return;

  redraw:
    if ((framePtr->tkwin != NULL) && !(framePtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayFrame, framePtr);
	framePtr->flags |= REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FrameCmdDeletedProc --
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
FrameCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    Frame *framePtr = clientData;
    Tk_Window tkwin = framePtr->tkwin;

    if (framePtr->menuName != NULL) {
	TkSetWindowMenuBar(framePtr->interp, framePtr->tkwin,
		framePtr->menuName, NULL);
	ckfree(framePtr->menuName);
	framePtr->menuName = NULL;
    }

    /*
     * This function could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this function destroys the
     * widget.
     */

    if (tkwin != NULL) {
	/*
	 * Some options need tkwin to be freed, so we free them here, before
	 * setting tkwin to NULL.
	 */

	DestroyFramePartly(framePtr);

	framePtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MapFrame --
 *
 *	This function is invoked as a when-idle handler to map a newly-created
 *	top-level frame.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The frame given by the clientData argument is mapped.
 *
 *----------------------------------------------------------------------
 */

static void
MapFrame(
    ClientData clientData)		/* Pointer to frame structure. */
{
    Frame *framePtr = clientData;

    /*
     * Wait for all other background events to be processed before mapping
     * window. This ensures that the window's correct geometry will have been
     * determined before it is first mapped, so that the window manager
     * doesn't get a false idea of its desired geometry.
     */

    Tcl_Preserve(framePtr);
    while (1) {
	if (Tcl_DoOneEvent(TCL_IDLE_EVENTS) == 0) {
	    break;
	}

	/*
	 * After each event, make sure that the window still exists and quit
	 * if the window has been destroyed.
	 */

	if (framePtr->tkwin == NULL) {
	    Tcl_Release(framePtr);
	    return;
	}
    }
    Tk_MapWindow(framePtr->tkwin);
    Tcl_Release(framePtr);
}

/*
 *--------------------------------------------------------------
 *
 * TkInstallFrameMenu --
 *
 *	This function is needed when a Windows HWND is created and a menubar
 *	has been set to the window with a system menu. It notifies the menu
 *	package so that the system menu can be rebuilt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The system menu (if any) is created for the menubar associated with
 *	this frame.
 *
 *--------------------------------------------------------------
 */

void
TkInstallFrameMenu(
    Tk_Window tkwin)		/* The window that was just created. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;

    if (winPtr->mainPtr != NULL) {
	Frame *framePtr = winPtr->instanceData;

	if (framePtr == NULL) {
	    Tcl_Panic("TkInstallFrameMenu couldn't get frame pointer");
	}
	TkpMenuNotifyToplevelCreate(winPtr->mainPtr->interp,
		framePtr->menuName);
    }
}

/*
 *--------------------------------------------------------------
 *
 * FrameStructureProc --
 *
 *	This function is invoked whenever StructureNotify events occur for a
 *	window that's managed as label for the frame. This procudure's only
 *	purpose is to clean up when windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is disassociated from the frame when it is deleted.
 *
 *--------------------------------------------------------------
 */

static void
FrameStructureProc(
    ClientData clientData,	/* Pointer to record describing frame. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    Labelframe *labelframePtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	/*
	 * This should only happen in a labelframe but it doesn't hurt to be
	 * careful.
	 */

	if (labelframePtr->frame.type == TYPE_LABELFRAME) {
	    labelframePtr->labelWin = NULL;
	    FrameWorldChanged(labelframePtr);
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * FrameRequestProc --
 *
 *	This function is invoked whenever a window that's associated with a
 *	frame changes its requested dimensions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size and location on the screen of the window may change depending
 *	on the options specified for the frame.
 *
 *--------------------------------------------------------------
 */

static void
FrameRequestProc(
    ClientData clientData,	/* Pointer to record for frame. */
    Tk_Window tkwin)		/* Window that changed its desired size. */
{
    Frame *framePtr = clientData;

    FrameWorldChanged(framePtr);
}

/*
 *--------------------------------------------------------------
 *
 * FrameLostSlaveProc --
 *
 *	This function is invoked by Tk whenever some other geometry claims
 *	control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all frame-related information about the slave.
 *
 *--------------------------------------------------------------
 */

static void
FrameLostSlaveProc(
    ClientData clientData,	/* Frame structure for slave window that was
				 * stolen away. */
    Tk_Window tkwin)		/* Tk's handle for the slave window. */
{
    Frame *framePtr = clientData;
    Labelframe *labelframePtr = clientData;

    /*
     * This should only happen in a labelframe but it doesn't hurt to be
     * careful.
     */

    if (labelframePtr->frame.type == TYPE_LABELFRAME) {
	Tk_DeleteEventHandler(labelframePtr->labelWin, StructureNotifyMask,
		FrameStructureProc, labelframePtr);
	if (framePtr->tkwin != Tk_Parent(labelframePtr->labelWin)) {
	    Tk_UnmaintainGeometry(labelframePtr->labelWin, framePtr->tkwin);
	}
	Tk_UnmapWindow(labelframePtr->labelWin);
	labelframePtr->labelWin = NULL;
    }
    FrameWorldChanged(framePtr);
}

void
TkMapTopFrame(
     Tk_Window tkwin)
{
    Frame *framePtr = ((TkWindow *) tkwin)->instanceData;
    Tk_OptionTable optionTable;

    if (Tk_IsTopLevel(tkwin) && framePtr->type == TYPE_FRAME) {
	framePtr->type = TYPE_TOPLEVEL;
	Tcl_DoWhenIdle(MapFrame, framePtr);
	if (framePtr->menuName != NULL) {
	    TkSetWindowMenuBar(framePtr->interp, framePtr->tkwin, NULL,
		    framePtr->menuName);
	}
    } else if (!Tk_IsTopLevel(tkwin) && framePtr->type == TYPE_TOPLEVEL) {
	framePtr->type = TYPE_FRAME;
    } else {
	/*
	 * Not a frame or toplevel, skip it.
	 */

	return;
    }

    /*
     * The option table has already been created so the cached pointer will be
     * returned.
     */

    optionTable = Tk_CreateOptionTable(framePtr->interp,
	    optionSpecs[framePtr->type]);
    framePtr->optionTable = optionTable;
}

/*
 *--------------------------------------------------------------
 *
 * TkToplevelWindowFromCommandToken --
 *
 *	If the given command name to the command for a toplevel window in the
 *	given interpreter, return the tkwin for that toplevel window. Note
 *	that this lookup can't be done using the standard tkwin internal table
 *	because the command might have been renamed.
 *
 * Results:
 *	A Tk_Window token, or NULL if the name does not refer to a toplevel
 *	window.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

Tk_Window
TkToplevelWindowForCommand(
    Tcl_Interp *interp,
    const char *cmdName)
{
    Tcl_CmdInfo cmdInfo;
    Frame *framePtr;

    if (Tcl_GetCommandInfo(interp, cmdName, &cmdInfo) == 0) {
	return NULL;
    }
    if (cmdInfo.objProc != FrameWidgetObjCmd) {
	return NULL;
    }
    framePtr = cmdInfo.objClientData;
    if (framePtr->type != TYPE_TOPLEVEL) {
	return NULL;
    }
    return framePtr->tkwin;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
