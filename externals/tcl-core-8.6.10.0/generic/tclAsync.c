/*
 * tclAsync.c --
 *
 *	This file provides low-level support needed to invoke signal handlers
 *	in a safe way. The code here doesn't actually handle signals, though.
 *	This code is based on proposals made by Mark Diekhans and Don Libes.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/* Forward declaration */
struct ThreadSpecificData;

/*
 * One of the following structures exists for each asynchronous handler:
 */

typedef struct AsyncHandler {
    int ready;			/* Non-zero means this handler should be
				 * invoked in the next call to
				 * Tcl_AsyncInvoke. */
    struct AsyncHandler *nextPtr;
				/* Next in list of all handlers for the
				 * process. */
    Tcl_AsyncProc *proc;	/* Procedure to call when handler is
				 * invoked. */
    ClientData clientData;	/* Value to pass to handler when it is
				 * invoked. */
    struct ThreadSpecificData *originTsd;
				/* Used in Tcl_AsyncMark to modify thread-
				 * specific data from outside the thread it is
				 * associated to. */
    Tcl_ThreadId originThrdId;	/* Origin thread where this token was created
				 * and where it will be yielded. */
} AsyncHandler;

typedef struct ThreadSpecificData {
    /*
     * The variables below maintain a list of all existing handlers specific
     * to the calling thread.
     */
    AsyncHandler *firstHandler;	/* First handler defined for process, or NULL
				 * if none. */
    AsyncHandler *lastHandler;	/* Last handler or NULL. */
    int asyncReady;		/* This is set to 1 whenever a handler becomes
				 * ready and it is cleared to zero whenever
				 * Tcl_AsyncInvoke is called. It can be
				 * checked elsewhere in the application by
				 * calling Tcl_AsyncReady to see if
				 * Tcl_AsyncInvoke should be invoked. */
    int asyncActive;		/* Indicates whether Tcl_AsyncInvoke is
				 * currently working. If so then we won't set
				 * asyncReady again until Tcl_AsyncInvoke
				 * returns. */
    Tcl_Mutex asyncMutex;	/* Thread-specific AsyncHandler linked-list
				 * lock */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeAsync --
 *
 *	Finalizes the mutex in the thread local data structure for the async
 *	subsystem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets knowledge of the mutex should it have been created.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeAsync(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->asyncMutex != NULL) {
	Tcl_MutexFinalize(&tsdPtr->asyncMutex);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AsyncCreate --
 *
 *	This procedure creates the data structures for an asynchronous
 *	handler, so that no memory has to be allocated when the handler is
 *	activated.
 *
 * Results:
 *	The return value is a token for the handler, which can be used to
 *	activate it later on.
 *
 * Side effects:
 *	Information about the handler is recorded.
 *
 *----------------------------------------------------------------------
 */

Tcl_AsyncHandler
Tcl_AsyncCreate(
    Tcl_AsyncProc *proc,	/* Procedure to call when handler is
				 * invoked. */
    ClientData clientData)	/* Argument to pass to handler. */
{
    AsyncHandler *asyncPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    asyncPtr = ckalloc(sizeof(AsyncHandler));
    asyncPtr->ready = 0;
    asyncPtr->nextPtr = NULL;
    asyncPtr->proc = proc;
    asyncPtr->clientData = clientData;
    asyncPtr->originTsd = tsdPtr;
    asyncPtr->originThrdId = Tcl_GetCurrentThread();

    Tcl_MutexLock(&tsdPtr->asyncMutex);
    if (tsdPtr->firstHandler == NULL) {
	tsdPtr->firstHandler = asyncPtr;
    } else {
	tsdPtr->lastHandler->nextPtr = asyncPtr;
    }
    tsdPtr->lastHandler = asyncPtr;
    Tcl_MutexUnlock(&tsdPtr->asyncMutex);
    return (Tcl_AsyncHandler) asyncPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AsyncMark --
 *
 *	This procedure is called to request that an asynchronous handler be
 *	invoked as soon as possible. It's typically called from an interrupt
 *	handler, where it isn't safe to do anything that depends on or
 *	modifies application state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The handler gets marked for invocation later.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AsyncMark(
    Tcl_AsyncHandler async)		/* Token for handler. */
{
    AsyncHandler *token = (AsyncHandler *) async;

    Tcl_MutexLock(&token->originTsd->asyncMutex);
    token->ready = 1;
    if (!token->originTsd->asyncActive) {
	token->originTsd->asyncReady = 1;
	Tcl_ThreadAlert(token->originThrdId);
    }
    Tcl_MutexUnlock(&token->originTsd->asyncMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AsyncInvoke --
 *
 *	This procedure is called at a "safe" time at background level to
 *	invoke any active asynchronous handlers.
 *
 * Results:
 *	The return value is a normal Tcl result, which is intended to replace
 *	the code argument as the current completion code for interp.
 *
 * Side effects:
 *	Depends on the handlers that are active.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AsyncInvoke(
    Tcl_Interp *interp,		/* If invoked from Tcl_Eval just after
				 * completing a command, points to
				 * interpreter. Otherwise it is NULL. */
    int code)			/* If interp is non-NULL, this gives
				 * completion code from command that just
				 * completed. */
{
    AsyncHandler *asyncPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    Tcl_MutexLock(&tsdPtr->asyncMutex);

    if (tsdPtr->asyncReady == 0) {
	Tcl_MutexUnlock(&tsdPtr->asyncMutex);
	return code;
    }
    tsdPtr->asyncReady = 0;
    tsdPtr->asyncActive = 1;
    if (interp == NULL) {
	code = 0;
    }

    /*
     * Make one or more passes over the list of handlers, invoking at most one
     * handler in each pass. After invoking a handler, go back to the start of
     * the list again so that (a) if a new higher-priority handler gets marked
     * while executing a lower priority handler, we execute the higher-
     * priority handler next, and (b) if a handler gets deleted during the
     * execution of a handler, then the list structure may change so it isn't
     * safe to continue down the list anyway.
     */

    while (1) {
	for (asyncPtr = tsdPtr->firstHandler; asyncPtr != NULL;
		asyncPtr = asyncPtr->nextPtr) {
	    if (asyncPtr->ready) {
		break;
	    }
	}
	if (asyncPtr == NULL) {
	    break;
	}
	asyncPtr->ready = 0;
	Tcl_MutexUnlock(&tsdPtr->asyncMutex);
	code = asyncPtr->proc(asyncPtr->clientData, interp, code);
	Tcl_MutexLock(&tsdPtr->asyncMutex);
    }
    tsdPtr->asyncActive = 0;
    Tcl_MutexUnlock(&tsdPtr->asyncMutex);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AsyncDelete --
 *
 *	Frees up all the state for an asynchronous handler. The handler should
 *	never be used again.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The state associated with the handler is deleted.
 *
 *	Failure to locate the handler in current thread private list
 *	of async handlers will result in panic; exception: the list
 *	is already empty (potential trouble?).
 *	Consequently, threads should create and delete handlers
 *	themselves.  I.e. a handler created by one should not be
 *	deleted by some other thread.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AsyncDelete(
    Tcl_AsyncHandler async)		/* Token for handler to delete. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    AsyncHandler *asyncPtr = (AsyncHandler *) async;
    AsyncHandler *prevPtr, *thisPtr;

    /*
     * Assure early handling of the constraint
     */

    if (asyncPtr->originThrdId != Tcl_GetCurrentThread()) {
	Tcl_Panic("Tcl_AsyncDelete: async handler deleted by the wrong thread");
    }

    /*
     * If we come to this point when TSD's for the current
     * thread have already been garbage-collected, we are
     * in the _serious_ trouble. OTOH, we tolerate calling
     * with already cleaned-up handler list (should we?).
     */

    Tcl_MutexLock(&tsdPtr->asyncMutex);
    if (tsdPtr->firstHandler != NULL) {
	prevPtr = thisPtr = tsdPtr->firstHandler;
	while (thisPtr != NULL && thisPtr != asyncPtr) {
	    prevPtr = thisPtr;
	    thisPtr = thisPtr->nextPtr;
	}
	if (thisPtr == NULL) {
	    Tcl_Panic("Tcl_AsyncDelete: cannot find async handler");
	}
	if (asyncPtr == tsdPtr->firstHandler) {
	    tsdPtr->firstHandler = asyncPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = asyncPtr->nextPtr;
	}
	if (asyncPtr == tsdPtr->lastHandler) {
	    tsdPtr->lastHandler = prevPtr;
	}
    }
    Tcl_MutexUnlock(&tsdPtr->asyncMutex);
    ckfree(asyncPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AsyncReady --
 *
 *	This procedure can be used to tell whether Tcl_AsyncInvoke needs to be
 *	called. This procedure is the external interface for checking the
 *	thread-specific asyncReady variable.
 *
 * Results:
 *	The return value is 1 whenever a handler is ready and is 0 when no
 *	handlers are ready.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AsyncReady(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    return tsdPtr->asyncReady;
}

int *
TclGetAsyncReadyPtr(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    return &(tsdPtr->asyncReady);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
