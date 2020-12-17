/* 
 * tixTList.c --
 *
 *	This module implements "TList" widgets.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000      Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixTList.c,v 1.6 2008/02/28 04:05:29 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>
#include <tixTList.h>

#define TIX_UP		1
#define TIX_DOWN	2
#define TIX_LEFT	3
#define TIX_RIGHT 	4

/*
 * Information used for argv parsing.
 */
static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-background", "background", "Background",
       DEF_TLIST_BG_COLOR, Tk_Offset(WidgetRecord, normalBg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-background", "background", "Background",
       DEF_TLIST_BG_MONO, Tk_Offset(WidgetRecord, normalBg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_BORDER, "-highlightbackground", "highlightBackground",
       "HighlightBackground",
       DEF_TLIST_BG_COLOR, Tk_Offset(WidgetRecord, border),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-highlightbackground", "highlightBackground",
       "HighlightBackground",
       DEF_TLIST_BG_MONO, Tk_Offset(WidgetRecord, border),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
       DEF_TLIST_BORDER_WIDTH, Tk_Offset(WidgetRecord, borderWidth), 0},

    {TK_CONFIG_STRING, "-browsecmd", "browseCmd", "BrowseCmd",
	DEF_TLIST_BROWSE_COMMAND, Tk_Offset(WidgetRecord, browseCmd),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-command", "command", "Command",
       DEF_TLIST_COMMAND, Tk_Offset(WidgetRecord, command),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
       DEF_TLIST_CURSOR, Tk_Offset(WidgetRecord, cursor),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_FONT, "-font", "font", "Font",
       DEF_TLIST_FONT, Tk_Offset(WidgetRecord, font), 0},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_TLIST_FG_COLOR, Tk_Offset(WidgetRecord, normalFg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_TLIST_FG_MONO, Tk_Offset(WidgetRecord, normalFg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_INT, "-height", "height", "Height",
       DEF_TLIST_HEIGHT, Tk_Offset(WidgetRecord, height), 0},

    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
       DEF_TLIST_HIGHLIGHT_COLOR, Tk_Offset(WidgetRecord, highlightColorPtr),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
       DEF_TLIST_HIGHLIGHT_MONO, Tk_Offset(WidgetRecord, highlightColorPtr),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
       "HighlightThickness",
       DEF_TLIST_HIGHLIGHT_WIDTH, Tk_Offset(WidgetRecord, highlightWidth), 0},

    {TK_CONFIG_CUSTOM, "-itemtype", "itemType", "ItemType",
       DEF_TLIST_ITEM_TYPE, Tk_Offset(WidgetRecord, diTypePtr),
       0, &tixConfigItemType},

    {TK_CONFIG_UID, "-orient", "orient", "Orient",
	DEF_TLIST_ORIENT, Tk_Offset(WidgetRecord, orientUid), 0},

    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
	DEF_TLIST_PADX, Tk_Offset(WidgetRecord, padX), 0},

    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
	DEF_TLIST_PADY, Tk_Offset(WidgetRecord, padY), 0},

    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
       DEF_TLIST_RELIEF, Tk_Offset(WidgetRecord, relief), 0},

    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_TLIST_SELECT_BG_COLOR, Tk_Offset(WidgetRecord, selectBorder),
	TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_TLIST_SELECT_BG_MONO, Tk_Offset(WidgetRecord, selectBorder),
	TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-selectborderwidth", "selectBorderWidth","BorderWidth",
       DEF_TLIST_SELECT_BORDERWIDTH,Tk_Offset(WidgetRecord, selBorderWidth),0},

    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
       DEF_TLIST_SELECT_FG_COLOR, Tk_Offset(WidgetRecord, selectFg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
       DEF_TLIST_SELECT_FG_MONO, Tk_Offset(WidgetRecord, selectFg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_UID, "-selectmode", "selectMode", "SelectMode",
	DEF_TLIST_SELECT_MODE, Tk_Offset(WidgetRecord, selectMode), 0},

    {TK_CONFIG_UID, "-state", (char*)NULL, (char*)NULL,
       DEF_TLIST_STATE, Tk_Offset(WidgetRecord, state), 0},

    {TK_CONFIG_STRING, "-sizecmd", "sizeCmd", "SizeCmd",
       DEF_TLIST_SIZE_COMMAND, Tk_Offset(WidgetRecord, sizeCmd),
       TK_CONFIG_NULL_OK},
 
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TLIST_TAKE_FOCUS, Tk_Offset(WidgetRecord, takeFocus),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_INT, "-width", "width", "Width",
	DEF_TLIST_WIDTH, Tk_Offset(WidgetRecord, width), 0},

    {TK_CONFIG_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
       DEF_TLIST_X_SCROLL_COMMAND,
       Tk_Offset(WidgetRecord, scrollInfo[0].command),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
       DEF_TLIST_Y_SCROLL_COMMAND,
       Tk_Offset(WidgetRecord, scrollInfo[1].command),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};	

#define DEF_TLISTENTRY_STATE	 "normal"

static Tk_ConfigSpec entryConfigSpecs[] = {

    {TK_CONFIG_UID, "-state", (char*)NULL, (char*)NULL,
       DEF_TLISTENTRY_STATE, Tk_Offset(ListEntry, state), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

static Tix_ListInfo entListInfo = {
    Tk_Offset(ListEntry, next),
    TIX_UNDEFINED,
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

	/* Extra procedures for this widget
	 */
static void		CancelRedrawWhenIdle _ANSI_ARGS_((WidgetPtr wPtr));
static void		CancelResizeWhenIdle _ANSI_ARGS_((WidgetPtr wPtr));
static int		ConfigElement _ANSI_ARGS_((WidgetPtr wPtr,
			    ListEntry *chPtr, int argc, CONST84 char ** argv,
			    int flags, int forced));
static void 		RedrawRows _ANSI_ARGS_((
			    WidgetPtr wPtr, Drawable pixmap));
static void		RedrawWhenIdle _ANSI_ARGS_((WidgetPtr wPtr));
static void 		ResizeRows _ANSI_ARGS_((
			    WidgetPtr wPtr, int winW, int winH));
static void		ResizeWhenIdle _ANSI_ARGS_((WidgetPtr wPtr));
static void		ResizeNow _ANSI_ARGS_((WidgetPtr wPtr));
static void		UpdateScrollBars _ANSI_ARGS_((WidgetPtr wPtr,
			    int sizeChanged));
static int		Tix_TLGetFromTo _ANSI_ARGS_((
			    Tcl_Interp *interp, WidgetPtr wPtr,
			    int argc, CONST84 char **argv,
			    ListEntry ** fromPtr_ret, ListEntry ** toPtr_ret));
static void		Tix_TLDItemSizeChanged _ANSI_ARGS_((
			    Tix_DItem *iPtr));
static void		MakeGeomRequest _ANSI_ARGS_((
			    WidgetPtr wPtr));
static int		Tix_TLGetNearest _ANSI_ARGS_((
   			    WidgetPtr wPtr, int posn[2]));
static int		Tix_TLGetAt _ANSI_ARGS_((WidgetPtr wPtr,
			    Tcl_Interp *interp, CONST84 char * spec, int *at));
static int		Tix_TLGetNeighbor _ANSI_ARGS_((
			    WidgetPtr wPtr, Tcl_Interp *interp,
			    int type, int argc, CONST84 char **argv));
static int		Tix_TranslateIndex _ANSI_ARGS_((
			    WidgetPtr wPtr, Tcl_Interp *interp, CONST84 char * string,
			    int * index, int isInsert));
static int		Tix_TLDeleteRange _ANSI_ARGS_((
			    WidgetPtr wPtr, ListEntry * fromPtr,
			    ListEntry *toPtr));
static ListEntry *	AllocEntry _ANSI_ARGS_((WidgetPtr wPtr));
static int		AddElement _ANSI_ARGS_((WidgetPtr wPtr,
			    ListEntry * chPtr, int at));
static void		FreeEntry _ANSI_ARGS_((WidgetPtr wPtr,
			    ListEntry * chPtr));
static int 		Tix_TLSpecialEntryInfo _ANSI_ARGS_((
			    WidgetPtr wPtr, Tcl_Interp *interp,
			    ListEntry * chPtr));
static void		Realloc _ANSI_ARGS_((WidgetPtr wPtr,int new_size));

static TIX_DECLARE_SUBCMD(Tix_TLInsert);
static TIX_DECLARE_SUBCMD(Tix_TLCGet);
static TIX_DECLARE_SUBCMD(Tix_TLConfig);
static TIX_DECLARE_SUBCMD(Tix_TLDelete);
static TIX_DECLARE_SUBCMD(Tix_TLEntryCget);
static TIX_DECLARE_SUBCMD(Tix_TLEntryConfig);
static TIX_DECLARE_SUBCMD(Tix_TLGeometryInfo);
static TIX_DECLARE_SUBCMD(Tix_TLIndex);
static TIX_DECLARE_SUBCMD(Tix_TLInfo);
static TIX_DECLARE_SUBCMD(Tix_TLNearest);
static TIX_DECLARE_SUBCMD(Tix_TLSee);
static TIX_DECLARE_SUBCMD(Tix_TLSelection);
static TIX_DECLARE_SUBCMD(Tix_TLSetSite);
static TIX_DECLARE_SUBCMD(Tix_TLView);

/*
 *--------------------------------------------------------------
 *
 * Tix_TListCmd --
 *
 *	This procedure is invoked to process the "tixTList" Tcl
 *	command.  It creates a new "TixTList" widget.
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
Tix_TListCmd(clientData, interp, argc, argv)
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

    Tk_SetClass(tkwin, "TixTList");

    /*
     * Allocate and initialize the widget record.
     */
    wPtr = (WidgetPtr) ckalloc(sizeof(WidgetRecord));
    memset(wPtr, 0, sizeof(WidgetRecord));

    wPtr->dispData.tkwin 	= tkwin;
    wPtr->dispData.display 	= Tk_Display(tkwin);
    wPtr->dispData.interp 	= interp;
    wPtr->dispData.sizeChangedProc = Tix_TLDItemSizeChanged;
    wPtr->backgroundGC		= None;
    wPtr->selectGC		= None;
    wPtr->anchorGC		= None;
    wPtr->anchorGC2		= None;
    wPtr->highlightGC		= None;
    wPtr->relief 		= TK_RELIEF_FLAT;
    wPtr->cursor 		= None;
    wPtr->state			= tixNormalUid;
    wPtr->rows			= (ListRow*)ckalloc(sizeof(ListRow) *1);
    wPtr->numRow		= 1;
    wPtr->numRowAllocd		= 1;

    Tix_LinkListInit(&wPtr->entList);
    Tix_InitScrollInfo((Tix_ScrollInfo*)&wPtr->scrollInfo[0], TIX_SCROLL_INT);
    Tix_InitScrollInfo((Tix_ScrollInfo*)&wPtr->scrollInfo[1], TIX_SCROLL_INT);

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

    Tcl_SetResult(interp, Tk_PathName(wPtr->dispData.tkwin), TCL_STATIC);
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
    size_t length;
    Tix_StyleTemplate stTmpl;

    oldfont = wPtr->font;

    if (Tk_ConfigureWidget(interp, wPtr->dispData.tkwin, configSpecs,
	    argc, argv, (char *) wPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    length = strlen(wPtr->orientUid);
    if (strncmp(wPtr->orientUid, "vertical", length) == 0) {
	wPtr->isVertical = 1;
    } else if (strncmp(wPtr->orientUid, "horizontal", length) == 0) {
	wPtr->isVertical = 0;
    } else {
	Tcl_AppendResult(interp, "bad orientation \"", wPtr->orientUid,
	    "\": must be vertical or horizontal", (char *) NULL);
	wPtr->orientUid = Tk_GetUid("vertical");
	wPtr->isVertical = 1;
	return TCL_ERROR;
    }

    if ((wPtr->state != tixNormalUid) && (wPtr->state != tixDisabledUid)) {
	Tcl_AppendResult(interp, "bad state value \"", wPtr->state,
	    "\":  must be normal or disabled", (char *) NULL);
	wPtr->state = tixNormalUid;
	return TCL_ERROR;
    }


    if (oldfont != wPtr->font) {
	/* Font has been changed (initialized) */
	TixComputeTextGeometry(wPtr->font, "0", 1,
	    0, &wPtr->scrollInfo[0].unit, &wPtr->scrollInfo[1].unit);
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

    /* The dotted anchor lines (for selected item) */
    newGC = Tix_GetAnchorGC(wPtr->dispData.tkwin,
            Tk_3DBorderColor(wPtr->selectBorder));
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC);
    }
    wPtr->anchorGC = newGC;

    /* The dotted anchor lines (for unselected item)  */
    newGC = Tix_GetAnchorGC(wPtr->dispData.tkwin, wPtr->normalBg);
    if (wPtr->anchorGC2 != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC2);
    }
    wPtr->anchorGC2 = newGC;

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

    /*
     * Probably the -font or -width or -height options have changed. Let's
     * make geometry request
     */
    MakeGeomRequest(wPtr);

    ResizeWhenIdle(wPtr);

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
	{TIX_DEFAULT_LEN, "active", 1, 2, Tix_TLSetSite,
	   "option ?index?"},
	{TIX_DEFAULT_LEN, "anchor", 1, 2, Tix_TLSetSite,
	   "option ?index?"},
	{TIX_DEFAULT_LEN, "cget", 1, 1, Tix_TLCGet,
	   "option"},
	{TIX_DEFAULT_LEN, "configure", 0, TIX_VAR_ARGS, Tix_TLConfig,
	   "?option? ?value? ?option value ... ?"},
	{TIX_DEFAULT_LEN, "delete", 1, 2, Tix_TLDelete,
	   "from ?to?"},
	{TIX_DEFAULT_LEN, "dragsite", 1, 2, Tix_TLSetSite,
	   "option ?entryPath?"},
	{TIX_DEFAULT_LEN, "dropsite", 1, 2, Tix_TLSetSite,
	   "option ?entryPath?"},
	{TIX_DEFAULT_LEN, "entrycget", 2, 2, Tix_TLEntryCget,
	   "entryPath option"},
	{TIX_DEFAULT_LEN, "entryconfigure", 1, TIX_VAR_ARGS, Tix_TLEntryConfig,
	   "index ?option? ?value? ?option value ... ?"},
	{TIX_DEFAULT_LEN, "geometryinfo", 0, 2, Tix_TLGeometryInfo,
	   "?width height?"},
	{TIX_DEFAULT_LEN, "index", 1, 1, Tix_TLIndex,
	   "index"},
	{TIX_DEFAULT_LEN, "info", 1, TIX_VAR_ARGS, Tix_TLInfo,
	   "option ?args ...?"},
	{TIX_DEFAULT_LEN, "insert", 1, TIX_VAR_ARGS, Tix_TLInsert,
	   "where ?option value ..."},
	{TIX_DEFAULT_LEN, "nearest", 2, 2, Tix_TLNearest,
	   "x y"},
	{TIX_DEFAULT_LEN, "see", 1, 1, Tix_TLSee,
	   "entryPath"},
	{TIX_DEFAULT_LEN, "selection", 1, 3, Tix_TLSelection,
	   "option arg ?arg ...?"},
	{TIX_DEFAULT_LEN, "xview", 0, 3, Tix_TLView,
	   "args"},
	{TIX_DEFAULT_LEN, "yview", 0, 3, Tix_TLView,
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

    switch (eventPtr->type) {
      case DestroyNotify:
	if (wPtr->dispData.tkwin != NULL) {
	    wPtr->dispData.tkwin = NULL;
	    Tcl_DeleteCommand(wPtr->dispData.interp, 
	        Tcl_GetCommandName(wPtr->dispData.interp, wPtr->widgetCmd));
	}
	CancelResizeWhenIdle(wPtr);
	CancelRedrawWhenIdle(wPtr);
	Tk_EventuallyFree((ClientData) wPtr, (Tix_FreeProc*)WidgetDestroy);
	break;

      case ConfigureNotify:
	ResizeWhenIdle(wPtr);
	break;

      case Expose:
	RedrawWhenIdle(wPtr);
	break;

      case FocusIn:
	wPtr->hasFocus = 1;
	RedrawWhenIdle(wPtr);
	break;

      case FocusOut:
	wPtr->hasFocus = 0;
	RedrawWhenIdle(wPtr);
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
    ClientData clientData;	/* Info about my widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;


    if (wPtr->backgroundGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->backgroundGC);
    }
    if (wPtr->selectGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->selectGC);
    }
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC);
    }
    if (wPtr->anchorGC2 != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC2);
    }
    if (wPtr->highlightGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->highlightGC);
    }

    if (wPtr->entList.numItems > 0) {
	ListEntry * fromPtr=NULL, *toPtr=NULL;
	CONST84 char * argv[2];
	argv[0] = "0";
	argv[1] = "end";

	Tix_TLGetFromTo(wPtr->dispData.interp, wPtr, 2, argv, &fromPtr,&toPtr);
	Tcl_ResetResult(wPtr->dispData.interp);

	if (fromPtr && toPtr) {
	    Tix_TLDeleteRange(wPtr, fromPtr, toPtr);
	}
    }

    if (wPtr->rows) {
	ckfree((char*)wPtr->rows);
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
    ClientData clientData;	/* Pointer to widget record for widget. */
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
    int winW, winH;
    Tk_Window tkwin = wPtr->dispData.tkwin;

    wPtr->resizing = 0;

    winW = Tk_Width(tkwin)  - 2*wPtr->highlightWidth - 2*wPtr->borderWidth;
    winH = Tk_Height(tkwin) - 2*wPtr->highlightWidth - 2*wPtr->borderWidth;

    ResizeRows(wPtr, winW, winH);
    /* Update scrollbars */
    UpdateScrollBars(wPtr, 1);

    RedrawWhenIdle(wPtr);
}

static void
MakeGeomRequest(wPtr)
    WidgetPtr wPtr;
{
    int reqW, reqH;
    
    reqW = wPtr->width  * wPtr->scrollInfo[0].unit;
    reqH = wPtr->height * wPtr->scrollInfo[1].unit;

    Tk_GeometryRequest(wPtr->dispData.tkwin, reqW, reqH);
}



/*----------------------------------------------------------------------
 * DItemSizeChanged --
 *
 *	This is called whenever the size of one of the HList's items
 *	changes its size.
 *----------------------------------------------------------------------
 */
static void
Tix_TLDItemSizeChanged(iPtr)
    Tix_DItem *iPtr;
{
    WidgetPtr wPtr = (WidgetPtr)iPtr->base.clientData;

    if (wPtr) {
	/* double-check: perhaps we haven't set the clientData yet! */
	ResizeWhenIdle(wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 * ResizeWhenIdle --
 *----------------------------------------------------------------------
 */
static void
ResizeWhenIdle(wPtr)
    WidgetPtr wPtr;
{
    if (wPtr->redrawing) {
	CancelRedrawWhenIdle(wPtr);
    }
    if (!wPtr->resizing) {
	wPtr->resizing = 1;
	Tk_DoWhenIdle(WidgetComputeGeometry, (ClientData)wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 * ResizeWhenIdle --
 *----------------------------------------------------------------------
 */
static void
ResizeNow(wPtr)
    WidgetPtr wPtr;
{
    if (wPtr->resizing) {
	Tk_CancelIdleCall(WidgetComputeGeometry, (ClientData)wPtr);

	WidgetComputeGeometry((ClientData)wPtr);
	wPtr->resizing = 0;
    }
}

/*
 *----------------------------------------------------------------------
 * CancelResizeWhenIdle --
 *----------------------------------------------------------------------
 */
static void
CancelResizeWhenIdle(wPtr)
    WidgetPtr wPtr;
{
    if (wPtr->resizing) {
	wPtr->resizing = 0;
	Tk_CancelIdleCall(WidgetComputeGeometry, (ClientData)wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 * RedrawWhenIdle --
 *----------------------------------------------------------------------
 */
static void
RedrawWhenIdle(wPtr)
    WidgetPtr wPtr;
{
    if (wPtr->resizing) {
	/*
	 * Resize will eventually call redraw.
	 */
	return;
    }
    if (!wPtr->redrawing && Tk_IsMapped(wPtr->dispData.tkwin)) {
	wPtr->redrawing = 1;
	Tk_DoWhenIdle(WidgetDisplay, (ClientData)wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 * CancelRedrawWhenIdle --
 *----------------------------------------------------------------------
 */
static void
CancelRedrawWhenIdle(wPtr)
    WidgetPtr wPtr;
{
    if (wPtr->redrawing) {
	wPtr->redrawing = 0;
	Tk_CancelIdleCall(WidgetDisplay, (ClientData)wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetDisplay --
 *
 *	Draw the widget to the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void
WidgetDisplay(clientData)
    ClientData clientData;	/* Info about my widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    Pixmap pixmap;
    Tk_Window tkwin = wPtr->dispData.tkwin;
    int winH, winW;
    int hl = wPtr->highlightWidth;
    int bd = wPtr->borderWidth;

    wPtr->redrawing = 0;		/* clear the redraw flag */
    wPtr->serial ++;

    pixmap = Tk_GetPixmap(wPtr->dispData.display, Tk_WindowId(tkwin),
	Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));

    /* Fill the background */
    XFillRectangle(wPtr->dispData.display, pixmap, wPtr->backgroundGC,
	0, 0, (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin));

    winW = Tk_Width(tkwin)  - 2*hl - 2*bd;
    winH = Tk_Height(tkwin) - 2*hl - 2*bd;

    if (winW > 0 && winH > 0) {
	RedrawRows(wPtr, pixmap);
    }

    /* Draw the border */
    Tk_Draw3DRectangle(wPtr->dispData.tkwin, pixmap, wPtr->border,
	    hl, hl, Tk_Width(tkwin)  - 2*hl, Tk_Height(tkwin) - 2*hl, bd,
	    wPtr->relief);
  
    /* Draw the highlight */
    if (hl > 0) {
	GC gc;

	if (wPtr->hasFocus) {
	    gc = wPtr->highlightGC;
	} else {
	    gc = Tk_3DBorderGC(wPtr->dispData.tkwin, wPtr->border, 
		TK_3D_FLAT_GC);
	}
	Tk_DrawFocusHighlight(tkwin, gc, hl, pixmap);
    }

    /*
     * Copy the information from the off-screen pixmap onto the screen,
     * then delete the pixmap.
     */
    XCopyArea(wPtr->dispData.display, pixmap, Tk_WindowId(tkwin),
	    wPtr->backgroundGC, 0, 0,
	    (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(wPtr->dispData.display, pixmap);

}

/*----------------------------------------------------------------------
 *  AddElement  --
 *
 *	Add the element at the position indicated by "at".
 *----------------------------------------------------------------------
 */
static int
AddElement(wPtr, chPtr, at)
    WidgetPtr wPtr;
    ListEntry * chPtr;
    int at;
{
    if (at >= wPtr->entList.numItems) {
	/* The "end" position */
	Tix_LinkListAppend(&entListInfo, &wPtr->entList, (char*)chPtr, 0);
    }
    else {
	Tix_ListIterator li;
	Tix_LinkListIteratorInit(&li);

	for (Tix_LinkListStart(&entListInfo, &wPtr->entList, &li);
	     !Tix_LinkListDone(&li);
	     Tix_LinkListNext (&entListInfo, &wPtr->entList, &li)) {

	    if (at == 0) {
		Tix_LinkListInsert(&entListInfo, &wPtr->entList,
		    (char*)chPtr, &li);
		break;
	    } else {
		-- at;
	    }
	}
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 *  AllocEntry  --
 *
 *	Allocates memory for a new entry and initializes it to a
 *	proper state.
 *----------------------------------------------------------------------
 */
static ListEntry *
AllocEntry(wPtr)
    WidgetPtr wPtr;
{
    ListEntry * chPtr;

    chPtr = (ListEntry *)ckalloc(sizeof(ListEntry));
    chPtr->state	= NULL;
    chPtr->selected	= 0;
    chPtr->iPtr		= NULL;

    return chPtr;
}

/*----------------------------------------------------------------------
 *  FreeEntry  --
 *
 *	Free the entry and all resources allocated to this entry.
 *	This entry must already be deleted from the list.
 *----------------------------------------------------------------------
 */
static void
FreeEntry(wPtr, chPtr)
    WidgetPtr wPtr;
    ListEntry * chPtr;
{

    if (wPtr->seeElemPtr == chPtr) {
	/*
	 * This is the element that should be visible the next time
	 * we draw the window. Adjust the "to see element" to an element
	 * that is close to it.
	 */
	if (chPtr->next != NULL) {
	    wPtr->seeElemPtr = chPtr->next;
	} else {
	    ListEntry *p;

	    wPtr->seeElemPtr = NULL;
	    for (p=(ListEntry*)wPtr->entList.head; p; p=p->next) {
		if (p->next == chPtr) {
		    wPtr->seeElemPtr = p;
		    break;
		}
	    }
	}
    }

    if (wPtr->anchor == chPtr) {
	wPtr->anchor = NULL;
    }
    if (wPtr->active == chPtr) {
	wPtr->active = NULL;
    }
    if (wPtr->dragSite == chPtr) {
	wPtr->dragSite = NULL;
    }
    if (wPtr->dropSite == chPtr) {
	wPtr->dropSite = NULL;
    }

    if (chPtr->iPtr != NULL) {
	if (Tix_DItemType(chPtr->iPtr) == TIX_DITEM_WINDOW) {
#if 0
	    Tix_WindowItemListRemove(&wPtr->mappedWindows,
		chPtr->iPtr);
#endif
	}
	Tix_DItemFree(chPtr->iPtr);
    }

    Tk_FreeOptions(entryConfigSpecs, (char *)chPtr,wPtr->dispData.display, 0);
    ckfree((char*)chPtr);
}

/*----------------------------------------------------------------------
 * "insert" sub command -- 
 *
 *	Insert a new item into the list
 *----------------------------------------------------------------------
 */
static int
Tix_TLInsert(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    ListEntry * chPtr = NULL;
    char buff[40];
    CONST84 char * ditemType;
    int at;
    int added = 0;
    int code = TCL_OK;

    /*------------------------------------------------------------
     * (1) We need to determine the options:
     *     -itemtype and -at.
     *------------------------------------------------------------
     */

    /* (1.1) Find out where */
    if (Tix_TranslateIndex(wPtr, interp, argv[0], &at, 1) != TCL_OK) {
	code = TCL_ERROR; goto done;
    }

    /* (1.2) Find out the -itemtype, if specified */
    ditemType = wPtr->diTypePtr->name;   		 /* default value */
    if (argc > 1) {
	size_t len;
	int i;
	if (argc %2 != 1) {
	    Tcl_AppendResult(interp, "value for \"", argv[argc-1],
		"\" missing", NULL);
	    code = TCL_ERROR; goto done;
	}
	for (i=1; i<argc; i+=2) {
	    len = strlen(argv[i]);
	    if (strncmp(argv[i], "-itemtype", len) == 0) {
		ditemType = argv[i+1];
	    }
	}
    }

    if (Tix_GetDItemType(interp, ditemType) == NULL) {
	code = TCL_ERROR; goto done;
    }

    /*
     * (2) Allocate a new entry
     */
    chPtr = AllocEntry(wPtr);

    /* (2.1) The Display item data */
    if ((chPtr->iPtr = Tix_DItemCreate(&wPtr->dispData, ditemType)) == NULL) {
	code = TCL_ERROR; goto done;
    }
    chPtr->iPtr->base.clientData = (ClientData)wPtr;
    chPtr->size[0] = chPtr->iPtr->base.size[0];
    chPtr->size[1] = chPtr->iPtr->base.size[1];

    /*
     * (3) Add the entry into the list
     */
    if (AddElement(wPtr, chPtr, at) != TCL_OK) {
	code = TCL_ERROR; goto done;
    } else {
	added = 1;
    }

    if (ConfigElement(wPtr, chPtr, argc-1, argv+1, 0, 1) != TCL_OK) {
	code = TCL_ERROR; goto done;
    }

    ResizeWhenIdle(wPtr);

  done:
    if (code == TCL_ERROR) {
	if (chPtr != NULL) {
	    if (added) {
		Tix_LinkListFindAndDelete(&entListInfo, &wPtr->entList, 
		    (char*)chPtr, NULL);
	    }
	    FreeEntry(wPtr, chPtr);
	}
    } else {
	sprintf(buff, "%d", at);
	Tcl_AppendResult(interp, buff, NULL);	
    }

    return code;
}

static int 
Tix_TLSpecialEntryInfo(wPtr, interp, chPtr)
    WidgetPtr wPtr;
    Tcl_Interp *interp;
    ListEntry * chPtr;
{
    char buff[100];

    if (chPtr) {
	int i;
	Tix_ListIterator li;

	Tix_LinkListIteratorInit(&li);

	for (i=0,Tix_LinkListStart(&entListInfo, &wPtr->entList, &li);
	     !Tix_LinkListDone(&li);
	     Tix_LinkListNext(&entListInfo, &wPtr->entList, &li),i++) {
	    if (li.curr == (char*)chPtr) {
		break;
	    }
	}
	if (li.curr != NULL) {
	    sprintf(buff, "%d", i);
	    Tcl_AppendResult(interp, buff, NULL);
	} else {
	    panic("TList list entry is invalid");
	}
    } else {
	Tcl_ResetResult(interp);
    }
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "index" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLIndex(clientData, interp, argc, argv)
    ClientData clientData;	/* TList widget record. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int index;
    char buff[100];

    if (Tix_TranslateIndex(wPtr, interp, argv[0], &index, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    sprintf(buff, "%d", index);
    Tcl_AppendResult(interp, buff, NULL);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "info" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLInfo(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    size_t len = strlen(argv[0]);

    if (strncmp(argv[0], "anchor", len)==0) {
	return Tix_TLSpecialEntryInfo(wPtr, interp, wPtr->anchor);
    }
    else if (strncmp(argv[0], "active", len)==0) {
	return Tix_TLSpecialEntryInfo(wPtr, interp, wPtr->active);
    }
    else if (strncmp(argv[0], "down", len)==0) {
	return Tix_TLGetNeighbor(wPtr, interp, TIX_DOWN,  argc-1, argv+1);
    }
    else if (strncmp(argv[0], "left", len)==0) {
	return Tix_TLGetNeighbor(wPtr, interp, TIX_LEFT,  argc-1, argv+1);
    }
    else if (strncmp(argv[0], "right", len)==0) {
	return Tix_TLGetNeighbor(wPtr, interp, TIX_RIGHT, argc-1, argv+1);
    }
    else if (strncmp(argv[0], "selection", len)==0) {
	ListEntry *chPtr;
	int i;
	char buffer[32];

	for (chPtr=(ListEntry*)wPtr->entList.head, i=0;
	     chPtr;
	     chPtr=chPtr->next, i++) {
	    
	    if (chPtr->selected) {
		if (i) {
		    Tcl_AppendResult(interp, " ", (char *) NULL);
		}
		sprintf(buffer, "%d", i);
		Tcl_AppendResult(interp, buffer, (char *) NULL);
	    }
	}
	return TCL_OK;
    }
    else if (strncmp(argv[0], "size", len)==0) {
	char buff[100];

	sprintf(buff, "%d", wPtr->entList.numItems);
	Tcl_AppendResult(interp, buff, NULL);

	return TCL_OK;
    }
    else if (strncmp(argv[0], "up", len)==0) {
	return Tix_TLGetNeighbor(wPtr, interp, TIX_UP,    argc-1, argv+1);
    }
    else {
	Tcl_AppendResult(interp, "unknown option \"", argv[0], 
	    "\": must be anchor or selection",
	    NULL);
	return TCL_ERROR;
    }
}

static int
Tix_TranslateIndex(wPtr, interp, string, index, isInsert)
    WidgetPtr wPtr;		/* TList widget record. */
    Tcl_Interp *interp;		/* Current interpreter. */
    CONST84 char * string;	/* String representation of the index. */
    int * index;		/* Returns the index value(0 = 1st element).*/
    int isInsert;		/* Is this function called by an "insert"
				 * operation? */
{
    if (strcmp(string, "end") == 0) {
	*index = wPtr->entList.numItems;
    }
    else if (Tix_TLGetAt(wPtr, interp, string, index) != TCL_OK) {
	if (Tcl_GetInt(interp, string, index) != TCL_OK) {
	    return TCL_ERROR;
	}
	else if (*index < 0) {
	    Tcl_AppendResult(interp,"expected non-negative integer but got \"",
	    	string, "\"", NULL);
	    return TCL_ERROR;
	}
    }


    /*
     * The meaning of "end" means:
     *     isInsert:wPtr->entList.numItems
     *    !isInsert:wPtr->entList.numItems-1;
     */

    if (isInsert) {
	if (*index > wPtr->entList.numItems) {
	    /*
	     * By default add it to the end, just to follow what TK
	     * does for the Listbox widget
	     */
	    *index = wPtr->entList.numItems;
	}
    } else {
	if (*index >= wPtr->entList.numItems) {
	    /*
	     * By default add it to the end, just to follow what TK
	     * does for the Listbox widget
	     */
	    *index = wPtr->entList.numItems - 1;
	}
    }

    if (*index < 0) {
	*index = 0;
    }

    return TCL_OK;
}

static int Tix_TLGetNeighbor(wPtr, interp, type, argc, argv)
    WidgetPtr wPtr;
    Tcl_Interp *interp;		/* Current interpreter. */
    int type;
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    int index;
    int dst = 0; /* lint */
    int xStep, yStep;
    int numPerRow;
    char buff[100];

    if (argc != 1) {
	Tix_ArgcError(interp, argc+3, argv-3, 3, "index");
    }

    if (Tix_TranslateIndex(wPtr, interp, argv[0], &index, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    if (wPtr->entList.numItems == 0) {
	Tcl_ResetResult(interp);
	return TCL_OK;
    }

    numPerRow = wPtr->rows[0].numEnt;

    if (wPtr->isVertical) {
	xStep = numPerRow;
	yStep = 1;
    } else {
	xStep = 1;
	yStep = numPerRow;
    }

    switch (type) {
      case TIX_UP:
	dst = index - yStep;
	break;
      case TIX_DOWN:
	dst = index + yStep;
	break;
      case TIX_LEFT:
	dst = index - xStep;
	break;
      case TIX_RIGHT:
	dst = index + xStep;
	break;
    }

    if (dst < 0) {
	dst = index;
    } else if (dst >= wPtr->entList.numItems) {
	dst = index;
    }

    sprintf(buff, "%d", dst);
    Tcl_AppendResult(interp, buff, NULL);	

    return TCL_OK;
}



/*----------------------------------------------------------------------
 * "cget" sub command --
 *----------------------------------------------------------------------
 */
static int
Tix_TLCGet(clientData, interp, argc, argv)
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
Tix_TLConfig(clientData, interp, argc, argv)
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
 * "geometryinfo" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLGeometryInfo(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int qSize[2];
    double first[2], last[2];
    char string[40];
    int i;

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

    for (i=0; i<2; i++) {
	qSize[i] -= 2*wPtr->borderWidth + 2*wPtr->highlightWidth;
	Tix_GetScrollFractions((Tix_ScrollInfo*)&wPtr->scrollInfo[i],
	    &first[i], &last[i]);
    }

    sprintf(string, "{%f %f} {%f %f}", first[0], last[0], first[1], last[1]);
    Tcl_AppendResult(interp, string, NULL);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "delete" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLDelete(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    ListEntry * fromPtr, *toPtr;
    int code = TCL_OK;

    if (argc < 1 || argc > 2) {
	Tix_ArgcError(interp, argc+2, argv-2, 2, "from ?to?");
	code = TCL_ERROR;
	goto done;
    }

    if (Tix_TLGetFromTo(interp, wPtr, argc, argv, &fromPtr, &toPtr)!= TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }
    if (fromPtr == NULL) {
	goto done;
    }

    if (Tix_TLDeleteRange(wPtr, fromPtr, toPtr)) {
	ResizeWhenIdle(wPtr);
    }

  done:
    return code;
}

/* returns true if some element has been deleted */
static int Tix_TLDeleteRange(wPtr, fromPtr, toPtr)
    WidgetPtr wPtr;
    ListEntry * fromPtr;
    ListEntry *toPtr;
{
    int started;
    Tix_ListIterator li;

    Tix_LinkListIteratorInit(&li);
    started = 0;
    for (Tix_LinkListStart(&entListInfo, &wPtr->entList, &li);
	 !Tix_LinkListDone(&li);
	 Tix_LinkListNext (&entListInfo, &wPtr->entList, &li)) {

	ListEntry * curr = (ListEntry *)li.curr;
	if (curr == fromPtr) {
	    started = 1;
	}
	if (started) {
	    Tix_LinkListDelete(&entListInfo, &wPtr->entList, &li);
	    FreeEntry(wPtr, curr);
	}
	if (curr == toPtr) {
	    break;
	}
    }

    return started;
}


/*----------------------------------------------------------------------
 * "entrycget" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLEntryCget(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    ListEntry * chPtr, * dummy;

    if (Tix_TLGetFromTo(interp, wPtr, 1, argv, &chPtr, &dummy)
	!= TCL_OK) {
	return TCL_ERROR;
    }

    if (chPtr == NULL) {
	Tcl_AppendResult(interp, "list entry \"", argv[0],
	    "\" does not exist", NULL);
	return TCL_ERROR;
    }

    return Tix_ConfigureValue2(interp, wPtr->dispData.tkwin, (char *)chPtr,
	entryConfigSpecs, chPtr->iPtr, argv[1], 0);
}

/*----------------------------------------------------------------------
 * "entryconfigure" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLEntryConfig(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    ListEntry * chPtr, * dummy;

    if (Tix_TLGetFromTo(interp, wPtr, 1, argv, &chPtr, &dummy)
	!= TCL_OK) {
	return TCL_ERROR;
    }

    if (chPtr == NULL) {
	Tcl_AppendResult(interp, "list entry \"", argv[0],
	    "\" does not exist", NULL);
	return TCL_ERROR;
    }

    if (argc == 1) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)chPtr, entryConfigSpecs, chPtr->iPtr, (char *) NULL, 0);
    } else if (argc == 2) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)chPtr, entryConfigSpecs, chPtr->iPtr, (char *) argv[1], 0);
    } else {
	return ConfigElement(wPtr, chPtr, argc-1, argv+1,
	    TK_CONFIG_ARGV_ONLY, 0);
    }
}

/*----------------------------------------------------------------------
 * "nearest" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLNearest(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int posn[2];
    int index;
    char buff[100];

    if (Tcl_GetInt(interp, argv[0], &posn[0]) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &posn[1]) != TCL_OK) {
	return TCL_ERROR;
    }

    index = Tix_TLGetNearest(wPtr, posn);
    Tcl_ResetResult(interp);

    if (index != -1) {
	sprintf(buff, "%d", index);
	Tcl_AppendResult(interp, buff, NULL);
    }
    return TCL_OK;
}

static int Tix_TLGetAt(wPtr, interp, spec, at)
    WidgetPtr wPtr;
    Tcl_Interp *interp;
    CONST84 char * spec;
    int *at;
{
    int posn[2];
    char *p, *end;

    if (spec[0] != '@') {
	return TCL_ERROR;
    }

    p = (char *) spec+1;
    posn[0] = strtol(p, &end, 0);
    if ((end == p) || (*end != ',')) {
	return TCL_ERROR;
    }
    p = end+1;
    posn[1] = strtol(p, &end, 0);
    if ((end == p) || (*end != 0)) {
	return TCL_ERROR;
    }

    *at = Tix_TLGetNearest(wPtr, posn);
    return TCL_OK;
}

static int Tix_TLGetNearest(wPtr, posn)
    WidgetPtr wPtr;
    int posn[2];
{
    int i, j, index;
    int maxX, maxY;
    int r, c;

    if (wPtr->resizing) {
	ResizeNow(wPtr);
    }

    if (wPtr->entList.numItems == 0) {
	return -1;
    }

    /* clip off the position with the border of the window */

    posn[0] -= wPtr->borderWidth + wPtr->highlightWidth;
    posn[1] -= wPtr->borderWidth + wPtr->highlightWidth;

    maxX = Tk_Width (wPtr->dispData.tkwin);
    maxY = Tk_Height(wPtr->dispData.tkwin);

    maxX -= 2*(wPtr->borderWidth + wPtr->highlightWidth);
    maxY -= 2*(wPtr->borderWidth + wPtr->highlightWidth);

    if (posn[0] >= maxX) {
	posn[0] =  maxX -1;
    }
    if (posn[1] >= maxY) {
	posn[1] =  maxY -1;
    }
    if (posn[0] < 0) {
	posn[0] = 0;
    }
    if (posn[1] < 0) {
	posn[1] = 0;
    }

    i = (wPtr->isVertical == 0);
    j = (wPtr->isVertical == 1);

    posn[0] += wPtr->scrollInfo[0].offset;
    posn[1] += wPtr->scrollInfo[1].offset;

    r = posn[i] / wPtr->maxSize[i];
    c = posn[j] / wPtr->maxSize[j];

    index = (r * wPtr->rows[0].numEnt) + c;

    if (index >= wPtr->entList.numItems) {
	index = wPtr->entList.numItems - 1;
    }

    return index;
}

/*----------------------------------------------------------------------
 * "selection" sub command
 * 	Modify the selection in this HList box
 *----------------------------------------------------------------------
 */
static int
Tix_TLGetFromTo(interp, wPtr, argc, argv, fromPtr_ret, toPtr_ret)
    Tcl_Interp *interp;
    WidgetPtr wPtr;
    int argc;
    CONST84 char **argv;
    ListEntry ** fromPtr_ret;
    ListEntry ** toPtr_ret;
{
    /*
     * ToDo: make it more efficient by saving the previous from and to
     * pointers and make the list of childrens a doubly-linked list
     */
    ListEntry * fromPtr;
    ListEntry * toPtr;
    int from, to, tmp;

    if (Tix_TranslateIndex(wPtr, interp, argv[0], &from, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (argc == 2) {
	if (Tix_TranslateIndex(wPtr, interp, argv[1], &to, 0) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	to = from;
    }

    if (from > to) {
	/* swap from and to */
	tmp = to; to = from; from = tmp;
    }

    fromPtr = NULL;
    toPtr   = NULL;

    if (from >= wPtr->entList.numItems) {
	fromPtr = (ListEntry *)wPtr->entList.tail;
	toPtr   = (ListEntry *)wPtr->entList.tail;
    }
    if (to >= wPtr->entList.numItems) {
	toPtr   = (ListEntry *)wPtr->entList.tail;
    }

    if (fromPtr == NULL) {
	for (fromPtr = (ListEntry*)wPtr->entList.head;
	     from > 0;
	     fromPtr=fromPtr->next) {

	    -- from;
	    -- to;
	}
    }
    if (toPtr == NULL) {
	for (toPtr = fromPtr; to > 0; toPtr=toPtr->next) {
	    -- to;
	}
    }

    * fromPtr_ret = fromPtr;
    if (toPtr_ret) {
	* toPtr_ret = toPtr;
    }
    return TCL_OK;
}

static int
Tix_TLSelection(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    size_t len = strlen(argv[0]);
    int code = TCL_OK;
    int changed = 0;
    ListEntry * chPtr, * fromPtr, * toPtr;

    if (strncmp(argv[0], "clear", len)==0) {
	if (argc == 1) {
	    /*
	     * Clear all entries
	     */
	    for (chPtr=(ListEntry*)wPtr->entList.head;
		 chPtr;
		 chPtr=chPtr->next) {

		chPtr->selected = 0;
	    }
	    changed = 1;
	}
	else {
	    if (Tix_TLGetFromTo(interp, wPtr, argc-1, argv+1, &fromPtr, &toPtr)
		    != TCL_OK) {
		code = TCL_ERROR;
		goto done;
	    }
	    if (fromPtr == NULL) {
		goto done;
	    }
	    else {
		while (1) {
		    fromPtr->selected = 0;
		    if (fromPtr == toPtr) {
			break;
		    } else {
			fromPtr=fromPtr->next;
		    }
		}
		changed = 1;
		goto done;
	    }
	}
    }
    else if (strncmp(argv[0], "includes", len)==0) {
	if (argc != 2) {
	    Tix_ArgcError(interp, argc+2, argv-2, 3, "index");
	    code = TCL_ERROR;
	    goto done;
	}
	if (Tix_TLGetFromTo(interp, wPtr, argc-1, argv+1, &fromPtr, &toPtr)
		!= TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
	if (fromPtr->selected) {
	    Tcl_AppendResult(interp, "1", NULL);
	} else {
	    Tcl_AppendResult(interp, "0", NULL);
	}
    }
    else if (strncmp(argv[0], "set", len)==0) {
	if (argc < 2 || argc > 3) {
	    Tix_ArgcError(interp, argc+2, argv-2, 3, "from ?to?");
	    code = TCL_ERROR;
	    goto done;
	}

	if (Tix_TLGetFromTo(interp, wPtr, argc-1, argv+1, &fromPtr, &toPtr)
		!= TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
	if (fromPtr == NULL) {
	    goto done;
	}
	else {
	    while (1) {
		fromPtr->selected = 1;
		if (fromPtr == toPtr) {
		    break;
		} else {
		    fromPtr=fromPtr->next;
		}
	    }
	    changed = 1;
	    goto done;
	}
    }
    else {
	Tcl_AppendResult(interp, "unknown option \"", argv[0], 
	    "\": must be anchor, clear, includes or set", NULL);
	code = TCL_ERROR;
    }

  done:
    if (changed) {
	RedrawWhenIdle(wPtr);
    }

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_TLSee --
 *
 *      Implements the "see" widget subcommand -- make sure that the
 *      given entry is visible as much as possible.
 *
 *      We don't scroll the widget immediately in this function.
 *      Instead, we set the wPtr->seeElemPtr and the scrolling will be
 *      set when the widget is redrawn. This make sure that the "see"
 *      command works even before the widget's contents has been laid
 *      out.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      Widget is scheduled to be redrawn.
 *
 *----------------------------------------------------------------------
 */

static int
Tix_TLSee(clientData, interp, argc, argv)
    ClientData clientData;      /* The widget data struct */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    ListEntry * chPtr, * dummy;

    if (argc == 1) {
	if (Tix_TLGetFromTo(interp, wPtr, 1, argv, &chPtr, &dummy) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (chPtr != NULL) {
	    wPtr->seeElemPtr = chPtr;
	    RedrawWhenIdle(wPtr);
	}
    } else {
	Tcl_AppendResult(interp, "wrong # of arguments, must be: ",
		Tk_PathName(wPtr->dispData.tkwin), " ", argv[-1],
		" index", NULL);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "anchor", "dragsite" and "dropsite" sub commands --
 *
 *	Set/remove the anchor element
 *----------------------------------------------------------------------
 */
static int
Tix_TLSetSite(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    int changed = 0;
    WidgetPtr wPtr = (WidgetPtr) clientData;
    ListEntry * fromPtr;
    ListEntry * toPtr;			/* unused */
    ListEntry ** changePtr;
    size_t len;

    /* Determine which site should be changed (the last else clause
     * doesn't need to check the string because HandleSubCommand
     * already ensures that only the valid options can be specified.
     **/
    len = strlen(argv[-1]);
    if (strncmp(argv[-1], "anchor", len)==0) {
	changePtr = &wPtr->anchor;
    }
    else if (strncmp(argv[-1], "active", len)==0) {
	changePtr = &wPtr->active;
    }
    else if (strncmp(argv[-1], "dragsite", len)==0) {
	changePtr = &wPtr->dragSite;
    }
    else {
	changePtr = &wPtr->dropSite;
    }

    len = strlen(argv[0]);
    if (strncmp(argv[0], "set", len)==0) {
	if (argc == 2) {
	    if (Tix_TLGetFromTo(interp,wPtr, argc-1, argv+1, &fromPtr, &toPtr)
		    != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (*changePtr != fromPtr) {
		*changePtr = fromPtr;
		changed = 1;
	    }
	} else {
	    Tcl_AppendResult(interp, "wrong # of arguments, must be: ",
		Tk_PathName(wPtr->dispData.tkwin), " ", argv[-1],
		" set index", NULL);
	    return TCL_ERROR;
	}
    }
    else if (strncmp(argv[0], "clear", len)==0) {
	if (*changePtr != NULL) {
	    *changePtr = NULL;
	    changed = 1;
	}
    }
    else {
	Tcl_AppendResult(interp, "wrong option \"", argv[0], "\", ",
	    "must be clear or set", NULL);
	return TCL_ERROR;
    }

    if (changed) {
	RedrawWhenIdle(wPtr);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "xview" and "yview" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_TLView(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int axis;

    if (argv[-1][0] == 'x') {
	axis = 0;
    } else {
	axis = 1;
    }

    if (argc == 0) {
	char string[80];
	double first, last;

	Tix_GetScrollFractions((Tix_ScrollInfo*)&wPtr->scrollInfo[axis],
	    &first, &last);

	sprintf(string, "{%f %f}", first, last);
	Tcl_AppendResult(interp, string, NULL);
	return TCL_OK;
    }
    else if (Tix_SetScrollBarView(interp, 
	(Tix_ScrollInfo*)&wPtr->scrollInfo[axis], argc, argv, 0) != TCL_OK) {

	return TCL_ERROR;
    }

    UpdateScrollBars(wPtr, 0);
    RedrawWhenIdle(wPtr);
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
    ListEntry *chPtr;
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
	chPtr->size[0] = chPtr->iPtr->base.size[0];
	chPtr->size[1] = chPtr->iPtr->base.size[1];
	ResizeWhenIdle(wPtr);
    } else {
	RedrawWhenIdle(wPtr);
    }
    return TCL_OK;
}

static void
Realloc(wPtr, new_size)
    WidgetPtr wPtr;
    int new_size;
{
    if (new_size < 1) {
	new_size = 1;
    }
    if (new_size == wPtr->numRowAllocd) {
	return;
    }
    wPtr->rows = (ListRow*)ckrealloc( (char *)wPtr->rows,
                                      sizeof(ListRow)*new_size);
    wPtr->numRowAllocd = new_size;
}

static void ResizeRows(wPtr, winW, winH)
    WidgetPtr wPtr;
    int winW;			/* -1 == current width */
    int winH;			/* -1 == current height */
{
    ListEntry * chPtr;
    ListEntry * rowHead;
    int n, c, r;
    int maxI;			/* max of width in the current column */
    int maxJ;			/* max of height among all elements */
    int curRow;
    int i, j;
    int sizeJ;
    int winSize[2];

    if (wPtr->isVertical) {
	i = 0;		/* Column major, 0,0 -> 0,1 -> 0,2 ... -> 1,0 */
	j = 1;
    } else {
	i = 1;		/* Row major, 0,0 -> 1,0 -> 2,0 ... -> 0,1 */
	j = 0;
    }

    if (winW == -1) {
	winW = Tk_Width (wPtr->dispData.tkwin);
    }
    if (winH == -1) {
	winH = Tk_Height(wPtr->dispData.tkwin);
    }

    winSize[0] = winW;
    winSize[1] = winH;
    
    if (wPtr->entList.numItems == 0) {
	wPtr->rows[0].chPtr = NULL;
	wPtr->rows[0].size[0] = 1;
	wPtr->rows[0].size[1] = 1;
	wPtr->rows[0].numEnt = 0;

	wPtr->numRow = 1;
	goto done;
    }

    /* 	    --  The following verbal description follows the "Column Major"
     *     	model. Row major are similar, just the i,j incides are swapped
     *
     * (1) (a) Search for the tallest element, use it as the height of all
     *         the elements;
     *     (b) Search for the widest element, use it as the width of all
     *         the elements;
     */
    for (maxJ=1, maxI=1, chPtr = (ListEntry*)wPtr->entList.head;
	 chPtr;
	 chPtr=chPtr->next) {

	if (maxJ < chPtr->iPtr->base.size[j]) {
	    maxJ = chPtr->iPtr->base.size[j];
	}
	if (maxI < chPtr->iPtr->base.size[i]) {
	    maxI = chPtr->iPtr->base.size[i];
	}
    }
    wPtr->maxSize[i] = maxI;
    wPtr->maxSize[j] = maxJ;

    /* (2) Calculate how many elements can be in each column
     *
     */
    n = winSize[j] / maxJ;
    if (n <=0) {
	n = 1;
    }

    wPtr->numRow = 0;
    curRow = 0;
    c = 0;
    sizeJ = 0;
    rowHead = (ListEntry*)wPtr->entList.head;
    for(chPtr = (ListEntry*)wPtr->entList.head; chPtr; chPtr=chPtr->next) {
	sizeJ += chPtr->iPtr->base.size[j];
	++ c;
	if (c == n || chPtr->next == NULL) {
	    if (curRow >= wPtr->numRowAllocd) {
		Realloc(wPtr, curRow*2);
	    }
	    wPtr->rows[curRow].chPtr   = rowHead;
	    wPtr->rows[curRow].size[i] = maxI;
	    wPtr->rows[curRow].size[j] = sizeJ;
	    wPtr->rows[curRow].numEnt  = c;
	    ++ curRow;
	    ++ wPtr->numRow;
	    c = 0;
	    rowHead = chPtr->next;
	    sizeJ = 0;
	}
    }

  done:
    /* calculate the size of the total and visible area */
    wPtr->scrollInfo[i].total = 0;
    wPtr->scrollInfo[j].total = 0;

    for (r=0; r<wPtr->numRow; r++) {
	wPtr->scrollInfo[i].total += wPtr->rows[r].size[i];
	if (wPtr->scrollInfo[j].total < wPtr->rows[r].size[j]) {
	    wPtr->scrollInfo[j].total = wPtr->rows[r].size[j];
	}
    }

    wPtr->scrollInfo[i].window = winSize[i];
    wPtr->scrollInfo[j].window = winSize[j];

    if (wPtr->scrollInfo[i].total < 1) {
	wPtr->scrollInfo[i].total = 1;
    }
    if (wPtr->scrollInfo[j].total < 1) {
	wPtr->scrollInfo[j].total = 1;
    }
    if (wPtr->scrollInfo[i].window < 1) {
	wPtr->scrollInfo[i].window = 1;
    }
    if (wPtr->scrollInfo[j].window < 1) {
	wPtr->scrollInfo[j].window = 1;
    }

    /* If we have much fewer rows now, adjust the size of the rows list */
    if (wPtr->numRowAllocd > (2*wPtr->numRow)) {
	Realloc(wPtr, 2*wPtr->numRow);
    }

    /* Update the scrollbars */

    UpdateScrollBars(wPtr, 1);
}
/*----------------------------------------------------------------------
 * RedrawRows --
 *
 *	Redraw the rows, according to the "offset" in both directions
 *----------------------------------------------------------------------
 */

static void
RedrawRows(wPtr, pixmap)
    WidgetPtr wPtr;
    Drawable pixmap;
{
    int r, n;
    int p[2];
    ListEntry * chPtr;
    int i, j;
    int total;
    int windowSize;

    if (wPtr->entList.numItems == 0) {
	return;
    }

    if (wPtr->isVertical) {
	i = 0;		/* Column major, 0,0 -> 0,1 -> 0,2 ... -> 1,0 */
	j = 1;
	windowSize = Tk_Width(wPtr->dispData.tkwin);
    } else {
	i = 1;		/* Row major, 0,0 -> 1,0 -> 2,0 ... -> 0,1 */
	j = 0;
	windowSize = Tk_Height(wPtr->dispData.tkwin);
    }

    p[i] = wPtr->highlightWidth + wPtr->borderWidth;
    windowSize -= 2*p[i];

    if (windowSize < 1) {
	windowSize = 1;
    }

    if (wPtr->seeElemPtr != NULL) {
	/*
	 * Adjust the scrolling so that the given the entry is visible in
         * its entirty (as much as allowed by the height of the TList)
	 */

	int start = 0;		/* y1 position of the element to see. */
	int size = 0;		/* height of the element to see. */
	int old = wPtr->scrollInfo[i].offset;

	for (r=0, n=0, chPtr=(ListEntry*)wPtr->entList.head; chPtr;
		chPtr=chPtr->next) {
	    if (chPtr == wPtr->seeElemPtr) {
		size = chPtr->size[i];
		break;
	    }
            n++;
	    if (n == wPtr->rows[r].numEnt) {
		n=0;
		r++;
		start += wPtr->rows[r].size[i];
	    }
	}

	if (wPtr->scrollInfo[i].offset + windowSize < start + size) {
            /*
             * Bottom of the entry is beneath the bottom edge
             */
	    wPtr->scrollInfo[i].offset = start + size - windowSize;
	}
	if (wPtr->scrollInfo[i].offset > start) {
            /*
             * Top of the entry is above the top edge
             */
	    wPtr->scrollInfo[i].offset = start;
	}
	if (wPtr->scrollInfo[i].offset != old) {
	    UpdateScrollBars(wPtr, 0);
	}
	wPtr->seeElemPtr = NULL;
    }

    /*
     * Search for the first row from the top (or first column from the left)
     * that is (possibly partially) visible
     */
    total=0; r=0;
    if (wPtr->scrollInfo[i].offset != 0) {
	for (; r<wPtr->numRow; r++) {
	    total += wPtr->rows[r].size[i];

	    if (total > wPtr->scrollInfo[i].offset) {
		p[i] -= wPtr->scrollInfo[i].offset - 
		        (total - wPtr->rows[r].size[i]);
		break;
	    }
	    if (total == wPtr->scrollInfo[i].offset) {
		r++;
		break;
	    }
	}
    }

    /* Redraw all the visible rows */
    for (; r<wPtr->numRow; r++) {

	p[j] = wPtr->highlightWidth + wPtr->borderWidth;

	total=0; n=0; chPtr=wPtr->rows[r].chPtr;
	if (wPtr->scrollInfo[j].offset > 0)  {
	    /* Search for a column that is (possibly partially) visible*/
	    for (;
		 n<wPtr->rows[r].numEnt;
		 n++, chPtr = chPtr->next) {

		total += chPtr->iPtr->base.size[j];
		if (total > wPtr->scrollInfo[j].offset) {
		    /* Adjust for the shift due to partially visible elements*/
		    p[j] -= wPtr->scrollInfo[j].offset - 
		      (total - chPtr->iPtr->base.size[j]);
		    break;
		}
		if (total == wPtr->scrollInfo[j].offset) {
		    n++; chPtr = chPtr->next;
		    break;
		}
	    }
	}

	/* Redraw all the visible columns in this row */
	for (; n<wPtr->rows[r].numEnt; n++, chPtr = chPtr->next) {
	    int flags = TIX_DITEM_NORMAL_FG | TIX_DITEM_NORMAL_BG;
	    int W, H;

	    if (chPtr->selected) {
		flags |= TIX_DITEM_SELECTED_FG;
		flags |= TIX_DITEM_SELECTED_BG;
	    }

	    if (wPtr->isVertical) {
		W = wPtr->rows[r].size[0];
		H = chPtr->iPtr->base.size[1];
	    } else {
		H = wPtr->rows[r].size[1];
		W = chPtr->iPtr->base.size[0];
	    }

            if (chPtr == wPtr->anchor && wPtr->hasFocus) {
                flags |= TIX_DITEM_ANCHOR;
            }

	    Tix_DItemDisplay(pixmap, chPtr->iPtr, p[0], p[1], W, H,
		0, 0, flags);

#if 0
	    if (chPtr == wPtr->anchor && wPtr->hasFocus) {
                GC ancGC;
                if (chPtr->selected) {
                    ancGC = wPtr->anchorGC;
                } else {
                    ancGC = wPtr->anchorGC2;
                }
		Tix_DrawAnchorLines(Tk_Display(wPtr->dispData.tkwin), pixmap,
	                ancGC, p[0], p[1], W, H);
	    }
#endif
	    p[j] += wPtr->maxSize[j];
	}

	/* advance to the next row */
	p[i]+= wPtr->rows[r].size[i];
    }
}

/*----------------------------------------------------------------------
 *  UpdateScrollBars
 *----------------------------------------------------------------------
 */
static void UpdateScrollBars(wPtr, sizeChanged)
    WidgetPtr wPtr;
    int sizeChanged;
{
    Tix_UpdateScrollBar(wPtr->dispData.interp,
	(Tix_ScrollInfo*)&wPtr->scrollInfo[0]);
    Tix_UpdateScrollBar(wPtr->dispData.interp,
	(Tix_ScrollInfo*)&wPtr->scrollInfo[1]);

    if (wPtr->sizeCmd && sizeChanged) {
	if (Tcl_Eval(wPtr->dispData.interp, wPtr->sizeCmd) != TCL_OK) {
	    Tcl_AddErrorInfo(wPtr->dispData.interp,
		"\n    (size command executed by tixTList)");
	    Tk_BackgroundError(wPtr->dispData.interp);
	}
    }
}
