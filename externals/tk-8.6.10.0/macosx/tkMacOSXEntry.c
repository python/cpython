/*
 * tkMacOSXEntry.c --
 *
 *	This file implements the native aqua entry widget.
 *
 * Copyright 2001, Apple Computer, Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2008-2009, Apple Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkEntry.h"

static ThemeButtonKind	ComputeIncDecParameters(int height, int *width);

#define HIOrientation kHIThemeOrientationNormal

/*
 *--------------------------------------------------------------
 *
 * ComputeIncDecParameters --
 *
 *	This procedure figures out which of the kThemeIncDec buttons to use.
 *	It also sets width to the width of the IncDec button.
 *
 * Results:
 *	The ThemeButtonKind of the button we should use.
 *
 * Side effects:
 *	May draw the entry border into pixmap.
 *
 *--------------------------------------------------------------
 */

static ThemeButtonKind
ComputeIncDecParameters(
    int height,
    int *width)
{
    ThemeButtonKind kind;

    if (height < 11 || height > 28) {
	*width = 0;
	kind = (ThemeButtonKind) 0;
    } else {
	if (height >= 21) {
	    *width = 13;
	    kind = kThemeIncDecButton;
	} else if (height >= 18) {
	    *width = 12;
	    kind = kThemeIncDecButtonSmall;
	} else {
	    *width = 11;
	    kind = kThemeIncDecButtonMini;
	}
    }

    return kind;
}

/*
 *--------------------------------------------------------------
 *
 * TkpDrawEntryBorderAndFocus --
 *
 *	This procedure redraws the border of an entry window. It overrides the
 *	generic border drawing code if the entry widget parameters are such
 *	that the native widget drawing is a good fit. This version just
 *	returns 1, so platforms that don't do special native drawing don't
 *	have to implement it.
 *
 * Results:
 *	1 if it has drawn the border, 0 if not.
 *
 * Side effects:
 *	May draw the entry border into pixmap.
 *
 *--------------------------------------------------------------
 */

int
TkpDrawEntryBorderAndFocus(
    Entry *entryPtr,
    Drawable d,
    int isSpinbox)
{
    CGRect bounds;
    TkMacOSXDrawingContext dc;
    GC bgGC;
    Tk_Window tkwin = entryPtr->tkwin;
    int oldWidth = 0;
    MacDrawable *macDraw = (MacDrawable *) d;
    const HIThemeFrameDrawInfo info = {
	.version = 0,
	.kind = kHIThemeFrameTextFieldSquare,
	.state = (entryPtr->state == STATE_DISABLED ? kThemeStateInactive :
		kThemeStateActive),
	.isFocused = (entryPtr->flags & GOT_FOCUS ? 1 : 0),
    };

    /*
     * I use 6 as the borderwidth. 2 of the 5 go into the actual frame the 3
     * are because the Mac OS Entry widgets leave more space around the Text
     * than Tk does on X11.
     */

    if (entryPtr->borderWidth != MAC_OSX_ENTRY_BORDER
	    || entryPtr->highlightWidth != MAC_OSX_FOCUS_WIDTH
	    || entryPtr->relief != MAC_OSX_ENTRY_RELIEF) {
	return 0;
    }

    /*
     * For the spinbox, we have to make the entry part smaller by the size of
     * the buttons. We also leave 2 pixels to the left (as per the HIG) and
     * space for one pixel to the right, 'cause it makes the buttons look
     * nicer.
     */

    if (isSpinbox) {
	int incDecWidth;

	/*
	 * If native spinbox buttons are going to be drawn, then temporarily
	 * change the width of the widget so that the same code can be used
	 * for drawing the Entry portion of the Spinbox as is used to draw
	 * an ordinary Entry.  The width must be restored before returning.
	 */

	oldWidth = Tk_Width(tkwin);
	if (ComputeIncDecParameters(Tk_Height(tkwin) - 2 * MAC_OSX_FOCUS_WIDTH,
		&incDecWidth) != 0) {
	    Tk_Width(tkwin) -= incDecWidth + 1;
	}
    }

   /*
    * The focus ring is drawn with an Alpha at the outside part of the ring,
    * so we have to draw over the edges of the ring before drawing the focus
    * or the text will peep through.
    */

    bgGC = Tk_GCForColor(entryPtr->highlightBgColorPtr, d);
    TkDrawInsetFocusHighlight(entryPtr->tkwin, bgGC, MAC_OSX_FOCUS_WIDTH, d, 0);

    /*
     * Inset the entry Frame by the maximum width of the focus rect, which is
     * 3 according to the Carbon docs.
     */

    bounds.origin.x = macDraw->xOff + MAC_OSX_FOCUS_WIDTH;
    bounds.origin.y = macDraw->yOff + MAC_OSX_FOCUS_WIDTH;
    bounds.size.width = Tk_Width(tkwin) - 2*MAC_OSX_FOCUS_WIDTH;
    bounds.size.height = Tk_Height(tkwin) - 2*MAC_OSX_FOCUS_WIDTH;
    if (!TkMacOSXSetupDrawingContext(d, NULL, 1, &dc)) {

	/*
	 * No graphics context is available.  If the widget is a Spinbox, we
	 * must restore its width before returning 0. (Ticket [273b6a4996].)
	 */

	if (isSpinbox) {
	    Tk_Width(tkwin) = oldWidth;
	}
	return 0;
    }
    ChkErr(HIThemeDrawFrame, &bounds, &info, dc.context, HIOrientation);
    TkMacOSXRestoreDrawingContext(&dc);
    if (isSpinbox) {
	Tk_Width(tkwin) = oldWidth;
    }
    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * TkpDrawSpinboxButtons --
 *
 *	This procedure redraws the buttons of an spinbox widget. It overrides
 *	the generic button drawing code if the spinbox widget parameters are
 *	such that the native widget drawing is a good fit. This version just
 *	returns 0, so platforms that don't do special native drawing don't
 *	have to implement it.
 *
 * Results:
 *	1 if it has drawn the buttons, 0 if not.
 *
 * Side effects:
 *	May draw the buttons into pixmap.
 *
 *--------------------------------------------------------------
 */

int
TkpDrawSpinboxButtons(
    Spinbox *sbPtr,
    Drawable d)
{
    CGRect bounds;
    Tk_Window tkwin = sbPtr->entry.tkwin;
    int height = Tk_Height(tkwin);
    int buttonHeight = height - 2 * MAC_OSX_FOCUS_WIDTH;
    int incDecWidth;
    TkMacOSXDrawingContext dc;
    XRectangle rects[1];
    GC bgGC;
    MacDrawable *macDraw = (MacDrawable *) d;
    HIThemeButtonDrawInfo info = {
	.version = 0,
	.adornment = kThemeAdornmentNone,
    };

    /*
     * FIXME: RAISED really makes more sense
     */

    if (sbPtr->buRelief != TK_RELIEF_FLAT) {
	return 0;
    }

    /*
     * The actual sizes of the IncDec button are 21 for the normal, 18 for the
     * small and 15 for the mini. But the spinbox still looks okay if the
     * entry is a little bigger than this, so we give it a little slop.
     */

    info.kind = ComputeIncDecParameters(buttonHeight, &incDecWidth);
    if (info.kind == (ThemeButtonKind) 0) {
	return 0;
    }

    if (sbPtr->entry.state == STATE_DISABLED) {
	info.state = kThemeStateInactive;
	info.value = kThemeButtonOff;
    } else if (sbPtr->selElement == SEL_BUTTONUP) {
	info.state = kThemeStatePressedUp;
	info.value = kThemeButtonOn;
    } else if (sbPtr->selElement == SEL_BUTTONDOWN) {
	info.state = kThemeStatePressedDown;
	info.value = kThemeButtonOn;
    } else {
	info.state = kThemeStateActive;
	info.value = kThemeButtonOff;
    }

    bounds.origin.x = macDraw->xOff + Tk_Width(tkwin) - incDecWidth - 1;
    bounds.origin.y = macDraw->yOff + MAC_OSX_FOCUS_WIDTH;
    bounds.size.width = incDecWidth;
    bounds.size.height = Tk_Height(tkwin) - 2*MAC_OSX_FOCUS_WIDTH;

    /*
     * We had to make the entry part of the window smaller so that we wouldn't
     * overdraw the spin buttons with the focus highlight. So now we have to
     * draw the highlightbackground.
     */

    bgGC = Tk_GCForColor(sbPtr->entry.highlightBgColorPtr, d);
    rects[0].x = Tk_Width(tkwin) - incDecWidth - 1;
    rects[0].y = 0;
    rects[0].width = incDecWidth + 1;
    rects[0].height = Tk_Height(tkwin);
    XFillRectangles(Tk_Display(tkwin), d, bgGC, rects, 1);

    if (!TkMacOSXSetupDrawingContext(d, NULL, 1, &dc)) {
	return 0;
    }
    ChkErr(HIThemeDrawButton, &bounds, &info, dc.context, HIOrientation, NULL);
    TkMacOSXRestoreDrawingContext(&dc);
    return 1;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
