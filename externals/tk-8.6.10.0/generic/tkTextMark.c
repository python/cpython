/*
 * tkTextMark.c --
 *
 *	This file contains the functions that implement marks for text
 *	widgets.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkText.h"
#include "tk3d.h"

/*
 * Macro that determines the size of a mark segment:
 */

#define MSEG_SIZE ((unsigned) (Tk_Offset(TkTextSegment, body) \
	+ sizeof(TkTextMark)))

/*
 * Forward references for functions defined in this file:
 */

static Tcl_Obj *	GetMarkName(TkText *textPtr, TkTextSegment *segPtr);
static void		InsertUndisplayProc(TkText *textPtr,
			    TkTextDispChunk *chunkPtr);
static int		MarkDeleteProc(TkTextSegment *segPtr,
			    TkTextLine *linePtr, int treeGone);
static TkTextSegment *	MarkCleanupProc(TkTextSegment *segPtr,
			    TkTextLine *linePtr);
static void		MarkCheckProc(TkTextSegment *segPtr,
			    TkTextLine *linePtr);
static int		MarkLayoutProc(TkText *textPtr, TkTextIndex *indexPtr,
			    TkTextSegment *segPtr, int offset, int maxX,
			    int maxChars, int noCharsYet, TkWrapMode wrapMode,
			    TkTextDispChunk *chunkPtr);
static int		MarkFindNext(Tcl_Interp *interp,
			    TkText *textPtr, Tcl_Obj *markName);
static int		MarkFindPrev(Tcl_Interp *interp,
			    TkText *textPtr, Tcl_Obj *markName);


/*
 * The following structures declare the "mark" segment types. There are
 * actually two types for marks, one with left gravity and one with right
 * gravity. They are identical except for their gravity property.
 */

const Tk_SegType tkTextRightMarkType = {
    "mark",			/* name */
    0,				/* leftGravity */
    NULL,			/* splitProc */
    MarkDeleteProc,		/* deleteProc */
    MarkCleanupProc,		/* cleanupProc */
    NULL,			/* lineChangeProc */
    MarkLayoutProc,		/* layoutProc */
    MarkCheckProc		/* checkProc */
};

const Tk_SegType tkTextLeftMarkType = {
    "mark",			/* name */
    1,				/* leftGravity */
    NULL,			/* splitProc */
    MarkDeleteProc,		/* deleteProc */
    MarkCleanupProc,		/* cleanupProc */
    NULL,			/* lineChangeProc */
    MarkLayoutProc,		/* layoutProc */
    MarkCheckProc		/* checkProc */
};

/*
 *--------------------------------------------------------------
 *
 * TkTextMarkCmd --
 *
 *	This function is invoked to process the "mark" options of the widget
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
TkTextMarkCmd(
    register TkText *textPtr,	/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "mark". */
{
    Tcl_HashEntry *hPtr;
    TkTextSegment *markPtr;
    Tcl_HashSearch search;
    TkTextIndex index;
    const Tk_SegType *newTypePtr;
    int optionIndex;
    static const char *const markOptionStrings[] = {
	"gravity", "names", "next", "previous", "set", "unset", NULL
    };
    enum markOptions {
	MARK_GRAVITY, MARK_NAMES, MARK_NEXT, MARK_PREVIOUS, MARK_SET,
	MARK_UNSET
    };

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[2], markOptionStrings,
	    sizeof(char *), "mark option", 0, &optionIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum markOptions) optionIndex) {
    case MARK_GRAVITY: {
	char c;
	int length;
	const char *str;

	if (objc < 4 || objc > 5) {
	    Tcl_WrongNumArgs(interp, 3, objv, "markName ?gravity?");
	    return TCL_ERROR;
	}
	str = Tcl_GetStringFromObj(objv[3], &length);
	if (length == 6 && !strcmp(str, "insert")) {
	    markPtr = textPtr->insertMarkPtr;
	} else if (length == 7 && !strcmp(str, "current")) {
	    markPtr = textPtr->currentMarkPtr;
	} else {
	    hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->markTable, str);
	    if (hPtr == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"there is no mark named \"%s\"", str));
		Tcl_SetErrorCode(interp, "TK", "LOOKUP", "TEXT_MARK", str,
			NULL);
		return TCL_ERROR;
	    }
	    markPtr = Tcl_GetHashValue(hPtr);
	}
	if (objc == 4) {
	    const char *typeStr;

	    if (markPtr->typePtr == &tkTextRightMarkType) {
		typeStr = "right";
	    } else {
		typeStr = "left";
	    }
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(typeStr, -1));
	    return TCL_OK;
	}
	str = Tcl_GetStringFromObj(objv[4],&length);
	c = str[0];
	if ((c == 'l') && (strncmp(str, "left", (unsigned) length) == 0)) {
	    newTypePtr = &tkTextLeftMarkType;
	} else if ((c == 'r') &&
		(strncmp(str, "right", (unsigned) length) == 0)) {
	    newTypePtr = &tkTextRightMarkType;
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad mark gravity \"%s\": must be left or right", str));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "MARK_GRAVITY", NULL);
	    return TCL_ERROR;
	}
	TkTextMarkSegToIndex(textPtr, markPtr, &index);
	TkBTreeUnlinkSegment(markPtr, markPtr->body.mark.linePtr);
	markPtr->typePtr = newTypePtr;
	TkBTreeLinkSegment(markPtr, &index);
	break;
    }
    case MARK_NAMES: {
	Tcl_Obj *resultObj;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 3, objv, NULL);
	    return TCL_ERROR;
	}
	resultObj = Tcl_NewObj();
	Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		"insert", -1));
	Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		"current", -1));
	for (hPtr = Tcl_FirstHashEntry(&textPtr->sharedTextPtr->markTable,
		&search); hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
		    Tcl_GetHashKey(&textPtr->sharedTextPtr->markTable, hPtr),
		    -1));
	}
	Tcl_SetObjResult(interp, resultObj);
	break;
    }
    case MARK_NEXT:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index");
	    return TCL_ERROR;
	}
	return MarkFindNext(interp, textPtr, objv[3]);
    case MARK_PREVIOUS:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "index");
	    return TCL_ERROR;
	}
	return MarkFindPrev(interp, textPtr, objv[3]);
    case MARK_SET:
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 3, objv, "markName index");
	    return TCL_ERROR;
	}
	if (TkTextGetObjIndex(interp, textPtr, objv[4], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	TkTextSetMark(textPtr, Tcl_GetString(objv[3]), &index);
	return TCL_OK;
    case MARK_UNSET: {
	int i;

	for (i = 3; i < objc; i++) {
	    hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->markTable,
		    Tcl_GetString(objv[i]));
	    if (hPtr != NULL) {
		markPtr = Tcl_GetHashValue(hPtr);

		/*
		 * Special case not needed with peer widgets.
		 */

		if ((markPtr == textPtr->insertMarkPtr)
			|| (markPtr == textPtr->currentMarkPtr)) {
		    continue;
		}
		TkBTreeUnlinkSegment(markPtr, markPtr->body.mark.linePtr);
		Tcl_DeleteHashEntry(hPtr);
		ckfree(markPtr);
	    }
	}
	break;
    }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextSetMark --
 *
 *	Set a mark to a particular position, creating a new mark if one
 *	doesn't already exist.
 *
 * Results:
 *	The return value is a pointer to the mark that was just set.
 *
 * Side effects:
 *	A new mark is created, or an existing mark is moved.
 *
 *----------------------------------------------------------------------
 */

TkTextSegment *
TkTextSetMark(
    TkText *textPtr,		/* Text widget in which to create mark. */
    const char *name,		/* Name of mark to set. */
    TkTextIndex *indexPtr)	/* Where to set mark. */
{
    Tcl_HashEntry *hPtr = NULL;
    TkTextSegment *markPtr;
    TkTextIndex insertIndex;
    int isNew, widgetSpecific;

    if (!strcmp(name, "insert")) {
	widgetSpecific = 1;
	markPtr = textPtr->insertMarkPtr;
	isNew = (markPtr == NULL ? 1 : 0);
    } else if (!strcmp(name, "current")) {
	widgetSpecific = 2;
	markPtr = textPtr->currentMarkPtr;
	isNew = (markPtr == NULL ? 1 : 0);
    } else {
	widgetSpecific = 0;
	hPtr = Tcl_CreateHashEntry(&textPtr->sharedTextPtr->markTable, name,
		&isNew);
	markPtr = Tcl_GetHashValue(hPtr);
    }
    if (!isNew) {
	/*
	 * If this is the insertion point that's being moved, be sure to force
	 * a display update at the old position. Also, don't let the insertion
	 * cursor be after the final newline of the file.
	 */

	if (markPtr == textPtr->insertMarkPtr) {
	    TkTextIndex index, index2;
            int nblines;

	    TkTextMarkSegToIndex(textPtr, textPtr->insertMarkPtr, &index);
	    TkTextIndexForwChars(NULL, &index, 1, &index2, COUNT_INDICES);

	    /*
	     * While we wish to redisplay, no heights have changed, so no need
	     * to call TkTextInvalidateLineMetrics.
	     */

	    TkTextChanged(NULL, textPtr, &index, &index2);

            /*
             * The number of lines in the widget is zero if and only if it is
             * a partial peer with -startline == -endline, i.e. an empty
             * peer. In this case the mark shall be set exactly at the given
             * index, and not one character backwards (bug 3487407).
             */

	    nblines = TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr);
	    if ((TkBTreeLinesTo(textPtr, indexPtr->linePtr) == nblines)
		    && (nblines > 0))  {
		TkTextIndexBackChars(NULL,indexPtr, 1, &insertIndex,
			COUNT_INDICES);
		indexPtr = &insertIndex;
	    }
	}
	TkBTreeUnlinkSegment(markPtr, markPtr->body.mark.linePtr);
    } else {
	markPtr = ckalloc(MSEG_SIZE);
	markPtr->typePtr = &tkTextRightMarkType;
	markPtr->size = 0;
	markPtr->body.mark.textPtr = textPtr;
	markPtr->body.mark.linePtr = indexPtr->linePtr;
	markPtr->body.mark.hPtr = hPtr;
	if (widgetSpecific == 0) {
	    Tcl_SetHashValue(hPtr, markPtr);
	} else if (widgetSpecific == 1) {
	    textPtr->insertMarkPtr = markPtr;
	} else {
	    textPtr->currentMarkPtr = markPtr;
	}
    }
    TkBTreeLinkSegment(markPtr, indexPtr);

    /*
     * If the mark is the insertion cursor, then update the screen at the
     * mark's new location.
     */

    if (markPtr == textPtr->insertMarkPtr) {
	TkTextIndex index2;

	TkTextIndexForwChars(NULL, indexPtr, 1, &index2, COUNT_INDICES);

	/*
	 * While we wish to redisplay, no heights have changed, so no need to
	 * call TkTextInvalidateLineMetrics
	 */

	TkTextChanged(NULL, textPtr, indexPtr, &index2);
    }
    return markPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextMarkSegToIndex --
 *
 *	Given a segment that is a mark, create an index that refers to the
 *	next text character (or other text segment with non-zero size) after
 *	the mark.
 *
 * Results:
 *	*IndexPtr is filled in with index information.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkTextMarkSegToIndex(
    TkText *textPtr,		/* Text widget containing mark. */
    TkTextSegment *markPtr,	/* Mark segment. */
    TkTextIndex *indexPtr)	/* Index information gets stored here.  */
{
    TkTextSegment *segPtr;

    indexPtr->tree = textPtr->sharedTextPtr->tree;
    indexPtr->linePtr = markPtr->body.mark.linePtr;
    indexPtr->byteIndex = 0;
    for (segPtr = indexPtr->linePtr->segPtr; segPtr != markPtr;
	    segPtr = segPtr->nextPtr) {
	indexPtr->byteIndex += segPtr->size;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkTextMarkNameToIndex --
 *
 *	Given the name of a mark, return an index corresponding to the mark
 *	name.
 *
 * Results:
 *	The return value is TCL_OK if "name" exists as a mark in the text
 *	widget and is located within its -starline/-endline range. In this
 *	case *indexPtr is filled in with the next segment who is after the
 *	mark whose size is non-zero. TCL_ERROR is returned if the mark
 *	doesn't exist in the text widget, or if it is out of its -starline/
 *	-endline range. In this latter case *indexPtr still contains valid
 *	information, in particular TkTextMarkNameToIndex called with the
 *	"insert" or "current" mark name may return TCL_ERROR, but *indexPtr
 *	contains the correct index of this mark before -startline or after
 *	-endline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkTextMarkNameToIndex(
    TkText *textPtr,		/* Text widget containing mark. */
    const char *name,		/* Name of mark. */
    TkTextIndex *indexPtr)	/* Index information gets stored here. */
{
    TkTextSegment *segPtr;
    TkTextIndex index;
    int start, end;

    if (textPtr == NULL) {
        return TCL_ERROR;
    }

    if (!strcmp(name, "insert")) {
	segPtr = textPtr->insertMarkPtr;
    } else if (!strcmp(name, "current")) {
	segPtr = textPtr->currentMarkPtr;
    } else {
	Tcl_HashEntry *hPtr =
		Tcl_FindHashEntry(&textPtr->sharedTextPtr->markTable, name);

	if (hPtr == NULL) {
	    return TCL_ERROR;
	}
	segPtr = Tcl_GetHashValue(hPtr);
    }
    TkTextMarkSegToIndex(textPtr, segPtr, indexPtr);

    /* If indexPtr refers to somewhere outside the -startline/-endline
     * range limits of the widget, error out since the mark indeed is not
     * reachable from this text widget (it may be reachable from a peer)
     * (bug 1630271).
     */

    if (textPtr->start != NULL) {
	start = TkBTreeLinesTo(NULL, textPtr->start);
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, NULL, start, 0,
		&index);
	if (TkTextIndexCmp(indexPtr, &index) < 0) {
	    return TCL_ERROR;
	}
    }
    if (textPtr->end != NULL) {
	end = TkBTreeLinesTo(NULL, textPtr->end);
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, NULL, end, 0,
		&index);
	if (TkTextIndexCmp(indexPtr, &index) > 0) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * MarkDeleteProc --
 *
 *	This function is invoked by the text B-tree code whenever a mark lies
 *	in a range of characters being deleted.
 *
 * Results:
 *	Returns 1 to indicate that deletion has been rejected.
 *
 * Side effects:
 *	None (even if the whole tree is being deleted we don't free up the
 *	mark; it will be done elsewhere).
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
MarkDeleteProc(
    TkTextSegment *segPtr,	/* Segment being deleted. */
    TkTextLine *linePtr,	/* Line containing segment. */
    int treeGone)		/* Non-zero means the entire tree is being
				 * deleted, so everything must get cleaned
				 * up. */
{
    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * MarkCleanupProc --
 *
 *	This function is invoked by the B-tree code whenever a mark segment is
 *	moved from one line to another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The linePtr field of the segment gets updated.
 *
 *--------------------------------------------------------------
 */

static TkTextSegment *
MarkCleanupProc(
    TkTextSegment *markPtr,	/* Mark segment that's being moved. */
    TkTextLine *linePtr)	/* Line that now contains segment. */
{
    markPtr->body.mark.linePtr = linePtr;
    return markPtr;
}

/*
 *--------------------------------------------------------------
 *
 * MarkLayoutProc --
 *
 *	This function is the "layoutProc" for mark segments.
 *
 * Results:
 *	If the mark isn't the insertion cursor then the return value is -1 to
 *	indicate that this segment shouldn't be displayed. If the mark is the
 *	insertion character then 1 is returned and the chunkPtr structure is
 *	filled in.
 *
 * Side effects:
 *	None, except for filling in chunkPtr.
 *
 *--------------------------------------------------------------
 */

static int
MarkLayoutProc(
    TkText *textPtr,		/* Text widget being layed out. */
    TkTextIndex *indexPtr,	/* Identifies first character in chunk. */
    TkTextSegment *segPtr,	/* Segment corresponding to indexPtr. */
    int offset,			/* Offset within segPtr corresponding to
				 * indexPtr (always 0). */
    int maxX,			/* Chunk must not occupy pixels at this
				 * position or higher. */
    int maxChars,		/* Chunk must not include more than this many
				 * characters. */
    int noCharsYet,		/* Non-zero means no characters have been
				 * assigned to this line yet. */
    TkWrapMode wrapMode,	/* Not used. */
    register TkTextDispChunk *chunkPtr)
				/* Structure to fill in with information about
				 * this chunk. The x field has already been
				 * set by the caller. */
{
    if (segPtr != textPtr->insertMarkPtr) {
	return -1;
    }

    chunkPtr->displayProc = TkTextInsertDisplayProc;
    chunkPtr->undisplayProc = InsertUndisplayProc;
    chunkPtr->measureProc = NULL;
    chunkPtr->bboxProc = NULL;
    chunkPtr->numBytes = 0;
    chunkPtr->minAscent = 0;
    chunkPtr->minDescent = 0;
    chunkPtr->minHeight = 0;
    chunkPtr->width = 0;

    /*
     * Note: can't break a line after the insertion cursor: this prevents the
     * insertion cursor from being stranded at the end of a line.
     */

    chunkPtr->breakIndex = -1;
    chunkPtr->clientData = textPtr;
    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextInsertDisplayProc --
 *
 *	This function is called to display the insertion cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Graphics are drawn.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
void
TkTextInsertDisplayProc(
    TkText *textPtr,		/* The current text widget. */
    TkTextDispChunk *chunkPtr,	/* Chunk that is to be drawn. */
    int x,			/* X-position in dst at which to draw this
				 * chunk (may differ from the x-position in
				 * the chunk because of scrolling). */
    int y,			/* Y-position at which to draw this chunk in
				 * dst (x-position is in the chunk itself). */
    int height,			/* Total height of line. */
    int baseline,		/* Offset of baseline from y. */
    Display *display,		/* Display to use for drawing. */
    Drawable dst,		/* Pixmap or window in which to draw chunk. */
    int screenY)		/* Y-coordinate in text window that
				 * corresponds to y. */
{
    /*
     * We have no need for the clientData.
     */

    /* TkText *textPtr = chunkPtr->clientData; */
    TkTextIndex index;
    int halfWidth = textPtr->insertWidth/2;
    int rightSideWidth;
    int ix = 0, iy = 0, iw = 0, ih = 0, charWidth = 0;

    if (textPtr->insertCursorType) {
	TkTextMarkSegToIndex(textPtr, textPtr->insertMarkPtr, &index);
	TkTextIndexBbox(textPtr, &index, &ix, &iy, &iw, &ih, &charWidth);
	rightSideWidth = charWidth + halfWidth;
    } else {
	rightSideWidth = halfWidth;
    }

    if ((x + rightSideWidth) < 0) {
	/*
	 * The insertion cursor is off-screen. Indicate caret at 0,0 and
	 * return.
	 */

	Tk_SetCaretPos(textPtr->tkwin, 0, 0, height);
	return;
    }

    Tk_SetCaretPos(textPtr->tkwin, x - halfWidth, screenY, height);

    /*
     * As a special hack to keep the cursor visible on mono displays (or
     * anywhere else that the selection and insertion cursors have the same
     * color) write the default background in the cursor area (instead of
     * nothing) when the cursor isn't on. Otherwise the selection might hide
     * the cursor.
     */

    if (textPtr->flags & GOT_FOCUS) {
	if (textPtr->flags & INSERT_ON) {
	    Tk_Fill3DRectangle(textPtr->tkwin, dst, textPtr->insertBorder,
		    x - halfWidth, y, charWidth + textPtr->insertWidth,
		    height, textPtr->insertBorderWidth, TK_RELIEF_RAISED);
	} else if (textPtr->selBorder == textPtr->insertBorder) {
	    Tk_Fill3DRectangle(textPtr->tkwin, dst, textPtr->border,
		    x - halfWidth, y, charWidth + textPtr->insertWidth,
		    height, 0, TK_RELIEF_FLAT);
	}
    } else if (textPtr->insertUnfocussed == TK_TEXT_INSERT_NOFOCUS_HOLLOW) {
	if (textPtr->insertBorderWidth < 1) {
	    /*
	     * Hack to work around the fact that a "solid" border always
	     * paints in black.
	     */

	    TkBorder *borderPtr = (TkBorder *) textPtr->insertBorder;

	    XDrawRectangle(Tk_Display(textPtr->tkwin), dst, borderPtr->bgGC,
		    x - halfWidth, y, charWidth + textPtr->insertWidth - 1,
		    height - 1);
	} else {
	    Tk_Draw3DRectangle(textPtr->tkwin, dst, textPtr->insertBorder,
		    x - halfWidth, y, charWidth + textPtr->insertWidth,
		    height, textPtr->insertBorderWidth, TK_RELIEF_RAISED);
	}
    } else if (textPtr->insertUnfocussed == TK_TEXT_INSERT_NOFOCUS_SOLID) {
	Tk_Fill3DRectangle(textPtr->tkwin, dst, textPtr->insertBorder,
		x - halfWidth, y, charWidth + textPtr->insertWidth, height,
		textPtr->insertBorderWidth, TK_RELIEF_RAISED);
    }
}

/*
 *--------------------------------------------------------------
 *
 * InsertUndisplayProc --
 *
 *	This function is called when the insertion cursor is no longer at a
 *	visible point on the display. It does nothing right now.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
InsertUndisplayProc(
    TkText *textPtr,		/* Overall information about text widget. */
    TkTextDispChunk *chunkPtr)	/* Chunk that is about to be freed. */
{
    return;
}

/*
 *--------------------------------------------------------------
 *
 * MarkCheckProc --
 *
 *	This function is invoked by the B-tree code to perform consistency
 *	checks on mark segments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The function panics if it detects anything wrong with
 *	the mark.
 *
 *--------------------------------------------------------------
 */

static void
MarkCheckProc(
    TkTextSegment *markPtr,	/* Segment to check. */
    TkTextLine *linePtr)	/* Line containing segment. */
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;

    if (markPtr->body.mark.linePtr != linePtr) {
	Tcl_Panic("MarkCheckProc: markPtr->body.mark.linePtr bogus");
    }

    /*
     * These two marks are not in the hash table
     */

    if (markPtr->body.mark.textPtr->insertMarkPtr == markPtr) {
        return;
    }
    if (markPtr->body.mark.textPtr->currentMarkPtr == markPtr) {
	return;
    }

    /*
     * Make sure that the mark is still present in the text's mark hash table.
     */

    for (hPtr = Tcl_FirstHashEntry(
	    &markPtr->body.mark.textPtr->sharedTextPtr->markTable,
	    &search); hPtr != markPtr->body.mark.hPtr;
	    hPtr = Tcl_NextHashEntry(&search)) {
	if (hPtr == NULL) {
	    Tcl_Panic("MarkCheckProc couldn't find hash table entry for mark");
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * MarkFindNext --
 *
 *	This function searches forward for the next mark.
 *
 * Results:
 *	A standard Tcl result, which is a mark name or an empty string.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
MarkFindNext(
    Tcl_Interp *interp,		/* For error reporting */
    TkText *textPtr,		/* The widget */
    Tcl_Obj *obj)			/* The starting index or mark name */
{
    TkTextIndex index;
    Tcl_HashEntry *hPtr;
    register TkTextSegment *segPtr;
    int offset;
    const char *string = Tcl_GetString(obj);

    if (!strcmp(string, "insert")) {
	segPtr = textPtr->insertMarkPtr;
	TkTextMarkSegToIndex(textPtr, segPtr, &index);
	segPtr = segPtr->nextPtr;
    } else if (!strcmp(string, "current")) {
	segPtr = textPtr->currentMarkPtr;
	TkTextMarkSegToIndex(textPtr, segPtr, &index);
	segPtr = segPtr->nextPtr;
    } else {
	hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->markTable, string);
	if (hPtr != NULL) {
	    /*
	     * If given a mark name, return the next mark in the list of
	     * segments, even if it happens to be at the same character
	     * position.
	     */

	    segPtr = Tcl_GetHashValue(hPtr);
	    TkTextMarkSegToIndex(textPtr, segPtr, &index);
	    segPtr = segPtr->nextPtr;
	} else {
	    /*
	     * For non-mark name indices we want to return any marks that are
	     * right at the index.
	     */

	    if (TkTextGetObjIndex(interp, textPtr, obj, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    for (offset = 0, segPtr = index.linePtr->segPtr;
		    segPtr != NULL && offset < index.byteIndex;
		    offset += segPtr->size, segPtr = segPtr->nextPtr) {
		/* Empty loop body */ ;
	    }
	}
    }

    while (1) {
	/*
	 * segPtr points at the first possible candidate, or NULL if we ran
	 * off the end of the line.
	 */

	for ( ; segPtr != NULL ; segPtr = segPtr->nextPtr) {
	    if (segPtr->typePtr == &tkTextRightMarkType ||
		    segPtr->typePtr == &tkTextLeftMarkType) {
		Tcl_Obj *markName = GetMarkName(textPtr, segPtr);

		if (markName != NULL) {
		    Tcl_SetObjResult(interp, markName);
		    return TCL_OK;
		}
	    }
	}
	index.linePtr = TkBTreeNextLine(textPtr, index.linePtr);
	if (index.linePtr == NULL) {
	    return TCL_OK;
	}
	index.byteIndex = 0;
	segPtr = index.linePtr->segPtr;
    }
}

/*
 *--------------------------------------------------------------
 *
 * MarkFindPrev --
 *
 *	This function searches backwards for the previous mark.
 *
 * Results:
 *	A standard Tcl result, which is a mark name or an empty string.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
MarkFindPrev(
    Tcl_Interp *interp,		/* For error reporting */
    TkText *textPtr,		/* The widget */
    Tcl_Obj *obj)			/* The starting index or mark name */
{
    TkTextIndex index;
    Tcl_HashEntry *hPtr;
    register TkTextSegment *segPtr, *seg2Ptr, *prevPtr;
    int offset;
    const char *string = Tcl_GetString(obj);

    if (!strcmp(string, "insert")) {
	segPtr = textPtr->insertMarkPtr;
	TkTextMarkSegToIndex(textPtr, segPtr, &index);
    } else if (!strcmp(string, "current")) {
	segPtr = textPtr->currentMarkPtr;
	TkTextMarkSegToIndex(textPtr, segPtr, &index);
    } else {
	hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->markTable, string);
	if (hPtr != NULL) {
	    /*
	     * If given a mark name, return the previous mark in the list of
	     * segments, even if it happens to be at the same character
	     * position.
	     */

	    segPtr = Tcl_GetHashValue(hPtr);
	    TkTextMarkSegToIndex(textPtr, segPtr, &index);
	} else {
	    /*
	     * For non-mark name indices we do not return any marks that are
	     * right at the index.
	     */

	    if (TkTextGetObjIndex(interp, textPtr, obj, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    for (offset = 0, segPtr = index.linePtr->segPtr;
		    segPtr != NULL && offset < index.byteIndex;
		    offset += segPtr->size, segPtr = segPtr->nextPtr) {
		/* Empty loop body */
	    }
	}
    }

    while (1) {
	/*
	 * segPtr points just past the first possible candidate, or at the
	 * beginning of the line.
	 */

	for (prevPtr = NULL, seg2Ptr = index.linePtr->segPtr;
		seg2Ptr != NULL && seg2Ptr != segPtr;
		seg2Ptr = seg2Ptr->nextPtr) {
	    if (seg2Ptr->typePtr == &tkTextRightMarkType ||
		    seg2Ptr->typePtr == &tkTextLeftMarkType) {
	        if (seg2Ptr->body.mark.hPtr == NULL) {
                    if (seg2Ptr != textPtr->currentMarkPtr &&
                            seg2Ptr != textPtr->insertMarkPtr) {
	                /*
                         * This is an insert or current mark from a
                         * peer of textPtr.
                         */
                        continue;
                    }
	        }
		prevPtr = seg2Ptr;
	    }
	}
	if (prevPtr != NULL) {
	    Tcl_Obj *markName = GetMarkName(textPtr, prevPtr);

	    if (markName != NULL) {
		Tcl_SetObjResult(interp, markName);
		return TCL_OK;
	    }
	}
	index.linePtr = TkBTreePreviousLine(textPtr, index.linePtr);
	if (index.linePtr == NULL) {
	    return TCL_OK;
	}
	segPtr = NULL;
    }
}

/*
 * ------------------------------------------------------------------------
 *
 * GetMarkName --
 *	Returns the name of the mark that is the given text segment, or NULL
 *	if it is unnamed (i.e., a widget-specific mark that isn't "current" or
 *	"insert").
 *
 * ------------------------------------------------------------------------
 */

static Tcl_Obj *
GetMarkName(
    TkText *textPtr,
    TkTextSegment *segPtr)
{
    const char *markName;

    if (segPtr == textPtr->currentMarkPtr) {
	markName = "current";
    } else if (segPtr == textPtr->insertMarkPtr) {
	markName = "insert";
    } else if (segPtr->body.mark.hPtr == NULL) {
	/*
	 * Ignore widget-specific marks for the other widgets. This is either
	 * an insert or a current mark (markPtr->body.mark.hPtr actually
	 * receives NULL for these marks in TkTextSetMark). The insert and
	 * current marks for textPtr having already been tested above, the
	 * current segment is an insert or current mark from a peer of
	 * textPtr, which we don't want to return.
	 */

	return NULL;
    } else {
	markName = Tcl_GetHashKey(&textPtr->sharedTextPtr->markTable,
		segPtr->body.mark.hPtr);
    }
    return Tcl_NewStringObj(markName, -1);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
