/*
 * tkWinScrollbar.c --
 *
 *	This file implements the Windows specific portion of the scrollbar
 *	widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"
#include "tkScrollbar.h"

/*
 * The following constant is used to specify the maximum scroll position. This
 * value is limited by the Win32 API to either 16-bits or 32-bits, depending
 * on the context. For now we'll just use a value small enough to fit in
 * 16-bits, but which gives us 4-digits of precision.
 */

#define MAX_SCROLL 10000

/*
 * Declaration of Windows specific scrollbar structure.
 */

typedef struct WinScrollbar {
    TkScrollbar info;		/* Generic scrollbar info. */
    WNDPROC oldProc;		/* Old window procedure. */
    int lastVertical;		/* 1 if was vertical at last refresh. */
    HWND hwnd;			/* Current window handle. */
    int winFlags;		/* Various flags; see below. */
} WinScrollbar;

/*
 * Flag bits for native scrollbars:
 *
 * IN_MODAL_LOOP:		Non-zero means this scrollbar is in the middle
 *				of a modal loop.
 * ALREADY_DEAD:		Non-zero means this scrollbar has been
 *				destroyed, but has not been cleaned up.
 */

#define IN_MODAL_LOOP	1
#define ALREADY_DEAD	2

/*
 * Cached system metrics used to determine scrollbar geometry.
 */

static int initialized = 0;
static int hArrowWidth, hThumb; /* Horizontal control metrics. */
static int vArrowHeight, vThumb; /* Vertical control metrics. */

TCL_DECLARE_MUTEX(winScrlbrMutex)

/*
 * Declarations for functions defined in this file.
 */

static Window		CreateProc(Tk_Window tkwin, Window parent,
			    ClientData instanceData);
static void		ModalLoop(WinScrollbar *, XEvent *eventPtr);
static LRESULT CALLBACK	ScrollbarProc(HWND hwnd, UINT message, WPARAM wParam,
			    LPARAM lParam);
static void		UpdateScrollbar(WinScrollbar *scrollPtr);
static void		UpdateScrollbarMetrics(void);

/*
 * The class procedure table for the scrollbar widget.
 */

const Tk_ClassProcs tkpScrollbarProcs = {
    sizeof(Tk_ClassProcs),	/* size */
    NULL,			/* worldChangedProc */
    CreateProc,			/* createProc */
    NULL 			/* modalProc */
};

static void
WinScrollbarEventProc(ClientData clientData, XEvent *eventPtr)
{
    WinScrollbar *scrollPtr = clientData;

    if (eventPtr->type == ButtonPress) {
	ModalLoop(scrollPtr, eventPtr);
    } else {
	TkScrollbarEventProc(clientData, eventPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * TkpCreateScrollbar --
 *
 *	Allocate a new TkScrollbar structure.
 *
 * Results:
 *	Returns a newly allocated TkScrollbar structure.
 *
 * Side effects:
 *	Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkScrollbar *
TkpCreateScrollbar(
    Tk_Window tkwin)
{
    WinScrollbar *scrollPtr;

    if (!initialized) {
	Tcl_MutexLock(&winScrlbrMutex);
	UpdateScrollbarMetrics();
	initialized = 1;
	Tcl_MutexUnlock(&winScrlbrMutex);
    }

    scrollPtr = ckalloc(sizeof(WinScrollbar));
    scrollPtr->winFlags = 0;
    scrollPtr->hwnd = NULL;

    Tk_CreateEventHandler(tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask|ButtonPressMask,
	    WinScrollbarEventProc, scrollPtr);

    return (TkScrollbar *) scrollPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateScrollbar --
 *
 *	This function updates the position and size of the scrollbar thumb
 *	based on the current settings.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Moves the thumb.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateScrollbar(
    WinScrollbar *scrollPtr)
{
    SCROLLINFO scrollInfo;
    double thumbSize;

    /*
     * Update the current scrollbar position and shape.
     */

    scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.nMin = 0;
    scrollInfo.nMax = MAX_SCROLL;
    thumbSize = (scrollPtr->info.lastFraction - scrollPtr->info.firstFraction);
    scrollInfo.nPage = ((UINT) (thumbSize * (double) MAX_SCROLL)) + 1;
    if (thumbSize < 1.0) {
	scrollInfo.nPos = (int)
		((scrollPtr->info.firstFraction / (1.0-thumbSize))
		* (MAX_SCROLL - (scrollInfo.nPage - 1)));
    } else {
	scrollInfo.nPos = 0;

	/*
	 * Disable the scrollbar when there is nothing to scroll. This is
	 * standard Windows style (see eg Notepad). Also prevents possible
	 * crash on XP+ systems [Bug #624116].
	 */

	scrollInfo.fMask |= SIF_DISABLENOSCROLL;
    }
    SetScrollInfo(scrollPtr->hwnd, SB_CTL, &scrollInfo, TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * CreateProc --
 *
 *	This function creates a new Scrollbar control, subclasses the
 *	instance, and generates a new Window object.
 *
 * Results:
 *	Returns the newly allocated Window object, or None on failure.
 *
 * Side effects:
 *	Causes a new Scrollbar control to come into existence.
 *
 *----------------------------------------------------------------------
 */

static Window
CreateProc(
    Tk_Window tkwin,		/* Token for window. */
    Window parentWin,		/* Parent of new window. */
    ClientData instanceData)	/* Scrollbar instance data. */
{
    DWORD style;
    Window window;
    HWND parent;
    TkWindow *winPtr;
    WinScrollbar *scrollPtr = (WinScrollbar *)instanceData;

    parent = Tk_GetHWND(parentWin);

    if (scrollPtr->info.vertical) {
	style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
		| SBS_VERT;
    } else {
	style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
		| SBS_HORZ;
    }

    scrollPtr->hwnd = CreateWindowW(L"SCROLLBAR", NULL, style,
	    Tk_X(tkwin), Tk_Y(tkwin), Tk_Width(tkwin), Tk_Height(tkwin),
	    parent, NULL, Tk_GetHINSTANCE(), NULL);

    /*
     * Ensure new window is inserted into the stacking order at the correct
     * place.
     */

    SetWindowPos(scrollPtr->hwnd, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

    for (winPtr = ((TkWindow*)tkwin)->nextPtr; winPtr != NULL;
	    winPtr = winPtr->nextPtr) {
	if ((winPtr->window != None) && !(winPtr->flags & TK_TOP_HIERARCHY)) {
	    TkWinSetWindowPos(scrollPtr->hwnd, Tk_GetHWND(winPtr->window),
		    Below);
	    break;
	}
    }

    scrollPtr->lastVertical = scrollPtr->info.vertical;
    scrollPtr->oldProc = (WNDPROC)SetWindowLongPtrW(scrollPtr->hwnd,
	    GWLP_WNDPROC, (LONG_PTR) ScrollbarProc);
    window = Tk_AttachHWND(tkwin, scrollPtr->hwnd);

    UpdateScrollbar(scrollPtr);
    return window;
}

/*
 *--------------------------------------------------------------
 *
 * TkpDisplayScrollbar --
 *
 *	This procedure redraws the contents of a scrollbar window. It is
 *	invoked as a do-when-idle handler, so it only runs when there's
 *	nothing else for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

void
TkpDisplayScrollbar(
    ClientData clientData)	/* Information about window. */
{
    WinScrollbar *scrollPtr = (WinScrollbar *) clientData;
    Tk_Window tkwin = scrollPtr->info.tkwin;

    scrollPtr->info.flags &= ~REDRAW_PENDING;
    if ((tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    /*
     * Destroy and recreate the scrollbar control if the orientation has
     * changed.
     */

    if (scrollPtr->lastVertical != scrollPtr->info.vertical) {
	HWND hwnd = Tk_GetHWND(Tk_WindowId(tkwin));

	SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR) scrollPtr->oldProc);
	DestroyWindow(hwnd);

	CreateProc(tkwin, Tk_WindowId(Tk_Parent(tkwin)),
		(ClientData) scrollPtr);
    } else {
	UpdateScrollbar(scrollPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyScrollbar --
 *
 *	Free data structures associated with the scrollbar control.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Restores the default control state.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyScrollbar(
    TkScrollbar *scrollPtr)
{
    WinScrollbar *winScrollPtr = (WinScrollbar *)scrollPtr;
    HWND hwnd = winScrollPtr->hwnd;

    if (hwnd) {
	SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (INT_PTR) winScrollPtr->oldProc);
	if (winScrollPtr->winFlags & IN_MODAL_LOOP) {
	    ((TkWindow *)scrollPtr->tkwin)->flags |= TK_DONT_DESTROY_WINDOW;
	    SetParent(hwnd, NULL);
	}
    }
    winScrollPtr->winFlags |= ALREADY_DEAD;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateScrollbarMetrics --
 *
 *	This function retrieves the current system metrics for a scrollbar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the geometry cache info for all scrollbars.
 *
 *----------------------------------------------------------------------
 */

void
UpdateScrollbarMetrics(void)
{
    int arrowWidth = GetSystemMetrics(SM_CXVSCROLL);

    hArrowWidth = GetSystemMetrics(SM_CXHSCROLL);
    hThumb = GetSystemMetrics(SM_CXHTHUMB);
    vArrowHeight = GetSystemMetrics(SM_CYVSCROLL);
    vThumb = GetSystemMetrics(SM_CYVTHUMB);

    sprintf(tkDefScrollbarWidth, "%d", arrowWidth);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeScrollbarGeometry --
 *
 *	After changes in a scrollbar's size or configuration, this procedure
 *	recomputes various geometry information used in displaying the
 *	scrollbar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The scrollbar will be displayed differently.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeScrollbarGeometry(
    register TkScrollbar *scrollPtr)
				/* Scrollbar whose geometry may have
				 * changed. */
{
    int fieldLength, minThumbSize;

    /*
     * Windows doesn't use focus rings on scrollbars, but we still perform
     * basic sanity checks to appease backwards compatibility.
     */

    if (scrollPtr->highlightWidth < 0) {
	scrollPtr->highlightWidth = 0;
    }

    if (scrollPtr->vertical) {
	scrollPtr->arrowLength = vArrowHeight;
	fieldLength = Tk_Height(scrollPtr->tkwin);
	minThumbSize = vThumb;
    } else {
	scrollPtr->arrowLength = hArrowWidth;
	fieldLength = Tk_Width(scrollPtr->tkwin);
	minThumbSize = hThumb;
    }
    fieldLength -= 2*scrollPtr->arrowLength;
    if (fieldLength < 0) {
	fieldLength = 0;
    }
    scrollPtr->sliderFirst = (int) ((double)fieldLength
	    * scrollPtr->firstFraction);
    scrollPtr->sliderLast = (int) ((double)fieldLength
	    * scrollPtr->lastFraction);

    /*
     * Adjust the slider so that some piece of it is always displayed in the
     * scrollbar and so that it has at least a minimal width (so it can be
     * grabbed with the mouse).
     */

    if (scrollPtr->sliderFirst > fieldLength) {
	scrollPtr->sliderFirst = fieldLength;
    }
    if (scrollPtr->sliderFirst < 0) {
	scrollPtr->sliderFirst = 0;
    }
    if (scrollPtr->sliderLast < (scrollPtr->sliderFirst
	    + minThumbSize)) {
	scrollPtr->sliderLast = scrollPtr->sliderFirst + minThumbSize;
    }
    if (scrollPtr->sliderLast > fieldLength) {
	scrollPtr->sliderLast = fieldLength;
    }
    scrollPtr->sliderFirst += scrollPtr->arrowLength;
    scrollPtr->sliderLast += scrollPtr->arrowLength;

    /*
     * Register the desired geometry for the window (leave enough space for
     * the two arrows plus a minimum-size slider, plus border around the whole
     * window, if any). Then arrange for the window to be redisplayed.
     */

    if (scrollPtr->vertical) {
	Tk_GeometryRequest(scrollPtr->tkwin,
		scrollPtr->width, 2*scrollPtr->arrowLength + minThumbSize);
    } else {
	Tk_GeometryRequest(scrollPtr->tkwin,
		2*scrollPtr->arrowLength + minThumbSize, scrollPtr->width);
    }
    Tk_SetInternalBorder(scrollPtr->tkwin, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ScrollbarProc --
 *
 *	This function is call by Windows whenever an event occurs on a
 *	scrollbar control created by Tk.
 *
 * Results:
 *	Standard Windows return value.
 *
 * Side effects:
 *	May generate events.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
ScrollbarProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result;
    POINT point;
    WinScrollbar *scrollPtr;
    Tk_Window tkwin = Tk_HWNDToWindow(hwnd);

    if (tkwin == NULL) {
	Tcl_Panic("ScrollbarProc called on an invalid HWND");
    }
    scrollPtr = (WinScrollbar *)((TkWindow*)tkwin)->instanceData;

    switch(message) {
    case WM_HSCROLL:
    case WM_VSCROLL: {
	Tcl_Interp *interp;
	Tcl_DString cmdString;
	int command = LOWORD(wParam);
	int code;

	GetCursorPos(&point);
	Tk_TranslateWinEvent(NULL, WM_MOUSEMOVE, 0,
		MAKELPARAM(point.x, point.y), &result);

	if (command == SB_ENDSCROLL) {
	    return 0;
	}

	/*
	 * Bail out immediately if there isn't a command to invoke.
	 */

	if (scrollPtr->info.commandSize == 0) {
	    Tcl_ServiceAll();
	    return 0;
	}

	Tcl_DStringInit(&cmdString);
	Tcl_DStringAppend(&cmdString, scrollPtr->info.command,
		scrollPtr->info.commandSize);

	if (command == SB_LINELEFT || command == SB_LINERIGHT) {
	    Tcl_DStringAppendElement(&cmdString, "scroll");
	    Tcl_DStringAppendElement(&cmdString,
		    (command == SB_LINELEFT ) ? "-1" : "1");
	    Tcl_DStringAppendElement(&cmdString, "units");
	} else if (command == SB_PAGELEFT || command == SB_PAGERIGHT) {
	    Tcl_DStringAppendElement(&cmdString, "scroll");
	    Tcl_DStringAppendElement(&cmdString,
		    (command == SB_PAGELEFT ) ? "-1" : "1");
	    Tcl_DStringAppendElement(&cmdString, "pages");
	} else {
	    char valueString[TCL_DOUBLE_SPACE];
	    double pos = 0.0;

	    switch (command) {
	    case SB_THUMBPOSITION:
		pos = ((double)HIWORD(wParam)) / MAX_SCROLL;
		break;
	    case SB_THUMBTRACK:
		pos = ((double)HIWORD(wParam)) / MAX_SCROLL;
		break;
	    case SB_TOP:
		pos = 0.0;
		break;
	    case SB_BOTTOM:
		pos = 1.0;
		break;
	    }

	    Tcl_PrintDouble(NULL, pos, valueString);
	    Tcl_DStringAppendElement(&cmdString, "moveto");
	    Tcl_DStringAppendElement(&cmdString, valueString);
	}

	interp = scrollPtr->info.interp;
	code = Tcl_EvalEx(interp, cmdString.string, -1, TCL_EVAL_GLOBAL);
	if (code != TCL_OK && code != TCL_CONTINUE && code != TCL_BREAK) {
	    Tcl_AddErrorInfo(interp, "\n    (scrollbar command)");
	    Tcl_BackgroundException(interp, code);
	}
	Tcl_DStringFree(&cmdString);

	Tcl_ServiceAll();
	return 0;
    }

    default:
	if (Tk_TranslateWinEvent(hwnd, message, wParam, lParam, &result)) {
	    return result;
	}
    }
    return CallWindowProcW(scrollPtr->oldProc, hwnd, message, wParam, lParam);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpConfigureScrollbar --
 *
 *	This procedure is called after the generic code has finished
 *	processing configuration options, in order to configure platform
 *	specific options.
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
TkpConfigureScrollbar(
    register TkScrollbar *scrollPtr)
				/* Information about widget; may or may not
				 * already have values for some fields. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * ModalLoop --
 *
 *	This function is invoked in response to a ButtonPress event.
 *	It resends the event to the Scrollbar window procedure,
 * 	which in turn enters a modal loop.
 *
 *----------------------------------------------------------------------
 */

static void
ModalLoop(
    WinScrollbar *scrollPtr,
    XEvent *eventPtr)
{
    int oldMode;

    if (scrollPtr->hwnd) {
	Tcl_Preserve((ClientData)scrollPtr);
	scrollPtr->winFlags |= IN_MODAL_LOOP;
	oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
	TkWinResendEvent(scrollPtr->oldProc, scrollPtr->hwnd, eventPtr);
	(void) Tcl_SetServiceMode(oldMode);
	scrollPtr->winFlags &= ~IN_MODAL_LOOP;
	if (scrollPtr->hwnd && scrollPtr->winFlags & ALREADY_DEAD) {
	    DestroyWindow(scrollPtr->hwnd);
	}
	Tcl_Release((ClientData)scrollPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpScrollbarPosition --
 *
 *	Determine the scrollbar element corresponding to a given position.
 *
 * Results:
 *	One of TOP_ARROW, TOP_GAP, etc., indicating which element of the
 *	scrollbar covers the position given by (x, y). If (x,y) is outside the
 *	scrollbar entirely, then OUTSIDE is returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkpScrollbarPosition(
    register TkScrollbar *scrollPtr,
				/* Scrollbar widget record. */
    int x, int y)		/* Coordinates within scrollPtr's window. */
{
    int length, width, tmp;

    if (scrollPtr->vertical) {
	length = Tk_Height(scrollPtr->tkwin);
	width = Tk_Width(scrollPtr->tkwin);
    } else {
	tmp = x;
	x = y;
	y = tmp;
	length = Tk_Width(scrollPtr->tkwin);
	width = Tk_Height(scrollPtr->tkwin);
    }

    if ((x < scrollPtr->inset) || (x >= (width - scrollPtr->inset))
	    || (y < scrollPtr->inset) || (y >= (length - scrollPtr->inset))) {
	return OUTSIDE;
    }

    /*
     * All of the calculations in this procedure mirror those in
     * TkpDisplayScrollbar. Be sure to keep the two consistent.
     */

    if (y < (scrollPtr->inset + scrollPtr->arrowLength)) {
	return TOP_ARROW;
    }
    if (y < scrollPtr->sliderFirst) {
	return TOP_GAP;
    }
    if (y < scrollPtr->sliderLast) {
	return SLIDER;
    }
    if (y >= (length - (scrollPtr->arrowLength + scrollPtr->inset))) {
	return BOTTOM_ARROW;
    }
    return BOTTOM_GAP;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
