/*
 * tclThreadJoin.c --
 *
 *	This file implements a platform independent emulation layer for the
 *	handling of joinable threads. The Windows platform uses this code to
 *	provide the functionality of joining threads.  This code is currently
 *	not necessary on Unix.
 *
 * Copyright (c) 2000 by Scriptics Corporation
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

#ifdef _WIN32

/*
 * The information about each joinable thread is remembered in a structure as
 * defined below.
 */

typedef struct JoinableThread {
    Tcl_ThreadId  id;		/* The id of the joinable thread. */
    int result;			/* A place for the result after the demise of
				 * the thread. */
    int done;			/* Boolean flag. Initialized to 0 and set to 1
				 * after the exit of the thread. This allows a
				 * thread requesting a join to detect when
				 * waiting is not necessary. */
    int waitedUpon;		/* Boolean flag. Initialized to 0 and set to 1
				 * by the thread waiting for this one via
				 * Tcl_JoinThread.  Used to lock any other
				 * thread trying to wait on this one. */
    Tcl_Mutex threadMutex;	/* The mutex used to serialize access to this
				 * structure. */
    Tcl_Condition cond;		/* This is the condition a thread has to wait
				 * upon to get notified of the end of the
				 * described thread. It is signaled indirectly
				 * by Tcl_ExitThread. */
    struct JoinableThread *nextThreadPtr;
				/* Reference to the next thread in the list of
				 * joinable threads. */
} JoinableThread;

/*
 * The following variable is used to maintain the global list of all joinable
 * threads. Usage by a thread is allowed only if the thread acquired the
 * 'joinMutex'.
 */

TCL_DECLARE_MUTEX(joinMutex)

static JoinableThread *firstThreadPtr;

/*
 *----------------------------------------------------------------------
 *
 * TclJoinThread --
 *
 *	This procedure waits for the exit of the thread with the specified id
 *	and returns its result.
 *
 * Results:
 *	A standard tcl result signaling the overall success/failure of the
 *	operation and an integer result delivered by the thread which was
 *	waited upon.
 *
 * Side effects:
 *	Deallocates the memory allocated by TclRememberJoinableThread.
 *	Removes the data associated to the thread waited upon from the list of
 *	joinable threads.
 *
 *----------------------------------------------------------------------
 */

int
TclJoinThread(
    Tcl_ThreadId id,		/* The id of the thread to wait upon. */
    int *result)		/* Reference to a location for the result of
				 * the thread we are waiting upon. */
{
    JoinableThread *threadPtr;

    /*
     * Steps done here:
     * i.    Acquire the joinMutex and search for the thread.
     * ii.   Error out if it could not be found.
     * iii.  If found, switch from exclusive access to the list to exclusive
     *	     access to the thread structure.
     * iv.   Error out if some other is already waiting.
     * v.    Skip the waiting part of the thread is already done.
     * vi.   Wait for the thread to exit, mark it as waited upon too.
     * vii.  Get the result form the structure,
     * viii. switch to exclusive access of the list,
     * ix.   remove the structure from the list,
     * x.    then switch back to exclusive access to the structure
     * xi.   and delete it.
     */

    Tcl_MutexLock(&joinMutex);

    threadPtr = firstThreadPtr;
    while (threadPtr!=NULL && threadPtr->id!=id) {
	threadPtr = threadPtr->nextThreadPtr;
    }

    if (threadPtr == NULL) {
	/*
	 * Thread not found. Either not joinable, or already waited upon and
	 * exited. Whatever, an error is in order.
	 */

	Tcl_MutexUnlock(&joinMutex);
	return TCL_ERROR;
    }

    /*
     * [1] If we don't lock the structure before giving up exclusive access to
     * the list some other thread just completing its wait on the same thread
     * can delete the structure from under us, leaving us with a dangling
     * pointer.
     */

    Tcl_MutexLock(&threadPtr->threadMutex);
    Tcl_MutexUnlock(&joinMutex);

    /*
     * [2] Now that we have the structure mutex any other thread that just
     * tries to delete structure will wait at location [3] until we are done
     * with the structure. And in that case we are done with it rather quickly
     * as 'waitedUpon' will be set and we will have to error out.
     */

    if (threadPtr->waitedUpon) {
	Tcl_MutexUnlock(&threadPtr->threadMutex);
	return TCL_ERROR;
    }

    /*
     * We are waiting now, let other threads recognize this.
     */

    threadPtr->waitedUpon = 1;

    while (!threadPtr->done) {
	Tcl_ConditionWait(&threadPtr->cond, &threadPtr->threadMutex, NULL);
    }

    /*
     * We have to release the structure before trying to access the list again
     * or we can run into deadlock with a thread at [1] (see above) because of
     * us holding the structure and the other holding the list.  There is no
     * problem with dangling pointers here as 'waitedUpon == 1' is still valid
     * and any other thread will error out and not come to this place. IOW,
     * the fact that we are here also means that no other thread came here
     * before us and is able to delete the structure.
     */

    Tcl_MutexUnlock(&threadPtr->threadMutex);
    Tcl_MutexLock(&joinMutex);

    /*
     * We have to search the list again as its structure may (may, almost
     * certainly) have changed while we were waiting. Especially now is the
     * time to compute the predecessor in the list. Any earlier result can be
     * dangling by now.
     */

    if (firstThreadPtr == threadPtr) {
	firstThreadPtr = threadPtr->nextThreadPtr;
    } else {
	JoinableThread *prevThreadPtr = firstThreadPtr;

	while (prevThreadPtr->nextThreadPtr != threadPtr) {
	    prevThreadPtr = prevThreadPtr->nextThreadPtr;
	}
	prevThreadPtr->nextThreadPtr = threadPtr->nextThreadPtr;
    }

    Tcl_MutexUnlock(&joinMutex);

    /*
     * [3] Now that the structure is not part of the list anymore no other
     * thread can acquire its mutex from now on. But it is possible that
     * another thread is still holding the mutex though, see location [2].  So
     * we have to acquire the mutex one more time to wait for that thread to
     * finish. We can (and have to) release the mutex immediately.
     */

    Tcl_MutexLock(&threadPtr->threadMutex);
    Tcl_MutexUnlock(&threadPtr->threadMutex);

    /*
     * Copy the result to us, finalize the synchronisation objects, then free
     * the structure and return.
     */

    *result = threadPtr->result;

    Tcl_ConditionFinalize(&threadPtr->cond);
    Tcl_MutexFinalize(&threadPtr->threadMutex);
    ckfree(threadPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclRememberJoinableThread --
 *
 *	This procedure remebers a thread as joinable. Only a call to
 *	TclJoinThread will remove the structre created (and initialized) here.
 *	IOW, not waiting upon a joinable thread will cause memory leaks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory, adds it to the global list of all joinable threads.
 *
 *----------------------------------------------------------------------
 */

void
TclRememberJoinableThread(
    Tcl_ThreadId id)		/* The thread to remember as joinable */
{
    JoinableThread *threadPtr;

    threadPtr = ckalloc(sizeof(JoinableThread));
    threadPtr->id = id;
    threadPtr->done = 0;
    threadPtr->waitedUpon = 0;
    threadPtr->threadMutex = (Tcl_Mutex) NULL;
    threadPtr->cond = (Tcl_Condition) NULL;

    Tcl_MutexLock(&joinMutex);

    threadPtr->nextThreadPtr = firstThreadPtr;
    firstThreadPtr = threadPtr;

    Tcl_MutexUnlock(&joinMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TclSignalExitThread --
 *
 *	This procedure signals that the specified thread is done with its
 *	work. If the thread is joinable this signal is propagated to the
 *	thread waiting upon it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the associated structure to hold the result.
 *
 *----------------------------------------------------------------------
 */

void
TclSignalExitThread(
    Tcl_ThreadId id,		/* Id of the thread signaling its exit. */
    int result)			/* The result from the thread. */
{
    JoinableThread *threadPtr;

    Tcl_MutexLock(&joinMutex);

    threadPtr = firstThreadPtr;
    while ((threadPtr != NULL) && (threadPtr->id != id)) {
	threadPtr = threadPtr->nextThreadPtr;
    }

    if (threadPtr == NULL) {
	/*
	 * Thread not found. Not joinable. No problem, nothing to do.
	 */

	Tcl_MutexUnlock(&joinMutex);
	return;
    }

    /*
     * Switch over the exclusive access from the list to the structure, then
     * store the result, set the flag and notify the waiting thread, provided
     * that it exists. The order of lock/unlock ensures that a thread entering
     * 'TclJoinThread' will not interfere with us.
     */

    Tcl_MutexLock(&threadPtr->threadMutex);
    Tcl_MutexUnlock(&joinMutex);

    threadPtr->done = 1;
    threadPtr->result = result;

    if (threadPtr->waitedUpon) {
	Tcl_ConditionNotify(&threadPtr->cond);
    }

    Tcl_MutexUnlock(&threadPtr->threadMutex);
}
#endif /* _WIN32 */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
