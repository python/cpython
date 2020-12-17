/*
 * waitpid.c --
 *
 *	This procedure emulates the POSIX waitpid kernel call on BSD systems
 *	that don't have waitpid but do have wait3. This code is based on a
 *	prototype version written by Mark Diekhans and Karl Lehenbauer.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclPort.h"

#ifndef pid_t
#define pid_t int
#endif

/*
 * A linked list of the following structures is used to keep track of
 * processes for which we received notification from the kernel, but the
 * application hasn't waited for them yet (this can happen because wait may
 * not return the process we really want). We save the information here until
 * the application finally does wait for the process.
 */

typedef struct WaitInfo {
    pid_t pid;			/* Pid of process that exited. */
    WAIT_STATUS_TYPE status;	/* Status returned when child exited or
				 * suspended. */
    struct WaitInfo *nextPtr;	/* Next in list of exited processes. */
} WaitInfo;

static WaitInfo *deadList = NULL;
				/* First in list of all dead processes. */

/*
 *----------------------------------------------------------------------
 *
 * waitpid --
 *
 *	This procedure emulates the functionality of the POSIX waitpid kernel
 *	call, using the BSD wait3 kernel call. Note: it doesn't emulate
 *	absolutely all of the waitpid functionality, in that it doesn't
 *	support pid's of 0 or < -1.
 *
 * Results:
 *	-1 is returned if there is an error in the wait kernel call. Otherwise
 *	the pid of an exited or suspended process is returned and *statusPtr
 *	is set to the status value of the process.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef waitpid
#   undef waitpid
#endif

pid_t
waitpid(
    pid_t pid,			/* The pid to wait on. Must be -1 or greater
				 * than zero. */
    int *statusPtr,		/* Where to store wait status for the
				 * process. */
    int options)		/* OR'ed combination of WNOHANG and
				 * WUNTRACED. */
{
    register WaitInfo *waitPtr, *prevPtr;
    pid_t result;
    WAIT_STATUS_TYPE status;

    if ((pid < -1) || (pid == 0)) {
	errno = EINVAL;
	return -1;
    }

    /*
     * See if there's a suitable process that has already stopped or exited.
     * If so, remove it from the list of exited processes and return its
     * information.
     */

    for (waitPtr = deadList, prevPtr = NULL; waitPtr != NULL;
	    prevPtr = waitPtr, waitPtr = waitPtr->nextPtr) {
	if ((pid != waitPtr->pid) && (pid != -1)) {
	    continue;
	}
	if (!(options & WUNTRACED) && (WIFSTOPPED(waitPtr->status))) {
	    continue;
	}
	result = waitPtr->pid;
	*statusPtr = *((int *) &waitPtr->status);
	if (prevPtr == NULL) {
	    deadList = waitPtr->nextPtr;
	} else {
	    prevPtr->nextPtr = waitPtr->nextPtr;
	}
	ckfree((char *) waitPtr);
	return result;
    }

    /*
     * Wait for any process to stop or exit. If it's an acceptable one then
     * return it to the caller; otherwise store information about it in the
     * list of exited processes and try again. On systems that have only wait
     * but not wait3, there are several situations we can't handle, but we do
     * the best we can (e.g. can still handle some combinations of options by
     * invoking wait instead of wait3).
     */

    while (1) {
#if NO_WAIT3
	if (options & WNOHANG) {
	    return 0;
	}
	if (options != 0) {
	    errno = EINVAL;
	    return -1;
	}
	result = wait(&status);
#else
	result = wait3(&status, options, 0);
#endif
	if ((result == -1) && (errno == EINTR)) {
	    continue;
	}
	if (result <= 0) {
	    return result;
	}

	if ((pid != result) && (pid != -1)) {
	    goto saveInfo;
	}
	if (!(options & WUNTRACED) && (WIFSTOPPED(status))) {
	    goto saveInfo;
	}
	*statusPtr = *((int *) &status);
	return result;

	/*
	 * Can't return this info to caller. Save it in the list of stopped or
	 * exited processes. Tricky point: first check for an existing entry
	 * for the process and overwrite it if it exists (e.g. a previously
	 * stopped process might now be dead).
	 */

    saveInfo:
	for (waitPtr = deadList; waitPtr != NULL; waitPtr = waitPtr->nextPtr) {
	    if (waitPtr->pid == result) {
		waitPtr->status = status;
		goto waitAgain;
	    }
	}
	waitPtr = (WaitInfo *) ckalloc(sizeof(WaitInfo));
	waitPtr->pid = result;
	waitPtr->status = status;
	waitPtr->nextPtr = deadList;
	deadList = waitPtr;

    waitAgain:
	continue;
    }
}
