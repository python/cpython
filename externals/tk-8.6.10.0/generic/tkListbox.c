/*
 * tkListbox.c --
 *
 *	This module implements listbox widgets for the Tk toolkit. A listbox
 *	displays a collection of strings, one per line, and provides scrolling
 *	and selection.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"

#ifdef _WIN32
#include "tkWinInt.h"
#endif

typedef struct {
    Tk_OptionTable listboxOptionTable;
				/* Table defining configuration options
				 * available for the listbox. */
    Tk_OptionTable itemAttrOptionTable;
				/* Table defining configuration options
				 * available for listbox items. */
} ListboxOptionTables;

/*
 * A data structure of the following type is kept for each listbox widget
 * managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the listbox. NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up. */
    Display *display;		/* Display containing widget. Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with listbox. */
    Tcl_Command widgetCmd;	/* Token for listbox's widget command. */
    Tk_OptionTable optionTable;	/* Table that defines configuration options
				 * available for this widget. */
    Tk_OptionTable itemAttrOptionTable;
				/* Table that defines configuration options
				 * available for listbox items. */
    char *listVarName;		/* List variable name */
    Tcl_Obj *listObj;		/* Pointer to the list object being used */
    int nElements;		/* Holds the current count of elements */
    Tcl_HashTable *selection;	/* Tracks selection */
    Tcl_HashTable *itemAttrTable;
				/* Tracks item attributes */

    /*
     * Information used when displaying widget:
     */

    Tk_3DBorder normalBorder;	/* Used for drawing border around whole
				 * window, plus used for background. */
    int borderWidth;		/* Width of 3-D border around window. */
    int relief;			/* 3-D effect: TK_RELIEF_RAISED, etc. */
    int highlightWidth;		/* Width in pixels of highlight to draw around
				 * widget when it has the focus. <= 0 means
				 * don't draw a highlight. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight area
				 * when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int inset;			/* Total width of all borders, including
				 * traversal highlight and 3-D border.
				 * Indicates how much interior stuff must be
				 * offset from outside edges to leave room for
				 * borders. */
    Tk_Font tkfont;		/* Information about text font, or NULL. */
    XColor *fgColorPtr;		/* Text color in normal mode. */
    XColor *dfgColorPtr;	/* Text color in disabled mode. */
    GC textGC;			/* For drawing normal text. */
    Tk_3DBorder selBorder;	/* Borders and backgrounds for selected
				 * elements. */
    int selBorderWidth;		/* Width of border around selection. */
    XColor *selFgColorPtr;	/* Foreground color for selected elements. */
    GC selTextGC;		/* For drawing selected text. */
    int width;			/* Desired width of window, in characters. */
    int height;			/* Desired height of window, in lines. */
    int lineHeight;		/* Number of pixels allocated for each line in
				 * display. */
    int topIndex;		/* Index of top-most element visible in
				 * window. */
    int fullLines;		/* Number of lines that are completely
				 * visible in window. There may be one
				 * additional line at the bottom that is
				 * partially visible. */
    int partialLine;		/* 0 means that the window holds exactly
				 * fullLines lines. 1 means that there is one
				 * additional line that is partially
				 * visible. */
    int setGrid;		/* Non-zero means pass gridding information to
				 * window manager. */

    /*
     * Information to support horizontal scrolling:
     */

    int maxWidth;		/* Width (in pixels) of widest string in
				 * listbox. */
    int xScrollUnit;		/* Number of pixels in one "unit" for
				 * horizontal scrolling (window scrolls
				 * horizontally in increments of this size).
				 * This is an average character size. */
    int xOffset;		/* The left edge of each string in the listbox
				 * is offset to the left by this many pixels
				 * (0 means no offset, positive means there is
				 * an offset). This is x scrolling information
                                 * is not linked to justification. */

    /*
     * Information about what's selected or active, if any.
     */

    Tk_Uid selectMode;		/* Selection style: single, browse, multiple,
				 * or extended. This value isn't used in C
				 * code, but the Tcl bindings use it. */
    int numSelected;		/* Number of elements currently selected. */
    int selectAnchor;		/* Fixed end of selection (i.e. element at
				 * which selection was started.) */
    int exportSelection;	/* Non-zero means tie internal listbox to X
				 * selection. */
    int active;			/* Index of "active" element (the one that has
				 * been selected by keyboard traversal). -1
				 * means none. */
    int activeStyle;		/* Style in which to draw the active element.
				 * One of: underline, none, dotbox */

    /*
     * Information for scanning:
     */

    int scanMarkX;		/* X-position at which scan started (e.g.
				 * button was pressed here). */
    int scanMarkY;		/* Y-position at which scan started (e.g.
				 * button was pressed here). */
    int scanMarkXOffset;	/* Value of "xOffset" field when scan
				 * started. */
    int scanMarkYIndex;		/* Index of line that was at top of window
				 * when scan started. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option; not used in the
				 * C code, but used by keyboard traversal
				 * scripts. Malloc'ed, but may be NULL. */
    char *yScrollCmd;		/* Command prefix for communicating with
				 * vertical scrollbar. NULL means no command
				 * to issue. Malloc'ed. */
    char *xScrollCmd;		/* Command prefix for communicating with
				 * horizontal scrollbar. NULL means no command
				 * to issue. Malloc'ed. */
    int state;			/* Listbox state. */
    Pixmap gray;		/* Pixmap for displaying disabled text. */
    int flags;			/* Various flag bits: see below for
				 * definitions. */
    Tk_Justify justify;         /* Justification. */
} Listbox;

/*
 * How to encode the keys for the hash tables used to store what items are
 * selected and what the attributes are.
 */

#define KEY(i)		((char *) INT2PTR(i))

/*
 * ItemAttr structures are used to store item configuration information for
 * the items in a listbox
 */

typedef struct {
    Tk_3DBorder border;		/* Used for drawing background around text */
    Tk_3DBorder selBorder;	/* Used for selected text */
    XColor *fgColor;		/* Text color in normal mode. */
    XColor *selFgColor;		/* Text color in selected mode. */
} ItemAttr;

/*
 * Flag bits for listboxes:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler has
 *				already been queued to redraw this window.
 * UPDATE_V_SCROLLBAR:		Non-zero means vertical scrollbar needs to be
 *				updated.
 * UPDATE_H_SCROLLBAR:		Non-zero means horizontal scrollbar needs to
 *				be updated.
 * GOT_FOCUS:			Non-zero means this widget currently has the
 *				input focus.
 * MAXWIDTH_IS_STALE:		Stored maxWidth may be out-of-date.
 * LISTBOX_DELETED:		This listbox has been effectively destroyed.
 */

#define REDRAW_PENDING		1
#define UPDATE_V_SCROLLBAR	2
#define UPDATE_H_SCROLLBAR	4
#define GOT_FOCUS		8
#define MAXWIDTH_IS_STALE	16
#define LISTBOX_DELETED		32

/*
 * The following enum is used to define a type for the -state option of the
 * Listbox widget. These values are used as indices into the string table
 * below.
 */

enum state {
    STATE_DISABLED, STATE_NORMAL
};

static const char *const stateStrings[] = {
    "disabled", "normal", NULL
};

enum activeStyle {
    ACTIVE_STYLE_DOTBOX, ACTIVE_STYLE_NONE, ACTIVE_STYLE_UNDERLINE
};

static const char *const activeStyleStrings[] = {
    "dotbox", "none", "underline", NULL
};

/*
 * The optionSpecs table defines the valid configuration options for the
 * listbox widget.
 */

static const Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_STRING_TABLE, "-activestyle", "activeStyle", "ActiveStyle",
	DEF_LISTBOX_ACTIVE_STYLE, -1, Tk_Offset(Listbox, activeStyle),
	0, activeStyleStrings, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	 DEF_LISTBOX_BG_COLOR, -1, Tk_Offset(Listbox, normalBorder),
	 0, DEF_LISTBOX_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	 NULL, 0, -1, 0, "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
	 NULL, 0, -1, 0, "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	 DEF_LISTBOX_BORDER_WIDTH, -1, Tk_Offset(Listbox, borderWidth),
	 0, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	 DEF_LISTBOX_CURSOR, -1, Tk_Offset(Listbox, cursor),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-disabledforeground", "disabledForeground",
	 "DisabledForeground", DEF_LISTBOX_DISABLED_FG, -1,
	 Tk_Offset(Listbox, dfgColorPtr), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-exportselection", "exportSelection",
	 "ExportSelection", DEF_LISTBOX_EXPORT_SELECTION, -1,
	 Tk_Offset(Listbox, exportSelection), 0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", NULL,
	 NULL, 0, -1, 0, "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	 DEF_LISTBOX_FONT, -1, Tk_Offset(Listbox, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	 DEF_LISTBOX_FG, -1, Tk_Offset(Listbox, fgColorPtr), 0, 0, 0},
    {TK_OPTION_INT, "-height", "height", "Height",
	 DEF_LISTBOX_HEIGHT, -1, Tk_Offset(Listbox, height), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	 "HighlightBackground", DEF_LISTBOX_HIGHLIGHT_BG, -1,
	 Tk_Offset(Listbox, highlightBgColorPtr), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	 DEF_LISTBOX_HIGHLIGHT, -1, Tk_Offset(Listbox, highlightColorPtr),
	 0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	 "HighlightThickness", DEF_LISTBOX_HIGHLIGHT_WIDTH, -1,
	 Tk_Offset(Listbox, highlightWidth), 0, 0, 0},
    {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
	DEF_LISTBOX_JUSTIFY, -1, Tk_Offset(Listbox, justify), 0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	 DEF_LISTBOX_RELIEF, -1, Tk_Offset(Listbox, relief), 0, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", "selectBackground", "Foreground",
	 DEF_LISTBOX_SELECT_COLOR, -1, Tk_Offset(Listbox, selBorder),
	 0, DEF_LISTBOX_SELECT_MONO, 0},
    {TK_OPTION_PIXELS, "-selectborderwidth", "selectBorderWidth",
	 "BorderWidth", DEF_LISTBOX_SELECT_BD, -1,
	 Tk_Offset(Listbox, selBorderWidth), 0, 0, 0},
    {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "Background",
	 DEF_LISTBOX_SELECT_FG_COLOR, -1, Tk_Offset(Listbox, selFgColorPtr),
	 TK_OPTION_NULL_OK, DEF_LISTBOX_SELECT_FG_MONO, 0},
    {TK_OPTION_STRING, "-selectmode", "selectMode", "SelectMode",
	 DEF_LISTBOX_SELECT_MODE, -1, Tk_Offset(Listbox, selectMode),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-setgrid", "setGrid", "SetGrid",
	 DEF_LISTBOX_SET_GRID, -1, Tk_Offset(Listbox, setGrid), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
	DEF_LISTBOX_STATE, -1, Tk_Offset(Listbox, state),
	0, stateStrings, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	 DEF_LISTBOX_TAKE_FOCUS, -1, Tk_Offset(Listbox, takeFocus),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_INT, "-width", "width", "Width",
	 DEF_LISTBOX_WIDTH, -1, Tk_Offset(Listbox, width), 0, 0, 0},
    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	 DEF_LISTBOX_SCROLL_COMMAND, -1, Tk_Offset(Listbox, xScrollCmd),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	 DEF_LISTBOX_SCROLL_COMMAND, -1, Tk_Offset(Listbox, yScrollCmd),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-listvariable", "listVariable", "Variable",
	 DEF_LISTBOX_LIST_VARIABLE, -1, Tk_Offset(Listbox, listVarName),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, -1, 0, 0, 0}
};

/*
 * The itemAttrOptionSpecs table defines the valid configuration options for
 * listbox items.
 */

static const Tk_OptionSpec itemAttrOptionSpecs[] = {
    {TK_OPTION_BORDER, "-background", "background", "Background",
     NULL, -1, Tk_Offset(ItemAttr, border),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT,
     DEF_LISTBOX_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
     NULL, 0, -1, 0, "-background", 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", NULL,
     NULL, 0, -1, 0, "-foreground", 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
     NULL, -1, Tk_Offset(ItemAttr, fgColor),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", "selectBackground", "Foreground",
     NULL, -1, Tk_Offset(ItemAttr, selBorder),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT,
     DEF_LISTBOX_SELECT_MONO, 0},
    {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "Background",
     NULL, -1, Tk_Offset(ItemAttr, selFgColor),
     TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT,
     DEF_LISTBOX_SELECT_FG_MONO, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, -1, 0, 0, 0}
};

/*
 * The following tables define the listbox widget commands (and sub-commands)
 * and map the indexes into the string tables into enumerated types used to
 * dispatch the listbox widget command.
 */

static const char *const commandNames[] = {
    "activate", "bbox", "cget", "configure", "curselection", "delete", "get",
    "index", "insert", "itemcget", "itemconfigure", "nearest", "scan",
    "see", "selection", "size", "xview", "yview", NULL
};
enum command {
    COMMAND_ACTIVATE, COMMAND_BBOX, COMMAND_CGET, COMMAND_CONFIGURE,
    COMMAND_CURSELECTION, COMMAND_DELETE, COMMAND_GET, COMMAND_INDEX,
    COMMAND_INSERT, COMMAND_ITEMCGET, COMMAND_ITEMCONFIGURE,
    COMMAND_NEAREST, COMMAND_SCAN, COMMAND_SEE, COMMAND_SELECTION,
    COMMAND_SIZE, COMMAND_XVIEW, COMMAND_YVIEW
};

static const char *const selCommandNames[] = {
    "anchor", "clear", "includes", "set", NULL
};
enum selcommand {
    SELECTION_ANCHOR, SELECTION_CLEAR, SELECTION_INCLUDES, SELECTION_SET
};

static const char *const scanCommandNames[] = {
    "mark", "dragto", NULL
};
enum scancommand {
    SCAN_MARK, SCAN_DRAGTO
};

static const char *const indexNames[] = {
    "active", "anchor", "end", NULL
};
enum indices {
    INDEX_ACTIVE, INDEX_ANCHOR, INDEX_END
};

/*
 * Declarations for procedures defined later in this file.
 */

static void		ChangeListboxOffset(Listbox *listPtr, int offset);
static void		ChangeListboxView(Listbox *listPtr, int index);
static int		ConfigureListbox(Tcl_Interp *interp, Listbox *listPtr,
			    int objc, Tcl_Obj *const objv[]);
static int		ConfigureListboxItem(Tcl_Interp *interp,
			    Listbox *listPtr, ItemAttr *attrs, int objc,
			    Tcl_Obj *const objv[], int index);
static int		ListboxDeleteSubCmd(Listbox *listPtr,
			    int first, int last);
static void		DestroyListbox(void *memPtr);
static void		DestroyListboxOptionTables(ClientData clientData,
			    Tcl_Interp *interp);
static void		DisplayListbox(ClientData clientData);
static int		GetListboxIndex(Tcl_Interp *interp, Listbox *listPtr,
			    Tcl_Obj *index, int endIsSize, int *indexPtr);
static int		ListboxInsertSubCmd(Listbox *listPtr,
			    int index, int objc, Tcl_Obj *const objv[]);
static void		ListboxCmdDeletedProc(ClientData clientData);
static void		ListboxComputeGeometry(Listbox *listPtr,
			    int fontChanged, int maxIsStale, int updateGrid);
static void		ListboxEventProc(ClientData clientData,
			    XEvent *eventPtr);
static int		ListboxFetchSelection(ClientData clientData,
			    int offset, char *buffer, int maxBytes);
static void		ListboxLostSelection(ClientData clientData);
static void		GenerateListboxSelectEvent(Listbox *listPtr);
static void		EventuallyRedrawRange(Listbox *listPtr,
			    int first, int last);
static void		ListboxScanTo(Listbox *listPtr, int x, int y);
static int		ListboxSelect(Listbox *listPtr,
			    int first, int last, int select);
static void		ListboxUpdateHScrollbar(Listbox *listPtr);
static void		ListboxUpdateVScrollbar(Listbox *listPtr);
static int		ListboxWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		ListboxBboxSubCmd(Tcl_Interp *interp,
			    Listbox *listPtr, int index);
static int		ListboxSelectionSubCmd(Tcl_Interp *interp,
			    Listbox *listPtr, int objc, Tcl_Obj *const objv[]);
static int		ListboxXviewSubCmd(Tcl_Interp *interp,
			    Listbox *listPtr, int objc, Tcl_Obj *const objv[]);
static int		ListboxYviewSubCmd(Tcl_Interp *interp,
			    Listbox *listPtr, int objc, Tcl_Obj *const objv[]);
static ItemAttr *	ListboxGetItemAttributes(Tcl_Interp *interp,
			    Listbox *listPtr, int index);
static void		ListboxWorldChanged(ClientData instanceData);
static int		NearestListboxElement(Listbox *listPtr, int y);
static char *		ListboxListVarProc(ClientData clientData,
			    Tcl_Interp *interp, const char *name1,
			    const char *name2, int flags);
static void		MigrateHashEntries(Tcl_HashTable *table,
			    int first, int last, int offset);
static int		GetMaxOffset(Listbox *listPtr);

/*
 * The structure below defines button class behavior by means of procedures
 * that can be invoked from generic window code.
 */

static const Tk_ClassProcs listboxClass = {
    sizeof(Tk_ClassProcs),	/* size */
    ListboxWorldChanged,	/* worldChangedProc */
    NULL,			/* createProc */
    NULL			/* modalProc */
};

/*
 *--------------------------------------------------------------
 *
 * Tk_ListboxObjCmd --
 *
 *	This procedure is invoked to process the "listbox" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_ListboxObjCmd(
    ClientData clientData,	/* NULL. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register Listbox *listPtr;
    Tk_Window tkwin;
    ListboxOptionTables *optionTables;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?-option value ...?");
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp),
	    Tcl_GetString(objv[1]), NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    optionTables = Tcl_GetAssocData(interp, "ListboxOptionTables", NULL);
    if (optionTables == NULL) {
	/*
	 * We haven't created the option tables for this widget class yet. Do
	 * it now and save the a pointer to them as the ClientData for the
	 * command, so future invocations will have access to it.
	 */

	optionTables = ckalloc(sizeof(ListboxOptionTables));

	/*
	 * Set up an exit handler to free the optionTables struct.
	 */

	Tcl_SetAssocData(interp, "ListboxOptionTables",
		DestroyListboxOptionTables, optionTables);

	/*
	 * Create the listbox option table and the listbox item option table.
	 */

	optionTables->listboxOptionTable =
		Tk_CreateOptionTable(interp, optionSpecs);
	optionTables->itemAttrOptionTable =
		Tk_CreateOptionTable(interp, itemAttrOptionSpecs);
    }

    /*
     * Initialize the fields of the structure that won't be initialized by
     * ConfigureListbox, or that ConfigureListbox requires to be initialized
     * already (e.g. resource pointers).
     */

    listPtr			 = ckalloc(sizeof(Listbox));
    memset(listPtr, 0, sizeof(Listbox));

    listPtr->tkwin		 = tkwin;
    listPtr->display		 = Tk_Display(tkwin);
    listPtr->interp		 = interp;
    listPtr->widgetCmd		 = Tcl_CreateObjCommand(interp,
	    Tk_PathName(listPtr->tkwin), ListboxWidgetObjCmd, listPtr,
	    ListboxCmdDeletedProc);
    listPtr->optionTable	 = optionTables->listboxOptionTable;
    listPtr->itemAttrOptionTable = optionTables->itemAttrOptionTable;
    listPtr->selection		 = ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(listPtr->selection, TCL_ONE_WORD_KEYS);
    listPtr->itemAttrTable	 = ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(listPtr->itemAttrTable, TCL_ONE_WORD_KEYS);
    listPtr->relief		 = TK_RELIEF_RAISED;
    listPtr->textGC		 = NULL;
    listPtr->selFgColorPtr	 = NULL;
    listPtr->selTextGC		 = NULL;
    listPtr->fullLines		 = 1;
    listPtr->xScrollUnit	 = 1;
    listPtr->exportSelection	 = 1;
    listPtr->cursor		 = NULL;
    listPtr->state		 = STATE_NORMAL;
    listPtr->gray		 = None;
    listPtr->justify             = TK_JUSTIFY_LEFT;

    /*
     * Keep a hold of the associated tkwin until we destroy the listbox,
     * otherwise Tk might free it while we still need it.
     */

    Tcl_Preserve(listPtr->tkwin);

    Tk_SetClass(listPtr->tkwin, "Listbox");
    Tk_SetClassProcs(listPtr->tkwin, &listboxClass, listPtr);
    Tk_CreateEventHandler(listPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    ListboxEventProc, listPtr);
    Tk_CreateSelHandler(listPtr->tkwin, XA_PRIMARY, XA_STRING,
	    ListboxFetchSelection, listPtr, XA_STRING);
    if (Tk_InitOptions(interp, (char *)listPtr,
	    optionTables->listboxOptionTable, tkwin) != TCL_OK) {
	Tk_DestroyWindow(listPtr->tkwin);
	return TCL_ERROR;
    }

    if (ConfigureListbox(interp, listPtr, objc-2, objv+2) != TCL_OK) {
	Tk_DestroyWindow(listPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, TkNewWindowObj(listPtr->tkwin));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxWidgetObjCmd --
 *
 *	This Tcl_Obj based procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module. See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxWidgetObjCmd(
    ClientData clientData,	/* Information about listbox widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Arguments as Tcl_Obj's. */
{
    register Listbox *listPtr = clientData;
    int cmdIndex, index;
    int result = TCL_OK;
    Tcl_Obj *objPtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    /*
     * Parse the command by looking up the second argument in the list of
     * valid subcommand names.
     */

    result = Tcl_GetIndexFromObj(interp, objv[1], commandNames,
	    "option", 0, &cmdIndex);
    if (result != TCL_OK) {
	return result;
    }

    Tcl_Preserve(listPtr);

    /*
     * The subcommand was valid, so continue processing.
     */

    switch (cmdIndex) {
    case COMMAND_ACTIVATE:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index");
	    result = TCL_ERROR;
	    break;
	}
	result = GetListboxIndex(interp, listPtr, objv[2], 0, &index);
	if (result != TCL_OK) {
	    break;
	}

	if (!(listPtr->state & STATE_NORMAL)) {
	    break;
	}

	if (index >= listPtr->nElements) {
	    index = listPtr->nElements-1;
	}
	if (index < 0) {
	    index = 0;
	}
	listPtr->active = index;
	EventuallyRedrawRange(listPtr, listPtr->active, listPtr->active);
	result = TCL_OK;
	break;

    case COMMAND_BBOX:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index");
	    result = TCL_ERROR;
	    break;
	}
	result = GetListboxIndex(interp, listPtr, objv[2], 0, &index);
	if (result != TCL_OK) {
	    break;
	}

	result = ListboxBboxSubCmd(interp, listPtr, index);
	break;

    case COMMAND_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	    break;
	}

	objPtr = Tk_GetOptionValue(interp, (char *) listPtr,
		listPtr->optionTable, objv[2], listPtr->tkwin);
	if (objPtr == NULL) {
	    result = TCL_ERROR;
	    break;
	}
	Tcl_SetObjResult(interp, objPtr);
	result = TCL_OK;
	break;

    case COMMAND_CONFIGURE:
	if (objc <= 3) {
	    objPtr = Tk_GetOptionInfo(interp, (char *) listPtr,
		    listPtr->optionTable,
		    (objc == 3) ? objv[2] : NULL, listPtr->tkwin);
	    if (objPtr == NULL) {
		result = TCL_ERROR;
		break;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    result = TCL_OK;
	} else {
	    result = ConfigureListbox(interp, listPtr, objc-2, objv+2);
	}
	break;

    case COMMAND_CURSELECTION: {
	int i;

	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    result = TCL_ERROR;
	    break;
	}

	/*
	 * Of course, it would be more efficient to use the Tcl_HashTable
	 * search functions (Tcl_FirstHashEntry, Tcl_NextHashEntry), but then
	 * the result wouldn't be in sorted order. So instead we loop through
	 * the indices in order, adding them to the result if they are
	 * selected.
	 */

	objPtr = Tcl_NewObj();
	for (i = 0; i < listPtr->nElements; i++) {
	    if (Tcl_FindHashEntry(listPtr->selection, KEY(i))) {
		Tcl_ListObjAppendElement(NULL, objPtr, Tcl_NewIntObj(i));
	    }
	}
	Tcl_SetObjResult(interp, objPtr);
	result = TCL_OK;
	break;
    }

    case COMMAND_DELETE: {
	int first, last;

	if ((objc < 3) || (objc > 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "firstIndex ?lastIndex?");
	    result = TCL_ERROR;
	    break;
	}

	result = GetListboxIndex(interp, listPtr, objv[2], 0, &first);
	if (result != TCL_OK) {
	    break;
	}

	if (!(listPtr->state & STATE_NORMAL)) {
	    break;
	}

	if (first < listPtr->nElements) {
	    /*
	     * if a "last index" was given, get it now; otherwise, use the
	     * first index as the last index.
	     */

	    if (objc == 4) {
		result = GetListboxIndex(interp, listPtr, objv[3], 0, &last);
		if (result != TCL_OK) {
		    break;
		}
	    } else {
		last = first;
	    }
	    if (last >= listPtr->nElements) {
		last = listPtr->nElements - 1;
	    }
	    result = ListboxDeleteSubCmd(listPtr, first, last);
	} else {
	    result = TCL_OK;
	}
	break;
    }

    case COMMAND_GET: {
	int first, last, listLen;
	Tcl_Obj **elemPtrs;

	if (objc != 3 && objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "firstIndex ?lastIndex?");
	    result = TCL_ERROR;
	    break;
	}
	result = GetListboxIndex(interp, listPtr, objv[2], 0, &first);
	if (result != TCL_OK) {
	    break;
	}
	last = first;
	if (objc == 4) {
	    result = GetListboxIndex(interp, listPtr, objv[3], 0, &last);
	    if (result != TCL_OK) {
		break;
	    }
	}
	if (first >= listPtr->nElements) {
	    result = TCL_OK;
	    break;
	}
	if (last >= listPtr->nElements) {
	    last = listPtr->nElements - 1;
	}
	if (first < 0) {
	    first = 0;
	}
	if (first > last) {
	    result = TCL_OK;
	    break;
	}
	result = Tcl_ListObjGetElements(interp, listPtr->listObj, &listLen,
		&elemPtrs);
	if (result != TCL_OK) {
	    break;
	}
	if (objc == 3) {
	    /*
	     * One element request - we return a string
	     */

	    Tcl_SetObjResult(interp, elemPtrs[first]);
	} else {
	    Tcl_SetObjResult(interp,
		    Tcl_NewListObj(last-first+1, elemPtrs+first));
	}
	result = TCL_OK;
	break;
    }

    case COMMAND_INDEX:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index");
	    result = TCL_ERROR;
	    break;
	}
	result = GetListboxIndex(interp, listPtr, objv[2], 1, &index);
	if (result != TCL_OK) {
	    break;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(index));
	result = TCL_OK;
	break;

    case COMMAND_INSERT:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index ?element ...?");
	    result = TCL_ERROR;
	    break;
	}

	result = GetListboxIndex(interp, listPtr, objv[2], 1, &index);
	if (result != TCL_OK) {
	    break;
	}

	if (!(listPtr->state & STATE_NORMAL)) {
	    break;
	}

	result = ListboxInsertSubCmd(listPtr, index, objc-3, objv+3);
	break;

    case COMMAND_ITEMCGET: {
	ItemAttr *attrPtr;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index option");
	    result = TCL_ERROR;
	    break;
	}

	result = GetListboxIndex(interp, listPtr, objv[2], 0, &index);
	if (result != TCL_OK) {
	    break;
	}

	if (index < 0 || index >= listPtr->nElements) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "item number \"%s\" out of range",
		    Tcl_GetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TK", "LISTBOX", "ITEM_INDEX", NULL);
	    result = TCL_ERROR;
	    break;
	}

	attrPtr = ListboxGetItemAttributes(interp, listPtr, index);

	objPtr = Tk_GetOptionValue(interp, (char *) attrPtr,
		listPtr->itemAttrOptionTable, objv[3], listPtr->tkwin);
	if (objPtr == NULL) {
	    result = TCL_ERROR;
	    break;
	}
	Tcl_SetObjResult(interp, objPtr);
	result = TCL_OK;
	break;
    }

    case COMMAND_ITEMCONFIGURE: {
	ItemAttr *attrPtr;

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "index ?-option? ?value? ?-option value ...?");
	    result = TCL_ERROR;
	    break;
	}

	result = GetListboxIndex(interp, listPtr, objv[2], 0, &index);
	if (result != TCL_OK) {
	    break;
	}

	if (index < 0 || index >= listPtr->nElements) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "item number \"%s\" out of range",
		    Tcl_GetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TK", "LISTBOX", "ITEM_INDEX", NULL);
	    result = TCL_ERROR;
	    break;
	}

	attrPtr = ListboxGetItemAttributes(interp, listPtr, index);
	if (objc <= 4) {
	    objPtr = Tk_GetOptionInfo(interp, (char *) attrPtr,
		    listPtr->itemAttrOptionTable,
		    (objc == 4) ? objv[3] : NULL, listPtr->tkwin);
	    if (objPtr == NULL) {
		result = TCL_ERROR;
		break;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    result = TCL_OK;
	} else {
	    result = ConfigureListboxItem(interp, listPtr, attrPtr,
		    objc-3, objv+3, index);
	}
	break;
    }

    case COMMAND_NEAREST: {
	int y;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "y");
	    result = TCL_ERROR;
	    break;
	}

	result = Tcl_GetIntFromObj(interp, objv[2], &y);
	if (result != TCL_OK) {
	    break;
	}
	index = NearestListboxElement(listPtr, y);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(index));
	result = TCL_OK;
	break;
    }

    case COMMAND_SCAN: {
	int x, y, scanCmdIndex;

	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "mark|dragto x y");
	    result = TCL_ERROR;
	    break;
	}

	if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK
		|| Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	    result = TCL_ERROR;
	    break;
	}

	result = Tcl_GetIndexFromObj(interp, objv[2], scanCommandNames,
		"option", 0, &scanCmdIndex);
	if (result != TCL_OK) {
	    break;
	}
	switch (scanCmdIndex) {
	case SCAN_MARK:
	    listPtr->scanMarkX = x;
	    listPtr->scanMarkY = y;
	    listPtr->scanMarkXOffset = listPtr->xOffset;
	    listPtr->scanMarkYIndex = listPtr->topIndex;
	    break;
	case SCAN_DRAGTO:
	    ListboxScanTo(listPtr, x, y);
	    break;
	}
	result = TCL_OK;
	break;
    }

    case COMMAND_SEE: {
	int diff;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index");
	    result = TCL_ERROR;
	    break;
	}
	result = GetListboxIndex(interp, listPtr, objv[2], 0, &index);
	if (result != TCL_OK) {
	    break;
	}
	if (index >= listPtr->nElements) {
	    index = listPtr->nElements - 1;
	}
	if (index < 0) {
	    index = 0;
	}
	diff = listPtr->topIndex - index;
	if (diff > 0) {
	    if (diff <= listPtr->fullLines / 3) {
		ChangeListboxView(listPtr, index);
	    } else {
		ChangeListboxView(listPtr, index - (listPtr->fullLines-1)/2);
	    }
	} else {
	    diff = index - (listPtr->topIndex + listPtr->fullLines - 1);
	    if (diff > 0) {
		if (diff <= listPtr->fullLines / 3) {
		    ChangeListboxView(listPtr, listPtr->topIndex + diff);
		} else {
		    ChangeListboxView(listPtr, index-(listPtr->fullLines-1)/2);
		}
	    }
	}
	result = TCL_OK;
	break;
    }

    case COMMAND_SELECTION:
	result = ListboxSelectionSubCmd(interp, listPtr, objc, objv);
	break;
    case COMMAND_SIZE:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    result = TCL_ERROR;
	    break;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(listPtr->nElements));
	result = TCL_OK;
	break;
    case COMMAND_XVIEW:
	result = ListboxXviewSubCmd(interp, listPtr, objc, objv);
	break;
    case COMMAND_YVIEW:
	result = ListboxYviewSubCmd(interp, listPtr, objc, objv);
	break;
    }
    Tcl_Release(listPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxBboxSubCmd --
 *
 *	This procedure is invoked to process a listbox bbox request. See the
 *	user documentation for more information.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	For valid indices, places the bbox of the requested element in the
 *	interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxBboxSubCmd(
    Tcl_Interp *interp,		/* Pointer to the calling Tcl interpreter */
    Listbox *listPtr,		/* Information about the listbox */
    int index)			/* Index of the element to get bbox info on */
{
    register Tk_Window tkwin = listPtr->tkwin;
    int lastVisibleIndex;

    /*
     * Determine the index of the last visible item in the listbox.
     */

    lastVisibleIndex = listPtr->topIndex + listPtr->fullLines
	    + listPtr->partialLine;
    if (listPtr->nElements < lastVisibleIndex) {
	lastVisibleIndex = listPtr->nElements;
    }

    /*
     * Only allow bbox requests for indices that are visible.
     */

    if ((listPtr->topIndex <= index) && (index < lastVisibleIndex)) {
	Tcl_Obj *el, *results[4];
	const char *stringRep;
	int pixelWidth, stringLen, x, y, result;
	Tk_FontMetrics fm;

	/*
	 * Compute the pixel width of the requested element.
	 */

	result = Tcl_ListObjIndex(interp, listPtr->listObj, index, &el);
	if (result != TCL_OK) {
	    return result;
	}

	stringRep = Tcl_GetStringFromObj(el, &stringLen);
	Tk_GetFontMetrics(listPtr->tkfont, &fm);
	pixelWidth = Tk_TextWidth(listPtr->tkfont, stringRep, stringLen);

        if (listPtr->justify == TK_JUSTIFY_LEFT) {
            x = (listPtr->inset + listPtr->selBorderWidth) - listPtr->xOffset;
        } else if (listPtr->justify == TK_JUSTIFY_RIGHT) {
            x = Tk_Width(tkwin) - (listPtr->inset + listPtr->selBorderWidth)
                    - pixelWidth - listPtr->xOffset + GetMaxOffset(listPtr);
        } else {
            x = (Tk_Width(tkwin) - pixelWidth)/2
                    - listPtr->xOffset + GetMaxOffset(listPtr)/2;
        }
	y = ((index - listPtr->topIndex)*listPtr->lineHeight)
		+ listPtr->inset + listPtr->selBorderWidth;
	results[0] = Tcl_NewIntObj(x);
	results[1] = Tcl_NewIntObj(y);
	results[2] = Tcl_NewIntObj(pixelWidth);
	results[3] = Tcl_NewIntObj(fm.linespace);
	Tcl_SetObjResult(interp, Tcl_NewListObj(4, results));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxSelectionSubCmd --
 *
 *	This procedure is invoked to process the selection sub command for
 *	listbox widgets.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May set the interpreter's result field.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxSelectionSubCmd(
    Tcl_Interp *interp,		/* Pointer to the calling Tcl interpreter */
    Listbox *listPtr,		/* Information about the listbox */
    int objc,			/* Number of arguments in the objv array */
    Tcl_Obj *const objv[])	/* Array of arguments to the procedure */
{
    int selCmdIndex, first, last;
    int result = TCL_OK;

    if (objc != 4 && objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "option index ?index?");
	return TCL_ERROR;
    }
    result = GetListboxIndex(interp, listPtr, objv[3], 0, &first);
    if (result != TCL_OK) {
	return result;
    }
    last = first;
    if (objc == 5) {
	result = GetListboxIndex(interp, listPtr, objv[4], 0, &last);
	if (result != TCL_OK) {
	    return result;
	}
    }
    result = Tcl_GetIndexFromObj(interp, objv[2], selCommandNames,
	    "option", 0, &selCmdIndex);
    if (result != TCL_OK) {
	return result;
    }

    /*
     * Only allow 'selection includes' to respond if disabled. [Bug #632514]
     */

    if ((listPtr->state == STATE_DISABLED)
	    && (selCmdIndex != SELECTION_INCLUDES)) {
	return TCL_OK;
    }

    switch (selCmdIndex) {
    case SELECTION_ANCHOR:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index");
	    return TCL_ERROR;
	}
	if (first >= listPtr->nElements) {
	    first = listPtr->nElements - 1;
	}
	if (first < 0) {
	    first = 0;
	}
	listPtr->selectAnchor = first;
	result = TCL_OK;
	break;
    case SELECTION_CLEAR:
	result = ListboxSelect(listPtr, first, last, 0);
	break;
    case SELECTION_INCLUDES:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index");
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
		Tcl_FindHashEntry(listPtr->selection, KEY(first)) != NULL));
	result = TCL_OK;
	break;
    case SELECTION_SET:
	result = ListboxSelect(listPtr, first, last, 1);
	break;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxXviewSubCmd --
 *
 *	Process the listbox "xview" subcommand.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May change the listbox viewing area; may set the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxXviewSubCmd(
    Tcl_Interp *interp,		/* Pointer to the calling Tcl interpreter */
    Listbox *listPtr,		/* Information about the listbox */
    int objc,			/* Number of arguments in the objv array */
    Tcl_Obj *const objv[])	/* Array of arguments to the procedure */
{
    int index, count, windowWidth, windowUnits;
    int offset = 0;		/* Initialized to stop gcc warnings. */
    double fraction;

    windowWidth = Tk_Width(listPtr->tkwin)
	    - 2*(listPtr->inset + listPtr->selBorderWidth);
    if (objc == 2) {
	Tcl_Obj *results[2];

	if (listPtr->maxWidth == 0) {
	    results[0] = Tcl_NewDoubleObj(0.0);
	    results[1] = Tcl_NewDoubleObj(1.0);
	} else {
	    double fraction2;

	    fraction = listPtr->xOffset / (double) listPtr->maxWidth;
	    fraction2 = (listPtr->xOffset + windowWidth)
		    / (double) listPtr->maxWidth;
	    if (fraction2 > 1.0) {
		fraction2 = 1.0;
	    }
	    results[0] = Tcl_NewDoubleObj(fraction);
	    results[1] = Tcl_NewDoubleObj(fraction2);
	}
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, results));
    } else if (objc == 3) {
	if (Tcl_GetIntFromObj(interp, objv[2], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	ChangeListboxOffset(listPtr, index*listPtr->xScrollUnit);
    } else {
	switch (Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count)) {
	case TK_SCROLL_ERROR:
	    return TCL_ERROR;
	case TK_SCROLL_MOVETO:
	    offset = (int) (fraction*listPtr->maxWidth + 0.5);
	    break;
	case TK_SCROLL_PAGES:
	    windowUnits = windowWidth / listPtr->xScrollUnit;
	    if (windowUnits > 2) {
		offset = listPtr->xOffset
			+ count*listPtr->xScrollUnit*(windowUnits-2);
	    } else {
		offset = listPtr->xOffset + count*listPtr->xScrollUnit;
	    }
	    break;
	case TK_SCROLL_UNITS:
	    offset = listPtr->xOffset + count*listPtr->xScrollUnit;
	    break;
	}
	ChangeListboxOffset(listPtr, offset);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxYviewSubCmd --
 *
 *	Process the listbox "yview" subcommand.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May change the listbox viewing area; may set the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxYviewSubCmd(
    Tcl_Interp *interp,		/* Pointer to the calling Tcl interpreter */
    Listbox *listPtr,		/* Information about the listbox */
    int objc,			/* Number of arguments in the objv array */
    Tcl_Obj *const objv[])	/* Array of arguments to the procedure */
{
    int index, count;
    double fraction;

    if (objc == 2) {
	Tcl_Obj *results[2];

	if (listPtr->nElements == 0) {
	    results[0] = Tcl_NewDoubleObj(0.0);
	    results[1] = Tcl_NewDoubleObj(1.0);
	} else {
	    double fraction2, numEls = (double) listPtr->nElements;

	    fraction = listPtr->topIndex / numEls;
	    fraction2 = (listPtr->topIndex+listPtr->fullLines) / numEls;
	    if (fraction2 > 1.0) {
		fraction2 = 1.0;
	    }
	    results[0] = Tcl_NewDoubleObj(fraction);
	    results[1] = Tcl_NewDoubleObj(fraction2);
	}
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, results));
    } else if (objc == 3) {
	if (GetListboxIndex(interp, listPtr, objv[2], 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	ChangeListboxView(listPtr, index);
    } else {
	switch (Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count)) {
	case TK_SCROLL_MOVETO:
	    index = (int) (listPtr->nElements*fraction + 0.5);
	    break;
	case TK_SCROLL_PAGES:
	    if (listPtr->fullLines > 2) {
		index = listPtr->topIndex + count*(listPtr->fullLines-2);
	    } else {
		index = listPtr->topIndex + count;
	    }
	    break;
	case TK_SCROLL_UNITS:
	    index = listPtr->topIndex + count;
	    break;
	case TK_SCROLL_ERROR:
	default:
	    return TCL_ERROR;
	}
	ChangeListboxView(listPtr, index);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxGetItemAttributes --
 *
 *	Returns a pointer to the ItemAttr record for a given index, creating
 *	one if it does not already exist.
 *
 * Results:
 *	Pointer to an ItemAttr record.
 *
 * Side effects:
 *	Memory may be allocated for the ItemAttr record.
 *
 *----------------------------------------------------------------------
 */

static ItemAttr *
ListboxGetItemAttributes(
    Tcl_Interp *interp,		/* Pointer to the calling Tcl interpreter */
    Listbox *listPtr,		/* Information about the listbox */
    int index)			/* Index of the item to retrieve attributes
				 * for. */
{
    int isNew;
    Tcl_HashEntry *entry;
    ItemAttr *attrs;

    entry = Tcl_CreateHashEntry(listPtr->itemAttrTable, KEY(index), &isNew);
    if (isNew) {
	attrs = ckalloc(sizeof(ItemAttr));
	attrs->border = NULL;
	attrs->selBorder = NULL;
	attrs->fgColor = NULL;
	attrs->selFgColor = NULL;
	Tk_InitOptions(interp, (char *)attrs, listPtr->itemAttrOptionTable,
		listPtr->tkwin);
	Tcl_SetHashValue(entry, attrs);
    } else {
	attrs = Tcl_GetHashValue(entry);
    }
    return attrs;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyListbox --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *	clean up the internal structure of a listbox at a safe time (when
 *	no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the listbox is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyListbox(
    void *memPtr)		/* Info about listbox widget. */
{
    register Listbox *listPtr = memPtr;
    Tcl_HashEntry *entry;
    Tcl_HashSearch search;

    /*
     * If we have an internal list object, free it.
     */

    if (listPtr->listObj != NULL) {
	Tcl_DecrRefCount(listPtr->listObj);
	listPtr->listObj = NULL;
    }

    if (listPtr->listVarName != NULL) {
	Tcl_UntraceVar2(listPtr->interp, listPtr->listVarName, NULL,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		ListboxListVarProc, listPtr);
    }

    /*
     * Free the selection hash table.
     */

    Tcl_DeleteHashTable(listPtr->selection);
    ckfree(listPtr->selection);

    /*
     * Free the item attribute hash table.
     */

    for (entry = Tcl_FirstHashEntry(listPtr->itemAttrTable, &search);
	    entry != NULL; entry = Tcl_NextHashEntry(&search)) {
	ckfree(Tcl_GetHashValue(entry));
    }
    Tcl_DeleteHashTable(listPtr->itemAttrTable);
    ckfree(listPtr->itemAttrTable);

    /*
     * Free up all the stuff that requires special handling, then let
     * Tk_FreeOptions handle all the standard option-related stuff.
     */

    if (listPtr->textGC != NULL) {
	Tk_FreeGC(listPtr->display, listPtr->textGC);
    }
    if (listPtr->selTextGC != NULL) {
	Tk_FreeGC(listPtr->display, listPtr->selTextGC);
    }
    if (listPtr->gray != None) {
	Tk_FreeBitmap(Tk_Display(listPtr->tkwin), listPtr->gray);
    }

    Tk_FreeConfigOptions((char *) listPtr, listPtr->optionTable,
	    listPtr->tkwin);
    Tcl_Release(listPtr->tkwin);
    listPtr->tkwin = NULL;
    ckfree(listPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyListboxOptionTables --
 *
 *	This procedure is registered as an exit callback when the listbox
 *	command is first called. It cleans up the OptionTables structure
 *	allocated by that command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyListboxOptionTables(
    ClientData clientData,	/* Pointer to the OptionTables struct */
    Tcl_Interp *interp)		/* Pointer to the calling interp */
{
    ckfree(clientData);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureListbox --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a listbox
 *	widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width, etc. get set
 *	for listPtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureListbox(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register Listbox *listPtr,	/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc,			/* Number of valid entries in argv. */
    Tcl_Obj *const objv[])	/* Arguments. */
{
    Tk_SavedOptions savedOptions;
    Tcl_Obj *oldListObj = NULL;
    Tcl_Obj *errorResult = NULL;
    int oldExport, error;

    oldExport = (listPtr->exportSelection) && (!Tcl_IsSafe(listPtr->interp));
    if (listPtr->listVarName != NULL) {
	Tcl_UntraceVar2(interp, listPtr->listVarName, NULL,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		ListboxListVarProc, listPtr);
    }

    for (error = 0; error <= 1; error++) {
	if (!error) {
	    /*
	     * First pass: set options to new values.
	     */

	    if (Tk_SetOptions(interp, (char *) listPtr,
		    listPtr->optionTable, objc, objv,
		    listPtr->tkwin, &savedOptions, NULL) != TCL_OK) {
		continue;
	    }
	} else {
	    /*
	     * Second pass: restore options to old values.
	     */

	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}

	/*
	 * A few options need special processing, such as setting the
	 * background from a 3-D border.
	 */

	Tk_SetBackgroundFromBorder(listPtr->tkwin, listPtr->normalBorder);

	if (listPtr->highlightWidth < 0) {
	    listPtr->highlightWidth = 0;
	}
	listPtr->inset = listPtr->highlightWidth + listPtr->borderWidth;

	/*
	 * Claim the selection if we've suddenly started exporting it and
	 * there is a selection to export and this interp is unsafe.
	 */

	if (listPtr->exportSelection && (!oldExport)
		&& (!Tcl_IsSafe(listPtr->interp))
		&& (listPtr->numSelected != 0)) {
	    Tk_OwnSelection(listPtr->tkwin, XA_PRIMARY,
		    ListboxLostSelection, listPtr);
	}

	/*
	 * Verify the current status of the list var.
	 * PREVIOUS STATE | NEW STATE  | ACTION
	 * ---------------+------------+----------------------------------
	 * no listvar     | listvar    | If listvar does not exist, create it
	 *				 and copy the internal list obj's
	 *				 content to the new var. If it does
	 *				 exist, toss the internal list obj.
	 *
	 * listvar	  | no listvar | Copy old listvar content to the
	 *				 internal list obj
	 *
	 * listvar	  | listvar    | no special action
	 *
	 * no listvar     | no listvar | no special action
	 */

	oldListObj = listPtr->listObj;
	if (listPtr->listVarName != NULL) {
	    Tcl_Obj *listVarObj = Tcl_GetVar2Ex(interp, listPtr->listVarName,
		    NULL, TCL_GLOBAL_ONLY);
	    int dummy;

	    if (listVarObj == NULL) {
		listVarObj = (oldListObj ? oldListObj : Tcl_NewObj());
		if (Tcl_SetVar2Ex(interp, listPtr->listVarName, NULL,
			listVarObj, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG)
			== NULL) {
		    continue;
		}
	    }

	    /*
	     * Make sure the object is a good list object.
	     */

	    if (Tcl_ListObjLength(listPtr->interp, listVarObj, &dummy)
		    != TCL_OK) {
		Tcl_AppendResult(listPtr->interp,
			": invalid -listvariable value", NULL);
		continue;
	    }

	    listPtr->listObj = listVarObj;
	    Tcl_TraceVar2(listPtr->interp, listPtr->listVarName,
		    NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    ListboxListVarProc, listPtr);
	} else if (listPtr->listObj == NULL) {
	    listPtr->listObj = Tcl_NewObj();
	}
	Tcl_IncrRefCount(listPtr->listObj);
	if (oldListObj != NULL) {
	    Tcl_DecrRefCount(oldListObj);
	}
	break;
    }
    if (!error) {
	Tk_FreeSavedOptions(&savedOptions);
    }

    /*
     * Make sure that the list length is correct.
     */

    Tcl_ListObjLength(listPtr->interp, listPtr->listObj, &listPtr->nElements);

    if (error) {
	Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    }
    ListboxWorldChanged(listPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureListboxItem --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a listbox
 *	item.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width, etc. get set
 *	for a listbox item; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureListboxItem(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register Listbox *listPtr,	/* Information about widget; may or may not
				 * already have values for some fields. */
    ItemAttr *attrs,		/* Information about the item to configure */
    int objc,			/* Number of valid entries in argv. */
    Tcl_Obj *const objv[],	/* Arguments. */
    int index)			/* Index of the listbox item being configure */
{
    Tk_SavedOptions savedOptions;

    if (Tk_SetOptions(interp, (char *)attrs,
	    listPtr->itemAttrOptionTable, objc, objv, listPtr->tkwin,
	    &savedOptions, NULL) != TCL_OK) {
	Tk_RestoreSavedOptions(&savedOptions);
	return TCL_ERROR;
    }
    Tk_FreeSavedOptions(&savedOptions);

    /*
     * Redraw this index - ListboxWorldChanged would need to be called if item
     * attributes were checked in the "world".
     */

    EventuallyRedrawRange(listPtr, index, index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListboxWorldChanged --
 *
 *	This procedure is called when the world has changed in some way and
 *	the widget needs to recompute all its graphics contexts and determine
 *	its new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Listbox will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
ListboxWorldChanged(
    ClientData instanceData)	/* Information about widget. */
{
    XGCValues gcValues;
    GC gc;
    unsigned long mask;
    Listbox *listPtr = instanceData;

    if (listPtr->state & STATE_NORMAL) {
	gcValues.foreground = listPtr->fgColorPtr->pixel;
	gcValues.graphics_exposures = False;
	mask = GCForeground | GCFont | GCGraphicsExposures;
    } else if (listPtr->dfgColorPtr != NULL) {
	gcValues.foreground = listPtr->dfgColorPtr->pixel;
	gcValues.graphics_exposures = False;
	mask = GCForeground | GCFont | GCGraphicsExposures;
    } else {
	gcValues.foreground = listPtr->fgColorPtr->pixel;
	mask = GCForeground | GCFont;
	if (listPtr->gray == None) {
	    listPtr->gray = Tk_GetBitmap(NULL, listPtr->tkwin, "gray50");
	}
	if (listPtr->gray != None) {
	    gcValues.fill_style = FillStippled;
	    gcValues.stipple = listPtr->gray;
	    mask |= GCFillStyle | GCStipple;
	}
    }

    gcValues.font = Tk_FontId(listPtr->tkfont);
    gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
    if (listPtr->textGC != NULL) {
	Tk_FreeGC(listPtr->display, listPtr->textGC);
    }
    listPtr->textGC = gc;

    if (listPtr->selFgColorPtr != NULL) {
	gcValues.foreground = listPtr->selFgColorPtr->pixel;
    }
    gcValues.font = Tk_FontId(listPtr->tkfont);
    mask = GCForeground | GCFont;
    gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
    if (listPtr->selTextGC != NULL) {
	Tk_FreeGC(listPtr->display, listPtr->selTextGC);
    }
    listPtr->selTextGC = gc;

    /*
     * Register the desired geometry for the window and arrange for the window
     * to be redisplayed.
     */

    ListboxComputeGeometry(listPtr, 1, 1, 1);
    listPtr->flags |= UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR;
    EventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
}

/*
 *--------------------------------------------------------------
 *
 * DisplayListbox --
 *
 *	This procedure redraws the contents of a listbox window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayListbox(
    ClientData clientData)	/* Information about window. */
{
    register Listbox *listPtr = clientData;
    register Tk_Window tkwin = listPtr->tkwin;
    GC gc;
    int i, limit, x, y, prevSelected, freeGC, stringLen;
    Tk_FontMetrics fm;
    Tcl_Obj *curElement;
    Tcl_HashEntry *entry;
    const char *stringRep;
    ItemAttr *attrs;
    Tk_3DBorder selectedBg;
    XGCValues gcValues;
    unsigned long mask;
    int left, right;		/* Non-zero values here indicate that the left
				 * or right edge of the listbox is
				 * off-screen. */
    Pixmap pixmap;
    int textWidth;

    listPtr->flags &= ~REDRAW_PENDING;
    if (listPtr->flags & LISTBOX_DELETED) {
	return;
    }

    if (listPtr->flags & MAXWIDTH_IS_STALE) {
	ListboxComputeGeometry(listPtr, 0, 1, 0);
	listPtr->flags &= ~MAXWIDTH_IS_STALE;
	listPtr->flags |= UPDATE_H_SCROLLBAR;
    }

    Tcl_Preserve(listPtr);
    if (listPtr->flags & UPDATE_V_SCROLLBAR) {
	ListboxUpdateVScrollbar(listPtr);
	if ((listPtr->flags & LISTBOX_DELETED) || !Tk_IsMapped(tkwin)) {
	    Tcl_Release(listPtr);
	    return;
	}
    }
    if (listPtr->flags & UPDATE_H_SCROLLBAR) {
	ListboxUpdateHScrollbar(listPtr);
	if ((listPtr->flags & LISTBOX_DELETED) || !Tk_IsMapped(tkwin)) {
	    Tcl_Release(listPtr);
	    return;
	}
    }
    listPtr->flags &= ~(REDRAW_PENDING|UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR);
    Tcl_Release(listPtr);

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Redrawing is done in a temporary pixmap that is allocated here and
     * freed at the end of the procedure. All drawing is done to the pixmap,
     * and the pixmap is copied to the screen at the end of the procedure.
     * This provides the smoothest possible visual effects (no flashing on the
     * screen).
     */

    pixmap = Tk_GetPixmap(listPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
#else
    pixmap = Tk_WindowId(tkwin);
#endif /* TK_NO_DOUBLE_BUFFERING */
    Tk_Fill3DRectangle(tkwin, pixmap, listPtr->normalBorder, 0, 0,
	    Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    /*
     * Display each item in the listbox.
     */

    limit = listPtr->topIndex + listPtr->fullLines + listPtr->partialLine - 1;
    if (limit >= listPtr->nElements) {
	limit = listPtr->nElements-1;
    }
    left = right = 0;
    if (listPtr->xOffset > 0) {
	left = listPtr->selBorderWidth+1;
    }
    if ((listPtr->maxWidth - listPtr->xOffset) > (Tk_Width(listPtr->tkwin)
	    - 2*(listPtr->inset + listPtr->selBorderWidth))) {
	right = listPtr->selBorderWidth+1;
    }
    prevSelected = 0;

    for (i = listPtr->topIndex; i <= limit; i++) {
	int width = Tk_Width(tkwin);	/* zeroth approx to silence warning */

	x = listPtr->inset;
	y = ((i - listPtr->topIndex) * listPtr->lineHeight) + listPtr->inset;
	gc = listPtr->textGC;
	freeGC = 0;

	/*
	 * Lookup this item in the item attributes table, to see if it has
	 * special foreground/background colors.
	 */

	entry = Tcl_FindHashEntry(listPtr->itemAttrTable, KEY(i));

	/*
	 * If the listbox is enabled, items may be drawn differently; they may
	 * be drawn selected, or they may have special foreground or
	 * background colors.
	 */

	if (listPtr->state & STATE_NORMAL) {
	    if (Tcl_FindHashEntry(listPtr->selection, KEY(i))) {
		/*
		 * Selected items are drawn differently.
		 */

		gc = listPtr->selTextGC;
		width = Tk_Width(tkwin) - 2*listPtr->inset;
		selectedBg = listPtr->selBorder;

		/*
		 * If there is attribute information for this item, adjust the
		 * drawing accordingly.
		 */

		if (entry != NULL) {
		    attrs = Tcl_GetHashValue(entry);

		    /*
		     * Default GC has the values from the widget at large.
		     */

		    if (listPtr->selFgColorPtr) {
			gcValues.foreground = listPtr->selFgColorPtr->pixel;
		    } else {
			gcValues.foreground = listPtr->fgColorPtr->pixel;
		    }
		    gcValues.font = Tk_FontId(listPtr->tkfont);
		    gcValues.graphics_exposures = False;
		    mask = GCForeground | GCFont | GCGraphicsExposures;

		    if (attrs->selBorder != NULL) {
			selectedBg = attrs->selBorder;
		    }

		    if (attrs->selFgColor != NULL) {
			gcValues.foreground = attrs->selFgColor->pixel;
			gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
			freeGC = 1;
		    }
		}

		Tk_Fill3DRectangle(tkwin, pixmap, selectedBg, x, y,
			width, listPtr->lineHeight, 0, TK_RELIEF_FLAT);

		/*
		 * Draw beveled edges around the selection, if there are
		 * visible edges next to this element. Special considerations:
		 *
		 * 1. The left and right bevels may not be visible if
		 *	horizontal scrolling is enabled (the "left" & "right"
		 *	variables are zero to indicate that the corresponding
		 *	bevel is visible).
		 * 2. Top and bottom bevels are only drawn if this is the
		 *	first or last seleted item.
		 * 3. If the left or right bevel isn't visible, then the
		 *	"left" & "right" vars, computed above, have non-zero
		 *	values that extend the top and bottom bevels so that
		 *	the mitered corners are off-screen.
		 */

		/* Draw left bevel */
		if (left == 0) {
		    Tk_3DVerticalBevel(tkwin, pixmap, selectedBg,
			    x, y, listPtr->selBorderWidth, listPtr->lineHeight,
			    1, TK_RELIEF_RAISED);
		}
		/* Draw right bevel */
		if (right == 0) {
		    Tk_3DVerticalBevel(tkwin, pixmap, selectedBg,
			    x + width - listPtr->selBorderWidth, y,
			    listPtr->selBorderWidth, listPtr->lineHeight,
			    0, TK_RELIEF_RAISED);
		}
		/* Draw top bevel */
		if (!prevSelected) {
		    Tk_3DHorizontalBevel(tkwin, pixmap, selectedBg,
			    x-left, y, width+left+right,
			    listPtr->selBorderWidth,
			    1, 1, 1, TK_RELIEF_RAISED);
		}
		/* Draw bottom bevel */
		if (i + 1 == listPtr->nElements ||
			!Tcl_FindHashEntry(listPtr->selection, KEY(i + 1))) {
		    Tk_3DHorizontalBevel(tkwin, pixmap, selectedBg, x-left,
			    y + listPtr->lineHeight - listPtr->selBorderWidth,
			    width+left+right, listPtr->selBorderWidth, 0, 0, 0,
			    TK_RELIEF_RAISED);
		}
		prevSelected = 1;
	    } else {
		/*
		 * If there is an item attributes record for this item, draw
		 * the background box and set the foreground color accordingly.
		 */

		if (entry != NULL) {
		    attrs = Tcl_GetHashValue(entry);
		    gcValues.foreground = listPtr->fgColorPtr->pixel;
		    gcValues.font = Tk_FontId(listPtr->tkfont);
		    gcValues.graphics_exposures = False;
		    mask = GCForeground | GCFont | GCGraphicsExposures;

		    /*
		     * If the item has its own background color, draw it now.
		     */

		    if (attrs->border != NULL) {
			width = Tk_Width(tkwin) - 2*listPtr->inset;
			Tk_Fill3DRectangle(tkwin, pixmap, attrs->border, x, y,
				width, listPtr->lineHeight, 0, TK_RELIEF_FLAT);
		    }

		    /*
		     * If the item has its own foreground, use it to override
		     * the value in the gcValues structure.
		     */

		    if ((listPtr->state & STATE_NORMAL)
			    && attrs->fgColor != NULL) {
			gcValues.foreground = attrs->fgColor->pixel;
			gc = Tk_GetGC(listPtr->tkwin, mask, &gcValues);
			freeGC = 1;
		    }
		}
		prevSelected = 0;
	    }
	}

	/*
	 * Draw the actual text of this item.
	 */

        Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i, &curElement);
        stringRep = Tcl_GetStringFromObj(curElement, &stringLen);
        textWidth = Tk_TextWidth(listPtr->tkfont, stringRep, stringLen);

	Tk_GetFontMetrics(listPtr->tkfont, &fm);
	y += fm.ascent + listPtr->selBorderWidth;

        if (listPtr->justify == TK_JUSTIFY_LEFT) {
            x = (listPtr->inset + listPtr->selBorderWidth) - listPtr->xOffset;
        } else if (listPtr->justify == TK_JUSTIFY_RIGHT) {
            x = Tk_Width(tkwin) - (listPtr->inset + listPtr->selBorderWidth)
                    - textWidth - listPtr->xOffset + GetMaxOffset(listPtr);
        } else {
            x = (Tk_Width(tkwin) - textWidth)/2
                    - listPtr->xOffset + GetMaxOffset(listPtr)/2;
        }

        Tk_DrawChars(listPtr->display, pixmap, gc, listPtr->tkfont,
		stringRep, stringLen, x, y);

	/*
	 * If this is the active element, apply the activestyle to it.
	 */

	if ((i == listPtr->active) && (listPtr->flags & GOT_FOCUS)) {
	    if (listPtr->activeStyle == ACTIVE_STYLE_UNDERLINE) {
		/*
		 * Underline the text.
		 */

		Tk_UnderlineChars(listPtr->display, pixmap, gc,
			listPtr->tkfont, stringRep, x, y, 0, stringLen);
	    } else if (listPtr->activeStyle == ACTIVE_STYLE_DOTBOX) {
#ifdef _WIN32
		/*
		 * This provides for exact default look and feel on Windows.
		 */

		TkWinDCState state;
		HDC dc;
		RECT rect;

		dc = TkWinGetDrawableDC(listPtr->display, pixmap, &state);
		rect.left = listPtr->inset;
		rect.top = ((i - listPtr->topIndex) * listPtr->lineHeight)
			+ listPtr->inset;
		rect.right = rect.left + width;
		rect.bottom = rect.top + listPtr->lineHeight;
		DrawFocusRect(dc, &rect);
		TkWinReleaseDrawableDC(pixmap, dc, &state);
#else /* !_WIN32 */
		/*
		 * Draw a dotted box around the text.
		 */

		x = listPtr->inset;
		y = ((i - listPtr->topIndex) * listPtr->lineHeight)
			+ listPtr->inset;
		width = Tk_Width(tkwin) - 2*listPtr->inset - 1;

		gcValues.line_style = LineOnOffDash;
		gcValues.line_width = listPtr->selBorderWidth;
		if (gcValues.line_width <= 0) {
		    gcValues.line_width  = 1;
		}
		gcValues.dash_offset = 0;
		gcValues.dashes = 1;

		/*
		 * You would think the XSetDashes was necessary, but it
		 * appears that the default dotting for just saying we want
		 * dashes appears to work correctly.
		 static char dashList[] = { 1 };
		 static int dashLen = sizeof(dashList);
		 XSetDashes(listPtr->display, gc, 0, dashList, dashLen);
		 */

		mask = GCLineWidth | GCLineStyle | GCDashList | GCDashOffset;
		XChangeGC(listPtr->display, gc, mask, &gcValues);
		XDrawRectangle(listPtr->display, pixmap, gc, x, y,
			(unsigned) width, (unsigned) listPtr->lineHeight - 1);
		if (!freeGC) {
		    /*
		     * Don't bother changing if it is about to be freed.
		     */

		    gcValues.line_style = LineSolid;
		    XChangeGC(listPtr->display, gc, GCLineStyle, &gcValues);
		}
#endif /* _WIN32 */
	    }
	}

	if (freeGC) {
	    Tk_FreeGC(listPtr->display, gc);
	}
    }

    /*
     * Redraw the border for the listbox to make sure that it's on top of any
     * of the text of the listbox entries.
     */

    Tk_Draw3DRectangle(tkwin, pixmap, listPtr->normalBorder,
	    listPtr->highlightWidth, listPtr->highlightWidth,
	    Tk_Width(tkwin) - 2*listPtr->highlightWidth,
	    Tk_Height(tkwin) - 2*listPtr->highlightWidth,
	    listPtr->borderWidth, listPtr->relief);
    if (listPtr->highlightWidth > 0) {
	GC fgGC, bgGC;

	bgGC = Tk_GCForColor(listPtr->highlightBgColorPtr, pixmap);
	if (listPtr->flags & GOT_FOCUS) {
	    fgGC = Tk_GCForColor(listPtr->highlightColorPtr, pixmap);
	    TkpDrawHighlightBorder(tkwin, fgGC, bgGC,
		    listPtr->highlightWidth, pixmap);
	} else {
	    TkpDrawHighlightBorder(tkwin, bgGC, bgGC,
		    listPtr->highlightWidth, pixmap);
	}
    }
#ifndef TK_NO_DOUBLE_BUFFERING
    XCopyArea(listPtr->display, pixmap, Tk_WindowId(tkwin),
	    listPtr->textGC, 0, 0, (unsigned) Tk_Width(tkwin),
	    (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(listPtr->display, pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxComputeGeometry --
 *
 *	This procedure is invoked to recompute geometry information such as
 *	the sizes of the elements and the overall dimensions desired for the
 *	listbox.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Geometry information is updated and a new requested size is registered
 *	for the widget. Internal border and gridding information is also set.
 *
 *----------------------------------------------------------------------
 */

static void
ListboxComputeGeometry(
    Listbox *listPtr,		/* Listbox whose geometry is to be
				 * recomputed. */
    int fontChanged,		/* Non-zero means the font may have changed so
				 * per-element width information also has to
				 * be computed. */
    int maxIsStale,		/* Non-zero means the "maxWidth" field may no
				 * longer be up-to-date and must be
				 * recomputed. If fontChanged is 1 then this
				 * must be 1. */
    int updateGrid)		/* Non-zero means call Tk_SetGrid or
				 * Tk_UnsetGrid to update gridding for the
				 * window. */
{
    int width, height, pixelWidth, pixelHeight, textLength, i, result;
    Tk_FontMetrics fm;
    Tcl_Obj *element;
    const char *text;

    if (fontChanged || maxIsStale) {
	listPtr->xScrollUnit = Tk_TextWidth(listPtr->tkfont, "0", 1);
	if (listPtr->xScrollUnit == 0) {
	    listPtr->xScrollUnit = 1;
	}
	listPtr->maxWidth = 0;
	for (i = 0; i < listPtr->nElements; i++) {
	    /*
	     * Compute the pixel width of the current element.
	     */

	    result = Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i,
		    &element);
	    if (result != TCL_OK) {
		continue;
	    }
	    text = Tcl_GetStringFromObj(element, &textLength);
	    Tk_GetFontMetrics(listPtr->tkfont, &fm);
	    pixelWidth = Tk_TextWidth(listPtr->tkfont, text, textLength);
	    if (pixelWidth > listPtr->maxWidth) {
		listPtr->maxWidth = pixelWidth;
	    }
	}
    }

    Tk_GetFontMetrics(listPtr->tkfont, &fm);
    listPtr->lineHeight = fm.linespace + 1 + 2*listPtr->selBorderWidth;
    width = listPtr->width;
    if (width <= 0) {
	width = (listPtr->maxWidth + listPtr->xScrollUnit - 1)
		/ listPtr->xScrollUnit;
	if (width < 1) {
	    width = 1;
	}
    }
    pixelWidth = width*listPtr->xScrollUnit + 2*listPtr->inset
	    + 2*listPtr->selBorderWidth;
    height = listPtr->height;
    if (listPtr->height <= 0) {
	height = listPtr->nElements;
	if (height < 1) {
	    height = 1;
	}
    }
    pixelHeight = height*listPtr->lineHeight + 2*listPtr->inset;
    Tk_GeometryRequest(listPtr->tkwin, pixelWidth, pixelHeight);
    Tk_SetInternalBorder(listPtr->tkwin, listPtr->inset);
    if (updateGrid) {
	if (listPtr->setGrid) {
	    Tk_SetGrid(listPtr->tkwin, width, height, listPtr->xScrollUnit,
		    listPtr->lineHeight);
	} else {
	    Tk_UnsetGrid(listPtr->tkwin);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxInsertSubCmd --
 *
 *	This procedure is invoked to handle the listbox "insert" subcommand.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	New elements are added to the listbox pointed to by listPtr; a refresh
 *	callback is registered for the listbox.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxInsertSubCmd(
    register Listbox *listPtr,	/* Listbox that is to get the new elements. */
    int index,			/* Add the new elements before this
				 * element. */
    int objc,			/* Number of new elements to add. */
    Tcl_Obj *const objv[])	/* New elements (one per entry). */
{
    int i, oldMaxWidth, pixelWidth, result, length;
    Tcl_Obj *newListObj;
    const char *stringRep;

    oldMaxWidth = listPtr->maxWidth;
    for (i = 0; i < objc; i++) {
	/*
	 * Check if any of the new elements are wider than the current widest;
	 * if so, update our notion of "widest."
	 */

	stringRep = Tcl_GetStringFromObj(objv[i], &length);
	pixelWidth = Tk_TextWidth(listPtr->tkfont, stringRep, length);
	if (pixelWidth > listPtr->maxWidth) {
	    listPtr->maxWidth = pixelWidth;
	}
    }

    /*
     * Adjust selection and attribute information for every index after the
     * first index.
     */

    MigrateHashEntries(listPtr->selection, index, listPtr->nElements-1, objc);
    MigrateHashEntries(listPtr->itemAttrTable, index, listPtr->nElements-1,
	    objc);

    /*
     * If the object is shared, duplicate it before writing to it.
     */

    if (Tcl_IsShared(listPtr->listObj)) {
	newListObj = Tcl_DuplicateObj(listPtr->listObj);
    } else {
	newListObj = listPtr->listObj;
    }
    result = Tcl_ListObjReplace(listPtr->interp, newListObj, index, 0,
	    objc, objv);
    if (result != TCL_OK) {
	return result;
    }

    /*
     * Replace the current object and set attached listvar, if any. This may
     * error if listvar points to a var in a deleted namespace, but we ignore
     * those errors. If the namespace is recreated, it will auto-sync with the
     * current value. [Bug 1424513]
     */

    Tcl_IncrRefCount(newListObj);
    Tcl_DecrRefCount(listPtr->listObj);
    listPtr->listObj = newListObj;
    if (listPtr->listVarName != NULL) {
	Tcl_SetVar2Ex(listPtr->interp, listPtr->listVarName, NULL,
		listPtr->listObj, TCL_GLOBAL_ONLY);
    }

    /*
     * Get the new list length.
     */

    Tcl_ListObjLength(listPtr->interp, listPtr->listObj, &listPtr->nElements);

    /*
     * Update the "special" indices (anchor, topIndex, active) to account for
     * the renumbering that just occurred. Then arrange for the new
     * information to be displayed.
     */

    if (index <= listPtr->selectAnchor) {
	listPtr->selectAnchor += objc;
    }
    if (index < listPtr->topIndex) {
	listPtr->topIndex += objc;
    }
    if (index <= listPtr->active) {
	listPtr->active += objc;
	if ((listPtr->active >= listPtr->nElements) &&
		(listPtr->nElements > 0)) {
	    listPtr->active = listPtr->nElements-1;
	}
    }
    listPtr->flags |= UPDATE_V_SCROLLBAR;
    if (listPtr->maxWidth != oldMaxWidth) {
	listPtr->flags |= UPDATE_H_SCROLLBAR;
    }
    ListboxComputeGeometry(listPtr, 0, 0, 0);
    EventuallyRedrawRange(listPtr, index, listPtr->nElements-1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxDeleteSubCmd --
 *
 *	Process a listbox "delete" subcommand by removing one or more elements
 *	from a listbox widget.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	The listbox will be modified and (eventually) redisplayed.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxDeleteSubCmd(
    register Listbox *listPtr,	/* Listbox widget to modify. */
    int first,			/* Index of first element to delete. */
    int last)			/* Index of last element to delete. */
{
    int count, i, widthChanged, length, result, pixelWidth;
    Tcl_Obj *newListObj, *element;
    const char *stringRep;
    Tcl_HashEntry *entry;

    /*
     * Adjust the range to fit within the existing elements of the listbox,
     * and make sure there's something to delete.
     */

    if (first < 0) {
	first = 0;
    }
    if (last >= listPtr->nElements) {
	last = listPtr->nElements-1;
    }
    count = last + 1 - first;
    if (count <= 0) {
	return TCL_OK;
    }

    /*
     * Foreach deleted index we must:
     * a) remove selection information,
     * b) check the width of the element; if it is equal to the max, set
     *    widthChanged to 1, because it may be the only element with that
     *    width.
     */

    widthChanged = 0;
    for (i = first; i <= last; i++) {
	/*
	 * Remove selection information.
	 */

	entry = Tcl_FindHashEntry(listPtr->selection, KEY(i));
	if (entry != NULL) {
	    listPtr->numSelected--;
	    Tcl_DeleteHashEntry(entry);
	}

	entry = Tcl_FindHashEntry(listPtr->itemAttrTable, KEY(i));
	if (entry != NULL) {
	    ckfree(Tcl_GetHashValue(entry));
	    Tcl_DeleteHashEntry(entry);
	}

	/*
	 * Check width of the element. We only have to check if widthChanged
	 * has not already been set to 1, because we only need one maxWidth
	 * element to disappear for us to have to recompute the width.
	 */

	if (widthChanged == 0) {
	    Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i, &element);
	    stringRep = Tcl_GetStringFromObj(element, &length);
	    pixelWidth = Tk_TextWidth(listPtr->tkfont, stringRep, length);
	    if (pixelWidth == listPtr->maxWidth) {
		widthChanged = 1;
	    }
	}
    }

    /*
     * Adjust selection and attribute info for indices after lastIndex.
     */

    MigrateHashEntries(listPtr->selection, last+1,
	    listPtr->nElements-1, count*-1);
    MigrateHashEntries(listPtr->itemAttrTable, last+1,
	    listPtr->nElements-1, count*-1);

    /*
     * Delete the requested elements.
     */

    if (Tcl_IsShared(listPtr->listObj)) {
	newListObj = Tcl_DuplicateObj(listPtr->listObj);
    } else {
	newListObj = listPtr->listObj;
    }
    result = Tcl_ListObjReplace(listPtr->interp,
	    newListObj, first, count, 0, NULL);
    if (result != TCL_OK) {
	return result;
    }

    /*
     * Replace the current object and set attached listvar, if any. This may
     * error if listvar points to a var in a deleted namespace, but we ignore
     * those errors. If the namespace is recreated, it will auto-sync with the
     * current value. [Bug 1424513]
     */

    Tcl_IncrRefCount(newListObj);
    Tcl_DecrRefCount(listPtr->listObj);
    listPtr->listObj = newListObj;
    if (listPtr->listVarName != NULL) {
	Tcl_SetVar2Ex(listPtr->interp, listPtr->listVarName, NULL,
		listPtr->listObj, TCL_GLOBAL_ONLY);
    }

    /*
     * Get the new list length.
     */

    Tcl_ListObjLength(listPtr->interp, listPtr->listObj, &listPtr->nElements);

    /*
     * Update the selection and viewing information to reflect the change in
     * the element numbering, and redisplay to slide information up over the
     * elements that were deleted.
     */

    if (first <= listPtr->selectAnchor) {
	listPtr->selectAnchor -= count;
	if (listPtr->selectAnchor < first) {
	    listPtr->selectAnchor = first;
	}
    }
    if (first <= listPtr->topIndex) {
	listPtr->topIndex -= count;
	if (listPtr->topIndex < first) {
	    listPtr->topIndex = first;
	}
    }
    if (listPtr->topIndex > (listPtr->nElements - listPtr->fullLines)) {
	listPtr->topIndex = listPtr->nElements - listPtr->fullLines;
	if (listPtr->topIndex < 0) {
	    listPtr->topIndex = 0;
	}
    }
    if (listPtr->active > last) {
	listPtr->active -= count;
    } else if (listPtr->active >= first) {
	listPtr->active = first;
	if ((listPtr->active >= listPtr->nElements) &&
		(listPtr->nElements > 0)) {
	    listPtr->active = listPtr->nElements-1;
	}
    }
    listPtr->flags |= UPDATE_V_SCROLLBAR;
    ListboxComputeGeometry(listPtr, 0, widthChanged, 0);
    if (widthChanged) {
	listPtr->flags |= UPDATE_H_SCROLLBAR;
    }
    EventuallyRedrawRange(listPtr, first, listPtr->nElements-1);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ListboxEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various events on
 *	listboxes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up. When
 *	it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ListboxEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    Listbox *listPtr = clientData;

    if (eventPtr->type == Expose) {
	EventuallyRedrawRange(listPtr,
		NearestListboxElement(listPtr, eventPtr->xexpose.y),
		NearestListboxElement(listPtr, eventPtr->xexpose.y
		+ eventPtr->xexpose.height));
    } else if (eventPtr->type == DestroyNotify) {
	if (!(listPtr->flags & LISTBOX_DELETED)) {
	    listPtr->flags |= LISTBOX_DELETED;
	    Tcl_DeleteCommandFromToken(listPtr->interp, listPtr->widgetCmd);
	    if (listPtr->setGrid) {
		Tk_UnsetGrid(listPtr->tkwin);
	    }
	    if (listPtr->flags & REDRAW_PENDING) {
		Tcl_CancelIdleCall(DisplayListbox, clientData);
	    }
	    Tcl_EventuallyFree(clientData, (Tcl_FreeProc *) DestroyListbox);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	int vertSpace;

	vertSpace = Tk_Height(listPtr->tkwin) - 2*listPtr->inset;
	listPtr->fullLines = vertSpace / listPtr->lineHeight;
	if ((listPtr->fullLines*listPtr->lineHeight) < vertSpace) {
	    listPtr->partialLine = 1;
	} else {
	    listPtr->partialLine = 0;
	}
	listPtr->flags |= UPDATE_V_SCROLLBAR|UPDATE_H_SCROLLBAR;
	ChangeListboxView(listPtr, listPtr->topIndex);
	ChangeListboxOffset(listPtr, listPtr->xOffset);

	/*
	 * Redraw the whole listbox. It's hard to tell what needs to be
	 * redrawn (e.g. if the listbox has shrunk then we may only need to
	 * redraw the borders), so just redraw everything for safety.
	 */

	EventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    listPtr->flags |= GOT_FOCUS;
	    EventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    listPtr->flags &= ~GOT_FOCUS;
	    EventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted. If the
 *	widget isn't already in the process of being destroyed, this command
 *	destroys it.
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
ListboxCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    Listbox *listPtr = clientData;

    /*
     * This procedure could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this procedure destroys the
     * widget.
     */

    if (!(listPtr->flags & LISTBOX_DELETED)) {
	Tk_DestroyWindow(listPtr->tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * GetListboxIndex --
 *
 *	Parse an index into a listbox and return either its value or an error.
 *
 * Results:
 *	A standard Tcl result. If all went well, then *indexPtr is filled in
 *	with the index (into listPtr) corresponding to string. Otherwise an
 *	error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetListboxIndex(
    Tcl_Interp *interp,		/* For error messages. */
    Listbox *listPtr,		/* Listbox for which the index is being
				 * specified. */
    Tcl_Obj *indexObj,		/* Specifies an element in the listbox. */
    int endIsSize,		/* If 1, "end" refers to the number of entries
				 * in the listbox. If 0, "end" refers to 1
				 * less than the number of entries. */
    int *indexPtr)		/* Where to store converted index. */
{
    int result, index;
    const char *stringRep;

    /*
     * First see if the index is one of the named indices.
     */

    result = Tcl_GetIndexFromObj(NULL, indexObj, indexNames, "", 0, &index);
    if (result == TCL_OK) {
	switch (index) {
	case INDEX_ACTIVE:
	    /* "active" index */
	    *indexPtr = listPtr->active;
	    break;
	case INDEX_ANCHOR:
	    /* "anchor" index */
	    *indexPtr = listPtr->selectAnchor;
	    break;
	case INDEX_END:
	    /* "end" index */
	    if (endIsSize) {
		*indexPtr = listPtr->nElements;
	    } else {
		*indexPtr = listPtr->nElements - 1;
	    }
	    break;
	}
	return TCL_OK;
    }

    /*
     * The index didn't match any of the named indices; maybe it's an @x,y
     */

    stringRep = Tcl_GetString(indexObj);
    if (stringRep[0] == '@') {

        /*
         * @x,y index
         */

	int y;
	const char *start;
	char *end;

	start = stringRep + 1;
	y = strtol(start, &end, 0);
	if ((start == end) || (*end != ',')) {
	    goto badIndex;
	}
	start = end+1;
	y = strtol(start, &end, 0);
	if ((start == end) || (*end != '\0')) {
	    goto badIndex;
	}
	*indexPtr = NearestListboxElement(listPtr, y);
	return TCL_OK;
    }

    /*
     * Maybe the index is just an integer.
     */

    if (Tcl_GetIntFromObj(interp, indexObj, indexPtr) == TCL_OK) {
	return TCL_OK;
    }

    /*
     * Everything failed, nothing matched. Throw up an error message.
     */

  badIndex:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "bad listbox index \"%s\": must be active, anchor, end, @x,y,"
	    " or a number", Tcl_GetString(indexObj)));
    Tcl_SetErrorCode(interp, "TK", "VALUE", "LISTBOX_INDEX", NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ChangeListboxView --
 *
 *	Change the view on a listbox widget so that a given element is
 *	displayed at the top.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	What's displayed on the screen is changed. If there is a scrollbar
 *	associated with this widget, then the scrollbar is instructed to
 *	change its display too.
 *
 *----------------------------------------------------------------------
 */

static void
ChangeListboxView(
    register Listbox *listPtr,	/* Information about widget. */
    int index)			/* Index of element in listPtr that should now
				 * appear at the top of the listbox. */
{
    if (index >= (listPtr->nElements - listPtr->fullLines)) {
	index = listPtr->nElements - listPtr->fullLines;
    }
    if (index < 0) {
	index = 0;
    }
    if (listPtr->topIndex != index) {
	listPtr->topIndex = index;
	EventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
	listPtr->flags |= UPDATE_V_SCROLLBAR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ChangListboxOffset --
 *
 *	Change the horizontal offset for a listbox.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The listbox may be redrawn to reflect its new horizontal offset.
 *
 *----------------------------------------------------------------------
 */

static void
ChangeListboxOffset(
    register Listbox *listPtr,	/* Information about widget. */
    int offset)			/* Desired new "xOffset" for listbox. */
{
    int maxOffset;

    /*
     * Make sure that the new offset is within the allowable range, and round
     * it off to an even multiple of xScrollUnit.
     *
     * Add half a scroll unit to do entry/text-like synchronization. [Bug
     * #225025]
     */

    offset += listPtr->xScrollUnit / 2;
    maxOffset = GetMaxOffset(listPtr);
    if (offset > maxOffset) {
	offset = maxOffset;
    }
    if (offset < 0) {
	offset = 0;
    }
    offset -= offset % listPtr->xScrollUnit;
    if (offset != listPtr->xOffset) {
	listPtr->xOffset = offset;
	listPtr->flags |= UPDATE_H_SCROLLBAR;
	EventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxScanTo --
 *
 *	Given a point (presumably of the curent mouse location) drag the view
 *	in the window to implement the scan operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The view in the window may change.
 *
 *----------------------------------------------------------------------
 */

static void
ListboxScanTo(
    register Listbox *listPtr,	/* Information about widget. */
    int x,			/* X-coordinate to use for scan operation. */
    int y)			/* Y-coordinate to use for scan operation. */
{
    int newTopIndex, newOffset, maxIndex, maxOffset;

    maxIndex = listPtr->nElements - listPtr->fullLines;
    maxOffset = GetMaxOffset(listPtr);

    /*
     * Compute new top line for screen by amplifying the difference between
     * the current position and the place where the scan started (the "mark"
     * position). If we run off the top or bottom of the list, then reset the
     * mark point so that the current position continues to correspond to the
     * edge of the window. This means that the picture will start dragging as
     * soon as the mouse reverses direction (without this reset, might have to
     * slide mouse a long ways back before the picture starts moving again).
     */

    newTopIndex = listPtr->scanMarkYIndex
	    - (10*(y - listPtr->scanMarkY)) / listPtr->lineHeight;
    if (newTopIndex > maxIndex) {
	newTopIndex = listPtr->scanMarkYIndex = maxIndex;
	listPtr->scanMarkY = y;
    } else if (newTopIndex < 0) {
	newTopIndex = listPtr->scanMarkYIndex = 0;
	listPtr->scanMarkY = y;
    }
    ChangeListboxView(listPtr, newTopIndex);

    /*
     * Compute new left edge for display in a similar fashion by amplifying
     * the difference between the current position and the place where the
     * scan started.
     */

    newOffset = listPtr->scanMarkXOffset - 10*(x - listPtr->scanMarkX);
    if (newOffset > maxOffset) {
	newOffset = listPtr->scanMarkXOffset = maxOffset;
	listPtr->scanMarkX = x;
    } else if (newOffset < 0) {
	newOffset = listPtr->scanMarkXOffset = 0;
	listPtr->scanMarkX = x;
    }
    ChangeListboxOffset(listPtr, newOffset);
}

/*
 *----------------------------------------------------------------------
 *
 * NearestListboxElement --
 *
 *	Given a y-coordinate inside a listbox, compute the index of the
 *	element under that y-coordinate (or closest to that y-coordinate).
 *
 * Results:
 *	The return value is an index of an element of listPtr. If listPtr has
 *	no elements, then 0 is always returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
NearestListboxElement(
    register Listbox *listPtr,	/* Information about widget. */
    int y)			/* Y-coordinate in listPtr's window. */
{
    int index;

    index = (y - listPtr->inset) / listPtr->lineHeight;
    if (index >= (listPtr->fullLines + listPtr->partialLine)) {
	index = listPtr->fullLines + listPtr->partialLine - 1;
    }
    if (index < 0) {
	index = 0;
    }
    index += listPtr->topIndex;
    if (index >= listPtr->nElements) {
	index = listPtr->nElements-1;
    }
    return index;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxSelect --
 *
 *	Select or deselect one or more elements in a listbox..
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	All of the elements in the range between first and last are marked as
 *	either selected or deselected, depending on the "select" argument. Any
 *	items whose state changes are redisplayed. The selection is claimed
 *	from X when the number of selected elements changes from zero to
 *	non-zero.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxSelect(
    register Listbox *listPtr,	/* Information about widget. */
    int first,			/* Index of first element to select or
				 * deselect. */
    int last,			/* Index of last element to select or
				 * deselect. */
    int select)			/* 1 means select items, 0 means deselect
				 * them. */
{
    int i, firstRedisplay, oldCount, isNew;
    Tcl_HashEntry *entry;

    if (last < first) {
	i = first;
	first = last;
	last = i;
    }
    if ((last < 0) || (first >= listPtr->nElements)) {
	return TCL_OK;
    }
    if (first < 0) {
	first = 0;
    }
    if (last >= listPtr->nElements) {
	last = listPtr->nElements - 1;
    }
    oldCount = listPtr->numSelected;
    firstRedisplay = -1;

    /*
     * For each index in the range, find it in our selection hash table. If
     * it's not there but should be, add it. If it's there but shouldn't be,
     * remove it.
     */

    for (i = first; i <= last; i++) {
	entry = Tcl_FindHashEntry(listPtr->selection, KEY(i));
	if (entry != NULL) {
	    if (!select) {
		Tcl_DeleteHashEntry(entry);
		listPtr->numSelected--;
		if (firstRedisplay < 0) {
		    firstRedisplay = i;
		}
	    }
	} else {
	    if (select) {
		entry = Tcl_CreateHashEntry(listPtr->selection, KEY(i),
			&isNew);
		Tcl_SetHashValue(entry, NULL);
		listPtr->numSelected++;
		if (firstRedisplay < 0) {
		    firstRedisplay = i;
		}
	    }
	}
    }

    if (firstRedisplay >= 0) {
	EventuallyRedrawRange(listPtr, first, last);
    }
    if ((oldCount == 0) && (listPtr->numSelected > 0)
	    && (listPtr->exportSelection)
	    && (!Tcl_IsSafe(listPtr->interp))) {
	Tk_OwnSelection(listPtr->tkwin, XA_PRIMARY,
		ListboxLostSelection, listPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxFetchSelection --
 *
 *	This procedure is called back by Tk when the selection is requested by
 *	someone. It returns part or all of the selection in a buffer provided
 *	by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored at buffer.
 *	Buffer is filled (or partially filled) with a NULL-terminated string
 *	containing part or all of the selection, as given by offset and
 *	maxBytes. The selection is returned as a Tcl list with one list
 *	element for each element in the listbox.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ListboxFetchSelection(
    ClientData clientData,	/* Information about listbox widget. */
    int offset,			/* Offset within selection of first byte to be
				 * returned. */
    char *buffer,		/* Location in which to place selection. */
    int maxBytes)		/* Maximum number of bytes to place at buffer,
				 * not including terminating NULL
				 * character. */
{
    register Listbox *listPtr = clientData;
    Tcl_DString selection;
    int length, count, needNewline, stringLen, i;
    Tcl_Obj *curElement;
    const char *stringRep;
    Tcl_HashEntry *entry;

    if ((!listPtr->exportSelection) || Tcl_IsSafe(listPtr->interp)) {
	return -1;
    }

    /*
     * Use a dynamic string to accumulate the contents of the selection.
     */

    needNewline = 0;
    Tcl_DStringInit(&selection);
    for (i = 0; i < listPtr->nElements; i++) {
	entry = Tcl_FindHashEntry(listPtr->selection, KEY(i));
	if (entry != NULL) {
	    if (needNewline) {
		Tcl_DStringAppend(&selection, "\n", 1);
	    }
	    Tcl_ListObjIndex(listPtr->interp, listPtr->listObj, i,
		    &curElement);
	    stringRep = Tcl_GetStringFromObj(curElement, &stringLen);
	    Tcl_DStringAppend(&selection, stringRep, stringLen);
	    needNewline = 1;
	}
    }

    length = Tcl_DStringLength(&selection);
    if (length == 0) {
	return -1;
    }

    /*
     * Copy the requested portion of the selection to the buffer.
     */

    count = length - offset;
    if (count <= 0) {
	count = 0;
    } else {
	if (count > maxBytes) {
	    count = maxBytes;
	}
	memcpy(buffer, Tcl_DStringValue(&selection) + offset, (size_t) count);
    }
    buffer[count] = '\0';
    Tcl_DStringFree(&selection);
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxLostSelection --
 *
 *	This procedure is called back by Tk when the selection is grabbed away
 *	from a listbox widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is marked as
 *	not containing a selection.
 *
 *----------------------------------------------------------------------
 */

static void
ListboxLostSelection(
    ClientData clientData)	/* Information about listbox widget. */
{
    register Listbox *listPtr = clientData;

    if ((listPtr->exportSelection) && (!Tcl_IsSafe(listPtr->interp))
	    && (listPtr->nElements > 0)) {
	ListboxSelect(listPtr, 0, listPtr->nElements-1, 0);
        GenerateListboxSelectEvent(listPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateListboxSelectEvent --
 *
 *	Send an event that the listbox selection was updated. This is
 *	equivalent to event generate $listboxWidget <<ListboxSelect>>
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Any side effect possible, depending on bindings to this event.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateListboxSelectEvent(
    Listbox *listPtr)		/* Information about widget. */
{
    TkSendVirtualEvent(listPtr->tkwin, "ListboxSelect", NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * EventuallyRedrawRange --
 *
 *	Ensure that a given range of elements is eventually redrawn on the
 *	display (if those elements in fact appear on the display).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets redisplayed.
 *
 *----------------------------------------------------------------------
 */

static void
EventuallyRedrawRange(
    register Listbox *listPtr,	/* Information about widget. */
    int first,			/* Index of first element in list that needs
				 * to be redrawn. */
    int last)			/* Index of last element in list that needs to
				 * be redrawn. May be less than first; these
				 * just bracket a range. */
{
    /*
     * We don't have to register a redraw callback if one is already pending,
     * or if the window doesn't exist, or if the window isn't mapped.
     */

    if ((listPtr->flags & REDRAW_PENDING)
	    || (listPtr->flags & LISTBOX_DELETED)
	    || !Tk_IsMapped(listPtr->tkwin)) {
	return;
    }
    listPtr->flags |= REDRAW_PENDING;
    Tcl_DoWhenIdle(DisplayListbox, listPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxUpdateVScrollbar --
 *
 *	This procedure is invoked whenever information has changed in a
 *	listbox in a way that would invalidate a vertical scrollbar display.
 *	If there is an associated scrollbar, then this command updates it by
 *	invoking a Tcl command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional command may be invoked to
 *	process errors in the command.
 *
 *----------------------------------------------------------------------
 */

static void
ListboxUpdateVScrollbar(
    register Listbox *listPtr)	/* Information about widget. */
{
    char firstStr[TCL_DOUBLE_SPACE], lastStr[TCL_DOUBLE_SPACE];
    double first, last;
    int result;
    Tcl_Interp *interp;
    Tcl_DString buf;

    if (listPtr->yScrollCmd == NULL) {
	return;
    }
    if (listPtr->nElements == 0) {
	first = 0.0;
	last = 1.0;
    } else {
	first = listPtr->topIndex / (double) listPtr->nElements;
	last = (listPtr->topIndex + listPtr->fullLines)
		/ (double) listPtr->nElements;
	if (last > 1.0) {
	    last = 1.0;
	}
    }
    Tcl_PrintDouble(NULL, first, firstStr);
    Tcl_PrintDouble(NULL, last, lastStr);

    /*
     * We must hold onto the interpreter from the listPtr because the data at
     * listPtr might be freed as a result of the Tcl_VarEval.
     */

    interp = listPtr->interp;
    Tcl_Preserve(interp);
    Tcl_DStringInit(&buf);
    Tcl_DStringAppend(&buf, listPtr->yScrollCmd, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, firstStr, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, lastStr, -1);
    result = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
    Tcl_DStringFree(&buf);
    if (result != TCL_OK) {
	Tcl_AddErrorInfo(interp,
		"\n    (vertical scrolling command executed by listbox)");
	Tcl_BackgroundException(interp, result);
    }
    Tcl_Release(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxUpdateHScrollbar --
 *
 *	This procedure is invoked whenever information has changed in a
 *	listbox in a way that would invalidate a horizontal scrollbar display.
 *	If there is an associated horizontal scrollbar, then this command
 *	updates it by invoking a Tcl command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional command may be invoked to
 *	process errors in the command.
 *
 *----------------------------------------------------------------------
 */

static void
ListboxUpdateHScrollbar(
    register Listbox *listPtr)	/* Information about widget. */
{
    char firstStr[TCL_DOUBLE_SPACE], lastStr[TCL_DOUBLE_SPACE];
    int result, windowWidth;
    double first, last;
    Tcl_Interp *interp;
    Tcl_DString buf;

    if (listPtr->xScrollCmd == NULL) {
	return;
    }

    windowWidth = Tk_Width(listPtr->tkwin)
	    - 2*(listPtr->inset + listPtr->selBorderWidth);
    if (listPtr->maxWidth == 0) {
	first = 0;
	last = 1.0;
    } else {
	first = listPtr->xOffset / (double) listPtr->maxWidth;
	last = (listPtr->xOffset + windowWidth) / (double) listPtr->maxWidth;
	if (last > 1.0) {
	    last = 1.0;
	}
    }
    Tcl_PrintDouble(NULL, first, firstStr);
    Tcl_PrintDouble(NULL, last, lastStr);

    /*
     * We must hold onto the interpreter because the data referred to at
     * listPtr might be freed as a result of the call to Tcl_VarEval.
     */

    interp = listPtr->interp;
    Tcl_Preserve(interp);
    Tcl_DStringInit(&buf);
    Tcl_DStringAppend(&buf, listPtr->xScrollCmd, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, firstStr, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, lastStr, -1);
    result = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
    Tcl_DStringFree(&buf);
    if (result != TCL_OK) {
	Tcl_AddErrorInfo(interp,
		"\n    (horizontal scrolling command executed by listbox)");
	Tcl_BackgroundException(interp, result);
    }
    Tcl_Release(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * ListboxListVarProc --
 *
 *	Called whenever the trace on the listbox list var fires.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
ListboxListVarProc(
    ClientData clientData,	/* Information about button. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Not used. */
    const char *name2,		/* Not used. */
    int flags)			/* Information about what happened. */
{
    Listbox *listPtr = clientData;
    Tcl_Obj *oldListObj, *varListObj;
    int oldLength, i;
    Tcl_HashEntry *entry;

    /*
     * Bwah hahahaha! Puny mortal, you can't unset a -listvar'd variable!
     */

    if (flags & TCL_TRACE_UNSETS) {

        if (!Tcl_InterpDeleted(interp) && listPtr->listVarName) {
            ClientData probe = NULL;

            do {
                probe = Tcl_VarTraceInfo(interp,
                        listPtr->listVarName,
                        TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
                        ListboxListVarProc, probe);
                if (probe == (ClientData)listPtr) {
                    break;
                }
            } while (probe);
            if (probe) {
                /*
                 * We were able to fetch the unset trace for our
                 * listVarName, which means it is not unset and not
                 * the cause of this unset trace. Instead some outdated
                 * former variable must be, and we should ignore it.
                 */
                return NULL;
            }
	    Tcl_SetVar2Ex(interp, listPtr->listVarName, NULL,
		    listPtr->listObj, TCL_GLOBAL_ONLY);
	    Tcl_TraceVar2(interp, listPtr->listVarName,
		    NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    ListboxListVarProc, clientData);
	    return NULL;
	}
    } else {
	oldListObj = listPtr->listObj;
	varListObj = Tcl_GetVar2Ex(listPtr->interp, listPtr->listVarName,
		NULL, TCL_GLOBAL_ONLY);

	/*
	 * Make sure the new value is a good list; if it's not, disallow the
	 * change - the fact that it is a listvar means that it must always be
	 * a valid list - and return an error message.
	 */

	if (Tcl_ListObjLength(listPtr->interp, varListObj, &i) != TCL_OK) {
	    Tcl_SetVar2Ex(interp, listPtr->listVarName, NULL, oldListObj,
		    TCL_GLOBAL_ONLY);
	    return (char *) "invalid listvar value";
	}

	listPtr->listObj = varListObj;

	/*
	 * Incr the obj ref count so it doesn't vanish if the var is unset.
	 */

	Tcl_IncrRefCount(listPtr->listObj);

	/*
	 * Clean up the ref to our old list obj.
	 */

	Tcl_DecrRefCount(oldListObj);
    }

    /*
     * If the list length has decreased, then we should clean up selection and
     * attributes information for elements past the end of the new list.
     */

    oldLength = listPtr->nElements;
    Tcl_ListObjLength(listPtr->interp, listPtr->listObj, &listPtr->nElements);
    if (listPtr->nElements < oldLength) {
	for (i = listPtr->nElements; i < oldLength; i++) {
	    /*
	     * Clean up selection.
	     */

	    entry = Tcl_FindHashEntry(listPtr->selection, KEY(i));
	    if (entry != NULL) {
		listPtr->numSelected--;
		Tcl_DeleteHashEntry(entry);
	    }

	    /*
	     * Clean up attributes.
	     */

	    entry = Tcl_FindHashEntry(listPtr->itemAttrTable, KEY(i));
	    if (entry != NULL) {
		ckfree(Tcl_GetHashValue(entry));
		Tcl_DeleteHashEntry(entry);
	    }
	}
    }

    if (oldLength != listPtr->nElements) {
	listPtr->flags |= UPDATE_V_SCROLLBAR;
	if (listPtr->topIndex > (listPtr->nElements - listPtr->fullLines)) {
	    listPtr->topIndex = listPtr->nElements - listPtr->fullLines;
	    if (listPtr->topIndex < 0) {
		listPtr->topIndex = 0;
	    }
	}
    }

    /*
     * The computed maxWidth may have changed as a result of this operation.
     * However, we don't want to recompute it every time this trace fires
     * (imagine the user doing 1000 lappends to the listvar). Therefore, set
     * the MAXWIDTH_IS_STALE flag, which will cause the width to be recomputed
     * next time the list is redrawn.
     */

    listPtr->flags |= MAXWIDTH_IS_STALE;

    EventuallyRedrawRange(listPtr, 0, listPtr->nElements-1);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * MigrateHashEntries --
 *
 *	Given a hash table with entries keyed by a single integer value, move
 *	all entries in a given range by a fixed amount, so that if in the
 *	original table there was an entry with key n and the offset was i, in
 *	the new table that entry would have key n + i.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Rekeys some hash table entries.
 *
 *----------------------------------------------------------------------
 */

static void
MigrateHashEntries(
    Tcl_HashTable *table,
    int first,
    int last,
    int offset)
{
    int i, isNew;
    Tcl_HashEntry *entry;
    ClientData clientData;

    if (offset == 0) {
	return;
    }

    /*
     * It's more efficient to do one if/else and nest the for loops inside,
     * although we could avoid some code duplication if we nested the if/else
     * inside the for loops.
     */

    if (offset > 0) {
	for (i = last; i >= first; i--) {
	    entry = Tcl_FindHashEntry(table, KEY(i));
	    if (entry != NULL) {
		clientData = Tcl_GetHashValue(entry);
		Tcl_DeleteHashEntry(entry);
		entry = Tcl_CreateHashEntry(table, KEY(i + offset), &isNew);
		Tcl_SetHashValue(entry, clientData);
	    }
	}
    } else {
	for (i = first; i <= last; i++) {
	    entry = Tcl_FindHashEntry(table, KEY(i));
	    if (entry != NULL) {
		clientData = Tcl_GetHashValue(entry);
		Tcl_DeleteHashEntry(entry);
		entry = Tcl_CreateHashEntry(table, KEY(i + offset), &isNew);
		Tcl_SetHashValue(entry, clientData);
	    }
	}
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMaxOffset --
 *
 *	Passing in a listbox pointer, returns the maximum offset for the box,
 *	i.e. the maximum possible horizontal scrolling value (in pixels).
 *
 * Results:
 *	Listbox's maxOffset.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
*/
static int GetMaxOffset(
    register Listbox *listPtr)
{
    int maxOffset;

    maxOffset = listPtr->maxWidth -
            (Tk_Width(listPtr->tkwin) - 2*listPtr->inset -
            2*listPtr->selBorderWidth) + listPtr->xScrollUnit - 1;
    if (maxOffset < 0) {

        /*
         * Listbox is larger in width than its largest width item.
         */

        maxOffset = 0;
    }
    maxOffset -= maxOffset % listPtr->xScrollUnit;

    return maxOffset;
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
