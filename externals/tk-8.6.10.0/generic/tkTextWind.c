/*
 * tkTextWind.c --
 *
 *	This file contains code that allows arbitrary windows to be nested
 *	inside text widgets. It also implements the "window" widget command
 *	for texts.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkPort.h"
#include "tkText.h"

/*
 * The following structure is the official type record for the embedded window
 * geometry manager:
 */

static void		EmbWinRequestProc(ClientData clientData,
			    Tk_Window tkwin);
static void		EmbWinLostSlaveProc(ClientData clientData,
			    Tk_Window tkwin);

static const Tk_GeomMgr textGeomType = {
    "text",			/* name */
    EmbWinRequestProc,		/* requestProc */
    EmbWinLostSlaveProc,	/* lostSlaveProc */
};

/*
 * Macro that determines the size of an embedded window segment:
 */

#define EW_SEG_SIZE ((unsigned) (Tk_Offset(TkTextSegment, body) \
	+ sizeof(TkTextEmbWindow)))

/*
 * Prototypes for functions defined in this file:
 */

static TkTextSegment *	EmbWinCleanupProc(TkTextSegment *segPtr,
			    TkTextLine *linePtr);
static void		EmbWinCheckProc(TkTextSegment *segPtr,
			    TkTextLine *linePtr);
static void		EmbWinBboxProc(TkText *textPtr,
			    TkTextDispChunk *chunkPtr, int index, int y,
			    int lineHeight, int baseline, int *xPtr,int *yPtr,
			    int *widthPtr, int *heightPtr);
static int		EmbWinConfigure(TkText *textPtr, TkTextSegment *ewPtr,
			    int objc, Tcl_Obj *const objv[]);
static void		EmbWinDelayedUnmap(ClientData clientData);
static int		EmbWinDeleteProc(TkTextSegment *segPtr,
			    TkTextLine *linePtr, int treeGone);
static int		EmbWinLayoutProc(TkText *textPtr,
			    TkTextIndex *indexPtr, TkTextSegment *segPtr,
			    int offset, int maxX, int maxChars,int noCharsYet,
			    TkWrapMode wrapMode, TkTextDispChunk *chunkPtr);
static void		EmbWinStructureProc(ClientData clientData,
			    XEvent *eventPtr);
static void		EmbWinUndisplayProc(TkText *textPtr,
			    TkTextDispChunk *chunkPtr);
static TkTextEmbWindowClient *EmbWinGetClient(const TkText *textPtr,
			    TkTextSegment *ewPtr);

/*
 * The following structure declares the "embedded window" segment type.
 */

const Tk_SegType tkTextEmbWindowType = {
    "window",			/* name */
    0,				/* leftGravity */
    NULL,			/* splitProc */
    EmbWinDeleteProc,		/* deleteProc */
    EmbWinCleanupProc,		/* cleanupProc */
    NULL,			/* lineChangeProc */
    EmbWinLayoutProc,		/* layoutProc */
    EmbWinCheckProc		/* checkProc */
};

/*
 * Definitions for alignment values:
 */

static const char *const alignStrings[] = {
    "baseline", "bottom", "center", "top", NULL
};

typedef enum {
    ALIGN_BASELINE, ALIGN_BOTTOM, ALIGN_CENTER, ALIGN_TOP
} alignMode;

/*
 * Information used for parsing window configuration options:
 */

static const Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_STRING_TABLE, "-align", NULL, NULL,
	"center", -1, Tk_Offset(TkTextEmbWindow, align),
	0, alignStrings, 0},
    {TK_OPTION_STRING, "-create", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextEmbWindow, create), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-padx", NULL, NULL,
	"0", -1, Tk_Offset(TkTextEmbWindow, padX), 0, 0, 0},
    {TK_OPTION_PIXELS, "-pady", NULL, NULL,
	"0", -1, Tk_Offset(TkTextEmbWindow, padY), 0, 0, 0},
    {TK_OPTION_BOOLEAN, "-stretch", NULL, NULL,
	"0", -1, Tk_Offset(TkTextEmbWindow, stretch), 0, 0, 0},
    {TK_OPTION_WINDOW, "-window", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextEmbWindow, tkwin), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

/*
 *--------------------------------------------------------------
 *
 * TkTextWindowCmd --
 *
 *	This function implements the "window" widget command for text widgets.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result or error.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
TkTextWindowCmd(
    register TkText *textPtr,	/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "window". */
{
    int optionIndex;
    static const char *const windOptionStrings[] = {
	"cget", "configure", "create", "names", NULL
    };
    enum windOptions {
	WIND_CGET, WIND_CONFIGURE, WIND_CREATE, WIND_NAMES
    };
    register TkTextSegment *ewPtr;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[2], windOptionStrings,
	    sizeof(char *), "window option", 0, &optionIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum windOptions) optionIndex) {
    case WIND_CGET: {
	TkTextIndex index;
	TkTextSegment *ewPtr;
	Tcl_Obj *objPtr;
	TkTextEmbWindowClient *client;

	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index option");
	    return TCL_ERROR;
	}
	if (TkTextGetObjIndex(interp, textPtr, objv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	ewPtr = TkTextIndexToSeg(&index, NULL);
	if (ewPtr->typePtr != &tkTextEmbWindowType) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "no embedded window at index \"%s\"",
		    Tcl_GetString(objv[3])));
	    Tcl_SetErrorCode(interp, "TK", "TEXT", "NO_WINDOW", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Copy over client specific value before querying.
	 */

	client = EmbWinGetClient(textPtr, ewPtr);
	if (client != NULL) {
	    ewPtr->body.ew.tkwin = client->tkwin;
	} else {
	    ewPtr->body.ew.tkwin = NULL;
	}

	objPtr = Tk_GetOptionValue(interp, (char *) &ewPtr->body.ew,
		ewPtr->body.ew.optionTable, objv[4], textPtr->tkwin);
	if (objPtr == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, objPtr);
	return TCL_OK;
    }
    case WIND_CONFIGURE: {
	TkTextIndex index;
	TkTextSegment *ewPtr;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index ?-option value ...?");
	    return TCL_ERROR;
	}
	if (TkTextGetObjIndex(interp, textPtr, objv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	ewPtr = TkTextIndexToSeg(&index, NULL);
	if (ewPtr->typePtr != &tkTextEmbWindowType) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "no embedded window at index \"%s\"",
		    Tcl_GetString(objv[3])));
	    Tcl_SetErrorCode(interp, "TK", "TEXT", "NO_WINDOW", NULL);
	    return TCL_ERROR;
	}
	if (objc <= 5) {
	    TkTextEmbWindowClient *client;
	    Tcl_Obj *objPtr;

	    /*
	     * Copy over client specific value before querying.
	     */

	    client = EmbWinGetClient(textPtr, ewPtr);
	    if (client != NULL) {
		ewPtr->body.ew.tkwin = client->tkwin;
	    } else {
		ewPtr->body.ew.tkwin = NULL;
	    }

	    objPtr = Tk_GetOptionInfo(interp, (char *) &ewPtr->body.ew,
		    ewPtr->body.ew.optionTable, (objc == 5) ? objv[4] : NULL,
		    textPtr->tkwin);
	    if (objPtr == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    return TCL_OK;
	} else {
	    TkTextChanged(textPtr->sharedTextPtr, NULL, &index, &index);

	    /*
	     * It's probably not true that all window configuration can change
	     * the line height, so we could be more efficient here and only
	     * call this when necessary.
	     */

	    TkTextInvalidateLineMetrics(textPtr->sharedTextPtr, NULL,
		    index.linePtr, 0, TK_TEXT_INVALIDATE_ONLY);
	    return EmbWinConfigure(textPtr, ewPtr, objc-4, objv+4);
	}
    }
    case WIND_CREATE: {
	TkTextIndex index;
	int lineIndex;
	TkTextEmbWindowClient *client;
	int res;

	/*
	 * Add a new window. Find where to put the new window, and mark that
	 * position for redisplay.
	 */

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index ?-option value ...?");
	    return TCL_ERROR;
	}
	if (TkTextGetObjIndex(interp, textPtr, objv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * Don't allow insertions on the last (dummy) line of the text.
	 */

	lineIndex = TkBTreeLinesTo(textPtr, index.linePtr);
	if (lineIndex == TkBTreeNumLines(textPtr->sharedTextPtr->tree,
		textPtr)) {
	    lineIndex--;
	    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		    lineIndex, 1000000, &index);
	}

	/*
	 * Create the new window segment and initialize it.
	 */

	ewPtr = ckalloc(EW_SEG_SIZE);
	ewPtr->typePtr = &tkTextEmbWindowType;
	ewPtr->size = 1;
	ewPtr->body.ew.sharedTextPtr = textPtr->sharedTextPtr;
	ewPtr->body.ew.linePtr = NULL;
	ewPtr->body.ew.tkwin = NULL;
	ewPtr->body.ew.create = NULL;
	ewPtr->body.ew.align = ALIGN_CENTER;
	ewPtr->body.ew.padX = ewPtr->body.ew.padY = 0;
	ewPtr->body.ew.stretch = 0;
	ewPtr->body.ew.optionTable = Tk_CreateOptionTable(interp, optionSpecs);

	client = ckalloc(sizeof(TkTextEmbWindowClient));
	client->next = NULL;
	client->textPtr = textPtr;
	client->tkwin = NULL;
	client->chunkCount = 0;
	client->displayed = 0;
	client->parent = ewPtr;
	ewPtr->body.ew.clients = client;

	/*
	 * Link the segment into the text widget, then configure it (delete it
	 * again if the configuration fails).
	 */

	TkTextChanged(textPtr->sharedTextPtr, NULL, &index, &index);
	TkBTreeLinkSegment(ewPtr, &index);
	res = EmbWinConfigure(textPtr, ewPtr, objc-4, objv+4);
	client->tkwin = ewPtr->body.ew.tkwin;
	if (res != TCL_OK) {
	    TkTextIndex index2;

	    TkTextIndexForwChars(NULL, &index, 1, &index2, COUNT_INDICES);
	    TkBTreeDeleteIndexRange(textPtr->sharedTextPtr->tree, &index,
		    &index2);
	    return TCL_ERROR;
	}
	TkTextInvalidateLineMetrics(textPtr->sharedTextPtr, NULL,
		index.linePtr, 0, TK_TEXT_INVALIDATE_ONLY);
	break;
    }
    case WIND_NAMES: {
	Tcl_HashSearch search;
	Tcl_HashEntry *hPtr;
	Tcl_Obj *resultObj;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	resultObj = Tcl_NewObj();
	for (hPtr = Tcl_FirstHashEntry(&textPtr->sharedTextPtr->windowTable,
		&search); hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		    Tcl_GetHashKey(&textPtr->sharedTextPtr->markTable, hPtr),
		    -1));
	}
	Tcl_SetObjResult(interp, resultObj);
	break;
    }
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinConfigure --
 *
 *	This function is called to handle configuration options for an
 *	embedded window, using an objc/objv list.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message..
 *
 * Side effects:
 *	Configuration information for the embedded window changes, such as
 *	alignment, stretching, or name of the embedded window.
 *
 *	Note that this function may leave widget specific client information
 *	with a NULL tkwin attached to ewPtr. While we could choose to clean up
 *	the client data structure here, there is no need to do so, and it is
 *	likely that the user is going to adjust the tkwin again soon.
 *
 *--------------------------------------------------------------
 */

static int
EmbWinConfigure(
    TkText *textPtr,		/* Information about text widget that contains
				 * embedded window. */
    TkTextSegment *ewPtr,	/* Embedded window to be configured. */
    int objc,			/* Number of strings in objv. */
    Tcl_Obj *const objv[])	/* Array of objects describing configuration
				 * options. */
{
    Tk_Window oldWindow;
    TkTextEmbWindowClient *client;

    /*
     * Copy over client specific value before querying or setting.
     */

    client = EmbWinGetClient(textPtr, ewPtr);
    if (client != NULL) {
	ewPtr->body.ew.tkwin = client->tkwin;
    } else {
	ewPtr->body.ew.tkwin = NULL;
    }

    oldWindow = ewPtr->body.ew.tkwin;
    if (Tk_SetOptions(textPtr->interp, (char *) &ewPtr->body.ew,
	    ewPtr->body.ew.optionTable, objc, objv, textPtr->tkwin, NULL,
	    NULL) != TCL_OK) {
	return TCL_ERROR;
    }

    if (oldWindow != ewPtr->body.ew.tkwin) {
	if (oldWindow != NULL) {
	    Tcl_DeleteHashEntry(Tcl_FindHashEntry(
		    &textPtr->sharedTextPtr->windowTable,
		    Tk_PathName(oldWindow)));
	    Tk_DeleteEventHandler(oldWindow, StructureNotifyMask,
		    EmbWinStructureProc, client);
	    Tk_ManageGeometry(oldWindow, NULL, NULL);
	    if (textPtr->tkwin != Tk_Parent(oldWindow)) {
		Tk_UnmaintainGeometry(oldWindow, textPtr->tkwin);
	    } else {
		Tk_UnmapWindow(oldWindow);
	    }
	}
	if (client != NULL) {
	    client->tkwin = NULL;
	}
	if (ewPtr->body.ew.tkwin != NULL) {
	    Tk_Window ancestor, parent;
	    Tcl_HashEntry *hPtr;
	    int isNew;

	    /*
	     * Make sure that the text is either the parent of the embedded
	     * window or a descendant of that parent. Also, don't allow a
	     * top-level window to be managed inside a text.
	     */

	    parent = Tk_Parent(ewPtr->body.ew.tkwin);
	    for (ancestor = textPtr->tkwin; ; ancestor = Tk_Parent(ancestor)) {
		if (ancestor == parent) {
		    break;
		}
		if (Tk_TopWinHierarchy(ancestor)) {
		badMaster:
		    Tcl_SetObjResult(textPtr->interp, Tcl_ObjPrintf(
			    "can't embed %s in %s",
			    Tk_PathName(ewPtr->body.ew.tkwin),
			    Tk_PathName(textPtr->tkwin)));
		    Tcl_SetErrorCode(textPtr->interp, "TK", "GEOMETRY",
			    "HIERARCHY", NULL);
		    ewPtr->body.ew.tkwin = NULL;
		    if (client != NULL) {
			client->tkwin = NULL;
		    }
		    return TCL_ERROR;
		}
	    }
	    if (Tk_TopWinHierarchy(ewPtr->body.ew.tkwin)
		    || (ewPtr->body.ew.tkwin == textPtr->tkwin)) {
		goto badMaster;
	    }

	    if (client == NULL) {
		/*
		 * Have to make the new client.
		 */

		client = ckalloc(sizeof(TkTextEmbWindowClient));
		client->next = ewPtr->body.ew.clients;
		client->textPtr = textPtr;
		client->tkwin = NULL;
		client->chunkCount = 0;
		client->displayed = 0;
		client->parent = ewPtr;
		ewPtr->body.ew.clients = client;
	    }
	    client->tkwin = ewPtr->body.ew.tkwin;

	    /*
	     * Take over geometry management for the window, plus create an
	     * event handler to find out when it is deleted.
	     */

	    Tk_ManageGeometry(ewPtr->body.ew.tkwin, &textGeomType, client);
	    Tk_CreateEventHandler(ewPtr->body.ew.tkwin, StructureNotifyMask,
		    EmbWinStructureProc, client);

	    /*
	     * Special trick!  Must enter into the hash table *after* calling
	     * Tk_ManageGeometry: if the window was already managed elsewhere
	     * in this text, the Tk_ManageGeometry call will cause the entry
	     * to be removed, which could potentially lose the new entry.
	     */

	    hPtr = Tcl_CreateHashEntry(&textPtr->sharedTextPtr->windowTable,
		    Tk_PathName(ewPtr->body.ew.tkwin), &isNew);
	    Tcl_SetHashValue(hPtr, ewPtr);
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinStructureProc --
 *
 *	This function is invoked by the Tk event loop whenever StructureNotify
 *	events occur for a window that's embedded in a text widget. This
 *	function's only purpose is to clean up when windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is disassociated from the window segment, and the portion
 *	of the text is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
EmbWinStructureProc(
    ClientData clientData,	/* Pointer to record describing window item. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    TkTextEmbWindowClient *client = clientData;
    TkTextSegment *ewPtr = client->parent;
    TkTextIndex index;
    Tcl_HashEntry *hPtr;

    if (eventPtr->type != DestroyNotify) {
	return;
    }

    hPtr = Tcl_FindHashEntry(&ewPtr->body.ew.sharedTextPtr->windowTable,
	    Tk_PathName(client->tkwin));
    if (hPtr != NULL) {
	/*
	 * This may not exist if the entire widget is being deleted.
	 */

	Tcl_DeleteHashEntry(hPtr);
    }

    ewPtr->body.ew.tkwin = NULL;
    client->tkwin = NULL;
    index.tree = ewPtr->body.ew.sharedTextPtr->tree;
    index.linePtr = ewPtr->body.ew.linePtr;
    index.byteIndex = TkTextSegToOffset(ewPtr, ewPtr->body.ew.linePtr);
    TkTextChanged(ewPtr->body.ew.sharedTextPtr, NULL, &index, &index);
    TkTextInvalidateLineMetrics(ewPtr->body.ew.sharedTextPtr, NULL,
	    index.linePtr, 0, TK_TEXT_INVALIDATE_ONLY);
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinRequestProc --
 *
 *	This function is invoked whenever a window that's associated with a
 *	window canvas item changes its requested dimensions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size and location on the screen of the window may change,
 *	depending on the options specified for the window item.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
EmbWinRequestProc(
    ClientData clientData,	/* Pointer to record for window item. */
    Tk_Window tkwin)		/* Window that changed its desired size. */
{
    TkTextEmbWindowClient *client = clientData;
    TkTextSegment *ewPtr = client->parent;
    TkTextIndex index;

    index.tree = ewPtr->body.ew.sharedTextPtr->tree;
    index.linePtr = ewPtr->body.ew.linePtr;
    index.byteIndex = TkTextSegToOffset(ewPtr, ewPtr->body.ew.linePtr);
    TkTextChanged(ewPtr->body.ew.sharedTextPtr, NULL, &index, &index);
    TkTextInvalidateLineMetrics(ewPtr->body.ew.sharedTextPtr, NULL,
	    index.linePtr, 0, TK_TEXT_INVALIDATE_ONLY);
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinLostSlaveProc --
 *
 *	This function is invoked by the Tk geometry manager when a slave
 *	window managed by a text widget is claimed away by another geometry
 *	manager.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is disassociated from the window segment, and the portion
 *	of the text is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
EmbWinLostSlaveProc(
    ClientData clientData,	/* Pointer to record describing window item. */
    Tk_Window tkwin)		/* Window that was claimed away by another
				 * geometry manager. */
{
    TkTextEmbWindowClient *client = clientData;
    TkTextSegment *ewPtr = client->parent;
    TkTextIndex index;
    Tcl_HashEntry *hPtr;
    TkTextEmbWindowClient *loop;

    Tk_DeleteEventHandler(client->tkwin, StructureNotifyMask,
	    EmbWinStructureProc, client);
    Tcl_CancelIdleCall(EmbWinDelayedUnmap, client);
    if (client->textPtr->tkwin != Tk_Parent(tkwin)) {
	Tk_UnmaintainGeometry(tkwin, client->textPtr->tkwin);
    } else {
	Tk_UnmapWindow(tkwin);
    }
    hPtr = Tcl_FindHashEntry(&ewPtr->body.ew.sharedTextPtr->windowTable,
	    Tk_PathName(client->tkwin));
    Tcl_DeleteHashEntry(hPtr);
    client->tkwin = NULL;
    ewPtr->body.ew.tkwin = NULL;

    /*
     * Free up the memory allocation for this client.
     */

    loop = ewPtr->body.ew.clients;
    if (loop == client) {
	ewPtr->body.ew.clients = client->next;
    } else {
	while (loop->next != client) {
	    loop = loop->next;
	}
	loop->next = client->next;
    }
    ckfree(client);

    index.tree = ewPtr->body.ew.sharedTextPtr->tree;
    index.linePtr = ewPtr->body.ew.linePtr;
    index.byteIndex = TkTextSegToOffset(ewPtr, ewPtr->body.ew.linePtr);
    TkTextChanged(ewPtr->body.ew.sharedTextPtr, NULL, &index, &index);
    TkTextInvalidateLineMetrics(ewPtr->body.ew.sharedTextPtr, NULL,
	    index.linePtr, 0, TK_TEXT_INVALIDATE_ONLY);
}

/*
 *--------------------------------------------------------------
 *
 * TkTextWinFreeClient --
 *
 *	Free up the hash entry and client information for a given embedded
 *	window.
 *
 *	It is assumed the caller will manage the linked list of clients
 *	associated with the relevant TkTextSegment.
 *
 * Results:
 *	Nothing.
 *
 * Side effects:
 *	The embedded window information for a single client is deleted, if it
 *	exists, and any resources associated with it are released.
 *
 *--------------------------------------------------------------
 */

void
TkTextWinFreeClient(
    Tcl_HashEntry *hPtr,	/* Hash entry corresponding to this client, or
				 * NULL */
    TkTextEmbWindowClient *client)
				/* Client data structure, with the 'tkwin'
				 * field to be cleaned up. */
{
    if (hPtr != NULL) {
	/*
	 * (It's possible for there to be no hash table entry for this window,
	 * if an error occurred while creating the window segment but before
	 * the window got added to the table)
	 */

	Tcl_DeleteHashEntry(hPtr);
    }

    /*
     * Delete the event handler for the window before destroying the window,
     * so that EmbWinStructureProc doesn't get called (we'll already do
     * everything that it would have done, and it will just get confused).
     */

    if (client->tkwin != NULL) {
	Tk_DeleteEventHandler(client->tkwin, StructureNotifyMask,
		EmbWinStructureProc, client);
	Tk_DestroyWindow(client->tkwin);
    }
    Tcl_CancelIdleCall(EmbWinDelayedUnmap, client);

    /*
     * Free up this client.
     */

    ckfree(client);
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinDeleteProc --
 *
 *	This function is invoked by the text B-tree code whenever an embedded
 *	window lies in a range of characters being deleted.
 *
 * Results:
 *	Returns 0 to indicate that the deletion has been accepted.
 *
 * Side effects:
 *	The embedded window is deleted, if it exists, and any resources
 *	associated with it are released.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
EmbWinDeleteProc(
    TkTextSegment *ewPtr,	/* Segment being deleted. */
    TkTextLine *linePtr,	/* Line containing segment. */
    int treeGone)		/* Non-zero means the entire tree is being
				 * deleted, so everything must get cleaned
				 * up. */
{
    TkTextEmbWindowClient *client;
    client = ewPtr->body.ew.clients;

    while (client != NULL) {
	TkTextEmbWindowClient *next = client->next;
	Tcl_HashEntry *hPtr = NULL;

	if (client->tkwin != NULL) {
	    hPtr = Tcl_FindHashEntry(
		    &ewPtr->body.ew.sharedTextPtr->windowTable,
		    Tk_PathName(client->tkwin));
	}
	TkTextWinFreeClient(hPtr, client);
	client = next;
    }
    ewPtr->body.ew.clients = NULL;

    Tk_FreeConfigOptions((char *) &ewPtr->body.ew, ewPtr->body.ew.optionTable,
	    NULL);

    /*
     * Free up all memory allocated.
     */

    ckfree(ewPtr);
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinCleanupProc --
 *
 *	This function is invoked by the B-tree code whenever a segment
 *	containing an embedded window is moved from one line to another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The linePtr field of the segment gets updated.
 *
 *--------------------------------------------------------------
 */

static TkTextSegment *
EmbWinCleanupProc(
    TkTextSegment *ewPtr,	/* Mark segment that's being moved. */
    TkTextLine *linePtr)	/* Line that now contains segment. */
{
    ewPtr->body.ew.linePtr = linePtr;
    return ewPtr;
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinLayoutProc --
 *
 *	This function is the "layoutProc" for embedded window segments.
 *
 * Results:
 *	1 is returned to indicate that the segment should be displayed. The
 *	chunkPtr structure is filled in.
 *
 * Side effects:
 *	None, except for filling in chunkPtr.
 *
 *--------------------------------------------------------------
 */

	/*ARGSUSED*/
static int
EmbWinLayoutProc(
    TkText *textPtr,		/* Text widget being layed out. */
    TkTextIndex *indexPtr,	/* Identifies first character in chunk. */
    TkTextSegment *ewPtr,	/* Segment corresponding to indexPtr. */
    int offset,			/* Offset within segPtr corresponding to
				 * indexPtr (always 0). */
    int maxX,			/* Chunk must not occupy pixels at this
				 * position or higher. */
    int maxChars,		/* Chunk must not include more than this many
				 * characters. */
    int noCharsYet,		/* Non-zero means no characters have been
				 * assigned to this line yet. */
    TkWrapMode wrapMode,	/* Wrap mode to use for line:
				 * TEXT_WRAPMODE_CHAR, TEXT_WRAPMODE_NONE, or
				 * TEXT_WRAPMODE_WORD. */
    register TkTextDispChunk *chunkPtr)
				/* Structure to fill in with information about
				 * this chunk. The x field has already been
				 * set by the caller. */
{
    int width, height;
    TkTextEmbWindowClient *client;

    if (offset != 0) {
	Tcl_Panic("Non-zero offset in EmbWinLayoutProc");
    }

    client = EmbWinGetClient(textPtr, ewPtr);
    if (client == NULL) {
	ewPtr->body.ew.tkwin = NULL;
    } else {
	ewPtr->body.ew.tkwin = client->tkwin;
    }

    if ((ewPtr->body.ew.tkwin == NULL) && (ewPtr->body.ew.create != NULL)) {
	int code, isNew;
	Tk_Window ancestor;
	Tcl_HashEntry *hPtr;
	const char *before, *string;
	Tcl_DString buf, *dsPtr = NULL;
	Tcl_Obj *nameObj;

	before = ewPtr->body.ew.create;

	/*
	 * Find everything up to the next % character and append it to the
	 * result string.
	 */

	string = before;
	while (*string != 0) {
	    if ((*string == '%') && (string[1] == '%' || string[1] == 'W')) {
		if (dsPtr == NULL) {
		    Tcl_DStringInit(&buf);
		    dsPtr = &buf;
		}
		if (string != before) {
		    Tcl_DStringAppend(dsPtr, before, (int) (string-before));
		    before = string;
		}
		if (string[1] == '%') {
		    Tcl_DStringAppend(dsPtr, "%", 1);
		} else {
		    /*
		     * Substitute string as proper Tcl list element.
		     */

		    int spaceNeeded, cvtFlags, length;
		    const char *str = Tk_PathName(textPtr->tkwin);

		    spaceNeeded = Tcl_ScanElement(str, &cvtFlags);
		    length = Tcl_DStringLength(dsPtr);
		    Tcl_DStringSetLength(dsPtr, length + spaceNeeded);
		    spaceNeeded = Tcl_ConvertElement(str,
			    Tcl_DStringValue(dsPtr) + length,
			    cvtFlags | TCL_DONT_USE_BRACES);
		    Tcl_DStringSetLength(dsPtr, length + spaceNeeded);
		}
		before += 2;
		string++;
	    }
	    string++;
	}

	/*
	 * The window doesn't currently exist. Create it by evaluating the
	 * creation script. The script must return the window's path name:
	 * look up that name to get back to the window token. Then register
	 * ourselves as the geometry manager for the window.
	 */

	if (dsPtr != NULL) {
	    Tcl_DStringAppend(dsPtr, before, (int) (string-before));
	    code = Tcl_EvalEx(textPtr->interp, Tcl_DStringValue(dsPtr), -1, TCL_EVAL_GLOBAL);
	    Tcl_DStringFree(dsPtr);
	} else {
	    code = Tcl_EvalEx(textPtr->interp, ewPtr->body.ew.create, -1, TCL_EVAL_GLOBAL);
	}
	if (code != TCL_OK) {
	    Tcl_BackgroundException(textPtr->interp, code);
	    goto gotWindow;
	}
	nameObj = Tcl_GetObjResult(textPtr->interp);
	Tcl_IncrRefCount(nameObj);
	Tcl_ResetResult(textPtr->interp);
	ewPtr->body.ew.tkwin = Tk_NameToWindow(textPtr->interp,
		Tcl_GetString(nameObj), textPtr->tkwin);
	Tcl_DecrRefCount(nameObj);
	if (ewPtr->body.ew.tkwin == NULL) {
	    Tcl_BackgroundException(textPtr->interp, TCL_ERROR);
	    goto gotWindow;
	}

	for (ancestor = textPtr->tkwin; ; ancestor = Tk_Parent(ancestor)) {
	    if (ancestor == Tk_Parent(ewPtr->body.ew.tkwin)) {
		break;
	    }
	    if (Tk_TopWinHierarchy(ancestor)) {
		goto badMaster;
	    }
	}
	if (Tk_TopWinHierarchy(ewPtr->body.ew.tkwin)
		|| (textPtr->tkwin == ewPtr->body.ew.tkwin)) {
	badMaster:
	    Tcl_SetObjResult(textPtr->interp, Tcl_ObjPrintf(
		    "can't embed %s relative to %s",
		    Tk_PathName(ewPtr->body.ew.tkwin),
		    Tk_PathName(textPtr->tkwin)));
	    Tcl_SetErrorCode(textPtr->interp, "TK", "GEOMETRY", "HIERARCHY",
		    NULL);
	    Tcl_BackgroundException(textPtr->interp, TCL_ERROR);
	    ewPtr->body.ew.tkwin = NULL;
	    goto gotWindow;
	}

	if (client == NULL) {
	    /*
	     * We just used a '-create' script to make a new window, which we
	     * now need to add to our client list.
	     */

	    client = ckalloc(sizeof(TkTextEmbWindowClient));
	    client->next = ewPtr->body.ew.clients;
	    client->textPtr = textPtr;
	    client->tkwin = NULL;
	    client->chunkCount = 0;
	    client->displayed = 0;
	    client->parent = ewPtr;
	    ewPtr->body.ew.clients = client;
	}

	client->tkwin = ewPtr->body.ew.tkwin;
	Tk_ManageGeometry(client->tkwin, &textGeomType, client);
	Tk_CreateEventHandler(client->tkwin, StructureNotifyMask,
		EmbWinStructureProc, client);

	/*
	 * Special trick! Must enter into the hash table *after* calling
	 * Tk_ManageGeometry: if the window was already managed elsewhere in
	 * this text, the Tk_ManageGeometry call will cause the entry to be
	 * removed, which could potentially lose the new entry.
	 */

	hPtr = Tcl_CreateHashEntry(&textPtr->sharedTextPtr->windowTable,
		Tk_PathName(client->tkwin), &isNew);
	Tcl_SetHashValue(hPtr, ewPtr);
    }

    /*
     * See if there's room for this window on this line.
     */

  gotWindow:
    if (ewPtr->body.ew.tkwin == NULL) {
	width = 0;
	height = 0;
    } else {
	width = Tk_ReqWidth(ewPtr->body.ew.tkwin) + 2*ewPtr->body.ew.padX;
	height = Tk_ReqHeight(ewPtr->body.ew.tkwin) + 2*ewPtr->body.ew.padY;
    }
    if ((width > (maxX - chunkPtr->x))
	    && !noCharsYet && (textPtr->wrapMode != TEXT_WRAPMODE_NONE)) {
	return 0;
    }

    /*
     * Fill in the chunk structure.
     */

    chunkPtr->displayProc = TkTextEmbWinDisplayProc;
    chunkPtr->undisplayProc = EmbWinUndisplayProc;
    chunkPtr->measureProc = NULL;
    chunkPtr->bboxProc = EmbWinBboxProc;
    chunkPtr->numBytes = 1;
    if (ewPtr->body.ew.align == ALIGN_BASELINE) {
	chunkPtr->minAscent = height - ewPtr->body.ew.padY;
	chunkPtr->minDescent = ewPtr->body.ew.padY;
	chunkPtr->minHeight = 0;
    } else {
	chunkPtr->minAscent = 0;
	chunkPtr->minDescent = 0;
	chunkPtr->minHeight = height;
    }
    chunkPtr->width = width;
    chunkPtr->breakIndex = -1;
    chunkPtr->breakIndex = 1;
    chunkPtr->clientData = ewPtr;
    if (client != NULL) {
	client->chunkCount += 1;
    }
    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinCheckProc --
 *
 *	This function is invoked by the B-tree code to perform consistency
 *	checks on embedded windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The function panics if it detects anything wrong with the embedded
 *	window.
 *
 *--------------------------------------------------------------
 */

static void
EmbWinCheckProc(
    TkTextSegment *ewPtr,	/* Segment to check. */
    TkTextLine *linePtr)	/* Line containing segment. */
{
    if (ewPtr->nextPtr == NULL) {
	Tcl_Panic("EmbWinCheckProc: embedded window is last segment in line");
    }
    if (ewPtr->size != 1) {
	Tcl_Panic("EmbWinCheckProc: embedded window has size %d", ewPtr->size);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkTextEmbWinDisplayProc --
 *
 *	This function is invoked by the text displaying code when it is time
 *	to actually draw an embedded window chunk on the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The embedded window gets moved to the correct location and mapped onto
 *	the screen.
 *
 *--------------------------------------------------------------
 */

void
TkTextEmbWinDisplayProc(
    TkText *textPtr,		/* Information about text widget. */
    TkTextDispChunk *chunkPtr,	/* Chunk that is to be drawn. */
    int x,			/* X-position in dst at which to draw this
				 * chunk (differs from the x-position in the
				 * chunk because of scrolling). */
    int y,			/* Top of rectangular bounding box for line:
				 * tells where to draw this chunk in dst
				 * (x-position is in the chunk itself). */
    int lineHeight,		/* Total height of line. */
    int baseline,		/* Offset of baseline from y. */
    Display *display,		/* Display to use for drawing (unused).  */
    Drawable dst,		/* Pixmap or window in which to draw
				 * (unused).  */
    int screenY)		/* Y-coordinate in text window that
				 * corresponds to y. */
{
    int lineX, windowX, windowY, width, height;
    Tk_Window tkwin;
    TkTextSegment *ewPtr = chunkPtr->clientData;
    TkTextEmbWindowClient *client = EmbWinGetClient(textPtr, ewPtr);

    if (client == NULL) {
	return;
    }

    tkwin = client->tkwin;
    if (tkwin == NULL) {
	return;
    }

    if ((x + chunkPtr->width) <= 0) {
	/*
	 * The window is off-screen; just unmap it.
	 */

	if (textPtr->tkwin != Tk_Parent(tkwin)) {
	    Tk_UnmaintainGeometry(tkwin, textPtr->tkwin);
	} else {
	    Tk_UnmapWindow(tkwin);
	}
	return;
    }

    /*
     * Compute the window's location and size in the text widget, taking into
     * account the align and stretch values for the window.
     */

    EmbWinBboxProc(textPtr, chunkPtr, 0, screenY, lineHeight, baseline,
	    &lineX, &windowY, &width, &height);
    windowX = lineX - chunkPtr->x + x;

    /*
     * Mark the window as displayed so that it won't get unmapped.
     * This needs to be done before the next instruction block because
     * Tk_MaintainGeometry/Tk_MapWindow will run event handlers, in
     * particular for the <Map> event, and if the bound script deletes
     * the embedded window its clients will get freed.
     */

    client->displayed = 1;

    if (textPtr->tkwin == Tk_Parent(tkwin)) {
	if ((windowX != Tk_X(tkwin)) || (windowY != Tk_Y(tkwin))
		|| (Tk_ReqWidth(tkwin) != Tk_Width(tkwin))
		|| (height != Tk_Height(tkwin))) {
	    Tk_MoveResizeWindow(tkwin, windowX, windowY, width, height);
	}
	Tk_MapWindow(tkwin);
    } else {
	Tk_MaintainGeometry(tkwin, textPtr->tkwin, windowX, windowY,
		width, height);
    }
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinUndisplayProc --
 *
 *	This function is called when the chunk for an embedded window is no
 *	longer going to be displayed. It arranges for the window associated
 *	with the chunk to be unmapped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is scheduled for unmapping.
 *
 *--------------------------------------------------------------
 */

static void
EmbWinUndisplayProc(
    TkText *textPtr,		/* Overall information about text widget. */
    TkTextDispChunk *chunkPtr)	/* Chunk that is about to be freed. */
{
    TkTextSegment *ewPtr = chunkPtr->clientData;
    TkTextEmbWindowClient *client = EmbWinGetClient(textPtr, ewPtr);

    if (client == NULL) {
	return;
    }

    client->chunkCount--;
    if (client->chunkCount == 0) {
	/*
	 * Don't unmap the window immediately, since there's a good chance
	 * that it will immediately be redisplayed, perhaps even in the same
	 * place. Instead, schedule the window to be unmapped later; the call
	 * to EmbWinDelayedUnmap will be cancelled in the likely event that
	 * the unmap becomes unnecessary.
	 */

	client->displayed = 0;
	Tcl_DoWhenIdle(EmbWinDelayedUnmap, client);
    }
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinBboxProc --
 *
 *	This function is called to compute the bounding box of the area
 *	occupied by an embedded window.
 *
 * Results:
 *	There is no return value. *xPtr and *yPtr are filled in with the
 *	coordinates of the upper left corner of the window, and *widthPtr and
 *	*heightPtr are filled in with the dimensions of the window in pixels.
 *	Note: not all of the returned bbox is necessarily visible on the
 *	screen (the rightmost part might be off-screen to the right, and the
 *	bottommost part might be off-screen to the bottom).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
EmbWinBboxProc(
    TkText *textPtr,		/* Information about text widget. */
    TkTextDispChunk *chunkPtr,	/* Chunk containing desired char. */
    int index,			/* Index of desired character within the
				 * chunk. */
    int y,			/* Topmost pixel in area allocated for this
				 * line. */
    int lineHeight,		/* Total height of line. */
    int baseline,		/* Location of line's baseline, in pixels
				 * measured down from y. */
    int *xPtr, int *yPtr,	/* Gets filled in with coords of character's
				 * upper-left pixel. */
    int *widthPtr,		/* Gets filled in with width of window, in
				 * pixels. */
    int *heightPtr)		/* Gets filled in with height of window, in
				 * pixels. */
{
    Tk_Window tkwin;
    TkTextSegment *ewPtr = chunkPtr->clientData;
    TkTextEmbWindowClient *client = EmbWinGetClient(textPtr, ewPtr);

    if (client == NULL) {
	tkwin = NULL;
    } else {
	tkwin = client->tkwin;
    }
    if (tkwin != NULL) {
	*widthPtr = Tk_ReqWidth(tkwin);
	*heightPtr = Tk_ReqHeight(tkwin);
    } else {
	*widthPtr = 0;
	*heightPtr = 0;
    }
    *xPtr = chunkPtr->x + ewPtr->body.ew.padX;
    if (ewPtr->body.ew.stretch) {
	if (ewPtr->body.ew.align == ALIGN_BASELINE) {
	    *heightPtr = baseline - ewPtr->body.ew.padY;
	} else {
	    *heightPtr = lineHeight - 2*ewPtr->body.ew.padY;
	}
    }
    switch (ewPtr->body.ew.align) {
    case ALIGN_BOTTOM:
	*yPtr = y + (lineHeight - *heightPtr - ewPtr->body.ew.padY);
	break;
    case ALIGN_CENTER:
	*yPtr = y + (lineHeight - *heightPtr)/2;
	break;
    case ALIGN_TOP:
	*yPtr = y + ewPtr->body.ew.padY;
	break;
    case ALIGN_BASELINE:
	*yPtr = y + (baseline - *heightPtr);
	break;
    }
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinDelayedUnmap --
 *
 *	This function is an idle handler that does the actual work of
 *	unmapping an embedded window. See the comment in EmbWinUndisplayProc
 *	for details.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window gets unmapped, unless its chunk reference count has become
 *	non-zero again.
 *
 *--------------------------------------------------------------
 */

static void
EmbWinDelayedUnmap(
    ClientData clientData)	/* Token for the window to be unmapped. */
{
    TkTextEmbWindowClient *client = clientData;

    if (!client->displayed && (client->tkwin != NULL)) {
	if (client->textPtr->tkwin != Tk_Parent(client->tkwin)) {
	    Tk_UnmaintainGeometry(client->tkwin, client->textPtr->tkwin);
	} else {
	    Tk_UnmapWindow(client->tkwin);
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkTextWindowIndex --
 *
 *	Given the name of an embedded window within a text widget, returns an
 *	index corresponding to the window's position in the text.
 *
 * Results:
 *	The return value is 1 if there is an embedded window by the given name
 *	in the text widget, 0 otherwise. If the window exists, *indexPtr is
 *	filled in with its index.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkTextWindowIndex(
    TkText *textPtr,		/* Text widget containing window. */
    const char *name,		/* Name of window. */
    TkTextIndex *indexPtr)	/* Index information gets stored here. */
{
    Tcl_HashEntry *hPtr;
    TkTextSegment *ewPtr;

    if (textPtr == NULL) {
	return 0;
    }

    hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->windowTable, name);
    if (hPtr == NULL) {
	return 0;
    }

    ewPtr = Tcl_GetHashValue(hPtr);
    indexPtr->tree = textPtr->sharedTextPtr->tree;
    indexPtr->linePtr = ewPtr->body.ew.linePtr;
    indexPtr->byteIndex = TkTextSegToOffset(ewPtr, indexPtr->linePtr);
    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * EmbWinGetClient --
 *
 *	Given a text widget and a segment which contains an embedded window,
 *	find the text-widget specific information about the embedded window,
 *	if any.
 *
 *	This function performs a completely linear lookup for a matching data
 *	structure. If we envisage using this code with dozens of peer widgets,
 *	then performance could become an issue and a more sophisticated lookup
 *	mechanism might be desirable.
 *
 * Results:
 *	NULL if no widget-specific info exists, otherwise the structure is
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static TkTextEmbWindowClient *
EmbWinGetClient(
    const TkText *textPtr,	/* Information about text widget. */
    TkTextSegment *ewPtr)	/* Segment containing embedded window. */
{
    TkTextEmbWindowClient *client = ewPtr->body.ew.clients;

    while (client != NULL) {
	if (client->textPtr == textPtr) {
	    return client;
	}
	client = client->next;
    }
    return NULL;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
