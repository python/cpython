/* 
 * tixGrid.c --
 *
 *	This module implements "tixGrid" widgets.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixGrid.c,v 1.6 2008/02/28 04:10:43 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>
#include <tixGrid.h>

/*
 * Information used for argv parsing.
 */
static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-background", "background", "Background",
       DEF_GRID_BG_COLOR, Tk_Offset(WidgetRecord, normalBg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-background", "background", "Background",
       DEF_GRID_BG_MONO, Tk_Offset(WidgetRecord, normalBg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
       DEF_GRID_BORDER_WIDTH, Tk_Offset(WidgetRecord, borderWidth), 0},

    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
       DEF_GRID_CURSOR, Tk_Offset(WidgetRecord, cursor),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-editdonecmd", "editDoneCmd", "EditDoneCmd",
       DEF_GRID_EDITDONE_COMMAND, Tk_Offset(WidgetRecord, editDoneCmd),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-editnotifycmd", "editNotifyCmd", "EditNotifyCmd",
       DEF_GRID_EDITNOTIFY_COMMAND, Tk_Offset(WidgetRecord, editNotifyCmd),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_BOOLEAN, "-floatingcols", "floatingCols", "FloatingCols",
	DEF_GRID_FLOATING_COLS, Tk_Offset(WidgetRecord, floatRange[1]), 0},

    {TK_CONFIG_BOOLEAN, "-floatingrows", "floatingRows", "FloatingRows",
	DEF_GRID_FLOATING_ROWS, Tk_Offset(WidgetRecord, floatRange[0]), 0},

    {TK_CONFIG_FONT, "-font", "font", "Font",
       DEF_GRID_FONT, Tk_Offset(WidgetRecord, font), 0},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_GRID_FG_COLOR, Tk_Offset(WidgetRecord, normalFg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_GRID_FG_MONO, Tk_Offset(WidgetRecord, normalFg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_STRING, "-formatcmd", "formatCmd", "FormatCmd",
       DEF_GRID_FORMAT_COMMAND, Tk_Offset(WidgetRecord, formatCmd),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_PIXELS, "-height", "height", "Height",
       DEF_GRID_HEIGHT, Tk_Offset(WidgetRecord, reqSize[1]), 0},

    {TK_CONFIG_BORDER, "-highlightbackground", "highlightBackground",
       "HighlightBackground",
       DEF_GRID_BG_COLOR, Tk_Offset(WidgetRecord, border),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-highlightbackground", "highlightBackground",
       "HighlightBackground",
       DEF_GRID_BG_MONO, Tk_Offset(WidgetRecord, border),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
       DEF_GRID_HIGHLIGHT_COLOR, Tk_Offset(WidgetRecord, highlightColorPtr),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
       DEF_GRID_HIGHLIGHT_MONO, Tk_Offset(WidgetRecord, highlightColorPtr),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
       "HighlightThickness",
       DEF_GRID_HIGHLIGHT_WIDTH, Tk_Offset(WidgetRecord, highlightWidth), 0},

    {TK_CONFIG_INT, "-leftmargin", "leftMargin", "LeftMargin",
       DEF_GRID_LEFT_MARGIN, Tk_Offset(WidgetRecord, hdrSize[0]), 0},

    {TK_CONFIG_CUSTOM, "-itemtype", "itemType", "ItemType",
       DEF_GRID_ITEM_TYPE, Tk_Offset(WidgetRecord, diTypePtr),
       0, &tixConfigItemType},

    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
	DEF_GRID_PADX, Tk_Offset(WidgetRecord, padX), 0},

    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
	DEF_GRID_PADY, Tk_Offset(WidgetRecord, padY), 0},

    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
       DEF_GRID_RELIEF, Tk_Offset(WidgetRecord, relief), 0},

    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_GRID_SELECT_BG_COLOR, Tk_Offset(WidgetRecord, selectBorder),
	TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_GRID_SELECT_BG_MONO, Tk_Offset(WidgetRecord, selectBorder),
	TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-selectborderwidth", "selectBorderWidth","BorderWidth",
       DEF_GRID_SELECT_BORDERWIDTH,Tk_Offset(WidgetRecord, selBorderWidth),0},

    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
       DEF_GRID_SELECT_FG_COLOR, Tk_Offset(WidgetRecord, selectFg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
       DEF_GRID_SELECT_FG_MONO, Tk_Offset(WidgetRecord, selectFg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_UID, "-selectmode", "selectMode", "SelectMode",
	DEF_GRID_SELECT_MODE, Tk_Offset(WidgetRecord, selectMode), 0},

    {TK_CONFIG_UID, "-selectunit", "selectUnit", "SelectUnit",
	DEF_GRID_SELECT_UNIT, Tk_Offset(WidgetRecord, selectUnit), 0},

    {TK_CONFIG_STRING, "-sizecmd", "sizeCmd", "SizeCmd",
       DEF_GRID_SIZE_COMMAND, Tk_Offset(WidgetRecord, sizeCmd),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_UID, "-state", (char*)NULL, (char*)NULL,
       DEF_GRID_STATE, Tk_Offset(WidgetRecord, state), 0},
 
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_GRID_TAKE_FOCUS, Tk_Offset(WidgetRecord, takeFocus),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_INT, "-topmargin", "topMargin", "TopMargin",
       DEF_GRID_TOP_MARGIN, Tk_Offset(WidgetRecord, hdrSize[1]), 0},

    {TK_CONFIG_PIXELS, "-width", "width", "Width",
	DEF_GRID_WIDTH, Tk_Offset(WidgetRecord, reqSize[0]), 0},

    {TK_CONFIG_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
       DEF_GRID_X_SCROLL_COMMAND,
       Tk_Offset(WidgetRecord, scrollInfo[0].command),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
       DEF_GRID_Y_SCROLL_COMMAND,
       Tk_Offset(WidgetRecord, scrollInfo[1].command),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};	

static Tk_ConfigSpec entryConfigSpecs[] = {
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};


/*
 * Forward declarations for procedures defined later in this file:
 */

	/* These are standard procedures for TK widgets
	 * implemeted in C
	 */

static void		WidgetCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static int		WidgetConfigure _ANSI_ARGS_((Tcl_Interp *interp,
			    WidgetPtr wPtr, int argc, CONST84 char **argv,
			    int flags));
static void		WidgetDestroy _ANSI_ARGS_((ClientData clientData));
static void		WidgetEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		WidgetCommand _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *, int argc, CONST84 char **argv));
static void		WidgetDisplay _ANSI_ARGS_((ClientData clientData));
static void		WidgetComputeGeometry _ANSI_ARGS_((
			    ClientData clientData));
static void		IdleHandler _ANSI_ARGS_((
			    ClientData clientData));
	/* Extra procedures for this widget
	 */
static int		ConfigElement _ANSI_ARGS_((WidgetPtr wPtr,
			    TixGrEntry *chPtr, int argc, CONST84 char ** argv,
			    int flags, int forced));
static void		Tix_GrDisplayMainBody _ANSI_ARGS_((
			    WidgetPtr wPtr, Drawable buffer,
			    int winW, int winH));
static void		Tix_GrDrawBackground _ANSI_ARGS_((WidgetPtr wPtr,
			    RenderInfo * riPtr,Drawable drawable));
static void		Tix_GrDrawCells _ANSI_ARGS_((WidgetPtr wPtr,
			    RenderInfo * riPtr,Drawable drawable));
static void		Tix_GrDrawSites _ANSI_ARGS_((WidgetPtr wPtr,
			    RenderInfo * riPtr,Drawable drawable));
int			Tix_GrGetElementPosn _ANSI_ARGS_((
    			    WidgetPtr wPtr, int x, int y,
			    int rect[2][2], int clipOK, int isSite,
			    int isScr, int nearest));
static void		UpdateScrollBars _ANSI_ARGS_((WidgetPtr wPtr,
			    int sizeChanged));
static void		GetScrollFractions _ANSI_ARGS_((
			    WidgetPtr wPtr, Tix_GridScrollInfo *siPtr,
			    double * first_ret, double * last_ret));
static void		Tix_GrDItemSizeChanged _ANSI_ARGS_((
			    Tix_DItem *iPtr));
static TixGrEntry *	Tix_GrFindCreateElem _ANSI_ARGS_((Tcl_Interp * interp,
			    WidgetPtr wPtr, int x, int y));
static TixGrEntry *	Tix_GrFindElem _ANSI_ARGS_((Tcl_Interp * interp,
			    WidgetPtr wPtr, int x, int y));
static void		Tix_GrPropagateSize _ANSI_ARGS_((
			    WidgetPtr wPtr, TixGrEntry * chPtr));
static RenderBlock *	Tix_GrAllocateRenderBlock _ANSI_ARGS_((
			    WidgetPtr wPtr, int winW, int winH,
			    int *exactW, int *exactH));
static void		Tix_GrFreeRenderBlock _ANSI_ARGS_((
			    WidgetPtr wPtr, RenderBlock * rbPtr));
static void		Tix_GrComputeSelection _ANSI_ARGS_((
			    WidgetPtr wPtr));
static int		Tix_GrBBox _ANSI_ARGS_((Tcl_Interp * interp,
			    WidgetPtr wPtr, int x, int y));
static int		TranslateFromTo _ANSI_ARGS_((Tcl_Interp * interp,
			    WidgetPtr wPtr, int argc, CONST84 char **argv, int *from,
			    int * to, int *which));
static void		Tix_GrComputeSubSelection _ANSI_ARGS_((
			    WidgetPtr wPtr, int rect[2][2], int offs[2]));
static int		Tix_GrCallFormatCmd _ANSI_ARGS_((WidgetPtr wPtr,
			    int which));
static void		RecalScrollRegion _ANSI_ARGS_((WidgetPtr wPtr,
			    int winW, int winH,
			    Tix_GridScrollInfo *scrollInfo));
static void		Tix_GrResetRenderBlocks _ANSI_ARGS_((WidgetPtr wPtr));

static TIX_DECLARE_SUBCMD(Tix_GrBdType);
static TIX_DECLARE_SUBCMD(Tix_GrCGet);
static TIX_DECLARE_SUBCMD(Tix_GrConfig);
static TIX_DECLARE_SUBCMD(Tix_GrDelete);
static TIX_DECLARE_SUBCMD(Tix_GrEdit);
static TIX_DECLARE_SUBCMD(Tix_GrEntryCget);
static TIX_DECLARE_SUBCMD(Tix_GrEntryConfig);
EXTERN TIX_DECLARE_SUBCMD(Tix_GrFormat);
static TIX_DECLARE_SUBCMD(Tix_GrGeometryInfo);
static TIX_DECLARE_SUBCMD(Tix_GrInfo);
static TIX_DECLARE_SUBCMD(Tix_GrIndex);
static TIX_DECLARE_SUBCMD(Tix_GrMove);
static TIX_DECLARE_SUBCMD(Tix_GrNearest);
EXTERN TIX_DECLARE_SUBCMD(Tix_GrSelection);
static TIX_DECLARE_SUBCMD(Tix_GrSet);
EXTERN TIX_DECLARE_SUBCMD(Tix_GrSetSize);
static TIX_DECLARE_SUBCMD(Tix_GrSetSite);
EXTERN TIX_DECLARE_SUBCMD(Tix_GrSort);
static TIX_DECLARE_SUBCMD(Tix_GrView);
static TIX_DECLARE_SUBCMD(Tix_GrUnset);


/*
 *--------------------------------------------------------------
 *
 * Tix_GridCmd --
 *
 *	This procedure is invoked to process the "tixGrid" Tcl
 *	command.  It creates a new "TixGrid" widget.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	A new widget is created and configured.
 *
 *--------------------------------------------------------------
 */
int
Tix_GridCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window mainwin = (Tk_Window) clientData;
    WidgetPtr wPtr;
    Tk_Window tkwin;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args:  should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, mainwin, argv[1], (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    Tk_SetClass(tkwin, "TixGrid");

    /*
     * Allocate and initialize the widget record.
     */
    wPtr = (WidgetPtr) ckalloc(sizeof(WidgetRecord));

    wPtr->dispData.tkwin 	= tkwin;
    wPtr->dispData.display 	= Tk_Display(tkwin);
    wPtr->dispData.interp 	= interp;
    wPtr->dispData.sizeChangedProc = Tix_GrDItemSizeChanged;
    wPtr->font		= NULL;
    wPtr->normalBg 		= NULL;
    wPtr->normalFg		= NULL;
    wPtr->command 		= NULL;
    wPtr->border 		= NULL;
    wPtr->borderWidth 		= 0;
    wPtr->selectBorder 		= NULL;
    wPtr->selBorderWidth 	= 0;
    wPtr->selectFg		= NULL;
    wPtr->backgroundGC		= None;
    wPtr->selectGC		= None;
    wPtr->anchorGC		= None;
    wPtr->highlightWidth	= 0;
    wPtr->highlightColorPtr	= NULL;
    wPtr->highlightGC		= None;
    wPtr->relief 		= TK_RELIEF_FLAT;
    wPtr->cursor 		= None;
    wPtr->selectMode		= NULL;
    wPtr->selectUnit		= NULL;
    wPtr->anchor[0] 		= TIX_SITE_NONE;
    wPtr->anchor[1] 		= TIX_SITE_NONE;
    wPtr->dragSite[0] 		= TIX_SITE_NONE;
    wPtr->dragSite[1] 		= TIX_SITE_NONE;
    wPtr->dropSite[0] 		= TIX_SITE_NONE;
    wPtr->dropSite[1] 		= TIX_SITE_NONE;
    wPtr->browseCmd		= 0;
    wPtr->formatCmd		= 0;
    wPtr->editDoneCmd		= 0;
    wPtr->editNotifyCmd		= 0;
    wPtr->sizeCmd		= 0;
    wPtr->takeFocus		= NULL;
    wPtr->serial		= 0;
    wPtr->mainRB		= (RenderBlock*)NULL;
    wPtr->hdrSize[0]		= 1;
    wPtr->hdrSize[1]		= 1;
    wPtr->expArea.x1 		= 10000;
    wPtr->expArea.y1 		= 10000;
    wPtr->expArea.x2 		= 0;
    wPtr->expArea.y2 		= 0;
    wPtr->dataSet		= TixGridDataSetInit();
    wPtr->renderInfo		= NULL;
    wPtr->defSize[0].sizeType	= TIX_GR_DEFINED_CHAR;
    wPtr->defSize[0].charValue	= 10.0;
    wPtr->defSize[0].pad0	= 2;
    wPtr->defSize[0].pad1	= 2;
    wPtr->defSize[1].sizeType	= TIX_GR_DEFINED_CHAR;
    wPtr->defSize[1].charValue	= 1.2;
    wPtr->defSize[1].pad0	= 2;
    wPtr->defSize[1].pad1	= 2;
    wPtr->gridSize[0]		= 0;
    wPtr->gridSize[1]		= 0;
    wPtr->reqSize[0]		= 0;
    wPtr->reqSize[1]		= 0;
    wPtr->state			= tixNormalUid;
    wPtr->colorInfoCounter	= 0;

    /* The flags */
    wPtr->idleEvent 		= 0;
    wPtr->toRedraw 		= 0;
    wPtr->toResize 		= 0;
    wPtr->toResetRB 		= 0;
    wPtr->toComputeSel 		= 0;
    wPtr->toRedrawHighlight 	= 0;

    wPtr->scrollInfo[0].command  = NULL;
    wPtr->scrollInfo[1].command  = NULL;

    wPtr->scrollInfo[0].max  = 1;
    wPtr->scrollInfo[0].unit   = 1;
    wPtr->scrollInfo[0].offset = 0;
    wPtr->scrollInfo[0].window = 1.0;
    wPtr->scrollInfo[1].max  = 1;
    wPtr->scrollInfo[1].unit   = 1;
    wPtr->scrollInfo[1].offset = 0;
    wPtr->scrollInfo[1].window = 1.0;

    Tix_SimpleListInit(&wPtr->colorInfo);
    Tix_SimpleListInit(&wPtr->selList);
    Tix_SimpleListInit(&wPtr->mappedWindows);

    Tk_CreateEventHandler(wPtr->dispData.tkwin,
	ExposureMask|StructureNotifyMask|FocusChangeMask,
	WidgetEventProc, (ClientData) wPtr);
    wPtr->widgetCmd = Tcl_CreateCommand(interp,
	Tk_PathName(wPtr->dispData.tkwin), WidgetCommand, (ClientData) wPtr,
	WidgetCmdDeletedProc);

    if (WidgetConfigure(interp, wPtr, argc-2, argv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(wPtr->dispData.tkwin);
	return TCL_ERROR;
    }

    Tcl_SetResult(interp, Tk_PathName(wPtr->dispData.tkwin), TCL_VOLATILE);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetConfigure --
 *
 *	This procedure is called to process an argv/argc list in
 *	conjunction with the Tk option database to configure (or
 *	reconfigure) a List widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for wPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */
static int
WidgetConfigure(interp, wPtr, argc, argv, flags)
    Tcl_Interp *interp;			/* Used for error reporting. */
    WidgetPtr wPtr;			/* Information about widget. */
    int argc;				/* Number of valid entries in argv. */
    CONST84 char **argv;		/* Arguments. */
    int flags;				/* Flags to pass to
					 * Tk_ConfigureWidget. */
{
    XGCValues gcValues;
    GC newGC;
    TixFont oldfont;
    Tix_StyleTemplate stTmpl;

    oldfont = wPtr->font;

    if (Tk_ConfigureWidget(interp, wPtr->dispData.tkwin, configSpecs,
	    argc, argv, (char *) wPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    wPtr->bdPad = wPtr->highlightWidth + wPtr->borderWidth;

    if ((wPtr->state != tixNormalUid) && (wPtr->state != tixDisabledUid)) {
	Tcl_AppendResult(interp, "bad state value \"", wPtr->state,
	    "\":  must be normal or disabled", (char *) NULL);
	wPtr->state = tixNormalUid;
	return TCL_ERROR;
    }

    if (oldfont != wPtr->font) {
	int i;

	/*
	 * Font has been changed (initialized), we need to reset the render
	 * blocks
	 */
	wPtr->toResetRB = 1;

	TixComputeTextGeometry(wPtr->font, "0", 1,
	    0, &wPtr->fontSize[0], &wPtr->fontSize[1]);

	/* Recalculate the default size of the cells	
	 */
	for (i=0; i<2; i++) {
	    switch (wPtr->defSize[i].sizeType) {
	      case TIX_GR_DEFINED_CHAR:
		wPtr->defSize[i].pixels = (int)
		    (wPtr->defSize[i].charValue * wPtr->fontSize[i]);
		break;
	      case TIX_GR_AUTO:
		if (i==0) {
		    wPtr->defSize[i].pixels = 10 * wPtr->fontSize[0];
		}
		if (i==1) {
		    wPtr->defSize[i].pixels =  1 * wPtr->fontSize[1];
		}
		break;
	    }
	}
    }

    /*
     * A few options need special processing, such as setting the
     * background from a 3-D border, or filling in complicated
     * defaults that couldn't be specified to Tk_ConfigureWidget.
     */

    Tk_SetBackgroundFromBorder(wPtr->dispData.tkwin, wPtr->border);

    /*
     * Note: GraphicsExpose events are disabled in normalGC because it's
     * used to copy stuff from an off-screen pixmap onto the screen (we know
     * that there's no problem with obscured areas).
     */

    /* The background GC */
    gcValues.foreground 	= wPtr->normalBg->pixel;
    gcValues.graphics_exposures = False;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCGraphicsExposures, &gcValues);
    if (wPtr->backgroundGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->backgroundGC);
    }
    wPtr->backgroundGC = newGC;

    /* The selected text GC */
    gcValues.font 		= TixFontId(wPtr->font);
    gcValues.foreground 	= wPtr->selectFg->pixel;
    gcValues.background 	= Tk_3DBorderColor(wPtr->selectBorder)->pixel;
    gcValues.graphics_exposures = False;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCBackground|GCFont|GCGraphicsExposures, &gcValues);
    if (wPtr->selectGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->selectGC);
    }
    wPtr->selectGC = newGC;

    /* The dotted anchor lines */
    gcValues.foreground 	= wPtr->normalFg->pixel;
    gcValues.background 	= wPtr->normalBg->pixel;
    gcValues.graphics_exposures = False;
    gcValues.line_style         = LineDoubleDash;
    gcValues.dashes 		= 2;
    gcValues.subwindow_mode	= IncludeInferiors;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCBackground|GCGraphicsExposures|GCLineStyle|GCDashList|
	GCSubwindowMode, &gcValues);
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC);
    }
    wPtr->anchorGC = newGC;

    /* The highlight border */
    gcValues.background 	= wPtr->selectFg->pixel;
    gcValues.foreground 	= wPtr->highlightColorPtr->pixel;
    gcValues.graphics_exposures = False;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCBackground|GCGraphicsExposures, &gcValues);
    if (wPtr->highlightGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->highlightGC);
    }
    wPtr->highlightGC = newGC;

    /* We must set the options of the default styles so that
     * -- the default styles will change according to what is in
     *    stTmpl
     */
    stTmpl.font 			= wPtr->font;
    stTmpl.pad[0]  			= wPtr->padX;
    stTmpl.pad[1]  			= wPtr->padY;
    stTmpl.colors[TIX_DITEM_NORMAL].fg  = wPtr->normalFg;
    stTmpl.colors[TIX_DITEM_NORMAL].bg  = wPtr->normalBg;
    stTmpl.colors[TIX_DITEM_SELECTED].fg= wPtr->selectFg;
    stTmpl.colors[TIX_DITEM_SELECTED].bg= Tk_3DBorderColor(wPtr->selectBorder);
    stTmpl.flags = TIX_DITEM_FONT|TIX_DITEM_NORMAL_BG|
	TIX_DITEM_SELECTED_BG|TIX_DITEM_NORMAL_FG|TIX_DITEM_SELECTED_FG |
	TIX_DITEM_PADX|TIX_DITEM_PADY;

    Tix_SetDefaultStyleTemplate(wPtr->dispData.tkwin, &stTmpl);

    Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);

    return TCL_OK;
}
/*
 *--------------------------------------------------------------
 *
 * WidgetCommand --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
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
WidgetCommand(clientData, interp, argc, argv)
    ClientData clientData;		/* Information about the widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST84 char **argv;		/* Argument strings. */
{
    int code;

    static Tix_SubCmdInfo subCmdInfo[] = {
	{TIX_DEFAULT_LEN, "anchor", 1, 3, Tix_GrSetSite,
	   "option ?x y?"},
	{TIX_DEFAULT_LEN, "bdtype", 2, 4, Tix_GrBdType,
	   "x y ?xbdWidth ybdWidth?"},
	{TIX_DEFAULT_LEN, "cget", 1, 1, Tix_GrCGet,
	   "option"},
	{TIX_DEFAULT_LEN, "configure", 0, TIX_VAR_ARGS, Tix_GrConfig,
	   "?option? ?value? ?option value ... ?"},
	{TIX_DEFAULT_LEN, "delete", 2, 3, Tix_GrDelete,
	   "option from ?to?"},
	{TIX_DEFAULT_LEN, "dragsite", 1, 3, Tix_GrSetSite,
	   "option ?x y?"},
	{TIX_DEFAULT_LEN, "dropsite", 1, 3, Tix_GrSetSite,
	   "option ?x y?"},
	{TIX_DEFAULT_LEN, "entrycget", 3, 3, Tix_GrEntryCget,
	   "x y option"},
	{TIX_DEFAULT_LEN, "edit", 1, 3, Tix_GrEdit,
	   "option ?args ...?"},
	{TIX_DEFAULT_LEN, "entryconfigure", 2, TIX_VAR_ARGS, Tix_GrEntryConfig,
	   "x y ?option? ?value? ?option value ... ?"},
	{TIX_DEFAULT_LEN, "format", 1, TIX_VAR_ARGS, Tix_GrFormat,
	   "option ?args ...?"},
	{TIX_DEFAULT_LEN, "geometryinfo", 0, 2, Tix_GrGeometryInfo,
	   "?width height?"},
	{TIX_DEFAULT_LEN, "info", 1, TIX_VAR_ARGS, Tix_GrInfo,
	   "option ?args ...?"},
	{TIX_DEFAULT_LEN, "index", 2, 2, Tix_GrIndex,
	   "x y"},
	{TIX_DEFAULT_LEN, "move", 4, 4, Tix_GrMove,
	   "option from to by"},
	{TIX_DEFAULT_LEN, "nearest", 2, 2, Tix_GrNearest,
	   "x y"},
#if 0
	{TIX_DEFAULT_LEN, "see", 1, 1, Tix_GrSee,
	   "x y"},
#endif
	{TIX_DEFAULT_LEN, "selection", 3, 5, Tix_GrSelection,
	   "option x1 y1 ?x2 y2?"},
	{TIX_DEFAULT_LEN, "set", 2, TIX_VAR_ARGS, Tix_GrSet,
	   "x y ?option value ...?"},
	{TIX_DEFAULT_LEN, "size", 1, TIX_VAR_ARGS, Tix_GrSetSize,
	   "option ?args ...?"},
#ifndef __WIN32__
	{TIX_DEFAULT_LEN, "sort", 3, TIX_VAR_ARGS, Tix_GrSort,
	   "dimension start end ?args ...?"},
#endif
	{TIX_DEFAULT_LEN, "unset", 2, 2, Tix_GrUnset,
	   "x y"},
	{TIX_DEFAULT_LEN, "xview", 0, 3, Tix_GrView,
	   "args"},
	{TIX_DEFAULT_LEN, "yview", 0, 3, Tix_GrView,
	   "args"},
    };

    static Tix_CmdInfo cmdInfo = {
	Tix_ArraySize(subCmdInfo), 1, TIX_VAR_ARGS, "?option? arg ?arg ...?",
    };

    Tk_Preserve(clientData);
    code = Tix_HandleSubCmds(&cmdInfo, subCmdInfo, clientData,
	interp, argc, argv);
    Tk_Release(clientData);

    return code;
}

/*
 *--------------------------------------------------------------
 *
 * WidgetEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on Lists.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
WidgetEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int x2, y2;

    switch (eventPtr->type) {
      case DestroyNotify:
	if (wPtr->dispData.tkwin != NULL) {
	    wPtr->dispData.tkwin = NULL;
	    Tcl_DeleteCommand(wPtr->dispData.interp, 
	        Tcl_GetCommandName(wPtr->dispData.interp, wPtr->widgetCmd));
	}
	Tix_GrCancelDoWhenIdle(wPtr);
	Tk_EventuallyFree((ClientData) wPtr, (Tix_FreeProc*)WidgetDestroy);
	break;

      case ConfigureNotify:
	wPtr->expArea.x1 = 0;
	wPtr->expArea.y1 = 0;
	wPtr->expArea.x2 = Tk_Width (wPtr->dispData.tkwin) - 1;
	wPtr->expArea.y2 = Tk_Height(wPtr->dispData.tkwin) - 1;
	Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);
	break;

      case Expose:
	if (wPtr->expArea.x1 > eventPtr->xexpose.x) {
	    wPtr->expArea.x1 = eventPtr->xexpose.x;
	}
	if (wPtr->expArea.y1 > eventPtr->xexpose.y) {
	    wPtr->expArea.y1 = eventPtr->xexpose.y;
	}
	x2 = eventPtr->xexpose.x + eventPtr->xexpose.width  - 1;
	y2 = eventPtr->xexpose.y + eventPtr->xexpose.height - 1;

	if (wPtr->expArea.x2 < x2) {
	    wPtr->expArea.x2 = x2;
	}
	if (wPtr->expArea.y2 < y2) {
	    wPtr->expArea.y2 = y2;
	}
	wPtr->toRedrawHighlight = 1;
	Tix_GrDoWhenIdle(wPtr, TIX_GR_REDRAW);
	break;

      case FocusIn:
	wPtr->hasFocus = 1;
	wPtr->toRedrawHighlight = 1;
	Tix_GrDoWhenIdle(wPtr, TIX_GR_REDRAW);
	break;

      case FocusOut:
	wPtr->hasFocus = 0;
	wPtr->toRedrawHighlight = 1;
	Tix_GrDoWhenIdle(wPtr, TIX_GR_REDRAW);
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetDestroy --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a List at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the List is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
WidgetDestroy(clientData)
    ClientData clientData;	/* Info about the Grid widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    if (wPtr->dataSet) {
	Tix_GrDataRowSearch rowSearch;
	Tix_GrDataCellSearch cellSearch;
	int rowDone, cellDone;

	for (rowDone = TixGrDataFirstRow(wPtr->dataSet, &rowSearch);
	        !rowDone;
	        rowDone = TixGrDataNextRow(&rowSearch)) {


	    for (cellDone = TixGrDataFirstCell(&rowSearch, &cellSearch);
		    !cellDone;
		    cellDone = TixGrDataNextCell(&cellSearch)) {

		TixGridDataDeleteSearchedEntry(&cellSearch);
		Tix_GrFreeElem((TixGrEntry*)cellSearch.data);
	    }
	}

	TixGridDataSetFree(wPtr->dataSet);
    }

    if (wPtr->backgroundGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->backgroundGC);
    }
    if (wPtr->selectGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->selectGC);
    }
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC);
    }
    if (wPtr->highlightGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->highlightGC);
    }

    if (wPtr->mainRB) {
	Tix_GrFreeRenderBlock(wPtr, wPtr->mainRB);
    }

    Tix_GrFreeUnusedColors(wPtr, 1);

    if (!Tix_IsLinkListEmpty(wPtr->mappedWindows)) {
	/*
	 * All mapped windows should have been unmapped when the
	 * the entries were deleted
	 */
	panic("tixGrid: mappedWindows not NULL");
    }

    Tk_FreeOptions(configSpecs, (char *) wPtr, wPtr->dispData.display, 0);
    ckfree((char *) wPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */
static void
WidgetCmdDeletedProc(clientData)
    ClientData clientData;	/* Info about Grid widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */
    if (wPtr->dispData.tkwin != NULL) {
	Tk_Window tkwin = wPtr->dispData.tkwin;
	wPtr->dispData.tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

static void
RecalScrollRegion(wPtr, winW, winH, scrollInfo)
    WidgetPtr wPtr;		/* Info about Grid widget. */
    int winW;
    int winH;
    Tix_GridScrollInfo *scrollInfo;
{
    int gridSize[2];
    int winSize[2];
    int i, k;
    int count;
    int visibleSize;
    int totalSize;
    int pad0, pad1;

    winSize[0] = winW;
    winSize[1] = winH;
    
    TixGridDataGetGridSize(wPtr->dataSet, &gridSize[0],
	&gridSize[1]);

    for (i=0; i<2; i++) {
	for (k=0; k<wPtr->hdrSize[i] && k<gridSize[i]; k++) {
	    winSize[i] -= TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		k, &wPtr->defSize[i], &pad0, &pad1);
	    winSize[i] -= pad0 + pad1;
	}
	if (winSize[i] <= 0) {
	    /*
	     * The window's contents are not visible.
	     */
	    scrollInfo[i].max    = 0;
	    scrollInfo[i].window = 1.0;
	    continue;
	}
	if (wPtr->hdrSize[i] >= gridSize[i]) {
	    /*
	     * There is no scrollable stuff in this dimension.
	     */
	    scrollInfo[i].max    = 0;
	    scrollInfo[i].window = 1.0;
	    continue;
	}

	visibleSize = winSize[i];

	for (count=0,k=gridSize[i]-1; k>=wPtr->hdrSize[i]&&k>=0; count++,k--) {
	    winSize[i] -= TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		k, &wPtr->defSize[i], &pad0, &pad1);
	    winSize[i] -= pad0 + pad1;

	    if (winSize[i] == 0) {
		++ count;
		break;
	    }
	    else if (winSize[i] < 0) {
		break;
	    }
	}

	if (count == 0) {
	    /*
	     * There is only one scrollable element and it is  *partially*
	     * visible.
	     */
	    count = 1;
	}
	scrollInfo[i].max  = (gridSize[i]-wPtr->hdrSize[i]) - count;

	/*
	 * calculate the total pixel size  (%%SLOOOOOOW)
	 */
	for (totalSize=0,k=wPtr->hdrSize[i];k<gridSize[i];k++) {
	    totalSize += TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
	        k, &wPtr->defSize[i], &pad0, &pad1);
	    totalSize += pad0 + pad1;
	}

	/* 
	 *we may need some left over spaces after the last element.
	 */
	totalSize += (-winSize[i]);

	scrollInfo[i].window =
	  (double)(visibleSize) / (double)totalSize;
    }
    for (i=0; i<2; i++) {
	if (scrollInfo[i].offset < 0) {
	    scrollInfo[i].offset  = 0;
	}
	if (scrollInfo[i].offset > scrollInfo[i].max) {
	    scrollInfo[i].offset = scrollInfo[i].max;
	}
    }
}


/*
 *--------------------------------------------------------------
 *
 * WidgetComputeGeometry --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	none
 *
 *--------------------------------------------------------------
 */
static void
WidgetComputeGeometry(clientData)
    ClientData clientData;
{
    WidgetPtr wPtr = (WidgetPtr)clientData;
    int i, k;
    int gridSize[2];
    int req[2], pad0, pad1;
    Tk_Window tkwin = wPtr->dispData.tkwin;

    TixGridDataGetGridSize(wPtr->dataSet, &gridSize[0],
	&gridSize[1]);

    for (i=0; i<2; i++) {
	int end = wPtr->reqSize[i];
	if (end == 0) {
	    end = gridSize[0] + 1;
	}
	for (req[i]=0,k=0; k<end; k++) {
	    req[i] += TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		k, &wPtr->defSize[i], &pad0, &pad1);
	    req[i] += pad0 + pad1;
	}

	req[i] += 2*(wPtr->highlightWidth + wPtr->borderWidth);
    }

    if (Tk_ReqWidth(tkwin) != req[0] || Tk_ReqHeight(tkwin) != req[0]) {
	Tk_GeometryRequest(tkwin, req[0], req[1]);
    }

    /* arrange for the widget to be redrawn */
    wPtr->toResetRB      = 1;
    wPtr->toComputeSel   = 1;
    wPtr->toRedrawHighlight = 1;

    Tix_GrDoWhenIdle(wPtr, TIX_GR_REDRAW);
}

static void
Tix_GrResetRenderBlocks(wPtr)
    WidgetPtr wPtr;
{
    int winW, winH, exactW, exactH;
    Tk_Window tkwin = wPtr->dispData.tkwin;

    winW = Tk_Width (tkwin) - 2*wPtr->highlightWidth - 2*wPtr->borderWidth;
    winH = Tk_Height(tkwin) - 2*wPtr->highlightWidth - 2*wPtr->borderWidth;

    RecalScrollRegion(wPtr, winW, winH, wPtr->scrollInfo);

    UpdateScrollBars(wPtr, 1);

    if (wPtr->mainRB) {
	Tix_GrFreeRenderBlock(wPtr, wPtr->mainRB);
    }
    wPtr->mainRB = Tix_GrAllocateRenderBlock(wPtr, winW, winH,&exactW,&exactH);

    wPtr->expArea.x1 = 0;
    wPtr->expArea.y1 = 0;
    wPtr->expArea.x2 = Tk_Width (wPtr->dispData.tkwin) - 1;
    wPtr->expArea.y2 = Tk_Height(wPtr->dispData.tkwin) - 1;
}

/*----------------------------------------------------------------------
 * DItemSizeChanged --
 *
 *	This is called whenever the size of one of the HList's items
 *	changes its size.
 *----------------------------------------------------------------------
 */
static void
Tix_GrDItemSizeChanged(iPtr)
    Tix_DItem *iPtr;
{
    WidgetPtr wPtr = (WidgetPtr)iPtr->base.clientData;

    if (wPtr) {
	/* double-check: perhaps we haven't set the clientData yet! */
	Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);
    }
}

/*
 *----------------------------------------------------------------------
 * Tix_GrDoWhenIdle --
 *----------------------------------------------------------------------
 */
void
Tix_GrDoWhenIdle(wPtr, type)
    WidgetPtr wPtr;
    int type;
{
    switch (type) {
      case TIX_GR_RESIZE:
	wPtr->toResize = 1;
	break;
      case TIX_GR_REDRAW:
	wPtr->toRedraw = 1;
	break;
    }

    if (!wPtr->idleEvent) {
	wPtr->idleEvent = 1;
	Tk_DoWhenIdle(IdleHandler, (ClientData)wPtr);
    }
}

static void
IdleHandler(clientData)
    ClientData clientData;	/* Info about my widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    if (!wPtr->idleEvent) {	/* sanity check */
	return;
    }
    wPtr->idleEvent = 0;    

    if (wPtr->toResize) {
	wPtr->toResize = 0;
	WidgetComputeGeometry(clientData);
    }
    else if (wPtr->toRedraw) {
	wPtr->toRedraw = 0;
	WidgetDisplay(clientData);
    }
}

/*
 *----------------------------------------------------------------------
 * Tix_GrCancelDoWhenIdle --
 *----------------------------------------------------------------------
 */
void
Tix_GrCancelDoWhenIdle(wPtr)
    WidgetPtr wPtr;
{
    wPtr->toResize = 0;
    wPtr->toRedraw = 0;

    if (wPtr->idleEvent) {
	Tk_CancelIdleCall(IdleHandler, (ClientData)wPtr);
	wPtr->idleEvent = 0;
    }
}

/*----------------------------------------------------------------------
 * WidgetDisplay --
 *
 *	Display the widget: the borders, the background and the entries.
 *
 *----------------------------------------------------------------------
 */
static void
WidgetDisplay(clientData)
    ClientData clientData;	/* Info about my widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    Drawable buffer = None;
    Tk_Window tkwin = wPtr->dispData.tkwin;
    int winH, winW, expW, expH;
    GC highlightGC;

    if (!Tk_IsMapped(tkwin)) {
	return;
    }
    wPtr->serial ++;

    winW = Tk_Width(tkwin)  - 2*wPtr->highlightWidth - 2*wPtr->borderWidth;
    winH = Tk_Height(tkwin) - 2*wPtr->highlightWidth - 2*wPtr->borderWidth;

    if (winW <= 0 || winH <= 0) {	/* nothing to draw */
	goto done;
    }

    if (wPtr->toResetRB) {
	Tix_GrResetRenderBlocks(wPtr);
	wPtr->toResetRB = 0;
    }
    if (wPtr->toComputeSel) {
	Tix_GrComputeSelection(wPtr);
	wPtr->toComputeSel = 0;
    }

    /* clip the exposed area to the visible part of the widget,
     * just in case some of the routines had made it larger than
     * it should be
     */
    if (wPtr->expArea.x1 < wPtr->bdPad) {
	wPtr->expArea.x1 = wPtr->bdPad;
    }
    if (wPtr->expArea.y1 < wPtr->bdPad) {
	wPtr->expArea.y1 = wPtr->bdPad;
    }
    if (wPtr->expArea.x2 >= Tk_Width(tkwin)  - wPtr->bdPad) {
	wPtr->expArea.x2  = Tk_Width(tkwin)  - wPtr->bdPad - 1;
    }
    if (wPtr->expArea.y2 >= Tk_Height(tkwin) - wPtr->bdPad) {
	wPtr->expArea.y2  = Tk_Height(tkwin) - wPtr->bdPad - 1;
    }

    expW = wPtr->expArea.x2 - wPtr->expArea.x1 + 1;
    expH = wPtr->expArea.y2 - wPtr->expArea.y1 + 1;

    if (expW <= 0 || expH <= 0) {	/* no cells to draw */
	goto drawBorder;
    }

    buffer = Tix_GetRenderBuffer(wPtr->dispData.display, Tk_WindowId(tkwin),
	expW, expH, Tk_Depth(tkwin));

    if (buffer == Tk_WindowId(tkwin)) {
	/* clear the window directly */
	XFillRectangle(wPtr->dispData.display, buffer, wPtr->backgroundGC,
		wPtr->expArea.x1, wPtr->expArea.y1,
		(unsigned) expW, (unsigned) expH);
    } else {
	XFillRectangle(wPtr->dispData.display, buffer, wPtr->backgroundGC,
		0, 0, (unsigned) expW, (unsigned) expH);
    }

    if (wPtr->mainRB) {
	Tix_GrDisplayMainBody(wPtr, buffer, winW, winH);
    }

    if (buffer != Tk_WindowId(tkwin)) {
	XCopyArea(wPtr->dispData.display, buffer, Tk_WindowId(tkwin),
	    wPtr->backgroundGC, 0, 0, (unsigned) expW, (unsigned) expH,
	    wPtr->expArea.x1, wPtr->expArea.y1);
	Tk_FreePixmap(wPtr->dispData.display, buffer);
    }

  drawBorder:
    Tk_Draw3DRectangle(tkwin, Tk_WindowId(tkwin), wPtr->border,
	wPtr->highlightWidth,
	wPtr->highlightWidth,
	Tk_Width(tkwin)  - 2*wPtr->highlightWidth, 
	Tk_Height(tkwin) - 2*wPtr->highlightWidth,
	wPtr->borderWidth, wPtr->relief);

    if (wPtr->toRedrawHighlight && wPtr->highlightWidth > 0) {
	if (wPtr->hasFocus) {
	    highlightGC = wPtr->highlightGC;
	} else {
	    highlightGC = Tk_3DBorderGC(tkwin, wPtr->border, 
	        TK_3D_FLAT_GC);
	}

	Tk_DrawFocusHighlight(tkwin, highlightGC, wPtr->highlightWidth,
	    Tk_WindowId(tkwin));
    }

  done:
    wPtr->expArea.x1 = 10000;
    wPtr->expArea.y1 = 10000;
    wPtr->expArea.x2 = 0;
    wPtr->expArea.y2 = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_GrDisplayMainBody  --
 *
 *	Draw the background and cells
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void Tix_GrDisplayMainBody(wPtr, buffer, winW, winH)
    WidgetPtr wPtr;
    Drawable buffer;
    int winW;
    int winH;
{
    Tk_Window tkwin = wPtr->dispData.tkwin;
    RenderInfo mainRI;			/* render info for main body */
    int i, j;

    if (buffer == Tk_WindowId(tkwin)) {
	/* rendering directly into the window */
	mainRI.origin[0] = wPtr->highlightWidth + wPtr->borderWidth;
	mainRI.origin[1] = wPtr->highlightWidth + wPtr->borderWidth;

    } else {
	/* rendering into a pixmap */
	mainRI.origin[0] = wPtr->highlightWidth + wPtr->borderWidth
	    - wPtr->expArea.x1;
	mainRI.origin[1] = wPtr->highlightWidth + wPtr->borderWidth
	    - wPtr->expArea.y1;
    }

    mainRI.drawable = buffer;
    wPtr->colorInfoCounter ++;

    wPtr->renderInfo = &mainRI;

    /* 1. Draw the backgrounds
     */
    for (i=0; i<wPtr->mainRB->size[0]; i++) {
	for (j=0; j<wPtr->mainRB->size[1]; j++) {
	    wPtr->mainRB->elms[i][j].borderW[0][0] = 0;
	    wPtr->mainRB->elms[i][j].borderW[1][0] = 0;
	    wPtr->mainRB->elms[i][j].borderW[0][1] = 0;
	    wPtr->mainRB->elms[i][j].borderW[1][1] = 0;
	    wPtr->mainRB->elms[i][j].filled        = 0;
	}
    }
    Tix_GrDrawBackground(wPtr, &mainRI, buffer);

    /* 2. Draw the cells
     */
    Tix_GrDrawCells(wPtr, &mainRI, buffer);

    /* 3. Draw the special sites (anchor, drag, drop).
     */
    Tix_GrDrawSites(wPtr, &mainRI, buffer);

    /* done */
    wPtr->renderInfo = NULL;

    /* Free the unwanted colors: they are left overs from the "format"
     * widget command.
     */
    Tix_GrFreeUnusedColors(wPtr, 0);
}

/*----------------------------------------------------------------------
 * Tix_GrDrawCells --
 *
 *	Redraws the cells of the Grid
 *----------------------------------------------------------------------
 */
static void Tix_GrDrawCells(wPtr, riPtr, drawable)
    WidgetPtr wPtr;
    RenderInfo * riPtr;
    Drawable drawable;
{
    int x, y, i, j;
    int x1, y1, x2, y2;
    TixGrEntry * chPtr;
    int margin = wPtr->borderWidth + wPtr->highlightWidth;

    for (x=0,i=0; i<wPtr->mainRB->size[0]; i++) {
	x1 = x  + margin;
	x2 = x1 - 1 + wPtr->mainRB->dispSize[0][i].total;

	if (x1 > wPtr->expArea.x2) {
	    goto nextCol;
	}
	if (x2 < wPtr->expArea.x1) {
	    goto nextCol;
	}
	/*
	 * iterate over the columns
	 */
	for (y=0,j=0; j<wPtr->mainRB->size[1]; j++) {
	    /*
	     * iterate over each item in the column, from top
	     * to bottom
	     */
	    y1 = y  + margin;
	    y2 = y1 - 1 + wPtr->mainRB->dispSize[1][j].total;

	    if (y1 > wPtr->expArea.y2) {
		goto nextRow;
	    }
	    if (y2 < wPtr->expArea.y1) {
		goto nextRow;
	    }
	    if (!wPtr->mainRB->elms[i][j].filled) {
		if (wPtr->mainRB->elms[i][j].selected) {

		    Tk_Fill3DRectangle(wPtr->dispData.tkwin,
		    	drawable, wPtr->selectBorder,
			x+riPtr->origin[0]+
			    wPtr->mainRB->elms[i][j].borderW[0][0],
			y+riPtr->origin[1]+
			    wPtr->mainRB->elms[i][j].borderW[1][0],
			wPtr->mainRB->dispSize[0][i].total - 
			    wPtr->mainRB->elms[i][j].borderW[0][0] -
			    wPtr->mainRB->elms[i][j].borderW[0][1],
			wPtr->mainRB->dispSize[1][j].total - 
			    wPtr->mainRB->elms[i][j].borderW[1][0] -
			    wPtr->mainRB->elms[i][j].borderW[1][1],
		    	0, TK_RELIEF_FLAT);
		}
	    }

	    chPtr = wPtr->mainRB->elms[i][j].chPtr;
	    if (chPtr != NULL) {
		if (Tix_DItemType(chPtr->iPtr) == TIX_DITEM_WINDOW) {
		    Tix_DItemDisplay(Tk_WindowId(wPtr->dispData.tkwin),
			chPtr->iPtr, x1, y1, 
			wPtr->mainRB->dispSize[0][i].size,
	       		wPtr->mainRB->dispSize[1][j].size, 0, 0,
		        TIX_DITEM_NORMAL_FG);
		} else {
		    int drawX, drawY;
		    drawX = x + riPtr->origin[0] +
		        wPtr->mainRB->dispSize[0][i].preBorder;
		    drawY = y + riPtr->origin[1] +
		        wPtr->mainRB->dispSize[1][j].preBorder;

		    Tix_DItemDisplay(drawable, chPtr->iPtr,
		    	drawX, drawY,
			wPtr->mainRB->dispSize[0][i].size,
			wPtr->mainRB->dispSize[1][j].size, 0, 0,
			TIX_DITEM_NORMAL_FG);
		}
	    }
	  nextRow:
	    y+= wPtr->mainRB->dispSize[1][j].total;
	}
      nextCol:
	x+= wPtr->mainRB->dispSize[0][i].total;
    }
 
    for (i=0; i<wPtr->mainRB->size[0]; i++) {
	for (j=0; j<wPtr->mainRB->size[1]; j++) {
	    chPtr = wPtr->mainRB->elms[i][j].chPtr;
	    if (chPtr != NULL) {
		if (Tix_DItemType(chPtr->iPtr) == TIX_DITEM_WINDOW) {

		    Tix_SetWindowItemSerial(&wPtr->mappedWindows,
			chPtr->iPtr, wPtr->serial);
		}
	    }
	}
    }

    /* unmap those windows we mapped the last time */
    Tix_UnmapInvisibleWindowItems(&wPtr->mappedWindows, wPtr->serial);
}

/*----------------------------------------------------------------------
 * Tix_GrDrawSites --
 *
 *	Redraws the special sites (anchor, drag, drop)
 *----------------------------------------------------------------------
 */
static void Tix_GrDrawSites(wPtr, riPtr, drawable)
    WidgetPtr wPtr;
    RenderInfo * riPtr;
    Drawable drawable;
{
    int rect[2][2];
    int visible;

    visible = Tix_GrGetElementPosn(wPtr, wPtr->anchor[0], wPtr->anchor[1],
	rect, 0, 1, 0, 0);
    if (!visible) {
	return;
    }

    Tix_DrawAnchorLines(Tk_Display(wPtr->dispData.tkwin), drawable,
	wPtr->anchorGC,
	rect[0][0] + riPtr->origin[0],
        rect[1][0] + riPtr->origin[1],
	rect[0][1] - rect[0][0] + 1,
	rect[1][1] - rect[1][0] + 1);
}

/*----------------------------------------------------------------------
 *
 * Tix_GrGetElementPosn --
 *
 *	Returns the position of a visible element on the screen.
 *
 * Arguments
 *	x,y:	index of the element.
 *      rect:	stores the return values: four sides of the cell.
 *	clipOK:	if true and element is only partially visible, return only
 *		the visible portion.
 *	isSite:	if (x,y) is a site, the return value depends on the
 *		selectUnit variable.
 *	isScr:  should we return the position within the widget (true)
 *		or within the main display area (false).
 *	nearest:if the element is outside of the widget, should we return
 *		the position of the nearest element?
 *
 *----------------------------------------------------------------------
 */

int
Tix_GrGetElementPosn(wPtr, x, y, rect, clipOK, isSite, isScr, nearest)
    WidgetPtr wPtr;
    int x;
    int y;
    int rect[2][2];
    int clipOK;		/* %% ignored */
    int isSite;
    int isScr;
    int nearest;
{
    int i, j, pos[2];
    int axis;
    int useAxis;

    if (wPtr->selectUnit == tixRowUid) {
	axis = 0;
	useAxis = 1;
    }
    else if (wPtr->selectUnit == tixColumnUid) {
	axis = 1;
	useAxis = 1;
    }
    else {
	axis = 0; /* lint */
	useAxis = 0;
    }

    /* %% didn't take care of the headers, etc */

    pos[0] = x;
    pos[1] = y;

    /* clip the anchor site with the visible cells */
    for (i=0; i<2; i++) {
	if (pos[i] == TIX_SITE_NONE) {
	    return 0;
	}

	if (isSite && useAxis && i == axis) {
	    rect[i][0] = 0;
	    rect[i][1] = wPtr->mainRB->visArea[i]-1;
	} else {
	    if (pos[i] >= wPtr->hdrSize[i]) {
		pos[i] -= wPtr->scrollInfo[i].offset;
		if (pos[i] < wPtr->hdrSize[i]) {
		    /* This cell has been scrolled "under the margins" */
		    return 0;
		}
	    }

	    if (pos[i] < 0) {
		if (!nearest) {
		    return 0;
		}
		pos[i] = 0;
	    }
	    if (pos[i] >= wPtr->mainRB->size[i]) {
		if (!nearest) {
		    return 0;
		}
		pos[i] = wPtr->mainRB->size[i] - 1;
	    }
	    rect[i][0] = 0;
	    for (j=0; j<pos[i]; j++) {
		rect[i][0] += wPtr->mainRB->dispSize[i][j].total;
	    }
	    rect[i][1] = rect[i][0] + wPtr->mainRB->dispSize[i][j].total - 1;
	}
    }

    if (isScr) {
	rect[0][0] += wPtr->bdPad;
	rect[1][0] += wPtr->bdPad;
	rect[0][1] += wPtr->bdPad;
	rect[1][1] += wPtr->bdPad;
    }

    return 1;
}

/*----------------------------------------------------------------------
 *
 * "bdtype" sub command --
 *
 *	Returns if the the screen position is at a border. This is useful
 *	for changing the mouse cursor when the user points at a border
 *	area. This indicates that the border can be adjusted interactively.
 *
 *----------------------------------------------------------------------
 */
static int
Tix_GrBdType(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    Tk_Window tkwin = wPtr->dispData.tkwin;
    int i, k, screenPos[2], bd[2], pos[2], in[2], bdWidth[2];
    char buf[100];
    int inX = 0;
    int inY = 0;

    if (argc != 2 && argc != 4) {
	return Tix_ArgcError(interp, argc+2, argv-2, 2,
	    "x y ?xbdWidth ybdWidth?");
    }

    if (Tcl_GetInt(interp, argv[0], &screenPos[0]) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &screenPos[1]) != TCL_OK) {
	return TCL_ERROR;
    }
    if (argc == 4) {
	if (Tcl_GetInt(interp, argv[2], &bdWidth[0]) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[3], &bdWidth[1]) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	bdWidth[0] = -1;
	bdWidth[1] = -1;
    }

    if (!Tk_IsMapped(tkwin)) {
	Tcl_ResetResult(interp);
	return TCL_OK;
    }

    if (wPtr->mainRB == NULL || wPtr->toResetRB) {
	Tix_GrResetRenderBlocks(wPtr);
	wPtr->toResetRB = 0;
    }

    screenPos[0] -= wPtr->highlightWidth - wPtr->borderWidth;
    screenPos[1] -= wPtr->highlightWidth - wPtr->borderWidth;

    for (i=0; i<2; i++) {
	bd[i]  = -1;
	pos[i] = 0;
	in[i]  = 0;
	for (k=0; k<wPtr->mainRB->size[i]; k++) {
	    ElmDispSize * elm = &wPtr->mainRB->dispSize[i][k];
	    if (screenPos[i] - elm->total <= 0) {
		if (bdWidth[i] != -1) {
		    if (screenPos[i] < bdWidth[i]) {
			bd[i]  = k - 1;
			pos[i] = k;
		    }
		    else if ((elm->total - screenPos[i]) <= bdWidth[i]) {
			bd[i]  = k;
			pos[i] = k + 1;
		    }
		    else {
			pos[i] = k;
		    }
		} else {
		    if (screenPos[i] < elm->preBorder) {
			bd[i]  = k - 1;
			pos[i] = k;
		    }
		    else if ((screenPos[i] - elm->preBorder - elm->size)>= 0) {
			bd[i]  = k;
			pos[i] = k + 1;
		    }
		    else {
			pos[i] = k;
		    }
		}
		in[i] = k;
		break;
	    } else {
		screenPos[i] -= elm->total;
	    }
	}
    }

    if (in[0] < wPtr->hdrSize[0] && bd[1] >= 0) {
	inY = 1;
    }
    else if (in[1] < wPtr->hdrSize[1] && bd[0] >= 0) {
	inX = 1;
    }
    
    if (bd[0] < 0) {
	bd[0] = 0;
    }
    if (bd[1] < 0) {
	bd[1] = 0;
    }

    if (inX && inY) {
	sprintf(buf, "xy %d %d", bd[0], bd[1]);
    } else if (inX) {
	sprintf(buf, "x %d %d", bd[0], bd[1]);
    } else if (inY) {
	sprintf(buf, "y %d %d", bd[0], bd[1]);
    } else {
	buf[0] = '\0';
    }

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, buf, NULL);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "set" sub command -- 
 *
 *	Sets the item at the position on the grid. This either creates
 *	a new element or modifies the existing element. (if you don't want
 *	to change the -itemtype of the existing element, it will be more
 *	efficient to call the "itemconfigure" command).
 *----------------------------------------------------------------------
 */
static int
Tix_GrSet(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    TixGrEntry * chPtr = NULL;
    Tix_DItem * iPtr;
    int x, y;
    CONST84 char * ditemType;
    int code = TCL_OK;

    /*------------------------------------------------------------
     * (0) We need to find out where you want to set
     *------------------------------------------------------------
     */
    if (TixGridDataGetIndex(interp, wPtr, argv[0], argv[1], &x, &y)!=TCL_OK) {
	return TCL_ERROR;
    }

    /*------------------------------------------------------------
     * (1) We need to determine the option: -itemtype.
     *------------------------------------------------------------
     */
    /* (1.0) Find out the -itemtype, if specified */
    ditemType = wPtr->diTypePtr->name; 	 /* default value */
    if (argc > 2) {
	size_t len;
	int i;
	if (argc %2 != 0) {
	    Tcl_AppendResult(interp, "value for \"", argv[argc-1],
		"\" missing", NULL);
	    code = TCL_ERROR;
	    goto done;
	}
	for (i=2; i<argc; i+=2) {
	    len = strlen(argv[i]);
	    if (strncmp(argv[i], "-itemtype", len) == 0) {
		ditemType = argv[i+1];
	    }
	}
    }

    if (Tix_GetDItemType(interp, ditemType) == NULL) {
	code = TCL_ERROR;
	goto done;
    }

    /*
     * (2) Get this item (a new item will be allocated if it does not exist
     *     yet)
     */
    chPtr = Tix_GrFindCreateElem(interp, wPtr, x, y);

    /* (2.1) The Display item data */
    if ((iPtr = Tix_DItemCreate(&wPtr->dispData, ditemType)) == NULL) {
	code = TCL_ERROR;
	goto done;
    }
    iPtr->base.clientData = (ClientData)wPtr;     /* %%%% */

    if (chPtr->iPtr) {
	Tix_DItemFree(chPtr->iPtr);
    }
    chPtr->iPtr = iPtr;

    if (ConfigElement(wPtr, chPtr, argc-2, argv+2, 0, 1) != TCL_OK) {
	code = TCL_ERROR; goto done;
    }
    Tix_GrPropagateSize(wPtr, chPtr);

  done:
    if (code == TCL_ERROR) {
	/* ? */
    } else {
	Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);
    }

    return code;
}

/*----------------------------------------------------------------------
 * "unset" sub command
 *----------------------------------------------------------------------
 */

static int
Tix_GrUnset(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    TixGrEntry * chPtr;
    int x, y;

    if (TixGridDataGetIndex(interp, wPtr, argv[0], argv[1], &x, &y)
	    !=TCL_OK) {
	return TCL_ERROR;
    }

    chPtr = Tix_GrFindElem(interp, wPtr, x, y);
    if (chPtr != NULL) {
	TixGridDataDeleteEntry(wPtr->dataSet, x, y);
	Tix_GrFreeElem(chPtr);
	Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);
    }
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "cget" sub command --
 *----------------------------------------------------------------------
 */
static int
Tix_GrCGet(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    return Tk_ConfigureValue(interp, wPtr->dispData.tkwin, configSpecs,
	(char *)wPtr, argv[0], 0);
}

/*----------------------------------------------------------------------
 * "configure" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrConfig(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    if (argc == 0) {
	return Tk_ConfigureInfo(interp, wPtr->dispData.tkwin, configSpecs,
	    (char *) wPtr, (char *) NULL, 0);
    } else if (argc == 1) {
	return Tk_ConfigureInfo(interp, wPtr->dispData.tkwin, configSpecs,
	    (char *) wPtr, argv[0], 0);
    } else {
	return WidgetConfigure(interp, wPtr, argc, argv,
	    TK_CONFIG_ARGV_ONLY);
    }
}

/*----------------------------------------------------------------------
 * "delete" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrDelete(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int from, to, which;

    if (TranslateFromTo(interp, wPtr, argc, argv, &from, &to, &which)!=TCL_OK){
	return TCL_ERROR;
    }
    TixGridDataDeleteRange(wPtr, wPtr->dataSet, which, from, to);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "edit" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrEdit(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int x, y;
    Tcl_DString dstring;
    char buff[20];
    size_t len;
    int code;

    len = strlen(argv[0]);
    if (strncmp(argv[0], "set", len) == 0) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # of arguments, must be: ",
		argv[-2], " edit set x y", NULL);
	}
	if (TixGridDataGetIndex(interp, wPtr, argv[1], argv[2], &x, &y)
		!=TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_DStringInit(&dstring);

	Tcl_DStringAppendElement(&dstring, "tixGrid:EditCell");
	Tcl_DStringAppendElement(&dstring, Tk_PathName(wPtr->dispData.tkwin));
	sprintf(buff, "%d", x);
	Tcl_DStringAppendElement(&dstring, buff);
	sprintf(buff, "%d", y);
	Tcl_DStringAppendElement(&dstring, buff);
    } else if (strncmp(argv[0], "apply", len) == 0) {
	if (argc != 1) {
	    Tcl_AppendResult(interp, "wrong # of arguments, must be: ",
		argv[-2], " edit apply", NULL);
	}
	Tcl_DStringInit(&dstring);

	Tcl_DStringAppendElement(&dstring, "tixGrid:EditApply");
	Tcl_DStringAppendElement(&dstring, Tk_PathName(wPtr->dispData.tkwin));
    } else {
	Tcl_AppendResult(interp, "unknown option \"", argv[0],
		"\", must be apply or set", NULL);
	return TCL_ERROR;
    }

    code = Tcl_GlobalEval(interp, dstring.string);
    Tcl_DStringFree(&dstring);

    return code;
}

/*----------------------------------------------------------------------
 * "entrycget" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrEntryCget(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int x, y;
    TixGrEntry * chPtr;

    if (TixGridDataGetIndex(interp, wPtr, argv[0], argv[1], &x, &y)!=TCL_OK) {
	return TCL_ERROR;
    }

    chPtr = Tix_GrFindElem(interp, wPtr, x, y);
    if (!chPtr) {
	Tcl_AppendResult(interp, "entry \"", argv[0], ",", argv[1],
	    "\" does not exist", NULL);
	return TCL_ERROR;
    }

    return Tix_ConfigureValue2(interp, wPtr->dispData.tkwin, (char *)chPtr,
	entryConfigSpecs, chPtr->iPtr, argv[2], 0);
}

/*----------------------------------------------------------------------
 * "entryconfigure" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrEntryConfig(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int x, y;
    TixGrEntry * chPtr;

    if (TixGridDataGetIndex(interp, wPtr, argv[0], argv[1], &x, &y)!=TCL_OK) {
	return TCL_ERROR;
    }

    chPtr = Tix_GrFindElem(interp, wPtr, x, y);
    if (!chPtr) {
	Tcl_AppendResult(interp, "entry \"", argv[0], ",", argv[1],
	    "\" does not exist", NULL);
	return TCL_ERROR;
    }

    if (argc == 2) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)chPtr, entryConfigSpecs, chPtr->iPtr, (char *) NULL, 0);
    } else if (argc == 3) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)chPtr, entryConfigSpecs, chPtr->iPtr, (char *) argv[2], 0);
    } else {
	return ConfigElement(wPtr, chPtr, argc-2, argv+2,
	    TK_CONFIG_ARGV_ONLY, 0);
    }
}

/*----------------------------------------------------------------------
 * "geometryinfo" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrGeometryInfo(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int qSize[2];
    double first[2], last[2];
    char string[80];
    int i;
    Tix_GridScrollInfo scrollInfo[2];

    if (argc == 2) {
	if (Tcl_GetInt(interp, argv[0], &qSize[0]) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[1], &qSize[1]) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	qSize[0] = Tk_Width (wPtr->dispData.tkwin);
	qSize[1] = Tk_Height(wPtr->dispData.tkwin);
    }
    qSize[0] -= 2*wPtr->borderWidth + 2*wPtr->highlightWidth;
    qSize[1] -= 2*wPtr->borderWidth + 2*wPtr->highlightWidth;

    RecalScrollRegion(wPtr, qSize[0], qSize[1], scrollInfo);

    for (i=0; i<2; i++) {
	qSize[i] -= 2*wPtr->borderWidth + 2*wPtr->highlightWidth;
	GetScrollFractions(wPtr, &scrollInfo[i],
	    &first[i], &last[i]);
    }

    sprintf(string, "{%f %f} {%f %f}", first[0], last[0], first[1], last[1]);
    Tcl_AppendResult(interp, string, NULL);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "index" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrIndex(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int x, y;
    char buff[100];

    if (TixGridDataGetIndex(interp, wPtr, argv[0], argv[1], &x, &y)!=TCL_OK) {
	return TCL_ERROR;
    }

    sprintf(buff, "%d %d", x, y);
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, buff, NULL);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "info" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrInfo(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    size_t len = strlen(argv[0]);
    int x, y;

    if (strncmp(argv[0], "bbox", len)==0) {
	if (argc != 3) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "x y");
	}
	if (TixGridDataGetIndex(interp, wPtr, argv[1], argv[2], &x, &y)
		!=TCL_OK) {
	    return TCL_ERROR;
	}

	return Tix_GrBBox(interp, wPtr, x, y);
    }
    else if (strncmp(argv[0], "exists", len)==0) {
	if (argc != 3) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "x y");
	}
	if (TixGridDataGetIndex(interp, wPtr, argv[1], argv[2], &x, &y)
		!=TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tix_GrFindElem(interp, wPtr, x, y)) {
	    Tcl_SetResult(interp, "1", TCL_STATIC);
	} else {
	    Tcl_SetResult(interp, "0", TCL_STATIC);
	}
	return TCL_OK;
    }
    else {
	Tcl_AppendResult(interp, "unknown option \"", argv[0], 
	    "\": must be bbox or exists",
	    NULL);
	return TCL_ERROR;
    }
}

/*----------------------------------------------------------------------
 * "move" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrMove(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int from, to, which, by;

    if (TranslateFromTo(interp, wPtr, 3, argv, &from, &to, &which)!=TCL_OK){
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &by) != TCL_OK) {
	return TCL_ERROR;
    }

TixGridDataMoveRange(wPtr, wPtr->dataSet, which, from, to, by);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "nearest" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrNearest(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    Tk_Window tkwin = wPtr->dispData.tkwin;
    int i, k, screenPos[2], rbPos[2];
    char buf[100];
    RenderBlockElem* rePtr;

    if (Tcl_GetInt(interp, argv[0], &screenPos[0]) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &screenPos[1]) != TCL_OK) {
	return TCL_ERROR;
    }

    if (!Tk_IsMapped(tkwin)) {
	Tcl_ResetResult(interp);
	return TCL_OK;
    }

    if (wPtr->mainRB == NULL || wPtr->toResetRB) {
	Tix_GrResetRenderBlocks(wPtr);
	wPtr->toResetRB = 0;
    }

    screenPos[0] -= wPtr->highlightWidth - wPtr->borderWidth;
    screenPos[1] -= wPtr->highlightWidth - wPtr->borderWidth;

    for (i=0; i<2; i++) {
	for (k=0; k<wPtr->mainRB->size[i]; k++) {
	    screenPos[i] -=  wPtr->mainRB->dispSize[i][k].total;
	    if (screenPos[i]<=0) {
		break;
	    }
	}
	if (k >= wPtr->mainRB->size[i]) {
	    k = wPtr->mainRB->size[i] - 1;
	}
	rbPos[i] = k;
    }
    rePtr = &(wPtr->mainRB->elms[rbPos[0]][rbPos[1]]);

    sprintf(buf, "%d %d", rePtr->index[0], rePtr->index[1]);
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, buf, NULL);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "anchor", "dragsite" and "dropsire" sub commands --
 *
 *	Set/remove the anchor element
 *----------------------------------------------------------------------
 */
static int
Tix_GrSetSite(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    int changed = 0;
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int * changePtr;
    size_t len;
    int changedRect[2][2];

    /*
     * Determine which site should be changed (the last else clause
     * doesn't need to check the string because HandleSubCommand
     * already ensures that only the valid options can be specified.
     */
    len = strlen(argv[-1]);
    if (strncmp(argv[-1], "anchor", len)==0) {
	changePtr = wPtr->anchor;
    }
    else if (strncmp(argv[-1], "dragsite", len)==0) {
	changePtr = wPtr->dragSite;
    }
    else {
	changePtr = wPtr->dropSite;
    }

    len = strlen(argv[0]);
    if (strncmp(argv[0], "get", len)==0) {
	char buf[100];

	sprintf(buf, "%d %d", changePtr[0], changePtr[1]);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);

	return TCL_OK;
    } else if (strncmp(argv[0], "set", len)==0) {
	if (argc == 3) {
	    int x, y;

	    if (TixGridDataGetIndex(interp, wPtr, argv[1], argv[2],
		     &x, &y)!=TCL_OK) {
		return TCL_ERROR;
	    }
	    if (x != changePtr[0] || y != changePtr[1]) {
		changedRect[0][0] = x;
		changedRect[1][0] = y;
		changedRect[0][1] = changePtr[0];
		changedRect[1][1] = changePtr[1];
		changed = 1;

		changePtr[0] = x;
		changePtr[1] = y;
	    }
	} else {
	    Tcl_AppendResult(interp, "wrong # of arguments, must be: ",
		Tk_PathName(wPtr->dispData.tkwin), " ", argv[-1],
		" set x y", NULL);
	    return TCL_ERROR;
	}
    }
    else if (strncmp(argv[0], "clear", len)==0) {
	if (argc == 1) {
	    if (changePtr[0] !=TIX_SITE_NONE || changePtr[1] !=TIX_SITE_NONE) {
		changedRect[0][0] = TIX_SITE_NONE;
		changedRect[1][0] = TIX_SITE_NONE;
		changedRect[0][1] = changePtr[0];
		changedRect[1][1] = changePtr[1];
		changed = 1;

		changePtr[0] = TIX_SITE_NONE;
		changePtr[1] = TIX_SITE_NONE;
	    }
	} else {
	    Tcl_AppendResult(interp, "wrong # of arguments, must be: ",
		Tk_PathName(wPtr->dispData.tkwin), " ", argv[-1],
		" clear", NULL);
	    return TCL_ERROR;
	}
    }
    else {
	Tcl_AppendResult(interp, "wrong option \"", argv[0], "\", ",
	    "must be clear, get or set", NULL);
	return TCL_ERROR;
    }

    if (changed) {
	Tix_GrAddChangedRect(wPtr, changedRect, 1);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * Tix_GrAddChangedRect --
 *
 *	Add the "changed" region to the exposedArea structure.
 *----------------------------------------------------------------------
 */
void
Tix_GrAddChangedRect(wPtr, changedRect, isSite)
    WidgetPtr wPtr;
    int changedRect[2][2];
    int isSite;
{
    int rect[2][2];
    int visible;
    int i;
    int changed = 0;

    if (wPtr->mainRB == NULL) {
	/*
	 * The grid will be completely refreshed. Don't do anything
	 */
	return;
    }

    for (i=0; i<2; i++) {
	visible = Tix_GrGetElementPosn(wPtr, changedRect[0][i],
	    changedRect[1][i], rect, 1, isSite, 1, 1);
	if (!visible) {
	    continue;
	}
	if (wPtr->expArea.x1 > rect[0][0]) {
	    wPtr->expArea.x1 = rect[0][0];
	    changed = 1;
	}
	if (wPtr->expArea.x2 < rect[0][1]) {
	    wPtr->expArea.x2 = rect[0][1];
	    changed = 1;
	}
	if (wPtr->expArea.y1 > rect[1][0]) {
	    wPtr->expArea.y1 = rect[1][0];
	    changed = 1;
	}
	if (wPtr->expArea.y2 < rect[1][1]) {
	    wPtr->expArea.y2 = rect[1][1];
	    changed = 1;
	}
    }
    if (changed) {
	Tix_GrDoWhenIdle(wPtr, TIX_GR_REDRAW);
    }
}

void Tix_GrScrollPage(wPtr, count, axis)
    WidgetPtr wPtr;
    int count;
    int axis;
{
    int k, i = axis;
    int winSize, sz, start, num;
    int pad0, pad1; 

    Tix_GridScrollInfo * siPtr = &wPtr->scrollInfo[axis];
    int gridSize[2];

    if (count == 0) {
	return;
    }

    TixGridDataGetGridSize(wPtr->dataSet, &gridSize[0],
	&gridSize[1]);

    if (gridSize[i] < wPtr->hdrSize[i]) {	 /* no scrollable data */
	return;
    }

    if (axis == 0) {
	winSize = Tk_Width(wPtr->dispData.tkwin);
    } else {
	winSize = Tk_Height(wPtr->dispData.tkwin);
    }
    winSize -= 2*wPtr->highlightWidth + 2*wPtr->borderWidth;

    for (k=0; k<wPtr->hdrSize[i] && k<gridSize[i]; k++) {
	winSize -= TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		k, &wPtr->defSize[i], &pad0, &pad1);
	winSize -= pad0 + pad1;
    }

    if (winSize <= 0) {
	return;
    }

    if (count > 0) {
	start = siPtr->offset + wPtr->hdrSize[i];
	for (; count > 0; count--) {
	    sz = winSize;

	    for (num=0,k=start; k<gridSize[i]; k++,num++) {
		sz -= TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		    k, &wPtr->defSize[i], &pad0, &pad1);
		sz -= pad0 + pad1;
		if (sz == 0) {
		    num++;
		    break;
		}
		if (sz < 0) {
		    break;
		}
	    }
	    if (num==0) {
		num++;
	    }
	    start += num;
	}
	siPtr->offset = start - wPtr->hdrSize[i];
    }
    else {
	start = siPtr->offset + wPtr->hdrSize[i];

	for (; count < 0; count++) {
	    sz = winSize;

	    for (num=0,k=start-1; k>=wPtr->hdrSize[i]; k--,num++) {
		sz -= TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		    k, &wPtr->defSize[i], &pad0, &pad1);
		sz -= pad0 + pad1;
		if (sz == 0) {
		    num++;
		    break;
		}
		if (sz < 0) {
		    break;
		}
	    }
	    if (num==0) {
		num++;
	    }
	    start -= num;
	}
	siPtr->offset = start - wPtr->hdrSize[i];
    }
}

/*----------------------------------------------------------------------
 * "xview" and "yview" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_GrView(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int axis, oldXOff, oldYOff;
    Tix_GridScrollInfo * siPtr;

    if (argv[-1][0] == 'x') {
	axis = 0;
    } else {
	axis = 1;
    }

    oldXOff = wPtr->scrollInfo[0].offset;
    oldYOff = wPtr->scrollInfo[1].offset;

    if (argc == 0) {
	char string[100];
	double first, last;

	GetScrollFractions(wPtr, &wPtr->scrollInfo[axis], &first, &last);
	sprintf(string, "%f %f", first, last);
	Tcl_AppendResult(interp, string, NULL);
	return TCL_OK;
    }
    else {
	int offset;
	siPtr = &wPtr->scrollInfo[axis];

	if (Tcl_GetInt(interp, argv[0], &offset) == TCL_OK) {
	    /* backward-compatible mode */
	    siPtr->offset = offset;
	} else {
	    int type, count;
	    double fraction;

	    Tcl_ResetResult(interp);

	    /* Tk_GetScrollInfo () wants strange argc,argv combinations .. */
	    type = Tk_GetScrollInfo(interp, argc+2, argv-2, &fraction, &count);

	    switch (type) {
	      case TK_SCROLL_ERROR:
		return TCL_ERROR;

	      case TK_SCROLL_MOVETO:
		if (siPtr->window < 1.0) {
		    fraction /= (1.0 - siPtr->window);
		}

		siPtr->offset = (int)(fraction * (siPtr->max+1));
		break;

	      case TK_SCROLL_PAGES:
		Tix_GrScrollPage(wPtr, count, axis);
		break;

	      case TK_SCROLL_UNITS:
		siPtr->offset += count * siPtr->unit;
		break;
	    }
	}
	/* check ... */
	if (siPtr->offset < 0) {
	    siPtr->offset  = 0;
	}
	if (siPtr->offset > siPtr->max) {
	    siPtr->offset = siPtr->max;
	}
    }

#if 0
    printf("Configing Scrollbars: (%d %f %d) (%d %f %d)\n", 
        wPtr->scrollInfo[0].max,
        wPtr->scrollInfo[0].window,
        wPtr->scrollInfo[0].offset,
        wPtr->scrollInfo[1].max,
        wPtr->scrollInfo[1].window,
        wPtr->scrollInfo[1].offset);
#endif

    if (oldXOff != wPtr->scrollInfo[0].offset ||
	oldYOff != wPtr->scrollInfo[1].offset) {
	wPtr->toResetRB    = 1;
	wPtr->toComputeSel = 1;
	Tix_GrDoWhenIdle(wPtr, TIX_GR_REDRAW);
    }
    return TCL_OK;
}
/*----------------------------------------------------------------------
 *
 *
 * 			Memory Management Section
 *
 *
 *----------------------------------------------------------------------
 */
static int
ConfigElement(wPtr, chPtr, argc, argv, flags, forced)
    WidgetPtr wPtr;
    TixGrEntry *chPtr;
    int argc;
    CONST84 char ** argv;
    int flags;
    int forced;
{
    int sizeChanged;

    if (Tix_WidgetConfigure2(wPtr->dispData.interp, wPtr->dispData.tkwin,
	(char*)chPtr, entryConfigSpecs, chPtr->iPtr, argc, argv, flags,
	forced, &sizeChanged) != TCL_OK) {
	return TCL_ERROR;
    }

    if (sizeChanged) {
	/* %% be smart here: sometimes the size request doesn't need to
	 * be changed
	 */
	Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);
    } else {
	/* set the exposed area */
	Tix_GrDoWhenIdle(wPtr, TIX_GR_REDRAW);
    }
    return TCL_OK;
}

static CONST84 char * areaNames[4] = {
    "s-margin",
    "x-margin",
    "y-margin",
    "main"
};

static int
Tix_GrCallFormatCmd(wPtr, which)
    WidgetPtr wPtr;
    int which;
{
#define STATIC_SPACE_SIZE (128 + TCL_INTEGER_SPACE *4)
    unsigned int size;
    int code;
    char buff[STATIC_SPACE_SIZE];
    char * cmd = buff;

    size = strlen(wPtr->formatCmd) + 10 + (TCL_INTEGER_SPACE *4) + 10;
    if (size > STATIC_SPACE_SIZE) {
	cmd = (char*)ckalloc(size);
    }

    wPtr->renderInfo->fmt.whichArea = which;
    sprintf(cmd, "%s %s %d %d %d %d", wPtr->formatCmd, areaNames[which],
	wPtr->renderInfo->fmt.x1, 
	wPtr->renderInfo->fmt.y1, 
	wPtr->renderInfo->fmt.x2, 
	wPtr->renderInfo->fmt.y2);
    code = Tcl_EvalEx(wPtr->dispData.interp, cmd, -1, TCL_GLOBAL_ONLY);

    if (code != TCL_OK) {
	Tcl_AddErrorInfo(wPtr->dispData.interp,
	    "\n    (format command executed by tixGrid)");
	Tk_BackgroundError(wPtr->dispData.interp);
    }

    if (cmd != buff) {
	ckfree((char*)cmd);
    }

    return code;
#undef STATIC_SPACE_SIZE
}


static void Tix_GrDrawBackground(wPtr, riPtr, drawable)
    WidgetPtr wPtr;
    RenderInfo * riPtr;
    Drawable drawable;
{
    int mainSize[2];
    int visibleHdr[2];

    if (wPtr->formatCmd == NULL) {
	return;
    }

    /* The visible size of the main area
     */
    mainSize[0] = wPtr->mainRB->size[0] - wPtr->hdrSize[0];
    mainSize[1] = wPtr->mainRB->size[1] - wPtr->hdrSize[1];
    if (mainSize[0] < 0) {
	mainSize[0] = 0;
    }
    if (mainSize[1] < 0) {
	mainSize[1] = 0;
    }

    /* the visible header size
     */
    if (wPtr->mainRB->size[0] < wPtr->hdrSize[0]) {
	visibleHdr[0] = wPtr->mainRB->size[0];
    } else {
	visibleHdr[0] = wPtr->hdrSize[0];
    }
    if (wPtr->mainRB->size[1] < wPtr->hdrSize[1]) {
	visibleHdr[1] = wPtr->mainRB->size[1];
    } else {
	visibleHdr[1] = wPtr->hdrSize[1];
    }


    /* the horizontal margin
     */
    if (wPtr->hdrSize[1] > 0 && mainSize[0] > 0) {
	wPtr->renderInfo->fmt.x1 =
	  wPtr->scrollInfo[0].offset + wPtr->hdrSize[0];
	wPtr->renderInfo->fmt.x2 =
	  wPtr->renderInfo->fmt.x1 + mainSize[0] - 1;
	wPtr->renderInfo->fmt.y1 = 0;
	wPtr->renderInfo->fmt.y2 = visibleHdr[1] - 1;

	Tix_GrCallFormatCmd(wPtr, TIX_X_MARGIN);
    }

    /* the vertical margin
     */
    if (wPtr->hdrSize[0] > 0 && mainSize[1] > 0) {
	wPtr->renderInfo->fmt.x1 = 0;
	wPtr->renderInfo->fmt.x2 = visibleHdr[0] - 1;
	wPtr->renderInfo->fmt.y1 = 
	  wPtr->scrollInfo[1].offset + wPtr->hdrSize[1];
	wPtr->renderInfo->fmt.y2 =
	  wPtr->renderInfo->fmt.y1 + mainSize[1] - 1;

	Tix_GrCallFormatCmd(wPtr, TIX_Y_MARGIN);
    }
    
    /* the stationary part of the margin
     */
    if (visibleHdr[0] > 0 && visibleHdr[1] > 0) {
	wPtr->renderInfo->fmt.x1 = 0;
	wPtr->renderInfo->fmt.x2 = visibleHdr[0] - 1;
	wPtr->renderInfo->fmt.y1 = 0;
	wPtr->renderInfo->fmt.y2 = visibleHdr[1] - 1;
    
	Tix_GrCallFormatCmd(wPtr, TIX_S_MARGIN);
    }

    /* the main area
     */
    if (mainSize[0] > 0 && mainSize[1] > 0) {
	wPtr->renderInfo->fmt.x1 =
	  wPtr->scrollInfo[0].offset + wPtr->hdrSize[0];
	wPtr->renderInfo->fmt.x2 =
	  wPtr->renderInfo->fmt.x1 + mainSize[0] - 1;
	wPtr->renderInfo->fmt.y1 = 
	  wPtr->scrollInfo[1].offset + wPtr->hdrSize[1];
	wPtr->renderInfo->fmt.y2 =
	  wPtr->renderInfo->fmt.y1 + mainSize[1] - 1;
    
	Tix_GrCallFormatCmd(wPtr, TIX_MAIN);
    }
}

static void
Tix_GrComputeSubSelection(wPtr, rect, offs)
    WidgetPtr wPtr;
    int rect[2][2];
    int offs[2];
{
    int iMin, iMax, jMin, jMax;
    Tix_ListIterator li;
    SelectBlock * sbPtr;
    int i, j, x, y;

    Tix_SimpleListIteratorInit(&li);
    for (Tix_SimpleListStart(&wPtr->selList, &li);
	 !Tix_SimpleListDone(&li);
	 Tix_SimpleListNext (&wPtr->selList, &li)) {

	sbPtr = (SelectBlock *)li.curr;

	/* clip the X direction	
	 */
	if (rect[0][0] > sbPtr->range[0][0]) {
	    iMin = rect[0][0];
	} else {
	    iMin = sbPtr->range[0][0];
	}

	if (rect[0][1]<sbPtr->range[0][1] || sbPtr->range[0][1]==TIX_GR_MAX) {
	    iMax = rect[0][1];
	} else {
	    iMax = sbPtr->range[0][1];
	}
	if (iMin > iMax) {
	    continue;
	}

	/* clip the Y direction
	 */
	if (rect[1][0] > sbPtr->range[1][0]) {
	    jMin = rect[1][0];
	} else {
	    jMin = sbPtr->range[1][0];
	}
	if (rect[1][1]<sbPtr->range[1][1] || sbPtr->range[1][1]==TIX_GR_MAX) {
	    jMax = rect[1][1];
	} else {
	    jMax = sbPtr->range[1][1];
	}
	if (jMin > jMax) {
	    continue;
	}

	for (i=iMin; i<=iMax; i++) {
	    for (j=jMin; j<=jMax; j++) {
		x = i - offs[0];
		y = j - offs[1];

		switch (sbPtr->type) {
		  case TIX_GR_CLEAR:
		    wPtr->mainRB->elms[x][y].selected = 0;
		    break;
		  case TIX_GR_SET:
		    wPtr->mainRB->elms[x][y].selected = 1;
		    break;
		  case TIX_GR_TOGGLE:
		    wPtr->mainRB->elms[x][y].selected = 
		      !wPtr->mainRB->elms[x][y].selected;
		    break;

		}
	    }
	}
    }
}

static void Tix_GrComputeSelection(wPtr)
    WidgetPtr wPtr;
{
    int rect[2][2], offs[2];
    int i, j;
    int mainSize[2];
    int visibleHdr[2];

    for (i=0; i<wPtr->mainRB->size[0]; i++) {
	for (j=0; j<wPtr->mainRB->size[1]; j++) {
	    wPtr->mainRB->elms[i][j].selected = 0;
	}
    }

    /* Get the visible size of the main area
     */
    mainSize[0] = wPtr->mainRB->size[0] - wPtr->hdrSize[0];
    mainSize[1] = wPtr->mainRB->size[1] - wPtr->hdrSize[1];
    if (mainSize[0] < 0) {
	mainSize[0] = 0;
    }
    if (mainSize[1] < 0) {
	mainSize[1] = 0;
    }

    /* Get the visible header size
     */
    if (wPtr->mainRB->size[0] < wPtr->hdrSize[0]) {
	visibleHdr[0] = wPtr->mainRB->size[0];
    } else {
	visibleHdr[0] = wPtr->hdrSize[0];
    }
    if (wPtr->mainRB->size[1] < wPtr->hdrSize[1]) {
	visibleHdr[1] = wPtr->mainRB->size[1];
    } else {
	visibleHdr[1] = wPtr->hdrSize[1];
    }
    
    /* Compute selection on the stationary part of the margin
     */
    if (visibleHdr[0] > 0 && visibleHdr[1] > 0) {
	rect[0][0] = 0;
	rect[0][1] = visibleHdr[0] - 1;
	rect[1][0] = 0;
	rect[1][1] = visibleHdr[1] - 1;
	offs[0]    = 0;
	offs[1]    = 0;
    
	Tix_GrComputeSubSelection(wPtr, rect, offs);    
    }

    /* Compute selection on the horizontal margin
     */
    if (wPtr->hdrSize[1] > 0 && mainSize[0] > 0) {
	rect[0][0] = wPtr->scrollInfo[0].offset + wPtr->hdrSize[0];
	rect[0][1] = rect[0][0] + mainSize[0] - 1;
	rect[1][0] = 0;
	rect[1][1] = visibleHdr[1] - 1;
	offs[0]    = wPtr->scrollInfo[0].offset;;
	offs[1]    = 0;
    
	Tix_GrComputeSubSelection(wPtr, rect, offs);
    }

    /* Compute selection on the vertical margin
     */
    if (wPtr->hdrSize[0] > 0 && mainSize[1] > 0) {
	rect[0][0] = 0;
	rect[0][1] = visibleHdr[0] - 1;
	rect[1][0] = wPtr->scrollInfo[1].offset + wPtr->hdrSize[1];
	rect[1][1] = rect[1][0] + mainSize[1] - 1;
	offs[0]    = 0;
	offs[1]    = wPtr->scrollInfo[1].offset;;
    
	Tix_GrComputeSubSelection(wPtr, rect, offs);
    }

    /* Compute selection on the main area
     */
    if (mainSize[0] > 0 && mainSize[1] > 0) {
	rect[0][0] = wPtr->scrollInfo[0].offset + wPtr->hdrSize[0];
	rect[0][1] = rect[0][0] + mainSize[0] - 1;
	rect[1][0] = wPtr->scrollInfo[1].offset + wPtr->hdrSize[1];
	rect[1][1] = rect[1][0] + mainSize[1] - 1;
	offs[0]    = wPtr->scrollInfo[0].offset;;
	offs[1]    = wPtr->scrollInfo[1].offset;;
    
	Tix_GrComputeSubSelection(wPtr, rect, offs);
    }
}

/*----------------------------------------------------------------------
 *  UpdateScrollBars
 *----------------------------------------------------------------------
 */
static void
GetScrollFractions(wPtr, siPtr, first_ret, last_ret)
    WidgetPtr wPtr;
    Tix_GridScrollInfo *siPtr;
    double * first_ret;
    double * last_ret;
{
    double first, last;
    double usuable;

    usuable = 1.0 - siPtr->window;
    
    if (siPtr->max > 0) {
	first = usuable * (double)(siPtr->offset) / (double)(siPtr->max);
	last  = first + siPtr->window;
    } else {
	first = 0.0;
	last  = 1.0;
    }

    *first_ret = first;
    *last_ret  = last;
}

static void UpdateScrollBars(wPtr, sizeChanged)
    WidgetPtr wPtr;
    int sizeChanged;
{
    int i;
    Tix_GridScrollInfo *siPtr;
    Tcl_Interp * interp = wPtr->dispData.interp;

    for (i=0; i<2; i++) {
	double first, last;
	double usuable;

	siPtr = &wPtr->scrollInfo[i];

	usuable = 1.0 - siPtr->window;

	if (siPtr->max > 0) {
	    first = usuable * (double)(siPtr->offset) / (double)(siPtr->max);
	    last  = first + siPtr->window;
	} else {
	    first = 0.0;
	    last  = 1.0;
	}

	if (siPtr->command) {
	    char buff[60];

	    sprintf(buff, " %f %f", first, last);
	    if (Tcl_VarEval(interp, siPtr->command, buff, 
	        (char *) NULL) != TCL_OK) {
		Tcl_AddErrorInfo(interp,
		    "\n    (scrolling command executed by tixGrid)");
		Tk_BackgroundError(interp);
	    }
	}
    }

    if (wPtr->sizeCmd && sizeChanged) {
	if (Tcl_GlobalEval(wPtr->dispData.interp, wPtr->sizeCmd) != TCL_OK) {
	    Tcl_AddErrorInfo(wPtr->dispData.interp,
		"\n    (size command executed by tixGrid)");
	    Tk_BackgroundError(wPtr->dispData.interp);
	}
    }
}

/*----------------------------------------------------------------------
 * Tix_GrFindCreateElem --
 *
 *	Returns the element. If it doesn't exist, create a new one
 *	and return it.
 *----------------------------------------------------------------------
 */

static TixGrEntry *
Tix_GrFindCreateElem(interp, wPtr, x, y)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    int x;
    int y;
{
    static TixGrEntry * defaultEntry = NULL;
    TixGrEntry * chPtr;

    if (defaultEntry == NULL) {
	defaultEntry = (TixGrEntry*)ckalloc(sizeof(TixGrEntry));
	defaultEntry->iPtr     = NULL;
    }

    chPtr = (TixGrEntry*)TixGridDataCreateEntry(wPtr->dataSet, x, y,
	(char*)defaultEntry);

    if (chPtr == defaultEntry) {
	defaultEntry = NULL;
    }

    return chPtr;
}

/*----------------------------------------------------------------------
 * Tix_GrFindElem --
 *
 *	Return the element if it exists. Otherwise returns 0.
 *----------------------------------------------------------------------
 */
static TixGrEntry *
Tix_GrFindElem(interp, wPtr, x, y)
    Tcl_Interp * interp;	/* Used for error reporting */
    WidgetPtr wPtr;		/* The grid widget */
    int x;			/* X coord of the entry */
    int y;			/* Y coord of the entry */
{
    return (TixGrEntry*)TixGridDataFindEntry(wPtr->dataSet, x, y);
}

/*----------------------------------------------------------------------
 * Tix_GrFreeElem --
 *
 *	Frees the element.
 *
 *----------------------------------------------------------------------
 */

void
Tix_GrFreeElem(chPtr)
    TixGrEntry * chPtr;		/* The element fo free */
{
    if (chPtr->iPtr) {
	Tix_DItemFree(chPtr->iPtr);
    }
    ckfree((char*)chPtr);
}

static void
Tix_GrPropagateSize(wPtr, chPtr)
    WidgetPtr wPtr;
    TixGrEntry * chPtr;
{
#if 0
    int i;

    for (i=0; i<2; i++) {
	TreeListRoot * rPtr;
	GridHeader * hdr;

	rPtr = chPtr->nodes[i].root;
	hdr = (GridHeader*) rPtr->data;

	if (hdr->size < chPtr->size[i]) {
	    hdr->size = chPtr->size[i];
	    hdr->recalSize = 0;
	}
    }
#endif
}

static RenderBlock * Tix_GrAllocateRenderBlock(wPtr, winW, winH, exactW,exactH)
    WidgetPtr wPtr;
    int winW;
    int winH;
    int * exactW;
    int * exactH;
{
    RenderBlock * rbPtr;
    int i, j, k;
    int offset[2];		/* how much the entries were scrolled */
    int winSize[2];
    int exactSize[2];		/* BOOL: are all the visible coloums and rows
				 * displayed in whole */
    int pad0, pad1; 

    offset[0] = wPtr->scrollInfo[0].offset + wPtr->hdrSize[0];
    offset[1] = wPtr->scrollInfo[1].offset + wPtr->hdrSize[1];

    winSize[0] = winW;
    winSize[1] = winH;

    rbPtr = (RenderBlock*)ckalloc(sizeof(RenderBlock));

    rbPtr->size[0]=0;
    rbPtr->size[1]=0;
    rbPtr->visArea[0] = winW;
    rbPtr->visArea[1] = winH;

    /* (1) find out the size requirement of each row and column.
     *     The results are stored in rbPtr->size[i] and
     *     rbPtr->dispSize[i][0 .. (rbPtr->size[i]-1)]
     */
    for (i=0; i<2; i++) {
	/* i=0 : handle the column sizes;
	 * i=1 : handle the row    sizes;
	 */
	int index;
	int pixelSize = 0;

	/* The margins */
	for (index=0; index<wPtr->hdrSize[i] && pixelSize<winSize[i]; index++){
	    pixelSize += TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		index, &wPtr->defSize[i], &pad0, &pad1);
	    pixelSize += pad0 + pad1;
	    rbPtr->size[i] ++;
	}

	for (index=offset[i]; pixelSize<winSize[i]; index++) {
	    pixelSize += TixGridDataGetRowColSize(wPtr, wPtr->dataSet, i,
		index, &wPtr->defSize[i], &pad0, &pad1);
	    pixelSize += pad0 + pad1;
	    rbPtr->size[i] ++;
	}
	if (pixelSize == winSize[i]) {
	    exactSize[i] = 1;
	} else {
	    exactSize[i] = 0;
	}
    }

    /* return values */

    *exactW = exactSize[0];
    *exactH = exactSize[1];

    rbPtr->dispSize[0] = (ElmDispSize*)
	ckalloc(sizeof(ElmDispSize)*rbPtr->size[0]);
    rbPtr->dispSize[1] = (ElmDispSize*)
	ckalloc(sizeof(ElmDispSize)*rbPtr->size[1]);

    /*
     * (2) fill the size info of all the visible rows and cols into
     *     the dispSize arrays;
     */

    for (i=0; i<2; i++) {
	/*
	 * i=0 : handle the column sizes;
	 * i=1 : handle the row    sizes;
	 */
	int index;

	for (k=0; k<rbPtr->size[i]; k++) {
	    if (k < wPtr->hdrSize[i]) {			/* The margins */
		index = k;
	    } else {
		index = k + offset[i] - wPtr->hdrSize[i];
	    }

	    rbPtr->dispSize[i][k].size = TixGridDataGetRowColSize(wPtr, 
		wPtr->dataSet, i, index, &wPtr->defSize[i], &pad0, &pad1);
	    rbPtr->dispSize[i][k].preBorder  = pad0;
	    rbPtr->dispSize[i][k].postBorder = pad1;
	}
    }

    /*
     * (3) Put the visible elements into the render block array,
     *     rbPtr->elms[*][*].
     */
    rbPtr->elms = (RenderBlockElem**)
        ckalloc(sizeof(RenderBlockElem*)*rbPtr->size[0]);

    for (i=0; i<rbPtr->size[0]; i++) {
	rbPtr->elms[i] = (RenderBlockElem*)
	    ckalloc(sizeof(RenderBlockElem) * rbPtr->size[1]);
	for (j=0; j<rbPtr->size[1]; j++) {
	    rbPtr->elms[i][j].chPtr = NULL;
	    rbPtr->elms[i][j].selected = 0;
	}
    }

    for (i=0; i<rbPtr->size[0]; i++) {
	for (j=0; j<rbPtr->size[1]; j++) {
	    int x, y;

	    if (i<wPtr->hdrSize[0]) {
		x = i;
	    } else {
		x = i+offset[0]-wPtr->hdrSize[0];
	    }

	    if (j<wPtr->hdrSize[1]) {
		y = j;
	    } else {
		y = j+offset[1]-wPtr->hdrSize[1];
	    }

	    rbPtr->elms[i][j].chPtr = (TixGrEntry*) TixGridDataFindEntry(
		wPtr->dataSet, x, y);
	    rbPtr->elms[i][j].index[0] = x;
	    rbPtr->elms[i][j].index[1] = y;
	}
    }

    for (k=0; k<2; k++) {
	for (i=0; i<rbPtr->size[k]; i++) {
	    rbPtr->dispSize[k][i].total = 
	          rbPtr->dispSize[k][i].preBorder
	        + rbPtr->dispSize[k][i].size
	        + rbPtr->dispSize[k][i].postBorder;
	}
    }

    return rbPtr;
}

static void
Tix_GrFreeRenderBlock(wPtr, rbPtr)
    WidgetPtr wPtr;
    RenderBlock * rbPtr;
{
    int i;

    for (i=0; i<rbPtr->size[0]; i++) {
	ckfree((char*)rbPtr->elms[i]);
    }
    ckfree((char*)rbPtr->elms);
    ckfree((char*)rbPtr->dispSize[0]);
    ckfree((char*)rbPtr->dispSize[1]);
    ckfree((char*)rbPtr);
}

/*----------------------------------------------------------------------
 * Tix_GrBBox --
 *
 *	Returns the visible bounding box of a entry.
 *
 * Return value:
 *	See user documenetation.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

static int Tix_GrBBox(interp, wPtr, x, y)
    Tcl_Interp * interp;	/* Interpreter to report the bbox. */
    WidgetPtr wPtr;		/* HList widget. */
    int x;			/* X coordinate of the entry.*/
    int y;			/* Y coordinate of the entry.*/
{
    int rect[2][2];
    int visible;
    char buff[100];

    if (!Tk_IsMapped(wPtr->dispData.tkwin)) {
	return TCL_OK;
    }

    visible = Tix_GrGetElementPosn(wPtr, wPtr->anchor[0], wPtr->anchor[1],
	rect, 0, 0, 1, 0);
    if (!visible) {
	return TCL_OK;
    }

    sprintf(buff, "%d %d %d %d", rect[0][0], rect[1][0],
	rect[0][1] - rect[0][0] + 1,
	rect[1][1] - rect[1][0] + 1);

    Tcl_AppendResult(interp, buff, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * TranslateFromTo --
 *
 *	Translate the "option from ?to?" arguments from string to integer.
 *
 * Results:
 *	Standard Tcl results.
 *
 * Side effects:
 *	On success, *from and *to contains the from and to values.
 *----------------------------------------------------------------------
 */

static int
TranslateFromTo(interp, wPtr, argc, argv, from, to, which)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    int argc;
    CONST84 char **argv;
    int *from;
    int *to;
    int * which;
{
    int dummy;
    size_t len = strlen(argv[0]);

    if (strncmp(argv[0], "row", len) == 0) {
	*which = 1;

	if (TixGridDataGetIndex(interp, wPtr, "0", argv[1], &dummy, from)
		!=TCL_OK) {
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    if (TixGridDataGetIndex(interp, wPtr, "0", argv[2], &dummy, to)
		    !=TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    *to = *from;
	}
    } else if (strncmp(argv[0], "column", len) == 0) {
	*which = 0;
	if (TixGridDataGetIndex(interp, wPtr, argv[1], "0", from, &dummy)
		!=TCL_OK) {
	    return TCL_ERROR;
	}

	if (argc == 3) {
	    if (TixGridDataGetIndex(interp, wPtr, argv[2], "0", to, &dummy)
		    !=TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    *to = *from;
	}
    }

    return TCL_OK;
}
