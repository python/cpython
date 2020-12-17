/*
 * tclVar.c --
 *
 *	This file contains routines that implement Tcl variables (both scalars
 *	and arrays).
 *
 *	The implementation of arrays is modelled after an initial
 *	implementation by Mark Diekhans and Karl Lehenbauer.
 *
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 2001 by Kevin B. Kenny. All rights reserved.
 * Copyright (c) 2007 Miguel Sofer
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclOOInt.h"

/*
 * Prototypes for the variable hash key methods.
 */

static Tcl_HashEntry *	AllocVarEntry(Tcl_HashTable *tablePtr, void *keyPtr);
static void		FreeVarEntry(Tcl_HashEntry *hPtr);
static int		CompareVarKeys(void *keyPtr, Tcl_HashEntry *hPtr);

static const Tcl_HashKeyType tclVarHashKeyType = {
    TCL_HASH_KEY_TYPE_VERSION,	/* version */
    0,				/* flags */
    TclHashObjKey,		/* hashKeyProc */
    CompareVarKeys,		/* compareKeysProc */
    AllocVarEntry,		/* allocEntryProc */
    FreeVarEntry		/* freeEntryProc */
};

static inline Var *	VarHashCreateVar(TclVarHashTable *tablePtr,
			    Tcl_Obj *key, int *newPtr);
static inline Var *	VarHashFirstVar(TclVarHashTable *tablePtr,
			    Tcl_HashSearch *searchPtr);
static inline Var *	VarHashNextVar(Tcl_HashSearch *searchPtr);
static inline void	CleanupVar(Var *varPtr, Var *arrayPtr);

#define VarHashGetValue(hPtr) \
    ((Var *) ((char *)hPtr - TclOffset(VarInHash, entry)))

/*
 * NOTE: VarHashCreateVar increments the recount of its key argument.
 * All callers that will call Tcl_DecrRefCount on that argument must
 * call Tcl_IncrRefCount on it before passing it in.  This requirement
 * can bubble up to callers of callers .... etc.
 */

static inline Var *
VarHashCreateVar(
    TclVarHashTable *tablePtr,
    Tcl_Obj *key,
    int *newPtr)
{
    Tcl_HashEntry *hPtr = Tcl_CreateHashEntry(&tablePtr->table,
	    key, newPtr);

    if (hPtr) {
	return VarHashGetValue(hPtr);
    } else {
	return NULL;
    }
}

#define VarHashFindVar(tablePtr, key) \
    VarHashCreateVar((tablePtr), (key), NULL)

#define VarHashInvalidateEntry(varPtr) \
    ((varPtr)->flags |= VAR_DEAD_HASH)

#define VarHashDeleteEntry(varPtr) \
    Tcl_DeleteHashEntry(&(((VarInHash *) varPtr)->entry))

#define VarHashFirstEntry(tablePtr, searchPtr) \
    Tcl_FirstHashEntry(&(tablePtr)->table, (searchPtr))

#define VarHashNextEntry(searchPtr) \
    Tcl_NextHashEntry((searchPtr))

static inline Var *
VarHashFirstVar(
    TclVarHashTable *tablePtr,
    Tcl_HashSearch *searchPtr)
{
    Tcl_HashEntry *hPtr = VarHashFirstEntry(tablePtr, searchPtr);

    if (hPtr) {
	return VarHashGetValue(hPtr);
    } else {
	return NULL;
    }
}

static inline Var *
VarHashNextVar(
    Tcl_HashSearch *searchPtr)
{
    Tcl_HashEntry *hPtr = VarHashNextEntry(searchPtr);

    if (hPtr) {
	return VarHashGetValue(hPtr);
    } else {
	return NULL;
    }
}

#define VarHashGetKey(varPtr) \
    (((VarInHash *)(varPtr))->entry.key.objPtr)

#define VarHashDeleteTable(tablePtr) \
    Tcl_DeleteHashTable(&(tablePtr)->table)

/*
 * The strings below are used to indicate what went wrong when a variable
 * access is denied.
 */

static const char *noSuchVar =		"no such variable";
static const char *isArray =		"variable is array";
static const char *needArray =		"variable isn't array";
static const char *noSuchElement =	"no such element in array";
static const char *danglingElement =
	"upvar refers to element in deleted array";
static const char *danglingVar =
	"upvar refers to variable in deleted namespace";
static const char *badNamespace =	"parent namespace doesn't exist";
static const char *missingName =	"missing variable name";
static const char *isArrayElement =
	"name refers to an element in an array";

/*
 * A test to see if we are in a call frame that has local variables. This is
 * true if we are inside a procedure body.
 */

#define HasLocalVars(framePtr) ((framePtr)->isProcCallFrame & FRAME_IS_PROC)

/*
 * The following structure describes an enumerative search in progress on an
 * array variable; this are invoked with options to the "array" command.
 */

typedef struct ArraySearch {
    int id;			/* Integer id used to distinguish among
				 * multiple concurrent searches for the same
				 * array. */
    struct Var *varPtr;		/* Pointer to array variable that's being
				 * searched. */
    Tcl_HashSearch search;	/* Info kept by the hash module about progress
				 * through the array. */
    Tcl_HashEntry *nextEntry;	/* Non-null means this is the next element to
				 * be enumerated (it's leftover from the
				 * Tcl_FirstHashEntry call or from an "array
				 * anymore" command). NULL means must call
				 * Tcl_NextHashEntry to get value to
				 * return. */
    struct ArraySearch *nextPtr;/* Next in list of all active searches for
				 * this variable, or NULL if this is the last
				 * one. */
} ArraySearch;

/*
 * Forward references to functions defined later in this file:
 */

static void		AppendLocals(Tcl_Interp *interp, Tcl_Obj *listPtr,
			    Tcl_Obj *patternPtr, int includeLinks);
static void		DeleteSearches(Interp *iPtr, Var *arrayVarPtr);
static void		DeleteArray(Interp *iPtr, Tcl_Obj *arrayNamePtr,
			    Var *varPtr, int flags, int index);
static int		LocateArray(Tcl_Interp *interp, Tcl_Obj *name,
			    Var **varPtrPtr, int *isArrayPtr);
static int		NotArrayError(Tcl_Interp *interp, Tcl_Obj *name);
static Tcl_Var		ObjFindNamespaceVar(Tcl_Interp *interp,
			    Tcl_Obj *namePtr, Tcl_Namespace *contextNsPtr,
			    int flags);
static int		ObjMakeUpvar(Tcl_Interp *interp,
			    CallFrame *framePtr, Tcl_Obj *otherP1Ptr,
			    const char *otherP2, const int otherFlags,
			    Tcl_Obj *myNamePtr, int myFlags, int index);
static ArraySearch *	ParseSearchId(Tcl_Interp *interp, const Var *varPtr,
			    Tcl_Obj *varNamePtr, Tcl_Obj *handleObj);
static void		UnsetVarStruct(Var *varPtr, Var *arrayPtr,
			    Interp *iPtr, Tcl_Obj *part1Ptr,
			    Tcl_Obj *part2Ptr, int flags, int index);
static int		SetArraySearchObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr);

/*
 * Functions defined in this file that may be exported in the future for use
 * by the bytecode compiler and engine or to the public interface.
 */

MODULE_SCOPE Var *	TclLookupSimpleVar(Tcl_Interp *interp,
			    Tcl_Obj *varNamePtr, int flags, const int create,
			    const char **errMsgPtr, int *indexPtr);

static Tcl_DupInternalRepProc	DupLocalVarName;
static Tcl_FreeInternalRepProc	FreeLocalVarName;
static Tcl_UpdateStringProc	PanicOnUpdateVarName;

static Tcl_FreeInternalRepProc	FreeParsedVarName;
static Tcl_DupInternalRepProc	DupParsedVarName;
static Tcl_UpdateStringProc	UpdateParsedVarName;

static Tcl_UpdateStringProc	PanicOnUpdateVarName;
static Tcl_SetFromAnyProc	PanicOnSetVarName;

/*
 * Types of Tcl_Objs used to cache variable lookups.
 *
 * localVarName - INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1:   pointer to name obj in varFramePtr->localCache
 *			  or NULL if it is this same obj
 *   twoPtrValue.ptr2: index into locals table
 *
 * parsedVarName - INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1:	pointer to the array name Tcl_Obj, or NULL if it is a
 *			scalar variable
 *   twoPtrValue.ptr2:	pointer to the element name string (owned by this
 *			Tcl_Obj), or NULL if it is a scalar variable
 */

static const Tcl_ObjType localVarNameType = {
    "localVarName",
    FreeLocalVarName, DupLocalVarName, PanicOnUpdateVarName, PanicOnSetVarName
};

static const Tcl_ObjType tclParsedVarNameType = {
    "parsedVarName",
    FreeParsedVarName, DupParsedVarName, UpdateParsedVarName, PanicOnSetVarName
};

/*
 * Type of Tcl_Objs used to speed up array searches.
 *
 * INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1:	searchIdNumber (cast to pointer)
 *   twoPtrValue.ptr2:	variableNameStartInString (cast to pointer)
 *
 * Note that the value stored in ptr2 is the offset into the string of the
 * start of the variable name and not the address of the variable name itself,
 * as this can be safely copied.
 */

const Tcl_ObjType tclArraySearchType = {
    "array search",
    NULL, NULL, NULL, SetArraySearchObj
};

Var *
TclVarHashCreateVar(
    TclVarHashTable *tablePtr,
    const char *key,
    int *newPtr)
{
    Tcl_Obj *keyPtr;
    Var *varPtr;

    keyPtr = Tcl_NewStringObj(key, -1);
    Tcl_IncrRefCount(keyPtr);
    varPtr = VarHashCreateVar(tablePtr, keyPtr, newPtr);
    Tcl_DecrRefCount(keyPtr);

    return varPtr;
}

static int
LocateArray(
    Tcl_Interp *interp,
    Tcl_Obj *name,
    Var **varPtrPtr,
    int *isArrayPtr)
{
    Var *arrayPtr, *varPtr = TclObjLookupVarEx(interp, name, NULL, /*flags*/ 0,
	    /*msg*/ 0, /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);

    if (TclCheckArrayTraces(interp, varPtr, arrayPtr, name, -1) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (varPtrPtr) {
	*varPtrPtr = varPtr;
    }
    if (isArrayPtr) {
	*isArrayPtr = varPtr && !TclIsVarUndefined(varPtr)
		&& TclIsVarArray(varPtr);
    }
    return TCL_OK;
}

static int
NotArrayError(
    Tcl_Interp *interp,
    Tcl_Obj *name)
{
    const char *nameStr = Tcl_GetString(name);

    Tcl_SetObjResult(interp,
	    Tcl_ObjPrintf("\"%s\" isn't an array", nameStr));
    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ARRAY", nameStr, NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCleanupVar --
 *
 *	This function is called when it looks like it may be OK to free up a
 *	variable's storage. If the variable is in a hashtable, its Var
 *	structure and hash table entry will be freed along with those of its
 *	containing array, if any. This function is called, for example, when
 *	a trace on a variable deletes a variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the variable (or its containing array) really is dead and in a
 *	hashtable, then its Var structure, and possibly its hash table entry,
 *	is freed up.
 *
 *----------------------------------------------------------------------
 */

static inline void
CleanupVar(
    Var *varPtr,		/* Pointer to variable that may be a candidate
				 * for being expunged. */
    Var *arrayPtr)		/* Array that contains the variable, or NULL
				 * if this variable isn't an array element. */
{
    if (TclIsVarUndefined(varPtr) && TclIsVarInHash(varPtr)
	    && !TclIsVarTraced(varPtr)
	    && (VarHashRefCount(varPtr) == !TclIsVarDeadHash(varPtr))) {
	if (VarHashRefCount(varPtr) == 0) {
	    ckfree(varPtr);
	} else {
	    VarHashDeleteEntry(varPtr);
	}
    }
    if (arrayPtr != NULL && TclIsVarUndefined(arrayPtr) &&
	    TclIsVarInHash(arrayPtr) && !TclIsVarTraced(arrayPtr) &&
	    (VarHashRefCount(arrayPtr) == !TclIsVarDeadHash(arrayPtr))) {
	if (VarHashRefCount(arrayPtr) == 0) {
	    ckfree(arrayPtr);
	} else {
	    VarHashDeleteEntry(arrayPtr);
	}
    }
}

void
TclCleanupVar(
    Var *varPtr,		/* Pointer to variable that may be a candidate
				 * for being expunged. */
    Var *arrayPtr)		/* Array that contains the variable, or NULL
				 * if this variable isn't an array element. */
{
    CleanupVar(varPtr, arrayPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclLookupVar --
 *
 *	This function is used to locate a variable given its name(s). It has
 *	been mostly superseded by TclObjLookupVar, it is now only used by the
 *	trace code. It is kept in tcl8.5 mainly because it is in the internal
 *	stubs table, so that some extension may be calling it.
 *
 * Results:
 *	The return value is a pointer to the variable structure indicated by
 *	part1 and part2, or NULL if the variable couldn't be found. If the
 *	variable is found, *arrayPtrPtr is filled in with the address of the
 *	variable structure for the array that contains the variable (or NULL
 *	if the variable is a scalar). If the variable can't be found and
 *	either createPart1 or createPart2 are 1, a new as-yet-undefined
 *	(VAR_UNDEFINED) variable structure is created, entered into a hash
 *	table, and returned.
 *
 *	If the variable isn't found and creation wasn't specified, or some
 *	other error occurs, NULL is returned and an error message is left in
 *	the interp's result if TCL_LEAVE_ERR_MSG is set in flags.
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED even
 *	if createPart1 or createPart2 are 1 (these only cause the hash table
 *	entry or array to be created). For example, the variable might be a
 *	global that has been unset but is still referenced by a procedure, or
 *	a variable that has been unset but it only being kept in existence (if
 *	VAR_UNDEFINED) by a trace.
 *
 * Side effects:
 *	New hashtable entries may be created if createPart1 or createPart2
 *	are 1.
 *
 *----------------------------------------------------------------------
 */

Var *
TclLookupVar(
    Tcl_Interp *interp,		/* Interpreter to use for lookup. */
    const char *part1,		/* If part2 isn't NULL, this is the name of an
				 * array. Otherwise, this is a full variable
				 * name that could include a parenthesized
				 * array element. */
    const char *part2,		/* Name of element within array, or NULL. */
    int flags,			/* Only TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits matter. */
    const char *msg,		/* Verb to use in error messages, e.g. "read"
				 * or "set". Only needed if TCL_LEAVE_ERR_MSG
				 * is set in flags. */
    int createPart1,		/* If 1, create hash table entry for part 1 of
				 * name, if it doesn't already exist. If 0,
				 * return error if it doesn't exist. */
    int createPart2,		/* If 1, create hash table entry for part 2 of
				 * name, if it doesn't already exist. If 0,
				 * return error if it doesn't exist. */
    Var **arrayPtrPtr)		/* If the name refers to an element of an
				 * array, *arrayPtrPtr gets filled in with
				 * address of array variable. Otherwise this
				 * is set to NULL. */
{
    Var *varPtr;
    Tcl_Obj *part1Ptr = Tcl_NewStringObj(part1, -1);

    if (createPart1) {
	Tcl_IncrRefCount(part1Ptr);
    }

    varPtr = TclObjLookupVar(interp, part1Ptr, part2, flags, msg,
	    createPart1, createPart2, arrayPtrPtr);

    TclDecrRefCount(part1Ptr);
    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjLookupVar, TclObjLookupVarEx --
 *
 *	This function is used by virtually all of the variable code to locate
 *	a variable given its name(s). The parsing into array/element
 *	components and (if possible) the lookup results are cached in
 *	part1Ptr, which is converted to one of the varNameTypes.
 *
 * Results:
 *	The return value is a pointer to the variable structure indicated by
 *	part1Ptr and part2, or NULL if the variable couldn't be found. If *
 *	the variable is found, *arrayPtrPtr is filled with the address of the
 *	variable structure for the array that contains the variable (or NULL
 *	if the variable is a scalar). If the variable can't be found and
 *	either createPart1 or createPart2 are 1, a new as-yet-undefined
 *	(VAR_UNDEFINED) variable structure is created, entered into a hash
 *	table, and returned.
 *
 *	If the variable isn't found and creation wasn't specified, or some
 *	other error occurs, NULL is returned and an error message is left in
 *	the interp's result if TCL_LEAVE_ERR_MSG is set in flags.
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED even
 *	if createPart1 or createPart2 are 1 (these only cause the hash table
 *	entry or array to be created). For example, the variable might be a
 *	global that has been unset but is still referenced by a procedure, or
 *	a variable that has been unset but it only being kept in existence (if
 *	VAR_UNDEFINED) by a trace.
 *
 * Side effects:
 *	New hashtable entries may be created if createPart1 or createPart2
 *	are 1. The object part1Ptr is converted to one of localVarNameType,
 *	tclNsVarNameType or tclParsedVarNameType and caches as much of the
 *	lookup as it can.
 *	When createPart1 is 1, callers must IncrRefCount part1Ptr if they
 *	plan to DecrRefCount it.
 *
 *----------------------------------------------------------------------
 */

Var *
TclObjLookupVar(
    Tcl_Interp *interp,		/* Interpreter to use for lookup. */
    register Tcl_Obj *part1Ptr,	/* If part2 isn't NULL, this is the name of an
				 * array. Otherwise, this is a full variable
				 * name that could include a parenthesized
				 * array element. */
    const char *part2,		/* Name of element within array, or NULL. */
    int flags,			/* Only TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits matter. */
    const char *msg,		/* Verb to use in error messages, e.g. "read"
				 * or "set". Only needed if TCL_LEAVE_ERR_MSG
				 * is set in flags. */
    const int createPart1,	/* If 1, create hash table entry for part 1 of
				 * name, if it doesn't already exist. If 0,
				 * return error if it doesn't exist. */
    const int createPart2,	/* If 1, create hash table entry for part 2 of
				 * name, if it doesn't already exist. If 0,
				 * return error if it doesn't exist. */
    Var **arrayPtrPtr)		/* If the name refers to an element of an
				 * array, *arrayPtrPtr gets filled in with
				 * address of array variable. Otherwise this
				 * is set to NULL. */
{
    Tcl_Obj *part2Ptr = NULL;
    Var *resPtr;

    if (part2) {
	part2Ptr = Tcl_NewStringObj(part2, -1);
	if (createPart2) {
	    Tcl_IncrRefCount(part2Ptr);
	}
    }

    resPtr = TclObjLookupVarEx(interp, part1Ptr, part2Ptr,
	    flags, msg, createPart1, createPart2, arrayPtrPtr);

    if (part2Ptr) {
	Tcl_DecrRefCount(part2Ptr);
    }

    return resPtr;
}

/*
 *	When createPart1 is 1, callers must IncrRefCount part1Ptr if they
 *	plan to DecrRefCount it.
 *	When createPart2 is 1, callers must IncrRefCount part2Ptr if they
 *	plan to DecrRefCount it.
 */
Var *
TclObjLookupVarEx(
    Tcl_Interp *interp,		/* Interpreter to use for lookup. */
    Tcl_Obj *part1Ptr,		/* If part2Ptr isn't NULL, this is the name of
				 * an array. Otherwise, this is a full
				 * variable name that could include a
				 * parenthesized array element. */
    Tcl_Obj *part2Ptr,		/* Name of element within array, or NULL. */
    int flags,			/* Only TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * and TCL_LEAVE_ERR_MSG bits matter. */
    const char *msg,		/* Verb to use in error messages, e.g. "read"
				 * or "set". Only needed if TCL_LEAVE_ERR_MSG
				 * is set in flags. */
    const int createPart1,	/* If 1, create hash table entry for part 1 of
				 * name, if it doesn't already exist. If 0,
				 * return error if it doesn't exist. */
    const int createPart2,	/* If 1, create hash table entry for part 2 of
				 * name, if it doesn't already exist. If 0,
				 * return error if it doesn't exist. */
    Var **arrayPtrPtr)		/* If the name refers to an element of an
				 * array, *arrayPtrPtr gets filled in with
				 * address of array variable. Otherwise this
				 * is set to NULL. */
{
    Interp *iPtr = (Interp *) interp;
    register Var *varPtr;	/* Points to the variable's in-frame Var
				 * structure. */
    const char *part1;
    int index, len1, len2;
    int parsed = 0;
    Tcl_Obj *objPtr;
    const Tcl_ObjType *typePtr = part1Ptr->typePtr;
    const char *errMsg = NULL;
    CallFrame *varFramePtr = iPtr->varFramePtr;
    const char *part2 = part2Ptr? TclGetString(part2Ptr):NULL;
    char *newPart2 = NULL;
    *arrayPtrPtr = NULL;

    if (typePtr == &localVarNameType) {
	int localIndex;

    localVarNameTypeHandling:
	localIndex = PTR2INT(part1Ptr->internalRep.twoPtrValue.ptr2);
	if (HasLocalVars(varFramePtr)
		&& !(flags & (TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY))
		&& (localIndex < varFramePtr->numCompiledLocals)) {
	    /*
	     * Use the cached index if the names coincide.
	     */

	    Tcl_Obj *namePtr = part1Ptr->internalRep.twoPtrValue.ptr1;
	    Tcl_Obj *checkNamePtr = localName(iPtr->varFramePtr, localIndex);

	    if ((!namePtr && (checkNamePtr == part1Ptr)) ||
		    (namePtr && (checkNamePtr == namePtr))) {
		varPtr = (Var *) &(varFramePtr->compiledLocals[localIndex]);
		goto donePart1;
	    }
	}
	goto doneParsing;
    }

    /*
     * If part1Ptr is a tclParsedVarNameType, separate it into the pre-parsed
     * parts.
     */

    if (typePtr == &tclParsedVarNameType) {
	if (part1Ptr->internalRep.twoPtrValue.ptr1 != NULL) {
	    if (part2Ptr != NULL) {
		/*
		 * ERROR: part1Ptr is already an array element, cannot specify
		 * a part2.
		 */

		if (flags & TCL_LEAVE_ERR_MSG) {
		    TclObjVarErrMsg(interp, part1Ptr, part2Ptr, msg,
			    noSuchVar, -1);
		    Tcl_SetErrorCode(interp, "TCL", "VALUE", "VARNAME", NULL);
		}
		return NULL;
	    }
	    part2 = newPart2 = part1Ptr->internalRep.twoPtrValue.ptr2;
	    if (newPart2) {
		part2Ptr = Tcl_NewStringObj(newPart2, -1);
		if (createPart2) {
		    Tcl_IncrRefCount(part2Ptr);
		}
	    }
	    part1Ptr = part1Ptr->internalRep.twoPtrValue.ptr1;
	    typePtr = part1Ptr->typePtr;
	    if (typePtr == &localVarNameType) {
		goto localVarNameTypeHandling;
	    }
	}
	parsed = 1;
    }
    part1 = TclGetStringFromObj(part1Ptr, &len1);

    if (!parsed && len1 && (*(part1 + len1 - 1) == ')')) {
	/*
	 * part1Ptr is possibly an unparsed array element.
	 */

	register int i;

	len2 = -1;
	for (i = 0; i < len1; i++) {
	    if (*(part1 + i) == '(') {
		if (part2Ptr != NULL) {
		    if (flags & TCL_LEAVE_ERR_MSG) {
			TclObjVarErrMsg(interp, part1Ptr, part2Ptr, msg,
				needArray, -1);
			Tcl_SetErrorCode(interp, "TCL", "VALUE", "VARNAME",
				NULL);
		    }
		    return NULL;
		}

		/*
		 * part1Ptr points to an array element; first copy the element
		 * name to a new string part2.
		 */

		part2 = part1 + i + 1;
		len2 = len1 - i - 2;
		len1 = i;

		newPart2 = ckalloc(len2 + 1);
		memcpy(newPart2, part2, (unsigned) len2);
		*(newPart2+len2) = '\0';
		part2 = newPart2;
		part2Ptr = Tcl_NewStringObj(newPart2, -1);
		if (createPart2) {
		    Tcl_IncrRefCount(part2Ptr);
		}

		/*
		 * Free the internal rep of the original part1Ptr, now renamed
		 * objPtr, and set it to tclParsedVarNameType.
		 */

		objPtr = part1Ptr;
		TclFreeIntRep(objPtr);
		objPtr->typePtr = &tclParsedVarNameType;

		/*
		 * Define a new string object to hold the new part1Ptr, i.e.,
		 * the array name. Set the internal rep of objPtr, reset
		 * typePtr and part1 to contain the references to the array
		 * name.
		 */

		TclNewStringObj(part1Ptr, part1, len1);
		Tcl_IncrRefCount(part1Ptr);

		objPtr->internalRep.twoPtrValue.ptr1 = part1Ptr;
		objPtr->internalRep.twoPtrValue.ptr2 = (void *) part2;

		typePtr = part1Ptr->typePtr;
		part1 = TclGetString(part1Ptr);
		break;
	    }
	}
    }

  doneParsing:
    /*
     * part1Ptr is not an array element; look it up, and convert it to one of
     * the cached types if possible.
     */

    TclFreeIntRep(part1Ptr);

    varPtr = TclLookupSimpleVar(interp, part1Ptr, flags, createPart1,
	    &errMsg, &index);
    if (varPtr == NULL) {
	if ((errMsg != NULL) && (flags & TCL_LEAVE_ERR_MSG)) {
	    TclObjVarErrMsg(interp, part1Ptr, part2Ptr, msg, errMsg, -1);
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARNAME",
		    TclGetString(part1Ptr), NULL);
	}
	if (newPart2) {
	    Tcl_DecrRefCount(part2Ptr);
	}
	return NULL;
    }

    /*
     * Cache the newly found variable if possible.
     */

    if (index >= 0) {
	/*
	 * An indexed local variable.
	 */
	Tcl_Obj *cachedNamePtr = localName(iPtr->varFramePtr, index);

	part1Ptr->typePtr = &localVarNameType;
	if (part1Ptr != cachedNamePtr) {
	    part1Ptr->internalRep.twoPtrValue.ptr1 = cachedNamePtr;
	    Tcl_IncrRefCount(cachedNamePtr);
	    if (cachedNamePtr->typePtr != &localVarNameType
		    || cachedNamePtr->internalRep.twoPtrValue.ptr1 != NULL) {
	        TclFreeIntRep(cachedNamePtr);
	    }
	} else {
	    part1Ptr->internalRep.twoPtrValue.ptr1 = NULL;
	}
	part1Ptr->internalRep.twoPtrValue.ptr2 = INT2PTR(index);
    } else {
	/*
	 * At least mark part1Ptr as already parsed.
	 */

	part1Ptr->typePtr = &tclParsedVarNameType;
	part1Ptr->internalRep.twoPtrValue.ptr1 = NULL;
	part1Ptr->internalRep.twoPtrValue.ptr2 = NULL;
    }

  donePart1:
    while (TclIsVarLink(varPtr)) {
	varPtr = varPtr->value.linkPtr;
    }

    if (part2Ptr != NULL) {
	/*
	 * Array element sought: look it up.
	 */

	*arrayPtrPtr = varPtr;
	varPtr = TclLookupArrayElement(interp, part1Ptr, part2Ptr, flags, msg,
		createPart1, createPart2, varPtr, -1);
	if (newPart2) {
	    Tcl_DecrRefCount(part2Ptr);
	}
    }
    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclLookupSimpleVar --
 *
 *	This function is used by to locate a simple variable (i.e., not an
 *	array element) given its name.
 *
 * Results:
 *	The return value is a pointer to the variable structure indicated by
 *	varName, or NULL if the variable couldn't be found. If the variable
 *	can't be found and create is 1, a new as-yet-undefined (VAR_UNDEFINED)
 *	variable structure is created, entered into a hash table, and
 *	returned.
 *
 *	If the current CallFrame corresponds to a proc and the variable found
 *	is one of the compiledLocals, its index is placed in *indexPtr.
 *	Otherwise, *indexPtr will be set to (according to the needs of
 *	TclObjLookupVar):
 *	    -1 a global reference
 *	    -2 a reference to a namespace variable
 *	    -3 a non-cachable reference, i.e., one of:
 *		. non-indexed local var
 *		. a reference of unknown origin;
 *		. resolution by a namespace or interp resolver
 *
 *	If the variable isn't found and creation wasn't specified, or some
 *	other error occurs, NULL is returned and the corresponding error
 *	message is left in *errMsgPtr.
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED even
 *	if create is 1 (this only causes the hash table entry to be created).
 *	For example, the variable might be a global that has been unset but is
 *	still referenced by a procedure, or a variable that has been unset but
 *	it only being kept in existence (if VAR_UNDEFINED) by a trace.
 *
 * Side effects:
 *	A new hashtable entry may be created if create is 1.
 *	Callers must Incr varNamePtr if they plan to Decr it if create is 1.
 *
 *----------------------------------------------------------------------
 */

Var *
TclLookupSimpleVar(
    Tcl_Interp *interp,		/* Interpreter to use for lookup. */
    Tcl_Obj *varNamePtr,	/* This is a simple variable name that could
				 * represent a scalar or an array. */
    int flags,			/* Only TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_AVOID_RESOLVERS and TCL_LEAVE_ERR_MSG
				 * bits matter. */
    const int create,		/* If 1, create hash table entry for varname,
				 * if it doesn't already exist. If 0, return
				 * error if it doesn't exist. */
    const char **errMsgPtr,
    int *indexPtr)
{
    Interp *iPtr = (Interp *) interp;
    CallFrame *varFramePtr = iPtr->varFramePtr;
				/* Points to the procedure call frame whose
				 * variables are currently in use. Same as the
				 * current procedure's frame, if any, unless
				 * an "uplevel" is executing. */
    TclVarHashTable *tablePtr;	/* Points to the hashtable, if any, in which
				 * to look up the variable. */
    Tcl_Var var;		/* Used to search for global names. */
    Var *varPtr;		/* Points to the Var structure returned for
				 * the variable. */
    Namespace *varNsPtr, *cxtNsPtr, *dummy1Ptr, *dummy2Ptr;
    ResolverScheme *resPtr;
    int isNew, i, result, varLen;
    const char *varName = TclGetStringFromObj(varNamePtr, &varLen);

    varPtr = NULL;
    varNsPtr = NULL;		/* Set non-NULL if a nonlocal variable. */
    *indexPtr = -3;

    if (flags & TCL_GLOBAL_ONLY) {
	cxtNsPtr = iPtr->globalNsPtr;
    } else {
	cxtNsPtr = iPtr->varFramePtr->nsPtr;
    }

    /*
     * If this namespace has a variable resolver, then give it first crack at
     * the variable resolution. It may return a Tcl_Var value, it may signal
     * to continue onward, or it may signal an error.
     */

    if ((cxtNsPtr->varResProc != NULL || iPtr->resolverPtr != NULL)
	    && !(flags & TCL_AVOID_RESOLVERS)) {
	resPtr = iPtr->resolverPtr;
	if (cxtNsPtr->varResProc) {
	    result = cxtNsPtr->varResProc(interp, varName,
		    (Tcl_Namespace *) cxtNsPtr, flags, &var);
	} else {
	    result = TCL_CONTINUE;
	}

	while (result == TCL_CONTINUE && resPtr) {
	    if (resPtr->varResProc) {
		result = resPtr->varResProc(interp, varName,
			(Tcl_Namespace *) cxtNsPtr, flags, &var);
	    }
	    resPtr = resPtr->nextPtr;
	}

	if (result == TCL_OK) {
	    return (Var *) var;
	} else if (result != TCL_CONTINUE) {
	    return NULL;
	}
    }

    /*
     * Look up varName. Look it up as either a namespace variable or as a
     * local variable in a procedure call frame (varFramePtr). Interpret
     * varName as a namespace variable if:
     *    1) so requested by a TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY flag,
     *    2) there is no active frame (we're at the global :: scope),
     *    3) the active frame was pushed to define the namespace context for a
     *	     "namespace eval" or "namespace inscope" command,
     *    4) the name has namespace qualifiers ("::"s).
     * Otherwise, if varName is a local variable, search first in the frame's
     * array of compiler-allocated local variables, then in its hashtable for
     * runtime-created local variables.
     *
     * If create and the variable isn't found, create the variable and, if
     * necessary, create varFramePtr's local var hashtable.
     */

    if (((flags & (TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY)) != 0)
	    || !HasLocalVars(varFramePtr)
	    || (strstr(varName, "::") != NULL)) {
	const char *tail;
	int lookGlobal = (flags & TCL_GLOBAL_ONLY)
		|| (cxtNsPtr == iPtr->globalNsPtr)
		|| ((*varName == ':') && (*(varName+1) == ':'));

	if (lookGlobal) {
	    *indexPtr = -1;
	    flags = (flags | TCL_GLOBAL_ONLY) & ~TCL_NAMESPACE_ONLY;
	} else {
	    if (flags & TCL_AVOID_RESOLVERS) {
		flags = (flags | TCL_NAMESPACE_ONLY);
	    }
	    if (flags & TCL_NAMESPACE_ONLY) {
		*indexPtr = -2;
	    }
	}

	/*
	 * Don't pass TCL_LEAVE_ERR_MSG, we may yet create the variable, or
	 * otherwise generate our own error!
	 */

	varPtr = (Var *) ObjFindNamespaceVar(interp, varNamePtr,
		(Tcl_Namespace *) cxtNsPtr,
		(flags | TCL_AVOID_RESOLVERS) & ~TCL_LEAVE_ERR_MSG);
	if (varPtr == NULL) {
	    Tcl_Obj *tailPtr;

	    if (create) {	/* Var wasn't found so create it. */
		TclGetNamespaceForQualName(interp, varName, cxtNsPtr,
			flags, &varNsPtr, &dummy1Ptr, &dummy2Ptr, &tail);
		if (varNsPtr == NULL) {
		    *errMsgPtr = badNamespace;
		    return NULL;
		} else if (tail == NULL) {
		    *errMsgPtr = missingName;
		    return NULL;
		}
		if (tail != varName) {
		    tailPtr = Tcl_NewStringObj(tail, -1);
		} else {
		    tailPtr = varNamePtr;
		}
		varPtr = VarHashCreateVar(&varNsPtr->varTable, tailPtr,
			&isNew);
		if (lookGlobal) {
		    /*
		     * The variable was created starting from the global
		     * namespace: a global reference is returned even if it
		     * wasn't explicitly requested.
		     */

		    *indexPtr = -1;
		} else {
		    *indexPtr = -2;
		}
	    } else {		/* Var wasn't found and not to create it. */
		*errMsgPtr = noSuchVar;
		return NULL;
	    }
	}
    } else {			/* Local var: look in frame varFramePtr. */
	int localCt = varFramePtr->numCompiledLocals;

	if (localCt > 0) {
	    Tcl_Obj **objPtrPtr = &varFramePtr->localCachePtr->varName0;
	    const char *localNameStr;
	    int localLen;

	    for (i=0 ; i<localCt ; i++, objPtrPtr++) {
		register Tcl_Obj *objPtr = *objPtrPtr;

		if (objPtr) {
		    localNameStr = TclGetStringFromObj(objPtr, &localLen);

		    if ((varLen == localLen) && (varName[0] == localNameStr[0])
			&& !memcmp(varName, localNameStr, varLen)) {
			*indexPtr = i;
			return (Var *) &varFramePtr->compiledLocals[i];
		    }
		}
	    }
	}
	tablePtr = varFramePtr->varTablePtr;
	if (create) {
	    if (tablePtr == NULL) {
		tablePtr = ckalloc(sizeof(TclVarHashTable));
		TclInitVarHashTable(tablePtr, NULL);
		varFramePtr->varTablePtr = tablePtr;
	    }
	    varPtr = VarHashCreateVar(tablePtr, varNamePtr, &isNew);
	} else {
	    varPtr = NULL;
	    if (tablePtr != NULL) {
		varPtr = VarHashFindVar(tablePtr, varNamePtr);
	    }
	    if (varPtr == NULL) {
		*errMsgPtr = noSuchVar;
	    }
	}
    }
    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclLookupArrayElement --
 *
 *	This function is used to locate a variable which is in an array's
 *	hashtable given a pointer to the array's Var structure and the
 *	element's name.
 *
 * Results:
 *	The return value is a pointer to the variable structure , or NULL if
 *	the variable couldn't be found.
 *
 *	If arrayPtr points to a variable that isn't an array and createPart1
 *	is 1, the corresponding variable will be converted to an array.
 *	Otherwise, NULL is returned and an error message is left in the
 *	interp's result if TCL_LEAVE_ERR_MSG is set in flags.
 *
 *	If the variable is not found and createPart2 is 1, the variable is
 *	created. Otherwise, NULL is returned and an error message is left in
 *	the interp's result if TCL_LEAVE_ERR_MSG is set in flags.
 *
 *	Note: it's possible for the variable returned to be VAR_UNDEFINED even
 *	if createPart1 or createPart2 are 1 (these only cause the hash table
 *	entry or array to be created). For example, the variable might be a
 *	global that has been unset but is still referenced by a procedure, or
 *	a variable that has been unset but it only being kept in existence (if
 *	VAR_UNDEFINED) by a trace.
 *
 * Side effects:
 *	The variable at arrayPtr may be converted to be an array if
 *	createPart1 is 1. A new hashtable entry may be created if createPart2
 *	is 1.
 *	When createElem is 1, callers must incr elNamePtr if they plan
 *	to decr it.
 *
 *----------------------------------------------------------------------
 */

Var *
TclLookupArrayElement(
    Tcl_Interp *interp,		/* Interpreter to use for lookup. */
    Tcl_Obj *arrayNamePtr,	/* This is the name of the array, or NULL if
				 * index>= 0. */
    Tcl_Obj *elNamePtr,		/* Name of element within array. */
    const int flags,		/* Only TCL_LEAVE_ERR_MSG bit matters. */
    const char *msg,		/* Verb to use in error messages, e.g. "read"
				 * or "set". Only needed if TCL_LEAVE_ERR_MSG
				 * is set in flags. */
    const int createArray,	/* If 1, transform arrayName to be an array if
				 * it isn't one yet and the transformation is
				 * possible. If 0, return error if it isn't
				 * already an array. */
    const int createElem,	/* If 1, create hash table entry for the
				 * element, if it doesn't already exist. If 0,
				 * return error if it doesn't exist. */
    Var *arrayPtr,		/* Pointer to the array's Var structure. */
    int index)			/* If >=0, the index of the local array. */
{
    int isNew;
    Var *varPtr;
    TclVarHashTable *tablePtr;
    Namespace *nsPtr;

    /*
     * We're dealing with an array element. Make sure the variable is an array
     * and look up the element (create the element if desired).
     */

    if (TclIsVarUndefined(arrayPtr) && !TclIsVarArrayElement(arrayPtr)) {
	if (!createArray) {
	    if (flags & TCL_LEAVE_ERR_MSG) {
		TclObjVarErrMsg(interp, arrayNamePtr, elNamePtr, msg,
			noSuchVar, index);
		Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARNAME",
			arrayNamePtr?TclGetString(arrayNamePtr):NULL, NULL);
	    }
	    return NULL;
	}

	/*
	 * Make sure we are not resurrecting a namespace variable from a
	 * deleted namespace!
	 */

	if (TclIsVarDeadHash(arrayPtr)) {
	    if (flags & TCL_LEAVE_ERR_MSG) {
		TclObjVarErrMsg(interp, arrayNamePtr, elNamePtr, msg,
			danglingVar, index);
		Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARNAME",
			arrayNamePtr?TclGetString(arrayNamePtr):NULL, NULL);
	    }
	    return NULL;
	}

	TclSetVarArray(arrayPtr);
	tablePtr = ckalloc(sizeof(TclVarHashTable));
	arrayPtr->value.tablePtr = tablePtr;

	if (TclIsVarInHash(arrayPtr) && TclGetVarNsPtr(arrayPtr)) {
	    nsPtr = TclGetVarNsPtr(arrayPtr);
	} else {
	    nsPtr = NULL;
	}
	TclInitVarHashTable(arrayPtr->value.tablePtr, nsPtr);
    } else if (!TclIsVarArray(arrayPtr)) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    TclObjVarErrMsg(interp, arrayNamePtr, elNamePtr, msg, needArray,
		    index);
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARNAME",
		    arrayNamePtr?TclGetString(arrayNamePtr):NULL, NULL);
	}
	return NULL;
    }

    if (createElem) {
	varPtr = VarHashCreateVar(arrayPtr->value.tablePtr, elNamePtr,
		&isNew);
	if (isNew) {
	    if (arrayPtr->flags & VAR_SEARCH_ACTIVE) {
		DeleteSearches((Interp *) interp, arrayPtr);
	    }
	    TclSetVarArrayElement(varPtr);
	}
    } else {
	varPtr = VarHashFindVar(arrayPtr->value.tablePtr, elNamePtr);
	if (varPtr == NULL) {
	    if (flags & TCL_LEAVE_ERR_MSG) {
		TclObjVarErrMsg(interp, arrayNamePtr, elNamePtr, msg,
			noSuchElement, index);
		Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ELEMENT",
			TclGetString(elNamePtr), NULL);
	    }
	}
    }
    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVar --
 *
 *	Return the value of a Tcl variable as a string.
 *
 * Results:
 *	The return value points to the current value of varName as a string.
 *	If the variable is not defined or can't be read because of a clash in
 *	array usage then a NULL pointer is returned and an error message is
 *	left in the interp's result if the TCL_LEAVE_ERR_MSG flag is set.
 *	Note: the return value is only valid up until the next change to the
 *	variable; if you depend on the value lasting longer than that, then
 *	make yourself a private copy.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_GetVar
const char *
Tcl_GetVar(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    const char *varName,	/* Name of a variable in interp. */
    int flags)			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY or TCL_LEAVE_ERR_MSG
				 * bits. */
{
    Tcl_Obj *varNamePtr = Tcl_NewStringObj(varName, -1);
    Tcl_Obj *resultPtr = Tcl_ObjGetVar2(interp, varNamePtr, NULL, flags);

    TclDecrRefCount(varNamePtr);

    if (resultPtr == NULL) {
	return NULL;
    }
    return TclGetString(resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVar2 --
 *
 *	Return the value of a Tcl variable as a string, given a two-part name
 *	consisting of array name and element within array.
 *
 * Results:
 *	The return value points to the current value of the variable given by
 *	part1 and part2 as a string. If the specified variable doesn't exist,
 *	or if there is a clash in array usage, then NULL is returned and a
 *	message will be left in the interp's result if the TCL_LEAVE_ERR_MSG
 *	flag is set. Note: the return value is only valid up until the next
 *	change to the variable; if you depend on the value lasting longer than
 *	that, then make yourself a private copy.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_GetVar2(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    const char *part1,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    const char *part2,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    int flags)			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY and TCL_LEAVE_ERR_MSG *
				 * bits. */
{
    Tcl_Obj *resultPtr;
    Tcl_Obj *part2Ptr = NULL, *part1Ptr = Tcl_NewStringObj(part1, -1);

    if (part2) {
	part2Ptr = Tcl_NewStringObj(part2, -1);
	Tcl_IncrRefCount(part2Ptr);
    }

    resultPtr = Tcl_ObjGetVar2(interp, part1Ptr, part2Ptr, flags);

    Tcl_DecrRefCount(part1Ptr);
    if (part2Ptr) {
	Tcl_DecrRefCount(part2Ptr);
    }
    if (resultPtr == NULL) {
	return NULL;
    }
    return TclGetString(resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVar2Ex --
 *
 *	Return the value of a Tcl variable as a Tcl object, given a two-part
 *	name consisting of array name and element within array.
 *
 * Results:
 *	The return value points to the current object value of the variable
 *	given by part1Ptr and part2Ptr. If the specified variable doesn't
 *	exist, or if there is a clash in array usage, then NULL is returned
 *	and a message will be left in the interpreter's result if the
 *	TCL_LEAVE_ERR_MSG flag is set.
 *
 * Side effects:
 *	The ref count for the returned object is _not_ incremented to reflect
 *	the returned reference; if you want to keep a reference to the object
 *	you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_GetVar2Ex(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    const char *part1,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    const char *part2,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    int flags)			/* OR-ed combination of TCL_GLOBAL_ONLY, and
				 * TCL_LEAVE_ERR_MSG bits. */
{
    Tcl_Obj *resPtr, *part2Ptr = NULL, *part1Ptr = Tcl_NewStringObj(part1, -1);

    if (part2) {
	part2Ptr = Tcl_NewStringObj(part2, -1);
	Tcl_IncrRefCount(part2Ptr);
    }

    resPtr = Tcl_ObjGetVar2(interp, part1Ptr, part2Ptr, flags);

    Tcl_DecrRefCount(part1Ptr);
    if (part2Ptr) {
	Tcl_DecrRefCount(part2Ptr);
    }

    return resPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ObjGetVar2 --
 *
 *	Return the value of a Tcl variable as a Tcl object, given a two-part
 *	name consisting of array name and element within array.
 *
 * Results:
 *	The return value points to the current object value of the variable
 *	given by part1Ptr and part2Ptr. If the specified variable doesn't
 *	exist, or if there is a clash in array usage, then NULL is returned
 *	and a message will be left in the interpreter's result if the
 *	TCL_LEAVE_ERR_MSG flag is set.
 *
 * Side effects:
 *	The ref count for the returned object is _not_ incremented to reflect
 *	the returned reference; if you want to keep a reference to the object
 *	you must increment its ref count yourself.
 *
 *	Callers must incr part2Ptr if they plan to decr it.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ObjGetVar2(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    register Tcl_Obj *part1Ptr,	/* Points to an object holding the name of an
				 * array (if part2 is non-NULL) or the name of
				 * a variable. */
    register Tcl_Obj *part2Ptr,	/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    int flags)			/* OR-ed combination of TCL_GLOBAL_ONLY and
				 * TCL_LEAVE_ERR_MSG bits. */
{
    Var *varPtr, *arrayPtr;

    /*
     * Filter to pass through only the flags this interface supports.
     */

    flags &= (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY|TCL_LEAVE_ERR_MSG);
    varPtr = TclObjLookupVarEx(interp, part1Ptr, part2Ptr, flags, "read",
	    /*createPart1*/ 0, /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return NULL;
    }

    return TclPtrGetVarIdx(interp, varPtr, arrayPtr, part1Ptr, part2Ptr,
	    flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrGetVar --
 *
 *	Return the value of a Tcl variable as a Tcl object, given the pointers
 *	to the variable's (and possibly containing array's) VAR structure.
 *
 * Results:
 *	The return value points to the current object value of the variable
 *	given by varPtr. If the specified variable doesn't exist, or if there
 *	is a clash in array usage, then NULL is returned and a message will be
 *	left in the interpreter's result if the TCL_LEAVE_ERR_MSG flag is set.
 *
 * Side effects:
 *	The ref count for the returned object is _not_ incremented to reflect
 *	the returned reference; if you want to keep a reference to the object
 *	you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrGetVar(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    Tcl_Var varPtr,		/* The variable to be read.*/
    Tcl_Var arrayPtr,		/* NULL for scalar variables, pointer to the
				 * containing array otherwise. */
    Tcl_Obj *part1Ptr,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    Tcl_Obj *part2Ptr,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    const int flags)		/* OR-ed combination of TCL_GLOBAL_ONLY, and
				 * TCL_LEAVE_ERR_MSG bits. */
{
    if (varPtr == NULL) {
	Tcl_Panic("varPtr must not be NULL");
    }
    if (part1Ptr == NULL) {
	Tcl_Panic("part1Ptr must not be NULL");
    }
    return TclPtrGetVarIdx(interp, (Var *) varPtr, (Var *) arrayPtr,
	    part1Ptr, part2Ptr, flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrGetVarIdx --
 *
 *	Return the value of a Tcl variable as a Tcl object, given the pointers
 *	to the variable's (and possibly containing array's) VAR structure.
 *
 * Results:
 *	The return value points to the current object value of the variable
 *	given by varPtr. If the specified variable doesn't exist, or if there
 *	is a clash in array usage, then NULL is returned and a message will be
 *	left in the interpreter's result if the TCL_LEAVE_ERR_MSG flag is set.
 *
 * Side effects:
 *	The ref count for the returned object is _not_ incremented to reflect
 *	the returned reference; if you want to keep a reference to the object
 *	you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrGetVarIdx(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    register Var *varPtr,	/* The variable to be read.*/
    Var *arrayPtr,		/* NULL for scalar variables, pointer to the
				 * containing array otherwise. */
    Tcl_Obj *part1Ptr,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    Tcl_Obj *part2Ptr,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    const int flags,		/* OR-ed combination of TCL_GLOBAL_ONLY, and
				 * TCL_LEAVE_ERR_MSG bits. */
    int index)			/* Index into the local variable table of the
				 * variable, or -1. Only used when part1Ptr is
				 * NULL. */
{
    Interp *iPtr = (Interp *) interp;
    const char *msg;

    /*
     * Invoke any read traces that have been set for the variable.
     */

    if ((varPtr->flags & VAR_TRACED_READ)
	    || (arrayPtr && (arrayPtr->flags & VAR_TRACED_READ))) {
	if (TCL_ERROR == TclObjCallVarTraces(iPtr, arrayPtr, varPtr,
		part1Ptr, part2Ptr,
		(flags & (TCL_NAMESPACE_ONLY|TCL_GLOBAL_ONLY))
		| TCL_TRACE_READS, (flags & TCL_LEAVE_ERR_MSG), index)) {
	    goto errorReturn;
	}
    }

    /*
     * Return the element if it's an existing scalar variable.
     */

    if (TclIsVarScalar(varPtr) && !TclIsVarUndefined(varPtr)) {
	return varPtr->value.objPtr;
    }

    if (flags & TCL_LEAVE_ERR_MSG) {
	if (TclIsVarUndefined(varPtr) && arrayPtr
		&& !TclIsVarUndefined(arrayPtr)) {
	    msg = noSuchElement;
	} else if (TclIsVarArray(varPtr)) {
	    msg = isArray;
	} else {
	    msg = noSuchVar;
	}
	TclObjVarErrMsg(interp, part1Ptr, part2Ptr, "read", msg, index);
    }

    /*
     * An error. If the variable doesn't exist anymore and no-one's using it,
     * then free up the relevant structures and hash table entries.
     */

  errorReturn:
    Tcl_SetErrorCode(interp, "TCL", "READ", "VARNAME", NULL);
    if (TclIsVarUndefined(varPtr)) {
	TclCleanupVar(varPtr, arrayPtr);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetObjCmd --
 *
 *	This function is invoked to process the "set" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result value.
 *
 * Side effects:
 *	A variable's value may be changed.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_SetObjCmd(
    ClientData dummy,		/* Not used. */
    register Tcl_Interp *interp,/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *varValueObj;

    if (objc == 2) {
	varValueObj = Tcl_ObjGetVar2(interp, objv[1], NULL,TCL_LEAVE_ERR_MSG);
	if (varValueObj == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, varValueObj);
	return TCL_OK;
    } else if (objc == 3) {
	varValueObj = Tcl_ObjSetVar2(interp, objv[1], NULL, objv[2],
		TCL_LEAVE_ERR_MSG);
	if (varValueObj == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, varValueObj);
	return TCL_OK;
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "varName ?newValue?");
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetVar --
 *
 *	Change the value of a variable.
 *
 * Results:
 *	Returns a pointer to the malloc'ed string which is the character
 *	representation of the variable's new value. The caller must not modify
 *	this string. If the write operation was disallowed then NULL is
 *	returned; if the TCL_LEAVE_ERR_MSG flag is set, then an explanatory
 *	message will be left in the interp's result. Note that the returned
 *	string may not be the same as newValue; this is because variable
 *	traces may modify the variable's value.
 *
 * Side effects:
 *	If varName is defined as a local or global variable in interp, its
 *	value is changed to newValue. If varName isn't currently defined, then
 *	a new global variable by that name is created.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_SetVar
const char *
Tcl_SetVar(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    const char *varName,	/* Name of a variable in interp. */
    const char *newValue,	/* New value for varName. */
    int flags)			/* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_APPEND_VALUE, TCL_LIST_ELEMENT,
				 * TCL_LEAVE_ERR_MSG. */
{
    Tcl_Obj *varValuePtr, *varNamePtr = Tcl_NewStringObj(varName, -1);

    Tcl_IncrRefCount(varNamePtr);
    varValuePtr = Tcl_ObjSetVar2(interp, varNamePtr, NULL,
	    Tcl_NewStringObj(newValue, -1), flags);
    Tcl_DecrRefCount(varNamePtr);

    if (varValuePtr == NULL) {
	return NULL;
    }
    return TclGetString(varValuePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetVar2 --
 *
 *	Given a two-part variable name, which may refer either to a scalar
 *	variable or an element of an array, change the value of the variable.
 *	If the named scalar or array or element doesn't exist then create one.
 *
 * Results:
 *	Returns a pointer to the malloc'ed string which is the character
 *	representation of the variable's new value. The caller must not modify
 *	this string. If the write operation was disallowed because an array
 *	was expected but not found (or vice versa), then NULL is returned; if
 *	the TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will be
 *	left in the interp's result. Note that the returned string may not be
 *	the same as newValue; this is because variable traces may modify the
 *	variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new one is created.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_SetVar2(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    const char *part1,		/* If part2 is NULL, this is name of scalar
				 * variable. Otherwise it is the name of an
				 * array. */
    const char *part2,		/* Name of an element within an array, or
				 * NULL. */
    const char *newValue,	/* New value for variable. */
    int flags)			/* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_APPEND_VALUE, TCL_LIST_ELEMENT, or
				 * TCL_LEAVE_ERR_MSG. */
{
    Tcl_Obj *varValuePtr = Tcl_SetVar2Ex(interp, part1, part2,
	    Tcl_NewStringObj(newValue, -1), flags);

    if (varValuePtr == NULL) {
	return NULL;
    }
    return TclGetString(varValuePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetVar2Ex --
 *
 *	Given a two-part variable name, which may refer either to a scalar
 *	variable or an element of an array, change the value of the variable
 *	to a new Tcl object value. If the named scalar or array or element
 *	doesn't exist then create one.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the write operation was disallowed because an array was
 *	expected but not found (or vice versa), then NULL is returned; if the
 *	TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will be
 *	left in the interpreter's result. Note that the returned object may
 *	not be the same one referenced by newValuePtr; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new variable is created.
 *
 *	The reference count is decremented for any old value of the variable
 *	and incremented for its new value. If the new value for the variable
 *	is not the same one referenced by newValuePtr (perhaps as a result of
 *	a variable trace), then newValuePtr's ref count is left unchanged by
 *	Tcl_SetVar2Ex. newValuePtr's ref count is also left unchanged if we
 *	are appending it as a string value: that is, if "flags" includes
 *	TCL_APPEND_VALUE but not TCL_LIST_ELEMENT.
 *
 *	The reference count for the returned object is _not_ incremented: if
 *	you want to keep a reference to the object you must increment its ref
 *	count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_SetVar2Ex(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be found. */
    const char *part1,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    const char *part2,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    Tcl_Obj *newValuePtr,	/* New value for variable. */
    int flags)			/* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_APPEND_VALUE, TCL_LIST_ELEMENT or
				 * TCL_LEAVE_ERR_MSG. */
{
    Tcl_Obj *resPtr, *part2Ptr = NULL, *part1Ptr = Tcl_NewStringObj(part1, -1);

    Tcl_IncrRefCount(part1Ptr);
    if (part2) {
	part2Ptr = Tcl_NewStringObj(part2, -1);
	Tcl_IncrRefCount(part2Ptr);
    }

    resPtr = Tcl_ObjSetVar2(interp, part1Ptr, part2Ptr, newValuePtr, flags);

    Tcl_DecrRefCount(part1Ptr);
    if (part2Ptr) {
	Tcl_DecrRefCount(part2Ptr);
    }

    return resPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ObjSetVar2 --
 *
 *	This function is the same as Tcl_SetVar2Ex above, except the variable
 *	names are passed in Tcl object instead of strings.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the write operation was disallowed because an array was
 *	expected but not found (or vice versa), then NULL is returned; if the
 *	TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will be
 *	left in the interpreter's result. Note that the returned object may
 *	not be the same one referenced by newValuePtr; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new variable is created.
 *	Callers must Incr part1Ptr if they plan to Decr it.
 *	Callers must Incr part2Ptr if they plan to Decr it.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ObjSetVar2(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be found. */
    register Tcl_Obj *part1Ptr,	/* Points to an object holding the name of an
				 * array (if part2 is non-NULL) or the name of
				 * a variable. */
    register Tcl_Obj *part2Ptr,	/* If non-NULL, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    Tcl_Obj *newValuePtr,	/* New value for variable. */
    int flags)			/* Various flags that tell how to set value:
				 * any of TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_APPEND_VALUE, TCL_LIST_ELEMENT, or
				 * TCL_LEAVE_ERR_MSG. */
{
    Var *varPtr, *arrayPtr;

    /*
     * Filter to pass through only the flags this interface supports.
     */

    flags &= (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY|TCL_LEAVE_ERR_MSG
	    |TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
    varPtr = TclObjLookupVarEx(interp, part1Ptr, part2Ptr, flags, "set",
	    /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	if (newValuePtr->refCount == 0) {
	    Tcl_DecrRefCount(newValuePtr);
	}
	return NULL;
    }

    return TclPtrSetVarIdx(interp, varPtr, arrayPtr, part1Ptr, part2Ptr,
	    newValuePtr, flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrSetVar --
 *
 *	This function is the same as Tcl_SetVar2Ex above, except that it
 *	requires pointers to the variable's Var structs in addition to the
 *	variable names.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the write operation was disallowed because an array was
 *	expected but not found (or vice versa), then NULL is returned; if the
 *	TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will be
 *	left in the interpreter's result. Note that the returned object may
 *	not be the same one referenced by newValuePtr; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new variable is created.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrSetVar(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    Tcl_Var varPtr,		/* Reference to the variable to set. */
    Tcl_Var arrayPtr,		/* Reference to the array containing the
				 * variable, or NULL if the variable is a
				 * scalar. */
    Tcl_Obj *part1Ptr,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    Tcl_Obj *part2Ptr,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    Tcl_Obj *newValuePtr,	/* New value for variable. */
    const int flags)		/* OR-ed combination of TCL_GLOBAL_ONLY, and
				 * TCL_LEAVE_ERR_MSG bits. */
{
    if (varPtr == NULL) {
	Tcl_Panic("varPtr must not be NULL");
    }
    if (part1Ptr == NULL) {
	Tcl_Panic("part1Ptr must not be NULL");
    }
    if (newValuePtr == NULL) {
	Tcl_Panic("newValuePtr must not be NULL");
    }
    return TclPtrSetVarIdx(interp, (Var *) varPtr, (Var *) arrayPtr,
	    part1Ptr, part2Ptr, newValuePtr, flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrSetVarIdx --
 *
 *	This function is the same as Tcl_SetVar2Ex above, except that it
 *	requires pointers to the variable's Var structs in addition to the
 *	variable names.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the write operation was disallowed because an array was
 *	expected but not found (or vice versa), then NULL is returned; if the
 *	TCL_LEAVE_ERR_MSG flag is set, then an explanatory message will be
 *	left in the interpreter's result. Note that the returned object may
 *	not be the same one referenced by newValuePtr; this is because
 *	variable traces may modify the variable's value.
 *
 * Side effects:
 *	The value of the given variable is set. If either the array or the
 *	entry didn't exist then a new variable is created.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrSetVarIdx(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be looked up. */
    register Var *varPtr,	/* Reference to the variable to set. */
    Var *arrayPtr,		/* Reference to the array containing the
				 * variable, or NULL if the variable is a
				 * scalar. */
    Tcl_Obj *part1Ptr,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. NULL if the 'index'
				 * parameter is >= 0 */
    Tcl_Obj *part2Ptr,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    Tcl_Obj *newValuePtr,	/* New value for variable. */
    const int flags,		/* OR-ed combination of TCL_GLOBAL_ONLY, and
				 * TCL_LEAVE_ERR_MSG bits. */
    int index)			/* Index of local var where part1 is to be
				 * found. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *oldValuePtr;
    Tcl_Obj *resultPtr = NULL;
    int result;
    int cleanupOnEarlyError = (newValuePtr->refCount == 0);

    /*
     * If the variable is in a hashtable and its hPtr field is NULL, then we
     * may have an upvar to an array element where the array was deleted or an
     * upvar to a namespace variable whose namespace was deleted. Generate an
     * error (allowing the variable to be reset would screw up our storage
     * allocation and is meaningless anyway).
     */

    if (TclIsVarDeadHash(varPtr)) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    if (TclIsVarArrayElement(varPtr)) {
		TclObjVarErrMsg(interp, part1Ptr, part2Ptr, "set",
			danglingElement, index);
		Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ELEMENT", NULL);
	    } else {
		TclObjVarErrMsg(interp, part1Ptr, part2Ptr, "set",
			danglingVar, index);
		Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARNAME", NULL);
	    }
	}
	goto earlyError;
    }

    /*
     * It's an error to try to set an array variable itself.
     */

    if (TclIsVarArray(varPtr)) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    TclObjVarErrMsg(interp, part1Ptr, part2Ptr, "set", isArray,index);
	    Tcl_SetErrorCode(interp, "TCL", "WRITE", "ARRAY", NULL);
	}
	goto earlyError;
    }

    /*
     * Invoke any read traces that have been set for the variable if it is
     * requested. This was done for INST_LAPPEND_* but that was inconsistent
     * with the non-bc instruction, and would cause failures trying to
     * lappend to any non-existing ::env var, which is inconsistent with
     * documented behavior. [Bug #3057639].
     */

    if ((flags & TCL_TRACE_READS) && ((varPtr->flags & VAR_TRACED_READ)
	    || (arrayPtr && (arrayPtr->flags & VAR_TRACED_READ)))) {
	if (TCL_ERROR == TclObjCallVarTraces(iPtr, arrayPtr, varPtr,
		part1Ptr, part2Ptr,
		TCL_TRACE_READS, (flags & TCL_LEAVE_ERR_MSG), index)) {
	    goto earlyError;
	}
    }

    /*
     * Set the variable's new value. If appending, append the new value to the
     * variable, either as a list element or as a string. Also, if appending,
     * then if the variable's old value is unshared we can modify it directly,
     * otherwise we must create a new copy to modify: this is "copy on write".
     */

    oldValuePtr = varPtr->value.objPtr;
    if (flags & TCL_LIST_ELEMENT && !(flags & TCL_APPEND_VALUE)) {
	varPtr->value.objPtr = NULL;
    }
    if (flags & (TCL_APPEND_VALUE|TCL_LIST_ELEMENT)) {
	if (flags & TCL_LIST_ELEMENT) {		/* Append list element. */
	    if (oldValuePtr == NULL) {
		TclNewObj(oldValuePtr);
		varPtr->value.objPtr = oldValuePtr;
		Tcl_IncrRefCount(oldValuePtr);	/* Since var is referenced. */
	    } else if (Tcl_IsShared(oldValuePtr)) {
		varPtr->value.objPtr = Tcl_DuplicateObj(oldValuePtr);
		TclDecrRefCount(oldValuePtr);
		oldValuePtr = varPtr->value.objPtr;
		Tcl_IncrRefCount(oldValuePtr);	/* Since var is referenced. */
	    }
	    result = Tcl_ListObjAppendElement(interp, oldValuePtr,
		    newValuePtr);
	    if (result != TCL_OK) {
		goto earlyError;
	    }
	} else {				/* Append string. */
	    /*
	     * We append newValuePtr's bytes but don't change its ref count.
	     */

	    if (oldValuePtr == NULL) {
		varPtr->value.objPtr = newValuePtr;
		Tcl_IncrRefCount(newValuePtr);
	    } else {
		if (Tcl_IsShared(oldValuePtr)) {	/* Append to copy. */
		    varPtr->value.objPtr = Tcl_DuplicateObj(oldValuePtr);

		    TclContinuationsCopy(varPtr->value.objPtr, oldValuePtr);

		    TclDecrRefCount(oldValuePtr);
		    oldValuePtr = varPtr->value.objPtr;
		    Tcl_IncrRefCount(oldValuePtr);	/* Since var is ref */
		}
		Tcl_AppendObjToObj(oldValuePtr, newValuePtr);
		if (newValuePtr->refCount == 0) {
		    Tcl_DecrRefCount(newValuePtr);
		}
	    }
	}
    } else if (newValuePtr != oldValuePtr) {
	/*
	 * In this case we are replacing the value, so we don't need to do
	 * more than swap the objects.
	 */

	varPtr->value.objPtr = newValuePtr;
	Tcl_IncrRefCount(newValuePtr);		/* Var is another ref. */
	if (oldValuePtr != NULL) {
	    TclDecrRefCount(oldValuePtr);	/* Discard old value. */
	}
    }

    /*
     * Invoke any write traces for the variable.
     */

    if ((varPtr->flags & VAR_TRACED_WRITE)
	    || (arrayPtr && (arrayPtr->flags & VAR_TRACED_WRITE))) {
	if (TCL_ERROR == TclObjCallVarTraces(iPtr, arrayPtr, varPtr, part1Ptr,
		part2Ptr, (flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY))
		| TCL_TRACE_WRITES, (flags & TCL_LEAVE_ERR_MSG), index)) {
	    goto cleanup;
	}
    }

    /*
     * Return the variable's value unless the variable was changed in some
     * gross way by a trace (e.g. it was unset and then recreated as an
     * array).
     */

    if (TclIsVarScalar(varPtr) && !TclIsVarUndefined(varPtr)) {
	return varPtr->value.objPtr;
    }

    /*
     * A trace changed the value in some gross way. Return an empty string
     * object.
     */

    resultPtr = iPtr->emptyObjPtr;

    /*
     * If the variable doesn't exist anymore and no-one's using it, then free
     * up the relevant structures and hash table entries.
     */

  cleanup:
    if (resultPtr == NULL) {
	Tcl_SetErrorCode(interp, "TCL", "WRITE", "VARNAME", NULL);
    }
    if (TclIsVarUndefined(varPtr)) {
	TclCleanupVar(varPtr, arrayPtr);
    }
    return resultPtr;

  earlyError:
    if (cleanupOnEarlyError) {
	Tcl_DecrRefCount(newValuePtr);
    }
    goto cleanup;
}

/*
 *----------------------------------------------------------------------
 *
 * TclIncrObjVar2 --
 *
 *	Given a two-part variable name, which may refer either to a scalar
 *	variable or an element of an array, increment the Tcl object value of
 *	the variable by a specified Tcl_Obj increment value.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the specified variable doesn't exist, or there is a clash
 *	in array usage, or an error occurs while executing variable traces,
 *	then NULL is returned and a message will be left in the interpreter's
 *	result.
 *
 * Side effects:
 *	The value of the given variable is incremented by the specified
 *	amount. If either the array or the entry didn't exist then a new
 *	variable is created. The ref count for the returned object is _not_
 *	incremented to reflect the returned reference; if you want to keep a
 *	reference to the object you must increment its ref count yourself.
 *	Callers must Incr part1Ptr if they plan to Decr it.
 *	Callers must Incr part2Ptr if they plan to Decr it.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclIncrObjVar2(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be found. */
    Tcl_Obj *part1Ptr,		/* Points to an object holding the name of an
				 * array (if part2 is non-NULL) or the name of
				 * a variable. */
    Tcl_Obj *part2Ptr,		/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    Tcl_Obj *incrPtr,		/* Amount to be added to variable. */
    int flags)			/* Various flags that tell how to incr value:
				 * any of TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_APPEND_VALUE, TCL_LIST_ELEMENT,
				 * TCL_LEAVE_ERR_MSG. */
{
    Var *varPtr, *arrayPtr;

    varPtr = TclObjLookupVarEx(interp, part1Ptr, part2Ptr, flags, "read",
	    1, 1, &arrayPtr);
    if (varPtr == NULL) {
	Tcl_AddErrorInfo(interp,
		"\n    (reading value of variable to increment)");
	return NULL;
    }
    return TclPtrIncrObjVarIdx(interp, varPtr, arrayPtr, part1Ptr, part2Ptr,
	    incrPtr, flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrIncrObjVar --
 *
 *	Given the pointers to a variable and possible containing array,
 *	increment the Tcl object value of the variable by a Tcl_Obj increment.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the specified variable doesn't exist, or there is a clash
 *	in array usage, or an error occurs while executing variable traces,
 *	then NULL is returned and a message will be left in the interpreter's
 *	result.
 *
 * Side effects:
 *	The value of the given variable is incremented by the specified
 *	amount. If either the array or the entry didn't exist then a new
 *	variable is created. The ref count for the returned object is _not_
 *	incremented to reflect the returned reference; if you want to keep a
 *	reference to the object you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrIncrObjVar(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be found. */
    Tcl_Var varPtr,		/* Reference to the variable to set. */
    Tcl_Var arrayPtr,		/* Reference to the array containing the
				 * variable, or NULL if the variable is a
				 * scalar. */
    Tcl_Obj *part1Ptr,		/* Points to an object holding the name of an
				 * array (if part2 is non-NULL) or the name of
				 * a variable. */
    Tcl_Obj *part2Ptr,		/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    Tcl_Obj *incrPtr,		/* Increment value. */
/* TODO: Which of these flag values really make sense? */
    const int flags)		/* Various flags that tell how to incr value:
				 * any of TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_APPEND_VALUE, TCL_LIST_ELEMENT,
				 * TCL_LEAVE_ERR_MSG. */
{
    if (varPtr == NULL) {
	Tcl_Panic("varPtr must not be NULL");
    }
    if (part1Ptr == NULL) {
	Tcl_Panic("part1Ptr must not be NULL");
    }
    return TclPtrIncrObjVarIdx(interp, (Var *) varPtr, (Var *) arrayPtr,
	    part1Ptr, part2Ptr, incrPtr, flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrIncrObjVarIdx --
 *
 *	Given the pointers to a variable and possible containing array,
 *	increment the Tcl object value of the variable by a Tcl_Obj increment.
 *
 * Results:
 *	Returns a pointer to the Tcl_Obj holding the new value of the
 *	variable. If the specified variable doesn't exist, or there is a clash
 *	in array usage, or an error occurs while executing variable traces,
 *	then NULL is returned and a message will be left in the interpreter's
 *	result.
 *
 * Side effects:
 *	The value of the given variable is incremented by the specified
 *	amount. If either the array or the entry didn't exist then a new
 *	variable is created. The ref count for the returned object is _not_
 *	incremented to reflect the returned reference; if you want to keep a
 *	reference to the object you must increment its ref count yourself.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclPtrIncrObjVarIdx(
    Tcl_Interp *interp,		/* Command interpreter in which variable is to
				 * be found. */
    Var *varPtr,		/* Reference to the variable to set. */
    Var *arrayPtr,		/* Reference to the array containing the
				 * variable, or NULL if the variable is a
				 * scalar. */
    Tcl_Obj *part1Ptr,		/* Points to an object holding the name of an
				 * array (if part2 is non-NULL) or the name of
				 * a variable. */
    Tcl_Obj *part2Ptr,		/* If non-null, points to an object holding
				 * the name of an element in the array
				 * part1Ptr. */
    Tcl_Obj *incrPtr,		/* Increment value. */
/* TODO: Which of these flag values really make sense? */
    const int flags,		/* Various flags that tell how to incr value:
				 * any of TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_APPEND_VALUE, TCL_LIST_ELEMENT,
				 * TCL_LEAVE_ERR_MSG. */
    int index)			/* Index into the local variable table of the
				 * variable, or -1. Only used when part1Ptr is
				 * NULL. */
{
    register Tcl_Obj *varValuePtr;

    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)++;
    }
    varValuePtr = TclPtrGetVarIdx(interp, varPtr, arrayPtr, part1Ptr,
	    part2Ptr, flags, index);
    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)--;
    }
    if (varValuePtr == NULL) {
	varValuePtr = Tcl_NewIntObj(0);
    }
    if (Tcl_IsShared(varValuePtr)) {
	/* Copy on write */
	varValuePtr = Tcl_DuplicateObj(varValuePtr);

	if (TCL_OK == TclIncrObj(interp, varValuePtr, incrPtr)) {
	    return TclPtrSetVarIdx(interp, varPtr, arrayPtr, part1Ptr,
		    part2Ptr, varValuePtr, flags, index);
	} else {
	    Tcl_DecrRefCount(varValuePtr);
	    return NULL;
	}
    } else {
	/* Unshared - can Incr in place */
	if (TCL_OK == TclIncrObj(interp, varValuePtr, incrPtr)) {

	    /*
	     * This seems dumb to write the incremeted value into the var
	     * after we just adjusted the value in place, but the spec for
	     * [incr] requires that write traces fire, and making this call
	     * is the way to make that happen.
	     */

	    return TclPtrSetVarIdx(interp, varPtr, arrayPtr, part1Ptr,
		    part2Ptr, varValuePtr, flags, index);
	} else {
	    return NULL;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnsetVar --
 *
 *	Delete a variable, so that it may not be accessed anymore.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR if
 *	the variable can't be unset. In the event of an error, if the
 *	TCL_LEAVE_ERR_MSG flag is set then an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	If varName is defined as a local or global variable in interp, it is
 *	deleted.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_UnsetVar
int
Tcl_UnsetVar(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    const char *varName,	/* Name of a variable in interp. May be either
				 * a scalar name or an array name or an
				 * element in an array. */
    int flags)			/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY or
				 * TCL_LEAVE_ERR_MSG. */
{
    int result;
    Tcl_Obj *varNamePtr;

    varNamePtr = Tcl_NewStringObj(varName, -1);
    Tcl_IncrRefCount(varNamePtr);

    /*
     * Filter to pass through only the flags this interface supports.
     */

    flags &= (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY|TCL_LEAVE_ERR_MSG);
    result = TclObjUnsetVar2(interp, varNamePtr, NULL, flags);

    Tcl_DecrRefCount(varNamePtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnsetVar2 --
 *
 *	Delete a variable, given a 2-part name.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR if
 *	the variable can't be unset. In the event of an error, if the
 *	TCL_LEAVE_ERR_MSG flag is set then an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	If part1 and part2 indicate a local or global variable in interp, it
 *	is deleted. If part1 is an array name and part2 is NULL, then the
 *	whole array is deleted.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UnsetVar2(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    const char *part1,		/* Name of variable or array. */
    const char *part2,		/* Name of element within array or NULL. */
    int flags)			/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_LEAVE_ERR_MSG. */
{
    int result;
    Tcl_Obj *part2Ptr = NULL, *part1Ptr = Tcl_NewStringObj(part1, -1);

    if (part2) {
	part2Ptr = Tcl_NewStringObj(part2, -1);
    }

    /*
     * Filter to pass through only the flags this interface supports.
     */

    flags &= (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY|TCL_LEAVE_ERR_MSG);
    result = TclObjUnsetVar2(interp, part1Ptr, part2Ptr, flags);

    Tcl_DecrRefCount(part1Ptr);
    if (part2Ptr) {
	Tcl_DecrRefCount(part2Ptr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjUnsetVar2 --
 *
 *	Delete a variable, given a 2-object name.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR if
 *	the variable can't be unset. In the event of an error, if the
 *	TCL_LEAVE_ERR_MSG flag is set then an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	If part1ptr and part2Ptr indicate a local or global variable in
 *	interp, it is deleted. If part1Ptr is an array name and part2Ptr is
 *	NULL, then the whole array is deleted.
 *
 *----------------------------------------------------------------------
 */

int
TclObjUnsetVar2(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    Tcl_Obj *part1Ptr,		/* Name of variable or array. */
    Tcl_Obj *part2Ptr,		/* Name of element within array or NULL. */
    int flags)			/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_LEAVE_ERR_MSG. */
{
    Var *varPtr, *arrayPtr;

    varPtr = TclObjLookupVarEx(interp, part1Ptr, part2Ptr, flags, "unset",
	    /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
    if (varPtr == NULL) {
	return TCL_ERROR;
    }

    return TclPtrUnsetVarIdx(interp, varPtr, arrayPtr, part1Ptr, part2Ptr,
	    flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrUnsetVar --
 *
 *	Delete a variable, given the pointers to the variable's (and possibly
 *	containing array's) VAR structure.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR if
 *	the variable can't be unset. In the event of an error, if the
 *	TCL_LEAVE_ERR_MSG flag is set then an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	If varPtr and arrayPtr indicate a local or global variable in interp,
 *	it is deleted. If varPtr is an array reference and part2Ptr is NULL,
 *	then the whole array is deleted.
 *
 *----------------------------------------------------------------------
 */

int
TclPtrUnsetVar(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    Tcl_Var varPtr,		/* The variable to be unset. */
    Tcl_Var arrayPtr,		/* NULL for scalar variables, pointer to the
				 * containing array otherwise. */
    Tcl_Obj *part1Ptr,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    Tcl_Obj *part2Ptr,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    const int flags)		/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_LEAVE_ERR_MSG. */
{
    if (varPtr == NULL) {
	Tcl_Panic("varPtr must not be NULL");
    }
    if (part1Ptr == NULL) {
	Tcl_Panic("part1Ptr must not be NULL");
    }
    return TclPtrUnsetVarIdx(interp, (Var *) varPtr, (Var *) arrayPtr,
	    part1Ptr, part2Ptr, flags, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrUnsetVarIdx --
 *
 *	Delete a variable, given the pointers to the variable's (and possibly
 *	containing array's) VAR structure.
 *
 * Results:
 *	Returns TCL_OK if the variable was successfully deleted, TCL_ERROR if
 *	the variable can't be unset. In the event of an error, if the
 *	TCL_LEAVE_ERR_MSG flag is set then an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	If varPtr and arrayPtr indicate a local or global variable in interp,
 *	it is deleted. If varPtr is an array reference and part2Ptr is NULL,
 *	then the whole array is deleted.
 *
 *----------------------------------------------------------------------
 */

int
TclPtrUnsetVarIdx(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    register Var *varPtr,	/* The variable to be unset. */
    Var *arrayPtr,		/* NULL for scalar variables, pointer to the
				 * containing array otherwise. */
    Tcl_Obj *part1Ptr,		/* Name of an array (if part2 is non-NULL) or
				 * the name of a variable. */
    Tcl_Obj *part2Ptr,		/* If non-NULL, gives the name of an element
				 * in the array part1. */
    const int flags,		/* OR-ed combination of any of
				 * TCL_GLOBAL_ONLY, TCL_NAMESPACE_ONLY,
				 * TCL_LEAVE_ERR_MSG. */
    int index)			/* Index into the local variable table of the
				 * variable, or -1. Only used when part1Ptr is
				 * NULL. */
{
    Interp *iPtr = (Interp *) interp;
    int result = (TclIsVarUndefined(varPtr)? TCL_ERROR : TCL_OK);

    /*
     * Keep the variable alive until we're done with it. We used to
     * increase/decrease the refCount for each operation, making it hard to
     * find [Bug 735335] - caused by unsetting the variable whose value was
     * the variable's name.
     */

    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)++;
    }

    UnsetVarStruct(varPtr, arrayPtr, iPtr, part1Ptr, part2Ptr, flags, index);

    /*
     * It's an error to unset an undefined variable.
     */

    if (result != TCL_OK) {
	if (flags & TCL_LEAVE_ERR_MSG) {
	    TclObjVarErrMsg(interp, part1Ptr, part2Ptr, "unset",
		    ((arrayPtr == NULL) ? noSuchVar : noSuchElement), index);
	    Tcl_SetErrorCode(interp, "TCL", "UNSET", "VARNAME", NULL);
	}
    }

    /*
     * Finally, if the variable is truly not in use then free up its Var
     * structure and remove it from its hash table, if any. The ref count of
     * its value object, if any, was decremented above.
     */

    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)--;
	CleanupVar(varPtr, arrayPtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * UnsetVarStruct --
 *
 *	Unset and delete a variable. This does the internal work for
 *	TclObjUnsetVar2 and TclDeleteNamespaceVars, which call here for each
 *	variable to be unset and deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the arguments indicate a local or global variable in iPtr, it is
 *	unset and deleted.
 *
 *----------------------------------------------------------------------
 */

static void
UnsetVarStruct(
    Var *varPtr,
    Var *arrayPtr,
    Interp *iPtr,
    Tcl_Obj *part1Ptr,
    Tcl_Obj *part2Ptr,
    int flags,
    int index)
{
    Var dummyVar;
    int traced = TclIsVarTraced(varPtr)
	    || (arrayPtr && (arrayPtr->flags & VAR_TRACED_UNSET));

    if (arrayPtr && (arrayPtr->flags & VAR_SEARCH_ACTIVE)) {
	DeleteSearches(iPtr, arrayPtr);
    } else if (varPtr->flags & VAR_SEARCH_ACTIVE) {
	DeleteSearches(iPtr, varPtr);
    }

    /*
     * The code below is tricky, because of the possibility that a trace
     * function might try to access a variable being deleted. To handle this
     * situation gracefully, do things in three steps:
     * 1. Copy the contents of the variable to a dummy variable structure, and
     *    mark the original Var structure as undefined.
     * 2. Invoke traces and clean up the variable, using the dummy copy.
     * 3. If at the end of this the original variable is still undefined and
     *    has no outstanding references, then delete it (but it could have
     *    gotten recreated by a trace).
     */

    dummyVar = *varPtr;
    dummyVar.flags &= ~VAR_ALL_HASH;
    TclSetVarUndefined(varPtr);

    /*
     * Call trace functions for the variable being deleted. Then delete its
     * traces. Be sure to abort any other traces for the variable that are
     * still pending. Special tricks:
     * 1. We need to increment varPtr's refCount around this: TclCallVarTraces
     *    will use dummyVar so it won't increment varPtr's refCount itself.
     * 2. Turn off the VAR_TRACE_ACTIVE flag in dummyVar: we want to call
     *    unset traces even if other traces are pending.
     */

    if (traced) {
	VarTrace *tracePtr = NULL;
	Tcl_HashEntry *tPtr;

	if (TclIsVarTraced(&dummyVar)) {
	    /*
	     * Transfer any existing traces on var, IF there are unset traces.
	     * Otherwise just delete them.
	     */

	    int isNew;

	    tPtr = Tcl_FindHashEntry(&iPtr->varTraces, varPtr);
	    tracePtr = Tcl_GetHashValue(tPtr);
	    varPtr->flags &= ~VAR_ALL_TRACES;
	    Tcl_DeleteHashEntry(tPtr);
	    if (dummyVar.flags & VAR_TRACED_UNSET) {
		tPtr = Tcl_CreateHashEntry(&iPtr->varTraces,
			&dummyVar, &isNew);
		Tcl_SetHashValue(tPtr, tracePtr);
	    }
	}

	if ((dummyVar.flags & VAR_TRACED_UNSET)
		|| (arrayPtr && (arrayPtr->flags & VAR_TRACED_UNSET))) {
	    dummyVar.flags &= ~VAR_TRACE_ACTIVE;
	    TclObjCallVarTraces(iPtr, arrayPtr, &dummyVar, part1Ptr, part2Ptr,
		    (flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY))
			    | TCL_TRACE_UNSETS,
		    /* leaveErrMsg */ 0, index);

	    /*
	     * The traces that we just called may have triggered a change in
	     * the set of traces. If so, reload the traces to manipulate.
	     */

	    tracePtr = NULL;
	    if (TclIsVarTraced(&dummyVar)) {
		tPtr = Tcl_FindHashEntry(&iPtr->varTraces, &dummyVar);
		if (tPtr) {
		    tracePtr = Tcl_GetHashValue(tPtr);
		    Tcl_DeleteHashEntry(tPtr);
		}
	    }
	}

	if (tracePtr) {
	    ActiveVarTrace *activePtr;

	    while (tracePtr) {
		VarTrace *prevPtr = tracePtr;

		tracePtr = tracePtr->nextPtr;
		prevPtr->nextPtr = NULL;
		Tcl_EventuallyFree(prevPtr, TCL_DYNAMIC);
	    }
	    for (activePtr = iPtr->activeVarTracePtr;  activePtr != NULL;
		    activePtr = activePtr->nextPtr) {
		if (activePtr->varPtr == varPtr) {
		    activePtr->nextTracePtr = NULL;
		}
	    }
	    dummyVar.flags &= ~VAR_ALL_TRACES;
	}
    }

    if (TclIsVarScalar(&dummyVar) && (dummyVar.value.objPtr != NULL)) {
	/*
	 * Decrement the ref count of the var's value.
	 */

	Tcl_Obj *objPtr = dummyVar.value.objPtr;

	TclDecrRefCount(objPtr);
    } else if (TclIsVarArray(&dummyVar)) {
	/*
	 * If the variable is an array, delete all of its elements. This must
	 * be done after calling and deleting the traces on the array, above
	 * (that's the way traces are defined). If the array name is not
	 * present and is required for a trace on some element, it will be
	 * computed at DeleteArray.
	 */

	DeleteArray(iPtr, part1Ptr, (Var *) &dummyVar, (flags
		& (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY)) | TCL_TRACE_UNSETS,
		index);
    } else if (TclIsVarLink(&dummyVar)) {
	/*
	 * For global/upvar variables referenced in procedures, decrement the
	 * reference count on the variable referred to, and free the
	 * referenced variable if it's no longer needed.
	 */

	Var *linkPtr = dummyVar.value.linkPtr;

	if (TclIsVarInHash(linkPtr)) {
	    VarHashRefCount(linkPtr)--;
	    CleanupVar(linkPtr, NULL);
	}
    }

    /*
     * If the variable was a namespace variable, decrement its reference
     * count.
     */

    TclClearVarNamespaceVar(varPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnsetObjCmd --
 *
 *	This object-based function is invoked to process the "unset" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_UnsetObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register int i, flags = TCL_LEAVE_ERR_MSG;
    register const char *name;

    if (objc == 1) {
	/*
	 * Do nothing if no arguments supplied, so as to match command
	 * documentation.
	 */

	return TCL_OK;
    }

    /*
     * Simple, restrictive argument parsing. The only options are -- and
     * -nocomplain (which must come first and be given exactly to be an
     * option).
     */

    i = 1;
    name = TclGetString(objv[i]);
    if (name[0] == '-') {
	if (strcmp("-nocomplain", name) == 0) {
	    i++;
	    if (i == objc) {
		return TCL_OK;
	    }
	    flags = 0;
	    name = TclGetString(objv[i]);
	}
	if (strcmp("--", name) == 0) {
	    i++;
	}
    }

    for (; i < objc; i++) {
	if ((TclObjUnsetVar2(interp, objv[i], NULL, flags) != TCL_OK)
		&& (flags == TCL_LEAVE_ERR_MSG)) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendObjCmd --
 *
 *	This object-based function is invoked to process the "append" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	A variable's value may be changed.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_AppendObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Var *varPtr, *arrayPtr;
    register Tcl_Obj *varValuePtr = NULL;
				/* Initialized to avoid compiler warning. */
    int i;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "varName ?value ...?");
	return TCL_ERROR;
    }

    if (objc == 2) {
	varValuePtr = Tcl_ObjGetVar2(interp, objv[1], NULL,TCL_LEAVE_ERR_MSG);
	if (varValuePtr == NULL) {
	    return TCL_ERROR;
	}
    } else {
	varPtr = TclObjLookupVarEx(interp, objv[1], NULL, TCL_LEAVE_ERR_MSG,
		"set", /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
	if (varPtr == NULL) {
	    return TCL_ERROR;
	}
	for (i=2 ; i<objc ; i++) {
	    /*
	     * Note that we do not need to increase the refCount of the Var
	     * pointers: should a trace delete the variable, the return value
	     * of TclPtrSetVarIdx will be NULL or emptyObjPtr, and we will not
	     * access the variable again.
	     */

	    varValuePtr = TclPtrSetVarIdx(interp, varPtr, arrayPtr, objv[1],
		    NULL, objv[i], TCL_APPEND_VALUE|TCL_LEAVE_ERR_MSG, -1);
	    if ((varValuePtr == NULL) ||
		    (varValuePtr == ((Interp *) interp)->emptyObjPtr)) {
		return TCL_ERROR;
	    }
	}
    }
    Tcl_SetObjResult(interp, varValuePtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LappendObjCmd --
 *
 *	This object-based function is invoked to process the "lappend" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	A variable's value may be changed.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LappendObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *varValuePtr, *newValuePtr;
    int numElems, createdNewObj;
    Var *varPtr, *arrayPtr;
    int result;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "varName ?value ...?");
	return TCL_ERROR;
    }
    if (objc == 2) {
	newValuePtr = Tcl_ObjGetVar2(interp, objv[1], NULL, 0);
	if (newValuePtr == NULL) {
	    /*
	     * The variable doesn't exist yet. Just create it with an empty
	     * initial value.
	     */

	    TclNewObj(varValuePtr);
	    newValuePtr = Tcl_ObjSetVar2(interp, objv[1], NULL, varValuePtr,
		    TCL_LEAVE_ERR_MSG);
	    if (newValuePtr == NULL) {
		return TCL_ERROR;
	    }
	} else {
	    result = TclListObjLength(interp, newValuePtr, &numElems);
	    if (result != TCL_OK) {
		return result;
	    }
	}
    } else {
	/*
	 * We have arguments to append. We used to call Tcl_SetVar2 to append
	 * each argument one at a time to ensure that traces were run for each
	 * append step. We now append the arguments all at once because it's
	 * faster. Note that a read trace and a write trace for the variable
	 * will now each only be called once. Also, if the variable's old
	 * value is unshared we modify it directly, otherwise we create a new
	 * copy to modify: this is "copy on write".
	 */

	createdNewObj = 0;

	/*
	 * Protect the variable pointers around the TclPtrGetVarIdx call
	 * to insure that they remain valid even if the variable was undefined
	 * and unused.
	 */

	varPtr = TclObjLookupVarEx(interp, objv[1], NULL, TCL_LEAVE_ERR_MSG,
		"set", /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
	if (varPtr == NULL) {
	    return TCL_ERROR;
	}
	if (TclIsVarInHash(varPtr)) {
	    VarHashRefCount(varPtr)++;
	}
	if (arrayPtr && TclIsVarInHash(arrayPtr)) {
	    VarHashRefCount(arrayPtr)++;
	}
	varValuePtr = TclPtrGetVarIdx(interp, varPtr, arrayPtr, objv[1], NULL,
		TCL_LEAVE_ERR_MSG, -1);
	if (TclIsVarInHash(varPtr)) {
	    VarHashRefCount(varPtr)--;
	}
	if (arrayPtr && TclIsVarInHash(arrayPtr)) {
	    VarHashRefCount(arrayPtr)--;
	}

	if (varValuePtr == NULL) {
	    /*
	     * We couldn't read the old value: either the var doesn't yet
	     * exist or it's an array element. If it's new, we will try to
	     * create it with Tcl_ObjSetVar2 below.
	     */

	    TclNewObj(varValuePtr);
	    createdNewObj = 1;
	} else if (Tcl_IsShared(varValuePtr)) {
	    varValuePtr = Tcl_DuplicateObj(varValuePtr);
	    createdNewObj = 1;
	}

	result = TclListObjLength(interp, varValuePtr, &numElems);
	if (result == TCL_OK) {
	    result = Tcl_ListObjReplace(interp, varValuePtr, numElems, 0,
		    (objc-2), (objv+2));
	}
	if (result != TCL_OK) {
	    if (createdNewObj) {
		TclDecrRefCount(varValuePtr); /* Free unneeded obj. */
	    }
	    return result;
	}

	/*
	 * Now store the list object back into the variable. If there is an
	 * error setting the new value, decrement its ref count if it was new
	 * and we didn't create the variable.
	 */

	newValuePtr = TclPtrSetVarIdx(interp, varPtr, arrayPtr, objv[1], NULL,
		varValuePtr, TCL_LEAVE_ERR_MSG, -1);
	if (newValuePtr == NULL) {
	    return TCL_ERROR;
	}
    }

    /*
     * Set the interpreter's object result to refer to the variable's value
     * object.
     */

    Tcl_SetObjResult(interp, newValuePtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayStartSearchCmd --
 *
 *	This object-based function is invoked to process the "array
 *	startsearch" Tcl command. See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayStartSearchCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Interp *iPtr = (Interp *)interp;
    Var *varPtr;
    Tcl_HashEntry *hPtr;
    int isNew, isArray;
    ArraySearch *searchPtr;
    const char *varName;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName");
	return TCL_ERROR;
    }

    if (TCL_ERROR == LocateArray(interp, objv[1], &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    if (!isArray) {
	return NotArrayError(interp, objv[1]);
    }

    /*
     * Make a new array search with a free name.
     */

    varName = TclGetString(objv[1]);
    searchPtr = ckalloc(sizeof(ArraySearch));
    hPtr = Tcl_CreateHashEntry(&iPtr->varSearches, varPtr, &isNew);
    if (isNew) {
	searchPtr->id = 1;
	varPtr->flags |= VAR_SEARCH_ACTIVE;
	searchPtr->nextPtr = NULL;
    } else {
	searchPtr->id = ((ArraySearch *) Tcl_GetHashValue(hPtr))->id + 1;
	searchPtr->nextPtr = Tcl_GetHashValue(hPtr);
    }
    searchPtr->varPtr = varPtr;
    searchPtr->nextEntry = VarHashFirstEntry(varPtr->value.tablePtr,
	    &searchPtr->search);
    Tcl_SetHashValue(hPtr, searchPtr);
    Tcl_SetObjResult(interp,
	    Tcl_ObjPrintf("s-%d-%s", searchPtr->id, varName));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayAnyMoreCmd --
 *
 *	This object-based function is invoked to process the "array anymore"
 *	Tcl command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayAnyMoreCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Interp *iPtr = (Interp *)interp;
    Var *varPtr;
    Tcl_Obj *varNameObj, *searchObj;
    int gotValue, isArray;
    ArraySearch *searchPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName searchId");
	return TCL_ERROR;
    }
    varNameObj = objv[1];
    searchObj = objv[2];

    if (TCL_ERROR == LocateArray(interp, varNameObj, &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    if (!isArray) {
	return NotArrayError(interp, varNameObj);
    }

    /*
     * Get the search.
     */

    searchPtr = ParseSearchId(interp, varPtr, varNameObj, searchObj);
    if (searchPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Scan forward to find if there are any further elements in the array
     * that are defined.
     */

    while (1) {
	if (searchPtr->nextEntry != NULL) {
	    varPtr = VarHashGetValue(searchPtr->nextEntry);
	    if (!TclIsVarUndefined(varPtr)) {
		gotValue = 1;
		break;
	    }
	}
	searchPtr->nextEntry = Tcl_NextHashEntry(&searchPtr->search);
	if (searchPtr->nextEntry == NULL) {
	    gotValue = 0;
	    break;
	}
    }
    Tcl_SetObjResult(interp, iPtr->execEnvPtr->constants[gotValue]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayNextElementCmd --
 *
 *	This object-based function is invoked to process the "array
 *	nextelement" Tcl command. See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayNextElementCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Var *varPtr;
    Tcl_Obj *varNameObj, *searchObj;
    ArraySearch *searchPtr;
    int isArray;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName searchId");
	return TCL_ERROR;
    }
    varNameObj = objv[1];
    searchObj = objv[2];

    if (TCL_ERROR == LocateArray(interp, varNameObj, &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    if (!isArray) {
	return NotArrayError(interp, varNameObj);
    }

    /*
     * Get the search.
     */

    searchPtr = ParseSearchId(interp, varPtr, varNameObj, searchObj);
    if (searchPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Get the next element from the search, or the empty string on
     * exhaustion. Note that the [array anymore] command may well have already
     * pulled a value from the hash enumeration, so we have to check the cache
     * there first.
     */

    while (1) {
	Tcl_HashEntry *hPtr = searchPtr->nextEntry;

	if (hPtr == NULL) {
	    hPtr = Tcl_NextHashEntry(&searchPtr->search);
	    if (hPtr == NULL) {
		return TCL_OK;
	    }
	} else {
	    searchPtr->nextEntry = NULL;
	}
	varPtr = VarHashGetValue(hPtr);
	if (!TclIsVarUndefined(varPtr)) {
	    Tcl_SetObjResult(interp, VarHashGetKey(varPtr));
	    return TCL_OK;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayDoneSearchCmd --
 *
 *	This object-based function is invoked to process the "array
 *	donesearch" Tcl command. See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayDoneSearchCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Interp *iPtr = (Interp *)interp;
    Var *varPtr;
    Tcl_HashEntry *hPtr;
    Tcl_Obj *varNameObj, *searchObj;
    ArraySearch *searchPtr, *prevPtr;
    int isArray;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName searchId");
	return TCL_ERROR;
    }
    varNameObj = objv[1];
    searchObj = objv[2];

    if (TCL_ERROR == LocateArray(interp, varNameObj, &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    if (!isArray) {
	return NotArrayError(interp, varNameObj);
    }

    /*
     * Get the search.
     */

    searchPtr = ParseSearchId(interp, varPtr, varNameObj, searchObj);
    if (searchPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Unhook the search from the list of searches associated with the
     * variable.
     */

    hPtr = Tcl_FindHashEntry(&iPtr->varSearches, varPtr);
    if (searchPtr == Tcl_GetHashValue(hPtr)) {
	if (searchPtr->nextPtr) {
	    Tcl_SetHashValue(hPtr, searchPtr->nextPtr);
	} else {
	    varPtr->flags &= ~VAR_SEARCH_ACTIVE;
	    Tcl_DeleteHashEntry(hPtr);
	}
    } else {
	for (prevPtr=Tcl_GetHashValue(hPtr) ;; prevPtr=prevPtr->nextPtr) {
	    if (prevPtr->nextPtr == searchPtr) {
		prevPtr->nextPtr = searchPtr->nextPtr;
		break;
	    }
	}
    }
    ckfree(searchPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayExistsCmd --
 *
 *	This object-based function is invoked to process the "array exists"
 *	Tcl command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayExistsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Interp *iPtr = (Interp *)interp;
    int isArray;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName");
	return TCL_ERROR;
    }

    if (TCL_ERROR == LocateArray(interp, objv[1], NULL, &isArray)) {
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, iPtr->execEnvPtr->constants[isArray]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayGetCmd --
 *
 *	This object-based function is invoked to process the "array get" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayGetCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Var *varPtr, *varPtr2;
    Tcl_Obj *varNameObj, *nameObj, *valueObj, *nameLstObj, *tmpResObj;
    Tcl_Obj **nameObjPtr, *patternObj;
    Tcl_HashSearch search;
    const char *pattern;
    int i, count, result, isArray;

    switch (objc) {
    case 2:
	varNameObj = objv[1];
	patternObj = NULL;
	break;
    case 3:
	varNameObj = objv[1];
	patternObj = objv[2];
	break;
    default:
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName ?pattern?");
	return TCL_ERROR;
    }

    if (TCL_ERROR == LocateArray(interp, varNameObj, &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    /* If not an array, it's an empty result. */
    if (!isArray) {
	return TCL_OK;
    }

    pattern = (patternObj ? TclGetString(patternObj) : NULL);

    /*
     * Store the array names in a new object.
     */

    TclNewObj(nameLstObj);
    Tcl_IncrRefCount(nameLstObj);
    if ((patternObj != NULL) && TclMatchIsTrivial(pattern)) {
	varPtr2 = VarHashFindVar(varPtr->value.tablePtr, patternObj);
	if (varPtr2 == NULL) {
	    goto searchDone;
	}
	if (TclIsVarUndefined(varPtr2)) {
	    goto searchDone;
	}
	result = Tcl_ListObjAppendElement(interp, nameLstObj,
		VarHashGetKey(varPtr2));
	if (result != TCL_OK) {
	    TclDecrRefCount(nameLstObj);
	    return result;
	}
	goto searchDone;
    }

    for (varPtr2 = VarHashFirstVar(varPtr->value.tablePtr, &search);
	    varPtr2; varPtr2 = VarHashNextVar(&search)) {
	if (TclIsVarUndefined(varPtr2)) {
	    continue;
	}
	nameObj = VarHashGetKey(varPtr2);
	if (patternObj && !Tcl_StringMatch(TclGetString(nameObj), pattern)) {
	    continue;		/* Element name doesn't match pattern. */
	}

	result = Tcl_ListObjAppendElement(interp, nameLstObj, nameObj);
	if (result != TCL_OK) {
	    TclDecrRefCount(nameLstObj);
	    return result;
	}
    }

    /*
     * Make sure the Var structure of the array is not removed by a trace
     * while we're working.
     */

  searchDone:
    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)++;
    }

    /*
     * Get the array values corresponding to each element name.
     */

    TclNewObj(tmpResObj);
    result = Tcl_ListObjGetElements(interp, nameLstObj, &count, &nameObjPtr);
    if (result != TCL_OK) {
	goto errorInArrayGet;
    }

    for (i=0 ; i<count ; i++) {
	nameObj = *nameObjPtr++;
	valueObj = Tcl_ObjGetVar2(interp, varNameObj, nameObj,
		TCL_LEAVE_ERR_MSG);
	if (valueObj == NULL) {
	    /*
	     * Some trace played a trick on us; we need to diagnose to adapt
	     * our behaviour: was the array element unset, or did the
	     * modification modify the complete array?
	     */

	    if (TclIsVarArray(varPtr)) {
		/*
		 * The array itself looks OK, the variable was undefined:
		 * forget it.
		 */

		continue;
	    }
	    result = TCL_ERROR;
	    goto errorInArrayGet;
	}
	result = Tcl_DictObjPut(interp, tmpResObj, nameObj, valueObj);
	if (result != TCL_OK) {
	    goto errorInArrayGet;
	}
    }
    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)--;
    }
    Tcl_SetObjResult(interp, tmpResObj);
    TclDecrRefCount(nameLstObj);
    return TCL_OK;

  errorInArrayGet:
    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)--;
    }
    TclDecrRefCount(nameLstObj);
    TclDecrRefCount(tmpResObj);	/* Free unneeded temp result. */
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayNamesCmd --
 *
 *	This object-based function is invoked to process the "array names" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayNamesCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    static const char *const options[] = {
	"-exact", "-glob", "-regexp", NULL
    };
    enum options { OPT_EXACT, OPT_GLOB, OPT_REGEXP };
    Var *varPtr, *varPtr2;
    Tcl_Obj *nameObj, *resultObj, *patternObj;
    Tcl_HashSearch search;
    const char *pattern = NULL;
    int isArray, mode = OPT_GLOB;

    if ((objc < 2) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName ?mode? ?pattern?");
	return TCL_ERROR;
    }
    patternObj = (objc > 2 ? objv[objc-1] : NULL);

    if (TCL_ERROR == LocateArray(interp, objv[1], &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    /*
     * Finish parsing the arguments.
     */

    if ((objc == 4) && Tcl_GetIndexFromObj(interp, objv[2], options, "option",
	    0, &mode) != TCL_OK) {
	return TCL_ERROR;
    }

    /* If not an array, the result is empty. */

    if (!isArray) {
	return TCL_OK;
    }

    /*
     * Check for the trivial cases where we can use a direct lookup.
     */

    TclNewObj(resultObj);
    if (patternObj) {
	pattern = TclGetString(patternObj);
    }
    if ((mode==OPT_GLOB && patternObj && TclMatchIsTrivial(pattern))
	    || (mode==OPT_EXACT)) {
	varPtr2 = VarHashFindVar(varPtr->value.tablePtr, patternObj);
	if ((varPtr2 != NULL) && !TclIsVarUndefined(varPtr2)) {
	    /*
	     * This can't fail; lappending to an empty object always works.
	     */

	    Tcl_ListObjAppendElement(NULL, resultObj, VarHashGetKey(varPtr2));
	}
	Tcl_SetObjResult(interp, resultObj);
	return TCL_OK;
    }

    /*
     * Must scan the array to select the elements.
     */

    for (varPtr2=VarHashFirstVar(varPtr->value.tablePtr, &search);
	    varPtr2!=NULL ; varPtr2=VarHashNextVar(&search)) {
	if (TclIsVarUndefined(varPtr2)) {
	    continue;
	}
	nameObj = VarHashGetKey(varPtr2);
	if (patternObj) {
	    const char *name = TclGetString(nameObj);
	    int matched = 0;

	    switch ((enum options) mode) {
	    case OPT_EXACT:
		Tcl_Panic("exact matching shouldn't get here");
	    case OPT_GLOB:
		matched = Tcl_StringMatch(name, pattern);
		break;
	    case OPT_REGEXP:
		matched = Tcl_RegExpMatchObj(interp, nameObj, patternObj);
		if (matched < 0) {
		    TclDecrRefCount(resultObj);
		    return TCL_ERROR;
		}
		break;
	    }
	    if (matched == 0) {
		continue;
	    }
	}

	Tcl_ListObjAppendElement(NULL, resultObj, nameObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFindArrayPtrElements --
 *
 *	Fill out a hash table (which *must* use Tcl_Obj* keys) with an entry
 *	for each existing element of the given array. The provided hash table
 *	is assumed to be initially empty.
 *
 * Result:
 *	none
 *
 * Side effects:
 *	The keys of the array gain an extra reference. The supplied hash table
 *	has elements added to it.
 *
 *----------------------------------------------------------------------
 */

void
TclFindArrayPtrElements(
    Var *arrayPtr,
    Tcl_HashTable *tablePtr)
{
    Var *varPtr;
    Tcl_HashSearch search;

    if ((arrayPtr == NULL) || !TclIsVarArray(arrayPtr)
	    || TclIsVarUndefined(arrayPtr)) {
	return;
    }

    for (varPtr=VarHashFirstVar(arrayPtr->value.tablePtr, &search);
	    varPtr!=NULL ; varPtr=VarHashNextVar(&search)) {
	Tcl_HashEntry *hPtr;
	Tcl_Obj *nameObj;
	int dummy;

	if (TclIsVarUndefined(varPtr)) {
	    continue;
	}
	nameObj = VarHashGetKey(varPtr);
	hPtr = Tcl_CreateHashEntry(tablePtr, (char *) nameObj, &dummy);
	Tcl_SetHashValue(hPtr, nameObj);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ArraySetCmd --
 *
 *	This object-based function is invoked to process the "array set" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArraySetCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_Obj *arrayNameObj;
    Tcl_Obj *arrayElemObj;
    Var *varPtr, *arrayPtr;
    int result, i;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName list");
	return TCL_ERROR;
    }

    if (TCL_ERROR == LocateArray(interp, objv[1], NULL, NULL)) {
	return TCL_ERROR;
    }

    arrayNameObj = objv[1];
    varPtr = TclObjLookupVarEx(interp, arrayNameObj, NULL,
	    /*flags*/ TCL_LEAVE_ERR_MSG, /*msg*/ "set", /*createPart1*/ 1,
	    /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return TCL_ERROR;
    }
    if (arrayPtr) {
	CleanupVar(varPtr, arrayPtr);
	TclObjVarErrMsg(interp, arrayNameObj, NULL, "set", needArray, -1);
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARNAME",
		TclGetString(arrayNameObj), NULL);
	return TCL_ERROR;
    }

    /*
     * Install the contents of the dictionary or list into the array.
     */

    arrayElemObj = objv[2];
    if (arrayElemObj->typePtr == &tclDictType && arrayElemObj->bytes == NULL) {
	Tcl_Obj *keyPtr, *valuePtr;
	Tcl_DictSearch search;
	int done;

	if (Tcl_DictObjSize(interp, arrayElemObj, &done) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (done == 0) {
	    /*
	     * Empty, so we'll just force the array to be properly existing
	     * instead.
	     */

	    goto ensureArray;
	}

	/*
	 * Don't need to look at result of Tcl_DictObjFirst as we've just
	 * successfully used a dictionary operation on the same object.
	 */

	for (Tcl_DictObjFirst(interp, arrayElemObj, &search,
		&keyPtr, &valuePtr, &done) ; !done ;
		Tcl_DictObjNext(&search, &keyPtr, &valuePtr, &done)) {
	    /*
	     * At this point, it would be nice if the key was directly usable
	     * by the array. This isn't the case though.
	     */

	    Var *elemVarPtr = TclLookupArrayElement(interp, arrayNameObj,
		    keyPtr, TCL_LEAVE_ERR_MSG, "set", 1, 1, varPtr, -1);

	    if ((elemVarPtr == NULL) ||
		    (TclPtrSetVarIdx(interp, elemVarPtr, varPtr, arrayNameObj,
		    keyPtr, valuePtr, TCL_LEAVE_ERR_MSG, -1) == NULL)) {
		Tcl_DictObjDone(&search);
		return TCL_ERROR;
	    }
	}
	return TCL_OK;
    } else {
	/*
	 * Not a dictionary, so assume (and convert to, for backward-
	 * -compatibility reasons) a list.
	 */

	int elemLen;
	Tcl_Obj **elemPtrs, *copyListObj;

	result = TclListObjGetElements(interp, arrayElemObj,
		&elemLen, &elemPtrs);
	if (result != TCL_OK) {
	    return result;
	}
	if (elemLen & 1) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "list must have an even number of elements", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "FORMAT", NULL);
	    return TCL_ERROR;
	}
	if (elemLen == 0) {
	    goto ensureArray;
	}

	/*
	 * We needn't worry about traces invalidating arrayPtr: should that be
	 * the case, TclPtrSetVarIdx will return NULL so that we break out of
	 * the loop and return an error.
	 */

	copyListObj = TclListObjCopy(NULL, arrayElemObj);
	for (i=0 ; i<elemLen ; i+=2) {
	    Var *elemVarPtr = TclLookupArrayElement(interp, arrayNameObj,
		    elemPtrs[i], TCL_LEAVE_ERR_MSG, "set", 1, 1, varPtr, -1);

	    if ((elemVarPtr == NULL) ||
		    (TclPtrSetVarIdx(interp, elemVarPtr, varPtr, arrayNameObj,
		    elemPtrs[i],elemPtrs[i+1],TCL_LEAVE_ERR_MSG,-1) == NULL)){
		result = TCL_ERROR;
		break;
	    }
	}
	Tcl_DecrRefCount(copyListObj);
	return result;
    }

    /*
     * The list is empty make sure we have an array, or create one if
     * necessary.
     */

  ensureArray:
    if (varPtr != NULL) {
	if (TclIsVarArray(varPtr)) {
	    /*
	     * Already an array, done.
	     */

	    return TCL_OK;
	}
	if (TclIsVarArrayElement(varPtr) || !TclIsVarUndefined(varPtr)) {
	    /*
	     * Either an array element, or a scalar: lose!
	     */

	    TclObjVarErrMsg(interp, arrayNameObj, NULL, "array set",
		    needArray, -1);
	    Tcl_SetErrorCode(interp, "TCL", "WRITE", "ARRAY", NULL);
	    return TCL_ERROR;
	}
    }
    TclSetVarArray(varPtr);
    varPtr->value.tablePtr = ckalloc(sizeof(TclVarHashTable));
    TclInitVarHashTable(varPtr->value.tablePtr, TclGetVarNsPtr(varPtr));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArraySizeCmd --
 *
 *	This object-based function is invoked to process the "array size" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArraySizeCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Var *varPtr;
    Tcl_HashSearch search;
    Var *varPtr2;
    int isArray, size = 0;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName");
	return TCL_ERROR;
    }

    if (TCL_ERROR == LocateArray(interp, objv[1], &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    /* We can only iterate over the array if it exists... */

    if (isArray) {
	/*
	 * Must iterate in order to get chance to check for present but
	 * "undefined" entries.
	 */

	for (varPtr2=VarHashFirstVar(varPtr->value.tablePtr, &search);
		varPtr2!=NULL ; varPtr2=VarHashNextVar(&search)) {
	    if (!TclIsVarUndefined(varPtr2)) {
		size++;
	    }
	}
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(size));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayStatsCmd --
 *
 *	This object-based function is invoked to process the "array
 *	statistics" Tcl command. See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayStatsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Var *varPtr;
    Tcl_Obj *varNameObj;
    char *stats;
    int isArray;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName");
	return TCL_ERROR;
    }
    varNameObj = objv[1];

    if (TCL_ERROR == LocateArray(interp, varNameObj, &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    if (!isArray) {
	return NotArrayError(interp, varNameObj);
    }

    stats = Tcl_HashStats((Tcl_HashTable *) varPtr->value.tablePtr);
    if (stats == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"error reading array statistics", -1));
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(stats, -1));
    ckfree(stats);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ArrayUnsetCmd --
 *
 *	This object-based function is invoked to process the "array unset" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result object.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ArrayUnsetCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Var *varPtr, *varPtr2, *protectedVarPtr;
    Tcl_Obj *varNameObj, *patternObj, *nameObj;
    Tcl_HashSearch search;
    const char *pattern;
    const int unsetFlags = 0;	/* Should this be TCL_LEAVE_ERR_MSG? */
    int isArray;

    switch (objc) {
    case 2:
	varNameObj = objv[1];
	patternObj = NULL;
	break;
    case 3:
	varNameObj = objv[1];
	patternObj = objv[2];
	break;
    default:
	Tcl_WrongNumArgs(interp, 1, objv, "arrayName ?pattern?");
	return TCL_ERROR;
    }

    if (TCL_ERROR == LocateArray(interp, varNameObj, &varPtr, &isArray)) {
	return TCL_ERROR;
    }

    if (!isArray) {
	return TCL_OK;
    }

    if (!patternObj) {
	/*
	 * When no pattern is given, just unset the whole array.
	 */

	return TclObjUnsetVar2(interp, varNameObj, NULL, 0);
    }

    /*
     * With a trivial pattern, we can just unset.
     */

    pattern = TclGetString(patternObj);
    if (TclMatchIsTrivial(pattern)) {
	varPtr2 = VarHashFindVar(varPtr->value.tablePtr, patternObj);
	if (!varPtr2 || TclIsVarUndefined(varPtr2)) {
	    return TCL_OK;
	}
	return TclPtrUnsetVarIdx(interp, varPtr2, varPtr, varNameObj,
		patternObj, unsetFlags, -1);
    }

    /*
     * Non-trivial case (well, deeply tricky really). We peek inside the hash
     * iterator in order to allow us to guarantee that the following element
     * in the array will not be scrubbed until we have dealt with it. This
     * stops the overall iterator from ending up pointing into deallocated
     * memory. [Bug 2939073]
     */

    protectedVarPtr = NULL;
    for (varPtr2=VarHashFirstVar(varPtr->value.tablePtr, &search);
	    varPtr2!=NULL ; varPtr2=VarHashNextVar(&search)) {
	/*
	 * Drop the extra ref immediately. We don't need to free it at this
	 * point though; we'll be unsetting it if necessary soon.
	 */

	if (varPtr2 == protectedVarPtr) {
	    VarHashRefCount(varPtr2)--;
	}

	/*
	 * Guard the next (peeked) item in the search chain by incrementing
	 * its refcount. This guarantees that the hash table iterator won't be
	 * dangling on the next time through the loop.
	 */

	if (search.nextEntryPtr != NULL) {
	    protectedVarPtr = VarHashGetValue(search.nextEntryPtr);
	    VarHashRefCount(protectedVarPtr)++;
	} else {
	    protectedVarPtr = NULL;
	}

	/*
	 * If the variable is undefined, clean it out as it has been hit by
	 * something else (i.e., an unset trace).
	 */

	if (TclIsVarUndefined(varPtr2)) {
	    CleanupVar(varPtr2, varPtr);
	    continue;
	}

	nameObj = VarHashGetKey(varPtr2);
	if (Tcl_StringMatch(TclGetString(nameObj), pattern)
		&& TclPtrUnsetVarIdx(interp, varPtr2, varPtr, varNameObj,
			nameObj, unsetFlags, -1) != TCL_OK) {
	    /*
	     * If we incremented a refcount, we must decrement it here as we
	     * will not be coming back properly due to the error.
	     */

	    if (protectedVarPtr) {
		VarHashRefCount(protectedVarPtr)--;
		CleanupVar(protectedVarPtr, varPtr);
	    }
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitArrayCmd --
 *
 *	This creates the ensemble for the "array" command.
 *
 * Results:
 *	The handle for the created ensemble.
 *
 * Side effects:
 *	Creates a command in the global namespace.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
Tcl_Command
TclInitArrayCmd(
    Tcl_Interp *interp)		/* Current interpreter. */
{
    static const EnsembleImplMap arrayImplMap[] = {
	{"anymore",	ArrayAnyMoreCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{"donesearch",	ArrayDoneSearchCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{"exists",	ArrayExistsCmd,		TclCompileArrayExistsCmd, NULL, NULL, 0},
	{"get",		ArrayGetCmd,		TclCompileBasic1Or2ArgCmd, NULL, NULL, 0},
	{"names",	ArrayNamesCmd,		TclCompileBasic1To3ArgCmd, NULL, NULL, 0},
	{"nextelement",	ArrayNextElementCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{"set",		ArraySetCmd,		TclCompileArraySetCmd, NULL, NULL, 0},
	{"size",	ArraySizeCmd,		TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"startsearch",	ArrayStartSearchCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"statistics",	ArrayStatsCmd,		TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"unset",	ArrayUnsetCmd,		TclCompileArrayUnsetCmd, NULL, NULL, 0},
	{NULL, NULL, NULL, NULL, NULL, 0}
    };

    return TclMakeEnsemble(interp, "array", arrayImplMap);
}

/*
 *----------------------------------------------------------------------
 *
 * ObjMakeUpvar --
 *
 *	This function does all of the work of the "global" and "upvar"
 *	commands.
 *
 * Results:
 *	A standard Tcl completion code. If an error occurs then an error
 *	message is left in iPtr->result.
 *
 * Side effects:
 *	The variable given by myName is linked to the variable in framePtr
 *	given by otherP1 and otherP2, so that references to myName are
 *	redirected to the other variable like a symbolic link.
 *	Callers must Incr myNamePtr if they plan to Decr it.
 *	Callers must Incr otherP1Ptr if they plan to Decr it.
 *
 *----------------------------------------------------------------------
 */

static int
ObjMakeUpvar(
    Tcl_Interp *interp,		/* Interpreter containing variables. Used for
				 * error messages, too. */
    CallFrame *framePtr,	/* Call frame containing "other" variable.
				 * NULL means use global :: context. */
    Tcl_Obj *otherP1Ptr,
    const char *otherP2,	/* Two-part name of variable in framePtr. */
    const int otherFlags,	/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of "other" variable. */
    Tcl_Obj *myNamePtr,		/* Name of variable which will refer to
				 * otherP1/otherP2. Must be a scalar. */
    int myFlags,		/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of myName. */
    int index)			/* If the variable to be linked is an indexed
				 * scalar, this is its index. Otherwise, -1 */
{
    Interp *iPtr = (Interp *) interp;
    Var *otherPtr, *arrayPtr;
    CallFrame *varFramePtr;

    /*
     * Find "other" in "framePtr". If not looking up other in just the current
     * namespace, temporarily replace the current var frame pointer in the
     * interpreter in order to use TclObjLookupVar.
     */

    if (framePtr == NULL) {
	framePtr = iPtr->rootFramePtr;
    }

    varFramePtr = iPtr->varFramePtr;
    if (!(otherFlags & TCL_NAMESPACE_ONLY)) {
	iPtr->varFramePtr = framePtr;
    }
    otherPtr = TclObjLookupVar(interp, otherP1Ptr, otherP2,
	    (otherFlags | TCL_LEAVE_ERR_MSG), "access",
	    /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    if (!(otherFlags & TCL_NAMESPACE_ONLY)) {
	iPtr->varFramePtr = varFramePtr;
    }
    if (otherPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Check that we are not trying to create a namespace var linked to a
     * local variable in a procedure. If we allowed this, the local
     * variable in the shorter-lived procedure frame could go away leaving
     * the namespace var's reference invalid.
     */

    if (index < 0) {
	if (!(arrayPtr != NULL
		     ? (TclIsVarInHash(arrayPtr) && TclGetVarNsPtr(arrayPtr))
		     : (TclIsVarInHash(otherPtr) && TclGetVarNsPtr(otherPtr)))
		&& ((myFlags & (TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY))
			|| (varFramePtr == NULL)
			|| !HasLocalVars(varFramePtr)
			|| (strstr(TclGetString(myNamePtr), "::") != NULL))) {
	    Tcl_SetObjResult((Tcl_Interp *) iPtr, Tcl_ObjPrintf(
		    "bad variable name \"%s\": can't create namespace "
		    "variable that refers to procedure variable",
		    TclGetString(myNamePtr)));
	    Tcl_SetErrorCode(interp, "TCL", "UPVAR", "INVERTED", NULL);
	    return TCL_ERROR;
	}
    }

    return TclPtrObjMakeUpvarIdx(interp, otherPtr, myNamePtr, myFlags, index);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPtrMakeUpvar --
 *
 *	This procedure does all of the work of the "global" and "upvar"
 *	commands.
 *
 * Results:
 *	A standard Tcl completion code. If an error occurs then an error
 *	message is left in iPtr->result.
 *
 * Side effects:
 *	The variable given by myName is linked to the variable in framePtr
 *	given by otherP1 and otherP2, so that references to myName are
 *	redirected to the other variable like a symbolic link.
 *
 *----------------------------------------------------------------------
 */

int
TclPtrMakeUpvar(
    Tcl_Interp *interp,		/* Interpreter containing variables. Used for
				 * error messages, too. */
    Var *otherPtr,		/* Pointer to the variable being linked-to. */
    const char *myName,		/* Name of variable which will refer to
				 * otherP1/otherP2. Must be a scalar. */
    int myFlags,		/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of myName. */
    int index)			/* If the variable to be linked is an indexed
				 * scalar, this is its index. Otherwise, -1 */
{
    Tcl_Obj *myNamePtr = NULL;
    int result;

    if (myName) {
	myNamePtr = Tcl_NewStringObj(myName, -1);
	Tcl_IncrRefCount(myNamePtr);
    }
    result = TclPtrObjMakeUpvarIdx(interp, otherPtr, myNamePtr, myFlags,
	    index);
    if (myNamePtr) {
	Tcl_DecrRefCount(myNamePtr);
    }
    return result;
}

int
TclPtrObjMakeUpvar(
    Tcl_Interp *interp,		/* Interpreter containing variables. Used for
				 * error messages, too. */
    Tcl_Var otherPtr,		/* Pointer to the variable being linked-to. */
    Tcl_Obj *myNamePtr,		/* Name of variable which will refer to
				 * otherP1/otherP2. Must be a scalar. */
    int myFlags)		/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of myName. */
{
    return TclPtrObjMakeUpvarIdx(interp, (Var *) otherPtr, myNamePtr, myFlags,
	    -1);
}

/* Callers must Incr myNamePtr if they plan to Decr it. */

int
TclPtrObjMakeUpvarIdx(
    Tcl_Interp *interp,		/* Interpreter containing variables. Used for
				 * error messages, too. */
    Var *otherPtr,		/* Pointer to the variable being linked-to. */
    Tcl_Obj *myNamePtr,		/* Name of variable which will refer to
				 * otherP1/otherP2. Must be a scalar. */
    int myFlags,		/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of myName. */
    int index)			/* If the variable to be linked is an indexed
				 * scalar, this is its index. Otherwise, -1 */
{
    Interp *iPtr = (Interp *) interp;
    CallFrame *varFramePtr = iPtr->varFramePtr;
    const char *errMsg, *p, *myName;
    Var *varPtr;

    if (index >= 0) {
	if (!HasLocalVars(varFramePtr)) {
	    Tcl_Panic("ObjMakeUpvar called with an index outside from a proc");
	}
	varPtr = (Var *) &(varFramePtr->compiledLocals[index]);
	myNamePtr = localName(iPtr->varFramePtr, index);
	myName = myNamePtr? TclGetString(myNamePtr) : NULL;
    } else {
	/*
	 * Do not permit the new variable to look like an array reference, as
	 * it will not be reachable in that case [Bug 600812, TIP 184]. The
	 * "definition" of what "looks like an array reference" is consistent
	 * (and must remain consistent) with the code in TclObjLookupVar().
	 */

	myName = TclGetString(myNamePtr);
	p = strstr(myName, "(");
	if (p != NULL) {
	    p += strlen(p)-1;
	    if (*p == ')') {
		/*
		 * myName looks like an array reference.
		 */

		Tcl_SetObjResult((Tcl_Interp *) iPtr, Tcl_ObjPrintf(
			"bad variable name \"%s\": can't create a scalar "
			"variable that looks like an array element", myName));
		Tcl_SetErrorCode(interp, "TCL", "UPVAR", "LOCAL_ELEMENT",
			NULL);
		return TCL_ERROR;
	    }
	}

	/*
	 * Lookup and eventually create the new variable. Set the flag bit
	 * TCL_AVOID_RESOLVERS to indicate the special resolution rules for
	 * upvar purposes:
	 *   - Bug #696893 - variable is either proc-local or in the current
	 *     namespace; never follow the second (global) resolution path.
	 *   - Bug #631741 - do not use special namespace or interp resolvers.
	 */

	varPtr = TclLookupSimpleVar(interp, myNamePtr,
		myFlags|TCL_AVOID_RESOLVERS, /* create */ 1, &errMsg, &index);
	if (varPtr == NULL) {
	    TclObjVarErrMsg(interp, myNamePtr, NULL, "create", errMsg, -1);
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARNAME",
		    TclGetString(myNamePtr), NULL);
	    return TCL_ERROR;
	}
    }

    if (varPtr == otherPtr) {
	Tcl_SetObjResult((Tcl_Interp *) iPtr, Tcl_NewStringObj(
		"can't upvar from variable to itself", -1));
	Tcl_SetErrorCode(interp, "TCL", "UPVAR", "SELF", NULL);
	return TCL_ERROR;
    }

    if (TclIsVarTraced(varPtr)) {
	Tcl_SetObjResult((Tcl_Interp *) iPtr, Tcl_ObjPrintf(
		"variable \"%s\" has traces: can't use for upvar", myName));
	Tcl_SetErrorCode(interp, "TCL", "UPVAR", "TRACED", NULL);
	return TCL_ERROR;
    } else if (!TclIsVarUndefined(varPtr)) {
	Var *linkPtr;

	/*
	 * The variable already existed. Make sure this variable "varPtr"
	 * isn't the same as "otherPtr" (avoid circular links). Also, if it's
	 * not an upvar then it's an error. If it is an upvar, then just
	 * disconnect it from the thing it currently refers to.
	 */

	if (!TclIsVarLink(varPtr)) {
	    Tcl_SetObjResult((Tcl_Interp *) iPtr, Tcl_ObjPrintf(
		    "variable \"%s\" already exists", myName));
	    Tcl_SetErrorCode(interp, "TCL", "UPVAR", "EXISTS", NULL);
	    return TCL_ERROR;
	}

	linkPtr = varPtr->value.linkPtr;
	if (linkPtr == otherPtr) {
	    return TCL_OK;
	}
	if (TclIsVarInHash(linkPtr)) {
	    VarHashRefCount(linkPtr)--;
	    if (TclIsVarUndefined(linkPtr)) {
		CleanupVar(linkPtr, NULL);
	    }
	}
    }
    TclSetVarLink(varPtr);
    varPtr->value.linkPtr = otherPtr;
    if (TclIsVarInHash(otherPtr)) {
	VarHashRefCount(otherPtr)++;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpVar --
 *
 *	This function links one variable to another, just like the "upvar"
 *	command.
 *
 * Results:
 *	A standard Tcl completion code. If an error occurs then an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	The variable in frameName whose name is given by varName becomes
 *	accessible under the name localNameStr, so that references to
 *	localNameStr are redirected to the other variable like a symbolic
 *	link.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_UpVar
int
Tcl_UpVar(
    Tcl_Interp *interp,		/* Command interpreter in which varName is to
				 * be looked up. */
    const char *frameName,	/* Name of the frame containing the source
				 * variable, such as "1" or "#0". */
    const char *varName,	/* Name of a variable in interp to link to.
				 * May be either a scalar name or an element
				 * in an array. */
    const char *localNameStr,	/* Name of link variable. */
    int flags)			/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of localNameStr. */
{
    int result;
    CallFrame *framePtr;
    Tcl_Obj *varNamePtr, *localNamePtr;

    if (TclGetFrame(interp, frameName, &framePtr) == -1) {
	return TCL_ERROR;
    }

    varNamePtr = Tcl_NewStringObj(varName, -1);
    Tcl_IncrRefCount(varNamePtr);
    localNamePtr = Tcl_NewStringObj(localNameStr, -1);
    Tcl_IncrRefCount(localNamePtr);

    result = ObjMakeUpvar(interp, framePtr, varNamePtr, NULL, 0,
	    localNamePtr, flags, -1);
    Tcl_DecrRefCount(varNamePtr);
    Tcl_DecrRefCount(localNamePtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpVar2 --
 *
 *	This function links one variable to another, just like the "upvar"
 *	command.
 *
 * Results:
 *	A standard Tcl completion code. If an error occurs then an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	The variable in frameName whose name is given by part1 and part2
 *	becomes accessible under the name localNameStr, so that references to
 *	localNameStr are redirected to the other variable like a symbolic
 *	link.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UpVar2(
    Tcl_Interp *interp,		/* Interpreter containing variables. Used for
				 * error messages too. */
    const char *frameName,	/* Name of the frame containing the source
				 * variable, such as "1" or "#0". */
    const char *part1,
    const char *part2,		/* Two parts of source variable name to link
				 * to. */
    const char *localNameStr,	/* Name of link variable. */
    int flags)			/* 0, TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY:
				 * indicates scope of localNameStr. */
{
    int result;
    CallFrame *framePtr;
    Tcl_Obj *part1Ptr, *localNamePtr;

    if (TclGetFrame(interp, frameName, &framePtr) == -1) {
	return TCL_ERROR;
    }

    part1Ptr = Tcl_NewStringObj(part1, -1);
    Tcl_IncrRefCount(part1Ptr);
    localNamePtr = Tcl_NewStringObj(localNameStr, -1);
    Tcl_IncrRefCount(localNamePtr);

    result = ObjMakeUpvar(interp, framePtr, part1Ptr, part2, 0,
	    localNamePtr, flags, -1);
    Tcl_DecrRefCount(part1Ptr);
    Tcl_DecrRefCount(localNamePtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVariableFullName --
 *
 *	Given a Tcl_Var token returned by Tcl_FindNamespaceVar, this function
 *	appends to an object the namespace variable's full name, qualified by
 *	a sequence of parent namespace names.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The variable's fully-qualified name is appended to the string
 *	representation of objPtr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetVariableFullName(
    Tcl_Interp *interp,		/* Interpreter containing the variable. */
    Tcl_Var variable,		/* Token for the variable returned by a
				 * previous call to Tcl_FindNamespaceVar. */
    Tcl_Obj *objPtr)		/* Points to the object onto which the
				 * variable's full name is appended. */
{
    Interp *iPtr = (Interp *) interp;
    register Var *varPtr = (Var *) variable;
    Tcl_Obj *namePtr;
    Namespace *nsPtr;

    if (!varPtr || TclIsVarArrayElement(varPtr)) {
	return;
    }

    /*
     * Add the full name of the containing namespace (if any), followed by the
     * "::" separator, then the variable name.
     */

    nsPtr = TclGetVarNsPtr(varPtr);
    if (nsPtr) {
	Tcl_AppendToObj(objPtr, nsPtr->fullName, -1);
	if (nsPtr != iPtr->globalNsPtr) {
	    Tcl_AppendToObj(objPtr, "::", 2);
	}
    }
    if (TclIsVarInHash(varPtr)) {
	if (!TclIsVarDeadHash(varPtr)) {
	    namePtr = VarHashGetKey(varPtr);
	    Tcl_AppendObjToObj(objPtr, namePtr);
	}
    } else if (iPtr->varFramePtr->procPtr) {
	int index = varPtr - iPtr->varFramePtr->compiledLocals;

	if (index >= 0 && index < iPtr->varFramePtr->numCompiledLocals) {
	    namePtr = localName(iPtr->varFramePtr, index);
	    Tcl_AppendObjToObj(objPtr, namePtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GlobalObjCmd --
 *
 *	This object-based function is invoked to process the "global" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GlobalObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    register Tcl_Obj *objPtr, *tailPtr;
    const char *varName;
    register const char *tail;
    int result, i;

    /*
     * If we are not executing inside a Tcl procedure, just return.
     */

    if (!HasLocalVars(iPtr->varFramePtr)) {
	return TCL_OK;
    }

    for (i=1 ; i<objc ; i++) {
	/*
	 * Make a local variable linked to its counterpart in the global ::
	 * namespace.
	 */

	objPtr = objv[i];
	varName = TclGetString(objPtr);

	/*
	 * The variable name might have a scope qualifier, but the name for
	 * the local "link" variable must be the simple name at the tail.
	 */

	for (tail=varName ; *tail!='\0' ; tail++) {
	    /* empty body */
	}
	while ((tail > varName) && ((*tail != ':') || (*(tail-1) != ':'))) {
	    tail--;
	}
	if ((*tail == ':') && (tail > varName)) {
	    tail++;
	}

	if (tail == varName) {
	    tailPtr = objPtr;
	} else {
	    tailPtr = Tcl_NewStringObj(tail, -1);
	    Tcl_IncrRefCount(tailPtr);
	}

	/*
	 * Link to the variable "varName" in the global :: namespace.
	 */

	result = ObjMakeUpvar(interp, NULL, objPtr, NULL,
		TCL_GLOBAL_ONLY, /*myName*/ tailPtr, /*myFlags*/ 0, -1);

	if (tail != varName) {
	    Tcl_DecrRefCount(tailPtr);
	}

	if (result != TCL_OK) {
	    return result;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VariableObjCmd --
 *
 *	Invoked to implement the "variable" command that creates one or more
 *	global variables. Handles the following syntax:
 *
 *	    variable ?name value...? name ?value?
 *
 *	One or more variables can be created. The variables are initialized
 *	with the specified values. The value for the last variable is
 *	optional.
 *
 *	If the variable does not exist, it is created and given the optional
 *	value. If it already exists, it is simply set to the optional value.
 *	Normally, "name" is an unqualified name, so it is created in the
 *	current namespace. If it includes namespace qualifiers, it can be
 *	created in another namespace.
 *
 *	If the variable command is executed inside a Tcl procedure, it creates
 *	a local variable linked to the newly-created namespace variable.
 *
 * Results:
 *	Returns TCL_OK if the variable is found or created. Returns TCL_ERROR
 *	if anything goes wrong.
 *
 * Side effects:
 *	If anything goes wrong, this function returns an error message as the
 *	result in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_VariableObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    const char *varName, *tail, *cp;
    Var *varPtr, *arrayPtr;
    Tcl_Obj *varValuePtr;
    int i, result;
    Tcl_Obj *varNamePtr, *tailPtr;

    for (i=1 ; i<objc ; i+=2) {
	/*
	 * Look up each variable in the current namespace context, creating it
	 * if necessary.
	 */

	varNamePtr = objv[i];
	varName = TclGetString(varNamePtr);
	varPtr = TclObjLookupVarEx(interp, varNamePtr, NULL,
		(TCL_NAMESPACE_ONLY | TCL_LEAVE_ERR_MSG), "define",
		/*createPart1*/ 1, /*createPart2*/ 0, &arrayPtr);

	if (arrayPtr != NULL) {
	    /*
	     * Variable cannot be an element in an array. If arrayPtr is
	     * non-NULL, it is, so throw up an error and return.
	     */

	    TclObjVarErrMsg(interp, varNamePtr, NULL, "define",
		    isArrayElement, -1);
	    Tcl_SetErrorCode(interp, "TCL", "UPVAR", "LOCAL_ELEMENT", NULL);
	    return TCL_ERROR;
	}

	if (varPtr == NULL) {
	    return TCL_ERROR;
	}

	/*
	 * Mark the variable as a namespace variable and increment its
	 * reference count so that it will persist until its namespace is
	 * destroyed or until the variable is unset.
	 */

	TclSetVarNamespaceVar(varPtr);

	/*
	 * If a value was specified, set the variable to that value.
	 * Otherwise, if the variable is new, leave it undefined. (If the
	 * variable already exists and no value was specified, leave its value
	 * unchanged; just create the local link if we're in a Tcl procedure).
	 */

	if (i+1 < objc) {	/* A value was specified. */
	    varValuePtr = TclPtrSetVarIdx(interp, varPtr, arrayPtr,
		    varNamePtr, NULL, objv[i+1],
		    (TCL_NAMESPACE_ONLY | TCL_LEAVE_ERR_MSG), -1);
	    if (varValuePtr == NULL) {
		return TCL_ERROR;
	    }
	}

	/*
	 * If we are executing inside a Tcl procedure, create a local variable
	 * linked to the new namespace variable "varName".
	 */

	if (HasLocalVars(iPtr->varFramePtr)) {
	    /*
	     * varName might have a scope qualifier, but the name for the
	     * local "link" variable must be the simple name at the tail.
	     *
	     * Locate tail in one pass: drop any prefix after two *or more*
	     * consecutive ":" characters).
	     */

	    for (tail=cp=varName ; *cp!='\0' ;) {
		if (*cp++ == ':') {
		    while (*cp == ':') {
			tail = ++cp;
		    }
		}
	    }

	    /*
	     * Create a local link "tail" to the variable "varName" in the
	     * current namespace.
	     */

	    if (tail == varName) {
		tailPtr = varNamePtr;
	    } else {
		tailPtr = Tcl_NewStringObj(tail, -1);
		Tcl_IncrRefCount(tailPtr);
	    }

	    result = ObjMakeUpvar(interp, NULL, varNamePtr, /*otherP2*/ NULL,
		    /*otherFlags*/ TCL_NAMESPACE_ONLY,
		    /*myName*/ tailPtr, /*myFlags*/ 0, -1);

	    if (tail != varName) {
		Tcl_DecrRefCount(tailPtr);
	    }

	    if (result != TCL_OK) {
		return result;
	    }
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpvarObjCmd --
 *
 *	This object-based function is invoked to process the "upvar" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_UpvarObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    CallFrame *framePtr;
    int result, hasLevel;
    Tcl_Obj *levelObj;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?level? otherVar localVar ?otherVar localVar ...?");
	return TCL_ERROR;
    }

    if (objc & 1) {
	/*
	 * Even number of arguments, so use the default level of "1" by
	 * passing NULL to TclObjGetFrame.
	 */

	levelObj = NULL;
	hasLevel = 0;
    } else {
	/*
	 * Odd number of arguments, so objv[1] must contain the level.
	 */

	levelObj = objv[1];
	hasLevel = 1;
    }

    /*
     * Find the call frame containing each of the "other variables" to be
     * linked to.
     */

    result = TclObjGetFrame(interp, levelObj, &framePtr);
    if (result == -1) {
	return TCL_ERROR;
    }
    if ((result == 0) && hasLevel) {
	/*
	 * Synthesize an error message since TclObjGetFrame doesn't do this
	 * for this particular case.
	 */

	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad level \"%s\"", TclGetString(levelObj)));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "LEVEL",
		TclGetString(levelObj), NULL);
	return TCL_ERROR;
    }

    /*
     * We've now finished with parsing levels; skip to the variable names.
     */

    objc -= hasLevel + 1;
    objv += hasLevel + 1;

    /*
     * Iterate over each (other variable, local variable) pair. Divide the
     * other variable name into two parts, then call MakeUpvar to do all the
     * work of linking it to the local variable.
     */

    for (; objc>0 ; objc-=2, objv+=2) {
	result = ObjMakeUpvar(interp, framePtr, /* othervarName */ objv[0],
		NULL, 0, /* myVarName */ objv[1], /*flags*/ 0, -1);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SetArraySearchObj --
 *
 *	This function converts the given tcl object into one that has the
 *	"array search" internal type.
 *
 * Results:
 *	TCL_OK if the conversion succeeded, and TCL_ERROR if it failed (when
 *	an error message will be placed in the interpreter's result.)
 *
 * Side effects:
 *	Updates the internal type and representation of the object to make
 *	this an array-search object. See the tclArraySearchType declaration
 *	above for details of the internal representation.
 *
 *----------------------------------------------------------------------
 */

static int
SetArraySearchObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    const char *string;
    char *end;			/* Can't be const due to strtoul defn. */
    int id;
    size_t offset;

    /*
     * Get the string representation. Make it up-to-date if necessary.
     */

    string = TclGetString(objPtr);

    /*
     * Parse the id into the three parts separated by dashes.
     */

    if ((string[0] != 's') || (string[1] != '-')) {
	goto syntax;
    }
    id = strtoul(string+2, &end, 10);
    if ((end == (string+2)) || (*end != '-')) {
	goto syntax;
    }

    /*
     * Can't perform value check in this context, so place reference to place
     * in string to use for the check in the object instead.
     */

    end++;
    offset = end - string;

    TclFreeIntRep(objPtr);
    objPtr->typePtr = &tclArraySearchType;
    objPtr->internalRep.twoPtrValue.ptr1 = INT2PTR(id);
    objPtr->internalRep.twoPtrValue.ptr2 = INT2PTR(offset);
    return TCL_OK;

  syntax:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "illegal search identifier \"%s\"", string));
    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ARRAYSEARCH", string, NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseSearchId --
 *
 *	This function translates from a tcl object to a pointer to an active
 *	array search (if there is one that matches the string).
 *
 * Results:
 *	The return value is a pointer to the array search indicated by string,
 *	or NULL if there isn't one. If NULL is returned, the interp's result
 *	contains an error message.
 *
 * Side effects:
 *	The tcl object might have its internal type and representation
 *	modified.
 *
 *----------------------------------------------------------------------
 */

static ArraySearch *
ParseSearchId(
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const Var *varPtr,		/* Array variable search is for. */
    Tcl_Obj *varNamePtr,	/* Name of array variable that search is
				 * supposed to be for. */
    Tcl_Obj *handleObj)		/* Object containing id of search. Must have
				 * form "search-num-var" where "num" is a
				 * decimal number and "var" is a variable
				 * name. */
{
    Interp *iPtr = (Interp *) interp;
    register const char *string;
    register size_t offset;
    int id;
    ArraySearch *searchPtr;
    const char *varName = TclGetString(varNamePtr);

    /*
     * Parse the id.
     */

    if ((handleObj->typePtr != &tclArraySearchType)
	    && (SetArraySearchObj(interp, handleObj) != TCL_OK)) {
	return NULL;
    }

    /*
     * Extract the information out of the Tcl_Obj.
     */

    id = PTR2INT(handleObj->internalRep.twoPtrValue.ptr1);
    string = TclGetString(handleObj);
    offset = PTR2INT(handleObj->internalRep.twoPtrValue.ptr2);

    /*
     * This test cannot be placed inside the Tcl_Obj machinery, since it is
     * dependent on the variable context.
     */

    if (strcmp(string+offset, varName) != 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"search identifier \"%s\" isn't for variable \"%s\"",
		string, varName));
	goto badLookup;
    }

    /*
     * Search through the list of active searches on the interpreter to see if
     * the desired one exists.
     *
     * Note that we cannot store the searchPtr directly in the Tcl_Obj as that
     * would run into trouble when DeleteSearches() was called so we must scan
     * this list every time.
     */

    if (varPtr->flags & VAR_SEARCH_ACTIVE) {
	Tcl_HashEntry *hPtr =
		Tcl_FindHashEntry(&iPtr->varSearches, varPtr);

	for (searchPtr = Tcl_GetHashValue(hPtr); searchPtr != NULL;
		searchPtr = searchPtr->nextPtr) {
	    if (searchPtr->id == id) {
		return searchPtr;
	    }
	}
    }
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "couldn't find search \"%s\"", string));
  badLookup:
    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ARRAYSEARCH", string, NULL);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteSearches --
 *
 *	This function is called to free up all of the searches associated
 *	with an array variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is released to the storage allocator.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteSearches(
    Interp *iPtr,
    register Var *arrayVarPtr)	/* Variable whose searches are to be
				 * deleted. */
{
    ArraySearch *searchPtr, *nextPtr;
    Tcl_HashEntry *sPtr;

    if (arrayVarPtr->flags & VAR_SEARCH_ACTIVE) {
	sPtr = Tcl_FindHashEntry(&iPtr->varSearches, arrayVarPtr);
	for (searchPtr = Tcl_GetHashValue(sPtr); searchPtr != NULL;
		searchPtr = nextPtr) {
	    nextPtr = searchPtr->nextPtr;
	    ckfree(searchPtr);
	}
	arrayVarPtr->flags &= ~VAR_SEARCH_ACTIVE;
	Tcl_DeleteHashEntry(sPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteNamespaceVars --
 *
 *	This function is called to recycle all the storage space associated
 *	with a namespace's table of variables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variables are deleted and trace functions are invoked, if any are
 *	declared.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteNamespaceVars(
    Namespace *nsPtr)
{
    TclVarHashTable *tablePtr = &nsPtr->varTable;
    Tcl_Interp *interp = nsPtr->interp;
    Interp *iPtr = (Interp *)interp;
    Tcl_HashSearch search;
    int flags = 0;
    Var *varPtr;

    /*
     * Determine what flags to pass to the trace callback functions.
     */

    if (nsPtr == iPtr->globalNsPtr) {
	flags = TCL_GLOBAL_ONLY;
    } else if (nsPtr == (Namespace *) TclGetCurrentNamespace(interp)) {
	flags = TCL_NAMESPACE_ONLY;
    }

    for (varPtr = VarHashFirstVar(tablePtr, &search);  varPtr != NULL;
	    varPtr = VarHashFirstVar(tablePtr, &search)) {
	Tcl_Obj *objPtr = Tcl_NewObj();
	VarHashRefCount(varPtr)++;	/* Make sure we get to remove from
					 * hash. */
	Tcl_GetVariableFullName(interp, (Tcl_Var) varPtr, objPtr);
	UnsetVarStruct(varPtr, NULL, iPtr, /* part1 */ objPtr,
		NULL, flags, -1);

	/*
	 * We just unset the variable. However, an unset trace might
	 * have re-set it, or might have re-established traces on it.
	 * This namespace and its vartable are going away unconditionally,
	 * so we cannot let such things linger. That would be a leak.
	 *
	 * First we destroy all traces. ...
	 */

	if (TclIsVarTraced(varPtr)) {
	    Tcl_HashEntry *tPtr = Tcl_FindHashEntry(&iPtr->varTraces, varPtr);
	    VarTrace *tracePtr = Tcl_GetHashValue(tPtr);
	    ActiveVarTrace *activePtr;

	    while (tracePtr) {
		VarTrace *prevPtr = tracePtr;

		tracePtr = tracePtr->nextPtr;
		prevPtr->nextPtr = NULL;
		Tcl_EventuallyFree(prevPtr, TCL_DYNAMIC);
	    }
	    Tcl_DeleteHashEntry(tPtr);
	    varPtr->flags &= ~VAR_ALL_TRACES;
	    for (activePtr = iPtr->activeVarTracePtr; activePtr != NULL;
		    activePtr = activePtr->nextPtr) {
		if (activePtr->varPtr == varPtr) {
		    activePtr->nextTracePtr = NULL;
		}
	    }
	}

	/*
	 * ...and then, if the variable still holds a value, we unset it
	 * again. This time with no traces left, we're sure it goes away.
	 */

	if (!TclIsVarUndefined(varPtr)) {
	    UnsetVarStruct(varPtr, NULL, iPtr, /* part1 */ objPtr,
		    NULL, flags, -1);
	}
	Tcl_DecrRefCount(objPtr); /* free no longer needed obj */
	VarHashRefCount(varPtr)--;
	VarHashDeleteEntry(varPtr);
    }
    VarHashDeleteTable(tablePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteVars --
 *
 *	This function is called to recycle all the storage space associated
 *	with a table of variables. For this function to work correctly, it
 *	must not be possible for any of the variables in the table to be
 *	accessed from Tcl commands (e.g. from trace functions).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variables are deleted and trace functions are invoked, if any are
 *	declared.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteVars(
    Interp *iPtr,		/* Interpreter to which variables belong. */
    TclVarHashTable *tablePtr)	/* Hash table containing variables to
				 * delete. */
{
    Tcl_Interp *interp = (Tcl_Interp *) iPtr;
    Tcl_HashSearch search;
    register Var *varPtr;
    int flags;
    Namespace *currNsPtr = (Namespace *) TclGetCurrentNamespace(interp);

    /*
     * Determine what flags to pass to the trace callback functions.
     */

    flags = TCL_TRACE_UNSETS;
    if (tablePtr == &iPtr->globalNsPtr->varTable) {
	flags |= TCL_GLOBAL_ONLY;
    } else if (tablePtr == &currNsPtr->varTable) {
	flags |= TCL_NAMESPACE_ONLY;
    }

    for (varPtr = VarHashFirstVar(tablePtr, &search); varPtr != NULL;
	 varPtr = VarHashFirstVar(tablePtr, &search)) {
	UnsetVarStruct(varPtr, NULL, iPtr, VarHashGetKey(varPtr), NULL, flags,
		-1);
	VarHashDeleteEntry(varPtr);
    }
    VarHashDeleteTable(tablePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteCompiledLocalVars --
 *
 *	This function is called to recycle storage space associated with the
 *	compiler-allocated array of local variables in a procedure call frame.
 *	This function resembles TclDeleteVars above except that each variable
 *	is stored in a call frame and not a hash table. For this function to
 *	work correctly, it must not be possible for any of the variable in the
 *	table to be accessed from Tcl commands (e.g. from trace functions).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variables are deleted and trace functions are invoked, if any are
 *	declared.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteCompiledLocalVars(
    Interp *iPtr,		/* Interpreter to which variables belong. */
    CallFrame *framePtr)	/* Procedure call frame containing compiler-
				 * assigned local variables to delete. */
{
    register Var *varPtr;
    int numLocals, i;
    Tcl_Obj **namePtrPtr;

    numLocals = framePtr->numCompiledLocals;
    varPtr = framePtr->compiledLocals;
    namePtrPtr = &localName(framePtr, 0);
    for (i=0 ; i<numLocals ; i++, namePtrPtr++, varPtr++) {
	UnsetVarStruct(varPtr, NULL, iPtr, *namePtrPtr, NULL,
		TCL_TRACE_UNSETS, i);
    }
    framePtr->numCompiledLocals = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteArray --
 *
 *	This function is called to free up everything in an array variable.
 *	It's the caller's responsibility to make sure that the array is no
 *	longer accessible before this function is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All storage associated with varPtr's array elements is deleted
 *	(including the array's hash table). Deletion trace functions for
 *	array elements are invoked, then deleted. Any pending traces for array
 *	elements are also deleted.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteArray(
    Interp *iPtr,		/* Interpreter containing array. */
    Tcl_Obj *arrayNamePtr,	/* Name of array (used for trace callbacks),
				 * or NULL if it is to be computed on
				 * demand. */
    Var *varPtr,		/* Pointer to variable structure. */
    int flags,			/* Flags to pass to TclCallVarTraces:
				 * TCL_TRACE_UNSETS and sometimes
				 * TCL_NAMESPACE_ONLY or TCL_GLOBAL_ONLY. */
    int index)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *tPtr;
    register Var *elPtr;
    ActiveVarTrace *activePtr;
    Tcl_Obj *objPtr;
    VarTrace *tracePtr;

    for (elPtr = VarHashFirstVar(varPtr->value.tablePtr, &search);
	    elPtr != NULL; elPtr = VarHashNextVar(&search)) {
	if (TclIsVarScalar(elPtr) && (elPtr->value.objPtr != NULL)) {
	    objPtr = elPtr->value.objPtr;
	    TclDecrRefCount(objPtr);
	    elPtr->value.objPtr = NULL;
	}

	/*
	 * Lie about the validity of the hashtable entry. In this way the
	 * variables will be deleted by VarHashDeleteTable.
	 */

	VarHashInvalidateEntry(elPtr);
	if (TclIsVarTraced(elPtr)) {
	    /*
	     * Compute the array name if it was not supplied.
	     */

	    if (elPtr->flags & VAR_TRACED_UNSET) {
		Tcl_Obj *elNamePtr = VarHashGetKey(elPtr);

		elPtr->flags &= ~VAR_TRACE_ACTIVE;
		TclObjCallVarTraces(iPtr, NULL, elPtr, arrayNamePtr,
			elNamePtr, flags,/* leaveErrMsg */ 0, index);
	    }
	    tPtr = Tcl_FindHashEntry(&iPtr->varTraces, elPtr);
	    tracePtr = Tcl_GetHashValue(tPtr);
	    while (tracePtr) {
		VarTrace *prevPtr = tracePtr;

		tracePtr = tracePtr->nextPtr;
		prevPtr->nextPtr = NULL;
		Tcl_EventuallyFree(prevPtr, TCL_DYNAMIC);
	    }
	    Tcl_DeleteHashEntry(tPtr);
	    elPtr->flags &= ~VAR_ALL_TRACES;
	    for (activePtr = iPtr->activeVarTracePtr; activePtr != NULL;
		    activePtr = activePtr->nextPtr) {
		if (activePtr->varPtr == elPtr) {
		    activePtr->nextTracePtr = NULL;
		}
	    }
	}
	TclSetVarUndefined(elPtr);

	/*
	 * Even though array elements are not supposed to be namespace
	 * variables, some combinations of [upvar] and [variable] may create
	 * such beasts - see [Bug 604239]. This is necessary to avoid leaking
	 * the corresponding Var struct, and is otherwise harmless.
	 */

	TclClearVarNamespaceVar(elPtr);
    }
    VarHashDeleteTable(varPtr->value.tablePtr);
    ckfree(varPtr->value.tablePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjVarErrMsg --
 *
 *	Generate a reasonable error message describing why a variable
 *	operation failed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interp's result is set to hold a message identifying the variable
 *	given by part1 and part2 and describing why the variable operation
 *	failed.
 *
 *----------------------------------------------------------------------
 */

void
TclVarErrMsg(
    Tcl_Interp *interp,		/* Interpreter in which to record message. */
    const char *part1,
    const char *part2,		/* Variable's two-part name. */
    const char *operation,	/* String describing operation that failed,
				 * e.g. "read", "set", or "unset". */
    const char *reason)		/* String describing why operation failed. */
{
    Tcl_Obj *part2Ptr = NULL, *part1Ptr = Tcl_NewStringObj(part1, -1);

    if (part2) {
	part2Ptr = Tcl_NewStringObj(part2, -1);
    }

    TclObjVarErrMsg(interp, part1Ptr, part2Ptr, operation, reason, -1);

    Tcl_DecrRefCount(part1Ptr);
    if (part2Ptr) {
	Tcl_DecrRefCount(part2Ptr);
    }
}

void
TclObjVarErrMsg(
    Tcl_Interp *interp,		/* Interpreter in which to record message. */
    Tcl_Obj *part1Ptr,		/* (may be NULL, if index >= 0) */
    Tcl_Obj *part2Ptr,		/* Variable's two-part name. */
    const char *operation,	/* String describing operation that failed,
				 * e.g. "read", "set", or "unset". */
    const char *reason,		/* String describing why operation failed. */
    int index)			/* Index into the local variable table of the
				 * variable, or -1. Only used when part1Ptr is
				 * NULL. */
{
    if (!part1Ptr) {
	if (index == -1) {
	    Tcl_Panic("invalid part1Ptr and invalid index together");
	}
	part1Ptr = localName(((Interp *)interp)->varFramePtr, index);
    }
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("can't %s \"%s%s%s%s\": %s",
	    operation, TclGetString(part1Ptr), (part2Ptr ? "(" : ""),
	    (part2Ptr ? TclGetString(part2Ptr) : ""), (part2Ptr ? ")" : ""),
	    reason));
}

/*
 *----------------------------------------------------------------------
 *
 * Internal functions for variable name object types --
 *
 *----------------------------------------------------------------------
 */

/*
 * Panic functions that should never be called in normal operation.
 */

static void
PanicOnUpdateVarName(
    Tcl_Obj *objPtr)
{
    Tcl_Panic("%s of type %s should not be called", "updateStringProc",
	    objPtr->typePtr->name);
}

static int
PanicOnSetVarName(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    Tcl_Panic("%s of type %s should not be called", "setFromAnyProc",
	    objPtr->typePtr->name);
    return TCL_ERROR;
}

/*
 * localVarName -
 *
 * INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1:   pointer to name obj in varFramePtr->localCache
 *			  or NULL if it is this same obj
 *   twoPtrValue.ptr2: index into locals table
 */

static void
FreeLocalVarName(
    Tcl_Obj *objPtr)
{
    Tcl_Obj *namePtr = objPtr->internalRep.twoPtrValue.ptr1;

    if (namePtr) {
	Tcl_DecrRefCount(namePtr);
    }
    objPtr->typePtr = NULL;
}

static void
DupLocalVarName(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    Tcl_Obj *namePtr = srcPtr->internalRep.twoPtrValue.ptr1;

    if (!namePtr) {
	namePtr = srcPtr;
    }
    dupPtr->internalRep.twoPtrValue.ptr1 = namePtr;
    Tcl_IncrRefCount(namePtr);

    dupPtr->internalRep.twoPtrValue.ptr2 =
	    srcPtr->internalRep.twoPtrValue.ptr2;
    dupPtr->typePtr = &localVarNameType;
}

/*
 * parsedVarName -
 *
 * INTERNALREP DEFINITION:
 *   twoPtrValue.ptr1 = pointer to the array name Tcl_Obj (NULL if scalar)
 *   twoPtrValue.ptr2 = pointer to the element name string (owned by this
 *			Tcl_Obj), or NULL if it is a scalar variable
 */

static void
FreeParsedVarName(
    Tcl_Obj *objPtr)
{
    register Tcl_Obj *arrayPtr = objPtr->internalRep.twoPtrValue.ptr1;
    register char *elem = objPtr->internalRep.twoPtrValue.ptr2;

    if (arrayPtr != NULL) {
	TclDecrRefCount(arrayPtr);
	ckfree(elem);
    }
    objPtr->typePtr = NULL;
}

static void
DupParsedVarName(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    register Tcl_Obj *arrayPtr = srcPtr->internalRep.twoPtrValue.ptr1;
    register char *elem = srcPtr->internalRep.twoPtrValue.ptr2;
    char *elemCopy;
    unsigned elemLen;

    if (arrayPtr != NULL) {
	Tcl_IncrRefCount(arrayPtr);
	elemLen = strlen(elem);
	elemCopy = ckalloc(elemLen + 1);
	memcpy(elemCopy, elem, elemLen);
	*(elemCopy + elemLen) = '\0';
	elem = elemCopy;
    }

    dupPtr->internalRep.twoPtrValue.ptr1 = arrayPtr;
    dupPtr->internalRep.twoPtrValue.ptr2 = elem;
    dupPtr->typePtr = &tclParsedVarNameType;
}

static void
UpdateParsedVarName(
    Tcl_Obj *objPtr)
{
    Tcl_Obj *arrayPtr = objPtr->internalRep.twoPtrValue.ptr1;
    char *part2 = objPtr->internalRep.twoPtrValue.ptr2;
    const char *part1;
    char *p;
    int len1, len2, totalLen;

    if (arrayPtr == NULL) {
	/*
	 * This is a parsed scalar name: what is it doing here?
	 */

	Tcl_Panic("scalar parsedVarName without a string rep");
    }

    part1 = TclGetStringFromObj(arrayPtr, &len1);
    len2 = strlen(part2);

    totalLen = len1 + len2 + 2;
    p = ckalloc(totalLen + 1);
    objPtr->bytes = p;
    objPtr->length = totalLen;

    memcpy(p, part1, (unsigned) len1);
    p += len1;
    *p++ = '(';
    memcpy(p, part2, (unsigned) len2);
    p += len2;
    *p++ = ')';
    *p = '\0';
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FindNamespaceVar -- MOVED OVER from tclNamesp.c
 *
 *	Searches for a namespace variable, a variable not local to a
 *	procedure. The variable can be either a scalar or an array, but may
 *	not be an element of an array.
 *
 * Results:
 *	Returns a token for the variable if it is found. Otherwise, if it
 *	can't be found or there is an error, returns NULL and leaves an error
 *	message in the interpreter's result object if "flags" contains
 *	TCL_LEAVE_ERR_MSG.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Var
Tcl_FindNamespaceVar(
    Tcl_Interp *interp,		/* The interpreter in which to find the
				 * variable. */
    const char *name,		/* Variable's name. If it starts with "::",
				 * will be looked up in global namespace.
				 * Else, looked up first in contextNsPtr
				 * (current namespace if contextNsPtr is
				 * NULL), then in global namespace. */
    Tcl_Namespace *contextNsPtr,/* Ignored if TCL_GLOBAL_ONLY flag set.
				 * Otherwise, points to namespace in which to
				 * resolve name. If NULL, look up name in the
				 * current namespace. */
    int flags)			/* An OR'd combination of:
				 * TCL_AVOID_RESOLVERS, TCL_GLOBAL_ONLY (look
				 * up name only in global namespace),
				 * TCL_NAMESPACE_ONLY (look up only in
				 * contextNsPtr, or the current namespace if
				 * contextNsPtr is NULL), and
				 * TCL_LEAVE_ERR_MSG. If both TCL_GLOBAL_ONLY
				 * and TCL_NAMESPACE_ONLY are given,
				 * TCL_GLOBAL_ONLY is ignored. */
{
    Tcl_Obj *namePtr = Tcl_NewStringObj(name, -1);
    Tcl_Var var;

    var = ObjFindNamespaceVar(interp, namePtr, contextNsPtr, flags);
    Tcl_DecrRefCount(namePtr);
    return var;
}

static Tcl_Var
ObjFindNamespaceVar(
    Tcl_Interp *interp,		/* The interpreter in which to find the
				 * variable. */
    Tcl_Obj *namePtr,		/* Variable's name. If it starts with "::",
				 * will be looked up in global namespace.
				 * Else, looked up first in contextNsPtr
				 * (current namespace if contextNsPtr is
				 * NULL), then in global namespace. */
    Tcl_Namespace *contextNsPtr,/* Ignored if TCL_GLOBAL_ONLY flag set.
				 * Otherwise, points to namespace in which to
				 * resolve name. If NULL, look up name in the
				 * current namespace. */
    int flags)			/* An OR'd combination of:
				 * TCL_AVOID_RESOLVERS, TCL_GLOBAL_ONLY (look
				 * up name only in global namespace),
				 * TCL_NAMESPACE_ONLY (look up only in
				 * contextNsPtr, or the current namespace if
				 * contextNsPtr is NULL), and
				 * TCL_LEAVE_ERR_MSG. If both TCL_GLOBAL_ONLY
				 * and TCL_NAMESPACE_ONLY are given,
				 * TCL_GLOBAL_ONLY is ignored. */
{
    Interp *iPtr = (Interp *) interp;
    ResolverScheme *resPtr;
    Namespace *nsPtr[2], *cxtNsPtr;
    const char *simpleName;
    Var *varPtr;
    register int search;
    int result;
    Tcl_Var var;
    Tcl_Obj *simpleNamePtr;
    const char *name = TclGetString(namePtr);

    /*
     * If this namespace has a variable resolver, then give it first crack at
     * the variable resolution. It may return a Tcl_Var value, it may signal
     * to continue onward, or it may signal an error.
     */

    if ((flags & TCL_GLOBAL_ONLY) != 0) {
	cxtNsPtr = (Namespace *) TclGetGlobalNamespace(interp);
    } else if (contextNsPtr != NULL) {
	cxtNsPtr = (Namespace *) contextNsPtr;
    } else {
	cxtNsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    }

    if (!(flags & TCL_AVOID_RESOLVERS) &&
	    (cxtNsPtr->varResProc != NULL || iPtr->resolverPtr != NULL)) {
	resPtr = iPtr->resolverPtr;

	if (cxtNsPtr->varResProc) {
	    result = cxtNsPtr->varResProc(interp, name,
		    (Tcl_Namespace *) cxtNsPtr, flags, &var);
	} else {
	    result = TCL_CONTINUE;
	}

	while (result == TCL_CONTINUE && resPtr) {
	    if (resPtr->varResProc) {
		result = resPtr->varResProc(interp, name,
			(Tcl_Namespace *) cxtNsPtr, flags, &var);
	    }
	    resPtr = resPtr->nextPtr;
	}

	if (result == TCL_OK) {
	    return var;
	} else if (result != TCL_CONTINUE) {
	    return NULL;
	}
    }

    /*
     * Find the namespace(s) that contain the variable.
     */

    TclGetNamespaceForQualName(interp, name, (Namespace *) contextNsPtr,
	    flags, &nsPtr[0], &nsPtr[1], &cxtNsPtr, &simpleName);

    /*
     * Look for the variable in the variable table of its namespace. Be sure
     * to check both possible search paths: from the specified namespace
     * context and from the global namespace.
     */

    varPtr = NULL;
    if (simpleName != name) {
	simpleNamePtr = Tcl_NewStringObj(simpleName, -1);
    } else {
	simpleNamePtr = namePtr;
    }

    for (search = 0;  (search < 2) && (varPtr == NULL);  search++) {
	if ((nsPtr[search] != NULL) && (simpleName != NULL)) {
	    varPtr = VarHashFindVar(&nsPtr[search]->varTable, simpleNamePtr);
	}
    }
    if (simpleName != name) {
	Tcl_DecrRefCount(simpleNamePtr);
    }
    if ((varPtr == NULL) && (flags & TCL_LEAVE_ERR_MSG)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown variable \"%s\"", name));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "VARIABLE", name, NULL);
    }
    return (Tcl_Var) varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * InfoVarsCmd -- (moved over from tclCmdIL.c)
 *
 *	Called to implement the "info vars" command that returns the list of
 *	variables in the interpreter that match an optional pattern. The
 *	pattern, if any, consists of an optional sequence of namespace names
 *	separated by "::" qualifiers, which is followed by a glob-style
 *	pattern that restricts which variables are returned. Handles the
 *	following syntax:
 *
 *	    info vars ?pattern?
 *
 * Results:
 *	Returns TCL_OK if successful and TCL_ERROR if there is an error.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If there is an
 *	error, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

int
TclInfoVarsCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    const char *varName, *pattern, *simplePattern;
    Tcl_HashSearch search;
    Var *varPtr;
    Namespace *nsPtr;
    Namespace *globalNsPtr = (Namespace *) Tcl_GetGlobalNamespace(interp);
    Namespace *currNsPtr = (Namespace *) Tcl_GetCurrentNamespace(interp);
    Tcl_Obj *listPtr, *elemObjPtr, *varNamePtr;
    int specificNsInPattern = 0;/* Init. to avoid compiler warning. */
    Tcl_Obj *simplePatternPtr = NULL;

    /*
     * Get the pattern and find the "effective namespace" in which to list
     * variables. We only use this effective namespace if there's no active
     * Tcl procedure frame.
     */

    if (objc == 1) {
	simplePattern = NULL;
	nsPtr = currNsPtr;
	specificNsInPattern = 0;
    } else if (objc == 2) {
	/*
	 * From the pattern, get the effective namespace and the simple
	 * pattern (no namespace qualifiers or ::'s) at the end. If an error
	 * was found while parsing the pattern, return it. Otherwise, if the
	 * namespace wasn't found, just leave nsPtr NULL: we will return an
	 * empty list since no variables there can be found.
	 */

	Namespace *dummy1NsPtr, *dummy2NsPtr;

	pattern = TclGetString(objv[1]);
	TclGetNamespaceForQualName(interp, pattern, NULL, /*flags*/ 0,
		&nsPtr, &dummy1NsPtr, &dummy2NsPtr, &simplePattern);

	if (nsPtr != NULL) {	/* We successfully found the pattern's ns. */
	    specificNsInPattern = (strcmp(simplePattern, pattern) != 0);
	    if (simplePattern == pattern) {
		simplePatternPtr = objv[1];
	    } else {
		simplePatternPtr = Tcl_NewStringObj(simplePattern, -1);
	    }
	    Tcl_IncrRefCount(simplePatternPtr);
	}
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?pattern?");
	return TCL_ERROR;
    }

    /*
     * If the namespace specified in the pattern wasn't found, just return.
     */

    if (nsPtr == NULL) {
	return TCL_OK;
    }

    listPtr = Tcl_NewListObj(0, NULL);

    if (!HasLocalVars(iPtr->varFramePtr) || specificNsInPattern) {
	/*
	 * There is no frame pointer, the frame pointer was pushed only to
	 * activate a namespace, or we are in a procedure call frame but a
	 * specific namespace was specified. Create a list containing only the
	 * variables in the effective namespace's variable table.
	 */

	if (simplePattern && TclMatchIsTrivial(simplePattern)) {
	    /*
	     * If we can just do hash lookups, that simplifies things a lot.
	     */

	    varPtr = VarHashFindVar(&nsPtr->varTable, simplePatternPtr);
	    if (varPtr) {
		if (!TclIsVarUndefined(varPtr)
			|| TclIsVarNamespaceVar(varPtr)) {
		    if (specificNsInPattern) {
			elemObjPtr = Tcl_NewObj();
			Tcl_GetVariableFullName(interp, (Tcl_Var) varPtr,
				elemObjPtr);
		    } else {
			elemObjPtr = VarHashGetKey(varPtr);
		    }
		    Tcl_ListObjAppendElement(interp, listPtr, elemObjPtr);
		}
	    } else if ((nsPtr != globalNsPtr) && !specificNsInPattern) {
		varPtr = VarHashFindVar(&globalNsPtr->varTable,
			simplePatternPtr);
		if (varPtr) {
		    if (!TclIsVarUndefined(varPtr)
			    || TclIsVarNamespaceVar(varPtr)) {
			Tcl_ListObjAppendElement(interp, listPtr,
				VarHashGetKey(varPtr));
		    }
		}
	    }
	} else {
	    /*
	     * Have to scan the tables of variables.
	     */

	    varPtr = VarHashFirstVar(&nsPtr->varTable, &search);
	    while (varPtr) {
		if (!TclIsVarUndefined(varPtr)
			|| TclIsVarNamespaceVar(varPtr)) {
		    varNamePtr = VarHashGetKey(varPtr);
		    varName = TclGetString(varNamePtr);
		    if ((simplePattern == NULL)
			    || Tcl_StringMatch(varName, simplePattern)) {
			if (specificNsInPattern) {
			    elemObjPtr = Tcl_NewObj();
			    Tcl_GetVariableFullName(interp, (Tcl_Var) varPtr,
				    elemObjPtr);
			} else {
			    elemObjPtr = varNamePtr;
			}
			Tcl_ListObjAppendElement(interp, listPtr, elemObjPtr);
		    }
		}
		varPtr = VarHashNextVar(&search);
	    }

	    /*
	     * If the effective namespace isn't the global :: namespace, and a
	     * specific namespace wasn't requested in the pattern (i.e., the
	     * pattern only specifies variable names), then add in all global
	     * :: variables that match the simple pattern. Of course, add in
	     * only those variables that aren't hidden by a variable in the
	     * effective namespace.
	     */

	    if ((nsPtr != globalNsPtr) && !specificNsInPattern) {
		varPtr = VarHashFirstVar(&globalNsPtr->varTable,&search);
		while (varPtr) {
		    if (!TclIsVarUndefined(varPtr)
			    || TclIsVarNamespaceVar(varPtr)) {
			varNamePtr = VarHashGetKey(varPtr);
			varName = TclGetString(varNamePtr);
			if ((simplePattern == NULL)
				|| Tcl_StringMatch(varName, simplePattern)) {
			    if (VarHashFindVar(&nsPtr->varTable,
				    varNamePtr) == NULL) {
				Tcl_ListObjAppendElement(interp, listPtr,
					varNamePtr);
			    }
			}
		    }
		    varPtr = VarHashNextVar(&search);
		}
	    }
	}
    } else if (iPtr->varFramePtr->procPtr != NULL) {
	AppendLocals(interp, listPtr, simplePatternPtr, 1);
    }

    if (simplePatternPtr) {
	Tcl_DecrRefCount(simplePatternPtr);
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * InfoGlobalsCmd -- (moved over from tclCmdIL.c)
 *
 *	Called to implement the "info globals" command that returns the list
 *	of global variables matching an optional pattern. Handles the
 *	following syntax:
 *
 *	    info globals ?pattern?
 *
 * Results:
 *	Returns TCL_OK if successful and TCL_ERROR if there is an error.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If there is an
 *	error, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

int
TclInfoGlobalsCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *varName, *pattern;
    Namespace *globalNsPtr = (Namespace *) Tcl_GetGlobalNamespace(interp);
    Tcl_HashSearch search;
    Var *varPtr;
    Tcl_Obj *listPtr, *varNamePtr, *patternPtr;

    if (objc == 1) {
	pattern = NULL;
    } else if (objc == 2) {
	pattern = TclGetString(objv[1]);

	/*
	 * Strip leading global-namespace qualifiers. [Bug 1057461]
	 */

	if (pattern[0] == ':' && pattern[1] == ':') {
	    while (*pattern == ':') {
		pattern++;
	    }
	}
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?pattern?");
	return TCL_ERROR;
    }

    /*
     * Scan through the global :: namespace's variable table and create a list
     * of all global variables that match the pattern.
     */

    listPtr = Tcl_NewListObj(0, NULL);
    if (pattern != NULL && TclMatchIsTrivial(pattern)) {
	if (pattern == TclGetString(objv[1])) {
	    patternPtr = objv[1];
	} else {
	    patternPtr = Tcl_NewStringObj(pattern, -1);
	}
	Tcl_IncrRefCount(patternPtr);

	varPtr = VarHashFindVar(&globalNsPtr->varTable, patternPtr);
	if (varPtr) {
	    if (!TclIsVarUndefined(varPtr)) {
		Tcl_ListObjAppendElement(interp, listPtr,
			VarHashGetKey(varPtr));
	    }
	}
	Tcl_DecrRefCount(patternPtr);
    } else {
	for (varPtr = VarHashFirstVar(&globalNsPtr->varTable, &search);
		varPtr != NULL;
		varPtr = VarHashNextVar(&search)) {
	    if (TclIsVarUndefined(varPtr)) {
		continue;
	    }
	    varNamePtr = VarHashGetKey(varPtr);
	    varName = TclGetString(varNamePtr);
	    if ((pattern == NULL) || Tcl_StringMatch(varName, pattern)) {
		Tcl_ListObjAppendElement(interp, listPtr, varNamePtr);
	    }
	}
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInfoLocalsCmd -- (moved over from tclCmdIl.c)
 *
 *	Called to implement the "info locals" command to return a list of
 *	local variables that match an optional pattern. Handles the following
 *	syntax:
 *
 *	    info locals ?pattern?
 *
 * Results:
 *	Returns TCL_OK if successful and TCL_ERROR if there is an error.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If there is an
 *	error, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

int
TclInfoLocalsCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *patternPtr, *listPtr;

    if (objc == 1) {
	patternPtr = NULL;
    } else if (objc == 2) {
	patternPtr = objv[1];
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?pattern?");
	return TCL_ERROR;
    }

    if (!HasLocalVars(iPtr->varFramePtr)) {
	return TCL_OK;
    }

    /*
     * Return a list containing names of first the compiled locals (i.e. the
     * ones stored in the call frame), then the variables in the local hash
     * table (if one exists).
     */

    listPtr = Tcl_NewListObj(0, NULL);
    AppendLocals(interp, listPtr, patternPtr, 0);
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AppendLocals --
 *
 *	Append the local variables for the current frame to the specified list
 *	object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AppendLocals(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *listPtr,		/* List object to append names to. */
    Tcl_Obj *patternPtr,	/* Pattern to match against. */
    int includeLinks)		/* 1 if upvars should be included, else 0. */
{
    Interp *iPtr = (Interp *) interp;
    Var *varPtr;
    int i, localVarCt, added;
    Tcl_Obj *objNamePtr;
    const char *varName;
    TclVarHashTable *localVarTablePtr;
    Tcl_HashSearch search;
    Tcl_HashTable addedTable;
    const char *pattern = patternPtr? TclGetString(patternPtr) : NULL;

    localVarCt = iPtr->varFramePtr->numCompiledLocals;
    varPtr = iPtr->varFramePtr->compiledLocals;
    localVarTablePtr = iPtr->varFramePtr->varTablePtr;
    if (includeLinks) {
	Tcl_InitObjHashTable(&addedTable);
    }

    if (localVarCt > 0) {
	Tcl_Obj **varNamePtr = &iPtr->varFramePtr->localCachePtr->varName0;

	for (i = 0; i < localVarCt; i++, varNamePtr++) {
	    /*
	     * Skip nameless (temporary) variables and undefined variables.
	     */

	    if (*varNamePtr && !TclIsVarUndefined(varPtr)
		&& (includeLinks || !TclIsVarLink(varPtr))) {
		varName = TclGetString(*varNamePtr);
		if ((pattern == NULL) || Tcl_StringMatch(varName, pattern)) {
		    Tcl_ListObjAppendElement(interp, listPtr, *varNamePtr);
		    if (includeLinks) {
			Tcl_CreateHashEntry(&addedTable, *varNamePtr, &added);
		    }
		}
	    }
	    varPtr++;
	}
    }

    /*
     * Do nothing if no local variables.
     */

    if (localVarTablePtr == NULL) {
	goto objectVars;
    }

    /*
     * Check for the simple and fast case.
     */

    if ((pattern != NULL) && TclMatchIsTrivial(pattern)) {
	varPtr = VarHashFindVar(localVarTablePtr, patternPtr);
	if (varPtr != NULL) {
	    if (!TclIsVarUndefined(varPtr)
		    && (includeLinks || !TclIsVarLink(varPtr))) {
		Tcl_ListObjAppendElement(interp, listPtr,
			VarHashGetKey(varPtr));
		if (includeLinks) {
		    Tcl_CreateHashEntry(&addedTable, VarHashGetKey(varPtr),
			    &added);
		}
	    }
	}
	goto objectVars;
    }

    /*
     * Scan over and process all local variables.
     */

    for (varPtr = VarHashFirstVar(localVarTablePtr, &search);
	    varPtr != NULL;
	    varPtr = VarHashNextVar(&search)) {
	if (!TclIsVarUndefined(varPtr)
		&& (includeLinks || !TclIsVarLink(varPtr))) {
	    objNamePtr = VarHashGetKey(varPtr);
	    varName = TclGetString(objNamePtr);
	    if ((pattern == NULL) || Tcl_StringMatch(varName, pattern)) {
		Tcl_ListObjAppendElement(interp, listPtr, objNamePtr);
		if (includeLinks) {
		    Tcl_CreateHashEntry(&addedTable, objNamePtr, &added);
		}
	    }
	}
    }

  objectVars:
    if (!includeLinks) {
	return;
    }

    if (iPtr->varFramePtr->isProcCallFrame & FRAME_IS_METHOD) {
	CallContext *contextPtr = iPtr->varFramePtr->clientData;
	Method *mPtr = contextPtr->callPtr->chain[contextPtr->index].mPtr;

	if (mPtr->declaringObjectPtr) {
	    FOREACH(objNamePtr, mPtr->declaringObjectPtr->variables) {
		Tcl_CreateHashEntry(&addedTable, objNamePtr, &added);
		if (added && (!pattern ||
			Tcl_StringMatch(TclGetString(objNamePtr), pattern))) {
		    Tcl_ListObjAppendElement(interp, listPtr, objNamePtr);
		}
	    }
	} else {
	    FOREACH(objNamePtr, mPtr->declaringClassPtr->variables) {
		Tcl_CreateHashEntry(&addedTable, objNamePtr, &added);
		if (added && (!pattern ||
			Tcl_StringMatch(TclGetString(objNamePtr), pattern))) {
		    Tcl_ListObjAppendElement(interp, listPtr, objNamePtr);
		}
	    }
	}
    }
    Tcl_DeleteHashTable(&addedTable);
}

/*
 * Hash table implementation - first, just copy and adapt the obj key stuff
 */

void
TclInitVarHashTable(
    TclVarHashTable *tablePtr,
    Namespace *nsPtr)
{
    Tcl_InitCustomHashTable(&tablePtr->table,
	    TCL_CUSTOM_TYPE_KEYS, &tclVarHashKeyType);
    tablePtr->nsPtr = nsPtr;
}

static Tcl_HashEntry *
AllocVarEntry(
    Tcl_HashTable *tablePtr,	/* Hash table. */
    void *keyPtr)		/* Key to store in the hash table entry. */
{
    Tcl_Obj *objPtr = keyPtr;
    Tcl_HashEntry *hPtr;
    Var *varPtr;

    varPtr = ckalloc(sizeof(VarInHash));
    varPtr->flags = VAR_IN_HASHTABLE;
    varPtr->value.objPtr = NULL;
    VarHashRefCount(varPtr) = 1;

    hPtr = &(((VarInHash *) varPtr)->entry);
    Tcl_SetHashValue(hPtr, varPtr);
    hPtr->key.objPtr = objPtr;
    Tcl_IncrRefCount(objPtr);

    return hPtr;
}

static void
FreeVarEntry(
    Tcl_HashEntry *hPtr)
{
    Var *varPtr = VarHashGetValue(hPtr);
    Tcl_Obj *objPtr = hPtr->key.objPtr;

    if (TclIsVarUndefined(varPtr) && !TclIsVarTraced(varPtr)
	    && (VarHashRefCount(varPtr) == 1)) {
	ckfree(varPtr);
    } else {
	VarHashInvalidateEntry(varPtr);
	TclSetVarUndefined(varPtr);
	VarHashRefCount(varPtr)--;
    }
    Tcl_DecrRefCount(objPtr);
}

static int
CompareVarKeys(
    void *keyPtr,		/* New key to compare. */
    Tcl_HashEntry *hPtr)	/* Existing key to compare. */
{
    Tcl_Obj *objPtr1 = keyPtr;
    Tcl_Obj *objPtr2 = hPtr->key.objPtr;
    register const char *p1, *p2;
    register int l1, l2;

    /*
     * If the object pointers are the same then they match.
     * OPT: this comparison was moved to the caller

       if (objPtr1 == objPtr2) return 1;
    */

    /*
     * Don't use Tcl_GetStringFromObj as it would prevent l1 and l2 being in a
     * register.
     */

    p1 = TclGetString(objPtr1);
    l1 = objPtr1->length;
    p2 = TclGetString(objPtr2);
    l2 = objPtr2->length;

    /*
     * Only compare string representations of the same length.
     */

    return ((l1 == l2) && !memcmp(p1, p2, l1));
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
