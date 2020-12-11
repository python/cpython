/*
 *	Image specifications and image element factory.
 *
 * Copyright (C) 2004 Pat Thoyts <patthoyts@users.sf.net>
 * Copyright (C) 2004 Joe English
 *
 * An imageSpec is a multi-element list; the first element
 * is the name of the default image to use, the remainder of the
 * list is a sequence of statespec/imagename options as per
 * [style map].
 */

#include <string.h>
#include <tk.h>
#include "ttkTheme.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/*------------------------------------------------------------------------
 * +++ ImageSpec management.
 */

struct TtkImageSpec {
    Tk_Image 		baseImage;	/* Base image to use */
    int 		mapCount;	/* #state-specific overrides */
    Ttk_StateSpec	*states;	/* array[mapCount] of states ... */
    Tk_Image		*images;	/* ... per-state images to use */
    Tk_ImageChangedProc *imageChanged;
    ClientData		imageChangedClientData;
};

/* NullImageChanged --
 * 	Do-nothing Tk_ImageChangedProc.
 */
static void NullImageChanged(ClientData clientData,
    int x, int y, int width, int height, int imageWidth, int imageHeight)
{ /* No-op */ }

/* ImageSpecImageChanged --
 *     Image changes should trigger a repaint.
 */
static void ImageSpecImageChanged(ClientData clientData,
    int x, int y, int width, int height, int imageWidth, int imageHeight)
{
    Ttk_ImageSpec *imageSpec = (Ttk_ImageSpec *)clientData;
    if (imageSpec->imageChanged != NULL) {
	imageSpec->imageChanged(imageSpec->imageChangedClientData,
		x, y, width, height,
		imageWidth, imageHeight);
    }
}

/* TtkGetImageSpec --
 * 	Constructs a Ttk_ImageSpec * from a Tcl_Obj *.
 * 	Result must be released using TtkFreeImageSpec.
 *
 */
Ttk_ImageSpec *
TtkGetImageSpec(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr)
{
    return TtkGetImageSpecEx(interp, tkwin, objPtr, NULL, NULL);
}

/* TtkGetImageSpecEx --
 * 	Constructs a Ttk_ImageSpec * from a Tcl_Obj *.
 * 	Result must be released using TtkFreeImageSpec.
 * 	imageChangedProc will be called when not NULL when
 * 	the image changes to allow widgets to repaint.
 */
Ttk_ImageSpec *
TtkGetImageSpecEx(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr,
    Tk_ImageChangedProc *imageChangedProc, ClientData imageChangedClientData)
{
    Ttk_ImageSpec *imageSpec = 0;
    int i = 0, n = 0, objc;
    Tcl_Obj **objv;

    imageSpec = ckalloc(sizeof(*imageSpec));
    imageSpec->baseImage = 0;
    imageSpec->mapCount = 0;
    imageSpec->states = 0;
    imageSpec->images = 0;
    imageSpec->imageChanged = imageChangedProc;
    imageSpec->imageChangedClientData = imageChangedClientData;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	goto error;
    }

    if ((objc % 2) != 1) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"image specification must contain an odd number of elements",
		-1));
	    Tcl_SetErrorCode(interp, "TTK", "IMAGE", "SPEC", NULL);
	}
	goto error;
    }

    n = (objc - 1) / 2;
    imageSpec->states = ckalloc(n * sizeof(Ttk_StateSpec));
    imageSpec->images = ckalloc(n * sizeof(Tk_Image *));

    /* Get base image:
    */
    imageSpec->baseImage = Tk_GetImage(
	    interp, tkwin, Tcl_GetString(objv[0]), ImageSpecImageChanged, imageSpec);
    if (!imageSpec->baseImage) {
    	goto error;
    }

    /* Extract state and image specifications:
     */
    for (i = 0; i < n; ++i) {
	Tcl_Obj *stateSpec = objv[2*i + 1];
	const char *imageName = Tcl_GetString(objv[2*i + 2]);
	Ttk_StateSpec state;

	if (Ttk_GetStateSpecFromObj(interp, stateSpec, &state) != TCL_OK) {
	    goto error;
	}
	imageSpec->states[i] = state;

	imageSpec->images[i] = Tk_GetImage(
	    interp, tkwin, imageName, NullImageChanged, NULL);
	if (imageSpec->images[i] == NULL) {
	    goto error;
	}
	imageSpec->mapCount = i+1;
    }

    return imageSpec;

error:
    TtkFreeImageSpec(imageSpec);
    return NULL;
}

/* TtkFreeImageSpec --
 * 	Dispose of an image specification.
 */
void TtkFreeImageSpec(Ttk_ImageSpec *imageSpec)
{
    int i;

    for (i=0; i < imageSpec->mapCount; ++i) {
	Tk_FreeImage(imageSpec->images[i]);
    }

    if (imageSpec->baseImage) { Tk_FreeImage(imageSpec->baseImage); }
    if (imageSpec->states) { ckfree(imageSpec->states); }
    if (imageSpec->images) { ckfree(imageSpec->images); }

    ckfree(imageSpec);
}

/* TtkSelectImage --
 * 	Return a state-specific image from an ImageSpec
 */
Tk_Image TtkSelectImage(Ttk_ImageSpec *imageSpec, Ttk_State state)
{
    int i;
    for (i = 0; i < imageSpec->mapCount; ++i) {
	if (Ttk_StateMatches(state, imageSpec->states+i)) {
	    return imageSpec->images[i];
	}
    }
    return imageSpec->baseImage;
}

/*------------------------------------------------------------------------
 * +++ Drawing utilities.
 */

/* LPadding, CPadding, RPadding --
 * 	Split a box+padding pair into left, center, and right boxes.
 */
static Ttk_Box LPadding(Ttk_Box b, Ttk_Padding p)
    { return Ttk_MakeBox(b.x, b.y, p.left, b.height); }

static Ttk_Box CPadding(Ttk_Box b, Ttk_Padding p)
    { return Ttk_MakeBox(b.x+p.left, b.y, b.width-p.left-p.right, b.height); }

static Ttk_Box RPadding(Ttk_Box b, Ttk_Padding p)
    { return  Ttk_MakeBox(b.x+b.width-p.right, b.y, p.right, b.height); }

/* TPadding, MPadding, BPadding --
 * 	Split a box+padding pair into top, middle, and bottom parts.
 */
static Ttk_Box TPadding(Ttk_Box b, Ttk_Padding p)
    { return Ttk_MakeBox(b.x, b.y, b.width, p.top); }

static Ttk_Box MPadding(Ttk_Box b, Ttk_Padding p)
    { return Ttk_MakeBox(b.x, b.y+p.top, b.width, b.height-p.top-p.bottom); }

static Ttk_Box BPadding(Ttk_Box b, Ttk_Padding p)
    { return Ttk_MakeBox(b.x, b.y+b.height-p.bottom, b.width, p.bottom); }

/* Ttk_Fill --
 *	Fill the destination area of the drawable by replicating
 *	the source area of the image.
 */
static void Ttk_Fill(
    Tk_Window tkwin, Drawable d, Tk_Image image, Ttk_Box src, Ttk_Box dst)
{
    int dr = dst.x + dst.width;
    int db = dst.y + dst.height;
    int x,y;

    if (!(src.width && src.height && dst.width && dst.height))
	return;

    for (x = dst.x; x < dr; x += src.width) {
	int cw = MIN(src.width, dr - x);
	for (y = dst.y; y <= db; y += src.height) {
	    int ch = MIN(src.height, db - y);
	    Tk_RedrawImage(image, src.x, src.y, cw, ch, d, x, y);
	}
    }
}

/* Ttk_Stripe --
 * 	Fill a horizontal stripe of the destination drawable.
 */
static void Ttk_Stripe(
    Tk_Window tkwin, Drawable d, Tk_Image image,
    Ttk_Box src, Ttk_Box dst, Ttk_Padding p)
{
    Ttk_Fill(tkwin, d, image, LPadding(src,p), LPadding(dst,p));
    Ttk_Fill(tkwin, d, image, CPadding(src,p), CPadding(dst,p));
    Ttk_Fill(tkwin, d, image, RPadding(src,p), RPadding(dst,p));
}

/* Ttk_Tile --
 * 	Fill successive horizontal stripes of the destination drawable.
 */
static void Ttk_Tile(
    Tk_Window tkwin, Drawable d, Tk_Image image,
    Ttk_Box src, Ttk_Box dst, Ttk_Padding p)
{
    Ttk_Stripe(tkwin, d, image, TPadding(src,p), TPadding(dst,p), p);
    Ttk_Stripe(tkwin, d, image, MPadding(src,p), MPadding(dst,p), p);
    Ttk_Stripe(tkwin, d, image, BPadding(src,p), BPadding(dst,p), p);
}

/*------------------------------------------------------------------------
 * +++ Image element definition.
 */

typedef struct {		/* ClientData for image elements */
    Ttk_ImageSpec *imageSpec;	/* Image(s) to use */
    int minWidth;		/* Minimum width; overrides image width */
    int minHeight;		/* Minimum width; overrides image width */
    Ttk_Sticky sticky;		/* -stickiness specification */
    Ttk_Padding border;		/* Fixed border region */
    Ttk_Padding padding;	/* Internal padding */

#if TILE_07_COMPAT
    Ttk_ResourceCache cache;	/* Resource cache for images */
    Ttk_StateMap imageMap;	/* State-based lookup table for images */
#endif
} ImageData;

static void FreeImageData(void *clientData)
{
    ImageData *imageData = clientData;
    if (imageData->imageSpec)	{ TtkFreeImageSpec(imageData->imageSpec); }
#if TILE_07_COMPAT
    if (imageData->imageMap)	{ Tcl_DecrRefCount(imageData->imageMap); }
#endif
    ckfree(clientData);
}

static void ImageElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ImageData *imageData = clientData;
    Tk_Image image = imageData->imageSpec->baseImage;

    if (image) {
	Tk_SizeOfImage(image, widthPtr, heightPtr);
    }
    if (imageData->minWidth >= 0) {
	*widthPtr = imageData->minWidth;
    }
    if (imageData->minHeight >= 0) {
	*heightPtr = imageData->minHeight;
    }

    *paddingPtr = imageData->padding;
}

static void ImageElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    ImageData *imageData = clientData;
    Tk_Image image = 0;
    int imgWidth, imgHeight;
    Ttk_Box src, dst;

#if TILE_07_COMPAT
    if (imageData->imageMap) {
	Tcl_Obj *imageObj = Ttk_StateMapLookup(NULL,imageData->imageMap,state);
	if (imageObj) {
	    image = Ttk_UseImage(imageData->cache, tkwin, imageObj);
	}
    }
    if (!image) {
	image = TtkSelectImage(imageData->imageSpec, state);
    }
#else
    image = TtkSelectImage(imageData->imageSpec, state);
#endif

    if (!image) {
	return;
    }

    Tk_SizeOfImage(image, &imgWidth, &imgHeight);
    src = Ttk_MakeBox(0, 0, imgWidth, imgHeight);
    dst = Ttk_StickBox(b, imgWidth, imgHeight, imageData->sticky);

    Ttk_Tile(tkwin, d, image, src, dst, imageData->border);
}

static Ttk_ElementSpec ImageElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    ImageElementSize,
    ImageElementDraw
};

/*------------------------------------------------------------------------
 * +++ Image element factory.
 */
static int
Ttk_CreateImageElement(
    Tcl_Interp *interp,
    void *clientData,
    Ttk_Theme theme,
    const char *elementName,
    int objc, Tcl_Obj *const objv[])
{
    static const char *optionStrings[] =
	 { "-border","-height","-padding","-sticky","-width",NULL };
    enum { O_BORDER, O_HEIGHT, O_PADDING, O_STICKY, O_WIDTH };

    Ttk_ImageSpec *imageSpec = 0;
    ImageData *imageData = 0;
    int padding_specified = 0;
    int i;

    if (objc <= 0) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"Must supply a base image", -1));
	Tcl_SetErrorCode(interp, "TTK", "IMAGE", "BASE", NULL);
	return TCL_ERROR;
    }

    imageSpec = TtkGetImageSpec(interp, Tk_MainWindow(interp), objv[0]);
    if (!imageSpec) {
	return TCL_ERROR;
    }

    imageData = ckalloc(sizeof(*imageData));
    imageData->imageSpec = imageSpec;
    imageData->minWidth = imageData->minHeight = -1;
    imageData->sticky = TTK_FILL_BOTH;
    imageData->border = imageData->padding = Ttk_UniformPadding(0);
#if TILE_07_COMPAT
    imageData->cache = Ttk_GetResourceCache(interp);
    imageData->imageMap = 0;
#endif

    for (i = 1; i < objc; i += 2) {
	int option;

	if (i == objc - 1) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "Value for %s missing", Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TTK", "IMAGE", "VALUE", NULL);
	    goto error;
	}

#if TILE_07_COMPAT
	if (!strcmp("-map", Tcl_GetString(objv[i]))) {
	    imageData->imageMap = objv[i+1];
	    Tcl_IncrRefCount(imageData->imageMap);
	    continue;
	}
#endif

	if (Tcl_GetIndexFromObjStruct(interp, objv[i], optionStrings,
		sizeof(char *), "option", 0, &option) != TCL_OK) {
	    goto error;
	}

	switch (option) {
	    case O_BORDER:
		if (Ttk_GetBorderFromObj(interp, objv[i+1], &imageData->border)
			!= TCL_OK) {
		    goto error;
		}
		if (!padding_specified) {
		    imageData->padding = imageData->border;
		}
		break;
	    case O_PADDING:
		if (Ttk_GetBorderFromObj(interp, objv[i+1], &imageData->padding)
			!= TCL_OK) { goto error; }
		padding_specified = 1;
		break;
	    case O_WIDTH:
		if (Tcl_GetIntFromObj(interp, objv[i+1], &imageData->minWidth)
			!= TCL_OK) { goto error; }
		break;
	    case O_HEIGHT:
		if (Tcl_GetIntFromObj(interp, objv[i+1], &imageData->minHeight)
			!= TCL_OK) { goto error; }
		break;
	    case O_STICKY:
		if (Ttk_GetStickyFromObj(interp, objv[i+1], &imageData->sticky)
			!= TCL_OK) { goto error; }
	}
    }

    if (!Ttk_RegisterElement(interp, theme, elementName, &ImageElementSpec,
		imageData))
    {
	goto error;
    }

    Ttk_RegisterCleanup(interp, imageData, FreeImageData);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(elementName, -1));
    return TCL_OK;

error:
    FreeImageData(imageData);
    return TCL_ERROR;
}

MODULE_SCOPE
void TtkImage_Init(Tcl_Interp *interp)
{
    Ttk_RegisterElementFactory(interp, "image", Ttk_CreateImageElement, NULL);
}

/*EOF*/
