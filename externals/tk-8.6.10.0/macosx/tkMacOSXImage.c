/*
 * tkMacOSXImage.c --
 *
 *	The code in this file provides an interface for XImages,
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2017-2018 Marc Culler.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "xbytes.h"

#pragma mark XImage handling

int
_XInitImageFuncPtrs(
    XImage *image)
{
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXCreateCGImageWithXImage --
 *
 *	Create CGImage from XImage, copying the image data.  Called
 *      in Tk_PutImage and (currently) nowhere else.
 *
 * Results:
 *	CGImage, release after use.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void ReleaseData(void *info, const void *data, size_t size) {
    ckfree(info);
}

CGImageRef
TkMacOSXCreateCGImageWithXImage(
    XImage *image)
{
    CGImageRef img = NULL;
    size_t bitsPerComponent, bitsPerPixel;
    size_t len = image->bytes_per_line * image->height;
    const CGFloat *decode = NULL;
    CGBitmapInfo bitmapInfo;
    CGDataProviderRef provider = NULL;
    char *data = NULL;
    CGDataProviderReleaseDataCallback releaseData = ReleaseData;

    if (image->bits_per_pixel == 1) {
	/*
	 * BW image
	 */

	/* Reverses the sense of the bits */
	static const CGFloat decodeWB[2] = {1, 0};
	decode = decodeWB;

	bitsPerComponent = 1;
	bitsPerPixel = 1;
	if (image->bitmap_bit_order != MSBFirst) {
	    char *srcPtr = image->data + image->xoffset;
	    char *endPtr = srcPtr + len;
	    char *destPtr = (data = ckalloc(len));

	    while (srcPtr < endPtr) {
		*destPtr++ = xBitReverseTable[(unsigned char)(*(srcPtr++))];
	    }
	} else {
	    data = memcpy(ckalloc(len), image->data + image->xoffset, len);
	}
	if (data) {
	    provider = CGDataProviderCreateWithData(data, data, len,
		    releaseData);
	}
	if (provider) {
	    img = CGImageMaskCreate(image->width, image->height,
		    bitsPerComponent, bitsPerPixel, image->bytes_per_line,
		    provider, decode, 0);
	}
    } else if ((image->format == ZPixmap) && (image->bits_per_pixel == 32)) {
	/*
	 * Color image
	 */

	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();

	if (image->width == 0 && image->height == 0) {
	    /*
	     * CGCreateImage complains on early macOS releases.
	     */

	    return NULL;
	}
	bitsPerComponent = 8;
	bitsPerPixel = 32;
	bitmapInfo = (image->byte_order == MSBFirst ?
		kCGBitmapByteOrder32Little : kCGBitmapByteOrder32Big);
	bitmapInfo |= kCGImageAlphaLast;
	data = memcpy(ckalloc(len), image->data + image->xoffset, len);
	if (data) {
	    provider = CGDataProviderCreateWithData(data, data, len,
		    releaseData);
	}
	if (provider) {
	    img = CGImageCreate(image->width, image->height, bitsPerComponent,
		    bitsPerPixel, image->bytes_per_line, colorspace, bitmapInfo,
		    provider, decode, 0, kCGRenderingIntentDefault);
	    CFRelease(provider);
	}
	if (colorspace) {
	    CFRelease(colorspace);
	}
    } else {
	TkMacOSXDbgMsg("Unsupported image type");
    }
    return img;
}

/*
 *----------------------------------------------------------------------
 *
 * XGetImage --
 *
 *	This function copies data from a pixmap or window into an XImage.  It
 *      is essentially never used. At one time it was called by
 *      pTkImgPhotoDisplay, but that is no longer the case. Currently it is
 *      called two places, one of which is requesting an XY image which we do
 *      not support.  It probably does not work correctly -- see the comments
 *      for TkMacOSXBitmapRepFromDrawableRect.
 *
 * Results:
 *	Returns a newly allocated XImage containing the data from the given
 *	rectangle of the given drawable, or NULL if the XImage could not be
 *	constructed.  NOTE: If we are copying from a window on a Retina
 *	display, the dimensions of the XImage will be 2*width x 2*height.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
struct pixel_fmt {int r; int g; int b; int a;};
static struct pixel_fmt bgra = {2, 1, 0, 3};
static struct pixel_fmt abgr = {3, 2, 1, 0};

XImage *
XGetImage(
    Display *display,
    Drawable drawable,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned long plane_mask,
    int format)
{
    NSBitmapImageRep* bitmap_rep = NULL;
    NSUInteger bitmap_fmt = 0;
    XImage* imagePtr = NULL;
    char* bitmap = NULL;
    char R, G, B, A;
    int depth = 32, offset = 0, bitmap_pad = 0;
    unsigned int bytes_per_row, size, row, n, m;
    unsigned int scalefactor=1, scaled_height=height, scaled_width=width;
    NSWindow *win = TkMacOSXDrawableWindow(drawable);
    static enum {unknown, no, yes} has_retina = unknown;

    if (win && has_retina == unknown) {
#ifdef __clang__
	has_retina = [win respondsToSelector:@selector(backingScaleFactor)] ?
		yes : no;
#else
	has_retina = no;
#endif
    }

    if (has_retina == yes) {
	/*
	 * We only allow scale factors 1 or 2, as Apple currently does.
	 */

#ifdef __clang__
	scalefactor = [win backingScaleFactor] == 2.0 ? 2 : 1;
#endif
	scaled_height *= scalefactor;
	scaled_width *= scalefactor;
    }

    if (format == ZPixmap) {
	if (width == 0 || height == 0) {
	    return NULL;
	}

	bitmap_rep = TkMacOSXBitmapRepFromDrawableRect(drawable,
		x, y, width, height);
	if (!bitmap_rep) {
	    TkMacOSXDbgMsg("XGetImage: Failed to construct NSBitmapRep");
	    return NULL;
	}
	bitmap_fmt = [bitmap_rep bitmapFormat];
	size = [bitmap_rep bytesPerPlane];
	bytes_per_row = [bitmap_rep bytesPerRow];
	bitmap = ckalloc(size);
	if (!bitmap
		|| (bitmap_fmt != 0 && bitmap_fmt != 1)
		|| [bitmap_rep samplesPerPixel] != 4
		|| [bitmap_rep isPlanar] != 0
		|| bytes_per_row < 4 * scaled_width
		|| size != bytes_per_row * scaled_height) {
	    TkMacOSXDbgMsg("XGetImage: Unrecognized bitmap format");
	    CFRelease(bitmap_rep);
	    return NULL;
	}
	memcpy(bitmap, (char *)[bitmap_rep bitmapData], size);
	CFRelease(bitmap_rep);

	/*
	 * When Apple extracts a bitmap from an NSView, it may be in either
	 * BGRA or ABGR format.  For an XImage we need RGBA.
	 */

	struct pixel_fmt pixel = bitmap_fmt == 0 ? bgra : abgr;

	for (row = 0, n = 0; row < scaled_height; row++, n += bytes_per_row) {
	    for (m = n; m < n + 4*scaled_width; m += 4) {
		R = *(bitmap + m + pixel.r);
		G = *(bitmap + m + pixel.g);
		B = *(bitmap + m + pixel.b);
		A = *(bitmap + m + pixel.a);

		*(bitmap + m)     = R;
		*(bitmap + m + 1) = G;
		*(bitmap + m + 2) = B;
		*(bitmap + m + 3) = A;
	    }
	}
	imagePtr = XCreateImage(display, NULL, depth, format, offset,
		(char*) bitmap, scaled_width, scaled_height,
		bitmap_pad, bytes_per_row);
	if (scalefactor == 2) {
	    imagePtr->pixelpower = 1;
	}
    } else {
	/*
	 * There are some calls to XGetImage in the generic Tk code which pass
	 * an XYPixmap rather than a ZPixmap.  XYPixmaps should be handled
	 * here.
	 */
	TkMacOSXDbgMsg("XGetImage does not handle XYPixmaps at the moment.");
    }
    return imagePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyImage --
 *
 *	Destroys storage associated with an image.
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
    XImage *image)
{
    if (image) {
	if (image->data) {
	    ckfree(image->data);
	}
	ckfree(image);
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
    int x,
    int y)
{
    unsigned char r = 0, g = 0, b = 0;

    if (image && image->data) {
	unsigned char *srcPtr = ((unsigned char*) image->data)
		+ (y * image->bytes_per_line)
		+ (((image->xoffset + x) * image->bits_per_pixel) / NBBY);

	switch (image->bits_per_pixel) {
	case 32:
	    r = (*((unsigned int*) srcPtr) >> 16) & 0xff;
	    g = (*((unsigned int*) srcPtr) >>  8) & 0xff;
	    b = (*((unsigned int*) srcPtr)      ) & 0xff;
	    /*if (image->byte_order == LSBFirst) {
		r = srcPtr[2]; g = srcPtr[1]; b = srcPtr[0];
	    } else {
		r = srcPtr[1]; g = srcPtr[2]; b = srcPtr[3];
	    }*/
	    break;
	case 16:
	    r = (*((unsigned short*) srcPtr) >> 7) & 0xf8;
	    g = (*((unsigned short*) srcPtr) >> 2) & 0xf8;
	    b = (*((unsigned short*) srcPtr) << 3) & 0xf8;
	    break;
	case 8:
	    r = (*srcPtr << 2) & 0xc0;
	    g = (*srcPtr << 4) & 0xc0;
	    b = (*srcPtr << 6) & 0xc0;
	    r |= r >> 2 | r >> 4 | r >> 6;
	    g |= g >> 2 | g >> 4 | g >> 6;
	    b |= b >> 2 | b >> 4 | b >> 6;
	    break;
	case 4: {
	    unsigned char c = (x % 2) ? *srcPtr : (*srcPtr >> 4);

	    r = (c & 0x04) ? 0xff : 0;
	    g = (c & 0x02) ? 0xff : 0;
	    b = (c & 0x01) ? 0xff : 0;
	    break;
	}
	case 1:
	    r = g = b = ((*srcPtr) & (0x80 >> (x % 8))) ? 0xff : 0;
	    break;
	}
    }
    return (PIXEL_MAGIC << 24) | (r << 16) | (g << 8) | b;
}

/*
 *----------------------------------------------------------------------
 *
 * ImagePutPixel --
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
ImagePutPixel(
    XImage *image,
    int x,
    int y,
    unsigned long pixel)
{
    if (image && image->data) {
	unsigned char *dstPtr = ((unsigned char*) image->data)
		+ (y * image->bytes_per_line)
		+ (((image->xoffset + x) * image->bits_per_pixel) / NBBY);

	if (image->bits_per_pixel == 32) {
	    *((unsigned int*) dstPtr) = pixel;
	} else {
	    unsigned char r = ((pixel & image->red_mask)   >> 16) & 0xff;
	    unsigned char g = ((pixel & image->green_mask) >>  8) & 0xff;
	    unsigned char b = ((pixel & image->blue_mask)       ) & 0xff;
	    switch (image->bits_per_pixel) {
	    case 16:
		*((unsigned short*) dstPtr) = ((r & 0xf8) << 7) |
			((g & 0xf8) << 2) | ((b & 0xf8) >> 3);
		break;
	    case 8:
		*dstPtr = ((r & 0xc0) >> 2) | ((g & 0xc0) >> 4) |
			((b & 0xc0) >> 6);
		break;
	    case 4: {
		unsigned char c = ((r & 0x80) >> 5) | ((g & 0x80) >> 6) |
			((b & 0x80) >> 7);
		*dstPtr = (x % 2) ? ((*dstPtr & 0xf0) | (c & 0x0f)) :
			((*dstPtr & 0x0f) | ((c << 4) & 0xf0));
		break;
		}
	    case 1:
		*dstPtr = ((r|g|b) & 0x80) ? (*dstPtr | (0x80 >> (x % 8))) :
			(*dstPtr & ~(0x80 >> (x % 8)));
		break;
	    }
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
    Display* display,
    Visual* visual,
    unsigned int depth,
    int format,
    int offset,
    char* data,
    unsigned int width,
    unsigned int height,
    int bitmap_pad,
    int bytes_per_line)
{
    XImage *ximage;

    display->request++;
    ximage = ckalloc(sizeof(XImage));

    ximage->height = height;
    ximage->width = width;
    ximage->depth = depth;
    ximage->xoffset = offset;
    ximage->format = format;
    ximage->data = data;
    ximage->obdata = NULL;

    /*
     * The default pixelpower is 0.  This must be explicitly set to 1 in the
     * case of an XImage extracted from a Retina display.
     */

    ximage->pixelpower = 0;

    if (format == ZPixmap) {
	ximage->bits_per_pixel = 32;
	ximage->bitmap_unit = 32;
    } else {
	ximage->bits_per_pixel = 1;
	ximage->bitmap_unit = 8;
    }
    if (bitmap_pad) {
	ximage->bitmap_pad = bitmap_pad;
    } else {
	/*
	 * Use 16 byte alignment for best Quartz perfomance.
	 */

	ximage->bitmap_pad = 128;
    }
    if (bytes_per_line) {
	ximage->bytes_per_line = bytes_per_line;
    } else {
	ximage->bytes_per_line = ((width * ximage->bits_per_pixel +
		(ximage->bitmap_pad - 1)) >> 3) &
		~((ximage->bitmap_pad >> 3) - 1);
    }
#ifdef WORDS_BIGENDIAN
    ximage->byte_order = MSBFirst;
    ximage->bitmap_bit_order = MSBFirst;
#else
    ximage->byte_order = LSBFirst;
    ximage->bitmap_bit_order = LSBFirst;
#endif
    ximage->red_mask = 0x00FF0000;
    ximage->green_mask = 0x0000FF00;
    ximage->blue_mask = 0x000000FF;
    ximage->f.create_image = NULL;
    ximage->f.destroy_image = DestroyImage;
    ximage->f.get_pixel = ImageGetPixel;
    ximage->f.put_pixel = ImagePutPixel;
    ximage->f.sub_image = NULL;
    ximage->f.add_pixel = NULL;

    return ximage;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPutImage, XPutImage --
 *
 *	Copies a rectangular subimage of an XImage into a drawable.  Currently
 *      this is only called by TkImgPhotoDisplay, using a Window as the
 *      drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Draws the image on the specified drawable.
 *
 *----------------------------------------------------------------------
 */

int
XPutImage(
    Display* display,		/* Display. */
    Drawable drawable,		/* Drawable to place image on. */
    GC gc,			/* GC to use. */
    XImage* image,		/* Image to place. */
    int src_x,			/* Source X & Y. */
    int src_y,
    int dest_x,			/* Destination X & Y. */
    int dest_y,
    unsigned int width,	        /* Same width & height for both */
    unsigned int height)	/* distination and source. */
{
    TkMacOSXDrawingContext dc;
    MacDrawable *macDraw = (MacDrawable *) drawable;

    display->request++;
    if (!TkMacOSXSetupDrawingContext(drawable, gc, 1, &dc)) {
	return BadDrawable;
    }
    if (dc.context) {
	CGRect bounds, srcRect, dstRect;
	CGImageRef img = TkMacOSXCreateCGImageWithXImage(image);

	/*
	 * The CGContext for a pixmap is RGB only, with A = 0.
	 */

	if (!(macDraw->flags & TK_IS_PIXMAP)) {
	    CGContextSetBlendMode(dc.context, kCGBlendModeSourceAtop);
	}
	if (img) {

	    /*
	     * If the XImage has big pixels, the source is rescaled to reflect
	     * the actual pixel dimensions.  This is not currently used, but
	     * could arise if the image were copied from a retina monitor and
	     * redrawn on an ordinary monitor.
	     */

	    int pp = image->pixelpower;

	    bounds = CGRectMake(0, 0, image->width, image->height);
	    srcRect = CGRectMake(src_x<<pp, src_y<<pp, width<<pp, height<<pp);
	    dstRect = CGRectMake(dest_x, dest_y, width, height);
	    TkMacOSXDrawCGImage(drawable, gc, dc.context,
				img, gc->foreground, gc->background,
				bounds, srcRect, dstRect);
	    CFRelease(img);
	} else {
	    TkMacOSXDbgMsg("Invalid source drawable");
	}
    } else {
	TkMacOSXDbgMsg("Invalid destination drawable");
    }
    TkMacOSXRestoreDrawingContext(&dc);
    return Success;
}

int
TkPutImage(
    unsigned long *colors,	/* Array of pixel values used by this image.
				 * May be NULL. */
    int ncolors,		/* Number of colors used, or 0. */
    Display *display,
    Drawable d,			/* Destination drawable. */
    GC gc,
    XImage *image,		/* Source image. */
    int src_x, int src_y,	/* Offset of subimage. */
    int dest_x, int dest_y,	/* Position of subimage origin in drawable. */
    unsigned int width, unsigned int height)
				/* Dimensions of subimage. */
{
    return XPutImage(display, d, gc, image, src_x, src_y, dest_x, dest_y, width, height);
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
