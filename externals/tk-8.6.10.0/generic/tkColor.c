/*
 * tkColor.c --
 *
 *	This file maintains a database of color values for the Tk toolkit, in
 *	order to avoid round-trips to the server to map color names to pixel
 *	values.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkColor.h"

/*
 * Structures of the following following type are used as keys for
 * colorValueTable (in TkDisplay).
 */

typedef struct {
    int red, green, blue;	/* Values for desired color. */
    Colormap colormap;		/* Colormap from which color will be
				 * allocated. */
    Display *display;		/* Display for colormap. */
} ValueKey;

/*
 * The structure below is used to allocate thread-local data.
 */

typedef struct {
    char rgbString[20];		/* */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations for functions defined in this file:
 */

static void		ColorInit(TkDisplay *dispPtr);
static void		DupColorObjProc(Tcl_Obj *srcObjPtr,Tcl_Obj *dupObjPtr);
static void		FreeColorObj(Tcl_Obj *objPtr);
static void		FreeColorObjProc(Tcl_Obj *objPtr);
static void		InitColorObj(Tcl_Obj *objPtr);

/*
 * The following structure defines the implementation of the "color" Tcl
 * object, which maps a string color name to a TkColor object. The ptr1 field
 * of the Tcl_Obj points to a TkColor object.
 */

const Tcl_ObjType tkColorObjType = {
    "color",			/* name */
    FreeColorObjProc,		/* freeIntRepProc */
    DupColorObjProc,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

/*
 *----------------------------------------------------------------------
 *
 * Tk_AllocColorFromObj --
 *
 *	Given a Tcl_Obj *, map the value to a corresponding XColor structure
 *	based on the tkwin given.
 *
 * Results:
 *	The return value is a pointer to an XColor structure that indicates
 *	the red, blue, and green intensities for the color given by the string
 *	in objPtr, and also specifies a pixel value to use to draw in that
 *	color. If an error occurs, NULL is returned and an error message will
 *	be left in interp's result (unless interp is NULL).
 *
 * Side effects:
 *	The color is added to an internal database with a reference count. For
 *	each call to this function, there should eventually be a call to
 *	Tk_FreeColorFromObj so that the database is cleaned up when colors
 *	aren't in use anymore.
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_AllocColorFromObj(
    Tcl_Interp *interp,		/* Used only for error reporting. If NULL,
				 * then no messages are provided. */
    Tk_Window tkwin,		/* Window in which the color will be used.*/
    Tcl_Obj *objPtr)		/* Object that describes the color; string
				 * value is a color name such as "red" or
				 * "#ff0000".*/
{
    TkColor *tkColPtr;

    if (objPtr->typePtr != &tkColorObjType) {
	InitColorObj(objPtr);
    }
    tkColPtr = (TkColor *) objPtr->internalRep.twoPtrValue.ptr1;

    /*
     * If the object currently points to a TkColor, see if it's the one we
     * want. If so, increment its reference count and return.
     */

    if (tkColPtr != NULL) {
	if (tkColPtr->resourceRefCount == 0) {
	    /*
	     * This is a stale reference: it refers to a TkColor that's no
	     * longer in use. Clear the reference.
	     */

	    FreeColorObj(objPtr);
	    tkColPtr = NULL;
	} else if ((Tk_Screen(tkwin) == tkColPtr->screen)
		&& (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
	    tkColPtr->resourceRefCount++;
	    return (XColor *) tkColPtr;
	}
    }

    /*
     * The object didn't point to the TkColor that we wanted. Search the list
     * of TkColors with the same name to see if one of the other TkColors is
     * the right one.
     */

    if (tkColPtr != NULL) {
	TkColor *firstColorPtr = Tcl_GetHashValue(tkColPtr->hashPtr);

	FreeColorObj(objPtr);
	for (tkColPtr = firstColorPtr; tkColPtr != NULL;
		tkColPtr = tkColPtr->nextPtr) {
	    if ((Tk_Screen(tkwin) == tkColPtr->screen)
		    && (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
		tkColPtr->resourceRefCount++;
		tkColPtr->objRefCount++;
		objPtr->internalRep.twoPtrValue.ptr1 = tkColPtr;
		return (XColor *) tkColPtr;
	    }
	}
    }

    /*
     * Still no luck. Call Tk_GetColor to allocate a new TkColor object.
     */

    tkColPtr = (TkColor *) Tk_GetColor(interp, tkwin, Tcl_GetString(objPtr));
    objPtr->internalRep.twoPtrValue.ptr1 = tkColPtr;
    if (tkColPtr != NULL) {
	tkColPtr->objRefCount++;
    }
    return (XColor *) tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetColor --
 *
 *	Given a string name for a color, map the name to a corresponding
 *	XColor structure.
 *
 * Results:
 *	The return value is a pointer to an XColor structure that indicates
 *	the red, blue, and green intensities for the color given by "name",
 *	and also specifies a pixel value to use to draw in that color. If an
 *	error occurs, NULL is returned and an error message will be left in
 *	the interp's result.
 *
 * Side effects:
 *	The color is added to an internal database with a reference count. For
 *	each call to this function, there should eventually be a call to
 *	Tk_FreeColor so that the database is cleaned up when colors aren't in
 *	use anymore.
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_GetColor(
    Tcl_Interp *interp,		/* Place to leave error message if color can't
				 * be found. */
    Tk_Window tkwin,		/* Window in which color will be used. */
    Tk_Uid name)		/* Name of color to be allocated (in form
				 * suitable for passing to XParseColor). */
{
    Tcl_HashEntry *nameHashPtr;
    int isNew;
    TkColor *tkColPtr;
    TkColor *existingColPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (!dispPtr->colorInit) {
	ColorInit(dispPtr);
    }

    /*
     * First, check to see if there's already a mapping for this color name.
     */

    nameHashPtr = Tcl_CreateHashEntry(&dispPtr->colorNameTable, name, &isNew);
    if (!isNew) {
	existingColPtr = Tcl_GetHashValue(nameHashPtr);
	for (tkColPtr = existingColPtr; tkColPtr != NULL;
		tkColPtr = tkColPtr->nextPtr) {
	    if ((tkColPtr->screen == Tk_Screen(tkwin))
		    && (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
		tkColPtr->resourceRefCount++;
		return &tkColPtr->color;
	    }
	}
    } else {
	existingColPtr = NULL;
    }

    /*
     * The name isn't currently known. Map from the name to a pixel value.
     */

    tkColPtr = TkpGetColor(tkwin, name);
    if (tkColPtr == NULL) {
	if (interp != NULL) {
	    if (*name == '#') {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"invalid color name \"%s\"", name));
		Tcl_SetErrorCode(interp, "TK", "VALUE", "COLOR", NULL);
	    } else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"unknown color name \"%s\"", name));
		Tcl_SetErrorCode(interp, "TK", "LOOKUP", "COLOR", name, NULL);
	    }
	}
	if (isNew) {
	    Tcl_DeleteHashEntry(nameHashPtr);
	}
	return NULL;
    }

    /*
     * Now create a new TkColor structure and add it to colorNameTable (in
     * TkDisplay).
     */

    tkColPtr->magic = COLOR_MAGIC;
    tkColPtr->gc = NULL;
    tkColPtr->screen = Tk_Screen(tkwin);
    tkColPtr->colormap = Tk_Colormap(tkwin);
    tkColPtr->visual = Tk_Visual(tkwin);
    tkColPtr->resourceRefCount = 1;
    tkColPtr->objRefCount = 0;
    tkColPtr->type = TK_COLOR_BY_NAME;
    tkColPtr->hashPtr = nameHashPtr;
    tkColPtr->nextPtr = existingColPtr;
    Tcl_SetHashValue(nameHashPtr, tkColPtr);

    return &tkColPtr->color;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetColorByValue --
 *
 *	Given a desired set of red-green-blue intensities for a color, locate
 *	a pixel value to use to draw that color in a given window.
 *
 * Results:
 *	The return value is a pointer to an XColor structure that indicates
 *	the closest red, blue, and green intensities available to those
 *	specified in colorPtr, and also specifies a pixel value to use to draw
 *	in that color.
 *
 * Side effects:
 *	The color is added to an internal database with a reference count. For
 *	each call to this function, there should eventually be a call to
 *	Tk_FreeColor, so that the database is cleaned up when colors aren't in
 *	use anymore.
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_GetColorByValue(
    Tk_Window tkwin,		/* Window where color will be used. */
    XColor *colorPtr)		/* Red, green, and blue fields indicate
				 * desired color. */
{
    ValueKey valueKey;
    Tcl_HashEntry *valueHashPtr;
    int isNew;
    TkColor *tkColPtr;
    Display *display = Tk_Display(tkwin);
    TkDisplay *dispPtr = TkGetDisplay(display);

    if (!dispPtr->colorInit) {
	ColorInit(dispPtr);
    }

    /*
     * First, check to see if there's already a mapping for this color name.
     * Must clear the structure first; it's not tightly packed on 64-bit
     * systems. [Bug 2911570]
     */

    memset(&valueKey, 0, sizeof(ValueKey));
    valueKey.red = colorPtr->red;
    valueKey.green = colorPtr->green;
    valueKey.blue = colorPtr->blue;
    valueKey.colormap = Tk_Colormap(tkwin);
    valueKey.display = display;
    valueHashPtr = Tcl_CreateHashEntry(&dispPtr->colorValueTable,
	    (char *) &valueKey, &isNew);
    if (!isNew) {
	tkColPtr = Tcl_GetHashValue(valueHashPtr);
	tkColPtr->resourceRefCount++;
	return &tkColPtr->color;
    }

    /*
     * The name isn't currently known. Find a pixel value for this color and
     * add a new structure to colorValueTable (in TkDisplay).
     */

    tkColPtr = TkpGetColorByValue(tkwin, colorPtr);
    tkColPtr->magic = COLOR_MAGIC;
    tkColPtr->gc = NULL;
    tkColPtr->screen = Tk_Screen(tkwin);
    tkColPtr->colormap = valueKey.colormap;
    tkColPtr->visual = Tk_Visual(tkwin);
    tkColPtr->resourceRefCount = 1;
    tkColPtr->objRefCount = 0;
    tkColPtr->type = TK_COLOR_BY_VALUE;
    tkColPtr->hashPtr = valueHashPtr;
    tkColPtr->nextPtr = NULL;
    Tcl_SetHashValue(valueHashPtr, tkColPtr);
    return &tkColPtr->color;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_NameOfColor --
 *
 *	Given a color, return a textual string identifying the color.
 *
 * Results:
 *	If colorPtr was created by Tk_GetColor, then the return value is the
 *	"string" that was used to create it. Otherwise the return value is a
 *	string that could have been passed to Tk_GetColor to allocate that
 *	color. The storage for the returned string is only guaranteed to
 *	persist up until the next call to this function.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

const char *
Tk_NameOfColor(
    XColor *colorPtr)		/* Color whose name is desired. */
{
    register TkColor *tkColPtr = (TkColor *) colorPtr;

    if (tkColPtr->magic==COLOR_MAGIC && tkColPtr->type==TK_COLOR_BY_NAME) {
	return tkColPtr->hashPtr->key.string;
    } else {
	ThreadSpecificData *tsdPtr =
		Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

	sprintf(tsdPtr->rgbString, "#%04x%04x%04x", colorPtr->red,
		colorPtr->green, colorPtr->blue);

	/*
	 * If the string has the form #RSRSTUTUVWVW (where equal letters
	 * denote equal hexdigits) then this is equivalent to #RSTUVW. Then
	 * output the shorter form.
	 */

	if ((tsdPtr->rgbString[1] == tsdPtr->rgbString[3])
		&& (tsdPtr->rgbString[2] == tsdPtr->rgbString[4])
		&& (tsdPtr->rgbString[5] == tsdPtr->rgbString[7])
		&& (tsdPtr->rgbString[6] == tsdPtr->rgbString[8])
		&& (tsdPtr->rgbString[9] == tsdPtr->rgbString[11])
		&& (tsdPtr->rgbString[10] == tsdPtr->rgbString[12])) {
	    tsdPtr->rgbString[3] = tsdPtr->rgbString[5];
	    tsdPtr->rgbString[4] = tsdPtr->rgbString[6];
	    tsdPtr->rgbString[5] = tsdPtr->rgbString[9];
	    tsdPtr->rgbString[6] = tsdPtr->rgbString[10];
	    tsdPtr->rgbString[7] = '\0';
	}
	return tsdPtr->rgbString;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GCForColor --
 *
 *	Given a color allocated from this module, this function returns a GC
 *	that can be used for simple drawing with that color.
 *
 * Results:
 *	The return value is a GC with color set as its foreground color and
 *	all other fields defaulted. This GC is only valid as long as the color
 *	exists; it is freed automatically when the last reference to the color
 *	is freed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

GC
Tk_GCForColor(
    XColor *colorPtr,		/* Color for which a GC is desired. Must have
				 * been allocated by Tk_GetColor. */
    Drawable drawable)		/* Drawable in which the color will be used
				 * (must have same screen and depth as the one
				 * for which the color was allocated). */
{
    TkColor *tkColPtr = (TkColor *) colorPtr;
    XGCValues gcValues;

    /*
     * Do a quick sanity check to make sure this color was really allocated by
     * Tk_GetColor.
     */

    if (tkColPtr->magic != COLOR_MAGIC) {
	Tcl_Panic("Tk_GCForColor called with bogus color");
    }

    if (tkColPtr->gc == NULL) {
	gcValues.foreground = tkColPtr->color.pixel;
	tkColPtr->gc = XCreateGC(DisplayOfScreen(tkColPtr->screen), drawable,
		GCForeground, &gcValues);
    }
    return tkColPtr->gc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeColor --
 *
 *	This function is called to release a color allocated by Tk_GetColor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with colorPtr is deleted, and the color
 *	is released to X if there are no remaining uses for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeColor(
    XColor *colorPtr)		/* Color to be released. Must have been
				 * allocated by Tk_GetColor or
				 * Tk_GetColorByValue. */
{
    TkColor *tkColPtr = (TkColor *) colorPtr;
    Screen *screen = tkColPtr->screen;
    TkColor *prevPtr;

    /*
     * Do a quick sanity check to make sure this color was really allocated by
     * Tk_GetColor.
     */

    if (tkColPtr->magic != COLOR_MAGIC) {
	Tcl_Panic("Tk_FreeColor called with bogus color");
    }

    tkColPtr->resourceRefCount--;
    if (tkColPtr->resourceRefCount > 0) {
	return;
    }

    /*
     * This color is no longer being actively used, so free the color
     * resources associated with it and remove it from the hash table. No
     * longer any objects referencing it.
     */

    if (tkColPtr->gc != NULL) {
	XFreeGC(DisplayOfScreen(screen), tkColPtr->gc);
	tkColPtr->gc = NULL;
    }
    TkpFreeColor(tkColPtr);

    prevPtr = Tcl_GetHashValue(tkColPtr->hashPtr);
    if (prevPtr == tkColPtr) {
	if (tkColPtr->nextPtr == NULL) {
	    Tcl_DeleteHashEntry(tkColPtr->hashPtr);
	} else {
	    Tcl_SetHashValue(tkColPtr->hashPtr, tkColPtr->nextPtr);
	}
    } else {
	while (prevPtr->nextPtr != tkColPtr) {
	    prevPtr = prevPtr->nextPtr;
	}
	prevPtr->nextPtr = tkColPtr->nextPtr;
    }

    /*
     * Free the TkColor structure if there are no objects referencing it.
     * However, if there are objects referencing it then keep the structure
     * around; it will get freed when the last reference is cleared
     */

    if (tkColPtr->objRefCount == 0) {
	ckfree(tkColPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeColorFromObj --
 *
 *	This function is called to release a color allocated by
 *	Tk_AllocColorFromObj. It does not throw away the Tcl_Obj *; it only
 *	gets rid of the hash table entry for this color and clears the cached
 *	value that is normally stored in the object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the color represented by objPtr is
 *	decremented, and the color is released to X if there are no remaining
 *	uses for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeColorFromObj(
    Tk_Window tkwin,		/* The window this color lives in. Needed for
				 * the screen and colormap values. */
    Tcl_Obj *objPtr)		/* The Tcl_Obj * to be freed. */
{
    Tk_FreeColor(Tk_GetColorFromObj(tkwin, objPtr));
    FreeColorObj(objPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeColorObjProc, FreeColorObj --
 *
 *	This proc is called to release an object reference to a color. Called
 *	when the object's internal rep is released or when the cached tkColPtr
 *	needs to be changed.
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
FreeColorObjProc(
    Tcl_Obj *objPtr)		/* The object we are releasing. */
{
    FreeColorObj(objPtr);
    objPtr->typePtr = NULL;
}

static void
FreeColorObj(
    Tcl_Obj *objPtr)		/* The object we are releasing. */
{
    TkColor *tkColPtr = objPtr->internalRep.twoPtrValue.ptr1;

    if (tkColPtr != NULL) {
	tkColPtr->objRefCount--;
	if ((tkColPtr->objRefCount == 0)
		&& (tkColPtr->resourceRefCount == 0)) {
	    ckfree(tkColPtr);
	}
	objPtr->internalRep.twoPtrValue.ptr1 = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DupColorObjProc --
 *
 *	When a cached color object is duplicated, this is called to update the
 *	internal reps.
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
DupColorObjProc(
    Tcl_Obj *srcObjPtr,		/* The object we are copying from. */
    Tcl_Obj *dupObjPtr)		/* The object we are copying to. */
{
    TkColor *tkColPtr = srcObjPtr->internalRep.twoPtrValue.ptr1;

    dupObjPtr->typePtr = srcObjPtr->typePtr;
    dupObjPtr->internalRep.twoPtrValue.ptr1 = tkColPtr;

    if (tkColPtr != NULL) {
	tkColPtr->objRefCount++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetColorFromObj --
 *
 *	Returns the color referred to by a Tcl object. The color must already
 *	have been allocated via a call to Tk_AllocColorFromObj or Tk_GetColor.
 *
 * Results:
 *	Returns the XColor * that matches the tkwin and the string rep of
 *	objPtr.
 *
 * Side effects:
 *	If the object is not already a color, the conversion will free any old
 *	internal representation.
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_GetColorFromObj(
    Tk_Window tkwin,		/* The window in which the color will be
				 * used. */
    Tcl_Obj *objPtr)		/* String value contains the name of the
				 * desired color. */
{
    TkColor *tkColPtr;
    Tcl_HashEntry *hashPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (objPtr->typePtr != &tkColorObjType) {
	InitColorObj(objPtr);
    }

    /*
     * First check to see if the internal representation of the object is
     * defined and is a color that is valid for the current screen and color
     * map. If it is, we are done.
     */

    tkColPtr = objPtr->internalRep.twoPtrValue.ptr1;
    if ((tkColPtr != NULL)
	    && (tkColPtr->resourceRefCount > 0)
	    && (Tk_Screen(tkwin) == tkColPtr->screen)
	    && (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
	/*
	 * The object already points to the right TkColor structure. Just
	 * return it.
	 */

	return (XColor *) tkColPtr;
    }

    /*
     * If we reach this point, it means that the TkColor structure that we
     * have cached in the internal representation is not valid for the current
     * screen and colormap. But there is a list of other TkColor structures
     * attached to the TkDisplay. Walk this list looking for the right TkColor
     * structure.
     */

    hashPtr = Tcl_FindHashEntry(&dispPtr->colorNameTable,
	    Tcl_GetString(objPtr));
    if (hashPtr == NULL) {
	goto error;
    }
    for (tkColPtr = Tcl_GetHashValue(hashPtr);
	    (tkColPtr != NULL); tkColPtr = tkColPtr->nextPtr) {
	if ((Tk_Screen(tkwin) == tkColPtr->screen)
		&& (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
	    FreeColorObj(objPtr);
	    objPtr->internalRep.twoPtrValue.ptr1 = tkColPtr;
	    tkColPtr->objRefCount++;
	    return (XColor *) tkColPtr;
	}
    }

  error:
    Tcl_Panic("Tk_GetColorFromObj called with non-existent color!");
    /*
     * The following code isn't reached; it's just there to please compilers.
     */
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * InitColorObj --
 *
 *	Bookeeping function to change an objPtr to a color type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The old internal rep of the object is freed. The object's type is set
 *	to color with a NULL TkColor pointer (the pointer will be set later by
 *	either Tk_AllocColorFromObj or Tk_GetColorFromObj).
 *
 *----------------------------------------------------------------------
 */

static void
InitColorObj(
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
    objPtr->typePtr = &tkColorObjType;
    objPtr->internalRep.twoPtrValue.ptr1 = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ColorInit --
 *
 *	Initialize the structure used for color management.
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
ColorInit(
    TkDisplay *dispPtr)
{
    if (!dispPtr->colorInit) {
	dispPtr->colorInit = 1;
	Tcl_InitHashTable(&dispPtr->colorNameTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&dispPtr->colorValueTable,
		sizeof(ValueKey)/sizeof(int));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkDebugColor --
 *
 *	This function returns debugging information about a color.
 *
 * Results:
 *	The return value is a list with one sublist for each TkColor
 *	corresponding to "name". Each sublist has two elements that contain
 *	the resourceRefCount and objRefCount fields from the TkColor
 *	structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkDebugColor(
    Tk_Window tkwin,		/* The window in which the color will be used
				 * (not currently used). */
    const char *name)		/* Name of the desired color. */
{
    Tcl_HashEntry *hashPtr;
    Tcl_Obj *resultPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    resultPtr = Tcl_NewObj();
    hashPtr = Tcl_FindHashEntry(&dispPtr->colorNameTable, name);
    if (hashPtr != NULL) {
	TkColor *tkColPtr = Tcl_GetHashValue(hashPtr);

	if (tkColPtr == NULL) {
	    Tcl_Panic("TkDebugColor found empty hash table entry");
	}
	for ( ; (tkColPtr != NULL); tkColPtr = tkColPtr->nextPtr) {
	    Tcl_Obj *objPtr = Tcl_NewObj();

	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(tkColPtr->resourceRefCount));
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(tkColPtr->objRefCount));
	    Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
	}
    }
    return resultPtr;
}

#ifndef _WIN32

/* This function is not necessary for Win32,
 * since XParseColor already does the right thing */

#undef XParseColor

const char *const tkWebColors[20] = {
    /* 'a' */ "qua\0#0000ffffffff",
    /* 'b' */ NULL,
    /* 'c' */ "rimson\0#dcdc14143c3c",
    /* 'd' */ NULL,
    /* 'e' */ NULL,
    /* 'f' */ "uchsia\0#ffff0000ffff",
    /* 'g' */ "reen\0#000080800000",
    /* 'h' */ NULL,
    /* 'i' */ "ndigo\0#4b4b00008282",
    /* 'j' */ NULL,
    /* 'k' */ NULL,
    /* 'l' */ "ime\0#0000ffff0000",
    /* 'm' */ "aroon\0#808000000000",
    /* 'n' */ NULL,
    /* 'o' */ "live\0#808080800000",
    /* 'p' */ "urple\0#808000008080",
    /* 'q' */ NULL,
    /* 'r' */ NULL,
    /* 's' */ "ilver\0#c0c0c0c0c0c0",
    /* 't' */ "eal\0#000080808080"
};

Status
TkParseColor(
    Display *display,		/* The display */
    Colormap map,			/* Color map */
    const char *name,     /* String to be parsed */
    XColor *color)
{
    char buf[14];
    if (*name == '#') {
	buf[0] = '#'; buf[13] = '\0';
	if (!*(++name) || !*(++name) || !*(++name)) {
	    /* Not at least 3 hex digits, so invalid */
	return 0;
	} else if (!*(++name)) {
	    /* Exactly 3 hex digits */
	    buf[9] = buf[10] = buf[11] = buf[12] = *(--name);
	    buf[5] = buf[6] = buf[7] = buf[8] = *(--name);
	    buf[1] = buf[2] = buf[3] = buf[4] = *(--name);
	    name = buf;
	} else if (!*(++name)	|| !*(++name)) {
	    /* Not at least 6 hex digits, so invalid */
	    return 0;
	} else if (!*(++name)) {
	    /* Exactly 6 hex digits */
	    buf[10] = buf[12] = *(--name);
	    buf[9] = buf[11] = *(--name);
	    buf[6] = buf[8] = *(--name);
	    buf[5] = buf[7] = *(--name);
	    buf[2] = buf[4] = *(--name);
	    buf[1] = buf[3] = *(--name);
	    name = buf;
	} else if (!*(++name) || !*(++name)) {
	    /* Not at least 9 hex digits, so invalid */
	    return 0;
	} else if (!*(++name)) {
	    /* Exactly 9 hex digits */
	    buf[11] = *(--name);
	    buf[10] = *(--name);
	    buf[9] = buf[12] = *(--name);
	    buf[7] = *(--name);
	    buf[6] = *(--name);
	    buf[5] = buf[8] = *(--name);
	    buf[3] = *(--name);
	    buf[2] = *(--name);
	    buf[1] = buf[4] = *(--name);
	    name = buf;
	} else if (!*(++name) || !*(++name) || *(++name)) {
	    /* Not exactly 12 hex digits, so invalid */
	    return 0;
	} else {
	    name -= 13;
	}
	goto done;
    } else if (((*name - 'A') & 0xdf) < sizeof(tkWebColors)/sizeof(tkWebColors[0])) {
	if (!((name[0] - 'G') & 0xdf) && !((name[1] - 'R') & 0xdf)
		&& !((name[2] - 'A') & 0xdb) && !((name[3] - 'Y') & 0xdf)
		&& !name[4]) {
	    name = "#808080808080";
	    goto done;
	} else {
	    const char *p = tkWebColors[((*name - 'A') & 0x1f)];
	    if (p) {
		const char *q = name;
		while (!((*p - *(++q)) & 0xdf)) {
		    if (!*p++) {
			name = p;
			goto done;
		    }
		}
	    }
	}
    }
    if (strlen(name) > 99) {
	return 0;
    }
done:
    return XParseColor(display, map, name, color);
}
#endif /* _WIN32 */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
