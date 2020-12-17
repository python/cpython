/*
 * tkSelect.c --
 *
 *	This file manages the selection for the Tk toolkit, translating
 *	between the standard X ICCCM conventions and Tcl commands.
 *
 * Copyright (c) 1990-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkSelect.h"

/*
 * When a selection handler is set up by invoking "selection handle", one of
 * the following data structures is set up to hold information about the
 * command to invoke and its interpreter.
 */

typedef struct {
    Tcl_Interp *interp;		/* Interpreter in which to invoke command. */
    int cmdLength;		/* # of non-NULL bytes in command. */
    int charOffset;		/* The offset of the next char to retrieve. */
    int byteOffset;		/* The expected byte offset of the next
				 * chunk. */
    char buffer[4];	/* A buffer to hold part of a UTF character
				 * that is split across chunks. */
    char command[1];		/* Command to invoke. Actual space is
				 * allocated as large as necessary. This must
				 * be the last entry in the structure. */
} CommandInfo;

/*
 * When selection ownership is claimed with the "selection own" Tcl command,
 * one of the following structures is created to record the Tcl command to be
 * executed when the selection is lost again.
 */

typedef struct LostCommand {
    Tcl_Interp *interp;		/* Interpreter in which to invoke command. */
    Tcl_Obj *cmdObj;		/* Reference to command to invoke. */
} LostCommand;

/*
 * The structure below is used to keep each thread's pending list separate.
 */

typedef struct {
    TkSelInProgress *pendingPtr;
				/* Topmost search in progress, or NULL if
				 * none. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations for functions defined in this file:
 */

static int		HandleTclCommand(ClientData clientData,
			    int offset, char *buffer, int maxBytes);
static void		LostSelection(ClientData clientData);
static int		SelGetProc(ClientData clientData,
			    Tcl_Interp *interp, const char *portion);

/*
 *--------------------------------------------------------------
 *
 * Tk_CreateSelHandler --
 *
 *	This function is called to register a function as the handler for
 *	selection requests of a particular target type on a particular window
 *	for a particular selection.
 *
 * Results:
 *	None.
 *
 * Side effects:

 *	In the future, whenever the selection is in tkwin's window and someone
 *	requests the selection in the form given by target, proc will be
 *	invoked to provide part or all of the selection in the given form. If
 *	there was already a handler declared for the given window, target and
 *	selection type, then it is replaced. Proc should have the following
 *	form:
 *
 *	int
 *	proc(
 *	    ClientData clientData,
 *	    int offset,
 *	    char *buffer,
 *	    int maxBytes)
 *	{
 *	}
 *
 *	The clientData argument to proc will be the same as the clientData
 *	argument to this function. The offset argument indicates which portion
 *	of the selection to return: skip the first offset bytes. Buffer is a
 *	pointer to an area in which to place the converted selection, and
 *	maxBytes gives the number of bytes available at buffer. Proc should
 *	place the selection in buffer as a string, and return a count of the
 *	number of bytes of selection actually placed in buffer (not including
 *	the terminating NULL character). If the return value equals maxBytes,
 *	this is a sign that there is probably still more selection information
 *	available.
 *
 *--------------------------------------------------------------
 */

void
Tk_CreateSelHandler(
    Tk_Window tkwin,		/* Token for window. */
    Atom selection,		/* Selection to be handled. */
    Atom target,		/* The kind of selection conversions that can
				 * be handled by proc, e.g. TARGETS or
				 * STRING. */
    Tk_SelectionProc *proc,	/* Function to invoke to convert selection to
				 * type "target". */
    ClientData clientData,	/* Value to pass to proc. */
    Atom format)		/* Format in which the selection information
				 * should be returned to the requestor.
				 * XA_STRING is best by far, but anything
				 * listed in the ICCCM will be tolerated
				 * (blech). */
{
    register TkSelHandler *selPtr;
    TkWindow *winPtr = (TkWindow *) tkwin;

    if (winPtr->dispPtr->multipleAtom == None) {
	TkSelInit(tkwin);
    }

    /*
     * See if there's already a handler for this target and selection on this
     * window. If so, re-use it. If not, create a new one.
     */

    for (selPtr = winPtr->selHandlerList; ; selPtr = selPtr->nextPtr) {
	if (selPtr == NULL) {
	    selPtr = ckalloc(sizeof(TkSelHandler));
	    selPtr->nextPtr = winPtr->selHandlerList;
	    winPtr->selHandlerList = selPtr;
	    break;
	}
	if ((selPtr->selection == selection) && (selPtr->target == target)) {
	    /*
	     * Special case: when replacing handler created by "selection
	     * handle", free up memory. Should there be a callback to allow
	     * other clients to do this too?
	     */

	    if (selPtr->proc == HandleTclCommand) {
		ckfree(selPtr->clientData);
	    }
	    break;
	}
    }
    selPtr->selection = selection;
    selPtr->target = target;
    selPtr->format = format;
    selPtr->proc = proc;
    selPtr->clientData = clientData;
    if (format == XA_STRING) {
	selPtr->size = 8;
    } else {
	selPtr->size = 32;
    }

    if ((target == XA_STRING) && (winPtr->dispPtr->utf8Atom != (Atom) 0)) {
	/*
	 * If the user asked for a STRING handler and we understand
	 * UTF8_STRING, we implicitly create a UTF8_STRING handler for them.
	 */

	target = winPtr->dispPtr->utf8Atom;
	for (selPtr = winPtr->selHandlerList; ; selPtr = selPtr->nextPtr) {
	    if (selPtr == NULL) {
		selPtr = ckalloc(sizeof(TkSelHandler));
		selPtr->nextPtr = winPtr->selHandlerList;
		winPtr->selHandlerList = selPtr;
		selPtr->selection = selection;
		selPtr->target = target;
		selPtr->format = target; /* We want UTF8_STRING format */
		selPtr->proc = proc;
		if (selPtr->proc == HandleTclCommand) {
		    /*
		     * The clientData is selection controlled memory, so we
		     * should make a copy for this selPtr.
		     */

		    unsigned cmdInfoLen = Tk_Offset(CommandInfo, command) + 1 +
			    ((CommandInfo *)clientData)->cmdLength;

		    selPtr->clientData = ckalloc(cmdInfoLen);
		    memcpy(selPtr->clientData, clientData, cmdInfoLen);
		} else {
		    selPtr->clientData = clientData;
		}
		selPtr->size = 8;
		break;
	    }
	    if (selPtr->selection==selection && selPtr->target==target) {
		/*
		 * Looks like we had a utf-8 target already. Leave it alone.
		 */

		break;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DeleteSelHandler --
 *
 *	Remove the selection handler for a given window, target, and
 *	selection, if it exists.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection handler for tkwin and target is removed. If there is no
 *	such handler then nothing happens.
 *
 *----------------------------------------------------------------------
 */

void
Tk_DeleteSelHandler(
    Tk_Window tkwin,		/* Token for window. */
    Atom selection,		/* The selection whose handler is to be
				 * removed. */
    Atom target)		/* The target whose selection handler is to be
				 * removed. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    register TkSelHandler *selPtr, *prevPtr;
    register TkSelInProgress *ipPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Find the selection handler to be deleted, or return if it doesn't
     * exist.
     */

    for (selPtr = winPtr->selHandlerList, prevPtr = NULL; ;
	    prevPtr = selPtr, selPtr = selPtr->nextPtr) {
	if (selPtr == NULL) {
	    return;
	}
	if ((selPtr->selection == selection) && (selPtr->target == target)) {
	    break;
	}
    }

    /*
     * If ConvertSelection is processing this handler, tell it that the
     * handler is dead.
     */

    for (ipPtr = tsdPtr->pendingPtr; ipPtr != NULL;
	    ipPtr = ipPtr->nextPtr) {
	if (ipPtr->selPtr == selPtr) {
	    ipPtr->selPtr = NULL;
	}
    }

    /*
     * Free resources associated with the handler.
     */

    if (prevPtr == NULL) {
	winPtr->selHandlerList = selPtr->nextPtr;
    } else {
	prevPtr->nextPtr = selPtr->nextPtr;
    }

    if ((target == XA_STRING) && (winPtr->dispPtr->utf8Atom != (Atom) 0)) {
	/*
	 * If the user asked for a STRING handler and we understand
	 * UTF8_STRING, we may have implicitly created a UTF8_STRING handler
	 * for them. Look for it and delete it as necessary.
	 */

	TkSelHandler *utf8selPtr;

	target = winPtr->dispPtr->utf8Atom;
	for (utf8selPtr = winPtr->selHandlerList; utf8selPtr != NULL;
		utf8selPtr = utf8selPtr->nextPtr) {
	    if ((utf8selPtr->selection == selection)
		    && (utf8selPtr->target == target)) {
		break;
	    }
	}
	if (utf8selPtr != NULL) {
	    if ((utf8selPtr->format == target)
		    && (utf8selPtr->proc == selPtr->proc)
		    && (utf8selPtr->size == selPtr->size)) {
		/*
		 * This recursive call is OK, because we've changed the value
		 * of 'target'.
		 */

		Tk_DeleteSelHandler(tkwin, selection, target);
	    }
	}
    }

    if (selPtr->proc == HandleTclCommand) {
	/*
	 * Mark the CommandInfo as deleted and free it if we can.
	 */

	((CommandInfo *) selPtr->clientData)->interp = NULL;
	Tcl_EventuallyFree(selPtr->clientData, TCL_DYNAMIC);
    }
    ckfree(selPtr);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_OwnSelection --
 *
 *	Arrange for tkwin to become the owner of a selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, requests for the selection will be directed to functions
 *	associated with tkwin (they must have been declared with calls to
 *	Tk_CreateSelHandler). When the selection is lost by this window, proc
 *	will be invoked (see the manual entry for details). This function may
 *	invoke callbacks, including Tcl scripts, so any calling function
 *	should be reentrant at the point where Tk_OwnSelection is invoked.
 *
 *--------------------------------------------------------------
 */

void
Tk_OwnSelection(
    Tk_Window tkwin,		/* Window to become new selection owner. */
    Atom selection,		/* Selection that window should own. */
    Tk_LostSelProc *proc,	/* Function to call when selection is taken
				 * away from tkwin. */
    ClientData clientData)	/* Arbitrary one-word argument to pass to
				 * proc. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkSelectionInfo *infoPtr;
    Tk_LostSelProc *clearProc = NULL;
    ClientData clearData = NULL;/* Initialization needed only to prevent
				 * compiler warning. */

    if (dispPtr->multipleAtom == None) {
	TkSelInit(tkwin);
    }
    Tk_MakeWindowExist(tkwin);

    /*
     * This code is somewhat tricky. First, we find the specified selection on
     * the selection list. If the previous owner is in this process, and is a
     * different window, then we need to invoke the clearProc. However, it's
     * dangerous to call the clearProc right now, because it could invoke a
     * Tcl script that wrecks the current state (e.g. it could delete the
     * window). To be safe, defer the call until the end of the function when
     * we no longer care about the state.
     */

    for (infoPtr = dispPtr->selectionInfoPtr; infoPtr != NULL;
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->selection == selection) {
	    break;
	}
    }
    if (infoPtr == NULL) {
	infoPtr = ckalloc(sizeof(TkSelectionInfo));
	infoPtr->selection = selection;
	infoPtr->nextPtr = dispPtr->selectionInfoPtr;
	dispPtr->selectionInfoPtr = infoPtr;
    } else if (infoPtr->clearProc != NULL) {
	if (infoPtr->owner != tkwin) {
	    clearProc = infoPtr->clearProc;
	    clearData = infoPtr->clearData;
	} else if (infoPtr->clearProc == LostSelection) {
	    /*
	     * If the selection handler is one created by "selection own", be
	     * sure to free the record for it; otherwise there will be a
	     * memory leak.
	     */

	    ckfree(infoPtr->clearData);
	}
    }

    infoPtr->owner = tkwin;
    infoPtr->serial = NextRequest(winPtr->display);
    infoPtr->clearProc = proc;
    infoPtr->clearData = clientData;

    /*
     * Note that we are using CurrentTime, even though ICCCM recommends
     * against this practice (the problem is that we don't necessarily have a
     * valid time to use). We will not be able to retrieve a useful timestamp
     * for the TIMESTAMP target later.
     */

    infoPtr->time = CurrentTime;

    /*
     * Note that we are not checking to see if the selection claim succeeded.
     * If the ownership does not change, then the clearProc may never be
     * invoked, and we will return incorrect information when queried for the
     * current selection owner.
     */

    XSetSelectionOwner(winPtr->display, infoPtr->selection, winPtr->window,
	    infoPtr->time);

    /*
     * Now that we are done, we can invoke clearProc without running into
     * reentrancy problems.
     */

    if (clearProc != NULL) {
	clearProc(clearData);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ClearSelection --
 *
 *	Eliminate the specified selection on tkwin's display, if there is one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified selection is cleared, so that future requests to
 *	retrieve it will fail until some application owns it again. This
 *	function invokes callbacks, possibly including Tcl scripts, so any
 *	calling function should be reentrant at the point Tk_ClearSelection is
 *	invoked.
 *
 *----------------------------------------------------------------------
 */

void
Tk_ClearSelection(
    Tk_Window tkwin,		/* Window that selects a display. */
    Atom selection)		/* Selection to be cancelled. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkSelectionInfo *infoPtr;
    TkSelectionInfo *prevPtr;
    TkSelectionInfo *nextPtr;
    Tk_LostSelProc *clearProc = NULL;
    ClientData clearData = NULL;/* Initialization needed only to prevent
				 * compiler warning. */

    if (dispPtr->multipleAtom == None) {
	TkSelInit(tkwin);
    }

    for (infoPtr = dispPtr->selectionInfoPtr, prevPtr = NULL;
	    infoPtr != NULL; infoPtr = nextPtr) {
	nextPtr = infoPtr->nextPtr;
	if (infoPtr->selection == selection) {
	    if (prevPtr == NULL) {
		dispPtr->selectionInfoPtr = nextPtr;
	    } else {
		prevPtr->nextPtr = nextPtr;
	    }
	    break;
	}
	prevPtr = infoPtr;
    }

    if (infoPtr != NULL) {
	clearProc = infoPtr->clearProc;
	clearData = infoPtr->clearData;
	ckfree(infoPtr);
    }
    XSetSelectionOwner(winPtr->display, selection, None, CurrentTime);

    if (clearProc != NULL) {
	clearProc(clearData);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_GetSelection --
 *
 *	Retrieve the value of a selection and pass it off (in pieces,
 *	possibly) to a given function.
 *
 * Results:
 *	The return value is a standard Tcl return value. If an error occurs
 *	(such as no selection exists) then an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	The standard X11 protocols are used to retrieve the selection. When it
 *	arrives, it is passed to proc. If the selection is very large, it will
 *	be passed to proc in several pieces. Proc should have the following
 *	structure:
 *
 *	int
 *	proc(
 *	    ClientData clientData,
 *	    Tcl_Interp *interp,
 *	    char *portion)
 *	{
 *	}
 *
 *	The interp and clientData arguments to proc will be the same as the
 *	corresponding arguments to Tk_GetSelection. The portion argument
 *	points to a character string containing part of the selection, and
 *	numBytes indicates the length of the portion, not including the
 *	terminating NULL character. If the selection arrives in several
 *	pieces, the "portion" arguments in separate calls will contain
 *	successive parts of the selection. Proc should normally return TCL_OK.
 *	If it detects an error then it should return TCL_ERROR and leave an
 *	error message in the interp's result; the remainder of the selection
 *	retrieval will be aborted.
 *
 *--------------------------------------------------------------
 */

int
Tk_GetSelection(
    Tcl_Interp *interp,		/* Interpreter to use for reporting errors. */
    Tk_Window tkwin,		/* Window on whose behalf to retrieve the
				 * selection (determines display from which to
				 * retrieve). */
    Atom selection,		/* Selection to retrieve. */
    Atom target,		/* Desired form in which selection is to be
				 * returned. */
    Tk_GetSelProc *proc,	/* Function to call to process the selection,
				 * once it has been retrieved. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkSelectionInfo *infoPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (dispPtr->multipleAtom == None) {
	TkSelInit(tkwin);
    }

    /*
     * If the selection is owned by a window managed by this process, then
     * call the retrieval function directly, rather than going through the X
     * server (it's dangerous to go through the X server in this case because
     * it could result in deadlock if an INCR-style selection results).
     */

    for (infoPtr = dispPtr->selectionInfoPtr; infoPtr != NULL;
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->selection == selection) {
	    break;
	}
    }
    if (infoPtr != NULL) {
	register TkSelHandler *selPtr;
	int offset, result, count;
	char buffer[TK_SEL_BYTES_AT_ONCE+1];
	TkSelInProgress ip;

	for (selPtr = ((TkWindow *) infoPtr->owner)->selHandlerList;
		selPtr != NULL; selPtr = selPtr->nextPtr) {
	    if (selPtr->target==target && selPtr->selection==selection) {
		break;
	    }
	}
	if (selPtr == NULL) {
	    Atom type;

	    count = TkSelDefaultSelection(infoPtr, target, buffer,
		    TK_SEL_BYTES_AT_ONCE, &type);
	    if (count > TK_SEL_BYTES_AT_ONCE) {
		Tcl_Panic("selection handler returned too many bytes");
	    }
	    if (count < 0) {
		goto cantget;
	    }
	    buffer[count] = 0;
	    result = proc(clientData, interp, buffer);
	} else {
	    offset = 0;
	    result = TCL_OK;
	    ip.selPtr = selPtr;
	    ip.nextPtr = tsdPtr->pendingPtr;
	    tsdPtr->pendingPtr = &ip;
	    while (1) {
		count = selPtr->proc(selPtr->clientData, offset, buffer,
			TK_SEL_BYTES_AT_ONCE);
		if ((count < 0) || (ip.selPtr == NULL)) {
		    tsdPtr->pendingPtr = ip.nextPtr;
		    goto cantget;
		}
		if (count > TK_SEL_BYTES_AT_ONCE) {
		    Tcl_Panic("selection handler returned too many bytes");
		}
		buffer[count] = '\0';
		result = proc(clientData, interp, buffer);
		if ((result != TCL_OK) || (count < TK_SEL_BYTES_AT_ONCE)
			|| (ip.selPtr == NULL)) {
		    break;
		}
		offset += count;
	    }
	    tsdPtr->pendingPtr = ip.nextPtr;
	}
	return result;
    }

    /*
     * The selection is owned by some other process.
     */

    return TkSelGetSelection(interp, tkwin, selection, target, proc,
	    clientData);

  cantget:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "%s selection doesn't exist or form \"%s\" not defined",
	    Tk_GetAtomName(tkwin, selection),
	    Tk_GetAtomName(tkwin, target)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_SelectionObjCmd --
 *
 *	This function is invoked to process the "selection" Tcl command. See
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
Tk_SelectionObjCmd(
    ClientData clientData,	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    const char *path = NULL;
    Atom selection;
    const char *selName = NULL;
    const char *string;
    int count, index;
    Tcl_Obj **objs;
    static const char *const optionStrings[] = {
	"clear", "get", "handle", "own", NULL
    };
    enum options {
	SELECTION_CLEAR, SELECTION_GET, SELECTION_HANDLE, SELECTION_OWN
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case SELECTION_CLEAR: {
	static const char *const clearOptionStrings[] = {
	    "-displayof", "-selection", NULL
	};
	enum clearOptions { CLEAR_DISPLAYOF, CLEAR_SELECTION };
	int clearIndex;

	for (count = objc-2, objs = ((Tcl_Obj **)objv)+2; count > 0;
		count-=2, objs+=2) {
	    string = Tcl_GetString(objs[0]);
	    if (string[0] != '-') {
		break;
	    }
	    if (count < 2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"value for \"%s\" missing", string));
		Tcl_SetErrorCode(interp, "TK", "SELECTION", "VALUE", NULL);
		return TCL_ERROR;
	    }

	    if (Tcl_GetIndexFromObj(interp, objs[0], clearOptionStrings,
		    "option", 0, &clearIndex) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum clearOptions) clearIndex) {
	    case CLEAR_DISPLAYOF:
		path = Tcl_GetString(objs[1]);
		break;
	    case CLEAR_SELECTION:
		selName = Tcl_GetString(objs[1]);
		break;
	    }
	}

	if (count == 1) {
	    path = Tcl_GetString(objs[0]);
	} else if (count > 1) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-option value ...?");
	    return TCL_ERROR;
	}
	if (path != NULL) {
	    tkwin = Tk_NameToWindow(interp, path, tkwin);
	}
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (selName != NULL) {
	    selection = Tk_InternAtom(tkwin, selName);
	} else {
	    selection = XA_PRIMARY;
	}

	Tk_ClearSelection(tkwin, selection);
	break;
    }

    case SELECTION_GET: {
	Atom target;
	const char *targetName = NULL;
	Tcl_DString selBytes;
	int result;
	static const char *const getOptionStrings[] = {
	    "-displayof", "-selection", "-type", NULL
	};
	enum getOptions { GET_DISPLAYOF, GET_SELECTION, GET_TYPE };
	int getIndex;

	for (count = objc-2, objs = ((Tcl_Obj **)objv)+2; count>0;
		count-=2, objs+=2) {
	    string = Tcl_GetString(objs[0]);
	    if (string[0] != '-') {
		break;
	    }
	    if (count < 2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"value for \"%s\" missing", string));
		Tcl_SetErrorCode(interp, "TK", "SELECTION", "VALUE", NULL);
		return TCL_ERROR;
	    }

	    if (Tcl_GetIndexFromObj(interp, objs[0], getOptionStrings,
		    "option", 0, &getIndex) != TCL_OK) {
		return TCL_ERROR;
	    }

	    switch ((enum getOptions) getIndex) {
	    case GET_DISPLAYOF:
		path = Tcl_GetString(objs[1]);
		break;
	    case GET_SELECTION:
		selName = Tcl_GetString(objs[1]);
		break;
	    case GET_TYPE:
		targetName = Tcl_GetString(objs[1]);
		break;
	    }
	}

	if (path != NULL) {
	    tkwin = Tk_NameToWindow(interp, path, tkwin);
	}
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (selName != NULL) {
	    selection = Tk_InternAtom(tkwin, selName);
	} else {
	    selection = XA_PRIMARY;
	}
	if (count > 1) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-option value ...?");
	    return TCL_ERROR;
	} else if (count == 1) {
	    target = Tk_InternAtom(tkwin, Tcl_GetString(objs[0]));
	} else if (targetName != NULL) {
	    target = Tk_InternAtom(tkwin, targetName);
	} else {
	    target = XA_STRING;
	}

	Tcl_DStringInit(&selBytes);
	result = Tk_GetSelection(interp, tkwin, selection, target,
		SelGetProc, &selBytes);
	if (result == TCL_OK) {
	    Tcl_DStringResult(interp, &selBytes);
	} else {
	    Tcl_DStringFree(&selBytes);
	}
	return result;
    }

    case SELECTION_HANDLE: {
	Atom target, format;
	const char *targetName = NULL;
	const char *formatName = NULL;
	register CommandInfo *cmdInfoPtr;
	int cmdLength;
	static const char *const handleOptionStrings[] = {
	    "-format", "-selection", "-type", NULL
	};
	enum handleOptions {
	    HANDLE_FORMAT, HANDLE_SELECTION, HANDLE_TYPE
	};
	int handleIndex;

	for (count = objc-2, objs = ((Tcl_Obj **)objv)+2; count > 0;
		count-=2, objs+=2) {
	    string = Tcl_GetString(objs[0]);
	    if (string[0] != '-') {
		break;
	    }
	    if (count < 2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"value for \"%s\" missing", string));
		Tcl_SetErrorCode(interp, "TK", "SELECTION", "VALUE", NULL);
		return TCL_ERROR;
	    }

	    if (Tcl_GetIndexFromObj(interp, objs[0],handleOptionStrings,
		    "option", 0, &handleIndex) != TCL_OK) {
		return TCL_ERROR;
	    }

	    switch ((enum handleOptions) handleIndex) {
	    case HANDLE_FORMAT:
		formatName = Tcl_GetString(objs[1]);
		break;
	    case HANDLE_SELECTION:
		selName = Tcl_GetString(objs[1]);
		break;
	    case HANDLE_TYPE:
		targetName = Tcl_GetString(objs[1]);
		break;
	    }
	}

	if ((count < 2) || (count > 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "?-option value ...? window command");
	    return TCL_ERROR;
	}
	tkwin = Tk_NameToWindow(interp, Tcl_GetString(objs[0]), tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (selName != NULL) {
	    selection = Tk_InternAtom(tkwin, selName);
	} else {
	    selection = XA_PRIMARY;
	}

	if (count > 2) {
	    target = Tk_InternAtom(tkwin, Tcl_GetString(objs[2]));
	} else if (targetName != NULL) {
	    target = Tk_InternAtom(tkwin, targetName);
	} else {
	    target = XA_STRING;
	}
	if (count > 3) {
	    format = Tk_InternAtom(tkwin, Tcl_GetString(objs[3]));
	} else if (formatName != NULL) {
	    format = Tk_InternAtom(tkwin, formatName);
	} else {
	    format = XA_STRING;
	}
	string = Tcl_GetStringFromObj(objs[1], &cmdLength);
	if (cmdLength == 0) {
	    Tk_DeleteSelHandler(tkwin, selection, target);
	} else {
	    cmdInfoPtr = ckalloc(Tk_Offset(CommandInfo, command)
		    + 1 + cmdLength);
	    cmdInfoPtr->interp = interp;
	    cmdInfoPtr->charOffset = 0;
	    cmdInfoPtr->byteOffset = 0;
	    cmdInfoPtr->buffer[0] = '\0';
	    cmdInfoPtr->cmdLength = cmdLength;
	    memcpy(cmdInfoPtr->command, string, cmdLength + 1);
	    Tk_CreateSelHandler(tkwin, selection, target, HandleTclCommand,
		    cmdInfoPtr, format);
	}
	return TCL_OK;
    }

    case SELECTION_OWN: {
	register LostCommand *lostPtr;
	Tcl_Obj *commandObj = NULL;
	static const char *const ownOptionStrings[] = {
	    "-command", "-displayof", "-selection", NULL
	};
	enum ownOptions { OWN_COMMAND, OWN_DISPLAYOF, OWN_SELECTION };
	int ownIndex;

	for (count = objc-2, objs = ((Tcl_Obj **)objv)+2; count > 0;
		count-=2, objs+=2) {
	    string = Tcl_GetString(objs[0]);
	    if (string[0] != '-') {
		break;
	    }
	    if (count < 2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"value for \"%s\" missing", string));
		Tcl_SetErrorCode(interp, "TK", "SELECTION", "VALUE", NULL);
		return TCL_ERROR;
	    }

	    if (Tcl_GetIndexFromObj(interp, objs[0], ownOptionStrings,
		    "option", 0, &ownIndex) != TCL_OK) {
		return TCL_ERROR;
	    }

	    switch ((enum ownOptions) ownIndex) {
	    case OWN_COMMAND:
		commandObj = objs[1];
		break;
	    case OWN_DISPLAYOF:
		path = Tcl_GetString(objs[1]);
		break;
	    case OWN_SELECTION:
		selName = Tcl_GetString(objs[1]);
		break;
	    }
	}

	if (count > 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-option value ...? ?window?");
	    return TCL_ERROR;
	}
	if (selName != NULL) {
	    selection = Tk_InternAtom(tkwin, selName);
	} else {
	    selection = XA_PRIMARY;
	}

	if (count == 0) {
	    TkSelectionInfo *infoPtr;
	    TkWindow *winPtr;

	    if (path != NULL) {
		tkwin = Tk_NameToWindow(interp, path, tkwin);
	    }
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    winPtr = (TkWindow *) tkwin;
	    for (infoPtr = winPtr->dispPtr->selectionInfoPtr;
		    infoPtr != NULL; infoPtr = infoPtr->nextPtr) {
		if (infoPtr->selection == selection) {
		    break;
		}
	    }

	    /*
	     * Ignore the internal clipboard window.
	     */

	    if ((infoPtr != NULL)
		    && (infoPtr->owner != winPtr->dispPtr->clipWindow)) {
		Tcl_SetObjResult(interp, TkNewWindowObj(infoPtr->owner));
	    }
	    return TCL_OK;
	}

	tkwin = Tk_NameToWindow(interp, Tcl_GetString(objs[0]), tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (count == 2) {
	    commandObj = objs[1];
	}
	if (commandObj == NULL) {
	    Tk_OwnSelection(tkwin, selection, NULL, NULL);
	    return TCL_OK;
	}
	lostPtr = ckalloc(sizeof(LostCommand));
	lostPtr->interp = interp;
	lostPtr->cmdObj = commandObj;
	Tcl_IncrRefCount(commandObj);
	Tk_OwnSelection(tkwin, selection, LostSelection, lostPtr);
	return TCL_OK;
    }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelGetInProgress --
 *
 *	This function returns a pointer to the thread-local list of pending
 *	searches.
 *
 * Results:
 *	The return value is a pointer to the first search in progress, or NULL
 *	if there are none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkSelInProgress *
TkSelGetInProgress(void)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    return tsdPtr->pendingPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelSetInProgress --
 *
 *	This function is used to set the thread-local list of pending
 *	searches. It is required because the pending list is kept in thread
 *	local storage.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
TkSelSetInProgress(
    TkSelInProgress *pendingPtr)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->pendingPtr = pendingPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelDeadWindow --
 *
 *	This function is invoked just before a TkWindow is deleted. It
 *	performs selection-related cleanup.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees up memory associated with the selection.
 *
 *----------------------------------------------------------------------
 */

void
TkSelDeadWindow(
    register TkWindow *winPtr)	/* Window that's being deleted. */
{
    register TkSelHandler *selPtr;
    register TkSelInProgress *ipPtr;
    TkSelectionInfo *infoPtr, *prevPtr, *nextPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * While deleting all the handlers, be careful to check whether
     * ConvertSelection or TkSelPropProc are about to process one of the
     * deleted handlers.
     */

    while (winPtr->selHandlerList != NULL) {
	selPtr = winPtr->selHandlerList;
	winPtr->selHandlerList = selPtr->nextPtr;
	for (ipPtr = tsdPtr->pendingPtr; ipPtr != NULL;
		ipPtr = ipPtr->nextPtr) {
	    if (ipPtr->selPtr == selPtr) {
		ipPtr->selPtr = NULL;
	    }
	}
	if (selPtr->proc == HandleTclCommand) {
	    /*
	     * Mark the CommandInfo as deleted and free it when we can.
	     */

	    ((CommandInfo *) selPtr->clientData)->interp = NULL;
	    Tcl_EventuallyFree(selPtr->clientData, TCL_DYNAMIC);
	}
	ckfree(selPtr);
    }

    /*
     * Remove selections owned by window being deleted.
     */

    for (infoPtr = winPtr->dispPtr->selectionInfoPtr, prevPtr = NULL;
	    infoPtr != NULL; infoPtr = nextPtr) {
	nextPtr = infoPtr->nextPtr;
	if (infoPtr->owner == (Tk_Window) winPtr) {
	    if (infoPtr->clearProc == LostSelection) {
		ckfree(infoPtr->clearData);
	    }
	    ckfree(infoPtr);
	    infoPtr = prevPtr;
	    if (prevPtr == NULL) {
		winPtr->dispPtr->selectionInfoPtr = nextPtr;
	    } else {
		prevPtr->nextPtr = nextPtr;
	    }
	}
	prevPtr = infoPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelInit --
 *
 *	Initialize selection-related information for a display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Selection-related information is initialized.
 *
 *----------------------------------------------------------------------
 */

void
TkSelInit(
    Tk_Window tkwin)		/* Window token (used to find display to
				 * initialize). */
{
    register TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    /*
     * Fetch commonly-used atoms.
     */

    dispPtr->multipleAtom	= Tk_InternAtom(tkwin, "MULTIPLE");
    dispPtr->incrAtom		= Tk_InternAtom(tkwin, "INCR");
    dispPtr->targetsAtom	= Tk_InternAtom(tkwin, "TARGETS");
    dispPtr->timestampAtom	= Tk_InternAtom(tkwin, "TIMESTAMP");
    dispPtr->textAtom		= Tk_InternAtom(tkwin, "TEXT");
    dispPtr->compoundTextAtom	= Tk_InternAtom(tkwin, "COMPOUND_TEXT");
    dispPtr->applicationAtom	= Tk_InternAtom(tkwin, "TK_APPLICATION");
    dispPtr->windowAtom		= Tk_InternAtom(tkwin, "TK_WINDOW");
    dispPtr->clipboardAtom	= Tk_InternAtom(tkwin, "CLIPBOARD");
    dispPtr->atomPairAtom	= Tk_InternAtom(tkwin, "ATOM_PAIR");

    /*
     * Using UTF8_STRING instead of the XA_UTF8_STRING macro allows us to
     * support older X servers that didn't have UTF8_STRING yet. This is
     * necessary on Unix systems. For more information, see:
     *	  http://www.cl.cam.ac.uk/~mgk25/unicode.html#x11
     */

#if !defined(_WIN32)
    dispPtr->utf8Atom		= Tk_InternAtom(tkwin, "UTF8_STRING");
#else
    dispPtr->utf8Atom		= (Atom) 0;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelClearSelection --
 *
 *	This function is invoked to process a SelectionClear event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invokes the clear function for the window which lost the
 *	selection.
 *
 *----------------------------------------------------------------------
 */

void
TkSelClearSelection(
    Tk_Window tkwin,		/* Window for which event was targeted. */
    register XEvent *eventPtr)	/* X SelectionClear event. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkSelectionInfo *infoPtr;
    TkSelectionInfo *prevPtr;

    /*
     * Invoke clear function for window that just lost the selection. This
     * code is a bit tricky, because any callbacks due to selection changes
     * between windows managed by the process have already been made. Thus,
     * ignore the event unless it refers to the window that's currently the
     * selection owner and the event was generated after the server saw the
     * SetSelectionOwner request.
     */

    for (infoPtr = dispPtr->selectionInfoPtr, prevPtr = NULL;
	    infoPtr != NULL; infoPtr = infoPtr->nextPtr) {
	if (infoPtr->selection == eventPtr->xselectionclear.selection) {
	    break;
	}
	prevPtr = infoPtr;
    }

    if (infoPtr != NULL && (infoPtr->owner == tkwin) &&
	    (eventPtr->xselectionclear.serial >= (unsigned) infoPtr->serial)) {
	if (prevPtr == NULL) {
	    dispPtr->selectionInfoPtr = infoPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = infoPtr->nextPtr;
	}

	/*
	 * Because of reentrancy problems, calling clearProc must be done
	 * after the infoPtr has been removed from the selectionInfoPtr list
	 * (clearProc could modify the list, e.g. by creating a new
	 * selection).
	 */

	if (infoPtr->clearProc != NULL) {
	    infoPtr->clearProc(infoPtr->clearData);
	}
	ckfree(infoPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * SelGetProc --
 *
 *	This function is invoked to process pieces of the selection as they
 *	arrive during "selection get" commands.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	Bytes get appended to the dynamic string pointed to by the clientData
 *	argument.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
SelGetProc(
    ClientData clientData,	/* Dynamic string holding partially assembled
				 * selection. */
    Tcl_Interp *interp,		/* Interpreter used for error reporting (not
				 * used). */
    const char *portion)	/* New information to be appended. */
{
    Tcl_DStringAppend(clientData, portion, -1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * HandleTclCommand --
 *
 *	This function acts as selection handler for handlers created by the
 *	"selection handle" command. It invokes a Tcl command to retrieve the
 *	selection.
 *
 * Results:
 *	The return value is a count of the number of bytes actually stored at
 *	buffer, or -1 if an error occurs while executing the Tcl command to
 *	retrieve the selection.
 *
 * Side effects:
 *	None except for things done by the Tcl command.
 *
 *----------------------------------------------------------------------
 */

static int
HandleTclCommand(
    ClientData clientData,	/* Information about command to execute. */
    int offset,			/* Return selection bytes starting at this
				 * offset. */
    char *buffer,		/* Place to store converted selection. */
    int maxBytes)		/* Maximum # of bytes to store at buffer. */
{
    CommandInfo *cmdInfoPtr = clientData;
    int length;
    Tcl_Obj *command;
    const char *string;
    Tcl_Interp *interp = cmdInfoPtr->interp;
    Tcl_InterpState savedState;
    int extraBytes, charOffset, count, numChars, code;
    const char *p;

    /*
     * We must also protect the interpreter and the command from being deleted
     * too soon.
     */

    Tcl_Preserve(clientData);
    Tcl_Preserve(interp);

    /*
     * Compute the proper byte offset in the case where the last chunk split a
     * character.
     */

    if (offset == cmdInfoPtr->byteOffset) {
	charOffset = cmdInfoPtr->charOffset;
	extraBytes = strlen(cmdInfoPtr->buffer);
	if (extraBytes > 0) {
	    strcpy(buffer, cmdInfoPtr->buffer);
	    maxBytes -= extraBytes;
	    buffer += extraBytes;
	}
    } else {
	cmdInfoPtr->byteOffset = 0;
	cmdInfoPtr->charOffset = 0;
	extraBytes = 0;
	charOffset = 0;
    }

    /*
     * First, generate a command by taking the command string and appending
     * the offset and maximum # of bytes.
     */

    command = Tcl_ObjPrintf("%s %d %d",
	    cmdInfoPtr->command, charOffset, maxBytes);
    Tcl_IncrRefCount(command);

    /*
     * Execute the command. Be sure to restore the state of the interpreter
     * after executing the command.
     */

    savedState = Tcl_SaveInterpState(interp, TCL_OK);
    code = Tcl_EvalObjEx(interp, command, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(command);
    if (code == TCL_OK) {
	/*
	 * TODO: This assumes that bytes are characters; that's not true!
	 */

	string = Tcl_GetStringFromObj(Tcl_GetObjResult(interp), &length);
	count = (length > maxBytes) ? maxBytes : length;
	memcpy(buffer, string, (size_t) count);
	buffer[count] = '\0';

	/*
	 * Update the partial character information for the next retrieval if
	 * the command has not been deleted.
	 */

	if (cmdInfoPtr->interp != NULL) {
	    if (length <= maxBytes) {
		cmdInfoPtr->charOffset += Tcl_NumUtfChars(string, -1);
		cmdInfoPtr->buffer[0] = '\0';
	    } else {
		p = string;
		string += count;
		numChars = 0;
		while (p < string) {
		    p = Tcl_UtfNext(p);
		    numChars++;
		}
		cmdInfoPtr->charOffset += numChars;
		length = p - string;
		if (length > 0) {
		    strncpy(cmdInfoPtr->buffer, string, (size_t) length);
		}
		cmdInfoPtr->buffer[length] = '\0';
	    }
	    cmdInfoPtr->byteOffset += count + extraBytes;
	}
	count += extraBytes;
    } else {
	/*
	 * Something went wrong. Log errors as background errors, and silently
	 * drop everything else.
	 */

	if (code == TCL_ERROR) {
	    Tcl_AddErrorInfo(interp, "\n    (command handling selection)");
	    Tcl_BackgroundException(interp, code);
	}
	count = -1;
    }
    (void) Tcl_RestoreInterpState(interp, savedState);

    Tcl_Release(clientData);
    Tcl_Release(interp);
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelDefaultSelection --
 *
 *	This function is called to generate selection information for a few
 *	standard targets such as TIMESTAMP and TARGETS. It is invoked only if
 *	no handler has been declared by the application.
 *
 * Results:
 *	If "target" is a standard target understood by this function, the
 *	selection is converted to that form and stored as a character string
 *	in buffer. The type of the selection (e.g. STRING or ATOM) is stored
 *	in *typePtr, and the return value is a count of the # of non-NULL
 *	bytes at buffer. If the target wasn't understood, or if there isn't
 *	enough space at buffer to hold the entire selection (no INCR-mode
 *	transfers for this stuff!), then -1 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkSelDefaultSelection(
    TkSelectionInfo *infoPtr,	/* Info about selection being retrieved. */
    Atom target,		/* Desired form of selection. */
    char *buffer,		/* Place to put selection characters. */
    int maxBytes,		/* Maximum # of bytes to store at buffer. */
    Atom *typePtr)		/* Store here the type of the selection, for
				 * use in converting to proper X format. */
{
    register TkWindow *winPtr = (TkWindow *) infoPtr->owner;
    TkDisplay *dispPtr = winPtr->dispPtr;

    if (target == dispPtr->timestampAtom) {
	if (maxBytes < 20) {
	    return -1;
	}
	sprintf(buffer, "0x%x", (unsigned int) infoPtr->time);
	*typePtr = XA_INTEGER;
	return strlen(buffer);
    }

    if (target == dispPtr->targetsAtom) {
	register TkSelHandler *selPtr;
	int length;
	Tcl_DString ds;

	if (maxBytes < 50) {
	    return -1;
	}
	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds,
		"MULTIPLE TARGETS TIMESTAMP TK_APPLICATION TK_WINDOW", -1);
	for (selPtr = winPtr->selHandlerList; selPtr != NULL;
		selPtr = selPtr->nextPtr) {
	    if ((selPtr->selection == infoPtr->selection)
		    && (selPtr->target != dispPtr->applicationAtom)
		    && (selPtr->target != dispPtr->windowAtom)) {
		const char *atomString = Tk_GetAtomName((Tk_Window) winPtr,
			selPtr->target);

		Tcl_DStringAppendElement(&ds, atomString);
	    }
	}
	length = Tcl_DStringLength(&ds);
	if (length >= maxBytes) {
	    Tcl_DStringFree(&ds);
	    return -1;
	}
	memcpy(buffer, Tcl_DStringValue(&ds), (unsigned) (1+length));
	Tcl_DStringFree(&ds);
	*typePtr = XA_ATOM;
	return length;
    }

    if (target == dispPtr->applicationAtom) {
	int length;
	Tk_Uid name = winPtr->mainPtr->winPtr->nameUid;

	length = strlen(name);
	if (maxBytes <= length) {
	    return -1;
	}
	strcpy(buffer, name);
	*typePtr = XA_STRING;
	return length;
    }

    if (target == dispPtr->windowAtom) {
	int length;
	char *name = winPtr->pathName;

	length = strlen(name);
	if (maxBytes <= length) {
	    return -1;
	}
	strcpy(buffer, name);
	*typePtr = XA_STRING;
	return length;
    }

    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * LostSelection --
 *
 *	This function is invoked when a window has lost ownership of the
 *	selection and the ownership was claimed with the command "selection
 *	own".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl script is executed; it can do almost anything.
 *
 *----------------------------------------------------------------------
 */

static void
LostSelection(
    ClientData clientData)	/* Pointer to LostCommand structure. */
{
    LostCommand *lostPtr = clientData;
    Tcl_Interp *interp = lostPtr->interp;
    Tcl_InterpState savedState;
    int code;

    Tcl_Preserve(interp);

    /*
     * Execute the command. Save the interpreter's result, if any, and restore
     * it after executing the command.
     */

    savedState = Tcl_SaveInterpState(interp, TCL_OK);
    Tcl_ResetResult(interp);
    code = Tcl_EvalObjEx(interp, lostPtr->cmdObj, TCL_EVAL_GLOBAL);
    if (code != TCL_OK) {
	Tcl_BackgroundException(interp, code);
    }
    (void) Tcl_RestoreInterpState(interp, savedState);

    /*
     * Free the storage for the command, since we're done with it now.
     */

    Tcl_DecrRefCount(lostPtr->cmdObj);
    ckfree(lostPtr);
    Tcl_Release(interp);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
