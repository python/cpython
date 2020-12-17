/*
 * tkImgBmap.c --
 *
 *	This procedure implements images of type "bitmap" for Tk.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * The following data structure represents the master for a bitmap
 * image:
 */

typedef struct BitmapMaster {
    Tk_ImageMaster tkMaster;	/* Tk's token for image master. NULL means the
				 * image is being deleted. */
    Tcl_Interp *interp;		/* Interpreter for application that is using
				 * image. */
    Tcl_Command imageCmd;	/* Token for image command (used to delete it
				 * when the image goes away). NULL means the
				 * image command has already been deleted. */
    int width, height;		/* Dimensions of image. */
    char *data;			/* Data comprising bitmap (suitable for input
				 * to XCreateBitmapFromData). May be NULL if
				 * no data. Malloc'ed. */
    char *maskData;		/* Data for bitmap's mask (suitable for input
				 * to XCreateBitmapFromData). Malloc'ed. */
    Tk_Uid fgUid;		/* Value of -foreground option (malloc'ed). */
    Tk_Uid bgUid;		/* Value of -background option (malloc'ed). */
    char *fileString;		/* Value of -file option (malloc'ed). */
    char *dataString;		/* Value of -data option (malloc'ed). */
    char *maskFileString;	/* Value of -maskfile option (malloc'ed). */
    char *maskDataString;	/* Value of -maskdata option (malloc'ed). */
    struct BitmapInstance *instancePtr;
				/* First in list of all instances associated
				 * with this master. */
} BitmapMaster;

/*
 * The following data structure represents all of the instances of an image
 * that lie within a particular window:
 */

typedef struct BitmapInstance {
    int refCount;		/* Number of instances that share this data
				 * structure. */
    BitmapMaster *masterPtr;	/* Pointer to master for image. */
    Tk_Window tkwin;		/* Window in which the instances will be
				 * displayed. */
    XColor *fg;			/* Foreground color for displaying image. */
    XColor *bg;			/* Background color for displaying image. */
    Pixmap bitmap;		/* The bitmap to display. */
    Pixmap mask;		/* Mask: only display bitmap pixels where
				 * there are 1's here. */
    GC gc;			/* Graphics context for displaying bitmap.
				 * None means there was an error while setting
				 * up the instance, so it cannot be
				 * displayed. */
    struct BitmapInstance *nextPtr;
				/* Next in list of all instance structures
				 * associated with masterPtr (NULL means end
				 * of list). */
} BitmapInstance;

/*
 * The type record for bitmap images:
 */

static int		GetByte(Tcl_Channel chan);
static int		ImgBmapCreate(Tcl_Interp *interp,
			    const char *name, int argc, Tcl_Obj *const objv[],
			    const Tk_ImageType *typePtr, Tk_ImageMaster master,
			    ClientData *clientDataPtr);
static ClientData	ImgBmapGet(Tk_Window tkwin, ClientData clientData);
static void		ImgBmapDisplay(ClientData clientData,
			    Display *display, Drawable drawable,
			    int imageX, int imageY, int width, int height,
			    int drawableX, int drawableY);
static void		ImgBmapFree(ClientData clientData, Display *display);
static void		ImgBmapDelete(ClientData clientData);
static int		ImgBmapPostscript(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin,
			    Tk_PostscriptInfo psinfo, int x, int y,
			    int width, int height, int prepass);

Tk_ImageType tkBitmapImageType = {
    "bitmap",			/* name */
    ImgBmapCreate,		/* createProc */
    ImgBmapGet,			/* getProc */
    ImgBmapDisplay,		/* displayProc */
    ImgBmapFree,		/* freeProc */
    ImgBmapDelete,		/* deleteProc */
    ImgBmapPostscript,		/* postscriptProc */
    NULL,			/* nextPtr */
    NULL
};

/*
 * Information used for parsing configuration specs:
 */

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_UID, "-background", NULL, NULL,
	"", Tk_Offset(BitmapMaster, bgUid), 0, NULL},
    {TK_CONFIG_STRING, "-data", NULL, NULL,
	NULL, Tk_Offset(BitmapMaster, dataString), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_STRING, "-file", NULL, NULL,
	NULL, Tk_Offset(BitmapMaster, fileString), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_UID, "-foreground", NULL, NULL,
	"#000000", Tk_Offset(BitmapMaster, fgUid), 0, NULL},
    {TK_CONFIG_STRING, "-maskdata", NULL, NULL,
	NULL, Tk_Offset(BitmapMaster, maskDataString), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_STRING, "-maskfile", NULL, NULL,
	NULL, Tk_Offset(BitmapMaster, maskFileString), TK_CONFIG_NULL_OK, NULL},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};

/*
 * The following data structure is used to describe the state of parsing a
 * bitmap file or string. It is used for communication between TkGetBitmapData
 * and NextBitmapWord.
 */

#define MAX_WORD_LENGTH 100
typedef struct ParseInfo {
    const char *string; /* Next character of string data for bitmap,
				 * or NULL if bitmap is being read from
				 * file. */
    Tcl_Channel chan;		/* File containing bitmap data, or NULL if no
				 * file. */
    char word[MAX_WORD_LENGTH+1];
				/* Current word of bitmap data, NULL
				 * terminated. */
    int wordLength;		/* Number of non-NULL bytes in word. */
} ParseInfo;

/*
 * Prototypes for procedures used only locally in this file:
 */

static int		ImgBmapCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, Tcl_Obj *const objv[]);
static void		ImgBmapCmdDeletedProc(ClientData clientData);
static void		ImgBmapConfigureInstance(BitmapInstance *instancePtr);
static int		ImgBmapConfigureMaster(BitmapMaster *masterPtr,
			    int argc, Tcl_Obj *const objv[], int flags);
static int		NextBitmapWord(ParseInfo *parseInfoPtr);

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapCreate --
 *
 *	This procedure is called by the Tk image code to create "test" images.
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
ImgBmapCreate(
    Tcl_Interp *interp,		/* Interpreter for application containing
				 * image. */
    const char *name,			/* Name to use for image. */
    int argc,			/* Number of arguments. */
    Tcl_Obj *const argv[],	/* Argument objects for options (doesn't
				 * include image name or type). */
    const Tk_ImageType *typePtr,/* Pointer to our type record (not used). */
    Tk_ImageMaster master,	/* Token for image, to be used by us in later
				 * callbacks. */
    ClientData *clientDataPtr)	/* Store manager's token for image here; it
				 * will be returned in later callbacks. */
{
    BitmapMaster *masterPtr = ckalloc(sizeof(BitmapMaster));

    masterPtr->tkMaster = master;
    masterPtr->interp = interp;
    masterPtr->imageCmd = Tcl_CreateObjCommand(interp, name, ImgBmapCmd,
	    masterPtr, ImgBmapCmdDeletedProc);
    masterPtr->width = masterPtr->height = 0;
    masterPtr->data = NULL;
    masterPtr->maskData = NULL;
    masterPtr->fgUid = NULL;
    masterPtr->bgUid = NULL;
    masterPtr->fileString = NULL;
    masterPtr->dataString = NULL;
    masterPtr->maskFileString = NULL;
    masterPtr->maskDataString = NULL;
    masterPtr->instancePtr = NULL;
    if (ImgBmapConfigureMaster(masterPtr, argc, argv, 0) != TCL_OK) {
	ImgBmapDelete(masterPtr);
	return TCL_ERROR;
    }
    *clientDataPtr = masterPtr;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapConfigureMaster --
 *
 *	This procedure is called when a bitmap image is created or
 *	reconfigured. It process configuration options and resets any
 *	instances of the image.
 *
 * Results:
 *	A standard Tcl return value. If TCL_ERROR is returned then an error
 *	message is left in the masterPtr->interp's result.
 *
 * Side effects:
 *	Existing instances of the image will be redisplayed to match the new
 *	configuration options.
 *
 *----------------------------------------------------------------------
 */

static int
ImgBmapConfigureMaster(
    BitmapMaster *masterPtr,	/* Pointer to data structure describing
				 * overall bitmap image to (reconfigure). */
    int objc,			/* Number of entries in objv. */
    Tcl_Obj *const objv[],	/* Pairs of configuration options for image. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget, such
				 * as TK_CONFIG_ARGV_ONLY. */
{
    BitmapInstance *instancePtr;
    int maskWidth, maskHeight, dummy1, dummy2;
    const char **argv = ckalloc((objc+1) * sizeof(char *));

    for (dummy1 = 0; dummy1 < objc; dummy1++) {
	argv[dummy1] = Tcl_GetString(objv[dummy1]);
    }
    argv[objc] = NULL;

    if (Tk_ConfigureWidget(masterPtr->interp, Tk_MainWindow(masterPtr->interp),
	    configSpecs, objc, argv, (char *) masterPtr, flags) != TCL_OK) {
	ckfree(argv);
	return TCL_ERROR;
    }
    ckfree(argv);

    /*
     * Parse the bitmap and/or mask to create binary data. Make sure that the
     * bitmap and mask have the same dimensions.
     */

    if (masterPtr->data != NULL) {
	ckfree(masterPtr->data);
	masterPtr->data = NULL;
    }
    if ((masterPtr->fileString != NULL) || (masterPtr->dataString != NULL)) {
	masterPtr->data = TkGetBitmapData(masterPtr->interp,
		masterPtr->dataString, masterPtr->fileString,
		&masterPtr->width, &masterPtr->height, &dummy1, &dummy2);
	if (masterPtr->data == NULL) {
	    return TCL_ERROR;
	}
    }
    if (masterPtr->maskData != NULL) {
	ckfree(masterPtr->maskData);
	masterPtr->maskData = NULL;
    }
    if ((masterPtr->maskFileString != NULL)
	    || (masterPtr->maskDataString != NULL)) {
	if (masterPtr->data == NULL) {
	    Tcl_SetObjResult(masterPtr->interp, Tcl_NewStringObj(
		    "can't have mask without bitmap", -1));
	    Tcl_SetErrorCode(masterPtr->interp, "TK", "IMAGE", "BITMAP",
		    "NO_BITMAP", NULL);
	    return TCL_ERROR;
	}
	masterPtr->maskData = TkGetBitmapData(masterPtr->interp,
		masterPtr->maskDataString, masterPtr->maskFileString,
		&maskWidth, &maskHeight, &dummy1, &dummy2);
	if (masterPtr->maskData == NULL) {
	    return TCL_ERROR;
	}
	if ((maskWidth != masterPtr->width)
		|| (maskHeight != masterPtr->height)) {
	    ckfree(masterPtr->maskData);
	    masterPtr->maskData = NULL;
	    Tcl_SetObjResult(masterPtr->interp, Tcl_NewStringObj(
		    "bitmap and mask have different sizes", -1));
	    Tcl_SetErrorCode(masterPtr->interp, "TK", "IMAGE", "BITMAP",
		    "MASK_SIZE", NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Cycle through all of the instances of this image, regenerating the
     * information for each instance. Then force the image to be redisplayed
     * everywhere that it is used.
     */

    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	ImgBmapConfigureInstance(instancePtr);
    }
    Tk_ImageChanged(masterPtr->tkMaster, 0, 0, masterPtr->width,
	    masterPtr->height, masterPtr->width, masterPtr->height);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapConfigureInstance --
 *
 *	This procedure is called to create displaying information for a bitmap
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

static void
ImgBmapConfigureInstance(
    BitmapInstance *instancePtr)/* Instance to reconfigure. */
{
    BitmapMaster *masterPtr = instancePtr->masterPtr;
    XColor *colorPtr;
    XGCValues gcValues;
    GC gc;
    unsigned int mask;
    Pixmap oldBitmap, oldMask;

    /*
     * For each of the options in masterPtr, translate the string form into an
     * internal form appropriate for instancePtr.
     */

    if (*masterPtr->bgUid != 0) {
	colorPtr = Tk_GetColor(masterPtr->interp, instancePtr->tkwin,
		masterPtr->bgUid);
	if (colorPtr == NULL) {
	    goto error;
	}
    } else {
	colorPtr = NULL;
    }
    if (instancePtr->bg != NULL) {
	Tk_FreeColor(instancePtr->bg);
    }
    instancePtr->bg = colorPtr;

    colorPtr = Tk_GetColor(masterPtr->interp, instancePtr->tkwin,
	    masterPtr->fgUid);
    if (colorPtr == NULL) {
	goto error;
    }
    if (instancePtr->fg != NULL) {
	Tk_FreeColor(instancePtr->fg);
    }
    instancePtr->fg = colorPtr;

    /*
     * Careful: We have to allocate new Pixmaps before deleting the old ones.
     * Otherwise, The XID allocator will always return the same XID for the
     * new Pixmaps as was used for the old Pixmaps. And that will prevent the
     * data and/or mask from changing in the GC below.
     */

    oldBitmap = instancePtr->bitmap;
    instancePtr->bitmap = None;
    oldMask = instancePtr->mask;
    instancePtr->mask = None;

    if (masterPtr->data != NULL) {
	instancePtr->bitmap = XCreateBitmapFromData(
		Tk_Display(instancePtr->tkwin),
		RootWindowOfScreen(Tk_Screen(instancePtr->tkwin)),
		masterPtr->data, (unsigned) masterPtr->width,
		(unsigned) masterPtr->height);
    }
    if (masterPtr->maskData != NULL) {
	instancePtr->mask = XCreateBitmapFromData(
		Tk_Display(instancePtr->tkwin),
		RootWindowOfScreen(Tk_Screen(instancePtr->tkwin)),
		masterPtr->maskData, (unsigned) masterPtr->width,
		(unsigned) masterPtr->height);
    }

    if (oldMask != None) {
	Tk_FreePixmap(Tk_Display(instancePtr->tkwin), oldMask);
    }
    if (oldBitmap != None) {
	Tk_FreePixmap(Tk_Display(instancePtr->tkwin), oldBitmap);
    }

    if (masterPtr->data != NULL) {
	gcValues.foreground = instancePtr->fg->pixel;
	gcValues.graphics_exposures = False;
	mask = GCForeground|GCGraphicsExposures;
	if (instancePtr->bg != NULL) {
	    gcValues.background = instancePtr->bg->pixel;
	    mask |= GCBackground;
	    if (instancePtr->mask != None) {
		gcValues.clip_mask = instancePtr->mask;
		mask |= GCClipMask;
	    }
	} else {
	    gcValues.clip_mask = instancePtr->bitmap;
	    mask |= GCClipMask;
	}
	gc = Tk_GetGC(instancePtr->tkwin, mask, &gcValues);
    } else {
	gc = None;
    }
    if (instancePtr->gc != None) {
	Tk_FreeGC(Tk_Display(instancePtr->tkwin), instancePtr->gc);
    }
    instancePtr->gc = gc;
    return;

  error:
    /*
     * An error occurred: clear the graphics context in the instance to make
     * it clear that this instance cannot be displayed. Then report the error.
     */

    if (instancePtr->gc != None) {
	Tk_FreeGC(Tk_Display(instancePtr->tkwin), instancePtr->gc);
    }
    instancePtr->gc = None;
    Tcl_AppendObjToErrorInfo(masterPtr->interp, Tcl_ObjPrintf(
	    "\n    (while configuring image \"%s\")", Tk_NameOfImage(
	    masterPtr->tkMaster)));
    Tcl_BackgroundException(masterPtr->interp, TCL_ERROR);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetBitmapData --
 *
 *	Given a file name or ASCII string, this procedure parses the file or
 *	string contents to produce binary data for a bitmap.
 *
 * Results:
 *	If the bitmap description was parsed successfully then the return
 *	value is a malloc-ed array containing the bitmap data. The dimensions
 *	of the data are stored in *widthPtr and *heightPtr. *hotXPtr and
 *	*hotYPtr are set to the bitmap hotspot if one is defined, otherwise
 *	they are set to -1, -1. If an error occurred, NULL is returned and an
 *	error message is left in the interp's result.
 *
 * Side effects:
 *	A bitmap is created.
 *
 *----------------------------------------------------------------------
 */

char *
TkGetBitmapData(
    Tcl_Interp *interp,		/* For reporting errors, or NULL. */
    const char *string,		/* String describing bitmap. May be NULL. */
    const char *fileName,	/* Name of file containing bitmap description.
				 * Used only if string is NULL. Must not be
				 * NULL if string is NULL. */
    int *widthPtr, int *heightPtr,
				/* Dimensions of bitmap get returned here. */
    int *hotXPtr, int *hotYPtr)	/* Position of hot spot or -1,-1. */
{
    int width, height, numBytes, hotX, hotY;
    const char *expandedFileName;
    char *p, *end;
    ParseInfo pi;
    char *data = NULL;
    Tcl_DString buffer;

    pi.string = string;
    if (string == NULL) {
	if ((interp != NULL) && Tcl_IsSafe(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't get bitmap data from a file in a safe interpreter",
		    -1));
	    Tcl_SetErrorCode(interp, "TK", "SAFE", "BITMAP_FILE", NULL);
	    return NULL;
	}
	expandedFileName = Tcl_TranslateFileName(interp, fileName, &buffer);
	if (expandedFileName == NULL) {
	    return NULL;
	}
	pi.chan = Tcl_OpenFileChannel(interp, expandedFileName, "r", 0);
	Tcl_DStringFree(&buffer);
	if (pi.chan == NULL) {
	    if (interp != NULL) {
		Tcl_ResetResult(interp);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't read bitmap file \"%s\": %s",
			fileName, Tcl_PosixError(interp)));
	    }
	    return NULL;
	}

	if (Tcl_SetChannelOption(interp, pi.chan, "-translation", "binary")
		!= TCL_OK) {
	    return NULL;
	}
	if (Tcl_SetChannelOption(interp, pi.chan, "-encoding", "binary")
		!= TCL_OK) {
	    return NULL;
	}
    } else {
	pi.chan = NULL;
    }

    /*
     * Parse the lines that define the dimensions of the bitmap, plus the
     * first line that defines the bitmap data (it declares the name of a data
     * variable but doesn't include any actual data). These lines look
     * something like the following:
     *
     *		#define foo_width 16
     *		#define foo_height 16
     *		#define foo_x_hot 3
     *		#define foo_y_hot 3
     *		static char foo_bits[] = {
     *
     * The x_hot and y_hot lines may or may not be present. It's important to
     * check for "char" in the last line, in order to reject old X10-style
     * bitmaps that used shorts.
     */

    width = 0;
    height = 0;
    hotX = -1;
    hotY = -1;
    while (1) {
	if (NextBitmapWord(&pi) != TCL_OK) {
	    goto error;
	}
	if ((pi.wordLength >= 6) && (pi.word[pi.wordLength-6] == '_')
		&& (strcmp(pi.word+pi.wordLength-6, "_width") == 0)) {
	    if (NextBitmapWord(&pi) != TCL_OK) {
		goto error;
	    }
	    width = strtol(pi.word, &end, 0);
	    if ((end == pi.word) || (*end != 0)) {
		goto error;
	    }
	} else if ((pi.wordLength >= 7) && (pi.word[pi.wordLength-7] == '_')
		&& (strcmp(pi.word+pi.wordLength-7, "_height") == 0)) {
	    if (NextBitmapWord(&pi) != TCL_OK) {
		goto error;
	    }
	    height = strtol(pi.word, &end, 0);
	    if ((end == pi.word) || (*end != 0)) {
		goto error;
	    }
	} else if ((pi.wordLength >= 6) && (pi.word[pi.wordLength-6] == '_')
		&& (strcmp(pi.word+pi.wordLength-6, "_x_hot") == 0)) {
	    if (NextBitmapWord(&pi) != TCL_OK) {
		goto error;
	    }
	    hotX = strtol(pi.word, &end, 0);
	    if ((end == pi.word) || (*end != 0)) {
		goto error;
	    }
	} else if ((pi.wordLength >= 6) && (pi.word[pi.wordLength-6] == '_')
		&& (strcmp(pi.word+pi.wordLength-6, "_y_hot") == 0)) {
	    if (NextBitmapWord(&pi) != TCL_OK) {
		goto error;
	    }
	    hotY = strtol(pi.word, &end, 0);
	    if ((end == pi.word) || (*end != 0)) {
		goto error;
	    }
	} else if ((pi.word[0] == 'c') && (strcmp(pi.word, "char") == 0)) {
	    while (1) {
		if (NextBitmapWord(&pi) != TCL_OK) {
		    goto error;
		}
		if ((pi.word[0] == '{') && (pi.word[1] == 0)) {
		    goto getData;
		}
	    }
	} else if ((pi.word[0] == '{') && (pi.word[1] == 0)) {
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"format error in bitmap data; looks like it's an"
			" obsolete X10 bitmap file", -1));
		Tcl_SetErrorCode(interp, "TK", "IMAGE", "BITMAP", "OBSOLETE",
			NULL);
	    }
	    goto errorCleanup;
	}
    }

    /*
     * Now we've read everything but the data. Allocate an array and read in
     * the data.
     */

  getData:
    if ((width <= 0) || (height <= 0)) {
	goto error;
    }
    numBytes = ((width+7)/8) * height;
    data = ckalloc(numBytes);
    for (p = data; numBytes > 0; p++, numBytes--) {
	if (NextBitmapWord(&pi) != TCL_OK) {
	    goto error;
	}
	*p = (char) strtol(pi.word, &end, 0);
	if (end == pi.word) {
	    goto error;
	}
    }

    /*
     * All done. Clean up and return.
     */

    if (pi.chan != NULL) {
	Tcl_Close(NULL, pi.chan);
    }
    *widthPtr = width;
    *heightPtr = height;
    *hotXPtr = hotX;
    *hotYPtr = hotY;
    return data;

  error:
    if (interp != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"format error in bitmap data", -1));
	Tcl_SetErrorCode(interp, "TK", "IMAGE", "BITMAP", "FORMAT", NULL);
    }

  errorCleanup:
    if (data != NULL) {
	ckfree(data);
    }
    if (pi.chan != NULL) {
	Tcl_Close(NULL, pi.chan);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * NextBitmapWord --
 *
 *	This procedure retrieves the next word of information (stuff between
 *	commas or white space) from a bitmap description.
 *
 * Results:
 *	Returns TCL_OK if all went well. In this case the next word, and its
 *	length, will be availble in *parseInfoPtr. If the end of the bitmap
 *	description was reached then TCL_ERROR is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
NextBitmapWord(
    ParseInfo *parseInfoPtr)	/* Describes what we're reading and where we
				 * are in it. */
{
    const char *src;
    char *dst;
    int c;

    parseInfoPtr->wordLength = 0;
    dst = parseInfoPtr->word;
    if (parseInfoPtr->string != NULL) {
	for (src = parseInfoPtr->string; isspace(UCHAR(*src)) || (*src == ',');
		src++) {
	    if (*src == 0) {
		return TCL_ERROR;
	    }
	}
	for ( ; !isspace(UCHAR(*src)) && (*src != ',') && (*src != 0); src++) {
	    *dst = *src;
	    dst++;
	    parseInfoPtr->wordLength++;
	    if (parseInfoPtr->wordLength > MAX_WORD_LENGTH) {
		return TCL_ERROR;
	    }
	}
	parseInfoPtr->string = src;
    } else {
	for (c = GetByte(parseInfoPtr->chan); isspace(UCHAR(c)) || (c == ',');
		c = GetByte(parseInfoPtr->chan)) {
	    if (c == EOF) {
		return TCL_ERROR;
	    }
	}
	for ( ; !isspace(UCHAR(c)) && (c != ',') && (c != EOF);
		c = GetByte(parseInfoPtr->chan)) {
	    *dst = c;
	    dst++;
	    parseInfoPtr->wordLength++;
	    if (parseInfoPtr->wordLength > MAX_WORD_LENGTH) {
		return TCL_ERROR;
	    }
	}
    }
    if (parseInfoPtr->wordLength == 0) {
	return TCL_ERROR;
    }
    parseInfoPtr->word[parseInfoPtr->wordLength] = 0;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ImgBmapCmd --
 *
 *	This procedure is invoked to process the Tcl command that corresponds
 *	to an image managed by this module. See the user documentation for
 *	details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
ImgBmapCmd(
    ClientData clientData,	/* Information about the image master. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const bmapOptions[] = {"cget", "configure", NULL};
    BitmapMaster *masterPtr = clientData;
    int index;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], bmapOptions,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch (index) {
    case 0: /* cget */
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    return TCL_ERROR;
	}
	return Tk_ConfigureValue(interp, Tk_MainWindow(interp), configSpecs,
		(char *) masterPtr, Tcl_GetString(objv[2]), 0);
    case 1: /* configure */
	if (objc == 2) {
	    return Tk_ConfigureInfo(interp, Tk_MainWindow(interp),
		    configSpecs, (char *) masterPtr, NULL, 0);
	} else if (objc == 3) {
	    return Tk_ConfigureInfo(interp, Tk_MainWindow(interp),
		    configSpecs, (char *) masterPtr,
		    Tcl_GetString(objv[2]), 0);
	} else {
	    return ImgBmapConfigureMaster(masterPtr, objc-2, objv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    default:
	Tcl_Panic("bad const entries to bmapOptions in ImgBmapCmd");
	return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapGet --
 *
 *	This procedure is called for each use of a bitmap image in a widget.
 *
 * Results:
 *	The return value is a token for the instance, which is passed back to
 *	us in calls to ImgBmapDisplay and ImgBmapFree.
 *
 * Side effects:
 *	A data structure is set up for the instance (or, an existing instance
 *	is re-used for the new one).
 *
 *----------------------------------------------------------------------
 */

static ClientData
ImgBmapGet(
    Tk_Window tkwin,		/* Window in which the instance will be
				 * used. */
    ClientData masterData)	/* Pointer to our master structure for the
				 * image. */
{
    BitmapMaster *masterPtr = masterData;
    BitmapInstance *instancePtr;

    /*
     * See if there is already an instance for this window. If so then just
     * re-use it.
     */

    for (instancePtr = masterPtr->instancePtr; instancePtr != NULL;
	    instancePtr = instancePtr->nextPtr) {
	if (instancePtr->tkwin == tkwin) {
	    instancePtr->refCount++;
	    return instancePtr;
	}
    }

    /*
     * The image isn't already in use in this window. Make a new instance of
     * the image.
     */

    instancePtr = ckalloc(sizeof(BitmapInstance));
    instancePtr->refCount = 1;
    instancePtr->masterPtr = masterPtr;
    instancePtr->tkwin = tkwin;
    instancePtr->fg = NULL;
    instancePtr->bg = NULL;
    instancePtr->bitmap = None;
    instancePtr->mask = None;
    instancePtr->gc = None;
    instancePtr->nextPtr = masterPtr->instancePtr;
    masterPtr->instancePtr = instancePtr;
    ImgBmapConfigureInstance(instancePtr);

    /*
     * If this is the first instance, must set the size of the image.
     */

    if (instancePtr->nextPtr == NULL) {
	Tk_ImageChanged(masterPtr->tkMaster, 0, 0, 0, 0, masterPtr->width,
		masterPtr->height);
    }

    return instancePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapDisplay --
 *
 *	This procedure is invoked to draw a bitmap image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A portion of the image gets rendered in a pixmap or window.
 *
 *----------------------------------------------------------------------
 */

static void
ImgBmapDisplay(
    ClientData clientData,	/* Pointer to BitmapInstance structure for
				 * instance to be displayed. */
    Display *display,		/* Display on which to draw image. */
    Drawable drawable,		/* Pixmap or window in which to draw image. */
    int imageX, int imageY,	/* Upper-left corner of region within image to
				 * draw. */
    int width, int height,	/* Dimensions of region within image to draw. */
    int drawableX, int drawableY)
				/* Coordinates within drawable that correspond
				 * to imageX and imageY. */
{
    BitmapInstance *instancePtr = clientData;
    int masking;

    /*
     * If there's no graphics context, it means that an error occurred while
     * creating the image instance so it can't be displayed.
     */

    if (instancePtr->gc == None) {
	return;
    }

    /*
     * If masking is in effect, must modify the mask origin within the
     * graphics context to line up with the image's origin. Then draw the
     * image and reset the clip origin, if there's a mask.
     */

    masking = (instancePtr->mask != None) || (instancePtr->bg == NULL);
    if (masking) {
	XSetClipOrigin(display, instancePtr->gc, drawableX - imageX,
		drawableY - imageY);
    }
    XCopyPlane(display, instancePtr->bitmap, drawable, instancePtr->gc,
	    imageX, imageY, (unsigned) width, (unsigned) height,
	    drawableX, drawableY, 1);
    if (masking) {
	XSetClipOrigin(display, instancePtr->gc, 0, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapFree --
 *
 *	This procedure is called when a widget ceases to use a particular
 *	instance of an image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Internal data structures get cleaned up.
 *
 *----------------------------------------------------------------------
 */

static void
ImgBmapFree(
    ClientData clientData,	/* Pointer to BitmapInstance structure for
				 * instance to be displayed. */
    Display *display)		/* Display containing window that used image. */
{
    BitmapInstance *instancePtr = clientData;
    BitmapInstance *prevPtr;

    instancePtr->refCount--;
    if (instancePtr->refCount > 0) {
	return;
    }

    /*
     * There are no more uses of the image within this widget. Free the
     * instance structure.
     */

    if (instancePtr->fg != NULL) {
	Tk_FreeColor(instancePtr->fg);
    }
    if (instancePtr->bg != NULL) {
	Tk_FreeColor(instancePtr->bg);
    }
    if (instancePtr->bitmap != None) {
	Tk_FreePixmap(display, instancePtr->bitmap);
    }
    if (instancePtr->mask != None) {
	Tk_FreePixmap(display, instancePtr->mask);
    }
    if (instancePtr->gc != None) {
	Tk_FreeGC(display, instancePtr->gc);
    }
    if (instancePtr->masterPtr->instancePtr == instancePtr) {
	instancePtr->masterPtr->instancePtr = instancePtr->nextPtr;
    } else {
	for (prevPtr = instancePtr->masterPtr->instancePtr;
		prevPtr->nextPtr != instancePtr; prevPtr = prevPtr->nextPtr) {
	    /* Empty loop body */
	}
	prevPtr->nextPtr = instancePtr->nextPtr;
    }
    ckfree(instancePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapDelete --
 *
 *	This procedure is called by the image code to delete the master
 *	structure for an image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with the image get freed.
 *
 *----------------------------------------------------------------------
 */

static void
ImgBmapDelete(
    ClientData masterData)	/* Pointer to BitmapMaster structure for
				 * image. Must not have any more instances. */
{
    BitmapMaster *masterPtr = masterData;

    if (masterPtr->instancePtr != NULL) {
	Tcl_Panic("tried to delete bitmap image when instances still exist");
    }
    masterPtr->tkMaster = NULL;
    if (masterPtr->imageCmd != NULL) {
	Tcl_DeleteCommandFromToken(masterPtr->interp, masterPtr->imageCmd);
    }
    if (masterPtr->data != NULL) {
	ckfree(masterPtr->data);
    }
    if (masterPtr->maskData != NULL) {
	ckfree(masterPtr->maskData);
    }
    Tk_FreeOptions(configSpecs, (char *) masterPtr, NULL, 0);
    ckfree(masterPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapCmdDeletedProc --
 *
 *	This procedure is invoked when the image command for an image is
 *	deleted. It deletes the image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
ImgBmapCmdDeletedProc(
    ClientData clientData)	/* Pointer to BitmapMaster structure for
				 * image. */
{
    BitmapMaster *masterPtr = clientData;

    masterPtr->imageCmd = NULL;
    if (masterPtr->tkMaster != NULL) {
	Tk_DeleteImage(masterPtr->interp, Tk_NameOfImage(masterPtr->tkMaster));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetByte --
 *
 *	Get the next byte from the open channel.
 *
 * Results:
 *	The next byte or EOF.
 *
 * Side effects:
 *	We read from the channel.
 *
 *----------------------------------------------------------------------
 */

static int
GetByte(
    Tcl_Channel chan)	/* The channel we read from. */
{
    char buffer;
    int size;

    size = Tcl_Read(chan, &buffer, 1);
    if (size <= 0) {
	return EOF;
    } else {
	return buffer;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapPsImagemask --
 *
 *	This procedure generates postscript suitable for rendering a single
 *	bitmap of an image. A single bitmap image might contain both a
 *	foreground and a background bitmap. This routine is called once for
 *	each such bitmap in a bitmap image.
 *
 *	Prior to invoking this routine, the following setup has occurred:
 *
 *	   1.  The postscript foreground color has been set to the color used
 *	       to render the bitmap.
 *
 *	   2.  The origin of the postscript coordinate system is set to the
 *	       lower left corner of the bitmap.
 *
 *	   3.  The postscript coordinate system has been scaled so that the
 *	       entire bitmap is one unit squared.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Postscript code is appended to psObj.
 *
 *----------------------------------------------------------------------
 */

static void
ImgBmapPsImagemask(
    Tcl_Obj *psObj,		/* Append postscript to this buffer. */
    int width, int height,	/* Width and height of the bitmap in pixels */
    char *data)			/* Data for the bitmap. */
{
    int i, j, nBytePerRow;

    /*
     * The bit order of bitmaps in Tk is the opposite of the bit order that
     * postscript uses. (In Tk, the least significant bit is on the right side
     * of the bitmap and in postscript the least significant bit is shown on
     * the left.) The following array is used to reverse the order of bits
     * within a byte so that the bits will be in the order postscript expects.
     */

    static const unsigned char bit_reverse[] = {
       0, 128, 64, 192, 32, 160,  96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
       8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
       4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
      12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
       2, 130, 66, 194, 34, 162,  98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
      10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
       6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
      14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
       1, 129, 65, 193, 33, 161,  97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
       9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
       5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
      13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
       3, 131, 67, 195, 35, 163,  99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
      11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
       7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
      15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255,
    };

    Tcl_AppendPrintfToObj(psObj,
	    "0 0 moveto %d %d true [%d 0 0 %d 0 %d] {<\n",
	    width, height, width, -height, height);

    nBytePerRow = (width + 7) / 8;
    for (i=0; i<height; i++) {
	for (j=0; j<nBytePerRow; j++) {
	    Tcl_AppendPrintfToObj(psObj, " %02x",
		    bit_reverse[0xff & data[i*nBytePerRow + j]]);
	}
	Tcl_AppendToObj(psObj, "\n", -1);
    }

    Tcl_AppendToObj(psObj, ">} imagemask \n", -1);
}

/*
 *----------------------------------------------------------------------
 *
 * ImgBmapPostscript --
 *
 *	This procedure generates postscript for rendering a bitmap image.
 *
 * Results:
 *	On success, this routine writes postscript code into interp->result
 *	and returns TCL_OK TCL_ERROR is returned and an error message is left
 *	in interp->result if anything goes wrong.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ImgBmapPostscript(
    ClientData clientData,
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tk_PostscriptInfo psinfo,
    int x, int y, int width, int height,
    int prepass)
{
    BitmapMaster *masterPtr = clientData;
    Tcl_InterpState interpState;
    Tcl_Obj *psObj;

    if (prepass) {
	return TCL_OK;
    }

    /*
     * There is nothing to do for bitmaps with zero width or height.
     */

    if (width<=0 || height<=0 || masterPtr->width<=0 || masterPtr->height<=0){
	return TCL_OK;
    }

    /*
     * Some postscript implementations cannot handle bitmap strings longer
     * than about 60k characters. If the bitmap data is that big or bigger,
     * we bail out.
     */

    if (masterPtr->width*masterPtr->height > 60000) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"unable to generate postscript for bitmaps larger than 60000"
		" pixels", -1));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "MEMLIMIT", NULL);
	return TCL_ERROR;
    }

    /*
     * Make our working space.
     */

    psObj = Tcl_NewObj();
    interpState = Tcl_SaveInterpState(interp, TCL_OK);

    /*
     * Translate the origin of the coordinate system to be the lower-left
     * corner of the bitmap and adjust the scale of the coordinate system so
     * that entire bitmap covers one square unit of the page. The calling
     * function put a "gsave" into the postscript and will add a "grestore" at
     * after this routine returns, so it is safe to make whatever changes are
     * necessary here.
     */

    if (x != 0 || y != 0) {
	Tcl_AppendPrintfToObj(psObj, "%d %d moveto\n", x, y);
    }
    if (width != 1 || height != 1) {
	Tcl_AppendPrintfToObj(psObj, "%d %d scale\n", width, height);
    }

    /*
     * Color the background, if there is one. This step is skipped if the
     * background is transparent. If the background is not transparent and
     * there is no background mask, then color the complete rectangle that
     * encloses the bitmap. If there is a background mask, then only apply
     * color to the bits specified by the mask.
     */

    if ((masterPtr->bgUid != NULL) && (masterPtr->bgUid[0] != '\000')) {
	XColor color;

	TkParseColor(Tk_Display(tkwin), Tk_Colormap(tkwin), masterPtr->bgUid,
		&color);
	Tcl_ResetResult(interp);
	if (Tk_PostscriptColor(interp, psinfo, &color) != TCL_OK) {
	    goto error;
	}
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	if (masterPtr->maskData == NULL) {
	    Tcl_AppendToObj(psObj,
		    "0 0 moveto 1 0 rlineto 0 1 rlineto -1 0 rlineto "
		    "closepath fill\n", -1);
	} else {
	    ImgBmapPsImagemask(psObj, masterPtr->width, masterPtr->height,
		    masterPtr->maskData);
	}
    }

    /*
     * Draw the bitmap foreground, assuming there is one.
     */

    if ((masterPtr->fgUid != NULL) && (masterPtr->data != NULL)) {
	XColor color;

	TkParseColor(Tk_Display(tkwin), Tk_Colormap(tkwin), masterPtr->fgUid,
		&color);
	Tcl_ResetResult(interp);
	if (Tk_PostscriptColor(interp, psinfo, &color) != TCL_OK) {
	    goto error;
	}
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

	ImgBmapPsImagemask(psObj, masterPtr->width, masterPtr->height,
		masterPtr->data);
    }

    /*
     * Plug the accumulated postscript back into the result.
     */

    (void) Tcl_RestoreInterpState(interp, interpState);
    Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
    Tcl_DecrRefCount(psObj);
    return TCL_OK;

  error:
    Tcl_DiscardInterpState(interpState);
    Tcl_DecrRefCount(psObj);
    return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
