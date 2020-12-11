/*
 * tclIndexObj.c --
 *
 *	This file implements objects of type "index". This object type is used
 *	to lookup a keyword in a table of valid values and cache the index of
 *	the matching entry. Also provides table-based argv/argc processing.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1997 Sun Microsystems, Inc.
 * Copyright (c) 2006 Sam Bromley.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Prototypes for functions defined later in this file:
 */

static int		GetIndexFromObjList(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, Tcl_Obj *tableObjPtr,
			    const char *msg, int flags, int *indexPtr);
static int		SetIndexFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);
static void		UpdateStringOfIndex(Tcl_Obj *objPtr);
static void		DupIndex(Tcl_Obj *srcPtr, Tcl_Obj *dupPtr);
static void		FreeIndex(Tcl_Obj *objPtr);
static int		PrefixAllObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		PrefixLongestObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		PrefixMatchObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		PrintUsage(Tcl_Interp *interp,
			    const Tcl_ArgvInfo *argTable);

/*
 * The structure below defines the index Tcl object type by means of functions
 * that can be invoked by generic object code.
 */

static const Tcl_ObjType indexType = {
    "index",			/* name */
    FreeIndex,			/* freeIntRepProc */
    DupIndex,			/* dupIntRepProc */
    UpdateStringOfIndex,	/* updateStringProc */
    SetIndexFromAny		/* setFromAnyProc */
};

/*
 * The definition of the internal representation of the "index" object; The
 * internalRep.twoPtrValue.ptr1 field of an object of "index" type will be a
 * pointer to one of these structures.
 *
 * Keep this structure declaration in sync with tclTestObj.c
 */

typedef struct {
    void *tablePtr;		/* Pointer to the table of strings */
    int offset;			/* Offset between table entries */
    int index;			/* Selected index into table. */
} IndexRep;

/*
 * The following macros greatly simplify moving through a table...
 */

#define STRING_AT(table, offset) \
	(*((const char *const *)(((char *)(table)) + (offset))))
#define NEXT_ENTRY(table, offset) \
	(&(STRING_AT(table, offset)))
#define EXPAND_OF(indexRep) \
	STRING_AT((indexRep)->tablePtr, (indexRep)->offset*(indexRep)->index)

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetIndexFromObj --
 *
 *	This function looks up an object's value in a table of strings and
 *	returns the index of the matching string, if any.
 *
 * Results:
 *	If the value of objPtr is identical to or a unique abbreviation for
 *	one of the entries in tablePtr, then the return value is TCL_OK and the
 *	index of the matching entry is stored at *indexPtr. If there isn't a
 *	proper match, then TCL_ERROR is returned and an error message is left
 *	in interp's result (unless interp is NULL). The msg argument is used
 *	in the error message; for example, if msg has the value "option" then
 *	the error message will say something flag 'bad option "foo": must be
 *	...'
 *
 * Side effects:
 *	The result of the lookup is cached as the internal rep of objPtr, so
 *	that repeated lookups can be done quickly.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_GetIndexFromObj
int
Tcl_GetIndexFromObj(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr,		/* Object containing the string to lookup. */
    const char *const*tablePtr,	/* Array of strings to compare against the
				 * value of objPtr; last entry must be NULL
				 * and there must not be duplicate entries. */
    const char *msg,		/* Identifying word to use in error
				 * messages. */
    int flags,			/* 0 or TCL_EXACT */
    int *indexPtr)		/* Place to store resulting integer index. */
{

    /*
     * See if there is a valid cached result from a previous lookup (doing the
     * check here saves the overhead of calling Tcl_GetIndexFromObjStruct in
     * the common case where the result is cached).
     */

    if (objPtr->typePtr == &indexType) {
	IndexRep *indexRep = objPtr->internalRep.twoPtrValue.ptr1;

	/*
	 * Here's hoping we don't get hit by unfortunate packing constraints
	 * on odd platforms like a Cray PVP...
	 */

	if (indexRep->tablePtr == (void *) tablePtr
		&& indexRep->offset == sizeof(char *)) {
	    *indexPtr = indexRep->index;
	    return TCL_OK;
	}
    }
    return Tcl_GetIndexFromObjStruct(interp, objPtr, tablePtr, sizeof(char *),
	    msg, flags, indexPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * GetIndexFromObjList --
 *
 *	This procedure looks up an object's value in a table of strings and
 *	returns the index of the matching string, if any.
 *
 * Results:
 *	If the value of objPtr is identical to or a unique abbreviation for
 *	one of the entries in tableObjPtr, then the return value is TCL_OK and
 *	the index of the matching entry is stored at *indexPtr. If there isn't
 *	a proper match, then TCL_ERROR is returned and an error message is
 *	left in interp's result (unless interp is NULL). The msg argument is
 *	used in the error message; for example, if msg has the value "option"
 *	then the error message will say something flag 'bad option "foo": must
 *	be ...'
 *
 * Side effects:
 *	Removes any internal representation that the object might have. (TODO:
 *	find a way to cache the lookup.)
 *
 *----------------------------------------------------------------------
 */

int
GetIndexFromObjList(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr,		/* Object containing the string to lookup. */
    Tcl_Obj *tableObjPtr,	/* List of strings to compare against the
				 * value of objPtr. */
    const char *msg,		/* Identifying word to use in error
				 * messages. */
    int flags,			/* 0 or TCL_EXACT */
    int *indexPtr)		/* Place to store resulting integer index. */
{

    int objc, result, t;
    Tcl_Obj **objv;
    const char **tablePtr;

    /*
     * Use Tcl_GetIndexFromObjStruct to do the work to avoid duplicating most
     * of the code there. This is a bit ineffiecient but simpler.
     */

    result = Tcl_ListObjGetElements(interp, tableObjPtr, &objc, &objv);
    if (result != TCL_OK) {
	return result;
    }

    /*
     * Build a string table from the list.
     */

    tablePtr = ckalloc((objc + 1) * sizeof(char *));
    for (t = 0; t < objc; t++) {
	if (objv[t] == objPtr) {
	    /*
	     * An exact match is always chosen, so we can stop here.
	     */

	    ckfree(tablePtr);
	    *indexPtr = t;
	    return TCL_OK;
	}

	tablePtr[t] = Tcl_GetString(objv[t]);
    }
    tablePtr[objc] = NULL;

    result = Tcl_GetIndexFromObjStruct(interp, objPtr, tablePtr,
	    sizeof(char *), msg, flags, indexPtr);

    /*
     * The internal rep must be cleared since tablePtr will go away.
     */

    TclFreeIntRep(objPtr);
    ckfree(tablePtr);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetIndexFromObjStruct --
 *
 *	This function looks up an object's value given a starting string and
 *	an offset for the amount of space between strings. This is useful when
 *	the strings are embedded in some other kind of array.
 *
 * Results:
 *	If the value of objPtr is identical to or a unique abbreviation for
 *	one of the entries in tablePtr, then the return value is TCL_OK and
 *	the index of the matching entry is stored at *indexPtr. If there isn't
 *	a proper match, then TCL_ERROR is returned and an error message is
 *	left in interp's result (unless interp is NULL). The msg argument is
 *	used in the error message; for example, if msg has the value "option"
 *	then the error message will say something like 'bad option "foo": must
 *	be ...'
 *
 * Side effects:
 *	The result of the lookup is cached as the internal rep of objPtr, so
 *	that repeated lookups can be done quickly.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetIndexFromObjStruct(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr,		/* Object containing the string to lookup. */
    const void *tablePtr,	/* The first string in the table. The second
				 * string will be at this address plus the
				 * offset, the third plus the offset again,
				 * etc. The last entry must be NULL and there
				 * must not be duplicate entries. */
    int offset,			/* The number of bytes between entries */
    const char *msg,		/* Identifying word to use in error
				 * messages. */
    int flags,			/* 0 or TCL_EXACT */
    int *indexPtr)		/* Place to store resulting integer index. */
{
    int index, idx, numAbbrev;
    const char *key, *p1;
    const char *p2;
    const char *const *entryPtr;
    Tcl_Obj *resultPtr;
    IndexRep *indexRep;

    /* Protect against invalid values, like -1 or 0. */
    if (offset < (int)sizeof(char *)) {
	offset = (int)sizeof(char *);
    }
    /*
     * See if there is a valid cached result from a previous lookup.
     */

    if (objPtr->typePtr == &indexType) {
	indexRep = objPtr->internalRep.twoPtrValue.ptr1;
	if (indexRep->tablePtr==tablePtr && indexRep->offset==offset) {
	    *indexPtr = indexRep->index;
	    return TCL_OK;
	}
    }

    /*
     * Lookup the value of the object in the table. Accept unique
     * abbreviations unless TCL_EXACT is set in flags.
     */

    key = TclGetString(objPtr);
    index = -1;
    numAbbrev = 0;

    /*
     * Scan the table looking for one of:
     *  - An exact match (always preferred)
     *  - A single abbreviation (allowed depending on flags)
     *  - Several abbreviations (never allowed, but overridden by exact match)
     */

    for (entryPtr = tablePtr, idx = 0; *entryPtr != NULL;
	    entryPtr = NEXT_ENTRY(entryPtr, offset), idx++) {
	for (p1 = key, p2 = *entryPtr; *p1 == *p2; p1++, p2++) {
	    if (*p1 == '\0') {
		index = idx;
		goto done;
	    }
	}
	if (*p1 == '\0') {
	    /*
	     * The value is an abbreviation for this entry. Continue checking
	     * other entries to make sure it's unique. If we get more than one
	     * unique abbreviation, keep searching to see if there is an exact
	     * match, but remember the number of unique abbreviations and
	     * don't allow either.
	     */

	    numAbbrev++;
	    index = idx;
	}
    }

    /*
     * Check if we were instructed to disallow abbreviations.
     */

    if ((flags & TCL_EXACT) || (key[0] == '\0') || (numAbbrev != 1)) {
	goto error;
    }

  done:
    /*
     * Cache the found representation. Note that we want to avoid allocating a
     * new internal-rep if at all possible since that is potentially a slow
     * operation.
     */

    if (objPtr->typePtr == &indexType) {
	indexRep = objPtr->internalRep.twoPtrValue.ptr1;
    } else {
	TclFreeIntRep(objPtr);
	indexRep = ckalloc(sizeof(IndexRep));
	objPtr->internalRep.twoPtrValue.ptr1 = indexRep;
	objPtr->typePtr = &indexType;
    }
    indexRep->tablePtr = (void *) tablePtr;
    indexRep->offset = offset;
    indexRep->index = index;

    *indexPtr = index;
    return TCL_OK;

  error:
    if (interp != NULL) {
	/*
	 * Produce a fancy error message.
	 */

	int count = 0;

	TclNewObj(resultPtr);
	entryPtr = tablePtr;
	while ((*entryPtr != NULL) && !**entryPtr) {
	    entryPtr = NEXT_ENTRY(entryPtr, offset);
	}
	Tcl_AppendStringsToObj(resultPtr,
		(numAbbrev>1 && !(flags & TCL_EXACT) ? "ambiguous " : "bad "),
		msg, " \"", key, NULL);
	if (*entryPtr == NULL) {
	    Tcl_AppendStringsToObj(resultPtr, "\": no valid options", NULL);
	} else {
	    Tcl_AppendStringsToObj(resultPtr, "\": must be ",
		    *entryPtr, NULL);
	    entryPtr = NEXT_ENTRY(entryPtr, offset);
	    while (*entryPtr != NULL) {
		if (*NEXT_ENTRY(entryPtr, offset) == NULL) {
		    Tcl_AppendStringsToObj(resultPtr, (count > 0 ? "," : ""),
			    " or ", *entryPtr, NULL);
		} else if (**entryPtr) {
		    Tcl_AppendStringsToObj(resultPtr, ", ", *entryPtr, NULL);
		    count++;
		}
		entryPtr = NEXT_ENTRY(entryPtr, offset);
	    }
	}
	Tcl_SetObjResult(interp, resultPtr);
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INDEX", msg, key, NULL);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * SetIndexFromAny --
 *
 *	This function is called to convert a Tcl object to index internal
 *	form. However, this doesn't make sense (need to have a table of
 *	keywords in order to do the conversion) so the function always
 *	generates an error.
 *
 * Results:
 *	The return value is always TCL_ERROR, and an error message is left in
 *	interp's result if interp isn't NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SetIndexFromAny(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr)	/* The object to convert. */
{
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
	    "can't convert value to index except via Tcl_GetIndexFromObj API",
	    -1));
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfIndex --
 *
 *	This function is called to convert a Tcl object from index internal
 *	form to its string form. No abbreviation is ever generated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The string representation of the object is updated.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfIndex(
    Tcl_Obj *objPtr)
{
    IndexRep *indexRep = objPtr->internalRep.twoPtrValue.ptr1;
    register char *buf;
    register unsigned len;
    register const char *indexStr = EXPAND_OF(indexRep);

    len = strlen(indexStr);
    buf = ckalloc(len + 1);
    memcpy(buf, indexStr, len+1);
    objPtr->bytes = buf;
    objPtr->length = len;
}

/*
 *----------------------------------------------------------------------
 *
 * DupIndex --
 *
 *	This function is called to copy the internal rep of an index Tcl
 *	object from to another object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The internal representation of the target object is updated and the
 *	type is set.
 *
 *----------------------------------------------------------------------
 */

static void
DupIndex(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    IndexRep *srcIndexRep = srcPtr->internalRep.twoPtrValue.ptr1;
    IndexRep *dupIndexRep = ckalloc(sizeof(IndexRep));

    memcpy(dupIndexRep, srcIndexRep, sizeof(IndexRep));
    dupPtr->internalRep.twoPtrValue.ptr1 = dupIndexRep;
    dupPtr->typePtr = &indexType;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeIndex --
 *
 *	This function is called to delete the internal rep of an index Tcl
 *	object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The internal representation of the target object is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
FreeIndex(
    Tcl_Obj *objPtr)
{
    ckfree(objPtr->internalRep.twoPtrValue.ptr1);
    objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitPrefixCmd --
 *
 *	This procedure creates the "prefix" Tcl command. See the user
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

Tcl_Command
TclInitPrefixCmd(
    Tcl_Interp *interp)		/* Current interpreter. */
{
    static const EnsembleImplMap prefixImplMap[] = {
	{"all",	    PrefixAllObjCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{"longest", PrefixLongestObjCmd,TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{"match",   PrefixMatchObjCmd,	TclCompileBasicMin2ArgCmd, NULL, NULL, 0},
	{NULL, NULL, NULL, NULL, NULL, 0}
    };
    Tcl_Command prefixCmd;

    prefixCmd = TclMakeEnsemble(interp, "::tcl::prefix", prefixImplMap);
    Tcl_Export(interp, Tcl_FindNamespace(interp, "::tcl", NULL, 0),
	    "prefix", 0);
    return prefixCmd;
}

/*----------------------------------------------------------------------
 *
 * PrefixMatchObjCmd --
 *
 *	This function implements the 'prefix match' Tcl command. Refer to the
 *	user documentation for details on what it does.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
PrefixMatchObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int flags = 0, result, index;
    int dummyLength, i, errorLength;
    Tcl_Obj *errorPtr = NULL;
    const char *message = "option";
    Tcl_Obj *tablePtr, *objPtr, *resultPtr;
    static const char *const matchOptions[] = {
	"-error", "-exact", "-message", NULL
    };
    enum matchOptions {
	PRFMATCH_ERROR, PRFMATCH_EXACT, PRFMATCH_MESSAGE
    };

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "?options? table string");
	return TCL_ERROR;
    }

    for (i = 1; i < (objc - 2); i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], matchOptions, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum matchOptions) index) {
	case PRFMATCH_EXACT:
	    flags |= TCL_EXACT;
	    break;
	case PRFMATCH_MESSAGE:
	    if (i > objc-4) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"missing value for -message", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "NOARG", NULL);
		return TCL_ERROR;
	    }
	    i++;
	    message = Tcl_GetString(objv[i]);
	    break;
	case PRFMATCH_ERROR:
	    if (i > objc-4) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"missing value for -error", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "NOARG", NULL);
		return TCL_ERROR;
	    }
	    i++;
	    result = Tcl_ListObjLength(interp, objv[i], &errorLength);
	    if (result != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((errorLength % 2) != 0) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"error options must have an even number of elements",
			-1));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "DICTIONARY", NULL);
		return TCL_ERROR;
	    }
	    errorPtr = objv[i];
	    break;
	}
    }

    tablePtr = objv[objc - 2];
    objPtr = objv[objc - 1];

    /*
     * Check that table is a valid list first, since we want to handle that
     * error case regardless of level.
     */

    result = Tcl_ListObjLength(interp, tablePtr, &dummyLength);
    if (result != TCL_OK) {
	return result;
    }

    result = GetIndexFromObjList(interp, objPtr, tablePtr, message, flags,
	    &index);
    if (result != TCL_OK) {
	if (errorPtr != NULL && errorLength == 0) {
	    Tcl_ResetResult(interp);
	    return TCL_OK;
	} else if (errorPtr == NULL) {
	    return TCL_ERROR;
	}

	if (Tcl_IsShared(errorPtr)) {
	    errorPtr = Tcl_DuplicateObj(errorPtr);
	}
	Tcl_ListObjAppendElement(interp, errorPtr,
		Tcl_NewStringObj("-code", 5));
	Tcl_ListObjAppendElement(interp, errorPtr, Tcl_NewIntObj(result));

	return Tcl_SetReturnOptions(interp, errorPtr);
    }

    result = Tcl_ListObjIndex(interp, tablePtr, index, &resultPtr);
    if (result != TCL_OK) {
	return result;
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 * PrefixAllObjCmd --
 *
 *	This function implements the 'prefix all' Tcl command. Refer to the
 *	user documentation for details on what it does.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
PrefixAllObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int tableObjc, result, t, length, elemLength;
    const char *string, *elemString;
    Tcl_Obj **tableObjv, *resultPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "table string");
	return TCL_ERROR;
    }

    result = Tcl_ListObjGetElements(interp, objv[1], &tableObjc, &tableObjv);
    if (result != TCL_OK) {
	return result;
    }
    resultPtr = Tcl_NewListObj(0, NULL);
    string = Tcl_GetStringFromObj(objv[2], &length);

    for (t = 0; t < tableObjc; t++) {
	elemString = Tcl_GetStringFromObj(tableObjv[t], &elemLength);

	/*
	 * A prefix cannot match if it is longest.
	 */

	if (length <= elemLength) {
	    if (TclpUtfNcmp2(elemString, string, length) == 0) {
		Tcl_ListObjAppendElement(interp, resultPtr, tableObjv[t]);
	    }
	}
    }

    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 * PrefixLongestObjCmd --
 *
 *	This function implements the 'prefix longest' Tcl command. Refer to
 *	the user documentation for details on what it does.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
PrefixLongestObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int tableObjc, result, i, t, length, elemLength, resultLength;
    const char *string, *elemString, *resultString;
    Tcl_Obj **tableObjv;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "table string");
	return TCL_ERROR;
    }

    result = Tcl_ListObjGetElements(interp, objv[1], &tableObjc, &tableObjv);
    if (result != TCL_OK) {
	return result;
    }
    string = Tcl_GetStringFromObj(objv[2], &length);

    resultString = NULL;
    resultLength = 0;

    for (t = 0; t < tableObjc; t++) {
	elemString = Tcl_GetStringFromObj(tableObjv[t], &elemLength);

	/*
	 * First check if the prefix string matches the element. A prefix
	 * cannot match if it is longest.
	 */

	if ((length > elemLength) ||
		TclpUtfNcmp2(elemString, string, length) != 0) {
	    continue;
	}

	if (resultString == NULL) {
	    /*
	     * If this is the first match, the longest common substring this
	     * far is the complete string. The result is part of this string
	     * so we only need to adjust the length later.
	     */

	    resultString = elemString;
	    resultLength = elemLength;
	} else {
	    /*
	     * Longest common substring cannot be longer than shortest string.
	     */

	    if (elemLength < resultLength) {
		resultLength = elemLength;
	    }

	    /*
	     * Compare strings.
	     */

	    for (i = 0; i < resultLength; i++) {
		if (resultString[i] != elemString[i]) {
		    /*
		     * Adjust in case we stopped in the middle of a UTF char.
		     */

		    resultLength = Tcl_UtfPrev(&resultString[i+1],
			    resultString) - resultString;
		    break;
		}
	    }
	}
    }
    if (resultLength > 0) {
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj(resultString, resultLength));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WrongNumArgs --
 *
 *	This function generates a "wrong # args" error message in an
 *	interpreter. It is used as a utility function by many command
 *	functions, including the function that implements procedures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error message is generated in interp's result object to indicate
 *	that a command was invoked with the wrong number of arguments. The
 *	message has the form
 *		wrong # args: should be "foo bar additional stuff"
 *	where "foo" and "bar" are the initial objects in objv (objc determines
 *	how many of these are printed) and "additional stuff" is the contents
 *	of the message argument.
 *
 *	The message printed is modified somewhat if the command is wrapped
 *	inside an ensemble. In that case, the error message generated is
 *	rewritten in such a way that it appears to be generated from the
 *	user-visible command and not how that command is actually implemented,
 *	giving a better overall user experience.
 *
 *	Internally, the Tcl core may set the flag INTERP_ALTERNATE_WRONG_ARGS
 *	in the interpreter to generate complex multi-part messages by calling
 *	this function repeatedly. This allows the code that knows how to
 *	handle ensemble-related error messages to be kept here while still
 *	generating suitable error messages for commands like [read] and
 *	[socket]. Ideally, this would be done through an extra flags argument,
 *	but that wouldn't be source-compatible with the existing API and it's
 *	a fairly rare requirement anyway.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_WrongNumArgs(
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments to print from objv. */
    Tcl_Obj *const objv[],	/* Initial argument objects, which should be
				 * included in the error message. */
    const char *message)	/* Error message to print after the leading
				 * objects in objv. The message may be
				 * NULL. */
{
    Tcl_Obj *objPtr;
    int i, len, elemLen;
    char flags;
    Interp *iPtr = (Interp *) interp;
    const char *elementStr;

    /*
     * [incr Tcl] does something fairly horrific when generating error
     * messages for its ensembles; it passes the whole set of ensemble
     * arguments as a list in the first argument. This means that this code
     * causes a problem in iTcl if it attempts to correctly quote all
     * arguments, which would be the correct thing to do. We work around this
     * nasty behaviour for now, and hope that we can remove it all in the
     * future...
     */

#ifndef AVOID_HACKS_FOR_ITCL
    int isFirst = 1;		/* Special flag used to inhibit the treating
				 * of the first word as a list element so the
				 * hacky way Itcl generates error messages for
				 * its ensembles will still work. [Bug
				 * 1066837] */
#   define MAY_QUOTE_WORD	(!isFirst)
#   define AFTER_FIRST_WORD	(isFirst = 0)
#else /* !AVOID_HACKS_FOR_ITCL */
#   define MAY_QUOTE_WORD	1
#   define AFTER_FIRST_WORD	(void) 0
#endif /* AVOID_HACKS_FOR_ITCL */

    TclNewObj(objPtr);
    if (iPtr->flags & INTERP_ALTERNATE_WRONG_ARGS) {
	iPtr->flags &= ~INTERP_ALTERNATE_WRONG_ARGS;
	Tcl_AppendObjToObj(objPtr, Tcl_GetObjResult(interp));
	Tcl_AppendToObj(objPtr, " or \"", -1);
    } else {
	Tcl_AppendToObj(objPtr, "wrong # args: should be \"", -1);
    }

    /*
     * Check to see if we are processing an ensemble implementation, and if so
     * rewrite the results in terms of how the ensemble was invoked.
     */

    if (iPtr->ensembleRewrite.sourceObjs != NULL) {
	int toSkip = iPtr->ensembleRewrite.numInsertedObjs;
	int toPrint = iPtr->ensembleRewrite.numRemovedObjs;
	Tcl_Obj *const *origObjv = iPtr->ensembleRewrite.sourceObjs;

	/*
	 * Check for spelling fixes, and substitute the fixed values.
	 */

	if (origObjv[0] == NULL) {
	    origObjv = (Tcl_Obj *const *)origObjv[2];
	}

	/*
	 * We only know how to do rewriting if all the replaced objects are
	 * actually arguments (in objv) to this function. Otherwise it just
	 * gets too complicated and we'd be better off just giving a slightly
	 * confusing error message...
	 */

	if (objc < toSkip) {
	    goto addNormalArgumentsToMessage;
	}

	/*
	 * Strip out the actual arguments that the ensemble inserted.
	 */

	objv += toSkip;
	objc -= toSkip;

	/*
	 * We assume no object is of index type.
	 */

	for (i=0 ; i<toPrint ; i++) {
	    /*
	     * Add the element, quoting it if necessary.
	     */

	    if (origObjv[i]->typePtr == &indexType) {
		register IndexRep *indexRep =
			origObjv[i]->internalRep.twoPtrValue.ptr1;

		elementStr = EXPAND_OF(indexRep);
		elemLen = strlen(elementStr);
	    } else {
		elementStr = TclGetStringFromObj(origObjv[i], &elemLen);
	    }
	    flags = 0;
	    len = TclScanElement(elementStr, elemLen, &flags);

	    if (MAY_QUOTE_WORD && len != elemLen) {
		char *quotedElementStr = TclStackAlloc(interp,
			(unsigned)len + 1);

		len = TclConvertElement(elementStr, elemLen,
			quotedElementStr, flags);
		Tcl_AppendToObj(objPtr, quotedElementStr, len);
		TclStackFree(interp, quotedElementStr);
	    } else {
		Tcl_AppendToObj(objPtr, elementStr, elemLen);
	    }

	    AFTER_FIRST_WORD;

	    /*
	     * Add a space if the word is not the last one (which has a
	     * moderately complex condition here).
	     */

	    if (i<toPrint-1 || objc!=0 || message!=NULL) {
		Tcl_AppendStringsToObj(objPtr, " ", NULL);
	    }
	}
    }

    /*
     * Now add the arguments (other than those rewritten) that the caller took
     * from its calling context.
     */

  addNormalArgumentsToMessage:
    for (i = 0; i < objc; i++) {
	/*
	 * If the object is an index type use the index table which allows for
	 * the correct error message even if the subcommand was abbreviated.
	 * Otherwise, just use the string rep.
	 */

	if (objv[i]->typePtr == &indexType) {
	    register IndexRep *indexRep = objv[i]->internalRep.twoPtrValue.ptr1;

	    Tcl_AppendStringsToObj(objPtr, EXPAND_OF(indexRep), NULL);
	} else {
	    /*
	     * Quote the argument if it contains spaces (Bug 942757).
	     */

	    elementStr = TclGetStringFromObj(objv[i], &elemLen);
	    flags = 0;
	    len = TclScanElement(elementStr, elemLen, &flags);

	    if (MAY_QUOTE_WORD && len != elemLen) {
		char *quotedElementStr = TclStackAlloc(interp,
			(unsigned) len + 1);

		len = TclConvertElement(elementStr, elemLen,
			quotedElementStr, flags);
		Tcl_AppendToObj(objPtr, quotedElementStr, len);
		TclStackFree(interp, quotedElementStr);
	    } else {
		Tcl_AppendToObj(objPtr, elementStr, elemLen);
	    }
	}

	AFTER_FIRST_WORD;

	/*
	 * Append a space character (" ") if there is more text to follow
	 * (either another element from objv, or the message string).
	 */

	if (i<objc-1 || message!=NULL) {
	    Tcl_AppendStringsToObj(objPtr, " ", NULL);
	}
    }

    /*
     * Add any trailing message bits and set the resulting string as the
     * interpreter result. Caller is responsible for reporting this as an
     * actual error.
     */

    if (message != NULL) {
	Tcl_AppendStringsToObj(objPtr, message, NULL);
    }
    Tcl_AppendStringsToObj(objPtr, "\"", NULL);
    Tcl_SetErrorCode(interp, "TCL", "WRONGARGS", NULL);
    Tcl_SetObjResult(interp, objPtr);
#undef MAY_QUOTE_WORD
#undef AFTER_FIRST_WORD
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseArgsObjv --
 *
 *	Process an objv array according to a table of expected command-line
 *	options. See the manual page for more details.
 *
 * Results:
 *	The return value is a standard Tcl return value. If an error occurs
 *	then an error message is left in the interp's result. Under normal
 *	conditions, both *objcPtr and *objv are modified to return the
 *	arguments that couldn't be processed here (they didn't match the
 *	option table, or followed an TCL_ARGV_REST argument).
 *
 * Side effects:
 *	Variables may be modified, or procedures may be called. It all depends
 *	on the arguments and their entries in argTable. See the user
 *	documentation for details.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseArgsObjv(
    Tcl_Interp *interp,		/* Place to store error message. */
    const Tcl_ArgvInfo *argTable,
				/* Array of option descriptions. */
    int *objcPtr,		/* Number of arguments in objv. Modified to
				 * hold # args left in objv at end. */
    Tcl_Obj *const *objv,	/* Array of arguments to be parsed. */
    Tcl_Obj ***remObjv)		/* Pointer to array of arguments that were not
				 * processed here. Should be NULL if no return
				 * of arguments is desired. */
{
    Tcl_Obj **leftovers;	/* Array to write back to remObjv on
				 * successful exit. Will include the name of
				 * the command. */
    int nrem;			/* Size of leftovers.*/
    register const Tcl_ArgvInfo *infoPtr;
				/* Pointer to the current entry in the table
				 * of argument descriptions. */
    const Tcl_ArgvInfo *matchPtr;
				/* Descriptor that matches current argument */
    Tcl_Obj *curArg;		/* Current argument */
    const char *str = NULL;
    register char c;		/* Second character of current arg (used for
				 * quick check for matching; use 2nd char.
				 * because first char. will almost always be
				 * '-'). */
    int srcIndex;		/* Location from which to read next argument
				 * from objv. */
    int dstIndex;		/* Used to keep track of current arguments
				 * being processed, primarily for error
				 * reporting. */
    int objc;			/* # arguments in objv still to process. */
    int length;			/* Number of characters in current argument */

    if (remObjv != NULL) {
	/*
	 * Then we should copy the name of the command (0th argument). The
	 * upper bound on the number of elements is known, and (undocumented,
	 * but historically true) there should be a NULL argument after the
	 * last result. [Bug 3413857]
	 */

	nrem = 1;
	leftovers = ckalloc((1 + *objcPtr) * sizeof(Tcl_Obj *));
	leftovers[0] = objv[0];
    } else {
	nrem = 0;
	leftovers = NULL;
    }

    /*
     * OK, now start processing from the second element (1st argument).
     */

    srcIndex = dstIndex = 1;
    objc = *objcPtr-1;

    while (objc > 0) {
	curArg = objv[srcIndex];
	srcIndex++;
	objc--;
	str = Tcl_GetStringFromObj(curArg, &length);
	if (length > 0) {
	    c = str[1];
	} else {
	    c = 0;
	}

	/*
	 * Loop throught the argument descriptors searching for one with the
	 * matching key string. If found, leave a pointer to it in matchPtr.
	 */

	matchPtr = NULL;
	infoPtr = argTable;
	for (; infoPtr != NULL && infoPtr->type != TCL_ARGV_END ; infoPtr++) {
	    if (infoPtr->keyStr == NULL) {
		continue;
	    }
	    if ((infoPtr->keyStr[1] != c)
		    || (strncmp(infoPtr->keyStr, str, length) != 0)) {
		continue;
	    }
	    if (infoPtr->keyStr[length] == 0) {
		matchPtr = infoPtr;
		goto gotMatch;
	    }
	    if (matchPtr != NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"ambiguous option \"%s\"", str));
		goto error;
	    }
	    matchPtr = infoPtr;
	}
	if (matchPtr == NULL) {
	    /*
	     * Unrecognized argument. Just copy it down, unless the caller
	     * prefers an error to be registered.
	     */

	    if (remObjv == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"unrecognized argument \"%s\"", str));
		goto error;
	    }

	    dstIndex++;		/* This argument is now handled */
	    leftovers[nrem++] = curArg;
	    continue;
	}

	/*
	 * Take the appropriate action based on the option type
	 */

    gotMatch:
	infoPtr = matchPtr;
	switch (infoPtr->type) {
	case TCL_ARGV_CONSTANT:
	    *((int *) infoPtr->dstPtr) = PTR2INT(infoPtr->srcPtr);
	    break;
	case TCL_ARGV_INT:
	    if (objc == 0) {
		goto missingArg;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[srcIndex],
		    (int *) infoPtr->dstPtr) == TCL_ERROR) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"expected integer argument for \"%s\" but got \"%s\"",
			infoPtr->keyStr, Tcl_GetString(objv[srcIndex])));
		goto error;
	    }
	    srcIndex++;
	    objc--;
	    break;
	case TCL_ARGV_STRING:
	    if (objc == 0) {
		goto missingArg;
	    }
	    *((const char **) infoPtr->dstPtr) =
		    Tcl_GetString(objv[srcIndex]);
	    srcIndex++;
	    objc--;
	    break;
	case TCL_ARGV_REST:
	    /*
	     * Only store the point where we got to if it's not to be written
	     * to NULL, so that TCL_ARGV_AUTO_REST works.
	     */

	    if (infoPtr->dstPtr != NULL) {
		*((int *) infoPtr->dstPtr) = dstIndex;
	    }
	    goto argsDone;
	case TCL_ARGV_FLOAT:
	    if (objc == 0) {
		goto missingArg;
	    }
	    if (Tcl_GetDoubleFromObj(interp, objv[srcIndex],
		    (double *) infoPtr->dstPtr) == TCL_ERROR) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"expected floating-point argument for \"%s\" but got \"%s\"",
			infoPtr->keyStr, Tcl_GetString(objv[srcIndex])));
		goto error;
	    }
	    srcIndex++;
	    objc--;
	    break;
	case TCL_ARGV_FUNC: {
	    Tcl_ArgvFuncProc *handlerProc = (Tcl_ArgvFuncProc *)
		    infoPtr->srcPtr;
	    Tcl_Obj *argObj;

	    if (objc == 0) {
		argObj = NULL;
	    } else {
		argObj = objv[srcIndex];
	    }
	    if (handlerProc(infoPtr->clientData, argObj, infoPtr->dstPtr)) {
		srcIndex++;
		objc--;
	    }
	    break;
	}
	case TCL_ARGV_GENFUNC: {
	    Tcl_ArgvGenFuncProc *handlerProc = (Tcl_ArgvGenFuncProc *)
		    infoPtr->srcPtr;

	    objc = handlerProc(infoPtr->clientData, interp, objc,
		    &objv[srcIndex], infoPtr->dstPtr);
	    if (objc < 0) {
		goto error;
	    }
	    break;
	}
	case TCL_ARGV_HELP:
	    PrintUsage(interp, argTable);
	    goto error;
	default:
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad argument type %d in Tcl_ArgvInfo", infoPtr->type));
	    goto error;
	}
    }

    /*
     * If we broke out of the loop because of an OPT_REST argument, copy the
     * remaining arguments down. Note that there is always at least one
     * argument left over - the command name - so we always have a result if
     * our caller is willing to receive it. [Bug 3413857]
     */

  argsDone:
    if (remObjv == NULL) {
	/*
	 * Nothing to do.
	 */

	return TCL_OK;
    }

    if (objc > 0) {
	memcpy(leftovers+nrem, objv+srcIndex, objc*sizeof(Tcl_Obj *));
	nrem += objc;
    }
    leftovers[nrem] = NULL;
    *objcPtr = nrem++;
    *remObjv = ckrealloc(leftovers, nrem * sizeof(Tcl_Obj *));
    return TCL_OK;

    /*
     * Make sure to handle freeing any temporary space we've allocated on the
     * way to an error.
     */

  missingArg:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "\"%s\" option requires an additional argument", str));
  error:
    if (leftovers != NULL) {
	ckfree(leftovers);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintUsage --
 *
 *	Generate a help string describing command-line options.
 *
 * Results:
 *	The interp's result will be modified to hold a help string describing
 *	all the options in argTable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PrintUsage(
    Tcl_Interp *interp,		/* Place information in this interp's result
				 * area. */
    const Tcl_ArgvInfo *argTable)
				/* Array of command-specific argument
				 * descriptions. */
{
    register const Tcl_ArgvInfo *infoPtr;
    int width, numSpaces;
#define NUM_SPACES 20
    static const char spaces[] = "                    ";
    char tmp[TCL_DOUBLE_SPACE];
    Tcl_Obj *msg;

    /*
     * First, compute the width of the widest option key, so that we can make
     * everything line up.
     */

    width = 4;
    for (infoPtr = argTable; infoPtr->type != TCL_ARGV_END; infoPtr++) {
	int length;

	if (infoPtr->keyStr == NULL) {
	    continue;
	}
	length = strlen(infoPtr->keyStr);
	if (length > width) {
	    width = length;
	}
    }

    /*
     * Now add the option information, with pretty-printing.
     */

    msg = Tcl_NewStringObj("Command-specific options:", -1);
    for (infoPtr = argTable; infoPtr->type != TCL_ARGV_END; infoPtr++) {
	if ((infoPtr->type == TCL_ARGV_HELP) && (infoPtr->keyStr == NULL)) {
	    Tcl_AppendPrintfToObj(msg, "\n%s", infoPtr->helpStr);
	    continue;
	}
	Tcl_AppendPrintfToObj(msg, "\n %s:", infoPtr->keyStr);
	numSpaces = width + 1 - strlen(infoPtr->keyStr);
	while (numSpaces > 0) {
	    if (numSpaces >= NUM_SPACES) {
		Tcl_AppendToObj(msg, spaces, NUM_SPACES);
	    } else {
		Tcl_AppendToObj(msg, spaces, numSpaces);
	    }
	    numSpaces -= NUM_SPACES;
	}
	Tcl_AppendToObj(msg, infoPtr->helpStr, -1);
	switch (infoPtr->type) {
	case TCL_ARGV_INT:
	    Tcl_AppendPrintfToObj(msg, "\n\t\tDefault value: %d",
		    *((int *) infoPtr->dstPtr));
	    break;
	case TCL_ARGV_FLOAT:
	    Tcl_AppendPrintfToObj(msg, "\n\t\tDefault value: %g",
		    *((double *) infoPtr->dstPtr));
	    sprintf(tmp, "%g", *((double *) infoPtr->dstPtr));
	    break;
	case TCL_ARGV_STRING: {
	    char *string = *((char **) infoPtr->dstPtr);

	    if (string != NULL) {
		Tcl_AppendPrintfToObj(msg, "\n\t\tDefault value: \"%s\"",
			string);
	    }
	    break;
	}
	default:
	    break;
	}
    }
    Tcl_SetObjResult(interp, msg);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetCompletionCodeFromObj --
 *
 *	Parses Completion code Code
 *
 * Results:
 *	Returns TCL_ERROR if the value is an invalid completion code.
 *	Otherwise, returns TCL_OK, and writes the completion code to the
 *	pointer provided.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclGetCompletionCodeFromObj(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *value,
    int *codePtr)		/* Argument objects. */
{
    static const char *const returnCodes[] = {
	"ok", "error", "return", "break", "continue", NULL
    };

    if ((value->typePtr != &indexType)
	    && TclGetIntFromObj(NULL, value, codePtr) == TCL_OK) {
	return TCL_OK;
    }
    if (Tcl_GetIndexFromObj(NULL, value, returnCodes, NULL, TCL_EXACT,
	    codePtr) == TCL_OK) {
	return TCL_OK;
    }

    /*
     * Value is not a legal completion code.
     */

    if (interp != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad completion code \"%s\": must be"
		" ok, error, return, break, continue, or an integer",
		TclGetString(value)));
	Tcl_SetErrorCode(interp, "TCL", "RESULT", "ILLEGAL_CODE", NULL);
    }
    return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
