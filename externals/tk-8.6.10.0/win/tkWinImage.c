/*
 * tkWinImage.c --
 *
 *	This file contains routines for manipulation full-color images.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"

static int		DestroyImage(XImage* data);
static unsigned long	ImageGetPixel(XImage *image, int x, int y);
static int		PutPixel(XImage *image, int x, int y,
			    unsigned long pixel);

/*
 *----------------------------------------------------------------------
 *
 * DestroyImage --
 *
 *	This is a trivial wrapper around ckfree to make it possible to pass
 *	ckfree as a pointer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates the image.
 *
 *----------------------------------------------------------------------
 */

static int
DestroyImage(
    XImage *imagePtr)		/* Image to free. */
{
    if (imagePtr) {
	if (imagePtr->data) {
	    ckfree(imagePtr->data);
	}
	ckfree(imagePtr);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageGetPixel --
 *
 *	Get a single pixel from an image.
 *
 * Results:
 *	Returns the 32 bit pixel value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned long
ImageGetPixel(
    XImage *image,
    int x, int y)
{
    unsigned long pixel = 0;
    unsigned char *srcPtr = (unsigned char *) &(image->data[(y * image->bytes_per_line)
	    + ((x * image->bits_per_pixel) / NBBY)]);

    switch (image->bits_per_pixel) {
    case 32:
    case 24:
	pixel = RGB(srcPtr[2], srcPtr[1], srcPtr[0]);
	break;
    case 16:
	pixel = RGB(((((WORD*)srcPtr)[0]) >> 7) & 0xf8,
		((((WORD*)srcPtr)[0]) >> 2) & 0xf8,
		((((WORD*)srcPtr)[0]) << 3) & 0xf8);
	break;
    case 8:
	pixel = srcPtr[0];
	break;
    case 4:
	pixel = ((x%2) ? (*srcPtr) : ((*srcPtr) >> 4)) & 0x0f;
	break;
    case 1:
	pixel = ((*srcPtr) & (0x80 >> (x%8))) ? 1 : 0;
	break;
    }
    return pixel;
}

/*
 *----------------------------------------------------------------------
 *
 * PutPixel --
 *
 *	Set a single pixel in an image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
PutPixel(
    XImage *image,
    int x, int y,
    unsigned long pixel)
{
    unsigned char *destPtr = (unsigned char *) &(image->data[(y * image->bytes_per_line)
	    + ((x * image->bits_per_pixel) / NBBY)]);

    switch  (image->bits_per_pixel) {
    case 32:
	/*
	 * Pixel is DWORD: 0x00BBGGRR
	 */

	destPtr[3] = 0;
	/* FALLTHRU */
    case 24:
	/*
	 * Pixel is triplet: 0xBBGGRR.
	 */

	destPtr[0] = (unsigned char) GetBValue(pixel);
	destPtr[1] = (unsigned char) GetGValue(pixel);
	destPtr[2] = (unsigned char) GetRValue(pixel);
	break;
    case 16:
	/*
	 * Pixel is WORD: 5-5-5 (R-G-B)
	 */

	(*(WORD*)destPtr) = ((GetRValue(pixel) & 0xf8) << 7)
		| ((GetGValue(pixel) & 0xf8) <<2)
		| ((GetBValue(pixel) & 0xf8) >> 3);
	break;
    case 8:
	/*
	 * Pixel is 8-bit index into color table.
	 */

	(*destPtr) = (unsigned char) pixel;
	break;
    case 4:
	/*
	 * Pixel is 4-bit index in MSBFirst order.
	 */

	if (x%2) {
	    (*destPtr) = (unsigned char) (((*destPtr) & 0xf0)
		    | (pixel & 0x0f));
	} else {
	    (*destPtr) = (unsigned char) (((*destPtr) & 0x0f)
		    | ((pixel << 4) & 0xf0));
	}
	break;
    case 1: {
	/*
	 * Pixel is bit in MSBFirst order.
	 */

	int mask = (0x80 >> (x%8));

	if (pixel) {
	    (*destPtr) |= mask;
	} else {
	    (*destPtr) &= ~mask;
	}
	break;
    }
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * XCreateImage --
 *
 *	Allocates storage for a new XImage.
 *
 * Results:
 *	Returns a newly allocated XImage.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

XImage *
XCreateImage(
    Display *display,
    Visual *visual,
    unsigned int depth,
    int format,
    int offset,
    char *data,
    unsigned int width,
    unsigned int height,
    int bitmap_pad,
    int bytes_per_line)
{
    XImage* imagePtr = ckalloc(sizeof(XImage));
    imagePtr->width = width;
    imagePtr->height = height;
    imagePtr->xoffset = offset;
    imagePtr->format = format;
    imagePtr->data = data;
    imagePtr->byte_order = LSBFirst;
    imagePtr->bitmap_unit = 8;
    imagePtr->bitmap_bit_order = LSBFirst;
    imagePtr->bitmap_pad = bitmap_pad;
    imagePtr->bits_per_pixel = depth;
    imagePtr->depth = depth;

    /*
     * Under Windows, bitmap_pad must be on an LONG data-type boundary.
     */

#define LONGBITS    (sizeof(LONG) * 8)

    bitmap_pad = (bitmap_pad + LONGBITS - 1) / LONGBITS * LONGBITS;

    /*
     * Round to the nearest bitmap_pad boundary.
     */

    if (bytes_per_line) {
	imagePtr->bytes_per_line = bytes_per_line;
    } else {
	imagePtr->bytes_per_line = (((depth * width)
		+ (bitmap_pad - 1)) >> 3) & ~((bitmap_pad >> 3) - 1);
    }

    imagePtr->red_mask = 0;
    imagePtr->green_mask = 0;
    imagePtr->blue_mask = 0;

    imagePtr->f.put_pixel = PutPixel;
    imagePtr->f.get_pixel = ImageGetPixel;
    imagePtr->f.destroy_image = DestroyImage;
    imagePtr->f.create_image = NULL;
    imagePtr->f.sub_image = NULL;
    imagePtr->f.add_pixel = NULL;

    return imagePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * XGetImageZPixmap --
 *
 *	This function copies data from a pixmap or window into an XImage. This
 *	handles the ZPixmap case only.
 *
 * Results:
 *	Returns a newly allocated image containing the data from the given
 *	rectangle of the given drawable.
 *
 * Side effects:
 *	None.
 *
 * This procedure is adapted from the XGetImage implementation in TkNT. That
 * code is Copyright (c) 1994 Software Research Associates, Inc.
 *
 *----------------------------------------------------------------------
 */

static XImage *
XGetImageZPixmap(
    Display *display,
    Drawable d,
    int x, int y,
    unsigned int width, unsigned int height,
    unsigned long plane_mask,
    int	format)
{
    TkWinDrawable *twdPtr = (TkWinDrawable *)d;
    XImage *ret_image;
    HDC hdc, hdcMem;
    HBITMAP hbmp, hbmpPrev;
    BITMAPINFO *bmInfo = NULL;
    HPALETTE hPal, hPalPrev1 = 0, hPalPrev2 = 0;
    int size;
    unsigned int n;
    unsigned int depth;
    unsigned char *data;
    TkWinDCState state;
    BOOL ret;

    if (format != ZPixmap) {
	TkpDisplayWarning("Only ZPixmap types are implemented",
		"XGetImageZPixmap Failure");
	return NULL;
    }

    hdc = TkWinGetDrawableDC(display, d, &state);

    /*
     * Need to do a Blt operation to copy into a new bitmap.
     */

    hbmp = CreateCompatibleBitmap(hdc, (int) width, (int) height);
    hdcMem = CreateCompatibleDC(hdc);
    hbmpPrev = SelectObject(hdcMem, hbmp);
    hPal = state.palette;
    if (hPal) {
	hPalPrev1 = SelectPalette(hdcMem, hPal, FALSE);
	n = RealizePalette(hdcMem);
	if (n > 0) {
	    UpdateColors(hdcMem);
	}
	hPalPrev2 = SelectPalette(hdc, hPal, FALSE);
	n = RealizePalette(hdc);
	if (n > 0) {
	    UpdateColors(hdc);
	}
    }

    ret = BitBlt(hdcMem, 0, 0, (int) width, (int) height, hdc, x, y, SRCCOPY);
    if (hPal) {
	SelectPalette(hdc, hPalPrev2, FALSE);
    }
    SelectObject(hdcMem, hbmpPrev);
    TkWinReleaseDrawableDC(d, hdc, &state);
    if (ret == FALSE) {
	ret_image = NULL;
	goto cleanup;
    }
    if (twdPtr->type == TWD_WINDOW) {
	depth = Tk_Depth((Tk_Window) twdPtr->window.winPtr);
    } else {
	depth = twdPtr->bitmap.depth;
    }

    size = sizeof(BITMAPINFO);
    if (depth <= 8) {
	size += sizeof(unsigned short) << depth;
    }
    bmInfo = ckalloc(size);

    bmInfo->bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
    bmInfo->bmiHeader.biWidth		= width;
    bmInfo->bmiHeader.biHeight		= -(int) height;
    bmInfo->bmiHeader.biPlanes		= 1;
    bmInfo->bmiHeader.biBitCount	= depth;
    bmInfo->bmiHeader.biCompression	= BI_RGB;
    bmInfo->bmiHeader.biSizeImage	= 0;
    bmInfo->bmiHeader.biXPelsPerMeter	= 0;
    bmInfo->bmiHeader.biYPelsPerMeter	= 0;
    bmInfo->bmiHeader.biClrUsed		= 0;
    bmInfo->bmiHeader.biClrImportant	= 0;

    if (depth == 1) {
	unsigned char *p, *pend;

	GetDIBits(hdcMem, hbmp, 0, height, NULL, bmInfo, DIB_PAL_COLORS);
	data = ckalloc(bmInfo->bmiHeader.biSizeImage);
	if (!data) {
	    /* printf("Failed to allocate data area for XImage.\n"); */
	    ret_image = NULL;
	    goto cleanup;
	}
	ret_image = XCreateImage(display, NULL, depth, ZPixmap, 0, (char *) data,
		width, height, 32, (int) ((width + 31) >> 3) & ~1);
	if (ret_image == NULL) {
	    ckfree(data);
	    goto cleanup;
	}

	/*
	 * Get the BITMAP info into the Image.
	 */

	if (GetDIBits(hdcMem, hbmp, 0, height, data, bmInfo,
		DIB_PAL_COLORS) == 0) {
	    ckfree(ret_image->data);
	    ckfree(ret_image);
	    ret_image = NULL;
	    goto cleanup;
	}
	p = data;
	pend = data + bmInfo->bmiHeader.biSizeImage;
	while (p < pend) {
	    *p = ~*p;
	    p++;
	}
    } else if (depth == 8) {
	unsigned short *palette;
	unsigned int i;
	unsigned char *p;

	GetDIBits(hdcMem, hbmp, 0, height, NULL, bmInfo, DIB_PAL_COLORS);
	data = ckalloc(bmInfo->bmiHeader.biSizeImage);
	if (!data) {
	    /* printf("Failed to allocate data area for XImage.\n"); */
	    ret_image = NULL;
	    goto cleanup;
	}
	ret_image = XCreateImage(display, NULL, 8, ZPixmap, 0, (char *) data,
		width, height, 8, (int) width);
	if (ret_image == NULL) {
	    ckfree(data);
	    goto cleanup;
	}

	/*
	 * Get the BITMAP info into the Image.
	 */

	if (GetDIBits(hdcMem, hbmp, 0, height, data, bmInfo,
		DIB_PAL_COLORS) == 0) {
	    ckfree(ret_image->data);
	    ckfree(ret_image);
	    ret_image = NULL;
	    goto cleanup;
	}
	p = data;
	palette = (unsigned short *) bmInfo->bmiColors;
	for (i = 0; i < bmInfo->bmiHeader.biSizeImage; i++, p++) {
	    *p = (unsigned char) palette[*p];
	}
    } else if (depth == 16) {
	GetDIBits(hdcMem, hbmp, 0, height, NULL, bmInfo, DIB_RGB_COLORS);
	data = ckalloc(bmInfo->bmiHeader.biSizeImage);
	if (!data) {
	    /* printf("Failed to allocate data area for XImage.\n"); */
	    ret_image = NULL;
	    goto cleanup;
	}
	ret_image = XCreateImage(display, NULL, 16, ZPixmap, 0, (char *) data,
		width, height, 16, 0 /* will be calc'ed from bitmap_pad */);
	if (ret_image == NULL) {
	    ckfree(data);
	    goto cleanup;
	}

	/*
	 * Get the BITMAP info directly into the Image.
	 */

	if (GetDIBits(hdcMem, hbmp, 0, height, ret_image->data, bmInfo,
		DIB_RGB_COLORS) == 0) {
	    ckfree(ret_image->data);
	    ckfree(ret_image);
	    ret_image = NULL;
	    goto cleanup;
	}
    } else {
	GetDIBits(hdcMem, hbmp, 0, height, NULL, bmInfo, DIB_RGB_COLORS);
	data = ckalloc(width * height * 4);
	if (!data) {
	    /* printf("Failed to allocate data area for XImage.\n"); */
	    ret_image = NULL;
	    goto cleanup;
	}
	ret_image = XCreateImage(display, NULL, 32, ZPixmap, 0, (char *) data,
		width, height, 0, (int) width * 4);
	if (ret_image == NULL) {
	    ckfree(data);
	    goto cleanup;
	}

	if (depth <= 24) {
	    /*
	     * This used to handle 16 and 24 bpp, but now just handles 24. It
	     * can likely be optimized for that. -- hobbs
	     */

	    unsigned char *smallBitData, *smallBitBase, *bigBitData;
	    unsigned int byte_width, h, w;

	    byte_width = ((width * 3 + 3) & ~(unsigned)3);
	    smallBitBase = ckalloc(byte_width * height);
	    if (!smallBitBase) {
		ckfree(ret_image->data);
		ckfree(ret_image);
		ret_image = NULL;
		goto cleanup;
	    }
	    smallBitData = smallBitBase;

	    /*
	     * Get the BITMAP info into the Image.
	     */

	    if (GetDIBits(hdcMem, hbmp, 0, height, smallBitData, bmInfo,
		    DIB_RGB_COLORS) == 0) {
		ckfree(ret_image->data);
		ckfree(ret_image);
		ckfree(smallBitBase);
		ret_image = NULL;
		goto cleanup;
	    }

	    /*
	     * Copy the 24 Bit Pixmap to a 32-Bit one.
	     */

	    for (h = 0; h < height; h++) {
		bigBitData   = (unsigned char *) ret_image->data + h * ret_image->bytes_per_line;
		smallBitData = smallBitBase + h * byte_width;

		for (w = 0; w < width; w++) {
		    *bigBitData++ = ((*smallBitData++));
		    *bigBitData++ = ((*smallBitData++));
		    *bigBitData++ = ((*smallBitData++));
		    *bigBitData++ = 0;
		}
	    }

	    /*
	     * Free the Device contexts, and the Bitmap.
	     */

	    ckfree(smallBitBase);
	} else {
	    /*
	     * Get the BITMAP info directly into the Image.
	     */

	    if (GetDIBits(hdcMem, hbmp, 0, height, ret_image->data, bmInfo,
		    DIB_RGB_COLORS) == 0) {
		ckfree(ret_image->data);
		ckfree(ret_image);
		ret_image = NULL;
		goto cleanup;
	    }
	}
    }

  cleanup:
    if (bmInfo) {
	ckfree(bmInfo);
    }
    if (hPal) {
	SelectPalette(hdcMem, hPalPrev1, FALSE);
    }
    DeleteDC(hdcMem);
    DeleteObject(hbmp);

    return ret_image;
}

/*
 *----------------------------------------------------------------------
 *
 * XGetImage --
 *
 *	This function copies data from a pixmap or window into an XImage.
 *
 * Results:
 *	Returns a newly allocated image containing the data from the given
 *	rectangle of the given drawable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

XImage *
XGetImage(
    Display* display,
    Drawable d,
    int x, int y,
    unsigned int width, unsigned int height,
    unsigned long plane_mask,
    int	format)
{
    TkWinDrawable *twdPtr = (TkWinDrawable *)d;
    XImage *imagePtr;
    HDC dc;

    display->request++;

    if (twdPtr == NULL) {
	/*
	 * Avoid unmapped windows or bad drawables
	 */

	return NULL;
    }

    if (twdPtr->type != TWD_BITMAP) {
	/*
	 * This handles TWD_WINDOW or TWD_WINDC, always creating a 32bit
	 * image. If the window being copied isn't visible (unmapped or
	 * obscured), we quietly stop copying (no user error). The user will
	 * see black where the widget should be. This branch is likely
	 * followed in favor of XGetImageZPixmap as postscript printed widgets
	 * require RGB data.
	 */

	TkWinDCState state;
	unsigned int xx, yy, size;
	COLORREF pixel;

	dc = TkWinGetDrawableDC(display, d, &state);

	imagePtr = XCreateImage(display, NULL, 32, format, 0, NULL,
		width, height, 32, 0);
	size = imagePtr->bytes_per_line * imagePtr->height;
	imagePtr->data = ckalloc(size);
	ZeroMemory(imagePtr->data, size);

	for (yy = 0; yy < height; yy++) {
	    for (xx = 0; xx < width; xx++) {
		pixel = GetPixel(dc, x+(int)xx, y+(int)yy);
		if (pixel == CLR_INVALID) {
		    break;
		}
		PutPixel(imagePtr, (int) xx, (int) yy, pixel);
	    }
	}

	TkWinReleaseDrawableDC(d, dc, &state);
    } else if (format == ZPixmap) {
	/*
	 * This actually handles most TWD_WINDOW requests, but it varies from
	 * the above in that it really does a screen capture of an area, which
	 * is consistent with the Unix behavior, but does not appear to handle
	 * all bit depths correctly. -- hobbs
	 */

	imagePtr = XGetImageZPixmap(display, d, x, y,
		width, height, plane_mask, format);
    } else {
	const char *errMsg = NULL;
	char infoBuf[sizeof(BITMAPINFO) + sizeof(RGBQUAD)];
	BITMAPINFO *infoPtr = (BITMAPINFO*)infoBuf;

	if (twdPtr->bitmap.handle == NULL) {
	    errMsg = "XGetImage: not implemented for empty bitmap handles";
	} else if (format != XYPixmap) {
	    errMsg = "XGetImage: not implemented for format != XYPixmap";
	} else if (plane_mask != 1) {
	    errMsg = "XGetImage: not implemented for plane_mask != 1";
	}
	if (errMsg != NULL) {
	    /*
	     * Do a soft warning for the unsupported XGetImage types.
	     */

	    TkpDisplayWarning(errMsg, "XGetImage Failure");
	    return NULL;
	}

	imagePtr = XCreateImage(display, NULL, 1, XYBitmap, 0, NULL,
		width, height, 32, 0);
	imagePtr->data = ckalloc(imagePtr->bytes_per_line * imagePtr->height);

	dc = GetDC(NULL);

	GetDIBits(dc, twdPtr->bitmap.handle, 0, height, NULL,
		infoPtr, DIB_RGB_COLORS);

	infoPtr->bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	infoPtr->bmiHeader.biWidth		= width;
	infoPtr->bmiHeader.biHeight		= -(LONG)height;
	infoPtr->bmiHeader.biPlanes		= 1;
	infoPtr->bmiHeader.biBitCount		= 1;
	infoPtr->bmiHeader.biCompression	= BI_RGB;
	infoPtr->bmiHeader.biSizeImage		= 0;
	infoPtr->bmiHeader.biXPelsPerMeter	= 0;
	infoPtr->bmiHeader.biYPelsPerMeter	= 0;
	infoPtr->bmiHeader.biClrUsed		= 0;
	infoPtr->bmiHeader.biClrImportant	= 0;

	GetDIBits(dc, twdPtr->bitmap.handle, 0, height, imagePtr->data,
		infoPtr, DIB_RGB_COLORS);
	ReleaseDC(NULL, dc);
    }

    return imagePtr;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
