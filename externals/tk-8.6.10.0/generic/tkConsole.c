/*
 * tkConsole.c --
 *
 *	This file implements a Tcl console for systems that may not otherwise
 *	have access to a console. It uses the Text widget and provides special
 *	access via a console command.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * Each console is associated with an instance of the ConsoleInfo struct.
 * It keeps track of what interp holds the Tk application that displays
 * the console, and what interp is controlled by the interactions in that
 * console.  A refCount permits the struct to be shared as instance data
 * by commands and by channels.
 */

typedef struct ConsoleInfo {
    Tcl_Interp *consoleInterp;	/* Interpreter displaying the console. */
    Tcl_Interp *interp;		/* Interpreter controlled by console. */
    int refCount;
} ConsoleInfo;

/*
 * Each console channel holds an instance of the ChannelData struct as
 * its instance data.  It contains ConsoleInfo, so the channel can work
 * with the appropriate console window, and a type value to distinguish
 * the stdout channel from the stderr channel.
 */

typedef struct ChannelData {
    ConsoleInfo *info;
    int type;			/* TCL_STDOUT or TCL_STDERR */
} ChannelData;

/*
 * Prototypes for local procedures defined in this file:
 */

static int	ConsoleClose(ClientData instanceData, Tcl_Interp *interp);
static void	ConsoleDeleteProc(ClientData clientData);
static void	ConsoleEventProc(ClientData clientData, XEvent *eventPtr);
static int	ConsoleHandle(ClientData instanceData, int direction,
		    ClientData *handlePtr);
static int	ConsoleInput(ClientData instanceData, char *buf, int toRead,
		    int *errorCode);
static int	ConsoleObjCmd(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *const objv[]);
static int	ConsoleOutput(ClientData instanceData, const char *buf,
		    int toWrite, int *errorCode);
static void	ConsoleWatch(ClientData instanceData, int mask);
static void	DeleteConsoleInterp(ClientData clientData);
static void	InterpDeleteProc(ClientData clientData, Tcl_Interp *interp);
static int	InterpreterObjCmd(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *const objv[]);

/*
 * This structure describes the channel type structure for file based IO:
 */

static const Tcl_ChannelType consoleChannelType = {
    "console",			/* Type name. */
    TCL_CHANNEL_VERSION_4,	/* v4 channel */
    ConsoleClose,		/* Close proc. */
    ConsoleInput,		/* Input proc. */
    ConsoleOutput,		/* Output proc. */
    NULL,			/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    ConsoleWatch,		/* Watch for events on console. */
    ConsoleHandle,		/* Get a handle from the device. */
    NULL,			/* close2proc. */
    NULL,			/* Always non-blocking.*/
    NULL,			/* flush proc. */
    NULL,			/* handler proc. */
    NULL,			/* wide seek proc */
    NULL,			/* thread action proc */
    NULL
};

#ifdef _WIN32
#include <windows.h>

/*
 *----------------------------------------------------------------------
 *
 * ShouldUseConsoleChannel
 *
 *	Check to see if console window should be used for a given standard
 *	channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the console channel and installs it as the standard channels.
 *
 *----------------------------------------------------------------------
 */

static int
ShouldUseConsoleChannel(
    int type)
{
    DWORD handleId;		/* Standard handle to retrieve. */
    DCB dcb;
    DWORD consoleParams;
    DWORD fileType;
    HANDLE handle;

    switch (type) {
    case TCL_STDIN:
	handleId = STD_INPUT_HANDLE;
	break;
    case TCL_STDOUT:
	handleId = STD_OUTPUT_HANDLE;
	break;
    case TCL_STDERR:
	handleId = STD_ERROR_HANDLE;
	break;
    default:
	return 0;
	break;
    }

    handle = GetStdHandle(handleId);

    /*
     * Note that we need to check for 0 because Windows will return 0 if this
     * is not a console mode application, even though this is not a valid
     * handle.
     */

    if ((handle == INVALID_HANDLE_VALUE) || (handle == 0)) {
	return 1;
    }

    /*
     * Win2K BUG: GetStdHandle(STD_OUTPUT_HANDLE) can return what appears to
     * be a valid handle. See TclpGetDefaultStdChannel() for this change
     * implemented. We didn't change it here because GetFileType() [below]
     * will catch this with FILE_TYPE_UNKNOWN and appropriately return a value
     * of 1, anyways.
     *
     *    char dummyBuff[1];
     *    DWORD dummyWritten;
     *
     *    if ((type == TCL_STDOUT)
     *		&& !WriteFile(handle, dummyBuff, 0, &dummyWritten, NULL)) {
     *	     return 1;
     *    }
     */

    fileType = GetFileType(handle);

    /*
     * If the file is a character device, we need to try to figure out whether
     * it is a serial port, a console, or something else. We test for the
     * console case first because this is more common.
     */

    if (fileType == FILE_TYPE_CHAR) {
	dcb.DCBlength = sizeof(DCB);
	if (!GetConsoleMode(handle, &consoleParams) &&
		!GetCommState(handle, &dcb)) {
	    /*
	     * Don't use a CHAR type channel for stdio, otherwise Tk runs into
	     * trouble with the MS DevStudio debugger.
	     */

	    return 1;
	}
    } else if (fileType == FILE_TYPE_UNKNOWN) {
	return 1;
    } else if (Tcl_GetStdChannel(type) == NULL) {
	return 1;
    }

    return 0;
}
#else
/*
 * Mac should always use a console channel, Unix should if it's trying to
 */

#define ShouldUseConsoleChannel(chan) (1)
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tk_InitConsoleChannels --
 *
 * 	Create the console channels and install them as the standard channels.
 * 	All I/O will be discarded until Tk_CreateConsoleWindow is called to
 * 	attach the console to a text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the console channel and installs it as the standard channels.
 *
 *----------------------------------------------------------------------
 */

void
Tk_InitConsoleChannels(
    Tcl_Interp *interp)
{
    static Tcl_ThreadDataKey consoleInitKey;
    int *consoleInitPtr, doIn, doOut, doErr;
    ConsoleInfo *info;
    Tcl_Channel consoleChannel;

    /*
     * Ensure that we are getting a compatible version of Tcl.
     */

    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
        return;
    }

    consoleInitPtr = Tcl_GetThreadData(&consoleInitKey, (int) sizeof(int));
    if (*consoleInitPtr) {
	/*
	 * We've already initialized console channels in this thread.
	 */

	return;
    }
    *consoleInitPtr = 1;

    doIn = ShouldUseConsoleChannel(TCL_STDIN);
    doOut = ShouldUseConsoleChannel(TCL_STDOUT);
    doErr = ShouldUseConsoleChannel(TCL_STDERR);

    if (!(doIn || doOut || doErr)) {
	/*
	 * No std channels should be tied to the console; thus, no need to
	 * create the console.
	 */

	return;
    }

    /*
     * At least one std channel wants to be tied to the console, so create the
     * interp for it to live in.
     */

    info = ckalloc(sizeof(ConsoleInfo));
    info->consoleInterp = NULL;
    info->interp = NULL;
    info->refCount = 0;

    if (doIn) {
	ChannelData *data = ckalloc(sizeof(ChannelData));

	data->info = info;
	data->info->refCount++;
	data->type = TCL_STDIN;
	consoleChannel = Tcl_CreateChannel(&consoleChannelType, "console0",
		data, TCL_READABLE);
	if (consoleChannel != NULL) {
	    Tcl_SetChannelOption(NULL, consoleChannel, "-translation", "lf");
	    Tcl_SetChannelOption(NULL, consoleChannel, "-buffering", "none");
	    Tcl_SetChannelOption(NULL, consoleChannel, "-encoding", "utf-8");
	}
	Tcl_SetStdChannel(consoleChannel, TCL_STDIN);
	Tcl_RegisterChannel(NULL, consoleChannel);
    }

    if (doOut) {
	ChannelData *data = ckalloc(sizeof(ChannelData));

	data->info = info;
	data->info->refCount++;
	data->type = TCL_STDOUT;
	consoleChannel = Tcl_CreateChannel(&consoleChannelType, "console1",
		data, TCL_WRITABLE);
	if (consoleChannel != NULL) {
	    Tcl_SetChannelOption(NULL, consoleChannel, "-translation", "lf");
	    Tcl_SetChannelOption(NULL, consoleChannel, "-buffering", "none");
	    Tcl_SetChannelOption(NULL, consoleChannel, "-encoding", "utf-8");
	}
	Tcl_SetStdChannel(consoleChannel, TCL_STDOUT);
	Tcl_RegisterChannel(NULL, consoleChannel);
    }

    if (doErr) {
	ChannelData *data = ckalloc(sizeof(ChannelData));

	data->info = info;
	data->info->refCount++;
	data->type = TCL_STDERR;
	consoleChannel = Tcl_CreateChannel(&consoleChannelType, "console2",
		data, TCL_WRITABLE);
	if (consoleChannel != NULL) {
	    Tcl_SetChannelOption(NULL, consoleChannel, "-translation", "lf");
	    Tcl_SetChannelOption(NULL, consoleChannel, "-buffering", "none");
	    Tcl_SetChannelOption(NULL, consoleChannel, "-encoding", "utf-8");
	}
	Tcl_SetStdChannel(consoleChannel, TCL_STDERR);
	Tcl_RegisterChannel(NULL, consoleChannel);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateConsoleWindow --
 *
 *	Initialize the console. This code actually creates a new application
 *	and associated interpreter. This effectively hides the implementation
 *	from the main application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new console it created.
 *
 *----------------------------------------------------------------------
 */

int
Tk_CreateConsoleWindow(
    Tcl_Interp *interp)		/* Interpreter to use for prompting. */
{
    Tcl_Channel chan;
    ConsoleInfo *info;
    Tk_Window mainWindow;
    Tcl_Command token;
    int result = TCL_OK;
    int haveConsoleChannel = 1;

    /* Init an interp with Tcl and Tk */
    Tcl_Interp *consoleInterp = Tcl_CreateInterp();
    if (Tcl_Init(consoleInterp) != TCL_OK) {
	Tcl_Obj *result_obj = Tcl_GetObjResult(consoleInterp);
	Tcl_SetObjResult(interp, result_obj);
	goto error;
    }
    if (Tk_Init(consoleInterp) != TCL_OK) {
	Tcl_Obj *result_obj = Tcl_GetObjResult(consoleInterp);
	Tcl_SetObjResult(interp, result_obj);
	goto error;
    }

    /*
     * Fetch the instance data from whatever std channel is a
     * console channel.  If none, create fresh instance data.
     */

    if (Tcl_GetChannelType(chan = Tcl_GetStdChannel(TCL_STDIN))
	    == &consoleChannelType) {
    } else if (Tcl_GetChannelType(chan = Tcl_GetStdChannel(TCL_STDOUT))
	    == &consoleChannelType) {
    } else if (Tcl_GetChannelType(chan = Tcl_GetStdChannel(TCL_STDERR))
	    == &consoleChannelType) {
    } else {
	haveConsoleChannel = 0;
    }

    if (haveConsoleChannel) {
	ChannelData *data = (ChannelData *) Tcl_GetChannelInstanceData(chan);
	info = data->info;
	if (info->consoleInterp) {
	    /*
	     * New ConsoleInfo for a new console window.
	     */

	    info = ckalloc(sizeof(ConsoleInfo));
	    info->refCount = 0;

	    /*
	     * Update any console channels to make use of the new console.
	     */

	    if (Tcl_GetChannelType(chan = Tcl_GetStdChannel(TCL_STDIN))
		    == &consoleChannelType) {
		data = (ChannelData *) Tcl_GetChannelInstanceData(chan);
		data->info->refCount--;
		data->info = info;
		data->info->refCount++;
	    }
	    if (Tcl_GetChannelType(chan = Tcl_GetStdChannel(TCL_STDOUT))
		    == &consoleChannelType) {
		data = (ChannelData *) Tcl_GetChannelInstanceData(chan);
		data->info->refCount--;
		data->info = info;
		data->info->refCount++;
	    }
	    if (Tcl_GetChannelType(chan = Tcl_GetStdChannel(TCL_STDERR))
		    == &consoleChannelType) {
		data = (ChannelData *) Tcl_GetChannelInstanceData(chan);
		data->info->refCount--;
		data->info = info;
		data->info->refCount++;
	    }
	}
    } else {
	info = ckalloc(sizeof(ConsoleInfo));
	info->refCount = 0;
    }

    info->consoleInterp = consoleInterp;
    info->interp = interp;

    Tcl_CallWhenDeleted(consoleInterp, InterpDeleteProc, info);
    info->refCount++;
    Tcl_CreateThreadExitHandler(DeleteConsoleInterp, consoleInterp);

    /*
     * Add console commands to the interp
     */

    token = Tcl_CreateObjCommand(interp, "console", ConsoleObjCmd, info,
	    ConsoleDeleteProc);
    info->refCount++;

    /*
     * We don't have to count the ref held by the [consoleinterp] command
     * in the consoleInterp.  The ref held by the consoleInterp delete
     * handler takes care of us.
     */
    Tcl_CreateObjCommand(consoleInterp, "consoleinterp", InterpreterObjCmd,
	    info, NULL);

    mainWindow = Tk_MainWindow(interp);
    if (mainWindow) {
	Tk_CreateEventHandler(mainWindow, StructureNotifyMask,
		ConsoleEventProc, info);
	info->refCount++;
    }

    Tcl_Preserve(consoleInterp);
    result = Tcl_EvalEx(consoleInterp, "source $tk_library/console.tcl",
	    -1, TCL_EVAL_GLOBAL);
    if (result == TCL_ERROR) {
	Tcl_SetReturnOptions(interp,
		Tcl_GetReturnOptions(consoleInterp, result));
	Tcl_SetObjResult(interp, Tcl_GetObjResult(consoleInterp));
    }
    Tcl_Release(consoleInterp);
    if (result == TCL_ERROR) {
	Tcl_DeleteCommandFromToken(interp, token);
	mainWindow = Tk_MainWindow(interp);
	if (mainWindow) {
	    Tk_DeleteEventHandler(mainWindow, StructureNotifyMask,
		    ConsoleEventProc, info);
	    if (--info->refCount <= 0) {
		ckfree(info);
	    }
	}
	goto error;
    }
    return TCL_OK;

  error:
    Tcl_AddErrorInfo(interp, "\n    (creating console window)");
    if (!Tcl_InterpDeleted(consoleInterp)) {
	Tcl_DeleteInterp(consoleInterp);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleOutput--
 *
 *	Writes the given output on the IO channel. Returns count of how many
 *	characters were actually written, and an error indication.
 *
 * Results:
 *	A count of how many characters were written is returned and an error
 *	indication is returned in an output argument.
 *
 * Side effects:
 *	Writes output on the actual channel.
 *
 *----------------------------------------------------------------------
 */

static int
ConsoleOutput(
    ClientData instanceData,	/* Indicates which device to use. */
    const char *buf,		/* The data buffer. */
    int toWrite,		/* How many bytes to write? */
    int *errorCode)		/* Where to store error code. */
{
    ChannelData *data = instanceData;
    ConsoleInfo *info = data->info;

    *errorCode = 0;
    Tcl_SetErrno(0);

    if (info) {
	Tcl_Interp *consoleInterp = info->consoleInterp;

	if (consoleInterp && !Tcl_InterpDeleted(consoleInterp)) {
	    Tcl_DString ds;
	    Tcl_Encoding utf8 = Tcl_GetEncoding(NULL, "utf-8");

	    /*
	     * Not checking for utf8 == NULL.  Did not check for TCL_ERROR
	     * from Tcl_SetChannelOption() in Tk_InitConsoleChannels() either.
	     * Assumption is utf-8 Tcl_Encoding is reliably present.
	     */

	    const char *bytes
		    = Tcl_ExternalToUtfDString(utf8, buf, toWrite, &ds);
	    int numBytes = Tcl_DStringLength(&ds);
	    Tcl_Obj *cmd = Tcl_NewStringObj("tk::ConsoleOutput", -1);

	    Tcl_FreeEncoding(utf8);

	    if (data->type == TCL_STDERR) {
		Tcl_ListObjAppendElement(NULL, cmd,
			Tcl_NewStringObj("stderr", -1));
	    } else {
		Tcl_ListObjAppendElement(NULL, cmd,
			Tcl_NewStringObj("stdout", -1));
	    }
	    Tcl_ListObjAppendElement(NULL, cmd,
		    Tcl_NewStringObj(bytes, numBytes));

	    Tcl_DStringFree(&ds);
	    Tcl_IncrRefCount(cmd);
	    Tcl_EvalObjEx(consoleInterp, cmd, TCL_EVAL_GLOBAL);
	    Tcl_DecrRefCount(cmd);
	}
    }
    return toWrite;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleInput --
 *
 *	Read input from the console. Not currently implemented.
 *
 * Results:
 *	Always returns EOF.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConsoleInput(
    ClientData instanceData,	/* Unused. */
    char *buf,			/* Where to store data read. */
    int bufSize,		/* How much space is available in the
				 * buffer? */
    int *errorCode)		/* Where to store error code. */
{
    return 0;			/* Always return EOF. */
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleClose --
 *
 *	Closes the IO channel.
 *
 * Results:
 *	Always returns 0 (success).
 *
 * Side effects:
 *	Frees the dummy file associated with the channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConsoleClose(
    ClientData instanceData,	/* Unused. */
    Tcl_Interp *interp)		/* Unused. */
{
    ChannelData *data = instanceData;
    ConsoleInfo *info = data->info;

    if (info) {
	if (--info->refCount <= 0) {
	    /*
	     * Assuming the Tcl_Interp * fields must already be NULL.
	     */

	    ckfree(info);
	}
    }
    ckfree(data);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleWatch --
 *
 *	Called by the notifier to set up the console device so that events
 *	will be noticed. Since there are no events on the console, this
 *	routine just returns without doing anything.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
ConsoleWatch(
    ClientData instanceData,	/* Device ID for the channel. */
    int mask)			/* OR-ed combination of TCL_READABLE,
				 * TCL_WRITABLE and TCL_EXCEPTION, for the
				 * events we are interested in. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleHandle --
 *
 *	Invoked by the generic IO layer to get a handle from a channel.
 *	Because console channels are not devices, this function always fails.
 *
 * Results:
 *	Always returns TCL_ERROR.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConsoleHandle(
    ClientData instanceData,	/* Device ID for the channel. */
    int direction,		/* TCL_READABLE or TCL_WRITABLE to indicate
				 * which direction of the channel is being
				 * requested. */
    ClientData *handlePtr)	/* Where to store handle */
{
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleObjCmd --
 *
 *	The console command implements a Tcl interface to the various console
 *	options.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ConsoleObjCmd(
    ClientData clientData,	/* Access to the console interp */
    Tcl_Interp *interp,		/* Current interpreter */
    int objc,			/* Number of arguments */
    Tcl_Obj *const objv[])	/* Argument objects */
{
    int index, result;
    static const char *const options[] = {
	"eval", "hide", "show", "title", NULL};
    enum option {CON_EVAL, CON_HIDE, CON_SHOW, CON_TITLE};
    Tcl_Obj *cmd = NULL;
    ConsoleInfo *info = clientData;
    Tcl_Interp *consoleInterp = info->consoleInterp;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], options,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum option) index) {
    case CON_EVAL:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "script");
	    return TCL_ERROR;
	}
	cmd = objv[2];
	break;
    case CON_HIDE:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	cmd = Tcl_NewStringObj("wm withdraw .", -1);
	break;
    case CON_SHOW:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	cmd = Tcl_NewStringObj("wm deiconify .", -1);
	break;
    case CON_TITLE:
	if (objc > 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?title?");
	    return TCL_ERROR;
	}
	cmd = Tcl_NewStringObj("wm title .", -1);
	if (objc == 3) {
	    Tcl_ListObjAppendElement(NULL, cmd, objv[2]);
	}
	break;
    default:
	CLANG_ASSERT(0);
    }

    Tcl_IncrRefCount(cmd);
    if (consoleInterp && !Tcl_InterpDeleted(consoleInterp)) {
	Tcl_Preserve(consoleInterp);
	result = Tcl_EvalObjEx(consoleInterp, cmd, TCL_EVAL_GLOBAL);
	Tcl_SetReturnOptions(interp,
		Tcl_GetReturnOptions(consoleInterp, result));
	Tcl_SetObjResult(interp, Tcl_GetObjResult(consoleInterp));
	Tcl_Release(consoleInterp);
    } else {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"no active console interp", -1));
	Tcl_SetErrorCode(interp, "TK", "CONSOLE", "NONE", NULL);
	result = TCL_ERROR;
    }
    Tcl_DecrRefCount(cmd);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * InterpreterObjCmd --
 *
 *	This command allows the console interp to communicate with the main
 *	interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 *----------------------------------------------------------------------
 */

static int
InterpreterObjCmd(
    ClientData clientData,	/* */
    Tcl_Interp *interp,		/* Current interpreter */
    int objc,			/* Number of arguments */
    Tcl_Obj *const objv[])	/* Argument objects */
{
    int index, result = TCL_OK;
    static const char *const options[] = {"eval", "record", NULL};
    enum option {OTHER_EVAL, OTHER_RECORD};
    ConsoleInfo *info = clientData;
    Tcl_Interp *otherInterp = info->interp;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option arg");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], options,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "script");
	return TCL_ERROR;
    }

    if ((otherInterp == NULL) || Tcl_InterpDeleted(otherInterp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"no active master interp", -1));
	Tcl_SetErrorCode(interp, "TK", "CONSOLE", "NO_INTERP", NULL);
	return TCL_ERROR;
    }

    Tcl_Preserve(otherInterp);
    switch ((enum option) index) {
    case OTHER_EVAL:
   	result = Tcl_EvalObjEx(otherInterp, objv[2], TCL_EVAL_GLOBAL);

	/*
	 * TODO: Should exceptions be filtered here?
	 */

	Tcl_SetReturnOptions(interp,
		Tcl_GetReturnOptions(otherInterp, result));
	Tcl_SetObjResult(interp, Tcl_GetObjResult(otherInterp));
	break;
    case OTHER_RECORD:
   	Tcl_RecordAndEvalObj(otherInterp, objv[2], TCL_EVAL_GLOBAL);

	/*
	 * By not setting result, we discard any exceptions or errors here and
	 * always return TCL_OK. All the caller wants is the interp result to
	 * display, whether that's result or error message.
	 */

	Tcl_SetObjResult(interp, Tcl_GetObjResult(otherInterp));
	break;
    }
    Tcl_Release(otherInterp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteConsoleInterp --
 *
 *	Thread exit handler to destroy a console interp when the thread it
 *	lives in gets torn down.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteConsoleInterp(
    ClientData clientData)
{
    Tcl_Interp *interp = clientData;

    Tcl_DeleteInterp(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * InterpDeleteProc --
 *
 *	React when the interp in which the console is displayed is deleted for
 *	any reason.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new console it created.
 *
 *----------------------------------------------------------------------
 */

static void
InterpDeleteProc(
    ClientData clientData,
    Tcl_Interp *interp)
{
    ConsoleInfo *info = clientData;

    if (info->consoleInterp == interp) {
	Tcl_DeleteThreadExitHandler(DeleteConsoleInterp, info->consoleInterp);
	info->consoleInterp = NULL;
    }
    if (--info->refCount <= 0) {
	ckfree(info);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleDeleteProc --
 *
 *	If the console command is deleted we destroy the console window and
 *	all associated data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new console it created.
 *
 *----------------------------------------------------------------------
 */

static void
ConsoleDeleteProc(
    ClientData clientData)
{
    ConsoleInfo *info = clientData;

    if (info->consoleInterp) {
	Tcl_DeleteInterp(info->consoleInterp);
    }
    if (--info->refCount <= 0) {
	ckfree(info);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleEventProc --
 *
 *	This event function is registered on the main window of the slave
 *	interpreter. If the user or a running script causes the main window to
 *	be destroyed, then we need to inform the console interpreter by
 *	invoking "::tk::ConsoleExit".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invokes the "::tk::ConsoleExit" command in the console interp.
 *
 *----------------------------------------------------------------------
 */

static void
ConsoleEventProc(
    ClientData clientData,
    XEvent *eventPtr)
{
    if (eventPtr->type == DestroyNotify) {
	ConsoleInfo *info = clientData;
	Tcl_Interp *consoleInterp = info->consoleInterp;

	if (consoleInterp && !Tcl_InterpDeleted(consoleInterp)) {
	    Tcl_EvalEx(consoleInterp, "tk::ConsoleExit", -1, TCL_EVAL_GLOBAL);
	}

	if (--info->refCount <= 0) {
	    ckfree(info);
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
