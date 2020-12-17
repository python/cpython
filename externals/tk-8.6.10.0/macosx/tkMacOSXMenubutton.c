/*
 * tkMacOSXMenubutton.c --
 *
 *	This file implements the Macintosh specific portion of the menubutton
 *	widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 * Copyright (c) 2006-2007 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2007 Revar Desmera.
 * Copyright 2015 Kevin Walzer/WordTech Communications LLC.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tkMacOSXPrivate.h"
#include "tkMenu.h"
#include "tkMenubutton.h"
#include "tkMacOSXFont.h"
#include "tkMacOSXDebug.h"

#define FIRST_DRAW	    2
#define ACTIVE		    4


typedef struct {
    Tk_3DBorder border;
    int relief;
    GC gc;
    int hasImageOrBitmap;
} DrawParams;

/*
 * Declaration of Mac specific button structure.
 */

typedef struct MacMenuButton {
    TkMenuButton info;		/* Generic button info. */
    int flags;
    ThemeButtonKind btnkind;
    HIThemeButtonDrawInfo drawinfo;
    HIThemeButtonDrawInfo lastdrawinfo;
    DrawParams drawParams;
} MacMenuButton;

/*
 * Forward declarations for static functions defined later in this file:
 */

static void		MenuButtonEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		MenuButtonBackgroundDrawCB(MacMenuButton *ptr,
			    SInt16 depth, Boolean isColorDev);
static void		MenuButtonContentDrawCB(ThemeButtonKind kind,
			    const HIThemeButtonDrawInfo *info,
			    MacMenuButton *ptr, SInt16 depth,
			    Boolean isColorDev);
static void		MenuButtonEventProc(ClientData clientData,
			    XEvent *eventPtr);
static void		TkMacOSXComputeMenuButtonParams(TkMenuButton *butPtr,
			    ThemeButtonKind *btnkind,
			    HIThemeButtonDrawInfo *drawinfo);
static void		TkMacOSXComputeMenuButtonDrawParams(
			    TkMenuButton *butPtr, DrawParams *dpPtr);
static void		TkMacOSXDrawMenuButton(MacMenuButton *butPtr, GC gc,
			    Pixmap pixmap);
static void		DrawMenuButtonImageAndText(TkMenuButton *butPtr);

/*
 * The structure below defines menubutton class behavior by means of
 * procedures that can be invoked from generic window code.
 */

Tk_ClassProcs tkpMenubuttonClass = {
    sizeof(Tk_ClassProcs),	/* size */
    TkMenuButtonWorldChanged,	/* worldChangedProc */
};

/*
 * We use Apple's Pop-Up Button widget to represent the Tk Menubutton.
 * However, we do not use the NSPopUpButton class for this control.  Instead we
 * render the Pop-Up Button using the HITheme library.  This imposes some
 * constraints on what can be done.  The HITheme renderer allows only specific
 * dimensions for the button.
 *
 * The HITheme library allows drawing a Pop-Up Button with an arbitrary bounds
 * rectangle.  However the button is always drawn as a rounded box which is 22
 * pixels high.  If the bounds rectangle is less than 22 pixels high, the
 * button is drawn at the top of the rectangle and the bottom of the button is
 * clipped away.  So we set a minimum height of 22 pixels for a Menubutton.  If
 * the bounds rectangle is more than 22 pixels high, then the button is drawn
 * centered vertically in the bounds rectangle.
 *
 * The content rectangle of the button is inset by 14 pixels on the left and 28
 * pixels on the right.  The rightmost part of the button contains the blue
 * double-arrow symbol which is 28 pixels wide.
 *
 * To maintain compatibility with code that runs on multiple operating systems,
 * the width and height of the content rectangle includes the borderWidth, the
 * highlightWidth and the padX and padY dimensions of the Menubutton.  However,
 * to be consistent with the standard Apple appearance, the content is always
 * be drawn at the left side of the content rectangle.  All of the excess space
 * appears on the right side of the content, and the anchor property is
 * ignored.  The easiest way to comply with Apple's Human Interface Guidelines
 * would be to set bd = highlightthickness = padx = 0 and to specify an
 * explicit width for the button.  Apple also recommends using the same width
 * for all Pop-Up Buttons in a given window.
 */

#define LEFT_INSET 8
#define RIGHT_INSET 28
#define MIN_HEIGHT 22

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateMenuButton  --
 *
 *	Allocate a new TkMenuButton structure.
 *
 * Results:
 *	Returns a newly allocated TkMenuButton structure.
 *
 * Side effects:
 *	Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkMenuButton *
TkpCreateMenuButton(
    Tk_Window tkwin)
{
    MacMenuButton *mbPtr = (MacMenuButton *) ckalloc(sizeof(MacMenuButton));

    Tk_CreateEventHandler(tkwin, ActivateMask, MenuButtonEventProc, mbPtr);
    mbPtr->flags = FIRST_DRAW;
    mbPtr->btnkind = kThemePopupButton;
    bzero(&mbPtr->drawinfo, sizeof(mbPtr->drawinfo));
    bzero(&mbPtr->lastdrawinfo, sizeof(mbPtr->lastdrawinfo));
    return (TkMenuButton *) mbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayMenuButton --
 *
 *	This procedure is invoked to display a menubutton widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menubutton in its current mode.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayMenuButton(
    ClientData clientData)	/* Information about widget. */
{
    MacMenuButton *mbPtr = clientData;
    TkMenuButton *butPtr = clientData;
    Tk_Window tkwin = butPtr->tkwin;
    Pixmap pixmap;
    DrawParams *dpPtr = &mbPtr->drawParams;

    butPtr->flags &= ~REDRAW_PENDING;
    if ((butPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
        return;
    }

    pixmap = (Pixmap) Tk_WindowId(tkwin);

    TkMacOSXComputeMenuButtonDrawParams(butPtr, dpPtr);

    /*
     * Set up clipping region.  Make sure the we are using the port for this
     * button, or we will set the wrong window's clip.
     */

    TkMacOSXSetUpClippingRgn(pixmap);

    /*
     * Draw the native portion of the buttons.
     */

    TkMacOSXDrawMenuButton(mbPtr,  dpPtr->gc, pixmap);

    /*
     * Draw highlight border, if needed.
     */

    if (butPtr->highlightWidth < 3) {
        if (butPtr->flags & GOT_FOCUS) {
	    GC gc = Tk_GCForColor(butPtr->highlightColorPtr, pixmap);
	    TkMacOSXDrawSolidBorder(tkwin, gc, 0, butPtr->highlightWidth);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenuButton --
 *
 *	Free data structures associated with the menubutton control. This is a
 *      no-op on the Mac.
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
TkpDestroyMenuButton(
    TkMenuButton *mbPtr)
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeMenuButtonGeometry --
 *
 *	After changes in a menu button's text or bitmap, this procedure
 *	recomputes the menu button's geometry and passes this information
 *	along to the geometry manager for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu button's window may change size.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeMenuButtonGeometry(butPtr)
    register TkMenuButton *butPtr;	/* Widget record for menu button. */
{
    int width, height, avgWidth, haveImage = 0, haveText = 0;
    int txtWidth, txtHeight;
    Tk_FontMetrics fm;
    int highlightWidth = butPtr->highlightWidth > 0 ? butPtr->highlightWidth : 0;

    /*
     * First compute the size of the contents of the button.
     */

    width = 0;
    height = 0;
    txtWidth = 0;
    txtHeight = 0;
    avgWidth = 0;

    if (butPtr->image != NULL) {
        Tk_SizeOfImage(butPtr->image, &width, &height);
        haveImage = 1;
    } else if (butPtr->bitmap != None) {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
        haveImage = 1;
    }

    if (butPtr->text && strlen(butPtr->text) > 0) {
	haveText = 1;
        Tk_FreeTextLayout(butPtr->textLayout);
        butPtr->textLayout = Tk_ComputeTextLayout(butPtr->tkfont,
                butPtr->text, -1, butPtr->wrapLength,
                butPtr->justify, 0, &butPtr->textWidth, &butPtr->textHeight);
        txtWidth = butPtr->textWidth;
        txtHeight = butPtr->textHeight;
        avgWidth = Tk_TextWidth(butPtr->tkfont, "0", 1);
        Tk_GetFontMetrics(butPtr->tkfont, &fm);
    }

    /*
     * If the button is compound (ie, it shows both an image and text), the new
     * geometry is a combination of the image and text geometry. We only honor
     * the compound bit if the button has both text and an image, because
     * otherwise it is not really a compound button.
     */

    if (haveImage && haveText) {
        switch ((enum compound) butPtr->compound) {
	case COMPOUND_TOP:
	case COMPOUND_BOTTOM:
	    /*
	     * Image is above or below text
	     */

	    height += txtHeight + butPtr->padY;
	    width = (width > txtWidth ? width : txtWidth);
	    break;
	case COMPOUND_LEFT:
	case COMPOUND_RIGHT:
	    /*
	     * Image is left or right of text
	     */

	    width += txtWidth + butPtr->padX;
	    height = (height > txtHeight ? height : txtHeight);
	    break;
	case COMPOUND_CENTER:
	    /*
	     * Image and text are superimposed
	     */

	    width = (width > txtWidth ? width : txtWidth);
	    height = (height > txtHeight ? height : txtHeight);
	    break;
	case COMPOUND_NONE:
	    break;
        }

        if (butPtr->width > 0) {
            width = butPtr->width;
        }
        if (butPtr->height > 0) {
            height = butPtr->height;
        }

    } else {
        if (haveImage) { /* Image only */
            if (butPtr->width > 0) {
                width = butPtr->width;
            }
            if (butPtr->height > 0) {
                height = butPtr->height;
            }
        } else { /* Text only */
            width = txtWidth;
            height = txtHeight;
            if (butPtr->width > 0) {
                width = butPtr->width * avgWidth + 2*butPtr->padX;
            }
            if (butPtr->height > 0) {
                height = butPtr->height * fm.linespace + 2*butPtr->padY;
            }
        }
    }

    butPtr->inset = highlightWidth + butPtr->borderWidth;
    width += LEFT_INSET + RIGHT_INSET + 2*butPtr->inset;
    height += 2*butPtr->inset;
    height = height < MIN_HEIGHT ? MIN_HEIGHT : height;
    Tk_GeometryRequest(butPtr->tkwin, width, height);
    Tk_SetInternalBorder(butPtr->tkwin, butPtr->inset);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuButtonImageAndText --
 *
 *        Draws the image and text associated witha button or label.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The image and text are drawn.
 *
 *----------------------------------------------------------------------
 */
void
DrawMenuButtonImageAndText(
    TkMenuButton *butPtr)
{
    MacMenuButton *mbPtr = (MacMenuButton *) butPtr;
    Tk_Window tkwin  = butPtr->tkwin;
    Pixmap pixmap;
    int haveImage = 0, haveText = 0;
    int imageWidth = 0, imageHeight = 0;
    int imageXOffset = 0, imageYOffset = 0;
    int textXOffset = 0, textYOffset = 0;
    int width = 0, height = 0;
    int fullWidth = 0, fullHeight = 0;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
        return;
    }

    DrawParams *dpPtr = &mbPtr->drawParams;
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

    haveText = (butPtr->textWidth != 0 && butPtr->textHeight != 0);
    if (butPtr->compound != COMPOUND_NONE && haveImage && haveText) {
        int x = 0, y = 0;

        textXOffset = 0;
        textYOffset = 0;
        fullWidth = 0;
        fullHeight = 0;

        switch ((enum compound) butPtr->compound) {
	case COMPOUND_TOP:
	case COMPOUND_BOTTOM:
	    /*
	     * Image is above or below text.
	     */

	    if (butPtr->compound == COMPOUND_TOP) {
		textYOffset = height + butPtr->padY;
	    } else {
		imageYOffset = butPtr->textHeight + butPtr->padY;
	    }
	    fullHeight = height + butPtr->textHeight + butPtr->padY;
	    fullWidth = (width > butPtr->textWidth ?
		     width : butPtr->textWidth);
	    textXOffset = (fullWidth - butPtr->textWidth)/2;
	    imageXOffset = (fullWidth - width)/2;
	    break;
	case COMPOUND_LEFT:
	case COMPOUND_RIGHT:
	    /*
	     * Image is left or right of text
	     */

	    if (butPtr->compound == COMPOUND_LEFT) {
		textXOffset = width + butPtr->padX - 2;
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

	    fullWidth = (width > butPtr->textWidth ? width : butPtr->textWidth);
	    fullHeight = (height > butPtr->textHeight ? height :
                    butPtr->textHeight);
	    textXOffset = (fullWidth - butPtr->textWidth) / 2;
	    imageXOffset = (fullWidth - width) / 2;
	    textYOffset = (fullHeight - butPtr->textHeight) / 2;
	    imageYOffset = (fullHeight - height) / 2;
	    break;
	case COMPOUND_NONE:
	    break;
	}

        TkComputeAnchor(butPtr->anchor, tkwin,
                butPtr->padX + butPtr->inset, butPtr->padY + butPtr->inset,
                fullWidth, fullHeight, &x, &y);
        imageXOffset = LEFT_INSET;
        imageYOffset += y;
        textYOffset -= 1;

        if (butPtr->image != NULL) {
	    Tk_RedrawImage(butPtr->image, 0, 0, width,
                    height, pixmap, imageXOffset, imageYOffset);
        } else {
            XSetClipOrigin(butPtr->display, dpPtr->gc,
                    imageXOffset, imageYOffset);
            XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, dpPtr->gc,
                    0, 0, (unsigned int) width, (unsigned int) height,
                    imageXOffset, imageYOffset, 1);
            XSetClipOrigin(butPtr->display, dpPtr->gc, 0, 0);
        }

        Tk_DrawTextLayout(butPtr->display, pixmap,
                dpPtr->gc, butPtr->textLayout,
                x + textXOffset, y + textYOffset, 0, -1);
        Tk_UnderlineTextLayout(butPtr->display, pixmap, dpPtr->gc,
                butPtr->textLayout, x + textXOffset, y + textYOffset,
                butPtr->underline);
    } else {
	int x, y;

        if (haveImage) {
            TkComputeAnchor(butPtr->anchor, tkwin,
                    butPtr->padX + butPtr->borderWidth,
                    butPtr->padY + butPtr->borderWidth,
                    width, height, &x, &y);
	    imageXOffset = LEFT_INSET;
	    imageYOffset += y;
	    if (butPtr->image != NULL) {
		Tk_RedrawImage(butPtr->image, 0, 0, width, height,
			       pixmap, imageXOffset, imageYOffset);
            } else {
                XSetClipOrigin(butPtr->display, dpPtr->gc, x, y);
                XCopyPlane(butPtr->display, butPtr->bitmap,
			   pixmap, dpPtr->gc,
			   0, 0, (unsigned int) width,
			   (unsigned int) height,
			   imageXOffset, imageYOffset, 1);
                XSetClipOrigin(butPtr->display, dpPtr->gc, 0, 0);
            }
        } else {
	    textXOffset = LEFT_INSET;
	    TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
		    butPtr->textWidth, butPtr->textHeight, &x, &y);
	    Tk_DrawTextLayout(butPtr->display, pixmap, dpPtr->gc,
		    butPtr->textLayout, textXOffset, y, 0, -1);
	    y += butPtr->textHeight/2;
	}
   }
}

/*
 *--------------------------------------------------------------
 *
 * TkMacOSXDrawMenuButton --
 *
 *        This function draws the tk menubutton using Mac controls. In
 *        addition, this code may apply custom colors passed in the
 *        TkMenubutton.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        None.
 *
 *--------------------------------------------------------------
 */

static void
TkMacOSXDrawMenuButton(
    MacMenuButton *mbPtr, /* Mac menubutton. */
    GC gc,                /* The GC we are drawing into - needed for the bevel
                           * button */
    Pixmap pixmap)        /* The pixmap we are drawing into - needed for the
                           * bevel button */
{
    TkMenuButton *butPtr = (TkMenuButton *) mbPtr;
    TkWindow *winPtr = (TkWindow *) butPtr->tkwin;
    HIRect cntrRect;
    TkMacOSXDrawingContext dc;
    DrawParams *dpPtr = &mbPtr->drawParams;
    int useNewerHITools = 1;

    TkMacOSXComputeMenuButtonParams(butPtr, &mbPtr->btnkind, &mbPtr->drawinfo);

    cntrRect = CGRectMake(winPtr->privatePtr->xOff, winPtr->privatePtr->yOff,
	    Tk_Width(butPtr->tkwin), Tk_Height(butPtr->tkwin));

    if (useNewerHITools == 1) {
        HIRect contHIRec;
        static HIThemeButtonDrawInfo hiinfo;

        MenuButtonBackgroundDrawCB(mbPtr, 32, true);
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
	 * To avoid menubuttons with white text on a white background, we
	 * always set the state to inactive in Dark Mode.  It isn't perfect but
	 * it is usable.  Using a ttk::menubutton would be a better choice,
	 * however.
	 */

	if (TkMacOSXInDarkMode(butPtr->tkwin)) {
	    hiinfo.state = kThemeStateInactive;
	}

        HIThemeDrawButton(&cntrRect, &hiinfo, dc.context,
		kHIThemeOrientationNormal, &contHIRec);
	TkMacOSXRestoreDrawingContext(&dc);
        MenuButtonContentDrawCB(mbPtr->btnkind, &mbPtr->drawinfo,
		mbPtr, 32, true);
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
 * MenuButtonBackgroundDrawCB --
 *
 *      This function draws the background that lies under checkboxes and
 *      radiobuttons.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The background gets updated to the current color.
 *
 *--------------------------------------------------------------
 */

static void
MenuButtonBackgroundDrawCB (
    MacMenuButton *ptr,
    SInt16 depth,
    Boolean isColorDev)
{
    TkMenuButton* butPtr = (TkMenuButton *) ptr;
    Tk_Window tkwin = butPtr->tkwin;
    Pixmap pixmap;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
        return;
    }
    pixmap = (Pixmap) Tk_WindowId(tkwin);
    Tk_Fill3DRectangle(tkwin, pixmap, butPtr->normalBorder, 0, 0,
            Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonContentDrawCB --
 *
 *      This function draws the label and image for the button.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The content of the button gets updated.
 *
 *--------------------------------------------------------------
 */

static void
MenuButtonContentDrawCB (
    ThemeButtonKind kind,
    const HIThemeButtonDrawInfo *drawinfo,
    MacMenuButton *ptr,
    SInt16 depth,
    Boolean isColorDev)
{
    TkMenuButton *butPtr = (TkMenuButton *) ptr;
    Tk_Window tkwin = butPtr->tkwin;

    if (tkwin == NULL || !Tk_IsMapped(tkwin)) {
        return;
    }
    DrawMenuButtonImageAndText(butPtr);
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonEventProc --
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
MenuButtonEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkMenuButton *buttonPtr = clientData;
    MacMenuButton *mbPtr = clientData;

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
	    Tcl_DoWhenIdle(TkpDisplayMenuButton, (ClientData) buttonPtr);
	    buttonPtr->flags |= REDRAW_PENDING;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXComputeMenuButtonParams --
 *
 *      This procedure computes the various parameters used when creating a
 *      Carbon Appearance control. These are determined by the various Tk
 *      button parameters
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Sets the btnkind and drawinfo parameters
 *
 *----------------------------------------------------------------------
 */

static void
TkMacOSXComputeMenuButtonParams(
    TkMenuButton *butPtr,
    ThemeButtonKind *btnkind,
    HIThemeButtonDrawInfo *drawinfo)
{
    MacMenuButton *mbPtr = (MacMenuButton *) butPtr;

    if (butPtr->image || butPtr->bitmap || butPtr->text) {
	/* TODO: allow for Small and Mini menubuttons. */
	*btnkind = kThemePopupButton;
    } else { /* This should never happen. */
	*btnkind = kThemeArrowButton;
    }

    drawinfo->value = kThemeButtonOff;

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
    } else {
        drawinfo->state = kThemeStateActive;
    }

    drawinfo->adornment = kThemeAdornmentNone;
    if (butPtr->highlightWidth >= 3) {
        if ((butPtr->flags & GOT_FOCUS)) {
            drawinfo->adornment |= kThemeAdornmentFocus;
        }
    }
    drawinfo->adornment |= kThemeAdornmentArrowDoubleArrow;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXComputeMenuButtonDrawParams --
 *
 *      This procedure selects an appropriate drawing context for drawing a
 *      menubutton.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Sets the button draw parameters.
 *
 *----------------------------------------------------------------------
 */

static void
TkMacOSXComputeMenuButtonDrawParams(
    TkMenuButton *butPtr,
    DrawParams *dpPtr)
{
    dpPtr->hasImageOrBitmap =
	    ((butPtr->image != NULL) || (butPtr->bitmap != None));
    dpPtr->border = butPtr->normalBorder;
    if ((butPtr->state == STATE_DISABLED) && (butPtr->disabledFg != NULL)) {
        dpPtr->gc = butPtr->disabledGC;
    } else if (butPtr->state == STATE_ACTIVE) {
        dpPtr->gc = butPtr->activeTextGC;
        dpPtr->border = butPtr->activeBorder;
    } else {
        dpPtr->gc = butPtr->normalTextGC;
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
