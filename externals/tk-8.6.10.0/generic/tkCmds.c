/*
 * tkCmds.c --
 *
 *	This file contains a collection of Tk-related Tcl commands that didn't
 *	fit in any particular file of the toolkit.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000 Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

#if defined(_WIN32)
#include "tkWinInt.h"
#elif defined(MAC_OSX_TK)
#include "tkMacOSXInt.h"
#else
#include "tkUnixInt.h"
#endif

/*
 * Forward declarations for functions defined later in this file:
 */

static TkWindow *	GetTopHierarchy(Tk_Window tkwin);
static char *		WaitVariableProc(ClientData clientData,
			    Tcl_Interp *interp, const char *name1,
			    const char *name2, int flags);
static void		WaitVisibilityProc(ClientData clientData,
			    XEvent *eventPtr);
static void		WaitWindowProc(ClientData clientData,
			    XEvent *eventPtr);
static int		AppnameCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const *objv);
static int		CaretCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const *objv);
static int		InactiveCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const *objv);
static int		ScalingCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const *objv);
static int		UseinputmethodsCmd(ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const *objv);
static int		WindowingsystemCmd(ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const *objv);

#if defined(_WIN32) || defined(MAC_OSX_TK)
MODULE_SCOPE const TkEnsemble tkFontchooserEnsemble[];
#else
#define tkFontchooserEnsemble NULL
#endif

/*
 * Table of tk subcommand names and implementations.
 */

static const TkEnsemble tkCmdMap[] = {
    {"appname",		AppnameCmd, NULL },
    {"busy",		Tk_BusyObjCmd, NULL },
    {"caret",		CaretCmd, NULL },
    {"inactive",	InactiveCmd, NULL },
    {"scaling",		ScalingCmd, NULL },
    {"useinputmethods",	UseinputmethodsCmd, NULL },
    {"windowingsystem",	WindowingsystemCmd, NULL },
    {"fontchooser",	NULL, tkFontchooserEnsemble},
    {NULL, NULL, NULL}
};

/*
 *----------------------------------------------------------------------
 *
 * Tk_BellObjCmd --
 *
 *	This function is invoked to process the "bell" Tcl command. See the
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
Tk_BellObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const bellOptions[] = {
	"-displayof", "-nice", NULL
    };
    enum options { TK_BELL_DISPLAYOF, TK_BELL_NICE };
    Tk_Window tkwin = clientData;
    int i, index, nice = 0;
    Tk_ErrorHandler handler;

    if (objc > 4) {
    wrongArgs:
	Tcl_WrongNumArgs(interp, 1, objv, "?-displayof window? ?-nice?");
	return TCL_ERROR;
    }

    for (i = 1; i < objc; i++) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], bellOptions,
		sizeof(char *), "option", 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum options) index) {
	case TK_BELL_DISPLAYOF:
	    if (++i >= objc) {
		goto wrongArgs;
	    }
	    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[i]), tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    break;
	case TK_BELL_NICE:
	    nice = 1;
	    break;
	}
    }
    handler = Tk_CreateErrorHandler(Tk_Display(tkwin), -1, -1, -1, NULL, NULL);
    XBell(Tk_Display(tkwin), 0);
    if (!nice) {
	XForceScreenSaver(Tk_Display(tkwin), ScreenSaverReset);
    }
    (void)XFlush(Tk_Display(tkwin));
    Tk_DeleteErrorHandler(handler);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_BindObjCmd --
 *
 *	This function is invoked to process the "bind" Tcl command. See the
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
Tk_BindObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    TkWindow *winPtr;
    ClientData object;
    const char *string;

    if ((objc < 2) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "window ?pattern? ?command?");
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[1]);

    /*
     * Bind tags either a window name or a tag name for the first argument.
     * If the argument starts with ".", assume it is a window; otherwise, it
     * is a tag.
     */

    if (string[0] == '.') {
	winPtr = (TkWindow *) Tk_NameToWindow(interp, string, tkwin);
	if (winPtr == NULL) {
	    return TCL_ERROR;
	}
	object = (ClientData) winPtr->pathName;
    } else {
	winPtr = clientData;
	object = (ClientData) Tk_GetUid(string);
    }

    /*
     * If there are four arguments, the command is modifying a binding. If
     * there are three arguments, the command is querying a binding. If there
     * are only two arguments, the command is querying all the bindings for
     * the given tag/window.
     */

    if (objc == 4) {
	int append = 0;
	unsigned long mask;
	const char *sequence = Tcl_GetString(objv[2]);
	const char *script = Tcl_GetString(objv[3]);

	/*
	 * If the script is null, just delete the binding.
	 */

	if (script[0] == 0) {
	    return Tk_DeleteBinding(interp, winPtr->mainPtr->bindingTable,
		    object, sequence);
	}

	/*
	 * If the script begins with "+", append this script to the existing
	 * binding.
	 */

	if (script[0] == '+') {
	    script++;
	    append = 1;
	}
	mask = Tk_CreateBinding(interp, winPtr->mainPtr->bindingTable,
		object, sequence, script, append);
	if (mask == 0) {
	    return TCL_ERROR;
	}
    } else if (objc == 3) {
	const char *command;

	command = Tk_GetBinding(interp, winPtr->mainPtr->bindingTable,
		object, Tcl_GetString(objv[2]));
	if (command == NULL) {
	    Tcl_ResetResult(interp);
	    return TCL_OK;
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(command, -1));
    } else {
	Tk_GetAllBindings(interp, winPtr->mainPtr->bindingTable, object);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkBindEventProc --
 *
 *	This function is invoked by Tk_HandleEvent for each event; it causes
 *	any appropriate bindings for that event to be invoked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what bindings have been established with the "bind"
 *	command.
 *
 *----------------------------------------------------------------------
 */

void
TkBindEventProc(
    TkWindow *winPtr,		/* Pointer to info about window. */
    XEvent *eventPtr)		/* Information about event. */
{
#define MAX_OBJS 20
    ClientData objects[MAX_OBJS], *objPtr;
    TkWindow *topLevPtr;
    int i, count;
    const char *p;
    Tcl_HashEntry *hPtr;

    if ((winPtr->mainPtr == NULL) || (winPtr->mainPtr->bindingTable == NULL)) {
	return;
    }

    objPtr = objects;
    if (winPtr->numTags != 0) {
	/*
	 * Make a copy of the tags for the window, replacing window names with
	 * pointers to the pathName from the appropriate window.
	 */

	if (winPtr->numTags > MAX_OBJS) {
	    objPtr = ckalloc(winPtr->numTags * sizeof(ClientData));
	}
	for (i = 0; i < winPtr->numTags; i++) {
	    p = winPtr->tagPtr[i];
	    if (*p == '.') {
		hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->nameTable, p);
		if (hPtr != NULL) {
		    p = ((TkWindow *) Tcl_GetHashValue(hPtr))->pathName;
		} else {
		    p = NULL;
		}
	    }
	    objPtr[i] = (ClientData) p;
	}
	count = winPtr->numTags;
    } else {
	objPtr[0] = (ClientData) winPtr->pathName;
	objPtr[1] = (ClientData) winPtr->classUid;
	for (topLevPtr = winPtr;
		(topLevPtr != NULL) && !(topLevPtr->flags & TK_TOP_HIERARCHY);
		topLevPtr = topLevPtr->parentPtr) {
	    /* Empty loop body. */
	}
	if ((winPtr != topLevPtr) && (topLevPtr != NULL)) {
	    count = 4;
	    objPtr[2] = (ClientData) topLevPtr->pathName;
	} else {
	    count = 3;
	}
	objPtr[count-1] = (ClientData) Tk_GetUid("all");
    }
    Tk_BindEvent(winPtr->mainPtr->bindingTable, eventPtr, (Tk_Window) winPtr,
	    count, objPtr);
    if (objPtr != objects) {
	ckfree(objPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_BindtagsObjCmd --
 *
 *	This function is invoked to process the "bindtags" Tcl command. See
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

int
Tk_BindtagsObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    TkWindow *winPtr, *winPtr2;
    int i, length;
    const char *p;
    Tcl_Obj *listPtr, **tags;

    if ((objc < 2) || (objc > 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "window ?taglist?");
	return TCL_ERROR;
    }
    winPtr = (TkWindow *) Tk_NameToWindow(interp, Tcl_GetString(objv[1]),
	    tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    if (objc == 2) {
	listPtr = Tcl_NewObj();
	if (winPtr->numTags == 0) {
	    Tcl_ListObjAppendElement(NULL, listPtr,
		    Tcl_NewStringObj(winPtr->pathName, -1));
	    Tcl_ListObjAppendElement(NULL, listPtr,
		    Tcl_NewStringObj(winPtr->classUid, -1));
	    winPtr2 = winPtr;
	    while ((winPtr2 != NULL) && !(Tk_TopWinHierarchy(winPtr2))) {
		winPtr2 = winPtr2->parentPtr;
	    }
	    if ((winPtr != winPtr2) && (winPtr2 != NULL)) {
		Tcl_ListObjAppendElement(NULL, listPtr,
			Tcl_NewStringObj(winPtr2->pathName, -1));
	    }
	    Tcl_ListObjAppendElement(NULL, listPtr,
		    Tcl_NewStringObj("all", -1));
	} else {
	    for (i = 0; i < winPtr->numTags; i++) {
		Tcl_ListObjAppendElement(NULL, listPtr,
			Tcl_NewStringObj((char *) winPtr->tagPtr[i], -1));
	    }
	}
	Tcl_SetObjResult(interp, listPtr);
	return TCL_OK;
    }
    if (winPtr->tagPtr != NULL) {
	TkFreeBindingTags(winPtr);
    }
    if (Tcl_ListObjGetElements(interp, objv[2], &length, &tags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (length == 0) {
	return TCL_OK;
    }

    winPtr->numTags = length;
    winPtr->tagPtr = ckalloc(length * sizeof(ClientData));
    for (i = 0; i < length; i++) {
	p = Tcl_GetString(tags[i]);
	if (p[0] == '.') {
	    char *copy;

	    /*
	     * Handle names starting with "." specially: store a malloc'ed
	     * string, rather than a Uid; at event time we'll look up the name
	     * in the window table and use the corresponding window, if there
	     * is one.
	     */

	    copy = ckalloc(strlen(p) + 1);
	    strcpy(copy, p);
	    winPtr->tagPtr[i] = (ClientData) copy;
	} else {
	    winPtr->tagPtr[i] = (ClientData) Tk_GetUid(p);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFreeBindingTags --
 *
 *	This function is called to free all of the binding tags associated
 *	with a window; typically it is only invoked where there are
 *	window-specific tags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any binding tags for winPtr are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkFreeBindingTags(
    TkWindow *winPtr)		/* Window whose tags are to be released. */
{
    int i;
    const char *p;

    for (i = 0; i < winPtr->numTags; i++) {
	p = winPtr->tagPtr[i];
	if (*p == '.') {
	    /*
	     * Names starting with "." are malloced rather than Uids, so they
	     * have to be freed.
	     */

	    ckfree((char *)p);
	}
    }
    ckfree(winPtr->tagPtr);
    winPtr->numTags = 0;
    winPtr->tagPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DestroyObjCmd --
 *
 *	This function is invoked to process the "destroy" Tcl command. See the
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
Tk_DestroyObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window window;
    Tk_Window tkwin = clientData;
    int i;

    for (i = 1; i < objc; i++) {
	window = Tk_NameToWindow(interp, Tcl_GetString(objv[i]), tkwin);
	if (window == NULL) {
	    Tcl_ResetResult(interp);
	    continue;
	}
	Tk_DestroyWindow(window);
	if (window == tkwin) {
	    /*
	     * We just deleted the main window for the application! This makes
	     * it impossible to do anything more (tkwin isn't valid anymore).
	     */

	    break;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_LowerObjCmd --
 *
 *	This function is invoked to process the "lower" Tcl command. See the
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

	/* ARGSUSED */
int
Tk_LowerObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window mainwin = clientData;
    Tk_Window tkwin, other;

    if ((objc != 2) && (objc != 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "window ?belowThis?");
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[1]), mainwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (objc == 2) {
	other = NULL;
    } else {
	other = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), mainwin);
	if (other == NULL) {
	    return TCL_ERROR;
	}
    }
    if (Tk_RestackWindow(tkwin, Below, other) != TCL_OK) {
	if (other) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't lower \"%s\" below \"%s\"",
		    Tcl_GetString(objv[1]), Tcl_GetString(objv[2])));
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't lower \"%s\" to bottom", Tcl_GetString(objv[1])));
	}
	Tcl_SetErrorCode(interp, "TK", "RESTACK", "LOWER", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_RaiseObjCmd --
 *
 *	This function is invoked to process the "raise" Tcl command. See the
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

	/* ARGSUSED */
int
Tk_RaiseObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window mainwin = clientData;
    Tk_Window tkwin, other;

    if ((objc != 2) && (objc != 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "window ?aboveThis?");
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[1]), mainwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (objc == 2) {
	other = NULL;
    } else {
	other = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), mainwin);
	if (other == NULL) {
	    return TCL_ERROR;
	}
    }
    if (Tk_RestackWindow(tkwin, Above, other) != TCL_OK) {
	if (other) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't raise \"%s\" above \"%s\"",
		    Tcl_GetString(objv[1]), Tcl_GetString(objv[2])));
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't raise \"%s\" to top", Tcl_GetString(objv[1])));
	}
	Tcl_SetErrorCode(interp, "TK", "RESTACK", "RAISE", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TkInitTkCmd --
 *
 *	Set up the tk ensemble.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */

int
TkInitTkCmd(
    Tcl_Interp *interp,
    ClientData clientData)
{
    TkMakeEnsemble(interp, "::", "tk", clientData, tkCmdMap);
#if defined(_WIN32) || defined(MAC_OSX_TK)
    TkInitFontchooser(interp, clientData);
#endif
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AppnameCmd, CaretCmd, ScalingCmd, UseinputmethodsCmd,
 * WindowingsystemCmd, InactiveCmd --
 *
 *	These functions are invoked to process the "tk" ensemble subcommands.
 *	See the user documentation for details on what they do.
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
AppnameCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    TkWindow *winPtr;
    const char *string;

    if (Tcl_IsSafe(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"appname not accessible in a safe interpreter", -1));
	Tcl_SetErrorCode(interp, "TK", "SAFE", "APPLICATION", NULL);
	return TCL_ERROR;
    }

    winPtr = (TkWindow *) tkwin;

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?newName?");
	return TCL_ERROR;
    }
    if (objc == 2) {
	string = Tcl_GetString(objv[1]);
	winPtr->nameUid = Tk_GetUid(Tk_SetAppName(tkwin, string));
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(winPtr->nameUid, -1));
    return TCL_OK;
}

int
CaretCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    int index;
    Tcl_Obj *objPtr;
    TkCaret *caretPtr;
    Tk_Window window;
    static const char *const caretStrings[] = {
	"-x",	"-y", "-height", NULL
    };
    enum caretOptions {
	TK_CARET_X, TK_CARET_Y, TK_CARET_HEIGHT
    };

    if ((objc < 2) || ((objc > 3) && !!(objc & 1))) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"window ?-x x? ?-y y? ?-height height?");
	return TCL_ERROR;
    }
    window = Tk_NameToWindow(interp, Tcl_GetString(objv[1]), tkwin);
    if (window == NULL) {
	return TCL_ERROR;
    }
    caretPtr = &(((TkWindow *) window)->dispPtr->caret);
    if (objc == 2) {
	/*
	 * Return all the current values
	 */

	objPtr = Tcl_NewObj();
	Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewStringObj("-height", 7));
	Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewIntObj(caretPtr->height));
	Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewStringObj("-x", 2));
	Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewIntObj(caretPtr->x));
	Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewStringObj("-y", 2));
	Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewIntObj(caretPtr->y));
	Tcl_SetObjResult(interp, objPtr);
    } else if (objc == 3) {
	int value;

	/*
	 * Return the current value of the selected option
	 */

	if (Tcl_GetIndexFromObj(interp, objv[2], caretStrings,
		"caret option", 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == TK_CARET_X) {
	    value = caretPtr->x;
	} else if (index == TK_CARET_Y) {
	    value = caretPtr->y;
	} else /* if (index == TK_CARET_HEIGHT) -- last case */ {
	    value = caretPtr->height;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(value));
    } else {
	int i, value, x = 0, y = 0, height = -1;

	for (i = 2; i < objc; i += 2) {
	    if ((Tcl_GetIndexFromObj(interp, objv[i], caretStrings,
		    "caret option", 0, &index) != TCL_OK) ||
		    Tcl_GetIntFromObj(interp,objv[i+1],&value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (index == TK_CARET_X) {
		x = value;
	    } else if (index == TK_CARET_Y) {
		y = value;
	    } else /* if (index == TK_CARET_HEIGHT) -- last case */ {
		height = value;
	    }
	}
	if (height < 0) {
	    height = Tk_Height(window);
	}
	Tk_SetCaretPos(window, x, y, height);
    }
    return TCL_OK;
}

int
ScalingCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    Screen *screenPtr;
    int skip, width, height;
    double d;

    if (Tcl_IsSafe(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"scaling not accessible in a safe interpreter", -1));
	Tcl_SetErrorCode(interp, "TK", "SAFE", "SCALING", NULL);
	return TCL_ERROR;
    }

    skip = TkGetDisplayOf(interp, objc - 1, objv + 1, &tkwin);
    if (skip < 0) {
	return TCL_ERROR;
    }
    screenPtr = Tk_Screen(tkwin);
    if (objc - skip == 1) {
	d = 25.4 / 72;
	d *= WidthOfScreen(screenPtr);
	d /= WidthMMOfScreen(screenPtr);
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(d));
    } else if (objc - skip == 2) {
	if (Tcl_GetDoubleFromObj(interp, objv[1+skip], &d) != TCL_OK) {
	    return TCL_ERROR;
	}
	d = (25.4 / 72) / d;
	width = (int) (d * WidthOfScreen(screenPtr) + 0.5);
	if (width <= 0) {
	    width = 1;
	}
	height = (int) (d * HeightOfScreen(screenPtr) + 0.5);
	if (height <= 0) {
	    height = 1;
	}
	WidthMMOfScreen(screenPtr) = width;
	HeightMMOfScreen(screenPtr) = height;
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?-displayof window? ?factor?");
	return TCL_ERROR;
    }
    return TCL_OK;
}

int
UseinputmethodsCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    TkDisplay *dispPtr;
    int skip;

    if (Tcl_IsSafe(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"useinputmethods not accessible in a safe interpreter", -1));
	Tcl_SetErrorCode(interp, "TK", "SAFE", "INPUT_METHODS", NULL);
	return TCL_ERROR;
    }

    skip = TkGetDisplayOf(interp, objc-1, objv+1, &tkwin);
    if (skip < 0) {
	return TCL_ERROR;
    }
    dispPtr = ((TkWindow *) tkwin)->dispPtr;
    if ((objc - skip) == 2) {
	/*
	 * In the case where TK_USE_INPUT_METHODS is not defined, this
	 * will be ignored and we will always return 0. That will indicate
	 * to the user that input methods are just not available.
	 */

	int boolVal;

	if (Tcl_GetBooleanFromObj(interp, objv[1+skip],
		&boolVal) != TCL_OK) {
	    return TCL_ERROR;
	}
#ifdef TK_USE_INPUT_METHODS
	if (boolVal) {
	    dispPtr->flags |= TK_DISPLAY_USE_IM;
	} else {
	    dispPtr->flags &= ~TK_DISPLAY_USE_IM;
	}
#endif /* TK_USE_INPUT_METHODS */
    } else if ((objc - skip) != 1) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-displayof window? ?boolean?");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
	    Tcl_NewBooleanObj(dispPtr->flags & TK_DISPLAY_USE_IM));
    return TCL_OK;
}

int
WindowingsystemCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *windowingsystem;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }
#if defined(_WIN32)
    windowingsystem = "win32";
#elif defined(MAC_OSX_TK)
    windowingsystem = "aqua";
#else
    windowingsystem = "x11";
#endif
    Tcl_SetObjResult(interp, Tcl_NewStringObj(windowingsystem, -1));
    return TCL_OK;
}

int
InactiveCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    int skip = TkGetDisplayOf(interp, objc - 1, objv + 1, &tkwin);

    if (skip < 0) {
	return TCL_ERROR;
    }
    if (objc - skip == 1) {
	long inactive;

	inactive = (Tcl_IsSafe(interp) ? -1 :
		Tk_GetUserInactiveTime(Tk_Display(tkwin)));
	Tcl_SetObjResult(interp, Tcl_NewLongObj(inactive));
    } else if (objc - skip == 2) {
	const char *string;

	string = Tcl_GetString(objv[objc-1]);
	if (strcmp(string, "reset") != 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad option \"%s\": must be reset", string));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INDEX", "option",
		    string, NULL);
	    return TCL_ERROR;
	}
	if (Tcl_IsSafe(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "resetting the user inactivity timer "
		    "is not allowed in a safe interpreter", -1));
	    Tcl_SetErrorCode(interp, "TK", "SAFE", "INACTIVITY_TIMER", NULL);
	    return TCL_ERROR;
	}
	Tk_ResetUserInactiveTime(Tk_Display(tkwin));
	Tcl_ResetResult(interp);
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?-displayof window? ?reset?");
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TkwaitObjCmd --
 *
 *	This function is invoked to process the "tkwait" Tcl command. See the
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

	/* ARGSUSED */
int
Tk_TkwaitObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    int done, index;
    int code = TCL_OK;
    static const char *const optionStrings[] = {
	"variable", "visibility", "window", NULL
    };
    enum options {
	TKWAIT_VARIABLE, TKWAIT_VISIBILITY, TKWAIT_WINDOW
    };

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "variable|visibility|window name");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case TKWAIT_VARIABLE:
	if (Tcl_TraceVar2(interp, Tcl_GetString(objv[2]),
		NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		WaitVariableProc, &done) != TCL_OK) {
	    return TCL_ERROR;
	}
	done = 0;
	while (!done) {
	    if (Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG) == TCL_ERROR) {
		code = TCL_ERROR;
		break;
	    }
	    Tcl_DoOneEvent(0);
	}
	Tcl_UntraceVar2(interp, Tcl_GetString(objv[2]),
		NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		WaitVariableProc, &done);
	break;

    case TKWAIT_VISIBILITY: {
	Tk_Window window;

	window = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	Tk_CreateEventHandler(window,
		VisibilityChangeMask|StructureNotifyMask,
		WaitVisibilityProc, &done);
	done = 0;
	while (!done) {
	    if (Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG) == TCL_ERROR) {
		code = TCL_ERROR;
		break;
	    }
	    Tcl_DoOneEvent(0);
	}
	if ((done != 0) && (done != 1)) {
	    /*
	     * Note that we do not delete the event handler because it was
	     * deleted automatically when the window was destroyed.
	     */

	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" was deleted before its visibility changed",
		    Tcl_GetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TK", "WAIT", "PREMATURE", NULL);
	    return TCL_ERROR;
	}
	Tk_DeleteEventHandler(window,
		VisibilityChangeMask|StructureNotifyMask,
		WaitVisibilityProc, &done);
	break;
    }

    case TKWAIT_WINDOW: {
	Tk_Window window;

	window = Tk_NameToWindow(interp, Tcl_GetString(objv[2]), tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	Tk_CreateEventHandler(window, StructureNotifyMask,
		WaitWindowProc, &done);
	done = 0;
	while (!done) {
	    if (Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG) == TCL_ERROR) {
		code = TCL_ERROR;
		break;
	    }
	    Tcl_DoOneEvent(0);
	}

	/*
	 * Note: normally there's no need to delete the event handler. It was
	 * deleted automatically when the window was destroyed; however, if
	 * the wait operation was canceled, we need to delete it.
	 */

	if (done == 0) {
	    Tk_DeleteEventHandler(window, StructureNotifyMask,
		    WaitWindowProc, &done);
	}
	break;
    }
    }

    /*
     * Clear out the interpreter's result, since it may have been set by event
     * handlers. This is skipped if an error occurred above, such as the wait
     * operation being canceled.
     */

    if (code == TCL_OK)
    Tcl_ResetResult(interp);

    return code;
}

	/* ARGSUSED */
static char *
WaitVariableProc(
    ClientData clientData,	/* Pointer to integer to set to 1. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Name of variable. */
    const char *name2,		/* Second part of variable name. */
    int flags)			/* Information about what happened. */
{
    int *donePtr = clientData;

    *donePtr = 1;
    return NULL;
}

	/*ARGSUSED*/
static void
WaitVisibilityProc(
    ClientData clientData,	/* Pointer to integer to set to 1. */
    XEvent *eventPtr)		/* Information about event (not used). */
{
    int *donePtr = clientData;

    if (eventPtr->type == VisibilityNotify) {
	*donePtr = 1;
    } else if (eventPtr->type == DestroyNotify) {
	*donePtr = 2;
    }
}

static void
WaitWindowProc(
    ClientData clientData,	/* Pointer to integer to set to 1. */
    XEvent *eventPtr)		/* Information about event. */
{
    int *donePtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	*donePtr = 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_UpdateObjCmd --
 *
 *	This function is invoked to process the "update" Tcl command. See the
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

	/* ARGSUSED */
int
Tk_UpdateObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const updateOptions[] = {"idletasks", NULL};
    int flags, index;
    TkDisplay *dispPtr;
    int code = TCL_OK;

    if (objc == 1) {
	flags = TCL_DONT_WAIT;
    } else if (objc == 2) {
	if (Tcl_GetIndexFromObj(interp, objv[1], updateOptions, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	flags = TCL_IDLE_EVENTS;
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?idletasks?");
	return TCL_ERROR;
    }

    /*
     * Handle all pending events, sync all displays, and repeat over and over
     * again until all pending events have been handled. Special note: it's
     * possible that the entire application could be destroyed by an event
     * handler that occurs during the update. Thus, don't use any information
     * from tkwin after calling Tcl_DoOneEvent.
     */

    while (1) {
	while (Tcl_DoOneEvent(flags) != 0) {
	    if (Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG) == TCL_ERROR) {
		code = TCL_ERROR;
		break;
	    }
	}

	/*
	 * If event processing was canceled proceed no further.
	 */

	if (code == TCL_ERROR)
	    break;

	for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
		dispPtr = dispPtr->nextPtr) {
	    XSync(dispPtr->display, False);
	}

	/*
	 * Check again if event processing has been canceled because the inner
	 * loop (above) may not have checked (i.e. no events were processed and
	 * the loop body was skipped).
	 */

	if (Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG) == TCL_ERROR) {
	    code = TCL_ERROR;
	    break;
	}

	if (Tcl_DoOneEvent(flags) == 0) {
	    break;
	}
    }

    /*
     * Must clear the interpreter's result because event handlers could have
     * executed commands. This is skipped if an error occurred above, such as
     * the wait operation being canceled.
     */

    if (code == TCL_OK)
    Tcl_ResetResult(interp);

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_WinfoObjCmd --
 *
 *	This function is invoked to process the "winfo" Tcl command. See the
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
Tk_WinfoObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int index, x, y, width, height, useX, useY, c_class, skip;
    const char *string;
    TkWindow *winPtr;
    Tk_Window tkwin = clientData;

    static const TkStateMap visualMap[] = {
	{PseudoColor,	"pseudocolor"},
	{GrayScale,	"grayscale"},
	{DirectColor,	"directcolor"},
	{TrueColor,	"truecolor"},
	{StaticColor,	"staticcolor"},
	{StaticGray,	"staticgray"},
	{-1,		NULL}
    };
    static const char *const optionStrings[] = {
	"cells",	"children",	"class",	"colormapfull",
	"depth",	"geometry",	"height",	"id",
	"ismapped",	"manager",	"name",		"parent",
	"pointerx",	"pointery",	"pointerxy",	"reqheight",
	"reqwidth",	"rootx",	"rooty",	"screen",
	"screencells",	"screendepth",	"screenheight",	"screenwidth",
	"screenmmheight","screenmmwidth","screenvisual","server",
	"toplevel",	"viewable",	"visual",	"visualid",
	"vrootheight",	"vrootwidth",	"vrootx",	"vrooty",
	"width",	"x",		"y",

	"atom",		"atomname",	"containing",	"interps",
	"pathname",

	"exists",	"fpixels",	"pixels",	"rgb",
	"visualsavailable",

	NULL
    };
    enum options {
	WIN_CELLS,	WIN_CHILDREN,	WIN_CLASS,	WIN_COLORMAPFULL,
	WIN_DEPTH,	WIN_GEOMETRY,	WIN_HEIGHT,	WIN_ID,
	WIN_ISMAPPED,	WIN_MANAGER,	WIN_NAME,	WIN_PARENT,
	WIN_POINTERX,	WIN_POINTERY,	WIN_POINTERXY,	WIN_REQHEIGHT,
	WIN_REQWIDTH,	WIN_ROOTX,	WIN_ROOTY,	WIN_SCREEN,
	WIN_SCREENCELLS,WIN_SCREENDEPTH,WIN_SCREENHEIGHT,WIN_SCREENWIDTH,
	WIN_SCREENMMHEIGHT,WIN_SCREENMMWIDTH,WIN_SCREENVISUAL,WIN_SERVER,
	WIN_TOPLEVEL,	WIN_VIEWABLE,	WIN_VISUAL,	WIN_VISUALID,
	WIN_VROOTHEIGHT,WIN_VROOTWIDTH,	WIN_VROOTX,	WIN_VROOTY,
	WIN_WIDTH,	WIN_X,		WIN_Y,

	WIN_ATOM,	WIN_ATOMNAME,	WIN_CONTAINING,	WIN_INTERPS,
	WIN_PATHNAME,

	WIN_EXISTS,	WIN_FPIXELS,	WIN_PIXELS,	WIN_RGB,
	WIN_VISUALSAVAILABLE
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (index < WIN_ATOM) {
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[2]);
	tkwin = Tk_NameToWindow(interp, string, tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    }
    winPtr = (TkWindow *) tkwin;

    switch ((enum options) index) {
    case WIN_CELLS:
	Tcl_SetObjResult(interp,
		Tcl_NewIntObj(Tk_Visual(tkwin)->map_entries));
	break;
    case WIN_CHILDREN: {
	Tcl_Obj *strPtr, *resultPtr = Tcl_NewObj();

	winPtr = winPtr->childList;
	for ( ; winPtr != NULL; winPtr = winPtr->nextPtr) {
	    if (!(winPtr->flags & TK_ANONYMOUS_WINDOW)) {
		strPtr = Tcl_NewStringObj(winPtr->pathName, -1);
		Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
	    }
	}
	Tcl_SetObjResult(interp, resultPtr);
	break;
    }
    case WIN_CLASS:
	Tcl_SetObjResult(interp, Tcl_NewStringObj(Tk_Class(tkwin), -1));
	break;
    case WIN_COLORMAPFULL:
	Tcl_SetObjResult(interp,
		Tcl_NewBooleanObj(TkpCmapStressed(tkwin,Tk_Colormap(tkwin))));
	break;
    case WIN_DEPTH:
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tk_Depth(tkwin)));
	break;
    case WIN_GEOMETRY:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("%dx%d+%d+%d",
		Tk_Width(tkwin), Tk_Height(tkwin), Tk_X(tkwin), Tk_Y(tkwin)));
	break;
    case WIN_HEIGHT:
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tk_Height(tkwin)));
	break;
    case WIN_ID: {
	char buf[TCL_INTEGER_SPACE];

	Tk_MakeWindowExist(tkwin);
	TkpPrintWindowId(buf, Tk_WindowId(tkwin));
	Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, -1));
	break;
    }
    case WIN_ISMAPPED:
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(Tk_IsMapped(tkwin)));
	break;
    case WIN_MANAGER:
	if (winPtr->geomMgrPtr != NULL) {
	    Tcl_SetObjResult(interp,
		    Tcl_NewStringObj(winPtr->geomMgrPtr->name, -1));
	}
	break;
    case WIN_NAME:
	Tcl_SetObjResult(interp, Tcl_NewStringObj(Tk_Name(tkwin), -1));
	break;
    case WIN_PARENT:
	if (winPtr->parentPtr != NULL) {
	    Tcl_SetObjResult(interp,
		    Tcl_NewStringObj(winPtr->parentPtr->pathName, -1));
	}
	break;
    case WIN_POINTERX:
	useX = 1;
	useY = 0;
	goto pointerxy;
    case WIN_POINTERY:
	useX = 0;
	useY = 1;
	goto pointerxy;
    case WIN_POINTERXY:
	useX = 1;
	useY = 1;

    pointerxy:
	winPtr = GetTopHierarchy(tkwin);
	if (winPtr == NULL) {
	    x = -1;
	    y = -1;
	} else {
	    TkGetPointerCoords((Tk_Window) winPtr, &x, &y);
	}
	if (useX & useY) {
	    Tcl_Obj *xyObj[2];

	    xyObj[0] = Tcl_NewIntObj(x);
	    xyObj[1] = Tcl_NewIntObj(y);
	    Tcl_SetObjResult(interp, Tcl_NewListObj(2, xyObj));
	} else if (useX) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(x));
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(y));
	}
	break;
    case WIN_REQHEIGHT:
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tk_ReqHeight(tkwin)));
	break;
    case WIN_REQWIDTH:
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tk_ReqWidth(tkwin)));
	break;
    case WIN_ROOTX:
	Tk_GetRootCoords(tkwin, &x, &y);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(x));
	break;
    case WIN_ROOTY:
	Tk_GetRootCoords(tkwin, &x, &y);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(y));
	break;
    case WIN_SCREEN:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s.%d",
		Tk_DisplayName(tkwin), Tk_ScreenNumber(tkwin)));
	break;
    case WIN_SCREENCELLS:
	Tcl_SetObjResult(interp,
		Tcl_NewIntObj(CellsOfScreen(Tk_Screen(tkwin))));
	break;
    case WIN_SCREENDEPTH:
	Tcl_SetObjResult(interp,
		Tcl_NewIntObj(DefaultDepthOfScreen(Tk_Screen(tkwin))));
	break;
    case WIN_SCREENHEIGHT:
	Tcl_SetObjResult(interp,
		Tcl_NewIntObj(HeightOfScreen(Tk_Screen(tkwin))));
	break;
    case WIN_SCREENWIDTH:
	Tcl_SetObjResult(interp,
		Tcl_NewIntObj(WidthOfScreen(Tk_Screen(tkwin))));
	break;
    case WIN_SCREENMMHEIGHT:
	Tcl_SetObjResult(interp,
		Tcl_NewIntObj(HeightMMOfScreen(Tk_Screen(tkwin))));
	break;
    case WIN_SCREENMMWIDTH:
	Tcl_SetObjResult(interp,
		Tcl_NewIntObj(WidthMMOfScreen(Tk_Screen(tkwin))));
	break;
    case WIN_SCREENVISUAL:
	c_class = DefaultVisualOfScreen(Tk_Screen(tkwin))->c_class;
	goto visual;
    case WIN_SERVER:
	TkGetServerInfo(interp, tkwin);
	break;
    case WIN_TOPLEVEL:
	winPtr = GetTopHierarchy(tkwin);
	if (winPtr != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(winPtr->pathName, -1));
	}
	break;
    case WIN_VIEWABLE: {
	int viewable = 0;

	for ( ; ; winPtr = winPtr->parentPtr) {
	    if ((winPtr == NULL) || !(winPtr->flags & TK_MAPPED)) {
		break;
	    }
	    if (winPtr->flags & TK_TOP_HIERARCHY) {
		viewable = 1;
		break;
	    }
	}

	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(viewable));
	break;
    }
    case WIN_VISUAL:
	c_class = Tk_Visual(tkwin)->c_class;

    visual:
	string = TkFindStateString(visualMap, c_class);
	if (string == NULL) {
	    string = "unknown";
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(string, -1));
	break;
    case WIN_VISUALID:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("0x%x", (unsigned)
		XVisualIDFromVisual(Tk_Visual(tkwin))));
	break;
    case WIN_VROOTHEIGHT:
	Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(height));
	break;
    case WIN_VROOTWIDTH:
	Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(width));
	break;
    case WIN_VROOTX:
	Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(x));
	break;
    case WIN_VROOTY:
	Tk_GetVRootGeometry(tkwin, &x, &y, &width, &height);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(y));
	break;
    case WIN_WIDTH:
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tk_Width(tkwin)));
	break;
    case WIN_X:
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tk_X(tkwin)));
	break;
    case WIN_Y:
	Tcl_SetObjResult(interp, Tcl_NewIntObj(Tk_Y(tkwin)));
	break;

	/*
	 * Uses -displayof.
	 */

    case WIN_ATOM:
	skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	if (skip < 0) {
	    return TCL_ERROR;
	}
	if (objc - skip != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window? name");
	    return TCL_ERROR;
	}
	objv += skip;
	string = Tcl_GetString(objv[2]);
	Tcl_SetObjResult(interp,
		Tcl_NewLongObj((long) Tk_InternAtom(tkwin, string)));
	break;
    case WIN_ATOMNAME: {
	const char *name;
	long id;

	skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	if (skip < 0) {
	    return TCL_ERROR;
	}
	if (objc - skip != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window? id");
	    return TCL_ERROR;
	}
	objv += skip;
	if (Tcl_GetLongFromObj(interp, objv[2], &id) != TCL_OK) {
	    return TCL_ERROR;
	}
	name = Tk_GetAtomName(tkwin, (Atom) id);
	if (strcmp(name, "?bad atom?") == 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "no atom exists with id \"%s\"", Tcl_GetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "ATOM",
		    Tcl_GetString(objv[2]), NULL);
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));
	break;
    }
    case WIN_CONTAINING:
	skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	if (skip < 0) {
	    return TCL_ERROR;
	}
	if (objc - skip != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "?-displayof window? rootX rootY");
	    return TCL_ERROR;
	}
	objv += skip;
	string = Tcl_GetString(objv[2]);
	if (Tk_GetPixels(interp, tkwin, string, &x) != TCL_OK) {
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[3]);
	if (Tk_GetPixels(interp, tkwin, string, &y) != TCL_OK) {
	    return TCL_ERROR;
	}
	tkwin = Tk_CoordsToWindow(x, y, tkwin);
	if (tkwin != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(Tk_PathName(tkwin),-1));
	}
	break;
    case WIN_INTERPS:
	skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	if (skip < 0) {
	    return TCL_ERROR;
	}
	if (objc - skip != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window?");
	    return TCL_ERROR;
	}
	return TkGetInterpNames(interp, tkwin);
    case WIN_PATHNAME: {
	Window id;

	skip = TkGetDisplayOf(interp, objc - 2, objv + 2, &tkwin);
	if (skip < 0) {
	    return TCL_ERROR;
	}
	if (objc - skip != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window? id");
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[2 + skip]);
	if (TkpScanWindowId(interp, string, &id) != TCL_OK) {
	    return TCL_ERROR;
	}
	winPtr = (TkWindow *) Tk_IdToWindow(Tk_Display(tkwin), id);
	if ((winPtr == NULL) ||
		(winPtr->mainPtr != ((TkWindow *) tkwin)->mainPtr)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window id \"%s\" doesn't exist in this application",
		    string));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "WINDOW", string, NULL);
	    return TCL_ERROR;
	}

	/*
	 * If the window is a utility window with no associated path (such as
	 * a wrapper window or send communication window), just return an
	 * empty string.
	 */

	tkwin = (Tk_Window) winPtr;
	if (Tk_PathName(tkwin) != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(Tk_PathName(tkwin),-1));
	}
	break;
    }

	/*
	 * objv[3] is window.
	 */

    case WIN_EXISTS: {
	int alive;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[2]);
	winPtr = (TkWindow *) Tk_NameToWindow(interp, string, tkwin);
	Tcl_ResetResult(interp);

	alive = 1;
	if ((winPtr == NULL) || (winPtr->flags & TK_ALREADY_DEAD)) {
	    alive = 0;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(alive));
	break;
    }
    case WIN_FPIXELS: {
	double mm, pixels;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window number");
	    return TCL_ERROR;
	}
	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin) != TCL_OK) {
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[3]);
	if (Tk_GetScreenMM(interp, tkwin, string, &mm) != TCL_OK) {
	    return TCL_ERROR;
	}
	pixels = mm * WidthOfScreen(Tk_Screen(tkwin))
		/ WidthMMOfScreen(Tk_Screen(tkwin));
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(pixels));
	break;
    }
    case WIN_PIXELS: {
	int pixels;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window number");
	    return TCL_ERROR;
	}
	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin) != TCL_OK) {
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[3]);
	if (Tk_GetPixels(interp, tkwin, string, &pixels) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(pixels));
	break;
    }
    case WIN_RGB: {
	XColor *colorPtr;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window colorName");
	    return TCL_ERROR;
	}
	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin) != TCL_OK) {
	    return TCL_ERROR;
	}
	colorPtr = Tk_GetColor(interp, tkwin, Tcl_GetString(objv[3]));
	if (colorPtr == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("%d %d %d",
		colorPtr->red, colorPtr->green, colorPtr->blue));
	Tk_FreeColor(colorPtr);
	break;
    }
    case WIN_VISUALSAVAILABLE: {
	XVisualInfo template, *visInfoPtr;
	int count, i;
	int includeVisualId;
	Tcl_Obj *strPtr, *resultPtr;
	char buf[16 + TCL_INTEGER_SPACE];
	char visualIdString[TCL_INTEGER_SPACE];

	if (objc == 3) {
	    includeVisualId = 0;
	} else if ((objc == 4)
		&& (strcmp(Tcl_GetString(objv[3]), "includeids") == 0)) {
	    includeVisualId = 1;
	} else {
	    Tcl_WrongNumArgs(interp, 2, objv, "window ?includeids?");
	    return TCL_ERROR;
	}

	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin) != TCL_OK) {
	    return TCL_ERROR;
	}

	template.screen = Tk_ScreenNumber(tkwin);
	visInfoPtr = XGetVisualInfo(Tk_Display(tkwin), VisualScreenMask,
		&template, &count);
	if (visInfoPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't find any visuals for screen", -1));
	    Tcl_SetErrorCode(interp, "TK", "VISUAL", "NONE", NULL);
	    return TCL_ERROR;
	}
	resultPtr = Tcl_NewObj();
	for (i = 0; i < count; i++) {
	    string = TkFindStateString(visualMap, visInfoPtr[i].c_class);
	    if (string == NULL) {
		strcpy(buf, "unknown");
	    } else {
		sprintf(buf, "%s %d", string, visInfoPtr[i].depth);
	    }
	    if (includeVisualId) {
		sprintf(visualIdString, " 0x%lx",
			(unsigned long) visInfoPtr[i].visualid);
		strcat(buf, visualIdString);
	    }
	    strPtr = Tcl_NewStringObj(buf, -1);
	    Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
	}
	Tcl_SetObjResult(interp, resultPtr);
	XFree((char *) visInfoPtr);
	break;
    }
    }
    return TCL_OK;
}

#if 0
/*
 *----------------------------------------------------------------------
 *
 * Tk_WmObjCmd --
 *
 *	This function is invoked to process the "wm" Tcl command. See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
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
    Tk_Window tkwin;
    TkWindow *winPtr;

    static const char *const optionStrings[] = {
	"aspect",	"client",	"command",	"deiconify",
	"focusmodel",	"frame",	"geometry",	"grid",
	"group",	"iconbitmap",	"iconify",	"iconmask",
	"iconname",	"iconposition",	"iconwindow",	"maxsize",
	"minsize",	"overrideredirect",	"positionfrom",	"protocol",
	"resizable",	"sizefrom",	"state",	"title",
	"tracing",	"transient",	"withdraw",	NULL
    };
    enum options {
	TKWM_ASPECT,	TKWM_CLIENT,	TKWM_COMMAND,	TKWM_DEICONIFY,
	TKWM_FOCUSMOD,	TKWM_FRAME,	TKWM_GEOMETRY,	TKWM_GRID,
	TKWM_GROUP,	TKWM_ICONBMP,	TKWM_ICONIFY,	TKWM_ICONMASK,
	TKWM_ICONNAME,	TKWM_ICONPOS,	TKWM_ICONWIN,	TKWM_MAXSIZE,
	TKWM_MINSIZE,	TKWM_OVERRIDE,	TKWM_POSFROM,	TKWM_PROTOCOL,
	TKWM_RESIZABLE,	TKWM_SIZEFROM,	TKWM_STATE,	TKWM_TITLE,
	TKWM_TRACING,	TKWM_TRANSIENT,	TKWM_WITHDRAW
    };

    tkwin = (Tk_Window) clientData;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option window ?arg?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (index == TKWM_TRACING) {
	int wmTracing;
	TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

	if ((objc != 2) && (objc != 3)) {
	    Tcl_WrongNumArgs(interp, 1, objv, "tracing ?boolean?");
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

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window ?arg?");
	return TCL_ERROR;
    }

    winPtr = (TkWindow *) Tk_NameToWindow(interp,
	    Tcl_GetString(objv[2]), tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"window \"%s\" isn't a top-level window", winPtr->pathName));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "TOPLEVEL", winPtr->pathName,
		NULL);
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case TKWM_ASPECT:
	TkpWmAspectCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_CLIENT:
	TkpWmClientCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_COMMAND:
	TkpWmCommandCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_DEICONIFY:
	TkpWmDeiconifyCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_FOCUSMOD:
	TkpWmFocusmodCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_FRAME:
	TkpWmFrameCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_GEOMETRY:
	TkpWmGeometryCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_GRID:
	TkpWmGridCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_GROUP:
	TkpWmGroupCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_ICONBMP:
	TkpWmIconbitmapCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_ICONIFY:
	TkpWmIconifyCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_ICONMASK:
	TkpWmIconmaskCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_ICONNAME:
	/*
	 * Slight Unix variation.
	 */
	TkpWmIconnameCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_ICONPOS:
	/*
	 * nearly same - 1 line more on Unix.
	 */
	TkpWmIconpositionCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_ICONWIN:
	TkpWmIconwindowCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_MAXSIZE:
	/*
	 * Nearly same, win diffs.
	 */
	TkpWmMaxsizeCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_MINSIZE:
	/*
	 * Nearly same, win diffs
	 */
	TkpWmMinsizeCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_OVERRIDE:
	/*
	 * Almost same.
	 */
	TkpWmOverrideCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_POSFROM:
	/*
	 * Equal across platforms
	 */
	TkpWmPositionfromCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_PROTOCOL:
	/*
	 * Equal across platforms
	 */
	TkpWmProtocolCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_RESIZABLE:
	/*
	 * Almost same
	 */
	TkpWmResizableCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_SIZEFROM:
	/*
	 * Equal across platforms
	 */
	TkpWmSizefromCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_STATE:
	TkpWmStateCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_TITLE:
	TkpWmTitleCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_TRANSIENT:
	TkpWmTransientCmd(interp, tkwin, winPtr, objc, objv);
	break;
    case TKWM_WITHDRAW:
	TkpWmWithdrawCmd(interp, tkwin, winPtr, objc, objv);
	break;
    }

  updateGeom:
    if (!(wmPtr->flags & (WM_UPDATE_PENDING|WM_NEVER_MAPPED))) {
	Tcl_DoWhenIdle(UpdateGeometryInfo, winPtr);
	wmPtr->flags |= WM_UPDATE_PENDING;
    }
    return TCL_OK;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * TkGetDisplayOf --
 *
 *	Parses a "-displayof window" option for various commands. If present,
 *	the literal "-displayof" should be in objv[0] and the window name in
 *	objv[1].
 *
 * Results:
 *	The return value is 0 if the argument strings did not contain the
 *	"-displayof" option. The return value is 2 if the argument strings
 *	contained both the "-displayof" option and a valid window name.
 *	Otherwise, the return value is -1 if the window name was missing or
 *	did not specify a valid window.
 *
 *	If the return value was 2, *tkwinPtr is filled with the token for the
 *	window specified on the command line. If the return value was -1, an
 *	error message is left in interp's result object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkGetDisplayOf(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects. If it is present,
				 * "-displayof" should be in objv[0] and
				 * objv[1] the name of a window. */
    Tk_Window *tkwinPtr)	/* On input, contains main window of
				 * application associated with interp. On
				 * output, filled with window specified as
				 * option to "-displayof" argument, or
				 * unmodified if "-displayof" argument was not
				 * present. */
{
    const char *string;
    int length;

    if (objc < 1) {
	return 0;
    }
    string = Tcl_GetStringFromObj(objv[0], &length);
    if ((length >= 2) &&
	    (strncmp(string, "-displayof", (unsigned) length) == 0)) {
        if (objc < 2) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "value for \"-displayof\" missing", -1));
	    Tcl_SetErrorCode(interp, "TK", "NO_VALUE", "DISPLAYOF", NULL);
	    return -1;
	}
	*tkwinPtr = Tk_NameToWindow(interp, Tcl_GetString(objv[1]), *tkwinPtr);
	if (*tkwinPtr == NULL) {
	    return -1;
	}
	return 2;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDeadAppObjCmd --
 *
 *	If an application has been deleted then all Tk commands will be
 *	re-bound to this function.
 *
 * Results:
 *	A standard Tcl error is reported to let the user know that the
 *	application is dead.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkDeadAppObjCmd(
    ClientData clientData,	/* Dummy. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument strings. */
{
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "can't invoke \"%s\" command: application has been destroyed",
	    Tcl_GetString(objv[0])));
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTopHierarchy --
 *
 *	Retrieves the top-of-hierarchy window which is the nearest ancestor of
 *	the specified window.
 *
 * Results:
 *	Returns the top-of-hierarchy window, or NULL if the window has no
 *	ancestor which is at the top of a physical window hierarchy.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TkWindow *
GetTopHierarchy(
    Tk_Window tkwin)		/* Window for which the top-of-hierarchy
				 * ancestor should be deterined. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;

    while ((winPtr != NULL) && !(winPtr->flags & TK_TOP_HIERARCHY)) {
	winPtr = winPtr->parentPtr;
    }
    return winPtr;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
