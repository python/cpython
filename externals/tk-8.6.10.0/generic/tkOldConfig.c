/*
 * tkOldConfig.c --
 *
 *	This file contains the Tk_ConfigureWidget function. THIS FILE IS HERE
 *	FOR BACKWARD COMPATIBILITY; THE NEW CONFIGURATION PACKAGE SHOULD BE
 *	USED FOR NEW PROJECTS.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * Values for "flags" field of Tk_ConfigSpec structures. Be sure to coordinate
 * these values with those defined in tk.h (TK_CONFIG_COLOR_ONLY, etc.) There
 * must not be overlap!
 *
 * INIT -		Non-zero means (char *) things have been converted to
 *			Tk_Uid's.
 */

#define INIT		0x20

/*
 * Forward declarations for functions defined later in this file:
 */

static int		DoConfig(Tcl_Interp *interp, Tk_Window tkwin,
			    Tk_ConfigSpec *specPtr, Tk_Uid value,
			    int valueIsUid, char *widgRec);
static Tk_ConfigSpec *	FindConfigSpec(Tcl_Interp *interp,
			    Tk_ConfigSpec *specs, const char *argvName,
			    int needFlags, int hateFlags);
static char *		FormatConfigInfo(Tcl_Interp *interp, Tk_Window tkwin,
			    const Tk_ConfigSpec *specPtr, char *widgRec);
static const char *	FormatConfigValue(Tcl_Interp *interp, Tk_Window tkwin,
			    const Tk_ConfigSpec *specPtr, char *widgRec,
			    char *buffer, Tcl_FreeProc **freeProcPtr);
static Tk_ConfigSpec *	GetCachedSpecs(Tcl_Interp *interp,
			    const Tk_ConfigSpec *staticSpecs);
static void		DeleteSpecCacheTable(ClientData clientData,
			    Tcl_Interp *interp);

/*
 *--------------------------------------------------------------
 *
 * Tk_ConfigureWidget --
 *
 *	Process command-line options and database options to fill in fields of
 *	a widget record with resources and other parameters.
 *
 * Results:
 *	A standard Tcl return value. In case of an error, the interp's result
 *	will hold an error message.
 *
 * Side effects:
 *	The fields of widgRec get filled in with information from argc/argv
 *	and the option database. Old information in widgRec's fields gets
 *	recycled. A copy of the spec-table is taken with (some of) the char*
 *	fields converted into Tk_Uid fields; this copy will be released when
 *	the interpreter terminates.
 *
 *--------------------------------------------------------------
 */

int
Tk_ConfigureWidget(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window containing widget (needed to set up
				 * X resources). */
    const Tk_ConfigSpec *specs,	/* Describes legal options. */
    int argc,			/* Number of elements in argv. */
    const char **argv,		/* Command-line options. */
    char *widgRec,		/* Record whose fields are to be modified.
				 * Values must be properly initialized. */
    int flags)			/* Used to specify additional flags that must
				 * be present in config specs for them to be
				 * considered. Also, may have
				 * TK_CONFIG_ARGV_ONLY set. */
{
    register Tk_ConfigSpec *specPtr, *staticSpecs;
    Tk_Uid value;		/* Value of option from database. */
    int needFlags;		/* Specs must contain this set of flags or
				 * else they are not considered. */
    int hateFlags;		/* If a spec contains any bits here, it's not
				 * considered. */

    if (tkwin == NULL) {
	/*
	 * Either we're not really in Tk, or the main window was destroyed and
	 * we're on our way out of the application
	 */

	Tcl_SetObjResult(interp, Tcl_NewStringObj("NULL main window", -1));
	Tcl_SetErrorCode(interp, "TK", "NO_MAIN_WINDOW", NULL);
	return TCL_ERROR;
    }

    needFlags = flags & ~(TK_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
	hateFlags = TK_CONFIG_COLOR_ONLY;
    } else {
	hateFlags = TK_CONFIG_MONO_ONLY;
    }

    /*
     * Get the build of the config for this interpreter.
     */

    staticSpecs = GetCachedSpecs(interp, specs);

    for (specPtr = staticSpecs; specPtr->type != TK_CONFIG_END; specPtr++) {
	specPtr->specFlags &= ~TK_CONFIG_OPTION_SPECIFIED;
    }

    /*
     * Pass one: scan through all of the arguments, processing those that
     * match entries in the specs.
     */

    for ( ; argc > 0; argc -= 2, argv += 2) {
	const char *arg;

	if (flags & TK_CONFIG_OBJS) {
	    arg = Tcl_GetString((Tcl_Obj *) *argv);
	} else {
	    arg = *argv;
	}
	specPtr = FindConfigSpec(interp, staticSpecs, arg, needFlags, hateFlags);
	if (specPtr == NULL) {
	    return TCL_ERROR;
	}

	/*
	 * Process the entry.
	 */

	if (argc < 2) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", arg));
	    Tcl_SetErrorCode(interp, "TK", "VALUE_MISSING", NULL);
	    return TCL_ERROR;
	}
	if (flags & TK_CONFIG_OBJS) {
	    arg = Tcl_GetString((Tcl_Obj *) argv[1]);
	} else {
	    arg = argv[1];
	}
	if (DoConfig(interp, tkwin, specPtr, arg, 0, widgRec) != TCL_OK) {
	    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		    "\n    (processing \"%.40s\" option)",specPtr->argvName));
	    return TCL_ERROR;
	}
	if (!(flags & TK_CONFIG_ARGV_ONLY)) {
	    specPtr->specFlags |= TK_CONFIG_OPTION_SPECIFIED;
	}
    }

    /*
     * Pass two: scan through all of the specs again; if no command-line
     * argument matched a spec, then check for info in the option database.
     * If there was nothing in the database, then use the default.
     */

    if (!(flags & TK_CONFIG_ARGV_ONLY)) {
	for (specPtr = staticSpecs; specPtr->type != TK_CONFIG_END; specPtr++) {
	    if ((specPtr->specFlags & TK_CONFIG_OPTION_SPECIFIED)
		    || (specPtr->argvName == NULL)
		    || (specPtr->type == TK_CONFIG_SYNONYM)) {
		continue;
	    }
	    if (((specPtr->specFlags & needFlags) != needFlags)
		    || (specPtr->specFlags & hateFlags)) {
		continue;
	    }
	    value = NULL;
	    if (specPtr->dbName != NULL) {
		value = Tk_GetOption(tkwin, specPtr->dbName, specPtr->dbClass);
	    }
	    if (value != NULL) {
		if (DoConfig(interp, tkwin, specPtr, value, 1, widgRec) !=
			TCL_OK) {
		    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
			    "\n    (%s \"%.50s\" in widget \"%.50s\")",
			    "database entry for", specPtr->dbName,
			    Tk_PathName(tkwin)));
		    return TCL_ERROR;
		}
	    } else {
		if (specPtr->defValue != NULL) {
		    value = Tk_GetUid(specPtr->defValue);
		} else {
		    value = NULL;
		}
		if ((value != NULL) && !(specPtr->specFlags
			& TK_CONFIG_DONT_SET_DEFAULT)) {
		    if (DoConfig(interp, tkwin, specPtr, value, 1, widgRec) !=
			    TCL_OK) {
			Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
				"\n    (%s \"%.50s\" in widget \"%.50s\")",
				"default value for", specPtr->dbName,
				Tk_PathName(tkwin)));
			return TCL_ERROR;
		    }
		}
	    }
	}
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * FindConfigSpec --
 *
 *	Search through a table of configuration specs, looking for one that
 *	matches a given argvName.
 *
 * Results:
 *	The return value is a pointer to the matching entry, or NULL if
 *	nothing matched. In that case an error message is left in the interp's
 *	result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static Tk_ConfigSpec *
FindConfigSpec(
    Tcl_Interp *interp,		/* Used for reporting errors. */
    Tk_ConfigSpec *specs,	/* Pointer to table of configuration
				 * specifications for a widget. */
    const char *argvName,	/* Name (suitable for use in a "config"
				 * command) identifying particular option. */
    int needFlags,		/* Flags that must be present in matching
				 * entry. */
    int hateFlags)		/* Flags that must NOT be present in matching
				 * entry. */
{
    register Tk_ConfigSpec *specPtr;
    register char c;		/* First character of current argument. */
    Tk_ConfigSpec *matchPtr;	/* Matching spec, or NULL. */
    size_t length;

    c = argvName[1];
    length = strlen(argvName);
    matchPtr = NULL;
    for (specPtr = specs; specPtr->type != TK_CONFIG_END; specPtr++) {
	if (specPtr->argvName == NULL) {
	    continue;
	}
	if ((specPtr->argvName[1] != c)
		|| (strncmp(specPtr->argvName, argvName, length) != 0)) {
	    continue;
	}
	if (((specPtr->specFlags & needFlags) != needFlags)
		|| (specPtr->specFlags & hateFlags)) {
	    continue;
	}
	if (specPtr->argvName[length] == 0) {
	    matchPtr = specPtr;
	    goto gotMatch;
	}
	if (matchPtr != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "ambiguous option \"%s\"", argvName));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "OPTION", argvName,NULL);
	    return NULL;
	}
	matchPtr = specPtr;
    }

    if (matchPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown option \"%s\"", argvName));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "OPTION", argvName, NULL);
	return NULL;
    }

    /*
     * Found a matching entry. If it's a synonym, then find the entry that
     * it's a synonym for.
     */

  gotMatch:
    specPtr = matchPtr;
    if (specPtr->type == TK_CONFIG_SYNONYM) {
	for (specPtr = specs; ; specPtr++) {
	    if (specPtr->type == TK_CONFIG_END) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't find synonym for option \"%s\"",
			argvName));
		Tcl_SetErrorCode(interp, "TK", "LOOKUP", "OPTION", argvName,
			NULL);
		return NULL;
	    }
	    if ((specPtr->dbName == matchPtr->dbName)
		    && (specPtr->type != TK_CONFIG_SYNONYM)
		    && ((specPtr->specFlags & needFlags) == needFlags)
		    && !(specPtr->specFlags & hateFlags)) {
		break;
	    }
	}
    }
    return specPtr;
}

/*
 *--------------------------------------------------------------
 *
 * DoConfig --
 *
 *	This function applies a single configuration option to a widget
 *	record.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	WidgRec is modified as indicated by specPtr and value. The old value
 *	is recycled, if that is appropriate for the value type.
 *
 *--------------------------------------------------------------
 */

static int
DoConfig(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window containing widget (needed to set up
				 * X resources). */
    Tk_ConfigSpec *specPtr,	/* Specifier to apply. */
    Tk_Uid value,		/* Value to use to fill in widgRec. */
    int valueIsUid,		/* Non-zero means value is a Tk_Uid; zero
				 * means it's an ordinary string. */
    char *widgRec)		/* Record whose fields are to be modified.
				 * Values must be properly initialized. */
{
    char *ptr;
    Tk_Uid uid;
    int nullValue;

    nullValue = 0;
    if ((*value == 0) && (specPtr->specFlags & TK_CONFIG_NULL_OK)) {
	nullValue = 1;
    }

    do {
	ptr = widgRec + specPtr->offset;
	switch (specPtr->type) {
	case TK_CONFIG_BOOLEAN:
	    if (Tcl_GetBoolean(interp, value, (int *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_INT:
	    if (Tcl_GetInt(interp, value, (int *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_DOUBLE:
	    if (Tcl_GetDouble(interp, value, (double *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_STRING: {
	    char *oldStr, *newStr;

	    if (nullValue) {
		newStr = NULL;
	    } else {
		newStr = ckalloc(strlen(value) + 1);
		strcpy(newStr, value);
	    }
	    oldStr = *((char **) ptr);
	    if (oldStr != NULL) {
		ckfree(oldStr);
	    }
	    *((char **) ptr) = newStr;
	    break;
	}
	case TK_CONFIG_UID:
	    if (nullValue) {
		*((Tk_Uid *) ptr) = NULL;
	    } else {
		uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
		*((Tk_Uid *) ptr) = uid;
	    }
	    break;
	case TK_CONFIG_COLOR: {
	    XColor *newPtr, *oldPtr;

	    if (nullValue) {
		newPtr = NULL;
	    } else {
		uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
		newPtr = Tk_GetColor(interp, tkwin, uid);
		if (newPtr == NULL) {
		    return TCL_ERROR;
		}
	    }
	    oldPtr = *((XColor **) ptr);
	    if (oldPtr != NULL) {
		Tk_FreeColor(oldPtr);
	    }
	    *((XColor **) ptr) = newPtr;
	    break;
	}
	case TK_CONFIG_FONT: {
	    Tk_Font newFont;

	    if (nullValue) {
		newFont = NULL;
	    } else {
		newFont = Tk_GetFont(interp, tkwin, value);
		if (newFont == NULL) {
		    return TCL_ERROR;
		}
	    }
	    Tk_FreeFont(*((Tk_Font *) ptr));
	    *((Tk_Font *) ptr) = newFont;
	    break;
	}
	case TK_CONFIG_BITMAP: {
	    Pixmap newBmp, oldBmp;

	    if (nullValue) {
		newBmp = None;
	    } else {
		uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
		newBmp = Tk_GetBitmap(interp, tkwin, uid);
		if (newBmp == None) {
		    return TCL_ERROR;
		}
	    }
	    oldBmp = *((Pixmap *) ptr);
	    if (oldBmp != None) {
		Tk_FreeBitmap(Tk_Display(tkwin), oldBmp);
	    }
	    *((Pixmap *) ptr) = newBmp;
	    break;
	}
	case TK_CONFIG_BORDER: {
	    Tk_3DBorder newBorder, oldBorder;

	    if (nullValue) {
		newBorder = NULL;
	    } else {
		uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
		newBorder = Tk_Get3DBorder(interp, tkwin, uid);
		if (newBorder == NULL) {
		    return TCL_ERROR;
		}
	    }
	    oldBorder = *((Tk_3DBorder *) ptr);
	    if (oldBorder != NULL) {
		Tk_Free3DBorder(oldBorder);
	    }
	    *((Tk_3DBorder *) ptr) = newBorder;
	    break;
	}
	case TK_CONFIG_RELIEF:
	    uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
	    if (Tk_GetRelief(interp, uid, (int *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_CURSOR:
	case TK_CONFIG_ACTIVE_CURSOR: {
	    Tk_Cursor newCursor, oldCursor;

	    if (nullValue) {
		newCursor = NULL;
	    } else {
		uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
		newCursor = Tk_GetCursor(interp, tkwin, uid);
		if (newCursor == NULL) {
		    return TCL_ERROR;
		}
	    }
	    oldCursor = *((Tk_Cursor *) ptr);
	    if (oldCursor != NULL) {
		Tk_FreeCursor(Tk_Display(tkwin), oldCursor);
	    }
	    *((Tk_Cursor *) ptr) = newCursor;
	    if (specPtr->type == TK_CONFIG_ACTIVE_CURSOR) {
		Tk_DefineCursor(tkwin, newCursor);
	    }
	    break;
	}
	case TK_CONFIG_JUSTIFY:
	    uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
	    if (Tk_GetJustify(interp, uid, (Tk_Justify *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_ANCHOR:
	    uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
	    if (Tk_GetAnchor(interp, uid, (Tk_Anchor *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_CAP_STYLE:
	    uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
	    if (Tk_GetCapStyle(interp, uid, (int *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_JOIN_STYLE:
	    uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
	    if (Tk_GetJoinStyle(interp, uid, (int *) ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_PIXELS:
	    if (Tk_GetPixels(interp, tkwin, value, (int *) ptr)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_MM:
	    if (Tk_GetScreenMM(interp, tkwin, value, (double*)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TK_CONFIG_WINDOW: {
	    Tk_Window tkwin2;

	    if (nullValue) {
		tkwin2 = NULL;
	    } else {
		tkwin2 = Tk_NameToWindow(interp, value, tkwin);
		if (tkwin2 == NULL) {
		    return TCL_ERROR;
		}
	    }
	    *((Tk_Window *) ptr) = tkwin2;
	    break;
	}
	case TK_CONFIG_CUSTOM:
	    if (specPtr->customPtr->parseProc(specPtr->customPtr->clientData,
		    interp, tkwin, value, widgRec, specPtr->offset)!=TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	default:
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad config table: unknown type %d", specPtr->type));
	    Tcl_SetErrorCode(interp, "TK", "BAD_CONFIG", NULL);
	    return TCL_ERROR;
	}
	specPtr++;
    } while ((specPtr->argvName == NULL) && (specPtr->type != TK_CONFIG_END));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_ConfigureInfo --
 *
 *	Return information about the configuration options for a window, and
 *	their current values.
 *
 * Results:
 *	Always returns TCL_OK. The interp's result will be modified hold a
 *	description of either a single configuration option available for
 *	"widgRec" via "specs", or all the configuration options available. In
 *	the "all" case, the result will available for "widgRec" via "specs".
 *	The result will be a list, each of whose entries describes one option.
 *	Each entry will itself be a list containing the option's name for use
 *	on command lines, database name, database class, default value, and
 *	current value (empty string if none). For options that are synonyms,
 *	the list will contain only two values: name and synonym name. If the
 *	"name" argument is non-NULL, then the only information returned is
 *	that for the named argument (i.e. the corresponding entry in the
 *	overall list is returned).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_ConfigureInfo(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window corresponding to widgRec. */
    const Tk_ConfigSpec *specs, /* Describes legal options. */
    char *widgRec,		/* Record whose fields contain current values
				 * for options. */
    const char *argvName,	/* If non-NULL, indicates a single option
				 * whose info is to be returned. Otherwise
				 * info is returned for all options. */
    int flags)			/* Used to specify additional flags that must
				 * be present in config specs for them to be
				 * considered. */
{
    register Tk_ConfigSpec *specPtr, *staticSpecs;
    int needFlags, hateFlags;
    char *list;
    const char *leader = "{";

    needFlags = flags & ~(TK_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
	hateFlags = TK_CONFIG_COLOR_ONLY;
    } else {
	hateFlags = TK_CONFIG_MONO_ONLY;
    }

    /*
     * Get the build of the config for this interpreter.
     */

    staticSpecs = GetCachedSpecs(interp, specs);

    /*
     * If information is only wanted for a single configuration spec, then
     * handle that one spec specially.
     */

    Tcl_ResetResult(interp);
    if (argvName != NULL) {
	specPtr = FindConfigSpec(interp, staticSpecs, argvName, needFlags,
		hateFlags);
	if (specPtr == NULL) {
	    return TCL_ERROR;
	}
	list = FormatConfigInfo(interp, tkwin, specPtr, widgRec);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(list, -1));
	ckfree(list);
	return TCL_OK;
    }

    /*
     * Loop through all the specs, creating a big list with all their
     * information.
     */

    for (specPtr = staticSpecs; specPtr->type != TK_CONFIG_END; specPtr++) {
	if ((argvName != NULL) && (specPtr->argvName != argvName)) {
	    continue;
	}
	if (((specPtr->specFlags & needFlags) != needFlags)
		|| (specPtr->specFlags & hateFlags)) {
	    continue;
	}
	if (specPtr->argvName == NULL) {
	    continue;
	}
	list = FormatConfigInfo(interp, tkwin, specPtr, widgRec);
	Tcl_AppendResult(interp, leader, list, "}", NULL);
	ckfree(list);
	leader = " {";
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * FormatConfigInfo --
 *
 *	Create a valid Tcl list holding the configuration information for a
 *	single configuration option.
 *
 * Results:
 *	A Tcl list, dynamically allocated. The caller is expected to arrange
 *	for this list to be freed eventually.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *--------------------------------------------------------------
 */

static char *
FormatConfigInfo(
    Tcl_Interp *interp,		/* Interpreter to use for things like
				 * floating-point precision. */
    Tk_Window tkwin,		/* Window corresponding to widget. */
    register const Tk_ConfigSpec *specPtr,
				/* Pointer to information describing
				 * option. */
    char *widgRec)		/* Pointer to record holding current values of
				 * info for widget. */
{
    const char *argv[6];
    char *result;
    char buffer[200];
    Tcl_FreeProc *freeProc = NULL;

    argv[0] = specPtr->argvName;
    argv[1] = specPtr->dbName;
    argv[2] = specPtr->dbClass;
    argv[3] = specPtr->defValue;
    if (specPtr->type == TK_CONFIG_SYNONYM) {
	return Tcl_Merge(2, argv);
    }
    argv[4] = FormatConfigValue(interp, tkwin, specPtr, widgRec, buffer,
	    &freeProc);
    if (argv[1] == NULL) {
	argv[1] = "";
    }
    if (argv[2] == NULL) {
	argv[2] = "";
    }
    if (argv[3] == NULL) {
	argv[3] = "";
    }
    if (argv[4] == NULL) {
	argv[4] = "";
    }
    result = Tcl_Merge(5, argv);
    if (freeProc != NULL) {
	if ((freeProc == TCL_DYNAMIC) || (freeProc == (Tcl_FreeProc *) free)) {
	    ckfree((char *) argv[4]);
	} else {
	    freeProc((char *) argv[4]);
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * FormatConfigValue --
 *
 *	This function formats the current value of a configuration option.
 *
 * Results:
 *	The return value is the formatted value of the option given by specPtr
 *	and widgRec. If the value is static, so that it need not be freed,
 *	*freeProcPtr will be set to NULL; otherwise *freeProcPtr will be set
 *	to the address of a function to free the result, and the caller must
 *	invoke this function when it is finished with the result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static const char *
FormatConfigValue(
    Tcl_Interp *interp,		/* Interpreter for use in real conversions. */
    Tk_Window tkwin,		/* Window corresponding to widget. */
    const Tk_ConfigSpec *specPtr, /* Pointer to information describing option.
				 * Must not point to a synonym option. */
    char *widgRec,		/* Pointer to record holding current values of
				 * info for widget. */
    char *buffer,		/* Static buffer to use for small values.
				 * Must have at least 200 bytes of storage. */
    Tcl_FreeProc **freeProcPtr)	/* Pointer to word to fill in with address of
				 * function to free the result, or NULL if
				 * result is static. */
{
    const char *ptr, *result;

    *freeProcPtr = NULL;
    ptr = widgRec + specPtr->offset;
    result = "";
    switch (specPtr->type) {
    case TK_CONFIG_BOOLEAN:
	if (*((int *) ptr) == 0) {
	    result = "0";
	} else {
	    result = "1";
	}
	break;
    case TK_CONFIG_INT:
	sprintf(buffer, "%d", *((int *) ptr));
	result = buffer;
	break;
    case TK_CONFIG_DOUBLE:
	Tcl_PrintDouble(interp, *((double *) ptr), buffer);
	result = buffer;
	break;
    case TK_CONFIG_STRING:
	result = (*(char **) ptr);
	if (result == NULL) {
	    result = "";
	}
	break;
    case TK_CONFIG_UID: {
	Tk_Uid uid = *((Tk_Uid *) ptr);

	if (uid != NULL) {
	    result = uid;
	}
	break;
    }
    case TK_CONFIG_COLOR: {
	XColor *colorPtr = *((XColor **) ptr);

	if (colorPtr != NULL) {
	    result = Tk_NameOfColor(colorPtr);
	}
	break;
    }
    case TK_CONFIG_FONT: {
	Tk_Font tkfont = *((Tk_Font *) ptr);

	if (tkfont != NULL) {
	    result = Tk_NameOfFont(tkfont);
	}
	break;
    }
    case TK_CONFIG_BITMAP: {
	Pixmap pixmap = *((Pixmap *) ptr);

	if (pixmap != None) {
	    result = Tk_NameOfBitmap(Tk_Display(tkwin), pixmap);
	}
	break;
    }
    case TK_CONFIG_BORDER: {
	Tk_3DBorder border = *((Tk_3DBorder *) ptr);

	if (border != NULL) {
	    result = Tk_NameOf3DBorder(border);
	}
	break;
    }
    case TK_CONFIG_RELIEF:
	result = Tk_NameOfRelief(*((int *) ptr));
	break;
    case TK_CONFIG_CURSOR:
    case TK_CONFIG_ACTIVE_CURSOR: {
	Tk_Cursor cursor = *((Tk_Cursor *) ptr);

	if (cursor != NULL) {
	    result = Tk_NameOfCursor(Tk_Display(tkwin), cursor);
	}
	break;
    }
    case TK_CONFIG_JUSTIFY:
	result = Tk_NameOfJustify(*((Tk_Justify *) ptr));
	break;
    case TK_CONFIG_ANCHOR:
	result = Tk_NameOfAnchor(*((Tk_Anchor *) ptr));
	break;
    case TK_CONFIG_CAP_STYLE:
	result = Tk_NameOfCapStyle(*((int *) ptr));
	break;
    case TK_CONFIG_JOIN_STYLE:
	result = Tk_NameOfJoinStyle(*((int *) ptr));
	break;
    case TK_CONFIG_PIXELS:
	sprintf(buffer, "%d", *((int *) ptr));
	result = buffer;
	break;
    case TK_CONFIG_MM:
	Tcl_PrintDouble(interp, *((double *) ptr), buffer);
	result = buffer;
	break;
    case TK_CONFIG_WINDOW: {
	Tk_Window tkwin;

	tkwin = *((Tk_Window *) ptr);
	if (tkwin != NULL) {
	    result = Tk_PathName(tkwin);
	}
	break;
    }
    case TK_CONFIG_CUSTOM:
	result = specPtr->customPtr->printProc(specPtr->customPtr->clientData,
		tkwin, widgRec, specPtr->offset, freeProcPtr);
	break;
    default:
	result = "?? unknown type ??";
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ConfigureValue --
 *
 *	This function returns the current value of a configuration option for
 *	a widget.
 *
 * Results:
 *	The return value is a standard Tcl completion code (TCL_OK or
 *	TCL_ERROR). The interp's result will be set to hold either the value
 *	of the option given by argvName (if TCL_OK is returned) or an error
 *	message (if TCL_ERROR is returned).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ConfigureValue(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window corresponding to widgRec. */
    const Tk_ConfigSpec *specs, /* Describes legal options. */
    char *widgRec,		/* Record whose fields contain current values
				 * for options. */
    const char *argvName,	/* Gives the command-line name for the option
				 * whose value is to be returned. */
    int flags)			/* Used to specify additional flags that must
				 * be present in config specs for them to be
				 * considered. */
{
    Tk_ConfigSpec *specPtr;
    int needFlags, hateFlags;
    Tcl_FreeProc *freeProc;
    const char *result;
    char buffer[200];

    needFlags = flags & ~(TK_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
	hateFlags = TK_CONFIG_COLOR_ONLY;
    } else {
	hateFlags = TK_CONFIG_MONO_ONLY;
    }

    /*
     * Get the build of the config for this interpreter.
     */

    specPtr = GetCachedSpecs(interp, specs);

    specPtr = FindConfigSpec(interp, specPtr, argvName, needFlags, hateFlags);
    if (specPtr == NULL) {
	return TCL_ERROR;
    }
    result = FormatConfigValue(interp, tkwin, specPtr, widgRec, buffer,
	    &freeProc);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result, -1));
    if (freeProc != NULL) {
	if ((freeProc == TCL_DYNAMIC) || (freeProc == (Tcl_FreeProc *) free)) {
	    ckfree((char *) result);
	} else {
	    freeProc((char *) result);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeOptions --
 *
 *	Free up all resources associated with configuration options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any resource in widgRec that is controlled by a configuration option
 *	(e.g. a Tk_3DBorder or XColor) is freed in the appropriate fashion.
 *
 * Notes:
 *	Since this is not looking anything up, this uses the static version of
 *	the config specs.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
void
Tk_FreeOptions(
    const Tk_ConfigSpec *specs,	/* Describes legal options. */
    char *widgRec,		/* Record whose fields contain current values
				 * for options. */
    Display *display,		/* X display; needed for freeing some
				 * resources. */
    int needFlags)		/* Used to specify additional flags that must
				 * be present in config specs for them to be
				 * considered. */
{
    register const Tk_ConfigSpec *specPtr;
    char *ptr;

    for (specPtr = specs; specPtr->type != TK_CONFIG_END; specPtr++) {
	if ((specPtr->specFlags & needFlags) != needFlags) {
	    continue;
	}
	ptr = widgRec + specPtr->offset;
	switch (specPtr->type) {
	case TK_CONFIG_STRING:
	    if (*((char **) ptr) != NULL) {
		ckfree(*((char **) ptr));
		*((char **) ptr) = NULL;
	    }
	    break;
	case TK_CONFIG_COLOR:
	    if (*((XColor **) ptr) != NULL) {
		Tk_FreeColor(*((XColor **) ptr));
		*((XColor **) ptr) = NULL;
	    }
	    break;
	case TK_CONFIG_FONT:
	    Tk_FreeFont(*((Tk_Font *) ptr));
	    *((Tk_Font *) ptr) = NULL;
	    break;
	case TK_CONFIG_BITMAP:
	    if (*((Pixmap *) ptr) != None) {
		Tk_FreeBitmap(display, *((Pixmap *) ptr));
		*((Pixmap *) ptr) = None;
	    }
	    break;
	case TK_CONFIG_BORDER:
	    if (*((Tk_3DBorder *) ptr) != NULL) {
		Tk_Free3DBorder(*((Tk_3DBorder *) ptr));
		*((Tk_3DBorder *) ptr) = NULL;
	    }
	    break;
	case TK_CONFIG_CURSOR:
	case TK_CONFIG_ACTIVE_CURSOR:
	    if (*((Tk_Cursor *) ptr) != NULL) {
		Tk_FreeCursor(display, *((Tk_Cursor *) ptr));
		*((Tk_Cursor *) ptr) = NULL;
	    }
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * GetCachedSpecs --
 *
 *	Returns a writable per-interpreter (and hence thread-local) copy of
 *	the given spec-table with (some of) the char* fields converted into
 *	Tk_Uid fields; this copy will be released when the interpreter
 *	terminates (during AssocData cleanup).
 *
 * Results:
 *	A pointer to the copied table.
 *
 * Notes:
 *	The conversion to Tk_Uid is only done the first time, when the table
 *	copy is taken. After that, the table is assumed to have Tk_Uids where
 *	they are needed. The time of deletion of the caches isn't very
 *	important unless you've got a lot of code that uses Tk_ConfigureWidget
 *	(or *Info or *Value} when the interpreter is being deleted.
 *
 *--------------------------------------------------------------
 */

static Tk_ConfigSpec *
GetCachedSpecs(
    Tcl_Interp *interp,		/* Interpreter in which to store the cache. */
    const Tk_ConfigSpec *staticSpecs)
				/* Value to cache a copy of; it is also used
				 * as a key into the cache. */
{
    Tk_ConfigSpec *cachedSpecs;
    Tcl_HashTable *specCacheTablePtr;
    Tcl_HashEntry *entryPtr;
    int isNew;

    /*
     * Get (or allocate if it doesn't exist) the hash table that the writable
     * copies of the widget specs are stored in. In effect, this is
     * self-initializing code.
     */

    specCacheTablePtr =
	    Tcl_GetAssocData(interp, "tkConfigSpec.threadTable", NULL);
    if (specCacheTablePtr == NULL) {
	specCacheTablePtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(specCacheTablePtr, TCL_ONE_WORD_KEYS);
	Tcl_SetAssocData(interp, "tkConfigSpec.threadTable",
		DeleteSpecCacheTable, specCacheTablePtr);
    }

    /*
     * Look up or create the hash entry that the constant specs are mapped to,
     * which will have the writable specs as its associated value.
     */

    entryPtr = Tcl_CreateHashEntry(specCacheTablePtr, (char *) staticSpecs,
	    &isNew);
    if (isNew) {
	unsigned int entrySpace = sizeof(Tk_ConfigSpec);
	const Tk_ConfigSpec *staticSpecPtr;
	Tk_ConfigSpec *specPtr;

	/*
	 * OK, no working copy in this interpreter so copy. Need to work out
	 * how much space to allocate first.
	 */

	for (staticSpecPtr=staticSpecs; staticSpecPtr->type!=TK_CONFIG_END;
		staticSpecPtr++) {
	    entrySpace += sizeof(Tk_ConfigSpec);
	}

	/*
	 * Now allocate our working copy's space and copy over the contents
	 * from the master copy.
	 */

	cachedSpecs = ckalloc(entrySpace);
	memcpy(cachedSpecs, staticSpecs, entrySpace);
	Tcl_SetHashValue(entryPtr, cachedSpecs);

	/*
	 * Finally, go through and replace database names, database classes
	 * and default values with Tk_Uids. This is the bit that has to be
	 * per-thread.
	 */

	for (specPtr=cachedSpecs; specPtr->type!=TK_CONFIG_END; specPtr++) {
	    if (specPtr->argvName != NULL) {
		if (specPtr->dbName != NULL) {
		    specPtr->dbName = Tk_GetUid(specPtr->dbName);
		}
		if (specPtr->dbClass != NULL) {
		    specPtr->dbClass = Tk_GetUid(specPtr->dbClass);
		}
		if (specPtr->defValue != NULL) {
		    specPtr->defValue = Tk_GetUid(specPtr->defValue);
		}
	    }
	}
    } else {
	cachedSpecs = Tcl_GetHashValue(entryPtr);
    }

    return cachedSpecs;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteSpecCacheTable --
 *
 *	Delete the per-interpreter copy of all the Tk_ConfigSpec tables which
 *	were stored in the interpreter's assoc-data store.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None (does *not* use any Tk API).
 *
 *--------------------------------------------------------------
 */

static void
DeleteSpecCacheTable(
    ClientData clientData,
    Tcl_Interp *interp)
{
    Tcl_HashTable *tablePtr = clientData;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;

    for (entryPtr = Tcl_FirstHashEntry(tablePtr,&search); entryPtr != NULL;
	    entryPtr = Tcl_NextHashEntry(&search)) {
	/*
	 * Someone else deallocates the Tk_Uids themselves.
	 */

	ckfree(Tcl_GetHashValue(entryPtr));
    }
    Tcl_DeleteHashTable(tablePtr);
    ckfree(tablePtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
