/*
 * tkWindow.c --
 *
 *	This file provides basic window-manipulation functions, which are
 *	equivalent to functions in Xlib (and even invoke them) but also
 *	maintain the local Tk_Window structure.
 *
 * Copyright (c) 1989-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

#ifdef _WIN32
#include "tkWinInt.h"
#elif !defined(MAC_OSX_TK)
#include "tkUnixInt.h"
#endif

/*
 * Type used to keep track of Window objects that were only partially
 * deallocated by Tk_DestroyWindow.
 */

#define HD_CLEANUP		1
#define HD_FOCUS		2
#define HD_MAIN_WIN		4
#define HD_DESTROY_COUNT	8
#define HD_DESTROY_EVENT	0x10

typedef struct TkHalfdeadWindow {
    int flags;
    struct TkWindow *winPtr;
    struct TkHalfdeadWindow *nextPtr;
} TkHalfdeadWindow;

typedef struct {
    int numMainWindows;		/* Count of numver of main windows currently
				 * open in this thread. */
    TkMainInfo *mainWindowList;
				/* First in list of all main windows managed
				 * by this thread. */
    TkHalfdeadWindow *halfdeadWindowList;
				/* First in list of partially deallocated
				 * windows. */
    TkDisplay *displayList;	/* List of all displays currently in use by
				 * the current thread. */
    int initialized;		/* 0 means the structures above need
				 * initializing. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Default values for "changes" and "atts" fields of TkWindows. Note that Tk
 * always requests all events for all windows, except StructureNotify events
 * on internal windows: these events are generated internally.
 */

static const XWindowChanges defChanges = {
    0, 0, 1, 1, 0, 0, Above
};
#define ALL_EVENTS_MASK \
    KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask| \
    EnterWindowMask|LeaveWindowMask|PointerMotionMask|ExposureMask| \
    VisibilityChangeMask|PropertyChangeMask|ColormapChangeMask
static const XSetWindowAttributes defAtts= {
    None,			/* background_pixmap */
    0,				/* background_pixel */
    CopyFromParent,		/* border_pixmap */
    0,				/* border_pixel */
    NorthWestGravity,		/* bit_gravity */
    NorthWestGravity,		/* win_gravity */
    NotUseful,			/* backing_store */
    (unsigned) ~0,		/* backing_planes */
    0,				/* backing_pixel */
    False,			/* save_under */
    ALL_EVENTS_MASK,		/* event_mask */
    0,				/* do_not_propagate_mask */
    False,			/* override_redirect */
    CopyFromParent,		/* colormap */
    None			/* cursor */
};

/*
 * The following structure defines all of the commands supported by Tk, and
 * the C functions that execute them.
 */

#define ISSAFE 1
#define PASSMAINWINDOW 2
#define WINMACONLY 4
#define USEINITPROC 8

typedef int (TkInitProc)(Tcl_Interp *interp, ClientData clientData);
typedef struct {
    const char *name;		/* Name of command. */
    Tcl_ObjCmdProc *objProc;	/* Command's object- (or string-) based
				 * function, or initProc. */
    int flags;
} TkCmd;

static const TkCmd commands[] = {
    /*
     * Commands that are part of the intrinsics:
     */

    {"bell",		Tk_BellObjCmd,		PASSMAINWINDOW},
    {"bind",		Tk_BindObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"bindtags",	Tk_BindtagsObjCmd,	PASSMAINWINDOW|ISSAFE},
    {"clipboard",	Tk_ClipboardObjCmd,	PASSMAINWINDOW},
    {"destroy",		Tk_DestroyObjCmd,	PASSMAINWINDOW|ISSAFE},
    {"event",		Tk_EventObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"focus",		Tk_FocusObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"font",		Tk_FontObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"grab",		Tk_GrabObjCmd,		PASSMAINWINDOW},
    {"grid",		Tk_GridObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"image",		Tk_ImageObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"lower",		Tk_LowerObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"option",		Tk_OptionObjCmd,	PASSMAINWINDOW|ISSAFE},
    {"pack",		Tk_PackObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"place",		Tk_PlaceObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"raise",		Tk_RaiseObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"selection",	Tk_SelectionObjCmd,	PASSMAINWINDOW},
    {"tk",		(Tcl_ObjCmdProc *) TkInitTkCmd,  USEINITPROC|PASSMAINWINDOW|ISSAFE},
    {"tkwait",		Tk_TkwaitObjCmd,	PASSMAINWINDOW|ISSAFE},
    {"update",		Tk_UpdateObjCmd,	PASSMAINWINDOW|ISSAFE},
    {"winfo",		Tk_WinfoObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"wm",		Tk_WmObjCmd,		PASSMAINWINDOW},

    /*
     * Default widget class commands.
     */

    {"button",		Tk_ButtonObjCmd,	ISSAFE},
    {"canvas",		Tk_CanvasObjCmd,	PASSMAINWINDOW|ISSAFE},
    {"checkbutton",	Tk_CheckbuttonObjCmd,	ISSAFE},
    {"entry",		Tk_EntryObjCmd,		ISSAFE},
    {"frame",		Tk_FrameObjCmd,		ISSAFE},
    {"label",		Tk_LabelObjCmd,		ISSAFE},
    {"labelframe",	Tk_LabelframeObjCmd,	ISSAFE},
    {"listbox",		Tk_ListboxObjCmd,	ISSAFE},
    {"menu",		Tk_MenuObjCmd,	PASSMAINWINDOW},
    {"menubutton",	Tk_MenubuttonObjCmd,	ISSAFE},
    {"message",		Tk_MessageObjCmd,	ISSAFE},
    {"panedwindow",	Tk_PanedWindowObjCmd,	ISSAFE},
    {"radiobutton",	Tk_RadiobuttonObjCmd,	ISSAFE},
    {"scale",		Tk_ScaleObjCmd,		ISSAFE},
    {"scrollbar",	Tk_ScrollbarObjCmd, PASSMAINWINDOW|ISSAFE},
    {"spinbox",		Tk_SpinboxObjCmd,	ISSAFE},
    {"text",		Tk_TextObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"toplevel",	Tk_ToplevelObjCmd,	0},

    /*
     * Classic widget class commands.
     */

    {"::tk::button",	Tk_ButtonObjCmd,	ISSAFE},
    {"::tk::canvas",	Tk_CanvasObjCmd,	PASSMAINWINDOW|ISSAFE},
    {"::tk::checkbutton",Tk_CheckbuttonObjCmd,	ISSAFE},
    {"::tk::entry",	Tk_EntryObjCmd,		ISSAFE},
    {"::tk::frame",	Tk_FrameObjCmd,		ISSAFE},
    {"::tk::label",	Tk_LabelObjCmd,		ISSAFE},
    {"::tk::labelframe",Tk_LabelframeObjCmd,	ISSAFE},
    {"::tk::listbox",	Tk_ListboxObjCmd,	ISSAFE},
    {"::tk::menubutton",Tk_MenubuttonObjCmd,	ISSAFE},
    {"::tk::message",	Tk_MessageObjCmd,	ISSAFE},
    {"::tk::panedwindow",Tk_PanedWindowObjCmd,	ISSAFE},
    {"::tk::radiobutton",Tk_RadiobuttonObjCmd,	ISSAFE},
    {"::tk::scale",	Tk_ScaleObjCmd,		ISSAFE},
    {"::tk::scrollbar",	Tk_ScrollbarObjCmd, PASSMAINWINDOW|ISSAFE},
    {"::tk::spinbox",	Tk_SpinboxObjCmd,	ISSAFE},
    {"::tk::text",	Tk_TextObjCmd,		PASSMAINWINDOW|ISSAFE},
    {"::tk::toplevel",	Tk_ToplevelObjCmd,	0},

    /*
     * Standard dialog support. Note that the Unix/X11 platform implements
     * these commands differently (via the script library).
     */

#if defined(_WIN32) || defined(MAC_OSX_TK)
    {"tk_chooseColor",	Tk_ChooseColorObjCmd,	PASSMAINWINDOW},
    {"tk_chooseDirectory", Tk_ChooseDirectoryObjCmd,WINMACONLY|PASSMAINWINDOW},
    {"tk_getOpenFile",	Tk_GetOpenFileObjCmd,	WINMACONLY|PASSMAINWINDOW},
    {"tk_getSaveFile",	Tk_GetSaveFileObjCmd,	WINMACONLY|PASSMAINWINDOW},
    {"tk_messageBox",	Tk_MessageBoxObjCmd,	PASSMAINWINDOW},
#endif

    /*
     * Misc.
     */

#ifdef MAC_OSX_TK
    {"::tk::unsupported::MacWindowStyle",
			TkUnsupported1ObjCmd,	PASSMAINWINDOW|ISSAFE},
#endif
    {NULL,		NULL,			0}
};

/*
 * Forward declarations to functions defined later in this file:
 */

static Tk_Window	CreateTopLevelWindow(Tcl_Interp *interp,
			    Tk_Window parent, const char *name,
			    const char *screenName, unsigned int flags);
static void		DeleteWindowsExitProc(ClientData clientData);
static TkDisplay *	GetScreen(Tcl_Interp *interp, const char *screenName,
			    int *screenPtr);
static int		Initialize(Tcl_Interp *interp);
static int		NameWindow(Tcl_Interp *interp, TkWindow *winPtr,
			    TkWindow *parentPtr, const char *name);
static void		UnlinkWindow(TkWindow *winPtr);

/*
 *----------------------------------------------------------------------
 *
 * TkCloseDisplay --
 *
 *	Closing the display can lead to order of deletion problems. We defer
 *	it until exit handling for Mac/Win, but since Unix can use many
 *	displays, try and clean it up as best as possible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with the display will be free. The display may
 *	not be referenced at all after this.
 *
 *----------------------------------------------------------------------
 */

static void
TkCloseDisplay(
    TkDisplay *dispPtr)
{
    TkClipCleanup(dispPtr);

    TkpCancelWarp(dispPtr);

    if (dispPtr->name != NULL) {
	ckfree(dispPtr->name);
    }

    if (dispPtr->atomInit) {
	Tcl_DeleteHashTable(&dispPtr->nameTable);
	Tcl_DeleteHashTable(&dispPtr->atomTable);
	dispPtr->atomInit = 0;
    }

    if (dispPtr->errorPtr != NULL) {
	TkErrorHandler *errorPtr;

	for (errorPtr = dispPtr->errorPtr;
		errorPtr != NULL;
		errorPtr = dispPtr->errorPtr) {
	    dispPtr->errorPtr = errorPtr->nextPtr;
	    ckfree(errorPtr);
	}
    }

    TkGCCleanup(dispPtr);

    TkpCloseDisplay(dispPtr);

    /*
     * Delete winTable after TkpCloseDisplay since special windows may need
     * call Tk_DestroyWindow and it checks the winTable.
     */

    Tcl_DeleteHashTable(&dispPtr->winTable);

    ckfree(dispPtr);

    /*
     * There is more to clean up, we leave it at this for the time being.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * CreateTopLevelWindow --
 *
 *	Make a new window that will be at top-level (its parent will be the
 *	root window of a screen).
 *
 * Results:
 *	The return value is a token for the new window, or NULL if an error
 *	prevented the new window from being created. If NULL is returned, an
 *	error message will be left in the interp's result.
 *
 * Side effects:
 *	A new window structure is allocated locally. An X window is NOT
 *	initially created, but will be created the first time the window is
 *	mapped.
 *
 *----------------------------------------------------------------------
 */

static Tk_Window
CreateTopLevelWindow(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window parent,		/* Token for logical parent of new window
				 * (used for naming, options, etc.). May be
				 * NULL. */
    const char *name,		/* Name for new window; if parent is non-NULL,
				 * must be unique among parent's children. */
    const char *screenName,	/* Name of screen on which to create window.
				 * NULL means use DISPLAY environment variable
				 * to determine. Empty string means use
				 * parent's screen, or DISPLAY if no
				 * parent. */
    unsigned int flags)		/* Additional flags to set on the window. */
{
    register TkWindow *winPtr;
    register TkDisplay *dispPtr;
    int screenId;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;

	/*
	 * Create built-in image types.
	 */

	Tk_CreateImageType(&tkBitmapImageType);
	Tk_CreateImageType(&tkPhotoImageType);

	/*
	 * Create built-in photo image formats.
	 */

	Tk_CreatePhotoImageFormat(&tkImgFmtGIF);
	Tk_CreatePhotoImageFormat(&tkImgFmtPNG);
	Tk_CreatePhotoImageFormat(&tkImgFmtPPM);
    }

    if ((parent != NULL) && (screenName != NULL) && (screenName[0] == '\0')) {
	dispPtr = ((TkWindow *) parent)->dispPtr;
	screenId = Tk_ScreenNumber(parent);
    } else {
	dispPtr = GetScreen(interp, screenName, &screenId);
	if (dispPtr == NULL) {
	    return NULL;
	}
    }

    winPtr = TkAllocWindow(dispPtr, screenId, (TkWindow *) parent);

    /*
     * Set the flags specified in the call.
     */

#ifdef TK_USE_INPUT_METHODS
    winPtr->ximGeneration = 0;
#endif /*TK_USE_INPUT_METHODS*/
    winPtr->flags |= flags;

    /*
     * Force the window to use a border pixel instead of border pixmap. This
     * is needed for the case where the window doesn't use the default visual.
     * In this case, the default border is a pixmap inherited from the root
     * window, which won't work because it will have the wrong visual.
     */

    winPtr->dirtyAtts |= CWBorderPixel;

    /*
     * (Need to set the TK_TOP_HIERARCHY flag immediately here; otherwise
     * Tk_DestroyWindow will core dump if it is called before the flag has
     * been set.)
     */

    winPtr->flags |=
	    TK_TOP_HIERARCHY|TK_TOP_LEVEL|TK_HAS_WRAPPER|TK_WIN_MANAGED;

    if (parent != NULL) {
	if (NameWindow(interp, winPtr, (TkWindow *) parent, name) != TCL_OK) {
	    Tk_DestroyWindow((Tk_Window) winPtr);
	    return NULL;
	}
    }
    TkWmNewWindow(winPtr);

    return (Tk_Window) winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetScreen --
 *
 *	Given a string name for a display-plus-screen, find the TkDisplay
 *	structure for the display and return the screen number too.
 *
 * Results:
 *	The return value is a pointer to information about the display, or
 *	NULL if the display couldn't be opened. In this case, an error message
 *	is left in the interp's result. The location at *screenPtr is
 *	overwritten with the screen number parsed from screenName.
 *
 * Side effects:
 *	A new connection is opened to the display if there is no connection
 *	already. A new TkDisplay data structure is also setup, if necessary.
 *
 *----------------------------------------------------------------------
 */

static TkDisplay *
GetScreen(
    Tcl_Interp *interp,		/* Place to leave error message. */
    const char *screenName,	/* Name for screen. NULL or empty means use
				 * DISPLAY envariable. */
    int *screenPtr)		/* Where to store screen number. */
{
    register TkDisplay *dispPtr;
    const char *p;
    int screenId;
    size_t length;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Separate the screen number from the rest of the display name.
     * ScreenName is assumed to have the syntax <display>.<screen> with the
     * dot and the screen being optional.
     */

    screenName = TkGetDefaultScreenName(interp, screenName);
    if (screenName == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"no display name and no $DISPLAY environment variable", -1));
	Tcl_SetErrorCode(interp, "TK", "NO_DISPLAY", NULL);
	return NULL;
    }
    length = strlen(screenName);
    screenId = 0;
    p = screenName+length-1;
    while (isdigit(UCHAR(*p)) && (p != screenName)) {
	p--;
    }
    if ((*p == '.') && (p[1] != '\0')) {
	length = p - screenName;
	screenId = strtoul(p+1, NULL, 10);
    }

    /*
     * See if we already have a connection to this display. If not, then open
     * a new connection.
     */

    for (dispPtr = tsdPtr->displayList; ; dispPtr = dispPtr->nextPtr) {
	if (dispPtr == NULL) {
	    /*
	     * The private function zeros out dispPtr when it is created, so
	     * we only need to initialize the non-zero items.
	     */

	    dispPtr = TkpOpenDisplay(screenName);
	    if (dispPtr == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't connect to display \"%s\"", screenName));
		Tcl_SetErrorCode(interp, "TK", "DISPLAY", "CONNECT", NULL);
		return NULL;
	    }
	    dispPtr->nextPtr = tsdPtr->displayList; /* TkGetDisplayList(); */
	    tsdPtr->displayList = dispPtr;

	    dispPtr->lastEventTime = CurrentTime;
	    dispPtr->bindInfoStale = 1;
	    dispPtr->cursorFont = None;
	    dispPtr->warpWindow = NULL;
	    dispPtr->multipleAtom = None;

	    /*
	     * By default we do want to collapse motion events in
	     * Tk_QueueWindowEvent.
	     */

	    dispPtr->flags |= TK_DISPLAY_COLLAPSE_MOTION_EVENTS;

	    Tcl_InitHashTable(&dispPtr->winTable, TCL_ONE_WORD_KEYS);

	    dispPtr->name = ckalloc(length + 1);
	    strncpy(dispPtr->name, screenName, length);
	    dispPtr->name[length] = '\0';
	    break;
	}
	if ((strncmp(dispPtr->name, screenName, length) == 0)
		&& (dispPtr->name[length] == '\0')) {
	    break;
	}
    }
    if (screenId >= ScreenCount(dispPtr->display)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad screen number \"%d\"", screenId));
	Tcl_SetErrorCode(interp, "TK", "DISPLAY", "SCREEN_NUMBER", NULL);
	return NULL;
    }
    *screenPtr = screenId;
    return dispPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetDisplay --
 *
 *	Given an X display, TkGetDisplay returns the TkDisplay structure for
 *	the display.
 *
 * Results:
 *	The return value is a pointer to information about the display, or
 *	NULL if the display did not have a TkDisplay structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkDisplay *
TkGetDisplay(
    Display *display)		/* X's display pointer */
{
    TkDisplay *dispPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (dispPtr = tsdPtr->displayList; dispPtr != NULL;
	    dispPtr = dispPtr->nextPtr) {
	if (dispPtr->display == display) {
	    break;
	}
    }
    return dispPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkGetDisplayList --
 *
 *	This function returns a pointer to the thread-local list of TkDisplays
 *	corresponding to the open displays.
 *
 * Results:
 *	The return value is a pointer to the first TkDisplay structure in
 *	thread-local-storage.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

TkDisplay *
TkGetDisplayList(void)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    return tsdPtr->displayList;
}

/*
 *--------------------------------------------------------------
 *
 * TkGetMainInfoList --
 *
 *	This function returns a pointer to the list of structures containing
 *	information about all main windows for the current thread.
 *
 * Results:
 *	The return value is a pointer to the first TkMainInfo structure in
 *	thread local storage.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

TkMainInfo *
TkGetMainInfoList(void)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    return tsdPtr->mainWindowList;
}
/*
 *--------------------------------------------------------------
 *
 * TkAllocWindow --
 *
 *	This function creates and initializes a TkWindow structure.
 *
 * Results:
 *	The return value is a pointer to the new window.
 *
 * Side effects:
 *	A new window structure is allocated and all its fields are
 *	initialized.
 *
 *--------------------------------------------------------------
 */

TkWindow *
TkAllocWindow(
    TkDisplay *dispPtr,		/* Display associated with new window. */
    int screenNum,		/* Index of screen for new window. */
    TkWindow *parentPtr)	/* Parent from which this window should
				 * inherit visual information. NULL means use
				 * screen defaults instead of inheriting. */
{
    register TkWindow *winPtr = ckalloc(sizeof(TkWindow));

    winPtr->display = dispPtr->display;
    winPtr->dispPtr = dispPtr;
    winPtr->screenNum = screenNum;
    if ((parentPtr != NULL) && (parentPtr->display == winPtr->display)
	    && (parentPtr->screenNum == winPtr->screenNum)) {
	winPtr->visual = parentPtr->visual;
	winPtr->depth = parentPtr->depth;
    } else {
	winPtr->visual = DefaultVisual(dispPtr->display, screenNum);
	winPtr->depth = DefaultDepth(dispPtr->display, screenNum);
    }
    winPtr->window = None;
    winPtr->childList = NULL;
    winPtr->lastChildPtr = NULL;
    winPtr->parentPtr = NULL;
    winPtr->nextPtr = NULL;
    winPtr->mainPtr = NULL;
    winPtr->pathName = NULL;
    winPtr->nameUid = NULL;
    winPtr->classUid = NULL;
    winPtr->changes = defChanges;
    winPtr->dirtyChanges = CWX|CWY|CWWidth|CWHeight|CWBorderWidth;
    winPtr->atts = defAtts;
    if ((parentPtr != NULL) && (parentPtr->display == winPtr->display)
	    && (parentPtr->screenNum == winPtr->screenNum)) {
	winPtr->atts.colormap = parentPtr->atts.colormap;
    } else {
	winPtr->atts.colormap = DefaultColormap(dispPtr->display, screenNum);
    }
    winPtr->dirtyAtts = CWEventMask|CWColormap|CWBitGravity;
    winPtr->flags = 0;
    winPtr->handlerList = NULL;
#ifdef TK_USE_INPUT_METHODS
    winPtr->ximGeneration = 0;
    winPtr->inputContext = NULL;
#endif /* TK_USE_INPUT_METHODS */
    winPtr->tagPtr = NULL;
    winPtr->numTags = 0;
    winPtr->optionLevel = -1;
    winPtr->selHandlerList = NULL;
    winPtr->geomMgrPtr = NULL;
    winPtr->geomData = NULL;
    winPtr->geomMgrName = NULL;
    winPtr->maintainerPtr = NULL;
    winPtr->reqWidth = winPtr->reqHeight = 1;
    winPtr->internalBorderLeft = 0;
    winPtr->wmInfoPtr = NULL;
    winPtr->classProcsPtr = NULL;
    winPtr->instanceData = NULL;
    winPtr->privatePtr = NULL;
    winPtr->internalBorderRight = 0;
    winPtr->internalBorderTop = 0;
    winPtr->internalBorderBottom = 0;
    winPtr->minReqWidth = 0;
    winPtr->minReqHeight = 0;

    return winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * NameWindow --
 *
 *	This function is invoked to give a window a name and insert the window
 *	into the hierarchy associated with a particular application.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	See above.
 *
 *----------------------------------------------------------------------
 */

static int
NameWindow(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    register TkWindow *winPtr,	/* Window that is to be named and inserted. */
    TkWindow *parentPtr,	/* Pointer to logical parent for winPtr (used
				 * for naming, options, etc.). */
    const char *name)		/* Name for winPtr; must be unique among
				 * parentPtr's children. */
{
#define FIXED_SIZE 200
    char staticSpace[FIXED_SIZE];
    char *pathName;
    int isNew;
    Tcl_HashEntry *hPtr;
    size_t length1, length2;

    /*
     * Setup all the stuff except name right away, then do the name stuff
     * last. This is so that if the name stuff fails, everything else will be
     * properly initialized (needed to destroy the window cleanly after the
     * naming failure).
     */

    winPtr->parentPtr = parentPtr;
    winPtr->nextPtr = NULL;
    if (parentPtr->childList == NULL) {
	parentPtr->childList = winPtr;
    } else {
	parentPtr->lastChildPtr->nextPtr = winPtr;
    }
    parentPtr->lastChildPtr = winPtr;
    winPtr->mainPtr = parentPtr->mainPtr;
    winPtr->mainPtr->refCount++;

    /*
     * If this is an anonymous window (ie, it has no name), just return OK
     * now.
     */

    if (winPtr->flags & TK_ANONYMOUS_WINDOW) {
	return TCL_OK;
    }

    /*
     * Don't permit names that start with an upper-case letter: this will just
     * cause confusion with class names in the option database.
     */

    if (isupper(UCHAR(name[0]))) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"window name starts with an upper-case letter: \"%s\"",
		name));
	Tcl_SetErrorCode(interp, "TK", "VALUE", "WINDOW", "NOTCLASS", NULL);
	return TCL_ERROR;
    }

    /*
     * For non-anonymous windows, set up the window name.
     */

    winPtr->nameUid = Tk_GetUid(name);

    /*
     * To permit names of arbitrary length, must be prepared to malloc a
     * buffer to hold the new path name. To run fast in the common case where
     * names are short, use a fixed-size buffer on the stack.
     */

    length1 = strlen(parentPtr->pathName);
    length2 = strlen(name);
    if ((length1 + length2 + 2) <= FIXED_SIZE) {
	pathName = staticSpace;
    } else {
	pathName = ckalloc(length1 + length2 + 2);
    }
    if (length1 == 1) {
	pathName[0] = '.';
	strcpy(pathName+1, name);
    } else {
	strcpy(pathName, parentPtr->pathName);
	pathName[length1] = '.';
	strcpy(pathName+length1+1, name);
    }
    hPtr = Tcl_CreateHashEntry(&parentPtr->mainPtr->nameTable, pathName,
	    &isNew);
    if (pathName != staticSpace) {
	ckfree(pathName);
    }
    if (!isNew) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"window name \"%s\" already exists in parent", name));
	Tcl_SetErrorCode(interp, "TK", "VALUE", "WINDOW", "EXISTS", NULL);
	return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, winPtr);
    winPtr->pathName = Tcl_GetHashKey(&parentPtr->mainPtr->nameTable, hPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkCreateMainWindow --
 *
 *	Make a new main window. A main window is a special kind of top-level
 *	window used as the outermost window in an application.
 *
 * Results:
 *	The return value is a token for the new window, or NULL if an error
 *	prevented the new window from being created. If NULL is returned, an
 *	error message will be left in the interp's result.
 *
 * Side effects:
 *	A new window structure is allocated locally; "interp" is associated
 *	with the window and registered for "send" commands under "baseName".
 *	BaseName may be extended with an instance number in the form "#2" if
 *	necessary to make it globally unique. Tk-related commands are bound
 *	into interp.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
TkCreateMainWindow(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    const char *screenName,	/* Name of screen on which to create window.
				 * Empty or NULL string means use DISPLAY
				 * environment variable. */
    const char *baseName)	/* Base name for application; usually of the
				 * form "prog instance". */
{
    Tk_Window tkwin;
    int dummy, isSafe;
    Tcl_HashEntry *hPtr;
    register TkMainInfo *mainPtr;
    register TkWindow *winPtr;
    register const TkCmd *cmdPtr;
    ClientData clientData;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Panic if someone updated the TkWindow structure without also updating
     * the Tk_FakeWin structure (or vice versa).
     */

    if (sizeof(TkWindow) != sizeof(Tk_FakeWin)) {
	Tcl_Panic("TkWindow and Tk_FakeWin are not the same size");
    }

    /*
     * Create the basic TkWindow structure.
     */

    tkwin = CreateTopLevelWindow(interp, NULL, baseName,
	    screenName, /* flags */ 0);
    if (tkwin == NULL) {
	return NULL;
    }

    /*
     * Create the TkMainInfo structure for this application, and set up
     * name-related information for the new window.
     */

    winPtr = (TkWindow *) tkwin;
    mainPtr = ckalloc(sizeof(TkMainInfo));
    mainPtr->winPtr = winPtr;
    mainPtr->refCount = 1;
    mainPtr->interp = interp;
    Tcl_InitHashTable(&mainPtr->nameTable, TCL_STRING_KEYS);
    mainPtr->deletionEpoch = 0l;
    TkEventInit();
    TkBindInit(mainPtr);
    TkFontPkgInit(mainPtr);
    TkStylePkgInit(mainPtr);
    mainPtr->tlFocusPtr = NULL;
    mainPtr->displayFocusPtr = NULL;
    mainPtr->optionRootPtr = NULL;
    Tcl_InitHashTable(&mainPtr->imageTable, TCL_STRING_KEYS);
    mainPtr->strictMotif = 0;
    mainPtr->alwaysShowSelection = 0;
    if (Tcl_LinkVar(interp, "tk_strictMotif", (char *) &mainPtr->strictMotif,
	    TCL_LINK_BOOLEAN) != TCL_OK) {
	Tcl_ResetResult(interp);
    }
    if (Tcl_CreateNamespace(interp, "::tk", NULL, NULL) == NULL) {
	Tcl_ResetResult(interp);
    }
    if (Tcl_LinkVar(interp, "::tk::AlwaysShowSelection",
	    (char *) &mainPtr->alwaysShowSelection,
	    TCL_LINK_BOOLEAN) != TCL_OK) {
	Tcl_ResetResult(interp);
    }
    mainPtr->nextPtr = tsdPtr->mainWindowList;
    tsdPtr->mainWindowList = mainPtr;
    winPtr->mainPtr = mainPtr;
    hPtr = Tcl_CreateHashEntry(&mainPtr->nameTable, ".", &dummy);
    Tcl_SetHashValue(hPtr, winPtr);
    winPtr->pathName = Tcl_GetHashKey(&mainPtr->nameTable, hPtr);
    Tcl_InitHashTable(&mainPtr->busyTable, TCL_ONE_WORD_KEYS);

    /*
     * We have just created another Tk application; increment the refcount on
     * the display pointer.
     */

    winPtr->dispPtr->refCount++;

    /*
     * Register the interpreter for "send" purposes.
     */

    winPtr->nameUid = Tk_GetUid(Tk_SetAppName(tkwin, baseName));

    /*
     * Bind in Tk's commands.
     */

    isSafe = Tcl_IsSafe(interp);
    for (cmdPtr = commands; cmdPtr->name != NULL; cmdPtr++) {
	if (cmdPtr->objProc == NULL) {
	    Tcl_Panic("TkCreateMainWindow: builtin command with NULL string and object procs");
	}

#if defined(_WIN32) && !defined(STATIC_BUILD)
	if ((cmdPtr->flags & WINMACONLY) && tclStubsPtr->reserved9) {
	    /*
	     * We are running on Cygwin, so don't use the win32 dialogs.
	     */

	    continue;
	}
#endif /* _WIN32 && !STATIC_BUILD */

	if (cmdPtr->flags & PASSMAINWINDOW) {
	    clientData = tkwin;
	} else {
	    clientData = NULL;
	}
	if (cmdPtr->flags & USEINITPROC) {
	    ((TkInitProc *) cmdPtr->objProc)(interp, clientData);
	} else {
	    Tcl_CreateObjCommand(interp, cmdPtr->name, cmdPtr->objProc,
		    clientData, NULL);
	}
	if (isSafe && !(cmdPtr->flags & ISSAFE)) {
	    Tcl_HideCommand(interp, cmdPtr->name, cmdPtr->name);
	}
    }

    /*
     * Set variables for the intepreter.
     */

    Tcl_SetVar2(interp, "tk_patchLevel", NULL, TK_PATCH_LEVEL, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, "tk_version",    NULL, TK_VERSION,     TCL_GLOBAL_ONLY);

    tsdPtr->numMainWindows++;
    return tkwin;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CreateWindow --
 *
 *	Create a new internal or top-level window as a child of an existing
 *	window.
 *
 * Results:
 *	The return value is a token for the new window. This is not the same
 *	as X's token for the window. If an error occurred in creating the
 *	window (e.g. no such display or screen), then an error message is left
 *	in the interp's result and NULL is returned.
 *
 * Side effects:
 *	A new window structure is allocated locally. An X window is not
 *	initially created, but will be created the first time the window is
 *	mapped.
 *
 *--------------------------------------------------------------
 */

Tk_Window
Tk_CreateWindow(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting.
				 * the interp's result is assumed to be
				 * initialized by the caller. */
    Tk_Window parent,		/* Token for parent of new window. */
    const char *name,		/* Name for new window. Must be unique among
				 * parent's children. */
    const char *screenName)	/* If NULL, new window will be internal on
				 * same screen as its parent. If non-NULL,
				 * gives name of screen on which to create new
				 * window; window will be a top-level
				 * window. */
{
    TkWindow *parentPtr = (TkWindow *) parent;

    if (parentPtr) {
	if (parentPtr->flags & TK_ALREADY_DEAD) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't create window: parent has been destroyed", -1));
	    Tcl_SetErrorCode(interp, "TK", "CREATE", "DEAD_PARENT", NULL);
	    return NULL;
	} else if (parentPtr->flags & TK_CONTAINER) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't create window: its parent has -container = yes",
		    -1));
	    Tcl_SetErrorCode(interp, "TK", "CREATE", "CONTAINER", NULL);
	    return NULL;
	} else if (screenName == NULL) {
	    TkWindow *winPtr = TkAllocWindow(parentPtr->dispPtr,
		    parentPtr->screenNum, parentPtr);

	    if (NameWindow(interp, winPtr, parentPtr, name) != TCL_OK) {
		Tk_DestroyWindow((Tk_Window) winPtr);
		return NULL;
	    }
	    return (Tk_Window) winPtr;
	}
    }
    return CreateTopLevelWindow(interp, parent, name, screenName,
	    /* flags */ 0);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CreateAnonymousWindow --
 *
 *	Create a new internal or top-level window as a child of an existing
 *	window; this window will be anonymous (unnamed), so it will not be
 *	visible at the Tcl level.
 *
 * Results:
 *	The return value is a token for the new window. This is not the same
 *	as X's token for the window. If an error occurred in creating the
 *	window (e.g. no such display or screen), then an error message is left
 *	in the interp's result and NULL is returned.
 *
 * Side effects:
 *	A new window structure is allocated locally. An X window is not
 *	initially created, but will be created the first time the window is
 *	mapped.
 *
 *--------------------------------------------------------------
 */

Tk_Window
Tk_CreateAnonymousWindow(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting.
				 * the interp's result is assumed to be
				 * initialized by the caller. */
    Tk_Window parent,		/* Token for parent of new window. */
    const char *screenName)	/* If NULL, new window will be internal on
				 * same screen as its parent. If non-NULL,
				 * gives name of screen on which to create new
				 * window; window will be a top-level
				 * window. */
{
    TkWindow *parentPtr = (TkWindow *) parent;

    if (parentPtr) {
	if (parentPtr->flags & TK_ALREADY_DEAD) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't create window: parent has been destroyed", -1));
	    Tcl_SetErrorCode(interp, "TK", "CREATE", "DEAD_PARENT", NULL);
	    return NULL;
	} else if (parentPtr->flags & TK_CONTAINER) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't create window: its parent has -container = yes",
		    -1));
	    Tcl_SetErrorCode(interp, "TK", "CREATE", "CONTAINER", NULL);
	    return NULL;
	} else if (screenName == NULL) {
	    TkWindow *winPtr = TkAllocWindow(parentPtr->dispPtr,
		    parentPtr->screenNum, parentPtr);
	    /*
	     * Add the anonymous window flag now, so that NameWindow will
	     * behave correctly.
	     */

	    winPtr->flags |= TK_ANONYMOUS_WINDOW;
	    if (NameWindow(interp, winPtr, parentPtr, NULL) != TCL_OK) {
		Tk_DestroyWindow((Tk_Window) winPtr);
		return NULL;
	    }
	    return (Tk_Window) winPtr;
	}
    }
    return CreateTopLevelWindow(interp, parent, NULL, screenName,
	    TK_ANONYMOUS_WINDOW);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateWindowFromPath --
 *
 *	This function is similar to Tk_CreateWindow except that it uses a path
 *	name to create the window, rather than a parent and a child name.
 *
 * Results:
 *	The return value is a token for the new window. This is not the same
 *	as X's token for the window. If an error occurred in creating the
 *	window (e.g. no such display or screen), then an error message is left
 *	in the interp's result and NULL is returned.
 *
 * Side effects:
 *	A new window structure is allocated locally. An X window is not
 *	initially created, but will be created the first time the window is
 *	mapped.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_CreateWindowFromPath(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting.
				 * the interp's result is assumed to be
				 * initialized by the caller. */
    Tk_Window tkwin,		/* Token for any window in application that is
				 * to contain new window. */
    const char *pathName,	/* Path name for new window within the
				 * application of tkwin. The parent of this
				 * window must already exist, but the window
				 * itself must not exist. */
    const char *screenName)	/* If NULL, new window will be on same screen
				 * as its parent. If non-NULL, gives name of
				 * screen on which to create new window;
				 * window will be a top-level window. */
{
#define FIXED_SPACE 5
    char fixedSpace[FIXED_SPACE+1];
    char *p;
    Tk_Window parent;
    int numChars;

    /*
     * Strip the parent's name out of pathName (it's everything up to the last
     * dot). There are two tricky parts: (a) must copy the parent's name
     * somewhere else to avoid modifying the pathName string (for large names,
     * space for the copy will have to be malloc'ed); (b) must special-case
     * the situation where the parent is ".".
     */

    p = strrchr(pathName, '.');
    if (p == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad window path name \"%s\"", pathName));
	Tcl_SetErrorCode(interp, "TK", "VALUE", "WINDOW_PATH", NULL);
	return NULL;
    }
    numChars = (int) (p-pathName);
    if (numChars > FIXED_SPACE) {
	p = ckalloc(numChars + 1);
    } else {
	p = fixedSpace;
    }
    if (numChars == 0) {
	*p = '.';
	p[1] = '\0';
    } else {
	strncpy(p, pathName, (size_t) numChars);
	p[numChars] = '\0';
    }

    /*
     * Find the parent window.
     */

    parent = Tk_NameToWindow(interp, p, tkwin);
    if (p != fixedSpace) {
	ckfree(p);
    }
    if (parent == NULL) {
	return NULL;
    }
    if (((TkWindow *) parent)->flags & TK_ALREADY_DEAD) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"can't create window: parent has been destroyed", -1));
	Tcl_SetErrorCode(interp, "TK", "CREATE", "DEAD_PARENT", NULL);
	return NULL;
    } else if (((TkWindow *) parent)->flags & TK_CONTAINER) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"can't create window: its parent has -container = yes", -1));
	Tcl_SetErrorCode(interp, "TK", "CREATE", "CONTAINER", NULL);
	return NULL;
    }

    /*
     * Create the window.
     */

    if (screenName == NULL) {
	TkWindow *parentPtr = (TkWindow *) parent;
	TkWindow *winPtr;

	winPtr = TkAllocWindow(parentPtr->dispPtr, parentPtr->screenNum,
		parentPtr);
	if (NameWindow(interp, winPtr, parentPtr, pathName+numChars+1)
		!= TCL_OK) {
	    Tk_DestroyWindow((Tk_Window) winPtr);
	    return NULL;
	}
	return (Tk_Window) winPtr;
    }

    return CreateTopLevelWindow(interp, parent, pathName+numChars+1,
	    screenName, /* flags */ 0);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_DestroyWindow --
 *
 *	Destroy an existing window. After this call, the caller should never
 *	again use the token. Note that this function can be reentered to
 *	destroy a window that was only partially destroyed before a call to
 *	exit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is deleted, along with all of its children. Relevant
 *	callback functions are invoked.
 *
 *--------------------------------------------------------------
 */

void
Tk_DestroyWindow(
    Tk_Window tkwin)		/* Window to destroy. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    XEvent event;
    TkHalfdeadWindow *halfdeadPtr, *prev_halfdeadPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (winPtr->flags & TK_ALREADY_DEAD) {
	/*
	 * A destroy event binding caused the window to be destroyed again.
	 * Ignore the request.
	 */

	return;
    }
    winPtr->flags |= TK_ALREADY_DEAD;

    /*
     * Unless we are cleaning up a half dead window from
     * DeleteWindowsExitProc, add this window to the half dead list.
     */

    if (tsdPtr->halfdeadWindowList &&
	    (tsdPtr->halfdeadWindowList->flags & HD_CLEANUP) &&
	    (tsdPtr->halfdeadWindowList->winPtr == winPtr)) {
	halfdeadPtr = tsdPtr->halfdeadWindowList;
    } else {
	halfdeadPtr = ckalloc(sizeof(TkHalfdeadWindow));
	halfdeadPtr->flags = 0;
	halfdeadPtr->winPtr = winPtr;
	halfdeadPtr->nextPtr = tsdPtr->halfdeadWindowList;
	tsdPtr->halfdeadWindowList = halfdeadPtr;
    }

    /*
     * Some cleanup needs to be done immediately, rather than later, because
     * it needs information that will be destoyed before we get to the main
     * cleanup point. For example, TkFocusDeadWindow needs to access the
     * parentPtr field from a window, but if a Destroy event handler deletes
     * the window's parent this field will be NULL before the main cleanup
     * point is reached.
     */

    if (!(halfdeadPtr->flags & HD_FOCUS)) {
	halfdeadPtr->flags |= HD_FOCUS;
	TkFocusDeadWindow(winPtr);
    }

    /*
     * If this is a main window, remove it from the list of main windows.
     * This needs to be done now (rather than later with all the other main
     * window cleanup) to handle situations where a destroy binding for a
     * window calls "exit". In this case the child window cleanup isn't
     * complete when exit is called. This situation is dealt with using the
     * half dead window list. Windows that are half dead gets cleaned up
     * during exit.
     *
     * Also decrement the display refcount so that if this is the last Tk
     * application in this process on this display, the display can be closed
     * and its data structures deleted.
     */

    if (!(halfdeadPtr->flags & HD_MAIN_WIN) &&
	    winPtr->mainPtr != NULL && winPtr->mainPtr->winPtr == winPtr) {
	halfdeadPtr->flags |= HD_MAIN_WIN;
	dispPtr->refCount--;
	if (tsdPtr->mainWindowList == winPtr->mainPtr) {
	    tsdPtr->mainWindowList = winPtr->mainPtr->nextPtr;
	} else {
	    TkMainInfo *prevPtr;

	    for (prevPtr = tsdPtr->mainWindowList;
		    prevPtr->nextPtr != winPtr->mainPtr;
		    prevPtr = prevPtr->nextPtr) {
		/* Empty loop body. */
	    }
	    prevPtr->nextPtr = winPtr->mainPtr->nextPtr;
	}
	tsdPtr->numMainWindows--;
    }

    /*
     * Recursively destroy children. Note that this child window block may
     * need to be run multiple times in the case where a child window has a
     * Destroy binding that calls exit.
     */

    if (!(halfdeadPtr->flags & HD_DESTROY_COUNT)) {
	halfdeadPtr->flags |= HD_DESTROY_COUNT;
    }

    while (winPtr->childList != NULL) {
	TkWindow *childPtr = winPtr->childList;

	childPtr->flags |= TK_DONT_DESTROY_WINDOW;
	Tk_DestroyWindow((Tk_Window) childPtr);
	if (winPtr->childList == childPtr) {
	    /*
	     * The child didn't remove itself from the child list, so let's
	     * remove it here. This can happen in some strange conditions,
	     * such as when a Destroy event handler for a window destroys the
	     * window's parent.
	     */

	    winPtr->childList = childPtr->nextPtr;
	    childPtr->parentPtr = NULL;
	}
    }
    if ((winPtr->flags & (TK_CONTAINER|TK_BOTH_HALVES))
	    == (TK_CONTAINER|TK_BOTH_HALVES)) {
	/*
	 * This is the container for an embedded application, and the embedded
	 * application is also in this process. Delete the embedded window
	 * in-line here, for the same reasons we delete children in-line
	 * (otherwise, for example, the Tk window may appear to exist even
	 * though its X window is gone; this could cause errors). Special
	 * note: it's possible that the embedded window has already been
	 * deleted, in which case TkpGetOtherWindow will return NULL.
	 */

	TkWindow *childPtr = TkpGetOtherWindow(winPtr);

	if (childPtr != NULL) {
	    childPtr->flags |= TK_DONT_DESTROY_WINDOW;
	    Tk_DestroyWindow((Tk_Window) childPtr);
	}
    }

    /*
     * Generate a DestroyNotify event. In order for the DestroyNotify event to
     * be processed correctly, need to make sure the window exists. This is a
     * bit of a kludge, and may be unnecessarily expensive, but without it no
     * event handlers will get called for windows that don't exist yet.
     *
     * Note: if the window's pathName is NULL and the window is not an
     * anonymous window, it means that the window was not successfully
     * initialized in the first place, so we should not make the window exist
     * or generate the event.
     */

    if (!(halfdeadPtr->flags & HD_DESTROY_EVENT) &&
	    winPtr->pathName != NULL &&
	    !(winPtr->flags & TK_ANONYMOUS_WINDOW)) {
	halfdeadPtr->flags |= HD_DESTROY_EVENT;
	if (winPtr->window == None) {
	    Tk_MakeWindowExist(tkwin);
	}
	event.type = DestroyNotify;
	event.xdestroywindow.serial =
		LastKnownRequestProcessed(winPtr->display);
	event.xdestroywindow.send_event = False;
	event.xdestroywindow.display = winPtr->display;
	event.xdestroywindow.event = winPtr->window;
	event.xdestroywindow.window = winPtr->window;
	Tk_HandleEvent(&event);
    }

    /*
     * No additional bindings that could call exit should be invoked from this
     * point on, so it is safe to remove this window from the half dead list.
     */

    for (prev_halfdeadPtr = NULL,
	    halfdeadPtr = tsdPtr->halfdeadWindowList;
	    halfdeadPtr != NULL; ) {
	if (halfdeadPtr->winPtr == winPtr) {
	    if (prev_halfdeadPtr == NULL) {
		tsdPtr->halfdeadWindowList = halfdeadPtr->nextPtr;
	    } else {
		prev_halfdeadPtr->nextPtr = halfdeadPtr->nextPtr;
	    }
	    ckfree(halfdeadPtr);
	    break;
	}
	prev_halfdeadPtr = halfdeadPtr;
	halfdeadPtr = halfdeadPtr->nextPtr;
    }
    if (halfdeadPtr == NULL) {
	Tcl_Panic("window not found on half dead list");
    }

    /*
     * Cleanup the data structures associated with this window.
     */

    if (winPtr->flags & TK_WIN_MANAGED) {
	TkWmDeadWindow(winPtr);
    } else if (winPtr->flags & TK_WM_COLORMAP_WINDOW) {
	TkWmRemoveFromColormapWindows(winPtr);
    }
    if (winPtr->window != None) {
#if defined(MAC_OSX_TK) || defined(_WIN32)
	XDestroyWindow(winPtr->display, winPtr->window);
#else
	if ((winPtr->flags & TK_TOP_HIERARCHY)
		|| !(winPtr->flags & TK_DONT_DESTROY_WINDOW)) {
	    /*
	     * The parent has already been destroyed and this isn't a
	     * top-level window, so this window will be destroyed implicitly
	     * when the parent's X window is destroyed; it's much faster not
	     * to do an explicit destroy of this X window.
	     */

	    XDestroyWindow(winPtr->display, winPtr->window);
	}
#endif
	Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->winTable,
		(char *) winPtr->window));
	winPtr->window = None;
    }
    UnlinkWindow(winPtr);
    TkEventDeadWindow(winPtr);
#ifdef TK_USE_INPUT_METHODS
    if (winPtr->inputContext != NULL &&
	    winPtr->ximGeneration == winPtr->dispPtr->ximGeneration) {
	XDestroyIC(winPtr->inputContext);
    }
    winPtr->inputContext = NULL;
#endif /* TK_USE_INPUT_METHODS */
    if (winPtr->tagPtr != NULL) {
	TkFreeBindingTags(winPtr);
    }
    TkOptionDeadWindow(winPtr);
    TkSelDeadWindow(winPtr);
    TkGrabDeadWindow(winPtr);
    if (winPtr->geomMgrName != NULL) {
	ckfree(winPtr->geomMgrName);
	winPtr->geomMgrName = NULL;
    }
    if (winPtr->mainPtr != NULL) {
	if (winPtr->pathName != NULL) {
	    Tk_DeleteAllBindings(winPtr->mainPtr->bindingTable,
		    winPtr->pathName);
	    Tcl_DeleteHashEntry(Tcl_FindHashEntry(&winPtr->mainPtr->nameTable,
		    winPtr->pathName));

	    /*
	     * The memory pointed to by pathName has been deallocated. Keep
	     * users from accessing it after the window has been destroyed by
	     * setting it to NULL.
	     */

	    winPtr->pathName = NULL;

	    /*
	     * Invalidate all objects referring to windows with the same main
	     * window.
	     */

	    winPtr->mainPtr->deletionEpoch++;
	}
	if (winPtr->mainPtr->refCount-- <= 1) {
	    register const TkCmd *cmdPtr;

	    /*
	     * We just deleted the last window in the application. Delete the
	     * TkMainInfo structure too and replace all of Tk's commands with
	     * dummy commands that return errors. Also delete the "send"
	     * command to unregister the interpreter.
	     *
	     * NOTE: Only replace the commands it if the interpreter is not
	     * being deleted. If it *is*, the interpreter cleanup will do all
	     * the needed work.
	     */

	    if ((winPtr->mainPtr->interp != NULL) &&
		    !Tcl_InterpDeleted(winPtr->mainPtr->interp)) {
		for (cmdPtr = commands; cmdPtr->name != NULL; cmdPtr++) {
		    Tcl_CreateObjCommand(winPtr->mainPtr->interp, cmdPtr->name,
			    TkDeadAppObjCmd, NULL, NULL);
		}
		Tcl_CreateObjCommand(winPtr->mainPtr->interp, "send",
			TkDeadAppObjCmd, NULL, NULL);
		Tcl_UnlinkVar(winPtr->mainPtr->interp, "tk_strictMotif");
		Tcl_UnlinkVar(winPtr->mainPtr->interp,
			"::tk::AlwaysShowSelection");
	    }

	    Tcl_DeleteHashTable(&winPtr->mainPtr->busyTable);
	    Tcl_DeleteHashTable(&winPtr->mainPtr->nameTable);
	    TkBindFree(winPtr->mainPtr);
	    TkDeleteAllImages(winPtr->mainPtr);
	    TkFontPkgFree(winPtr->mainPtr);
	    TkFocusFree(winPtr->mainPtr);
	    TkStylePkgFree(winPtr->mainPtr);

	    /*
	     * When embedding Tk into other applications, make sure that all
	     * destroy events reach the server. Otherwise the embedding
	     * application may also attempt to destroy the windows, resulting
	     * in an X error
	     */

	    if (winPtr->flags & TK_EMBEDDED) {
		XSync(winPtr->display, False);
	    }
	    ckfree(winPtr->mainPtr);

	    /*
	     * If no other applications are using the display, close the
	     * display now and relinquish its data structures.
	     */

#if !defined(_WIN32) && defined(NOT_YET)
	    if (dispPtr->refCount <= 0) {
		/*
		 * I have disabled this code because on Windows there are
		 * still order dependencies in close-down. All displays and
		 * resources will get closed down properly anyway at exit,
		 * through the exit handler. -- jyl
		 *
		 * Ideally this should be enabled, as unix Tk can use multiple
		 * displays. However, there are order issues still, as well as
		 * the handling of queued events and such that must be
		 * addressed before this can be enabled. The current cleanup
		 * works except for send event issues. -- hobbs 04/2002
		 */

		TkDisplay *theDispPtr, *backDispPtr;

		/*
		 * Splice this display out of the list of displays.
		 */

		for (theDispPtr = tsdPtr->displayList, backDispPtr = NULL;
			(theDispPtr!=winPtr->dispPtr) && (theDispPtr!=NULL);
			theDispPtr = theDispPtr->nextPtr) {
		    backDispPtr = theDispPtr;
		}
		if (theDispPtr == NULL) {
		    Tcl_Panic("could not find display to close!");
		}
		if (backDispPtr == NULL) {
		    tsdPtr->displayList = theDispPtr->nextPtr;
		} else {
		    backDispPtr->nextPtr = theDispPtr->nextPtr;
		}

		/*
		 * Calling XSync creates X server traffic, but addresses a
		 * focus issue on close (but not the send issue). -- hobbs
		 *
		 *	XSync(dispPtr->display, True);
		 */

		/*
		 * Found and spliced it out, now actually do the cleanup.
		 */

		TkCloseDisplay(dispPtr);
	    }
#endif /* !_WIN32 && NOT_YET */
	}
    }
    Tcl_EventuallyFree(winPtr, TCL_DYNAMIC);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_MapWindow --
 *
 *	Map a window within its parent. This may require the window and/or its
 *	parents to actually be created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given window will be mapped. Windows may also be created.
 *
 *--------------------------------------------------------------
 */

void
Tk_MapWindow(
    Tk_Window tkwin)		/* Token for window to map. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    XEvent event;

    if (winPtr->flags & TK_MAPPED) {
	return;
    }
    if (winPtr->window == None) {
	Tk_MakeWindowExist(tkwin);
    }
    /*
     * [Bug 2645457]: the previous call permits events to be processed and can
     * lead to the destruction of the window under some conditions.
     */
    if (winPtr->flags & TK_ALREADY_DEAD) {
	return;
    }
    if (winPtr->flags & TK_WIN_MANAGED) {
	/*
	 * Lots of special processing has to be done for top-level windows.
	 * Let tkWm.c handle everything itself.
	 */

	TkWmMapWindow(winPtr);
	return;
    }
    winPtr->flags |= TK_MAPPED;
    XMapWindow(winPtr->display, winPtr->window);
    event.type = MapNotify;
    event.xmap.serial = LastKnownRequestProcessed(winPtr->display);
    event.xmap.send_event = False;
    event.xmap.display = winPtr->display;
    event.xmap.event = winPtr->window;
    event.xmap.window = winPtr->window;
    event.xmap.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_MakeWindowExist --
 *
 *	Ensure that a particular window actually exists. This function should
 *	not normally need to be invoked from outside the Tk package, but may
 *	be needed if someone wants to manipulate a window before mapping it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the function returns, the X window associated with tkwin is
 *	guaranteed to exist. This may require the window's ancestors to be
 *	created also.
 *
 *--------------------------------------------------------------
 */

void
Tk_MakeWindowExist(
    Tk_Window tkwin)		/* Token for window. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;
    TkWindow *winPtr2;
    Window parent;
    Tcl_HashEntry *hPtr;
    Tk_ClassCreateProc *createProc;
    int isNew;

    if (winPtr->window != None) {
	return;
    }

    if ((winPtr->parentPtr == NULL) || (winPtr->flags & TK_TOP_HIERARCHY)) {
	parent = XRootWindow(winPtr->display, winPtr->screenNum);
    } else {
	if (winPtr->parentPtr->window == None) {
	    Tk_MakeWindowExist((Tk_Window) winPtr->parentPtr);
	}
	parent = winPtr->parentPtr->window;
    }

    createProc = Tk_GetClassProc(winPtr->classProcsPtr, createProc);
    if (createProc != NULL && parent != None) {
	winPtr->window = createProc(tkwin, parent, winPtr->instanceData);
    } else {
	winPtr->window = TkpMakeWindow(winPtr, parent);
    }

    hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable,
	    (char *) winPtr->window, &isNew);
    Tcl_SetHashValue(hPtr, winPtr);
    winPtr->dirtyAtts = 0;
    winPtr->dirtyChanges = 0;

    if (!(winPtr->flags & TK_TOP_HIERARCHY)) {
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
	    if ((winPtr2->window != None)
		    && !(winPtr2->flags & (TK_TOP_HIERARCHY|TK_REPARENTED))) {
		XWindowChanges changes;

		changes.sibling = winPtr2->window;
		changes.stack_mode = Below;
		XConfigureWindow(winPtr->display, winPtr->window,
			CWSibling|CWStackMode, &changes);
		break;
	    }
	}

	/*
	 * If this window has a different colormap than its parent, add the
	 * window to the WM_COLORMAP_WINDOWS property for its top-level.
	 */

	if ((winPtr->parentPtr != NULL) &&
		(winPtr->atts.colormap != winPtr->parentPtr->atts.colormap)) {
	    TkWmAddToColormapWindows(winPtr);
	    winPtr->flags |= TK_WM_COLORMAP_WINDOW;
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
	TkDoConfigureNotify(winPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_UnmapWindow, etc. --
 *
 *	There are several functions under here, each of which mirrors an
 *	existing X function. In addition to performing the functions of the
 *	corresponding function, each function also updates the local window
 *	structure and synthesizes an X event (if the window's structure is
 *	being managed internally).
 *
 * Results:
 *	See the manual entries.
 *
 * Side effects:
 *	See the manual entries.
 *
 *--------------------------------------------------------------
 */

void
Tk_UnmapWindow(
    Tk_Window tkwin)		/* Token for window to unmap. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if (!(winPtr->flags & TK_MAPPED) || (winPtr->flags & TK_ALREADY_DEAD)) {
	return;
    }
    if (winPtr->flags & TK_WIN_MANAGED) {
	/*
	 * Special processing has to be done for top-level windows. Let tkWm.c
	 * handle everything itself.
	 */

	TkWmUnmapWindow(winPtr);
	return;
    }
    winPtr->flags &= ~TK_MAPPED;
    XUnmapWindow(winPtr->display, winPtr->window);
    if (!(winPtr->flags & TK_TOP_HIERARCHY)) {
	XEvent event;

	event.type = UnmapNotify;
	event.xunmap.serial = LastKnownRequestProcessed(winPtr->display);
	event.xunmap.send_event = False;
	event.xunmap.display = winPtr->display;
	event.xunmap.event = winPtr->window;
	event.xunmap.window = winPtr->window;
	event.xunmap.from_configure = False;
	Tk_HandleEvent(&event);
    }
}

void
Tk_ConfigureWindow(
    Tk_Window tkwin,		/* Window to re-configure. */
    unsigned int valueMask,	/* Mask indicating which parts of *valuePtr
				 * are to be used. */
    XWindowChanges *valuePtr)	/* New values. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if (valueMask & CWX) {
	winPtr->changes.x = valuePtr->x;
    }
    if (valueMask & CWY) {
	winPtr->changes.y = valuePtr->y;
    }
    if (valueMask & CWWidth) {
	winPtr->changes.width = valuePtr->width;
    }
    if (valueMask & CWHeight) {
	winPtr->changes.height = valuePtr->height;
    }
    if (valueMask & CWBorderWidth) {
	winPtr->changes.border_width = valuePtr->border_width;
    }
    if (valueMask & (CWSibling|CWStackMode)) {
	Tcl_Panic("Can't set sibling or stack mode from Tk_ConfigureWindow");
    }

    if (winPtr->window != None) {
	XConfigureWindow(winPtr->display, winPtr->window,
		valueMask, valuePtr);
	TkDoConfigureNotify(winPtr);
    } else {
	winPtr->dirtyChanges |= valueMask;
	winPtr->flags |= TK_NEED_CONFIG_NOTIFY;
    }
}

void
Tk_MoveWindow(
    Tk_Window tkwin,		/* Window to move. */
    int x, int y)		/* New location for window (within parent). */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->changes.x = x;
    winPtr->changes.y = y;
    if (winPtr->window != None) {
	XMoveWindow(winPtr->display, winPtr->window, x, y);
	TkDoConfigureNotify(winPtr);
    } else {
	winPtr->dirtyChanges |= CWX|CWY;
	winPtr->flags |= TK_NEED_CONFIG_NOTIFY;
    }
}

void
Tk_ResizeWindow(
    Tk_Window tkwin,		/* Window to resize. */
    int width, int height)	/* New dimensions for window. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->changes.width = (unsigned) width;
    winPtr->changes.height = (unsigned) height;
    if (winPtr->window != None) {
	XResizeWindow(winPtr->display, winPtr->window, (unsigned) width,
		(unsigned) height);
	TkDoConfigureNotify(winPtr);
    } else {
	winPtr->dirtyChanges |= CWWidth|CWHeight;
	winPtr->flags |= TK_NEED_CONFIG_NOTIFY;
    }
}

void
Tk_MoveResizeWindow(
    Tk_Window tkwin,		/* Window to move and resize. */
    int x, int y,		/* New location for window (within parent). */
    int width, int height)	/* New dimensions for window. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->changes.x = x;
    winPtr->changes.y = y;
    winPtr->changes.width = (unsigned) width;
    winPtr->changes.height = (unsigned) height;
    if (winPtr->window != None) {
	XMoveResizeWindow(winPtr->display, winPtr->window, x, y,
		(unsigned) width, (unsigned) height);
	TkDoConfigureNotify(winPtr);
    } else {
	winPtr->dirtyChanges |= CWX|CWY|CWWidth|CWHeight;
	winPtr->flags |= TK_NEED_CONFIG_NOTIFY;
    }
}

void
Tk_SetWindowBorderWidth(
    Tk_Window tkwin,		/* Window to modify. */
    int width)			/* New border width for window. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->changes.border_width = width;
    if (winPtr->window != None) {
	XSetWindowBorderWidth(winPtr->display, winPtr->window,
		(unsigned) width);
	TkDoConfigureNotify(winPtr);
    } else {
	winPtr->dirtyChanges |= CWBorderWidth;
	winPtr->flags |= TK_NEED_CONFIG_NOTIFY;
    }
}

void
Tk_ChangeWindowAttributes(
    Tk_Window tkwin,		/* Window to manipulate. */
    unsigned long valueMask,	/* OR'ed combination of bits, indicating which
				 * fields of *attsPtr are to be used. */
    register XSetWindowAttributes *attsPtr)
				/* New values for some attributes. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if (valueMask & CWBackPixmap) {
	winPtr->atts.background_pixmap = attsPtr->background_pixmap;
    }
    if (valueMask & CWBackPixel) {
	winPtr->atts.background_pixel = attsPtr->background_pixel;
    }
    if (valueMask & CWBorderPixmap) {
	winPtr->atts.border_pixmap = attsPtr->border_pixmap;
    }
    if (valueMask & CWBorderPixel) {
	winPtr->atts.border_pixel = attsPtr->border_pixel;
    }
    if (valueMask & CWBitGravity) {
	winPtr->atts.bit_gravity = attsPtr->bit_gravity;
    }
    if (valueMask & CWWinGravity) {
	winPtr->atts.win_gravity = attsPtr->win_gravity;
    }
    if (valueMask & CWBackingStore) {
	winPtr->atts.backing_store = attsPtr->backing_store;
    }
    if (valueMask & CWBackingPlanes) {
	winPtr->atts.backing_planes = attsPtr->backing_planes;
    }
    if (valueMask & CWBackingPixel) {
	winPtr->atts.backing_pixel = attsPtr->backing_pixel;
    }
    if (valueMask & CWOverrideRedirect) {
	winPtr->atts.override_redirect = attsPtr->override_redirect;
    }
    if (valueMask & CWSaveUnder) {
	winPtr->atts.save_under = attsPtr->save_under;
    }
    if (valueMask & CWEventMask) {
	winPtr->atts.event_mask = attsPtr->event_mask;
    }
    if (valueMask & CWDontPropagate) {
	winPtr->atts.do_not_propagate_mask
		= attsPtr->do_not_propagate_mask;
    }
    if (valueMask & CWColormap) {
	winPtr->atts.colormap = attsPtr->colormap;
    }
    if (valueMask & CWCursor) {
	winPtr->atts.cursor = attsPtr->cursor;
    }

    if (winPtr->window != None) {
	XChangeWindowAttributes(winPtr->display, winPtr->window,
		valueMask, attsPtr);
    } else {
	winPtr->dirtyAtts |= valueMask;
    }
}

void
Tk_SetWindowBackground(
    Tk_Window tkwin,		/* Window to manipulate. */
    unsigned long pixel)	/* Pixel value to use for window's
				 * background. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->atts.background_pixel = pixel;

    if (winPtr->window != None) {
	XSetWindowBackground(winPtr->display, winPtr->window, pixel);
    } else {
	winPtr->dirtyAtts = (winPtr->dirtyAtts & (unsigned) ~CWBackPixmap)
		| CWBackPixel;
    }
}

void
Tk_SetWindowBackgroundPixmap(
    Tk_Window tkwin,		/* Window to manipulate. */
    Pixmap pixmap)		/* Pixmap to use for window's background. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->atts.background_pixmap = pixmap;

    if (winPtr->window != None) {
	XSetWindowBackgroundPixmap(winPtr->display,
		winPtr->window, pixmap);
    } else {
	winPtr->dirtyAtts = (winPtr->dirtyAtts & (unsigned) ~CWBackPixel)
		| CWBackPixmap;
    }
}

void
Tk_SetWindowBorder(
    Tk_Window tkwin,		/* Window to manipulate. */
    unsigned long pixel)	/* Pixel value to use for window's border. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->atts.border_pixel = pixel;

    if (winPtr->window != None) {
	XSetWindowBorder(winPtr->display, winPtr->window, pixel);
    } else {
	winPtr->dirtyAtts = (winPtr->dirtyAtts & (unsigned) ~CWBorderPixmap)
		| CWBorderPixel;
    }
}

void
Tk_SetWindowBorderPixmap(
    Tk_Window tkwin,		/* Window to manipulate. */
    Pixmap pixmap)		/* Pixmap to use for window's border. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->atts.border_pixmap = pixmap;

    if (winPtr->window != None) {
	XSetWindowBorderPixmap(winPtr->display,
		winPtr->window, pixmap);
    } else {
	winPtr->dirtyAtts = (winPtr->dirtyAtts & (unsigned) ~CWBorderPixel)
		| CWBorderPixmap;
    }
}

void
Tk_DefineCursor(
    Tk_Window tkwin,		/* Window to manipulate. */
    Tk_Cursor cursor)		/* Cursor to use for window (may be None). */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

#if defined(MAC_OSX_TK)
    winPtr->atts.cursor = (XCursor) cursor;
#else
    winPtr->atts.cursor = (Cursor) cursor;
#endif

    if (winPtr->window != None) {
	XDefineCursor(winPtr->display, winPtr->window, winPtr->atts.cursor);
    } else {
	winPtr->dirtyAtts = winPtr->dirtyAtts | CWCursor;
    }
}

void
Tk_UndefineCursor(
    Tk_Window tkwin)		/* Window to manipulate. */
{
    Tk_DefineCursor(tkwin, NULL);
}

void
Tk_SetWindowColormap(
    Tk_Window tkwin,		/* Window to manipulate. */
    Colormap colormap)		/* Colormap to use for window. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->atts.colormap = colormap;

    if (winPtr->window != None) {
	XSetWindowColormap(winPtr->display, winPtr->window, colormap);
	if (!(winPtr->flags & TK_WIN_MANAGED)) {
	    TkWmAddToColormapWindows(winPtr);
	    winPtr->flags |= TK_WM_COLORMAP_WINDOW;
	}
    } else {
	winPtr->dirtyAtts |= CWColormap;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetWindowVisual --
 *
 *	This function is called to specify a visual to be used for a Tk window
 *	when it is created. This function, if called at all, must be called
 *	before the X window is created (i.e. before Tk_MakeWindowExist is
 *	called).
 *
 * Results:
 *	The return value is 1 if successful, or 0 if the X window has been
 *	already created.
 *
 * Side effects:
 *	The information given is stored for when the window is created.
 *
 *----------------------------------------------------------------------
 */

int
Tk_SetWindowVisual(
    Tk_Window tkwin,		/* Window to manipulate. */
    Visual *visual,		/* New visual for window. */
    int depth,			/* New depth for window. */
    Colormap colormap)		/* An appropriate colormap for the visual. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if (winPtr->window != None) {
	/* Too late! */
	return 0;
    }

    winPtr->visual = visual;
    winPtr->depth = depth;
    winPtr->atts.colormap = colormap;
    winPtr->dirtyAtts |= CWColormap;

    /*
     * The following code is needed to make sure that the window doesn't
     * inherit the parent's border pixmap, which would result in a BadMatch
     * error.
     */

    if (!(winPtr->dirtyAtts & CWBorderPixmap)) {
	winPtr->dirtyAtts |= CWBorderPixel;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDoConfigureNotify --
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

void
TkDoConfigureNotify(
    register TkWindow *winPtr)	/* Window whose configuration was just
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
 * Tk_SetClass --
 *
 *	This function is used to give a window a class.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new class is stored for tkwin, replacing any existing class for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetClass(
    Tk_Window tkwin,		/* Token for window to assign class. */
    const char *className)	/* New class for tkwin. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->classUid = Tk_GetUid(className);
    if (winPtr->flags & TK_WIN_MANAGED) {
	TkWmSetClass(winPtr);
    }
    TkOptionClassChanged(winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetClassProcs --
 *
 *	This function is used to set the class functions and instance data for
 *	a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new set of class functions and instance data is stored for tkwin,
 *	replacing any existing values.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetClassProcs(
    Tk_Window tkwin,		/* Token for window to modify. */
    const Tk_ClassProcs *procs,	/* Class procs structure. */
    ClientData instanceData)	/* Data to be passed to class functions. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->classProcsPtr = procs;
    winPtr->instanceData = instanceData;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_NameToWindow --
 *
 *	Given a string name for a window, this function returns the token for
 *	the window, if there exists a window corresponding to the given name.
 *
 * Results:
 *	The return result is either a token for the window corresponding to
 *	"name", or else NULL to indicate that there is no such window. In this
 *	case, an error message is left in the interp's result, unless interp
 *      is NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_NameToWindow(
    Tcl_Interp *interp,		/* Where to report errors. */
    const char *pathName,	/* Path name of window. */
    Tk_Window tkwin)		/* Token for window: name is assumed to belong
				 * to the same main window as tkwin. */
{
    Tcl_HashEntry *hPtr;

    if (tkwin == NULL) {
	/*
	 * Either we're not really in Tk, or the main window was destroyed and
	 * we're on our way out of the application.
	 */

	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("NULL main window",-1));
	    Tcl_SetErrorCode(interp, "TK", "NO_MAIN_WINDOW", NULL);
	}
	return NULL;
    }

    hPtr = Tcl_FindHashEntry(&((TkWindow *) tkwin)->mainPtr->nameTable,
	    pathName);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad window path name \"%s\"", pathName));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "WINDOW", pathName,
		    NULL);
	}
	return NULL;
    }
    return Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_IdToWindow --
 *
 *	Given an X display and window ID, this function returns the Tk token
 *	for the window, if there exists a Tk window corresponding to the given
 *	ID.
 *
 * Results:
 *	The return result is either a token for the window corresponding to
 *	the given X id, or else NULL to indicate that there is no such window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_IdToWindow(
    Display *display,		/* X display containing the window. */
    Window window)		/* X window window id. */
{
    TkDisplay *dispPtr;
    Tcl_HashEntry *hPtr;

    for (dispPtr = TkGetDisplayList(); ; dispPtr = dispPtr->nextPtr) {
	if (dispPtr == NULL) {
	    return NULL;
	}
	if (dispPtr->display == display) {
	    break;
	}
    }
    if (window == None) {
	return NULL;
    }

    hPtr = Tcl_FindHashEntry(&dispPtr->winTable, (char *) window);
    if (hPtr == NULL) {
	return NULL;
    }
    return Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DisplayName --
 *
 *	Return the textual name of a window's display.
 *
 * Results:
 *	The return value is the string name of the display associated with
 *	tkwin.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tk_DisplayName(
    Tk_Window tkwin)		/* Window whose display name is desired. */
{
    return ((TkWindow *) tkwin)->dispPtr->name;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_Interp --
 *
 *	Get the Tcl interpreter from a Tk window.
 *
 * Results:
 *	A pointer to the interpreter or NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tk_Interp(
    Tk_Window tkwin)
{
    if (tkwin != NULL && ((TkWindow *) tkwin)->mainPtr != NULL) {
	return ((TkWindow *) tkwin)->mainPtr->interp;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * UnlinkWindow --
 *
 *	This function removes a window from the childList of its parent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is unlinked from its childList.
 *
 *----------------------------------------------------------------------
 */

static void
UnlinkWindow(
    TkWindow *winPtr)		/* Child window to be unlinked. */
{
    TkWindow *prevPtr;

    if (winPtr->parentPtr == NULL) {
	return;
    }
    prevPtr = winPtr->parentPtr->childList;
    if (prevPtr == winPtr) {
	winPtr->parentPtr->childList = winPtr->nextPtr;
	if (winPtr->nextPtr == NULL) {
	    winPtr->parentPtr->lastChildPtr = NULL;
	}
    } else {
	while (prevPtr->nextPtr != winPtr) {
	    prevPtr = prevPtr->nextPtr;
	    if (prevPtr == NULL) {
		Tcl_Panic("UnlinkWindow couldn't find child in parent");
	    }
	}
	prevPtr->nextPtr = winPtr->nextPtr;
	if (winPtr->nextPtr == NULL) {
	    winPtr->parentPtr->lastChildPtr = prevPtr;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_RestackWindow --
 *
 *	Change a window's position in the stacking order.
 *
 * Results:
 *	TCL_OK is normally returned. If other is not a descendant of tkwin's
 *	parent then TCL_ERROR is returned and tkwin is not repositioned.
 *
 * Side effects:
 *	Tkwin is repositioned in the stacking order.
 *
 *----------------------------------------------------------------------
 */

int
Tk_RestackWindow(
    Tk_Window tkwin,		/* Token for window whose position in the
				 * stacking order is to change. */
    int aboveBelow,		/* Indicates new position of tkwin relative to
				 * other; must be Above or Below. */
    Tk_Window other)		/* Tkwin will be moved to a position that puts
				 * it just above or below this window. If NULL
				 * then tkwin goes above or below all windows
				 * in the same parent. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkWindow *otherPtr = (TkWindow *) other;

    /*
     * Special case: if winPtr is a top-level window then just find the
     * top-level ancestor of otherPtr and restack winPtr above otherPtr
     * without changing any of Tk's childLists.
     */

    if (winPtr->flags & TK_WIN_MANAGED) {
	while ((otherPtr != NULL) && !(otherPtr->flags & TK_TOP_HIERARCHY)) {
	    otherPtr = otherPtr->parentPtr;
	}
	TkWmRestackToplevel(winPtr, aboveBelow, otherPtr);
	return TCL_OK;
    }

    /*
     * Find an ancestor of otherPtr that is a sibling of winPtr.
     */

    if (winPtr->parentPtr == NULL) {
	/*
	 * Window is going to be deleted shortly; don't do anything.
	 */

	return TCL_OK;
    }
    if (otherPtr == NULL) {
	if (aboveBelow == Above) {
	    otherPtr = winPtr->parentPtr->lastChildPtr;
	} else {
	    otherPtr = winPtr->parentPtr->childList;
	}
    } else {
	while (winPtr->parentPtr != otherPtr->parentPtr) {
	    if ((otherPtr == NULL) || (otherPtr->flags & TK_TOP_HIERARCHY)) {
		return TCL_ERROR;
	    }
	    otherPtr = otherPtr->parentPtr;
	}
    }
    if (otherPtr == winPtr) {
	return TCL_OK;
    }

    /*
     * Reposition winPtr in the stacking order.
     */

    UnlinkWindow(winPtr);
    if (aboveBelow == Above) {
	winPtr->nextPtr = otherPtr->nextPtr;
	if (winPtr->nextPtr == NULL) {
	    winPtr->parentPtr->lastChildPtr = winPtr;
	}
	otherPtr->nextPtr = winPtr;
    } else {
	TkWindow *prevPtr;

	prevPtr = winPtr->parentPtr->childList;
	if (prevPtr == otherPtr) {
	    winPtr->parentPtr->childList = winPtr;
	} else {
	    while (prevPtr->nextPtr != otherPtr) {
		prevPtr = prevPtr->nextPtr;
	    }
	    prevPtr->nextPtr = winPtr;
	}
	winPtr->nextPtr = otherPtr;
    }

    /*
     * Notify the X server of the change. If winPtr hasn't yet been created
     * then there's no need to tell the X server now, since the stacking order
     * will be handled properly when the window is finally created.
     */

    if (winPtr->window != None) {
	XWindowChanges changes;
	unsigned int mask = CWStackMode;

	changes.stack_mode = Above;
	for (otherPtr = winPtr->nextPtr; otherPtr != NULL;
		otherPtr = otherPtr->nextPtr) {
	    if ((otherPtr->window != None)
		    && !(otherPtr->flags & (TK_TOP_HIERARCHY|TK_REPARENTED))){
		changes.sibling = otherPtr->window;
		changes.stack_mode = Below;
		mask = CWStackMode|CWSibling;
		break;
	    }
	}
	XConfigureWindow(winPtr->display, winPtr->window, mask, &changes);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MainWindow --
 *
 *	Returns the main window for an application.
 *
 * Results:
 *	If interp has a Tk application associated with it, the main window for
 *	the application is returned. Otherwise NULL is returned and an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_MainWindow(
    Tcl_Interp *interp)		/* Interpreter that embodies the application.
				 * Used for error reporting also. */
{
    TkMainInfo *mainPtr;
    ThreadSpecificData *tsdPtr;

    if (interp == NULL) {
	return NULL;
    }
#ifdef USE_TCL_STUBS
    if (tclStubsPtr == NULL) {
	return NULL;
    }
#endif
    tsdPtr = Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    for (mainPtr = tsdPtr->mainWindowList; mainPtr != NULL;
	    mainPtr = mainPtr->nextPtr) {
	if (mainPtr->interp == interp) {
	    return (Tk_Window) mainPtr->winPtr;
	}
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
	    "this isn't a Tk application", -1));
    Tcl_SetErrorCode(interp, "TK", "NO_MAIN_WINDOW", NULL);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_StrictMotif --
 *
 *	Indicates whether strict Motif compliance has been specified for the
 *	given window.
 *
 * Results:
 *	The return value is 1 if strict Motif compliance has been requested
 *	for tkwin's application by setting the tk_strictMotif variable in its
 *	interpreter to a true value. 0 is returned if tk_strictMotif has a
 *	false value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_StrictMotif(
    Tk_Window tkwin)		/* Window whose application is to be
				 * checked. */
{
    return ((TkWindow *) tkwin)->mainPtr->strictMotif;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetNumMainWindows --
 *
 *	This function returns the number of main windows currently open in
 *	this process.
 *
 * Results:
 *	The number of main windows open in this process.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetNumMainWindows(void)
{
    ThreadSpecificData *tsdPtr;

#ifdef USE_TCL_STUBS
    if (tclStubsPtr == NULL) {
	return 0;
    }
#endif

    tsdPtr = Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    return tsdPtr->numMainWindows;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpAlwaysShowSelection --
 *
 *	Indicates whether text/entry widgets should always display
 *	their selection, regardless of window focus.
 *
 * Results:
 *	The return value is 1 if always showing the selection has been
 *	requested for tkwin's application by setting the
 *	::tk::AlwaysShowSelection variable in its interpreter to a true value.
 *	0 is returned if it has a false value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpAlwaysShowSelection(
    Tk_Window tkwin)		/* Window whose application is to be
				 * checked. */
{
    return ((TkWindow *) tkwin)->mainPtr->alwaysShowSelection;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteWindowsExitProc --
 *
 *	This function is invoked as an exit handler. It deletes all of the
 *	main windows in the current thread. We really should be using a thread
 *	local exit handler to delete windows and a process exit handler to
 *	close the display but Tcl does not provide support for this usage.
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
DeleteWindowsExitProc(
    ClientData clientData)	/* tsdPtr when handler was created. */
{
    TkDisplay *dispPtr, *nextPtr;
    Tcl_Interp *interp;
    ThreadSpecificData *tsdPtr = clientData;

    if (tsdPtr == NULL) {
	return;
    }

    /*
     * Finish destroying any windows that are in a half-dead state. We must
     * protect the interpreter while destroying the window, because of
     * <Destroy> bindings which could destroy the interpreter while the window
     * is being deleted. This would leave frames on the call stack pointing at
     * deleted memory, causing core dumps.
     */

    while (tsdPtr->halfdeadWindowList != NULL) {
	interp = tsdPtr->halfdeadWindowList->winPtr->mainPtr->interp;
	Tcl_Preserve(interp);
	tsdPtr->halfdeadWindowList->flags |= HD_CLEANUP;
	tsdPtr->halfdeadWindowList->winPtr->flags &= ~TK_ALREADY_DEAD;
	Tk_DestroyWindow((Tk_Window) tsdPtr->halfdeadWindowList->winPtr);
	Tcl_Release(interp);
    }

    /*
     * Destroy any remaining main windows.
     */

    while (tsdPtr->mainWindowList != NULL) {
	interp = tsdPtr->mainWindowList->interp;
	Tcl_Preserve(interp);
	Tk_DestroyWindow((Tk_Window) tsdPtr->mainWindowList->winPtr);
	Tcl_Release(interp);
    }

    /*
     * Let error handlers catch up before actual close of displays.
     * Must be done before tsdPtr->displayList is cleared, otherwise
     * ErrorProc() in tkError.c cannot associate the pending X errors
     * to the remaining error handlers.
     */

    for (dispPtr = tsdPtr->displayList; dispPtr != NULL;
           dispPtr = dispPtr->nextPtr) {
       XSync(dispPtr->display, False);
    }

    /*
     * Iterate destroying the displays until no more displays remain. It is
     * possible for displays to get recreated during exit by any code that
     * calls GetScreen, so we must destroy these new displays as well as the
     * old ones.
     */

    for (dispPtr = tsdPtr->displayList; dispPtr != NULL;
	    dispPtr = tsdPtr->displayList) {
	/*
	 * Now iterate over the current list of open displays, and first set
	 * the global pointer to NULL so we will be able to notice if any new
	 * displays got created during deletion of the current set. We must
	 * also do this to ensure that Tk_IdToWindow does not find the old
	 * display as it is being destroyed, when it wants to see if it needs
	 * to dispatch a message.
	 */

	for (tsdPtr->displayList = NULL; dispPtr != NULL; dispPtr = nextPtr) {
	    nextPtr = dispPtr->nextPtr;
	    TkCloseDisplay(dispPtr);
	}
    }

    tsdPtr->numMainWindows = 0;
    tsdPtr->mainWindowList = NULL;
    tsdPtr->initialized = 0;
}

#if defined(_WIN32)

static HMODULE tkcygwindll = NULL;

/*
 * Run Tk_MainEx from libtk8.?.dll
 *
 * This function is only ever called from wish8.4.exe, the cygwin port of Tcl.
 * This means that the system encoding is utf-8, so we don't have to do any
 * encoding conversions.
 */

int
TkCygwinMainEx(
    int argc,			/* Number of arguments. */
    char **argv,		/* Array of argument strings. */
    Tcl_AppInitProc *appInitProc,
				/* Application-specific initialization
				 * procedure to call after most initialization
				 * but before starting to execute commands. */
    Tcl_Interp *interp)
{
    WCHAR name[MAX_PATH];
    int len;
    void (*tkmainex)(int, char **, Tcl_AppInitProc *, Tcl_Interp *);

    /* construct "<path>/libtk8.?.dll", from "<path>/tk8?.dll" */
    len = GetModuleFileNameW(Tk_GetHINSTANCE(), name, MAX_PATH);
    name[len-2] = '.';
    name[len-1] = name[len-5];
    wcscpy(name+len, L".dll");
    memcpy(name+len-8, L"libtk8", 6 * sizeof(WCHAR));

    tkcygwindll = LoadLibraryW(name);
    if (!tkcygwindll) {
	/* dll is not present */
	return 0;
    }
    tkmainex = (void (*)(int, char **, Tcl_AppInitProc *, Tcl_Interp *))
	    GetProcAddress(tkcygwindll, "Tk_MainEx");
    if (!tkmainex) {
	return 0;
    }
    tkmainex(argc, argv, appInitProc, interp);
    return 1;
}
#endif /* _WIN32 */

/*
 *----------------------------------------------------------------------
 *
 * Tk_Init --
 *
 *	This function is invoked to add Tk to an interpreter. It incorporates
 *	all of Tk's commands into the interpreter and creates the main window
 *	for a new Tk application. If the interpreter contains a variable
 *	"argv", this function extracts several arguments from that variable,
 *	uses them to configure the main window, and modifies argv to exclude
 *	the arguments (see the "wish" documentation for a list of the
 *	arguments that are extracted).
 *
 * Results:
 *	Returns a standard Tcl completion code and sets the interp's result if
 *	there is an error.
 *
 * Side effects:
 *	Depends on various initialization scripts that get invoked.
 *
 *----------------------------------------------------------------------
 */

int
Tk_Init(
    Tcl_Interp *interp)		/* Interpreter to initialize. */
{
#if defined(_WIN32)
    if (tkcygwindll) {
	int (*tkinit)(Tcl_Interp *);

	tkinit = (int(*)(Tcl_Interp *)) GetProcAddress(tkcygwindll,"Tk_Init");
	if (tkinit) {
	    return tkinit(interp);
	}
    }
#endif /* _WIN32 */
    return Initialize(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SafeInit --
 *
 *	This function is invoked to add Tk to a safe interpreter. It invokes
 *	the internal function that does the real work.
 *
 * Results:
 *	Returns a standard Tcl completion code and sets the interp's result if
 *	there is an error.
 *
 * Side effects:
 *	Depends on various initialization scripts that are invoked.
 *
 *----------------------------------------------------------------------
 */

int
Tk_SafeInit(
    Tcl_Interp *interp)		/* Interpreter to initialize. */
{
    /*
     * Initialize the interpreter with Tk, safely. This removes all the Tk
     * commands that are unsafe.
     *
     * Rationale:
     *
     * - Toplevel and menu are unsafe because they can be used to cover the
     *   entire screen and to steal input from the user.
     * - Continuous ringing of the bell is a nuisance.
     * - Cannot allow access to the clipboard because a malicious script can
     *   replace the contents with the string "rm -r *" and lead to surprises
     *   when the contents of the clipboard are pasted. Similarly, the
     *   selection command is blocked.
     * - Cannot allow send because it can be used to cause unsafe interpreters
     *   to execute commands. The tk command recreates the send command, so
     *   that too must be hidden.
     * - Focus can be used to grab the focus away from another window, in
     *   effect stealing user input. Cannot allow that.
     *   NOTE: We currently do *not* hide focus as it would make it impossible
     *   to provide keyboard input to Tk in a safe interpreter.
     * - Grab can be used to block the user from using any other apps on the
     *   screen.
     * - Tkwait can block the containing process forever. Use bindings,
     *   fileevents and split the protocol into before-the-wait and
     *   after-the-wait parts. More work but necessary.
     * - Wm is unsafe because (if toplevels are allowed, in the future) it can
     *   be used to remove decorations, move windows around, cover the entire
     *   screen etc etc.
     *
     * Current risks:
     *
     * - No CPU time limit, no memory allocation limits, no color limits.
     *   CPU time limits can be imposed by an unsafe master interpreter.
     *
     * The actual code called is the same as Tk_Init but Tcl_IsSafe() is
     * checked at several places to differentiate the two initialisations.
     */

#if defined(_WIN32)
    if (tkcygwindll) {
	int (*tksafeinit)(Tcl_Interp *);

	tksafeinit = (int (*)(Tcl_Interp *))
		GetProcAddress(tkcygwindll, "Tk_SafeInit");
	if (tksafeinit) {
	    return tksafeinit(interp);
	}
    }
#endif /* _WIN32 */
    return Initialize(interp);
}

MODULE_SCOPE const TkStubs tkStubs;

/*
 *----------------------------------------------------------------------
 *
 * Initialize --
 *
 *	The core of the initialization code for Tk, called from Tk_Init and
 *	Tk_SafeInit.
 *
 * Results:
 *	A standard Tcl result. Also leaves an error message in the interp's
 *	result if there was an error.
 *
 * Side effects:
 *	Depends on the initialization scripts that are invoked.
 *
 *----------------------------------------------------------------------
 */

static int
CopyValue(
    ClientData dummy,
    Tcl_Obj *objPtr,
    void *dstPtr)
{
    *(Tcl_Obj **)dstPtr = objPtr;
    return 1;
}

static int
Initialize(
    Tcl_Interp *interp)		/* Interpreter to initialize. */
{
    int code = TCL_OK;
    ThreadSpecificData *tsdPtr;
    Tcl_Obj *value = NULL;
    Tcl_Obj *cmd;

    Tcl_Obj *nameObj = NULL;
    Tcl_Obj *classObj = NULL;
    Tcl_Obj *displayObj = NULL;
    Tcl_Obj *colorMapObj = NULL;
    Tcl_Obj *useObj = NULL;
    Tcl_Obj *visualObj = NULL;
    Tcl_Obj *geometryObj = NULL;

    int sync = 0;

    const Tcl_ArgvInfo table[] = {
	{TCL_ARGV_CONSTANT, "-sync", INT2PTR(1), &sync,
		"Use synchronous mode for display server", NULL},
	{TCL_ARGV_FUNC, "-colormap", CopyValue, &colorMapObj,
		"Colormap for main window", NULL},
	{TCL_ARGV_FUNC, "-display", CopyValue, &displayObj,
		"Display to use", NULL},
	{TCL_ARGV_FUNC, "-geometry", CopyValue, &geometryObj,
		"Initial geometry for window", NULL},
	{TCL_ARGV_FUNC, "-name", CopyValue, &nameObj,
		"Name to use for application", NULL},
	{TCL_ARGV_FUNC, "-visual", CopyValue, &visualObj,
		"Visual for main window", NULL},
	{TCL_ARGV_FUNC, "-use", CopyValue, &useObj,
		"Id of window in which to embed application", NULL},
	TCL_ARGV_AUTO_REST, TCL_ARGV_AUTO_HELP, TCL_ARGV_TABLE_END
    };

    /*
     * Ensure that we are getting a compatible version of Tcl.
     */

    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
	return TCL_ERROR;
    }

    /*
     * Ensure that our obj-types are registered with the Tcl runtime.
     */

    TkRegisterObjTypes();

    tsdPtr = Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * We start by resetting the result because it might not be clean.
     */

    Tcl_ResetResult(interp);

    if (Tcl_IsSafe(interp)) {
	/*
	 * Get the clearance to start Tk and the "argv" parameters from the
	 * master.
	 */

	/*
	 * Step 1 : find the master and construct the interp name (could be a
	 * function if new APIs were ok). We could also construct the path
	 * while walking, but there is no API to get the name of an interp
	 * either.
	 */

	Tcl_Interp *master = interp;

	while (Tcl_IsSafe(master)) {
	    master = Tcl_GetMaster(master);
	    if (master == NULL) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"no controlling master interpreter", -1));
		Tcl_SetErrorCode(interp, "TK", "SAFE", "NO_MASTER", NULL);
		return TCL_ERROR;
	    }
	}

	/*
	 * Construct the name (rewalk...)
	 */

	code = Tcl_GetInterpPath(master, interp);
	if (code != TCL_OK) {
	    Tcl_Panic("Tcl_GetInterpPath broken!");
	}

	/*
	 * Build the command to eval in trusted master.
	 */

	cmd = Tcl_NewListObj(2, NULL);
	Tcl_ListObjAppendElement(NULL, cmd,
		Tcl_NewStringObj("::safe::TkInit", -1));
	Tcl_ListObjAppendElement(NULL, cmd, Tcl_GetObjResult(master));

	/*
	 * Step 2 : Eval in the master. The argument is the *reversed* interp
	 * path of the slave.
	 */

	Tcl_IncrRefCount(cmd);
	code = Tcl_EvalObjEx(master, cmd, 0);
	Tcl_DecrRefCount(cmd);
	Tcl_TransferResult(master, code, interp);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Use the master's result as argv. Note: We don't use the Obj
	 * interfaces to avoid dealing with cross interp refcounting and
	 * changing the code below.
	 */

	value = Tcl_GetObjResult(interp);
    } else {
	/*
	 * If there is an "argv" variable, get its value, extract out relevant
	 * arguments from it, and rewrite the variable without the arguments
	 * that we used.
	 */

	value = Tcl_GetVar2Ex(interp, "argv", NULL, TCL_GLOBAL_ONLY);
    }

    if (value) {
	int objc;
	Tcl_Obj **objv, **rest;
	Tcl_Obj *parseList = Tcl_NewListObj(1, NULL);

	Tcl_ListObjAppendElement(NULL, parseList, Tcl_NewObj());

	Tcl_IncrRefCount(value);
	if (TCL_OK != Tcl_ListObjAppendList(interp, parseList, value) ||
	    TCL_OK != Tcl_ListObjGetElements(NULL, parseList, &objc, &objv) ||
	    TCL_OK != Tcl_ParseArgsObjv(interp, table, &objc, objv, &rest)) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (processing arguments in argv variable)");
	    code = TCL_ERROR;
	}
	if (code == TCL_OK) {
	    Tcl_SetVar2Ex(interp, "argv", NULL,
		    Tcl_NewListObj(objc-1, rest+1), TCL_GLOBAL_ONLY);
	    Tcl_SetVar2Ex(interp, "argc", NULL,
		    Tcl_NewIntObj(objc-1), TCL_GLOBAL_ONLY);
	    ckfree(rest);
	}
	Tcl_DecrRefCount(parseList);
	if (code != TCL_OK) {
	    goto done;
	}
    }

    /*
     * Figure out the application's name and class.
     */

    /*
     * If we got no -name argument, fetch from TkpGetAppName().
     */

    if (nameObj == NULL) {
	Tcl_DString nameDS;

	Tcl_DStringInit(&nameDS);
	TkpGetAppName(interp, &nameDS);
	nameObj = Tcl_NewStringObj(Tcl_DStringValue(&nameDS),
		Tcl_DStringLength(&nameDS));
	Tcl_DStringFree(&nameDS);
    }

    /*
     * The -class argument is always the ToTitle of the -name
     */

    {
	int numBytes;
	const char *bytes = Tcl_GetStringFromObj(nameObj, &numBytes);

	classObj = Tcl_NewStringObj(bytes, numBytes);

	numBytes = Tcl_UtfToTitle(Tcl_GetString(classObj));
	Tcl_SetObjLength(classObj, numBytes);
    }

    /*
     * Create an argument list for creating the top-level window, using the
     * information parsed from argv, if any.
     */

    cmd = Tcl_NewStringObj("toplevel . -class", -1);

    Tcl_ListObjAppendElement(NULL, cmd, classObj);
    classObj = NULL;

    if (displayObj) {
	Tcl_ListObjAppendElement(NULL, cmd, Tcl_NewStringObj("-screen", -1));
	Tcl_ListObjAppendElement(NULL, cmd, displayObj);

	/*
	 * If this is the first application for this process, save the display
	 * name in the DISPLAY environment variable so that it will be
	 * available to subprocesses created by us.
	 */

	if (tsdPtr->numMainWindows == 0) {
	    Tcl_SetVar2Ex(interp, "env", "DISPLAY", displayObj, TCL_GLOBAL_ONLY);
	}
	displayObj = NULL;
    }
    if (colorMapObj) {
	Tcl_ListObjAppendElement(NULL, cmd, Tcl_NewStringObj("-colormap", -1));
	Tcl_ListObjAppendElement(NULL, cmd, colorMapObj);
	colorMapObj = NULL;
    }
    if (useObj) {
	Tcl_ListObjAppendElement(NULL, cmd, Tcl_NewStringObj("-use", -1));
	Tcl_ListObjAppendElement(NULL, cmd, useObj);
	useObj = NULL;
    }
    if (visualObj) {
	Tcl_ListObjAppendElement(NULL, cmd, Tcl_NewStringObj("-visual", -1));
	Tcl_ListObjAppendElement(NULL, cmd, visualObj);
	visualObj = NULL;
    }

    code = TkListCreateFrame(NULL, interp, cmd, 1, nameObj);

    Tcl_DecrRefCount(cmd);

    if (code != TCL_OK) {
	goto done;
    }
    Tcl_ResetResult(interp);
    if (sync) {
	(void)XSynchronize(Tk_Display(Tk_MainWindow(interp)), True);
    }

    /*
     * Set the geometry of the main window, if requested. Put the requested
     * geometry into the "geometry" variable.
     */

    if (geometryObj) {

	Tcl_SetVar2Ex(interp, "geometry", NULL, geometryObj, TCL_GLOBAL_ONLY);

	cmd = Tcl_NewStringObj("wm geometry .", -1);
	Tcl_ListObjAppendElement(NULL, cmd, geometryObj);
	Tcl_IncrRefCount(cmd);
	code = Tcl_EvalObjEx(interp, cmd, 0);
	Tcl_DecrRefCount(cmd);
	geometryObj = NULL;
	if (code != TCL_OK) {
	    goto done;
	}
    }

    /*
     * Provide Tk and its stub table.
     */

    code = Tcl_PkgProvideEx(interp, "Tk", TK_PATCH_LEVEL,
	    (ClientData) &tkStubs);
    if (code != TCL_OK) {
	goto done;
    }

    /*
     * If we were able to provide ourselves as a package, then set the main
     * loop function in Tcl to our main loop proc. This will cause tclsh to be
     * event-aware when Tk is dynamically loaded. This will have no effect in
     * wish, which already is prepared to run the event loop.
     */

    Tcl_SetMainLoop(Tk_MainLoop);

    /*
     * Initialized the themed widget set
     */

    code = Ttk_Init(interp);
    if (code != TCL_OK) {
	goto done;
    }

    /*
     * Invoke platform-specific initialization. Unlock mutex before entering
     * TkpInit, as that may run through the Tk_Init routine again for the
     * console window interpreter.
     */

    code = TkpInit(interp);
    if (code == TCL_OK) {

	/*
	 * In order to find tk.tcl during initialization, we evaluate the
	 * following script.  It calls on the Tcl command [tcl_findLibrary]
	 * to perform the search.  See the docs for that command for details
	 * on where it looks.
	 *
	 * Note that this entire search mechanism can be bypassed by defining
	 * an alternate [tkInit] command before calling Tk_Init().
	 */

	code = Tcl_EvalEx(interp,
"if {[namespace which -command tkInit] eq \"\"} {\n\
  proc tkInit {} {\n\
    global tk_library tk_version tk_patchLevel\n\
      rename tkInit {}\n\
    tcl_findLibrary tk $tk_version $tk_patchLevel tk.tcl TK_LIBRARY tk_library\n\
  }\n\
}\n\
tkInit", -1, 0);
    }
    if (code == TCL_OK) {
	/*
	 * Create exit handlers to delete all windows when the application or
	 * thread exits. The handler need to be invoked before other platform
	 * specific cleanups take place to avoid panics in finalization.
	 */

	TkCreateThreadExitHandler(DeleteWindowsExitProc, tsdPtr);
    }
  done:
    if (value) {
	Tcl_DecrRefCount(value);
	value = NULL;
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PkgInitStubsCheck --
 *
 *	This is a replacement routine for Tk_InitStubs() that is called
 *	from code where -DUSE_TK_STUBS has not been enabled.
 *
 * Results:
 *	Returns the version of a conforming Tk stubs table, or NULL, if
 *	the table version doesn't satisfy the requested requirements,
 *	according to historical practice.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tk_PkgInitStubsCheck(
    Tcl_Interp *interp,
    const char * version,
    int exact)
{
    const char *actualVersion = Tcl_PkgRequireEx(interp, "Tk", version, 0, NULL);

    if (exact && actualVersion) {
	const char *p = version;
	int count = 0;

	while (*p) {
	    count += !isdigit(UCHAR(*p++));
	}
	if (count == 1) {
	    if (0 != strncmp(version, actualVersion, strlen(version))) {
		/* Construct error message */
		Tcl_PkgPresentEx(interp, "Tk", version, 1, NULL);
		return NULL;
	    }
	} else {
	    return Tcl_PkgPresentEx(interp, "Tk", version, 1, NULL);
	}
    }
    return actualVersion;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
