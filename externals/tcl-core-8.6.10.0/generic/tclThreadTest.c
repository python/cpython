/*
 * tclThreadTest.c --
 *
 *	This file implements the testthread command. Eventually this should be
 *	tclThreadCmd.c
 *	Some of this code is based on work done by Richard Hipp on behalf of
 *	Conservation Through Innovation, Limited, with their permission.
 *
 * Copyright (c) 1998 by Sun Microsystems, Inc.
 * Copyright (c) 2006-2008 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef USE_TCL_STUBS
#   define USE_TCL_STUBS
#endif
#include "tclInt.h"

#ifdef TCL_THREADS
/*
 * Each thread has an single instance of the following structure. There is one
 * instance of this structure per thread even if that thread contains multiple
 * interpreters. The interpreter identified by this structure is the main
 * interpreter for the thread.
 *
 * The main interpreter is the one that will process any messages received by
 * a thread. Any thread can send messages but only the main interpreter can
 * receive them.
 */

typedef struct ThreadSpecificData {
    Tcl_ThreadId threadId;	/* Tcl ID for this thread */
    Tcl_Interp *interp;		/* Main interpreter for this thread */
    int flags;			/* See the TP_ defines below... */
    struct ThreadSpecificData *nextPtr;
				/* List for "thread names" */
    struct ThreadSpecificData *prevPtr;
				/* List for "thread names" */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * This list is used to list all threads that have interpreters. This is
 * protected by threadMutex.
 */

static ThreadSpecificData *threadList = NULL;

/*
 * The following bit-values are legal for the "flags" field of the
 * ThreadSpecificData structure.
 */

#define TP_Dying		0x001 /* This thread is being canceled */

/*
 * An instance of the following structure contains all information that is
 * passed into a new thread when the thread is created using either the
 * "thread create" Tcl command or the ThreadCreate() C function.
 */

typedef struct ThreadCtrl {
    const char *script;		/* The Tcl command this thread should
				 * execute */
    int flags;			/* Initial value of the "flags" field in the
				 * ThreadSpecificData structure for the new
				 * thread. Might contain TP_Detached or
				 * TP_TclThread. */
    Tcl_Condition condWait;	/* This condition variable is used to
				 * synchronize the parent and child threads.
				 * The child won't run until it acquires
				 * threadMutex, and the parent function won't
				 * complete until signaled on this condition
				 * variable. */
} ThreadCtrl;

/*
 * This is the event used to send scripts to other threads.
 */

typedef struct ThreadEvent {
    Tcl_Event event;		/* Must be first */
    char *script;		/* The script to execute. */
    struct ThreadEventResult *resultPtr;
				/* To communicate the result. This is NULL if
				 * we don't care about it. */
} ThreadEvent;

typedef struct ThreadEventResult {
    Tcl_Condition done;		/* Signaled when the script completes */
    int code;			/* Return value of Tcl_Eval */
    char *result;		/* Result from the script */
    char *errorInfo;		/* Copy of errorInfo variable */
    char *errorCode;		/* Copy of errorCode variable */
    Tcl_ThreadId srcThreadId;	/* Id of sending thread, in case it dies */
    Tcl_ThreadId dstThreadId;	/* Id of target thread, in case it dies */
    struct ThreadEvent *eventPtr;	/* Back pointer */
    struct ThreadEventResult *nextPtr;	/* List for cleanup */
    struct ThreadEventResult *prevPtr;

} ThreadEventResult;

static ThreadEventResult *resultList;

/*
 * This is for simple error handling when a thread script exits badly.
 */

static Tcl_ThreadId mainThreadId;
static Tcl_ThreadId errorThreadId;
static char *errorProcString;

/*
 * Access to the list of threads and to the thread send results is guarded by
 * this mutex.
 */

TCL_DECLARE_MUTEX(threadMutex)

static int		ThreadObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		ThreadCreate(Tcl_Interp *interp, const char *script,
			    int joinable);
static int		ThreadList(Tcl_Interp *interp);
static int		ThreadSend(Tcl_Interp *interp, Tcl_ThreadId id,
			    const char *script, int wait);
static int		ThreadCancel(Tcl_Interp *interp, Tcl_ThreadId id,
			    const char *result, int flags);

static Tcl_ThreadCreateType	NewTestThread(ClientData clientData);
static void		ListRemove(ThreadSpecificData *tsdPtr);
static void		ListUpdateInner(ThreadSpecificData *tsdPtr);
static int		ThreadEventProc(Tcl_Event *evPtr, int mask);
static void		ThreadErrorProc(Tcl_Interp *interp);
static void		ThreadFreeProc(ClientData clientData);
static int		ThreadDeleteEvent(Tcl_Event *eventPtr,
			    ClientData clientData);
static void		ThreadExitProc(ClientData clientData);
extern int		Tcltest_Init(Tcl_Interp *interp);

/*
 *----------------------------------------------------------------------
 *
 * TclThread_Init --
 *
 *	Initialize the test thread command.
 *
 * Results:
 *	TCL_OK if the package was properly initialized.
 *
 * Side effects:
 *	Add the "testthread" command to the interp.
 *
 *----------------------------------------------------------------------
 */

int
TclThread_Init(
    Tcl_Interp *interp)		/* The current Tcl interpreter */
{
    /*
     * If the main thread Id has not been set, do it now.
     */

    Tcl_MutexLock(&threadMutex);
    if (mainThreadId == 0) {
	mainThreadId = Tcl_GetCurrentThread();
    }
    Tcl_MutexUnlock(&threadMutex);

    Tcl_CreateObjCommand(interp, "testthread", ThreadObjCmd, NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadObjCmd --
 *
 *	This procedure is invoked to process the "testthread" Tcl command. See
 *	the user documentation for details on what it does.
 *
 *	thread cancel ?-unwind? id ?result?
 *	thread create ?-joinable? ?script?
 *	thread send ?-async? id script
 *	thread event
 *	thread exit
 *	thread id ?-main?
 *	thread names
 *	thread wait
 *	thread errorproc proc
 *	thread join id
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
static int
ThreadObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    int option;
    static const char *const threadOptions[] = {
	"cancel", "create", "event", "exit", "id",
	"join", "names", "send", "wait", "errorproc",
	NULL
    };
    enum options {
	THREAD_CANCEL, THREAD_CREATE, THREAD_EVENT, THREAD_EXIT,
	THREAD_ID, THREAD_JOIN, THREAD_NAMES, THREAD_SEND,
	THREAD_WAIT, THREAD_ERRORPROC
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], threadOptions, "option", 0,
	    &option) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Make sure the initial thread is on the list before doing anything.
     */

    if (tsdPtr->interp == NULL) {
	Tcl_MutexLock(&threadMutex);
	tsdPtr->interp = interp;
	ListUpdateInner(tsdPtr);
	Tcl_CreateThreadExitHandler(ThreadExitProc, NULL);
	Tcl_MutexUnlock(&threadMutex);
    }

    switch ((enum options)option) {
    case THREAD_CANCEL: {
	Tcl_WideInt id;
	const char *result;
	int flags, arg;

	if ((objc < 3) || (objc > 5)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-unwind? id ?result?");
	    return TCL_ERROR;
	}
	flags = 0;
	arg = 2;
	if ((objc == 4) || (objc == 5)) {
	    if (strcmp("-unwind", Tcl_GetString(objv[arg])) == 0) {
		flags = TCL_CANCEL_UNWIND;
		arg++;
	    }
	}
	if (Tcl_GetWideIntFromObj(interp, objv[arg], &id) != TCL_OK) {
	    return TCL_ERROR;
	}
	arg++;
	if (arg < objc) {
	    result = Tcl_GetString(objv[arg]);
	} else {
	    result = NULL;
	}
	return ThreadCancel(interp, (Tcl_ThreadId) (size_t) id, result, flags);
    }
    case THREAD_CREATE: {
	const char *script;
	int joinable, len;

	if (objc == 2) {
	    /*
	     * Neither joinable nor special script
	     */

	    joinable = 0;
	    script = "testthread wait";		/* Just enter event loop */
	} else if (objc == 3) {
	    /*
	     * Possibly -joinable, then no special script, no joinable, then
	     * its a script.
	     */

	    script = Tcl_GetStringFromObj(objv[2], &len);

	    if ((len > 1) && (script[0] == '-') && (script[1] == 'j') &&
		    (0 == strncmp(script, "-joinable", (size_t) len))) {
		joinable = 1;
		script = "testthread wait";	/* Just enter event loop */
	    } else {
		/*
		 * Remember the script
		 */

		joinable = 0;
	    }
	} else if (objc == 4) {
	    /*
	     * Definitely a script available, but is the flag -joinable?
	     */

	    script = Tcl_GetStringFromObj(objv[2], &len);
	    joinable = ((len > 1) && (script[0] == '-') && (script[1] == 'j')
		    && (0 == strncmp(script, "-joinable", (size_t) len)));
	    script = Tcl_GetString(objv[3]);
	} else {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-joinable? ?script?");
	    return TCL_ERROR;
	}
	return ThreadCreate(interp, script, joinable);
    }
    case THREAD_EXIT:
	if (objc > 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	ListRemove(NULL);
	Tcl_ExitThread(0);
	return TCL_OK;
    case THREAD_ID:
	if (objc == 2 || objc == 3) {
	    Tcl_Obj *idObj;

	    /*
	     * Check if they want the main thread id or the current thread id.
	     */

	    if (objc == 2) {
		idObj = Tcl_NewWideIntObj((Tcl_WideInt)(size_t)Tcl_GetCurrentThread());
	    } else if (objc == 3
		    && strcmp("-main", Tcl_GetString(objv[2])) == 0) {
		Tcl_MutexLock(&threadMutex);
		idObj = Tcl_NewWideIntObj((Tcl_WideInt)(size_t)mainThreadId);
		Tcl_MutexUnlock(&threadMutex);
	    } else {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	    }

	    Tcl_SetObjResult(interp, idObj);
	    return TCL_OK;
	} else {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
    case THREAD_JOIN: {
	Tcl_WideInt id;
	int result, status;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "id");
	    return TCL_ERROR;
	}
	if (Tcl_GetWideIntFromObj(interp, objv[2], &id) != TCL_OK) {
	    return TCL_ERROR;
	}

	result = Tcl_JoinThread((Tcl_ThreadId)(size_t)id, &status);
	if (result == TCL_OK) {
	    Tcl_SetIntObj(Tcl_GetObjResult(interp), status);
	} else {
	    char buf[20];

	    sprintf(buf, "%" TCL_LL_MODIFIER "d", id);
	    Tcl_AppendResult(interp, "cannot join thread ", buf, NULL);
	}
	return result;
    }
    case THREAD_NAMES:
	if (objc > 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	return ThreadList(interp);
    case THREAD_SEND: {
	Tcl_WideInt id;
	const char *script;
	int wait, arg;

	if ((objc != 4) && (objc != 5)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-async? id script");
	    return TCL_ERROR;
	}
	if (objc == 5) {
	    if (strcmp("-async", Tcl_GetString(objv[2])) != 0) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-async? id script");
		return TCL_ERROR;
	    }
	    wait = 0;
	    arg = 3;
	} else {
	    wait = 1;
	    arg = 2;
	}
	if (Tcl_GetWideIntFromObj(interp, objv[arg], &id) != TCL_OK) {
	    return TCL_ERROR;
	}
	arg++;
	script = Tcl_GetString(objv[arg]);
	return ThreadSend(interp, (Tcl_ThreadId)(size_t)id, script, wait);
    }
    case THREAD_EVENT: {
	if (objc > 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(
		Tcl_DoOneEvent(TCL_ALL_EVENTS | TCL_DONT_WAIT)));
	return TCL_OK;
    }
    case THREAD_ERRORPROC: {
	/*
	 * Arrange for this proc to handle thread death errors.
	 */

	const char *proc;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "proc");
	    return TCL_ERROR;
	}
	Tcl_MutexLock(&threadMutex);
	errorThreadId = Tcl_GetCurrentThread();
	if (errorProcString) {
	    ckfree(errorProcString);
	}
	proc = Tcl_GetString(objv[2]);
	errorProcString = ckalloc(strlen(proc) + 1);
	strcpy(errorProcString, proc);
	Tcl_MutexUnlock(&threadMutex);
	return TCL_OK;
    }
    case THREAD_WAIT:
	if (objc > 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, "");
	    return TCL_ERROR;
	}
	while (1) {
	    /*
	     * If the script has been unwound, bail out immediately. This does
	     * not follow the recommended guidelines for how extensions should
	     * handle the script cancellation functionality because this is
	     * not a "normal" extension. Most extensions do not have a command
	     * that simply enters an infinite Tcl event loop. Normal
	     * extensions should not specify the TCL_CANCEL_UNWIND when
	     * calling Tcl_Canceled to check if the command has been canceled.
	     */

	    if (Tcl_Canceled(interp,
		    TCL_LEAVE_ERR_MSG | TCL_CANCEL_UNWIND) == TCL_ERROR) {
		break;
	    }
	    (void) Tcl_DoOneEvent(TCL_ALL_EVENTS);
	}

	/*
	 * If we get to this point, we have been canceled by another thread,
	 * which is considered to be an "error".
	 */

	ThreadErrorProc(interp);
	return TCL_OK;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadCreate --
 *
 *	This procedure is invoked to create a thread containing an interp to
 *	run a script. This returns after the thread has started executing.
 *
 * Results:
 *	A standard Tcl result, which is the thread ID.
 *
 * Side effects:
 *	Create a thread.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ThreadCreate(
    Tcl_Interp *interp,		/* Current interpreter. */
    const char *script,		/* Script to execute */
    int joinable)		/* Flag, joinable thread or not */
{
    ThreadCtrl ctrl;
    Tcl_ThreadId id;

    ctrl.script = script;
    ctrl.condWait = NULL;
    ctrl.flags = 0;

    joinable = joinable ? TCL_THREAD_JOINABLE : TCL_THREAD_NOFLAGS;

    Tcl_MutexLock(&threadMutex);
    if (Tcl_CreateThread(&id, NewTestThread, (ClientData) &ctrl,
	    TCL_THREAD_STACK_DEFAULT, joinable) != TCL_OK) {
	Tcl_MutexUnlock(&threadMutex);
	Tcl_AppendResult(interp, "can't create a new thread", NULL);
	return TCL_ERROR;
    }

    /*
     * Wait for the thread to start because it is using something on our stack!
     */

    Tcl_ConditionWait(&ctrl.condWait, &threadMutex, NULL);
    Tcl_MutexUnlock(&threadMutex);
    Tcl_ConditionFinalize(&ctrl.condWait);
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt)(size_t)id));
    return TCL_OK;
}

/*
 *------------------------------------------------------------------------
 *
 * NewTestThread --
 *
 *	This routine is the "main()" for a new thread whose task is to execute
 *	a single Tcl script. The argument to this function is a pointer to a
 *	structure that contains the text of the TCL script to be executed.
 *
 *	Space to hold the script field of the ThreadControl structure passed
 *	in as the only argument was obtained from malloc() and must be freed
 *	by this function before it exits. Space to hold the ThreadControl
 *	structure itself is released by the calling function, and the two
 *	condition variables in the ThreadControl structure are destroyed by
 *	the calling function. The calling function will destroy the
 *	ThreadControl structure and the condition variable as soon as
 *	ctrlPtr->condWait is signaled, so this routine must make copies of any
 *	data it might need after that point.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	A Tcl script is executed in a new thread.
 *
 *------------------------------------------------------------------------
 */

Tcl_ThreadCreateType
NewTestThread(
    ClientData clientData)
{
    ThreadCtrl *ctrlPtr = clientData;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    int result;
    char *threadEvalScript;

    /*
     * Initialize the interpreter. This should be more general.
     */

    tsdPtr->interp = Tcl_CreateInterp();
    result = Tcl_Init(tsdPtr->interp);
    if (result != TCL_OK) {
	ThreadErrorProc(tsdPtr->interp);
    }

    /*
     * This is part of the test facility. Initialize _ALL_ test commands for
     * use by the new thread.
     */

    result = Tcltest_Init(tsdPtr->interp);
    if (result != TCL_OK) {
	ThreadErrorProc(tsdPtr->interp);
    }

    /*
     * Update the list of threads.
     */

    Tcl_MutexLock(&threadMutex);
    ListUpdateInner(tsdPtr);

    /*
     * We need to keep a pointer to the alloc'ed mem of the script we are
     * eval'ing, for the case that we exit during evaluation
     */

    threadEvalScript = ckalloc(strlen(ctrlPtr->script) + 1);
    strcpy(threadEvalScript, ctrlPtr->script);

    Tcl_CreateThreadExitHandler(ThreadExitProc, threadEvalScript);

    /*
     * Notify the parent we are alive.
     */

    Tcl_ConditionNotify(&ctrlPtr->condWait);
    Tcl_MutexUnlock(&threadMutex);

    /*
     * Run the script.
     */

    Tcl_Preserve(tsdPtr->interp);
    result = Tcl_EvalEx(tsdPtr->interp, threadEvalScript, -1, 0);
    if (result != TCL_OK) {
	ThreadErrorProc(tsdPtr->interp);
    }

    /*
     * Clean up.
     */

    Tcl_DeleteInterp(tsdPtr->interp);
    Tcl_Release(tsdPtr->interp);
    ListRemove(tsdPtr);
    Tcl_ExitThread(result);

    TCL_THREAD_CREATE_RETURN;
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadErrorProc --
 *
 *	Send a message to the thread willing to hear about errors.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Send an event.
 *
 *------------------------------------------------------------------------
 */

static void
ThreadErrorProc(
    Tcl_Interp *interp)		/* Interp that failed */
{
    Tcl_Channel errChannel;
    const char *errorInfo, *argv[3];
    char *script;
    char buf[TCL_DOUBLE_SPACE+1];

    sprintf(buf, "%" TCL_LL_MODIFIER "d", (Tcl_WideInt)(size_t)Tcl_GetCurrentThread());

    errorInfo = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    if (errorProcString == NULL) {
	errChannel = Tcl_GetStdChannel(TCL_STDERR);
	Tcl_WriteChars(errChannel, "Error from thread ", -1);
	Tcl_WriteChars(errChannel, buf, -1);
	Tcl_WriteChars(errChannel, "\n", 1);
	Tcl_WriteChars(errChannel, errorInfo, -1);
	Tcl_WriteChars(errChannel, "\n", 1);
    } else {
	argv[0] = errorProcString;
	argv[1] = buf;
	argv[2] = errorInfo;
	script = Tcl_Merge(3, argv);
	ThreadSend(interp, errorThreadId, script, 0);
	ckfree(script);
    }
}


/*
 *------------------------------------------------------------------------
 *
 * ListUpdateInner --
 *
 *	Add the thread local storage to the list. This assumes the caller has
 *	obtained the mutex.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Add the thread local storage to its list.
 *
 *------------------------------------------------------------------------
 */

static void
ListUpdateInner(
    ThreadSpecificData *tsdPtr)
{
    if (tsdPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);
    }
    tsdPtr->threadId = Tcl_GetCurrentThread();
    tsdPtr->nextPtr = threadList;
    if (threadList) {
	threadList->prevPtr = tsdPtr;
    }
    tsdPtr->prevPtr = NULL;
    threadList = tsdPtr;
}

/*
 *------------------------------------------------------------------------
 *
 * ListRemove --
 *
 *	Remove the thread local storage from its list. This grabs the mutex to
 *	protect the list.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Remove the thread local storage from its list.
 *
 *------------------------------------------------------------------------
 */

static void
ListRemove(
    ThreadSpecificData *tsdPtr)
{
    if (tsdPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);
    }
    Tcl_MutexLock(&threadMutex);
    if (tsdPtr->prevPtr) {
	tsdPtr->prevPtr->nextPtr = tsdPtr->nextPtr;
    } else {
	threadList = tsdPtr->nextPtr;
    }
    if (tsdPtr->nextPtr) {
	tsdPtr->nextPtr->prevPtr = tsdPtr->prevPtr;
    }
    tsdPtr->nextPtr = tsdPtr->prevPtr = 0;
    tsdPtr->interp = NULL;
    Tcl_MutexUnlock(&threadMutex);
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadList --
 *
 *    Return a list of threads running Tcl interpreters.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------
 */
static int
ThreadList(
    Tcl_Interp *interp)
{
    ThreadSpecificData *tsdPtr;
    Tcl_Obj *listPtr;

    listPtr = Tcl_NewListObj(0, NULL);
    Tcl_MutexLock(&threadMutex);
    for (tsdPtr = threadList ; tsdPtr ; tsdPtr = tsdPtr->nextPtr) {
	Tcl_ListObjAppendElement(interp, listPtr,
		Tcl_NewWideIntObj((Tcl_WideInt)(size_t)tsdPtr->threadId));
    }
    Tcl_MutexUnlock(&threadMutex);
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadSend --
 *
 *    Send a script to another thread.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------
 */

static int
ThreadSend(
    Tcl_Interp *interp,		/* The current interpreter. */
    Tcl_ThreadId id,		/* Thread Id of other interpreter. */
    const char *script,		/* The script to evaluate. */
    int wait)			/* If 1, we block for the result. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ThreadEvent *threadEventPtr;
    ThreadEventResult *resultPtr;
    int found, code;
    Tcl_ThreadId threadId = (Tcl_ThreadId) id;

    /*
     * Verify the thread exists.
     */

    Tcl_MutexLock(&threadMutex);
    found = 0;
    for (tsdPtr = threadList ; tsdPtr ; tsdPtr = tsdPtr->nextPtr) {
	if (tsdPtr->threadId == threadId) {
	    found = 1;
	    break;
	}
    }
    if (!found) {
	Tcl_MutexUnlock(&threadMutex);
	Tcl_AppendResult(interp, "invalid thread id", NULL);
	return TCL_ERROR;
    }

    /*
     * Short circut sends to ourself. Ought to do something with -async, like
     * run in an idle handler.
     */

    if (threadId == Tcl_GetCurrentThread()) {
	Tcl_MutexUnlock(&threadMutex);
	return Tcl_EvalEx(interp, script,-1,TCL_EVAL_GLOBAL);
    }

    /*
     * Create the event for its event queue.
     */

    threadEventPtr = ckalloc(sizeof(ThreadEvent));
    threadEventPtr->script = ckalloc(strlen(script) + 1);
    strcpy(threadEventPtr->script, script);
    if (!wait) {
	resultPtr = threadEventPtr->resultPtr = NULL;
    } else {
	resultPtr = ckalloc(sizeof(ThreadEventResult));
	threadEventPtr->resultPtr = resultPtr;

	/*
	 * Initialize the result fields.
	 */

	resultPtr->done = NULL;
	resultPtr->code = 0;
	resultPtr->result = NULL;
	resultPtr->errorInfo = NULL;
	resultPtr->errorCode = NULL;

	/*
	 * Maintain the cleanup list.
	 */

	resultPtr->srcThreadId = Tcl_GetCurrentThread();
	resultPtr->dstThreadId = threadId;
	resultPtr->eventPtr = threadEventPtr;
	resultPtr->nextPtr = resultList;
	if (resultList) {
	    resultList->prevPtr = resultPtr;
	}
	resultPtr->prevPtr = NULL;
	resultList = resultPtr;
    }

    /*
     * Queue the event and poke the other thread's notifier.
     */

    threadEventPtr->event.proc = ThreadEventProc;
    Tcl_ThreadQueueEvent(threadId, (Tcl_Event *) threadEventPtr,
	    TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(threadId);

    if (!wait) {
	Tcl_MutexUnlock(&threadMutex);
	return TCL_OK;
    }

    /*
     * Block on the results and then get them.
     */

    Tcl_ResetResult(interp);
    while (resultPtr->result == NULL) {
	Tcl_ConditionWait(&resultPtr->done, &threadMutex, NULL);
    }

    /*
     * Unlink result from the result list.
     */

    if (resultPtr->prevPtr) {
	resultPtr->prevPtr->nextPtr = resultPtr->nextPtr;
    } else {
	resultList = resultPtr->nextPtr;
    }
    if (resultPtr->nextPtr) {
	resultPtr->nextPtr->prevPtr = resultPtr->prevPtr;
    }
    resultPtr->eventPtr = NULL;
    resultPtr->nextPtr = NULL;
    resultPtr->prevPtr = NULL;

    Tcl_MutexUnlock(&threadMutex);

    if (resultPtr->code != TCL_OK) {
	if (resultPtr->errorCode) {
	    Tcl_SetErrorCode(interp, resultPtr->errorCode, NULL);
	    ckfree(resultPtr->errorCode);
	}
	if (resultPtr->errorInfo) {
	    Tcl_AddErrorInfo(interp, resultPtr->errorInfo);
	    ckfree(resultPtr->errorInfo);
	}
    }
    Tcl_AppendResult(interp, resultPtr->result, NULL);
    Tcl_ConditionFinalize(&resultPtr->done);
    code = resultPtr->code;

    ckfree(resultPtr->result);
    ckfree(resultPtr);

    return code;
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadCancel --
 *
 *    Cancels a script in another thread.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------
 */

static int
ThreadCancel(
    Tcl_Interp *interp,		/* The current interpreter. */
    Tcl_ThreadId id,		/* Thread Id of other interpreter. */
    const char *result,		/* The result or NULL for default. */
    int flags)			/* Flags for Tcl_CancelEval. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    int found;
    Tcl_ThreadId threadId = (Tcl_ThreadId) id;

    /*
     * Verify the thread exists.
     */

    Tcl_MutexLock(&threadMutex);
    found = 0;
    for (tsdPtr = threadList ; tsdPtr ; tsdPtr = tsdPtr->nextPtr) {
	if (tsdPtr->threadId == threadId) {
	    found = 1;
	    break;
	}
    }
    if (!found) {
	Tcl_MutexUnlock(&threadMutex);
	Tcl_AppendResult(interp, "invalid thread id", NULL);
	return TCL_ERROR;
    }

    /*
     * Since Tcl_CancelEval can be safely called from any thread,
     * we do it now.
     */

    Tcl_MutexUnlock(&threadMutex);
    Tcl_ResetResult(interp);
    return Tcl_CancelEval(tsdPtr->interp,
    	(result != NULL) ? Tcl_NewStringObj(result, -1) : NULL, 0, flags);
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadEventProc --
 *
 *    Handle the event in the target thread.
 *
 * Results:
 *    Returns 1 to indicate that the event was processed.
 *
 * Side effects:
 *    Fills out the ThreadEventResult struct.
 *
 *------------------------------------------------------------------------
 */

static int
ThreadEventProc(
    Tcl_Event *evPtr,		/* Really ThreadEvent */
    int mask)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ThreadEvent *threadEventPtr = (ThreadEvent *) evPtr;
    ThreadEventResult *resultPtr = threadEventPtr->resultPtr;
    Tcl_Interp *interp = tsdPtr->interp;
    int code;
    const char *result, *errorCode, *errorInfo;

    if (interp == NULL) {
	code = TCL_ERROR;
	result = "no target interp!";
	errorCode = "THREAD";
	errorInfo = "";
    } else {
	Tcl_Preserve(interp);
	Tcl_ResetResult(interp);
	Tcl_CreateThreadExitHandler(ThreadFreeProc, threadEventPtr->script);
	code = Tcl_EvalEx(interp, threadEventPtr->script,-1,TCL_EVAL_GLOBAL);
	Tcl_DeleteThreadExitHandler(ThreadFreeProc, threadEventPtr->script);
	if (code != TCL_OK) {
	    errorCode = Tcl_GetVar(interp, "errorCode", TCL_GLOBAL_ONLY);
	    errorInfo = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
	} else {
	    errorCode = errorInfo = NULL;
	}
	result = Tcl_GetStringResult(interp);
    }
    ckfree(threadEventPtr->script);
    if (resultPtr) {
	Tcl_MutexLock(&threadMutex);
	resultPtr->code = code;
	resultPtr->result = ckalloc(strlen(result) + 1);
	strcpy(resultPtr->result, result);
	if (errorCode != NULL) {
	    resultPtr->errorCode = ckalloc(strlen(errorCode) + 1);
	    strcpy(resultPtr->errorCode, errorCode);
	}
	if (errorInfo != NULL) {
	    resultPtr->errorInfo = ckalloc(strlen(errorInfo) + 1);
	    strcpy(resultPtr->errorInfo, errorInfo);
	}
	Tcl_ConditionNotify(&resultPtr->done);
	Tcl_MutexUnlock(&threadMutex);
    }
    if (interp != NULL) {
	Tcl_Release(interp);
    }
    return 1;
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadFreeProc --
 *
 *    This is called from when we are exiting and memory needs
 *    to be freed.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *	Clears up mem specified in ClientData
 *
 *------------------------------------------------------------------------
 */

     /* ARGSUSED */
static void
ThreadFreeProc(
    ClientData clientData)
{
    if (clientData) {
	ckfree(clientData);
    }
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadDeleteEvent --
 *
 *    This is called from the ThreadExitProc to delete memory related
 *    to events that we put on the queue.
 *
 * Results:
 *    1 it was our event and we want it removed, 0 otherwise.
 *
 * Side effects:
 *	It cleans up our events in the event queue for this thread.
 *
 *------------------------------------------------------------------------
 */

     /* ARGSUSED */
static int
ThreadDeleteEvent(
    Tcl_Event *eventPtr,	/* Really ThreadEvent */
    ClientData clientData)	/* dummy */
{
    if (eventPtr->proc == ThreadEventProc) {
	ckfree(((ThreadEvent *) eventPtr)->script);
	return 1;
    }

    /*
     * If it was NULL, we were in the middle of servicing the event and it
     * should be removed
     */

    return (eventPtr->proc == NULL);
}

/*
 *------------------------------------------------------------------------
 *
 * ThreadExitProc --
 *
 *    This is called when the thread exits.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *	It unblocks anyone that is waiting on a send to this thread. It cleans
 *	up any events in the event queue for this thread.
 *
 *------------------------------------------------------------------------
 */

     /* ARGSUSED */
static void
ThreadExitProc(
    ClientData clientData)
{
    char *threadEvalScript = clientData;
    ThreadEventResult *resultPtr, *nextPtr;
    Tcl_ThreadId self = Tcl_GetCurrentThread();
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->interp != NULL) {
	ListRemove(tsdPtr);
    }

    Tcl_MutexLock(&threadMutex);

    if (self == errorThreadId) {
	if (errorProcString) {	/* Extra safety */
	    ckfree(errorProcString);
	    errorProcString = NULL;
	}
	errorThreadId = 0;
    }

    if (threadEvalScript) {
	ckfree(threadEvalScript);
	threadEvalScript = NULL;
    }
    Tcl_DeleteEvents((Tcl_EventDeleteProc *) ThreadDeleteEvent, NULL);

    for (resultPtr = resultList ; resultPtr ; resultPtr = nextPtr) {
	nextPtr = resultPtr->nextPtr;
	if (resultPtr->srcThreadId == self) {
	    /*
	     * We are going away. By freeing up the result we signal to the
	     * other thread we don't care about the result.
	     */

	    if (resultPtr->prevPtr) {
		resultPtr->prevPtr->nextPtr = resultPtr->nextPtr;
	    } else {
		resultList = resultPtr->nextPtr;
	    }
	    if (resultPtr->nextPtr) {
		resultPtr->nextPtr->prevPtr = resultPtr->prevPtr;
	    }
	    resultPtr->nextPtr = resultPtr->prevPtr = 0;
	    resultPtr->eventPtr->resultPtr = NULL;
	    ckfree(resultPtr);
	} else if (resultPtr->dstThreadId == self) {
	    /*
	     * Dang. The target is going away. Unblock the caller. The result
	     * string must be dynamically allocated because the main thread is
	     * going to call free on it.
	     */

	    const char *msg = "target thread died";

	    resultPtr->result = ckalloc(strlen(msg) + 1);
	    strcpy(resultPtr->result, msg);
	    resultPtr->code = TCL_ERROR;
	    Tcl_ConditionNotify(&resultPtr->done);
	}
    }
    Tcl_MutexUnlock(&threadMutex);
}
#endif /* TCL_THREADS */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
