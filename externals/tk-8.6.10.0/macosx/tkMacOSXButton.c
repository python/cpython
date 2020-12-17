/*
 * tkMacOSXButton.c --
 *
 *	This file implements the Macintosh specific portion of the button
 *	widgets.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 * Copyright (c) 2006-2007 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2007 Revar Desmera.
 * Copyright 2015 Kevin Walzer/WordTech Communications LLC.
 * Copyright 2015 Marc Culler.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tkMacOSXPrivate.h"
#include "tkButton.h"
#include "tkMacOSXFont.h"
#include "tkMacOSXDebug.h"

#define FIRST_DRAW	    2
#define ACTIVE		    4

/*
 * Extra padding used for computing the content size that should
 * be allowed when drawing the HITheme button.
 */

#define HI_PADX 14
#define HI_PADY 1

/*
 * The delay in milliseconds between pulsing default button redraws.
 */
#define PULSE_TIMER_MSECS 62   /* Largest value that didn't look stuttery */

/*
 * Declaration of Mac specific button structure.
 */


typedef struct {
    Tk_3DBorder border;
    int relief;
    int offset;			/* 0 means this is a normal widget. 1 means
				 * it is an image button, so we offset the
				 * image to make the button appear to move
				 * up and down as the relief changes. */
    GC gc;
    int hasImageOrBitmap;
} DrawParams;

typedef struct {
    TkButton info;		/* Generic button info */
    int id;
    int usingControl;
    int useTkText;
    int flags;			/* Initialisation status */
    ThemeButtonKind btnkind;
    HIThemeButtonDrawInfo drawinfo;
    HIThemeButtonDrawInfo lastdrawinfo;
    DrawParams drawParams;
    Tcl_TimerToken defaultPulseHandler;
} MacButton;

/*
 * Forward declarations for procedures defined later in this file:
 */

static void	ButtonBackgroundDrawCB(const HIRect *btnbounds,
		    MacButton *ptr, SInt16 depth, Boolean isColorDev);
static void	ButtonContentDrawCB(const HIRect *bounds,
		    ThemeButtonKind kind,
		    const HIThemeButtonDrawInfo *info, MacButton *ptr,
		    SInt16 depth, Boolean isColorDev);
static void	ButtonEventProc(ClientData clientData,
		    XEvent *eventPtr);
static void	TkMacOSXComputeButtonParams(TkButton *butPtr,
		    ThemeButtonKind *btnkind,
		    HIThemeButtonDrawInfo *drawinfo);
static int	TkMacOSXComputeButtonDrawParams(TkButton *butPtr,
		    DrawParams * dpPtr);
static void	TkMacOSXDrawButton(MacButton *butPtr, GC gc,
		    Pixmap pixmap);
static void	DrawButtonImageAndText(TkButton *butPtr);
static void	PulseDefaultButtonProc(ClientData clientData);

/*
 * The class procedure table for the button widgets.
 */

const Tk_ClassProcs tkpButtonProcs = {
    sizeof(Tk_ClassProcs),	/* size */
    TkButtonWorldChanged,	/* worldChangedProc */
};

static int bCount;

/*
 *----------------------------------------------------------------------
 *
 * TkpButtonSetDefaults --
 *
 *	This procedure is invoked before option tables are created for buttons.
 *	It modifies some of the default values to match the current values
 *	defined for this platform.
 *
 * Results:
 *	Some of the default values in *specPtr are modified.
 *
 * Side effects:
 *	Updates some of.
 *
 *----------------------------------------------------------------------
 */

void
TkpButtonSetDefaults()
{
    /*No-op.*/
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateButton --
 *
 *	Allocate a new TkButton structure.
 *
 * Results:
 *	Returns a newly allocated TkButton structure.
 *
 * Side effects:
 *	Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkButton *
TkpCreateButton(
    Tk_Window tkwin)
{
    MacButton *macButtonPtr = ckalloc(sizeof(MacButton));

    Tk_CreateEventHandler(tkwin, ActivateMask,
	    ButtonEventProc, macButtonPtr);
    macButtonPtr->id = bCount++;
    macButtonPtr->flags = FIRST_DRAW;
    macButtonPtr->btnkind = kThemePushButton;
    macButtonPtr->defaultPulseHandler = NULL;
    bzero(&macButtonPtr->drawinfo, sizeof(macButtonPtr->drawinfo));
    bzero(&macButtonPtr->lastdrawinfo, sizeof(macButtonPtr->lastdrawinfo));

    return (TkButton *) macButtonPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayButton --
 *
 *	This procedure is invoked to display a button widget. It is normally
 *	invoked as an idle handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the button in its current mode. The
 *	REDRAW_PENDING flag is cleared.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayButton(
    ClientData clientData)	/* Information about widget. */
{
    MacButton *macButtonPtr = clientData;
    TkButton *butPtr = clientData;
    Tk_Window tkwin = butPtr->tkwin;
    Pixmap pixmap;
    DrawParams* dpPtr = &macButtonPtr->drawParams;
    int needhighlight = 0;

    if (butPtr->flags & BUTTON_DELETED) {
	return;
    }
    butPtr->flags &= ~REDRAW_PENDING;
    if ((butPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }
    pixmap = (Pixmap) Tk_WindowId(tkwin);

    /*
     * Set up clipping region. Make sure the we are using the port
     * for this button, or we will set the wrong window's clip.
     */

    TkMacOSXSetUpClippingRgn(Tk_WindowId(tkwin));

    if (TkMacOSXComputeButtonDrawParams(butPtr, dpPtr)) {
	macButtonPtr->useTkText = 0;
    } else {
	macButtonPtr->useTkText = 1;
    }
    if (macButtonPtr->useTkText) {
	if (butPtr->type == TYPE_BUTTON) {
	    Tk_Fill3DRectangle(tkwin, pixmap, butPtr->highlightBorder, 0, 0,
		    Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
	} else {
	    Tk_Fill3DRectangle(tkwin, pixmap, butPtr->normalBorder, 0, 0,
		    Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
	}

        /*
	 * Display image or bitmap or text for labels or custom controls.
	 */

	DrawButtonImageAndText(butPtr);
        needhighlight = 1;
    } else {
        /*
	 * Draw the native portion of the buttons.
	 */

        TkMacOSXDrawButton(macButtonPtr, dpPtr->gc, pixmap);

        /*
	 * Ask for the highlight border, if needed.
	 */

        if (butPtr->highlightWidth < 3) {
            needhighlight = 1;
        }
    }

    /*
     * Draw highlight border, if needed.
     */

    if (needhighlight) {
	GC gc = NULL;
        if ((butPtr->flags & GOT_FOCUS) && butPtr->highlightColorPtr) {
	    gc = Tk_GCForColor(butPtr->highlightColorPtr, pixmap);
	} else if (butPtr->type == TYPE_LABEL) {
	    gc = Tk_GCForColor(Tk_3DBorderColor(butPtr->highlightBorder), pixmap);
	}
	if (gc) {
	    TkMacOSXDrawSolidBorder(tkwin, gc, 0, butPtr->highlightWidth);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeButtonGeometry --
 *
 *	After changes in a button's text or bitmap, this procedure recomputes
 *	the button's geometry and passes this information along to the geometry
 *	manager for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The button's window may change size.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeButtonGeometry(
    TkButton *butPtr)		/* Button whose geometry may have changed. */
{
    int width = 0, height = 0, charWidth = 1, haveImage = 0, haveText = 0;
    int txtWidth = 0, txtHeight = 0;
    MacButton *mbPtr = (MacButton *) butPtr;
    Tk_FontMetrics fm;
    char *text = Tcl_GetString(butPtr->textPtr);

    TkMacOSXComputeButtonParams(butPtr, &mbPtr->btnkind, &mbPtr->drawinfo);

    /*
     * If the indicator is on, get its size.
     */

    if (butPtr->indicatorOn) {
	switch (butPtr->type) {
	case TYPE_RADIO_BUTTON:
	    GetThemeMetric(kThemeMetricRadioButtonWidth,
		    (SInt32 *) &butPtr->indicatorDiameter);
	    break;
	case TYPE_CHECK_BUTTON:
	    GetThemeMetric(kThemeMetricCheckBoxWidth,
		    (SInt32 *) &butPtr->indicatorDiameter);
	    break;
	default:
	    break;
	}

	/*
	 * Allow 2px extra space next to the indicator.
	 */

	butPtr->indicatorSpace = butPtr->indicatorDiameter + 2;
    } else {
	butPtr->indicatorSpace = 0;
	butPtr->indicatorDiameter = 0;
    }

    if (butPtr->image != NULL) {
	Tk_SizeOfImage(butPtr->image, &width, &height);
	haveImage = 1;
    } else if (butPtr->bitmap != None) {
	Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
	haveImage = 1;
    }

    if (haveImage == 0 || butPtr->compound != COMPOUND_NONE) {
	Tk_FreeTextLayout(butPtr->textLayout);
	butPtr->textLayout = Tk_ComputeTextLayout(butPtr->tkfont,
		text, -1, butPtr->wrapLength, butPtr->justify, 0,
		&butPtr->textWidth, &butPtr->textHeight);

	txtWidth = butPtr->textWidth + 2*butPtr->padX;
	txtHeight = butPtr->textHeight + 2*butPtr->padY;
	haveText = 1;
    }

    if (haveImage && haveText) { /* Image and Text */
	switch ((enum compound) butPtr->compound) {
	case COMPOUND_TOP:
	case COMPOUND_BOTTOM:
	    /*
	     * Image is above or below text.
	     */

	    height += txtHeight + butPtr->padY;
	    width = (width > txtWidth ? width : txtWidth);
	    break;
	case COMPOUND_LEFT:
	case COMPOUND_RIGHT:
	    /*
	     * Image is left or right of text.
	     */

	    width += txtWidth + 2*butPtr->padX;
	    height = (height > txtHeight ? height : txtHeight);
	    break;
	case COMPOUND_CENTER:
	    /*
	     * Image and text are superimposed.
	     */

	    width = (width > txtWidth ? width : txtWidth);
	    height = (height > txtHeight ? height : txtHeight);
	    break;
	default:
	    break;
	}
	width += butPtr->indicatorSpace;
    } else if (haveImage) { /* Image only */
	width = butPtr->width > 0 ? butPtr->width : width + butPtr->indicatorSpace;
	height = butPtr->height > 0 ? butPtr->height : height;
	if (butPtr->type == TYPE_BUTTON) {
	    /*
	     * Allow room to shift the image.
	     */
	    width += 2;
	    height += 2;
	}
    } else { /* Text only */
        width = txtWidth + butPtr->indicatorSpace;
	height = txtHeight;
	if (butPtr->width > 0) {
	    charWidth = Tk_TextWidth(butPtr->tkfont, "0", 1);
	    width = butPtr->width * charWidth + 2*butPtr->padX;
	}
	if (butPtr->height > 0) {
	    Tk_GetFontMetrics(butPtr->tkfont, &fm);
	    height = butPtr->height * fm.linespace + 2*butPtr->padY;
	}
    }

    /*
     * Now figure out the size of the border decorations for the button.
     */

    if (butPtr->highlightWidth < 0) {
	butPtr->highlightWidth = 0;
    }

    butPtr->inset = butPtr->borderWidth + butPtr->highlightWidth;

    width += butPtr->inset*2;
    height += butPtr->inset*2;
    if ([NSApp macMinorVersion] == 6) {
	width += 12;
    }
    if (mbPtr->btnkind == kThemePushButton) {
        HIRect tmpRect;
    	HIRect contBounds;

	/*
	 * A PushButton has a minimum size.  We make sure that we are not
	 * underestimating the size by requesting the content size of a
	 * Pushbutton whose overall size is our content size expanded by the
	 * standard padding.
	 */

	tmpRect = CGRectMake(0, 0, width + 2*HI_PADX, height + 2*HI_PADY);
        HIThemeGetButtonContentBounds(&tmpRect, &mbPtr->drawinfo, &contBounds);
        if (height < contBounds.size.height) {
	    height = contBounds.size.height;
        }
        if (width < contBounds.size.width) {
	    width = contBounds.size.width;
        }
	height += 2*HI_PADY;
	width += 2*HI_PADX;
    }
    Tk_GeometryRequest(butPtr->tkwin, width, height);
    Tk_SetInternalBorder(butPtr->tkwin, butPtr->inset);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawButtonImageAndText --
 *
 *	Draws the image and text associated with a button or label.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image and text are drawn.
 *
 *----------------------------------------------------------------------
 */
static void
DrawButtonImageAndText(
    TkButton *butPtr)
{

    MacButton *mbPtr = (MacButton *) butPtr;
    Tk_Window tkwin = butPtr->tkwin;
    Pixmap pixmap;
    int haveImage = 0, haveText = 0, pressed = 0;
    int imageWidth = 0, imageHeight = 0;
    int imageXOffset = 0, imageYOffset = 0;
    int textXOffset = 0, textYOffset = 0;
    int width = 0, height = 0;
    int fullWidth = 0, fullHeight = 0;
    DrawParams *dpPtr = &mbPtr->drawParams;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
        return;
    }

    pixmap = (Pixmap) Tk_WindowId(tkwin);

    if (butPtr->image != None) {
        Tk_SizeOfImage(butPtr->image, &width, &height);
        haveImage = 1;
    } else if (butPtr->bitmap != None) {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
        haveImage = 1;
    }

    imageWidth = width;
    imageHeight = height;

    if (mbPtr->drawinfo.state == kThemeStatePressed) {
        pressed = 1;
    }

    haveText = (butPtr->textWidth != 0 && butPtr->textHeight != 0);
    if (haveImage && haveText) { /* Image and Text */
        int x, y;

        switch ((enum compound) butPtr->compound) {
	case COMPOUND_TOP:
	case COMPOUND_BOTTOM:
	    /* Image is above or below text */
	    if (butPtr->compound == COMPOUND_TOP) {
		textYOffset = height + butPtr->padY;
	    } else {
		imageYOffset = butPtr->textHeight + butPtr->padY;
	    }
	    fullHeight = height + butPtr->textHeight + butPtr->padY;
	    fullWidth = (width > butPtr->textWidth ? width : butPtr->textWidth);
	    textXOffset = (fullWidth - butPtr->textWidth)/2;
	    imageXOffset = (fullWidth - width)/2;
	    break;
	case COMPOUND_LEFT:
	case COMPOUND_RIGHT:
	    /*
	     * Image is left or right of text
	     */

	    if (butPtr->compound == COMPOUND_LEFT) {
		textXOffset = width + butPtr->padX;
	    } else {
		imageXOffset = butPtr->textWidth + butPtr->padX;
	    }
	    fullWidth = butPtr->textWidth + butPtr->padX + width;
	    fullHeight = (height > butPtr->textHeight ? height :
		    butPtr->textHeight);
	    textYOffset = (fullHeight - butPtr->textHeight)/2;
	    imageYOffset = (fullHeight - height)/2;
	    break;
	case COMPOUND_CENTER:
	    /*
	     * Image and text are superimposed
	     */

	    fullWidth = (width > butPtr->textWidth ? width :
		    butPtr->textWidth);
	    fullHeight = (height > butPtr->textHeight ? height :
		    butPtr->textHeight);
	    textXOffset = (fullWidth - butPtr->textWidth)/2;
	    imageXOffset = (fullWidth - width)/2;
	    textYOffset = (fullHeight - butPtr->textHeight)/2;
	    imageYOffset = (fullHeight - height)/2;
	    break;
	default:
	    break;
	}

        TkComputeAnchor(butPtr->anchor, tkwin,
                butPtr->padX + butPtr->borderWidth,
                butPtr->padY + butPtr->borderWidth,
                fullWidth + butPtr->indicatorSpace, fullHeight, &x, &y);
	x += butPtr->indicatorSpace;

        if (dpPtr->relief == TK_RELIEF_SUNKEN) {
            x += dpPtr->offset;
            y += dpPtr->offset;
        } else if (dpPtr->relief == TK_RELIEF_RAISED) {
            x -= dpPtr->offset;
            y -= dpPtr->offset;
        }
        if (pressed) {
            x += dpPtr->offset;
            y += dpPtr->offset;
        }
        imageXOffset += x;
        imageYOffset += y;

        if (butPtr->image != NULL) {
	    if ((butPtr->selectImage != NULL) &&
		    (butPtr->flags & SELECTED)) {
		Tk_RedrawImage(butPtr->selectImage, 0, 0,
			width, height, pixmap, imageXOffset, imageYOffset);
	    } else if ((butPtr->tristateImage != NULL) &&
		    (butPtr->flags & TRISTATED)) {
		Tk_RedrawImage(butPtr->tristateImage, 0, 0,
			width, height, pixmap, imageXOffset, imageYOffset);
	    } else {
		Tk_RedrawImage(butPtr->image, 0, 0, width,
			height, pixmap, imageXOffset, imageYOffset);
	    }
        } else {
	    XSetClipOrigin(butPtr->display, dpPtr->gc,
		    imageXOffset, imageYOffset);
	    XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, dpPtr->gc,
		    0, 0, (unsigned int) width, (unsigned int) height,
		    imageXOffset, imageYOffset, 1);
	    XSetClipOrigin(butPtr->display, dpPtr->gc, 0, 0);
        }
	y += 1; /* Tweak to match native buttons. */
        Tk_DrawTextLayout(butPtr->display, pixmap, dpPtr->gc, butPtr->textLayout,
			  x + textXOffset, y + textYOffset, 0, -1);
        Tk_UnderlineTextLayout(butPtr->display, pixmap, dpPtr->gc,
                butPtr->textLayout,
                x + textXOffset, y + textYOffset,
                butPtr->underline);
    } else if (haveImage) { /* Image only */
        int x = 0, y;

	TkComputeAnchor(butPtr->anchor, tkwin,
		butPtr->padX + butPtr->borderWidth,
		butPtr->padY + butPtr->borderWidth,
		width + butPtr->indicatorSpace, height, &x, &y);
        x += butPtr->indicatorSpace;
	if (pressed) {
	    x += dpPtr->offset;
	    y += dpPtr->offset;
	}
	imageXOffset += x;
	imageYOffset += y;

	if (butPtr->image != NULL) {
	    if ((butPtr->selectImage != NULL) &&
		    (butPtr->flags & SELECTED)) {
		Tk_RedrawImage(butPtr->selectImage, 0, 0, width,
			height, pixmap, imageXOffset, imageYOffset);
	    } else if ((butPtr->tristateImage != NULL) &&
		    (butPtr->flags & TRISTATED)) {
		Tk_RedrawImage(butPtr->tristateImage, 0, 0, width,
			height, pixmap, imageXOffset, imageYOffset);
	    } else {
		Tk_RedrawImage(butPtr->image, 0, 0, width, height,
			pixmap, imageXOffset, imageYOffset);
	    }
	} else {
	    XSetClipOrigin(butPtr->display, dpPtr->gc, x, y);
	    XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, dpPtr->gc,
		    0, 0, (unsigned int) width, (unsigned int) height,
		    imageXOffset, imageYOffset, 1);
	    XSetClipOrigin(butPtr->display, dpPtr->gc, 0, 0);
	}
    } else { /* Text only */
        int x, y;

	TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
		butPtr->textWidth + butPtr->indicatorSpace,
		butPtr->textHeight, &x, &y);
	x += butPtr->indicatorSpace;
	y += 1; /* Tweak to match native buttons */
	Tk_DrawTextLayout(butPtr->display, pixmap, dpPtr->gc, butPtr->textLayout,
			  x, y, 0, -1);
    }

    /*
     * If the button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.  If the widget is
     * selected and we use a different background color when selected, must
     * temporarily modify the GC so the stippling is the right color.
     */

    if (mbPtr->useTkText) {
        if ((butPtr->state == STATE_DISABLED)
                && ((butPtr->disabledFg == NULL) || (butPtr->image != NULL))) {
            if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
                    && (butPtr->selectBorder != NULL)) {
                XSetForeground(butPtr->display, butPtr->stippleGC,
                        Tk_3DBorderColor(butPtr->selectBorder)->pixel);
            }
            /*
             * Stipple the whole button if no disabledFg was specified,
             * otherwise restrict stippling only to displayed image
             */
            if (butPtr->disabledFg == NULL) {
                XFillRectangle(butPtr->display, pixmap, butPtr->stippleGC,
                        0, 0, (unsigned) Tk_Width(tkwin),
                        (unsigned) Tk_Height(tkwin));
            } else {
                XFillRectangle(butPtr->display, pixmap, butPtr->stippleGC,
                        imageXOffset, imageYOffset,
                        (unsigned) imageWidth, (unsigned) imageHeight);
            }
            if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
                && (butPtr->selectBorder != NULL)
            ) {
                XSetForeground(butPtr->display, butPtr->stippleGC,
                        Tk_3DBorderColor(butPtr->normalBorder)->pixel);
            }
        }

        /*
         * Draw the border and traversal highlight last.  This way, if the
         * button's contents overflow they'll be covered up by the border.
         */

        if (dpPtr->relief != TK_RELIEF_FLAT) {
	    int inset = butPtr->highlightWidth;

	    Tk_Draw3DRectangle(tkwin, pixmap, dpPtr->border, inset, inset,
		    Tk_Width(tkwin) - 2*inset, Tk_Height(tkwin) - 2*inset,
		    butPtr->borderWidth, dpPtr->relief);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyButton --
 *
 *	Free data structures associated with the button control.
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
TkpDestroyButton(
    TkButton *butPtr)
{
    MacButton *mbPtr = (MacButton *) butPtr; /* Mac button. */

    if (mbPtr->defaultPulseHandler) {
        Tcl_DeleteTimerHandler(mbPtr->defaultPulseHandler);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkMacOSXDrawButton --
 *
 *        This function draws the tk button using Mac controls. In addition,
 *        this code may apply custom colors passed in the TkButton.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *      The control is created, or reinitialised as needed
 *
 *--------------------------------------------------------------
 */

static void
TkMacOSXDrawButton(
    MacButton *mbPtr,    /* Mac button. */
    GC gc,               /* The GC we are drawing into - needed for
                          * the bevel button */
    Pixmap pixmap)       /* The pixmap we are drawing into - needed
                          * for the bevel button */
{
    TkButton *butPtr = (TkButton *) mbPtr;
    TkWindow *winPtr = (TkWindow *) butPtr->tkwin;
    HIRect cntrRect;
    TkMacOSXDrawingContext dc;
    DrawParams *dpPtr = &mbPtr->drawParams;
    int useNewerHITools = 1;

    TkMacOSXComputeButtonParams(butPtr, &mbPtr->btnkind, &mbPtr->drawinfo);

    cntrRect = CGRectMake(winPtr->privatePtr->xOff, winPtr->privatePtr->yOff,
	    Tk_Width(butPtr->tkwin), Tk_Height(butPtr->tkwin));

    cntrRect = CGRectInset(cntrRect, butPtr->inset, butPtr->inset);

    if (useNewerHITools == 1) {
        HIRect contHIRec;
        static HIThemeButtonDrawInfo hiinfo;

        ButtonBackgroundDrawCB(&cntrRect, mbPtr, 32, true);

	if (!TkMacOSXSetupDrawingContext(pixmap, dpPtr->gc, 1, &dc)) {
	    return;
	}

        hiinfo.version = 0;
        hiinfo.state = mbPtr->drawinfo.state;
        hiinfo.kind = mbPtr->btnkind;
        hiinfo.value = mbPtr->drawinfo.value;
        hiinfo.adornment = mbPtr->drawinfo.adornment;
        hiinfo.animation.time.current = CFAbsoluteTimeGetCurrent();
        if (hiinfo.animation.time.start == 0) {
            hiinfo.animation.time.start = hiinfo.animation.time.current;
        }

	/*
	 * To avoid buttons with white text on a white background, we set the
	 * state to inactive in Dark Mode unless the button is pressed or is a
	 * -default active button.  This isn't perfect but it is mostly usable.
	 * Using a ttk::button would be a much better choice, however.
	 */

	if (TkMacOSXInDarkMode(butPtr->tkwin) &&
	    mbPtr->drawinfo.state != kThemeStatePressed &&
	    !(mbPtr->drawinfo.adornment & kThemeAdornmentDefault)) {
	    hiinfo.state = kThemeStateInactive;
	}
	HIThemeDrawButton(&cntrRect, &hiinfo, dc.context,
		kHIThemeOrientationNormal, &contHIRec);

	TkMacOSXRestoreDrawingContext(&dc);
        ButtonContentDrawCB(&contHIRec, mbPtr->btnkind, &mbPtr->drawinfo,
		(MacButton *) mbPtr, 32, true);
    } else {
	if (!TkMacOSXSetupDrawingContext(pixmap, dpPtr->gc, 1, &dc)) {
	    return;
	}

	TkMacOSXRestoreDrawingContext(&dc);
    }
    mbPtr->lastdrawinfo = mbPtr->drawinfo;
}

/*
 *--------------------------------------------------------------
 *
 * ButtonBackgroundDrawCB --
 *
 *        This function draws the background that lies under checkboxes and
 *        radiobuttons.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The background gets updated to the current color.
 *
 *--------------------------------------------------------------
 */

static void
ButtonBackgroundDrawCB(
    const HIRect *btnbounds,
    MacButton *ptr,
    SInt16 depth,
    Boolean isColorDev)
{
    MacButton *mbPtr = (MacButton *) ptr;
    TkButton *butPtr = (TkButton *) mbPtr;
    Tk_Window tkwin = butPtr->tkwin;
    Pixmap pixmap;
    int usehlborder = 0;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
        return;
    }
    pixmap = (Pixmap) Tk_WindowId(tkwin);

    if (butPtr->type != TYPE_LABEL) {
        switch (mbPtr->btnkind) {
	case kThemeSmallBevelButton:
	case kThemeBevelButton:
	case kThemeRoundedBevelButton:
	case kThemePushButton:
	    usehlborder = 1;
	    break;
        }
    }
    if (usehlborder) {
        Tk_Fill3DRectangle(tkwin, pixmap, butPtr->highlightBorder, 0, 0,
            Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
    } else {
        Tk_Fill3DRectangle(tkwin, pixmap, butPtr->normalBorder, 0, 0,
            Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ButtonContentDrawCB --
 *
 *        This function draws the label and image for the button.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The content of the button gets updated.
 *
 *--------------------------------------------------------------
 */
static void
ButtonContentDrawCB (
    const HIRect * btnbounds,
    ThemeButtonKind kind,
    const HIThemeButtonDrawInfo *drawinfo,
    MacButton *ptr,
    SInt16 depth,
    Boolean isColorDev)
{
    TkButton *butPtr = (TkButton *) ptr;
    Tk_Window tkwin = butPtr->tkwin;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
        return;
    }

    /*
     * Overlay Tk elements over button native region: drawing elements within
     * button boundaries/native region causes unpredictable metrics.
     */

    DrawButtonImageAndText( butPtr);
}

/*
 *--------------------------------------------------------------
 *
 * ButtonEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various events on
 *	buttons.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ButtonEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkButton *buttonPtr = clientData;
    MacButton *mbPtr = clientData;

    if (eventPtr->type == ActivateNotify
	    || eventPtr->type == DeactivateNotify) {
	if ((buttonPtr->tkwin == NULL) || (!Tk_IsMapped(buttonPtr->tkwin))) {
	    return;
	}
	if (eventPtr->type == ActivateNotify) {
	    mbPtr->flags |= ACTIVE;
	} else {
	    mbPtr->flags &= ~ACTIVE;
	}
	if ((buttonPtr->flags & REDRAW_PENDING) == 0) {
	    Tcl_DoWhenIdle(TkpDisplayButton, buttonPtr);
	    buttonPtr->flags |= REDRAW_PENDING;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXComputeButtonParams --
 *
 *      This procedure computes the various parameters used when creating a
 *      Carbon Appearance control.  These are determined by the various tk
 *      button parameters
 *
 * Results:
 *	None.
 *
 * Side effects:
 *        Sets the btnkind and drawinfo parameters
 *
 *----------------------------------------------------------------------
 */

static void
TkMacOSXComputeButtonParams(
    TkButton *butPtr,
    ThemeButtonKind *btnkind,
    HIThemeButtonDrawInfo *drawinfo)
{
    MacButton *mbPtr = (MacButton *) butPtr;

    if (butPtr->borderWidth <= 2) {
        *btnkind = kThemeSmallBevelButton;
    } else if (butPtr->borderWidth == 3) {
        *btnkind = kThemeBevelButton;
    } else if (butPtr->borderWidth == 4) {
        *btnkind = kThemeRoundedBevelButton;
    } else {
        *btnkind = kThemePushButton;
    }

    if ((butPtr->image == None) && (butPtr->bitmap == None)) {
        switch (butPtr->type) {
	case TYPE_BUTTON:
	    *btnkind = kThemePushButton;
	    break;
	case TYPE_RADIO_BUTTON:
	    if (butPtr->borderWidth <= 1) {
		*btnkind = kThemeSmallRadioButton;
	    } else {
		*btnkind = kThemeRadioButton;
	    }
	    break;
	case TYPE_CHECK_BUTTON:
	    if (butPtr->borderWidth <= 1) {
		*btnkind = kThemeSmallCheckBox;
	    } else {
		*btnkind = kThemeCheckBox;
	    }
	    break;
	}
    }

    if (butPtr->indicatorOn) {
        switch (butPtr->type) {
	case TYPE_RADIO_BUTTON:
	    if (butPtr->borderWidth <= 1) {
		*btnkind = kThemeSmallRadioButton;
	    } else {
		*btnkind = kThemeRadioButton;
	    }
	    break;
	case TYPE_CHECK_BUTTON:
	    if (butPtr->borderWidth <= 1) {
		*btnkind = kThemeSmallCheckBox;
	    } else {
		*btnkind = kThemeCheckBox;
	    }
	    break;
        }
    } else {
        if (butPtr->type == TYPE_RADIO_BUTTON ||
		butPtr->type == TYPE_CHECK_BUTTON) {
	    if (*btnkind == kThemePushButton) {
		*btnkind = kThemeBevelButton;
	    }
        }
    }

    if (butPtr->flags & SELECTED) {
        drawinfo->value = kThemeButtonOn;
    } else if (butPtr->flags & TRISTATED) {
        drawinfo->value = kThemeButtonMixed;
    } else {
        drawinfo->value = kThemeButtonOff;
    }

    if ((mbPtr->flags & FIRST_DRAW) != 0) {
	mbPtr->flags &= ~FIRST_DRAW;
	if (Tk_MacOSXIsAppInFront()) {
	    mbPtr->flags |= ACTIVE;
	}
    }

    drawinfo->state = kThemeStateInactive;
    if ((mbPtr->flags & ACTIVE) == 0) {
        if (butPtr->state == STATE_DISABLED) {
            drawinfo->state = kThemeStateUnavailableInactive;
        } else {
            drawinfo->state = kThemeStateInactive;
        }
    } else if (butPtr->state == STATE_DISABLED) {
        drawinfo->state = kThemeStateUnavailable;
    } else if (butPtr->state == STATE_ACTIVE) {
        drawinfo->state = kThemeStatePressed;
    } else {
        drawinfo->state = kThemeStateActive;
    }

    drawinfo->adornment = kThemeAdornmentNone;
    if (butPtr->defaultState == DEFAULT_ACTIVE) {
	if (drawinfo->state != kThemeStatePressed) {
	    drawinfo->adornment |= kThemeAdornmentDefault;
	}
        if (!mbPtr->defaultPulseHandler) {
            mbPtr->defaultPulseHandler = Tcl_CreateTimerHandler(
                    PULSE_TIMER_MSECS, PulseDefaultButtonProc, butPtr);
        }
    } else if (mbPtr->defaultPulseHandler) {
        Tcl_DeleteTimerHandler(mbPtr->defaultPulseHandler);
    }
    if (butPtr->highlightWidth >= 3) {
        if ((butPtr->flags & GOT_FOCUS)) {
            drawinfo->adornment |= kThemeAdornmentFocus;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXComputeButtonDrawParams --
 *
 *	This procedure computes the various parameters used when drawing a
 *	button. These are determined by the various tk button parameters
 *
 * Results:
 *	1 if control will be used, 0 otherwise.
 *
 * Side effects:
 *	Sets the button draw parameters
 *
 *----------------------------------------------------------------------
 */

static int
TkMacOSXComputeButtonDrawParams(
    TkButton *butPtr,
    DrawParams *dpPtr)
{
    MacButton *mbPtr = (MacButton *) butPtr;

    dpPtr->hasImageOrBitmap = ((butPtr->image != NULL)
	    || (butPtr->bitmap != None));

    if (butPtr->type != TYPE_LABEL) {
        dpPtr->offset = 0;
        if (dpPtr->hasImageOrBitmap) {
            switch (mbPtr->btnkind) {
	    case kThemeSmallBevelButton:
	    case kThemeBevelButton:
	    case kThemeRoundedBevelButton:
	    case kThemePushButton:
		dpPtr->offset = 1;
		break;
            }
        }
    }

    dpPtr->border = butPtr->normalBorder;
    if ((butPtr->state == STATE_DISABLED) && (butPtr->disabledFg != NULL)) {
	dpPtr->gc = butPtr->disabledGC;
    } else if (butPtr->type == TYPE_BUTTON && butPtr->state == STATE_ACTIVE) {
	dpPtr->gc = butPtr->activeTextGC;
	dpPtr->border = butPtr->activeBorder;
    } else if ((mbPtr->drawinfo.adornment & kThemeAdornmentDefault) &&
	       mbPtr->drawinfo.state == kThemeStateActive) {
	/*
	 * This is a "-default active" button in the front window.
	 */

	dpPtr->gc = butPtr->activeTextGC;
    } else {
	dpPtr->gc = butPtr->normalTextGC;
    }

    if ((butPtr->flags & SELECTED) && (butPtr->state != STATE_ACTIVE)
	    && (butPtr->selectBorder != NULL) && !butPtr->indicatorOn) {
	dpPtr->border = butPtr->selectBorder;
    }

    /*
     * Override the relief specified for the button if this is a checkbutton or
     * radiobutton and there's no indicator.
     */

    dpPtr->relief = butPtr->relief;

    if ((butPtr->type >= TYPE_CHECK_BUTTON) && !butPtr->indicatorOn) {
	if (!dpPtr->hasImageOrBitmap) {
	    dpPtr->relief = (butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN
		    : TK_RELIEF_RAISED;
	}
    }

    if (butPtr->type != TYPE_LABEL && (butPtr->type == TYPE_BUTTON ||
	    butPtr->indicatorOn || dpPtr->hasImageOrBitmap)) {
	/*
	 * Draw this widget as a native control.
	 */

	return 1;
    } else {
	/*
	 * Draw this widget from scratch.
	 */

	return 0;
    }
}

/*
 *--------------------------------------------------------------
 *
 * PulseDefaultButtonProc --
 *
 *     This function redraws the button on a timer, to pulse
 *     default buttons.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Sets a timer to run itself again.
 *
 *--------------------------------------------------------------
 */

static void
PulseDefaultButtonProc(ClientData clientData)
{
    MacButton *mbPtr = clientData;

    TkpDisplayButton(clientData);
    mbPtr->defaultPulseHandler = Tcl_CreateTimerHandler(
            PULSE_TIMER_MSECS, PulseDefaultButtonProc, clientData);
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
