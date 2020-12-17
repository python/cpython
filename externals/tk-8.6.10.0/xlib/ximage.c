/*
 * ximage.c --
 *
 *	X bitmap and image routines.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 *----------------------------------------------------------------------
 *
 * XCreateBitmapFromData --
 *
 *	Construct a single plane pixmap from bitmap data.
 *
 *	NOTE: This procedure has the correct behavior on Windows and the
 *	Macintosh, but not on UNIX. This is probably because the emulation for
 *	XPutImage on those platforms compensates for whatever is wrong here
 *	:-)
 *
 * Results:
 *	Returns a new Pixmap.
 *
 * Side effects:
 *	Allocates a new bitmap and drawable.
 *
 *----------------------------------------------------------------------
 */

Pixmap
XCreateBitmapFromData(
    Display *display,
    Drawable d,
    _Xconst char *data,
    unsigned int width,
    unsigned int height)
{
    XImage *ximage;
    GC gc;
    Pixmap pix;

    pix = Tk_GetPixmap(display, d, (int) width, (int) height, 1);
    gc = XCreateGC(display, pix, 0, NULL);
    if (gc == NULL) {
	return None;
    }
    ximage = XCreateImage(display, NULL, 1, XYBitmap, 0, (char*) data, width,
	    height, 8, (width + 7) / 8);
    ximage->bitmap_bit_order = LSBFirst;
    _XInitImageFuncPtrs(ximage);
    TkPutImage(NULL, 0, display, pix, gc, ximage, 0, 0, 0, 0, width, height);
    ximage->data = NULL;
    XDestroyImage(ximage);
    XFreeGC(display, gc);
    return pix;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
