/*
 * tkTextDisp.c --
 *
 *	This module provides facilities to display text widgets. It is the
 *	only place where information is kept about the screen layout of text
 *	widgets. (Well, strictly, each TkTextLine and B-tree node caches its
 *	last observed pixel height, but that information originates here).
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkText.h"

#ifdef _WIN32
#include "tkWinInt.h"
#elif defined(__CYGWIN__)
#include "tkUnixInt.h"
#endif

#ifdef MAC_OSX_TK
#include "tkMacOSXInt.h"
#define OK_TO_LOG (!TkpAppIsDrawing())
#define FORCE_DISPLAY(winPtr) TkpDisplayWindow(winPtr)
#else
#define OK_TO_LOG 1
#define FORCE_DISPLAY(winPtr)
#endif

/*
 * "Calculations of line pixel heights and the size of the vertical
 * scrollbar."
 *
 * Given that tag, font and elide changes can happen to large numbers of
 * diverse chunks in a text widget containing megabytes of text, it is not
 * possible to recalculate all affected height information immediately any
 * such change takes place and maintain a responsive user-experience. Yet, for
 * an accurate vertical scrollbar to be drawn, we must know the total number
 * of vertical pixels shown on display versus the number available to be
 * displayed.
 *
 * The way the text widget solves this problem is by maintaining cached line
 * pixel heights (in the BTree for each logical line), and having asynchronous
 * timer callbacks (i) to iterate through the logical lines recalculating
 * their heights, and (ii) to recalculate the vertical scrollbar's position
 * and size.
 *
 * Typically this works well but there are some situations where the overall
 * functional design of this file causes some problems. These problems can
 * only arise because the calculations used to display lines on screen are not
 * connected to those in the iterating-line- recalculation-process.
 *
 * The reason for this disconnect is that the display calculations operate in
 * display lines, and the iteration and cache operates in logical lines.
 * Given that the display calculations both need not contain complete logical
 * lines (at top or bottom of display), and that they do not actually keep
 * track of logical lines (for simplicity of code and historical design), this
 * means a line may be known and drawn with a different pixel height to that
 * which is cached in the BTree, and this might cause some temporary
 * undesirable mismatch between display and the vertical scrollbar.
 *
 * All such mismatches should be temporary, however, since the asynchronous
 * height calculations will always catch up eventually.
 *
 * For further details see the comments before and within the following
 * functions below: LayoutDLine, AsyncUpdateLineMetrics, GetYView,
 * GetYPixelCount, TkTextUpdateOneLine, TkTextUpdateLineMetrics.
 *
 * For details of the way in which the BTree keeps track of pixel heights, see
 * tkTextBTree.c. Basically the BTree maintains two pieces of information: the
 * logical line indices and the pixel height cache.
 */

/*
 * TK_LAYOUT_WITH_BASE_CHUNKS:
 *
 *	With this macro set, collect all char chunks that have no holes
 *	between them, that are on the same line and use the same font and font
 *	size. Allocate the chars of all these chunks, the so-called "stretch",
 *	in a DString in the first chunk, the so-called "base chunk". Use the
 *	base chunk string for measuring and drawing, so that these actions are
 *	always performed with maximum context.
 *
 *	This is necessary for text rendering engines that provide ligatures
 *	and sub-pixel layout, like ATSU on Mac. If we don't do this, the
 *	measuring will change all the time, leading to an ugly "tremble and
 *	shiver" effect. This is because of the continuous splitting and
 *	re-merging of chunks that goes on in a text widget, when the cursor or
 *	the selection move.
 *
 * Side effects:
 *
 *	Memory management changes. Instead of attaching the character data to
 *	the clientData structures of the char chunks, an additional DString is
 *	used. The collection process will even lead to resizing this DString
 *	for large stretches (> TCL_DSTRING_STATIC_SIZE == 200). We could
 *	reduce the overall memory footprint by copying the result to a plain
 *	char array after the line breaking process, but that would complicate
 *	the code and make performance even worse speedwise. See also TODOs.
 *
 * TODOs:
 *
 *    -	Move the character collection process from the LayoutProc into
 *	LayoutDLine(), so that the collection can be done before actual
 *	layout. In this way measuring can look at the following text, too,
 *	right from the beginning. Memory handling can also be improved with
 *	this. Problem: We don't easily know which chunks are adjacent until
 *	all the other chunks have calculated their width. Apparently marks
 *	would return width==0. A separate char collection loop would have to
 *	know these things.
 *
 *    -	Use a new context parameter to pass the context from LayoutDLine() to
 *	the LayoutProc instead of using a global variable like now. Not
 *	pressing until the previous point gets implemented.
 */

/*
 * The following structure describes how to display a range of characters.
 * The information is generated by scanning all of the tags associated with
 * the characters and combining that with default information for the overall
 * widget. These structures form the hash keys for dInfoPtr->styleTable.
 */

typedef struct StyleValues {
    Tk_3DBorder border;		/* Used for drawing background under text.
				 * NULL means use widget background. */
    int borderWidth;		/* Width of 3-D border for background. */
    int relief;			/* 3-D relief for background. */
    Pixmap bgStipple;		/* Stipple bitmap for background. None means
				 * draw solid. */
    XColor *fgColor;		/* Foreground color for text. */
    Tk_Font tkfont;		/* Font for displaying text. */
    Pixmap fgStipple;		/* Stipple bitmap for text and other
				 * foreground stuff. None means draw solid.*/
    int justify;		/* Justification style for text. */
    int lMargin1;		/* Left margin, in pixels, for first display
				 * line of each text line. */
    int lMargin2;		/* Left margin, in pixels, for second and
				 * later display lines of each text line. */
    Tk_3DBorder lMarginColor;	/* Color of left margins (1 and 2). */
    int offset;			/* Offset in pixels of baseline, relative to
				 * baseline of line. */
    int overstrike;		/* Non-zero means draw overstrike through
				 * text. */
    XColor *overstrikeColor;	/* Foreground color for overstrike through
                                 * text. */
    int rMargin;		/* Right margin, in pixels. */
    Tk_3DBorder rMarginColor;	/* Color of right margin. */
    int spacing1;		/* Spacing above first dline in text line. */
    int spacing2;		/* Spacing between lines of dline. */
    int spacing3;		/* Spacing below last dline in text line. */
    TkTextTabArray *tabArrayPtr;/* Locations and types of tab stops (may be
				 * NULL). */
    int tabStyle;		/* One of TABULAR or WORDPROCESSOR. */
    int underline;		/* Non-zero means draw underline underneath
				 * text. */
    XColor *underlineColor;	/* Foreground color for underline underneath
                                 * text. */
    int elide;			/* Zero means draw text, otherwise not. */
    TkWrapMode wrapMode;	/* How to handle wrap-around for this tag.
				 * One of TEXT_WRAPMODE_CHAR,
				 * TEXT_WRAPMODE_NONE or TEXT_WRAPMODE_WORD.*/
} StyleValues;

/*
 * The following structure extends the StyleValues structure above with
 * graphics contexts used to actually draw the characters. The entries in
 * dInfoPtr->styleTable point to structures of this type.
 */

typedef struct TextStyle {
    int refCount;		/* Number of times this structure is
				 * referenced in Chunks. */
    GC bgGC;			/* Graphics context for background. None means
				 * use widget background. */
    GC fgGC;			/* Graphics context for foreground. */
    GC ulGC;			/* Graphics context for underline. */
    GC ovGC;			/* Graphics context for overstrike. */
    StyleValues *sValuePtr;	/* Raw information from which GCs were
				 * derived. */
    Tcl_HashEntry *hPtr;	/* Pointer to entry in styleTable. Used to
				 * delete entry. */
} TextStyle;

/*
 * The following macro determines whether two styles have the same background
 * so that, for example, no beveled border should be drawn between them.
 */

#define SAME_BACKGROUND(s1, s2) \
    (((s1)->sValuePtr->border == (s2)->sValuePtr->border) \
	&& ((s1)->sValuePtr->borderWidth == (s2)->sValuePtr->borderWidth) \
	&& ((s1)->sValuePtr->relief == (s2)->sValuePtr->relief) \
	&& ((s1)->sValuePtr->bgStipple == (s2)->sValuePtr->bgStipple))

/*
 * The following macro is used to compare two floating-point numbers to within
 * a certain degree of scale. Direct comparison fails on processors where the
 * processor and memory representations of FP numbers of a particular
 * precision is different (e.g. Intel)
 */

#define FP_EQUAL_SCALE(double1, double2, scaleFactor) \
    (fabs((double1)-(double2))*((scaleFactor)+1.0) < 0.3)

/*
 * Macros to make debugging/testing logging a little easier.
 *
 * On OSX 10.14 Drawing procedures are sometimes run because the system has
 * decided to redraw the window.  This can corrupt the data that a test is
 * trying to collect.  So we don't write to the logging variables when the
 * drawing procedure is being run that way.  Other systems can always log.
 */

#define LOG(toVar,what)							\
    if (OK_TO_LOG)							\
        Tcl_SetVar2(textPtr->interp, toVar, NULL, (what),		\
		    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT)
#define CLEAR(var)							\
    if (OK_TO_LOG)							\
	Tcl_SetVar2(interp, var, NULL, "", TCL_GLOBAL_ONLY)

/*
 * The following structure describes one line of the display, which may be
 * either part or all of one line of the text.
 */

typedef struct DLine {
    TkTextIndex index;		/* Identifies first character in text that is
				 * displayed on this line. */
    int byteCount;		/* Number of bytes accounted for by this
				 * display line, including a trailing space or
				 * newline that isn't actually displayed. */
    int logicalLinesMerged;	/* Number of extra logical lines merged into
				 * this one due to elided newlines. */
    int y;			/* Y-position at which line is supposed to be
				 * drawn (topmost pixel of rectangular area
				 * occupied by line). */
    int oldY;			/* Y-position at which line currently appears
				 * on display. This is used to move lines by
				 * scrolling rather than re-drawing. If
				 * 'flags' have the OLD_Y_INVALID bit set,
				 * then we will never examine this field
				 * (which means line isn't currently visible
				 * on display and must be redrawn). */
    int height;			/* Height of line, in pixels. */
    int baseline;		/* Offset of text baseline from y, in
				 * pixels. */
    int spaceAbove;		/* How much extra space was added to the top
				 * of the line because of spacing options.
				 * This is included in height and baseline. */
    int spaceBelow;		/* How much extra space was added to the
				 * bottom of the line because of spacing
				 * options. This is included in height. */
    Tk_3DBorder lMarginColor;	/* Background color of the area corresponding
				 * to the left margin of the display line. */
    int lMarginWidth;           /* Pixel width of the area corresponding to
                                 * the left margin. */
    Tk_3DBorder rMarginColor;	/* Background color of the area corresponding
				 * to the right margin of the display line. */
    int rMarginWidth;           /* Pixel width of the area corresponding to
                                 * the right margin. */
    int length;			/* Total length of line, in pixels. */
    TkTextDispChunk *chunkPtr;	/* Pointer to first chunk in list of all of
				 * those that are displayed on this line of
				 * the screen. */
    struct DLine *nextPtr;	/* Next in list of all display lines for this
				 * window. The list is sorted in order from
				 * top to bottom. Note: the next DLine doesn't
				 * always correspond to the next line of text:
				 * (a) can have multiple DLines for one text
				 * line (wrapping), (b) can have elided newlines,
				 * and (c) can have gaps where DLine's
				 * have been deleted because they're out of
				 * date. */
    int flags;			/* Various flag bits: see below for values. */
} DLine;

/*
 * Flag bits for DLine structures:
 *
 * HAS_3D_BORDER -		Non-zero means that at least one of the chunks
 *				in this line has a 3D border, so it
 *				potentially interacts with 3D borders in
 *				neighboring lines (see DisplayLineBackground).
 * NEW_LAYOUT -			Non-zero means that the line has been
 *				re-layed out since the last time the display
 *				was updated.
 * TOP_LINE -			Non-zero means that this was the top line in
 *				in the window the last time that the window
 *				was laid out. This is important because a line
 *				may be displayed differently if its at the top
 *				or bottom than if it's in the middle
 *				(e.g. beveled edges aren't displayed for
 *				middle lines if the adjacent line has a
 *				similar background).
 * BOTTOM_LINE -		Non-zero means that this was the bottom line
 *				in the window the last time that the window
 *				was laid out.
 * OLD_Y_INVALID -		The value of oldY in the structure is not
 *				valid or useful and should not be examined.
 *				'oldY' is only useful when the DLine is
 *				currently displayed at a different position
 *				and we wish to re-display it via scrolling, so
 *				this means the DLine needs redrawing.
 */

#define HAS_3D_BORDER	1
#define NEW_LAYOUT	2
#define TOP_LINE	4
#define BOTTOM_LINE	8
#define OLD_Y_INVALID  16

/*
 * Overall display information for a text widget:
 */

typedef struct TextDInfo {
    Tcl_HashTable styleTable;	/* Hash table that maps from StyleValues to
				 * TextStyles for this widget. */
    DLine *dLinePtr;		/* First in list of all display lines for this
				 * widget, in order from top to bottom. */
    int topPixelOffset;		/* Identifies first pixel in top display line
				 * to display in window. */
    int newTopPixelOffset;	/* Desired first pixel in top display line to
				 * display in window. */
    GC copyGC;			/* Graphics context for copying from off-
				 * screen pixmaps onto screen. */
    GC scrollGC;		/* Graphics context for copying from one place
				 * in the window to another (scrolling):
				 * differs from copyGC in that we need to get
				 * GraphicsExpose events. */
    int x;			/* First x-coordinate that may be used for
				 * actually displaying line information.
				 * Leaves space for border, etc. */
    int y;			/* First y-coordinate that may be used for
				 * actually displaying line information.
				 * Leaves space for border, etc. */
    int maxX;			/* First x-coordinate to right of available
				 * space for displaying lines. */
    int maxY;			/* First y-coordinate below available space
				 * for displaying lines. */
    int topOfEof;		/* Top-most pixel (lowest y-value) that has
				 * been drawn in the appropriate fashion for
				 * the portion of the window after the last
				 * line of the text. This field is used to
				 * figure out when to redraw part or all of
				 * the eof field. */

    /*
     * Information used for scrolling:
     */

    int newXPixelOffset;	/* Desired x scroll position, measured as the
				 * number of pixels off-screen to the left for
				 * a line with no left margin. */
    int curXPixelOffset;	/* Actual x scroll position, measured as the
				 * number of pixels off-screen to the left. */
    int maxLength;		/* Length in pixels of longest line that's
				 * visible in window (length may exceed window
				 * size). If there's no wrapping, this will be
				 * zero. */
    double xScrollFirst, xScrollLast;
				/* Most recent values reported to horizontal
				 * scrollbar; used to eliminate unnecessary
				 * reports. */
    double yScrollFirst, yScrollLast;
				/* Most recent values reported to vertical
				 * scrollbar; used to eliminate unnecessary
				 * reports. */

    /*
     * The following information is used to implement scanning:
     */

    int scanMarkXPixel;		/* Pixel index of left edge of the window when
				 * the scan started. */
    int scanMarkX;		/* X-position of mouse at time scan started. */
    int scanTotalYScroll;	/* Total scrolling (in screen pixels) that has
				 * occurred since scanMarkY was set. */
    int scanMarkY;		/* Y-position of mouse at time scan started. */

    /*
     * Miscellaneous information:
     */

    int dLinesInvalidated;	/* This value is set to 1 whenever something
				 * happens that invalidates information in
				 * DLine structures; if a redisplay is in
				 * progress, it will see this and abort the
				 * redisplay. This is needed because, for
				 * example, an embedded window could change
				 * its size when it is first displayed,
				 * invalidating the DLine that is currently
				 * being displayed. If redisplay continues, it
				 * will use freed memory and could dump
				 * core. */
    int flags;			/* Various flag values: see below for
				 * definitions. */
    /*
     * Information used to handle the asynchronous updating of the y-scrollbar
     * and the vertical height calculations:
     */

    int lineMetricUpdateEpoch;	/* Stores a number which is incremented each
				 * time the text widget changes in a
				 * significant way (e.g. resizing or
				 * geometry-influencing tag changes). */
    int currentMetricUpdateLine;/* Stores a counter which is used to iterate
				 * over the logical lines contained in the
				 * widget and update their geometry
				 * calculations, if they are out of date. */
    TkTextIndex metricIndex;	/* If the current metric update line wraps
				 * into very many display lines, then this is
				 * used to keep track of what index we've got
				 * to so far... */
    int metricPixelHeight;	/* ...and this is for the height calculation
				 * so far...*/
    int metricEpoch;		/* ...and this for the epoch of the partial
				 * calculation so it can be cancelled if
				 * things change once more. This field will be
				 * -1 if there is no long-line calculation in
				 * progress, and take a non-negative value if
				 * there is such a calculation in progress. */
    int lastMetricUpdateLine;	/* When the current update line reaches this
				 * line, we are done and should stop the
				 * asychronous callback mechanism. */
    Tcl_TimerToken lineUpdateTimer;
				/* A token pointing to the current line metric
				 * update callback. */
    Tcl_TimerToken scrollbarTimer;
				/* A token pointing to the current scrollbar
				 * update callback. */
} TextDInfo;

/*
 * In TkTextDispChunk structures for character segments, the clientData field
 * points to one of the following structures:
 */

#if !TK_LAYOUT_WITH_BASE_CHUNKS

typedef struct CharInfo {
    int numBytes;		/* Number of bytes to display. */
    char chars[1];		/* UTF characters to display. Actual size will
				 * be numBytes, not 1. THIS MUST BE THE LAST
				 * FIELD IN THE STRUCTURE. */
} CharInfo;

#else /* TK_LAYOUT_WITH_BASE_CHUNKS */

typedef struct CharInfo {
    TkTextDispChunk *baseChunkPtr;
    int baseOffset;		/* Starting offset in base chunk
				 * baseChars. */
    int numBytes;		/* Number of bytes that belong to this
				 * chunk. */
    const char *chars;		/* UTF characters to display. Actually points
				 * into the baseChars of the base chunk. Only
				 * valid after FinalizeBaseChunk(). */
} CharInfo;

/*
 * The BaseCharInfo is a CharInfo with some additional data added.
 */

typedef struct BaseCharInfo {
    CharInfo ci;
    Tcl_DString baseChars;	/* Actual characters for the stretch of text
				 * represented by this base chunk. */
    int width;			/* Width in pixels of the whole string, if
				 * known, else -1. Valid during
				 * LayoutDLine(). */
} BaseCharInfo;

/* TODO: Thread safety */
static TkTextDispChunk *baseCharChunkPtr = NULL;

#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */

/*
 * Flag values for TextDInfo structures:
 *
 * DINFO_OUT_OF_DATE:		Non-zero means that the DLine structures for
 *				this window are partially or completely out of
 *				date and need to be recomputed.
 * REDRAW_PENDING:		Means that a when-idle handler has been
 *				scheduled to update the display.
 * REDRAW_BORDERS:		Means window border or pad area has
 *				potentially been damaged and must be redrawn.
 * REPICK_NEEDED:		1 means that the widget has been modified in a
 *				way that could change the current character (a
 *				different character might be under the mouse
 *				cursor now). Need to recompute the current
 *				character before the next redisplay.
 * OUT_OF_SYNC                  1 means that the last <<WidgetViewSync>> event had
 *                              value 0, indicating that the widget is out of sync.
 */

#define DINFO_OUT_OF_DATE	1
#define REDRAW_PENDING		2
#define REDRAW_BORDERS		4
#define REPICK_NEEDED		8
#define OUT_OF_SYNC             16
/*
 * Action values for FreeDLines:
 *
 * DLINE_FREE:		Free the lines, but no need to unlink them from the
 *			current list of actual display lines.
 * DLINE_UNLINK:	Free and unlink from current display.
 * DLINE_FREE_TEMP:	Free, but don't unlink, and also don't set
 *			'dLinesInvalidated'.
 */

#define DLINE_FREE	  0
#define DLINE_UNLINK	  1
#define DLINE_FREE_TEMP	  2

/*
 * The following counters keep statistics about redisplay that can be checked
 * to see how clever this code is at reducing redisplays.
 */

static int numRedisplays;	/* Number of calls to DisplayText. */
static int linesRedrawn;	/* Number of calls to DisplayDLine. */
static int numCopies;		/* Number of calls to XCopyArea to copy part
				 * of the screen. */
static int lineHeightsRecalculated;
				/* Number of line layouts purely for height
				 * calculation purposes.*/
/*
 * Forward declarations for functions defined later in this file:
 */

static void		AdjustForTab(TkText *textPtr,
			    TkTextTabArray *tabArrayPtr, int index,
			    TkTextDispChunk *chunkPtr);
static void		CharBboxProc(TkText *textPtr,
			    TkTextDispChunk *chunkPtr, int index, int y,
			    int lineHeight, int baseline, int *xPtr,
			    int *yPtr, int *widthPtr, int *heightPtr);
static int		CharChunkMeasureChars(TkTextDispChunk *chunkPtr,
			    const char *chars, int charsLen,
			    int start, int end, int startX, int maxX,
			    int flags, int *nextX);
static void		CharDisplayProc(TkText *textPtr,
			    TkTextDispChunk *chunkPtr, int x, int y,
			    int height, int baseline, Display *display,
			    Drawable dst, int screenY);
static int		CharMeasureProc(TkTextDispChunk *chunkPtr, int x);
static void		CharUndisplayProc(TkText *textPtr,
			    TkTextDispChunk *chunkPtr);
#if TK_LAYOUT_WITH_BASE_CHUNKS
static void		FinalizeBaseChunk(TkTextDispChunk *additionalChunkPtr);
static void		FreeBaseChunk(TkTextDispChunk *baseChunkPtr);
static int		IsSameFGStyle(TextStyle *style1, TextStyle *style2);
static void		RemoveFromBaseChunk(TkTextDispChunk *chunkPtr);
#endif
/*
 * Definitions of elided procs. Compiler can't inline these since we use
 * pointers to these functions. ElideDisplayProc and ElideUndisplayProc are
 * special-cased for speed, as potentially many elided DLine chunks if large,
 * tag toggle-filled elided region.
 */
static void		ElideBboxProc(TkText *textPtr,
			    TkTextDispChunk *chunkPtr, int index, int y,
			    int lineHeight, int baseline, int *xPtr,
			    int *yPtr, int *widthPtr, int *heightPtr);
static int		ElideMeasureProc(TkTextDispChunk *chunkPtr, int x);
static void		DisplayDLine(TkText *textPtr, DLine *dlPtr,
			    DLine *prevPtr, Pixmap pixmap);
static void		DisplayLineBackground(TkText *textPtr, DLine *dlPtr,
			    DLine *prevPtr, Pixmap pixmap);
static void		DisplayText(ClientData clientData);
static DLine *		FindDLine(TkText *textPtr, DLine *dlPtr,
                            const TkTextIndex *indexPtr);
static void		FreeDLines(TkText *textPtr, DLine *firstPtr,
			    DLine *lastPtr, int action);
static void		FreeStyle(TkText *textPtr, TextStyle *stylePtr);
static TextStyle *	GetStyle(TkText *textPtr, const TkTextIndex *indexPtr);
static void		GetXView(Tcl_Interp *interp, TkText *textPtr,
			    int report);
static void		GetYView(Tcl_Interp *interp, TkText *textPtr,
			    int report);
static int		GetYPixelCount(TkText *textPtr, DLine *dlPtr);
static DLine *		LayoutDLine(TkText *textPtr,
			    const TkTextIndex *indexPtr);
static int		MeasureChars(Tk_Font tkfont, const char *source,
			    int maxBytes, int rangeStart, int rangeLength,
			    int startX, int maxX, int flags, int *nextXPtr);
static void		MeasureUp(TkText *textPtr,
			    const TkTextIndex *srcPtr, int distance,
			    TkTextIndex *dstPtr, int *overlap);
static int		NextTabStop(Tk_Font tkfont, int x, int tabOrigin);
static void		UpdateDisplayInfo(TkText *textPtr);
static void		YScrollByLines(TkText *textPtr, int offset);
static void		YScrollByPixels(TkText *textPtr, int offset);
static int		SizeOfTab(TkText *textPtr, int tabStyle,
			    TkTextTabArray *tabArrayPtr, int *indexPtr, int x,
			    int maxX);
static void		TextChanged(TkText *textPtr,
			    const TkTextIndex *index1Ptr,
			    const TkTextIndex *index2Ptr);
static void		TextInvalidateRegion(TkText *textPtr, TkRegion region);
static void		TextRedrawTag(TkText *textPtr,
			    TkTextIndex *index1Ptr, TkTextIndex *index2Ptr,
			    TkTextTag *tagPtr, int withTag);
static void		TextInvalidateLineMetrics(TkText *textPtr,
			    TkTextLine *linePtr, int lineCount, int action);
static int		CalculateDisplayLineHeight(TkText *textPtr,
			    const TkTextIndex *indexPtr, int *byteCountPtr,
			    int *mergedLinePtr);
static void		DlineIndexOfX(TkText *textPtr,
			    DLine *dlPtr, int x, TkTextIndex *indexPtr);
static int		DlineXOfIndex(TkText *textPtr,
			    DLine *dlPtr, int byteIndex);
static int		TextGetScrollInfoObj(Tcl_Interp *interp,
			    TkText *textPtr, int objc,
			    Tcl_Obj *const objv[], double *dblPtr,
			    int *intPtr);
static void		AsyncUpdateLineMetrics(ClientData clientData);
static void		GenerateWidgetViewSyncEvent(TkText *textPtr, Bool InSync);
static void		AsyncUpdateYScrollbar(ClientData clientData);
static int              IsStartOfNotMergedLine(TkText *textPtr,
                            const TkTextIndex *indexPtr);

/*
 * Result values returned by TextGetScrollInfoObj:
 */

#define TKTEXT_SCROLL_MOVETO	1
#define TKTEXT_SCROLL_PAGES	2
#define TKTEXT_SCROLL_UNITS	3
#define TKTEXT_SCROLL_ERROR	4
#define TKTEXT_SCROLL_PIXELS	5

/*
 *----------------------------------------------------------------------
 *
 * TkTextCreateDInfo --
 *
 *	This function is called when a new text widget is created. Its job is
 *	to set up display-related information for the widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A TextDInfo data structure is allocated and initialized and attached
 *	to textPtr.
 *
 *----------------------------------------------------------------------
 */

void
TkTextCreateDInfo(
    TkText *textPtr)		/* Overall information for text widget. */
{
    register TextDInfo *dInfoPtr;
    XGCValues gcValues;

    dInfoPtr = ckalloc(sizeof(TextDInfo));
    Tcl_InitHashTable(&dInfoPtr->styleTable, sizeof(StyleValues)/sizeof(int));
    dInfoPtr->dLinePtr = NULL;
    dInfoPtr->copyGC = NULL;
    gcValues.graphics_exposures = True;
    dInfoPtr->scrollGC = Tk_GetGC(textPtr->tkwin, GCGraphicsExposures,
	    &gcValues);
    dInfoPtr->topOfEof = 0;
    dInfoPtr->newXPixelOffset = 0;
    dInfoPtr->curXPixelOffset = 0;
    dInfoPtr->maxLength = 0;
    dInfoPtr->xScrollFirst = -1;
    dInfoPtr->xScrollLast = -1;
    dInfoPtr->yScrollFirst = -1;
    dInfoPtr->yScrollLast = -1;
    dInfoPtr->scanMarkXPixel = 0;
    dInfoPtr->scanMarkX = 0;
    dInfoPtr->scanTotalYScroll = 0;
    dInfoPtr->scanMarkY = 0;
    dInfoPtr->dLinesInvalidated = 0;
    dInfoPtr->flags = 0;
    dInfoPtr->topPixelOffset = 0;
    dInfoPtr->newTopPixelOffset = 0;
    dInfoPtr->currentMetricUpdateLine = -1;
    dInfoPtr->lastMetricUpdateLine = -1;
    dInfoPtr->lineMetricUpdateEpoch = 1;
    dInfoPtr->metricEpoch = -1;
    dInfoPtr->metricIndex.textPtr = NULL;
    dInfoPtr->metricIndex.linePtr = NULL;
    dInfoPtr->lineUpdateTimer = NULL;
    dInfoPtr->scrollbarTimer = NULL;

    textPtr->dInfoPtr = dInfoPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextFreeDInfo --
 *
 *	This function is called to free up all of the private display
 *	information kept by this file for a text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lots of resources get freed.
 *
 *----------------------------------------------------------------------
 */

void
TkTextFreeDInfo(
    TkText *textPtr)		/* Overall information for text widget. */
{
    register TextDInfo *dInfoPtr = textPtr->dInfoPtr;

    /*
     * Be careful to free up styleTable *after* freeing up all the DLines, so
     * that the hash table is still intact to free up the style-related
     * information from the lines. Once the lines are all free then styleTable
     * will be empty.
     */

    FreeDLines(textPtr, dInfoPtr->dLinePtr, NULL, DLINE_UNLINK);
    Tcl_DeleteHashTable(&dInfoPtr->styleTable);
    if (dInfoPtr->copyGC != NULL) {
	Tk_FreeGC(textPtr->display, dInfoPtr->copyGC);
    }
    Tk_FreeGC(textPtr->display, dInfoPtr->scrollGC);
    if (dInfoPtr->flags & REDRAW_PENDING) {
	Tcl_CancelIdleCall(DisplayText, textPtr);
    }
    if (dInfoPtr->lineUpdateTimer != NULL) {
	Tcl_DeleteTimerHandler(dInfoPtr->lineUpdateTimer);
	textPtr->refCount--;
	dInfoPtr->lineUpdateTimer = NULL;
    }
    if (dInfoPtr->scrollbarTimer != NULL) {
	Tcl_DeleteTimerHandler(dInfoPtr->scrollbarTimer);
	textPtr->refCount--;
	dInfoPtr->scrollbarTimer = NULL;
    }
    ckfree(dInfoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * GetStyle --
 *
 *	This function creates all the information needed to display text at a
 *	particular location.
 *
 * Results:
 *	The return value is a pointer to a TextStyle structure that
 *	corresponds to *sValuePtr.
 *
 * Side effects:
 *	A new entry may be created in the style table for the widget.
 *
 *----------------------------------------------------------------------
 */

static TextStyle *
GetStyle(
    TkText *textPtr,		/* Overall information about text widget. */
    const TkTextIndex *indexPtr)/* The character in the text for which display
				 * information is wanted. */
{
    TkTextTag **tagPtrs;
    register TkTextTag *tagPtr;
    StyleValues styleValues;
    TextStyle *stylePtr;
    Tcl_HashEntry *hPtr;
    int numTags, isNew, i;
    int isSelected;
    XGCValues gcValues;
    unsigned long mask;
    /*
     * The variables below keep track of the highest-priority specification
     * that has occurred for each of the various fields of the StyleValues.
     */
    int borderPrio, borderWidthPrio, reliefPrio, bgStipplePrio;
    int fgPrio, fontPrio, fgStipplePrio;
    int underlinePrio, elidePrio, justifyPrio, offsetPrio;
    int lMargin1Prio, lMargin2Prio, rMarginPrio;
    int lMarginColorPrio, rMarginColorPrio;
    int spacing1Prio, spacing2Prio, spacing3Prio;
    int overstrikePrio, tabPrio, tabStylePrio, wrapPrio;

    /*
     * Find out what tags are present for the character, then compute a
     * StyleValues structure corresponding to those tags (scan through all of
     * the tags, saving information for the highest-priority tag).
     */

    tagPtrs = TkBTreeGetTags(indexPtr, textPtr, &numTags);
    borderPrio = borderWidthPrio = reliefPrio = bgStipplePrio = -1;
    fgPrio = fontPrio = fgStipplePrio = -1;
    underlinePrio = elidePrio = justifyPrio = offsetPrio = -1;
    lMargin1Prio = lMargin2Prio = rMarginPrio = -1;
    lMarginColorPrio = rMarginColorPrio = -1;
    spacing1Prio = spacing2Prio = spacing3Prio = -1;
    overstrikePrio = tabPrio = tabStylePrio = wrapPrio = -1;
    memset(&styleValues, 0, sizeof(StyleValues));
    styleValues.relief = TK_RELIEF_FLAT;
    styleValues.fgColor = textPtr->fgColor;
    styleValues.underlineColor = textPtr->fgColor;
    styleValues.overstrikeColor = textPtr->fgColor;
    styleValues.tkfont = textPtr->tkfont;
    styleValues.justify = TK_JUSTIFY_LEFT;
    styleValues.spacing1 = textPtr->spacing1;
    styleValues.spacing2 = textPtr->spacing2;
    styleValues.spacing3 = textPtr->spacing3;
    styleValues.tabArrayPtr = textPtr->tabArrayPtr;
    styleValues.tabStyle = textPtr->tabStyle;
    styleValues.wrapMode = textPtr->wrapMode;
    styleValues.elide = 0;
    isSelected = 0;

    for (i = 0 ; i < numTags; i++) {
        if (textPtr->selTagPtr == tagPtrs[i]) {
            isSelected = 1;
            break;
        }
    }

    for (i = 0 ; i < numTags; i++) {
	Tk_3DBorder border;
        XColor *fgColor;

	tagPtr = tagPtrs[i];
	border = tagPtr->border;
        fgColor = tagPtr->fgColor;

	/*
	 * If this is the selection tag, and inactiveSelBorder is NULL (the
	 * default on Windows), then we need to skip it if we don't have the
	 * focus.
	 */

	if ((tagPtr == textPtr->selTagPtr) && !(textPtr->flags & GOT_FOCUS)) {
	    if (textPtr->inactiveSelBorder == NULL
#ifdef MAC_OSX_TK
		    /* Don't show inactive selection in disabled widgets. */
		    || textPtr->state == TK_TEXT_STATE_DISABLED
#endif
	    ) {
		continue;
	    }
	    border = textPtr->inactiveSelBorder;
	}

        if ((tagPtr->selBorder != NULL) && (isSelected)) {
            border = tagPtr->selBorder;
        }

        if ((tagPtr->selFgColor != NULL) && isSelected) {
            fgColor = tagPtr->selFgColor;
        }

	if ((border != NULL) && (tagPtr->priority > borderPrio)) {
	    styleValues.border = border;
	    borderPrio = tagPtr->priority;
	}
	if ((tagPtr->borderWidthPtr != NULL)
		&& (Tcl_GetString(tagPtr->borderWidthPtr)[0] != '\0')
		&& (tagPtr->priority > borderWidthPrio)) {
	    styleValues.borderWidth = tagPtr->borderWidth;
	    borderWidthPrio = tagPtr->priority;
	}
	if ((tagPtr->reliefString != NULL)
		&& (tagPtr->priority > reliefPrio)) {
	    if (styleValues.border == NULL) {
		styleValues.border = textPtr->border;
	    }
	    styleValues.relief = tagPtr->relief;
	    reliefPrio = tagPtr->priority;
	}
	if ((tagPtr->bgStipple != None)
		&& (tagPtr->priority > bgStipplePrio)) {
	    styleValues.bgStipple = tagPtr->bgStipple;
	    bgStipplePrio = tagPtr->priority;
	}
	if ((fgColor != NULL) && (tagPtr->priority > fgPrio)) {
	    styleValues.fgColor = fgColor;
	    fgPrio = tagPtr->priority;
	}
	if ((tagPtr->tkfont != NULL) && (tagPtr->priority > fontPrio)) {
	    styleValues.tkfont = tagPtr->tkfont;
	    fontPrio = tagPtr->priority;
	}
	if ((tagPtr->fgStipple != None)
		&& (tagPtr->priority > fgStipplePrio)) {
	    styleValues.fgStipple = tagPtr->fgStipple;
	    fgStipplePrio = tagPtr->priority;
	}
	if ((tagPtr->justifyString != NULL)
		&& (tagPtr->priority > justifyPrio)) {
	    styleValues.justify = tagPtr->justify;
	    justifyPrio = tagPtr->priority;
	}
	if ((tagPtr->lMargin1String != NULL)
		&& (tagPtr->priority > lMargin1Prio)) {
	    styleValues.lMargin1 = tagPtr->lMargin1;
	    lMargin1Prio = tagPtr->priority;
	}
	if ((tagPtr->lMargin2String != NULL)
		&& (tagPtr->priority > lMargin2Prio)) {
	    styleValues.lMargin2 = tagPtr->lMargin2;
	    lMargin2Prio = tagPtr->priority;
	}
	if ((tagPtr->lMarginColor != NULL)
		&& (tagPtr->priority > lMarginColorPrio)) {
	    styleValues.lMarginColor = tagPtr->lMarginColor;
	    lMarginColorPrio = tagPtr->priority;
	}
	if ((tagPtr->offsetString != NULL)
		&& (tagPtr->priority > offsetPrio)) {
	    styleValues.offset = tagPtr->offset;
	    offsetPrio = tagPtr->priority;
	}
	if ((tagPtr->overstrikeString != NULL)
		&& (tagPtr->priority > overstrikePrio)) {
	    styleValues.overstrike = tagPtr->overstrike;
	    overstrikePrio = tagPtr->priority;
            if (tagPtr->overstrikeColor != NULL) {
                 styleValues.overstrikeColor = tagPtr->overstrikeColor;
            } else if (fgColor != NULL) {
                 styleValues.overstrikeColor = fgColor;
            }
	}
	if ((tagPtr->rMarginString != NULL)
		&& (tagPtr->priority > rMarginPrio)) {
	    styleValues.rMargin = tagPtr->rMargin;
	    rMarginPrio = tagPtr->priority;
	}
	if ((tagPtr->rMarginColor != NULL)
		&& (tagPtr->priority > rMarginColorPrio)) {
	    styleValues.rMarginColor = tagPtr->rMarginColor;
	    rMarginColorPrio = tagPtr->priority;
	}
	if ((tagPtr->spacing1String != NULL)
		&& (tagPtr->priority > spacing1Prio)) {
	    styleValues.spacing1 = tagPtr->spacing1;
	    spacing1Prio = tagPtr->priority;
	}
	if ((tagPtr->spacing2String != NULL)
		&& (tagPtr->priority > spacing2Prio)) {
	    styleValues.spacing2 = tagPtr->spacing2;
	    spacing2Prio = tagPtr->priority;
	}
	if ((tagPtr->spacing3String != NULL)
		&& (tagPtr->priority > spacing3Prio)) {
	    styleValues.spacing3 = tagPtr->spacing3;
	    spacing3Prio = tagPtr->priority;
	}
	if ((tagPtr->tabStringPtr != NULL)
		&& (tagPtr->priority > tabPrio)) {
	    styleValues.tabArrayPtr = tagPtr->tabArrayPtr;
	    tabPrio = tagPtr->priority;
	}
	if ((tagPtr->tabStyle != TK_TEXT_TABSTYLE_NONE)
		&& (tagPtr->priority > tabStylePrio)) {
	    styleValues.tabStyle = tagPtr->tabStyle;
	    tabStylePrio = tagPtr->priority;
	}
	if ((tagPtr->underlineString != NULL)
		&& (tagPtr->priority > underlinePrio)) {
	    styleValues.underline = tagPtr->underline;
	    underlinePrio = tagPtr->priority;
            if (tagPtr->underlineColor != NULL) {
                 styleValues.underlineColor = tagPtr->underlineColor;
            } else if (fgColor != NULL) {
                 styleValues.underlineColor = fgColor;
            }
	}
	if ((tagPtr->elideString != NULL)
		&& (tagPtr->priority > elidePrio)) {
	    styleValues.elide = tagPtr->elide;
	    elidePrio = tagPtr->priority;
	}
	if ((tagPtr->wrapMode != TEXT_WRAPMODE_NULL)
		&& (tagPtr->priority > wrapPrio)) {
	    styleValues.wrapMode = tagPtr->wrapMode;
	    wrapPrio = tagPtr->priority;
	}
    }
    if (tagPtrs != NULL) {
	ckfree(tagPtrs);
    }

    /*
     * Use an existing style if there's one around that matches.
     */

    hPtr = Tcl_CreateHashEntry(&textPtr->dInfoPtr->styleTable,
	    (char *) &styleValues, &isNew);
    if (!isNew) {
	stylePtr = Tcl_GetHashValue(hPtr);
	stylePtr->refCount++;
	return stylePtr;
    }

    /*
     * No existing style matched. Make a new one.
     */

    stylePtr = ckalloc(sizeof(TextStyle));
    stylePtr->refCount = 1;
    if (styleValues.border != NULL) {
	gcValues.foreground = Tk_3DBorderColor(styleValues.border)->pixel;
	mask = GCForeground;
	if (styleValues.bgStipple != None) {
	    gcValues.stipple = styleValues.bgStipple;
	    gcValues.fill_style = FillStippled;
	    mask |= GCStipple|GCFillStyle;
	}
	stylePtr->bgGC = Tk_GetGC(textPtr->tkwin, mask, &gcValues);
    } else {
	stylePtr->bgGC = NULL;
    }
    mask = GCFont;
    gcValues.font = Tk_FontId(styleValues.tkfont);
    mask |= GCForeground;
    gcValues.foreground = styleValues.fgColor->pixel;
    if (styleValues.fgStipple != None) {
	gcValues.stipple = styleValues.fgStipple;
	gcValues.fill_style = FillStippled;
	mask |= GCStipple|GCFillStyle;
    }
    stylePtr->fgGC = Tk_GetGC(textPtr->tkwin, mask, &gcValues);
    mask = GCForeground;
    gcValues.foreground = styleValues.underlineColor->pixel;
    stylePtr->ulGC = Tk_GetGC(textPtr->tkwin, mask, &gcValues);
    gcValues.foreground = styleValues.overstrikeColor->pixel;
    stylePtr->ovGC = Tk_GetGC(textPtr->tkwin, mask, &gcValues);
    stylePtr->sValuePtr = (StyleValues *)
	    Tcl_GetHashKey(&textPtr->dInfoPtr->styleTable, hPtr);
    stylePtr->hPtr = hPtr;
    Tcl_SetHashValue(hPtr, stylePtr);
    return stylePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeStyle --
 *
 *	This function is called when a TextStyle structure is no longer
 *	needed. It decrements the reference count and frees up the space for
 *	the style structure if the reference count is 0.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The storage and other resources associated with the style are freed up
 *	if no-one's still using it.
 *
 *----------------------------------------------------------------------
 */

static void
FreeStyle(
    TkText *textPtr,		/* Information about overall widget. */
    register TextStyle *stylePtr)
				/* Information about style to free. */
{
    stylePtr->refCount--;
    if (stylePtr->refCount == 0) {
	if (stylePtr->bgGC != NULL) {
	    Tk_FreeGC(textPtr->display, stylePtr->bgGC);
	}
	if (stylePtr->fgGC != NULL) {
	    Tk_FreeGC(textPtr->display, stylePtr->fgGC);
	}
	if (stylePtr->ulGC != NULL) {
	    Tk_FreeGC(textPtr->display, stylePtr->ulGC);
	}
	if (stylePtr->ovGC != NULL) {
	    Tk_FreeGC(textPtr->display, stylePtr->ovGC);
	}
	Tcl_DeleteHashEntry(stylePtr->hPtr);
	ckfree(stylePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * LayoutDLine --
 *
 *	This function generates a single DLine structure for a display line
 *	whose leftmost character is given by indexPtr.
 *
 * Results:
 *	The return value is a pointer to a DLine structure describing the
 *	display line. All fields are filled in and correct except for y and
 *	nextPtr.
 *
 * Side effects:
 *	Storage is allocated for the new DLine.
 *
 *	See the comments in 'GetYView' for some thoughts on what the side-
 *	effects of this call (or its callers) should be; the synchronisation
 *	of TkTextLine->pixelHeight with the sum of the results of this
 *	function operating on all display lines within each logical line.
 *	Ideally the code should be refactored to ensure the cached pixel
 *	height is never behind what is known when this function is called
 *	elsewhere.
 *
 *	Unfortunately, this function is currently called from many different
 *	places, not just to layout a display line for actual display, but also
 *	simply to calculate some metric or other of one or more display lines
 *	(typically the height). It would be a good idea to do some profiling
 *	of typical text widget usage and the way in which this is called and
 *	see if some optimization could or should be done.
 *
 *----------------------------------------------------------------------
 */

static DLine *
LayoutDLine(
    TkText *textPtr,		/* Overall information about text widget. */
    const TkTextIndex *indexPtr)/* Beginning of display line. May not
				 * necessarily point to a character
				 * segment. */
{
    register DLine *dlPtr;	/* New display line. */
    TkTextSegment *segPtr;	/* Current segment in text. */
    TkTextDispChunk *lastChunkPtr;
				/* Last chunk allocated so far for line. */
    TkTextDispChunk *chunkPtr;	/* Current chunk. */
    TkTextIndex curIndex;
    TkTextDispChunk *breakChunkPtr;
				/* Chunk containing best word break point, if
				 * any. */
    TkTextIndex breakIndex;	/* Index of first character in
				 * breakChunkPtr. */
    int breakByteOffset;	/* Byte offset of character within
				 * breakChunkPtr just to right of best break
				 * point. */
    int noCharsYet;		/* Non-zero means that no characters have been
				 * placed on the line yet. */
    int paragraphStart;		/* Non-zero means that we are on the first
				 * line of a paragraph (used to choose between
				 * lmargin1, lmargin2). */
    int justify;		/* How to justify line: taken from style for
				 * the first character in line. */
    int jIndent;		/* Additional indentation (beyond margins) due
				 * to justification. */
    int rMargin;		/* Right margin width for line. */
    TkWrapMode wrapMode;	/* Wrap mode to use for this line. */
    int x = 0, maxX = 0;	/* Initializations needed only to stop
				 * compiler warnings. */
    int wholeLine;		/* Non-zero means this display line runs to
				 * the end of the text line. */
    int tabIndex;		/* Index of the current tab stop. */
    int gotTab;			/* Non-zero means the current chunk contains a
				 * tab. */
    TkTextDispChunk *tabChunkPtr;
				/* Pointer to the chunk containing the
				 * previous tab stop. */
    int maxBytes;		/* Maximum number of bytes to include in this
				 * chunk. */
    TkTextTabArray *tabArrayPtr;/* Tab stops for line; taken from style for
				 * the first character on line. */
    int tabStyle;		/* One of TABULAR or WORDPROCESSOR. */
    int tabSize;		/* Number of pixels consumed by current tab
				 * stop. */
    TkTextDispChunk *lastCharChunkPtr;
				/* Pointer to last chunk in display lines with
				 * numBytes > 0. Used to drop 0-sized chunks
				 * from the end of the line. */
    int byteOffset, ascent, descent, code, elide, elidesize;
    StyleValues *sValuePtr;
    TkTextElideInfo info;	/* Keep track of elide state. */

    /*
     * Create and initialize a new DLine structure.
     */

    dlPtr = ckalloc(sizeof(DLine));
    dlPtr->index = *indexPtr;
    dlPtr->byteCount = 0;
    dlPtr->y = 0;
    dlPtr->oldY = 0;		/* Only set to avoid compiler warnings. */
    dlPtr->height = 0;
    dlPtr->baseline = 0;
    dlPtr->chunkPtr = NULL;
    dlPtr->nextPtr = NULL;
    dlPtr->flags = NEW_LAYOUT | OLD_Y_INVALID;
    dlPtr->logicalLinesMerged = 0;
    dlPtr->lMarginColor = NULL;
    dlPtr->lMarginWidth = 0;
    dlPtr->rMarginColor = NULL;
    dlPtr->rMarginWidth = 0;

    /*
     * This is not necessarily totally correct, where we have merged logical
     * lines. Fixing this would require a quite significant overhaul, though,
     * so currently we make do with this.
     */

    paragraphStart = (indexPtr->byteIndex == 0);

    /*
     * Special case entirely elide line as there may be 1000s or more.
     */

    elide = TkTextIsElided(textPtr, indexPtr, &info);
    if (elide && indexPtr->byteIndex == 0) {
	maxBytes = 0;
	for (segPtr = info.segPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    if (segPtr->size > 0) {
		if (elide == 0) {
		    /*
		     * We toggled a tag and the elide state changed to
		     * visible, and we have something of non-zero size.
		     * Therefore we must bail out.
		     */

		    break;
		}
		maxBytes += segPtr->size;

		/*
		 * Reset tag elide priority, since we're on a new character.
		 */

	    } else if ((segPtr->typePtr == &tkTextToggleOffType)
		    || (segPtr->typePtr == &tkTextToggleOnType)) {
		TkTextTag *tagPtr = segPtr->body.toggle.tagPtr;

		/*
		 * The elide state only changes if this tag is either the
		 * current highest priority tag (and is therefore being
		 * toggled off), or it's a new tag with higher priority.
		 */

		if (tagPtr->elideString != NULL) {
		    info.tagCnts[tagPtr->priority]++;
		    if (info.tagCnts[tagPtr->priority] & 1) {
			info.tagPtrs[tagPtr->priority] = tagPtr;
		    }
		    if (tagPtr->priority >= info.elidePriority) {
			if (segPtr->typePtr == &tkTextToggleOffType) {
			    /*
			     * If it is being toggled off, and it has an elide
			     * string, it must actually be the current highest
			     * priority tag, so this check is redundant:
			     */

			    if (tagPtr->priority != info.elidePriority) {
				Tcl_Panic("Bad tag priority being toggled off");
			    }

			    /*
			     * Find previous elide tag, if any (if not then
			     * elide will be zero, of course).
			     */

			    elide = 0;
			    while (--info.elidePriority > 0) {
				if (info.tagCnts[info.elidePriority] & 1) {
				    elide = info.tagPtrs[info.elidePriority]
					    ->elide;
				    break;
				}
			    }
			} else {
			    elide = tagPtr->elide;
			    info.elidePriority = tagPtr->priority;
			}
		    }
		}
	    }
	}

	if (elide) {
	    dlPtr->byteCount = maxBytes;
	    dlPtr->spaceAbove = dlPtr->spaceBelow = dlPtr->length = 0;
	    if (dlPtr->index.byteIndex == 0) {
		/*
		 * Elided state goes from beginning to end of an entire
		 * logical line. This means we can update the line's pixel
		 * height, and bring its pixel calculation up to date.
		 */

		TkBTreeLinePixelEpoch(textPtr, dlPtr->index.linePtr)
			= textPtr->dInfoPtr->lineMetricUpdateEpoch;

		if (TkBTreeLinePixelCount(textPtr,dlPtr->index.linePtr) != 0) {
		    TkBTreeAdjustPixelHeight(textPtr,
			    dlPtr->index.linePtr, 0, 0);
		}
	    }
	    TkTextFreeElideInfo(&info);
	    return dlPtr;
	}
    }
    TkTextFreeElideInfo(&info);

    /*
     * Each iteration of the loop below creates one TkTextDispChunk for the
     * new display line. The line will always have at least one chunk (for the
     * newline character at the end, if there's nothing else available).
     */

    curIndex = *indexPtr;
    lastChunkPtr = NULL;
    chunkPtr = NULL;
    noCharsYet = 1;
    elide = 0;
    breakChunkPtr = NULL;
    breakByteOffset = 0;
    justify = TK_JUSTIFY_LEFT;
    tabIndex = -1;
    tabChunkPtr = NULL;
    tabArrayPtr = NULL;
    tabStyle = TK_TEXT_TABSTYLE_TABULAR;
    rMargin = 0;
    wrapMode = TEXT_WRAPMODE_CHAR;
    tabSize = 0;
    lastCharChunkPtr = NULL;

    /*
     * Find the first segment to consider for the line. Can't call
     * TkTextIndexToSeg for this because it won't return a segment with zero
     * size (such as the insertion cursor's mark).
     */

  connectNextLogicalLine:
    byteOffset = curIndex.byteIndex;
    segPtr = curIndex.linePtr->segPtr;
    while ((byteOffset > 0) && (byteOffset >= segPtr->size)) {
	byteOffset -= segPtr->size;
	segPtr = segPtr->nextPtr;

	if (segPtr == NULL) {
	    /*
	     * Two logical lines merged into one display line through eliding
	     * of a newline.
	     */

	    TkTextLine *linePtr = TkBTreeNextLine(NULL, curIndex.linePtr);
	    if (linePtr == NULL) {
		break;
	    }

	    dlPtr->logicalLinesMerged++;
	    curIndex.byteIndex = 0;
	    curIndex.linePtr = linePtr;
	    segPtr = curIndex.linePtr->segPtr;
	}
    }

    while (segPtr != NULL) {
	/*
	 * Every logical line still gets at least one chunk due to
	 * expectations in the rest of the code, but we are able to skip
	 * elided portions of the line quickly.
	 *
	 * If current chunk is elided and last chunk was too, coalese.
	 *
	 * This also means that each logical line which is entirely elided
	 * still gets laid out into a DLine, but with zero height. This isn't
	 * particularly a problem, but it does seem somewhat unnecessary. We
	 * may wish to redesign the code to remove these zero height DLines in
	 * the future.
	 */

	if (elide && (lastChunkPtr != NULL)
		&& (lastChunkPtr->displayProc == NULL /*ElideDisplayProc*/)) {
	    elidesize = segPtr->size - byteOffset;
	    if (elidesize > 0) {
		curIndex.byteIndex += elidesize;
		lastChunkPtr->numBytes += elidesize;
		breakByteOffset = lastChunkPtr->breakIndex
			= lastChunkPtr->numBytes;

		/*
		 * If have we have a tag toggle, there is a chance that
		 * invisibility state changed, so bail out.
		 */
	    } else if ((segPtr->typePtr == &tkTextToggleOffType)
		    || (segPtr->typePtr == &tkTextToggleOnType)) {
		if (segPtr->body.toggle.tagPtr->elideString != NULL) {
		    elide = (segPtr->typePtr == &tkTextToggleOffType)
			    ^ segPtr->body.toggle.tagPtr->elide;
		}
	    }

	    byteOffset = 0;
	    segPtr = segPtr->nextPtr;

	    if (segPtr == NULL) {
		/*
		 * Two logical lines merged into one display line through
		 * eliding of a newline.
		 */

		TkTextLine *linePtr = TkBTreeNextLine(NULL, curIndex.linePtr);

		if (linePtr != NULL) {
		    dlPtr->logicalLinesMerged++;
		    curIndex.byteIndex = 0;
		    curIndex.linePtr = linePtr;
		    goto connectNextLogicalLine;
		}
	    }

	    /*
	     * Code no longer needed, now that we allow logical lines to merge
	     * into a single display line.
	     *
	    if (segPtr == NULL && chunkPtr != NULL) {
		ckfree(chunkPtr);
		chunkPtr = NULL;
	    }
	     */

	    continue;
	}

	if (segPtr->typePtr->layoutProc == NULL) {
	    segPtr = segPtr->nextPtr;
	    byteOffset = 0;
	    continue;
	}
	if (chunkPtr == NULL) {
	    chunkPtr = ckalloc(sizeof(TkTextDispChunk));
	    chunkPtr->nextPtr = NULL;
	    chunkPtr->clientData = NULL;
	}
	chunkPtr->stylePtr = GetStyle(textPtr, &curIndex);
	elide = chunkPtr->stylePtr->sValuePtr->elide;

	/*
	 * Save style information such as justification and indentation, up
	 * until the first character is encountered, then retain that
	 * information for the rest of the line.
	 */

	if (!elide && noCharsYet) {
	    tabArrayPtr = chunkPtr->stylePtr->sValuePtr->tabArrayPtr;
	    tabStyle = chunkPtr->stylePtr->sValuePtr->tabStyle;
	    justify = chunkPtr->stylePtr->sValuePtr->justify;
	    rMargin = chunkPtr->stylePtr->sValuePtr->rMargin;
	    wrapMode = chunkPtr->stylePtr->sValuePtr->wrapMode;

	    /*
	     * See above - this test may not be entirely correct where we have
	     * partially elided lines (and therefore merged logical lines).
	     * In such a case a byteIndex of zero doesn't necessarily mean the
	     * beginning of a logical line.
	     */

	    if (paragraphStart) {
		/*
		 * Beginning of logical line.
		 */

		x = chunkPtr->stylePtr->sValuePtr->lMargin1;
	    } else {
		/*
		 * Beginning of display line.
		 */

		x = chunkPtr->stylePtr->sValuePtr->lMargin2;
	    }
            dlPtr->lMarginWidth = x;
	    if (wrapMode == TEXT_WRAPMODE_NONE) {
		maxX = -1;
	    } else {
		maxX = textPtr->dInfoPtr->maxX - textPtr->dInfoPtr->x
			- rMargin;
		if (maxX < x) {
		    maxX = x;
		}
	    }
	}

	gotTab = 0;
	maxBytes = segPtr->size - byteOffset;
	if (segPtr->typePtr == &tkTextCharType) {

	    /*
	     * See if there is a tab in the current chunk; if so, only layout
	     * characters up to (and including) the tab.
	     */

	    if (!elide && justify == TK_JUSTIFY_LEFT) {
		char *p;

		for (p = segPtr->body.chars + byteOffset; *p != 0; p++) {
		    if (*p == '\t') {
			maxBytes = (p + 1 - segPtr->body.chars) - byteOffset;
			gotTab = 1;
			break;
		    }
		}
	    }

#if TK_LAYOUT_WITH_BASE_CHUNKS
	    if (baseCharChunkPtr != NULL) {
		int expectedX =
			((BaseCharInfo *) baseCharChunkPtr->clientData)->width
			+ baseCharChunkPtr->x;

		if ((expectedX != x) || !IsSameFGStyle(
			baseCharChunkPtr->stylePtr, chunkPtr->stylePtr)) {
		    FinalizeBaseChunk(NULL);
		}
	    }
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */
	}
	chunkPtr->x = x;
	if (elide /*&& maxBytes*/) {
	    /*
	     * Don't free style here, as other code expects to be able to do
	     * that.
	     */

	    /* breakByteOffset =*/
	    chunkPtr->breakIndex = chunkPtr->numBytes = maxBytes;
	    chunkPtr->width = 0;
	    chunkPtr->minAscent = chunkPtr->minDescent
		    = chunkPtr->minHeight = 0;

	    /*
	     * Would just like to point to canonical empty chunk.
	     */

	    chunkPtr->displayProc = NULL;
	    chunkPtr->undisplayProc = NULL;
	    chunkPtr->measureProc = ElideMeasureProc;
	    chunkPtr->bboxProc = ElideBboxProc;

	    code = 1;
	} else {
	    code = segPtr->typePtr->layoutProc(textPtr, &curIndex, segPtr,
		    byteOffset, maxX-tabSize, maxBytes, noCharsYet, wrapMode,
		    chunkPtr);
	}
	if (code <= 0) {
	    FreeStyle(textPtr, chunkPtr->stylePtr);
	    if (code < 0) {
		/*
		 * This segment doesn't wish to display itself (e.g. most
		 * marks).
		 */

		segPtr = segPtr->nextPtr;
		byteOffset = 0;
		continue;
	    }

	    /*
	     * No characters from this segment fit in the window: this means
	     * we're at the end of the display line.
	     */

	    if (chunkPtr != NULL) {
		ckfree(chunkPtr);
	    }
	    break;
	}

	/*
	 * We currently say we have some characters (and therefore something
	 * from which to examine tag values for the first character of the
	 * line) even if those characters are actually elided. This behaviour
	 * is not well documented, and it might be more consistent to
	 * completely ignore such elided characters and their tags. To do so
	 * change this to:
	 *
	 * if (!elide && chunkPtr->numBytes > 0).
	 */

	if (!elide && chunkPtr->numBytes > 0) {
	    noCharsYet = 0;
	    lastCharChunkPtr = chunkPtr;
	}
	if (lastChunkPtr == NULL) {
	    dlPtr->chunkPtr = chunkPtr;
	} else {
	    lastChunkPtr->nextPtr = chunkPtr;
	}
	lastChunkPtr = chunkPtr;
	x += chunkPtr->width;
	if (chunkPtr->breakIndex > 0) {
	    breakByteOffset = chunkPtr->breakIndex;
	    breakIndex = curIndex;
	    breakChunkPtr = chunkPtr;
	}
	if (chunkPtr->numBytes != maxBytes) {
	    break;
	}

	/*
	 * If we're at a new tab, adjust the layout for all the chunks
	 * pertaining to the previous tab. Also adjust the amount of space
	 * left in the line to account for space that will be eaten up by the
	 * tab.
	 */

	if (gotTab) {
	    if (tabIndex >= 0) {
		AdjustForTab(textPtr, tabArrayPtr, tabIndex, tabChunkPtr);
		x = chunkPtr->x + chunkPtr->width;
	    }
	    tabChunkPtr = chunkPtr;
	    tabSize = SizeOfTab(textPtr, tabStyle, tabArrayPtr, &tabIndex, x,
		    maxX);
	    if ((maxX >= 0) && (tabSize >= maxX - x)) {
		break;
	    }
	}
	curIndex.byteIndex += chunkPtr->numBytes;
	byteOffset += chunkPtr->numBytes;
	if (byteOffset >= segPtr->size) {
	    byteOffset = 0;
	    segPtr = segPtr->nextPtr;
	    if (elide && segPtr == NULL) {
		/*
		 * An elided section started on this line, and carries on
		 * until the newline. Hence the newline is actually elided,
		 * and we want to merge the display of the next logical line
		 * with this one.
		 */

		TkTextLine *linePtr = TkBTreeNextLine(NULL, curIndex.linePtr);

		if (linePtr != NULL) {
		    dlPtr->logicalLinesMerged++;
		    curIndex.byteIndex = 0;
		    curIndex.linePtr = linePtr;
		    chunkPtr = NULL;
		    goto connectNextLogicalLine;
		}
	    }
	}

	chunkPtr = NULL;
    }
#if TK_LAYOUT_WITH_BASE_CHUNKS
    FinalizeBaseChunk(NULL);
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */
    if (noCharsYet) {
	dlPtr->spaceAbove = 0;
	dlPtr->spaceBelow = 0;
	dlPtr->length = 0;

	/*
	 * We used to Tcl_Panic here, saying that LayoutDLine couldn't place
	 * any characters on a line, but I believe a more appropriate response
	 * is to return a DLine with zero height. With elided lines, tag
	 * transitions and asynchronous line height calculations, it is hard
	 * to avoid this situation ever arising with the current code design.
	 */

	return dlPtr;
    }
    wholeLine = (segPtr == NULL);

    /*
     * We're at the end of the display line. Throw away everything after the
     * most recent word break, if there is one; this may potentially require
     * the last chunk to be layed out again.
     */

    if (breakChunkPtr == NULL) {
	/*
	 * This code makes sure that we don't accidentally display chunks with
	 * no characters at the end of the line (such as the insertion
	 * cursor). These chunks belong on the next line. So, throw away
	 * everything after the last chunk that has characters in it.
	 */

	breakChunkPtr = lastCharChunkPtr;
	breakByteOffset = breakChunkPtr->numBytes;
    }
    if ((breakChunkPtr != NULL) && ((lastChunkPtr != breakChunkPtr)
	    || (breakByteOffset != lastChunkPtr->numBytes))) {
	while (1) {
	    chunkPtr = breakChunkPtr->nextPtr;
	    if (chunkPtr == NULL) {
		break;
	    }
	    FreeStyle(textPtr, chunkPtr->stylePtr);
	    breakChunkPtr->nextPtr = chunkPtr->nextPtr;
	    if (chunkPtr->undisplayProc != NULL) {
		chunkPtr->undisplayProc(textPtr, chunkPtr);
	    }
	    ckfree(chunkPtr);
	}
	if (breakByteOffset != breakChunkPtr->numBytes) {
	    if (breakChunkPtr->undisplayProc != NULL) {
		breakChunkPtr->undisplayProc(textPtr, breakChunkPtr);
	    }
	    segPtr = TkTextIndexToSeg(&breakIndex, &byteOffset);
	    segPtr->typePtr->layoutProc(textPtr, &breakIndex, segPtr,
		    byteOffset, maxX, breakByteOffset, 0, wrapMode,
		    breakChunkPtr);
#if TK_LAYOUT_WITH_BASE_CHUNKS
	    FinalizeBaseChunk(NULL);
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */
	}
	lastChunkPtr = breakChunkPtr;
	wholeLine = 0;
    }

    /*
     * Make tab adjustments for the last tab stop, if there is one.
     */

    if ((tabIndex >= 0) && (tabChunkPtr != NULL)) {
	AdjustForTab(textPtr, tabArrayPtr, tabIndex, tabChunkPtr);
    }

    /*
     * Make one more pass over the line to recompute various things like its
     * height, length, and total number of bytes. Also modify the x-locations
     * of chunks to reflect justification. If we're not wrapping, I'm not sure
     * what is the best way to handle right and center justification: should
     * the total length, for purposes of justification, be (a) the window
     * width, (b) the length of the longest line in the window, or (c) the
     * length of the longest line in the text? (c) isn't available, (b) seems
     * weird, since it can change with vertical scrolling, so (a) is what is
     * implemented below.
     */

    if (wrapMode == TEXT_WRAPMODE_NONE) {
	maxX = textPtr->dInfoPtr->maxX - textPtr->dInfoPtr->x - rMargin;
    }
    dlPtr->length = lastChunkPtr->x + lastChunkPtr->width;
    if (justify == TK_JUSTIFY_LEFT) {
	jIndent = 0;
    } else if (justify == TK_JUSTIFY_RIGHT) {
	jIndent = maxX - dlPtr->length;
    } else {
	jIndent = (maxX - dlPtr->length)/2;
    }
    ascent = descent = 0;
    for (chunkPtr = dlPtr->chunkPtr; chunkPtr != NULL;
	    chunkPtr = chunkPtr->nextPtr) {
	chunkPtr->x += jIndent;
	dlPtr->byteCount += chunkPtr->numBytes;
	if (chunkPtr->minAscent > ascent) {
	    ascent = chunkPtr->minAscent;
	}
	if (chunkPtr->minDescent > descent) {
	    descent = chunkPtr->minDescent;
	}
	if (chunkPtr->minHeight > dlPtr->height) {
	    dlPtr->height = chunkPtr->minHeight;
	}
	sValuePtr = chunkPtr->stylePtr->sValuePtr;
	if ((sValuePtr->borderWidth > 0)
		&& (sValuePtr->relief != TK_RELIEF_FLAT)) {
	    dlPtr->flags |= HAS_3D_BORDER;
	}
    }
    if (dlPtr->height < (ascent + descent)) {
	dlPtr->height = ascent + descent;
	dlPtr->baseline = ascent;
    } else {
	dlPtr->baseline = ascent + (dlPtr->height - ascent - descent)/2;
    }
    sValuePtr = dlPtr->chunkPtr->stylePtr->sValuePtr;
    if (dlPtr->index.byteIndex == 0) {
	dlPtr->spaceAbove = sValuePtr->spacing1;
    } else {
	dlPtr->spaceAbove = sValuePtr->spacing2 - sValuePtr->spacing2/2;
    }
    if (wholeLine) {
	dlPtr->spaceBelow = sValuePtr->spacing3;
    } else {
	dlPtr->spaceBelow = sValuePtr->spacing2/2;
    }
    dlPtr->height += dlPtr->spaceAbove + dlPtr->spaceBelow;
    dlPtr->baseline += dlPtr->spaceAbove;
    dlPtr->lMarginColor = sValuePtr->lMarginColor;
    dlPtr->rMarginColor = sValuePtr->rMarginColor;
    if (wrapMode != TEXT_WRAPMODE_NONE) {
        dlPtr->rMarginWidth = rMargin;
    }

    /*
     * Recompute line length: may have changed because of justification.
     */

    dlPtr->length = lastChunkPtr->x + lastChunkPtr->width;

    return dlPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateDisplayInfo --
 *
 *	This function is invoked to recompute some or all of the DLine
 *	structures for a text widget. At the time it is called the DLine
 *	structures still left in the widget are guaranteed to be correct
 *	except that (a) the y-coordinates aren't necessarily correct, (b)
 *	there may be missing structures (the DLine structures get removed as
 *	soon as they are potentially out-of-date), and (c) DLine structures
 *	that don't start at the beginning of a line may be incorrect if
 *	previous information in the same line changed size in a way that moved
 *	a line boundary (DLines for any info that changed will have been
 *	deleted, but not DLines for unchanged info in the same text line).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Upon return, the DLine information for textPtr correctly reflects the
 *	positions where characters will be displayed. However, this function
 *	doesn't actually bring the display up-to-date.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateDisplayInfo(
    TkText *textPtr)		/* Text widget to update. */
{
    register TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    register DLine *dlPtr, *prevPtr;
    TkTextIndex index;
    TkTextLine *lastLinePtr;
    int y, maxY, xPixelOffset, maxOffset, lineHeight;

    if (!(dInfoPtr->flags & DINFO_OUT_OF_DATE)) {
	return;
    }
    dInfoPtr->flags &= ~DINFO_OUT_OF_DATE;

    /*
     * Delete any DLines that are now above the top of the window.
     */

    index = textPtr->topIndex;
    dlPtr = FindDLine(textPtr, dInfoPtr->dLinePtr, &index);
    if ((dlPtr != NULL) && (dlPtr != dInfoPtr->dLinePtr)) {
	FreeDLines(textPtr, dInfoPtr->dLinePtr, dlPtr, DLINE_UNLINK);
    }
    if (index.byteIndex == 0) {
	lineHeight = 0;
    } else {
	lineHeight = -1;
    }

    /*
     * Scan through the contents of the window from top to bottom, recomputing
     * information for lines that are missing.
     */

    lastLinePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr,
	    TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr));
    dlPtr = dInfoPtr->dLinePtr;
    prevPtr = NULL;
    y = dInfoPtr->y - dInfoPtr->newTopPixelOffset;
    maxY = dInfoPtr->maxY;
    while (1) {
	register DLine *newPtr;

	if (index.linePtr == lastLinePtr) {
	    break;
	}

	/*
	 * There are three possibilities right now:
	 * (a) the next DLine (dlPtr) corresponds exactly to the next
	 *     information we want to display: just use it as-is.
	 * (b) the next DLine corresponds to a different line, or to a segment
	 *     that will be coming later in the same line: leave this DLine
	 *     alone in the hopes that we'll be able to use it later, then
	 *     create a new DLine in front of it.
	 * (c) the next DLine corresponds to a segment in the line we want,
	 *     but it's a segment that has already been processed or will
	 *     never be processed. Delete the DLine and try again.
	 *
	 * One other twist on all this. It's possible for 3D borders to
	 * interact between lines (see DisplayLineBackground) so if a line is
	 * relayed out and has styles with 3D borders, its neighbors have to
	 * be redrawn if they have 3D borders too, since the interactions
	 * could have changed (the neighbors don't have to be relayed out,
	 * just redrawn).
	 */

	if ((dlPtr == NULL) || (dlPtr->index.linePtr != index.linePtr)) {
	    /*
	     * Case (b) -- must make new DLine.
	     */

	makeNewDLine:
	    if (tkTextDebug) {
		char string[TK_POS_CHARS];

		/*
		 * Debugging is enabled, so keep a log of all the lines that
		 * were re-layed out. The test suite uses this information.
		 */

		TkTextPrintIndex(textPtr, &index, string);
		LOG("tk_textRelayout", string);
	    }
	    newPtr = LayoutDLine(textPtr, &index);
	    if (prevPtr == NULL) {
		dInfoPtr->dLinePtr = newPtr;
	    } else {
		prevPtr->nextPtr = newPtr;
		if (prevPtr->flags & HAS_3D_BORDER) {
		    prevPtr->flags |= OLD_Y_INVALID;
		}
	    }
	    newPtr->nextPtr = dlPtr;
	    dlPtr = newPtr;
	} else {
	    /*
	     * DlPtr refers to the line we want. Next check the index within
	     * the line.
	     */

	    if (index.byteIndex == dlPtr->index.byteIndex) {
		/*
		 * Case (a) - can use existing display line as-is.
		 */

		if ((dlPtr->flags & HAS_3D_BORDER) && (prevPtr != NULL)
			&& (prevPtr->flags & (NEW_LAYOUT))) {
		    dlPtr->flags |= OLD_Y_INVALID;
		}
		goto lineOK;
	    }
	    if (index.byteIndex < dlPtr->index.byteIndex) {
		goto makeNewDLine;
	    }

	    /*
	     * Case (c) - dlPtr is useless. Discard it and start again with
	     * the next display line.
	     */

	    newPtr = dlPtr->nextPtr;
	    FreeDLines(textPtr, dlPtr, newPtr, DLINE_FREE);
	    dlPtr = newPtr;
	    if (prevPtr != NULL) {
		prevPtr->nextPtr = newPtr;
	    } else {
		dInfoPtr->dLinePtr = newPtr;
	    }
	    continue;
	}

	/*
	 * Advance to the start of the next line.
	 */

    lineOK:
	dlPtr->y = y;
	y += dlPtr->height;
	if (lineHeight != -1) {
	    lineHeight += dlPtr->height;
	}
	TkTextIndexForwBytes(textPtr, &index, dlPtr->byteCount, &index);
	prevPtr = dlPtr;
	dlPtr = dlPtr->nextPtr;

	/*
	 * If we switched text lines, delete any DLines left for the old text
	 * line.
	 */

	if (index.linePtr != prevPtr->index.linePtr) {
	    register DLine *nextPtr;

	    nextPtr = dlPtr;
	    while ((nextPtr != NULL)
		    && (nextPtr->index.linePtr == prevPtr->index.linePtr)) {
		nextPtr = nextPtr->nextPtr;
	    }
	    if (nextPtr != dlPtr) {
		FreeDLines(textPtr, dlPtr, nextPtr, DLINE_FREE);
		prevPtr->nextPtr = nextPtr;
		dlPtr = nextPtr;
	    }

	    if ((lineHeight != -1) && (TkBTreeLinePixelCount(textPtr,
		    prevPtr->index.linePtr) != lineHeight)) {
		/*
		 * The logical line height we just calculated is actually
		 * different to the currently cached height of the text line.
		 * That is fine (the text line heights are only calculated
		 * asynchronously), but we must update the cached height so
		 * that any counts made with DLine pointers are the same as
		 * counts made through the BTree. This helps to ensure that
		 * the scrollbar size corresponds accurately to that displayed
		 * contents, even as the window is re-sized.
		 */

		TkBTreeAdjustPixelHeight(textPtr, prevPtr->index.linePtr,
			lineHeight, 0);

		/*
		 * I believe we can be 100% sure that we started at the
		 * beginning of the logical line, so we can also adjust the
		 * 'pixelCalculationEpoch' to mark it as being up to date.
		 * There is a slight concern that we might not have got this
		 * right for the first line in the re-display.
		 */

		TkBTreeLinePixelEpoch(textPtr, prevPtr->index.linePtr) =
			dInfoPtr->lineMetricUpdateEpoch;
	    }
	    lineHeight = 0;
	}

	/*
	 * It's important to have the following check here rather than in the
	 * while statement for the loop, so that there's always at least one
	 * DLine generated, regardless of how small the window is. This keeps
	 * a lot of other code from breaking.
	 */

	if (y >= maxY) {
	    break;
	}
    }

    /*
     * Delete any DLine structures that don't fit on the screen.
     */

    FreeDLines(textPtr, dlPtr, NULL, DLINE_UNLINK);

    /*
     * If there is extra space at the bottom of the window (because we've hit
     * the end of the text), then bring in more lines at the top of the
     * window, if there are any, to fill in the view.
     *
     * Since the top line may only be partially visible, we try first to
     * simply show more pixels from that line (newTopPixelOffset). If that
     * isn't enough, we have to layout more lines.
     */

    if (y < maxY) {
	/*
	 * This counts how many vertical pixels we have left to fill by
	 * pulling in more display pixels either from the first currently
	 * displayed, or the lines above it.
	 */

	int spaceLeft = maxY - y;

	if (spaceLeft <= dInfoPtr->newTopPixelOffset) {
	    /*
	     * We can fill up all the needed space just by showing more of the
	     * current top line.
	     */

	    dInfoPtr->newTopPixelOffset -= spaceLeft;
	    y += spaceLeft;
	    spaceLeft = 0;
	} else {
	    int lineNum, bytesToCount;
	    DLine *lowestPtr;

	    /*
	     * Add in all of the current top line, which won't be enough to
	     * bring y up to maxY (if it was we would be in the 'if' block
	     * above).
	     */

	    y += dInfoPtr->newTopPixelOffset;
	    dInfoPtr->newTopPixelOffset = 0;

	    /*
	     * Layout an entire text line (potentially > 1 display line), then
	     * link in as many display lines as fit without moving the bottom
	     * line out of the window. Repeat this until all the extra space
	     * has been used up or we've reached the beginning of the text.
	     */

	    spaceLeft = maxY - y;
	    if (dInfoPtr->dLinePtr == NULL) {
		/*
		 * No lines have been laid out. This must be an empty peer
		 * widget.
		 */

                lineNum = TkBTreeNumLines(textPtr->sharedTextPtr->tree,
                        textPtr) - 1;
                bytesToCount = INT_MAX;
	    } else {
		lineNum = TkBTreeLinesTo(textPtr,
			dInfoPtr->dLinePtr->index.linePtr);
		bytesToCount = dInfoPtr->dLinePtr->index.byteIndex;
		if (bytesToCount == 0) {
		    bytesToCount = INT_MAX;
		    lineNum--;
		}
	    }
	    for ( ; (lineNum >= 0) && (spaceLeft > 0); lineNum--) {
		int pixelHeight = 0;

		index.linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree,
			textPtr, lineNum);
		index.byteIndex = 0;
		lowestPtr = NULL;

		do {
		    dlPtr = LayoutDLine(textPtr, &index);
		    pixelHeight += dlPtr->height;
		    dlPtr->nextPtr = lowestPtr;
		    lowestPtr = dlPtr;
		    if (dlPtr->length == 0 && dlPtr->height == 0) {
			bytesToCount--;
			break;
		    }	/* elide */
		    TkTextIndexForwBytes(textPtr, &index, dlPtr->byteCount,
			    &index);
		    bytesToCount -= dlPtr->byteCount;
		} while ((bytesToCount > 0)
			&& (index.linePtr == lowestPtr->index.linePtr));

		/*
		 * We may not have examined the entire line (depending on the
		 * value of 'bytesToCount', so we only want to set this if it
		 * is genuinely bigger).
		 */

		if (pixelHeight > TkBTreeLinePixelCount(textPtr,
			lowestPtr->index.linePtr)) {
		    TkBTreeAdjustPixelHeight(textPtr,
			    lowestPtr->index.linePtr, pixelHeight, 0);
		    if (index.linePtr != lowestPtr->index.linePtr) {
			/*
			 * We examined the entire line, so can update the
			 * epoch.
			 */

			TkBTreeLinePixelEpoch(textPtr,
				lowestPtr->index.linePtr) =
				dInfoPtr->lineMetricUpdateEpoch;
		    }
		}

		/*
		 * Scan through the display lines from the bottom one up to
		 * the top one.
		 */

		while (lowestPtr != NULL) {
		    dlPtr = lowestPtr;
		    spaceLeft -= dlPtr->height;
		    lowestPtr = dlPtr->nextPtr;
		    dlPtr->nextPtr = dInfoPtr->dLinePtr;
		    dInfoPtr->dLinePtr = dlPtr;
		    if (tkTextDebug) {
			char string[TK_POS_CHARS];

			TkTextPrintIndex(textPtr, &dlPtr->index, string);
			LOG("tk_textRelayout", string);
		    }
		    if (spaceLeft <= 0) {
			break;
		    }
		}
		FreeDLines(textPtr, lowestPtr, NULL, DLINE_FREE);
		bytesToCount = INT_MAX;
	    }

	    /*
	     * We've either filled in the space we wanted to or we've run out
	     * of display lines at the top of the text. Note that we already
	     * set dInfoPtr->newTopPixelOffset to zero above.
	     */

	    if (spaceLeft < 0) {
		/*
		 * We've laid out a few too many vertical pixels at or above
		 * the first line. Therefore we only want to show part of the
		 * first displayed line, so that the last displayed line just
		 * fits in the window.
		 */

		dInfoPtr->newTopPixelOffset = -spaceLeft;
		if (dInfoPtr->newTopPixelOffset>=dInfoPtr->dLinePtr->height) {
		    /*
		     * Somehow the entire first line we laid out is shorter
		     * than the new offset. This should not occur and would
		     * indicate a bad problem in the logic above.
		     */

		    Tcl_Panic("Error in pixel height consistency while filling in spacesLeft");
		}
	    }
	}

	/*
	 * Now we're all done except that the y-coordinates in all the DLines
	 * are wrong and the top index for the text is wrong. Update them.
	 */

	if (dInfoPtr->dLinePtr != NULL) {
	    textPtr->topIndex = dInfoPtr->dLinePtr->index;
	    y = dInfoPtr->y - dInfoPtr->newTopPixelOffset;
	    for (dlPtr = dInfoPtr->dLinePtr; dlPtr != NULL;
		    dlPtr = dlPtr->nextPtr) {
		if (y > dInfoPtr->maxY) {
		    Tcl_Panic("Added too many new lines in UpdateDisplayInfo");
		}
		dlPtr->y = y;
		y += dlPtr->height;
	    }
	}
    }

    /*
     * If the old top or bottom line has scrolled elsewhere on the screen, we
     * may not be able to re-use its old contents by copying bits (e.g., a
     * beveled edge that was drawn when it was at the top or bottom won't be
     * drawn when the line is in the middle and its neighbor has a matching
     * background). Similarly, if the new top or bottom line came from
     * somewhere else on the screen, we may not be able to copy the old bits.
     */

    dlPtr = dInfoPtr->dLinePtr;
    if (dlPtr != NULL) {
	if ((dlPtr->flags & HAS_3D_BORDER) && !(dlPtr->flags & TOP_LINE)) {
	    dlPtr->flags |= OLD_Y_INVALID;
	}
	while (1) {
	    if ((dlPtr->flags & TOP_LINE) && (dlPtr != dInfoPtr->dLinePtr)
		    && (dlPtr->flags & HAS_3D_BORDER)) {
		dlPtr->flags |= OLD_Y_INVALID;
	    }

	    /*
	     * If the old top-line was not completely showing (i.e. the
	     * pixelOffset is non-zero) and is no longer the top-line, then we
	     * must re-draw it.
	     */

	    if ((dlPtr->flags & TOP_LINE) &&
		    dInfoPtr->topPixelOffset!=0 && dlPtr!=dInfoPtr->dLinePtr) {
		dlPtr->flags |= OLD_Y_INVALID;
	    }
	    if ((dlPtr->flags & BOTTOM_LINE) && (dlPtr->nextPtr != NULL)
		    && (dlPtr->flags & HAS_3D_BORDER)) {
		dlPtr->flags |= OLD_Y_INVALID;
	    }
	    if (dlPtr->nextPtr == NULL) {
		if ((dlPtr->flags & HAS_3D_BORDER)
			&& !(dlPtr->flags & BOTTOM_LINE)) {
		    dlPtr->flags |= OLD_Y_INVALID;
		}
		dlPtr->flags &= ~TOP_LINE;
		dlPtr->flags |= BOTTOM_LINE;
		break;
	    }
	    dlPtr->flags &= ~(TOP_LINE|BOTTOM_LINE);
	    dlPtr = dlPtr->nextPtr;
	}
	dInfoPtr->dLinePtr->flags |= TOP_LINE;
	dInfoPtr->topPixelOffset = dInfoPtr->newTopPixelOffset;
    }

    /*
     * Arrange for scrollbars to be updated.
     */

    textPtr->flags |= UPDATE_SCROLLBARS;

    /*
     * Deal with horizontal scrolling:
     * 1. If there's empty space to the right of the longest line, shift the
     *	  screen to the right to fill in the empty space.
     * 2. If the desired horizontal scroll position has changed, force a full
     *	  redisplay of all the lines in the widget.
     * 3. If the wrap mode isn't "none" then re-scroll to the base position.
     */

    dInfoPtr->maxLength = 0;
    for (dlPtr = dInfoPtr->dLinePtr; dlPtr != NULL;
	    dlPtr = dlPtr->nextPtr) {
	if (dlPtr->length > dInfoPtr->maxLength) {
	    dInfoPtr->maxLength = dlPtr->length;
	}
    }
    maxOffset = dInfoPtr->maxLength - (dInfoPtr->maxX - dInfoPtr->x);

    xPixelOffset = dInfoPtr->newXPixelOffset;
    if (xPixelOffset > maxOffset) {
	xPixelOffset = maxOffset;
    }
    if (xPixelOffset < 0) {
	xPixelOffset = 0;
    }

    /*
     * Here's a problem: see the tests textDisp-29.2.1-4
     *
     * If the widget is being created, but has not yet been configured it will
     * have a maxY of 1 above, and we won't have examined all the lines
     * (just the first line, in fact), and so maxOffset will not be a true
     * reflection of the widget's lines. Therefore we must not overwrite the
     * original newXPixelOffset in this case.
     */

    if (!(((Tk_FakeWin *) (textPtr->tkwin))->flags & TK_NEED_CONFIG_NOTIFY)) {
	dInfoPtr->newXPixelOffset = xPixelOffset;
    }

    if (xPixelOffset != dInfoPtr->curXPixelOffset) {
	dInfoPtr->curXPixelOffset = xPixelOffset;
	for (dlPtr = dInfoPtr->dLinePtr; dlPtr != NULL;
		dlPtr = dlPtr->nextPtr) {
	    dlPtr->flags |= OLD_Y_INVALID;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeDLines --
 *
 *	This function is called to free up all of the resources associated
 *	with one or more DLine structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets freed and various other resources are released.
 *
 *----------------------------------------------------------------------
 */

static void
FreeDLines(
    TkText *textPtr,		/* Information about overall text widget. */
    register DLine *firstPtr,	/* Pointer to first DLine to free up. */
    DLine *lastPtr,		/* Pointer to DLine just after last one to
				 * free (NULL means everything starting with
				 * firstPtr). */
    int action)			/* DLINE_UNLINK means DLines are currently
				 * linked into the list rooted at
				 * textPtr->dInfoPtr->dLinePtr and they have
				 * to be unlinked. DLINE_FREE means just free
				 * without unlinking. DLINE_FREE_TEMP means
				 * the DLine given is just a temporary one and
				 * we shouldn't invalidate anything for the
				 * overall widget. */
{
    register TkTextDispChunk *chunkPtr, *nextChunkPtr;
    register DLine *nextDLinePtr;

    if (action == DLINE_FREE_TEMP) {
	lineHeightsRecalculated++;
	if (tkTextDebug) {
	    char string[TK_POS_CHARS];

	    /*
	     * Debugging is enabled, so keep a log of all the lines whose
	     * height was recalculated. The test suite uses this information.
	     */

	    TkTextPrintIndex(textPtr, &firstPtr->index, string);
	    LOG("tk_textHeightCalc", string);
	}
    } else if (action == DLINE_UNLINK) {
	if (textPtr->dInfoPtr->dLinePtr == firstPtr) {
	    textPtr->dInfoPtr->dLinePtr = lastPtr;
	} else {
	    register DLine *prevPtr;

	    for (prevPtr = textPtr->dInfoPtr->dLinePtr;
		    prevPtr->nextPtr != firstPtr; prevPtr = prevPtr->nextPtr) {
		/* Empty loop body. */
	    }
	    prevPtr->nextPtr = lastPtr;
	}
    }
    while (firstPtr != lastPtr) {
	nextDLinePtr = firstPtr->nextPtr;
	for (chunkPtr = firstPtr->chunkPtr; chunkPtr != NULL;
		chunkPtr = nextChunkPtr) {
	    if (chunkPtr->undisplayProc != NULL) {
		chunkPtr->undisplayProc(textPtr, chunkPtr);
	    }
	    FreeStyle(textPtr, chunkPtr->stylePtr);
	    nextChunkPtr = chunkPtr->nextPtr;
	    ckfree(chunkPtr);
	}
	ckfree(firstPtr);
	firstPtr = nextDLinePtr;
    }
    if (action != DLINE_FREE_TEMP) {
	textPtr->dInfoPtr->dLinesInvalidated = 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayDLine --
 *
 *	This function is invoked to draw a single line on the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The line given by dlPtr is drawn at its correct position in textPtr's
 *	window. Note that this is one *display* line, not one *text* line.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayDLine(
    TkText *textPtr,		/* Text widget in which to draw line. */
    register DLine *dlPtr,	/* Information about line to draw. */
    DLine *prevPtr,		/* Line just before one to draw, or NULL if
				 * dlPtr is the top line. */
    Pixmap pixmap)		/* Pixmap to use for double-buffering. Caller
				 * must make sure it's large enough to hold
				 * line. */
{
    register TkTextDispChunk *chunkPtr;
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    Display *display;
    int height, y_off;
#ifndef TK_NO_DOUBLE_BUFFERING
    const int y = 0;
#else
    const int y = dlPtr->y;
#endif /* TK_NO_DOUBLE_BUFFERING */

    if (dlPtr->chunkPtr == NULL) return;

    display = Tk_Display(textPtr->tkwin);

    height = dlPtr->height;
    if ((height + dlPtr->y) > dInfoPtr->maxY) {
	height = dInfoPtr->maxY - dlPtr->y;
    }
    if (dlPtr->y < dInfoPtr->y) {
	y_off = dInfoPtr->y - dlPtr->y;
	height -= y_off;
    } else {
	y_off = 0;
    }

#ifdef TK_NO_DOUBLE_BUFFERING
    TkpClipDrawableToRect(display, pixmap, dInfoPtr->x, y + y_off,
	    dInfoPtr->maxX - dInfoPtr->x, height);
#endif /* TK_NO_DOUBLE_BUFFERING */

    /*
     * First, clear the area of the line to the background color for the text
     * widget.
     */

    Tk_Fill3DRectangle(textPtr->tkwin, pixmap, textPtr->border, 0, y,
	    Tk_Width(textPtr->tkwin), dlPtr->height, 0, TK_RELIEF_FLAT);

    /*
     * Second, draw background information for the whole line.
     */

    DisplayLineBackground(textPtr, dlPtr, prevPtr, pixmap);

    /*
     * Third, draw the background color of the left and right margins.
     */
    if (dlPtr->lMarginColor != NULL) {
        Tk_Fill3DRectangle(textPtr->tkwin, pixmap, dlPtr->lMarginColor, 0, y,
                dlPtr->lMarginWidth + dInfoPtr->x - dInfoPtr->curXPixelOffset,
                dlPtr->height, 0, TK_RELIEF_FLAT);
    }
    if (dlPtr->rMarginColor != NULL) {
        Tk_Fill3DRectangle(textPtr->tkwin, pixmap, dlPtr->rMarginColor,
                dInfoPtr->maxX - dlPtr->rMarginWidth + dInfoPtr->curXPixelOffset,
                y, dlPtr->rMarginWidth, dlPtr->height, 0, TK_RELIEF_FLAT);
    }

    /*
     * Make another pass through all of the chunks to redraw the insertion
     * cursor, if it is visible on this line. Must do it here rather than in
     * the foreground pass below because otherwise a wide insertion cursor
     * will obscure the character to its left.
     */

    if (textPtr->state == TK_TEXT_STATE_NORMAL) {
	for (chunkPtr = dlPtr->chunkPtr; (chunkPtr != NULL);
		chunkPtr = chunkPtr->nextPtr) {
	    if (chunkPtr->displayProc == TkTextInsertDisplayProc) {
		int x = chunkPtr->x + dInfoPtr->x - dInfoPtr->curXPixelOffset;

		chunkPtr->displayProc(textPtr, chunkPtr, x,
			y + dlPtr->spaceAbove,
			dlPtr->height - dlPtr->spaceAbove - dlPtr->spaceBelow,
			dlPtr->baseline - dlPtr->spaceAbove, display, pixmap,
			dlPtr->y + dlPtr->spaceAbove);
	    }
	}
    }

    /*
     * Make yet another pass through all of the chunks to redraw all of
     * foreground information. Note: we have to call the displayProc even for
     * chunks that are off-screen. This is needed, for example, so that
     * embedded windows can be unmapped in this case.
     */

    for (chunkPtr = dlPtr->chunkPtr; (chunkPtr != NULL);
	    chunkPtr = chunkPtr->nextPtr) {
	if (chunkPtr->displayProc == TkTextInsertDisplayProc) {
	    /*
	     * Already displayed the insertion cursor above. Don't do it again
	     * here.
	     */

	    continue;
	}

	/*
	 * Don't call if elide. This tax OK since not very many visible DLines
	 * in an area, but potentially many elide ones.
	 */

	if (chunkPtr->displayProc != NULL) {
	    int x = chunkPtr->x + dInfoPtr->x - dInfoPtr->curXPixelOffset;

	    if ((x + chunkPtr->width <= 0) || (x >= dInfoPtr->maxX)) {
		/*
		 * Note: we have to call the displayProc even for chunks that
		 * are off-screen. This is needed, for example, so that
		 * embedded windows can be unmapped in this case. Display the
		 * chunk at a coordinate that can be clearly identified by the
		 * displayProc as being off-screen to the left (the
		 * displayProc may not be able to tell if something is off to
		 * the right).
		 */

		x = -chunkPtr->width;
	    }
	    chunkPtr->displayProc(textPtr, chunkPtr, x,
		    y + dlPtr->spaceAbove, dlPtr->height - dlPtr->spaceAbove -
		    dlPtr->spaceBelow, dlPtr->baseline - dlPtr->spaceAbove,
		    display, pixmap, dlPtr->y + dlPtr->spaceAbove);
	}

	if (dInfoPtr->dLinesInvalidated) {
	    return;
	}
    }

#ifndef TK_NO_DOUBLE_BUFFERING
    /*
     * Copy the pixmap onto the screen. If this is the first or last line on
     * the screen then copy a piece of the line, so that it doesn't overflow
     * into the border area. Another special trick: copy the padding area to
     * the left of the line; this is because the insertion cursor sometimes
     * overflows onto that area and we want to get as much of the cursor as
     * possible.
     */

    XCopyArea(display, pixmap, Tk_WindowId(textPtr->tkwin), dInfoPtr->copyGC,
	    dInfoPtr->x, y + y_off, (unsigned) (dInfoPtr->maxX - dInfoPtr->x),
	    (unsigned) height, dInfoPtr->x, dlPtr->y + y_off);
#else
    TkpClipDrawableToRect(display, pixmap, 0, 0, -1, -1);
#endif /* TK_NO_DOUBLE_BUFFERING */
    linesRedrawn++;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayLineBackground --
 *
 *	This function is called to fill in the background for a display line.
 *	It draws 3D borders cleverly so that adjacent chunks with the same
 *	style (whether on the same line or different lines) have a single 3D
 *	border around the whole region.
 *
 * Results:
 *	There is no return value. Pixmap is filled in with background
 *	information for dlPtr.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
DisplayLineBackground(
    TkText *textPtr,		/* Text widget containing line. */
    register DLine *dlPtr,	/* Information about line to draw. */
    DLine *prevPtr,		/* Line just above dlPtr, or NULL if dlPtr is
				 * the top-most line in the window. */
    Pixmap pixmap)		/* Pixmap to use for double-buffering. Caller
				 * must make sure it's large enough to hold
				 * line. Caller must also have filled it with
				 * the background color for the widget. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    TkTextDispChunk *chunkPtr;	/* Pointer to chunk in the current line. */
    TkTextDispChunk *chunkPtr2;	/* Pointer to chunk in the line above or below
				 * the current one. NULL if we're to the left
				 * of or to the right of the chunks in the
				 * line. */
    TkTextDispChunk *nextPtr2;	/* Next chunk after chunkPtr2 (it's not the
				 * same as chunkPtr2->nextPtr in the case
				 * where chunkPtr2 is NULL because the line is
				 * indented). */
    int leftX;			/* The left edge of the region we're currently
				 * working on. */
    int leftXIn;		/* 1 means beveled edge at leftX slopes right
				 * as it goes down, 0 means it slopes left as
				 * it goes down. */
    int rightX;			/* Right edge of chunkPtr. */
    int rightX2;		/* Right edge of chunkPtr2. */
    int matchLeft;		/* Does the style of this line match that of
				 * its neighbor just to the left of the
				 * current x coordinate? */
    int matchRight;		/* Does line's style match its neighbor just
				 * to the right of the current x-coord? */
    int minX, maxX, xOffset, bw;
    StyleValues *sValuePtr;
    Display *display;
#ifndef TK_NO_DOUBLE_BUFFERING
    const int y = 0;
#else
    const int y = dlPtr->y;
#endif /* TK_NO_DOUBLE_BUFFERING */

    /*
     * Pass 1: scan through dlPtr from left to right. For each range of chunks
     * with the same style, draw the main background for the style plus the
     * vertical parts of the 3D borders (the left and right edges).
     */

    display = Tk_Display(textPtr->tkwin);
    minX = dInfoPtr->curXPixelOffset;
    xOffset = dInfoPtr->x - minX;
    maxX = minX + dInfoPtr->maxX - dInfoPtr->x;
    chunkPtr = dlPtr->chunkPtr;

    /*
     * Note A: in the following statement, and a few others later in this file
     * marked with "See Note A above", the right side of the assignment was
     * replaced with 0 on 6/18/97. This has the effect of highlighting the
     * empty space to the left of a line whenever the leftmost character of
     * the line is highlighted. This way, multi-line highlights always line up
     * along their left edges. However, this may look funny in the case where
     * a single word is highlighted. To undo the change, replace "leftX = 0"
     * with "leftX = chunkPtr->x" and "rightX2 = 0" with "rightX2 =
     * nextPtr2->x" here and at all the marked points below. This restores the
     * old behavior where empty space to the left of a line is not
     * highlighted, leaving a ragged left edge for multi-line highlights.
     */

    leftX = 0;
    for (; leftX < maxX; chunkPtr = chunkPtr->nextPtr) {
	if ((chunkPtr->nextPtr != NULL)
		&& SAME_BACKGROUND(chunkPtr->nextPtr->stylePtr,
		chunkPtr->stylePtr)) {
	    continue;
	}
	sValuePtr = chunkPtr->stylePtr->sValuePtr;
	rightX = chunkPtr->x + chunkPtr->width;
	if ((chunkPtr->nextPtr == NULL) && (rightX < maxX)) {
	    rightX = maxX;
	}
	if (chunkPtr->stylePtr->bgGC != NULL) {
	    /*
	     * Not visible - bail out now.
	     */

	    if (rightX + xOffset <= 0) {
		leftX = rightX;
		continue;
	    }

	    /*
	     * Trim the start position for drawing to be no further away than
	     * -borderWidth. The reason is that on many X servers drawing from
	     * -32768 (or less) to +something simply does not display
	     * correctly. [Patch #541999]
	     */

	    if ((leftX + xOffset) < -(sValuePtr->borderWidth)) {
		leftX = -sValuePtr->borderWidth - xOffset;
	    }
	    if ((rightX - leftX) > 32767) {
		rightX = leftX + 32767;
	    }

            /*
             * Prevent the borders from leaking on adjacent characters,
             * which would happen for too large border width.
             */

            bw = sValuePtr->borderWidth;
            if (leftX + sValuePtr->borderWidth > rightX) {
                bw = rightX - leftX;
            }

	    XFillRectangle(display, pixmap, chunkPtr->stylePtr->bgGC,
		    leftX + xOffset, y, (unsigned int) (rightX - leftX),
		    (unsigned int) dlPtr->height);
	    if (sValuePtr->relief != TK_RELIEF_FLAT) {
		Tk_3DVerticalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
			leftX + xOffset, y, bw, dlPtr->height, 1,
			sValuePtr->relief);
		Tk_3DVerticalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
			rightX - bw + xOffset, y, bw, dlPtr->height, 0,
			sValuePtr->relief);
	    }
	}
	leftX = rightX;
    }

    /*
     * Pass 2: draw the horizontal bevels along the top of the line. To do
     * this, scan through dlPtr from left to right while simultaneously
     * scanning through the line just above dlPtr. ChunkPtr2 and nextPtr2
     * refer to two adjacent chunks in the line above.
     */

    chunkPtr = dlPtr->chunkPtr;
    leftX = 0;				/* See Note A above. */
    leftXIn = 1;
    rightX = chunkPtr->x + chunkPtr->width;
    if ((chunkPtr->nextPtr == NULL) && (rightX < maxX)) {
	rightX = maxX;
    }
    chunkPtr2 = NULL;
    if (prevPtr != NULL && prevPtr->chunkPtr != NULL) {
	/*
	 * Find the chunk in the previous line that covers leftX.
	 */

	nextPtr2 = prevPtr->chunkPtr;
	rightX2 = 0;			/* See Note A above. */
	while (rightX2 <= leftX) {
	    chunkPtr2 = nextPtr2;
	    if (chunkPtr2 == NULL) {
		break;
	    }
	    nextPtr2 = chunkPtr2->nextPtr;
	    rightX2 = chunkPtr2->x + chunkPtr2->width;
	    if (nextPtr2 == NULL) {
		rightX2 = INT_MAX;
	    }
	}
    } else {
	nextPtr2 = NULL;
	rightX2 = INT_MAX;
    }

    while (leftX < maxX) {
	matchLeft = (chunkPtr2 != NULL)
		&& SAME_BACKGROUND(chunkPtr2->stylePtr, chunkPtr->stylePtr);
	sValuePtr = chunkPtr->stylePtr->sValuePtr;
	if (rightX <= rightX2) {
	    /*
	     * The chunk in our line is about to end. If its style changes
	     * then draw the bevel for the current style.
	     */

	    if ((chunkPtr->nextPtr == NULL)
		    || !SAME_BACKGROUND(chunkPtr->stylePtr,
		    chunkPtr->nextPtr->stylePtr)) {
		if (!matchLeft && (sValuePtr->relief != TK_RELIEF_FLAT)) {
		    Tk_3DHorizontalBevel(textPtr->tkwin, pixmap,
			    sValuePtr->border, leftX + xOffset, y,
			    rightX - leftX, sValuePtr->borderWidth, leftXIn,
			    1, 1, sValuePtr->relief);
		}
		leftX = rightX;
		leftXIn = 1;

		/*
		 * If the chunk in the line above is also ending at the same
		 * point then advance to the next chunk in that line.
		 */

		if ((rightX == rightX2) && (chunkPtr2 != NULL)) {
		    goto nextChunk2;
		}
	    }
	    chunkPtr = chunkPtr->nextPtr;
	    if (chunkPtr == NULL) {
		break;
	    }
	    rightX = chunkPtr->x + chunkPtr->width;
	    if ((chunkPtr->nextPtr == NULL) && (rightX < maxX)) {
		rightX = maxX;
	    }
	    continue;
	}

	/*
	 * The chunk in the line above is ending at an x-position where there
	 * is no change in the style of the current line. If the style above
	 * matches the current line on one side of the change but not on the
	 * other, we have to draw an L-shaped piece of bevel.
	 */

	matchRight = (nextPtr2 != NULL)
		&& SAME_BACKGROUND(nextPtr2->stylePtr, chunkPtr->stylePtr);
	if (matchLeft && !matchRight) {
            bw = sValuePtr->borderWidth;
            if (rightX2 - sValuePtr->borderWidth < leftX) {
                bw = rightX2 - leftX;
            }
	    if (sValuePtr->relief != TK_RELIEF_FLAT) {
		Tk_3DVerticalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
			rightX2 - bw + xOffset, y, bw,
			sValuePtr->borderWidth, 0, sValuePtr->relief);
	    }
            leftX = rightX2 - bw;
	    leftXIn = 0;
	} else if (!matchLeft && matchRight
		&& (sValuePtr->relief != TK_RELIEF_FLAT)) {
            bw = sValuePtr->borderWidth;
            if (rightX2 + sValuePtr->borderWidth > rightX) {
                bw = rightX - rightX2;
            }
	    Tk_3DVerticalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
		    rightX2 + xOffset, y, bw, sValuePtr->borderWidth,
		    1, sValuePtr->relief);
	    Tk_3DHorizontalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
		    leftX + xOffset, y, rightX2 + bw - leftX,
		    sValuePtr->borderWidth, leftXIn, 0, 1,
		    sValuePtr->relief);
	}

    nextChunk2:
	chunkPtr2 = nextPtr2;
	if (chunkPtr2 == NULL) {
	    rightX2 = INT_MAX;
	} else {
	    nextPtr2 = chunkPtr2->nextPtr;
	    rightX2 = chunkPtr2->x + chunkPtr2->width;
	    if (nextPtr2 == NULL) {
		rightX2 = INT_MAX;
	    }
	}
    }

    /*
     * Pass 3: draw the horizontal bevels along the bottom of the line. This
     * uses the same approach as pass 2.
     */

    chunkPtr = dlPtr->chunkPtr;
    leftX = 0;				/* See Note A above. */
    leftXIn = 0;
    rightX = chunkPtr->x + chunkPtr->width;
    if ((chunkPtr->nextPtr == NULL) && (rightX < maxX)) {
	rightX = maxX;
    }
    chunkPtr2 = NULL;
    if (dlPtr->nextPtr != NULL && dlPtr->nextPtr->chunkPtr != NULL) {
	/*
	 * Find the chunk in the next line that covers leftX.
	 */

	nextPtr2 = dlPtr->nextPtr->chunkPtr;
	rightX2 = 0;			/* See Note A above. */
	while (rightX2 <= leftX) {
	    chunkPtr2 = nextPtr2;
	    if (chunkPtr2 == NULL) {
		break;
	    }
	    nextPtr2 = chunkPtr2->nextPtr;
	    rightX2 = chunkPtr2->x + chunkPtr2->width;
	    if (nextPtr2 == NULL) {
		rightX2 = INT_MAX;
	    }
	}
    } else {
	nextPtr2 = NULL;
	rightX2 = INT_MAX;
    }

    while (leftX < maxX) {
	matchLeft = (chunkPtr2 != NULL)
		&& SAME_BACKGROUND(chunkPtr2->stylePtr, chunkPtr->stylePtr);
	sValuePtr = chunkPtr->stylePtr->sValuePtr;
	if (rightX <= rightX2) {
	    if ((chunkPtr->nextPtr == NULL)
		    || !SAME_BACKGROUND(chunkPtr->stylePtr,
		    chunkPtr->nextPtr->stylePtr)) {
		if (!matchLeft && (sValuePtr->relief != TK_RELIEF_FLAT)) {
		    Tk_3DHorizontalBevel(textPtr->tkwin, pixmap,
			    sValuePtr->border, leftX + xOffset,
			    y + dlPtr->height - sValuePtr->borderWidth,
			    rightX - leftX, sValuePtr->borderWidth, leftXIn,
			    0, 0, sValuePtr->relief);
		}
		leftX = rightX;
		leftXIn = 0;
		if ((rightX == rightX2) && (chunkPtr2 != NULL)) {
		    goto nextChunk2b;
		}
	    }
	    chunkPtr = chunkPtr->nextPtr;
	    if (chunkPtr == NULL) {
		break;
	    }
	    rightX = chunkPtr->x + chunkPtr->width;
	    if ((chunkPtr->nextPtr == NULL) && (rightX < maxX)) {
		rightX = maxX;
	    }
	    continue;
	}

	matchRight = (nextPtr2 != NULL)
		&& SAME_BACKGROUND(nextPtr2->stylePtr, chunkPtr->stylePtr);
	if (matchLeft && !matchRight) {
            bw = sValuePtr->borderWidth;
            if (rightX2 - sValuePtr->borderWidth < leftX) {
                bw = rightX2 - leftX;
            }
	    if (sValuePtr->relief != TK_RELIEF_FLAT) {
		Tk_3DVerticalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
			rightX2 - bw + xOffset,
			y + dlPtr->height - sValuePtr->borderWidth,
			bw, sValuePtr->borderWidth, 0, sValuePtr->relief);
	    }
	    leftX = rightX2 - bw;
	    leftXIn = 1;
	} else if (!matchLeft && matchRight
		&& (sValuePtr->relief != TK_RELIEF_FLAT)) {
            bw = sValuePtr->borderWidth;
            if (rightX2 + sValuePtr->borderWidth > rightX) {
                bw = rightX - rightX2;
            }
	    Tk_3DVerticalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
		    rightX2 + xOffset,
		    y + dlPtr->height - sValuePtr->borderWidth, bw,
		    sValuePtr->borderWidth, 1, sValuePtr->relief);
	    Tk_3DHorizontalBevel(textPtr->tkwin, pixmap, sValuePtr->border,
		    leftX + xOffset,
		    y + dlPtr->height - sValuePtr->borderWidth,
		    rightX2 + bw - leftX, sValuePtr->borderWidth, leftXIn,
		    1, 0, sValuePtr->relief);
	}

    nextChunk2b:
	chunkPtr2 = nextPtr2;
	if (chunkPtr2 == NULL) {
	    rightX2 = INT_MAX;
	} else {
	    nextPtr2 = chunkPtr2->nextPtr;
	    rightX2 = chunkPtr2->x + chunkPtr2->width;
	    if (nextPtr2 == NULL) {
		rightX2 = INT_MAX;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AsyncUpdateLineMetrics --
 *
 *	This function is invoked as a background handler to update the pixel-
 *	height calculations of individual lines in an asychronous manner.
 *
 *	Currently a timer-handler is used for this purpose, which continuously
 *	reschedules itself. It may well be better to use some other approach
 *	(e.g., a background thread). We can't use an idle-callback because of
 *	a known bug in Tcl/Tk in which idle callbacks are not allowed to
 *	re-schedule themselves. This just causes an effective infinite loop.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Line heights may be recalculated.
 *
 *----------------------------------------------------------------------
 */

static void
AsyncUpdateLineMetrics(
    ClientData clientData)	/* Information about widget. */
{
    register TkText *textPtr = clientData;
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    int lineNum;

    dInfoPtr->lineUpdateTimer = NULL;

    if ((textPtr->tkwin == NULL) || (textPtr->flags & DESTROYED)
            || !Tk_IsMapped(textPtr->tkwin)) {
	/*
	 * The widget has been deleted, or is not mapped. Don't do anything.
	 */

	if (textPtr->refCount-- <= 1) {
	    ckfree(textPtr);
	}
	return;
    }

    if (dInfoPtr->flags & REDRAW_PENDING) {
	dInfoPtr->lineUpdateTimer = Tcl_CreateTimerHandler(1,
		AsyncUpdateLineMetrics, clientData);
	return;
    }

    /*
     * Reify where we end or all hell breaks loose with the calculations when
     * we try to update. [Bug 2677890]
     */

    lineNum = dInfoPtr->currentMetricUpdateLine;
    if (dInfoPtr->lastMetricUpdateLine == -1) {
	dInfoPtr->lastMetricUpdateLine =
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr);
    }

    /*
     * Update the lines in blocks of about 24 recalculations, or 250+ lines
     * examined, so we pass in 256 for 'doThisMuch'.
     */

    lineNum = TkTextUpdateLineMetrics(textPtr, lineNum,
	    dInfoPtr->lastMetricUpdateLine, 256);

    dInfoPtr->currentMetricUpdateLine = lineNum;

    if (tkTextDebug) {
	char buffer[2 * TCL_INTEGER_SPACE + 1];

	sprintf(buffer, "%d %d", lineNum, dInfoPtr->lastMetricUpdateLine);
	LOG("tk_textInvalidateLine", buffer);
    }

    /*
     * If we're not in the middle of a long-line calculation (metricEpoch==-1)
     * and we've reached the last line, then we're done.
     */

    if (dInfoPtr->metricEpoch == -1
	    && lineNum == dInfoPtr->lastMetricUpdateLine) {
	/*
	 * We have looped over all lines, so we're done. We must release our
	 * refCount on the widget (the timer token was already set to NULL
	 * above). If there is a registered aftersync command, run that first.
	 * Cancel any pending idle task which would try to run the command
	 * after the afterSyncCmd pointer had been set to NULL.
	 */

        if (textPtr->afterSyncCmd) {
            int code;
	    Tcl_CancelIdleCall(TkTextRunAfterSyncCmd, textPtr);
            Tcl_Preserve((ClientData) textPtr->interp);
            code = Tcl_EvalObjEx(textPtr->interp, textPtr->afterSyncCmd,
                    TCL_EVAL_GLOBAL);
	    if (code == TCL_ERROR) {
                Tcl_AddErrorInfo(textPtr->interp, "\n    (text sync)");
                Tcl_BackgroundError(textPtr->interp);
	    }
            Tcl_Release((ClientData) textPtr->interp);
            Tcl_DecrRefCount(textPtr->afterSyncCmd);
            textPtr->afterSyncCmd = NULL;
	}

        /*
         * Fire the <<WidgetViewSync>> event since the widget view is in sync
         * with its internal data (actually it will be after the next trip
         * through the event loop, because the widget redraws at idle-time).
         */
        GenerateWidgetViewSyncEvent(textPtr, 1);

	if (textPtr->refCount-- <= 1) {
	    ckfree(textPtr);
	}
	return;
    }

    /*
     * Re-arm the timer. We already have a refCount on the text widget so no
     * need to adjust that.
     */

    dInfoPtr->lineUpdateTimer = Tcl_CreateTimerHandler(1,
	    AsyncUpdateLineMetrics, textPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateWidgetViewSyncEvent --
 *
 *      Send the <<WidgetViewSync>> event related to the text widget
 *      line metrics asynchronous update.  These events should only
 *      be sent when the sync status has changed.  So this function
 *      compares the requested state with the state saved in the
 *      TkText structure, and only generates the event if they are
 *      different.  This means that it is safe to call this function
 *      at any time when the state is known.
 *
 *      If an event is sent, the effect is equivalent to:
 *         event generate $textWidget <<WidgetViewSync>> -data $s
 *      where $s is the sync status: true (when the widget view is in
 *      sync with its internal data) or false (when it is not).
 *
 * Results:
 *      None
 *
 * Side effects:
 *      If corresponding bindings are present, they will trigger.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateWidgetViewSyncEvent(
    TkText *textPtr,	  /* Information about text widget. */
    Bool InSync)          /* true if becoming in sync, false otherwise */
{
    Bool NewSyncState = (InSync != 0); /* ensure 0 or 1 value */
    Bool OldSyncState = !(textPtr->dInfoPtr->flags & OUT_OF_SYNC);

    /*
     * OSX 10.14 needs to be told to display the window when the Text Widget
     * is in sync.  (That is, to run DisplayText inside of the drawRect
     * method.)  Otherwise the screen might not get updated until an event
     * like a mouse click is received.  But that extra drawing corrupts the
     * data that the test suite is trying to collect.
     */

    if (!tkTextDebug) {
	FORCE_DISPLAY(textPtr->tkwin);
    }

    if (NewSyncState != OldSyncState) {
	if (NewSyncState) {
	    textPtr->dInfoPtr->flags &= ~OUT_OF_SYNC;
	} else {
	    textPtr->dInfoPtr->flags |= OUT_OF_SYNC;
	}
        TkSendVirtualEvent(textPtr->tkwin, "WidgetViewSync",
                           Tcl_NewBooleanObj(NewSyncState));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextUpdateLineMetrics --
 *
 *	This function updates the pixel height calculations of a range of
 *	lines in the widget. The range is from lineNum to endLine, but, if
 *	doThisMuch is positive, then the function may return earlier, once a
 *	certain number of lines has been examined. The line counts are from 0.
 *
 *	If doThisMuch is -1, then all lines in the range will be updated. This
 *	will potentially take quite some time for a large text widget.
 *
 *	Note: with bad input for lineNum and endLine, this function can loop
 *	indefinitely.
 *
 * Results:
 *	The index of the last line examined (or -1 if we are about to wrap
 *	around from end to beginning of the widget, and the next line will be
 *	the first line).
 *
 * Side effects:
 *	Line heights may be recalculated.
 *
 *----------------------------------------------------------------------
 */

int
TkTextUpdateLineMetrics(
    TkText *textPtr,		/* Information about widget. */
    int lineNum,		/* Start at this line. */
    int endLine,		/* Go no further than this line. */
    int doThisMuch)		/* How many lines to check, or how many 10s of
				 * lines to recalculate. If '-1' then do
				 * everything in the range (which may take a
				 * while). */
{
    TkTextLine *linePtr = NULL;
    int count = 0;
    int totalLines = TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr);
    int fullUpdateRequested = (lineNum == 0 &&
                               endLine == totalLines &&
                               doThisMuch == -1);

    if (totalLines == 0) {
	/*
	 * Empty peer widget.
	 */

	return endLine;
    }

    while (1) {

	/*
	 * Get a suitable line.
	 */

	if (lineNum == -1 && linePtr == NULL) {
	    lineNum = 0;
	    linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr,
		    lineNum);
	} else {
	    if (lineNum == -1 || linePtr == NULL) {
		if (lineNum == -1) {
		    lineNum = 0;
		}
		linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree,
			textPtr, lineNum);
	    } else {
		lineNum++;
		linePtr = TkBTreeNextLine(textPtr, linePtr);
	    }

	    /*
	     * If we're in the middle of a partial-line height calculation,
	     * then we can't be done.
	     */

	    if (textPtr->dInfoPtr->metricEpoch == -1 && lineNum == endLine) {

		/*
		 * We have looped over all lines, so we're done.
		 */

		break;
	    }
	}

	if (lineNum < totalLines) {
	    if (tkTextDebug) {
		char buffer[4 * TCL_INTEGER_SPACE + 3];

		sprintf(buffer, "%d %d %d %d",
			lineNum, endLine, totalLines, count);
		LOG("tk_textInvalidateLine", buffer);
	    }

	    /*
	     * Now update the line's metrics if necessary.
	     */

	    if (TkBTreeLinePixelEpoch(textPtr, linePtr)
		    == textPtr->dInfoPtr->lineMetricUpdateEpoch) {

		/*
		 * This line is already up to date. That means there's nothing
		 * to do here.
		 */

	    } else if (doThisMuch == -1) {
		count += 8 * TkTextUpdateOneLine(textPtr, linePtr, 0,NULL,0);
	    } else {
		TkTextIndex index;
		TkTextIndex *indexPtr;
		int pixelHeight;

		/*
		 * If the metric epoch is the same as the widget's epoch, then
		 * we know that indexPtrs are still valid, and if the cached
		 * metricIndex (if any) is for the same line as we wish to
		 * examine, then we are looking at a long line wrapped many
		 * times, which we will examine in pieces.
		 */

		if (textPtr->dInfoPtr->metricEpoch ==
			textPtr->sharedTextPtr->stateEpoch &&
			textPtr->dInfoPtr->metricIndex.linePtr==linePtr) {
		    indexPtr = &textPtr->dInfoPtr->metricIndex;
		    pixelHeight = textPtr->dInfoPtr->metricPixelHeight;
		} else {

		    /*
		     * We must reset the partial line height calculation data
		     * here, so we don't use it when it is out of date.
		     */

		    textPtr->dInfoPtr->metricEpoch = -1;
		    index.tree = textPtr->sharedTextPtr->tree;
		    index.linePtr = linePtr;
		    index.byteIndex = 0;
		    index.textPtr = NULL;
		    indexPtr = &index;
		    pixelHeight = 0;
		}

		/*
		 * Update the line and update the counter, counting 8 for each
		 * display line we actually re-layout.
		 */

		count += 8 * TkTextUpdateOneLine(textPtr, linePtr,
			pixelHeight, indexPtr, 1);

		if (indexPtr->linePtr == linePtr) {

		    /*
		     * We didn't complete the logical line, because it
		     * produced very many display lines, which must be because
		     * it must be a long line wrapped many times. So we must
		     * cache as far as we got for next time around.
		     */

		    if (pixelHeight == 0) {

			/*
			 * These have already been stored, unless we just
			 * started the new line.
			 */

			textPtr->dInfoPtr->metricIndex = index;
			textPtr->dInfoPtr->metricEpoch =
				textPtr->sharedTextPtr->stateEpoch;
		    }
		    textPtr->dInfoPtr->metricPixelHeight =
			    TkBTreeLinePixelCount(textPtr, linePtr);
		    break;
		}

		/*
		 * We're done with this long line.
		 */

		textPtr->dInfoPtr->metricEpoch = -1;
	    }
	} else {

	    /*
	     * We must never recalculate the height of the last artificial
	     * line. It must stay at zero, and if we recalculate it, it will
	     * change.
	     */

	    if (endLine >= totalLines) {
		lineNum = endLine;
		break;
	    }

	    /*
	     * Set things up for the next loop through.
	     */

	    lineNum = -1;
	}
	count++;

	if (doThisMuch != -1 && count >= doThisMuch) {
	    break;
	}
    }
    if (doThisMuch == -1) {

	/*
	 * If we were requested to update the entire range, then also update
	 * the scrollbar.
	 */

	GetYView(textPtr->interp, textPtr, 1);
    }
    if (fullUpdateRequested) {
        GenerateWidgetViewSyncEvent(textPtr, 1);
    }
    return lineNum;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextInvalidateLineMetrics, TextInvalidateLineMetrics --
 *
 *	Mark a number of text lines as having invalid line metric
 *	calculations. Never call this with linePtr as the last (artificial)
 *	line in the text. Depending on 'action' which indicates whether the
 *	given lines are simply invalid or have been inserted or deleted, the
 *	pre-existing asynchronous line update range may need to be adjusted.
 *
 *	If linePtr is NULL then 'lineCount' and 'action' are ignored and all
 *	lines are invalidated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May schedule an asychronous callback.
 *
 *----------------------------------------------------------------------
 */

void
TkTextInvalidateLineMetrics(
    TkSharedText *sharedTextPtr,/* Shared widget section for all peers, or
				 * NULL. */
    TkText *textPtr,		/* Widget record for text widget. */
    TkTextLine *linePtr,	/* Invalidation starts from this line. */
    int lineCount,		/* And includes this many following lines. */
    int action)			/* Indicates what type of invalidation
				 * occurred (insert, delete, or simple). */
{
    if (sharedTextPtr == NULL) {
	TextInvalidateLineMetrics(textPtr, linePtr, lineCount, action);
    } else {
	textPtr = sharedTextPtr->peers;
	while (textPtr != NULL) {
	    TextInvalidateLineMetrics(textPtr, linePtr, lineCount, action);
	    textPtr = textPtr->next;
	}
    }
}

static void
TextInvalidateLineMetrics(
    TkText *textPtr,		/* Widget record for text widget. */
    TkTextLine *linePtr,	/* Invalidation starts from this line. */
    int lineCount,		/* And includes this many following lines. */
    int action)			/* Indicates what type of invalidation
				 * occurred (insert, delete, or simple). */
{
    int fromLine;
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;

    if (linePtr != NULL) {
	int counter = lineCount;

	fromLine = TkBTreeLinesTo(textPtr, linePtr);

	/*
	 * Invalidate the height calculations of each line in the given range.
	 */

	TkBTreeLinePixelEpoch(textPtr, linePtr) = 0;
	while (counter > 0 && linePtr != NULL) {
	    linePtr = TkBTreeNextLine(textPtr, linePtr);
	    if (linePtr != NULL) {
		TkBTreeLinePixelEpoch(textPtr, linePtr) = 0;
	    }
	    counter--;
	}

	/*
	 * Now schedule an examination of each line in the union of the old
	 * and new update ranges, including the (possibly empty) range in
	 * between. If that between range is not-empty, then we are examining
	 * more lines than is strictly necessary (but the examination of the
	 * extra lines should be quick, since their pixelCalculationEpoch will
	 * be up to date). However, to keep track of that would require more
	 * complex record-keeping than what we have.
	 */

	if (dInfoPtr->lineUpdateTimer == NULL) {
	    dInfoPtr->currentMetricUpdateLine = fromLine;
	    if (action == TK_TEXT_INVALIDATE_DELETE) {
		lineCount = 0;
	    }
	    dInfoPtr->lastMetricUpdateLine = fromLine + lineCount + 1;
	} else {
	    int toLine = fromLine + lineCount + 1;

	    if (action == TK_TEXT_INVALIDATE_DELETE) {
		if (toLine <= dInfoPtr->currentMetricUpdateLine) {
		    dInfoPtr->currentMetricUpdateLine = fromLine;
		    if (dInfoPtr->lastMetricUpdateLine != -1) {
			dInfoPtr->lastMetricUpdateLine -= lineCount;
		    }
		} else if (fromLine <= dInfoPtr->currentMetricUpdateLine) {
		    dInfoPtr->currentMetricUpdateLine = fromLine;
		    if (toLine <= dInfoPtr->lastMetricUpdateLine) {
			dInfoPtr->lastMetricUpdateLine -= lineCount;
		    }
		} else {
		    if (dInfoPtr->lastMetricUpdateLine != -1) {
			dInfoPtr->lastMetricUpdateLine = toLine;
		    }
		}
	    } else if (action == TK_TEXT_INVALIDATE_INSERT) {
		if (toLine <= dInfoPtr->currentMetricUpdateLine) {
		    dInfoPtr->currentMetricUpdateLine = fromLine;
		    if (dInfoPtr->lastMetricUpdateLine != -1) {
			dInfoPtr->lastMetricUpdateLine += lineCount;
		    }
		} else if (fromLine <= dInfoPtr->currentMetricUpdateLine) {
		    dInfoPtr->currentMetricUpdateLine = fromLine;
		    if (toLine <= dInfoPtr->lastMetricUpdateLine) {
			dInfoPtr->lastMetricUpdateLine += lineCount;
		    }
		    if (toLine > dInfoPtr->lastMetricUpdateLine) {
			dInfoPtr->lastMetricUpdateLine = toLine;
		    }
		} else {
		    if (dInfoPtr->lastMetricUpdateLine != -1) {
			dInfoPtr->lastMetricUpdateLine = toLine;
		    }
		}
	    } else {
		if (fromLine < dInfoPtr->currentMetricUpdateLine) {
		    dInfoPtr->currentMetricUpdateLine = fromLine;
		}
		if (dInfoPtr->lastMetricUpdateLine != -1
			&& toLine > dInfoPtr->lastMetricUpdateLine) {
		    dInfoPtr->lastMetricUpdateLine = toLine;
		}
	    }
	}
    } else {
	/*
	 * This invalidates the height of all lines in the widget.
	 */

	if ((++dInfoPtr->lineMetricUpdateEpoch) == 0) {
	    dInfoPtr->lineMetricUpdateEpoch++;
	}

	/*
	 * This has the effect of forcing an entire new loop of update checks
	 * on all lines in the widget.
	 */

	if (dInfoPtr->lineUpdateTimer == NULL) {
	    dInfoPtr->currentMetricUpdateLine = -1;
	}
	dInfoPtr->lastMetricUpdateLine = dInfoPtr->currentMetricUpdateLine;
    }

    /*
     * Now re-set the current update calculations.
     */

    if (dInfoPtr->lineUpdateTimer == NULL) {
	textPtr->refCount++;
	dInfoPtr->lineUpdateTimer = Tcl_CreateTimerHandler(1,
		AsyncUpdateLineMetrics, textPtr);
    }

    /*
     * The widget is out of sync: send a <<WidgetViewSync>> event.
     */
    GenerateWidgetViewSyncEvent(textPtr, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextFindDisplayLineEnd --
 *
 *	This function is invoked to find the index of the beginning or end of
 *	the particular display line on which the given index sits, whether
 *	that line is displayed or not.
 *
 *	If 'end' is zero, we look for the start, and if 'end' is one we look
 *	for the end.
 *
 *	If the beginning of the current display line is elided, and we are
 *	looking for the start of the line, then the returned index will be the
 *	first elided index on the display line.
 *
 *	Similarly if the end of the current display line is elided and we are
 *	looking for the end, then the returned index will be the last elided
 *	index on the display line.
 *
 * Results:
 *	Modifies indexPtr to point to the given end.
 *
 *	If xOffset is non-NULL, it is set to the x-pixel offset of the given
 *	original index within the given display line.
 *
 * Side effects:
 *	The combination of 'LayoutDLine' and 'FreeDLines' seems like a rather
 *	time-consuming way of gathering the information we need, so this would
 *	be a good place to look to speed up the calculations. In particular
 *	these calls will map and unmap embedded windows respectively, which I
 *	would hope isn't exactly necessary!
 *
 *----------------------------------------------------------------------
 */

void
TkTextFindDisplayLineEnd(
    TkText *textPtr,		/* Widget record for text widget. */
    TkTextIndex *indexPtr,	/* Index we will adjust to the display line
				 * start or end. */
    int end,			/* 0 = start, 1 = end. */
    int *xOffset)		/* NULL, or used to store the x-pixel offset
				 * of the original index within its display
				 * line. */
{
    TkTextIndex index;

    if (!end && IsStartOfNotMergedLine(textPtr, indexPtr)) {
	/*
	 * Nothing to do.
	 */

	if (xOffset != NULL) {
	    *xOffset = 0;
	}
	return;
    }

    index = *indexPtr;
    index.byteIndex = 0;
    index.textPtr = NULL;

    while (1) {
	TkTextIndex endOfLastLine;

	if (TkTextIndexBackBytes(textPtr, &index, 1, &endOfLastLine)) {
	    /*
	     * Reached beginning of text.
	     */

	    break;
	}

	if (!TkTextIsElided(textPtr, &endOfLastLine, NULL)) {
	    /*
	     * The eol is not elided, so 'index' points to the start of a
	     * display line (as well as logical line).
	     */

	    break;
	}

	/*
	 * indexPtr's logical line is actually merged with the previous
	 * logical line whose eol is elided. Continue searching back to get a
	 * real line start.
	 */

	index = endOfLastLine;
	index.byteIndex = 0;
    }

    while (1) {
	DLine *dlPtr;
	int byteCount;
	TkTextIndex nextLineStart;

	dlPtr = LayoutDLine(textPtr, &index);
	byteCount = dlPtr->byteCount;

	TkTextIndexForwBytes(textPtr, &index, byteCount, &nextLineStart);

	/*
	 * 'byteCount' goes up to the beginning of the next display line, so
	 * equality here says we need one more line. We try to perform a quick
	 * comparison which is valid for the case where the logical line is
	 * the same, but otherwise fall back on a full TkTextIndexCmp.
	 */

	if (((index.linePtr == indexPtr->linePtr)
		&& (index.byteIndex + byteCount > indexPtr->byteIndex))
		|| (dlPtr->logicalLinesMerged > 0
		&& TkTextIndexCmp(&nextLineStart, indexPtr) > 0)) {
	    /*
	     * It's on this display line.
	     */

	    if (xOffset != NULL) {
		/*
		 * This call takes a byte index relative to the start of the
		 * current _display_ line, not logical line. We are about to
		 * overwrite indexPtr->byteIndex, so we must do this now.
		 */

		*xOffset = DlineXOfIndex(textPtr, dlPtr,
			TkTextIndexCountBytes(textPtr, &dlPtr->index,
			indexPtr));
	    }
	    if (end) {
		/*
		 * The index we want is one less than the number of bytes in
		 * the display line.
		 */

		TkTextIndexBackBytes(textPtr, &nextLineStart, 1, indexPtr);
	    } else {
		*indexPtr = index;
	    }
	    FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);
	    return;
	}

	FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);
	index = nextLineStart;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CalculateDisplayLineHeight --
 *
 *	This function is invoked to recalculate the height of the particular
 *	display line which starts with the given index, whether that line is
 *	displayed or not.
 *
 *	This function does not, in itself, update any cached information about
 *	line heights. That should be done, where necessary, by its callers.
 *
 *	The behaviour of this function is _undefined_ if indexPtr is not
 *	currently at the beginning of a display line.
 *
 * Results:
 *	The number of vertical pixels used by the display line.
 *
 *	If 'byteCountPtr' is non-NULL, then returns in that pointer the number
 *	of byte indices on the given display line (which can be used to update
 *	indexPtr in a loop).
 *
 *	If 'mergedLinePtr' is non-NULL, then returns in that pointer the
 *	number of extra logical lines merged into the given display line.
 *
 * Side effects:
 *	The combination of 'LayoutDLine' and 'FreeDLines' seems like a rather
 *	time-consuming way of gathering the information we need, so this would
 *	be a good place to look to speed up the calculations. In particular
 *	these calls will map and unmap embedded windows respectively, which I
 *	would hope isn't exactly necessary!
 *
 *----------------------------------------------------------------------
 */

static int
CalculateDisplayLineHeight(
    TkText *textPtr,		/* Widget record for text widget. */
    const TkTextIndex *indexPtr,/* The index at the beginning of the display
				 * line of interest. */
    int *byteCountPtr,		/* NULL or used to return the number of byte
				 * indices on the given display line. */
    int *mergedLinePtr)		/* NULL or used to return if the given display
				 * line merges with a following logical line
				 * (because the eol is elided). */
{
    DLine *dlPtr;
    int pixelHeight;

    if (tkTextDebug) {
        int oldtkTextDebug = tkTextDebug;
        /*
         * Check that the indexPtr we are given really is at the start of a
         * display line. The gymnastics with tkTextDebug is to prevent
         * failure of a test suite test, that checks that lines are rendered
         * exactly once. TkTextFindDisplayLineEnd is used here for checking
         * indexPtr but it calls LayoutDLine/FreeDLine which makes the
         * counting wrong. The debug mode shall therefore be switched off
         * when calling TkTextFindDisplayLineEnd.
         */

        TkTextIndex indexPtr2 = *indexPtr;
        tkTextDebug = 0;
        TkTextFindDisplayLineEnd(textPtr, &indexPtr2, 0, NULL);
        tkTextDebug = oldtkTextDebug;
        if (TkTextIndexCmp(&indexPtr2,indexPtr) != 0) {
            Tcl_Panic("CalculateDisplayLineHeight called with bad indexPtr");
        }
    }

    /*
     * Special case for artificial last line. May be better to move this
     * inside LayoutDLine.
     */

    if (indexPtr->byteIndex == 0
	    && TkBTreeNextLine(textPtr, indexPtr->linePtr) == NULL) {
	if (byteCountPtr != NULL) {
	    *byteCountPtr = 0;
	}
	if (mergedLinePtr != NULL) {
	    *mergedLinePtr = 0;
	}
	return 0;
    }

    /*
     * Layout, find the information we need and then free the display-line we
     * laid-out. We must use 'FreeDLines' because it will actually call the
     * relevant code to unmap any embedded windows which were mapped in the
     * LayoutDLine call!
     */

    dlPtr = LayoutDLine(textPtr, indexPtr);
    pixelHeight = dlPtr->height;
    if (byteCountPtr != NULL) {
	*byteCountPtr = dlPtr->byteCount;
    }
    if (mergedLinePtr != NULL) {
	*mergedLinePtr = dlPtr->logicalLinesMerged;
    }
    FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);

    return pixelHeight;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextIndexYPixels --
 *
 *	This function is invoked to calculate the number of vertical pixels
 *	between the first index of the text widget and the given index. The
 *	range from first logical line to given logical line is determined
 *	using the cached values, and the range inside the given logical line
 *	is calculated on the fly.
 *
 * Results:
 *	The pixel distance between first pixel in the widget and the
 *	top of the index's current display line (could be zero).
 *
 * Side effects:
 *	Just those of 'CalculateDisplayLineHeight'.
 *
 *----------------------------------------------------------------------
 */

int
TkTextIndexYPixels(
    TkText *textPtr,		/* Widget record for text widget. */
    const TkTextIndex *indexPtr)/* The index of which we want the pixel
				 * distance from top of logical line to top of
				 * index. */
{
    int pixelHeight;
    TkTextIndex index;
    int alreadyStartOfLine = 1;

    /*
     * Find the index denoting the closest position being at the same time
     * the start of a logical line above indexPtr and the start of a display
     * line.
     */

    index = *indexPtr;
    while (1) {
        TkTextFindDisplayLineEnd(textPtr, &index, 0, NULL);
        if (index.byteIndex == 0) {
            break;
        }
        TkTextIndexBackBytes(textPtr, &index, 1, &index);
        alreadyStartOfLine = 0;
    }

    pixelHeight = TkBTreePixelsTo(textPtr, index.linePtr);

    /*
     * Shortcut to avoid layout of a superfluous display line. We know there
     * is nothing more to add up to the height if the index we were given was
     * already on the first display line of a logical line.
     */

    if (alreadyStartOfLine) {
        return pixelHeight;
    }

    /*
     * Iterate through display lines, starting at the logical line belonging
     * to index, adding up the pixel height of each such display line as we
     * go along, until we go past 'indexPtr'.
     */

    while (1) {
	int bytes, height, compare;

	/*
	 * Currently this call doesn't have many side-effects. However, if in
	 * the future we change the code so there are side-effects (such as
	 * adjusting linePtr->pixelHeight), then the code might not quite work
	 * as intended, specifically the 'linePtr->pixelHeight == pixelHeight'
	 * test below this while loop.
	 */

	height = CalculateDisplayLineHeight(textPtr, &index, &bytes, NULL);

        TkTextIndexForwBytes(textPtr, &index, bytes, &index);

        compare = TkTextIndexCmp(&index,indexPtr);
        if (compare > 0) {
	    return pixelHeight;
	}

	if (height > 0) {
	    pixelHeight += height;
	}

        if (compare == 0) {
	    return pixelHeight;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextUpdateOneLine --
 *
 *	This function is invoked to recalculate the height of a particular
 *	logical line, whether that line is displayed or not.
 *
 *	It must NEVER be called for the artificial last TkTextLine which is
 *	used internally for administrative purposes only. That line must
 *	retain its initial height of 0 otherwise the pixel height calculation
 *	maintained by the B-tree will be wrong.
 *
 * Results:
 *	The number of display lines in the logical line. This could be zero if
 *	the line is totally elided.
 *
 * Side effects:
 *	Line heights may be recalculated, and a timer to update the scrollbar
 *	may be installed. Also see the called function
 *	CalculateDisplayLineHeight for its side effects.
 *
 *----------------------------------------------------------------------
 */

int
TkTextUpdateOneLine(
    TkText *textPtr,		/* Widget record for text widget. */
    TkTextLine *linePtr,	/* The line of which to calculate the
				 * height. */
    int pixelHeight,		/* If indexPtr is non-NULL, then this is the
				 * number of pixels in the logical line
				 * linePtr, up to the index which has been
				 * given. */
    TkTextIndex *indexPtr,	/* Either NULL or an index at the start of a
				 * display line belonging to linePtr, at which
				 * we wish to start (e.g. up to which we have
				 * already calculated). On return this will be
				 * set to the first index on the next line. */
    int partialCalc)		/* Set to 1 if we are allowed to do partial
				 * height calculations of long-lines. In this
				 * case we'll only return what we know so
				 * far. */
{
    TkTextIndex index;
    int displayLines;
    int mergedLines;

    if (indexPtr == NULL) {
	index.tree = textPtr->sharedTextPtr->tree;
	index.linePtr = linePtr;
	index.byteIndex = 0;
	index.textPtr = NULL;
	indexPtr = &index;
	pixelHeight = 0;
    }

    /*
     * CalculateDisplayLineHeight _must_ be called (below) with an index at
     * the beginning of a display line. Force this to happen. This is needed
     * when TkTextUpdateOneLine is called with a line that is merged with its
     * previous line: the number of merged logical lines in a display line is
     * calculated correctly only when CalculateDisplayLineHeight receives
     * an index at the beginning of a display line. In turn this causes the
     * merged lines to receive their correct zero pixel height in
     * TkBTreeAdjustPixelHeight.
     */

    TkTextFindDisplayLineEnd(textPtr, indexPtr, 0, NULL);
    linePtr = indexPtr->linePtr;

    /*
     * Iterate through all display-lines corresponding to the single logical
     * line 'linePtr' (and lines merged into this line due to eol elision),
     * adding up the pixel height of each such display line as we go along.
     * The final total is, therefore, the total height of all display lines
     * made up by the logical line 'linePtr' and subsequent logical lines
     * merged into this line.
     */

    displayLines = 0;
    mergedLines = 0;

    while (1) {
	int bytes, height, logicalLines;

	/*
	 * Currently this call doesn't have many side-effects. However, if in
	 * the future we change the code so there are side-effects (such as
	 * adjusting linePtr->pixelHeight), then the code might not quite work
	 * as intended, specifically the 'linePtr->pixelHeight == pixelHeight'
	 * test below this while loop.
	 */

        height = CalculateDisplayLineHeight(textPtr, indexPtr, &bytes,
		&logicalLines);

	if (height > 0) {
	    pixelHeight += height;
	    displayLines++;
	}

	mergedLines += logicalLines;

	if (TkTextIndexForwBytes(textPtr, indexPtr, bytes, indexPtr)) {
	    break;
	}

        if (mergedLines == 0) {
            if (indexPtr->linePtr != linePtr) {
                /*
                 * If we reached the end of the logical line, then either way
                 * we don't have a partial calculation.
                 */

                partialCalc = 0;
                break;
            }
        } else {
            if (IsStartOfNotMergedLine(textPtr, indexPtr)) {
                /*
                 * We've ended a logical line.
                 */

                partialCalc = 0;
                break;
            }

            /*
             * We must still be on the same wrapped line, on a new logical
             * line merged with the logical line 'linePtr'.
             */
        }
	if (partialCalc && displayLines > 50 && mergedLines == 0) {
	    /*
	     * Only calculate 50 display lines at a time, to avoid huge
	     * delays. In any case it is very rare that a single line wraps 50
	     * times!
	     *
	     * If we have any merged lines, we must complete the full logical
	     * line layout here and now, because the partial-calculation code
	     * isn't designed to handle merged logical lines. Hence the
	     * 'mergedLines == 0' check.
	     */

	    break;
	}
    }

    if (!partialCalc) {
	int changed = 0;

	/*
	 * Cancel any partial line height calculation state.
	 */

	textPtr->dInfoPtr->metricEpoch = -1;

	/*
	 * Mark the logical line as being up to date (caution: it isn't yet up
	 * to date, that will happen in TkBTreeAdjustPixelHeight just below).
	 */

	TkBTreeLinePixelEpoch(textPtr, linePtr)
		= textPtr->dInfoPtr->lineMetricUpdateEpoch;
	if (TkBTreeLinePixelCount(textPtr, linePtr) != pixelHeight) {
	    changed = 1;
	}

	if (mergedLines > 0) {
	    int i = mergedLines;
	    TkTextLine *mergedLinePtr;

	    /*
	     * Loop over all merged logical lines, marking them up to date
	     * (again, the pixel count setting will actually happen in
	     * TkBTreeAdjustPixelHeight).
	     */

	    mergedLinePtr = linePtr;
	    while (i-- > 0) {
		mergedLinePtr = TkBTreeNextLine(textPtr, mergedLinePtr);
		TkBTreeLinePixelEpoch(textPtr, mergedLinePtr)
			= textPtr->dInfoPtr->lineMetricUpdateEpoch;
		if (TkBTreeLinePixelCount(textPtr, mergedLinePtr) != 0) {
		    changed = 1;
		}
	    }
	}

	if (!changed) {
	    /*
	     * If there's nothing to change, then we can already return.
	     */

	    return displayLines;
	}
    }

    /*
     * We set the line's height, but the return value is now the height of the
     * entire widget, which may be used just below for reporting/debugging
     * purposes.
     */

    pixelHeight = TkBTreeAdjustPixelHeight(textPtr, linePtr, pixelHeight,
	    mergedLines);

    if (tkTextDebug) {
	char buffer[2 * TCL_INTEGER_SPACE + 1];

	if (TkBTreeNextLine(textPtr, linePtr) == NULL) {
	    Tcl_Panic("Mustn't ever update line height of last artificial line");
	}

	sprintf(buffer, "%d %d", TkBTreeLinesTo(textPtr,linePtr), pixelHeight);
	LOG("tk_textNumPixels", buffer);
    }
    if (textPtr->dInfoPtr->scrollbarTimer == NULL) {
	textPtr->refCount++;
	textPtr->dInfoPtr->scrollbarTimer = Tcl_CreateTimerHandler(200,
		AsyncUpdateYScrollbar, textPtr);
    }
    return displayLines;
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayText --
 *
 *	This function is invoked as a when-idle handler to update the display.
 *	It only redisplays the parts of the text widget that are out of date.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is redrawn on the screen.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayText(
    ClientData clientData)	/* Information about widget. */
{
    register TkText *textPtr = clientData;
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    register DLine *dlPtr;
    DLine *prevPtr;
    Pixmap pixmap;
    int maxHeight, borders;
    int bottomY = 0;		/* Initialization needed only to stop compiler
				 * warnings. */
    Tcl_Interp *interp;

#ifdef MAC_OSX_TK
    /*
     * If drawing is disabled, all we need to do is
     * clear the REDRAW_PENDING flag.
     */
    TkWindow *winPtr = (TkWindow *)(textPtr->tkwin);
    MacDrawable *macWin = winPtr->privatePtr;
    if (macWin && (macWin->flags & TK_DO_NOT_DRAW)){
	dInfoPtr->flags &= ~REDRAW_PENDING;
    	return;
     }
#endif

    if ((textPtr->tkwin == NULL) || (textPtr->flags & DESTROYED)) {
	/*
	 * The widget has been deleted.	 Don't do anything.
	 */

	return;
    }

    interp = textPtr->interp;
    Tcl_Preserve(interp);

    if (tkTextDebug) {
	CLEAR("tk_textRelayout");
    }

    if (!Tk_IsMapped(textPtr->tkwin) || (dInfoPtr->maxX <= dInfoPtr->x)
	    || (dInfoPtr->maxY <= dInfoPtr->y)) {
	UpdateDisplayInfo(textPtr);
	dInfoPtr->flags &= ~REDRAW_PENDING;
	goto doScrollbars;
    }
    numRedisplays++;
    if (tkTextDebug) {
	CLEAR("tk_textRedraw");
    }

    /*
     * Choose a new current item if that is needed (this could cause event
     * handlers to be invoked, hence the preserve/release calls and the loop,
     * since the handlers could conceivably necessitate yet another current
     * item calculation). The tkwin check is because the whole window could go
     * away in the Tcl_Release call.
     */

    while (dInfoPtr->flags & REPICK_NEEDED) {
	textPtr->refCount++;
	dInfoPtr->flags &= ~REPICK_NEEDED;
	TkTextPickCurrent(textPtr, &textPtr->pickEvent);
	if (textPtr->refCount-- <= 1) {
	    ckfree(textPtr);
	    goto end;
	}
	if ((textPtr->tkwin == NULL) || (textPtr->flags & DESTROYED)) {
	    goto end;
	}
    }

    /*
     * First recompute what's supposed to be displayed.
     */

    UpdateDisplayInfo(textPtr);
    dInfoPtr->dLinesInvalidated = 0;

    /*
     * See if it's possible to bring some parts of the screen up-to-date by
     * scrolling (copying from other parts of the screen). We have to be
     * particularly careful with the top and bottom lines of the display,
     * since these may only be partially visible and therefore not helpful for
     * some scrolling purposes.
     */

    for (dlPtr = dInfoPtr->dLinePtr; dlPtr != NULL; dlPtr = dlPtr->nextPtr) {
	register DLine *dlPtr2;
	int offset, height, y, oldY;
	TkRegion damageRgn;

	/*
	 * These tests are, in order:
	 *
	 * 1. If the line is already marked as invalid
	 * 2. If the line hasn't moved
	 * 3. If the line overlaps the bottom of the window and we are
	 *    scrolling up.
	 * 4. If the line overlaps the top of the window and we are scrolling
	 *    down.
	 *
	 * If any of these tests are true, then we can't scroll this line's
	 * part of the display.
	 *
	 * Note that even if tests 3 or 4 aren't true, we may be able to
	 * scroll the line, but we still need to be sure to call embedded
	 * window display procs on top and bottom lines if they have any
	 * portion non-visible (see below).
	 */

	if ((dlPtr->flags & OLD_Y_INVALID)
		|| (dlPtr->y == dlPtr->oldY)
		|| (((dlPtr->oldY + dlPtr->height) > dInfoPtr->maxY)
		    && (dlPtr->y < dlPtr->oldY))
		|| ((dlPtr->oldY < dInfoPtr->y) && (dlPtr->y > dlPtr->oldY))) {
	    continue;
	}

	/*
	 * This line is already drawn somewhere in the window so it only needs
	 * to be copied to its new location. See if there's a group of lines
	 * that can all be copied together.
	 */

	offset = dlPtr->y - dlPtr->oldY;
	height = dlPtr->height;
	y = dlPtr->y;
	for (dlPtr2 = dlPtr->nextPtr; dlPtr2 != NULL;
		dlPtr2 = dlPtr2->nextPtr) {
	    if ((dlPtr2->flags & OLD_Y_INVALID)
		    || ((dlPtr2->oldY + offset) != dlPtr2->y)
		    || ((dlPtr2->oldY + dlPtr2->height) > dInfoPtr->maxY)) {
		break;
	    }
	    height += dlPtr2->height;
	}

	/*
	 * Reduce the height of the area being copied if necessary to avoid
	 * overwriting the border area.
	 */

	if ((y + height) > dInfoPtr->maxY) {
	    height = dInfoPtr->maxY - y;
	}
	oldY = dlPtr->oldY;
	if (y < dInfoPtr->y) {
	    /*
	     * Adjust if the area being copied is going to overwrite the top
	     * border of the window (so the top line is only half onscreen).
	     */

	    int y_off = dInfoPtr->y - dlPtr->y;
	    height -= y_off;
	    oldY += y_off;
	    y = dInfoPtr->y;
	}

	/*
	 * Update the lines we are going to scroll to show that they have been
	 * copied.
	 */

	while (1) {
	    /*
	     * The DLine already has OLD_Y_INVALID cleared.
	     */

	    dlPtr->oldY = dlPtr->y;
	    if (dlPtr->nextPtr == dlPtr2) {
		break;
	    }
	    dlPtr = dlPtr->nextPtr;
	}
	/*
	 * Scan through the lines following the copied ones to see if we are
	 * going to overwrite them with the copy operation. If so, mark them
	 * for redisplay.
	 */

	for ( ; dlPtr2 != NULL; dlPtr2 = dlPtr2->nextPtr) {
	    if ((!(dlPtr2->flags & OLD_Y_INVALID))
		    && ((dlPtr2->oldY + dlPtr2->height) > y)
		    && (dlPtr2->oldY < (y + height))) {
		dlPtr2->flags |= OLD_Y_INVALID;
	    }
	}

	/*
	 * Now scroll the lines. This may generate damage which we handle by
	 * calling TextInvalidateRegion to mark the display blocks as stale.
	 */

	damageRgn = TkCreateRegion();
	if (TkScrollWindow(textPtr->tkwin, dInfoPtr->scrollGC, dInfoPtr->x,
		oldY, dInfoPtr->maxX-dInfoPtr->x, height, 0, y-oldY,
		damageRgn)) {
	    TextInvalidateRegion(textPtr, damageRgn);
	}
	numCopies++;
	TkDestroyRegion(damageRgn);
    }

    /*
     * Clear the REDRAW_PENDING flag here. This is actually pretty tricky. We
     * want to wait until *after* doing the scrolling, since that could
     * generate more areas to redraw and don't want to reschedule a redisplay
     * for them. On the other hand, we can't wait until after all the
     * redisplaying, because the act of redisplaying could actually generate
     * more redisplays (e.g. in the case of a nested window with event
     * bindings triggered by redisplay).
     */

    dInfoPtr->flags &= ~REDRAW_PENDING;

    /*
     * Redraw the borders if that's needed.
     */

    if (dInfoPtr->flags & REDRAW_BORDERS) {
	if (tkTextDebug) {
	    LOG("tk_textRedraw", "borders");
	}

	if (textPtr->tkwin == NULL) {
	    /*
	     * The widget has been deleted. Don't do anything.
	     */

	    goto end;
	}

	Tk_Draw3DRectangle(textPtr->tkwin, Tk_WindowId(textPtr->tkwin),
		textPtr->border, textPtr->highlightWidth,
		textPtr->highlightWidth,
		Tk_Width(textPtr->tkwin) - 2*textPtr->highlightWidth,
		Tk_Height(textPtr->tkwin) - 2*textPtr->highlightWidth,
		textPtr->borderWidth, textPtr->relief);
	if (textPtr->highlightWidth != 0) {
	    GC fgGC, bgGC;

	    bgGC = Tk_GCForColor(textPtr->highlightBgColorPtr,
		    Tk_WindowId(textPtr->tkwin));
	    if (textPtr->flags & GOT_FOCUS) {
		fgGC = Tk_GCForColor(textPtr->highlightColorPtr,
			Tk_WindowId(textPtr->tkwin));
		TkpDrawHighlightBorder(textPtr->tkwin, fgGC, bgGC,
			textPtr->highlightWidth, Tk_WindowId(textPtr->tkwin));
	    } else {
		TkpDrawHighlightBorder(textPtr->tkwin, bgGC, bgGC,
			textPtr->highlightWidth, Tk_WindowId(textPtr->tkwin));
	    }
	}
	borders = textPtr->borderWidth + textPtr->highlightWidth;
	if (textPtr->padY > 0) {
	    Tk_Fill3DRectangle(textPtr->tkwin, Tk_WindowId(textPtr->tkwin),
		    textPtr->border, borders, borders,
		    Tk_Width(textPtr->tkwin) - 2*borders, textPtr->padY,
		    0, TK_RELIEF_FLAT);
	    Tk_Fill3DRectangle(textPtr->tkwin, Tk_WindowId(textPtr->tkwin),
		    textPtr->border, borders,
		    Tk_Height(textPtr->tkwin) - borders - textPtr->padY,
		    Tk_Width(textPtr->tkwin) - 2*borders,
		    textPtr->padY, 0, TK_RELIEF_FLAT);
	}
	if (textPtr->padX > 0) {
	    Tk_Fill3DRectangle(textPtr->tkwin, Tk_WindowId(textPtr->tkwin),
		    textPtr->border, borders, borders + textPtr->padY,
		    textPtr->padX,
		    Tk_Height(textPtr->tkwin) - 2*borders -2*textPtr->padY,
		    0, TK_RELIEF_FLAT);
	    Tk_Fill3DRectangle(textPtr->tkwin, Tk_WindowId(textPtr->tkwin),
		    textPtr->border,
		    Tk_Width(textPtr->tkwin) - borders - textPtr->padX,
		    borders + textPtr->padY, textPtr->padX,
		    Tk_Height(textPtr->tkwin) - 2*borders -2*textPtr->padY,
		    0, TK_RELIEF_FLAT);
	}
	dInfoPtr->flags &= ~REDRAW_BORDERS;
    }

    /*
     * Now we have to redraw the lines that couldn't be updated by scrolling.
     * First, compute the height of the largest line and allocate an off-
     * screen pixmap to use for double-buffered displays.
     */

    maxHeight = -1;
    for (dlPtr = dInfoPtr->dLinePtr; dlPtr != NULL;
	    dlPtr = dlPtr->nextPtr) {
	if ((dlPtr->height > maxHeight) &&
		((dlPtr->flags&OLD_Y_INVALID) || (dlPtr->oldY != dlPtr->y))) {
	    maxHeight = dlPtr->height;
	}
	bottomY = dlPtr->y + dlPtr->height;
    }

    /*
     * There used to be a line here which restricted 'maxHeight' to be no
     * larger than 'dInfoPtr->maxY', but this is incorrect for the case where
     * individual lines may be taller than the widget _and_ we have smooth
     * scrolling. What we can do is restrict maxHeight to be no larger than
     * 'dInfoPtr->maxY + dInfoPtr->topPixelOffset'.
     */

    if (maxHeight > (dInfoPtr->maxY + dInfoPtr->topPixelOffset)) {
	maxHeight = (dInfoPtr->maxY + dInfoPtr->topPixelOffset);
    }

    if (maxHeight > 0) {
#ifndef TK_NO_DOUBLE_BUFFERING
	pixmap = Tk_GetPixmap(Tk_Display(textPtr->tkwin),
		Tk_WindowId(textPtr->tkwin), Tk_Width(textPtr->tkwin),
		maxHeight, Tk_Depth(textPtr->tkwin));
#else
	pixmap = Tk_WindowId(textPtr->tkwin);
#endif /* TK_NO_DOUBLE_BUFFERING */
	for (prevPtr = NULL, dlPtr = textPtr->dInfoPtr->dLinePtr;
		(dlPtr != NULL) && (dlPtr->y < dInfoPtr->maxY);
		prevPtr = dlPtr, dlPtr = dlPtr->nextPtr) {
	    if (dlPtr->chunkPtr == NULL) {
		continue;
	    }
	    if ((dlPtr->flags & OLD_Y_INVALID) || dlPtr->oldY != dlPtr->y) {
		if (tkTextDebug) {
		    char string[TK_POS_CHARS];

		    TkTextPrintIndex(textPtr, &dlPtr->index, string);
		    LOG("tk_textRedraw", string);
		}
		DisplayDLine(textPtr, dlPtr, prevPtr, pixmap);
		if (dInfoPtr->dLinesInvalidated) {
#ifndef TK_NO_DOUBLE_BUFFERING
		    Tk_FreePixmap(Tk_Display(textPtr->tkwin), pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */
		    return;
		}
		dlPtr->oldY = dlPtr->y;
		dlPtr->flags &= ~(NEW_LAYOUT | OLD_Y_INVALID);
#ifdef MAC_OSX_TK
	    } else if (dlPtr->chunkPtr != NULL) {
		/*
		 * On macOS we need to redisplay all embedded windows which
		 * were moved by the call to TkScrollWindows above.  This is
		 * not necessary on Unix or Windows because XScrollWindow will
		 * have included the bounding rectangles of all of these
		 * windows in the damage region.  The macosx implementation of
		 * TkScrollWindow does not do this.  It simply generates a
		 * damage region which is the scroll source rectangle minus
		 * the scroll destination rectangle.  This is because there is
		 * no efficient process available for iterating through the
		 * subwindows which meet the scrolled area.  (On Unix this is
		 * handled by GraphicsExpose events generated by XCopyArea and
		 * on Windows by ScrollWindowEx.  On macOS the low level
		 * scrolling is accomplished by calling [view scrollRect:by:].
		 * This method does not provide any damage information and, in
		 * any case, could not be aware of Tk windows which were not
		 * based on NSView objects.
		 *
		 * On the other hand, this loop is already iterating through
		 * all embedded windows which could possibly have been moved
		 * by the scrolling.  So it is as efficient to redisplay them
		 * here as it would have been if they had been redisplayed by
		 * the call to TextInvalidateRegion above.
		 */
#else
	    } else if (dlPtr->chunkPtr != NULL && ((dlPtr->y < 0)
		    || (dlPtr->y + dlPtr->height > dInfoPtr->maxY))) {
		/*
		 * On platforms other than the Mac:
		 *
		 * It's the first or last DLine which are also overlapping the
		 * top or bottom of the window, but we decided above it wasn't
		 * necessary to display them (we were able to update them by
		 * scrolling). This is fine, except that if the lines contain
		 * any embedded windows, we must still call the display proc
		 * on them because they might need to be unmapped or they
		 * might need to be moved to reflect their new position.
		 * Otherwise, everything else moves, but the embedded window
		 * doesn't!
		 *
		 * So, we loop through all the chunks, calling the display
		 * proc of embedded windows only.
		 */
#endif
		register TkTextDispChunk *chunkPtr;

		for (chunkPtr = dlPtr->chunkPtr; (chunkPtr != NULL);
			chunkPtr = chunkPtr->nextPtr) {
		    int x;
		    if (chunkPtr->displayProc != TkTextEmbWinDisplayProc) {
			continue;
		    }
		    x = chunkPtr->x + dInfoPtr->x - dInfoPtr->curXPixelOffset;
		    if ((x + chunkPtr->width <= 0) || (x >= dInfoPtr->maxX)) {
			/*
			 * Note: we have to call the displayProc even for
			 * chunks that are off-screen. This is needed, for
			 * example, so that embedded windows can be unmapped
			 * in this case. Display the chunk at a coordinate
			 * that can be clearly identified by the displayProc
			 * as being off-screen to the left (the displayProc
			 * may not be able to tell if something is off to the
			 * right).
			 */

			x = -chunkPtr->width;
		    }
		    if (tkTextDebug) {
			char string[TK_POS_CHARS];

			TkTextPrintIndex(textPtr, &dlPtr->index, string);
			LOG("tk_textEmbWinDisplay", string);
		    }
		    TkTextEmbWinDisplayProc(textPtr, chunkPtr, x,
			    dlPtr->spaceAbove,
			    dlPtr->height-dlPtr->spaceAbove-dlPtr->spaceBelow,
			    dlPtr->baseline - dlPtr->spaceAbove, NULL,
			    None, dlPtr->y + dlPtr->spaceAbove);
		}
	    }
	}
#ifndef TK_NO_DOUBLE_BUFFERING
	Tk_FreePixmap(Tk_Display(textPtr->tkwin), pixmap);
#endif /* TK_NO_DOUBLE_BUFFERING */
    }

    /*
     * See if we need to refresh the part of the window below the last line of
     * text (if there is any such area). Refresh the padding area on the left
     * too, since the insertion cursor might have been displayed there
     * previously).
     */

    if (dInfoPtr->topOfEof > dInfoPtr->maxY) {
	dInfoPtr->topOfEof = dInfoPtr->maxY;
    }
    if (bottomY < dInfoPtr->topOfEof) {
	if (tkTextDebug) {
	    LOG("tk_textRedraw", "eof");
	}

	if ((textPtr->tkwin == NULL) || (textPtr->flags & DESTROYED)) {
	    /*
	     * The widget has been deleted. Don't do anything.
	     */

	    goto end;
	}

	Tk_Fill3DRectangle(textPtr->tkwin, Tk_WindowId(textPtr->tkwin),
		textPtr->border, dInfoPtr->x - textPtr->padX, bottomY,
		dInfoPtr->maxX - (dInfoPtr->x - textPtr->padX),
		dInfoPtr->topOfEof-bottomY, 0, TK_RELIEF_FLAT);
    }
    dInfoPtr->topOfEof = bottomY;

    /*
     * Update the vertical scrollbar, if there is one. Note: it's important to
     * clear REDRAW_PENDING here, just in case the scroll function does
     * something that requires redisplay.
     */

  doScrollbars:
    if (textPtr->flags & UPDATE_SCROLLBARS) {
	textPtr->flags &= ~UPDATE_SCROLLBARS;
	if (textPtr->yScrollCmd != NULL) {
	    GetYView(textPtr->interp, textPtr, 1);
	}

	if ((textPtr->tkwin == NULL) || (textPtr->flags & DESTROYED)) {
	    /*
	     * The widget has been deleted. Don't do anything.
	     */

	    goto end;
	}

	/*
	 * Update the horizontal scrollbar, if any.
	 */

	if (textPtr->xScrollCmd != NULL) {
	    GetXView(textPtr->interp, textPtr, 1);
	}
    }

  end:
    Tcl_Release(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextEventuallyRepick --
 *
 *	This function is invoked whenever something happens that could change
 *	the current character or the tags associated with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A repick is scheduled as an idle handler.
 *
 *----------------------------------------------------------------------
 */

void
TkTextEventuallyRepick(
    TkText *textPtr)		/* Widget record for text widget. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;

    dInfoPtr->flags |= REPICK_NEEDED;
    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	dInfoPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextRedrawRegion --
 *
 *	This function is invoked to schedule a redisplay for a given region of
 *	a text widget. The redisplay itself may not occur immediately: it's
 *	scheduled as a when-idle handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information will eventually be redrawn on the screen.
 *
 *----------------------------------------------------------------------
 */

void
TkTextRedrawRegion(
    TkText *textPtr,		/* Widget record for text widget. */
    int x, int y,		/* Coordinates of upper-left corner of area to
				 * be redrawn, in pixels relative to textPtr's
				 * window. */
    int width, int height)	/* Width and height of area to be redrawn. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    TkRegion damageRgn = TkCreateRegion();
    XRectangle rect;

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    TkUnionRectWithRegion(&rect, damageRgn, damageRgn);

    TextInvalidateRegion(textPtr, damageRgn);

    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	dInfoPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    TkDestroyRegion(damageRgn);
}

/*
 *----------------------------------------------------------------------
 *
 * TextInvalidateRegion --
 *
 *	Mark a region of text as invalid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the display information for the text widget.
 *
 *----------------------------------------------------------------------
 */

static void
TextInvalidateRegion(
    TkText *textPtr,		/* Widget record for text widget. */
    TkRegion region)		/* Region of area to redraw. */
{
    register DLine *dlPtr;
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    int maxY, inset;
    XRectangle rect;

    /*
     * Find all lines that overlap the given region and mark them for
     * redisplay.
     */

    TkClipBox(region, &rect);
    maxY = rect.y + rect.height;
    for (dlPtr = dInfoPtr->dLinePtr; dlPtr != NULL;
	    dlPtr = dlPtr->nextPtr) {
	if ((!(dlPtr->flags & OLD_Y_INVALID))
		&& (TkRectInRegion(region, rect.x, dlPtr->y,
		rect.width, (unsigned int) dlPtr->height) != RectangleOut)) {
	    dlPtr->flags |= OLD_Y_INVALID;
	}
    }
    if (dInfoPtr->topOfEof < maxY) {
	dInfoPtr->topOfEof = maxY;
    }

    /*
     * Schedule the redisplay operation if there isn't one already scheduled.
     */

    inset = textPtr->borderWidth + textPtr->highlightWidth;
    if ((rect.x < (inset + textPtr->padX))
	    || (rect.y < (inset + textPtr->padY))
	    || ((int) (rect.x + rect.width) > (Tk_Width(textPtr->tkwin)
		    - inset - textPtr->padX))
	    || (maxY > (Tk_Height(textPtr->tkwin) - inset - textPtr->padY))) {
	dInfoPtr->flags |= REDRAW_BORDERS;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextChanged, TextChanged --
 *
 *	This function is invoked when info in a text widget is about to be
 *	modified in a way that changes how it is displayed (e.g. characters
 *	were inserted or deleted, or tag information was changed). This
 *	function must be called *before* a change is made, so that indexes in
 *	the display information are still valid.
 *
 *	Note: if the range of indices may change geometry as well as simply
 *	requiring redisplay, then the caller should also call
 *	TkTextInvalidateLineMetrics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The range of character between index1Ptr (inclusive) and index2Ptr
 *	(exclusive) will be redisplayed at some point in the future (the
 *	actual redisplay is scheduled as a when-idle handler).
 *
 *----------------------------------------------------------------------
 */

void
TkTextChanged(
    TkSharedText *sharedTextPtr,/* Shared widget section, or NULL. */
    TkText *textPtr,		/* Widget record for text widget, or NULL. */
    const TkTextIndex*index1Ptr,/* Index of first character to redisplay. */
    const TkTextIndex*index2Ptr)/* Index of character just after last one to
				 * redisplay. */
{
    if (sharedTextPtr == NULL) {
	TextChanged(textPtr, index1Ptr, index2Ptr);
    } else {
	textPtr = sharedTextPtr->peers;
	while (textPtr != NULL) {
	    TextChanged(textPtr, index1Ptr, index2Ptr);
	    textPtr = textPtr->next;
	}
    }
}

static void
TextChanged(
    TkText *textPtr,		/* Widget record for text widget, or NULL. */
    const TkTextIndex*index1Ptr,/* Index of first character to redisplay. */
    const TkTextIndex*index2Ptr)/* Index of character just after last one to
				 * redisplay. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    DLine *firstPtr, *lastPtr;
    TkTextIndex rounded;
    TkTextLine *linePtr;
    int notBegin;

    /*
     * Schedule both a redisplay and a recomputation of display information.
     * It's done here rather than the end of the function for two reasons:
     *
     * 1. If there are no display lines to update we'll want to return
     *	  immediately, well before the end of the function.
     * 2. It's important to arrange for the redisplay BEFORE calling
     *	  FreeDLines. The reason for this is subtle and has to do with
     *	  embedded windows. The chunk delete function for an embedded window
     *	  will schedule an idle handler to unmap the window. However, we want
     *	  the idle handler for redisplay to be called first, so that it can
     *	  put the embedded window back on the screen again (if appropriate).
     *	  This will prevent the window from ever being unmapped, and thereby
     *	  avoid flashing.
     */

    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    dInfoPtr->flags |= REDRAW_PENDING|DINFO_OUT_OF_DATE|REPICK_NEEDED;

    /*
     * Find the DLines corresponding to index1Ptr and index2Ptr. There is one
     * tricky thing here, which is that we have to relayout in units of whole
     * text lines: This is necessary because the indices stored in the display
     * lines will no longer be valid. It's also needed because any edit could
     * change the way lines wrap.
     * To relayout in units of whole text (logical) lines, round index1Ptr
     * back to the beginning of its text line (or, if this line start is
     * elided, to the beginning of the text line that starts the display line
     * it is included in), and include all the display lines after index2Ptr,
     * up to the end of its text line (or, if this line end is elided, up to
     * the end of the first non elided text line after this line end).
     */

    rounded = *index1Ptr;
    rounded.byteIndex = 0;
    notBegin = 0;
    while (!IsStartOfNotMergedLine(textPtr, &rounded) && notBegin) {
        notBegin = !TkTextIndexBackBytes(textPtr, &rounded, 1, &rounded);
        rounded.byteIndex = 0;
    }

    /*
     * 'rounded' now points to the start of a display line as well as the
     * real (non elided) start of a logical line, and this index is the
     * closest before index1Ptr.
     */

    firstPtr = FindDLine(textPtr, dInfoPtr->dLinePtr, &rounded);

    if (firstPtr == NULL) {
        /*
         * index1Ptr pertains to no display line, i.e this index is after
         * the last display line. Since index2Ptr is after index1Ptr, there
         * is no display line to free/redisplay and we can return early.
         */

	return;
    }

    rounded = *index2Ptr;
    linePtr = index2Ptr->linePtr;
    do {
        linePtr = TkBTreeNextLine(textPtr, linePtr);
        if (linePtr == NULL) {
            break;
        }
        rounded.linePtr = linePtr;
        rounded.byteIndex = 0;
    } while (!IsStartOfNotMergedLine(textPtr, &rounded));

    if (linePtr == NULL) {
        lastPtr = NULL;
    } else {
        /*
         * 'rounded' now points to the start of a display line as well as the
         * start of a logical line not merged with its previous line, and
         * this index is the closest after index2Ptr.
         */

        lastPtr = FindDLine(textPtr, dInfoPtr->dLinePtr, &rounded);

        /*
         * At least one display line is supposed to change. This makes the
         * redisplay OK in case the display line we expect to get here was
         * unlinked by a previous call to TkTextChanged and the text widget
         * did not update before reaching this point. This happens for
         * instance when moving the cursor up one line.
         * Note that lastPtr != NULL here, otherwise we would have returned
         * earlier when we tested for firstPtr being NULL.
         */

        if (lastPtr == firstPtr) {
            lastPtr = lastPtr->nextPtr;
        }
    }

    /*
     * Delete all the DLines from firstPtr up to but not including lastPtr.
     */

    FreeDLines(textPtr, firstPtr, lastPtr, DLINE_UNLINK);
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextRedrawTag, TextRedrawTag --
 *
 *	This function is invoked to request a redraw of all characters in a
 *	given range that have a particular tag on or off. It's called, for
 *	example, when tag options change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information on the screen may be redrawn, and the layout of the screen
 *	may change.
 *
 *----------------------------------------------------------------------
 */

void
TkTextRedrawTag(
    TkSharedText *sharedTextPtr,/* Shared widget section, or NULL. */
    TkText *textPtr,		/* Widget record for text widget. */
    TkTextIndex *index1Ptr,	/* First character in range to consider for
				 * redisplay. NULL means start at beginning of
				 * text. */
    TkTextIndex *index2Ptr,	/* Character just after last one to consider
				 * for redisplay. NULL means process all the
				 * characters in the text. */
    TkTextTag *tagPtr,		/* Information about tag. */
    int withTag)		/* 1 means redraw characters that have the
				 * tag, 0 means redraw those without. */
{
    if (sharedTextPtr == NULL) {
	TextRedrawTag(textPtr, index1Ptr, index2Ptr, tagPtr, withTag);
    } else {
	textPtr = sharedTextPtr->peers;
	while (textPtr != NULL) {
	    TextRedrawTag(textPtr, index1Ptr, index2Ptr, tagPtr, withTag);
	    textPtr = textPtr->next;
	}
    }
}

static void
TextRedrawTag(
    TkText *textPtr,		/* Widget record for text widget. */
    TkTextIndex *index1Ptr,	/* First character in range to consider for
				 * redisplay. NULL means start at beginning of
				 * text. */
    TkTextIndex *index2Ptr,	/* Character just after last one to consider
				 * for redisplay. NULL means process all the
				 * characters in the text. */
    TkTextTag *tagPtr,		/* Information about tag. */
    int withTag)		/* 1 means redraw characters that have the
				 * tag, 0 means redraw those without. */
{
    register DLine *dlPtr;
    DLine *endPtr;
    int tagOn;
    TkTextSearch search;
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    TkTextIndex *curIndexPtr;
    TkTextIndex endOfText, *endIndexPtr;

    /*
     * Invalidate the pixel calculation of all lines in the given range. This
     * may be a bit over-aggressive, so we could consider more subtle
     * techniques here in the future. In particular, when we create a tag for
     * the first time with '.t tag configure foo -font "Arial 20"', say, even
     * though that obviously can't apply to anything at all (the tag didn't
     * exist a moment ago), we invalidate every single line in the widget.
     */

    if (tagPtr->affectsDisplayGeometry) {
	TkTextLine *startLine, *endLine;
	int lineCount;

	if (index2Ptr == NULL) {
	    endLine = NULL;
	    lineCount = TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr);
	} else {
	    endLine = index2Ptr->linePtr;
	    lineCount = TkBTreeLinesTo(textPtr, endLine);
	}
	if (index1Ptr == NULL) {
	    startLine = NULL;
	} else {
	    startLine = index1Ptr->linePtr;
	    lineCount -= TkBTreeLinesTo(textPtr, startLine);
	}
	TkTextInvalidateLineMetrics(NULL, textPtr, startLine, lineCount,
		TK_TEXT_INVALIDATE_ONLY);
    }

    /*
     * Round up the starting position if it's before the first line visible on
     * the screen (we only care about what's on the screen).
     */

    dlPtr = dInfoPtr->dLinePtr;
    if (dlPtr == NULL) {
	return;
    }
    if ((index1Ptr == NULL) || (TkTextIndexCmp(&dlPtr->index, index1Ptr)>0)) {
	index1Ptr = &dlPtr->index;
    }

    /*
     * Set the stopping position if it wasn't specified.
     */

    if (index2Ptr == NULL) {
	int lastLine = TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr);

	index2Ptr = TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		lastLine, 0, &endOfText);
    }

    /*
     * Initialize a search through all transitions on the tag, starting with
     * the first transition where the tag's current state is different from
     * what it will eventually be.
     */

    TkBTreeStartSearch(index1Ptr, index2Ptr, tagPtr, &search);

    /*
     * Make our own curIndex because at this point search.curIndex may not
     * equal index1Ptr->curIndex in the case the first tag toggle comes after
     * index1Ptr (See the use of FindTagStart in TkBTreeStartSearch).
     */

    curIndexPtr = index1Ptr;
    tagOn = TkBTreeCharTagged(index1Ptr, tagPtr);
    if (tagOn != withTag) {
	if (!TkBTreeNextTag(&search)) {
	    return;
	}
	curIndexPtr = &search.curIndex;
    }

    /*
     * Schedule a redisplay and layout recalculation if they aren't already
     * pending. This has to be done before calling FreeDLines, for the reason
     * given in TkTextChanged.
     */

    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    dInfoPtr->flags |= REDRAW_PENDING|DINFO_OUT_OF_DATE|REPICK_NEEDED;

    /*
     * Each loop through the loop below is for one range of characters where
     * the tag's current state is different than its eventual state. At the
     * top of the loop, search contains information about the first character
     * in the range.
     */

    while (1) {
	/*
	 * Find the first DLine structure in the range. Note: if the desired
	 * character isn't the first in its text line, then look for the
	 * character just before it instead. This is needed to handle the case
	 * where the first character of a wrapped display line just got
	 * smaller, so that it now fits on the line before: need to relayout
	 * the line containing the previous character.
	 */

	if (IsStartOfNotMergedLine(textPtr, curIndexPtr)) {
	    dlPtr = FindDLine(textPtr, dlPtr, curIndexPtr);
	} else {
	    TkTextIndex tmp = *curIndexPtr;

            TkTextIndexBackBytes(textPtr, &tmp, 1, &tmp);
	    dlPtr = FindDLine(textPtr, dlPtr, &tmp);
	}
	if (dlPtr == NULL) {
	    break;
	}

	/*
	 * Find the first DLine structure that's past the end of the range.
	 */

	if (!TkBTreeNextTag(&search)) {
	    endIndexPtr = index2Ptr;
	} else {
	    curIndexPtr = &search.curIndex;
	    endIndexPtr = curIndexPtr;
	}
	endPtr = FindDLine(textPtr, dlPtr, endIndexPtr);
	if ((endPtr != NULL)
                && (TkTextIndexCmp(&endPtr->index,endIndexPtr) < 0)) {
	    endPtr = endPtr->nextPtr;
	}

	/*
	 * Delete all of the display lines in the range, so that they'll be
	 * re-layed out and redrawn.
	 */

	FreeDLines(textPtr, dlPtr, endPtr, DLINE_UNLINK);
	dlPtr = endPtr;

	/*
	 * Find the first text line in the next range.
	 */

	if (!TkBTreeNextTag(&search)) {
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextRelayoutWindow --
 *
 *	This function is called when something has happened that invalidates
 *	the whole layout of characters on the screen, such as a change in a
 *	configuration option for the overall text widget or a change in the
 *	window size. It causes all display information to be recomputed and
 *	the window to be redrawn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All the display information will be recomputed for the window and the
 *	window will be redrawn.
 *
 *----------------------------------------------------------------------
 */

void
TkTextRelayoutWindow(
    TkText *textPtr,		/* Widget record for text widget. */
    int mask)			/* OR'd collection of bits showing what has
				 * changed. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    GC newGC;
    XGCValues gcValues;
    Bool inSync = 1;

    /*
     * Schedule the window redisplay. See TkTextChanged for the reason why
     * this has to be done before any calls to FreeDLines.
     */

    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayText, textPtr);
	inSync = 0;
    }
    dInfoPtr->flags |= REDRAW_PENDING|REDRAW_BORDERS|DINFO_OUT_OF_DATE
	    |REPICK_NEEDED;

    /*
     * (Re-)create the graphics context for drawing the traversal highlight.
     */

    gcValues.graphics_exposures = False;
    newGC = Tk_GetGC(textPtr->tkwin, GCGraphicsExposures, &gcValues);
    if (dInfoPtr->copyGC != NULL) {
	Tk_FreeGC(textPtr->display, dInfoPtr->copyGC);
    }
    dInfoPtr->copyGC = newGC;

    /*
     * Throw away all the current layout information.
     */

    FreeDLines(textPtr, dInfoPtr->dLinePtr, NULL, DLINE_UNLINK);
    dInfoPtr->dLinePtr = NULL;

    /*
     * Recompute some overall things for the layout. Even if the window gets
     * very small, pretend that there's at least one pixel of drawing space in
     * it.
     */

    if (textPtr->highlightWidth < 0) {
	textPtr->highlightWidth = 0;
    }
    dInfoPtr->x = textPtr->highlightWidth + textPtr->borderWidth
	    + textPtr->padX;
    dInfoPtr->y = textPtr->highlightWidth + textPtr->borderWidth
	    + textPtr->padY;
    dInfoPtr->maxX = Tk_Width(textPtr->tkwin) - textPtr->highlightWidth
	    - textPtr->borderWidth - textPtr->padX;
    if (dInfoPtr->maxX <= dInfoPtr->x) {
	dInfoPtr->maxX = dInfoPtr->x + 1;
    }

    /*
     * This is the only place where dInfoPtr->maxY is set.
     */

    dInfoPtr->maxY = Tk_Height(textPtr->tkwin) - textPtr->highlightWidth
	    - textPtr->borderWidth - textPtr->padY;
    if (dInfoPtr->maxY <= dInfoPtr->y) {
	dInfoPtr->maxY = dInfoPtr->y + 1;
    }
    dInfoPtr->topOfEof = dInfoPtr->maxY;

    /*
     * If the upper-left character isn't the first in a line, recompute it.
     * This is necessary because a change in the window's size or options
     * could change the way lines wrap.
     */

    if (!IsStartOfNotMergedLine(textPtr, &textPtr->topIndex)) {
	TkTextFindDisplayLineEnd(textPtr, &textPtr->topIndex, 0, NULL);
    }

    /*
     * Invalidate cached scrollbar positions, so that scrollbars sliders will
     * be udpated.
     */

    dInfoPtr->xScrollFirst = dInfoPtr->xScrollLast = -1;
    dInfoPtr->yScrollFirst = dInfoPtr->yScrollLast = -1;

    if (mask & TK_TEXT_LINE_GEOMETRY) {

	/*
	 * Set up line metric recalculation.
	 *
	 * Avoid the special zero value, since that is used to mark individual
	 * lines as being out of date.
	 */

	if ((++dInfoPtr->lineMetricUpdateEpoch) == 0) {
	    dInfoPtr->lineMetricUpdateEpoch++;
	}

	dInfoPtr->currentMetricUpdateLine = -1;

	/*
	 * Also cancel any partial line-height calculations (for long-wrapped
	 * lines) in progress.
	 */

	dInfoPtr->metricEpoch = -1;

	if (dInfoPtr->lineUpdateTimer == NULL) {
	    textPtr->refCount++;
	    dInfoPtr->lineUpdateTimer = Tcl_CreateTimerHandler(1,
		    AsyncUpdateLineMetrics, textPtr);
	    inSync = 0;
	}

        GenerateWidgetViewSyncEvent(textPtr, inSync);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextSetYView --
 *
 *	This function is called to specify what lines are to be displayed in a
 *	text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The display will (eventually) be updated so that the position given by
 *	"indexPtr" is visible on the screen at the position determined by
 *	"pickPlace".
 *
 *----------------------------------------------------------------------
 */

void
TkTextSetYView(
    TkText *textPtr,		/* Widget record for text widget. */
    TkTextIndex *indexPtr,	/* Position that is to appear somewhere in the
				 * view. */
    int pickPlace)		/* 0 means the given index must appear exactly
				 * at the top of the screen. TK_TEXT_PICKPLACE
				 * (-1) means we get to pick where it appears:
				 * minimize screen motion or else display line
				 * at center of screen. TK_TEXT_NOPIXELADJUST
				 * (-2) indicates to make the given index the
				 * top line, but if it is already the top
				 * line, don't nudge it up or down by a few
				 * pixels just to make sure it is entirely
				 * displayed. Positive numbers indicate the
				 * number of pixels of the index's line which
				 * are to be off the top of the screen. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    register DLine *dlPtr;
    int bottomY, close, lineIndex;
    TkTextIndex tmpIndex, rounded;
    int lineHeight;

    /*
     * If the specified position is the extra line at the end of the text,
     * round it back to the last real line.
     */

    lineIndex = TkBTreeLinesTo(textPtr, indexPtr->linePtr);
    if (lineIndex == TkBTreeNumLines(indexPtr->tree, textPtr)) {
	TkTextIndexBackChars(textPtr, indexPtr, 1, &rounded, COUNT_INDICES);
	indexPtr = &rounded;
    }

    if (pickPlace == TK_TEXT_NOPIXELADJUST) {
	if (textPtr->topIndex.linePtr == indexPtr->linePtr
		&& textPtr->topIndex.byteIndex == indexPtr->byteIndex) {
	    pickPlace = dInfoPtr->topPixelOffset;
	} else {
	    pickPlace = 0;
	}
    }

    if (pickPlace != TK_TEXT_PICKPLACE) {
	/*
	 * The specified position must go at the top of the screen. Just leave
	 * all the DLine's alone: we may be able to reuse some of the
	 * information that's currently on the screen without redisplaying it
	 * all.
	 */

	textPtr->topIndex = *indexPtr;
        if (!IsStartOfNotMergedLine(textPtr, indexPtr)) {
            TkTextFindDisplayLineEnd(textPtr, &textPtr->topIndex, 0, NULL);
        }
	dInfoPtr->newTopPixelOffset = pickPlace;
	goto scheduleUpdate;
    }

    /*
     * We have to pick where to display the index. First, bring the display
     * information up to date and see if the index will be completely visible
     * in the current screen configuration. If so then there's nothing to do.
     */

    if (dInfoPtr->flags & DINFO_OUT_OF_DATE) {
	UpdateDisplayInfo(textPtr);
    }
    dlPtr = FindDLine(textPtr, dInfoPtr->dLinePtr, indexPtr);
    if (dlPtr != NULL) {
	if ((dlPtr->y + dlPtr->height) > dInfoPtr->maxY) {
	    /*
	     * Part of the line hangs off the bottom of the screen; pretend
	     * the whole line is off-screen.
	     */

	    dlPtr = NULL;
        } else {
            if (TkTextIndexCmp(&dlPtr->index, indexPtr) <= 0) {
                if (dInfoPtr->dLinePtr == dlPtr && dInfoPtr->topPixelOffset != 0) {
                    /*
                     * It is on the top line, but that line is hanging off the top
                     * of the screen. Change the top overlap to zero and update.
                     */

                    dInfoPtr->newTopPixelOffset = 0;
                    goto scheduleUpdate;
                }
                /*
                 * The line is already on screen, with no need to scroll.
                 */
                return;
            }
        }
    }

    /*
     * The desired line isn't already on-screen. Figure out what it means to
     * be "close" to the top or bottom of the screen. Close means within 1/3
     * of the screen height or within three lines, whichever is greater.
     *
     * If the line is not close, place it in the center of the window.
     */

    tmpIndex = *indexPtr;
    TkTextFindDisplayLineEnd(textPtr, &tmpIndex, 0, NULL);
    lineHeight = CalculateDisplayLineHeight(textPtr, &tmpIndex, NULL, NULL);

    /*
     * It would be better if 'bottomY' were calculated using the actual height
     * of the given line, not 'textPtr->charHeight'.
     */

    bottomY = (dInfoPtr->y + dInfoPtr->maxY + lineHeight)/2;
    close = (dInfoPtr->maxY - dInfoPtr->y)/3;
    if (close < 3*textPtr->charHeight) {
	close = 3*textPtr->charHeight;
    }
    if (dlPtr != NULL) {
	int overlap;

	/*
	 * The desired line is above the top of screen. If it is "close" to
	 * the top of the window then make it the top line on the screen.
	 * MeasureUp counts from the bottom of the given index upwards, so we
	 * add an extra half line to be sure we count far enough.
	 */

	MeasureUp(textPtr, &textPtr->topIndex, close + textPtr->charHeight/2,
		&tmpIndex, &overlap);
	if (TkTextIndexCmp(&tmpIndex, indexPtr) <= 0) {
	    textPtr->topIndex = *indexPtr;
	    TkTextFindDisplayLineEnd(textPtr, &textPtr->topIndex, 0, NULL);
	    dInfoPtr->newTopPixelOffset = 0;
	    goto scheduleUpdate;
	}
    } else {
	int overlap;

	/*
	 * The desired line is below the bottom of the screen. If it is
	 * "close" to the bottom of the screen then position it at the bottom
	 * of the screen.
	 */

	MeasureUp(textPtr, indexPtr, close + lineHeight
		- textPtr->charHeight/2, &tmpIndex, &overlap);
	if (FindDLine(textPtr, dInfoPtr->dLinePtr, &tmpIndex) != NULL) {
	    bottomY = dInfoPtr->maxY - dInfoPtr->y;
	}
    }

    /*
     * If the window height is smaller than the line height, prefer to make
     * the top of the line visible.
     */

    if (dInfoPtr->maxY - dInfoPtr->y < lineHeight) {
        bottomY = lineHeight;
    }

    /*
     * Our job now is to arrange the display so that indexPtr appears as low
     * on the screen as possible but with its bottom no lower than bottomY.
     * BottomY is the bottom of the window if the desired line is just below
     * the current screen, otherwise it is a half-line lower than the center
     * of the window.
     */

    MeasureUp(textPtr, indexPtr, bottomY, &textPtr->topIndex,
	    &dInfoPtr->newTopPixelOffset);

  scheduleUpdate:
    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    dInfoPtr->flags |= REDRAW_PENDING|DINFO_OUT_OF_DATE|REPICK_NEEDED;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextMeasureDown --
 *
 *	Given one index, find the index of the first character on the highest
 *	display line that would be displayed no more than "distance" pixels
 *	below the top of the given index.
 *
 * Results:
 *	The srcPtr is manipulated in place to reflect the new position. We
 *	return the number of pixels by which 'distance' overlaps the srcPtr.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkTextMeasureDown(
    TkText *textPtr,		/* Text widget in which to measure. */
    TkTextIndex *srcPtr,	/* Index of character from which to start
				 * measuring. */
    int distance)		/* Vertical distance in pixels measured from
				 * the top pixel in srcPtr's logical line. */
{
    TkTextLine *lastLinePtr;
    DLine *dlPtr;
    TkTextIndex loop;

    lastLinePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr,
	    TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr));

    do {
	dlPtr = LayoutDLine(textPtr, srcPtr);
	dlPtr->nextPtr = NULL;

	if (distance < dlPtr->height) {
	    FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);
	    break;
	}
	distance -= dlPtr->height;
	TkTextIndexForwBytes(textPtr, srcPtr, dlPtr->byteCount, &loop);
	FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);
	if (loop.linePtr == lastLinePtr) {
	    break;
	}
	*srcPtr = loop;
    } while (distance > 0);

    return distance;
}

/*
 *--------------------------------------------------------------
 *
 * MeasureUp --
 *
 *	Given one index, find the index of the first character on the highest
 *	display line that would be displayed no more than "distance" pixels
 *	above the given index.
 *
 *	If this function is called with distance=0, it simply finds the first
 *	index on the same display line as srcPtr. However, there is a another
 *	function TkTextFindDisplayLineEnd designed just for that task which is
 *	probably better to use.
 *
 * Results:
 *	*dstPtr is filled in with the index of the first character on a
 *	display line. The display line is found by measuring up "distance"
 *	pixels above the pixel just below an imaginary display line that
 *	contains srcPtr. If the display line that covers this coordinate
 *	actually extends above the coordinate, then return any excess pixels
 *	in *overlap, if that is non-NULL.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
MeasureUp(
    TkText *textPtr,		/* Text widget in which to measure. */
    const TkTextIndex *srcPtr,	/* Index of character from which to start
				 * measuring. */
    int distance,		/* Vertical distance in pixels measured from
				 * the pixel just below the lowest one in
				 * srcPtr's line. */
    TkTextIndex *dstPtr,	/* Index to fill in with result. */
    int *overlap)		/* Used to store how much of the final index
				 * returned was not covered by 'distance'. */
{
    int lineNum;		/* Number of current line. */
    int bytesToCount;		/* Maximum number of bytes to measure in
				 * current line. */
    TkTextIndex index;
    DLine *dlPtr, *lowestPtr;

    bytesToCount = srcPtr->byteIndex + 1;
    index.tree = srcPtr->tree;
    for (lineNum = TkBTreeLinesTo(textPtr, srcPtr->linePtr); lineNum >= 0;
	    lineNum--) {
	/*
	 * Layout an entire text line (potentially > 1 display line).
	 *
	 * For the first line, which contains srcPtr, only layout the part up
	 * through srcPtr (bytesToCount is non-infinite to accomplish this).
	 * Make a list of all the display lines in backwards order (the lowest
	 * DLine on the screen is first in the list).
	 */

	index.linePtr = TkBTreeFindLine(srcPtr->tree, textPtr, lineNum);
	index.byteIndex = 0;
        TkTextFindDisplayLineEnd(textPtr, &index, 0, NULL);
        lineNum = TkBTreeLinesTo(textPtr, index.linePtr);
	lowestPtr = NULL;
	do {
	    dlPtr = LayoutDLine(textPtr, &index);
	    dlPtr->nextPtr = lowestPtr;
	    lowestPtr = dlPtr;
	    TkTextIndexForwBytes(textPtr, &index, dlPtr->byteCount, &index);
	    bytesToCount -= dlPtr->byteCount;
	} while (bytesToCount>0 && index.linePtr==dlPtr->index.linePtr);

	/*
	 * Scan through the display lines to see if we've covered enough
	 * vertical distance. If so, save the starting index for the line at
	 * the desired location. If distance was zero to start with then we
	 * simply get the first index on the same display line as the original
	 * index.
	 */

	for (dlPtr = lowestPtr; dlPtr != NULL; dlPtr = dlPtr->nextPtr) {
	    distance -= dlPtr->height;
	    if (distance <= 0) {
                *dstPtr = dlPtr->index;

                /*
                 * dstPtr is the start of a display line that is or is not
                 * the start of a logical line. If it is the start of a
                 * logical line, we must check whether this line is merged
                 * with the previous logical line, and if so we must adjust
                 * dstPtr to the start of the display line since a display
                 * line start needs to be returned.
                 */
                if (!IsStartOfNotMergedLine(textPtr, dstPtr)) {
                    TkTextFindDisplayLineEnd(textPtr, dstPtr, 0, NULL);
                }

                if (overlap != NULL) {
		    *overlap = -distance;
		}
		break;
	    }
	}

	/*
	 * Discard the display lines, then either return or prepare for the
	 * next display line to lay out.
	 */

	FreeDLines(textPtr, lowestPtr, NULL, DLINE_FREE);
	if (distance <= 0) {
	    return;
	}
	bytesToCount = INT_MAX;		/* Consider all chars. in next line. */
    }

    /*
     * Ran off the beginning of the text. Return the first character in the
     * text.
     */

    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0, dstPtr);
    if (overlap != NULL) {
	*overlap = 0;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkTextSeeCmd --
 *
 *	This function is invoked to process the "see" option for the widget
 *	command for text widgets. See the user documentation for details on
 *	what it does.
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
TkTextSeeCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "see". */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    TkTextIndex index;
    int x, y, width, height, lineWidth, byteCount, oneThird, delta;
    DLine *dlPtr;
    TkTextDispChunk *chunkPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "index");
	return TCL_ERROR;
    }
    if (TkTextGetObjIndex(interp, textPtr, objv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * If the specified position is the extra line at the end of the text,
     * round it back to the last real line.
     */

    if (TkBTreeLinesTo(textPtr, index.linePtr)
	    == TkBTreeNumLines(index.tree, textPtr)) {
	TkTextIndexBackChars(textPtr, &index, 1, &index, COUNT_INDICES);
    }

    /*
     * First get the desired position into the vertical range of the window.
     */

    TkTextSetYView(textPtr, &index, TK_TEXT_PICKPLACE);

    /*
     * Now make sure that the character is in view horizontally.
     */

    if (dInfoPtr->flags & DINFO_OUT_OF_DATE) {
	UpdateDisplayInfo(textPtr);
    }
    lineWidth = dInfoPtr->maxX - dInfoPtr->x;
    if (dInfoPtr->maxLength < lineWidth) {
	return TCL_OK;
    }

    /*
     * Find the display line containing the desired index. dlPtr may be NULL
     * if the widget is not mapped. [Bug #641778]
     */

    dlPtr = FindDLine(textPtr, dInfoPtr->dLinePtr, &index);
    if (dlPtr == NULL) {
	return TCL_OK;
    }

    /*
     * Find the chunk within the display line that contains the desired
     * index. The chunks making the display line are skipped up to but not
     * including the one crossing index. Skipping is done based on a
     * byteCount offset possibly spanning several logical lines in case
     * they are elided.
     */

    byteCount = TkTextIndexCountBytes(textPtr, &dlPtr->index, &index);
    for (chunkPtr = dlPtr->chunkPtr; chunkPtr != NULL ;
	    chunkPtr = chunkPtr->nextPtr) {
	if (byteCount < chunkPtr->numBytes) {
	    break;
	}
	byteCount -= chunkPtr->numBytes;
    }

    /*
     * Call a chunk-specific function to find the horizontal range of the
     * character within the chunk. chunkPtr is NULL if trying to see in elided
     * region.
     */

    if (chunkPtr != NULL) {
        chunkPtr->bboxProc(textPtr, chunkPtr, byteCount,
                dlPtr->y + dlPtr->spaceAbove,
                dlPtr->height - dlPtr->spaceAbove - dlPtr->spaceBelow,
                dlPtr->baseline - dlPtr->spaceAbove, &x, &y, &width,
                &height);
        delta = x - dInfoPtr->curXPixelOffset;
        oneThird = lineWidth/3;
        if (delta < 0) {
            if (delta < -oneThird) {
                dInfoPtr->newXPixelOffset = x - lineWidth/2;
            } else {
                dInfoPtr->newXPixelOffset += delta;
            }
        } else {
            delta -= lineWidth - width;
            if (delta <= 0) {
                return TCL_OK;
            }
            if (delta > oneThird) {
                dInfoPtr->newXPixelOffset = x - lineWidth/2;
            } else {
                dInfoPtr->newXPixelOffset += delta;
            }
        }
    }
    dInfoPtr->flags |= DINFO_OUT_OF_DATE;
    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	dInfoPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextXviewCmd --
 *
 *	This function is invoked to process the "xview" option for the widget
 *	command for text widgets. See the user documentation for details on
 *	what it does.
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
TkTextXviewCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "xview". */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    int type, count;
    double fraction;

    if (dInfoPtr->flags & DINFO_OUT_OF_DATE) {
	UpdateDisplayInfo(textPtr);
    }

    if (objc == 2) {
	GetXView(interp, textPtr, 0);
	return TCL_OK;
    }

    type = TextGetScrollInfoObj(interp, textPtr, objc, objv,
	    &fraction, &count);
    switch (type) {
    case TKTEXT_SCROLL_ERROR:
	return TCL_ERROR;
    case TKTEXT_SCROLL_MOVETO:
	if (fraction > 1.0) {
	    fraction = 1.0;
	}
	if (fraction < 0) {
	    fraction = 0;
	}
	dInfoPtr->newXPixelOffset = (int)
		(fraction * dInfoPtr->maxLength + 0.5);
	break;
    case TKTEXT_SCROLL_PAGES: {
	int pixelsPerPage;

	pixelsPerPage = (dInfoPtr->maxX-dInfoPtr->x) - 2*textPtr->charWidth;
	if (pixelsPerPage < 1) {
	    pixelsPerPage = 1;
	}
	dInfoPtr->newXPixelOffset += pixelsPerPage * count;
	break;
    }
    case TKTEXT_SCROLL_UNITS:
	dInfoPtr->newXPixelOffset += count * textPtr->charWidth;
	break;
    case TKTEXT_SCROLL_PIXELS:
	dInfoPtr->newXPixelOffset += count;
	break;
    }

    dInfoPtr->flags |= DINFO_OUT_OF_DATE;
    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	dInfoPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * YScrollByPixels --
 *
 *	This function is called to scroll a text widget up or down by a given
 *	number of pixels.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The view in textPtr's window changes to reflect the value of "offset".
 *
 *----------------------------------------------------------------------
 */

static void
YScrollByPixels(
    TkText *textPtr,		/* Widget to scroll. */
    int offset)			/* Amount by which to scroll, in pixels.
				 * Positive means that information later in
				 * text becomes visible, negative means that
				 * information earlier in the text becomes
				 * visible. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;

    if (offset < 0) {
	/*
	 * Now we want to measure up this number of pixels from the top of the
	 * screen. But the top line may not be totally visible. Note that
	 * 'count' is negative here.
	 */

	offset -= CalculateDisplayLineHeight(textPtr,
		&textPtr->topIndex, NULL, NULL) - dInfoPtr->topPixelOffset;
	MeasureUp(textPtr, &textPtr->topIndex, -offset,
		&textPtr->topIndex, &dInfoPtr->newTopPixelOffset);
    } else if (offset > 0) {
	DLine *dlPtr;
	TkTextLine *lastLinePtr;
	TkTextIndex newIdx;

	/*
	 * Scrolling down by pixels. Layout lines starting at the top index
	 * and count through the desired vertical distance.
	 */

	lastLinePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr,
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr));
	offset += dInfoPtr->topPixelOffset;
	dInfoPtr->newTopPixelOffset = 0;
	while (offset > 0) {
	    dlPtr = LayoutDLine(textPtr, &textPtr->topIndex);
	    dlPtr->nextPtr = NULL;
	    TkTextIndexForwBytes(textPtr, &textPtr->topIndex,
		    dlPtr->byteCount, &newIdx);
	    if (offset <= dlPtr->height) {
		/*
		 * Adjust the top overlap accordingly.
		 */

		dInfoPtr->newTopPixelOffset = offset;
	    }
	    offset -= dlPtr->height;
	    FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);
	    if (newIdx.linePtr == lastLinePtr || offset <= 0) {
		break;
	    }
	    textPtr->topIndex = newIdx;
	}
    } else {
	/*
	 * offset = 0, so no scrolling required.
	 */

	return;
    }
    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    dInfoPtr->flags |= REDRAW_PENDING|DINFO_OUT_OF_DATE|REPICK_NEEDED;
}

/*
 *----------------------------------------------------------------------
 *
 * YScrollByLines --
 *
 *	This function is called to scroll a text widget up or down by a given
 *	number of lines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The view in textPtr's window changes to reflect the value of "offset".
 *
 *----------------------------------------------------------------------
 */

static void
YScrollByLines(
    TkText *textPtr,		/* Widget to scroll. */
    int offset)			/* Amount by which to scroll, in display
				 * lines. Positive means that information
				 * later in text becomes visible, negative
				 * means that information earlier in the text
				 * becomes visible. */
{
    int i, bytesToCount, lineNum;
    TkTextIndex newIdx, index;
    TkTextLine *lastLinePtr;
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    DLine *dlPtr, *lowestPtr;

    if (offset < 0) {
	/*
	 * Must scroll up (to show earlier information in the text). The code
	 * below is similar to that in MeasureUp, except that it counts lines
	 * instead of pixels.
	 */

	bytesToCount = textPtr->topIndex.byteIndex + 1;
	index.tree = textPtr->sharedTextPtr->tree;
	offset--;		/* Skip line containing topIndex. */
	for (lineNum = TkBTreeLinesTo(textPtr, textPtr->topIndex.linePtr);
		lineNum >= 0; lineNum--) {
	    index.linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree,
		    textPtr, lineNum);
	    index.byteIndex = 0;
	    lowestPtr = NULL;
	    do {
		dlPtr = LayoutDLine(textPtr, &index);
		dlPtr->nextPtr = lowestPtr;
		lowestPtr = dlPtr;
		TkTextIndexForwBytes(textPtr, &index, dlPtr->byteCount,&index);
		bytesToCount -= dlPtr->byteCount;
	    } while ((bytesToCount > 0)
		    && (index.linePtr == dlPtr->index.linePtr));

	    for (dlPtr = lowestPtr; dlPtr != NULL; dlPtr = dlPtr->nextPtr) {
		offset++;
		if (offset == 0) {
		    textPtr->topIndex = dlPtr->index;

                    /*
                     * topIndex is the start of a logical line. However, if
                     * the eol of the previous logical line is elided, then
                     * topIndex may be elsewhere than the first character of
                     * a display line, which is unwanted. Adjust to the start
                     * of the display line, if needed.
                     * topIndex is the start of a display line that is or is
                     * not the start of a logical line. If it is the start of
                     * a logical line, we must check whether this line is
                     * merged with the previous logical line, and if so we
                     * must adjust topIndex to the start of the display line.
                     */
                    if (!IsStartOfNotMergedLine(textPtr, &textPtr->topIndex)) {
                        TkTextFindDisplayLineEnd(textPtr, &textPtr->topIndex,
                                0, NULL);
                    }

                    break;
		}
	    }

	    /*
	     * Discard the display lines, then either return or prepare for
	     * the next display line to lay out.
	     */

	    FreeDLines(textPtr, lowestPtr, NULL, DLINE_FREE);
	    if (offset >= 0) {
		goto scheduleUpdate;
	    }
	    bytesToCount = INT_MAX;
	}

	/*
	 * Ran off the beginning of the text. Return the first character in
	 * the text, and make sure we haven't left anything overlapping the
	 * top window border.
	 */

	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0,
		&textPtr->topIndex);
	dInfoPtr->newTopPixelOffset = 0;
    } else {
	/*
	 * Scrolling down, to show later information in the text. Just count
	 * lines from the current top of the window.
	 */

	lastLinePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, textPtr,
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr));
	for (i = 0; i < offset; i++) {
	    dlPtr = LayoutDLine(textPtr, &textPtr->topIndex);
	    if (dlPtr->length == 0 && dlPtr->height == 0) {
		offset++;
	    }
	    dlPtr->nextPtr = NULL;
	    TkTextIndexForwBytes(textPtr, &textPtr->topIndex,
		    dlPtr->byteCount, &newIdx);
	    FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE);
	    if (newIdx.linePtr == lastLinePtr) {
		break;
	    }
	    textPtr->topIndex = newIdx;
	}
    }

  scheduleUpdate:
    if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayText, textPtr);
    }
    dInfoPtr->flags |= REDRAW_PENDING|DINFO_OUT_OF_DATE|REPICK_NEEDED;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextYviewCmd --
 *
 *	This function is invoked to process the "yview" option for the widget
 *	command for text widgets. See the user documentation for details on
 *	what it does.
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
TkTextYviewCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "yview". */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    int pickPlace, type;
    int pixels, count;
    int switchLength;
    double fraction;
    TkTextIndex index;

    if (dInfoPtr->flags & DINFO_OUT_OF_DATE) {
	UpdateDisplayInfo(textPtr);
    }

    if (objc == 2) {
	GetYView(interp, textPtr, 0);
	return TCL_OK;
    }

    /*
     * Next, handle the old syntax: "pathName yview ?-pickplace? where"
     */

    pickPlace = 0;
    if (Tcl_GetString(objv[2])[0] == '-') {
	register const char *switchStr =
		Tcl_GetStringFromObj(objv[2], &switchLength);

	if ((switchLength >= 2) && (strncmp(switchStr, "-pickplace",
		(unsigned) switchLength) == 0)) {
	    pickPlace = 1;
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "lineNum|index");
		return TCL_ERROR;
	    }
	}
    }
    if ((objc == 3) || pickPlace) {
	int lineNum;

	if (Tcl_GetIntFromObj(interp, objv[2+pickPlace], &lineNum) == TCL_OK) {
	    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		    lineNum, 0, &index);
	    TkTextSetYView(textPtr, &index, 0);
	    return TCL_OK;
	}

	/*
	 * The argument must be a regular text index.
	 */

	Tcl_ResetResult(interp);
	if (TkTextGetObjIndex(interp, textPtr, objv[2+pickPlace],
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	TkTextSetYView(textPtr, &index, (pickPlace ? TK_TEXT_PICKPLACE : 0));
	return TCL_OK;
    }

    /*
     * New syntax: dispatch based on objv[2].
     */

    type = TextGetScrollInfoObj(interp, textPtr, objc,objv, &fraction, &count);
    switch (type) {
    case TKTEXT_SCROLL_ERROR:
	return TCL_ERROR;
    case TKTEXT_SCROLL_MOVETO: {
	int numPixels = TkBTreeNumPixels(textPtr->sharedTextPtr->tree,
		textPtr);
	int topMostPixel;

	if (numPixels == 0) {
	    /*
	     * If the window is totally empty no scrolling is needed, and the
	     * TkTextMakePixelIndex call below will fail.
	     */

	    break;
	}
	if (fraction > 1.0) {
	    fraction = 1.0;
	}
	if (fraction < 0) {
	    fraction = 0;
	}

	/*
	 * Calculate the pixel count for the new topmost pixel in the topmost
	 * line of the window. Note that the interpretation of 'fraction' is
	 * that it counts from 0 (top pixel in buffer) to 1.0 (one pixel past
	 * the last pixel in buffer).
	 */

	topMostPixel = (int) (0.5 + fraction * numPixels);
	if (topMostPixel >= numPixels) {
	    topMostPixel = numPixels -1;
	}

	/*
	 * This function returns the number of pixels by which the given line
	 * should overlap the top of the visible screen.
	 *
	 * This is then used to provide smooth scrolling.
	 */

	pixels = TkTextMakePixelIndex(textPtr, topMostPixel, &index);
	TkTextSetYView(textPtr, &index, pixels);
	break;
    }
    case TKTEXT_SCROLL_PAGES: {
	/*
	 * Scroll up or down by screenfuls. Actually, use the window height
	 * minus two lines, so that there's some overlap between adjacent
	 * pages.
	 */

	int height = dInfoPtr->maxY - dInfoPtr->y;

	if (textPtr->charHeight * 4 >= height) {
	    /*
	     * A single line is more than a quarter of the display. We choose
	     * to scroll by 3/4 of the height instead.
	     */

	    pixels = 3*height/4;
	    if (pixels < textPtr->charHeight) {
		/*
		 * But, if 3/4 of the height is actually less than a single
		 * typical character height, then scroll by the minimum of the
		 * linespace or the total height.
		 */

		if (textPtr->charHeight < height) {
		    pixels = textPtr->charHeight;
		} else {
		    pixels = height;
		}
	    }
	    pixels *= count;
	} else {
	    pixels = (height - 2*textPtr->charHeight)*count;
	}
	YScrollByPixels(textPtr, pixels);
	break;
    }
    case TKTEXT_SCROLL_PIXELS:
	YScrollByPixels(textPtr, count);
	break;
    case TKTEXT_SCROLL_UNITS:
	YScrollByLines(textPtr, count);
	break;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextPendingsync --
 *
 *	This function checks if any line heights are not up-to-date.
 *
 * Results:
 *	Returns a boolean true if it is the case, or false if all line
 *      heights are up-to-date.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

Bool
TkTextPendingsync(
    TkText *textPtr)		/* Information about text widget. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;

    return ((dInfoPtr->flags & OUT_OF_SYNC) != 0);
}

/*
 *--------------------------------------------------------------
 *
 * TkTextScanCmd --
 *
 *	This function is invoked to process the "scan" option for the widget
 *	command for text widgets. See the user documentation for details on
 *	what it does.
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
TkTextScanCmd(
    register TkText *textPtr,	/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "scan". */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    TkTextIndex index;
    int c, x, y, totalScroll, gain=10;
    size_t length;

    if ((objc != 5) && (objc != 6)) {
	Tcl_WrongNumArgs(interp, 2, objv, "mark x y");
	Tcl_AppendResult(interp, " or \"", Tcl_GetString(objv[0]),
		" scan dragto x y ?gain?\"", NULL);
	/*
	 * Ought to be:
	 * Tcl_WrongNumArgs(interp, 2, objc, "dragto x y ?gain?");
	 */
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((objc == 6) && (Tcl_GetIntFromObj(interp, objv[5], &gain) != TCL_OK)) {
	return TCL_ERROR;
    }
    c = Tcl_GetString(objv[2])[0];
    length = strlen(Tcl_GetString(objv[2]));
    if (c=='d' && strncmp(Tcl_GetString(objv[2]), "dragto", length)==0) {
	int newX, maxX;

	/*
	 * Amplify the difference between the current position and the mark
	 * position to compute how much the view should shift, then update the
	 * mark position to correspond to the new view. If we run off the edge
	 * of the text, reset the mark point so that the current position
	 * continues to correspond to the edge of the window. This means that
	 * the picture will start dragging as soon as the mouse reverses
	 * direction (without this reset, might have to slide mouse a long
	 * ways back before the picture starts moving again).
	 */

	newX = dInfoPtr->scanMarkXPixel + gain*(dInfoPtr->scanMarkX - x);
	maxX = 1 + dInfoPtr->maxLength - (dInfoPtr->maxX - dInfoPtr->x);
	if (newX < 0) {
	    newX = 0;
	    dInfoPtr->scanMarkXPixel = 0;
	    dInfoPtr->scanMarkX = x;
	} else if (newX > maxX) {
	    newX = maxX;
	    dInfoPtr->scanMarkXPixel = maxX;
	    dInfoPtr->scanMarkX = x;
	}
	dInfoPtr->newXPixelOffset = newX;

	totalScroll = gain*(dInfoPtr->scanMarkY - y);
	if (totalScroll != dInfoPtr->scanTotalYScroll) {
	    index = textPtr->topIndex;
	    YScrollByPixels(textPtr, totalScroll-dInfoPtr->scanTotalYScroll);
	    dInfoPtr->scanTotalYScroll = totalScroll;
	    if ((index.linePtr == textPtr->topIndex.linePtr) &&
		    (index.byteIndex == textPtr->topIndex.byteIndex)) {
		dInfoPtr->scanTotalYScroll = 0;
		dInfoPtr->scanMarkY = y;
	    }
	}
	dInfoPtr->flags |= DINFO_OUT_OF_DATE;
	if (!(dInfoPtr->flags & REDRAW_PENDING)) {
	    dInfoPtr->flags |= REDRAW_PENDING;
	    Tcl_DoWhenIdle(DisplayText, textPtr);
	}
    } else if (c=='m' && strncmp(Tcl_GetString(objv[2]), "mark", length)==0) {
	dInfoPtr->scanMarkXPixel = dInfoPtr->newXPixelOffset;
	dInfoPtr->scanMarkX = x;
	dInfoPtr->scanTotalYScroll = 0;
	dInfoPtr->scanMarkY = y;
    } else {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad scan option \"%s\": must be mark or dragto",
		Tcl_GetString(objv[2])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INDEX", "scan option",
		Tcl_GetString(objv[2]), NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetXView --
 *
 *	This function computes the fractions that indicate what's visible in a
 *	text window and, optionally, evaluates a Tcl script to report them to
 *	the text's associated scrollbar.
 *
 * Results:
 *	If report is zero, then the interp's result is filled in with two real
 *	numbers separated by a space, giving the position of the left and
 *	right edges of the window as fractions from 0 to 1, where 0 means the
 *	left edge of the text and 1 means the right edge. If report is
 *	non-zero, then the interp's result isn't modified directly, but
 *	instead a script is evaluated in interp to report the new horizontal
 *	scroll position to the scrollbar (if the scroll position hasn't
 *	changed then no script is invoked).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetXView(
    Tcl_Interp *interp,		/* If "report" is FALSE, string describing
				 * visible range gets stored in the interp's
				 * result. */
    TkText *textPtr,		/* Information about text widget. */
    int report)			/* Non-zero means report info to scrollbar if
				 * it has changed. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    double first, last;
    int code;
    Tcl_Obj *listObj;

    if (dInfoPtr->maxLength > 0) {
	first = ((double) dInfoPtr->curXPixelOffset)
		/ dInfoPtr->maxLength;
	last = ((double) (dInfoPtr->curXPixelOffset + dInfoPtr->maxX
		- dInfoPtr->x))/dInfoPtr->maxLength;
	if (last > 1.0) {
	    last = 1.0;
	}
    } else {
	first = 0;
	last = 1.0;
    }
    if (!report) {
	listObj = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(interp, listObj, Tcl_NewDoubleObj(first));
	Tcl_ListObjAppendElement(interp, listObj, Tcl_NewDoubleObj(last));
	Tcl_SetObjResult(interp, listObj);
	return;
    }
    if (FP_EQUAL_SCALE(first, dInfoPtr->xScrollFirst, dInfoPtr->maxLength) &&
	    FP_EQUAL_SCALE(last, dInfoPtr->xScrollLast, dInfoPtr->maxLength)) {
	return;
    }
    dInfoPtr->xScrollFirst = first;
    dInfoPtr->xScrollLast = last;
    if (textPtr->xScrollCmd != NULL) {
	char buf1[TCL_DOUBLE_SPACE+1];
	char buf2[TCL_DOUBLE_SPACE+1];
	Tcl_DString buf;

	buf1[0] = ' ';
	buf2[0] = ' ';
	Tcl_PrintDouble(NULL, first, buf1+1);
	Tcl_PrintDouble(NULL, last, buf2+1);
	Tcl_DStringInit(&buf);
	Tcl_DStringAppend(&buf, textPtr->xScrollCmd, -1);
	Tcl_DStringAppend(&buf, buf1, -1);
	Tcl_DStringAppend(&buf, buf2, -1);
	code = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
	Tcl_DStringFree(&buf);
	if (code != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (horizontal scrolling command executed by text)");
	    Tcl_BackgroundException(interp, code);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetYPixelCount --
 *
 *	How many pixels are there between the absolute top of the widget and
 *	the top of the given DLine.
 *
 *	While this function will work for any valid DLine, it is only ever
 *	called when dlPtr is the first display line in the widget (by
 *	'GetYView'). This means that usually this function is a very quick
 *	calculation, since it can use the pre-calculated linked-list of DLines
 *	for height information.
 *
 *	The only situation where this breaks down is if dlPtr's logical line
 *	wraps enough times to fill the text widget's current view - in this
 *	case we won't have enough dlPtrs in the linked list to be able to
 *	subtract off what we want.
 *
 * Results:
 *	The number of pixels.
 *
 *	This value has a valid range between '0' (the very top of the widget)
 *	and the number of pixels in the total widget minus the pixel-height of
 *	the last line.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetYPixelCount(
    TkText *textPtr,		/* Information about text widget. */
    DLine *dlPtr)		/* Information about the layout of a given
				 * index. */
{
    TkTextLine *linePtr = dlPtr->index.linePtr;
    int count;

    /*
     * Get the pixel count to the top of dlPtr's logical line. The rest of the
     * function is then concerned with updating 'count' for any difference
     * between the top of the logical line and the display line.
     */

    count = TkBTreePixelsTo(textPtr, linePtr);

    /*
     * For the common case where this dlPtr is also the start of the logical
     * line, we can return right away.
     */

    if (IsStartOfNotMergedLine(textPtr, &dlPtr->index)) {
	return count;
    }

    /*
     * Add on the logical line's height to reach one pixel beyond the bottom
     * of the logical line. And then subtract off the heights of all the
     * display lines from dlPtr to the end of its logical line.
     *
     * A different approach would be to lay things out from the start of the
     * logical line until we reach dlPtr, but since none of those are
     * pre-calculated, it'll usually take a lot longer. (But there are cases
     * where it would be more efficient: say if we're on the second of 1000
     * wrapped lines all from a single logical line - but that sort of
     * optimization is left for the future).
     */

    count += TkBTreeLinePixelCount(textPtr, linePtr);

    do {
	count -= dlPtr->height;
	if (dlPtr->nextPtr == NULL) {
	    /*
	     * We've run out of pre-calculated display lines, so we have to
	     * lay them out ourselves until the end of the logical line. Here
	     * is where we could be clever and ask: what's faster, to layout
	     * all lines from here to line-end, or all lines from the original
	     * dlPtr to the line-start? We just assume the former.
	     */

	    TkTextIndex index;
	    int notFirst = 0;

	    while (1) {
		TkTextIndexForwBytes(textPtr, &dlPtr->index,
			dlPtr->byteCount, &index);
		if (notFirst) {
		    FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);
		}
		if (index.linePtr != linePtr) {
		    break;
		}
		dlPtr = LayoutDLine(textPtr, &index);

		if (tkTextDebug) {
		    char string[TK_POS_CHARS];

		    /*
		     * Debugging is enabled, so keep a log of all the lines
		     * whose height was recalculated. The test suite uses this
		     * information.
		     */

		    TkTextPrintIndex(textPtr, &index, string);
		    LOG("tk_textHeightCalc", string);
		}
		count -= dlPtr->height;
		notFirst = 1;
	    }
	    break;
	}
	dlPtr = dlPtr->nextPtr;
    } while (dlPtr->index.linePtr == linePtr);

    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * GetYView --
 *
 *	This function computes the fractions that indicate what's visible in a
 *	text window and, optionally, evaluates a Tcl script to report them to
 *	the text's associated scrollbar.
 *
 * Results:
 *	If report is zero, then the interp's result is filled in with two real
 *	numbers separated by a space, giving the position of the top and
 *	bottom of the window as fractions from 0 to 1, where 0 means the
 *	beginning of the text and 1 means the end. If report is non-zero, then
 *	the interp's result isn't modified directly, but a script is evaluated
 *	in interp to report the new scroll position to the scrollbar (if the
 *	scroll position hasn't changed then no script is invoked).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetYView(
    Tcl_Interp *interp,		/* If "report" is FALSE, string describing
				 * visible range gets stored in the interp's
				 * result. */
    TkText *textPtr,		/* Information about text widget. */
    int report)			/* Non-zero means report info to scrollbar if
				 * it has changed. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    double first, last;
    DLine *dlPtr;
    int totalPixels, code, count;
    Tcl_Obj *listObj;

    dlPtr = dInfoPtr->dLinePtr;

    if (dlPtr == NULL) {
	return;
    }

    totalPixels = TkBTreeNumPixels(textPtr->sharedTextPtr->tree, textPtr);

    if (totalPixels == 0) {
	first = 0.0;
	last = 1.0;
    } else {
	/*
	 * Get the pixel count for the first visible pixel of the first
	 * visible line. If the first visible line is only partially visible,
	 * then we use 'topPixelOffset' to get the difference.
	 */

	count = GetYPixelCount(textPtr, dlPtr);
	first = (count + dInfoPtr->topPixelOffset) / (double) totalPixels;

	/*
	 * Add on the total number of visible pixels to get the count to one
	 * pixel _past_ the last visible pixel. This is how the 'yview'
	 * command is documented, and also explains why we are dividing by
	 * 'totalPixels' and not 'totalPixels-1'.
	 */

	while (1) {
	    int extra;

	    count += dlPtr->height;
	    extra = dlPtr->y + dlPtr->height - dInfoPtr->maxY;
	    if (extra > 0) {
		/*
		 * This much of the last line is not visible, so don't count
		 * these pixels. Since we've reached the bottom of the window,
		 * we break out of the loop.
		 */

		count -= extra;
		break;
	    }
	    if (dlPtr->nextPtr == NULL) {
		break;
	    }
	    dlPtr = dlPtr->nextPtr;
	}

	if (count > totalPixels) {
	    /*
	     * It can be possible, if we do not update each line's pixelHeight
	     * cache when we lay out individual DLines that the count
	     * generated here is more up-to-date than that maintained by the
	     * BTree. In such a case, the best we can do here is to fix up
	     * 'count' and continue, which might result in small, temporary
	     * perturbations to the size of the scrollbar. This is basically
	     * harmless, but in a perfect world we would not have this
	     * problem.
	     *
	     * For debugging purposes, if anyone wishes to improve the text
	     * widget further, the following 'panic' can be activated. In
	     * principle it should be possible to ensure the BTree is always
	     * at least as up to date as the display, so in the future we
	     * might be able to leave the 'panic' in permanently when we
	     * believe we have resolved the cache synchronisation issue.
	     *
	     * However, to achieve that goal would, I think, require a fairly
	     * substantial refactorisation of the code in this file so that
	     * there is much more obvious and explicit coordination between
	     * calls to LayoutDLine and updating of each TkTextLine's
	     * pixelHeight. The complicated bit is that LayoutDLine deals with
	     * individual display lines, but pixelHeight is for a logical
	     * line.
	     */

#if 0
	    Tcl_Panic("Counted more pixels (%d) than expected (%d) total "
		    "pixels in text widget scroll bar calculation.", count,
		    totalPixels);
#endif
	    count = totalPixels;
	}

	last = ((double) count)/((double)totalPixels);
    }

    if (!report) {
	listObj = Tcl_NewListObj(0,NULL);
	Tcl_ListObjAppendElement(interp, listObj, Tcl_NewDoubleObj(first));
	Tcl_ListObjAppendElement(interp, listObj, Tcl_NewDoubleObj(last));
	Tcl_SetObjResult(interp, listObj);
	return;
    }

    if (FP_EQUAL_SCALE(first, dInfoPtr->yScrollFirst, totalPixels) &&
	    FP_EQUAL_SCALE(last, dInfoPtr->yScrollLast, totalPixels)) {
	return;
    }

    dInfoPtr->yScrollFirst = first;
    dInfoPtr->yScrollLast = last;
    if (textPtr->yScrollCmd != NULL) {
	char buf1[TCL_DOUBLE_SPACE+1];
	char buf2[TCL_DOUBLE_SPACE+1];
	Tcl_DString buf;

	buf1[0] = ' ';
	buf2[0] = ' ';
	Tcl_PrintDouble(NULL, first, buf1+1);
	Tcl_PrintDouble(NULL, last, buf2+1);
	Tcl_DStringInit(&buf);
	Tcl_DStringAppend(&buf, textPtr->yScrollCmd, -1);
	Tcl_DStringAppend(&buf, buf1, -1);
	Tcl_DStringAppend(&buf, buf2, -1);
	code = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
	Tcl_DStringFree(&buf);
	if (code != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (vertical scrolling command executed by text)");
	    Tcl_BackgroundException(interp, code);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AsyncUpdateYScrollbar --
 *
 *	This function is called to update the vertical scrollbar asychronously
 *	as the pixel height calculations progress for lines in the widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See 'GetYView'. In particular the scrollbar position and size may be
 *	changed.
 *
 *----------------------------------------------------------------------
 */

static void
AsyncUpdateYScrollbar(
    ClientData clientData)	/* Information about widget. */
{
    register TkText *textPtr = clientData;

    textPtr->dInfoPtr->scrollbarTimer = NULL;

    if (!(textPtr->flags & DESTROYED)) {
	GetYView(textPtr->interp, textPtr, 1);
    }

    if (textPtr->refCount-- <= 1) {
	ckfree(textPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FindDLine --
 *
 *	This function is called to find the DLine corresponding to a given
 *	text index.
 *
 * Results:
 *	The return value is a pointer to the first DLine found in the list
 *	headed by dlPtr that displays information at or after the specified
 *	position. If there is no such line in the list then NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static DLine *
FindDLine(
    TkText *textPtr,		/* Widget record for text widget. */
    register DLine *dlPtr,	/* Pointer to first in list of DLines to
				 * search. */
    const TkTextIndex *indexPtr)/* Index of desired character. */
{
    DLine *dlPtrPrev;
    TkTextIndex indexPtr2;

    if (dlPtr == NULL) {
	return NULL;
    }
    if (TkBTreeLinesTo(NULL, indexPtr->linePtr)
	    < TkBTreeLinesTo(NULL, dlPtr->index.linePtr)) {
	/*
	 * The first display line is already past the desired line.
	 */

	return dlPtr;
    }

    /*
     * The display line containing the desired index is such that the index
     * of the first character of this display line is at or before the
     * desired index, and the index of the first character of the next
     * display line is after the desired index.
     */

    while (TkTextIndexCmp(&dlPtr->index,indexPtr) < 0) {
        dlPtrPrev = dlPtr;
        dlPtr = dlPtr->nextPtr;
        if (dlPtr == NULL) {
            /*
             * We're past the last display line, either because the desired
             * index lies past the visible text, or because the desired index
             * is on the last display line.
             */
            indexPtr2 = dlPtrPrev->index;
            TkTextIndexForwBytes(textPtr, &indexPtr2, dlPtrPrev->byteCount,
                    &indexPtr2);
            if (TkTextIndexCmp(&indexPtr2,indexPtr) > 0) {
                /*
                 * The desired index is on the last display line.
                 * --> return this display line.
                 */
                dlPtr = dlPtrPrev;
            } else {
                /*
                 * The desired index is past the visible text. There is no
                 * display line displaying something at the desired index.
                 * --> return NULL.
                 */
            }
            break;
        }
        if (TkTextIndexCmp(&dlPtr->index,indexPtr) > 0) {
            /*
             * If we're here then we would normally expect that:
             *   dlPtrPrev->index  <=  indexPtr  <  dlPtr->index
             * i.e. we have found the searched display line being dlPtr.
             * However it is possible that some DLines were unlinked
             * previously, leading to a situation where going through
             * the list of display lines skips display lines that did
             * exist just a moment ago.
             */
            indexPtr2 = dlPtrPrev->index;
            TkTextIndexForwBytes(textPtr, &indexPtr2, dlPtrPrev->byteCount,
                    &indexPtr2);
            if (TkTextIndexCmp(&indexPtr2,indexPtr) > 0) {
                /*
                 * Confirmed:
                 *   dlPtrPrev->index  <=  indexPtr  <  dlPtr->index
                 * --> return dlPtrPrev.
                 */
                dlPtr = dlPtrPrev;
            } else {
                /*
                 * The last (rightmost) index shown by dlPtrPrev is still
                 * before the desired index. This may be because there was
                 * previously a display line between dlPtrPrev and dlPtr
                 * and this display line has been unlinked.
                 * --> return dlPtr.
                 */
            }
            break;
        }
    }

    return dlPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * IsStartOfNotMergedLine --
 *
 *	This function checks whether the given index is the start of a
 *      logical line that is not merged with the previous logical line
 *      (due to elision of the eol of the previous line).
 *
 * Results:
 *	Returns whether the given index denotes the first index of a
*       logical line not merged with its previous line.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
IsStartOfNotMergedLine(
      TkText *textPtr,              /* Widget record for text widget. */
      const TkTextIndex *indexPtr)  /* Index to check. */
{
    TkTextIndex indexPtr2;

    if (indexPtr->byteIndex != 0) {
        /*
         * Not the start of a logical line.
         */
        return 0;
    }

    if (TkTextIndexBackBytes(textPtr, indexPtr, 1, &indexPtr2)) {
        /*
         * indexPtr is the first index of the text widget.
         */
        return 1;
    }

    if (!TkTextIsElided(textPtr, &indexPtr2, NULL)) {
        /*
         * The eol of the line just before indexPtr is elided.
         */
        return 1;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextPixelIndex --
 *
 *	Given an (x,y) coordinate on the screen, find the location of the
 *	character closest to that location.
 *
 * Results:
 *	The index at *indexPtr is modified to refer to the character on the
 *	display that is closest to (x,y).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkTextPixelIndex(
    TkText *textPtr,		/* Widget record for text widget. */
    int x, int y,		/* Pixel coordinates of point in widget's
				 * window. */
    TkTextIndex *indexPtr,	/* This index gets filled in with the index of
				 * the character nearest to (x,y). */
    int *nearest)		/* If non-NULL then gets set to 0 if (x,y) is
				 * actually over the returned index, and 1 if
				 * it is just nearby (e.g. if x,y is on the
				 * border of the widget). */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    register DLine *dlPtr, *validDlPtr;
    int nearby = 0;

    /*
     * Make sure that all of the layout information about what's displayed
     * where on the screen is up-to-date.
     */

    if (dInfoPtr->flags & DINFO_OUT_OF_DATE) {
	UpdateDisplayInfo(textPtr);
    }

    /*
     * If the coordinates are above the top of the window, then adjust them to
     * refer to the upper-right corner of the window. If they're off to one
     * side or the other, then adjust to the closest side.
     */

    if (y < dInfoPtr->y) {
	y = dInfoPtr->y;
	x = dInfoPtr->x;
	nearby = 1;
    }
    if (x >= dInfoPtr->maxX) {
	x = dInfoPtr->maxX - 1;
	nearby = 1;
    }
    if (x < dInfoPtr->x) {
	x = dInfoPtr->x;
	nearby = 1;
    }

    /*
     * Find the display line containing the desired y-coordinate.
     */

    if (dInfoPtr->dLinePtr == NULL) {
	if (nearest != NULL) {
	    *nearest = 1;
	}
	*indexPtr = textPtr->topIndex;
	return;
    }
    for (dlPtr = validDlPtr = dInfoPtr->dLinePtr;
	    y >= (dlPtr->y + dlPtr->height);
	    dlPtr = dlPtr->nextPtr) {
	if (dlPtr->chunkPtr != NULL) {
	    validDlPtr = dlPtr;
	}
	if (dlPtr->nextPtr == NULL) {
	    /*
	     * Y-coordinate is off the bottom of the displayed text. Use the
	     * last character on the last line.
	     */

	    x = dInfoPtr->maxX - 1;
	    nearby = 1;
	    break;
	}
    }
    if (dlPtr->chunkPtr == NULL) {
	dlPtr = validDlPtr;
    }

    if (nearest != NULL) {
	*nearest = nearby;
    }

    DlineIndexOfX(textPtr, dlPtr, x, indexPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DlineIndexOfX --
 *
 *	Given an x coordinate in a display line, find the index of the
 *	character closest to that location.
 *
 *	This is effectively the opposite of DlineXOfIndex.
 *
 * Results:
 *	The index at *indexPtr is modified to refer to the character on the
 *	display line that is closest to x.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
DlineIndexOfX(
    TkText *textPtr,		/* Widget record for text widget. */
    DLine *dlPtr,		/* Display information for this display
				 * line. */
    int x,			/* Pixel x coordinate of point in widget's
				 * window. */
    TkTextIndex *indexPtr)	/* This index gets filled in with the index of
				 * the character nearest to x. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    register TkTextDispChunk *chunkPtr;

    /*
     * Scan through the line's chunks to find the one that contains the
     * desired x-coordinate. Before doing this, translate the x-coordinate
     * from the coordinate system of the window to the coordinate system of
     * the line (to take account of x-scrolling).
     */

    *indexPtr = dlPtr->index;
    x = x - dInfoPtr->x + dInfoPtr->curXPixelOffset;
    chunkPtr = dlPtr->chunkPtr;

    if (chunkPtr == NULL || x == 0) {
	/*
	 * This may occur if everything is elided, or if we're simply already
	 * at the beginning of the line.
	 */

	return;
    }

    while (x >= (chunkPtr->x + chunkPtr->width)) {
	/*
	 * Note that this forward then backward movement of the index can be
	 * problematic at the end of the buffer (we can't move forward, and
	 * then when we move backward, we do, leading to the wrong position).
	 * Hence when x == 0 we take special action above.
	 */

	if (TkTextIndexForwBytes(NULL,indexPtr,chunkPtr->numBytes,indexPtr)) {
	    /*
	     * We've reached the end of the text.
	     */

            TkTextIndexBackChars(NULL, indexPtr, 1, indexPtr, COUNT_INDICES);
	    return;
	}
	if (chunkPtr->nextPtr == NULL) {
	    /*
	     * We've reached the end of the display line.
	     */

            TkTextIndexBackChars(NULL, indexPtr, 1, indexPtr, COUNT_INDICES);
	    return;
	}
	chunkPtr = chunkPtr->nextPtr;
    }

    /*
     * If the chunk has more than one byte in it, ask it which character is at
     * the desired location. In this case we can manipulate
     * 'indexPtr->byteIndex' directly, because we know we're staying inside a
     * single logical line.
     */

    if (chunkPtr->numBytes > 1) {
	indexPtr->byteIndex += chunkPtr->measureProc(chunkPtr, x);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextIndexOfX --
 *
 *	Given a logical x coordinate (i.e. distance in pixels from the
 *	beginning of the display line, not taking into account any information
 *	about the window, scrolling etc.) on the display line starting with
 *	the given index, adjust that index to refer to the object under the x
 *	coordinate.
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
TkTextIndexOfX(
    TkText *textPtr,		/* Widget record for text widget. */
    int x,			/* The x coordinate for which we want the
				 * index. */
    TkTextIndex *indexPtr)	/* Index of display line start, which will be
				 * adjusted to the index under the given x
				 * coordinate. */
{
    DLine *dlPtr = LayoutDLine(textPtr, indexPtr);
    DlineIndexOfX(textPtr, dlPtr, x + textPtr->dInfoPtr->x
		- textPtr->dInfoPtr->curXPixelOffset, indexPtr);
    FreeDLines(textPtr, dlPtr, NULL, DLINE_FREE_TEMP);
}

/*
 *----------------------------------------------------------------------
 *
 * DlineXOfIndex --
 *
 *	Given a relative byte index on a given display line (i.e. the number
 *	of byte indices from the beginning of the given display line), find
 *	the x coordinate of that index within the abstract display line,
 *	without adjusting for the x-scroll state of the line.
 *
 *	This is effectively the opposite of DlineIndexOfX.
 *
 *	NB. The 'byteIndex' is relative to the display line, NOT the logical
 *	line.
 *
 * Results:
 *	The x coordinate.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
DlineXOfIndex(
    TkText *textPtr,		/* Widget record for text widget. */
    DLine *dlPtr,		/* Display information for this display
				 * line. */
    int byteIndex)		/* The byte index for which we want the
				 * coordinate. */
{
    register TkTextDispChunk *chunkPtr = dlPtr->chunkPtr;
    int x = 0;

    if (byteIndex == 0 || chunkPtr == NULL) {
	return x;
    }

    /*
     * Scan through the line's chunks to find the one that contains the
     * desired byte index.
     */

    chunkPtr = dlPtr->chunkPtr;
    while (byteIndex > 0) {
	if (byteIndex < chunkPtr->numBytes) {
	    int y, width, height;

	    chunkPtr->bboxProc(textPtr, chunkPtr, byteIndex,
		    dlPtr->y + dlPtr->spaceAbove,
		    dlPtr->height - dlPtr->spaceAbove - dlPtr->spaceBelow,
		    dlPtr->baseline - dlPtr->spaceAbove, &x, &y, &width,
		    &height);
	    break;
	}
	byteIndex -= chunkPtr->numBytes;
	if (chunkPtr->nextPtr == NULL || byteIndex == 0) {
	    x = chunkPtr->x + chunkPtr->width;
	    break;
	}
	chunkPtr = chunkPtr->nextPtr;
    }

    return x;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextIndexBbox --
 *
 *	Given an index, find the bounding box of the screen area occupied by
 *	the entity (character, window, image) at that index.
 *
 * Results:
 *	Zero is returned if the index is on the screen. -1 means the index is
 *	not on the screen. If the return value is 0, then the bounding box of
 *	the part of the index that's visible on the screen is returned to
 *	*xPtr, *yPtr, *widthPtr, and *heightPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkTextIndexBbox(
    TkText *textPtr,		/* Widget record for text widget. */
    const TkTextIndex *indexPtr,/* Index whose bounding box is desired. */
    int *xPtr, int *yPtr,	/* Filled with index's upper-left
				 * coordinate. */
    int *widthPtr, int *heightPtr,
				/* Filled in with index's dimensions. */
    int *charWidthPtr)		/* If the 'index' is at the end of a display
				 * line and therefore takes up a very large
				 * width, this is used to return the smaller
				 * width actually desired by the index. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    DLine *dlPtr;
    register TkTextDispChunk *chunkPtr;
    int byteCount;

    /*
     * Make sure that all of the screen layout information is up to date.
     */

    if (dInfoPtr->flags & DINFO_OUT_OF_DATE) {
	UpdateDisplayInfo(textPtr);
    }

    /*
     * Find the display line containing the desired index.
     */

    dlPtr = FindDLine(textPtr, dInfoPtr->dLinePtr, indexPtr);

    /*
     * Two cases shall be trapped here because the logic later really
     * needs dlPtr to be the display line containing indexPtr:
     *   1. if no display line contains the desired index (NULL dlPtr)
     *   2. if indexPtr is before the first display line, in which case
     *      dlPtr currently points to the first display line
     */

    if ((dlPtr == NULL) || (TkTextIndexCmp(&dlPtr->index, indexPtr) > 0)) {
	return -1;
    }

    /*
     * Find the chunk within the display line that contains the desired
     * index. The chunks making the display line are skipped up to but not
     * including the one crossing indexPtr. Skipping is done based on
     * a byteCount offset possibly spanning several logical lines in case
     * they are elided.
     */

    byteCount = TkTextIndexCountBytes(textPtr, &dlPtr->index, indexPtr);
    for (chunkPtr = dlPtr->chunkPtr; ; chunkPtr = chunkPtr->nextPtr) {
	if (chunkPtr == NULL) {
	    return -1;
	}
	if (byteCount < chunkPtr->numBytes) {
	    break;
	}
	byteCount -= chunkPtr->numBytes;
    }

    /*
     * Call a chunk-specific function to find the horizontal range of the
     * character within the chunk, then fill in the vertical range. The
     * x-coordinate returned by bboxProc is a coordinate within a line, not a
     * coordinate on the screen. Translate it to reflect horizontal scrolling.
     */

    chunkPtr->bboxProc(textPtr, chunkPtr, byteCount,
	    dlPtr->y + dlPtr->spaceAbove,
	    dlPtr->height - dlPtr->spaceAbove - dlPtr->spaceBelow,
	    dlPtr->baseline - dlPtr->spaceAbove, xPtr, yPtr, widthPtr,
	    heightPtr);
    *xPtr = *xPtr + dInfoPtr->x - dInfoPtr->curXPixelOffset;
    if ((byteCount == chunkPtr->numBytes-1) && (chunkPtr->nextPtr == NULL)) {
	/*
	 * Last character in display line. Give it all the space up to the
	 * line.
	 */

	if (charWidthPtr != NULL) {
	    *charWidthPtr = dInfoPtr->maxX - *xPtr;
            if (*charWidthPtr > textPtr->charWidth) {
                *charWidthPtr = textPtr->charWidth;
            }
	}
	if (*xPtr > dInfoPtr->maxX) {
	    *xPtr = dInfoPtr->maxX;
	}
	*widthPtr = dInfoPtr->maxX - *xPtr;
    } else {
	if (charWidthPtr != NULL) {
	    *charWidthPtr = *widthPtr;
	}
    }
    if (*widthPtr == 0) {
	/*
	 * With zero width (e.g. elided text) we just need to make sure it is
	 * onscreen, where the '=' case here is ok.
	 */

	if (*xPtr < dInfoPtr->x) {
	    return -1;
	}
    } else {
	if ((*xPtr + *widthPtr) <= dInfoPtr->x) {
	    return -1;
	}
    }
    if ((*xPtr + *widthPtr) > dInfoPtr->maxX) {
	*widthPtr = dInfoPtr->maxX - *xPtr;
	if (*widthPtr <= 0) {
	    return -1;
	}
    }
    if ((*yPtr + *heightPtr) > dInfoPtr->maxY) {
	*heightPtr = dInfoPtr->maxY - *yPtr;
	if (*heightPtr <= 0) {
	    return -1;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextDLineInfo --
 *
 *	Given an index, return information about the display line containing
 *	that character.
 *
 * Results:
 *	Zero is returned if the character is on the screen. -1 means the
 *	character isn't on the screen. If the return value is 0, then
 *	information is returned in the variables pointed to by xPtr, yPtr,
 *	widthPtr, heightPtr, and basePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkTextDLineInfo(
    TkText *textPtr,		/* Widget record for text widget. */
    const TkTextIndex *indexPtr,/* Index of character whose bounding box is
				 * desired. */
    int *xPtr, int *yPtr,	/* Filled with line's upper-left
				 * coordinate. */
    int *widthPtr, int *heightPtr,
				/* Filled in with line's dimensions. */
    int *basePtr)		/* Filled in with the baseline position,
				 * measured as an offset down from *yPtr. */
{
    TextDInfo *dInfoPtr = textPtr->dInfoPtr;
    DLine *dlPtr;
    int dlx;

    /*
     * Make sure that all of the screen layout information is up to date.
     */

    if (dInfoPtr->flags & DINFO_OUT_OF_DATE) {
	UpdateDisplayInfo(textPtr);
    }

    /*
     * Find the display line containing the desired index.
     */

    dlPtr = FindDLine(textPtr, dInfoPtr->dLinePtr, indexPtr);

    /*
     * Two cases shall be trapped here because the logic later really
     * needs dlPtr to be the display line containing indexPtr:
     *   1. if no display line contains the desired index (NULL dlPtr)
     *   2. if indexPtr is before the first display line, in which case
     *      dlPtr currently points to the first display line
     */

    if ((dlPtr == NULL) || (TkTextIndexCmp(&dlPtr->index, indexPtr) > 0)) {
	return -1;
    }

    dlx = (dlPtr->chunkPtr != NULL? dlPtr->chunkPtr->x: 0);
    *xPtr = dInfoPtr->x - dInfoPtr->curXPixelOffset + dlx;
    *widthPtr = dlPtr->length - dlx;
    *yPtr = dlPtr->y;
    if ((dlPtr->y + dlPtr->height) > dInfoPtr->maxY) {
	*heightPtr = dInfoPtr->maxY - dlPtr->y;
    } else {
	*heightPtr = dlPtr->height;
    }
    *basePtr = dlPtr->baseline;
    return 0;
}

/*
 * Get bounding-box information about an elided chunk.
 */

static void
ElideBboxProc(
    TkText *textPtr,
    TkTextDispChunk *chunkPtr,	/* Chunk containing desired char. */
    int index,			/* Index of desired character within the
				 * chunk. */
    int y,			/* Topmost pixel in area allocated for this
				 * line. */
    int lineHeight,		/* Height of line, in pixels. */
    int baseline,		/* Location of line's baseline, in pixels
				 * measured down from y. */
    int *xPtr, int *yPtr,	/* Gets filled in with coords of character's
				 * upper-left pixel. X-coord is in same
				 * coordinate system as chunkPtr->x. */
    int *widthPtr,		/* Gets filled in with width of character, in
				 * pixels. */
    int *heightPtr)		/* Gets filled in with height of character, in
				 * pixels. */
{
    *xPtr = chunkPtr->x;
    *yPtr = y;
    *widthPtr = *heightPtr = 0;
}

/*
 * Measure an elided chunk.
 */

static int
ElideMeasureProc(
    TkTextDispChunk *chunkPtr,	/* Chunk containing desired coord. */
    int x)			/* X-coordinate, in same coordinate system as
				 * chunkPtr->x. */
{
    return 0 /*chunkPtr->numBytes - 1*/;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextCharLayoutProc --
 *
 *	This function is the "layoutProc" for character segments.
 *
 * Results:
 *	If there is something to display for the chunk then a non-zero value
 *	is returned and the fields of chunkPtr will be filled in (see the
 *	declaration of TkTextDispChunk in tkText.h for details). If zero is
 *	returned it means that no characters from this chunk fit in the
 *	window. If -1 is returned it means that this segment just doesn't need
 *	to be displayed (never happens for text).
 *
 * Side effects:
 *	Memory is allocated to hold additional information about the chunk.
 *
 *--------------------------------------------------------------
 */

int
TkTextCharLayoutProc(
    TkText *textPtr,		/* Text widget being layed out. */
    TkTextIndex *indexPtr,	/* Index of first character to lay out
				 * (corresponds to segPtr and offset). */
    TkTextSegment *segPtr,	/* Segment being layed out. */
    int byteOffset,		/* Byte offset within segment of first
				 * character to consider. */
    int maxX,			/* Chunk must not occupy pixels at this
				 * position or higher. */
    int maxBytes,		/* Chunk must not include more than this many
				 * characters. */
    int noCharsYet,		/* Non-zero means no characters have been
				 * assigned to this display line yet. */
    TkWrapMode wrapMode,	/* How to handle line wrapping:
				 * TEXT_WRAPMODE_CHAR, TEXT_WRAPMODE_NONE, or
				 * TEXT_WRAPMODE_WORD. */
    register TkTextDispChunk *chunkPtr)
				/* Structure to fill in with information about
				 * this chunk. The x field has already been
				 * set by the caller. */
{
    Tk_Font tkfont;
    int nextX, bytesThatFit, count;
    CharInfo *ciPtr;
    char *p;
    TkTextSegment *nextPtr;
    Tk_FontMetrics fm;
#if TK_LAYOUT_WITH_BASE_CHUNKS
    const char *line;
    int lineOffset;
    BaseCharInfo *bciPtr;
    Tcl_DString *baseString;
#endif

    /*
     * Figure out how many characters will fit in the space we've got. Include
     * the next character, even though it won't fit completely, if any of the
     * following is true:
     *	 (a) the chunk contains no characters and the display line contains no
     *	     characters yet (i.e. the line isn't wide enough to hold even a
     *	     single character).
     *	 (b) at least one pixel of the character is visible, we have not
     *	     already exceeded the character limit, and the next character is a
     *	     white space character.
     * In the specific case of 'word' wrapping mode however, include all space
     * characters following the characters that fit in the space we've got,
     * even if no pixel of them is visible.
     */

    p = segPtr->body.chars + byteOffset;
    tkfont = chunkPtr->stylePtr->sValuePtr->tkfont;

#if TK_LAYOUT_WITH_BASE_CHUNKS
    if (baseCharChunkPtr == NULL) {
	baseCharChunkPtr = chunkPtr;
	bciPtr = ckalloc(sizeof(BaseCharInfo));
	baseString = &bciPtr->baseChars;
	Tcl_DStringInit(baseString);
	bciPtr->width = 0;

	ciPtr = &bciPtr->ci;
    } else {
	bciPtr = baseCharChunkPtr->clientData;
	ciPtr = ckalloc(sizeof(CharInfo));
	baseString = &bciPtr->baseChars;
    }

    lineOffset = Tcl_DStringLength(baseString);
    line = Tcl_DStringAppend(baseString,p,maxBytes);

    chunkPtr->clientData = ciPtr;
    ciPtr->baseChunkPtr = baseCharChunkPtr;
    ciPtr->baseOffset = lineOffset;
    ciPtr->chars = NULL;
    ciPtr->numBytes = 0;

    bytesThatFit = CharChunkMeasureChars(chunkPtr, line,
	    lineOffset + maxBytes, lineOffset, -1, chunkPtr->x, maxX,
	    TK_ISOLATE_END, &nextX);
#else /* !TK_LAYOUT_WITH_BASE_CHUNKS */
    bytesThatFit = CharChunkMeasureChars(chunkPtr, p, maxBytes, 0, -1,
	    chunkPtr->x, maxX, TK_ISOLATE_END, &nextX);
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */

    if (bytesThatFit < maxBytes) {
	if ((bytesThatFit == 0) && noCharsYet) {
	    int ch;
	    int chLen = TkUtfToUniChar(p, &ch);

#if TK_LAYOUT_WITH_BASE_CHUNKS
	    bytesThatFit = CharChunkMeasureChars(chunkPtr, line,
		    lineOffset+chLen, lineOffset, -1, chunkPtr->x, -1, 0,
		    &nextX);
#else /* !TK_LAYOUT_WITH_BASE_CHUNKS */
	    bytesThatFit = CharChunkMeasureChars(chunkPtr, p, chLen, 0, -1,
		    chunkPtr->x, -1, 0, &nextX);
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */
	}
	if ((nextX < maxX) && ((p[bytesThatFit] == ' ')
		|| (p[bytesThatFit] == '\t'))) {
	    /*
	     * Space characters are funny, in that they are considered to fit
	     * if there is at least one pixel of space left on the line. Just
	     * give the space character whatever space is left.
	     */

	    nextX = maxX;
	    bytesThatFit++;
	}
        if (wrapMode == TEXT_WRAPMODE_WORD) {
            while (p[bytesThatFit] == ' ') {
                /*
                 * Space characters that would go at the beginning of the
                 * next line are allocated to the current line. This gives
                 * the effect of trimming white spaces that would otherwise
                 * be seen at the beginning of wrapped lines.
                 * Note that testing for '\t' is useless here because the
                 * chunk always includes at most one trailing \t, see
                 * LayoutDLine.
                 */

                bytesThatFit++;
            }
        }
	if (p[bytesThatFit] == '\n') {
	    /*
	     * A newline character takes up no space, so if the previous
	     * character fits then so does the newline.
	     */

	    bytesThatFit++;
	}
	if (bytesThatFit == 0) {
#if TK_LAYOUT_WITH_BASE_CHUNKS
	    chunkPtr->clientData = NULL;
	    if (chunkPtr == baseCharChunkPtr) {
		baseCharChunkPtr = NULL;
		Tcl_DStringFree(baseString);
	    } else {
		Tcl_DStringSetLength(baseString,lineOffset);
	    }
	    ckfree(ciPtr);
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */
	    return 0;
	}
    }

    Tk_GetFontMetrics(tkfont, &fm);

    /*
     * Fill in the chunk structure and allocate and initialize a CharInfo
     * structure. If the last character is a newline then don't bother to
     * display it.
     */

    chunkPtr->displayProc = CharDisplayProc;
    chunkPtr->undisplayProc = CharUndisplayProc;
    chunkPtr->measureProc = CharMeasureProc;
    chunkPtr->bboxProc = CharBboxProc;
    chunkPtr->numBytes = bytesThatFit;
    chunkPtr->minAscent = fm.ascent + chunkPtr->stylePtr->sValuePtr->offset;
    chunkPtr->minDescent = fm.descent - chunkPtr->stylePtr->sValuePtr->offset;
    chunkPtr->minHeight = 0;
    chunkPtr->width = nextX - chunkPtr->x;
    chunkPtr->breakIndex = -1;

#if !TK_LAYOUT_WITH_BASE_CHUNKS
    ciPtr = ckalloc((Tk_Offset(CharInfo, chars) + 1) + bytesThatFit);
    chunkPtr->clientData = ciPtr;
    memcpy(ciPtr->chars, p, (unsigned) bytesThatFit);
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */

    ciPtr->numBytes = bytesThatFit;
    if (p[bytesThatFit - 1] == '\n') {
	ciPtr->numBytes--;
    }

#if TK_LAYOUT_WITH_BASE_CHUNKS
    /*
     * Final update for the current base chunk data.
     */

    Tcl_DStringSetLength(baseString,lineOffset+ciPtr->numBytes);
    bciPtr->width = nextX - baseCharChunkPtr->x;

    /*
     * Finalize the base chunk if this chunk ends in a tab, which definitly
     * breaks the context and needs to be handled on a higher level.
     */

    if (ciPtr->numBytes > 0 && p[ciPtr->numBytes - 1] == '\t') {
	FinalizeBaseChunk(chunkPtr);
    }
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */

    /*
     * Compute a break location. If we're in word wrap mode, a break can occur
     * after any space character, or at the end of the chunk if the next
     * segment (ignoring those with zero size) is not a character segment.
     */

    if (wrapMode != TEXT_WRAPMODE_WORD) {
	chunkPtr->breakIndex = chunkPtr->numBytes;
    } else {
	for (count = bytesThatFit, p += bytesThatFit - 1; count > 0;
		count--, p--) {
	    /*
	     * Don't use isspace(); effects are unpredictable and can lead to
	     * odd word-wrapping problems on some platforms. Also don't use
	     * Tcl_UniCharIsSpace here either, as it identifies non-breaking
	     * spaces as places to break. What we actually want is only the
	     * ASCII space characters, so use them explicitly...
	     */

	    switch (*p) {
	    case '\t': case '\n': case '\v': case '\f': case '\r': case ' ':
		chunkPtr->breakIndex = count;
		goto checkForNextChunk;
	    }
	}
    checkForNextChunk:
	if ((bytesThatFit + byteOffset) == segPtr->size) {
	    for (nextPtr = segPtr->nextPtr; nextPtr != NULL;
		    nextPtr = nextPtr->nextPtr) {
		if (nextPtr->size != 0) {
		    if (nextPtr->typePtr != &tkTextCharType) {
			chunkPtr->breakIndex = chunkPtr->numBytes;
		    }
		    break;
		}
	    }
	}
    }
    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * CharChunkMeasureChars --
 *
 *	Determine the number of characters from a char chunk that will fit in
 *	the given horizontal span.
 *
 *	This is the same as MeasureChars (which see), but in the context of a
 *	char chunk, i.e. on a higher level of abstraction. Use this function
 *	whereever possible instead of plain MeasureChars, so that the right
 *	context is used automatically.
 *
 * Results:
 *	The return value is the number of bytes from the range of start to end
 *	in source that fit in the span given by startX and maxX. *nextXPtr is
 *	filled in with the x-coordinate at which the first character that
 *	didn't fit would be drawn, if it were to be drawn.
 *
 * Side effects:
 *	None.
 *--------------------------------------------------------------
 */

static int
CharChunkMeasureChars(
    TkTextDispChunk *chunkPtr,	/* Chunk from which to measure. */
    const char *chars,		/* Chars to use, instead of the chunk's own.
				 * Used by the layoutproc during chunk setup.
				 * All other callers use NULL. Not
				 * NUL-terminated. */
    int charsLen,		/* Length of the "chars" parameter. */
    int start, int end,		/* The range of chars to measure inside the
				 * chunk (or inside the additional chars). */
    int startX,			/* Starting x coordinate where the measured
				 * span will begin. */
    int maxX,			/* Maximum pixel width of the span. May be -1
				 * for unlimited. */
    int flags,			/* Flags to pass to MeasureChars. */
    int *nextXPtr)		/* The function puts the newly calculated
				 * right border x-position of the span
				 * here. */
{
    Tk_Font tkfont = chunkPtr->stylePtr->sValuePtr->tkfont;
    CharInfo *ciPtr = chunkPtr->clientData;

#if !TK_LAYOUT_WITH_BASE_CHUNKS
    if (chars == NULL) {
	chars = ciPtr->chars;
	charsLen = ciPtr->numBytes;
    }
    if (end == -1) {
	end = charsLen;
    }

    return MeasureChars(tkfont, chars, charsLen, start, end-start,
	    startX, maxX, flags, nextXPtr);
#else /* TK_LAYOUT_WITH_BASE_CHUNKS */
    {
	int xDisplacement;
	int fit, bstart = start, bend = end;

	if (chars == NULL) {
	    Tcl_DString *baseChars = &((BaseCharInfo *)
		    ciPtr->baseChunkPtr->clientData)->baseChars;

	    chars = Tcl_DStringValue(baseChars);
	    charsLen = Tcl_DStringLength(baseChars);
	    bstart += ciPtr->baseOffset;
	    if (bend == -1) {
		bend = ciPtr->baseOffset + ciPtr->numBytes;
	    } else {
		bend += ciPtr->baseOffset;
	    }
	} else if (bend == -1) {
	    bend = charsLen;
	}

	if (bstart == ciPtr->baseOffset) {
	    xDisplacement = startX - chunkPtr->x;
	} else {
	    int widthUntilStart = 0;

	    MeasureChars(tkfont, chars, charsLen, 0, bstart,
		    0, -1, 0, &widthUntilStart);
	    xDisplacement = startX - widthUntilStart - ciPtr->baseChunkPtr->x;
	}

	fit = MeasureChars(tkfont, chars, charsLen, 0, bend,
		ciPtr->baseChunkPtr->x + xDisplacement, maxX, flags, nextXPtr);

	if (fit < bstart) {
	    return 0;
	} else {
	    return fit - bstart;
	}
    }
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */
}

/*
 *--------------------------------------------------------------
 *
 * CharDisplayProc --
 *
 *	This function is called to display a character chunk on the screen or
 *	in an off-screen pixmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Graphics are drawn.
 *
 *--------------------------------------------------------------
 */

static void
CharDisplayProc(
    TkText *textPtr,
    TkTextDispChunk *chunkPtr,	/* Chunk that is to be drawn. */
    int x,			/* X-position in dst at which to draw this
				 * chunk (may differ from the x-position in
				 * the chunk because of scrolling). */
    int y,			/* Y-position at which to draw this chunk in
				 * dst. */
    int height,			/* Total height of line. */
    int baseline,		/* Offset of baseline from y. */
    Display *display,		/* Display to use for drawing. */
    Drawable dst,		/* Pixmap or window in which to draw chunk. */
    int screenY)		/* Y-coordinate in text window that
				 * corresponds to y. */
{
    CharInfo *ciPtr = chunkPtr->clientData;
    const char *string;
    TextStyle *stylePtr;
    StyleValues *sValuePtr;
    int numBytes, offsetBytes, offsetX;
#if TK_DRAW_IN_CONTEXT
    BaseCharInfo *bciPtr;
#endif /* TK_DRAW_IN_CONTEXT */

    if ((x + chunkPtr->width) <= 0) {
	/*
	 * The chunk is off-screen.
	 */

	return;
    }

#if TK_DRAW_IN_CONTEXT
    bciPtr = ciPtr->baseChunkPtr->clientData;
    numBytes = Tcl_DStringLength(&bciPtr->baseChars);
    string = Tcl_DStringValue(&bciPtr->baseChars);

#elif TK_LAYOUT_WITH_BASE_CHUNKS
    if (ciPtr->baseChunkPtr != chunkPtr) {
	/*
	 * Without context drawing only base chunks display their foreground.
	 */

	return;
    }

    numBytes = Tcl_DStringLength(&((BaseCharInfo *) ciPtr)->baseChars);
    string = ciPtr->chars;

#else /* !TK_LAYOUT_WITH_BASE_CHUNKS */
    numBytes = ciPtr->numBytes;
    string = ciPtr->chars;
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */

    stylePtr = chunkPtr->stylePtr;
    sValuePtr = stylePtr->sValuePtr;

    /*
     * If the text sticks out way to the left of the window, skip over the
     * characters that aren't in the visible part of the window. This is
     * essential if x is very negative (such as less than 32K); otherwise
     * overflow problems will occur in servers that use 16-bit arithmetic,
     * like X.
     */

    offsetX = x;
    offsetBytes = 0;
    if (x < 0) {
	offsetBytes = CharChunkMeasureChars(chunkPtr, NULL, 0, 0, -1,
		x, 0, 0, &offsetX);
    }

    /*
     * Draw the text, underline, and overstrike for this chunk.
     */

    if (!sValuePtr->elide && (numBytes > offsetBytes)
	    && (stylePtr->fgGC != NULL)) {
#if TK_DRAW_IN_CONTEXT
	int start = ciPtr->baseOffset + offsetBytes;
	int len = ciPtr->numBytes - offsetBytes;
	int xDisplacement = x - chunkPtr->x;

	if ((len > 0) && (string[start + len - 1] == '\t')) {
	    len--;
	}
	if (len <= 0) {
	    return;
	}

	TkpDrawCharsInContext(display, dst, stylePtr->fgGC, sValuePtr->tkfont,
		string, numBytes, start, len,
		ciPtr->baseChunkPtr->x + xDisplacement,
		y + baseline - sValuePtr->offset);

	if (sValuePtr->underline) {
	    TkUnderlineCharsInContext(display, dst, stylePtr->ulGC,
		    sValuePtr->tkfont, string, numBytes,
		    ciPtr->baseChunkPtr->x + xDisplacement,
		    y + baseline - sValuePtr->offset,
		    start, start+len);
	}
	if (sValuePtr->overstrike) {
	    Tk_FontMetrics fm;

	    Tk_GetFontMetrics(sValuePtr->tkfont, &fm);
	    TkUnderlineCharsInContext(display, dst, stylePtr->ovGC,
		    sValuePtr->tkfont, string, numBytes,
		    ciPtr->baseChunkPtr->x + xDisplacement,
		    y + baseline - sValuePtr->offset
			    - fm.descent - (fm.ascent * 3) / 10,
		    start, start+len);
	}
#else /* !TK_DRAW_IN_CONTEXT */
	string += offsetBytes;
	numBytes -= offsetBytes;

	if ((numBytes > 0) && (string[numBytes - 1] == '\t')) {
	    numBytes--;
	}
	Tk_DrawChars(display, dst, stylePtr->fgGC, sValuePtr->tkfont, string,
		numBytes, offsetX, y + baseline - sValuePtr->offset);
	if (sValuePtr->underline) {
	    Tk_UnderlineChars(display, dst, stylePtr->ulGC, sValuePtr->tkfont,
		    string, offsetX,
		    y + baseline - sValuePtr->offset,
		    0, numBytes);

	}
	if (sValuePtr->overstrike) {
	    Tk_FontMetrics fm;

	    Tk_GetFontMetrics(sValuePtr->tkfont, &fm);
	    Tk_UnderlineChars(display, dst, stylePtr->ovGC, sValuePtr->tkfont,
		    string, offsetX,
		    y + baseline - sValuePtr->offset
			    - fm.descent - (fm.ascent * 3) / 10,
		    0, numBytes);
	}
#endif /* TK_DRAW_IN_CONTEXT */
    }
}

/*
 *--------------------------------------------------------------
 *
 * CharUndisplayProc --
 *
 *	This function is called when a character chunk is no longer going to
 *	be displayed. It frees up resources that were allocated to display the
 *	chunk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and other resources get freed.
 *
 *--------------------------------------------------------------
 */

static void
CharUndisplayProc(
    TkText *textPtr,		/* Overall information about text widget. */
    TkTextDispChunk *chunkPtr)	/* Chunk that is about to be freed. */
{
    CharInfo *ciPtr = chunkPtr->clientData;

    if (ciPtr) {
#if TK_LAYOUT_WITH_BASE_CHUNKS
	if (chunkPtr == ciPtr->baseChunkPtr) {
	    /*
	     * Basechunks are undisplayed first, when DLines are freed or
	     * partially freed, so this makes sure we don't access their data
	     * any more.
	     */

	    FreeBaseChunk(chunkPtr);
	} else if (ciPtr->baseChunkPtr != NULL) {
	    /*
	     * When other char chunks are undisplayed, drop their characters
	     * from the base chunk. This usually happens, when they are last
	     * in a line and need to be re-layed out.
	     */

	    RemoveFromBaseChunk(chunkPtr);
	}

	ciPtr->baseChunkPtr = NULL;
	ciPtr->chars = NULL;
	ciPtr->numBytes = 0;
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */

	ckfree(ciPtr);
	chunkPtr->clientData = NULL;
    }
}

/*
 *--------------------------------------------------------------
 *
 * CharMeasureProc --
 *
 *	This function is called to determine which character in a character
 *	chunk lies over a given x-coordinate.
 *
 * Results:
 *	The return value is the index *within the chunk* of the character that
 *	covers the position given by "x".
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
CharMeasureProc(
    TkTextDispChunk *chunkPtr,	/* Chunk containing desired coord. */
    int x)			/* X-coordinate, in same coordinate system as
				 * chunkPtr->x. */
{
    int endX;

    return CharChunkMeasureChars(chunkPtr, NULL, 0, 0, chunkPtr->numBytes-1,
	    chunkPtr->x, x, 0, &endX); /* CHAR OFFSET */
}

/*
 *--------------------------------------------------------------
 *
 * CharBboxProc --
 *
 *	This function is called to compute the bounding box of the area
 *	occupied by a single character.
 *
 * Results:
 *	There is no return value. *xPtr and *yPtr are filled in with the
 *	coordinates of the upper left corner of the character, and *widthPtr
 *	and *heightPtr are filled in with the dimensions of the character in
 *	pixels. Note: not all of the returned bbox is necessarily visible on
 *	the screen (the rightmost part might be off-screen to the right, and
 *	the bottommost part might be off-screen to the bottom).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
CharBboxProc(
    TkText *textPtr,
    TkTextDispChunk *chunkPtr,	/* Chunk containing desired char. */
    int byteIndex,		/* Byte offset of desired character within the
				 * chunk. */
    int y,			/* Topmost pixel in area allocated for this
				 * line. */
    int lineHeight,		/* Height of line, in pixels. */
    int baseline,		/* Location of line's baseline, in pixels
				 * measured down from y. */
    int *xPtr, int *yPtr,	/* Gets filled in with coords of character's
				 * upper-left pixel. X-coord is in same
				 * coordinate system as chunkPtr->x. */
    int *widthPtr,		/* Gets filled in with width of character, in
				 * pixels. */
    int *heightPtr)		/* Gets filled in with height of character, in
				 * pixels. */
{
    CharInfo *ciPtr = chunkPtr->clientData;
    int maxX;

    maxX = chunkPtr->width + chunkPtr->x;
    CharChunkMeasureChars(chunkPtr, NULL, 0, 0, byteIndex,
	    chunkPtr->x, -1, 0, xPtr);

    if (byteIndex == ciPtr->numBytes) {
	/*
	 * This situation only happens if the last character in a line is a
	 * space character, in which case it absorbs all of the extra space in
	 * the line (see TkTextCharLayoutProc).
	 */

	*widthPtr = maxX - *xPtr;
    } else if ((ciPtr->chars[byteIndex] == '\t')
	    && (byteIndex == ciPtr->numBytes - 1)) {
	/*
	 * The desired character is a tab character that terminates a chunk;
	 * give it all the space left in the chunk.
	 */

	*widthPtr = maxX - *xPtr;
    } else {
	CharChunkMeasureChars(chunkPtr, NULL, 0, byteIndex, byteIndex+1,
		*xPtr, -1, 0, widthPtr);
	if (*widthPtr > maxX) {
	    *widthPtr = maxX - *xPtr;
	} else {
	    *widthPtr -= *xPtr;
	}
    }
    *yPtr = y + baseline - chunkPtr->minAscent;
    *heightPtr = chunkPtr->minAscent + chunkPtr->minDescent;
}

/*
 *----------------------------------------------------------------------
 *
 * AdjustForTab --
 *
 *	This function is called to move a series of chunks right in order to
 *	align them with a tab stop.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The width of chunkPtr gets adjusted so that it absorbs the extra space
 *	due to the tab. The x locations in all the chunks after chunkPtr are
 *	adjusted rightward to align with the tab stop given by tabArrayPtr and
 *	index.
 *
 *----------------------------------------------------------------------
 */

static void
AdjustForTab(
    TkText *textPtr,		/* Information about the text widget as a
				 * whole. */
    TkTextTabArray *tabArrayPtr,/* Information about the tab stops that apply
				 * to this line. May be NULL to indicate
				 * default tabbing (every 8 chars). */
    int index,			/* Index of current tab stop. */
    TkTextDispChunk *chunkPtr)	/* Chunk whose last character is the tab; the
				 * following chunks contain information to be
				 * shifted right. */
{
    int x, desired, delta, width, decimal, i, gotDigit;
    TkTextDispChunk *chunkPtr2, *decimalChunkPtr;
    CharInfo *ciPtr;
    int tabX, spaceWidth;
    const char *p;
    TkTextTabAlign alignment;

    if (chunkPtr->nextPtr == NULL) {
	/*
	 * Nothing after the actual tab; just return.
	 */

	return;
    }

    x = chunkPtr->nextPtr->x;

    /*
     * If no tab information has been given, assuming tab stops are at 8
     * average-sized characters. Still ensure we respect the tabular versus
     * wordprocessor tab style.
     */

    if ((tabArrayPtr == NULL) || (tabArrayPtr->numTabs == 0)) {
	/*
	 * No tab information has been given, so use the default
	 * interpretation of tabs.
	 */

	if (textPtr->tabStyle == TK_TEXT_TABSTYLE_TABULAR) {
	    int tabWidth = Tk_TextWidth(textPtr->tkfont, "0", 1) * 8;
	    if (tabWidth == 0) {
		tabWidth = 1;
	    }

	    desired = tabWidth * (index + 1);
	} else {
	    desired = NextTabStop(textPtr->tkfont, x, 0);
	}

	goto update;
    }

    if (index < tabArrayPtr->numTabs) {
	alignment = tabArrayPtr->tabs[index].alignment;
	tabX = tabArrayPtr->tabs[index].location;
    } else {
	/*
	 * Ran out of tab stops; compute a tab position by extrapolating from
	 * the last two tab positions.
	 */

	tabX = (int) (tabArrayPtr->lastTab +
		(index + 1 - tabArrayPtr->numTabs)*tabArrayPtr->tabIncrement +
		0.5);
	alignment = tabArrayPtr->tabs[tabArrayPtr->numTabs-1].alignment;
    }

    if (alignment == LEFT) {
	desired = tabX;
	goto update;
    }

    if ((alignment == CENTER) || (alignment == RIGHT)) {
	/*
	 * Compute the width of all the information in the tab group, then use
	 * it to pick a desired location.
	 */

	width = 0;
	for (chunkPtr2 = chunkPtr->nextPtr; chunkPtr2 != NULL;
		chunkPtr2 = chunkPtr2->nextPtr) {
	    width += chunkPtr2->width;
	}
	if (alignment == CENTER) {
	    desired = tabX - width/2;
	} else {
	    desired = tabX - width;
	}
	goto update;
    }

    /*
     * Must be numeric alignment. Search through the text to be tabbed,
     * looking for the last , or . before the first character that isn't a
     * number, comma, period, or sign.
     */

    decimalChunkPtr = NULL;
    decimal = gotDigit = 0;
    for (chunkPtr2 = chunkPtr->nextPtr; chunkPtr2 != NULL;
	    chunkPtr2 = chunkPtr2->nextPtr) {
	if (chunkPtr2->displayProc != CharDisplayProc) {
	    continue;
	}
	ciPtr = chunkPtr2->clientData;
	for (p = ciPtr->chars, i = 0; i < ciPtr->numBytes; p++, i++) {
	    if (isdigit(UCHAR(*p))) {
		gotDigit = 1;
	    } else if ((*p == '.') || (*p == ',')) {
		decimal = p-ciPtr->chars;
		decimalChunkPtr = chunkPtr2;
	    } else if (gotDigit) {
		if (decimalChunkPtr == NULL) {
		    decimal = p-ciPtr->chars;
		    decimalChunkPtr = chunkPtr2;
		}
		goto endOfNumber;
	    }
	}
    }

  endOfNumber:
    if (decimalChunkPtr != NULL) {
	int curX;

	ciPtr = decimalChunkPtr->clientData;
	CharChunkMeasureChars(decimalChunkPtr, NULL, 0, 0, decimal,
		decimalChunkPtr->x, -1, 0, &curX);
	desired = tabX - (curX - x);
	goto update;
    }

    /*
     * There wasn't a decimal point. Right justify the text.
     */

    width = 0;
    for (chunkPtr2 = chunkPtr->nextPtr; chunkPtr2 != NULL;
	    chunkPtr2 = chunkPtr2->nextPtr) {
	width += chunkPtr2->width;
    }
    desired = tabX - width;

    /*
     * Shift all of the chunks to the right so that the left edge is at the
     * desired location, then expand the chunk containing the tab. Be sure
     * that the tab occupies at least the width of a space character.
     */

  update:
    delta = desired - x;
    MeasureChars(textPtr->tkfont, " ", 1, 0, 1, 0, -1, 0, &spaceWidth);
    if (delta < spaceWidth) {
	delta = spaceWidth;
    }
    for (chunkPtr2 = chunkPtr->nextPtr; chunkPtr2 != NULL;
	    chunkPtr2 = chunkPtr2->nextPtr) {
	chunkPtr2->x += delta;
    }
    chunkPtr->width += delta;
}

/*
 *----------------------------------------------------------------------
 *
 * SizeOfTab --
 *
 *	This returns an estimate of the amount of white space that will be
 *	consumed by a tab.
 *
 * Results:
 *	The return value is the minimum number of pixels that will be occupied
 *	by the next tab of tabArrayPtr, assuming that the current position on
 *	the line is x and the end of the line is maxX. The 'next tab' is
 *	determined by a combination of the current position (x) which it must
 *	be equal to or beyond, and the tab count in indexPtr.
 *
 *	For numeric tabs, this is a conservative estimate. The return value is
 *	always >= 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SizeOfTab(
    TkText *textPtr,		/* Information about the text widget as a
				 * whole. */
    int tabStyle,		/* One of TK_TEXT_TABSTYLE_TABULAR
				 * or TK_TEXT_TABSTYLE_WORDPROCESSOR. */
    TkTextTabArray *tabArrayPtr,/* Information about the tab stops that apply
				 * to this line. NULL means use default
				 * tabbing (every 8 chars.) */
    int *indexPtr,		/* Contains index of previous tab stop, will
				 * be updated to reflect the number of stops
				 * used. */
    int x,			/* Current x-location in line. */
    int maxX)			/* X-location of pixel just past the right
				 * edge of the line. */
{
    int tabX, result, index, spaceWidth, tabWidth;
    TkTextTabAlign alignment;

    index = *indexPtr;

    if ((tabArrayPtr == NULL) || (tabArrayPtr->numTabs == 0)) {
	/*
	 * We're using a default tab spacing of 8 characters.
	 */

	tabWidth = Tk_TextWidth(textPtr->tkfont, "0", 1) * 8;
	if (tabWidth == 0) {
	    tabWidth = 1;
	}
    } else {
	tabWidth = 0;		/* Avoid compiler error. */
    }

    do {
	/*
	 * We were given the count before this tab, so increment it first.
	 */

	index++;

	if ((tabArrayPtr == NULL) || (tabArrayPtr->numTabs == 0)) {
	    /*
	     * We're using a default tab spacing calculated above.
	     */

	    tabX = tabWidth * (index + 1);
	    alignment = LEFT;
	} else if (index < tabArrayPtr->numTabs) {
	    tabX = tabArrayPtr->tabs[index].location;
	    alignment = tabArrayPtr->tabs[index].alignment;
	} else {
	    /*
	     * Ran out of tab stops; compute a tab position by extrapolating.
	     */

	    tabX = (int) (tabArrayPtr->lastTab
		    + (index + 1 - tabArrayPtr->numTabs)
		    * tabArrayPtr->tabIncrement + 0.5);
	    alignment = tabArrayPtr->tabs[tabArrayPtr->numTabs-1].alignment;
	}

	/*
	 * If this tab stop is before the current x position, then we have two
	 * cases:
	 *
	 * With 'wordprocessor' style tabs, we must obviously continue until
	 * we reach the text tab stop.
	 *
	 * With 'tabular' style tabs, we always use the index'th tab stop.
	 */
    } while (tabX <= x && (tabStyle == TK_TEXT_TABSTYLE_WORDPROCESSOR));

    /*
     * Inform our caller of how many tab stops we've used up.
     */

    *indexPtr = index;

    if (alignment == CENTER) {
	/*
	 * Be very careful in the arithmetic below, because maxX may be the
	 * largest positive number: watch out for integer overflow.
	 */

	if ((maxX-tabX) < (tabX - x)) {
	    result = (maxX - x) - 2*(maxX - tabX);
	} else {
	    result = 0;
	}
	goto done;
    }
    if (alignment == RIGHT) {
	result = 0;
	goto done;
    }

    /*
     * Note: this treats NUMERIC alignment the same as LEFT alignment, which
     * is somewhat conservative. However, it's pretty tricky at this point to
     * figure out exactly where the damn decimal point will be.
     */

    if (tabX > x) {
	result = tabX - x;
    } else {
	result = 0;
    }

  done:
    MeasureChars(textPtr->tkfont, " ", 1, 0, 1, 0, -1, 0, &spaceWidth);
    if (result < spaceWidth) {
	result = spaceWidth;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTabStop --
 *
 *	Given the current position, determine where the next default tab stop
 *	would be located. This function is called when the current chunk in
 *	the text has no tabs defined and so the default tab spacing for the
 *	font should be used, provided we are using wordprocessor style tabs.
 *
 * Results:
 *	The location in pixels of the next tab stop.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
NextTabStop(
    Tk_Font tkfont,		/* Font in which chunk that contains tab stop
				 * will be drawn. */
    int x,			/* X-position in pixels where last character
				 * was drawn. The next tab stop occurs
				 * somewhere after this location. */
    int tabOrigin)		/* The origin for tab stops. May be non-zero
				 * if text has been scrolled. */
{
    int tabWidth, rem;

    tabWidth = Tk_TextWidth(tkfont, "0", 1) * 8;
    if (tabWidth == 0) {
	tabWidth = 1;
    }

    x += tabWidth;
    rem = (x - tabOrigin) % tabWidth;
    if (rem < 0) {
	rem += tabWidth;
    }
    x -= rem;
    return x;
}

/*
 *---------------------------------------------------------------------------
 *
 * MeasureChars --
 *
 *	Determine the number of characters from the string that will fit in
 *	the given horizontal span. The measurement is done under the
 *	assumption that Tk_DrawChars will be used to actually display the
 *	characters.
 *
 *	If tabs are encountered in the string, they will be ignored (they
 *	should only occur as last character of the string anyway).
 *
 *	If a newline is encountered in the string, the line will be broken at
 *	that point.
 *
 * Results:
 *	The return value is the number of bytes from the range of start to end
 *	in source that fit in the span given by startX and maxX. *nextXPtr is
 *	filled in with the x-coordinate at which the first character that
 *	didn't fit would be drawn, if it were to be drawn.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
MeasureChars(
    Tk_Font tkfont,		/* Font in which to draw characters. */
    const char *source,		/* Characters to be displayed. Need not be
				 * NULL-terminated. */
    int maxBytes,		/* Maximum # of bytes to consider from
				 * source. */
    int rangeStart, int rangeLength,
				/* Range of bytes to consider in source.*/
    int startX,			/* X-position at which first character will be
				 * drawn. */
    int maxX,			/* Don't consider any character that would
				 * cross this x-position. */
    int flags,			/* Flags to pass to Tk_MeasureChars. */
    int *nextXPtr)		/* Return x-position of terminating character
				 * here. */
{
    int curX, width, ch;
    const char *special, *end, *start;

    ch = 0;			/* lint. */
    curX = startX;
    start = source + rangeStart;
    end = start + rangeLength;
    special = start;
    while (start < end) {
	if (start >= special) {
	    /*
	     * Find the next special character in the string.
	     */

	    for (special = start; special < end; special++) {
		ch = *special;
		if ((ch == '\t') || (ch == '\n')) {
		    break;
		}
	    }
	}

	/*
	 * Special points at the next special character (or the end of the
	 * string). Process characters between start and special.
	 */

	if ((maxX >= 0) && (curX >= maxX)) {
	    break;
	}
#if TK_DRAW_IN_CONTEXT
	start += TkpMeasureCharsInContext(tkfont, source, maxBytes,
		start - source, special - start,
		maxX >= 0 ? maxX - curX : -1, flags, &width);
#else
	(void) maxBytes;
	start += Tk_MeasureChars(tkfont, start, special - start,
		maxX >= 0 ? maxX - curX : -1, flags, &width);
#endif /* TK_DRAW_IN_CONTEXT */
	curX += width;
	if (start < special) {
	    /*
	     * No more chars fit in line.
	     */

	    break;
	}
	if (special < end) {
	    if (ch != '\t') {
		break;
	    }
	    start++;
	}
    }

    *nextXPtr = curX;
    return start - (source+rangeStart);
}

/*
 *----------------------------------------------------------------------
 *
 * TextGetScrollInfoObj --
 *
 *	This function is invoked to parse "xview" and "yview" scrolling
 *	commands for text widgets using the new scrolling command syntax
 *	("moveto" or "scroll" options). It extends the public
 *	Tk_GetScrollInfoObj function with the addition of "pixels" as a valid
 *	unit alongside "pages" and "units". It is a shame the core API isn't
 *	more flexible in this regard.
 *
 * Results:
 *	The return value is either TKTEXT_SCROLL_MOVETO, TKTEXT_SCROLL_PAGES,
 *	TKTEXT_SCROLL_UNITS, TKTEXT_SCROLL_PIXELS or TKTEXT_SCROLL_ERROR. This
 *	indicates whether the command was successfully parsed and what form
 *	the command took. If TKTEXT_SCROLL_MOVETO, *dblPtr is filled in with
 *	the desired position; if TKTEXT_SCROLL_PAGES, TKTEXT_SCROLL_PIXELS or
 *	TKTEXT_SCROLL_UNITS, *intPtr is filled in with the number of
 *	pages/pixels/lines to move (may be negative); if TKTEXT_SCROLL_ERROR,
 *	the interp's result contains an error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TextGetScrollInfoObj(
    Tcl_Interp *interp,		/* Used for error reporting. */
    TkText *textPtr,		/* Information about the text widget. */
    int objc,			/* # arguments for command. */
    Tcl_Obj *const objv[],	/* Arguments for command. */
    double *dblPtr,		/* Filled in with argument "moveto" option, if
				 * any. */
    int *intPtr)		/* Filled in with number of pages or lines or
				 * pixels to scroll, if any. */
{
    static const char *const subcommands[] = {
	"moveto", "scroll", NULL
    };
    enum viewSubcmds {
	VIEW_MOVETO, VIEW_SCROLL
    };
    static const char *const units[] = {
	"units", "pages", "pixels", NULL
    };
    enum viewUnits {
	VIEW_SCROLL_UNITS, VIEW_SCROLL_PAGES, VIEW_SCROLL_PIXELS
    };
    int index;

    if (Tcl_GetIndexFromObjStruct(interp, objv[2], subcommands,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TKTEXT_SCROLL_ERROR;
    }

    switch ((enum viewSubcmds) index) {
    case VIEW_MOVETO:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "fraction");
	    return TKTEXT_SCROLL_ERROR;
	}
	if (Tcl_GetDoubleFromObj(interp, objv[3], dblPtr) != TCL_OK) {
	    return TKTEXT_SCROLL_ERROR;
	}
	return TKTEXT_SCROLL_MOVETO;
    case VIEW_SCROLL:
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 3, objv, "number units|pages|pixels");
	    return TKTEXT_SCROLL_ERROR;
	}
	if (Tcl_GetIndexFromObjStruct(interp, objv[4], units,
		sizeof(char *), "argument", 0, &index) != TCL_OK) {
	    return TKTEXT_SCROLL_ERROR;
	}
	switch ((enum viewUnits) index) {
	case VIEW_SCROLL_PAGES:
	    if (Tcl_GetIntFromObj(interp, objv[3], intPtr) != TCL_OK) {
		return TKTEXT_SCROLL_ERROR;
	    }
	    return TKTEXT_SCROLL_PAGES;
	case VIEW_SCROLL_PIXELS:
	    if (Tk_GetPixelsFromObj(interp, textPtr->tkwin, objv[3],
		    intPtr) != TCL_OK) {
		return TKTEXT_SCROLL_ERROR;
	    }
	    return TKTEXT_SCROLL_PIXELS;
	case VIEW_SCROLL_UNITS:
	    if (Tcl_GetIntFromObj(interp, objv[3], intPtr) != TCL_OK) {
		return TKTEXT_SCROLL_ERROR;
	    }
	    return TKTEXT_SCROLL_UNITS;
	}
    }
    Tcl_Panic("unexpected switch fallthrough");
    return TKTEXT_SCROLL_ERROR;
}

#if TK_LAYOUT_WITH_BASE_CHUNKS
/*
 *----------------------------------------------------------------------
 *
 * FinalizeBaseChunk --
 *
 *	This procedure makes sure that all the chunks of the stretch are
 *	up-to-date. It is invoked when the LayoutProc has been called for all
 *	chunks and the base chunk is stable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The CharInfo.chars of all dependent chunks point into
 *	BaseCharInfo.baseChars for easy access (and compatibility).
 *
 *----------------------------------------------------------------------
 */

static void
FinalizeBaseChunk(
    TkTextDispChunk *addChunkPtr)
				/* An additional chunk to add to the stretch,
				 * even though it may not be in the linked
				 * list yet. Used by the LayoutProc, otherwise
				 * NULL. */
{
    const char *baseChars;
    TkTextDispChunk *chunkPtr;
    CharInfo *ciPtr;
#if TK_DRAW_IN_CONTEXT
    int widthAdjust = 0;
    int newwidth;
#endif /* TK_DRAW_IN_CONTEXT */

    if (baseCharChunkPtr == NULL) {
	return;
    }

    baseChars = Tcl_DStringValue(
	    &((BaseCharInfo *) baseCharChunkPtr->clientData)->baseChars);

    for (chunkPtr = baseCharChunkPtr; chunkPtr != NULL;
	    chunkPtr = chunkPtr->nextPtr) {
#if TK_DRAW_IN_CONTEXT
	chunkPtr->x += widthAdjust;
#endif /* TK_DRAW_IN_CONTEXT */

	if (chunkPtr->displayProc != CharDisplayProc) {
	    continue;
	}
	ciPtr = chunkPtr->clientData;
	if (ciPtr->baseChunkPtr != baseCharChunkPtr) {
	    break;
	}
	ciPtr->chars = baseChars + ciPtr->baseOffset;

#if TK_DRAW_IN_CONTEXT
	newwidth = 0;
	CharChunkMeasureChars(chunkPtr, NULL, 0, 0, -1, 0, -1, 0, &newwidth);
	if (newwidth < chunkPtr->width) {
	    widthAdjust += newwidth - chunkPtr->width;
	    chunkPtr->width = newwidth;
	}
#endif /* TK_DRAW_IN_CONTEXT */
    }

    if (addChunkPtr != NULL) {
	ciPtr = addChunkPtr->clientData;
	ciPtr->chars = baseChars + ciPtr->baseOffset;

#if TK_DRAW_IN_CONTEXT
	addChunkPtr->x += widthAdjust;
	CharChunkMeasureChars(addChunkPtr, NULL, 0, 0, -1, 0, -1, 0,
		&addChunkPtr->width);
#endif /* TK_DRAW_IN_CONTEXT */
    }

    baseCharChunkPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeBaseChunk --
 *
 *	This procedure makes sure that all the chunks of the stretch are
 *	disconnected from the base chunk and the base chunk specific data is
 *	freed. It is invoked from the UndisplayProc. The procedure doesn't
 *	ckfree the base chunk clientData itself, that's up to the main
 *	UndisplayProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The CharInfo.chars of all dependent chunks are set to NULL. Memory
 *	that belongs specifically to the base chunk is freed.
 *
 *----------------------------------------------------------------------
 */

static void
FreeBaseChunk(
    TkTextDispChunk *baseChunkPtr)
				/* The base chunk of the stretch and head of
				 * the linked list. */
{
    TkTextDispChunk *chunkPtr;
    CharInfo *ciPtr;

    if (baseCharChunkPtr == baseChunkPtr) {
	baseCharChunkPtr = NULL;
    }

    for (chunkPtr=baseChunkPtr; chunkPtr!=NULL; chunkPtr=chunkPtr->nextPtr) {
	if (chunkPtr->undisplayProc != CharUndisplayProc) {
	    continue;
	}
	ciPtr = chunkPtr->clientData;
	if (ciPtr->baseChunkPtr != baseChunkPtr) {
	    break;
	}

	ciPtr->baseChunkPtr = NULL;
	ciPtr->chars = NULL;
    }

    if (baseChunkPtr) {
	Tcl_DStringFree(&((BaseCharInfo *) baseChunkPtr->clientData)->baseChars);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * IsSameFGStyle --
 *
 *	Compare the foreground attributes of two styles. Specifically must
 *	consider: foreground color, font, font style and font decorations,
 *	elide, "offset" and foreground stipple. Do *not* consider: background
 *	color, border, relief or background stipple.
 *
 *	If we use TkpDrawCharsInContext(), we also don't need to check
 *	foreground color, font decorations, elide, offset and foreground
 *	stipple, so all that is left is font (including font size and font
 *	style) and "offset".
 *
 * Results:
 *	1 if the two styles match, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
IsSameFGStyle(
    TextStyle *style1,
    TextStyle *style2)
{
    StyleValues *sv1;
    StyleValues *sv2;

    if (style1 == style2) {
	return 1;
    }

#if !TK_DRAW_IN_CONTEXT
    if (
#ifdef MAC_OSX_TK
	    !TkMacOSXCompareColors(style1->fgGC->foreground,
		    style2->fgGC->foreground)
#else
	    style1->fgGC->foreground != style2->fgGC->foreground
#endif
	    ) {
	return 0;
    }
#endif /* !TK_DRAW_IN_CONTEXT */

    sv1 = style1->sValuePtr;
    sv2 = style2->sValuePtr;

#if TK_DRAW_IN_CONTEXT
    return sv1->tkfont == sv2->tkfont && sv1->offset == sv2->offset;
#else
    return sv1->tkfont == sv2->tkfont
	    && sv1->underline == sv2->underline
	    && sv1->overstrike == sv2->overstrike
	    && sv1->elide == sv2->elide
	    && sv1->offset == sv2->offset
	    && sv1->fgStipple == sv1->fgStipple;
#endif /* TK_DRAW_IN_CONTEXT */
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveFromBaseChunk --
 *
 *	This procedure removes a chunk from the stretch as a result of
 *	UndisplayProc. The chunk in question should be the last in a stretch.
 *	This happens during re-layouting of the break position.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The characters that belong to this chunk are removed from the base
 *	chunk. It is assumed that LayoutProc and FinalizeBaseChunk are called
 *	next to repair any damage that this causes to the integrity of the
 *	stretch and the other chunks. For that reason the base chunk is also
 *	put into baseCharChunkPtr automatically, so that LayoutProc can resume
 *	correctly.
 *
 *----------------------------------------------------------------------
 */

static void
RemoveFromBaseChunk(
    TkTextDispChunk *chunkPtr)	/* The chunk to remove from the end of the
				 * stretch. */
{
    CharInfo *ciPtr;
    BaseCharInfo *bciPtr;

    if (chunkPtr->displayProc != CharDisplayProc) {
#ifdef DEBUG_LAYOUT_WITH_BASE_CHUNKS
	fprintf(stderr,"RemoveFromBaseChunk called with wrong chunk type\n");
#endif
	return;
    }

    /*
     * Reinstitute this base chunk for re-layout.
     */

    ciPtr = chunkPtr->clientData;
    baseCharChunkPtr = ciPtr->baseChunkPtr;

    /*
     * Remove the chunk data from the base chunk data.
     */

    bciPtr = baseCharChunkPtr->clientData;

#ifdef DEBUG_LAYOUT_WITH_BASE_CHUNKS
    if ((ciPtr->baseOffset + ciPtr->numBytes)
	    != Tcl_DStringLength(&bciPtr->baseChars)) {
	fprintf(stderr,"RemoveFromBaseChunk called with wrong chunk "
		"(not last)\n");
    }
#endif

    Tcl_DStringSetLength(&bciPtr->baseChars, ciPtr->baseOffset);

    /*
     * Invalidate the stored pixel width of the base chunk.
     */

    bciPtr->width = -1;
}
#endif /* TK_LAYOUT_WITH_BASE_CHUNKS */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
