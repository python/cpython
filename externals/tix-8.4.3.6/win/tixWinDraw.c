/*
 * tixWinDraw.c --
 *
 *	Implement the Windows specific function calls for drawing.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000      Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixWinDraw.c,v 1.5 2004/03/28 02:44:57 hobbs Exp $
 */

#include <tkWinInt.h>
#include <tixInt.h>
#include <tixPort.h>


/*----------------------------------------------------------------------
 *
 * TixpDrawTmpLine --
 *
 *	Draws a "temporarily" line on the desktop window with XOR
 *	drawing mode. This function is used by the PanedWindow and
 *	ResizeHandler to draw the rubberband lines. Calling the
 *	function again with the same parameters cancels the temporary
 *	lines without affecting what was originally on the screen.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

void
TixpDrawTmpLine(x1, y1, x2, y2, tkwin)
    int x1;
    int y1;
    int x2;
    int y2;
    Tk_Window tkwin;
{
    HWND desktop;
    HDC hdc;
    HPEN hpen;
    HGDIOBJ old;

    desktop = GetDesktopWindow();
    hdc = GetWindowDC(desktop);
    hpen = CreatePen(PS_SOLID, 0, RGB(255,255,255));

    old = SelectObject(hdc, hpen);
    SetROP2(hdc, R2_XORPEN);

    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);

    SelectObject(hdc, old);
    DeleteObject(hpen);
    ReleaseDC(desktop, hdc);	
}

/*
 *----------------------------------------------------------------------
 *
 * TixpDrawAnchorLines --
 *
 *	See comments near Tix_DrawAnchorLines in tixUtils.c.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

void
TixpDrawAnchorLines(display, drawable, gc, x, y, w, h)
    Display *display;
    Drawable drawable;
    GC gc;
    int x;
    int y;
    int w;
    int h;
{
    HDC hdc;
    TkWinDCState state;
    RECT rect;

    if (w < 2 || h < 2) {
        /*
         * Area too small to show effect. Don't bother
         */
	return;
    }

    hdc = TkWinGetDrawableDC(display, drawable, &state);
    rect.left   = x;
    rect.top    = y;
    rect.right  = x+w;
    rect.bottom = y+h;
    DrawFocusRect(hdc, &rect);
    TkWinReleaseDrawableDC(drawable, hdc, &state);
}

/*
 *----------------------------------------------------------------------
 *
 * TixpStartSubRegionDraw --
 *
 *      This function is used by the Tix DItem code to implement
 *      clipped rendering -- if a DItem is larger than the region
 *      where the DItem is displayed (with the Tix_DItemDisplay
 *      function), we clip the DItem so that all the rendering
 *      happens inside the region.
 *
 *      This Win32 implementation is tricky (which explains why the
 *      TixpSubRegDrawXXX API looks so arcane.) Tk does not support
 *      a portable API for setting the clip region of a GC. We could
 *      hack into Tk's Win32 implementation of GC to get the clipping
 *      to work, but that may run into future incompatibilities.
 *
 *      For a clean and (almost) portable, albeit a bit slow,
 *      implemetation of clipping, we allocate a pixmap when clipping
 *      is required. All subsequent drawing goes into this
 *      pixmap. When TixpEndSubRegionDraw is called we then copy from
 *      the pixmap back to the destination drawable.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      A Tk pixmap may be created and saved into subRegPtr->pixmap in
 *      for the clipped drawing operations.
 *
 *----------------------------------------------------------------------
 */

void
TixpStartSubRegionDraw(display, drawable, gc, subRegPtr, origX, origY,
	x, y, width, height, needWidth, needHeight)
    Display *display;
    Drawable drawable;
    GC gc;
    TixpSubRegion * subRegPtr;
    int origX;
    int origY;
    int x;
    int y;
    int width;
    int height;
    int needWidth;
    int needHeight;
{
    TkWinDrawable * wdrPtr;
    int depth;

    if ((width < needWidth) || (height < needHeight)) {
	subRegPtr->origX  = origX;
	subRegPtr->origY  = origY;
	subRegPtr->x	  = x;
	subRegPtr->y	  = y;
	subRegPtr->width  = width;
	subRegPtr->height = height;

	/*
	 * Find out the depth of the drawable and create a pixmap of
	 * the same depth.
	 */

	wdrPtr = (TkWinDrawable *)drawable;
	if (wdrPtr->type == TWD_BITMAP) {
	    depth = wdrPtr->bitmap.depth;
	} else {
	    depth = wdrPtr->window.winPtr->depth;
	}

	subRegPtr->pixmap = Tk_GetPixmap(display, drawable, width, height,
		depth);

	if (subRegPtr->pixmap != None) {
	    /*
	     * It could be None if we have somehow exhausted the Windows
	     * GDI resources.
	     */
	    XCopyArea(display, drawable, subRegPtr->pixmap, gc, x, y,
		    (unsigned) width, (unsigned) height, 0, 0);
	}
    } else {
	subRegPtr->pixmap = None;
    }
}

/*----------------------------------------------------------------------
 * TixpEndSubRegionDraw --
 *
 *
 *----------------------------------------------------------------------
 */

void
TixpEndSubRegionDraw(display, drawable, gc, subRegPtr)
    Display *display;
    Drawable drawable;
    GC gc;
    TixpSubRegion * subRegPtr;
{
    if (subRegPtr->pixmap != None) {
	XCopyArea(display, subRegPtr->pixmap, drawable, gc, 0, 0,
		(unsigned) subRegPtr->width, (unsigned) subRegPtr->height,
		subRegPtr->x, subRegPtr->y);
	Tk_FreePixmap(display, subRegPtr->pixmap);
	subRegPtr->pixmap = None;
    }
}

void
TixpSubRegSetClip(display, subRegPtr, gc)
    Display *display;
    TixpSubRegion * subRegPtr;
    GC gc;
{
    /* Do nothing */
}

void
TixpSubRegUnsetClip(display, subRegPtr, gc)
    Display *display;
    TixpSubRegion * subRegPtr;
    GC gc;
{
    /* Do nothing */
}

/*
 *----------------------------------------------------------------------
 *
 * TixpSubRegDisplayText --
 *
 *	Display a text string on one or more lines in a sub region.
 *
 * Results:
 *	See TkDisplayText
 *
 * Side effects:
 *	See TkDisplayText
 *
 *----------------------------------------------------------------------
 */

void
TixpSubRegDisplayText(display, drawable, gc, subRegPtr, font, string,
	numChars, x, y,	length, justify, underline)
    Display *display;		/* X display to use for drawing text. */
    Drawable drawable;		/* Window or pixmap in which to draw the
				 * text. */
    GC gc;			/* Graphics context to use for drawing text. */
    TixpSubRegion * subRegPtr;	/* Information about the subregion */
    TixFont font;		/* Font that determines geometry of text
				 * (should be same as font in gc). */
    CONST84 char *string;	/* String to display;  may contain embedded
				 * newlines. */
    int numChars;		/* Number of characters to use from string. */
    int x, y;			/* Pixel coordinates within drawable of
				 * upper left corner of display area. */
    int length;			/* Line length in pixels;  used to compute
				 * word wrap points and also for
				 * justification.   Must be > 0. */
    Tk_Justify justify;		/* How to justify lines. */
    int underline;		/* Index of character to underline, or < 0
				 * for no underlining. */
{
    if (subRegPtr->pixmap != None) {
	TixDisplayText(display, subRegPtr->pixmap, font, string,
		numChars, x - subRegPtr->x, y - subRegPtr->y,
		length, justify, underline, gc);
    } else {
	TixDisplayText(display, drawable, font, string,
		numChars, x, y,	length, justify, underline, gc);
    }
}

/*----------------------------------------------------------------------
 * TixpSubRegFillRectangle --
 *
 *
 *----------------------------------------------------------------------
 */

void
TixpSubRegFillRectangle(display, drawable, gc, subRegPtr, x, y, width, height)
    Display *display;		/* X display to use for drawing rectangle. */
    Drawable drawable;		/* Window or pixmap in which to draw the
				 * rectangle. */
    GC gc;			/* Graphics context to use for drawing. */
    TixpSubRegion * subRegPtr;	/* Information about the subregion */
    int x, y;			/* Pixel coordinates within drawable of
				 * upper left corner of display area. */
    int width, height;		/* Size of the rectangle. */
{
    if (subRegPtr->pixmap != None) {
	XFillRectangle(display, subRegPtr->pixmap, gc,
		x - subRegPtr->x, y - subRegPtr->y, width, height);
    } else {
	XFillRectangle(display, drawable, gc, x, y, width, height);
    }
}

/*----------------------------------------------------------------------
 * TixpSubRegDrawImage	--
 *
 *	Draws a Tk image in a subregion.
 *----------------------------------------------------------------------
 */

void
TixpSubRegDrawImage(subRegPtr, image, imageX, imageY, width, height,
	drawable, drawableX, drawableY)
    TixpSubRegion * subRegPtr;
    Tk_Image image;
    int imageX;
    int imageY;
    int width;
    int height;
    Drawable drawable;
    int drawableX;
    int drawableY;
{
    Drawable dest;

    if (subRegPtr->pixmap != None) {
        dest = subRegPtr->pixmap;
        drawableX -= subRegPtr->x;
        drawableY -= subRegPtr->y;
    } else {
        dest = drawable;
    }

    Tk_RedrawImage(image, imageX, imageY, width, height, dest,
	    drawableX, drawableY);
}

void
TixpSubRegDrawBitmap(display, drawable, gc, subRegPtr, bitmap, src_x, src_y,
	width, height, dest_x, dest_y, plane)
    Display *display;
    Drawable drawable;
    GC gc;
    TixpSubRegion * subRegPtr;
    Pixmap bitmap;
    int src_x, src_y;
    int width, height;
    int dest_x, dest_y;
    unsigned long plane;
{
    XSetClipOrigin(display, gc, dest_x, dest_y);
    if (subRegPtr->pixmap != None) {
	XCopyPlane(display, bitmap, subRegPtr->pixmap, gc, src_x, src_y,
		width, height, dest_x - subRegPtr->x, dest_y - subRegPtr->y,
		plane);
    } else {
	XCopyPlane(display, bitmap, drawable, gc, src_x, src_y, width, height,
	        dest_x, dest_y, plane);
    }
    XSetClipOrigin(display, gc, 0, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TixpSubRegDrawAnchorLines --
 *
 *	Draw anchor lines inside the given sub region.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

void
TixpSubRegDrawAnchorLines(display, drawable, gc, subRegPtr, x, y, w, h)
    Display *display;           /* Display to draw on. */
    Drawable drawable;          /* Drawable to draw on. */
    GC gc;                      /* Use the foreground color of this GC. */
    TixpSubRegion * subRegPtr;  /* Describes the subregion. */
    int x;                      /* x pos of top-left corner of anchor rect */
    int y;                      /* y pos of top-left corner of anchor rect */
    int w;                      /* width of anchor rect */
    int h;                      /* height of anchor rect */
{
    Drawable dest;

    if (subRegPtr->pixmap != None) {
        dest = subRegPtr->pixmap;
        x -= subRegPtr->x;
        y -= subRegPtr->y;
    } else {
        dest = drawable;
    }

    TixpDrawAnchorLines(display, dest, gc, x, y, w, h);
}
