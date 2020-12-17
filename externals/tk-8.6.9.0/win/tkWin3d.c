/*
 * tkWin3d.c --
 *
 *	This file contains the platform specific routines for drawing 3D
 *	borders in the Windows 95 style.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"
#include "tk3d.h"

/*
 * This structure is used to keep track of the extra colors used by Windows 3D
 * borders.
 */

typedef struct {
    TkBorder info;
    XColor *light2ColorPtr;	/* System3dLight */
    XColor *dark2ColorPtr;	/* System3dDarkShadow */
} WinBorder;

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
    WinBorder *borderPtr = ckalloc(sizeof(WinBorder));

    borderPtr->light2ColorPtr = NULL;
    borderPtr->dark2ColorPtr = NULL;
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
    WinBorder *winBorderPtr = (WinBorder *) borderPtr;
    if (winBorderPtr->light2ColorPtr) {
	Tk_FreeColor(winBorderPtr->light2ColorPtr);
    }
    if (winBorderPtr->dark2ColorPtr) {
	Tk_FreeColor(winBorderPtr->dark2ColorPtr);
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
    int left, right;
    Display *display = Tk_Display(tkwin);
    TkWinDCState state;
    HDC dc = TkWinGetDrawableDC(display, drawable, &state);
    int half;

    if ((borderPtr->lightGC == None) && (relief != TK_RELIEF_FLAT)) {
	TkpGetShadows(borderPtr, tkwin);
    }

    switch (relief) {
    case TK_RELIEF_RAISED:
	left = (leftBevel)
		? borderPtr->lightGC->foreground
		: borderPtr->darkGC->foreground;
	right = (leftBevel)
		? ((WinBorder *)borderPtr)->light2ColorPtr->pixel
		: ((WinBorder *)borderPtr)->dark2ColorPtr->pixel;
	break;
    case TK_RELIEF_SUNKEN:
	left = (leftBevel)
		? borderPtr->darkGC->foreground
		: ((WinBorder *)borderPtr)->light2ColorPtr->pixel;
	right = (leftBevel)
		? ((WinBorder *)borderPtr)->dark2ColorPtr->pixel
		: borderPtr->lightGC->foreground;
	break;
    case TK_RELIEF_RIDGE:
	left = borderPtr->lightGC->foreground;
	right = borderPtr->darkGC->foreground;
	break;
    case TK_RELIEF_GROOVE:
	left = borderPtr->darkGC->foreground;
	right = borderPtr->lightGC->foreground;
	break;
    case TK_RELIEF_FLAT:
	left = right = borderPtr->bgGC->foreground;
	break;
    case TK_RELIEF_SOLID:
    default:
	left = right = RGB(0,0,0);
	break;
    }
    half = width/2;
    if (leftBevel && (width & 1)) {
	half++;
    }
    TkWinFillRect(dc, x, y, half, height, left);
    TkWinFillRect(dc, x+half, y, width-half, height, right);
    TkWinReleaseDrawableDC(drawable, dc, &state);
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
    TkWinDCState state;
    HDC dc = TkWinGetDrawableDC(display, drawable, &state);
    int topColor, bottomColor;

    if ((borderPtr->lightGC == None) && (relief != TK_RELIEF_FLAT)) {
	TkpGetShadows(borderPtr, tkwin);
    }

    /*
     * Compute a GC for the top half of the bevel and a GC for the bottom half
     * (they're the same in many cases).
     */

    switch (relief) {
    case TK_RELIEF_RAISED:
	topColor = (topBevel)
		? borderPtr->lightGC->foreground
		: borderPtr->darkGC->foreground;
	bottomColor = (topBevel)
		? ((WinBorder *)borderPtr)->light2ColorPtr->pixel
		: ((WinBorder *)borderPtr)->dark2ColorPtr->pixel;
	break;
    case TK_RELIEF_SUNKEN:
	topColor = (topBevel)
		? borderPtr->darkGC->foreground
		: ((WinBorder *)borderPtr)->light2ColorPtr->pixel;
	bottomColor = (topBevel)
		? ((WinBorder *)borderPtr)->dark2ColorPtr->pixel
		: borderPtr->lightGC->foreground;
	break;
    case TK_RELIEF_RIDGE:
	topColor = borderPtr->lightGC->foreground;
	bottomColor = borderPtr->darkGC->foreground;
	break;
    case TK_RELIEF_GROOVE:
	topColor = borderPtr->darkGC->foreground;
	bottomColor = borderPtr->lightGC->foreground;
	break;
    case TK_RELIEF_FLAT:
	topColor = bottomColor = borderPtr->bgGC->foreground;
	break;
    case TK_RELIEF_SOLID:
    default:
	topColor = bottomColor = RGB(0,0,0);
    }

    /*
     * Compute various other geometry-related stuff.
     */

    if (leftIn) {
	x1 = x+1;
    } else {
	x1 = x+height-1;
    }
    x2 = x+width;
    if (rightIn) {
	x2--;
    } else {
	x2 -= height;
    }
    x1Delta = (leftIn) ? 1 : -1;
    x2Delta = (rightIn) ? -1 : 1;
    halfway = y + height/2;
    if (topBevel && (height & 1)) {
	halfway++;
    }
    bottom = y + height;

    /*
     * Draw one line for each y-coordinate covered by the bevel.
     */

    for ( ; y < bottom; y++) {
	/*
	 * In some weird cases (such as large border widths for skinny
	 * rectangles) x1 can be >= x2. Don't draw the lines in these cases.
	 */

	if (x1 < x2) {
	    TkWinFillRect(dc, x1, y, x2-x1, 1,
		(y < halfway) ? topColor : bottomColor);
	}
	x1 += x1Delta;
	x2 += x2Delta;
    }
    TkWinReleaseDrawableDC(drawable, dc, &state);
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
    int tmp1, tmp2;
    int r, g, b;
    XGCValues gcValues;

    if (borderPtr->lightGC != None) {
	return;
    }

    /*
     * Handle the special case of the default system colors.
     */

    if ((TkWinIndexOfColor(borderPtr->bgColorPtr) == COLOR_3DFACE)
	    || (TkWinIndexOfColor(borderPtr->bgColorPtr) == COLOR_WINDOW)) {
	borderPtr->darkColorPtr = Tk_GetColor(NULL, tkwin,
		Tk_GetUid("SystemButtonShadow"));
	gcValues.foreground = borderPtr->darkColorPtr->pixel;
	borderPtr->darkGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
	borderPtr->lightColorPtr = Tk_GetColor(NULL, tkwin,
		Tk_GetUid("SystemButtonHighlight"));
	gcValues.foreground = borderPtr->lightColorPtr->pixel;
	borderPtr->lightGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
	((WinBorder*)borderPtr)->dark2ColorPtr = Tk_GetColor(NULL, tkwin,
		Tk_GetUid("System3dDarkShadow"));
	((WinBorder*)borderPtr)->light2ColorPtr = Tk_GetColor(NULL, tkwin,
		Tk_GetUid("System3dLight"));
	return;
    }
    darkColor.red = 0;
    darkColor.green = 0;
    darkColor.blue = 0;
    ((WinBorder*)borderPtr)->dark2ColorPtr = Tk_GetColorByValue(tkwin,
	    &darkColor);
    lightColor = *(borderPtr->bgColorPtr);
    ((WinBorder*)borderPtr)->light2ColorPtr = Tk_GetColorByValue(tkwin,
	    &lightColor);

    /*
     * First, handle the case of a color display with lots of colors. The
     * shadow colors get computed using whichever formula results in the
     * greatest change in color:
     * 1. Lighter shadow is half-way to white, darker shadow is half way to
     *    dark.
     * 2. Lighter shadow is 40% brighter than background, darker shadow is 40%
     *    darker than background.
     */

    if (Tk_Depth(tkwin) >= 6) {
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
	 * Compute the dark shadow color
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
	 * Allocate the dark shadow color and its GC
	 */

	borderPtr->darkColorPtr = Tk_GetColorByValue(tkwin, &darkColor);
	gcValues.foreground = borderPtr->darkColorPtr->pixel;
	borderPtr->darkGC = Tk_GetGC(tkwin, GCForeground, &gcValues);

	/*
	 * Compute the light shadow color
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
	 * Allocate the light shadow color and its GC
	 */

	borderPtr->lightColorPtr = Tk_GetColorByValue(tkwin, &lightColor);
	gcValues.foreground = borderPtr->lightColorPtr->pixel;
	borderPtr->lightGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
	return;
    }

    if (borderPtr->shadow == None) {
	borderPtr->shadow = Tk_GetBitmap((Tcl_Interp *) NULL, tkwin,
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
	gcValues.foreground = WhitePixelOfScreen(borderPtr->screen);
	gcValues.background = borderPtr->bgColorPtr->pixel;
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
 *----------------------------------------------------------------------
 *
 * TkWinGetBorderPixels --
 *
 *	This routine returns the 5 COLORREFs used to draw a given 3d border.
 *
 * Results:
 *	Returns the colors in the specified array.
 *
 * Side effects:
 *	May cause the remaining colors to be allocated.
 *
 *----------------------------------------------------------------------
 */

COLORREF
TkWinGetBorderPixels(
    Tk_Window tkwin,
    Tk_3DBorder border,
    int which)			/* One of TK_3D_FLAT_GC, TK_3D_LIGHT_GC,
				 * TK_3D_DARK_GC, TK_3D_LIGHT2, TK_3D_DARK2 */
{
    WinBorder *borderPtr = (WinBorder *) border;

    if (borderPtr->info.lightGC == None) {
	TkpGetShadows(&borderPtr->info, tkwin);
    }
    switch (which) {
    case TK_3D_FLAT_GC:
	return borderPtr->info.bgColorPtr->pixel;
    case TK_3D_LIGHT_GC:
	if (borderPtr->info.lightColorPtr == NULL) {
	    return WhitePixelOfScreen(borderPtr->info.screen);
	}
	return borderPtr->info.lightColorPtr->pixel;
    case TK_3D_DARK_GC:
	if (borderPtr->info.darkColorPtr == NULL) {
	    return BlackPixelOfScreen(borderPtr->info.screen);
	}
	return borderPtr->info.darkColorPtr->pixel;
    case TK_3D_LIGHT2:
	return borderPtr->light2ColorPtr->pixel;
    case TK_3D_DARK2:
	return borderPtr->dark2ColorPtr->pixel;
    }
    return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
