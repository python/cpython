/*
 * tkText.c --
 *
 *	This module provides a big chunk of the implementation of multi-line
 *	editable text widgets for Tk. Among other things, it provides the Tcl
 *	command interfaces to text widgets. The B-tree representation of text
 *	and its actual display are implemented elsewhere.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"
#include "tkUndo.h"

#if defined(MAC_OSX_TK)
#define Style TkStyle
#define DInfo TkDInfo
#endif

/*
 * For compatibility with Tk 4.0 through 8.4.x, we allow tabs to be
 * mis-specified with non-increasing values. These are converted into tabs
 * which are the equivalent of at least a character width apart.
 */

#if (TK_MAJOR_VERSION < 9)
#define _TK_ALLOW_DECREASING_TABS
#endif

#include "tkText.h"

/*
 * Used to avoid having to allocate and deallocate arrays on the fly for
 * commonly used functions. Must be > 0.
 */

#define PIXEL_CLIENTS 5

/*
 * The 'TkTextState' enum in tkText.h is used to define a type for the -state
 * option of the Text widget. These values are used as indices into the string
 * table below.
 */

static const char *const stateStrings[] = {
    "disabled", "normal", NULL
};

/*
 * The 'TkWrapMode' enum in tkText.h is used to define a type for the -wrap
 * option of the Text widget. These values are used as indices into the string
 * table below.
 */

static const char *const wrapStrings[] = {
    "char", "none", "word", NULL
};

/*
 * The 'TkTextTabStyle' enum in tkText.h is used to define a type for the
 * -tabstyle option of the Text widget. These values are used as indices into
 * the string table below.
 */

static const char *const tabStyleStrings[] = {
    "tabular", "wordprocessor", NULL
};

/*
 * The 'TkTextInsertUnfocussed' enum in tkText.h is used to define a type for
 * the -insertunfocussed option of the Text widget. These values are used as
 * indice into the string table below.
 */

static const char *const insertUnfocussedStrings[] = {
    "hollow", "none", "solid", NULL
};

/*
 * The following functions and custom option type are used to define the
 * "line" option type, and thereby handle the text widget '-startline',
 * '-endline' configuration options which are of that type.
 *
 * We do not need a 'freeProc' because all changes to these two options are
 * handled through the TK_TEXT_LINE_RANGE flag in the optionSpecs list, and
 * the internal storage is just a pointer, which therefore doesn't need
 * freeing.
 */

static int		SetLineStartEnd(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin,
			    Tcl_Obj **value, char *recordPtr,
			    int internalOffset, char *oldInternalPtr,
			    int flags);
static Tcl_Obj *	GetLineStartEnd(ClientData clientData,
			    Tk_Window tkwin, char *recordPtr,
			    int internalOffset);
static void		RestoreLineStartEnd(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr,
			    char *oldInternalPtr);
static int		ObjectIsEmpty(Tcl_Obj *objPtr);

static const Tk_ObjCustomOption lineOption = {
    "line",			/* name */
    SetLineStartEnd,		/* setProc */
    GetLineStartEnd,		/* getProc */
    RestoreLineStartEnd,	/* restoreProc */
    NULL,			/* freeProc */
    0
};

/*
 * Information used to parse text configuration options:
 */

static const Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BOOLEAN, "-autoseparators", "autoSeparators",
	"AutoSeparators", DEF_TEXT_AUTO_SEPARATORS, -1,
	Tk_Offset(TkText, autoSeparators),
	TK_OPTION_DONT_SET_DEFAULT, 0, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_TEXT_BG_COLOR, -1, Tk_Offset(TkText, border),
	0, DEF_TEXT_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	NULL, 0, -1, 0, "-borderwidth",
	TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
	NULL, 0, -1, 0, "-background", 0},
    {TK_OPTION_BOOLEAN, "-blockcursor", "blockCursor",
	"BlockCursor", DEF_TEXT_BLOCK_CURSOR, -1,
	Tk_Offset(TkText, insertCursorType), 0, 0, 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_TEXT_BORDER_WIDTH, -1, Tk_Offset(TkText, borderWidth),
	0, 0, TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_TEXT_CURSOR, -1, Tk_Offset(TkText, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CUSTOM, "-endline", NULL, NULL,
	 NULL, -1, Tk_Offset(TkText, end), TK_OPTION_NULL_OK,
	 &lineOption, TK_TEXT_LINE_RANGE},
    {TK_OPTION_BOOLEAN, "-exportselection", "exportSelection",
	"ExportSelection", DEF_TEXT_EXPORT_SELECTION, -1,
	Tk_Offset(TkText, exportSelection), 0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", "foreground", NULL,
	NULL, 0, -1, 0, "-foreground", 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_TEXT_FONT, -1, Tk_Offset(TkText, tkfont), 0, 0,
	TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_TEXT_FG, -1, Tk_Offset(TkText, fgColor), 0,
	0, 0},
    {TK_OPTION_PIXELS, "-height", "height", "Height",
	DEF_TEXT_HEIGHT, -1, Tk_Offset(TkText, height), 0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_TEXT_HIGHLIGHT_BG,
	-1, Tk_Offset(TkText, highlightBgColorPtr),
	0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_TEXT_HIGHLIGHT, -1, Tk_Offset(TkText, highlightColorPtr),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_TEXT_HIGHLIGHT_WIDTH, -1,
	Tk_Offset(TkText, highlightWidth), 0, 0, TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_BORDER, "-inactiveselectbackground","inactiveSelectBackground",
	"Foreground",
	DEF_TEXT_INACTIVE_SELECT_COLOR,
	-1, Tk_Offset(TkText, inactiveSelBorder),
	TK_OPTION_NULL_OK, DEF_TEXT_SELECT_MONO, 0},
    {TK_OPTION_BORDER, "-insertbackground", "insertBackground", "Foreground",
	DEF_TEXT_INSERT_BG,
	-1, Tk_Offset(TkText, insertBorder),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-insertborderwidth", "insertBorderWidth",
	"BorderWidth", DEF_TEXT_INSERT_BD_COLOR, -1,
	Tk_Offset(TkText, insertBorderWidth), 0,
	(ClientData) DEF_TEXT_INSERT_BD_MONO, 0},
    {TK_OPTION_INT, "-insertofftime", "insertOffTime", "OffTime",
	DEF_TEXT_INSERT_OFF_TIME, -1, Tk_Offset(TkText, insertOffTime),
	0, 0, 0},
    {TK_OPTION_INT, "-insertontime", "insertOnTime", "OnTime",
	DEF_TEXT_INSERT_ON_TIME, -1, Tk_Offset(TkText, insertOnTime),
	0, 0, 0},
    {TK_OPTION_STRING_TABLE,
	"-insertunfocussed", "insertUnfocussed", "InsertUnfocussed",
	DEF_TEXT_INSERT_UNFOCUSSED, -1, Tk_Offset(TkText, insertUnfocussed),
	0, insertUnfocussedStrings, 0},
    {TK_OPTION_PIXELS, "-insertwidth", "insertWidth", "InsertWidth",
	DEF_TEXT_INSERT_WIDTH, -1, Tk_Offset(TkText, insertWidth),
	0, 0, 0},
    {TK_OPTION_INT, "-maxundo", "maxUndo", "MaxUndo",
	DEF_TEXT_MAX_UNDO, -1, Tk_Offset(TkText, maxUndo),
	TK_OPTION_DONT_SET_DEFAULT, 0, 0},
    {TK_OPTION_PIXELS, "-padx", "padX", "Pad",
	DEF_TEXT_PADX, -1, Tk_Offset(TkText, padX), 0, 0,
	TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_PIXELS, "-pady", "padY", "Pad",
	DEF_TEXT_PADY, -1, Tk_Offset(TkText, padY), 0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_TEXT_RELIEF, -1, Tk_Offset(TkText, relief), 0, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_TEXT_SELECT_COLOR, -1, Tk_Offset(TkText, selBorder),
	0, DEF_TEXT_SELECT_MONO, 0},
    {TK_OPTION_PIXELS, "-selectborderwidth", "selectBorderWidth",
	"BorderWidth", DEF_TEXT_SELECT_BD_COLOR,
	Tk_Offset(TkText, selBorderWidthPtr),
	Tk_Offset(TkText, selBorderWidth),
	TK_OPTION_NULL_OK, DEF_TEXT_SELECT_BD_MONO, 0},
    {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_TEXT_SELECT_FG_COLOR, -1, Tk_Offset(TkText, selFgColorPtr),
	TK_OPTION_NULL_OK, DEF_TEXT_SELECT_FG_MONO, 0},
    {TK_OPTION_BOOLEAN, "-setgrid", "setGrid", "SetGrid",
	DEF_TEXT_SET_GRID, -1, Tk_Offset(TkText, setGrid), 0, 0, 0},
    {TK_OPTION_PIXELS, "-spacing1", "spacing1", "Spacing",
	DEF_TEXT_SPACING1, -1, Tk_Offset(TkText, spacing1),
	0, 0 , TK_TEXT_LINE_GEOMETRY },
    {TK_OPTION_PIXELS, "-spacing2", "spacing2", "Spacing",
	DEF_TEXT_SPACING2, -1, Tk_Offset(TkText, spacing2),
	0, 0 , TK_TEXT_LINE_GEOMETRY },
    {TK_OPTION_PIXELS, "-spacing3", "spacing3", "Spacing",
	DEF_TEXT_SPACING3, -1, Tk_Offset(TkText, spacing3),
	0, 0 , TK_TEXT_LINE_GEOMETRY },
    {TK_OPTION_CUSTOM, "-startline", NULL, NULL,
	 NULL, -1, Tk_Offset(TkText, start), TK_OPTION_NULL_OK,
	 &lineOption, TK_TEXT_LINE_RANGE},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
	DEF_TEXT_STATE, -1, Tk_Offset(TkText, state),
	0, stateStrings, 0},
    {TK_OPTION_STRING, "-tabs", "tabs", "Tabs",
	DEF_TEXT_TABS, Tk_Offset(TkText, tabOptionPtr), -1,
	TK_OPTION_NULL_OK, 0, TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_STRING_TABLE, "-tabstyle", "tabStyle", "TabStyle",
	DEF_TEXT_TABSTYLE, -1, Tk_Offset(TkText, tabStyle),
	0, tabStyleStrings, TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TEXT_TAKE_FOCUS, -1, Tk_Offset(TkText, takeFocus),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-undo", "undo", "Undo",
	DEF_TEXT_UNDO, -1, Tk_Offset(TkText, undo),
	TK_OPTION_DONT_SET_DEFAULT, 0 , 0},
    {TK_OPTION_INT, "-width", "width", "Width",
	DEF_TEXT_WIDTH, -1, Tk_Offset(TkText, width), 0, 0,
	TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_STRING_TABLE, "-wrap", "wrap", "Wrap",
	DEF_TEXT_WRAP, -1, Tk_Offset(TkText, wrapMode),
	0, wrapStrings, TK_TEXT_LINE_GEOMETRY},
    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	DEF_TEXT_XSCROLL_COMMAND, -1, Tk_Offset(TkText, xScrollCmd),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	DEF_TEXT_YSCROLL_COMMAND, -1, Tk_Offset(TkText, yScrollCmd),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0}
};

/*
 * These three typedefs, the structure and the SearchPerform, SearchCore
 * functions below are used for line-based searches of the text widget, and,
 * in particular, to handle multi-line matching even though the text widget is
 * a single-line based data structure. They are completely abstracted away
 * from the Text widget internals, however, so could easily be re-used with
 * any line-based entity to provide multi-line matching.
 *
 * We have abstracted this code away from the text widget to try to keep Tk as
 * modular as possible.
 */

struct SearchSpec;	/* Forward declaration. */

typedef ClientData	SearchAddLineProc(int lineNum,
			    struct SearchSpec *searchSpecPtr,
			    Tcl_Obj *theLine, int *lenPtr,
			    int *extraLinesPtr);
typedef int		SearchMatchProc(int lineNum,
			    struct SearchSpec *searchSpecPtr,
			    ClientData clientData, Tcl_Obj *theLine,
			    int matchOffset, int matchLength);
typedef int		SearchLineIndexProc(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, struct SearchSpec *searchSpecPtr,
			    int *linePosPtr, int *offsetPosPtr);

typedef struct SearchSpec {
    int exact;			/* Whether search is exact or regexp. */
    int noCase;			/* Case-insenstivive? */
    int noLineStop;		/* If not set, a regexp search will use the
				 * TCL_REG_NLSTOP flag. */
    int overlap;		/* If set, results from multiple searches
				 * (-all) are allowed to overlap each
				 * other. */
    int strictLimits;		/* If set, matches must be completely inside
				 * the from,to range. Otherwise the limits
				 * only apply to the start of each match. */
    int all;			/* Whether all or the first match should be
				 * reported. */
    int startLine;		/* First line to examine. */
    int startOffset;		/* Index in first line to start at. */
    int stopLine;		/* Last line to examine, or -1 when we search
				 * all available text. */
    int stopOffset;		/* Index to stop at, provided stopLine is not
				 * -1. */
    int numLines;		/* Total lines which are available. */
    int backwards;		/* Searching forwards or backwards. */
    Tcl_Obj *varPtr;		/* If non-NULL, store length(s) of match(es)
				 * in this variable. */
    Tcl_Obj *countPtr;		/* Keeps track of currently found lengths. */
    Tcl_Obj *resPtr;		/* Keeps track of currently found locations */
    int searchElide;		/* Search in hidden text as well. */
    SearchAddLineProc *addLineProc;
				/* Function to call when we need to add
				 * another line to the search string so far */
    SearchMatchProc *foundMatchProc;
				/* Function to call when we have found a
				 * match. */
    SearchLineIndexProc *lineIndexProc;
				/* Function to call when we have found a
				 * match. */
    ClientData clientData;	/* Information about structure being searched,
				 * in this case a text widget. */
} SearchSpec;

/*
 * The text-widget-independent functions which actually perform the search,
 * handling both regexp and exact searches.
 */

static int		SearchCore(Tcl_Interp *interp,
			    SearchSpec *searchSpecPtr, Tcl_Obj *patObj);
static int		SearchPerform(Tcl_Interp *interp,
			    SearchSpec *searchSpecPtr, Tcl_Obj *patObj,
			    Tcl_Obj *fromPtr, Tcl_Obj *toPtr);

/*
 * Boolean variable indicating whether or not special debugging code should be
 * executed.
 */

int tkTextDebug = 0;

/*
 * Forward declarations for functions defined later in this file:
 */

static int		ConfigureText(Tcl_Interp *interp,
			    TkText *textPtr, int objc, Tcl_Obj *const objv[]);
static int		DeleteIndexRange(TkSharedText *sharedPtr,
			    TkText *textPtr, const TkTextIndex *indexPtr1,
			    const TkTextIndex *indexPtr2, int viewUpdate);
static int		CountIndices(const TkText *textPtr,
			    const TkTextIndex *indexPtr1,
			    const TkTextIndex *indexPtr2,
			    TkTextCountType type);
static void		DestroyText(TkText *textPtr);
static int		InsertChars(TkSharedText *sharedTextPtr,
			    TkText *textPtr, TkTextIndex *indexPtr,
			    Tcl_Obj *stringPtr, int viewUpdate);
static void		TextBlinkProc(ClientData clientData);
static void		TextCmdDeletedProc(ClientData clientData);
static int		CreateWidget(TkSharedText *sharedPtr, Tk_Window tkwin,
			    Tcl_Interp *interp, const TkText *parent,
			    int objc, Tcl_Obj *const objv[]);
static void		TextEventProc(ClientData clientData,
			    XEvent *eventPtr);
static int		TextFetchSelection(ClientData clientData, int offset,
			    char *buffer, int maxBytes);
static int		TextIndexSortProc(const void *first,
			    const void *second);
static int		TextInsertCmd(TkSharedText *sharedTextPtr,
			    TkText *textPtr, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[],
			    const TkTextIndex *indexPtr, int viewUpdate);
static int		TextReplaceCmd(TkText *textPtr, Tcl_Interp *interp,
			    const TkTextIndex *indexFromPtr,
			    const TkTextIndex *indexToPtr,
			    int objc, Tcl_Obj *const objv[], int viewUpdate);
static int		TextSearchCmd(TkText *textPtr, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		TextEditCmd(TkText *textPtr, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		TextWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		SharedTextObjCmd(ClientData clientData,
			    Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static void		TextWorldChangedCallback(ClientData instanceData);
static void		TextWorldChanged(TkText *textPtr, int mask);
static int		TextDumpCmd(TkText *textPtr, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		DumpLine(Tcl_Interp *interp, TkText *textPtr,
			    int what, TkTextLine *linePtr, int start, int end,
			    int lineno, Tcl_Obj *command);
static int		DumpSegment(TkText *textPtr, Tcl_Interp *interp,
			    const char *key, const char *value,
			    Tcl_Obj *command, const TkTextIndex *index,
			    int what);
static int		TextEditUndo(TkText *textPtr);
static int		TextEditRedo(TkText *textPtr);
static Tcl_Obj *	TextGetText(const TkText *textPtr,
			    const TkTextIndex *index1,
			    const TkTextIndex *index2, int visibleOnly);
static void		GenerateModifiedEvent(TkText *textPtr);
static void		GenerateUndoStackEvent(TkText *textPtr);
static void		UpdateDirtyFlag(TkSharedText *sharedPtr);
static void		TextPushUndoAction(TkText *textPtr,
			    Tcl_Obj *undoString, int insert,
			    const TkTextIndex *index1Ptr,
			    const TkTextIndex *index2Ptr);
static int		TextSearchIndexInLine(const SearchSpec *searchSpecPtr,
			    TkTextLine *linePtr, int byteIndex);
static int		TextPeerCmd(TkText *textPtr, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static TkUndoProc	TextUndoRedoCallback;

/*
 * Declarations of the three search procs required by the multi-line search
 * routines.
 */

static SearchMatchProc		TextSearchFoundMatch;
static SearchAddLineProc	TextSearchAddNextLine;
static SearchLineIndexProc	TextSearchGetLineIndex;

/*
 * The structure below defines text class behavior by means of functions that
 * can be invoked from generic window code.
 */

static const Tk_ClassProcs textClass = {
    sizeof(Tk_ClassProcs),	/* size */
    TextWorldChangedCallback,	/* worldChangedProc */
    NULL,					/* createProc */
    NULL					/* modalProc */
};

/*
 *--------------------------------------------------------------
 *
 * Tk_TextObjCmd --
 *
 *	This function is invoked to process the "text" Tcl command. See the
 *	user documentation for details on what it does.
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
Tk_TextObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?-option value ...?");
	return TCL_ERROR;
    }

    return CreateWidget(NULL, tkwin, interp, NULL, objc, objv);
}

/*
 *--------------------------------------------------------------
 *
 * CreateWidget --
 *
 *	This function is invoked to process the "text" Tcl command, (when
 *	called by Tk_TextObjCmd) and the "$text peer create" text widget
 *	sub-command (called from TextPeerCmd).
 *
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result, places the name of the widget created into the
 *	interp's result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
CreateWidget(
    TkSharedText *sharedPtr,	/* Shared widget info, or NULL. */
    Tk_Window tkwin,		/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    const TkText *parent,	/* If non-NULL then take default start, end
				 * from this parent. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register TkText *textPtr;
    Tk_OptionTable optionTable;
    TkTextIndex startIndex;
    Tk_Window newWin;

    /*
     * Create the window.
     */

    newWin = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(objv[1]),
	    NULL);
    if (newWin == NULL) {
	return TCL_ERROR;
    }

    /*
     * Create the text widget and initialize everything to zero, then set the
     * necessary initial (non-NULL) values. It is important that the 'set' tag
     * and 'insert', 'current' mark pointers are all NULL to start.
     */

    textPtr = ckalloc(sizeof(TkText));
    memset(textPtr, 0, sizeof(TkText));

    textPtr->tkwin = newWin;
    textPtr->display = Tk_Display(newWin);
    textPtr->interp = interp;
    textPtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(textPtr->tkwin), TextWidgetObjCmd,
	    textPtr, TextCmdDeletedProc);

    if (sharedPtr == NULL) {
	sharedPtr = ckalloc(sizeof(TkSharedText));
	memset(sharedPtr, 0, sizeof(TkSharedText));

	sharedPtr->refCount = 0;
	sharedPtr->peers = NULL;
	sharedPtr->tree = TkBTreeCreate(sharedPtr);

	Tcl_InitHashTable(&sharedPtr->tagTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&sharedPtr->markTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&sharedPtr->windowTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&sharedPtr->imageTable, TCL_STRING_KEYS);
	sharedPtr->undoStack = TkUndoInitStack(interp,0);
	sharedPtr->undo = 0;
	sharedPtr->isDirty = 0;
	sharedPtr->dirtyMode = TK_TEXT_DIRTY_NORMAL;
	sharedPtr->autoSeparators = 1;
	sharedPtr->lastEditMode = TK_TEXT_EDIT_OTHER;
	sharedPtr->stateEpoch = 0;
    }

    /*
     * Add the new widget to the shared list.
     */

    textPtr->sharedTextPtr = sharedPtr;
    sharedPtr->refCount++;
    textPtr->next = sharedPtr->peers;
    sharedPtr->peers = textPtr;

    /*
     * This refCount will be held until DestroyText is called. Note also that
     * the later call to 'TkTextCreateDInfo' will add more refCounts.
     */

    textPtr->refCount = 1;

    /*
     * Specify start and end lines in the B-tree. The default is the same as
     * the parent, but this can be adjusted to display more or less if the
     * start, end where given as configuration options.
     */

    if (parent != NULL) {
	textPtr->start = parent->start;
	textPtr->end = parent->end;
    } else {
	textPtr->start = NULL;
	textPtr->end = NULL;
    }

    textPtr->state = TK_TEXT_STATE_NORMAL;
    textPtr->relief = TK_RELIEF_FLAT;
    textPtr->cursor = NULL;
    textPtr->charWidth = 1;
    textPtr->charHeight = 10;
    textPtr->wrapMode = TEXT_WRAPMODE_CHAR;
    textPtr->prevWidth = Tk_Width(newWin);
    textPtr->prevHeight = Tk_Height(newWin);

    /*
     * Register with the B-tree. In some sense it would be best if we could do
     * this later (after configuration options), so that any changes to
     * start,end do not require a total recalculation.
     */

    TkBTreeAddClient(sharedPtr->tree, textPtr, textPtr->charHeight);

    /*
     * This will add refCounts to textPtr.
     */

    TkTextCreateDInfo(textPtr);
    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0,
	    &startIndex);
    TkTextSetYView(textPtr, &startIndex, 0);
    textPtr->exportSelection = 1;
    textPtr->pickEvent.type = LeaveNotify;
    textPtr->undo = textPtr->sharedTextPtr->undo;
    textPtr->maxUndo = textPtr->sharedTextPtr->maxUndo;
    textPtr->autoSeparators = textPtr->sharedTextPtr->autoSeparators;
    textPtr->tabOptionPtr = NULL;

    /*
     * Create the "sel" tag and the "current" and "insert" marks.
     */

    textPtr->selBorder = NULL;
    textPtr->inactiveSelBorder = NULL;
    textPtr->selBorderWidth = 0;
    textPtr->selBorderWidthPtr = NULL;
    textPtr->selFgColorPtr = NULL;

    /*
     * Note: it is important that textPtr->selTagPtr is NULL before this
     * initial call.
     */

    textPtr->selTagPtr = TkTextCreateTag(textPtr, "sel", NULL);
    textPtr->selTagPtr->reliefString =
	    ckalloc(sizeof(DEF_TEXT_SELECT_RELIEF));
    strcpy(textPtr->selTagPtr->reliefString, DEF_TEXT_SELECT_RELIEF);
    Tk_GetRelief(interp, DEF_TEXT_SELECT_RELIEF, &textPtr->selTagPtr->relief);
    textPtr->currentMarkPtr = TkTextSetMark(textPtr, "current", &startIndex);
    textPtr->insertMarkPtr = TkTextSetMark(textPtr, "insert", &startIndex);

    /*
     * Create the option table for this widget class. If it has already been
     * created, the cached pointer will be returned.
     */

    optionTable = Tk_CreateOptionTable(interp, optionSpecs);

    Tk_SetClass(textPtr->tkwin, "Text");
    Tk_SetClassProcs(textPtr->tkwin, &textClass, textPtr);
    textPtr->optionTable = optionTable;

    Tk_CreateEventHandler(textPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    TextEventProc, textPtr);
    Tk_CreateEventHandler(textPtr->tkwin, KeyPressMask|KeyReleaseMask
	    |ButtonPressMask|ButtonReleaseMask|EnterWindowMask
	    |LeaveWindowMask|PointerMotionMask|VirtualEventMask,
	    TkTextBindProc, textPtr);
    Tk_CreateSelHandler(textPtr->tkwin, XA_PRIMARY, XA_STRING,
	    TextFetchSelection, textPtr, XA_STRING);

    if (Tk_InitOptions(interp, (char *) textPtr, optionTable, textPtr->tkwin)
	    != TCL_OK) {
	Tk_DestroyWindow(textPtr->tkwin);
	return TCL_ERROR;
    }
    if (ConfigureText(interp, textPtr, objc-2, objv+2) != TCL_OK) {
	Tk_DestroyWindow(textPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, TkNewWindowObj(textPtr->tkwin));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TextWidgetObjCmd --
 *
 *	This function is invoked to process the Tcl command that corresponds
 *	to a text widget. See the user documentation for details on what it
 *	does.
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
TextWidgetObjCmd(
    ClientData clientData,	/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register TkText *textPtr = clientData;
    int result = TCL_OK;
    int index;

    static const char *const optionStrings[] = {
	"bbox", "cget", "compare", "configure", "count", "debug", "delete",
	"dlineinfo", "dump", "edit", "get", "image", "index", "insert",
	"mark", "peer", "pendingsync", "replace", "scan", "search",
	"see", "sync", "tag", "window", "xview", "yview", NULL
    };
    enum options {
	TEXT_BBOX, TEXT_CGET, TEXT_COMPARE, TEXT_CONFIGURE, TEXT_COUNT,
	TEXT_DEBUG, TEXT_DELETE, TEXT_DLINEINFO, TEXT_DUMP, TEXT_EDIT,
	TEXT_GET, TEXT_IMAGE, TEXT_INDEX, TEXT_INSERT, TEXT_MARK,
	TEXT_PEER, TEXT_PENDINGSYNC, TEXT_REPLACE, TEXT_SCAN,
	TEXT_SEARCH, TEXT_SEE, TEXT_SYNC, TEXT_TAG, TEXT_WINDOW,
	TEXT_XVIEW, TEXT_YVIEW
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], optionStrings,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    textPtr->refCount++;

    switch ((enum options) index) {
    case TEXT_BBOX: {
	int x, y, width, height;
	const TkTextIndex *indexPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index");
	    result = TCL_ERROR;
	    goto done;
	}
	indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[2]);
	if (indexPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextIndexBbox(textPtr, indexPtr, &x, &y, &width, &height,
		NULL) == 0) {
	    Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);

	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(x));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(y));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(width));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(height));

	    Tcl_SetObjResult(interp, listObj);
	}
	break;
    }
    case TEXT_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	    goto done;
	} else {
	    Tcl_Obj *objPtr = Tk_GetOptionValue(interp, (char *) textPtr,
		    textPtr->optionTable, objv[2], textPtr->tkwin);

	    if (objPtr == NULL) {
		result = TCL_ERROR;
		goto done;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    result = TCL_OK;
	}
	break;
    case TEXT_COMPARE: {
	int relation, value;
	const char *p;
	const TkTextIndex *index1Ptr, *index2Ptr;

	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index1 op index2");
	    result = TCL_ERROR;
	    goto done;
	}
	index1Ptr = TkTextGetIndexFromObj(interp, textPtr, objv[2]);
	index2Ptr = TkTextGetIndexFromObj(interp, textPtr, objv[4]);
	if (index1Ptr == NULL || index2Ptr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	relation = TkTextIndexCmp(index1Ptr, index2Ptr);
	p = Tcl_GetString(objv[3]);
	if (p[0] == '<') {
	    value = (relation < 0);
	    if ((p[1] == '=') && (p[2] == 0)) {
		value = (relation <= 0);
	    } else if (p[1] != 0) {
		goto compareError;
	    }
	} else if (p[0] == '>') {
	    value = (relation > 0);
	    if ((p[1] == '=') && (p[2] == 0)) {
		value = (relation >= 0);
	    } else if (p[1] != 0) {
		goto compareError;
	    }
	} else if ((p[0] == '=') && (p[1] == '=') && (p[2] == 0)) {
	    value = (relation == 0);
	} else if ((p[0] == '!') && (p[1] == '=') && (p[2] == 0)) {
	    value = (relation != 0);
	} else {
	    goto compareError;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(value));
	break;

    compareError:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad comparison operator \"%s\": must be"
		" <, <=, ==, >=, >, or !=", Tcl_GetString(objv[3])));
	Tcl_SetErrorCode(interp, "TK", "VALUE", "COMPARISON", NULL);
	result = TCL_ERROR;
	goto done;
    }
    case TEXT_CONFIGURE:
	if (objc <= 3) {
	    Tcl_Obj *objPtr = Tk_GetOptionInfo(interp, (char *) textPtr,
		    textPtr->optionTable, ((objc == 3) ? objv[2] : NULL),
		    textPtr->tkwin);

	    if (objPtr == NULL) {
		result = TCL_ERROR;
		goto done;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	} else {
	    result = ConfigureText(interp, textPtr, objc-2, objv+2);
	}
	break;
    case TEXT_COUNT: {
	const TkTextIndex *indexFromPtr, *indexToPtr;
	int i, found = 0, update = 0;
	Tcl_Obj *objPtr = NULL;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "?-option value ...? index1 index2");
	    result = TCL_ERROR;
	    goto done;
	}

	indexFromPtr = TkTextGetIndexFromObj(interp, textPtr, objv[objc-2]);
	if (indexFromPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	indexToPtr = TkTextGetIndexFromObj(interp, textPtr, objv[objc-1]);
	if (indexToPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}

	for (i = 2; i < objc-2; i++) {
	    int value;
	    size_t length;
	    const char *option = Tcl_GetString(objv[i]);
	    char c;

	    length = objv[i]->length;
	    if (length < 2 || option[0] != '-') {
		goto badOption;
	    }
	    c = option[1];
	    if (c == 'c' && !strncmp("-chars", option, length)) {
		value = CountIndices(textPtr, indexFromPtr, indexToPtr,
			COUNT_CHARS);
	    } else if (c == 'd' && (length > 8)
		    && !strncmp("-displaychars", option, length)) {
		value = CountIndices(textPtr, indexFromPtr, indexToPtr,
			COUNT_DISPLAY_CHARS);
	    } else if (c == 'd' && (length > 8)
		    && !strncmp("-displayindices", option,length)) {
		value = CountIndices(textPtr, indexFromPtr, indexToPtr,
			COUNT_DISPLAY_INDICES);
	    } else if (c == 'd' && (length > 8)
		    && !strncmp("-displaylines", option, length)) {
		TkTextLine *fromPtr, *lastPtr;
		TkTextIndex index, index2;

		int compare = TkTextIndexCmp(indexFromPtr, indexToPtr);
		value = 0;

		if (compare == 0) {
		    goto countDone;
		}

		if (compare > 0) {
		    const TkTextIndex *tmpPtr = indexFromPtr;

		    indexFromPtr = indexToPtr;
		    indexToPtr = tmpPtr;
		}

		lastPtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree,
			textPtr,
			TkBTreeNumLines(textPtr->sharedTextPtr->tree,textPtr));
		fromPtr = indexFromPtr->linePtr;
		if (fromPtr == lastPtr) {
		    goto countDone;
		}

		/*
		 * Caution: we must NEVER call TkTextUpdateOneLine with the
		 * last artificial line in the widget.
		 */

		index = *indexFromPtr;
		index.byteIndex = 0;

		/*
		 * We're going to count up all display lines in the logical
		 * line of 'indexFromPtr' up to, but not including the logical
		 * line of 'indexToPtr' (except if this line is elided), and
		 * then subtract off what came in too much from elided lines,
		 * also subtract off what we didn't want from 'from' and add
		 * on what we didn't count from 'to'.
		 */

		while (TkTextIndexCmp(&index,indexToPtr) < 0) {
		    value += TkTextUpdateOneLine(textPtr, index.linePtr,
			    0, &index, 0);
		}

		index2 = index;

		/*
		 * Now we need to adjust the count to:
		 *   - subtract off the number of display lines between
		 *     indexToPtr and index2, since we might have skipped past
		 *     indexToPtr, if we have several logical lines in a
		 *     single display line
		 *   - subtract off the number of display lines overcounted
		 *     in the first logical line
		 *   - add on the number of display lines in the last logical
		 *     line
		 * This logic is still ok if both indexFromPtr and indexToPtr
		 * are in the same logical line.
		 */

		index = *indexToPtr;
		index.byteIndex = 0;
		while (TkTextIndexCmp(&index,&index2) < 0) {
		    value -= TkTextUpdateOneLine(textPtr, index.linePtr,
			    0, &index, 0);
		}
		index.linePtr = indexFromPtr->linePtr;
		index.byteIndex = 0;
		while (1) {
		    TkTextFindDisplayLineEnd(textPtr, &index, 1, NULL);
		    if (TkTextIndexCmp(&index,indexFromPtr) >= 0) {
			break;
		    }
		    TkTextIndexForwBytes(textPtr, &index, 1, &index);
		    value--;

		}
		if (indexToPtr->linePtr != lastPtr) {
		    index.linePtr = indexToPtr->linePtr;
		    index.byteIndex = 0;
		    while (1) {
			TkTextFindDisplayLineEnd(textPtr, &index, 1, NULL);
			if (TkTextIndexCmp(&index,indexToPtr) >= 0) {
			    break;
			}
			TkTextIndexForwBytes(textPtr, &index, 1, &index);
			value++;
		    }
		}

		if (compare > 0) {
		    value = -value;
		}
	    } else if (c == 'i'
		    && !strncmp("-indices", option, length)) {
		value = CountIndices(textPtr, indexFromPtr, indexToPtr,
			COUNT_INDICES);
	    } else if (c == 'l'
		    && !strncmp("-lines", option, length)) {
		value = TkBTreeLinesTo(textPtr, indexToPtr->linePtr)
			- TkBTreeLinesTo(textPtr, indexFromPtr->linePtr);
	    } else if (c == 'u'
		    && !strncmp("-update", option, length)) {
		update = 1;
		continue;
	    } else if (c == 'x'
		    && !strncmp("-xpixels", option, length)) {
		int x1, x2;
		TkTextIndex index;

		index = *indexFromPtr;
		TkTextFindDisplayLineEnd(textPtr, &index, 0, &x1);
		index = *indexToPtr;
		TkTextFindDisplayLineEnd(textPtr, &index, 0, &x2);
		value = x2 - x1;
	    } else if (c == 'y'
		    && !strncmp("-ypixels", option, length)) {
		if (update) {
		    TkTextUpdateLineMetrics(textPtr,
			    TkBTreeLinesTo(textPtr, indexFromPtr->linePtr),
			    TkBTreeLinesTo(textPtr, indexToPtr->linePtr), -1);
		}
		value = TkTextIndexYPixels(textPtr, indexToPtr)
			- TkTextIndexYPixels(textPtr, indexFromPtr);
	    } else {
		goto badOption;
	    }

	countDone:
	    found++;
	    if (found == 1) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(value));
	    } else {
		if (found == 2) {
		    /*
		     * Move the first item we put into the result into the
		     * first element of the list object.
		     */

		    objPtr = Tcl_NewObj();
		    Tcl_ListObjAppendElement(NULL, objPtr,
			    Tcl_GetObjResult(interp));
		}
		Tcl_ListObjAppendElement(NULL, objPtr, Tcl_NewIntObj(value));
	    }
	}

	if (found == 0) {
	    /*
	     * Use the default '-indices'.
	     */

	    int value = CountIndices(textPtr, indexFromPtr, indexToPtr,
		    COUNT_INDICES);

	    Tcl_SetObjResult(interp, Tcl_NewIntObj(value));
	} else if (found > 1) {
	    Tcl_SetObjResult(interp, objPtr);
	}
	break;

    badOption:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad option \"%s\" must be -chars, -displaychars, "
		"-displayindices, -displaylines, -indices, -lines, -update, "
		"-xpixels, or -ypixels", Tcl_GetString(objv[i])));
	Tcl_SetErrorCode(interp, "TK", "TEXT", "INDEX_OPTION", NULL);
	result = TCL_ERROR;
	goto done;
    }
    case TEXT_DEBUG:
	if (objc > 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "boolean");
	    result = TCL_ERROR;
	    goto done;
	}
	if (objc == 2) {
	    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(tkBTreeDebug));
	} else {
	    if (Tcl_GetBooleanFromObj(interp, objv[2],
		    &tkBTreeDebug) != TCL_OK) {
		result = TCL_ERROR;
		goto done;
	    }
	    tkTextDebug = tkBTreeDebug;
	}
	break;
    case TEXT_DELETE:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index1 ?index2 ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	if (textPtr->state == TK_TEXT_STATE_NORMAL) {
	    if (objc < 5) {
		/*
		 * Simple case requires no predetermination of indices.
		 */

		const TkTextIndex *indexPtr1, *indexPtr2;

		/*
		 * Parse the starting and stopping indices.
		 */

		indexPtr1 = TkTextGetIndexFromObj(textPtr->interp, textPtr,
			objv[2]);
		if (indexPtr1 == NULL) {
		    result = TCL_ERROR;
		    goto done;
		}
		if (objc == 4) {
		    indexPtr2 = TkTextGetIndexFromObj(textPtr->interp,
			    textPtr, objv[3]);
		    if (indexPtr2 == NULL) {
			result = TCL_ERROR;
			goto done;
		    }
		} else {
		    indexPtr2 = NULL;
		}
		DeleteIndexRange(NULL, textPtr, indexPtr1, indexPtr2, 1);
	    } else {
		/*
		 * Multi-index pair case requires that we prevalidate the
		 * indices and sort from last to first so that deletes occur
		 * in the exact (unshifted) text. It also needs to handle
		 * partial and fully overlapping ranges. We have to do this
		 * with multiple passes.
		 */

		TkTextIndex *indices, *ixStart, *ixEnd, *lastStart;
		char *useIdx;
		int i;

		objc -= 2;
		objv += 2;
		indices = ckalloc((objc + 1) * sizeof(TkTextIndex));

		/*
		 * First pass verifies that all indices are valid.
		 */

		for (i = 0; i < objc; i++) {
		    const TkTextIndex *indexPtr =
			    TkTextGetIndexFromObj(interp, textPtr, objv[i]);

		    if (indexPtr == NULL) {
			result = TCL_ERROR;
			ckfree(indices);
			goto done;
		    }
		    indices[i] = *indexPtr;
		}

		/*
		 * Pad out the pairs evenly to make later code easier.
		 */

		if (objc & 1) {
		    indices[i] = indices[i-1];
		    TkTextIndexForwChars(NULL, &indices[i], 1, &indices[i],
			    COUNT_INDICES);
		    objc++;
		}
		useIdx = ckalloc(objc);
		memset(useIdx, 0, (size_t) objc);

		/*
		 * Do a decreasing order sort so that we delete the end ranges
		 * first to maintain index consistency.
		 */

		qsort(indices, (size_t) objc / 2,
			2 * sizeof(TkTextIndex), TextIndexSortProc);
		lastStart = NULL;

		/*
		 * Second pass will handle bogus ranges (end < start) and
		 * overlapping ranges.
		 */

		for (i = 0; i < objc; i += 2) {
		    ixStart = &indices[i];
		    ixEnd = &indices[i+1];
		    if (TkTextIndexCmp(ixEnd, ixStart) <= 0) {
			continue;
		    }
		    if (lastStart) {
			if (TkTextIndexCmp(ixStart, lastStart) == 0) {
			    /*
			     * Start indices were equal, and the sort placed
			     * the longest range first, so skip this one.
			     */

			    continue;
			} else if (TkTextIndexCmp(lastStart, ixEnd) < 0) {
			    /*
			     * The next pair has a start range before the end
			     * point of the last range. Constrain the delete
			     * range, but use the pointer values.
			     */

			    *ixEnd = *lastStart;
			    if (TkTextIndexCmp(ixEnd, ixStart) <= 0) {
				continue;
			    }
			}
		    }
		    lastStart = ixStart;
		    useIdx[i] = 1;
		}

		/*
		 * Final pass take the input from the previous and deletes the
		 * ranges which are flagged to be deleted.
		 */

		for (i = 0; i < objc; i += 2) {
		    if (useIdx[i]) {
			/*
			 * We don't need to check the return value because all
			 * indices are preparsed above.
			 */

			DeleteIndexRange(NULL, textPtr, &indices[i],
				&indices[i+1], 1);
		    }
		}
		ckfree(indices);
	    }
	}
	break;
    case TEXT_DLINEINFO: {
	int x, y, width, height, base;
	const TkTextIndex *indexPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index");
	    result = TCL_ERROR;
	    goto done;
	}
	indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[2]);
	if (indexPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextDLineInfo(textPtr, indexPtr, &x, &y, &width, &height,
		&base) == 0) {
	    Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);

	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(x));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(y));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(width));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(height));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(base));

	    Tcl_SetObjResult(interp, listObj);
	}
	break;
    }
    case TEXT_DUMP:
	result = TextDumpCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_EDIT:
	result = TextEditCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_GET: {
	Tcl_Obj *objPtr = NULL;
	int i, found = 0, visible = 0;
	const char *name;
	size_t length;

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "?-displaychars? ?--? index1 ?index2 ...?");
	    result = TCL_ERROR;
	    goto done;
	}

	/*
	 * Simple, restrictive argument parsing. The only options are -- and
	 * -displaychars (or any unique prefix).
	 */

	i = 2;
	if (objc > 3) {
	    name = Tcl_GetString(objv[i]);
	    length = objv[i]->length;
	    if (length > 1 && name[0] == '-') {
		if (strncmp("-displaychars", name, length) == 0) {
		    i++;
		    visible = 1;
		    name = Tcl_GetString(objv[i]);
		    length = objv[i]->length;
		}
		if ((i < objc-1) && (length == 2) && !strcmp("--", name)) {
		    i++;
		}
	    }
	}

	for (; i < objc; i += 2) {
	    const TkTextIndex *index1Ptr, *index2Ptr;
	    TkTextIndex index2;

	    index1Ptr = TkTextGetIndexFromObj(interp, textPtr, objv[i]);
	    if (index1Ptr == NULL) {
		if (objPtr) {
		    Tcl_DecrRefCount(objPtr);
		}
		result = TCL_ERROR;
		goto done;
	    }

	    if (i+1 == objc) {
		TkTextIndexForwChars(NULL, index1Ptr, 1, &index2,
			COUNT_INDICES);
		index2Ptr = &index2;
	    } else {
		index2Ptr = TkTextGetIndexFromObj(interp, textPtr, objv[i+1]);
		if (index2Ptr == NULL) {
		    if (objPtr) {
			Tcl_DecrRefCount(objPtr);
		    }
		    result = TCL_ERROR;
		    goto done;
		}
	    }

	    if (TkTextIndexCmp(index1Ptr, index2Ptr) < 0) {
		/*
		 * We want to move the text we get from the window into the
		 * result, but since this could in principle be a megabyte or
		 * more, we want to do it efficiently!
		 */

		Tcl_Obj *get = TextGetText(textPtr, index1Ptr, index2Ptr,
			visible);

		found++;
		if (found == 1) {
		    Tcl_SetObjResult(interp, get);
		} else {
		    if (found == 2) {
			/*
			 * Move the first item we put into the result into the
			 * first element of the list object.
			 */

			objPtr = Tcl_NewObj();
			Tcl_ListObjAppendElement(NULL, objPtr,
				Tcl_GetObjResult(interp));
		    }
		    Tcl_ListObjAppendElement(NULL, objPtr, get);
		}
	    }
	}
	if (found > 1) {
	    Tcl_SetObjResult(interp, objPtr);
	}
	break;
    }
    case TEXT_IMAGE:
	result = TkTextImageCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_INDEX: {
	const TkTextIndex *indexPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index");
	    result = TCL_ERROR;
	    goto done;
	}

	indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[2]);
	if (indexPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	Tcl_SetObjResult(interp, TkTextNewIndexObj(textPtr, indexPtr));
	break;
    }
    case TEXT_INSERT: {
	const TkTextIndex *indexPtr;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "index chars ?tagList chars tagList ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[2]);
	if (indexPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (textPtr->state == TK_TEXT_STATE_NORMAL) {
	    result = TextInsertCmd(NULL, textPtr, interp, objc-3, objv+3,
		    indexPtr, 1);
	}
	break;
    }
    case TEXT_MARK:
	result = TkTextMarkCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_PEER:
	result = TextPeerCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_PENDINGSYNC: {
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	Tcl_SetObjResult(interp,
		Tcl_NewBooleanObj(TkTextPendingsync(textPtr)));
	break;
    }
    case TEXT_REPLACE: {
	const TkTextIndex *indexFromPtr, *indexToPtr;

	if (objc < 5) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "index1 index2 chars ?tagList chars tagList ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	indexFromPtr = TkTextGetIndexFromObj(interp, textPtr, objv[2]);
	if (indexFromPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	indexToPtr = TkTextGetIndexFromObj(interp, textPtr, objv[3]);
	if (indexToPtr == NULL) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (TkTextIndexCmp(indexFromPtr, indexToPtr) > 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "index \"%s\" before \"%s\" in the text",
		    Tcl_GetString(objv[3]), Tcl_GetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TK", "TEXT", "INDEX_ORDER", NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	if (textPtr->state == TK_TEXT_STATE_NORMAL) {
	    int lineNum, byteIndex;
	    TkTextIndex index;

	    /*
	     * The 'replace' operation is quite complex to do correctly,
	     * because we want a number of criteria to hold:
	     *
	     * 1.  The insertion point shouldn't move, unless it is within the
	     *	   deleted range. In this case it should end up after the new
	     *	   text.
	     *
	     * 2.  The window should not change the text it shows - should not
	     *	   scroll vertically - unless the result of the replace is
	     *	   that the insertion position which used to be on-screen is
	     *	   now off-screen.
	     */

	    byteIndex = textPtr->topIndex.byteIndex;
	    lineNum = TkBTreeLinesTo(textPtr, textPtr->topIndex.linePtr);

	    TkTextMarkSegToIndex(textPtr, textPtr->insertMarkPtr, &index);
	    if ((TkTextIndexCmp(indexFromPtr, &index) < 0)
		    && (TkTextIndexCmp(indexToPtr, &index) > 0)) {
		/*
		 * The insertion point is inside the range to be replaced, so
		 * we have to do some calculations to ensure it doesn't move
		 * unnecessarily.
		 */

		int deleteInsertOffset, insertLength, j, indexFromLine, indexFromByteOffset;

		insertLength = 0;
		for (j = 4; j < objc; j += 2) {
		    insertLength += Tcl_GetCharLength(objv[j]);
		}

		/*
		 * Calculate 'deleteInsertOffset' as an offset we will apply
		 * to the insertion point after this operation.
		 */

		deleteInsertOffset = CountIndices(textPtr, indexFromPtr,
			&index, COUNT_CHARS);
		if (deleteInsertOffset > insertLength) {
		    deleteInsertOffset = insertLength;
		}

                indexFromLine = TkBTreeLinesTo(textPtr, indexFromPtr->linePtr);
                indexFromByteOffset = indexFromPtr->byteIndex;

		result = TextReplaceCmd(textPtr, interp, indexFromPtr,
			indexToPtr, objc, objv, 0);

		if (result == TCL_OK) {
		    /*
		     * Move the insertion position to the correct place.
		     */

                    TkTextIndex indexTmp;

                    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, indexFromLine,
                            indexFromByteOffset, &indexTmp);
                    TkTextIndexForwChars(NULL, &indexTmp,
			    deleteInsertOffset, &index, COUNT_INDICES);
		    TkBTreeUnlinkSegment(textPtr->insertMarkPtr,
			    textPtr->insertMarkPtr->body.mark.linePtr);
		    TkBTreeLinkSegment(textPtr->insertMarkPtr, &index);
		}
	    } else {
		result = TextReplaceCmd(textPtr, interp, indexFromPtr,
			indexToPtr, objc, objv, 1);
	    }
	    if (result == TCL_OK) {
		/*
		 * Now ensure the top-line is in the right place.
		 */

		TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			lineNum, byteIndex, &index);
		TkTextSetYView(textPtr, &index, TK_TEXT_NOPIXELADJUST);
	    }
	}
	break;
    }
    case TEXT_SCAN:
	result = TkTextScanCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_SEARCH:
	result = TextSearchCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_SEE:
	result = TkTextSeeCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_SYNC: {
	if (objc == 4) {
	    Tcl_Obj *cmd = objv[3];
	    const char *option = Tcl_GetString(objv[2]);
	    if (strncmp(option, "-command", objv[2]->length)) {
		Tcl_AppendResult(interp, "wrong option \"", option, "\": should be \"-command\"", NULL);
		result = TCL_ERROR;
		goto done;
	    }
	    Tcl_IncrRefCount(cmd);
	    if (TkTextPendingsync(textPtr)) {
		if (textPtr->afterSyncCmd) {
		    Tcl_DecrRefCount(textPtr->afterSyncCmd);
		}
		textPtr->afterSyncCmd = cmd;
	    } else {
		textPtr->afterSyncCmd = cmd;
		Tcl_DoWhenIdle(TkTextRunAfterSyncCmd, (ClientData) textPtr);
	    }
	    break;
	} else if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-command command?");
	    result = TCL_ERROR;
	    goto done;
	}
	if (textPtr->afterSyncCmd) {
	    Tcl_DecrRefCount(textPtr->afterSyncCmd);
	}
	textPtr->afterSyncCmd = NULL;
	TkTextUpdateLineMetrics(textPtr, 0,
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr), -1);
	break;
    }
    case TEXT_TAG:
	result = TkTextTagCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_WINDOW:
	result = TkTextWindowCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_XVIEW:
	result = TkTextXviewCmd(textPtr, interp, objc, objv);
	break;
    case TEXT_YVIEW:
	result = TkTextYviewCmd(textPtr, interp, objc, objv);
	break;
    }

  done:
    if (textPtr->refCount-- <= 1) {
	ckfree(textPtr);
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * SharedTextObjCmd --
 *
 *	This function is invoked to process commands on the shared portion of
 *	a text widget. Currently it is not actually exported as a Tcl command,
 *	and is only used internally to process parts of undo/redo scripts.
 *	See the user documentation for 'text' for details on what it does -
 *	the only subcommands it currently supports are 'insert' and 'delete'.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation for "text".
 *
 *--------------------------------------------------------------
 */

static int
SharedTextObjCmd(
    ClientData clientData,	/* Information about shared test B-tree. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register TkSharedText *sharedPtr = clientData;
    int result = TCL_OK;
    int index;

    static const char *const optionStrings[] = {
	"delete", "insert", NULL
    };
    enum options {
	TEXT_DELETE, TEXT_INSERT
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], optionStrings,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case TEXT_DELETE:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "index1 ?index2 ...?");
	    return TCL_ERROR;
	}
	if (objc < 5) {
	    /*
	     * Simple case requires no predetermination of indices.
	     */

	    TkTextIndex index1;

	    /*
	     * Parse the starting and stopping indices.
	     */

	    result = TkTextSharedGetObjIndex(interp, sharedPtr, objv[2],
		    &index1);
	    if (result != TCL_OK) {
		return result;
	    }
	    if (objc == 4) {
		TkTextIndex index2;

		result = TkTextSharedGetObjIndex(interp, sharedPtr, objv[3],
			&index2);
		if (result != TCL_OK) {
		    return result;
		}
		DeleteIndexRange(sharedPtr, NULL, &index1, &index2, 1);
	    } else {
		DeleteIndexRange(sharedPtr, NULL, &index1, NULL, 1);
	    }
	    return TCL_OK;
	} else {
	    /* Too many arguments */
	    return TCL_ERROR;
	}
	break;
    case TEXT_INSERT: {
	TkTextIndex index1;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "index chars ?tagList chars tagList ...?");
	    return TCL_ERROR;
	}
	result = TkTextSharedGetObjIndex(interp, sharedPtr, objv[2],
		&index1);
	if (result != TCL_OK) {
	    return result;
	}
	return TextInsertCmd(sharedPtr, NULL, interp, objc-3, objv+3, &index1,
		1);
    }
    default:
	return TCL_OK;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TextPeerCmd --
 *
 *	This function is invoked to process the "text peer" Tcl command. See
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

static int
TextPeerCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = textPtr->tkwin;
    int index;

    static const char *const peerOptionStrings[] = {
	"create", "names", NULL
    };
    enum peerOptions {
	PEER_CREATE, PEER_NAMES
    };

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[2], peerOptionStrings,
	    sizeof(char *), "peer option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum peerOptions) index) {
    case PEER_CREATE:
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "pathName ?-option value ...?");
	    return TCL_ERROR;
	}
	return CreateWidget(textPtr->sharedTextPtr, tkwin, interp, textPtr,
		objc-2, objv+2);
    case PEER_NAMES: {
	TkText *tPtr = textPtr->sharedTextPtr->peers;
	Tcl_Obj *peersObj;

	if (objc > 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	peersObj = Tcl_NewObj();
	while (tPtr != NULL) {
	    if (tPtr != textPtr) {
		Tcl_ListObjAppendElement(NULL, peersObj,
			TkNewWindowObj(tPtr->tkwin));
	    }
	    tPtr = tPtr->next;
	}
	Tcl_SetObjResult(interp, peersObj);
    }
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TextReplaceCmd --
 *
 *	This function is invoked to process part of the "replace" widget
 *	command for text widgets.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *	If 'viewUpdate' is false, then textPtr->topIndex may no longer be a
 *	valid index after this function returns. The caller is responsible for
 *	ensuring a correct index is in place.
 *
 *----------------------------------------------------------------------
 */

static int
TextReplaceCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    const TkTextIndex *indexFromPtr,
				/* Index from which to replace. */
    const TkTextIndex *indexToPtr,
				/* Index to which to replace. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects. */
    int viewUpdate)		/* Update vertical view if set. */
{
    /*
     * Perform the deletion and insertion, but ensure no undo-separator is
     * placed between the two operations. Since we are using the helper
     * functions 'DeleteIndexRange' and 'TextInsertCmd' we have to pretend
     * that the autoSeparators setting is off, so that we don't get an
     * undo-separator between the delete and insert.
     */

    int origAutoSep = textPtr->sharedTextPtr->autoSeparators;
    int result, lineNumber;
    TkTextIndex indexTmp;

    if (textPtr->sharedTextPtr->undo) {
	textPtr->sharedTextPtr->autoSeparators = 0;
	if (origAutoSep &&
		textPtr->sharedTextPtr->lastEditMode!=TK_TEXT_EDIT_REPLACE) {
	    TkUndoInsertUndoSeparator(textPtr->sharedTextPtr->undoStack);
	}
    }

    /*
     * Must save and restore line in indexFromPtr based on line number; can't
     * keep the line itself as that might be eliminated/invalidated when
     * deleting the range. [Bug 1602537]
     */

    indexTmp = *indexFromPtr;
    lineNumber = TkBTreeLinesTo(textPtr, indexFromPtr->linePtr);
    DeleteIndexRange(NULL, textPtr, indexFromPtr, indexToPtr, viewUpdate);
    indexTmp.linePtr = TkBTreeFindLine(indexTmp.tree, textPtr, lineNumber);
    result = TextInsertCmd(NULL, textPtr, interp, objc-4, objv+4,
	    &indexTmp, viewUpdate);

    if (textPtr->sharedTextPtr->undo) {
	textPtr->sharedTextPtr->lastEditMode = TK_TEXT_EDIT_REPLACE;
	textPtr->sharedTextPtr->autoSeparators = origAutoSep;
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TextIndexSortProc --
 *
 *	This function is called by qsort when sorting an array of indices in
 *	*decreasing* order (last to first).
 *
 * Results:
 *	The return value is -1 if the first argument should be before the
 *	second element, 0 if it's equivalent, and 1 if it should be after the
 *	second element.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextIndexSortProc(
    const void *first,		/* Elements to be compared. */
    const void *second)
{
    TkTextIndex *pair1 = (TkTextIndex *) first;
    TkTextIndex *pair2 = (TkTextIndex *) second;
    int cmp = TkTextIndexCmp(&pair1[1], &pair2[1]);

    if (cmp == 0) {
	/*
	 * If the first indices were equal, we want the second index of the
	 * pair also to be the greater. Use pointer magic to access the second
	 * index pair.
	 */

	cmp = TkTextIndexCmp(&pair1[0], &pair2[0]);
    }
    if (cmp > 0) {
	return -1;
    } else if (cmp < 0) {
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyText --
 *
 *	This function is invoked when we receive a destroy event to clean up
 *	the internal structure of a text widget. We will free up most of the
 *	internal structure and delete the associated Tcl command. If there are
 *	no outstanding references to the widget, we also free up the textPtr
 *	itself.
 *
 *	The widget has already been flagged as deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Either everything or almost everything associated with the text is
 *	freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyText(
    TkText *textPtr)		/* Info about text widget. */
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    TkTextTag *tagPtr;
    TkSharedText *sharedTextPtr = textPtr->sharedTextPtr;

    /*
     * Free up all the stuff that requires special handling. We have already
     * called let Tk_FreeConfigOptions to handle all the standard
     * option-related stuff (and so none of that exists when we are called).
     * Special note: free up display-related information before deleting the
     * B-tree, since display-related stuff may refer to stuff in the B-tree.
     */

    TkTextFreeDInfo(textPtr);
    textPtr->dInfoPtr = NULL;

    /*
     * Remove ourselves from the peer list.
     */

    if (sharedTextPtr->peers == textPtr) {
	sharedTextPtr->peers = textPtr->next;
    } else {
	TkText *nextPtr = sharedTextPtr->peers;
	while (nextPtr != NULL) {
	    if (nextPtr->next == textPtr) {
		nextPtr->next = textPtr->next;
		break;
	    }
	    nextPtr = nextPtr->next;
	}
    }

    /*
     * Always clean up the widget-specific tags first. Common tags (i.e. most)
     * will only be cleaned up when the shared structure is cleaned up.
     *
     * We also need to clean up widget-specific marks ('insert', 'current'),
     * since otherwise marks will never disappear from the B-tree.
     */

    TkTextDeleteTag(textPtr, textPtr->selTagPtr);
    TkBTreeUnlinkSegment(textPtr->insertMarkPtr,
	    textPtr->insertMarkPtr->body.mark.linePtr);
    ckfree(textPtr->insertMarkPtr);
    TkBTreeUnlinkSegment(textPtr->currentMarkPtr,
	    textPtr->currentMarkPtr->body.mark.linePtr);
    ckfree(textPtr->currentMarkPtr);

    /*
     * Now we've cleaned up everything of relevance to us in the B-tree, so we
     * disassociate outselves from it.
     *
     * When the refCount reaches zero, it's time to clean up the shared
     * portion of the text widget.
     */

    if (sharedTextPtr->refCount-- > 1) {
	TkBTreeRemoveClient(sharedTextPtr->tree, textPtr);

	/*
	 * Free up any embedded windows which belong to this widget.
	 */

	for (hPtr = Tcl_FirstHashEntry(&sharedTextPtr->windowTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    TkTextEmbWindowClient *loop;
	    TkTextSegment *ewPtr = Tcl_GetHashValue(hPtr);

	    loop = ewPtr->body.ew.clients;
	    if (loop->textPtr == textPtr) {
		ewPtr->body.ew.clients = loop->next;
		TkTextWinFreeClient(hPtr, loop);
	    } else {
		TkTextEmbWindowClient *client = ewPtr->body.ew.clients;

		client = loop->next;
		while (client != NULL) {
		    if (client->textPtr == textPtr) {
			loop->next = client->next;
			TkTextWinFreeClient(hPtr, client);
			break;
		    } else {
			loop = loop->next;
		    }
		    client = loop->next;
		}
	    }
	}
    } else {
	/*
	 * No need to call 'TkBTreeRemoveClient' first, since this will do
	 * everything in one go, more quickly.
	 */

	TkBTreeDestroy(sharedTextPtr->tree);

	for (hPtr = Tcl_FirstHashEntry(&sharedTextPtr->tagTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    tagPtr = Tcl_GetHashValue(hPtr);

	    /*
	     * No need to use 'TkTextDeleteTag' since we've already removed
	     * the B-tree completely.
	     */

	    TkTextFreeTag(textPtr, tagPtr);
	}
	Tcl_DeleteHashTable(&sharedTextPtr->tagTable);
	for (hPtr = Tcl_FirstHashEntry(&sharedTextPtr->markTable, &search);
	     hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    ckfree(Tcl_GetHashValue(hPtr));
	}
	Tcl_DeleteHashTable(&sharedTextPtr->markTable);
	TkUndoFreeStack(sharedTextPtr->undoStack);

	Tcl_DeleteHashTable(&sharedTextPtr->windowTable);
	Tcl_DeleteHashTable(&sharedTextPtr->imageTable);

	if (sharedTextPtr->bindingTable != NULL) {
	    Tk_DeleteBindingTable(sharedTextPtr->bindingTable);
	}
	ckfree(sharedTextPtr);
    }

    if (textPtr->tabArrayPtr != NULL) {
	ckfree(textPtr->tabArrayPtr);
    }
    if (textPtr->insertBlinkHandler != NULL) {
	Tcl_DeleteTimerHandler(textPtr->insertBlinkHandler);
    }

    textPtr->tkwin = NULL;
    Tcl_DeleteCommandFromToken(textPtr->interp, textPtr->widgetCmd);
    if (textPtr->afterSyncCmd){
	Tcl_DecrRefCount(textPtr->afterSyncCmd);
	textPtr->afterSyncCmd = NULL;
    }
    if (textPtr->refCount-- <= 1) {
	ckfree(textPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureText --
 *
 *	This function is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a text widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for textPtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureText(
    Tcl_Interp *interp,		/* Used for error reporting. */
    register TkText *textPtr,	/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_SavedOptions savedOptions;
    int oldExport = (textPtr->exportSelection) && (!Tcl_IsSafe(textPtr->interp));
    int mask = 0;

    if (Tk_SetOptions(interp, (char *) textPtr, textPtr->optionTable,
	    objc, objv, textPtr->tkwin, &savedOptions, &mask) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Copy down shared flags.
     */

    textPtr->sharedTextPtr->undo = textPtr->undo;
    textPtr->sharedTextPtr->maxUndo = textPtr->maxUndo;
    textPtr->sharedTextPtr->autoSeparators = textPtr->autoSeparators;

    TkUndoSetMaxDepth(textPtr->sharedTextPtr->undoStack,
	    textPtr->sharedTextPtr->maxUndo);

    /*
     * A few other options also need special processing, such as parsing the
     * geometry and setting the background from a 3-D border.
     */

    Tk_SetBackgroundFromBorder(textPtr->tkwin, textPtr->border);

    if (mask & TK_TEXT_LINE_RANGE) {
	int start, end, current;
	TkTextIndex index1, index2, index3;

	/*
	 * Line start and/or end have been adjusted. We need to validate the
	 * first displayed line and arrange for re-layout.
	 */

	TkBTreeClientRangeChanged(textPtr, textPtr->charHeight);

	if (textPtr->start != NULL) {
	    start = TkBTreeLinesTo(NULL, textPtr->start);
	} else {
	    start = 0;
	}
	if (textPtr->end != NULL) {
	    end = TkBTreeLinesTo(NULL, textPtr->end);
	} else {
	    end = TkBTreeNumLines(textPtr->sharedTextPtr->tree, NULL);
	}
	if (start > end) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "-startline must be less than or equal to -endline", -1));
	    Tcl_SetErrorCode(interp, "TK", "TEXT", "INDEX_ORDER", NULL);
	    Tk_RestoreSavedOptions(&savedOptions);
	    return TCL_ERROR;
	}
	current = TkBTreeLinesTo(NULL, textPtr->topIndex.linePtr);
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, NULL, start, 0,
		    &index1);
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, NULL, end, 0,
		    &index2);
	if (current < start || current > end) {
	    TkTextSearch search;
	    TkTextIndex first, last;
	    int selChanged = 0;

	    TkTextSetYView(textPtr, &index1, 0);

	    /*
	     * We may need to adjust the selection. So we have to check
	     * whether the "sel" tag was applied to anything outside the
	     * current start,end.
	     */

	    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, NULL, 0, 0,
		    &first);
	    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, NULL,
		    TkBTreeNumLines(textPtr->sharedTextPtr->tree, NULL),
		    0, &last);
	    TkBTreeStartSearch(&first, &last, textPtr->selTagPtr, &search);
	    if (!TkBTreeCharTagged(&first, textPtr->selTagPtr)
		    && !TkBTreeNextTag(&search)) {
		/* Nothing tagged with "sel" */
	    } else {
		int line = TkBTreeLinesTo(NULL, search.curIndex.linePtr);

		if (line < start) {
		    selChanged = 1;
		} else {
		    TkTextLine *linePtr = search.curIndex.linePtr;

		    while (TkBTreeNextTag(&search)) {
			linePtr = search.curIndex.linePtr;
		    }
		    line = TkBTreeLinesTo(NULL, linePtr);
		    if (line >= end) {
			selChanged = 1;
		    }
		}
	    }
	    if (selChanged) {
		/*
		 * Send an event that the selection has changed, and abort any
		 * partial-selections in progress.
		 */

		TkTextSelectionEvent(textPtr);
		textPtr->abortSelections = 1;
	    }
	}

	/* Indices are potentially obsolete after changing -startline and/or
	 * -endline, therefore increase the epoch.
	 * Also, clamp the insert and current (unshared) marks to the new
	 * -startline/-endline range limits of the widget. All other (shared)
	 * marks are unchanged.
	 * The return value of TkTextMarkNameToIndex does not need to be
	 * checked: "insert" and "current" marks always exist, and the
	 * purpose of the code below precisely is to move them inside the
	 * -startline/-endline range.
	 */

	textPtr->sharedTextPtr->stateEpoch++;
	TkTextMarkNameToIndex(textPtr, "insert", &index3);
	if (TkTextIndexCmp(&index3, &index1) < 0) {
	    textPtr->insertMarkPtr = TkTextSetMark(textPtr, "insert", &index1);
	}
	if (TkTextIndexCmp(&index3, &index2) > 0) {
	    textPtr->insertMarkPtr = TkTextSetMark(textPtr, "insert", &index2);
	}
	TkTextMarkNameToIndex(textPtr, "current", &index3);
	if (TkTextIndexCmp(&index3, &index1) < 0) {
	    textPtr->currentMarkPtr = TkTextSetMark(textPtr, "current", &index1);
	}
	if (TkTextIndexCmp(&index3, &index2) > 0) {
	    textPtr->currentMarkPtr = TkTextSetMark(textPtr, "current", &index2);
	}
    }

    /*
     * Don't allow negative spacings.
     */

    if (textPtr->spacing1 < 0) {
	textPtr->spacing1 = 0;
    }
    if (textPtr->spacing2 < 0) {
	textPtr->spacing2 = 0;
    }
    if (textPtr->spacing3 < 0) {
	textPtr->spacing3 = 0;
    }

    /*
     * Parse tab stops.
     */

    if (textPtr->tabArrayPtr != NULL) {
	ckfree(textPtr->tabArrayPtr);
	textPtr->tabArrayPtr = NULL;
    }
    if (textPtr->tabOptionPtr != NULL) {
	textPtr->tabArrayPtr = TkTextGetTabs(interp, textPtr,
		textPtr->tabOptionPtr);
	if (textPtr->tabArrayPtr == NULL) {
	    Tcl_AddErrorInfo(interp,"\n    (while processing -tabs option)");
	    Tk_RestoreSavedOptions(&savedOptions);
	    return TCL_ERROR;
	}
    }

    /*
     * Make sure that configuration options are properly mirrored between the
     * widget record and the "sel" tags. NOTE: we don't have to free up
     * information during the mirroring; old information was freed when it was
     * replaced in the widget record.
     */

    if (textPtr->selTagPtr->selBorder == NULL) {
	textPtr->selTagPtr->border = textPtr->selBorder;
    } else {
	textPtr->selTagPtr->selBorder = textPtr->selBorder;
    }
    if (textPtr->selTagPtr->borderWidthPtr != textPtr->selBorderWidthPtr) {
	textPtr->selTagPtr->borderWidthPtr = textPtr->selBorderWidthPtr;
	textPtr->selTagPtr->borderWidth = textPtr->selBorderWidth;
    }
    if (textPtr->selTagPtr->selFgColor == NULL) {
	textPtr->selTagPtr->fgColor = textPtr->selFgColorPtr;
    } else {
	textPtr->selTagPtr->selFgColor = textPtr->selFgColorPtr;
    }
    textPtr->selTagPtr->affectsDisplay = 0;
    textPtr->selTagPtr->affectsDisplayGeometry = 0;
    if ((textPtr->selTagPtr->elideString != NULL)
	    || (textPtr->selTagPtr->tkfont != NULL)
	    || (textPtr->selTagPtr->justifyString != NULL)
	    || (textPtr->selTagPtr->lMargin1String != NULL)
	    || (textPtr->selTagPtr->lMargin2String != NULL)
	    || (textPtr->selTagPtr->offsetString != NULL)
	    || (textPtr->selTagPtr->rMarginString != NULL)
	    || (textPtr->selTagPtr->spacing1String != NULL)
	    || (textPtr->selTagPtr->spacing2String != NULL)
	    || (textPtr->selTagPtr->spacing3String != NULL)
	    || (textPtr->selTagPtr->tabStringPtr != NULL)
	    || (textPtr->selTagPtr->wrapMode != TEXT_WRAPMODE_NULL)) {
	textPtr->selTagPtr->affectsDisplay = 1;
	textPtr->selTagPtr->affectsDisplayGeometry = 1;
    }
    if ((textPtr->selTagPtr->border != NULL)
	    || (textPtr->selTagPtr->selBorder != NULL)
	    || (textPtr->selTagPtr->reliefString != NULL)
	    || (textPtr->selTagPtr->bgStipple != None)
	    || (textPtr->selTagPtr->fgColor != NULL)
	    || (textPtr->selTagPtr->selFgColor != NULL)
	    || (textPtr->selTagPtr->fgStipple != None)
	    || (textPtr->selTagPtr->overstrikeString != NULL)
	    || (textPtr->selTagPtr->overstrikeColor != NULL)
	    || (textPtr->selTagPtr->underlineString != NULL)
	    || (textPtr->selTagPtr->underlineColor != NULL)
	    || (textPtr->selTagPtr->lMarginColor != NULL)
	    || (textPtr->selTagPtr->rMarginColor != NULL)) {
	textPtr->selTagPtr->affectsDisplay = 1;
    }
    TkTextRedrawTag(NULL, textPtr, NULL, NULL, textPtr->selTagPtr, 1);

    /*
     * Claim the selection if we've suddenly started exporting it and there
     * are tagged characters.
     */

    if (textPtr->exportSelection && (!oldExport) && (!Tcl_IsSafe(textPtr->interp))) {
	TkTextSearch search;
	TkTextIndex first, last;

	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0,
		&first);
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr),
		0, &last);
	TkBTreeStartSearch(&first, &last, textPtr->selTagPtr, &search);
	if (TkBTreeCharTagged(&first, textPtr->selTagPtr)
		|| TkBTreeNextTag(&search)) {
	    Tk_OwnSelection(textPtr->tkwin, XA_PRIMARY, TkTextLostSelection,
		    textPtr);
	    textPtr->flags |= GOT_SELECTION;
	}
    }

    /*
     * Account for state changes that would reenable blinking cursor state.
     */

    if (textPtr->flags & GOT_FOCUS) {
	Tcl_DeleteTimerHandler(textPtr->insertBlinkHandler);
	textPtr->insertBlinkHandler = NULL;
	TextBlinkProc(textPtr);
    }

    /*
     * Register the desired geometry for the window, and arrange for the
     * window to be redisplayed.
     */

    if (textPtr->width <= 0) {
	textPtr->width = 1;
    }
    if (textPtr->height <= 0) {
	textPtr->height = 1;
    }
    Tk_FreeSavedOptions(&savedOptions);
    TextWorldChanged(textPtr, mask);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextWorldChangedCallback --
 *
 *	This function is called when the world has changed in some way and the
 *	widget needs to recompute all its graphics contexts and determine its
 *	new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Configures all tags in the Text with a empty objc/objv, for the side
 *	effect of causing all the items to recompute their geometry and to be
 *	redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
TextWorldChangedCallback(
    ClientData instanceData)	/* Information about widget. */
{
    TkText *textPtr = instanceData;

    TextWorldChanged(textPtr, TK_TEXT_LINE_GEOMETRY);
}

/*
 *---------------------------------------------------------------------------
 *
 * TextWorldChanged --
 *
 *	This function is called when the world has changed in some way and the
 *	widget needs to recompute all its graphics contexts and determine its
 *	new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Configures all tags in the Text with a empty objc/objv, for the side
 *	effect of causing all the items to recompute their geometry and to be
 *	redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
TextWorldChanged(
    TkText *textPtr,		/* Information about widget. */
    int mask)			/* OR'd collection of bits showing what has
				 * changed. */
{
    Tk_FontMetrics fm;
    int border;
    int oldCharHeight = textPtr->charHeight;

    textPtr->charWidth = Tk_TextWidth(textPtr->tkfont, "0", 1);
    if (textPtr->charWidth <= 0) {
	textPtr->charWidth = 1;
    }
    Tk_GetFontMetrics(textPtr->tkfont, &fm);

    textPtr->charHeight = fm.linespace;
    if (textPtr->charHeight <= 0) {
	textPtr->charHeight = 1;
    }
    if (textPtr->charHeight != oldCharHeight) {
	TkBTreeClientRangeChanged(textPtr, textPtr->charHeight);
    }
    border = textPtr->borderWidth + textPtr->highlightWidth;
    Tk_GeometryRequest(textPtr->tkwin,
	    textPtr->width * textPtr->charWidth + 2*textPtr->padX + 2*border,
	    textPtr->height*(fm.linespace+textPtr->spacing1+textPtr->spacing3)
		    + 2*textPtr->padY + 2*border);

    Tk_SetInternalBorderEx(textPtr->tkwin,
	    border + textPtr->padX, border + textPtr->padX,
	    border + textPtr->padY, border + textPtr->padY);
    if (textPtr->setGrid) {
	Tk_SetGrid(textPtr->tkwin, textPtr->width, textPtr->height,
		textPtr->charWidth, textPtr->charHeight);
    } else {
	Tk_UnsetGrid(textPtr->tkwin);
    }

    TkTextRelayoutWindow(textPtr, mask);
}

/*
 *--------------------------------------------------------------
 *
 * TextEventProc --
 *
 *	This function is invoked by the Tk dispatcher on structure changes to
 *	a text. For texts with 3D borders, this function is also invoked for
 *	exposures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up.
 *	When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
TextEventProc(
    ClientData clientData,	/* Information about window. */
    register XEvent *eventPtr)	/* Information about event. */
{
    register TkText *textPtr = clientData;
    TkTextIndex index, index2;

    if (eventPtr->type == Expose) {
	TkTextRedrawRegion(textPtr, eventPtr->xexpose.x,
		eventPtr->xexpose.y, eventPtr->xexpose.width,
		eventPtr->xexpose.height);
    } else if (eventPtr->type == ConfigureNotify) {
	if ((textPtr->prevWidth != Tk_Width(textPtr->tkwin))
		|| (textPtr->prevHeight != Tk_Height(textPtr->tkwin))) {
	    int mask = 0;

	    if (textPtr->prevWidth != Tk_Width(textPtr->tkwin)) {
		mask = TK_TEXT_LINE_GEOMETRY;
	    }
	    TkTextRelayoutWindow(textPtr, mask);
	    textPtr->prevWidth = Tk_Width(textPtr->tkwin);
	    textPtr->prevHeight = Tk_Height(textPtr->tkwin);
	}
    } else if (eventPtr->type == DestroyNotify) {
	/*
	 * NOTE: we must zero out selBorder, selBorderWidthPtr and
	 * selFgColorPtr: they are duplicates of information in the "sel" tag,
	 * which will be freed up when we delete all tags. Hence we don't want
	 * the automatic config options freeing process to delete them as
	 * well.
	 */

	textPtr->selBorder = NULL;
	textPtr->selBorderWidthPtr = NULL;
	textPtr->selBorderWidth = 0;
	textPtr->selFgColorPtr = NULL;
	if (textPtr->setGrid) {
	    Tk_UnsetGrid(textPtr->tkwin);
	    textPtr->setGrid = 0;
	}
	if (!(textPtr->flags & OPTIONS_FREED)) {
	    Tk_FreeConfigOptions((char *) textPtr, textPtr->optionTable,
		    textPtr->tkwin);
	    textPtr->flags |= OPTIONS_FREED;
	}
	textPtr->flags |= DESTROYED;

	/*
	 * Call 'DestroyTest' to handle the deletion for us. The actual
	 * textPtr may still exist after this, if there are some outstanding
	 * references. But we have flagged it as DESTROYED just above, so
	 * nothing will try to make use of it very extensively.
	 */

	DestroyText(textPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail == NotifyInferior
		|| eventPtr->xfocus.detail == NotifyAncestor
		|| eventPtr->xfocus.detail == NotifyNonlinear) {
	    Tcl_DeleteTimerHandler(textPtr->insertBlinkHandler);
	    if (eventPtr->type == FocusIn) {
		textPtr->flags |= GOT_FOCUS | INSERT_ON;
		if (textPtr->insertOffTime != 0) {
		    textPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
			    textPtr->insertOnTime, TextBlinkProc, textPtr);
		}
	    } else {
		textPtr->flags &= ~(GOT_FOCUS | INSERT_ON);
		textPtr->insertBlinkHandler = NULL;
	    }
	    if (textPtr->inactiveSelBorder != textPtr->selBorder) {
		TkTextRedrawTag(NULL, textPtr, NULL, NULL, textPtr->selTagPtr,
			1);
	    }
	    TkTextMarkSegToIndex(textPtr, textPtr->insertMarkPtr, &index);
	    TkTextIndexForwChars(NULL, &index, 1, &index2, COUNT_INDICES);

	    /*
	     * While we wish to redisplay, no heights have changed, so no need
	     * to call TkTextInvalidateLineMetrics.
	     */

	    TkTextChanged(NULL, textPtr, &index, &index2);
	    if (textPtr->highlightWidth > 0) {
		TkTextRedrawRegion(textPtr, 0, 0, textPtr->highlightWidth,
			textPtr->highlightWidth);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TextCmdDeletedProc --
 *
 *	This function is invoked when a widget command is deleted. If the
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
TextCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    TkText *textPtr = clientData;
    Tk_Window tkwin = textPtr->tkwin;

    /*
     * This function could be invoked either because the window was destroyed
     * and the command was then deleted (in which this flag is already set) or
     * because the command was deleted, and then this function destroys the
     * widget.
     */

    if (!(textPtr->flags & DESTROYED)) {
	if (textPtr->setGrid) {
	    Tk_UnsetGrid(textPtr->tkwin);
	    textPtr->setGrid = 0;
	}
	textPtr->flags |= DESTROYED;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InsertChars --
 *
 *	This function implements most of the functionality of the "insert"
 *	widget command.
 *
 * Results:
 *	The length of the inserted string.
 *
 * Side effects:
 *	The characters in "stringPtr" get added to the text just before the
 *	character indicated by "indexPtr".
 *
 *	If 'viewUpdate' is true, we may adjust the window contents'
 *	y-position, and scrollbar setting.
 *
 *----------------------------------------------------------------------
 */

static int
InsertChars(
    TkSharedText *sharedTextPtr,
    TkText *textPtr,		/* Overall information about text widget. */
    TkTextIndex *indexPtr,	/* Where to insert new characters. May be
				 * modified if the index is not valid for
				 * insertion (e.g. if at "end"). */
    Tcl_Obj *stringPtr,		/* Null-terminated string containing new
				 * information to add to text. */
    int viewUpdate)		/* Update the view if set. */
{
    int lineIndex;
    size_t length;
    TkText *tPtr;
    int *lineAndByteIndex;
    int resetViewCount;
    int pixels[2*PIXEL_CLIENTS];
    const char *string = Tcl_GetString(stringPtr);

    length = stringPtr->length;
    if (sharedTextPtr == NULL) {
	sharedTextPtr = textPtr->sharedTextPtr;
    }

    /*
     * Don't allow insertions on the last (dummy) line of the text. This is
     * the only place in this function where the indexPtr is modified.
     */

    lineIndex = TkBTreeLinesTo(textPtr, indexPtr->linePtr);
    if (lineIndex == TkBTreeNumLines(sharedTextPtr->tree, textPtr)) {
	lineIndex--;
	TkTextMakeByteIndex(sharedTextPtr->tree, textPtr, lineIndex, 1000000,
		indexPtr);
    }

    /*
     * Notify the display module that lines are about to change, then do the
     * insertion. If the insertion occurs on the top line of the widget
     * (textPtr->topIndex), then we have to recompute topIndex after the
     * insertion, since the insertion could invalidate it.
     */

    resetViewCount = 0;
    if (sharedTextPtr->refCount > PIXEL_CLIENTS) {
	lineAndByteIndex = ckalloc(sizeof(int) * 2 * sharedTextPtr->refCount);
    } else {
	lineAndByteIndex = pixels;
    }
    for (tPtr = sharedTextPtr->peers; tPtr != NULL ; tPtr = tPtr->next) {
	lineAndByteIndex[resetViewCount] = -1;
	if (indexPtr->linePtr == tPtr->topIndex.linePtr) {
	    lineAndByteIndex[resetViewCount] =
		    TkBTreeLinesTo(tPtr, indexPtr->linePtr);
	    lineAndByteIndex[resetViewCount+1] = tPtr->topIndex.byteIndex;
	    if (lineAndByteIndex[resetViewCount+1] > indexPtr->byteIndex) {
		lineAndByteIndex[resetViewCount+1] += length;
	    }
	}
	resetViewCount += 2;
    }

    TkTextChanged(sharedTextPtr, NULL, indexPtr, indexPtr);

    sharedTextPtr->stateEpoch++;

    TkBTreeInsertChars(sharedTextPtr->tree, indexPtr, string);

    /*
     * Push the insertion on the undo stack, and update the modified status of
     * the widget.
     */

    if (length > 0) {
	if (sharedTextPtr->undo) {
	    TkTextIndex toIndex;

	    if (sharedTextPtr->autoSeparators &&
		sharedTextPtr->lastEditMode != TK_TEXT_EDIT_INSERT) {
		TkUndoInsertUndoSeparator(sharedTextPtr->undoStack);
	    }

	    sharedTextPtr->lastEditMode = TK_TEXT_EDIT_INSERT;

	    TkTextIndexForwBytes(textPtr, indexPtr, length, &toIndex);
	    TextPushUndoAction(textPtr, stringPtr, 1, indexPtr, &toIndex);
	}

	UpdateDirtyFlag(sharedTextPtr);
    }

    resetViewCount = 0;
    for (tPtr = sharedTextPtr->peers; tPtr != NULL ; tPtr = tPtr->next) {
	if (lineAndByteIndex[resetViewCount] != -1) {
	    if ((tPtr != textPtr) || viewUpdate) {
		TkTextIndex newTop;

		TkTextMakeByteIndex(sharedTextPtr->tree, tPtr,
			lineAndByteIndex[resetViewCount], 0, &newTop);
		TkTextIndexForwBytes(tPtr, &newTop,
			lineAndByteIndex[resetViewCount+1], &newTop);
		TkTextSetYView(tPtr, &newTop, 0);
	    }
	}
	resetViewCount += 2;
    }
    if (sharedTextPtr->refCount > PIXEL_CLIENTS) {
	ckfree(lineAndByteIndex);
    }

    /*
     * Invalidate any selection retrievals in progress, and send an event
     * that the selection changed if that is the case.
     */

    for (tPtr = sharedTextPtr->peers; tPtr != NULL ; tPtr = tPtr->next) {
        if (TkBTreeCharTagged(indexPtr, tPtr->selTagPtr)) {
            TkTextSelectionEvent(tPtr);
        }
	tPtr->abortSelections = 1;
    }

    /*
     * For convenience, return the length of the string.
     */

    return length;
}

/*
 *----------------------------------------------------------------------
 *
 * TextPushUndoAction --
 *
 *	Shared by insert and delete actions. Stores the appropriate scripts
 *	into our undo stack. We will add a single refCount to the 'undoString'
 *	object, so, if it previously had a refCount of zero, the caller should
 *	not free it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Items pushed onto stack.
 *
 *----------------------------------------------------------------------
 */

static void
TextPushUndoAction(
    TkText *textPtr,		/* Overall information about text widget. */
    Tcl_Obj *undoString,	/* New text. */
    int insert,			/* 1 if insert, else delete. */
    const TkTextIndex *index1Ptr,
				/* Index describing first location. */
    const TkTextIndex *index2Ptr)
				/* Index describing second location. */
{
    TkUndoSubAtom *iAtom, *dAtom;
    int canUndo, canRedo;

    /*
     * Create the helpers.
     */

    Tcl_Obj *seeInsertObj = Tcl_NewObj();
    Tcl_Obj *markSet1InsertObj = Tcl_NewObj();
    Tcl_Obj *markSet2InsertObj = NULL;
    Tcl_Obj *insertCmdObj = Tcl_NewObj();
    Tcl_Obj *deleteCmdObj = Tcl_NewObj();

    /*
     * Get the index positions.
     */

    Tcl_Obj *index1Obj = TkTextNewIndexObj(NULL, index1Ptr);
    Tcl_Obj *index2Obj = TkTextNewIndexObj(NULL, index2Ptr);

    /*
     * These need refCounts, because they are used more than once below.
     */

    Tcl_IncrRefCount(seeInsertObj);
    Tcl_IncrRefCount(index1Obj);
    Tcl_IncrRefCount(index2Obj);

    Tcl_ListObjAppendElement(NULL, seeInsertObj,
	    Tcl_NewStringObj(Tk_PathName(textPtr->tkwin), -1));
    Tcl_ListObjAppendElement(NULL, seeInsertObj, Tcl_NewStringObj("see", 3));
    Tcl_ListObjAppendElement(NULL, seeInsertObj,
	    Tcl_NewStringObj("insert", 6));

    Tcl_ListObjAppendElement(NULL, markSet1InsertObj,
	    Tcl_NewStringObj(Tk_PathName(textPtr->tkwin), -1));
    Tcl_ListObjAppendElement(NULL, markSet1InsertObj,
	    Tcl_NewStringObj("mark", 4));
    Tcl_ListObjAppendElement(NULL, markSet1InsertObj,
	    Tcl_NewStringObj("set", 3));
    Tcl_ListObjAppendElement(NULL, markSet1InsertObj,
	    Tcl_NewStringObj("insert", 6));
    markSet2InsertObj = Tcl_DuplicateObj(markSet1InsertObj);
    Tcl_ListObjAppendElement(NULL, markSet1InsertObj, index1Obj);
    Tcl_ListObjAppendElement(NULL, markSet2InsertObj, index2Obj);

    Tcl_ListObjAppendElement(NULL, insertCmdObj,
	    Tcl_NewStringObj("insert", 6));
    Tcl_ListObjAppendElement(NULL, insertCmdObj, index1Obj);

    /*
     * Only use of 'undoString' is here.
     */

    Tcl_ListObjAppendElement(NULL, insertCmdObj, undoString);

    Tcl_ListObjAppendElement(NULL, deleteCmdObj,
	    Tcl_NewStringObj("delete", 6));
    Tcl_ListObjAppendElement(NULL, deleteCmdObj, index1Obj);
    Tcl_ListObjAppendElement(NULL, deleteCmdObj, index2Obj);

    /*
     * Note: we don't wish to use textPtr->widgetCmd in these callbacks
     * because if we delete the textPtr, but peers still exist, we will then
     * have references to a non-existent Tcl_Command in the undo stack, which
     * will lead to crashes later. Also, the behaviour of the widget w.r.t.
     * bindings (%W substitutions) always uses the widget path name, so there
     * is no good reason the undo stack should do otherwise.
     *
     * For the 'insert' and 'delete' actions, we have to register a functional
     * callback, because these actions are defined to operate on the
     * underlying data shared by all peers.
     */

    iAtom = TkUndoMakeSubAtom(&TextUndoRedoCallback, textPtr->sharedTextPtr,
	    insertCmdObj, NULL);
    TkUndoMakeCmdSubAtom(NULL, markSet2InsertObj, iAtom);
    TkUndoMakeCmdSubAtom(NULL, seeInsertObj, iAtom);

    dAtom = TkUndoMakeSubAtom(&TextUndoRedoCallback, textPtr->sharedTextPtr,
	    deleteCmdObj, NULL);
    TkUndoMakeCmdSubAtom(NULL, markSet1InsertObj, dAtom);
    TkUndoMakeCmdSubAtom(NULL, seeInsertObj, dAtom);

    Tcl_DecrRefCount(seeInsertObj);
    Tcl_DecrRefCount(index1Obj);
    Tcl_DecrRefCount(index2Obj);

    canUndo = TkUndoCanUndo(textPtr->sharedTextPtr->undoStack);
    canRedo = TkUndoCanRedo(textPtr->sharedTextPtr->undoStack);

    /*
     * Depending whether the action is to insert or delete, we provide the
     * appropriate second and third arguments to TkUndoPushAction. (The first
     * is the 'actionCommand', and the second the 'revertCommand').
     */

    if (insert) {
	TkUndoPushAction(textPtr->sharedTextPtr->undoStack, iAtom, dAtom);
    } else {
	TkUndoPushAction(textPtr->sharedTextPtr->undoStack, dAtom, iAtom);
    }

    if (!canUndo || canRedo) {
	GenerateUndoStackEvent(textPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TextUndoRedoCallback --
 *
 *	This function is registered with the generic undo/redo code to handle
 *	'insert' and 'delete' actions on all text widgets. We cannot perform
 *	those actions on any particular text widget, because that text widget
 *	might have been deleted by the time we get here.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Will insert or delete text, depending on the first word contained in
 *	objPtr.
 *
 *----------------------------------------------------------------------
 */

int
TextUndoRedoCallback(
    Tcl_Interp *interp,		/* Current interpreter. */
    ClientData clientData,	/* Passed from undo code, but contains our
				 * shared text data structure. */
    Tcl_Obj *objPtr)		/* Arguments of a command to be handled by the
				 * shared text data structure. */
{
    TkSharedText *sharedPtr = clientData;
    int res, objc;
    Tcl_Obj **objv;
    TkText *textPtr;

    res = Tcl_ListObjGetElements(interp, objPtr, &objc, &objv);
    if (res != TCL_OK) {
	return res;
    }

    /*
     * If possible, use a real text widget to perform the undo/redo action
     * (i.e. insertion or deletion of text). This provides maximum
     * compatibility with older versions of Tk, in which the user may rename
     * the text widget to allow capture of undo or redo actions.
     *
     * In particular, this sorting of capture is useful in text editors based
     * on the Tk text widget, which need to know which new text needs
     * re-coloring.
     *
     * It would be better if the text widget provided some other mechanism to
     * allow capture of this information ("What has just changed in the text
     * widget?"). What we have here is not entirely satisfactory under all
     * circumstances.
     */

    textPtr = sharedPtr->peers;
    while (textPtr != NULL) {
	if (textPtr->start == NULL && textPtr->end == NULL) {
	    Tcl_Obj *cmdNameObj, *evalObj;

	    evalObj = Tcl_NewObj();
	    Tcl_IncrRefCount(evalObj);

	    /*
	     * We might wish to use the real, current command-name for the
	     * widget, but this will break any code that has over-ridden the
	     * widget, and is expecting to observe the insert/delete actions
	     * which are caused by undo/redo operations.
	     *
	     * cmdNameObj = Tcl_NewObj();
	     * Tcl_GetCommandFullName(interp, textPtr->widgetCmd, cmdNameObj);
	     *
	     * While such interception is not explicitly documented as
	     * supported, it does occur, and so until we can provide some
	     * alternative mechanism for such code to do what it needs, we
	     * allow it to take place here.
	     */

	    cmdNameObj = Tcl_NewStringObj(Tk_PathName(textPtr->tkwin), -1);
	    Tcl_ListObjAppendElement(NULL, evalObj, cmdNameObj);
	    Tcl_ListObjAppendList(NULL, evalObj, objPtr);
	    res = Tcl_EvalObjEx(interp, evalObj, TCL_EVAL_GLOBAL);
	    Tcl_DecrRefCount(evalObj);
	    return res;
	}
	textPtr = textPtr->next;
    }

    /*
     * If there's no current text widget which shows everything, then we fall
     * back on acting directly. This means there is no way to intercept from
     * the Tcl level.
     */

    return SharedTextObjCmd(sharedPtr, interp, objc+1, objv-1);
}

/*
 *----------------------------------------------------------------------
 *
 * CountIndices --
 *
 *	This function implements most of the functionality of the "count"
 *	widget command.
 *
 *	Note that 'textPtr' is only used if we need to check for elided
 *	attributes, i.e. if type is COUNT_DISPLAY_INDICES or
 *	COUNT_DISPLAY_CHARS
 *
 * Results:
 *	Returns the number of characters in the range.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CountIndices(
    const TkText *textPtr,	/* Overall information about text widget. */
    const TkTextIndex *indexPtr1,
				/* Index describing location of first
				 * character to delete. */
    const TkTextIndex *indexPtr2,
				/* Index describing location of last character
				 * to delete. NULL means just delete the one
				 * character given by indexPtr1. */
    TkTextCountType type)	/* The kind of indices to count. */
{
    /*
     * Order the starting and stopping indices.
     */

    int compare = TkTextIndexCmp(indexPtr1, indexPtr2);

    if (compare == 0) {
	return 0;
    } else if (compare > 0) {
	return -TkTextIndexCount(textPtr, indexPtr2, indexPtr1, type);
    } else {
	return TkTextIndexCount(textPtr, indexPtr1, indexPtr2, type);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteIndexRange --
 *
 *	This function implements most of the functionality of the "delete"
 *	widget command.
 *
 * Results:
 *	Returns a standard Tcl result, currently always TCL_OK.
 *
 * Side effects:
 *	Characters and other entities (windows, images) get deleted from the
 *	text.
 *
 *	If 'viewUpdate' is true, we may adjust the window contents'
 *	y-position, and scrollbar setting.
 *
 *	If 'viewUpdate' is true we can guarantee that textPtr->topIndex
 *	points to a valid TkTextLine after this function returns. However, if
 *	'viewUpdate' is false, then there is no such guarantee (since
 *	topIndex.linePtr can be garbage). The caller is expected to take
 *	actions to ensure the topIndex is validated before laying out the
 *	window again.
 *
 *----------------------------------------------------------------------
 */

static int
DeleteIndexRange(
    TkSharedText *sharedTextPtr,/* Shared portion of peer widgets. */
    TkText *textPtr,		/* Overall information about text widget. */
    const TkTextIndex *indexPtr1,
				/* Index describing location of first
				 * character (or other entity) to delete. */
    const TkTextIndex *indexPtr2,
				/* Index describing location of last
				 * character (or other entity) to delete.
				 * NULL means just delete the one character
				 * given by indexPtr1. */
    int viewUpdate)		/* Update vertical view if set. */
{
    int line1, line2;
    TkTextIndex index1, index2;
    TkText *tPtr;
    int *lineAndByteIndex;
    int resetViewCount;
    int pixels[2*PIXEL_CLIENTS];
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    int i;

    if (sharedTextPtr == NULL) {
	sharedTextPtr = textPtr->sharedTextPtr;
    }

    /*
     * Prepare the starting and stopping indices.
     */

    index1 = *indexPtr1;
    if (indexPtr2 != NULL) {
	index2 = *indexPtr2;
    } else {
	index2 = index1;
	TkTextIndexForwChars(NULL, &index2, 1, &index2, COUNT_INDICES);
    }

    /*
     * Make sure there's really something to delete.
     */

    if (TkTextIndexCmp(&index1, &index2) >= 0) {
	return TCL_OK;
    }

    /*
     * The code below is ugly, but it's needed to make sure there is always a
     * dummy empty line at the end of the text. If the final newline of the
     * file (just before the dummy line) is being deleted, then back up index
     * to just before the newline. If there is a newline just before the first
     * character being deleted, then back up the first index too. The idea is
     * that a deletion involving a range starting at a line start and
     * including the final \n (i.e. index2 is "end") is an attempt to delete
     * complete lines, so the \n before the deleted block shall become the new
     * final \n. Furthermore, remove any tags that are present on the newline
     * that isn't going to be deleted after all (this simulates deleting the
     * newline and then adding a "clean" one back again). Note that index1 and
     * index2 might now be equal again which means that no text will be
     * deleted but tags might be removed.
     */

    line1 = TkBTreeLinesTo(textPtr, index1.linePtr);
    line2 = TkBTreeLinesTo(textPtr, index2.linePtr);
    if (line2 == TkBTreeNumLines(sharedTextPtr->tree, textPtr)) {
	TkTextTag **arrayPtr;
	int arraySize, i;
	TkTextIndex oldIndex2;

	oldIndex2 = index2;
	TkTextIndexBackChars(NULL, &oldIndex2, 1, &index2, COUNT_INDICES);
	line2--;
	if ((index1.byteIndex == 0) && (line1 != 0)) {
	    TkTextIndexBackChars(NULL, &index1, 1, &index1, COUNT_INDICES);
	    line1--;
	}
	arrayPtr = TkBTreeGetTags(&index2, NULL, &arraySize);
	if (arrayPtr != NULL) {
	    for (i = 0; i < arraySize; i++) {
		TkBTreeTag(&index2, &oldIndex2, arrayPtr[i], 0);
	    }
	    ckfree(arrayPtr);
	}
    }

    /*
     * For speed, we remove all tags from the range first. If we don't
     * do this, the code below can (when there are many tags) grow
     * non-linearly in execution time.
     */

    for (i=0, hPtr=Tcl_FirstHashEntry(&sharedTextPtr->tagTable, &search);
	    hPtr != NULL; i++, hPtr = Tcl_NextHashEntry(&search)) {
        TkTextTag *tagPtr = Tcl_GetHashValue(hPtr);

        TkBTreeTag(&index1, &index2, tagPtr, 0);
    }

    /*
     * Special case for the sel tag which is not in the hash table. We
     * need to do this once for each peer text widget.
     */

    for (tPtr = sharedTextPtr->peers; tPtr != NULL ;
	    tPtr = tPtr->next) {
        if (TkBTreeTag(&index1, &index2, tPtr->selTagPtr, 0)) {
	    /*
	     * Send an event that the selection changed. This is
	     * equivalent to:
	     *	event generate $textWidget <<Selection>>
	     */

	    TkTextSelectionEvent(textPtr);
	    tPtr->abortSelections = 1;
        }
    }

    /*
     * Tell the display what's about to happen so it can discard obsolete
     * display information, then do the deletion. Also, if the deletion
     * involves the top line on the screen, then we have to reset the view
     * (the deletion will invalidate textPtr->topIndex). Compute what the new
     * first character will be, then do the deletion, then reset the view.
     */

    TkTextChanged(sharedTextPtr, NULL, &index1, &index2);

    resetViewCount = 0;
    if (sharedTextPtr->refCount > PIXEL_CLIENTS) {
	lineAndByteIndex = ckalloc(sizeof(int) * 2 * sharedTextPtr->refCount);
    } else {
	lineAndByteIndex = pixels;
    }
    for (tPtr = sharedTextPtr->peers; tPtr != NULL ; tPtr = tPtr->next) {
	int line = 0;
	int byteIndex = 0;
	int resetView = 0;

	if (TkTextIndexCmp(&index2, &tPtr->topIndex) >= 0) {
	    if (TkTextIndexCmp(&index1, &tPtr->topIndex) <= 0) {
		/*
		 * Deletion range straddles topIndex: use the beginning of the
		 * range as the new topIndex.
		 */

		resetView = 1;
		line = line1;
		byteIndex = index1.byteIndex;
	    } else if (index1.linePtr == tPtr->topIndex.linePtr) {
		/*
		 * Deletion range starts on top line but after topIndex. Use
		 * the current topIndex as the new one.
		 */

		resetView = 1;
		line = line1;
		byteIndex = tPtr->topIndex.byteIndex;
	    } else {
		/*
		 * Deletion range starts after the top line. This peers's view
		 * will not need to be reset. Nothing to do.
		 */
	    }
	} else if (index2.linePtr == tPtr->topIndex.linePtr) {
	    /*
	     * Deletion range ends on top line but before topIndex. Figure out
	     * what will be the new character index for the character
	     * currently pointed to by topIndex.
	     */

	    resetView = 1;
	    line = line2;
	    byteIndex = tPtr->topIndex.byteIndex;
	    if (index1.linePtr != index2.linePtr) {
		byteIndex -= index2.byteIndex;
	    } else {
		byteIndex -= (index2.byteIndex - index1.byteIndex);
	    }
	} else {
	    /*
	     * Deletion range ends before the top line. This peers's view
	     * will not need to be reset. Nothing to do.
	     */
	}
	if (resetView) {
	    lineAndByteIndex[resetViewCount] = line;
	    lineAndByteIndex[resetViewCount+1] = byteIndex;
	} else {
	    lineAndByteIndex[resetViewCount] = -1;
	}
	resetViewCount += 2;
    }

    /*
     * Push the deletion on the undo stack if something was actually deleted.
     */

    if (TkTextIndexCmp(&index1, &index2) < 0) {
	if (sharedTextPtr->undo) {
	    Tcl_Obj *get;

	    if (sharedTextPtr->autoSeparators
		    && (sharedTextPtr->lastEditMode != TK_TEXT_EDIT_DELETE)) {
		TkUndoInsertUndoSeparator(sharedTextPtr->undoStack);
	    }

	    sharedTextPtr->lastEditMode = TK_TEXT_EDIT_DELETE;

	    get = TextGetText(textPtr, &index1, &index2, 0);
	    TextPushUndoAction(textPtr, get, 0, &index1, &index2);
	}
	sharedTextPtr->stateEpoch++;

	TkBTreeDeleteIndexRange(sharedTextPtr->tree, &index1, &index2);

    	UpdateDirtyFlag(sharedTextPtr);
    }

    resetViewCount = 0;
    for (tPtr = sharedTextPtr->peers; tPtr != NULL ; tPtr = tPtr->next) {
	int line = lineAndByteIndex[resetViewCount];

	if (line != -1) {
	    int byteIndex = lineAndByteIndex[resetViewCount+1];
	    TkTextIndex indexTmp;

	    if (tPtr == textPtr) {
		if (viewUpdate) {
		    /*
		     * line cannot be before -startline of textPtr because
		     * this line corresponds to an index which is necessarily
		     * between "1.0" and "end" relative to textPtr.
		     * Therefore no need to clamp line to the -start/-end
		     * range.
		     */

		    TkTextMakeByteIndex(sharedTextPtr->tree, textPtr, line,
			    byteIndex, &indexTmp);
		    TkTextSetYView(tPtr, &indexTmp, 0);
		}
	    } else {
		TkTextMakeByteIndex(sharedTextPtr->tree, tPtr, line,
			byteIndex, &indexTmp);
		/*
		 * line may be before -startline of tPtr and must be
		 * clamped to -startline before providing it to
		 * TkTextSetYView otherwise lines before -startline
		 * would be displayed.
		 * There is no need to worry about -endline however,
		 * because the view will only be reset if the deletion
		 * involves the TOP line of the screen
		 */

		if (tPtr->start != NULL) {
		    int start;
		    TkTextIndex indexStart;

		    start = TkBTreeLinesTo(NULL, tPtr->start);
		    TkTextMakeByteIndex(sharedTextPtr->tree, NULL, start,
			    0, &indexStart);
		    if (TkTextIndexCmp(&indexTmp, &indexStart) < 0) {
			indexTmp = indexStart;
		    }
		}
		TkTextSetYView(tPtr, &indexTmp, 0);
	    }
	}
	resetViewCount += 2;
    }
    if (sharedTextPtr->refCount > PIXEL_CLIENTS) {
	ckfree(lineAndByteIndex);
    }

    if (line1 >= line2) {
	/*
	 * Invalidate any selection retrievals in progress, assuming we didn't
	 * check for this case above.
	 */

	for (tPtr = sharedTextPtr->peers; tPtr != NULL ; tPtr = tPtr->next) {
	    tPtr->abortSelections = 1;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TextFetchSelection --
 *
 *	This function is called back by Tk when the selection is requested by
 *	someone. It returns part or all of the selection in a buffer provided
 *	by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored at buffer.
 *	Buffer is filled (or partially filled) with a NULL-terminated string
 *	containing part or all of the selection, as given by offset and
 *	maxBytes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextFetchSelection(
    ClientData clientData,	/* Information about text widget. */
    int offset,			/* Offset within selection of first character
				 * to be returned. */
    char *buffer,		/* Location in which to place selection. */
    int maxBytes)		/* Maximum number of bytes to place at buffer,
				 * not including terminating NULL
				 * character. */
{
    register TkText *textPtr = clientData;
    TkTextIndex eof;
    int count, chunkSize, offsetInSeg;
    TkTextSearch search;
    TkTextSegment *segPtr;

    if ((!textPtr->exportSelection) || Tcl_IsSafe(textPtr->interp)) {
	return -1;
    }

    /*
     * Find the beginning of the next range of selected text. Note: if the
     * selection is being retrieved in multiple pieces (offset != 0) and some
     * modification has been made to the text that affects the selection then
     * reject the selection request (make 'em start over again).
     */

    if (offset == 0) {
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0,
		&textPtr->selIndex);
	textPtr->abortSelections = 0;
    } else if (textPtr->abortSelections) {
	return 0;
    }
    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
	    TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr), 0, &eof);
    TkBTreeStartSearch(&textPtr->selIndex, &eof, textPtr->selTagPtr, &search);
    if (!TkBTreeCharTagged(&textPtr->selIndex, textPtr->selTagPtr)) {
	if (!TkBTreeNextTag(&search)) {
	    if (offset == 0) {
		return -1;
	    } else {
		return 0;
	    }
	}
	textPtr->selIndex = search.curIndex;
    }

    /*
     * Each iteration through the outer loop below scans one selected range.
     * Each iteration through the inner loop scans one segment in the selected
     * range.
     */

    count = 0;
    while (1) {
	/*
	 * Find the end of the current range of selected text.
	 */

	if (!TkBTreeNextTag(&search)) {
	    Tcl_Panic("TextFetchSelection couldn't find end of range");
	}

	/*
	 * Copy information from character segments into the buffer until
	 * either we run out of space in the buffer or we get to the end of
	 * this range of text.
	 */

	while (1) {
	    if (maxBytes == 0) {
		goto fetchDone;
	    }
	    segPtr = TkTextIndexToSeg(&textPtr->selIndex, &offsetInSeg);
	    chunkSize = segPtr->size - offsetInSeg;
	    if (chunkSize > maxBytes) {
		chunkSize = maxBytes;
	    }
	    if (textPtr->selIndex.linePtr == search.curIndex.linePtr) {
		int leftInRange;

		leftInRange = search.curIndex.byteIndex
			- textPtr->selIndex.byteIndex;
		if (leftInRange < chunkSize) {
		    chunkSize = leftInRange;
		    if (chunkSize <= 0) {
			break;
		    }
		}
	    }
	    if ((segPtr->typePtr == &tkTextCharType)
		    && !TkTextIsElided(textPtr, &textPtr->selIndex, NULL)) {
		memcpy(buffer, segPtr->body.chars + offsetInSeg,
			(size_t) chunkSize);
		buffer += chunkSize;
		maxBytes -= chunkSize;
		count += chunkSize;
	    }
	    TkTextIndexForwBytes(textPtr, &textPtr->selIndex, chunkSize,
		    &textPtr->selIndex);
	}

	/*
	 * Find the beginning of the next range of selected text.
	 */

	if (!TkBTreeNextTag(&search)) {
	    break;
	}
	textPtr->selIndex = search.curIndex;
    }

  fetchDone:
    *buffer = 0;
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextLostSelection --
 *
 *	This function is called back by Tk when the selection is grabbed away
 *	from a text widget. On Windows and Mac systems, we want to remember
 *	the selection for the next time the focus enters the window. On Unix,
 *	just remove the "sel" tag from everything in the widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The "sel" tag is cleared from the window.
 *
 *----------------------------------------------------------------------
 */

void
TkTextLostSelection(
    ClientData clientData)	/* Information about text widget. */
{
    register TkText *textPtr = clientData;

    if (TkpAlwaysShowSelection(textPtr->tkwin)) {
	TkTextIndex start, end;

	if ((!textPtr->exportSelection) || Tcl_IsSafe(textPtr->interp)) {
	    return;
	}

	/*
	 * On Windows and Mac systems, we want to remember the selection for
	 * the next time the focus enters the window. On Unix, just remove the
	 * "sel" tag from everything in the widget.
	 */

	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		0, 0, &start);
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr),
		0, &end);
	TkTextRedrawTag(NULL, textPtr, &start, &end, textPtr->selTagPtr, 1);
	TkBTreeTag(&start, &end, textPtr->selTagPtr, 0);
    }

    /*
     * Send an event that the selection changed. This is equivalent to:
     *	   event generate $textWidget <<Selection>>
     */

    TkTextSelectionEvent(textPtr);

    textPtr->flags &= ~GOT_SELECTION;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextSelectionEvent --
 *
 *	When anything relevant to the "sel" tag has been changed, call this
 *	function to generate a <<Selection>> event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If <<Selection>> bindings are present, they will trigger.
 *
 *----------------------------------------------------------------------
 */

void
TkTextSelectionEvent(
    TkText *textPtr)
{
    /*
     * Send an event that the selection changed. This is equivalent to:
     *     event generate $textWidget <<Selection>>
     */

    TkSendVirtualEvent(textPtr->tkwin, "Selection", NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * TextBlinkProc --
 *
 *	This function is called as a timer handler to blink the insertion
 *	cursor off and on.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off, redisplay gets invoked, and this
 *	function reschedules itself.
 *
 *----------------------------------------------------------------------
 */

static void
TextBlinkProc(
    ClientData clientData)	/* Pointer to record describing text. */
{
    register TkText *textPtr = clientData;
    TkTextIndex index;
    int x, y, w, h, charWidth;

    if ((textPtr->state == TK_TEXT_STATE_DISABLED) ||
	    !(textPtr->flags & GOT_FOCUS) || (textPtr->insertOffTime == 0)) {
	if (!(textPtr->flags & GOT_FOCUS) &&
		(textPtr->insertUnfocussed != TK_TEXT_INSERT_NOFOCUS_NONE)) {
	    /*
	     * The widget doesn't have the focus yet it is configured to
	     * display the cursor when it doesn't have the focus. Act now!
	     */

	    textPtr->flags |= INSERT_ON;
	    goto redrawInsert;
	}
	if ((textPtr->insertOffTime == 0) && !(textPtr->flags & INSERT_ON)) {
	    /*
	     * The widget was configured to have zero offtime while the
	     * insertion point was not displayed. We have to display it once.
	     */

	    textPtr->flags |= INSERT_ON;
	    goto redrawInsert;
	}
	return;
    }
    if (textPtr->flags & INSERT_ON) {
	textPtr->flags &= ~INSERT_ON;
	textPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		textPtr->insertOffTime, TextBlinkProc, textPtr);
    } else {
	textPtr->flags |= INSERT_ON;
	textPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		textPtr->insertOnTime, TextBlinkProc, textPtr);
    }
  redrawInsert:
    TkTextMarkSegToIndex(textPtr, textPtr->insertMarkPtr, &index);
    if (TkTextIndexBbox(textPtr, &index, &x, &y, &w, &h, &charWidth) == 0) {
	if (textPtr->insertCursorType) {
	    /* Block cursor */
	    TkTextRedrawRegion(textPtr, x - textPtr->width / 2, y,
		    charWidth + textPtr->insertWidth / 2, h);
	} else {
	    /* I-beam cursor */
	    TkTextRedrawRegion(textPtr, x - textPtr->insertWidth / 2, y,
		    textPtr->insertWidth, h);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TextInsertCmd --
 *
 *	This function is invoked to process the "insert" and "replace" widget
 *	commands for text widgets.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *	If 'viewUpdate' is true, we may adjust the window contents'
 *	y-position, and scrollbar setting.
 *
 *----------------------------------------------------------------------
 */

static int
TextInsertCmd(
    TkSharedText *sharedTextPtr,/* Shared portion of peer widgets. */
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects. */
    const TkTextIndex *indexPtr,/* Index at which to insert. */
    int viewUpdate)		/* Update the view if set. */
{
    TkTextIndex index1, index2;
    int j;

    if (sharedTextPtr == NULL) {
	sharedTextPtr = textPtr->sharedTextPtr;
    }

    index1 = *indexPtr;
    for (j = 0; j < objc; j += 2) {
	/*
	 * Here we rely on this call to modify index1 if it is outside the
	 * acceptable range. In particular, if index1 is "end", it must be set
	 * to the last allowable index for insertion, otherwise subsequent tag
	 * insertions will fail.
	 */

	int length = InsertChars(sharedTextPtr, textPtr, &index1, objv[j],
		viewUpdate);

	if (objc > (j+1)) {
	    Tcl_Obj **tagNamePtrs;
	    TkTextTag **oldTagArrayPtr;
	    int numTags;

	    TkTextIndexForwBytes(textPtr, &index1, length, &index2);
	    oldTagArrayPtr = TkBTreeGetTags(&index1, NULL, &numTags);
	    if (oldTagArrayPtr != NULL) {
		int i;

		for (i = 0; i < numTags; i++) {
		    TkBTreeTag(&index1, &index2, oldTagArrayPtr[i], 0);
		}
		ckfree(oldTagArrayPtr);
	    }
	    if (Tcl_ListObjGetElements(interp, objv[j+1], &numTags,
		    &tagNamePtrs) != TCL_OK) {
		return TCL_ERROR;
	    } else {
		int i;

		for (i = 0; i < numTags; i++) {
		    const char *strTag = Tcl_GetString(tagNamePtrs[i]);

		    TkBTreeTag(&index1, &index2,
			    TkTextCreateTag(textPtr, strTag, NULL), 1);
		}
		index1 = index2;
	    }
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TextSearchCmd --
 *
 *	This function is invoked to process the "search" widget command for
 *	text widgets. See the user documentation for details on what it does.
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
TextSearchCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int i, argsLeft, code;
    SearchSpec searchSpec;

    static const char *const switchStrings[] = {
	"-hidden",
	"--", "-all", "-backwards", "-count", "-elide", "-exact", "-forwards",
	"-nocase", "-nolinestop", "-overlap", "-regexp", "-strictlimits", NULL
    };
    enum SearchSwitches {
	TK_TEXT_SEARCH_HIDDEN,
	TK_TEXT_SEARCH_END, TK_TEXT_SEARCH_ALL, TK_TEXT_SEARCH_BACK, TK_TEXT_SEARCH_COUNT, TK_TEXT_SEARCH_ELIDE,
	TK_TEXT_SEARCH_EXACT, TK_TEXT_SEARCH_FWD, TK_TEXT_SEARCH_NOCASE,
	TK_TEXT_SEARCH_NOLINESTOP, TK_TEXT_SEARCH_OVERLAP, TK_TEXT_SEARCH_REGEXP, TK_TEXT_SEARCH_STRICTLIMITS
    };

    /*
     * Set up the search specification, including the last 4 fields which are
     * text widget specific.
     */

    searchSpec.exact = 1;
    searchSpec.noCase = 0;
    searchSpec.all = 0;
    searchSpec.backwards = 0;
    searchSpec.varPtr = NULL;
    searchSpec.countPtr = NULL;
    searchSpec.resPtr = NULL;
    searchSpec.searchElide = 0;
    searchSpec.noLineStop = 0;
    searchSpec.overlap = 0;
    searchSpec.strictLimits = 0;
    searchSpec.numLines =
	    TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr);
    searchSpec.clientData = textPtr;
    searchSpec.addLineProc = &TextSearchAddNextLine;
    searchSpec.foundMatchProc = &TextSearchFoundMatch;
    searchSpec.lineIndexProc = &TextSearchGetLineIndex;

    /*
     * Parse switches and other arguments.
     */

    for (i=2 ; i<objc ; i++) {
	int index;

	if (Tcl_GetString(objv[i])[0] != '-') {
	    break;
	}

	if (Tcl_GetIndexFromObjStruct(NULL, objv[i], switchStrings,
		sizeof(char *), "switch", 0, &index) != TCL_OK) {
	    /*
	     * Hide the -hidden option, generating the error description with
	     * the side effects of T_GIFO.
	     */

	    (void) Tcl_GetIndexFromObjStruct(interp, objv[i], switchStrings+1,
		    sizeof(char *), "switch", 0, &index);
	    return TCL_ERROR;
	}

	switch ((enum SearchSwitches) index) {
	case TK_TEXT_SEARCH_END:
	    i++;
	    goto endOfSwitchProcessing;
	case TK_TEXT_SEARCH_ALL:
	    searchSpec.all = 1;
	    break;
	case TK_TEXT_SEARCH_BACK:
	    searchSpec.backwards = 1;
	    break;
	case TK_TEXT_SEARCH_COUNT:
	    if (i >= objc-1) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"no value given for \"-count\" option", -1));
		Tcl_SetErrorCode(interp, "TK", "TEXT", "VALUE", NULL);
		return TCL_ERROR;
	    }
	    i++;

	    /*
	     * Assumption objv[i] isn't going to disappear on us during this
	     * function, which is fair.
	     */

	    searchSpec.varPtr = objv[i];
	    break;
	case TK_TEXT_SEARCH_ELIDE:
	case TK_TEXT_SEARCH_HIDDEN:
	    searchSpec.searchElide = 1;
	    break;
	case TK_TEXT_SEARCH_EXACT:
	    searchSpec.exact = 1;
	    break;
	case TK_TEXT_SEARCH_FWD:
	    searchSpec.backwards = 0;
	    break;
	case TK_TEXT_SEARCH_NOCASE:
	    searchSpec.noCase = 1;
	    break;
	case TK_TEXT_SEARCH_NOLINESTOP:
	    searchSpec.noLineStop = 1;
	    break;
	case TK_TEXT_SEARCH_OVERLAP:
	    searchSpec.overlap = 1;
	    break;
	case TK_TEXT_SEARCH_STRICTLIMITS:
	    searchSpec.strictLimits = 1;
	    break;
	case TK_TEXT_SEARCH_REGEXP:
	    searchSpec.exact = 0;
	    break;
	default:
	    Tcl_Panic("unexpected switch fallthrough");
	}
    }
  endOfSwitchProcessing:

    argsLeft = objc - (i+2);
    if ((argsLeft != 0) && (argsLeft != 1)) {
	Tcl_WrongNumArgs(interp, 2, objv,
		"?switches? pattern index ?stopIndex?");
	return TCL_ERROR;
    }

    if (searchSpec.noLineStop && searchSpec.exact) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"the \"-nolinestop\" option requires the \"-regexp\" option"
		" to be present", -1));
	Tcl_SetErrorCode(interp, "TK", "TEXT", "SEARCH_USAGE", NULL);
	return TCL_ERROR;
    }

    if (searchSpec.overlap && !searchSpec.all) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"the \"-overlap\" option requires the \"-all\" option"
		" to be present", -1));
	Tcl_SetErrorCode(interp, "TK", "TEXT", "SEARCH_USAGE", NULL);
	return TCL_ERROR;
    }

    /*
     * Scan through all of the lines of the text circularly, starting at the
     * given index. 'objv[i]' is the pattern which may be an exact string or a
     * regexp pattern depending on the flags set above.
     */

    code = SearchPerform(interp, &searchSpec, objv[i], objv[i+1],
	    (argsLeft == 1 ? objv[i+2] : NULL));
    if (code != TCL_OK) {
	goto cleanup;
    }

    /*
     * Set the '-count' variable, if given.
     */

    if (searchSpec.varPtr != NULL && searchSpec.countPtr != NULL) {
	Tcl_IncrRefCount(searchSpec.countPtr);
	if (Tcl_ObjSetVar2(interp, searchSpec.varPtr, NULL,
		searchSpec.countPtr, TCL_LEAVE_ERR_MSG) == NULL) {
	    code = TCL_ERROR;
	    goto cleanup;
	}
    }

    /*
     * Set the result.
     */

    if (searchSpec.resPtr != NULL) {
	Tcl_SetObjResult(interp, searchSpec.resPtr);
	searchSpec.resPtr = NULL;
    }

  cleanup:
    if (searchSpec.countPtr != NULL) {
	Tcl_DecrRefCount(searchSpec.countPtr);
    }
    if (searchSpec.resPtr != NULL) {
	Tcl_DecrRefCount(searchSpec.resPtr);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TextSearchGetLineIndex --
 *
 *	Extract a row, text offset index position from an objPtr
 *
 *	This means we ignore any embedded windows/images and elidden text
 *	(unless we are searching that).
 *
 * Results:
 *	Standard Tcl error code (with a message in the interpreter on error
 *	conditions).
 *
 *	The offset placed in offsetPosPtr is a utf-8 char* byte index for
 *	exact searches, and a Unicode character index for regexp searches.
 *
 *	The line number should start at zero (searches which wrap around
 *	assume the first line is numbered 0).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextSearchGetLineIndex(
    Tcl_Interp *interp,		/* For error messages. */
    Tcl_Obj *objPtr,		/* Contains a textual index like "1.2" */
    SearchSpec *searchSpecPtr,	/* Contains other search parameters. */
    int *linePosPtr,		/* For returning the line number. */
    int *offsetPosPtr)		/* For returning the text offset in the
				 * line. */
{
    const TkTextIndex *indexPtr;
    int line;
    TkText *textPtr = searchSpecPtr->clientData;

    indexPtr = TkTextGetIndexFromObj(interp, textPtr, objPtr);
    if (indexPtr == NULL) {
	return TCL_ERROR;
    }

    line = TkBTreeLinesTo(textPtr, indexPtr->linePtr);
    if (line >= searchSpecPtr->numLines) {
	TkTextLine *linePtr;
	int count = 0;
	TkTextSegment *segPtr;

	line = searchSpecPtr->numLines-1;
	linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr, line);

	/*
	 * Count the number of bytes in this line.
	 */

	for (segPtr=linePtr->segPtr ; segPtr!=NULL ; segPtr=segPtr->nextPtr) {
	    count += segPtr->size;
	}
	*offsetPosPtr = TextSearchIndexInLine(searchSpecPtr, linePtr, count);
    } else {
	*offsetPosPtr = TextSearchIndexInLine(searchSpecPtr,
		indexPtr->linePtr, indexPtr->byteIndex);
    }

    *linePosPtr = line;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TextSearchIndexInLine --
 *
 *	Find textual index of 'byteIndex' in the searchable characters of
 *	'linePtr'.
 *
 *	This means we ignore any embedded windows/images and elidden text
 *	(unless we are searching that).
 *
 * Results:
 *	The returned index is a utf-8 char* byte index for exact searches, and
 *	a Unicode character index for regexp searches.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextSearchIndexInLine(
    const SearchSpec *searchSpecPtr,
				/* Search parameters. */
    TkTextLine *linePtr,	/* The line we're looking at. */
    int byteIndex)		/* Index into the line. */
{
    TkTextSegment *segPtr;
    TkTextIndex curIndex;
    int index, leftToScan;
    TkText *textPtr = searchSpecPtr->clientData;

    index = 0;
    curIndex.tree = textPtr->sharedTextPtr->tree;
    curIndex.linePtr = linePtr; curIndex.byteIndex = 0;
    for (segPtr = linePtr->segPtr, leftToScan = byteIndex;
	    leftToScan > 0;
	    curIndex.byteIndex += segPtr->size, segPtr = segPtr->nextPtr) {
	if ((segPtr->typePtr == &tkTextCharType) &&
		(searchSpecPtr->searchElide
		|| !TkTextIsElided(textPtr, &curIndex, NULL))) {
	    if (leftToScan < segPtr->size) {
		if (searchSpecPtr->exact) {
		    index += leftToScan;
		} else {
		    index += Tcl_NumUtfChars(segPtr->body.chars, leftToScan);
		}
	    } else if (searchSpecPtr->exact) {
		index += segPtr->size;
	    } else {
		index += Tcl_NumUtfChars(segPtr->body.chars, -1);
	    }
	}
	leftToScan -= segPtr->size;
    }
    return index;
}

/*
 *----------------------------------------------------------------------
 *
 * TextSearchAddNextLine --
 *
 *	Adds a line from the text widget to the object 'theLine'.
 *
 * Results:
 *	A pointer to the TkTextLine corresponding to the given line, or NULL
 *	if there was no available line.
 *
 *	Also 'lenPtr' (if non-NULL) is filled in with the total length of
 *	'theLine' (not just what we added to it, but the length including what
 *	was already in there). This is in bytes for an exact search and in
 *	chars for a regexp search.
 *
 *	Also 'extraLinesPtr' (if non-NULL) will have its value incremented by
 *	1 for each additional logical line we have added because a newline is
 *	elided (this will only ever happen if we have chosen not to search
 *	elided text, of course).
 *
 * Side effects:
 *	Memory may be allocated or re-allocated for theLine's string
 *	representation.
 *
 *----------------------------------------------------------------------
 */

static ClientData
TextSearchAddNextLine(
    int lineNum,		/* Line we must add. */
    SearchSpec *searchSpecPtr,	/* Search parameters. */
    Tcl_Obj *theLine,		/* Object to append to. */
    int *lenPtr,		/* For returning the total length. */
    int *extraLinesPtr)		/* If non-NULL, will have its value
				 * incremented by the number of additional
				 * logical lines which are merged into this
				 * one by newlines being elided. */
{
    TkTextLine *linePtr, *thisLinePtr;
    TkTextIndex curIndex;
    TkTextSegment *segPtr;
    TkText *textPtr = searchSpecPtr->clientData;
    int nothingYet = 1;

    /*
     * Extract the text from the line.
     */

    linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr, lineNum);
    if (linePtr == NULL) {
	return NULL;
    }
    curIndex.tree = textPtr->sharedTextPtr->tree;
    thisLinePtr = linePtr;

    while (thisLinePtr != NULL) {
	int elideWraps = 0;

	curIndex.linePtr = thisLinePtr;
	curIndex.byteIndex = 0;
	for (segPtr = thisLinePtr->segPtr; segPtr != NULL;
		curIndex.byteIndex += segPtr->size, segPtr = segPtr->nextPtr) {
	    if (!searchSpecPtr->searchElide
		    && TkTextIsElided(textPtr, &curIndex, NULL)) {
		/*
		 * If we reach the end of the logical line, and if we have at
		 * least one character in the string, then we continue
		 * wrapping to the next logical line. If there are no
		 * characters yet, then the entire line of characters is
		 * elided and there's no need to complicate matters by
		 * wrapping - we'll look at the next line in due course.
		 */

		if (segPtr->nextPtr == NULL && !nothingYet) {
		    elideWraps = 1;
		}
		continue;
	    }
	    if (segPtr->typePtr != &tkTextCharType) {
		continue;
	    }
	    Tcl_AppendToObj(theLine, segPtr->body.chars, segPtr->size);
	    nothingYet = 0;
	}
	if (!elideWraps) {
	    break;
	}
	lineNum++;
	if (lineNum >= searchSpecPtr->numLines) {
	    break;
	}
	thisLinePtr = TkBTreeNextLine(textPtr, thisLinePtr);
	if (thisLinePtr != NULL && extraLinesPtr != NULL) {
	    /*
	     * Tell our caller we have an extra line merged in.
	     */

	    *extraLinesPtr = (*extraLinesPtr) + 1;
	}
    }

    /*
     * If we're ignoring case, convert the line to lower case. There is no
     * need to do this for regexp searches, since they handle a flag for this
     * purpose.
     */

    if (searchSpecPtr->exact && searchSpecPtr->noCase) {
	Tcl_SetObjLength(theLine, Tcl_UtfToLower(Tcl_GetString(theLine)));
    }

    if (lenPtr != NULL) {
	if (searchSpecPtr->exact) {
	    Tcl_GetString(theLine);
	    *lenPtr = theLine->length;
	} else {
	    *lenPtr = Tcl_GetCharLength(theLine);
	}
    }
    return linePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TextSearchFoundMatch --
 *
 *	Stores information from a successful search.
 *
 * Results:
 *	1 if the information was stored, 0 if the position at which the match
 *	was found actually falls outside the allowable search region (and
 *	therefore the search is actually complete).
 *
 * Side effects:
 *	Memory may be allocated in the 'countPtr' and 'resPtr' fields of
 *	'searchSpecPtr'. Each of those objects will have refCount zero and
 *	must eventually be freed or stored elsewhere as appropriate.
 *
 *----------------------------------------------------------------------
 */

static int
TextSearchFoundMatch(
    int lineNum,		/* Line on which match was found. */
    SearchSpec *searchSpecPtr,	/* Search parameters. */
    ClientData clientData,	/* Token returned by the 'addNextLineProc',
				 * TextSearchAddNextLine. May be NULL, in
				 * which we case we must generate it (from
				 * lineNum). */
    Tcl_Obj *theLine,		/* Text from current line, only accessed for
				 * exact searches, and is allowed to be NULL
				 * for regexp searches. */
    int matchOffset,		/* Offset of found item in utf-8 bytes for
				 * exact search, Unicode chars for regexp. */
    int matchLength)		/* Length also in bytes/chars as per search
				 * type. */
{
    int numChars;
    int leftToScan;
    TkTextIndex curIndex, foundIndex;
    TkTextSegment *segPtr;
    TkTextLine *linePtr;
    TkText *textPtr = searchSpecPtr->clientData;

    if (lineNum == searchSpecPtr->stopLine) {
	/*
	 * If the current index is on the wrong side of the stopIndex, then
	 * the item we just found is actually outside the acceptable range,
	 * and the search is over.
	 */

	if (searchSpecPtr->backwards ^
		(matchOffset >= searchSpecPtr->stopOffset)) {
	    return 0;
	}
    }

    /*
     * Calculate the character count, which may need augmenting if there are
     * embedded windows or elidden text.
     */

    if (searchSpecPtr->exact) {
	const char *startOfLine = Tcl_GetString(theLine);

	numChars = Tcl_NumUtfChars(startOfLine + matchOffset, matchLength);
    } else {
	numChars = matchLength;
    }

    /*
     * If we're using strict limits checking, ensure that the match with its
     * full length fits inside the given range.
     */

    if (searchSpecPtr->strictLimits && lineNum == searchSpecPtr->stopLine) {
	if (searchSpecPtr->backwards ^
		((matchOffset + numChars) > searchSpecPtr->stopOffset)) {
	    return 0;
	}
    }

    /*
     * The index information returned by the regular expression parser only
     * considers textual information: it doesn't account for embedded windows,
     * elided text (when we are not searching elided text) or any other
     * non-textual info. Scan through the line's segments again to adjust both
     * matchChar and matchCount.
     *
     * We will walk through the segments of this line until we have either
     * reached the end of the match or we have reached the end of the line.
     */

    linePtr = clientData;
    if (linePtr == NULL) {
	linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr,
		lineNum);
    }

    curIndex.tree = textPtr->sharedTextPtr->tree;

    /*
     * Find the starting point.
     */

    leftToScan = matchOffset;
    while (1) {
	curIndex.linePtr = linePtr;
	curIndex.byteIndex = 0;

	/*
	 * Note that we allow leftToScan to be zero because we want to skip
	 * over any preceding non-textual items.
	 */

	for (segPtr = linePtr->segPtr; leftToScan >= 0 && segPtr;
		segPtr = segPtr->nextPtr) {
	    if (segPtr->typePtr != &tkTextCharType) {
		matchOffset += segPtr->size;
	    } else if (!searchSpecPtr->searchElide
		    && TkTextIsElided(textPtr, &curIndex, NULL)) {
		if (searchSpecPtr->exact) {
		    matchOffset += segPtr->size;
		} else {
		    matchOffset += Tcl_NumUtfChars(segPtr->body.chars, -1);
		}
	    } else {
		if (searchSpecPtr->exact) {
		    leftToScan -= segPtr->size;
		} else {
		    leftToScan -= Tcl_NumUtfChars(segPtr->body.chars, -1);
		}
	    }
	    curIndex.byteIndex += segPtr->size;
	}
	if (segPtr == NULL && leftToScan >= 0) {
	    /*
	     * This will only happen if we are eliding newlines.
	     */

	    linePtr = TkBTreeNextLine(textPtr, linePtr);
	    if (linePtr == NULL) {
		/*
		 * If we reach the end of the text, we have a serious problem,
		 * unless there's actually nothing left to look for.
		 */

		if (leftToScan == 0) {
		    break;
		} else {
		    Tcl_Panic("Reached end of text in a match");
		}
	    }

	    /*
	     * We've wrapped to the beginning of the next logical line, which
	     * has been merged with the previous one whose newline was elided.
	     */

	    lineNum++;
	    matchOffset = 0;
	} else {
	    break;
	}
    }

    /*
     * Calculate and store the found index in the result.
     */

    if (searchSpecPtr->exact) {
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, lineNum,
		matchOffset, &foundIndex);
    } else {
	TkTextMakeCharIndex(textPtr->sharedTextPtr->tree, textPtr, lineNum,
		matchOffset, &foundIndex);
    }

    if (searchSpecPtr->all) {
	if (searchSpecPtr->resPtr == NULL) {
	    searchSpecPtr->resPtr = Tcl_NewObj();
	}
	Tcl_ListObjAppendElement(NULL, searchSpecPtr->resPtr,
		TkTextNewIndexObj(textPtr, &foundIndex));
    } else {
	searchSpecPtr->resPtr = TkTextNewIndexObj(textPtr, &foundIndex);
    }

    /*
     * Find the end point. Here 'leftToScan' could be negative already as a
     * result of the above loop if the segment we reached spanned the start of
     * the string. When we add matchLength it will become non-negative.
     */

    for (leftToScan += matchLength; leftToScan > 0;
	    curIndex.byteIndex += segPtr->size, segPtr = segPtr->nextPtr) {
	if (segPtr == NULL) {
	    /*
	     * We are on the next line - this of course should only ever
	     * happen with searches which have matched across multiple lines.
	     */

	    linePtr = TkBTreeNextLine(textPtr, linePtr);
	    segPtr = linePtr->segPtr;
	    curIndex.linePtr = linePtr; curIndex.byteIndex = 0;
	}
	if (segPtr->typePtr != &tkTextCharType) {
	    /*
	     * Anything we didn't count in the search needs adding.
	     */

	    numChars += segPtr->size;
	    continue;
	} else if (!searchSpecPtr->searchElide
		&& TkTextIsElided(textPtr, &curIndex, NULL)) {
	    numChars += Tcl_NumUtfChars(segPtr->body.chars, -1);
	    continue;
	}
	if (searchSpecPtr->exact) {
	    leftToScan -= segPtr->size;
	} else {
	    leftToScan -= Tcl_NumUtfChars(segPtr->body.chars, -1);
	}
    }

    /*
     * Now store the count result, if it is wanted.
     */

    if (searchSpecPtr->varPtr != NULL) {
	Tcl_Obj *tmpPtr = Tcl_NewIntObj(numChars);
	if (searchSpecPtr->all) {
	    if (searchSpecPtr->countPtr == NULL) {
		searchSpecPtr->countPtr = Tcl_NewObj();
	    }
	    Tcl_ListObjAppendElement(NULL, searchSpecPtr->countPtr, tmpPtr);
	} else {
	    searchSpecPtr->countPtr = tmpPtr;
	}
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextGetTabs --
 *
 *	Parses a string description of a set of tab stops.
 *
 * Results:
 *	The return value is a pointer to a malloc'ed structure holding parsed
 *	information about the tab stops. If an error occurred then the return
 *	value is NULL and an error message is left in the interp's result.
 *
 * Side effects:
 *	Memory is allocated for the structure that is returned. It is up to
 *	the caller to free this structure when it is no longer needed.
 *
 *----------------------------------------------------------------------
 */

TkTextTabArray *
TkTextGetTabs(
    Tcl_Interp *interp,		/* Used for error reporting. */
    TkText *textPtr,		/* Information about the text widget. */
    Tcl_Obj *stringPtr)		/* Description of the tab stops. See the text
				 * manual entry for details. */
{
    int objc, i, count;
    Tcl_Obj **objv;
    TkTextTabArray *tabArrayPtr;
    TkTextTab *tabPtr;
    int ch;
    double prevStop, lastStop;
    /*
     * Map these strings to TkTextTabAlign values.
     */
    static const char *const tabOptionStrings[] = {
	"left", "right", "center", "numeric", NULL
    };

    if (Tcl_ListObjGetElements(interp, stringPtr, &objc, &objv) != TCL_OK) {
	return NULL;
    }

    /*
     * First find out how many entries we need to allocate in the tab array.
     */

    count = 0;
    for (i = 0; i < objc; i++) {
	char c = Tcl_GetString(objv[i])[0];

	if ((c != 'l') && (c != 'r') && (c != 'c') && (c != 'n')) {
	    count++;
	}
    }

    /*
     * Parse the elements of the list one at a time to fill in the array.
     */

    tabArrayPtr = ckalloc(sizeof(TkTextTabArray)
	    + (count - 1) * sizeof(TkTextTab));
    tabArrayPtr->numTabs = 0;
    prevStop = 0.0;
    lastStop = 0.0;
    for (i = 0, tabPtr = &tabArrayPtr->tabs[0]; i < objc; i++, tabPtr++) {
	int index;

	/*
	 * This will round fractional pixels above 0.5 upwards, and otherwise
	 * downwards, to find the right integer pixel position.
	 */

	if (Tk_GetPixelsFromObj(interp, textPtr->tkwin, objv[i],
		&tabPtr->location) != TCL_OK) {
	    goto error;
	}

	if (tabPtr->location <= 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "tab stop \"%s\" is not at a positive distance",
		    Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "TAB_STOP", NULL);
	    goto error;
	}

	prevStop = lastStop;
	if (Tk_GetDoublePixelsFromObj(interp, textPtr->tkwin, objv[i],
		&lastStop) != TCL_OK) {
	    goto error;
	}

	if (i > 0 && (tabPtr->location <= (tabPtr-1)->location)) {
	    /*
	     * This tab is actually to the left of the previous one, which is
	     * illegal.
	     */

#ifdef _TK_ALLOW_DECREASING_TABS
	    /*
	     * Force the tab to be a typical character width to the right of
	     * the previous one, and update the 'lastStop' with the changed
	     * position.
	     */

	    if (textPtr->charWidth > 0) {
		tabPtr->location = (tabPtr-1)->location + textPtr->charWidth;
	    } else {
		tabPtr->location = (tabPtr-1)->location + 8;
	    }
	    lastStop = tabPtr->location;
#else
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "tabs must be monotonically increasing, but \"%s\" is "
		    "smaller than or equal to the previous tab",
		    Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "TAB_STOP", NULL);
	    goto error;
#endif /* _TK_ALLOW_DECREASING_TABS */
	}

	tabArrayPtr->numTabs++;

	/*
	 * See if there is an explicit alignment in the next list element.
	 * Otherwise just use "left".
	 */

	tabPtr->alignment = LEFT;
	if ((i+1) == objc) {
	    continue;
	}

	/*
	 * There may be a more efficient way of getting this.
	 */

	TkUtfToUniChar(Tcl_GetString(objv[i+1]), &ch);
	if (!Tcl_UniCharIsAlpha(ch)) {
	    continue;
	}
	i += 1;

	if (Tcl_GetIndexFromObjStruct(interp, objv[i], tabOptionStrings,
		sizeof(char *), "tab alignment", 0, &index) != TCL_OK) {
	    goto error;
	}
	tabPtr->alignment = (TkTextTabAlign) index;
    }

    /*
     * For when we need to interpolate tab stops, store these two so we know
     * the tab stop size to very high precision. With the above checks, we can
     * guarantee that tabIncrement is strictly positive here.
     */

    tabArrayPtr->lastTab = lastStop;
    tabArrayPtr->tabIncrement = lastStop - prevStop;

    return tabArrayPtr;

  error:
    ckfree(tabArrayPtr);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TextDumpCmd --
 *
 *	Return information about the text, tags, marks, and embedded windows
 *	and images in a text widget. See the man page for the description of
 *	the text dump operation for all the details.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Memory is allocated for the result, if needed (standard Tcl result
 *	side effects).
 *
 *----------------------------------------------------------------------
 */

static int
TextDumpCmd(
    register TkText *textPtr,	/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "dump". */
{
    TkTextIndex index1, index2;
    int arg;
    int lineno;			/* Current line number. */
    int what = 0;		/* bitfield to select segment types. */
    int atEnd;			/* True if dumping up to logical end. */
    TkTextLine *linePtr;
    Tcl_Obj *command = NULL;	/* Script callback to apply to segments. */
#define TK_DUMP_TEXT	0x1
#define TK_DUMP_MARK	0x2
#define TK_DUMP_TAG	0x4
#define TK_DUMP_WIN	0x8
#define TK_DUMP_IMG	0x10
#define TK_DUMP_ALL	(TK_DUMP_TEXT|TK_DUMP_MARK|TK_DUMP_TAG| \
	TK_DUMP_WIN|TK_DUMP_IMG)
    static const char *const optStrings[] = {
	"-all", "-command", "-image", "-mark", "-tag", "-text", "-window",
	NULL
    };
    enum opts {
	DUMP_ALL, DUMP_CMD, DUMP_IMG, DUMP_MARK, DUMP_TAG, DUMP_TXT, DUMP_WIN
    };

    for (arg=2 ; arg < objc ; arg++) {
	int index;
	if (Tcl_GetString(objv[arg])[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObjStruct(interp, objv[arg], optStrings,
		sizeof(char *), "option", 0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum opts) index) {
	case DUMP_ALL:
	    what = TK_DUMP_ALL;
	    break;
	case DUMP_TXT:
	    what |= TK_DUMP_TEXT;
	    break;
	case DUMP_TAG:
	    what |= TK_DUMP_TAG;
	    break;
	case DUMP_MARK:
	    what |= TK_DUMP_MARK;
	    break;
	case DUMP_IMG:
	    what |= TK_DUMP_IMG;
	    break;
	case DUMP_WIN:
	    what |= TK_DUMP_WIN;
	    break;
	case DUMP_CMD:
	    arg++;
	    if (arg >= objc) {
		goto wrongArgs;
	    }
	    command = objv[arg];
	    break;
	default:
	    Tcl_Panic("unexpected switch fallthrough");
	}
    }
    if (arg >= objc || arg+2 < objc) {
    wrongArgs:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Usage: %s dump ?-all -image -text -mark -tag -window? "
		"?-command script? index ?index2?", Tcl_GetString(objv[0])));
	Tcl_SetErrorCode(interp, "TCL", "WRONGARGS", NULL);
	return TCL_ERROR;
    }
    if (what == 0) {
	what = TK_DUMP_ALL;
    }
    if (TkTextGetObjIndex(interp, textPtr, objv[arg], &index1) != TCL_OK) {
	return TCL_ERROR;
    }
    arg++;
    atEnd = 0;
    if (objc == arg) {
	TkTextIndexForwChars(NULL, &index1, 1, &index2, COUNT_INDICES);
    } else {
	size_t length;
	const char *str;

	if (TkTextGetObjIndex(interp, textPtr, objv[arg], &index2) != TCL_OK) {
	    return TCL_ERROR;
	}
	str = Tcl_GetString(objv[arg]);
	length = objv[arg]->length;
	if (strncmp(str, "end", length) == 0) {
	    atEnd = 1;
	}
    }
    if (TkTextIndexCmp(&index1, &index2) >= 0) {
	return TCL_OK;
    }
    lineno = TkBTreeLinesTo(textPtr, index1.linePtr);
    if (index1.linePtr == index2.linePtr) {
	DumpLine(interp, textPtr, what, index1.linePtr,
		index1.byteIndex, index2.byteIndex, lineno, command);
    } else {
	int textChanged;
	int lineend = TkBTreeLinesTo(textPtr, index2.linePtr);
	int endByteIndex = index2.byteIndex;

	textChanged = DumpLine(interp, textPtr, what, index1.linePtr,
		index1.byteIndex, 32000000, lineno, command);
	if (textChanged) {
	    if (textPtr->flags & DESTROYED) {
		return TCL_OK;
	    }
	    linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree,
		    textPtr, lineno);
	    textChanged = 0;
	} else {
	    linePtr = index1.linePtr;
	}
	while ((linePtr = TkBTreeNextLine(textPtr, linePtr)) != NULL) {
	    lineno++;
	    if (lineno == lineend) {
		break;
	    }
	    textChanged = DumpLine(interp, textPtr, what, linePtr, 0,
		    32000000, lineno, command);
	    if (textChanged) {
		if (textPtr->flags & DESTROYED) {
		    return TCL_OK;
		}
		linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree,
			textPtr, lineno);
		textChanged = 0;
	    }
	}
	if (linePtr != NULL) {
	    DumpLine(interp, textPtr, what, linePtr, 0, endByteIndex, lineno,
		    command);
	    if (textPtr->flags & DESTROYED) {
		return TCL_OK;
	    }
	}
    }

    /*
     * Special case to get the leftovers hiding at the end mark.
     */

    if (atEnd) {
	if (textPtr->flags & DESTROYED) {
	    return TCL_OK;
	}

	/*
	 * Re-get the end index, in case it has changed.
	 */

	if (TkTextGetObjIndex(interp, textPtr, objv[arg], &index2) != TCL_OK) {
	    return TCL_ERROR;
	}
	DumpLine(interp, textPtr, what & ~TK_DUMP_TEXT, index2.linePtr,
		0, 1, lineno, command);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DumpLine
 *
 *	Return information about a given text line from character position
 *	"start" up to, but not including, "end".
 *
 * Results:
 *	Returns 1 if the command callback made any changes to the text widget
 *	which will have invalidated internal structures such as TkTextSegment,
 *	TkTextIndex, pointers. Our caller can then take action to recompute
 *	such entities. Returns 0 otherwise.
 *
 * Side effects:
 *	None, but see DumpSegment which can have arbitrary side-effects
 *
 *----------------------------------------------------------------------
 */

static int
DumpLine(
    Tcl_Interp *interp,
    TkText *textPtr,
    int what,			/* Bit flags to select segment types. */
    TkTextLine *linePtr,	/* The current line. */
    int startByte, int endByte,	/* Byte range to dump. */
    int lineno,			/* Line number for indices dump. */
    Tcl_Obj *command)		/* Script to apply to the segment. */
{
    TkTextSegment *segPtr;
    TkTextIndex index;
    int offset = 0, textChanged = 0;

    /*
     * Must loop through line looking at its segments.
     * character
     * toggleOn, toggleOff
     * mark
     * image
     * window
     */

    segPtr = linePtr->segPtr;
    while ((offset < endByte) && (segPtr != NULL)) {
	int lineChanged = 0;
	int currentSize = segPtr->size;

	if ((what & TK_DUMP_TEXT) && (segPtr->typePtr == &tkTextCharType) &&
		(offset + currentSize > startByte)) {
	    int last = currentSize;	/* Index of last char in seg. */
	    int first = 0;		/* Index of first char in seg. */

	    if (offset + currentSize > endByte) {
		last = endByte - offset;
	    }
	    if (startByte > offset) {
		first = startByte - offset;
	    }
	    if (last != currentSize) {
		/*
		 * To avoid modifying the string in place we copy over just
		 * the segment that we want. Since DumpSegment can modify the
		 * text, we could not confidently revert the modification
		 * here.
		 */

		int length = last - first;
		char *range = ckalloc(length + 1);

		memcpy(range, segPtr->body.chars + first, length);
		range[length] = '\0';

		TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			lineno, offset + first, &index);
		lineChanged = DumpSegment(textPtr, interp, "text", range,
			command, &index, what);
		ckfree(range);
	    } else {
		TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			lineno, offset + first, &index);
		lineChanged = DumpSegment(textPtr, interp, "text",
			segPtr->body.chars + first, command, &index, what);
	    }
	} else if ((offset >= startByte)) {
	    if ((what & TK_DUMP_MARK)
		    && (segPtr->typePtr == &tkTextLeftMarkType
		    || segPtr->typePtr == &tkTextRightMarkType)) {
		const char *name;
		TkTextMark *markPtr = &segPtr->body.mark;

		if (segPtr == textPtr->insertMarkPtr) {
		    name = "insert";
		} else if (segPtr == textPtr->currentMarkPtr) {
		    name = "current";
		} else if (markPtr->hPtr == NULL) {
		    name = NULL;
		    lineChanged = 0;
		} else {
		    name = Tcl_GetHashKey(&textPtr->sharedTextPtr->markTable,
			    markPtr->hPtr);
		}
		if (name != NULL) {
		    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			    lineno, offset, &index);
		    lineChanged = DumpSegment(textPtr, interp, "mark", name,
			    command, &index, what);
		}
	    } else if ((what & TK_DUMP_TAG) &&
		    (segPtr->typePtr == &tkTextToggleOnType)) {
		TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			lineno, offset, &index);
		lineChanged = DumpSegment(textPtr, interp, "tagon",
			segPtr->body.toggle.tagPtr->name, command, &index,
			what);
	    } else if ((what & TK_DUMP_TAG) &&
		    (segPtr->typePtr == &tkTextToggleOffType)) {
		TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			lineno, offset, &index);
		lineChanged = DumpSegment(textPtr, interp, "tagoff",
			segPtr->body.toggle.tagPtr->name, command, &index,
			what);
	    } else if ((what & TK_DUMP_IMG) &&
		    (segPtr->typePtr == &tkTextEmbImageType)) {
		TkTextEmbImage *eiPtr = &segPtr->body.ei;
		const char *name = (eiPtr->name == NULL) ? "" : eiPtr->name;

		TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			lineno, offset, &index);
		lineChanged = DumpSegment(textPtr, interp, "image", name,
			command, &index, what);
	    } else if ((what & TK_DUMP_WIN) &&
		    (segPtr->typePtr == &tkTextEmbWindowType)) {
		TkTextEmbWindow *ewPtr = &segPtr->body.ew;
		const char *pathname;

		if (ewPtr->tkwin == NULL) {
		    pathname = "";
		} else {
		    pathname = Tk_PathName(ewPtr->tkwin);
		}
		TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
			lineno, offset, &index);
		lineChanged = DumpSegment(textPtr, interp, "window", pathname,
			command, &index, what);
	    }
	}

	offset += currentSize;
	if (lineChanged) {
	    TkTextSegment *newSegPtr;
	    int newOffset = 0;

	    textChanged = 1;

	    /*
	     * Our indices are no longer valid.
	     */

	    if (textPtr->flags & DESTROYED) {
		return textChanged;
	    }
	    linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree,
		    textPtr, lineno);
	    newSegPtr = linePtr->segPtr;
	    if (segPtr != newSegPtr) {
		while ((newOffset < endByte) && (newOffset < offset)
			&& (newSegPtr != NULL)) {
		    newOffset += currentSize;
		    newSegPtr = newSegPtr->nextPtr;
		    if (segPtr == newSegPtr) {
			break;
		    }
		}
		if (segPtr != newSegPtr && newOffset == offset
			&& currentSize == 0) {
		    TkTextSegment *searchPtr = newSegPtr;

		    while (searchPtr != NULL && searchPtr->size == 0) {
			if (searchPtr == segPtr) {
			    newSegPtr = searchPtr;
			    break;
			}
			searchPtr = searchPtr->nextPtr;
		    }
		}
		segPtr = newSegPtr;
	    }
	}
	if (segPtr != NULL) {
	    segPtr = segPtr->nextPtr;
	}
    }
    return textChanged;
}

/*
 *----------------------------------------------------------------------
 *
 * DumpSegment
 *
 *	Either append information about the current segment to the result, or
 *	make a script callback with that information as arguments.
 *
 * Results:
 *	Returns 1 if the command callback made any changes to the text widget
 *	which will have invalidated internal structures such as TkTextSegment,
 *	TkTextIndex, pointers. Our caller can then take action to recompute
 *	such entities. Returns 0 otherwise.
 *
 * Side effects:
 *	Either evals the callback or appends elements to the result string.
 *	The callback can have arbitrary side-effects.
 *
 *----------------------------------------------------------------------
 */

static int
DumpSegment(
    TkText *textPtr,
    Tcl_Interp *interp,
    const char *key,		/* Segment type key. */
    const char *value,		/* Segment value. */
    Tcl_Obj *command,		/* Script callback. */
    const TkTextIndex *index,	/* index with line/byte position info. */
    int what)			/* Look for TK_DUMP_INDEX bit. */
{
    char buffer[TK_POS_CHARS];
    Tcl_Obj *values[3], *tuple;

    TkTextPrintIndex(textPtr, index, buffer);
    values[0] = Tcl_NewStringObj(key, -1);
    values[1] = Tcl_NewStringObj(value, -1);
    values[2] = Tcl_NewStringObj(buffer, -1);
    tuple = Tcl_NewListObj(3, values);
    if (command == NULL) {
	Tcl_ListObjAppendList(NULL, Tcl_GetObjResult(interp), tuple);
	Tcl_DecrRefCount(tuple);
	return 0;
    } else {
	int oldStateEpoch = TkBTreeEpoch(textPtr->sharedTextPtr->tree);
	Tcl_DString buf;
	int code;

	Tcl_DStringInit(&buf);
	Tcl_DStringAppend(&buf, Tcl_GetString(command), -1);
	Tcl_DStringAppend(&buf, " ", -1);
	Tcl_DStringAppend(&buf, Tcl_GetString(tuple), -1);
	code = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
	Tcl_DStringFree(&buf);
	if (code != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (segment dumping command executed by text)");
	    Tcl_BackgroundException(interp, code);
	}
	Tcl_DecrRefCount(tuple);
	return ((textPtr->flags & DESTROYED) ||
		TkBTreeEpoch(textPtr->sharedTextPtr->tree) != oldStateEpoch);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TextEditUndo --
 *
 *	Undo the last change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Apart from manipulating the undo and redo stacks, the state of the
 *	rest of the widget may also change (due to whatever is being undone).
 *
 *----------------------------------------------------------------------
 */

static int
TextEditUndo(
    TkText *textPtr)		/* Overall information about text widget. */
{
    int status;

    if (!textPtr->sharedTextPtr->undo) {
	return TCL_OK;
    }

    /*
     * Turn off the undo feature while we revert a compound action, setting
     * the dirty handling mode to undo for the duration (unless it is
     * 'fixed').
     */

    textPtr->sharedTextPtr->undo = 0;
    if (textPtr->sharedTextPtr->dirtyMode != TK_TEXT_DIRTY_FIXED) {
	textPtr->sharedTextPtr->dirtyMode = TK_TEXT_DIRTY_UNDO;
    }

    status = TkUndoRevert(textPtr->sharedTextPtr->undoStack);

    if (textPtr->sharedTextPtr->dirtyMode != TK_TEXT_DIRTY_FIXED) {
	textPtr->sharedTextPtr->dirtyMode = TK_TEXT_DIRTY_NORMAL;
    }
    textPtr->sharedTextPtr->undo = 1;

    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * TextEditRedo --
 *
 *	Redo the last undone change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Apart from manipulating the undo and redo stacks, the state of the
 *	rest of the widget may also change (due to whatever is being redone).
 *
 *----------------------------------------------------------------------
 */

static int
TextEditRedo(
    TkText *textPtr)		/* Overall information about text widget. */
{
    int status;

    if (!textPtr->sharedTextPtr->undo) {
	return TCL_OK;
    }

    /*
     * Turn off the undo feature temporarily while we revert a previously
     * undone compound action, setting the dirty handling mode to redo for the
     * duration (unless it is 'fixed').
     */

    textPtr->sharedTextPtr->undo = 0;
    if (textPtr->sharedTextPtr->dirtyMode != TK_TEXT_DIRTY_FIXED) {
	textPtr->sharedTextPtr->dirtyMode = TK_TEXT_DIRTY_REDO;
    }

    status = TkUndoApply(textPtr->sharedTextPtr->undoStack);

    if (textPtr->sharedTextPtr->dirtyMode != TK_TEXT_DIRTY_FIXED) {
	textPtr->sharedTextPtr->dirtyMode = TK_TEXT_DIRTY_NORMAL;
    }
    textPtr->sharedTextPtr->undo = 1;
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * TextEditCmd --
 *
 *	Handle the subcommands to "$text edit ...". See documentation for
 *	details.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextEditCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int index, setModified, oldModified;
    int canRedo = 0;
    int canUndo = 0;

    static const char *const editOptionStrings[] = {
	"canundo", "canredo", "modified", "redo", "reset", "separator",
	"undo", NULL
    };
    enum editOptions {
	EDIT_CANUNDO, EDIT_CANREDO, EDIT_MODIFIED, EDIT_REDO, EDIT_RESET,
	EDIT_SEPARATOR, EDIT_UNDO
    };

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[2], editOptionStrings,
	    sizeof(char *), "edit option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum editOptions) index) {
    case EDIT_CANREDO:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	     return TCL_ERROR;
	}
	if (textPtr->sharedTextPtr->undo) {
	    canRedo = TkUndoCanRedo(textPtr->sharedTextPtr->undoStack);
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(canRedo));
	break;
    case EDIT_CANUNDO:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	     return TCL_ERROR;
	}
	if (textPtr->sharedTextPtr->undo) {
	    canUndo = TkUndoCanUndo(textPtr->sharedTextPtr->undoStack);
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(canUndo));
	break;
    case EDIT_MODIFIED:
	if (objc == 3) {
	    Tcl_SetObjResult(interp,
		    Tcl_NewBooleanObj(textPtr->sharedTextPtr->isDirty));
	    return TCL_OK;
	} else if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "?boolean?");
	    return TCL_ERROR;
	} else if (Tcl_GetBooleanFromObj(interp, objv[3],
		&setModified) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * Set or reset the dirty info, and trigger a Modified event.
	 */

	setModified = setModified ? 1 : 0;

	oldModified = textPtr->sharedTextPtr->isDirty;
	textPtr->sharedTextPtr->isDirty = setModified;
	if (setModified) {
	    textPtr->sharedTextPtr->dirtyMode = TK_TEXT_DIRTY_FIXED;
	} else {
	    textPtr->sharedTextPtr->dirtyMode = TK_TEXT_DIRTY_NORMAL;
	}

	/*
	 * Only issue the <<Modified>> event if the flag actually changed.
	 * However, degree of modified-ness doesn't matter. [Bug 1799782]
	 */

	if ((!oldModified) != (!setModified)) {
	    GenerateModifiedEvent(textPtr);
	}
	break;
    case EDIT_REDO:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	canUndo = TkUndoCanUndo(textPtr->sharedTextPtr->undoStack);
	if (TextEditRedo(textPtr)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("nothing to redo", -1));
	    Tcl_SetErrorCode(interp, "TK", "TEXT", "NO_REDO", NULL);
	    return TCL_ERROR;
	}
	canRedo = TkUndoCanRedo(textPtr->sharedTextPtr->undoStack);
	if (!canUndo || !canRedo) {
	    GenerateUndoStackEvent(textPtr);
	}
	break;
    case EDIT_RESET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	canUndo = TkUndoCanUndo(textPtr->sharedTextPtr->undoStack);
	canRedo = TkUndoCanRedo(textPtr->sharedTextPtr->undoStack);
	TkUndoClearStacks(textPtr->sharedTextPtr->undoStack);
	if (canUndo || canRedo) {
	    GenerateUndoStackEvent(textPtr);
	}
	break;
    case EDIT_SEPARATOR:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	TkUndoInsertUndoSeparator(textPtr->sharedTextPtr->undoStack);
	break;
    case EDIT_UNDO:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	canRedo = TkUndoCanRedo(textPtr->sharedTextPtr->undoStack);
	if (TextEditUndo(textPtr)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("nothing to undo", -1));
	    Tcl_SetErrorCode(interp, "TK", "TEXT", "NO_UNDO", NULL);
	    return TCL_ERROR;
	}
	canUndo = TkUndoCanUndo(textPtr->sharedTextPtr->undoStack);
	if (!canRedo || !canUndo) {
	    GenerateUndoStackEvent(textPtr);
	}
	break;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TextGetText --
 *
 *	Returns the text from indexPtr1 to indexPtr2, placing that text in a
 *	string object which is returned with a refCount of zero.
 *
 *	Since the amount of text may potentially be several megabytes (e.g.
 *	in text editors built on the text widget), efficiency is very
 *	important. We may want to investigate the efficiency of the
 *	Tcl_AppendToObj more carefully (e.g. if we know we are going to be
 *	appending several thousand lines, we could attempt to pre-allocate a
 *	larger space).
 *
 *	Also the result is built up as a utf-8 string, but, if we knew we
 *	wanted it as Unicode, we could potentially save a huge conversion by
 *	building it up as Unicode directly. This could be as simple as
 *	replacing Tcl_NewObj by Tcl_NewUnicodeObj.
 *
 * Results:
 *	Tcl_Obj of string type containing the specified text. If the
 *	visibleOnly flag is set to 1, then only those characters which are not
 *	elided will be returned. Otherwise (flag is 0) all characters in the
 *	given range are returned.
 *
 * Side effects:
 *	Memory will be allocated for the new object. Remember to free it if it
 *	isn't going to be stored appropriately.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
TextGetText(
    const TkText *textPtr,	/* Information about text widget. */
    const TkTextIndex *indexPtr1,
				/* Get text from this index... */
    const TkTextIndex *indexPtr2,
				/* ...to this index. */
    int visibleOnly)		/* If non-zero, then only return non-elided
				 * characters. */
{
    TkTextIndex tmpIndex;
    Tcl_Obj *resultPtr = Tcl_NewObj();

    TkTextMakeByteIndex(indexPtr1->tree, textPtr,
	    TkBTreeLinesTo(textPtr, indexPtr1->linePtr),
	    indexPtr1->byteIndex, &tmpIndex);

    if (TkTextIndexCmp(indexPtr1, indexPtr2) < 0) {
	while (1) {
	    int offset;
	    TkTextSegment *segPtr = TkTextIndexToSeg(&tmpIndex, &offset);
	    int last = segPtr->size, last2;

	    if (tmpIndex.linePtr == indexPtr2->linePtr) {
		/*
		 * The last line that was requested must be handled carefully,
		 * because we may need to break out of this loop in the middle
		 * of the line.
		 */

		if (indexPtr2->byteIndex == tmpIndex.byteIndex) {
		    break;
		}
		last2 = indexPtr2->byteIndex - tmpIndex.byteIndex + offset;
		if (last2 < last) {
		    last = last2;
		}
	    }
	    if (segPtr->typePtr == &tkTextCharType &&
		    !(visibleOnly && TkTextIsElided(textPtr,&tmpIndex,NULL))){
		Tcl_AppendToObj(resultPtr, segPtr->body.chars + offset,
			last - offset);
	    }
	    TkTextIndexForwBytes(textPtr, &tmpIndex, last-offset, &tmpIndex);
	}
    }
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateModifiedEvent --
 *
 *	Send an event that the text was modified. This is equivalent to:
 *	   event generate $textWidget <<Modified>>
 *	for all peers of $textWidget.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	May force the text window into existence.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateModifiedEvent(
    TkText *textPtr)	/* Information about text widget. */
{
    for (textPtr = textPtr->sharedTextPtr->peers; textPtr != NULL;
	    textPtr = textPtr->next) {
	Tk_MakeWindowExist(textPtr->tkwin);
	TkSendVirtualEvent(textPtr->tkwin, "Modified", NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateUndoStackEvent --
 *
 *	Send an event that the undo or redo stack became empty or unempty.
 *	This is equivalent to:
 *	   event generate $textWidget <<UndoStack>>
 *	for all peers of $textWidget.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	May force the text window (and all peers) into existence.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateUndoStackEvent(
    TkText *textPtr)	/* Information about text widget. */
{
    for (textPtr = textPtr->sharedTextPtr->peers; textPtr != NULL;
	    textPtr = textPtr->next) {
	Tk_MakeWindowExist(textPtr->tkwin);
	TkSendVirtualEvent(textPtr->tkwin, "UndoStack", NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateDirtyFlag --
 *
 *	Updates the dirtyness of the text widget
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateDirtyFlag(
    TkSharedText *sharedTextPtr)/* Information about text widget. */
{
    int oldDirtyFlag;

    /*
     * If we've been forced to be dirty, we stay dirty (until explicitly
     * reset, of course).
     */

    if (sharedTextPtr->dirtyMode == TK_TEXT_DIRTY_FIXED) {
	return;
    }

    if (sharedTextPtr->isDirty < 0
	    && sharedTextPtr->dirtyMode == TK_TEXT_DIRTY_NORMAL) {
	/*
	 * If dirty flag is negative, only redo operations can make it zero
	 * again. If we do a normal operation, it can never become zero any
	 * more (other than by explicit reset).
	 */

	sharedTextPtr->dirtyMode = TK_TEXT_DIRTY_FIXED;
	return;
    }

    oldDirtyFlag = sharedTextPtr->isDirty;
    if (sharedTextPtr->dirtyMode == TK_TEXT_DIRTY_UNDO) {
	sharedTextPtr->isDirty--;
    } else {
	sharedTextPtr->isDirty++;
    }

    if (sharedTextPtr->isDirty == 0 || oldDirtyFlag == 0) {
	GenerateModifiedEvent(sharedTextPtr->peers);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextRunAfterSyncCmd --
 *
 *	This function is called by the event loop and executes the command
 *      scheduled by [.text sync -command $cmd].
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Anything may happen, depending on $cmd contents.
 *
 *----------------------------------------------------------------------
 */

void
TkTextRunAfterSyncCmd(
    ClientData clientData)		/* Information about text widget. */
{
    register TkText *textPtr = (TkText *) clientData;
    int code;

    if ((textPtr->tkwin == NULL) || (textPtr->flags & DESTROYED)) {
	/*
	* The widget has been deleted. Don't do anything.
	*/

	if (textPtr->refCount-- <= 1) {
	    ckfree((char *) textPtr);
	}
	return;
    }

    Tcl_Preserve((ClientData) textPtr->interp);
    code = Tcl_EvalObjEx(textPtr->interp, textPtr->afterSyncCmd, TCL_EVAL_GLOBAL);
    if (code == TCL_ERROR) {
	Tcl_AddErrorInfo(textPtr->interp, "\n    (text sync)");
	Tcl_BackgroundError(textPtr->interp);
    }
    Tcl_Release((ClientData) textPtr->interp);
    Tcl_DecrRefCount(textPtr->afterSyncCmd);
    textPtr->afterSyncCmd = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * SearchPerform --
 *
 *	Overall control of search process. Is given a pattern, a starting
 *	index and an ending index, and attempts to perform a search. This
 *	function is actually completely independent of Tk, and could in the
 *	future be split off.
 *
 * Results:
 *	Standard Tcl result code. In particular, if fromPtr or toPtr are not
 *	considered valid by the 'lineIndexProc', an error will be thrown and
 *	no search performed.
 *
 * Side effects:
 *	See 'SearchCore'.
 *
 *----------------------------------------------------------------------
 */

static int
SearchPerform(
    Tcl_Interp *interp,		/* For error messages. */
    SearchSpec *searchSpecPtr,	/* Search parameters. */
    Tcl_Obj *patObj,		/* Contains an exact string or a regexp
				 * pattern. Must have a refCount > 0. */
    Tcl_Obj *fromPtr,		/* Contains information describing the first
				 * index. */
    Tcl_Obj *toPtr)		/* NULL or information describing the last
				 * index. */
{
    /*
     * Find the starting line and starting offset (measured in Unicode chars
     * for regexp search, utf-8 bytes for exact search).
     */

    if (searchSpecPtr->lineIndexProc(interp, fromPtr, searchSpecPtr,
	    &searchSpecPtr->startLine,
	    &searchSpecPtr->startOffset) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Find the optional end location, similarly.
     */

    if (toPtr != NULL) {
	const TkTextIndex *indexToPtr, *indexFromPtr;
	TkText *textPtr = searchSpecPtr->clientData;

	indexToPtr = TkTextGetIndexFromObj(interp, textPtr, toPtr);
	if (indexToPtr == NULL) {
	    return TCL_ERROR;
	}
	indexFromPtr = TkTextGetIndexFromObj(interp, textPtr, fromPtr);

	/*
	 * Check for any empty search range here. It might be better in the
	 * future to embed that in SearchCore (whose default behaviour is to
	 * wrap when given a negative search range).
	 */

	if (TkTextIndexCmp(indexFromPtr, indexToPtr) ==
		(searchSpecPtr->backwards ? -1 : 1)) {
	    return TCL_OK;
	}

	if (searchSpecPtr->lineIndexProc(interp, toPtr, searchSpecPtr,
		&searchSpecPtr->stopLine,
		&searchSpecPtr->stopOffset) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	searchSpecPtr->stopLine = -1;
    }

    /*
     * Scan through all of the lines of the text circularly, starting at the
     * given index. 'patObj' is the pattern which may be an exact string or a
     * regexp pattern depending on the flags in searchSpecPtr.
     */

    return SearchCore(interp, searchSpecPtr, patObj);
}

/*
 *----------------------------------------------------------------------
 *
 * SearchCore --
 *
 *	The core of the search function. This function is actually completely
 *	independent of Tk, and could in the future be split off.
 *
 *	The function assumes regexp-based searches operate on Unicode strings,
 *	and exact searches on utf-8 strings. Therefore the 'foundMatchProc'
 *	and 'addLineProc' need to be aware of this distinction.
 *
 * Results:
 *	Standard Tcl result code.
 *
 * Side effects:
 *	Only those of the 'searchSpecPtr->foundMatchProc' which is called
 *	whenever a match is found.
 *
 *	Note that the way matching across multiple lines is implemented, we
 *	start afresh with each line we have available, even though we may
 *	already have examined the contents of that line (and further ones) if
 *	we were attempting a multi-line match using the previous line. This
 *	means there may be ways to speed this up a lot by not throwing away
 *	all the multi-line information one has accumulated. Profiling should
 *	be done to see where the bottlenecks lie before attempting this,
 *	however. We would also need to be very careful such optimisation keep
 *	within the specified search bounds.
 *
 *----------------------------------------------------------------------
 */

static int
SearchCore(
    Tcl_Interp *interp,		/* For error messages. */
    SearchSpec *searchSpecPtr,	/* Search parameters. */
    Tcl_Obj *patObj)		/* Contains an exact string or a regexp
				 * pattern. Must have a refCount > 0. */
{
    /*
     * For exact searches these are utf-8 char* offsets, for regexp searches
     * they are Unicode char offsets.
     */

    int firstOffset, lastOffset, matchOffset, matchLength;
    int passes;
    int lineNum = searchSpecPtr->startLine;
    int code = TCL_OK;
    Tcl_Obj *theLine;
    int alreadySearchOffset = -1;

    const char *pattern = NULL;	/* For exact searches only. */
    int firstNewLine = -1; 	/* For exact searches only. */
    Tcl_RegExp regexp = NULL;	/* For regexp searches only. */

    /*
     * These items are for backward regexp searches only. They are for two
     * purposes: to allow us to report backwards matches in the correct order,
     * even though the implementation uses repeated forward searches; and to
     * provide for overlap checking between backwards matches on different
     * text lines.
     */

#define LOTS_OF_MATCHES 20
    int matchNum = LOTS_OF_MATCHES;
    int smArray[2 * LOTS_OF_MATCHES];
    int *storeMatch = smArray;
    int *storeLength = smArray + LOTS_OF_MATCHES;
    int lastBackwardsLineMatch = -1;
    int lastBackwardsMatchOffset = -1;

    if (searchSpecPtr->exact) {
	/*
	 * Convert the pattern to lower-case if we're supposed to ignore case.
	 */

	if (searchSpecPtr->noCase) {
	    patObj = Tcl_DuplicateObj(patObj);

	    /*
	     * This can change the length of the string behind the object's
	     * back, so ensure it is correctly synchronised.
	     */

	    Tcl_SetObjLength(patObj, Tcl_UtfToLower(Tcl_GetString(patObj)));
	}
    } else {
	/*
	 * Compile the regular expression. We want '^$' to match after and
	 * before \n respectively, so use the TCL_REG_NLANCH flag.
	 */

	regexp = Tcl_GetRegExpFromObj(interp, patObj,
		(searchSpecPtr->noCase ? TCL_REG_NOCASE : 0)
		| (searchSpecPtr->noLineStop ? 0 : TCL_REG_NLSTOP)
		| TCL_REG_ADVANCED | TCL_REG_CANMATCH | TCL_REG_NLANCH);
	if (regexp == NULL) {
	    return TCL_ERROR;
	}
    }

    /*
     * For exact strings, we want to know where the first newline is, and we
     * will also use this as a flag to test whether it is even possible to
     * match the pattern on a single line. If not we will have to search
     * across multiple lines.
     */

    if (searchSpecPtr->exact) {
	const char *nl;

	/*
	 * We only need to set the matchLength once for exact searches, and we
	 * do it here. It is also used below as the actual pattern length, so
	 * it has dual purpose.
	 */

	pattern = Tcl_GetString(patObj);
	matchLength = patObj->length;
	nl = strchr(pattern, '\n');

	/*
	 * If there is no newline, or it is the very end of the string, then
	 * we don't need any special treatment, since single-line matching
	 * will work fine.
	 */

	if (nl != NULL && nl[1] != '\0') {
	    firstNewLine = (nl - pattern);
	}
    } else {
	matchLength = 0;	/* Only needed to prevent compiler warnings. */
    }

    /*
     * Keep a reference here, so that we can be sure the object doesn't
     * disappear behind our backs and invalidate its contents which we are
     * using.
     */

    Tcl_IncrRefCount(patObj);

    /*
     * For building up the current line being checked.
     */

    theLine = Tcl_NewObj();
    Tcl_IncrRefCount(theLine);

    for (passes = 0; passes < 2; ) {
	ClientData lineInfo;
	int linesSearched = 1;
	int extraLinesSearched = 0;

	if (lineNum >= searchSpecPtr->numLines) {
	    /*
	     * Don't search the dummy last line of the text.
	     */

	    goto nextLine;
	}

	/*
	 * Extract the text from the line, storing its length in 'lastOffset'
	 * (in bytes if exact, chars if regexp), since obviously the length is
	 * the maximum offset at which it is possible to find something on
	 * this line, which is what 'lastOffset' represents.
	 */

	lineInfo = searchSpecPtr->addLineProc(lineNum, searchSpecPtr, theLine,
		&lastOffset, &linesSearched);

	if (lineInfo == NULL) {
	    /*
	     * This should not happen, since 'lineNum' should be valid in the
	     * call above. However, let's try to be flexible and not cause a
	     * crash below.
	     */

	    goto nextLine;
	}

	if (lineNum == searchSpecPtr->stopLine && searchSpecPtr->backwards) {
	    firstOffset = searchSpecPtr->stopOffset;
	} else {
	    firstOffset = 0;
	}

	if (alreadySearchOffset >= 0) {
	    if (searchSpecPtr->backwards) {
		if (alreadySearchOffset < lastOffset) {
		    lastOffset = alreadySearchOffset;
		}
	    } else {
		if (alreadySearchOffset > firstOffset) {
		    firstOffset = alreadySearchOffset;
		}
	    }
	    alreadySearchOffset = -1;
	}

	if (lineNum == searchSpecPtr->startLine) {
	    /*
	     * The starting line is tricky: the first time we see it we check
	     * one part of the line, and the second pass through we check the
	     * other part of the line.
	     */

	    passes++;
	    if ((passes == 1) ^ searchSpecPtr->backwards) {
		/*
		 * Forward search and first pass, or backward search and
		 * second pass.
		 *
		 * Only use the last part of the line.
		 */

		if (searchSpecPtr->startOffset > firstOffset) {
		    firstOffset = searchSpecPtr->startOffset;
		}
		if ((firstOffset >= lastOffset)
		    && ((lastOffset != 0) || searchSpecPtr->exact)) {
		    goto nextLine;
		}
	    } else {
		/*
		 * Use only the first part of the line.
		 */

		if (searchSpecPtr->startOffset < lastOffset) {
		    lastOffset = searchSpecPtr->startOffset;
		}
	    }
	}

	/*
	 * Check for matches within the current line 'lineNum'. If so, and if
	 * we're searching backwards or for all matches, repeat the search
	 * until we find the last match in the line. The 'lastOffset' is one
	 * beyond the last position in the line at which a match is allowed to
	 * begin.
	 */

	matchOffset = -1;

	if (searchSpecPtr->exact) {
	    int maxExtraLines = 0;
	    const char *startOfLine = Tcl_GetString(theLine);

	    CLANG_ASSERT(pattern);
	    do {
		int ch;
		const char *p;
		int lastFullLine = lastOffset;

		if (firstNewLine == -1) {
		    if (searchSpecPtr->strictLimits
			    && (firstOffset + matchLength > lastOffset)) {
			/*
			 * Not enough characters to match.
			 */

			break;
		    }

		    /*
		     * Single line matching. We want to scan forwards or
		     * backwards as appropriate.
		     */

		    if (searchSpecPtr->backwards) {
			/*
			 * Search back either from the previous match or from
			 * 'startOfLine + lastOffset - 1' until we find a
			 * match.
			 */

			const char c = matchLength ? pattern[0] : '\0';

			if (alreadySearchOffset >= 0) {
			    p = startOfLine + alreadySearchOffset;
			    alreadySearchOffset = -1;
			} else {
			    p = startOfLine + lastOffset -1;
			}
			while (p >= startOfLine + firstOffset) {
			    if (matchLength == 0 || (p[0] == c && !strncmp(
				     p, pattern, (size_t) matchLength))) {
				goto backwardsMatch;
			    }
			    p--;
			}
			break;
		    } else {
			p = strstr(startOfLine + firstOffset, pattern);
		    }
		    if (p == NULL) {
			/*
			 * Single line match failed.
			 */

			break;
		    }
		} else if (firstNewLine >= (lastOffset - firstOffset)) {
		    /*
		     * Multi-line match, but not enough characters to match.
		     */

		    break;
		} else {
		    /*
		     * Multi-line match has only one possible match position,
		     * because we know where the '\n' is.
		     */

		    p = startOfLine + lastOffset - firstNewLine - 1;
		    if (strncmp(p, pattern, (unsigned) firstNewLine + 1)) {
			/*
			 * No match.
			 */

			break;
		    } else {
			int extraLines = 1;

			/*
			 * If we find a match that overlaps more than one
			 * line, we will use this value to determine the first
			 * allowed starting offset for the following search
			 * (to avoid overlapping results).
			 */

			int lastTotal = lastOffset;
			int skipFirst = lastOffset - firstNewLine -1;

			/*
			 * We may be able to match if given more text. The
			 * following 'while' block handles multi-line exact
			 * searches.
			 */

			while (1) {
			    lastFullLine = lastTotal;

			    if (lineNum+extraLines>=searchSpecPtr->numLines) {
				p = NULL;
				break;
			    }

			    /*
			     * Only add the line if we haven't already done so
			     * already.
			     */

			    if (extraLines > maxExtraLines) {
				if (searchSpecPtr->addLineProc(lineNum
					+ extraLines, searchSpecPtr, theLine,
					&lastTotal, &extraLines) == NULL) {
				    p = NULL;
				    if (!searchSpecPtr->backwards) {
					extraLinesSearched = extraLines;
				    }
				    break;
				}
				maxExtraLines = extraLines;
			    }

			    startOfLine = Tcl_GetString(theLine);
			    p = startOfLine + skipFirst;

			    /*
			     * Use the fact that 'matchLength = patLength' for
			     * exact searches.
			     */

			    if ((lastTotal - skipFirst) >= matchLength) {
				/*
				 * We now have enough text to match, so we
				 * make a final test and break whatever the
				 * result.
				 */

				if (strncmp(p,pattern,(size_t)matchLength)) {
				    p = NULL;
				}
				break;
			    } else {
				/*
				 * Not enough text yet, but check the prefix.
				 */

				if (strncmp(p, pattern,
					(unsigned)(lastTotal - skipFirst))) {
				    p = NULL;
				    break;
				}

				/*
				 * The prefix matches, so keep looking.
				 */
			    }
			    extraLines++;
			}
			/*
			 * If we reach here, with p != NULL, we've found a
			 * multi-line match, else we started a multi-match but
			 * didn't finish it off, so we go to the next line.
			 */

			if (p == NULL) {
			    break;
			}

			/*
			 * We've found a multi-line match.
			 */

			if (extraLines > 0) {
			    extraLinesSearched = extraLines - 1;
			}
		    }
		}

	    backwardsMatch:
		if ((p - startOfLine) >= lastOffset) {
		    break;
		}

		/*
		 * Remember the match.
		 */

		matchOffset = p - startOfLine;

		if (searchSpecPtr->all &&
			!searchSpecPtr->foundMatchProc(lineNum, searchSpecPtr,
			lineInfo, theLine, matchOffset, matchLength)) {
		    /*
		     * We reached the end of the search.
		     */

		    goto searchDone;
		}

		if (!searchSpecPtr->overlap) {
		    if (searchSpecPtr->backwards) {
			alreadySearchOffset = p - startOfLine;
			if (firstNewLine != -1) {
			    break;
			} else {
			    alreadySearchOffset -= (matchLength ? matchLength : 1);
                            if (alreadySearchOffset < 0) {
                                break;
                            }
			}
		    } else {
                        firstOffset = matchLength ? p - startOfLine + matchLength
                                                  : p - startOfLine + 1;
			if (firstOffset >= lastOffset) {
			    /*
			     * Now, we have to be careful not to find
			     * overlapping matches either on the same or
			     * following lines. Assume that if we did find
			     * something, it goes until the last extra line we
			     * added.
			     *
			     * We can break out of the loop, since we know no
			     * more will be found.
			     */

			    if (!searchSpecPtr->backwards) {
				alreadySearchOffset =
					firstOffset - lastFullLine;
				break;
			    }
			}
		    }
		} else {
		    if (searchSpecPtr->backwards) {
			alreadySearchOffset = p - startOfLine - 1;
			if (alreadySearchOffset < 0) {
			    break;
			}
		    } else {
			firstOffset = p - startOfLine +
				TkUtfToUniChar(startOfLine+matchOffset,&ch);
		    }
		}
	    } while (searchSpecPtr->all);
	} else {
	    int maxExtraLines = 0;
	    int matches = 0;
	    int lastNonOverlap = -1;

	    do {
		Tcl_RegExpInfo info;
		int match;
		int lastFullLine = lastOffset;

		match = Tcl_RegExpExecObj(interp, regexp, theLine,
			firstOffset, 1, (firstOffset>0 ? TCL_REG_NOTBOL : 0));
		if (match < 0) {
		    code = TCL_ERROR;
		    goto searchDone;
		}
		Tcl_RegExpGetInfo(regexp, &info);

		/*
		 * If we don't have a match, or if we do, but it extends to
		 * the end of the line, we must try to add more lines to get a
		 * full greedy match.
		 */

		if (!match ||
			((info.extendStart == info.matches[0].start)
			&& (info.matches[0].end == lastOffset-firstOffset))) {
		    int extraLines = 0;
		    int prevFullLine;

		    /*
		     * If we find a match that overlaps more than one line, we
		     * will use this value to determine the first allowed
		     * starting offset for the following search (to avoid
		     * overlapping results).
		     */

		    int lastTotal = lastOffset;

		    if ((lastBackwardsLineMatch != -1)
			    && (lastBackwardsLineMatch == (lineNum + 1))) {
			lastNonOverlap = lastTotal;
		    }

		    if (info.extendStart < 0) {
			/*
			 * No multi-line match is possible.
			 */

			break;
		    }

		    /*
		     * We may be able to match if given more text. The
		     * following 'while' block handles multi-line regexp
		     * searches.
		     */

		    while (1) {
			prevFullLine = lastTotal;

			/*
			 * Move firstOffset to first possible start.
			 */

			if (!match) {
			    firstOffset += info.extendStart;
			}
			if (firstOffset >= lastOffset) {
			    /*
			     * We're being told that the only possible new
			     * match is starting after the end of the line.
			     * But, that is the next line which we will handle
			     * when we look at that line.
			     */

			    if (!match && !searchSpecPtr->backwards
				    && (firstOffset == 0)) {
				extraLinesSearched = extraLines;
			    }
			    break;
			}

			if (lineNum + extraLines >= searchSpecPtr->numLines) {
			    break;
			}

			/*
			 * Add next line, provided we haven't already done so.
			 */

			if (extraLines > maxExtraLines) {
			    if (searchSpecPtr->addLineProc(lineNum
				    + extraLines, searchSpecPtr, theLine,
				    &lastTotal, &extraLines) == NULL) {
				/*
				 * There are no more acceptable lines, so we
				 * can say we have searched all of these.
				 */

				if (!match && !searchSpecPtr->backwards) {
				    extraLinesSearched = extraLines;
				}
				break;
			    }

			    maxExtraLines = extraLines;
			    if ((lastBackwardsLineMatch != -1)
				    && (lastBackwardsLineMatch
				    == (lineNum + extraLines + 1))) {
				lastNonOverlap = lastTotal;
			    }
			}

			match = Tcl_RegExpExecObj(interp, regexp, theLine,
				firstOffset, 1,
				((firstOffset > 0) ? TCL_REG_NOTBOL : 0));
			if (match < 0) {
			    code = TCL_ERROR;
			    goto searchDone;
			}
			Tcl_RegExpGetInfo(regexp, &info);

			/*
			 * Unfortunately there are bugs in Tcl's regexp
			 * library, which tells us that info.extendStart is
			 * zero when it should not be (should be -1), which
			 * makes our task a bit more complicated here. We
			 * check if there was a match, and the end of the
			 * match leaves an entire extra line unmatched, then
			 * we stop searching. Clearly it still might sometimes
			 * be possible to add more text and match again, but
			 * Tcl's regexp library doesn't tell us that.
			 *
			 * This means we often add and search one more line
			 * than might be necessary if Tcl were able to give us
			 * a correct value of info.extendStart under all
			 * circumstances.
			 */

			if ((match &&
				firstOffset+info.matches[0].end != lastTotal &&
				firstOffset+info.matches[0].end < prevFullLine)
				|| info.extendStart < 0) {
			    break;
			}

			/*
			 * If there is a match, but that match starts after
			 * the end of the first line, then we'll handle that
			 * next time around, when we're actually looking at
			 * that line.
			 */

			if (match && (info.matches[0].start >= lastOffset)) {
			    break;
			}
			if (match && ((firstOffset + info.matches[0].end)
				>= prevFullLine)) {
			    if (extraLines > 0) {
				extraLinesSearched = extraLines - 1;
			    }
			    lastFullLine = prevFullLine;
			}

			/*
			 * The prefix matches, so keep looking.
			 */

			extraLines++;
		    }

		    /*
		     * If we reach here with 'match == 1', we've found a
		     * multi-line match, which we will record in the code
		     * which follows directly else we started a multi-line
		     * match but didn't finish it off, so we go to the next
		     * line.
		     */

		    if (!match) {
			/*
			 * Here is where we could perform an optimisation,
			 * since we have already retrieved the contents of the
			 * next line (perhaps many more), so we shouldn't
			 * really throw it all away and start again. This
			 * could be particularly important for complex regexp
			 * searches.
			 *
			 * This 'break' will take us to just before the
			 * 'nextLine:' below.
			 */

			break;
		    }

		    if (lastBackwardsLineMatch != -1) {
			if ((lineNum + linesSearched + extraLinesSearched)
				== lastBackwardsLineMatch) {
			    /*
			     * Possible overlap or inclusion.
			     */

			    int thisOffset = firstOffset + info.matches[0].end
				    - info.matches[0].start;

			    if (lastNonOverlap != -1) {
				/*
				 * Possible overlap or enclosure.
				 */

				if (thisOffset-lastNonOverlap >=
					lastBackwardsMatchOffset+matchLength){
				    /*
				     * Totally encloses previous match, so
				     * forget the previous match.
				     */

				    lastBackwardsLineMatch = -1;
				} else if ((thisOffset - lastNonOverlap)
					> lastBackwardsMatchOffset) {
				    /*
				     * Overlap. Previous match is ok, and the
				     * current match is only ok if we are
				     * searching with -overlap.
				     */

				    if (searchSpecPtr->overlap) {
					goto recordBackwardsMatch;
				    } else {
					match = 0;
					break;
				    }
				} else {
				    /*
				     * No overlap, although the same line was
				     * reached.
				     */

				    goto recordBackwardsMatch;
				}
			    } else {
				/*
				 * No overlap.
				 */

				goto recordBackwardsMatch;
			    }
			} else if (lineNum+linesSearched+extraLinesSearched
				< lastBackwardsLineMatch) {
			    /*
			     * No overlap.
			     */

			    goto recordBackwardsMatch;
			} else {
			    /*
			     * Totally enclosed.
			     */

			    lastBackwardsLineMatch = -1;
			}
		    }

		} else {
		    /*
		     * Matched in a single line.
		     */

		    if (lastBackwardsLineMatch != -1) {
		    recordBackwardsMatch:
			searchSpecPtr->foundMatchProc(lastBackwardsLineMatch,
				searchSpecPtr, NULL, NULL,
				lastBackwardsMatchOffset, matchLength);
			lastBackwardsLineMatch = -1;
			if (!searchSpecPtr->all) {
			    goto searchDone;
			}
		    }
		}

		firstOffset += info.matches[0].start;
		if (firstOffset >= lastOffset) {
		    break;
		}

		/*
		 * Update our local variables with the match, if we haven't
		 * yet found anything, or if we're doing '-all' or
		 * '-backwards' _and_ this match isn't fully enclosed in the
		 * previous match.
		 */

		if (matchOffset == -1 ||
			((searchSpecPtr->all || searchSpecPtr->backwards)
			&& ((firstOffset < matchOffset)
			|| ((firstOffset + info.matches[0].end
				- info.matches[0].start)
				> (matchOffset + matchLength))))) {

		    matchOffset = firstOffset;
		    matchLength = info.matches[0].end - info.matches[0].start;

		    if (searchSpecPtr->backwards) {
			/*
			 * To get backwards searches in the correct order, we
			 * must store them away here.
			 */

			if (matches == matchNum) {
			    /*
			     * We've run out of space in our normal store, so
			     * we must allocate space for these backwards
			     * matches on the heap.
			     */

			    int *newArray =
				    ckalloc(4 * matchNum * sizeof(int));
			    memcpy(newArray, storeMatch, matchNum*sizeof(int));
			    memcpy(newArray + 2*matchNum, storeLength,
				    matchNum * sizeof(int));
			    if (storeMatch != smArray) {
				ckfree(storeMatch);
			    }
			    matchNum *= 2;
			    storeMatch = newArray;
			    storeLength = newArray + matchNum;
			}
			storeMatch[matches] = matchOffset;
			storeLength[matches] = matchLength;
			matches++;
		    } else {
			/*
			 * Now actually record the match, but only if we are
			 * doing an '-all' search.
			 */

			if (searchSpecPtr->all &&
				!searchSpecPtr->foundMatchProc(lineNum,
				searchSpecPtr, lineInfo, theLine, matchOffset,
				matchLength)) {
			    /*
			     * We reached the end of the search.
			     */

			    goto searchDone;
			}
		    }

		    /*
		     * For forward matches, unless we allow overlaps, we move
		     * this on by the length of the current match so that we
		     * explicitly disallow overlapping matches.
		     */

		    if (matchLength > 0 && !searchSpecPtr->overlap
			    && !searchSpecPtr->backwards) {
			firstOffset += matchLength;
			if (firstOffset >= lastOffset) {
			    /*
			     * Now, we have to be careful not to find
			     * overlapping matches either on the same or
			     * following lines. Assume that if we did find
			     * something, it goes until the last extra line we
			     * added.
			     *
			     * We can break out of the loop, since we know no
			     * more will be found.
			     */

			    alreadySearchOffset = firstOffset - lastFullLine;
			    break;
			}

			/*
			 * We'll add this on again just below.
			 */

			firstOffset --;
		    }
		}

		/*
		 * Move the starting point on, in case we are doing repeated
		 * or backwards searches (for the latter, we actually do
		 * repeated forward searches).
		 */

		firstOffset++;
	    } while (searchSpecPtr->backwards || searchSpecPtr->all);

	    if (matches > 0) {
		/*
		 * Now we have all the matches in our array, but not stored
		 * with 'foundMatchProc' yet.
		 */

		matches--;
		matchOffset = storeMatch[matches];
		matchLength = storeLength[matches];
		while (--matches >= 0) {
		    if (lineNum == searchSpecPtr->stopLine) {
			/*
			 * It appears as if a condition like:
			 *
			 * if (storeMatch[matches]<searchSpecPtr->stopOffset)
			 *	break;
			 *
			 * might be needed here, but no test case has been
			 * found which would exercise such a problem.
			 */
		    }
		    if (storeMatch[matches] + storeLength[matches]
			    >= matchOffset + matchLength) {
			/*
			 * The new match totally encloses the previous one, so
			 * we overwrite the previous one.
			 */

			matchOffset = storeMatch[matches];
			matchLength = storeLength[matches];
			continue;
		    }
		    if (!searchSpecPtr->overlap) {
			if (storeMatch[matches] + storeLength[matches]
				> matchOffset) {
			    continue;
			}
		    }
		    searchSpecPtr->foundMatchProc(lineNum, searchSpecPtr,
			    lineInfo, theLine, matchOffset, matchLength);
		    if (!searchSpecPtr->all) {
			goto searchDone;
		    }
		    matchOffset = storeMatch[matches];
		    matchLength = storeLength[matches];
		}
		if (searchSpecPtr->all && matches > 0) {
		    /*
		     * We only need to do this for the '-all' case, because
		     * just below we will call the foundMatchProc for the
		     * non-all case.
		     */

		    searchSpecPtr->foundMatchProc(lineNum, searchSpecPtr,
			    lineInfo, theLine, matchOffset, matchLength);
		} else {
		    lastBackwardsLineMatch = lineNum;
		    lastBackwardsMatchOffset = matchOffset;
		}
	    }
	}

	/*
	 * If the 'all' flag is set, we will already have stored all matches,
	 * so we just proceed to the next line.
	 *
	 * If not, and there is a match we need to store that information and
	 * we are done.
	 */

	if ((lastBackwardsLineMatch == -1) && (matchOffset >= 0)
		&& !searchSpecPtr->all) {
	    searchSpecPtr->foundMatchProc(lineNum, searchSpecPtr, lineInfo,
		    theLine, matchOffset, matchLength);
	    goto searchDone;
	}

	/*
	 * Go to the next (or previous) line;
	 */

    nextLine:
	linesSearched += extraLinesSearched;

	while (linesSearched-- > 0) {
	    /*
	     * If we have just completed the 'stopLine', we are done.
	     */

	    if (lineNum == searchSpecPtr->stopLine) {
		goto searchDone;
	    }

	    if (searchSpecPtr->backwards) {
		lineNum--;

		if (lastBackwardsLineMatch != -1
			&& ((lineNum < 0)
			|| (lineNum + 2 < lastBackwardsLineMatch))) {
		    searchSpecPtr->foundMatchProc(lastBackwardsLineMatch,
			    searchSpecPtr, NULL, NULL,
			    lastBackwardsMatchOffset, matchLength);
		    lastBackwardsLineMatch = -1;
		    if (!searchSpecPtr->all) {
			goto searchDone;
		    }
		}

		if (lineNum < 0) {
		    lineNum = searchSpecPtr->numLines-1;
		}
		if (!searchSpecPtr->exact) {
		    /*
		     * The 'exact' search loops above are designed to give us
		     * an accurate picture of the number of lines which we can
		     * skip here. For 'regexp' searches, on the other hand,
		     * which can match potentially variable lengths, we cannot
		     * skip multiple lines when searching backwards. Therefore
		     * we only allow one line to be skipped here.
		     */

		    break;
		}
	    } else {
		lineNum++;
		if (lineNum >= searchSpecPtr->numLines) {
		    lineNum = 0;
		}
	    }
	    if (lineNum == searchSpecPtr->startLine && linesSearched > 0) {
		/*
		 * We've just searched all the way round and have gone right
		 * through the start line without finding anything in the last
		 * attempt.
		 */

		break;
	    }
	}

	Tcl_SetObjLength(theLine, 0);
    }
  searchDone:

    if (lastBackwardsLineMatch != -1) {
	searchSpecPtr->foundMatchProc(lastBackwardsLineMatch, searchSpecPtr,
		NULL, NULL, lastBackwardsMatchOffset, matchLength);
    }

    /*
     * Free up the cached line and pattern.
     */

    Tcl_DecrRefCount(theLine);
    Tcl_DecrRefCount(patObj);

    /*
     * Free up any extra space we allocated.
     */

    if (storeMatch != smArray) {
	ckfree(storeMatch);
    }

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * GetLineStartEnd -
 *
 *	Converts an internal TkTextLine ptr into a Tcl string obj containing
 *	the line number. (Handler for the 'line' configuration option type.)
 *
 * Results:
 *	Tcl_Obj containing the string representation of the line value.
 *
 * Side effects:
 *	Creates a new Tcl_Obj.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
GetLineStartEnd(
    ClientData clientData,
    Tk_Window tkwin,
    char *recordPtr,		/* Pointer to widget record. */
    int internalOffset)		/* Offset within *recordPtr containing the
				 * line value. */
{
    TkTextLine *linePtr = *(TkTextLine **)(recordPtr + internalOffset);

    if (linePtr == NULL) {
	return Tcl_NewObj();
    }
    return Tcl_NewIntObj(1 + TkBTreeLinesTo(NULL, linePtr));
}

/*
 *----------------------------------------------------------------------
 *
 * SetLineStartEnd --
 *
 *	Converts a Tcl_Obj representing a widget's (start or end) line into a
 *	TkTextLine* value. (Handler for the 'line' configuration option type.)
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	May store the TkTextLine* value into the internal representation
 *	pointer. May change the pointer to the Tcl_Obj to NULL to indicate
 *	that the specified string was empty and that is acceptable.
 *
 *----------------------------------------------------------------------
 */

static int
SetLineStartEnd(
    ClientData clientData,
    Tcl_Interp *interp,		/* Current interp; may be used for errors. */
    Tk_Window tkwin,		/* Window for which option is being set. */
    Tcl_Obj **value,		/* Pointer to the pointer to the value object.
				 * We use a pointer to the pointer because we
				 * may need to return a value (NULL). */
    char *recordPtr,		/* Pointer to storage for the widget record. */
    int internalOffset,		/* Offset within *recordPtr at which the
				 * internal value is to be stored. */
    char *oldInternalPtr,	/* Pointer to storage for the old value. */
    int flags)			/* Flags for the option, set Tk_SetOptions. */
{
    TkTextLine *linePtr = NULL;
    char *internalPtr;
    TkText *textPtr = (TkText *) recordPtr;

    if (internalOffset >= 0) {
	internalPtr = recordPtr + internalOffset;
    } else {
	internalPtr = NULL;
    }

    if (flags & TK_OPTION_NULL_OK && ObjectIsEmpty(*value)) {
	*value = NULL;
    } else {
	int line;

	if (Tcl_GetIntFromObj(interp, *value, &line) != TCL_OK) {
	    return TCL_ERROR;
	}
	linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, NULL, line-1);
    }

    if (internalPtr != NULL) {
	*((TkTextLine **) oldInternalPtr) = *((TkTextLine **) internalPtr);
	*((TkTextLine **) internalPtr) = linePtr;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RestoreLineStartEnd --
 *
 *	Restore a line option value from a saved value. (Handler for the
 *	'line' configuration option type.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Restores the old value.
 *
 *----------------------------------------------------------------------
 */

static void
RestoreLineStartEnd(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr,		/* Pointer to storage for value. */
    char *oldInternalPtr)	/* Pointer to old value. */
{
    *(TkTextLine **)internalPtr = *(TkTextLine **)oldInternalPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ObjectIsEmpty --
 *
 *	This function tests whether the string value of an object is empty.
 *
 * Results:
 *	The return value is 1 if the string value of objPtr has length zero,
 *	and 0 otherwise.
 *
 * Side effects:
 *	May cause object shimmering, since this function can force a
 *	conversion to a string object.
 *
 *----------------------------------------------------------------------
 */

static int
ObjectIsEmpty(
    Tcl_Obj *objPtr)		/* Object to test. May be NULL. */
{
    if (objPtr == NULL) {
	return 1;
    }
    if (objPtr->bytes == NULL) {
	Tcl_GetString(objPtr);
    }
    return (objPtr->length == 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpTesttextCmd --
 *
 *	This function implements the "testtext" command. It provides a set of
 *	functions for testing text widgets and the associated functions in
 *	tkText*.c.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Depends on option; see below.
 *
 *----------------------------------------------------------------------
 */

int
TkpTesttextCmd(
    ClientData clientData,	/* Main window for application. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument strings. */
{
    TkText *textPtr;
    size_t len;
    int lineIndex, byteIndex, byteOffset;
    TkTextIndex index;
    char buf[64];
    Tcl_CmdInfo info;

    if (objc < 3) {
	return TCL_ERROR;
    }

    if (Tcl_GetCommandInfo(interp, Tcl_GetString(objv[1]), &info) == 0) {
	return TCL_ERROR;
    }
    textPtr = info.objClientData;
    len = strlen(Tcl_GetString(objv[2]));
    if (strncmp(Tcl_GetString(objv[2]), "byteindex", len) == 0) {
	if (objc != 5) {
	    return TCL_ERROR;
	}
	lineIndex = atoi(Tcl_GetString(objv[3])) - 1;
	byteIndex = atoi(Tcl_GetString(objv[4]));

	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, lineIndex,
		byteIndex, &index);
    } else if (strncmp(Tcl_GetString(objv[2]), "forwbytes", len) == 0) {
	if (objc != 5) {
	    return TCL_ERROR;
	}
	if (TkTextGetIndex(interp, textPtr, Tcl_GetString(objv[3]), &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	byteOffset = atoi(Tcl_GetString(objv[4]));
	TkTextIndexForwBytes(textPtr, &index, byteOffset, &index);
    } else if (strncmp(Tcl_GetString(objv[2]), "backbytes", len) == 0) {
	if (objc != 5) {
	    return TCL_ERROR;
	}
	if (TkTextGetIndex(interp, textPtr, Tcl_GetString(objv[3]), &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	byteOffset = atoi(Tcl_GetString(objv[4]));
	TkTextIndexBackBytes(textPtr, &index, byteOffset, &index);
    } else {
	return TCL_ERROR;
    }

    TkTextSetMark(textPtr, "insert", &index);
    TkTextPrintIndex(textPtr, &index, buf);
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s %d", buf, index.byteIndex));
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
