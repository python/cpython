/*
 * tkUnixWm.c --
 *
 *	This module takes care of the interactions between a Tk-based
 *	application and the window manager. Among other things, it implements
 *	the "wm" command and passes geometry information to the window
 *	manager.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"

/*
 * A data structure of the following type holds information for each window
 * manager protocol (such as WM_DELETE_WINDOW) for which a handler (i.e. a Tcl
 * command) has been defined for a particular top-level window.
 */

typedef struct ProtocolHandler {
    Atom protocol;		/* Identifies the protocol. */
    struct ProtocolHandler *nextPtr;
				/* Next in list of protocol handlers for the
				 * same top-level window, or NULL for end of
				 * list. */
    Tcl_Interp *interp;		/* Interpreter in which to invoke command. */
    char command[1];		/* Tcl command to invoke when a client message
				 * for this protocol arrives. The actual size
				 * of the structure varies to accommodate the
				 * needs of the actual command. THIS MUST BE
				 * THE LAST FIELD OF THE STRUCTURE. */
} ProtocolHandler;

#define HANDLER_SIZE(cmdLength) \
    ((unsigned) ((Tk_Offset(ProtocolHandler, command) + 1) + cmdLength))

/*
 * Data for [wm attributes] command:
 */

typedef struct {
    double alpha;		/* Transparency; 0.0=transparent, 1.0=opaque */
    int topmost;		/* Flag: true=>stay-on-top */
    int zoomed;			/* Flag: true=>maximized */
    int fullscreen;		/* Flag: true=>fullscreen */
} WmAttributes;

typedef enum {
    WMATT_ALPHA, WMATT_TOPMOST, WMATT_ZOOMED, WMATT_FULLSCREEN,
    WMATT_TYPE, _WMATT_LAST_ATTRIBUTE
} WmAttribute;

static const char *const WmAttributeNames[] = {
    "-alpha", "-topmost", "-zoomed", "-fullscreen",
    "-type", NULL
};

/*
 * A data structure of the following type holds window-manager-related
 * information for each top-level window in an application.
 */

typedef struct TkWmInfo {
    TkWindow *winPtr;		/* Pointer to main Tk information for this
				 * window. */
    Window reparent;		/* If the window has been reparented, this
				 * gives the ID of the ancestor of the window
				 * that is a child of the root window (may not
				 * be window's immediate parent). If the
				 * window isn't reparented, this has the value
				 * None. */
    char *title;		/* Title to display in window caption. If
				 * NULL, use name of widget. Malloced. */
    char *iconName;		/* Name to display in icon. Malloced. */
    XWMHints hints;		/* Various pieces of information for window
				 * manager. */
    char *leaderName;		/* Path name of leader of window group
				 * (corresponds to hints.window_group).
				 * Malloc-ed. Note: this field doesn't get
				 * updated if leader is destroyed. */
    TkWindow *masterPtr;	/* Master window for TRANSIENT_FOR property,
				 * or NULL. */
    Tk_Window icon;		/* Window to use as icon for this window, or
				 * NULL. */
    Tk_Window iconFor;		/* Window for which this window is icon, or
				 * NULL if this isn't an icon for anyone. */
    int withdrawn;		/* Non-zero means window has been withdrawn. */

    /*
     * In order to support menubars transparently under X, each toplevel
     * window is encased in an additional window, called the wrapper, that
     * holds the toplevel and the menubar, if any. The information below is
     * used to keep track of the wrapper and the menubar.
     */

    TkWindow *wrapperPtr;	/* Pointer to information about the wrapper.
				 * This is the "real" toplevel window as seen
				 * by the window manager. Although this is an
				 * official Tk window, it doesn't appear in
				 * the application's window hierarchy. NULL
				 * means that the wrapper hasn't been created
				 * yet. */
    Tk_Window menubar;		/* Pointer to information about the menubar,
				 * or NULL if there is no menubar for this
				 * toplevel. */
    int menuHeight;		/* Amount of vertical space needed for
				 * menubar, measured in pixels. If menubar is
				 * non-NULL, this is >= 1 (X servers don't
				 * like dimensions of 0). */

    /*
     * Information used to construct an XSizeHints structure for the window
     * manager:
     */

    int sizeHintsFlags;		/* Flags word for XSizeHints structure. If the
				 * PBaseSize flag is set then the window is
				 * gridded; otherwise it isn't gridded. */
    int minWidth, minHeight;	/* Minimum dimensions of window, in pixels or
				 * grid units. */
    int maxWidth, maxHeight;	/* Maximum dimensions of window, in pixels or
				 * grid units. 0 to default.*/
    Tk_Window gridWin;		/* Identifies the window that controls
				 * gridding for this top-level, or NULL if the
				 * top-level isn't currently gridded. */
    int widthInc, heightInc;	/* Increments for size changes (# pixels per
				 * step). */
    struct {
	int x;			/* numerator */
	int y;			/* denominator */
    } minAspect, maxAspect;	/* Min/max aspect ratios for window. */
    int reqGridWidth, reqGridHeight;
				/* The dimensions of the window (in grid
				 * units) requested through the geometry
				 * manager. */
    int gravity;		/* Desired window gravity. */

    /*
     * Information used to manage the size and location of a window.
     */

    int width, height;		/* Desired dimensions of window, specified in
				 * pixels or grid units. These values are set
				 * by the "wm geometry" command and by
				 * ConfigureNotify events (for when wm resizes
				 * window). -1 means user hasn't requested
				 * dimensions. */
    int x, y;			/* Desired X and Y coordinates for window.
				 * These values are set by "wm geometry", plus
				 * by ConfigureNotify events (when wm moves
				 * window). These numbers are different than
				 * the numbers stored in winPtr->changes
				 * because (a) they could be measured from the
				 * right or bottom edge of the screen (see
				 * WM_NEGATIVE_X and WM_NEGATIVE_Y flags) and
				 * (b) if the window has been reparented then
				 * they refer to the parent rather than the
				 * window itself. */
    int parentWidth, parentHeight;
				/* Width and height of reparent, in pixels
				 * *including border*. If window hasn't been
				 * reparented then these will be the outer
				 * dimensions of the window, including
				 * border. */
    int xInParent, yInParent;	/* Offset of wrapperPtr within reparent,
				 * measured in pixels from upper-left outer
				 * corner of reparent's border to upper-left
				 * outer corner of wrapperPtr's border. If not
				 * reparented then these are zero. */
    int configWidth, configHeight;
				/* Dimensions passed to last request that we
				 * issued to change geometry of the wrapper.
				 * Used to eliminate redundant resize
				 * operations. */

    /*
     * Information about the virtual root window for this top-level, if there
     * is one.
     */

    Window vRoot;		/* Virtual root window for this top-level, or
				 * None if there is no virtual root window
				 * (i.e. just use the screen's root). */
    int vRootX, vRootY;		/* Position of the virtual root inside the
				 * root window. If the WM_VROOT_OFFSET_STALE
				 * flag is set then this information may be
				 * incorrect and needs to be refreshed from
				 * the X server. If vRoot is None then these
				 * values are both 0. */
    int vRootWidth, vRootHeight;/* Dimensions of the virtual root window. If
				 * vRoot is None, gives the dimensions of the
				 * containing screen. This information is
				 * never stale, even though vRootX and vRootY
				 * can be. */

    /*
     * Miscellaneous information.
     */

    WmAttributes attributes;	/* Current state of [wm attributes] */
    WmAttributes reqState;	/* Requested state of [wm attributes] */
    ProtocolHandler *protPtr;	/* First in list of protocol handlers for this
				 * window (NULL means none). */
    int cmdArgc;		/* Number of elements in cmdArgv below. */
    const char **cmdArgv;	/* Array of strings to store in the WM_COMMAND
				 * property. NULL means nothing available. */
    char *clientMachine;	/* String to store in WM_CLIENT_MACHINE
				 * property, or NULL. */
    int flags;			/* Miscellaneous flags, defined below. */
    int numTransients;		/* number of transients on this window */
    int iconDataSize;		/* size of iconphoto image data */
    unsigned char *iconDataPtr;	/* iconphoto image data, if set */
    struct TkWmInfo *nextPtr;	/* Next in list of all top-level windows. */
} WmInfo;

/*
 * Flag values for WmInfo structures:
 *
 * WM_NEVER_MAPPED -		non-zero means window has never been mapped;
 *				need to update all info when window is first
 *				mapped.
 * WM_UPDATE_PENDING -		non-zero means a call to UpdateGeometryInfo
 *				has already been scheduled for this window;
 *				no need to schedule another one.
 * WM_NEGATIVE_X -		non-zero means x-coordinate is measured in
 *				pixels from right edge of screen, rather than
 *				from left edge.
 * WM_NEGATIVE_Y -		non-zero means y-coordinate is measured in
 *				pixels up from bottom of screen, rather than
 *				down from top.
 * WM_UPDATE_SIZE_HINTS -	non-zero means that new size hints need to be
 *				propagated to window manager.
 * WM_SYNC_PENDING -		set to non-zero while waiting for the window
 *				manager to respond to some state change.
 * WM_VROOT_OFFSET_STALE -	non-zero means that (x,y) offset information
 *				about the virtual root window is stale and
 *				needs to be fetched fresh from the X server.
 * WM_ABOUT_TO_MAP -		non-zero means that the window is about to be
 *				mapped by TkWmMapWindow. This is used by
 *				UpdateGeometryInfo to modify its behavior.
 * WM_MOVE_PENDING -		non-zero means the application has requested a
 *				new position for the window, but it hasn't
 *				been reflected through the window manager yet.
 * WM_COLORMAPS_EXPLICIT -	non-zero means the colormap windows were set
 *				explicitly via "wm colormapwindows".
 * WM_ADDED_TOPLEVEL_COLORMAP - non-zero means that when "wm colormapwindows"
 *				was called the top-level itself wasn't
 *				specified, so we added it implicitly at the
 *				end of the list.
 * WM_WIDTH_NOT_RESIZABLE -	non-zero means that we're not supposed to
 *				allow the user to change the width of the
 *				window (controlled by "wm resizable" command).
 * WM_HEIGHT_NOT_RESIZABLE -	non-zero means that we're not supposed to
 *				allow the user to change the height of the
 *				window (controlled by "wm resizable" command).
 * WM_WITHDRAWN -		non-zero means that this window has explicitly
 *				been withdrawn. If it's a transient, it should
 *				not mirror state changes in the master.
 */

#define WM_NEVER_MAPPED			1
#define WM_UPDATE_PENDING		2
#define WM_NEGATIVE_X			4
#define WM_NEGATIVE_Y			8
#define WM_UPDATE_SIZE_HINTS		0x10
#define WM_SYNC_PENDING			0x20
#define WM_VROOT_OFFSET_STALE		0x40
#define WM_ABOUT_TO_MAP			0x100
#define WM_MOVE_PENDING			0x200
#define WM_COLORMAPS_EXPLICIT		0x400
#define WM_ADDED_TOPLEVEL_COLORMAP	0x800
#define WM_WIDTH_NOT_RESIZABLE		0x1000
#define WM_HEIGHT_NOT_RESIZABLE		0x2000
#define WM_WITHDRAWN			0x4000

/*
 * Wrapper for XGetWindowProperty and XChangeProperty to make them a *bit*
 * less verbose.
 */

#define GetWindowProperty(wrapperPtr, atom, length, type, typePtr, formatPtr, numItemsPtr, bytesAfterPtr, itemsPtr) \
    (XGetWindowProperty((wrapperPtr)->display, (wrapperPtr)->window,	\
	    (atom), 0, (long) (length), False, (type),			\
	    (typePtr), (formatPtr), (numItemsPtr), (bytesAfterPtr),	\
	    (unsigned char **) (itemsPtr)) == Success)
#define SetWindowProperty(wrapperPtr, atomName, type, width, data, length) \
    XChangeProperty((wrapperPtr)->display, (wrapperPtr)->window,	\
	    Tk_InternAtom((Tk_Window) wrapperPtr, (atomName)),		\
	    (type), (width), PropModeReplace, (unsigned char *) (data), \
	    (int) (length))

/*
 * This module keeps a list of all top-level windows, primarily to simplify
 * the job of Tk_CoordsToWindow. The list is called firstWmPtr and is stored
 * in the TkDisplay structure.
 */

/*
 * The following structures are the official type records for geometry
 * management of top-level and menubar windows.
 */

static void		TopLevelReqProc(ClientData dummy, Tk_Window tkwin);
static void		RemapWindows(TkWindow *winPtr, TkWindow *parentPtr);
static void		MenubarReqProc(ClientData clientData,
			    Tk_Window tkwin);

static const Tk_GeomMgr wmMgrType = {
    "wm",			/* name */
    TopLevelReqProc,		/* requestProc */
    NULL,			/* lostSlaveProc */
};
static const Tk_GeomMgr menubarMgrType = {
    "menubar",			/* name */
    MenubarReqProc,		/* requestProc */
    NULL,			/* lostSlaveProc */
};

/*
 * Structures of the following type are used for communication between
 * WaitForEvent, WaitRestrictProc, and WaitTimeoutProc.
 */

typedef struct WaitRestrictInfo {
    Display *display;		/* Window belongs to this display. */
    WmInfo *wmInfoPtr;
    int type;			/* We only care about this type of event. */
    XEvent *eventPtr;		/* Where to store the event when it's found. */
    int foundEvent;		/* Non-zero means that an event of the desired
				 * type has been found. */
} WaitRestrictInfo;

/*
 * Forward declarations for functions defined in this file:
 */

static int		ComputeReparentGeometry(WmInfo *wmPtr);
static void		ConfigureEvent(WmInfo *wmPtr,
			    XConfigureEvent *eventPtr);
static void		CreateWrapper(WmInfo *wmPtr);
static void		GetMaxSize(WmInfo *wmPtr, int *maxWidthPtr,
			    int *maxHeightPtr);
static void		MenubarDestroyProc(ClientData clientData,
			    XEvent *eventPtr);
static int		ParseGeometry(Tcl_Interp *interp, const char *string,
			    TkWindow *winPtr);
static void		ReparentEvent(WmInfo *wmPtr, XReparentEvent *eventPtr);
static void		PropertyEvent(WmInfo *wmPtr, XPropertyEvent *eventPtr);
static void		TkWmStackorderToplevelWrapperMap(TkWindow *winPtr,
			    Display *display, Tcl_HashTable *reparentTable);
static void		TopLevelReqProc(ClientData dummy, Tk_Window tkwin);
static void		RemapWindows(TkWindow *winPtr, TkWindow *parentPtr);
static void		UpdateCommand(TkWindow *winPtr);
static void		UpdateGeometryInfo(ClientData clientData);
static void		UpdateHints(TkWindow *winPtr);
static void		UpdateSizeHints(TkWindow *winPtr,
			    int newWidth, int newHeight);
static void		UpdateTitle(TkWindow *winPtr);
static void		UpdatePhotoIcon(TkWindow *winPtr);
static void		UpdateVRootGeometry(WmInfo *wmPtr);
static void		UpdateWmProtocols(WmInfo *wmPtr);
static int		SetNetWmType(TkWindow *winPtr, Tcl_Obj *typePtr);
static Tcl_Obj *	GetNetWmType(TkWindow *winPtr);
static void 		SetNetWmState(TkWindow*, const char *atomName, int on);
static void 		CheckNetWmState(WmInfo *, Atom *atoms, int numAtoms);
static void 		UpdateNetWmState(WmInfo *);
static void		WaitForConfigureNotify(TkWindow *winPtr,
			    unsigned long serial);
static int		WaitForEvent(Display *display,
			    WmInfo *wmInfoPtr, int type, XEvent *eventPtr);
static void		WaitForMapNotify(TkWindow *winPtr, int mapped);
static Tk_RestrictProc WaitRestrictProc;
static void		WrapperEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		WmWaitMapProc(ClientData clientData,
			    XEvent *eventPtr);
static int		WmAspectCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmAttributesCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmClientCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmColormapwindowsCmd(Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmCommandCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmDeiconifyCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmFocusmodelCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmForgetCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmFrameCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmGeometryCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmGridCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmGroupCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmIconbitmapCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmIconifyCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmIconmaskCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmIconnameCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmIconphotoCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmIconpositionCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmIconwindowCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmManageCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmMaxsizeCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmMinsizeCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmOverrideredirectCmd(Tk_Window tkwin,
			    TkWindow *winPtr, Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmPositionfromCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmProtocolCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmResizableCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmSizefromCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmStackorderCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmStateCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmTitleCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmTransientCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		WmWithdrawCmd(Tk_Window tkwin, TkWindow *winPtr,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		WmUpdateGeom(WmInfo *wmPtr, TkWindow *winPtr);

/*
 *--------------------------------------------------------------
 *
 * TkWmCleanup --
 *
 *	This function is invoked to cleanup remaining wm resources associated
 *	with a display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All WmInfo structure resources are freed and invalidated.
 *
 *--------------------------------------------------------------
 */

void TkWmCleanup(
    TkDisplay *dispPtr)
{
    WmInfo *wmPtr, *nextPtr;

    for (wmPtr = dispPtr->firstWmPtr; wmPtr != NULL; wmPtr = nextPtr) {
	/*
	 * We can't assume we have access to winPtr's anymore, so some cleanup
	 * requiring winPtr data is avoided.
	 */

	nextPtr = wmPtr->nextPtr;
	if (wmPtr->title != NULL) {
	    ckfree(wmPtr->title);
	}
	if (wmPtr->iconName != NULL) {
	    ckfree(wmPtr->iconName);
	}
	if (wmPtr->iconDataPtr != NULL) {
	    ckfree(wmPtr->iconDataPtr);
	}
	if (wmPtr->leaderName != NULL) {
	    ckfree(wmPtr->leaderName);
	}
	if (wmPtr->menubar != NULL) {
	    Tk_DestroyWindow(wmPtr->menubar);
	}
	if (wmPtr->wrapperPtr != NULL) {
	    Tk_DestroyWindow((Tk_Window) wmPtr->wrapperPtr);
	}
	while (wmPtr->protPtr != NULL) {
	    ProtocolHandler *protPtr = wmPtr->protPtr;

	    wmPtr->protPtr = protPtr->nextPtr;
	    Tcl_EventuallyFree(protPtr, TCL_DYNAMIC);
	}
	if (wmPtr->cmdArgv != NULL) {
	    ckfree(wmPtr->cmdArgv);
	}
	if (wmPtr->clientMachine != NULL) {
	    ckfree(wmPtr->clientMachine);
	}
	ckfree(wmPtr);
    }
    if (dispPtr->iconDataPtr != NULL) {
	ckfree(dispPtr->iconDataPtr);
	dispPtr->iconDataPtr = NULL;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkWmNewWindow --
 *
 *	This function is invoked whenever a new top-level window is created.
 *	Its job is to initialize the WmInfo structure for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A WmInfo structure gets allocated and initialized.
 *
 *--------------------------------------------------------------
 */

void
TkWmNewWindow(
    TkWindow *winPtr)		/* Newly-created top-level window. */
{
    register WmInfo *wmPtr;
    TkDisplay *dispPtr = winPtr->dispPtr;

    wmPtr = ckalloc(sizeof(WmInfo));
    memset(wmPtr, 0, sizeof(WmInfo));
    wmPtr->winPtr = winPtr;
    wmPtr->reparent = None;
    wmPtr->masterPtr = NULL;
    wmPtr->numTransients = 0;
    wmPtr->hints.flags = InputHint | StateHint;
    wmPtr->hints.input = True;
    wmPtr->hints.initial_state = NormalState;
    wmPtr->hints.icon_pixmap = None;
    wmPtr->hints.icon_window = None;
    wmPtr->hints.icon_x = wmPtr->hints.icon_y = 0;
    wmPtr->hints.icon_mask = None;
    wmPtr->hints.window_group = None;

    /*
     * Initialize attributes.
     */

    wmPtr->attributes.alpha = 1.0;
    wmPtr->attributes.topmost = 0;
    wmPtr->attributes.zoomed = 0;
    wmPtr->attributes.fullscreen = 0;
    wmPtr->reqState = wmPtr->attributes;

    /*
     * Default the maximum dimensions to the size of the display, minus a
     * guess about how space is needed for window manager decorations.
     */

    wmPtr->gridWin = NULL;
    wmPtr->minWidth = wmPtr->minHeight = 1;
    wmPtr->maxWidth = wmPtr->maxHeight = 0;
    wmPtr->widthInc = wmPtr->heightInc = 1;
    wmPtr->minAspect.x = wmPtr->minAspect.y = 1;
    wmPtr->maxAspect.x = wmPtr->maxAspect.y = 1;
    wmPtr->reqGridWidth = wmPtr->reqGridHeight = -1;
    wmPtr->gravity = NorthWestGravity;
    wmPtr->width = -1;
    wmPtr->height = -1;
    wmPtr->x = winPtr->changes.x;
    wmPtr->y = winPtr->changes.y;
    wmPtr->parentWidth = winPtr->changes.width
	    + 2*winPtr->changes.border_width;
    wmPtr->parentHeight = winPtr->changes.height
	    + 2*winPtr->changes.border_width;
    wmPtr->configWidth = -1;
    wmPtr->configHeight = -1;
    wmPtr->vRoot = None;
    wmPtr->flags = WM_NEVER_MAPPED;
    wmPtr->nextPtr = (WmInfo *) dispPtr->firstWmPtr;
    dispPtr->firstWmPtr = wmPtr;
    winPtr->wmInfoPtr = wmPtr;

    UpdateVRootGeometry(wmPtr);

    /*
     * Arrange for geometry requests to be reflected from the window to the
     * window manager.
     */

    Tk_ManageGeometry((Tk_Window) winPtr, &wmMgrType, NULL);
}

/*
 *--------------------------------------------------------------
 *
 * TkWmMapWindow --
 *
 *	This function is invoked to map a top-level window. This module gets a
 *	chance to update all window-manager-related information in properties
 *	before the window manager sees the map event and checks the
 *	properties. It also gets to decide whether or not to even map the
 *	window after all.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Properties of winPtr may get updated to provide up-to-date information
 *	to the window manager. The window may also get mapped, but it may not
 *	be if this function decides that isn't appropriate (e.g. because the
 *	window is withdrawn).
 *
 *--------------------------------------------------------------
 */

void
TkWmMapWindow(
    TkWindow *winPtr)		/* Top-level window that's about to be
				 * mapped. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    XTextProperty textProp;

    if (wmPtr->flags & WM_NEVER_MAPPED) {
	Tcl_DString ds;

	wmPtr->flags &= ~WM_NEVER_MAPPED;

	/*
	 * This is the first time this window has ever been mapped. First
	 * create the wrapper window that provides space for a menubar.
	 */

	if (wmPtr->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr);
	}

	/*
	 * Store all the window-manager-related information for the window.
	 */

	TkWmSetClass(winPtr);
	UpdateTitle(winPtr);
	UpdatePhotoIcon(winPtr);

	if (wmPtr->masterPtr != NULL) {
	    /*
	     * Don't map a transient if the master is not mapped.
	     */

	    if (!Tk_IsMapped(wmPtr->masterPtr)) {
		wmPtr->withdrawn = 1;
		wmPtr->hints.initial_state = WithdrawnState;
	    }

	    /*
	     * Make sure that we actually set the transient-for property, even
	     * if we are withdrawn. [Bug 1163496]
	     */

	    XSetTransientForHint(winPtr->display, wmPtr->wrapperPtr->window,
		    wmPtr->masterPtr->wmInfoPtr->wrapperPtr->window);
	}

	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
	UpdateHints(winPtr);
	UpdateWmProtocols(wmPtr);
	if (wmPtr->cmdArgv != NULL) {
	    UpdateCommand(winPtr);
	}
	if (wmPtr->clientMachine != NULL) {
	    Tcl_UtfToExternalDString(NULL, wmPtr->clientMachine, -1, &ds);
	    if (XStringListToTextProperty(&(Tcl_DStringValue(&ds)), 1,
		    &textProp) != 0) {
		unsigned long pid = (unsigned long) getpid();

		XSetWMClientMachine(winPtr->display,
			wmPtr->wrapperPtr->window, &textProp);
		XFree((char *) textProp.value);

		/*
		 * Inform the server (and more particularly any session
		 * manager) what our process ID is. We only do this when the
		 * CLIENT_MACHINE property is set since the spec for
		 * _NET_WM_PID requires that to be set too.
		 */

		SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_PID",
			XA_CARDINAL, 32, &pid, 1);
	    }
	    Tcl_DStringFree(&ds);
	}
    }
    if (wmPtr->hints.initial_state == WithdrawnState) {
	return;
    }
    if (wmPtr->iconFor != NULL) {
	/*
	 * This window is an icon for somebody else. Make sure that the
	 * geometry is up-to-date, then return without mapping the window.
	 */

	if (wmPtr->flags & WM_UPDATE_PENDING) {
	    Tcl_CancelIdleCall(UpdateGeometryInfo, winPtr);
	}
	UpdateGeometryInfo(winPtr);
	return;
    }
    wmPtr->flags |= WM_ABOUT_TO_MAP;
    if (wmPtr->flags & WM_UPDATE_PENDING) {
	Tcl_CancelIdleCall(UpdateGeometryInfo, winPtr);
    }
    UpdateGeometryInfo(winPtr);
    wmPtr->flags &= ~WM_ABOUT_TO_MAP;

    /*
     * Update _NET_WM_STATE hints:
     */
    UpdateNetWmState(wmPtr);

    /*
     * Map the window, then wait to be sure that the window manager has
     * processed the map operation.
     */

    XMapWindow(winPtr->display, wmPtr->wrapperPtr->window);
    if (wmPtr->hints.initial_state == NormalState) {
	WaitForMapNotify(winPtr, 1);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkWmUnmapWindow --
 *
 *	This function is invoked to unmap a top-level window. The only thing
 *	it does special is to wait for the window actually to be unmapped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unmaps the window.
 *
 *--------------------------------------------------------------
 */

void
TkWmUnmapWindow(
    TkWindow *winPtr)		/* Top-level window that's about to be
				 * mapped. */
{
    /*
     * It seems to be important to wait after unmapping a top-level window
     * until the window really gets unmapped. I don't completely understand
     * all the interactions with the window manager, but if we go on without
     * waiting, and if the window is then mapped again quickly, events seem to
     * get lost so that we think the window isn't mapped when in fact it is
     * mapped. I suspect that this has something to do with the window manager
     * filtering Map events (and possily not filtering Unmap events?).
     */

    XUnmapWindow(winPtr->display, winPtr->wmInfoPtr->wrapperPtr->window);
    WaitForMapNotify(winPtr, 0);
}

/*
 *--------------------------------------------------------------
 *
 * TkWmDeadWindow --
 *
 *	This function is invoked when a top-level window is about to be
 *	deleted. It cleans up the wm-related data structures for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The WmInfo structure for winPtr gets freed up.
 *
 *--------------------------------------------------------------
 */

void
TkWmDeadWindow(
    TkWindow *winPtr)		/* Top-level window that's being deleted. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    WmInfo *wmPtr2;

    if (wmPtr == NULL) {
	return;
    }
    if ((WmInfo *) winPtr->dispPtr->firstWmPtr == wmPtr) {
	winPtr->dispPtr->firstWmPtr = wmPtr->nextPtr;
    } else {
	register WmInfo *prevPtr;

	for (prevPtr = (WmInfo *) winPtr->dispPtr->firstWmPtr; ;
		prevPtr = prevPtr->nextPtr) {
	    /* ASSERT: prevPtr != NULL [Bug 1789819] */
	    if (prevPtr->nextPtr == wmPtr) {
		prevPtr->nextPtr = wmPtr->nextPtr;
		break;
	    }
	}
    }
    if (wmPtr->title != NULL) {
	ckfree(wmPtr->title);
    }
    if (wmPtr->iconName != NULL) {
	ckfree(wmPtr->iconName);
    }
    if (wmPtr->iconDataPtr != NULL) {
	ckfree(wmPtr->iconDataPtr);
    }
    if (wmPtr->hints.flags & IconPixmapHint) {
	Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_pixmap);
    }
    if (wmPtr->hints.flags & IconMaskHint) {
	Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_mask);
    }
    if (wmPtr->leaderName != NULL) {
	ckfree(wmPtr->leaderName);
    }
    if (wmPtr->icon != NULL) {
	wmPtr2 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
	wmPtr2->iconFor = NULL;
	wmPtr2->withdrawn = 1;
    }
    if (wmPtr->iconFor != NULL) {
	wmPtr2 = ((TkWindow *) wmPtr->iconFor)->wmInfoPtr;
	wmPtr2->icon = NULL;
	wmPtr2->hints.flags &= ~IconWindowHint;
	UpdateHints((TkWindow *) wmPtr->iconFor);
    }
    if (wmPtr->menubar != NULL) {
	Tk_DestroyWindow(wmPtr->menubar);
    }
    if (wmPtr->wrapperPtr != NULL) {
	/*
	 * The rest of Tk doesn't know that we reparent the toplevel inside
	 * the wrapper, so reparent it back out again before deleting the
	 * wrapper; otherwise the toplevel will get deleted twice (once
	 * implicitly by the deletion of the wrapper).
	 */

	XUnmapWindow(winPtr->display, winPtr->window);
	XReparentWindow(winPtr->display, winPtr->window,
		XRootWindow(winPtr->display, winPtr->screenNum), 0, 0);
	Tk_DestroyWindow((Tk_Window) wmPtr->wrapperPtr);
    }
    while (wmPtr->protPtr != NULL) {
	ProtocolHandler *protPtr = wmPtr->protPtr;

	wmPtr->protPtr = protPtr->nextPtr;
	Tcl_EventuallyFree(protPtr, TCL_DYNAMIC);
    }
    if (wmPtr->cmdArgv != NULL) {
	ckfree(wmPtr->cmdArgv);
    }
    if (wmPtr->clientMachine != NULL) {
	ckfree(wmPtr->clientMachine);
    }
    if (wmPtr->flags & WM_UPDATE_PENDING) {
	Tcl_CancelIdleCall(UpdateGeometryInfo, winPtr);
    }

    /*
     * Reset all transient windows whose master is the dead window.
     */

    for (wmPtr2 = winPtr->dispPtr->firstWmPtr; wmPtr2 != NULL;
	    wmPtr2 = wmPtr2->nextPtr) {
	if (wmPtr2->masterPtr == winPtr) {
	    wmPtr->numTransients--;
	    Tk_DeleteEventHandler((Tk_Window) wmPtr2->masterPtr,
		    StructureNotifyMask, WmWaitMapProc, wmPtr2->winPtr);
	    wmPtr2->masterPtr = NULL;
	    if (!(wmPtr2->flags & WM_NEVER_MAPPED)) {
		XDeleteProperty(winPtr->display, wmPtr2->wrapperPtr->window,
			Tk_InternAtom((Tk_Window) winPtr, "WM_TRANSIENT_FOR"));

		/*
		 * FIXME: Need a call like Win32's UpdateWrapper() so we can
		 * recreate the wrapper and get rid of the transient window
		 * decorations.
		 */
	    }
	}
    }
    /* ASSERT: numTransients == 0 [Bug 1789819] */

    if (wmPtr->masterPtr != NULL) {
	wmPtr2 = wmPtr->masterPtr->wmInfoPtr;

	/*
	 * If we had a master, tell them that we aren't tied to them anymore
	 */

	if (wmPtr2 != NULL) {
	    wmPtr2->numTransients--;
	}
	Tk_DeleteEventHandler((Tk_Window) wmPtr->masterPtr,
		StructureNotifyMask, WmWaitMapProc, winPtr);
	wmPtr->masterPtr = NULL;
    }
    ckfree(wmPtr);
    winPtr->wmInfoPtr = NULL;
}

/*
 *--------------------------------------------------------------
 *
 * TkWmSetClass --
 *
 *	This function is invoked whenever a top-level window's class is
 *	changed. If the window has been mapped then this function updates the
 *	window manager property for the class. If the window hasn't been
 *	mapped, the update is deferred until just before the first mapping.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A window property may get updated.
 *
 *--------------------------------------------------------------
 */

void
TkWmSetClass(
    TkWindow *winPtr)		/* Newly-created top-level window. */
{
    if (winPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	return;
    }

    if (winPtr->classUid != NULL) {
	XClassHint *classPtr;
	Tcl_DString name, ds;

	Tcl_UtfToExternalDString(NULL, winPtr->nameUid, -1, &name);
	Tcl_UtfToExternalDString(NULL, winPtr->classUid, -1, &ds);
	classPtr = XAllocClassHint();
	classPtr->res_name = Tcl_DStringValue(&name);
	classPtr->res_class = Tcl_DStringValue(&ds);
	XSetClassHint(winPtr->display, winPtr->wmInfoPtr->wrapperPtr->window,
		classPtr);
	XFree((char *) classPtr);
	Tcl_DStringFree(&name);
	Tcl_DStringFree(&ds);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_WmObjCmd --
 *
 *	This function is invoked to process the "wm" Tcl command.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_WmObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    static const char *const optionStrings[] = {
	"aspect", "attributes", "client", "colormapwindows",
	"command", "deiconify", "focusmodel", "forget",
	"frame", "geometry", "grid", "group", "iconbitmap",
	"iconify", "iconmask", "iconname", "iconphoto",
	"iconposition", "iconwindow", "manage", "maxsize",
	"minsize", "overrideredirect", "positionfrom",
	"protocol", "resizable", "sizefrom", "stackorder",
	"state", "title", "transient", "withdraw", NULL };
    enum options {
	WMOPT_ASPECT, WMOPT_ATTRIBUTES, WMOPT_CLIENT, WMOPT_COLORMAPWINDOWS,
	WMOPT_COMMAND, WMOPT_DEICONIFY, WMOPT_FOCUSMODEL, WMOPT_FORGET,
	WMOPT_FRAME, WMOPT_GEOMETRY, WMOPT_GRID, WMOPT_GROUP,
	WMOPT_ICONBITMAP,
	WMOPT_ICONIFY, WMOPT_ICONMASK, WMOPT_ICONNAME, WMOPT_ICONPHOTO,
	WMOPT_ICONPOSITION, WMOPT_ICONWINDOW, WMOPT_MANAGE, WMOPT_MAXSIZE,
	WMOPT_MINSIZE, WMOPT_OVERRIDEREDIRECT, WMOPT_POSITIONFROM,
	WMOPT_PROTOCOL, WMOPT_RESIZABLE, WMOPT_SIZEFROM, WMOPT_STACKORDER,
	WMOPT_STATE, WMOPT_TITLE, WMOPT_TRANSIENT, WMOPT_WITHDRAW };
    int index, length;
    const char *argv1;
    TkWindow *winPtr;
    Tk_Window targetWin;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (objc < 2) {
    wrongNumArgs:
	Tcl_WrongNumArgs(interp, 1, objv, "option window ?arg ...?");
	return TCL_ERROR;
    }

    argv1 = Tcl_GetStringFromObj(objv[1], &length);
    if ((argv1[0] == 't') && (strncmp(argv1, "tracing", (size_t) length) == 0)
	    && (length >= 3)) {
	int wmTracing;

	if ((objc != 2) && (objc != 3)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?boolean?");
	    return TCL_ERROR;
	}
	if (objc == 2) {
	    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
		    dispPtr->flags & TK_DISPLAY_WM_TRACING));
	    return TCL_OK;
	}
	if (Tcl_GetBooleanFromObj(interp, objv[2], &wmTracing) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (wmTracing) {
	    dispPtr->flags |= TK_DISPLAY_WM_TRACING;
	} else {
	    dispPtr->flags &= ~TK_DISPLAY_WM_TRACING;
	}
	return TCL_OK;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], optionStrings,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc < 3) {
	goto wrongNumArgs;
    }

    if (TkGetWindowFromObj(interp, tkwin, objv[2], &targetWin) != TCL_OK) {
	return TCL_ERROR;
    }
    winPtr = (TkWindow *) targetWin;
    if (!Tk_IsTopLevel(winPtr) &&
	    (index != WMOPT_MANAGE) && (index != WMOPT_FORGET)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"window \"%s\" isn't a top-level window", winPtr->pathName));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "TOPLEVEL", winPtr->pathName,
		NULL);
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case WMOPT_ASPECT:
	return WmAspectCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ATTRIBUTES:
	return WmAttributesCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_CLIENT:
	return WmClientCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_COLORMAPWINDOWS:
	return WmColormapwindowsCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_COMMAND:
	return WmCommandCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_DEICONIFY:
	return WmDeiconifyCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_FOCUSMODEL:
	return WmFocusmodelCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_FORGET:
	return WmForgetCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_FRAME:
	return WmFrameCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_GEOMETRY:
	return WmGeometryCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_GRID:
	return WmGridCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_GROUP:
	return WmGroupCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ICONBITMAP:
	return WmIconbitmapCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ICONIFY:
	return WmIconifyCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ICONMASK:
	return WmIconmaskCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ICONNAME:
	return WmIconnameCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ICONPHOTO:
	return WmIconphotoCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ICONPOSITION:
	return WmIconpositionCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_ICONWINDOW:
	return WmIconwindowCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_MANAGE:
	return WmManageCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_MAXSIZE:
	return WmMaxsizeCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_MINSIZE:
	return WmMinsizeCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_OVERRIDEREDIRECT:
	return WmOverrideredirectCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_POSITIONFROM:
	return WmPositionfromCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_PROTOCOL:
	return WmProtocolCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_RESIZABLE:
	return WmResizableCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_SIZEFROM:
	return WmSizefromCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_STACKORDER:
	return WmStackorderCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_STATE:
	return WmStateCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_TITLE:
	return WmTitleCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_TRANSIENT:
	return WmTransientCmd(tkwin, winPtr, interp, objc, objv);
    case WMOPT_WITHDRAW:
	return WmWithdrawCmd(tkwin, winPtr, interp, objc, objv);
    }

    /* This should not happen */
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * WmAspectCmd --
 *
 *	This function is invoked to process the "wm aspect" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmAspectCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int numer1, denom1, numer2, denom2;

    if ((objc != 3) && (objc != 7)) {
	Tcl_WrongNumArgs(interp, 2, objv,
		"window ?minNumer minDenom maxNumer maxDenom?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->sizeHintsFlags & PAspect) {
	    Tcl_Obj *results[4];

	    results[0] = Tcl_NewIntObj(wmPtr->minAspect.x);
	    results[1] = Tcl_NewIntObj(wmPtr->minAspect.y);
	    results[2] = Tcl_NewIntObj(wmPtr->maxAspect.x);
	    results[3] = Tcl_NewIntObj(wmPtr->maxAspect.y);
	    Tcl_SetObjResult(interp, Tcl_NewListObj(4, results));
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->sizeHintsFlags &= ~PAspect;
    } else {
	if ((Tcl_GetIntFromObj(interp, objv[3], &numer1) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &denom1) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[5], &numer2) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[6], &denom2) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if ((numer1 <= 0) || (denom1 <= 0) || (numer2 <= 0) ||
		(denom2 <= 0)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "aspect number can't be <= 0", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "ASPECT", NULL);
	    return TCL_ERROR;
	}
	wmPtr->minAspect.x = numer1;
	wmPtr->minAspect.y = denom1;
	wmPtr->maxAspect.x = numer2;
	wmPtr->maxAspect.y = denom2;
	wmPtr->sizeHintsFlags |= PAspect;
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmSetAttribute --
 *
 * 	Helper routine for WmAttributesCmd. Sets the value of the specified
 * 	attribute.
 *
 * Returns:
 *
 * 	TCL_OK if successful, TCL_ERROR otherwise. In case of an error, leaves
 * 	a message in the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
WmSetAttribute(
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter */
    WmAttribute attribute,	/* Code of attribute to set */
    Tcl_Obj *value)		/* New value */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    switch (attribute) {
    case WMATT_ALPHA: {
	unsigned long opacity;	/* 0=transparent, 0xFFFFFFFF=opaque */

	if (TCL_OK != Tcl_GetDoubleFromObj(interp, value,
		&wmPtr->reqState.alpha)) {
	    return TCL_ERROR;
	}
	if (wmPtr->reqState.alpha < 0.0) {
	    wmPtr->reqState.alpha = 0.0;
	}
	if (wmPtr->reqState.alpha > 1.0) {
	    wmPtr->reqState.alpha = 1.0;
	}

	if (!wmPtr->wrapperPtr) {
	    break;
	}

	opacity = 0xFFFFFFFFul * wmPtr->reqState.alpha;
	SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_WINDOW_OPACITY",
		XA_CARDINAL, 32, &opacity, 1L);
	wmPtr->attributes.alpha = wmPtr->reqState.alpha;

	break;
    }
    case WMATT_TOPMOST:
	if (Tcl_GetBooleanFromObj(interp, value,
		&wmPtr->reqState.topmost) != TCL_OK) {
	    return TCL_ERROR;
	}
	SetNetWmState(winPtr, "_NET_WM_STATE_ABOVE", wmPtr->reqState.topmost);
	break;
    case WMATT_TYPE:
	if (TCL_OK != SetNetWmType(winPtr, value))
	    return TCL_ERROR;
	break;
    case WMATT_ZOOMED:
	if (Tcl_GetBooleanFromObj(interp, value,
		&wmPtr->reqState.zoomed) != TCL_OK) {
	    return TCL_ERROR;
	}
	SetNetWmState(winPtr, "_NET_WM_STATE_MAXIMIZED_VERT",
		wmPtr->reqState.zoomed);
	SetNetWmState(winPtr, "_NET_WM_STATE_MAXIMIZED_HORZ",
		wmPtr->reqState.zoomed);
	break;
    case WMATT_FULLSCREEN:
	if (Tcl_GetBooleanFromObj(interp, value,
		&wmPtr->reqState.fullscreen) != TCL_OK) {
	    return TCL_ERROR;
	}
	SetNetWmState(winPtr, "_NET_WM_STATE_FULLSCREEN",
		wmPtr->reqState.fullscreen);
	break;
    case _WMATT_LAST_ATTRIBUTE:	/* NOTREACHED */
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmGetAttribute --
 *
 * 	Helper routine for WmAttributesCmd. Returns the current value of the
 * 	specified attribute.
 *
 * See also: CheckNetWmState().
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
WmGetAttribute(
    TkWindow *winPtr,		/* Toplevel to work with */
    WmAttribute attribute)	/* Code of attribute to get */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    switch (attribute) {
    case WMATT_ALPHA:
	return Tcl_NewDoubleObj(wmPtr->attributes.alpha);
    case WMATT_TOPMOST:
	return Tcl_NewBooleanObj(wmPtr->attributes.topmost);
    case WMATT_ZOOMED:
	return Tcl_NewBooleanObj(wmPtr->attributes.zoomed);
    case WMATT_FULLSCREEN:
	return Tcl_NewBooleanObj(wmPtr->attributes.fullscreen);
    case WMATT_TYPE:
	return GetNetWmType(winPtr);
    case _WMATT_LAST_ATTRIBUTE:	/*NOTREACHED*/
	break;
    }
    /*NOTREACHED*/
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * WmAttributesCmd --
 *
 *	This function is invoked to process the "wm attributes" Tcl command.
 *
 * Syntax:
 *
 * 	wm attributes $win ?-attribute ?value attribute value...??
 *
 * Notes:
 *
 *	Attributes of mapped windows are set by sending a _NET_WM_STATE
 *	ClientMessage to the root window (see SetNetWmState). For withdrawn
 *	windows, we keep track of the requested attribute state, and set the
 *	_NET_WM_STATE property ourselves immediately prior to mapping the
 *	window.
 *
 * See also: TIP#231, EWMH.
 *
 *----------------------------------------------------------------------
 */

static int
WmAttributesCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int attribute = 0;

    if (objc == 3) {		/* wm attributes $win */
	Tcl_Obj *result = Tcl_NewListObj(0,0);

	for (attribute = 0; attribute < _WMATT_LAST_ATTRIBUTE; ++attribute) {
	    Tcl_ListObjAppendElement(interp, result,
		    Tcl_NewStringObj(WmAttributeNames[attribute], -1));
	    Tcl_ListObjAppendElement(interp, result,
		    WmGetAttribute(winPtr, attribute));
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
    } else if (objc == 4) {	/* wm attributes $win -attribute */
	if (Tcl_GetIndexFromObjStruct(interp, objv[3], WmAttributeNames,
		sizeof(char *), "attribute", 0, &attribute) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, WmGetAttribute(winPtr, attribute));
	return TCL_OK;
    } else if ((objc - 3) % 2 == 0) {	/* wm attributes $win -att value... */
	int i;

	for (i = 3; i < objc; i += 2) {
	    if (Tcl_GetIndexFromObjStruct(interp, objv[i], WmAttributeNames,
		    sizeof(char *), "attribute", 0, &attribute) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (WmSetAttribute(winPtr,interp,attribute,objv[i+1]) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	return TCL_OK;
    }

    Tcl_WrongNumArgs(interp, 2, objv, "window ?-attribute ?value ...??");
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * WmClientCmd --
 *
 *	This function is invoked to process the "wm client" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmClientCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    const char *argv3;
    int length;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?name?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->clientMachine != NULL) {
	    Tcl_SetObjResult(interp,
		    Tcl_NewStringObj(wmPtr->clientMachine, -1));
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetStringFromObj(objv[3], &length);
    if (argv3[0] == 0) {
	if (wmPtr->clientMachine != NULL) {
	    ckfree(wmPtr->clientMachine);
	    wmPtr->clientMachine = NULL;
	    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
		XDeleteProperty(winPtr->display, wmPtr->wrapperPtr->window,
			Tk_InternAtom((Tk_Window) winPtr,
				"WM_CLIENT_MACHINE"));
	    }
	}
	return TCL_OK;
    }
    if (wmPtr->clientMachine != NULL) {
	ckfree(wmPtr->clientMachine);
    }
    wmPtr->clientMachine = ckalloc(length + 1);
    strcpy(wmPtr->clientMachine, argv3);
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	XTextProperty textProp;
	Tcl_DString ds;

	Tcl_UtfToExternalDString(NULL, wmPtr->clientMachine, -1, &ds);
	if (XStringListToTextProperty(&(Tcl_DStringValue(&ds)), 1,
		&textProp) != 0) {
	    unsigned long pid = (unsigned long) getpid();

	    XSetWMClientMachine(winPtr->display, wmPtr->wrapperPtr->window,
		    &textProp);
	    XFree((char *) textProp.value);

	    /*
	     * Inform the server (and more particularly any session manager)
	     * what our process ID is. We only do this when the CLIENT_MACHINE
	     * property is set since the spec for _NET_WM_PID requires that to
	     * be set too.
	     */

	    SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_PID", XA_CARDINAL,
		    32, &pid, 1);
	}
	Tcl_DStringFree(&ds);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmColormapwindowsCmd --
 *
 *	This function is invoked to process the "wm colormapwindows" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmColormapwindowsCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Window *cmapList;
    TkWindow *winPtr2;
    int count, i, windowObjc, gotToplevel;
    Tcl_Obj **windowObjv, *resultObj;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?windowList?");
	return TCL_ERROR;
    }
    Tk_MakeWindowExist((Tk_Window) winPtr);
    if (wmPtr->wrapperPtr == NULL) {
	CreateWrapper(wmPtr);
    }
    if (objc == 3) {
	if (XGetWMColormapWindows(winPtr->display,
		wmPtr->wrapperPtr->window, &cmapList, &count) == 0) {
	    return TCL_OK;
	}
	resultObj = Tcl_NewObj();
	for (i = 0; i < count; i++) {
	    if ((i == (count-1))
		    && (wmPtr->flags & WM_ADDED_TOPLEVEL_COLORMAP)) {
		break;
	    }
	    winPtr2 = (TkWindow *)
		   Tk_IdToWindow(winPtr->display, cmapList[i]);
	    if (winPtr2 == NULL) {
		Tcl_ListObjAppendElement(NULL, resultObj,
			Tcl_ObjPrintf("0x%lx", cmapList[i]));
	    } else {
		Tcl_ListObjAppendElement(NULL, resultObj,
			Tcl_NewStringObj(winPtr2->pathName, -1));
	    }
	}
	XFree((char *) cmapList);
	Tcl_SetObjResult(interp, resultObj);
	return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objv[3], &windowObjc, &windowObjv)
	    != TCL_OK) {
	return TCL_ERROR;
    }
    cmapList = ckalloc((windowObjc+1) * sizeof(Window));
    gotToplevel = 0;
    for (i = 0; i < windowObjc; i++) {
	Tk_Window mapWin;

	if (TkGetWindowFromObj(interp, tkwin, windowObjv[i],
		&mapWin) != TCL_OK) {
	    ckfree(cmapList);
	    return TCL_ERROR;
	}
	winPtr2 = (TkWindow *) mapWin;
	if (winPtr2 == winPtr) {
	    gotToplevel = 1;
	}
	if (winPtr2->window == None) {
	    Tk_MakeWindowExist((Tk_Window) winPtr2);
	}
	cmapList[i] = winPtr2->window;
    }
    if (!gotToplevel) {
	wmPtr->flags |= WM_ADDED_TOPLEVEL_COLORMAP;
	cmapList[windowObjc] = wmPtr->wrapperPtr->window;
	windowObjc++;
    } else {
	wmPtr->flags &= ~WM_ADDED_TOPLEVEL_COLORMAP;
    }
    wmPtr->flags |= WM_COLORMAPS_EXPLICIT;
    XSetWMColormapWindows(winPtr->display, wmPtr->wrapperPtr->window,
	    cmapList, windowObjc);
    ckfree(cmapList);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmCommandCmd --
 *
 *	This function is invoked to process the "wm command" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmCommandCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    const char *argv3;
    int cmdArgc;
    const char **cmdArgv;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?value?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->cmdArgv != NULL) {
	    char *arg = Tcl_Merge(wmPtr->cmdArgc, wmPtr->cmdArgv);

	    Tcl_SetObjResult(interp, Tcl_NewStringObj(arg, -1));
	    ckfree(arg);
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetString(objv[3]);
    if (argv3[0] == 0) {
	if (wmPtr->cmdArgv != NULL) {
	    ckfree(wmPtr->cmdArgv);
	    wmPtr->cmdArgv = NULL;
	    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
		XDeleteProperty(winPtr->display, wmPtr->wrapperPtr->window,
			Tk_InternAtom((Tk_Window) winPtr, "WM_COMMAND"));
	    }
	}
	return TCL_OK;
    }
    if (Tcl_SplitList(interp, argv3, &cmdArgc, &cmdArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (wmPtr->cmdArgv != NULL) {
	ckfree(wmPtr->cmdArgv);
    }
    wmPtr->cmdArgc = cmdArgc;
    wmPtr->cmdArgv = cmdArgv;
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	UpdateCommand(winPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmDeiconifyCmd --
 *
 *	This function is invoked to process the "wm deiconify" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmDeiconifyCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    if (wmPtr->iconFor != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't deiconify %s: it is an icon for %s",
		Tcl_GetString(objv[2]), Tk_PathName(wmPtr->iconFor)));
	Tcl_SetErrorCode(interp, "TK", "WM", "DEICONIFY", "ICON", NULL);
	return TCL_ERROR;
    }
    if (winPtr->flags & TK_EMBEDDED) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't deiconify %s: it is an embedded window",
		winPtr->pathName));
	Tcl_SetErrorCode(interp, "TK", "WM", "DEICONIFY", "EMBEDDED", NULL);
	return TCL_ERROR;
    }
    wmPtr->flags &= ~WM_WITHDRAWN;
    TkpWmSetState(winPtr, NormalState);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmFocusmodelCmd --
 *
 *	This function is invoked to process the "wm focusmodel" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmFocusmodelCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static const char *const optionStrings[] = {
	"active", "passive", NULL };
    enum options {
	OPT_ACTIVE, OPT_PASSIVE };
    int index;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?active|passive?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		wmPtr->hints.input ? "passive" : "active", -1));
	return TCL_OK;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[3], optionStrings,
	    sizeof(char *), "argument", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index == OPT_ACTIVE) {
	wmPtr->hints.input = False;
    } else { /* OPT_PASSIVE */
	wmPtr->hints.input = True;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmForgetCmd --
 *
 *	This procedure is invoked to process the "wm forget" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmForgetCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel or Frame to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register Tk_Window frameWin = (Tk_Window) winPtr;

    if (Tk_IsTopLevel(frameWin)) {
	TkFocusJoin(winPtr);
	Tk_UnmapWindow(frameWin);
	TkWmDeadWindow(winPtr);
	winPtr->flags &=
		~(TK_TOP_HIERARCHY|TK_TOP_LEVEL|TK_HAS_WRAPPER|TK_WIN_MANAGED);
	RemapWindows(winPtr, winPtr->parentPtr);

        /*
         * Make sure wm no longer manages this window
         */
        Tk_ManageGeometry(frameWin, NULL, NULL);

	/*
	 * Flags (above) must be cleared before calling TkMapTopFrame (below).
	 */

	TkMapTopFrame(frameWin);
    } else {
	/*
	 * Already not managed by wm - ignore it.
	 */
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmFrameCmd --
 *
 *	This function is invoked to process the "wm frame" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmFrameCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Window window;
    char buf[TCL_INTEGER_SPACE];

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    window = wmPtr->reparent;
    if (window == None) {
	window = Tk_WindowId((Tk_Window) winPtr);
    }
    sprintf(buf, "0x%" TCL_Z_MODIFIER "x", (size_t)window);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, -1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmGeometryCmd --
 *
 *	This function is invoked to process the "wm geometry" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmGeometryCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    char xSign, ySign;
    int width, height;
    const char *argv3;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?newGeometry?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	xSign = (wmPtr->flags & WM_NEGATIVE_X) ? '-' : '+';
	ySign = (wmPtr->flags & WM_NEGATIVE_Y) ? '-' : '+';
	if (wmPtr->gridWin != NULL) {
	    width = wmPtr->reqGridWidth + (winPtr->changes.width
		    - winPtr->reqWidth)/wmPtr->widthInc;
	    height = wmPtr->reqGridHeight + (winPtr->changes.height
		    - winPtr->reqHeight)/wmPtr->heightInc;
	} else {
	    width = winPtr->changes.width;
	    height = winPtr->changes.height;
	}
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("%dx%d%c%d%c%d",
		width, height, xSign, wmPtr->x, ySign, wmPtr->y));
	return TCL_OK;
    }
    argv3 = Tcl_GetString(objv[3]);
    if (*argv3 == '\0') {
	wmPtr->width = -1;
	wmPtr->height = -1;
	WmUpdateGeom(wmPtr, winPtr);
	return TCL_OK;
    }
    return ParseGeometry(interp, argv3, winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * WmGridCmd --
 *
 *	This function is invoked to process the "wm grid" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmGridCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int reqWidth, reqHeight, widthInc, heightInc;

    if ((objc != 3) && (objc != 7)) {
	Tcl_WrongNumArgs(interp, 2, objv,
		"window ?baseWidth baseHeight widthInc heightInc?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->sizeHintsFlags & PBaseSize) {
	    Tcl_Obj *results[4];

	    results[0] = Tcl_NewIntObj(wmPtr->reqGridWidth);
	    results[1] = Tcl_NewIntObj(wmPtr->reqGridHeight);
	    results[2] = Tcl_NewIntObj(wmPtr->widthInc);
	    results[3] = Tcl_NewIntObj(wmPtr->heightInc);
	    Tcl_SetObjResult(interp, Tcl_NewListObj(4, results));
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	/*
	 * Turn off gridding and reset the width and height to make sense as
	 * ungridded numbers.
	 */

	wmPtr->sizeHintsFlags &= ~(PBaseSize|PResizeInc);
	if (wmPtr->width != -1) {
	    wmPtr->width = winPtr->reqWidth + (wmPtr->width
		    - wmPtr->reqGridWidth)*wmPtr->widthInc;
	    wmPtr->height = winPtr->reqHeight + (wmPtr->height
		    - wmPtr->reqGridHeight)*wmPtr->heightInc;
	}
	wmPtr->widthInc = 1;
	wmPtr->heightInc = 1;
    } else {
	if ((Tcl_GetIntFromObj(interp, objv[3], &reqWidth) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &reqHeight) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[5], &widthInc) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[6], &heightInc) !=TCL_OK)) {
	    return TCL_ERROR;
	}
	if (reqWidth < 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "baseWidth can't be < 0", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "GRID", NULL);
	    return TCL_ERROR;
	}
	if (reqHeight < 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "baseHeight can't be < 0", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "GRID", NULL);
	    return TCL_ERROR;
	}
	if (widthInc <= 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "widthInc can't be <= 0", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "GRID", NULL);
	    return TCL_ERROR;
	}
	if (heightInc <= 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "heightInc can't be <= 0", -1));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "GRID", NULL);
	    return TCL_ERROR;
	}
	Tk_SetGrid((Tk_Window) winPtr, reqWidth, reqHeight, widthInc,
		heightInc);
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmGroupCmd --
 *
 *	This function is invoked to process the "wm group" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmGroupCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Tk_Window tkwin2;
    WmInfo *wmPtr2;
    const char *argv3;
    int length;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?pathName?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & WindowGroupHint) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(wmPtr->leaderName, -1));
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetStringFromObj(objv[3], &length);
    if (*argv3 == '\0') {
	wmPtr->hints.flags &= ~WindowGroupHint;
	if (wmPtr->leaderName != NULL) {
	    ckfree(wmPtr->leaderName);
	}
	wmPtr->leaderName = NULL;
    } else {
	if (TkGetWindowFromObj(interp, tkwin, objv[3], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	while (!Tk_TopWinHierarchy(tkwin2)) {
	    /*
	     * Ensure that the group leader is actually a Tk toplevel.
	     */

	    tkwin2 = Tk_Parent(tkwin2);
	}
	Tk_MakeWindowExist(tkwin2);
	wmPtr2 = ((TkWindow *) tkwin2)->wmInfoPtr;
	if (wmPtr2->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr2);
	}
	if (wmPtr->leaderName != NULL) {
	    ckfree(wmPtr->leaderName);
	}
	wmPtr->hints.window_group = Tk_WindowId(wmPtr2->wrapperPtr);
	wmPtr->hints.flags |= WindowGroupHint;
	wmPtr->leaderName = ckalloc(length + 1);
	strcpy(wmPtr->leaderName, argv3);
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconbitmapCmd --
 *
 *	This function is invoked to process the "wm iconbitmap" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmIconbitmapCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Pixmap pixmap;
    const char *argv3;

    if ((objc < 3) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?bitmap?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & IconPixmapHint) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    Tk_NameOfBitmap(winPtr->display,
			    wmPtr->hints.icon_pixmap), -1));
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetString(objv[3]);
    if (*argv3 == '\0') {
	if (wmPtr->hints.icon_pixmap != None) {
	    Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_pixmap);
	    wmPtr->hints.icon_pixmap = None;
	}
	wmPtr->hints.flags &= ~IconPixmapHint;
    } else {
	pixmap = Tk_GetBitmap(interp, (Tk_Window) winPtr, argv3);
	if (pixmap == None) {
	    return TCL_ERROR;
	}
	wmPtr->hints.icon_pixmap = pixmap;
	wmPtr->hints.flags |= IconPixmapHint;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconifyCmd --
 *
 *	This function is invoked to process the "wm iconify" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmIconifyCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    if (Tk_Attributes((Tk_Window) winPtr)->override_redirect) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't iconify \"%s\": override-redirect flag is set",
		winPtr->pathName));
	Tcl_SetErrorCode(interp, "TK", "WM", "ICONIFY", "OVERRIDE_REDIRECT",
		NULL);
	return TCL_ERROR;
    }
    if (wmPtr->masterPtr != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't iconify \"%s\": it is a transient",
		winPtr->pathName));
	Tcl_SetErrorCode(interp, "TK", "WM", "ICONIFY", "TRANSIENT", NULL);
	return TCL_ERROR;
    }
    if (wmPtr->iconFor != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't iconify %s: it is an icon for %s",
		winPtr->pathName, Tk_PathName(wmPtr->iconFor)));
	Tcl_SetErrorCode(interp, "TK", "WM", "ICONIFY", "ICON", NULL);
	return TCL_ERROR;
    }
    if (winPtr->flags & TK_EMBEDDED) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't iconify %s: it is an embedded window",
		winPtr->pathName));
	Tcl_SetErrorCode(interp, "TK", "WM", "ICONIFY", "EMBEDDED", NULL);
	return TCL_ERROR;
    }
    if (TkpWmSetState(winPtr, IconicState) == 0) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"couldn't send iconify message to window manager", -1));
	Tcl_SetErrorCode(interp, "TK", "WM", "COMMUNICATION", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconmaskCmd --
 *
 *	This function is invoked to process the "wm iconmask" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmIconmaskCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Pixmap pixmap;
    const char *argv3;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?bitmap?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & IconMaskHint) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    Tk_NameOfBitmap(winPtr->display, wmPtr->hints.icon_mask),
		    -1));
	}
	return TCL_OK;
    }
    argv3 = Tcl_GetString(objv[3]);
    if (*argv3 == '\0') {
	if (wmPtr->hints.icon_mask != None) {
	    Tk_FreeBitmap(winPtr->display, wmPtr->hints.icon_mask);
	}
	wmPtr->hints.flags &= ~IconMaskHint;
    } else {
	pixmap = Tk_GetBitmap(interp, tkwin, argv3);
	if (pixmap == None) {
	    return TCL_ERROR;
	}
	wmPtr->hints.icon_mask = pixmap;
	wmPtr->hints.flags |= IconMaskHint;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconnameCmd --
 *
 *	This function is invoked to process the "wm iconname" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmIconnameCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    const char *argv3;
    int length;

    if (objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?newName?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->iconName != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(wmPtr->iconName, -1));
	}
	return TCL_OK;
    } else {
	if (wmPtr->iconName != NULL) {
	    ckfree(wmPtr->iconName);
	}
	argv3 = Tcl_GetStringFromObj(objv[3], &length);
	wmPtr->iconName = ckalloc(length + 1);
	strcpy(wmPtr->iconName, argv3);
	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    UpdateTitle(winPtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconphotoCmd --
 *
 *	This function is invoked to process the "wm iconphoto" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmIconphotoCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Tk_PhotoHandle photo;
    Tk_PhotoImageBlock block;
    int i, size = 0, width, height, index = 0, x, y, isDefault = 0;
    unsigned long *iconPropertyData;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
		"window ?-default? image1 ?image2 ...?");
	return TCL_ERROR;
    }
    if (strcmp(Tcl_GetString(objv[3]), "-default") == 0) {
	isDefault = 1;
	if (objc == 4) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "window ?-default? image1 ?image2 ...?");
	    return TCL_ERROR;
	}
    }

    /*
     * Iterate over all images to retrieve their sizes, in order to allocate a
     * buffer large enough to hold all images.
     */

    for (i = 3 + isDefault; i < objc; i++) {
	photo = Tk_FindPhoto(interp, Tcl_GetString(objv[i]));
	if (photo == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't use \"%s\" as iconphoto: not a photo image",
		    Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "WM", "ICONPHOTO", "PHOTO", NULL);
	    return TCL_ERROR;
	}
	Tk_PhotoGetSize(photo, &width, &height);

	/*
	 * We need to cardinals for width & height and one cardinal for each
	 * image pixel.
	 */

	size += 2 + width * height;
    }

    /*
     * We have calculated the size of the data. Try to allocate the needed
     * memory space. This is an unsigned long array (despite this being twice
     * as much as is really needed on LP64 platforms) because that's what X
     * defines CARD32 arrays to use. [Bug 2902814]
     */

    iconPropertyData = attemptckalloc(sizeof(unsigned long) * size);
    if (iconPropertyData == NULL) {
	return TCL_ERROR;
    }
    memset(iconPropertyData, 0, sizeof(unsigned long) * size);

    for (i = 3 + isDefault; i < objc; i++) {
	photo = Tk_FindPhoto(interp, Tcl_GetString(objv[i]));
	if (photo == NULL) {
	    ckfree((char *) iconPropertyData);
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	        "failed to create an iconphoto with image \"%s\"",
		Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "WM", "ICONPHOTO", "IMAGE", NULL);
	    return TCL_ERROR;
	}
	Tk_PhotoGetSize(photo, &width, &height);
	Tk_PhotoGetImage(photo, &block);

	/*
	 * Each image data will be placed as an array of 32bit packed
	 * CARDINAL, in a window property named "_NET_WM_ICON": _NET_WM_ICON
	 *
	 * _NET_WM_ICON CARDINAL[][2+n]/32
	 *
	 * This is an array of possible icons for the client. This spec. does
	 * not stipulate what size these icons should be, but individual
	 * desktop environments or toolkits may do so. The Window Manager MAY
	 * scale any of these icons to an appropriate size.
	 *
	 * This is an array of 32bit packed CARDINAL ARGB with high byte being
	 * A, low byte being B. The first two cardinals are width, height.
	 * Data is in rows, left to right and top to bottom. The data will be
	 * endian-swapped going to the server if necessary. [Bug 2830420]
	 *
	 * The image data will be encoded in the iconPropertyData array.
	 */

	iconPropertyData[index++] = (unsigned long) width;
	iconPropertyData[index++] = (unsigned long) height;
	for (y = 0; y < height; y++) {
	    for (x = 0; x < width; x++) {
		register unsigned char *pixelPtr =
			block.pixelPtr + x*block.pixelSize + y*block.pitch;
		register unsigned long R, G, B, A;

		R = pixelPtr[block.offset[0]];
		G = pixelPtr[block.offset[1]];
		B = pixelPtr[block.offset[2]];
		A = pixelPtr[block.offset[3]];
		iconPropertyData[index++] = A<<24 | R<<16 | G<<8 | B<<0;
	    }
	}
    }
    if (wmPtr->iconDataPtr != NULL) {
	ckfree(wmPtr->iconDataPtr);
	wmPtr->iconDataPtr = NULL;
    }
    if (isDefault) {
	if (winPtr->dispPtr->iconDataPtr != NULL) {
	    ckfree(winPtr->dispPtr->iconDataPtr);
	}
	winPtr->dispPtr->iconDataPtr = (unsigned char *) iconPropertyData;
	winPtr->dispPtr->iconDataSize = size;
    } else {
	wmPtr->iconDataPtr = (unsigned char *) iconPropertyData;
	wmPtr->iconDataSize = size;
    }
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	UpdatePhotoIcon(winPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconpositionCmd --
 *
 *	This function is invoked to process the "wm iconposition" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmIconpositionCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int x, y;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?x y?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->hints.flags & IconPositionHint) {
	    Tcl_Obj *results[2];

	    results[0] = Tcl_NewIntObj(wmPtr->hints.icon_x);
	    results[1] = Tcl_NewIntObj(wmPtr->hints.icon_y);
	    Tcl_SetObjResult(interp, Tcl_NewListObj(2, results));
	}
	return TCL_OK;
    }
    if (Tcl_GetString(objv[3])[0] == '\0') {
	wmPtr->hints.flags &= ~IconPositionHint;
    } else {
	if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	    return TCL_ERROR;
	}
	wmPtr->hints.icon_x = x;
	wmPtr->hints.icon_y = y;
	wmPtr->hints.flags |= IconPositionHint;
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmIconwindowCmd --
 *
 *	This function is invoked to process the "wm iconwindow" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmIconwindowCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Tk_Window tkwin2;
    WmInfo *wmPtr2;
    XSetWindowAttributes atts;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?pathName?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->icon != NULL) {
	    Tcl_SetObjResult(interp, TkNewWindowObj(wmPtr->icon));
	}
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->hints.flags &= ~IconWindowHint;
	if (wmPtr->icon != NULL) {
	    /*
	     * Remove the icon window relationship. In principle we should
	     * also re-enable button events for the window, but this doesn't
	     * work in general because the window manager is probably
	     * selecting on them (we'll get an error if we try to re-enable
	     * the events). So, just leave the icon window event-challenged;
	     * the user will have to recreate it if they want button events.
	     */

	    wmPtr2 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;
	    wmPtr2->iconFor = NULL;
	    wmPtr2->withdrawn = 1;
	    wmPtr2->hints.initial_state = WithdrawnState;
	}
	wmPtr->icon = NULL;
    } else {
	if (TkGetWindowFromObj(interp, tkwin, objv[3], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (!Tk_IsTopLevel(tkwin2)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't use %s as icon window: not at top level",
		    Tcl_GetString(objv[3])));
	    Tcl_SetErrorCode(interp, "TK", "WM", "ICONWINDOW", "INNER", NULL);
	    return TCL_ERROR;
	}
	wmPtr2 = ((TkWindow *) tkwin2)->wmInfoPtr;
	if (wmPtr2->iconFor != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "%s is already an icon for %s",
		    Tcl_GetString(objv[3]), Tk_PathName(wmPtr2->iconFor)));
	    Tcl_SetErrorCode(interp, "TK", "WM", "ICONWINDOW", "ICON", NULL);
	    return TCL_ERROR;
	}
	if (wmPtr->icon != NULL) {
	    WmInfo *wmPtr3 = ((TkWindow *) wmPtr->icon)->wmInfoPtr;

	    wmPtr3->iconFor = NULL;
	    wmPtr3->withdrawn = 1;
	    wmPtr3->hints.initial_state = WithdrawnState;
	}

	/*
	 * Disable button events in the icon window: some window managers
	 * (like olvwm) want to get the events themselves, but X only allows
	 * one application at a time to receive button events for a window.
	 */

	atts.event_mask = Tk_Attributes(tkwin2)->event_mask
		& ~ButtonPressMask;
	Tk_ChangeWindowAttributes(tkwin2, CWEventMask, &atts);
	Tk_MakeWindowExist(tkwin2);
	if (wmPtr2->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr2);
	}
	wmPtr->hints.icon_window = Tk_WindowId(wmPtr2->wrapperPtr);
	wmPtr->hints.flags |= IconWindowHint;
	wmPtr->icon = tkwin2;
	wmPtr2->iconFor = (Tk_Window) winPtr;
	if (!wmPtr2->withdrawn && !(wmPtr2->flags & WM_NEVER_MAPPED)) {
	    wmPtr2->withdrawn = 0;
	    if (XWithdrawWindow(Tk_Display(tkwin2),
		    Tk_WindowId(wmPtr2->wrapperPtr),
		    Tk_ScreenNumber(tkwin2)) == 0) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"couldn't send withdraw message to window manager",
			-1));
		Tcl_SetErrorCode(interp, "TK", "WM", "COMMUNICATION", NULL);
		return TCL_ERROR;
	    }
	    WaitForMapNotify((TkWindow *) tkwin2, 0);
	}
    }
    UpdateHints(winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmManageCmd --
 *
 *	This procedure is invoked to process the "wm manage" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmManageCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel or Frame to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register Tk_Window frameWin = (Tk_Window) winPtr;
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (!Tk_IsTopLevel(frameWin)) {
	if (!Tk_IsManageable(frameWin)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" is not manageable: must be a frame,"
		    " labelframe or toplevel", Tk_PathName(frameWin)));
	    Tcl_SetErrorCode(interp, "TK", "WM", "MANAGE", NULL);
	    return TCL_ERROR;
	}
	TkFocusSplit(winPtr);
	Tk_UnmapWindow(frameWin);
	winPtr->flags |=
		TK_TOP_HIERARCHY|TK_TOP_LEVEL|TK_HAS_WRAPPER|TK_WIN_MANAGED;
	if (wmPtr == NULL) {
	    TkWmNewWindow(winPtr);
	    TkWmMapWindow(winPtr);
	    Tk_UnmapWindow(frameWin);
	}
	wmPtr = winPtr->wmInfoPtr;
	winPtr->flags &= ~TK_MAPPED;
	RemapWindows(winPtr, wmPtr->wrapperPtr);

	/*
	 * Flags (above) must be set before calling TkMapTopFrame (below).
	 */

	TkMapTopFrame(frameWin);
    } else if (Tk_IsTopLevel(frameWin)) {
	/*
	 * Already managed by wm - ignore it.
	 */
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmMaxsizeCmd --
 *
 *	This function is invoked to process the "wm maxsize" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmMaxsizeCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int width, height;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?width height?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_Obj *results[2];

	GetMaxSize(wmPtr, &width, &height);
	results[0] = Tcl_NewIntObj(width);
	results[1] = Tcl_NewIntObj(height);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, results));
	return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &width) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objv[4], &height) != TCL_OK)) {
	return TCL_ERROR;
    }
    wmPtr->maxWidth = width;
    wmPtr->maxHeight = height;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;

    if (width <= 0 && height <= 0) {
	wmPtr->sizeHintsFlags &= ~PMaxSize;
    } else {
	wmPtr->sizeHintsFlags |= PMaxSize;
    }

    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmMinsizeCmd --
 *
 *	This function is invoked to process the "wm minsize" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmMinsizeCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int width, height;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?width height?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_Obj *results[2];

	results[0] = Tcl_NewIntObj(wmPtr->minWidth);
	results[1] = Tcl_NewIntObj(wmPtr->minHeight);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, results));
	return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &width) != TCL_OK)
	    || (Tcl_GetIntFromObj(interp, objv[4], &height) != TCL_OK)) {
	return TCL_ERROR;
    }
    wmPtr->minWidth = width;
    wmPtr->minHeight = height;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmOverrideredirectCmd --
 *
 *	This function is invoked to process the "wm overrideredirect" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmOverrideredirectCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int boolean, curValue;
    XSetWindowAttributes atts;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?boolean?");
	return TCL_ERROR;
    }
    curValue = Tk_Attributes((Tk_Window) winPtr)->override_redirect;
    if (objc == 3) {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(curValue));
	return TCL_OK;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[3], &boolean) != TCL_OK) {
	return TCL_ERROR;
    }
    if (curValue != boolean) {
	/*
	 * Only do this if we are really changing value, because it causes
	 * some funky stuff to occur
	 */

	atts.override_redirect = (boolean) ? True : False;
	Tk_ChangeWindowAttributes((Tk_Window) winPtr, CWOverrideRedirect,
		&atts);
	if (winPtr->wmInfoPtr->wrapperPtr != NULL) {
	    Tk_ChangeWindowAttributes(
		    (Tk_Window) winPtr->wmInfoPtr->wrapperPtr,
		    CWOverrideRedirect, &atts);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmPositionfromCmd --
 *
 *	This function is invoked to process the "wm positionfrom" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmPositionfromCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static const char *const optionStrings[] = {
	"program", "user", NULL };
    enum options {
	OPT_PROGRAM, OPT_USER };
    int index;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?user/program?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	const char *sourceStr = "";

	if (wmPtr->sizeHintsFlags & USPosition) {
	    sourceStr = "user";
	} else if (wmPtr->sizeHintsFlags & PPosition) {
	    sourceStr = "program";
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(sourceStr, -1));
	return TCL_OK;
    }
    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->sizeHintsFlags &= ~(USPosition|PPosition);
    } else {
	if (Tcl_GetIndexFromObjStruct(interp, objv[3], optionStrings,
		sizeof(char *), "argument", 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == OPT_USER) {
	    wmPtr->sizeHintsFlags &= ~PPosition;
	    wmPtr->sizeHintsFlags |= USPosition;
	} else {
	    wmPtr->sizeHintsFlags &= ~USPosition;
	    wmPtr->sizeHintsFlags |= PPosition;
	}
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmProtocolCmd --
 *
 *	This function is invoked to process the "wm protocol" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmProtocolCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    register ProtocolHandler *protPtr, *prevPtr;
    Atom protocol;
    const char *cmd;
    int cmdLength;

    if ((objc < 3) || (objc > 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?name? ?command?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	/*
	 * Return a list of all defined protocols for the window.
	 */

	Tcl_Obj *resultObj = Tcl_NewObj();

	for (protPtr = wmPtr->protPtr; protPtr != NULL;
		protPtr = protPtr->nextPtr) {
	    Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		    Tk_GetAtomName((Tk_Window)winPtr, protPtr->protocol),-1));
	}
	Tcl_SetObjResult(interp, resultObj);
	return TCL_OK;
    }
    protocol = Tk_InternAtom((Tk_Window) winPtr, Tcl_GetString(objv[3]));
    if (objc == 4) {
	/*
	 * Return the command to handle a given protocol.
	 */

	for (protPtr = wmPtr->protPtr; protPtr != NULL;
		protPtr = protPtr->nextPtr) {
	    if (protPtr->protocol == protocol) {
		Tcl_SetObjResult(interp,
			Tcl_NewStringObj(protPtr->command, -1));
		return TCL_OK;
	    }
	}
	return TCL_OK;
    }

    /*
     * Special case for _NET_WM_PING: that's always handled directly.
     */

    if (strcmp(Tcl_GetString(objv[3]), "_NET_WM_PING") == 0) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"may not alter handling of that protocol", -1));
	Tcl_SetErrorCode(interp, "TK", "WM", "PROTOCOL", "RESERVED", NULL);
	return TCL_ERROR;
    }

    /*
     * Delete any current protocol handler, then create a new one with the
     * specified command, unless the command is empty.
     */

    for (protPtr = wmPtr->protPtr, prevPtr = NULL; protPtr != NULL;
	    prevPtr = protPtr, protPtr = protPtr->nextPtr) {
	if (protPtr->protocol == protocol) {
	    if (prevPtr == NULL) {
		wmPtr->protPtr = protPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = protPtr->nextPtr;
	    }
	    Tcl_EventuallyFree(protPtr, TCL_DYNAMIC);
	    break;
	}
    }
    cmd = Tcl_GetStringFromObj(objv[4], &cmdLength);
    if (cmdLength > 0) {
	protPtr = ckalloc(HANDLER_SIZE(cmdLength));
	protPtr->protocol = protocol;
	protPtr->nextPtr = wmPtr->protPtr;
	wmPtr->protPtr = protPtr;
	protPtr->interp = interp;
	memcpy(protPtr->command, cmd, cmdLength + 1);
    }
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	UpdateWmProtocols(wmPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmResizableCmd --
 *
 *	This function is invoked to process the "wm resizable" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmResizableCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int width, height;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?width height?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	Tcl_Obj *results[2];

	results[0] = Tcl_NewBooleanObj(!(wmPtr->flags&WM_WIDTH_NOT_RESIZABLE));
	results[1] = Tcl_NewBooleanObj(!(wmPtr->flags&WM_HEIGHT_NOT_RESIZABLE));
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, results));
	return TCL_OK;
    }
    if ((Tcl_GetBooleanFromObj(interp, objv[3], &width) != TCL_OK)
	    || (Tcl_GetBooleanFromObj(interp, objv[4], &height) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (width) {
	wmPtr->flags &= ~WM_WIDTH_NOT_RESIZABLE;
    } else {
	wmPtr->flags |= WM_WIDTH_NOT_RESIZABLE;
    }
    if (height) {
	wmPtr->flags &= ~WM_HEIGHT_NOT_RESIZABLE;
    } else {
	wmPtr->flags |= WM_HEIGHT_NOT_RESIZABLE;
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmSizefromCmd --
 *
 *	This function is invoked to process the "wm sizefrom" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmSizefromCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static const char *const optionStrings[] = {
	"program", "user", NULL };
    enum options {
	OPT_PROGRAM, OPT_USER };
    int index;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?user|program?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	const char *sourceStr = "";

	if (wmPtr->sizeHintsFlags & USSize) {
	    sourceStr = "user";
	} else if (wmPtr->sizeHintsFlags & PSize) {
	    sourceStr = "program";
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(sourceStr, -1));
	return TCL_OK;
    }

    if (*Tcl_GetString(objv[3]) == '\0') {
	wmPtr->sizeHintsFlags &= ~(USSize|PSize);
    } else {
	if (Tcl_GetIndexFromObjStruct(interp, objv[3], optionStrings,
		sizeof(char *), "argument", 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == OPT_USER) {
	    wmPtr->sizeHintsFlags &= ~PSize;
	    wmPtr->sizeHintsFlags |= USSize;
	} else { /* OPT_PROGRAM */
	    wmPtr->sizeHintsFlags &= ~USSize;
	    wmPtr->sizeHintsFlags |= PSize;
	}
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    WmUpdateGeom(wmPtr, winPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmStackorderCmd --
 *
 *	This function is invoked to process the "wm stackorder" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmStackorderCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    TkWindow **windows, **window_ptr;
    static const char *const optionStrings[] = {
	"isabove", "isbelow", NULL };
    enum options {
	OPT_ISABOVE, OPT_ISBELOW };
    int index;

    if ((objc != 3) && (objc != 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?isabove|isbelow window?");
	return TCL_ERROR;
    }

    if (objc == 3) {
	windows = TkWmStackorderToplevel(winPtr);
	if (windows != NULL) {
	    Tcl_Obj *resultObj = Tcl_NewObj();

	    /* ASSERT: true [Bug 1789819]*/
	    for (window_ptr = windows; *window_ptr ; window_ptr++) {
		Tcl_ListObjAppendElement(NULL, resultObj,
			Tcl_NewStringObj((*window_ptr)->pathName, -1));
	    }
	    ckfree(windows);
	    Tcl_SetObjResult(interp, resultObj);
	    return TCL_OK;
	} else {
	    return TCL_ERROR;
	}
    } else {
	Tk_Window relWin;
	TkWindow *winPtr2;
	int index1=-1, index2=-1, result;

	if (TkGetWindowFromObj(interp, tkwin, objv[4], &relWin) != TCL_OK) {
	    return TCL_ERROR;
	}
	winPtr2 = (TkWindow *) relWin;

	if (!Tk_IsTopLevel(winPtr2)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" isn't a top-level window",
		    winPtr2->pathName));
	    Tcl_SetErrorCode(interp, "TK", "WM", "STACK", "TOPLEVEL", NULL);
	    return TCL_ERROR;
	}

	if (!Tk_IsMapped(winPtr)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" isn't mapped", winPtr->pathName));
	    Tcl_SetErrorCode(interp, "TK", "WM", "STACK", "MAPPED", NULL);
	    return TCL_ERROR;
	}

	if (!Tk_IsMapped(winPtr2)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" isn't mapped", winPtr2->pathName));
	    Tcl_SetErrorCode(interp, "TK", "WM", "STACK", "MAPPED", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Lookup stacking order of all toplevels that are children of "." and
	 * find the position of winPtr and winPtr2 in the stacking order.
	 */

	windows = TkWmStackorderToplevel(winPtr->mainPtr->winPtr);
	if (windows == NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "TkWmStackorderToplevel failed", -1));
	    Tcl_SetErrorCode(interp, "TK", "WM", "COMMUNICATION", NULL);
	    return TCL_ERROR;
	}

	for (window_ptr = windows; *window_ptr ; window_ptr++) {
	    if (*window_ptr == winPtr) {
		index1 = (window_ptr - windows);
	    }
	    if (*window_ptr == winPtr2) {
		index2 = (window_ptr - windows);
	    }
	}
	/* ASSERT: index1 != -1 && index2 != -2 [Bug 1789819] */
	ckfree(windows);

	if (Tcl_GetIndexFromObjStruct(interp, objv[3], optionStrings,
		sizeof(char *), "argument", 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == OPT_ISABOVE) {
	    result = index1 > index2;
	} else { /* OPT_ISBELOW */
	    result = index1 < index2;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
	return TCL_OK;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmStateCmd --
 *
 *	This function is invoked to process the "wm state" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmStateCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    static const char *const optionStrings[] = {
	"normal", "iconic", "withdrawn", NULL };
    enum options {
	OPT_NORMAL, OPT_ICONIC, OPT_WITHDRAWN };
    int index;

    if ((objc < 3) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?state?");
	return TCL_ERROR;
    }
    if (objc == 4) {
	if (wmPtr->iconFor != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't change state of %s: it is an icon for %s",
		    Tcl_GetString(objv[2]), Tk_PathName(wmPtr->iconFor)));
	    Tcl_SetErrorCode(interp, "TK", "WM", "STATE", "ICON", NULL);
	    return TCL_ERROR;
	}

	if (Tcl_GetIndexFromObjStruct(interp, objv[3], optionStrings,
		sizeof(char *), "argument", 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (index == OPT_NORMAL) {
	    wmPtr->flags &= ~WM_WITHDRAWN;
	    (void) TkpWmSetState(winPtr, NormalState);
	} else if (index == OPT_ICONIC) {
	    if (Tk_Attributes((Tk_Window) winPtr)->override_redirect) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"can't iconify \"%s\": override-redirect flag is set",
			winPtr->pathName));
		Tcl_SetErrorCode(interp, "TK", "WM", "STATE",
			"OVERRIDE_REDIRECT", NULL);
		return TCL_ERROR;
	    }
	    if (wmPtr->masterPtr != NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"can't iconify \"%s\": it is a transient",
			winPtr->pathName));
		Tcl_SetErrorCode(interp, "TK", "WM", "STATE", "TRANSIENT",
			NULL);
		return TCL_ERROR;
	    }
	    if (TkpWmSetState(winPtr, IconicState) == 0) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"couldn't send iconify message to window manager",
			-1));
		Tcl_SetErrorCode(interp, "TK", "WM", "COMMUNICATION", NULL);
		return TCL_ERROR;
	    }
	} else { /* OPT_WITHDRAWN */
	    wmPtr->flags |= WM_WITHDRAWN;
	    if (TkpWmSetState(winPtr, WithdrawnState) == 0) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"couldn't send withdraw message to window manager",
			-1));
		Tcl_SetErrorCode(interp, "TK", "WM", "COMMUNICATION", NULL);
		return TCL_ERROR;
	    }
	}
    } else {
	const char *state;

	if (wmPtr->iconFor != NULL) {
	    state = "icon";
	} else if (wmPtr->withdrawn) {
	    state = "withdrawn";
	} else if (Tk_IsMapped((Tk_Window) winPtr)
		|| ((wmPtr->flags & WM_NEVER_MAPPED)
			&& (wmPtr->hints.initial_state == NormalState))) {
	    state = "normal";
	} else {
	    state = "iconic";
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(state, -1));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmTitleCmd --
 *
 *	This function is invoked to process the "wm title" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmTitleCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    const char *argv3;
    int length;

    if (objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?newTitle?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (wmPtr->title) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(wmPtr->title, -1));
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(winPtr->nameUid, -1));
	}
    } else {
	if (wmPtr->title != NULL) {
	    ckfree(wmPtr->title);
	}
	argv3 = Tcl_GetStringFromObj(objv[3], &length);
	wmPtr->title = ckalloc(length + 1);
	strcpy(wmPtr->title, argv3);

	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    UpdateTitle(winPtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmTransientCmd --
 *
 *	This function is invoked to process the "wm transient" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmTransientCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    TkWindow *masterPtr = wmPtr->masterPtr, *w;
    WmInfo *wmPtr2;

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?master?");
	return TCL_ERROR;
    }
    if (objc == 3) {
	if (masterPtr != NULL) {
	    Tcl_SetObjResult(interp, TkNewWindowObj((Tk_Window) masterPtr));
	}
	return TCL_OK;
    }
    if (Tcl_GetString(objv[3])[0] == '\0') {
	if (masterPtr != NULL) {
	    /*
	     * If we had a master, tell them that we aren't tied to them
	     * anymore
	     */

	    masterPtr->wmInfoPtr->numTransients--;
	    Tk_DeleteEventHandler((Tk_Window) masterPtr, StructureNotifyMask,
		    WmWaitMapProc, winPtr);

	    /*
	     * FIXME: Need a call like Win32's UpdateWrapper() so we can
	     * recreate the wrapper and get rid of the transient window
	     * decorations.
	     */
	}

	wmPtr->masterPtr = NULL;
    } else {
	Tk_Window masterWin;

	if (TkGetWindowFromObj(interp, tkwin, objv[3], &masterWin)!=TCL_OK) {
	    return TCL_ERROR;
	}
	masterPtr = (TkWindow *) masterWin;
	while (!Tk_TopWinHierarchy(masterPtr)) {
	    /*
	     * Ensure that the master window is actually a Tk toplevel.
	     */

	    masterPtr = masterPtr->parentPtr;
	}
	Tk_MakeWindowExist((Tk_Window) masterPtr);

	if (wmPtr->iconFor != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't make \"%s\" a transient: it is an icon for %s",
		    Tcl_GetString(objv[2]), Tk_PathName(wmPtr->iconFor)));
	    Tcl_SetErrorCode(interp, "TK", "WM", "TRANSIENT", "ICON", NULL);
	    return TCL_ERROR;
	}

	wmPtr2 = masterPtr->wmInfoPtr;
	if (wmPtr2->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr2);
	}

	if (wmPtr2->iconFor != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't make \"%s\" a master: it is an icon for %s",
		    Tcl_GetString(objv[3]), Tk_PathName(wmPtr2->iconFor)));
	    Tcl_SetErrorCode(interp, "TK", "WM", "TRANSIENT", "ICON", NULL);
	    return TCL_ERROR;
	}

	for (w = masterPtr; w != NULL && w->wmInfoPtr != NULL;
	     w = (TkWindow *)w->wmInfoPtr->masterPtr) {
	    if (w == winPtr) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "setting \"%s\" as master creates a transient/master cycle",
		    Tk_PathName(masterPtr)));
		Tcl_SetErrorCode(interp, "TK", "WM", "TRANSIENT", "SELF", NULL);
		return TCL_ERROR;
	    }
	}

	if (masterPtr != wmPtr->masterPtr) {
	    /*
	     * Remove old master map/unmap binding before setting the new
	     * master. The event handler will ensure that transient states
	     * reflect the state of the master.
	     */

	    if (wmPtr->masterPtr != NULL) {
		wmPtr->masterPtr->wmInfoPtr->numTransients--;
		Tk_DeleteEventHandler((Tk_Window) wmPtr->masterPtr,
			StructureNotifyMask, WmWaitMapProc, winPtr);
	    }

	    masterPtr->wmInfoPtr->numTransients++;
	    Tk_CreateEventHandler((Tk_Window) masterPtr,
		    StructureNotifyMask, WmWaitMapProc, winPtr);

	    wmPtr->masterPtr = masterPtr;
	}
    }
    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	if (wmPtr->masterPtr != NULL && !Tk_IsMapped(wmPtr->masterPtr)) {
	    if (TkpWmSetState(winPtr, WithdrawnState) == 0) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"couldn't send withdraw message to window manager",
			-1));
		Tcl_SetErrorCode(interp, "TK", "WM", "COMMUNICATION", NULL);
		return TCL_ERROR;
	    }
	} else {
	    if (wmPtr->masterPtr != NULL) {
		XSetTransientForHint(winPtr->display,
			wmPtr->wrapperPtr->window,
			wmPtr->masterPtr->wmInfoPtr->wrapperPtr->window);
	    } else {
		XDeleteProperty(winPtr->display, wmPtr->wrapperPtr->window,
			Tk_InternAtom((Tk_Window) winPtr,"WM_TRANSIENT_FOR"));
	    }
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WmWithdrawCmd --
 *
 *	This function is invoked to process the "wm withdraw" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
WmWithdrawCmd(
    Tk_Window tkwin,		/* Main window of the application. */
    TkWindow *winPtr,		/* Toplevel to work with */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }
    if (wmPtr->iconFor != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't withdraw %s: it is an icon for %s",
		Tcl_GetString(objv[2]), Tk_PathName(wmPtr->iconFor)));
	Tcl_SetErrorCode(interp, "TK", "WM", "WITHDRAW", "ICON", NULL);
	return TCL_ERROR;
    }
    wmPtr->flags |= WM_WITHDRAWN;
    if (TkpWmSetState(winPtr, WithdrawnState) == 0) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"couldn't send withdraw message to window manager", -1));
	Tcl_SetErrorCode(interp, "TK", "WM", "COMMUNICATION", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * Invoked by those wm subcommands that affect geometry. Schedules a geometry
 * update.
 */

static void
WmUpdateGeom(
    WmInfo *wmPtr,
    TkWindow *winPtr)
{
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 * Invoked when a MapNotify or UnmapNotify event is delivered for a toplevel
 * that is the master of a transient toplevel.
 */

static void
WmWaitMapProc(
    ClientData clientData,	/* Pointer to window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkWindow *winPtr = clientData;
    TkWindow *masterPtr = winPtr->wmInfoPtr->masterPtr;

    if (masterPtr == NULL) {
	return;
    }

    if (eventPtr->type == MapNotify) {
	if (!(winPtr->wmInfoPtr->flags & WM_WITHDRAWN)) {
	    (void) TkpWmSetState(winPtr, NormalState);
	}
    } else if (eventPtr->type == UnmapNotify) {
	(void) TkpWmSetState(winPtr, WithdrawnState);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetGrid --
 *
 *	This function is invoked by a widget when it wishes to set a grid
 *	coordinate system that controls the size of a top-level window. It
 *	provides a C interface equivalent to the "wm grid" command and is
 *	usually associated with the -setgrid option.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Grid-related information will be passed to the window manager, so that
 *	the top-level window associated with tkwin will resize on even grid
 *	units. If some other window already controls gridding for the
 *	top-level window then this function call has no effect.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetGrid(
    Tk_Window tkwin,		/* Token for window. New window mgr info will
				 * be posted for the top-level window
				 * associated with this window. */
    int reqWidth,		/* Width (in grid units) corresponding to the
				 * requested geometry for tkwin. */
    int reqHeight,		/* Height (in grid units) corresponding to the
				 * requested geometry for tkwin. */
    int widthInc, int heightInc)/* Pixel increments corresponding to a change
				 * of one grid unit. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr;

    /*
     * Ensure widthInc and heightInc are greater than 0
     */

    if (widthInc <= 0) {
	widthInc = 1;
    }
    if (heightInc <= 0) {
	heightInc = 1;
    }

    /*
     * Find the top-level window for tkwin, plus the window manager
     * information.
     */

    while (!(winPtr->flags & TK_TOP_HIERARCHY)) {
	winPtr = winPtr->parentPtr;
	if (winPtr == NULL) {
	    /*
	     * The window is being deleted... just skip this operation.
	     */

	    return;
	}
    }
    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }

    if ((wmPtr->gridWin != NULL) && (wmPtr->gridWin != tkwin)) {
	return;
    }

    if ((wmPtr->reqGridWidth == reqWidth)
	    && (wmPtr->reqGridHeight == reqHeight)
	    && (wmPtr->widthInc == widthInc)
	    && (wmPtr->heightInc == heightInc)
	    && ((wmPtr->sizeHintsFlags & (PBaseSize|PResizeInc))
		    == (PBaseSize|PResizeInc))) {
	return;
    }

    /*
     * If gridding was previously off, then forget about any window size
     * requests made by the user or via "wm geometry": these are in pixel
     * units and there's no easy way to translate them to grid units since the
     * new requested size of the top-level window in pixels may not yet have
     * been registered yet (it may filter up the hierarchy in DoWhenIdle
     * handlers). However, if the window has never been mapped yet then just
     * leave the window size alone: assume that it is intended to be in grid
     * units but just happened to have been specified before this function was
     * called.
     */

    if ((wmPtr->gridWin == NULL) && !(wmPtr->flags & WM_NEVER_MAPPED)) {
	wmPtr->width = -1;
	wmPtr->height = -1;
    }

    /*
     * Set the new gridding information, and start the process of passing all
     * of this information to the window manager.
     */

    wmPtr->gridWin = tkwin;
    wmPtr->reqGridWidth = reqWidth;
    wmPtr->reqGridHeight = reqHeight;
    wmPtr->widthInc = widthInc;
    wmPtr->heightInc = heightInc;
    wmPtr->sizeHintsFlags |= PBaseSize|PResizeInc;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_UnsetGrid --
 *
 *	This function cancels the effect of a previous call to Tk_SetGrid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If tkwin currently controls gridding for its top-level window,
 *	gridding is cancelled for that top-level window; if some other window
 *	controls gridding then this function has no effect.
 *
 *----------------------------------------------------------------------
 */

void
Tk_UnsetGrid(
    Tk_Window tkwin)		/* Token for window that is currently
				 * controlling gridding. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr;

    /*
     * Find the top-level window for tkwin, plus the window manager
     * information.
     */

    while (!(winPtr->flags & TK_TOP_HIERARCHY)) {
	winPtr = winPtr->parentPtr;
	if (winPtr == NULL) {
	    /*
	     * The window is being deleted... just skip this operation.
	     */

	    return;
	}
    }
    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }

    if (tkwin != wmPtr->gridWin) {
	return;
    }

    wmPtr->gridWin = NULL;
    wmPtr->sizeHintsFlags &= ~(PBaseSize|PResizeInc);
    if (wmPtr->width != -1) {
	wmPtr->width = winPtr->reqWidth + (wmPtr->width
		- wmPtr->reqGridWidth)*wmPtr->widthInc;
	wmPtr->height = winPtr->reqHeight + (wmPtr->height
		- wmPtr->reqGridHeight)*wmPtr->heightInc;
    }
    wmPtr->widthInc = 1;
    wmPtr->heightInc = 1;

    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureEvent --
 *
 *	This function is called to handle ConfigureNotify events on wrapper
 *	windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets updated in the WmInfo structure for the window and
 *	the toplevel itself gets repositioned within the wrapper.
 *
 *----------------------------------------------------------------------
 */

static void
ConfigureEvent(
    WmInfo *wmPtr,		/* Information about toplevel window. */
    XConfigureEvent *configEventPtr)
				/* Event that just occurred for
				 * wmPtr->wrapperPtr. */
{
    TkWindow *wrapperPtr = wmPtr->wrapperPtr;
    TkWindow *winPtr = wmPtr->winPtr;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;
    Tk_ErrorHandler handler;

    /*
     * Update size information from the event. There are a couple of tricky
     * points here:
     *
     * 1. If the user changed the size externally then set wmPtr->width and
     *    wmPtr->height just as if a "wm geometry" command had been invoked
     *    with the same information.
     * 2. However, if the size is changing in response to a request coming
     *    from us (WM_SYNC_PENDING is set), then don't set wmPtr->width or
     *    wmPtr->height if they were previously -1 (otherwise the window will
     *    stop tracking geometry manager requests).
     */

    if (((wrapperPtr->changes.width != configEventPtr->width)
	    || (wrapperPtr->changes.height != configEventPtr->height))
	    && !(wmPtr->flags & WM_SYNC_PENDING)) {
	if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("TopLevelEventProc: user changed %s size to %dx%d\n",
		    winPtr->pathName, configEventPtr->width,
		    configEventPtr->height);
	}
	if ((wmPtr->width == -1)
		&& (configEventPtr->width == winPtr->reqWidth)) {
	    /*
	     * Don't set external width, since the user didn't change it from
	     * what the widgets asked for.
	     */
	} else {
	    /*
	     * Note: if this window is embedded then don't set the external
	     * size, since it came from the containing application, not the
	     * user. In this case we want to keep sending our size requests to
	     * the containing application; if the user fixes the size of that
	     * application then it will still percolate down to us in the
	     * right way.
	     */

	    if (!(winPtr->flags & TK_EMBEDDED)) {
		if (wmPtr->gridWin != NULL) {
		    wmPtr->width = wmPtr->reqGridWidth
			    + (configEventPtr->width
			    - winPtr->reqWidth)/wmPtr->widthInc;
		    if (wmPtr->width < 0) {
			wmPtr->width = 0;
		    }
		} else {
		    wmPtr->width = configEventPtr->width;
		}
	    }
	}
	if ((wmPtr->height == -1)
		&& (configEventPtr->height ==
			(winPtr->reqHeight + wmPtr->menuHeight))) {
	    /*
	     * Don't set external height, since the user didn't change it from
	     * what the widgets asked for.
	     */
	} else {
	    /*
	     * See note for wmPtr->width about not setting external size for
	     * embedded windows.
	     */

	    if (!(winPtr->flags & TK_EMBEDDED)) {
		if (wmPtr->gridWin != NULL) {
		    wmPtr->height = wmPtr->reqGridHeight
			    + (configEventPtr->height - wmPtr->menuHeight
			    - winPtr->reqHeight)/wmPtr->heightInc;
		    if (wmPtr->height < 0) {
			wmPtr->height = 0;
		    }
		} else {
		    wmPtr->height = configEventPtr->height - wmPtr->menuHeight;
		}
	    }
	}
	wmPtr->configWidth = configEventPtr->width;
	wmPtr->configHeight = configEventPtr->height;
    }

    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("ConfigureEvent: %s x = %d y = %d, width = %d, height = %d\n",
		winPtr->pathName, configEventPtr->x, configEventPtr->y,
		configEventPtr->width, configEventPtr->height);
	printf("    send_event = %d, serial = %ld (win %p, wrapper %p)\n",
		configEventPtr->send_event, configEventPtr->serial,
		winPtr, wrapperPtr);
    }
    wrapperPtr->changes.width = configEventPtr->width;
    wrapperPtr->changes.height = configEventPtr->height;
    wrapperPtr->changes.border_width = configEventPtr->border_width;
    wrapperPtr->changes.sibling = configEventPtr->above;
    wrapperPtr->changes.stack_mode = Above;

    /*
     * Reparenting window managers make life difficult. If the window manager
     * reparents a top-level window then the x and y information that comes in
     * events for the window is wrong: it gives the location of the window
     * inside its decorative parent, rather than the location of the window in
     * root coordinates, which is what we want. Window managers are supposed
     * to send synthetic events with the correct information, but ICCCM
     * doesn't require them to do this under all conditions, and the
     * information provided doesn't include everything we need here. So, the
     * code below maintains a bunch of information about the parent window.
     * If the window hasn't been reparented, we pretend that there is a parent
     * shrink-wrapped around the window.
     */

    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("    %s parent == %p, above %p\n",
		winPtr->pathName, (void *) wmPtr->reparent,
		(void *) configEventPtr->above);
    }

    if ((wmPtr->reparent == None) || !ComputeReparentGeometry(wmPtr)) {
	wmPtr->parentWidth = configEventPtr->width
		+ 2*configEventPtr->border_width;
	wmPtr->parentHeight = configEventPtr->height
		+ 2*configEventPtr->border_width;
	wrapperPtr->changes.x = wmPtr->x = configEventPtr->x;
	wrapperPtr->changes.y = wmPtr->y = configEventPtr->y;
	if (wmPtr->flags & WM_NEGATIVE_X) {
	    wmPtr->x = wmPtr->vRootWidth - (wmPtr->x + wmPtr->parentWidth);
	}
	if (wmPtr->flags & WM_NEGATIVE_Y) {
	    wmPtr->y = wmPtr->vRootHeight - (wmPtr->y + wmPtr->parentHeight);
	}
    }

    /*
     * Make sure that the toplevel and menubar are properly positioned within
     * the wrapper. If the menuHeight happens to be zero, we'll get a BadValue
     * X error that we want to ignore [Bug: 3377]
     */

    handler = Tk_CreateErrorHandler(winPtr->display, -1, -1, -1, NULL, NULL);
    XMoveResizeWindow(winPtr->display, winPtr->window, 0,
	    wmPtr->menuHeight, (unsigned) wrapperPtr->changes.width,
	    (unsigned) (wrapperPtr->changes.height - wmPtr->menuHeight));
    Tk_DeleteErrorHandler(handler);
    if ((wmPtr->menubar != NULL)
	    && ((Tk_Width(wmPtr->menubar) != wrapperPtr->changes.width)
	    || (Tk_Height(wmPtr->menubar) != wmPtr->menuHeight))) {
	Tk_MoveResizeWindow(wmPtr->menubar, 0, 0, wrapperPtr->changes.width,
		wmPtr->menuHeight);
    }

    /*
     * Update the coordinates in the toplevel (they should refer to the
     * position in root window coordinates, not the coordinates of the wrapper
     * window). Then synthesize a ConfigureNotify event to tell the
     * application about the change.
     */

    winPtr->changes.x = wrapperPtr->changes.x;
    winPtr->changes.y = wrapperPtr->changes.y + wmPtr->menuHeight;
    winPtr->changes.width = wrapperPtr->changes.width;
    winPtr->changes.height = wrapperPtr->changes.height - wmPtr->menuHeight;
    TkDoConfigureNotify(winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ReparentEvent --
 *
 *	This function is called to handle ReparentNotify events on wrapper
 *	windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets updated in the WmInfo structure for the window.
 *
 *----------------------------------------------------------------------
 */

static void
ReparentEvent(
    WmInfo *wmPtr,		/* Information about toplevel window. */
    XReparentEvent *reparentEventPtr)
				/* Event that just occurred for
				 * wmPtr->wrapperPtr. */
{
    TkWindow *wrapperPtr = wmPtr->wrapperPtr;
    Window vRoot, ancestor, *children, dummy2, *virtualRootPtr, **vrPtrPtr;
    Atom actualType;
    int actualFormat;
    unsigned long numItems, bytesAfter;
    unsigned dummy;
    Tk_ErrorHandler handler;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;
    Atom WM_ROOT = Tk_InternAtom((Tk_Window) wrapperPtr, "__WM_ROOT");
    Atom SWM_ROOT = Tk_InternAtom((Tk_Window) wrapperPtr, "__SWM_ROOT");

    /*
     * Identify the root window for wrapperPtr. This is tricky because of
     * virtual root window managers like tvtwm. If the window has a property
     * named __SWM_ROOT or __WM_ROOT then this property gives the id for a
     * virtual root window that should be used instead of the root window of
     * the screen.
     */

    vRoot = RootWindow(wrapperPtr->display, wrapperPtr->screenNum);
    wmPtr->vRoot = None;
    handler = Tk_CreateErrorHandler(wrapperPtr->display, -1,-1,-1, NULL,NULL);
    vrPtrPtr = &virtualRootPtr;		/* Silence GCC warning */
    if ((GetWindowProperty(wrapperPtr, WM_ROOT, 1, XA_WINDOW,
	    &actualType, &actualFormat, &numItems, &bytesAfter, vrPtrPtr)
	    && (actualType == XA_WINDOW))
	|| (GetWindowProperty(wrapperPtr, SWM_ROOT, 1, XA_WINDOW,
	    &actualType, &actualFormat, &numItems, &bytesAfter, vrPtrPtr)
	    && (actualType == XA_WINDOW))) {
	if ((actualFormat == 32) && (numItems == 1)) {
	    vRoot = wmPtr->vRoot = *virtualRootPtr;
	} else if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("%s format %d numItems %ld\n",
		    "ReparentEvent got bogus VROOT property:", actualFormat,
		    numItems);
	}
	XFree((char *) virtualRootPtr);
    }
    Tk_DeleteErrorHandler(handler);

    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("ReparentEvent: %s (%p) reparented to 0x%x, vRoot = 0x%x\n",
		wmPtr->winPtr->pathName, wmPtr->winPtr,
		(unsigned) reparentEventPtr->parent, (unsigned) vRoot);
    }

    /*
     * Fetch correct geometry information for the new virtual root.
     */

    UpdateVRootGeometry(wmPtr);

    /*
     * If the window's new parent is the root window, then mark it as no
     * longer reparented.
     */

    if (reparentEventPtr->parent == vRoot) {
    noReparent:
	wmPtr->reparent = None;
	wmPtr->parentWidth = wrapperPtr->changes.width;
	wmPtr->parentHeight = wrapperPtr->changes.height;
	wmPtr->xInParent = wmPtr->yInParent = 0;
	wrapperPtr->changes.x = reparentEventPtr->x;
	wrapperPtr->changes.y = reparentEventPtr->y;
	wmPtr->winPtr->changes.x = reparentEventPtr->x;
	wmPtr->winPtr->changes.y = reparentEventPtr->y + wmPtr->menuHeight;
	return;
    }

    /*
     * Search up the window hierarchy to find the ancestor of this window that
     * is just below the (virtual) root. This is tricky because it's possible
     * that things have changed since the event was generated so that the
     * ancestry indicated by the event no longer exists. If this happens then
     * an error will occur and we just discard the event (there will be a more
     * up-to-date ReparentNotify event coming later).
     */

    handler = Tk_CreateErrorHandler(wrapperPtr->display, -1,-1,-1, NULL,NULL);
    wmPtr->reparent = reparentEventPtr->parent;
    while (1) {
	if (XQueryTree(wrapperPtr->display, wmPtr->reparent, &dummy2,
		&ancestor, &children, &dummy) == 0) {
	    Tk_DeleteErrorHandler(handler);
	    goto noReparent;
	}
	XFree((char *) children);
	if ((ancestor == vRoot) ||
		(ancestor == RootWindow(wrapperPtr->display,
		wrapperPtr->screenNum))) {
	    break;
	}
	wmPtr->reparent = ancestor;
    }
    Tk_DeleteErrorHandler(handler);

    if (!ComputeReparentGeometry(wmPtr)) {
	goto noReparent;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeReparentGeometry --
 *
 *	This function is invoked to recompute geometry information related to
 *	a reparented top-level window, such as the position and total size of
 *	the parent and the position within it of the top-level window.
 *
 * Results:
 *	The return value is 1 if everything completed successfully and 0 if an
 *	error occurred while querying information about winPtr's parents. In
 *	this case winPtr is marked as no longer being reparented.
 *
 * Side effects:
 *	Geometry information in wmPtr, wmPtr->winPtr, and wmPtr->wrapperPtr
 *	gets updated.
 *
 *----------------------------------------------------------------------
 */

static int
ComputeReparentGeometry(
    WmInfo *wmPtr)		/* Information about toplevel window whose
				 * reparent info is to be recomputed. */
{
    TkWindow *wrapperPtr = wmPtr->wrapperPtr;
    int width, height, bd;
    unsigned dummy;
    int xOffset, yOffset, x, y;
    Window dummy2;
    Status status;
    Tk_ErrorHandler handler;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;

    handler = Tk_CreateErrorHandler(wrapperPtr->display, -1,-1,-1, NULL,NULL);
    (void) XTranslateCoordinates(wrapperPtr->display, wrapperPtr->window,
	    wmPtr->reparent, 0, 0, &xOffset, &yOffset, &dummy2);
    status = XGetGeometry(wrapperPtr->display, wmPtr->reparent,
	    &dummy2, &x, &y, (unsigned *) &width, (unsigned *) &height,
	    (unsigned *) &bd, &dummy);
    Tk_DeleteErrorHandler(handler);
    if (status == 0) {
	/*
	 * It appears that the reparented parent went away and no-one told us.
	 * Reset the window to indicate that it's not reparented.
	 */

	wmPtr->reparent = None;
	wmPtr->xInParent = wmPtr->yInParent = 0;
	return 0;
    }
    wmPtr->xInParent = xOffset + bd;
    wmPtr->yInParent = yOffset + bd;
    wmPtr->parentWidth = width + 2*bd;
    wmPtr->parentHeight = height + 2*bd;

    /*
     * Some tricky issues in updating wmPtr->x and wmPtr->y:
     *
     * 1. Don't update them if the event occurred because of something we did
     * (i.e. WM_SYNC_PENDING and WM_MOVE_PENDING are both set). This is
     * because window managers treat coords differently than Tk, and no two
     * window managers are alike. If the window manager moved the window
     * because we told it to, remember the coordinates we told it, not the
     * ones it actually moved it to. This allows us to move the window back to
     * the same coordinates later and get the same result. Without this check,
     * windows can "walk" across the screen under some conditions.
     *
     * 2. Don't update wmPtr->x and wmPtr->y unless wrapperPtr->changes.x or
     * wrapperPtr->changes.y has changed (otherwise a size change can spoof us
     * into thinking that the position changed too and defeat the intent of
     * (1) above.
     *
     * (As of 9/96 the above 2 comments appear to be stale. They're being left
     * in place as a reminder of what was once true (and perhaps should still
     * be true?)).
     *
     * 3. Ignore size changes coming from the window system if we're about to
     * change the size ourselves but haven't seen the event for it yet: our
     * size change is supposed to take priority.
     */

    if (!(wmPtr->flags & WM_MOVE_PENDING)
	    && ((wrapperPtr->changes.x != (x + wmPtr->xInParent))
	    || (wrapperPtr->changes.y != (y + wmPtr->yInParent)))) {
	wmPtr->x = x;
	if (wmPtr->flags & WM_NEGATIVE_X) {
	    wmPtr->x = wmPtr->vRootWidth - (wmPtr->x + wmPtr->parentWidth);
	}
	wmPtr->y = y;
	if (wmPtr->flags & WM_NEGATIVE_Y) {
	    wmPtr->y = wmPtr->vRootHeight - (wmPtr->y + wmPtr->parentHeight);
	}
    }

    wrapperPtr->changes.x = x + wmPtr->xInParent;
    wrapperPtr->changes.y = y + wmPtr->yInParent;
    if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("wrapperPtr %p coords %d,%d\n",
		wrapperPtr, wrapperPtr->changes.x, wrapperPtr->changes.y);
	printf("     wmPtr %p coords %d,%d, offsets %d %d\n",
		wmPtr, wmPtr->x, wmPtr->y, wmPtr->xInParent, wmPtr->yInParent);
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * PropertyEvent --
 *
 *	Handle PropertyNotify events on wrapper windows. The following
 *	properties are of interest:
 *
 *	_NET_WM_STATE:
 *		Used to keep wmPtr->attributes up to date.
 *
 *----------------------------------------------------------------------
 */

static void
PropertyEvent(
    WmInfo *wmPtr,		/* Information about toplevel window. */
    XPropertyEvent *eventPtr)	/* PropertyNotify event structure */
{
    TkWindow *wrapperPtr = wmPtr->wrapperPtr;
    Atom _NET_WM_STATE =
	    Tk_InternAtom((Tk_Window) wmPtr->winPtr, "_NET_WM_STATE");

    if (eventPtr->atom == _NET_WM_STATE) {
	Atom actualType;
	int actualFormat;
	unsigned long numItems, bytesAfter;
	unsigned char *propertyValue = 0;
	long maxLength = 1024;

	if (GetWindowProperty(wrapperPtr, _NET_WM_STATE, maxLength, XA_ATOM,
		&actualType, &actualFormat, &numItems, &bytesAfter,
		&propertyValue)) {
	    CheckNetWmState(wmPtr, (Atom *) propertyValue, (int) numItems);
	    XFree(propertyValue);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WrapperEventProc --
 *
 *	This function is invoked by the event loop when a wrapper window is
 *	restructured.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tk's internal data structures for the window get modified to reflect
 *	the structural change.
 *
 *----------------------------------------------------------------------
 */

static const unsigned WrapperEventMask =
	(StructureNotifyMask | PropertyChangeMask);

static void
WrapperEventProc(
    ClientData clientData,	/* Information about toplevel window. */
    XEvent *eventPtr)		/* Event that just happened. */
{
    WmInfo *wmPtr = clientData;
    XEvent mapEvent;
    TkDisplay *dispPtr = wmPtr->winPtr->dispPtr;

    wmPtr->flags |= WM_VROOT_OFFSET_STALE;
    if (eventPtr->type == DestroyNotify) {
	Tk_ErrorHandler handler;

	if (!(wmPtr->wrapperPtr->flags & TK_ALREADY_DEAD)) {
	    /*
	     * A top-level window was deleted externally (e.g., by the window
	     * manager). This is probably not a good thing, but cleanup as
	     * best we can. The error handler is needed because
	     * Tk_DestroyWindow will try to destroy the window, but of course
	     * it's already gone.
	     */

	    handler = Tk_CreateErrorHandler(wmPtr->winPtr->display, -1, -1, -1,
		    NULL, NULL);
	    Tk_DestroyWindow((Tk_Window) wmPtr->winPtr);
	    Tk_DeleteErrorHandler(handler);
	}
	if (dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("TopLevelEventProc: %s deleted\n", wmPtr->winPtr->pathName);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	/*
	 * Ignore the event if the window has never been mapped yet. Such an
	 * event occurs only in weird cases like changing the internal border
	 * width of a top-level window, which results in a synthetic Configure
	 * event. These events are not relevant to us, and if we process them
	 * confusion may result (e.g. we may conclude erroneously that the
	 * user repositioned or resized the window).
	 */

	if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	    ConfigureEvent(wmPtr, &eventPtr->xconfigure);
	}
    } else if (eventPtr->type == MapNotify) {
	wmPtr->wrapperPtr->flags |= TK_MAPPED;
	wmPtr->winPtr->flags |= TK_MAPPED;
	XMapWindow(wmPtr->winPtr->display, wmPtr->winPtr->window);
	goto doMapEvent;
    } else if (eventPtr->type == UnmapNotify) {
	wmPtr->wrapperPtr->flags &= ~TK_MAPPED;
	wmPtr->winPtr->flags &= ~TK_MAPPED;
	XUnmapWindow(wmPtr->winPtr->display, wmPtr->winPtr->window);
	goto doMapEvent;
    } else if (eventPtr->type == ReparentNotify) {
	ReparentEvent(wmPtr, &eventPtr->xreparent);
    } else if (eventPtr->type == PropertyNotify) {
	PropertyEvent(wmPtr, &eventPtr->xproperty);
    }
    return;

  doMapEvent:
    mapEvent = *eventPtr;
    mapEvent.xmap.event = wmPtr->winPtr->window;
    mapEvent.xmap.window = wmPtr->winPtr->window;
    Tk_HandleEvent(&mapEvent);
}

/*
 *----------------------------------------------------------------------
 *
 * TopLevelReqProc --
 *
 *	This function is invoked by the geometry manager whenever the
 *	requested size for a top-level window is changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arrange for the window to be resized to satisfy the request (this
 *	happens as a when-idle action).
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
TopLevelReqProc(
    ClientData dummy,		/* Not used. */
    Tk_Window tkwin)		/* Information about window. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (wmPtr == NULL) {
	return;
    }

    if ((wmPtr->width >= 0) && (wmPtr->height >= 0)) {
	/*
	 * Explicit dimensions have been set for this window, so we should
	 * ignore the geometry request. It's actually important to ignore the
	 * geometry request because, due to quirks in window managers,
	 * invoking UpdateGeometryInfo may cause the window to move. For
	 * example, if "wm geometry -10-20" was invoked, the window may be
	 * positioned incorrectly the first time it appears (because we didn't
	 * know the proper width of the window manager borders); if we invoke
	 * UpdateGeometryInfo again, the window will be positioned correctly,
	 * which may cause it to jump on the screen.
	 */

	return;
    }

    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }

    /*
     * If the window isn't being positioned by its upper left corner then we
     * have to move it as well.
     */

    if (wmPtr->flags & (WM_NEGATIVE_X | WM_NEGATIVE_Y)) {
	wmPtr->flags |= WM_MOVE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateGeometryInfo --
 *
 *	This function is invoked when a top-level window is first mapped, and
 *	also as a when-idle function, to bring the geometry and/or position of
 *	a top-level window back into line with what has been requested by the
 *	user and/or widgets. This function doesn't return until the window
 *	manager has responded to the geometry change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size and location of both the toplevel window and its wrapper may
 *	change, unless the WM prevents that from happening.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateGeometryInfo(
    ClientData clientData)	/* Pointer to the window's record. */
{
    register TkWindow *winPtr = clientData;
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int x, y, width, height, min, max;
    unsigned long serial;

    wmPtr->flags &= ~WM_UPDATE_PENDING;

    /*
     * Compute the new size for the top-level window. See the user
     * documentation for details on this, but the size requested depends on
     * (a) the size requested internally by the window's widgets, (b) the size
     * requested by the user in a "wm geometry" command or via wm-based
     * interactive resizing (if any), (c) whether or not the window is
     * gridded, and (d) the current min or max size for the toplevel. Don't
     * permit sizes <= 0 because this upsets the X server.
     */

    if (wmPtr->width == -1) {
	width = winPtr->reqWidth;
    } else if (wmPtr->gridWin != NULL) {
	width = winPtr->reqWidth
		+ (wmPtr->width - wmPtr->reqGridWidth)*wmPtr->widthInc;
    } else {
	width = wmPtr->width;
    }
    if (width <= 0) {
	width = 1;
    }

    /*
     * Account for window max/min width
     */

    if (wmPtr->gridWin != NULL) {
	min = winPtr->reqWidth
		+ (wmPtr->minWidth - wmPtr->reqGridWidth)*wmPtr->widthInc;
	if (wmPtr->maxWidth > 0) {
	    max = winPtr->reqWidth
		+ (wmPtr->maxWidth - wmPtr->reqGridWidth)*wmPtr->widthInc;
	} else {
	    max = 0;
	}
    } else {
	min = wmPtr->minWidth;
	max = wmPtr->maxWidth;
    }
    if (width < min) {
	width = min;
    } else if ((max > 0) && (width > max)) {
	width = max;
    }

    if (wmPtr->height == -1) {
	height = winPtr->reqHeight;
    } else if (wmPtr->gridWin != NULL) {
	height = winPtr->reqHeight
		+ (wmPtr->height - wmPtr->reqGridHeight)*wmPtr->heightInc;
    } else {
	height = wmPtr->height;
    }
    if (height <= 0) {
	height = 1;
    }

    /*
     * Account for window max/min height
     */

    if (wmPtr->gridWin != NULL) {
	min = winPtr->reqHeight
		+ (wmPtr->minHeight - wmPtr->reqGridHeight)*wmPtr->heightInc;
	if (wmPtr->maxHeight > 0) {
	    max = winPtr->reqHeight
		+ (wmPtr->maxHeight - wmPtr->reqGridHeight)*wmPtr->heightInc;
	} else {
	    max = 0;
	}
    } else {
	min = wmPtr->minHeight;
	max = wmPtr->maxHeight;
    }
    if (height < min) {
	height = min;
    } else if ((max > 0) && (height > max)) {
	height = max;
    }

    /*
     * Compute the new position for the upper-left pixel of the window's
     * decorative frame. This is tricky, because we need to include the border
     * widths supplied by a reparented parent in this calculation, but can't
     * use the parent's current overall size since that may change as a result
     * of this code.
     */

    if (wmPtr->flags & WM_NEGATIVE_X) {
	x = wmPtr->vRootWidth - wmPtr->x
		- (width + (wmPtr->parentWidth - winPtr->changes.width));
    } else {
	x = wmPtr->x;
    }
    if (wmPtr->flags & WM_NEGATIVE_Y) {
	y = wmPtr->vRootHeight - wmPtr->y
		- (height + (wmPtr->parentHeight - winPtr->changes.height));
    } else {
	y = wmPtr->y;
    }

    /*
     * If the window's size is going to change and the window is supposed to
     * not be resizable by the user, then we have to update the size hints.
     * There may also be a size-hint-update request pending from somewhere
     * else, too.
     */

    if (((width != winPtr->changes.width)
	    || (height != winPtr->changes.height))
	    && (wmPtr->gridWin == NULL)
	    && !(wmPtr->sizeHintsFlags & (PMinSize|PMaxSize))) {
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    }
    if (wmPtr->flags & WM_UPDATE_SIZE_HINTS) {
	UpdateSizeHints(winPtr, width, height);
    }

    /*
     * Reconfigure the wrapper if it isn't already configured correctly. A few
     * tricky points:
     *
     * 1. If the window is embedded and the container is also in this process,
     *    don't actually reconfigure the window; just pass the desired size on
     *    to the container. Also, zero out any position information, since
     *    embedded windows are not allowed to move.
     * 2. Sometimes the window manager will give us a different size than we
     *    asked for (e.g. mwm has a minimum size for windows), so base the
     *    size check on what we *asked for* last time, not what we got.
     * 3. Can't just reconfigure always, because we may not get a
     *    ConfigureNotify event back if nothing changed, so
     *    WaitForConfigureNotify will hang a long time.
     * 4. Don't move window unless a new position has been requested for it.
     *	  This is because of "features" in some window managers (e.g. twm, as
     *	  of 4/24/91) where they don't interpret coordinates according to
     *	  ICCCM. Moving a window to its current location may cause it to shift
     *	  position on the screen.
     */

    if ((winPtr->flags & (TK_EMBEDDED|TK_BOTH_HALVES))
	    == (TK_EMBEDDED|TK_BOTH_HALVES)) {
	TkWindow *childPtr = TkpGetOtherWindow(winPtr);

	/*
	 * This window is embedded and the container is also in this process,
	 * so we don't need to do anything special about the geometry, except
	 * to make sure that the desired size is known by the container. Also,
	 * zero out any position information, since embedded windows are not
	 * allowed to move.
	 */

	wmPtr->x = wmPtr->y = 0;
	wmPtr->flags &= ~(WM_NEGATIVE_X|WM_NEGATIVE_Y);
	height += wmPtr->menuHeight;
	if (childPtr != NULL) {
	    Tk_GeometryRequest((Tk_Window) childPtr, width, height);
	}
	return;
    }
    serial = NextRequest(winPtr->display);
    height += wmPtr->menuHeight;
    if (wmPtr->flags & WM_MOVE_PENDING) {
	if ((x + wmPtr->xInParent == winPtr->changes.x) &&
		(y+wmPtr->yInParent+wmPtr->menuHeight == winPtr->changes.y)
		&& (width == wmPtr->wrapperPtr->changes.width)
		&& (height == wmPtr->wrapperPtr->changes.height)) {
	    /*
	     * The window already has the correct geometry, so don't bother to
	     * configure it; the X server appears to ignore these requests, so
	     * we won't get back a ConfigureNotify and the
	     * WaitForConfigureNotify call below will hang for a while.
	     */

	    wmPtr->flags &= ~WM_MOVE_PENDING;
	    return;
	}
	wmPtr->configWidth = width;
	wmPtr->configHeight = height;
	if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("UpdateGeometryInfo moving to %d %d, resizing to %dx%d,\n",
		    x, y, width, height);
	}
	XMoveResizeWindow(winPtr->display, wmPtr->wrapperPtr->window, x, y,
		(unsigned) width, (unsigned) height);
    } else if ((width != wmPtr->configWidth)
	    || (height != wmPtr->configHeight)) {
	if ((width == wmPtr->wrapperPtr->changes.width)
		&& (height == wmPtr->wrapperPtr->changes.height)) {
	    /*
	     * The window is already just the size we want, so don't bother to
	     * configure it; the X server appears to ignore these requests, so
	     * we won't get back a ConfigureNotify and the
	     * WaitForConfigureNotify call below will hang for a while.
	     */

	    return;
	}
	wmPtr->configWidth = width;
	wmPtr->configHeight = height;
	if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	    printf("UpdateGeometryInfo resizing %p to %d x %d\n",
		    (void *) wmPtr->wrapperPtr->window, width, height);
	}
	XResizeWindow(winPtr->display, wmPtr->wrapperPtr->window,
		(unsigned) width, (unsigned) height);
    } else if ((wmPtr->menubar != NULL)
	    && ((Tk_Width(wmPtr->menubar) != wmPtr->wrapperPtr->changes.width)
	    || (Tk_Height(wmPtr->menubar) != wmPtr->menuHeight))) {
	/*
	 * It is possible that the window's overall size has not changed but
	 * the menu size has.
	 */

	Tk_MoveResizeWindow(wmPtr->menubar, 0, 0,
		wmPtr->wrapperPtr->changes.width, wmPtr->menuHeight);
	XResizeWindow(winPtr->display, wmPtr->wrapperPtr->window,
		(unsigned) width, (unsigned) height);
    } else {
	return;
    }

    /*
     * Wait for the configure operation to complete. Don't need to do this,
     * however, if the window is about to be mapped: it will be taken care of
     * elsewhere.
     */

    if (!(wmPtr->flags & WM_ABOUT_TO_MAP)) {
	WaitForConfigureNotify(winPtr, serial);
    }
}

/*
 *--------------------------------------------------------------
 *
 * UpdateSizeHints --
 *
 *	This function is called to update the window manager's size hints
 *	information from the information in a WmInfo structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Properties get changed for winPtr.
 *
 *--------------------------------------------------------------
 */

static void
UpdateSizeHints(
    TkWindow *winPtr,
    int newWidth,
    int newHeight)
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    XSizeHints *hintsPtr;
    int maxWidth, maxHeight;

    wmPtr->flags &= ~WM_UPDATE_SIZE_HINTS;

    hintsPtr = XAllocSizeHints();
    if (hintsPtr == NULL) {
	return;
    }

    /*
     * Compute the pixel-based sizes for the various fields in the size hints
     * structure, based on the grid-based sizes in our structure.
     */

    GetMaxSize(wmPtr, &maxWidth, &maxHeight);
    if (wmPtr->gridWin != NULL) {
	hintsPtr->base_width = winPtr->reqWidth
		- (wmPtr->reqGridWidth * wmPtr->widthInc);
	if (hintsPtr->base_width < 0) {
	    hintsPtr->base_width = 0;
	}
	hintsPtr->base_height = winPtr->reqHeight + wmPtr->menuHeight
		- (wmPtr->reqGridHeight * wmPtr->heightInc);
	if (hintsPtr->base_height < 0) {
	    hintsPtr->base_height = 0;
	}
	hintsPtr->min_width = hintsPtr->base_width
		+ (wmPtr->minWidth * wmPtr->widthInc);
	hintsPtr->min_height = hintsPtr->base_height
		+ (wmPtr->minHeight * wmPtr->heightInc);
	hintsPtr->max_width = hintsPtr->base_width
		+ (maxWidth * wmPtr->widthInc);
	hintsPtr->max_height = hintsPtr->base_height
		+ (maxHeight * wmPtr->heightInc);
    } else {
	hintsPtr->min_width = wmPtr->minWidth;
	hintsPtr->min_height = wmPtr->minHeight;
	hintsPtr->max_width = maxWidth;
	hintsPtr->max_height = maxHeight;
	hintsPtr->base_width = 0;
	hintsPtr->base_height = 0;
    }
    hintsPtr->width_inc = wmPtr->widthInc;
    hintsPtr->height_inc = wmPtr->heightInc;
    hintsPtr->min_aspect.x = wmPtr->minAspect.x;
    hintsPtr->min_aspect.y = wmPtr->minAspect.y;
    hintsPtr->max_aspect.x = wmPtr->maxAspect.x;
    hintsPtr->max_aspect.y = wmPtr->maxAspect.y;
    hintsPtr->win_gravity = wmPtr->gravity;
    hintsPtr->flags = wmPtr->sizeHintsFlags | PMinSize;

    /*
     * If the window isn't supposed to be resizable, then set the minimum and
     * maximum dimensions to be the same.
     */

    if (wmPtr->flags & WM_WIDTH_NOT_RESIZABLE) {
	hintsPtr->max_width = hintsPtr->min_width = newWidth;
	hintsPtr->flags |= PMaxSize;
    }
    if (wmPtr->flags & WM_HEIGHT_NOT_RESIZABLE) {
	hintsPtr->max_height = hintsPtr->min_height =
		newHeight + wmPtr->menuHeight;
	hintsPtr->flags |= PMaxSize;
    }

    XSetWMNormalHints(winPtr->display, wmPtr->wrapperPtr->window, hintsPtr);

    XFree((char *) hintsPtr);
}

/*
 *--------------------------------------------------------------
 *
 * UpdateTitle --
 *
 *	This function is called to update the window title and icon name. It
 *	sets the ICCCM-defined properties WM_NAME and WM_ICON_NAME for older
 *	window managers, and the freedesktop.org-defined _NET_WM_NAME and
 *	_NET_WM_ICON_NAME properties for newer ones. The ICCCM properties are
 *	stored in the system encoding, the newer properties are stored in
 *	UTF-8.
 *
 *	NOTE: the ICCCM specifies that WM_NAME and WM_ICON_NAME are stored in
 *	ISO-Latin-1. Tk has historically used the default system encoding
 *	(since 8.1). It's not clear whether this is correct or not.
 *
 * Side effects:
 *	Properties get changed for winPtr.
 *
 *--------------------------------------------------------------
 */

static void
UpdateTitle(
    TkWindow *winPtr)
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    Atom XA_UTF8_STRING = Tk_InternAtom((Tk_Window) winPtr, "UTF8_STRING");
    const char *string;
    Tcl_DString ds;

    /*
     * Set window title:
     */

    string = (wmPtr->title != NULL) ? wmPtr->title : winPtr->nameUid;
    Tcl_UtfToExternalDString(NULL, string, -1, &ds);
    XStoreName(winPtr->display, wmPtr->wrapperPtr->window,
	    Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);

    SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_NAME", XA_UTF8_STRING, 8,
	    string, strlen(string));

    /*
     * Set icon name:
     */

    if (wmPtr->iconName != NULL) {
	Tcl_UtfToExternalDString(NULL, wmPtr->iconName, -1, &ds);
	XSetIconName(winPtr->display, wmPtr->wrapperPtr->window,
		Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);

	SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_ICON_NAME",
		XA_UTF8_STRING, 8, wmPtr->iconName, strlen(wmPtr->iconName));
    }
}

/*
 *--------------------------------------------------------------
 *
 * UpdatePhotoIcon --
 *
 *	This function is called to update the window photo icon. It sets the
 *	EWMH-defined properties _NET_WM_ICON.
 *
 * Side effects:
 *	Properties get changed for winPtr.
 *
 *--------------------------------------------------------------
 */

static void
UpdatePhotoIcon(
    TkWindow *winPtr)
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    unsigned char *data = wmPtr->iconDataPtr;
    int size = wmPtr->iconDataSize;

    if (data == NULL) {
	data = winPtr->dispPtr->iconDataPtr;
	size = winPtr->dispPtr->iconDataSize;
    }
    if (data != NULL) {
	SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_ICON", XA_CARDINAL, 32,
		data, size);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetNetWmState --
 *
 * 	Sets the specified state property by sending a _NET_WM_STATE
 * 	ClientMessage to the root window.
 *
 * Preconditions:
 * 	Wrapper window must be created.
 *
 * See also:
 * 	UpdateNetWmState; EWMH spec, section _NET_WM_STATE.
 *
 *----------------------------------------------------------------------
 */

#define _NET_WM_STATE_REMOVE    0l
#define _NET_WM_STATE_ADD       1l
#define _NET_WM_STATE_TOGGLE    2l

static void
SetNetWmState(
    TkWindow *winPtr,
    const char *atomName,
    int on)
{
    Tk_Window tkwin = (Tk_Window) winPtr;
    Atom messageType = Tk_InternAtom(tkwin, "_NET_WM_STATE");
    Atom action = on ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
    Atom property = Tk_InternAtom(tkwin, atomName);
    XEvent e;

    if (!winPtr->wmInfoPtr->wrapperPtr) {
	return;
    }

    e.xany.type = ClientMessage;
    e.xany.window = winPtr->wmInfoPtr->wrapperPtr->window;
    e.xclient.message_type = messageType;
    e.xclient.format = 32;
    e.xclient.data.l[0] = action;
    e.xclient.data.l[1] = property;
    e.xclient.data.l[2] = e.xclient.data.l[3] = e.xclient.data.l[4] = 0l;

    XSendEvent(winPtr->display,
	RootWindow(winPtr->display, winPtr->screenNum), 0,
	SubstructureNotifyMask|SubstructureRedirectMask, &e);
}

/*
 *----------------------------------------------------------------------
 *
 * CheckNetWmState --
 *
 * 	Updates the window attributes whenever the _NET_WM_STATE property
 * 	changes.
 *
 * Notes:
 *
 * 	Tk uses a single -zoomed state, while the EWMH spec supports separate
 * 	vertical and horizontal maximization. We consider the window to be
 * 	"zoomed" if _NET_WM_STATE_MAXIMIZED_VERT and
 * 	_NET_WM_STATE_MAXIMIZED_HORZ are both set.
 *
 *----------------------------------------------------------------------
 */

static void
CheckNetWmState(
    WmInfo *wmPtr,
    Atom *atoms,
    int numAtoms)
{
    Tk_Window tkwin = (Tk_Window) wmPtr->wrapperPtr;
    int i;
    Atom _NET_WM_STATE_ABOVE
	    = Tk_InternAtom(tkwin, "_NET_WM_STATE_ABOVE"),
	_NET_WM_STATE_MAXIMIZED_VERT
	    = Tk_InternAtom(tkwin, "_NET_WM_STATE_MAXIMIZED_VERT"),
	_NET_WM_STATE_MAXIMIZED_HORZ
	    = Tk_InternAtom(tkwin, "_NET_WM_STATE_MAXIMIZED_HORZ"),
	_NET_WM_STATE_FULLSCREEN
	    = Tk_InternAtom(tkwin, "_NET_WM_STATE_FULLSCREEN");

    wmPtr->attributes.topmost = 0;
    wmPtr->attributes.zoomed = 0;
    wmPtr->attributes.fullscreen = 0;
    for (i = 0; i < numAtoms; ++i) {
	if (atoms[i] == _NET_WM_STATE_ABOVE) {
	    wmPtr->attributes.topmost = 1;
	} else if (atoms[i] == _NET_WM_STATE_MAXIMIZED_VERT) {
	    wmPtr->attributes.zoomed |= 1;
	} else if (atoms[i] == _NET_WM_STATE_MAXIMIZED_HORZ) {
	    wmPtr->attributes.zoomed |= 2;
	} else if (atoms[i] == _NET_WM_STATE_FULLSCREEN) {
	    wmPtr->attributes.fullscreen = 1;
	}
    }

    wmPtr->attributes.zoomed = (wmPtr->attributes.zoomed == 3);

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateNetWmState --
 *
 * 	Sets the _NET_WM_STATE property to match the requested attribute state
 * 	just prior to mapping a withdrawn window.
 *
 *----------------------------------------------------------------------
 */

#define NET_WM_STATE_MAX_ATOMS 4

static void
UpdateNetWmState(
    WmInfo *wmPtr)
{
    Tk_Window tkwin = (Tk_Window) wmPtr->wrapperPtr;
    Atom atoms[NET_WM_STATE_MAX_ATOMS];
    long numAtoms = 0;

    if (wmPtr->reqState.topmost) {
	atoms[numAtoms++] = Tk_InternAtom(tkwin,"_NET_WM_STATE_ABOVE");
    }
    if (wmPtr->reqState.zoomed) {
	atoms[numAtoms++] = Tk_InternAtom(tkwin,"_NET_WM_STATE_MAXIMIZED_VERT");
	atoms[numAtoms++] = Tk_InternAtom(tkwin,"_NET_WM_STATE_MAXIMIZED_HORZ");
    }
    if (wmPtr->reqState.fullscreen) {
	atoms[numAtoms++] = Tk_InternAtom(tkwin, "_NET_WM_STATE_FULLSCREEN");
    }

    SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_STATE", XA_ATOM, 32, atoms,
	    numAtoms);
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForConfigureNotify --
 *
 *	This function is invoked in order to synchronize with the window
 *	manager. It waits for a ConfigureNotify event to arrive, signalling
 *	that the window manager has seen an attempt on our part to move or
 *	resize a top-level window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Delays the execution of the process until a ConfigureNotify event
 *	arrives with serial number at least as great as serial. This is useful
 *	for two reasons:
 *
 *	1. It's important to distinguish ConfigureNotify events that are
 *	   coming in response to a request we've made from those generated
 *	   spontaneously by the user. The reason for this is that if the user
 *	   resizes the window we take that as an order to ignore geometry
 *	   requests coming from inside the window hierarchy. If we
 *	   accidentally interpret a response to our request as a user-
 *	   initiated action, the window will stop responding to new geometry
 *	   requests. To make this distinction, (a) this function sets a flag
 *	   for TopLevelEventProc to indicate that we're waiting to sync with
 *	   the wm, and (b) all changes to the size of a top-level window are
 *	   followed by calls to this function.
 *	2. Races and confusion can come about if there are multiple operations
 *	   outstanding at a time (e.g. two different resizes of the top-level
 *	   window: it's hard to tell which of the ConfigureNotify events
 *	   coming back is for which request).
 *	While waiting, some events covered by StructureNotifyMask are
 *	processed (ConfigureNotify, MapNotify, and UnmapNotify) and all others
 *	are deferred.
 *
 *----------------------------------------------------------------------
 */

static void
WaitForConfigureNotify(
    TkWindow *winPtr,		/* Top-level window for which we want to see a
				 * ConfigureNotify. */
    unsigned long serial)	/* Serial number of resize request. Want to be
				 * sure wm has seen this. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    XEvent event;
    int diff, code;
    int gotConfig = 0;

    /*
     * One more tricky detail about this function. In some cases the window
     * manager will decide to ignore a configure request (e.g. because it
     * thinks the window is already in the right place). To avoid hanging in
     * this situation, only wait for a few seconds, then give up.
     */

    while (!gotConfig) {
	wmPtr->flags |= WM_SYNC_PENDING;
	code = WaitForEvent(winPtr->display, wmPtr, ConfigureNotify, &event);
	wmPtr->flags &= ~WM_SYNC_PENDING;
	if (code != TCL_OK) {
	    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
		printf("WaitForConfigureNotify giving up on %s\n",
			winPtr->pathName);
	    }
	    break;
	}
	diff = event.xconfigure.serial - serial;
	if (diff >= 0) {
	    gotConfig = 1;
	}
    }
    wmPtr->flags &= ~WM_MOVE_PENDING;
    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("WaitForConfigureNotify finished with %s, serial %ld\n",
		winPtr->pathName, serial);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForEvent --
 *
 *	This function is used by WaitForConfigureNotify and WaitForMapNotify
 *	to wait for an event of a certain type to arrive.
 *
 * Results:
 *	Under normal conditions, TCL_OK is returned and an event for display
 *	and window that matches "mask" is stored in *eventPtr. This event has
 *	already been processed by Tk before this function returns. If a long
 *	time goes by with no event of the right type arriving, or if an error
 *	occurs while waiting for the event to arrive, then TCL_ERROR is
 *	returned.
 *
 * Side effects:
 *	While waiting for the desired event to occur, Configurenotify,
 *	MapNotify, and UnmapNotify events for window are processed, as are all
 *	ReparentNotify events.
 *
 *----------------------------------------------------------------------
 */

static int
WaitForEvent(
    Display *display,		/* Display event is coming from. */
    WmInfo *wmInfoPtr,		/* Window for which event is desired. */
    int type,			/* Type of event that is wanted. */
    XEvent *eventPtr)		/* Place to store event. */
{
    WaitRestrictInfo info;
    Tk_RestrictProc *prevProc;
    ClientData prevArg;
    Tcl_Time timeout;

    /*
     * Set up an event filter to select just the events we want, and a timer
     * handler, then wait for events until we get the event we want or a
     * timeout happens.
     */

    info.display = display;
    info.wmInfoPtr = wmInfoPtr;
    info.type = type;
    info.eventPtr = eventPtr;
    info.foundEvent = 0;
    prevProc = Tk_RestrictEvents(WaitRestrictProc, &info, &prevArg);

    Tcl_GetTime(&timeout);
    timeout.sec += 2;

    while (!info.foundEvent) {
	if (!TkUnixDoOneXEvent(&timeout)) {
	    break;
	}
    }
    Tk_RestrictEvents(prevProc, prevArg, &prevArg);
    if (info.foundEvent) {
	return TCL_OK;
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * WaitRestrictProc --
 *
 *	This function is a Tk_RestrictProc that is used to filter events while
 *	WaitForEvent is active.
 *
 * Results:
 *	Returns TK_PROCESS_EVENT if the right event is found. Also returns
 *	TK_PROCESS_EVENT if any ReparentNotify event is found or if the event
 *	is a ConfigureNotify, MapNotify, or UnmapNotify for window. Otherwise
 *	returns TK_DEFER_EVENT.
 *
 * Side effects:
 *	An event may get stored in the area indicated by the caller of
 *	WaitForEvent.
 *
 *----------------------------------------------------------------------
 */

static Tk_RestrictAction
WaitRestrictProc(
    ClientData clientData,	/* Pointer to WaitRestrictInfo structure. */
    XEvent *eventPtr)		/* Event that is about to be handled. */
{
    WaitRestrictInfo *infoPtr = clientData;

    if (eventPtr->type == ReparentNotify) {
	return TK_PROCESS_EVENT;
    }
    if (((eventPtr->xany.window != infoPtr->wmInfoPtr->wrapperPtr->window)
	    && (eventPtr->xany.window != infoPtr->wmInfoPtr->reparent))
	    || (eventPtr->xany.display != infoPtr->display)) {
	return TK_DEFER_EVENT;
    }
    if (eventPtr->type == infoPtr->type) {
	*infoPtr->eventPtr = *eventPtr;
	infoPtr->foundEvent = 1;
	return TK_PROCESS_EVENT;
    }
    if (eventPtr->type == ConfigureNotify || eventPtr->type == MapNotify
	    || eventPtr->type == UnmapNotify) {
	return TK_PROCESS_EVENT;
    }
    return TK_DEFER_EVENT;
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForMapNotify --
 *
 *	This function is invoked in order to synchronize with the window
 *	manager. It waits for the window's mapped state to reach the value
 *	given by mapped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Delays the execution of the process until winPtr becomes mapped or
 *	unmapped, depending on the "mapped" argument. This allows us to
 *	synchronize with the window manager, and allows us to identify changes
 *	in window size that come about when the window manager first starts
 *	managing the window (as opposed to those requested interactively by
 *	the user later). See the comments for WaitForConfigureNotify and
 *	WM_SYNC_PENDING. While waiting, some events covered by
 *	StructureNotifyMask are processed and all others are deferred.
 *
 *----------------------------------------------------------------------
 */

static void
WaitForMapNotify(
    TkWindow *winPtr,		/* Top-level window for which we want to see a
				 * particular mapping state. */
    int mapped)			/* If non-zero, wait for window to become
				 * mapped, otherwise wait for it to become
				 * unmapped. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;
    XEvent event;
    int code;

    while (1) {
	if (mapped) {
	    if (winPtr->flags & TK_MAPPED) {
		break;
	    }
	} else if (!(winPtr->flags & TK_MAPPED)) {
	    break;
	}
	wmPtr->flags |= WM_SYNC_PENDING;
	code = WaitForEvent(winPtr->display, wmPtr,
		mapped ? MapNotify : UnmapNotify, &event);
	wmPtr->flags &= ~WM_SYNC_PENDING;
	if (code != TCL_OK) {
	    /*
	     * There are some bizarre situations in which the window manager
	     * can't respond or chooses not to (e.g. if we've got a grab set
	     * it can't respond). If this happens then just quit.
	     */

	    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
		printf("WaitForMapNotify giving up on %s\n", winPtr->pathName);
	    }
	    break;
	}
    }
    wmPtr->flags &= ~WM_MOVE_PENDING;
    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("WaitForMapNotify finished with %s (winPtr %p, wmPtr %p)\n",
		winPtr->pathName, winPtr, wmPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * UpdateHints --
 *
 *	This function is called to update the window manager's hints
 *	information from the information in a WmInfo structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Properties get changed for winPtr.
 *
 *--------------------------------------------------------------
 */

static void
UpdateHints(
    TkWindow *winPtr)
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (wmPtr->flags & WM_NEVER_MAPPED) {
	return;
    }
    XSetWMHints(winPtr->display, wmPtr->wrapperPtr->window, &wmPtr->hints);
}

/*
 *----------------------------------------------------------------------
 *
 * SetNetWmType --
 *
 *	Set the extended window manager hints for a toplevel window to the
 *	types provided. The specification states that this may be a list of
 *	window types in preferred order. To permit for future type
 *	definitions, the set of names is unconstrained and names are converted
 *	to upper-case and appended to "_NET_WM_WINDOW_TYPE_" before being
 *	converted to an Atom.
 *
 *----------------------------------------------------------------------
 */

static int
SetNetWmType(
    TkWindow *winPtr,
    Tcl_Obj *typePtr)
{
    Atom *atoms = NULL;
    WmInfo *wmPtr;
    Tcl_Obj **objv;
    int objc, n;
    Tk_Window tkwin = (Tk_Window) winPtr;
    Tcl_Interp *interp = Tk_Interp(tkwin);

    if (TCL_OK != Tcl_ListObjGetElements(interp, typePtr, &objc, &objv)) {
	return TCL_ERROR;
    }

    if (!Tk_HasWrapper(tkwin)) {
	return TCL_OK; /* error?? */
    }

    if (objc > 0) {
	atoms = ckalloc(sizeof(Atom) * objc);
    }

    for (n = 0; n < objc; ++n) {
	Tcl_DString ds, dsName;
	int len;
	char *name = Tcl_GetStringFromObj(objv[n], &len);

	Tcl_UtfToUpper(name);
	Tcl_UtfToExternalDString(NULL, name, len, &dsName);
	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, "_NET_WM_WINDOW_TYPE_", 20);
	Tcl_DStringAppend(&ds, Tcl_DStringValue(&dsName),
		Tcl_DStringLength(&dsName));
	Tcl_DStringFree(&dsName);
	atoms[n] = Tk_InternAtom(tkwin, Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
    }

    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr->wrapperPtr == NULL) {
	CreateWrapper(wmPtr);
    }

    SetWindowProperty(wmPtr->wrapperPtr, "_NET_WM_WINDOW_TYPE", XA_ATOM, 32,
	    atoms, objc);

    ckfree(atoms);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetNetWmType --
 *
 *	Read the extended window manager type hint from a window and return as
 *	a list of names suitable for use with SetNetWmType.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
GetNetWmType(
    TkWindow *winPtr)
{
    Atom typeAtom, actualType, *atoms;
    int actualFormat;
    unsigned long n, count, bytesAfter;
    unsigned char *propertyValue = NULL;
    long maxLength = 1024;
    Tk_Window tkwin = (Tk_Window) winPtr;
    TkWindow *wrapperPtr;
    Tcl_Obj *typePtr;
    Tcl_Interp *interp;
    Tcl_DString ds;

    interp = Tk_Interp(tkwin);
    typePtr = Tcl_NewListObj(0, NULL);

    if (winPtr->wmInfoPtr->wrapperPtr == NULL) {
	CreateWrapper(winPtr->wmInfoPtr);
    }
    wrapperPtr = winPtr->wmInfoPtr->wrapperPtr;

    typeAtom = Tk_InternAtom(tkwin, "_NET_WM_WINDOW_TYPE");
    if (GetWindowProperty(wrapperPtr, typeAtom, maxLength, XA_ATOM,
	    &actualType, &actualFormat, &count, &bytesAfter, &propertyValue)){
	atoms = (Atom *) propertyValue;
	for (n = 0; n < count; ++n) {
	    const char *name = Tk_GetAtomName(tkwin, atoms[n]);

	    if (strncmp("_NET_WM_WINDOW_TYPE_", name, 20) == 0) {
		Tcl_ExternalToUtfDString(NULL, name+20, -1, &ds);
		Tcl_UtfToLower(Tcl_DStringValue(&ds));
		Tcl_ListObjAppendElement(interp, typePtr,
			Tcl_NewStringObj(Tcl_DStringValue(&ds),
				Tcl_DStringLength(&ds)));
		Tcl_DStringFree(&ds);
	    }
	}
	XFree(propertyValue);
    }

    return typePtr;
}

/*
 *--------------------------------------------------------------
 *
 * ParseGeometry --
 *
 *	This function parses a geometry string and updates information used to
 *	control the geometry of a top-level window.
 *
 * Results:
 *	A standard Tcl return value, plus an error message in the interp's
 *	result if an error occurs.
 *
 * Side effects:
 *	The size and/or location of winPtr may change.
 *
 *--------------------------------------------------------------
 */

static int
ParseGeometry(
    Tcl_Interp *interp,		/* Used for error reporting. */
    const char *string,		/* String containing new geometry. Has the
				 * standard form "=wxh+x+y". */
    TkWindow *winPtr)		/* Pointer to top-level window whose geometry
				 * is to be changed. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    int x, y, width, height, flags;
    char *end;
    register const char *p = string;

    /*
     * The leading "=" is optional.
     */

    if (*p == '=') {
	p++;
    }

    /*
     * Parse the width and height, if they are present. Don't actually update
     * any of the fields of wmPtr until we've successfully parsed the entire
     * geometry string.
     */

    width = wmPtr->width;
    height = wmPtr->height;
    x = wmPtr->x;
    y = wmPtr->y;
    flags = wmPtr->flags;
    if (isdigit(UCHAR(*p))) {
	width = strtoul(p, &end, 10);
	p = end;
	if (*p != 'x') {
	    goto error;
	}
	p++;
	if (!isdigit(UCHAR(*p))) {
	    goto error;
	}
	height = strtoul(p, &end, 10);
	p = end;
    }

    /*
     * Parse the X and Y coordinates, if they are present.
     */

    if (*p != '\0') {
	flags &= ~(WM_NEGATIVE_X | WM_NEGATIVE_Y);
	if (*p == '-') {
	    flags |= WM_NEGATIVE_X;
	} else if (*p != '+') {
	    goto error;
	}
	p++;
	if (!isdigit(UCHAR(*p)) && (*p != '-')) {
	    goto error;
	}
	x = strtol(p, &end, 10);
	p = end;
	if (*p == '-') {
	    flags |= WM_NEGATIVE_Y;
	} else if (*p != '+') {
	    goto error;
	}
	p++;
	if (!isdigit(UCHAR(*p)) && (*p != '-')) {
	    goto error;
	}
	y = strtol(p, &end, 10);
	if (*end != '\0') {
	    goto error;
	}

	/*
	 * Assume that the geometry information came from the user, unless an
	 * explicit source has been specified. Otherwise most window managers
	 * assume that the size hints were program-specified and they ignore
	 * them.
	 */

	if (!(wmPtr->sizeHintsFlags & (USPosition|PPosition))) {
	    wmPtr->sizeHintsFlags |= USPosition;
	    flags |= WM_UPDATE_SIZE_HINTS;
	}
    }

    /*
     * Everything was parsed OK. Update the fields of *wmPtr and arrange for
     * the appropriate information to be percolated out to the window manager
     * at the next idle moment.
     */

    wmPtr->width = width;
    wmPtr->height = height;
    wmPtr->x = x;
    wmPtr->y = y;
    flags |= WM_MOVE_PENDING;
    wmPtr->flags = flags;

    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
    return TCL_OK;

  error:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "bad geometry specifier \"%s\"", string));
    Tcl_SetErrorCode(interp, "TK", "VALUE", "GEOMETRY", NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetRootCoords --
 *
 *	Given a token for a window, this function traces through the window's
 *	lineage to find the (virtual) root-window coordinates corresponding to
 *	point (0,0) in the window.
 *
 * Results:
 *	The locations pointed to by xPtr and yPtr are filled in with the root
 *	coordinates of the (0,0) point in tkwin. If a virtual root window is
 *	in effect for the window, then the coordinates in the virtual root are
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_GetRootCoords(
    Tk_Window tkwin,		/* Token for window. */
    int *xPtr,			/* Where to store x-displacement of (0,0). */
    int *yPtr)			/* Where to store y-displacement of (0,0). */
{
    int x, y;
    register TkWindow *winPtr = (TkWindow *) tkwin;

    /*
     * Search back through this window's parents all the way to a top-level
     * window, combining the offsets of each window within its parent.
     */

    x = y = 0;
    while (1) {
	x += winPtr->changes.x + winPtr->changes.border_width;
	y += winPtr->changes.y + winPtr->changes.border_width;
	if ((winPtr->wmInfoPtr != NULL)
		&& (winPtr->wmInfoPtr->menubar == (Tk_Window) winPtr)) {
	    /*
	     * This window is a special menubar; switch over to its associated
	     * toplevel, compensate for their differences in y coordinates,
	     * then continue with the toplevel (in case it's embedded).
	     */

	    y -= winPtr->wmInfoPtr->menuHeight;
	    winPtr = winPtr->wmInfoPtr->winPtr;
	    continue;
	}
	if (winPtr->flags & TK_TOP_LEVEL) {
	    TkWindow *otherPtr;

	    if (!(winPtr->flags & TK_EMBEDDED)) {
		break;
	    }
	    otherPtr = TkpGetOtherWindow(winPtr);
	    if (otherPtr == NULL) {
		/*
		 * The container window is not in the same application. Query
		 * the X server.
		 */

		Window root, dummyChild;
		int rootX, rootY;

		root = winPtr->wmInfoPtr->vRoot;
		if (root == None) {
		    root = RootWindowOfScreen(Tk_Screen((Tk_Window) winPtr));
		}
		XTranslateCoordinates(winPtr->display, winPtr->window,
			root, 0, 0, &rootX, &rootY, &dummyChild);
		x += rootX;
		y += rootY;
		break;
	    } else {
		/*
		 * The container window is in the same application. Let's
		 * query its coordinates.
		 */

		winPtr = otherPtr;
		continue;
	    }
	}
	winPtr = winPtr->parentPtr;
	if (winPtr == NULL) {
	    break;
	}
    }
    *xPtr = x;
    *yPtr = y;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CoordsToWindow --
 *
 *	Given the (virtual) root coordinates of a point, this function returns
 *	the token for the top-most window covering that point, if there exists
 *	such a window in this application.
 *
 * Results:
 *	The return result is either a token for the window corresponding to
 *	rootX and rootY, or else NULL to indicate that there is no such
 *	window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int PointInWindow(
    int x,
    int y,
    WmInfo *wmPtr)
{
    XWindowChanges changes = wmPtr->winPtr->changes;
    return (x >= changes.x &&
            x < changes.x + changes.width &&
            y >= changes.y - wmPtr->menuHeight &&
            y < changes.y + changes.height);
}

Tk_Window
Tk_CoordsToWindow(
    int rootX, int rootY,	/* Coordinates of point in root window. If a
				 * virtual-root window manager is in use,
				 * these coordinates refer to the virtual
				 * root, not the real root. */
    Tk_Window tkwin)		/* Token for any window in application; used
				 * to identify the display. */
{
    Window window, parent, child;
    int x, y, childX, childY, tmpx, tmpy, bd;
    WmInfo *wmPtr;
    TkWindow *winPtr, *childPtr, *nextPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    Tk_ErrorHandler handler = NULL;

    /*
     * Step 1: scan the list of toplevel windows to see if there is a virtual
     * root for the screen we're interested in. If so, we have to translate
     * the coordinates from virtual root to root coordinates.
     */

    parent = window = RootWindowOfScreen(Tk_Screen(tkwin));
    x = rootX;
    y = rootY;
    for (wmPtr = (WmInfo *) dispPtr->firstWmPtr; wmPtr != NULL;
	    wmPtr = wmPtr->nextPtr) {
	if (Tk_Screen(wmPtr->winPtr) != Tk_Screen(tkwin)) {
	    continue;
	}
	if (wmPtr->vRoot == None) {
	    continue;
	}
	UpdateVRootGeometry(wmPtr);
	parent = wmPtr->vRoot;
	break;
    }

    /*
     * Step 2: work down through the window hierarchy starting at the root.
     * For each window, find the child that contains the given point and then
     * see if this child is either a wrapper for one of our toplevel windows
     * or a window manager decoration window for one of our toplevels. This
     * approach handles several tricky cases:
     *
     * 1. There may be a virtual root window between the root and one of our
     *    toplevels.
     * 2. If a toplevel is embedded, we may have to search through the
     *    windows of the container application(s) before getting to the
     *    toplevel.
     */

    handler = Tk_CreateErrorHandler(Tk_Display(tkwin), -1, -1, -1, NULL, NULL);
    while (1) {
	if (XTranslateCoordinates(Tk_Display(tkwin), parent, window,
		x, y, &childX, &childY, &child) == False) {
	    /*
	     * We can end up here when the window is in the middle of being
	     * deleted
	     */

	    Tk_DeleteErrorHandler(handler);
	    return NULL;
	}
	if (child == None) {
	    Tk_DeleteErrorHandler(handler);
	    return NULL;
	}
	for (wmPtr = (WmInfo *) dispPtr->firstWmPtr; wmPtr != NULL;
		wmPtr = wmPtr->nextPtr) {
            if (wmPtr->winPtr->mainPtr == NULL) {
                continue;
            }
	    if (child == wmPtr->reparent) {
                if (PointInWindow(x, y, wmPtr)) {
                    goto gotToplevel;
                } else {

                    /*
                     * Return NULL if the point is in the title bar or border.
                     */

                    return NULL;
                }
	    }
	    if (wmPtr->wrapperPtr != NULL) {
		if (child == wmPtr->wrapperPtr->window) {
		    goto gotToplevel;
		} else if (wmPtr->winPtr->flags & TK_EMBEDDED &&
                           TkpGetOtherWindow(wmPtr->winPtr) == NULL) {

                    /*
                     * This toplevel is embedded in a window belonging to
                     * a different application.
                     */

                    int rx, ry;
                    Tk_GetRootCoords((Tk_Window) wmPtr->winPtr, &rx, &ry);
                    childX -= rx;
                    childY -= ry;
                    goto gotToplevel;
                }
	    } else if (child == wmPtr->winPtr->window) {
		goto gotToplevel;
	    }
	}
	x = childX;
	y = childY;
	parent = window;
	window = child;
    }

  gotToplevel:
    if (handler) {
	/*
	 * Check value of handler, because we can reach this label from above
	 * or below
	 */

	Tk_DeleteErrorHandler(handler);
	handler = NULL;
    }
    winPtr = wmPtr->winPtr;

    /*
     * Step 3: at this point winPtr and wmPtr refer to the toplevel that
     * contains the given coordinates, and childX and childY give the
     * translated coordinates in the *parent* of the toplevel. Now decide
     * whether the coordinates are in the menubar or the actual toplevel, and
     * translate the coordinates into the coordinate system of that window.
     */

    x = childX - winPtr->changes.x;
    y = childY - winPtr->changes.y;
    if ((x < 0) || (x >= winPtr->changes.width)
	    || (y >= winPtr->changes.height)) {
	return NULL;
    }
    if (y < 0) {
	winPtr = (TkWindow *) wmPtr->menubar;
	if (winPtr == NULL) {
	    return NULL;
	}
	y += wmPtr->menuHeight;
	if (y < 0) {
	    return NULL;
	}
    }

    /*
     * Step 4: work down through the hierarchy underneath the current window.
     * At each level, scan through all the children to find the highest one in
     * the stacking order that contains the point. Then repeat the whole
     * process on that child.
     */

    while (1) {
	nextPtr = NULL;
	for (childPtr = winPtr->childList; childPtr != NULL;
		childPtr = childPtr->nextPtr) {
	    if (!Tk_IsMapped(childPtr)
		    || (childPtr->flags & TK_TOP_HIERARCHY)) {
		continue;
	    }
	    if (childPtr->flags & TK_REPARENTED) {
		continue;
	    }
	    tmpx = x - childPtr->changes.x;
	    tmpy = y - childPtr->changes.y;
	    bd = childPtr->changes.border_width;
	    if ((tmpx >= -bd) && (tmpy >= -bd)
		    && (tmpx < (childPtr->changes.width + bd))
		    && (tmpy < (childPtr->changes.height + bd))) {
		nextPtr = childPtr;
	    }
	}
	if (nextPtr == NULL) {
	    break;
	}
	x -= nextPtr->changes.x;
	y -= nextPtr->changes.y;
	if ((nextPtr->flags & TK_CONTAINER)
		&& (nextPtr->flags & TK_BOTH_HALVES)) {
	    /*
	     * The window containing the point is a container, and the
	     * embedded application is in this same process. Switch over to
	     * the toplevel for the embedded application and start processing
	     * that toplevel from scratch.
	     */
	    winPtr = TkpGetOtherWindow(nextPtr);
	    if (winPtr == NULL) {
		return (Tk_Window) nextPtr;
	    }
	    wmPtr = winPtr->wmInfoPtr;
	    childX = x;
	    childY = y;
	    goto gotToplevel;
	} else {
            winPtr = nextPtr;
        }
    }
    if (winPtr->mainPtr != ((TkWindow *) tkwin)->mainPtr) {
        return NULL;
    }
    return (Tk_Window) winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateVRootGeometry --
 *
 *	This function is called to update all the virtual root geometry
 *	information in wmPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The vRootX, vRootY, vRootWidth, and vRootHeight fields in wmPtr are
 *	filled with the most up-to-date information.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateVRootGeometry(
    WmInfo *wmPtr)		/* Window manager information to be updated.
				 * The wmPtr->vRoot field must be valid. */
{
    TkWindow *winPtr = wmPtr->winPtr;
    int bd;
    unsigned dummy;
    Window dummy2;
    Status status;
    Tk_ErrorHandler handler;

    /*
     * If this isn't a virtual-root window manager, just return information
     * about the screen.
     */

    wmPtr->flags &= ~WM_VROOT_OFFSET_STALE;
    if (wmPtr->vRoot == None) {
    noVRoot:
	wmPtr->vRootX = wmPtr->vRootY = 0;
	wmPtr->vRootWidth = DisplayWidth(winPtr->display, winPtr->screenNum);
	wmPtr->vRootHeight = DisplayHeight(winPtr->display, winPtr->screenNum);
	return;
    }

    /*
     * Refresh the virtual root information if it's out of date.
     */

    handler = Tk_CreateErrorHandler(winPtr->display, -1, -1, -1, NULL, NULL);
    status = XGetGeometry(winPtr->display, wmPtr->vRoot,
	    &dummy2, &wmPtr->vRootX, &wmPtr->vRootY,
	    (unsigned *) &wmPtr->vRootWidth,
	    (unsigned *) &wmPtr->vRootHeight, (unsigned *) &bd,
	    &dummy);
    if (winPtr->dispPtr->flags & TK_DISPLAY_WM_TRACING) {
	printf("UpdateVRootGeometry: x = %d, y = %d, width = %d, ",
		wmPtr->vRootX, wmPtr->vRootY, wmPtr->vRootWidth);
	printf("height = %d, status = %d\n", wmPtr->vRootHeight, status);
    }
    Tk_DeleteErrorHandler(handler);
    if (status == 0) {
	/*
	 * The virtual root is gone! Pretend that it never existed.
	 */

	wmPtr->vRoot = None;
	goto noVRoot;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetVRootGeometry --
 *
 *	This function returns information about the virtual root window
 *	corresponding to a particular Tk window.
 *
 * Results:
 *	The values at xPtr, yPtr, widthPtr, and heightPtr are set with the
 *	offset and dimensions of the root window corresponding to tkwin. If
 *	tkwin is being managed by a virtual root window manager these values
 *	correspond to the virtual root window being used for tkwin; otherwise
 *	the offsets will be 0 and the dimensions will be those of the screen.
 *
 * Side effects:
 *	Vroot window information is refreshed if it is out of date.
 *
 *----------------------------------------------------------------------
 */

void
Tk_GetVRootGeometry(
    Tk_Window tkwin,		/* Window whose virtual root is to be
				 * queried. */
    int *xPtr, int *yPtr,	/* Store x and y offsets of virtual root
				 * here. */
    int *widthPtr, int *heightPtr)
				/* Store dimensions of virtual root here. */
{
    WmInfo *wmPtr;
    TkWindow *winPtr = (TkWindow *) tkwin;

    /*
     * Find the top-level window for tkwin, and locate the window manager
     * information for that window.
     */

    while (!(winPtr->flags & TK_TOP_HIERARCHY)
	    && (winPtr->parentPtr != NULL)) {
	winPtr = winPtr->parentPtr;
    }
    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	/* Punt. */
	*xPtr = 0;
	*yPtr = 0;
	*widthPtr = 0;
	*heightPtr = 0;
    }

    /*
     * Make sure that the geometry information is up-to-date, then copy it out
     * to the caller.
     */

    if (wmPtr->flags & WM_VROOT_OFFSET_STALE) {
	UpdateVRootGeometry(wmPtr);
    }
    *xPtr = wmPtr->vRootX;
    *yPtr = wmPtr->vRootY;
    *widthPtr = wmPtr->vRootWidth;
    *heightPtr = wmPtr->vRootHeight;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MoveToplevelWindow --
 *
 *	This function is called instead of Tk_MoveWindow to adjust the x-y
 *	location of a top-level window. It delays the actual move to a later
 *	time and keeps window-manager information up-to-date with the move
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is eventually moved so that its upper-left corner
 *	(actually, the upper-left corner of the window's decorative frame, if
 *	there is one) is at (x,y).
 *
 *----------------------------------------------------------------------
 */

void
Tk_MoveToplevelWindow(
    Tk_Window tkwin,		/* Window to move. */
    int x, int y)		/* New location for window (within parent). */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	Tcl_Panic("Tk_MoveToplevelWindow called with non-toplevel window");
    }
    wmPtr->x = x;
    wmPtr->y = y;
    wmPtr->flags |= WM_MOVE_PENDING;
    wmPtr->flags &= ~(WM_NEGATIVE_X|WM_NEGATIVE_Y);
    if (!(wmPtr->sizeHintsFlags & (USPosition|PPosition))) {
	wmPtr->sizeHintsFlags |= USPosition;
	wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    }

    /*
     * If the window has already been mapped, must bring its geometry
     * up-to-date immediately, otherwise an event might arrive from the server
     * that would overwrite wmPtr->x and wmPtr->y and lose the new position.
     */

    if (!(wmPtr->flags & WM_NEVER_MAPPED)) {
	if (wmPtr->flags & WM_UPDATE_PENDING) {
	    Tcl_CancelIdleCall(UpdateGeometryInfo, winPtr);
	}
	UpdateGeometryInfo(winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateWmProtocols --
 *
 *	This function transfers the most up-to-date information about window
 *	manager protocols from the WmInfo structure to the actual property on
 *	the top-level window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The WM_PROTOCOLS property gets changed for wmPtr's window.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateWmProtocols(
    register WmInfo *wmPtr)	/* Information about top-level window. */
{
    register ProtocolHandler *protPtr;
    Atom deleteWindowAtom, pingAtom;
    int count;
    Atom *arrayPtr, *atomPtr;

    /*
     * There are only two tricky parts here. First, there could be any number
     * of atoms for the window, so count them and malloc an array to hold all
     * of their atoms. Second, we *always* want to respond to the
     * WM_DELETE_WINDOW and _NET_WM_PING protocols, even if no-one's
     * officially asked.
     */

    for (protPtr = wmPtr->protPtr, count = 2; protPtr != NULL;
	    protPtr = protPtr->nextPtr, count++) {
	/* Empty loop body; we're just counting the handlers. */
    }
    arrayPtr = ckalloc(count * sizeof(Atom));
    deleteWindowAtom = Tk_InternAtom((Tk_Window) wmPtr->winPtr,
	    "WM_DELETE_WINDOW");
    pingAtom = Tk_InternAtom((Tk_Window) wmPtr->winPtr, "_NET_WM_PING");
    arrayPtr[0] = deleteWindowAtom;
    arrayPtr[1] = pingAtom;
    for (protPtr = wmPtr->protPtr, atomPtr = &arrayPtr[1];
	    protPtr != NULL; protPtr = protPtr->nextPtr) {
	if (protPtr->protocol != deleteWindowAtom
		&& protPtr->protocol != pingAtom) {
	    *(atomPtr++) = protPtr->protocol;
	}
    }
    SetWindowProperty(wmPtr->wrapperPtr, "WM_PROTOCOLS", XA_ATOM, 32,
	    arrayPtr, atomPtr-arrayPtr);
    ckfree(arrayPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmProtocolEventProc --
 *
 *	This function is called by the Tk_HandleEvent whenever a ClientMessage
 *	event arrives whose type is "WM_PROTOCOLS". This function handles the
 *	message from the window manager in an appropriate fashion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what sort of handler, if any, was set up for the protocol.
 *
 *----------------------------------------------------------------------
 */

void
TkWmProtocolEventProc(
    TkWindow *winPtr,		/* Window to which the event was sent. */
    XEvent *eventPtr)		/* X event. */
{
    WmInfo *wmPtr;
    register ProtocolHandler *protPtr;
    Atom protocol;
    int result;
    const char *protocolName;
    Tcl_Interp *interp;

    protocol = (Atom) eventPtr->xclient.data.l[0];

    /*
     * If this is a _NET_WM_PING message, send it back to the root window
     * immediately. We do that here because scripts *cannot* respond correctly
     * to this protocol.
     */

    if (protocol == Tk_InternAtom((Tk_Window) winPtr, "_NET_WM_PING")) {
	Window root = XRootWindow(winPtr->display, winPtr->screenNum);

	eventPtr->xclient.window = root;
	(void) XSendEvent(winPtr->display, root, False,
		(SubstructureNotifyMask|SubstructureRedirectMask), eventPtr);
	return;
    }

    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }

    /*
     * Note: it's very important to retrieve the protocol name now, before
     * invoking the command, even though the name won't be used until after
     * the command returns. This is because the command could delete winPtr,
     * making it impossible for us to use it later in the call to
     * Tk_GetAtomName.
     */

    protocolName = Tk_GetAtomName((Tk_Window) winPtr, protocol);
    for (protPtr = wmPtr->protPtr; protPtr != NULL;
	    protPtr = protPtr->nextPtr) {
	if (protocol == protPtr->protocol) {
	    Tcl_Preserve(protPtr);
	    interp = protPtr->interp;
	    Tcl_Preserve(interp);
	    result = Tcl_EvalEx(interp, protPtr->command, -1, TCL_EVAL_GLOBAL);
	    if (result != TCL_OK) {
		Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
			"\n    (command for \"%s\" window manager protocol)",
			protocolName));
		Tcl_BackgroundException(interp, result);
	    }
	    Tcl_Release(interp);
	    Tcl_Release(protPtr);
	    return;
	}
    }

    /*
     * No handler was present for this protocol. If this is a WM_DELETE_WINDOW
     * message then just destroy the window.
     */

    if (protocol == Tk_InternAtom((Tk_Window) winPtr, "WM_DELETE_WINDOW")) {
	Tk_DestroyWindow((Tk_Window) wmPtr->winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmStackorderToplevelWrapperMap --
 *
 *	This function will create a table that maps the reparent wrapper X id
 *	for a toplevel to the TkWindow structure that is wraps. Tk keeps track
 *	of a mapping from the window X id to the TkWindow structure but that
 *	does us no good here since we only get the X id of the wrapper window.
 *	Only those toplevel windows that are mapped have a position in the
 *	stacking order.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds entries to the passed hashtable.
 *
 *----------------------------------------------------------------------
 */

static void
TkWmStackorderToplevelWrapperMap(
    TkWindow *winPtr,		/* TkWindow to recurse on */
    Display *display,		/* X display of parent window */
    Tcl_HashTable *table)	/* Maps X id to TkWindow */
{
    TkWindow *childPtr;

    if (Tk_IsMapped(winPtr) && Tk_IsTopLevel(winPtr) &&
	    !Tk_IsEmbedded(winPtr) && (winPtr->display == display)) {
	Window wrapper = (winPtr->wmInfoPtr->reparent != None)
		? winPtr->wmInfoPtr->reparent
		: winPtr->wmInfoPtr->wrapperPtr->window;
	Tcl_HashEntry *hPtr;
	int newEntry;

	hPtr = Tcl_CreateHashEntry(table, (char *) wrapper, &newEntry);
	Tcl_SetHashValue(hPtr, winPtr);
    }

    for (childPtr = winPtr->childList; childPtr != NULL;
	    childPtr = childPtr->nextPtr) {
	TkWmStackorderToplevelWrapperMap(childPtr, display, table);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmStackorderToplevel --
 *
 *	This function returns the stack order of toplevel windows.
 *
 * Results:
 *	An array of pointers to tk window objects in stacking order or else
 *	NULL if there was an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow **
TkWmStackorderToplevel(
    TkWindow *parentPtr)	/* Parent toplevel window. */
{
    Window dummy1, dummy2, vRoot;
    Window *children;
    unsigned numChildren, i;
    TkWindow *childWinPtr, **windows, **window_ptr;
    Tcl_HashTable table;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    /*
     * Map X Window ids to a TkWindow of the wrapped toplevel.
     */

    Tcl_InitHashTable(&table, TCL_ONE_WORD_KEYS);
    TkWmStackorderToplevelWrapperMap(parentPtr, parentPtr->display, &table);

    window_ptr = windows = ckalloc((table.numEntries+1) * sizeof(TkWindow *));
    if (windows == NULL) {
	return NULL;
    }

    /*
     * Special cases: If zero or one toplevels were mapped there is no need to
     * call XQueryTree.
     */

    switch (table.numEntries) {
    case 0:
	windows[0] = NULL;
	goto done;
    case 1:
	hPtr = Tcl_FirstHashEntry(&table, &search);
	windows[0] = Tcl_GetHashValue(hPtr);
	windows[1] = NULL;
	goto done;
    }

    vRoot = parentPtr->wmInfoPtr->vRoot;
    if (vRoot == None) {
	vRoot = RootWindowOfScreen(Tk_Screen((Tk_Window) parentPtr));
    }

    if (XQueryTree(parentPtr->display, vRoot, &dummy1, &dummy2,
	    &children, &numChildren) == 0) {
	ckfree(windows);
	windows = NULL;
    } else {
	for (i = 0; i < numChildren; i++) {
	    hPtr = Tcl_FindHashEntry(&table, (char *) children[i]);
	    if (hPtr != NULL) {
		childWinPtr = Tcl_GetHashValue(hPtr);
		*window_ptr++ = childWinPtr;
	    }
	}

	/*
	 * ASSERT: window_ptr - windows == table.numEntries
	 * (#matched toplevel windows == #children) [Bug 1789819]
	 */

	*window_ptr = NULL;
	if (numChildren) {
	    XFree((char *) children);
	}
    }

  done:
    Tcl_DeleteHashTable(&table);
    return windows;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmRestackToplevel --
 *
 *	This function restacks a top-level window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	WinPtr gets restacked as specified by aboveBelow and otherPtr.
 *
 *----------------------------------------------------------------------
 */

void
TkWmRestackToplevel(
    TkWindow *winPtr,		/* Window to restack. */
    int aboveBelow,		/* Gives relative position for restacking;
				 * must be Above or Below. */
    TkWindow *otherPtr)		/* Window relative to which to restack; if
				 * NULL, then winPtr gets restacked above or
				 * below *all* siblings. */
{
    XWindowChanges changes;
    unsigned mask;
    TkWindow *wrapperPtr;

    memset(&changes, 0, sizeof(XWindowChanges));
    changes.stack_mode = aboveBelow;
    mask = CWStackMode;

    /*
     * Make sure that winPtr and its wrapper window have been created.
     */

    if (winPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	TkWmMapWindow(winPtr);
    }
    wrapperPtr = winPtr->wmInfoPtr->wrapperPtr;

    if (otherPtr != NULL) {
	/*
	 * The window is to be restacked with respect to another toplevel.
	 * Make sure it has been created as well.
	 */

	if (otherPtr->wmInfoPtr->flags & WM_NEVER_MAPPED) {
	    TkWmMapWindow(otherPtr);
	}
	changes.sibling = otherPtr->wmInfoPtr->wrapperPtr->window;
	mask |= CWSibling;
    }

    /*
     * Reconfigure the window. Note that we use XReconfigureWMWindow instead
     * of XConfigureWindow, in order to handle the case where the window is to
     * be restacked with respect to another toplevel. See [ICCCM] 4.1.5
     * "Configuring the Window" and XReconfigureWMWindow(3) for details.
     */

    XReconfigureWMWindow(winPtr->display, wrapperPtr->window,
	    Tk_ScreenNumber((Tk_Window) winPtr), mask, &changes);
}


/*
 *----------------------------------------------------------------------
 *
 * TkWmAddToColormapWindows --
 *
 *	This function is called to add a given window to the
 *	WM_COLORMAP_WINDOWS property for its top-level, if it isn't already
 *	there. It is invoked by the Tk code that creates a new colormap, in
 *	order to make sure that colormap information is propagated to the
 *	window manager by default.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	WinPtr's window gets added to the WM_COLORMAP_WINDOWS property of its
 *	nearest top-level ancestor, unless the colormaps have been set
 *	explicitly with the "wm colormapwindows" command.
 *
 *----------------------------------------------------------------------
 */

void
TkWmAddToColormapWindows(
    TkWindow *winPtr)		/* Window with a non-default colormap. Should
				 * not be a top-level window. */
{
    TkWindow *wrapperPtr;
    TkWindow *topPtr;
    Window *oldPtr, *newPtr;
    int count, i;

    if (winPtr->window == None) {
	return;
    }

    for (topPtr = winPtr->parentPtr; ; topPtr = topPtr->parentPtr) {
	if (topPtr == NULL) {
	    /*
	     * Window is being deleted. Skip the whole operation.
	     */

	    return;
	}
	if (topPtr->flags & TK_TOP_HIERARCHY) {
	    break;
	}
    }
    if (topPtr->wmInfoPtr == NULL) {
	return;
    }

    if (topPtr->wmInfoPtr->flags & WM_COLORMAPS_EXPLICIT) {
	return;
    }
    if (topPtr->wmInfoPtr->wrapperPtr == NULL) {
	CreateWrapper(topPtr->wmInfoPtr);
    }
    wrapperPtr = topPtr->wmInfoPtr->wrapperPtr;

    /*
     * Fetch the old value of the property.
     */

    if (XGetWMColormapWindows(topPtr->display, wrapperPtr->window,
	    &oldPtr, &count) == 0) {
	oldPtr = NULL;
	count = 0;
    }

    /*
     * Make sure that the window isn't already in the list.
     */

    for (i = 0; i < count; i++) {
	if (oldPtr[i] == winPtr->window) {
	    return;
	}
    }

    /*
     * Make a new bigger array and use it to reset the property. Automatically
     * add the toplevel itself as the last element of the list.
     */

    newPtr = ckalloc((count+2) * sizeof(Window));
    for (i = 0; i < count; i++) {
	newPtr[i] = oldPtr[i];
    }
    if (count == 0) {
	count++;
    }
    newPtr[count-1] = winPtr->window;
    newPtr[count] = topPtr->window;
    XSetWMColormapWindows(topPtr->display, wrapperPtr->window, newPtr,
	    count+1);
    ckfree(newPtr);
    if (oldPtr != NULL) {
	XFree((char *) oldPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmRemoveFromColormapWindows --
 *
 *	This function is called to remove a given window from the
 *	WM_COLORMAP_WINDOWS property for its top-level. It is invoked when
 *	windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	WinPtr's window gets removed from the WM_COLORMAP_WINDOWS property of
 *	its nearest top-level ancestor, unless the top-level itself is being
 *	deleted too.
 *
 *----------------------------------------------------------------------
 */

void
TkWmRemoveFromColormapWindows(
    TkWindow *winPtr)		/* Window that may be present in
				 * WM_COLORMAP_WINDOWS property for its
				 * top-level. Should not be a top-level
				 * window. */
{
    TkWindow *wrapperPtr;
    TkWindow *topPtr;
    Window *oldPtr;
    int count, i, j;

    if (winPtr->window == None) {
	return;
    }

    for (topPtr = winPtr->parentPtr; ; topPtr = topPtr->parentPtr) {
	if (topPtr == NULL) {
	    /*
	     * Ancestors have been deleted, so skip the whole operation.
	     * Seems like this can't ever happen?
	     */

	    return;
	}
	if (topPtr->flags & TK_TOP_HIERARCHY) {
	    break;
	}
    }
    if (topPtr->flags & TK_ALREADY_DEAD) {
	/*
	 * Top-level is being deleted, so there's no need to cleanup the
	 * WM_COLORMAP_WINDOWS property.
	 */

	return;
    }
    if (topPtr->wmInfoPtr == NULL) {
	return;
    }

    if (topPtr->wmInfoPtr->wrapperPtr == NULL) {
	CreateWrapper(topPtr->wmInfoPtr);
    }
    wrapperPtr = topPtr->wmInfoPtr->wrapperPtr;
    if (wrapperPtr == NULL) {
	return;
    }

    /*
     * Fetch the old value of the property.
     */

    if (XGetWMColormapWindows(topPtr->display, wrapperPtr->window,
	    &oldPtr, &count) == 0) {
	return;
    }

    /*
     * Find the window and slide the following ones down to cover it up.
     */

    for (i = 0; i < count; i++) {
	if (oldPtr[i] == winPtr->window) {
	    for (j = i ; j < count-1; j++) {
		oldPtr[j] = oldPtr[j+1];
	    }
	    XSetWMColormapWindows(topPtr->display, wrapperPtr->window,
		    oldPtr, count-1);
	    break;
	}
    }
    XFree((char *) oldPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetPointerCoords --
 *
 *	Fetch the position of the mouse pointer.
 *
 * Results:
 *	*xPtr and *yPtr are filled in with the (virtual) root coordinates of
 *	the mouse pointer for tkwin's display. If the pointer isn't on tkwin's
 *	screen, then -1 values are returned for both coordinates. The argument
 *	tkwin must be a toplevel window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkGetPointerCoords(
    Tk_Window tkwin,		/* Toplevel window that identifies screen on
				 * which lookup is to be done. */
    int *xPtr, int *yPtr)	/* Store pointer coordinates here. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WmInfo *wmPtr;
    Window w, root, child;
    int rootX, rootY;
    unsigned mask;

    wmPtr = winPtr->wmInfoPtr;

    w = wmPtr->vRoot;
    if (w == None) {
	w = RootWindow(winPtr->display, winPtr->screenNum);
    }
    if (XQueryPointer(winPtr->display, w, &root, &child, &rootX, &rootY,
	    xPtr, yPtr, &mask) != True) {
	*xPtr = -1;
	*yPtr = -1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMaxSize --
 *
 *	This function computes the current maxWidth and maxHeight values for a
 *	window, taking into account the possibility that they may be
 *	defaulted.
 *
 * Results:
 *	The values at *maxWidthPtr and *maxHeightPtr are filled in with the
 *	maximum allowable dimensions of wmPtr's window, in grid units. If no
 *	maximum has been specified for the window, then this function computes
 *	the largest sizes that will fit on the screen.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMaxSize(
    WmInfo *wmPtr,		/* Window manager information for the
				 * window. */
    int *maxWidthPtr,		/* Where to store the current maximum width of
				 * the window. */
    int *maxHeightPtr)		/* Where to store the current maximum height
				 * of the window. */
{
    int tmp;

    if (wmPtr->maxWidth > 0) {
	*maxWidthPtr = wmPtr->maxWidth;
    } else {
	/*
	 * Must compute a default width. Fill up the display, leaving a bit of
	 * extra space for the window manager's borders.
	 */

	tmp = DisplayWidth(wmPtr->winPtr->display, wmPtr->winPtr->screenNum)
	    - 15;
	if (wmPtr->gridWin != NULL) {
	    /*
	     * Gridding is turned on; convert from pixels to grid units.
	     */

	    tmp = wmPtr->reqGridWidth
		    + (tmp - wmPtr->winPtr->reqWidth)/wmPtr->widthInc;
	}
	*maxWidthPtr = tmp;
    }
    if (wmPtr->maxHeight > 0) {
	*maxHeightPtr = wmPtr->maxHeight;
    } else {
	tmp = DisplayHeight(wmPtr->winPtr->display, wmPtr->winPtr->screenNum)
	    - 30;
	if (wmPtr->gridWin != NULL) {
	    tmp = wmPtr->reqGridHeight
		    + (tmp - wmPtr->winPtr->reqHeight)/wmPtr->heightInc;
	}
	*maxHeightPtr = tmp;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetTransientFor --
 *
 *	Set a Tk window to be transient with reference to a specified
 *	parent or the toplevel ancestor if None is passed as parent.
 *
 *----------------------------------------------------------------------
 */

static void
TkSetTransientFor(Tk_Window tkwin, Tk_Window parent)
{
    if (parent == None) {
	parent = Tk_Parent(tkwin);
	while (!Tk_IsTopLevel(parent))
	    parent = Tk_Parent(parent);
    }
    /*
     * Prevent crash due to incomplete initialization, or other problems.
     * [Bugs 3554026, 3561016]
     */
    if (((TkWindow *)parent)->wmInfoPtr->wrapperPtr == NULL) {
	CreateWrapper(((TkWindow *)parent)->wmInfoPtr);
    }
    XSetTransientForHint(Tk_Display(tkwin),
	((TkWindow *)tkwin)->wmInfoPtr->wrapperPtr->window,
	((TkWindow *)parent)->wmInfoPtr->wrapperPtr->window);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMakeMenuWindow --
 *
 *	Configure the window to be either a pull-down menu, a pop-up menu, or
 *	as a toplevel (torn-off) menu or palette.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the style bit used to create a new Mac toplevel.
 *
 *----------------------------------------------------------------------
 */

void
TkpMakeMenuWindow(
    Tk_Window tkwin,		/* New window. */
    int typeFlag)		/* TK_MAKE_MENU_DROPDOWN means menu is only
				 * posted briefly as a pulldown or cascade,
				 * TK_MAKE_MENU_POPUP means it is a popup.
				 * TK_MAKE_MENU_TEAROFF means menu is always
				 * visible, e.g. as a torn-off menu.
				 * Determines whether save_under and
				 * override_redirect should be set, plus how
				 * to flag it for the window manager. */
{
    WmInfo *wmPtr;
    XSetWindowAttributes atts;
    TkWindow *wrapperPtr;
    Tcl_Obj *typeObj;

    if (!Tk_HasWrapper(tkwin)) {
	return;
    }
    wmPtr = ((TkWindow *) tkwin)->wmInfoPtr;
    if (wmPtr->wrapperPtr == NULL) {
	CreateWrapper(wmPtr);
    }
    wrapperPtr = wmPtr->wrapperPtr;
    if (typeFlag == TK_MAKE_MENU_TEAROFF) {
	atts.override_redirect = False;
	atts.save_under = False;
	typeObj = Tcl_NewStringObj("menu", -1);
	TkSetTransientFor(tkwin, NULL);
    } else {
	atts.override_redirect = True;
	atts.save_under = True;
	if (typeFlag == TK_MAKE_MENU_DROPDOWN) {
	    typeObj = Tcl_NewStringObj("dropdown_menu", -1);
	} else {
	    typeObj = Tcl_NewStringObj("popup_menu", -1);
	}
    }
    SetNetWmType((TkWindow *)tkwin, typeObj);

    /*
     * The override-redirect and save-under bits must be set on the wrapper
     * window in order to have the desired effect. However, also set the
     * override-redirect bit on the window itself, so that the "wm
     * overrideredirect" command will see it.
     */

    if ((atts.override_redirect!=Tk_Attributes(wrapperPtr)->override_redirect)
	    || (atts.save_under != Tk_Attributes(wrapperPtr)->save_under)) {
	Tk_ChangeWindowAttributes((Tk_Window) wrapperPtr,
		CWOverrideRedirect|CWSaveUnder, &atts);
    }
    if (atts.override_redirect != Tk_Attributes(tkwin)->override_redirect) {
	Tk_ChangeWindowAttributes(tkwin, CWOverrideRedirect, &atts);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateWrapper --
 *
 *	This function is invoked to create the wrapper window for a toplevel
 *	window. It is called just before a toplevel is mapped for the first
 *	time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The wrapper is created and the toplevel is reparented inside it.
 *
 *----------------------------------------------------------------------
 */

static void
CreateWrapper(
    WmInfo *wmPtr)		/* Window manager information for the
				 * window. */
{
    TkWindow *winPtr, *wrapperPtr;
    Window parent;
    Tcl_HashEntry *hPtr;
    int new;

    winPtr = wmPtr->winPtr;
    if (winPtr->window == None) {
	Tk_MakeWindowExist((Tk_Window) winPtr);
    }

    /*
     * The code below is copied from CreateTopLevelWindow, Tk_MakeWindowExist,
     * and TkpMakeWindow. The idea is to create an "official" Tk window (so
     * that we can get events on it), but to hide the window outside the
     * official Tk hierarchy so that it isn't visible to the application. See
     * the comments for the other functions if you have questions about this
     * code.
     */

    wmPtr->wrapperPtr = wrapperPtr = TkAllocWindow(winPtr->dispPtr,
	    Tk_ScreenNumber((Tk_Window) winPtr), winPtr);
    wrapperPtr->dirtyAtts |= CWBorderPixel;

    /*
     * Tk doesn't normally select for StructureNotifyMask events because the
     * events are synthesized internally. However, for wrapper windows we need
     * to know when the window manager modifies the window configuration. We
     * also need to select on focus change events; these are the only windows
     * for which we care about focus changes.
     */

    wrapperPtr->flags |= TK_WRAPPER;
    wrapperPtr->atts.event_mask |= StructureNotifyMask|FocusChangeMask;
    wrapperPtr->atts.override_redirect = winPtr->atts.override_redirect;
    if (winPtr->flags & TK_EMBEDDED) {
	parent = TkUnixContainerId(winPtr);
    } else {
	parent = XRootWindow(wrapperPtr->display, wrapperPtr->screenNum);
    }
    wrapperPtr->window = XCreateWindow(wrapperPtr->display,
	    parent, wrapperPtr->changes.x, wrapperPtr->changes.y,
	    (unsigned) wrapperPtr->changes.width,
	    (unsigned) wrapperPtr->changes.height,
	    (unsigned) wrapperPtr->changes.border_width, wrapperPtr->depth,
	    InputOutput, wrapperPtr->visual,
	    wrapperPtr->dirtyAtts|CWOverrideRedirect, &wrapperPtr->atts);
    hPtr = Tcl_CreateHashEntry(&wrapperPtr->dispPtr->winTable,
	    (char *) wrapperPtr->window, &new);
    Tcl_SetHashValue(hPtr, wrapperPtr);
    wrapperPtr->mainPtr = winPtr->mainPtr;
    wrapperPtr->mainPtr->refCount++;
    wrapperPtr->dirtyAtts = 0;
    wrapperPtr->dirtyChanges = 0;
    wrapperPtr->wmInfoPtr = wmPtr;

    /*
     * Reparent the toplevel window inside the wrapper.
     */

    XReparentWindow(wrapperPtr->display, winPtr->window, wrapperPtr->window,
	    0, 0);

    /*
     * Tk must monitor structure events for wrapper windows in order to detect
     * changes made by window managers such as resizing, mapping, unmapping,
     * etc..
     */

    Tk_CreateEventHandler((Tk_Window) wmPtr->wrapperPtr,
	    WrapperEventMask, WrapperEventProc, wmPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmFocusToplevel --
 *
 *	This is a utility function invoked by focus-management code. The focus
 *	code responds to externally generated focus-related events on wrapper
 *	windows but ignores those events for any other windows. This function
 *	determines whether a given window is a wrapper window and, if so,
 *	returns the toplevel window corresponding to the wrapper.
 *
 * Results:
 *	If winPtr is a wrapper window, returns a pointer to the corresponding
 *	toplevel window; otherwise returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkWmFocusToplevel(
    TkWindow *winPtr)		/* Window that received a focus-related
				 * event. */
{
    if (!(winPtr->flags & TK_WRAPPER)) {
	return NULL;
    }
    return winPtr->wmInfoPtr->winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkUnixSetMenubar --
 *
 *	This function is invoked by menu management code to specify the window
 *	to use as a menubar for a given toplevel window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window given by menubar will be mapped and positioned inside the
 *	wrapper for tkwin and above tkwin. Menubar will automatically be
 *	resized to maintain the height specified by TkUnixSetMenuHeight the
 *	same width as tkwin. Any previous menubar specified for tkwin will be
 *	unmapped and ignored from now on.
 *
 *----------------------------------------------------------------------
 */

void
TkUnixSetMenubar(
    Tk_Window tkwin,		/* Token for toplevel window. */
    Tk_Window menubar)		/* Token for window that is to serve as
				 * menubar for tkwin. Must not be a toplevel
				 * window. If NULL, any existing menubar is
				 * canceled and the menu height is reset to
				 * 0. */
{
    WmInfo *wmPtr = ((TkWindow *) tkwin)->wmInfoPtr;
    Tk_Window parent;
    TkWindow *menubarPtr = (TkWindow *) menubar;

    /*
     * Could be a Frame (i.e. not a toplevel).
     */

    if (wmPtr == NULL) {
	return;
    }

    if (wmPtr->menubar != NULL) {
	/*
	 * There's already a menubar for this toplevel. If it isn't the same
	 * as the new menubar, unmap it so that it is out of the way, and
	 * reparent it back to its original parent.
	 */

	if (wmPtr->menubar == menubar) {
	    return;
	}
	((TkWindow *) wmPtr->menubar)->wmInfoPtr = NULL;
	((TkWindow *) wmPtr->menubar)->flags &= ~TK_REPARENTED;
	Tk_UnmapWindow(wmPtr->menubar);
	parent = Tk_Parent(wmPtr->menubar);
	if (parent != NULL) {
	    Tk_MakeWindowExist(parent);
	    XReparentWindow(Tk_Display(wmPtr->menubar),
		    Tk_WindowId(wmPtr->menubar), Tk_WindowId(parent), 0, 0);
	}
	Tk_DeleteEventHandler(wmPtr->menubar, StructureNotifyMask,
		MenubarDestroyProc, wmPtr->menubar);
	Tk_ManageGeometry(wmPtr->menubar, NULL, NULL);
    }

    wmPtr->menubar = menubar;
    if (menubar == NULL) {
	wmPtr->menuHeight = 0;
    } else {
	if ((menubarPtr->flags & TK_TOP_LEVEL)
		|| (Tk_Screen(menubar) != Tk_Screen(tkwin))) {
	    Tcl_Panic("TkUnixSetMenubar got bad menubar");
	}
	wmPtr->menuHeight = Tk_ReqHeight(menubar);
	if (wmPtr->menuHeight == 0) {
	    wmPtr->menuHeight = 1;
	}
	Tk_MakeWindowExist(tkwin);
	Tk_MakeWindowExist(menubar);
	if (wmPtr->wrapperPtr == NULL) {
	    CreateWrapper(wmPtr);
	}
	XReparentWindow(Tk_Display(menubar), Tk_WindowId(menubar),
		wmPtr->wrapperPtr->window, 0, 0);
	menubarPtr->wmInfoPtr = wmPtr;
	Tk_MoveResizeWindow(menubar, 0, 0, Tk_Width(tkwin), wmPtr->menuHeight);
	Tk_MapWindow(menubar);
	Tk_CreateEventHandler(menubar, StructureNotifyMask,
		MenubarDestroyProc, menubar);
	Tk_ManageGeometry(menubar, &menubarMgrType, wmPtr);
	menubarPtr->flags |= TK_REPARENTED;
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, tkwin);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenubarDestroyProc --
 *
 *	This function is invoked by the event dispatcher whenever a menubar
 *	window is destroyed (it's also invoked for a few other kinds of
 *	events, but we ignore those).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The association between the window and its toplevel is broken, so that
 *	the window is no longer considered to be a menubar.
 *
 *----------------------------------------------------------------------
 */

static void
MenubarDestroyProc(
    ClientData clientData,	/* TkWindow pointer for menubar. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    WmInfo *wmPtr;

    if (eventPtr->type != DestroyNotify) {
	return;
    }
    wmPtr = ((TkWindow *) clientData)->wmInfoPtr;
    wmPtr->menubar = NULL;
    wmPtr->menuHeight = 0;
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, wmPtr->winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenubarReqProc --
 *
 *	This function is invoked by the Tk geometry management code whenever a
 *	menubar calls Tk_GeometryRequest to request a new size.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
MenubarReqProc(
    ClientData clientData,	/* Pointer to the window manager information
				 * for tkwin's toplevel. */
    Tk_Window tkwin)		/* Handle for menubar window. */
{
    WmInfo *wmPtr = clientData;

    wmPtr->menuHeight = Tk_ReqHeight(tkwin);
    if (wmPtr->menuHeight <= 0) {
	wmPtr->menuHeight = 1;
    }
    wmPtr->flags |= WM_UPDATE_SIZE_HINTS;
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, wmPtr->winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetWrapperWindow --
 *
 *	Given a toplevel window return the hidden wrapper window for the
 *	toplevel window if available.
 *
 * Results:
 *	The wrapper window. NULL is we were not passed a toplevel window or
 *	the wrapper has yet to be created.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkpGetWrapperWindow(
    TkWindow *winPtr)		/* A toplevel window pointer. */
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;

    if ((winPtr == NULL) || (wmPtr == NULL)) {
	return NULL;
    }

    return wmPtr->wrapperPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateCommand --
 *
 *	Update the WM_COMMAND property, taking care to translate the command
 *	strings into the external encoding.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateCommand(
    TkWindow *winPtr)
{
    register WmInfo *wmPtr = winPtr->wmInfoPtr;
    Tcl_DString cmds, ds;
    int i, *offsets;
    char **cmdArgv;

    /*
     * Translate the argv strings into the external encoding. To avoid
     * allocating lots of memory, the strings are appended to a buffer with
     * nulls between each string.
     *
     * This code is tricky because we need to pass and array of pointers to
     * XSetCommand. However, we can't compute the pointers as we go because
     * the DString buffer space could get reallocated. So, store offsets for
     * each element as we go, then compute pointers from the offsets once the
     * entire DString is done.
     */

    cmdArgv = ckalloc(sizeof(char *) * wmPtr->cmdArgc);
    offsets = ckalloc(sizeof(int) * wmPtr->cmdArgc);
    Tcl_DStringInit(&cmds);
    for (i = 0; i < wmPtr->cmdArgc; i++) {
	Tcl_UtfToExternalDString(NULL, wmPtr->cmdArgv[i], -1, &ds);
	offsets[i] = Tcl_DStringLength(&cmds);
	Tcl_DStringAppend(&cmds, Tcl_DStringValue(&ds),
		Tcl_DStringLength(&ds)+1);
	Tcl_DStringFree(&ds);
    }
    cmdArgv[0] = Tcl_DStringValue(&cmds);
    for (i = 1; i < wmPtr->cmdArgc; i++) {
	cmdArgv[i] = cmdArgv[0] + offsets[i];
    }

    XSetCommand(winPtr->display, wmPtr->wrapperPtr->window,
	    cmdArgv, wmPtr->cmdArgc);
    Tcl_DStringFree(&cmds);
    ckfree(cmdArgv);
    ckfree(offsets);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpWmSetState --
 *
 *	Sets the window manager state for the wrapper window of a given
 *	toplevel window.
 *
 * Results:
 *	0 on error, 1 otherwise
 *
 * Side effects:
 *	May minimize, restore, or withdraw a window.
 *
 *----------------------------------------------------------------------
 */

int
TkpWmSetState(
     TkWindow *winPtr,		/* Toplevel window to operate on. */
     int state)			/* One of IconicState, NormalState, or
				 * WithdrawnState. */
{
    WmInfo *wmPtr = winPtr->wmInfoPtr;

    if (state == WithdrawnState) {
	wmPtr->hints.initial_state = WithdrawnState;
	wmPtr->withdrawn = 1;
	if (wmPtr->flags & WM_NEVER_MAPPED) {
	    return 1;
	}
	if (XWithdrawWindow(winPtr->display, wmPtr->wrapperPtr->window,
		winPtr->screenNum) == 0) {
	    return 0;
	}
	WaitForMapNotify(winPtr, 0);
    } else if (state == NormalState) {
	wmPtr->hints.initial_state = NormalState;
	wmPtr->withdrawn = 0;
	if (wmPtr->flags & WM_NEVER_MAPPED) {
	    return 1;
	}
	UpdateHints(winPtr);
	Tk_MapWindow((Tk_Window) winPtr);
    } else if (state == IconicState) {
	wmPtr->hints.initial_state = IconicState;
	if (wmPtr->flags & WM_NEVER_MAPPED) {
	    return 1;
	}
	if (wmPtr->withdrawn) {
	    UpdateHints(winPtr);
	    Tk_MapWindow((Tk_Window) winPtr);
	    wmPtr->withdrawn = 0;
	} else {
	    if (XIconifyWindow(winPtr->display, wmPtr->wrapperPtr->window,
		    winPtr->screenNum) == 0) {
		return 0;
	    }
	    WaitForMapNotify(winPtr, 0);
	}
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * RemapWindows
 *
 *	Adjust parent/child relationships of the given window hierarchy.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Keeps windowing system (X11) happy
 *
 *----------------------------------------------------------------------
 */

static void
RemapWindows(
    TkWindow *winPtr,
    TkWindow *parentPtr)
{
    XWindowAttributes win_attr;

    if (winPtr->window) {
	XGetWindowAttributes(winPtr->display, winPtr->window, &win_attr);
	if (parentPtr == NULL) {
	    XReparentWindow(winPtr->display, winPtr->window,
		    XRootWindow(winPtr->display, winPtr->screenNum),
		    win_attr.x, win_attr.y);
	} else if (parentPtr->window) {
	    XReparentWindow(parentPtr->display, winPtr->window,
		    parentPtr->window,
		    win_attr.x, win_attr.y);
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
