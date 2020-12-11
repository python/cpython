/*
 * tkBitmap.c --
 *
 *	This file maintains a database of read-only bitmaps for the Tk
 *	toolkit. This allows bitmaps to be shared between widgets and also
 *	avoids interactions with the X server.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * The includes below are for pre-defined bitmaps.
 *
 * Platform-specific issue: Windows complains when the bitmaps are included,
 * because an array of characters is being initialized with integers as
 * elements. For lint purposes, the following pragmas temporarily turn off
 * that warning message.
 */

#if defined(_MSC_VER)
#pragma warning (disable : 4305)
#endif

#include "error.xbm"
#include "gray12.xbm"
#include "gray25.xbm"
#include "gray50.xbm"
#include "gray75.xbm"
#include "hourglass.xbm"
#include "info.xbm"
#include "questhead.xbm"
#include "question.xbm"
#include "warning.xbm"

#if defined(_MSC_VER)
#pragma warning (default : 4305)
#endif

/*
 * One of the following data structures exists for each bitmap that is
 * currently in use. Each structure is indexed with both "idTable" and
 * "nameTable".
 */

typedef struct TkBitmap {
    Pixmap bitmap;		/* X identifier for bitmap. None means this
				 * bitmap was created by Tk_DefineBitmap and
				 * it isn't currently in use. */
    int width, height;		/* Dimensions of bitmap. */
    Display *display;		/* Display for which bitmap is valid. */
    int screenNum;		/* Screen on which bitmap is valid. */
    int resourceRefCount;	/* Number of active uses of this bitmap (each
				 * active use corresponds to a call to
				 * Tk_AllocBitmapFromObj or Tk_GetBitmap). If
				 * this count is 0, then this TkBitmap
				 * structure is no longer valid and it isn't
				 * present in nameTable: it is being kept
				 * around only because there are objects
				 * referring to it. The structure is freed
				 * when resourceRefCount and objRefCount are
				 * both 0. */
    int objRefCount;		/* Number of Tcl_Obj's that reference this
				 * structure. */
    Tcl_HashEntry *nameHashPtr;	/* Entry in nameTable for this structure
				 * (needed when deleting). */
    Tcl_HashEntry *idHashPtr;	/* Entry in idTable for this structure (needed
				 * when deleting). */
    struct TkBitmap *nextPtr;	/* Points to the next TkBitmap structure with
				 * the same name. All bitmaps with the same
				 * name (but different displays or screens)
				 * are chained together off a single entry in
				 * nameTable. */
} TkBitmap;

/*
 * Used in bitmapDataTable, stored in the TkDisplay structure, to map between
 * in-core data about a bitmap to its TkBitmap structure.
 */

typedef struct {
    const char *source;		/* Bitmap bits. */
    int width, height;		/* Dimensions of bitmap. */
} DataKey;

typedef struct {
    int initialized;		/* 0 means table below needs initializing. */
    Tcl_HashTable predefBitmapTable;
				/* Hash table created by Tk_DefineBitmap to
				 * map from a name to a collection of in-core
				 * data about a bitmap. The table is indexed
				 * by the address of the data for the bitmap,
				 * and the entries contain pointers to
				 * TkPredefBitmap structures. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations for functions defined in this file:
 */

static void		BitmapInit(TkDisplay *dispPtr);
static void		DupBitmapObjProc(Tcl_Obj *srcObjPtr,
			    Tcl_Obj *dupObjPtr);
static void		FreeBitmap(TkBitmap *bitmapPtr);
static void		FreeBitmapObj(Tcl_Obj *objPtr);
static void		FreeBitmapObjProc(Tcl_Obj *objPtr);
static TkBitmap *	GetBitmap(Tcl_Interp *interp, Tk_Window tkwin,
			    const char *name);
static TkBitmap *	GetBitmapFromObj(Tk_Window tkwin, Tcl_Obj *objPtr);
static void		InitBitmapObj(Tcl_Obj *objPtr);

/*
 * The following structure defines the implementation of the "bitmap" Tcl
 * object, which maps a string bitmap name to a TkBitmap object. The ptr1
 * field of the Tcl_Obj points to a TkBitmap object.
 */

const Tcl_ObjType tkBitmapObjType = {
    "bitmap",			/* name */
    FreeBitmapObjProc,		/* freeIntRepProc */
    DupBitmapObjProc,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

/*
 *----------------------------------------------------------------------
 *
 * Tk_AllocBitmapFromObj --
 *
 *	Given a Tcl_Obj *, map the value to a corresponding Pixmap structure
 *	based on the tkwin given.
 *
 * Results:
 *	The return value is the X identifer for the desired bitmap (i.e. a
 *	Pixmap with a single plane), unless string couldn't be parsed
 *	correctly. In this case, None is returned and an error message is left
 *	in the interp's result. The caller should never modify the bitmap that
 *	is returned, and should eventually call Tk_FreeBitmapFromObj when the
 *	bitmap is no longer needed.
 *
 * Side effects:
 *	The bitmap is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeBitmapFromObj, so that the database can be cleaned up when
 *	bitmaps aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

Pixmap
Tk_AllocBitmapFromObj(
    Tcl_Interp *interp,		/* Interp for error results. This may be
				 * NULL. */
    Tk_Window tkwin,		/* Need the screen the bitmap is used on.*/
    Tcl_Obj *objPtr)		/* Object describing bitmap; see manual entry
				 * for legal syntax of string value. */
{
    TkBitmap *bitmapPtr;

    if (objPtr->typePtr != &tkBitmapObjType) {
	InitBitmapObj(objPtr);
    }
    bitmapPtr = objPtr->internalRep.twoPtrValue.ptr1;

    /*
     * If the object currently points to a TkBitmap, see if it's the one we
     * want. If so, increment its reference count and return.
     */

    if (bitmapPtr != NULL) {
	if (bitmapPtr->resourceRefCount == 0) {
	    /*
	     * This is a stale reference: it refers to a TkBitmap that's no
	     * longer in use. Clear the reference.
	     */

	    FreeBitmapObj(objPtr);
	    bitmapPtr = NULL;
	} else if ((Tk_Display(tkwin) == bitmapPtr->display)
		&& (Tk_ScreenNumber(tkwin) == bitmapPtr->screenNum)) {
	    bitmapPtr->resourceRefCount++;
	    return bitmapPtr->bitmap;
	}
    }

    /*
     * The object didn't point to the TkBitmap that we wanted. Search the list
     * of TkBitmaps with the same name to see if one of the others is the
     * right one.
     */

    if (bitmapPtr != NULL) {
	TkBitmap *firstBitmapPtr = Tcl_GetHashValue(bitmapPtr->nameHashPtr);

	FreeBitmapObj(objPtr);
	for (bitmapPtr = firstBitmapPtr; bitmapPtr != NULL;
		bitmapPtr = bitmapPtr->nextPtr) {
	    if ((Tk_Display(tkwin) == bitmapPtr->display) &&
		    (Tk_ScreenNumber(tkwin) == bitmapPtr->screenNum)) {
		bitmapPtr->resourceRefCount++;
		bitmapPtr->objRefCount++;
		objPtr->internalRep.twoPtrValue.ptr1 = bitmapPtr;
		return bitmapPtr->bitmap;
	    }
	}
    }

    /*
     * Still no luck. Call GetBitmap to allocate a new TkBitmap object.
     */

    bitmapPtr = GetBitmap(interp, tkwin, Tcl_GetString(objPtr));
    objPtr->internalRep.twoPtrValue.ptr1 = bitmapPtr;
    if (bitmapPtr == NULL) {
	return None;
    }
    bitmapPtr->objRefCount++;
    return bitmapPtr->bitmap;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetBitmap --
 *
 *	Given a string describing a bitmap, locate (or create if necessary) a
 *	bitmap that fits the description.
 *
 * Results:
 *	The return value is the X identifer for the desired bitmap (i.e. a
 *	Pixmap with a single plane), unless string couldn't be parsed
 *	correctly. In this case, None is returned and an error message is left
 *	in the interp's result. The caller should never modify the bitmap that
 *	is returned, and should eventually call Tk_FreeBitmap when the bitmap
 *	is no longer needed.
 *
 * Side effects:
 *	The bitmap is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeBitmap, so that the database can be cleaned up when bitmaps
 *	aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

Pixmap
Tk_GetBitmap(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting,
				 * this may be NULL. */
    Tk_Window tkwin,		/* Window in which bitmap will be used. */
    const char *string)		/* Description of bitmap. See manual entry for
				 * details on legal syntax. */
{
    TkBitmap *bitmapPtr = GetBitmap(interp, tkwin, string);

    if (bitmapPtr == NULL) {
	return None;
    }
    return bitmapPtr->bitmap;
}

/*
 *----------------------------------------------------------------------
 *
 * GetBitmap --
 *
 *	Given a string describing a bitmap, locate (or create if necessary) a
 *	bitmap that fits the description. This routine returns the internal
 *	data structure for the bitmap. This avoids extra hash table lookups in
 *	Tk_AllocBitmapFromObj.
 *
 * Results:
 *	The return value is the X identifer for the desired bitmap (i.e. a
 *	Pixmap with a single plane), unless string couldn't be parsed
 *	correctly. In this case, None is returned and an error message is left
 *	in the interp's result. The caller should never modify the bitmap that
 *	is returned, and should eventually call Tk_FreeBitmap when the bitmap
 *	is no longer needed.
 *
 * Side effects:
 *	The bitmap is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeBitmap or Tk_FreeBitmapFromObj, so that the database can be
 *	cleaned up when bitmaps aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

static TkBitmap *
GetBitmap(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting,
				 * this may be NULL. */
    Tk_Window tkwin,		/* Window in which bitmap will be used. */
    const char *string)		/* Description of bitmap. See manual entry for
				 * details on legal syntax. */
{
    Tcl_HashEntry *nameHashPtr, *predefHashPtr;
    TkBitmap *bitmapPtr, *existingBitmapPtr;
    TkPredefBitmap *predefPtr;
    Pixmap bitmap;
    int isNew, width = 0, height = 0, dummy2;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!dispPtr->bitmapInit) {
	BitmapInit(dispPtr);
    }

    nameHashPtr = Tcl_CreateHashEntry(&dispPtr->bitmapNameTable, string,
	    &isNew);
    if (!isNew) {
	existingBitmapPtr = Tcl_GetHashValue(nameHashPtr);
	for (bitmapPtr = existingBitmapPtr; bitmapPtr != NULL;
		bitmapPtr = bitmapPtr->nextPtr) {
	    if ((Tk_Display(tkwin) == bitmapPtr->display) &&
		    (Tk_ScreenNumber(tkwin) == bitmapPtr->screenNum)) {
		bitmapPtr->resourceRefCount++;
		return bitmapPtr;
	    }
	}
    } else {
	existingBitmapPtr = NULL;
    }

    /*
     * No suitable bitmap exists. Create a new bitmap from the information
     * contained in the string. If the string starts with "@" then the rest of
     * the string is a file name containing the bitmap. Otherwise the string
     * must refer to a bitmap defined by a call to Tk_DefineBitmap.
     */

    if (*string == '@') {	/* INTL: ISO char */
	Tcl_DString buffer;
	int result;

	if (Tcl_IsSafe(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't specify bitmap with '@' in a safe interpreter",
		    -1));
	    Tcl_SetErrorCode(interp, "TK", "SAFE", "BITMAP_FILE", NULL);
	    goto error;
	}

	/*
	 * Note that we need to cast away the const from the string because
	 * Tcl_TranslateFileName is non-const, even though it doesn't modify
	 * the string.
	 */

	string = Tcl_TranslateFileName(interp, (char *) string + 1, &buffer);
	if (string == NULL) {
	    goto error;
	}
	result = TkReadBitmapFile(Tk_Display(tkwin),
		RootWindowOfScreen(Tk_Screen(tkwin)), string,
		(unsigned int *) &width, (unsigned int *) &height,
		&bitmap, &dummy2, &dummy2);
	if (result != BitmapSuccess) {
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"error reading bitmap file \"%s\"", string));
		Tcl_SetErrorCode(interp, "TK", "BITMAP", "FILE_ERROR", NULL);
	    }
	    Tcl_DStringFree(&buffer);
	    goto error;
	}
	Tcl_DStringFree(&buffer);
    } else {
	predefHashPtr = Tcl_FindHashEntry(&tsdPtr->predefBitmapTable, string);
	if (predefHashPtr == NULL) {
	    /*
	     * The following platform specific call allows the user to define
	     * bitmaps that may only exist during run time. If it returns None
	     * nothing was found and we return the error.
	     */

	    bitmap = TkpGetNativeAppBitmap(Tk_Display(tkwin), string,
		    &width, &height);

	    if (bitmap == None) {
		if (interp != NULL) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "bitmap \"%s\" not defined", string));
		    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "BITMAP", string,
			    NULL);
		}
		goto error;
	    }
	} else {
	    predefPtr = Tcl_GetHashValue(predefHashPtr);
	    width = predefPtr->width;
	    height = predefPtr->height;
	    if (predefPtr->native) {
		bitmap = TkpCreateNativeBitmap(Tk_Display(tkwin),
		    predefPtr->source);
		if (bitmap == None) {
		    Tcl_Panic("native bitmap creation failed");
		}
	    } else {
		bitmap = XCreateBitmapFromData(Tk_Display(tkwin),
			RootWindowOfScreen(Tk_Screen(tkwin)),
			predefPtr->source, (unsigned)width, (unsigned)height);
	    }
	}
    }

    /*
     * Add information about this bitmap to our database.
     */

    bitmapPtr = ckalloc(sizeof(TkBitmap));
    bitmapPtr->bitmap = bitmap;
    bitmapPtr->width = width;
    bitmapPtr->height = height;
    bitmapPtr->display = Tk_Display(tkwin);
    bitmapPtr->screenNum = Tk_ScreenNumber(tkwin);
    bitmapPtr->resourceRefCount = 1;
    bitmapPtr->objRefCount = 0;
    bitmapPtr->nameHashPtr = nameHashPtr;
    bitmapPtr->idHashPtr = Tcl_CreateHashEntry(&dispPtr->bitmapIdTable,
	    (char *) bitmap, &isNew);
    if (!isNew) {
	Tcl_Panic("bitmap already registered in Tk_GetBitmap");
    }
    bitmapPtr->nextPtr = existingBitmapPtr;
    Tcl_SetHashValue(nameHashPtr, bitmapPtr);
    Tcl_SetHashValue(bitmapPtr->idHashPtr, bitmapPtr);
    return bitmapPtr;

  error:
    if (isNew) {
	Tcl_DeleteHashEntry(nameHashPtr);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DefineBitmap --
 *
 *	This function associates a textual name with a binary bitmap
 *	description, so that the name may be used to refer to the bitmap in
 *	future calls to Tk_GetBitmap.
 *
 * Results:
 *	A standard Tcl result. If an error occurs then TCL_ERROR is returned
 *	and a message is left in the interp's result.
 *
 * Side effects:
 *	"Name" is entered into the bitmap table and may be used from here on
 *	to refer to the given bitmap.
 *
 *----------------------------------------------------------------------
 */

int
Tk_DefineBitmap(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    const char *name,		/* Name to use for bitmap. Must not already be
				 * defined as a bitmap. */
    const void *source,		/* Address of bits for bitmap. */
    int width,			/* Width of bitmap. */
    int height)			/* Height of bitmap. */
{
    int isNew;
    Tcl_HashEntry *predefHashPtr;
    TkPredefBitmap *predefPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * Initialize the Bitmap module if not initialized already for this
     * thread. Since the current TkDisplay structure cannot be introspected
     * from here, pass a NULL pointer to BitmapInit, which will know to
     * initialize only the data in the ThreadSpecificData structure for the
     * current thread.
     */

    if (!tsdPtr->initialized) {
	BitmapInit(NULL);
    }

    predefHashPtr = Tcl_CreateHashEntry(&tsdPtr->predefBitmapTable,
	    name, &isNew);
    if (!isNew) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bitmap \"%s\" is already defined", name));
	Tcl_SetErrorCode(interp, "TK", "BITMAP", "EXISTS", NULL);
	return TCL_ERROR;
    }
    predefPtr = ckalloc(sizeof(TkPredefBitmap));
    predefPtr->source = source;
    predefPtr->width = width;
    predefPtr->height = height;
    predefPtr->native = 0;
    Tcl_SetHashValue(predefHashPtr, predefPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_NameOfBitmap --
 *
 *	Given a bitmap, return a textual string identifying the bitmap.
 *
 * Results:
 *	The return value is the string name associated with bitmap.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

const char *
Tk_NameOfBitmap(
    Display *display,		/* Display for which bitmap was allocated. */
    Pixmap bitmap)		/* Bitmap whose name is wanted. */
{
    Tcl_HashEntry *idHashPtr;
    TkBitmap *bitmapPtr;
    TkDisplay *dispPtr = TkGetDisplay(display);

    if (dispPtr == NULL || !dispPtr->bitmapInit) {
    unknown:
	Tcl_Panic("Tk_NameOfBitmap received unknown bitmap argument");
    }

    idHashPtr = Tcl_FindHashEntry(&dispPtr->bitmapIdTable, (char *) bitmap);
    if (idHashPtr == NULL) {
	goto unknown;
    }
    bitmapPtr = Tcl_GetHashValue(idHashPtr);
    return bitmapPtr->nameHashPtr->key.string;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_SizeOfBitmap --
 *
 *	Given a bitmap managed by this module, returns the width and height of
 *	the bitmap.
 *
 * Results:
 *	The words at *widthPtr and *heightPtr are filled in with the
 *	dimenstions of bitmap.
 *
 * Side effects:
 *	If bitmap isn't managed by this module then the function panics..
 *
 *--------------------------------------------------------------
 */

void
Tk_SizeOfBitmap(
    Display *display,		/* Display for which bitmap was allocated. */
    Pixmap bitmap,		/* Bitmap whose size is wanted. */
    int *widthPtr,		/* Store bitmap width here. */
    int *heightPtr)		/* Store bitmap height here. */
{
    Tcl_HashEntry *idHashPtr;
    TkBitmap *bitmapPtr;
    TkDisplay *dispPtr = TkGetDisplay(display);

    if (!dispPtr->bitmapInit) {
    unknownBitmap:
	Tcl_Panic("Tk_SizeOfBitmap received unknown bitmap argument");
    }

    idHashPtr = Tcl_FindHashEntry(&dispPtr->bitmapIdTable, (char *) bitmap);
    if (idHashPtr == NULL) {
	goto unknownBitmap;
    }
    bitmapPtr = Tcl_GetHashValue(idHashPtr);
    *widthPtr = bitmapPtr->width;
    *heightPtr = bitmapPtr->height;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeBitmap --
 *
 *	This function does all the work of releasing a bitmap allocated by
 *	Tk_GetBitmap or TkGetBitmapFromData. It is invoked by both
 *	Tk_FreeBitmap and Tk_FreeBitmapFromObj
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with bitmap is decremented, and it is
 *	officially deallocated if no-one is using it anymore.
 *
 *----------------------------------------------------------------------
 */

static void
FreeBitmap(
    TkBitmap *bitmapPtr)	/* Bitmap to be released. */
{
    TkBitmap *prevPtr;

    bitmapPtr->resourceRefCount--;
    if (bitmapPtr->resourceRefCount > 0) {
	return;
    }

    Tk_FreePixmap(bitmapPtr->display, bitmapPtr->bitmap);
    Tcl_DeleteHashEntry(bitmapPtr->idHashPtr);
    prevPtr = Tcl_GetHashValue(bitmapPtr->nameHashPtr);
    if (prevPtr == bitmapPtr) {
	if (bitmapPtr->nextPtr == NULL) {
	    Tcl_DeleteHashEntry(bitmapPtr->nameHashPtr);
	} else {
	    Tcl_SetHashValue(bitmapPtr->nameHashPtr, bitmapPtr->nextPtr);
	}
    } else {
	while (prevPtr->nextPtr != bitmapPtr) {
	    prevPtr = prevPtr->nextPtr;
	}
	prevPtr->nextPtr = bitmapPtr->nextPtr;
    }
    if (bitmapPtr->objRefCount == 0) {
	ckfree(bitmapPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeBitmap --
 *
 *	This function is called to release a bitmap allocated by Tk_GetBitmap
 *	or TkGetBitmapFromData.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with bitmap is decremented, and it is
 *	officially deallocated if no-one is using it anymore.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeBitmap(
    Display *display,		/* Display for which bitmap was allocated. */
    Pixmap bitmap)		/* Bitmap to be released. */
{
    Tcl_HashEntry *idHashPtr;
    TkDisplay *dispPtr = TkGetDisplay(display);

    if (!dispPtr->bitmapInit) {
	Tcl_Panic("Tk_FreeBitmap called before Tk_GetBitmap");
    }

    idHashPtr = Tcl_FindHashEntry(&dispPtr->bitmapIdTable, (char *) bitmap);
    if (idHashPtr == NULL) {
	Tcl_Panic("Tk_FreeBitmap received unknown bitmap argument");
    }
    FreeBitmap(Tcl_GetHashValue(idHashPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeBitmapFromObj --
 *
 *	This function is called to release a bitmap allocated by
 *	Tk_AllocBitmapFromObj. It does not throw away the Tcl_Obj *; it only
 *	gets rid of the hash table entry for this bitmap and clears the cached
 *	value that is normally stored in the object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the bitmap represented by objPtr
 *	is decremented, and the bitmap is released to X if there are no
 *	remaining uses for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeBitmapFromObj(
    Tk_Window tkwin,		/* The window this bitmap lives in. Needed for
				 * the display value. */
    Tcl_Obj *objPtr)		/* The Tcl_Obj * to be freed. */
{
    FreeBitmap(GetBitmapFromObj(tkwin, objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeBitmapObjProc, FreeBitmapObj --
 *
 *	This proc is called to release an object reference to a bitmap.
 *	Called when the object's internal rep is released or when the cached
 *	bitmapPtr needs to be changed.
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
FreeBitmapObjProc(
    Tcl_Obj *objPtr)		/* The object we are releasing. */
{
    FreeBitmapObj(objPtr);
    objPtr->typePtr = NULL;
}

static void
FreeBitmapObj(
    Tcl_Obj *objPtr)		/* The object we are releasing. */
{
    TkBitmap *bitmapPtr = objPtr->internalRep.twoPtrValue.ptr1;

    if (bitmapPtr != NULL) {
	bitmapPtr->objRefCount--;
	if ((bitmapPtr->objRefCount == 0)
		&& (bitmapPtr->resourceRefCount == 0)) {
	    ckfree(bitmapPtr);
	}
	objPtr->internalRep.twoPtrValue.ptr1 = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DupBitmapObjProc --
 *
 *	When a cached bitmap object is duplicated, this is called to update
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
DupBitmapObjProc(
    Tcl_Obj *srcObjPtr,		/* The object we are copying from. */
    Tcl_Obj *dupObjPtr)		/* The object we are copying to. */
{
    TkBitmap *bitmapPtr = srcObjPtr->internalRep.twoPtrValue.ptr1;

    dupObjPtr->typePtr = srcObjPtr->typePtr;
    dupObjPtr->internalRep.twoPtrValue.ptr1 = bitmapPtr;

    if (bitmapPtr != NULL) {
	bitmapPtr->objRefCount++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetBitmapFromData --
 *
 *	Given a description of the bits for a bitmap, make a bitmap that has
 *	the given properties. *** NOTE: this function is obsolete and really
 *	shouldn't be used anymore. ***
 *
 * Results:
 *	The return value is the X identifer for the desired bitmap (a
 *	one-plane Pixmap), unless it couldn't be created properly. In this
 *	case, None is returned and an error message is left in the interp's
 *	result. The caller should never modify the bitmap that is returned,
 *	and should eventually call Tk_FreeBitmap when the bitmap is no longer
 *	needed.
 *
 * Side effects:
 *	The bitmap is added to an internal database with a reference count.
 *	For each call to this function, there should eventually be a call to
 *	Tk_FreeBitmap, so that the database can be cleaned up when bitmaps
 *	aren't needed anymore.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
Pixmap
Tk_GetBitmapFromData(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window in which bitmap will be used. */
    const void *source,		/* Bitmap data for bitmap shape. */
    int width, int height)	/* Dimensions of bitmap. */
{
    DataKey nameKey;
    Tcl_HashEntry *dataHashPtr;
    int isNew;
    char string[16 + TCL_INTEGER_SPACE];
    char *name;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (!tsdPtr->initialized) {
	BitmapInit(dispPtr);
    }

    nameKey.source = source;
    nameKey.width = width;
    nameKey.height = height;
    dataHashPtr = Tcl_CreateHashEntry(&dispPtr->bitmapDataTable,
	    (char *) &nameKey, &isNew);
    if (!isNew) {
	name = Tcl_GetHashValue(dataHashPtr);
    } else {
	dispPtr->bitmapAutoNumber++;
	sprintf(string, "_tk%d", dispPtr->bitmapAutoNumber);
	name = string;
	Tcl_SetHashValue(dataHashPtr, name);
	if (Tk_DefineBitmap(interp, name, source, width, height) != TCL_OK) {
	    Tcl_DeleteHashEntry(dataHashPtr);
	    return TCL_ERROR;
	}
    }
    return Tk_GetBitmap(interp, tkwin, name);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetBitmapFromObj --
 *
 *	Returns the bitmap referred to by a Tcl object. The bitmap must
 *	already have been allocated via a call to Tk_AllocBitmapFromObj or
 *	Tk_GetBitmap.
 *
 * Results:
 *	Returns the Pixmap that matches the tkwin and the string rep of
 *	objPtr.
 *
 * Side effects:
 *	If the object is not already a bitmap, the conversion will free any
 *	old internal representation.
 *
 *----------------------------------------------------------------------
 */

Pixmap
Tk_GetBitmapFromObj(
    Tk_Window tkwin,
    Tcl_Obj *objPtr)		/* The object from which to get pixels. */
{
    TkBitmap *bitmapPtr = GetBitmapFromObj(tkwin, objPtr);

    return bitmapPtr->bitmap;
}

/*
 *----------------------------------------------------------------------
 *
 * GetBitmapFromObj --
 *
 *	Returns the bitmap referred to by a Tcl object. The bitmap must
 *	already have been allocated via a call to Tk_AllocBitmapFromObj or
 *	Tk_GetBitmap.
 *
 * Results:
 *	Returns the TkBitmap * that matches the tkwin and the string rep of
 *	objPtr.
 *
 * Side effects:
 *	If the object is not already a bitmap, the conversion will free any
 *	old internal representation.
 *
 *----------------------------------------------------------------------
 */

static TkBitmap *
GetBitmapFromObj(
    Tk_Window tkwin,		/* Window in which the bitmap will be used. */
    Tcl_Obj *objPtr)		/* The object that describes the desired
				 * bitmap. */
{
    TkBitmap *bitmapPtr;
    Tcl_HashEntry *hashPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (objPtr->typePtr != &tkBitmapObjType) {
	InitBitmapObj(objPtr);
    }

    bitmapPtr = objPtr->internalRep.twoPtrValue.ptr1;
    if (bitmapPtr != NULL) {
	if ((bitmapPtr->resourceRefCount > 0)
		&& (Tk_Display(tkwin) == bitmapPtr->display)) {
	    return bitmapPtr;
	}
	hashPtr = bitmapPtr->nameHashPtr;
	FreeBitmapObj(objPtr);
    } else {
	hashPtr = Tcl_FindHashEntry(&dispPtr->bitmapNameTable,
		Tcl_GetString(objPtr));
	if (hashPtr == NULL) {
	    goto error;
	}
    }

    /*
     * At this point we've got a hash table entry, off of which hang one or
     * more TkBitmap structures. See if any of them will work.
     */

    for (bitmapPtr = Tcl_GetHashValue(hashPtr); bitmapPtr != NULL;
	    bitmapPtr = bitmapPtr->nextPtr) {
	if (Tk_Display(tkwin) == bitmapPtr->display) {
	    objPtr->internalRep.twoPtrValue.ptr1 = bitmapPtr;
	    bitmapPtr->objRefCount++;
	    return bitmapPtr;
	}
    }

  error:
    Tcl_Panic("GetBitmapFromObj called with non-existent bitmap!");
    /*
     * The following code isn't reached; it's just there to please compilers.
     */
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * InitBitmapObj --
 *
 *	Bookeeping function to change an objPtr to a bitmap type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The old internal rep of the object is freed. The internal rep is
 *	cleared. The final form of the object is set by either
 *	Tk_AllocBitmapFromObj or GetBitmapFromObj.
 *
 *----------------------------------------------------------------------
 */

static void
InitBitmapObj(
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
    objPtr->typePtr = &tkBitmapObjType;
    objPtr->internalRep.twoPtrValue.ptr1 = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * BitmapInit --
 *
 *	Initializes hash tables used by this module. Initializes tables stored
 *	in TkDisplay structure if a TkDisplay pointer is passed in. Also
 *	initializes the thread-local data in the current thread's
 *	ThreadSpecificData structure.
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
BitmapInit(
    TkDisplay *dispPtr)		/* TkDisplay structure encapsulating
				 * thread-specific data used by this module,
				 * or NULL if unavailable. */
{
    Tcl_Interp *dummy;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * First initialize the data in the ThreadSpecificData strucuture, if
     * needed.
     */

    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	dummy = Tcl_CreateInterp();
	Tcl_InitHashTable(&tsdPtr->predefBitmapTable, TCL_STRING_KEYS);

	Tk_DefineBitmap(dummy, "error", error_bits,
		error_width, error_height);
	Tk_DefineBitmap(dummy, "gray75", gray75_bits,
		gray75_width, gray75_height);
	Tk_DefineBitmap(dummy, "gray50", gray50_bits,
		gray50_width, gray50_height);
	Tk_DefineBitmap(dummy, "gray25", gray25_bits,
		gray25_width, gray25_height);
	Tk_DefineBitmap(dummy, "gray12", gray12_bits,
		gray12_width, gray12_height);
	Tk_DefineBitmap(dummy, "hourglass", hourglass_bits,
		hourglass_width, hourglass_height);
	Tk_DefineBitmap(dummy, "info", info_bits,
		info_width, info_height);
	Tk_DefineBitmap(dummy, "questhead", questhead_bits,
		questhead_width, questhead_height);
	Tk_DefineBitmap(dummy, "question", question_bits,
		question_width, question_height);
	Tk_DefineBitmap(dummy, "warning", warning_bits,
		warning_width, warning_height);

	TkpDefineNativeBitmaps();
	Tcl_DeleteInterp(dummy);
    }

    /*
     * Was a valid TkDisplay pointer passed? If so, initialize the Bitmap
     * module tables in that structure.
     */

    if (dispPtr != NULL) {
	dispPtr->bitmapInit = 1;
	Tcl_InitHashTable(&dispPtr->bitmapNameTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&dispPtr->bitmapDataTable,
		sizeof(DataKey) / sizeof(int));

	/*
	 * The call below is tricky: can't use sizeof(IdKey) because it gets
	 * padded with extra unpredictable bytes on some 64-bit machines.
	 */

	/*
	 * The comment above doesn't make sense...
	 */

	Tcl_InitHashTable(&dispPtr->bitmapIdTable, TCL_ONE_WORD_KEYS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkReadBitmapFile --
 *
 *	Loads a bitmap image in X bitmap format into the specified drawable.
 *	This is equivelent to the XReadBitmapFile in X.
 *
 * Results:
 *	Sets the size, hotspot, and bitmap on success.
 *
 * Side effects:
 *	Creates a new bitmap from the file data.
 *
 *----------------------------------------------------------------------
 */

int
TkReadBitmapFile(
    Display *display,
    Drawable d,
    const char *filename,
    unsigned int *width_return,
    unsigned int *height_return,
    Pixmap *bitmap_return,
    int *x_hot_return,
    int *y_hot_return)
{
    char *data;

    data = TkGetBitmapData(NULL, NULL, filename,
	    (int *) width_return, (int *) height_return, x_hot_return,
	    y_hot_return);
    if (data == NULL) {
	return BitmapFileInvalid;
    }

    *bitmap_return = XCreateBitmapFromData(display, d, data, *width_return,
	    *height_return);
    ckfree(data);
    return BitmapSuccess;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDebugBitmap --
 *
 *	This function returns debugging information about a bitmap.
 *
 * Results:
 *	The return value is a list with one sublist for each TkBitmap
 *	corresponding to "name". Each sublist has two elements that contain
 *	the resourceRefCount and objRefCount fields from the TkBitmap
 *	structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkDebugBitmap(
    Tk_Window tkwin,		/* The window in which the bitmap will be used
				 * (not currently used). */
    const char *name)		/* Name of the desired color. */
{
    TkBitmap *bitmapPtr;
    Tcl_HashEntry *hashPtr;
    Tcl_Obj *resultPtr, *objPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    resultPtr = Tcl_NewObj();
    hashPtr = Tcl_FindHashEntry(&dispPtr->bitmapNameTable, name);
    if (hashPtr != NULL) {
	bitmapPtr = Tcl_GetHashValue(hashPtr);
	if (bitmapPtr == NULL) {
	    Tcl_Panic("TkDebugBitmap found empty hash table entry");
	}
	for ( ; (bitmapPtr != NULL); bitmapPtr = bitmapPtr->nextPtr) {
	    objPtr = Tcl_NewObj();
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(bitmapPtr->resourceRefCount));
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(bitmapPtr->objRefCount));
	    Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
	}
    }
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetBitmapPredefTable --
 *
 *	This function is used by tkMacBitmap.c to access the thread-specific
 *	predefBitmap table that maps from the names of the predefined bitmaps
 *	to data associated with those bitmaps. It is required because the
 *	table is allocated in thread-local storage and is not visible outside
 *	this file.

 * Results:
 *	Returns a pointer to the predefined bitmap hash table for the current
 *	thread.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashTable *
TkGetBitmapPredefTable(void)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    return &tsdPtr->predefBitmapTable;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
