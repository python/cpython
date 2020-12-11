
/*	$Id: tixWinXpm.c,v 1.2 2004/03/28 02:44:57 hobbs Exp $	*/

/*
 * tixWinImgXpm.c --
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

#include <tkWinInt.h>
#include <tix.h>
#include <tixImgXpm.h>

typedef struct PixmapData {
    HDC bitmapDC;               /* Bitmap used on Windows platforms */
    HDC maskDC;                 /* Mask used on Windows platforms */
    HBITMAP bitmap, bitmapOld;
    HBITMAP maskBm, maskBmOld;
} PixmapData;

static void		CopyTransparent _ANSI_ARGS_((Display* display,
			    HDC srcDC, Drawable dest,
			    int src_x, int src_y, int width,
			    int height, int dest_x, int dest_y,
			    HDC maskDC));


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
    dataPtr->maskDC = NULL;
    dataPtr->bitmapDC = NULL;

    instancePtr->clientData = (ClientData)dataPtr;
}


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
    XImage * image = NULL;		/* Unused. Always return NULL. */
    XImage * mask;
    Display *display = Tk_Display(instancePtr->tkwin);
    int depth;
    int maskSize;
    int i;
    int wordBits  = sizeof(WORD)*8;		/* # of bits in WORD */
    int wordBytes = sizeof(WORD)/sizeof(char);	/* # of bytes in WORD */
    int words_per_line;

    depth = Tk_Depth(instancePtr->tkwin);

    instancePtr->pixmap = Tk_GetPixmap(display,
	Tk_WindowId(instancePtr->tkwin),
	masterPtr->size[0], masterPtr->size[1], depth);

    mask = (XImage*)ckalloc(sizeof(XImage));

    mask->width = masterPtr->size[0];
    mask->height = masterPtr->size[1];

    /*
     * In Windows, each scan line in the the mask data must be aligned
     * to words. The padding bits must be zero'ed.
     */
    words_per_line = (mask->width + (wordBits-1))/wordBits;
    mask->bytes_per_line = words_per_line * wordBytes;

    maskSize = mask->bytes_per_line  * mask->height;
    mask->data = (char *)ckalloc(maskSize);
    for (i=0; i<maskSize; i++) {
	mask->data[i] = 0;
    }

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
	ckfree((char*)mask);
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
    char * p;
    int n;
    GC gc;
    XGCValues gcValues;
    Display *display = Tk_Display(instancePtr->tkwin);

    if (colorPtr != NULL) {
	gcValues.foreground = colorPtr->pixel;
	gc = Tk_GetGC(instancePtr->tkwin, GCForeground, &gcValues);
	XDrawRectangle(display, instancePtr->pixmap, gc, x, y, 1, 1);
	Tk_FreeGC(display, gc);
    }

    p = mask->data;
    p+= y*(mask->bytes_per_line);
    p+= x/8;
    n = x%8;

    if (colorPtr != NULL) {
	*p |=  (1 << (7-n));
    } else {
	*p &= ~(1 << (7-n));
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
{
    Display *display = Tk_Display(instancePtr->tkwin);
    PixmapData *dataPtr = (PixmapData*)instancePtr->clientData;
    HDC dc, bitmapDC;
    TkWinDCState dcState;
    HBITMAP bitmap, bitmapOld;
    int w, h;

    w = masterPtr->size[0];
    h = masterPtr->size[1];

    dc = TkWinGetDrawableDC(display, instancePtr->pixmap, &dcState);
    bitmapDC = CreateCompatibleDC(dc);

    bitmap = CreateCompatibleBitmap(dc, w, h);
    bitmapOld = SelectObject(bitmapDC, bitmap);

    BitBlt(bitmapDC, 0, 0, w, h, dc, 0, 0, SRCCOPY);

    if (isTransp) {
	HDC maskDC;
	HBITMAP maskBm, maskBmOld;

	/*
	 * There are transparent pixels. We need a mask.
	 */
	maskDC = CreateCompatibleDC(dc);
	maskBm = CreateBitmap(w, h, 1, 1, (CONST VOID*)mask->data);
	maskBmOld = SelectObject(maskDC, maskBm);

	BitBlt(bitmapDC, 0, 0, w, h, maskDC, 0, 0, SRCAND);
	BitBlt(maskDC,   0, 0, w, h, maskDC, 0, 0, NOTSRCCOPY);

	dataPtr->maskDC = maskDC;
	dataPtr->maskBm = maskBm;
	dataPtr->maskBmOld = maskBmOld;
    } else {
	dataPtr->maskDC = NULL;
    }
    dataPtr->bitmapDC = bitmapDC;
    dataPtr->bitmap = bitmap;
    dataPtr->bitmapOld = bitmapOld;
    TkWinReleaseDrawableDC(instancePtr->pixmap, dc, &dcState);
}

void
TixpXpmFreeInstanceData(instancePtr, delete, display)
    PixmapInstance *instancePtr;	/* Pixmap instance. */
    int delete;				/* Should the instance data structure
					 * be deleted as well? */
    Display * display;			/* Unused on Windows. */
{
    PixmapData *dataPtr = (PixmapData*)instancePtr->clientData;

    if (dataPtr->maskDC != NULL) {
	DeleteObject(SelectObject(dataPtr->maskDC,
	    dataPtr->maskBmOld));
	DeleteDC(dataPtr->maskDC);
	dataPtr->maskDC = NULL;
    }
    if (dataPtr->bitmapDC != NULL) {
	DeleteObject(SelectObject(dataPtr->bitmapDC,
	    dataPtr->bitmapOld));
	DeleteDC(dataPtr->bitmapDC);
	dataPtr->bitmapDC = NULL;
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

    CopyTransparent(display, dataPtr->bitmapDC, drawable,
	imageX, imageY, width, height,
	drawableX, drawableY, dataPtr->maskDC);
}

static void
CopyTransparent(display, srcDC, dest, src_x, src_y, width, height, dest_x,
        dest_y,	maskDC)
    Display* display;
    HDC srcDC;
    Drawable dest;
    int src_x;
    int src_y;
    int width;
    int height;
    int dest_x;
    int dest_y;
    HDC maskDC;
{
    HDC destDC;
    TkWinDCState destState;

    destDC = TkWinGetDrawableDC(display, dest, &destState);

    if (maskDC) {
	BitBlt(destDC, dest_x, dest_y, width, height, maskDC, src_x, src_y,
	    SRCAND);
	BitBlt(destDC, dest_x, dest_y, width, height, srcDC, src_x, src_y,
	    SRCPAINT);
    } else {
	BitBlt(destDC, dest_x, dest_y, width, height, srcDC, src_x, src_y,
	    SRCCOPY);
    }

    TkWinReleaseDrawableDC(dest, destDC, &destState);
}

