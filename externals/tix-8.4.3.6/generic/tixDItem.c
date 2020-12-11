/*
 * tixDItem.c --
 *
 *	This file implements the "Display Items" in the Tix library.
 *
 *	Since many Tix widgets use the same type of display items, for
 *	example, text items, image items, or text-image items (used in
 *	HList, TList and Table), it makes sense to provide a set of
 *	common routines to support these display items. Code re-use is
 *	the major issue: we don't want to re-define almost the same
 *	configSpecs again and again in different widgets. Therefore,
 *	all display items provide common methods to configure,
 *	display, calculate geometry, etc.
 *
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixDItem.c,v 1.3 2004/03/28 02:44:56 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>


static int   DItemParseProc _ANSI_ARGS_((ClientData clientData,
		Tcl_Interp *interp, Tk_Window tkwin, CONST84 char *value,
		char *widRec, int offset));

static char *DItemPrintProc _ANSI_ARGS_((
		ClientData clientData, Tk_Window tkwin, char *widRec,
		int offset, Tcl_FreeProc **freeProcPtr));

/*----------------------------------------------------------------------
 *
 *
 *			   PUBLIC INTERFACE
 *
 *
 * The following functions are called by widget implementors
 *
 *----------------------------------------------------------------------
 */
/* Tix_AddDItemType, Tix_GetDItemType --
 *
 *
 *	Maintain a list of item types, each identifies by a unique string
 *	name;
 */

static Tix_DItemInfo * diTypes = NULL;

void Tix_AddDItemType(diTypePtr)
    Tix_DItemInfo * diTypePtr;
{
    diTypePtr->next =  diTypes;
    diTypes = diTypePtr;
}

Tix_DItemInfo * Tix_GetDItemType(interp, type)
    Tcl_Interp * interp;
    CONST84 char * type;
{
    Tix_DItemInfo * diTypePtr;

    for (diTypePtr = diTypes; diTypePtr; diTypePtr = diTypePtr->next) {
	if (strcmp(type, diTypePtr->name)==0) {
	    return diTypePtr;
	}
    }

    if (interp) {
	Tcl_AppendResult(interp, "unknown display type \"", type, "\"", NULL);
    }
    return NULL;
}


/*----------------------------------------------------------------------
 *  Tix_DItemCreate --
 *
 *	Create a display item according to the "type" string.
 *
 *----------------------------------------------------------------------
 */
Tix_DItem * Tix_DItemCreate(ddPtr, type)
   Tix_DispData * ddPtr;
   CONST84 char * type;
{
    Tix_DItemInfo * diTypePtr;

    if ((diTypePtr = Tix_GetDItemType(ddPtr->interp, type)) == NULL) {
	return NULL;
    }

    return diTypePtr->createProc(ddPtr, diTypePtr);
}

/*----------------------------------------------------------------------
 *  Tix_DItemConfigure --
 *
 *	Configures a display item.
 *
 *----------------------------------------------------------------------
 */
int Tix_DItemConfigure(iPtr, argc, argv, flags)
    Tix_DItem * iPtr;
    int argc;
    CONST84 char ** argv;
    int flags;
{
    return iPtr->base.diTypePtr->configureProc(iPtr, argc, argv, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_DItemDisplay --
 *
 *	Display a DItem. {x, y, width, height} specifies a region
 *      for to display this item in. {xOffset, yOffset} gives the
 *      offset of the top-left corner of the item relative to
 *      the top-left corder of the region.
 *
 *      Background and foreground of the item are displayed according
 *      to the flags parameter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * 
 *----------------------------------------------------------------------
 */

void
Tix_DItemDisplay(drawable, iPtr, x, y, width, height, xOffset, yOffset,flags)
    Drawable drawable;
    Tix_DItem * iPtr;
    int x;
    int y;
    int width;
    int height;
    int xOffset;
    int yOffset;
    int flags;
{
    iPtr->base.diTypePtr->displayProc(drawable, iPtr, x, y,
            width, height, xOffset, yOffset, flags);
}

void Tix_DItemFree(iPtr)
    Tix_DItem * iPtr;
{
    iPtr->base.diTypePtr->freeProc(iPtr);

    /*
     * When it comes to here, iPtr is no longer a valid pointer!
     */
}

void Tix_DItemCalculateSize(iPtr)
    Tix_DItem * iPtr;
{
    iPtr->base.diTypePtr->calculateSizeProc(iPtr);
}

char * Tix_DItemComponent(iPtr, x, y)
    Tix_DItem * iPtr;
    int x;
    int y;
{
    return (iPtr->base.diTypePtr->componentProc(iPtr, x, y));
}


/*----------------------------------------------------------------------
 * Tix_FreeArgumentList --
 *
 *	Free the argument lists allocated by Tix_SplitConfig;
 *----------------------------------------------------------------------
 */
void
Tix_FreeArgumentList(argListPtr)
    Tix_ArgumentList *argListPtr;
{
    int i;

    for (i=0; i<argListPtr->numLists; i++) {
	ckfree((char*)argListPtr->arg[i].argv);
    }
    if (argListPtr->arg != argListPtr->preAlloc) {
	ckfree((char*)argListPtr->arg);
    }
}

/*----------------------------------------------------------------------
 * Tix_SplitConfig --
 *
 *	Split the command line arguments according for several configurable
 *	objects.
 *----------------------------------------------------------------------
 */
int
Tix_SplitConfig(interp, tkwin, specsList, numLists, argc, argv, argListPtr)
    Tcl_Interp * interp;
    Tk_Window tkwin;
    Tk_ConfigSpec  ** specsList;	/* a list of two or more config spec
					 * arrays */
    int numLists;
    int argc;
    CONST84 char ** argv;
    Tix_ArgumentList * argListPtr;
{
    Tix_Argument *arg;
    int i, n, code = TCL_OK;
    Tk_ConfigSpec  *specPtr;
    size_t len;
    int found;

    if (argc % 2) {
	Tcl_AppendResult(interp, "value for \"", argv[argc-1],
	    "\" missing", (char *) NULL);
	return TCL_ERROR;
    }

    if (numLists > FIXED_SIZE) {
	arg = (Tix_Argument*)ckalloc(numLists * sizeof(Tix_Argument));
    } else {
	arg = argListPtr->preAlloc;
    }
    argListPtr->arg = arg;
    argListPtr->numLists = numLists;
    for (i=0; i<numLists; i++) {
	arg[i].argc = 0;
	arg[i].argv = (CONST84 char**)ckalloc(argc * sizeof(char*));
    }

    /* Split the arguments for the appropriate objects */
    for (n=0; n<argc; n+=2) {
	len = strlen(argv[n]);
	found = 0;

	for (i=0; i<numLists; i++) {
	    for (specPtr=specsList[i];
		 specPtr->type != TK_CONFIG_END;
		 specPtr++) {

		if (specPtr->argvName == NULL) {
		    continue;
		}

		if (strncmp(argv[n], specPtr->argvName, len) == 0) {
		    arg[i].argv[arg[i].argc++] = argv[n  ];
		    arg[i].argv[arg[i].argc++] = argv[n+1];
		    found = 1;
		    break;
		}
	    }
	}
	if (found == 0) {
	    Tcl_AppendResult(interp, "unknown option \"", argv[n],
		    "\"", (char *) NULL);
	    code = TCL_ERROR;
	    goto done;
	}
    }

  done:
    if (code == TCL_ERROR) {
	Tix_FreeArgumentList(argListPtr);
    }
    return code;
}

int
Tix_MultiConfigureInfo(interp, tkwin, specsList, numLists, widgRecList,
	argvName, flags, request)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tk_Window tkwin;		/* Window corresponding to widgRec. */
    Tk_ConfigSpec **specsList;	/* Describes legal options. */
    int numLists;
    CONST84 char **widgRecList;	/* Record whose fields contain current
				 * values for options. */
    CONST84 char *argvName;	/* If non-NULL, indicates a single option
				 * whose info is to be returned.  Otherwise
				 * info is returned for all options. */
    int flags;			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
    int request;
{
    int i, found;
    Tk_ConfigSpec *specPtr;
    Tcl_DString dString;
    size_t len;
    CONST84 char *result;

    if (argvName != NULL) {
	len = strlen(argvName);
	for (found=0,i=0; i<numLists; i++) {
	    for (specPtr=specsList[i];
		 specPtr->type != TK_CONFIG_END;
		 specPtr++) {

		if (specPtr->argvName == NULL) {
		    continue;
		}

		if (strncmp(argvName, specPtr->argvName, len) == 0) {
		    found = 1;
		    goto done;
		}
	    }
	}
      done:
	if (!found) {
	    Tcl_AppendResult(interp, "unknown option \"", argvName,
		"\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (request == TIX_CONFIG_INFO) {
	    if (widgRecList[i] != NULL) {
		return Tk_ConfigureInfo(interp, tkwin, specsList[i],
			(char *) widgRecList[i], argvName, flags);
	    } else {
		return TCL_OK;
	    }
	} else {
	    if (widgRecList[i] != NULL) {
		return Tk_ConfigureValue(interp, tkwin, specsList[i],
			(char *) widgRecList[i], argvName, flags);
	    } else {
		return TCL_OK;
	    }
	}
    }

    Tcl_DStringInit(&dString);
    for (i=0; i<numLists; i++) {
	if (i != 0) {
	    Tcl_DStringAppend(&dString, " ", 1);
	}
	if (widgRecList[i] != NULL) {
	    Tk_ConfigureInfo(interp, tkwin, specsList[i],
		    (char *) widgRecList[i], NULL, flags);
	}
	result = Tcl_GetStringResult(interp);
	Tcl_DStringAppend(&dString, result, (int) strlen(result));
    }
    Tcl_ResetResult(interp);
    Tcl_DStringResult(interp, &dString);
    Tcl_DStringFree(&dString);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * Tix_ConfigureValue2 --
 *
 *
 *	Returns the config information of a entry element (of an HList,
 *	for example) and its display item.
 *----------------------------------------------------------------------
 */
int
Tix_ConfigureValue2(interp, tkwin, entRec, entConfigSpecs, iPtr,
	argvName, flags)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tk_Window tkwin;		/* Window corresponding to widgRec. */
    CONST84 char * entRec;
    Tk_ConfigSpec *entConfigSpecs; /* Describes legal options of the entry */
    Tix_DItem * iPtr;		/* points to the entry's data record */
    CONST84 char *argvName;	/* If non-NULL, indicates a single option
				 * whose info is to be returned.  Otherwise
				 * info is returned for all options. */
    int flags;			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    Tk_ConfigSpec *specsList[2];
    CONST84 char *widgRecList[2];

    specsList[0]   = entConfigSpecs;
    specsList[1]   = Tix_DItemConfigSpecs(iPtr);
    widgRecList[1] = (CONST84 char*)iPtr;
    widgRecList[0] = entRec;

    return Tix_MultiConfigureInfo(interp, tkwin, specsList, 2, widgRecList,
	argvName, flags, TIX_CONFIG_VALUE);
}

/*----------------------------------------------------------------------
 * Tix_ConfigureInfo2 --
 *
 *
 *	Returns the config information of a entry element (of an HList,
 *	for example) and its display item.
 *----------------------------------------------------------------------
 */
int
Tix_ConfigureInfo2(interp, tkwin, entRec, entConfigSpecs, iPtr,
	argvName, flags)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tk_Window tkwin;		/* Window corresponding to widgRec. */
    CONST84 char * entRec;
    Tk_ConfigSpec *entConfigSpecs; /* Describes legal options of the entry */
    Tix_DItem * iPtr;		/* points to the entry's data record */
    CONST84 char *argvName;	/* If non-NULL, indicates a single option
				 * whose info is to be returned.  Otherwise
				 * info is returned for all options. */
    int flags;			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    Tk_ConfigSpec *specsList[2];
    CONST84 char *widgRecList[2];

    specsList[0]   = entConfigSpecs;
    specsList[1]   = Tix_DItemConfigSpecs(iPtr);
    widgRecList[1] = (CONST84 char*)iPtr;
    widgRecList[0] = (CONST84 char*)entRec;

    return Tix_MultiConfigureInfo(interp, tkwin, specsList, 2, widgRecList,
	argvName, flags, TIX_CONFIG_INFO);
}

int
Tix_WidgetConfigure2(interp, tkwin, entRec, entConfigSpecs, iPtr,
		     argc, argv, flags, forced, sizeChanged_ret)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tk_Window tkwin;		/* Window corresponding to widgRec. */
    CONST84 char * entRec;
    Tk_ConfigSpec *entConfigSpecs; /* Describes legal options of the entry */
    Tix_DItem * iPtr;		/* points to the entry's data record */
    int argc;
    CONST84 char ** argv;
    int flags;
    int forced;			/* forced configure of DItem? */
    int * sizeChanged_ret;
{
    Tix_ArgumentList argList;
    Tk_ConfigSpec *specsList[2];
    CONST84 char *widgRecList[2];
    int code = TCL_OK;
    int dummy;

    if (sizeChanged_ret == NULL) {
	sizeChanged_ret = &dummy;
    }

    specsList[0] = entConfigSpecs;
    specsList[1] = Tix_DItemConfigSpecs(iPtr);
    widgRecList[0] = (CONST84 char*)entRec;
    widgRecList[1] = (CONST84 char*)iPtr;

    if (Tix_SplitConfig(interp, tkwin, specsList,
 	2, argc, argv, &argList) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Handle the info specific to the entry */
    if (argList.arg[0].argc > 0) {
	if (Tk_ConfigureWidget(interp, tkwin,
	    entConfigSpecs, argList.arg[0].argc, argList.arg[0].argv,
	    (char*)entRec, flags) != TCL_OK) {

	    code = TCL_ERROR;
	    goto done;
	}
    }

    if (iPtr == NULL) {
	goto done;
    }
    if (argList.arg[1].argc > 0 || forced) {
	int oldSize[2];
	oldSize[0] = iPtr->base.size[0];
	oldSize[1] = iPtr->base.size[1];
	if (Tix_DItemConfigure(iPtr, argList.arg[1].argc,
	    argList.arg[1].argv, flags) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}

	if (oldSize[0] != iPtr->base.size[0] ||
	    oldSize[1] != iPtr->base.size[1]) {
	    * sizeChanged_ret = 1;
	} else {
	    * sizeChanged_ret = 0;
	}
    }

  done:

    Tix_FreeArgumentList(&argList);
    return code;
}

/*----------------------------------------------------------------------
 *
 *		 The Tix Customed Config Options
 *
 *----------------------------------------------------------------------
 */
/*
 * The global data structures to use in widget configSpecs arrays
 *
 * These are declared in <tixConfig.h>
 */

Tk_CustomOption tixConfigItemType = {
    DItemParseProc, DItemPrintProc, 0,
};

/*----------------------------------------------------------------------
 *  DItemParseProc --
 *
 *	Parse the text string and store the Tix_DItemType information
 *	inside the widget record.
 *----------------------------------------------------------------------
 */
static int DItemParseProc(clientData, interp, tkwin, value, widRec,offset)
    ClientData clientData;
    Tcl_Interp *interp;
    Tk_Window tkwin;
    CONST84 char *value;
    char *widRec;
    int offset;
{
    Tix_DItemInfo  *newPtr;
    Tix_DItemInfo **ptr = (Tix_DItemInfo **)(widRec + offset);

    if (value == NULL) {
	newPtr = NULL;
    } else {
	newPtr = Tix_GetDItemType(interp, value);
	if (newPtr == NULL) {
	    return TCL_ERROR;
	}
    }
    *ptr = newPtr;

    return TCL_OK;
}

static char *DItemPrintProc(clientData, tkwin, widRec,offset, freeProcPtr)
    ClientData clientData;
    Tk_Window tkwin;
    char *widRec;
    int offset;
    Tcl_FreeProc **freeProcPtr;
{
    Tix_DItemInfo *diTypePtr = *((Tix_DItemInfo**)(widRec+offset));

    if (diTypePtr != NULL) {
	return diTypePtr->name;
    } else {
	return 0;
    }
}
/*----------------------------------------------------------------------
 *
 *
 *			   PRIVATE INTERFACE
 *
 *
 * The following functions are called by display type implementors
 *
 *----------------------------------------------------------------------
 */

/* The priority is selected > disabled > active > normal */

void
TixGetColorDItemGC(iPtr, backGC_ret, foreGC_ret, anchorGC_ret, flags)
    Tix_DItem * iPtr;
    GC * backGC_ret;
    GC * foreGC_ret;
    GC * anchorGC_ret;
    int flags;
{
    TixColorStyle * stylePtr = (TixColorStyle *) iPtr->base.stylePtr;
    GC dummy;
    int bg = 0;

    if (backGC_ret == NULL) {
        backGC_ret = &dummy;
    }
    if (foreGC_ret == NULL) {
        foreGC_ret = &dummy;
    }
    if (anchorGC_ret == NULL) {
        anchorGC_ret = &dummy;
    }

    if (flags & TIX_DITEM_SELECTED_FG) {
	*foreGC_ret = stylePtr->colors[TIX_DITEM_SELECTED].foreGC;
    }
    else if (flags & TIX_DITEM_DISABLED_FG) {
	*foreGC_ret = stylePtr->colors[TIX_DITEM_DISABLED].foreGC;
    }
    else if (flags & TIX_DITEM_ACTIVE_FG) {
	*foreGC_ret = stylePtr->colors[TIX_DITEM_ACTIVE].foreGC;
    }
    else if (flags & TIX_DITEM_NORMAL_FG) {
	*foreGC_ret = stylePtr->colors[TIX_DITEM_NORMAL].foreGC;
    }
    else {
	*foreGC_ret = None;
    }

    if (flags & TIX_DITEM_SELECTED_BG) {
        bg = TIX_DITEM_SELECTED;
    } else if (flags & TIX_DITEM_DISABLED_BG) {
        bg = TIX_DITEM_DISABLED;
    } else if (flags & TIX_DITEM_ACTIVE_BG) {
        bg = TIX_DITEM_ACTIVE;
    } else if (flags & TIX_DITEM_NORMAL_BG) {
        bg = TIX_DITEM_NORMAL;
    } else {
        bg = -1;
    }

    if (bg != -1) {
	*backGC_ret = stylePtr->colors[bg].backGC;
    } else {
        *backGC_ret = None;
    }
        

    if ((flags & TIX_DITEM_ANCHOR) && (bg != -1)) {
	*anchorGC_ret = stylePtr->colors[bg].anchorGC;
    } else {
        *anchorGC_ret = None;
    }
}

/*----------------------------------------------------------------------
 * TixDItemGetAnchor --
 *
 *	Calculate the position of the element according to its anchor
 *----------------------------------------------------------------------
 */
void
TixDItemGetAnchor(anchor, x, y, cav_w, cav_h, width, height, x_ret, y_ret)
    Tk_Anchor anchor;
    int x;
    int y;
    int cav_w;
    int cav_h;
    int width;
    int height;
    int * x_ret;
    int * y_ret;
{
    if (width > cav_w) {
	* x_ret = x;
    } else {
	int rem = cav_w - width;

	switch (anchor) {
	  case TK_ANCHOR_NW: case TK_ANCHOR_W: case TK_ANCHOR_SW:
	    * x_ret = x;
	    break;
	  case TK_ANCHOR_N: case TK_ANCHOR_CENTER: case TK_ANCHOR_S:
	    * x_ret = x + rem/2;
	    break;
	  default:
	    * x_ret = x + rem;
	}
    }
    if (height > cav_h) {
	* y_ret = y;
    }
    else {
	int rem = cav_h - height;
	switch (anchor) {
	  case TK_ANCHOR_NW: case TK_ANCHOR_N: case TK_ANCHOR_NE:
	    * y_ret = y;
	    break;
	  case TK_ANCHOR_W: case TK_ANCHOR_CENTER: case TK_ANCHOR_E:
	    * y_ret = y + rem/2;
	    if ((rem % 2) == 1) {
		/* Usually it looks better if we shift down one pixel
		 * if the hight of the region is an odd number of pixels
		 */
		* y_ret += 1;
	    }
	    break;
	  default:
	    * y_ret = y + rem;
	}
    }
}

int
Tix_DItemFillNormalBG(drawable, subRegPtr, iPtr, x, y, width, height,
       xOffset, yOffset, flags)
    Drawable drawable;          /* Where to display this item */
    TixpSubRegion *subRegPtr;
    Tix_DItem * iPtr;           /* Item to display */
    int x;                      /* x pos of top-left corner of region
                                 * to display item in */
    int y;                      /* y pos of top-left corner of region */
    int width;                  /* width of region */
    int height;                 /* height of region */
    int xOffset;                /* X offset of item within region */
    int yOffset;                /* Y offset of item within region */
    int flags;                  /* Controls how fg/bg/anchor lines are
                                 * drawn */
{
    GC gc;

    if ((flags & TIX_DITEM_NORMAL_BG) == 0) {
        return 0;
    }

    if (width == iPtr->base.size[0] && height == iPtr->base.size[1] && 
            xOffset == 0 && yOffset == 0 &&
            (flags & TIX_DITEM_OTHER_BG) != 0) {
        /*
         * The background area will be filled by another bg color, so
         * there's no need to fill it with NORMAL_BG here.
         */
       
        return 0;
    }

    gc = iPtr->base.stylePtr->colors[TIX_DITEM_NORMAL].backGC;
    if (gc != None) {
        TixpSubRegFillRectangle(iPtr->base.ddPtr->display, drawable,
	        gc, subRegPtr, x, y, width, height);
        return 1;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_DItemDrawBackground --
 *
 *      Display the background color of the item. The background and
 *      foreground of the item are displayed according to the flags
 *      parameter:
 *
 *      If TIX_DITEM_NORMAL_BG is specified in flags, the entire {x, y,
 *      width, height} region is filled with the NORMAL_BG.
 *
 *      If one of the following is specified, a filled highlight rect
 *      is drawn around the selectable area of the item.
 *
 *              TIX_DITEM_ACTIVE_BG
 *              TIX_DITEM_SELECTED_BG
 *              TIX_DITEM_DISABLED_BG
 *
 *      In addition, if TIX_DITEM_ANCHOR is specified in flags, an
 *      anchor rectangle is drawn around the selectable area of the
 *      item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * 
 *----------------------------------------------------------------------
 */

void
Tix_DItemDrawBackground(drawable, subRegPtr, iPtr, x, y, width, height,
       xOffset, yOffset, flags)
    Drawable drawable;          /* Where to display this item */
    TixpSubRegion *subRegPtr;
    Tix_DItem * iPtr;           /* Item to display */
    int x;                      /* x pos of top-left corner of region
                                 * to display item in */
    int y;                      /* y pos of top-left corner of region */
    int width;                  /* width of region */
    int height;                 /* height of region */
    int xOffset;                /* X offset of item within region */
    int yOffset;                /* Y offset of item within region */
    int flags;                  /* Controls how fg/bg/anchor lines are
                                 * drawn */
{
    Display * display = iPtr->base.ddPtr->display;
    GC backGC, anchorGC;
    int bodyX, bodyY, bodyW, bodyH;

    TixGetColorDItemGC(iPtr, &backGC, NULL, &anchorGC, flags);

    /*
     * Fill the entire region with the normal background color first 
     * (if necessary).
     */

    Tix_DItemFillNormalBG(drawable, subRegPtr, iPtr, x, y, width, height,
            xOffset, yOffset, flags);

    /*
     * Calculate the location of the item body.
     */

    TixDItemGetAnchor(iPtr->base.stylePtr->anchor, x, y, width, height,
	    iPtr->base.size[0], iPtr->base.size[1], &x, &y);

    x += xOffset;
    y += yOffset;

    bodyX = x + iPtr->base.selX;
    bodyY = y + iPtr->base.selY;
    bodyW = iPtr->base.selW;
    bodyH = iPtr->base.selH;

    /*
     * Fill the selected background and draw anchor lines only around
     * the item body itself
     */

    if ((flags & TIX_DITEM_OTHER_BG) && (backGC != None)) {
        TixpSubRegSetClip(display, subRegPtr, backGC);
	TixpSubRegFillRectangle(display, drawable, backGC,
		subRegPtr, bodyX, bodyY, bodyW, bodyH);
        TixpSubRegUnsetClip(display, subRegPtr, backGC);
    }

    if (anchorGC != None) {
        TixpSubRegSetClip(display, subRegPtr, anchorGC);
	TixpSubRegDrawAnchorLines(display, drawable,
	        anchorGC, subRegPtr, bodyX, bodyY, bodyW, bodyH);
        TixpSubRegUnsetClip(display, subRegPtr, anchorGC);
    }
}
