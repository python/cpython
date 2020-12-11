/*
 * tkMacOSXScrollbar.c --
 *
 *	This file implements the Macintosh specific portion of the scrollbar
 *	widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2015 Kevin Walzer/WordTech Commununications LLC.
 * Copyright (c) 2018-2019 Marc Culler
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkScrollbar.h"
#include "tkMacOSXPrivate.h"

/*
 * Minimum slider length, in pixels (designed to make sure that the slider is
 * always easy to grab with the mouse).
 */

#define MIN_SLIDER_LENGTH	18
#define MIN_GAP			4

/*
 * Borrowed from ttkMacOSXTheme.c to provide appropriate scaling.
 */

#ifdef __LP64__
#define RangeToFactor(maximum)	(((double) (INT_MAX >> 1)) / (maximum))
#else
#define RangeToFactor(maximum)	(((double) (LONG_MAX >> 1)) / (maximum))
#endif /* __LP64__ */

/*
 * Apple reversed the scroll direction with the release of OSX 10.7 Lion.
 */

#define SNOW_LEOPARD_STYLE	(NSAppKitVersionNumber < 1138)

/*
 * Declaration of an extended scrollbar structure with Mac specific additions.
 */

typedef struct MacScrollbar {
    TkScrollbar information;	/* Generic scrollbar info. */
    GC troughGC;		/* For drawing trough. */
    GC copyGC;			/* Used for copying from pixmap onto screen. */
    Bool buttonDown;            /* Is the mouse button down? */
    Bool mouseOver;             /* Is the pointer over the scrollbar. */
    HIThemeTrackDrawInfo info;	/* Controls how the scrollbar is drawn. */
} MacScrollbar;

/* Used to initialize a MacScrollbar's info field. */
HIThemeTrackDrawInfo defaultInfo = {
    .version = 0,
    .min = 0.0,
    .max = 100.0,
    .attributes = kThemeTrackShowThumb,
};

/*
 * The class procedure table for the scrollbar widget. All fields except size
 * are left initialized to NULL, which should happen automatically since the
 * variable is declared at this scope.
 */

const Tk_ClassProcs tkpScrollbarProcs = {
    sizeof(Tk_ClassProcs),	/* size */
    NULL,			/* worldChangedProc */
    NULL,			/* createProc */
    NULL			/* modalProc */
};

/*
 * Information on scrollbar layout, metrics, and draw info.
 */

typedef struct ScrollbarMetrics {
    SInt32 width, minThumbHeight;
    int minHeight, topArrowHeight, bottomArrowHeight;
    NSControlSize controlSize;
} ScrollbarMetrics;

static ScrollbarMetrics metrics = {
    /* kThemeScrollBarMedium */
    15, MIN_SLIDER_LENGTH, 26, 14, 14, kControlSizeNormal
};

/*
 * Declarations of static functions defined later in this file:
 */

static void		ScrollbarEventProc(ClientData clientData,
			    XEvent *eventPtr);
static int		ScrollbarEvent(TkScrollbar *scrollPtr,
			    XEvent *eventPtr);
static void		UpdateControlValues(TkScrollbar *scrollPtr);

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
    MacScrollbar *scrollPtr = ckalloc(sizeof(MacScrollbar));

    scrollPtr->troughGC = NULL;
    scrollPtr->copyGC = NULL;
    scrollPtr->info = defaultInfo;
    scrollPtr->buttonDown = false;

    Tk_CreateEventHandler(tkwin,
	    ExposureMask        |
	    StructureNotifyMask |
	    FocusChangeMask     |
	    ButtonPressMask     |
	    ButtonReleaseMask   |
	    EnterWindowMask     |
	    LeaveWindowMask     |
	    VisibilityChangeMask,
	    ScrollbarEventProc, scrollPtr);

    return (TkScrollbar *) scrollPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkpDisplayScrollbar --
 *
 *	This procedure redraws the contents of a scrollbar window. It is
 *	invoked as a do-when-idle handler, so it only runs when there's nothing
 *	else for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws a scrollbar on the screen.
 *
 *--------------------------------------------------------------
 */

#if MAC_OS_X_VERSION_MAX_ALLOWED > 1080

/*
 * This stand-alone drawing function is used on macOS 10.9 and newer because
 * the HIToolbox does not draw the scrollbar thumb at the expected size on
 * those systems.  The thumb is drawn too large, causing a mouse click on the
 * thumb to be interpreted as a mouse click in the trough.
 */

static void drawMacScrollbar(
    TkScrollbar *scrollPtr,
    MacScrollbar *msPtr,
    CGContextRef context)
{
    MacDrawable *macWin = (MacDrawable *) Tk_WindowId(scrollPtr->tkwin);
    NSView *view = TkMacOSXDrawableView(macWin);
    CGPathRef path;
    CGPoint inner[2], outer[2], thumbOrigin;
    CGSize thumbSize;
    CGRect troughBounds = msPtr->info.bounds;
    troughBounds.origin.y = [view bounds].size.height -
	(troughBounds.origin.y + troughBounds.size.height);
    if (scrollPtr->vertical) {
	thumbOrigin.x = troughBounds.origin.x + MIN_GAP;
	thumbOrigin.y = troughBounds.origin.y + scrollPtr->sliderFirst;
	thumbSize.width = troughBounds.size.width - 2*MIN_GAP + 1;
	thumbSize.height = scrollPtr->sliderLast - scrollPtr->sliderFirst;
	inner[0] = troughBounds.origin;
	inner[1] = CGPointMake(inner[0].x,
			       inner[0].y + troughBounds.size.height);
	outer[0] = CGPointMake(inner[0].x + troughBounds.size.width - 1,
			       inner[0].y);
	outer[1] = CGPointMake(outer[0].x, inner[1].y);
    } else {
	thumbOrigin.x = troughBounds.origin.x + scrollPtr->sliderFirst;
	thumbOrigin.y = troughBounds.origin.y + MIN_GAP;
	thumbSize.width = scrollPtr->sliderLast - scrollPtr->sliderFirst;
	thumbSize.height = troughBounds.size.height - 2*MIN_GAP + 1;
	inner[0] = troughBounds.origin;
	inner[1] = CGPointMake(inner[0].x + troughBounds.size.width,
			       inner[0].y + 1);
	outer[0] = CGPointMake(inner[0].x,
			       inner[0].y + troughBounds.size.height);
	outer[1] = CGPointMake(inner[1].x, outer[0].y);
    }
    CGContextSetShouldAntialias(context, false);
    CGContextSetGrayFillColor(context, 250.0 / 255, 1.0);
    CGContextFillRect(context, troughBounds);
    CGContextSetGrayStrokeColor(context, 232.0 / 255, 1.0);
    CGContextStrokeLineSegments(context, inner, 2);
    CGContextSetGrayStrokeColor(context, 238.0 / 255, 1.0);
    CGContextStrokeLineSegments(context, outer, 2);

    /*
     * Do not display the thumb unless scrolling is possible, in accordance
     * with macOS behavior.
     *
     * Native scrollbars and Ttk scrollbars are always 15 pixels wide, but we
     * allow Tk scrollbars to have any width, even if it looks bad. To prevent
     * sporadic assertion errors when drawing skinny thumbs we must make sure
     * the radius is at most half the width.
     */

    if (scrollPtr->firstFraction > 0.0 || scrollPtr->lastFraction < 1.0) {
	CGRect thumbBounds = {thumbOrigin, thumbSize};
	int width = scrollPtr->vertical ? thumbSize.width : thumbSize.height;
	int radius = width >= 8 ? 4 : width >> 1;
	path = CGPathCreateWithRoundedRect(thumbBounds, radius, radius, NULL);
	CGContextBeginPath(context);
	CGContextAddPath(context, path);
	if (msPtr->info.trackInfo.scrollbar.pressState != 0) {
	    CGContextSetGrayFillColor(context, 133.0 / 255, 1.0);
	} else {
	    CGContextSetGrayFillColor(context, 200.0 / 255, 1.0);
	}
	CGContextSetShouldAntialias(context, true);
	CGContextFillPath(context);
	CFRelease(path);
    }
}
#endif

void
TkpDisplayScrollbar(
    ClientData clientData)	/* Information about window. */
{
    register TkScrollbar *scrollPtr = clientData;
    MacScrollbar *msPtr = (MacScrollbar *) scrollPtr;
    register Tk_Window tkwin = scrollPtr->tkwin;
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkMacOSXDrawingContext dc;

    scrollPtr->flags &= ~REDRAW_PENDING;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
	return;
    }

    MacDrawable *macWin = (MacDrawable *) winPtr->window;
    NSView *view = TkMacOSXDrawableView(macWin);

    if ((view == NULL)
	    || (macWin->flags & TK_DO_NOT_DRAW)
	    || !TkMacOSXSetupDrawingContext((Drawable) macWin, NULL, 1, &dc)) {
	return;
    }

    /*
     * Transform NSView coordinates to CoreGraphics coordinates.
     */

    CGFloat viewHeight = [view bounds].size.height;
    CGAffineTransform t = {
	.a = 1, .b = 0,
	.c = 0, .d = -1,
	.tx = 0, .ty = viewHeight
    };

    CGContextConcatCTM(dc.context, t);

    /*
     * Draw a 3D rectangle to provide a base for the native scrollbar.
     */

    if (scrollPtr->highlightWidth != 0) {
    	GC fgGC, bgGC;

    	bgGC = Tk_GCForColor(scrollPtr->highlightBgColorPtr, (Pixmap) macWin);
    	if (scrollPtr->flags & GOT_FOCUS) {
    	    fgGC = Tk_GCForColor(scrollPtr->highlightColorPtr, (Pixmap) macWin);
    	} else {
    	    fgGC = bgGC;
    	}
    	TkpDrawHighlightBorder(tkwin, fgGC, bgGC, scrollPtr->highlightWidth,
    		(Pixmap) macWin);
    }

    Tk_Draw3DRectangle(tkwin, (Pixmap) macWin, scrollPtr->bgBorder,
	    scrollPtr->highlightWidth, scrollPtr->highlightWidth,
	    Tk_Width(tkwin) - 2*scrollPtr->highlightWidth,
	    Tk_Height(tkwin) - 2*scrollPtr->highlightWidth,
	    scrollPtr->borderWidth, scrollPtr->relief);
    Tk_Fill3DRectangle(tkwin, (Pixmap) macWin, scrollPtr->bgBorder,
	    scrollPtr->inset, scrollPtr->inset,
	    Tk_Width(tkwin) - 2*scrollPtr->inset,
	    Tk_Height(tkwin) - 2*scrollPtr->inset, 0, TK_RELIEF_FLAT);

    /*
     * Update values and then draw the native scrollbar over the rectangle.
     */

    UpdateControlValues(scrollPtr);

    if (SNOW_LEOPARD_STYLE) {
	HIThemeDrawTrack(&msPtr->info, 0, dc.context,
			 kHIThemeOrientationInverted);
    } else if ([NSApp macMinorVersion] <= 8) {
	HIThemeDrawTrack(&msPtr->info, 0, dc.context,
			 kHIThemeOrientationNormal);
    } else {
#if MAC_OS_X_VERSION_MAX_ALLOWED > 1080

	/*
	 * Switch back to NSView coordinates and draw a modern scrollbar.
	 */

	CGContextConcatCTM(dc.context, t);
	drawMacScrollbar(scrollPtr, msPtr, dc.context);
#endif
    }
    TkMacOSXRestoreDrawingContext(&dc);
    scrollPtr->flags &= ~REDRAW_PENDING;
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

extern void
TkpComputeScrollbarGeometry(
    register TkScrollbar *scrollPtr)
				/* Scrollbar whose geometry may have
				 * changed. */
{
    /*
     * The code below is borrowed from tkUnixScrlbr.c but has been adjusted to
     * account for some differences between macOS and X11. The Unix scrollbar
     * has an arrow button on each end.  On macOS 10.6 (Snow Leopard) the
     * scrollbars by default have both arrow buttons at the bottom or right.
     * (There is a preferences setting to use the Unix layout, but we are not
     * supporting that!)  On more recent versions of macOS there are no arrow
     * buttons at all. The case of no arrow buttons can be handled as a special
     * case of having both buttons at the end, but where scrollPtr->arrowLength
     * happens to be zero.  To adjust for having both arrows at the same end we
     * shift the scrollbar up by the arrowLength.
     */

    int fieldLength;

    if (scrollPtr->highlightWidth < 0) {
	scrollPtr->highlightWidth = 0;
    }
    scrollPtr->inset = scrollPtr->highlightWidth + scrollPtr->borderWidth;
    if ([NSApp macMinorVersion] == 6) {
	scrollPtr->arrowLength = scrollPtr->width;
    } else {
	scrollPtr->arrowLength = 0;
    }
    fieldLength = (scrollPtr->vertical ? Tk_Height(scrollPtr->tkwin)
	    : Tk_Width(scrollPtr->tkwin))
	    - 2*(scrollPtr->arrowLength + scrollPtr->inset);
    if (fieldLength < 0) {
	fieldLength = 0;
    }
    scrollPtr->sliderFirst = fieldLength*scrollPtr->firstFraction;
    scrollPtr->sliderLast = fieldLength*scrollPtr->lastFraction;

    /*
     * Adjust the slider so that it has at least a minimal size and so there
     * is a small gap on either end which can be used to scroll by one page.
     */

    if (scrollPtr->sliderFirst < MIN_GAP) {
	scrollPtr->sliderFirst = MIN_GAP;
	scrollPtr->sliderLast += MIN_GAP;
    }
    if (scrollPtr->sliderLast > fieldLength - MIN_GAP) {
	scrollPtr->sliderLast = fieldLength - MIN_GAP;
	scrollPtr->sliderFirst -= MIN_GAP;
    }
    if (scrollPtr->sliderFirst > fieldLength - MIN_SLIDER_LENGTH) {
	scrollPtr->sliderFirst = fieldLength - MIN_SLIDER_LENGTH;
    }
    if (scrollPtr->sliderLast < scrollPtr->sliderFirst + MIN_SLIDER_LENGTH) {
	scrollPtr->sliderLast = scrollPtr->sliderFirst + MIN_SLIDER_LENGTH;
    }
    scrollPtr->sliderFirst += -scrollPtr->arrowLength + scrollPtr->inset;
    scrollPtr->sliderLast += scrollPtr->inset;

    /*
     * Register the desired geometry for the window. Leave enough space for the
     * two arrows, if there are any arrows, plus a minimum-size slider, plus
     * border around the whole window, if any. Then arrange for the window to
     * be redisplayed.
     */

    if (scrollPtr->vertical) {
	Tk_GeometryRequest(scrollPtr->tkwin,
		scrollPtr->width + 2*scrollPtr->inset,
		2*(scrollPtr->arrowLength + scrollPtr->borderWidth
		+ scrollPtr->inset) + metrics.minThumbHeight);
    } else {
	Tk_GeometryRequest(scrollPtr->tkwin,
		2*(scrollPtr->arrowLength + scrollPtr->borderWidth
		+ scrollPtr->inset) + metrics.minThumbHeight,
		scrollPtr->width + 2*scrollPtr->inset);
    }
    Tk_SetInternalBorder(scrollPtr->tkwin, scrollPtr->inset);
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
 *	Frees the GCs associated with the scrollbar.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyScrollbar(
    TkScrollbar *scrollPtr)
{
    MacScrollbar *macScrollPtr = (MacScrollbar *) scrollPtr;

    if (macScrollPtr->troughGC != None) {
	Tk_FreeGC(scrollPtr->display, macScrollPtr->troughGC);
    }
    if (macScrollPtr->copyGC != None) {
	Tk_FreeGC(scrollPtr->display, macScrollPtr->copyGC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpConfigureScrollbar --
 *
 *	This procedure is called after the generic code has finished processing
 *	configuration options, in order to configure platform specific options.
 *	There are no such option on the Mac, however.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Currently, none.
 *
 *----------------------------------------------------------------------
 */

void
TkpConfigureScrollbar(
    register TkScrollbar *scrollPtr)
{
    /* empty */
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
    /*
     * The code below is borrowed from tkUnixScrlbr.c and needs no adjustment
     * since it does not involve the arrow buttons.
     */

    int length, width, tmp;
    register const int inset = scrollPtr->inset;

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

    if (x < inset || x >= width - inset ||
	    y < inset || y >= length - inset) {
	return OUTSIDE;
    }

    /*
     * Here we assume that the scrollbar is layed out with both arrow buttons
     * at the bottom (or right).  Except on 10.6, however, the arrows do not
     * actually exist, i.e. the arrowLength is 0.  These are the same
     * assumptions which are being made in TkpComputeScrollbarGeometry.
     */

    if (y < scrollPtr->sliderFirst + scrollPtr->arrowLength) {
	return TOP_GAP;
    }
    if (y < scrollPtr->sliderLast) {
	return SLIDER;
    }
    if (y < length - (2*scrollPtr->arrowLength + inset)) {
	return BOTTOM_GAP;
    }

    /*
     * On systems newer than 10.6 we have already returned.
     */

    if (y < length - (scrollPtr->arrowLength + inset)) {
	return TOP_ARROW;
    }
    return BOTTOM_ARROW;
}

/*
 *--------------------------------------------------------------
 *
 * UpdateControlValues --
 *
 *	This procedure updates the Macintosh scrollbar control to display the
 *	values defined by the Tk scrollbar. This is the key interface to the
 *	Mac-native scrollbar; the Unix bindings drive scrolling in the Tk
 *	window and all the Mac scrollbar has to do is redraw itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Macintosh control is updated.
 *
 *--------------------------------------------------------------
 */

static void
UpdateControlValues(
    TkScrollbar *scrollPtr)		/* Scrollbar data struct. */
{
    MacScrollbar *msPtr = (MacScrollbar *) scrollPtr;
    Tk_Window tkwin = scrollPtr->tkwin;
    MacDrawable *macWin = (MacDrawable *) Tk_WindowId(scrollPtr->tkwin);
    double dViewSize;
    HIRect contrlRect;
    short width, height;

    NSView *view = TkMacOSXDrawableView(macWin);
    CGFloat viewHeight = [view bounds].size.height;
    NSRect frame;

    frame = NSMakeRect(macWin->xOff, macWin->yOff, Tk_Width(tkwin),
	    Tk_Height(tkwin));
    frame = NSInsetRect(frame, scrollPtr->inset, scrollPtr->inset);
    frame.origin.y = viewHeight - (frame.origin.y + frame.size.height);

    contrlRect = NSRectToCGRect(frame);
    msPtr->info.bounds = contrlRect;

    width = contrlRect.size.width;
    height = contrlRect.size.height - scrollPtr->arrowLength;

    /*
     * Ensure we set scrollbar control bounds only once all size adjustments
     * have been computed.
     */

    msPtr->info.bounds = contrlRect;
    if (scrollPtr->vertical) {
	msPtr->info.attributes &= ~kThemeTrackHorizontal;
    } else {
	msPtr->info.attributes |= kThemeTrackHorizontal;
    }

    /*
     * Given the Tk parameters for the fractions of the start and end of the
     * thumb, the following calculation determines the location for the
     * Macintosh thumb. The Aqua scroll control works as follows. The
     * scrollbar's value is the position of the left (or top) side of the view
     * area in the content area being scrolled. The maximum value of the
     * control is therefore the dimension of the content area less the size of
     * the view area.
     */

    double factor = RangeToFactor(100.0);
    dViewSize = (scrollPtr->lastFraction - scrollPtr->firstFraction) * factor;
    msPtr->info.max = factor - dViewSize;
    msPtr->info.trackInfo.scrollbar.viewsize = dViewSize;
    if (scrollPtr->vertical) {
	if (SNOW_LEOPARD_STYLE) {
	    msPtr->info.value = factor * scrollPtr->firstFraction;
	} else {
	    msPtr->info.value = msPtr->info.max -
		    factor * scrollPtr->firstFraction;
	}
    } else {
	msPtr->info.value = factor * scrollPtr->firstFraction;
    }

    if ((scrollPtr->firstFraction <= 0.0 && scrollPtr->lastFraction >= 1.0)
	    || height <= metrics.minHeight) {
    	msPtr->info.enableState = kThemeTrackHideTrack;
    } else {
        msPtr->info.enableState = kThemeTrackActive;
    	msPtr->info.attributes =
		kThemeTrackShowThumb | kThemeTrackThumbRgnIsNotGhost;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarEvent --
 *
 *	This procedure is invoked in response to <ButtonPress>,
 *      <ButtonRelease>, <EnterNotify>, and <LeaveNotify> events.  The
 *      Scrollbar appearance is modified for each event.
 *
 *--------------------------------------------------------------
 */

static int
ScrollbarEvent(
    TkScrollbar *scrollPtr,
    XEvent *eventPtr)
{
    MacScrollbar *msPtr = (MacScrollbar *) scrollPtr;

    /*
     * The pressState does not indicate whether the moused button was pressed
     * at some location in the Scrollbar.  Rather, it indicates that the
     * scrollbar should appear as if it were pressed in that location. The
     * standard Mac behavior is that once the button is pressed inside the
     * Scrollbar the appearance should not change until the button is released,
     * even if the mouse moves outside of the scrollbar.  However, if the mouse
     * lies over the scrollbar but the button is not pressed then the
     * appearance should be the same as if the button had been pressed on the
     * slider, i.e. kThemeThumbPressed.  See the file Appearance.r, or
     * HIToolbox.bridgesupport on 10.14.
     */

    if (eventPtr->type == ButtonPress) {
	msPtr->buttonDown = true;
	UpdateControlValues(scrollPtr);

	int where = TkpScrollbarPosition(scrollPtr,
		eventPtr->xbutton.x, eventPtr->xbutton.y);

	switch (where) {
	case OUTSIDE:
	    msPtr->info.trackInfo.scrollbar.pressState = 0;
	    break;
	case TOP_GAP:
	    msPtr->info.trackInfo.scrollbar.pressState = kThemeTopTrackPressed;
	    break;
	case SLIDER:
	    msPtr->info.trackInfo.scrollbar.pressState = kThemeThumbPressed;
	    break;
	case BOTTOM_GAP:
	    msPtr->info.trackInfo.scrollbar.pressState =
		    kThemeBottomTrackPressed;
	    break;
	case TOP_ARROW:

	    /*
	     * This looks wrong and the docs say it is wrong but it works.
	     */

	    msPtr->info.trackInfo.scrollbar.pressState =
		    kThemeTopInsideArrowPressed;
	    break;
	case BOTTOM_ARROW:
	    msPtr->info.trackInfo.scrollbar.pressState =
		    kThemeBottomOutsideArrowPressed;
	    break;
	}
    }
    if (eventPtr->type == ButtonRelease) {
	msPtr->buttonDown = false;
	if (!msPtr->mouseOver) {
	    msPtr->info.trackInfo.scrollbar.pressState = 0;
	}
    }
    if (eventPtr->type == EnterNotify) {
	msPtr->mouseOver = true;
	if (!msPtr->buttonDown) {
	    msPtr->info.trackInfo.scrollbar.pressState = kThemeThumbPressed;
	}
    }
    if (eventPtr->type == LeaveNotify) {
	msPtr->mouseOver = false;
	if (!msPtr->buttonDown) {
	    msPtr->info.trackInfo.scrollbar.pressState = 0;
	}
    }
    TkScrollbarEventuallyRedraw(scrollPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various events on
 *	scrollbars.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up. When
 *	it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ScrollbarEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkScrollbar *scrollPtr = clientData;

    switch (eventPtr->type) {
    case UnmapNotify:
	TkMacOSXSetScrollbarGrow((TkWindow *) scrollPtr->tkwin, false);
	break;
    case ActivateNotify:
    case DeactivateNotify:
	TkScrollbarEventuallyRedraw(scrollPtr);
	break;
    case ButtonPress:
    case ButtonRelease:
    case EnterNotify:
    case LeaveNotify:
    	ScrollbarEvent(clientData, eventPtr);
	break;
    default:
	TkScrollbarEventProc(clientData, eventPtr);
    }
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
