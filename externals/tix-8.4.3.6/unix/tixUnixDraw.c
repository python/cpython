/*
 * tixUnixDraw.c --
 *
 *	Implement the Unix specific function calls for drawing.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000      Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixUnixDraw.c,v 1.6 2005/03/25 20:15:53 hobbs Exp $
 */

#include <tixPort.h>
#include <tixUnixInt.h>


/*
 *----------------------------------------------------------------------
 *
 * TixpDrawTmpLine --
 *
 *	Draws a "temporary" line between the two points. The line can be
 *	removed by calling the function again with the same parameters.
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
TixpDrawTmpLine(x1, y1, x2, y2, tkwin)
    int x1;
    int y1;
    int x2;
    int y2;
    Tk_Window tkwin;
{
    GC gc;
    XGCValues values;
    unsigned long valuemask = GCForeground | GCSubwindowMode | GCFunction;
    Window winId;		/* The Window to draw into. */
    Tk_Window toplevel;		/* Toplevel containing the tkwin. */
    int rootx1, rooty1;		/* Root x and y of the toplevel window. */
    int rootx2, rooty2;

    for (toplevel=tkwin; !Tk_IsTopLevel(toplevel);
	    toplevel=Tk_Parent(toplevel)) {
	;
    }

    Tk_GetRootCoords(toplevel, &rootx1, &rooty1);
    rootx2 = rootx1 + Tk_Width(toplevel)  - 1;
    rooty2 = rooty1 + Tk_Height(toplevel) - 1;

    if (x1 >= rootx1 && x2 <= rootx2 &&	y1 >= rooty1 && y2 <= rooty2) {
	/*
	 * The line is completely inside the toplevel containing
	 * tkwin. It's better to draw into this window because on some
	 * X servers, especially PC X Servers running on Windows,
	 * drawing into the root window shows no effect.
	 */
	winId = Tk_WindowId(toplevel);
	x1 -= rootx1;
	y1 -= rooty1;
	x2 -= rootx1;
	y2 -= rooty1;
    } else {
	winId = XRootWindow(Tk_Display(tkwin), Tk_ScreenNumber(tkwin));
    }

    values.foreground	  = 0xff;
    values.subwindow_mode = IncludeInferiors;
    values.function	  = GXxor;

    gc = XCreateGC(Tk_Display(tkwin), winId, valuemask, &values);
    XDrawLine(Tk_Display(tkwin), winId, gc, x1, y1, x2, y2);
    XFreeGC(Tk_Display(tkwin), gc);
}

/*
 *----------------------------------------------------------------------
 *
 * TixpDrawAnchorLines --
 *
 *	See comments near Tix_DrawAnchorLines.
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
    int n;
    int draw = 1;

    /*
     * TODO: (perf) use XDrawPoints to reduce the number of X calls.
     */
    if (w < 2 || h < 2) {
        /*
         * Area too small to show effect. Don't bother
         */
	return;
    }

    for (n=0; n<w; n++, draw = !draw) {
        if (draw) {
            XDrawPoint(display, drawable, gc, x+n, y);
        }
    }

    for (n=1; n<h; n++, draw = !draw) {
        if (draw) {
            XDrawPoint(display, drawable, gc, x+w-1, y+n);
        }
    }

    for (n=1; n<w; n++, draw = !draw) {
        if (draw) {
            XDrawPoint(display, drawable, gc, x+w-n-1, y+h-1);
        }
    }

    for (n=1; n<h-1; n++, draw = !draw) {
        if (draw) {
            XDrawPoint(display, drawable, gc, x, y+h-n-1);
        }
    }
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
 *      If you're wondering why the SubReg API is necessary at all,
 *      please consult the file tixWinDraw.c.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Some infomation is saved in subRegPtr for use by the
 *      TixpSubRegDrawXXX functions.
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
    if ((width < needWidth) || (height < needHeight)) {
	subRegPtr->rectUsed    = 1;
        subRegPtr->origX       = origX;
        subRegPtr->origY       = origY;
	subRegPtr->rect.x      = (short)x;
	subRegPtr->rect.y      = (short)y;
	subRegPtr->rect.width  = (short)width;
	subRegPtr->rect.height = (short)height;
#ifndef MAC_OSX_TK
	XSetClipRectangles(display, gc, origX, origY, &subRegPtr->rect,
		1, Unsorted);
#else
	subRegPtr->pixmap = Tk_GetPixmap(display, drawable, width, height,
		32);

	if (subRegPtr->pixmap != None) {
	    XCopyArea(display, drawable, subRegPtr->pixmap, gc, x, y,
		    width, height, 0, 0);
	}
#endif
    } else {
	subRegPtr->rectUsed    = 0;
#ifdef MAC_OSX_TK
	subRegPtr->pixmap = None;
#endif
    }
}

void
TixpSubRegSetClip(display, subRegPtr, gc)
    Display *display;
    TixpSubRegion * subRegPtr;
    GC gc;
{
#ifndef MAC_OSX_TK
    if (subRegPtr->rectUsed) {
	XSetClipRectangles(display, gc, subRegPtr->origX, subRegPtr->origY,
                &subRegPtr->rect, 1, Unsorted);
    }

#endif
}

void
TixpSubRegUnsetClip(display, subRegPtr, gc)
    Display *display;
    TixpSubRegion * subRegPtr;
    GC gc;
{
#ifndef MAC_OSX_TK
    XRectangle rect;

    if (subRegPtr->rectUsed) {
	rect.x      = 0;
	rect.y      = 0;
	rect.width  = 20000;
	rect.height = 20000;
	XSetClipRectangles(display, gc, 0, 0, &rect, 1, Unsorted);
    }
#endif
}

/*
 *----------------------------------------------------------------------
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
#ifndef MAC_OSX_TK
    TixpSubRegUnsetClip(display, subRegPtr, gc);
#else
    if (subRegPtr->pixmap != None) {
	XCopyArea(display, subRegPtr->pixmap, drawable, gc, 0, 0,
		subRegPtr->rect.width, subRegPtr->rect.height,
		subRegPtr->rect.x, subRegPtr->rect.y);
	Tk_FreePixmap(display, subRegPtr->pixmap);
	subRegPtr->pixmap = None;
    }
#endif
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
#ifdef MAC_OSX_TK
    if (subRegPtr->pixmap != None) {
	TixDisplayText(display, subRegPtr->pixmap, font, string,
		numChars, x - subRegPtr->x, y - subRegPtr->y,
		length, justify, underline, gc);
    } else 
#endif	
    {
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
#ifdef MAC_OSX_TK
    if (subRegPtr->pixmap != None) {
	XFillRectangle(display, subRegPtr->pixmap, gc,
		x - subRegPtr->x, y - subRegPtr->y, width, height);
    } else
#endif	
    {
    XFillRectangle(display, drawable, gc, x, y,
	    (unsigned) width, (unsigned) height);
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
#ifdef MAC_OSX_TK
    if (subRegPtr->pixmap != None) {
        drawableX -= subRegPtr->x;
        drawableY -= subRegPtr->y;
        Tk_RedrawImage(image, imageX, imageY, width, height, subRegPtr->pixmap,
	        drawableX, drawableY);
    } else
#endif	
    {
    if (subRegPtr->rectUsed) {
        /*
         * We need to do the clipping by hand because Tk_RedrawImage()
         * Does not take in a GC so we can't set its clip region.
         */

	if (drawableX < subRegPtr->rect.x) {
	    width  -= subRegPtr->rect.x - drawableX;
	    imageX += subRegPtr->rect.x - drawableX;
	    drawableX = subRegPtr->rect.x;
	}
	if (drawableX + width > subRegPtr->rect.x + subRegPtr->rect.width) {
	    width = subRegPtr->rect.x - drawableX + subRegPtr->rect.width;
	}

	if (drawableY < subRegPtr->rect.y) {
	    height -= subRegPtr->rect.y - drawableY;
	    imageY += subRegPtr->rect.y - drawableY;
	    drawableY = subRegPtr->rect.y;
	}
	if (drawableY + height > subRegPtr->rect.y + subRegPtr->rect.height) {
	    height = subRegPtr->rect.y - drawableY + subRegPtr->rect.height;
	}
    }

    Tk_RedrawImage(image, imageX, imageY, width, height, drawable,
	    drawableX, drawableY);
    }
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
#ifdef MAC_OSX_TK
    XSetClipOrigin(display, gc, dest_x, dest_y);
    if (subRegPtr->pixmap != None) {
	XCopyPlane(display, bitmap, subRegPtr->pixmap, gc, src_x, src_y,
		width, height, dest_x - subRegPtr->x, dest_y - subRegPtr->y,
		plane);
    } else {
#endif	
    XCopyPlane(display, bitmap, drawable, gc, src_x, src_y,
	    (unsigned) width, (unsigned) height, dest_x, dest_y, plane);
#ifdef MAC_OSX_TK
    }
    XSetClipOrigin(display, gc, 0, 0);
#endif	
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
#ifdef MAC_OSX_TK
    if (subRegPtr->pixmap != None) {
        x -= subRegPtr->x;
        y -= subRegPtr->y;
        TixpDrawAnchorLines(display, subRegPtr->pixmap, gc, x, y, w, h);
    } else
#endif	
    {
    TixpDrawAnchorLines(display, drawable, gc, x, y, w, h);
    }
}
