/*
 * tkImgPhInstance.c --
 *
 *	Implements the rendering of images of type "photo" for Tk. Photo
 *	images are stored in full color (32 bits per pixel including alpha
 *	channel) and displayed using dithering if necessary.
 *
 * Copyright (c) 1994 The Australian National University.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2002-2008 Donal K. Fellows
 * Copyright (c) 2003 ActiveState Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Author: Paul Mackerras (paulus@cs.anu.edu.au),
 *	   Department of Computer Science,
 *	   Australian National University.
 */

#include "tkImgPhoto.h"
#ifdef MAC_OSX_TK
#define TKPUTIMAGE_CAN_BLEND
#endif

/*
 * Declaration for internal Xlib function used here:
 */

extern int		_XInitImageFuncPtrs(XImage *image);

/*
 * Forward declarations
 */

#ifndef TKPUTIMAGE_CAN_BLEND
static void		BlendComplexAlpha(XImage *bgImg, PhotoInstance *iPtr,
			    int xOffset, int yOffset, int width, int height);
#endif
static int		IsValidPalette(PhotoInstance *instancePtr,
			    const char *palette);
static int		CountBits(pixel mask);
static void		GetColorTable(PhotoInstance *instancePtr);
static void		FreeColorTable(ColorTable *colorPtr, int force);
static void		AllocateColors(ColorTable *colorPtr);
static void		DisposeColorTable(ClientData clientData);
static int		ReclaimColors(ColorTableId *id, int numColors);

/*
 * Hash table used to hash from (display, colormap, palette, gamma) to
 * ColorTable address.
 */

static Tcl_HashTable imgPhotoColorHash;
static int imgPhotoColorHashInitialized;
#define N_COLOR_HASH	(sizeof(ColorTableId) / sizeof(int))

/*
 *----------------------------------------------------------------------
 *
 * TkImgPhotoConfigureInstance --
 *
 *	This function is called to create displaying information for a photo
 *	image instance based on the configuration information in the master.
 *	It is invoked both when new instances are created and when the master
 *	is reconfigured.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates errors via Tcl_BackgroundException if there are problems in
 *	setting up the instance.
 *
 *----------------------------------------------------------------------
 */

void
TkImgPhotoConfigureInstance(
    PhotoInstance *instancePtr)	/* Instance to reconfigure. */
{
    PhotoMaster *masterPtr = instancePtr->masterPtr;
    XImage *imagePtr;
    int bitsPerPixel;
    ColorTable *colorTablePtr;
    XRectangle validBox;

    /*
     * If the -palette configuration option has been set for the master, use
     * the value specified for our palette, but only if it is a valid palette
     * for our windows. Use the gamma value specified the master.
     */

    if ((masterPtr->palette && masterPtr->palette[0])
	    && IsValidPalette(instancePtr, masterPtr->palette)) {
	instancePtr->palette = masterPtr->palette;
    } else {
	instancePtr->palette = instancePtr->defaultPalette;
    }
    instancePtr->gamma = masterPtr->gamma;

    /*
     * If we don't currently have a color table, or if the one we have no
     * longer applies (e.g. because our palette or gamma has changed), get a
     * new one.
     */

    colorTablePtr = instancePtr->colorTablePtr;
    if ((colorTablePtr == NULL)
	    || (instancePtr->colormap != colorTablePtr->id.colormap)
	    || (instancePtr->palette != colorTablePtr->id.palette)
	    || (instancePtr->gamma != colorTablePtr->id.gamma)) {
	/*
	 * Free up our old color table, and get a new one.
	 */

	if (colorTablePtr != NULL) {
	    colorTablePtr->liveRefCount -= 1;
	    FreeColorTable(colorTablePtr, 0);
	}
	GetColorTable(instancePtr);

	/*
	 * Create a new XImage structure for sending data to the X server, if
	 * necessary.
	 */

	if (instancePtr->colorTablePtr->flags & BLACK_AND_WHITE) {
	    bitsPerPixel = 1;
	} else {
	    bitsPerPixel = instancePtr->visualInfo.depth;
	}

	if ((instancePtr->imagePtr == NULL)
		|| (instancePtr->imagePtr->bits_per_pixel != bitsPerPixel)) {
	    if (instancePtr->imagePtr != NULL) {
		XDestroyImage(instancePtr->imagePtr);
	    }
	    imagePtr = XCreateImage(instancePtr->display,
		    instancePtr->visualInfo.visual, (unsigned) bitsPerPixel,
		    (bitsPerPixel > 1? ZPixmap: XYBitmap), 0, NULL,
		    1, 1, 32, 0);
	    instancePtr->imagePtr = imagePtr;

	    /*
	     * We create images using the local host's endianness, rather than
	     * the endianness of the server; otherwise we would have to
	     * byte-swap any 16 or 32 bit values that we store in the image
	     * if the server's endianness is different from ours.
	     */

	    if (imagePtr != NULL) {
#ifdef WORDS_BIGENDIAN
		imagePtr->byte_order = MSBFirst;
#else
		imagePtr->byte_order = LSBFirst;
#endif
		_XInitImageFuncPtrs(imagePtr);
	    }
	}
    }

    /*
     * If the user has specified a width and/or height for the master which is
     * different from our current width/height, set the size to the values
     * specified by the user. If we have no pixmap, we do this also, since it
     * has the side effect of allocating a pixmap for us.
     */

    if ((instancePtr->pixels == None) || (instancePtr->error == NULL)
	    || (instancePtr->width != masterPtr->width)
	    || (instancePtr->height != masterPtr->height)) {
	TkImgPhotoInstanceSetSize(instancePtr);
    }

    /*
     * Redither this instance if necessary.
     */

    if ((masterPtr->flags & IMAGE_CHANGED)
	    || (instancePtr->colorTablePtr != colorTablePtr)) {
	TkClipBox(masterPtr->validRegion, &validBox);
	if ((validBox.width > 0) && (validBox.height > 0)) {
	    TkImgDitherInstance(instancePtr, validBox.x, validBox.y,
		    validBox.width, validBox.height);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkImgPhotoGet --
 *
 *	This function is called for each use of a photo image in a widget.
 *
 * Results:
 *	The return value is a token for the instance, which is passed back to
 *	us in calls to TkImgPhotoDisplay and ImgPhotoFree.
 *
 * Side effects:
 *	A data structure is set up for the instance (or, an existing instance
 *	is re-used for the new one).
 *
 *----------------------------------------------------------------------
 */

ClientData
TkImgPhotoGet(
    Tk_Window tkwin,		/* Window in which the instance will be
				 * used. */
    ClientData masterData)	/* Pointer to our master structure for the
				 * image. */
{
    PhotoMaster *masterPtr = masterData;
    PhotoInstance *instancePtr;
    Colormap colormap;
    int mono, nRed, nGreen, nBlue, numVisuals;
    XVisualInfo visualInfo, *visInfoPtr;
    char buf[TCL_INTEGER_SPACE * 3];
    XColor *white, *black;
    XGCValues gcValues;

    /*
     * Table of "best" choices for palette for PseudoColor displays with
     * between 3 and 15 bits/pixel.
     */

    static const int paletteChoice[13][3] = {
	/*  #red, #green, #blue */
	 {2,  2,  2,		/* 3 bits, 8 colors */},
	 {2,  3,  2,		/* 4 bits, 12 colors */},
	 {3,  4,  2,		/* 5 bits, 24 colors */},
	 {4,  5,  3,		/* 6 bits, 60 colors */},
	 {5,  6,  4,		/* 7 bits, 120 colors */},
	 {7,  7,  4,		/* 8 bits, 198 colors */},
	 {8, 10,  6,		/* 9 bits, 480 colors */},
	{10, 12,  8,		/* 10 bits, 960 colors */},
	{14, 15,  9,		/* 11 bits, 1890 colors */},
	{16, 20, 12,		/* 12 bits, 3840 colors */},
	{20, 24, 16,		/* 13 bits, 7680 colors */},
	{26, 30, 20,		/* 14 bits, 15600 colors */},
	{32, 32, 30,		/* 15 bits, 30720 colors */}
    };

    /*
     * See if there is already an instance for windows using the same
     * colormap. If so then just re-use it.
     */

    colormap = Tk_Colormap(tkwin);
    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	if ((colormap == instancePtr->colormap)
		&& (Tk_Display(tkwin) == instancePtr->display)) {
	    /*
	     * Re-use this instance.
	     */

	    if (instancePtr->refCount == 0) {
		/*
		 * We are resurrecting this instance.
		 */

		Tcl_CancelIdleCall(TkImgDisposeInstance, instancePtr);
		if (instancePtr->colorTablePtr != NULL) {
		    FreeColorTable(instancePtr->colorTablePtr, 0);
		}
		GetColorTable(instancePtr);
	    }
	    instancePtr->refCount++;
	    return instancePtr;
	}
    }

    /*
     * The image isn't already in use in a window with the same colormap. Make
     * a new instance of the image.
     */

    instancePtr = ckalloc(sizeof(PhotoInstance));
    instancePtr->masterPtr = masterPtr;
    instancePtr->display = Tk_Display(tkwin);
    instancePtr->colormap = Tk_Colormap(tkwin);
    Tk_PreserveColormap(instancePtr->display, instancePtr->colormap);
    instancePtr->refCount = 1;
    instancePtr->colorTablePtr = NULL;
    instancePtr->pixels = None;
    instancePtr->error = NULL;
    instancePtr->width = 0;
    instancePtr->height = 0;
    instancePtr->imagePtr = 0;
    instancePtr->nextPtr = masterPtr->instancePtr;
    masterPtr->instancePtr = instancePtr;

    /*
     * Obtain information about the visual and decide on the default palette.
     */

    visualInfo.screen = Tk_ScreenNumber(tkwin);
    visualInfo.visualid = XVisualIDFromVisual(Tk_Visual(tkwin));
    visInfoPtr = XGetVisualInfo(Tk_Display(tkwin),
	    VisualScreenMask | VisualIDMask, &visualInfo, &numVisuals);
    if (visInfoPtr == NULL) {
	Tcl_Panic("TkImgPhotoGet couldn't find visual for window");
    }

    nRed = 2;
    nGreen = nBlue = 0;
    mono = 1;
    instancePtr->visualInfo = *visInfoPtr;
    switch (visInfoPtr->c_class) {
    case DirectColor:
    case TrueColor:
	nRed = 1 << CountBits(visInfoPtr->red_mask);
	nGreen = 1 << CountBits(visInfoPtr->green_mask);
	nBlue = 1 << CountBits(visInfoPtr->blue_mask);
	mono = 0;
	break;
    case PseudoColor:
    case StaticColor:
	if (visInfoPtr->depth > 15) {
	    nRed = 32;
	    nGreen = 32;
	    nBlue = 32;
	    mono = 0;
	} else if (visInfoPtr->depth >= 3) {
	    const int *ip = paletteChoice[visInfoPtr->depth - 3];

	    nRed = ip[0];
	    nGreen = ip[1];
	    nBlue = ip[2];
	    mono = 0;
	}
	break;
    case GrayScale:
    case StaticGray:
	nRed = 1 << visInfoPtr->depth;
	break;
    }
    XFree((char *) visInfoPtr);

    if (mono) {
	sprintf(buf, "%d", nRed);
    } else {
	sprintf(buf, "%d/%d/%d", nRed, nGreen, nBlue);
    }
    instancePtr->defaultPalette = Tk_GetUid(buf);

    /*
     * Make a GC with background = black and foreground = white.
     */

    white = Tk_GetColor(masterPtr->interp, tkwin, "white");
    black = Tk_GetColor(masterPtr->interp, tkwin, "black");
    gcValues.foreground = (white != NULL)? white->pixel:
	    WhitePixelOfScreen(Tk_Screen(tkwin));
    gcValues.background = (black != NULL)? black->pixel:
	    BlackPixelOfScreen(Tk_Screen(tkwin));
    Tk_FreeColor(white);
    Tk_FreeColor(black);
    gcValues.graphics_exposures = False;
    instancePtr->gc = Tk_GetGC(tkwin,
	    GCForeground|GCBackground|GCGraphicsExposures, &gcValues);

    /*
     * Set configuration options and finish the initialization of the
     * instance. This will also dither the image if necessary.
     */

    TkImgPhotoConfigureInstance(instancePtr);

    /*
     * If this is the first instance, must set the size of the image.
     */

    if (instancePtr->nextPtr == NULL) {
	Tk_ImageChanged(masterPtr->tkMaster, 0, 0, 0, 0,
		masterPtr->width, masterPtr->height);
    }

    return instancePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * BlendComplexAlpha --
 *
 *	This function is called when an image with partially transparent
 *	pixels must be drawn over another image. It blends the photo data onto
 *	a local copy of the surface that we are drawing on, *including* the
 *	pixels drawn by everything that should be drawn underneath the image.
 *
 *	Much of this code has hard-coded values in for speed because this
 *	routine is performance critical for complex image drawing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Background image passed in gets drawn over with image data.
 *
 * Notes:
 *	This should work on all platforms that set mask and shift data
 *	properly from the visualInfo. RGB is really only a 24+ bpp version
 *	whereas RGB15 is the correct version and works for 15bpp+, but it
 *	slower, so it's only used for 15bpp+.
 *
 *	Note that Win32 pre-defines those operations that we really need.
 *
 *	Note that on MacOS, if the background comes from a Retina display
 *	then it will be twice as wide and twice as high as the photoimage.
 *
 *----------------------------------------------------------------------
 */
#ifndef TKPUTIMAGE_CAN_BLEND
#ifndef _WIN32
#define GetRValue(rgb)	(UCHAR(((rgb) & red_mask) >> red_shift))
#define GetGValue(rgb)	(UCHAR(((rgb) & green_mask) >> green_shift))
#define GetBValue(rgb)	(UCHAR(((rgb) & blue_mask) >> blue_shift))
#define RGB(r, g, b)	((unsigned)( \
	(UCHAR(r) << red_shift)   | \
	(UCHAR(g) << green_shift) | \
	(UCHAR(b) << blue_shift)  ))
#define RGB15(r, g, b)	((unsigned)( \
	(((r) * red_mask / 255)   & red_mask)   | \
	(((g) * green_mask / 255) & green_mask) | \
	(((b) * blue_mask / 255)  & blue_mask)  ))
#endif /* !_WIN32 */

static void
BlendComplexAlpha(
    XImage *bgImg,		/* Background image to draw on. */
    PhotoInstance *iPtr,	/* Image instance to draw. */
    int xOffset, int yOffset,	/* X & Y offset into image instance to
				 * draw. */
    int width, int height)	/* Width & height of image to draw. */
{
    int x, y, line;
    unsigned long pixel;
    unsigned char r, g, b, alpha, unalpha, *masterPtr;
    unsigned char *alphaAr = iPtr->masterPtr->pix32;

    /*
     * This blending is an integer version of the Source-Over compositing rule
     * (see Porter&Duff, "Compositing Digital Images", proceedings of SIGGRAPH
     * 1984) that has been hard-coded (for speed) to work with targetting a
     * solid surface.
     *
     * The 'unalpha' field must be 255-alpha; it is separated out to encourage
     * more efficient compilation.
     */

#define ALPHA_BLEND(bgPix, imgPix, alpha, unalpha) \
	((bgPix * unalpha + imgPix * alpha) / 255)

    /*
     * We have to get the mask and shift info from the visual on non-Win32 so
     * that the macros Get*Value(), RGB() and RGB15() work correctly. This
     * might be cached for better performance.
     */

#ifndef _WIN32
    unsigned long red_mask, green_mask, blue_mask;
    unsigned long red_shift, green_shift, blue_shift;
    Visual *visual = iPtr->visualInfo.visual;

    red_mask = visual->red_mask;
    green_mask = visual->green_mask;
    blue_mask = visual->blue_mask;
    red_shift = 0;
    green_shift = 0;
    blue_shift = 0;
    while ((0x0001 & (red_mask >> red_shift)) == 0) {
	red_shift++;
    }
    while ((0x0001 & (green_mask >> green_shift)) == 0) {
	green_shift++;
    }
    while ((0x0001 & (blue_mask >> blue_shift)) == 0) {
	blue_shift++;
    }
#endif /* !_WIN32 */

    /*
     * Only UNIX requires the special case for <24bpp. It varies with 3 extra
     * shifts and uses RGB15. The 24+bpp version could also then be further
     * optimized.
     */

#if !defined(_WIN32)
    if (bgImg->depth < 24) {
	unsigned char red_mlen, green_mlen, blue_mlen;

	red_mlen = 8 - CountBits(red_mask >> red_shift);
	green_mlen = 8 - CountBits(green_mask >> green_shift);
	blue_mlen = 8 - CountBits(blue_mask >> blue_shift);
	for (y = 0; y < height; y++) {
	    line = (y + yOffset) * iPtr->masterPtr->width;
	    for (x = 0; x < width; x++) {
		masterPtr = alphaAr + ((line + x + xOffset) * 4);
		alpha = masterPtr[3];

		/*
		 * Ignore pixels that are fully transparent
		 */

		if (alpha) {
		    /*
		     * We could perhaps be more efficient than XGetPixel for
		     * 24 and 32 bit displays, but this seems "fast enough".
		     */

		    r = masterPtr[0];
		    g = masterPtr[1];
		    b = masterPtr[2];
		    if (alpha != 255) {
			/*
			 * Only blend pixels that have some transparency
			 */

			unsigned char ra, ga, ba;

			pixel = XGetPixel(bgImg, x, y);
			ra = GetRValue(pixel) << red_mlen;
			ga = GetGValue(pixel) << green_mlen;
			ba = GetBValue(pixel) << blue_mlen;
			unalpha = 255 - alpha;	/* Calculate once. */
			r = ALPHA_BLEND(ra, r, alpha, unalpha);
			g = ALPHA_BLEND(ga, g, alpha, unalpha);
			b = ALPHA_BLEND(ba, b, alpha, unalpha);
		    }
		    XPutPixel(bgImg, x, y, RGB15(r, g, b));
		}
	    }
	}
	return;
    }
#endif /* !_WIN32 */

    for (y = 0; y < height; y++) {
	line = (y + yOffset) * iPtr->masterPtr->width;
	for (x = 0; x < width; x++) {
	    masterPtr = alphaAr + ((line + x + xOffset) * 4);
	    alpha = masterPtr[3];

	    /*
	     * Ignore pixels that are fully transparent
	     */

	    if (alpha) {
		/*
		 * We could perhaps be more efficient than XGetPixel for 24
		 * and 32 bit displays, but this seems "fast enough".
		 */

		r = masterPtr[0];
		g = masterPtr[1];
		b = masterPtr[2];
		if (alpha != 255) {
		    /*
		     * Only blend pixels that have some transparency
		     */

		    unsigned char ra, ga, ba;

		    pixel = XGetPixel(bgImg, x, y);
		    ra = GetRValue(pixel);
		    ga = GetGValue(pixel);
		    ba = GetBValue(pixel);
		    unalpha = 255 - alpha;	/* Calculate once. */
		    r = ALPHA_BLEND(ra, r, alpha, unalpha);
		    g = ALPHA_BLEND(ga, g, alpha, unalpha);
		    b = ALPHA_BLEND(ba, b, alpha, unalpha);
		}
		XPutPixel(bgImg, x, y, RGB(r, g, b));
	    }
	}
    }
#undef ALPHA_BLEND
}
#endif /* TKPUTIMAGE_CAN_BLEND */

/*
 *----------------------------------------------------------------------
 *
 * TkImgPhotoDisplay --
 *
 *	This function is invoked to draw a photo image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A portion of the image gets rendered in a pixmap or window.
 *
 *----------------------------------------------------------------------
 */

void
TkImgPhotoDisplay(
    ClientData clientData,	/* Pointer to PhotoInstance structure for
				 * instance to be displayed. */
    Display *display,		/* Display on which to draw image. */
    Drawable drawable,		/* Pixmap or window in which to draw image. */
    int imageX, int imageY,	/* Upper-left corner of region within image to
				 * draw. */
    int width, int height,	/* Dimensions of region within image to
				 * draw. */
    int drawableX,int drawableY)/* Coordinates within drawable that correspond
				 * to imageX and imageY. */
{
    PhotoInstance *instancePtr = clientData;
#ifndef TKPUTIMAGE_CAN_BLEND
    XVisualInfo visInfo = instancePtr->visualInfo;
#endif

    /*
     * If there's no pixmap, it means that an error occurred while creating
     * the image instance so it can't be displayed.
     */

    if (instancePtr->pixels == None) {
	return;
    }

#ifdef TKPUTIMAGE_CAN_BLEND
    /*
     * If TkPutImage can handle RGBA Ximages directly there is
     * no need to call XGetImage or to do the Porter-Duff compositing by hand.
     */

    unsigned char *rgbaPixels = instancePtr->masterPtr->pix32;
    XImage *photo = XCreateImage(display, NULL, 32, ZPixmap, 0, (char*)rgbaPixels,
				 (unsigned int)instancePtr->width,
				 (unsigned int)instancePtr->height,
				 0, (unsigned int)(4 * instancePtr->width));
    TkPutImage(NULL, 0, display, drawable, instancePtr->gc,
	       photo, imageX, imageY, drawableX, drawableY,
	       (unsigned int) width, (unsigned int) height);
    photo->data = NULL;
    XDestroyImage(photo);
#else

    if ((instancePtr->masterPtr->flags & COMPLEX_ALPHA)
	    && visInfo.depth >= 15
	    && (visInfo.c_class == DirectColor || visInfo.c_class == TrueColor)) {
	Tk_ErrorHandler handler;
	XImage *bgImg = NULL;

	/*
	 * Create an error handler to suppress the case where the input was
	 * not properly constrained, which can cause an X error. [Bug 979239]
	 */

	handler = Tk_CreateErrorHandler(display, -1, -1, -1, NULL, NULL);

	/*
	 * Pull the current background from the display to blend with
	 */

	bgImg = XGetImage(display, drawable, drawableX, drawableY,
		(unsigned int)width, (unsigned int)height, AllPlanes, ZPixmap);
	if (bgImg == NULL) {
	    Tk_DeleteErrorHandler(handler);
	    /* We failed to get the image, so draw without blending alpha.
	     * It's the best we can do.
	     */
	    goto fallBack;
	}

	BlendComplexAlpha(bgImg, instancePtr, imageX, imageY, width, height);

	/*
	 * Color info is unimportant as we only do this operation for depth >=
	 * 15.
	 */

	TkPutImage(NULL, 0, display, drawable, instancePtr->gc,
		bgImg, 0, 0, drawableX, drawableY,
		(unsigned int) width, (unsigned int) height);
	XDestroyImage(bgImg);
	Tk_DeleteErrorHandler(handler);
    } else {
	/*
	 * masterPtr->region describes which parts of the image contain valid
	 * data. We set this region as the clip mask for the gc, setting its
	 * origin appropriately, and use it when drawing the image.
	 */

    fallBack:
	TkSetRegion(display, instancePtr->gc,
		instancePtr->masterPtr->validRegion);
	XSetClipOrigin(display, instancePtr->gc, drawableX - imageX,
		drawableY - imageY);
	XCopyArea(display, instancePtr->pixels, drawable, instancePtr->gc,
		imageX, imageY, (unsigned) width, (unsigned) height,
		drawableX, drawableY);
	XSetClipMask(display, instancePtr->gc, None);
	XSetClipOrigin(display, instancePtr->gc, 0, 0);
    }
    (void)XFlush(display);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TkImgPhotoFree --
 *
 *	This function is called when a widget ceases to use a particular
 *	instance of an image. We don't actually get rid of the instance until
 *	later because we may be about to get this instance again.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Internal data structures get cleaned up, later.
 *
 *----------------------------------------------------------------------
 */

void
TkImgPhotoFree(
    ClientData clientData,	/* Pointer to PhotoInstance structure for
				 * instance to be displayed. */
    Display *display)		/* Display containing window that used
				 * image. */
{
    PhotoInstance *instancePtr = clientData;
    ColorTable *colorPtr;

    if (instancePtr->refCount-- > 1) {
	return;
    }

    /*
     * There are no more uses of the image within this widget. Decrement the
     * count of live uses of its color table, so that its colors can be
     * reclaimed if necessary, and set up an idle call to free the instance
     * structure.
     */

    colorPtr = instancePtr->colorTablePtr;
    if (colorPtr != NULL) {
	colorPtr->liveRefCount -= 1;
    }

    Tcl_DoWhenIdle(TkImgDisposeInstance, instancePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkImgPhotoInstanceSetSize --
 *
 *	This function reallocates the instance pixmap and dithering error
 *	array for a photo instance, as necessary, to change the image's size
 *	to `width' x `height' pixels.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Storage gets reallocated, here and in the X server.
 *
 *----------------------------------------------------------------------
 */

void
TkImgPhotoInstanceSetSize(
    PhotoInstance *instancePtr)	/* Instance whose size is to be changed. */
{
    PhotoMaster *masterPtr;
    schar *newError, *errSrcPtr, *errDestPtr;
    int h, offset;
    XRectangle validBox;
    Pixmap newPixmap;

    masterPtr = instancePtr->masterPtr;
    TkClipBox(masterPtr->validRegion, &validBox);

    if ((instancePtr->width != masterPtr->width)
	    || (instancePtr->height != masterPtr->height)
	    || (instancePtr->pixels == None)) {
	newPixmap = Tk_GetPixmap(instancePtr->display,
		RootWindow(instancePtr->display,
			instancePtr->visualInfo.screen),
		(masterPtr->width > 0) ? masterPtr->width: 1,
		(masterPtr->height > 0) ? masterPtr->height: 1,
		instancePtr->visualInfo.depth);
	if (!newPixmap) {
	    Tcl_Panic("Fail to create pixmap with Tk_GetPixmap in TkImgPhotoInstanceSetSize");
	}

	/*
	 * The following is a gross hack needed to properly support colormaps
	 * under Windows. Before the pixels can be copied to the pixmap, the
	 * relevent colormap must be associated with the drawable. Normally we
	 * can infer this association from the window that was used to create
	 * the pixmap. However, in this case we're using the root window, so
	 * we have to be more explicit.
	 */

	TkSetPixmapColormap(newPixmap, instancePtr->colormap);

	if (instancePtr->pixels != None) {
	    /*
	     * Copy any common pixels from the old pixmap and free it.
	     */

	    XCopyArea(instancePtr->display, instancePtr->pixels, newPixmap,
		    instancePtr->gc, validBox.x, validBox.y,
		    validBox.width, validBox.height, validBox.x, validBox.y);
	    Tk_FreePixmap(instancePtr->display, instancePtr->pixels);
	}
	instancePtr->pixels = newPixmap;
    }

    if ((instancePtr->width != masterPtr->width)
	    || (instancePtr->height != masterPtr->height)
	    || (instancePtr->error == NULL)) {
	if (masterPtr->height > 0 && masterPtr->width > 0) {
	    /*
	     * TODO: use attemptckalloc() here once there is a strategy that
	     * will allow us to recover from failure. Right now, there's no
	     * such possibility.
	     */

	    newError = ckalloc(masterPtr->height * masterPtr->width
		    * 3 * sizeof(schar));

	    /*
	     * Zero the new array so that we don't get bogus error values
	     * propagating into areas we dither later.
	     */

	    if ((instancePtr->error != NULL)
		    && ((instancePtr->width == masterPtr->width)
		    || (validBox.width == masterPtr->width))) {
		if (validBox.y > 0) {
		    memset(newError, 0, (size_t)
			    validBox.y * masterPtr->width * 3 * sizeof(schar));
		}
		h = validBox.y + validBox.height;
		if (h < masterPtr->height) {
		    memset(newError + h*masterPtr->width*3, 0,
			    (size_t) (masterPtr->height - h)
			    * masterPtr->width * 3 * sizeof(schar));
		}
	    } else {
		memset(newError, 0, (size_t)
			masterPtr->height * masterPtr->width *3*sizeof(schar));
	    }
	} else {
	    newError = NULL;
	}

	if (instancePtr->error != NULL) {
	    /*
	     * Copy the common area over to the new array and free the old
	     * array.
	     */

	    if (masterPtr->width == instancePtr->width) {
		offset = validBox.y * masterPtr->width * 3;
		memcpy(newError + offset, instancePtr->error + offset,
			(size_t) (validBox.height
			* masterPtr->width * 3 * sizeof(schar)));

	    } else if (validBox.width > 0 && validBox.height > 0) {
		errDestPtr = newError +
			(validBox.y * masterPtr->width + validBox.x) * 3;
		errSrcPtr = instancePtr->error +
			(validBox.y * instancePtr->width + validBox.x) * 3;

		for (h = validBox.height; h > 0; --h) {
		    memcpy(errDestPtr, errSrcPtr,
			    validBox.width * 3 * sizeof(schar));
		    errDestPtr += masterPtr->width * 3;
		    errSrcPtr += instancePtr->width * 3;
		}
	    }
	    ckfree(instancePtr->error);
	}

	instancePtr->error = newError;
    }

    instancePtr->width = masterPtr->width;
    instancePtr->height = masterPtr->height;
}

/*
 *----------------------------------------------------------------------
 *
 * IsValidPalette --
 *
 *	This function is called to check whether a value given for the
 *	-palette option is valid for a particular instance of a photo image.
 *
 * Results:
 *	A boolean value: 1 if the palette is acceptable, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
IsValidPalette(
    PhotoInstance *instancePtr,	/* Instance to which the palette specification
				 * is to be applied. */
    const char *palette)	/* Palette specification string. */
{
    int nRed, nGreen, nBlue, mono, numColors;
    char *endp;

    /*
     * First parse the specification: it must be of the form %d or %d/%d/%d.
     */

    nRed = strtol(palette, &endp, 10);
    if ((endp == palette) || ((*endp != 0) && (*endp != '/'))
	    || (nRed < 2) || (nRed > 256)) {
	return 0;
    }

    if (*endp == 0) {
	mono = 1;
	nGreen = nBlue = nRed;
    } else {
	palette = endp + 1;
	nGreen = strtol(palette, &endp, 10);
	if ((endp == palette) || (*endp != '/') || (nGreen < 2)
		|| (nGreen > 256)) {
	    return 0;
	}
	palette = endp + 1;
	nBlue = strtol(palette, &endp, 10);
	if ((endp == palette) || (*endp != 0) || (nBlue < 2)
		|| (nBlue > 256)) {
	    return 0;
	}
	mono = 0;
    }

    switch (instancePtr->visualInfo.c_class) {
    case DirectColor:
    case TrueColor:
	if ((nRed > (1 << CountBits(instancePtr->visualInfo.red_mask)))
		|| (nGreen>(1<<CountBits(instancePtr->visualInfo.green_mask)))
		|| (nBlue>(1<<CountBits(instancePtr->visualInfo.blue_mask)))) {
	    return 0;
	}
	break;
    case PseudoColor:
    case StaticColor:
	numColors = nRed;
	if (!mono) {
	    numColors *= nGreen * nBlue;
	}
	if (numColors > (1 << instancePtr->visualInfo.depth)) {
	    return 0;
	}
	break;
    case GrayScale:
    case StaticGray:
	if (!mono || (nRed > (1 << instancePtr->visualInfo.depth))) {
	    return 0;
	}
	break;
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * CountBits --
 *
 *	This function counts how many bits are set to 1 in `mask'.
 *
 * Results:
 *	The integer number of bits.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CountBits(
    pixel mask)			/* Value to count the 1 bits in. */
{
    int n;

    for (n=0 ; mask!=0 ; mask&=mask-1) {
	n++;
    }
    return n;
}

/*
 *----------------------------------------------------------------------
 *
 * GetColorTable --
 *
 *	This function is called to allocate a table of colormap information
 *	for an instance of a photo image. Only one such table is allocated for
 *	all photo instances using the same display, colormap, palette and
 *	gamma values, so that the application need only request a set of
 *	colors from the X server once for all such photo widgets. This
 *	function maintains a hash table to find previously-allocated
 *	ColorTables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new ColorTable may be allocated and placed in the hash table, and
 *	have colors allocated for it.
 *
 *----------------------------------------------------------------------
 */

static void
GetColorTable(
    PhotoInstance *instancePtr)	/* Instance needing a color table. */
{
    ColorTable *colorPtr;
    Tcl_HashEntry *entry;
    ColorTableId id;
    int isNew;

    /*
     * Look for an existing ColorTable in the hash table.
     */

    memset(&id, 0, sizeof(id));
    id.display = instancePtr->display;
    id.colormap = instancePtr->colormap;
    id.palette = instancePtr->palette;
    id.gamma = instancePtr->gamma;
    if (!imgPhotoColorHashInitialized) {
	Tcl_InitHashTable(&imgPhotoColorHash, N_COLOR_HASH);
	imgPhotoColorHashInitialized = 1;
    }
    entry = Tcl_CreateHashEntry(&imgPhotoColorHash, (char *) &id, &isNew);

    if (!isNew) {
	/*
	 * Re-use the existing entry.
	 */

	colorPtr = Tcl_GetHashValue(entry);
    } else {
	/*
	 * No color table currently available; need to make one.
	 */

	colorPtr = ckalloc(sizeof(ColorTable));

	/*
	 * The following line of code should not normally be needed due to the
	 * assignment in the following line. However, it compensates for bugs
	 * in some compilers (HP, for example) where sizeof(ColorTable) is 24
	 * but the assignment only copies 20 bytes, leaving 4 bytes
	 * uninitialized; these cause problems when using the id for lookups
	 * in imgPhotoColorHash, and can result in core dumps.
	 */

	memset(&colorPtr->id, 0, sizeof(ColorTableId));
	colorPtr->id = id;
	Tk_PreserveColormap(colorPtr->id.display, colorPtr->id.colormap);
	colorPtr->flags = 0;
	colorPtr->refCount = 0;
	colorPtr->liveRefCount = 0;
	colorPtr->numColors = 0;
	colorPtr->visualInfo = instancePtr->visualInfo;
	colorPtr->pixelMap = NULL;
	Tcl_SetHashValue(entry, colorPtr);
    }

    colorPtr->refCount++;
    colorPtr->liveRefCount++;
    instancePtr->colorTablePtr = colorPtr;
    if (colorPtr->flags & DISPOSE_PENDING) {
	Tcl_CancelIdleCall(DisposeColorTable, colorPtr);
	colorPtr->flags &= ~DISPOSE_PENDING;
    }

    /*
     * Allocate colors for this color table if necessary.
     */

    if ((colorPtr->numColors == 0) && !(colorPtr->flags & BLACK_AND_WHITE)) {
	AllocateColors(colorPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeColorTable --
 *
 *	This function is called when an instance ceases using a color table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If no other instances are using this color table, a when-idle handler
 *	is registered to free up the color table and the colors allocated for
 *	it.
 *
 *----------------------------------------------------------------------
 */

static void
FreeColorTable(
    ColorTable *colorPtr,	/* Pointer to the color table which is no
				 * longer required by an instance. */
    int force)			/* Force free to happen immediately. */
{
    colorPtr->refCount--;
    if (colorPtr->refCount > 0) {
	return;
    }

    if (force) {
	if (colorPtr->flags & DISPOSE_PENDING) {
	    Tcl_CancelIdleCall(DisposeColorTable, colorPtr);
	    colorPtr->flags &= ~DISPOSE_PENDING;
	}
	DisposeColorTable(colorPtr);
    } else if (!(colorPtr->flags & DISPOSE_PENDING)) {
	Tcl_DoWhenIdle(DisposeColorTable, colorPtr);
	colorPtr->flags |= DISPOSE_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AllocateColors --
 *
 *	This function allocates the colors required by a color table, and sets
 *	up the fields in the color table data structure which are used in
 *	dithering.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Colors are allocated from the X server. Fields in the color table data
 *	structure are updated.
 *
 *----------------------------------------------------------------------
 */

static void
AllocateColors(
    ColorTable *colorPtr)	/* Pointer to the color table requiring colors
				 * to be allocated. */
{
    int i, r, g, b, rMult, mono;
    int numColors, nRed, nGreen, nBlue;
    double fr, fg, fb, igam;
    XColor *colors;
    unsigned long *pixels;

    /*
     * 16-bit intensity value for i/n of full intensity.
     */
#define CFRAC(i, n)	((i) * 65535 / (n))

    /* As for CFRAC, but apply exponent of g. */
#define CGFRAC(i, n, g)	((int)(65535 * pow((double)(i) / (n), (g))))

    /*
     * First parse the palette specification to get the required number of
     * shades of each primary.
     */

    mono = sscanf(colorPtr->id.palette, "%d/%d/%d", &nRed, &nGreen, &nBlue)
	    <= 1;
    igam = 1.0 / colorPtr->id.gamma;

    /*
     * Each time around this loop, we reduce the number of colors we're trying
     * to allocate until we succeed in allocating all of the colors we need.
     */

    for (;;) {
	/*
	 * If we are using 1 bit/pixel, we don't need to allocate any colors
	 * (we just use the foreground and background colors in the GC).
	 */

	if (mono && (nRed <= 2)) {
	    colorPtr->flags |= BLACK_AND_WHITE;
	    return;
	}

	/*
	 * Calculate the RGB coordinates of the colors we want to allocate and
	 * store them in *colors.
	 */

	if ((colorPtr->visualInfo.c_class == DirectColor)
		|| (colorPtr->visualInfo.c_class == TrueColor)) {

	    /*
	     * Direct/True Color: allocate shades of red, green, blue
	     * independently.
	     */

	    if (mono) {
		numColors = nGreen = nBlue = nRed;
	    } else {
		numColors = MAX(MAX(nRed, nGreen), nBlue);
	    }
	    colors = ckalloc(numColors * sizeof(XColor));

	    for (i = 0; i < numColors; ++i) {
		if (igam == 1.0) {
		    colors[i].red = CFRAC(i, nRed - 1);
		    colors[i].green = CFRAC(i, nGreen - 1);
		    colors[i].blue = CFRAC(i, nBlue - 1);
		} else {
		    colors[i].red = CGFRAC(i, nRed - 1, igam);
		    colors[i].green = CGFRAC(i, nGreen - 1, igam);
		    colors[i].blue = CGFRAC(i, nBlue - 1, igam);
		}
	    }
	} else {
	    /*
	     * PseudoColor, StaticColor, GrayScale or StaticGray visual: we
	     * have to allocate each color in the color cube separately.
	     */

	    numColors = (mono) ? nRed: (nRed * nGreen * nBlue);
	    colors = ckalloc(numColors * sizeof(XColor));

	    if (!mono) {
		/*
		 * Color display using a PseudoColor or StaticColor visual.
		 */

		i = 0;
		for (r = 0; r < nRed; ++r) {
		    for (g = 0; g < nGreen; ++g) {
			for (b = 0; b < nBlue; ++b) {
			    if (igam == 1.0) {
				colors[i].red = CFRAC(r, nRed - 1);
				colors[i].green = CFRAC(g, nGreen - 1);
				colors[i].blue = CFRAC(b, nBlue - 1);
			    } else {
				colors[i].red = CGFRAC(r, nRed - 1, igam);
				colors[i].green = CGFRAC(g, nGreen - 1, igam);
				colors[i].blue = CGFRAC(b, nBlue - 1, igam);
			    }
			    i++;
			}
		    }
		}
	    } else {
		/*
		 * Monochrome display - allocate the shades of gray we want.
		 */

		for (i = 0; i < numColors; ++i) {
		    if (igam == 1.0) {
			r = CFRAC(i, numColors - 1);
		    } else {
			r = CGFRAC(i, numColors - 1, igam);
		    }
		    colors[i].red = colors[i].green = colors[i].blue = r;
		}
	    }
	}

	/*
	 * Now try to allocate the colors we've calculated.
	 */

	pixels = ckalloc(numColors * sizeof(unsigned long));
	for (i = 0; i < numColors; ++i) {
	    if (!XAllocColor(colorPtr->id.display, colorPtr->id.colormap,
		    &colors[i])) {
		/*
		 * Can't get all the colors we want in the default colormap;
		 * first try freeing colors from other unused color tables.
		 */

		if (!ReclaimColors(&colorPtr->id, numColors - i)
			|| !XAllocColor(colorPtr->id.display,
			colorPtr->id.colormap, &colors[i])) {
		    /*
		     * Still can't allocate the color.
		     */

		    break;
		}
	    }
	    pixels[i] = colors[i].pixel;
	}

	/*
	 * If we didn't get all of the colors, reduce the resolution of the
	 * color cube, free the ones we got, and try again.
	 */

	if (i >= numColors) {
	    break;
	}
	XFreeColors(colorPtr->id.display, colorPtr->id.colormap, pixels, i, 0);
	ckfree(colors);
	ckfree(pixels);

	if (!mono) {
	    if ((nRed == 2) && (nGreen == 2) && (nBlue == 2)) {
		/*
		 * Fall back to 1-bit monochrome display.
		 */

		mono = 1;
	    } else {
		/*
		 * Reduce the number of shades of each primary to about 3/4 of
		 * the previous value. This should reduce the total number of
		 * colors required to about half the previous value for
		 * PseudoColor displays.
		 */

		nRed = (nRed * 3 + 2) / 4;
		nGreen = (nGreen * 3 + 2) / 4;
		nBlue = (nBlue * 3 + 2) / 4;
	    }
	} else {
	    /*
	     * Reduce the number of shades of gray to about 1/2.
	     */

	    nRed = nRed / 2;
	}
    }

    /*
     * We have allocated all of the necessary colors: fill in various fields
     * of the ColorTable record.
     */

    if (!mono) {
	colorPtr->flags |= COLOR_WINDOW;

	/*
	 * The following is a hairy hack. We only want to index into the
	 * pixelMap on colormap displays. However, if the display is on
	 * Windows, then we actually want to store the index not the value
	 * since we will be passing the color table into the TkPutImage call.
	 */

#ifndef _WIN32
	if ((colorPtr->visualInfo.c_class != DirectColor)
		&& (colorPtr->visualInfo.c_class != TrueColor)) {
	    colorPtr->flags |= MAP_COLORS;
	}
#endif /* _WIN32 */
    }

    colorPtr->numColors = numColors;
    colorPtr->pixelMap = pixels;

    /*
     * Set up quantization tables for dithering.
     */

    rMult = nGreen * nBlue;
    for (i = 0; i < 256; ++i) {
	r = (i * (nRed - 1) + 127) / 255;
	if (mono) {
	    fr = (double) colors[r].red / 65535.0;
	    if (colorPtr->id.gamma != 1.0 ) {
		fr = pow(fr, colorPtr->id.gamma);
	    }
	    colorPtr->colorQuant[0][i] = (int)(fr * 255.99);
	    colorPtr->redValues[i] = colors[r].pixel;
	} else {
	    g = (i * (nGreen - 1) + 127) / 255;
	    b = (i * (nBlue - 1) + 127) / 255;
	    if ((colorPtr->visualInfo.c_class == DirectColor)
		    || (colorPtr->visualInfo.c_class == TrueColor)) {
		colorPtr->redValues[i] =
			colors[r].pixel & colorPtr->visualInfo.red_mask;
		colorPtr->greenValues[i] =
			colors[g].pixel & colorPtr->visualInfo.green_mask;
		colorPtr->blueValues[i] =
			colors[b].pixel & colorPtr->visualInfo.blue_mask;
	    } else {
		r *= rMult;
		g *= nBlue;
		colorPtr->redValues[i] = r;
		colorPtr->greenValues[i] = g;
		colorPtr->blueValues[i] = b;
	    }
	    fr = (double) colors[r].red / 65535.0;
	    fg = (double) colors[g].green / 65535.0;
	    fb = (double) colors[b].blue / 65535.0;
	    if (colorPtr->id.gamma != 1.0) {
		fr = pow(fr, colorPtr->id.gamma);
		fg = pow(fg, colorPtr->id.gamma);
		fb = pow(fb, colorPtr->id.gamma);
	    }
	    colorPtr->colorQuant[0][i] = (int)(fr * 255.99);
	    colorPtr->colorQuant[1][i] = (int)(fg * 255.99);
	    colorPtr->colorQuant[2][i] = (int)(fb * 255.99);
	}
    }

    ckfree(colors);
}

/*
 *----------------------------------------------------------------------
 *
 * DisposeColorTable --
 *
 *	Release a color table and its associated resources.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The colors in the argument color table are freed, as is the color
 *	table structure itself. The color table is removed from the hash table
 *	which is used to locate color tables.
 *
 *----------------------------------------------------------------------
 */

static void
DisposeColorTable(
    ClientData clientData)	/* Pointer to the ColorTable whose
				 * colors are to be released. */
{
    ColorTable *colorPtr = clientData;
    Tcl_HashEntry *entry;

    if (colorPtr->pixelMap != NULL) {
	if (colorPtr->numColors > 0) {
	    XFreeColors(colorPtr->id.display, colorPtr->id.colormap,
		    colorPtr->pixelMap, colorPtr->numColors, 0);
	    Tk_FreeColormap(colorPtr->id.display, colorPtr->id.colormap);
	}
	ckfree(colorPtr->pixelMap);
    }

    entry = Tcl_FindHashEntry(&imgPhotoColorHash, (char *) &colorPtr->id);
    if (entry == NULL) {
	Tcl_Panic("DisposeColorTable couldn't find hash entry");
    }
    Tcl_DeleteHashEntry(entry);

    ckfree(colorPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ReclaimColors --
 *
 *	This function is called to try to free up colors in the colormap used
 *	by a color table. It looks for other color tables with the same
 *	colormap and with a zero live reference count, and frees their colors.
 *	It only does so if there is the possibility of freeing up at least
 *	`numColors' colors.
 *
 * Results:
 *	The return value is TRUE if any colors were freed, FALSE otherwise.
 *
 * Side effects:
 *	ColorTables which are not currently in use may lose their color
 *	allocations.
 *
 *----------------------------------------------------------------------
 */

static int
ReclaimColors(
    ColorTableId *id,		/* Pointer to information identifying
				 * the color table which needs more colors. */
    int numColors)		/* Number of colors required. */
{
    Tcl_HashSearch srch;
    Tcl_HashEntry *entry;
    ColorTable *colorPtr;
    int nAvail = 0;

    /*
     * First scan through the color hash table to get an upper bound on how
     * many colors we might be able to free.
     */

    entry = Tcl_FirstHashEntry(&imgPhotoColorHash, &srch);
    while (entry != NULL) {
	colorPtr = Tcl_GetHashValue(entry);
	if ((colorPtr->id.display == id->display)
		&& (colorPtr->id.colormap == id->colormap)
		&& (colorPtr->liveRefCount == 0 )&& (colorPtr->numColors != 0)
		&& ((colorPtr->id.palette != id->palette)
			|| (colorPtr->id.gamma != id->gamma))) {
	    /*
	     * We could take this guy's colors off him.
	     */

	    nAvail += colorPtr->numColors;
	}
	entry = Tcl_NextHashEntry(&srch);
    }

    /*
     * nAvail is an (over)estimate of the number of colors we could free.
     */

    if (nAvail < numColors) {
	return 0;
    }

    /*
     * Scan through a second time freeing colors.
     */

    entry = Tcl_FirstHashEntry(&imgPhotoColorHash, &srch);
    while ((entry != NULL) && (numColors > 0)) {
	colorPtr = Tcl_GetHashValue(entry);
	if ((colorPtr->id.display == id->display)
		&& (colorPtr->id.colormap == id->colormap)
		&& (colorPtr->liveRefCount == 0) && (colorPtr->numColors != 0)
		&& ((colorPtr->id.palette != id->palette)
			|| (colorPtr->id.gamma != id->gamma))) {
	    /*
	     * Free the colors that this ColorTable has.
	     */

	    XFreeColors(colorPtr->id.display, colorPtr->id.colormap,
		    colorPtr->pixelMap, colorPtr->numColors, 0);
	    numColors -= colorPtr->numColors;
	    colorPtr->numColors = 0;
	    ckfree(colorPtr->pixelMap);
	    colorPtr->pixelMap = NULL;
	}

	entry = Tcl_NextHashEntry(&srch);
    }
    return 1;			/* We freed some colors. */
}

/*
 *----------------------------------------------------------------------
 *
 * TkImgDisposeInstance --
 *
 *	This function is called to finally free up an instance of a photo
 *	image which is no longer required.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The instance data structure and the resources it references are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkImgDisposeInstance(
    ClientData clientData)	/* Pointer to the instance whose resources are
				 * to be released. */
{
    PhotoInstance *instancePtr = clientData;
    PhotoInstance *prevPtr;

    if (instancePtr->pixels != None) {
	Tk_FreePixmap(instancePtr->display, instancePtr->pixels);
    }
    if (instancePtr->gc != NULL) {
	Tk_FreeGC(instancePtr->display, instancePtr->gc);
    }
    if (instancePtr->imagePtr != NULL) {
	XDestroyImage(instancePtr->imagePtr);
    }
    if (instancePtr->error != NULL) {
	ckfree(instancePtr->error);
    }
    if (instancePtr->colorTablePtr != NULL) {
	FreeColorTable(instancePtr->colorTablePtr, 1);
    }

    if (instancePtr->masterPtr->instancePtr == instancePtr) {
	instancePtr->masterPtr->instancePtr = instancePtr->nextPtr;
    } else {
	for (prevPtr = instancePtr->masterPtr->instancePtr;
		prevPtr->nextPtr != instancePtr; prevPtr = prevPtr->nextPtr) {
	    /* Empty loop body. */
	}
	prevPtr->nextPtr = instancePtr->nextPtr;
    }
    Tk_FreeColormap(instancePtr->display, instancePtr->colormap);
    ckfree(instancePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkImgDitherInstance --
 *
 *	This function is called to update an area of an instance's pixmap by
 *	dithering the corresponding area of the master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The instance's pixmap gets updated.
 *
 *----------------------------------------------------------------------
 */

void
TkImgDitherInstance(
    PhotoInstance *instancePtr,	/* The instance to be updated. */
    int xStart, int yStart,	/* Coordinates of the top-left pixel in the
				 * block to be dithered. */
    int width, int height)	/* Dimensions of the block to be dithered. */
{
    PhotoMaster *masterPtr = instancePtr->masterPtr;
    ColorTable *colorPtr = instancePtr->colorTablePtr;
    XImage *imagePtr;
    int nLines, bigEndian, i, c, x, y, xEnd, doDithering = 1;
    int bitsPerPixel, bytesPerLine, lineLength;
    unsigned char *srcLinePtr;
    schar *errLinePtr;
    pixel firstBit, word, mask;

    /*
     * Turn dithering off in certain cases where it is not needed (TrueColor,
     * DirectColor with many colors).
     */

    if ((colorPtr->visualInfo.c_class == DirectColor)
	    || (colorPtr->visualInfo.c_class == TrueColor)) {
	int nRed, nGreen, nBlue, result;

	result = sscanf(colorPtr->id.palette, "%d/%d/%d", &nRed,
		&nGreen, &nBlue);
	if ((nRed >= 256)
		&& ((result == 1) || ((nGreen >= 256) && (nBlue >= 256)))) {
	    doDithering = 0;
	}
    }

    /*
     * First work out how many lines to do at a time, then how many bytes
     * we'll need for pixel storage, and allocate it.
     */

    nLines = (MAX_PIXELS + width - 1) / width;
    if (nLines < 1) {
	nLines = 1;
    }
    if (nLines > height ) {
	nLines = height;
    }

    imagePtr = instancePtr->imagePtr;
    if (imagePtr == NULL) {
	return;			/* We must be really tight on memory. */
    }
    bitsPerPixel = imagePtr->bits_per_pixel;
    bytesPerLine = ((bitsPerPixel * width + 31) >> 3) & ~3;
    imagePtr->width = width;
    imagePtr->height = nLines;
    imagePtr->bytes_per_line = bytesPerLine;

    /*
     * TODO: use attemptckalloc() here once we have some strategy for
     * recovering from the failure.
     */

    imagePtr->data = ckalloc(imagePtr->bytes_per_line * nLines);
    bigEndian = imagePtr->bitmap_bit_order == MSBFirst;
    firstBit = bigEndian? (1 << (imagePtr->bitmap_unit - 1)): 1;

    lineLength = masterPtr->width * 3;
    srcLinePtr = masterPtr->pix32 + (yStart * masterPtr->width + xStart) * 4;
    errLinePtr = instancePtr->error + yStart * lineLength + xStart * 3;
    xEnd = xStart + width;

    /*
     * Loop over the image, doing at most nLines lines before updating the
     * screen image.
     */

    for (; height > 0; height -= nLines) {
	unsigned char *dstLinePtr = (unsigned char *) imagePtr->data;
	int yEnd;

	if (nLines > height) {
	    nLines = height;
	}
	yEnd = yStart + nLines;
	for (y = yStart; y < yEnd; ++y) {
	    unsigned char *srcPtr = srcLinePtr;
	    schar *errPtr = errLinePtr;
	    unsigned char *destBytePtr = dstLinePtr;
	    pixel *destLongPtr = (pixel *) dstLinePtr;

	    if (colorPtr->flags & COLOR_WINDOW) {
		/*
		 * Color window. We dither the three components independently,
		 * using Floyd-Steinberg dithering, which propagates errors
		 * from the quantization of pixels to the pixels below and to
		 * the right.
		 */

		for (x = xStart; x < xEnd; ++x) {
		    int col[3];

		    if (doDithering) {
			for (i = 0; i < 3; ++i) {
			    /*
			     * Compute the error propagated into this pixel
			     * for this component. If e[x,y] is the array of
			     * quantization error values, we compute
			     *     7/16 * e[x-1,y] + 1/16 * e[x-1,y-1]
			     *   + 5/16 * e[x,y-1] + 3/16 * e[x+1,y-1]
			     * and round it to an integer.
			     *
			     * The expression ((c + 2056) >> 4) - 128 computes
			     * round(c / 16), and works correctly on machines
			     * without a sign-extending right shift.
			     */

			    c = (x > 0) ? errPtr[-3] * 7: 0;
			    if (y > 0) {
				if (x > 0) {
				    c += errPtr[-lineLength-3];
				}
				c += errPtr[-lineLength] * 5;
				if ((x + 1) < masterPtr->width) {
				    c += errPtr[-lineLength+3] * 3;
				}
			    }

			    /*
			     * Add the propagated error to the value of this
			     * component, quantize it, and store the
			     * quantization error.
			     */

			    c = ((c + 2056) >> 4) - 128 + *srcPtr++;
			    if (c < 0) {
				c = 0;
			    } else if (c > 255) {
				c = 255;
			    }
			    col[i] = colorPtr->colorQuant[i][c];
			    *errPtr++ = c - col[i];
			}
		    } else {
			/*
			 * Output is virtually continuous in this case, so
			 * don't bother dithering.
			 */

			col[0] = *srcPtr++;
			col[1] = *srcPtr++;
			col[2] = *srcPtr++;
		    }
		    srcPtr++;

		    /*
		     * Translate the quantized component values into an X
		     * pixel value, and store it in the image.
		     */

		    i = colorPtr->redValues[col[0]]
			    + colorPtr->greenValues[col[1]]
			    + colorPtr->blueValues[col[2]];
		    if (colorPtr->flags & MAP_COLORS) {
			i = colorPtr->pixelMap[i];
		    }
		    switch (bitsPerPixel) {
		    case NBBY:
			*destBytePtr++ = i;
			break;
#ifndef _WIN32
			/*
			 * This case is not valid for Windows because the
			 * image format is different from the pixel format in
			 * Win32. Eventually we need to fix the image code in
			 * Tk to use the Windows native image ordering. This
			 * would speed up the image code for all of the common
			 * sizes.
			 */

		    case NBBY * sizeof(pixel):
			*destLongPtr++ = i;
			break;
#endif
		    default:
			XPutPixel(imagePtr, x - xStart, y - yStart,
				(unsigned) i);
		    }
		}

	    } else if (bitsPerPixel > 1) {
		/*
		 * Multibit monochrome window. The operation here is similar
		 * to the color window case above, except that there is only
		 * one component. If the master image is in color, use the
		 * luminance computed as
		 *	0.344 * red + 0.5 * green + 0.156 * blue.
		 */

		for (x = xStart; x < xEnd; ++x) {
		    c = (x > 0) ? errPtr[-1] * 7: 0;
		    if (y > 0) {
			if (x > 0) {
			    c += errPtr[-lineLength-1];
			}
			c += errPtr[-lineLength] * 5;
			if (x + 1 < masterPtr->width) {
			    c += errPtr[-lineLength+1] * 3;
			}
		    }
		    c = ((c + 2056) >> 4) - 128;

		    if (masterPtr->flags & COLOR_IMAGE) {
			c += (unsigned) (srcPtr[0] * 11 + srcPtr[1] * 16
				+ srcPtr[2] * 5 + 16) >> 5;
		    } else {
			c += srcPtr[0];
		    }
		    srcPtr += 4;

		    if (c < 0) {
			c = 0;
		    } else if (c > 255) {
			c = 255;
		    }
		    i = colorPtr->colorQuant[0][c];
		    *errPtr++ = c - i;
		    i = colorPtr->redValues[i];
		    switch (bitsPerPixel) {
		    case NBBY:
			*destBytePtr++ = i;
			break;
#ifndef _WIN32
			/*
			 * This case is not valid for Windows because the
			 * image format is different from the pixel format in
			 * Win32. Eventually we need to fix the image code in
			 * Tk to use the Windows native image ordering. This
			 * would speed up the image code for all of the common
			 * sizes.
			 */

		    case NBBY * sizeof(pixel):
			*destLongPtr++ = i;
			break;
#endif
		    default:
			XPutPixel(imagePtr, x - xStart, y - yStart,
				(unsigned) i);
		    }
		}
	    } else {
		/*
		 * 1-bit monochrome window. This is similar to the multibit
		 * monochrome case above, except that the quantization is
		 * simpler (we only have black = 0 and white = 255), and we
		 * produce an XY-Bitmap.
		 */

		word = 0;
		mask = firstBit;
		for (x = xStart; x < xEnd; ++x) {
		    /*
		     * If we have accumulated a whole word, store it in the
		     * image and start a new word.
		     */

		    if (mask == 0) {
			*destLongPtr++ = word;
			mask = firstBit;
			word = 0;
		    }

		    c = (x > 0) ? errPtr[-1] * 7: 0;
		    if (y > 0) {
			if (x > 0) {
			    c += errPtr[-lineLength-1];
			}
			c += errPtr[-lineLength] * 5;
			if (x + 1 < masterPtr->width) {
			    c += errPtr[-lineLength+1] * 3;
			}
		    }
		    c = ((c + 2056) >> 4) - 128;

		    if (masterPtr->flags & COLOR_IMAGE) {
			c += (unsigned)(srcPtr[0] * 11 + srcPtr[1] * 16
				+ srcPtr[2] * 5 + 16) >> 5;
		    } else {
			c += srcPtr[0];
		    }
		    srcPtr += 4;

		    if (c < 0) {
			c = 0;
		    } else if (c > 255) {
			c = 255;
		    }
		    if (c >= 128) {
			word |= mask;
			*errPtr++ = c - 255;
		    } else {
			*errPtr++ = c;
		    }
		    mask = bigEndian? (mask >> 1): (mask << 1);
		}
		*destLongPtr = word;
	    }
	    srcLinePtr += masterPtr->width * 4;
	    errLinePtr += lineLength;
	    dstLinePtr += bytesPerLine;
	}

	/*
	 * Update the pixmap for this instance with the block of pixels that
	 * we have just computed.
	 */

	TkPutImage(colorPtr->pixelMap, colorPtr->numColors,
		instancePtr->display, instancePtr->pixels,
		instancePtr->gc, imagePtr, 0, 0, xStart, yStart,
		(unsigned) width, (unsigned) nLines);
	yStart = yEnd;
    }

    ckfree(imagePtr->data);
    imagePtr->data = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkImgResetDither --
 *
 *	This function is called to eliminate the content of a photo instance's
 *	dither error buffer. It's called when the overall image is blanked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The instance's dither buffer gets cleared.
 *
 *----------------------------------------------------------------------
 */

void
TkImgResetDither(
    PhotoInstance *instancePtr)
{
    if (instancePtr->error) {
	memset(instancePtr->error, 0,
	       /*(size_t)*/ (instancePtr->masterPtr->width
		* instancePtr->masterPtr->height * 3 * sizeof(schar)));
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
