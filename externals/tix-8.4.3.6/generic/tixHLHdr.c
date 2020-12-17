
/*	$Id: tixHLHdr.c,v 1.3 2004/03/28 02:44:56 hobbs Exp $	*/

/*
 *  tixHLHdr.c ---
 *
 *
 *	Implements headers for tixHList widgets
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixHList.h>
#include <tixDef.h>

static TIX_DECLARE_SUBCMD(Tix_HLHdrCreate);
static TIX_DECLARE_SUBCMD(Tix_HLHdrConfig);
static TIX_DECLARE_SUBCMD(Tix_HLHdrCGet);
static TIX_DECLARE_SUBCMD(Tix_HLHdrDelete);
static TIX_DECLARE_SUBCMD(Tix_HLHdrExist);
static TIX_DECLARE_SUBCMD(Tix_HLHdrSize);

static void		FreeWindowItem _ANSI_ARGS_((Tcl_Interp  *interp,
			    WidgetPtr wPtr, HListHeader *hPtr));
static void		FreeHeader _ANSI_ARGS_((Tcl_Interp  *interp,
			    WidgetPtr wPtr, HListHeader *hPtr));
static HListHeader*	AllocHeader _ANSI_ARGS_((Tcl_Interp *interp,
			    WidgetPtr wPtr));
static HListHeader*	Tix_HLGetHeader _ANSI_ARGS_((Tcl_Interp * interp,
			    WidgetPtr wPtr, CONST84 char * string,
			    int requireIPtr));

static Tk_ConfigSpec headerConfigSpecs[] = {
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_PIXELS, "-borderwidth", "headerBorderWidth", "BorderWidth",
       DEF_HLISTHEADER_BORDER_WIDTH, Tk_Offset(HListHeader, borderWidth), 0},

    {TK_CONFIG_BORDER, "-headerbackground", "headerBackground", "Background",
       DEF_HLISTHEADER_BG_COLOR, Tk_Offset(HListHeader, background),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-headerbackground", "headerBackground", "Background",
       DEF_HLISTHEADER_BG_MONO, Tk_Offset(HListHeader, background),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_RELIEF, "-relief", "headerRelief", "Relief",
       DEF_HLISTHEADER_RELIEF, Tk_Offset(HListHeader, relief), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

/*----------------------------------------------------------------------
 *
 * 		Local functions
 *
 *----------------------------------------------------------------------
 */

static HListHeader*
AllocHeader(interp, wPtr)
    Tcl_Interp *interp;
    WidgetPtr wPtr;
{
    HListHeader * hPtr = (HListHeader*)ckalloc(sizeof(HListHeader));
    hPtr->type 		= HLTYPE_HEADER;
    hPtr->self 		= (char*)hPtr;
    hPtr->wPtr 		= wPtr;
    hPtr->iPtr 		= NULL;
    hPtr->width 	= 0;
    hPtr->background	= NULL;
    hPtr->relief	= TK_RELIEF_RAISED;
    hPtr->borderWidth	= 2;

    if (Tk_ConfigureWidget(interp, wPtr->dispData.tkwin, headerConfigSpecs,
	    0, 0, (char *)hPtr, 0) != TCL_OK) {
	/* some unrecoverable errors */
	return NULL;
    }
    return hPtr;
}

static void
FreeWindowItem(interp, wPtr, hPtr)
    Tcl_Interp  *interp;
    WidgetPtr    wPtr;
    HListHeader *hPtr;
{
    Tix_WindowItemListRemove(&wPtr->mappedWindows,
	hPtr->iPtr);
}

static void
FreeHeader(interp, wPtr, hPtr)
    Tcl_Interp  *interp;
    WidgetPtr    wPtr;
    HListHeader *hPtr;
{
    if (hPtr->iPtr) {
	if (Tix_DItemType(hPtr->iPtr) == TIX_DITEM_WINDOW) {
	    FreeWindowItem(interp, wPtr, hPtr);
	}
	Tix_DItemFree(hPtr->iPtr);
    }

    Tk_FreeOptions(headerConfigSpecs, (char *)hPtr, wPtr->dispData.display, 0);
    ckfree((char*) hPtr);
}

static HListHeader*
Tix_HLGetHeader(interp, wPtr, string, requireIPtr)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    CONST84 char * string;
    int requireIPtr;
{
    int column;

    if (Tcl_GetInt(interp, string, &column) != TCL_OK) {
	return NULL;
    }
    if (column >= wPtr->numColumns || column < 0) {
	Tcl_AppendResult(interp, "Column \"", string,
	    "\" does not exist", (char*)NULL);
	return NULL;
    }
    if (requireIPtr && wPtr->headers[column]->iPtr == NULL) {
	Tcl_AppendResult(interp, "Column \"", string,
	    "\" does not have a header", (char*)NULL);
	return NULL;
    }

    return wPtr->headers[column];
}

int
Tix_HLCreateHeaders(interp, wPtr)
    Tcl_Interp *interp;
    WidgetPtr wPtr;
{
    int i;

    wPtr->headers = (HListHeader**)
      ckalloc(sizeof(HListHeader*) * wPtr->numColumns);

    for (i=0; i<wPtr->numColumns; i++) {
	wPtr->headers[i] = NULL;
    }

    for (i=0; i<wPtr->numColumns; i++) {
	if ((wPtr->headers[i] = AllocHeader(interp, wPtr)) == NULL) {
	    return TCL_ERROR;
	}
    }

    wPtr->headerDirty = 1;
    return TCL_OK;
}

void Tix_HLFreeHeaders(interp, wPtr)
    Tcl_Interp *interp;
    WidgetPtr wPtr;
{
    int i;

    if (wPtr->headers == NULL) {
	return;
    }

    for (i=0; i<wPtr->numColumns; i++) {
	if (wPtr->headers[i] != NULL) {
	    FreeHeader(interp, wPtr, wPtr->headers[i]);
	}
    }

    ckfree((char*)wPtr->headers);
}

void
Tix_HLDrawHeader(wPtr, pixmap, gc, hdrX, hdrY, hdrW, hdrH, xOffset)
    WidgetPtr wPtr;
    Pixmap pixmap;
    GC gc;
    int hdrX;
    int hdrY;
    int hdrW;
    int hdrH;
    int xOffset;
{
    int i, x, y;
    int drawnWidth;		/* how much of the header have I drawn? */
    int width;			/* width of the current header item */
    int winItemExtra;		/* window items need a bit extra offset
				 * because they must be places relative to
				 * the main window, not the header subwindow
				 */
    x = hdrX - xOffset;
    y = hdrY;
    drawnWidth = 0;

    winItemExtra = wPtr->borderWidth + wPtr->highlightWidth;

    if (wPtr->needToRaise) {
	/* the needToRaise flag is set every time a new window item is
	 * created inside the header of the HList.
	 * 
	 * We need to make sure that the windows items in the list
	 * body are clipped by the header subwindow. However, the window
	 * items inside the header should be over the header subwindow.
	 *
	 * The two XRaiseWindow calls in this function make sure that
	 * the stacking relationship as described above always hold
	 */
	XRaiseWindow(Tk_Display(wPtr->headerWin),
	    Tk_WindowId(wPtr->headerWin));
    }

    for (i=0; i<wPtr->numColumns; i++) {
	HListHeader * hPtr = wPtr->headers[i];
	width = wPtr->actualSize[i].width;

	if (i == wPtr->numColumns-1) {
	    /* If the widget is wider than required,
	     * We need to extend the last item to the end of the list,
	     * or otherwise we'll see a curtailed header
	     */
	    if (drawnWidth + width <hdrW) {
		width = hdrW - drawnWidth;
	    }
	}

        Tk_Fill3DRectangle(wPtr->dispData.tkwin, pixmap, hPtr->background,
	    x, y, width, wPtr->headerHeight, hPtr->borderWidth,
	    hPtr->relief);

	/* Note when we draw the item, we use the
	 * wPtr->actualSize[i].width instead of the (possibly extended) width
	 * so that the header is well-aligned with the element columns.
	 */
	if (hPtr->iPtr) {
	    int itemX, itemY;
	    itemX = x+hPtr->borderWidth;
	    itemY = y+hPtr->borderWidth;

	    if (Tix_DItemType(hPtr->iPtr) == TIX_DITEM_WINDOW) {
		itemX += winItemExtra;
		itemY += winItemExtra;
	    }

	    Tix_DItemDisplay(pixmap, hPtr->iPtr,
	        itemX, itemY,
		wPtr->actualSize[i].width - 2*hPtr->borderWidth,
		wPtr->headerHeight        - 2*hPtr->borderWidth,
                0, 0,
		TIX_DITEM_NORMAL_FG);

	    if (wPtr->needToRaise && 
		Tix_DItemType(hPtr->iPtr) == TIX_DITEM_WINDOW) {
		TixWindowItem * wiPtr;

		wiPtr = (TixWindowItem *)hPtr->iPtr;
		if (Tk_WindowId(wiPtr->tkwin) == None) {
		    Tk_MakeWindowExist(wiPtr->tkwin);
		}

		XRaiseWindow(Tk_Display(wiPtr->tkwin),
		    Tk_WindowId(wiPtr->tkwin));
	    }
	}

	x += width;
	drawnWidth += width;

#if 0
	/* %% funny, doesn't work */
	if (drawnWidth >= hdrW) {
	    /* The rest is invisible. Don't bother to draw */
	    break;
	}
#endif
    }

    wPtr->needToRaise = 0;
}

void Tix_HLComputeHeaderGeometry(wPtr)
    WidgetPtr wPtr;
{
    int i;

    wPtr->headerHeight = 0;

    for (i=0; i<wPtr->numColumns; i++) {
	int height;
	int width;

	if (wPtr->headers[i]->iPtr) {
	    width  = Tix_DItemWidth (wPtr->headers[i]->iPtr);
	    height = Tix_DItemHeight(wPtr->headers[i]->iPtr);
	} else {
	    width  = 0;
	    height = 0;
	}

	width  += wPtr->headers[i]->borderWidth * 2;
	height += wPtr->headers[i]->borderWidth * 2;

	wPtr->headers[i]->width = width;

	if (height > wPtr->headerHeight) {
	    wPtr->headerHeight = height;
	}
    }

    wPtr->headerDirty = 0;
}
 
/*----------------------------------------------------------------------
 * "header" sub command
 *----------------------------------------------------------------------
 */
int
Tix_HLHeader(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    static Tix_SubCmdInfo subCmdInfo[] = {
	{TIX_DEFAULT_LEN, "cget", 2, 2, Tix_HLHdrCGet,
	   "column option"},
	{TIX_DEFAULT_LEN, "configure", 1, TIX_VAR_ARGS, Tix_HLHdrConfig,
	   "column ?option? ?value ...?"},
	{TIX_DEFAULT_LEN, "create", 1, TIX_VAR_ARGS, Tix_HLHdrCreate,
	   "column ?option value ...?"},
	{TIX_DEFAULT_LEN, "delete", 1, 1, Tix_HLHdrDelete,
	   "column"},
	{TIX_DEFAULT_LEN, "exist", 1, 1, Tix_HLHdrExist,
	   "column"},
	{TIX_DEFAULT_LEN, "size", 1, 1, Tix_HLHdrSize,
	   "column"},
    };
    static Tix_CmdInfo cmdInfo = {
	Tix_ArraySize(subCmdInfo), 1, TIX_VAR_ARGS, "?option? ?arg ...?",
    };

    return Tix_HandleSubCmds(&cmdInfo, subCmdInfo, clientData,
	interp, argc+1, argv-1);
}

/*----------------------------------------------------------------------
 * "header cget" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLHdrCGet(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListHeader * hPtr;

    if ((hPtr=Tix_HLGetHeader(interp, wPtr, argv[0], 1)) == NULL) {
	return TCL_ERROR;
    }

    return Tix_ConfigureValue2(interp, wPtr->dispData.tkwin,
	(char*)hPtr, headerConfigSpecs, hPtr->iPtr, argv[1], 0);
}

/*----------------------------------------------------------------------
 * "header configure" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLHdrConfig(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListHeader * hPtr;

    if ((hPtr=Tix_HLGetHeader(interp, wPtr, argv[0], 1)) == NULL) {
	return TCL_ERROR;
    }

    if (argc == 1) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)hPtr, headerConfigSpecs, hPtr->iPtr,
	    (char *) NULL, 0);
    } else if (argc == 2) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)hPtr, headerConfigSpecs, hPtr->iPtr,
	    (char *) argv[1], 0);
    } else {
	int sizeChanged = 0;

	if (Tix_WidgetConfigure2(interp, wPtr->dispData.tkwin,
	    (char*)hPtr, headerConfigSpecs, hPtr->iPtr,
	    argc-1, argv+1, TK_CONFIG_ARGV_ONLY, 0, &sizeChanged) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (sizeChanged) {
	    wPtr->headerDirty = 1;
	    Tix_HLResizeWhenIdle(wPtr);
	}
	return TCL_OK;
    }
}


/*----------------------------------------------------------------------
 * "header create" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLHdrCreate(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListHeader * hPtr;
    int i;
    Tix_DItem * iPtr;
    CONST84 char * ditemType = NULL;

    if ((hPtr=Tix_HLGetHeader(interp, wPtr, argv[0], 0)) == NULL) {
	return TCL_ERROR;
    }

    if ((argc %2) == 0) {
	Tcl_AppendResult(interp, "value for \"", argv[argc-1],
	    "\" missing", NULL);
	return TCL_ERROR;
    }
    for (i=1; i<argc; i+=2) {
	if (strncmp(argv[i], "-itemtype", strlen(argv[i])) == 0) {
	    ditemType = argv[i+1];
	}
    }
    if (ditemType == NULL) {
	ditemType = wPtr->diTypePtr->name;
    }

    iPtr = Tix_DItemCreate(&wPtr->dispData, ditemType);
    if (iPtr == NULL) {
	return TCL_ERROR;
    }
    if (Tix_DItemType(iPtr) == TIX_DITEM_WINDOW) {
	wPtr->needToRaise = 1;
    }

    /*
     * mark clientData to NULL. This will tell DItemSizeChanged()
     * that ths item belongs to the header
     */
    iPtr->base.clientData = (ClientData)hPtr;

    if (hPtr->iPtr != NULL) {
	if (Tix_DItemType(hPtr->iPtr) == TIX_DITEM_WINDOW) {
	    FreeWindowItem(interp, wPtr, hPtr);
	}
	Tix_DItemFree(hPtr->iPtr);
    }
    hPtr->iPtr = iPtr;

    if (Tix_WidgetConfigure2(wPtr->dispData.interp, wPtr->dispData.tkwin,
	(char*)hPtr, headerConfigSpecs, hPtr->iPtr, argc-1, argv+1, 0,
	1, NULL) != TCL_OK) {
	return TCL_ERROR;
    }


    wPtr->headerDirty = 1;
    Tix_HLResizeWhenIdle(wPtr);

    return TCL_OK;
}
/*----------------------------------------------------------------------
 * "header delete" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLHdrDelete(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListHeader * hPtr;

    if ((hPtr=Tix_HLGetHeader(interp, wPtr, argv[0], 1)) == NULL) {
	return TCL_ERROR;
    }

    if (Tix_DItemType(hPtr->iPtr) == TIX_DITEM_WINDOW) {
	FreeWindowItem(interp, wPtr, hPtr);
    }

    /* Free the item and leave a blank! */
    Tix_DItemFree(hPtr->iPtr);
    hPtr->iPtr = NULL;

    wPtr->headerDirty = 1;
    Tix_HLResizeWhenIdle(wPtr);

    return TCL_OK;
}
/*----------------------------------------------------------------------
 * "header exist" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLHdrExist(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListHeader * hPtr;

    if ((hPtr=Tix_HLGetHeader(interp, wPtr, argv[0], 0)) == NULL) {
	return TCL_ERROR;
    }

    if (hPtr->iPtr == NULL) {
	Tcl_AppendResult(interp, "0", NULL);
    } else {
	Tcl_AppendResult(interp, "1", NULL);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "header size" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLHdrSize(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListHeader * hPtr;
    char buff[128];

    if ((hPtr=Tix_HLGetHeader(interp, wPtr, argv[0], 1)) == NULL) {
	return TCL_ERROR;
    }

    if (hPtr->iPtr == NULL) {
	Tcl_AppendResult(interp, "entry \"", argv[0],
	    "\" does not have a header", (char*)NULL); 
	return TCL_ERROR;
    }
    sprintf(buff, "%d %d",
	Tix_DItemWidth(hPtr->iPtr),
	Tix_DItemHeight(hPtr->iPtr));
    Tcl_AppendResult(interp, buff, NULL);
    return TCL_OK;
}
