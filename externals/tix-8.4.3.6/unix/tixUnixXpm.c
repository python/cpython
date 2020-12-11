
/*	$Id: tixUnixXpm.c,v 1.4 2006/11/15 22:18:36 hobbs Exp $	*/

/*
 * tixUnixImgXpm.c --
 *
 *	Implement the Windows specific function calls for the pixmap
 *	image type.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixUnixInt.h>
#include <tixImgXpm.h>
#include <X11/Xutil.h>

#if !defined(WIN32) && !defined(MAC_OSX_TK)
#define TkPutImage(colors, ncolors, display, pixels, gc, image, destx, desty, srcx, srcy, width, height) \
	XPutImage(display, pixels, gc, image, destx, desty, srcx, \
	srcy, width, height);
#endif

typedef struct PixmapData {
    Pixmap mask;		/* Mask: only display pixmap pixels where
				 * there are 1's here. */
    GC gc;			/* Graphics context for displaying pixmap.
				 * None means there was an error while
				 * setting up the instance, so it cannot
				 * be displayed. */
} PixmapData;


/*----------------------------------------------------------------------
 * TixpInitPixmapInstance --
 *
 *	Initializes the platform-specific data of a pixmap instance
 *
 *----------------------------------------------------------------------
 */

void
TixpInitPixmapInstance(masterPtr, instancePtr)
    PixmapMaster *masterPtr;	/* Pointer to master for image. */
    PixmapInstance *instancePtr;/* The pixmap instance. */
{
    PixmapData * dataPtr;

    dataPtr = (PixmapData *)ckalloc(sizeof(PixmapData));
    dataPtr->mask = None;
    dataPtr->gc = None;

    instancePtr->clientData = (ClientData)dataPtr;
}

#ifdef MAC_OSX_TK
static int
MacPutPixel(image, x, y, pixel)
    XImage *image;
    int x, y;
    unsigned long pixel;
{
    unsigned char *destPtr = &(image->data[(y * image->bytes_per_line)
	    + ((x * image->bits_per_pixel) / NBBY)]);
    int i=0;
	
    switch  (image->bits_per_pixel) {
	case 32:

	    destPtr[i++] = 0;
	case 24:

	    destPtr[i++] = (unsigned char) ((pixel >> 16) & 0xff);
	    destPtr[i++] = (unsigned char) ((pixel >> 8) & 0xff);
	    destPtr[i++] = (unsigned char) (pixel & 0xff);
	    break;
    }
    return 0;
}
#endif
/*----------------------------------------------------------------------
 * TixpXpmAllocTmpBuffer --
 *
 *	Allocate a temporary space to draw the image.
 *
 *----------------------------------------------------------------------
 */

void
TixpXpmAllocTmpBuffer(masterPtr, instancePtr, imagePtr, maskPtr)
    PixmapMaster * masterPtr;
    PixmapInstance * instancePtr;
    XImage ** imagePtr;
    XImage ** maskPtr;
{
    int pad;
    XImage * image = NULL, * mask = NULL;
    Display *display = Tk_Display(instancePtr->tkwin);
    int depth;

    depth = Tk_Depth(instancePtr->tkwin);

    if (depth > 16) {
	pad = 32;
    } else if (depth > 8) {
	pad = 16;
    } else {
	pad = 8;
    }

    /*
     * Create the XImage structures to store the temporary image
     */
    image = XCreateImage(display, Tk_Visual(instancePtr->tkwin),
	    (unsigned) depth, ZPixmap, 0, 0,
	    (unsigned) masterPtr->size[0], (unsigned) masterPtr->size[1],
	    pad, 0);
    image->data =
      (char *)ckalloc((unsigned) image->bytes_per_line * masterPtr->size[1]);
#ifdef MAC_OSX_TK
    image->f.put_pixel = MacPutPixel;
#endif

    mask  = XCreateImage(display, Tk_Visual(instancePtr->tkwin),
	    1, XYPixmap, 0, 0,
	    (unsigned) masterPtr->size[0], (unsigned) masterPtr->size[1],
	    pad, 0);

    mask->data =
	(char *)ckalloc((unsigned) mask->bytes_per_line  * masterPtr->size[1]);
#ifdef MAC_OSX_TK
    mask->f.put_pixel = MacPutPixel;
#endif

    *imagePtr = image;
    *maskPtr = mask;
}

void
TixpXpmFreeTmpBuffer(masterPtr, instancePtr, image, mask)
    PixmapMaster * masterPtr;
    PixmapInstance * instancePtr;
    XImage * image;
    XImage * mask;
{
    if (image) {
	ckfree((char*)image->data);
	image->data = NULL;
	XDestroyImage(image);
    }
    if (mask) {
	ckfree((char*)mask->data);
	mask->data = NULL;
	XDestroyImage(mask);
    }
}

/*----------------------------------------------------------------------
 * TixpXpmSetPixel --
 *
 *	Sets the pixel at the given (x,y) coordinate to be the given
 *	color.
 *----------------------------------------------------------------------
 */
void
TixpXpmSetPixel(instancePtr, image, mask, x, y, colorPtr, isTranspPtr)
    PixmapInstance * instancePtr;
    XImage * image;
    XImage * mask;
    int x;
    int y;
    XColor * colorPtr;
    int * isTranspPtr;
{
    if (colorPtr != NULL) {
	XPutPixel(image, x, y, colorPtr->pixel);
	XPutPixel(mask,  x, y, 1);
    } else {
	XPutPixel(mask,  x, y, 0);
	*isTranspPtr = 1;
    }
}

/*----------------------------------------------------------------------
 * TixpXpmRealizePixmap --
 *
 *	On Unix: 	Create the pixmap from the buffer.
 *	On Windows:	Free the mask if there are no transparent pixels.
 *----------------------------------------------------------------------
 */

void
TixpXpmRealizePixmap(masterPtr, instancePtr, image, mask, isTransp)
    PixmapMaster * masterPtr;
    PixmapInstance * instancePtr;
    XImage * image;
    XImage * mask;
    int isTransp;
{
    Display *display = Tk_Display(instancePtr->tkwin);
    int depth = Tk_Depth(instancePtr->tkwin);
    PixmapData *dataPtr = (PixmapData*)instancePtr->clientData;
    unsigned int gcMask;
    XGCValues gcValues;
    GC gc;

    instancePtr->pixmap = Tk_GetPixmap(display,
	Tk_WindowId(instancePtr->tkwin),
	masterPtr->size[0], masterPtr->size[1], depth);

    gc = Tk_GetGC(instancePtr->tkwin, 0, NULL);

    TkPutImage(0, 0, display, instancePtr->pixmap,
	    gc, image, 0, 0, 0, 0,
	    (unsigned) masterPtr->size[0], (unsigned) masterPtr->size[1]);

    Tk_FreeGC(display, gc);

    if (isTransp) {
	/*
	 * There are transparent pixels. We need a mask.
	 */
	dataPtr->mask = Tk_GetPixmap(display,
	    Tk_WindowId(instancePtr->tkwin),
	    masterPtr->size[0], masterPtr->size[1], 1);
	gc = XCreateGC(display, dataPtr->mask, 0, NULL);
	TkPutImage(0, 0, display, dataPtr->mask,
		gc, mask,  0, 0, 0, 0,
		(unsigned) masterPtr->size[0], (unsigned) masterPtr->size[1]);
	XFreeGC(display, gc);
    } else {
	dataPtr->mask = None;
    }

    /*
     * Allocate a GC for drawing this instance (mask is not used if there
     * is no transparent pixels inside the image).
     */
    if (dataPtr->mask != None) {
	gcMask = GCGraphicsExposures|GCClipMask;
    } else {
	gcMask = GCGraphicsExposures;
    }
    gcValues.graphics_exposures = False;
    gcValues.clip_mask = dataPtr->mask;
    
    gc = Tk_GetGC(instancePtr->tkwin, gcMask, &gcValues);
    dataPtr->gc = gc;
}

void
TixpXpmFreeInstanceData(instancePtr, delete, display)
    PixmapInstance *instancePtr;/* Pixmap instance. */
    int delete;			/* Should the instance data structure
				 * be deleted as well? */
    Display *display;		/* Display containing window that used image.*/
{
    PixmapData *dataPtr = (PixmapData*)instancePtr->clientData;

    if (dataPtr->mask != None) {
	Tk_FreePixmap(display, dataPtr->mask);
	dataPtr->mask = None;
    }
    if (dataPtr->gc != None) {
	Tk_FreeGC(display, dataPtr->gc);
	dataPtr->gc = None;
    }
    if (delete) {
	ckfree((char*)dataPtr);
	instancePtr->clientData = NULL;
    }
}

void
TixpXpmDisplay(clientData, display, drawable, imageX, imageY, width,
	height, drawableX, drawableY)
    ClientData clientData;	/* Pointer to PixmapInstance structure for
				 * for instance to be displayed. */
    Display *display;		/* Display on which to draw image. */
    Drawable drawable;		/* Pixmap or window in which to draw image. */
    int imageX, imageY;		/* Upper-left corner of region within image
				 * to draw. */
    int width, height;		/* Dimensions of region within image to draw.*/
    int drawableX, drawableY;	/* Coordinates within drawable that
				 * correspond to imageX and imageY. */
{
    PixmapInstance *instancePtr = (PixmapInstance *) clientData;
    PixmapData *dataPtr = (PixmapData*)instancePtr->clientData;

    /*
     * If there's no graphics context, it means that an error occurred
     * while creating the image instance so it can't be displayed.
     */
    if (dataPtr->gc == None) {
	return;
    }

    /*
     * We always use clipping: modify the clip origin within
     * the graphics context to line up with the image's origin.
     * Then draw the image and reset the clip origin.
     */
    XSetClipOrigin(display, dataPtr->gc, drawableX - imageX,
	drawableY - imageY);
    XCopyArea(display, instancePtr->pixmap, drawable, dataPtr->gc,
	imageX, imageY, (unsigned) width, (unsigned) height,
	drawableX, drawableY);
    XSetClipOrigin(display, dataPtr->gc, 0, 0);
}
