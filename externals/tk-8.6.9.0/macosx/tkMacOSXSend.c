/*
 * tkMacOSXSend.c --
 *
 *	This file provides procedures that implement the "send" command,
 *	allowing commands to be passed from interpreter to interpreter. This
 *	current implementation for the Mac has most functionality stubed out.
 *
 *	The current plan, which we have not had time to implement, is for the
 *	first Wish app to create a gestalt of type 'WIsH'. This gestalt will
 *	point to a table, in system memory, of Tk apps. Each Tk app, when it
 *	starts up, will register their name, and process ID, in this table.
 *	This will allow us to implement "tk appname".
 *
 *	Then the send command will look up the process id of the target app in
 *	this table, and send an AppleEvent to that process. The AppleEvent
 *	handler is much like the do script handler, except that you have to
 *	specify the name of the tk app as well, since there may be many
 *	interps in one wish app, and you need to send it to the right one.
 *
 *	Implementing this has been on our list of things to do, but what with
 *	the demise of Tcl at Sun, and the lack of resources at Scriptics it
 *	may not get done for awhile. So this sketch is offered for the brave
 *	to attempt if they need the functionality...
 *
 * Copyright (c) 1989-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXInt.h"

/*
 * The following structure is used to keep track of the interpreters
 * registered by this process.
 */

typedef struct RegisteredInterp {
    char *name;			/* Interpreter's name (malloc-ed). */
    Tcl_Interp *interp;		/* Interpreter associated with name. */
    struct RegisteredInterp *nextPtr;
				/* Next in list of names associated with
				 * interps in this process. NULL means end of
				 * list. */
} RegisteredInterp;

/*
 * A registry of all interpreters for a display is kept in a property
 * "InterpRegistry" on the root window of the display. It is organized as a
 * series of zero or more concatenated strings (in no particular order), each
 * of the form
 *	window space name '\0'
 * where "window" is the hex id of the comm. window to use to talk to an
 * interpreter named "name".
 *
 * When the registry is being manipulated by an application (e.g. to add or
 * remove an entry), it is loaded into memory using a structure of the
 * following type:
 */

typedef struct NameRegistry {
    TkDisplay *dispPtr;		/* Display from which the registry was
				 * read. */
    int locked;			/* Non-zero means that the display was locked
				 * when the property was read in. */
    int modified;		/* Non-zero means that the property has been
				 * modified, so it needs to be written out
				 * when the NameRegistry is closed. */
    unsigned long propLength;	/* Length of the property, in bytes. */
    char *property;		/* The contents of the property, or NULL if
				 * none. See format description above; this is
				 * *not* terminated by the first null
				 * character. Dynamically allocated. */
    int allocedByX;		/* Non-zero means must free property with
				 * XFree; zero means use ckfree. */
} NameRegistry;

static int initialized = 0; /* A flag to denote if we have initialized
				 * yet. */

static RegisteredInterp *interpListPtr = NULL;
				/* List of all interpreters registered by this
				 * process. */

/*
 * The information below is used for communication between processes during
 * "send" commands. Each process keeps a private window, never even mapped,
 * with one property, "Comm". When a command is sent to an interpreter, the
 * command is appended to the comm property of the communication window
 * associated with the interp's process. Similarly, when a result is returned
 * from a sent command, it is also appended to the comm property.
 *
 * Each command and each result takes the form of ASCII text. For a command,
 * the text consists of a zero character followed by several null-terminated
 * ASCII strings. The first string consists of the single letter "c".
 * Subsequent strings have the form "option value" where the following options
 * are supported:
 *
 * -r commWindow serial
 *
 *	This option means that a response should be sent to the window whose X
 *	identifier is "commWindow" (in hex), and the response should be
 *	identified with the serial number given by "serial" (in decimal). If
 *	this option isn't specified then the send is asynchronous and no
 *	response is sent.
 *
 * -n name
 *
 *	"Name" gives the name of the application for which the command is
 *	intended. This option must be present.
 *
 * -s script
 *
 *	"Script" is the script to be executed. This option must be present.
 *
 * The options may appear in any order. The -n and -s options must be present,
 * but -r may be omitted for asynchronous RPCs. For compatibility with future
 * releases that may add new features, there may be additional options
 * present; as long as they start with a "-" character, they will be ignored.
 *
 *
 * A result also consists of a zero character followed by several null-
 * terminated ASCII strings. The first string consists of the single letter
 * "r". Subsequent strings have the form "option value" where the following
 * options are supported:
 *
 * -s serial
 *
 *	Identifies the command for which this is the result. It is the same as
 *	the "serial" field from the -s option in the command. This option must
 *	be present.
 *
 * -c code
 *
 *	"Code" is the completion code for the script, in decimal. If the code
 *	is omitted it defaults to TCL_OK.
 *
 * -r result
 *
 *	"Result" is the result string for the script, which may be either a
 *	result or an error message. If this field is omitted then it defaults
 *	to an empty string.
 *
 * -i errorInfo
 *
 *	"ErrorInfo" gives a string with which to initialize the errorInfo
 *	variable. This option may be omitted; it is ignored unless the
 *	completion code is TCL_ERROR.
 *
 * -e errorCode
 *
 *	"ErrorCode" gives a string with with to initialize the errorCode
 *	variable. This option may be omitted; it is ignored unless the
 *	completion code is TCL_ERROR.
 *
 * Options may appear in any order, and only the -s option must be present. As
 * with commands, there may be additional options besides these; unknown
 * options are ignored.
 */

/*
 * Maximum size property that can be read at one time by this module:
 */

#define MAX_PROP_WORDS 100000

/*
 * Forward declarations for procedures defined later in this file:
 */

static int SendInit(Tcl_Interp *interp);

/*
 *--------------------------------------------------------------
 *
 * Tk_SetAppName --
 *
 *	This procedure is called to associate an ASCII name with a Tk
 *	application. If the application has already been named, the name
 *	replaces the old one.
 *
 * Results:
 *	The return value is the name actually given to the application. This
 *	will normally be the same as name, but if name was already in use for
 *	an application then a name of the form "name #2" will be chosen, with
 *	a high enough number to make the name unique.
 *
 * Side effects:
 *	Registration info is saved, thereby allowing the "send" command to be
 *	used later to invoke commands in the application. In addition, the
 *	"send" command is created in the application's interpreter. The
 *	registration will be removed automatically if the interpreter is
 *	deleted or the "send" command is removed.
 *
 *--------------------------------------------------------------
 */

const char *
Tk_SetAppName(
    Tk_Window tkwin,		/* Token for any window in the application to
				 * be named: it is just used to identify the
				 * application and the display. */
    const char *name)		/* The name that will be used to refer to the
				 * interpreter in later "send" commands. Must
				 * be globally unique. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    Tcl_Interp *interp = winPtr->mainPtr->interp;
    int i, suffix, offset, result;
    RegisteredInterp *riPtr, *prevPtr;
    const char *actualName;
    Tcl_DString dString;
    Tcl_Obj *resultObjPtr, *interpNamePtr;
    char *interpName;

    if (!initialized) {
	SendInit(interp);
    }

    /*
     * See if the application is already registered; if so, remove its current
     * name from the registry. The deletion of the command will take care of
     * disposing of this entry.
     */

    for (riPtr = interpListPtr, prevPtr = NULL; riPtr != NULL;
	    prevPtr = riPtr, riPtr = riPtr->nextPtr) {
	if (riPtr->interp == interp) {
	    if (prevPtr == NULL) {
		interpListPtr = interpListPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = riPtr->nextPtr;
	    }
	    break;
	}
    }

    /*
     * Pick a name to use for the application. Use "name" if it's not already
     * in use. Otherwise add a suffix such as " #2", trying larger and larger
     * numbers until we eventually find one that is unique.
     */

    actualName = name;
    suffix = 1;
    offset = 0;
    Tcl_DStringInit(&dString);

    TkGetInterpNames(interp, tkwin);
    resultObjPtr = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(resultObjPtr);
    for (i = 0; ; ) {
	result = Tcl_ListObjIndex(NULL, resultObjPtr, i, &interpNamePtr);
	if (result != TCL_OK || interpNamePtr == NULL) {
	    break;
	}
	interpName = Tcl_GetString(interpNamePtr);
	if (strcmp(actualName, interpName) == 0) {
	    if (suffix == 1) {
		Tcl_DStringAppend(&dString, name, -1);
		Tcl_DStringAppend(&dString, " #", 2);
		offset = Tcl_DStringLength(&dString);
		Tcl_DStringSetLength(&dString, offset + 10);
		actualName = Tcl_DStringValue(&dString);
	    }
	    suffix++;
	    sprintf(Tcl_DStringValue(&dString) + offset, "%d", suffix);
	    i = 0;
	} else {
	    i++;
	}
    }

    Tcl_DecrRefCount(resultObjPtr);
    Tcl_ResetResult(interp);

    /*
     * We have found a unique name. Now add it to the registry.
     */

    riPtr = ckalloc(sizeof(RegisteredInterp));
    riPtr->interp = interp;
    riPtr->name = ckalloc(strlen(actualName) + 1);
    riPtr->nextPtr = interpListPtr;
    interpListPtr = riPtr;
    strcpy(riPtr->name, actualName);

    /*
     * TODO: DeleteProc
     */

    Tcl_CreateObjCommand(interp, "send", Tk_SendObjCmd, riPtr, NULL);
    if (Tcl_IsSafe(interp)) {
	Tcl_HideCommand(interp, "send", "send");
    }
    Tcl_DStringFree(&dString);

    return riPtr->name;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_SendObjCmd --
 *
 *	This procedure is invoked to process the "send" Tcl command. See the
 *	user documentation for details on what it does.
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
Tk_SendObjCmd(
    ClientData clientData,	/* Used only for deletion */
    Tcl_Interp *interp,		/* The interp we are sending from */
    int objc,			/* Number of arguments */
    Tcl_Obj *const objv[])	/* The arguments */
{
    const char *const sendOptions[] = {"-async", "-displayof", "-", NULL};
    char *stringRep, *destName;
    /*int async = 0;*/
    int i, index, firstArg;
    RegisteredInterp *riPtr;
    Tcl_Obj *listObjPtr;
    int result = TCL_OK;

    for (i = 1; i < (objc - 1); ) {
	stringRep = Tcl_GetString(objv[i]);
	if (stringRep[0] == '-') {
	    if (Tcl_GetIndexFromObjStruct(interp, objv[i], sendOptions,
		    sizeof(char *), "option", 0, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (index == 0) {
		/*async = 1;*/
		i++;
	    } else if (index == 1) {
		i += 2;
	    } else {
		i++;
	    }
	} else {
	    break;
	}
    }

    if (objc < (i + 2)) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-option value ...? interpName arg ?arg ...?");
	return TCL_ERROR;
    }

    destName = Tcl_GetString(objv[i]);
    firstArg = i + 1;

    /*
     * See if the target interpreter is local. If so, execute the command
     * directly without going through the DDE server. The only tricky thing is
     * passing the result from the target interpreter to the invoking
     * interpreter. Watch out: they could be the same!
     */

    for (riPtr = interpListPtr; (riPtr != NULL)
	    && (strcmp(destName, riPtr->name)); riPtr = riPtr->nextPtr) {
	/*
	 * Empty loop body.
	 */
    }

    if (riPtr != NULL) {
	/*
	 * This command is to a local interp. No need to go through the
	 * server.
	 */

	Tcl_Interp *localInterp;

	Tcl_Preserve(riPtr);
	localInterp = riPtr->interp;
	Tcl_Preserve(localInterp);
	if (firstArg == (objc - 1)) {
	    /*
	     * This might be one of those cases where the new parser is
	     * faster.
	     */

	    result = Tcl_EvalObjEx(localInterp, objv[firstArg],
		    TCL_EVAL_DIRECT);
	} else {
	    listObjPtr = Tcl_NewListObj(0, NULL);
	    for (i = firstArg; i < objc; i++) {
		Tcl_ListObjAppendList(interp, listObjPtr, objv[i]);
	    }
	    Tcl_IncrRefCount(listObjPtr);
	    result = Tcl_EvalObjEx(localInterp, listObjPtr, TCL_EVAL_DIRECT);
	    Tcl_DecrRefCount(listObjPtr);
	}
	if (interp != localInterp) {
	    if (result == TCL_ERROR) {
		/* Tcl_Obj *errorObjPtr; */

		/*
		 * An error occurred, so transfer error information from the
		 * destination interpreter back to our interpreter. Must clear
		 * interp's result before calling Tcl_AddErrorInfo, since
		 * Tcl_AddErrorInfo will store the interp's result in
		 * errorInfo before appending riPtr's $errorInfo; we've
		 * already got everything we need in riPtr's $errorInfo.
		 */

		Tcl_ResetResult(interp);
		Tcl_AddErrorInfo(interp, Tcl_GetVar2(localInterp,
			"errorInfo", NULL, TCL_GLOBAL_ONLY));
		/* errorObjPtr = Tcl_GetObjVar2(localInterp, "errorCode", NULL,
			TCL_GLOBAL_ONLY);
		Tcl_SetObjErrorCode(interp, errorObjPtr); */
	    }
	    Tcl_SetObjResult(interp, Tcl_GetObjResult(localInterp));
	}
	Tcl_Release(riPtr);
	Tcl_Release(localInterp);
    } else {
	/*
	 * TODO: This is a non-local request. Send the script to the server
	 * and poll it for a result.
	 */
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetInterpNames --
 *
 *	This procedure is invoked to fetch a list of all the interpreter names
 *	currently registered for the display of a particular window.
 *
 * Results:
 *	A standard Tcl return value. Interp->result will be set to hold a list
 *	of all the interpreter names defined for tkwin's display. If an error
 *	occurs, then TCL_ERROR is returned and interp->result will hold an
 *	error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkGetInterpNames(
    Tcl_Interp *interp,		/* Interpreter for returning a result. */
    Tk_Window tkwin)		/* Window whose display is to be used for the
				 * lookup. */
{
    Tcl_Obj *listObjPtr;
    RegisteredInterp *riPtr;

    listObjPtr = Tcl_NewListObj(0, NULL);
    riPtr = interpListPtr;
    while (riPtr != NULL) {
	Tcl_ListObjAppendElement(interp, listObjPtr,
		Tcl_NewStringObj(riPtr->name, -1));
	riPtr = riPtr->nextPtr;
    }

    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * SendInit --
 *
 *	This procedure is called to initialize the communication channels for
 *	sending commands and receiving results.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up various data structures and windows.
 *
 *--------------------------------------------------------------
 */

static int
SendInit(
    Tcl_Interp *interp)		/* Interpreter to use for error reporting (no
				 * errors are ever returned, but the
				 * interpreter is needed anyway). */
{
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
