/*
 * tclPipe.c --
 *
 *	This file contains the generic portion of the command channel driver
 *	as well as various utility routines used in managing subprocesses.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * A linked list of the following structures is used to keep track of child
 * processes that have been detached but haven't exited yet, so we can make
 * sure that they're properly "reaped" (officially waited for) and don't lie
 * around as zombies cluttering the system.
 */

typedef struct Detached {
    Tcl_Pid pid;		/* Id of process that's been detached but
				 * isn't known to have exited. */
    struct Detached *nextPtr;	/* Next in list of all detached processes. */
} Detached;

static Detached *detList = NULL;/* List of all detached proceses. */
TCL_DECLARE_MUTEX(pipeMutex)	/* Guard access to detList. */

/*
 * Declarations for local functions defined in this file:
 */

static TclFile		FileForRedirect(Tcl_Interp *interp, const char *spec,
			    int atOk, const char *arg, const char *nextArg,
			    int flags, int *skipPtr, int *closePtr,
			    int *releasePtr);

/*
 *----------------------------------------------------------------------
 *
 * FileForRedirect --
 *
 *	This function does much of the work of parsing redirection operators.
 *	It handles "@" if specified and allowed, and a file name, and opens
 *	the file if necessary.
 *
 * Results:
 *	The return value is the descriptor number for the file. If an error
 *	occurs then NULL is returned and an error message is left in the
 *	interp's result. Several arguments are side-effected; see the argument
 *	list below for details.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TclFile
FileForRedirect(
    Tcl_Interp *interp,		/* Intepreter to use for error reporting. */
    const char *spec,		/* Points to character just after redirection
				 * character. */
    int atOK,			/* Non-zero means that '@' notation can be
				 * used to specify a channel, zero means that
				 * it isn't. */
    const char *arg,		/* Pointer to entire argument containing spec:
				 * used for error reporting. */
    const char *nextArg,	/* Next argument in argc/argv array, if needed
				 * for file name or channel name. May be
				 * NULL. */
    int flags,			/* Flags to use for opening file or to specify
				 * mode for channel. */
    int *skipPtr,		/* Filled with 1 if redirection target was in
				 * spec, 2 if it was in nextArg. */
    int *closePtr,		/* Filled with one if the caller should close
				 * the file when done with it, zero
				 * otherwise. */
    int *releasePtr)
{
    int writing = (flags & O_WRONLY);
    Tcl_Channel chan;
    TclFile file;

    *skipPtr = 1;
    if ((atOK != 0) && (*spec == '@')) {
	spec++;
	if (*spec == '\0') {
	    spec = nextArg;
	    if (spec == NULL) {
		goto badLastArg;
	    }
	    *skipPtr = 2;
	}
	chan = Tcl_GetChannel(interp, spec, NULL);
	if (chan == (Tcl_Channel) NULL) {
	    return NULL;
	}
	file = TclpMakeFile(chan, writing ? TCL_WRITABLE : TCL_READABLE);
	if (file == NULL) {
	    Tcl_Obj *msg;

	    Tcl_GetChannelError(chan, &msg);
	    if (msg) {
		Tcl_SetObjResult(interp, msg);
	    } else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"channel \"%s\" wasn't opened for %s",
			Tcl_GetChannelName(chan),
			((writing) ? "writing" : "reading")));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC",
			"BADCHAN", NULL);
	    }
	    return NULL;
	}
	*releasePtr = 1;
	if (writing) {
	    /*
	     * Be sure to flush output to the file, so that anything written
	     * by the child appears after stuff we've already written.
	     */

	    Tcl_Flush(chan);
	}
    } else {
	const char *name;
	Tcl_DString nameString;

	if (*spec == '\0') {
	    spec = nextArg;
	    if (spec == NULL) {
		goto badLastArg;
	    }
	    *skipPtr = 2;
	}
	name = Tcl_TranslateFileName(interp, spec, &nameString);
	if (name == NULL) {
	    return NULL;
	}
	file = TclpOpenFile(name, flags);
	Tcl_DStringFree(&nameString);
	if (file == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "couldn't %s file \"%s\": %s",
		    (writing ? "write" : "read"), spec,
		    Tcl_PosixError(interp)));
	    return NULL;
	}
	*closePtr = 1;
    }
    return file;

  badLastArg:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "can't specify \"%s\" as last word in command", arg));
    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC", "SYNTAX", NULL);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DetachPids --
 *
 *	This function is called to indicate that one or more child processes
 *	have been placed in background and will never be waited for; they
 *	should eventually be reaped by Tcl_ReapDetachedProcs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DetachPids(
    int numPids,		/* Number of pids to detach: gives size of
				 * array pointed to by pidPtr. */
    Tcl_Pid *pidPtr)		/* Array of pids to detach. */
{
    register Detached *detPtr;
    int i;

    Tcl_MutexLock(&pipeMutex);
    for (i = 0; i < numPids; i++) {
	detPtr = ckalloc(sizeof(Detached));
	detPtr->pid = pidPtr[i];
	detPtr->nextPtr = detList;
	detList = detPtr;
    }
    Tcl_MutexUnlock(&pipeMutex);

}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ReapDetachedProcs --
 *
 *	This function checks to see if any detached processes have exited and,
 *	if so, it "reaps" them by officially waiting on them. It should be
 *	called "occasionally" to make sure that all detached processes are
 *	eventually reaped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Processes are waited on, so that they can be reaped by the system.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ReapDetachedProcs(void)
{
    register Detached *detPtr;
    Detached *nextPtr, *prevPtr;
    int status;
    Tcl_Pid pid;

    Tcl_MutexLock(&pipeMutex);
    for (detPtr = detList, prevPtr = NULL; detPtr != NULL; ) {
	pid = Tcl_WaitPid(detPtr->pid, &status, WNOHANG);
	if ((pid == 0) || ((pid == (Tcl_Pid) -1) && (errno != ECHILD))) {
	    prevPtr = detPtr;
	    detPtr = detPtr->nextPtr;
	    continue;
	}
	nextPtr = detPtr->nextPtr;
	if (prevPtr == NULL) {
	    detList = detPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = detPtr->nextPtr;
	}
	ckfree(detPtr);
	detPtr = nextPtr;
    }
    Tcl_MutexUnlock(&pipeMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCleanupChildren --
 *
 *	This is a utility function used to wait for child processes to exit,
 *	record information about abnormal exits, and then collect any stderr
 *	output generated by them.
 *
 * Results:
 *	The return value is a standard Tcl result. If anything at weird
 *	happened with the child processes, TCL_ERROR is returned and a message
 *	is left in the interp's result.
 *
 * Side effects:
 *	If the last character of the interp's result is a newline, then it is
 *	removed unless keepNewline is non-zero. File errorId gets closed, and
 *	pidPtr is freed back to the storage allocator.
 *
 *----------------------------------------------------------------------
 */

int
TclCleanupChildren(
    Tcl_Interp *interp,		/* Used for error messages. */
    int numPids,		/* Number of entries in pidPtr array. */
    Tcl_Pid *pidPtr,		/* Array of process ids of children. */
    Tcl_Channel errorChan)	/* Channel for file containing stderr output
				 * from pipeline. NULL means there isn't any
				 * stderr output. */
{
    int result = TCL_OK;
    int i, abnormalExit, anyErrorInfo;
    Tcl_Pid pid;
    int waitStatus;
    const char *msg;
    unsigned long resolvedPid;

    abnormalExit = 0;
    for (i = 0; i < numPids; i++) {
	/*
	 * We need to get the resolved pid before we wait on it as the windows
	 * implementation of Tcl_WaitPid deletes the information such that any
	 * following calls to TclpGetPid fail.
	 */

	resolvedPid = TclpGetPid(pidPtr[i]);
	pid = Tcl_WaitPid(pidPtr[i], &waitStatus, 0);
	if (pid == (Tcl_Pid) -1) {
	    result = TCL_ERROR;
	    if (interp != NULL) {
		msg = Tcl_PosixError(interp);
		if (errno == ECHILD) {
		    /*
		     * This changeup in message suggested by Mark Diekhans to
		     * remind people that ECHILD errors can occur on some
		     * systems if SIGCHLD isn't in its default state.
		     */

		    msg =
			"child process lost (is SIGCHLD ignored or trapped?)";
		}
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"error waiting for process to exit: %s", msg));
	    }
	    continue;
	}

	/*
	 * Create error messages for unusual process exits. An extra newline
	 * gets appended to each error message, but it gets removed below (in
	 * the same fashion that an extra newline in the command's output is
	 * removed).
	 */

	if (!WIFEXITED(waitStatus) || (WEXITSTATUS(waitStatus) != 0)) {
	    char msg1[TCL_INTEGER_SPACE], msg2[TCL_INTEGER_SPACE];

	    result = TCL_ERROR;
	    sprintf(msg1, "%lu", resolvedPid);
	    if (WIFEXITED(waitStatus)) {
		if (interp != NULL) {
		    sprintf(msg2, "%u", WEXITSTATUS(waitStatus));
		    Tcl_SetErrorCode(interp, "CHILDSTATUS", msg1, msg2, NULL);
		}
		abnormalExit = 1;
	    } else if (interp != NULL) {
		const char *p;

		if (WIFSIGNALED(waitStatus)) {
		    p = Tcl_SignalMsg(WTERMSIG(waitStatus));
		    Tcl_SetErrorCode(interp, "CHILDKILLED", msg1,
			    Tcl_SignalId(WTERMSIG(waitStatus)), p, NULL);
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "child killed: %s\n", p));
		} else if (WIFSTOPPED(waitStatus)) {
		    p = Tcl_SignalMsg(WSTOPSIG(waitStatus));
		    Tcl_SetErrorCode(interp, "CHILDSUSP", msg1,
			    Tcl_SignalId(WSTOPSIG(waitStatus)), p, NULL);
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "child suspended: %s\n", p));
		} else {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "child wait status didn't make sense\n", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC",
			    "ODDWAITRESULT", msg1, NULL);
		}
	    }
	}
    }

    /*
     * Read the standard error file. If there's anything there, then return an
     * error and add the file's contents to the result string.
     */

    anyErrorInfo = 0;
    if (errorChan != NULL) {
	/*
	 * Make sure we start at the beginning of the file.
	 */

	if (interp != NULL) {
	    int count;
	    Tcl_Obj *objPtr;

	    Tcl_Seek(errorChan, (Tcl_WideInt)0, SEEK_SET);
	    objPtr = Tcl_NewObj();
	    count = Tcl_ReadChars(errorChan, objPtr, -1, 0);
	    if (count < 0) {
		result = TCL_ERROR;
		Tcl_DecrRefCount(objPtr);
		Tcl_ResetResult(interp);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"error reading stderr output file: %s",
			Tcl_PosixError(interp)));
	    } else if (count > 0) {
		anyErrorInfo = 1;
		Tcl_SetObjResult(interp, objPtr);
		result = TCL_ERROR;
	    } else {
		Tcl_DecrRefCount(objPtr);
	    }
	}
	Tcl_Close(NULL, errorChan);
    }

    /*
     * If a child exited abnormally but didn't output any error information at
     * all, generate an error message here.
     */

    if ((abnormalExit != 0) && (anyErrorInfo == 0) && (interp != NULL)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"child process exited abnormally", -1));
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreatePipeline --
 *
 *	Given an argc/argv array, instantiate a pipeline of processes as
 *	described by the argv.
 *
 *	This function is unofficially exported for use by BLT.
 *
 * Results:
 *	The return value is a count of the number of new processes created, or
 *	-1 if an error occurred while creating the pipeline. *pidArrayPtr is
 *	filled in with the address of a dynamically allocated array giving the
 *	ids of all of the processes. It is up to the caller to free this array
 *	when it isn't needed anymore. If inPipePtr is non-NULL, *inPipePtr is
 *	filled in with the file id for the input pipe for the pipeline (if
 *	any): the caller must eventually close this file. If outPipePtr isn't
 *	NULL, then *outPipePtr is filled in with the file id for the output
 *	pipe from the pipeline: the caller must close this file. If errFilePtr
 *	isn't NULL, then *errFilePtr is filled with a file id that may be used
 *	to read error output after the pipeline completes.
 *
 * Side effects:
 *	Processes and pipes are created.
 *
 *----------------------------------------------------------------------
 */

int
TclCreatePipeline(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    int argc,			/* Number of entries in argv. */
    const char **argv,		/* Array of strings describing commands in
				 * pipeline plus I/O redirection with <, <<,
				 * >, etc. Argv[argc] must be NULL. */
    Tcl_Pid **pidArrayPtr,	/* Word at *pidArrayPtr gets filled in with
				 * address of array of pids for processes in
				 * pipeline (first pid is first process in
				 * pipeline). */
    TclFile *inPipePtr,		/* If non-NULL, input to the pipeline comes
				 * from a pipe (unless overridden by
				 * redirection in the command). The file id
				 * with which to write to this pipe is stored
				 * at *inPipePtr. NULL means command specified
				 * its own input source. */
    TclFile *outPipePtr,	/* If non-NULL, output to the pipeline goes to
				 * a pipe, unless overridden by redirection in
				 * the command. The file id with which to read
				 * frome this pipe is stored at *outPipePtr.
				 * NULL means command specified its own output
				 * sink. */
    TclFile *errFilePtr)	/* If non-NULL, all stderr output from the
				 * pipeline will go to a temporary file
				 * created here, and a descriptor to read the
				 * file will be left at *errFilePtr. The file
				 * will be removed already, so closing this
				 * descriptor will be the end of the file. If
				 * this is NULL, then all stderr output goes
				 * to our stderr. If the pipeline specifies
				 * redirection then the file will still be
				 * created but it will never get any data. */
{
    Tcl_Pid *pidPtr = NULL;	/* Points to malloc-ed array holding all the
				 * pids of child processes. */
    int numPids;		/* Actual number of processes that exist at
				 * *pidPtr right now. */
    int cmdCount;		/* Count of number of distinct commands found
				 * in argc/argv. */
    const char *inputLiteral = NULL;
				/* If non-null, then this points to a string
				 * containing input data (specified via <<) to
				 * be piped to the first process in the
				 * pipeline. */
    TclFile inputFile = NULL;	/* If != NULL, gives file to use as input for
				 * first process in pipeline (specified via <
				 * or <@). */
    int inputClose = 0;		/* If non-zero, then inputFile should be
				 * closed when cleaning up. */
    int inputRelease = 0;
    TclFile outputFile = NULL;	/* Writable file for output from last command
				 * in pipeline (could be file or pipe). NULL
				 * means use stdout. */
    int outputClose = 0;	/* If non-zero, then outputFile should be
				 * closed when cleaning up. */
    int outputRelease = 0;
    TclFile errorFile = NULL;	/* Writable file for error output from all
				 * commands in pipeline. NULL means use
				 * stderr. */
    int errorClose = 0;		/* If non-zero, then errorFile should be
				 * closed when cleaning up. */
    int errorRelease = 0;
    const char *p;
    const char *nextArg;
    int skip, lastBar, lastArg, i, j, atOK, flags, needCmd, errorToOutput = 0;
    Tcl_DString execBuffer;
    TclFile pipeIn;
    TclFile curInFile, curOutFile, curErrFile;
    Tcl_Channel channel;

    if (inPipePtr != NULL) {
	*inPipePtr = NULL;
    }
    if (outPipePtr != NULL) {
	*outPipePtr = NULL;
    }
    if (errFilePtr != NULL) {
	*errFilePtr = NULL;
    }

    Tcl_DStringInit(&execBuffer);

    pipeIn = NULL;
    curInFile = NULL;
    curOutFile = NULL;
    numPids = 0;

    /*
     * First, scan through all the arguments to figure out the structure of
     * the pipeline. Process all of the input and output redirection arguments
     * and remove them from the argument list in the pipeline. Count the
     * number of distinct processes (it's the number of "|" arguments plus
     * one) but don't remove the "|" arguments because they'll be used in the
     * second pass to seperate the individual child processes. Cannot start
     * the child processes in this pass because the redirection symbols may
     * appear anywhere in the command line - e.g., the '<' that specifies the
     * input to the entire pipe may appear at the very end of the argument
     * list.
     */

    lastBar = -1;
    cmdCount = 1;
    needCmd = 1;
    for (i = 0; i < argc; i++) {
	errorToOutput = 0;
	skip = 0;
	p = argv[i];
	switch (*p++) {
	case '|':
	    if (*p == '&') {
		p++;
	    }
	    if (*p == '\0') {
		if ((i == (lastBar + 1)) || (i == (argc - 1))) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "illegal use of | or |& in command", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC",
			    "PIPESYNTAX", NULL);
		    goto error;
		}
	    }
	    lastBar = i;
	    cmdCount++;
	    needCmd = 1;
	    break;

	case '<':
	    if (inputClose != 0) {
		inputClose = 0;
		TclpCloseFile(inputFile);
	    }
	    if (inputRelease != 0) {
		inputRelease = 0;
		TclpReleaseFile(inputFile);
	    }
	    if (*p == '<') {
		inputFile = NULL;
		inputLiteral = p + 1;
		skip = 1;
		if (*inputLiteral == '\0') {
		    inputLiteral = ((i + 1) == argc) ? NULL : argv[i + 1];
		    if (inputLiteral == NULL) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf(
				"can't specify \"%s\" as last word in command",
				argv[i]));
			Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC",
				"PIPESYNTAX", NULL);
			goto error;
		    }
		    skip = 2;
		}
	    } else {
		nextArg = ((i + 1) == argc) ? NULL : argv[i + 1];
		inputLiteral = NULL;
		inputFile = FileForRedirect(interp, p, 1, argv[i], nextArg,
			O_RDONLY, &skip, &inputClose, &inputRelease);
		if (inputFile == NULL) {
		    goto error;
		}
	    }
	    break;

	case '>':
	    atOK = 1;
	    flags = O_WRONLY | O_CREAT | O_TRUNC;
	    if (*p == '>') {
		p++;
		atOK = 0;

		/*
		 * Note that the O_APPEND flag only has an effect on POSIX
		 * platforms. On Windows, we just have to carry on regardless.
		 */

		flags = O_WRONLY | O_CREAT | O_APPEND;
	    }
	    if (*p == '&') {
		if (errorClose != 0) {
		    errorClose = 0;
		    TclpCloseFile(errorFile);
		}
		errorToOutput = 1;
		p++;
	    }

	    /*
	     * Close the old output file, but only if the error file is not
	     * also using it.
	     */

	    if (outputClose != 0) {
		outputClose = 0;
		if (errorFile == outputFile) {
		    errorClose = 1;
		} else {
		    TclpCloseFile(outputFile);
		}
	    }
	    if (outputRelease != 0) {
		outputRelease = 0;
		if (errorFile == outputFile) {
		    errorRelease = 1;
		} else {
		    TclpReleaseFile(outputFile);
		}
	    }
	    nextArg = ((i + 1) == argc) ? NULL : argv[i + 1];
	    outputFile = FileForRedirect(interp, p, atOK, argv[i], nextArg,
		    flags, &skip, &outputClose, &outputRelease);
	    if (outputFile == NULL) {
		goto error;
	    }
	    if (errorToOutput) {
		if (errorClose != 0) {
		    errorClose = 0;
		    TclpCloseFile(errorFile);
		}
		if (errorRelease != 0) {
		    errorRelease = 0;
		    TclpReleaseFile(errorFile);
		}
		errorFile = outputFile;
	    }
	    break;

	case '2':
	    if (*p != '>') {
		break;
	    }
	    p++;
	    atOK = 1;
	    flags = O_WRONLY | O_CREAT | O_TRUNC;
	    if (*p == '>') {
		p++;
		atOK = 0;

		/*
		 * Note that the O_APPEND flag only has an effect on POSIX
		 * platforms. On Windows, we just have to carry on regardless.
		 */

		flags = O_WRONLY | O_CREAT | O_APPEND;
	    }
	    if (errorClose != 0) {
		errorClose = 0;
		TclpCloseFile(errorFile);
	    }
	    if (errorRelease != 0) {
		errorRelease = 0;
		TclpReleaseFile(errorFile);
	    }
	    if (atOK && p[0] == '@' && p[1] == '1' && p[2] == '\0') {
		/*
		 * Special case handling of 2>@1 to redirect stderr to the
		 * exec/open output pipe as well. This is meant for the end of
		 * the command string, otherwise use |& between commands.
		 */

		if (i != argc-1) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "must specify \"%s\" as last word in command",
			    argv[i]));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC",
			    "PIPESYNTAX", NULL);
		    goto error;
		}
		errorFile = outputFile;
		errorToOutput = 2;
		skip = 1;
	    } else {
		nextArg = ((i + 1) == argc) ? NULL : argv[i + 1];
		errorFile = FileForRedirect(interp, p, atOK, argv[i],
			nextArg, flags, &skip, &errorClose, &errorRelease);
		if (errorFile == NULL) {
		    goto error;
		}
	    }
	    break;

	default:
	    /*
	     * Got a command word, not a redirection.
	     */

	    needCmd = 0;
	    break;
	}

	if (skip != 0) {
	    for (j = i + skip; j < argc; j++) {
		argv[j - skip] = argv[j];
	    }
	    argc -= skip;
	    i -= 1;
	}
    }

    if (needCmd) {
	/*
	 * We had a bar followed only by redirections.
	 */

	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"illegal use of | or |& in command", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC", "PIPESYNTAX",
		NULL);
	goto error;
    }

    if (inputFile == NULL) {
	if (inputLiteral != NULL) {
	    /*
	     * The input for the first process is immediate data coming from
	     * Tcl. Create a temporary file for it and put the data into the
	     * file.
	     */

	    inputFile = TclpCreateTempFile(inputLiteral);
	    if (inputFile == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't create input file for command: %s",
			Tcl_PosixError(interp)));
		goto error;
	    }
	    inputClose = 1;
	} else if (inPipePtr != NULL) {
	    /*
	     * The input for the first process in the pipeline is to come from
	     * a pipe that can be written from by the caller.
	     */

	    if (TclpCreatePipe(&inputFile, inPipePtr) == 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't create input pipe for command: %s",
			Tcl_PosixError(interp)));
		goto error;
	    }
	    inputClose = 1;
	} else {
	    /*
	     * The input for the first process comes from stdin.
	     */

	    channel = Tcl_GetStdChannel(TCL_STDIN);
	    if (channel != NULL) {
		inputFile = TclpMakeFile(channel, TCL_READABLE);
		if (inputFile != NULL) {
		    inputRelease = 1;
		}
	    }
	}
    }

    if (outputFile == NULL) {
	if (outPipePtr != NULL) {
	    /*
	     * Output from the last process in the pipeline is to go to a pipe
	     * that can be read by the caller.
	     */

	    if (TclpCreatePipe(outPipePtr, &outputFile) == 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't create output pipe for command: %s",
			Tcl_PosixError(interp)));
		goto error;
	    }
	    outputClose = 1;
	} else {
	    /*
	     * The output for the last process goes to stdout.
	     */

	    channel = Tcl_GetStdChannel(TCL_STDOUT);
	    if (channel) {
		outputFile = TclpMakeFile(channel, TCL_WRITABLE);
		if (outputFile != NULL) {
		    outputRelease = 1;
		}
	    }
	}
    }

    if (errorFile == NULL) {
	if (errorToOutput == 2) {
	    /*
	     * Handle 2>@1 special case at end of cmd line.
	     */

	    errorFile = outputFile;
	} else if (errFilePtr != NULL) {
	    /*
	     * Set up the standard error output sink for the pipeline, if
	     * requested. Use a temporary file which is opened, then deleted.
	     * Could potentially just use pipe, but if it filled up it could
	     * cause the pipeline to deadlock: we'd be waiting for processes
	     * to complete before reading stderr, and processes couldn't
	     * complete because stderr was backed up.
	     */

	    errorFile = TclpCreateTempFile(NULL);
	    if (errorFile == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't create error file for command: %s",
			Tcl_PosixError(interp)));
		goto error;
	    }
	    *errFilePtr = errorFile;
	} else {
	    /*
	     * Errors from the pipeline go to stderr.
	     */

	    channel = Tcl_GetStdChannel(TCL_STDERR);
	    if (channel) {
		errorFile = TclpMakeFile(channel, TCL_WRITABLE);
		if (errorFile != NULL) {
		    errorRelease = 1;
		}
	    }
	}
    }

    /*
     * Scan through the argc array, creating a process for each group of
     * arguments between the "|" characters.
     */

    Tcl_ReapDetachedProcs();
    pidPtr = ckalloc(cmdCount * sizeof(Tcl_Pid));

    curInFile = inputFile;

    for (i = 0; i < argc; i = lastArg + 1) {
	int result, joinThisError;
	Tcl_Pid pid;
	const char *oldName;

	/*
	 * Convert the program name into native form.
	 */

	if (Tcl_TranslateFileName(interp, argv[i], &execBuffer) == NULL) {
	    goto error;
	}

	/*
	 * Find the end of the current segment of the pipeline.
	 */

	joinThisError = 0;
	for (lastArg = i; lastArg < argc; lastArg++) {
	    if (argv[lastArg][0] != '|') {
		continue;
	    }
	    if (argv[lastArg][1] == '\0') {
		break;
	    }
	    if ((argv[lastArg][1] == '&') && (argv[lastArg][2] == '\0')) {
		joinThisError = 1;
		break;
	    }
	}

	/*
	 * If this is the last segment, use the specified outputFile.
	 * Otherwise create an intermediate pipe. pipeIn will become the
	 * curInFile for the next segment of the pipe.
	 */

	if (lastArg == argc) {
	    curOutFile = outputFile;
	} else {
	    argv[lastArg] = NULL;
	    if (TclpCreatePipe(&pipeIn, &curOutFile) == 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"couldn't create pipe: %s", Tcl_PosixError(interp)));
		goto error;
	    }
	}

	if (joinThisError != 0) {
	    curErrFile = curOutFile;
	} else {
	    curErrFile = errorFile;
	}

	/*
	 * Restore argv[i], since a caller wouldn't expect the contents of
	 * argv to be modified.
	 */

	oldName = argv[i];
	argv[i] = Tcl_DStringValue(&execBuffer);
	result = TclpCreateProcess(interp, lastArg - i, argv + i,
		curInFile, curOutFile, curErrFile, &pid);
	argv[i] = oldName;
	if (result != TCL_OK) {
	    goto error;
	}
	Tcl_DStringFree(&execBuffer);

	pidPtr[numPids] = pid;
	numPids++;

	/*
	 * Close off our copies of file descriptors that were set up for this
	 * child, then set up the input for the next child.
	 */

	if ((curInFile != NULL) && (curInFile != inputFile)) {
	    TclpCloseFile(curInFile);
	}
	curInFile = pipeIn;
	pipeIn = NULL;

	if ((curOutFile != NULL) && (curOutFile != outputFile)) {
	    TclpCloseFile(curOutFile);
	}
	curOutFile = NULL;
    }

    *pidArrayPtr = pidPtr;

    /*
     * All done. Cleanup open files lying around and then return.
     */

  cleanup:
    Tcl_DStringFree(&execBuffer);

    if (inputClose) {
	TclpCloseFile(inputFile);
    } else if (inputRelease) {
	TclpReleaseFile(inputFile);
    }
    if (outputClose) {
	TclpCloseFile(outputFile);
    } else if (outputRelease) {
	TclpReleaseFile(outputFile);
    }
    if (errorClose) {
	TclpCloseFile(errorFile);
    } else if (errorRelease) {
	TclpReleaseFile(errorFile);
    }
    return numPids;

    /*
     * An error occurred. There could have been extra files open, such as
     * pipes between children. Clean them all up. Detach any child processes
     * that have been created.
     */

  error:
    if (pipeIn != NULL) {
	TclpCloseFile(pipeIn);
    }
    if ((curOutFile != NULL) && (curOutFile != outputFile)) {
	TclpCloseFile(curOutFile);
    }
    if ((curInFile != NULL) && (curInFile != inputFile)) {
	TclpCloseFile(curInFile);
    }
    if ((inPipePtr != NULL) && (*inPipePtr != NULL)) {
	TclpCloseFile(*inPipePtr);
	*inPipePtr = NULL;
    }
    if ((outPipePtr != NULL) && (*outPipePtr != NULL)) {
	TclpCloseFile(*outPipePtr);
	*outPipePtr = NULL;
    }
    if ((errFilePtr != NULL) && (*errFilePtr != NULL)) {
	TclpCloseFile(*errFilePtr);
	*errFilePtr = NULL;
    }
    if (pidPtr != NULL) {
	for (i = 0; i < numPids; i++) {
	    if (pidPtr[i] != (Tcl_Pid) -1) {
		Tcl_DetachPids(1, &pidPtr[i]);
	    }
	}
	ckfree(pidPtr);
    }
    numPids = -1;
    goto cleanup;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_OpenCommandChannel --
 *
 *	Opens an I/O channel to one or more subprocesses specified by argc and
 *	argv. The flags argument determines the disposition of the stdio
 *	handles. If the TCL_STDIN flag is set then the standard input for the
 *	first subprocess will be tied to the channel: writing to the channel
 *	will provide input to the subprocess. If TCL_STDIN is not set, then
 *	standard input for the first subprocess will be the same as this
 *	application's standard input. If TCL_STDOUT is set then standard
 *	output from the last subprocess can be read from the channel;
 *	otherwise it goes to this application's standard output. If TCL_STDERR
 *	is set, standard error output for all subprocesses is returned to the
 *	channel and results in an error when the channel is closed; otherwise
 *	it goes to this application's standard error. If TCL_ENFORCE_MODE is
 *	not set, then argc and argv can redirect the stdio handles to override
 *	TCL_STDIN, TCL_STDOUT, and TCL_STDERR; if it is set, then it is an
 *	error for argc and argv to override stdio channels for which
 *	TCL_STDIN, TCL_STDOUT, and TCL_STDERR have been set.
 *
 * Results:
 *	A new command channel, or NULL on failure with an error message left
 *	in interp.
 *
 * Side effects:
 *	Creates processes, opens pipes.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_OpenCommandChannel(
    Tcl_Interp *interp,		/* Interpreter for error reporting. Can NOT be
				 * NULL. */
    int argc,			/* How many arguments. */
    const char **argv,		/* Array of arguments for command pipe. */
    int flags)			/* Or'ed combination of TCL_STDIN, TCL_STDOUT,
				 * TCL_STDERR, and TCL_ENFORCE_MODE. */
{
    TclFile *inPipePtr, *outPipePtr, *errFilePtr;
    TclFile inPipe, outPipe, errFile;
    int numPids;
    Tcl_Pid *pidPtr;
    Tcl_Channel channel;

    inPipe = outPipe = errFile = NULL;

    inPipePtr = (flags & TCL_STDIN) ? &inPipe : NULL;
    outPipePtr = (flags & TCL_STDOUT) ? &outPipe : NULL;
    errFilePtr = (flags & TCL_STDERR) ? &errFile : NULL;

    numPids = TclCreatePipeline(interp, argc, argv, &pidPtr, inPipePtr,
	    outPipePtr, errFilePtr);

    if (numPids < 0) {
	goto error;
    }

    /*
     * Verify that the pipes that were created satisfy the readable/writable
     * constraints.
     */

    if (flags & TCL_ENFORCE_MODE) {
	if ((flags & TCL_STDOUT) && (outPipe == NULL)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't read output from command:"
		    " standard output was redirected", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC",
		    "BADREDIRECT", NULL);
	    goto error;
	}
	if ((flags & TCL_STDIN) && (inPipe == NULL)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't write input to command:"
		    " standard input was redirected", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC",
		    "BADREDIRECT", NULL);
	    goto error;
	}
    }

    channel = TclpCreateCommandChannel(outPipe, inPipe, errFile,
	    numPids, pidPtr);

    if (channel == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"pipe for command could not be created", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "EXEC", "NOPIPE", NULL);
	goto error;
    }
    return channel;

  error:
    if (numPids > 0) {
	Tcl_DetachPids(numPids, pidPtr);
	ckfree(pidPtr);
    }
    if (inPipe != NULL) {
	TclpCloseFile(inPipe);
    }
    if (outPipe != NULL) {
	TclpCloseFile(outPipe);
    }
    if (errFile != NULL) {
	TclpCloseFile(errFile);
    }
    return NULL;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
