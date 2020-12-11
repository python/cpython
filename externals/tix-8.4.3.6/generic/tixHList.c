/* 
 * tixHList.c --
 *
 *	This module implements "HList" widgets.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000      Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixHList.c,v 1.10 2008/02/28 04:05:29 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixHList.h>
#include <tixDef.h>

/*
 * Information used for argv parsing.
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-background", "background", "Background",
       DEF_HLIST_BG_COLOR, Tk_Offset(WidgetRecord, normalBg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-background", "background", "Background",
       DEF_HLIST_BG_MONO, Tk_Offset(WidgetRecord, normalBg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
       DEF_HLIST_BORDER_WIDTH, Tk_Offset(WidgetRecord, borderWidth), 0},

    {TK_CONFIG_STRING, "-browsecmd", "browseCmd", "BrowseCmd",
	DEF_HLIST_BROWSE_COMMAND, Tk_Offset(WidgetRecord, browseCmd),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_INT, "-columns", "columns", "Columns",
       DEF_HLIST_COLUMNS, Tk_Offset(WidgetRecord, numColumns),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-command", "command", "Command",
       DEF_HLIST_COMMAND, Tk_Offset(WidgetRecord, command),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
       DEF_HLIST_CURSOR, Tk_Offset(WidgetRecord, cursor),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-dragcmd", "dragCmd", "DragCmd",
	DEF_HLIST_DRAG_COMMAND, Tk_Offset(WidgetRecord, dragCmd),
	TK_CONFIG_NULL_OK},
 
    {TK_CONFIG_BOOLEAN, "-drawbranch", "drawBranch", "DrawBranch",
       DEF_HLIST_DRAW_BRANCH, Tk_Offset(WidgetRecord, drawBranch), 0},

    {TK_CONFIG_STRING, "-dropcmd", "dropCmd", "DropCmd",
	DEF_HLIST_DROP_COMMAND, Tk_Offset(WidgetRecord, dropCmd),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_FONT, "-font", "font", "Font",
       DEF_HLIST_FONT, Tk_Offset(WidgetRecord, font), 0},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_HLIST_FG_COLOR, Tk_Offset(WidgetRecord, normalFg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
       DEF_HLIST_FG_MONO, Tk_Offset(WidgetRecord, normalFg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-gap", "gap", "Gap",
       DEF_HLIST_GAP, Tk_Offset(WidgetRecord, gap), 0},

    {TK_CONFIG_BOOLEAN, "-header", "header", "Header",
       DEF_HLIST_HEADER, Tk_Offset(WidgetRecord, useHeader), 0},

    {TK_CONFIG_INT, "-height", "height", "Height",
       DEF_HLIST_HEIGHT, Tk_Offset(WidgetRecord, height), 0},

    {TK_CONFIG_BORDER, "-highlightbackground", "highlightBackground",
       "HighlightBackground",
       DEF_HLIST_BG_COLOR, Tk_Offset(WidgetRecord, border),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-highlightbackground", "highlightBackground",
       "HighlightBackground",
       DEF_HLIST_BG_MONO, Tk_Offset(WidgetRecord, border),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
       DEF_HLIST_HIGHLIGHT_COLOR, Tk_Offset(WidgetRecord, highlightColorPtr),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
       DEF_HLIST_HIGHLIGHT_MONO, Tk_Offset(WidgetRecord, highlightColorPtr),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness",
	DEF_HLIST_HIGHLIGHT_WIDTH, Tk_Offset(WidgetRecord, highlightWidth), 0},

    {TK_CONFIG_PIXELS, "-indent", "indent", "Indent",
       DEF_HLIST_INDENT, Tk_Offset(WidgetRecord, indent), 0},

    {TK_CONFIG_BOOLEAN, "-indicator", "indicator", "Indicator",
       DEF_HLIST_INDICATOR, Tk_Offset(WidgetRecord, useIndicator), 0},

    {TK_CONFIG_STRING, "-indicatorcmd", "indicatorCmd", "IndicatorCmd",
	DEF_HLIST_INDICATOR_CMD, Tk_Offset(WidgetRecord, indicatorCmd),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_CUSTOM, "-itemtype", "itemType", "ItemType",
       DEF_HLIST_ITEM_TYPE, Tk_Offset(WidgetRecord, diTypePtr),
       0, &tixConfigItemType},

     {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
	DEF_HLIST_PADX, Tk_Offset(WidgetRecord, padX), 0},

    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
	DEF_HLIST_PADY, Tk_Offset(WidgetRecord, padY), 0},

    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
       DEF_HLIST_RELIEF, Tk_Offset(WidgetRecord, relief), 0},

    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_HLIST_SELECT_BG_COLOR, Tk_Offset(WidgetRecord, selectBorder),
	TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_HLIST_SELECT_BG_MONO, Tk_Offset(WidgetRecord, selectBorder),
	TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_PIXELS, "-selectborderwidth", "selectBorderWidth","BorderWidth",
       DEF_HLIST_SELECT_BORDERWIDTH,Tk_Offset(WidgetRecord, selBorderWidth),0},

    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
       DEF_HLIST_SELECT_FG_COLOR, Tk_Offset(WidgetRecord, selectFg),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR, "-selectforeground", "selectForeground", "Background",
       DEF_HLIST_SELECT_FG_MONO, Tk_Offset(WidgetRecord, selectFg),
       TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_UID, "-selectmode", "selectMode", "SelectMode",
	DEF_HLIST_SELECT_MODE, Tk_Offset(WidgetRecord, selectMode), 0},

    {TK_CONFIG_STRING, "-separator", "separator", "Separator",
       DEF_HLIST_SEPARATOR, Tk_Offset(WidgetRecord, separator), 0},

    {TK_CONFIG_STRING, "-sizecmd", "sizeCmd", "SizeCmd",
       DEF_HLIST_SIZE_COMMAND, Tk_Offset(WidgetRecord, sizeCmd),
       TK_CONFIG_NULL_OK},
 
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_HLIST_TAKE_FOCUS, Tk_Offset(WidgetRecord, takeFocus),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_BOOLEAN, "-wideselection", "wideSelection", "WideSelection",
       DEF_HLIST_WIDE_SELECT, Tk_Offset(WidgetRecord, wideSelect), 0},

    {TK_CONFIG_INT, "-width", "width", "Width",
	DEF_HLIST_WIDTH, Tk_Offset(WidgetRecord, width), 0},

    {TK_CONFIG_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	DEF_HLIST_X_SCROLL_COMMAND, Tk_Offset(WidgetRecord, xScrollCmd),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	DEF_HLIST_Y_SCROLL_COMMAND, Tk_Offset(WidgetRecord, yScrollCmd),
	TK_CONFIG_NULL_OK},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

static Tk_ConfigSpec entryConfigSpecs[] = {
    {TK_CONFIG_STRING, "-data", (char *) NULL, (char *) NULL,
       DEF_HLISTENTRY_DATA, Tk_Offset(HListElement, data), TK_CONFIG_NULL_OK},

    {TK_CONFIG_UID, "-state", (char*)NULL, (char*)NULL,
       DEF_HLISTENTRY_STATE, Tk_Offset(HListElement, state), 0},

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

	/* Extra procedures for this widget
	 */
static HListElement *	AllocElement _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * parent, CONST84 char * pathName, 
			    CONST84 char * name, CONST84 char * ditemType));
static void		AppendList _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement *parent, HListElement *chPtr, int at,
			    HListElement *afterPtr,
			    HListElement *beforePtr));
static void		CancelRedrawWhenIdle _ANSI_ARGS_((
			    WidgetPtr wPtr));
static void		CheckScrollBar _ANSI_ARGS_((WidgetPtr wPtr,
			    int which));
static void		ComputeBranchPosition _ANSI_ARGS_((
			    WidgetPtr wPtr, HListElement *chPtr));
static void		ComputeElementGeometry _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement *chPtr, int indent));
static void		ComputeOneElementGeometry _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement *chPtr, int indent));
static int		ConfigElement _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement *chPtr, int argc, CONST84 char ** argv, 
			    int flags, int forced));
static int		CurSelection _ANSI_ARGS_((Tcl_Interp * interp,
			    WidgetPtr wPtr, HListElement * chPtr));
static void		DeleteNode _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static void		DeleteOffsprings _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static void		DeleteSiblings _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static void		DrawElements _ANSI_ARGS_((WidgetPtr wPtr,
			    Drawable drawable, HListElement * chPtr,
			    int x, int y, int xOffset));
static void		DrawOneElement _ANSI_ARGS_((WidgetPtr wPtr, 
			    Drawable drawable, HListElement * chPtr,
			    int x, int y, int xOffset));
static HListElement *	FindElementAtPosition _ANSI_ARGS_((WidgetPtr wPtr,
			    int y));
static HListElement *	FindNextEntry  _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static HListElement *	FindPrevEntry  _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static void		FreeElement _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static HListElement *	NewElement _ANSI_ARGS_((Tcl_Interp *interp,
			    WidgetPtr wPtr, int argc, CONST84 char ** argv,
			    CONST84 char * pathName, CONST84 char * defParentName,
			    int * newArgc));
static void		RedrawWhenIdle _ANSI_ARGS_((WidgetPtr wPtr));
static int		XScrollByPages _ANSI_ARGS_((WidgetPtr wPtr,
			    int count));
static int		XScrollByUnits _ANSI_ARGS_((WidgetPtr wPtr,
			    int count));
static int		YScrollByPages _ANSI_ARGS_((WidgetPtr wPtr,
			    int count));
static int		YScrollByUnits _ANSI_ARGS_((WidgetPtr wPtr,
			    int count));
static int		SelectionModifyRange _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * from, HListElement * to,
			    int select));
static void		SelectionAdd _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static void		HL_SelectionClear _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
static void		HL_SelectionClearAll _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr, int *changed_ret));
static void		HL_SelectionClearNotifyAncestors _ANSI_ARGS_((
			    WidgetPtr wPtr, HListElement * chPtr));
static void		SelectionNotifyAncestors _ANSI_ARGS_((
			    WidgetPtr wPtr, HListElement * chPtr));
static void		UpdateOneScrollBar _ANSI_ARGS_((WidgetPtr wPtr,
			    CONST84 char * command, int total, int window, int first));
static void		UpdateScrollBars _ANSI_ARGS_((WidgetPtr wPtr,
			    int sizeChanged));
static void		DItemSizeChangedProc _ANSI_ARGS_((
			    Tix_DItem *iPtr));
static void		SubWindowEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		GetScrollFractions _ANSI_ARGS_((int total,
			    int window, int first, double * first_ret,
			    double * last_ret));
static int		Tix_HLSeeElement _ANSI_ARGS_((
			    WidgetPtr wPtr, HListElement * chPtr,
			    int callRedraw));
static int		Tix_HLBBox _ANSI_ARGS_((Tcl_Interp * interp,
			    WidgetPtr wPtr, HListElement * chPtr));

static TIX_DECLARE_SUBCMD(Tix_HLAdd);
static TIX_DECLARE_SUBCMD(Tix_HLAddChild);
static TIX_DECLARE_SUBCMD(Tix_HLCGet);
static TIX_DECLARE_SUBCMD(Tix_HLConfig);
static TIX_DECLARE_SUBCMD(Tix_HLDelete);
static TIX_DECLARE_SUBCMD(Tix_HLEntryCget);
static TIX_DECLARE_SUBCMD(Tix_HLEntryConfig);
static TIX_DECLARE_SUBCMD(Tix_HLGeometryInfo);
static TIX_DECLARE_SUBCMD(Tix_HLHide);
static TIX_DECLARE_SUBCMD(Tix_HLInfo);
static TIX_DECLARE_SUBCMD(Tix_HLNearest);
static TIX_DECLARE_SUBCMD(Tix_HLSee);
static TIX_DECLARE_SUBCMD(Tix_HLSelection);
static TIX_DECLARE_SUBCMD(Tix_HLSetSite);
static TIX_DECLARE_SUBCMD(Tix_HLShow);
static TIX_DECLARE_SUBCMD(Tix_HLXView);
static TIX_DECLARE_SUBCMD(Tix_HLYView);



/*
 *--------------------------------------------------------------
 *
 * Tix_HListCmd --
 *
 *	This procedure is invoked to process the "HList" Tcl
 *	command.  It creates a new "HList" widget.
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
Tix_HListCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window mainwin = (Tk_Window) clientData;
    WidgetPtr wPtr;
    Tk_Window tkwin, subwin;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args:	 should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Allocate the main window for this window. Then allocate a subwindow
     * to act as the header. The subwidget will always be raised to the top
     * so that it won't be obscured by any window items
     */
    tkwin = Tk_CreateWindowFromPath(interp, mainwin, argv[1], (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    subwin = Tix_CreateSubWindow(interp, tkwin, "header");
    if (subwin == NULL) {
	Tk_DestroyWindow(tkwin);
	return TCL_ERROR;
    }

    Tk_SetClass(tkwin, "TixHList");
    Tk_SetClass(subwin, "TixHListHeader");

    /*
     * Allocate and initialize the widget record.
     */
    wPtr = (WidgetPtr) ckalloc(sizeof(WidgetRecord));

    /* Init the hash table first (needed before calling AllocElement) */
    Tcl_InitHashTable(&wPtr->childTable, TCL_STRING_KEYS);

    wPtr->dispData.tkwin	= tkwin;
    wPtr->dispData.display	= Tk_Display(tkwin);
    wPtr->dispData.interp	= interp;
    wPtr->dispData.sizeChangedProc = DItemSizeChangedProc;
    wPtr->font			= NULL;
    wPtr->normalBg		= NULL;
    wPtr->normalFg		= NULL;
    wPtr->border		= NULL;
    wPtr->borderWidth		= 0;
    wPtr->selectBorder		= NULL;
    wPtr->selBorderWidth	= 0;
    wPtr->selectFg		= NULL;
    wPtr->backgroundGC		= None;
    wPtr->normalGC		= None;
    wPtr->selectGC		= None;
    wPtr->anchorGC		= None;
    wPtr->dropSiteGC		= None;
    wPtr->highlightWidth	= 0;
    wPtr->highlightColorPtr	= NULL;
    wPtr->highlightGC		= None;
    wPtr->relief		= TK_RELIEF_FLAT;
    wPtr->cursor		= None;
    wPtr->indent		= 0;
    wPtr->resizing		= 0;
    wPtr->redrawing		= 0;
    wPtr->hasFocus		= 0;
    wPtr->topPixel		= 0;
    wPtr->leftPixel		= 0;
    wPtr->separator		= NULL;
    wPtr->selectMode		= NULL;
    wPtr->anchor		= NULL;
    wPtr->dragSite		= NULL;
    wPtr->dropSite		= NULL;
    wPtr->command		= NULL;
    wPtr->browseCmd		= NULL;
    wPtr->sizeCmd		= NULL;
    wPtr->dragCmd		= NULL;
    wPtr->dropCmd		= NULL;
    wPtr->takeFocus		= NULL;
    wPtr->xScrollCmd		= NULL;
    wPtr->yScrollCmd		= NULL;
    wPtr->scrollUnit[0]		= 1;
    wPtr->scrollUnit[1]		= 1;
    wPtr->serial		= 0;
    wPtr->numColumns		= 1;
    wPtr->initialized		= 0;
    wPtr->allDirty		= 0;
    wPtr->headerDirty		= 0;
    wPtr->needToRaise		= 0;
    wPtr->drawBranch		= 1;
    wPtr->wideSelect		= 0;
    wPtr->diTypePtr		= NULL;
    wPtr->reqSize		= NULL;
    wPtr->actualSize		= NULL;
    wPtr->root			= NULL;
    wPtr->totalSize[0]		= 1;
    wPtr->totalSize[1]		= 1;
    wPtr->useIndicator		= 0;
    wPtr->indicatorCmd		= NULL;
    wPtr->headers		= NULL;
    wPtr->useHeader		= 0;
    wPtr->headerHeight		= 0;
    wPtr->headerWin		= subwin;
    wPtr->elmToSee		= 0;

    Tix_LinkListInit(&wPtr->mappedWindows);

    Tk_CreateEventHandler(wPtr->dispData.tkwin,
	ExposureMask|StructureNotifyMask|FocusChangeMask,
	WidgetEventProc, (ClientData) wPtr);
    Tk_CreateEventHandler(wPtr->headerWin,
	ExposureMask|StructureNotifyMask,
	SubWindowEventProc, (ClientData) wPtr);

    wPtr->widgetCmd = Tcl_CreateCommand(interp,
	Tk_PathName(wPtr->dispData.tkwin), WidgetCommand, (ClientData) wPtr,
	WidgetCmdDeletedProc);
    if (WidgetConfigure(interp, wPtr, argc-2, argv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(wPtr->dispData.tkwin);
	return TCL_ERROR;
    }
    if (Tix_HLCreateHeaders(interp, wPtr) != TCL_OK) {
	Tk_DestroyWindow(wPtr->dispData.tkwin);
	return TCL_ERROR;
    }

    /* Must call this **after** wPtr->numColumns is set */
    wPtr->reqSize    = Tix_HLAllocColumn(wPtr, NULL);
    wPtr->actualSize = Tix_HLAllocColumn(wPtr, NULL);
    wPtr->root	     = AllocElement(wPtr, 0, 0, 0, 0);

    wPtr->initialized = 1;

    Tcl_SetResult(interp, Tk_PathName(wPtr->dispData.tkwin), TCL_STATIC);
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
	{TIX_DEFAULT_LEN, "add", 1, TIX_VAR_ARGS, Tix_HLAdd,
	   "entryPath"},
	{TIX_DEFAULT_LEN, "addchild", 1, TIX_VAR_ARGS, Tix_HLAddChild,
	   "parentEntryPath"},
	{TIX_DEFAULT_LEN, "anchor", 1, 2, Tix_HLSetSite,
	   "option ?entryPath?"},
	{TIX_DEFAULT_LEN, "cget", 1, 1, Tix_HLCGet,
	   "option"},
	{TIX_DEFAULT_LEN, "column", 0, TIX_VAR_ARGS, Tix_HLColumn,
	   "?option? ?args ...?"},
	{TIX_DEFAULT_LEN, "configure", 0, TIX_VAR_ARGS, Tix_HLConfig,
	   "?option? ?value? ?option value ... ?"},
	{TIX_DEFAULT_LEN, "delete", 1, 2, Tix_HLDelete,
	   "option ?entryPath?"},
	{TIX_DEFAULT_LEN, "dragsite", 1, 2, Tix_HLSetSite,
	   "option ?entryPath?"},
	{TIX_DEFAULT_LEN, "dropsite", 1, 2, Tix_HLSetSite,
	   "option ?entryPath?"},
	{TIX_DEFAULT_LEN, "entrycget", 2, 2, Tix_HLEntryCget,
	   "entryPath option"},
	{TIX_DEFAULT_LEN, "entryconfigure", 1, TIX_VAR_ARGS, Tix_HLEntryConfig,
	   "entryPath ?option? ?value? ?option value ... ?"},
	{TIX_DEFAULT_LEN, "geometryinfo", 0, 2, Tix_HLGeometryInfo,
	   "?width height?"},
	{TIX_DEFAULT_LEN, "header", 1, TIX_VAR_ARGS, Tix_HLHeader,
	   "option ?args ...?"},
	{TIX_DEFAULT_LEN, "hide", 2, 2, Tix_HLHide,
	   "option entryPath"},
	{TIX_DEFAULT_LEN, "item", 0, TIX_VAR_ARGS, Tix_HLItem,
	   "?option? ?args ...?"},
	{TIX_DEFAULT_LEN, "indicator", 1, TIX_VAR_ARGS, Tix_HLIndicator,
	   "option ?args ...?"},
	{TIX_DEFAULT_LEN, "info", 1, TIX_VAR_ARGS, Tix_HLInfo,
	   "option ?args ...?"},
	{TIX_DEFAULT_LEN, "nearest", 1, 1, Tix_HLNearest,
	   "y"},
	{TIX_DEFAULT_LEN, "see", 1, 1, Tix_HLSee,
	   "entryPath"},
	{TIX_DEFAULT_LEN, "selection", 1, 3, Tix_HLSelection,
	   "option arg ?arg ...?"},
	{TIX_DEFAULT_LEN, "show", 2, 2, Tix_HLShow,
	   "option entryPath"},
	{TIX_DEFAULT_LEN, "xview", 0, 3, Tix_HLXView,
	   "args"},
	{TIX_DEFAULT_LEN, "yview", 0, 3, Tix_HLYView,
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

/*----------------------------------------------------------------------
 * "add" sub command -- 
 *
 *	Add a new item into the list
 *----------------------------------------------------------------------
 */
static int
Tix_HLAdd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    CONST84 char * pathName = argv[0];

    argc --;
    argv ++;

    if ((chPtr = NewElement(interp, wPtr, argc, argv, pathName,
	 NULL, &argc)) == NULL) {
	return TCL_ERROR;
    }

    if (argc > 0) {
	if (ConfigElement(wPtr, chPtr, argc, argv, 0, 1) != TCL_OK) {
	    DeleteNode(wPtr, chPtr);
	    return TCL_ERROR;
	}
    } else {
	if (Tix_DItemConfigure(chPtr->col[0].iPtr, 0, 0, 0) != TCL_OK) {
	    DeleteNode(wPtr, chPtr);
	    return TCL_ERROR;
	}
    }

    Tcl_AppendResult(interp, chPtr->pathName, NULL);	
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "addchild" sub command --
 *
 *	Replacement for "add" sub command: it is more flexible and
 *	you can have default names for entries.
 *
 *	Add a new item into the list
 *----------------------------------------------------------------------
 */
static int
Tix_HLAddChild(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    CONST84 char * parentName;

    parentName = argv[0];
    if (argv[0] && strcmp(argv[0], "") == 0) {
	parentName = NULL;
    }

    argc --;
    argv ++;
    if ((chPtr = NewElement(interp, wPtr, argc, argv, NULL,
	 parentName, &argc)) == NULL) {
	return TCL_ERROR;
    }

    if (argc > 0) {
	if (ConfigElement(wPtr, chPtr, argc, argv, 0, 1) != TCL_OK) {
	    DeleteNode(wPtr, chPtr);
	    return TCL_ERROR;
	}
    } else {
	if (Tix_DItemConfigure(chPtr->col[0].iPtr, 0, 0, 0) != TCL_OK) {
	    DeleteNode(wPtr, chPtr);
	    return TCL_ERROR;
	}
    }

    Tcl_AppendResult(interp, chPtr->pathName, NULL);	
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "anchor", "dragsite" and "dropsire" sub commands --
 *
 *	Set/remove the anchor element
 *----------------------------------------------------------------------
 */
static int
Tix_HLSetSite(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    int changed = 0;
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    HListElement ** changePtr;
    size_t len ;

    /*
     * Determine which site should be changed.
     */
    len = strlen(argv[-1]);
    if (strncmp(argv[-1], "anchor", len)==0) {
	changePtr = &wPtr->anchor;
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
	    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
		return TCL_ERROR;
	    }
	    if (*changePtr != chPtr) {
		*changePtr = chPtr;
		changed = 1;
	    }
	} else {
	    Tcl_AppendResult(interp, "wrong # of arguments, must be: ",
		Tk_PathName(wPtr->dispData.tkwin), " ", argv[-1],
		" set entryPath", NULL);
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
 * "cget" sub command --
 *----------------------------------------------------------------------
 */
static int
Tix_HLCGet(clientData, interp, argc, argv)
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
Tix_HLConfig(clientData, interp, argc, argv)
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
Tix_HLDelete(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    size_t len;

    if (strcmp(argv[0], "all") == 0) {
	Tix_HLMarkElementDirty(wPtr, wPtr->root);
	DeleteOffsprings(wPtr, wPtr->root);

	Tix_HLResizeWhenIdle(wPtr);
	return TCL_OK;
    }
    len = strlen(argv[0]);

    if (argc != 2) {
	if ((strncmp(argv[0], "entry", len) == 0) ||
	    (strncmp(argv[0], "offsprings", len) == 0) ||
	    (strncmp(argv[0], "siblings", len) == 0)) {

	    goto wrong_arg;
	}
	else {
	    goto wrong_option;
	}
    }

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	return TCL_ERROR;
    }

    if (strncmp(argv[0], "entry", len) == 0) {
	Tix_HLMarkElementDirty(wPtr, chPtr->parent);
	DeleteNode(wPtr, chPtr);
    }
    else if (strncmp(argv[0], "offsprings", len) == 0) {
	Tix_HLMarkElementDirty(wPtr, chPtr);
	DeleteOffsprings(wPtr, chPtr);
    }
    else if (strncmp(argv[0], "siblings", len) == 0) {
	Tix_HLMarkElementDirty(wPtr, chPtr);
	DeleteSiblings(wPtr, chPtr);
    }
    else {
	goto wrong_arg;
    }

    Tix_HLResizeWhenIdle(wPtr);
    return TCL_OK;

wrong_arg:

    Tcl_AppendResult(interp, 
	"wrong # of arguments, should be pathName delete ", argv[0],
	" entryPath", NULL);
    return TCL_ERROR;

wrong_option:

    Tcl_AppendResult(interp, "unknown option \"", argv[0],
	"\" must be all, entry, offsprings or siblings", NULL);
    return TCL_ERROR;

}

/*----------------------------------------------------------------------
 * "entrycget" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLEntryCget(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if (chPtr->col[0].iPtr == NULL) {
	Tcl_AppendResult(interp, "Item \"", argv[0], 
	    "\" does not exist", (char*)NULL);
	return TCL_ERROR;
    }
    return Tix_ConfigureValue2(interp, wPtr->dispData.tkwin, (char *)chPtr,
	entryConfigSpecs, chPtr->col[0].iPtr, argv[1], 0);
}

/*----------------------------------------------------------------------
 * "entryconfigure" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLEntryConfig(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }

    if (argc == 1) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)chPtr, entryConfigSpecs, chPtr->col[0].iPtr,
	    (char *) NULL, 0);
    } else if (argc == 2) {
	return Tix_ConfigureInfo2(interp, wPtr->dispData.tkwin,
	    (char*)chPtr, entryConfigSpecs, chPtr->col[0].iPtr,
	    (char *) argv[1], 0);
    } else {
	return ConfigElement(wPtr, chPtr, argc-1, argv+1,
	    TK_CONFIG_ARGV_ONLY, 0);
    }
}

/*----------------------------------------------------------------------
 * "geometryinfo" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLGeometryInfo(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int qSize[2];
    double first[2], last[2];
    char string[80];

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

    if (wPtr->useHeader) {
	qSize[1] -= wPtr->headerHeight;
    }

    GetScrollFractions(wPtr->totalSize[0], qSize[0], wPtr->leftPixel,
	&first[0], &last[0]);
    GetScrollFractions(wPtr->totalSize[1], qSize[1], wPtr->topPixel,
	&first[1], &last[1]);

    sprintf(string, "{%f %f} {%f %f}", first[0], last[0], first[1], last[1]);
    Tcl_AppendResult(interp, string, NULL);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "hide" sub command
 *----------------------------------------------------------------------
 */

/* %% ToDo: implement the siblings ... etc options, to match those of "delete"
 */
static int
Tix_HLHide(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	return TCL_ERROR;
    }

    Tix_HLMarkElementDirty(wPtr, chPtr->parent);
    chPtr->hidden = 1;

    Tix_HLResizeWhenIdle(wPtr);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "show" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLShow(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	return TCL_ERROR;
    }

    Tix_HLMarkElementDirty(wPtr, chPtr->parent);
    chPtr->hidden = 0;

    Tix_HLResizeWhenIdle(wPtr);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "info" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLInfo(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    size_t len = strlen(argv[0]);
    
    if (strncmp(argv[0], "anchor", len)==0) {
	if (wPtr->anchor) {
	    Tcl_AppendResult(interp, wPtr->anchor->pathName, NULL);
	}
	return TCL_OK;
    }
    else if (strncmp(argv[0], "bbox", len)==0) {
	HListElement * chPtr;

	if (argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "entryPath");
	}
	if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	    return TCL_ERROR;
	}

	return Tix_HLBBox(interp, wPtr, chPtr);
    }
    else if (strncmp(argv[0], "children", len)==0) {
	HListElement * ptr;

	if (argc != 1 && argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "?entryPath?");
	}
	if (argc == 1 || (argc == 2 && *argv[1]==0)) {
	    chPtr = wPtr->root;
	} else {
	    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
		return TCL_ERROR;
	    }
	}

	for (ptr=chPtr->childHead; ptr; ptr=ptr->next) {
	    Tcl_AppendElement(interp, ptr->pathName);
	}
	return TCL_OK;
    }
    else if (strncmp(argv[0], "data", len)==0) {
	if (argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "entryPath");
	}
	if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	    return TCL_ERROR;
	}

	Tcl_AppendResult(interp, chPtr->data, NULL);
	return TCL_OK;
    }
    else if (strncmp(argv[0], "dragsite", len)==0) {
	if (wPtr->dragSite) {
	    Tcl_AppendResult(interp, wPtr->dragSite->pathName, NULL);
	}
	return TCL_OK;
    }
    else if (strncmp(argv[0], "dropsite", len)==0) {
	if (wPtr->dropSite) {
	    Tcl_AppendResult(interp, wPtr->dropSite->pathName, NULL);
	}
	return TCL_OK;
    }
    else if (strncmp(argv[0], "exists", len)==0) {
	if (argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "entryPath");
	}
	chPtr = Tix_HLFindElement(interp, wPtr, argv[1]);

	if (chPtr) {
	    Tcl_AppendResult(interp, "1", NULL);
	} else {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "0", NULL);
	}
	return TCL_OK;
    }
    else if (strncmp(argv[0], "hidden", len)==0) {
	if (argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "entryPath");
	}
	if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	    return TCL_ERROR;
	}
	if (chPtr->hidden) {
	    Tcl_AppendElement(interp, "1");
	} else {
	    Tcl_AppendElement(interp, "0");
	}

	return TCL_OK;
    }
    else if (strncmp(argv[0], "item", len)==0) {
	return Tix_HLItemInfo(interp, wPtr, argc-1, argv+1);
    }
    else if (strncmp(argv[0], "next", len)==0) {
	HListElement * nextPtr;

	if (argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "entryPath");
	}

	if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	    return TCL_ERROR;
	}

	nextPtr=FindNextEntry(wPtr, chPtr);

	if (nextPtr) {
	    Tcl_AppendResult(interp, nextPtr->pathName, NULL);
	}	    

	return TCL_OK;
    }
    else if (strncmp(argv[0], "parent", len)==0) {
	if (argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "entryPath");
	}
	if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	    return TCL_ERROR;
	}

	Tcl_AppendResult(interp, chPtr->parent->pathName, NULL);
	return TCL_OK;
    }
    else if (strncmp(argv[0], "prev", len)==0) {
	HListElement * prevPtr;

	if (argc != 2) {
	    return Tix_ArgcError(interp, argc+2, argv-2, 3, "entryPath");
	}
	if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	    return TCL_ERROR;
	}
	prevPtr = FindPrevEntry(wPtr, chPtr);
	if (prevPtr) {
	    Tcl_AppendResult(interp, prevPtr->pathName, NULL);
	}	    

	return TCL_OK;
    }
    else if (strncmp(argv[0], "selection", len)==0) {
	return CurSelection(interp, wPtr, wPtr->root);
    }
    else {
	Tcl_AppendResult(interp, "unknown option \"", argv[0], 
	    "\": must be anchor, bbox, children, data, dragsite, dropsite, ",
	    "exists, hidden, item, next, parent, prev or selection",
	    NULL);
	return TCL_ERROR;
    }
}

/*----------------------------------------------------------------------
 * "info item" sub-sub command
 * argv[0] = x
 * argv[1] = y
 *
 *	returns {entryPath (indicator|column#) type component}
 *----------------------------------------------------------------------
 */
int
Tix_HLItemInfo(interp, wPtr, argc, argv)
    Tcl_Interp *interp;		/* Current interpreter. */
    WidgetPtr wPtr;		/* HList widget */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    HListElement * chPtr;
    int itemX, itemY;
    int listX, listY;
    int widX,  widY;
    int i, m, n;
    char column[20];

    if (argc != 2) {
	return Tix_ArgcError(interp, argc+3, argv-3, 3, "x y");
    }
    if (Tcl_GetInt(interp, argv[0], &widX) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &widY) != TCL_OK) {
	return TCL_ERROR;
    }
    if (wPtr->root->dirty || wPtr->allDirty) {
	/*
	 * We must update the geometry NOW otherwise we will get a wrong entry
	 */
	Tix_HLCancelResizeWhenIdle(wPtr);
	Tix_HLComputeGeometry((ClientData)wPtr);
    }
    if ((chPtr = FindElementAtPosition(wPtr, widY)) == NULL) {
	goto none;
    }

    listX = widX - wPtr->borderWidth - wPtr->highlightWidth + wPtr->leftPixel;
    listY = widY - wPtr->borderWidth - wPtr->highlightWidth + wPtr->topPixel;

    if (wPtr->useHeader) {
	listY -= wPtr->headerHeight;
    }

    itemX = listX - Tix_HLElementLeftOffset(wPtr, chPtr);
    itemY = listY - Tix_HLElementTopOffset (wPtr, chPtr);

    if (itemY < 0 || itemY >= chPtr->height) {
	goto none;
    }
    if (itemX < 0) {
	goto none;
    }

    if (wPtr->useIndicator && itemX < wPtr->indent) {
	if (chPtr->indicator) {
	    int indCenterX;
	    int indOffX, indOffY;
	    int indX, indY;

	    /* This "if" is a BIG HACK */
	    if (chPtr->parent == wPtr->root) {
		indCenterX = wPtr->indent/2;
	    }
	    else if (chPtr->parent->parent == wPtr->root) {
		indCenterX = chPtr->parent->branchX - wPtr->indent;
	    } else {
		indCenterX = chPtr->parent->branchX;
	    }

	    indOffX = indCenterX   - Tix_DItemWidth (chPtr->indicator)/2;
	    indOffY = chPtr->iconY - Tix_DItemHeight(chPtr->indicator)/2;

	    indX = itemX - indOffX;
	    indY = itemY - indOffY;

	    /* Is it outside of the indicator? */
	    if (indX < 0 || indX >= Tix_DItemWidth (chPtr->indicator)) {
		goto none;
	    }
	    if (indY < 0 || indY >= Tix_DItemHeight(chPtr->indicator)) {
		goto none;
	    }
	    Tcl_AppendElement(interp, chPtr->pathName);
	    Tcl_AppendElement(interp, "indicator");
	    Tcl_AppendElement(interp, Tix_DItemTypeName(chPtr->indicator));
	    Tcl_AppendElement(interp,
		Tix_DItemComponent(chPtr->indicator, indX, indY));
	    return TCL_OK;
	} else {
	    goto none;
	}
    }

    /* skip the indent part */

    if (!wPtr->useIndicator && chPtr->parent == wPtr->root) {
	/* indent not used only in this case */
    } else {
	itemX -= wPtr->indent;
    }

    for (m=n=0,i=0; i<wPtr->numColumns; i++) {
	n += wPtr->actualSize[i].width;
	if (listX < n) {
	    if (n > 1) {
		itemX = listX - m;
	    }
	    goto _column;
	}
	m += wPtr->actualSize[i].width;
    }
    goto none;

_column:
    sprintf(column, "%d", i);
    Tcl_AppendElement(interp, chPtr->pathName);
    Tcl_AppendElement(interp, column);

    if (chPtr->col[i].iPtr != NULL) {
	Tcl_AppendElement(interp, Tix_DItemTypeName(chPtr->col[i].iPtr));
	Tcl_AppendElement(interp,
	    Tix_DItemComponent(chPtr->col[i].iPtr, itemX, itemY));
    }
    return TCL_OK;
	
none:
    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "nearest" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLNearest(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    int y;

    if (Tcl_GetInt(interp, argv[0], &y) != TCL_OK) {
	return TCL_ERROR;
    }
    if (wPtr->root->dirty || wPtr->allDirty) {
	/*
	 * We must update the geometry NOW otherwise we will get a
	 * wrong entry.
	 */
	Tix_HLCancelResizeWhenIdle(wPtr);
	Tix_HLComputeGeometry((ClientData)wPtr);
    }

    if ((chPtr = FindElementAtPosition(wPtr, y)) != NULL) {
	Tcl_AppendResult(interp, chPtr->pathName, NULL);
    }
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "see" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLSee(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if (wPtr->resizing || wPtr->redrawing) {
	if (wPtr->elmToSee) {
	    ckfree(wPtr->elmToSee);
	}
	wPtr->elmToSee = tixStrDup(argv[0]);
	return TCL_OK;
    } else {
	Tix_HLSeeElement(wPtr, chPtr, 1);

	return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_HLBBox --
 *
 *	Returns the visible bounding box of an HList element (x1, y1,
 *	x2, y2).  Currently only y1 and y2 matters. x1 and x2 are
 *	always the left and right edges of the window.
 *
 * Return value:
 *	See user documenetation.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int Tix_HLBBox(interp, wPtr, chPtr)
    Tcl_Interp * interp;	/* Interpreter to report the bbox. */
    WidgetPtr wPtr;		/* HList widget. */
    HListElement * chPtr;	/* Get the BBox for this element.*/
{
    int y, height;
    int wXSize, wYSize;		/* size of the listbox window area */
    int pad;

    if (!Tk_IsMapped(wPtr->dispData.tkwin)) {
	return TCL_OK;
    }

    if (wPtr->root->dirty || wPtr->allDirty) {
	/*
	 * We must update the geometry NOW otherwise we will wrong geometry 
	 * info
	 */
	Tix_HLCancelResizeWhenIdle(wPtr);
	Tix_HLComputeGeometry((ClientData)wPtr);
    }

    y = Tix_HLElementTopOffset(wPtr, chPtr) - wPtr->topPixel;
    pad = wPtr->borderWidth + wPtr->highlightWidth;
    wXSize = Tk_Width(wPtr->dispData.tkwin ) - 2*pad;
    wYSize = Tk_Height(wPtr->dispData.tkwin) - 2*pad;

    if (wXSize <= 0) {
	wXSize = 1;
    }
    if (wYSize <= 0) {
	wYSize = 1;
    }

    height = chPtr->height;
    if (height <= 0) {
	height = 1;
    }

    if (y >= wYSize || (y+height) <= 0) {
	/*
	 * The element is not visible
	 */
	return TCL_OK;
    } else {
	int x1;
	int y1, y2;
	char buff[100];

	/*
	 * The bounding box is clipped with the visible area of the widget.
	 */

	x1 = pad;
	y1 = y + wPtr->borderWidth + wPtr->highlightWidth;
	y2 = y1 + height-1;

	if (y1 < pad) {
	    y1 = pad;
	}
	if (y2 >= pad+wYSize) {
	    y2 = pad+wYSize -1;
	}

	if (y2 >= y1) {
	    sprintf(buff, "%d %d %d %d", x1, y1, x1+wXSize-1, y2);
	    Tcl_SetResult(interp, buff, TCL_VOLATILE);
	}
	return TCL_OK;
    }
}

static int Tix_HLSeeElement(wPtr, chPtr, callRedraw)
    WidgetPtr wPtr;
    HListElement * chPtr;
    int callRedraw;
{
    int x, y;
    int cXSize, cYSize;		/* element size */
    int wXSize, wYSize;		/* size of the listbox window area */
    int top, left;		/* new top and left offset of the HLIst */
    int oldTop, oldLeft;

    oldLeft = wPtr->leftPixel;
    oldTop  = wPtr->topPixel;

    x = Tix_HLElementLeftOffset(wPtr, chPtr);
    y = Tix_HLElementTopOffset(wPtr, chPtr);
    if (chPtr->col[0].iPtr) {
	cXSize = Tix_DItemWidth(chPtr->col[0].iPtr);
    } else {
	cXSize = chPtr->col[0].width;
    }
    cYSize = chPtr->height;
    wXSize = Tk_Width(wPtr->dispData.tkwin) - 
      (2*wPtr->borderWidth + 2*wPtr->highlightWidth);
    wYSize = Tk_Height(wPtr->dispData.tkwin) -
      (2*wPtr->borderWidth + 2*wPtr->highlightWidth);

    if (wPtr->useHeader) {
	wYSize -= wPtr->headerHeight;
    }

    if (wXSize < 0 || wYSize < 0) {
	/* The window is probably not visible */
	return TCL_OK;
    }

    if (cXSize < wXSize && wPtr->numColumns == 1) {
	/* Align on the X direction */
	left = wPtr->leftPixel;
	if ((x < wPtr->leftPixel) || (x+cXSize > wPtr->leftPixel+wXSize)) {
	    if (wXSize > cXSize) {
		left = x - (wXSize-cXSize)/2;
	    } else {
		left = x;
	    }
	}
    } else {
	left = wPtr->leftPixel;
    }

    /* Align on the Y direction */
    top = wPtr->topPixel;

    if (cYSize < wYSize) {
	if ((wPtr->topPixel-y)>wYSize || (y-wPtr->topPixel-wYSize) > wYSize) {
	    /* far away, make it middle */
	    top = y - (wYSize-cYSize)/2;
	}
	else if (y < wPtr->topPixel) {
	    top = y;
	}
	else if (y+cYSize > wPtr->topPixel+wYSize){
	    top = y+cYSize - wYSize ;
	}
    }

    if (oldLeft != left || oldTop != top) {
	wPtr->leftPixel = left;
	wPtr->topPixel	= top;

	if (callRedraw) {
	    UpdateScrollBars(wPtr, 0);
	    RedrawWhenIdle(wPtr);
	}
	return 1;
    } else {
	return 0;
    }
}

/*----------------------------------------------------------------------
 * "selection" sub command
 *	Modify the selection in this HList box
 *----------------------------------------------------------------------
 */
static int
Tix_HLSelection(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    size_t len = strlen(argv[0]);
    int code = TCL_OK;
    int changed = 0;

    if (strncmp(argv[0], "clear", len)==0) {
	if (argc == 1) {
	    HL_SelectionClearAll(wPtr, wPtr->root, &changed);
	}
	else {
	    HListElement * from, * to;

	    from = Tix_HLFindElement(interp, wPtr, argv[1]);
	    if (from == NULL) {
		code = TCL_ERROR;
		goto done;
	    }

	    if (argc == 3) {
		to = Tix_HLFindElement(interp, wPtr, argv[2]);
		if (to == NULL) {
		    code = TCL_ERROR;
		    goto done;
		}
		changed = SelectionModifyRange(wPtr, from, to, 0);
	    }
	    else {
		if (from->selected == 1) {
		    HL_SelectionClear(wPtr, from);
		    changed = 1;
		}
	    }
	}
    }
    else if (strncmp(argv[0], "includes", len)==0) {
	if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[1])) == NULL) {
	    code = TCL_ERROR;
	    goto done;
	}
	if (chPtr->selected) {
	    Tcl_AppendResult(interp, "1", NULL);
	} else {
	    Tcl_AppendResult(interp, "0", NULL);
	}
    }
    else if (strncmp(argv[0], "get", len)==0) {
	if (argc != 1) {
	    Tix_ArgcError(interp, argc+2, argv-2, 3, "");
	    code = TCL_ERROR;
	} else {
	    code = CurSelection(interp, wPtr, wPtr->root);
	}
    }
    else if (strncmp(argv[0], "set", len)==0) {
	HListElement * from, * to;

	if (argc < 2 || argc > 3) {
	    Tix_ArgcError(interp, argc+2, argv-2, 3, "from ?to?");
	    code = TCL_ERROR;
	    goto done;
	}

	from = Tix_HLFindElement(interp, wPtr, argv[1]);
	if (from == NULL) {
	    code = TCL_ERROR;
	    goto done;
	}

	if (argc == 3) {
	    to = Tix_HLFindElement(interp, wPtr, argv[2]);
	    if (to == NULL) {
		code = TCL_ERROR;
		goto done;
	    }
	    changed = SelectionModifyRange(wPtr, from, to, 1);
	} else {
	    if (!from->selected && !from->hidden) {
		SelectionAdd(wPtr, from);
		changed = 1;
	    }
	}
    }
    else {
	Tcl_AppendResult(interp, "unknown option \"", argv[0], 
	    "\": must be anchor, clear, get, includes or set", NULL);
	code = TCL_ERROR;
    }

  done:
    if (changed) {
	RedrawWhenIdle(wPtr);
    }

    return code;
}

/*----------------------------------------------------------------------
 * "xview" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLXView(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    int leftPixel;
    int oldLeft = wPtr->leftPixel;
    if (argc == 0) {
	char string[20];

	sprintf(string, "%d", wPtr->leftPixel);
	Tcl_AppendResult(interp, string, NULL);
	return TCL_OK;
    }
    else if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) != NULL) {
	leftPixel = Tix_HLElementLeftOffset(wPtr, chPtr);
    }
    else if (Tcl_GetInt(interp, argv[0], &leftPixel) == TCL_OK) {
	/* %% todo backward-compatible mode */

    }
    else {
	int type, count;
	double fraction;

	Tcl_ResetResult(interp);

	/* Tk_GetScrollInfo () wants strange argc,argv combinations .. */
	type = Tk_GetScrollInfo(interp, argc+2, argv-2, &fraction, &count);
	switch (type) {
	  case TK_SCROLL_ERROR:
	    return TCL_ERROR;

	  case TK_SCROLL_MOVETO:
	    leftPixel = (int)(fraction * (double)wPtr->totalSize[0]);
	    break;

	  case TK_SCROLL_PAGES:
	    leftPixel = XScrollByPages(wPtr, count);
	    break;

	  case TK_SCROLL_UNITS:
	    leftPixel = XScrollByUnits(wPtr, count);
	    break;
	}
    }

    if (oldLeft != leftPixel) {
	wPtr->leftPixel = leftPixel;
	UpdateScrollBars(wPtr, 0);

	RedrawWhenIdle(wPtr);
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "yview" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLYView(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    int topPixel;
    int oldTop = wPtr->topPixel;

    if (argc == 0) {
	char string[20];

	sprintf(string, "%d", wPtr->topPixel);
	Tcl_AppendResult(interp, string, NULL);
	return TCL_OK;
    }
    else if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) != NULL) {
	topPixel = Tix_HLElementTopOffset(wPtr, chPtr);
    }
    else if (Tcl_GetInt(interp, argv[0], &topPixel) == TCL_OK) {
	/* %% todo backward-compatible mode */
    }
    else {
	int type, count;
	double fraction;

	Tcl_ResetResult(interp);

	/* Tk_GetScrollInfo () wants strange argc,argv combinations .. */
	type = Tk_GetScrollInfo(interp, argc+2, argv-2, &fraction, &count);
	switch (type) {
	  case TK_SCROLL_ERROR:
	    return TCL_ERROR;

	  case TK_SCROLL_MOVETO:
	    topPixel = (int)(fraction * (double)wPtr->totalSize[1]);
	    break;

	  case TK_SCROLL_PAGES:
	    topPixel = YScrollByPages(wPtr, count);
	    break;

	  case TK_SCROLL_UNITS:
	    topPixel = YScrollByUnits(wPtr, count);
	    break;
	}
    }

    if (oldTop != topPixel) {
	wPtr->topPixel = topPixel;
	UpdateScrollBars(wPtr, 0);

	RedrawWhenIdle(wPtr);
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetConfigure --
 *
 *	This procedure is called to process an argv/argc list in
 *	conjunction with the Tk option database to configure (or
 *	reconfigure) a HList widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for wPtr;	old resources get freed,
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
    int oldColumns;
    Tix_StyleTemplate stTmpl;

    oldfont = wPtr->font;
    oldColumns = wPtr->numColumns;
    if (Tk_ConfigureWidget(interp, wPtr->dispData.tkwin, configSpecs,
	    argc, argv, (char *) wPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    if (wPtr->initialized && oldColumns != wPtr->numColumns) {
	Tcl_AppendResult(interp, "Cannot change the number of columns ",
	    (char *) NULL);
	wPtr->numColumns = oldColumns;
	return TCL_ERROR;
    }
    if (wPtr->numColumns < 1) {
	wPtr->numColumns = 1;
    }

    if (wPtr->separator == 0 || wPtr->separator[0] == 0) {
	if (wPtr->separator != 0) {
	    ckfree(wPtr->separator);
	}
	wPtr->separator = tixStrDup(".");
    }

    if (oldfont != wPtr->font) {
	/*
	 * Font has been changed (initialized)
	 */
	TixComputeTextGeometry(wPtr->font, "0", 1,
	    0, &wPtr->scrollUnit[0], &wPtr->scrollUnit[1]);
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
    gcValues.foreground		= wPtr->normalBg->pixel;
    gcValues.graphics_exposures = False;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCGraphicsExposures, &gcValues);
    if (wPtr->backgroundGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->backgroundGC);
    }
    wPtr->backgroundGC = newGC;

    /* The normal text GC */
    gcValues.font		= TixFontId(wPtr->font);
    gcValues.foreground		= wPtr->normalFg->pixel;
    gcValues.background		= wPtr->normalBg->pixel;
    gcValues.graphics_exposures = False;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCBackground|GCFont|GCGraphicsExposures, &gcValues);
    if (wPtr->normalGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->normalGC);
    }
    wPtr->normalGC = newGC;

    /* The selected text GC [TODO: why need the BG??]*/
    gcValues.font		= TixFontId(wPtr->font);
    gcValues.foreground		= wPtr->selectFg->pixel;
    gcValues.background		= Tk_3DBorderColor(wPtr->selectBorder)->pixel;
    gcValues.graphics_exposures = False;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCBackground|GCFont|GCGraphicsExposures, &gcValues);
    if (wPtr->selectGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->selectGC);
    }
    wPtr->selectGC = newGC;

    /* The dotted anchor lines */
    newGC = Tix_GetAnchorGC(wPtr->dispData.tkwin,
            Tk_3DBorderColor(wPtr->selectBorder));
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC);
    }
    wPtr->anchorGC = newGC;

    /* The sloid dropsite lines */
    gcValues.foreground		= wPtr->normalFg->pixel;
    gcValues.background		= wPtr->normalBg->pixel;
    gcValues.graphics_exposures = False;
    gcValues.subwindow_mode	= IncludeInferiors;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCBackground|GCGraphicsExposures|GCSubwindowMode,
	    &gcValues);
    if (wPtr->dropSiteGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->dropSiteGC);
    }
    wPtr->dropSiteGC = newGC;

    /* The highlight border */
    gcValues.background		= wPtr->selectFg->pixel;
    gcValues.foreground		= wPtr->highlightColorPtr->pixel;
    gcValues.subwindow_mode	= IncludeInferiors;

    newGC = Tk_GetGC(wPtr->dispData.tkwin,
	GCForeground|GCBackground|GCGraphicsExposures, &gcValues);
    if (wPtr->highlightGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->highlightGC);
    }
    wPtr->highlightGC = newGC;

    /* We must set the options of the default styles so that
     * -- the default styles will change according to what is in
     *	  stTmpl
     */

    stTmpl.font				= wPtr->font;
    stTmpl.pad[0]			= wPtr->padX;
    stTmpl.pad[1]			= wPtr->padY;
    stTmpl.colors[TIX_DITEM_NORMAL].fg	= wPtr->normalFg;
    stTmpl.colors[TIX_DITEM_NORMAL].bg	= wPtr->normalBg;
    stTmpl.colors[TIX_DITEM_SELECTED].fg= wPtr->selectFg;
    stTmpl.colors[TIX_DITEM_SELECTED].bg= Tk_3DBorderColor(wPtr->selectBorder);
    stTmpl.flags = TIX_DITEM_FONT|TIX_DITEM_NORMAL_BG|
	TIX_DITEM_SELECTED_BG|TIX_DITEM_NORMAL_FG|TIX_DITEM_SELECTED_FG |
	TIX_DITEM_PADX|TIX_DITEM_PADY;
    Tix_SetDefaultStyleTemplate(wPtr->dispData.tkwin, &stTmpl);

    /* Probably the size of the elements in this has changed */
    Tix_HLResizeWhenIdle(wPtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * WidgetEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on HLists.
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
	Tix_HLCancelResizeWhenIdle(wPtr);
	CancelRedrawWhenIdle(wPtr);
	Tk_EventuallyFree((ClientData)wPtr, (Tix_FreeProc*)WidgetDestroy);
	break;

      case ConfigureNotify:
	RedrawWhenIdle(wPtr);
	UpdateScrollBars(wPtr, 1);
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
 *--------------------------------------------------------------
 *
 * SubWindowEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on the header subwindow.
 *--------------------------------------------------------------
 */
static void
SubWindowEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
#ifdef TK_PARENT_DESTROYED
    Tk_FakeWin * fw;
#endif

    switch (eventPtr->type) {
      case DestroyNotify:

#ifdef TK_PARENT_DESTROYED
	/*
	 * The TK_PARENT_DESTROYED symbol is no longer defined in Tk 8.0
         *
         * TODO: do we still have to handle this??
	 */
	fw = (Tk_FakeWin *) (wPtr->headerWin);
	if (fw->flags & TK_PARENT_DESTROYED) {
	    break;
	}
	if (wPtr->headerWin != NULL) {
	    panic("HList: header subwindow deleted illegally\n");
	}
#endif
	break;

      case Expose:
	if (wPtr->headerWin != NULL) {
	    RedrawWhenIdle(wPtr);
	}
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WidgetDestroy --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a HList at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the HList is freed up.
 *
 *----------------------------------------------------------------------
 */
static void
WidgetDestroy(clientData)
    ClientData clientData;	/* Info about my widget. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;

    if (wPtr->root != NULL) {
	DeleteOffsprings(wPtr, wPtr->root);
	FreeElement(wPtr, wPtr->root);
    }

    if (wPtr->backgroundGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->backgroundGC);
    }
    if (wPtr->normalGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->normalGC);
    }
    if (wPtr->selectGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->selectGC);
    }
    if (wPtr->anchorGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->anchorGC);
    }
    if (wPtr->dropSiteGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->dropSiteGC);
    }
    if (wPtr->highlightGC != None) {
	Tk_FreeGC(wPtr->dispData.display, wPtr->highlightGC);
    }

    /* the following two members will be NULL if the widget was destroyed
     * during its creation (e.g., wrong arguments during creation
     */
    if (wPtr->reqSize != NULL) {
	ckfree((char*)wPtr->reqSize);
    }
    if (wPtr->actualSize != NULL) {
	ckfree((char*)wPtr->actualSize);
    }
    if (wPtr->elmToSee != NULL) {
	ckfree(wPtr->elmToSee);
	wPtr->elmToSee = NULL;
    }

    Tix_HLFreeHeaders(wPtr->dispData.interp, wPtr);

    if (!Tix_IsLinkListEmpty(wPtr->mappedWindows)) {
	/*
	 * All mapped windows should have been unmapped when the
	 * the entries were deleted
	 */
	panic("tixHList: mappedWindows not NULL");
    }
    if (wPtr->headerWin) {
	wPtr->headerWin = NULL;
    }
    Tcl_DeleteHashTable(&wPtr->childTable);

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
 * Tix_HLComputeGeometry --
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
void
Tix_HLComputeGeometry(clientData)
    ClientData clientData;
{
    WidgetPtr wPtr = (WidgetPtr)clientData;
    int i, reqW, reqH;
    int sizeChanged = 0;
    int width;
    wPtr->resizing = 0;

    /* Update geometry request */
    if (wPtr->useHeader && wPtr->headerDirty) {
	Tix_HLComputeHeaderGeometry(wPtr);
    }

    if (wPtr->root->dirty || wPtr->allDirty) {
	if (wPtr->useIndicator) {
	    /*
	     * If we use indicator, then the toplevel elements are indented
	     * by wPtr->indent. Otherwise they are indented by 0 pixels
	     */
	    ComputeElementGeometry(wPtr, wPtr->root, wPtr->indent);
	} else {
	    ComputeElementGeometry(wPtr, wPtr->root, 0);
	}
    }
    width = 0;
    for (i=0; i<wPtr->numColumns; i++) {
	if (wPtr->reqSize[i].width != UNINITIALIZED) {
	    wPtr->actualSize[i].width = wPtr->reqSize[i].width;
	}
	else {
	    /* This is the req size of the entry columns */
	    int entReq = wPtr->root->col[i].width;

	    /* This is the req size of the header columns */
	    int hdrReq = wPtr->headers[i]->width;

	    if (wPtr->useHeader && hdrReq > entReq) {
		wPtr->actualSize[i].width = hdrReq;
	    } else {
		wPtr->actualSize[i].width = entReq;
	    }
	}
	width += wPtr->actualSize[i].width;
    }
    sizeChanged = 1;
    wPtr->allDirty = 0;

    wPtr->totalSize[0] = width;
    wPtr->totalSize[1] = wPtr->root->allHeight;

    if (wPtr->width > 0) {
	reqW = wPtr->width * wPtr->scrollUnit[0];
    } else {
	reqW = width;
    }
    if (wPtr->height > 0) {
	reqH = wPtr->height * wPtr->scrollUnit[1];
    } else {
	reqH = wPtr->root->allHeight;
    }

    wPtr->totalSize[0] += 2*wPtr->borderWidth + 2*wPtr->highlightWidth;
    wPtr->totalSize[1] += 2*wPtr->borderWidth + 2*wPtr->highlightWidth;
    reqW	       += 2*wPtr->borderWidth + 2*wPtr->highlightWidth;
    reqH	       += 2*wPtr->borderWidth + 2*wPtr->highlightWidth;

    if (wPtr->useHeader) {
	reqH	       += wPtr->headerHeight;
    }

    /* Now we need to handle the multiple columns mode */

    Tk_GeometryRequest(wPtr->dispData.tkwin, reqW, reqH);

    /* Update scrollbars */
    UpdateScrollBars(wPtr, sizeChanged);

    RedrawWhenIdle(wPtr);
}

/*
 *----------------------------------------------------------------------
 * Tix_HLResizeWhenIdle --
 *----------------------------------------------------------------------
 */
void
Tix_HLResizeWhenIdle(wPtr)
    WidgetPtr wPtr;
{
    if (!wPtr->resizing) {
	wPtr->resizing = 1;
	Tk_DoWhenIdle(Tix_HLComputeGeometry, (ClientData)wPtr);
    }
    if (wPtr->redrawing) {
	CancelRedrawWhenIdle(wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 * Tix_HLResizeNow --
 *----------------------------------------------------------------------
 */
void
Tix_HLResizeNow(wPtr)
    WidgetPtr wPtr;
{
    if (wPtr->resizing) {
	wPtr->resizing = 0;
	Tk_CancelIdleCall(Tix_HLComputeGeometry, (ClientData)wPtr);
	Tix_HLComputeGeometry((ClientData)wPtr);
    }
}

/*
 *----------------------------------------------------------------------
 * Tix_HLCancelResizeWhenIdle --
 *----------------------------------------------------------------------
 */
void
Tix_HLCancelResizeWhenIdle(wPtr)
    WidgetPtr wPtr;
{
    if (wPtr->resizing) {
	wPtr->resizing = 0;
	Tk_CancelIdleCall(Tix_HLComputeGeometry, (ClientData)wPtr);
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

/*----------------------------------------------------------------------
 * DItemSizeChangedProc --
 *
 *	This is called whenever the size of one of the HList's items
 *	changes its size.
 *----------------------------------------------------------------------
 */
static void DItemSizeChangedProc(iPtr)
    Tix_DItem *iPtr;
{
    HLItemTypeInfo * info = (HLItemTypeInfo *)iPtr->base.clientData;
    HListColumn * colPtr;
    HListElement * chPtr;
    HListHeader * hPtr;
    WidgetPtr wPtr;

    if (info == NULL) {
	/* Perhaps we haven't set the clientData yet! */
	return;
    }

    switch (info->type) {
      case HLTYPE_COLUMN:
	colPtr = (HListColumn*) info;
	chPtr = colPtr->chPtr;

	if (chPtr) {	/* Sanity check */
	    Tix_HLMarkElementDirty(chPtr->wPtr, chPtr);
	    Tix_HLResizeWhenIdle(chPtr->wPtr);
	}
	break;
      case HLTYPE_HEADER:
	hPtr = (HListHeader*)info;
	wPtr = hPtr->wPtr;
	wPtr->headerDirty = 1;
	if (wPtr->useHeader) {
	    Tix_HLResizeWhenIdle(wPtr);
	}
	break;
      case HLTYPE_ENTRY:
	chPtr = (HListElement*)info;

	if (chPtr) {	/* Sanity check */
	    Tix_HLMarkElementDirty(chPtr->wPtr, chPtr);
	    Tix_HLResizeWhenIdle(chPtr->wPtr);
	}
	break;
    }
}

/*
 *--------------------------------------------------------------
 *
 * AllocElement --
 *
 *	Allocates a new structure for the new element and record it
 *	in the hash table
 *
 * Results:
 *	a pointer to the new element's structure
 *
 * Side effects:
 *	Has table is changed
 *--------------------------------------------------------------
 */
static HListElement *
AllocElement(wPtr, parent, pathName, name, ditemType)
    WidgetPtr wPtr;
    HListElement * parent;
    CONST84 char * pathName;
    CONST84 char * name;
    CONST84 char * ditemType;
{
    HListElement      * chPtr;
    Tcl_HashEntry     * hashPtr;
    int			dummy;
    Tix_DItem	      * iPtr;

    if (ditemType == NULL) {
	iPtr = NULL;
    } else {
	if ((iPtr = Tix_DItemCreate(&wPtr->dispData, ditemType)) == NULL) {
	    return NULL;
	}
    }

    chPtr = (HListElement*)ckalloc(sizeof(HListElement));

    if (pathName) {
	/* pathName == 0 is the root element */
	hashPtr = Tcl_CreateHashEntry(&wPtr->childTable, pathName, &dummy);
	Tcl_SetHashValue(hashPtr, (char*)chPtr);
    }

    if (parent) {
	++ parent->numCreatedChild;
    }

    if (wPtr->numColumns > 1) {
	chPtr->col		= Tix_HLAllocColumn(wPtr, chPtr);
    } else {
	chPtr->col		= &chPtr->_oneCol;
	chPtr->_oneCol.type	= HLTYPE_COLUMN;
	chPtr->_oneCol.self	= (char*) &chPtr->_oneCol;
	chPtr->_oneCol.chPtr	= chPtr;
	chPtr->_oneCol.iPtr	= NULL;
	chPtr->_oneCol.width	= 0;
    }
    if (pathName) {
	chPtr->pathName		= tixStrDup(pathName);
    } else {
	chPtr->pathName		= NULL;
    }

    if (name) {
	chPtr->name		= tixStrDup(name);
    } else {
	chPtr->name		= NULL;
    }

    chPtr->type = HLTYPE_ENTRY;
    chPtr->self = (char*)chPtr;
    chPtr->wPtr			= wPtr;
    chPtr->parent		= parent;
    chPtr->prev			= NULL;
    chPtr->next			= NULL;
    chPtr->childHead		= NULL;
    chPtr->childTail		= NULL;
    chPtr->numSelectedChild	= 0;
    chPtr->numCreatedChild	= 0;
    chPtr->col[0].iPtr		= iPtr;
    chPtr->indicator		= NULL;

    chPtr->height		= 0;
    chPtr->allHeight		= 0;
    chPtr->selected		= 0;
    chPtr->dirty		= 0;
    chPtr->hidden		= 0;
    chPtr->state		= tixNormalUid;
    chPtr->data			= NULL;
    chPtr->branchX		= 0;
    chPtr->branchY		= 0;

    if (iPtr) {
	/* The clientdata is usedful for the DItemSizeChangedProc() */
	iPtr->base.clientData = (ClientData)&chPtr->col[0];
    }

    return chPtr;
}

static void
FreeElement(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    Tcl_HashEntry * hashPtr;
    int i;

    if (chPtr->selected) {
	HL_SelectionClear(wPtr, chPtr);
    }
    if (wPtr->anchor == chPtr) {
	wPtr->anchor = NULL;
    }
    if (wPtr->dragSite == chPtr) {
	wPtr->dragSite = NULL;
    }
    if (wPtr->dropSite == chPtr) {
	wPtr->dropSite = NULL;
    }

    /*
     * Free all the display items
     */
    for (i=0; i<wPtr->numColumns; i++) {
	if (chPtr->col[i].iPtr) {
	    if (Tix_DItemType(chPtr->col[i].iPtr) == TIX_DITEM_WINDOW) {
		Tix_WindowItemListRemove(&wPtr->mappedWindows,
		    chPtr->col[i].iPtr);
	    }
	    Tix_DItemFree(chPtr->col[i].iPtr);
	}
    }
    if (chPtr->indicator != NULL) {
	if (Tix_DItemType(chPtr->indicator) == TIX_DITEM_WINDOW) {
	    Tix_WindowItemListRemove(&wPtr->mappedWindows,
		chPtr->indicator);
	}
	Tix_DItemFree(chPtr->indicator);
    }

    if (chPtr->col != &chPtr->_oneCol) {
	/*
	 * This space was allocated dynamically
	 */
	ckfree((char*)chPtr->col);
    }

    if (chPtr->pathName) {
	/*
	 * Root does not have an entry in the hash table
	 */
	if ((hashPtr = Tcl_FindHashEntry(&wPtr->childTable, chPtr->pathName))){
	    Tcl_DeleteHashEntry(hashPtr);
	}
    }
    if (chPtr->name != NULL) {
	ckfree(chPtr->name);
    }
    if (chPtr->pathName != NULL) {
	ckfree(chPtr->pathName);
    }
    if (chPtr->data != NULL) {
	ckfree(chPtr->data);
    }

    ckfree((char*)chPtr);
}

static void
AppendList(wPtr, parent, chPtr, at, afterPtr, beforePtr)
    WidgetPtr wPtr;
    HListElement *parent;
    HListElement *chPtr;
    int at;			/* At what position should this entry be added
				 * default is "-1": add at the end */
    HListElement *afterPtr;	/* after which entry should this entry be
				 * added. Default is NULL : ignore */
    HListElement *beforePtr;	/* before which entry should this entry be
				 * added. Default is NULL : ignore */
{
    if (parent->childHead == NULL) {
	parent->childHead = chPtr;
	parent->childTail = chPtr;
	chPtr->prev = NULL;
	chPtr->next = NULL;
    }
    else {
	if (at >= 0) {
	    /*
	     * Find the current element at the "at" position
	     */
	    HListElement *ptr;
	    for (ptr=parent->childHead;
		 ptr!=NULL && at > 0;
		 ptr=ptr->next, --at) {
		; /* do nothing, just keep counting */
	    }
	    if (ptr != NULL) {
		/*
		 * We need to insert the new element *before* ptr.E.g,
		 * if at == 0, then the new element should be the first
		 * of the list
		 */
		beforePtr = ptr;
	    } else {
		/* Seems like we walked past the end of the list. Well, do
		 * nothing here. By default, the new element will be
		 * append to the end of the list
		 */
	    }
	}
	if (afterPtr != NULL) {
	    if (afterPtr == parent->childTail) {
		parent->childTail = chPtr;
	    } else {
		afterPtr->next->prev = chPtr;
	    }
	    chPtr->prev = afterPtr;
	    chPtr->next = afterPtr->next;
	    afterPtr->next = chPtr;
	    return;
	}
	if (beforePtr !=NULL) {
	    if (beforePtr == parent->childHead) {
		parent->childHead = chPtr;
	    } else {
		beforePtr->prev->next = chPtr;
	    }
	    chPtr->prev = beforePtr->prev;
	    chPtr->next = beforePtr;
	    beforePtr->prev = chPtr;
	    return;
	}

	/*
	 * By default, append it at the end of the list
	 */
	parent->childTail->next = chPtr;
	chPtr->prev = parent->childTail;
	chPtr->next = NULL;
	parent->childTail = chPtr;
    }
}

/*
 *--------------------------------------------------------------
 *
 * NewElement --
 *
 *	This procedure is creates a new element and record it both
 *	the hash table and in the tree.
 *
 * Results:
 *	pointer to new element
 *
 * Side effects:
 *	Hash table and tree changed if successful
 *--------------------------------------------------------------
 */
static HListElement *
NewElement(interp, wPtr, argc, argv, pathName, defParentName, newArgc)
    Tcl_Interp *interp;
    WidgetPtr wPtr;
    int argc;
    CONST84 char ** argv;
    CONST84 char * pathName;	/* Default pathname, if -pathname is not
				 * specified in the options */
    CONST84 char * defParentName;/* Default parent name (will NULL if pathName 
				 * is not NULL */
    int * newArgc;
{
#define FIXED_SPACE 20
    char fixedSpace[FIXED_SPACE+1];
    CONST84 char *p;
    CONST84 char *name;			/* Last part of the name */
    char *parentName = NULL;
    int i, n, numChars;
    HListElement *parent;
    HListElement *chPtr;
    char sep = wPtr->separator[0];
    int allocated = 0;
    CONST84 char * ditemType = NULL;
    HListElement *afterPtr  = NULL;
    HListElement *beforePtr = NULL;
    int at = -1;
    int numSwitches = 0;		/* counter on how many of the
					 * -after, -before and -at switches
					 * have been used. No more than one
					 * of then can be used */

    /*
     * (1) We need to determine the options:
     *	   -itemtype, -after, -before and/or -at.
     *
     */
    if (argc > 0) {
	size_t len;
	if (argc %2 != 0) {
	    Tcl_AppendResult(interp, "value for \"", argv[argc-1],
		"\" missing", NULL);
	    chPtr = NULL;
	    goto done;
	}
	for (n=i=0; i<argc; i+=2) {
	    len = strlen(argv[i]);
	    if (strncmp(argv[i], "-itemtype", len) == 0) {
		ditemType = argv[i+1];
		goto copy;
	    }
	    else if (strncmp(argv[i], "-after", len) == 0) {
		afterPtr = Tix_HLFindElement(interp, wPtr, argv[i+1]);
		if (afterPtr == NULL) {
		    chPtr = NULL;
		    goto done;
		}
		++ numSwitches;
		continue;
	    }
	    else if (strncmp(argv[i], "-before", len) == 0) {
		beforePtr = Tix_HLFindElement(interp, wPtr, argv[i+1]);
		if (beforePtr == NULL) {
		    chPtr = NULL;
		    goto done;
		}
		++ numSwitches;
		continue;
	    }
	    else if (strncmp(argv[i], "-at", len) == 0) {
		if (Tcl_GetInt(interp, argv[i+1], &at) != TCL_OK) {
		    chPtr = NULL;
		    goto done;
		}
		++ numSwitches;
		continue;
	    }

	  copy:
	    if (n!=i) {
		argv[n] = argv[i];
		argv[n+1] = argv[i+1];
	    }
	    n+=2;
	}
	* newArgc = n;
    } else {
	* newArgc = 0;
    }
    if (numSwitches > 1) {
	Tcl_AppendResult(interp, "No more than one of the -after, -before ",
	    "and -at options can be used", NULL);
	chPtr = NULL;
	goto done;
    }
    if (ditemType == NULL) {
	ditemType = wPtr->diTypePtr->name;
    }
    if (Tix_GetDItemType(interp, ditemType) == NULL) {
	chPtr = NULL;
	goto done;
    }

    /*------------------------------------------------------------
     * (2) Create the new entry. The method depends on whether
     *	   the "add" or "addchild" command has been called
     *------------------------------------------------------------
     */
    if (pathName == NULL) {
	/* (2.a) Called by the "addchild" command. We need to generate
	 *     a default name for the child
	 *
	 */
	char buff[40];

	parentName = (char *) defParentName;
	if (parentName == NULL) {
	    parent = wPtr->root;
	} else {
	    if ((parent=Tix_HLFindElement(interp, wPtr, parentName))== NULL) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "parent element \"", parentName,
		    "\" does not exist", (char *) NULL);
		chPtr = NULL;
		goto done;
	    }
	}

	/* 
         * Generate a default name for this entry
         *
         * TODO: make sure this name is unique!
         */
	sprintf(buff, "%d", parent->numCreatedChild);
	name = buff;

	if (parentName == NULL) {
	    pathName = tixStrDup(name);
	    allocated = 1;
	}
	else {
	    pathName = ckalloc(strlen(parentName)+1+ strlen(name)+1);
	    allocated = 1;
	    sprintf((char *) pathName, "%s%c%s", parentName, sep, name);
	}
    }
    else {
	/* (2.b) Called by the "add" command.
	 *
	 * Strip the parent's name out of pathName (it's everything up
	 * to the last dot).  There are two tricky parts: (a) must
	 * copy the parent's name somewhere else to avoid modifying
	 * the pathName string (for large names, space for the copy
	 * will have to be malloc'ed);	(b) must special-case the
	 * situation where the parent is ".".
	 */

	if ((p = strrchr(pathName, (int)sep)) == NULL) {
	    /* This is a toplevel element  (no "." in it) */
	    name = pathName;
	    parentName = NULL;
	}
	else {
	    name = p+1;
	    numChars = p-pathName;
	    if (numChars > FIXED_SPACE) {
		parentName = (char *) ckalloc((unsigned)(numChars+1));
	    } else {
		parentName = fixedSpace;
	    }
	    if (numChars == 0) {
		if ((pathName[0] == sep) && (pathName[1] == '\0')) {
		    /*
		     * The separator by itself is also a toplevel entry
		     */		      
		    parentName = 0;
		} else {
		    parentName[0] = sep;
		    parentName[1] = '\0';
		}
	    }
	    else {
		strncpy(parentName, pathName, (size_t) numChars);
		parentName[numChars] = '\0';
	    }
	}

	if (parentName == NULL) {
	    parent = wPtr->root;
	} else {
	    if ((parent = Tix_HLFindElement(interp, wPtr, parentName))==NULL) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "parent element \"", parentName,
		"\" does not exist", (char *) NULL);
		chPtr = NULL;
		goto done;
	    }
	}

    }
    if (Tix_HLFindElement(interp, wPtr, pathName) != NULL) {
	Tcl_AppendResult(interp, "element \"", pathName,
	    "\" already exists", (char *) NULL);
	chPtr = NULL;
	goto done;
    }
    else {
	if (afterPtr != NULL && afterPtr->parent != parent) {
	    Tcl_AppendResult(interp, "cannot add entry after \"",
		afterPtr->pathName, "\"", NULL);
	    chPtr = NULL;
	    goto done;
	}
	if (beforePtr != NULL && beforePtr->parent != parent) {
	    Tcl_AppendResult(interp, "cannot add entry before \"",
		beforePtr->pathName, "\"", NULL);
	    chPtr = NULL;
	    goto done;
	}

	Tcl_ResetResult(interp);
	if ((chPtr = AllocElement(wPtr, parent, pathName, name, ditemType))
	     == NULL) {
	    /* Some error, now chPtr == NULL */
	    goto done;
	}
	AppendList(wPtr, parent, chPtr, at, afterPtr, beforePtr);
	Tix_HLMarkElementDirty(wPtr, chPtr);
	Tix_HLResizeWhenIdle(wPtr);
	goto done;		/* success */
    }

  done:
    if (allocated) {
	ckfree((char*)pathName);
    }
    if (parentName && parentName != fixedSpace && parentName !=defParentName) {
	ckfree((char*)parentName);
    }
    return chPtr;
}

/*--------------------------------------------------------------
 * ConfigElement --
 *
 *	This procedure configures the element according to the
 *	options.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Hash table and tree changed if successful
 *--------------------------------------------------------------
 */

static int
ConfigElement(wPtr, chPtr, argc, argv, flags, forced)
    WidgetPtr wPtr;
    HListElement *chPtr;
    int argc;
    CONST84 char ** argv;
    int flags;
    int forced;			/* We need a "forced" configure to ensure that
				 * the DItem is initialized properly */
{
    int sizeChanged;

    if (Tix_WidgetConfigure2(wPtr->dispData.interp, wPtr->dispData.tkwin,
	(char*)chPtr, entryConfigSpecs, chPtr->col[0].iPtr, argc, argv, flags,
	forced, &sizeChanged) != TCL_OK) {
	return TCL_ERROR;
    }

    if (sizeChanged) {
	Tix_HLMarkElementDirty(wPtr, chPtr);
	Tix_HLResizeWhenIdle(wPtr);
    } else {
	RedrawWhenIdle(wPtr);
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * FindElementAtPosition --
 *
 *	Finds a visible element nearest to a Y position
 *
 * Results:
 *	Pointer to the element.
 *
 * Side effects:
 *	None
 *--------------------------------------------------------------
 */
static HListElement * FindElementAtPosition(wPtr, y)
    WidgetPtr wPtr;
    int y;
{
    HListElement * chPtr = wPtr->root;
    int top = 0;

    y -= wPtr->borderWidth + wPtr->highlightWidth;
    y += wPtr->topPixel;

    if (wPtr->useHeader) {
	y -= wPtr->headerHeight;
    }

    if (y < 0) {
	/*
	 * Position is above the top of the list, return the first element in
	 * the list of toplevel entries.
	 */
	if (wPtr->root != NULL) {
	    for (chPtr=wPtr->root->childHead; chPtr!=NULL; chPtr=chPtr->next) {
		if (!chPtr->hidden) {
		    return chPtr;
		}
	    }
	}
	return NULL;
    }
    if (y >= chPtr->allHeight) {
	/*
	 * Position is past the end of the list, return the last element.
	 */
	HListElement * vis;

	chPtr=wPtr->root;
	while (1) {
	    if (chPtr->childTail == NULL) {
		break;
	    }
	    for (vis = chPtr->childTail; vis && vis->hidden; vis=vis->prev) {
		;
	    }
	    if (vis == NULL) {
		break;
	    } else {
		chPtr = vis;
		continue;
	    }
	}
	if (chPtr == wPtr->root) {
	    /*
	     * There is either no element, or all elements are not visible
	     */
	    return NULL;
	} else {
	    return chPtr;
	}
    }

    /*
     * The following is a tail-recursive function flatten out in a while
     * loop.
     */

    while (1) {
    again:
	for (chPtr=chPtr->childHead; chPtr!=NULL; chPtr=chPtr->next) {
	    if (!chPtr->hidden) {
		if (top <= y && y < top + chPtr->allHeight) {
		    if (y < top + chPtr->height) {
			return chPtr;
		    } else {
			top += chPtr->height;
			goto again;
		    }
		} else {
		    top += chPtr->allHeight;
		}
	    }
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tix_HLFindElement --
 *
 *	Finds an element according to its pathname.
 *
 * Results:
 *	Pointer to the element if found. Otherwise NULL.
 *
 * Side effects:
 *	None
 *--------------------------------------------------------------
 */
HListElement * Tix_HLFindElement(interp, wPtr, pathName)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    CONST84 char * pathName;
{
    Tcl_HashEntry     * hashPtr;

    if (pathName) {
	hashPtr = Tcl_FindHashEntry(&wPtr->childTable, pathName);

	if (hashPtr) {
	    return (HListElement*) Tcl_GetHashValue(hashPtr);
	} else {
	    Tcl_AppendResult(interp, "Entry \"", pathName,
		"\" not found", NULL);
	    return NULL;
	}
    }
    else {
	/* pathName == 0 is the root element */
	return wPtr->root;
    }
}

/*
 *--------------------------------------------------------------
 *
 * SelectionModifyRange --
 *
 *	Select or de-select all the elements between from and to 
 *	(inclusive), according to the "select" argument.
 *
 *	select == 1 : select
 *	select == 0 : de-select
 *
 * Return value:
 *	Whether the selection was actually changed
 *--------------------------------------------------------------
 */
static int SelectionModifyRange(wPtr, from, to, select)
    WidgetPtr wPtr;
    HListElement * from;
    HListElement * to;
    int select;
{
    int changed = 0;

    if (Tix_HLElementTopOffset(wPtr, from) > Tix_HLElementTopOffset(wPtr, to)){
	HListElement * tmp;
	tmp  = to;
	to   = from;
	from = tmp;
    }

    while (1) {
	if (!from->hidden && (int)from->selected != select) {
	    if (select) {
		SelectionAdd(wPtr, from);
	    } else {
		HL_SelectionClear(wPtr, from);
		changed = 1;
	    }
	}

	if (from == to) {
	    /*
	     * Iterated to the end of the region
	     */
	    break;
	}

	/*
	 * Go to the next list entry
	 */
	if (from->childHead) {
	    from = from->childHead;
	}
	else if (from->next) {
	    from = from->next;
	}
	else {
	    /*
	     * go to a different branch
	     */
	    while (from->parent->next == NULL && from != wPtr->root) {
		from = from->parent;
	    }
	    if (from == wPtr->root) {
		/*
		 * Iterated over all list entries
		 */
		break;
	    } else {
		from = from->parent->next;
	    }
	}
    }

    return changed;
}

/*
 *--------------------------------------------------------------
 *
 * Tix_HLElementTopOffset --
 *
 *--------------------------------------------------------------
 */
int Tix_HLElementTopOffset(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    int top;
    HListElement * ptr;

    if (chPtr == wPtr->root) {
	return 0;
    }
    top = Tix_HLElementTopOffset(wPtr, chPtr->parent);
    top += chPtr->parent->height;

    for (ptr=chPtr->parent->childHead; ptr!=NULL; ptr=ptr->next) {
	if (ptr == chPtr) {
	    break;
	}
	if (ptr->hidden) {
	    continue;
	}
	top += ptr->allHeight;
    }
    return top;
}

/*
 *--------------------------------------------------------------
 *
 * Tix_HLElementLeftOffset --
 *
 *--------------------------------------------------------------
 */
int Tix_HLElementLeftOffset(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    int left;

    if (chPtr == wPtr->root || chPtr->parent == wPtr->root) {
	return 0;
    }

    left = Tix_HLElementLeftOffset(wPtr, chPtr->parent);
    left += wPtr->indent;

    return left;
}

/*
 *--------------------------------------------------------------
 *
 * CurSelection --
 *
 *	returns the current selection in the result of interp;
 *
 *--------------------------------------------------------------
 */
static int CurSelection(interp, wPtr, chPtr)
    Tcl_Interp * interp;
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    HListElement * ptr;

    /* Since this recursion starts with wPtr->root, we determine
     * whether a node is selected when its *parent* is called. This
     * will save one level of recursion (otherwise all leave nodes will
     * be recursed once and will be slow ...
     */
    for (ptr=chPtr->childHead; ptr; ptr=ptr->next) {
	if (ptr->selected && !(ptr->hidden)) {
	    Tcl_AppendElement(interp, ptr->pathName);
	}
	if (ptr->childHead) {
	    CurSelection(interp, wPtr, ptr);
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tix_HLMarkElementDirty --
 *
 *	Marks a element "dirty", i.e., its geometry needs to be
 *	recalculated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The element and all its ancestores are marked dirty
 *--------------------------------------------------------------
 */
void Tix_HLMarkElementDirty(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement *chPtr;
{
    HListElement *ptr;

    for (ptr=chPtr; ptr!= NULL && ptr->dirty == 0; ptr=ptr->parent) {
	ptr->dirty = 1;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeElementGeometry --
 *
 *	Compute the geometry of this element (if its dirty) and the
 *	geometry of all its dirty child elements
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The element and all its decendants are marked non-dirty
 *--------------------------------------------------------------
 */

static void ComputeElementGeometry(wPtr, chPtr, indent)
    WidgetPtr wPtr;
    HListElement *chPtr;
    int indent;
{
    HListElement *ptr;
    int i;

    if (!chPtr->dirty && !wPtr->allDirty) {
	return;
    } else {
	chPtr->dirty = 0;
    }

    if (chPtr == wPtr->root) {
	int i;
	chPtr->height = 0;
	chPtr->indent = 0;
	for (i=0; i<wPtr->numColumns; i++) {
	    chPtr->col[i].width = 0;
	}
    } else {
	ComputeOneElementGeometry(wPtr, chPtr, indent);
	indent += wPtr->indent;
    }

    chPtr->allHeight = chPtr->height;

    for (ptr=chPtr->childHead; ptr!=NULL; ptr=ptr->next) {
	if (ptr->hidden) {
	    continue;
	}
	if (ptr->dirty || wPtr->allDirty) {
	    ComputeElementGeometry(wPtr, ptr, indent);
	}

	/* Propagate the child's size to the parent 
	 *
	 */
	for (i=0; i<wPtr->numColumns; i++) {
	    if (chPtr->col[i].width < ptr->col[i].width) {
		chPtr->col[i].width = ptr->col[i].width;
	    }
	}
	chPtr->allHeight += ptr->allHeight;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeOneElementGeometry --
 *
 *	Compute the geometry of the element itself, not including 
 *	its children, according to its current display type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The chPtr->height fields are updated.
 *--------------------------------------------------------------
 */
static void ComputeOneElementGeometry(wPtr, chPtr, indent)
    WidgetPtr wPtr;
    HListElement *chPtr;
    int indent;
{
    int i;

    chPtr->indent = indent;
    chPtr->height = 0;

    ComputeBranchPosition(wPtr, chPtr);

    for (i=0; i<wPtr->numColumns; i++) {
	Tix_DItem * iPtr = chPtr->col[i].iPtr;
	int width  = 2*wPtr->selBorderWidth;
	int height = 2*wPtr->selBorderWidth;

	if (iPtr != NULL) {
	    Tix_DItemCalculateSize(iPtr);
	    /*
             * Tix_DItemWidth() and Tix_DItemHeight() already include padding
	     */
	    width  += Tix_DItemWidth (iPtr);
	    height += Tix_DItemHeight(iPtr);
	}
	if (chPtr->height < height) {
	    chPtr->height = height;
	}
	chPtr->col[i].width = width;
    }
    chPtr->col[0].width += indent;
}

/*
 *--------------------------------------------------------------
 *
 * ComputeBranchPosition --
 *
 *	Compute the position of the branches
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The chPtr->branchX and chPtr->branchY fields are updated.
 *--------------------------------------------------------------
 */
static void ComputeBranchPosition(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement *chPtr;
{
    Tix_DItem * iPtr = chPtr->col[0].iPtr;
    int branchX, branchY;
    int iconX;
    int iconY;
    int diff;

    if (iPtr) {
	if (Tix_DItemType(iPtr) == TIX_DITEM_IMAGETEXT) {
	    /*
	     * Calculate the bottom-middle position of the bitmap/image branch
	     */
	    if (iPtr->imagetext.image != NULL) {
		branchX = iPtr->imagetext.imageW / 2;
		branchY = iPtr->imagetext.imageH;
		if (Tix_DItemHeight(iPtr) > iPtr->imagetext.imageH) {
                    int diff = Tix_DItemHeight(iPtr) - iPtr->imagetext.imageH;
		    branchY += diff /2;
                    if (diff % 2) {
                        branchY += 1;
                    }
		}
	    }
	    else if (iPtr->imagetext.bitmap != None) {
		branchX = iPtr->imagetext.bitmapW / 2;
		branchY = iPtr->imagetext.bitmapH;
		if (Tix_DItemHeight(iPtr) >iPtr->imagetext.bitmapH) {
                    int diff = Tix_DItemHeight(iPtr) - iPtr->imagetext.bitmapH;
		    branchY += diff /2;
                    if (diff % 2) {
                        branchY += 1;
                    }
		}
	    }
	    else {
		branchX = wPtr->indent/2;
		branchY = Tix_DItemHeight(iPtr);
	    }
	} else {
	    branchX = wPtr->indent/2;
	    branchY = Tix_DItemHeight(iPtr);
	}


	/* X adjustment
	 */
	iconX = Tix_DItemPadX(iPtr);
	branchX += Tix_DItemPadX(iPtr);

	/* Y adjustment
	 */
	iconY = Tix_DItemHeight(iPtr) / 2;
	diff = chPtr->height - Tix_DItemHeight(iPtr);
	if (diff > 0) {
	    switch (iPtr->base.stylePtr->anchor) {
	      case TK_ANCHOR_NW: case TK_ANCHOR_N: case TK_ANCHOR_NE:
		diff = 0;
		break;
	      case TK_ANCHOR_W: case TK_ANCHOR_CENTER: case TK_ANCHOR_E:
		diff /= 2;
		break;
	      default:
		/* Do nothing */
		;
	    }
	    branchY += diff;
	    iconY   += diff;
	}
    }
    else {
	branchX = wPtr->indent/2;
	branchY = chPtr->height;
	iconX	= 0;
	iconY	= chPtr->height/2;
    }

    if (wPtr->useIndicator && chPtr->parent == wPtr->root) {
	branchX += wPtr->indent;
    }

    chPtr->branchX = branchX - 1;
    chPtr->branchY = branchY - 1;
    chPtr->iconX   = iconX   - 1;
    chPtr->iconY   = iconY   - 1;

    if (chPtr->branchX < 0) {
	chPtr->branchX = 0;
    }
    if (chPtr->branchY < 0) {
	chPtr->branchY = 0;
    }
    if (chPtr->iconX < 0) {
	chPtr->iconX = 0;
    }
    if (chPtr->iconY < 0) {
	chPtr->iconY = 0;
    }

    chPtr->branchX += wPtr->selBorderWidth;
    chPtr->branchY += wPtr->selBorderWidth;
    chPtr->iconX   += wPtr->selBorderWidth;
    chPtr->iconY   += wPtr->selBorderWidth;
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
    Drawable buffer;
    Tk_Window tkwin = wPtr->dispData.tkwin;
    int elmX, elmY;
    Tcl_Interp *interp = wPtr->dispData.interp;

    wPtr->redrawing = 0;		/* clear the redraw flag */
    wPtr->serial ++;

    if (wPtr->elmToSee != NULL) {
	HListElement *chPtr;
	  
	if ((chPtr = Tix_HLFindElement(interp, wPtr,
		wPtr->elmToSee)) == NULL) {
	    Tcl_ResetResult(interp);
	} else {
	    Tix_HLSeeElement(wPtr, chPtr, 0);
	    UpdateScrollBars(wPtr, 0);
	}

	ckfree(wPtr->elmToSee);
	wPtr->elmToSee = NULL;
    }


    /*
     *	STEP (1)
     *		Calculate the drawing parameters
     */
    if (wPtr->wideSelect) {
	wPtr->selectWidth = Tk_Width(wPtr->dispData.tkwin) - 
	  (2*wPtr->borderWidth + 2*wPtr->highlightWidth);
	if (wPtr->selectWidth < wPtr->totalSize[0]) {
	    wPtr->selectWidth = wPtr->totalSize[0];
	}
    }

    /* Used to clip off elements that are too low to see */
    wPtr->bottomPixel = Tk_Height(wPtr->dispData.tkwin) - 2*wPtr->borderWidth
      - 2*wPtr->highlightWidth;

    elmX = wPtr->borderWidth + wPtr->highlightWidth - wPtr->leftPixel;
    elmY = wPtr->borderWidth + wPtr->highlightWidth - wPtr->topPixel;

    if (wPtr->useHeader) {
	elmY += wPtr->headerHeight;
    }

    /*
     *	STEP (2)
     *		Draw the list body
     */
    buffer = Tix_GetRenderBuffer(wPtr->dispData.display, Tk_WindowId(tkwin),
	Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));

    /* Fill the background */
    XFillRectangle(wPtr->dispData.display, buffer, wPtr->backgroundGC,
	    0, 0, (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin));

    DrawElements(wPtr, buffer, wPtr->root, elmX, elmY,
	wPtr->borderWidth + wPtr->highlightWidth - wPtr->leftPixel);

    if (wPtr->borderWidth > 0) {
	/* Draw the border */
	Tk_Draw3DRectangle(wPtr->dispData.tkwin, buffer, wPtr->border,
	    wPtr->highlightWidth, wPtr->highlightWidth,
	    Tk_Width(tkwin)  - 2*wPtr->highlightWidth, 
	    Tk_Height(tkwin) - 2*wPtr->highlightWidth, wPtr->borderWidth,
	    wPtr->relief);
    }

    if (wPtr->highlightWidth > 0) {
	/* Draw the highlight */
	GC gc;

	if (wPtr->hasFocus) {
	    gc = wPtr->highlightGC;
	} else {
	    gc = Tk_3DBorderGC(wPtr->dispData.tkwin, wPtr->border,
		TK_3D_FLAT_GC);
	}
	Tk_DrawFocusHighlight(tkwin, gc, wPtr->highlightWidth, buffer);
    }

    if (buffer != Tk_WindowId(tkwin)) {
	/*
	 * Copy the information from the off-screen pixmap onto the screen,
	 * then delete the pixmap.
	 */

	XCopyArea(wPtr->dispData.display, buffer, Tk_WindowId(tkwin),
		wPtr->normalGC, 0, 0,
		(unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin), 0, 0);
	Tk_FreePixmap(wPtr->dispData.display, buffer);
    }

    /*
     *	STEP (3)
     *		Draw the header
     */
    if (wPtr->useHeader) {
	/* We need to draw the header after the elements, because some
	 * half-scrolled elements may overwrite the space for the header
	 */
	int hdrX, hdrY, hdrW, hdrH, pad, xOffset;
	Drawable buffer;

	pad  = wPtr->borderWidth + wPtr->highlightWidth;
	hdrX = pad;
	hdrY = pad;
	hdrW = Tk_Width(tkwin) - 2*pad;
	hdrH = wPtr->headerHeight;
	xOffset = wPtr->leftPixel;

	Tk_MoveResizeWindow(wPtr->headerWin, hdrX, hdrY, hdrW, hdrH);
	Tk_MapWindow(wPtr->headerWin);

	buffer = Tix_GetRenderBuffer(wPtr->dispData.display,
	    Tk_WindowId(wPtr->headerWin), hdrW, hdrH,
	    Tk_Depth(wPtr->headerWin));

	XFillRectangle(wPtr->dispData.display, buffer,
	    wPtr->backgroundGC, 0, 0, (unsigned) hdrW, (unsigned) hdrH);

	Tix_HLDrawHeader(wPtr, buffer, wPtr->normalGC,
	    0, 0, hdrW, hdrH, xOffset);

	if (buffer != Tk_WindowId(wPtr->headerWin)) {
	    XCopyArea(wPtr->dispData.display, buffer, 
		Tk_WindowId(wPtr->headerWin), wPtr->normalGC,
		0, 0, (unsigned) hdrW, (unsigned) hdrH, 0, 0);

	    Tk_FreePixmap(wPtr->dispData.display, buffer);
	}

	/* If we map the header window, that may change the size requirement
	 * of the HList
	 * %% Call only when geometry is *really* changed
	 */
	if (wPtr->sizeCmd) {
	    if (Tcl_GlobalEval(wPtr->dispData.interp, wPtr->sizeCmd)
		 != TCL_OK) {
		Tcl_AddErrorInfo(wPtr->dispData.interp,
		    "\n	   (size command executed by tixHList)");
		Tk_BackgroundError(wPtr->dispData.interp);
	    }
	}
    } else {
	Tk_UnmapWindow(wPtr->headerWin);
    }

    /* unmap those windows we mapped the last time */
    Tix_UnmapInvisibleWindowItems(&wPtr->mappedWindows, wPtr->serial);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawElements() --
 *
 *      Draws an element and all of its visible descendants.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static void
DrawElements(wPtr, drawable, chPtr, x, y, xOffset)
    WidgetPtr wPtr;                     /* Widget to render */
    Drawable drawable;                  /* Drawable to draw into */
    HListElement * chPtr;               /* Draw this element and its visible
                                         * descendants */
    int x;                              /* x position of the element's 
                                         * top-left corner */
    int y;                              /* y position of the element's 
                                         * top-left corner */
    int xOffset;                        /* x-offset due to horizontal
                                         * scrolling */
{
    HListElement * ptr, * lastVisible;
    GC gc = wPtr->normalGC;
    int myIconX = 0, myIconY = 0;		/* center of my icon */
    int childIconX, childIconY;		/* center of child's icon */
    int childY, childX;
    int oldY;
    int top    = wPtr->useHeader ? wPtr->headerHeight : 0;
    int left   = 0;
    int bottom = Tk_Height(wPtr->dispData.tkwin);
    int right  = Tk_Width(wPtr->dispData.tkwin);

    if (chPtr != wPtr->root) {
	if (bottom > y  && (y + chPtr->height) >= top) {
	    /*
             * This element is (at least partialy) visible
             */

	    DrawOneElement(wPtr, drawable, chPtr, x, y, xOffset);
	}
	myIconX = x + chPtr->branchX;
	myIconY = y + chPtr->branchY;

	if (wPtr->useIndicator && chPtr->parent == wPtr->root) {
	    childX = x + 2 * wPtr->indent;
	} else {
	    childX = x +  wPtr->indent;
	}
	childY = y + chPtr->height;
	if (myIconX > childX) {
	    /* Can't shift the vertical branch too much to the right */
	    myIconX = childX;
	}
    } else {
	childX = x;
	childY = y;
    }

    oldY = childY;		/* saved for 2nd iteration */

    /* find the last non-hidden element, 
     * to determine when to draw the vertical line 
     */
    lastVisible = NULL;
    for (ptr = chPtr->childTail; ptr!=NULL; ptr=ptr->prev) {
	if (! ptr->hidden) {
	    lastVisible = ptr;
	    break;
	}
    }

    if (lastVisible == NULL) {
	/* No child is visible */
	return;
    }

    /* First iteration : draw the entries and branches */
    for (ptr = chPtr->childHead; ptr!=NULL; ptr=ptr->next) {
	if (ptr->hidden) {
	    continue;
	}

	childIconX = childX + ptr->iconX;
	childIconY = childY + ptr->iconY;

	if (bottom > childY && (childY + ptr->allHeight) >= top) {
	    DrawElements(wPtr, drawable, ptr, childX, childY, xOffset);

	    if (wPtr->drawBranch && chPtr != wPtr->root
		    && top <= childIconY && childIconY <= bottom) {
		/*
                 * Draw a horizontal branch to the child. We draw the line
                 * after drawing the element, because DrawElement may
                 * clear out some area around the element.
                 */
		XDrawLine(wPtr->dispData.display, drawable, gc, myIconX,
		    childIconY, childIconX, childIconY);
	    }
	}

	/*
	 * -- no branches for toplevel elements
	 * -- for last element, draw a vertical branch, even if element
	 *    is not seen
	 */
	if (ptr == lastVisible      && wPtr->drawBranch
	    && chPtr != wPtr->root  && childIconY >= top
	    && left <= myIconX      && myIconX <= right) {
	    /*
             * clip vertical lines to avoid wrap-around
             */
            int y0 = myIconY < 0 ? 0 : myIconY;
            int y1 = childIconY > bottom ? bottom : childIconY;

	    XDrawLine(wPtr->dispData.display, drawable, gc, myIconX, y0,
		    myIconX, y1);
	}
	childY += ptr->allHeight;
    }

    if (!wPtr->useIndicator) {
	return;
    }
    childY = oldY;

    /* Second iteration : draw the indicators */
    for (ptr = chPtr->childHead; ptr!=NULL; ptr=ptr->next) {
	int cY = childY;

	if (ptr->hidden) {
	    continue;
	}
	childY += ptr->allHeight;
	childIconY = cY + ptr->iconY;

	if (bottom > cY && (cY + ptr->allHeight) >= top
	    && ptr->indicator != NULL
	) {
	    /* Otherwise all descendants of ptr are not seen at all
	     */
	    int justMapped,
	        indW = Tix_DItemWidth (ptr->indicator),
	        indH = Tix_DItemHeight(ptr->indicator),
	        indY = childIconY - indH/2,
	        indX = (chPtr == wPtr->root
		       ? (wPtr->indent / 2 + wPtr->borderWidth
		         + wPtr->highlightWidth - wPtr->leftPixel)
		       : myIconX) - indW/2;
	    if ( indX > right || (indX + indW) < left
		 || indY > bottom ||(indY + indH) < top
	    ) {
		continue;  /* indicator not visible */
	    }

	    justMapped = 0;
	    if (Tix_DItemType(ptr->indicator) == TIX_DITEM_WINDOW) {
		Tix_SetWindowItemSerial(&wPtr->mappedWindows,
		    ptr->indicator, wPtr->serial);
		if (!Tk_IsMapped(ptr->indicator->window.tkwin)) {
		    justMapped = 1;
		}
	    }

	    /* Put down the indicator */
	    Tix_DItemDisplay(drawable, ptr->indicator,
		    indX, indY, indW, indH, 0, 0,
                    TIX_DITEM_NORMAL_FG|TIX_DITEM_NORMAL_BG);

	    if (justMapped) {
		Tk_RestackWindow(ptr->indicator->window.tkwin, Below, NULL);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawOneElement --
 *--------------------------------------------------------------
 */

static void
DrawOneElement(wPtr, drawable, chPtr, x, y, xOffset)
    WidgetPtr wPtr;                     /* Widget to render */
    Drawable drawable;                  /* Drawable to draw into */
    HListElement * chPtr;               /* Draw this element and its visible
                                         * descendants */
    int x;                              /* x position of the element's 
                                         * top-left corner */
    int y;                              /* y position of the element's 
                                         * top-left corner */
    int xOffset;                        /* x-offset due to horizontal
                                         * scrolling */
{
    int flags0 = TIX_DITEM_NORMAL_FG;   /* DItem flags for column 0 */
    int flags1 = TIX_DITEM_NORMAL_FG;   /* DItem flags for column 1 and above*/
    int i;
    int selectWidth, selectX;
    int hasDrawnBackground = 0;

    x = xOffset + chPtr->indent;

    if (wPtr->wideSelect) {
	selectWidth = wPtr->selectWidth;
	selectX = xOffset;
    } else {
	selectWidth = Tix_DItemWidth(chPtr->col[0].iPtr)
	        + 2*wPtr->selBorderWidth;
	selectX = x;
    }

    if (chPtr->selected && wPtr->wideSelect) {
	Tk_Fill3DRectangle(wPtr->dispData.tkwin, drawable, wPtr->selectBorder,
	        selectX, y, selectWidth, chPtr->height, wPtr->selBorderWidth,
	        TK_RELIEF_RAISED);
        hasDrawnBackground = 1;
    }

    if (chPtr->selected) {
	flags0 |= TIX_DITEM_SELECTED_FG;

	if (hasDrawnBackground) {
	    flags1 |= TIX_DITEM_SELECTED_FG;
        } else {
	    flags0 |= TIX_DITEM_SELECTED_BG;
            flags0 |= TIX_DITEM_NORMAL_BG;
	}
    } else {
        flags0 |= TIX_DITEM_NORMAL_BG;
    }

    if (!hasDrawnBackground) {
        flags1 |= TIX_DITEM_NORMAL_BG;
    }

    if (chPtr == wPtr->anchor) {
	flags0 |= TIX_DITEM_ACTIVE_FG;

	if (hasDrawnBackground) {
            flags1 |= TIX_DITEM_ACTIVE_FG;
        } else {
	    flags0 |= TIX_DITEM_ACTIVE_BG;
            if (wPtr->hasFocus && !(wPtr->wideSelect)) {
                flags0 |= TIX_DITEM_ANCHOR;
            }
	}
    }

    if (chPtr == wPtr->dropSite) {
        /*
         * Draw a rectangle to indicate that this is a drop site.
         *
         * TOD: what about wide-select??
         */

	XDrawRectangle(Tk_Display(wPtr->dispData.tkwin), drawable,
	        wPtr->dropSiteGC, selectX, y,
		(unsigned) selectWidth-1, (unsigned) chPtr->height-1);
    }

    /*
     * Now Draw the display items in each column
     *
     * %% ToDo: clip off the non-visible items
     */

    for (x=xOffset, i=0; i<wPtr->numColumns; i++) {
	Tix_DItem * iPtr = chPtr->col[i].iPtr;
	int drawX, drawY, drawW, drawH;
        int drawOffX, drawOffY;
        int flags;

	if (iPtr != NULL) {
	    int justMapped = 0;


	    if (Tix_DItemType(iPtr) == TIX_DITEM_WINDOW) {
		Tix_SetWindowItemSerial(&wPtr->mappedWindows,iPtr,
		    wPtr->serial);
		if (!Tk_IsMapped(iPtr->window.tkwin)) {
		    justMapped = 1;
		}
	    }

            drawX = x;
            drawY = y;
            drawW = wPtr->actualSize[i].width;
            drawH = chPtr->height;

            drawOffX = wPtr->selBorderWidth;
            drawOffY = wPtr->selBorderWidth;

            if (i == 0) {
                flags = flags0;
                drawOffX += chPtr->indent;
            } else {
                flags = flags1;
            }

	    Tix_DItemDisplay(drawable, iPtr, drawX, drawY, drawW, drawH,
                    drawOffX, drawOffY, flags);

	    if (justMapped) {
		/*
		 * We need to lower it so that it doesn't
		 * overlap the header subwindow
		 */

		Tk_RestackWindow(iPtr->window.tkwin, Below, NULL);
	    }
	}

	x += wPtr->actualSize[i].width;
    }

    if (chPtr == wPtr->anchor && wPtr->hasFocus && wPtr->wideSelect) {
	int ancX, ancY, ancW, ancH;
        ancX = selectX + wPtr->selBorderWidth;
        ancY = y       + wPtr->selBorderWidth;
	ancW = selectWidth   - 2*wPtr->selBorderWidth;
	ancH = chPtr->height - 2*wPtr->selBorderWidth;

        /*
         * TODO: use a different anchorGC on normal background
         */
	Tix_DrawAnchorLines(Tk_Display(wPtr->dispData.tkwin), drawable,
	        wPtr->anchorGC, ancX, ancY, ancW, ancH);
    }
}

/*
 *----------------------------------------------------------------------
 * SelectionAdd --
 *--------------------------------------------------------------
 */
static void SelectionAdd(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    if (chPtr->selected) {		/* sanity check */
	return;
    }

    chPtr->selected = 1;
    SelectionNotifyAncestors(wPtr, chPtr->parent);
}

/*
 *----------------------------------------------------------------------
 * HL_SelectionClear --
 *--------------------------------------------------------------
 */
static void HL_SelectionClear(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    if (! chPtr->selected) {		/* sanity check */
	return;
    }

    chPtr->selected = 0;
    HL_SelectionClearNotifyAncestors(wPtr, chPtr->parent);
}

/*
 *----------------------------------------------------------------------
 * HL_SelectionClearAll --
 *--------------------------------------------------------------
 */
static void HL_SelectionClearAll(wPtr, chPtr, changed_ret)
    WidgetPtr wPtr;
    HListElement * chPtr;
    int * changed_ret;
{
    HListElement * ptr;

    if (chPtr->selected) {
	*changed_ret = 1;
	chPtr->selected = 0;
    }

    if (chPtr->numSelectedChild == 0) {
	return;
    } else {
	chPtr->numSelectedChild = 0;

	for (ptr=chPtr->childHead; ptr; ptr=ptr->next) {
	    HL_SelectionClearAll(wPtr, ptr, changed_ret);
	}
    }
}

/*
 *----------------------------------------------------------------------
 * SelectionNotifyAncestors --
 *
 *	!!This has nothing to do with SelectionNotify in X!!
 *
 *	HList keeps a counter in every entry on how many of its
 *	child entries has been selected. This will make the
 *	"selection clear" very efficient. To keep this counter
 *	up-to-date, we must call SelectionNotifyAncestors() or
 *	HL_SelectionClearNotifyAncestors every time the selection
 *	has changed.
 *--------------------------------------------------------------
 */
static void SelectionNotifyAncestors(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    chPtr->numSelectedChild ++;

    if (chPtr->selected || (chPtr->numSelectedChild > 1)) {
	/* My ancestors already know that I have selections */
	return;
    } else {
	if (chPtr != wPtr->root) {
	    SelectionNotifyAncestors(wPtr, chPtr->parent);
	}
    }
}

static void HL_SelectionClearNotifyAncestors(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    chPtr->numSelectedChild --;

    if (chPtr->selected || (chPtr->numSelectedChild > 0)) {
	/* I still have selections, don't need to notify parent */
	return;
    } else {
	if (chPtr != wPtr->root) {
	    SelectionNotifyAncestors(wPtr, chPtr->parent);
	}
    }
}
/*
 *--------------------------------------------------------------
 * DeleteOffsprings --
 *--------------------------------------------------------------
 */
static void DeleteOffsprings(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    HListElement * ptr;
    HListElement * toFree;

    ptr=chPtr->childHead;
    while (ptr) {
	DeleteOffsprings(wPtr, ptr);
	toFree = ptr;
	ptr=ptr->next;
	FreeElement(wPtr, toFree);
    }

    chPtr->childHead = 0;
    chPtr->childTail = 0;
}

/*
 *--------------------------------------------------------------
 * DeleteSiblings --
 *--------------------------------------------------------------
 */
static void DeleteSiblings(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    HListElement * ptr;

    for (ptr=chPtr->parent->childHead; ptr; ptr=ptr->next) {
	if (ptr != chPtr) {
	    DeleteNode(wPtr, ptr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 * DeleteNode --
 *--------------------------------------------------------------
 */
static void DeleteNode(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    if (chPtr->parent == NULL) {
	/*
	 * This is root node : can't delete
	 */
	return;
    }

    DeleteOffsprings(wPtr, chPtr);

    /*
     * Check for deleting parent's first child
     */
    if (chPtr == chPtr->parent->childHead) {
	chPtr->parent->childHead = chPtr->next;
    }
    else {
	chPtr->prev->next = chPtr->next;
    }

    /*
     * Check for 'last' child (could be both first AND last)
     */
    if (chPtr == chPtr->parent->childTail) {
	chPtr->parent->childTail = chPtr->prev;
    }
    else {
	chPtr->next->prev = chPtr->prev;
    }
 
    FreeElement(wPtr, chPtr);
}

/*
 *----------------------------------------------------------------------
 * UpdateOneScrollBar --
 *--------------------------------------------------------------
 */
static void UpdateOneScrollBar(wPtr, command, total, window, first)
    WidgetPtr wPtr;
    CONST84 char * command;
    int total;
    int window;
    int first;
{
    char string[100];
    double d_first, d_last;

    GetScrollFractions(total, window, first, &d_first, &d_last);

    sprintf(string, " %g %g", d_first, d_last);
    if (Tix_GlobalVarEval(wPtr->dispData.interp, command, string,
	    (char *) NULL) != TCL_OK) {
	Tcl_AddErrorInfo(wPtr->dispData.interp,
		"\n    (scrolling command executed by tixHList)");
	Tk_BackgroundError(wPtr->dispData.interp);
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
    int total, window, first;

    CheckScrollBar(wPtr, TIX_X);
    CheckScrollBar(wPtr, TIX_Y);
 
    if (wPtr->xScrollCmd) {
	total  = wPtr->totalSize[0];
	window = Tk_Width(wPtr->dispData.tkwin)
	  - 2*wPtr->borderWidth - 2*wPtr->highlightWidth;
	first  = wPtr->leftPixel;
	UpdateOneScrollBar(wPtr, wPtr->xScrollCmd, total, window, first);
    }

    if (wPtr->yScrollCmd) {
	total  = wPtr->totalSize[1];
	window = Tk_Height(wPtr->dispData.tkwin)
	  - 2*wPtr->borderWidth - 2*wPtr->highlightWidth;
	first  = wPtr->topPixel;

	if (wPtr->useHeader) {
	    window -= wPtr->headerHeight;
	}
	UpdateOneScrollBar(wPtr, wPtr->yScrollCmd, total, window, first);
    }

    if (wPtr->sizeCmd && sizeChanged) {
	if (Tcl_GlobalEval(wPtr->dispData.interp, wPtr->sizeCmd) != TCL_OK) {
	    Tcl_AddErrorInfo(wPtr->dispData.interp,
		"\n    (size command executed by tixHList)");
	    Tk_BackgroundError(wPtr->dispData.interp);
	}
    }
}

/*----------------------------------------------------------------------
 * XScrollByUnits
 *----------------------------------------------------------------------
 */
static int XScrollByUnits(wPtr, count)
    WidgetPtr wPtr;
    int count;
{
    return wPtr->leftPixel + count*wPtr->scrollUnit[0];
}

/*----------------------------------------------------------------------
 * XScrollByPages
 *----------------------------------------------------------------------
 */
static int XScrollByPages(wPtr, count)
    WidgetPtr wPtr;
    int count;
{
    return wPtr->leftPixel + count*Tk_Width(wPtr->dispData.tkwin);
}

/*----------------------------------------------------------------------
 * YScrollByUnits
 *----------------------------------------------------------------------
 */
static int YScrollByUnits(wPtr, count)
    WidgetPtr wPtr;
    int count;
{
    HListElement * chPtr;
    int height;

    if ((chPtr = FindElementAtPosition(wPtr, 0))) {
	height = chPtr->height;
    } else if (wPtr->root->childHead) {
	height = wPtr->root->childHead->height;
    } else {
	height = 0;
    }

    return wPtr->topPixel + count*height;
}

/*----------------------------------------------------------------------
 * YScrollByPages
 *----------------------------------------------------------------------
 */
static int YScrollByPages(wPtr, count)
    WidgetPtr wPtr;
    int count;
{
    int window = Tk_Height(wPtr->dispData.tkwin)
      - 2*wPtr->borderWidth - 2*wPtr->highlightWidth;

    if (wPtr->useHeader) {
	window -= wPtr->headerHeight;
    }

    return wPtr->topPixel + count*window;
}

/*----------------------------------------------------------------------
 * CheckScrollBar
 *
 *	Make sures that the seeting of the scrollbars are correct: i.e.
 *	the bottom element will never be scrolled up by too much.
 *----------------------------------------------------------------------
 */
static void CheckScrollBar(wPtr, which)
    WidgetPtr wPtr;
    int which;
{
    int window;
    int total;
    int first;

    if (which == TIX_Y) {
	window = Tk_Height(wPtr->dispData.tkwin)
	  - 2*wPtr->borderWidth - 2*wPtr->highlightWidth;
	if (wPtr->useHeader) {
	    window -= wPtr->headerHeight;
	}
	total  = wPtr->totalSize[1];
	first  = wPtr->topPixel;
    } else {
	window = Tk_Width(wPtr->dispData.tkwin)
	  - 2*wPtr->borderWidth - 2*wPtr->highlightWidth;
	total  = wPtr->totalSize[0];
	first  = wPtr->leftPixel;
    }

    /* Check whether the topPixel is out of bound */
    if (first < 0) {
	first = 0;
    } else {
	if (window > total) {
	    first = 0;
	} else if ((first + window) > total) {
	    first = total - window;
	}
    }

    if (which == TIX_Y) {
	wPtr->topPixel = first;
    } else {
	wPtr->leftPixel = first;
    }
}

/*----------------------------------------------------------------------
 * GetScrollFractions --
 *
 * Compute the fractions of a scroll-able widget.
 *
 */
static void GetScrollFractions(total, window, first, first_ret, last_ret)
    int total;
    int window;
    int first;
    double * first_ret;
    double * last_ret;
{
    if (total == 0 || total < window) {
	*first_ret = 0.0;
	*last_ret  = 1.0;
    } else {
	*first_ret = (double)(first) / (double)(total);
	*last_ret  = (double)(first+window) / (double)(total);
    }
}

/*----------------------------------------------------------------------
 * Find the element that's immediately below this element.
 *
 *----------------------------------------------------------------------
 */
static HListElement *
FindNextEntry(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    if (chPtr->childHead != NULL) {
	return chPtr->childHead;
    }
    if (chPtr->next) {
	return chPtr->next;
    }
	
    /* go to a different branch */
    while (1) {
	if (chPtr == wPtr->root) {
	    return (HListElement *)NULL;
	}
	chPtr = chPtr->parent;
	if (chPtr->next) {
	    return chPtr->next;
	}
    }
}

/*----------------------------------------------------------------------
 * Find the element that's immediately above this element.
 *
 *----------------------------------------------------------------------
 */
static HListElement *
FindPrevEntry(wPtr, chPtr)
    WidgetPtr wPtr;
    HListElement * chPtr;
{
    if (chPtr->prev) {
	/* Find the bottom of this sub-tree
	 */
	for (chPtr=chPtr->prev; chPtr->childTail; chPtr = chPtr->childTail)
	  ;

	return chPtr;
    } else {
	if (chPtr->parent == wPtr->root) {
	    return 0;
	} else {
	    return chPtr->parent;
	}
    }
}

