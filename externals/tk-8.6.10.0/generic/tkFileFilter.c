/*
 * tkFileFilter.c --
 *
 *	Process the -filetypes option for the file dialogs on Windows and the
 *	Mac.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkFileFilter.h"

static int		AddClause(Tcl_Interp *interp, FileFilter *filterPtr,
			    Tcl_Obj *patternsObj, Tcl_Obj *ostypesObj,
			    int isWindows);
static FileFilter *	GetFilter(FileFilterList *flistPtr, const char *name);

/*
 *----------------------------------------------------------------------
 *
 * TkInitFileFilters --
 *
 *	Initializes a FileFilterList data structure. A FileFilterList must be
 *	initialized EXACTLY ONCE before any calls to TkGetFileFilters() is
 *	made. The usual flow of control is:
 *		TkInitFileFilters(&flist);
 *		    TkGetFileFilters(&flist, ...);
 *		    TkGetFileFilters(&flist, ...);
 *		    ...
 *		TkFreeFileFilters(&flist);
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields in flistPtr are initialized.
 *
 *----------------------------------------------------------------------
 */

void
TkInitFileFilters(
    FileFilterList *flistPtr)	/* The structure to be initialized. */
{
    flistPtr->filters = NULL;
    flistPtr->filtersTail = NULL;
    flistPtr->numFilters = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetFileFilters --
 *
 *	This function is called by the Mac and Windows implementation of
 *	tk_getOpenFile and tk_getSaveFile to translate the string value of the
 *	-filetypes option into an easy-to-parse C structure (flistPtr). The
 *	caller of this function will then use flistPtr to perform filetype
 *	matching in a platform specific way.
 *
 *	flistPtr must be initialized (See comments in TkInitFileFilters).
 *
 * Results:
 *	A standard TCL return value.
 *
 * Side effects:
 *	The fields in flistPtr are changed according to 'types'.
 *
 *----------------------------------------------------------------------
 */

int
TkGetFileFilters(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    FileFilterList *flistPtr,	/* Stores the list of file filters. */
    Tcl_Obj *types,		/* Value of the -filetypes option. */
    int isWindows)		/* True if we are running on Windows. */
{
    int listObjc;
    Tcl_Obj ** listObjv = NULL;
    int i;

    if (types == NULL) {
	return TCL_OK;
    }

    if (Tcl_ListObjGetElements(interp, types, &listObjc,
	    &listObjv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (listObjc == 0) {
	return TCL_OK;
    }

    /*
     * Free the filter information that have been allocated the previous time;
     * the -filefilters option may have been used more than once in the
     * command line.
     */

    TkFreeFileFilters(flistPtr);

    for (i = 0; i<listObjc; i++) {
	/*
	 * Each file type should have two or three elements: the first one is
	 * the name of the type and the second is the filter of the type. The
	 * third is the Mac OSType ID, but we don't care about them here.
	 */

	int count;
	FileFilter *filterPtr;
	Tcl_Obj **typeInfo;

	if (Tcl_ListObjGetElements(interp, listObjv[i], &count,
		&typeInfo) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (count != 2 && count != 3) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad file type \"%s\", should be "
		    "\"typeName {extension ?extensions ...?} "
		    "?{macType ?macTypes ...?}?\"",
		    Tcl_GetString(listObjv[i])));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "FILE_TYPE", NULL);
	    return TCL_ERROR;
	}

	filterPtr = GetFilter(flistPtr, Tcl_GetString(typeInfo[0]));

	if (AddClause(interp, filterPtr, typeInfo[1],
		(count==2 ? NULL : typeInfo[2]), isWindows) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFreeFileFilters --
 *
 *	Frees the malloc'ed file filter information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields allocated by TkGetFileFilters() are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkFreeFileFilters(
    FileFilterList *flistPtr)	/* List of file filters to free */
{
    FileFilter *filterPtr;
    FileFilterClause *clausePtr;
    GlobPattern *globPtr;
    MacFileType *mfPtr;
    register void *toFree;	/* A pointer that we are about to free. */

    for (filterPtr = flistPtr->filters; filterPtr != NULL; ) {
	for (clausePtr = filterPtr->clauses; clausePtr != NULL; ) {
	    /*
	     * Squelch each of the glob patterns.
	     */

	    for (globPtr = clausePtr->patterns; globPtr != NULL; ) {
		ckfree(globPtr->pattern);
		toFree = globPtr;
		globPtr = globPtr->next;
		ckfree(toFree);
	    }

	    /*
	     * Squelch each of the Mac file type codes.
	     */

	    for (mfPtr = clausePtr->macTypes; mfPtr != NULL; ) {
		toFree = mfPtr;
		mfPtr = mfPtr->next;
		ckfree(toFree);
	    }
	    toFree = clausePtr;
	    clausePtr = clausePtr->next;
	    ckfree(toFree);
	}

	/*
	 * Squelch the name of the filter and the overall structure.
	 */

	ckfree(filterPtr->name);
	toFree = filterPtr;
	filterPtr = filterPtr->next;
	ckfree(toFree);
    }
    flistPtr->filters = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * AddClause --
 *
 *	Add one FileFilterClause to filterPtr.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	The list of filter clauses are updated in filterPtr.
 *
 *----------------------------------------------------------------------
 */

static int
AddClause(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    FileFilter *filterPtr,	/* Stores the new filter clause */
    Tcl_Obj *patternsObj,	/* A Tcl list of glob patterns. */
    Tcl_Obj *ostypesObj,	/* A Tcl list of Mac OSType strings. */
    int isWindows)		/* True if we are running on Windows; False if
				 * we are running on the Mac; Glob patterns
				 * need to be processed differently on these
				 * two platforms */
{
    Tcl_Obj **globList = NULL, **ostypeList = NULL;
    int globCount, ostypeCount, i, code = TCL_OK;
    FileFilterClause *clausePtr;
    Tcl_Encoding macRoman = NULL;

    if (Tcl_ListObjGetElements(interp, patternsObj,
	    &globCount, &globList) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }
    if (ostypesObj != NULL) {
	if (Tcl_ListObjGetElements(interp, ostypesObj,
		&ostypeCount, &ostypeList) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}

	/*
	 * We probably need this encoding now...
	 */

	macRoman = Tcl_GetEncoding(NULL, "macRoman");

	/*
	 * Might be cleaner to use 'Tcl_GetOSTypeFromObj' but that is actually
	 * static to the MacOS X/Darwin version of Tcl, and would therefore
	 * require further code refactoring.
	 */

	for (i=0; i<ostypeCount; i++) {
	    int len;
	    const char *strType = Tcl_GetStringFromObj(ostypeList[i], &len);

	    /*
	     * If len is < 4, it is definitely an error. If equal or longer,
	     * we need to use the macRoman encoding to determine the correct
	     * length (assuming there may be non-ascii characters, e.g.,
	     * embedded nulls or accented characters in the string, the
	     * macRoman length will be different).
	     *
	     * If we couldn't load the encoding, then we can't actually check
	     * the correct length. But here we assume we're probably operating
	     * on unix/windows with a minimal set of encodings and so don't
	     * care about MacOS types. So we won't signal an error.
	     */

	    if (len >= 4 && macRoman != NULL) {
		Tcl_DString osTypeDS;

		/*
		 * Convert utf to macRoman, since MacOS types are defined to
		 * be 4 macRoman characters long
		 */

		Tcl_UtfToExternalDString(macRoman, strType, len, &osTypeDS);
		len = Tcl_DStringLength(&osTypeDS);
		Tcl_DStringFree(&osTypeDS);
	    }
	    if (len != 4) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"bad Macintosh file type \"%s\"",
			Tcl_GetString(ostypeList[i])));
		Tcl_SetErrorCode(interp, "TK", "VALUE", "MAC_TYPE", NULL);
		code = TCL_ERROR;
		goto done;
	    }
	}
    }

    /*
     * Add the clause into the list of clauses
     */

    clausePtr = ckalloc(sizeof(FileFilterClause));
    clausePtr->patterns = NULL;
    clausePtr->patternsTail = NULL;
    clausePtr->macTypes = NULL;
    clausePtr->macTypesTail = NULL;

    if (filterPtr->clauses == NULL) {
	filterPtr->clauses = filterPtr->clausesTail = clausePtr;
    } else {
	filterPtr->clausesTail->next = clausePtr;
	filterPtr->clausesTail = clausePtr;
    }
    clausePtr->next = NULL;

    if (globCount > 0 && globList != NULL) {
	for (i=0; i<globCount; i++) {
	    GlobPattern *globPtr = ckalloc(sizeof(GlobPattern));
	    int len;
	    const char *str = Tcl_GetStringFromObj(globList[i], &len);

	    len = (len + 1) * sizeof(char);
	    if (str[0] && str[0] != '*') {
		/*
		 * Prepend a "*" to patterns that do not have a leading "*"
		 */

		globPtr->pattern = ckalloc(len + 1);
		globPtr->pattern[0] = '*';
		strcpy(globPtr->pattern+1, str);
	    } else if (isWindows) {
		if (strcmp(str, "*") == 0) {
		    globPtr->pattern = ckalloc(4);
		    strcpy(globPtr->pattern, "*.*");
		} else if (strcmp(str, "") == 0) {
		    /*
		     * An empty string means "match all files with no
		     * extensions"
		     * TODO: "*." actually matches with all files on Win95
		     */

		    globPtr->pattern = ckalloc(3);
		    strcpy(globPtr->pattern, "*.");
		} else {
		    globPtr->pattern = ckalloc(len);
		    strcpy(globPtr->pattern, str);
		}
	    } else {
		globPtr->pattern = ckalloc(len);
		strcpy(globPtr->pattern, str);
	    }

	    /*
	     * Add the glob pattern into the list of patterns.
	     */

	    if (clausePtr->patterns == NULL) {
		clausePtr->patterns = clausePtr->patternsTail = globPtr;
	    } else {
		clausePtr->patternsTail->next = globPtr;
		clausePtr->patternsTail = globPtr;
	    }
	    globPtr->next = NULL;
	}
    }
    if (ostypeList != NULL && ostypeCount > 0) {
	if (macRoman == NULL) {
	    macRoman = Tcl_GetEncoding(NULL, "macRoman");
	}
	for (i=0; i<ostypeCount; i++) {
	    Tcl_DString osTypeDS;
	    int len;
	    MacFileType *mfPtr = ckalloc(sizeof(MacFileType));
	    const char *strType = Tcl_GetStringFromObj(ostypeList[i], &len);
	    char *string;

	    /*
	     * Convert utf to macRoman, since MacOS types are defined to be 4
	     * macRoman characters long
	     */

	    Tcl_UtfToExternalDString(macRoman, strType, len, &osTypeDS);
	    string = Tcl_DStringValue(&osTypeDS);
	    mfPtr->type = (OSType) string[0] << 24 | (OSType) string[1] << 16 |
		    (OSType) string[2] <<  8 | (OSType) string[3];
	    Tcl_DStringFree(&osTypeDS);

	    /*
	     * Add the Mac type pattern into the list of Mac types
	     */

	    if (clausePtr->macTypes == NULL) {
		clausePtr->macTypes = clausePtr->macTypesTail = mfPtr;
	    } else {
		clausePtr->macTypesTail->next = mfPtr;
		clausePtr->macTypesTail = mfPtr;
	    }
	    mfPtr->next = NULL;
	}
    }

  done:
    if (macRoman != NULL) {
	Tcl_FreeEncoding(macRoman);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * GetFilter --
 *
 *	Add one FileFilter to flistPtr.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	The list of filters are updated in flistPtr.
 *
 *----------------------------------------------------------------------
 */

static FileFilter *
GetFilter(
    FileFilterList *flistPtr,	/* The FileFilterList that contains the newly
				 * created filter */
    const char *name)		/* Name of the filter. It is usually displayed
				 * in the "File Types" listbox in the file
				 * dialogs. */
{
    FileFilter *filterPtr = flistPtr->filters;
    size_t len;

    for (; filterPtr; filterPtr=filterPtr->next) {
	if (strcmp(filterPtr->name, name) == 0) {
	    return filterPtr;
	}
    }

    filterPtr = ckalloc(sizeof(FileFilter));
    filterPtr->clauses = NULL;
    filterPtr->clausesTail = NULL;
    len = strlen(name) + 1;
    filterPtr->name = ckalloc(len);
    memcpy(filterPtr->name, name, len);

    if (flistPtr->filters == NULL) {
	flistPtr->filters = flistPtr->filtersTail = filterPtr;
    } else {
	flistPtr->filtersTail->next = filterPtr;
	flistPtr->filtersTail = filterPtr;
    }
    filterPtr->next = NULL;

    ++flistPtr->numFilters;
    return filterPtr;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
