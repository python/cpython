/*
 * tkWinClipboard.c --
 *
 *	This file contains functions for managing the clipboard.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"
#include "tkSelect.h"
#include <shlobj.h>    /* for DROPFILES */

static void		UpdateClipboard(HWND hwnd);

/*
 *----------------------------------------------------------------------
 *
 * TkSelGetSelection --
 *
 *	Retrieve the specified selection from another process. For now, only
 *	fetching XA_STRING from CLIPBOARD is supported. Eventually other types
 *	should be allowed.
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
    Tk_GetSelProc *proc,	/* Procedure to call to process the selection,
				 * once it has been retrieved. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    char *data, *destPtr;
    Tcl_DString ds;
    HGLOBAL handle;
    Tcl_Encoding encoding;
    int result, locale, noBackslash = 0;

    if (!OpenClipboard(NULL)) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	        "clipboard cannot be opened, another application grabbed it"));
        Tcl_SetErrorCode(interp, "TK", "CLIPBOARD", "BUSY", NULL);
        return TCL_ERROR;
    }
    if ((selection != Tk_InternAtom(tkwin, "CLIPBOARD"))
	    || (target != XA_STRING)) {
	goto error;
    }

    /*
     * Attempt to get the data in Unicode form if available as this is less
     * work that CF_TEXT.
     */

    result = TCL_ERROR;
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
	handle = GetClipboardData(CF_UNICODETEXT);
	if (!handle) {
	    CloseClipboard();
	    goto error;
	}
	data = GlobalLock(handle);
	Tcl_WinTCharToUtf((LPCTSTR)data, -1, &ds);
	GlobalUnlock(handle);
    } else if (IsClipboardFormatAvailable(CF_TEXT)) {
	/*
	 * Determine the encoding to use to convert this text.
	 */

	if (IsClipboardFormatAvailable(CF_LOCALE)) {
	    handle = GetClipboardData(CF_LOCALE);
	    if (!handle) {
		CloseClipboard();
		goto error;
	    }

	    /*
	     * Get the locale identifier, determine the proper code page to
	     * use, and find the corresponding encoding.
	     */

	    Tcl_DStringInit(&ds);
	    Tcl_DStringAppend(&ds, "cp######", -1);
	    data = GlobalLock(handle);

	    /*
	     * Even though the documentation claims that GetLocaleInfo expects
	     * an LCID, on Windows 9x it really seems to expect a LanguageID.
	     */

	    locale = LANGIDFROMLCID(*((int*)data));
	    GetLocaleInfoA(locale, LOCALE_IDEFAULTANSICODEPAGE,
		    Tcl_DStringValue(&ds)+2, Tcl_DStringLength(&ds)-2);
	    GlobalUnlock(handle);

	    encoding = Tcl_GetEncoding(NULL, Tcl_DStringValue(&ds));
	    Tcl_DStringFree(&ds);
	} else {
	    encoding = NULL;
	}

	/*
	 * Fetch the text and convert it to UTF.
	 */

	handle = GetClipboardData(CF_TEXT);
	if (!handle) {
	    if (encoding) {
		Tcl_FreeEncoding(encoding);
	    }
	    CloseClipboard();
	    goto error;
	}
	data = GlobalLock(handle);
	Tcl_ExternalToUtfDString(encoding, data, -1, &ds);
	GlobalUnlock(handle);
	if (encoding) {
	    Tcl_FreeEncoding(encoding);
	}
    } else if (IsClipboardFormatAvailable(CF_HDROP)) {
	DROPFILES *drop;

	handle = GetClipboardData(CF_HDROP);
	if (!handle) {
	    CloseClipboard();
	    goto error;
	}
	Tcl_DStringInit(&ds);
	drop = (DROPFILES *) GlobalLock(handle);
	if (drop->fWide) {
	    WCHAR *fname = (WCHAR *) ((char *) drop + drop->pFiles);
	    Tcl_DString dsTmp;
	    int count = 0;
	    size_t len;

	    while (*fname != 0) {
		if (count) {
		    Tcl_DStringAppend(&ds, "\n", 1);
		}
		len = wcslen(fname);
		Tcl_WinTCharToUtf((LPCTSTR)fname, len * sizeof(WCHAR), &dsTmp);
		Tcl_DStringAppend(&ds, Tcl_DStringValue(&dsTmp),
			Tcl_DStringLength(&dsTmp));
		Tcl_DStringFree(&dsTmp);
		fname += len + 1;
		count++;
	    }
	    noBackslash = (count > 0);
	}
	GlobalUnlock(handle);
    } else {
	CloseClipboard();
	goto error;
    }

    /*
     * Translate CR/LF to LF.
     */

    data = destPtr = Tcl_DStringValue(&ds);
    while (*data) {
	if (data[0] == '\r' && data[1] == '\n') {
	    data++;
	} else if (noBackslash && data[0] == '\\') {
	    data++;
	    *destPtr++ = '/';
	} else {
	    *destPtr++ = *data++;
	}
    }
    *destPtr = '\0';

    /*
     * Pass the data off to the selection procedure.
     */

    result = proc(clientData, interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    CloseClipboard();
    return result;

  error:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "%s selection doesn't exist or form \"%s\" not defined",
	    Tk_GetAtomName(tkwin, selection), Tk_GetAtomName(tkwin, target)));
    Tcl_SetErrorCode(interp, "TK", "SELECTION", "EXISTS", NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetSelectionOwner --
 *
 *	This function claims ownership of the specified selection. If the
 *	selection is CLIPBOARD, then we empty the system clipboard.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Empties the system clipboard, and claims ownership.
 *
 *----------------------------------------------------------------------
 */

int
XSetSelectionOwner(
    Display *display,
    Atom selection,
    Window owner,
    Time time)
{
    HWND hwnd = owner ? TkWinGetHWND(owner) : NULL;
    Tk_Window tkwin;

    /*
     * This is a gross hack because the Tk_InternAtom interface is broken. It
     * expects a Tk_Window, even though it only needs a Tk_Display.
     */

    tkwin = (Tk_Window) TkGetMainInfoList()->winPtr;

    if (selection == Tk_InternAtom(tkwin, "CLIPBOARD")) {
	/*
	 * Only claim and empty the clipboard if we aren't already the owner
	 * of the clipboard.
	 */

	if (GetClipboardOwner() != hwnd) {
	    UpdateClipboard(hwnd);
	}
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinClipboardRender --
 *
 *	This function supplies the contents of the clipboard in response to a
 *	WM_RENDERFORMAT message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the contents of the clipboard.
 *
 *----------------------------------------------------------------------
 */

void
TkWinClipboardRender(
    TkDisplay *dispPtr,
    UINT format)
{
    TkClipboardTarget *targetPtr;
    TkClipboardBuffer *cbPtr;
    HGLOBAL handle;
    char *buffer, *p, *rawText, *endPtr;
    int length;
    Tcl_DString ds;

    for (targetPtr = dispPtr->clipTargetPtr; targetPtr != NULL;
	    targetPtr = targetPtr->nextPtr) {
	if (targetPtr->type == XA_STRING) {
	    break;
	}
    }

    /*
     * Count the number of newlines so we can add space for them in the
     * resulting string.
     */

    length = 0;
    if (targetPtr != NULL) {
	for (cbPtr = targetPtr->firstBufferPtr; cbPtr != NULL;
		cbPtr = cbPtr->nextPtr) {
	    length += cbPtr->length;
	    for (p = cbPtr->buffer, endPtr = p + cbPtr->length;
		    p < endPtr; p++) {
		if (*p == '\n') {
		    length++;
		}
	    }
	}
    }

    /*
     * Copy the data and change EOL characters.
     */

    buffer = rawText = ckalloc(length + 1);
    if (targetPtr != NULL) {
	for (cbPtr = targetPtr->firstBufferPtr; cbPtr != NULL;
		cbPtr = cbPtr->nextPtr) {
	    for (p = cbPtr->buffer, endPtr = p + cbPtr->length;
		    p < endPtr; p++) {
		if (*p == '\n') {
		    *buffer++ = '\r';
		}
		*buffer++ = *p;
	    }
	}
    }
    *buffer = '\0';

    Tcl_DStringInit(&ds);
    Tcl_WinUtfToTChar(rawText, -1, &ds);
    ckfree(rawText);
    handle = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,
	    Tcl_DStringLength(&ds) + 2);
    if (!handle) {
	Tcl_DStringFree(&ds);
	return;
    }
    buffer = GlobalLock(handle);
    memcpy(buffer, Tcl_DStringValue(&ds),
	    Tcl_DStringLength(&ds) + 2);
    GlobalUnlock(handle);
    Tcl_DStringFree(&ds);
    SetClipboardData(CF_UNICODETEXT, handle);
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelUpdateClipboard --
 *
 *	This function is called to force the clipboard to be updated after new
 *	data is added.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the current contents of the clipboard.
 *
 *----------------------------------------------------------------------
 */

void
TkSelUpdateClipboard(
    TkWindow *winPtr,
    TkClipboardTarget *targetPtr)
{
    HWND hwnd = TkWinGetHWND(winPtr->window);
    UpdateClipboard(hwnd);
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateClipboard --
 *
 *	Take ownership of the clipboard, clear it, and indicate to the system
 *	the supported formats.
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
UpdateClipboard(
    HWND hwnd)
{
    TkWinUpdatingClipboard(TRUE);
    OpenClipboard(hwnd);
    EmptyClipboard();

    SetClipboardData(CF_UNICODETEXT, NULL);
    CloseClipboard();
    TkWinUpdatingClipboard(FALSE);
}

/*
 *--------------------------------------------------------------
 *
 * TkSelEventProc --
 *
 *	This procedure is invoked whenever a selection-related event occurs.
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
    if (eventPtr->type == SelectionClear) {
	TkSelClearSelection(tkwin, eventPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelPropProc --
 *
 *	This procedure is invoked when property-change events occur on windows
 *	not known to the toolkit. This is a stub function under Windows.
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
TkSelPropProc(
    register XEvent *eventPtr)	/* X PropertyChange event. */
{
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
