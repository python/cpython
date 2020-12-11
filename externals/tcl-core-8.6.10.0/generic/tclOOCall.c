/*
 * tclOOCall.c --
 *
 *	This file contains the method call chain management code for the
 *	object-system core.
 *
 * Copyright (c) 2005-2012 by Donal K. Fellows
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "tclInt.h"
#include "tclOOInt.h"

/*
 * Structure containing a CallContext and any other values needed only during
 * the construction of the CallContext.
 */

struct ChainBuilder {
    CallChain *callChainPtr;	/* The call chain being built. */
    int filterLength;		/* Number of entries in the call chain that
				 * are due to processing filters and not the
				 * main call chain. */
    Object *oPtr;		/* The object that we are building the chain
				 * for. */
};

/*
 * Extra flags used for call chain management.
 */

#define DEFINITE_PROTECTED 0x100000
#define DEFINITE_PUBLIC    0x200000
#define KNOWN_STATE	   (DEFINITE_PROTECTED | DEFINITE_PUBLIC)
#define SPECIAL		   (CONSTRUCTOR | DESTRUCTOR | FORCE_UNKNOWN)
#define BUILDING_MIXINS	   0x400000
#define TRAVERSED_MIXIN	   0x800000
#define OBJECT_MIXIN	   0x1000000
#define MIXIN_CONSISTENT(flags) \
    (((flags) & OBJECT_MIXIN) ||					\
	!((flags) & BUILDING_MIXINS) == !((flags) & TRAVERSED_MIXIN))

/*
 * Function declarations for things defined in this file.
 */

static void		AddClassFiltersToCallContext(Object *const oPtr,
			    Class *clsPtr, struct ChainBuilder *const cbPtr,
			    Tcl_HashTable *const doneFilters, int flags);
static void		AddClassMethodNames(Class *clsPtr, const int flags,
			    Tcl_HashTable *const namesPtr,
			    Tcl_HashTable *const examinedClassesPtr);
static inline void	AddMethodToCallChain(Method *const mPtr,
			    struct ChainBuilder *const cbPtr,
			    Tcl_HashTable *const doneFilters,
			    Class *const filterDecl, int flags);
static inline void	AddSimpleChainToCallContext(Object *const oPtr,
			    Tcl_Obj *const methodNameObj,
			    struct ChainBuilder *const cbPtr,
			    Tcl_HashTable *const doneFilters, int flags,
			    Class *const filterDecl);
static void		AddSimpleClassChainToCallContext(Class *classPtr,
			    Tcl_Obj *const methodNameObj,
			    struct ChainBuilder *const cbPtr,
			    Tcl_HashTable *const doneFilters, int flags,
			    Class *const filterDecl);
static int		CmpStr(const void *ptr1, const void *ptr2);
static void		DupMethodNameRep(Tcl_Obj *srcPtr, Tcl_Obj *dstPtr);
static Tcl_NRPostProc	FinalizeMethodRefs;
static void		FreeMethodNameRep(Tcl_Obj *objPtr);
static inline int	IsStillValid(CallChain *callPtr, Object *oPtr,
			    int flags, int reuseMask);
static Tcl_NRPostProc	ResetFilterFlags;
static Tcl_NRPostProc	SetFilterFlags;
static inline void	StashCallChain(Tcl_Obj *objPtr, CallChain *callPtr);

/*
 * Object type used to manage type caches attached to method names.
 */

static const Tcl_ObjType methodNameType = {
    "TclOO method name",
    FreeMethodNameRep,
    DupMethodNameRep,
    NULL,
    NULL
};

/*
 * ----------------------------------------------------------------------
 *
 * TclOODeleteContext --
 *
 *	Destroys a method call-chain context, which should not be in use.
 *
 * ----------------------------------------------------------------------
 */

void
TclOODeleteContext(
    CallContext *contextPtr)
{
    register Object *oPtr = contextPtr->oPtr;

    TclOODeleteChain(contextPtr->callPtr);
    if (oPtr != NULL) {
	TclStackFree(oPtr->fPtr->interp, contextPtr);

	/*
	 * Corresponding AddRef() in TclOO.c/TclOOObjectCmdCore
	 */

	TclOODecrRefCount(oPtr);
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODeleteChainCache --
 *
 *	Destroy the cache of method call-chains.
 *
 * ----------------------------------------------------------------------
 */

void
TclOODeleteChainCache(
    Tcl_HashTable *tablePtr)
{
    FOREACH_HASH_DECLS;
    CallChain *callPtr;

    FOREACH_HASH_VALUE(callPtr, tablePtr) {
	if (callPtr) {
	    TclOODeleteChain(callPtr);
	}
    }
    Tcl_DeleteHashTable(tablePtr);
    ckfree(tablePtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODeleteChain --
 *
 *	Destroys a method call-chain.
 *
 * ----------------------------------------------------------------------
 */

void
TclOODeleteChain(
    CallChain *callPtr)
{
    if (callPtr == NULL || callPtr->refCount-- > 1) {
	return;
    }
    if (callPtr->chain != callPtr->staticChain) {
	ckfree(callPtr->chain);
    }
    ckfree(callPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOStashContext --
 *
 *	Saves a reference to a method call context in a Tcl_Obj's internal
 *	representation.
 *
 * ----------------------------------------------------------------------
 */

static inline void
StashCallChain(
    Tcl_Obj *objPtr,
    CallChain *callPtr)
{
    callPtr->refCount++;
    TclGetString(objPtr);
    TclFreeIntRep(objPtr);
    objPtr->typePtr = &methodNameType;
    objPtr->internalRep.twoPtrValue.ptr1 = callPtr;
}

void
TclOOStashContext(
    Tcl_Obj *objPtr,
    CallContext *contextPtr)
{
    StashCallChain(objPtr, contextPtr->callPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * DupMethodNameRep, FreeMethodNameRep --
 *
 *	Functions to implement the required parts of the Tcl_Obj guts needed
 *	for caching of method contexts in Tcl_Objs.
 *
 * ----------------------------------------------------------------------
 */

static void
DupMethodNameRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dstPtr)
{
    register CallChain *callPtr = srcPtr->internalRep.twoPtrValue.ptr1;

    dstPtr->typePtr = &methodNameType;
    dstPtr->internalRep.twoPtrValue.ptr1 = callPtr;
    callPtr->refCount++;
}

static void
FreeMethodNameRep(
    Tcl_Obj *objPtr)
{
    register CallChain *callPtr = objPtr->internalRep.twoPtrValue.ptr1;

    TclOODeleteChain(callPtr);
    objPtr->typePtr = NULL;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOInvokeContext --
 *
 *	Invokes a single step along a method call-chain context. Note that the
 *	invocation of a step along the chain can cause further steps along the
 *	chain to be invoked. Note that this function is written to be as light
 *	in stack usage as possible.
 *
 * ----------------------------------------------------------------------
 */

int
TclOOInvokeContext(
    ClientData clientData,	/* The method call context. */
    Tcl_Interp *interp,		/* Interpreter for error reporting, and many
				 * other sorts of context handling (e.g.,
				 * commands, variables) depending on method
				 * implementation. */
    int objc,			/* The number of arguments. */
    Tcl_Obj *const objv[])	/* The arguments as actually seen. */
{
    register CallContext *const contextPtr = clientData;
    Method *const mPtr = contextPtr->callPtr->chain[contextPtr->index].mPtr;
    const int isFilter =
	    contextPtr->callPtr->chain[contextPtr->index].isFilter;

    /*
     * If this is the first step along the chain, we preserve the method
     * entries in the chain so that they do not get deleted out from under our
     * feet.
     */

    if (contextPtr->index == 0) {
	int i;

	for (i = 0 ; i < contextPtr->callPtr->numChain ; i++) {
	    AddRef(contextPtr->callPtr->chain[i].mPtr);
	}

	/*
	 * Ensure that the method name itself is part of the arguments when
	 * we're doing unknown processing.
	 */

	if (contextPtr->callPtr->flags & OO_UNKNOWN_METHOD) {
	    contextPtr->skip--;
	}

	/*
	 * Add a callback to ensure that method references are dropped once
	 * this call is finished.
	 */

	TclNRAddCallback(interp, FinalizeMethodRefs, contextPtr, NULL, NULL,
		NULL);
    }

    /*
     * Save whether we were in a filter and set up whether we are now.
     */

    if (contextPtr->oPtr->flags & FILTER_HANDLING) {
	TclNRAddCallback(interp, SetFilterFlags, contextPtr, NULL,NULL,NULL);
    } else {
	TclNRAddCallback(interp, ResetFilterFlags,contextPtr,NULL,NULL,NULL);
    }
    if (isFilter || contextPtr->callPtr->flags & FILTER_HANDLING) {
	contextPtr->oPtr->flags |= FILTER_HANDLING;
    } else {
	contextPtr->oPtr->flags &= ~FILTER_HANDLING;
    }

    /*
     * Run the method implementation.
     */

    return mPtr->typePtr->callProc(mPtr->clientData, interp,
	    (Tcl_ObjectContext) contextPtr, objc, objv);
}

static int
SetFilterFlags(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CallContext *contextPtr = data[0];

    contextPtr->oPtr->flags |= FILTER_HANDLING;
    return result;
}

static int
ResetFilterFlags(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CallContext *contextPtr = data[0];

    contextPtr->oPtr->flags &= ~FILTER_HANDLING;
    return result;
}

static int
FinalizeMethodRefs(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CallContext *contextPtr = data[0];
    int i;

    for (i = 0 ; i < contextPtr->callPtr->numChain ; i++) {
	TclOODelMethodRef(contextPtr->callPtr->chain[i].mPtr);
    }
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOGetSortedMethodList, TclOOGetSortedClassMethodList --
 *
 *	Discovers the list of method names supported by an object or class.
 *
 * ----------------------------------------------------------------------
 */

int
TclOOGetSortedMethodList(
    Object *oPtr,		/* The object to get the method names for. */
    int flags,			/* Whether we just want the public method
				 * names. */
    const char ***stringsPtr)	/* Where to write a pointer to the array of
				 * strings to. */
{
    Tcl_HashTable names;	/* Tcl_Obj* method name to "wanted in list"
				 * mapping. */
    Tcl_HashTable examinedClasses;
				/* Used to track what classes have been looked
				 * at. Is set-like in nature and keyed by
				 * pointer to class. */
    FOREACH_HASH_DECLS;
    int i;
    Class *mixinPtr;
    Tcl_Obj *namePtr;
    Method *mPtr;
    int isWantedIn;
    void *isWanted;

    Tcl_InitObjHashTable(&names);
    Tcl_InitHashTable(&examinedClasses, TCL_ONE_WORD_KEYS);

    /*
     * Name the bits used in the names table values.
     */
#define IN_LIST 1
#define NO_IMPLEMENTATION 2

    /*
     * Process method names due to the object.
     */

    if (oPtr->methodsPtr) {
	FOREACH_HASH(namePtr, mPtr, oPtr->methodsPtr) {
	    int isNew;

	    if ((mPtr->flags & PRIVATE_METHOD) && !(flags & PRIVATE_METHOD)) {
		continue;
	    }
	    hPtr = Tcl_CreateHashEntry(&names, (char *) namePtr, &isNew);
	    if (isNew) {
		isWantedIn = ((!(flags & PUBLIC_METHOD)
			|| mPtr->flags & PUBLIC_METHOD) ? IN_LIST : 0);
		isWantedIn |= (mPtr->typePtr == NULL ? NO_IMPLEMENTATION : 0);
		Tcl_SetHashValue(hPtr, INT2PTR(isWantedIn));
	    }
	}
    }

    /*
     * Process method names due to private methods on the object's class.
     */

    if (flags & PRIVATE_METHOD) {
	FOREACH_HASH(namePtr, mPtr, &oPtr->selfCls->classMethods) {
	    if (mPtr->flags & PRIVATE_METHOD) {
		int isNew;

		hPtr = Tcl_CreateHashEntry(&names, (char *) namePtr, &isNew);
		if (isNew) {
		    isWantedIn = IN_LIST;
		    if (mPtr->typePtr == NULL) {
			isWantedIn |= NO_IMPLEMENTATION;
		    }
		    Tcl_SetHashValue(hPtr, INT2PTR(isWantedIn));
		} else if (mPtr->typePtr != NULL) {
		    isWantedIn = PTR2INT(Tcl_GetHashValue(hPtr));
		    if (isWantedIn & NO_IMPLEMENTATION) {
			isWantedIn &= ~NO_IMPLEMENTATION;
			Tcl_SetHashValue(hPtr, INT2PTR(isWantedIn));
		    }
		}
	    }
	}
    }

    /*
     * Process (normal) method names from the class hierarchy and the mixin
     * hierarchy.
     */

    AddClassMethodNames(oPtr->selfCls, flags, &names, &examinedClasses);
    FOREACH(mixinPtr, oPtr->mixins) {
	AddClassMethodNames(mixinPtr, flags|TRAVERSED_MIXIN, &names,
		&examinedClasses);
    }

    Tcl_DeleteHashTable(&examinedClasses);

    /*
     * See how many (visible) method names there are. If none, we do not (and
     * should not) try to sort the list of them.
     */

    i = 0;
    if (names.numEntries != 0) {
	const char **strings;

	/*
	 * We need to build the list of methods to sort. We will be using
	 * qsort() for this, because it is very unlikely that the list will be
	 * heavily sorted when it is long enough to matter.
	 */

	strings = ckalloc(sizeof(char *) * names.numEntries);
	FOREACH_HASH(namePtr, isWanted, &names) {
	    if (!(flags & PUBLIC_METHOD) || (PTR2INT(isWanted) & IN_LIST)) {
		if (PTR2INT(isWanted) & NO_IMPLEMENTATION) {
		    continue;
		}
		strings[i++] = TclGetString(namePtr);
	    }
	}

	/*
	 * Note that 'i' may well be less than names.numEntries when we are
	 * dealing with public method names.
	 */

	if (i > 0) {
	    if (i > 1) {
		qsort((void *) strings, (unsigned) i, sizeof(char *), CmpStr);
	    }
	    *stringsPtr = strings;
	} else {
	    ckfree(strings);
	}
    }

    Tcl_DeleteHashTable(&names);
    return i;
}

int
TclOOGetSortedClassMethodList(
    Class *clsPtr,		/* The class to get the method names for. */
    int flags,			/* Whether we just want the public method
				 * names. */
    const char ***stringsPtr)	/* Where to write a pointer to the array of
				 * strings to. */
{
    Tcl_HashTable names;	/* Tcl_Obj* method name to "wanted in list"
				 * mapping. */
    Tcl_HashTable examinedClasses;
				/* Used to track what classes have been looked
				 * at. Is set-like in nature and keyed by
				 * pointer to class. */
    FOREACH_HASH_DECLS;
    int i;
    Tcl_Obj *namePtr;
    void *isWanted;

    Tcl_InitObjHashTable(&names);
    Tcl_InitHashTable(&examinedClasses, TCL_ONE_WORD_KEYS);

    /*
     * Process method names from the class hierarchy and the mixin hierarchy.
     */

    AddClassMethodNames(clsPtr, flags, &names, &examinedClasses);
    Tcl_DeleteHashTable(&examinedClasses);

    /*
     * See how many (visible) method names there are. If none, we do not (and
     * should not) try to sort the list of them.
     */

    i = 0;
    if (names.numEntries != 0) {
	const char **strings;

	/*
	 * We need to build the list of methods to sort. We will be using
	 * qsort() for this, because it is very unlikely that the list will be
	 * heavily sorted when it is long enough to matter.
	 */

	strings = ckalloc(sizeof(char *) * names.numEntries);
	FOREACH_HASH(namePtr, isWanted, &names) {
	    if (!(flags & PUBLIC_METHOD) || (PTR2INT(isWanted) & IN_LIST)) {
		if (PTR2INT(isWanted) & NO_IMPLEMENTATION) {
		    continue;
		}
		strings[i++] = TclGetString(namePtr);
	    }
	}

	/*
	 * Note that 'i' may well be less than names.numEntries when we are
	 * dealing with public method names.
	 */

	if (i > 0) {
	    if (i > 1) {
		qsort((void *) strings, (unsigned) i, sizeof(char *), CmpStr);
	    }
	    *stringsPtr = strings;
	} else {
	    ckfree(strings);
	}
    }

    Tcl_DeleteHashTable(&names);
    return i;
}

/*
 * Comparator for GetSortedMethodList
 */

static int
CmpStr(
    const void *ptr1,
    const void *ptr2)
{
    const char **strPtr1 = (const char **) ptr1;
    const char **strPtr2 = (const char **) ptr2;

    return TclpUtfNcmp2(*strPtr1, *strPtr2, strlen(*strPtr1) + 1);
}

/*
 * ----------------------------------------------------------------------
 *
 * AddClassMethodNames --
 *
 *	Adds the method names defined by a class (or its superclasses) to the
 *	collection being built. The collection is built in a hash table to
 *	ensure that duplicates are excluded. Helper for GetSortedMethodList().
 *
 * ----------------------------------------------------------------------
 */

static void
AddClassMethodNames(
    Class *clsPtr,		/* Class to get method names from. */
    const int flags,		/* Whether we are interested in just the
				 * public method names. */
    Tcl_HashTable *const namesPtr,
				/* Reference to the hash table to put the
				 * information in. The hash table maps the
				 * Tcl_Obj * method name to an integral value
				 * describing whether the method is wanted.
				 * This ensures that public/private override
				 * semantics are handled correctly. */
    Tcl_HashTable *const examinedClassesPtr)
				/* Hash table that tracks what classes have
				 * already been looked at. The keys are the
				 * pointers to the classes, and the values are
				 * immaterial. */
{
    /*
     * If we've already started looking at this class, stop working on it now
     * to prevent repeated work.
     */

    if (Tcl_FindHashEntry(examinedClassesPtr, (char *) clsPtr)) {
	return;
    }

    /*
     * Scope all declarations so that the compiler can stand a good chance of
     * making the recursive step highly efficient. We also hand-implement the
     * tail-recursive case using a while loop; C compilers typically cannot do
     * tail-recursion optimization usefully.
     */

    while (1) {
	FOREACH_HASH_DECLS;
	Tcl_Obj *namePtr;
	Method *mPtr;
	int isNew;

	(void) Tcl_CreateHashEntry(examinedClassesPtr, (char *) clsPtr,
		&isNew);
	if (!isNew) {
	    break;
	}

	if (clsPtr->mixins.num != 0) {
	    Class *mixinPtr;
	    int i;

	    FOREACH(mixinPtr, clsPtr->mixins) {
		if (mixinPtr != clsPtr) {
		    AddClassMethodNames(mixinPtr, flags|TRAVERSED_MIXIN,
			    namesPtr, examinedClassesPtr);
		}
	    }
	}

	FOREACH_HASH(namePtr, mPtr, &clsPtr->classMethods) {
	    hPtr = Tcl_CreateHashEntry(namesPtr, (char *) namePtr, &isNew);
	    if (isNew) {
		int isWanted = (!(flags & PUBLIC_METHOD)
			|| (mPtr->flags & PUBLIC_METHOD)) ? IN_LIST : 0;

		isWanted |= (mPtr->typePtr == NULL ? NO_IMPLEMENTATION : 0);
		Tcl_SetHashValue(hPtr, INT2PTR(isWanted));
	    } else if ((PTR2INT(Tcl_GetHashValue(hPtr)) & NO_IMPLEMENTATION)
		    && mPtr->typePtr != NULL) {
		int isWanted = PTR2INT(Tcl_GetHashValue(hPtr));

		isWanted &= ~NO_IMPLEMENTATION;
		Tcl_SetHashValue(hPtr, INT2PTR(isWanted));
	    }
	}

	if (clsPtr->superclasses.num != 1) {
	    break;
	}
	clsPtr = clsPtr->superclasses.list[0];
    }
    if (clsPtr->superclasses.num != 0) {
	Class *superPtr;
	int i;

	FOREACH(superPtr, clsPtr->superclasses) {
	    AddClassMethodNames(superPtr, flags, namesPtr,
		    examinedClassesPtr);
	}
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * AddSimpleChainToCallContext --
 *
 *	The core of the call-chain construction engine, this handles calling a
 *	particular method on a particular object. Note that filters and
 *	unknown handling are already handled by the logic that uses this
 *	function.
 *
 * ----------------------------------------------------------------------
 */

static inline void
AddSimpleChainToCallContext(
    Object *const oPtr,		/* Object to add call chain entries for. */
    Tcl_Obj *const methodNameObj,
				/* Name of method to add the call chain
				 * entries for. */
    struct ChainBuilder *const cbPtr,
				/* Where to add the call chain entries. */
    Tcl_HashTable *const doneFilters,
				/* Where to record what call chain entries
				 * have been processed. */
    int flags,			/* What sort of call chain are we building. */
    Class *const filterDecl)	/* The class that declared the filter. If
				 * NULL, either the filter was declared by the
				 * object or this isn't a filter. */
{
    int i;

    if (!(flags & (KNOWN_STATE | SPECIAL)) && oPtr->methodsPtr) {
	Tcl_HashEntry *hPtr = Tcl_FindHashEntry(oPtr->methodsPtr,
		(char *) methodNameObj);

	if (hPtr != NULL) {
	    Method *mPtr = Tcl_GetHashValue(hPtr);

	    if (flags & PUBLIC_METHOD) {
		if (!(mPtr->flags & PUBLIC_METHOD)) {
		    return;
		} else {
		    flags |= DEFINITE_PUBLIC;
		}
	    } else {
		flags |= DEFINITE_PROTECTED;
	    }
	}
    }
    if (!(flags & SPECIAL)) {
	Tcl_HashEntry *hPtr;
	Class *mixinPtr;

	FOREACH(mixinPtr, oPtr->mixins) {
	    AddSimpleClassChainToCallContext(mixinPtr, methodNameObj, cbPtr,
		    doneFilters, flags|TRAVERSED_MIXIN, filterDecl);
	}
	if (oPtr->methodsPtr) {
	    hPtr = Tcl_FindHashEntry(oPtr->methodsPtr, (char*) methodNameObj);
	    if (hPtr != NULL) {
		AddMethodToCallChain(Tcl_GetHashValue(hPtr), cbPtr,
			doneFilters, filterDecl, flags);
	    }
	}
    }
    AddSimpleClassChainToCallContext(oPtr->selfCls, methodNameObj, cbPtr,
	    doneFilters, flags, filterDecl);
}

/*
 * ----------------------------------------------------------------------
 *
 * AddMethodToCallChain --
 *
 *	Utility method that manages the adding of a particular method
 *	implementation to a call-chain.
 *
 * ----------------------------------------------------------------------
 */

static inline void
AddMethodToCallChain(
    Method *const mPtr,		/* Actual method implementation to add to call
				 * chain (or NULL, a no-op). */
    struct ChainBuilder *const cbPtr,
				/* The call chain to add the method
				 * implementation to. */
    Tcl_HashTable *const doneFilters,
				/* Where to record what filters have been
				 * processed. If NULL, not processing filters.
				 * Note that this function does not update
				 * this hashtable. */
    Class *const filterDecl,	/* The class that declared the filter. If
				 * NULL, either the filter was declared by the
				 * object or this isn't a filter. */
    int flags)			/* Used to check if we're mixin-consistent
				 * only. Mixin-consistent means that either
				 * we're looking to add things from a mixin
				 * and we have passed a mixin, or we're not
				 * looking to add things from a mixin and have
				 * not passed a mixin. */
{
    register CallChain *callPtr = cbPtr->callChainPtr;
    int i;

    /*
     * Return if this is just an entry used to record whether this is a public
     * method. If so, there's nothing real to call and so nothing to add to
     * the call chain.
     *
     * This is also where we enforce mixin-consistency.
     */

    if (mPtr == NULL || mPtr->typePtr == NULL || !MIXIN_CONSISTENT(flags)) {
	return;
    }

    /*
     * Enforce real private method handling here. We will skip adding this
     * method IF
     *  1) we are not allowing private methods, AND
     *  2) this is a private method, AND
     *  3) this is a class method, AND
     *  4) this method was not declared by the class of the current object.
     *
     * This does mean that only classes really handle private methods. This
     * should be sufficient for [incr Tcl] support though.
     */

    if (!(callPtr->flags & PRIVATE_METHOD)
	    && (mPtr->flags & PRIVATE_METHOD)
	    && (mPtr->declaringClassPtr != NULL)
	    && (mPtr->declaringClassPtr != cbPtr->oPtr->selfCls)) {
	return;
    }

    /*
     * First test whether the method is already in the call chain. Skip over
     * any leading filters.
     */

    for (i = cbPtr->filterLength ; i < callPtr->numChain ; i++) {
	if (callPtr->chain[i].mPtr == mPtr &&
		callPtr->chain[i].isFilter == (doneFilters != NULL)) {
	    /*
	     * Call chain semantics states that methods come as *late* in the
	     * call chain as possible. This is done by copying down the
	     * following methods. Note that this does not change the number of
	     * method invocations in the call chain; it just rearranges them.
	     */

	    Class *declCls = callPtr->chain[i].filterDeclarer;

	    for (; i + 1 < callPtr->numChain ; i++) {
		callPtr->chain[i] = callPtr->chain[i + 1];
	    }
	    callPtr->chain[i].mPtr = mPtr;
	    callPtr->chain[i].isFilter = (doneFilters != NULL);
	    callPtr->chain[i].filterDeclarer = declCls;
	    return;
	}
    }

    /*
     * Need to really add the method. This is made a bit more complex by the
     * fact that we are using some "static" space initially, and only start
     * realloc-ing if the chain gets long.
     */

    if (callPtr->numChain == CALL_CHAIN_STATIC_SIZE) {
	callPtr->chain =
		ckalloc(sizeof(struct MInvoke) * (callPtr->numChain + 1));
	memcpy(callPtr->chain, callPtr->staticChain,
		sizeof(struct MInvoke) * callPtr->numChain);
    } else if (callPtr->numChain > CALL_CHAIN_STATIC_SIZE) {
	callPtr->chain = ckrealloc(callPtr->chain,
		sizeof(struct MInvoke) * (callPtr->numChain + 1));
    }
    callPtr->chain[i].mPtr = mPtr;
    callPtr->chain[i].isFilter = (doneFilters != NULL);
    callPtr->chain[i].filterDeclarer = filterDecl;
    callPtr->numChain++;
}

/*
 * ----------------------------------------------------------------------
 *
 * InitCallChain --
 *	Encoding of the policy of how to set up a call chain. Doesn't populate
 *	the chain with the method implementation data.
 *
 * ----------------------------------------------------------------------
 */

static inline void
InitCallChain(
    CallChain *callPtr,
    Object *oPtr,
    int flags)
{
    callPtr->flags = flags &
	    (PUBLIC_METHOD | PRIVATE_METHOD | SPECIAL | FILTER_HANDLING);
    if (oPtr->flags & USE_CLASS_CACHE) {
	oPtr = oPtr->selfCls->thisPtr;
	callPtr->flags |= USE_CLASS_CACHE;
    }
    callPtr->epoch = oPtr->fPtr->epoch;
    callPtr->objectCreationEpoch = oPtr->creationEpoch;
    callPtr->objectEpoch = oPtr->epoch;
    callPtr->refCount = 1;
    callPtr->numChain = 0;
    callPtr->chain = callPtr->staticChain;
}

/*
 * ----------------------------------------------------------------------
 *
 * IsStillValid --
 *	Calculates whether the given call chain can be used for executing a
 *	method for the given object. The condition on a chain from a cached
 *	location being reusable is:
 *	- Refers to the same object (same creation epoch), and
 *	- Still across the same class structure (same global epoch), and
 *	- Still across the same object strucutre (same local epoch), and
 *	- No public/private/filter magic leakage (same flags, modulo the fact
 *	  that a public chain will satisfy a non-public call).
 *
 * ----------------------------------------------------------------------
 */

static inline int
IsStillValid(
    CallChain *callPtr,
    Object *oPtr,
    int flags,
    int mask)
{
    if ((oPtr->flags & USE_CLASS_CACHE)) {
	oPtr = oPtr->selfCls->thisPtr;
	flags |= USE_CLASS_CACHE;
    }
    return ((callPtr->objectCreationEpoch == oPtr->creationEpoch)
	    && (callPtr->epoch == oPtr->fPtr->epoch)
	    && (callPtr->objectEpoch == oPtr->epoch)
	    && ((callPtr->flags & mask) == (flags & mask)));
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOGetCallContext --
 *
 *	Responsible for constructing the call context, an ordered list of all
 *	method implementations to be called as part of a method invocation.
 *	This method is central to the whole operation of the OO system.
 *
 * ----------------------------------------------------------------------
 */

CallContext *
TclOOGetCallContext(
    Object *oPtr,		/* The object to get the context for. */
    Tcl_Obj *methodNameObj,	/* The name of the method to get the context
				 * for. NULL when getting a constructor or
				 * destructor chain. */
    int flags,			/* What sort of context are we looking for.
				 * Only the bits PUBLIC_METHOD, CONSTRUCTOR,
				 * PRIVATE_METHOD, DESTRUCTOR and
				 * FILTER_HANDLING are useful. */
    Tcl_Obj *cacheInThisObj)	/* What object to cache in, or NULL if it is
				 * to be in the same object as the
				 * methodNameObj. */
{
    CallContext *contextPtr;
    CallChain *callPtr;
    struct ChainBuilder cb;
    int i, count, doFilters;
    Tcl_HashEntry *hPtr;
    Tcl_HashTable doneFilters;

    if (cacheInThisObj == NULL) {
	cacheInThisObj = methodNameObj;
    }
    if (flags&(SPECIAL|FILTER_HANDLING) || (oPtr->flags&FILTER_HANDLING)) {
	hPtr = NULL;
	doFilters = 0;

	/*
	 * Check if we have a cached valid constructor or destructor.
	 */

	if (flags & CONSTRUCTOR) {
	    callPtr = oPtr->selfCls->constructorChainPtr;
	    if ((callPtr != NULL)
		    && (callPtr->objectEpoch == oPtr->selfCls->thisPtr->epoch)
		    && (callPtr->epoch == oPtr->fPtr->epoch)) {
		callPtr->refCount++;
		goto returnContext;
	    }
	} else if (flags & DESTRUCTOR) {
	    callPtr = oPtr->selfCls->destructorChainPtr;
	    if ((oPtr->mixins.num == 0) && (callPtr != NULL)
		    && (callPtr->objectEpoch == oPtr->selfCls->thisPtr->epoch)
		    && (callPtr->epoch == oPtr->fPtr->epoch)) {
		callPtr->refCount++;
		goto returnContext;
	    }
	}
    } else {
	/*
	 * Check if we can get the chain out of the Tcl_Obj method name or out
	 * of the cache. This is made a bit more complex by the fact that
	 * there are multiple different layers of cache (in the Tcl_Obj, in
	 * the object, and in the class).
	 */

	const int reuseMask = ((flags & PUBLIC_METHOD) ? ~0 : ~PUBLIC_METHOD);

	if (cacheInThisObj->typePtr == &methodNameType) {
	    callPtr = cacheInThisObj->internalRep.twoPtrValue.ptr1;
	    if (IsStillValid(callPtr, oPtr, flags, reuseMask)) {
		callPtr->refCount++;
		goto returnContext;
	    }
	    FreeMethodNameRep(cacheInThisObj);
	}

	if (oPtr->flags & USE_CLASS_CACHE) {
	    if (oPtr->selfCls->classChainCache != NULL) {
		hPtr = Tcl_FindHashEntry(oPtr->selfCls->classChainCache,
			(char *) methodNameObj);
	    } else {
		hPtr = NULL;
	    }
	} else {
	    if (oPtr->chainCache != NULL) {
		hPtr = Tcl_FindHashEntry(oPtr->chainCache,
			(char *) methodNameObj);
	    } else {
		hPtr = NULL;
	    }
	}

	if (hPtr != NULL && Tcl_GetHashValue(hPtr) != NULL) {
	    callPtr = Tcl_GetHashValue(hPtr);
	    if (IsStillValid(callPtr, oPtr, flags, reuseMask)) {
		callPtr->refCount++;
		goto returnContext;
	    }
	    Tcl_SetHashValue(hPtr, NULL);
	    TclOODeleteChain(callPtr);
	}

	doFilters = 1;
    }

    callPtr = ckalloc(sizeof(CallChain));
    InitCallChain(callPtr, oPtr, flags);

    cb.callChainPtr = callPtr;
    cb.filterLength = 0;
    cb.oPtr = oPtr;

    /*
     * If we're working with a forced use of unknown, do that now.
     */

    if (flags & FORCE_UNKNOWN) {
	AddSimpleChainToCallContext(oPtr, oPtr->fPtr->unknownMethodNameObj,
		&cb, NULL, BUILDING_MIXINS, NULL);
	AddSimpleChainToCallContext(oPtr, oPtr->fPtr->unknownMethodNameObj,
		&cb, NULL, 0, NULL);
	callPtr->flags |= OO_UNKNOWN_METHOD;
	callPtr->epoch = -1;
	if (callPtr->numChain == 0) {
	    TclOODeleteChain(callPtr);
	    return NULL;
	}
	goto returnContext;
    }

    /*
     * Add all defined filters (if any, and if we're going to be processing
     * them; they're not processed for constructors, destructors or when we're
     * in the middle of processing a filter).
     */

    if (doFilters) {
	Tcl_Obj *filterObj;
	Class *mixinPtr;

	doFilters = 1;
	Tcl_InitObjHashTable(&doneFilters);
	FOREACH(mixinPtr, oPtr->mixins) {
	    AddClassFiltersToCallContext(oPtr, mixinPtr, &cb, &doneFilters,
		    TRAVERSED_MIXIN|BUILDING_MIXINS|OBJECT_MIXIN);
	    AddClassFiltersToCallContext(oPtr, mixinPtr, &cb, &doneFilters,
		    OBJECT_MIXIN);
	}
	FOREACH(filterObj, oPtr->filters) {
	    AddSimpleChainToCallContext(oPtr, filterObj, &cb, &doneFilters,
		    BUILDING_MIXINS, NULL);
	    AddSimpleChainToCallContext(oPtr, filterObj, &cb, &doneFilters, 0,
		    NULL);
	}
	AddClassFiltersToCallContext(oPtr, oPtr->selfCls, &cb, &doneFilters,
		BUILDING_MIXINS);
	AddClassFiltersToCallContext(oPtr, oPtr->selfCls, &cb, &doneFilters,
		0);
	Tcl_DeleteHashTable(&doneFilters);
    }
    count = cb.filterLength = callPtr->numChain;

    /*
     * Add the actual method implementations. We have to do this twice to
     * handle class mixins right.
     */

    AddSimpleChainToCallContext(oPtr, methodNameObj, &cb, NULL,
	    flags|BUILDING_MIXINS, NULL);
    AddSimpleChainToCallContext(oPtr, methodNameObj, &cb, NULL, flags, NULL);

    /*
     * Check to see if the method has no implementation. If so, we probably
     * need to add in a call to the unknown method. Otherwise, set up the
     * cacheing of the method implementation (if relevant).
     */

    if (count == callPtr->numChain) {
	/*
	 * Method does not actually exist. If we're dealing with constructors
	 * or destructors, this isn't a problem.
	 */

	if (flags & SPECIAL) {
	    TclOODeleteChain(callPtr);
	    return NULL;
	}
	AddSimpleChainToCallContext(oPtr, oPtr->fPtr->unknownMethodNameObj,
		&cb, NULL, BUILDING_MIXINS, NULL);
	AddSimpleChainToCallContext(oPtr, oPtr->fPtr->unknownMethodNameObj,
		&cb, NULL, 0, NULL);
	callPtr->flags |= OO_UNKNOWN_METHOD;
	callPtr->epoch = -1;
	if (count == callPtr->numChain) {
	    TclOODeleteChain(callPtr);
	    return NULL;
	}
    } else if (doFilters) {
	if (hPtr == NULL) {
	    if (oPtr->flags & USE_CLASS_CACHE) {
		if (oPtr->selfCls->classChainCache == NULL) {
		    oPtr->selfCls->classChainCache =
			    ckalloc(sizeof(Tcl_HashTable));

		    Tcl_InitObjHashTable(oPtr->selfCls->classChainCache);
		}
		hPtr = Tcl_CreateHashEntry(oPtr->selfCls->classChainCache,
			(char *) methodNameObj, &i);
	    } else {
		if (oPtr->chainCache == NULL) {
		    oPtr->chainCache = ckalloc(sizeof(Tcl_HashTable));

		    Tcl_InitObjHashTable(oPtr->chainCache);
		}
		hPtr = Tcl_CreateHashEntry(oPtr->chainCache,
			(char *) methodNameObj, &i);
	    }
	}
	callPtr->refCount++;
	Tcl_SetHashValue(hPtr, callPtr);
	StashCallChain(cacheInThisObj, callPtr);
    } else if (flags & CONSTRUCTOR) {
	if (oPtr->selfCls->constructorChainPtr) {
	    TclOODeleteChain(oPtr->selfCls->constructorChainPtr);
	}
	oPtr->selfCls->constructorChainPtr = callPtr;
	callPtr->refCount++;
    } else if ((flags & DESTRUCTOR) && oPtr->mixins.num == 0) {
	if (oPtr->selfCls->destructorChainPtr) {
	    TclOODeleteChain(oPtr->selfCls->destructorChainPtr);
	}
	oPtr->selfCls->destructorChainPtr = callPtr;
	callPtr->refCount++;
    }

  returnContext:
    contextPtr = TclStackAlloc(oPtr->fPtr->interp, sizeof(CallContext));
    contextPtr->oPtr = oPtr;

    /*
     * Corresponding TclOODecrRefCount() in TclOODeleteContext
     */

    AddRef(oPtr);
    contextPtr->callPtr = callPtr;
    contextPtr->skip = 2;
    contextPtr->index = 0;
    return contextPtr;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOGetStereotypeCallChain --
 *
 *	Construct a call-chain for a method that would be used by a
 *	stereotypical instance of the given class (i.e., where the object has
 *	no definitions special to itself).
 *
 * ----------------------------------------------------------------------
 */

CallChain *
TclOOGetStereotypeCallChain(
    Class *clsPtr,		/* The object to get the context for. */
    Tcl_Obj *methodNameObj,	/* The name of the method to get the context
				 * for. NULL when getting a constructor or
				 * destructor chain. */
    int flags)			/* What sort of context are we looking for.
				 * Only the bits PUBLIC_METHOD, CONSTRUCTOR,
				 * PRIVATE_METHOD, DESTRUCTOR and
				 * FILTER_HANDLING are useful. */
{
    CallChain *callPtr;
    struct ChainBuilder cb;
    int i, count;
    Foundation *fPtr = clsPtr->thisPtr->fPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashTable doneFilters;
    Object obj;

    /*
     * Synthesize a temporary stereotypical object so that we can use existing
     * machinery to produce the stereotypical call chain.
     */

    memset(&obj, 0, sizeof(Object));
    obj.fPtr = fPtr;
    obj.selfCls = clsPtr;
    obj.refCount = 1;
    obj.flags = USE_CLASS_CACHE;

    /*
     * Check if we can get the chain out of the Tcl_Obj method name or out of
     * the cache. This is made a bit more complex by the fact that there are
     * multiple different layers of cache (in the Tcl_Obj, in the object, and
     * in the class).
     */

    if (clsPtr->classChainCache != NULL) {
	hPtr = Tcl_FindHashEntry(clsPtr->classChainCache,
		(char *) methodNameObj);
	if (hPtr != NULL && Tcl_GetHashValue(hPtr) != NULL) {
	    const int reuseMask =
		    ((flags & PUBLIC_METHOD) ? ~0 : ~PUBLIC_METHOD);

	    callPtr = Tcl_GetHashValue(hPtr);
	    if (IsStillValid(callPtr, &obj, flags, reuseMask)) {
		callPtr->refCount++;
		return callPtr;
	    }
	    Tcl_SetHashValue(hPtr, NULL);
	    TclOODeleteChain(callPtr);
	}
    } else {
	hPtr = NULL;
    }

    callPtr = ckalloc(sizeof(CallChain));
    memset(callPtr, 0, sizeof(CallChain));
    callPtr->flags = flags & (PUBLIC_METHOD|PRIVATE_METHOD|FILTER_HANDLING);
    callPtr->epoch = fPtr->epoch;
    callPtr->objectCreationEpoch = fPtr->tsdPtr->nsCount;
    callPtr->objectEpoch = clsPtr->thisPtr->epoch;
    callPtr->refCount = 1;
    callPtr->chain = callPtr->staticChain;

    cb.callChainPtr = callPtr;
    cb.filterLength = 0;
    cb.oPtr = &obj;

    /*
     * Add all defined filters (if any, and if we're going to be processing
     * them; they're not processed for constructors, destructors or when we're
     * in the middle of processing a filter).
     */

    Tcl_InitObjHashTable(&doneFilters);
    AddClassFiltersToCallContext(&obj, clsPtr, &cb, &doneFilters,
	    BUILDING_MIXINS);
    AddClassFiltersToCallContext(&obj, clsPtr, &cb, &doneFilters, 0);
    Tcl_DeleteHashTable(&doneFilters);
    count = cb.filterLength = callPtr->numChain;

    /*
     * Add the actual method implementations.
     */

    AddSimpleChainToCallContext(&obj, methodNameObj, &cb, NULL,
	    flags|BUILDING_MIXINS, NULL);
    AddSimpleChainToCallContext(&obj, methodNameObj, &cb, NULL, flags, NULL);

    /*
     * Check to see if the method has no implementation. If so, we probably
     * need to add in a call to the unknown method. Otherwise, set up the
     * cacheing of the method implementation (if relevant).
     */

    if (count == callPtr->numChain) {
	AddSimpleChainToCallContext(&obj, fPtr->unknownMethodNameObj, &cb,
		NULL, BUILDING_MIXINS, NULL);
	AddSimpleChainToCallContext(&obj, fPtr->unknownMethodNameObj, &cb,
		NULL, 0, NULL);
	callPtr->flags |= OO_UNKNOWN_METHOD;
	callPtr->epoch = -1;
	if (count == callPtr->numChain) {
	    TclOODeleteChain(callPtr);
	    return NULL;
	}
    } else {
	if (hPtr == NULL) {
	    if (clsPtr->classChainCache == NULL) {
		clsPtr->classChainCache = ckalloc(sizeof(Tcl_HashTable));
		Tcl_InitObjHashTable(clsPtr->classChainCache);
	    }
	    hPtr = Tcl_CreateHashEntry(clsPtr->classChainCache,
		    (char *) methodNameObj, &i);
	}
	callPtr->refCount++;
	Tcl_SetHashValue(hPtr, callPtr);
	StashCallChain(methodNameObj, callPtr);
    }
    return callPtr;
}

/*
 * ----------------------------------------------------------------------
 *
 * AddClassFiltersToCallContext --
 *
 *	Logic to make extracting all the filters from the class context much
 *	easier.
 *
 * ----------------------------------------------------------------------
 */

static void
AddClassFiltersToCallContext(
    Object *const oPtr,		/* Object that the filters operate on. */
    Class *clsPtr,		/* Class to get the filters from. */
    struct ChainBuilder *const cbPtr,
				/* Context to fill with call chain entries. */
    Tcl_HashTable *const doneFilters,
				/* Where to record what filters have been
				 * processed. Keys are objects, values are
				 * ignored. */
    int flags)			/* Whether we've gone along a mixin link
				 * yet. */
{
    int i, clearedFlags =
	    flags & ~(TRAVERSED_MIXIN|OBJECT_MIXIN|BUILDING_MIXINS);
    Class *superPtr, *mixinPtr;
    Tcl_Obj *filterObj;

  tailRecurse:
    if (clsPtr == NULL) {
	return;
    }

    /*
     * Add all the filters defined by classes mixed into the main class
     * hierarchy.
     */

    FOREACH(mixinPtr, clsPtr->mixins) {
	AddClassFiltersToCallContext(oPtr, mixinPtr, cbPtr, doneFilters,
		flags|TRAVERSED_MIXIN);
    }

    /*
     * Add all the class filters from the current class. Note that the filters
     * are added starting at the object root, as this allows the object to
     * override how filters work to extend their behaviour.
     */

    if (MIXIN_CONSISTENT(flags)) {
	FOREACH(filterObj, clsPtr->filters) {
	    int isNew;

	    (void) Tcl_CreateHashEntry(doneFilters, (char *) filterObj,
		    &isNew);
	    if (isNew) {
		AddSimpleChainToCallContext(oPtr, filterObj, cbPtr,
			doneFilters, clearedFlags|BUILDING_MIXINS, clsPtr);
		AddSimpleChainToCallContext(oPtr, filterObj, cbPtr,
			doneFilters, clearedFlags, clsPtr);
	    }
	}
    }

    /*
     * Now process the recursive case. Notice the tail-call optimization.
     */

    switch (clsPtr->superclasses.num) {
    case 1:
	clsPtr = clsPtr->superclasses.list[0];
	goto tailRecurse;
    default:
	FOREACH(superPtr, clsPtr->superclasses) {
	    AddClassFiltersToCallContext(oPtr, superPtr, cbPtr, doneFilters,
		    flags);
	}
    case 0:
	return;
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * AddSimpleClassChainToCallContext --
 *
 *	Construct a call-chain from a class hierarchy.
 *
 * ----------------------------------------------------------------------
 */

static void
AddSimpleClassChainToCallContext(
    Class *classPtr,		/* Class to add the call chain entries for. */
    Tcl_Obj *const methodNameObj,
				/* Name of method to add the call chain
				 * entries for. */
    struct ChainBuilder *const cbPtr,
				/* Where to add the call chain entries. */
    Tcl_HashTable *const doneFilters,
				/* Where to record what call chain entries
				 * have been processed. */
    int flags,			/* What sort of call chain are we building. */
    Class *const filterDecl)	/* The class that declared the filter. If
				 * NULL, either the filter was declared by the
				 * object or this isn't a filter. */
{
    int i;
    Class *superPtr;

    /*
     * We hard-code the tail-recursive form. It's by far the most common case
     * *and* it is much more gentle on the stack.
     *
     * Note that mixins must be processed before the main class hierarchy.
     * [Bug 1998221]
     */

  tailRecurse:
    FOREACH(superPtr, classPtr->mixins) {
	AddSimpleClassChainToCallContext(superPtr, methodNameObj, cbPtr,
		doneFilters, flags|TRAVERSED_MIXIN, filterDecl);
    }

    if (flags & CONSTRUCTOR) {
	AddMethodToCallChain(classPtr->constructorPtr, cbPtr, doneFilters,
		filterDecl, flags);
    } else if (flags & DESTRUCTOR) {
	AddMethodToCallChain(classPtr->destructorPtr, cbPtr, doneFilters,
		filterDecl, flags);
    } else {
	Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&classPtr->classMethods,
		(char *) methodNameObj);

	if (hPtr != NULL) {
	    register Method *mPtr = Tcl_GetHashValue(hPtr);

	    if (!(flags & KNOWN_STATE)) {
		if (flags & PUBLIC_METHOD) {
		    if (mPtr->flags & PUBLIC_METHOD) {
			flags |= DEFINITE_PUBLIC;
		    } else {
			return;
		    }
		} else {
		    flags |= DEFINITE_PROTECTED;
		}
	    }
	    AddMethodToCallChain(mPtr, cbPtr, doneFilters, filterDecl, flags);
	}
    }

    switch (classPtr->superclasses.num) {
    case 1:
	classPtr = classPtr->superclasses.list[0];
	goto tailRecurse;
    default:
	FOREACH(superPtr, classPtr->superclasses) {
	    AddSimpleClassChainToCallContext(superPtr, methodNameObj, cbPtr,
		    doneFilters, flags, filterDecl);
	}
    case 0:
	return;
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOORenderCallChain --
 *
 *	Create a description of a call chain. Used in [info object call],
 *	[info class call], and [self call].
 *
 * ----------------------------------------------------------------------
 */

Tcl_Obj *
TclOORenderCallChain(
    Tcl_Interp *interp,
    CallChain *callPtr)
{
    Tcl_Obj *filterLiteral, *methodLiteral, *objectLiteral;
    Tcl_Obj *resultObj, *descObjs[4], **objv;
    Foundation *fPtr = TclOOGetFoundation(interp);
    int i;

    /*
     * Allocate the literals (potentially) used in our description.
     */

    filterLiteral = Tcl_NewStringObj("filter", -1);
    Tcl_IncrRefCount(filterLiteral);
    methodLiteral = Tcl_NewStringObj("method", -1);
    Tcl_IncrRefCount(methodLiteral);
    objectLiteral = Tcl_NewStringObj("object", -1);
    Tcl_IncrRefCount(objectLiteral);

    /*
     * Do the actual construction of the descriptions. They consist of a list
     * of triples that describe the details of how a method is understood. For
     * each triple, the first word is the type of invocation ("method" is
     * normal, "unknown" is special because it adds the method name as an
     * extra argument when handled by some method types, and "filter" is
     * special because it's a filter method). The second word is the name of
     * the method in question (which differs for "unknown" and "filter" types)
     * and the third word is the full name of the class that declares the
     * method (or "object" if it is declared on the instance).
     */

    objv = TclStackAlloc(interp, callPtr->numChain * sizeof(Tcl_Obj *));
    for (i = 0 ; i < callPtr->numChain ; i++) {
	struct MInvoke *miPtr = &callPtr->chain[i];

	descObjs[0] = miPtr->isFilter
		? filterLiteral
		: callPtr->flags & OO_UNKNOWN_METHOD
			? fPtr->unknownMethodNameObj
			: methodLiteral;
	descObjs[1] = callPtr->flags & CONSTRUCTOR
		? fPtr->constructorName
		: callPtr->flags & DESTRUCTOR
			? fPtr->destructorName
			: miPtr->mPtr->namePtr;
	descObjs[2] = miPtr->mPtr->declaringClassPtr
		? Tcl_GetObjectName(interp,
			(Tcl_Object) miPtr->mPtr->declaringClassPtr->thisPtr)
		: objectLiteral;
	descObjs[3] = Tcl_NewStringObj(miPtr->mPtr->typePtr->name, -1);

	objv[i] = Tcl_NewListObj(4, descObjs);
    }

    /*
     * Drop the local references to the literals; if they're actually used,
     * they'll live on the description itself.
     */

    Tcl_DecrRefCount(filterLiteral);
    Tcl_DecrRefCount(methodLiteral);
    Tcl_DecrRefCount(objectLiteral);

    /*
     * Finish building the description and return it.
     */

    resultObj = Tcl_NewListObj(callPtr->numChain, objv);
    TclStackFree(interp, objv);
    return resultObj;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
