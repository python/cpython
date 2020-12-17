/*
 * Copyright 2004, Joe English
 *
 * Support routines for scrollable widgets.
 *
 * (This is sort of half-baked; needs some work)
 *
 * Scrollable interface:
 *
 * 	+ 'first' is controlled by [xy]view widget command
 * 	  and other scrolling commands like 'see';
 *      + 'total' depends on widget contents;
 *      + 'last' depends on first, total, and widget size.
 *
 * Choreography (typical usage):
 *
 * 	1. User adjusts scrollbar, scrollbar widget calls its -command
 * 	2. Scrollbar -command invokes the scrollee [xy]view widget method
 * 	3. TtkScrollviewCommand calls TtkScrollTo(), which updates
 * 	   'first' and schedules a redisplay.
 * 	4. Once the scrollee knows 'total' and 'last' (typically in
 * 	   the LayoutProc), call TtkScrolled(h,first,last,total) to
 * 	   synchronize the scrollbar.
 * 	5. The scrollee -[xy]scrollcommand is called (in an idle callback)
 * 	6. Which calls the scrollbar 'set' method and redisplays the scrollbar.
 *
 * If the scrollee has internal scrolling (e.g., a 'see' method),
 * it should TtkScrollTo() directly (step 2).
 *
 * If the widget value changes, it should call TtkScrolled() (step 4).
 * (This usually happens automatically when the widget is redisplayed).
 *
 * If the scrollee's -[xy]scrollcommand changes, it should call
 * TtkScrollbarUpdateRequired, which will invoke step (5) (@@@ Fix this)
 */

#include <tkInt.h>
#include "ttkTheme.h"
#include "ttkWidget.h"

/* Private data:
 */
#define SCROLL_UPDATE_PENDING  (0x1)
#define SCROLL_UPDATE_REQUIRED (0x2)

struct ScrollHandleRec
{
    unsigned 	flags;
    WidgetCore	*corePtr;
    Scrollable	*scrollPtr;
};

/* TtkCreateScrollHandle --
 * 	Initialize scroll handle.
 */
ScrollHandle TtkCreateScrollHandle(WidgetCore *corePtr, Scrollable *scrollPtr)
{
    ScrollHandle h = ckalloc(sizeof(*h));

    h->flags = 0;
    h->corePtr = corePtr;
    h->scrollPtr = scrollPtr;

    scrollPtr->first = 0;
    scrollPtr->last = 1;
    scrollPtr->total = 1;
    return h;
}

/* UpdateScrollbar --
 *	Call the -scrollcommand callback to sync the scrollbar.
 * 	Returns: Whatever the -scrollcommand does.
 */
static int UpdateScrollbar(Tcl_Interp *interp, ScrollHandle h)
{
    Scrollable *s = h->scrollPtr;
    WidgetCore *corePtr = h->corePtr;
    char arg1[TCL_DOUBLE_SPACE + 2];
    char arg2[TCL_DOUBLE_SPACE + 2];
    int code;
    Tcl_DString buf;

    h->flags &= ~SCROLL_UPDATE_REQUIRED;

    if (s->scrollCmd == NULL) {
	return TCL_OK;
    }

    arg1[0] = arg2[0] = ' ';
    Tcl_PrintDouble(interp, (double)s->first / s->total, arg1+1);
    Tcl_PrintDouble(interp, (double)s->last / s->total, arg2+1);
    Tcl_DStringInit(&buf);
    Tcl_DStringAppend(&buf, s->scrollCmd, -1);
    Tcl_DStringAppend(&buf, arg1, -1);
    Tcl_DStringAppend(&buf, arg2, -1);

    Tcl_Preserve(corePtr);
    code = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, TCL_EVAL_GLOBAL);
    Tcl_DStringFree(&buf);
    if (WidgetDestroyed(corePtr)) {
	Tcl_Release(corePtr);
	return TCL_ERROR;
    }
    Tcl_Release(corePtr);

    if (code != TCL_OK && !Tcl_InterpDeleted(interp)) {
	/* Disable the -scrollcommand, add to stack trace:
	 */
	ckfree(s->scrollCmd);
	s->scrollCmd = 0;

	Tcl_AddErrorInfo(interp, /* @@@ "horizontal" / "vertical" */
		"\n    (scrolling command executed by ");
	Tcl_AddErrorInfo(interp, Tk_PathName(h->corePtr->tkwin));
	Tcl_AddErrorInfo(interp, ")");
    }
    return code;
}

/* UpdateScrollbarBG --
 * 	Idle handler to update the scrollbar.
 */
static void UpdateScrollbarBG(ClientData clientData)
{
    ScrollHandle h = (ScrollHandle)clientData;
    Tcl_Interp *interp = h->corePtr->interp;
    int code;

    h->flags &= ~SCROLL_UPDATE_PENDING;
    Tcl_Preserve((ClientData) interp);
    code = UpdateScrollbar(interp, h);
    if (code == TCL_ERROR && !Tcl_InterpDeleted(interp)) {
	Tcl_BackgroundException(interp, code);
    }
    Tcl_Release((ClientData) interp);
}

/* TtkScrolled --
 * 	Update scroll info, schedule scrollbar update.
 */
void TtkScrolled(ScrollHandle h, int first, int last, int total)
{
    Scrollable *s = h->scrollPtr;

    /* Sanity-check inputs:
     */
    if (total <= 0) {
	first = 0;
	last = 1;
	total = 1;
    }

    if (last > total) {
	first -= (last - total);
	if (first < 0) first = 0;
	last = total;
    }

    if (s->first != first || s->last != last || s->total != total
	    || (h->flags & SCROLL_UPDATE_REQUIRED))
    {
	s->first = first;
	s->last = last;
	s->total = total;

	if (!(h->flags & SCROLL_UPDATE_PENDING)) {
	    Tcl_DoWhenIdle(UpdateScrollbarBG, (ClientData)h);
	    h->flags |= SCROLL_UPDATE_PENDING;
	}
    }
}

/* TtkScrollbarUpdateRequired --
 * 	Force a scrollbar update at the next call to TtkScrolled(),
 * 	even if scroll parameters haven't changed (e.g., if
 * 	-yscrollcommand has changed).
 */

void TtkScrollbarUpdateRequired(ScrollHandle h)
{
    h->flags |= SCROLL_UPDATE_REQUIRED;
}

/* TtkScrollviewCommand --
 * 	Widget [xy]view command implementation.
 *
 *  $w [xy]view -- return current view region
 *  $w [xy]view $index -- set topmost item
 *  $w [xy]view moveto $fraction
 *  $w [xy]view scroll $number $what -- scrollbar interface
 */
int TtkScrollviewCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *const objv[], ScrollHandle h)
{
    Scrollable *s = h->scrollPtr;
    int newFirst = s->first;

    if (objc == 2) {
	Tcl_Obj *result[2];
	result[0] = Tcl_NewDoubleObj((double)s->first / s->total);
	result[1] = Tcl_NewDoubleObj((double)s->last / s->total);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, result));
	return TCL_OK;
    } else if (objc == 3) {
	if (Tcl_GetIntFromObj(interp, objv[2], &newFirst) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	double fraction;
	int count;

	switch (Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count)) {
	    case TK_SCROLL_ERROR:
		return TCL_ERROR;
	    case TK_SCROLL_MOVETO:
		newFirst = (int) ((fraction * s->total) + 0.5);
		break;
	    case TK_SCROLL_UNITS:
		newFirst = s->first + count;
		break;
	    case TK_SCROLL_PAGES: {
		int perPage = s->last - s->first;	/* @@@ */
		newFirst = s->first + count * perPage;
		break;
	    }
	}
    }

    TtkScrollTo(h, newFirst);

    return TCL_OK;
}

void TtkScrollTo(ScrollHandle h, int newFirst)
{
    Scrollable *s = h->scrollPtr;

    if (newFirst >= s->total)
	newFirst = s->total - 1;
    if (newFirst > s->first && s->last >= s->total) /* don't scroll past end */
	newFirst = s->first;
    if (newFirst < 0)
	newFirst = 0;

    if (newFirst != s->first) {
	s->first = newFirst;
	TtkRedisplayWidget(h->corePtr);
    }
}

void TtkFreeScrollHandle(ScrollHandle h)
{
    if (h->flags & SCROLL_UPDATE_PENDING) {
	Tcl_CancelIdleCall(UpdateScrollbarBG, (ClientData)h);
    }
    ckfree(h);
}

