/*
 * tclIOCmd.c --
 *
 *	Contains the definitions of most of the Tcl commands relating to IO.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Callback structure for accept callback in a TCP server.
 */

typedef struct AcceptCallback {
    char *script;		/* Script to invoke. */
    Tcl_Interp *interp;		/* Interpreter in which to run it. */
} AcceptCallback;

/*
 * Thread local storage used to maintain a per-thread stdout channel obj.
 * It must be per-thread because of std channel limitations.
 */

typedef struct ThreadSpecificData {
    int initialized;		/* Set to 1 when the module is initialized. */
    Tcl_Obj *stdoutObjPtr;	/* Cached stdout channel Tcl_Obj */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * Static functions for this file:
 */

static void		FinalizeIOCmdTSD(ClientData clientData);
static void		AcceptCallbackProc(ClientData callbackData,
			    Tcl_Channel chan, char *address, int port);
static int		ChanPendingObjCmd(ClientData unused,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		ChanTruncateObjCmd(ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		RegisterTcpServerInterpCleanup(Tcl_Interp *interp,
			    AcceptCallback *acceptCallbackPtr);
static void		TcpAcceptCallbacksDeleteProc(ClientData clientData,
			    Tcl_Interp *interp);
static void		TcpServerCloseProc(ClientData callbackData);
static void		UnregisterTcpServerInterpCleanupProc(
			    Tcl_Interp *interp,
			    AcceptCallback *acceptCallbackPtr);

/*
 *----------------------------------------------------------------------
 *
 * FinalizeIOCmdTSD --
 *
 *	Release the storage associated with the per-thread cache.
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
FinalizeIOCmdTSD(
    ClientData clientData)	/* Not used. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->stdoutObjPtr != NULL) {
	Tcl_DecrRefCount(tsdPtr->stdoutObjPtr);
	tsdPtr->stdoutObjPtr = NULL;
    }
    tsdPtr->initialized = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PutsObjCmd --
 *
 *	This function is invoked to process the "puts" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Produces output on a channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_PutsObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to puts on. */
    Tcl_Obj *string;		/* String to write. */
    Tcl_Obj *chanObjPtr = NULL;	/* channel object. */
    int newline;		/* Add a newline at end? */
    int result;			/* Result of puts operation. */
    int mode;			/* Mode in which channel is opened. */
    ThreadSpecificData *tsdPtr;

    switch (objc) {
    case 2:			/* [puts $x] */
	string = objv[1];
	newline = 1;
	break;

    case 3:			/* [puts -nonewline $x] or [puts $chan $x] */
	if (strcmp(TclGetString(objv[1]), "-nonewline") == 0) {
	    newline = 0;
	} else {
	    newline = 1;
	    chanObjPtr = objv[1];
	}
	string = objv[2];
	break;

    case 4:			/* [puts -nonewline $chan $x] or
				 * [puts $chan $x nonewline] */
	newline = 0;
	if (strcmp(TclGetString(objv[1]), "-nonewline") == 0) {
	    chanObjPtr = objv[2];
	    string = objv[3];
	    break;
#if TCL_MAJOR_VERSION < 9
	} else if (strcmp(TclGetString(objv[3]), "nonewline") == 0) {
	    /*
	     * The code below provides backwards compatibility with an old
	     * form of the command that is no longer recommended or
	     * documented. See also [Bug #3151675]. Will be removed in Tcl 9,
	     * maybe even earlier.
	     */

	    chanObjPtr = objv[1];
	    string = objv[2];
	    break;
#endif
	}
	/* Fall through */
    default:			/* [puts] or
				 * [puts some bad number of arguments...] */
	Tcl_WrongNumArgs(interp, 1, objv, "?-nonewline? ?channelId? string");
	return TCL_ERROR;
    }

    if (chanObjPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);

	if (!tsdPtr->initialized) {
	    tsdPtr->initialized = 1;
	    TclNewLiteralStringObj(tsdPtr->stdoutObjPtr, "stdout");
	    Tcl_IncrRefCount(tsdPtr->stdoutObjPtr);
	    Tcl_CreateThreadExitHandler(FinalizeIOCmdTSD, NULL);
	}
	chanObjPtr = tsdPtr->stdoutObjPtr;
    }
    if (TclGetChannelFromObj(interp, chanObjPtr, &chan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!(mode & TCL_WRITABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"channel \"%s\" wasn't opened for writing",
		TclGetString(chanObjPtr)));
	return TCL_ERROR;
    }

    TclChannelPreserve(chan);
    result = Tcl_WriteObj(chan, string);
    if (result < 0) {
	goto error;
    }
    if (newline != 0) {
	result = Tcl_WriteChars(chan, "\n", 1);
	if (result < 0) {
	    goto error;
	}
    }
    TclChannelRelease(chan);
    return TCL_OK;

    /*
     * TIP #219.
     * Capture error messages put by the driver into the bypass area and put
     * them into the regular interpreter result. Fall back to the regular
     * message if nothing was found in the bypass.
     */

  error:
    if (!TclChanCaughtErrorBypass(interp, chan)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("error writing \"%s\": %s",
		TclGetString(chanObjPtr), Tcl_PosixError(interp)));
    }
    TclChannelRelease(chan);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FlushObjCmd --
 *
 *	This function is called to process the Tcl "flush" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May cause output to appear on the specified channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FlushObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *chanObjPtr;
    Tcl_Channel chan;		/* The channel to flush on. */
    int mode;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }
    chanObjPtr = objv[1];
    if (TclGetChannelFromObj(interp, chanObjPtr, &chan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!(mode & TCL_WRITABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"channel \"%s\" wasn't opened for writing",
		TclGetString(chanObjPtr)));
	return TCL_ERROR;
    }

    TclChannelPreserve(chan);
    if (Tcl_Flush(chan) != TCL_OK) {
	/*
	 * TIP #219.
	 * Capture error messages put by the driver into the bypass area and
	 * put them into the regular interpreter result. Fall back to the
	 * regular message if nothing was found in the bypass.
	 */

	if (!TclChanCaughtErrorBypass(interp, chan)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error flushing \"%s\": %s",
		    TclGetString(chanObjPtr), Tcl_PosixError(interp)));
	}
	TclChannelRelease(chan);
	return TCL_ERROR;
    }
    TclChannelRelease(chan);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetsObjCmd --
 *
 *	This function is called to process the Tcl "gets" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May consume input from channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_GetsObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to read from. */
    int lineLen;		/* Length of line just read. */
    int mode;			/* Mode in which channel is opened. */
    Tcl_Obj *linePtr, *chanObjPtr;
    int code = TCL_OK;

    if ((objc != 2) && (objc != 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?varName?");
	return TCL_ERROR;
    }
    chanObjPtr = objv[1];
    if (TclGetChannelFromObj(interp, chanObjPtr, &chan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!(mode & TCL_READABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"channel \"%s\" wasn't opened for reading",
		TclGetString(chanObjPtr)));
	return TCL_ERROR;
    }

    TclChannelPreserve(chan);
    linePtr = Tcl_NewObj();
    lineLen = Tcl_GetsObj(chan, linePtr);
    if (lineLen < 0) {
	if (!Tcl_Eof(chan) && !Tcl_InputBlocked(chan)) {
	    Tcl_DecrRefCount(linePtr);

	    /*
	     * TIP #219.
	     * Capture error messages put by the driver into the bypass area
	     * and put them into the regular interpreter result. Fall back to
	     * the regular message if nothing was found in the bypass.
	     */

	    if (!TclChanCaughtErrorBypass(interp, chan)) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"error reading \"%s\": %s",
			TclGetString(chanObjPtr), Tcl_PosixError(interp)));
	    }
	    code = TCL_ERROR;
	    goto done;
	}
	lineLen = -1;
    }
    if (objc == 3) {
	if (Tcl_ObjSetVar2(interp, objv[2], NULL, linePtr,
		TCL_LEAVE_ERR_MSG) == NULL) {
	    code = TCL_ERROR;
	    goto done;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(lineLen));
    } else {
	Tcl_SetObjResult(interp, linePtr);
    }
  done:
    TclChannelRelease(chan);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ReadObjCmd --
 *
 *	This function is invoked to process the Tcl "read" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May consume input from channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ReadObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to read from. */
    int newline, i;		/* Discard newline at end? */
    int toRead;			/* How many bytes to read? */
    int charactersRead;		/* How many characters were read? */
    int mode;			/* Mode in which channel is opened. */
    Tcl_Obj *resultPtr, *chanObjPtr;

    if ((objc != 2) && (objc != 3)) {
	Interp *iPtr;

    argerror:
	iPtr = (Interp *) interp;
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?numChars?");

	/*
	 * Do not append directly; that makes ensembles using this command as
	 * a subcommand produce the wrong message.
	 */

	iPtr->flags |= INTERP_ALTERNATE_WRONG_ARGS;
	Tcl_WrongNumArgs(interp, 1, objv, "?-nonewline? channelId");
	return TCL_ERROR;
    }

    i = 1;
    newline = 0;
    if (strcmp(TclGetString(objv[1]), "-nonewline") == 0) {
	newline = 1;
	i++;
    }

    if (i == objc) {
	goto argerror;
    }

    chanObjPtr = objv[i];
    if (TclGetChannelFromObj(interp, chanObjPtr, &chan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!(mode & TCL_READABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"channel \"%s\" wasn't opened for reading",
		TclGetString(chanObjPtr)));
	return TCL_ERROR;
    }
    i++;			/* Consumed channel name. */

    /*
     * Compute how many bytes to read.
     */

    toRead = -1;
    if (i < objc) {
	if ((TclGetIntFromObj(interp, objv[i], &toRead) != TCL_OK)
		|| (toRead < 0)) {
#if TCL_MAJOR_VERSION < 9
	    /*
	     * The code below provides backwards compatibility with an old
	     * form of the command that is no longer recommended or
	     * documented. See also [Bug #3151675]. Will be removed in Tcl 9,
	     * maybe even earlier.
	     */

	    if (strcmp(TclGetString(objv[i]), "nonewline") != 0) {
#endif
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"expected non-negative integer but got \"%s\"",
			TclGetString(objv[i])));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", "NUMBER", NULL);
		return TCL_ERROR;
#if TCL_MAJOR_VERSION < 9
	    }
	    newline = 1;
#endif
	}
    }

    resultPtr = Tcl_NewObj();
    Tcl_IncrRefCount(resultPtr);
    TclChannelPreserve(chan);
    charactersRead = Tcl_ReadChars(chan, resultPtr, toRead, 0);
    if (charactersRead < 0) {
	/*
	 * TIP #219.
	 * Capture error messages put by the driver into the bypass area and
	 * put them into the regular interpreter result. Fall back to the
	 * regular message if nothing was found in the bypass.
	 */

	if (!TclChanCaughtErrorBypass(interp, chan)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error reading \"%s\": %s",
		    TclGetString(chanObjPtr), Tcl_PosixError(interp)));
	}
	TclChannelRelease(chan);
	Tcl_DecrRefCount(resultPtr);
	return TCL_ERROR;
    }

    /*
     * If requested, remove the last newline in the channel if at EOF.
     */

    if ((charactersRead > 0) && (newline != 0)) {
	const char *result;
	int length;

	result = TclGetStringFromObj(resultPtr, &length);
	if (result[length - 1] == '\n') {
	    Tcl_SetObjLength(resultPtr, length - 1);
	}
    }
    Tcl_SetObjResult(interp, resultPtr);
    TclChannelRelease(chan);
    Tcl_DecrRefCount(resultPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SeekObjCmd --
 *
 *	This function is invoked to process the Tcl "seek" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Moves the position of the access point on the specified channel.  May
 *	flush queued output.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_SeekObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to tell on. */
    Tcl_WideInt offset;		/* Where to seek? */
    int mode;			/* How to seek? */
    Tcl_WideInt result;		/* Of calling Tcl_Seek. */
    int optionIndex;
    static const char *const originOptions[] = {
	"start", "current", "end", NULL
    };
    static const int modeArray[] = {SEEK_SET, SEEK_CUR, SEEK_END};

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId offset ?origin?");
	return TCL_ERROR;
    }
    if (TclGetChannelFromObj(interp, objv[1], &chan, NULL, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetWideIntFromObj(interp, objv[2], &offset) != TCL_OK) {
	return TCL_ERROR;
    }
    mode = SEEK_SET;
    if (objc == 4) {
	if (Tcl_GetIndexFromObj(interp, objv[3], originOptions, "origin", 0,
		&optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	mode = modeArray[optionIndex];
    }

    TclChannelPreserve(chan);
    result = Tcl_Seek(chan, offset, mode);
    if (result == Tcl_LongAsWide(-1)) {
	/*
	 * TIP #219.
	 * Capture error messages put by the driver into the bypass area and
	 * put them into the regular interpreter result. Fall back to the
	 * regular message if nothing was found in the bypass.
	 */

	if (!TclChanCaughtErrorBypass(interp, chan)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error during seek on \"%s\": %s",
		    TclGetString(objv[1]), Tcl_PosixError(interp)));
	}
	TclChannelRelease(chan);
	return TCL_ERROR;
    }
    TclChannelRelease(chan);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TellObjCmd --
 *
 *	This function is invoked to process the Tcl "tell" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_TellObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to tell on. */
    Tcl_WideInt newLoc;
    int code;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }

    /*
     * Try to find a channel with the right name and permissions in the IO
     * channel table of this interpreter.
     */

    if (TclGetChannelFromObj(interp, objv[1], &chan, NULL, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    TclChannelPreserve(chan);
    newLoc = Tcl_Tell(chan);

    /*
     * TIP #219.
     * Capture error messages put by the driver into the bypass area and put
     * them into the regular interpreter result.
     */


    code  = TclChanCaughtErrorBypass(interp, chan);
    TclChannelRelease(chan);
    if (code) {
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(newLoc));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CloseObjCmd --
 *
 *	This function is invoked to process the Tcl "close" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May discard queued input; may flush queued output.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_CloseObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;		/* The channel to close. */
    static const char *const dirOptions[] = {
	"read", "write", NULL
    };
    static const int dirArray[] = {TCL_CLOSE_READ, TCL_CLOSE_WRITE};

    if ((objc != 2) && (objc != 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?direction?");
	return TCL_ERROR;
    }

    if (TclGetChannelFromObj(interp, objv[1], &chan, NULL, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	int index, dir;

	/*
	 * Get direction requested to close, and check syntax.
	 */

	if (Tcl_GetIndexFromObj(interp, objv[2], dirOptions, "direction", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	dir = dirArray[index];

	/*
	 * Check direction against channel mode. It is an error if we try to
	 * close a direction not supported by the channel (already closed, or
	 * never opened for that direction).
	 */

	if (!(dir & Tcl_GetChannelMode(chan))) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "Half-close of %s-side not possible, side not opened"
		    " or already closed", dirOptions[index]));
	    return TCL_ERROR;
	}

	/*
	 * Special handling is needed if and only if the channel mode supports
	 * more than the direction to close. Because if the close the last
	 * direction suppported we can and will go through the regular
	 * process.
	 */

	if ((Tcl_GetChannelMode(chan) &
		(TCL_CLOSE_READ|TCL_CLOSE_WRITE)) != dir) {
	    return Tcl_CloseEx(interp, chan, dir);
	}
    }

    if (Tcl_UnregisterChannel(interp, chan) != TCL_OK) {
	/*
	 * If there is an error message and it ends with a newline, remove the
	 * newline. This is done for command pipeline channels where the error
	 * output from the subprocesses is stored in interp's result.
	 *
	 * NOTE: This is likely to not have any effect on regular error
	 * messages produced by drivers during the closing of a channel,
	 * because the Tcl convention is that such error messages do not have
	 * a terminating newline.
	 */

	Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);
	const char *string;
	int len;

	if (Tcl_IsShared(resultPtr)) {
	    resultPtr = Tcl_DuplicateObj(resultPtr);
	    Tcl_SetObjResult(interp, resultPtr);
	}
	string = TclGetStringFromObj(resultPtr, &len);
	if ((len > 0) && (string[len - 1] == '\n')) {
	    Tcl_SetObjLength(resultPtr, len - 1);
	}
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FconfigureObjCmd --
 *
 *	This function is invoked to process the Tcl "fconfigure" command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May modify the behavior of an IO channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FconfigureObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *optionName, *valueName;
    Tcl_Channel chan;		/* The channel to set a mode on. */
    int i;			/* Iterate over arg-value pairs. */

    if ((objc < 2) || (((objc % 2) == 1) && (objc != 3))) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?-option value ...?");
	return TCL_ERROR;
    }

    if (TclGetChannelFromObj(interp, objv[1], &chan, NULL, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc == 2) {
	Tcl_DString ds;		/* DString to hold result of calling
				 * Tcl_GetChannelOption. */

	Tcl_DStringInit(&ds);
	if (Tcl_GetChannelOption(interp, chan, NULL, &ds) != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	Tcl_DStringResult(interp, &ds);
	return TCL_OK;
    } else if (objc == 3) {
	Tcl_DString ds;		/* DString to hold result of calling
				 * Tcl_GetChannelOption. */

	Tcl_DStringInit(&ds);
	optionName = TclGetString(objv[2]);
	if (Tcl_GetChannelOption(interp, chan, optionName, &ds) != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	Tcl_DStringResult(interp, &ds);
	return TCL_OK;
    }

    for (i = 3; i < objc; i += 2) {
	optionName = TclGetString(objv[i-1]);
	valueName = TclGetString(objv[i]);
	if (Tcl_SetChannelOption(interp, chan, optionName, valueName)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_EofObjCmd --
 *
 *	This function is invoked to process the Tcl "eof" command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Sets interp's result to boolean true or false depending on whether the
 *	specified channel has an EOF condition.
 *
 *---------------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_EofObjCmd(
    ClientData unused,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }

    if (TclGetChannelFromObj(interp, objv[1], &chan, NULL, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(Tcl_Eof(chan)));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ExecObjCmd --
 *
 *	This function is invoked to process the "exec" Tcl command. See the
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
Tcl_ExecObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *resultPtr;
    const char **argv;		/* An array for the string arguments. Stored
				 * on the _Tcl_ stack. */
    const char *string;
    Tcl_Channel chan;
    int argc, background, i, index, keepNewline, result, skip, length;
    int ignoreStderr;
    static const char *const options[] = {
	"-ignorestderr", "-keepnewline", "--", NULL
    };
    enum options {
	EXEC_IGNORESTDERR, EXEC_KEEPNEWLINE, EXEC_LAST
    };

    /*
     * Check for any leading option arguments.
     */

    keepNewline = 0;
    ignoreStderr = 0;
    for (skip = 1; skip < objc; skip++) {
	string = TclGetString(objv[skip]);
	if (string[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[skip], options, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (index == EXEC_KEEPNEWLINE) {
	    keepNewline = 1;
	} else if (index == EXEC_IGNORESTDERR) {
	    ignoreStderr = 1;
	} else {
	    skip++;
	    break;
	}
    }
    if (objc <= skip) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-option ...? arg ?arg ...?");
	return TCL_ERROR;
    }

    /*
     * See if the command is to be run in background.
     */

    background = 0;
    string = TclGetString(objv[objc - 1]);
    if ((string[0] == '&') && (string[1] == '\0')) {
	objc--;
	background = 1;
    }

    /*
     * Create the string argument array "argv". Make sure argv is large enough
     * to hold the argc arguments plus 1 extra for the zero end-of-argv word.
     */

    argc = objc - skip;
    argv = TclStackAlloc(interp, (unsigned)(argc + 1) * sizeof(char *));

    /*
     * Copy the string conversions of each (post option) object into the
     * argument vector.
     */

    for (i = 0; i < argc; i++) {
	argv[i] = TclGetString(objv[i + skip]);
    }
    argv[argc] = NULL;
    chan = Tcl_OpenCommandChannel(interp, argc, argv, (background ? 0 :
	    ignoreStderr ? TCL_STDOUT : TCL_STDOUT|TCL_STDERR));

    /*
     * Free the argv array.
     */

    TclStackFree(interp, (void *) argv);

    if (chan == NULL) {
	return TCL_ERROR;
    }

    if (background) {
	/*
	 * Store the list of PIDs from the pipeline in interp's result and
	 * detach the PIDs (instead of waiting for them).
	 */

	TclGetAndDetachPids(interp, chan);
	if (Tcl_Close(interp, chan) != TCL_OK) {
	    return TCL_ERROR;
	}
	return TCL_OK;
    }

    resultPtr = Tcl_NewObj();
    if (Tcl_GetChannelHandle(chan, TCL_READABLE, NULL) == TCL_OK) {
	if (Tcl_ReadChars(chan, resultPtr, -1, 0) < 0) {
	    /*
	     * TIP #219.
	     * Capture error messages put by the driver into the bypass area
	     * and put them into the regular interpreter result. Fall back to
	     * the regular message if nothing was found in the bypass.
	     */

	    if (!TclChanCaughtErrorBypass(interp, chan)) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"error reading output from command: %s",
			Tcl_PosixError(interp)));
		Tcl_DecrRefCount(resultPtr);
	    }
	    return TCL_ERROR;
	}
    }

    /*
     * If the process produced anything on stderr, it will have been returned
     * in the interpreter result. It needs to be appended to the result
     * string.
     */

    result = Tcl_Close(interp, chan);
    Tcl_AppendObjToObj(resultPtr, Tcl_GetObjResult(interp));

    /*
     * If the last character of the result is a newline, then remove the
     * newline character.
     */

    if (keepNewline == 0) {
	string = TclGetStringFromObj(resultPtr, &length);
	if ((length > 0) && (string[length - 1] == '\n')) {
	    Tcl_SetObjLength(resultPtr, length - 1);
	}
    }
    Tcl_SetObjResult(interp, resultPtr);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FblockedObjCmd --
 *
 *	This function is invoked to process the Tcl "fblocked" command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Sets interp's result to boolean true or false depending on whether the
 *	preceeding input operation on the channel would have blocked.
 *
 *---------------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FblockedObjCmd(
    ClientData unused,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;
    int mode;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId");
	return TCL_ERROR;
    }

    if (TclGetChannelFromObj(interp, objv[1], &chan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!(mode & TCL_READABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"channel \"%s\" wasn't opened for reading",
		TclGetString(objv[1])));
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(Tcl_InputBlocked(chan)));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_OpenObjCmd --
 *
 *	This function is invoked to process the "open" Tcl command. See the
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
Tcl_OpenObjCmd(
    ClientData notUsed,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int pipeline, prot;
    const char *modeString, *what;
    Tcl_Channel chan;

    if ((objc < 2) || (objc > 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "fileName ?access? ?permissions?");
	return TCL_ERROR;
    }
    prot = 0666;
    if (objc == 2) {
	modeString = "r";
    } else {
	modeString = TclGetString(objv[2]);
	if (objc == 4) {
	    const char *permString = TclGetString(objv[3]);
	    int code = TCL_ERROR;
	    int scanned = TclParseAllWhiteSpace(permString, -1);

	    /*
	     * Support legacy octal numbers.
	     */

	    if ((permString[scanned] == '0')
		    && (permString[scanned+1] >= '0')
		    && (permString[scanned+1] <= '7')) {
		Tcl_Obj *permObj;

		TclNewLiteralStringObj(permObj, "0o");
		Tcl_AppendToObj(permObj, permString+scanned+1, -1);
		code = TclGetIntFromObj(NULL, permObj, &prot);
		Tcl_DecrRefCount(permObj);
	    }

	    if ((code == TCL_ERROR)
		    && TclGetIntFromObj(interp, objv[3], &prot) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }

    pipeline = 0;
    what = TclGetString(objv[1]);
    if (what[0] == '|') {
	pipeline = 1;
    }

    /*
     * Open the file or create a process pipeline.
     */

    if (!pipeline) {
	chan = Tcl_FSOpenFileChannel(interp, objv[1], modeString, prot);
    } else {
	int mode, seekFlag, cmdObjc, binary;
	const char **cmdArgv;

	if (Tcl_SplitList(interp, what+1, &cmdObjc, &cmdArgv) != TCL_OK) {
	    return TCL_ERROR;
	}

	mode = TclGetOpenModeEx(interp, modeString, &seekFlag, &binary);
	if (mode == -1) {
	    chan = NULL;
	} else {
	    int flags = TCL_STDERR | TCL_ENFORCE_MODE;

	    switch (mode & (O_RDONLY | O_WRONLY | O_RDWR)) {
	    case O_RDONLY:
		flags |= TCL_STDOUT;
		break;
	    case O_WRONLY:
		flags |= TCL_STDIN;
		break;
	    case O_RDWR:
		flags |= (TCL_STDIN | TCL_STDOUT);
		break;
	    default:
		Tcl_Panic("Tcl_OpenCmd: invalid mode value");
		break;
	    }
	    chan = Tcl_OpenCommandChannel(interp, cmdObjc, cmdArgv, flags);
	    if (binary && chan) {
		Tcl_SetChannelOption(interp, chan, "-translation", "binary");
	    }
	}
	ckfree(cmdArgv);
    }
    if (chan == NULL) {
	return TCL_ERROR;
    }
    Tcl_RegisterChannel(interp, chan);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_GetChannelName(chan), -1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TcpAcceptCallbacksDeleteProc --
 *
 *	Assocdata cleanup routine called when an interpreter is being deleted
 *	to set the interp field of all the accept callback records registered
 *	with the interpreter to NULL. This will prevent the interpreter from
 *	being used in the future to eval accept scripts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates memory and sets the interp field of all the accept
 *	callback records to NULL to prevent this interpreter from being used
 *	subsequently to eval accept scripts.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
TcpAcceptCallbacksDeleteProc(
    ClientData clientData,	/* Data which was passed when the assocdata
				 * was registered. */
    Tcl_Interp *interp)		/* Interpreter being deleted - not used. */
{
    Tcl_HashTable *hTblPtr = clientData;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch hSearch;

    for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&hSearch)) {
	AcceptCallback *acceptCallbackPtr = Tcl_GetHashValue(hPtr);

	acceptCallbackPtr->interp = NULL;
    }
    Tcl_DeleteHashTable(hTblPtr);
    ckfree(hTblPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RegisterTcpServerInterpCleanup --
 *
 *	Registers an accept callback record to have its interp field set to
 *	NULL when the interpreter is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When, in the future, the interpreter is deleted, the interp field of
 *	the accept callback data structure will be set to NULL. This will
 *	prevent attempts to eval the accept script in a deleted interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
RegisterTcpServerInterpCleanup(
    Tcl_Interp *interp,		/* Interpreter for which we want to be
				 * informed of deletion. */
    AcceptCallback *acceptCallbackPtr)
				/* The accept callback record whose interp
				 * field we want set to NULL when the
				 * interpreter is deleted. */
{
    Tcl_HashTable *hTblPtr;	/* Hash table for accept callback records to
				 * smash when the interpreter will be
				 * deleted. */
    Tcl_HashEntry *hPtr;	/* Entry for this record. */
    int isNew;			/* Is the entry new? */

    hTblPtr = Tcl_GetAssocData(interp, "tclTCPAcceptCallbacks", NULL);

    if (hTblPtr == NULL) {
	hTblPtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(hTblPtr, TCL_ONE_WORD_KEYS);
	Tcl_SetAssocData(interp, "tclTCPAcceptCallbacks",
		TcpAcceptCallbacksDeleteProc, hTblPtr);
    }

    hPtr = Tcl_CreateHashEntry(hTblPtr, acceptCallbackPtr, &isNew);
    if (!isNew) {
	Tcl_Panic("RegisterTcpServerCleanup: damaged accept record table");
    }
    Tcl_SetHashValue(hPtr, acceptCallbackPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * UnregisterTcpServerInterpCleanupProc --
 *
 *	Unregister a previously registered accept callback record. The interp
 *	field of this record will no longer be set to NULL in the future when
 *	the interpreter is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prevents the interp field of the accept callback record from being set
 *	to NULL in the future when the interpreter is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
UnregisterTcpServerInterpCleanupProc(
    Tcl_Interp *interp,		/* Interpreter in which the accept callback
				 * record was registered. */
    AcceptCallback *acceptCallbackPtr)
				/* The record for which to delete the
				 * registration. */
{
    Tcl_HashTable *hTblPtr;
    Tcl_HashEntry *hPtr;

    hTblPtr = Tcl_GetAssocData(interp, "tclTCPAcceptCallbacks", NULL);
    if (hTblPtr == NULL) {
	return;
    }

    hPtr = Tcl_FindHashEntry(hTblPtr, (char *) acceptCallbackPtr);
    if (hPtr != NULL) {
	Tcl_DeleteHashEntry(hPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AcceptCallbackProc --
 *
 *	This callback is invoked by the TCP channel driver when it accepts a
 *	new connection from a client on a server socket.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the script does.
 *
 *----------------------------------------------------------------------
 */

static void
AcceptCallbackProc(
    ClientData callbackData,	/* The data stored when the callback was
				 * created in the call to
				 * Tcl_OpenTcpServer. */
    Tcl_Channel chan,		/* Channel for the newly accepted
				 * connection. */
    char *address,		/* Address of client that was accepted. */
    int port)			/* Port of client that was accepted. */
{
    AcceptCallback *acceptCallbackPtr = callbackData;

    /*
     * Check if the callback is still valid; the interpreter may have gone
     * away, this is signalled by setting the interp field of the callback
     * data to NULL.
     */

    if (acceptCallbackPtr->interp != NULL) {
	char portBuf[TCL_INTEGER_SPACE];
	char *script = acceptCallbackPtr->script;
	Tcl_Interp *interp = acceptCallbackPtr->interp;
	int result;

	Tcl_Preserve(script);
	Tcl_Preserve(interp);

	TclFormatInt(portBuf, port);
	Tcl_RegisterChannel(interp, chan);

	/*
	 * Artificially bump the refcount to protect the channel from being
	 * deleted while the script is being evaluated.
	 */

	Tcl_RegisterChannel(NULL, chan);

	result = Tcl_VarEval(interp, script, " ", Tcl_GetChannelName(chan),
		" ", address, " ", portBuf, NULL);
	if (result != TCL_OK) {
	    Tcl_BackgroundException(interp, result);
	    Tcl_UnregisterChannel(interp, chan);
	}

	/*
	 * Decrement the artificially bumped refcount. After this it is not
	 * safe anymore to use "chan", because it may now be deleted.
	 */

	Tcl_UnregisterChannel(NULL, chan);

	Tcl_Release(interp);
	Tcl_Release(script);
    } else {
	/*
	 * The interpreter has been deleted, so there is no useful way to use
	 * the client socket - just close it.
	 */

	Tcl_Close(NULL, chan);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TcpServerCloseProc --
 *
 *	This callback is called when the TCP server channel for which it was
 *	registered is being closed. It informs the interpreter in which the
 *	accept script is evaluated (if that interpreter still exists) that
 *	this channel no longer needs to be informed if the interpreter is
 *	deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	In the future, if the interpreter is deleted this channel will no
 *	longer be informed.
 *
 *----------------------------------------------------------------------
 */

static void
TcpServerCloseProc(
    ClientData callbackData)	/* The data passed in the call to
				 * Tcl_CreateCloseHandler. */
{
    AcceptCallback *acceptCallbackPtr = callbackData;
				/* The actual data. */

    if (acceptCallbackPtr->interp != NULL) {
	UnregisterTcpServerInterpCleanupProc(acceptCallbackPtr->interp,
		acceptCallbackPtr);
    }
    Tcl_EventuallyFree(acceptCallbackPtr->script, TCL_DYNAMIC);
    ckfree(acceptCallbackPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SocketObjCmd --
 *
 *	This function is invoked to process the "socket" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates a socket based channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SocketObjCmd(
    ClientData notUsed,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const socketOptions[] = {
	"-async", "-myaddr", "-myport", "-server", NULL
    };
    enum socketOptions {
	SKT_ASYNC, SKT_MYADDR, SKT_MYPORT, SKT_SERVER
    };
    int optionIndex, a, server = 0, port, myport = 0, async = 0;
    const char *host, *script = NULL, *myaddr = NULL;
    Tcl_Channel chan;

    if (TclpHasSockets(interp) != TCL_OK) {
	return TCL_ERROR;
    }

    for (a = 1; a < objc; a++) {
	const char *arg = Tcl_GetString(objv[a]);

	if (arg[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[a], socketOptions, "option",
		TCL_EXACT, &optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum socketOptions) optionIndex) {
	case SKT_ASYNC:
	    if (server == 1) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"cannot set -async option for server sockets", -1));
		return TCL_ERROR;
	    }
	    async = 1;
	    break;
	case SKT_MYADDR:
	    a++;
	    if (a >= objc) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"no argument given for -myaddr option", -1));
		return TCL_ERROR;
	    }
	    myaddr = TclGetString(objv[a]);
	    break;
	case SKT_MYPORT: {
	    const char *myPortName;

	    a++;
	    if (a >= objc) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"no argument given for -myport option", -1));
		return TCL_ERROR;
	    }
	    myPortName = TclGetString(objv[a]);
	    if (TclSockGetPort(interp, myPortName, "tcp", &myport) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	}
	case SKT_SERVER:
	    if (async == 1) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"cannot set -async option for server sockets", -1));
		return TCL_ERROR;
	    }
	    server = 1;
	    a++;
	    if (a >= objc) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"no argument given for -server option", -1));
		return TCL_ERROR;
	    }
	    script = TclGetString(objv[a]);
	    break;
	default:
	    Tcl_Panic("Tcl_SocketObjCmd: bad option index to SocketOptions");
	}
    }
    if (server) {
	host = myaddr;		/* NULL implies INADDR_ANY */
	if (myport != 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "option -myport is not valid for servers", -1));
	    return TCL_ERROR;
	}
    } else if (a < objc) {
	host = TclGetString(objv[a]);
	a++;
    } else {
	Interp *iPtr;

    wrongNumArgs:
	iPtr = (Interp *) interp;
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-myaddr addr? ?-myport myport? ?-async? host port");
	iPtr->flags |= INTERP_ALTERNATE_WRONG_ARGS;
	Tcl_WrongNumArgs(interp, 1, objv,
		"-server command ?-myaddr addr? port");
	return TCL_ERROR;
    }

    if (a == objc-1) {
	if (TclSockGetPort(interp, TclGetString(objv[a]), "tcp",
		&port) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	goto wrongNumArgs;
    }

    if (server) {
	AcceptCallback *acceptCallbackPtr =
		ckalloc(sizeof(AcceptCallback));
	unsigned len = strlen(script) + 1;
	char *copyScript = ckalloc(len);

	memcpy(copyScript, script, len);
	acceptCallbackPtr->script = copyScript;
	acceptCallbackPtr->interp = interp;
	chan = Tcl_OpenTcpServer(interp, port, host, AcceptCallbackProc,
		acceptCallbackPtr);
	if (chan == NULL) {
	    ckfree(copyScript);
	    ckfree(acceptCallbackPtr);
	    return TCL_ERROR;
	}

	/*
	 * Register with the interpreter to let us know when the interpreter
	 * is deleted (by having the callback set the interp field of the
	 * acceptCallbackPtr's structure to NULL). This is to avoid trying to
	 * eval the script in a deleted interpreter.
	 */

	RegisterTcpServerInterpCleanup(interp, acceptCallbackPtr);

	/*
	 * Register a close callback. This callback will inform the
	 * interpreter (if it still exists) that this channel does not need to
	 * be informed when the interpreter is deleted.
	 */

	Tcl_CreateCloseHandler(chan, TcpServerCloseProc, acceptCallbackPtr);
    } else {
	chan = Tcl_OpenTcpClient(interp, port, host, myaddr, myport, async);
	if (chan == NULL) {
	    return TCL_ERROR;
	}
    }

    Tcl_RegisterChannel(interp, chan);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_GetChannelName(chan), -1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FcopyObjCmd --
 *
 *	This function is invoked to process the "fcopy" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Moves data between two channels and possibly sets up a background copy
 *	handler.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_FcopyObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel inChan, outChan;
    int mode, i, index;
    Tcl_WideInt toRead;
    Tcl_Obj *cmdPtr;
    static const char *const switches[] = { "-size", "-command", NULL };
    enum { FcopySize, FcopyCommand };

    if ((objc < 3) || (objc > 7) || (objc == 4) || (objc == 6)) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"input output ?-size size? ?-command callback?");
	return TCL_ERROR;
    }

    /*
     * Parse the channel arguments and verify that they are readable or
     * writable, as appropriate.
     */

    if (TclGetChannelFromObj(interp, objv[1], &inChan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!(mode & TCL_READABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"channel \"%s\" wasn't opened for reading",
		TclGetString(objv[1])));
	return TCL_ERROR;
    }
    if (TclGetChannelFromObj(interp, objv[2], &outChan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!(mode & TCL_WRITABLE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"channel \"%s\" wasn't opened for writing",
		TclGetString(objv[2])));
	return TCL_ERROR;
    }

    toRead = -1;
    cmdPtr = NULL;
    for (i = 3; i < objc; i += 2) {
	if (Tcl_GetIndexFromObj(interp, objv[i], switches, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch (index) {
	case FcopySize:
	    if (Tcl_GetWideIntFromObj(interp, objv[i+1], &toRead) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (toRead < 0) {
		/*
		 * Handle all negative sizes like -1, meaning 'copy all'. By
		 * resetting toRead we avoid changes in the core copying
		 * functions (which explicitly check for -1 and crash on any
		 * other negative value).
		 */

		toRead = -1;
	    }
	    break;
	case FcopyCommand:
	    cmdPtr = objv[i+1];
	    break;
	}
    }

    return TclCopyChannel(interp, inChan, outChan, toRead, cmdPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ChanPendingObjCmd --
 *
 *	This function is invoked to process the Tcl "chan pending" command
 *	(TIP #287). See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Sets interp's result to the number of bytes of buffered input or
 *	output (depending on whether the first argument is "input" or
 *	"output"), or -1 if the channel wasn't opened for that mode.
 *
 *---------------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ChanPendingObjCmd(
    ClientData unused,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;
    int index, mode;
    static const char *const options[] = {"input", "output", NULL};
    enum options {PENDING_INPUT, PENDING_OUTPUT};

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "mode channelId");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "mode", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (TclGetChannelFromObj(interp, objv[2], &chan, &mode, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case PENDING_INPUT:
	if (!(mode & TCL_READABLE)) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(-1));
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(Tcl_InputBuffered(chan)));
	}
	break;
    case PENDING_OUTPUT:
	if (!(mode & TCL_WRITABLE)) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(-1));
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(Tcl_OutputBuffered(chan)));
	}
	break;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ChanTruncateObjCmd --
 *
 *	This function is invoked to process the "chan truncate" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Truncates a channel (or rather a file underlying a channel).
 *
 *----------------------------------------------------------------------
 */

static int
ChanTruncateObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel chan;
    Tcl_WideInt length;

    if ((objc < 2) || (objc > 3)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId ?length?");
	return TCL_ERROR;
    }
    if (TclGetChannelFromObj(interp, objv[1], &chan, NULL, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	/*
	 * User is supplying an explicit length.
	 */

	if (Tcl_GetWideIntFromObj(interp, objv[2], &length) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (length < 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "cannot truncate to negative length of file", -1));
	    return TCL_ERROR;
	}
    } else {
	/*
	 * User wants to truncate to the current file position.
	 */

	length = Tcl_Tell(chan);
	if (length == Tcl_WideAsLong(-1)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "could not determine current location in \"%s\": %s",
		    TclGetString(objv[1]), Tcl_PosixError(interp)));
	    return TCL_ERROR;
	}
    }

    if (Tcl_TruncateChannel(chan, length) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"error during truncate on \"%s\": %s",
		TclGetString(objv[1]), Tcl_PosixError(interp)));
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ChanPipeObjCmd --
 *
 *	This function is invoked to process the "chan pipe" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates a pair of Tcl channels wrapping both ends of a new
 *	anonymous pipe.
 *
 *----------------------------------------------------------------------
 */

static int
ChanPipeObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Channel rchan, wchan;
    const char *channelNames[2];
    Tcl_Obj *resultPtr;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "");
	return TCL_ERROR;
    }

    if (Tcl_CreatePipe(interp, &rchan, &wchan, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    channelNames[0] = Tcl_GetChannelName(rchan);
    channelNames[1] = Tcl_GetChannelName(wchan);

    resultPtr = Tcl_NewObj();
    Tcl_ListObjAppendElement(NULL, resultPtr,
	    Tcl_NewStringObj(channelNames[0], -1));
    Tcl_ListObjAppendElement(NULL, resultPtr,
	    Tcl_NewStringObj(channelNames[1], -1));
    Tcl_SetObjResult(interp, resultPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclChannelNamesCmd --
 *
 *	This function is invoked to process the "chan names" and "file
 *	channels" Tcl commands.  See the user documentation for details on
 *	what they do.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclChannelNamesCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    if (objc < 1 || objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?pattern?");
	return TCL_ERROR;
    }
    return Tcl_GetChannelNamesEx(interp,
	    ((objc == 1) ? NULL : TclGetString(objv[1])));
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitChanCmd --
 *
 *	This function is invoked to create the "chan" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A Tcl command handle.
 *
 * Side effects:
 *	None (since nothing is byte-compiled).
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
TclInitChanCmd(
    Tcl_Interp *interp)
{
    /*
     * Most commands are plugged directly together, but some are done via
     * alias-like rewriting; [chan configure] is this way for security reasons
     * (want overwriting of [fconfigure] to control that nicely), and [chan
     * names] because the functionality isn't available as a separate command
     * function at the moment.
     */
    static const EnsembleImplMap initMap[] = {
	{"blocked",	Tcl_FblockedObjCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"close",	Tcl_CloseObjCmd,	TclCompileBasic1Or2ArgCmd, NULL, NULL, 0},
	{"copy",	Tcl_FcopyObjCmd,	NULL, NULL, NULL, 0},
	{"create",	TclChanCreateObjCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},		/* TIP #219 */
	{"eof",		Tcl_EofObjCmd,		TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"event",	Tcl_FileEventObjCmd,	TclCompileBasic2Or3ArgCmd, NULL, NULL, 0},
	{"flush",	Tcl_FlushObjCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"gets",	Tcl_GetsObjCmd,		TclCompileBasic1Or2ArgCmd, NULL, NULL, 0},
	{"names",	TclChannelNamesCmd,	TclCompileBasic0Or1ArgCmd, NULL, NULL, 0},
	{"pending",	ChanPendingObjCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},		/* TIP #287 */
	{"pipe",	ChanPipeObjCmd,		TclCompileBasic0ArgCmd, NULL, NULL, 0},		/* TIP #304 */
	{"pop",		TclChanPopObjCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},		/* TIP #230 */
	{"postevent",	TclChanPostEventObjCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},	/* TIP #219 */
	{"push",	TclChanPushObjCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},		/* TIP #230 */
	{"puts",	Tcl_PutsObjCmd,		NULL, NULL, NULL, 0},
	{"read",	Tcl_ReadObjCmd,		NULL, NULL, NULL, 0},
	{"seek",	Tcl_SeekObjCmd,		TclCompileBasic2Or3ArgCmd, NULL, NULL, 0},
	{"tell",	Tcl_TellObjCmd,		TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"truncate",	ChanTruncateObjCmd,	TclCompileBasic1Or2ArgCmd, NULL, NULL, 0},		/* TIP #208 */
	{NULL, NULL, NULL, NULL, NULL, 0}
    };
    static const char *const extras[] = {
	"configure",	"::fconfigure",
	NULL
    };
    Tcl_Command ensemble;
    Tcl_Obj *mapObj;
    int i;

    ensemble = TclMakeEnsemble(interp, "chan", initMap);
    Tcl_GetEnsembleMappingDict(NULL, ensemble, &mapObj);
    for (i=0 ; extras[i] ; i+=2) {
	/*
	 * Can assume that reference counts are all incremented.
	 */

	Tcl_DictObjPut(NULL, mapObj, Tcl_NewStringObj(extras[i], -1),
		Tcl_NewStringObj(extras[i+1], -1));
    }
    Tcl_SetEnsembleMappingDict(interp, ensemble, mapObj);
    return ensemble;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
