/*
 * tkCanvPs.c --
 *
 *	This module provides Postscript output support for canvases, including
 *	the "postscript" widget command plus a few utility functions used for
 *	generating Postscript.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkCanvas.h"
#include "tkFont.h"

/*
 * See tkCanvas.h for key data structures used to implement canvases.
 */

/*
 * The following definition is used in generating postscript for images and
 * windows.
 */

typedef struct TkColormapData {	/* Hold color information for a window */
    int separated;		/* Whether to use separate color bands */
    int color;			/* Whether window is color or black/white */
    int ncolors;		/* Number of color values stored */
    XColor *colors;		/* Pixel value -> RGB mappings */
    int red_mask, green_mask, blue_mask;	/* Masks and shifts for each */
    int red_shift, green_shift, blue_shift;	/* color band */
} TkColormapData;

/*
 * One of the following structures is created to keep track of Postscript
 * output being generated. It consists mostly of information provided on the
 * widget command line.
 */

typedef struct TkPostscriptInfo {
    int x, y, width, height;	/* Area to print, in canvas pixel
				 * coordinates. */
    int x2, y2;			/* x+width and y+height. */
    char *pageXString;		/* String value of "-pagex" option or NULL. */
    char *pageYString;		/* String value of "-pagey" option or NULL. */
    double pageX, pageY;	/* Postscript coordinates (in points)
				 * corresponding to pageXString and
				 * pageYString. Don't forget that y-values
				 * grow upwards for Postscript! */
    char *pageWidthString;	/* Printed width of output. */
    char *pageHeightString;	/* Printed height of output. */
    double scale;		/* Scale factor for conversion: each pixel
				 * maps into this many points. */
    Tk_Anchor pageAnchor;	/* How to anchor bbox on Postscript page. */
    int rotate;			/* Non-zero means output should be rotated on
				 * page (landscape mode). */
    char *fontVar;		/* If non-NULL, gives name of global variable
				 * containing font mapping information.
				 * Malloc'ed. */
    char *colorVar;		/* If non-NULL, give name of global variable
				 * containing color mapping information.
				 * Malloc'ed. */
    char *colorMode;		/* Mode for handling colors: "monochrome",
				 * "gray", or "color".  Malloc'ed. */
    int colorLevel;		/* Numeric value corresponding to colorMode: 0
				 * for mono, 1 for gray, 2 for color. */
    char *fileName;		/* Name of file in which to write Postscript;
				 * NULL means return Postscript info as
				 * result. Malloc'ed. */
    char *channelName;		/* If -channel is specified, the name of the
				 * channel to use. */
    Tcl_Channel chan;		/* Open channel corresponding to fileName. */
    Tcl_HashTable fontTable;	/* Hash table containing names of all font
				 * families used in output. The hash table
				 * values are not used. */
    int prepass;		/* Non-zero means that we're currently in the
				 * pre-pass that collects font information, so
				 * the Postscript generated isn't relevant. */
    int prolog;			/* Non-zero means output should contain the
				 * standard prolog in the header. Generated in
				 * library/mkpsenc.tcl, stored in the variable
				 * ::tk::ps_preamable [sic]. */
    Tk_Window tkwin;		/* Window to get font pixel/point transform
				 * from. */
} TkPostscriptInfo;

/*
 * The table below provides a template that's used to process arguments to the
 * canvas "postscript" command and fill in TkPostscriptInfo structures.
 */

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_STRING, "-colormap", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, colorVar), 0, NULL},
    {TK_CONFIG_STRING, "-colormode", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, colorMode), 0, NULL},
    {TK_CONFIG_STRING, "-file", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, fileName), 0, NULL},
    {TK_CONFIG_STRING, "-channel", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, channelName), 0, NULL},
    {TK_CONFIG_STRING, "-fontmap", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, fontVar), 0, NULL},
    {TK_CONFIG_PIXELS, "-height", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, height), 0, NULL},
    {TK_CONFIG_ANCHOR, "-pageanchor", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, pageAnchor), 0, NULL},
    {TK_CONFIG_STRING, "-pageheight", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, pageHeightString), 0, NULL},
    {TK_CONFIG_STRING, "-pagewidth", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, pageWidthString), 0, NULL},
    {TK_CONFIG_STRING, "-pagex", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, pageXString), 0, NULL},
    {TK_CONFIG_STRING, "-pagey", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, pageYString), 0, NULL},
    {TK_CONFIG_BOOLEAN, "-prolog", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, prolog), 0, NULL},
    {TK_CONFIG_BOOLEAN, "-rotate", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, rotate), 0, NULL},
    {TK_CONFIG_PIXELS, "-width", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, width), 0, NULL},
    {TK_CONFIG_PIXELS, "-x", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, x), 0, NULL},
    {TK_CONFIG_PIXELS, "-y", NULL, NULL,
	"", Tk_Offset(TkPostscriptInfo, y), 0, NULL},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};

/*
 * Forward declarations for functions defined later in this file:
 */

static int		GetPostscriptPoints(Tcl_Interp *interp,
			    char *string, double *doublePtr);
static void		PostscriptBitmap(Tk_Window tkwin, Pixmap bitmap,
			    int startX, int startY, int width, int height,
			    Tcl_Obj *psObj);
static inline Tcl_Obj *	GetPostscriptBuffer(Tcl_Interp *interp);

/*
 *--------------------------------------------------------------
 *
 * TkCanvPostscriptCmd --
 *
 *	This function is invoked to process the "postscript" options of the
 *	widget command for canvas widgets. See the user documentation for
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

    /* ARGSUSED */
int
TkCanvPostscriptCmd(
    TkCanvas *canvasPtr,	/* Information about canvas widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. Caller has already parsed
				 * this command enough to know that argv[1] is
				 * "postscript". */
{
    TkPostscriptInfo psInfo, *psInfoPtr = &psInfo;
    Tk_PostscriptInfo oldInfoPtr;
    int result;
    Tk_Item *itemPtr;
#define STRING_LENGTH 400
    const char *p;
    time_t now;
    size_t length;
    Tk_Window tkwin = canvasPtr->tkwin;
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Tcl_DString buffer;
    Tcl_Obj *preambleObj;
    Tcl_Obj *psObj;
    int deltaX = 0, deltaY = 0;	/* Offset of lower-left corner of area to be
				 * marked up, measured in canvas units from
				 * the positioning point on the page (reflects
				 * anchor position). Initial values needed
				 * only to stop compiler warnings. */

    /*
     * Get the generic preamble. We only ever bother with the ASCII encoding;
     * the others just make life too complicated and never actually worked as
     * such.
     */

    result = Tcl_EvalEx(interp, "::tk::ensure_psenc_is_loaded", -1, 0);
    if (result != TCL_OK) {
	return result;
    }
    preambleObj = Tcl_GetVar2Ex(interp, "::tk::ps_preamble", NULL,
	    TCL_LEAVE_ERR_MSG);
    if (preambleObj == NULL) {
	return TCL_ERROR;
    }
    Tcl_IncrRefCount(preambleObj);
    Tcl_ResetResult(interp);
    psObj = Tcl_NewObj();

    /*
     * Initialize the data structure describing Postscript generation, then
     * process all the arguments to fill the data structure in.
     */

    oldInfoPtr = canvasPtr->psInfo;
    canvasPtr->psInfo = (Tk_PostscriptInfo) psInfoPtr;
    psInfo.x = canvasPtr->xOrigin;
    psInfo.y = canvasPtr->yOrigin;
    psInfo.width = -1;
    psInfo.height = -1;
    psInfo.pageXString = NULL;
    psInfo.pageYString = NULL;
    psInfo.pageX = 72*4.25;
    psInfo.pageY = 72*5.5;
    psInfo.pageWidthString = NULL;
    psInfo.pageHeightString = NULL;
    psInfo.scale = 1.0;
    psInfo.pageAnchor = TK_ANCHOR_CENTER;
    psInfo.rotate = 0;
    psInfo.fontVar = NULL;
    psInfo.colorVar = NULL;
    psInfo.colorMode = NULL;
    psInfo.colorLevel = 0;
    psInfo.fileName = NULL;
    psInfo.channelName = NULL;
    psInfo.chan = NULL;
    psInfo.prepass = 0;
    psInfo.prolog = 1;
    psInfo.tkwin = tkwin;
    Tcl_InitHashTable(&psInfo.fontTable, TCL_STRING_KEYS);
    result = Tk_ConfigureWidget(interp, tkwin, configSpecs, argc-2, argv+2,
	    (char *) &psInfo, TK_CONFIG_ARGV_ONLY);
    if (result != TCL_OK) {
	goto cleanup;
    }

    if (psInfo.width == -1) {
	psInfo.width = Tk_Width(tkwin);
    }
    if (psInfo.height == -1) {
	psInfo.height = Tk_Height(tkwin);
    }
    psInfo.x2 = psInfo.x + psInfo.width;
    psInfo.y2 = psInfo.y + psInfo.height;

    if (psInfo.pageXString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageXString,
		&psInfo.pageX) != TCL_OK) {
	    goto cleanup;
	}
    }
    if (psInfo.pageYString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageYString,
		&psInfo.pageY) != TCL_OK) {
	    goto cleanup;
	}
    }
    if (psInfo.pageWidthString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageWidthString,
		&psInfo.scale) != TCL_OK) {
	    goto cleanup;
	}
	psInfo.scale /= psInfo.width;
    } else if (psInfo.pageHeightString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageHeightString,
		&psInfo.scale) != TCL_OK) {
	    goto cleanup;
	}
	psInfo.scale /= psInfo.height;
    } else {
	psInfo.scale = (72.0/25.4)*WidthMMOfScreen(Tk_Screen(tkwin));
	psInfo.scale /= WidthOfScreen(Tk_Screen(tkwin));
    }
    switch (psInfo.pageAnchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_W:
    case TK_ANCHOR_SW:
	deltaX = 0;
	break;
    case TK_ANCHOR_N:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_S:
	deltaX = -psInfo.width/2;
	break;
    case TK_ANCHOR_NE:
    case TK_ANCHOR_E:
    case TK_ANCHOR_SE:
	deltaX = -psInfo.width;
	break;
    }
    switch (psInfo.pageAnchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_N:
    case TK_ANCHOR_NE:
	deltaY = - psInfo.height;
	break;
    case TK_ANCHOR_W:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_E:
	deltaY = -psInfo.height/2;
	break;
    case TK_ANCHOR_SW:
    case TK_ANCHOR_S:
    case TK_ANCHOR_SE:
	deltaY = 0;
	break;
    }

    if (psInfo.colorMode == NULL) {
	psInfo.colorLevel = 2;
    } else {
	length = strlen(psInfo.colorMode);
	if (strncmp(psInfo.colorMode, "monochrome", length) == 0) {
	    psInfo.colorLevel = 0;
	} else if (strncmp(psInfo.colorMode, "gray", length) == 0) {
	    psInfo.colorLevel = 1;
	} else if (strncmp(psInfo.colorMode, "color", length) == 0) {
	    psInfo.colorLevel = 2;
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad color mode \"%s\": must be monochrome, gray, or color",
		    psInfo.colorMode));
	    Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "COLORMODE", NULL);
	    result = TCL_ERROR;
	    goto cleanup;
	}
    }

    if (psInfo.fileName != NULL) {
	/*
	 * Check that -file and -channel are not both specified.
	 */

	if (psInfo.channelName != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't specify both -file and -channel", -1));
	    Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "USAGE", NULL);
	    result = TCL_ERROR;
	    goto cleanup;
	}

	/*
	 * Check that we are not in a safe interpreter. If we are, disallow
	 * the -file specification.
	 */

	if (Tcl_IsSafe(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't specify -file in a safe interpreter", -1));
	    Tcl_SetErrorCode(interp, "TK", "SAFE", "PS_FILE", NULL);
	    result = TCL_ERROR;
	    goto cleanup;
	}

	p = Tcl_TranslateFileName(interp, psInfo.fileName, &buffer);
	if (p == NULL) {
	    goto cleanup;
	}
	psInfo.chan = Tcl_OpenFileChannel(interp, p, "w", 0666);
	Tcl_DStringFree(&buffer);
	if (psInfo.chan == NULL) {
	    goto cleanup;
	}
    }

    if (psInfo.channelName != NULL) {
	int mode;

	/*
	 * Check that the channel is found in this interpreter and that it is
	 * open for writing.
	 */

	psInfo.chan = Tcl_GetChannel(interp, psInfo.channelName, &mode);
	if (psInfo.chan == NULL) {
	    result = TCL_ERROR;
	    goto cleanup;
	}
	if (!(mode & TCL_WRITABLE)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "channel \"%s\" wasn't opened for writing",
		    psInfo.channelName));
	    Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "UNWRITABLE",NULL);
	    result = TCL_ERROR;
	    goto cleanup;
	}
    }

    /*
     * Make a pre-pass over all of the items, generating Postscript and then
     * throwing it away. The purpose of this pass is just to collect
     * information about all the fonts in use, so that we can output font
     * information in the proper form required by the Document Structuring
     * Conventions.
     */

    psInfo.prepass = 1;
    for (itemPtr = canvasPtr->firstItemPtr; itemPtr != NULL;
	    itemPtr = itemPtr->nextPtr) {
	if ((itemPtr->x1 >= psInfo.x2) || (itemPtr->x2 < psInfo.x)
		|| (itemPtr->y1 >= psInfo.y2) || (itemPtr->y2 < psInfo.y)) {
	    continue;
	}
	if (itemPtr->typePtr->postscriptProc == NULL) {
	    continue;
	}
	result = itemPtr->typePtr->postscriptProc(interp,
		(Tk_Canvas) canvasPtr, itemPtr, 1);
	Tcl_ResetResult(interp);
	if (result != TCL_OK) {
	    /*
	     * An error just occurred. Just skip out of this loop. There's no
	     * need to report the error now; it can be reported later (errors
	     * can happen later that don't happen now, so we still have to
	     * check for errors later anyway).
	     */

	    break;
	}
    }
    psInfo.prepass = 0;

    /*
     * Generate the header and prolog for the Postscript.
     */

    if (psInfo.prolog) {
	Tcl_AppendToObj(psObj,
		"%!PS-Adobe-3.0 EPSF-3.0\n"
		"%%Creator: Tk Canvas Widget\n", -1);

#ifdef HAVE_PW_GECOS
	if (!Tcl_IsSafe(interp)) {
	    struct passwd *pwPtr = getpwuid(getuid());	/* INTL: Native. */

	    Tcl_AppendPrintfToObj(psObj,
		    "%%%%For: %s\n", (pwPtr ? pwPtr->pw_gecos : "Unknown"));
	    endpwent();
	}
#endif /* HAVE_PW_GECOS */
	Tcl_AppendPrintfToObj(psObj,
		"%%%%Title: Window %s\n", Tk_PathName(tkwin));
	time(&now);
	Tcl_AppendPrintfToObj(psObj,
		"%%%%CreationDate: %s", ctime(&now));	/* INTL: Native. */
	if (!psInfo.rotate) {
	    Tcl_AppendPrintfToObj(psObj,
		    "%%%%BoundingBox: %d %d %d %d\n",
		    (int) (psInfo.pageX + psInfo.scale*deltaX),
		    (int) (psInfo.pageY + psInfo.scale*deltaY),
		    (int) (psInfo.pageX + psInfo.scale*(deltaX + psInfo.width)
			    + 1.0),
		    (int) (psInfo.pageY + psInfo.scale*(deltaY + psInfo.height)
			    + 1.0));
	} else {
	    Tcl_AppendPrintfToObj(psObj,
		    "%%%%BoundingBox: %d %d %d %d\n",
		    (int) (psInfo.pageX - psInfo.scale*(deltaY+psInfo.height)),
		    (int) (psInfo.pageY + psInfo.scale*deltaX),
		    (int) (psInfo.pageX - psInfo.scale*deltaY + 1.0),
		    (int) (psInfo.pageY + psInfo.scale*(deltaX + psInfo.width)
			    + 1.0));
	}
	Tcl_AppendPrintfToObj(psObj,
		"%%%%Pages: 1\n"
		"%%%%DocumentData: Clean7Bit\n"
		"%%%%Orientation: %s\n",
		psInfo.rotate ? "Landscape" : "Portrait");
	p = "%%%%DocumentNeededResources: font %s\n";
	for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_AppendPrintfToObj(psObj, p,
		    Tcl_GetHashKey(&psInfo.fontTable, hPtr));
	    p = "%%%%+ font %s\n";
	}
	Tcl_AppendToObj(psObj, "%%EndComments\n\n", -1);

	/*
	 * Insert the prolog
	 */

	Tcl_AppendObjToObj(psObj, preambleObj);

	if (psInfo.chan != NULL) {
	    if (Tcl_WriteObj(psInfo.chan, psObj) == -1) {
	    channelWriteFailed:
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"problem writing postscript data to channel: %s",
			Tcl_PosixError(interp)));
		result = TCL_ERROR;
		goto cleanup;
	    }
	    Tcl_DecrRefCount(psObj);
	    psObj = Tcl_NewObj();
	}

	/*
	 * Document setup:  set the color level and include fonts.
	 */

	Tcl_AppendPrintfToObj(psObj,
		"%%%%BeginSetup\n/CL %d def\n", psInfo.colorLevel);
	for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_AppendPrintfToObj(psObj,
		    "%%%%IncludeResource: font %s\n",
		    (char *) Tcl_GetHashKey(&psInfo.fontTable, hPtr));
	}
	Tcl_AppendToObj(psObj, "%%EndSetup\n\n", -1);

	/*
	 * Page setup: move to page positioning point, rotate if needed, set
	 * scale factor, offset for proper anchor position, and set clip
	 * region.
	 */

	Tcl_AppendToObj(psObj, "%%Page: 1 1\nsave\n", -1);
	Tcl_AppendPrintfToObj(psObj,
		"%.1f %.1f translate\n", psInfo.pageX, psInfo.pageY);
	if (psInfo.rotate) {
	    Tcl_AppendToObj(psObj, "90 rotate\n", -1);
	}
	Tcl_AppendPrintfToObj(psObj,
		"%.4g %.4g scale\n", psInfo.scale, psInfo.scale);
	Tcl_AppendPrintfToObj(psObj,
		"%d %d translate\n", deltaX - psInfo.x, deltaY);
	Tcl_AppendPrintfToObj(psObj,
		"%d %.15g moveto %d %.15g lineto %d %.15g lineto %d %.15g "
		"lineto closepath clip newpath\n",
		psInfo.x, Tk_PostscriptY((double)psInfo.y,
			(Tk_PostscriptInfo)psInfoPtr),
		psInfo.x2, Tk_PostscriptY((double)psInfo.y,
			(Tk_PostscriptInfo)psInfoPtr),
		psInfo.x2, Tk_PostscriptY((double)psInfo.y2,
			(Tk_PostscriptInfo)psInfoPtr),
		psInfo.x, Tk_PostscriptY((double)psInfo.y2,
			(Tk_PostscriptInfo)psInfoPtr));
	if (psInfo.chan != NULL) {
	    if (Tcl_WriteObj(psInfo.chan, psObj) == -1) {
		goto channelWriteFailed;
	    }
	    Tcl_DecrRefCount(psObj);
	    psObj = Tcl_NewObj();
	}
    }

    /*
     * Iterate through all the items, having each relevant one draw itself.
     * Quit if any of the items returns an error.
     */

    result = TCL_OK;
    for (itemPtr = canvasPtr->firstItemPtr; itemPtr != NULL;
	    itemPtr = itemPtr->nextPtr) {
	if ((itemPtr->x1 >= psInfo.x2) || (itemPtr->x2 < psInfo.x)
		|| (itemPtr->y1 >= psInfo.y2) || (itemPtr->y2 < psInfo.y)) {
	    continue;
	}
	if (itemPtr->typePtr->postscriptProc == NULL) {
	    continue;
	}
	if (itemPtr->state == TK_STATE_HIDDEN) {
	    continue;
	}

	Tcl_ResetResult(interp);
	result = itemPtr->typePtr->postscriptProc(interp,
		(Tk_Canvas) canvasPtr, itemPtr, 0);
	if (result != TCL_OK) {
	    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		    "\n    (generating Postscript for item %d)",
		    itemPtr->id));
	    goto cleanup;
	}

	Tcl_AppendToObj(psObj, "gsave\n", -1);
	Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
	Tcl_AppendToObj(psObj, "grestore\n", -1);

	if (psInfo.chan != NULL) {
	    if (Tcl_WriteObj(psInfo.chan, psObj) == -1) {
		goto channelWriteFailed;
	    }
	    Tcl_DecrRefCount(psObj);
	    psObj = Tcl_NewObj();
	}
    }

    /*
     * Output page-end information, such as commands to print the page and
     * document trailer stuff.
     */

    if (psInfo.prolog) {
	Tcl_AppendToObj(psObj,
		"restore showpage\n\n"
		"%%Trailer\n"
		"end\n"
		"%%EOF\n", -1);

	if (psInfo.chan != NULL) {
	    if (Tcl_WriteObj(psInfo.chan, psObj) == -1) {
		goto channelWriteFailed;
	    }
	}
    }

    if (psInfo.chan == NULL) {
	Tcl_SetObjResult(interp, psObj);
	psObj = Tcl_NewObj();
    }

    /*
     * Clean up psInfo to release malloc'ed stuff.
     */

  cleanup:
    if (psInfo.pageXString != NULL) {
	ckfree(psInfo.pageXString);
    }
    if (psInfo.pageYString != NULL) {
	ckfree(psInfo.pageYString);
    }
    if (psInfo.pageWidthString != NULL) {
	ckfree(psInfo.pageWidthString);
    }
    if (psInfo.pageHeightString != NULL) {
	ckfree(psInfo.pageHeightString);
    }
    if (psInfo.fontVar != NULL) {
	ckfree(psInfo.fontVar);
    }
    if (psInfo.colorVar != NULL) {
	ckfree(psInfo.colorVar);
    }
    if (psInfo.colorMode != NULL) {
	ckfree(psInfo.colorMode);
    }
    if (psInfo.fileName != NULL) {
	ckfree(psInfo.fileName);
    }
    if ((psInfo.chan != NULL) && (psInfo.channelName == NULL)) {
	Tcl_Close(interp, psInfo.chan);
    }
    if (psInfo.channelName != NULL) {
	ckfree(psInfo.channelName);
    }
    Tcl_DeleteHashTable(&psInfo.fontTable);
    canvasPtr->psInfo = (Tk_PostscriptInfo) oldInfoPtr;
    Tcl_DecrRefCount(preambleObj);
    Tcl_DecrRefCount(psObj);
    return result;
}

static inline Tcl_Obj *
GetPostscriptBuffer(
    Tcl_Interp *interp)
{
    Tcl_Obj *psObj = Tcl_GetObjResult(interp);

    if (Tcl_IsShared(psObj)) {
	psObj = Tcl_DuplicateObj(psObj);
	Tcl_SetObjResult(interp, psObj);
    }
    return psObj;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptColor --
 *
 *	This function is called by individual canvas items when they want to
 *	set a color value for output. Given information about an X color, this
 *	function will generate Postscript commands to set up an appropriate
 *	color in Postscript.
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in the interp's result. If no error occurs, then
 *	additional Postscript will be appended to the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptColor(
    Tcl_Interp *interp,
    Tk_PostscriptInfo psInfo,	/* Postscript info. */
    XColor *colorPtr)		/* Information about color. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    double red, green, blue;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    /*
     * If there is a color map defined, then look up the color's name in the
     * map and use the Postscript commands found there, if there are any.
     */

    if (psInfoPtr->colorVar != NULL) {
	const char *cmdString = Tcl_GetVar2(interp, psInfoPtr->colorVar,
		Tk_NameOfColor(colorPtr), 0);

	if (cmdString != NULL) {
	    Tcl_AppendPrintfToObj(GetPostscriptBuffer(interp),
		    "%s\n", cmdString);
	    return TCL_OK;
	}
    }

    /*
     * No color map entry for this color. Grab the color's intensities and
     * output Postscript commands for them. Special note: X uses a range of
     * 0-65535 for intensities, but most displays only use a range of 0-255,
     * which maps to (0, 256, 512, ... 65280) in the X scale. This means that
     * there's no way to get perfect white, since the highest intensity is
     * only 65280 out of 65535. To work around this problem, rescale the X
     * intensity to a 0-255 scale and use that as the basis for the Postscript
     * colors. This scheme still won't work if the display only uses 4 bits
     * per color, but most diplays use at least 8 bits.
     */

    red = ((double) (((int) colorPtr->red) >> 8))/255.0;
    green = ((double) (((int) colorPtr->green) >> 8))/255.0;
    blue = ((double) (((int) colorPtr->blue) >> 8))/255.0;
    Tcl_AppendPrintfToObj(GetPostscriptBuffer(interp),
	    "%.3f %.3f %.3f setrgbcolor AdjustColor\n",
	    red, green, blue);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptFont --
 *
 *	This function is called by individual canvas items when they want to
 *	output text. Given information about an X font, this function will
 *	generate Postscript commands to set up an appropriate font in
 *	Postscript.
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in the interp's result. If no error occurs, then
 *	additional Postscript will be appended to the interp's result.
 *
 * Side effects:
 *	The Postscript font name is entered into psInfoPtr->fontTable if it
 *	wasn't already there.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptFont(
    Tcl_Interp *interp,
    Tk_PostscriptInfo psInfo,	/* Postscript Info. */
    Tk_Font tkfont)		/* Information about font in which text is to
				 * be printed. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    Tcl_DString ds;
    int i, points;
    const char *fontname;

    /*
     * First, look up the font's name in the font map, if there is one. If
     * there is an entry for this font, it consists of a list containing font
     * name and size. Use this information.
     */

    if (psInfoPtr->fontVar != NULL) {
	const char *name = Tk_NameOfFont(tkfont);
	Tcl_Obj **objv;
	int objc;
	double size;
	Tcl_Obj *list = Tcl_GetVar2Ex(interp, psInfoPtr->fontVar, name, 0);

	if (list != NULL) {
	    if (Tcl_ListObjGetElements(interp, list, &objc, &objv) != TCL_OK
		    || objc != 2
		    || (fontname = Tcl_GetString(objv[0]))[0] == '\0'
		    || strchr(fontname, ' ') != NULL
		    || Tcl_GetDoubleFromObj(interp, objv[1], &size) != TCL_OK
		    || size <= 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"bad font map entry for \"%s\": \"%s\"",
			name, Tcl_GetString(list)));
		Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "FONTMAP",
			NULL);
		return TCL_ERROR;
	    }

	    Tcl_AppendPrintfToObj(GetPostscriptBuffer(interp),
		    "/%s findfont %d scalefont%s setfont\n",
		    fontname, (int) size,
		    strncasecmp(fontname, "Symbol", 7) ? " ISOEncode" : "");
	    Tcl_CreateHashEntry(&psInfoPtr->fontTable, fontname, &i);
	    return TCL_OK;
	}
    }

    /*
     * Nothing in the font map, so fall back to the old guessing technique.
     */

    Tcl_DStringInit(&ds);
    points = Tk_PostscriptFontName(tkfont, &ds);
    fontname = Tcl_DStringValue(&ds);
    Tcl_AppendPrintfToObj(GetPostscriptBuffer(interp),
	    "/%s findfont %d scalefont%s setfont\n",
	    fontname, (int)(TkFontGetPoints(psInfoPtr->tkwin, points) + 0.5),
	    strncasecmp(fontname, "Symbol", 7) ? " ISOEncode" : "");
    Tcl_CreateHashEntry(&psInfoPtr->fontTable, Tcl_DStringValue(&ds), &i);
    Tcl_DStringFree(&ds);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptBitmap --
 *
 *	This function is called to output the contents of a sub-region of a
 *	bitmap in proper image data format for Postscript (i.e. data between
 *	angle brackets, one bit per pixel).
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in the interp's result. If no error occurs, then
 *	additional Postscript will be appended to the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptBitmap(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tk_PostscriptInfo psInfo,	/* Postscript info. */
    Pixmap bitmap,		/* Bitmap for which to generate Postscript. */
    int startX, int startY,	/* Coordinates of upper-left corner of
				 * rectangular region to output. */
    int width, int height)	/* Height of rectangular region. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    PostscriptBitmap(tkwin, bitmap, startX, startY, width, height,
	    GetPostscriptBuffer(interp));
    return TCL_OK;
}

static void
PostscriptBitmap(
    Tk_Window tkwin,
    Pixmap bitmap,		/* Bitmap for which to generate Postscript. */
    int startX, int startY,	/* Coordinates of upper-left corner of
				 * rectangular region to output. */
    int width, int height,	/* Height of rectangular region. */
    Tcl_Obj *psObj)		/* Where to append the postscript. */
{
    XImage *imagePtr;
    int charsInLine, x, y, lastX, lastY, value, mask;
    unsigned int totalWidth, totalHeight;
    Window dummyRoot;
    int dummyX, dummyY;
    unsigned dummyBorderwidth, dummyDepth;

    /*
     * The following call should probably be a call to Tk_SizeOfBitmap
     * instead, but it seems that we are occasionally invoked by custom item
     * types that create their own bitmaps without registering them with Tk.
     * XGetGeometry is a bit slower than Tk_SizeOfBitmap, but it shouldn't
     * matter here.
     */

    XGetGeometry(Tk_Display(tkwin), bitmap, &dummyRoot,
	    (int *) &dummyX, (int *) &dummyY, (unsigned int *) &totalWidth,
	    (unsigned int *) &totalHeight, &dummyBorderwidth, &dummyDepth);
    imagePtr = XGetImage(Tk_Display(tkwin), bitmap, 0, 0,
	    totalWidth, totalHeight, 1, XYPixmap);

    Tcl_AppendToObj(psObj, "<", -1);
    mask = 0x80;
    value = 0;
    charsInLine = 0;
    lastX = startX + width - 1;
    lastY = startY + height - 1;
    for (y = lastY; y >= startY; y--) {
	for (x = startX; x <= lastX; x++) {
	    if (XGetPixel(imagePtr, x, y)) {
		value |= mask;
	    }
	    mask >>= 1;
	    if (mask == 0) {
		Tcl_AppendPrintfToObj(psObj, "%02x", value);
		mask = 0x80;
		value = 0;
		charsInLine += 2;
		if (charsInLine >= 60) {
		    Tcl_AppendToObj(psObj, "\n", -1);
		    charsInLine = 0;
		}
	    }
	}
	if (mask != 0x80) {
	    Tcl_AppendPrintfToObj(psObj, "%02x", value);
	    mask = 0x80;
	    value = 0;
	    charsInLine += 2;
	}
    }
    Tcl_AppendToObj(psObj, ">", -1);

    XDestroyImage(imagePtr);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptStipple --
 *
 *	This function is called by individual canvas items when they have
 *	created a path that they'd like to be filled with a stipple pattern.
 *	Given information about an X bitmap, this function will generate
 *	Postscript commands to fill the current clip region using a stipple
 *	pattern defined by the bitmap.
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in the interp's result. If no error occurs, then
 *	additional Postscript will be appended to the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptStipple(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tk_PostscriptInfo psInfo,	/* Interpreter for returning Postscript or
				 * error message. */
    Pixmap bitmap)		/* Bitmap to use for stippling. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    int width, height;
    Window dummyRoot;
    int dummyX, dummyY;
    unsigned dummyBorderwidth, dummyDepth;
    Tcl_Obj *psObj;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    /*
     * The following call should probably be a call to Tk_SizeOfBitmap
     * instead, but it seems that we are occasionally invoked by custom item
     * types that create their own bitmaps without registering them with Tk.
     * XGetGeometry is a bit slower than Tk_SizeOfBitmap, but it shouldn't
     * matter here.
     */

    XGetGeometry(Tk_Display(tkwin), bitmap, &dummyRoot,
	    (int *) &dummyX, (int *) &dummyY, (unsigned *) &width,
	    (unsigned *) &height, &dummyBorderwidth, &dummyDepth);

    psObj = GetPostscriptBuffer(interp);
    Tcl_AppendPrintfToObj(psObj, "%d %d ", width, height);
    PostscriptBitmap(tkwin, bitmap, 0, 0, width, height, psObj);
    Tcl_AppendToObj(psObj, " StippleFill\n", -1);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptY --
 *
 *	Given a y-coordinate in local coordinates, this function returns a
 *	y-coordinate to use for Postscript output. Required because canvases
 *	have their origin in the top-left, but postscript pages have their
 *	origin in the bottom left.
 *
 * Results:
 *	Returns the Postscript coordinate that corresponds to "y".
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
Tk_PostscriptY(
    double y,			/* Y-coordinate in canvas coords. */
    Tk_PostscriptInfo psInfo)	/* Postscript info */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;

    return psInfoPtr->y2 - y;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptPath --
 *
 *	Given an array of points for a path, generate Postscript commands to
 *	create the path.
 *
 * Results:
 *	Postscript commands get appended to what's in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
Tk_PostscriptPath(
    Tcl_Interp *interp,
    Tk_PostscriptInfo psInfo,	/* Canvas on whose behalf Postscript is being
				 * generated. */
    double *coordPtr,		/* Pointer to first in array of 2*numPoints
				 * coordinates giving points for path. */
    int numPoints)		/* Number of points at *coordPtr. */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    Tcl_Obj *psObj;

    if (psInfoPtr->prepass) {
	return;
    }

    psObj = GetPostscriptBuffer(interp);
    Tcl_AppendPrintfToObj(psObj, "%.15g %.15g moveto\n",
	    coordPtr[0], Tk_PostscriptY(coordPtr[1], psInfo));
    for (numPoints--, coordPtr += 2; numPoints > 0;
	    numPoints--, coordPtr += 2) {
	Tcl_AppendPrintfToObj(psObj, "%.15g %.15g lineto\n",
		coordPtr[0], Tk_PostscriptY(coordPtr[1], psInfo));
    }
}

/*
 *--------------------------------------------------------------
 *
 * GetPostscriptPoints --
 *
 *	Given a string, returns the number of Postscript points corresponding
 *	to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result. If TCL_OK is
 *	returned, then everything went well and the screen distance is stored
 *	at *doublePtr; otherwise TCL_ERROR is returned and an error message is
 *	left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetPostscriptPoints(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    char *string,		/* String describing a screen distance. */
    double *doublePtr)		/* Place to store converted result. */
{
    char *end;
    double d;

    d = strtod(string, &end);
    if (end == string) {
	goto error;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
	end++;
    }
    switch (*end) {
    case 'c':
	d *= 72.0/2.54;
	end++;
	break;
    case 'i':
	d *= 72.0;
	end++;
	break;
    case 'm':
	d *= 72.0/25.4;
	end++;
	break;
    case 0:
	break;
    case 'p':
	end++;
	break;
    default:
	goto error;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
	end++;
    }
    if (*end != 0) {
	goto error;
    }
    *doublePtr = d;
    return TCL_OK;

  error:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad distance \"%s\"", string));
    Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "POINTS", NULL);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * TkImageGetColor --
 *
 *	This function converts a pixel value to three floating point numbers,
 *	representing the amount of red, green, and blue in that pixel on the
 *	screen. It makes use of colormap data passed as an argument, and
 *	should work for all Visual types.
 *
 *	This implementation is bogus on Windows because the colormap data is
 *	never filled in. Instead all postscript generated data coming through
 *	here is expected to be RGB color data. To handle lower bit-depth
 *	images properly, XQueryColors must be implemented for Windows.
 *
 * Results:
 *	Returns red, green, and blue color values in the range 0 to 1. There
 *	are no error returns.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

#ifdef _WIN32
#include <windows.h>

/*
 * We could just define these instead of pulling in windows.h.
 #define GetRValue(rgb)	((BYTE)(rgb))
 #define GetGValue(rgb)	((BYTE)(((WORD)(rgb)) >> 8))
 #define GetBValue(rgb)	((BYTE)((rgb)>>16))
 */

#else /* !_WIN32 */

#define GetRValue(rgb)	((rgb & cdata->red_mask) >> cdata->red_shift)
#define GetGValue(rgb)	((rgb & cdata->green_mask) >> cdata->green_shift)
#define GetBValue(rgb)	((rgb & cdata->blue_mask) >> cdata->blue_shift)

#endif /* _WIN32 */

#if defined(_WIN32) || defined(MAC_OSX_TK)
static void
TkImageGetColor(
    TkColormapData *cdata,	/* Colormap data */
    unsigned long pixel,	/* Pixel value to look up */
    double *red, double *green, double *blue)
				/* Color data to return */
{
    *red   = (double) GetRValue(pixel) / 255.0;
    *green = (double) GetGValue(pixel) / 255.0;
    *blue  = (double) GetBValue(pixel) / 255.0;
}
#else /* ! (_WIN32 || MAC_OSX_TK) */
static void
TkImageGetColor(
    TkColormapData *cdata,	/* Colormap data */
    unsigned long pixel,	/* Pixel value to look up */
    double *red, double *green, double *blue)
				/* Color data to return */
{
    if (cdata->separated) {
	int r = GetRValue(pixel);
	int g = GetGValue(pixel);
	int b = GetBValue(pixel);

	*red   = cdata->colors[r].red / 65535.0;
	*green = cdata->colors[g].green / 65535.0;
	*blue  = cdata->colors[b].blue / 65535.0;
    } else {
	*red   = cdata->colors[pixel].red / 65535.0;
	*green = cdata->colors[pixel].green / 65535.0;
	*blue  = cdata->colors[pixel].blue / 65535.0;
    }
}
#endif /* _WIN32 || MAC_OSX_TK */

/*
 *--------------------------------------------------------------
 *
 * TkPostscriptImage --
 *
 *	This function is called to output the contents of an image in
 *	Postscript, using a format appropriate for the current color mode
 *	(i.e. one bit per pixel in monochrome, one byte per pixel in gray, and
 *	three bytes per pixel in color).
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in interp->result. If no error occurs, then
 *	additional Postscript will be appended to interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkPostscriptImage(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tk_PostscriptInfo psInfo,	/* postscript info */
    XImage *ximage,		/* Image to draw */
    int x, int y,		/* First pixel to output */
    int width, int height)	/* Width and height of area */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    int xx, yy, band, maxRows;
    double red, green, blue;
    int bytesPerLine = 0, maxWidth = 0;
    int level = psInfoPtr->colorLevel;
    Colormap cmap;
    int i, ncolors;
    Visual *visual;
    TkColormapData cdata;
    Tcl_Obj *psObj;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    cmap = Tk_Colormap(tkwin);
    visual = Tk_Visual(tkwin);

    /*
     * Obtain information about the colormap, ie the mapping between pixel
     * values and RGB values. The code below should work for all Visual types.
     */

    ncolors = visual->map_entries;
    cdata.colors = ckalloc(sizeof(XColor) * ncolors);
    cdata.ncolors = ncolors;

    if (visual->c_class == DirectColor || visual->c_class == TrueColor) {
	cdata.separated = 1;
	cdata.red_mask = visual->red_mask;
	cdata.green_mask = visual->green_mask;
	cdata.blue_mask = visual->blue_mask;
	cdata.red_shift = 0;
	cdata.green_shift = 0;
	cdata.blue_shift = 0;

	while ((0x0001 & (cdata.red_mask >> cdata.red_shift)) == 0) {
	    cdata.red_shift ++;
	}
	while ((0x0001 & (cdata.green_mask >> cdata.green_shift)) == 0) {
	    cdata.green_shift ++;
	}
	while ((0x0001 & (cdata.blue_mask >> cdata.blue_shift)) == 0) {
	    cdata.blue_shift ++;
	}

	for (i = 0; i < ncolors; i ++) {
	    cdata.colors[i].pixel =
		    ((i << cdata.red_shift) & cdata.red_mask) |
		    ((i << cdata.green_shift) & cdata.green_mask) |
		    ((i << cdata.blue_shift) & cdata.blue_mask);
	}
    } else {
	cdata.separated=0;
	for (i = 0; i < ncolors; i ++) {
	    cdata.colors[i].pixel = i;
	}
    }

    if (visual->c_class == StaticGray || visual->c_class == GrayScale) {
	cdata.color = 0;
    } else {
	cdata.color = 1;
    }

    XQueryColors(Tk_Display(tkwin), cmap, cdata.colors, ncolors);

    /*
     * Figure out which color level to use (possibly lower than the one
     * specified by the user). For example, if the user specifies color with
     * monochrome screen, use gray or monochrome mode instead.
     */

    if (!cdata.color && level >= 2) {
	level = 1;
    }

    if (!cdata.color && cdata.ncolors == 2) {
	level = 0;
    }

    /*
     * Check that at least one row of the image can be represented with a
     * string less than 64 KB long (this is a limit in the Postscript
     * interpreter).
     */

    switch (level) {
    case 0: bytesPerLine = (width + 7) / 8;  maxWidth = 240000; break;
    case 1: bytesPerLine = width;	     maxWidth = 60000;  break;
    default: bytesPerLine = 3 * width;	     maxWidth = 20000;  break;
    }

    if (bytesPerLine > 60000) {
	Tcl_ResetResult(interp);
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't generate Postscript for images more than %d pixels wide",
		maxWidth));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "MEMLIMIT", NULL);
	ckfree(cdata.colors);
	return TCL_ERROR;
    }

    maxRows = 60000 / bytesPerLine;
    psObj = GetPostscriptBuffer(interp);

    for (band = height-1; band >= 0; band -= maxRows) {
	int rows = (band >= maxRows) ? maxRows : band + 1;
	int lineLen = 0;

	switch (level) {
	case 0:
	    Tcl_AppendPrintfToObj(psObj, "%d %d 1 matrix {\n<", width, rows);
	    break;
	case 1:
	    Tcl_AppendPrintfToObj(psObj, "%d %d 8 matrix {\n<", width, rows);
	    break;
	default:
	    Tcl_AppendPrintfToObj(psObj, "%d %d 8 matrix {\n<", width, rows);
	    break;
	}
	for (yy = band; yy > band - rows; yy--) {
	    switch (level) {
	    case 0: {
		/*
		 * Generate data for image in monochrome mode. No attempt at
		 * dithering is made--instead, just set a threshold.
		 */

		unsigned char mask = 0x80;
		unsigned char data = 0x00;

		for (xx = x; xx< x+width; xx++) {
		    TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
			    &red, &green, &blue);
		    if (0.30 * red + 0.59 * green + 0.11 * blue > 0.5) {
			data |= mask;
		    }
		    mask >>= 1;
		    if (mask == 0) {
			Tcl_AppendPrintfToObj(psObj, "%02X", data);
			lineLen += 2;
			if (lineLen > 60) {
			    lineLen = 0;
			    Tcl_AppendToObj(psObj, "\n", -1);
			}
			mask = 0x80;
			data = 0x00;
		    }
		}
		if ((width % 8) != 0) {
		    Tcl_AppendPrintfToObj(psObj, "%02X", data);
		    mask = 0x80;
		    data = 0x00;
		}
		break;
	    }
	    case 1:
		/*
		 * Generate data in gray mode; in this case, take a weighted
		 * sum of the red, green, and blue values.
		 */

		for (xx = x; xx < x+width; xx ++) {
		    TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
			    &red, &green, &blue);
		    Tcl_AppendPrintfToObj(psObj, "%02X",
			    (int) floor(0.5 + 255.0 *
			    (0.30 * red + 0.59 * green + 0.11 * blue)));
		    lineLen += 2;
		    if (lineLen > 60) {
			lineLen = 0;
			Tcl_AppendToObj(psObj, "\n", -1);
		    }
		}
		break;
	    default:
		/*
		 * Finally, color mode. Here, just output the red, green, and
		 * blue values directly.
		 */

		for (xx = x; xx < x+width; xx++) {
		    TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
			    &red, &green, &blue);
		    Tcl_AppendPrintfToObj(psObj, "%02X%02X%02X",
			    (int) floor(0.5 + 255.0 * red),
			    (int) floor(0.5 + 255.0 * green),
			    (int) floor(0.5 + 255.0 * blue));
		    lineLen += 6;
		    if (lineLen > 60) {
			lineLen = 0;
			Tcl_AppendToObj(psObj, "\n", -1);
		    }
		}
		break;
	    }
	}
	switch (level) {
	case 0: case 1:
	    Tcl_AppendToObj(psObj, ">\n} image\n", -1); break;
	default:
	    Tcl_AppendToObj(psObj, ">\n} false 3 colorimage\n", -1); break;
	}
	Tcl_AppendPrintfToObj(psObj, "0 %d translate\n", rows);
    }
    ckfree(cdata.colors);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PostscriptPhoto --
 *
 *	This function is called to output the contents of a photo image in
 *	Postscript, using a format appropriate for the requested postscript
 *	color mode (i.e. one byte per pixel in gray, and three bytes per pixel
 *	in color).
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in interp->result. If no error occurs, then
 *	additional Postscript will be appended to the interpreter's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PostscriptPhoto(
    Tcl_Interp *interp,
    Tk_PhotoImageBlock *blockPtr,
    Tk_PostscriptInfo psInfo,
    int width, int height)
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    int colorLevel = psInfoPtr->colorLevel;
    const char *displayOperation, *decode;
    unsigned char *pixelPtr;
    int bpc, xx, yy, lineLen, alpha;
    float red, green, blue;
    int bytesPerLine = 0, maxWidth = 0;
    unsigned char opaque = 255;
    unsigned char *alphaPtr;
    int alphaOffset, alphaPitch, alphaIncr;
    Tcl_Obj *psObj;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    if (colorLevel != 0) {
	/*
	 * Color and gray-scale code.
	 */

	displayOperation = "TkPhotoColor";
    } else {
	/*
	 * Monochrome-only code
	 */

	displayOperation = "TkPhotoMono";
    }

    /*
     * Check that at least one row of the image can be represented with a
     * string less than 64 KB long (this is a limit in the Postscript
     * interpreter).
     */

    switch (colorLevel) {
    case 0: bytesPerLine = (width + 7) / 8; maxWidth = 240000; break;
    case 1: bytesPerLine = width;	    maxWidth = 60000;  break;
    default: bytesPerLine = 3 * width;	    maxWidth = 20000;  break;
    }
    if (bytesPerLine > 60000) {
	Tcl_ResetResult(interp);
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't generate Postscript for images more than %d pixels wide",
		maxWidth));
	Tcl_SetErrorCode(interp, "TK", "CANVAS", "PS", "MEMLIMIT", NULL);
	return TCL_ERROR;
    }

    /*
     * Set up the postscript code except for the image-data stream.
     */

    psObj = GetPostscriptBuffer(interp);
    switch (colorLevel) {
    case 0:
	Tcl_AppendToObj(psObj, "/DeviceGray setcolorspace\n\n", -1);
	decode = "1 0";
	bpc = 1;
	break;
    case 1:
	Tcl_AppendToObj(psObj, "/DeviceGray setcolorspace\n\n", -1);
	decode = "0 1";
	bpc = 8;
	break;
    default:
	Tcl_AppendToObj(psObj, "/DeviceRGB setcolorspace\n\n", -1);
	decode = "0 1 0 1 0 1";
	bpc = 8;
	break;
    }

    Tcl_AppendPrintfToObj(psObj,
	    "<<\n  /ImageType 1\n"
	    "  /Width %d\n  /Height %d\n  /BitsPerComponent %d\n"
	    "  /DataSource currentfile\n  /ASCIIHexDecode filter\n"
	    "  /ImageMatrix [1 0 0 -1 0 %d]\n  /Decode [%s]\n>>\n"
	    "1 %s\n",
	    width, height, bpc, height, decode, displayOperation);

    /*
     * Check the PhotoImageBlock information. We assume that:
     *     if pixelSize is 1,2 or 4, the image is R,G,B,A;
     *     if pixelSize is 3, the image is R,G,B and offset[3] is bogus.
     */

    if (blockPtr->pixelSize == 3) {
	/*
	 * No alpha information: the whole image is opaque.
	 */

	alphaPtr = &opaque;
	alphaPitch = alphaIncr = alphaOffset = 0;
    } else {
	/*
	 * Set up alpha handling.
	 */

	alphaPtr = blockPtr->pixelPtr;
	alphaPitch = blockPtr->pitch;
	alphaIncr = blockPtr->pixelSize;
	alphaOffset = blockPtr->offset[3];
    }

    for (yy = 0, lineLen=0; yy < height; yy++) {
	switch (colorLevel) {
	case 0: {
	    /*
	     * Generate data for image in monochrome mode. No attempt at
	     * dithering is made--instead, just set a threshold. To handle
	     * transparecies we need to output two lines: one for the black
	     * pixels, one for the white ones.
	     */

	    unsigned char mask = 0x80;
	    unsigned char data = 0x00;

	    for (xx = 0; xx< width; xx ++) {
		pixelPtr = blockPtr->pixelPtr + (yy * blockPtr->pitch)
			+ (xx *blockPtr->pixelSize);

		red = pixelPtr[blockPtr->offset[0]];
		green = pixelPtr[blockPtr->offset[1]];
		blue = pixelPtr[blockPtr->offset[2]];

		alpha = *(alphaPtr + (yy * alphaPitch)
			+ (xx * alphaIncr) + alphaOffset);

		/*
		 * If pixel is less than threshold, then it is black.
		 */

		if ((alpha != 0) &&
			(0.3086*red + 0.6094*green + 0.082*blue < 128)) {
		    data |= mask;
		}
		mask >>= 1;
		if (mask == 0) {
		    Tcl_AppendPrintfToObj(psObj, "%02X", data);
		    lineLen += 2;
		    if (lineLen >= 60) {
			lineLen = 0;
			Tcl_AppendToObj(psObj, "\n", -1);
		    }
		    mask = 0x80;
		    data = 0x00;
		}
	    }
	    if ((width % 8) != 0) {
		Tcl_AppendPrintfToObj(psObj, "%02X", data);
		mask = 0x80;
		data = 0x00;
	    }

	    mask = 0x80;
	    data = 0x00;
	    for (xx=0 ; xx<width ; xx++) {
		pixelPtr = blockPtr->pixelPtr + (yy * blockPtr->pitch)
			+ (xx *blockPtr->pixelSize);

		red = pixelPtr[blockPtr->offset[0]];
		green = pixelPtr[blockPtr->offset[1]];
		blue = pixelPtr[blockPtr->offset[2]];

		alpha = *(alphaPtr + (yy * alphaPitch)
			+ (xx * alphaIncr) + alphaOffset);

		/*
		 * If pixel is greater than threshold, then it is white.
		 */

		if ((alpha != 0) &&
			(0.3086*red + 0.6094*green + 0.082*blue >= 128)) {
		    data |= mask;
		}
		mask >>= 1;
		if (mask == 0) {
		    Tcl_AppendPrintfToObj(psObj, "%02X", data);
		    lineLen += 2;
		    if (lineLen >= 60) {
			lineLen = 0;
			Tcl_AppendToObj(psObj, "\n", -1);
		    }
		    mask = 0x80;
		    data = 0x00;
		}
	    }
	    if ((width % 8) != 0) {
		Tcl_AppendPrintfToObj(psObj, "%02X", data);
		mask = 0x80;
		data = 0x00;
	    }
	    break;
	}
	case 1: {
	    /*
	     * Generate transparency data. We must prevent a transparent value
	     * of 0 because of a bug in some HP printers.
	     */

	    for (xx = 0; xx < width; xx ++) {
		alpha = *(alphaPtr + (yy * alphaPitch)
			+ (xx * alphaIncr) + alphaOffset);
		Tcl_AppendPrintfToObj(psObj, "%02X", alpha | 0x01);
		lineLen += 2;
		if (lineLen >= 60) {
		    lineLen = 0;
		    Tcl_AppendToObj(psObj, "\n", -1);
		}
	    }

	    /*
	     * Generate data in gray mode; in this case, take a weighted sum
	     * of the red, green, and blue values.
	     */

	    for (xx = 0; xx < width; xx ++) {
		pixelPtr = blockPtr->pixelPtr + (yy * blockPtr->pitch)
			+ (xx *blockPtr->pixelSize);

		red = pixelPtr[blockPtr->offset[0]];
		green = pixelPtr[blockPtr->offset[1]];
		blue = pixelPtr[blockPtr->offset[2]];

		Tcl_AppendPrintfToObj(psObj, "%02X", (int) floor(0.5 +
			( 0.3086 * red + 0.6094 * green + 0.0820 * blue)));
		lineLen += 2;
		if (lineLen >= 60) {
		    lineLen = 0;
		    Tcl_AppendToObj(psObj, "\n", -1);
		}
	    }
	    break;
	}
	default:
	    /*
	     * Generate transparency data. We must prevent a transparent value
	     * of 0 because of a bug in some HP printers.
	     */

	    for (xx = 0; xx < width; xx ++) {
		alpha = *(alphaPtr + (yy * alphaPitch)
			+ (xx * alphaIncr) + alphaOffset);
		Tcl_AppendPrintfToObj(psObj, "%02X", alpha | 0x01);
		lineLen += 2;
		if (lineLen >= 60) {
		    lineLen = 0;
		    Tcl_AppendToObj(psObj, "\n", -1);
		}
	    }

	    /*
	     * Finally, color mode. Here, just output the red, green, and blue
	     * values directly.
	     */

	    for (xx = 0; xx < width; xx ++) {
		pixelPtr = blockPtr->pixelPtr + (yy * blockPtr->pitch)
			+ (xx * blockPtr->pixelSize);

		Tcl_AppendPrintfToObj(psObj, "%02X%02X%02X",
			pixelPtr[blockPtr->offset[0]],
			pixelPtr[blockPtr->offset[1]],
			pixelPtr[blockPtr->offset[2]]);
		lineLen += 6;
		if (lineLen >= 60) {
		    lineLen = 0;
		    Tcl_AppendToObj(psObj, "\n", -1);
		}
	    }
	    break;
	}
    }

    /*
     * The end-of-data marker.
     */

    Tcl_AppendToObj(psObj, ">\n", -1);
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
