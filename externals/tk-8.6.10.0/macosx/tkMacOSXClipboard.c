/*
 * tkMacOSXClipboard.c --
 *
 *	This file manages the clipboard for the Tk toolkit.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXConstants.h"
#include "tkSelect.h"

static NSInteger changeCount = -1;
static Tk_Window clipboardOwner = NULL;

#pragma mark TKApplication(TKClipboard)

@implementation TKApplication(TKClipboard)
- (void) tkProvidePasteboard: (TkDisplay *) dispPtr
	pasteboard: (NSPasteboard *) sender
	provideDataForType: (NSString *) type
{
    NSMutableString *string = [NSMutableString new];

    if (dispPtr && dispPtr->clipboardActive &&
	    [type isEqualToString:NSStringPboardType]) {
	for (TkClipboardTarget *targetPtr = dispPtr->clipTargetPtr; targetPtr;
		targetPtr = targetPtr->nextPtr) {
	    if (targetPtr->type == XA_STRING ||
		    targetPtr->type == dispPtr->utf8Atom) {
		for (TkClipboardBuffer *cbPtr = targetPtr->firstBufferPtr;
			cbPtr; cbPtr = cbPtr->nextPtr) {
		    NSString *s = TclUniToNSString(cbPtr->buffer, cbPtr->length);
		    [string appendString:s];
		    [s release];
		}
		break;
	    }
	}
    }
    [sender setString:string forType:type];
    [string release];
}

- (void) tkProvidePasteboard: (TkDisplay *) dispPtr
{
    if (dispPtr && dispPtr->clipboardActive) {
	[self tkProvidePasteboard:dispPtr
		pasteboard:[NSPasteboard generalPasteboard]
		provideDataForType:NSStringPboardType];
    }
}

- (void) pasteboard: (NSPasteboard *) sender
	provideDataForType: (NSString *) type
{
    [self tkProvidePasteboard:TkGetDisplayList() pasteboard:sender
	    provideDataForType:type];
}

- (void) tkCheckPasteboard
{
    if (clipboardOwner && [[NSPasteboard generalPasteboard] changeCount] !=
	    changeCount) {
	TkDisplay *dispPtr = TkGetDisplayList();
	if (dispPtr) {
	    XEvent event;
	    event.xany.type = SelectionClear;
	    event.xany.serial = NextRequest(Tk_Display(clipboardOwner));
	    event.xany.send_event = False;
	    event.xany.window = Tk_WindowId(clipboardOwner);
	    event.xany.display = Tk_Display(clipboardOwner);
	    event.xselectionclear.selection = dispPtr->clipboardAtom;
	    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	}
	clipboardOwner = NULL;
    }
}
@end

#pragma mark -

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
    int result = TCL_ERROR;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    int haveExternalClip =
	    ([[NSPasteboard generalPasteboard] changeCount] != changeCount);

    if (dispPtr && (haveExternalClip || dispPtr->clipboardActive)
	        && selection == dispPtr->clipboardAtom
	        && (target == XA_STRING || target == dispPtr->utf8Atom)) {
	NSString *string = nil;
	NSPasteboard *pb = [NSPasteboard generalPasteboard];
	NSString *type = [pb availableTypeFromArray:[NSArray arrayWithObject:
		NSStringPboardType]];

	if (type) {
	    string = [pb stringForType:type];
	}
	if (string) {

	    /*
	     * Encode the string using the encoding which is used in Tcl
	     * when TCL_UTF_MAX = 3.  This replaces each UTF-16 surrogate with
	     * a 3-byte sequence generated using the UTF-8 algorithm. (Even
	     * though UTF-8 does not allow encoding surrogates, the algorithm
	     * does produce a 3-byte sequence.)
	     */

	    char *bytes = NSStringToTclUni(string, NULL);
	    result = proc(clientData, interp, bytes);
	    if (bytes) {
		ckfree(bytes);
	    }
	}
    } else {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"%s selection doesn't exist or form \"%s\" not defined",
		Tk_GetAtomName(tkwin, selection),
		Tk_GetAtomName(tkwin, target)));
	Tcl_SetErrorCode(interp, "TK", "SELECTION", "EXISTS", NULL);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * XSetSelectionOwner --
 *
 *	This function claims ownership of the specified selection. If the
 *	selection is CLIPBOARD, then we empty the system clipboard.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
XSetSelectionOwner(
    Display *display,		/* X Display. */
    Atom selection,		/* What selection to own. */
    Window owner,		/* Window to be the owner. */
    Time time)			/* The current time? */
{
    TkDisplay *dispPtr = TkGetDisplayList();

    if (dispPtr && selection == dispPtr->clipboardAtom) {
	clipboardOwner = owner ? Tk_IdToWindow(display, owner) : NULL;
	if (!dispPtr->clipboardActive) {
	    NSPasteboard *pb = [NSPasteboard generalPasteboard];

	    changeCount = [pb declareTypes:[NSArray array] owner:NSApp];
	}
    }
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXSelDeadWindow --
 *
 *	This function is invoked just before a TkWindow is deleted. It performs
 *	selection-related cleanup.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	clipboardOwner is cleared.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXSelDeadWindow(
    TkWindow *winPtr)
{
    if (winPtr && winPtr == (TkWindow *)clipboardOwner) {
	clipboardOwner = NULL;
    }
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
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkSelUpdateClipboard(
    TkWindow *winPtr,		/* Window associated with clipboard. */
    TkClipboardTarget *targetPtr)
				/* Info about the content. */
{
    NSPasteboard *pb = [NSPasteboard generalPasteboard];

    changeCount = [pb addTypes:[NSArray arrayWithObject:NSStringPboardType]
	    owner:NSApp];
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
	clipboardOwner = NULL;
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
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
