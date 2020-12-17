/*
 * tclLiteral.c --
 *
 *	Implementation of the global and ByteCode-local literal tables used to
 *	manage the Tcl objects created for literal values during compilation
 *	of Tcl scripts. This implementation borrows heavily from the more
 *	general hashtable implementation of Tcl hash tables that appears in
 *	tclHash.c.
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 * Copyright (c) 2004 by Kevin B. Kenny.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"

/*
 * When there are this many entries per bucket, on average, rebuild a
 * literal's hash table to make it larger.
 */

#define REBUILD_MULTIPLIER	3

/*
 * Function prototypes for static functions in this file:
 */

static int		AddLocalLiteralEntry(CompileEnv *envPtr,
			    Tcl_Obj *objPtr, int localHash);
static void		ExpandLocalLiteralArray(CompileEnv *envPtr);
static unsigned		HashString(const char *string, int length);
#ifdef TCL_COMPILE_DEBUG
static LiteralEntry *	LookupLiteralEntry(Tcl_Interp *interp,
			    Tcl_Obj *objPtr);
#endif
static void		RebuildLiteralTable(LiteralTable *tablePtr);

/*
 *----------------------------------------------------------------------
 *
 * TclInitLiteralTable --
 *
 *	This function is called to initialize the fields of a literal table
 *	structure for either an interpreter or a compilation's CompileEnv
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The literal table is made ready for use.
 *
 *----------------------------------------------------------------------
 */

void
TclInitLiteralTable(
    register LiteralTable *tablePtr)
				/* Pointer to table structure, which is
				 * supplied by the caller. */
{
#if (TCL_SMALL_HASH_TABLE != 4)
    Tcl_Panic("%s: TCL_SMALL_HASH_TABLE is %d, not 4", "TclInitLiteralTable",
	    TCL_SMALL_HASH_TABLE);
#endif

    tablePtr->buckets = tablePtr->staticBuckets;
    tablePtr->staticBuckets[0] = tablePtr->staticBuckets[1] = 0;
    tablePtr->staticBuckets[2] = tablePtr->staticBuckets[3] = 0;
    tablePtr->numBuckets = TCL_SMALL_HASH_TABLE;
    tablePtr->numEntries = 0;
    tablePtr->rebuildSize = TCL_SMALL_HASH_TABLE * REBUILD_MULTIPLIER;
    tablePtr->mask = 3;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteLiteralTable --
 *
 *	This function frees up everything associated with a literal table
 *	except for the table's structure itself. It is called when the
 *	interpreter is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Each literal in the table is released: i.e., its reference count in
 *	the global literal table is decremented and, if it becomes zero, the
 *	literal is freed. In addition, the table's bucket array is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteLiteralTable(
    Tcl_Interp *interp,		/* Interpreter containing shared literals
				 * referenced by the table to delete. */
    LiteralTable *tablePtr)	/* Points to the literal table to delete. */
{
    LiteralEntry *entryPtr, *nextPtr;
    Tcl_Obj *objPtr;
    int i;

    /*
     * Release remaining literals in the table. Note that releasing a literal
     * might release other literals, modifying the table, so we restart the
     * search from the bucket chain we last found an entry.
     */

#ifdef TCL_COMPILE_DEBUG
    TclVerifyGlobalLiteralTable((Interp *) interp);
#endif /*TCL_COMPILE_DEBUG*/

    /*
     * We used to call TclReleaseLiteral for each literal in the table, which
     * is rather inefficient as it causes one lookup-by-hash for each
     * reference to the literal. We now rely at interp-deletion on each
     * bytecode object to release its references to the literal Tcl_Obj
     * without requiring that it updates the global table itself, and deal
     * here only with the table.
     */

    for (i=0 ; i<tablePtr->numBuckets ; i++) {
	entryPtr = tablePtr->buckets[i];
	while (entryPtr != NULL) {
	    objPtr = entryPtr->objPtr;
	    TclDecrRefCount(objPtr);
	    nextPtr = entryPtr->nextPtr;
	    ckfree(entryPtr);
	    entryPtr = nextPtr;
	}
    }

    /*
     * Free up the table's bucket array if it was dynamically allocated.
     */

    if (tablePtr->buckets != tablePtr->staticBuckets) {
	ckfree(tablePtr->buckets);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreateLiteral --
 *
 *	Find, or if necessary create, an object in the interpreter's literal
 *	table that has a string representation matching the argument
 *	string. If nsPtr!=NULL then only literals stored for the namespace are
 *	considered.
 *
 * Results:
 *	The literal object. If it was created in this call *newPtr is set to
 *	1, else 0. NULL is returned if newPtr==NULL and no literal is found.
 *
 * Side effects:
 *	Increments the ref count of the global LiteralEntry since the caller
 *	now holds a reference. If LITERAL_ON_HEAP is set in flags, this
 *	function is given ownership of the string: if an object is created
 *	then its string representation is set directly from string, otherwise
 *	the string is freed. Typically, a caller sets LITERAL_ON_HEAP if
 *	"string" is an already heap-allocated buffer holding the result of
 *	backslash substitutions.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclCreateLiteral(
    Interp *iPtr,
    char *bytes,		/* The start of the string. Note that this is
				 * not a NUL-terminated string. */
    int length,			/* Number of bytes in the string. */
    unsigned hash,		/* The string's hash. If -1, it will be
				 * computed here. */
    int *newPtr,
    Namespace *nsPtr,
    int flags,
    LiteralEntry **globalPtrPtr)
{
    LiteralTable *globalTablePtr = &iPtr->literalTable;
    LiteralEntry *globalPtr;
    int globalHash;
    Tcl_Obj *objPtr;

    /*
     * Is it in the interpreter's global literal table?
     */

    if (hash == (unsigned) -1) {
	hash = HashString(bytes, length);
    }
    globalHash = (hash & globalTablePtr->mask);
    for (globalPtr=globalTablePtr->buckets[globalHash] ; globalPtr!=NULL;
	    globalPtr = globalPtr->nextPtr) {
	objPtr = globalPtr->objPtr;
	if ((globalPtr->nsPtr == nsPtr)
		&& (objPtr->length == length) && ((length == 0)
		|| ((objPtr->bytes[0] == bytes[0])
		&& (memcmp(objPtr->bytes, bytes, (unsigned) length) == 0)))) {
	    /*
	     * A literal was found: return it
	     */

	    if (newPtr) {
		*newPtr = 0;
	    }
	    if (globalPtrPtr) {
		*globalPtrPtr = globalPtr;
	    }
	    if ((flags & LITERAL_ON_HEAP)) {
		ckfree(bytes);
	    }
	    globalPtr->refCount++;
	    return objPtr;
	}
    }
    if (!newPtr) {
	if ((flags & LITERAL_ON_HEAP)) {
	    ckfree(bytes);
	}
	return NULL;
    }

    /*
     * The literal is new to the interpreter. Add it to the global literal
     * table.
     */

    TclNewObj(objPtr);
    if ((flags & LITERAL_ON_HEAP)) {
	objPtr->bytes = bytes;
	objPtr->length = length;
    } else {
	TclInitStringRep(objPtr, bytes, length);
    }

    if ((flags & LITERAL_UNSHARED)) {
	/*
	 * Make clear, that no global value is returned
	 */
	if (globalPtrPtr != NULL) {
	    *globalPtrPtr = NULL;
	}
	return objPtr;
    }

#ifdef TCL_COMPILE_DEBUG
    if (LookupLiteralEntry((Tcl_Interp *) iPtr, objPtr) != NULL) {
	Tcl_Panic("%s: literal \"%.*s\" found globally but shouldn't be",
		"TclRegisterLiteral", (length>60? 60 : length), bytes);
    }
#endif

    globalPtr = ckalloc(sizeof(LiteralEntry));
    globalPtr->objPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    globalPtr->refCount = 1;
    globalPtr->nsPtr = nsPtr;
    globalPtr->nextPtr = globalTablePtr->buckets[globalHash];
    globalTablePtr->buckets[globalHash] = globalPtr;
    globalTablePtr->numEntries++;

    /*
     * If the global literal table has exceeded a decent size, rebuild it with
     * more buckets.
     */

    if (globalTablePtr->numEntries >= globalTablePtr->rebuildSize) {
	RebuildLiteralTable(globalTablePtr);
    }

#ifdef TCL_COMPILE_DEBUG
    TclVerifyGlobalLiteralTable(iPtr);
    {
	LiteralEntry *entryPtr;
	int found, i;

	found = 0;
	for (i=0 ; i<globalTablePtr->numBuckets ; i++) {
	    for (entryPtr=globalTablePtr->buckets[i]; entryPtr!=NULL ;
		    entryPtr=entryPtr->nextPtr) {
		if ((entryPtr == globalPtr) && (entryPtr->objPtr == objPtr)) {
		    found = 1;
		}
	    }
	}
	if (!found) {
	    Tcl_Panic("%s: literal \"%.*s\" wasn't global",
		    "TclRegisterLiteral", (length>60? 60 : length), bytes);
	}
    }
#endif /*TCL_COMPILE_DEBUG*/

#ifdef TCL_COMPILE_STATS
    iPtr->stats.numLiteralsCreated++;
    iPtr->stats.totalLitStringBytes += (double) (length + 1);
    iPtr->stats.currentLitStringBytes += (double) (length + 1);
    iPtr->stats.literalCount[TclLog2(length)]++;
#endif /*TCL_COMPILE_STATS*/

    if (globalPtrPtr) {
	*globalPtrPtr = globalPtr;
    }
    *newPtr = 1;
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFetchLiteral --
 *
 *	Fetch from a CompileEnv the literal value identified by an index
 *	value, as returned by a prior call to TclRegisterLiteral().
 *
 * Results:
 *	The literal value, or NULL if the index is out of range.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclFetchLiteral(
    CompileEnv *envPtr,		/* Points to the CompileEnv from which to
				 * fetch the registered literal value. */
    unsigned int index)		/* Index of the desired literal, as returned
				 * by prior call to TclRegisterLiteral() */
{
    if (index >= (unsigned int) envPtr->literalArrayNext) {
	return NULL;
    }
    return envPtr->literalArrayPtr[index].objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclRegisterLiteral --
 *
 *	Find, or if necessary create, an object in a CompileEnv literal array
 *	that has a string representation matching the argument string.
 *
 * Results:
 *	The index in the CompileEnv's literal array that references a shared
 *	literal matching the string. The object is created if necessary.
 *
 * Side effects:
 *	To maximize sharing, we look up the string in the interpreter's global
 *	literal table. If not found, we create a new shared literal in the
 *	global table. We then add a reference to the shared literal in the
 *	CompileEnv's literal array.
 *
 *	If LITERAL_ON_HEAP is set in flags, this function is given ownership
 *	of the string: if an object is created then its string representation
 *	is set directly from string, otherwise the string is freed. Typically,
 *	a caller sets LITERAL_ON_HEAP if "string" is an already heap-allocated
 *	buffer holding the result of backslash substitutions.
 *
 *----------------------------------------------------------------------
 */

int
TclRegisterLiteral(
    void *ePtr,		/* Points to the CompileEnv in whose object
				 * array an object is found or created. */
    register char *bytes,	/* Points to string for which to find or
				 * create an object in CompileEnv's object
				 * array. */
    int length,			/* Number of bytes in the string. If < 0, the
				 * string consists of all bytes up to the
				 * first null character. */
    int flags)			/* If LITERAL_ON_HEAP then the caller already
				 * malloc'd bytes and ownership is passed to
				 * this function. If LITERAL_CMD_NAME then
				 * the literal should not be shared accross
				 * namespaces. */
{
    CompileEnv *envPtr = ePtr;
    Interp *iPtr = envPtr->iPtr;
    LiteralTable *localTablePtr = &envPtr->localLitTable;
    LiteralEntry *globalPtr, *localPtr;
    Tcl_Obj *objPtr;
    unsigned hash;
    int localHash, objIndex, new;
    Namespace *nsPtr;

    if (length < 0) {
	length = (bytes ? strlen(bytes) : 0);
    }
    hash = HashString(bytes, length);

    /*
     * Is the literal already in the CompileEnv's local literal array? If so,
     * just return its index.
     */

    localHash = (hash & localTablePtr->mask);
    for (localPtr=localTablePtr->buckets[localHash] ; localPtr!=NULL;
	    localPtr = localPtr->nextPtr) {
	objPtr = localPtr->objPtr;
	if ((objPtr->length == length) && ((length == 0)
		|| ((objPtr->bytes[0] == bytes[0])
		&& (memcmp(objPtr->bytes, bytes, (unsigned) length) == 0)))) {
	    if ((flags & LITERAL_ON_HEAP)) {
		ckfree(bytes);
	    }
	    objIndex = (localPtr - envPtr->literalArrayPtr);
#ifdef TCL_COMPILE_DEBUG
	    TclVerifyLocalLiteralTable(envPtr);
#endif /*TCL_COMPILE_DEBUG*/

	    return objIndex;
	}
    }

    /*
     * The literal is new to this CompileEnv. If it is a command name, avoid
     * sharing it accross namespaces, and try not to share it with non-cmd
     * literals. Note that FQ command names can be shared, so that we register
     * the namespace as the interp's global NS.
     */

    if ((flags & LITERAL_CMD_NAME)) {
	if ((length >= 2) && (bytes[0] == ':') && (bytes[1] == ':')) {
	    nsPtr = iPtr->globalNsPtr;
	} else {
	    nsPtr = iPtr->varFramePtr->nsPtr;
	}
    } else {
	nsPtr = NULL;
    }

    /*
     * Is it in the interpreter's global literal table? If not, create it.
     */

    globalPtr = NULL;
    objPtr = TclCreateLiteral(iPtr, bytes, length, hash, &new, nsPtr, flags,
	    &globalPtr);
    objIndex = AddLocalLiteralEntry(envPtr, objPtr, localHash);

#ifdef TCL_COMPILE_DEBUG
    if (globalPtr != NULL && globalPtr->refCount < 1) {
	Tcl_Panic("%s: global literal \"%.*s\" had bad refCount %d",
		"TclRegisterLiteral", (length>60? 60 : length), bytes,
		globalPtr->refCount);
    }
    TclVerifyLocalLiteralTable(envPtr);
#endif /*TCL_COMPILE_DEBUG*/
    return objIndex;
}

#ifdef TCL_COMPILE_DEBUG
/*
 *----------------------------------------------------------------------
 *
 * LookupLiteralEntry --
 *
 *	Finds the LiteralEntry that corresponds to a literal Tcl object
 *	holding a literal.
 *
 * Results:
 *	Returns the matching LiteralEntry if found, otherwise NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static LiteralEntry *
LookupLiteralEntry(
    Tcl_Interp *interp,		/* Interpreter for which objPtr was created to
				 * hold a literal. */
    register Tcl_Obj *objPtr)	/* Points to a Tcl object holding a literal
				 * that was previously created by a call to
				 * TclRegisterLiteral. */
{
    Interp *iPtr = (Interp *) interp;
    LiteralTable *globalTablePtr = &iPtr->literalTable;
    register LiteralEntry *entryPtr;
    const char *bytes;
    int length, globalHash;

    bytes = TclGetStringFromObj(objPtr, &length);
    globalHash = (HashString(bytes, length) & globalTablePtr->mask);
    for (entryPtr=globalTablePtr->buckets[globalHash] ; entryPtr!=NULL;
	    entryPtr=entryPtr->nextPtr) {
	if (entryPtr->objPtr == objPtr) {
	    return entryPtr;
	}
    }
    return NULL;
}

#endif
/*
 *----------------------------------------------------------------------
 *
 * TclHideLiteral --
 *
 *	Remove a literal entry from the literal hash tables, leaving it in the
 *	literal array so existing references continue to function. This makes
 *	it possible to turn a shared literal into a private literal that
 *	cannot be shared.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the literal from the local hash table and decrements the
 *	global hash entry's reference count.
 *
 *----------------------------------------------------------------------
 */

void
TclHideLiteral(
    Tcl_Interp *interp,		/* Interpreter for which objPtr was created to
				 * hold a literal. */
    register CompileEnv *envPtr,/* Points to CompileEnv whose literal array
				 * contains the entry being hidden. */
    int index)			/* The index of the entry in the literal
				 * array. */
{
    LiteralEntry **nextPtrPtr, *entryPtr, *lPtr;
    LiteralTable *localTablePtr = &envPtr->localLitTable;
    int localHash, length;
    const char *bytes;
    Tcl_Obj *newObjPtr;

    lPtr = &envPtr->literalArrayPtr[index];

    /*
     * To avoid unwanted sharing we need to copy the object and remove it from
     * the local and global literal tables. It still has a slot in the literal
     * array so it can be referred to by byte codes, but it will not be
     * matched by literal searches.
     */

    newObjPtr = Tcl_DuplicateObj(lPtr->objPtr);
    Tcl_IncrRefCount(newObjPtr);
    TclReleaseLiteral(interp, lPtr->objPtr);
    lPtr->objPtr = newObjPtr;

    bytes = TclGetStringFromObj(newObjPtr, &length);
    localHash = (HashString(bytes, length) & localTablePtr->mask);
    nextPtrPtr = &localTablePtr->buckets[localHash];

    for (entryPtr=*nextPtrPtr ; entryPtr!=NULL ; entryPtr=*nextPtrPtr) {
	if (entryPtr == lPtr) {
	    *nextPtrPtr = lPtr->nextPtr;
	    lPtr->nextPtr = NULL;
	    localTablePtr->numEntries--;
	    break;
	}
	nextPtrPtr = &entryPtr->nextPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclAddLiteralObj --
 *
 *	Add a single literal object to the literal array. This function does
 *	not add the literal to the local or global literal tables. The caller
 *	is expected to add the entry to whatever tables are appropriate.
 *
 * Results:
 *	The index in the CompileEnv's literal array that references the
 *	literal. Stores the pointer to the new literal entry in the location
 *	referenced by the localPtrPtr argument.
 *
 * Side effects:
 *	Expands the literal array if necessary. Increments the refcount on the
 *	literal object.
 *
 *----------------------------------------------------------------------
 */

int
TclAddLiteralObj(
    register CompileEnv *envPtr,/* Points to CompileEnv in whose literal array
				 * the object is to be inserted. */
    Tcl_Obj *objPtr,		/* The object to insert into the array. */
    LiteralEntry **litPtrPtr)	/* The location where the pointer to the new
				 * literal entry should be stored. May be
				 * NULL. */
{
    register LiteralEntry *lPtr;
    int objIndex;

    if (envPtr->literalArrayNext >= envPtr->literalArrayEnd) {
	ExpandLocalLiteralArray(envPtr);
    }
    objIndex = envPtr->literalArrayNext;
    envPtr->literalArrayNext++;

    lPtr = &envPtr->literalArrayPtr[objIndex];
    lPtr->objPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    lPtr->refCount = -1;	/* i.e., unused */
    lPtr->nextPtr = NULL;

    if (litPtrPtr) {
	*litPtrPtr = lPtr;
    }

    return objIndex;
}

/*
 *----------------------------------------------------------------------
 *
 * AddLocalLiteralEntry --
 *
 *	Insert a new literal into a CompileEnv's local literal array.
 *
 * Results:
 *	The index in the CompileEnv's literal array that references the
 *	literal.
 *
 * Side effects:
 *	Expands the literal array if necessary. May rebuild the hash bucket
 *	array of the CompileEnv's literal array if it becomes too large.
 *
 *----------------------------------------------------------------------
 */

static int
AddLocalLiteralEntry(
    register CompileEnv *envPtr,/* Points to CompileEnv in whose literal array
				 * the object is to be inserted. */
    Tcl_Obj *objPtr,		/* The literal to add to the CompileEnv. */
    int localHash)		/* Hash value for the literal's string. */
{
    register LiteralTable *localTablePtr = &envPtr->localLitTable;
    LiteralEntry *localPtr;
    int objIndex;

    objIndex = TclAddLiteralObj(envPtr, objPtr, &localPtr);

    /*
     * Add the literal to the local table.
     */

    localPtr->nextPtr = localTablePtr->buckets[localHash];
    localTablePtr->buckets[localHash] = localPtr;
    localTablePtr->numEntries++;

    /*
     * If the CompileEnv's local literal table has exceeded a decent size,
     * rebuild it with more buckets.
     */

    if (localTablePtr->numEntries >= localTablePtr->rebuildSize) {
	RebuildLiteralTable(localTablePtr);
    }

#ifdef TCL_COMPILE_DEBUG
    TclVerifyLocalLiteralTable(envPtr);
    {
	char *bytes;
	int length, found, i;

	found = 0;
	for (i=0 ; i<localTablePtr->numBuckets ; i++) {
	    for (localPtr=localTablePtr->buckets[i] ; localPtr!=NULL ;
		    localPtr=localPtr->nextPtr) {
		if (localPtr->objPtr == objPtr) {
		    found = 1;
		}
	    }
	}

	if (!found) {
	    bytes = Tcl_GetStringFromObj(objPtr, &length);
	    Tcl_Panic("%s: literal \"%.*s\" wasn't found locally",
		    "AddLocalLiteralEntry", (length>60? 60 : length), bytes);
	}
    }
#endif /*TCL_COMPILE_DEBUG*/

    return objIndex;
}

/*
 *----------------------------------------------------------------------
 *
 * ExpandLocalLiteralArray --
 *
 *	Function that uses malloc to allocate more storage for a CompileEnv's
 *	local literal array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The literal array in *envPtr is reallocated to a new array of double
 *	the size, and if envPtr->mallocedLiteralArray is non-zero the old
 *	array is freed. Entries are copied from the old array to the new one.
 *	The local literal table is updated to refer to the new entries.
 *
 *----------------------------------------------------------------------
 */

static void
ExpandLocalLiteralArray(
    register CompileEnv *envPtr)/* Points to the CompileEnv whose object array
				 * must be enlarged. */
{
    /*
     * The current allocated local literal entries are stored between elements
     * 0 and (envPtr->literalArrayNext - 1) [inclusive].
     */

    LiteralTable *localTablePtr = &envPtr->localLitTable;
    int currElems = envPtr->literalArrayNext;
    size_t currBytes = (currElems * sizeof(LiteralEntry));
    LiteralEntry *currArrayPtr = envPtr->literalArrayPtr;
    LiteralEntry *newArrayPtr;
    int i;
    unsigned int newSize = (currBytes <= UINT_MAX / 2) ? 2*currBytes : UINT_MAX;

    if (currBytes == newSize) {
	Tcl_Panic("max size of Tcl literal array (%d literals) exceeded",
		currElems);
    }

    if (envPtr->mallocedLiteralArray) {
	newArrayPtr = ckrealloc(currArrayPtr, newSize);
    } else {
	/*
	 * envPtr->literalArrayPtr isn't a ckalloc'd pointer, so we must
	 * code a ckrealloc equivalent for ourselves.
	 */

	newArrayPtr = ckalloc(newSize);
	memcpy(newArrayPtr, currArrayPtr, currBytes);
	envPtr->mallocedLiteralArray = 1;
    }

    /*
     * Update the local literal table's bucket array.
     */

    if (currArrayPtr != newArrayPtr) {
	for (i=0 ; i<currElems ; i++) {
	    if (newArrayPtr[i].nextPtr != NULL) {
		newArrayPtr[i].nextPtr = newArrayPtr
			+ (newArrayPtr[i].nextPtr - currArrayPtr);
	    }
	}
	for (i=0 ; i<localTablePtr->numBuckets ; i++) {
	    if (localTablePtr->buckets[i] != NULL) {
		localTablePtr->buckets[i] = newArrayPtr
			+ (localTablePtr->buckets[i] - currArrayPtr);
	    }
	}
    }

    envPtr->literalArrayPtr = newArrayPtr;
    envPtr->literalArrayEnd = newSize / sizeof(LiteralEntry);
}

/*
 *----------------------------------------------------------------------
 *
 * TclReleaseLiteral --
 *
 *	This function releases a reference to one of the shared Tcl objects
 *	that hold literals. It is called to release the literals referenced by
 *	a ByteCode that is being destroyed, and it is also called by
 *	TclDeleteLiteralTable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count for the global LiteralTable entry that corresponds
 *	to the literal is decremented. If no other reference to a global
 *	literal object remains, it is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclReleaseLiteral(
    Tcl_Interp *interp,		/* Interpreter for which objPtr was created to
				 * hold a literal. */
    register Tcl_Obj *objPtr)	/* Points to a literal object that was
				 * previously created by a call to
				 * TclRegisterLiteral. */
{
    Interp *iPtr = (Interp *) interp;
    LiteralTable *globalTablePtr;
    register LiteralEntry *entryPtr, *prevPtr;
    const char *bytes;
    int length, index;

    if (iPtr == NULL) {
	goto done;
    }

    globalTablePtr = &iPtr->literalTable;
    bytes = TclGetStringFromObj(objPtr, &length);
    index = (HashString(bytes, length) & globalTablePtr->mask);

    /*
     * Check to see if the object is in the global literal table and remove
     * this reference. The object may not be in the table if it is a hidden
     * local literal.
     */

    for (prevPtr=NULL, entryPtr=globalTablePtr->buckets[index];
	    entryPtr!=NULL ; prevPtr=entryPtr, entryPtr=entryPtr->nextPtr) {
	if (entryPtr->objPtr == objPtr) {
	    entryPtr->refCount--;

	    /*
	     * If the literal is no longer being used by any ByteCode, delete
	     * the entry then remove the reference corresponding to the global
	     * literal table entry (decrement the ref count of the object).
	     */

	    if (entryPtr->refCount == 0) {
		if (prevPtr == NULL) {
		    globalTablePtr->buckets[index] = entryPtr->nextPtr;
		} else {
		    prevPtr->nextPtr = entryPtr->nextPtr;
		}
		ckfree(entryPtr);
		globalTablePtr->numEntries--;

		TclDecrRefCount(objPtr);

#ifdef TCL_COMPILE_STATS
		iPtr->stats.currentLitStringBytes -= (double) (length + 1);
#endif /*TCL_COMPILE_STATS*/
	    }
	    break;
	}
    }

    /*
     * Remove the reference corresponding to the local literal table entry.
     */

    done:
    Tcl_DecrRefCount(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * HashString --
 *
 *	Compute a one-word summary of a text string, which can be used to
 *	generate a hash index.
 *
 * Results:
 *	The return value is a one-word summary of the information in string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned
HashString(
    register const char *string,	/* String for which to compute hash value. */
    int length)			/* Number of bytes in the string. */
{
    register unsigned int result = 0;

    /*
     * I tried a zillion different hash functions and asked many other people
     * for advice. Many people had their own favorite functions, all
     * different, but no-one had much idea why they were good ones. I chose
     * the one below (multiply by 9 and add new character) because of the
     * following reasons:
     *
     * 1. Multiplying by 10 is perfect for keys that are decimal strings, and
     *    multiplying by 9 is just about as good.
     * 2. Times-9 is (shift-left-3) plus (old). This means that each
     *    character's bits hang around in the low-order bits of the hash value
     *    for ever, plus they spread fairly rapidly up to the high-order bits
     *    to fill out the hash value. This seems works well both for decimal
     *    and non-decimal strings.
     *
     * Note that this function is very weak against malicious strings; it's
     * very easy to generate multiple keys that have the same hashcode. On the
     * other hand, that hardly ever actually occurs and this function *is*
     * very cheap, even by comparison with industry-standard hashes like FNV.
     * If real strength of hash is required though, use a custom hash based on
     * Bob Jenkins's lookup3(), but be aware that it's significantly slower.
     * Tcl scripts tend to not have a big issue in this area, and literals
     * mostly aren't looked up by name anyway.
     *
     * See also HashStringKey in tclHash.c.
     * See also TclObjHashKey in tclObj.c.
     *
     * See [tcl-Feature Request #2958832]
     */

    if (length > 0) {
	result = UCHAR(*string);
	while (--length) {
	    result += (result << 3) + UCHAR(*++string);
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * RebuildLiteralTable --
 *
 *	This function is invoked when the ratio of entries to hash buckets
 *	becomes too large in a local or global literal table. It allocates a
 *	larger bucket array and moves the entries into the new buckets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get rehashed into new buckets.
 *
 *----------------------------------------------------------------------
 */

static void
RebuildLiteralTable(
    register LiteralTable *tablePtr)
				/* Local or global table to enlarge. */
{
    LiteralEntry **oldBuckets;
    register LiteralEntry **oldChainPtr, **newChainPtr;
    register LiteralEntry *entryPtr;
    LiteralEntry **bucketPtr;
    const char *bytes;
    unsigned int oldSize;
    int count, index, length;

    oldSize = tablePtr->numBuckets;
    oldBuckets = tablePtr->buckets;

    /*
     * Allocate and initialize the new bucket array, and set up hashing
     * constants for new array size.
     */

    if (oldSize > UINT_MAX/(4 * sizeof(LiteralEntry *))) {
	/*
	 * Memory allocator limitations will not let us create the
	 * next larger table size.  Best option is to limp along
	 * with what we have.
	 */

	return;
    }

    tablePtr->numBuckets *= 4;
    tablePtr->buckets = ckalloc(tablePtr->numBuckets * sizeof(LiteralEntry*));
    for (count=tablePtr->numBuckets, newChainPtr=tablePtr->buckets;
	    count>0 ; count--, newChainPtr++) {
	*newChainPtr = NULL;
    }
    tablePtr->rebuildSize *= 4;
    tablePtr->mask = (tablePtr->mask << 2) + 3;

    /*
     * Rehash all of the existing entries into the new bucket array.
     */

    for (oldChainPtr=oldBuckets ; oldSize>0 ; oldSize--,oldChainPtr++) {
	for (entryPtr=*oldChainPtr ; entryPtr!=NULL ; entryPtr=*oldChainPtr) {
	    bytes = TclGetStringFromObj(entryPtr->objPtr, &length);
	    index = (HashString(bytes, length) & tablePtr->mask);

	    *oldChainPtr = entryPtr->nextPtr;
	    bucketPtr = &tablePtr->buckets[index];
	    entryPtr->nextPtr = *bucketPtr;
	    *bucketPtr = entryPtr;
	}
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */

    if (oldBuckets != tablePtr->staticBuckets) {
	ckfree(oldBuckets);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclInvalidateCmdLiteral --
 *
 *	Invalidate a command literal entry, if present in the literal hash
 *	tables, by resetting its internal representation. This invalidation
 *	leaves it in the literal tables and in existing literal arrays. As a
 *	result, existing references continue to work but we force a fresh
 *	command look-up upon the next use (see, in particular,
 *	TclSetCmdNameObj()).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the internal representation of the CmdName Tcl_Obj
 *	using TclFreeIntRep().
 *
 *----------------------------------------------------------------------
 */

void
TclInvalidateCmdLiteral(
    Tcl_Interp *interp,		/* Interpreter for which to invalidate a
				 * command literal. */
    const char *name,		/* Points to the start of the cmd literal
				 * name. */
    Namespace *nsPtr)		/* The namespace for which to lookup and
				 * invalidate a cmd literal. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *literalObjPtr = TclCreateLiteral(iPtr, (char *) name,
	    strlen(name), -1, NULL, nsPtr, 0, NULL);

    if (literalObjPtr != NULL) {
	if (literalObjPtr->typePtr == &tclCmdNameType) {
	    TclFreeIntRep(literalObjPtr);
	}
	/* Balance the refcount effects of TclCreateLiteral() above */
	Tcl_IncrRefCount(literalObjPtr);
	TclReleaseLiteral(interp, literalObjPtr);
    }
}

#ifdef TCL_COMPILE_STATS
/*
 *----------------------------------------------------------------------
 *
 * TclLiteralStats --
 *
 *	Return statistics describing the layout of the hash table in its hash
 *	buckets.
 *
 * Results:
 *	The return value is a malloc-ed string containing information about
 *	tablePtr. It is the caller's responsibility to free this string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TclLiteralStats(
    LiteralTable *tablePtr)	/* Table for which to produce stats. */
{
#define NUM_COUNTERS 10
    int count[NUM_COUNTERS], overflow, i, j;
    double average, tmp;
    register LiteralEntry *entryPtr;
    char *result, *p;

    /*
     * Compute a histogram of bucket usage. For each bucket chain i, j is the
     * number of entries in the chain.
     */

    for (i=0 ; i<NUM_COUNTERS ; i++) {
	count[i] = 0;
    }
    overflow = 0;
    average = 0.0;
    for (i=0 ; i<tablePtr->numBuckets ; i++) {
	j = 0;
	for (entryPtr=tablePtr->buckets[i] ; entryPtr!=NULL;
		entryPtr=entryPtr->nextPtr) {
	    j++;
	}
	if (j < NUM_COUNTERS) {
	    count[j]++;
	} else {
	    overflow++;
	}
	tmp = j;
	average += (tmp+1.0)*(tmp/tablePtr->numEntries)/2.0;
    }

    /*
     * Print out the histogram and a few other pieces of information.
     */

    result = ckalloc(NUM_COUNTERS*60 + 300);
    sprintf(result, "%d entries in table, %d buckets\n",
	    tablePtr->numEntries, tablePtr->numBuckets);
    p = result + strlen(result);
    for (i=0 ; i<NUM_COUNTERS ; i++) {
	sprintf(p, "number of buckets with %d entries: %d\n",
		i, count[i]);
	p += strlen(p);
    }
    sprintf(p, "number of buckets with %d or more entries: %d\n",
	    NUM_COUNTERS, overflow);
    p += strlen(p);
    sprintf(p, "average search distance for entry: %.1f", average);
    return result;
}
#endif /*TCL_COMPILE_STATS*/

#ifdef TCL_COMPILE_DEBUG
/*
 *----------------------------------------------------------------------
 *
 * TclVerifyLocalLiteralTable --
 *
 *	Check a CompileEnv's local literal table for consistency.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tcl_Panic if problems are found.
 *
 *----------------------------------------------------------------------
 */

void
TclVerifyLocalLiteralTable(
    CompileEnv *envPtr)		/* Points to CompileEnv whose literal table is
				 * to be validated. */
{
    register LiteralTable *localTablePtr = &envPtr->localLitTable;
    register LiteralEntry *localPtr;
    char *bytes;
    register int i;
    int length, count;

    count = 0;
    for (i=0 ; i<localTablePtr->numBuckets ; i++) {
	for (localPtr=localTablePtr->buckets[i] ; localPtr!=NULL;
		localPtr=localPtr->nextPtr) {
	    count++;
	    if (localPtr->refCount != -1) {
		bytes = Tcl_GetStringFromObj(localPtr->objPtr, &length);
		Tcl_Panic("%s: local literal \"%.*s\" had bad refCount %d",
			"TclVerifyLocalLiteralTable",
			(length>60? 60 : length), bytes, localPtr->refCount);
	    }
	    if (localPtr->objPtr->bytes == NULL) {
		Tcl_Panic("%s: literal has NULL string rep",
			"TclVerifyLocalLiteralTable");
	    }
	}
    }
    if (count != localTablePtr->numEntries) {
	Tcl_Panic("%s: local literal table had %d entries, should be %d",
		"TclVerifyLocalLiteralTable", count,
		localTablePtr->numEntries);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclVerifyGlobalLiteralTable --
 *
 *	Check an interpreter's global literal table literal for consistency.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tcl_Panic if problems are found.
 *
 *----------------------------------------------------------------------
 */

void
TclVerifyGlobalLiteralTable(
    Interp *iPtr)		/* Points to interpreter whose global literal
				 * table is to be validated. */
{
    register LiteralTable *globalTablePtr = &iPtr->literalTable;
    register LiteralEntry *globalPtr;
    char *bytes;
    register int i;
    int length, count;

    count = 0;
    for (i=0 ; i<globalTablePtr->numBuckets ; i++) {
	for (globalPtr=globalTablePtr->buckets[i] ; globalPtr!=NULL;
		globalPtr=globalPtr->nextPtr) {
	    count++;
	    if (globalPtr->refCount < 1) {
		bytes = Tcl_GetStringFromObj(globalPtr->objPtr, &length);
		Tcl_Panic("%s: global literal \"%.*s\" had bad refCount %d",
			"TclVerifyGlobalLiteralTable",
			(length>60? 60 : length), bytes, globalPtr->refCount);
	    }
	    if (globalPtr->objPtr->bytes == NULL) {
		Tcl_Panic("%s: literal has NULL string rep",
			"TclVerifyGlobalLiteralTable");
	    }
	}
    }
    if (count != globalTablePtr->numEntries) {
	Tcl_Panic("%s: global literal table had %d entries, should be %d",
		"TclVerifyGlobalLiteralTable", count,
		globalTablePtr->numEntries);
    }
}
#endif /*TCL_COMPILE_DEBUG*/

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
