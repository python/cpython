/*
 * tkBusy.c --
 *
 *	This file provides functions that implement busy for Tk.
 *
 * Copyright 1993-1998 Lucent Technologies, Inc.
 *
 *	The "busy" command was created by George Howlett. Adapted for
 *	integration into Tk by Jos Decoster and Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkBusy.h"
#include "default.h"

/*
 * Things about the busy system that may be configured. Note that on some
 * platforms this may or may not have an effect.
 */

static const Tk_OptionSpec busyOptionSpecs[] = {
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_BUSY_CURSOR, -1, Tk_Offset(Busy, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

/*
 * Forward declarations of functions defined in this file.
 */

static void		BusyEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		BusyGeometryProc(ClientData clientData,
			    Tk_Window tkwin);
static void		BusyCustodyProc(ClientData clientData,
			    Tk_Window tkwin);
static int		ConfigureBusy(Tcl_Interp *interp, Busy *busyPtr,
			    int objc, Tcl_Obj *const objv[]);
static Busy *		CreateBusy(Tcl_Interp *interp, Tk_Window tkRef);
static void		DestroyBusy(void *dataPtr);
static void		DoConfigureNotify(Tk_FakeWin *winPtr);
static inline Tk_Window	FirstChild(Tk_Window parent);
static Busy *		GetBusy(Tcl_Interp *interp,
			    Tcl_HashTable *busyTablePtr,
			    Tcl_Obj *const windowObj);
static int		HoldBusy(Tcl_HashTable *busyTablePtr,
			    Tcl_Interp *interp, Tcl_Obj *const windowObj,
			    int configObjc, Tcl_Obj *const configObjv[]);
static void		MakeTransparentWindowExist(Tk_Window tkwin,
			    Window parent);
static inline Tk_Window	NextChild(Tk_Window tkwin);
static void		RefWinEventProc(ClientData clientData,
			    register XEvent *eventPtr);
static inline void	SetWindowInstanceData(Tk_Window tkwin,
			    ClientData instanceData);

/*
 * The "busy" geometry manager definition.
 */

static Tk_GeomMgr busyMgrInfo = {
    "busy",			/* Name of geometry manager used by winfo */
    BusyGeometryProc,		/* Procedure to for new geometry requests */
    BusyCustodyProc,		/* Procedure when window is taken away */
};

/*
 * Helper functions, need to check if a Tcl/Tk alternative already exists.
 */

static inline Tk_Window
FirstChild(
    Tk_Window parent)
{
    struct TkWindow *parentPtr = (struct TkWindow *) parent;

    return (Tk_Window) parentPtr->childList;
}

static inline Tk_Window
NextChild(
    Tk_Window tkwin)
{
    struct TkWindow *winPtr = (struct TkWindow *) tkwin;

    if (winPtr == NULL) {
	return NULL;
    }
    return (Tk_Window) winPtr->nextPtr;
}

static inline void
SetWindowInstanceData(
    Tk_Window tkwin,
    ClientData instanceData)
{
    struct TkWindow *winPtr = (struct TkWindow *) tkwin;

    winPtr->instanceData = instanceData;
}

/*
 *----------------------------------------------------------------------
 *
 * BusyCustodyProc --
 *
 *	This procedure is invoked when the busy window has been stolen by
 *	another geometry manager. The information and memory associated with
 *	the busy window is released. I don't know why anyone would try to pack
 *	a busy window, but this should keep everything sane, if it is.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Busy structure is freed at the next idle point.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
BusyCustodyProc(
    ClientData clientData,	/* Information about the busy window. */
    Tk_Window tkwin)		/* Not used. */
{
    Busy *busyPtr = clientData;

    Tk_DeleteEventHandler(busyPtr->tkBusy, StructureNotifyMask, BusyEventProc,
	    busyPtr);
    TkpHideBusyWindow(busyPtr);
    busyPtr->tkBusy = NULL;
    Tcl_EventuallyFree(busyPtr, (Tcl_FreeProc *)DestroyBusy);
}

/*
 *----------------------------------------------------------------------
 *
 * BusyGeometryProc --
 *
 *	This procedure is invoked by Tk_GeometryRequest for busy windows.
 *	Busy windows never request geometry, so it's unlikely that this
 *	function will ever be called;it exists simply as a place holder for
 *	the GeomProc in the Geometry Manager structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
BusyGeometryProc(
    ClientData clientData,	/* Information about window that got new
				 * preferred geometry. */
    Tk_Window tkwin)		/* Other Tk-related information about the
				 * window. */
{
    /* Should never get here */
}

/*
 *----------------------------------------------------------------------
 *
 * DoConfigureNotify --
 *
 *	Generate a ConfigureNotify event describing the current configuration
 *	of a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An event is generated and processed by Tk_HandleEvent.
 *
 *----------------------------------------------------------------------
 */

static void
DoConfigureNotify(
    Tk_FakeWin *winPtr)		/* Window whose configuration was just
				 * changed. */
{
    XEvent event;

    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above) {
	event.xconfigure.above = winPtr->changes.sibling;
    } else {
	event.xconfigure.above = None;
    }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
}

/*
 *----------------------------------------------------------------------
 *
 * RefWinEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for the following
 *	events on the reference window. If the reference and parent windows
 *	are the same, only the first event is important.
 *
 *	1) ConfigureNotify	The reference window has been resized or
 *				moved. Move and resize the busy window to be
 *				the same size and position of the reference
 *				window.
 *
 *	2) DestroyNotify	The reference window was destroyed. Destroy
 *				the busy window and the free resources used.
 *
 *	3) MapNotify		The reference window was (re)shown. Map the
 *				busy window again.
 *
 *	4) UnmapNotify		The reference window was hidden. Unmap the
 *				busy window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the reference window gets deleted, internal structures get
 *	cleaned up. When it gets resized, the busy window is resized
 *	accordingly. If it's displayed, the busy window is displayed. And when
 *	it's hidden, the busy window is unmapped.
 *
 *----------------------------------------------------------------------
 */

static void
RefWinEventProc(
    ClientData clientData,	/* Busy window record */
    register XEvent *eventPtr)	/* Event which triggered call to routine */
{
    register Busy *busyPtr = clientData;

    switch (eventPtr->type) {
    case ReparentNotify:
    case DestroyNotify:
	/*
	 * Arrange for the busy structure to be removed at a proper time.
	 */

	Tcl_EventuallyFree(busyPtr, (Tcl_FreeProc *)DestroyBusy);
	break;

    case ConfigureNotify:
	if ((busyPtr->width != Tk_Width(busyPtr->tkRef)) ||
		(busyPtr->height != Tk_Height(busyPtr->tkRef)) ||
		(busyPtr->x != Tk_X(busyPtr->tkRef)) ||
		(busyPtr->y != Tk_Y(busyPtr->tkRef))) {
	    int x, y;

	    busyPtr->width = Tk_Width(busyPtr->tkRef);
	    busyPtr->height = Tk_Height(busyPtr->tkRef);
	    busyPtr->x = Tk_X(busyPtr->tkRef);
	    busyPtr->y = Tk_Y(busyPtr->tkRef);

	    x = y = 0;

	    if (busyPtr->tkParent != busyPtr->tkRef) {
		Tk_Window tkwin;

		for (tkwin = busyPtr->tkRef; (tkwin != NULL) &&
			 (!Tk_IsTopLevel(tkwin)); tkwin = Tk_Parent(tkwin)) {
		    if (tkwin == busyPtr->tkParent) {
			break;
		    }
		    x += Tk_X(tkwin) + Tk_Changes(tkwin)->border_width;
		    y += Tk_Y(tkwin) + Tk_Changes(tkwin)->border_width;
		}
	    }
	    if (busyPtr->tkBusy != NULL) {
		Tk_MoveResizeWindow(busyPtr->tkBusy, x, y, busyPtr->width,
			busyPtr->height);
		TkpShowBusyWindow(busyPtr);
	    }
	}
	break;

    case MapNotify:
	if (busyPtr->tkParent != busyPtr->tkRef) {
	    TkpShowBusyWindow(busyPtr);
	}
	break;

    case UnmapNotify:
	if (busyPtr->tkParent != busyPtr->tkRef) {
	    TkpHideBusyWindow(busyPtr);
	}
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyBusy --
 *
 *	This procedure is called from the Tk event dispatcher. It releases X
 *	resources and memory used by the busy window and updates the internal
 *	hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and resources are released and the Tk event handler is removed.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyBusy(
    void *data)			/* Busy window structure record */
{
    register Busy *busyPtr = data;

    if (busyPtr->hashPtr != NULL) {
	Tcl_DeleteHashEntry(busyPtr->hashPtr);
    }
    Tk_DeleteEventHandler(busyPtr->tkRef, StructureNotifyMask,
	    RefWinEventProc, busyPtr);

    if (busyPtr->tkBusy != NULL) {
	Tk_FreeConfigOptions(data, busyPtr->optionTable, busyPtr->tkBusy);
	Tk_DeleteEventHandler(busyPtr->tkBusy, StructureNotifyMask,
		BusyEventProc, busyPtr);
	Tk_ManageGeometry(busyPtr->tkBusy, NULL, busyPtr);
	Tk_DestroyWindow(busyPtr->tkBusy);
    }
    ckfree(data);
}

/*
 *----------------------------------------------------------------------
 *
 * BusyEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for events on the busy
 *	window itself. We're only concerned with destroy events.
 *
 *	It might be necessary (someday) to watch resize events. Right now, I
 *	don't think there's any point in it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When a busy window is destroyed, all internal structures associated
 *	with it released at the next idle point.
 *
 *----------------------------------------------------------------------
 */

static void
BusyEventProc(
    ClientData clientData,	/* Busy window record */
    XEvent *eventPtr)		/* Event which triggered call to routine */
{
    Busy *busyPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	busyPtr->tkBusy = NULL;
	Tcl_EventuallyFree(busyPtr, (Tcl_FreeProc *)DestroyBusy);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MakeTransparentWindowExist --
 *
 *	Similar to Tk_MakeWindowExist but instead creates a transparent window
 *	to block for user events from sibling windows.
 *
 *	Differences from Tk_MakeWindowExist.
 *
 *	1. This is always a "busy" window. There's never a platform-specific
 *	   class procedure to execute instead.
 *	2. The window is transparent and never will contain children, so
 *	   colormap information is irrelevant.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the procedure returns, the internal window associated with tkwin
 *	is guaranteed to exist. This may require the window's ancestors to be
 *	created too.
 *
 *----------------------------------------------------------------------
 */

static void
MakeTransparentWindowExist(
    Tk_Window tkwin,		/* Token for window. */
    Window parent)		/* Parent window. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    Tcl_HashEntry *hPtr;
    int notUsed;
    TkDisplay *dispPtr;

    if (winPtr->window != None) {
	return;			/* Window already exists. */
    }

    /*
     * Create a transparent window and put it on top.
     */

    TkpMakeTransparentWindowExist(tkwin, parent);

    if (winPtr->window == None) {
	return;			/* Platform didn't make Window. */
    }

    dispPtr = winPtr->dispPtr;
    hPtr = Tcl_CreateHashEntry(&dispPtr->winTable, (char *) winPtr->window,
	    &notUsed);
    Tcl_SetHashValue(hPtr, winPtr);
    winPtr->dirtyAtts = 0;
    winPtr->dirtyChanges = 0;

    if (!(winPtr->flags & TK_TOP_HIERARCHY)) {
	TkWindow *winPtr2;

	/*
	 * If any siblings higher up in the stacking order have already been
	 * created then move this window to its rightful position in the
	 * stacking order.
	 *
	 * NOTE: this code ignores any changes anyone might have made to the
	 * sibling and stack_mode field of the window's attributes, so it
	 * really isn't safe for these to be manipulated except by calling
	 * Tk_RestackWindow.
	 */

	for (winPtr2 = winPtr->nextPtr; winPtr2 != NULL;
		winPtr2 = winPtr2->nextPtr) {
	    if ((winPtr2->window != None) &&
		    !(winPtr2->flags & (TK_TOP_HIERARCHY|TK_REPARENTED))) {
		XWindowChanges changes;

		changes.sibling = winPtr2->window;
		changes.stack_mode = Below;
		XConfigureWindow(winPtr->display, winPtr->window,
			CWSibling | CWStackMode, &changes);
		break;
	    }
	}
    }

    /*
     * Issue a ConfigureNotify event if there were deferred configuration
     * changes (but skip it if the window is being deleted; the
     * ConfigureNotify event could cause problems if we're being called from
     * Tk_DestroyWindow under some conditions).
     */

    if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
	    && !(winPtr->flags & TK_ALREADY_DEAD)) {
	winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;
	DoConfigureNotify((Tk_FakeWin *) tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateBusy --
 *
 *	Creates a child transparent window that obscures its parent window
 *	thereby effectively blocking device events. The size and position of
 *	the busy window is exactly that of the reference window.
 *
 *	We want to create sibling to the window to be blocked. If the busy
 *	window is a child of the window to be blocked, Enter/Leave events can
 *	sneak through. Futhermore under WIN32, messages of transparent windows
 *	are sent directly to the parent. The only exception to this are
 *	toplevels, since we can't make a sibling. Fortunately, toplevel
 *	windows rarely receive events that need blocking.
 *
 * Results:
 *	Returns a pointer to the new busy window structure.
 *
 * Side effects:
 *	When the busy window is eventually displayed, it will screen device
 *	events (in the area of the reference window) from reaching its parent
 *	window and its children. User feed back can be achieved by changing
 *	the cursor.
 *
 *----------------------------------------------------------------------
 */

static Busy *
CreateBusy(
    Tcl_Interp *interp,		/* Interpreter to report error to */
    Tk_Window tkRef)		/* Window hosting the busy window */
{
    Busy *busyPtr;
    size_t length;
    int x, y;
    const char *fmt;
    char *name;
    Tk_Window tkBusy, tkChild, tkParent;
    Window parent;
    Tk_FakeWin *winPtr;

    busyPtr = ckalloc(sizeof(Busy));
    x = y = 0;
    length = strlen(Tk_Name(tkRef));
    name = ckalloc(length + 6);
    if (Tk_IsTopLevel(tkRef)) {
	fmt = "_Busy";		/* Child */
	tkParent = tkRef;
    } else {
	Tk_Window tkwin;

	fmt = "%s_Busy";	/* Sibling */
	tkParent = Tk_Parent(tkRef);
	for (tkwin = tkRef; (tkwin != NULL) && !Tk_IsTopLevel(tkwin);
		tkwin = Tk_Parent(tkwin)) {
	    if (tkwin == tkParent) {
		break;
	    }
	    x += Tk_X(tkwin) + Tk_Changes(tkwin)->border_width;
	    y += Tk_Y(tkwin) + Tk_Changes(tkwin)->border_width;
	}
    }
    for (tkChild = FirstChild(tkParent); tkChild != NULL;
	    tkChild = NextChild(tkChild)) {
	Tk_MakeWindowExist(tkChild);
    }
    sprintf(name, fmt, Tk_Name(tkRef));
    tkBusy = Tk_CreateWindow(interp, tkParent, name, NULL);
    ckfree(name);

    if (tkBusy == NULL) {
	return NULL;
    }
    Tk_MakeWindowExist(tkRef);
    busyPtr->display = Tk_Display(tkRef);
    busyPtr->interp = interp;
    busyPtr->tkRef = tkRef;
    busyPtr->tkParent = tkParent;
    busyPtr->tkBusy = tkBusy;
    busyPtr->width = Tk_Width(tkRef);
    busyPtr->height = Tk_Height(tkRef);
    busyPtr->x = Tk_X(tkRef);
    busyPtr->y = Tk_Y(tkRef);
    busyPtr->cursor = NULL;
    Tk_SetClass(tkBusy, "Busy");
    busyPtr->optionTable = Tk_CreateOptionTable(interp, busyOptionSpecs);
    if (Tk_InitOptions(interp, (char *) busyPtr, busyPtr->optionTable,
	    tkBusy) != TCL_OK) {
	Tk_DestroyWindow(tkBusy);
	return NULL;
    }
    SetWindowInstanceData(tkBusy, busyPtr);
    winPtr = (Tk_FakeWin *) tkRef;

    TkpCreateBusy(winPtr, tkRef, &parent, tkParent, busyPtr);

    MakeTransparentWindowExist(tkBusy, parent);

    Tk_MoveResizeWindow(tkBusy, x, y, busyPtr->width, busyPtr->height);

    /*
     * Only worry if the busy window is destroyed.
     */

    Tk_CreateEventHandler(tkBusy, StructureNotifyMask, BusyEventProc,
	    busyPtr);

    /*
     * Indicate that the busy window's geometry is being managed. This will
     * also notify us if the busy window is ever packed.
     */

    Tk_ManageGeometry(tkBusy, &busyMgrInfo, busyPtr);
    if (busyPtr->cursor != NULL) {
	Tk_DefineCursor(tkBusy, busyPtr->cursor);
    }

    /*
     * Track the reference window to see if it is resized or destroyed.
     */

    Tk_CreateEventHandler(tkRef, StructureNotifyMask, RefWinEventProc,
	    busyPtr);
    return busyPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureBusy --
 *
 *	This procedure is called from the Tk event dispatcher. It releases X
 *	resources and memory used by the busy window and updates the internal
 *	hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and resources are released and the Tk event handler is removed.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureBusy(
    Tcl_Interp *interp,
    Busy *busyPtr,
    int objc,
    Tcl_Obj *const objv[])
{
    Tk_Cursor oldCursor = busyPtr->cursor;

    if (Tk_SetOptions(interp, (char *) busyPtr, busyPtr->optionTable, objc,
	    objv, busyPtr->tkBusy, NULL, NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    if (busyPtr->cursor != oldCursor) {
	if (busyPtr->cursor == NULL) {
	    Tk_UndefineCursor(busyPtr->tkBusy);
	} else {
	    Tk_DefineCursor(busyPtr->tkBusy, busyPtr->cursor);
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetBusy --
 *
 *	Returns the busy window structure associated with the reference
 *	window, keyed by its path name. The clientData argument is the main
 *	window of the interpreter, used to search for the reference window in
 *	its own window hierarchy.
 *
 * Results:
 *	If path name represents a reference window with a busy window, a
 *	pointer to the busy window structure is returned. Otherwise, NULL is
 *	returned and an error message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Busy *
GetBusy(
    Tcl_Interp *interp,		/* Interpreter to look up main window of. */
    Tcl_HashTable *busyTablePtr,/* Busy hash table */
    Tcl_Obj *const windowObj)	/* Path name of parent window */
{
    Tcl_HashEntry *hPtr;
    Tk_Window tkwin;

    if (TkGetWindowFromObj(interp, Tk_MainWindow(interp), windowObj,
	    &tkwin) != TCL_OK) {
	return NULL;
    }
    hPtr = Tcl_FindHashEntry(busyTablePtr, (char *) tkwin);
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't find busy window \"%s\"", Tcl_GetString(windowObj)));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "BUSY",
		Tcl_GetString(windowObj), NULL);
	return NULL;
    }
    return Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * HoldBusy --
 *
 *	Creates (if necessary) and maps a busy window, thereby preventing
 *	device events from being be received by the parent window and its
 *	children.
 *
 * Results:
 *	Returns a standard TCL result. If path name represents a busy window,
 *	it is unmapped and TCL_OK is returned. Otherwise, TCL_ERROR is
 *	returned and an error message is left in interp->result.
 *
 * Side effects:
 *	The busy window is created and displayed, blocking events from the
 *	parent window and its children.
 *
 *----------------------------------------------------------------------
 */

static int
HoldBusy(
    Tcl_HashTable *busyTablePtr,/* Busy hash table. */
    Tcl_Interp *interp,		/* Interpreter to report errors to. */
    Tcl_Obj *const windowObj,	/* Window name. */
    int configObjc,		/* Option pairs. */
    Tcl_Obj *const configObjv[])
{
    Tk_Window tkwin;
    Tcl_HashEntry *hPtr;
    Busy *busyPtr;
    int isNew, result;

    if (TkGetWindowFromObj(interp, Tk_MainWindow(interp), windowObj,
	    &tkwin) != TCL_OK) {
	return TCL_ERROR;
    }
    hPtr = Tcl_CreateHashEntry(busyTablePtr, (char *) tkwin, &isNew);
    if (isNew) {
	busyPtr = CreateBusy(interp, tkwin);
	if (busyPtr == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetHashValue(hPtr, busyPtr);
	busyPtr->hashPtr = hPtr;
    } else {
	busyPtr = Tcl_GetHashValue(hPtr);
    }

    busyPtr->tablePtr = busyTablePtr;
    result = ConfigureBusy(interp, busyPtr, configObjc, configObjv);

    /*
     * Don't map the busy window unless the reference window is also currently
     * displayed.
     */

    if (Tk_IsMapped(busyPtr->tkRef)) {
	TkpShowBusyWindow(busyPtr);
    } else {
	TkpHideBusyWindow(busyPtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_BusyObjCmd --
 *
 *	This function is invoked to process the "tk busy" Tcl command. See the
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

int
Tk_BusyObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    Tcl_HashTable *busyTablePtr = &((TkWindow *) tkwin)->mainPtr->busyTable;
    Busy *busyPtr;
    Tcl_Obj *objPtr;
    int index, result = TCL_OK;
    static const char *const optionStrings[] = {
	"cget", "configure", "current", "forget", "hold", "status", NULL
    };
    enum options {
	BUSY_CGET, BUSY_CONFIGURE, BUSY_CURRENT, BUSY_FORGET, BUSY_HOLD,
	BUSY_STATUS
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "options ?arg arg ...?");
	return TCL_ERROR;
    }

    /*
     * [tk busy <window>] command shortcut.
     */

    if (Tcl_GetString(objv[1])[0] == '.') {
	if (objc%2 == 1) {
	    Tcl_WrongNumArgs(interp, 1, objv, "window ?option value ...?");
	    return TCL_ERROR;
	}
	return HoldBusy(busyTablePtr, interp, objv[1], objc-2, objv+2);
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], optionStrings,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum options) index) {
    case BUSY_CGET:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window option");
	    return TCL_ERROR;
	}
	busyPtr = GetBusy(interp, busyTablePtr, objv[2]);
	if (busyPtr == NULL) {
	    return TCL_ERROR;
	}
	Tcl_Preserve(busyPtr);
	objPtr = Tk_GetOptionValue(interp, (char *) busyPtr,
		busyPtr->optionTable, objv[3], busyPtr->tkBusy);
	if (objPtr == NULL) {
	    result = TCL_ERROR;
	} else {
	    Tcl_SetObjResult(interp, objPtr);
	}
	Tcl_Release(busyPtr);
	return result;

    case BUSY_CONFIGURE:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window ?option? ?value ...?");
	    return TCL_ERROR;
	}
	busyPtr = GetBusy(interp, busyTablePtr, objv[2]);
	if (busyPtr == NULL) {
	    return TCL_ERROR;
	}
	Tcl_Preserve(busyPtr);
	if (objc <= 4) {
	    objPtr = Tk_GetOptionInfo(interp, (char *) busyPtr,
		    busyPtr->optionTable, (objc == 4) ? objv[3] : NULL,
		    busyPtr->tkBusy);
	    if (objPtr == NULL) {
		result = TCL_ERROR;
	    } else {
		Tcl_SetObjResult(interp, objPtr);
	    }
	} else {
	    result = ConfigureBusy(interp, busyPtr, objc-3, objv+3);
	}
	Tcl_Release(busyPtr);
	return result;

    case BUSY_CURRENT: {
	Tcl_HashEntry *hPtr;
	Tcl_HashSearch cursor;
	const char *pattern = (objc == 3 ? Tcl_GetString(objv[2]) : NULL);

	objPtr = Tcl_NewObj();
	for (hPtr = Tcl_FirstHashEntry(busyTablePtr, &cursor); hPtr != NULL;
		hPtr = Tcl_NextHashEntry(&cursor)) {
	    busyPtr = Tcl_GetHashValue(hPtr);
	    if (pattern == NULL ||
		    Tcl_StringCaseMatch(Tk_PathName(busyPtr->tkRef), pattern, 0)) {
		Tcl_ListObjAppendElement(interp, objPtr,
			TkNewWindowObj(busyPtr->tkRef));
	    }
	}
	Tcl_SetObjResult(interp, objPtr);
	return TCL_OK;
    }

    case BUSY_FORGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	busyPtr = GetBusy(interp, busyTablePtr, objv[2]);
	if (busyPtr == NULL) {
	    return TCL_ERROR;
	}
	TkpHideBusyWindow(busyPtr);
	Tcl_EventuallyFree(busyPtr, (Tcl_FreeProc *)DestroyBusy);
	return TCL_OK;

    case BUSY_HOLD:
	if (objc < 3 || objc%2 != 1) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window ?option value ...?");
	    return TCL_ERROR;
	}
	return HoldBusy(busyTablePtr, interp, objv[2], objc-3, objv+3);

    case BUSY_STATUS:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
		GetBusy(interp, busyTablePtr, objv[2]) != NULL));
	return TCL_OK;
    }

    Tcl_Panic("unhandled option: %d", index);
    return TCL_ERROR;		/* Unreachable */
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
