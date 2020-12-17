/*
 * tkUnixColor.c --
 *
 *	This file contains the platform specific color routines needed for X
 *	support.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"
#include "tkColor.h"

/*
 * If a colormap fills up, attempts to allocate new colors from that colormap
 * will fail. When that happens, we'll just choose the closest color from
 * those that are available in the colormap. One of the following structures
 * will be created for each "stressed" colormap to keep track of the colors
 * that are available in the colormap (otherwise we would have to re-query
 * from the server on each allocation, which would be very slow). These
 * entries are flushed after a few seconds, since other clients may release or
 * reallocate colors over time.
 */

struct TkStressedCmap {
    Colormap colormap;		/* X's token for the colormap. */
    int numColors;		/* Number of entries currently active at
				 * *colorPtr. */
    XColor *colorPtr;		/* Pointer to malloc'ed array of all colors
				 * that seem to be available in the colormap.
				 * Some may not actually be available, e.g.
				 * because they are read-write for another
				 * client; when we find this out, we remove
				 * them from the array. */
    struct TkStressedCmap *nextPtr;
				/* Next in list of all stressed colormaps for
				 * the display. */
};

/*
 * Forward declarations for functions defined in this file:
 */

static void		DeleteStressedCmap(Display *display,
			    Colormap colormap);
static void		FindClosestColor(Tk_Window tkwin,
			    XColor *desiredColorPtr, XColor *actualColorPtr);

/*
 *----------------------------------------------------------------------
 *
 * TkpFreeColor --
 *
 *	Release the specified color back to the system.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Invalidates the colormap cache for the colormap associated with the
 *	given color.
 *
 *----------------------------------------------------------------------
 */

void
TkpFreeColor(
    TkColor *tkColPtr)		/* Color to be released. Must have been
				 * allocated by TkpGetColor or
				 * TkpGetColorByValue. */
{
    Visual *visual;
    Screen *screen = tkColPtr->screen;

    /*
     * Careful! Don't free black or white, since this will make some servers
     * very unhappy. Also, there is a bug in some servers (such Sun's X11/NeWS
     * server) where reference counting is performed incorrectly, so that if a
     * color is allocated twice in different places and then freed twice, the
     * second free generates an error (this bug existed as of 10/1/92). To get
     * around this problem, ignore errors that occur during the free
     * operation.
     */

    visual = tkColPtr->visual;
    if ((visual->c_class != StaticGray) && (visual->c_class != StaticColor)
	    && (tkColPtr->color.pixel != BlackPixelOfScreen(screen))
	    && (tkColPtr->color.pixel != WhitePixelOfScreen(screen))) {
	Tk_ErrorHandler handler;

	handler = Tk_CreateErrorHandler(DisplayOfScreen(screen),
		-1, -1, -1, NULL, NULL);
	XFreeColors(DisplayOfScreen(screen), tkColPtr->colormap,
		&tkColPtr->color.pixel, 1, 0L);
	Tk_DeleteErrorHandler(handler);
    }
    DeleteStressedCmap(DisplayOfScreen(screen), tkColPtr->colormap);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColor --
 *
 *	Allocate a new TkColor for the color with the given name.
 *
 * Results:
 *	Returns a newly allocated TkColor, or NULL on failure.
 *
 * Side effects:
 *	May invalidate the colormap cache associated with tkwin upon
 *	allocating a new colormap entry. Allocates a new TkColor structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColor(
    Tk_Window tkwin,		/* Window in which color will be used. */
    Tk_Uid name)		/* Name of color to allocated (in form
				 * suitable for passing to XParseColor). */
{
    Display *display = Tk_Display(tkwin);
    Colormap colormap = Tk_Colormap(tkwin);
    XColor color;
    TkColor *tkColPtr;

    /*
     * Map from the name to a pixel value. Call XAllocNamedColor rather than
     * XParseColor for non-# names: this saves a server round-trip for those
     * names.
     */

    if (*name != '#') {
	XColor screen;

	if (((*name - 'A') & 0xdf) < sizeof(tkWebColors)/sizeof(tkWebColors[0])) {
	    if (!((name[0] - 'G') & 0xdf) && !((name[1] - 'R') & 0xdf)
		    && !((name[2] - 'A') & 0xdb) && !((name[3] - 'Y') & 0xdf)
		    && !name[4]) {
		name = "#808080808080";
		goto gotWebColor;
	    } else {
		const char *p = tkWebColors[((*name - 'A') & 0x1f)];
		if (p) {
		    const char *q = name;
		    while (!((*p - *(++q)) & 0xdf)) {
			if (!*p++) {
			    name = p;
			    goto gotWebColor;
			}
		    }
		}
	}
	}
	if (strlen(name) > 99) {
	/* Don't bother to parse this. [Bug 2809525]*/
	return NULL;
    } else if (XAllocNamedColor(display, colormap, name, &screen, &color) != 0) {
	    DeleteStressedCmap(display, colormap);
	} else {
	    /*
	     * Couldn't allocate the color. Try translating the name to a
	     * color value, to see whether the problem is a bad color name or
	     * a full colormap. If the colormap is full, then pick an
	     * approximation to the desired color.
	     */

	    if (XLookupColor(display, colormap, name, &color, &screen) == 0) {
		return NULL;
	    }
	    FindClosestColor(tkwin, &screen, &color);
	}
    } else {
    gotWebColor:
	if (TkParseColor(display, colormap, name, &color) == 0) {
	    return NULL;
	}
	if (XAllocColor(display, colormap, &color) != 0) {
	    DeleteStressedCmap(display, colormap);
	} else {
	    FindClosestColor(tkwin, &color, &color);
	}
    }

    tkColPtr = ckalloc(sizeof(TkColor));
    tkColPtr->color = color;

    return tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColorByValue --
 *
 *	Given a desired set of red-green-blue intensities for a color, locate
 *	a pixel value to use to draw that color in a given window.
 *
 * Results:
 *	The return value is a pointer to an TkColor structure that indicates
 *	the closest red, blue, and green intensities available to those
 *	specified in colorPtr, and also specifies a pixel value to use to draw
 *	in that color.
 *
 * Side effects:
 *	May invalidate the colormap cache for the specified window. Allocates
 *	a new TkColor structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColorByValue(
    Tk_Window tkwin,		/* Window in which color will be used. */
    XColor *colorPtr)		/* Red, green, and blue fields indicate
				 * desired color. */
{
    Display *display = Tk_Display(tkwin);
    Colormap colormap = Tk_Colormap(tkwin);
    TkColor *tkColPtr = ckalloc(sizeof(TkColor));

    tkColPtr->color.red = colorPtr->red;
    tkColPtr->color.green = colorPtr->green;
    tkColPtr->color.blue = colorPtr->blue;
    if (XAllocColor(display, colormap, &tkColPtr->color) != 0) {
	DeleteStressedCmap(display, colormap);
    } else {
	FindClosestColor(tkwin, &tkColPtr->color, &tkColPtr->color);
    }

    return tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FindClosestColor --
 *
 *	When Tk can't allocate a color because a colormap has filled up, this
 *	function is called to find and allocate the closest available color in
 *	the colormap.
 *
 * Results:
 *	There is no return value, but *actualColorPtr is filled in with
 *	information about the closest available color in tkwin's colormap.
 *	This color has been allocated via X, so it must be released by the
 *	caller when the caller is done with it.
 *
 * Side effects:
 *	A color is allocated.
 *
 *----------------------------------------------------------------------
 */

static void
FindClosestColor(
    Tk_Window tkwin,		/* Window where color will be used. */
    XColor *desiredColorPtr,	/* RGB values of color that was wanted (but
				 * unavailable). */
    XColor *actualColorPtr)	/* Structure to fill in with RGB and pixel for
				 * closest available color. */
{
    TkStressedCmap *stressPtr;
    double tmp, distance, closestDistance;
    int i, closest, numFound;
    XColor *colorPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    Colormap colormap = Tk_Colormap(tkwin);
    XVisualInfo template, *visInfoPtr;

    /*
     * Find the TkStressedCmap structure for this colormap, or create a new
     * one if needed.
     */

    for (stressPtr = dispPtr->stressPtr; ; stressPtr = stressPtr->nextPtr) {
	if (stressPtr == NULL) {
	    stressPtr = ckalloc(sizeof(TkStressedCmap));
	    stressPtr->colormap = colormap;
	    template.visualid = XVisualIDFromVisual(Tk_Visual(tkwin));

	    visInfoPtr = XGetVisualInfo(Tk_Display(tkwin),
		    VisualIDMask, &template, &numFound);
	    if (numFound < 1) {
		Tcl_Panic("FindClosestColor couldn't lookup visual");
	    }

	    stressPtr->numColors = visInfoPtr->colormap_size;
	    XFree((char *) visInfoPtr);
	    stressPtr->colorPtr =
		    ckalloc(stressPtr->numColors * sizeof(XColor));
	    for (i = 0; i < stressPtr->numColors; i++) {
		stressPtr->colorPtr[i].pixel = (unsigned long) i;
	    }

	    XQueryColors(dispPtr->display, colormap, stressPtr->colorPtr,
		    stressPtr->numColors);

	    stressPtr->nextPtr = dispPtr->stressPtr;
	    dispPtr->stressPtr = stressPtr;
	    break;
	}
	if (stressPtr->colormap == colormap) {
	    break;
	}
    }

    /*
     * Find the color that best approximates the desired one, then try to
     * allocate that color. If that fails, it must mean that the color was
     * read-write (so we can't use it, since it's owner might change it) or
     * else it was already freed. Try again, over and over again, until
     * something succeeds.
     */

    while (1) {
	if (stressPtr->numColors == 0) {
	    Tcl_Panic("FindClosestColor ran out of colors");
	}
	closestDistance = 1e30;
	closest = 0;
	for (colorPtr = stressPtr->colorPtr, i = 0; i < stressPtr->numColors;
		colorPtr++, i++) {
	    /*
	     * Use Euclidean distance in RGB space, weighted by Y (of YIQ) as
	     * the objective function; this accounts for differences in the
	     * color sensitivity of the eye.
	     */

	    tmp = .30*(((int) desiredColorPtr->red) - (int) colorPtr->red);
	    distance = tmp*tmp;
	    tmp = .61*(((int) desiredColorPtr->green) - (int) colorPtr->green);
	    distance += tmp*tmp;
	    tmp = .11*(((int) desiredColorPtr->blue) - (int) colorPtr->blue);
	    distance += tmp*tmp;
	    if (distance < closestDistance) {
		closest = i;
		closestDistance = distance;
	    }
	}
	if (XAllocColor(dispPtr->display, colormap,
		&stressPtr->colorPtr[closest]) != 0) {
	    *actualColorPtr = stressPtr->colorPtr[closest];
	    return;
	}

	/*
	 * Couldn't allocate the color. Remove it from the table and go back
	 * to look for the next best color.
	 */

	stressPtr->colorPtr[closest] =
		stressPtr->colorPtr[stressPtr->numColors-1];
	stressPtr->numColors -= 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteStressedCmap --
 *
 *	This function releases the information cached for "colormap" so that
 *	it will be refetched from the X server the next time it is needed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TkStressedCmap structure for colormap is deleted; the colormap is
 *	no longer considered to be "stressed".
 *
 * Note:
 *	This function is invoked whenever a color in a colormap is freed, and
 *	whenever a color allocation in a colormap succeeds. This guarantees
 *	that TkStressedCmap structures are always deleted before the
 *	corresponding Colormap is freed.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteStressedCmap(
    Display *display,		/* Xlib's handle for the display containing
				 * the colormap. */
    Colormap colormap)		/* Colormap to flush. */
{
    TkStressedCmap *prevPtr, *stressPtr;
    TkDisplay *dispPtr = TkGetDisplay(display);

    for (prevPtr = NULL, stressPtr = dispPtr->stressPtr; stressPtr != NULL;
	    prevPtr = stressPtr, stressPtr = stressPtr->nextPtr) {
	if (stressPtr->colormap == colormap) {
	    if (prevPtr == NULL) {
		dispPtr->stressPtr = stressPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = stressPtr->nextPtr;
	    }
	    ckfree(stressPtr->colorPtr);
	    ckfree(stressPtr);
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCmapStressed --
 *
 *	Check to see whether a given colormap is known to be out of entries.
 *
 * Results:
 *	1 is returned if "colormap" is stressed (i.e. it has run out of
 *	entries recently), 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpCmapStressed(
    Tk_Window tkwin,		/* Window that identifies the display
				 * containing the colormap. */
    Colormap colormap)		/* Colormap to check for stress. */
{
    TkStressedCmap *stressPtr;

    for (stressPtr = ((TkWindow *) tkwin)->dispPtr->stressPtr;
	    stressPtr != NULL; stressPtr = stressPtr->nextPtr) {
	if (stressPtr->colormap == colormap) {
	    return 1;
	}
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
