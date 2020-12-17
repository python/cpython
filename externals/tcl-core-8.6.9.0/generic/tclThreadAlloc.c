/*
 * tclThreadAlloc.c --
 *
 *	This is a very fast storage allocator for used with threads (designed
 *	avoid lock contention). The basic strategy is to allocate memory in
 *	fixed size blocks from block caches.
 *
 * The Initial Developer of the Original Code is America Online, Inc.
 * Portions created by AOL are Copyright (C) 1999 America Online, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#if defined(TCL_THREADS) && defined(USE_THREAD_ALLOC)

/*
 * If range checking is enabled, an additional byte will be allocated to store
 * the magic number at the end of the requested memory.
 */

#ifndef RCHECK
#ifdef  NDEBUG
#define RCHECK		0
#else
#define RCHECK		1
#endif
#endif

/*
 * The following define the number of Tcl_Obj's to allocate/move at a time and
 * the high water mark to prune a per-thread cache. On a 32 bit system,
 * sizeof(Tcl_Obj) = 24 so 800 * 24 = ~16k.
 */

#define NOBJALLOC	800

/* Actual definition moved to tclInt.h */
#define NOBJHIGH	ALLOC_NOBJHIGH

/*
 * The following union stores accounting information for each block including
 * two small magic numbers and a bucket number when in use or a next pointer
 * when free. The original requested size (not including the Block overhead)
 * is also maintained.
 */

typedef union Block {
    struct {
	union {
	    union Block *next;		/* Next in free list. */
	    struct {
		unsigned char magic1;	/* First magic number. */
		unsigned char bucket;	/* Bucket block allocated from. */
		unsigned char unused;	/* Padding. */
		unsigned char magic2;	/* Second magic number. */
	    } s;
	} u;
	size_t reqSize;			/* Requested allocation size. */
    } b;
    unsigned char padding[TCL_ALLOCALIGN];
} Block;
#define nextBlock	b.u.next
#define sourceBucket	b.u.s.bucket
#define magicNum1	b.u.s.magic1
#define magicNum2	b.u.s.magic2
#define MAGIC		0xEF
#define blockReqSize	b.reqSize

/*
 * The following defines the minimum and and maximum block sizes and the number
 * of buckets in the bucket cache.
 */

#define MINALLOC	((sizeof(Block) + 8 + (TCL_ALLOCALIGN-1)) & ~(TCL_ALLOCALIGN-1))
#define NBUCKETS	(11 - (MINALLOC >> 5))
#define MAXALLOC	(MINALLOC << (NBUCKETS - 1))

/*
 * The following structure defines a bucket of blocks with various accounting
 * and statistics information.
 */

typedef struct Bucket {
    Block *firstPtr;		/* First block available */
    Block *lastPtr;		/* End of block list */
    long numFree;		/* Number of blocks available */

    /* All fields below for accounting only */

    long numRemoves;		/* Number of removes from bucket */
    long numInserts;		/* Number of inserts into bucket */
    long numWaits;		/* Number of waits to acquire a lock */
    long numLocks;		/* Number of locks acquired */
    long totalAssigned;		/* Total space assigned to bucket */
} Bucket;

/*
 * The following structure defines a cache of buckets and objs, of which there
 * will be (at most) one per thread. Any changes need to be reflected in the
 * struct AllocCache defined in tclInt.h, possibly also in the initialisation
 * code in Tcl_CreateInterp().
 */

typedef struct Cache {
    struct Cache *nextPtr;	/* Linked list of cache entries */
    Tcl_ThreadId owner;		/* Which thread's cache is this? */
    Tcl_Obj *firstObjPtr;	/* List of free objects for thread */
    int numObjects;		/* Number of objects for thread */
    Tcl_Obj *lastPtr;		/* Last object in this cache */
    int totalAssigned;		/* Total space assigned to thread */
    Bucket buckets[NBUCKETS];	/* The buckets for this thread */
} Cache;

/*
 * The following array specifies various per-bucket limits and locks. The
 * values are statically initialized to avoid calculating them repeatedly.
 */

static struct {
    size_t blockSize;		/* Bucket blocksize. */
    int maxBlocks;		/* Max blocks before move to share. */
    int numMove;		/* Num blocks to move to share. */
    Tcl_Mutex *lockPtr;		/* Share bucket lock. */
} bucketInfo[NBUCKETS];

/*
 * Static functions defined in this file.
 */

static Cache *	GetCache(void);
static void	LockBucket(Cache *cachePtr, int bucket);
static void	UnlockBucket(Cache *cachePtr, int bucket);
static void	PutBlocks(Cache *cachePtr, int bucket, int numMove);
static int	GetBlocks(Cache *cachePtr, int bucket);
static Block *	Ptr2Block(char *ptr);
static char *	Block2Ptr(Block *blockPtr, int bucket, unsigned int reqSize);
static void	MoveObjs(Cache *fromPtr, Cache *toPtr, int numMove);
static void	PutObjs(Cache *fromPtr, int numMove);

/*
 * Local variables defined in this file and initialized at startup.
 */

static Tcl_Mutex *listLockPtr;
static Tcl_Mutex *objLockPtr;
static Cache sharedCache;
static Cache *sharedPtr = &sharedCache;
static Cache *firstCachePtr = &sharedCache;

#if defined(HAVE_FAST_TSD)
static __thread Cache *tcachePtr;

# define GETCACHE(cachePtr)			\
    do {					\
	if (!tcachePtr) {			\
	    tcachePtr = GetCache();		\
	}					\
	(cachePtr) = tcachePtr;			\
    } while (0)
#else
# define GETCACHE(cachePtr)			\
    do {					\
	(cachePtr) = TclpGetAllocCache();	\
	if ((cachePtr) == NULL) {		\
	    (cachePtr) = GetCache();		\
	}					\
    } while (0)
#endif

/*
 *----------------------------------------------------------------------
 *
 * GetCache ---
 *
 *	Gets per-thread memory cache, allocating it if necessary.
 *
 * Results:
 *	Pointer to cache.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Cache *
GetCache(void)
{
    Cache *cachePtr;

    /*
     * Check for first-time initialization.
     */

    if (listLockPtr == NULL) {
	Tcl_Mutex *initLockPtr;
	unsigned int i;

	initLockPtr = Tcl_GetAllocMutex();
	Tcl_MutexLock(initLockPtr);
	if (listLockPtr == NULL) {
	    listLockPtr = TclpNewAllocMutex();
	    objLockPtr = TclpNewAllocMutex();
	    for (i = 0; i < NBUCKETS; ++i) {
		bucketInfo[i].blockSize = MINALLOC << i;
		bucketInfo[i].maxBlocks = 1 << (NBUCKETS - 1 - i);
		bucketInfo[i].numMove = i < NBUCKETS - 1 ?
			1 << (NBUCKETS - 2 - i) : 1;
		bucketInfo[i].lockPtr = TclpNewAllocMutex();
	    }
	}
	Tcl_MutexUnlock(initLockPtr);
    }

    /*
     * Get this thread's cache, allocating if necessary.
     */

    cachePtr = TclpGetAllocCache();
    if (cachePtr == NULL) {
	cachePtr = TclpSysAlloc(sizeof(Cache), 0);
	if (cachePtr == NULL) {
	    Tcl_Panic("alloc: could not allocate new cache");
	}
        memset(cachePtr, 0, sizeof(Cache));
	Tcl_MutexLock(listLockPtr);
	cachePtr->nextPtr = firstCachePtr;
	firstCachePtr = cachePtr;
	Tcl_MutexUnlock(listLockPtr);
	cachePtr->owner = Tcl_GetCurrentThread();
	TclpSetAllocCache(cachePtr);
    }
    return cachePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFreeAllocCache --
 *
 *	Flush and delete a cache, removing from list of caches.
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
TclFreeAllocCache(
    void *arg)
{
    Cache *cachePtr = arg;
    Cache **nextPtrPtr;
    register unsigned int bucket;

    /*
     * Flush blocks.
     */

    for (bucket = 0; bucket < NBUCKETS; ++bucket) {
	if (cachePtr->buckets[bucket].numFree > 0) {
	    PutBlocks(cachePtr, bucket, cachePtr->buckets[bucket].numFree);
	}
    }

    /*
     * Flush objs.
     */

    if (cachePtr->numObjects > 0) {
	PutObjs(cachePtr, cachePtr->numObjects);
    }

    /*
     * Remove from pool list.
     */

    Tcl_MutexLock(listLockPtr);
    nextPtrPtr = &firstCachePtr;
    while (*nextPtrPtr != cachePtr) {
	nextPtrPtr = &(*nextPtrPtr)->nextPtr;
    }
    *nextPtrPtr = cachePtr->nextPtr;
    cachePtr->nextPtr = NULL;
    Tcl_MutexUnlock(listLockPtr);
    TclpSysFree(cachePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpAlloc --
 *
 *	Allocate memory.
 *
 * Results:
 *	Pointer to memory just beyond Block pointer.
 *
 * Side effects:
 *	May allocate more blocks for a bucket.
 *
 *----------------------------------------------------------------------
 */

char *
TclpAlloc(
    unsigned int reqSize)
{
    Cache *cachePtr;
    Block *blockPtr;
    register int bucket;
    size_t size;

#ifndef __LP64__
    if (sizeof(int) >= sizeof(size_t)) {
	/* An unsigned int overflow can also be a size_t overflow */
	const size_t zero = 0;
	const size_t max = ~zero;

	if (((size_t) reqSize) > max - sizeof(Block) - RCHECK) {
	    /* Requested allocation exceeds memory */
	    return NULL;
	}
    }
#endif

    GETCACHE(cachePtr);

    /*
     * Increment the requested size to include room for the Block structure.
     * Call TclpSysAlloc() directly if the required amount is greater than the
     * largest block, otherwise pop the smallest block large enough,
     * allocating more blocks if necessary.
     */

    blockPtr = NULL;
    size = reqSize + sizeof(Block);
#if RCHECK
    size++;
#endif
    if (size > MAXALLOC) {
	bucket = NBUCKETS;
	blockPtr = TclpSysAlloc(size, 0);
	if (blockPtr != NULL) {
	    cachePtr->totalAssigned += reqSize;
	}
    } else {
	bucket = 0;
	while (bucketInfo[bucket].blockSize < size) {
	    bucket++;
	}
	if (cachePtr->buckets[bucket].numFree || GetBlocks(cachePtr, bucket)) {
	    blockPtr = cachePtr->buckets[bucket].firstPtr;
	    cachePtr->buckets[bucket].firstPtr = blockPtr->nextBlock;
	    cachePtr->buckets[bucket].numFree--;
	    cachePtr->buckets[bucket].numRemoves++;
	    cachePtr->buckets[bucket].totalAssigned += reqSize;
	}
    }
    if (blockPtr == NULL) {
	return NULL;
    }
    return Block2Ptr(blockPtr, bucket, reqSize);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFree --
 *
 *	Return blocks to the thread block cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May move blocks to shared cache.
 *
 *----------------------------------------------------------------------
 */

void
TclpFree(
    char *ptr)
{
    Cache *cachePtr;
    Block *blockPtr;
    int bucket;

    if (ptr == NULL) {
	return;
    }

    GETCACHE(cachePtr);

    /*
     * Get the block back from the user pointer and call system free directly
     * for large blocks. Otherwise, push the block back on the bucket and move
     * blocks to the shared cache if there are now too many free.
     */

    blockPtr = Ptr2Block(ptr);
    bucket = blockPtr->sourceBucket;
    if (bucket == NBUCKETS) {
	cachePtr->totalAssigned -= blockPtr->blockReqSize;
	TclpSysFree(blockPtr);
	return;
    }

    cachePtr->buckets[bucket].totalAssigned -= blockPtr->blockReqSize;
    blockPtr->nextBlock = cachePtr->buckets[bucket].firstPtr;
    cachePtr->buckets[bucket].firstPtr = blockPtr;
    if (cachePtr->buckets[bucket].numFree == 0) {
	cachePtr->buckets[bucket].lastPtr = blockPtr;
    }
    cachePtr->buckets[bucket].numFree++;
    cachePtr->buckets[bucket].numInserts++;

    if (cachePtr != sharedPtr &&
	    cachePtr->buckets[bucket].numFree > bucketInfo[bucket].maxBlocks) {
	PutBlocks(cachePtr, bucket, bucketInfo[bucket].numMove);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpRealloc --
 *
 *	Re-allocate memory to a larger or smaller size.
 *
 * Results:
 *	Pointer to memory just beyond Block pointer.
 *
 * Side effects:
 *	Previous memory, if any, may be freed.
 *
 *----------------------------------------------------------------------
 */

char *
TclpRealloc(
    char *ptr,
    unsigned int reqSize)
{
    Cache *cachePtr;
    Block *blockPtr;
    void *newPtr;
    size_t size, min;
    int bucket;

    if (ptr == NULL) {
	return TclpAlloc(reqSize);
    }

#ifndef __LP64__
    if (sizeof(int) >= sizeof(size_t)) {
	/* An unsigned int overflow can also be a size_t overflow */
	const size_t zero = 0;
	const size_t max = ~zero;

	if (((size_t) reqSize) > max - sizeof(Block) - RCHECK) {
	    /* Requested allocation exceeds memory */
	    return NULL;
	}
    }
#endif

    GETCACHE(cachePtr);

    /*
     * If the block is not a system block and fits in place, simply return the
     * existing pointer. Otherwise, if the block is a system block and the new
     * size would also require a system block, call TclpSysRealloc() directly.
     */

    blockPtr = Ptr2Block(ptr);
    size = reqSize + sizeof(Block);
#if RCHECK
    size++;
#endif
    bucket = blockPtr->sourceBucket;
    if (bucket != NBUCKETS) {
	if (bucket > 0) {
	    min = bucketInfo[bucket-1].blockSize;
	} else {
	    min = 0;
	}
	if (size > min && size <= bucketInfo[bucket].blockSize) {
	    cachePtr->buckets[bucket].totalAssigned -= blockPtr->blockReqSize;
	    cachePtr->buckets[bucket].totalAssigned += reqSize;
	    return Block2Ptr(blockPtr, bucket, reqSize);
	}
    } else if (size > MAXALLOC) {
	cachePtr->totalAssigned -= blockPtr->blockReqSize;
	cachePtr->totalAssigned += reqSize;
	blockPtr = TclpSysRealloc(blockPtr, size);
	if (blockPtr == NULL) {
	    return NULL;
	}
	return Block2Ptr(blockPtr, NBUCKETS, reqSize);
    }

    /*
     * Finally, perform an expensive malloc/copy/free.
     */

    newPtr = TclpAlloc(reqSize);
    if (newPtr != NULL) {
	if (reqSize > blockPtr->blockReqSize) {
	    reqSize = blockPtr->blockReqSize;
	}
	memcpy(newPtr, ptr, reqSize);
	TclpFree(ptr);
    }
    return newPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclThreadAllocObj --
 *
 *	Allocate a Tcl_Obj from the per-thread cache.
 *
 * Results:
 *	Pointer to uninitialized Tcl_Obj.
 *
 * Side effects:
 *	May move Tcl_Obj's from shared cached or allocate new Tcl_Obj's if
 *	list is empty.
 *
 * Note:
 *	If this code is updated, the changes need to be reflected in the macro
 *	TclAllocObjStorageEx() defined in tclInt.h
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclThreadAllocObj(void)
{
    register Cache *cachePtr;
    register Tcl_Obj *objPtr;

    GETCACHE(cachePtr);

    /*
     * Get this thread's obj list structure and move or allocate new objs if
     * necessary.
     */

    if (cachePtr->numObjects == 0) {
	register int numMove;

	Tcl_MutexLock(objLockPtr);
	numMove = sharedPtr->numObjects;
	if (numMove > 0) {
	    if (numMove > NOBJALLOC) {
		numMove = NOBJALLOC;
	    }
	    MoveObjs(sharedPtr, cachePtr, numMove);
	}
	Tcl_MutexUnlock(objLockPtr);
	if (cachePtr->numObjects == 0) {
	    Tcl_Obj *newObjsPtr;

	    cachePtr->numObjects = numMove = NOBJALLOC;
	    newObjsPtr = TclpSysAlloc(sizeof(Tcl_Obj) * numMove, 0);
	    if (newObjsPtr == NULL) {
		Tcl_Panic("alloc: could not allocate %d new objects", numMove);
	    }
	    cachePtr->lastPtr = newObjsPtr + numMove - 1;
	    objPtr = cachePtr->firstObjPtr;	/* NULL */
	    while (--numMove >= 0) {
		newObjsPtr[numMove].internalRep.twoPtrValue.ptr1 = objPtr;
		objPtr = newObjsPtr + numMove;
	    }
	    cachePtr->firstObjPtr = newObjsPtr;
	}
    }

    /*
     * Pop the first object.
     */

    objPtr = cachePtr->firstObjPtr;
    cachePtr->firstObjPtr = objPtr->internalRep.twoPtrValue.ptr1;
    cachePtr->numObjects--;
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclThreadFreeObj --
 *
 *	Return a free Tcl_Obj to the per-thread cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May move free Tcl_Obj's to shared list upon hitting high water mark.
 *
 * Note:
 *	If this code is updated, the changes need to be reflected in the macro
 *	TclAllocObjStorageEx() defined in tclInt.h
 *
 *----------------------------------------------------------------------
 */

void
TclThreadFreeObj(
    Tcl_Obj *objPtr)
{
    Cache *cachePtr;

    GETCACHE(cachePtr);

    /*
     * Get this thread's list and push on the free Tcl_Obj.
     */

    objPtr->internalRep.twoPtrValue.ptr1 = cachePtr->firstObjPtr;
    cachePtr->firstObjPtr = objPtr;
    if (cachePtr->numObjects == 0) {
	cachePtr->lastPtr = objPtr;
    }
    cachePtr->numObjects++;

    /*
     * If the number of free objects has exceeded the high water mark, move
     * some blocks to the shared list.
     */

    if (cachePtr->numObjects > NOBJHIGH) {
	PutObjs(cachePtr, NOBJALLOC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetMemoryInfo --
 *
 *	Return a list-of-lists of memory stats.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	List appended to given dstring.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetMemoryInfo(
    Tcl_DString *dsPtr)
{
    Cache *cachePtr;
    char buf[200];
    unsigned int n;

    Tcl_MutexLock(listLockPtr);
    cachePtr = firstCachePtr;
    while (cachePtr != NULL) {
	Tcl_DStringStartSublist(dsPtr);
	if (cachePtr == sharedPtr) {
	    Tcl_DStringAppendElement(dsPtr, "shared");
	} else {
	    sprintf(buf, "thread%p", cachePtr->owner);
	    Tcl_DStringAppendElement(dsPtr, buf);
	}
	for (n = 0; n < NBUCKETS; ++n) {
	    sprintf(buf, "%lu %ld %ld %ld %ld %ld %ld",
		    (unsigned long) bucketInfo[n].blockSize,
		    cachePtr->buckets[n].numFree,
		    cachePtr->buckets[n].numRemoves,
		    cachePtr->buckets[n].numInserts,
		    cachePtr->buckets[n].totalAssigned,
		    cachePtr->buckets[n].numLocks,
		    cachePtr->buckets[n].numWaits);
	    Tcl_DStringAppendElement(dsPtr, buf);
	}
	Tcl_DStringEndSublist(dsPtr);
	cachePtr = cachePtr->nextPtr;
    }
    Tcl_MutexUnlock(listLockPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * MoveObjs --
 *
 *	Move Tcl_Obj's between caches.
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
MoveObjs(
    Cache *fromPtr,
    Cache *toPtr,
    int numMove)
{
    register Tcl_Obj *objPtr = fromPtr->firstObjPtr;
    Tcl_Obj *fromFirstObjPtr = objPtr;

    toPtr->numObjects += numMove;
    fromPtr->numObjects -= numMove;

    /*
     * Find the last object to be moved; set the next one (the first one not
     * to be moved) as the first object in the 'from' cache.
     */

    while (--numMove) {
	objPtr = objPtr->internalRep.twoPtrValue.ptr1;
    }
    fromPtr->firstObjPtr = objPtr->internalRep.twoPtrValue.ptr1;

    /*
     * Move all objects as a block - they are already linked to each other, we
     * just have to update the first and last.
     */

    toPtr->lastPtr = objPtr;
    objPtr->internalRep.twoPtrValue.ptr1 = toPtr->firstObjPtr; /* NULL */
    toPtr->firstObjPtr = fromFirstObjPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * PutObjs --
 *
 *	Move Tcl_Obj's from thread cache to shared cache.
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
PutObjs(
    Cache *fromPtr,
    int numMove)
{
    int keep = fromPtr->numObjects - numMove;
    Tcl_Obj *firstPtr, *lastPtr = NULL;

    fromPtr->numObjects = keep;
    firstPtr = fromPtr->firstObjPtr;
    if (keep == 0) {
	fromPtr->firstObjPtr = NULL;
    } else {
	do {
	    lastPtr = firstPtr;
	    firstPtr = firstPtr->internalRep.twoPtrValue.ptr1;
	} while (--keep > 0);
	lastPtr->internalRep.twoPtrValue.ptr1 = NULL;
    }

    /*
     * Move all objects as a block - they are already linked to each other, we
     * just have to update the first and last.
     */

    Tcl_MutexLock(objLockPtr);
    fromPtr->lastPtr->internalRep.twoPtrValue.ptr1 = sharedPtr->firstObjPtr;
    sharedPtr->firstObjPtr = firstPtr;
    if (sharedPtr->numObjects == 0) {
	sharedPtr->lastPtr = fromPtr->lastPtr;
    }
    sharedPtr->numObjects += numMove;
    Tcl_MutexUnlock(objLockPtr);

    fromPtr->lastPtr = lastPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Block2Ptr, Ptr2Block --
 *
 *	Convert between internal blocks and user pointers.
 *
 * Results:
 *	User pointer or internal block.
 *
 * Side effects:
 *	Invalid blocks will abort the server.
 *
 *----------------------------------------------------------------------
 */

static char *
Block2Ptr(
    Block *blockPtr,
    int bucket,
    unsigned int reqSize)
{
    register void *ptr;

    blockPtr->magicNum1 = blockPtr->magicNum2 = MAGIC;
    blockPtr->sourceBucket = bucket;
    blockPtr->blockReqSize = reqSize;
    ptr = ((void *) (blockPtr + 1));
#if RCHECK
    ((unsigned char *)(ptr))[reqSize] = MAGIC;
#endif
    return (char *) ptr;
}

static Block *
Ptr2Block(
    char *ptr)
{
    register Block *blockPtr;

    blockPtr = (((Block *) ptr) - 1);
    if (blockPtr->magicNum1 != MAGIC || blockPtr->magicNum2 != MAGIC) {
	Tcl_Panic("alloc: invalid block: %p: %x %x",
		blockPtr, blockPtr->magicNum1, blockPtr->magicNum2);
    }
#if RCHECK
    if (((unsigned char *) ptr)[blockPtr->blockReqSize] != MAGIC) {
	Tcl_Panic("alloc: invalid block: %p: %x %x %x",
		blockPtr, blockPtr->magicNum1, blockPtr->magicNum2,
		((unsigned char *) ptr)[blockPtr->blockReqSize]);
    }
#endif
    return blockPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * LockBucket, UnlockBucket --
 *
 *	Set/unset the lock to access a bucket in the shared cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lock activity and contention are monitored globally and on a per-cache
 *	basis.
 *
 *----------------------------------------------------------------------
 */

static void
LockBucket(
    Cache *cachePtr,
    int bucket)
{
    Tcl_MutexLock(bucketInfo[bucket].lockPtr);
    cachePtr->buckets[bucket].numLocks++;
    sharedPtr->buckets[bucket].numLocks++;
}

static void
UnlockBucket(
    Cache *cachePtr,
    int bucket)
{
    Tcl_MutexUnlock(bucketInfo[bucket].lockPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * PutBlocks --
 *
 *	Return unused blocks to the shared cache.
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
PutBlocks(
    Cache *cachePtr,
    int bucket,
    int numMove)
{
    /*
     * We have numFree.  Want to shed numMove. So compute how many
     * Blocks to keep.
     */

    int keep = cachePtr->buckets[bucket].numFree - numMove;
    Block *lastPtr = NULL, *firstPtr;

    cachePtr->buckets[bucket].numFree = keep;
    firstPtr = cachePtr->buckets[bucket].firstPtr;
    if (keep == 0) {
	cachePtr->buckets[bucket].firstPtr = NULL;
    } else {
	do {
	    lastPtr = firstPtr;
	    firstPtr = firstPtr->nextBlock;
	} while (--keep > 0);
	lastPtr->nextBlock = NULL;
    }

    /*
     * Aquire the lock and place the list of blocks at the front of the shared
     * cache bucket.
     */

    LockBucket(cachePtr, bucket);
    cachePtr->buckets[bucket].lastPtr->nextBlock
	    = sharedPtr->buckets[bucket].firstPtr;
    sharedPtr->buckets[bucket].firstPtr = firstPtr;
    if (sharedPtr->buckets[bucket].numFree == 0) {
	sharedPtr->buckets[bucket].lastPtr
		= cachePtr->buckets[bucket].lastPtr;
    }
    sharedPtr->buckets[bucket].numFree += numMove;
    UnlockBucket(cachePtr, bucket);

    cachePtr->buckets[bucket].lastPtr = lastPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetBlocks --
 *
 *	Get more blocks for a bucket.
 *
 * Results:
 *	1 if blocks where allocated, 0 otherwise.
 *
 * Side effects:
 *	Cache may be filled with available blocks.
 *
 *----------------------------------------------------------------------
 */

static int
GetBlocks(
    Cache *cachePtr,
    int bucket)
{
    register Block *blockPtr;
    register int n;

    /*
     * First, atttempt to move blocks from the shared cache. Note the
     * potentially dirty read of numFree before acquiring the lock which is a
     * slight performance enhancement. The value is verified after the lock is
     * actually acquired.
     */

    if (cachePtr != sharedPtr && sharedPtr->buckets[bucket].numFree > 0) {
	LockBucket(cachePtr, bucket);
	if (sharedPtr->buckets[bucket].numFree > 0) {

	    /*
	     * Either move the entire list or walk the list to find the last
	     * block to move.
	     */

	    n = bucketInfo[bucket].numMove;
	    if (n >= sharedPtr->buckets[bucket].numFree) {
		cachePtr->buckets[bucket].firstPtr =
			sharedPtr->buckets[bucket].firstPtr;
		cachePtr->buckets[bucket].lastPtr =
			sharedPtr->buckets[bucket].lastPtr;
		cachePtr->buckets[bucket].numFree =
			sharedPtr->buckets[bucket].numFree;
		sharedPtr->buckets[bucket].firstPtr = NULL;
		sharedPtr->buckets[bucket].numFree = 0;
	    } else {
		blockPtr = sharedPtr->buckets[bucket].firstPtr;
		cachePtr->buckets[bucket].firstPtr = blockPtr;
		sharedPtr->buckets[bucket].numFree -= n;
		cachePtr->buckets[bucket].numFree = n;
		while (--n > 0) {
		    blockPtr = blockPtr->nextBlock;
		}
		sharedPtr->buckets[bucket].firstPtr = blockPtr->nextBlock;
		cachePtr->buckets[bucket].lastPtr = blockPtr;
		blockPtr->nextBlock = NULL;
	    }
	}
	UnlockBucket(cachePtr, bucket);
    }

    if (cachePtr->buckets[bucket].numFree == 0) {
	register size_t size;

	/*
	 * If no blocks could be moved from shared, first look for a larger
	 * block in this cache to split up.
	 */

	blockPtr = NULL;
	n = NBUCKETS;
	size = 0; /* lint */
	while (--n > bucket) {
	    if (cachePtr->buckets[n].numFree > 0) {
		size = bucketInfo[n].blockSize;
		blockPtr = cachePtr->buckets[n].firstPtr;
		cachePtr->buckets[n].firstPtr = blockPtr->nextBlock;
		cachePtr->buckets[n].numFree--;
		break;
	    }
	}

	/*
	 * Otherwise, allocate a big new block directly.
	 */

	if (blockPtr == NULL) {
	    size = MAXALLOC;
	    blockPtr = TclpSysAlloc(size, 0);
	    if (blockPtr == NULL) {
		return 0;
	    }
	}

	/*
	 * Split the larger block into smaller blocks for this bucket.
	 */

	n = size / bucketInfo[bucket].blockSize;
	cachePtr->buckets[bucket].numFree = n;
	cachePtr->buckets[bucket].firstPtr = blockPtr;
	while (--n > 0) {
	    blockPtr->nextBlock = (Block *)
		((char *) blockPtr + bucketInfo[bucket].blockSize);
	    blockPtr = blockPtr->nextBlock;
	}
	cachePtr->buckets[bucket].lastPtr = blockPtr;
	blockPtr->nextBlock = NULL;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeThreadAlloc --
 *
 *	This procedure is used to destroy all private resources used in this
 *	file.
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
TclFinalizeThreadAlloc(void)
{
    unsigned int i;

    for (i = 0; i < NBUCKETS; ++i) {
	TclpFreeAllocMutex(bucketInfo[i].lockPtr);
	bucketInfo[i].lockPtr = NULL;
    }

    TclpFreeAllocMutex(objLockPtr);
    objLockPtr = NULL;

    TclpFreeAllocMutex(listLockPtr);
    listLockPtr = NULL;

    TclpFreeAllocCache(NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeThreadAllocThread --
 *
 *	This procedure is used to destroy single thread private resources
 *	defined in this file. Called either during Tcl_FinalizeThread() or
 *	Tcl_Finalize().
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
TclFinalizeThreadAllocThread(void)
{
    Cache *cachePtr = TclpGetAllocCache();
    if (cachePtr != NULL) {
	TclpFreeAllocCache(cachePtr);
    }
}

#else /* !(TCL_THREADS && USE_THREAD_ALLOC) */
/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetMemoryInfo --
 *
 *	Return a list-of-lists of memory stats.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	List appended to given dstring.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetMemoryInfo(
    Tcl_DString *dsPtr)
{
    Tcl_Panic("Tcl_GetMemoryInfo called when threaded memory allocator not in use");
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeThreadAlloc --
 *
 *	This procedure is used to destroy all private resources used in this
 *	file.
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
TclFinalizeThreadAlloc(void)
{
    Tcl_Panic("TclFinalizeThreadAlloc called when threaded memory allocator not in use");
}
#endif /* TCL_THREADS && USE_THREAD_ALLOC */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
