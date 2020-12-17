/*
 * tkCursor.c --
 *
 *	This file maintains a database of read-only cursors for the Tk
 *	toolkit. This allows cursors to be shared between widgets and also
 *	avoids round-trips to the X server.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * A TkCursor structure exists for each cursor that is currently active. Each
 * structure is indexed with two hash tables defined below. One of the tables
 * is cursorIdTable, and the other is either cursorNameTable or
 * cursorDataTable, each of which are stored in the TkDisplay structure for
 * the current thread.
 */

typedef struct {
    const char *source;		/* Cursor bits. */
    const char *mask;		/* Mask bits. */
    int width, height;		/* Dimensions of cursor (and data and
				 * mask). */
    int xHot, yHot;		/* Location of cursor hot-spot. */
    Tk_Uid fg, bg;		/* Colors for cursor. */
    Display *display;		/* Display on which cursor will be used. */
} DataKey;

/*
 * Forward declarations for functions defined in this file:
 */

static void		CursorInit(TkDisplay *dispPtr);
static void		DupCursorObjProc(Tcl_Obj *srcObjPtr,
			    Tcl_Obj *dupObjPtr);
static void		FreeCursor(TkCursor *cursorPtr);
static void		FreeCursorObj(Tcl_Obj *objPtr);
static void		FreeCursorObjProc(Tcl_Obj *objPtr);
static TkCursor *	TkcGetCursor(Tcl_Interp *interp, Tk_Window tkwin,
			    const char *name);
static TkCursor *	GetCursorFromObj(Tk_Window tkwin, Tcl_Obj *objPtr);
static void		InitCursorObj(Tcl_Obj *objPtr);

/*
 * The following structure defines the implementation of the "cursor" Tcl
 * object, used for drawing. The color object remembers the hash table
 * entry associated with a color. The actual allocation and deallocation
 * of the color should be done by the configuration package when the cursor
 * option is set.
 */

Tcl_ObjType const tkCursorObjType = {
    "cursor",			/* name */
    FreeCursorObjProc,		/* freeIntRepProc */
    DupCursorObjProc,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

/*
 *----------------------------------------------------------------------
 *
 * Tk_AllocCursorFromObj --
 *
 *	Given a Tcl_Obj *, map the value to a corresponding Tk_Cursor
 *	structure based on the tkwin given.
 *
 * Results:
 *	The return value is the X identifer for the desired cursor, unless
 *	objPtr couldn't be parsed correctly. In this case, None is returned
 *	and an error message is left in the interp's result. The caller should
 *	never modify the cursor that is returned, and should eventually call
 *	Tk_FreeCursorFromObj when the cursor is no longer needed.
 *
 * Side effects:
 *	The cursor is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeCursorFromObj, so that the database can be cleaned up when
 *	cursors aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

Tk_Cursor
Tk_AllocCursorFromObj(
    Tcl_Interp *interp,		/* Interp for error results. */
    Tk_Window tkwin,		/* Window in which the cursor will be used.*/
    Tcl_Obj *objPtr)		/* Object describing cursor; see manual entry
				 * for description of legal syntax of this
				 * obj's string rep. */
{
    TkCursor *cursorPtr;

    if (objPtr->typePtr != &tkCursorObjType) {
	InitCursorObj(objPtr);
    }
    cursorPtr = objPtr->internalRep.twoPtrValue.ptr1;

    /*
     * If the object currently points to a TkCursor, see if it's the one we
     * want. If so, increment its reference count and return.
     */

    if (cursorPtr != NULL) {
	if (cursorPtr->resourceRefCount == 0) {
	    /*
	     * This is a stale reference: it refers to a TkCursor that's no
	     * longer in use. Clear the reference.
	     */

	    FreeCursorObj(objPtr);
	    cursorPtr = NULL;
	} else if (Tk_Display(tkwin) == cursorPtr->display) {
	    cursorPtr->resourceRefCount++;
	    return cursorPtr->cursor;
	}
    }

    /*
     * The object didn't point to the TkCursor that we wanted. Search the list
     * of TkCursors with the same name to see if one of the other TkCursors is
     * the right one.
     */

    if (cursorPtr != NULL) {
	TkCursor *firstCursorPtr = Tcl_GetHashValue(cursorPtr->hashPtr);

	FreeCursorObj(objPtr);
	for (cursorPtr = firstCursorPtr;  cursorPtr != NULL;
		cursorPtr = cursorPtr->nextPtr) {
	    if (Tk_Display(tkwin) == cursorPtr->display) {
		cursorPtr->resourceRefCount++;
		cursorPtr->objRefCount++;
		objPtr->internalRep.twoPtrValue.ptr1 = cursorPtr;
		return cursorPtr->cursor;
	    }
	}
    }

    /*
     * Still no luck. Call TkcGetCursor to allocate a new TkCursor object.
     */

    cursorPtr = TkcGetCursor(interp, tkwin, Tcl_GetString(objPtr));
    objPtr->internalRep.twoPtrValue.ptr1 = cursorPtr;
    if (cursorPtr == NULL) {
	return None;
    }
    cursorPtr->objRefCount++;
    return cursorPtr->cursor;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetCursor --
 *
 *	Given a string describing a cursor, locate (or create if necessary) a
 *	cursor that fits the description.
 *
 * Results:
 *	The return value is the X identifer for the desired cursor, unless
 *	string couldn't be parsed correctly. In this case, None is returned
 *	and an error message is left in the interp's result. The caller should
 *	never modify the cursor that is returned, and should eventually call
 *	Tk_FreeCursor when the cursor is no longer needed.
 *
 * Side effects:
 *	The cursor is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeCursor, so that the database can be cleaned up when cursors
 *	aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

Tk_Cursor
Tk_GetCursor(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window in which cursor will be used. */
    Tk_Uid string)		/* Description of cursor. See manual entry for
				 * details on legal syntax. */
{
    TkCursor *cursorPtr = TkcGetCursor(interp, tkwin, string);

    if (cursorPtr == NULL) {
	return None;
    }
    return cursorPtr->cursor;
}

/*
 *----------------------------------------------------------------------
 *
 * TkcGetCursor --
 *
 *	Given a string describing a cursor, locate (or create if necessary) a
 *	cursor that fits the description. This routine returns the internal
 *	data structure for the cursor, which avoids extra hash table lookups
 *	in Tk_AllocCursorFromObj.
 *
 * Results:
 *	The return value is a pointer to the TkCursor for the desired cursor,
 *	unless string couldn't be parsed correctly. In this case, NULL is
 *	returned and an error message is left in the interp's result. The
 *	caller should never modify the cursor that is returned, and should
 *	eventually call Tk_FreeCursor when the cursor is no longer needed.
 *
 * Side effects:
 *	The cursor is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeCursor, so that the database can be cleaned up when cursors
 *	aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

static TkCursor *
TkcGetCursor(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window in which cursor will be used. */
    const char *string)		/* Description of cursor. See manual entry for
				 * details on legal syntax. */
{
    Tcl_HashEntry *nameHashPtr;
    register TkCursor *cursorPtr;
    TkCursor *existingCursorPtr = NULL;
    int isNew;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (!dispPtr->cursorInit) {
	CursorInit(dispPtr);
    }

    nameHashPtr = Tcl_CreateHashEntry(&dispPtr->cursorNameTable,
	    string, &isNew);
    if (!isNew) {
	existingCursorPtr = Tcl_GetHashValue(nameHashPtr);
	for (cursorPtr = existingCursorPtr; cursorPtr != NULL;
		cursorPtr = cursorPtr->nextPtr) {
	    if (Tk_Display(tkwin) == cursorPtr->display) {
		cursorPtr->resourceRefCount++;
		return cursorPtr;
	    }
	}
    } else {
	existingCursorPtr = NULL;
    }

    cursorPtr = TkGetCursorByName(interp, tkwin, string);

    if (cursorPtr == NULL) {
	if (isNew) {
	    Tcl_DeleteHashEntry(nameHashPtr);
	}
	return NULL;
    }

    /*
     * Add information about this cursor to our database.
     */

    cursorPtr->display = Tk_Display(tkwin);
    cursorPtr->resourceRefCount = 1;
    cursorPtr->objRefCount = 0;
    cursorPtr->otherTable = &dispPtr->cursorNameTable;
    cursorPtr->hashPtr = nameHashPtr;
    cursorPtr->nextPtr = existingCursorPtr;
    cursorPtr->idHashPtr = Tcl_CreateHashEntry(&dispPtr->cursorIdTable,
	    (char *) cursorPtr->cursor, &isNew);
    if (!isNew) {
	Tcl_Panic("cursor already registered in Tk_GetCursor");
    }
    Tcl_SetHashValue(nameHashPtr, cursorPtr);
    Tcl_SetHashValue(cursorPtr->idHashPtr, cursorPtr);

    return cursorPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetCursorFromData --
 *
 *	Given a description of the bits and colors for a cursor, make a cursor
 *	that has the given properties.
 *
 * Results:
 *	The return value is the X identifer for the desired cursor, unless it
 *	couldn't be created properly. In this case, None is returned and an
 *	error message is left in the interp's result. The caller should never
 *	modify the cursor that is returned, and should eventually call
 *	Tk_FreeCursor when the cursor is no longer needed.
 *
 * Side effects:
 *	The cursor is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeCursor, so that the database can be cleaned up when cursors
 *	aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

Tk_Cursor
Tk_GetCursorFromData(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window in which cursor will be used. */
    const char *source,		/* Bitmap data for cursor shape. */
    const char *mask,		/* Bitmap data for cursor mask. */
    int width, int height,	/* Dimensions of cursor. */
    int xHot, int yHot,		/* Location of hot-spot in cursor. */
    Tk_Uid fg,			/* Foreground color for cursor. */
    Tk_Uid bg)			/* Background color for cursor. */
{
    DataKey dataKey;
    Tcl_HashEntry *dataHashPtr;
    register TkCursor *cursorPtr;
    int isNew;
    XColor fgColor, bgColor;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (!dispPtr->cursorInit) {
	CursorInit(dispPtr);
    }

    dataKey.source = source;
    dataKey.mask = mask;
    dataKey.width = width;
    dataKey.height = height;
    dataKey.xHot = xHot;
    dataKey.yHot = yHot;
    dataKey.fg = fg;
    dataKey.bg = bg;
    dataKey.display = Tk_Display(tkwin);
    dataHashPtr = Tcl_CreateHashEntry(&dispPtr->cursorDataTable,
	    (char *) &dataKey, &isNew);
    if (!isNew) {
	cursorPtr = Tcl_GetHashValue(dataHashPtr);
	cursorPtr->resourceRefCount++;
	return cursorPtr->cursor;
    }

    /*
     * No suitable cursor exists yet. Make one using the data available and
     * add it to the database.
     */

    if (TkParseColor(dataKey.display, Tk_Colormap(tkwin), fg, &fgColor) == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"invalid color name \"%s\"", fg));
	Tcl_SetErrorCode(interp, "TK", "VALUE", "CURSOR", "COLOR", NULL);
	goto error;
    }
    if (TkParseColor(dataKey.display, Tk_Colormap(tkwin), bg, &bgColor) == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"invalid color name \"%s\"", bg));
	Tcl_SetErrorCode(interp, "TK", "VALUE", "CURSOR", "COLOR", NULL);
	goto error;
    }

    cursorPtr = TkCreateCursorFromData(tkwin, source, mask, width, height,
	    xHot, yHot, fgColor, bgColor);

    if (cursorPtr == NULL) {
	goto error;
    }

    cursorPtr->resourceRefCount = 1;
    cursorPtr->otherTable = &dispPtr->cursorDataTable;
    cursorPtr->hashPtr = dataHashPtr;
    cursorPtr->objRefCount = 0;
    cursorPtr->idHashPtr = Tcl_CreateHashEntry(&dispPtr->cursorIdTable,
	    (char *) cursorPtr->cursor, &isNew);
    cursorPtr->nextPtr = NULL;

    if (!isNew) {
	Tcl_Panic("cursor already registered in Tk_GetCursorFromData");
    }
    Tcl_SetHashValue(dataHashPtr, cursorPtr);
    Tcl_SetHashValue(cursorPtr->idHashPtr, cursorPtr);
    return cursorPtr->cursor;

  error:
    Tcl_DeleteHashEntry(dataHashPtr);
    return None;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_NameOfCursor --
 *
 *	Given a cursor, return a textual string identifying it.
 *
 * Results:
 *	If cursor was created by Tk_GetCursor, then the return value is the
 *	"string" that was used to create it. Otherwise the return value is a
 *	string giving the X identifier for the cursor. The storage for the
 *	returned string is only guaranteed to persist up until the next call
 *	to this function.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

const char *
Tk_NameOfCursor(
    Display *display,		/* Display for which cursor was allocated. */
    Tk_Cursor cursor)		/* Identifier for cursor whose name is
				 * wanted. */
{
    Tcl_HashEntry *idHashPtr;
    TkCursor *cursorPtr;
    TkDisplay *dispPtr;

    dispPtr = TkGetDisplay(display);

    if (!dispPtr->cursorInit) {
    printid:
	sprintf(dispPtr->cursorString, "cursor id %p", cursor);
	return dispPtr->cursorString;
    }
    idHashPtr = Tcl_FindHashEntry(&dispPtr->cursorIdTable, (char *) cursor);
    if (idHashPtr == NULL) {
	goto printid;
    }
    cursorPtr = Tcl_GetHashValue(idHashPtr);
    if (cursorPtr->otherTable != &dispPtr->cursorNameTable) {
	goto printid;
    }
    return cursorPtr->hashPtr->key.string;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeCursor --
 *
 *	This function is invoked by both Tk_FreeCursorFromObj and
 *	Tk_FreeCursor; it does all the real work of deallocating a cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with cursor is decremented, and it is
 *	officially deallocated if no-one is using it anymore.
 *
 *----------------------------------------------------------------------
 */

static void
FreeCursor(
    TkCursor *cursorPtr)	/* Cursor to be released. */
{
    TkCursor *prevPtr;

    cursorPtr->resourceRefCount--;
    if (cursorPtr->resourceRefCount > 0) {
	return;
    }

    Tcl_DeleteHashEntry(cursorPtr->idHashPtr);
    prevPtr = Tcl_GetHashValue(cursorPtr->hashPtr);
    if (prevPtr == cursorPtr) {
	if (cursorPtr->nextPtr == NULL) {
	    Tcl_DeleteHashEntry(cursorPtr->hashPtr);
	} else {
	    Tcl_SetHashValue(cursorPtr->hashPtr, cursorPtr->nextPtr);
	}
    } else {
	while (prevPtr->nextPtr != cursorPtr) {
	    prevPtr = prevPtr->nextPtr;
	}
	prevPtr->nextPtr = cursorPtr->nextPtr;
    }
    TkpFreeCursor(cursorPtr);
    if (cursorPtr->objRefCount == 0) {
	ckfree(cursorPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeCursor --
 *
 *	This function is called to release a cursor allocated by Tk_GetCursor
 *	or TkGetCursorFromData.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with cursor is decremented, and it is
 *	officially deallocated if no-one is using it anymore.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeCursor(
    Display *display,		/* Display for which cursor was allocated. */
    Tk_Cursor cursor)		/* Identifier for cursor to be released. */
{
    Tcl_HashEntry *idHashPtr;
    TkDisplay *dispPtr = TkGetDisplay(display);

    if (!dispPtr->cursorInit) {
	Tcl_Panic("Tk_FreeCursor called before Tk_GetCursor");
    }

    idHashPtr = Tcl_FindHashEntry(&dispPtr->cursorIdTable, (char *) cursor);
    if (idHashPtr == NULL) {
	Tcl_Panic("Tk_FreeCursor received unknown cursor argument");
    }
    FreeCursor(Tcl_GetHashValue(idHashPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeCursorFromObj --
 *
 *	This function is called to release a cursor allocated by
 *	Tk_AllocCursorFromObj. It does not throw away the Tcl_Obj *; it only
 *	gets rid of the hash table entry for this cursor and clears the cached
 *	value that is normally stored in the object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the cursor represented by objPtr
 *	is decremented, and the cursor is released to X if there are no
 *	remaining uses for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeCursorFromObj(
    Tk_Window tkwin,		/* The window this cursor lives in. Needed for
				 * the display value. */
    Tcl_Obj *objPtr)		/* The Tcl_Obj * to be freed. */
{
    FreeCursor(GetCursorFromObj(tkwin, objPtr));
    FreeCursorObj(objPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeCursorObjProc, FreeCursorObj --
 *
 *	This proc is called to release an object reference to a cursor.
 *	Called when the object's internal rep is released or when the cached
 *	tkColPtr needs to be changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object reference count is decremented. When both it and the hash
 *	ref count go to zero, the color's resources are released.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeCursorObjProc(
    Tcl_Obj *objPtr)		/* The object we are releasing. */
{
    FreeCursorObj(objPtr);
    objPtr->typePtr = NULL;
}

static void
FreeCursorObj(
    Tcl_Obj *objPtr)		/* The object we are releasing. */
{
    TkCursor *cursorPtr = objPtr->internalRep.twoPtrValue.ptr1;

    if (cursorPtr != NULL) {
	cursorPtr->objRefCount--;
	if ((cursorPtr->objRefCount == 0)
		&& (cursorPtr->resourceRefCount == 0)) {
	    ckfree(cursorPtr);
	}
	objPtr->internalRep.twoPtrValue.ptr1 = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DupCursorObjProc --
 *
 *	When a cached cursor object is duplicated, this is called to update
 *	the internal reps.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The color's objRefCount is incremented and the internal rep of the
 *	copy is set to point to it.
 *
 *---------------------------------------------------------------------------
 */

static void
DupCursorObjProc(
    Tcl_Obj *srcObjPtr,		/* The object we are copying from. */
    Tcl_Obj *dupObjPtr)		/* The object we are copying to. */
{
    TkCursor *cursorPtr = srcObjPtr->internalRep.twoPtrValue.ptr1;

    dupObjPtr->typePtr = srcObjPtr->typePtr;
    dupObjPtr->internalRep.twoPtrValue.ptr1 = cursorPtr;

    if (cursorPtr != NULL) {
	cursorPtr->objRefCount++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetCursorFromObj --
 *
 *	Returns the cursor referred to buy a Tcl object. The cursor must
 *	already have been allocated via a call to Tk_AllocCursorFromObj or
 *	Tk_GetCursor.
 *
 * Results:
 *	Returns the Tk_Cursor that matches the tkwin and the string rep of the
 *	name of the cursor given in objPtr.
 *
 * Side effects:
 *	If the object is not already a cursor, the conversion will free any
 *	old internal representation.
 *
 *----------------------------------------------------------------------
 */

Tk_Cursor
Tk_GetCursorFromObj(
    Tk_Window tkwin,
    Tcl_Obj *objPtr)		/* The object from which to get pixels. */
{
    TkCursor *cursorPtr = GetCursorFromObj(tkwin, objPtr);

    /*
     * GetCursorFromObj should never return NULL
     */

    return cursorPtr->cursor;
}

/*
 *----------------------------------------------------------------------
 *
 * GetCursorFromObj --
 *
 *	Returns the cursor referred to by a Tcl object. The cursor must
 *	already have been allocated via a call to Tk_AllocCursorFromObj or
 *	Tk_GetCursor.
 *
 * Results:
 *	Returns the TkCursor * that matches the tkwin and the string rep of
 *	the name of the cursor given in objPtr.
 *
 * Side effects:
 *	If the object is not already a cursor, the conversion will free any
 *	old internal representation.
 *
 *----------------------------------------------------------------------
 */

static TkCursor *
GetCursorFromObj(
    Tk_Window tkwin,		/* Window in which the cursor will be used. */
    Tcl_Obj *objPtr)		/* The object that describes the desired
				 * cursor. */
{
    TkCursor *cursorPtr;
    Tcl_HashEntry *hashPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (objPtr->typePtr != &tkCursorObjType) {
	InitCursorObj(objPtr);
    }

    /*
     * The internal representation is a cache of the last cursor used with the
     * given name. But there can be lots different cursors for each cursor
     * name; one cursor for each display. Check to see if the cursor we have
     * cached is the one that is needed.
     */

    cursorPtr = objPtr->internalRep.twoPtrValue.ptr1;
    if ((cursorPtr != NULL) && (Tk_Display(tkwin) == cursorPtr->display)) {
	return cursorPtr;
    }

    /*
     * If we get to here, it means the cursor we need is not in the cache.
     * Try to look up the cursor in the TkDisplay structure of the window.
     */

    hashPtr = Tcl_FindHashEntry(&dispPtr->cursorNameTable,
	    Tcl_GetString(objPtr));
    if (hashPtr == NULL) {
	goto error;
    }
    for (cursorPtr = Tcl_GetHashValue(hashPtr);
	    cursorPtr != NULL; cursorPtr = cursorPtr->nextPtr) {
	if (Tk_Display(tkwin) == cursorPtr->display) {
	    FreeCursorObj(objPtr);
	    objPtr->internalRep.twoPtrValue.ptr1 = cursorPtr;
	    cursorPtr->objRefCount++;
	    return cursorPtr;
	}
    }

  error:
    Tcl_Panic("GetCursorFromObj called with non-existent cursor!");
    /*
     * The following code isn't reached; it's just there to please compilers.
     */
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * InitCursorObj --
 *
 *	Bookeeping function to change an objPtr to a cursor type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The old internal rep of the object is freed. The internal rep is
 *	cleared. The final form of the object is set by either
 *	Tk_AllocCursorFromObj or GetCursorFromObj.
 *
 *----------------------------------------------------------------------
 */

static void
InitCursorObj(
    Tcl_Obj *objPtr)		/* The object to convert. */
{
    const Tcl_ObjType *typePtr;

    /*
     * Free the old internalRep before setting the new one.
     */

    Tcl_GetString(objPtr);
    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	typePtr->freeIntRepProc(objPtr);
    }
    objPtr->typePtr = &tkCursorObjType;
    objPtr->internalRep.twoPtrValue.ptr1 = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * CursorInit --
 *
 *	Initialize the structures used for cursor management.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read the code.
 *
 *----------------------------------------------------------------------
 */

static void
CursorInit(
    TkDisplay *dispPtr)		/* Display used to store thread-specific
				 * data. */
{
    Tcl_InitHashTable(&dispPtr->cursorNameTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&dispPtr->cursorDataTable, sizeof(DataKey)/sizeof(int));

    /*
     * The call below is tricky: can't use sizeof(IdKey) because it gets
     * padded with extra unpredictable bytes on some 64-bit machines.
     */

    /*
     * Old code....
     *     Tcl_InitHashTable(&dispPtr->cursorIdTable, sizeof(Display *)
     *                       /sizeof(int));
     *
     * The comment above doesn't make sense. However, XIDs should only be 32
     * bits, by the definition of X, so the code above causes Tk to crash.
     * Here is the real code:
     */

    Tcl_InitHashTable(&dispPtr->cursorIdTable, TCL_ONE_WORD_KEYS);

    dispPtr->cursorInit = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDebugCursor --
 *
 *	This function returns debugging information about a cursor.
 *
 * Results:
 *	The return value is a list with one sublist for each TkCursor
 *	corresponding to "name". Each sublist has two elements that contain
 *	the resourceRefCount and objRefCount fields from the TkCursor
 *	structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkDebugCursor(
    Tk_Window tkwin,		/* The window in which the cursor will be used
				 * (not currently used). */
    const char *name)		/* Name of the desired color. */
{
    TkCursor *cursorPtr;
    Tcl_HashEntry *hashPtr;
    Tcl_Obj *resultPtr, *objPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (!dispPtr->cursorInit) {
	CursorInit(dispPtr);
    }
    resultPtr = Tcl_NewObj();
    hashPtr = Tcl_FindHashEntry(&dispPtr->cursorNameTable, name);
    if (hashPtr != NULL) {
	cursorPtr = Tcl_GetHashValue(hashPtr);
	if (cursorPtr == NULL) {
	    Tcl_Panic("TkDebugCursor found empty hash table entry");
	}
	for ( ; (cursorPtr != NULL); cursorPtr = cursorPtr->nextPtr) {
	    objPtr = Tcl_NewObj();
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(cursorPtr->resourceRefCount));
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(cursorPtr->objRefCount));
	    Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
	}
    }
    return resultPtr;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
