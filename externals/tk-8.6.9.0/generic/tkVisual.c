/*
 * tkVisual.c --
 *
 *	This file contains library procedures for allocating and freeing
 *	visuals and colormaps. This code is based on a prototype
 *	implementation by Paul Mackerras.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * The table below maps from symbolic names for visual classes to the
 * associated X class symbols.
 */

typedef struct VisualDictionary {
    const char *name;		/* Textual name of class. */
    int minLength;		/* Minimum # characters that must be specified
				 * for an unambiguous match. */
    int class;			/* X symbol for class. */
} VisualDictionary;
static const VisualDictionary visualNames[] = {
    {"best",		1,	0},
    {"directcolor",	2,	DirectColor},
    {"grayscale",	1,	GrayScale},
    {"greyscale",	1,	GrayScale},
    {"pseudocolor",	1,	PseudoColor},
    {"staticcolor",	7,	StaticColor},
    {"staticgray",	7,	StaticGray},
    {"staticgrey",	7,	StaticGray},
    {"truecolor",	1,	TrueColor},
    {NULL,		0,	0},
};

/*
 * One of the following structures exists for each distinct non-default
 * colormap allocated for a display by Tk_GetColormap.
 */

struct TkColormap {
    Colormap colormap;		/* X's identifier for the colormap. */
    Visual *visual;		/* Visual for which colormap was allocated. */
    int refCount;		/* How many uses of the colormap are still
				 * outstanding (calls to Tk_GetColormap minus
				 * calls to Tk_FreeColormap). */
    int shareable;		/* 0 means this colormap was allocated by a
				 * call to Tk_GetColormap with "new", implying
				 * that the window wants it all for itself.  1
				 * means that the colormap was allocated as a
				 * default for a particular visual, so it can
				 * be shared. */
    struct TkColormap *nextPtr;	/* Next in list of colormaps for this display,
				 * or NULL for end of list. */
};

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetVisual --
 *
 *	Given a string identifying a particular kind of visual, this procedure
 *	returns a visual and depth that matches the specification.
 *
 * Results:
 *	The return value is normally a pointer to a visual. If an error
 *	occurred in looking up the visual, NULL is returned and an error
 *	message is left in the interp's result. The depth of the visual is
 *	returned to *depthPtr under normal returns. If colormapPtr is
 *	non-NULL, then this procedure also finds a suitable colormap for use
 *	with the visual in tkwin, and it returns that colormap in *colormapPtr
 *	unless an error occurs.
 *
 * Side effects:
 *	A new colormap may be allocated.
 *
 *----------------------------------------------------------------------
 */

Visual *
Tk_GetVisual(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window in which visual will be used. */
    const char *string,		/* String describing visual. See manual entry
				 * for details. */
    int *depthPtr,		/* The depth of the returned visual is stored
				 * here. */
    Colormap *colormapPtr)	/* If non-NULL, then a suitable colormap for
				 * visual is placed here. This colormap must
				 * eventually be freed by calling
				 * Tk_FreeColormap. */
{
    Tk_Window tkwin2;
    XVisualInfo template, *visInfoList, *bestPtr;
    long mask;
    Visual *visual;
    ptrdiff_t length;
    int c, numVisuals, prio, bestPrio, i;
    const char *p;
    const VisualDictionary *dictPtr;
    TkColormap *cmapPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    /*
     * Parse string and set up a template for use in searching for an
     * appropriate visual.
     */

    c = string[0];
    if (c == '.') {
	/*
	 * The string must be a window name. If the window is on the same
	 * screen as tkwin, then just use its visual. Otherwise use the
	 * information about the visual as a template for the search.
	 */

	tkwin2 = Tk_NameToWindow(interp, string, tkwin);
	if (tkwin2 == NULL) {
	    return NULL;
	}
	visual = Tk_Visual(tkwin2);
	if (Tk_Screen(tkwin) == Tk_Screen(tkwin2)) {
	    *depthPtr = Tk_Depth(tkwin2);
	    if (colormapPtr != NULL) {
		/*
		 * Use the colormap from the other window too (but be sure to
		 * increment its reference count if it's one of the ones
		 * allocated here).
		 */

		*colormapPtr = Tk_Colormap(tkwin2);
		for (cmapPtr = dispPtr->cmapPtr; cmapPtr != NULL;
			cmapPtr = cmapPtr->nextPtr) {
		    if (cmapPtr->colormap == *colormapPtr) {
			cmapPtr->refCount += 1;
			break;
		    }
		}
	    }
	    return visual;
	}
	template.depth = Tk_Depth(tkwin2);
	template.class = visual->class;
	template.red_mask = visual->red_mask;
	template.green_mask = visual->green_mask;
	template.blue_mask = visual->blue_mask;
	template.colormap_size = visual->map_entries;
	template.bits_per_rgb = visual->bits_per_rgb;
	mask = VisualDepthMask|VisualClassMask|VisualRedMaskMask
		|VisualGreenMaskMask|VisualBlueMaskMask|VisualColormapSizeMask
		|VisualBitsPerRGBMask;
    } else if ((c == 0) || ((c == 'd') && (string[1] != 0)
	    && (strncmp(string, "default", strlen(string)) == 0))) {
	/*
	 * Use the default visual for the window's screen.
	 */

	if (colormapPtr != NULL) {
	    *colormapPtr = DefaultColormapOfScreen(Tk_Screen(tkwin));
	}
	*depthPtr = DefaultDepthOfScreen(Tk_Screen(tkwin));
	return DefaultVisualOfScreen(Tk_Screen(tkwin));
    } else if (isdigit(UCHAR(c))) {
	int visualId;

	/*
	 * This is a visual ID.
	 */

	if (Tcl_GetInt(interp, string, &visualId) == TCL_ERROR) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad X identifier for visual: \"%s\"", string));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "VISUALID", NULL);
	    return NULL;
	}
	template.visualid = visualId;
	mask = VisualIDMask;
    } else {
	/*
	 * Parse the string into a class name (or "best") optionally followed
	 * by whitespace and a depth.
	 */

	for (p = string; *p != 0; p++) {
	    if (isspace(UCHAR(*p)) || isdigit(UCHAR(*p))) {
		break;
	    }
	}
	length = p - string;
	template.class = -1;
	for (dictPtr = visualNames; dictPtr->name != NULL; dictPtr++) {
	    if ((dictPtr->name[0] == c) && (length >= dictPtr->minLength)
		    && (strncmp(string, dictPtr->name,
		    (size_t) length) == 0)) {
		template.class = dictPtr->class;
		break;
	    }
	}
	if (template.class == -1) {
	    Tcl_Obj *msgObj = Tcl_ObjPrintf(
		    "unknown or ambiguous visual name \"%s\": class must be ",
		    string);

	    for (dictPtr = visualNames; dictPtr->name != NULL; dictPtr++) {
		Tcl_AppendPrintfToObj(msgObj, "%s, ", dictPtr->name);
	    }
	    Tcl_AppendToObj(msgObj, "or default", -1);
	    Tcl_SetObjResult(interp, msgObj);
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "VISUAL", string, NULL);
	    return NULL;
	}
	while (isspace(UCHAR(*p))) {
	    p++;
	}
	if (*p == 0) {
	    template.depth = 10000;
	} else if (Tcl_GetInt(interp, p, &template.depth) != TCL_OK) {
	    return NULL;
	}
	if (c == 'b') {
	    mask = 0;
	} else {
	    mask = VisualClassMask;
	}
    }

    /*
     * Find all visuals that match the template we've just created, and return
     * an error if there are none that match.
     */

    template.screen = Tk_ScreenNumber(tkwin);
    mask |= VisualScreenMask;
    visInfoList = XGetVisualInfo(Tk_Display(tkwin), mask, &template,
	    &numVisuals);
    if (visInfoList == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"couldn't find an appropriate visual", -1));
	Tcl_SetErrorCode(interp, "TK", "VISUAL", "INAPPROPRIATE", NULL);
	return NULL;
    }

    /*
     * Search through the visuals that were returned to find the best one.
     * The choice is based on the following criteria, in decreasing order of
     * importance:
     *
     * 1. Depth: choose a visual with exactly the desired depth, else one with
     *	  more bits than requested but as few bits as possible, else one with
     *	  fewer bits but as many as possible.
     * 2. Class: some visual classes are more desirable than others; pick the
     *    visual with the most desirable class.
     * 3. Default: the default visual for the screen gets preference over
     *    other visuals, all else being equal.
     */

    bestPrio = 0;
    bestPtr = NULL;
    for (i = 0; i < numVisuals; i++) {
	switch (visInfoList[i].class) {
	case DirectColor:
	    prio = 5; break;
	case GrayScale:
	    prio = 1; break;
	case PseudoColor:
	    prio = 7; break;
	case StaticColor:
	    prio = 3; break;
	case StaticGray:
	    prio = 1; break;
	case TrueColor:
	    prio = 5; break;
	default:
	    prio = 0; break;
	}
	if (visInfoList[i].visual
		== DefaultVisualOfScreen(Tk_Screen(tkwin))) {
	    prio++;
	}
	if (bestPtr == NULL) {
	    goto newBest;
	}
	if (visInfoList[i].depth < bestPtr->depth) {
	    if (visInfoList[i].depth >= template.depth) {
		goto newBest;
	    }
	} else if (visInfoList[i].depth > bestPtr->depth) {
	    if (bestPtr->depth < template.depth) {
		goto newBest;
	    }
	} else {
	    if (prio > bestPrio) {
		goto newBest;
	    }
	}
	continue;

    newBest:
	bestPtr = &visInfoList[i];
	bestPrio = prio;
    }
    CLANG_ASSERT(bestPtr);
    *depthPtr = bestPtr->depth;
    visual = bestPtr->visual;
    XFree((char *) visInfoList);

    /*
     * If we need to find a colormap for this visual, do it now. If the visual
     * is the default visual for the screen, then use the default colormap.
     * Otherwise search for an existing colormap that's shareable. If all else
     * fails, create a new colormap.
     */

    if (colormapPtr != NULL) {
	if (visual == DefaultVisualOfScreen(Tk_Screen(tkwin))) {
	    *colormapPtr = DefaultColormapOfScreen(Tk_Screen(tkwin));
	} else {
	    for (cmapPtr = dispPtr->cmapPtr; cmapPtr != NULL;
		    cmapPtr = cmapPtr->nextPtr) {
		if (cmapPtr->shareable && (cmapPtr->visual == visual)) {
		    *colormapPtr = cmapPtr->colormap;
		    cmapPtr->refCount += 1;
		    goto done;
		}
	    }
	    cmapPtr = ckalloc(sizeof(TkColormap));
	    cmapPtr->colormap = XCreateColormap(Tk_Display(tkwin),
		    RootWindowOfScreen(Tk_Screen(tkwin)), visual,
		    AllocNone);
	    cmapPtr->visual = visual;
	    cmapPtr->refCount = 1;
	    cmapPtr->shareable = 1;
	    cmapPtr->nextPtr = dispPtr->cmapPtr;
	    dispPtr->cmapPtr = cmapPtr;
	    *colormapPtr = cmapPtr->colormap;
	}
    }

  done:
    return visual;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetColormap --
 *
 *	Given a string identifying a colormap, this procedure finds an
 *	appropriate colormap.
 *
 * Results:
 *	The return value is normally the X resource identifier for the
 *	colormap. If an error occurs, None is returned and an error message is
 *	placed in the interp's result.
 *
 * Side effects:
 *	A reference count is incremented for the colormap, so Tk_FreeColormap
 *	must eventually be called exactly once for each call to
 *	Tk_GetColormap.
 *
 *----------------------------------------------------------------------
 */

Colormap
Tk_GetColormap(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window where colormap will be used. */
    const char *string)		/* String that identifies colormap: either
				 * "new" or the name of another window. */
{
    Colormap colormap;
    TkColormap *cmapPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    Tk_Window other;

    /*
     * Allocate a new colormap, if that's what is wanted.
     */

    if (strcmp(string, "new") == 0) {
	cmapPtr = ckalloc(sizeof(TkColormap));
	cmapPtr->colormap = XCreateColormap(Tk_Display(tkwin),
		RootWindowOfScreen(Tk_Screen(tkwin)), Tk_Visual(tkwin),
		AllocNone);
	cmapPtr->visual = Tk_Visual(tkwin);
	cmapPtr->refCount = 1;
	cmapPtr->shareable = 0;
	cmapPtr->nextPtr = dispPtr->cmapPtr;
	dispPtr->cmapPtr = cmapPtr;
	return cmapPtr->colormap;
    }

    /*
     * Use a colormap from an existing window. It must have the same visual as
     * tkwin (which means, among other things, that the other window must be
     * on the same screen).
     */

    other = Tk_NameToWindow(interp, string, tkwin);
    if (other == NULL) {
	return None;
    }
    if (Tk_Screen(other) != Tk_Screen(tkwin)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't use colormap for %s: not on same screen", string));
	Tcl_SetErrorCode(interp, "TK", "COLORMAP", "SCREEN", NULL);
	return None;
    }
    if (Tk_Visual(other) != Tk_Visual(tkwin)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't use colormap for %s: incompatible visuals", string));
	Tcl_SetErrorCode(interp, "TK", "COLORMAP", "INCOMPATIBLE", NULL);
	return None;
    }
    colormap = Tk_Colormap(other);

    /*
     * If the colormap was a special one allocated by code in this file,
     * increment its reference count.
     */

    for (cmapPtr = dispPtr->cmapPtr; cmapPtr != NULL;
	    cmapPtr = cmapPtr->nextPtr) {
	if (cmapPtr->colormap == colormap) {
	    cmapPtr->refCount += 1;
	}
    }
    return colormap;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeColormap --
 *
 *	This procedure is called to release a colormap that was previously
 *	allocated by Tk_GetColormap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The colormap's reference count is decremented. If this was the last
 *	reference to the colormap, then the colormap is freed.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeColormap(
    Display *display,		/* Display for which colormap was
				 * allocated. */
    Colormap colormap)		/* Colormap that is no longer needed. Must
				 * have been returned by previous call to
				 * Tk_GetColormap, or preserved by a previous
				 * call to Tk_PreserveColormap. */
{
    TkDisplay *dispPtr;
    TkColormap *cmapPtr, *prevPtr;

    /*
     * Find Tk's information about the display, then see if this colormap is a
     * non-default one (if it's a default one, there won't be an entry for it
     * in the display's list).
     */

    dispPtr = TkGetDisplay(display);
    if (dispPtr == NULL) {
	Tcl_Panic("unknown display passed to Tk_FreeColormap");
    }
    for (prevPtr = NULL, cmapPtr = dispPtr->cmapPtr; cmapPtr != NULL;
	    prevPtr = cmapPtr, cmapPtr = cmapPtr->nextPtr) {
	if (cmapPtr->colormap == colormap) {
	    cmapPtr->refCount -= 1;
	    if (cmapPtr->refCount == 0) {
		XFreeColormap(display, colormap);
		if (prevPtr == NULL) {
		    dispPtr->cmapPtr = cmapPtr->nextPtr;
		} else {
		    prevPtr->nextPtr = cmapPtr->nextPtr;
		}
		ckfree(cmapPtr);
	    }
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PreserveColormap --
 *
 *	This procedure is called to indicate to Tk that the specified colormap
 *	is being referenced from another location and should not be freed
 *	until all extra references are eliminated. The colormap must have been
 *	returned by Tk_GetColormap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The colormap's reference count is incremented, so Tk_FreeColormap must
 *	eventually be called exactly once for each call to
 *	Tk_PreserveColormap.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PreserveColormap(
    Display *display,		/* Display for which colormap was
				 * allocated. */
    Colormap colormap)		/* Colormap that should be preserved. */
{
    TkDisplay *dispPtr;
    TkColormap *cmapPtr;

    /*
     * Find Tk's information about the display, then see if this colormap is a
     * non-default one (if it's a default one, there won't be an entry for it
     * in the display's list).
     */

    dispPtr = TkGetDisplay(display);
    if (dispPtr == NULL) {
	Tcl_Panic("unknown display passed to Tk_PreserveColormap");
    }
    for (cmapPtr = dispPtr->cmapPtr; cmapPtr != NULL;
	    cmapPtr = cmapPtr->nextPtr) {
	if (cmapPtr->colormap == colormap) {
	    cmapPtr->refCount += 1;
	    return;
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
