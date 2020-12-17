/*
 * tkImage.c --
 *
 *	This module implements the image protocol, which allows lots of
 *	different kinds of images to be used in lots of different widgets.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * Each call to Tk_GetImage returns a pointer to one of the following
 * structures, which is used as a token by clients (widgets) that display
 * images.
 */

typedef struct Image {
    Tk_Window tkwin;		/* Window passed to Tk_GetImage (needed to
				 * "re-get" the image later if the manager
				 * changes). */
    Display *display;		/* Display for tkwin. Needed because when the
				 * image is eventually freed tkwin may not
				 * exist anymore. */
    struct ImageMaster *masterPtr;
				/* Master for this image (identifiers image
				 * manager, for example). */
    ClientData instanceData;	/* One word argument to pass to image manager
				 * when dealing with this image instance. */
    Tk_ImageChangedProc *changeProc;
				/* Code in widget to call when image changes
				 * in a way that affects redisplay. */
    ClientData widgetClientData;/* Argument to pass to changeProc. */
    struct Image *nextPtr;	/* Next in list of all image instances
				 * associated with the same name. */
} Image;

/*
 * For each image master there is one of the following structures, which
 * represents a name in the image table and all of the images instantiated
 * from it. Entries in mainPtr->imageTable point to these structures.
 */

typedef struct ImageMaster {
    Tk_ImageType *typePtr;	/* Information about image type. NULL means
				 * that no image manager owns this image: the
				 * image was deleted. */
    ClientData masterData;	/* One-word argument to pass to image mgr when
				 * dealing with the master, as opposed to
				 * instances. */
    int width, height;		/* Last known dimensions for image. */
    Tcl_HashTable *tablePtr;	/* Pointer to hash table containing image (the
				 * imageTable field in some TkMainInfo
				 * structure). */
    Tcl_HashEntry *hPtr;	/* Hash entry in mainPtr->imageTable for this
				 * structure (used to delete the hash
				 * entry). */
    Image *instancePtr;		/* Pointer to first in list of instances
				 * derived from this name. */
    int deleted;		/* Flag set when image is being deleted. */
    TkWindow *winPtr;		/* Main window of interpreter (used to detect
				 * when the world is falling apart.) */
} ImageMaster;

typedef struct {
    Tk_ImageType *imageTypeList;/* First in a list of all known image
				 * types. */
    Tk_ImageType *oldImageTypeList;
				/* First in a list of all known old-style
				 * image types. */
    int initialized;		/* Set to 1 if we've initialized the
				 * structure. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Prototypes for local functions:
 */

static void		ImageTypeThreadExitProc(ClientData clientData);
static void		DeleteImage(ImageMaster *masterPtr);
static void		EventuallyDeleteImage(ImageMaster *masterPtr,
			    int forgetImageHashNow);

/*
 *----------------------------------------------------------------------
 *
 * ImageTypeThreadExitProc --
 *
 *	Clean up the registered list of image types.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The thread's linked lists of photo image formats is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
ImageTypeThreadExitProc(
    ClientData clientData)	/* not used */
{
    Tk_ImageType *freePtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    while (tsdPtr->oldImageTypeList != NULL) {
	freePtr = tsdPtr->oldImageTypeList;
	tsdPtr->oldImageTypeList = tsdPtr->oldImageTypeList->nextPtr;
	ckfree(freePtr);
    }
    while (tsdPtr->imageTypeList != NULL) {
	freePtr = tsdPtr->imageTypeList;
	tsdPtr->imageTypeList = tsdPtr->imageTypeList->nextPtr;
	ckfree(freePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateOldImageType, Tk_CreateImageType --
 *
 *	This function is invoked by an image manager to tell Tk about a new
 *	kind of image and the functions that manage the new type. The function
 *	is typically invoked during Tcl_AppInit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The new image type is entered into a table used in the "image create"
 *	command.
 *
 *----------------------------------------------------------------------
 */

void
Tk_CreateOldImageType(
    const Tk_ImageType *typePtr)
				/* Structure describing the type. All of the
				 * fields except "nextPtr" must be filled in
				 * by caller. */
{
    Tk_ImageType *copyPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	Tcl_CreateThreadExitHandler(ImageTypeThreadExitProc, NULL);
    }
    copyPtr = ckalloc(sizeof(Tk_ImageType));
    *copyPtr = *typePtr;
    copyPtr->nextPtr = tsdPtr->oldImageTypeList;
    tsdPtr->oldImageTypeList = copyPtr;
}

void
Tk_CreateImageType(
    const Tk_ImageType *typePtr)
				/* Structure describing the type. All of the
				 * fields except "nextPtr" must be filled in
				 * by caller. */
{
    Tk_ImageType *copyPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	Tcl_CreateThreadExitHandler(ImageTypeThreadExitProc, NULL);
    }
    copyPtr = ckalloc(sizeof(Tk_ImageType));
    *copyPtr = *typePtr;
    copyPtr->nextPtr = tsdPtr->imageTypeList;
    tsdPtr->imageTypeList = copyPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ImageObjCmd --
 *
 *	This function is invoked to process the "image" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ImageObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    static const char *const imageOptions[] = {
	"create", "delete", "height", "inuse", "names", "type", "types",
	"width", NULL
    };
    enum options {
	IMAGE_CREATE, IMAGE_DELETE, IMAGE_HEIGHT, IMAGE_INUSE, IMAGE_NAMES,
	IMAGE_TYPE, IMAGE_TYPES, IMAGE_WIDTH
    };
    TkWindow *winPtr = clientData;
    int i, isNew, firstOption, index;
    Tk_ImageType *typePtr;
    ImageMaster *masterPtr;
    Image *imagePtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    char idString[16 + TCL_INTEGER_SPACE];
    TkDisplay *dispPtr = winPtr->dispPtr;
    const char *arg, *name;
    Tcl_Obj *resultObj;
    ThreadSpecificData *tsdPtr =
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], imageOptions,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum options) index) {
    case IMAGE_CREATE: {
	Tcl_Obj **args;
	int oldimage = 0;

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "type ?name? ?-option value ...?");
	    return TCL_ERROR;
	}

	/*
	 * Look up the image type.
	 */

	arg = Tcl_GetString(objv[2]);
	for (typePtr = tsdPtr->imageTypeList; typePtr != NULL;
		typePtr = typePtr->nextPtr) {
	    if ((*arg == typePtr->name[0])
		    && (strcmp(arg, typePtr->name) == 0)) {
		break;
	    }
	}
	if (typePtr == NULL) {
	    oldimage = 1;
	    for (typePtr = tsdPtr->oldImageTypeList; typePtr != NULL;
		    typePtr = typePtr->nextPtr) {
		if ((*arg == typePtr->name[0])
			&& (strcmp(arg, typePtr->name) == 0)) {
		    break;
		}
	    }
	}
	if (typePtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "image type \"%s\" doesn't exist", arg));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "IMAGE_TYPE", arg, NULL);
	    return TCL_ERROR;
	}

	/*
	 * Figure out a name to use for the new image.
	 */

	if ((objc == 3) || (*(arg = Tcl_GetString(objv[3])) == '-')) {
	    do {
		dispPtr->imageId++;
		sprintf(idString, "image%d", dispPtr->imageId);
		name = idString;
	    } while (Tcl_FindCommand(interp, name, NULL, 0) != NULL);
	    firstOption = 3;
	} else {
	    TkWindow *topWin;

	    name = arg;
	    firstOption = 4;

	    /*
	     * Need to check if the _command_ that we are about to create is
	     * the name of the current master widget command (normally "." but
	     * could have been renamed) and fail in that case before a really
	     * nasty and hard to stop crash happens.
	     */

	    topWin = (TkWindow *) TkToplevelWindowForCommand(interp, name);
	    if (topWin != NULL && winPtr->mainPtr->winPtr == topWin) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"images may not be named the same as the main window",
			-1));
		Tcl_SetErrorCode(interp, "TK", "IMAGE", "SMASH_MAIN", NULL);
		return TCL_ERROR;
	    }
	}

	/*
	 * Create the data structure for the new image.
	 */

	hPtr = Tcl_CreateHashEntry(&winPtr->mainPtr->imageTable, name, &isNew);
	if (isNew) {
	    masterPtr = ckalloc(sizeof(ImageMaster));
	    masterPtr->typePtr = NULL;
	    masterPtr->masterData = NULL;
	    masterPtr->width = masterPtr->height = 1;
	    masterPtr->tablePtr = &winPtr->mainPtr->imageTable;
	    masterPtr->hPtr = hPtr;
	    masterPtr->instancePtr = NULL;
	    masterPtr->deleted = 0;
	    masterPtr->winPtr = winPtr->mainPtr->winPtr;
	    Tcl_Preserve(masterPtr->winPtr);
	    Tcl_SetHashValue(hPtr, masterPtr);
	} else {
	    /*
	     * An image already exists by this name. Disconnect the instances
	     * from the master.
	     */

	    masterPtr = Tcl_GetHashValue(hPtr);
	    if (masterPtr->typePtr != NULL) {
		for (imagePtr = masterPtr->instancePtr; imagePtr != NULL;
			imagePtr = imagePtr->nextPtr) {
		    masterPtr->typePtr->freeProc(imagePtr->instanceData,
			    imagePtr->display);
		    imagePtr->changeProc(imagePtr->widgetClientData, 0, 0,
			    masterPtr->width, masterPtr->height,
			    masterPtr->width, masterPtr->height);
		}
		masterPtr->typePtr->deleteProc(masterPtr->masterData);
		masterPtr->typePtr = NULL;
	    }
	    masterPtr->deleted = 0;
	}

	/*
	 * Call the image type manager so that it can perform its own
	 * initialization, then re-"get" for any existing instances of the
	 * image.
	 */

	objv += firstOption;
	objc -= firstOption;
	args = (Tcl_Obj **) objv;
	if (oldimage) {
	    int i;

	    args = ckalloc((objc+1) * sizeof(char *));
	    for (i = 0; i < objc; i++) {
		args[i] = (Tcl_Obj *) Tcl_GetString(objv[i]);
	    }
	    args[objc] = NULL;
	}
	Tcl_Preserve(masterPtr);
	if (typePtr->createProc(interp, name, objc, args, typePtr,
		(Tk_ImageMaster)masterPtr, &masterPtr->masterData) != TCL_OK){
	    EventuallyDeleteImage(masterPtr, 0);
	    Tcl_Release(masterPtr);
	    if (oldimage) {
		ckfree(args);
	    }
	    return TCL_ERROR;
	}
	Tcl_Release(masterPtr);
	if (oldimage) {
	    ckfree(args);
	}
	masterPtr->typePtr = typePtr;
	for (imagePtr = masterPtr->instancePtr; imagePtr != NULL;
		imagePtr = imagePtr->nextPtr) {
	    imagePtr->instanceData = typePtr->getProc(imagePtr->tkwin,
		    masterPtr->masterData);
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		Tcl_GetHashKey(&winPtr->mainPtr->imageTable, hPtr), -1));
	break;
    }
    case IMAGE_DELETE:
	for (i = 2; i < objc; i++) {
	    arg = Tcl_GetString(objv[i]);
	    hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->imageTable, arg);
	    if (hPtr == NULL) {
		goto alreadyDeleted;
	    }
	    masterPtr = Tcl_GetHashValue(hPtr);
	    if (masterPtr->deleted) {
		goto alreadyDeleted;
	    }
	    DeleteImage(masterPtr);
	}
	break;
    case IMAGE_NAMES:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	hPtr = Tcl_FirstHashEntry(&winPtr->mainPtr->imageTable, &search);
	resultObj = Tcl_NewObj();
	for ( ; hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    masterPtr = Tcl_GetHashValue(hPtr);
	    if (masterPtr->deleted) {
		continue;
	    }
	    Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		    Tcl_GetHashKey(&winPtr->mainPtr->imageTable, hPtr), -1));
	}
	Tcl_SetObjResult(interp, resultObj);
	break;
    case IMAGE_TYPES:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	resultObj = Tcl_NewObj();
	for (typePtr = tsdPtr->imageTypeList; typePtr != NULL;
		typePtr = typePtr->nextPtr) {
	    Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		    typePtr->name, -1));
	}
	for (typePtr = tsdPtr->oldImageTypeList; typePtr != NULL;
		typePtr = typePtr->nextPtr) {
	    Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		    typePtr->name, -1));
	}
	Tcl_SetObjResult(interp, resultObj);
	break;

    case IMAGE_HEIGHT:
    case IMAGE_INUSE:
    case IMAGE_TYPE:
    case IMAGE_WIDTH:
	/*
	 * These operations all parse virtually identically. First check to
	 * see if three args are given. Then get a non-deleted master from the
	 * third arg.
	 */

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "name");
	    return TCL_ERROR;
	}

	arg = Tcl_GetString(objv[2]);
	hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->imageTable, arg);
	if (hPtr == NULL) {
	    goto alreadyDeleted;
	}
	masterPtr = Tcl_GetHashValue(hPtr);
	if (masterPtr->deleted) {
	    goto alreadyDeleted;
	}

	/*
	 * Now we read off the specific piece of data we were asked for.
	 */

	switch ((enum options) index) {
	case IMAGE_HEIGHT:
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(masterPtr->height));
	    break;
	case IMAGE_INUSE:
	    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
		    masterPtr->typePtr && masterPtr->instancePtr));
	    break;
	case IMAGE_TYPE:
	    if (masterPtr->typePtr != NULL) {
		Tcl_SetObjResult(interp,
			Tcl_NewStringObj(masterPtr->typePtr->name, -1));
	    }
	    break;
	case IMAGE_WIDTH:
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(masterPtr->width));
	    break;
	default:
	    Tcl_Panic("can't happen");
	}
	break;
    }
    return TCL_OK;

  alreadyDeleted:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("image \"%s\" doesn't exist",arg));
    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "IMAGE", arg, NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ImageChanged --
 *
 *	This function is called by an image manager whenever something has
 *	happened that requires the image to be redrawn (some of its pixels
 *	have changed, or its size has changed).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any widgets that display the image are notified so that they can
 *	redisplay themselves as appropriate.
 *
 *----------------------------------------------------------------------
 */

void
Tk_ImageChanged(
    Tk_ImageMaster imageMaster,	/* Image that needs redisplay. */
    int x, int y,		/* Coordinates of upper-left pixel of region
				 * of image that needs to be redrawn. */
    int width, int height,	/* Dimensions (in pixels) of region of image
				 * to redraw. If either dimension is zero then
				 * the image doesn't need to be redrawn
				 * (perhaps all that happened is that its size
				 * changed). */
    int imageWidth, int imageHeight)
				/* New dimensions of image. */
{
    ImageMaster *masterPtr = (ImageMaster *) imageMaster;
    Image *imagePtr;

    masterPtr->width = imageWidth;
    masterPtr->height = imageHeight;
    for (imagePtr = masterPtr->instancePtr; imagePtr != NULL;
	    imagePtr = imagePtr->nextPtr) {
	imagePtr->changeProc(imagePtr->widgetClientData, x, y, width, height,
		imageWidth, imageHeight);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_NameOfImage --
 *
 *	Given a token for an image master, this function returns the name of
 *	the image.
 *
 * Results:
 *	The return value is the string name for imageMaster.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tk_NameOfImage(
    Tk_ImageMaster imageMaster)	/* Token for image. */
{
    ImageMaster *masterPtr = (ImageMaster *) imageMaster;

    if (masterPtr->hPtr == NULL) {
	return NULL;
    }
    return Tcl_GetHashKey(masterPtr->tablePtr, masterPtr->hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetImage --
 *
 *	This function is invoked by a widget when it wants to use a particular
 *	image in a particular window.
 *
 * Results:
 *	The return value is a token for the image. If there is no image by the
 *	given name, then NULL is returned and an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	Tk records the fact that the widget is using the image, and it will
 *	invoke changeProc later if the widget needs redisplay (i.e. its size
 *	changes or some of its pixels change). The caller must eventually
 *	invoke Tk_FreeImage when it no longer needs the image.
 *
 *----------------------------------------------------------------------
 */

Tk_Image
Tk_GetImage(
    Tcl_Interp *interp,		/* Place to leave error message if image can't
				 * be found. */
    Tk_Window tkwin,		/* Token for window in which image will be
				 * used. */
    const char *name,		/* Name of desired image. */
    Tk_ImageChangedProc *changeProc,
				/* Function to invoke when redisplay is needed
				 * because image's pixels or size changed. */
    ClientData clientData)	/* One-word argument to pass to damageProc. */
{
    Tcl_HashEntry *hPtr;
    ImageMaster *masterPtr;
    Image *imagePtr;

    hPtr = Tcl_FindHashEntry(&((TkWindow *) tkwin)->mainPtr->imageTable, name);
    if (hPtr == NULL) {
	goto noSuchImage;
    }
    masterPtr = Tcl_GetHashValue(hPtr);
    if (masterPtr->typePtr == NULL) {
	goto noSuchImage;
    }
    if (masterPtr->deleted) {
	goto noSuchImage;
    }
    imagePtr = ckalloc(sizeof(Image));
    imagePtr->tkwin = tkwin;
    imagePtr->display = Tk_Display(tkwin);
    imagePtr->masterPtr = masterPtr;
    imagePtr->instanceData =
	    masterPtr->typePtr->getProc(tkwin, masterPtr->masterData);
    imagePtr->changeProc = changeProc;
    imagePtr->widgetClientData = clientData;
    imagePtr->nextPtr = masterPtr->instancePtr;
    masterPtr->instancePtr = imagePtr;
    return (Tk_Image) imagePtr;

  noSuchImage:
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"image \"%s\" doesn't exist", name));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "IMAGE", name, NULL);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeImage --
 *
 *	This function is invoked by a widget when it no longer needs an image
 *	acquired by a previous call to Tk_GetImage. For each call to
 *	Tk_GetImage there must be exactly one call to Tk_FreeImage.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The association between the image and the widget is removed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeImage(
    Tk_Image image)		/* Token for image that is no longer needed by
				 * a widget. */
{
    Image *imagePtr = (Image *) image;
    ImageMaster *masterPtr = imagePtr->masterPtr;
    Image *prevPtr;

    /*
     * Clean up the particular instance.
     */

    if (masterPtr->typePtr != NULL) {
	masterPtr->typePtr->freeProc(imagePtr->instanceData,
		imagePtr->display);
    }
    prevPtr = masterPtr->instancePtr;
    if (prevPtr == imagePtr) {
	masterPtr->instancePtr = imagePtr->nextPtr;
    } else {
	while (prevPtr->nextPtr != imagePtr) {
	    prevPtr = prevPtr->nextPtr;
	}
	prevPtr->nextPtr = imagePtr->nextPtr;
    }
    ckfree(imagePtr);

    /*
     * If there are no more instances left for the master, and if the master
     * image has been deleted, then delete the master too.
     */

    if ((masterPtr->typePtr == NULL) && (masterPtr->instancePtr == NULL)) {
	if (masterPtr->hPtr != NULL) {
	    Tcl_DeleteHashEntry(masterPtr->hPtr);
	}
	Tcl_Release(masterPtr->winPtr);
	ckfree(masterPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PostscriptImage --
 *
 *	This function is called by widgets that contain images in order to
 *	redisplay an image on the screen or an off-screen pixmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image's manager is notified, and it redraws the desired portion of
 *	the image before returning.
 *
 *----------------------------------------------------------------------
 */

int
Tk_PostscriptImage(
    Tk_Image image,		/* Token for image to redisplay. */
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tk_PostscriptInfo psinfo,	/* postscript info */
    int x, int y,		/* Upper-left pixel of region in image that
				 * needs to be redisplayed. */
    int width, int height,	/* Dimensions of region to redraw. */
    int prepass)
{
    Image *imagePtr = (Image *) image;
    int result;
    XImage *ximage;
    Pixmap pmap;
    GC newGC;
    XGCValues gcValues;

    if (imagePtr->masterPtr->typePtr == NULL) {
	/*
	 * No master for image, so nothing to display on postscript.
	 */

	return TCL_OK;
    }

    /*
     * Check if an image specific postscript-generation function exists;
     * otherwise go on with generic code.
     */

    if (imagePtr->masterPtr->typePtr->postscriptProc != NULL) {
	return imagePtr->masterPtr->typePtr->postscriptProc(
		imagePtr->masterPtr->masterData, interp, tkwin, psinfo,
		x, y, width, height, prepass);
    }

    if (prepass) {
	return TCL_OK;
    }

    /*
     * Create a Pixmap, tell the image to redraw itself there, and then
     * generate an XImage from the Pixmap. We can then read pixel values out
     * of the XImage.
     */

    pmap = Tk_GetPixmap(Tk_Display(tkwin), Tk_WindowId(tkwin), width, height,
	    Tk_Depth(tkwin));

    gcValues.foreground = WhitePixelOfScreen(Tk_Screen(tkwin));
    newGC = Tk_GetGC(tkwin, GCForeground, &gcValues);
    if (newGC != NULL) {
	XFillRectangle(Tk_Display(tkwin), pmap, newGC, 0, 0,
		(unsigned) width, (unsigned) height);
	Tk_FreeGC(Tk_Display(tkwin), newGC);
    }

    Tk_RedrawImage(image, x, y, width, height, pmap, 0, 0);

    ximage = XGetImage(Tk_Display(tkwin), pmap, 0, 0,
	    (unsigned) width, (unsigned) height, AllPlanes, ZPixmap);

    Tk_FreePixmap(Tk_Display(tkwin), pmap);

    if (ximage == NULL) {
	/*
	 * The XGetImage() function is apparently not implemented on this
	 * system. Just ignore it.
	 */

	return TCL_OK;
    }
    result = TkPostscriptImage(interp, tkwin, psinfo, ximage, x, y,
	    width, height);

    XDestroyImage(ximage);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_RedrawImage --
 *
 *	This function is called by widgets that contain images in order to
 *	redisplay an image on the screen or an off-screen pixmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image's manager is notified, and it redraws the desired portion of
 *	the image before returning.
 *
 *----------------------------------------------------------------------
 */

void
Tk_RedrawImage(
    Tk_Image image,		/* Token for image to redisplay. */
    int imageX, int imageY,	/* Upper-left pixel of region in image that
				 * needs to be redisplayed. */
    int width, int height,	/* Dimensions of region to redraw. */
    Drawable drawable,		/* Drawable in which to display image (window
				 * or pixmap). If this is a pixmap, it must
				 * have the same depth as the window used in
				 * the Tk_GetImage call for the image. */
    int drawableX, int drawableY)
				/* Coordinates in drawable that correspond to
				 * imageX and imageY. */
{
    Image *imagePtr = (Image *) image;

    if (imagePtr->masterPtr->typePtr == NULL) {
	/*
	 * No master for image, so nothing to display.
	 */

	return;
    }

    /*
     * Clip the redraw area to the area of the image.
     */

    if (imageX < 0) {
	width += imageX;
	drawableX -= imageX;
	imageX = 0;
    }
    if (imageY < 0) {
	height += imageY;
	drawableY -= imageY;
	imageY = 0;
    }
    if ((imageX + width) > imagePtr->masterPtr->width) {
	width = imagePtr->masterPtr->width - imageX;
    }
    if ((imageY + height) > imagePtr->masterPtr->height) {
	height = imagePtr->masterPtr->height - imageY;
    }
    imagePtr->masterPtr->typePtr->displayProc(imagePtr->instanceData,
	    imagePtr->display, drawable, imageX, imageY, width, height,
	    drawableX, drawableY);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SizeOfImage --
 *
 *	This function returns the current dimensions of an image.
 *
 * Results:
 *	The width and height of the image are returned in *widthPtr and
 *	*heightPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SizeOfImage(
    Tk_Image image,		/* Token for image whose size is wanted. */
    int *widthPtr,		/* Return width of image here. */
    int *heightPtr)		/* Return height of image here. */
{
    Image *imagePtr = (Image *) image;

    *widthPtr = imagePtr->masterPtr->width;
    *heightPtr = imagePtr->masterPtr->height;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DeleteImage --
 *
 *	Given the name of an image, this function destroys the image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image is destroyed; existing instances will display as blank
 *	areas. If no such image exists then the function does nothing.
 *
 *----------------------------------------------------------------------
 */

void
Tk_DeleteImage(
    Tcl_Interp *interp,		/* Interpreter in which the image was
				 * created. */
    const char *name)		/* Name of image. */
{
    Tcl_HashEntry *hPtr;
    TkWindow *winPtr;

    winPtr = (TkWindow *) Tk_MainWindow(interp);
    if (winPtr == NULL) {
	return;
    }
    hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->imageTable, name);
    if (hPtr == NULL) {
	return;
    }
    DeleteImage(Tcl_GetHashValue(hPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteImage --
 *
 *	This function is responsible for deleting an image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The connection is dropped between instances of this image and an image
 *	master. Image instances will redisplay themselves as empty areas, but
 *	existing instances will not be deleted.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteImage(
    ImageMaster *masterPtr)	/* Pointer to main data structure for image. */
{
    Image *imagePtr;
    Tk_ImageType *typePtr;

    typePtr = masterPtr->typePtr;
    masterPtr->typePtr = NULL;
    if (typePtr != NULL) {
	for (imagePtr = masterPtr->instancePtr; imagePtr != NULL;
		imagePtr = imagePtr->nextPtr) {
	    typePtr->freeProc(imagePtr->instanceData, imagePtr->display);
	    imagePtr->changeProc(imagePtr->widgetClientData, 0, 0,
		    masterPtr->width, masterPtr->height, masterPtr->width,
		    masterPtr->height);
	}
	typePtr->deleteProc(masterPtr->masterData);
    }
    if (masterPtr->instancePtr == NULL) {
	if (masterPtr->hPtr != NULL) {
	    Tcl_DeleteHashEntry(masterPtr->hPtr);
	}
	Tcl_Release(masterPtr->winPtr);
	ckfree(masterPtr);
    } else {
	masterPtr->deleted = 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EventuallyDeleteImage --
 *
 *	Arrange for an image to be deleted when it is safe to do so.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Image will get freed, though not until it is no longer Tcl_Preserve()d
 *	by anything. May be called multiple times on the same image without
 *	ill effects.
 *
 *----------------------------------------------------------------------
 */

static void
EventuallyDeleteImage(
    ImageMaster *masterPtr,	/* Pointer to main data structure for image. */
    int forgetImageHashNow)	/* Flag to say whether the hash table is about
				 * to vanish. */
{
    if (forgetImageHashNow) {
	masterPtr->hPtr = NULL;
    }
    if (!masterPtr->deleted) {
	masterPtr->deleted = 1;
	Tcl_EventuallyFree(masterPtr, (Tcl_FreeProc *) DeleteImage);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkDeleteAllImages --
 *
 *	This function is called when an application is deleted. It calls back
 *	all of the managers for all images so that they can cleanup, then it
 *	deletes all of Tk's internal information about images.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All information for all images gets deleted.
 *
 *----------------------------------------------------------------------
 */

void
TkDeleteAllImages(
    TkMainInfo *mainPtr)	/* Structure describing application that is
				 * going away. */
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;

    for (hPtr = Tcl_FirstHashEntry(&mainPtr->imageTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	EventuallyDeleteImage(Tcl_GetHashValue(hPtr), 1);
    }
    Tcl_DeleteHashTable(&mainPtr->imageTable);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetImageMasterData --
 *
 *	Given the name of an image, this function returns the type of the
 *	image and the clientData associated with its master.
 *
 * Results:
 *	If there is no image by the given name, then NULL is returned and a
 *	NULL value is stored at *typePtrPtr. Otherwise the return value is the
 *	clientData returned by the createProc when the image was created and a
 *	pointer to the type structure for the image is stored at *typePtrPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tk_GetImageMasterData(
    Tcl_Interp *interp,		/* Interpreter in which the image was
				 * created. */
    const char *name,		/* Name of image. */
    const Tk_ImageType **typePtrPtr)
				/* Points to location to fill in with pointer
				 * to type information for image. */
{
    TkWindow *winPtr = (TkWindow *) Tk_MainWindow(interp);
    Tcl_HashEntry *hPtr;
    ImageMaster *masterPtr;

    hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->imageTable, name);
    if (hPtr == NULL) {
	*typePtrPtr = NULL;
	return NULL;
    }
    masterPtr = Tcl_GetHashValue(hPtr);
    if (masterPtr->deleted) {
	*typePtrPtr = NULL;
	return NULL;
    }
    *typePtrPtr = masterPtr->typePtr;
    return masterPtr->masterData;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetTSOrigin --
 *
 *	Set the pattern origin of the tile to a common point (i.e. the origin
 *	(0,0) of the top level window) so that tiles from two different
 *	widgets will match up. This done by setting the GCTileStipOrigin field
 *	is set to the translated origin of the toplevel window in the
 *	hierarchy.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The GCTileStipOrigin is reset in the GC. This will cause the tile
 *	origin to change when the GC is used for drawing.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Tk_SetTSOrigin(
    Tk_Window tkwin,
    GC gc,
    int x, int y)
{
    while (!Tk_TopWinHierarchy(tkwin)) {
	x -= Tk_X(tkwin) + Tk_Changes(tkwin)->border_width;
	y -= Tk_Y(tkwin) + Tk_Changes(tkwin)->border_width;
	tkwin = Tk_Parent(tkwin);
    }
    XSetTSOrigin(Tk_Display(tkwin), gc, x, y);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
