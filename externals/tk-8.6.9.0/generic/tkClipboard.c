/*
 * tkClipboard.c --
 *
 * 	This file manages the clipboard for the Tk toolkit, maintaining a
 * 	collection of data buffers that will be supplied on demand to
 * 	requesting applications.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkSelect.h"

/*
 * Prototypes for functions used only in this file:
 */

static int		ClipboardAppHandler(ClientData clientData,
			    int offset, char *buffer, int maxBytes);
static int		ClipboardHandler(ClientData clientData,
			    int offset, char *buffer, int maxBytes);
static int		ClipboardWindowHandler(ClientData clientData,
			    int offset, char *buffer, int maxBytes);
static void		ClipboardLostSel(ClientData clientData);
static int		ClipboardGetProc(ClientData clientData,
			    Tcl_Interp *interp, const char *portion);

/*
 *----------------------------------------------------------------------
 *
 * ClipboardHandler --
 *
 *	This function acts as selection handler for the clipboard manager. It
 *	extracts the required chunk of data from the buffer chain for a given
 *	selection target.
 *
 * Results:
 *	The return value is a count of the number of bytes actually stored at
 *	buffer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ClipboardHandler(
    ClientData clientData,	/* Information about data to fetch. */
    int offset,			/* Return selection bytes starting at this
				 * offset. */
    char *buffer,		/* Place to store converted selection. */
    int maxBytes)		/* Maximum # of bytes to store at buffer. */
{
    TkClipboardTarget *targetPtr = clientData;
    TkClipboardBuffer *cbPtr;
    char *srcPtr, *destPtr;
    size_t count = 0;
    int scanned = 0;
    size_t length, freeCount;

    /*
     * Skip to buffer containing offset byte
     */

    for (cbPtr = targetPtr->firstBufferPtr; ; cbPtr = cbPtr->nextPtr) {
	if (cbPtr == NULL) {
	    return 0;
	}
	if (scanned + cbPtr->length > offset) {
	    break;
	}
	scanned += cbPtr->length;
    }

    /*
     * Copy up to maxBytes or end of list, switching buffers as needed.
     */

    freeCount = maxBytes;
    srcPtr = cbPtr->buffer + (offset - scanned);
    destPtr = buffer;
    length = cbPtr->length - (offset - scanned);
    while (1) {
	if (length > freeCount) {
	    strncpy(destPtr, srcPtr, freeCount);
	    return maxBytes;
	} else {
	    strncpy(destPtr, srcPtr, length);
	    destPtr += length;
	    count += length;
	    freeCount -= length;
	}
	cbPtr = cbPtr->nextPtr;
	if (cbPtr == NULL) {
	    break;
	}
	srcPtr = cbPtr->buffer;
	length = cbPtr->length;
    }
    return (int)count;
}

/*
 *----------------------------------------------------------------------
 *
 * ClipboardAppHandler --
 *
 *	This function acts as selection handler for retrievals of type
 *	TK_APPLICATION. It returns the name of the application that owns the
 *	clipboard. Note: we can't use the default Tk selection handler for
 *	this selection type, because the clipboard window isn't a "real"
 *	window and doesn't have the necessary information.
 *
 * Results:
 *	The return value is a count of the number of bytes actually stored at
 *	buffer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ClipboardAppHandler(
    ClientData clientData,	/* Pointer to TkDisplay structure. */
    int offset,			/* Return selection bytes starting at this
				 * offset. */
    char *buffer,		/* Place to store converted selection. */
    int maxBytes)		/* Maximum # of bytes to store at buffer. */
{
    TkDisplay *dispPtr = clientData;
    size_t length;
    const char *p;

    p = dispPtr->clipboardAppPtr->winPtr->nameUid;
    length = strlen(p);
    length -= offset;
    if (length <= 0) {
	return 0;
    }
    if (length > (size_t) maxBytes) {
	length = maxBytes;
    }
    strncpy(buffer, p, length);
    return (int)length;
}

/*
 *----------------------------------------------------------------------
 *
 * ClipboardWindowHandler --
 *
 *	This function acts as selection handler for retrievals of type
 *	TK_WINDOW. Since the clipboard doesn't correspond to any particular
 *	window, we just return ".". We can't use Tk's default handler for this
 *	selection type, because the clipboard window isn't a valid window.
 *
 * Results:
 *	The return value is 1, the number of non-null bytes stored at buffer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ClipboardWindowHandler(
    ClientData clientData,	/* Not used. */
    int offset,			/* Return selection bytes starting at this
				 * offset. */
    char *buffer,		/* Place to store converted selection. */
    int maxBytes)		/* Maximum # of bytes to store at buffer. */
{
    buffer[0] = '.';
    buffer[1] = 0;
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * ClipboardLostSel --
 *
 *	This function is invoked whenever clipboard ownership is claimed by
 *	another window. It just sets a flag so that we know the clipboard was
 *	taken away.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The clipboard is marked as inactive.
 *
 *----------------------------------------------------------------------
 */

static void
ClipboardLostSel(
    ClientData clientData)	/* Pointer to TkDisplay structure. */
{
    TkDisplay *dispPtr = clientData;

    dispPtr->clipboardActive = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ClipboardClear --
 *
 *	Take control of the clipboard and clear out the previous contents.
 *	This function must be invoked before any calls to Tk_ClipboardAppend.
 *
 * Results:
 *	A standard Tcl result. If an error occurs, an error message is left in
 *	the interp's result.
 *
 * Side effects:
 *	From now on, requests for the CLIPBOARD selection will be directed to
 *	the clipboard manager routines associated with clipWindow for the
 *	display of tkwin. In order to guarantee atomicity, no event handling
 *	should occur between Tk_ClipboardClear and the following
 *	Tk_ClipboardAppend calls. This function may cause a user-defined
 *	LostSel command to be invoked when the CLIPBOARD is claimed, so any
 *	calling function should be reentrant at the point Tk_ClipboardClear is
 *	invoked.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ClipboardClear(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin)		/* Window in application that is clearing
				 * clipboard; identifies application and
				 * display. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkClipboardTarget *targetPtr, *nextTargetPtr;
    TkClipboardBuffer *cbPtr, *nextCbPtr;

    if (dispPtr->clipWindow == NULL) {
	int result;

	result = TkClipInit(interp, dispPtr);
	if (result != TCL_OK) {
	    return result;
	}
    }

    /*
     * Discard any existing clipboard data and delete the selection handler(s)
     * associated with that data.
     */

    for (targetPtr = dispPtr->clipTargetPtr; targetPtr != NULL;
	    targetPtr = nextTargetPtr) {
	for (cbPtr = targetPtr->firstBufferPtr; cbPtr != NULL;
		cbPtr = nextCbPtr) {
	    ckfree(cbPtr->buffer);
	    nextCbPtr = cbPtr->nextPtr;
	    ckfree(cbPtr);
	}
	nextTargetPtr = targetPtr->nextPtr;
	Tk_DeleteSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
		targetPtr->type);
	ckfree(targetPtr);
    }
    dispPtr->clipTargetPtr = NULL;

    /*
     * Reclaim the clipboard selection if we lost it.
     */

    if (!dispPtr->clipboardActive) {
	Tk_OwnSelection(dispPtr->clipWindow, dispPtr->clipboardAtom,
		ClipboardLostSel, dispPtr);
	dispPtr->clipboardActive = 1;
    }
    dispPtr->clipboardAppPtr = winPtr->mainPtr;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ClipboardAppend --
 *
 *	Append a buffer of data to the clipboard. The first buffer of a given
 *	type determines the format for that type. Any successive appends to
 *	that type must have the same format or an error will be returned.
 *	Tk_ClipboardClear must be called before a sequence of
 *	Tk_ClipboardAppend calls can be issued. In order to guarantee
 *	atomicity, no event handling should occur between Tk_ClipboardClear
 *	and the following Tk_ClipboardAppend calls.
 *
 * Results:
 *	A standard Tcl result. If an error is returned, an error message is
 *	left in the interp's result.
 *
 * Side effects:
 *	The specified buffer will be copied onto the end of the clipboard.
 *	The clipboard maintains a list of buffers which will be used to supply
 *	the data for a selection get request. The first time a given type is
 *	appended, Tk_ClipboardAppend will register a selection handler of the
 *	appropriate type.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ClipboardAppend(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Window tkwin,		/* Window that selects a display. */
    Atom type,			/* The desired conversion type for this
				 * clipboard item, e.g. STRING or LENGTH. */
    Atom format,		/* Format in which the selection information
				 * should be returned to the requestor. */
    const char *buffer)		/* NULL terminated string containing the data
				 * to be added to the clipboard. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    TkClipboardTarget *targetPtr;
    TkClipboardBuffer *cbPtr;

    /*
     * If this application doesn't already own the clipboard, clear the
     * clipboard. If we don't own the clipboard selection, claim it.
     */

    if (dispPtr->clipboardAppPtr != winPtr->mainPtr) {
	Tk_ClipboardClear(interp, tkwin);
    } else if (!dispPtr->clipboardActive) {
	Tk_OwnSelection(dispPtr->clipWindow, dispPtr->clipboardAtom,
		ClipboardLostSel, dispPtr);
	dispPtr->clipboardActive = 1;
    }

    /*
     * Check to see if the specified target is already present on the
     * clipboard. If it isn't, we need to create a new target; otherwise, we
     * just append the new buffer to the clipboard list.
     */

    for (targetPtr = dispPtr->clipTargetPtr; targetPtr != NULL;
	    targetPtr = targetPtr->nextPtr) {
	if (targetPtr->type == type) {
	    break;
	}
    }
    if (targetPtr == NULL) {
	targetPtr = ckalloc(sizeof(TkClipboardTarget));
	targetPtr->type = type;
	targetPtr->format = format;
	targetPtr->firstBufferPtr = targetPtr->lastBufferPtr = NULL;
	targetPtr->nextPtr = dispPtr->clipTargetPtr;
	dispPtr->clipTargetPtr = targetPtr;
	Tk_CreateSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
		type, ClipboardHandler, targetPtr, format);
    } else if (targetPtr->format != format) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"format \"%s\" does not match current format \"%s\" for %s",
		Tk_GetAtomName(tkwin, format),
		Tk_GetAtomName(tkwin, targetPtr->format),
		Tk_GetAtomName(tkwin, type)));
	Tcl_SetErrorCode(interp, "TK", "CLIPBOARD", "FORMAT_MISMATCH", NULL);
	return TCL_ERROR;
    }

    /*
     * Append a new buffer to the buffer chain.
     */

    cbPtr = ckalloc(sizeof(TkClipboardBuffer));
    cbPtr->nextPtr = NULL;
    if (targetPtr->lastBufferPtr != NULL) {
	targetPtr->lastBufferPtr->nextPtr = cbPtr;
    } else {
	targetPtr->firstBufferPtr = cbPtr;
    }
    targetPtr->lastBufferPtr = cbPtr;

    cbPtr->length = strlen(buffer);
    cbPtr->buffer = ckalloc(cbPtr->length + 1);
    strcpy(cbPtr->buffer, buffer);

    TkSelUpdateClipboard((TkWindow *) dispPtr->clipWindow, targetPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ClipboardObjCmd --
 *
 *	This function is invoked to process the "clipboard" Tcl command. See
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
Tk_ClipboardObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    const char *path = NULL;
    Atom selection;
    static const char *const optionStrings[] = { "append", "clear", "get", NULL };
    enum options { CLIPBOARD_APPEND, CLIPBOARD_CLEAR, CLIPBOARD_GET };
    int index, i;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case CLIPBOARD_APPEND: {
	Atom target, format;
	const char *targetName = NULL;
	const char *formatName = NULL;
	const char *string;
	static const char *const appendOptionStrings[] = {
	    "-displayof", "-format", "-type", NULL
	};
	enum appendOptions { APPEND_DISPLAYOF, APPEND_FORMAT, APPEND_TYPE };
	int subIndex, length;

	for (i = 2; i < objc - 1; i++) {
	    string = Tcl_GetStringFromObj(objv[i], &length);
	    if (string[0] != '-') {
		break;
	    }

	    /*
	     * If the argument is "--", it signifies the end of arguments.
	     */
	    if (string[1] == '-' && length == 2) {
		i++;
		break;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[i], appendOptionStrings,
		    "option", 0, &subIndex) != TCL_OK) {
		return TCL_ERROR;
	    }

	    /*
	     * Increment i so that it points to the value for the flag instead
	     * of the flag itself.
	     */

	    i++;
	    if (i >= objc) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"value for \"%s\" missing", string));
		Tcl_SetErrorCode(interp, "TK", "CLIPBOARD", "VALUE", NULL);
		return TCL_ERROR;
	    }
	    switch ((enum appendOptions) subIndex) {
	    case APPEND_DISPLAYOF:
		path = Tcl_GetString(objv[i]);
		break;
	    case APPEND_FORMAT:
		formatName = Tcl_GetString(objv[i]);
		break;
	    case APPEND_TYPE:
		targetName = Tcl_GetString(objv[i]);
		break;
	    }
	}
	if (objc - i != 1) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-option value ...? data");
	    return TCL_ERROR;
	}
	if (path != NULL) {
	    tkwin = Tk_NameToWindow(interp, path, tkwin);
	}
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (targetName != NULL) {
	    target = Tk_InternAtom(tkwin, targetName);
	} else {
	    target = XA_STRING;
	}
	if (formatName != NULL) {
	    format = Tk_InternAtom(tkwin, formatName);
	} else {
	    format = XA_STRING;
	}
	return Tk_ClipboardAppend(interp, tkwin, target, format,
		Tcl_GetString(objv[i]));
    }
    case CLIPBOARD_CLEAR: {
	static const char *const clearOptionStrings[] = { "-displayof", NULL };
	enum clearOptions { CLEAR_DISPLAYOF };
	int subIndex;

	if (objc != 2 && objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-displayof window?");
	    return TCL_ERROR;
	}

	if (objc == 4) {
	    if (Tcl_GetIndexFromObj(interp, objv[2], clearOptionStrings,
		    "option", 0, &subIndex) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((enum clearOptions) subIndex == CLEAR_DISPLAYOF) {
		path = Tcl_GetString(objv[3]);
	    }
	}
	if (path != NULL) {
	    tkwin = Tk_NameToWindow(interp, path, tkwin);
	}
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	return Tk_ClipboardClear(interp, tkwin);
    }
    case CLIPBOARD_GET: {
	Atom target;
	const char *targetName = NULL;
	Tcl_DString selBytes;
	int result;
	const char *string;
	static const char *const getOptionStrings[] = {
	    "-displayof", "-type", NULL
	};
	enum getOptions { APPEND_DISPLAYOF, APPEND_TYPE };
	int subIndex;

	for (i = 2; i < objc; i++) {
	    string = Tcl_GetString(objv[i]);
	    if (string[0] != '-') {
		break;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[i], getOptionStrings,
		    "option", 0, &subIndex) != TCL_OK) {
		return TCL_ERROR;
	    }
	    i++;
	    if (i >= objc) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"value for \"%s\" missing", string));
		Tcl_SetErrorCode(interp, "TK", "CLIPBOARD", "VALUE", NULL);
		return TCL_ERROR;
	    }
	    switch ((enum getOptions) subIndex) {
	    case APPEND_DISPLAYOF:
		path = Tcl_GetString(objv[i]);
		break;
	    case APPEND_TYPE:
		targetName = Tcl_GetString(objv[i]);
		break;
	    }
	}
	if (path != NULL) {
	    tkwin = Tk_NameToWindow(interp, path, tkwin);
	}
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	selection = Tk_InternAtom(tkwin, "CLIPBOARD");

	if (objc - i > 1) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-option value ...?");
	    return TCL_ERROR;
	} else if (objc - i == 1) {
	    target = Tk_InternAtom(tkwin, Tcl_GetString(objv[i]));
	} else if (targetName != NULL) {
	    target = Tk_InternAtom(tkwin, targetName);
	} else {
	    target = XA_STRING;
	}

	Tcl_DStringInit(&selBytes);
	result = Tk_GetSelection(interp, tkwin, selection, target,
		ClipboardGetProc, &selBytes);
	if (result == TCL_OK) {
	    Tcl_DStringResult(interp, &selBytes);
	} else {
	    Tcl_DStringFree(&selBytes);
	}
	return result;
    }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkClipInit --
 *
 *	This function is called to initialize the window for claiming
 *	clipboard ownership and for receiving selection get results. This
 *	function is called from tkSelect.c as well as tkClipboard.c.
 *
 * Results:
 *	The result is a standard Tcl return value, which is normally TCL_OK.
 *	If an error occurs then an error message is left in the interp's
 *	result and TCL_ERROR is returned.
 *
 * Side effects:
 *	Sets up the clipWindow and related data structures.
 *
 *----------------------------------------------------------------------
 */

int
TkClipInit(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    register TkDisplay *dispPtr)/* Display to initialize. */
{
    XSetWindowAttributes atts;

    dispPtr->clipTargetPtr = NULL;
    dispPtr->clipboardActive = 0;
    dispPtr->clipboardAppPtr = NULL;

    /*
     * Create the window used for clipboard ownership and selection retrieval,
     * and set up an event handler for it.
     */

    dispPtr->clipWindow = (Tk_Window) TkAllocWindow(dispPtr,
	DefaultScreen(dispPtr->display), NULL);
    Tcl_Preserve(dispPtr->clipWindow);
    ((TkWindow *) dispPtr->clipWindow)->flags |=
	    TK_TOP_HIERARCHY|TK_TOP_LEVEL|TK_HAS_WRAPPER|TK_WIN_MANAGED;
    TkWmNewWindow((TkWindow *) dispPtr->clipWindow);
    atts.override_redirect = True;
    Tk_ChangeWindowAttributes(dispPtr->clipWindow, CWOverrideRedirect, &atts);
    Tk_MakeWindowExist(dispPtr->clipWindow);

    if (dispPtr->multipleAtom == None) {
	/*
	 * Need to invoke selection initialization to make sure that atoms we
	 * depend on below are defined.
	 */

	TkSelInit(dispPtr->clipWindow);
    }

    /*
     * Create selection handlers for types TK_APPLICATION and TK_WINDOW on
     * this window. Can't use the default handlers for these types because
     * this isn't a full-fledged window.
     */

    Tk_CreateSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
	    dispPtr->applicationAtom, ClipboardAppHandler, dispPtr,XA_STRING);
    Tk_CreateSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
	    dispPtr->windowAtom, ClipboardWindowHandler, dispPtr, XA_STRING);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ClipboardGetProc --
 *
 *	This function is invoked to process pieces of the selection as they
 *	arrive during "clipboard get" commands.
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
ClipboardGetProc(
    ClientData clientData,	/* Dynamic string holding partially assembled
				 * selection. */
    Tcl_Interp *interp,		/* Interpreter used for error reporting (not
				 * used). */
    const char *portion)	/* New information to be appended. */
{
    Tcl_DStringAppend((Tcl_DString *) clientData, portion, -1);
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
