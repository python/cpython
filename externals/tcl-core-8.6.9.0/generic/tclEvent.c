/*
 * tclEvent.c --
 *
 *	This file implements some general event related interfaces including
 *	background errors, exit handlers, and the "vwait" and "update" command
 *	functions.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 * Copyright (c) 2004 by Zoran Vasiljevic.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * The data structure below is used to report background errors. One such
 * structure is allocated for each error; it holds information about the
 * interpreter and the error until an idle handler command can be invoked.
 */

typedef struct BgError {
    Tcl_Obj *errorMsg;		/* Copy of the error message (the interp's
				 * result when the error occurred). */
    Tcl_Obj *returnOpts;	/* Active return options when the error
				 * occurred */
    struct BgError *nextPtr;	/* Next in list of all pending error reports
				 * for this interpreter, or NULL for end of
				 * list. */
} BgError;

/*
 * One of the structures below is associated with the "tclBgError" assoc data
 * for each interpreter. It keeps track of the head and tail of the list of
 * pending background errors for the interpreter.
 */

typedef struct ErrAssocData {
    Tcl_Interp *interp;		/* Interpreter in which error occurred. */
    Tcl_Obj *cmdPrefix;		/* First word(s) of the handler command */
    BgError *firstBgPtr;	/* First in list of all background errors
				 * waiting to be processed for this
				 * interpreter (NULL if none). */
    BgError *lastBgPtr;		/* Last in list of all background errors
				 * waiting to be processed for this
				 * interpreter (NULL if none). */
} ErrAssocData;

/*
 * For each exit handler created with a call to Tcl_Create(Late)ExitHandler
 * there is a structure of the following type:
 */

typedef struct ExitHandler {
    Tcl_ExitProc *proc;		/* Function to call when process exits. */
    ClientData clientData;	/* One word of information to pass to proc. */
    struct ExitHandler *nextPtr;/* Next in list of all exit handlers for this
				 * application, or NULL for end of list. */
} ExitHandler;

/*
 * There is both per-process and per-thread exit handlers. The first list is
 * controlled by a mutex. The other is in thread local storage.
 */

static ExitHandler *firstExitPtr = NULL;
				/* First in list of all exit handlers for
				 * application. */
static ExitHandler *firstLateExitPtr = NULL;
				/* First in list of all late exit handlers for
				 * application. */
TCL_DECLARE_MUTEX(exitMutex)

/*
 * This variable is set to 1 when Tcl_Exit is called. The variable is checked
 * by TclInExit() to allow different behavior for exit-time processing, e.g.,
 * in closing of files and pipes.
 */

static int inExit = 0;

static int subsystemsInitialized = 0;

/*
 * This variable contains the application wide exit handler. It will be called
 * by Tcl_Exit instead of the C-runtime exit if this variable is set to a
 * non-NULL value.
 */

static TCL_NORETURN1 Tcl_ExitProc *appExitPtr = NULL;

typedef struct ThreadSpecificData {
    ExitHandler *firstExitPtr;	/* First in list of all exit handlers for this
				 * thread. */
    int inExit;			/* True when this thread is exiting. This is
				 * used as a hack to decide to close the
				 * standard channels. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

#ifdef TCL_THREADS
typedef struct {
    Tcl_ThreadCreateProc *proc;	/* Main() function of the thread */
    ClientData clientData;	/* The one argument to Main() */
} ThreadClientData;
static Tcl_ThreadCreateType NewThreadProc(ClientData clientData);
#endif /* TCL_THREADS */

/*
 * Prototypes for functions referenced only in this file:
 */

static void		BgErrorDeleteProc(ClientData clientData,
			    Tcl_Interp *interp);
static void		HandleBgErrors(ClientData clientData);
static char *		VwaitVarProc(ClientData clientData,
			    Tcl_Interp *interp, const char *name1,
			    const char *name2, int flags);
static void		InvokeExitHandlers(void);
static void		FinalizeThread(int quick);

/*
 *----------------------------------------------------------------------
 *
 * Tcl_BackgroundError --
 *
 *	This function is invoked to handle errors that occur in Tcl commands
 *	that are invoked in "background" (e.g. from event or timer bindings).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A handler command is invoked later as an idle handler to process the
 *	error, passing it the interp result and return options.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_BackgroundError(
    Tcl_Interp *interp)		/* Interpreter in which an error has
				 * occurred. */
{
    Tcl_BackgroundException(interp, TCL_ERROR);
}

void
Tcl_BackgroundException(
    Tcl_Interp *interp,		/* Interpreter in which an exception has
				 * occurred. */
    int code)			/* The exception code value */
{
    BgError *errPtr;
    ErrAssocData *assocPtr;

    if (code == TCL_OK) {
	return;
    }

    errPtr = ckalloc(sizeof(BgError));
    errPtr->errorMsg = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(errPtr->errorMsg);
    errPtr->returnOpts = Tcl_GetReturnOptions(interp, code);
    Tcl_IncrRefCount(errPtr->returnOpts);
    errPtr->nextPtr = NULL;

    (void) TclGetBgErrorHandler(interp);
    assocPtr = Tcl_GetAssocData(interp, "tclBgError", NULL);
    if (assocPtr->firstBgPtr == NULL) {
	assocPtr->firstBgPtr = errPtr;
	Tcl_DoWhenIdle(HandleBgErrors, assocPtr);
    } else {
	assocPtr->lastBgPtr->nextPtr = errPtr;
    }
    assocPtr->lastBgPtr = errPtr;
    Tcl_ResetResult(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * HandleBgErrors --
 *
 *	This function is invoked as an idle handler to process all of the
 *	accumulated background errors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what actions the handler command takes for the errors.
 *
 *----------------------------------------------------------------------
 */

static void
HandleBgErrors(
    ClientData clientData)	/* Pointer to ErrAssocData structure. */
{
    ErrAssocData *assocPtr = clientData;
    Tcl_Interp *interp = assocPtr->interp;
    BgError *errPtr;

    /*
     * Not bothering to save/restore the interp state. Assume that any code
     * that has interp state it needs to keep will make its own
     * Tcl_SaveInterpState call before calling something like Tcl_DoOneEvent()
     * that could lead us here.
     */

    Tcl_Preserve(assocPtr);
    Tcl_Preserve(interp);
    while (assocPtr->firstBgPtr != NULL) {
	int code, prefixObjc;
	Tcl_Obj **prefixObjv, **tempObjv;

	/*
	 * Note we copy the handler command prefix each pass through, so we do
	 * support one handler setting another handler.
	 */

	Tcl_Obj *copyObj = TclListObjCopy(NULL, assocPtr->cmdPrefix);

	errPtr = assocPtr->firstBgPtr;

	Tcl_ListObjGetElements(NULL, copyObj, &prefixObjc, &prefixObjv);
	tempObjv = ckalloc((prefixObjc+2) * sizeof(Tcl_Obj *));
	memcpy(tempObjv, prefixObjv, prefixObjc*sizeof(Tcl_Obj *));
	tempObjv[prefixObjc] = errPtr->errorMsg;
	tempObjv[prefixObjc+1] = errPtr->returnOpts;
	Tcl_AllowExceptions(interp);
	code = Tcl_EvalObjv(interp, prefixObjc+2, tempObjv, TCL_EVAL_GLOBAL);

	/*
	 * Discard the command and the information about the error report.
	 */

	Tcl_DecrRefCount(copyObj);
	Tcl_DecrRefCount(errPtr->errorMsg);
	Tcl_DecrRefCount(errPtr->returnOpts);
	assocPtr->firstBgPtr = errPtr->nextPtr;
	ckfree(errPtr);
	ckfree(tempObjv);

	if (code == TCL_BREAK) {
	    /*
	     * Break means cancel any remaining error reports for this
	     * interpreter.
	     */

	    while (assocPtr->firstBgPtr != NULL) {
		errPtr = assocPtr->firstBgPtr;
		assocPtr->firstBgPtr = errPtr->nextPtr;
		Tcl_DecrRefCount(errPtr->errorMsg);
		Tcl_DecrRefCount(errPtr->returnOpts);
		ckfree(errPtr);
	    }
	} else if ((code == TCL_ERROR) && !Tcl_IsSafe(interp)) {
	    Tcl_Channel errChannel = Tcl_GetStdChannel(TCL_STDERR);

	    if (errChannel != NULL) {
		Tcl_Obj *options = Tcl_GetReturnOptions(interp, code);
		Tcl_Obj *keyPtr, *valuePtr = NULL;

		TclNewLiteralStringObj(keyPtr, "-errorinfo");
		Tcl_IncrRefCount(keyPtr);
		Tcl_DictObjGet(NULL, options, keyPtr, &valuePtr);
		Tcl_DecrRefCount(keyPtr);

		Tcl_WriteChars(errChannel,
			"error in background error handler:\n", -1);
		if (valuePtr) {
		    Tcl_WriteObj(errChannel, valuePtr);
		} else {
		    Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
		}
		Tcl_WriteChars(errChannel, "\n", 1);
		Tcl_Flush(errChannel);
		Tcl_DecrRefCount(options);
	    }
	}
    }
    assocPtr->lastBgPtr = NULL;
    Tcl_Release(interp);
    Tcl_Release(assocPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclDefaultBgErrorHandlerObjCmd --
 *
 *	This function is invoked to process the "::tcl::Bgerror" Tcl command.
 *	It is the default handler command registered with [interp bgerror] for
 *	the sake of compatibility with older Tcl releases.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	Depends on what actions the "bgerror" command takes for the errors.
 *
 *----------------------------------------------------------------------
 */

int
TclDefaultBgErrorHandlerObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *keyPtr, *valuePtr;
    Tcl_Obj *tempObjv[2];
    int result, code, level;
    Tcl_InterpState saved;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "msg options");
	return TCL_ERROR;
    }

    /*
     * Check for a valid return options dictionary.
     */

    TclNewLiteralStringObj(keyPtr, "-level");
    Tcl_IncrRefCount(keyPtr);
    result = Tcl_DictObjGet(NULL, objv[2], keyPtr, &valuePtr);
    Tcl_DecrRefCount(keyPtr);
    if (result != TCL_OK || valuePtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"missing return option \"-level\"", -1));
	Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "MISSING", NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, valuePtr, &level) == TCL_ERROR) {
	return TCL_ERROR;
    }
    TclNewLiteralStringObj(keyPtr, "-code");
    Tcl_IncrRefCount(keyPtr);
    result = Tcl_DictObjGet(NULL, objv[2], keyPtr, &valuePtr);
    Tcl_DecrRefCount(keyPtr);
    if (result != TCL_OK || valuePtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"missing return option \"-code\"", -1));
	Tcl_SetErrorCode(interp, "TCL", "ARGUMENT", "MISSING", NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, valuePtr, &code) == TCL_ERROR) {
	return TCL_ERROR;
    }

    if (level != 0) {
	/*
	 * We're handling a TCL_RETURN exception.
	 */

	code = TCL_RETURN;
    }
    if (code == TCL_OK) {
	/*
	 * Somehow we got to exception handling with no exception. (Pass
	 * TCL_OK to Tcl_BackgroundException()?) Just return without doing
	 * anything.
	 */

	return TCL_OK;
    }

    /*
     * Construct the bgerror command.
     */

    TclNewLiteralStringObj(tempObjv[0], "bgerror");
    Tcl_IncrRefCount(tempObjv[0]);

    /*
     * Determine error message argument.  Check the return options in case
     * a non-error exception brought us here.
     */

    switch (code) {
    case TCL_ERROR:
	tempObjv[1] = objv[1];
	break;
    case TCL_BREAK:
	TclNewLiteralStringObj(tempObjv[1],
		"invoked \"break\" outside of a loop");
	break;
    case TCL_CONTINUE:
	TclNewLiteralStringObj(tempObjv[1],
		"invoked \"continue\" outside of a loop");
	break;
    default:
	tempObjv[1] = Tcl_ObjPrintf("command returned bad code: %d", code);
	break;
    }
    Tcl_IncrRefCount(tempObjv[1]);

    if (code != TCL_ERROR) {
	Tcl_SetObjResult(interp, tempObjv[1]);
    }

    TclNewLiteralStringObj(keyPtr, "-errorcode");
    Tcl_IncrRefCount(keyPtr);
    result = Tcl_DictObjGet(NULL, objv[2], keyPtr, &valuePtr);
    Tcl_DecrRefCount(keyPtr);
    if (result == TCL_OK && valuePtr != NULL) {
	Tcl_SetObjErrorCode(interp, valuePtr);
    }

    TclNewLiteralStringObj(keyPtr, "-errorinfo");
    Tcl_IncrRefCount(keyPtr);
    result = Tcl_DictObjGet(NULL, objv[2], keyPtr, &valuePtr);
    Tcl_DecrRefCount(keyPtr);
    if (result == TCL_OK && valuePtr != NULL) {
	Tcl_AppendObjToErrorInfo(interp, valuePtr);
    }

    if (code == TCL_ERROR) {
	Tcl_SetObjResult(interp, tempObjv[1]);
    }

    /*
     * Save interpreter state so we can restore it if multiple handler
     * attempts are needed.
     */

    saved = Tcl_SaveInterpState(interp, code);

    /*
     * Invoke the bgerror command.
     */

    Tcl_AllowExceptions(interp);
    code = Tcl_EvalObjv(interp, 2, tempObjv, TCL_EVAL_GLOBAL);
    if (code == TCL_ERROR) {
	/*
	 * If the interpreter is safe, we look for a hidden command named
	 * "bgerror" and call that with the error information. Otherwise,
	 * simply ignore the error. The rationale is that this could be an
	 * error caused by a malicious applet trying to cause an infinite
	 * barrage of error messages. The hidden "bgerror" command can be used
	 * by a security policy to interpose on such attacks and e.g. kill the
	 * applet after a few attempts.
	 */

	if (Tcl_IsSafe(interp)) {
	    Tcl_RestoreInterpState(interp, saved);
	    TclObjInvoke(interp, 2, tempObjv, TCL_INVOKE_HIDDEN);
	} else {
	    Tcl_Channel errChannel = Tcl_GetStdChannel(TCL_STDERR);

	    if (errChannel != NULL) {
		Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);

		Tcl_IncrRefCount(resultPtr);
		if (Tcl_FindCommand(interp, "bgerror", NULL,
			TCL_GLOBAL_ONLY) == NULL) {
		    Tcl_RestoreInterpState(interp, saved);
		    Tcl_WriteObj(errChannel, Tcl_GetVar2Ex(interp,
			    "errorInfo", NULL, TCL_GLOBAL_ONLY));
		    Tcl_WriteChars(errChannel, "\n", -1);
		} else {
		    Tcl_DiscardInterpState(saved);
		    Tcl_WriteChars(errChannel,
			    "bgerror failed to handle background error.\n",-1);
		    Tcl_WriteChars(errChannel, "    Original error: ", -1);
		    Tcl_WriteObj(errChannel, tempObjv[1]);
		    Tcl_WriteChars(errChannel, "\n", -1);
		    Tcl_WriteChars(errChannel, "    Error in bgerror: ", -1);
		    Tcl_WriteObj(errChannel, resultPtr);
		    Tcl_WriteChars(errChannel, "\n", -1);
		}
		Tcl_DecrRefCount(resultPtr);
		Tcl_Flush(errChannel);
	    } else {
		Tcl_DiscardInterpState(saved);
	    }
	}
	code = TCL_OK;
    } else {
	Tcl_DiscardInterpState(saved);
    }

    Tcl_DecrRefCount(tempObjv[0]);
    Tcl_DecrRefCount(tempObjv[1]);
    Tcl_ResetResult(interp);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclSetBgErrorHandler --
 *
 *	This function sets the command prefix to be used to handle background
 *	errors in interp.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Error handler is registered.
 *
 *----------------------------------------------------------------------
 */

void
TclSetBgErrorHandler(
    Tcl_Interp *interp,
    Tcl_Obj *cmdPrefix)
{
    ErrAssocData *assocPtr = Tcl_GetAssocData(interp, "tclBgError", NULL);

    if (cmdPrefix == NULL) {
	Tcl_Panic("TclSetBgErrorHandler: NULL cmdPrefix argument");
    }
    if (assocPtr == NULL) {
	/*
	 * First access: initialize.
	 */

	assocPtr = ckalloc(sizeof(ErrAssocData));
	assocPtr->interp = interp;
	assocPtr->cmdPrefix = NULL;
	assocPtr->firstBgPtr = NULL;
	assocPtr->lastBgPtr = NULL;
	Tcl_SetAssocData(interp, "tclBgError", BgErrorDeleteProc, assocPtr);
    }
    if (assocPtr->cmdPrefix) {
	Tcl_DecrRefCount(assocPtr->cmdPrefix);
    }
    assocPtr->cmdPrefix = cmdPrefix;
    Tcl_IncrRefCount(assocPtr->cmdPrefix);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetBgErrorHandler --
 *
 *	This function retrieves the command prefix currently used to handle
 *	background errors in interp.
 *
 * Results:
 *	A (Tcl_Obj *) to a list of words (command prefix).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclGetBgErrorHandler(
    Tcl_Interp *interp)
{
    ErrAssocData *assocPtr = Tcl_GetAssocData(interp, "tclBgError", NULL);

    if (assocPtr == NULL) {
	Tcl_Obj *bgerrorObj;

	TclNewLiteralStringObj(bgerrorObj, "::tcl::Bgerror");
	TclSetBgErrorHandler(interp, bgerrorObj);
	assocPtr = Tcl_GetAssocData(interp, "tclBgError", NULL);
    }
    return assocPtr->cmdPrefix;
}

/*
 *----------------------------------------------------------------------
 *
 * BgErrorDeleteProc --
 *
 *	This function is associated with the "tclBgError" assoc data for an
 *	interpreter; it is invoked when the interpreter is deleted in order to
 *	free the information assoicated with any pending error reports.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Background error information is freed: if there were any pending error
 *	reports, they are canceled.
 *
 *----------------------------------------------------------------------
 */

static void
BgErrorDeleteProc(
    ClientData clientData,	/* Pointer to ErrAssocData structure. */
    Tcl_Interp *interp)		/* Interpreter being deleted. */
{
    ErrAssocData *assocPtr = clientData;
    BgError *errPtr;

    while (assocPtr->firstBgPtr != NULL) {
	errPtr = assocPtr->firstBgPtr;
	assocPtr->firstBgPtr = errPtr->nextPtr;
	Tcl_DecrRefCount(errPtr->errorMsg);
	Tcl_DecrRefCount(errPtr->returnOpts);
	ckfree(errPtr);
    }
    Tcl_CancelIdleCall(HandleBgErrors, assocPtr);
    Tcl_DecrRefCount(assocPtr->cmdPrefix);
    Tcl_EventuallyFree(assocPtr, TCL_DYNAMIC);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateExitHandler --
 *
 *	Arrange for a given function to be invoked just before the application
 *	exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc will be invoked with clientData as argument when the application
 *	exits.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateExitHandler(
    Tcl_ExitProc *proc,		/* Function to invoke. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr = ckalloc(sizeof(ExitHandler));

    exitPtr->proc = proc;
    exitPtr->clientData = clientData;
    Tcl_MutexLock(&exitMutex);
    exitPtr->nextPtr = firstExitPtr;
    firstExitPtr = exitPtr;
    Tcl_MutexUnlock(&exitMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreateLateExitHandler --
 *
 *	Arrange for a given function to be invoked after all pre-thread
 *	cleanups.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc will be invoked with clientData as argument when the application
 *	exits.
 *
 *----------------------------------------------------------------------
 */

void
TclCreateLateExitHandler(
    Tcl_ExitProc *proc,		/* Function to invoke. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr = ckalloc(sizeof(ExitHandler));

    exitPtr->proc = proc;
    exitPtr->clientData = clientData;
    Tcl_MutexLock(&exitMutex);
    exitPtr->nextPtr = firstLateExitPtr;
    firstLateExitPtr = exitPtr;
    Tcl_MutexUnlock(&exitMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteExitHandler --
 *
 *	This function cancels an existing exit handler matching proc and
 *	clientData, if such a handler exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is an exit handler corresponding to proc and clientData then
 *	it is canceled; if no such handler exists then nothing happens.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteExitHandler(
    Tcl_ExitProc *proc,		/* Function that was previously registered. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr, *prevPtr;

    Tcl_MutexLock(&exitMutex);
    for (prevPtr = NULL, exitPtr = firstExitPtr; exitPtr != NULL;
	    prevPtr = exitPtr, exitPtr = exitPtr->nextPtr) {
	if ((exitPtr->proc == proc)
		&& (exitPtr->clientData == clientData)) {
	    if (prevPtr == NULL) {
		firstExitPtr = exitPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = exitPtr->nextPtr;
	    }
	    ckfree(exitPtr);
	    break;
	}
    }
    Tcl_MutexUnlock(&exitMutex);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteLateExitHandler --
 *
 *	This function cancels an existing late exit handler matching proc and
 *	clientData, if such a handler exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is a late exit handler corresponding to proc and clientData
 *	then it is canceled; if no such handler exists then nothing happens.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteLateExitHandler(
    Tcl_ExitProc *proc,		/* Function that was previously registered. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr, *prevPtr;

    Tcl_MutexLock(&exitMutex);
    for (prevPtr = NULL, exitPtr = firstLateExitPtr; exitPtr != NULL;
	    prevPtr = exitPtr, exitPtr = exitPtr->nextPtr) {
	if ((exitPtr->proc == proc)
		&& (exitPtr->clientData == clientData)) {
	    if (prevPtr == NULL) {
		firstLateExitPtr = exitPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = exitPtr->nextPtr;
	    }
	    ckfree(exitPtr);
	    break;
	}
    }
    Tcl_MutexUnlock(&exitMutex);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateThreadExitHandler --
 *
 *	Arrange for a given function to be invoked just before the current
 *	thread exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc will be invoked with clientData as argument when the application
 *	exits.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateThreadExitHandler(
    Tcl_ExitProc *proc,		/* Function to invoke. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    exitPtr = ckalloc(sizeof(ExitHandler));
    exitPtr->proc = proc;
    exitPtr->clientData = clientData;
    exitPtr->nextPtr = tsdPtr->firstExitPtr;
    tsdPtr->firstExitPtr = exitPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteThreadExitHandler --
 *
 *	This function cancels an existing exit handler matching proc and
 *	clientData, if such a handler exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is an exit handler corresponding to proc and clientData then
 *	it is canceled; if no such handler exists then nothing happens.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteThreadExitHandler(
    Tcl_ExitProc *proc,		/* Function that was previously registered. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    ExitHandler *exitPtr, *prevPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    for (prevPtr = NULL, exitPtr = tsdPtr->firstExitPtr; exitPtr != NULL;
	    prevPtr = exitPtr, exitPtr = exitPtr->nextPtr) {
	if ((exitPtr->proc == proc)
		&& (exitPtr->clientData == clientData)) {
	    if (prevPtr == NULL) {
		tsdPtr->firstExitPtr = exitPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = exitPtr->nextPtr;
	    }
	    ckfree(exitPtr);
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetExitProc --
 *
 *	This function sets the application wide exit handler that will be
 *	called by Tcl_Exit in place of the C-runtime exit. If the application
 *	wide exit handler is NULL, the C-runtime exit will be used instead.
 *
 * Results:
 *	The previously set application wide exit handler.
 *
 * Side effects:
 *	Sets the application wide exit handler to the specified value.
 *
 *----------------------------------------------------------------------
 */

Tcl_ExitProc *
Tcl_SetExitProc(
    TCL_NORETURN1 Tcl_ExitProc *proc)		/* New exit handler for app or NULL */
{
    Tcl_ExitProc *prevExitProc;

    /*
     * Swap the old exit proc for the new one, saving the old one for our
     * return value.
     */

    Tcl_MutexLock(&exitMutex);
    prevExitProc = appExitPtr;
    appExitPtr = proc;
    Tcl_MutexUnlock(&exitMutex);

    return prevExitProc;
}


/*
 *----------------------------------------------------------------------
 *
 * InvokeExitHandlers --
 *
 *      Call the registered exit handlers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The exit handlers are invoked, and the ExitHandler struct is
 *      freed.
 *
 *----------------------------------------------------------------------
 */
static void
InvokeExitHandlers(void)
{
    ExitHandler *exitPtr;

    Tcl_MutexLock(&exitMutex);
    inExit = 1;

    for (exitPtr = firstExitPtr; exitPtr != NULL; exitPtr = firstExitPtr) {
	/*
	 * Be careful to remove the handler from the list before invoking its
	 * callback. This protects us against double-freeing if the callback
	 * should call Tcl_DeleteExitHandler on itself.
	 */

	firstExitPtr = exitPtr->nextPtr;
	Tcl_MutexUnlock(&exitMutex);
	exitPtr->proc(exitPtr->clientData);
	ckfree(exitPtr);
	Tcl_MutexLock(&exitMutex);
    }
    firstExitPtr = NULL;
    Tcl_MutexUnlock(&exitMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_Exit --
 *
 *	This function is called to terminate the application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All existing exit handlers are invoked, then the application ends.
 *
 *----------------------------------------------------------------------
 */

TCL_NORETURN void
Tcl_Exit(
    int status)			/* Exit status for application; typically 0
				 * for normal return, 1 for error return. */
{
    TCL_NORETURN1 Tcl_ExitProc *currentAppExitPtr;

    Tcl_MutexLock(&exitMutex);
    currentAppExitPtr = appExitPtr;
    Tcl_MutexUnlock(&exitMutex);

    if (currentAppExitPtr) {
	/*
	 * Warning: this code SHOULD NOT return, as there is code that depends
	 * on Tcl_Exit never returning. In fact, we will Tcl_Panic if anyone
	 * returns, so critical is this dependcy.
	 */

	currentAppExitPtr(INT2PTR(status));
	Tcl_Panic("AppExitProc returned unexpectedly");
    } else {

	if (TclFullFinalizationRequested()) {

	    /*
	     * Thorough finalization for Valgrind et al.
	     */

	    Tcl_Finalize();

	} else {

	    /*
	     * Fast and deterministic exit (default behavior)
	     */

	    InvokeExitHandlers();

	    /*
	     * Ensure the thread-specific data is initialised as it is used in
	     * Tcl_FinalizeThread()
	     */

	    (void) TCL_TSD_INIT(&dataKey);

	    /*
	     * Now finalize the calling thread only (others are not safely
	     * reachable).  Among other things, this triggers a flush of the
	     * Tcl_Channels that may have data enqueued.
	     */

	    FinalizeThread(/* quick */ 1);
	}
	TclpExit(status);
	Tcl_Panic("OS exit failed!");
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * TclInitSubsystems --
 *
 *	Initialize various subsytems in Tcl. This should be called the first
 *	time an interp is created, or before any of the subsystems are used.
 *	This function ensures an order for the initialization of subsystems:
 *
 *	1. that cannot be initialized in lazy order because they are mutually
 *	dependent.
 *
 *	2. so that they can be finalized in a known order w/o causing the
 *	subsequent re-initialization of a subsystem in the act of shutting
 *	down another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Varied, see the respective initialization routines.
 *
 *-------------------------------------------------------------------------
 */

void
TclInitSubsystems(void)
{
    if (inExit != 0) {
	Tcl_Panic("TclInitSubsystems called while exiting");
    }

    if (subsystemsInitialized == 0) {
	/*
	 * Double check inside the mutex. There are definitly calls back into
	 * this routine from some of the functions below.
	 */

	TclpInitLock();
	if (subsystemsInitialized == 0) {

		/*
	     * Initialize locks used by the memory allocators before anything
	     * interesting happens so we can use the allocators in the
	     * implementation of self-initializing locks.
	     */

	    TclInitThreadStorage();     /* Creates master hash table for
					 * thread local storage */
#if USE_TCLALLOC
	    TclInitAlloc();		/* Process wide mutex init */
#endif
#ifdef TCL_MEM_DEBUG
	    TclInitDbCkalloc();		/* Process wide mutex init */
#endif

	    TclpInitPlatform();		/* Creates signal handler(s) */
	    TclInitDoubleConversion();	/* Initializes constants for
					 * converting to/from double. */
	    TclInitObjSubsystem();	/* Register obj types, create
					 * mutexes. */
	    TclInitIOSubsystem();	/* Inits a tsd key (noop). */
	    TclInitEncodingSubsystem();	/* Process wide encoding init. */
	    TclpSetInterfaces();
	    TclInitNamespaceSubsystem();/* Register ns obj type (mutexed). */
	    subsystemsInitialized = 1;
	}
	TclpInitUnlock();
    }
    TclInitNotifier();
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Finalize --
 *
 *	Shut down Tcl. First calls registered exit handlers, then carefully
 *	shuts down various subsystems.  Should be invoked by user before the
 *	Tcl shared library is being unloaded in an embedded context.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Varied, see the respective finalization routines.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Finalize(void)
{
    ExitHandler *exitPtr;

    /*
     * Invoke exit handlers first.
     */

    InvokeExitHandlers();

    TclpInitLock();
    if (subsystemsInitialized == 0) {
	goto alreadyFinalized;
    }
    subsystemsInitialized = 0;

    /*
     * Ensure the thread-specific data is initialised as it is used in
     * Tcl_FinalizeThread()
     */

    (void) TCL_TSD_INIT(&dataKey);

    /*
     * Clean up after the current thread now, after exit handlers. In
     * particular, the testexithandler command sets up something that writes
     * to standard output, which gets closed. Note that there is no
     * thread-local storage or IO subsystem after this call.
     */

    Tcl_FinalizeThread();

    /*
     * Now invoke late (process-wide) exit handlers.
     */

    Tcl_MutexLock(&exitMutex);
    for (exitPtr = firstLateExitPtr; exitPtr != NULL;
	    exitPtr = firstLateExitPtr) {
	/*
	 * Be careful to remove the handler from the list before invoking its
	 * callback. This protects us against double-freeing if the callback
	 * should call Tcl_DeleteLateExitHandler on itself.
	 */

	firstLateExitPtr = exitPtr->nextPtr;
	Tcl_MutexUnlock(&exitMutex);
	exitPtr->proc(exitPtr->clientData);
	ckfree(exitPtr);
	Tcl_MutexLock(&exitMutex);
    }
    firstLateExitPtr = NULL;
    Tcl_MutexUnlock(&exitMutex);

    /*
     * Now finalize the Tcl execution environment. Note that this must be done
     * after the exit handlers, because there are order dependencies.
     */

    TclFinalizeEvaluation();
    TclFinalizeExecution();
    TclFinalizeEnvironment();

    /*
     * Finalizing the filesystem must come after anything which might
     * conceivably interact with the 'Tcl_FS' API.
     */

    TclFinalizeFilesystem();

    /*
     * Undo all Tcl_ObjType registrations, and reset the master list of free
     * Tcl_Obj's. After this returns, no more Tcl_Obj's should be allocated or
     * freed.
     *
     * Note in particular that TclFinalizeObjects() must follow
     * TclFinalizeFilesystem() because TclFinalizeFilesystem free's the
     * Tcl_Obj that holds the path of the current working directory.
     */

    TclFinalizeObjects();

    /*
     * We must be sure the encoding finalization doesn't need to examine the
     * filesystem in any way. Since it only needs to clean up internal data
     * structures, this is fine.
     */

    TclFinalizeEncodingSubsystem();

    /*
     * Repeat finalization of the thread local storage once more. Although
     * this step is already done by the Tcl_FinalizeThread call above, series
     * of events happening afterwards may re-initialize TSD slots. Those need
     * to be finalized again, otherwise we're leaking memory chunks. Very
     * important to note is that things happening afterwards should not
     * reference anything which may re-initialize TSD's. This includes freeing
     * Tcl_Objs's, among other things.
     *
     * This fixes the Tcl Bug #990552.
     */

    TclFinalizeThreadData(/* quick */ 0);

    /*
     * Now we can free constants for conversions to/from double.
     */

    TclFinalizeDoubleConversion();

    /*
     * There have been several bugs in the past that cause exit handlers to be
     * established during Tcl_Finalize processing. Such exit handlers leave
     * malloc'ed memory, and Tcl_FinalizeMemorySubsystem or
     * Tcl_FinalizeThreadAlloc will result in a corrupted heap. The result can
     * be a mysterious crash on process exit. Check here that nobody's done
     * this.
     */

    if (firstExitPtr != NULL) {
	Tcl_Panic("exit handlers were created during Tcl_Finalize");
    }

    TclFinalizePreserve();

    /*
     * Free synchronization objects. There really should only be one thread
     * alive at this moment.
     */

    TclFinalizeSynchronization();

    /*
     * Close down the thread-specific object allocator.
     */

#if defined(TCL_THREADS) && defined(USE_THREAD_ALLOC)
    TclFinalizeThreadAlloc();
#endif

    /*
     * We defer unloading of packages until very late to avoid memory access
     * issues. Both exit callbacks and synchronization variables may be stored
     * in packages.
     *
     * Note that TclFinalizeLoad unloads packages in the reverse of the order
     * they were loaded in (i.e. last to be loaded is the first to be
     * unloaded). This can be important for correct unloading when
     * dependencies exist.
     *
     * Once load has been finalized, we will have deleted any temporary copies
     * of shared libraries and can therefore reset the filesystem to its
     * original state.
     */

    TclFinalizeLoad();
    TclResetFilesystem();

    /*
     * At this point, there should no longer be any ckalloc'ed memory.
     */

    TclFinalizeMemorySubsystem();

  alreadyFinalized:
    TclFinalizeLock();
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FinalizeThread --
 *
 *	Runs the exit handlers to allow Tcl to clean up its state about a
 *	particular thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Varied, see the respective finalization routines.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FinalizeThread(void)
{
    FinalizeThread(/* quick */ 0);
}

void
FinalizeThread(
    int quick)
{
    ExitHandler *exitPtr;
    ThreadSpecificData *tsdPtr;

    /*
     * We use TclThreadDataKeyGet here, rather than Tcl_GetThreadData, because
     * we don't want to initialize the data block if it hasn't been
     * initialized already.
     */

    tsdPtr = TclThreadDataKeyGet(&dataKey);
    if (tsdPtr != NULL) {
	tsdPtr->inExit = 1;

	for (exitPtr = tsdPtr->firstExitPtr; exitPtr != NULL;
		exitPtr = tsdPtr->firstExitPtr) {
	    /*
	     * Be careful to remove the handler from the list before invoking
	     * its callback. This protects us against double-freeing if the
	     * callback should call Tcl_DeleteThreadExitHandler on itself.
	     */

	    tsdPtr->firstExitPtr = exitPtr->nextPtr;
	    exitPtr->proc(exitPtr->clientData);
	    ckfree(exitPtr);
	}
	TclFinalizeIOSubsystem();
	TclFinalizeNotifier();
	TclFinalizeAsync();
	TclFinalizeThreadObjects();
    }

    /*
     * Blow away all thread local storage blocks.
     *
     * Note that Tcl API allows creation of threads which do not use any Tcl
     * interp or other Tcl subsytems. Those threads might, however, use thread
     * local storage, so we must unconditionally finalize it.
     *
     * Fix [Bug #571002]
     */
    TclFinalizeThreadData(quick);
}

/*
 *----------------------------------------------------------------------
 *
 * TclInExit --
 *
 *	Determines if we are in the middle of exit-time cleanup.
 *
 * Results:
 *	If we are in the middle of exiting, 1, otherwise 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclInExit(void)
{
    return inExit;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInThreadExit --
 *
 *	Determines if we are in the middle of thread exit-time cleanup.
 *
 * Results:
 *	If we are in the middle of exiting this thread, 1, otherwise 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclInThreadExit(void)
{
    ThreadSpecificData *tsdPtr = TclThreadDataKeyGet(&dataKey);

    if (tsdPtr == NULL) {
	return 0;
    }
    return tsdPtr->inExit;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VwaitObjCmd --
 *
 *	This function is invoked to process the "vwait" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_VwaitObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int done, foundEvent;
    const char *nameString;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name");
	return TCL_ERROR;
    }
    nameString = Tcl_GetString(objv[1]);
    if (Tcl_TraceVar2(interp, nameString, NULL,
	    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	    VwaitVarProc, &done) != TCL_OK) {
	return TCL_ERROR;
    };
    done = 0;
    foundEvent = 1;
    while (!done && foundEvent) {
	foundEvent = Tcl_DoOneEvent(TCL_ALL_EVENTS);
	if (Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG) == TCL_ERROR) {
	    break;
	}
	if (Tcl_LimitExceeded(interp)) {
	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("limit exceeded", -1));
	    break;
	}
    }
    Tcl_UntraceVar2(interp, nameString, NULL,
	    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	    VwaitVarProc, &done);

    if (!foundEvent) {
	Tcl_ResetResult(interp);
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't wait for variable \"%s\": would wait forever",
		nameString));
	Tcl_SetErrorCode(interp, "TCL", "EVENT", "NO_SOURCES", NULL);
	return TCL_ERROR;
    }
    if (!done) {
	/*
	 * The interpreter's result was already set to the right error message
	 * prior to exiting the loop above.
	 */

	return TCL_ERROR;
    }

    /*
     * Clear out the interpreter's result, since it may have been set by event
     * handlers.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;
}

	/* ARGSUSED */
static char *
VwaitVarProc(
    ClientData clientData,	/* Pointer to integer to set to 1. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Name of variable. */
    const char *name2,		/* Second part of variable name. */
    int flags)			/* Information about what happened. */
{
    int *donePtr = clientData;

    *donePtr = 1;
    Tcl_UntraceVar(interp, name1, TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	    VwaitVarProc, clientData);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpdateObjCmd --
 *
 *	This function is invoked to process the "update" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_UpdateObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int optionIndex;
    int flags = 0;		/* Initialized to avoid compiler warning. */
    static const char *const updateOptions[] = {"idletasks", NULL};
    enum updateOptions {OPT_IDLETASKS};

    if (objc == 1) {
	flags = TCL_ALL_EVENTS|TCL_DONT_WAIT;
    } else if (objc == 2) {
	if (Tcl_GetIndexFromObj(interp, objv[1], updateOptions,
		"option", 0, &optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum updateOptions) optionIndex) {
	case OPT_IDLETASKS:
	    flags = TCL_WINDOW_EVENTS|TCL_IDLE_EVENTS|TCL_DONT_WAIT;
	    break;
	default:
	    Tcl_Panic("Tcl_UpdateObjCmd: bad option index to UpdateOptions");
	}
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?idletasks?");
	return TCL_ERROR;
    }

    while (Tcl_DoOneEvent(flags) != 0) {
	if (Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (Tcl_LimitExceeded(interp)) {
	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("limit exceeded", -1));
	    return TCL_ERROR;
	}
    }

    /*
     * Must clear the interpreter's result because event handlers could have
     * executed commands.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;
}

#ifdef TCL_THREADS
/*
 *----------------------------------------------------------------------
 *
 * NewThreadProc --
 *
 *	Bootstrap function of a new Tcl thread.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Initializes Tcl notifier for the current thread.
 *
 *----------------------------------------------------------------------
 */

static Tcl_ThreadCreateType
NewThreadProc(
    ClientData clientData)
{
    ThreadClientData *cdPtr = clientData;
    ClientData threadClientData;
    Tcl_ThreadCreateProc *threadProc;

    threadProc = cdPtr->proc;
    threadClientData = cdPtr->clientData;
    ckfree(clientData);		/* Allocated in Tcl_CreateThread() */

    threadProc(threadClientData);

    TCL_THREAD_CREATE_RETURN;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateThread --
 *
 *	This function creates a new thread. This actually belongs to the
 *	tclThread.c file but since we use some private data structures local
 *	to this file, it is placed here.
 *
 * Results:
 *	TCL_OK if the thread could be created. The thread ID is returned in a
 *	parameter.
 *
 * Side effects:
 *	A new thread is created.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CreateThread(
    Tcl_ThreadId *idPtr,	/* Return, the ID of the thread */
    Tcl_ThreadCreateProc *proc,	/* Main() function of the thread */
    ClientData clientData,	/* The one argument to Main() */
    int stackSize,		/* Size of stack for the new thread */
    int flags)			/* Flags controlling behaviour of the new
				 * thread. */
{
#ifdef TCL_THREADS
    ThreadClientData *cdPtr = ckalloc(sizeof(ThreadClientData));
    int result;

    cdPtr->proc = proc;
    cdPtr->clientData = clientData;
    result = TclpThreadCreate(idPtr, NewThreadProc, cdPtr, stackSize, flags);
    if (result != TCL_OK) {
	ckfree(cdPtr);
    }
    return result;
#else
    return TCL_ERROR;
#endif /* TCL_THREADS */
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
