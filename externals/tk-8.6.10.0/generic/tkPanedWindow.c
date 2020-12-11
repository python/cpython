/*
 * tkPanedWindow.c --
 *
 *	This module implements "paned window" widgets that are object based. A
 *	"paned window" is a widget that manages the geometry for some number
 *	of other widgets, placing a movable "sash" between them, which can be
 *	used to alter the relative sizes of adjacent widgets.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 * Copyright (c) 2000 Ajuba Solutions.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"

/*
 * Flag values for "sticky"ness. The 16 combinations subsume the packer's
 * notion of anchor and fill.
 *
 * STICK_NORTH  	This window sticks to the top of its cavity.
 * STICK_EAST		This window sticks to the right edge of its cavity.
 * STICK_SOUTH		This window sticks to the bottom of its cavity.
 * STICK_WEST		This window sticks to the left edge of its cavity.
 */

#define STICK_NORTH		1
#define STICK_EAST		2
#define STICK_SOUTH		4
#define STICK_WEST		8

/*
 * The following table defines the legal values for the -orient option.
 */

static const char *const orientStrings[] = {
    "horizontal", "vertical", NULL
};

enum orient { ORIENT_HORIZONTAL, ORIENT_VERTICAL };

/*
 * The following table defines the legal values for the -stretch option.
 */

static const char *const stretchStrings[] = {
    "always", "first", "last", "middle", "never", NULL
};

enum stretch {
    STRETCH_ALWAYS,		/* Always give extra space to this pane. */
    STRETCH_FIRST,		/* Give extra space to pane if it is first. */
    STRETCH_LAST,		/* Give extra space to pane if it is last. */
    STRETCH_MIDDLE,		/* Give extra space to pane only if it is
				 * neither first nor last. */
    STRETCH_NEVER		/* Never give extra space to this pane. */
};

/*
 * Codify the stretchiness rule in one place.
 */

#define IsStretchable(stretch,index,first,last)			\
    (((stretch) == STRETCH_ALWAYS) ||				\
     ((stretch) == STRETCH_FIRST && (index) == (first)) ||	\
     ((stretch) == STRETCH_LAST && (index) == (last)) ||	\
     ((stretch) == STRETCH_MIDDLE && (index) != (first) && (index) != (last)))

typedef struct {
    Tk_OptionTable pwOptions;	/* Token for paned window option table. */
    Tk_OptionTable slaveOpts;	/* Token for slave cget option table. */
} OptionTables;

/*
 * One structure of the following type is kept for each window
 * managed by a paned window widget.
 */

typedef struct Slave {
    Tk_Window tkwin;		/* Window being managed. */
    int minSize;		/* Minimum size of this pane, on the relevant
				 * axis, in pixels. */
    int padx;			/* Additional padding requested for slave, in
				 * the x dimension. */
    int pady;			/* Additional padding requested for slave, in
				 * the y dimension. */
    Tcl_Obj *widthPtr, *heightPtr;
				/* Tcl_Obj rep's of slave width/height, to
				 * allow for null values. */
    int width;			/* Slave width. */
    int height;			/* Slave height. */
    int sticky;			/* Sticky string. */
    int x, y;			/* Coordinates of the widget. */
    int paneWidth, paneHeight;	/* Pane dimensions (may be different from
				 * slave width/height). */
    int sashx, sashy;		/* Coordinates of the sash of the right or
				 * bottom of this pane. */
    int markx, marky;		/* Coordinates of the last mark set for the
				 * sash. */
    int handlex, handley;	/* Coordinates of the sash handle. */
    enum stretch stretch;	/* Controls how slave grows/shrinks */
    int hide;			/* Controls visibility of pane */
    struct PanedWindow *masterPtr;
				/* Paned window managing the window. */
    Tk_Window after;		/* Placeholder for parsing options. */
    Tk_Window before;		/* Placeholder for parsing options. */
} Slave;

/*
 * A data structure of the following type is kept for each paned window widget
 * managed by this file:
 */

typedef struct PanedWindow {
    Tk_Window tkwin;		/* Window that embodies the paned window. */
    Tk_Window proxywin;		/* Window for the resizing proxy. */
    Display *display;		/* X's token for the window's display. */
    Tcl_Interp *interp;		/* Interpreter associated with widget. */
    Tcl_Command widgetCmd;	/* Token for square's widget command. */
    Tk_OptionTable optionTable;	/* Token representing the configuration
				 * specifications. */
    Tk_OptionTable slaveOpts;	/* Token for slave cget table. */
    Tk_3DBorder background;	/* Background color. */
    int borderWidth;		/* Value of -borderwidth option. */
    int relief;			/* 3D border effect (TK_RELIEF_RAISED, etc) */
    Tcl_Obj *widthPtr;		/* Tcl_Obj rep for width. */
    Tcl_Obj *heightPtr;		/* Tcl_Obj rep for height. */
    int width, height;		/* Width and height of the widget. */
    enum orient orient;		/* Orientation of the widget. */
    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    int resizeOpaque;		/* Boolean indicating whether resize should be
				 * opaque or rubberband style. */
    int sashRelief;		/* Relief used to draw sash. */
    int sashWidth;		/* Width of each sash, in pixels. */
    Tcl_Obj *sashWidthPtr;	/* Tcl_Obj rep for sash width. */
    int sashPad;		/* Additional padding around each sash. */
    Tcl_Obj *sashPadPtr;	/* Tcl_Obj rep for sash padding. */
    int showHandle;		/* Boolean indicating whether sash handles
				 * should be drawn. */
    int handleSize;		/* Size of one side of a sash handle (handles
				 * are square), in pixels. */
    int handlePad;		/* Distance from border to draw handle. */
    Tcl_Obj *handleSizePtr;	/* Tcl_Obj rep for handle size. */
    Tk_Cursor sashCursor;	/* Cursor used when mouse is above a sash. */
    GC gc;			/* Graphics context for copying from
				 * off-screen pixmap onto screen. */
    int proxyx, proxyy;		/* Proxy x,y coordinates. */
    Tk_3DBorder proxyBackground;/* Background color used to draw proxy. If NULL, use background. */
    Tcl_Obj *proxyBorderWidthPtr; /* Tcl_Obj rep for proxyBorderWidth */
    int proxyBorderWidth;	/* Borderwidth used to draw proxy. */
    int proxyRelief;		/* Relief used to draw proxy, if TK_RELIEF_NULL then use relief. */
    Slave **slaves;		/* Pointer to array of Slaves. */
    int numSlaves;		/* Number of slaves. */
    int sizeofSlaves;		/* Number of elements in the slaves array. */
    int flags;			/* Flags for widget; see below. */
} PanedWindow;

/*
 * Flags used for paned windows:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler has been
 *				queued to redraw this window.
 *
 * WIDGET_DELETED:		Non-zero means that the paned window has been,
 *				or is in the process of being, deleted.
 *
 * RESIZE_PENDING:		Non-zero means that the window might need to
 *				change its size (or the size of its panes)
 *				because of a change in the size of one of its
 *				children.
 */

#define REDRAW_PENDING		0x0001
#define WIDGET_DELETED		0x0002
#define REQUESTED_RELAYOUT	0x0004
#define RECOMPUTE_GEOMETRY	0x0008
#define PROXY_REDRAW_PENDING	0x0010
#define RESIZE_PENDING		0x0020

/*
 * Forward declarations for functions defined later in this file:
 */

int			Tk_PanedWindowObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		PanedWindowCmdDeletedProc(ClientData clientData);
static int		ConfigurePanedWindow(Tcl_Interp *interp,
			    PanedWindow *pwPtr, int objc,
			    Tcl_Obj *const objv[]);
static void		DestroyPanedWindow(PanedWindow *pwPtr);
static void		DisplayPanedWindow(ClientData clientData);
static void		PanedWindowEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		ProxyWindowEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		DisplayProxyWindow(ClientData clientData);
static void		PanedWindowWorldChanged(ClientData instanceData);
static int		PanedWindowWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *, int objc, Tcl_Obj * const objv[]);
static void		PanedWindowLostSlaveProc(ClientData clientData,
			    Tk_Window tkwin);
static void		PanedWindowReqProc(ClientData clientData,
			    Tk_Window tkwin);
static void		ArrangePanes(ClientData clientData);
static void		Unlink(Slave *slavePtr);
static Slave *		GetPane(PanedWindow *pwPtr, Tk_Window tkwin);
static void		GetFirstLastVisiblePane(PanedWindow *pwPtr,
			    int *firstPtr, int *lastPtr);
static void		SlaveStructureProc(ClientData clientData,
			    XEvent *eventPtr);
static int		PanedWindowSashCommand(PanedWindow *pwPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * const objv[]);
static int		PanedWindowProxyCommand(PanedWindow *pwPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * const objv[]);
static void		ComputeGeometry(PanedWindow *pwPtr);
static int		ConfigureSlaves(PanedWindow *pwPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * const objv[]);
static void		DestroyOptionTables(ClientData clientData,
			    Tcl_Interp *interp);
static int		SetSticky(ClientData clientData, Tcl_Interp *interp,
			    Tk_Window tkwin, Tcl_Obj **value, char *recordPtr,
			    int internalOffset, char *oldInternalPtr,
			    int flags);
static Tcl_Obj *	GetSticky(ClientData clientData, Tk_Window tkwin,
			    char *recordPtr, int internalOffset);
static void		RestoreSticky(ClientData clientData, Tk_Window tkwin,
			    char *internalPtr, char *oldInternalPtr);
static void		AdjustForSticky(int sticky, int cavityWidth,
			    int cavityHeight, int *xPtr, int *yPtr,
			    int *slaveWidthPtr, int *slaveHeightPtr);
static void		MoveSash(PanedWindow *pwPtr, int sash, int diff);
static int		ObjectIsEmpty(Tcl_Obj *objPtr);
static char *		ComputeSlotAddress(char *recordPtr, int offset);
static int		PanedWindowIdentifyCoords(PanedWindow *pwPtr,
			    Tcl_Interp *interp, int x, int y);

/*
 * Sashes are between panes only, so there is one less sash than slaves
 */

#define ValidSashIndex(pwPtr, sash) \
	(((sash) >= 0) && ((sash) < ((pwPtr)->numSlaves-1)))

static const Tk_GeomMgr panedWindowMgrType = {
    "panedwindow",		/* name */
    PanedWindowReqProc,		/* requestProc */
    PanedWindowLostSlaveProc,	/* lostSlaveProc */
};

/*
 * Information used for objv parsing.
 */

#define GEOMETRY		0x0001

/*
 * The following structure contains pointers to functions used for processing
 * the custom "-sticky" option for slave windows.
 */

static const Tk_ObjCustomOption stickyOption = {
    "sticky",			/* name */
    SetSticky,			/* setProc */
    GetSticky,			/* getProc */
    RestoreSticky,		/* restoreProc */
    NULL,			/* freeProc */
    0
};

static const Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BORDER, "-background", "background", "Background",
	 DEF_PANEDWINDOW_BG_COLOR, -1, Tk_Offset(PanedWindow, background), 0,
	 DEF_PANEDWINDOW_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	 NULL, 0, -1, 0, "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
	 NULL, 0, -1, 0, "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	 DEF_PANEDWINDOW_BORDERWIDTH, -1, Tk_Offset(PanedWindow, borderWidth),
	 0, 0, GEOMETRY},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	 DEF_PANEDWINDOW_CURSOR, -1, Tk_Offset(PanedWindow, cursor),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-handlepad", "handlePad", "HandlePad",
	 DEF_PANEDWINDOW_HANDLEPAD, -1, Tk_Offset(PanedWindow, handlePad),
	 0, 0, GEOMETRY},
    {TK_OPTION_PIXELS, "-handlesize", "handleSize", "HandleSize",
	 DEF_PANEDWINDOW_HANDLESIZE, Tk_Offset(PanedWindow, handleSizePtr),
	 Tk_Offset(PanedWindow, handleSize), 0, 0, GEOMETRY},
    {TK_OPTION_PIXELS, "-height", "height", "Height",
	 DEF_PANEDWINDOW_HEIGHT, Tk_Offset(PanedWindow, heightPtr),
	 Tk_Offset(PanedWindow, height), TK_OPTION_NULL_OK, 0, GEOMETRY},
    {TK_OPTION_BOOLEAN, "-opaqueresize", "opaqueResize", "OpaqueResize",
	 DEF_PANEDWINDOW_OPAQUERESIZE, -1,
	 Tk_Offset(PanedWindow, resizeOpaque), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient",
	 DEF_PANEDWINDOW_ORIENT, -1, Tk_Offset(PanedWindow, orient),
	 0, orientStrings, GEOMETRY},
    {TK_OPTION_BORDER, "-proxybackground", "proxyBackground", "ProxyBackground",
	 0, -1, Tk_Offset(PanedWindow, proxyBackground), TK_OPTION_NULL_OK,
	 (ClientData) DEF_PANEDWINDOW_BG_MONO, 0},
    {TK_OPTION_PIXELS, "-proxyborderwidth", "proxyBorderWidth", "ProxyBorderWidth",
	 DEF_PANEDWINDOW_PROXYBORDER, Tk_Offset(PanedWindow, proxyBorderWidthPtr),
	 Tk_Offset(PanedWindow, proxyBorderWidth), 0, 0, GEOMETRY},
    {TK_OPTION_RELIEF, "-proxyrelief", "proxyRelief", "Relief",
	 0, -1, Tk_Offset(PanedWindow, proxyRelief),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	 DEF_PANEDWINDOW_RELIEF, -1, Tk_Offset(PanedWindow, relief), 0, 0, 0},
    {TK_OPTION_CURSOR, "-sashcursor", "sashCursor", "Cursor",
	 DEF_PANEDWINDOW_SASHCURSOR, -1, Tk_Offset(PanedWindow, sashCursor),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-sashpad", "sashPad", "SashPad",
	 DEF_PANEDWINDOW_SASHPAD, -1, Tk_Offset(PanedWindow, sashPad),
	 0, 0, GEOMETRY},
    {TK_OPTION_RELIEF, "-sashrelief", "sashRelief", "Relief",
	 DEF_PANEDWINDOW_SASHRELIEF, -1, Tk_Offset(PanedWindow, sashRelief),
	 0, 0, 0},
    {TK_OPTION_PIXELS, "-sashwidth", "sashWidth", "Width",
	 DEF_PANEDWINDOW_SASHWIDTH, Tk_Offset(PanedWindow, sashWidthPtr),
	 Tk_Offset(PanedWindow, sashWidth), 0, 0, GEOMETRY},
    {TK_OPTION_BOOLEAN, "-showhandle", "showHandle", "ShowHandle",
	 DEF_PANEDWINDOW_SHOWHANDLE, -1, Tk_Offset(PanedWindow, showHandle),
	 0, 0, GEOMETRY},
    {TK_OPTION_PIXELS, "-width", "width", "Width",
	 DEF_PANEDWINDOW_WIDTH, Tk_Offset(PanedWindow, widthPtr),
	 Tk_Offset(PanedWindow, width), TK_OPTION_NULL_OK, 0, GEOMETRY},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

static const Tk_OptionSpec slaveOptionSpecs[] = {
    {TK_OPTION_WINDOW, "-after", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_AFTER, -1, Tk_Offset(Slave, after),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_WINDOW, "-before", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_BEFORE, -1, Tk_Offset(Slave, before),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-height", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_HEIGHT, Tk_Offset(Slave, heightPtr),
	 Tk_Offset(Slave, height), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide",
	 DEF_PANEDWINDOW_PANE_HIDE, -1, Tk_Offset(Slave, hide), 0,0,GEOMETRY},
    {TK_OPTION_PIXELS, "-minsize", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_MINSIZE, -1, Tk_Offset(Slave, minSize), 0, 0, 0},
    {TK_OPTION_PIXELS, "-padx", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_PADX, -1, Tk_Offset(Slave, padx), 0, 0, 0},
    {TK_OPTION_PIXELS, "-pady", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_PADY, -1, Tk_Offset(Slave, pady), 0, 0, 0},
    {TK_OPTION_CUSTOM, "-sticky", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_STICKY, -1, Tk_Offset(Slave, sticky), 0,
	 &stickyOption, 0},
    {TK_OPTION_STRING_TABLE, "-stretch", "stretch", "Stretch",
	DEF_PANEDWINDOW_PANE_STRETCH, -1, Tk_Offset(Slave, stretch), 0,
	(ClientData) stretchStrings, 0},
    {TK_OPTION_PIXELS, "-width", NULL, NULL,
	 DEF_PANEDWINDOW_PANE_WIDTH, Tk_Offset(Slave, widthPtr),
	 Tk_Offset(Slave, width), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

/*
 *--------------------------------------------------------------
 *
 * Tk_PanedWindowObjCmd --
 *
 *	This function is invoked to process the "panedwindow" Tcl command. It
 *	creates a new "panedwindow" widget.
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
Tk_PanedWindowObjCmd(
    ClientData clientData,	/* NULL. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj * const objv[])	/* Argument objects. */
{
    PanedWindow *pwPtr;
    Tk_Window tkwin, parent;
    OptionTables *pwOpts;
    XSetWindowAttributes atts;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?-option value ...?");
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp),
	    Tcl_GetString(objv[1]), NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    pwOpts = (OptionTables *)
	    Tcl_GetAssocData(interp, "PanedWindowOptionTables", NULL);
    if (pwOpts == NULL) {
	/*
	 * The first time this function is invoked, the option tables will be
	 * NULL. We then create the option tables from the templates and store
	 * a pointer to the tables as the command's clinical so we'll have
	 * easy access to it in the future.
	 */

	pwOpts = ckalloc(sizeof(OptionTables));

	/*
	 * Set up an exit handler to free the optionTables struct.
	 */

	Tcl_SetAssocData(interp, "PanedWindowOptionTables",
		DestroyOptionTables, pwOpts);

	/*
	 * Create the paned window option tables.
	 */

	pwOpts->pwOptions = Tk_CreateOptionTable(interp, optionSpecs);
	pwOpts->slaveOpts = Tk_CreateOptionTable(interp, slaveOptionSpecs);
    }

    Tk_SetClass(tkwin, "Panedwindow");

    /*
     * Allocate and initialize the widget record.
     */

    pwPtr = ckalloc(sizeof(PanedWindow));
    memset((void *)pwPtr, 0, (sizeof(PanedWindow)));
    pwPtr->tkwin = tkwin;
    pwPtr->display = Tk_Display(tkwin);
    pwPtr->interp = interp;
    pwPtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(pwPtr->tkwin), PanedWindowWidgetObjCmd, pwPtr,
	    PanedWindowCmdDeletedProc);
    pwPtr->optionTable = pwOpts->pwOptions;
    pwPtr->slaveOpts = pwOpts->slaveOpts;
    pwPtr->relief = TK_RELIEF_RAISED;
    pwPtr->gc = NULL;
    pwPtr->cursor = NULL;
    pwPtr->sashCursor = NULL;

    /*
     * Keep a hold of the associated tkwin until we destroy the widget,
     * otherwise Tk might free it while we still need it.
     */

    Tcl_Preserve(pwPtr->tkwin);

    if (Tk_InitOptions(interp, (char *) pwPtr, pwOpts->pwOptions,
	    tkwin) != TCL_OK) {
	Tk_DestroyWindow(pwPtr->tkwin);
	return TCL_ERROR;
    }

    Tk_CreateEventHandler(pwPtr->tkwin, ExposureMask|StructureNotifyMask,
	    PanedWindowEventProc, pwPtr);

    /*
     * Find the toplevel ancestor of the panedwindow, and make a proxy win as
     * a child of that window; this way the proxy can always float above
     * slaves in the panedwindow.
     */

    parent = Tk_Parent(pwPtr->tkwin);
    while (!(Tk_IsTopLevel(parent))) {
	parent = Tk_Parent(parent);
	if (parent == NULL) {
	    parent = pwPtr->tkwin;
	    break;
	}
    }

    pwPtr->proxywin = Tk_CreateAnonymousWindow(interp, parent, NULL);

    /*
     * The proxy window has to be able to share GCs with the main panedwindow
     * despite being children of windows with potentially different
     * characteristics, and it looks better that way too. [Bug 702230] Also
     * set the X window save under attribute to avoid expose events as the
     * proxy sash is dragged across the panes. [Bug 1036963]
     */

    Tk_SetWindowVisual(pwPtr->proxywin,
	    Tk_Visual(tkwin), Tk_Depth(tkwin), Tk_Colormap(tkwin));
    Tk_CreateEventHandler(pwPtr->proxywin, ExposureMask, ProxyWindowEventProc,
	    pwPtr);
    atts.save_under = True;
    Tk_ChangeWindowAttributes(pwPtr->proxywin, CWSaveUnder, &atts);

    if (ConfigurePanedWindow(interp, pwPtr, objc - 2, objv + 2) != TCL_OK) {
	Tk_DestroyWindow(pwPtr->proxywin);
	Tk_DestroyWindow(pwPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, TkNewWindowObj(pwPtr->tkwin));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * PanedWindowWidgetObjCmd --
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
PanedWindowWidgetObjCmd(
    ClientData clientData,	/* Information about square widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj * const objv[])	/* Argument objects. */
{
    PanedWindow *pwPtr = clientData;
    int result = TCL_OK;
    static const char *const optionStrings[] = {
	"add", "cget", "configure", "forget", "identify", "panecget",
	"paneconfigure", "panes", "proxy", "sash", NULL
    };
    enum options {
	PW_ADD, PW_CGET, PW_CONFIGURE, PW_FORGET, PW_IDENTIFY, PW_PANECGET,
	PW_PANECONFIGURE, PW_PANES, PW_PROXY, PW_SASH
    };
    Tcl_Obj *resultObj;
    int index, count, i, x, y;
    Tk_Window tkwin;
    Slave *slavePtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "command",
	    0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_Preserve(pwPtr);

    switch ((enum options) index) {
    case PW_ADD:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "widget ?widget ...?");
	    result = TCL_ERROR;
	    break;
	}
	result = ConfigureSlaves(pwPtr, interp, objc, objv);
	break;

    case PW_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	    break;
	}
	resultObj = Tk_GetOptionValue(interp, (char *) pwPtr,
		pwPtr->optionTable, objv[2], pwPtr->tkwin);
	if (resultObj == NULL) {
	    result = TCL_ERROR;
	} else {
	    Tcl_SetObjResult(interp, resultObj);
	}
	break;

    case PW_CONFIGURE:
	resultObj = NULL;
	if (objc <= 3) {
	    resultObj = Tk_GetOptionInfo(interp, (char *) pwPtr,
		    pwPtr->optionTable,
		    (objc == 3) ? objv[2] : NULL, pwPtr->tkwin);
	    if (resultObj == NULL) {
		result = TCL_ERROR;
	    } else {
		Tcl_SetObjResult(interp, resultObj);
	    }
	} else {
	    result = ConfigurePanedWindow(interp, pwPtr, objc - 2, objv + 2);
	}
	break;

    case PW_FORGET: {
	int i;

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "widget ?widget ...?");
	    result = TCL_ERROR;
	    break;
	}

	/*
	 * Clean up each window named in the arg list.
	 */
	for (count = 0, i = 2; i < objc; i++) {
	    Tk_Window slave = Tk_NameToWindow(interp, Tcl_GetString(objv[i]),
		    pwPtr->tkwin);

	    if (slave == NULL) {
		continue;
	    }
	    slavePtr = GetPane(pwPtr, slave);
	    if ((slavePtr != NULL) && (slavePtr->masterPtr != NULL)) {
		count++;
		Tk_ManageGeometry(slave, NULL, NULL);
		Tk_UnmaintainGeometry(slavePtr->tkwin, pwPtr->tkwin);
		Tk_DeleteEventHandler(slavePtr->tkwin, StructureNotifyMask,
			SlaveStructureProc, slavePtr);
		Tk_UnmapWindow(slavePtr->tkwin);
		Unlink(slavePtr);
	    }
	    if (count != 0) {
		ComputeGeometry(pwPtr);
	    }
	}
	break;
    }

    case PW_IDENTIFY:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "x y");
	    result = TCL_ERROR;
	    break;
	}

	if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	    result = TCL_ERROR;
	    break;
	}
	result = PanedWindowIdentifyCoords(pwPtr, interp, x, y);
	break;

    case PW_PANECGET:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "pane option");
	    result = TCL_ERROR;
	    break;
	}
	tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), pwPtr->tkwin);
	if (tkwin == NULL) {
	    result = TCL_ERROR;
	    break;
	}
	resultObj = NULL;
	for (i = 0; i < pwPtr->numSlaves; i++) {
	    if (pwPtr->slaves[i]->tkwin == tkwin) {
		resultObj = Tk_GetOptionValue(interp,
			(char *) pwPtr->slaves[i], pwPtr->slaveOpts,
			objv[3], tkwin);
	    }
	}
	if (resultObj == NULL) {
	    if (i == pwPtr->numSlaves) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"not managed by this window", -1));
		Tcl_SetErrorCode(interp, "TK", "PANEDWINDOW", "UNMANAGED",
			NULL);
	    }
	    result = TCL_ERROR;
	} else {
	    Tcl_SetObjResult(interp, resultObj);
	}
	break;

    case PW_PANECONFIGURE:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "pane ?option? ?value option value ...?");
	    result = TCL_ERROR;
	    break;
	}
	resultObj = NULL;
	if (objc <= 4) {
	    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]),
		    pwPtr->tkwin);
            if (tkwin == NULL) {
                /*
                 * Just a plain old bad window; Tk_NameToWindow filled in an
                 * error message for us.
                 */

                result = TCL_ERROR;
                break;
            }
	    for (i = 0; i < pwPtr->numSlaves; i++) {
		if (pwPtr->slaves[i]->tkwin == tkwin) {
		    resultObj = Tk_GetOptionInfo(interp,
			    (char *) pwPtr->slaves[i], pwPtr->slaveOpts,
			    (objc == 4) ? objv[3] : NULL,
			    pwPtr->tkwin);
		    if (resultObj == NULL) {
			result = TCL_ERROR;
		    } else {
			Tcl_SetObjResult(interp, resultObj);
		    }
		    break;
		}
	    }
	} else {
	    result = ConfigureSlaves(pwPtr, interp, objc, objv);
	}
	break;

    case PW_PANES:
	resultObj = Tcl_NewObj();
	for (i = 0; i < pwPtr->numSlaves; i++) {
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    TkNewWindowObj(pwPtr->slaves[i]->tkwin));
	}
	Tcl_SetObjResult(interp, resultObj);
	break;

    case PW_PROXY:
	result = PanedWindowProxyCommand(pwPtr, interp, objc, objv);
	break;

    case PW_SASH:
	result = PanedWindowSashCommand(pwPtr, interp, objc, objv);
	break;
    }
    Tcl_Release(pwPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureSlaves --
 *
 *	Add or alter the configuration options of a slave in a paned window.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Depends on options; may add a slave to the paned window, may alter the
 *	geometry management options of a slave.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureSlaves(
    PanedWindow *pwPtr,		/* Information about paned window. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int i, firstOptionArg, j, found, doubleBw, index, numNewSlaves, haveLoc;
    int insertIndex;
    Tk_Window tkwin = NULL, ancestor, parent;
    Slave *slavePtr, **inserts, **newSlaves;
    Slave options;
    const char *arg;

    /*
     * Find the non-window name arguments; these are the configure options for
     * the slaves. Also validate that the window names given are legitimate
     * (ie, they are real windows, they are not the panedwindow itself, etc.).
     */

    for (i = 2; i < objc; i++) {
	arg = Tcl_GetString(objv[i]);
	if (arg[0] == '-') {
	    break;
	} else {
	    tkwin = Tk_NameToWindow(interp, arg, pwPtr->tkwin);
	    if (tkwin == NULL) {
		/*
		 * Just a plain old bad window; Tk_NameToWindow filled in an
		 * error message for us.
		 */

		return TCL_ERROR;
	    } else if (tkwin == pwPtr->tkwin) {
		/*
		 * A panedwindow cannot manage itself.
		 */

		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"can't add %s to itself", arg));
		Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "SELF", NULL);
		return TCL_ERROR;
	    } else if (Tk_IsTopLevel(tkwin)) {
		/*
		 * A panedwindow cannot manage a toplevel.
		 */

		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"can't add toplevel %s to %s", arg,
			Tk_PathName(pwPtr->tkwin)));
		Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "TOPLEVEL", NULL);
		return TCL_ERROR;
	    } else {
		/*
		 * Make sure the panedwindow is the parent of the slave,
		 * or a descendant of the slave's parent.
		 */

		parent = Tk_Parent(tkwin);
		for (ancestor = pwPtr->tkwin;;ancestor = Tk_Parent(ancestor)) {
		    if (ancestor == parent) {
			break;
		    }
		    if (Tk_IsTopLevel(ancestor)) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf(
				"can't add %s to %s", arg,
				Tk_PathName(pwPtr->tkwin)));
			Tcl_SetErrorCode(interp, "TK", "GEOMETRY",
				"HIERARCHY", NULL);
			return TCL_ERROR;
		    }
		}
	    }
	}
    }
    firstOptionArg = i;

    /*
     * Pre-parse the configuration options, to get the before/after specifiers
     * into an easy-to-find location (a local variable). Also, check the
     * return from Tk_SetOptions once, here, so we can save a little bit of
     * extra testing in the for loop below.
     */

    memset((void *)&options, 0, sizeof(Slave));
    if (Tk_SetOptions(interp, (char *) &options, pwPtr->slaveOpts,
	    objc - firstOptionArg, objv + firstOptionArg,
	    pwPtr->tkwin, NULL, NULL) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * If either -after or -before was given, find the numerical index that
     * corresponds to the given window. If both -after and -before are given,
     * the option precedence is: -after, then -before.
     */

    index = -1;
    haveLoc = 0;
    if (options.after != NULL) {
	tkwin = options.after;
	haveLoc = 1;
	for (i = 0; i < pwPtr->numSlaves; i++) {
	    if (options.after == pwPtr->slaves[i]->tkwin) {
		index = i + 1;
		break;
	    }
	}
    } else if (options.before != NULL) {
	tkwin = options.before;
	haveLoc = 1;
	for (i = 0; i < pwPtr->numSlaves; i++) {
	    if (options.before == pwPtr->slaves[i]->tkwin) {
		index = i;
		break;
	    }
	}
    }

    /*
     * If a window was given for -after/-before, but it's not a window managed
     * by the panedwindow, throw an error
     */

    if (haveLoc && index == -1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"window \"%s\" is not managed by %s",
		Tk_PathName(tkwin), Tk_PathName(pwPtr->tkwin)));
	Tcl_SetErrorCode(interp, "TK", "PANEDWINDOW", "UNMANAGED", NULL);
	Tk_FreeConfigOptions((char *) &options, pwPtr->slaveOpts,
		pwPtr->tkwin);
	return TCL_ERROR;
    }

    /*
     * Allocate an array to hold, in order, the pointers to the slave
     * structures corresponding to the windows specified. Some of those
     * structures may already have existed, some may be new.
     */

    inserts = ckalloc(sizeof(Slave *) * (firstOptionArg - 2));
    insertIndex = 0;

    /*
     * Populate the inserts array, creating new slave structures as necessary,
     * applying the options to each structure as we go, and, if necessary,
     * marking the spot in the original slaves array as empty (for
     * pre-existing slave structures).
     */

    for (i = 0, numNewSlaves = 0; i < firstOptionArg - 2; i++) {
	/*
	 * We don't check that tkwin is NULL here, because the pre-pass above
	 * guarantees that the input at this stage is good.
	 */

	tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[i + 2]),
		pwPtr->tkwin);

	found = 0;
	for (j = 0; j < pwPtr->numSlaves; j++) {
	    if (pwPtr->slaves[j] != NULL && pwPtr->slaves[j]->tkwin == tkwin) {
		Tk_SetOptions(interp, (char *) pwPtr->slaves[j],
			pwPtr->slaveOpts, objc - firstOptionArg,
			objv + firstOptionArg, pwPtr->tkwin, NULL, NULL);
		if (pwPtr->slaves[j]->minSize < 0) {
		    pwPtr->slaves[j]->minSize = 0;
		}
		found = 1;

		/*
		 * If the slave is supposed to move, add it to the inserts
		 * array now; otherwise, leave it where it is.
		 */

		if (index != -1) {
		    inserts[insertIndex++] = pwPtr->slaves[j];
		    pwPtr->slaves[j] = NULL;
		}
		break;
	    }
	}

	if (found) {
	    continue;
	}

	/*
	 * Make sure this slave wasn't already put into the inserts array,
	 * i.e., when the user specifies the same window multiple times in a
	 * single add commaned.
	 */
	for (j = 0; j < insertIndex; j++) {
	    if (inserts[j]->tkwin == tkwin) {
		found = 1;
		break;
	    }
	}
	if (found) {
	    continue;
	}

	/*
	 * Create a new slave structure and initialize it. All slaves start
	 * out with their "natural" dimensions.
	 */

	slavePtr = ckalloc(sizeof(Slave));
	memset(slavePtr, 0, sizeof(Slave));
	Tk_InitOptions(interp, (char *)slavePtr, pwPtr->slaveOpts,
		pwPtr->tkwin);
	Tk_SetOptions(interp, (char *)slavePtr, pwPtr->slaveOpts,
		objc - firstOptionArg, objv + firstOptionArg,
		pwPtr->tkwin, NULL, NULL);
	slavePtr->tkwin = tkwin;
	slavePtr->masterPtr = pwPtr;
	doubleBw = 2 * Tk_Changes(slavePtr->tkwin)->border_width;
	if (slavePtr->width > 0) {
	    slavePtr->paneWidth = slavePtr->width;
	} else {
	    slavePtr->paneWidth = Tk_ReqWidth(tkwin) + doubleBw;
	}
	if (slavePtr->height > 0) {
	    slavePtr->paneHeight = slavePtr->height;
	} else {
	    slavePtr->paneHeight = Tk_ReqHeight(tkwin) + doubleBw;
	}
	if (slavePtr->minSize < 0) {
	    slavePtr->minSize = 0;
	}

	/*
	 * Set up the geometry management callbacks for this slave.
	 */

	Tk_CreateEventHandler(slavePtr->tkwin, StructureNotifyMask,
		SlaveStructureProc, slavePtr);
	Tk_ManageGeometry(slavePtr->tkwin, &panedWindowMgrType, slavePtr);
	inserts[insertIndex++] = slavePtr;
	numNewSlaves++;
    }

    /*
     * Allocate the new slaves array, then copy the slaves into it, in order.
     */

    i = sizeof(Slave *) * (pwPtr->numSlaves + numNewSlaves);
    newSlaves = ckalloc(i);
    memset(newSlaves, 0, (size_t) i);
    if (index == -1) {
	/*
	 * If none of the existing slaves have to be moved, just copy the old
	 * and append the new.
	 */
	memcpy((void *)&(newSlaves[0]), pwPtr->slaves,
		sizeof(Slave *) * pwPtr->numSlaves);
	memcpy((void *)&(newSlaves[pwPtr->numSlaves]), inserts,
		sizeof(Slave *) * numNewSlaves);
    } else {
	/*
	 * If some of the existing slaves were moved, the old slaves array
	 * will be partially populated, with some valid and some invalid
	 * entries. Walk through it, copying valid entries to the new slaves
	 * array as we go; when we get to the insert location for the new
	 * slaves, copy the inserts array over, then finish off the old slaves
	 * array.
	 */

	for (i = 0, j = 0; i < index; i++) {
	    if (pwPtr->slaves[i] != NULL) {
		newSlaves[j] = pwPtr->slaves[i];
		j++;
	    }
	}

	memcpy((void *)&(newSlaves[j]), inserts, sizeof(Slave *)*insertIndex);
	j += firstOptionArg - 2;

	for (i = index; i < pwPtr->numSlaves; i++) {
	    if (pwPtr->slaves[i] != NULL) {
		newSlaves[j] = pwPtr->slaves[i];
		j++;
	    }
	}
    }

    /*
     * Make the new slaves array the paned window's slave array, and clean up.
     */

    ckfree(pwPtr->slaves);
    ckfree(inserts);
    pwPtr->slaves = newSlaves;

    /*
     * Set the paned window's slave count to the new value.
     */

    pwPtr->numSlaves += numNewSlaves;

    Tk_FreeConfigOptions((char *) &options, pwPtr->slaveOpts, pwPtr->tkwin);

    ComputeGeometry(pwPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * PanedWindowSashCommand --
 *
 *	Implementation of the panedwindow sash subcommand. See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Depends on the arguments.
 *
 *----------------------------------------------------------------------
 */

static int
PanedWindowSashCommand(
    PanedWindow *pwPtr,		/* Pointer to paned window information. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const sashOptionStrings[] = {
	"coord", "dragto", "mark", "place", NULL
    };
    enum sashOptions {
	SASH_COORD, SASH_DRAGTO, SASH_MARK, SASH_PLACE
    };
    int index, sash, x, y, diff;
    Tcl_Obj *coords[2];
    Slave *slavePtr;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], sashOptionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum sashOptions) index) {
    case SASH_COORD:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index");
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(interp, objv[3], &sash) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (!ValidSashIndex(pwPtr, sash)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "invalid sash index", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "SASH_INDEX", NULL);
	    return TCL_ERROR;
	}
	slavePtr = pwPtr->slaves[sash];

	coords[0] = Tcl_NewIntObj(slavePtr->sashx);
	coords[1] = Tcl_NewIntObj(slavePtr->sashy);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, coords));
	break;

    case SASH_MARK:
	if (objc != 6 && objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index ?x y?");
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(interp, objv[3], &sash) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (!ValidSashIndex(pwPtr, sash)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "invalid sash index", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "SASH_INDEX", NULL);
	    return TCL_ERROR;
	}

	if (objc == 6) {
	    if (Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) {
		return TCL_ERROR;
	    }

	    if (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK) {
		return TCL_ERROR;
	    }

	    pwPtr->slaves[sash]->markx = x;
	    pwPtr->slaves[sash]->marky = y;
	} else {
	    coords[0] = Tcl_NewIntObj(pwPtr->slaves[sash]->markx);
	    coords[1] = Tcl_NewIntObj(pwPtr->slaves[sash]->marky);
	    Tcl_SetObjResult(interp, Tcl_NewListObj(2, coords));
	}
	break;

    case SASH_DRAGTO:
    case SASH_PLACE:
	if (objc != 6) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index x y");
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(interp, objv[3], &sash) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (!ValidSashIndex(pwPtr, sash)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "invalid sash index", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "SASH_INDEX", NULL);
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK) {
	    return TCL_ERROR;
	}

	slavePtr = pwPtr->slaves[sash];
	if (pwPtr->orient == ORIENT_HORIZONTAL) {
	    if (index == SASH_PLACE) {
		diff = x - pwPtr->slaves[sash]->sashx;
	    } else {
		diff = x - pwPtr->slaves[sash]->markx;
	    }
	} else {
	    if (index == SASH_PLACE) {
		diff = y - pwPtr->slaves[sash]->sashy;
	    } else {
		diff = y - pwPtr->slaves[sash]->marky;
	    }
	}

	MoveSash(pwPtr, sash, diff);
	ComputeGeometry(pwPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigurePanedWindow --
 *
 *	This function is called to process an argv/argc list in conjunction
 *	with the Tk option database to configure (or reconfigure) a paned
 *	window widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width, etc. get set
 *	for pwPtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigurePanedWindow(
    Tcl_Interp *interp,		/* Used for error reporting. */
    PanedWindow *pwPtr,		/* Information about widget. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument values. */
{
    Tk_SavedOptions savedOptions;
    int typemask = 0;

    if (Tk_SetOptions(interp, (char *) pwPtr, pwPtr->optionTable, objc, objv,
	    pwPtr->tkwin, &savedOptions, &typemask) != TCL_OK) {
	Tk_RestoreSavedOptions(&savedOptions);
	return TCL_ERROR;
    }

    Tk_FreeSavedOptions(&savedOptions);

    PanedWindowWorldChanged(pwPtr);

    /*
     * If an option that affects geometry has changed, make a re-layout
     * request.
     */

    if (typemask & GEOMETRY) {
	ComputeGeometry(pwPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * PanedWindowWorldChanged --
 *
 *	This function is invoked anytime a paned window's world has changed in
 *	some way that causes the widget to have to recompute graphics contexts
 *	and geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Paned window will be relayed out and redisplayed.
 *
 *----------------------------------------------------------------------
 */

static void
PanedWindowWorldChanged(
    ClientData instanceData)	/* Information about the paned window. */
{
    XGCValues gcValues;
    GC newGC;
    PanedWindow *pwPtr = instanceData;

    /*
     * Allocated a graphics context for drawing the paned window widget
     * elements (background, sashes, etc.) and set the window background.
     */

    gcValues.background = Tk_3DBorderColor(pwPtr->background)->pixel;
    newGC = Tk_GetGC(pwPtr->tkwin, GCBackground, &gcValues);
    if (pwPtr->gc != NULL) {
	Tk_FreeGC(pwPtr->display, pwPtr->gc);
    }
    pwPtr->gc = newGC;
    Tk_SetWindowBackground(pwPtr->tkwin, gcValues.background);

    /*
     * Issue geometry size requests to Tk.
     */

    Tk_SetInternalBorder(pwPtr->tkwin, pwPtr->borderWidth);
    if (pwPtr->width > 0 && pwPtr->height > 0) {
	Tk_GeometryRequest(pwPtr->tkwin, pwPtr->width, pwPtr->height);
    }

    /*
     * Arrange for the window to be redrawn, if neccessary.
     */

    if (Tk_IsMapped(pwPtr->tkwin) && !(pwPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayPanedWindow, pwPtr);
	pwPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *--------------------------------------------------------------
 *
 * PanedWindowEventProc --
 *
 *	This function is invoked by the Tk dispatcher for various events on
 *	paned windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up. When
 *	it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
PanedWindowEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    PanedWindow *pwPtr = clientData;
    int i;

    if (eventPtr->type == Expose) {
	if (pwPtr->tkwin != NULL && !(pwPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayPanedWindow, pwPtr);
	    pwPtr->flags |= REDRAW_PENDING;
	}
    } else if (eventPtr->type == ConfigureNotify) {
	pwPtr->flags |= REQUESTED_RELAYOUT;
	if (pwPtr->tkwin != NULL && !(pwPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayPanedWindow, pwPtr);
	    pwPtr->flags |= REDRAW_PENDING;
	}
    } else if (eventPtr->type == DestroyNotify) {
	DestroyPanedWindow(pwPtr);
    } else if (eventPtr->type == UnmapNotify) {
        for (i = 0; i < pwPtr->numSlaves; i++) {
            if (!pwPtr->slaves[i]->hide) {
                Tk_UnmapWindow(pwPtr->slaves[i]->tkwin);
            }
        }
    } else if (eventPtr->type == MapNotify) {
        for (i = 0; i < pwPtr->numSlaves; i++) {
            if (!pwPtr->slaves[i]->hide) {
                Tk_MapWindow(pwPtr->slaves[i]->tkwin);
            }
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PanedWindowCmdDeletedProc --
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
PanedWindowCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    PanedWindow *pwPtr = clientData;

    /*
     * This function could be invoked either because the window was destroyed
     * and the command was then deleted or because the command was deleted,
     * and then this function destroys the widget. The WIDGET_DELETED flag
     * distinguishes these cases.
     */

    if (!(pwPtr->flags & WIDGET_DELETED)) {
	Tk_DestroyWindow(pwPtr->proxywin);
	Tk_DestroyWindow(pwPtr->tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * DisplayPanedWindow --
 *
 *	This function redraws the contents of a paned window widget. It is
 *	invoked as a do-when-idle handler, so it only runs when there's
 *	nothing else for the application to do.
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
DisplayPanedWindow(
    ClientData clientData)	/* Information about window. */
{
    PanedWindow *pwPtr = clientData;
    Slave *slavePtr;
    Pixmap pixmap;
    Tk_Window tkwin = pwPtr->tkwin;
    int i, sashWidth, sashHeight;
    const int horizontal = (pwPtr->orient == ORIENT_HORIZONTAL);
    int first, last;

    pwPtr->flags &= ~REDRAW_PENDING;
    if ((pwPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    if (pwPtr->flags & REQUESTED_RELAYOUT) {
	ArrangePanes(clientData);
    }

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Create a pixmap for double-buffering, if necessary.
     */

    pixmap = Tk_GetPixmap(Tk_Display(tkwin), Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
#else
    pixmap = Tk_WindowId(tkwin);
#endif /* TK_NO_DOUBLE_BUFFERING */

    /*
     * Redraw the widget's background and border.
     */

    Tk_Fill3DRectangle(tkwin, pixmap, pwPtr->background, 0, 0,
	    Tk_Width(tkwin), Tk_Height(tkwin), pwPtr->borderWidth,
	    pwPtr->relief);

    /*
     * Set up boilerplate geometry values for sashes (width, height, common
     * coordinates).
     */

    if (horizontal) {
	sashHeight = Tk_Height(tkwin) - (2 * Tk_InternalBorderWidth(tkwin));
	sashWidth = pwPtr->sashWidth;
    } else {
	sashWidth = Tk_Width(tkwin) - (2 * Tk_InternalBorderWidth(tkwin));
	sashHeight = pwPtr->sashWidth;
    }

    /*
     * Draw the sashes.
     */

    GetFirstLastVisiblePane(pwPtr, &first, &last);
    for (i = 0; i < pwPtr->numSlaves - 1; i++) {
	slavePtr = pwPtr->slaves[i];
	if (slavePtr->hide || i == last) {
	    continue;
	}
	if (sashWidth > 0 && sashHeight > 0) {
	    Tk_Fill3DRectangle(tkwin, pixmap, pwPtr->background,
		    slavePtr->sashx, slavePtr->sashy, sashWidth, sashHeight,
		    1, pwPtr->sashRelief);
	}
	if (pwPtr->showHandle) {
	    Tk_Fill3DRectangle(tkwin, pixmap, pwPtr->background,
		    slavePtr->handlex, slavePtr->handley,
		    pwPtr->handleSize, pwPtr->handleSize, 1,
		    TK_RELIEF_RAISED);
	}
    }

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Copy the information from the off-screen pixmap onto the screen, then
     * delete the pixmap.
     */

    XCopyArea(Tk_Display(tkwin), pixmap, Tk_WindowId(tkwin), pwPtr->gc, 0, 0,
	    (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(Tk_Display(tkwin), pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyPanedWindow --
 *
 *	This function is invoked by PanedWindowEventProc to free the internal
 *	structure of a paned window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the paned window is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyPanedWindow(
    PanedWindow *pwPtr)		/* Info about paned window widget. */
{
    int i;

    /*
     * First mark the widget as in the process of being deleted, so that any
     * code that causes calls to other paned window functions will abort.
     */

    pwPtr->flags |= WIDGET_DELETED;

    /*
     * Cancel idle callbacks for redrawing the widget and for rearranging the
     * panes.
     */

    if (pwPtr->flags & REDRAW_PENDING) {
	Tcl_CancelIdleCall(DisplayPanedWindow, pwPtr);
    }
    if (pwPtr->flags & RESIZE_PENDING) {
	Tcl_CancelIdleCall(ArrangePanes, pwPtr);
    }

    /*
     * Clean up the slave list; foreach slave:
     *  o  Cancel the slave's structure notification callback
     *  o  Cancel geometry management for the slave.
     *  o  Free memory for the slave
     */

    for (i = 0; i < pwPtr->numSlaves; i++) {
	Tk_DeleteEventHandler(pwPtr->slaves[i]->tkwin, StructureNotifyMask,
		SlaveStructureProc, pwPtr->slaves[i]);
	Tk_ManageGeometry(pwPtr->slaves[i]->tkwin, NULL, NULL);
	Tk_FreeConfigOptions((char *) pwPtr->slaves[i], pwPtr->slaveOpts,
		pwPtr->tkwin);
	ckfree(pwPtr->slaves[i]);
	pwPtr->slaves[i] = NULL;
    }
    if (pwPtr->slaves) {
	ckfree(pwPtr->slaves);
    }

    /*
     * Remove the widget command from the interpreter.
     */

    Tcl_DeleteCommandFromToken(pwPtr->interp, pwPtr->widgetCmd);

    /*
     * Let Tk_FreeConfigOptions clean up the rest.
     */

    Tk_FreeConfigOptions((char *) pwPtr, pwPtr->optionTable, pwPtr->tkwin);
    Tcl_Release(pwPtr->tkwin);
    pwPtr->tkwin = NULL;

    Tcl_EventuallyFree(pwPtr, TCL_DYNAMIC);
}

/*
 *--------------------------------------------------------------
 *
 * PanedWindowReqProc --
 *
 *	This function is invoked by Tk_GeometryRequest for windows managed by
 *	a paned window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for tkwin, and all its managed siblings, to be re-arranged at
 *	the next idle point.
 *
 *--------------------------------------------------------------
 */

static void
PanedWindowReqProc(
    ClientData clientData,	/* Paned window's information about window
				 * that got new preferred geometry. */
    Tk_Window tkwin)		/* Other Tk-related information about the
				 * window. */
{
    Slave *slavePtr = clientData;
    PanedWindow *pwPtr = (PanedWindow *) slavePtr->masterPtr;

    if (Tk_IsMapped(pwPtr->tkwin)) {
	if (!(pwPtr->flags & RESIZE_PENDING)) {
	    pwPtr->flags |= RESIZE_PENDING;
	    Tcl_DoWhenIdle(ArrangePanes, pwPtr);
	}
    } else {
	int doubleBw = 2 * Tk_Changes(slavePtr->tkwin)->border_width;

	if (slavePtr->width <= 0) {
	    slavePtr->paneWidth = Tk_ReqWidth(slavePtr->tkwin) + doubleBw;
	}
	if (slavePtr->height <= 0) {
	    slavePtr->paneHeight = Tk_ReqHeight(slavePtr->tkwin) + doubleBw;
	}
	ComputeGeometry(pwPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * PanedWindowLostSlaveProc --
 *
 *	This function is invoked by Tk whenever some other geometry claims
 *	control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all information about the slave. Causes geometry to be
 *	recomputed for the panedwindow.
 *
 *--------------------------------------------------------------
 */

static void
PanedWindowLostSlaveProc(
    ClientData clientData,	/* Grid structure for slave window that was
				 * stolen away. */
    Tk_Window tkwin)		/* Tk's handle for the slave window. */
{
    register Slave *slavePtr = clientData;
    PanedWindow *pwPtr = (PanedWindow *) slavePtr->masterPtr;

    if (pwPtr->tkwin != Tk_Parent(slavePtr->tkwin)) {
	Tk_UnmaintainGeometry(slavePtr->tkwin, pwPtr->tkwin);
    }
    Unlink(slavePtr);
    Tk_DeleteEventHandler(slavePtr->tkwin, StructureNotifyMask,
	    SlaveStructureProc, slavePtr);
    Tk_UnmapWindow(slavePtr->tkwin);
    slavePtr->tkwin = NULL;
    ckfree(slavePtr);
    ComputeGeometry(pwPtr);
}

/*
 *--------------------------------------------------------------
 *
 * ArrangePanes --
 *
 *	This function is invoked (using the Tcl_DoWhenIdle mechanism) to
 *	re-layout a set of windows managed by a paned window. It is invoked at
 *	idle time so that a series of pane requests can be merged into a
 *	single layout operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The slaves of masterPtr may get resized or moved.
 *
 *--------------------------------------------------------------
 */

static void
ArrangePanes(
    ClientData clientData)	/* Structure describing parent whose slaves
				 * are to be re-layed out. */
{
    register PanedWindow *pwPtr = clientData;
    register Slave *slavePtr;
    int i, slaveWidth, slaveHeight, slaveX, slaveY;
    int paneWidth, paneHeight, paneSize, paneMinSize;
    int doubleBw;
    int x, y;
    int sashWidth, sashOffset, sashCount, handleOffset;
    int sashReserve, sxReserve, syReserve;
    int internalBW;
    int paneDynSize, paneDynMinSize, pwHeight, pwWidth, pwSize;
    int first, last;
    int stretchReserve, stretchAmount;
    const int horizontal = (pwPtr->orient == ORIENT_HORIZONTAL);

    pwPtr->flags &= ~(REQUESTED_RELAYOUT|RESIZE_PENDING);

    /*
     * If the parent has no slaves anymore, then don't do anything at all:
     * just leave the parent's size as-is. Otherwise there is no way to
     * "relinquish" control over the parent so another geometry manager can
     * take over.
     */

    if (pwPtr->numSlaves == 0) {
	return;
    }

    Tcl_Preserve(pwPtr);

    /*
     * Find index of first and last visible panes.
     */

    GetFirstLastVisiblePane(pwPtr, &first, &last);

    /*
     * First pass; compute sizes
     */

    paneDynSize = paneDynMinSize = 0;
    internalBW = Tk_InternalBorderWidth(pwPtr->tkwin);
    pwHeight = Tk_Height(pwPtr->tkwin) - (2 * internalBW);
    pwWidth = Tk_Width(pwPtr->tkwin) - (2 * internalBW);
    x = y = internalBW;
    stretchReserve = (horizontal ? pwWidth : pwHeight);

    /*
     * Calculate the sash width, including handle and padding, and the sash
     * and handle offsets.
     */

    sashOffset = handleOffset = pwPtr->sashPad;
    if (pwPtr->showHandle && pwPtr->handleSize > pwPtr->sashWidth) {
	sashWidth = (2 * pwPtr->sashPad) + pwPtr->handleSize;
	sashOffset = ((pwPtr->handleSize - pwPtr->sashWidth) / 2)
		+ pwPtr->sashPad;
    } else {
	sashWidth = (2 * pwPtr->sashPad) + pwPtr->sashWidth;
	handleOffset = ((pwPtr->sashWidth - pwPtr->handleSize) / 2)
		+ pwPtr->sashPad;
    }

    for (i = sashCount = 0; i < pwPtr->numSlaves; i++) {
	slavePtr = pwPtr->slaves[i];

	if (slavePtr->hide) {
	    continue;
	}

	/*
	 * Compute the total size needed by all the slaves and the left-over,
	 * or shortage of space available.
	 */

	if (horizontal) {
            if (slavePtr->width > 0) {
                paneSize = slavePtr->width;
            } else {
                paneSize = slavePtr->paneWidth;
            }
	    stretchReserve -= paneSize + (2 * slavePtr->padx);
	} else {
            if (slavePtr->height > 0) {
                paneSize = slavePtr->height;
            } else {
                paneSize = slavePtr->paneHeight;
            }
	    stretchReserve -= paneSize + (2 * slavePtr->pady);
	}
	if (IsStretchable(slavePtr->stretch,i,first,last)
		&& Tk_IsMapped(pwPtr->tkwin)) {
	    paneDynSize += paneSize;
	    paneDynMinSize += slavePtr->minSize;
	}
	if (i != last) {
	    stretchReserve -= sashWidth;
	    sashCount++;
	}
    }

    /*
     * Second pass; adjust/arrange panes.
     */

    for (i = 0; i < pwPtr->numSlaves; i++) {
	slavePtr = pwPtr->slaves[i];

	if (slavePtr->hide) {
	    Tk_UnmaintainGeometry(slavePtr->tkwin, pwPtr->tkwin);
	    Tk_UnmapWindow(slavePtr->tkwin);
	    continue;
	}

	/*
	 * Compute the size of this slave. The algorithm (assuming a
	 * horizontal paned window) is:
	 *
	 * 1.  Get "base" dimensions. If a width or height is specified for
	 *     this slave, use those values; else use the ReqWidth/ReqHeight.
	 * 2.  Using base dimensions, pane dimensions, and sticky values,
	 *     determine the x and y, and actual width and height of the
	 *     widget.
	 */

	doubleBw = 2 * Tk_Changes(slavePtr->tkwin)->border_width;
	slaveWidth = (slavePtr->width > 0 ? slavePtr->width :
		Tk_ReqWidth(slavePtr->tkwin) + doubleBw);
	slaveHeight = (slavePtr->height > 0 ? slavePtr->height :
		Tk_ReqHeight(slavePtr->tkwin) + doubleBw);
	paneMinSize = slavePtr->minSize;

	/*
	 * Calculate pane width and height.
	 */

	if (horizontal) {
            if (slavePtr->width > 0) {
                paneSize = slavePtr->width;
            } else {
                paneSize = slavePtr->paneWidth;
            }
	    pwSize = pwWidth;
	} else {
            if (slavePtr->height > 0) {
                paneSize = slavePtr->height;
            } else {
                paneSize = slavePtr->paneHeight;
            }
	    pwSize = pwHeight;
	}
	if (IsStretchable(slavePtr->stretch, i, first, last)) {
	    double frac;

	    if (paneDynSize > 0) {
		frac = (double)paneSize / (double)paneDynSize;
	    } else {
		frac = (double)paneSize / (double)pwSize;
	    }

	    paneDynSize -= paneSize;
	    paneDynMinSize -= slavePtr->minSize;
	    stretchAmount = (int) (frac * stretchReserve);
	    if (paneSize + stretchAmount >= paneMinSize) {
		stretchReserve -= stretchAmount;
		paneSize += stretchAmount;
	    } else {
		stretchReserve += paneSize - paneMinSize;
		paneSize = paneMinSize;
	    }
	    if (i == last && stretchReserve > 0) {
		paneSize += stretchReserve;
		stretchReserve = 0;
	    }
	} else if (paneDynSize - paneDynMinSize + stretchReserve < 0) {
	    if (paneSize + paneDynSize - paneDynMinSize + stretchReserve
		    <= paneMinSize) {
		stretchReserve += paneSize - paneMinSize;
		paneSize = paneMinSize;
	    } else {
		paneSize += paneDynSize - paneDynMinSize + stretchReserve;
		stretchReserve = paneDynMinSize - paneDynSize;
	    }
	}
	if (horizontal) {
	    paneWidth = paneSize;
	    paneHeight = pwHeight - (2 * slavePtr->pady);
	} else {
	    paneWidth = pwWidth - (2 * slavePtr->padx);
	    paneHeight = paneSize;
	}

	/*
	 * Adjust for area reserved for sashes.
	 */

	if (sashCount) {
	    sashReserve = sashWidth * sashCount;
	    if (horizontal) {
		sxReserve = sashReserve;
		syReserve = 0;
	    } else {
		sxReserve = 0;
		syReserve = sashReserve;
	    }
	} else {
	    sxReserve = syReserve = 0;
	}

	if (pwWidth - sxReserve < x + paneWidth - internalBW) {
	    paneWidth = pwWidth - sxReserve - x + internalBW;
	}
	if (pwHeight - syReserve < y + paneHeight - internalBW) {
	    paneHeight = pwHeight - syReserve - y + internalBW;
	}

	if (slaveWidth > paneWidth) {
	    slaveWidth = paneWidth;
	}
	if (slaveHeight > paneHeight) {
	    slaveHeight = paneHeight;
	}

	slavePtr->x = x;
	slavePtr->y = y;

	/*
	 * Compute the location of the sash at the right or bottom of the
	 * parcel and the location of the next parcel.
	 */

	if (horizontal) {
	    x += paneWidth + (2 * slavePtr->padx);
	    if (x < internalBW) {
		x = internalBW;
	    }
	    slavePtr->sashx = x + sashOffset;
	    slavePtr->sashy = y;
	    slavePtr->handlex = x + handleOffset;
	    slavePtr->handley = y + pwPtr->handlePad;
	    x += sashWidth;
	} else {
	    y += paneHeight + (2 * slavePtr->pady);
	    if (y < internalBW) {
		y = internalBW;
	    }
	    slavePtr->sashx = x;
	    slavePtr->sashy = y + sashOffset;
	    slavePtr->handlex = x + pwPtr->handlePad;
	    slavePtr->handley = y + handleOffset;
	    y += sashWidth;
	}

	/*
	 * Compute the actual dimensions of the slave in the pane.
	 */

	slaveX = slavePtr->x;
	slaveY = slavePtr->y;
	AdjustForSticky(slavePtr->sticky, paneWidth, paneHeight,
		&slaveX, &slaveY, &slaveWidth, &slaveHeight);

	slaveX += slavePtr->padx;
	slaveY += slavePtr->pady;

	/*
	 * Now put the window in the proper spot.
	 */

	if (slaveWidth <= 0 || slaveHeight <= 0 ||
		(horizontal ? slaveX - internalBW > pwWidth :
		slaveY - internalBW > pwHeight)) {
	    Tk_UnmaintainGeometry(slavePtr->tkwin, pwPtr->tkwin);
	    Tk_UnmapWindow(slavePtr->tkwin);
	} else {
	    Tk_MaintainGeometry(slavePtr->tkwin, pwPtr->tkwin,
		    slaveX, slaveY, slaveWidth, slaveHeight);
	}
	sashCount--;
    }
    Tcl_Release(pwPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Unlink --
 *
 *	Remove a slave from a paned window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The paned window will be scheduled for re-arranging and redrawing.
 *
 *----------------------------------------------------------------------
 */

static void
Unlink(
    register Slave *slavePtr)		/* Window to unlink. */
{
    register PanedWindow *masterPtr;
    int i, j;

    masterPtr = slavePtr->masterPtr;
    if (masterPtr == NULL) {
	return;
    }

    /*
     * Find the specified slave in the panedwindow's list of slaves, then
     * remove it from that list.
     */

    for (i = 0; i < masterPtr->numSlaves; i++) {
	if (masterPtr->slaves[i] == slavePtr) {
	    for (j = i; j < masterPtr->numSlaves - 1; j++) {
		masterPtr->slaves[j] = masterPtr->slaves[j + 1];
	    }
	    break;
	}
    }

    /*
     * Clean out any -after or -before references to this slave
     */

    for (i = 0; i < masterPtr->numSlaves; i++) {
	if (masterPtr->slaves[i]->before == slavePtr->tkwin) {
	    masterPtr->slaves[i]->before = NULL;
	}
	if (masterPtr->slaves[i]->after == slavePtr->tkwin) {
	    masterPtr->slaves[i]->after = NULL;
	}
    }

    masterPtr->flags |= REQUESTED_RELAYOUT;
    if (!(masterPtr->flags & REDRAW_PENDING)) {
	masterPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayPanedWindow, masterPtr);
    }

    /*
     * Set the slave's masterPtr to NULL, so that we can tell that the slave
     * is no longer attached to any panedwindow.
     */

    slavePtr->masterPtr = NULL;

    masterPtr->numSlaves--;
}

/*
 *----------------------------------------------------------------------
 *
 * GetPane --
 *
 *	Given a token to a Tk window, find the pane that corresponds to that
 *	token in a given paned window.
 *
 * Results:
 *	Pointer to the slave structure, or NULL if the window is not managed
 *	by this paned window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Slave *
GetPane(
    PanedWindow *pwPtr,		/* Pointer to the paned window info. */
    Tk_Window tkwin)		/* Window to search for. */
{
    int i;

    for (i = 0; i < pwPtr->numSlaves; i++) {
	if (pwPtr->slaves[i]->tkwin == tkwin) {
	    return pwPtr->slaves[i];
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * GetFirstLastVisiblePane --
 *
 *	Given panedwindow, find the index of the first and last visible panes
 *	of that paned window.
 *
 * Results:
 *	Index of the first and last visible panes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetFirstLastVisiblePane(
    PanedWindow *pwPtr,		/* Pointer to the paned window info. */
    int *firstPtr, 		/* Returned index for first. */
    int *lastPtr)  		/* Returned index for last. */
{
    int i;

    for (i = 0, *lastPtr = 0, *firstPtr = -1; i < pwPtr->numSlaves; i++) {
	if (pwPtr->slaves[i]->hide == 0) {
	    if (*firstPtr < 0) {
		*firstPtr = i;
	    }
	    *lastPtr = i;
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * SlaveStructureProc --
 *
 *	This function is invoked whenever StructureNotify events occur for a
 *	window that's managed by a paned window. This function's only purpose
 *	is to clean up when windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The paned window slave structure associated with the window
 *	is freed, and the slave is disassociated from the paned
 *	window which managed it.
 *
 *--------------------------------------------------------------
 */

static void
SlaveStructureProc(
    ClientData clientData,	/* Pointer to record describing window item. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    Slave *slavePtr = clientData;
    PanedWindow *pwPtr = slavePtr->masterPtr;

    if (eventPtr->type == DestroyNotify) {
	Unlink(slavePtr);
	slavePtr->tkwin = NULL;
	ckfree(slavePtr);
	ComputeGeometry(pwPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeGeometry --
 *
 *	Compute geometry for the paned window, including coordinates of all
 *	slave windows and each sash.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Recomputes geometry information for a paned window.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeGeometry(
    PanedWindow *pwPtr)		/* Pointer to the Paned Window structure. */
{
    int i, x, y, doubleBw, internalBw;
    int sashWidth, sashOffset, handleOffset;
    int reqWidth, reqHeight, dim;
    Slave *slavePtr;
    const int horizontal = (pwPtr->orient == ORIENT_HORIZONTAL);

    pwPtr->flags |= REQUESTED_RELAYOUT;

    x = y = internalBw = Tk_InternalBorderWidth(pwPtr->tkwin);
    reqWidth = reqHeight = 0;

    /*
     * Sashes and handles share space on the display. To simplify processing
     * below, precompute the x and y offsets of the handles and sashes within
     * the space occupied by their combination; later, just add those offsets
     * blindly (avoiding the extra showHandle, etc, checks).
     */

    sashOffset = handleOffset = pwPtr->sashPad;
    if (pwPtr->showHandle && pwPtr->handleSize > pwPtr->sashWidth) {
	sashWidth = (2 * pwPtr->sashPad) + pwPtr->handleSize;
	sashOffset = ((pwPtr->handleSize - pwPtr->sashWidth) / 2)
		+ pwPtr->sashPad;
    } else {
	sashWidth = (2 * pwPtr->sashPad) + pwPtr->sashWidth;
	handleOffset = ((pwPtr->sashWidth - pwPtr->handleSize) / 2)
		+ pwPtr->sashPad;
    }

    for (i = 0; i < pwPtr->numSlaves; i++) {
	slavePtr = pwPtr->slaves[i];

	if (slavePtr->hide) {
	    continue;
	}

	/*
	 * First set the coordinates for the top left corner of the slave's
	 * parcel.
	 */

	slavePtr->x = x;
	slavePtr->y = y;

	/*
	 * Make sure the pane's paned dimension is at least minsize. This
	 * check may be redundant, since the only way to change a pane's size
	 * is by moving a sash, and that code checks the minsize.
	 */

	if (horizontal) {
	    if (slavePtr->paneWidth < slavePtr->minSize) {
		slavePtr->paneWidth = slavePtr->minSize;
	    }
	} else {
	    if (slavePtr->paneHeight < slavePtr->minSize) {
		slavePtr->paneHeight = slavePtr->minSize;
	    }
	}

	/*
	 * Compute the location of the sash at the right or bottom of the
	 * parcel.
	 */

	if (horizontal) {
	    x += slavePtr->paneWidth + (2 * slavePtr->padx);
	    slavePtr->sashx = x + sashOffset;
	    slavePtr->sashy = y;
	    slavePtr->handlex = x + handleOffset;
	    slavePtr->handley = y + pwPtr->handlePad;
	    x += sashWidth;
	} else {
	    y += slavePtr->paneHeight + (2 * slavePtr->pady);
	    slavePtr->sashx = x;
	    slavePtr->sashy = y + sashOffset;
	    slavePtr->handlex = x + pwPtr->handlePad;
	    slavePtr->handley = y + handleOffset;
	    y += sashWidth;
	}

	/*
	 * Find the maximum height/width of the slaves, for computing the
	 * requested height/width of the paned window.
	 */

	if (horizontal) {
	    /*
	     * If the slave has an explicit height set, use that; otherwise,
	     * use the slave's requested height.
	     */

	    if (slavePtr->height > 0) {
		dim = slavePtr->height;
	    } else {
		doubleBw = 2 * Tk_Changes(slavePtr->tkwin)->border_width;
		dim = Tk_ReqHeight(slavePtr->tkwin) + doubleBw;
	    }
	    dim += 2 * slavePtr->pady;
	    if (dim > reqHeight) {
		reqHeight = dim;
	    }
	} else {
	    /*
	     * If the slave has an explicit width set use that; otherwise, use
	     * the slave's requested width.
	     */

	    if (slavePtr->width > 0) {
		dim = slavePtr->width;
	    } else {
		doubleBw = 2 * Tk_Changes(slavePtr->tkwin)->border_width;
		dim = Tk_ReqWidth(slavePtr->tkwin) + doubleBw;
	    }
	    dim += 2 * slavePtr->padx;
	    if (dim > reqWidth) {
		reqWidth = dim;
	    }
	}
    }

    /*
     * The loop above should have left x (or y) equal to the sum of the widths
     * (or heights) of the widgets, plus the size of one sash and the sash
     * padding for each widget, plus the width of the left (or top) border of
     * the paned window.
     *
     * The requested width (or height) is therefore x (or y) minus the size of
     * one sash and padding, plus the width of the right (or bottom) border of
     * the paned window.
     *
     * The height (or width) is equal to the maximum height (or width) of the
     * slaves, plus the width of the border of the top and bottom (or left and
     * right) of the paned window.
     *
     * If the panedwindow has an explicit width/height set use that;
     * otherwise, use the requested width/height.
     */

    if (horizontal) {
	reqWidth = (pwPtr->width > 0 ?
		pwPtr->width : x - sashWidth + internalBw);
	reqHeight = (pwPtr->height > 0 ?
		pwPtr->height : reqHeight + (2 * internalBw));
    } else {
	reqWidth = (pwPtr->width > 0 ?
		pwPtr->width : reqWidth + (2 * internalBw));
	reqHeight = (pwPtr->height > 0 ?
		pwPtr->height : y - sashWidth + internalBw);
    }
    Tk_GeometryRequest(pwPtr->tkwin, reqWidth, reqHeight);
    if (Tk_IsMapped(pwPtr->tkwin) && !(pwPtr->flags & REDRAW_PENDING)) {
	pwPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayPanedWindow, pwPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyOptionTables --
 *
 *	This function is registered as an exit callback when the paned window
 *	command is first called. It cleans up the OptionTables structure
 *	allocated by that command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyOptionTables(
    ClientData clientData,	/* Pointer to the OptionTables struct */
    Tcl_Interp *interp)		/* Pointer to the calling interp */
{
    ckfree(clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * GetSticky -
 *
 *	Converts an internal boolean combination of "sticky" bits into a Tcl
 *	string obj containing zero or more of n, s, e, or w.
 *
 * Results:
 *	Tcl_Obj containing the string representation of the sticky value.
 *
 * Side effects:
 *	Creates a new Tcl_Obj.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
GetSticky(
    ClientData clientData,
    Tk_Window tkwin,
    char *recordPtr,		/* Pointer to widget record. */
    int internalOffset)		/* Offset within *recordPtr containing the
				 * sticky value. */
{
    int sticky = *(int *)(recordPtr + internalOffset);
    char buffer[5];
    char *p = &buffer[0];

    if (sticky & STICK_NORTH) {
	*p++ = 'n';
    }
    if (sticky & STICK_EAST) {
	*p++ = 'e';
    }
    if (sticky & STICK_SOUTH) {
	*p++ = 's';
    }
    if (sticky & STICK_WEST) {
	*p++ = 'w';
    }
    *p = '\0';

    return Tcl_NewStringObj(buffer, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * SetSticky --
 *
 *	Converts a Tcl_Obj representing a widgets stickyness into an integer
 *	value.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May store the integer value into the internal representation pointer.
 *	May change the pointer to the Tcl_Obj to NULL to indicate that the
 *	specified string was empty and that is acceptable.
 *
 *----------------------------------------------------------------------
 */

static int
SetSticky(
    ClientData clientData,
    Tcl_Interp *interp,		/* Current interp; may be used for errors. */
    Tk_Window tkwin,		/* Window for which option is being set. */
    Tcl_Obj **value,		/* Pointer to the pointer to the value object.
				 * We use a pointer to the pointer because we
				 * may need to return a value (NULL). */
    char *recordPtr,		/* Pointer to storage for the widget record. */
    int internalOffset,		/* Offset within *recordPtr at which the
				 * internal value is to be stored. */
    char *oldInternalPtr,	/* Pointer to storage for the old value. */
    int flags)			/* Flags for the option, set Tk_SetOptions. */
{
    int sticky = 0;
    char c, *internalPtr;
    const char *string;

    internalPtr = ComputeSlotAddress(recordPtr, internalOffset);

    if (flags & TK_OPTION_NULL_OK && ObjectIsEmpty(*value)) {
	*value = NULL;
    } else {
	/*
	 * Convert the sticky specifier into an integer value.
	 */

	string = Tcl_GetString(*value);

	while ((c = *string++) != '\0') {
	    switch (c) {
	    case 'n': case 'N':
		sticky |= STICK_NORTH;
		break;
	    case 'e': case 'E':
		sticky |= STICK_EAST;
		break;
	    case 's': case 'S':
		sticky |= STICK_SOUTH;
		break;
	    case 'w': case 'W':
		sticky |= STICK_WEST;
		break;
	    case ' ': case ',': case '\t': case '\r': case '\n':
		break;
	    default:
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"bad stickyness value \"%s\": must be a string"
			" containing zero or more of n, e, s, and w",
			Tcl_GetString(*value)));
		Tcl_SetErrorCode(interp, "TK", "VALUE", "STICKY", NULL);
		return TCL_ERROR;
	    }
	}
    }

    if (internalPtr != NULL) {
	*((int *) oldInternalPtr) = *((int *) internalPtr);
	*((int *) internalPtr) = sticky;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RestoreSticky --
 *
 *	Restore a sticky option value from a saved value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Restores the old value.
 *
 *----------------------------------------------------------------------
 */

static void
RestoreSticky(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr,		/* Pointer to storage for value. */
    char *oldInternalPtr)	/* Pointer to old value. */
{
    *(int *)internalPtr = *(int *)oldInternalPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * AdjustForSticky --
 *
 *	Given the x,y coords of the top-left corner of a pane, the dimensions
 *	of that pane, and the dimensions of a slave, compute the x,y coords
 *	and actual dimensions of the slave based on the slave's sticky value.
 *
 * Results:
 *	No direct return; sets the x, y, slaveWidth and slaveHeight to correct
 *	values.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AdjustForSticky(
    int sticky,			/* Sticky value; see top of file for
				 * definition. */
    int cavityWidth,		/* Width of the cavity. */
    int cavityHeight,		/* Height of the cavity. */
    int *xPtr, int *yPtr,	/* Initially, coordinates of the top-left
				 * corner of cavity; also return values for
				 * actual x, y coords of slave. */
    int *slaveWidthPtr,		/* Slave width. */
    int *slaveHeightPtr)	/* Slave height. */
{
    int diffx = 0;		/* Cavity width - slave width. */
    int diffy = 0;		/* Cavity hight - slave height. */

    if (cavityWidth > *slaveWidthPtr) {
	diffx = cavityWidth - *slaveWidthPtr;
    }

    if (cavityHeight > *slaveHeightPtr) {
	diffy = cavityHeight - *slaveHeightPtr;
    }

    if ((sticky & STICK_EAST) && (sticky & STICK_WEST)) {
	*slaveWidthPtr += diffx;
    }
    if ((sticky & STICK_NORTH) && (sticky & STICK_SOUTH)) {
	*slaveHeightPtr += diffy;
    }
    if (!(sticky & STICK_WEST)) {
	*xPtr += (sticky & STICK_EAST) ? diffx : diffx/2;
    }
    if (!(sticky & STICK_NORTH)) {
	*yPtr += (sticky & STICK_SOUTH) ? diffy : diffy/2;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MoveSash --
 *
 *	Move the sash given by index the amount given.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Recomputes the sizes of the panes in a panedwindow.
 *
 *----------------------------------------------------------------------
 */

static void
MoveSash(
    PanedWindow *pwPtr,
    int sash,
    int diff)
{
    int i;
    int expandPane, reduceFirst, reduceLast, reduceIncr, slaveSize, sashOffset;
    Slave *slavePtr;
    int stretchReserve = 0;
    int nextSash = sash + 1;
    const int horizontal = (pwPtr->orient == ORIENT_HORIZONTAL);

    if (diff == 0)
	return;

    /*
     * Update the slave sizes with their real sizes.
     */

    if (pwPtr->showHandle && pwPtr->handleSize > pwPtr->sashWidth) {
	sashOffset = ((pwPtr->handleSize - pwPtr->sashWidth) / 2)
		+ pwPtr->sashPad;
    } else {
	sashOffset = pwPtr->sashPad;
    }
    for (i = 0; i < pwPtr->numSlaves; i++) {
	slavePtr = pwPtr->slaves[i];
	if (slavePtr->hide) {
	    continue;
	}
	if (horizontal) {
	    slavePtr->paneWidth = slavePtr->width = slavePtr->sashx
		    - sashOffset - slavePtr->x - (2 * slavePtr->padx);
	} else {
	    slavePtr->paneHeight = slavePtr->height = slavePtr->sashy
		    - sashOffset - slavePtr->y - (2 * slavePtr->pady);
	}
    }

    /*
     * There must be a next sash since it is only possible to enter this
     * routine when moving an actual sash which implies there exists a visible
     * pane to either side of the sash.
     */

    while (nextSash < pwPtr->numSlaves-1 && pwPtr->slaves[nextSash]->hide) {
	nextSash++;
    }

    /*
     * Consolidate +/-diff variables to reduce duplicate code.
     */

    if (diff > 0) {
	expandPane = sash;
	reduceFirst = nextSash;
	reduceLast = pwPtr->numSlaves;
	reduceIncr = 1;
    } else {
	diff = abs(diff);
	expandPane = nextSash;
	reduceFirst = sash;
	reduceLast = -1;
	reduceIncr = -1;
    }

    /*
     * Calculate how much room we have to stretch in and adjust diff value
     * accordingly.
     */

    for (i = reduceFirst; i != reduceLast; i += reduceIncr) {
	slavePtr = pwPtr->slaves[i];
	if (slavePtr->hide) {
	    continue;
	}
	if (horizontal) {
	    stretchReserve += slavePtr->width - slavePtr->minSize;
	} else {
	    stretchReserve += slavePtr->height - slavePtr->minSize;
	}
    }
    if (stretchReserve <= 0) {
	return;
    }
    if (diff > stretchReserve) {
	diff = stretchReserve;
    }

    /*
     * Expand pane by diff amount.
     */

    slavePtr = pwPtr->slaves[expandPane];
    if (horizontal) {
	slavePtr->paneWidth = slavePtr->width += diff;
    } else {
	slavePtr->paneHeight = slavePtr->height += diff;
    }

    /*
     * Reduce panes, respecting minsize, until diff amount has been used.
     */

    for (i = reduceFirst; i != reduceLast; i += reduceIncr) {
	slavePtr = pwPtr->slaves[i];
	if (slavePtr->hide) {
	    continue;
	}
	if (horizontal) {
	    slaveSize = slavePtr->width;
	} else {
	    slaveSize = slavePtr->height;
	}
	if (diff > (slaveSize - slavePtr->minSize)) {
	    diff -= slaveSize - slavePtr->minSize;
	    slaveSize = slavePtr->minSize;
	} else {
	    slaveSize -= diff;
	    i = reduceLast - reduceIncr;
	}
	if (horizontal) {
	    slavePtr->paneWidth = slavePtr->width = slaveSize;
	} else {
	    slavePtr->paneHeight = slavePtr->height = slaveSize;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ProxyWindowEventProc --
 *
 *	This function is invoked by the Tk dispatcher for various events on
 *	paned window proxy windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up. When
 *	it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ProxyWindowEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    PanedWindow *pwPtr = clientData;

    if (eventPtr->type == Expose) {
	if (pwPtr->proxywin != NULL &&!(pwPtr->flags & PROXY_REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayProxyWindow, pwPtr);
	    pwPtr->flags |= PROXY_REDRAW_PENDING;
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * DisplayProxyWindow --
 *
 *	This function redraws a paned window proxy window. It is invoked as a
 *	do-when-idle handler, so it only runs when there's nothing else for
 *	the application to do.
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
DisplayProxyWindow(
    ClientData clientData)	/* Information about window. */
{
    PanedWindow *pwPtr = clientData;
    Pixmap pixmap;
    Tk_Window tkwin = pwPtr->proxywin;
    pwPtr->flags &= ~PROXY_REDRAW_PENDING;
    if ((tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Create a pixmap for double-buffering, if necessary.
     */

    pixmap = Tk_GetPixmap(Tk_Display(tkwin), Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
#else
    pixmap = Tk_WindowId(tkwin);
#endif /* TK_NO_DOUBLE_BUFFERING */

    /*
     * Redraw the widget's background and border.
     */

    Tk_Fill3DRectangle(tkwin, pixmap,
	    pwPtr->proxyBackground ? pwPtr->proxyBackground : pwPtr->background,
	    0, 0, Tk_Width(tkwin), Tk_Height(tkwin), pwPtr->proxyBorderWidth,
	    (pwPtr->proxyRelief != TK_RELIEF_NULL) ? pwPtr->proxyRelief : pwPtr->sashRelief);

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Copy the pixmap to the display.
     */

    XCopyArea(Tk_Display(tkwin), pixmap, Tk_WindowId(tkwin), pwPtr->gc, 0, 0,
	    (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(Tk_Display(tkwin), pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */
}

/*
 *----------------------------------------------------------------------
 *
 * PanedWindowProxyCommand --
 *
 *	Handles the panedwindow proxy subcommand. See the user documentation
 *	for details.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May map or unmap the proxy sash.
 *
 *----------------------------------------------------------------------
 */

static int
PanedWindowProxyCommand(
    PanedWindow *pwPtr,		/* Pointer to paned window information. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const optionStrings[] = {
	"coord", "forget", "place", NULL
    };
    enum options {
	PROXY_COORD, PROXY_FORGET, PROXY_PLACE
    };
    int index, x, y, sashWidth, sashHeight;
    int internalBW, pwWidth, pwHeight;
    Tcl_Obj *coords[2];

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case PROXY_COORD:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}

	coords[0] = Tcl_NewIntObj(pwPtr->proxyx);
	coords[1] = Tcl_NewIntObj(pwPtr->proxyy);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, coords));
	break;

    case PROXY_FORGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	if (Tk_IsMapped(pwPtr->proxywin)) {
	    Tk_UnmapWindow(pwPtr->proxywin);
	    Tk_UnmaintainGeometry(pwPtr->proxywin, pwPtr->tkwin);
	}
	break;

    case PROXY_PLACE:
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 3, objv, "x y");
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	    return TCL_ERROR;
	}

        internalBW = Tk_InternalBorderWidth(pwPtr->tkwin);
	if (pwPtr->orient == ORIENT_HORIZONTAL) {
	    if (x < 0) {
		x = 0;
	    }
            pwWidth = Tk_Width(pwPtr->tkwin) - (2 * internalBW);
            if (x > pwWidth) {
                x = pwWidth;
            }
            y = Tk_InternalBorderWidth(pwPtr->tkwin);
	    sashWidth = pwPtr->sashWidth;
	    sashHeight = Tk_Height(pwPtr->tkwin) -
		    (2 * Tk_InternalBorderWidth(pwPtr->tkwin));
	} else {
	    if (y < 0) {
		y = 0;
	    }
            pwHeight = Tk_Height(pwPtr->tkwin) - (2 * internalBW);
            if (y > pwHeight) {
                y = pwHeight;
            }
	    x = Tk_InternalBorderWidth(pwPtr->tkwin);
	    sashHeight = pwPtr->sashWidth;
	    sashWidth = Tk_Width(pwPtr->tkwin) -
		    (2 * Tk_InternalBorderWidth(pwPtr->tkwin));
	}

	if (sashWidth < 1) {
	    sashWidth = 1;
	}
	if (sashHeight < 1) {
	    sashHeight = 1;
	}

	/*
	 * Stash the proxy coordinates for future "proxy coord" calls.
	 */

	pwPtr->proxyx = x;
	pwPtr->proxyy = y;

	/*
	 * Make sure the proxy window is higher in the stacking order than the
	 * slaves, so that it will be visible when drawn. It would be more
	 * correct to push the proxy window just high enough to appear above
	 * the highest slave, but it's much easier to just force it all the
	 * way to the top of the stacking order.
	 */

	Tk_RestackWindow(pwPtr->proxywin, Above, NULL);

	/*
	 * Let Tk_MaintainGeometry take care of placing the window at the
	 * right coordinates.
	 */

	Tk_MaintainGeometry(pwPtr->proxywin, pwPtr->tkwin, x, y,
		sashWidth, sashHeight);
	break;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ObjectIsEmpty --
 *
 *	This function tests whether the string value of an object is empty.
 *
 * Results:
 *	The return value is 1 if the string value of objPtr has length zero,
 *	and 0 otherwise.
 *
 * Side effects:
 *	May cause object shimmering, since this function can force a
 *	conversion to a string object.
 *
 *----------------------------------------------------------------------
 */

static int
ObjectIsEmpty(
    Tcl_Obj *objPtr)		/* Object to test. May be NULL. */
{
    if (objPtr == NULL) {
	return 1;
    }
    if (objPtr->bytes == NULL) {
	Tcl_GetString(objPtr);
    }
    return (objPtr->length == 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeInternalPointer --
 *
 *	Given a pointer to the start of a record and the offset of a slot
 *	within that record, compute the address of that slot.
 *
 * Results:
 *	If offset is non-negative, returns the computed address; else, returns
 *	NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
ComputeSlotAddress(
    char *recordPtr,	/* Pointer to the start of a record. */
    int offset)		/* Offset of a slot within that record; may be < 0. */
{
    if (offset >= 0) {
	return recordPtr + offset;
    } else {
	return NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PanedWindowIdentifyCoords --
 *
 *	Given a pair of x,y coordinates, identify the panedwindow component at
 *	that point, if any.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Modifies the interpreter's result to contain either an empty list, or
 *	a two element list of the form {sash n} or {handle n} to indicate that
 *	the point lies within the n'th sash or handle.
 *
 *----------------------------------------------------------------------
 */

static int
PanedWindowIdentifyCoords(
    PanedWindow *pwPtr,		/* Information about the widget. */
    Tcl_Interp *interp,		/* Interpreter in which to store result. */
    int x, int y)		/* Coordinates of the point to identify. */
{
    int i, sashHeight, sashWidth, thisx, thisy;
    int found, isHandle, lpad, rpad, tpad, bpad;
    int first, last;

    if (pwPtr->orient == ORIENT_HORIZONTAL) {
	if (Tk_IsMapped(pwPtr->tkwin)) {
	    sashHeight = Tk_Height(pwPtr->tkwin);
	} else {
	    sashHeight = Tk_ReqHeight(pwPtr->tkwin);
	}
	sashHeight -= 2 * Tk_InternalBorderWidth(pwPtr->tkwin);
	if (pwPtr->showHandle && pwPtr->handleSize > pwPtr->sashWidth) {
	    sashWidth = pwPtr->handleSize;
	    lpad = (pwPtr->handleSize - pwPtr->sashWidth) / 2;
	    rpad = pwPtr->handleSize - lpad;
	    lpad += pwPtr->sashPad;
	    rpad += pwPtr->sashPad;
	} else {
	    sashWidth = pwPtr->sashWidth;
	    lpad = rpad = pwPtr->sashPad;
	}
	tpad = bpad = 0;
    } else {
	if (pwPtr->showHandle && pwPtr->handleSize > pwPtr->sashWidth) {
	    sashHeight = pwPtr->handleSize;
	    tpad = (pwPtr->handleSize - pwPtr->sashWidth) / 2;
	    bpad = pwPtr->handleSize - tpad;
	    tpad += pwPtr->sashPad;
	    bpad += pwPtr->sashPad;
	} else {
	    sashHeight = pwPtr->sashWidth;
	    tpad = bpad = pwPtr->sashPad;
	}
	if (Tk_IsMapped(pwPtr->tkwin)) {
	    sashWidth = Tk_Width(pwPtr->tkwin);
	} else {
	    sashWidth = Tk_ReqWidth(pwPtr->tkwin);
	}
	sashWidth -= 2 * Tk_InternalBorderWidth(pwPtr->tkwin);
	lpad = rpad = 0;
    }

    GetFirstLastVisiblePane(pwPtr, &first, &last);
    isHandle = 0;
    found = -1;
    for (i = 0; i < pwPtr->numSlaves - 1; i++) {
	if (pwPtr->slaves[i]->hide || i == last) {
	    continue;
	}
	thisx = pwPtr->slaves[i]->sashx;
	thisy = pwPtr->slaves[i]->sashy;

	if (((thisx - lpad) <= x && x <= (thisx + rpad + sashWidth)) &&
		((thisy - tpad) <= y && y <= (thisy + bpad + sashHeight))) {
	    found = i;

	    /*
	     * Determine if the point is over the handle or the sash.
	     */

	    if (pwPtr->showHandle) {
		thisx = pwPtr->slaves[i]->handlex;
		thisy = pwPtr->slaves[i]->handley;
		if (pwPtr->orient == ORIENT_HORIZONTAL) {
		    if (thisy <= y && y <= (thisy + pwPtr->handleSize)) {
			isHandle = 1;
		    }
		} else {
		    if (thisx <= x && x <= (thisx + pwPtr->handleSize)) {
			isHandle = 1;
		    }
		}
	    }
	    break;
	}
    }

    /*
     * Set results. Note that the empty string is the default (this function
     * is called inside the implementation of a command).
     */

    if (found != -1) {
	Tcl_Obj *list[2];

	list[0] = Tcl_NewIntObj(found);
	list[1] = Tcl_NewStringObj((isHandle ? "handle" : "sash"), -1);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, list));
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
