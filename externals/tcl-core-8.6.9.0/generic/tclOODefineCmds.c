/*
 * tclOODefineCmds.c --
 *
 *	This file contains the implementation of the ::oo::define command,
 *	part of the object-system core (NB: not Tcl_Obj, but ::oo).
 *
 * Copyright (c) 2006-2013 by Donal K. Fellows
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
 * The maximum length of fully-qualified object name to use in an errorinfo
 * message. Longer than this will be curtailed.
 */

#define OBJNAME_LENGTH_IN_ERRORINFO_LIMIT 30

/*
 * Some things that make it easier to declare a slot.
 */

struct DeclaredSlot {
    const char *name;
    const Tcl_MethodType getterType;
    const Tcl_MethodType setterType;
};

#define SLOT(name,getter,setter)					\
    {"::oo::" name,							\
	    {TCL_OO_METHOD_VERSION_CURRENT, "core method: " name " Getter", \
		    getter, NULL, NULL},				\
	    {TCL_OO_METHOD_VERSION_CURRENT, "core method: " name " Setter", \
		    setter, NULL, NULL}}

/*
 * Forward declarations.
 */

static inline void	BumpGlobalEpoch(Tcl_Interp *interp, Class *classPtr);
static Tcl_Command	FindCommand(Tcl_Interp *interp, Tcl_Obj *stringObj,
			    Tcl_Namespace *const namespacePtr);
static inline void	GenerateErrorInfo(Tcl_Interp *interp, Object *oPtr,
			    Tcl_Obj *savedNameObj, const char *typeOfSubject);
static inline int	MagicDefinitionInvoke(Tcl_Interp *interp,
			    Tcl_Namespace *nsPtr, int cmdIndex,
			    int objc, Tcl_Obj *const *objv);
static inline Class *	GetClassInOuterContext(Tcl_Interp *interp,
			    Tcl_Obj *className, const char *errMsg);
static inline int	InitDefineContext(Tcl_Interp *interp,
			    Tcl_Namespace *namespacePtr, Object *oPtr,
			    int objc, Tcl_Obj *const objv[]);
static inline void	RecomputeClassCacheFlag(Object *oPtr);
static int		RenameDeleteMethod(Tcl_Interp *interp, Object *oPtr,
			    int useClass, Tcl_Obj *const fromPtr,
			    Tcl_Obj *const toPtr);
static int		ClassFilterGet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ClassFilterSet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ClassMixinGet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ClassMixinSet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ClassSuperGet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ClassSuperSet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ClassVarsGet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ClassVarsSet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ObjFilterGet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ObjFilterSet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ObjMixinGet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ObjMixinSet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ObjVarsGet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);
static int		ObjVarsSet(ClientData clientData,
			    Tcl_Interp *interp, Tcl_ObjectContext context,
			    int objc, Tcl_Obj *const *objv);

/*
 * Now define the slots used in declarations.
 */

static const struct DeclaredSlot slots[] = {
    SLOT("define::filter",      ClassFilterGet, ClassFilterSet),
    SLOT("define::mixin",       ClassMixinGet,  ClassMixinSet),
    SLOT("define::superclass",  ClassSuperGet,  ClassSuperSet),
    SLOT("define::variable",    ClassVarsGet,   ClassVarsSet),
    SLOT("objdefine::filter",   ObjFilterGet,   ObjFilterSet),
    SLOT("objdefine::mixin",    ObjMixinGet,    ObjMixinSet),
    SLOT("objdefine::variable", ObjVarsGet,     ObjVarsSet),
    {NULL, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}
};

/*
 * ----------------------------------------------------------------------
 *
 * BumpGlobalEpoch --
 *	Utility that ensures that call chains that are invalid will get thrown
 *	away at an appropriate time. Note that exactly which epoch gets
 *	advanced will depend on exactly what the class is tangled up in; in
 *	the worst case, the simplest option is to advance the global epoch,
 *	causing *everything* to be thrown away on next usage.
 *
 * ----------------------------------------------------------------------
 */

static inline void
BumpGlobalEpoch(
    Tcl_Interp *interp,
    Class *classPtr)
{
    if (classPtr != NULL
	    && classPtr->subclasses.num == 0
	    && classPtr->instances.num == 0
	    && classPtr->mixinSubs.num == 0) {
	/*
	 * If a class has no subclasses or instances, and is not mixed into
	 * anything, a change to its structure does not require us to
	 * invalidate any call chains. Note that we still bump our object's
	 * epoch if it has any mixins; the relation between a class and its
	 * representative object is special. But it won't hurt.
	 */

	if (classPtr->thisPtr->mixins.num > 0) {
	    classPtr->thisPtr->epoch++;
	}
	return;
    }

    /*
     * Either there's no class (?!) or we're reconfiguring something that is
     * in use. Force regeneration of call chains.
     */

    TclOOGetFoundation(interp)->epoch++;
}

/*
 * ----------------------------------------------------------------------
 *
 * RecomputeClassCacheFlag --
 *	Determine whether the object is prototypical of its class, and hence
 *	able to use the class's method chain cache.
 *
 * ----------------------------------------------------------------------
 */

static inline void
RecomputeClassCacheFlag(
    Object *oPtr)
{
    if ((oPtr->methodsPtr == NULL || oPtr->methodsPtr->numEntries == 0)
	    && (oPtr->mixins.num == 0) && (oPtr->filters.num == 0)) {
	oPtr->flags |= USE_CLASS_CACHE;
    } else {
	oPtr->flags &= ~USE_CLASS_CACHE;
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOObjectSetFilters --
 *	Install a list of filter method names into an object.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOObjectSetFilters(
    Object *oPtr,
    int numFilters,
    Tcl_Obj *const *filters)
{
    int i;

    if (oPtr->filters.num) {
	Tcl_Obj *filterObj;

	FOREACH(filterObj, oPtr->filters) {
	    Tcl_DecrRefCount(filterObj);
	}
    }

    if (numFilters == 0) {
	/*
	 * No list of filters was supplied, so we're deleting filters.
	 */

	ckfree(oPtr->filters.list);
	oPtr->filters.list = NULL;
	oPtr->filters.num = 0;
	RecomputeClassCacheFlag(oPtr);
    } else {
	/*
	 * We've got a list of filters, so we're creating filters.
	 */

	Tcl_Obj **filtersList;
	int size = sizeof(Tcl_Obj *) * numFilters;	/* should be size_t */

	if (oPtr->filters.num == 0) {
	    filtersList = ckalloc(size);
	} else {
	    filtersList = ckrealloc(oPtr->filters.list, size);
	}
	for (i=0 ; i<numFilters ; i++) {
	    filtersList[i] = filters[i];
	    Tcl_IncrRefCount(filters[i]);
	}
	oPtr->filters.list = filtersList;
	oPtr->filters.num = numFilters;
	oPtr->flags &= ~USE_CLASS_CACHE;
    }
    oPtr->epoch++;		/* Only this object can be affected. */
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOClassSetFilters --
 *	Install a list of filter method names into a class.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOClassSetFilters(
    Tcl_Interp *interp,
    Class *classPtr,
    int numFilters,
    Tcl_Obj *const *filters)
{
    int i;

    if (classPtr->filters.num) {
	Tcl_Obj *filterObj;

	FOREACH(filterObj, classPtr->filters) {
	    Tcl_DecrRefCount(filterObj);
	}
    }

    if (numFilters == 0) {
	/*
	 * No list of filters was supplied, so we're deleting filters.
	 */

	ckfree(classPtr->filters.list);
	classPtr->filters.list = NULL;
	classPtr->filters.num = 0;
    } else {
	/*
	 * We've got a list of filters, so we're creating filters.
	 */

	Tcl_Obj **filtersList;
	int size = sizeof(Tcl_Obj *) * numFilters;	/* should be size_t */

	if (classPtr->filters.num == 0) {
	    filtersList = ckalloc(size);
	} else {
	    filtersList = ckrealloc(classPtr->filters.list, size);
	}
	for (i=0 ; i<numFilters ; i++) {
	    filtersList[i] = filters[i];
	    Tcl_IncrRefCount(filters[i]);
	}
	classPtr->filters.list = filtersList;
	classPtr->filters.num = numFilters;
    }

    /*
     * There may be many objects affected, so bump the global epoch.
     */

    BumpGlobalEpoch(interp, classPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOObjectSetMixins --
 *	Install a list of mixin classes into an object.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOObjectSetMixins(
    Object *oPtr,
    int numMixins,
    Class *const *mixins)
{
    Class *mixinPtr;
    int i;

    if (numMixins == 0) {
	if (oPtr->mixins.num != 0) {
	    FOREACH(mixinPtr, oPtr->mixins) {
		TclOORemoveFromInstances(oPtr, mixinPtr);
		TclOODecrRefCount(mixinPtr->thisPtr);
	    }
	    ckfree(oPtr->mixins.list);
	    oPtr->mixins.num = 0;
	}
	RecomputeClassCacheFlag(oPtr);
    } else {
	if (oPtr->mixins.num != 0) {
	    FOREACH(mixinPtr, oPtr->mixins) {
		if (mixinPtr && mixinPtr != oPtr->selfCls) {
		    TclOORemoveFromInstances(oPtr, mixinPtr);
		}
		TclOODecrRefCount(mixinPtr->thisPtr);
	    }
	    oPtr->mixins.list = ckrealloc(oPtr->mixins.list,
		    sizeof(Class *) * numMixins);
	} else {
	    oPtr->mixins.list = ckalloc(sizeof(Class *) * numMixins);
	    oPtr->flags &= ~USE_CLASS_CACHE;
	}
	oPtr->mixins.num = numMixins;
	memcpy(oPtr->mixins.list, mixins, sizeof(Class *) * numMixins);
	FOREACH(mixinPtr, oPtr->mixins) {
	    if (mixinPtr != oPtr->selfCls) {
		TclOOAddToInstances(oPtr, mixinPtr);
		/* For the new copy created by memcpy */
		AddRef(mixinPtr->thisPtr);
	    }
	}
    }
    oPtr->epoch++;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOClassSetMixins --
 *	Install a list of mixin classes into a class.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOClassSetMixins(
    Tcl_Interp *interp,
    Class *classPtr,
    int numMixins,
    Class *const *mixins)
{
    Class *mixinPtr;
    int i;

    if (numMixins == 0) {
	if (classPtr->mixins.num != 0) {
	    FOREACH(mixinPtr, classPtr->mixins) {
		TclOORemoveFromMixinSubs(classPtr, mixinPtr);
		TclOODecrRefCount(mixinPtr->thisPtr);
	    }
	    ckfree(classPtr->mixins.list);
	    classPtr->mixins.num = 0;
	}
    } else {
	if (classPtr->mixins.num != 0) {
	    FOREACH(mixinPtr, classPtr->mixins) {
		TclOORemoveFromMixinSubs(classPtr, mixinPtr);
		TclOODecrRefCount(mixinPtr->thisPtr);
	    }
	    classPtr->mixins.list = ckrealloc(classPtr->mixins.list,
		    sizeof(Class *) * numMixins);
	} else {
	    classPtr->mixins.list = ckalloc(sizeof(Class *) * numMixins);
	}
	classPtr->mixins.num = numMixins;
	memcpy(classPtr->mixins.list, mixins, sizeof(Class *) * numMixins);
	FOREACH(mixinPtr, classPtr->mixins) {
	    TclOOAddToMixinSubs(classPtr, mixinPtr);
	    /* For the new copy created by memcpy */
	    AddRef(mixinPtr->thisPtr);
	}
    }
    BumpGlobalEpoch(interp, classPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * RenameDeleteMethod --
 *	Core of the code to rename and delete methods.
 *
 * ----------------------------------------------------------------------
 */

static int
RenameDeleteMethod(
    Tcl_Interp *interp,
    Object *oPtr,
    int useClass,
    Tcl_Obj *const fromPtr,
    Tcl_Obj *const toPtr)
{
    Tcl_HashEntry *hPtr, *newHPtr = NULL;
    Method *mPtr;
    int isNew;

    if (!useClass) {
	if (!oPtr->methodsPtr) {
	noSuchMethod:
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "method %s does not exist", TclGetString(fromPtr)));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		    TclGetString(fromPtr), NULL);
	    return TCL_ERROR;
	}
	hPtr = Tcl_FindHashEntry(oPtr->methodsPtr, (char *) fromPtr);
	if (hPtr == NULL) {
	    goto noSuchMethod;
	}
	if (toPtr) {
	    newHPtr = Tcl_CreateHashEntry(oPtr->methodsPtr, (char *) toPtr,
		    &isNew);
	    if (hPtr == newHPtr) {
	    renameToSelf:
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"cannot rename method to itself", -1));
		Tcl_SetErrorCode(interp, "TCL", "OO", "RENAME_TO_SELF", NULL);
		return TCL_ERROR;
	    } else if (!isNew) {
	    renameToExisting:
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"method called %s already exists",
			TclGetString(toPtr)));
		Tcl_SetErrorCode(interp, "TCL", "OO", "RENAME_OVER", NULL);
		return TCL_ERROR;
	    }
	}
    } else {
	hPtr = Tcl_FindHashEntry(&oPtr->classPtr->classMethods,
		(char *) fromPtr);
	if (hPtr == NULL) {
	    goto noSuchMethod;
	}
	if (toPtr) {
	    newHPtr = Tcl_CreateHashEntry(&oPtr->classPtr->classMethods,
		    (char *) toPtr, &isNew);
	    if (hPtr == newHPtr) {
		goto renameToSelf;
	    } else if (!isNew) {
		goto renameToExisting;
	    }
	}
    }

    /*
     * Complete the splicing by changing the method's name.
     */

    mPtr = Tcl_GetHashValue(hPtr);
    if (toPtr) {
	Tcl_IncrRefCount(toPtr);
	Tcl_DecrRefCount(mPtr->namePtr);
	mPtr->namePtr = toPtr;
	Tcl_SetHashValue(newHPtr, mPtr);
    } else {
	if (!useClass) {
	    RecomputeClassCacheFlag(oPtr);
	}
	TclOODelMethodRef(mPtr);
    }
    Tcl_DeleteHashEntry(hPtr);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOUnknownDefinition --
 *	Handles what happens when an unknown command is encountered during the
 *	processing of a definition script. Works by finding a command in the
 *	operating definition namespace that the requested command is a unique
 *	prefix of.
 *
 * ----------------------------------------------------------------------
 */

int
TclOOUnknownDefinition(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Namespace *nsPtr = (Namespace *) Tcl_GetCurrentNamespace(interp);
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    int soughtLen;
    const char *soughtStr, *matchedStr = NULL;

    if (objc < 2) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"bad call of unknown handler", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "BAD_UNKNOWN", NULL);
	return TCL_ERROR;
    }
    if (TclOOGetDefineCmdContext(interp) == NULL) {
	return TCL_ERROR;
    }

    soughtStr = Tcl_GetStringFromObj(objv[1], &soughtLen);
    if (soughtLen == 0) {
	goto noMatch;
    }
    hPtr = Tcl_FirstHashEntry(&nsPtr->cmdTable, &search);
    while (hPtr != NULL) {
	const char *nameStr = Tcl_GetHashKey(&nsPtr->cmdTable, hPtr);

	if (strncmp(soughtStr, nameStr, soughtLen) == 0) {
	    if (matchedStr != NULL) {
		goto noMatch;
	    }
	    matchedStr = nameStr;
	}
	hPtr = Tcl_NextHashEntry(&search);
    }

    if (matchedStr != NULL) {
	/*
	 * Got one match, and only one match!
	 */

	Tcl_Obj **newObjv = TclStackAlloc(interp, sizeof(Tcl_Obj*)*(objc-1));
	int result;

	newObjv[0] = Tcl_NewStringObj(matchedStr, -1);
	Tcl_IncrRefCount(newObjv[0]);
	if (objc > 2) {
	    memcpy(newObjv+1, objv+2, sizeof(Tcl_Obj *) * (objc-2));
	}
	result = Tcl_EvalObjv(interp, objc-1, newObjv, 0);
	Tcl_DecrRefCount(newObjv[0]);
	TclStackFree(interp, newObjv);
	return result;
    }

  noMatch:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "invalid command name \"%s\"", soughtStr));
    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "COMMAND", soughtStr, NULL);
    return TCL_ERROR;
}

/*
 * ----------------------------------------------------------------------
 *
 * FindCommand --
 *	Specialized version of Tcl_FindCommand that handles command prefixes
 *	and disallows namespace magic.
 *
 * ----------------------------------------------------------------------
 */

static Tcl_Command
FindCommand(
    Tcl_Interp *interp,
    Tcl_Obj *stringObj,
    Tcl_Namespace *const namespacePtr)
{
    int length;
    const char *nameStr, *string = Tcl_GetStringFromObj(stringObj, &length);
    register Namespace *const nsPtr = (Namespace *) namespacePtr;
    FOREACH_HASH_DECLS;
    Tcl_Command cmd, cmd2;

    /*
     * If someone is playing games, we stop playing right now.
     */

    if (string[0] == '\0' || strstr(string, "::") != NULL) {
	return NULL;
    }

    /*
     * Do the exact lookup first.
     */

    cmd = Tcl_FindCommand(interp, string, namespacePtr, TCL_NAMESPACE_ONLY);
    if (cmd != NULL) {
	return cmd;
    }

    /*
     * Bother, need to perform an approximate match. Iterate across the hash
     * table of commands in the namespace.
     */

    FOREACH_HASH(nameStr, cmd2, &nsPtr->cmdTable) {
	if (strncmp(string, nameStr, length) == 0) {
	    if (cmd != NULL) {
		return NULL;
	    }
	    cmd = cmd2;
	}
    }

    /*
     * Either we found one thing or we found nothing. Either way, return it.
     */

    return cmd;
}

/*
 * ----------------------------------------------------------------------
 *
 * InitDefineContext --
 *	Does the magic incantations necessary to push the special stack frame
 *	used when processing object definitions. It is up to the caller to
 *	dispose of the frame (with TclPopStackFrame) when finished.
 *
 * ----------------------------------------------------------------------
 */

static inline int
InitDefineContext(
    Tcl_Interp *interp,
    Tcl_Namespace *namespacePtr,
    Object *oPtr,
    int objc,
    Tcl_Obj *const objv[])
{
    CallFrame *framePtr, **framePtrPtr = &framePtr;

    if (namespacePtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"cannot process definitions; support namespace deleted",
		-1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    /* framePtrPtr is needed to satisfy GCC 3.3's strict aliasing rules */

    (void) TclPushStackFrame(interp, (Tcl_CallFrame **) framePtrPtr,
	    namespacePtr, FRAME_IS_OO_DEFINE);
    framePtr->clientData = oPtr;
    framePtr->objc = objc;
    framePtr->objv = objv;	/* Reference counts do not need to be
				 * incremented here. */
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOGetDefineCmdContext --
 *	Extracts the magic token from the current stack frame, or returns NULL
 *	(and leaves an error message) otherwise.
 *
 * ----------------------------------------------------------------------
 */

Tcl_Object
TclOOGetDefineCmdContext(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Object object;

    if ((iPtr->varFramePtr == NULL)
	    || (iPtr->varFramePtr->isProcCallFrame != FRAME_IS_OO_DEFINE)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"this command may only be called from within the context of"
		" an ::oo::define or ::oo::objdefine command", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return NULL;
    }
    object = iPtr->varFramePtr->clientData;
    if (Tcl_ObjectDeleted(object)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"this command cannot be called when the object has been"
		" deleted", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return NULL;
    }
    return object;
}

/*
 * ----------------------------------------------------------------------
 *
 * GetClassInOuterContext --
 *	Wrapper round Tcl_GetObjectFromObj to perform the lookup in the
 *	context that called oo::define (or equivalent). Note that this may
 *	have to go up multiple levels to get the level that we started doing
 *	definitions at.
 *
 * ----------------------------------------------------------------------
 */

static inline Class *
GetClassInOuterContext(
    Tcl_Interp *interp,
    Tcl_Obj *className,
    const char *errMsg)
{
    Interp *iPtr = (Interp *) interp;
    Object *oPtr;
    CallFrame *savedFramePtr = iPtr->varFramePtr;

    while (iPtr->varFramePtr->isProcCallFrame == FRAME_IS_OO_DEFINE) {
	if (iPtr->varFramePtr->callerVarPtr == NULL) {
	    Tcl_Panic("getting outer context when already in global context");
	}
	iPtr->varFramePtr = iPtr->varFramePtr->callerVarPtr;
    }
    oPtr = (Object *) Tcl_GetObjectFromObj(interp, className);
    iPtr->varFramePtr = savedFramePtr;
    if (oPtr == NULL) {
	return NULL;
    }
    if (oPtr->classPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(errMsg, -1));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CLASS",
		TclGetString(className), NULL);
	return NULL;
    }
    return oPtr->classPtr;
}

/*
 * ----------------------------------------------------------------------
 *
 * GenerateErrorInfo --
 *	Factored out code to generate part of the error trace messages.
 *
 * ----------------------------------------------------------------------
 */

static inline void
GenerateErrorInfo(
    Tcl_Interp *interp,		/* Where to store the error info trace. */
    Object *oPtr,		/* What object (or class) was being configured
				 * when the error occurred? */
    Tcl_Obj *savedNameObj,	/* Name of object saved from before script was
				 * evaluated, which is needed if the object
				 * goes away part way through execution. OTOH,
				 * if the object isn't deleted then its
				 * current name (post-execution) has to be
				 * used. This matters, because the object
				 * could have been renamed... */
    const char *typeOfSubject)	/* Part of the message, saying whether it was
				 * an object, class or class-as-object that
				 * was being configured. */
{
    int length;
    Tcl_Obj *realNameObj = Tcl_ObjectDeleted((Tcl_Object) oPtr)
	    ? savedNameObj : TclOOObjectName(interp, oPtr);
    const char *objName = Tcl_GetStringFromObj(realNameObj, &length);
    int limit = OBJNAME_LENGTH_IN_ERRORINFO_LIMIT;
    int overflow = (length > limit);

    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
	    "\n    (in definition script for %s \"%.*s%s\" line %d)",
	    typeOfSubject, (overflow ? limit : length), objName,
	    (overflow ? "..." : ""), Tcl_GetErrorLine(interp)));
}

/*
 * ----------------------------------------------------------------------
 *
 * MagicDefinitionInvoke --
 *	Part of the implementation of the "oo::define" and "oo::objdefine"
 *	commands that is used to implement the more-than-one-argument case,
 *	applying ensemble-like tricks with dispatch so that error messages are
 *	clearer. Doesn't handle the management of the stack frame.
 *
 * ----------------------------------------------------------------------
 */

static inline int
MagicDefinitionInvoke(
    Tcl_Interp *interp,
    Tcl_Namespace *nsPtr,
    int cmdIndex,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Obj *objPtr, *obj2Ptr, **objs;
    Tcl_Command cmd;
    int isRoot, dummy, result, offset = cmdIndex + 1;

    /*
     * More than one argument: fire them through the ensemble processing
     * engine so that everything appears to be good and proper in error
     * messages. Note that we cannot just concatenate and send through
     * Tcl_EvalObjEx, as that doesn't do ensemble processing, and we cannot go
     * through Tcl_EvalObjv without the extra work to pre-find the command, as
     * that finds command names in the wrong namespace at the moment. Ugly!
     */

    isRoot = TclInitRewriteEnsemble(interp, offset, 1, objv);

    /*
     * Build the list of arguments using a Tcl_Obj as a workspace. See
     * comments above for why these contortions are necessary.
     */

    objPtr = Tcl_NewObj();
    obj2Ptr = Tcl_NewObj();
    cmd = FindCommand(interp, objv[cmdIndex], nsPtr);
    if (cmd == NULL) {
	/* punt this case! */
	Tcl_AppendObjToObj(obj2Ptr, objv[cmdIndex]);
    } else {
	Tcl_GetCommandFullName(interp, cmd, obj2Ptr);
    }
    Tcl_ListObjAppendElement(NULL, objPtr, obj2Ptr);
    /* TODO: overflow? */
    Tcl_ListObjReplace(NULL, objPtr, 1, 0, objc-offset, objv+offset);
    Tcl_ListObjGetElements(NULL, objPtr, &dummy, &objs);

    result = Tcl_EvalObjv(interp, objc-cmdIndex, objs, TCL_EVAL_INVOKE);
    if (isRoot) {
	TclResetRewriteEnsemble(interp, 1);
    }
    Tcl_DecrRefCount(objPtr);

    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineObjCmd --
 *	Implementation of the "oo::define" command. Works by effectively doing
 *	the same as 'namespace eval', but with extra magic applied so that the
 *	object to be modified is known to the commands in the target
 *	namespace. Also does ensemble-like tricks with dispatch so that error
 *	messages are clearer.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Foundation *fPtr = TclOOGetFoundation(interp);
    Object *oPtr;
    int result;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "className arg ?arg ...?");
	return TCL_ERROR;
    }

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (oPtr->classPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"%s does not refer to a class",TclGetString(objv[1])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CLASS",
		TclGetString(objv[1]), NULL);
	return TCL_ERROR;
    }

    /*
     * Make the oo::define namespace the current namespace and evaluate the
     * command(s).
     */

    if (InitDefineContext(interp, fPtr->defineNs, oPtr, objc,objv) != TCL_OK){
	return TCL_ERROR;
    }

    AddRef(oPtr);
    if (objc == 3) {
	Tcl_Obj *objNameObj = TclOOObjectName(interp, oPtr);

	Tcl_IncrRefCount(objNameObj);
	result = TclEvalObjEx(interp, objv[2], 0,
		((Interp *)interp)->cmdFramePtr, 2);
	if (result == TCL_ERROR) {
	    GenerateErrorInfo(interp, oPtr, objNameObj, "class");
	}
	TclDecrRefCount(objNameObj);
    } else {
	result = MagicDefinitionInvoke(interp, fPtr->defineNs, 2, objc, objv);
    }
    TclOODecrRefCount(oPtr);

    /*
     * Restore the previous "current" namespace.
     */

    TclPopStackFrame(interp);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOObjDefObjCmd --
 *	Implementation of the "oo::objdefine" command. Works by effectively
 *	doing the same as 'namespace eval', but with extra magic applied so
 *	that the object to be modified is known to the commands in the target
 *	namespace. Also does ensemble-like tricks with dispatch so that error
 *	messages are clearer.
 *
 * ----------------------------------------------------------------------
 */

int
TclOOObjDefObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Foundation *fPtr = TclOOGetFoundation(interp);
    Object *oPtr;
    int result;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "objectName arg ?arg ...?");
	return TCL_ERROR;
    }

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Make the oo::objdefine namespace the current namespace and evaluate the
     * command(s).
     */

    if (InitDefineContext(interp, fPtr->objdefNs, oPtr, objc,objv) != TCL_OK){
	return TCL_ERROR;
    }

    AddRef(oPtr);
    if (objc == 3) {
	Tcl_Obj *objNameObj = TclOOObjectName(interp, oPtr);

	Tcl_IncrRefCount(objNameObj);
	result = TclEvalObjEx(interp, objv[2], 0,
		((Interp *)interp)->cmdFramePtr, 2);
	if (result == TCL_ERROR) {
	    GenerateErrorInfo(interp, oPtr, objNameObj, "object");
	}
	TclDecrRefCount(objNameObj);
    } else {
	result = MagicDefinitionInvoke(interp, fPtr->objdefNs, 2, objc, objv);
    }
    TclOODecrRefCount(oPtr);

    /*
     * Restore the previous "current" namespace.
     */

    TclPopStackFrame(interp);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineSelfObjCmd --
 *	Implementation of the "self" subcommand of the "oo::define" command.
 *	Works by effectively doing the same as 'namespace eval', but with
 *	extra magic applied so that the object to be modified is known to the
 *	commands in the target namespace. Also does ensemble-like tricks with
 *	dispatch so that error messages are clearer.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineSelfObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Foundation *fPtr = TclOOGetFoundation(interp);
    Object *oPtr;
    int result;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "arg ?arg ...?");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Make the oo::objdefine namespace the current namespace and evaluate the
     * command(s).
     */

    if (InitDefineContext(interp, fPtr->objdefNs, oPtr, objc,objv) != TCL_OK){
	return TCL_ERROR;
    }

    AddRef(oPtr);
    if (objc == 2) {
	Tcl_Obj *objNameObj = TclOOObjectName(interp, oPtr);

	Tcl_IncrRefCount(objNameObj);
	result = TclEvalObjEx(interp, objv[1], 0,
		((Interp *)interp)->cmdFramePtr, 2);
	if (result == TCL_ERROR) {
	    GenerateErrorInfo(interp, oPtr, objNameObj, "class object");
	}
	TclDecrRefCount(objNameObj);
    } else {
	result = MagicDefinitionInvoke(interp, fPtr->objdefNs, 1, objc, objv);
    }
    TclOODecrRefCount(oPtr);

    /*
     * Restore the previous "current" namespace.
     */

    TclPopStackFrame(interp);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineClassObjCmd --
 *	Implementation of the "class" subcommand of the "oo::objdefine"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineClassObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr;
    Class *clsPtr;
    Foundation *fPtr = TclOOGetFoundation(interp);
    int wasClass, willBeClass;

    /*
     * Parse the context to get the object to operate on.
     */

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (oPtr->flags & ROOT_OBJECT) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"may not modify the class of the root object class", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }
    if (oPtr->flags & ROOT_CLASS) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"may not modify the class of the class of classes", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    /*
     * Parse the argument to get the class to set the object's class to.
     */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className");
	return TCL_ERROR;
    }
    clsPtr = GetClassInOuterContext(interp, objv[1],
	    "the class of an object must be a class");
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }
    if (oPtr == clsPtr->thisPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"may not change classes into an instance of themselves", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    /*
     * Set the object's class.
     */

    wasClass = (oPtr->classPtr != NULL);
    willBeClass = (TclOOIsReachable(fPtr->classCls, clsPtr));

    if (oPtr->selfCls != clsPtr) {
	TclOORemoveFromInstances(oPtr, oPtr->selfCls);
	TclOODecrRefCount(oPtr->selfCls->thisPtr);
	oPtr->selfCls = clsPtr;
	AddRef(oPtr->selfCls->thisPtr);
	TclOOAddToInstances(oPtr, oPtr->selfCls);

	/*
	 * Create or delete the class guts if necessary.
	 */

	if (wasClass && !willBeClass) {
	    /*
	     * This is the most global of all epochs. Bump it! No cache can be
	     * trusted!
	     */

	    TclOORemoveFromMixins(oPtr->classPtr, oPtr);
	    oPtr->fPtr->epoch++;
	    oPtr->flags |= DONT_DELETE;
	    TclOODeleteDescendants(interp, oPtr);
	    oPtr->flags &= ~DONT_DELETE;
	    TclOOReleaseClassContents(interp, oPtr);
		ckfree(oPtr->classPtr);
		oPtr->classPtr = NULL;
	} else if (!wasClass && willBeClass) {
	    TclOOAllocClass(interp, oPtr);
	}

	if (oPtr->classPtr != NULL) {
	    BumpGlobalEpoch(interp, oPtr->classPtr);
	} else {
	    oPtr->epoch++;
	}
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineConstructorObjCmd --
 *	Implementation of the "constructor" subcommand of the "oo::define"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineConstructorObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr;
    Class *clsPtr;
    Tcl_Method method;
    int bodyLength;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "arguments body");
	return TCL_ERROR;
    }

    /*
     * Extract and validate the context, which is the class that we wish to
     * modify.
     */

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    clsPtr = oPtr->classPtr;

    Tcl_GetStringFromObj(objv[2], &bodyLength);
    if (bodyLength > 0) {
	/*
	 * Create the method structure.
	 */

	method = (Tcl_Method) TclOONewProcMethod(interp, clsPtr,
		PUBLIC_METHOD, NULL, objv[1], objv[2], NULL);
	if (method == NULL) {
	    return TCL_ERROR;
	}
    } else {
	/*
	 * Delete the constructor method record and set the field in the
	 * class record to NULL.
	 */

	method = NULL;
    }

    /*
     * Place the method structure in the class record. Note that we might not
     * immediately delete the constructor as this might be being done during
     * execution of the constructor itself.
     */

    Tcl_ClassSetConstructor(interp, (Tcl_Class) clsPtr, method);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineDeleteMethodObjCmd --
 *	Implementation of the "deletemethod" subcommand of the "oo::define"
 *	and "oo::objdefine" commands.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineDeleteMethodObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int isInstanceDeleteMethod = (clientData != NULL);
    Object *oPtr;
    int i;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name ?name ...?");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (!isInstanceDeleteMethod && !oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    for (i=1 ; i<objc ; i++) {
	/*
	 * Delete the method structure from the appropriate hash table.
	 */

	if (RenameDeleteMethod(interp, oPtr, !isInstanceDeleteMethod,
		objv[i], NULL) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    if (isInstanceDeleteMethod) {
	oPtr->epoch++;
    } else {
	BumpGlobalEpoch(interp, oPtr->classPtr);
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineDestructorObjCmd --
 *	Implementation of the "destructor" subcommand of the "oo::define"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineDestructorObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr;
    Class *clsPtr;
    Tcl_Method method;
    int bodyLength;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "body");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    clsPtr = oPtr->classPtr;

    Tcl_GetStringFromObj(objv[1], &bodyLength);
    if (bodyLength > 0) {
	/*
	 * Create the method structure.
	 */

	method = (Tcl_Method) TclOONewProcMethod(interp, clsPtr,
		PUBLIC_METHOD, NULL, NULL, objv[1], NULL);
	if (method == NULL) {
	    return TCL_ERROR;
	}
    } else {
	/*
	 * Delete the destructor method record and set the field in the class
	 * record to NULL.
	 */

	method = NULL;
    }

    /*
     * Place the method structure in the class record. Note that we might not
     * immediately delete the destructor as this might be being done during
     * execution of the destructor itself. Also note that setting a
     * destructor during a destructor is fairly dumb anyway.
     */

    Tcl_ClassSetDestructor(interp, (Tcl_Class) clsPtr, method);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineExportObjCmd --
 *	Implementation of the "export" subcommand of the "oo::define" and
 *	"oo::objdefine" commands.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineExportObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int isInstanceExport = (clientData != NULL);
    Object *oPtr;
    Method *mPtr;
    Tcl_HashEntry *hPtr;
    Class *clsPtr;
    int i, isNew, changed = 0;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name ?name ...?");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    clsPtr = oPtr->classPtr;
    if (!isInstanceExport && !clsPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    for (i=1 ; i<objc ; i++) {
	/*
	 * Exporting is done by adding the PUBLIC_METHOD flag to the method
	 * record. If there is no such method in this object or class (i.e.
	 * the method comes from something inherited from or that we're an
	 * instance of) then we put in a blank record with that flag; such
	 * records are skipped over by the call chain engine *except* for
	 * their flags member.
	 */

	if (isInstanceExport) {
	    if (!oPtr->methodsPtr) {
		oPtr->methodsPtr = ckalloc(sizeof(Tcl_HashTable));
		Tcl_InitObjHashTable(oPtr->methodsPtr);
		oPtr->flags &= ~USE_CLASS_CACHE;
	    }
	    hPtr = Tcl_CreateHashEntry(oPtr->methodsPtr, (char *) objv[i],
		    &isNew);
	} else {
	    hPtr = Tcl_CreateHashEntry(&clsPtr->classMethods, (char*) objv[i],
		    &isNew);
	}

	if (isNew) {
	    mPtr = ckalloc(sizeof(Method));
	    memset(mPtr, 0, sizeof(Method));
	    mPtr->refCount = 1;
	    mPtr->namePtr = objv[i];
	    Tcl_IncrRefCount(objv[i]);
	    Tcl_SetHashValue(hPtr, mPtr);
	} else {
	    mPtr = Tcl_GetHashValue(hPtr);
	}
	if (isNew || !(mPtr->flags & PUBLIC_METHOD)) {
	    mPtr->flags |= PUBLIC_METHOD;
	    changed = 1;
	}
    }

    /*
     * Bump the right epoch if we actually changed anything.
     */

    if (changed) {
	if (isInstanceExport) {
	    oPtr->epoch++;
	} else {
	    BumpGlobalEpoch(interp, clsPtr);
	}
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineForwardObjCmd --
 *	Implementation of the "forward" subcommand of the "oo::define" and
 *	"oo::objdefine" commands.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineForwardObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int isInstanceForward = (clientData != NULL);
    Object *oPtr;
    Method *mPtr;
    int isPublic;
    Tcl_Obj *prefixObj;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "name cmdName ?arg ...?");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (!isInstanceForward && !oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }
    isPublic = Tcl_StringMatch(TclGetString(objv[1]), "[a-z]*")
	    ? PUBLIC_METHOD : 0;

    /*
     * Create the method structure.
     */

    prefixObj = Tcl_NewListObj(objc-2, objv+2);
    if (isInstanceForward) {
	mPtr = TclOONewForwardInstanceMethod(interp, oPtr, isPublic, objv[1],
		prefixObj);
    } else {
	mPtr = TclOONewForwardMethod(interp, oPtr->classPtr, isPublic,
		objv[1], prefixObj);
    }
    if (mPtr == NULL) {
	Tcl_DecrRefCount(prefixObj);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineMethodObjCmd --
 *	Implementation of the "method" subcommand of the "oo::define" and
 *	"oo::objdefine" commands.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineMethodObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int isInstanceMethod = (clientData != NULL);
    Object *oPtr;
    int isPublic;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "name args body");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (!isInstanceMethod && !oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }
    isPublic = Tcl_StringMatch(TclGetString(objv[1]), "[a-z]*")
	    ? PUBLIC_METHOD : 0;

    /*
     * Create the method by using the right back-end API.
     */

    if (isInstanceMethod) {
	if (TclOONewProcInstanceMethod(interp, oPtr, isPublic, objv[1],
		objv[2], objv[3], NULL) == NULL) {
	    return TCL_ERROR;
	}
    } else {
	if (TclOONewProcMethod(interp, oPtr->classPtr, isPublic, objv[1],
		objv[2], objv[3], NULL) == NULL) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineRenameMethodObjCmd --
 *	Implementation of the "renamemethod" subcommand of the "oo::define"
 *	and "oo::objdefine" commands.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineRenameMethodObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int isInstanceRenameMethod = (clientData != NULL);
    Object *oPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "oldName newName");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (!isInstanceRenameMethod && !oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    /*
     * Delete the method entry from the appropriate hash table, and transfer
     * the thing it points to to its new entry. To do this, we first need to
     * get the entries from the appropriate hash tables (this can generate a
     * range of errors...)
     */

    if (RenameDeleteMethod(interp, oPtr, !isInstanceRenameMethod,
	    objv[1], objv[2]) != TCL_OK) {
	return TCL_ERROR;
    }

    if (isInstanceRenameMethod) {
	oPtr->epoch++;
    } else {
	BumpGlobalEpoch(interp, oPtr->classPtr);
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineUnexportObjCmd --
 *	Implementation of the "unexport" subcommand of the "oo::define" and
 *	"oo::objdefine" commands.
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineUnexportObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int isInstanceUnexport = (clientData != NULL);
    Object *oPtr;
    Method *mPtr;
    Tcl_HashEntry *hPtr;
    Class *clsPtr;
    int i, isNew, changed = 0;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name ?name ...?");
	return TCL_ERROR;
    }

    oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    clsPtr = oPtr->classPtr;
    if (!isInstanceUnexport && !clsPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    for (i=1 ; i<objc ; i++) {
	/*
	 * Unexporting is done by removing the PUBLIC_METHOD flag from the
	 * method record. If there is no such method in this object or class
	 * (i.e. the method comes from something inherited from or that we're
	 * an instance of) then we put in a blank record without that flag;
	 * such records are skipped over by the call chain engine *except* for
	 * their flags member.
	 */

	if (isInstanceUnexport) {
	    if (!oPtr->methodsPtr) {
		oPtr->methodsPtr = ckalloc(sizeof(Tcl_HashTable));
		Tcl_InitObjHashTable(oPtr->methodsPtr);
		oPtr->flags &= ~USE_CLASS_CACHE;
	    }
	    hPtr = Tcl_CreateHashEntry(oPtr->methodsPtr, (char *) objv[i],
		    &isNew);
	} else {
	    hPtr = Tcl_CreateHashEntry(&clsPtr->classMethods, (char*) objv[i],
		    &isNew);
	}

	if (isNew) {
	    mPtr = ckalloc(sizeof(Method));
	    memset(mPtr, 0, sizeof(Method));
	    mPtr->refCount = 1;
	    mPtr->namePtr = objv[i];
	    Tcl_IncrRefCount(objv[i]);
	    Tcl_SetHashValue(hPtr, mPtr);
	} else {
	    mPtr = Tcl_GetHashValue(hPtr);
	}
	if (isNew || mPtr->flags & PUBLIC_METHOD) {
	    mPtr->flags &= ~PUBLIC_METHOD;
	    changed = 1;
	}
    }

    /*
     * Bump the right epoch if we actually changed anything.
     */

    if (changed) {
	if (isInstanceUnexport) {
	    oPtr->epoch++;
	} else {
	    BumpGlobalEpoch(interp, clsPtr);
	}
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * Tcl_ClassSetConstructor, Tcl_ClassSetDestructor --
 *	How to install a constructor or destructor into a class; API to call
 *	from C.
 *
 * ----------------------------------------------------------------------
 */

void
Tcl_ClassSetConstructor(
    Tcl_Interp *interp,
    Tcl_Class clazz,
    Tcl_Method method)
{
    Class *clsPtr = (Class *) clazz;

    if (method != (Tcl_Method) clsPtr->constructorPtr) {
	TclOODelMethodRef(clsPtr->constructorPtr);
	clsPtr->constructorPtr = (Method *) method;

	/*
	 * Remember to invalidate the cached constructor chain for this class.
	 * [Bug 2531577]
	 */

	if (clsPtr->constructorChainPtr) {
	    TclOODeleteChain(clsPtr->constructorChainPtr);
	    clsPtr->constructorChainPtr = NULL;
	}
	BumpGlobalEpoch(interp, clsPtr);
    }
}

void
Tcl_ClassSetDestructor(
    Tcl_Interp *interp,
    Tcl_Class clazz,
    Tcl_Method method)
{
    Class *clsPtr = (Class *) clazz;

    if (method != (Tcl_Method) clsPtr->destructorPtr) {
	TclOODelMethodRef(clsPtr->destructorPtr);
	clsPtr->destructorPtr = (Method *) method;
	if (clsPtr->destructorChainPtr) {
	    TclOODeleteChain(clsPtr->destructorChainPtr);
	    clsPtr->destructorChainPtr = NULL;
	}
	BumpGlobalEpoch(interp, clsPtr);
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODefineSlots --
 *	Create the "::oo::Slot" class and its standard instances. Class
 *	definition is empty at the stage (added by scripting).
 *
 * ----------------------------------------------------------------------
 */

int
TclOODefineSlots(
    Foundation *fPtr)
{
    const struct DeclaredSlot *slotInfoPtr;
    Tcl_Obj *getName = Tcl_NewStringObj("Get", -1);
    Tcl_Obj *setName = Tcl_NewStringObj("Set", -1);
    Class *slotCls;

    slotCls = ((Object *) Tcl_NewObjectInstance(fPtr->interp, (Tcl_Class)
	    fPtr->classCls, "::oo::Slot", NULL, -1, NULL, 0))->classPtr;
    if (slotCls == NULL) {
	return TCL_ERROR;
    }
    Tcl_IncrRefCount(getName);
    Tcl_IncrRefCount(setName);
    for (slotInfoPtr = slots ; slotInfoPtr->name ; slotInfoPtr++) {
	Tcl_Object slotObject = Tcl_NewObjectInstance(fPtr->interp,
		(Tcl_Class) slotCls, slotInfoPtr->name, NULL,-1,NULL,0);

	if (slotObject == NULL) {
	    continue;
	}
	Tcl_NewInstanceMethod(fPtr->interp, slotObject, getName, 0,
		&slotInfoPtr->getterType, NULL);
	Tcl_NewInstanceMethod(fPtr->interp, slotObject, setName, 0,
		&slotInfoPtr->setterType, NULL);
    }
    Tcl_DecrRefCount(getName);
    Tcl_DecrRefCount(setName);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ClassFilterGet, ClassFilterSet --
 *	Implementation of the "filter" slot accessors of the "oo::define"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

static int
ClassFilterGet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    Tcl_Obj *resultObj, *filterObj;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context) != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		NULL);
	return TCL_ERROR;
    }
    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(filterObj, oPtr->classPtr->filters) {
	Tcl_ListObjAppendElement(NULL, resultObj, filterObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

static int
ClassFilterSet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    int filterc;
    Tcl_Obj **filterv;

    if (Tcl_ObjectContextSkippedArgs(context)+1 != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		"filterList");
	return TCL_ERROR;
    }
    objv += Tcl_ObjectContextSkippedArgs(context);

    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    } else if (Tcl_ListObjGetElements(interp, objv[0], &filterc,
	    &filterv) != TCL_OK) {
	return TCL_ERROR;
    }

    TclOOClassSetFilters(interp, oPtr->classPtr, filterc, filterv);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ClassMixinGet, ClassMixinSet --
 *	Implementation of the "mixin" slot accessors of the "oo::define"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

static int
ClassMixinGet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    Tcl_Obj *resultObj;
    Class *mixinPtr;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context) != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		NULL);
	return TCL_ERROR;
    }
    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(mixinPtr, oPtr->classPtr->mixins) {
	Tcl_ListObjAppendElement(NULL, resultObj,
		TclOOObjectName(interp, mixinPtr->thisPtr));
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;

}

static int
ClassMixinSet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    int mixinc, i;
    Tcl_Obj **mixinv;
    Class **mixins;

    if (Tcl_ObjectContextSkippedArgs(context)+1 != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		"mixinList");
	return TCL_ERROR;
    }
    objv += Tcl_ObjectContextSkippedArgs(context);

    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    } else if (Tcl_ListObjGetElements(interp, objv[0], &mixinc,
	    &mixinv) != TCL_OK) {
	return TCL_ERROR;
    }

    mixins = TclStackAlloc(interp, sizeof(Class *) * mixinc);

    for (i=0 ; i<mixinc ; i++) {
	mixins[i] = GetClassInOuterContext(interp, mixinv[i],
		"may only mix in classes");
	if (mixins[i] == NULL) {
	    i--;
	    goto freeAndError;
	}
	if (TclOOIsReachable(oPtr->classPtr, mixins[i])) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "may not mix a class into itself", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OO", "SELF_MIXIN", NULL);
	    goto freeAndError;
	}
    }

    TclOOClassSetMixins(interp, oPtr->classPtr, mixinc, mixins);
    TclStackFree(interp, mixins);
    return TCL_OK;

  freeAndError:
    TclStackFree(interp, mixins);
    return TCL_ERROR;
}

/*
 * ----------------------------------------------------------------------
 *
 * ClassSuperGet, ClassSuperSet --
 *	Implementation of the "superclass" slot accessors of the "oo::define"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

static int
ClassSuperGet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    Tcl_Obj *resultObj;
    Class *superPtr;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context) != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		NULL);
	return TCL_ERROR;
    }
    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(superPtr, oPtr->classPtr->superclasses) {
	Tcl_ListObjAppendElement(NULL, resultObj,
		TclOOObjectName(interp, superPtr->thisPtr));
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

static int
ClassSuperSet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    int superc, i, j;
    Tcl_Obj **superv;
    Class **superclasses, *superPtr;

    if (Tcl_ObjectContextSkippedArgs(context)+1 != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		"superclassList");
	return TCL_ERROR;
    }
    objv += Tcl_ObjectContextSkippedArgs(context);

    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    } else if (oPtr == oPtr->fPtr->objectCls->thisPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"may not modify the superclass of the root object", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    } else if (Tcl_ListObjGetElements(interp, objv[0], &superc,
	    &superv) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Allocate some working space.
     */

    superclasses = (Class **) ckalloc(sizeof(Class *) * superc);

    /*
     * Parse the arguments to get the class to use as superclasses.
     *
     * Note that zero classes is special, as it is equivalent to just the
     * class of objects. [Bug 9d61624b3d]
     */

    if (superc == 0) {
	superclasses = ckrealloc(superclasses, sizeof(Class *));
	if (TclOOIsReachable(oPtr->fPtr->classCls, oPtr->classPtr)) {
	    superclasses[0] = oPtr->fPtr->classCls;
	} else {
	    superclasses[0] = oPtr->fPtr->objectCls;
	}
	superc = 1;
	AddRef(superclasses[0]->thisPtr);
    } else {
	for (i=0 ; i<superc ; i++) {
	    superclasses[i] = GetClassInOuterContext(interp, superv[i],
		    "only a class can be a superclass");
	    if (superclasses[i] == NULL) {
		i--;
		goto failedAfterAlloc;
	    }
	    for (j=0 ; j<i ; j++) {
		if (superclasses[j] == superclasses[i]) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "class should only be a direct superclass once",
			    -1));
		    Tcl_SetErrorCode(interp, "TCL", "OO", "REPETITIOUS",NULL);
		    goto failedAfterAlloc;
		}
	    }
	    if (TclOOIsReachable(oPtr->classPtr, superclasses[i])) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"attempt to form circular dependency graph", -1));
		Tcl_SetErrorCode(interp, "TCL", "OO", "CIRCULARITY", NULL);
	    failedAfterAlloc:
		for (; i > 0; i--) {
		    TclOODecrRefCount(superclasses[i]->thisPtr);
		}
		ckfree(superclasses);
		return TCL_ERROR;
	    }
	    /* Corresponding TclOODecrRefCount() is near the end of this
	     * function */
	    AddRef(superclasses[i]->thisPtr);
	}
    }

    /*
     * Install the list of superclasses into the class. Note that this also
     * involves splicing the class out of the superclasses' subclass list that
     * it used to be a member of and splicing it into the new superclasses'
     * subclass list.
     */

    if (oPtr->classPtr->superclasses.num != 0) {
	FOREACH(superPtr, oPtr->classPtr->superclasses) {
	    TclOORemoveFromSubclasses(oPtr->classPtr, superPtr);
	    TclOODecrRefCount(superPtr->thisPtr);
	}
	ckfree((char *) oPtr->classPtr->superclasses.list);
    }
    oPtr->classPtr->superclasses.list = superclasses;
    oPtr->classPtr->superclasses.num = superc;
    FOREACH(superPtr, oPtr->classPtr->superclasses) {
	TclOOAddToSubclasses(oPtr->classPtr, superPtr);
    }
    BumpGlobalEpoch(interp, oPtr->classPtr);

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ClassVarsGet, ClassVarsSet --
 *	Implementation of the "variable" slot accessors of the "oo::define"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

static int
ClassVarsGet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    Tcl_Obj *resultObj, *variableObj;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context) != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		NULL);
	return TCL_ERROR;
    }
    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(variableObj, oPtr->classPtr->variables) {
	Tcl_ListObjAppendElement(NULL, resultObj, variableObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

static int
ClassVarsSet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    int varc;
    Tcl_Obj **varv, *variableObj;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context)+1 != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		"filterList");
	return TCL_ERROR;
    }
    objv += Tcl_ObjectContextSkippedArgs(context);

    if (oPtr == NULL) {
	return TCL_ERROR;
    } else if (!oPtr->classPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to misuse API", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "MONKEY_BUSINESS", NULL);
	return TCL_ERROR;
    } else if (Tcl_ListObjGetElements(interp, objv[0], &varc,
	    &varv) != TCL_OK) {
	return TCL_ERROR;
    }

    for (i=0 ; i<varc ; i++) {
	const char *varName = Tcl_GetString(varv[i]);

	if (strstr(varName, "::") != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "invalid declared variable name \"%s\": must not %s",
		    varName, "contain namespace separators"));
	    Tcl_SetErrorCode(interp, "TCL", "OO", "BAD_DECLVAR", NULL);
	    return TCL_ERROR;
	}
	if (Tcl_StringMatch(varName, "*(*)")) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "invalid declared variable name \"%s\": must not %s",
		    varName, "refer to an array element"));
	    Tcl_SetErrorCode(interp, "TCL", "OO", "BAD_DECLVAR", NULL);
	    return TCL_ERROR;
	}
    }

    for (i=0 ; i<varc ; i++) {
	Tcl_IncrRefCount(varv[i]);
    }
    FOREACH(variableObj, oPtr->classPtr->variables) {
	Tcl_DecrRefCount(variableObj);
    }
    if (i != varc) {
	if (varc == 0) {
	    ckfree((char *) oPtr->classPtr->variables.list);
	} else if (i) {
	    oPtr->classPtr->variables.list = (Tcl_Obj **)
		    ckrealloc((char *) oPtr->classPtr->variables.list,
		    sizeof(Tcl_Obj *) * varc);
	} else {
	    oPtr->classPtr->variables.list = (Tcl_Obj **)
		    ckalloc(sizeof(Tcl_Obj *) * varc);
	}
    }

    oPtr->classPtr->variables.num = 0;
    if (varc > 0) {
	int created, n;
	Tcl_HashTable uniqueTable;

	Tcl_InitObjHashTable(&uniqueTable);
	for (i=n=0 ; i<varc ; i++) {
	    Tcl_CreateHashEntry(&uniqueTable, varv[i], &created);
	    if (created) {
		oPtr->classPtr->variables.list[n++] = varv[i];
	    } else {
		Tcl_DecrRefCount(varv[i]);
	    }
	}
	oPtr->classPtr->variables.num = n;

	/*
	 * Shouldn't be necessary, but maintain num/list invariant.
	 */

	oPtr->classPtr->variables.list = (Tcl_Obj **)
		ckrealloc((char *) oPtr->classPtr->variables.list,
		sizeof(Tcl_Obj *) * n);
	Tcl_DeleteHashTable(&uniqueTable);
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ObjectFilterGet, ObjectFilterSet --
 *	Implementation of the "filter" slot accessors of the "oo::objdefine"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

static int
ObjFilterGet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    Tcl_Obj *resultObj, *filterObj;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context) != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		NULL);
	return TCL_ERROR;
    } else if (oPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(filterObj, oPtr->filters) {
	Tcl_ListObjAppendElement(NULL, resultObj, filterObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

static int
ObjFilterSet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    int filterc;
    Tcl_Obj **filterv;

    if (Tcl_ObjectContextSkippedArgs(context)+1 != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		"filterList");
	return TCL_ERROR;
    } else if (oPtr == NULL) {
	return TCL_ERROR;
    }
    objv += Tcl_ObjectContextSkippedArgs(context);
    if (Tcl_ListObjGetElements(interp, objv[0], &filterc,
	    &filterv) != TCL_OK) {
	return TCL_ERROR;
    }

    TclOOObjectSetFilters(oPtr, filterc, filterv);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ObjectMixinGet, ObjectMixinSet --
 *	Implementation of the "mixin" slot accessors of the "oo::objdefine"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

static int
ObjMixinGet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    Tcl_Obj *resultObj;
    Class *mixinPtr;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context) != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		NULL);
	return TCL_ERROR;
    } else if (oPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(mixinPtr, oPtr->mixins) {
	if (mixinPtr) {
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    TclOOObjectName(interp, mixinPtr->thisPtr));
	}
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

static int
ObjMixinSet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    int mixinc;
    Tcl_Obj **mixinv;
    Class **mixins;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context)+1 != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		"mixinList");
	return TCL_ERROR;
    } else if (oPtr == NULL) {
	return TCL_ERROR;
    }
    objv += Tcl_ObjectContextSkippedArgs(context);
    if (Tcl_ListObjGetElements(interp, objv[0], &mixinc,
	    &mixinv) != TCL_OK) {
	return TCL_ERROR;
    }

    mixins = TclStackAlloc(interp, sizeof(Class *) * mixinc);

    for (i=0 ; i<mixinc ; i++) {
	mixins[i] = GetClassInOuterContext(interp, mixinv[i],
		"may only mix in classes");
	if (mixins[i] == NULL) {
	    TclStackFree(interp, mixins);
	    return TCL_ERROR;
	}
    }

    TclOOObjectSetMixins(oPtr, mixinc, mixins);
    TclStackFree(interp, mixins);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * ObjectVarsGet, ObjectVarsSet --
 *	Implementation of the "variable" slot accessors of the "oo::objdefine"
 *	command.
 *
 * ----------------------------------------------------------------------
 */

static int
ObjVarsGet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    Tcl_Obj *resultObj, *variableObj;
    int i;

    if (Tcl_ObjectContextSkippedArgs(context) != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		NULL);
	return TCL_ERROR;
    } else if (oPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(variableObj, oPtr->variables) {
	Tcl_ListObjAppendElement(NULL, resultObj, variableObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

static int
ObjVarsSet(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Object *oPtr = (Object *) TclOOGetDefineCmdContext(interp);
    int varc, i;
    Tcl_Obj **varv, *variableObj;

    if (Tcl_ObjectContextSkippedArgs(context)+1 != objc) {
	Tcl_WrongNumArgs(interp, Tcl_ObjectContextSkippedArgs(context), objv,
		"variableList");
	return TCL_ERROR;
    } else if (oPtr == NULL) {
	return TCL_ERROR;
    }
    objv += Tcl_ObjectContextSkippedArgs(context);
    if (Tcl_ListObjGetElements(interp, objv[0], &varc,
	    &varv) != TCL_OK) {
	return TCL_ERROR;
    }

    for (i=0 ; i<varc ; i++) {
	const char *varName = Tcl_GetString(varv[i]);

	if (strstr(varName, "::") != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "invalid declared variable name \"%s\": must not %s",
		    varName, "contain namespace separators"));
	    Tcl_SetErrorCode(interp, "TCL", "OO", "BAD_DECLVAR", NULL);
	    return TCL_ERROR;
	}
	if (Tcl_StringMatch(varName, "*(*)")) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "invalid declared variable name \"%s\": must not %s",
		    varName, "refer to an array element"));
	    Tcl_SetErrorCode(interp, "TCL", "OO", "BAD_DECLVAR", NULL);
	    return TCL_ERROR;
	}
    }
    for (i=0 ; i<varc ; i++) {
	Tcl_IncrRefCount(varv[i]);
    }

    FOREACH(variableObj, oPtr->variables) {
	Tcl_DecrRefCount(variableObj);
    }
    if (i != varc) {
	if (varc == 0) {
	    ckfree((char *) oPtr->variables.list);
	} else if (i) {
	    oPtr->variables.list = (Tcl_Obj **)
		    ckrealloc((char *) oPtr->variables.list,
		    sizeof(Tcl_Obj *) * varc);
	} else {
	    oPtr->variables.list = (Tcl_Obj **)
		    ckalloc(sizeof(Tcl_Obj *) * varc);
	}
    }
    oPtr->variables.num = 0;
    if (varc > 0) {
	int created, n;
	Tcl_HashTable uniqueTable;

	Tcl_InitObjHashTable(&uniqueTable);
	for (i=n=0 ; i<varc ; i++) {
	    Tcl_CreateHashEntry(&uniqueTable, varv[i], &created);
	    if (created) {
		oPtr->variables.list[n++] = varv[i];
	    } else {
		Tcl_DecrRefCount(varv[i]);
	    }
	}
	oPtr->variables.num = n;

	/*
	 * Shouldn't be necessary, but maintain num/list invariant.
	 */

	oPtr->variables.list = (Tcl_Obj **)
		ckrealloc((char *) oPtr->variables.list,
		sizeof(Tcl_Obj *) * n);
	Tcl_DeleteHashTable(&uniqueTable);
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
