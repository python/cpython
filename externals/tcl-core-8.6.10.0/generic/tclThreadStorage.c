/*
 * tclThreadStorage.c --
 *
 *	This file implements platform independent thread storage operations to
 *	work around system limits on the number of thread-specific variables.
 *
 * Copyright (c) 2003-2004 by Joe Mistachkin
 * Copyright (c) 2008 by George Peter Staplin
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

#ifdef TCL_THREADS
#include <signal.h>

/*
 * IMPLEMENTATION NOTES:
 *
 * The primary idea is that we create one platform-specific TSD slot, and use
 * it for storing a table pointer. Each Tcl_ThreadDataKey has an offset into
 * the table of TSD values. We don't use more than 1 platform-specific TSD
 * slot, because there is a hard limit on the number of TSD slots. Valid key
 * offsets are greater than 0; 0 is for the initialized Tcl_ThreadDataKey.
 */

/*
 * The master collection of information about TSDs. This is shared across the
 * whole process, and includes the mutex used to protect it.
 */

static struct TSDMaster {
    void *key;			/* Key into the system TSD structure. The
				 * collection of Tcl TSD values for a
				 * particular thread will hang off the
				 * back-end of this. */
    sig_atomic_t counter;	/* The number of different Tcl TSDs used
				 * across *all* threads. This is a strictly
				 * increasing value. */
    Tcl_Mutex mutex;		/* Protection for the rest of this structure,
				 * which holds per-process data. */
} tsdMaster = { NULL, 0, NULL };

/*
 * The type of the data held per thread in a system TSD.
 */

typedef struct TSDTable {
    ClientData *tablePtr;	/* The table of Tcl TSDs. */
    sig_atomic_t allocated;	/* The size of the table in the current
				 * thread. */
} TSDTable;

/*
 * The actual type of Tcl_ThreadDataKey.
 */

typedef union TSDUnion {
    volatile sig_atomic_t offset;
				/* The type is really an offset into the
				 * thread-local table of TSDs, which is this
				 * field. */
    void *ptr;			/* For alignment purposes only. Not actually
				 * accessed through this. */
} TSDUnion;

/*
 * Forward declarations of functions in this file.
 */

static TSDTable *	TSDTableCreate(void);
static void		TSDTableDelete(TSDTable *tsdTablePtr);
static void		TSDTableGrow(TSDTable *tsdTablePtr,
			    sig_atomic_t atLeast);

/*
 * Allocator and deallocator for a TSDTable structure.
 */

static TSDTable *
TSDTableCreate(void)
{
    TSDTable *tsdTablePtr;
    sig_atomic_t i;

    tsdTablePtr = TclpSysAlloc(sizeof(TSDTable), 0);
    if (tsdTablePtr == NULL) {
	Tcl_Panic("unable to allocate TSDTable");
    }

    tsdTablePtr->allocated = 8;
    tsdTablePtr->tablePtr =
	    TclpSysAlloc(sizeof(void *) * tsdTablePtr->allocated, 0);
    if (tsdTablePtr->tablePtr == NULL) {
	Tcl_Panic("unable to allocate TSDTable");
    }

    for (i = 0; i < tsdTablePtr->allocated; ++i) {
	tsdTablePtr->tablePtr[i] = NULL;
    }

    return tsdTablePtr;
}

static void
TSDTableDelete(
    TSDTable *tsdTablePtr)
{
    sig_atomic_t i;

    for (i=0 ; i<tsdTablePtr->allocated ; i++) {
	if (tsdTablePtr->tablePtr[i] != NULL) {
	    /*
	     * These values were allocated in Tcl_GetThreadData in tclThread.c
	     * and must now be deallocated or they will leak.
	     */

	    ckfree(tsdTablePtr->tablePtr[i]);
	}
    }

    TclpSysFree(tsdTablePtr->tablePtr);
    TclpSysFree(tsdTablePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TSDTableGrow --
 *
 *	This procedure makes the passed TSDTable grow to fit the atLeast
 *	value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The table is enlarged.
 *
 *----------------------------------------------------------------------
 */

static void
TSDTableGrow(
    TSDTable *tsdTablePtr,
    sig_atomic_t atLeast)
{
    sig_atomic_t newAllocated = tsdTablePtr->allocated * 2;
    ClientData *newTablePtr;
    sig_atomic_t i;

    if (newAllocated <= atLeast) {
	newAllocated = atLeast + 10;
    }

    newTablePtr = TclpSysRealloc(tsdTablePtr->tablePtr,
	    sizeof(ClientData) * newAllocated);
    if (newTablePtr == NULL) {
	Tcl_Panic("unable to reallocate TSDTable");
    }

    for (i = tsdTablePtr->allocated; i < newAllocated; ++i) {
	newTablePtr[i] = NULL;
    }

    tsdTablePtr->allocated = newAllocated;
    tsdTablePtr->tablePtr = newTablePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclThreadStorageKeyGet --
 *
 *    This procedure gets the value associated with the passed key.
 *
 * Results:
 *	A pointer value associated with the Tcl_ThreadDataKey or NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void *
TclThreadStorageKeyGet(
    Tcl_ThreadDataKey *dataKeyPtr)
{
    TSDTable *tsdTablePtr = TclpThreadGetMasterTSD(tsdMaster.key);
    ClientData resultPtr = NULL;
    TSDUnion *keyPtr = (TSDUnion *) dataKeyPtr;
    sig_atomic_t offset = keyPtr->offset;

    if ((tsdTablePtr != NULL) && (offset > 0)
	    && (offset < tsdTablePtr->allocated)) {
	resultPtr = tsdTablePtr->tablePtr[offset];
    }
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclThreadStorageKeySet --
 *
 *	This procedure set an association of value with the key passed. The
 *	associated value may be retrieved with TclThreadDataKeyGet().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The thread-specific table may be created or reallocated.
 *
 *----------------------------------------------------------------------
 */

void
TclThreadStorageKeySet(
    Tcl_ThreadDataKey *dataKeyPtr,
    void *value)
{
    TSDTable *tsdTablePtr = TclpThreadGetMasterTSD(tsdMaster.key);
    TSDUnion *keyPtr = (TSDUnion *) dataKeyPtr;

    if (tsdTablePtr == NULL) {
	tsdTablePtr = TSDTableCreate();
	TclpThreadSetMasterTSD(tsdMaster.key, tsdTablePtr);
    }

    /*
     * Get the lock while we check if this TSD is new or not. Note that this
     * is the only place where Tcl_ThreadDataKey values are set. We use a
     * double-checked lock to try to avoid having to grab this lock a lot,
     * since it is on quite a few critical paths and will only get set once in
     * each location.
     */

    if (keyPtr->offset == 0) {
	Tcl_MutexLock(&tsdMaster.mutex);
	if (keyPtr->offset == 0) {
	    /*
	     * The Tcl_ThreadDataKey hasn't been used yet. Make a new one.
	     */

	    keyPtr->offset = ++tsdMaster.counter;
	}
	Tcl_MutexUnlock(&tsdMaster.mutex);
    }

    /*
     * Check if this is the first time this Tcl_ThreadDataKey has been used
     * with the current thread. Note that we don't need to hold a lock when
     * doing this, as we are *definitely* the only point accessing this
     * tsdTablePtr right now; it's thread-local.
     */

    if (keyPtr->offset >= tsdTablePtr->allocated) {
	TSDTableGrow(tsdTablePtr, keyPtr->offset);
    }

    /*
     * Set the value in the Tcl thread-local variable.
     */

    tsdTablePtr->tablePtr[keyPtr->offset] = value;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeThreadDataThread --
 *
 *	This procedure finalizes the data for a single thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TSDTable is deleted/freed.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeThreadDataThread(void)
{
    TSDTable *tsdTablePtr = TclpThreadGetMasterTSD(tsdMaster.key);

    if (tsdTablePtr != NULL) {
	TSDTableDelete(tsdTablePtr);
	TclpThreadSetMasterTSD(tsdMaster.key, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitializeThreadStorage --
 *
 *	This procedure initializes the TSD subsystem with per-platform code.
 *	This should be called before any Tcl threads are created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates a system TSD.
 *
 *----------------------------------------------------------------------
 */

void
TclInitThreadStorage(void)
{
    tsdMaster.key = TclpThreadCreateKey();
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeThreadStorage --
 *
 *	This procedure cleans up the thread storage data key for all threads.
 *	IMPORTANT: All Tcl threads must be finalized before calling this!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases the thread data key.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeThreadStorage(void)
{
    TclpThreadDeleteKey(tsdMaster.key);
    tsdMaster.key = NULL;
}

#else /* !TCL_THREADS */
/*
 * Stub functions for non-threaded builds
 */

void
TclInitThreadStorage(void)
{
}

void
TclFinalizeThreadDataThread(void)
{
}

void
TclFinalizeThreadStorage(void)
{
}
#endif /* TCL_THREADS */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
