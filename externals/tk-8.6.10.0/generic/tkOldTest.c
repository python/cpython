/*
 * tkOldTest.c --
 *
 *	This file contains C command functions for additional Tcl
 *	commands that are used to test Tk's support for legacy
 *	interfaces.  These commands are not normally included in Tcl/Tk
 *	applications; they're only used for testing.
 *
 * Copyright (c) 1993-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Contributions by Don Porter, NIST, 2007.  (not subject to US copyright)
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#define USE_OLD_IMAGE
#ifndef USE_TCL_STUBS
#   define USE_TCL_STUBS
#endif
#ifndef USE_TK_STUBS
#   define USE_TK_STUBS
#endif
#include "tkInt.h"

/*
 * The following data structure represents the master for a test image:
 */

typedef struct TImageMaster {
    Tk_ImageMaster master;      /* Tk's token for image master. */
    Tcl_Interp *interp;         /* Interpreter for application. */
    int width, height;          /* Dimensions of image. */
    char *imageName;            /* Name of image (malloc-ed). */
    char *varName;              /* Name of variable in which to log events for
                                 * image (malloc-ed). */
} TImageMaster;

/*
 * The following data structure represents a particular use of a particular
 * test image.
 */

typedef struct TImageInstance {
    TImageMaster *masterPtr;    /* Pointer to master for image. */
    XColor *fg;                 /* Foreground color for drawing in image. */
    GC gc;                      /* Graphics context for drawing in image. */
} TImageInstance;

/*
 * The type record for test images:
 */

static int		ImageCreate(Tcl_Interp *interp,
			    char *name, int argc, char **argv,
			    Tk_ImageType *typePtr, Tk_ImageMaster master,
			    ClientData *clientDataPtr);
static ClientData	ImageGet(Tk_Window tkwin, ClientData clientData);
static void		ImageDisplay(ClientData clientData,
			    Display *display, Drawable drawable,
			    int imageX, int imageY, int width,
			    int height, int drawableX,
			    int drawableY);
static void		ImageFree(ClientData clientData, Display *display);
static void		ImageDelete(ClientData clientData);

static Tk_ImageType imageType = {
    "oldtest",			/* name */
    (Tk_ImageCreateProc *) ImageCreate, /* createProc */
    ImageGet,			/* getProc */
    ImageDisplay,		/* displayProc */
    ImageFree,			/* freeProc */
    ImageDelete,		/* deleteProc */
    NULL,			/* postscriptPtr */
    NULL,			/* nextPtr */
    NULL
};

/*
 * Forward declarations for functions defined later in this file:
 */

static int              ImageObjCmd(ClientData dummy,
                            Tcl_Interp *interp, int objc,
            			    Tcl_Obj * const objv[]);


/*
 *----------------------------------------------------------------------
 *
 * TkOldTestInit --
 *
 *	This function performs initialization for the Tk test suite
 *	extensions for testing support for legacy interfaces.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error message in
 *	the interp's result if an error occurs.
 *
 * Side effects:
 *	Creates several test commands.
 *
 *----------------------------------------------------------------------
 */

int
TkOldTestInit(
    Tcl_Interp *interp)
{
    static int initialized = 0;

    if (!initialized) {
	initialized = 1;
	Tk_CreateImageType(&imageType);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageCreate --
 *
 *	This function is called by the Tk image code to create "oldtest" images.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	The data structure for a new image is allocated.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ImageCreate(
    Tcl_Interp *interp,		/* Interpreter for application containing
				 * image. */
    char *name,			/* Name to use for image. */
    int argc,			/* Number of arguments. */
    char **argv,		/* Argument strings for options (doesn't
				 * include image name or type). */
    Tk_ImageType *typePtr,	/* Pointer to our type record (not used). */
    Tk_ImageMaster master,	/* Token for image, to be used by us in later
				 * callbacks. */
    ClientData *clientDataPtr)	/* Store manager's token for image here; it
				 * will be returned in later callbacks. */
{
    TImageMaster *timPtr;
    const char *varName;
    int i;

    varName = "log";
    for (i = 0; i < argc; i += 2) {
	if (strcmp(argv[i], "-variable") != 0) {
	    Tcl_AppendResult(interp, "bad option name \"",
		    argv[i], "\"", NULL);
	    return TCL_ERROR;
	}
	if ((i+1) == argc) {
	    Tcl_AppendResult(interp, "no value given for \"",
		    argv[i], "\" option", NULL);
	    return TCL_ERROR;
	}
	varName = argv[i+1];
    }

    timPtr = ckalloc(sizeof(TImageMaster));
    timPtr->master = master;
    timPtr->interp = interp;
    timPtr->width = 30;
    timPtr->height = 15;
    timPtr->imageName = ckalloc(strlen(name) + 1);
    strcpy(timPtr->imageName, name);
    timPtr->varName = ckalloc(strlen(varName) + 1);
    strcpy(timPtr->varName, varName);
    Tcl_CreateObjCommand(interp, name, ImageObjCmd, timPtr, NULL);
    *clientDataPtr = timPtr;
    Tk_ImageChanged(master, 0, 0, 30, 15, 30, 15);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageObjCmd --
 *
 *	This function implements the commands corresponding to individual
 *	images.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Forces windows to be created.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ImageObjCmd(
    ClientData clientData,	/* Main window for application. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument strings. */
{
    TImageMaster *timPtr = clientData;
    int x, y, width, height;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    if (strcmp(Tcl_GetString(objv[1]), "changed") == 0) {
	if (objc != 8) {
	    Tcl_WrongNumArgs(interp, 1, objv, "changed x y width height"
		    " imageWidth imageHeight");
	    return TCL_ERROR;
	}
	if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &width) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[5], &height) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[6], &timPtr->width) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[7], &timPtr->height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	Tk_ImageChanged(timPtr->master, x, y, width, height, timPtr->width,
		timPtr->height);
    } else {
	Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[1]),
		"\": must be changed", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageGet --
 *
 *	This function is called by Tk to set things up for using a test image
 *	in a particular widget.
 *
 * Results:
 *	The return value is a token for the image instance, which is used in
 *	future callbacks to ImageDisplay and ImageFree.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ClientData
ImageGet(
    Tk_Window tkwin,		/* Token for window in which image will be
				 * used. */
    ClientData clientData)	/* Pointer to TImageMaster for image. */
{
    TImageMaster *timPtr = clientData;
    TImageInstance *instPtr;
    char buffer[100];
    XGCValues gcValues;

    sprintf(buffer, "%s get", timPtr->imageName);
    Tcl_SetVar2(timPtr->interp, timPtr->varName, NULL, buffer,
	    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);

    instPtr = ckalloc(sizeof(TImageInstance));
    instPtr->masterPtr = timPtr;
    instPtr->fg = Tk_GetColor(timPtr->interp, tkwin, "#ff0000");
    gcValues.foreground = instPtr->fg->pixel;
    instPtr->gc = Tk_GetGC(tkwin, GCForeground, &gcValues);
    return instPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageDisplay --
 *
 *	This function is invoked to redisplay part or all of an image in a
 *	given drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image gets partially redrawn, as an "X" that shows the exact
 *	redraw area.
 *
 *----------------------------------------------------------------------
 */

static void
ImageDisplay(
    ClientData clientData,	/* Pointer to TImageInstance for image. */
    Display *display,		/* Display to use for drawing. */
    Drawable drawable,		/* Where to redraw image. */
    int imageX, int imageY,	/* Origin of area to redraw, relative to
				 * origin of image. */
    int width, int height,	/* Dimensions of area to redraw. */
    int drawableX, int drawableY)
				/* Coordinates in drawable corresponding to
				 * imageX and imageY. */
{
    TImageInstance *instPtr = clientData;
    char buffer[200 + TCL_INTEGER_SPACE * 6];

    sprintf(buffer, "%s display %d %d %d %d %d %d",
	    instPtr->masterPtr->imageName, imageX, imageY, width, height,
	    drawableX, drawableY);
    Tcl_SetVar2(instPtr->masterPtr->interp, instPtr->masterPtr->varName, NULL,
	    buffer, TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
    if (width > (instPtr->masterPtr->width - imageX)) {
	width = instPtr->masterPtr->width - imageX;
    }
    if (height > (instPtr->masterPtr->height - imageY)) {
	height = instPtr->masterPtr->height - imageY;
    }
    XDrawRectangle(display, drawable, instPtr->gc, drawableX, drawableY,
	    (unsigned) (width-1), (unsigned) (height-1));
    XDrawLine(display, drawable, instPtr->gc, drawableX, drawableY,
	    (int) (drawableX + width - 1), (int) (drawableY + height - 1));
    XDrawLine(display, drawable, instPtr->gc, drawableX,
	    (int) (drawableY + height - 1),
	    (int) (drawableX + width - 1), drawableY);
}

/*
 *----------------------------------------------------------------------
 *
 * ImageFree --
 *
 *	This function is called when an instance of an image is no longer
 *	used.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information related to the instance is freed.
 *
 *----------------------------------------------------------------------
 */

static void
ImageFree(
    ClientData clientData,	/* Pointer to TImageInstance for instance. */
    Display *display)		/* Display where image was to be drawn. */
{
    TImageInstance *instPtr = clientData;
    char buffer[200];

    sprintf(buffer, "%s free", instPtr->masterPtr->imageName);
    Tcl_SetVar2(instPtr->masterPtr->interp, instPtr->masterPtr->varName, NULL,
	    buffer, TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
    Tk_FreeColor(instPtr->fg);
    Tk_FreeGC(display, instPtr->gc);
    ckfree(instPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImageDelete --
 *
 *	This function is called to clean up a test image when an application
 *	goes away.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information about the image is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
ImageDelete(
    ClientData clientData)	/* Pointer to TImageMaster for image. When
				 * this function is called, no more instances
				 * exist. */
{
    TImageMaster *timPtr = clientData;
    char buffer[100];

    sprintf(buffer, "%s delete", timPtr->imageName);
    Tcl_SetVar2(timPtr->interp, timPtr->varName, NULL, buffer,
	    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);

    Tcl_DeleteCommand(timPtr->interp, timPtr->imageName);
    ckfree(timPtr->imageName);
    ckfree(timPtr->varName);
    ckfree(timPtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
