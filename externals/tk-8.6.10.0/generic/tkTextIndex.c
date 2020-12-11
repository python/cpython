/*
 * tkTextIndex.c --
 *
 *	This module provides functions that manipulate indices for text
 *	widgets.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"
#include "tkText.h"

/*
 * Index to use to select last character in line (very large integer):
 */

#define LAST_CHAR 1000000

/*
 * Modifiers for index parsing: 'display', 'any' or nothing.
 */

#define TKINDEX_NONE	0
#define TKINDEX_DISPLAY	1
#define TKINDEX_ANY	2

/*
 * Forward declarations for functions defined later in this file:
 */

static const char *	ForwBack(TkText *textPtr, const char *string,
			    TkTextIndex *indexPtr);
static const char *	StartEnd(TkText *textPtr, const char *string,
			    TkTextIndex *indexPtr);
static int		GetIndex(Tcl_Interp *interp, TkSharedText *sharedPtr,
			    TkText *textPtr, const char *string,
			    TkTextIndex *indexPtr, int *canCachePtr);
static int              IndexCountBytesOrdered(const TkText *textPtr,
                            const TkTextIndex *indexPtr1,
                            const TkTextIndex *indexPtr2);

/*
 * The "textindex" Tcl_Obj definition:
 */

static void		DupTextIndexInternalRep(Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr);
static void		FreeTextIndexInternalRep(Tcl_Obj *listPtr);
static void		UpdateStringOfTextIndex(Tcl_Obj *objPtr);

/*
 * Accessor macros for the "textindex" type.
 */

#define GET_TEXTINDEX(objPtr) \
	((TkTextIndex *) (objPtr)->internalRep.twoPtrValue.ptr1)
#define GET_INDEXEPOCH(objPtr) \
	(PTR2INT((objPtr)->internalRep.twoPtrValue.ptr2))
#define SET_TEXTINDEX(objPtr, indexPtr) \
	((objPtr)->internalRep.twoPtrValue.ptr1 = (void *) (indexPtr))
#define SET_INDEXEPOCH(objPtr, epoch) \
	((objPtr)->internalRep.twoPtrValue.ptr2 = INT2PTR(epoch))

/*
 * Define the 'textindex' object type, which Tk uses to represent indices in
 * text widgets internally.
 */

const Tcl_ObjType tkTextIndexType = {
    "textindex",		/* name */
    FreeTextIndexInternalRep,	/* freeIntRepProc */
    DupTextIndexInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

static void
FreeTextIndexInternalRep(
    Tcl_Obj *indexObjPtr)	/* TextIndex object with internal rep to
				 * free. */
{
    TkTextIndex *indexPtr = GET_TEXTINDEX(indexObjPtr);

    if (indexPtr->textPtr != NULL) {
	if (indexPtr->textPtr->refCount-- <= 1) {
	    /*
	     * The text widget has been deleted and we need to free it now.
	     */

	    ckfree(indexPtr->textPtr);
	}
    }
    ckfree(indexPtr);
    indexObjPtr->typePtr = NULL;
}

static void
DupTextIndexInternalRep(
    Tcl_Obj *srcPtr,		/* TextIndex obj with internal rep to copy. */
    Tcl_Obj *copyPtr)		/* TextIndex obj with internal rep to set. */
{
    int epoch;
    TkTextIndex *dupIndexPtr, *indexPtr;

    dupIndexPtr = ckalloc(sizeof(TkTextIndex));
    indexPtr = GET_TEXTINDEX(srcPtr);
    epoch = GET_INDEXEPOCH(srcPtr);

    dupIndexPtr->tree = indexPtr->tree;
    dupIndexPtr->linePtr = indexPtr->linePtr;
    dupIndexPtr->byteIndex = indexPtr->byteIndex;
    dupIndexPtr->textPtr = indexPtr->textPtr;
    if (dupIndexPtr->textPtr != NULL) {
	dupIndexPtr->textPtr->refCount++;
    }
    SET_TEXTINDEX(copyPtr, dupIndexPtr);
    SET_INDEXEPOCH(copyPtr, epoch);
    copyPtr->typePtr = &tkTextIndexType;
}

/*
 * This will not be called except by TkTextNewIndexObj below. This is because
 * if a TkTextIndex is no longer valid, it is not possible to regenerate the
 * string representation.
 */

static void
UpdateStringOfTextIndex(
    Tcl_Obj *objPtr)
{
    char buffer[TK_POS_CHARS];
    size_t len;
    const TkTextIndex *indexPtr = GET_TEXTINDEX(objPtr);

    len = TkTextPrintIndex(indexPtr->textPtr, indexPtr, buffer);

    objPtr->bytes = ckalloc(len + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = len;
}

/*
 *---------------------------------------------------------------------------
 *
 * MakeObjIndex --
 *
 *	This function generates a Tcl_Obj description of an index, suitable
 *	for reading in again later. If the 'textPtr' is NULL then we still
 *	generate an index object, but it's internal description is deemed
 *	non-cacheable, and therefore effectively useless (apart from as a
 *	temporary memory storage). This is used for indices whose meaning is
 *	very temporary (like @0,0 or the name of a mark or tag). The mapping
 *	from such strings/objects to actual TkTextIndex pointers is not stable
 *	to minor text widget changes which we do not track (we track
 *	insertions and deletions).
 *
 * Results:
 *	A pointer to an allocated TkTextIndex which will be freed
 *	automatically when the Tcl_Obj is used for other purposes.
 *
 * Side effects:
 *	A small amount of memory is allocated.
 *
 *---------------------------------------------------------------------------
 */

static TkTextIndex *
MakeObjIndex(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Obj *objPtr,		/* Object containing description of
				 * position. */
    const TkTextIndex *origPtr)	/* Pointer to index. */
{
    TkTextIndex *indexPtr = ckalloc(sizeof(TkTextIndex));

    indexPtr->tree = origPtr->tree;
    indexPtr->linePtr = origPtr->linePtr;
    indexPtr->byteIndex = origPtr->byteIndex;
    SET_TEXTINDEX(objPtr, indexPtr);
    objPtr->typePtr = &tkTextIndexType;
    indexPtr->textPtr = textPtr;

    if (textPtr != NULL) {
	textPtr->refCount++;
	SET_INDEXEPOCH(objPtr, textPtr->sharedTextPtr->stateEpoch);
    } else {
	SET_INDEXEPOCH(objPtr, 0);
    }
    return indexPtr;
}

const TkTextIndex *
TkTextGetIndexFromObj(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Obj *objPtr)		/* Object containing description of
				 * position. */
{
    TkTextIndex index;
    TkTextIndex *indexPtr = NULL;
    int cache;

    if (objPtr->typePtr == &tkTextIndexType) {
	int epoch;

	indexPtr = GET_TEXTINDEX(objPtr);
	epoch = GET_INDEXEPOCH(objPtr);

	if (epoch == textPtr->sharedTextPtr->stateEpoch) {
	    if (indexPtr->textPtr == textPtr) {
		return indexPtr;
	    }
	}
    }

    /*
     * The object is either not an index type or referred to a different text
     * widget, or referred to the correct widget, but it is out of date (text
     * has been added/deleted since).
     */

    if (GetIndex(interp, NULL, textPtr, Tcl_GetString(objPtr), &index,
	    &cache) != TCL_OK) {
	return NULL;
    }

    if (objPtr->typePtr != NULL) {
	if (objPtr->bytes == NULL) {
	    objPtr->typePtr->updateStringProc(objPtr);
	}
	if (objPtr->typePtr->freeIntRepProc != NULL) {
	    objPtr->typePtr->freeIntRepProc(objPtr);
	}
    }

    return MakeObjIndex((cache ? textPtr : NULL), objPtr, &index);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextNewIndexObj --
 *
 *	This function generates a Tcl_Obj description of an index, suitable
 *	for reading in again later. The index generated is effectively stable
 *	to all except insertion/deletion operations on the widget.
 *
 * Results:
 *	A new Tcl_Obj with refCount zero.
 *
 * Side effects:
 *	A small amount of memory is allocated.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj *
TkTextNewIndexObj(
    TkText *textPtr,		/* Text widget for this index */
    const TkTextIndex *indexPtr)/* Pointer to index. */
{
    Tcl_Obj *retVal;

    retVal = Tcl_NewObj();
    retVal->bytes = NULL;

    /*
     * Assumption that the above call returns an object with:
     * retVal->typePtr == NULL
     */

    MakeObjIndex(textPtr, retVal, indexPtr);

    /*
     * Unfortunately, it isn't possible for us to regenerate the string
     * representation so we have to create it here, while we can be sure the
     * contents of the index are still valid.
     */

    UpdateStringOfTextIndex(retVal);
    return retVal;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextMakePixelIndex --
 *
 *	Given a pixel index and a byte index, look things up in the B-tree and
 *	fill in a TkTextIndex structure.
 *
 *	The valid input range for pixelIndex is from 0 to the number of pixels
 *	in the widget-1. Anything outside that range will be rounded to the
 *	closest acceptable value.
 *
 * Results:
 *
 *	The structure at *indexPtr is filled in with information about the
 *	character at pixelIndex (or the closest existing character, if the
 *	specified one doesn't exist), and the number of excess pixels is
 *	returned as a result. This means if the given pixel index is exactly
 *	correct for the top-edge of the indexPtr, then zero will be returned,
 *	and otherwise we will return the calculation 'desired pixelIndex' -
 *	'actual pixel index of indexPtr'.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextMakePixelIndex(
    TkText *textPtr,		/* The Text Widget */
    int pixelIndex,		/* Pixel-index of desired line (0 means first
				 * pixel of first line of text). */
    TkTextIndex *indexPtr)	/* Structure to fill in. */
{
    int pixelOffset = 0;

    indexPtr->tree = textPtr->sharedTextPtr->tree;
    indexPtr->textPtr = textPtr;

    if (pixelIndex < 0) {
	pixelIndex = 0;
    }
    indexPtr->linePtr = TkBTreeFindPixelLine(textPtr->sharedTextPtr->tree,
	    textPtr, pixelIndex, &pixelOffset);

    /*
     * 'pixelIndex' was too large, so we try again, just to find the last
     * pixel in the window.
     */

    if (indexPtr->linePtr == NULL) {
	int lastMinusOne = TkBTreeNumPixels(textPtr->sharedTextPtr->tree,
		textPtr)-1;

	indexPtr->linePtr = TkBTreeFindPixelLine(textPtr->sharedTextPtr->tree,
		textPtr, lastMinusOne, &pixelOffset);
	indexPtr->byteIndex = 0;
	return pixelOffset;
    }
    indexPtr->byteIndex = 0;

    if (pixelOffset <= 0) {
	return 0;
    }
    return TkTextMeasureDown(textPtr, indexPtr, pixelOffset);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextMakeByteIndex --
 *
 *	Given a line index and a byte index, look things up in the B-tree and
 *	fill in a TkTextIndex structure.
 *
 * Results:
 *	The structure at *indexPtr is filled in with information about the
 *	character at lineIndex and byteIndex (or the closest existing
 *	character, if the specified one doesn't exist), and indexPtr is
 *	returned as result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TkTextIndex *
TkTextMakeByteIndex(
    TkTextBTree tree,	/* Tree that lineIndex and byteIndex refer
				 * to. */
    const TkText *textPtr,
    int lineIndex,		/* Index of desired line (0 means first line
				 * of text). */
    int byteIndex,		/* Byte index of desired character. */
    TkTextIndex *indexPtr)	/* Structure to fill in. */
{
    TkTextSegment *segPtr;
    int index;
    const char *p, *start;
    int ch;

    indexPtr->tree = tree;
    if (lineIndex < 0) {
	lineIndex = 0;
	byteIndex = 0;
    }
    if (byteIndex < 0) {
	byteIndex = 0;
    }
    indexPtr->linePtr = TkBTreeFindLine(tree, textPtr, lineIndex);
    if (indexPtr->linePtr == NULL) {
	indexPtr->linePtr = TkBTreeFindLine(tree, textPtr,
		TkBTreeNumLines(tree, textPtr));
	byteIndex = 0;
    }
    if (byteIndex == 0) {
	indexPtr->byteIndex = byteIndex;
	return indexPtr;
    }

    /*
     * Verify that the index is within the range of the line and points to a
     * valid character boundary.
     */

    index = 0;
    for (segPtr = indexPtr->linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (segPtr == NULL) {
	    /*
	     * Use the index of the last character in the line. Since the last
	     * character on the line is guaranteed to be a '\n', we can back
	     * up a constant sizeof(char) bytes.
	     */

	    indexPtr->byteIndex = index - sizeof(char);
	    break;
	}
	if (index + segPtr->size > byteIndex) {
	    indexPtr->byteIndex = byteIndex;
	    if ((byteIndex > index) && (segPtr->typePtr == &tkTextCharType)) {
		/*
		 * Prevent UTF-8 character from being split up by ensuring
		 * that byteIndex falls on a character boundary. If the index
		 * falls in the middle of a UTF-8 character, it will be
		 * adjusted to the end of that UTF-8 character.
		 */

		start = segPtr->body.chars + (byteIndex - index);
		p = Tcl_UtfPrev(start, segPtr->body.chars);
		p += TkUtfToUniChar(p, &ch);
		indexPtr->byteIndex += p - start;
	    }
	    break;
	}
	index += segPtr->size;
    }
    return indexPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextMakeCharIndex --
 *
 *	Given a line index and a character index, look things up in the B-tree
 *	and fill in a TkTextIndex structure.
 *
 * Results:
 *	The structure at *indexPtr is filled in with information about the
 *	character at lineIndex and charIndex (or the closest existing
 *	character, if the specified one doesn't exist), and indexPtr is
 *	returned as result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TkTextIndex *
TkTextMakeCharIndex(
    TkTextBTree tree,		/* Tree that lineIndex and charIndex refer
				 * to. */
    TkText *textPtr,
    int lineIndex,		/* Index of desired line (0 means first line
				 * of text). */
    int charIndex,		/* Index of desired character. */
    TkTextIndex *indexPtr)	/* Structure to fill in. */
{
    register TkTextSegment *segPtr;
    char *p, *start, *end;
    int index, offset;
    int ch;

    indexPtr->tree = tree;
    if (lineIndex < 0) {
	lineIndex = 0;
	charIndex = 0;
    }
    if (charIndex < 0) {
	charIndex = 0;
    }
    indexPtr->linePtr = TkBTreeFindLine(tree, textPtr, lineIndex);
    if (indexPtr->linePtr == NULL) {
	indexPtr->linePtr = TkBTreeFindLine(tree, textPtr,
		TkBTreeNumLines(tree, textPtr));
	charIndex = 0;
    }

    /*
     * Verify that the index is within the range of the line. If not, just use
     * the index of the last character in the line.
     */

    index = 0;
    for (segPtr = indexPtr->linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (segPtr == NULL) {
	    /*
	     * Use the index of the last character in the line. Since the last
	     * character on the line is guaranteed to be a '\n', we can back
	     * up a constant sizeof(char) bytes.
	     */

	    indexPtr->byteIndex = index - sizeof(char);
	    break;
	}
	if (segPtr->typePtr == &tkTextCharType) {
	    /*
	     * Turn character offset into a byte offset.
	     */

	    start = segPtr->body.chars;
	    end = start + segPtr->size;
	    for (p = start; p < end; p += offset) {
		if (charIndex == 0) {
		    indexPtr->byteIndex = index;
		    return indexPtr;
		}
		charIndex--;
		offset = TkUtfToUniChar(p, &ch);
		index += offset;
	    }
	} else {
	    if (charIndex < segPtr->size) {
		indexPtr->byteIndex = index;
		break;
	    }
	    charIndex -= segPtr->size;
	    index += segPtr->size;
	}
    }
    return indexPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexToSeg --
 *
 *	Given an index, this function returns the segment and offset within
 *	segment for the index.
 *
 * Results:
 *	The return value is a pointer to the segment referred to by indexPtr;
 *	this will always be a segment with non-zero size. The variable at
 *	*offsetPtr is set to hold the integer offset within the segment of the
 *	character given by indexPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TkTextSegment *
TkTextIndexToSeg(
    const TkTextIndex *indexPtr,/* Text index. */
    int *offsetPtr)		/* Where to store offset within segment, or
				 * NULL if offset isn't wanted. */
{
    TkTextSegment *segPtr;
    int offset;

    for (offset = indexPtr->byteIndex, segPtr = indexPtr->linePtr->segPtr;
	    offset >= segPtr->size;
	    offset -= segPtr->size, segPtr = segPtr->nextPtr) {
	/* Empty loop body. */
    }
    if (offsetPtr != NULL) {
	*offsetPtr = offset;
    }
    return segPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextSegToOffset --
 *
 *	Given a segment pointer and the line containing it, this function
 *	returns the offset of the segment within its line.
 *
 * Results:
 *	The return value is the offset (within its line) of the first
 *	character in segPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextSegToOffset(
    const TkTextSegment *segPtr,/* Segment whose offset is desired. */
    const TkTextLine *linePtr)	/* Line containing segPtr. */
{
    const TkTextSegment *segPtr2;
    int offset = 0;

    for (segPtr2 = linePtr->segPtr; segPtr2 != segPtr;
	    segPtr2 = segPtr2->nextPtr) {
	offset += segPtr2->size;
    }
    return offset;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextGetObjIndex --
 *
 *	Simpler wrapper around the string based function, but could be
 *	enhanced with a new object type in the future.
 *
 * Results:
 *	see TkTextGetIndex
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextGetObjIndex(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Obj *idxObj,		/* Object containing textual description of
				 * position. */
    TkTextIndex *indexPtr)	/* Index structure to fill in. */
{
    return GetIndex(interp, NULL, textPtr, Tcl_GetString(idxObj), indexPtr,
	    NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextSharedGetObjIndex --
 *
 *	Simpler wrapper around the string based function, but could be
 *	enhanced with a new object type in the future.
 *
 * Results:
 *	see TkTextGetIndex
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextSharedGetObjIndex(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    TkSharedText *sharedTextPtr,/* Information about text widget. */
    Tcl_Obj *idxObj,		/* Object containing textual description of
				 * position. */
    TkTextIndex *indexPtr)	/* Index structure to fill in. */
{
    return GetIndex(interp, sharedTextPtr, NULL, Tcl_GetString(idxObj),
	    indexPtr, NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextGetIndex --
 *
 *	Given a string, return the index that is described.
 *
 * Results:
 *	The return value is a standard Tcl return result. If TCL_OK is
 *	returned, then everything went well and the index at *indexPtr is
 *	filled in; otherwise TCL_ERROR is returned and an error message is
 *	left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextGetIndex(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    TkText *textPtr,		/* Information about text widget. */
    const char *string,		/* Textual description of position. */
    TkTextIndex *indexPtr)	/* Index structure to fill in. */
{
    return GetIndex(interp, NULL, textPtr, string, indexPtr, NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetIndex --
 *
 *	Given a string, return the index that is described.
 *
 * Results:
 *	The return value is a standard Tcl return result. If TCL_OK is
 *	returned, then everything went well and the index at *indexPtr is
 *	filled in; otherwise TCL_ERROR is returned and an error message is
 *	left in the interp's result.
 *
 *	If *canCachePtr is non-NULL, and everything went well, the integer it
 *	points to is set to 1 if the indexPtr is something which can be
 *	cached, and zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
GetIndex(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    TkSharedText *sharedPtr,
    TkText *textPtr,		/* Information about text widget. */
    const char *string,		/* Textual description of position. */
    TkTextIndex *indexPtr,	/* Index structure to fill in. */
    int *canCachePtr)		/* Pointer to integer to store whether we can
				 * cache the index (or NULL). */
{
    char *p, *end, *endOfBase;
    TkTextIndex first, last;
    int wantLast, result;
    char c;
    const char *cp;
    Tcl_DString copy;
    int canCache = 0;

    if (sharedPtr == NULL) {
	sharedPtr = textPtr->sharedTextPtr;
    }

    /*
     *---------------------------------------------------------------------
     * Stage 1: check to see if the index consists of nothing but a mark
     * name, an embedded window or an embedded image.  We do this check
     * now even though it's also done later, in order to allow mark names,
     * embedded window names or image names that include funny characters
     * such as spaces or "+1c".
     *---------------------------------------------------------------------
     */

    if (TkTextMarkNameToIndex(textPtr, string, indexPtr) == TCL_OK) {
	goto done;
    }

    if (TkTextWindowIndex(textPtr, string, indexPtr) != 0) {
	goto done;
    }

    if (TkTextImageIndex(textPtr, string, indexPtr) != 0) {
	goto done;
    }

    /*
     *------------------------------------------------
     * Stage 2: start again by parsing the base index.
     *------------------------------------------------
     */

    indexPtr->tree = sharedPtr->tree;

    /*
     * First look for the form "tag.first" or "tag.last" where "tag" is the
     * name of a valid tag. Try to use up as much as possible of the string in
     * this check (strrchr instead of strchr below). Doing the check now, and
     * in this way, allows tag names to include funny characters like "@" or
     * "+1c".
     */

    Tcl_DStringInit(&copy);
    p = strrchr(Tcl_DStringAppend(&copy, string, -1), '.');
    if (p != NULL) {
	TkTextSearch search;
	TkTextTag *tagPtr;
	Tcl_HashEntry *hPtr = NULL;
	const char *tagName;

	if ((p[1] == 'f') && (strncmp(p+1, "first", 5) == 0)) {
	    wantLast = 0;
	    endOfBase = p+6;
	} else if ((p[1] == 'l') && (strncmp(p+1, "last", 4) == 0)) {
	    wantLast = 1;
	    endOfBase = p+5;
	} else {
	    goto tryxy;
	}

	tagPtr = NULL;
	tagName = Tcl_DStringValue(&copy);
	if (((p - tagName) == 3) && !strncmp(tagName, "sel", 3)) {
	    /*
	     * Special case for sel tag which is not stored in the hash table.
	     */

	    tagPtr = textPtr->selTagPtr;
	} else {
	    *p = 0;
	    hPtr = Tcl_FindHashEntry(&sharedPtr->tagTable, tagName);
	    *p = '.';
	    if (hPtr != NULL) {
		tagPtr = Tcl_GetHashValue(hPtr);
	    }
	}

	if (tagPtr == NULL) {
	    goto tryxy;
	}

	TkTextMakeByteIndex(sharedPtr->tree, textPtr, 0, 0, &first);
	TkTextMakeByteIndex(sharedPtr->tree, textPtr,
		TkBTreeNumLines(sharedPtr->tree, textPtr), 0, &last);
	TkBTreeStartSearch(&first, &last, tagPtr, &search);
	if (!TkBTreeCharTagged(&first, tagPtr) && !TkBTreeNextTag(&search)) {
	    if (tagPtr == textPtr->selTagPtr) {
		tagName = "sel";
	    } else if (hPtr != NULL) {
		tagName = Tcl_GetHashKey(&sharedPtr->tagTable, hPtr);
	    }
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "text doesn't contain any characters tagged with \"%s\"",
		    tagName));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "TEXT_INDEX", tagName,
		    NULL);
	    Tcl_DStringFree(&copy);
	    return TCL_ERROR;
	}
	*indexPtr = search.curIndex;
	if (wantLast) {
	    while (TkBTreeNextTag(&search)) {
		*indexPtr = search.curIndex;
	    }
	}
	goto gotBase;
    }

  tryxy:
    if (string[0] == '@') {
	/*
	 * Find character at a given x,y location in the window.
	 */

	int x, y;

	cp = string+1;
	x = strtol(cp, &end, 0);
	if ((end == cp) || (*end != ',')) {
	    goto error;
	}
	cp = end+1;
	y = strtol(cp, &end, 0);
	if (end == cp) {
	    goto error;
	}
	TkTextPixelIndex(textPtr, x, y, indexPtr, NULL);
	endOfBase = end;
	goto gotBase;
    }

    if (isdigit(UCHAR(string[0])) || (string[0] == '-')) {
	int lineIndex, charIndex;

	/*
	 * Base is identified with line and character indices.
	 */

	lineIndex = strtol(string, &end, 0) - 1;
	if ((end == string) || (*end != '.')) {
	    goto error;
	}
	p = end+1;
	if ((*p == 'e') && (strncmp(p, "end", 3) == 0)) {
	    charIndex = LAST_CHAR;
	    endOfBase = p+3;
	} else {
	    charIndex = strtol(p, &end, 0);
	    if (end == p) {
		goto error;
	    }
	    endOfBase = end;
	}
	TkTextMakeCharIndex(sharedPtr->tree, textPtr, lineIndex, charIndex,
		indexPtr);
	canCache = 1;
	goto gotBase;
    }

    for (p = Tcl_DStringValue(&copy); *p != 0; p++) {
	if (isspace(UCHAR(*p)) || (*p == '+') || (*p == '-')) {
	    break;
	}
    }
    endOfBase = p;
    if (string[0] == '.') {
	/*
	 * See if the base position is the name of an embedded window.
	 */

	c = *endOfBase;
	*endOfBase = 0;
	result = TkTextWindowIndex(textPtr, Tcl_DStringValue(&copy), indexPtr);
	*endOfBase = c;
	if (result != 0) {
	    goto gotBase;
	}
    }
    if ((string[0] == 'e')
	    && (strncmp(string, "end",
	    (size_t) (endOfBase-Tcl_DStringValue(&copy))) == 0)) {
	/*
	 * Base position is end of text.
	 */

	TkTextMakeByteIndex(sharedPtr->tree, textPtr,
		TkBTreeNumLines(sharedPtr->tree, textPtr), 0, indexPtr);
	canCache = 1;
	goto gotBase;
    } else {
	/*
	 * See if the base position is the name of a mark.
	 */

	c = *endOfBase;
	*endOfBase = 0;
	result = TkTextMarkNameToIndex(textPtr, Tcl_DStringValue(&copy),
		indexPtr);
	*endOfBase = c;
	if (result == TCL_OK) {
	    goto gotBase;
	}

	/*
	 * See if the base position is the name of an embedded image.
	 */

	c = *endOfBase;
	*endOfBase = 0;
	result = TkTextImageIndex(textPtr, Tcl_DStringValue(&copy), indexPtr);
	*endOfBase = c;
	if (result != 0) {
	    goto gotBase;
	}
    }
    goto error;

    /*
     *-------------------------------------------------------------------
     * Stage 3: process zero or more modifiers. Each modifier is either a
     * keyword like "wordend" or "linestart", or it has the form "op count
     * units" where op is + or -, count is a number, and units is "chars" or
     * "lines".
     *-------------------------------------------------------------------
     */

  gotBase:
    cp = endOfBase;
    while (1) {
	while (isspace(UCHAR(*cp))) {
	    cp++;
	}
	if (*cp == 0) {
	    break;
	}

	if ((*cp == '+') || (*cp == '-')) {
	    cp = ForwBack(textPtr, cp, indexPtr);
	} else {
	    cp = StartEnd(textPtr, cp, indexPtr);
	}
	if (cp == NULL) {
	    goto error;
	}
    }
    Tcl_DStringFree(&copy);

  done:
    if (canCachePtr != NULL) {
	*canCachePtr = canCache;
    }
    if (indexPtr->linePtr == NULL) {
	Tcl_Panic("Bad index created");
    }
    return TCL_OK;

  error:
    Tcl_DStringFree(&copy);
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad text index \"%s\"", string));
    Tcl_SetErrorCode(interp, "TK", "TEXT", "BAD_INDEX", NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextPrintIndex --
 *
 *	This function generates a string description of an index, suitable for
 *	reading in again later.
 *
 * Results:
 *	The characters pointed to by string are modified. Returns the number
 *	of characters in the string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextPrintIndex(
    const TkText *textPtr,
    const TkTextIndex *indexPtr,/* Pointer to index. */
    char *string)		/* Place to store the position. Must have at
				 * least TK_POS_CHARS characters. */
{
    TkTextSegment *segPtr;
    TkTextLine *linePtr;
    int numBytes, charIndex;

    numBytes = indexPtr->byteIndex;
    charIndex = 0;
    linePtr = indexPtr->linePtr;

    for (segPtr = linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (segPtr == NULL) {
	    /*
	     * Two logical lines merged into one display line through eliding
	     * of a newline.
	     */

	    linePtr = TkBTreeNextLine(NULL, linePtr);
	    segPtr = linePtr->segPtr;
	}
	if (numBytes <= segPtr->size) {
	    break;
	}
	if (segPtr->typePtr == &tkTextCharType) {
	    charIndex += Tcl_NumUtfChars(segPtr->body.chars, segPtr->size);
	} else {
	    charIndex += segPtr->size;
	}
	numBytes -= segPtr->size;
    }

    if (segPtr->typePtr == &tkTextCharType) {
	charIndex += Tcl_NumUtfChars(segPtr->body.chars, numBytes);
    } else {
	charIndex += numBytes;
    }

    return sprintf(string, "%d.%d",
	    TkBTreeLinesTo(textPtr, indexPtr->linePtr) + 1, charIndex);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexCmp --
 *
 *	Compare two indices to see which one is earlier in the text.
 *
 * Results:
 *	The return value is 0 if index1Ptr and index2Ptr refer to the same
 *	position in the file, -1 if index1Ptr refers to an earlier position
 *	than index2Ptr, and 1 otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextIndexCmp(
    const TkTextIndex*index1Ptr,/* First index. */
    const TkTextIndex*index2Ptr)/* Second index. */
{
    int line1, line2;

    if (index1Ptr->linePtr == index2Ptr->linePtr) {
	if (index1Ptr->byteIndex < index2Ptr->byteIndex) {
	    return -1;
	} else if (index1Ptr->byteIndex > index2Ptr->byteIndex) {
	    return 1;
	} else {
	    return 0;
	}
    }

    /*
     * Assumption here that it is ok for comparisons to reflect the full
     * B-tree and not just the portion that is available to any client. This
     * should be true because the only indexPtr's we should be given are ones
     * which are valid for the current client.
     */

    line1 = TkBTreeLinesTo(NULL, index1Ptr->linePtr);
    line2 = TkBTreeLinesTo(NULL, index2Ptr->linePtr);
    if (line1 < line2) {
	return -1;
    }
    if (line1 > line2) {
	return 1;
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * ForwBack --
 *
 *	This function handles +/- modifiers for indices to adjust the index
 *	forwards or backwards.
 *
 * Results:
 *	If the modifier in string is successfully parsed then the return value
 *	is the address of the first character after the modifier, and
 *	*indexPtr is updated to reflect the modifier. If there is a syntax
 *	error in the modifier then NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static const char *
ForwBack(
    TkText *textPtr,		/* Information about text widget. */
    const char *string,		/* String to parse for additional info about
				 * modifier (count and units). Points to "+"
				 * or "-" that starts modifier. */
    TkTextIndex *indexPtr)	/* Index to update as specified in string. */
{
    register const char *p, *units;
    char *end;
    int count, lineIndex, modifier;
    size_t length;

    /*
     * Get the count (how many units forward or backward).
     */

    p = string+1;
    while (isspace(UCHAR(*p))) {
	p++;
    }
    count = strtol(p, &end, 0);
    if (end == p) {
	return NULL;
    }
    p = end;
    while (isspace(UCHAR(*p))) {
	p++;
    }

    /*
     * Find the end of this modifier (next space or + or - character), then
     * check if there is a textual 'display' or 'any' modifier. These
     * modifiers can be their own word (in which case they can be abbreviated)
     * or they can follow on to the actual unit in a single word (in which
     * case no abbreviation is allowed). So, 'display lines', 'd lines',
     * 'displaylin' are all ok, but 'dline' is not.
     */

    units = p;
    while ((*p != '\0') && !isspace(UCHAR(*p)) && (*p != '+') && (*p != '-')) {
	p++;
    }
    length = p - units;
    if ((*units == 'd') &&
	    (strncmp(units, "display", (length > 7 ? 7 : length)) == 0)) {
	modifier = TKINDEX_DISPLAY;
	if (length > 7) {
	    p -= (length - 7);
	}
    } else if ((*units == 'a') &&
	    (strncmp(units, "any", (length > 3 ? 3 : length)) == 0)) {
	modifier = TKINDEX_ANY;
	if (length > 3) {
	    p -= (length - 3);
	}
    } else {
	modifier = TKINDEX_NONE;
    }

    /*
     * If we had a modifier, which we interpreted ok, so now forward to the
     * actual units.
     */

    if (modifier != TKINDEX_NONE) {
	while (isspace(UCHAR(*p))) {
	    p++;
	}
	units = p;
	while (*p!='\0' && !isspace(UCHAR(*p)) && *p!='+' && *p!='-') {
	    p++;
	}
	length = p - units;
    }

    /*
     * Finally parse the units.
     */

    if ((*units == 'c') && (strncmp(units, "chars", length) == 0)) {
	TkTextCountType type;

	if (modifier == TKINDEX_NONE) {
	    type = COUNT_INDICES;
	} else if (modifier == TKINDEX_ANY) {
	    type = COUNT_CHARS;
	} else {
	    type = COUNT_DISPLAY_CHARS;
	}

	if (*string == '+') {
	    TkTextIndexForwChars(textPtr, indexPtr, count, indexPtr, type);
	} else {
	    TkTextIndexBackChars(textPtr, indexPtr, count, indexPtr, type);
	}
    } else if ((*units == 'i') && (strncmp(units, "indices", length) == 0)) {
	TkTextCountType type;

	if (modifier == TKINDEX_DISPLAY) {
	    type = COUNT_DISPLAY_INDICES;
	} else {
	    type = COUNT_INDICES;
	}

	if (*string == '+') {
	    TkTextIndexForwChars(textPtr, indexPtr, count, indexPtr, type);
	} else {
	    TkTextIndexBackChars(textPtr, indexPtr, count, indexPtr, type);
	}
    } else if ((*units == 'l') && (strncmp(units, "lines", length) == 0)) {
	if (modifier == TKINDEX_DISPLAY) {
	    /*
	     * Find the appropriate pixel offset of the current position
	     * within its display line. This also has the side-effect of
	     * moving indexPtr, but that doesn't matter since we will do it
	     * again below.
	     *
	     * Then find the right display line, and finally calculated the
	     * index we want in that display line, based on the original pixel
	     * offset.
	     */

	    int xOffset, forward;

	    if (TkTextIsElided(textPtr, indexPtr, NULL)) {
		/*
		 * Go forward to the first non-elided index.
		 */

		TkTextIndexForwChars(textPtr, indexPtr, 0, indexPtr,
			COUNT_DISPLAY_INDICES);
	    }

	    /*
	     * Unlike the Forw/BackChars code, the display line code is
	     * sensitive to whether we are genuinely going forwards or
	     * backwards. So, we need to determine that. This is important in
	     * the case where we have "+ -3 displaylines", for example.
	     */

	    if ((count < 0) ^ (*string == '-')) {
		forward = 0;
	    } else {
		forward = 1;
	    }

	    count = abs(count);
	    if (count == 0) {
		return p;
	    }

	    if (forward) {
		TkTextFindDisplayLineEnd(textPtr, indexPtr, 1, &xOffset);
		while (count-- > 0) {

		    /*
		     * Go to the end of the line, then forward one char/byte
		     * to get to the beginning of the next line.
		     */

		    TkTextFindDisplayLineEnd(textPtr, indexPtr, 1, NULL);
		    TkTextIndexForwChars(textPtr, indexPtr, 1, indexPtr,
			    COUNT_DISPLAY_INDICES);
		}
	    } else {
		TkTextFindDisplayLineEnd(textPtr, indexPtr, 0, &xOffset);
		while (count-- > 0) {
                    TkTextIndex indexPtr2;

		    /*
		     * Go to the beginning of the line, then backward one
		     * char/byte to get to the end of the previous line.
		     */

		    TkTextFindDisplayLineEnd(textPtr, indexPtr, 0, NULL);
		    TkTextIndexBackChars(textPtr, indexPtr, 1, &indexPtr2,
			    COUNT_DISPLAY_INDICES);

                    /*
                     * If we couldn't go to the previous line, then we wanted
                       to go before the start of the text: arrange for returning
                       the first index of the first display line.
                     */

                    if (!TkTextIndexCmp(indexPtr, &indexPtr2)) {
                        xOffset = 0;
                        break;
                    }
                    *indexPtr = indexPtr2;
		}
	    }
            TkTextFindDisplayLineEnd(textPtr, indexPtr, 0, NULL);

	    /*
	     * This call assumes indexPtr is the beginning of a display line
	     * and moves it to the 'xOffset' position of that line, which is
	     * just what we want.
	     */

	    TkTextIndexOfX(textPtr, xOffset, indexPtr);
	} else {
	    lineIndex = TkBTreeLinesTo(textPtr, indexPtr->linePtr);
	    if (*string == '+') {
		lineIndex += count;
	    } else {
		lineIndex -= count;

		/*
		 * The check below retains the character position, even if the
		 * line runs off the start of the file. Without it, the
		 * character position will get reset to 0 by TkTextMakeIndex.
		 */

		if (lineIndex < 0) {
		    lineIndex = 0;
		}
	    }

	    /*
	     * This doesn't work quite right if using a proportional font or
	     * UTF-8 characters with varying numbers of bytes, or if there are
	     * embedded windows, images, etc. The cursor will bop around,
	     * keeping a constant number of bytes (not characters) from the
	     * left edge (but making sure not to split any UTF-8 characters),
	     * regardless of the x-position the index corresponds to. The
	     * proper way to do this is to get the x-position of the index and
	     * then pick the character at the same x-position in the new line.
	     */

	    TkTextMakeByteIndex(indexPtr->tree, textPtr, lineIndex,
		    indexPtr->byteIndex, indexPtr);
	}
    } else {
	return NULL;
    }
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexForwBytes --
 *
 *	Given an index for a text widget, this function creates a new index
 *	that points "count" bytes ahead of the source index.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" bytes after
 *	srcPtr, or to the last character in the TkText if there aren't "count"
 *	bytes left.
 *
 *	In this latter case, the function returns '1' to indicate that not all
 *	of 'byteCount' could be used.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextIndexForwBytes(
    const TkText *textPtr,
    const TkTextIndex *srcPtr,	/* Source index. */
    int byteCount,		/* How many bytes forward to move. May be
				 * negative. */
    TkTextIndex *dstPtr)	/* Destination index: gets modified. */
{
    TkTextLine *linePtr;
    TkTextSegment *segPtr;
    int lineLength;

    if (byteCount < 0) {
	TkTextIndexBackBytes(textPtr, srcPtr, -byteCount, dstPtr);
	return 0;
    }

    *dstPtr = *srcPtr;
    dstPtr->byteIndex += byteCount;
    while (1) {
	/*
	 * Compute the length of the current line.
	 */

	lineLength = 0;
	for (segPtr = dstPtr->linePtr->segPtr; segPtr != NULL;
		segPtr = segPtr->nextPtr) {
	    lineLength += segPtr->size;
	}

	/*
	 * If the new index is in the same line then we're done. Otherwise go
	 * on to the next line.
	 */

	if (dstPtr->byteIndex < lineLength) {
	    return 0;
	}
	dstPtr->byteIndex -= lineLength;
	linePtr = TkBTreeNextLine(textPtr, dstPtr->linePtr);
	if (linePtr == NULL) {
	    dstPtr->byteIndex = lineLength - 1;
	    return 1;
	}
	dstPtr->linePtr = linePtr;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexForwChars --
 *
 *	Given an index for a text widget, this function creates a new index
 *	that points "count" items of type given by "type" ahead of the source
 *	index. "count" can be zero, which is useful in the case where one
 *	wishes to move forward by display (non-elided) chars or indices or one
 *	wishes to move forward by chars, skipping any intervening indices. In
 *	this case dstPtr will point to the first acceptable index which is
 *	encountered.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" items after
 *	srcPtr, or to the last character in the TkText if there aren't
 *	sufficient items left in the widget.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkTextIndexForwChars(
    const TkText *textPtr,	/* Overall information about text widget. */
    const TkTextIndex *srcPtr,	/* Source index. */
    int charCount,		/* How many characters forward to move. May
				 * be negative. */
    TkTextIndex *dstPtr,	/* Destination index: gets modified. */
    TkTextCountType type)	/* The type of item to count */
{
    TkTextLine *linePtr;
    TkTextSegment *segPtr;
    TkTextElideInfo *infoPtr = NULL;
    int byteOffset;
    char *start, *end, *p;
    int ch;
    int elide = 0;
    int checkElided = (type & COUNT_DISPLAY);

    if (charCount < 0) {
	TkTextIndexBackChars(textPtr, srcPtr, -charCount, dstPtr, type);
	return;
    }
    if (checkElided) {
	infoPtr = ckalloc(sizeof(TkTextElideInfo));
	elide = TkTextIsElided(textPtr, srcPtr, infoPtr);
    }

    *dstPtr = *srcPtr;

    /*
     * Find seg that contains src byteIndex. Move forward specified number of
     * chars.
     */

    if (checkElided) {
	/*
	 * In this case we have already calculated the information we need, so
	 * no need to use TkTextIndexToSeg()
	 */

	segPtr = infoPtr->segPtr;
	byteOffset = dstPtr->byteIndex - infoPtr->segOffset;
    } else {
	segPtr = TkTextIndexToSeg(dstPtr, &byteOffset);
    }

    while (1) {
	/*
	 * Go through each segment in line looking for specified character
	 * index.
	 */

	for ( ; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    /*
	     * If we do need to pay attention to the visibility of
	     * characters/indices, check that first. If the current segment
	     * isn't visible, then we simply continue the loop.
	     */

	    if (checkElided && ((segPtr->typePtr == &tkTextToggleOffType)
		    || (segPtr->typePtr == &tkTextToggleOnType))) {
		TkTextTag *tagPtr = segPtr->body.toggle.tagPtr;

		/*
		 * The elide state only changes if this tag is either the
		 * current highest priority tag (and is therefore being
		 * toggled off), or it's a new tag with higher priority.
		 */

		if (tagPtr->elideString != NULL) {
		    infoPtr->tagCnts[tagPtr->priority]++;
		    if (infoPtr->tagCnts[tagPtr->priority] & 1) {
			infoPtr->tagPtrs[tagPtr->priority] = tagPtr;
		    }

		    if (tagPtr->priority >= infoPtr->elidePriority) {
			if (segPtr->typePtr == &tkTextToggleOffType) {
			    /*
			     * If it is being toggled off, and it has an elide
			     * string, it must actually be the current highest
			     * priority tag, so this check is redundant:
			     */

			    if (tagPtr->priority != infoPtr->elidePriority) {
				Tcl_Panic("Bad tag priority being toggled off");
			    }

			    /*
			     * Find previous elide tag, if any (if not then
			     * elide will be zero, of course).
			     */

			    elide = 0;
			    while (--infoPtr->elidePriority > 0) {
				if (infoPtr->tagCnts[infoPtr->elidePriority]
					& 1) {
				    elide = infoPtr->tagPtrs
					    [infoPtr->elidePriority]->elide;
				    break;
				}
			    }
			} else {
			    elide = tagPtr->elide;
			    infoPtr->elidePriority = tagPtr->priority;
			}
		    }
		}
	    }

	    if (!elide) {
		if (segPtr->typePtr == &tkTextCharType) {
		    start = segPtr->body.chars + byteOffset;
		    end = segPtr->body.chars + segPtr->size;
		    for (p = start; p < end; p += TkUtfToUniChar(p, &ch)) {
			if (charCount == 0) {
			    dstPtr->byteIndex += (p - start);
			    goto forwardCharDone;
			}
			charCount--;
		    }
		} else if (type & COUNT_INDICES) {
		    if (charCount < segPtr->size - byteOffset) {
			dstPtr->byteIndex += charCount;
			goto forwardCharDone;
		    }
		    charCount -= segPtr->size - byteOffset;
		}
	    }

	    dstPtr->byteIndex += segPtr->size - byteOffset;
	    byteOffset = 0;
	}

	/*
	 * Go to the next line. If we are at the end of the text item, back up
	 * one byte (for the terminal '\n' character) and return that index.
	 */

	linePtr = TkBTreeNextLine(textPtr, dstPtr->linePtr);
	if (linePtr == NULL) {
	    dstPtr->byteIndex -= sizeof(char);
	    goto forwardCharDone;
	}
	dstPtr->linePtr = linePtr;
	dstPtr->byteIndex = 0;
	segPtr = dstPtr->linePtr->segPtr;
    }

  forwardCharDone:
    if (infoPtr != NULL) {
	TkTextFreeElideInfo(infoPtr);
	ckfree(infoPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexCountBytes --
 *
 *	Given a pair of indices in a text widget, this function counts how
 *	many bytes are between the two indices. The two indices do not need
 *	to be ordered.
 *
 * Results:
 *	The number of bytes in the given range.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextIndexCountBytes(
    const TkText *textPtr,
    const TkTextIndex *indexPtr1, /* Index describing one location. */
    const TkTextIndex *indexPtr2) /* Index describing second location. */
{
    int compare = TkTextIndexCmp(indexPtr1, indexPtr2);

    if (compare == 0) {
	return 0;
    } else if (compare > 0) {
	return IndexCountBytesOrdered(textPtr, indexPtr2, indexPtr1);
    } else {
	return IndexCountBytesOrdered(textPtr, indexPtr1, indexPtr2);
    }
}

static int
IndexCountBytesOrdered(
    const TkText *textPtr,
    const TkTextIndex *indexPtr1,
				/* Index describing location of character from
				 * which to count. */
    const TkTextIndex *indexPtr2)
				/* Index describing location of last character
				 * at which to stop the count. */
{
    int byteCount, offset;
    TkTextSegment *segPtr, *segPtr1;
    TkTextLine *linePtr;

    if (indexPtr1->linePtr == indexPtr2->linePtr) {
        return indexPtr2->byteIndex - indexPtr1->byteIndex;
    }

    /*
     * indexPtr2 is on a line strictly after the line containing indexPtr1.
     * Add up:
     *   bytes between indexPtr1 and end of its line
     *   bytes in lines strictly between indexPtr1 and indexPtr2
     *   bytes between start of the indexPtr2 line and indexPtr2
     */

    segPtr1 = TkTextIndexToSeg(indexPtr1, &offset);
    byteCount = -offset;
    for (segPtr = segPtr1; segPtr != NULL; segPtr = segPtr->nextPtr) {
        byteCount += segPtr->size;
    }

    linePtr = TkBTreeNextLine(textPtr, indexPtr1->linePtr);
    while (linePtr != indexPtr2->linePtr) {
	for (segPtr = linePtr->segPtr; segPtr != NULL;
                segPtr = segPtr->nextPtr) {
            byteCount += segPtr->size;
        }
        linePtr = TkBTreeNextLine(textPtr, linePtr);
        if (linePtr == NULL) {
            Tcl_Panic("TextIndexCountBytesOrdered ran out of lines");
        }
    }

    byteCount += indexPtr2->byteIndex;

    return byteCount;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexCount --
 *
 *	Given an ordered pair of indices in a text widget, this function
 *	counts how many characters (not bytes) are between the two indices.
 *
 *	It is illegal to call this function with unordered indices.
 *
 *	Note that 'textPtr' is only used if we need to check for elided
 *	attributes, i.e. if type is COUNT_DISPLAY_INDICES or
 *	COUNT_DISPLAY_CHARS.
 *
 * Results:
 *	The number of characters in the given range, which meet the
 *	appropriate 'type' attributes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextIndexCount(
    const TkText *textPtr,	/* Overall information about text widget. */
    const TkTextIndex *indexPtr1,
				/* Index describing location of character from
				 * which to count. */
    const TkTextIndex *indexPtr2,
				/* Index describing location of last character
				 * at which to stop the count. */
    TkTextCountType type)	/* The kind of indices to count. */
{
    TkTextLine *linePtr1;
    TkTextSegment *segPtr, *seg2Ptr = NULL;
    TkTextElideInfo *infoPtr = NULL;
    int byteOffset, maxBytes, count = 0, elide = 0;
    int checkElided = (type & COUNT_DISPLAY);

    /*
     * Find seg that contains src index, and remember how many bytes not to
     * count in the given segment.
     */

    segPtr = TkTextIndexToSeg(indexPtr1, &byteOffset);
    linePtr1 = indexPtr1->linePtr;

    seg2Ptr = TkTextIndexToSeg(indexPtr2, &maxBytes);

    if (checkElided) {
	infoPtr = ckalloc(sizeof(TkTextElideInfo));
	elide = TkTextIsElided(textPtr, indexPtr1, infoPtr);
    }

    while (1) {
	/*
	 * Go through each segment in line adding up the number of characters.
	 */

	for ( ; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    /*
	     * If we do need to pay attention to the visibility of
	     * characters/indices, check that first. If the current segment
	     * isn't visible, then we simply continue the loop.
	     */

	    if (checkElided) {
		if ((segPtr->typePtr == &tkTextToggleOffType)
			|| (segPtr->typePtr == &tkTextToggleOnType)) {
		    TkTextTag *tagPtr = segPtr->body.toggle.tagPtr;

		    /*
		     * The elide state only changes if this tag is either the
		     * current highest priority tag (and is therefore being
		     * toggled off), or it's a new tag with higher priority.
		     */

		    if (tagPtr->elideString != NULL) {
			infoPtr->tagCnts[tagPtr->priority]++;
			if (infoPtr->tagCnts[tagPtr->priority] & 1) {
			    infoPtr->tagPtrs[tagPtr->priority] = tagPtr;
			}
			if (tagPtr->priority >= infoPtr->elidePriority) {
			    if (segPtr->typePtr == &tkTextToggleOffType) {
				/*
				 * If it is being toggled off, and it has an
				 * elide string, it must actually be the
				 * current highest priority tag, so this check
				 * is redundant:
				 */

				if (tagPtr->priority!=infoPtr->elidePriority) {
				    Tcl_Panic("Bad tag priority being toggled off");
				}

				/*
				 * Find previous elide tag, if any (if not
				 * then elide will be zero, of course).
				 */

				elide = 0;
				while (--infoPtr->elidePriority > 0) {
				    if (infoPtr->tagCnts[
					    infoPtr->elidePriority] & 1) {
					elide = infoPtr->tagPtrs[
						infoPtr->elidePriority]->elide;
					break;
				    }
				}
			    } else {
				elide = tagPtr->elide;
				infoPtr->elidePriority = tagPtr->priority;
			    }
			}
		    }
		}
		if (elide) {
		    if (segPtr == seg2Ptr) {
			goto countDone;
		    }
		    byteOffset = 0;
		    continue;
		}
	    }

	    if (segPtr->typePtr == &tkTextCharType) {
		int byteLen = segPtr->size - byteOffset;
		register unsigned char *str = (unsigned char *)
			segPtr->body.chars + byteOffset;
		register int i;

		if (segPtr == seg2Ptr) {
		    if (byteLen > (maxBytes - byteOffset)) {
			byteLen = maxBytes - byteOffset;
		    }
		}
		i = byteLen;

		/*
		 * This is a speed sensitive function, so run specially over
		 * the string to count continuous ascii characters before
		 * resorting to the Tcl_NumUtfChars call. This is a long form
		 * of:
		 *
		 *   stringPtr->numChars =
		 *	     Tcl_NumUtfChars(objPtr->bytes, objPtr->length);
		 */

		while (i && (*str < 0xC0)) {
		    i--;
		    str++;
		}
		count += byteLen - i;
		if (i) {
		    count += Tcl_NumUtfChars(segPtr->body.chars + byteOffset
			    + (byteLen - i), i);
		}
	    } else {
		if (type & COUNT_INDICES) {
		    int byteLen = segPtr->size - byteOffset;

		    if (segPtr == seg2Ptr) {
			if (byteLen > (maxBytes - byteOffset)) {
			    byteLen = maxBytes - byteOffset;
			}
		    }
		    count += byteLen;
		}
	    }
	    if (segPtr == seg2Ptr) {
		goto countDone;
	    }
	    byteOffset = 0;
	}

	/*
	 * Go to the next line. If we are at the end of the text item, back up
	 * one byte (for the terminal '\n' character) and return that index.
	 */

	linePtr1 = TkBTreeNextLine(textPtr, linePtr1);
	if (linePtr1 == NULL) {
	    Tcl_Panic("Reached end of text widget when counting characters");
	}
	segPtr = linePtr1->segPtr;
    }

  countDone:
    if (infoPtr != NULL) {
	TkTextFreeElideInfo(infoPtr);
	ckfree(infoPtr);
    }
    return count;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexBackBytes --
 *
 *	Given an index for a text widget, this function creates a new index
 *	that points "count" bytes earlier than the source index.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" bytes before
 *	srcPtr, or to the first character in the TkText if there aren't
 *	"count" bytes earlier than srcPtr.
 *
 *	Returns 1 if we couldn't use all of 'byteCount' because we have run
 *	into the beginning or end of the text, and zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextIndexBackBytes(
    const TkText *textPtr,
    const TkTextIndex *srcPtr,	/* Source index. */
    int byteCount,		/* How many bytes backward to move. May be
				 * negative. */
    TkTextIndex *dstPtr)	/* Destination index: gets modified. */
{
    TkTextSegment *segPtr;
    int lineIndex;

    if (byteCount < 0) {
	return TkTextIndexForwBytes(textPtr, srcPtr, -byteCount, dstPtr);
    }

    *dstPtr = *srcPtr;
    dstPtr->byteIndex -= byteCount;
    lineIndex = -1;
    while (dstPtr->byteIndex < 0) {
	/*
	 * Move back one line in the text. If we run off the beginning of the
	 * file then just return the first character in the text.
	 */

	if (lineIndex < 0) {
	    lineIndex = TkBTreeLinesTo(textPtr, dstPtr->linePtr);
	}
	if (lineIndex == 0) {
	    dstPtr->byteIndex = 0;
	    return 1;
	}
	lineIndex--;
	dstPtr->linePtr = TkBTreeFindLine(dstPtr->tree, textPtr, lineIndex);

	/*
	 * Compute the length of the line and add that to dstPtr->charIndex.
	 */

	for (segPtr = dstPtr->linePtr->segPtr; segPtr != NULL;
		segPtr = segPtr->nextPtr) {
	    dstPtr->byteIndex += segPtr->size;
	}
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexBackChars --
 *
 *	Given an index for a text widget, this function creates a new index
 *	that points "count" items of type given by "type" earlier than the
 *	source index. "count" can be zero, which is useful in the case where
 *	one wishes to move backward by display (non-elided) chars or indices
 *	or one wishes to move backward by chars, skipping any intervening
 *	indices. In this case the returned index *dstPtr will point just
 *	_after_ the first acceptable index which is encountered.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" items before
 *	srcPtr, or to the first index in the window if there aren't sufficient
 *	items earlier than srcPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkTextIndexBackChars(
    const TkText *textPtr,	/* Overall information about text widget. */
    const TkTextIndex *srcPtr,	/* Source index. */
    int charCount,		/* How many characters backward to move. May
				 * be negative. */
    TkTextIndex *dstPtr,	/* Destination index: gets modified. */
    TkTextCountType type)	/* The type of item to count */
{
    TkTextSegment *segPtr, *oldPtr;
    TkTextElideInfo *infoPtr = NULL;
    int lineIndex, segSize;
    const char *p, *start, *end;
    int elide = 0;
    int checkElided = (type & COUNT_DISPLAY);

    if (charCount < 0) {
	TkTextIndexForwChars(textPtr, srcPtr, -charCount, dstPtr, type);
	return;
    }
    if (checkElided) {
	infoPtr = ckalloc(sizeof(TkTextElideInfo));
	elide = TkTextIsElided(textPtr, srcPtr, infoPtr);
    }

    *dstPtr = *srcPtr;

    /*
     * Find offset within seg that contains byteIndex. Move backward specified
     * number of chars.
     */

    lineIndex = -1;

    segSize = dstPtr->byteIndex;

    if (checkElided) {
	segPtr = infoPtr->segPtr;
	segSize -= infoPtr->segOffset;
    } else {
	TkTextLine *linePtr = dstPtr->linePtr;
	for (segPtr = linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	    if (segPtr == NULL) {
		/*
		 * Two logical lines merged into one display line through
		 * eliding of a newline.
		 */

		linePtr = TkBTreeNextLine(NULL, linePtr);
		segPtr = linePtr->segPtr;
	    }
	    if (segSize <= segPtr->size) {
		break;
	    }
	    segSize -= segPtr->size;
	}
    }

    /*
     * Now segPtr points to the segment containing the starting index.
     */

    while (1) {
	/*
	 * If we do need to pay attention to the visibility of
	 * characters/indices, check that first. If the current segment isn't
	 * visible, then we simply continue the loop.
	 */

	if (checkElided && ((segPtr->typePtr == &tkTextToggleOffType)
		|| (segPtr->typePtr == &tkTextToggleOnType))) {
	    TkTextTag *tagPtr = segPtr->body.toggle.tagPtr;

	    /*
	     * The elide state only changes if this tag is either the current
	     * highest priority tag (and is therefore being toggled off), or
	     * it's a new tag with higher priority.
	     */

	    if (tagPtr->elideString != NULL) {
		infoPtr->tagCnts[tagPtr->priority]++;
		if (infoPtr->tagCnts[tagPtr->priority] & 1) {
		    infoPtr->tagPtrs[tagPtr->priority] = tagPtr;
		}
		if (tagPtr->priority >= infoPtr->elidePriority) {
		    if (segPtr->typePtr == &tkTextToggleOnType) {
			/*
			 * If it is being toggled on, and it has an elide
			 * string, it must actually be the current highest
			 * priority tag, so this check is redundant:
			 */

			if (tagPtr->priority != infoPtr->elidePriority) {
			    Tcl_Panic("Bad tag priority being toggled on");
			}

			/*
			 * Find previous elide tag, if any (if not then elide
			 * will be zero, of course).
			 */

			elide = 0;
			while (--infoPtr->elidePriority > 0) {
			    if (infoPtr->tagCnts[infoPtr->elidePriority] & 1) {
				elide = infoPtr->tagPtrs[
					infoPtr->elidePriority]->elide;
				break;
			    }
			}
		    } else {
			elide = tagPtr->elide;
			infoPtr->elidePriority = tagPtr->priority;
		    }
		}
	    }
	}

	if (!elide) {
	    if (segPtr->typePtr == &tkTextCharType) {
		start = segPtr->body.chars;
		end = segPtr->body.chars + segSize;
		for (p = end; ; p = Tcl_UtfPrev(p, start)) {
		    if (charCount == 0) {
			dstPtr->byteIndex -= (end - p);
			goto backwardCharDone;
		    }
		    if (p == start) {
			break;
		    }
		    charCount--;
		}
	    } else {
		if (type & COUNT_INDICES) {
		    if (charCount <= segSize) {
			dstPtr->byteIndex -= charCount;
			goto backwardCharDone;
		    }
		    charCount -= segSize;
		}
	    }
	}
	dstPtr->byteIndex -= segSize;

	/*
	 * Move back into previous segment.
	 */

	oldPtr = segPtr;
	segPtr = dstPtr->linePtr->segPtr;
	if (segPtr != oldPtr) {
	    for ( ; segPtr->nextPtr != oldPtr; segPtr = segPtr->nextPtr) {
		/* Empty body. */
	    }
	    segSize = segPtr->size;
	    continue;
	}

	/*
	 * Move back to previous line.
	 */

	if (lineIndex < 0) {
	    lineIndex = TkBTreeLinesTo(textPtr, dstPtr->linePtr);
	}
	if (lineIndex == 0) {
	    dstPtr->byteIndex = 0;
	    goto backwardCharDone;
	}
	lineIndex--;
	dstPtr->linePtr = TkBTreeFindLine(dstPtr->tree, textPtr, lineIndex);

	/*
	 * Compute the length of the line and add that to dstPtr->byteIndex.
	 */

	oldPtr = dstPtr->linePtr->segPtr;
	for (segPtr = oldPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    dstPtr->byteIndex += segPtr->size;
	    oldPtr = segPtr;
	}
	segPtr = oldPtr;
	segSize = segPtr->size;
    }

  backwardCharDone:
    if (infoPtr != NULL) {
	TkTextFreeElideInfo(infoPtr);
	ckfree(infoPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StartEnd --
 *
 *	This function handles modifiers like "wordstart" and "lineend" to
 *	adjust indices forwards or backwards.
 *
 * Results:
 *	If the modifier is successfully parsed then the return value is the
 *	address of the first character after the modifier, and *indexPtr is
 *	updated to reflect the modifier. If there is a syntax error in the
 *	modifier then NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static const char *
StartEnd(
    TkText *textPtr,		/* Information about text widget. */
    const char *string,		/* String to parse for additional info about
				 * modifier (count and units). Points to first
				 * character of modifier word. */
    TkTextIndex *indexPtr)	/* Index to modify based on string. */
{
    const char *p;
    size_t length;
    register TkTextSegment *segPtr;
    int modifier;

    /*
     * Find the end of the modifier word.
     */

    for (p = string; isalnum(UCHAR(*p)); p++) {
	/* Empty loop body. */
    }

    length = p-string;
    if ((*string == 'd') &&
	    (strncmp(string, "display", (length > 7 ? 7 : length)) == 0)) {
	modifier = TKINDEX_DISPLAY;
	if (length > 7) {
	    p -= (length - 7);
	}
    } else if ((*string == 'a') &&
	    (strncmp(string, "any", (length > 3 ? 3 : length)) == 0)) {
	modifier = TKINDEX_ANY;
	if (length > 3) {
	    p -= (length - 3);
	}
    } else {
	modifier = TKINDEX_NONE;
    }

    /*
     * If we had a modifier, which we interpreted ok, so now forward to the
     * actual units.
     */

    if (modifier != TKINDEX_NONE) {
	while (isspace(UCHAR(*p))) {
	    p++;
	}
	string = p;
	while ((*p!='\0') && !isspace(UCHAR(*p)) && (*p!='+') && (*p!='-')) {
	    p++;
	}
	length = p - string;
    }

    if ((*string == 'l') && (strncmp(string, "lineend", length) == 0)
	    && (length >= 5)) {
	if (modifier == TKINDEX_DISPLAY) {
	    TkTextFindDisplayLineEnd(textPtr, indexPtr, 1, NULL);
	} else {
	    indexPtr->byteIndex = 0;
	    for (segPtr = indexPtr->linePtr->segPtr; segPtr != NULL;
		    segPtr = segPtr->nextPtr) {
		indexPtr->byteIndex += segPtr->size;
	    }

	    /*
	     * We know '\n' is encoded with a single byte index.
	     */

	    indexPtr->byteIndex -= sizeof(char);
	}
    } else if ((*string == 'l') && (strncmp(string, "linestart", length) == 0)
	    && (length >= 5)) {
	if (modifier == TKINDEX_DISPLAY) {
	    TkTextFindDisplayLineEnd(textPtr, indexPtr, 0, NULL);
	} else {
	    indexPtr->byteIndex = 0;
	}
    } else if ((*string == 'w') && (strncmp(string, "wordend", length) == 0)
	    && (length >= 5)) {
	int firstChar = 1;
	int offset;

	/*
	 * If the current character isn't part of a word then just move
	 * forward one character. Otherwise move forward until finding a
	 * character that isn't part of a word and stop there.
	 */

	if (modifier == TKINDEX_DISPLAY) {
	    TkTextIndexForwChars(textPtr, indexPtr, 0, indexPtr,
		    COUNT_DISPLAY_INDICES);
	}
	segPtr = TkTextIndexToSeg(indexPtr, &offset);
	while (1) {
	    int chSize = 1;

	    if (segPtr->typePtr == &tkTextCharType) {
		int ch;

		chSize = TkUtfToUniChar(segPtr->body.chars + offset, &ch);
		if (!Tcl_UniCharIsWordChar(ch)) {
		    break;
		}
		firstChar = 0;
	    }
	    offset += chSize;
	    indexPtr->byteIndex += chSize;
	    if (offset >= segPtr->size) {
		segPtr = TkTextIndexToSeg(indexPtr, &offset);
	    }
	}
	if (firstChar) {
	    if (modifier == TKINDEX_DISPLAY) {
		TkTextIndexForwChars(textPtr, indexPtr, 1, indexPtr,
			COUNT_DISPLAY_INDICES);
	    } else {
		TkTextIndexForwChars(NULL, indexPtr, 1, indexPtr,
			COUNT_INDICES);
	    }
	}
    } else if ((*string == 'w') && (strncmp(string, "wordstart", length) == 0)
	    && (length >= 5)) {
	int firstChar = 1;
	int offset;

	if (modifier == TKINDEX_DISPLAY) {
	    TkTextIndexForwChars(textPtr, indexPtr, 0, indexPtr,
		    COUNT_DISPLAY_INDICES);
	}

	/*
	 * Starting with the current character, look for one that's not part
	 * of a word and keep moving backward until you find one. Then if the
	 * character found wasn't the first one, move forward again one
	 * position.
	 */

	segPtr = TkTextIndexToSeg(indexPtr, &offset);
	while (1) {
	    int chSize = 1;

	    if (segPtr->typePtr == &tkTextCharType) {

		int ch;
		TkUtfToUniChar(segPtr->body.chars + offset, &ch);
		if (!Tcl_UniCharIsWordChar(ch)) {
		    break;
		}
		if (offset > 0) {
		    chSize = (segPtr->body.chars + offset
			    - Tcl_UtfPrev(segPtr->body.chars + offset,
			    segPtr->body.chars));
		}
		firstChar = 0;
	    }
            if (offset == 0) {
                if (modifier == TKINDEX_DISPLAY) {
                    TkTextIndexBackChars(textPtr, indexPtr, 1, indexPtr,
                        COUNT_DISPLAY_INDICES);
                } else {
                    TkTextIndexBackChars(NULL, indexPtr, 1, indexPtr,
                        COUNT_INDICES);
                }
            } else {
                indexPtr->byteIndex -= chSize;
            }
            offset -= chSize;
	    if (offset < 0) {
		if (indexPtr->byteIndex == 0) {
		    goto done;
		}
		segPtr = TkTextIndexToSeg(indexPtr, &offset);
	    }
	}

	if (!firstChar) {
	    if (modifier == TKINDEX_DISPLAY) {
		TkTextIndexForwChars(textPtr, indexPtr, 1, indexPtr,
			COUNT_DISPLAY_INDICES);
	    } else {
		TkTextIndexForwChars(NULL, indexPtr, 1, indexPtr,
			COUNT_INDICES);
	    }
	}
    } else {
	return NULL;
    }

  done:
    return p;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
