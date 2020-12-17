/*
 * tclResult.c --
 *
 *	This file contains code to manage the interpreter result.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Indices of the standard return options dictionary keys.
 */

enum returnKeys {
    KEY_CODE,	KEY_ERRORCODE,	KEY_ERRORINFO,	KEY_ERRORLINE,
    KEY_LEVEL,	KEY_OPTIONS,	KEY_ERRORSTACK,	KEY_LAST
};

/*
 * Function prototypes for local functions in this file:
 */

static Tcl_Obj **	GetKeys(void);
static void		ReleaseKeys(ClientData clientData);
static void		ResetObjResult(Interp *iPtr);
static void		SetupAppendBuffer(Interp *iPtr, int newSpace);

/*
 * This structure is used to take a snapshot of the interpreter state in
 * Tcl_SaveInterpState. You can snapshot the state, execute a command, and
 * then back up to the result or the error that was previously in progress.
 */

typedef struct InterpState {
    int status;			/* return code status */
    int flags;			/* Each remaining field saves the */
    int returnLevel;		/* corresponding field of the Interp */
    int returnCode;		/* struct. These fields taken together are */
    Tcl_Obj *errorInfo;		/* the "state" of the interp. */
    Tcl_Obj *errorCode;
    Tcl_Obj *returnOpts;
    Tcl_Obj *objResult;
    Tcl_Obj *errorStack;
    int resetErrorStack;
} InterpState;

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SaveInterpState --
 *
 *	Fills a token with a snapshot of the current state of the interpreter.
 *	The snapshot can be restored at any point by TclRestoreInterpState.
 *
 *	The token returned must be eventally passed to one of the routines
 *	TclRestoreInterpState or TclDiscardInterpState, or there will be a
 *	memory leak.
 *
 * Results:
 *	Returns a token representing the interp state.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_InterpState
Tcl_SaveInterpState(
    Tcl_Interp *interp,		/* Interpreter's state to be saved */
    int status)			/* status code for current operation */
{
    Interp *iPtr = (Interp *) interp;
    InterpState *statePtr = ckalloc(sizeof(InterpState));

    statePtr->status = status;
    statePtr->flags = iPtr->flags & ERR_ALREADY_LOGGED;
    statePtr->returnLevel = iPtr->returnLevel;
    statePtr->returnCode = iPtr->returnCode;
    statePtr->errorInfo = iPtr->errorInfo;
    statePtr->errorStack = iPtr->errorStack;
    statePtr->resetErrorStack = iPtr->resetErrorStack;
    if (statePtr->errorInfo) {
	Tcl_IncrRefCount(statePtr->errorInfo);
    }
    statePtr->errorCode = iPtr->errorCode;
    if (statePtr->errorCode) {
	Tcl_IncrRefCount(statePtr->errorCode);
    }
    statePtr->returnOpts = iPtr->returnOpts;
    if (statePtr->returnOpts) {
	Tcl_IncrRefCount(statePtr->returnOpts);
    }
    if (statePtr->errorStack) {
	Tcl_IncrRefCount(statePtr->errorStack);
    }
    statePtr->objResult = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(statePtr->objResult);
    return (Tcl_InterpState) statePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RestoreInterpState --
 *
 *	Accepts an interp and a token previously returned by
 *	Tcl_SaveInterpState. Restore the state of the interp to what it was at
 *	the time of the Tcl_SaveInterpState call.
 *
 * Results:
 *	Returns the status value originally passed in to Tcl_SaveInterpState.
 *
 * Side effects:
 *	Restores the interp state and frees memory held by token.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RestoreInterpState(
    Tcl_Interp *interp,		/* Interpreter's state to be restored. */
    Tcl_InterpState state)	/* Saved interpreter state. */
{
    Interp *iPtr = (Interp *) interp;
    InterpState *statePtr = (InterpState *) state;
    int status = statePtr->status;

    iPtr->flags &= ~ERR_ALREADY_LOGGED;
    iPtr->flags |= (statePtr->flags & ERR_ALREADY_LOGGED);

    iPtr->returnLevel = statePtr->returnLevel;
    iPtr->returnCode = statePtr->returnCode;
    iPtr->resetErrorStack = statePtr->resetErrorStack;
    if (iPtr->errorInfo) {
	Tcl_DecrRefCount(iPtr->errorInfo);
    }
    iPtr->errorInfo = statePtr->errorInfo;
    if (iPtr->errorInfo) {
	Tcl_IncrRefCount(iPtr->errorInfo);
    }
    if (iPtr->errorCode) {
	Tcl_DecrRefCount(iPtr->errorCode);
    }
    iPtr->errorCode = statePtr->errorCode;
    if (iPtr->errorCode) {
	Tcl_IncrRefCount(iPtr->errorCode);
    }
    if (iPtr->errorStack) {
	Tcl_DecrRefCount(iPtr->errorStack);
    }
    iPtr->errorStack = statePtr->errorStack;
    if (iPtr->errorStack) {
	Tcl_IncrRefCount(iPtr->errorStack);
    }
    if (iPtr->returnOpts) {
	Tcl_DecrRefCount(iPtr->returnOpts);
    }
    iPtr->returnOpts = statePtr->returnOpts;
    if (iPtr->returnOpts) {
	Tcl_IncrRefCount(iPtr->returnOpts);
    }
    Tcl_SetObjResult(interp, statePtr->objResult);
    Tcl_DiscardInterpState(state);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DiscardInterpState --
 *
 *	Accepts a token previously returned by Tcl_SaveInterpState. Frees the
 *	memory it uses.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DiscardInterpState(
    Tcl_InterpState state)	/* saved interpreter state */
{
    InterpState *statePtr = (InterpState *) state;

    if (statePtr->errorInfo) {
	Tcl_DecrRefCount(statePtr->errorInfo);
    }
    if (statePtr->errorCode) {
	Tcl_DecrRefCount(statePtr->errorCode);
    }
    if (statePtr->returnOpts) {
	Tcl_DecrRefCount(statePtr->returnOpts);
    }
    if (statePtr->errorStack) {
	Tcl_DecrRefCount(statePtr->errorStack);
    }
    Tcl_DecrRefCount(statePtr->objResult);
    ckfree(statePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SaveResult --
 *
 *	Takes a snapshot of the current result state of the interpreter. The
 *	snapshot can be restored at any point by Tcl_RestoreResult. Note that
 *	this routine does not preserve the errorCode, errorInfo, or flags
 *	fields so it should not be used if an error is in progress.
 *
 *	Once a snapshot is saved, it must be restored by calling
 *	Tcl_RestoreResult, or discarded by calling Tcl_DiscardResult.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the interpreter result.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_SaveResult
void
Tcl_SaveResult(
    Tcl_Interp *interp,		/* Interpreter to save. */
    Tcl_SavedResult *statePtr)	/* Pointer to state structure. */
{
    Interp *iPtr = (Interp *) interp;

    /*
     * Move the result object into the save state. Note that we don't need to
     * change its refcount because we're moving it, not adding a new
     * reference. Put an empty object into the interpreter.
     */

    statePtr->objResultPtr = iPtr->objResultPtr;
    iPtr->objResultPtr = Tcl_NewObj();
    Tcl_IncrRefCount(iPtr->objResultPtr);

    /*
     * Save the string result.
     */

    statePtr->freeProc = iPtr->freeProc;
    if (iPtr->result == iPtr->resultSpace) {
	/*
	 * Copy the static string data out of the interp buffer.
	 */

	statePtr->result = statePtr->resultSpace;
	strcpy(statePtr->result, iPtr->result);
	statePtr->appendResult = NULL;
    } else if (iPtr->result == iPtr->appendResult) {
	/*
	 * Move the append buffer out of the interp.
	 */

	statePtr->appendResult = iPtr->appendResult;
	statePtr->appendAvl = iPtr->appendAvl;
	statePtr->appendUsed = iPtr->appendUsed;
	statePtr->result = statePtr->appendResult;
	iPtr->appendResult = NULL;
	iPtr->appendAvl = 0;
	iPtr->appendUsed = 0;
    } else {
	/*
	 * Move the dynamic or static string out of the interpreter.
	 */

	statePtr->result = iPtr->result;
	statePtr->appendResult = NULL;
    }

    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
    iPtr->freeProc = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RestoreResult --
 *
 *	Restores the state of the interpreter to a snapshot taken by
 *	Tcl_SaveResult. After this call, the token for the interpreter state
 *	is no longer valid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Restores the interpreter result.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_RestoreResult
void
Tcl_RestoreResult(
    Tcl_Interp *interp,		/* Interpreter being restored. */
    Tcl_SavedResult *statePtr)	/* State returned by Tcl_SaveResult. */
{
    Interp *iPtr = (Interp *) interp;

    Tcl_ResetResult(interp);

    /*
     * Restore the string result.
     */

    iPtr->freeProc = statePtr->freeProc;
    if (statePtr->result == statePtr->resultSpace) {
	/*
	 * Copy the static string data into the interp buffer.
	 */

	iPtr->result = iPtr->resultSpace;
	strcpy(iPtr->result, statePtr->result);
    } else if (statePtr->result == statePtr->appendResult) {
	/*
	 * Move the append buffer back into the interp.
	 */

	if (iPtr->appendResult != NULL) {
	    ckfree(iPtr->appendResult);
	}

	iPtr->appendResult = statePtr->appendResult;
	iPtr->appendAvl = statePtr->appendAvl;
	iPtr->appendUsed = statePtr->appendUsed;
	iPtr->result = iPtr->appendResult;
    } else {
	/*
	 * Move the dynamic or static string back into the interpreter.
	 */

	iPtr->result = statePtr->result;
    }

    /*
     * Restore the object result.
     */

    Tcl_DecrRefCount(iPtr->objResultPtr);
    iPtr->objResultPtr = statePtr->objResultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DiscardResult --
 *
 *	Frees the memory associated with an interpreter snapshot taken by
 *	Tcl_SaveResult. If the snapshot is not restored, this function must be
 *	called to discard it, or the memory will be lost.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_DiscardResult
void
Tcl_DiscardResult(
    Tcl_SavedResult *statePtr)	/* State returned by Tcl_SaveResult. */
{
    TclDecrRefCount(statePtr->objResultPtr);

    if (statePtr->result == statePtr->appendResult) {
	ckfree(statePtr->appendResult);
    } else if (statePtr->freeProc == TCL_DYNAMIC) {
        ckfree(statePtr->result);
    } else if (statePtr->freeProc) {
        statePtr->freeProc(statePtr->result);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetResult --
 *
 *	Arrange for "result" to be the Tcl return value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	interp->result is left pointing either to "result" or to a copy of it.
 *	Also, the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetResult(
    Tcl_Interp *interp,		/* Interpreter with which to associate the
				 * return value. */
    register char *result,	/* Value to be returned. If NULL, the result
				 * is set to an empty string. */
    Tcl_FreeProc *freeProc)	/* Gives information about the string:
				 * TCL_STATIC, TCL_VOLATILE, or the address of
				 * a Tcl_FreeProc such as free. */
{
    Interp *iPtr = (Interp *) interp;
    register Tcl_FreeProc *oldFreeProc = iPtr->freeProc;
    char *oldResult = iPtr->result;

    if (result == NULL) {
	iPtr->resultSpace[0] = 0;
	iPtr->result = iPtr->resultSpace;
	iPtr->freeProc = 0;
    } else if (freeProc == TCL_VOLATILE) {
	int length = strlen(result);

	if (length > TCL_RESULT_SIZE) {
	    iPtr->result = ckalloc(length + 1);
	    iPtr->freeProc = TCL_DYNAMIC;
	} else {
	    iPtr->result = iPtr->resultSpace;
	    iPtr->freeProc = 0;
	}
	memcpy(iPtr->result, result, (unsigned) length+1);
    } else {
	iPtr->result = (char *) result;
	iPtr->freeProc = freeProc;
    }

    /*
     * If the old result was dynamically-allocated, free it up. Do it here,
     * rather than at the beginning, in case the new result value was part of
     * the old result value.
     */

    if (oldFreeProc != 0) {
	if (oldFreeProc == TCL_DYNAMIC) {
	    ckfree(oldResult);
	} else {
	    oldFreeProc(oldResult);
	}
    }

    /*
     * Reset the object result since we just set the string result.
     */

    ResetObjResult(iPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetStringResult --
 *
 *	Returns an interpreter's result value as a string.
 *
 * Results:
 *	The interpreter's result as a string.
 *
 * Side effects:
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_GetStringResult(
    register Tcl_Interp *interp)/* Interpreter whose result to return. */
{
    /*
     * If the string result is empty, move the object result to the string
     * result, then reset the object result.
     */

    Interp *iPtr = (Interp *) interp;

    if (*(iPtr->result) == 0) {
	Tcl_SetResult(interp, TclGetString(Tcl_GetObjResult(interp)),
		TCL_VOLATILE);
    }
    return iPtr->result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetObjResult --
 *
 *	Arrange for objPtr to be an interpreter's result value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	interp->objResultPtr is left pointing to the object referenced by
 *	objPtr. The object's reference count is incremented since there is now
 *	a new reference to it. The reference count for any old objResultPtr
 *	value is decremented. Also, the string result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetObjResult(
    Tcl_Interp *interp,		/* Interpreter with which to associate the
				 * return object value. */
    register Tcl_Obj *objPtr)	/* Tcl object to be returned. If NULL, the obj
				 * result is made an empty string object. */
{
    register Interp *iPtr = (Interp *) interp;
    register Tcl_Obj *oldObjResult = iPtr->objResultPtr;

    iPtr->objResultPtr = objPtr;
    Tcl_IncrRefCount(objPtr);	/* since interp result is a reference */

    /*
     * We wait until the end to release the old object result, in case we are
     * setting the result to itself.
     */

    TclDecrRefCount(oldObjResult);

    /*
     * Reset the string result since we just set the result object.
     */

    if (iPtr->freeProc != NULL) {
	if (iPtr->freeProc == TCL_DYNAMIC) {
	    ckfree(iPtr->result);
	} else {
	    iPtr->freeProc(iPtr->result);
	}
	iPtr->freeProc = 0;
    }
    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetObjResult --
 *
 *	Returns an interpreter's result value as a Tcl object. The object's
 *	reference count is not modified; the caller must do that if it needs
 *	to hold on to a long-term reference to it.
 *
 * Results:
 *	The interpreter's result as an object.
 *
 * Side effects:
 *	If the interpreter has a non-empty string result, the result object is
 *	either empty or stale because some function set interp->result
 *	directly. If so, the string result is moved to the result object then
 *	the string result is reset.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_GetObjResult(
    Tcl_Interp *interp)		/* Interpreter whose result to return. */
{
    register Interp *iPtr = (Interp *) interp;
    Tcl_Obj *objResultPtr;
    int length;

    /*
     * If the string result is non-empty, move the string result to the object
     * result, then reset the string result.
     */

    if (iPtr->result[0] != 0) {
	ResetObjResult(iPtr);

	objResultPtr = iPtr->objResultPtr;
	length = strlen(iPtr->result);
	TclInitStringRep(objResultPtr, iPtr->result, length);

	if (iPtr->freeProc != NULL) {
	    if (iPtr->freeProc == TCL_DYNAMIC) {
		ckfree(iPtr->result);
	    } else {
		iPtr->freeProc(iPtr->result);
	    }
	    iPtr->freeProc = 0;
	}
	iPtr->result = iPtr->resultSpace;
	iPtr->result[0] = 0;
    }
    return iPtr->objResultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendResultVA --
 *
 *	Append a variable number of strings onto the interpreter's result.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The result of the interpreter given by the first argument is extended
 *	by the strings in the va_list (up to a terminating NULL argument).
 *
 *	If the string result is non-empty, the object result forced to be a
 *	duplicate of it first. There will be a string result afterwards.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendResultVA(
    Tcl_Interp *interp,		/* Interpreter with which to associate the
				 * return value. */
    va_list argList)		/* Variable argument list. */
{
    Tcl_Obj *objPtr = Tcl_GetObjResult(interp);

    if (Tcl_IsShared(objPtr)) {
	objPtr = Tcl_DuplicateObj(objPtr);
    }
    Tcl_AppendStringsToObjVA(objPtr, argList);
    Tcl_SetObjResult(interp, objPtr);

    /*
     * Strictly we should call Tcl_GetStringResult(interp) here to make sure
     * that interp->result is correct according to the old contract, but that
     * makes the performance of much code (e.g. in Tk) absolutely awful. So we
     * leave it out; code that really wants interp->result can just insert the
     * calls to Tcl_GetStringResult() itself. [Patch 1041072 discussion]
     */

#ifdef USE_INTERP_RESULT
    /*
     * Ensure that the interp->result is legal so old Tcl 7.* code still
     * works. There's still embarrasingly much of it about...
     */

    (void) Tcl_GetStringResult(interp);
#endif /* USE_INTERP_RESULT */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendResult --
 *
 *	Append a variable number of strings onto the interpreter's result.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The result of the interpreter given by the first argument is extended
 *	by the strings given by the second and following arguments (up to a
 *	terminating NULL argument).
 *
 *	If the string result is non-empty, the object result forced to be a
 *	duplicate of it first. There will be a string result afterwards.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendResult(
    Tcl_Interp *interp, ...)
{
    va_list argList;

    va_start(argList, interp);
    Tcl_AppendResultVA(interp, argList);
    va_end(argList);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendElement --
 *
 *	Convert a string to a valid Tcl list element and append it to the
 *	result (which is ostensibly a list).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The result in the interpreter given by the first argument is extended
 *	with a list element converted from string. A separator space is added
 *	before the converted list element unless the current result is empty,
 *	contains the single character "{", or ends in " {".
 *
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendElement(
    Tcl_Interp *interp,		/* Interpreter whose result is to be
				 * extended. */
    const char *element)	/* String to convert to list element and add
				 * to result. */
{
    Interp *iPtr = (Interp *) interp;
    char *dst;
    int size;
    int flags;

    /*
     * If the string result is empty, move the object result to the string
     * result, then reset the object result.
     */

    (void) Tcl_GetStringResult(interp);

    /*
     * See how much space is needed, and grow the append buffer if needed to
     * accommodate the list element.
     */

    size = Tcl_ScanElement(element, &flags) + 1;
    if ((iPtr->result != iPtr->appendResult)
	    || (iPtr->appendResult[iPtr->appendUsed] != 0)
	    || ((size + iPtr->appendUsed) >= iPtr->appendAvl)) {
	SetupAppendBuffer(iPtr, size+iPtr->appendUsed);
    }

    /*
     * Convert the string into a list element and copy it to the buffer that's
     * forming, with a space separator if needed.
     */

    dst = iPtr->appendResult + iPtr->appendUsed;
    if (TclNeedSpace(iPtr->appendResult, dst)) {
	iPtr->appendUsed++;
	*dst = ' ';
	dst++;

	/*
	 * If we need a space to separate this element from preceding stuff,
	 * then this element will not lead a list, and need not have it's
	 * leading '#' quoted.
	 */

	flags |= TCL_DONT_QUOTE_HASH;
    }
    iPtr->appendUsed += Tcl_ConvertElement(element, dst, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * SetupAppendBuffer --
 *
 *	This function makes sure that there is an append buffer properly
 *	initialized, if necessary, from the interpreter's result, and that it
 *	has at least enough room to accommodate newSpace new bytes of
 *	information.
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
SetupAppendBuffer(
    Interp *iPtr,		/* Interpreter whose result is being set up. */
    int newSpace)		/* Make sure that at least this many bytes of
				 * new information may be added. */
{
    int totalSpace;

    /*
     * Make the append buffer larger, if that's necessary, then copy the
     * result into the append buffer and make the append buffer the official
     * Tcl result.
     */

    if (iPtr->result != iPtr->appendResult) {
	/*
	 * If an oversized buffer was used recently, then free it up so we go
	 * back to a smaller buffer. This avoids tying up memory forever after
	 * a large operation.
	 */

	if (iPtr->appendAvl > 500) {
	    ckfree(iPtr->appendResult);
	    iPtr->appendResult = NULL;
	    iPtr->appendAvl = 0;
	}
	iPtr->appendUsed = strlen(iPtr->result);
    } else if (iPtr->result[iPtr->appendUsed] != 0) {
	/*
	 * Most likely someone has modified a result created by
	 * Tcl_AppendResult et al. so that it has a different size. Just
	 * recompute the size.
	 */

	iPtr->appendUsed = strlen(iPtr->result);
    }

    totalSpace = newSpace + iPtr->appendUsed;
    if (totalSpace >= iPtr->appendAvl) {
	char *new;

	if (totalSpace < 100) {
	    totalSpace = 200;
	} else {
	    totalSpace *= 2;
	}
	new = ckalloc(totalSpace);
	strcpy(new, iPtr->result);
	if (iPtr->appendResult != NULL) {
	    ckfree(iPtr->appendResult);
	}
	iPtr->appendResult = new;
	iPtr->appendAvl = totalSpace;
    } else if (iPtr->result != iPtr->appendResult) {
	strcpy(iPtr->appendResult, iPtr->result);
    }

    Tcl_FreeResult((Tcl_Interp *) iPtr);
    iPtr->result = iPtr->appendResult;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FreeResult --
 *
 *	This function frees up the memory associated with an interpreter's
 *	string result. It also resets the interpreter's result object.
 *	Tcl_FreeResult is most commonly used when a function is about to
 *	replace one result value with another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the memory associated with interp's string result and sets
 *	interp->freeProc to zero, but does not change interp->result or clear
 *	error state. Resets interp's result object to an unshared empty
 *	object.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FreeResult(
    register Tcl_Interp *interp)/* Interpreter for which to free result. */
{
    register Interp *iPtr = (Interp *) interp;

    if (iPtr->freeProc != NULL) {
	if (iPtr->freeProc == TCL_DYNAMIC) {
	    ckfree(iPtr->result);
	} else {
	    iPtr->freeProc(iPtr->result);
	}
	iPtr->freeProc = 0;
    }

    ResetObjResult(iPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ResetResult --
 *
 *	This function resets both the interpreter's string and object results.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	It resets the result object to an unshared empty object. It then
 *	restores the interpreter's string result area to its default
 *	initialized state, freeing up any memory that may have been allocated.
 *	It also clears any error information for the interpreter.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ResetResult(
    register Tcl_Interp *interp)/* Interpreter for which to clear result. */
{
    register Interp *iPtr = (Interp *) interp;

    ResetObjResult(iPtr);
    if (iPtr->freeProc != NULL) {
	if (iPtr->freeProc == TCL_DYNAMIC) {
	    ckfree(iPtr->result);
	} else {
	    iPtr->freeProc(iPtr->result);
	}
	iPtr->freeProc = 0;
    }
    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
    if (iPtr->errorCode) {
	/* Legacy support */
	if (iPtr->flags & ERR_LEGACY_COPY) {
	    Tcl_ObjSetVar2(interp, iPtr->ecVar, NULL,
		    iPtr->errorCode, TCL_GLOBAL_ONLY);
	}
	Tcl_DecrRefCount(iPtr->errorCode);
	iPtr->errorCode = NULL;
    }
    if (iPtr->errorInfo) {
	/* Legacy support */
	if (iPtr->flags & ERR_LEGACY_COPY) {
	    Tcl_ObjSetVar2(interp, iPtr->eiVar, NULL,
		    iPtr->errorInfo, TCL_GLOBAL_ONLY);
	}
	Tcl_DecrRefCount(iPtr->errorInfo);
	iPtr->errorInfo = NULL;
    }
    iPtr->resetErrorStack = 1;
    iPtr->returnLevel = 1;
    iPtr->returnCode = TCL_OK;
    if (iPtr->returnOpts) {
	Tcl_DecrRefCount(iPtr->returnOpts);
	iPtr->returnOpts = NULL;
    }
    iPtr->flags &= ~(ERR_ALREADY_LOGGED | ERR_LEGACY_COPY);
}

/*
 *----------------------------------------------------------------------
 *
 * ResetObjResult --
 *
 *	Function used to reset an interpreter's Tcl result object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the interpreter's result object to an unshared empty string
 *	object with ref count one. It does not clear any error information in
 *	the interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
ResetObjResult(
    register Interp *iPtr)	/* Points to the interpreter whose result
				 * object should be reset. */
{
    register Tcl_Obj *objResultPtr = iPtr->objResultPtr;

    if (Tcl_IsShared(objResultPtr)) {
	TclDecrRefCount(objResultPtr);
	TclNewObj(objResultPtr);
	Tcl_IncrRefCount(objResultPtr);
	iPtr->objResultPtr = objResultPtr;
    } else {
	if (objResultPtr->bytes != tclEmptyStringRep) {
	    if (objResultPtr->bytes) {
		ckfree(objResultPtr->bytes);
	    }
	    objResultPtr->bytes = tclEmptyStringRep;
	    objResultPtr->length = 0;
	}
	TclFreeIntRep(objResultPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetErrorCodeVA --
 *
 *	This function is called to record machine-readable information about
 *	an error that is about to be returned.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The errorCode field of the interp is modified to hold all of the
 *	arguments to this function, in a list form with each argument becoming
 *	one element of the list.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetErrorCodeVA(
    Tcl_Interp *interp,		/* Interpreter in which to set errorCode */
    va_list argList)		/* Variable argument list. */
{
    Tcl_Obj *errorObj = Tcl_NewObj();

    /*
     * Scan through the arguments one at a time, appending them to the
     * errorCode field as list elements.
     */

    while (1) {
	char *elem = va_arg(argList, char *);

	if (elem == NULL) {
	    break;
	}
	Tcl_ListObjAppendElement(NULL, errorObj, Tcl_NewStringObj(elem, -1));
    }
    Tcl_SetObjErrorCode(interp, errorObj);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetErrorCode --
 *
 *	This function is called to record machine-readable information about
 *	an error that is about to be returned.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The errorCode field of the interp is modified to hold all of the
 *	arguments to this function, in a list form with each argument becoming
 *	one element of the list.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetErrorCode(
    Tcl_Interp *interp, ...)
{
    va_list argList;

    /*
     * Scan through the arguments one at a time, appending them to the
     * errorCode field as list elements.
     */

    va_start(argList, interp);
    Tcl_SetErrorCodeVA(interp, argList);
    va_end(argList);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetObjErrorCode --
 *
 *	This function is called to record machine-readable information about
 *	an error that is about to be returned. The caller should build a list
 *	object up and pass it to this routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The errorCode field of the interp is set to the new value.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetObjErrorCode(
    Tcl_Interp *interp,
    Tcl_Obj *errorObjPtr)
{
    Interp *iPtr = (Interp *) interp;

    if (iPtr->errorCode) {
	Tcl_DecrRefCount(iPtr->errorCode);
    }
    iPtr->errorCode = errorObjPtr;
    Tcl_IncrRefCount(iPtr->errorCode);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetErrorLine --
 *
 *      Returns the line number associated with the current error.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_GetErrorLine
int
Tcl_GetErrorLine(
    Tcl_Interp *interp)
{
    return ((Interp *) interp)->errorLine;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetErrorLine --
 *
 *      Sets the line number associated with the current error.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_SetErrorLine
void
Tcl_SetErrorLine(
    Tcl_Interp *interp,
    int value)
{
    ((Interp *) interp)->errorLine = value;
}

/*
 *----------------------------------------------------------------------
 *
 * GetKeys --
 *
 *	Returns a Tcl_Obj * array of the standard keys used in the return
 *	options dictionary.
 *
 *	Broadly sharing one copy of these key values helps with both memory
 *	efficiency and dictionary lookup times.
 *
 * Results:
 *	A Tcl_Obj * array.
 *
 * Side effects:
 *	First time called in a thread, creates the keys (allocating memory)
 *	and arranges for their cleanup at thread exit.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj **
GetKeys(void)
{
    static Tcl_ThreadDataKey returnKeysKey;
    Tcl_Obj **keys = Tcl_GetThreadData(&returnKeysKey,
	    (int) (KEY_LAST * sizeof(Tcl_Obj *)));

    if (keys[0] == NULL) {
	/*
	 * First call in this thread, create the keys...
	 */

	int i;

	TclNewLiteralStringObj(keys[KEY_CODE],	    "-code");
	TclNewLiteralStringObj(keys[KEY_ERRORCODE], "-errorcode");
	TclNewLiteralStringObj(keys[KEY_ERRORINFO], "-errorinfo");
	TclNewLiteralStringObj(keys[KEY_ERRORLINE], "-errorline");
	TclNewLiteralStringObj(keys[KEY_ERRORSTACK],"-errorstack");
	TclNewLiteralStringObj(keys[KEY_LEVEL],	    "-level");
	TclNewLiteralStringObj(keys[KEY_OPTIONS],   "-options");

	for (i = KEY_CODE; i < KEY_LAST; i++) {
	    Tcl_IncrRefCount(keys[i]);
	}

	/*
	 * ... and arrange for their clenaup.
	 */

	Tcl_CreateThreadExitHandler(ReleaseKeys, keys);
    }
    return keys;
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseKeys --
 *
 *	Called as a thread exit handler to cleanup return options dictionary
 *	keys.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 *----------------------------------------------------------------------
 */

static void
ReleaseKeys(
    ClientData clientData)
{
    Tcl_Obj **keys = clientData;
    int i;

    for (i = KEY_CODE; i < KEY_LAST; i++) {
	Tcl_DecrRefCount(keys[i]);
	keys[i] = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclProcessReturn --
 *
 *	Does the work of the [return] command based on the code, level, and
 *	returnOpts arguments. Note that the code argument must agree with the
 *	-code entry in returnOpts and the level argument must agree with the
 *	-level entry in returnOpts, as is the case for values returned from
 *	TclMergeReturnOptions.
 *
 * Results:
 *	Returns the return code the [return] command should return.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclProcessReturn(
    Tcl_Interp *interp,
    int code,
    int level,
    Tcl_Obj *returnOpts)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *valuePtr;
    Tcl_Obj **keys = GetKeys();

    /*
     * Store the merged return options.
     */

    if (iPtr->returnOpts != returnOpts) {
	if (iPtr->returnOpts) {
	    Tcl_DecrRefCount(iPtr->returnOpts);
	}
	iPtr->returnOpts = returnOpts;
	Tcl_IncrRefCount(iPtr->returnOpts);
    }

    if (code == TCL_ERROR) {
	if (iPtr->errorInfo) {
	    Tcl_DecrRefCount(iPtr->errorInfo);
	    iPtr->errorInfo = NULL;
	}
	Tcl_DictObjGet(NULL, iPtr->returnOpts, keys[KEY_ERRORINFO],
                &valuePtr);
	if (valuePtr != NULL) {
	    int infoLen;

	    (void) TclGetStringFromObj(valuePtr, &infoLen);
	    if (infoLen) {
		iPtr->errorInfo = valuePtr;
		Tcl_IncrRefCount(iPtr->errorInfo);
		iPtr->flags |= ERR_ALREADY_LOGGED;
	    }
	}
	Tcl_DictObjGet(NULL, iPtr->returnOpts, keys[KEY_ERRORSTACK],
                &valuePtr);
	if (valuePtr != NULL) {
            int len, valueObjc;
            Tcl_Obj **valueObjv;

            if (Tcl_IsShared(iPtr->errorStack)) {
                Tcl_Obj *newObj;

                newObj = Tcl_DuplicateObj(iPtr->errorStack);
                Tcl_DecrRefCount(iPtr->errorStack);
                Tcl_IncrRefCount(newObj);
                iPtr->errorStack = newObj;
            }

            /*
             * List extraction done after duplication to avoid moving the rug
             * if someone does [return -errorstack [info errorstack]]
             */

            if (Tcl_ListObjGetElements(interp, valuePtr, &valueObjc,
                    &valueObjv) == TCL_ERROR) {
                return TCL_ERROR;
            }
            iPtr->resetErrorStack = 0;
            Tcl_ListObjLength(interp, iPtr->errorStack, &len);

            /*
             * Reset while keeping the list intrep as much as possible.
             */

            Tcl_ListObjReplace(interp, iPtr->errorStack, 0, len, valueObjc,
                    valueObjv);
 	}
	Tcl_DictObjGet(NULL, iPtr->returnOpts, keys[KEY_ERRORCODE],
                &valuePtr);
	if (valuePtr != NULL) {
	    Tcl_SetObjErrorCode(interp, valuePtr);
	} else {
	    Tcl_SetErrorCode(interp, "NONE", NULL);
	}

	Tcl_DictObjGet(NULL, iPtr->returnOpts, keys[KEY_ERRORLINE],
                &valuePtr);
	if (valuePtr != NULL) {
	    TclGetIntFromObj(NULL, valuePtr, &iPtr->errorLine);
	}
    }
    if (level != 0) {
	iPtr->returnLevel = level;
	iPtr->returnCode = code;
	return TCL_RETURN;
    }
    if (code == TCL_ERROR) {
	iPtr->flags |= ERR_LEGACY_COPY;
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMergeReturnOptions --
 *
 *	Parses, checks, and stores the options to the [return] command.
 *
 * Results:
 *	Returns TCL_ERROR if any of the option values are invalid. Otherwise,
 *	returns TCL_OK, and writes the returnOpts, code, and level values to
 *	the pointers provided.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclMergeReturnOptions(
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects. */
    Tcl_Obj **optionsPtrPtr,	/* If not NULL, points to space for a (Tcl_Obj
				 * *) where the pointer to the merged return
				 * options dictionary should be written. */
    int *codePtr,		/* If not NULL, points to space where the
				 * -code value should be written. */
    int *levelPtr)		/* If not NULL, points to space where the
				 * -level value should be written. */
{
    int code = TCL_OK;
    int level = 1;
    Tcl_Obj *valuePtr;
    Tcl_Obj *returnOpts = Tcl_NewObj();
    Tcl_Obj **keys = GetKeys();

    for (;  objc > 1;  objv += 2, objc -= 2) {
	int optLen;
	const char *opt = TclGetStringFromObj(objv[0], &optLen);
	int compareLen;
	const char *compare =
		TclGetStringFromObj(keys[KEY_OPTIONS], &compareLen);

	if ((optLen == compareLen) && (memcmp(opt, compare, optLen) == 0)) {
	    Tcl_DictSearch search;
	    int done = 0;
	    Tcl_Obj *keyPtr;
	    Tcl_Obj *dict = objv[1];

	nestedOptions:
	    if (TCL_ERROR == Tcl_DictObjFirst(NULL, dict, &search,
		    &keyPtr, &valuePtr, &done)) {
		/*
		 * Value is not a legal dictionary.
		 */

		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                        "bad %s value: expected dictionary but got \"%s\"",
                        compare, TclGetString(objv[1])));
		Tcl_SetErrorCode(interp, "TCL", "RESULT", "ILLEGAL_OPTIONS",
			NULL);
		goto error;
	    }

	    while (!done) {
		Tcl_DictObjPut(NULL, returnOpts, keyPtr, valuePtr);
		Tcl_DictObjNext(&search, &keyPtr, &valuePtr, &done);
	    }

	    Tcl_DictObjGet(NULL, returnOpts, keys[KEY_OPTIONS], &valuePtr);
	    if (valuePtr != NULL) {
		dict = valuePtr;
		Tcl_DictObjRemove(NULL, returnOpts, keys[KEY_OPTIONS]);
		goto nestedOptions;
	    }

	} else {
	    Tcl_DictObjPut(NULL, returnOpts, objv[0], objv[1]);
	}
    }

    /*
     * Check for bogus -code value.
     */

    Tcl_DictObjGet(NULL, returnOpts, keys[KEY_CODE], &valuePtr);
    if (valuePtr != NULL) {
	if (TclGetCompletionCodeFromObj(interp, valuePtr,
                &code) == TCL_ERROR) {
	    goto error;
	}
	Tcl_DictObjRemove(NULL, returnOpts, keys[KEY_CODE]);
    }

    /*
     * Check for bogus -level value.
     */

    Tcl_DictObjGet(NULL, returnOpts, keys[KEY_LEVEL], &valuePtr);
    if (valuePtr != NULL) {
	if ((TCL_ERROR == TclGetIntFromObj(NULL, valuePtr, &level))
		|| (level < 0)) {
	    /*
	     * Value is not a legal level.
	     */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "bad -level value: expected non-negative integer but got"
                    " \"%s\"", TclGetString(valuePtr)));
	    Tcl_SetErrorCode(interp, "TCL", "RESULT", "ILLEGAL_LEVEL", NULL);
	    goto error;
	}
	Tcl_DictObjRemove(NULL, returnOpts, keys[KEY_LEVEL]);
    }

    /*
     * Check for bogus -errorcode value.
     */

    Tcl_DictObjGet(NULL, returnOpts, keys[KEY_ERRORCODE], &valuePtr);
    if (valuePtr != NULL) {
	int length;

	if (TCL_ERROR == Tcl_ListObjLength(NULL, valuePtr, &length )) {
	    /*
	     * Value is not a list, which is illegal for -errorcode.
	     */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "bad -errorcode value: expected a list but got \"%s\"",
                    TclGetString(valuePtr)));
	    Tcl_SetErrorCode(interp, "TCL", "RESULT", "ILLEGAL_ERRORCODE",
		    NULL);
	    goto error;
	}
    }

    /*
     * Check for bogus -errorstack value.
     */

    Tcl_DictObjGet(NULL, returnOpts, keys[KEY_ERRORSTACK], &valuePtr);
    if (valuePtr != NULL) {
	int length;

	if (TCL_ERROR == Tcl_ListObjLength(NULL, valuePtr, &length )) {
	    /*
	     * Value is not a list, which is illegal for -errorstack.
	     */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "bad -errorstack value: expected a list but got \"%s\"",
                    TclGetString(valuePtr)));
	    Tcl_SetErrorCode(interp, "TCL", "RESULT", "NONLIST_ERRORSTACK",
                    NULL);
	    goto error;
	}
        if (length % 2) {
            /*
             * Errorstack must always be an even-sized list
             */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "forbidden odd-sized list for -errorstack: \"%s\"",
		    TclGetString(valuePtr)));
	    Tcl_SetErrorCode(interp, "TCL", "RESULT",
                    "ODDSIZEDLIST_ERRORSTACK", NULL);
	    goto error;
        }
    }

    /*
     * Convert [return -code return -level X] to [return -code ok -level X+1]
     */

    if (code == TCL_RETURN) {
	level++;
	code = TCL_OK;
    }

    if (codePtr != NULL) {
	*codePtr = code;
    }
    if (levelPtr != NULL) {
	*levelPtr = level;
    }

    if (optionsPtrPtr == NULL) {
	/*
	 * Not passing back the options (?!), so clean them up.
	 */

	Tcl_DecrRefCount(returnOpts);
    } else {
	*optionsPtrPtr = returnOpts;
    }
    return TCL_OK;

  error:
    Tcl_DecrRefCount(returnOpts);
    return TCL_ERROR;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetReturnOptions --
 *
 *	Packs up the interp state into a dictionary of return options.
 *
 * Results:
 *	A dictionary of return options.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_GetReturnOptions(
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *options;
    Tcl_Obj **keys = GetKeys();

    if (iPtr->returnOpts) {
	options = Tcl_DuplicateObj(iPtr->returnOpts);
    } else {
	options = Tcl_NewObj();
    }

    if (result == TCL_RETURN) {
	Tcl_DictObjPut(NULL, options, keys[KEY_CODE],
		Tcl_NewIntObj(iPtr->returnCode));
	Tcl_DictObjPut(NULL, options, keys[KEY_LEVEL],
		Tcl_NewIntObj(iPtr->returnLevel));
    } else {
	Tcl_DictObjPut(NULL, options, keys[KEY_CODE],
		Tcl_NewIntObj(result));
	Tcl_DictObjPut(NULL, options, keys[KEY_LEVEL],
		Tcl_NewIntObj(0));
    }

    if (result == TCL_ERROR) {
	Tcl_AddErrorInfo(interp, "");
        Tcl_DictObjPut(NULL, options, keys[KEY_ERRORSTACK], iPtr->errorStack);
    }
    if (iPtr->errorCode) {
	Tcl_DictObjPut(NULL, options, keys[KEY_ERRORCODE], iPtr->errorCode);
    }
    if (iPtr->errorInfo) {
	Tcl_DictObjPut(NULL, options, keys[KEY_ERRORINFO], iPtr->errorInfo);
	Tcl_DictObjPut(NULL, options, keys[KEY_ERRORLINE],
		Tcl_NewIntObj(iPtr->errorLine));
    }
    return options;
}

/*
 *-------------------------------------------------------------------------
 *
 * TclNoErrorStack --
 *
 *	Removes the -errorstack entry from an options dict to avoid reference
 *	cycles.
 *
 * Results:
 *	The (unshared) argument options dict, modified in -place.
 *
 *-------------------------------------------------------------------------
 */

Tcl_Obj *
TclNoErrorStack(
    Tcl_Interp *interp,
    Tcl_Obj *options)
{
    Tcl_Obj **keys = GetKeys();

    Tcl_DictObjRemove(interp, options, keys[KEY_ERRORSTACK]);
    return options;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_SetReturnOptions --
 *
 *	Accepts an interp and a dictionary of return options, and sets the
 *	return options of the interp to match the dictionary.
 *
 * Results:
 *	A standard status code. Usually TCL_OK, but TCL_ERROR if an invalid
 *	option value was found in the dictionary. If a -level value of 0 is in
 *	the dictionary, then the -code value in the dictionary will be
 *	returned (TCL_OK default).
 *
 * Side effects:
 *	Sets the state of the interp.
 *
 *-------------------------------------------------------------------------
 */

int
Tcl_SetReturnOptions(
    Tcl_Interp *interp,
    Tcl_Obj *options)
{
    int objc, level, code;
    Tcl_Obj **objv, *mergedOpts;

    Tcl_IncrRefCount(options);
    if (TCL_ERROR == TclListObjGetElements(interp, options, &objc, &objv)
	    || (objc % 2)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "expected dict but got \"%s\"", TclGetString(options)));
	Tcl_SetErrorCode(interp, "TCL", "RESULT", "ILLEGAL_OPTIONS", NULL);
	code = TCL_ERROR;
    } else if (TCL_ERROR == TclMergeReturnOptions(interp, objc, objv,
	    &mergedOpts, &code, &level)) {
	code = TCL_ERROR;
    } else {
	code = TclProcessReturn(interp, code, level, mergedOpts);
    }

    Tcl_DecrRefCount(options);
    return code;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_TransferResult --
 *
 *	Copy the result (and error information) from one interp to another.
 *	Used when one interp has caused another interp to evaluate a script
 *	and then wants to transfer the results back to itself.
 *
 *	This routine copies the string reps of the result and error
 *	information. It does not simply increment the refcounts of the result
 *	and error information objects themselves. It is not legal to exchange
 *	objects between interps, because an object may be kept alive by one
 *	interp, but have an internal rep that is only valid while some other
 *	interp is alive.
 *
 * Results:
 *	The target interp's result is set to a copy of the source interp's
 *	result. The source's errorInfo field may be transferred to the
 *	target's errorInfo field, and the source's errorCode field may be
 *	transferred to the target's errorCode field.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

void
Tcl_TransferResult(
    Tcl_Interp *sourceInterp,	/* Interp whose result and error information
				 * should be moved to the target interp.
				 * After moving result, this interp's result
				 * is reset. */
    int result,			/* TCL_OK if just the result should be copied,
				 * TCL_ERROR if both the result and error
				 * information should be copied. */
    Tcl_Interp *targetInterp)	/* Interp where result and error information
				 * should be stored. If source and target are
				 * the same, nothing is done. */
{
    Interp *tiPtr = (Interp *) targetInterp;
    Interp *siPtr = (Interp *) sourceInterp;

    if (sourceInterp == targetInterp) {
	return;
    }

    if (result == TCL_OK && siPtr->returnOpts == NULL) {
	/*
	 * Special optimization for the common case of normal command return
	 * code and no explicit return options.
	 */

	if (tiPtr->returnOpts) {
	    Tcl_DecrRefCount(tiPtr->returnOpts);
	    tiPtr->returnOpts = NULL;
	}
    } else {
	Tcl_SetReturnOptions(targetInterp,
		Tcl_GetReturnOptions(sourceInterp, result));
	tiPtr->flags &= ~(ERR_ALREADY_LOGGED);
    }
    Tcl_SetObjResult(targetInterp, Tcl_GetObjResult(sourceInterp));
    Tcl_ResetResult(sourceInterp);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 */
