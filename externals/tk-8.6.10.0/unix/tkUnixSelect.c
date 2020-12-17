/*
 * tkUnixSelect.c --
 *
 *	This file contains X specific routines for manipulating selections.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkSelect.h"

typedef struct ConvertInfo {
    int offset;			/* The starting byte offset into the selection
				 * for the next chunk; -1 means all data has
				 * been transferred for this conversion. -2
				 * means only the final zero-length transfer
				 * still has to be done. Otherwise it is the
				 * offset of the next chunk of data to
				 * transfer. */
    Tcl_EncodingState state;	/* The encoding state needed across chunks. */
    char buffer[4];	/* A buffer to hold part of a UTF character
				 * that is split across chunks.*/
} ConvertInfo;

/*
 * When handling INCR-style selection retrievals, the selection owner uses the
 * following data structure to communicate between the ConvertSelection
 * function and TkSelPropProc.
 */

typedef struct IncrInfo {
    TkWindow *winPtr;		/* Window that owns selection. */
    Atom selection;		/* Selection that is being retrieved. */
    Atom *multAtoms;		/* Information about conversions to perform:
				 * one or more pairs of (target, property).
				 * This either points to a retrieved property
				 * (for MULTIPLE retrievals) or to a static
				 * array. */
    unsigned long numConversions;
				/* Number of entries in converts (same as # of
				 * pairs in multAtoms). */
    ConvertInfo *converts;	/* One entry for each pair in multAtoms. This
				 * array is malloc-ed. */
    char **tempBufs;		/* One pointer for each pair in multAtoms;
				 * each pointer is either NULL, or it points
				 * to a small bit of character data that was
				 * left over from the previous chunk. */
    Tcl_EncodingState *state;	/* One state info per pair in multAtoms: State
				 * info for encoding conversions that span
				 * multiple buffers. */
    int *flags;			/* One state flag per pair in multAtoms:
				 * Encoding flags, set to TCL_ENCODING_START
				 * at the beginning of an INCR transfer. */
    int numIncrs;		/* Number of entries in converts that aren't
				 * -1 (i.e. # of INCR-mode transfers not yet
				 * completed). */
    Tcl_TimerToken timeout;	/* Token for timer function. */
    int idleTime;		/* Number of seconds since we heard anything
				 * from the selection requestor. */
    Window reqWindow;		/* Requestor's window id. */
    Time time;			/* Timestamp corresponding to selection at
				 * beginning of request; used to abort
				 * transfer if selection changes. */
    struct IncrInfo *nextPtr;	/* Next in list of all INCR-style retrievals
				 * currently pending. */
} IncrInfo;

typedef struct {
    IncrInfo *pendingIncrs;	/* List of all incr structures currently
				 * active. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Largest property that we'll accept when sending or receiving the selection:
 */

#define MAX_PROP_WORDS 100000

static TkSelRetrievalInfo *pendingRetrievals = NULL;
				/* List of all retrievals currently being
				 * waited for. */

/*
 * Forward declarations for functions defined in this file:
 */

static void		ConvertSelection(TkWindow *winPtr,
			    XSelectionRequestEvent *eventPtr);
static void		IncrTimeoutProc(ClientData clientData);
static void		SelCvtFromX32(long *propPtr, int numValues, Atom type,
			    Tk_Window tkwin, Tcl_DString *dsPtr);
static void		SelCvtFromX8(char *propPtr, int numValues, Atom type,
			    Tk_Window tkwin, Tcl_DString *dsPtr);
static long *		SelCvtToX(char *string, Atom type, Tk_Window tkwin,
			    int *numLongsPtr);
static int		SelectionSize(TkSelHandler *selPtr);
static void		SelRcvIncrProc(ClientData clientData,
			    XEvent *eventPtr);
static void		SelTimeoutProc(ClientData clientData);

/*
 *----------------------------------------------------------------------
 *
 * TkSelGetSelection --
 *
 *	Retrieve the specified selection from another process.
 *
 * Results:
 *	The return value is a standard Tcl return value. If an error occurs
 *	(such as no selection exists) then an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkSelGetSelection(
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
    TkSelRetrievalInfo retr;
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;

    /*
     * The selection is owned by some other process. To retrieve it, first
     * record information about the retrieval in progress. Use an internal
     * window as the requestor.
     */

    retr.interp = interp;
    if (dispPtr->clipWindow == NULL) {
	int result;

	result = TkClipInit(interp, dispPtr);
	if (result != TCL_OK) {
	    return result;
	}
    }
    retr.winPtr = (TkWindow *) dispPtr->clipWindow;
    retr.selection = selection;
    retr.property = selection;
    retr.target = target;
    retr.proc = proc;
    retr.clientData = clientData;
    retr.result = -1;
    retr.idleTime = 0;
    retr.encFlags = TCL_ENCODING_START;
    retr.nextPtr = pendingRetrievals;
    Tcl_DStringInit(&retr.buf);
    pendingRetrievals = &retr;

    /*
     * Delete the property to indicate that no parameters are supplied for
     * the conversion request.
     */

    XDeleteProperty(winPtr->display, retr.winPtr->window, retr.property);

    /*
     * Initiate the request for the selection. Note: can't use TkCurrentTime
     * for the time. If we do, and this application hasn't received any X
     * events in a long time, the current time will be way in the past and
     * could even predate the time when the selection was made; if this
     * happens, the request will be rejected.
     */

    XConvertSelection(winPtr->display, retr.selection, retr.target,
	    retr.property, retr.winPtr->window, CurrentTime);

    /*
     * Enter a loop processing X events until the selection has been retrieved
     * and processed. If no response is received within a few seconds, then
     * timeout.
     */

    retr.timeout = Tcl_CreateTimerHandler(1000, SelTimeoutProc,
	    &retr);
    while (retr.result == -1) {
	Tcl_DoOneEvent(0);
    }
    Tcl_DeleteTimerHandler(retr.timeout);

    /*
     * Unregister the information about the selection retrieval in progress.
     */

    if (pendingRetrievals == &retr) {
	pendingRetrievals = retr.nextPtr;
    } else {
	TkSelRetrievalInfo *retrPtr;

	for (retrPtr = pendingRetrievals; retrPtr != NULL;
		retrPtr = retrPtr->nextPtr) {
	    if (retrPtr->nextPtr == &retr) {
		retrPtr->nextPtr = retr.nextPtr;
		break;
	    }
	}
    }
    Tcl_DStringFree(&retr.buf);
    return retr.result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelPropProc --
 *
 *	This function is invoked when property-change events occur on windows
 *	not known to the toolkit. Its function is to implement the sending
 *	side of the INCR selection retrieval protocol when the selection
 *	requestor deletes the property containing a part of the selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the property that is receiving the selection was just deleted, then
 *	a new piece of the selection is fetched and placed in the property,
 *	until eventually there's no more selection to fetch.
 *
 *----------------------------------------------------------------------
 */

void
TkSelPropProc(
    register XEvent *eventPtr)	/* X PropertyChange event. */
{
    register IncrInfo *incrPtr;
    register TkSelHandler *selPtr;
    int length, numItems;
    unsigned long i;
    Atom target, formatType;
    long buffer[TK_SEL_WORDS_AT_ONCE];
    TkDisplay *dispPtr = TkGetDisplay(eventPtr->xany.display);
    Tk_ErrorHandler errorHandler;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * See if this event announces the deletion of a property being used for
     * an INCR transfer. If so, then add the next chunk of data to the
     * property.
     */

    if (eventPtr->xproperty.state != PropertyDelete) {
	return;
    }
    for (incrPtr = tsdPtr->pendingIncrs; incrPtr != NULL;
	    incrPtr = incrPtr->nextPtr) {
	if (incrPtr->reqWindow != eventPtr->xproperty.window) {
	    continue;
	}

	/*
	 * For each conversion that has been requested, handle any chunks that
	 * haven't been transmitted yet.
	 */

	for (i = 0; i < incrPtr->numConversions; i++) {
	    if ((eventPtr->xproperty.atom != incrPtr->multAtoms[2*i + 1])
		    || (incrPtr->converts[i].offset == -1)) {
		continue;
	    }
	    target = incrPtr->multAtoms[2*i];
	    incrPtr->idleTime = 0;

	    /*
	     * Look for a matching selection handler.
	     */

	    for (selPtr = incrPtr->winPtr->selHandlerList; ;
		    selPtr = selPtr->nextPtr) {
		if (selPtr == NULL) {
		    /*
		     * No handlers match, so mark the conversion as done.
		     */

		    incrPtr->multAtoms[2*i + 1] = None;
		    incrPtr->converts[i].offset = -1;
		    incrPtr->numIncrs --;
		    return;
		}
		if ((selPtr->target == target)
			&& (selPtr->selection == incrPtr->selection)) {
		    break;
		}
	    }

	    /*
	     * We found a handler, so get the next chunk from it.
	     */

	    formatType = selPtr->format;
	    if (incrPtr->converts[i].offset == -2) {
		/*
		 * We already got the last chunk, so send a null chunk to
		 * indicate that we are finished.
		 */

		numItems = 0;
		length = 0;
	    } else {
		TkSelInProgress ip;

		ip.selPtr = selPtr;
		ip.nextPtr = TkSelGetInProgress();
		TkSelSetInProgress(&ip);

		/*
		 * Copy any bytes left over from a partial character at the
		 * end of the previous chunk into the beginning of the buffer.
		 * Pass the rest of the buffer space into the selection
		 * handler.
		 */

		length = strlen(incrPtr->converts[i].buffer);
		strcpy((char *)buffer, incrPtr->converts[i].buffer);

		numItems = selPtr->proc(selPtr->clientData,
			incrPtr->converts[i].offset,
			((char *) buffer) + length,
			TK_SEL_BYTES_AT_ONCE - length);
		TkSelSetInProgress(ip.nextPtr);
		if (ip.selPtr == NULL) {
		    /*
		     * The selection handler deleted itself.
		     */

		    return;
		}
		if (numItems < 0) {
		    numItems = 0;
		}
		numItems += length;
		if (numItems > TK_SEL_BYTES_AT_ONCE) {
		    Tcl_Panic("selection handler returned too many bytes");
		}
	    }
	    ((char *) buffer)[numItems] = 0;

	    errorHandler = Tk_CreateErrorHandler(eventPtr->xproperty.display,
		    -1, -1, -1, (int (*)()) NULL, NULL);

	    /*
	     * Encode the data using the proper format for each type.
	     */

	    if ((formatType == XA_STRING)
		    || (dispPtr && formatType==dispPtr->utf8Atom)
		    || (dispPtr && formatType==dispPtr->compoundTextAtom)) {
		Tcl_DString ds;
		int encodingCvtFlags;
		int srcLen, dstLen, result, srcRead, dstWrote, soFar;
		char *src, *dst;
		Tcl_Encoding encoding;

		/*
		 * Set up the encoding state based on the format and whether
		 * this is the first and/or last chunk.
		 */

		encodingCvtFlags = 0;
		if (incrPtr->converts[i].offset == 0) {
		    encodingCvtFlags |= TCL_ENCODING_START;
		}
		if (numItems < TK_SEL_BYTES_AT_ONCE) {
		    encodingCvtFlags |= TCL_ENCODING_END;
		}
		if (formatType == XA_STRING) {
		    encoding = Tcl_GetEncoding(NULL, "iso8859-1");
		} else if (dispPtr && formatType==dispPtr->utf8Atom) {
		    encoding = Tcl_GetEncoding(NULL, "utf-8");
		} else {
		    encoding = Tcl_GetEncoding(NULL, "iso2022");
		}

		/*
		 * Now convert the data.
		 */

		src = (char *)buffer;
		srcLen = numItems;
		Tcl_DStringInit(&ds);
		dst = Tcl_DStringValue(&ds);
		dstLen = ds.spaceAvl - 1;


		/*
		 * Now convert the data, growing the destination buffer as
		 * needed.
		 */

		while (1) {
		    result = Tcl_UtfToExternal(NULL, encoding, src, srcLen,
			    encodingCvtFlags, &incrPtr->converts[i].state,
			    dst, dstLen, &srcRead, &dstWrote, NULL);
		    soFar = dst + dstWrote - Tcl_DStringValue(&ds);
		    encodingCvtFlags &= ~TCL_ENCODING_START;
		    src += srcRead;
		    srcLen -= srcRead;
		    if (result != TCL_CONVERT_NOSPACE) {
			Tcl_DStringSetLength(&ds, soFar);
			break;
		    }
		    if (Tcl_DStringLength(&ds) == 0) {
			Tcl_DStringSetLength(&ds, dstLen);
		    }
		    Tcl_DStringSetLength(&ds, 2 * Tcl_DStringLength(&ds) + 1);
		    dst = Tcl_DStringValue(&ds) + soFar;
		    dstLen = Tcl_DStringLength(&ds) - soFar - 1;
		}
		Tcl_DStringSetLength(&ds, soFar);

		if (encoding) {
		    Tcl_FreeEncoding(encoding);
		}

		/*
		 * Set the property to the encoded string value.
		 */

		XChangeProperty(eventPtr->xproperty.display,
			eventPtr->xproperty.window, eventPtr->xproperty.atom,
			formatType, 8, PropModeReplace,
			(unsigned char *) Tcl_DStringValue(&ds),
			Tcl_DStringLength(&ds));

		/*
		 * Preserve any left-over bytes.
		 */

		if (srcLen > 3) {
		    Tcl_Panic("selection conversion left too many bytes unconverted");
		}
		memcpy(incrPtr->converts[i].buffer, src, (size_t) srcLen+1);
		Tcl_DStringFree(&ds);
	    } else {
		/*
		 * Set the property to the encoded string value.
		 */

		char *propPtr = (char *) SelCvtToX((char *) buffer,
			formatType, (Tk_Window) incrPtr->winPtr, &numItems);

		if (propPtr == NULL) {
		    numItems = 0;
		}
		XChangeProperty(eventPtr->xproperty.display,
			eventPtr->xproperty.window, eventPtr->xproperty.atom,
			formatType, 32, PropModeReplace,
			(unsigned char *) propPtr, numItems);
		if (propPtr != NULL) {
		    ckfree(propPtr);
		}
	    }
	    Tk_DeleteErrorHandler(errorHandler);

	    /*
	     * Compute the next offset value. If this was the last chunk, then
	     * set the offset to -2. If this was an empty chunk, then set the
	     * offset to -1 to indicate we are done.
	     */

	    if (numItems < TK_SEL_BYTES_AT_ONCE) {
		if (numItems <= 0) {
		    incrPtr->converts[i].offset = -1;
		    incrPtr->numIncrs--;
		} else {
		    incrPtr->converts[i].offset = -2;
		}
	    } else {
		/*
		 * Advance over the selection data that was consumed this
		 * time.
		 */

		incrPtr->converts[i].offset += numItems - length;
	    }
	    return;
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkSelEventProc --
 *
 *	This function is invoked whenever a selection-related event occurs.
 *	It does the lion's share of the work in implementing the selection
 *	protocol.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lots: depends on the type of event.
 *
 *--------------------------------------------------------------
 */

void
TkSelEventProc(
    Tk_Window tkwin,		/* Window for which event was targeted. */
    register XEvent *eventPtr)	/* X event: either SelectionClear,
				 * SelectionRequest, or SelectionNotify. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    Tcl_Interp *interp;

    /*
     * Case #1: SelectionClear events.
     */

    if (eventPtr->type == SelectionClear) {
	TkSelClearSelection(tkwin, eventPtr);
    }

    /*
     * Case #2: SelectionNotify events. Call the relevant function to handle
     * the incoming selection.
     */

    if (eventPtr->type == SelectionNotify) {
	register TkSelRetrievalInfo *retrPtr;
	char *propInfo, **propInfoPtr = &propInfo;
	Atom type;
	int format, result;
	unsigned long numItems, bytesAfter;
	Tcl_DString ds;

	for (retrPtr = pendingRetrievals; ; retrPtr = retrPtr->nextPtr) {
	    if (retrPtr == NULL) {
		return;
	    }
	    if ((retrPtr->winPtr == winPtr)
		    && (retrPtr->selection == eventPtr->xselection.selection)
		    && (retrPtr->target == eventPtr->xselection.target)
		    && (retrPtr->result == -1)) {
		if (retrPtr->property == eventPtr->xselection.property) {
		    break;
		}
		if (eventPtr->xselection.property == None) {
		    Tcl_SetObjResult(retrPtr->interp, Tcl_ObjPrintf(
			    "%s selection doesn't exist or form \"%s\" not defined",
			    Tk_GetAtomName(tkwin, retrPtr->selection),
			    Tk_GetAtomName(tkwin, retrPtr->target)));
		    Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION",
			    "NONE", NULL);
		    retrPtr->result = TCL_ERROR;
		    return;
		}
	    }
	}

	propInfo = NULL;
	result = XGetWindowProperty(eventPtr->xselection.display,
		eventPtr->xselection.requestor, retrPtr->property,
		0, MAX_PROP_WORDS, False, (Atom) AnyPropertyType,
		&type, &format, &numItems, &bytesAfter,
		(unsigned char **) propInfoPtr);
	if ((result != Success) || (type == None)) {
	    return;
	}
	if (bytesAfter != 0) {
	    Tcl_SetObjResult(retrPtr->interp, Tcl_NewStringObj(
		    "selection property too large", -1));
	    Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "SIZE",NULL);
	    retrPtr->result = TCL_ERROR;
	    XFree(propInfo);
	    return;
	}
	if ((type == XA_STRING) || (type == dispPtr->textAtom)
		|| (type == dispPtr->compoundTextAtom)) {
	    Tcl_Encoding encoding;

	    if (format != 8) {
		Tcl_SetObjResult(retrPtr->interp, Tcl_ObjPrintf(
			"bad format for string selection: wanted \"8\", got \"%d\"",
			format));
		Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "FORMAT",
			NULL);
		retrPtr->result = TCL_ERROR;
		return;
	    }
	    interp = retrPtr->interp;
	    Tcl_Preserve(interp);

	    /*
	     * Convert the X selection data into UTF before passing it to the
	     * selection callback. Note that the COMPOUND_TEXT uses a modified
	     * iso2022 encoding, not the current system encoding. For now
	     * we'll just blindly apply the iso2022 encoding. This is probably
	     * wrong, but it's a placeholder until we figure out what we're
	     * really supposed to do. For STRING, we need to use Latin-1
	     * instead. Again, it's not really the full iso8859-1 space, but
	     * this is close enough.
	     */

	    if (type == dispPtr->compoundTextAtom) {
		encoding = Tcl_GetEncoding(NULL, "iso2022");
	    } else {
		encoding = Tcl_GetEncoding(NULL, "iso8859-1");
	    }
	    Tcl_ExternalToUtfDString(encoding, propInfo, (int)numItems, &ds);
	    if (encoding) {
		Tcl_FreeEncoding(encoding);
	    }

	    retrPtr->result = retrPtr->proc(retrPtr->clientData, interp,
		    Tcl_DStringValue(&ds));
	    Tcl_DStringFree(&ds);
	    Tcl_Release(interp);
	} else if (type == dispPtr->utf8Atom) {
	    /*
	     * The X selection data is in UTF-8 format already. We can't
	     * guarantee that propInfo is NULL-terminated, so we might have to
	     * copy the string.
	     */

	    char *propData = propInfo;

	    if (format != 8) {
		Tcl_SetObjResult(retrPtr->interp, Tcl_ObjPrintf(
			"bad format for string selection: wanted \"8\", got \"%d\"",
			format));
		Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "FORMAT",
			NULL);
		retrPtr->result = TCL_ERROR;
		return;
	    }

	    if (propInfo[numItems] != '\0') {
		propData = ckalloc(numItems + 1);
		strcpy(propData, propInfo);
		propData[numItems] = '\0';
	    }
	    retrPtr->result = retrPtr->proc(retrPtr->clientData,
		    retrPtr->interp, propData);
	    if (propData != propInfo) {
		ckfree(propData);
	    }

	} else if (type == dispPtr->incrAtom) {
	    /*
	     * It's a !?#@!?!! INCR-style reception. Arrange to receive the
	     * selection in pieces, using the ICCCM protocol, then hang around
	     * until either the selection is all here or a timeout occurs.
	     */

	    retrPtr->idleTime = 0;
	    Tk_CreateEventHandler(tkwin, PropertyChangeMask, SelRcvIncrProc,
		    retrPtr);
	    XDeleteProperty(Tk_Display(tkwin), Tk_WindowId(tkwin),
		    retrPtr->property);
	    while (retrPtr->result == -1) {
		Tcl_DoOneEvent(0);
	    }
	    Tk_DeleteEventHandler(tkwin, PropertyChangeMask, SelRcvIncrProc,
		    retrPtr);
	} else {
	    Tcl_DString ds;

	    if (format != 32 && format != 8) {
		Tcl_SetObjResult(retrPtr->interp, Tcl_ObjPrintf(
			"bad format for selection: wanted \"32\" or "
			"\"8\", got \"%d\"", format));
		Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "FORMAT",
			NULL);
		retrPtr->result = TCL_ERROR;
		return;
	    }
	    Tcl_DStringInit(&ds);
	    if (format == 32) {
		SelCvtFromX32((long *) propInfo, (int) numItems, type,
			(Tk_Window) winPtr, &ds);
	    } else {
		SelCvtFromX8((char *) propInfo, (int) numItems, type,
			(Tk_Window) winPtr, &ds);
	    }
	    interp = retrPtr->interp;
	    Tcl_Preserve(interp);
	    retrPtr->result = retrPtr->proc(retrPtr->clientData,
		    interp, Tcl_DStringValue(&ds));
	    Tcl_Release(interp);
	    Tcl_DStringFree(&ds);
	}
	XFree(propInfo);
	return;
    }

    /*
     * Case #3: SelectionRequest events. Call ConvertSelection to do the dirty
     * work.
     */

    if (eventPtr->type == SelectionRequest) {
	ConvertSelection(winPtr, &eventPtr->xselectionrequest);
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SelTimeoutProc --
 *
 *	This function is invoked once every second while waiting for the
 *	selection to be returned. After a while it gives up and aborts the
 *	selection retrieval.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new timer callback is created to call us again in another second,
 *	unless time has expired, in which case an error is recorded for the
 *	retrieval.
 *
 *----------------------------------------------------------------------
 */

static void
SelTimeoutProc(
    ClientData clientData)	/* Information about retrieval in progress. */
{
    register TkSelRetrievalInfo *retrPtr = clientData;

    /*
     * Make sure that the retrieval is still in progress. Then see how long
     * it's been since any sort of response was received from the other side.
     */

    if (retrPtr->result != -1) {
	return;
    }
    retrPtr->idleTime++;
    if (retrPtr->idleTime >= 5) {
	/*
	 * Use a careful function to store the error message, because the
	 * result could already be partially filled in with a partial
	 * selection return.
	 */

	Tcl_SetObjResult(retrPtr->interp, Tcl_NewStringObj(
		"selection owner didn't respond", -1));
	Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "IGNORED", NULL);
	retrPtr->result = TCL_ERROR;
    } else {
	retrPtr->timeout = Tcl_CreateTimerHandler(1000, SelTimeoutProc,
		(ClientData) retrPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertSelection --
 *
 *	This function is invoked to handle SelectionRequest events. It
 *	responds to the requests, obeying the ICCCM protocols.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Properties are created for the selection requestor, and a
 *	SelectionNotify event is generated for the selection requestor. In the
 *	event of long selections, this function implements INCR-mode
 *	transfers, using the ICCCM protocol.
 *
 *----------------------------------------------------------------------
 */

static void
ConvertSelection(
    TkWindow *winPtr,		/* Window that received the conversion
				 * request; may not be selection's current
				 * owner, be we set it to the current
				 * owner. */
    register XSelectionRequestEvent *eventPtr)
				/* Event describing request. */
{
	union {
		XSelectionEvent xsel;
		XEvent ev;
	} reply;	/* Used to notify requestor that selection
				 * info is ready. */
    int multiple;		/* Non-zero means a MULTIPLE request is being
				 * handled. */
    IncrInfo incr;		/* State of selection conversion. */
    Atom singleInfo[2];		/* incr.multAtoms points here except for
				 * multiple conversions. */
    unsigned long i;
    Tk_ErrorHandler errorHandler;
    TkSelectionInfo *infoPtr;
    TkSelInProgress ip;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    errorHandler = Tk_CreateErrorHandler(eventPtr->display, -1, -1,-1,
	    (int (*)()) NULL, NULL);

    /*
     * Initialize the reply event.
     */

    reply.xsel.type = SelectionNotify;
    reply.xsel.serial = 0;
    reply.xsel.send_event = True;
    reply.xsel.display = eventPtr->display;
    reply.xsel.requestor = eventPtr->requestor;
    reply.xsel.selection = eventPtr->selection;
    reply.xsel.target = eventPtr->target;
    reply.xsel.property = eventPtr->property;
    if (reply.xsel.property == None) {
	reply.xsel.property = reply.xsel.target;
    }
    reply.xsel.time = eventPtr->time;

    for (infoPtr = winPtr->dispPtr->selectionInfoPtr; infoPtr != NULL;
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->selection == eventPtr->selection) {
	    break;
	}
    }
    if (infoPtr == NULL) {
	goto refuse;
    }
    winPtr = (TkWindow *) infoPtr->owner;

    /*
     * Figure out which kind(s) of conversion to perform. If handling a
     * MULTIPLE conversion, then read the property describing which
     * conversions to perform.
     */

    incr.winPtr = winPtr;
    incr.selection = eventPtr->selection;
    if (eventPtr->target != winPtr->dispPtr->multipleAtom) {
	multiple = 0;
	singleInfo[0] = reply.xsel.target;
	singleInfo[1] = reply.xsel.property;
	incr.multAtoms = singleInfo;
	incr.numConversions = 1;
    } else {
	Atom type, **multAtomsPtr = &incr.multAtoms;
	int format, result;
	unsigned long bytesAfter;

	multiple = 1;
	incr.multAtoms = NULL;
	if (eventPtr->property == None) {
	    goto refuse;
	}
	result = XGetWindowProperty(eventPtr->display, eventPtr->requestor,
		eventPtr->property, 0, MAX_PROP_WORDS, False,
		winPtr->dispPtr->atomPairAtom, &type, &format,
		&incr.numConversions, &bytesAfter,
		(unsigned char **) multAtomsPtr);
	if ((result != Success) || (bytesAfter != 0) || (format != 32)
		|| (type == None)) {
	    if (incr.multAtoms != NULL) {
		XFree((char *) incr.multAtoms);
	    }
	    goto refuse;
	}
	incr.numConversions /= 2;	/* Two atoms per conversion. */
    }

    /*
     * Loop through all of the requested conversions, and either return the
     * entire converted selection, if it can be returned in a single bunch, or
     * return INCR information only (the actual selection will be returned
     * below).
     */

    incr.converts = ckalloc(incr.numConversions * sizeof(ConvertInfo));
    incr.numIncrs = 0;
    for (i = 0; i < incr.numConversions; i++) {
	Atom target, property, type;
	long buffer[TK_SEL_WORDS_AT_ONCE];
	register TkSelHandler *selPtr;
	int numItems, format;
	char *propPtr;

	target = incr.multAtoms[2*i];
	property = incr.multAtoms[2*i + 1];
	incr.converts[i].offset = -1;
	incr.converts[i].buffer[0] = '\0';

	for (selPtr = winPtr->selHandlerList; selPtr != NULL;
		selPtr = selPtr->nextPtr) {
	    if ((selPtr->target == target)
		    && (selPtr->selection == eventPtr->selection)) {
		break;
	    }
	}

	if (selPtr == NULL) {
	    /*
	     * Nobody seems to know about this kind of request. If it's of a
	     * sort that we can handle without any help, do it. Otherwise mark
	     * the request as an errror.
	     */

	    numItems = TkSelDefaultSelection(infoPtr, target, (char *) buffer,
		    TK_SEL_BYTES_AT_ONCE, &type);
	    if (numItems < 0) {
		incr.multAtoms[2*i + 1] = None;
		continue;
	    }
	} else {
	    ip.selPtr = selPtr;
	    ip.nextPtr = TkSelGetInProgress();
	    TkSelSetInProgress(&ip);
	    type = selPtr->format;
	    numItems = selPtr->proc(selPtr->clientData, 0, (char *) buffer,
		    TK_SEL_BYTES_AT_ONCE);
	    TkSelSetInProgress(ip.nextPtr);
	    if ((ip.selPtr == NULL) || (numItems < 0)) {
		incr.multAtoms[2*i + 1] = None;
		continue;
	    }
	    if (numItems > TK_SEL_BYTES_AT_ONCE) {
		Tcl_Panic("selection handler returned too many bytes");
	    }
	    ((char *) buffer)[numItems] = '\0';
	}

	/*
	 * Got the selection; store it back on the requestor's property.
	 */

	if (numItems == TK_SEL_BYTES_AT_ONCE) {
	    /*
	     * Selection is too big to send at once; start an INCR-mode
	     * transfer.
	     */

	    incr.numIncrs++;
	    type = winPtr->dispPtr->incrAtom;
	    buffer[0] = SelectionSize(selPtr);
	    if (buffer[0] == 0) {
		incr.multAtoms[2*i + 1] = None;
		continue;
	    }
	    numItems = 1;
	    propPtr = (char *) buffer;
	    format = 32;
	    incr.converts[i].offset = 0;
	    XChangeProperty(reply.xsel.display, reply.xsel.requestor,
		    property, type, format, PropModeReplace,
		    (unsigned char *) propPtr, numItems);
	} else if (type == winPtr->dispPtr->utf8Atom) {
	    /*
	     * This matches selection requests of type UTF8_STRING, which
	     * allows us to pass our utf-8 information untouched.
	     */

	    XChangeProperty(reply.xsel.display, reply.xsel.requestor,
		    property, type, 8, PropModeReplace,
		    (unsigned char *) buffer, numItems);
	} else if ((type == XA_STRING)
		|| (type == winPtr->dispPtr->compoundTextAtom)) {
	    Tcl_DString ds;
	    Tcl_Encoding encoding;

	    /*
	     * STRING is Latin-1, COMPOUND_TEXT is an iso2022 variant. We need
	     * to convert the selection text into these external forms before
	     * modifying the property.
	     */

	    if (type == XA_STRING) {
		encoding = Tcl_GetEncoding(NULL, "iso8859-1");
	    } else {
		encoding = Tcl_GetEncoding(NULL, "iso2022");
	    }
	    Tcl_UtfToExternalDString(encoding, (char *) buffer, -1, &ds);
	    XChangeProperty(reply.xsel.display, reply.xsel.requestor,
		    property, type, 8, PropModeReplace,
		    (unsigned char *) Tcl_DStringValue(&ds),
		    Tcl_DStringLength(&ds));
	    if (encoding) {
		Tcl_FreeEncoding(encoding);
	    }
	    Tcl_DStringFree(&ds);
	} else {
	    propPtr = (char *) SelCvtToX((char *) buffer,
		    type, (Tk_Window) winPtr, &numItems);
	    if (propPtr == NULL) {
		goto refuse;
	    }
	    format = 32;
	    XChangeProperty(reply.xsel.display, reply.xsel.requestor,
		    property, type, format, PropModeReplace,
		    (unsigned char *) propPtr, numItems);
	    ckfree(propPtr);
	}
    }

    /*
     * Send an event back to the requestor to indicate that the first stage of
     * conversion is complete (everything is done except for long conversions
     * that have to be done in INCR mode).
     */

    if (incr.numIncrs > 0) {
	XSelectInput(reply.xsel.display, reply.xsel.requestor,
		PropertyChangeMask);
	incr.timeout = Tcl_CreateTimerHandler(1000, IncrTimeoutProc, &incr);
	incr.idleTime = 0;
	incr.reqWindow = reply.xsel.requestor;
	incr.time = infoPtr->time;
	incr.nextPtr = tsdPtr->pendingIncrs;
	tsdPtr->pendingIncrs = &incr;
    }
    if (multiple) {
	XChangeProperty(reply.xsel.display, reply.xsel.requestor,
		reply.xsel.property, winPtr->dispPtr->atomPairAtom,
		32, PropModeReplace, (unsigned char *) incr.multAtoms,
		(int) incr.numConversions*2);
    } else {
	/*
	 * Not a MULTIPLE request. The first property in "multAtoms" got set
	 * to None if there was an error in conversion.
	 */

	reply.xsel.property = incr.multAtoms[1];
    }
    XSendEvent(reply.xsel.display, reply.xsel.requestor, False, 0, &reply.ev);
    Tk_DeleteErrorHandler(errorHandler);

    /*
     * Handle any remaining INCR-mode transfers. This all happens in callbacks
     * to TkSelPropProc, so just wait until the number of uncompleted INCR
     * transfers drops to zero.
     */

    if (incr.numIncrs > 0) {
	IncrInfo *incrPtr2;

	while (incr.numIncrs > 0) {
	    Tcl_DoOneEvent(0);
	}
	Tcl_DeleteTimerHandler(incr.timeout);
	errorHandler = Tk_CreateErrorHandler(winPtr->display,
		-1, -1, -1, (int (*)()) NULL, NULL);
	XSelectInput(reply.xsel.display, reply.xsel.requestor, 0L);
	Tk_DeleteErrorHandler(errorHandler);
	if (tsdPtr->pendingIncrs == &incr) {
	    tsdPtr->pendingIncrs = incr.nextPtr;
	} else {
	    for (incrPtr2 = tsdPtr->pendingIncrs; incrPtr2 != NULL;
		    incrPtr2 = incrPtr2->nextPtr) {
		if (incrPtr2->nextPtr == &incr) {
		    incrPtr2->nextPtr = incr.nextPtr;
		    break;
		}
	    }
	}
    }

    /*
     * All done. Cleanup and return.
     */

    ckfree(incr.converts);
    if (multiple) {
	XFree((char *) incr.multAtoms);
    }
    return;

    /*
     * An error occurred. Send back a refusal message.
     */

  refuse:
    reply.xsel.property = None;
    XSendEvent(reply.xsel.display, reply.xsel.requestor, False, 0, &reply.ev);
    Tk_DeleteErrorHandler(errorHandler);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * SelRcvIncrProc --
 *
 *	This function handles the INCR protocol on the receiving side. It is
 *	invoked in response to property changes on the requestor's window
 *	(which hopefully are because a new chunk of the selection arrived).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a new piece of selection has arrived, a function is invoked to deal
 *	with that piece. When the whole selection is here, a flag is left for
 *	the higher-level function that initiated the selection retrieval.
 *
 *----------------------------------------------------------------------
 */

static void
SelRcvIncrProc(
    ClientData clientData,	/* Information about retrieval. */
    register XEvent *eventPtr)	/* X PropertyChange event. */
{
    register TkSelRetrievalInfo *retrPtr = clientData;
    char *propInfo, **propInfoPtr = &propInfo;
    Atom type;
    int format, result;
    unsigned long numItems, bytesAfter;
    Tcl_Interp *interp;

    if ((eventPtr->xproperty.atom != retrPtr->property)
	    || (eventPtr->xproperty.state != PropertyNewValue)
	    || (retrPtr->result != -1)) {
	return;
    }
    propInfo = NULL;
    result = XGetWindowProperty(eventPtr->xproperty.display,
	    eventPtr->xproperty.window, retrPtr->property, 0, MAX_PROP_WORDS,
	    True, (Atom) AnyPropertyType, &type, &format, &numItems,
	    &bytesAfter, (unsigned char **) propInfoPtr);
    if ((result != Success) || (type == None)) {
	return;
    }
    if (bytesAfter != 0) {
	Tcl_SetObjResult(retrPtr->interp, Tcl_NewStringObj(
		"selection property too large", -1));
	Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "SIZE", NULL);
	retrPtr->result = TCL_ERROR;
	goto done;
    }
    if ((type == XA_STRING)
	    || (type == retrPtr->winPtr->dispPtr->textAtom)
	    || (type == retrPtr->winPtr->dispPtr->utf8Atom)
	    || (type == retrPtr->winPtr->dispPtr->compoundTextAtom)) {
	char *dst, *src;
	int srcLen, dstLen, srcRead, dstWrote, soFar;
	Tcl_Encoding encoding;
	Tcl_DString *dstPtr, temp;

	if (format != 8) {
	    Tcl_SetObjResult(retrPtr->interp, Tcl_ObjPrintf(
		    "bad format for string selection: wanted \"8\", got \"%d\"",
		    format));
	    Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "FORMAT",
		    NULL);
	    retrPtr->result = TCL_ERROR;
	    goto done;
	}
	interp = retrPtr->interp;
	Tcl_Preserve(interp);

	if (type == retrPtr->winPtr->dispPtr->compoundTextAtom) {
	    encoding = Tcl_GetEncoding(NULL, "iso2022");
	} else if (type == retrPtr->winPtr->dispPtr->utf8Atom) {
	    encoding = Tcl_GetEncoding(NULL, "utf-8");
	} else {
	    encoding = Tcl_GetEncoding(NULL, "iso8859-1");
	}

	/*
	 * Check to see if there is any data left over from the previous
	 * chunk. If there is, copy the old data and the new data into a new
	 * buffer.
	 */

	Tcl_DStringInit(&temp);
	if (Tcl_DStringLength(&retrPtr->buf) > 0) {
	    Tcl_DStringAppend(&temp, Tcl_DStringValue(&retrPtr->buf),
		    Tcl_DStringLength(&retrPtr->buf));
	    if (numItems > 0) {
		Tcl_DStringAppend(&temp, propInfo, (int)numItems);
	    }
	    src = Tcl_DStringValue(&temp);
	    srcLen = Tcl_DStringLength(&temp);
	} else if (numItems == 0) {
	    /*
	     * There is no new data, so we're done.
	     */

	    retrPtr->result = TCL_OK;
	    Tcl_Release(interp);
	    goto done;
	} else {
	    src = propInfo;
	    srcLen = numItems;
	}

	/*
	 * Set up the destination buffer so we can use as much space as is
	 * available.
	 */

	dstPtr = &retrPtr->buf;
	dst = Tcl_DStringValue(dstPtr);
	dstLen = dstPtr->spaceAvl - 1;

	/*
	 * Now convert the data, growing the destination buffer as needed.
	 */

	while (1) {
	    result = Tcl_ExternalToUtf(NULL, encoding, src, srcLen,
		    retrPtr->encFlags, &retrPtr->encState,
		    dst, dstLen, &srcRead, &dstWrote, NULL);
	    soFar = dst + dstWrote - Tcl_DStringValue(dstPtr);
	    retrPtr->encFlags &= ~TCL_ENCODING_START;
	    src += srcRead;
	    srcLen -= srcRead;
	    if (result != TCL_CONVERT_NOSPACE) {
		Tcl_DStringSetLength(dstPtr, soFar);
		break;
	    }
	    if (Tcl_DStringLength(dstPtr) == 0) {
		Tcl_DStringSetLength(dstPtr, dstLen);
	    }
	    Tcl_DStringSetLength(dstPtr, 2 * Tcl_DStringLength(dstPtr) + 1);
	    dst = Tcl_DStringValue(dstPtr) + soFar;
	    dstLen = Tcl_DStringLength(dstPtr) - soFar - 1;
	}
	Tcl_DStringSetLength(dstPtr, soFar);

	result = retrPtr->proc(retrPtr->clientData, interp,
		Tcl_DStringValue(dstPtr));
	Tcl_Release(interp);

	/*
	 * Copy any unused data into the destination buffer so we can pick it
	 * up next time around.
	 */

	Tcl_DStringSetLength(dstPtr, 0);
	Tcl_DStringAppend(dstPtr, src, srcLen);

	Tcl_DStringFree(&temp);
	if (encoding) {
	    Tcl_FreeEncoding(encoding);
	}
	if (result != TCL_OK) {
	    retrPtr->result = result;
	}
    } else if (numItems == 0) {
	retrPtr->result = TCL_OK;
    } else {
	Tcl_DString ds;

	if (format != 32 && format != 8) {
	    Tcl_SetObjResult(retrPtr->interp, Tcl_ObjPrintf(
		    "bad format for selection: wanted \"32\" or "
		    "\"8\", got \"%d\"", format));
	    Tcl_SetErrorCode(retrPtr->interp, "TK", "SELECTION", "FORMAT",
		    NULL);
	    retrPtr->result = TCL_ERROR;
	    goto done;
	}
	Tcl_DStringInit(&ds);
	if (format == 32) {
	    SelCvtFromX32((long *) propInfo, (int) numItems, type,
		    (Tk_Window) retrPtr->winPtr, &ds);
	} else {
	    SelCvtFromX8((char *) propInfo, (int) numItems, type,
		    (Tk_Window) retrPtr->winPtr, &ds);
	}
	interp = retrPtr->interp;
	Tcl_Preserve(interp);
	result = retrPtr->proc(retrPtr->clientData, interp,
		Tcl_DStringValue(&ds));
	Tcl_Release(interp);
	Tcl_DStringFree(&ds);
	if (result != TCL_OK) {
	    retrPtr->result = result;
	}
    }

  done:
    XFree(propInfo);
    retrPtr->idleTime = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * SelectionSize --
 *
 *	This function is called when the selection is too large to send in a
 *	single buffer; it computes the total length of the selection in bytes.
 *
 * Results:
 *	The return value is the number of bytes in the selection given by
 *	selPtr.
 *
 * Side effects:
 *	The selection is retrieved from its current owner (this is the only
 *	way to compute its size).
 *
 *----------------------------------------------------------------------
 */

static int
SelectionSize(
    TkSelHandler *selPtr)	/* Information about how to retrieve the
				 * selection whose size is wanted. */
{
    char buffer[TK_SEL_BYTES_AT_ONCE+1];
    int size, chunkSize;
    TkSelInProgress ip;

    size = TK_SEL_BYTES_AT_ONCE;
    ip.selPtr = selPtr;
    ip.nextPtr = TkSelGetInProgress();
    TkSelSetInProgress(&ip);

    do {
	chunkSize = selPtr->proc(selPtr->clientData, size, (char *) buffer,
		TK_SEL_BYTES_AT_ONCE);
	if (ip.selPtr == NULL) {
	    size = 0;
	    break;
	}
	size += chunkSize;
    } while (chunkSize == TK_SEL_BYTES_AT_ONCE);

    TkSelSetInProgress(ip.nextPtr);
    return size;
}

/*
 *----------------------------------------------------------------------
 *
 * IncrTimeoutProc --
 *
 *	This function is invoked once a second while sending the selection to
 *	a requestor in INCR mode. After a while it gives up and aborts the
 *	selection operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new timeout gets registered so that this function gets called again
 *	in another second, unless too many seconds have elapsed, in which case
 *	incrPtr is marked as "all done".
 *
 *----------------------------------------------------------------------
 */

static void
IncrTimeoutProc(
    ClientData clientData)	/* Information about INCR-mode selection
				 * retrieval for which we are selection
				 * owner. */
{
    register IncrInfo *incrPtr = clientData;

    incrPtr->idleTime++;
    if (incrPtr->idleTime >= 5) {
	incrPtr->numIncrs = 0;
    } else {
	incrPtr->timeout = Tcl_CreateTimerHandler(1000, IncrTimeoutProc,
		incrPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SelCvtToX --
 *
 *	Given a selection represented as a string (the normal Tcl form),
 *	convert it to the ICCCM-mandated format for X, depending on the type
 *	argument. This function and SelCvtFromX are inverses.
 *
 * Results:
 *	The return value is a malloc'ed buffer holding a value equivalent to
 *	"string", but formatted as for "type". It is the caller's
 *	responsibility to free the string when done with it. The word at
 *	*numLongsPtr is filled in with the number of 32-bit words returned in
 *	the result. If NULL is returned, the input list was not actually a
 *	list.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static long *
SelCvtToX(
    char *string,		/* String representation of selection. */
    Atom type,			/* Atom specifying the X format that is
				 * desired for the selection. Should not be
				 * XA_STRING (if so, don't bother calling this
				 * function at all). */
    Tk_Window tkwin,		/* Window that governs atom conversion. */
    int *numLongsPtr)		/* Number of 32-bit words contained in the
				 * result. */
{
    const char **field;
    int numFields, i;
    long *propPtr;

    /*
     * The string is assumed to consist of fields separated by spaces. The
     * property gets generated by converting each field to an integer number,
     * in one of two ways:
     * 1. If type is XA_ATOM, convert each field to its corresponding atom.
     * 2. If type is anything else, convert each field from an ASCII number to
     *    a 32-bit binary number.
     */

    if (Tcl_SplitList(NULL, string, &numFields, &field) != TCL_OK) {
	return NULL;
    }
    propPtr = ckalloc(numFields * sizeof(long));

    /*
     * Convert the fields one-by-one.
     */

    for (i=0 ; i<numFields ; i++) {
	if (type == XA_ATOM) {
	    propPtr[i] = (long) Tk_InternAtom(tkwin, field[i]);
	} else {
	    char *dummy;

	    /*
	     * If this fails to parse a number, we just plunge on regardless
	     * anyway.
	     */

	    propPtr[i] = strtol(field[i], &dummy, 0);
	}
    }

    /*
     * Release the parsed list.
     */

    ckfree(field);
    *numLongsPtr = i;
    return propPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * SelCvtFromX32, SelCvtFromX8 --
 *
 *	Given an X property value, formatted as a collection of 32-bit or
 *	8-bit values according to "type" and the ICCCM conventions, convert
 *	the value to a string suitable for manipulation by Tcl. These
 *	functions are the inverse of SelCvtToX.
 *
 * Results:
 *	The return value (stored in a Tcl_DString) is the string equivalent of
 *	"property". It is up to the caller to initialize and free the DString.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
SelCvtFromX32(
    register long *propPtr,	/* Property value from X. */
    int numValues,		/* Number of 32-bit values in property. */
    Atom type,			/* Type of property Should not be XA_STRING
				 * (if so, don't bother calling this function
				 * at all). */
    Tk_Window tkwin,		/* Window to use for atom conversion. */
    Tcl_DString *dsPtr)		/* Where to store the converted string. */
{
    /*
     * Convert each long in the property to a string value, which is either
     * the name of an atom (if type is XA_ATOM) or a hexadecimal string. We
     * build the list in a Tcl_DString because this is easier than trying to
     * get the quoting correct ourselves; this is tricky because atoms can
     * contain spaces in their names (encountered when the atoms are really
     * MIME types). [Bug 1353414]
     */

    for ( ; numValues > 0; propPtr++, numValues--) {
	if (type == XA_ATOM) {
	    Tcl_DStringAppendElement(dsPtr,
		    Tk_GetAtomName(tkwin, (Atom) *propPtr));
	} else {
	    char buf[12];

	    sprintf(buf, "0x%x", (unsigned int) *propPtr);
	    Tcl_DStringAppendElement(dsPtr, buf);
	}
    }
    Tcl_DStringAppend(dsPtr, " ", 1);
}

static void
SelCvtFromX8(
    register char *propPtr,	/* Property value from X. */
    int numValues,		/* Number of 8-bit values in property. */
    Atom type,			/* Type of property Should not be XA_STRING
				 * (if so, don't bother calling this function
				 * at all). */
    Tk_Window tkwin,		/* Window to use for atom conversion. */
    Tcl_DString *dsPtr)		/* Where to store the converted string. */
{
    /*
     * Convert each long in the property to a string value, which is a
     * hexadecimal string. We build the list in a Tcl_DString because this is
     * easier than trying to get the quoting correct ourselves.
     */

    for ( ; numValues > 0; propPtr++, numValues--) {
	char buf[12];

	sprintf(buf, "0x%x", (unsigned char) *propPtr);
	Tcl_DStringAppendElement(dsPtr, buf);
    }
    Tcl_DStringAppend(dsPtr, " ", 1);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
