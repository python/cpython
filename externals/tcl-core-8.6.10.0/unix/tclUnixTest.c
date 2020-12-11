/*
 * tclUnixTest.c --
 *
 *	Contains platform specific test commands for the Unix platform.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef USE_TCL_STUBS
#   define USE_TCL_STUBS
#endif
#include "tclInt.h"

/*
 * The headers are needed for the testalarm command that verifies the use of
 * SA_RESTART in signal handlers.
 */

#include <signal.h>
#include <sys/resource.h>

/*
 * The following macros convert between TclFile's and fd's. The conversion
 * simple involves shifting fd's up by one to ensure that no valid fd is ever
 * the same as NULL. Note that this code is duplicated from tclUnixPipe.c
 */

#define MakeFile(fd)	((TclFile)INT2PTR(((int)(fd))+1))
#define GetFd(file)	(PTR2INT(file)-1)

/*
 * The stuff below is used to keep track of file handlers created and
 * exercised by the "testfilehandler" command.
 */

typedef struct Pipe {
    TclFile readFile;		/* File handle for reading from the pipe. NULL
				 * means pipe doesn't exist yet. */
    TclFile writeFile;		/* File handle for writing from the pipe. */
    int readCount;		/* Number of times the file handler for this
				 * file has triggered and the file was
				 * readable. */
    int writeCount;		/* Number of times the file handler for this
				 * file has triggered and the file was
				 * writable. */
} Pipe;

#define MAX_PIPES 10
static Pipe testPipes[MAX_PIPES];

/*
 * The stuff below is used by the testalarm and testgotsig ommands.
 */

static const char *gotsig = "0";

/*
 * Forward declarations of functions defined later in this file:
 */

static Tcl_CmdProc TestalarmCmd;
static Tcl_ObjCmdProc TestchmodCmd;
static Tcl_CmdProc TestfilehandlerCmd;
static Tcl_CmdProc TestfilewaitCmd;
static Tcl_CmdProc TestfindexecutableCmd;
static Tcl_ObjCmdProc TestforkObjCmd;
static Tcl_CmdProc TestgetdefencdirCmd;
static Tcl_CmdProc TestgetopenfileCmd;
static Tcl_CmdProc TestgotsigCmd;
static Tcl_CmdProc TestsetdefencdirCmd;
static Tcl_FileProc TestFileHandlerProc;
static void AlarmHandler(int signum);

/*
 *----------------------------------------------------------------------
 *
 * TclplatformtestInit --
 *
 *	Defines commands that test platform specific functionality for Unix
 *	platforms.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Defines new commands.
 *
 *----------------------------------------------------------------------
 */

int
TclplatformtestInit(
    Tcl_Interp *interp)		/* Interpreter to add commands to. */
{
    Tcl_CreateObjCommand(interp, "testchmod", TestchmodCmd,
	    NULL, NULL);
    Tcl_CreateCommand(interp, "testfilehandler", TestfilehandlerCmd,
	    NULL, NULL);
    Tcl_CreateCommand(interp, "testfilewait", TestfilewaitCmd,
	    NULL, NULL);
    Tcl_CreateCommand(interp, "testfindexecutable", TestfindexecutableCmd,
	    NULL, NULL);
    Tcl_CreateObjCommand(interp, "testfork", TestforkObjCmd,
        NULL, NULL);
    Tcl_CreateCommand(interp, "testgetopenfile", TestgetopenfileCmd,
	    NULL, NULL);
    Tcl_CreateCommand(interp, "testgetdefenc", TestgetdefencdirCmd,
	    NULL, NULL);
    Tcl_CreateCommand(interp, "testsetdefenc", TestsetdefencdirCmd,
	    NULL, NULL);
    Tcl_CreateCommand(interp, "testalarm", TestalarmCmd,
	    NULL, NULL);
    Tcl_CreateCommand(interp, "testgotsig", TestgotsigCmd,
	    NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestfilehandlerCmd --
 *
 *	This function implements the "testfilehandler" command. It is used to
 *	test Tcl_CreateFileHandler, Tcl_DeleteFileHandler, and TclWaitForFile.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestfilehandlerCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
    Pipe *pipePtr;
    int i, mask, timeout;
    static int initialized = 0;
    char buffer[4000];
    TclFile file;

    /*
     * NOTE: When we make this code work on Windows also, the following
     * variable needs to be made Unix-only.
     */

    if (!initialized) {
	for (i = 0; i < MAX_PIPES; i++) {
	    testPipes[i].readFile = NULL;
	}
	initialized = 1;
    }

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
		" option ... \"", NULL);
        return TCL_ERROR;
    }
    pipePtr = NULL;
    if (argc >= 3) {
	if (Tcl_GetInt(interp, argv[2], &i) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i >= MAX_PIPES) {
	    Tcl_AppendResult(interp, "bad index ", argv[2], NULL);
	    return TCL_ERROR;
	}
	pipePtr = &testPipes[i];
    }

    if (strcmp(argv[1], "close") == 0) {
	for (i = 0; i < MAX_PIPES; i++) {
	    if (testPipes[i].readFile != NULL) {
		TclpCloseFile(testPipes[i].readFile);
		testPipes[i].readFile = NULL;
		TclpCloseFile(testPipes[i].writeFile);
		testPipes[i].writeFile = NULL;
	    }
	}
    } else if (strcmp(argv[1], "clear") == 0) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		    argv[0], " clear index\"", NULL);
	    return TCL_ERROR;
	}
	pipePtr->readCount = pipePtr->writeCount = 0;
    } else if (strcmp(argv[1], "counts") == 0) {
	char buf[TCL_INTEGER_SPACE * 2];

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		    argv[0], " counts index\"", NULL);
	    return TCL_ERROR;
	}
	sprintf(buf, "%d %d", pipePtr->readCount, pipePtr->writeCount);
	Tcl_AppendResult(interp, buf, NULL);
    } else if (strcmp(argv[1], "create") == 0) {
	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		    argv[0], " create index readMode writeMode\"", NULL);
	    return TCL_ERROR;
	}
	if (pipePtr->readFile == NULL) {
	    if (!TclpCreatePipe(&pipePtr->readFile, &pipePtr->writeFile)) {
		Tcl_AppendResult(interp, "couldn't open pipe: ",
			Tcl_PosixError(interp), NULL);
		return TCL_ERROR;
	    }
#ifdef O_NONBLOCK
	    fcntl(GetFd(pipePtr->readFile), F_SETFL, O_NONBLOCK);
	    fcntl(GetFd(pipePtr->writeFile), F_SETFL, O_NONBLOCK);
#else
	    Tcl_AppendResult(interp, "can't make pipes non-blocking",
		    NULL);
	    return TCL_ERROR;
#endif
	}
	pipePtr->readCount = 0;
	pipePtr->writeCount = 0;

	if (strcmp(argv[3], "readable") == 0) {
	    Tcl_CreateFileHandler(GetFd(pipePtr->readFile), TCL_READABLE,
		    TestFileHandlerProc, pipePtr);
	} else if (strcmp(argv[3], "off") == 0) {
	    Tcl_DeleteFileHandler(GetFd(pipePtr->readFile));
	} else if (strcmp(argv[3], "disabled") == 0) {
	    Tcl_CreateFileHandler(GetFd(pipePtr->readFile), 0,
		    TestFileHandlerProc, pipePtr);
	} else {
	    Tcl_AppendResult(interp, "bad read mode \"", argv[3], "\"", NULL);
	    return TCL_ERROR;
	}
	if (strcmp(argv[4], "writable") == 0) {
	    Tcl_CreateFileHandler(GetFd(pipePtr->writeFile), TCL_WRITABLE,
		    TestFileHandlerProc, pipePtr);
	} else if (strcmp(argv[4], "off") == 0) {
	    Tcl_DeleteFileHandler(GetFd(pipePtr->writeFile));
	} else if (strcmp(argv[4], "disabled") == 0) {
	    Tcl_CreateFileHandler(GetFd(pipePtr->writeFile), 0,
		    TestFileHandlerProc, pipePtr);
	} else {
	    Tcl_AppendResult(interp, "bad read mode \"", argv[4], "\"", NULL);
	    return TCL_ERROR;
	}
    } else if (strcmp(argv[1], "empty") == 0) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		    argv[0], " empty index\"", NULL);
	    return TCL_ERROR;
	}

        while (read(GetFd(pipePtr->readFile), buffer, 4000) > 0) {
	    /* Empty loop body. */
        }
    } else if (strcmp(argv[1], "fill") == 0) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		    argv[0], " fill index\"", NULL);
	    return TCL_ERROR;
	}

	memset(buffer, 'a', 4000);
        while (write(GetFd(pipePtr->writeFile), buffer, 4000) > 0) {
	    /* Empty loop body. */
        }
    } else if (strcmp(argv[1], "fillpartial") == 0) {
	char buf[TCL_INTEGER_SPACE];

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		    argv[0], " fillpartial index\"", NULL);
	    return TCL_ERROR;
	}

	memset(buffer, 'b', 10);
	TclFormatInt(buf, write(GetFd(pipePtr->writeFile), buffer, 10));
	Tcl_AppendResult(interp, buf, NULL);
    } else if (strcmp(argv[1], "oneevent") == 0) {
	Tcl_DoOneEvent(TCL_FILE_EVENTS|TCL_DONT_WAIT);
    } else if (strcmp(argv[1], "wait") == 0) {
	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		    argv[0], " wait index readable|writable timeout\"", NULL);
	    return TCL_ERROR;
	}
	if (pipePtr->readFile == NULL) {
	    Tcl_AppendResult(interp, "pipe ", argv[2], " doesn't exist", NULL);
	    return TCL_ERROR;
	}
	if (strcmp(argv[3], "readable") == 0) {
	    mask = TCL_READABLE;
	    file = pipePtr->readFile;
	} else {
	    mask = TCL_WRITABLE;
	    file = pipePtr->writeFile;
	}
	if (Tcl_GetInt(interp, argv[4], &timeout) != TCL_OK) {
	    return TCL_ERROR;
	}
	i = TclUnixWaitForFile(GetFd(file), mask, timeout);
	if (i & TCL_READABLE) {
	    Tcl_AppendElement(interp, "readable");
	}
	if (i & TCL_WRITABLE) {
	    Tcl_AppendElement(interp, "writable");
	}
    } else if (strcmp(argv[1], "windowevent") == 0) {
	Tcl_DoOneEvent(TCL_WINDOW_EVENTS|TCL_DONT_WAIT);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be close, clear, counts, create, empty, fill, "
		"fillpartial, oneevent, wait, or windowevent", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
TestFileHandlerProc(
    ClientData clientData,	/* Points to a Pipe structure. */
    int mask)			/* Indicates which events happened:
				 * TCL_READABLE or TCL_WRITABLE. */
{
    Pipe *pipePtr = clientData;

    if (mask & TCL_READABLE) {
	pipePtr->readCount++;
    }
    if (mask & TCL_WRITABLE) {
	pipePtr->writeCount++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TestfilewaitCmd --
 *
 *	This function implements the "testfilewait" command. It is used to
 *	test TclUnixWaitForFile.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestfilewaitCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
    int mask, result, timeout;
    Tcl_Channel channel;
    int fd;
    ClientData data;

    if (argc != 4) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
		" file readable|writable|both timeout\"", NULL);
	return TCL_ERROR;
    }
    channel = Tcl_GetChannel(interp, argv[1], NULL);
    if (channel == NULL) {
	return TCL_ERROR;
    }
    if (strcmp(argv[2], "readable") == 0) {
	mask = TCL_READABLE;
    } else if (strcmp(argv[2], "writable") == 0){
	mask = TCL_WRITABLE;
    } else if (strcmp(argv[2], "both") == 0){
	mask = TCL_WRITABLE|TCL_READABLE;
    } else {
	Tcl_AppendResult(interp, "bad argument \"", argv[2],
		"\": must be readable, writable, or both", NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetChannelHandle(channel,
	    (mask & TCL_READABLE) ? TCL_READABLE : TCL_WRITABLE,
	    (ClientData*) &data) != TCL_OK) {
	Tcl_AppendResult(interp, "couldn't get channel file", NULL);
	return TCL_ERROR;
    }
    fd = PTR2INT(data);
    if (Tcl_GetInt(interp, argv[3], &timeout) != TCL_OK) {
	return TCL_ERROR;
    }
    result = TclUnixWaitForFile(fd, mask, timeout);
    if (result & TCL_READABLE) {
	Tcl_AppendElement(interp, "readable");
    }
    if (result & TCL_WRITABLE) {
	Tcl_AppendElement(interp, "writable");
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestfindexecutableCmd --
 *
 *	This function implements the "testfindexecutable" command. It is used
 *	to test TclpFindExecutable.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestfindexecutableCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
    Tcl_Obj *saveName;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
		" argv0\"", NULL);
	return TCL_ERROR;
    }

    saveName = TclGetObjNameOfExecutable();
    Tcl_IncrRefCount(saveName);

    TclpFindExecutable(argv[1]);
    Tcl_SetObjResult(interp, TclGetObjNameOfExecutable());

    TclSetObjNameOfExecutable(saveName, NULL);
    Tcl_DecrRefCount(saveName);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestgetopenfileCmd --
 *
 *	This function implements the "testgetopenfile" command. It is used to
 *	get a FILE * value from a registered channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestgetopenfileCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
    ClientData filePtr;

    if (argc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" channelName forWriting\"", NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetOpenFile(interp, argv[1], atoi(argv[2]), 1, &filePtr)
	    == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (filePtr == NULL) {
        Tcl_AppendResult(interp,
		"Tcl_GetOpenFile succeeded but FILE * NULL!", NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestsetdefencdirCmd --
 *
 *	This function implements the "testsetdefenc" command. It is used to
 *	test Tcl_SetDefaultEncodingDir().
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestsetdefencdirCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" defaultDir\"", NULL);
        return TCL_ERROR;
    }

    Tcl_SetDefaultEncodingDir(argv[1]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestforkObjCmd --
 *
 *	This function implements the "testfork" command. It is used to
 *	fork the Tcl process for specific test cases.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestforkObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument strings. */
{
    pid_t pid;

    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }
    pid = fork();
    if (pid == -1) {
        Tcl_AppendResult(interp,
                "Cannot fork", NULL);
        return TCL_ERROR;
    }
    /* Only needed when pthread_atfork is not present,
     * should not hurt otherwise. */
    if (pid==0) {
	Tcl_InitNotifier();
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(pid));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestgetdefencdirCmd --
 *
 *	This function implements the "testgetdefenc" command. It is used to
 *	test Tcl_GetDefaultEncodingDir().
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestgetdefencdirCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
    if (argc != 1) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], NULL);
        return TCL_ERROR;
    }

    Tcl_AppendResult(interp, Tcl_GetDefaultEncodingDir(), NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestalarmCmd --
 *
 *	Test that EINTR is handled correctly by generating and handling a
 *	signal. This requires using the SA_RESTART flag when registering the
 *	signal handler.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Sets up an signal and async handlers.
 *
 *----------------------------------------------------------------------
 */

static int
TestalarmCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
#ifdef SA_RESTART
    unsigned int sec;
    struct sigaction action;

    if (argc > 1) {
	Tcl_GetInt(interp, argv[1], (int *)&sec);
    } else {
	sec = 1;
    }

    /*
     * Setup the signal handling that automatically retries any interrupted
     * I/O system calls.
     */

    action.sa_handler = AlarmHandler;
    memset((void *) &action.sa_mask, 0, sizeof(sigset_t));
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &action, NULL) < 0) {
	Tcl_AppendResult(interp, "sigaction: ", Tcl_PosixError(interp), NULL);
	return TCL_ERROR;
    }
    (void) alarm(sec);
    return TCL_OK;
#else
    Tcl_AppendResult(interp,
	    "warning: sigaction SA_RESTART not support on this platform",
	    NULL);
    return TCL_ERROR;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * AlarmHandler --
 *
 *	Signal handler for the alarm command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	Calls the Tcl Async handler.
 *
 *----------------------------------------------------------------------
 */

static void
AlarmHandler(
    int signum)
{
    gotsig = "1";
}

/*
 *----------------------------------------------------------------------
 *
 * TestgotsigCmd --
 *
 * 	Verify the signal was handled after the testalarm command.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Resets the value of gotsig back to '0'.
 *
 *----------------------------------------------------------------------
 */

static int
TestgotsigCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    const char **argv)		/* Argument strings. */
{
    Tcl_AppendResult(interp, gotsig, NULL);
    gotsig = "0";
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TestchmodCmd --
 *
 *	Implements the "testchmod" cmd.  Used when testing "file" command.
 *	The only attribute used by the Windows platform is the user write
 *	flag; if this is not set, the file is made read-only.  Otehrwise, the
 *	file is made read-write.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Changes permissions of specified files.
 *
 *---------------------------------------------------------------------------
 */

static int
TestchmodCmd(
    ClientData dummy,			/* Not used. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument strings. */
{
    int i, mode;

    if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "mode file ?file ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[1], &mode) != TCL_OK) {
	return TCL_ERROR;
    }

    for (i = 2; i < objc; i++) {
	Tcl_DString buffer;
	const char *translated;

	translated = Tcl_TranslateFileName(interp, Tcl_GetString(objv[i]), &buffer);
	if (translated == NULL) {
	    return TCL_ERROR;
	}
	if (chmod(translated, (unsigned) mode) != 0) {
	    Tcl_AppendResult(interp, translated, ": ", Tcl_PosixError(interp),
		    NULL);
	    return TCL_ERROR;
	}
	Tcl_DStringFree(&buffer);
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
