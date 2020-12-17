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
 * Copyright (c) 2018 Marc Culler
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkScrollbar.h"
#include "tkMacOSXPrivate.h"


#define MIN_SCROLLBAR_VALUE		0

/*
 * Minimum slider length, in pixels (designed to make sure that the slider is
 * always easy to grab with the mouse).
 */

#define MIN_SLIDER_LENGTH	5

/*Borrowed from ttkMacOSXTheme.c to provide appropriate scaling.*/
#ifdef __LP64__
#define RangeToFactor(maximum) (((double) (INT_MAX >> 1)) / (maximum))
#else
#define RangeToFactor(maximum) (((double) (LONG_MAX >> 1)) / (maximum))
#endif /* __LP64__ */

/*
 * Apple reversed the scroll direction with the release of OSX 10.7 Lion.
 */

#define SNOW_LEOPARD_STYLE (NSAppKitVersionNumber < 1138)

/*
 * Declaration of an extended scrollbar structure with Mac specific additions.
 */

typedef struct MacScrollbar {
    TkScrollbar information;	/* Generic scrollbar info. */
    GC troughGC;		/* For drawing trough. */
    GC copyGC;			/* Used for copying from pixmap onto screen. */
    Bool buttonDown;            /* Is the mouse button down? */  
    Bool mouseOver;             /* Is the pointer over the scrollbar. */
    HIThemeTrackDrawInfo info;  /* Controls how the scrollbar is drawn. */
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
    NULL,					/* worldChangedProc */
    NULL,					/* createProc */
    NULL					/* modalProc */
};


/* Information on scrollbar layout, metrics, and draw info.*/
typedef struct ScrollbarMetrics {
    SInt32 width, minThumbHeight;
    int minHeight, topArrowHeight, bottomArrowHeight;
    NSControlSize controlSize;
} ScrollbarMetrics;


static ScrollbarMetrics metrics = {
  15, 54, 26, 14, 14, kControlSizeNormal /* kThemeScrollBarMedium */
};


/*
 * Declarations of static functions defined later in this file:
 */

static void ScrollbarEventProc(ClientData clientData, XEvent *eventPtr);
static int ScrollbarEvent(TkScrollbar *scrollPtr, XEvent *eventPtr);
static void UpdateControlValues(TkScrollbar  *scrollPtr);

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

    MacScrollbar *scrollPtr = (MacScrollbar *)ckalloc(sizeof(MacScrollbar));

    scrollPtr->troughGC = None;
    scrollPtr->copyGC = None;
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
 *	invoked as a do-when-idle handler, so it only runs when there's
 *	nothing else for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws a scrollbar on the screen.
 *
 *--------------------------------------------------------------
 */

void
TkpDisplayScrollbar(
		    ClientData clientData)	/* Information about window. */
{
    register TkScrollbar *scrollPtr = (TkScrollbar *) clientData;
    MacScrollbar *msPtr = (MacScrollbar *) scrollPtr;
    register Tk_Window tkwin = scrollPtr->tkwin;
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkMacOSXDrawingContext dc;

    scrollPtr->flags &= ~REDRAW_PENDING;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
  	return;
    }

    MacDrawable *macWin =  (MacDrawable *) winPtr->window;
    NSView *view = TkMacOSXDrawableView(macWin);
    if (!view ||
	macWin->flags & TK_DO_NOT_DRAW ||
	!TkMacOSXSetupDrawingContext((Drawable) macWin, NULL, 1, &dc)) {
      return;
    }

    CGFloat viewHeight = [view bounds].size.height;
    CGAffineTransform t = { .a = 1, .b = 0, .c = 0, .d = -1, .tx = 0,
			    .ty = viewHeight};
    CGContextConcatCTM(dc.context, t);

    /*Draw a 3D rectangle to provide a base for the native scrollbar.*/
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

    /* Update values and then draw the native scrollbar over the rectangle.*/
    UpdateControlValues(scrollPtr);

    if (SNOW_LEOPARD_STYLE) {
	HIThemeDrawTrack (&(msPtr->info), 0, dc.context, kHIThemeOrientationInverted);
    } else {
	HIThemeDrawTrack (&(msPtr->info), 0, dc.context, kHIThemeOrientationNormal);
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
     * Adjust the slider so that some piece of it is always displayed in the
     * scrollbar and so that it has at least a minimal width (so it can be
     * grabbed with the mouse).
     */

    if (scrollPtr->sliderFirst > fieldLength - MIN_SLIDER_LENGTH) {
       scrollPtr->sliderFirst = fieldLength - MIN_SLIDER_LENGTH;
    }
    if (scrollPtr->sliderFirst < 0) {
       scrollPtr->sliderFirst = 0;
    }
    if (scrollPtr->sliderLast < scrollPtr->sliderFirst + MIN_SLIDER_LENGTH) {
       scrollPtr->sliderLast = scrollPtr->sliderFirst + MIN_SLIDER_LENGTH;
    }
    if (scrollPtr->sliderLast > fieldLength) {
       scrollPtr->sliderLast = fieldLength;
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
    MacScrollbar *macScrollPtr = (MacScrollbar *)scrollPtr;

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
 *	This procedure is called after the generic code has finished
 *	processing configuration options, in order to configure platform
 *	specific options.  There are no such option on the Mac, however.
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
      /* On systems newer than 10.6 we have already returned. */
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
 *	This procedure updates the Macintosh scrollbar control to
 *	display the values defined by the Tk scrollbar. This is the
 *	key interface to the Mac-native  scrollbar; the Unix bindings
 *	drive scrolling in the Tk window and all the Mac scrollbar has
 *	to do is redraw itself.
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
    MacScrollbar *msPtr = (MacScrollbar *)scrollPtr;
    Tk_Window tkwin = scrollPtr->tkwin;
    MacDrawable *macWin = (MacDrawable *) Tk_WindowId(scrollPtr->tkwin);
    double dViewSize;
    HIRect  contrlRect;
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
    height = contrlRect.size.height;

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

    double maximum = 100,  factor;
    factor = RangeToFactor(maximum);
    dViewSize = (scrollPtr->lastFraction - scrollPtr->firstFraction)
	* factor;
    msPtr->info.max =  MIN_SCROLLBAR_VALUE +
	factor - dViewSize;
    msPtr->info.trackInfo.scrollbar.viewsize = dViewSize;
    if (scrollPtr->vertical) {
      if (SNOW_LEOPARD_STYLE) {
	msPtr->info.value = factor * scrollPtr->firstFraction;
      } else {
	msPtr->info.value = msPtr->info.max - factor * scrollPtr->firstFraction;
      }
    } else {
	 msPtr->info.value =  MIN_SCROLLBAR_VALUE + factor * scrollPtr->firstFraction;
    }

    if((scrollPtr->firstFraction <= 0.0 && scrollPtr->lastFraction >= 1.0)
       || height <= metrics.minHeight) {
    	msPtr->info.enableState = kThemeTrackHideTrack;
    } else {
        msPtr->info.enableState = kThemeTrackActive;
    	msPtr->info.attributes = kThemeTrackShowThumb |  kThemeTrackThumbRgnIsNotGhost;
    }

}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarEvent --
 *
 *	This procedure is invoked in response to <ButtonPress>, <ButtonRelease>,
 *      <EnterNotify>, and <LeaveNotify> events. The Scrollbar appearance is
 *      modified for each event.
 *
 *--------------------------------------------------------------
 */

static int
ScrollbarEvent(TkScrollbar *scrollPtr, XEvent *eventPtr)
{
    MacScrollbar *msPtr = (MacScrollbar *)scrollPtr;
    
    /* The pressState does not indicate whether the moused button was
     * pressed at some location in the Scrollbar.  Rather, it indicates
     * that the scrollbar should appear as if it were pressed in that
     * location. The standard Mac behavior is that once the button is
     * pressed inside the Scrollbar the appearance should not change until
     * the button is released, even if the mouse moves outside of the
     * scrollbar.  However, if the mouse lies over the scrollbar but the
     * button is not pressed then the appearance should be the same as if
     * the button had been pressed on the slider, i.e. kThemeThumbPressed.
     * See the file Appearance.r, or HIToolbox.bridgesupport on 10.14.
     */

    if (eventPtr->type == ButtonPress) {
	msPtr->buttonDown = true;
	UpdateControlValues(scrollPtr);
	int where = TkpScrollbarPosition(scrollPtr,
					 eventPtr->xbutton.x,
					 eventPtr->xbutton.y);
	switch(where) {
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
	    msPtr->info.trackInfo.scrollbar.pressState = kThemeBottomTrackPressed;
	    break;
	case TOP_ARROW:
	    /* This looks wrong and the docs say it is wrong but it works. */
	    msPtr->info.trackInfo.scrollbar.pressState = kThemeTopInsideArrowPressed;
	    break;
	case BOTTOM_ARROW:
	    msPtr->info.trackInfo.scrollbar.pressState = kThemeBottomOutsideArrowPressed;
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
