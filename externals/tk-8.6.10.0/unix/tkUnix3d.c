/*
 * tkUnix3d.c --
 *
 *	This file contains the platform specific routines for drawing 3d
 *	borders in the Motif style.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tk3d.h"

#if !(defined(_WIN32) || defined(MAC_OSX_TK))
#include "tkUnixInt.h"
#endif

/*
 * This structure is used to keep track of the extra colors used by Unix 3D
 * borders.
 */

typedef struct {
    TkBorder info;
    GC solidGC;		/* Used to draw solid relief. */
} UnixBorder;

/*
 *----------------------------------------------------------------------
 *
 * TkpGetBorder --
 *
 *	This function allocates a new TkBorder structure.
 *
 * Results:
 *	Returns a newly allocated TkBorder.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkBorder *
TkpGetBorder(void)
{
    UnixBorder *borderPtr = ckalloc(sizeof(UnixBorder));

    borderPtr->solidGC = NULL;
    return (TkBorder *) borderPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpFreeBorder --
 *
 *	This function frees any colors allocated by the platform specific part
 *	of this module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May deallocate some colors.
 *
 *----------------------------------------------------------------------
 */

void
TkpFreeBorder(
    TkBorder *borderPtr)
{
    UnixBorder *unixBorderPtr = (UnixBorder *) borderPtr;
    Display *display = DisplayOfScreen(borderPtr->screen);

    if (unixBorderPtr->solidGC != NULL) {
	Tk_FreeGC(display, unixBorderPtr->solidGC);
    }
}
/*
 *--------------------------------------------------------------
 *
 * Tk_3DVerticalBevel --
 *
 *	This procedure draws a vertical bevel along one side of an object. The
 *	bevel is always rectangular in shape:
 *			|||
 *			|||
 *			|||
 *			|||
 *			|||
 *			|||
 *	An appropriate shadow color is chosen for the bevel based on the
 *	leftBevel and relief arguments. Normally this procedure is called
 *	first, then Tk_3DHorizontalBevel is called next to draw neat corners.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Graphics are drawn in drawable.
 *
 *--------------------------------------------------------------
 */

void
Tk_3DVerticalBevel(
    Tk_Window tkwin,		/* Window for which border was allocated. */
    Drawable drawable,		/* X window or pixmap in which to draw. */
    Tk_3DBorder border,		/* Token for border to draw. */
    int x, int y, int width, int height,
				/* Area of vertical bevel. */
    int leftBevel,		/* Non-zero means this bevel forms the left
				 * side of the object; 0 means it forms the
				 * right side. */
    int relief)			/* Kind of bevel to draw. For example,
				 * TK_RELIEF_RAISED means interior of object
				 * should appear higher than exterior. */
{
    TkBorder *borderPtr = (TkBorder *) border;
    GC left, right;
    Display *display = Tk_Display(tkwin);

    if ((borderPtr->lightGC == NULL) && (relief != TK_RELIEF_FLAT)) {
	TkpGetShadows(borderPtr, tkwin);
    }

    if (relief == TK_RELIEF_RAISED) {
	XFillRectangle(display, drawable,
		(leftBevel) ? borderPtr->lightGC : borderPtr->darkGC,
		x, y, (unsigned) width, (unsigned) height);
    } else if (relief == TK_RELIEF_SUNKEN) {
	XFillRectangle(display, drawable,
		(leftBevel) ? borderPtr->darkGC : borderPtr->lightGC,
		x, y, (unsigned) width, (unsigned) height);
    } else if (relief == TK_RELIEF_RIDGE) {
	int half;

	left = borderPtr->lightGC;
	right = borderPtr->darkGC;
    ridgeGroove:
	half = width/2;
	if (!leftBevel && (width & 1)) {
	    half++;
	}
	XFillRectangle(display, drawable, left, x, y, (unsigned) half,
		(unsigned) height);
	XFillRectangle(display, drawable, right, x+half, y,
		(unsigned) (width-half), (unsigned) height);
    } else if (relief == TK_RELIEF_GROOVE) {
	left = borderPtr->darkGC;
	right = borderPtr->lightGC;
	goto ridgeGroove;
    } else if (relief == TK_RELIEF_FLAT) {
	XFillRectangle(display, drawable, borderPtr->bgGC, x, y,
		(unsigned) width, (unsigned) height);
    } else if (relief == TK_RELIEF_SOLID) {
	UnixBorder *unixBorderPtr = (UnixBorder *) borderPtr;
	if (unixBorderPtr->solidGC == NULL) {
	    XGCValues gcValues;

	    gcValues.foreground = BlackPixelOfScreen(borderPtr->screen);
	    unixBorderPtr->solidGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
	}
	XFillRectangle(display, drawable, unixBorderPtr->solidGC, x, y,
		(unsigned) width, (unsigned) height);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_3DHorizontalBevel --
 *
 *	This procedure draws a horizontal bevel along one side of an object.
 *	The bevel has mitered corners (depending on leftIn and rightIn
 *	arguments).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
Tk_3DHorizontalBevel(
    Tk_Window tkwin,		/* Window for which border was allocated. */
    Drawable drawable,		/* X window or pixmap in which to draw. */
    Tk_3DBorder border,		/* Token for border to draw. */
    int x, int y, int width, int height,
				/* Bounding box of area of bevel. Height gives
				 * width of border. */
    int leftIn, int rightIn,	/* Describes whether the left and right edges
				 * of the bevel angle in or out as they go
				 * down. For example, if "leftIn" is true, the
				 * left side of the bevel looks like this:
				 *	___________
				 *	 __________
				 *	  _________
				 *	   ________
				 */
    int topBevel,		/* Non-zero means this bevel forms the top
				 * side of the object; 0 means it forms the
				 * bottom side. */
    int relief)			/* Kind of bevel to draw. For example,
				 * TK_RELIEF_RAISED means interior of object
				 * should appear higher than exterior. */
{
    TkBorder *borderPtr = (TkBorder *) border;
    Display *display = Tk_Display(tkwin);
    int bottom, halfway, x1, x2, x1Delta, x2Delta;
    UnixBorder *unixBorderPtr = (UnixBorder *) borderPtr;
    GC topGC = NULL, bottomGC = NULL;
				/* Initializations needed only to prevent
				 * compiler warnings. */

    if ((borderPtr->lightGC == NULL) && (relief != TK_RELIEF_FLAT) &&
	    (relief != TK_RELIEF_SOLID)) {
	TkpGetShadows(borderPtr, tkwin);
    }

    /*
     * Compute a GC for the top half of the bevel and a GC for the bottom half
     * (they're the same in many cases).
     */

    switch (relief) {
    case TK_RELIEF_FLAT:
	topGC = bottomGC = borderPtr->bgGC;
	break;
    case TK_RELIEF_GROOVE:
	topGC = borderPtr->darkGC;
	bottomGC = borderPtr->lightGC;
	break;
    case TK_RELIEF_RAISED:
	topGC = bottomGC = (topBevel? borderPtr->lightGC : borderPtr->darkGC);
	break;
    case TK_RELIEF_RIDGE:
	topGC = borderPtr->lightGC;
	bottomGC = borderPtr->darkGC;
	break;
    case TK_RELIEF_SOLID:
	if (unixBorderPtr->solidGC == NULL) {
	    XGCValues gcValues;

	    gcValues.foreground = BlackPixelOfScreen(borderPtr->screen);
	    unixBorderPtr->solidGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
	}
	XFillRectangle(display, drawable, unixBorderPtr->solidGC, x, y,
		(unsigned) width, (unsigned) height);
	return;
    case TK_RELIEF_SUNKEN:
	topGC = bottomGC = (topBevel? borderPtr->darkGC : borderPtr->lightGC);
	break;
    }

    /*
     * Compute various other geometry-related stuff.
     */

    x1 = x;
    if (!leftIn) {
	x1 += height;
    }
    x2 = x+width;
    if (!rightIn) {
	x2 -= height;
    }
    x1Delta = (leftIn) ? 1 : -1;
    x2Delta = (rightIn) ? -1 : 1;
    halfway = y + height/2;
    if (!topBevel && (height & 1)) {
	halfway++;
    }
    bottom = y + height;

    /*
     * Draw one line for each y-coordinate covered by the bevel.
     */

    for ( ; y < bottom; y++) {
	/*
	 * X Dimensions are 16-bit, so avoid wraparound or display errors by
	 * limiting these here.
	 */

	if (x1 < -32767) {
	    x1 = -32767;
	}
	if (x2 > 32767) {
	    x2 = 32767;
	}

	/*
	 * In some weird cases (such as large border widths for skinny
	 * rectangles) x1 can be >= x2. Don't draw the lines in these cases.
	 */

	if (x1 < x2) {
	    XFillRectangle(display, drawable,
		    (y < halfway) ? topGC : bottomGC, x1, y,
		    (unsigned) (x2-x1), (unsigned) 1);
	}
	x1 += x1Delta;
	x2 += x2Delta;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetShadows --
 *
 *	This procedure computes the shadow colors for a 3-D border and fills
 *	in the corresponding fields of the Border structure. It's called
 *	lazily, so that the colors aren't allocated until something is
 *	actually drawn with them. That way, if a border is only used for flat
 *	backgrounds the shadow colors will never be allocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lightGC and darkGC fields in borderPtr get filled in, if they
 *	weren't already.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetShadows(
    TkBorder *borderPtr,	/* Information about border. */
    Tk_Window tkwin)		/* Window where border will be used for
				 * drawing. */
{
    XColor lightColor, darkColor;
    int stressed, tmp1, tmp2;
    int r, g, b;
    XGCValues gcValues;

    if (borderPtr->lightGC != NULL) {
	return;
    }
    stressed = TkpCmapStressed(tkwin, borderPtr->colormap);

    /*
     * First, handle the case of a color display with lots of colors. The
     * shadow colors get computed using whichever formula results in the
     * greatest change in color:
     * 1. Lighter shadow is half-way to white, darker shadow is half way to
     *    dark.
     * 2. Lighter shadow is 40% brighter than background, darker shadow is 40%
     *    darker than background.
     */

    if (!stressed && (Tk_Depth(tkwin) >= 6)) {
	/*
	 * This is a color display with lots of colors. For the dark shadow,
	 * cut 40% from each of the background color components. But if the
	 * background is already very dark, make the dark color a little
	 * lighter than the background by increasing each color component
	 * 1/4th of the way to MAX_INTENSITY.
	 *
	 * For the light shadow, boost each component by 40% or half-way to
	 * white, whichever is greater (the first approach works better for
	 * unsaturated colors, the second for saturated ones). But if the
	 * background is already very bright, instead choose a slightly darker
	 * color for the light shadow by reducing each color component by 10%.
	 *
	 * Compute the colors using integers, not using lightColor.red etc.:
	 * these are shorts and may have problems with integer overflow.
	 */

	/*
	 * Compute the dark shadow color.
	 */

	r = (int) borderPtr->bgColorPtr->red;
	g = (int) borderPtr->bgColorPtr->green;
	b = (int) borderPtr->bgColorPtr->blue;

	if (r*0.5*r + g*1.0*g + b*0.28*b < MAX_INTENSITY*0.05*MAX_INTENSITY) {
	    darkColor.red = (MAX_INTENSITY + 3*r)/4;
	    darkColor.green = (MAX_INTENSITY + 3*g)/4;
	    darkColor.blue = (MAX_INTENSITY + 3*b)/4;
	} else {
	    darkColor.red = (60 * r)/100;
	    darkColor.green = (60 * g)/100;
	    darkColor.blue = (60 * b)/100;
	}

	/*
	 * Allocate the dark shadow color and its GC.
	 */

	borderPtr->darkColorPtr = Tk_GetColorByValue(tkwin, &darkColor);
	gcValues.foreground = borderPtr->darkColorPtr->pixel;
	borderPtr->darkGC = Tk_GetGC(tkwin, GCForeground, &gcValues);

	/*
	 * Compute the light shadow color.
	 */

	if (g > MAX_INTENSITY*0.95) {
	    lightColor.red = (90 * r)/100;
	    lightColor.green = (90 * g)/100;
	    lightColor.blue = (90 * b)/100;
	} else {
	    tmp1 = (14 * r)/10;
	    if (tmp1 > MAX_INTENSITY) {
		tmp1 = MAX_INTENSITY;
	    }
	    tmp2 = (MAX_INTENSITY + r)/2;
	    lightColor.red = (tmp1 > tmp2) ? tmp1 : tmp2;
	    tmp1 = (14 * g)/10;
	    if (tmp1 > MAX_INTENSITY) {
		tmp1 = MAX_INTENSITY;
	    }
	    tmp2 = (MAX_INTENSITY + g)/2;
	    lightColor.green = (tmp1 > tmp2) ? tmp1 : tmp2;
	    tmp1 = (14 * b)/10;
	    if (tmp1 > MAX_INTENSITY) {
		tmp1 = MAX_INTENSITY;
	    }
	    tmp2 = (MAX_INTENSITY + b)/2;
	    lightColor.blue = (tmp1 > tmp2) ? tmp1 : tmp2;
	}

	/*
	 * Allocate the light shadow color and its GC.
	 */

	borderPtr->lightColorPtr = Tk_GetColorByValue(tkwin, &lightColor);
	gcValues.foreground = borderPtr->lightColorPtr->pixel;
	borderPtr->lightGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
	return;
    }

    if (borderPtr->shadow == None) {
	borderPtr->shadow = Tk_GetBitmap(NULL, tkwin,
		Tk_GetUid("gray50"));
	if (borderPtr->shadow == None) {
	    Tcl_Panic("TkpGetShadows couldn't allocate bitmap for border");
	}
    }
    if (borderPtr->visual->map_entries > 2) {
	/*
	 * This isn't a monochrome display, but the colormap either ran out of
	 * entries or didn't have very many to begin with. Generate the light
	 * shadows with a white stipple and the dark shadows with a black
	 * stipple.
	 */

	gcValues.foreground = borderPtr->bgColorPtr->pixel;
	gcValues.background = BlackPixelOfScreen(borderPtr->screen);
	gcValues.stipple = borderPtr->shadow;
	gcValues.fill_style = FillOpaqueStippled;
	borderPtr->darkGC = Tk_GetGC(tkwin,
		GCForeground|GCBackground|GCStipple|GCFillStyle, &gcValues);
	gcValues.background = WhitePixelOfScreen(borderPtr->screen);
	borderPtr->lightGC = Tk_GetGC(tkwin,
		GCForeground|GCBackground|GCStipple|GCFillStyle, &gcValues);
	return;
    }

    /*
     * This is just a measly monochrome display, hardly even worth its
     * existence on this earth. Make one shadow a 50% stipple and the other
     * the opposite of the background.
     */

    gcValues.foreground = WhitePixelOfScreen(borderPtr->screen);
    gcValues.background = BlackPixelOfScreen(borderPtr->screen);
    gcValues.stipple = borderPtr->shadow;
    gcValues.fill_style = FillOpaqueStippled;
    borderPtr->lightGC = Tk_GetGC(tkwin,
	    GCForeground|GCBackground|GCStipple|GCFillStyle, &gcValues);
    if (borderPtr->bgColorPtr->pixel
	    == WhitePixelOfScreen(borderPtr->screen)) {
	gcValues.foreground = BlackPixelOfScreen(borderPtr->screen);
	borderPtr->darkGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
    } else {
	borderPtr->darkGC = borderPtr->lightGC;
	borderPtr->lightGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
