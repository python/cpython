/*
 * tkUnixMenubu.c --
 *
 *	This file implements the Unix specific portion of the menubutton
 *	widget.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkMenubutton.h"


/*
 *----------------------------------------------------------------------
 *
 * TkpCreateMenuButton --
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
    return ckalloc(sizeof(TkMenuButton));
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayMenuButton --
 *
 *	This function is invoked to display a menubutton widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menubutton in its current
 *	mode.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayMenuButton(
    ClientData clientData)	/* Information about widget. */
{
    register TkMenuButton *mbPtr = (TkMenuButton *) clientData;
    GC gc;
    Tk_3DBorder border;
    Pixmap pixmap;
    int x = 0;			/* Initialization needed only to stop compiler
				 * warning. */
    int y = 0;
    register Tk_Window tkwin = mbPtr->tkwin;
    int fullWidth, fullHeight;
    int textXOffset, textYOffset;
    int imageWidth, imageHeight;
    int imageXOffset, imageYOffset;
    int width = 0, height = 0;
				/* Image information that will be used to
				 * restrict disabled pixmap as well */
    int haveImage = 0, haveText = 0;

    mbPtr->flags &= ~REDRAW_PENDING;
    if ((mbPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    if ((mbPtr->state == STATE_DISABLED) && (mbPtr->disabledFg != NULL)) {
	gc = mbPtr->disabledGC;
	border = mbPtr->normalBorder;
    } else if ((mbPtr->state == STATE_ACTIVE)
	       && !Tk_StrictMotif(mbPtr->tkwin)) {
	gc = mbPtr->activeTextGC;
	border = mbPtr->activeBorder;
    } else {
	gc = mbPtr->normalTextGC;
	border = mbPtr->normalBorder;
    }

    if (mbPtr->image != None) {
	Tk_SizeOfImage(mbPtr->image, &width, &height);
	haveImage = 1;
    } else if (mbPtr->bitmap != None) {
	Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
	haveImage = 1;
    }
    imageWidth	= width;
    imageHeight = height;

    haveText = (mbPtr->textWidth != 0 && mbPtr->textHeight != 0);

    /*
     * In order to avoid screen flashes, this function redraws the menu button
     * in a pixmap, then copies the pixmap to the screen in a single
     * operation. This means that there's no point in time where the on-sreen
     * image has been cleared.
     */

    pixmap = Tk_GetPixmap(mbPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
    Tk_Fill3DRectangle(tkwin, pixmap, border, 0, 0, Tk_Width(tkwin),
	    Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    imageXOffset = 0;
    imageYOffset = 0;
    textXOffset = 0;
    textYOffset = 0;
    fullWidth = 0;
    fullHeight = 0;

    if (mbPtr->compound != COMPOUND_NONE && haveImage && haveText) {
	switch ((enum compound) mbPtr->compound) {
	case COMPOUND_TOP:
	case COMPOUND_BOTTOM:
	    /*
	     * Image is above or below text.
	     */

	    if (mbPtr->compound == COMPOUND_TOP) {
		textYOffset = height + mbPtr->padY;
	    } else {
		imageYOffset = mbPtr->textHeight + mbPtr->padY;
	    }
	    fullHeight = height + mbPtr->textHeight + mbPtr->padY;
	    fullWidth = (width > mbPtr->textWidth ? width : mbPtr->textWidth);
	    textXOffset = (fullWidth - mbPtr->textWidth)/2;
	    imageXOffset = (fullWidth - width)/2;
	    break;
	case COMPOUND_LEFT:
	case COMPOUND_RIGHT:
	    /*
	     * Image is left or right of text.
	     */

	    if (mbPtr->compound == COMPOUND_LEFT) {
		textXOffset = width + mbPtr->padX;
	    } else {
		imageXOffset = mbPtr->textWidth + mbPtr->padX;
	    }
	    fullWidth = mbPtr->textWidth + mbPtr->padX + width;
	    fullHeight = (height > mbPtr->textHeight ? height :
		    mbPtr->textHeight);
	    textYOffset = (fullHeight - mbPtr->textHeight)/2;
	    imageYOffset = (fullHeight - height)/2;
	    break;
	case COMPOUND_CENTER:
	    /*
	     * Image and text are superimposed.
	     */

	    fullWidth = (width > mbPtr->textWidth ? width : mbPtr->textWidth);
	    fullHeight = (height > mbPtr->textHeight ? height :
		    mbPtr->textHeight);
	    textXOffset = (fullWidth - mbPtr->textWidth)/2;
	    imageXOffset = (fullWidth - width)/2;
	    textYOffset = (fullHeight - mbPtr->textHeight)/2;
	    imageYOffset = (fullHeight - height)/2;
	    break;
	case COMPOUND_NONE:
	    break;
	}

	TkComputeAnchor(mbPtr->anchor, tkwin, 0, 0,
		mbPtr->indicatorWidth + fullWidth, fullHeight, &x, &y);

	imageXOffset += x;
	imageYOffset += y;
	if (mbPtr->image != NULL) {
	    Tk_RedrawImage(mbPtr->image, 0, 0, width, height, pixmap,
		    imageXOffset, imageYOffset);
	} else if (mbPtr->bitmap != None) {
	    XSetClipOrigin(mbPtr->display, gc, imageXOffset, imageYOffset);
	    XCopyPlane(mbPtr->display, mbPtr->bitmap, pixmap,
		    gc, 0, 0, (unsigned) width, (unsigned) height,
		    imageXOffset, imageYOffset, 1);
	    XSetClipOrigin(mbPtr->display, gc, 0, 0);
	}

	Tk_DrawTextLayout(mbPtr->display, pixmap, gc, mbPtr->textLayout,
		x + textXOffset, y + textYOffset, 0, -1);
	Tk_UnderlineTextLayout(mbPtr->display, pixmap, gc, mbPtr->textLayout,
		x + textXOffset, y + textYOffset, mbPtr->underline);
    } else if (haveImage) {
	TkComputeAnchor(mbPtr->anchor, tkwin, 0, 0,
		width + mbPtr->indicatorWidth, height, &x, &y);
	imageXOffset += x;
	imageYOffset += y;
	if (mbPtr->image != NULL) {
	    Tk_RedrawImage(mbPtr->image, 0, 0, width, height, pixmap,
		    imageXOffset, imageYOffset);
	} else if (mbPtr->bitmap != None) {
	    XSetClipOrigin(mbPtr->display, gc, x, y);
	    XCopyPlane(mbPtr->display, mbPtr->bitmap, pixmap,
		    gc, 0, 0, (unsigned) width, (unsigned) height,
		    x, y, 1);
	    XSetClipOrigin(mbPtr->display, gc, 0, 0);
	}
    } else {
	TkComputeAnchor(mbPtr->anchor, tkwin, mbPtr->padX, mbPtr->padY,
		mbPtr->textWidth + mbPtr->indicatorWidth,
		mbPtr->textHeight, &x, &y);
	Tk_DrawTextLayout(mbPtr->display, pixmap, gc, mbPtr->textLayout,
		x + textXOffset, y + textYOffset, 0, -1);
	Tk_UnderlineTextLayout(mbPtr->display, pixmap, gc,
		mbPtr->textLayout, x + textXOffset, y + textYOffset,
		mbPtr->underline);
    }

    /*
     * If the menu button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.
     */

    if ((mbPtr->state == STATE_DISABLED)
	    && ((mbPtr->disabledFg == NULL) || (mbPtr->image != NULL))) {
	/*
	 * Stipple the whole button if no disabledFg was specified, otherwise
	 * restrict stippling only to displayed image
	 */

	if (mbPtr->disabledFg == NULL) {
	    XFillRectangle(mbPtr->display, pixmap, mbPtr->stippleGC,
		    mbPtr->inset, mbPtr->inset,
		    (unsigned) (Tk_Width(tkwin) - 2*mbPtr->inset),
		    (unsigned) (Tk_Height(tkwin) - 2*mbPtr->inset));
	} else {
	    XFillRectangle(mbPtr->display, pixmap, mbPtr->stippleGC,
		    imageXOffset, imageYOffset,
		    (unsigned) imageWidth, (unsigned) imageHeight);
	}
    }

    /*
     * Draw the cascade indicator for the menu button on the right side of the
     * window, if desired.
     */

    if (mbPtr->indicatorOn) {
	int borderWidth;

	borderWidth = (mbPtr->indicatorHeight+1)/3;
	if (borderWidth < 1) {
	    borderWidth = 1;
	}
	/*y += mbPtr->textHeight / 2;*/
	Tk_Fill3DRectangle(tkwin, pixmap, border,
		Tk_Width(tkwin) - mbPtr->inset - mbPtr->indicatorWidth
		+ mbPtr->indicatorHeight,
		((int) (Tk_Height(tkwin) - mbPtr->indicatorHeight))/2,
		mbPtr->indicatorWidth - 2*mbPtr->indicatorHeight,
		mbPtr->indicatorHeight, borderWidth, TK_RELIEF_RAISED);
    }

    /*
     * Draw the border and traversal highlight last. This way, if the menu
     * button's contents overflow onto the border they'll be covered up by the
     * border.
     */

    if (mbPtr->relief != TK_RELIEF_FLAT) {
	Tk_Draw3DRectangle(tkwin, pixmap, border,
		mbPtr->highlightWidth, mbPtr->highlightWidth,
		Tk_Width(tkwin) - 2*mbPtr->highlightWidth,
		Tk_Height(tkwin) - 2*mbPtr->highlightWidth,
		mbPtr->borderWidth, mbPtr->relief);
    }
    if (mbPtr->highlightWidth != 0) {
	GC gc;

	if (mbPtr->flags & GOT_FOCUS) {
	    gc = Tk_GCForColor(mbPtr->highlightColorPtr, pixmap);
	} else {
	    gc = Tk_GCForColor(mbPtr->highlightBgColorPtr, pixmap);
	}
	Tk_DrawFocusHighlight(tkwin, gc, mbPtr->highlightWidth, pixmap);
    }

    /*
     * Copy the information from the off-screen pixmap onto the screen, then
     * delete the pixmap.
     */

    XCopyArea(mbPtr->display, pixmap, Tk_WindowId(tkwin),
	    mbPtr->normalTextGC, 0, 0, (unsigned) Tk_Width(tkwin),
	    (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(mbPtr->display, pixmap);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenuButton --
 *
 *	Free data structures associated with the menubutton control.
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
TkpDestroyMenuButton(
    TkMenuButton *mbPtr)
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeMenuButtonGeometry --
 *
 *	After changes in a menu button's text or bitmap, this function
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
TkpComputeMenuButtonGeometry(
    TkMenuButton *mbPtr)	/* Widget record for menu button. */
{
    int width, height, mm, pixels;
    int	 avgWidth, txtWidth, txtHeight;
    int haveImage = 0, haveText = 0;
    Tk_FontMetrics fm;

    mbPtr->inset = mbPtr->highlightWidth + mbPtr->borderWidth;

    width = 0;
    height = 0;
    txtWidth = 0;
    txtHeight = 0;
    avgWidth = 0;

    if (mbPtr->image != None) {
	Tk_SizeOfImage(mbPtr->image, &width, &height);
	haveImage = 1;
    } else if (mbPtr->bitmap != None) {
	Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
	haveImage = 1;
    }

    if (haveImage == 0 || mbPtr->compound != COMPOUND_NONE) {
	Tk_FreeTextLayout(mbPtr->textLayout);

	mbPtr->textLayout = Tk_ComputeTextLayout(mbPtr->tkfont, mbPtr->text,
		-1, mbPtr->wrapLength, mbPtr->justify, 0, &mbPtr->textWidth,
		&mbPtr->textHeight);
	txtWidth = mbPtr->textWidth;
	txtHeight = mbPtr->textHeight;
	avgWidth = Tk_TextWidth(mbPtr->tkfont, "0", 1);
	Tk_GetFontMetrics(mbPtr->tkfont, &fm);
	haveText = (txtWidth != 0 && txtHeight != 0);
    }

    /*
     * If the menubutton is compound (ie, it shows both an image and text),
     * the new geometry is a combination of the image and text geometry. We
     * only honor the compound bit if the menubutton has both text and an
     * image, because otherwise it is not really a compound menubutton.
     */

    if (mbPtr->compound != COMPOUND_NONE && haveImage && haveText) {
	switch ((enum compound) mbPtr->compound) {
	case COMPOUND_TOP:
	case COMPOUND_BOTTOM:
	    /*
	     * Image is above or below text.
	     */

	    height += txtHeight + mbPtr->padY;
	    width = (width > txtWidth ? width : txtWidth);
	    break;
	case COMPOUND_LEFT:
	case COMPOUND_RIGHT:
	    /*
	     * Image is left or right of text.
	     */

	    width += txtWidth + mbPtr->padX;
	    height = (height > txtHeight ? height : txtHeight);
	    break;
	case COMPOUND_CENTER:
	    /*
	     * Image and text are superimposed.
	     */

	    width = (width > txtWidth ? width : txtWidth);
	    height = (height > txtHeight ? height : txtHeight);
	    break;
	case COMPOUND_NONE:
	    break;
	}
	if (mbPtr->width > 0) {
	    width = mbPtr->width;
	}
	if (mbPtr->height > 0) {
	    height = mbPtr->height;
	}
	width += 2*mbPtr->padX;
	height += 2*mbPtr->padY;
    } else {
	if (haveImage) {
	    if (mbPtr->width > 0) {
		width = mbPtr->width;
	    }
	    if (mbPtr->height > 0) {
		height = mbPtr->height;
	    }
	} else {
	    width = txtWidth;
	    height = txtHeight;
	    if (mbPtr->width > 0) {
		width = mbPtr->width * avgWidth;
	    }
	    if (mbPtr->height > 0) {
		height = mbPtr->height * fm.linespace;
	    }
	}
    }

    if (! haveImage) {
	width += 2*mbPtr->padX;
	height += 2*mbPtr->padY;
    }

    if (mbPtr->indicatorOn) {
	mm = WidthMMOfScreen(Tk_Screen(mbPtr->tkwin));
	pixels = WidthOfScreen(Tk_Screen(mbPtr->tkwin));
	mbPtr->indicatorHeight= (INDICATOR_HEIGHT * pixels)/(10*mm);
	mbPtr->indicatorWidth = (INDICATOR_WIDTH * pixels)/(10*mm)
		+ 2*mbPtr->indicatorHeight;
	width += mbPtr->indicatorWidth;
    } else {
	mbPtr->indicatorHeight = 0;
	mbPtr->indicatorWidth = 0;
    }

    Tk_GeometryRequest(mbPtr->tkwin, (int) (width + 2*mbPtr->inset),
	    (int) (height + 2*mbPtr->inset));
    Tk_SetInternalBorder(mbPtr->tkwin, mbPtr->inset);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
