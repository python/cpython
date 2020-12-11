/*
 * tclPreserve.c --
 *
 *	This file contains a collection of functions that are used to make
 *	sure that widget records and other data structures aren't reallocated
 *	when there are nested functions that depend on their existence.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * The following data structure is used to keep track of all the Tcl_Preserve
 * calls that are still in effect. It grows as needed to accommodate any
 * number of calls in effect.
 */

typedef struct {
    ClientData clientData;	/* Address of preserved block. */
    int refCount;		/* Number of Tcl_Preserve calls in effect for
				 * block. */
    int mustFree;		/* Non-zero means Tcl_EventuallyFree was
				 * called while a Tcl_Preserve call was in
				 * effect, so the structure must be freed when
				 * refCount becomes zero. */
    Tcl_FreeProc *freeProc;	/* Function to call to free. */
} Reference;

/*
 * Global data structures used to hold the list of preserved data references.
 * These variables are protected by "preserveMutex".
 */

static Reference *refArray = NULL;	/* First in array of references. */
static int spaceAvl = 0;	/* Total number of structures available at
				 * *firstRefPtr. */
static int inUse = 0;		/* Count of structures currently in use in
				 * refArray. */
TCL_DECLARE_MUTEX(preserveMutex)/* To protect the above statics */

#define INITIAL_SIZE	2	/* Initial number of reference slots to make */

/*
 * The following data structure is used to keep track of whether an arbitrary
 * block of memory has been deleted. This is used by the TclHandle code to
 * avoid the more time-expensive algorithm of Tcl_Preserve(). This mechanism
 * is mainly used when we have lots of references to a few big, expensive
 * objects that we don't want to live any longer than necessary.
 */

typedef struct HandleStruct {
    void *ptr;			/* Pointer to the memory block being tracked.
				 * This field will become NULL when the memory
				 * block is deleted. This field must be the
				 * first in the structure. */
#ifdef TCL_MEM_DEBUG
    void *ptr2;			/* Backup copy of the above pointer used to
				 * ensure that the contents of the handle are
				 * not changed by anyone else. */
#endif
    int refCount;		/* Number of TclHandlePreserve() calls in
				 * effect on this handle. */
} HandleStruct;

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizePreserve --
 *
 *	Called during exit processing to clean up the reference array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the storage of the reference array.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
void
TclFinalizePreserve(void)
{
    Tcl_MutexLock(&preserveMutex);
    if (spaceAvl != 0) {
	ckfree(refArray);
	refArray = NULL;
	inUse = 0;
	spaceAvl = 0;
    }
    Tcl_MutexUnlock(&preserveMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Preserve --
 *
 *	This function is used by a function to declare its interest in a
 *	particular block of memory, so that the block will not be reallocated
 *	until a matching call to Tcl_Release has been made.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is retained so that the block of memory will not be freed
 *	until at least the matching call to Tcl_Release.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Preserve(
    ClientData clientData)	/* Pointer to malloc'ed block of memory. */
{
    Reference *refPtr;
    int i;

    /*
     * See if there is already a reference for this pointer. If so, just
     * increment its reference count.
     */

    Tcl_MutexLock(&preserveMutex);
    for (i=0, refPtr=refArray ; i<inUse ; i++, refPtr++) {
	if (refPtr->clientData == clientData) {
	    refPtr->refCount++;
	    Tcl_MutexUnlock(&preserveMutex);
	    return;
	}
    }

    /*
     * Make a reference array if it doesn't already exist, or make it bigger
     * if it is full.
     */

    if (inUse == spaceAvl) {
	spaceAvl = spaceAvl ? 2*spaceAvl : INITIAL_SIZE;
	refArray = ckrealloc(refArray, spaceAvl * sizeof(Reference));
    }

    /*
     * Make a new entry for the new reference.
     */

    refPtr = &refArray[inUse];
    refPtr->clientData = clientData;
    refPtr->refCount = 1;
    refPtr->mustFree = 0;
    refPtr->freeProc = TCL_STATIC;
    inUse += 1;
    Tcl_MutexUnlock(&preserveMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Release --
 *
 *	This function is called to cancel a previous call to Tcl_Preserve,
 *	thereby allowing a block of memory to be freed (if no one else cares
 *	about it).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If Tcl_EventuallyFree has been called for clientData, and if no other
 *	call to Tcl_Preserve is still in effect, the block of memory is freed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Release(
    ClientData clientData)	/* Pointer to malloc'ed block of memory. */
{
    Reference *refPtr;
    int i;

    Tcl_MutexLock(&preserveMutex);
    for (i=0, refPtr=refArray ; i<inUse ; i++, refPtr++) {
	int mustFree;
	Tcl_FreeProc *freeProc;

	if (refPtr->clientData != clientData) {
	    continue;
	}

	if (--refPtr->refCount != 0) {
	    Tcl_MutexUnlock(&preserveMutex);
	    return;
	}

	/*
	 * Must remove information from the slot before calling freeProc to
	 * avoid reentrancy problems if the freeProc calls Tcl_Preserve on the
	 * same clientData. Copy down the last reference in the array to
	 * overwrite the current slot.
	 */

	freeProc = refPtr->freeProc;
	mustFree = refPtr->mustFree;
	inUse--;
	if (i < inUse) {
	    refArray[i] = refArray[inUse];
	}

	/*
	 * Now committed to disposing the data. But first, we've patched up
	 * all the global data structures so we should release the mutex now.
	 * Only then should we dabble around with potentially-slow memory
	 * managers...
	 */

	Tcl_MutexUnlock(&preserveMutex);
	if (mustFree) {
	    if (freeProc == TCL_DYNAMIC) {
		ckfree(clientData);
	    } else {
		freeProc(clientData);
	    }
	}
	return;
    }
    Tcl_MutexUnlock(&preserveMutex);

    /*
     * Reference not found. This is a bug in the caller.
     */

    Tcl_Panic("Tcl_Release couldn't find reference for %p", clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EventuallyFree --
 *
 *	Free up a block of memory, unless a call to Tcl_Preserve is in effect
 *	for that block. In this case, defer the free until all calls to
 *	Tcl_Preserve have been undone by matching calls to Tcl_Release.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Ptr may be released by calling free().
 *
 *----------------------------------------------------------------------
 */

void
Tcl_EventuallyFree(
    ClientData clientData,	/* Pointer to malloc'ed block of memory. */
    Tcl_FreeProc *freeProc)	/* Function to actually do free. */
{
    Reference *refPtr;
    int i;

    /*
     * See if there is a reference for this pointer. If so, set its "mustFree"
     * flag (the flag had better not be set already!).
     */

    Tcl_MutexLock(&preserveMutex);
    for (i = 0, refPtr = refArray; i < inUse; i++, refPtr++) {
	if (refPtr->clientData != clientData) {
	    continue;
	}
	if (refPtr->mustFree) {
	    Tcl_Panic("Tcl_EventuallyFree called twice for %p", clientData);
	}
	refPtr->mustFree = 1;
	refPtr->freeProc = freeProc;
	Tcl_MutexUnlock(&preserveMutex);
	return;
    }
    Tcl_MutexUnlock(&preserveMutex);

    /*
     * No reference for this block.  Free it now.
     */

    if (freeProc == TCL_DYNAMIC) {
	ckfree(clientData);
    } else {
	freeProc(clientData);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TclHandleCreate --
 *
 *	Allocate a handle that contains enough information to determine if an
 *	arbitrary malloc'd block has been deleted. This is used to avoid the
 *	more time-expensive algorithm of Tcl_Preserve().
 *
 * Results:
 *	The return value is a TclHandle that refers to the given malloc'd
 *	block. Doubly dereferencing the returned handle will give back the
 *	pointer to the block, or will give NULL if the block has been deleted.
 *
 * Side effects:
 *	The caller must keep track of this handle (generally by storing it in
 *	a field in the malloc'd block) and call TclHandleFree() on this handle
 *	when the block is deleted. Everything else that wishes to keep track
 *	of whether the malloc'd block has been deleted should use calls to
 *	TclHandlePreserve() and TclHandleRelease() on the associated handle.
 *
 *---------------------------------------------------------------------------
 */

TclHandle
TclHandleCreate(
    void *ptr)			/* Pointer to an arbitrary block of memory to
				 * be tracked for deletion. Must not be
				 * NULL. */
{
    HandleStruct *handlePtr = ckalloc(sizeof(HandleStruct));

    handlePtr->ptr = ptr;
#ifdef TCL_MEM_DEBUG
    handlePtr->ptr2 = ptr;
#endif
    handlePtr->refCount = 0;
    return (TclHandle) handlePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclHandleFree --
 *
 *	Called when the arbitrary malloc'd block associated with the handle is
 *	being deleted. Modifies the handle so that doubly dereferencing it
 *	will give NULL. This informs any user of the handle that the block of
 *	memory formerly referenced by the handle has been freed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If nothing is referring to the handle, the handle will be reclaimed.
 *
 *---------------------------------------------------------------------------
 */

void
TclHandleFree(
    TclHandle handle)		/* Previously created handle associated with a
				 * malloc'd block that is being deleted. The
				 * handle is modified so that doubly
				 * dereferencing it will give NULL. */
{
    HandleStruct *handlePtr;

    handlePtr = (HandleStruct *) handle;
#ifdef TCL_MEM_DEBUG
    if (handlePtr->refCount == 0x61616161) {
	Tcl_Panic("using previously disposed TclHandle %p", handlePtr);
    }
    if (handlePtr->ptr2 != handlePtr->ptr) {
	Tcl_Panic("someone has changed the block referenced by the handle %p\nfrom %p to %p",
		handlePtr, handlePtr->ptr2, handlePtr->ptr);
    }
#endif
    handlePtr->ptr = NULL;
    if (handlePtr->refCount == 0) {
	ckfree(handlePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TclHandlePreserve --
 *
 *	Declare an interest in the arbitrary malloc'd block associated with
 *	the handle.
 *
 * Results:
 *	The return value is the handle argument, with its ref count
 *	incremented.
 *
 * Side effects:
 *	For each call to TclHandlePreserve(), there should be a matching call
 *	to TclHandleRelease() when the caller is no longer interested in the
 *	malloc'd block associated with the handle.
 *
 *---------------------------------------------------------------------------
 */

TclHandle
TclHandlePreserve(
    TclHandle handle)		/* Declare an interest in the block of memory
				 * referenced by this handle. */
{
    HandleStruct *handlePtr;

    handlePtr = (HandleStruct *) handle;
#ifdef TCL_MEM_DEBUG
    if (handlePtr->refCount == 0x61616161) {
	Tcl_Panic("using previously disposed TclHandle %p", handlePtr);
    }
    if ((handlePtr->ptr != NULL) && (handlePtr->ptr != handlePtr->ptr2)) {
	Tcl_Panic("someone has changed the block referenced by the handle %p\nfrom %p to %p",
		handlePtr, handlePtr->ptr2, handlePtr->ptr);
    }
#endif
    handlePtr->refCount++;

    return handle;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclHandleRelease --
 *
 *	This function is called to release an interest in the malloc'd block
 *	associated with the handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The ref count of the handle is decremented. If the malloc'd block has
 *	been freed and if no one is using the handle any more, the handle will
 *	be reclaimed.
 *
 *---------------------------------------------------------------------------
 */

void
TclHandleRelease(
    TclHandle handle)		/* Unregister interest in the block of memory
				 * referenced by this handle. */
{
    HandleStruct *handlePtr;

    handlePtr = (HandleStruct *) handle;
#ifdef TCL_MEM_DEBUG
    if (handlePtr->refCount == 0x61616161) {
	Tcl_Panic("using previously disposed TclHandle %p", handlePtr);
    }
    if ((handlePtr->ptr != NULL) && (handlePtr->ptr != handlePtr->ptr2)) {
	Tcl_Panic("someone has changed the block referenced by the handle %p\nfrom %p to %p",
		handlePtr, handlePtr->ptr2, handlePtr->ptr);
    }
#endif
    if ((--handlePtr->refCount == 0) && (handlePtr->ptr == NULL)) {
	ckfree(handlePtr);
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
