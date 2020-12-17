/*
 * tclNamesp.c --
 *
 *	Contains support for namespaces, which provide a separate context of
 *	commands and global variables. The global :: namespace is the
 *	traditional Tcl "global" scope. Other namespaces are created as
 *	children of the global namespace. These other namespaces contain
 *	special-purpose commands and variables for packages.
 *
 * Copyright (c) 1993-1997 Lucent Technologies.
 * Copyright (c) 1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 2002-2005 Donal K. Fellows.
 * Copyright (c) 2006 Neil Madden.
 * Contributions from Don Porter, NIST, 2007. (not subject to US copyright)
 *
 * Originally implemented by
 *   Michael J. McLennan
 *   Bell Labs Innovations for Lucent Technologies
 *   mmclennan@lucent.com
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h" /* for TclLogCommandInfo visibility */

/*
 * Thread-local storage used to avoid having a global lock on data that is not
 * limited to a single interpreter.
 */

typedef struct ThreadSpecificData {
    long numNsCreated;		/* Count of the number of namespaces created
				 * within the thread. This value is used as a
				 * unique id for each namespace. Cannot be
				 * per-interp because the nsId is used to
				 * distinguish objects which can be passed
				 * around between interps in the same thread,
				 * but does not need to be global because
				 * object internal reps are always per-thread
				 * anyway. */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * This structure contains a cached pointer to a namespace that is the result
 * of resolving the namespace's name in some other namespace. It is the
 * internal representation for a nsName object. It contains the pointer along
 * with some information that is used to check the cached pointer's validity.
 */

typedef struct ResolvedNsName {
    Namespace *nsPtr;		/* A cached pointer to the Namespace that the
				 * name resolved to. */
    Namespace *refNsPtr;	/* Points to the namespace context in which
				 * the name was resolved. NULL if the name is
				 * fully qualified and thus the resolution
				 * does not depend on the context. */
    int refCount;		/* Reference count: 1 for each nsName object
				 * that has a pointer to this ResolvedNsName
				 * structure as its internal rep. This
				 * structure can be freed when refCount
				 * becomes zero. */
} ResolvedNsName;

/*
 * Declarations for functions local to this file:
 */

static void		DeleteImportedCmd(ClientData clientData);
static int		DoImport(Tcl_Interp *interp,
			    Namespace *nsPtr, Tcl_HashEntry *hPtr,
			    const char *cmdName, const char *pattern,
			    Namespace *importNsPtr, int allowOverwrite);
static void		DupNsNameInternalRep(Tcl_Obj *objPtr,Tcl_Obj *copyPtr);
static char *		ErrorCodeRead(ClientData clientData,Tcl_Interp *interp,
			    const char *name1, const char *name2, int flags);
static char *		ErrorInfoRead(ClientData clientData,Tcl_Interp *interp,
			    const char *name1, const char *name2, int flags);
static char *		EstablishErrorCodeTraces(ClientData clientData,
			    Tcl_Interp *interp, const char *name1,
			    const char *name2, int flags);
static char *		EstablishErrorInfoTraces(ClientData clientData,
			    Tcl_Interp *interp, const char *name1,
			    const char *name2, int flags);
static void		FreeNsNameInternalRep(Tcl_Obj *objPtr);
static int		GetNamespaceFromObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr, Tcl_Namespace **nsPtrPtr);
static int		InvokeImportedCmd(ClientData clientData,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		InvokeImportedNRCmd(ClientData clientData,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NamespaceChildrenCmd(ClientData dummy,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NamespaceCodeCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceCurrentCmd(ClientData dummy,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NamespaceDeleteCmd(ClientData dummy,Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceEvalCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NRNamespaceEvalCmd(ClientData dummy,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NamespaceExistsCmd(ClientData dummy,Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceExportCmd(ClientData dummy,Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceForgetCmd(ClientData dummy,Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static void		NamespaceFree(Namespace *nsPtr);
static int		NamespaceImportCmd(ClientData dummy,Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceInscopeCmd(ClientData dummy,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NRNamespaceInscopeCmd(ClientData dummy,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NamespaceOriginCmd(ClientData dummy,Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceParentCmd(ClientData dummy,Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespacePathCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceQualifiersCmd(ClientData dummy,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NamespaceTailCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceUpvarCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		NamespaceUnknownCmd(ClientData dummy,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NamespaceWhichCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		SetNsNameFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);
static void		UnlinkNsPath(Namespace *nsPtr);

static Tcl_NRPostProc NsEval_Callback;

/*
 * This structure defines a Tcl object type that contains a namespace
 * reference. It is used in commands that take the name of a namespace as an
 * argument. The namespace reference is resolved, and the result in cached in
 * the object.
 */

static const Tcl_ObjType nsNameType = {
    "nsName",			/* the type's name */
    FreeNsNameInternalRep,	/* freeIntRepProc */
    DupNsNameInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    SetNsNameFromAny		/* setFromAnyProc */
};

/*
 * Array of values describing how to implement each standard subcommand of the
 * "namespace" command.
 */

static const EnsembleImplMap defaultNamespaceMap[] = {
    {"children",   NamespaceChildrenCmd, TclCompileBasic0To2ArgCmd, NULL, NULL, 0},
    {"code",	   NamespaceCodeCmd,	TclCompileNamespaceCodeCmd, NULL, NULL, 0},
    {"current",	   NamespaceCurrentCmd,	TclCompileNamespaceCurrentCmd, NULL, NULL, 0},
    {"delete",	   NamespaceDeleteCmd,	TclCompileBasicMin0ArgCmd, NULL, NULL, 0},
    {"ensemble",   TclNamespaceEnsembleCmd, NULL, NULL, NULL, 0},
    {"eval",	   NamespaceEvalCmd,	NULL, NRNamespaceEvalCmd, NULL, 0},
    {"exists",	   NamespaceExistsCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"export",	   NamespaceExportCmd,	TclCompileBasicMin0ArgCmd, NULL, NULL, 0},
    {"forget",	   NamespaceForgetCmd,	TclCompileBasicMin0ArgCmd, NULL, NULL, 0},
    {"import",	   NamespaceImportCmd,	TclCompileBasicMin0ArgCmd, NULL, NULL, 0},
    {"inscope",	   NamespaceInscopeCmd,	NULL, NRNamespaceInscopeCmd, NULL, 0},
    {"origin",	   NamespaceOriginCmd,	TclCompileNamespaceOriginCmd, NULL, NULL, 0},
    {"parent",	   NamespaceParentCmd,	TclCompileBasic0Or1ArgCmd, NULL, NULL, 0},
    {"path",	   NamespacePathCmd,	TclCompileBasic0Or1ArgCmd, NULL, NULL, 0},
    {"qualifiers", NamespaceQualifiersCmd, TclCompileNamespaceQualifiersCmd, NULL, NULL, 0},
    {"tail",	   NamespaceTailCmd,	TclCompileNamespaceTailCmd, NULL, NULL, 0},
    {"unknown",	   NamespaceUnknownCmd, TclCompileBasic0Or1ArgCmd, NULL, NULL, 0},
    {"upvar",	   NamespaceUpvarCmd,	TclCompileNamespaceUpvarCmd, NULL, NULL, 0},
    {"which",	   NamespaceWhichCmd,	TclCompileNamespaceWhichCmd, NULL, NULL, 0},
    {NULL, NULL, NULL, NULL, NULL, 0}
};

/*
 *----------------------------------------------------------------------
 *
 * TclInitNamespaceSubsystem --
 *
 *	This function is called to initialize all the structures that are used
 *	by namespaces on a per-process basis.
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
TclInitNamespaceSubsystem(void)
{
    /*
     * Does nothing for now.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCurrentNamespace --
 *
 *	Returns a pointer to an interpreter's currently active namespace.
 *
 * Results:
 *	Returns a pointer to the interpreter's current namespace.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Namespace *
Tcl_GetCurrentNamespace(
    register Tcl_Interp *interp)/* Interpreter whose current namespace is
				 * being queried. */
{
    return TclGetCurrentNamespace(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetGlobalNamespace --
 *
 *	Returns a pointer to an interpreter's global :: namespace.
 *
 * Results:
 *	Returns a pointer to the specified interpreter's global namespace.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Namespace *
Tcl_GetGlobalNamespace(
    register Tcl_Interp *interp)/* Interpreter whose global namespace should
				 * be returned. */
{
    return TclGetGlobalNamespace(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PushCallFrame --
 *
 *	Pushes a new call frame onto the interpreter's Tcl call stack. Called
 *	when executing a Tcl procedure or a "namespace eval" or "namespace
 *	inscope" command.
 *
 * Results:
 *	Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *	message in the interpreter's result object) if something goes wrong.
 *
 * Side effects:
 *	Modifies the interpreter's Tcl call stack.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_PushCallFrame(
    Tcl_Interp *interp,		/* Interpreter in which the new call frame is
				 * to be pushed. */
    Tcl_CallFrame *callFramePtr,/* Points to a call frame structure to push.
				 * Storage for this has already been allocated
				 * by the caller; typically this is the
				 * address of a CallFrame structure allocated
				 * on the caller's C stack. The call frame
				 * will be initialized by this function. The
				 * caller can pop the frame later with
				 * Tcl_PopCallFrame, and it is responsible for
				 * freeing the frame's storage. */
    Tcl_Namespace *namespacePtr,/* Points to the namespace in which the frame
				 * will execute. If NULL, the interpreter's
				 * current namespace will be used. */
    int isProcCallFrame)	/* If nonzero, the frame represents a called
				 * Tcl procedure and may have local vars. Vars
				 * will ordinarily be looked up in the frame.
				 * If new variables are created, they will be
				 * created in the frame. If 0, the frame is
				 * for a "namespace eval" or "namespace
				 * inscope" command and var references are
				 * treated as references to namespace
				 * variables. */
{
    Interp *iPtr = (Interp *) interp;
    register CallFrame *framePtr = (CallFrame *) callFramePtr;
    register Namespace *nsPtr;

    if (namespacePtr == NULL) {
	nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    } else {
	nsPtr = (Namespace *) namespacePtr;

	/*
	 * TODO: Examine whether it would be better to guard based on NS_DYING
	 * or NS_KILLED. It appears that these are not tested because they can
	 * be set in a global interp that has been [namespace delete]d, but
	 * which never really completely goes away because of lingering global
	 * things like ::errorInfo and [::unknown] and hidden commands.
	 * Review of those designs might permit stricter checking here.
	 */

	if (nsPtr->flags & NS_DEAD) {
	    Tcl_Panic("Trying to push call frame for dead namespace");
	    /*NOTREACHED*/
	}
    }

    nsPtr->activationCount++;
    framePtr->nsPtr = nsPtr;
    framePtr->isProcCallFrame = isProcCallFrame;
    framePtr->objc = 0;
    framePtr->objv = NULL;
    framePtr->callerPtr = iPtr->framePtr;
    framePtr->callerVarPtr = iPtr->varFramePtr;
    if (iPtr->varFramePtr != NULL) {
	framePtr->level = (iPtr->varFramePtr->level + 1);
    } else {
	framePtr->level = 0;
    }
    framePtr->procPtr = NULL;		/* no called procedure */
    framePtr->varTablePtr = NULL;	/* and no local variables */
    framePtr->numCompiledLocals = 0;
    framePtr->compiledLocals = NULL;
    framePtr->clientData = NULL;
    framePtr->localCachePtr = NULL;
    framePtr->tailcallPtr = NULL;

    /*
     * Push the new call frame onto the interpreter's stack of procedure call
     * frames making it the current frame.
     */

    iPtr->framePtr = framePtr;
    iPtr->varFramePtr = framePtr;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PopCallFrame --
 *
 *	Removes a call frame from the Tcl call stack for the interpreter.
 *	Called to remove a frame previously pushed by Tcl_PushCallFrame.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the call stack of the interpreter. Resets various fields of
 *	the popped call frame. If a namespace has been deleted and has no more
 *	activations on the call stack, the namespace is destroyed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_PopCallFrame(
    Tcl_Interp *interp)		/* Interpreter with call frame to pop. */
{
    register Interp *iPtr = (Interp *) interp;
    register CallFrame *framePtr = iPtr->framePtr;
    Namespace *nsPtr;

    /*
     * It's important to remove the call frame from the interpreter's stack of
     * call frames before deleting local variables, so that traces invoked by
     * the variable deletion don't see the partially-deleted frame.
     */

    if (framePtr->callerPtr) {
	iPtr->framePtr = framePtr->callerPtr;
	iPtr->varFramePtr = framePtr->callerVarPtr;
    } else {
	/* Tcl_PopCallFrame: trying to pop rootCallFrame! */
    }

    if (framePtr->varTablePtr != NULL) {
	TclDeleteVars(iPtr, framePtr->varTablePtr);
	ckfree(framePtr->varTablePtr);
	framePtr->varTablePtr = NULL;
    }
    if (framePtr->numCompiledLocals > 0) {
	TclDeleteCompiledLocalVars(iPtr, framePtr);
	if (--framePtr->localCachePtr->refCount == 0) {
	    TclFreeLocalCache(interp, framePtr->localCachePtr);
	}
	framePtr->localCachePtr = NULL;
    }

    /*
     * Decrement the namespace's count of active call frames. If the namespace
     * is "dying" and there are no more active call frames, call
     * Tcl_DeleteNamespace to destroy it.
     */

    nsPtr = framePtr->nsPtr;
    nsPtr->activationCount--;
    if ((nsPtr->flags & NS_DYING)
	    && (nsPtr->activationCount - (nsPtr == iPtr->globalNsPtr) == 0)) {
	Tcl_DeleteNamespace((Tcl_Namespace *) nsPtr);
    }
    framePtr->nsPtr = NULL;

    if (framePtr->tailcallPtr) {
	TclSetTailcall(interp, framePtr->tailcallPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclPushStackFrame --
 *
 *	Allocates a new call frame in the interpreter's execution stack, then
 *	pushes it onto the interpreter's Tcl call stack. Called when executing
 *	a Tcl procedure or a "namespace eval" or "namespace inscope" command.
 *
 * Results:
 *	Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *	message in the interpreter's result object) if something goes wrong.
 *
 * Side effects:
 *	Modifies the interpreter's Tcl call stack.
 *
 *----------------------------------------------------------------------
 */

int
TclPushStackFrame(
    Tcl_Interp *interp,		/* Interpreter in which the new call frame is
				 * to be pushed. */
    Tcl_CallFrame **framePtrPtr,/* Place to store a pointer to the stack
				 * allocated call frame. */
    Tcl_Namespace *namespacePtr,/* Points to the namespace in which the frame
				 * will execute. If NULL, the interpreter's
				 * current namespace will be used. */
    int isProcCallFrame)	/* If nonzero, the frame represents a called
				 * Tcl procedure and may have local vars. Vars
				 * will ordinarily be looked up in the frame.
				 * If new variables are created, they will be
				 * created in the frame. If 0, the frame is
				 * for a "namespace eval" or "namespace
				 * inscope" command and var references are
				 * treated as references to namespace
				 * variables. */
{
    *framePtrPtr = TclStackAlloc(interp, sizeof(CallFrame));
    return Tcl_PushCallFrame(interp, *framePtrPtr, namespacePtr,
	    isProcCallFrame);
}

void
TclPopStackFrame(
    Tcl_Interp *interp)		/* Interpreter with call frame to pop. */
{
    CallFrame *freePtr = ((Interp *) interp)->framePtr;

    Tcl_PopCallFrame(interp);
    TclStackFree(interp, freePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * EstablishErrorCodeTraces --
 *
 *	Creates traces on the ::errorCode variable to keep its value
 *	consistent with the expectations of legacy code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read and unset traces are established on ::errorCode.
 *
 *----------------------------------------------------------------------
 */

static char *
EstablishErrorCodeTraces(
    ClientData clientData,
    Tcl_Interp *interp,
    const char *name1,
    const char *name2,
    int flags)
{
    Tcl_TraceVar2(interp, "errorCode", NULL, TCL_GLOBAL_ONLY|TCL_TRACE_READS,
	    ErrorCodeRead, NULL);
    Tcl_TraceVar2(interp, "errorCode", NULL, TCL_GLOBAL_ONLY|TCL_TRACE_UNSETS,
	    EstablishErrorCodeTraces, NULL);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ErrorCodeRead --
 *
 *	Called when the ::errorCode variable is read. Copies the current value
 *	of the interp's errorCode field into ::errorCode.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
ErrorCodeRead(
    ClientData clientData,
    Tcl_Interp *interp,
    const char *name1,
    const char *name2,
    int flags)
{
    Interp *iPtr = (Interp *) interp;

    if (Tcl_InterpDeleted(interp) || !(iPtr->flags & ERR_LEGACY_COPY)) {
	return NULL;
    }
    if (iPtr->errorCode) {
	Tcl_ObjSetVar2(interp, iPtr->ecVar, NULL,
		iPtr->errorCode, TCL_GLOBAL_ONLY);
	return NULL;
    }
    if (NULL == Tcl_ObjGetVar2(interp, iPtr->ecVar, NULL, TCL_GLOBAL_ONLY)) {
	Tcl_ObjSetVar2(interp, iPtr->ecVar, NULL,
		Tcl_NewObj(), TCL_GLOBAL_ONLY);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * EstablishErrorInfoTraces --
 *
 *	Creates traces on the ::errorInfo variable to keep its value
 *	consistent with the expectations of legacy code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read and unset traces are established on ::errorInfo.
 *
 *----------------------------------------------------------------------
 */

static char *
EstablishErrorInfoTraces(
    ClientData clientData,
    Tcl_Interp *interp,
    const char *name1,
    const char *name2,
    int flags)
{
    Tcl_TraceVar2(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY|TCL_TRACE_READS,
	    ErrorInfoRead, NULL);
    Tcl_TraceVar2(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY|TCL_TRACE_UNSETS,
	    EstablishErrorInfoTraces, NULL);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ErrorInfoRead --
 *
 *	Called when the ::errorInfo variable is read. Copies the current value
 *	of the interp's errorInfo field into ::errorInfo.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
ErrorInfoRead(
    ClientData clientData,
    Tcl_Interp *interp,
    const char *name1,
    const char *name2,
    int flags)
{
    Interp *iPtr = (Interp *) interp;

    if (Tcl_InterpDeleted(interp) || !(iPtr->flags & ERR_LEGACY_COPY)) {
	return NULL;
    }
    if (iPtr->errorInfo) {
	Tcl_ObjSetVar2(interp, iPtr->eiVar, NULL,
		iPtr->errorInfo, TCL_GLOBAL_ONLY);
	return NULL;
    }
    if (NULL == Tcl_ObjGetVar2(interp, iPtr->eiVar, NULL, TCL_GLOBAL_ONLY)) {
	Tcl_ObjSetVar2(interp, iPtr->eiVar, NULL,
		Tcl_NewObj(), TCL_GLOBAL_ONLY);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateNamespace --
 *
 *	Creates a new namespace with the given name. If there is no active
 *	namespace (i.e., the interpreter is being initialized), the global ::
 *	namespace is created and returned.
 *
 * Results:
 *	Returns a pointer to the new namespace if successful. If the namespace
 *	already exists or if another error occurs, this routine returns NULL,
 *	along with an error message in the interpreter's result object.
 *
 * Side effects:
 *	If the name contains "::" qualifiers and a parent namespace does not
 *	already exist, it is automatically created.
 *
 *----------------------------------------------------------------------
 */

Tcl_Namespace *
Tcl_CreateNamespace(
    Tcl_Interp *interp,		/* Interpreter in which a new namespace is
				 * being created. Also used for error
				 * reporting. */
    const char *name,		/* Name for the new namespace. May be a
				 * qualified name with names of ancestor
				 * namespaces separated by "::"s. */
    ClientData clientData,	/* One-word value to store with namespace. */
    Tcl_NamespaceDeleteProc *deleteProc)
				/* Function called to delete client data when
				 * the namespace is deleted. NULL if no
				 * function should be called. */
{
    Interp *iPtr = (Interp *) interp;
    register Namespace *nsPtr, *ancestorPtr;
    Namespace *parentPtr, *dummy1Ptr, *dummy2Ptr;
    Namespace *globalNsPtr = iPtr->globalNsPtr;
    const char *simpleName;
    Tcl_HashEntry *entryPtr;
    Tcl_DString buffer1, buffer2;
    Tcl_DString *namePtr, *buffPtr;
    int newEntry, nameLen;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    const char *nameStr;
    Tcl_DString tmpBuffer;

    Tcl_DStringInit(&tmpBuffer);

    /*
     * If there is no active namespace, the interpreter is being initialized.
     */

    if ((globalNsPtr == NULL) && (iPtr->varFramePtr == NULL)) {
	/*
	 * Treat this namespace as the global namespace, and avoid looking for
	 * a parent.
	 */

	parentPtr = NULL;
	simpleName = "";
	goto doCreate;
    }

    /*
     * Ensure that there are no trailing colons as that causes chaos when a
     * deleteProc is specified. [Bug d614d63989]
     */

    if (deleteProc != NULL) {
	nameStr = name + strlen(name) - 2;
	if (nameStr >= name && nameStr[1] == ':' && nameStr[0] == ':') {
	    Tcl_DStringAppend(&tmpBuffer, name, -1);
	    while ((nameLen = Tcl_DStringLength(&tmpBuffer)) > 0
		    && Tcl_DStringValue(&tmpBuffer)[nameLen-1] == ':') {
		Tcl_DStringSetLength(&tmpBuffer, nameLen-1);
	    }
	    name = Tcl_DStringValue(&tmpBuffer);
	}
    }

    /*
     * If we've ended up with an empty string now, we're attempting to create
     * the global namespace despite the global namespace existing. That's
     * naughty!
     */

    if (*name == '\0') {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("can't create namespace"
                " \"\": only global namespace can have empty name", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "NAMESPACE",
		"CREATEGLOBAL", NULL);
	Tcl_DStringFree(&tmpBuffer);
	return NULL;
    }

    /*
     * Find the parent for the new namespace.
     */

    TclGetNamespaceForQualName(interp, name, NULL, TCL_CREATE_NS_IF_UNKNOWN,
	    &parentPtr, &dummy1Ptr, &dummy2Ptr, &simpleName);

    /*
     * If the unqualified name at the end is empty, there were trailing "::"s
     * after the namespace's name which we ignore. The new namespace was
     * already (recursively) created and is pointed to by parentPtr.
     */

    if (*simpleName == '\0') {
	Tcl_DStringFree(&tmpBuffer);
	return (Tcl_Namespace *) parentPtr;
    }

    /*
     * Check for a bad namespace name and make sure that the name does not
     * already exist in the parent namespace.
     */

    if (
#ifndef BREAK_NAMESPACE_COMPAT
	Tcl_FindHashEntry(&parentPtr->childTable, simpleName) != NULL
#else
	parentPtr->childTablePtr != NULL &&
	Tcl_FindHashEntry(parentPtr->childTablePtr, simpleName) != NULL
#endif
    ) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't create namespace \"%s\": already exists", name));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "NAMESPACE",
		"CREATEEXISTING", NULL);
	Tcl_DStringFree(&tmpBuffer);
	return NULL;
    }

    /*
     * Create the new namespace and root it in its parent. Increment the count
     * of namespaces created.
     */

  doCreate:
    nsPtr = ckalloc(sizeof(Namespace));
    nameLen = strlen(simpleName) + 1;
    nsPtr->name = ckalloc(nameLen);
    memcpy(nsPtr->name, simpleName, nameLen);
    nsPtr->fullName = NULL;		/* Set below. */
    nsPtr->clientData = clientData;
    nsPtr->deleteProc = deleteProc;
    nsPtr->parentPtr = parentPtr;
#ifndef BREAK_NAMESPACE_COMPAT
    Tcl_InitHashTable(&nsPtr->childTable, TCL_STRING_KEYS);
#else
    nsPtr->childTablePtr = NULL;
#endif
    nsPtr->nsId = ++(tsdPtr->numNsCreated);
    nsPtr->interp = interp;
    nsPtr->flags = 0;
    nsPtr->activationCount = 0;
    nsPtr->refCount = 0;
    Tcl_InitHashTable(&nsPtr->cmdTable, TCL_STRING_KEYS);
    TclInitVarHashTable(&nsPtr->varTable, nsPtr);
    nsPtr->exportArrayPtr = NULL;
    nsPtr->numExportPatterns = 0;
    nsPtr->maxExportPatterns = 0;
    nsPtr->cmdRefEpoch = 0;
    nsPtr->resolverEpoch = 0;
    nsPtr->cmdResProc = NULL;
    nsPtr->varResProc = NULL;
    nsPtr->compiledVarResProc = NULL;
    nsPtr->exportLookupEpoch = 0;
    nsPtr->ensembles = NULL;
    nsPtr->unknownHandlerPtr = NULL;
    nsPtr->commandPathLength = 0;
    nsPtr->commandPathArray = NULL;
    nsPtr->commandPathSourceList = NULL;
    nsPtr->earlyDeleteProc = NULL;

    if (parentPtr != NULL) {
	entryPtr = Tcl_CreateHashEntry(
		TclGetNamespaceChildTable((Tcl_Namespace *) parentPtr),
		simpleName, &newEntry);
	Tcl_SetHashValue(entryPtr, nsPtr);
    } else {
	/*
	 * In the global namespace create traces to maintain the ::errorInfo
	 * and ::errorCode variables.
	 */

	iPtr->globalNsPtr = nsPtr;
	EstablishErrorInfoTraces(NULL, interp, NULL, NULL, 0);
	EstablishErrorCodeTraces(NULL, interp, NULL, NULL, 0);
    }

    /*
     * Build the fully qualified name for this namespace.
     */

    Tcl_DStringInit(&buffer1);
    Tcl_DStringInit(&buffer2);
    namePtr = &buffer1;
    buffPtr = &buffer2;
    for (ancestorPtr = nsPtr; ancestorPtr != NULL;
	    ancestorPtr = ancestorPtr->parentPtr) {
	if (ancestorPtr != globalNsPtr) {
	    register Tcl_DString *tempPtr = namePtr;

	    TclDStringAppendLiteral(buffPtr, "::");
	    Tcl_DStringAppend(buffPtr, ancestorPtr->name, -1);
	    TclDStringAppendDString(buffPtr, namePtr);

	    /*
	     * Clear the unwanted buffer or we end up appending to previous
	     * results, making the namespace fullNames of nested namespaces
	     * very wrong (and strange).
	     */

	    TclDStringClear(namePtr);

	    /*
	     * Now swap the buffer pointers so that we build in the other
	     * buffer. This is faster than repeated copying back and forth
	     * between buffers.
	     */

	    namePtr = buffPtr;
	    buffPtr = tempPtr;
	}
    }

    name = Tcl_DStringValue(namePtr);
    nameLen = Tcl_DStringLength(namePtr);
    nsPtr->fullName = ckalloc(nameLen + 1);
    memcpy(nsPtr->fullName, name, (unsigned) nameLen + 1);

    Tcl_DStringFree(&buffer1);
    Tcl_DStringFree(&buffer2);
    Tcl_DStringFree(&tmpBuffer);

    /*
     * If compilation of commands originating from the parent NS is
     * suppressed, suppress it for commands originating in this one too.
     */

    if (nsPtr->parentPtr != NULL &&
	    nsPtr->parentPtr->flags & NS_SUPPRESS_COMPILATION) {
	nsPtr->flags |= NS_SUPPRESS_COMPILATION;
    }

    /*
     * Return a pointer to the new namespace.
     */

    return (Tcl_Namespace *) nsPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteNamespace --
 *
 *	Deletes a namespace and all of the commands, variables, and other
 *	namespaces within it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When a namespace is deleted, it is automatically removed as a child of
 *	its parent namespace. Also, all its commands, variables and child
 *	namespaces are deleted.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteNamespace(
    Tcl_Namespace *namespacePtr)/* Points to the namespace to delete. */
{
    register Namespace *nsPtr = (Namespace *) namespacePtr;
    Interp *iPtr = (Interp *) nsPtr->interp;
    Namespace *globalNsPtr = (Namespace *)
	    TclGetGlobalNamespace((Tcl_Interp *) iPtr);
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;
    Command *cmdPtr;

    /*
     * Ensure that this namespace doesn't get deallocated in the meantime.
     */
    nsPtr->refCount++;

    /*
     * Give anyone interested - notably TclOO - a chance to use this namespace
     * normally despite the fact that the namespace is going to go. Allows the
     * calling of destructors. Will only be called once (unless re-established
     * by the called function). [Bug 2950259]
     *
     * Note that setting this field requires access to the internal definition
     * of namespaces, so it should only be accessed by code that knows about
     * being careful with reentrancy.
     */

    if (nsPtr->earlyDeleteProc != NULL) {
	Tcl_NamespaceDeleteProc *earlyDeleteProc = nsPtr->earlyDeleteProc;

	nsPtr->earlyDeleteProc = NULL;
	nsPtr->activationCount++;
	earlyDeleteProc(nsPtr->clientData);
	nsPtr->activationCount--;
    }

    /*
     * Delete all coroutine commands now: break the circular ref cycle between
     * the namespace and the coroutine command [Bug 2724403]. This code is
     * essentially duplicated in TclTeardownNamespace() for all other
     * commands. Don't optimize to Tcl_NextHashEntry() because of traces.
     *
     * NOTE: we could avoid traversing the ns's command list by keeping a
     * separate list of coros.
     */

    for (entryPtr = Tcl_FirstHashEntry(&nsPtr->cmdTable, &search);
	    entryPtr != NULL;) {
	cmdPtr = Tcl_GetHashValue(entryPtr);
	if (cmdPtr->nreProc == TclNRInterpCoroutine) {
	    Tcl_DeleteCommandFromToken((Tcl_Interp *) iPtr,
		    (Tcl_Command) cmdPtr);
	    entryPtr = Tcl_FirstHashEntry(&nsPtr->cmdTable, &search);
	} else {
	    entryPtr = Tcl_NextHashEntry(&search);
	}
    }

    /*
     * If the namespace has associated ensemble commands, delete them first.
     * This leaves the actual contents of the namespace alone (unless they are
     * linked ensemble commands, of course). Note that this code is actually
     * reentrant so command delete traces won't purturb things badly.
     */

    while (nsPtr->ensembles != NULL) {
	EnsembleConfig *ensemblePtr = (EnsembleConfig *) nsPtr->ensembles;

	/*
	 * Splice out and link to indicate that we've already been killed.
	 */

	nsPtr->ensembles = (Tcl_Ensemble *) ensemblePtr->next;
	ensemblePtr->next = ensemblePtr;
	Tcl_DeleteCommandFromToken(nsPtr->interp, ensemblePtr->token);
    }

    /*
     * If the namespace has a registered unknown handler (TIP 181), then free
     * it here.
     */

    if (nsPtr->unknownHandlerPtr != NULL) {
	Tcl_DecrRefCount(nsPtr->unknownHandlerPtr);
	nsPtr->unknownHandlerPtr = NULL;
    }

    /*
     * If the namespace is on the call frame stack, it is marked as "dying"
     * (NS_DYING is OR'd into its flags): the namespace can't be looked up by
     * name but its commands and variables are still usable by those active
     * call frames. When all active call frames referring to the namespace
     * have been popped from the Tcl stack, Tcl_PopCallFrame will call this
     * function again to delete everything in the namespace. If no nsName
     * objects refer to the namespace (i.e., if its refCount is zero), its
     * commands and variables are deleted and the storage for its namespace
     * structure is freed. Otherwise, if its refCount is nonzero, the
     * namespace's commands and variables are deleted but the structure isn't
     * freed. Instead, NS_DEAD is OR'd into the structure's flags to allow the
     * namespace resolution code to recognize that the namespace is "deleted".
     * The structure's storage is freed by FreeNsNameInternalRep when its
     * refCount reaches 0.
     */

    if (nsPtr->activationCount - (nsPtr == globalNsPtr) > 0) {
	nsPtr->flags |= NS_DYING;
	if (nsPtr->parentPtr != NULL) {
	    entryPtr = Tcl_FindHashEntry(
		    TclGetNamespaceChildTable((Tcl_Namespace *)
			    nsPtr->parentPtr), nsPtr->name);
	    if (entryPtr != NULL) {
		Tcl_DeleteHashEntry(entryPtr);
	    }
	}
	nsPtr->parentPtr = NULL;
    } else if (!(nsPtr->flags & NS_KILLED)) {
	/*
	 * Delete the namespace and everything in it. If this is the global
	 * namespace, then clear it but don't free its storage unless the
	 * interpreter is being torn down. Set the NS_KILLED flag to avoid
	 * recursive calls here - if the namespace is really in the process of
	 * being deleted, ignore any second call.
	 */

	nsPtr->flags |= (NS_DYING|NS_KILLED);

	TclTeardownNamespace(nsPtr);

	if ((nsPtr != globalNsPtr) || (iPtr->flags & DELETED)) {
	    /*
	     * If this is the global namespace, then it may have residual
	     * "errorInfo" and "errorCode" variables for errors that occurred
	     * while it was being torn down. Try to clear the variable list
	     * one last time.
	     */

	    TclDeleteNamespaceVars(nsPtr);

#ifndef BREAK_NAMESPACE_COMPAT
	    Tcl_DeleteHashTable(&nsPtr->childTable);
#else
	    if (nsPtr->childTablePtr != NULL) {
		Tcl_DeleteHashTable(nsPtr->childTablePtr);
		ckfree(nsPtr->childTablePtr);
	    }
#endif
	    Tcl_DeleteHashTable(&nsPtr->cmdTable);

	    nsPtr ->flags |= NS_DEAD;
	} else {
	    /*
	     * Restore the ::errorInfo and ::errorCode traces.
	     */

	    EstablishErrorInfoTraces(NULL, nsPtr->interp, NULL, NULL, 0);
	    EstablishErrorCodeTraces(NULL, nsPtr->interp, NULL, NULL, 0);

	    /*
	     * We didn't really kill it, so remove the KILLED marks, so it can
	     * get killed later, avoiding mem leaks.
	     */

	    nsPtr->flags &= ~(NS_DYING|NS_KILLED);
	}
    }
    TclNsDecrRefCount(nsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclTeardownNamespace --
 *
 *	Used internally to dismantle and unlink a namespace when it is
 *	deleted. Divorces the namespace from its parent, and deletes all
 *	commands, variables, and child namespaces.
 *
 *	This is kept separate from Tcl_DeleteNamespace so that the global
 *	namespace can be handled specially.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes this namespace from its parent's child namespace hashtable.
 *	Deletes all commands, variables and namespaces in this namespace.
 *
 *----------------------------------------------------------------------
 */

void
TclTeardownNamespace(
    register Namespace *nsPtr)	/* Points to the namespace to be dismantled
				 * and unlinked from its parent. */
{
    Interp *iPtr = (Interp *) nsPtr->interp;
    register Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;
    int i;

    /*
     * Start by destroying the namespace's variable table, since variables
     * might trigger traces. Variable table should be cleared but not freed!
     * TclDeleteNamespaceVars frees it, so we reinitialize it afterwards.
     */

    TclDeleteNamespaceVars(nsPtr);
    TclInitVarHashTable(&nsPtr->varTable, nsPtr);

    /*
     * Delete all commands in this namespace. Be careful when traversing the
     * hash table: when each command is deleted, it removes itself from the
     * command table. Because of traces (and the desire to avoid the quadratic
     * problems of just using Tcl_FirstHashEntry over and over, [Bug
     * f97d4ee020]) we copy to a temporary array and then delete all those
     * commands.
     */

    while (nsPtr->cmdTable.numEntries > 0) {
	int length = nsPtr->cmdTable.numEntries;
	Command **cmds = TclStackAlloc((Tcl_Interp *) iPtr,
		sizeof(Command *) * length);

	i = 0;
	for (entryPtr = Tcl_FirstHashEntry(&nsPtr->cmdTable, &search);
		entryPtr != NULL;
		entryPtr = Tcl_NextHashEntry(&search)) {
	    cmds[i] = Tcl_GetHashValue(entryPtr);
	    cmds[i]->refCount++;
	    i++;
	}
	for (i = 0 ; i < length ; i++) {
	    Tcl_DeleteCommandFromToken((Tcl_Interp *) iPtr,
		    (Tcl_Command) cmds[i]);
	    TclCleanupCommandMacro(cmds[i]);
	}
	TclStackFree((Tcl_Interp *) iPtr, cmds);
    }
    Tcl_DeleteHashTable(&nsPtr->cmdTable);
    Tcl_InitHashTable(&nsPtr->cmdTable, TCL_STRING_KEYS);

    /*
     * Remove the namespace from its parent's child hashtable.
     */

    if (nsPtr->parentPtr != NULL) {
	entryPtr = Tcl_FindHashEntry(
		TclGetNamespaceChildTable((Tcl_Namespace *)
			nsPtr->parentPtr), nsPtr->name);
	if (entryPtr != NULL) {
	    Tcl_DeleteHashEntry(entryPtr);
	}
    }
    nsPtr->parentPtr = NULL;

    /*
     * Delete the namespace path if one is installed.
     */

    if (nsPtr->commandPathLength != 0) {
	UnlinkNsPath(nsPtr);
	nsPtr->commandPathLength = 0;
    }
    if (nsPtr->commandPathSourceList != NULL) {
	NamespacePathEntry *nsPathPtr = nsPtr->commandPathSourceList;

	do {
	    if (nsPathPtr->nsPtr != NULL && nsPathPtr->creatorNsPtr != NULL) {
		nsPathPtr->creatorNsPtr->cmdRefEpoch++;
	    }
	    nsPathPtr->nsPtr = NULL;
	    nsPathPtr = nsPathPtr->nextPtr;
	} while (nsPathPtr != NULL);
	nsPtr->commandPathSourceList = NULL;
    }

    /*
     * Delete all the child namespaces.
     *
     * BE CAREFUL: When each child is deleted, it will divorce itself from its
     * parent. You can't traverse a hash table properly if its elements are
     * being deleted.  Because of traces (and the desire to avoid the
     * quadratic problems of just using Tcl_FirstHashEntry over and over, [Bug
     * f97d4ee020]) we copy to a temporary array and then delete all those
     * namespaces.
     *
     * Important: leave the hash table itself still live.
     */

#ifndef BREAK_NAMESPACE_COMPAT
    while (nsPtr->childTable.numEntries > 0) {
	int length = nsPtr->childTable.numEntries;
	Namespace **children = TclStackAlloc((Tcl_Interp *) iPtr,
		sizeof(Namespace *) * length);

	i = 0;
	for (entryPtr = Tcl_FirstHashEntry(&nsPtr->childTable, &search);
		entryPtr != NULL;
		entryPtr = Tcl_NextHashEntry(&search)) {
	    children[i] = Tcl_GetHashValue(entryPtr);
	    children[i]->refCount++;
	    i++;
	}
	for (i = 0 ; i < length ; i++) {
	    Tcl_DeleteNamespace((Tcl_Namespace *) children[i]);
	    TclNsDecrRefCount(children[i]);
	}
	TclStackFree((Tcl_Interp *) iPtr, children);
    }
#else
    if (nsPtr->childTablePtr != NULL) {
	while (nsPtr->childTablePtr->numEntries > 0) {
	    int length = nsPtr->childTablePtr->numEntries;
	    Namespace **children = TclStackAlloc((Tcl_Interp *) iPtr,
		    sizeof(Namespace *) * length);

	    i = 0;
	    for (entryPtr = Tcl_FirstHashEntry(nsPtr->childTablePtr, &search);
		    entryPtr != NULL;
		    entryPtr = Tcl_NextHashEntry(&search)) {
		children[i] = Tcl_GetHashValue(entryPtr);
		children[i]->refCount++;
		i++;
	    }
	    for (i = 0 ; i < length ; i++) {
		Tcl_DeleteNamespace((Tcl_Namespace *) children[i]);
		TclNsDecrRefCount(children[i]);
	    }
	    TclStackFree((Tcl_Interp *) iPtr, children);
	}
    }
#endif

    /*
     * Free the namespace's export pattern array.
     */

    if (nsPtr->exportArrayPtr != NULL) {
	for (i = 0;  i < nsPtr->numExportPatterns;  i++) {
	    ckfree(nsPtr->exportArrayPtr[i]);
	}
	ckfree(nsPtr->exportArrayPtr);
	nsPtr->exportArrayPtr = NULL;
	nsPtr->numExportPatterns = 0;
	nsPtr->maxExportPatterns = 0;
    }

    /*
     * Free any client data associated with the namespace.
     */

    if (nsPtr->deleteProc != NULL) {
	nsPtr->deleteProc(nsPtr->clientData);
    }
    nsPtr->deleteProc = NULL;
    nsPtr->clientData = NULL;

    /*
     * Reset the namespace's id field to ensure that this namespace won't be
     * interpreted as valid by, e.g., the cache validation code for cached
     * command references in Tcl_GetCommandFromObj.
     */

    nsPtr->nsId = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceFree --
 *
 *	Called after a namespace has been deleted, when its reference count
 *	reaches 0. Frees the data structure representing the namespace.
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
NamespaceFree(
    register Namespace *nsPtr)	/* Points to the namespace to free. */
{
    /*
     * Most of the namespace's contents are freed when the namespace is
     * deleted by Tcl_DeleteNamespace. All that remains is to free its names
     * (for error messages), and the structure itself.
     */

    ckfree(nsPtr->name);
    ckfree(nsPtr->fullName);
    ckfree(nsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclNsDecrRefCount --
 *
 *	Drops a reference to a namespace and frees it if the namespace has
 *	been deleted and the last reference has just been dropped.
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
TclNsDecrRefCount(
    Namespace *nsPtr)
{
    nsPtr->refCount--;
    if ((nsPtr->refCount == 0) && (nsPtr->flags & NS_DEAD)) {
	NamespaceFree(nsPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Export --
 *
 *	Makes all the commands matching a pattern available to later be
 *	imported from the namespace specified by namespacePtr (or the current
 *	namespace if namespacePtr is NULL). The specified pattern is appended
 *	onto the namespace's export pattern list, which is optionally cleared
 *	beforehand.
 *
 * Results:
 *	Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *	message in the interpreter's result) if something goes wrong.
 *
 * Side effects:
 *	Appends the export pattern onto the namespace's export list.
 *	Optionally reset the namespace's export pattern list.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Export(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Namespace *namespacePtr,/* Points to the namespace from which commands
				 * are to be exported. NULL for the current
				 * namespace. */
    const char *pattern,	/* String pattern indicating which commands to
				 * export. This pattern may not include any
				 * namespace qualifiers; only commands in the
				 * specified namespace may be exported. */
    int resetListFirst)		/* If nonzero, resets the namespace's export
				 * list before appending. */
{
#define INIT_EXPORT_PATTERNS 5
    Namespace *nsPtr, *exportNsPtr, *dummyPtr;
    Namespace *currNsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    const char *simplePattern;
    char *patternCpy;
    int neededElems, len, i;

    /*
     * If the specified namespace is NULL, use the current namespace.
     */

    if (namespacePtr == NULL) {
	nsPtr = (Namespace *) currNsPtr;
    } else {
	nsPtr = (Namespace *) namespacePtr;
    }

    /*
     * If resetListFirst is true (nonzero), clear the namespace's export
     * pattern list.
     */

    if (resetListFirst) {
	if (nsPtr->exportArrayPtr != NULL) {
	    for (i = 0;  i < nsPtr->numExportPatterns;  i++) {
		ckfree(nsPtr->exportArrayPtr[i]);
	    }
	    ckfree(nsPtr->exportArrayPtr);
	    nsPtr->exportArrayPtr = NULL;
	    TclInvalidateNsCmdLookup(nsPtr);
	    nsPtr->numExportPatterns = 0;
	    nsPtr->maxExportPatterns = 0;
	}
    }

    /*
     * Check that the pattern doesn't have namespace qualifiers.
     */

    TclGetNamespaceForQualName(interp, pattern, nsPtr, TCL_NAMESPACE_ONLY,
	    &exportNsPtr, &dummyPtr, &dummyPtr, &simplePattern);

    if ((exportNsPtr != nsPtr) || (strcmp(pattern, simplePattern) != 0)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid export pattern"
                " \"%s\": pattern can't specify a namespace", pattern));
	Tcl_SetErrorCode(interp, "TCL", "EXPORT", "INVALID", NULL);
	return TCL_ERROR;
    }

    /*
     * Make sure that we don't already have the pattern in the array
     */

    if (nsPtr->exportArrayPtr != NULL) {
	for (i = 0;  i < nsPtr->numExportPatterns;  i++) {
	    if (strcmp(pattern, nsPtr->exportArrayPtr[i]) == 0) {
		/*
		 * The pattern already exists in the list.
		 */

		return TCL_OK;
	    }
	}
    }

    /*
     * Make sure there is room in the namespace's pattern array for the new
     * pattern.
     */

    neededElems = nsPtr->numExportPatterns + 1;
    if (neededElems > nsPtr->maxExportPatterns) {
	nsPtr->maxExportPatterns = nsPtr->maxExportPatterns ?
		2 * nsPtr->maxExportPatterns : INIT_EXPORT_PATTERNS;
	nsPtr->exportArrayPtr = ckrealloc(nsPtr->exportArrayPtr,
		sizeof(char *) * nsPtr->maxExportPatterns);
    }

    /*
     * Add the pattern to the namespace's array of export patterns.
     */

    len = strlen(pattern);
    patternCpy = ckalloc(len + 1);
    memcpy(patternCpy, pattern, (unsigned) len + 1);

    nsPtr->exportArrayPtr[nsPtr->numExportPatterns] = patternCpy;
    nsPtr->numExportPatterns++;

    /*
     * The list of commands actually exported from the namespace might have
     * changed (probably will have!) However, we do not need to recompute this
     * just yet; next time we need the info will be soon enough.
     */

    TclInvalidateNsCmdLookup(nsPtr);

    return TCL_OK;
#undef INIT_EXPORT_PATTERNS
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendExportList --
 *
 *	Appends onto the argument object the list of export patterns for the
 *	specified namespace.
 *
 * Results:
 *	The return value is normally TCL_OK; in this case the object
 *	referenced by objPtr has each export pattern appended to it. If an
 *	error occurs, TCL_ERROR is returned and the interpreter's result holds
 *	an error message.
 *
 * Side effects:
 *	If necessary, the object referenced by objPtr is converted into a list
 *	object.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppendExportList(
    Tcl_Interp *interp,		/* Interpreter used for error reporting. */
    Tcl_Namespace *namespacePtr,/* Points to the namespace whose export
				 * pattern list is appended onto objPtr. NULL
				 * for the current namespace. */
    Tcl_Obj *objPtr)		/* Points to the Tcl object onto which the
				 * export pattern list is appended. */
{
    Namespace *nsPtr;
    int i, result;

    /*
     * If the specified namespace is NULL, use the current namespace.
     */

    if (namespacePtr == NULL) {
	nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    } else {
	nsPtr = (Namespace *) namespacePtr;
    }

    /*
     * Append the export pattern list onto objPtr.
     */

    for (i = 0;  i < nsPtr->numExportPatterns;  i++) {
	result = Tcl_ListObjAppendElement(interp, objPtr,
		Tcl_NewStringObj(nsPtr->exportArrayPtr[i], -1));
	if (result != TCL_OK) {
	    return result;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Import --
 *
 *	Imports all of the commands matching a pattern into the namespace
 *	specified by namespacePtr (or the current namespace if contextNsPtr is
 *	NULL). This is done by creating a new command (the "imported command")
 *	that points to the real command in its original namespace.
 *
 *	If matching commands are on the autoload path but haven't been loaded
 *	yet, this command forces them to be loaded, then creates the links to
 *	them.
 *
 * Results:
 *	Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *	message in the interpreter's result) if something goes wrong.
 *
 * Side effects:
 *	Creates new commands in the importing namespace. These indirect calls
 *	back to the real command and are deleted if the real commands are
 *	deleted.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Import(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Namespace *namespacePtr,/* Points to the namespace into which the
				 * commands are to be imported. NULL for the
				 * current namespace. */
    const char *pattern,	/* String pattern indicating which commands to
				 * import. This pattern should be qualified by
				 * the name of the namespace from which to
				 * import the command(s). */
    int allowOverwrite)		/* If nonzero, allow existing commands to be
				 * overwritten by imported commands. If 0,
				 * return an error if an imported cmd
				 * conflicts with an existing one. */
{
    Namespace *nsPtr, *importNsPtr, *dummyPtr;
    const char *simplePattern;
    register Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    /*
     * If the specified namespace is NULL, use the current namespace.
     */

    if (namespacePtr == NULL) {
	nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    } else {
	nsPtr = (Namespace *) namespacePtr;
    }

    /*
     * First, invoke the "auto_import" command with the pattern being
     * imported. This command is part of the Tcl library. It looks for
     * imported commands in autoloaded libraries and loads them in. That way,
     * they will be found when we try to create links below.
     *
     * Note that we don't just call Tcl_EvalObjv() directly because we do not
     * want absence of the command to be a failure case.
     */

    if (Tcl_FindCommand(interp,"auto_import",NULL,TCL_GLOBAL_ONLY) != NULL) {
	Tcl_Obj *objv[2];
	int result;

	TclNewLiteralStringObj(objv[0], "auto_import");
	objv[1] = Tcl_NewStringObj(pattern, -1);

	Tcl_IncrRefCount(objv[0]);
	Tcl_IncrRefCount(objv[1]);
	result = Tcl_EvalObjv(interp, 2, objv, TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(objv[0]);
	Tcl_DecrRefCount(objv[1]);

	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_ResetResult(interp);
    }

    /*
     * From the pattern, find the namespace from which we are importing and
     * get the simple pattern (no namespace qualifiers or ::'s) at the end.
     */

    if (strlen(pattern) == 0) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("empty import pattern",-1));
	Tcl_SetErrorCode(interp, "TCL", "IMPORT", "EMPTY", NULL);
	return TCL_ERROR;
    }
    TclGetNamespaceForQualName(interp, pattern, nsPtr, TCL_NAMESPACE_ONLY,
	    &importNsPtr, &dummyPtr, &dummyPtr, &simplePattern);

    if (importNsPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "unknown namespace in import pattern \"%s\"", pattern));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "NAMESPACE", pattern, NULL);
	return TCL_ERROR;
    }
    if (importNsPtr == nsPtr) {
	if (pattern == simplePattern) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "no namespace specified in import pattern \"%s\"",
                    pattern));
	    Tcl_SetErrorCode(interp, "TCL", "IMPORT", "ORIGIN", NULL);
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "import pattern \"%s\" tries to import from namespace"
                    " \"%s\" into itself", pattern, importNsPtr->name));
	    Tcl_SetErrorCode(interp, "TCL", "IMPORT", "SELF", NULL);
	}
	return TCL_ERROR;
    }

    /*
     * Scan through the command table in the source namespace and look for
     * exported commands that match the string pattern. Create an "imported
     * command" in the current namespace for each imported command; these
     * commands redirect their invocations to the "real" command.
     */

    if ((simplePattern != NULL) && TclMatchIsTrivial(simplePattern)) {
	hPtr = Tcl_FindHashEntry(&importNsPtr->cmdTable, simplePattern);
	if (hPtr == NULL) {
	    return TCL_OK;
	}
	return DoImport(interp, nsPtr, hPtr, simplePattern, pattern,
		importNsPtr, allowOverwrite);
    }
    for (hPtr = Tcl_FirstHashEntry(&importNsPtr->cmdTable, &search);
	    (hPtr != NULL); hPtr = Tcl_NextHashEntry(&search)) {
	char *cmdName = Tcl_GetHashKey(&importNsPtr->cmdTable, hPtr);

	if (Tcl_StringMatch(cmdName, simplePattern) &&
		DoImport(interp, nsPtr, hPtr, cmdName, pattern, importNsPtr,
		allowOverwrite) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DoImport --
 *
 *	Import a particular command from one namespace into another. Helper
 *	for Tcl_Import().
 *
 * Results:
 *	Standard Tcl result code. If TCL_ERROR, appends an error message to
 *	the interpreter result.
 *
 * Side effects:
 *	A new command is created in the target namespace unless this is a
 *	reimport of exactly the same command as before.
 *
 *----------------------------------------------------------------------
 */

static int
DoImport(
    Tcl_Interp *interp,
    Namespace *nsPtr,
    Tcl_HashEntry *hPtr,
    const char *cmdName,
    const char *pattern,
    Namespace *importNsPtr,
    int allowOverwrite)
{
    int i = 0, exported = 0;
    Tcl_HashEntry *found;

    /*
     * The command cmdName in the source namespace matches the pattern. Check
     * whether it was exported. If it wasn't, we ignore it.
     */

    while (!exported && (i < importNsPtr->numExportPatterns)) {
	exported |= Tcl_StringMatch(cmdName,
		importNsPtr->exportArrayPtr[i++]);
    }
    if (!exported) {
	return TCL_OK;
    }

    /*
     * Unless there is a name clash, create an imported command in the current
     * namespace that refers to cmdPtr.
     */

    found = Tcl_FindHashEntry(&nsPtr->cmdTable, cmdName);
    if ((found == NULL) || allowOverwrite) {
	/*
	 * Create the imported command and its client data. To create the new
	 * command in the current namespace, generate a fully qualified name
	 * for it.
	 */

	Tcl_DString ds;
	Tcl_Command importedCmd;
	ImportedCmdData *dataPtr;
	Command *cmdPtr;
	ImportRef *refPtr;

	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, nsPtr->fullName, -1);
	if (nsPtr != ((Interp *) interp)->globalNsPtr) {
	    TclDStringAppendLiteral(&ds, "::");
	}
	Tcl_DStringAppend(&ds, cmdName, -1);

	/*
	 * Check whether creating the new imported command in the current
	 * namespace would create a cycle of imported command references.
	 */

	cmdPtr = Tcl_GetHashValue(hPtr);
	if (found != NULL && cmdPtr->deleteProc == DeleteImportedCmd) {
	    Command *overwrite = Tcl_GetHashValue(found);
	    Command *linkCmd = cmdPtr;

	    while (linkCmd->deleteProc == DeleteImportedCmd) {
		dataPtr = linkCmd->objClientData;
		linkCmd = dataPtr->realCmdPtr;
		if (overwrite == linkCmd) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                            "import pattern \"%s\" would create a loop"
                            " containing command \"%s\"",
                            pattern, Tcl_DStringValue(&ds)));
		    Tcl_DStringFree(&ds);
		    Tcl_SetErrorCode(interp, "TCL", "IMPORT", "LOOP", NULL);
		    return TCL_ERROR;
		}
	    }
	}

	dataPtr = ckalloc(sizeof(ImportedCmdData));
	importedCmd = Tcl_NRCreateCommand(interp, Tcl_DStringValue(&ds),
		InvokeImportedCmd, InvokeImportedNRCmd, dataPtr,
		DeleteImportedCmd);
	dataPtr->realCmdPtr = cmdPtr;
	dataPtr->selfPtr = (Command *) importedCmd;
	dataPtr->selfPtr->compileProc = cmdPtr->compileProc;
	Tcl_DStringFree(&ds);

	/*
	 * Create an ImportRef structure describing this new import command
	 * and add it to the import ref list in the "real" command.
	 */

	refPtr = ckalloc(sizeof(ImportRef));
	refPtr->importedCmdPtr = (Command *) importedCmd;
	refPtr->nextPtr = cmdPtr->importRefPtr;
	cmdPtr->importRefPtr = refPtr;
    } else {
	Command *overwrite = Tcl_GetHashValue(found);

	if (overwrite->deleteProc == DeleteImportedCmd) {
	    ImportedCmdData *dataPtr = overwrite->objClientData;

	    if (dataPtr->realCmdPtr == Tcl_GetHashValue(hPtr)) {
		/*
		 * Repeated import of same command is acceptable.
		 */

		return TCL_OK;
	    }
	}
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can't import command \"%s\": already exists", cmdName));
	Tcl_SetErrorCode(interp, "TCL", "IMPORT", "OVERWRITE", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ForgetImport --
 *
 *	Deletes commands previously imported into the namespace indicated.
 *	The by namespacePtr, or the current namespace of interp, when
 *	namespacePtr is NULL. The pattern controls which imported commands are
 *	deleted. A simple pattern, one without namespace separators, matches
 *	the current command names of imported commands in the namespace.
 *	Matching imported commands are deleted. A qualified pattern is
 *	interpreted as deletion selection on the basis of where the command is
 *	imported from. The original command and "first link" command for each
 *	imported command are determined, and they are matched against the
 *	pattern. A match leads to deletion of the imported command.
 *
 * Results:
 *	Returns TCL_ERROR and records an error message in the interp result if
 *	a namespace qualified pattern refers to a namespace that does not
 *	exist. Otherwise, returns TCL_OK.
 *
 * Side effects:
 *	May delete commands.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ForgetImport(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Namespace *namespacePtr,/* Points to the namespace from which
				 * previously imported commands should be
				 * removed. NULL for current namespace. */
    const char *pattern)	/* String pattern indicating which imported
				 * commands to remove. */
{
    Namespace *nsPtr, *sourceNsPtr, *dummyPtr;
    const char *simplePattern;
    char *cmdName;
    register Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    /*
     * If the specified namespace is NULL, use the current namespace.
     */

    if (namespacePtr == NULL) {
	nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    } else {
	nsPtr = (Namespace *) namespacePtr;
    }

    /*
     * Parse the pattern into its namespace-qualification (if any) and the
     * simple pattern.
     */

    TclGetNamespaceForQualName(interp, pattern, nsPtr, TCL_NAMESPACE_ONLY,
	    &sourceNsPtr, &dummyPtr, &dummyPtr, &simplePattern);

    if (sourceNsPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown namespace in namespace forget pattern \"%s\"",
		pattern));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "NAMESPACE", pattern, NULL);
	return TCL_ERROR;
    }

    if (strcmp(pattern, simplePattern) == 0) {
	/*
	 * The pattern is simple. Delete any imported commands that match it.
	 */

	if (TclMatchIsTrivial(simplePattern)) {
	    hPtr = Tcl_FindHashEntry(&nsPtr->cmdTable, simplePattern);
	    if (hPtr != NULL) {
		Command *cmdPtr = Tcl_GetHashValue(hPtr);

		if (cmdPtr && (cmdPtr->deleteProc == DeleteImportedCmd)) {
		    Tcl_DeleteCommandFromToken(interp, (Tcl_Command) cmdPtr);
		}
	    }
	    return TCL_OK;
	}
	for (hPtr = Tcl_FirstHashEntry(&nsPtr->cmdTable, &search);
		(hPtr != NULL); hPtr = Tcl_NextHashEntry(&search)) {
	    Command *cmdPtr = Tcl_GetHashValue(hPtr);

	    if (cmdPtr->deleteProc != DeleteImportedCmd) {
		continue;
	    }
	    cmdName = Tcl_GetHashKey(&nsPtr->cmdTable, hPtr);
	    if (Tcl_StringMatch(cmdName, simplePattern)) {
		Tcl_DeleteCommandFromToken(interp, (Tcl_Command) cmdPtr);
	    }
	}
	return TCL_OK;
    }

    /*
     * The pattern was namespace-qualified.
     */

    for (hPtr = Tcl_FirstHashEntry(&nsPtr->cmdTable, &search); (hPtr != NULL);
	    hPtr = Tcl_NextHashEntry(&search)) {
	Tcl_CmdInfo info;
	Tcl_Command token = Tcl_GetHashValue(hPtr);
	Tcl_Command origin = TclGetOriginalCommand(token);

	if (Tcl_GetCommandInfoFromToken(origin, &info) == 0) {
	    continue;			/* Not an imported command. */
	}
	if (info.namespacePtr != (Tcl_Namespace *) sourceNsPtr) {
	    /*
	     * Original not in namespace we're matching. Check the first link
	     * in the import chain.
	     */

	    Command *cmdPtr = (Command *) token;
	    ImportedCmdData *dataPtr = cmdPtr->objClientData;
	    Tcl_Command firstToken = (Tcl_Command) dataPtr->realCmdPtr;

	    if (firstToken == origin) {
		continue;
	    }
	    Tcl_GetCommandInfoFromToken(firstToken, &info);
	    if (info.namespacePtr != (Tcl_Namespace *) sourceNsPtr) {
		continue;
	    }
	    origin = firstToken;
	}
	if (Tcl_StringMatch(Tcl_GetCommandName(NULL, origin), simplePattern)){
	    Tcl_DeleteCommandFromToken(interp, token);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetOriginalCommand --
 *
 *	An imported command is created in an namespace when a "real" command
 *	is imported from another namespace. If the specified command is an
 *	imported command, this function returns the original command it refers
 *	to.
 *
 * Results:
 *	If the command was imported into a sequence of namespaces a, b,...,n
 *	where each successive namespace just imports the command from the
 *	previous namespace, this function returns the Tcl_Command token in the
 *	first namespace, a. Otherwise, if the specified command is not an
 *	imported command, the function returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
TclGetOriginalCommand(
    Tcl_Command command)	/* The imported command for which the original
				 * command should be returned. */
{
    register Command *cmdPtr = (Command *) command;
    ImportedCmdData *dataPtr;

    if (cmdPtr->deleteProc != DeleteImportedCmd) {
	return NULL;
    }

    while (cmdPtr->deleteProc == DeleteImportedCmd) {
	dataPtr = cmdPtr->objClientData;
	cmdPtr = dataPtr->realCmdPtr;
    }
    return (Tcl_Command) cmdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * InvokeImportedCmd --
 *
 *	Invoked by Tcl whenever the user calls an imported command that was
 *	created by Tcl_Import. Finds the "real" command (in another
 *	namespace), and passes control to it.
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result object is set to an error message.
 *
 *----------------------------------------------------------------------
 */

static int
InvokeImportedNRCmd(
    ClientData clientData,	/* Points to the imported command's
				 * ImportedCmdData structure. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* The argument objects. */
{
    ImportedCmdData *dataPtr = clientData;
    Command *realCmdPtr = dataPtr->realCmdPtr;

    TclSkipTailcall(interp);
    return TclNREvalObjv(interp, objc, objv, TCL_EVAL_NOERR, realCmdPtr);
}

static int
InvokeImportedCmd(
    ClientData clientData,	/* Points to the imported command's
				 * ImportedCmdData structure. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* The argument objects. */
{
    return Tcl_NRCallObjProc(interp, InvokeImportedNRCmd, clientData,
	    objc, objv);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteImportedCmd --
 *
 *	Invoked by Tcl whenever an imported command is deleted. The "real"
 *	command keeps a list of all the imported commands that refer to it, so
 *	those imported commands can be deleted when the real command is
 *	deleted. This function removes the imported command reference from the
 *	real command's list, and frees up the memory associated with the
 *	imported command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the imported command from the real command's import list.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteImportedCmd(
    ClientData clientData)	/* Points to the imported command's
				 * ImportedCmdData structure. */
{
    ImportedCmdData *dataPtr = clientData;
    Command *realCmdPtr = dataPtr->realCmdPtr;
    Command *selfPtr = dataPtr->selfPtr;
    register ImportRef *refPtr, *prevPtr;

    prevPtr = NULL;
    for (refPtr = realCmdPtr->importRefPtr; refPtr != NULL;
	    refPtr = refPtr->nextPtr) {
	if (refPtr->importedCmdPtr == selfPtr) {
	    /*
	     * Remove *refPtr from real command's list of imported commands
	     * that refer to it.
	     */

	    if (prevPtr == NULL) { /* refPtr is first in list. */
		realCmdPtr->importRefPtr = refPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = refPtr->nextPtr;
	    }
	    ckfree(refPtr);
	    ckfree(dataPtr);
	    return;
	}
	prevPtr = refPtr;
    }

    Tcl_Panic("DeleteImportedCmd: did not find cmd in real cmd's list of import references");
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetNamespaceForQualName --
 *
 *	Given a qualified name specifying a command, variable, or namespace,
 *	and a namespace in which to resolve the name, this function returns a
 *	pointer to the namespace that contains the item. A qualified name
 *	consists of the "simple" name of an item qualified by the names of an
 *	arbitrary number of containing namespace separated by "::"s. If the
 *	qualified name starts with "::", it is interpreted absolutely from the
 *	global namespace. Otherwise, it is interpreted relative to the
 *	namespace specified by cxtNsPtr if it is non-NULL. If cxtNsPtr is
 *	NULL, the name is interpreted relative to the current namespace.
 *
 *	A relative name like "foo::bar::x" can be found starting in either the
 *	current namespace or in the global namespace. So each search usually
 *	follows two tracks, and two possible namespaces are returned. If the
 *	function sets either *nsPtrPtr or *altNsPtrPtr to NULL, then that path
 *	failed.
 *
 *	If "flags" contains TCL_GLOBAL_ONLY, the relative qualified name is
 *	sought only in the global :: namespace. The alternate search (also)
 *	starting from the global namespace is ignored and *altNsPtrPtr is set
 *	NULL.
 *
 *	If "flags" contains TCL_NAMESPACE_ONLY, the relative qualified name is
 *	sought only in the namespace specified by cxtNsPtr. The alternate
 *	search starting from the global namespace is ignored and *altNsPtrPtr
 *	is set NULL. If both TCL_GLOBAL_ONLY and TCL_NAMESPACE_ONLY are
 *	specified, TCL_GLOBAL_ONLY is ignored and the search starts from the
 *	namespace specified by cxtNsPtr.
 *
 *	If "flags" contains TCL_CREATE_NS_IF_UNKNOWN, all namespace components
 *	of the qualified name that cannot be found are automatically created
 *	within their specified parent. This makes sure that functions like
 *	Tcl_CreateCommand always succeed. There is no alternate search path,
 *	so *altNsPtrPtr is set NULL.
 *
 *	If "flags" contains TCL_FIND_ONLY_NS, the qualified name is treated as
 *	a reference to a namespace, and the entire qualified name is followed.
 *	If the name is relative, the namespace is looked up only in the
 *	current namespace. A pointer to the namespace is stored in *nsPtrPtr
 *	and NULL is stored in *simpleNamePtr. Otherwise, if TCL_FIND_ONLY_NS
 *	is not specified, only the leading components are treated as namespace
 *	names, and a pointer to the simple name of the final component is
 *	stored in *simpleNamePtr.
 *
 * Results:
 *	It sets *nsPtrPtr and *altNsPtrPtr to point to the two possible
 *	namespaces which represent the last (containing) namespace in the
 *	qualified name. If the function sets either *nsPtrPtr or *altNsPtrPtr
 *	to NULL, then the search along that path failed. The function also
 *	stores a pointer to the simple name of the final component in
 *	*simpleNamePtr. If the qualified name is "::" or was treated as a
 *	namespace reference (TCL_FIND_ONLY_NS), the function stores a pointer
 *	to the namespace in *nsPtrPtr, NULL in *altNsPtrPtr, and sets
 *	*simpleNamePtr to point to an empty string.
 *
 *	If there is an error, this function returns TCL_ERROR. If "flags"
 *	contains TCL_LEAVE_ERR_MSG, an error message is returned in the
 *	interpreter's result object. Otherwise, the interpreter's result
 *	object is left unchanged.
 *
 *	*actualCxtPtrPtr is set to the actual context namespace. It is set to
 *	the input context namespace pointer in cxtNsPtr. If cxtNsPtr is NULL,
 *	it is set to the current namespace context.
 *
 *	For backwards compatibility with the TclPro byte code loader, this
 *	function always returns TCL_OK.
 *
 * Side effects:
 *	If "flags" contains TCL_CREATE_NS_IF_UNKNOWN, new namespaces may be
 *	created.
 *
 *----------------------------------------------------------------------
 */

int
TclGetNamespaceForQualName(
    Tcl_Interp *interp,		/* Interpreter in which to find the namespace
				 * containing qualName. */
    const char *qualName,	/* A namespace-qualified name of an command,
				 * variable, or namespace. */
    Namespace *cxtNsPtr,	/* The namespace in which to start the search
				 * for qualName's namespace. If NULL start
				 * from the current namespace. Ignored if
				 * TCL_GLOBAL_ONLY is set. */
    int flags,			/* Flags controlling the search: an OR'd
				 * combination of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_FIND_ONLY_NS, and
				 * TCL_CREATE_NS_IF_UNKNOWN. */
    Namespace **nsPtrPtr,	/* Address where function stores a pointer to
				 * containing namespace if qualName is found
				 * starting from *cxtNsPtr or, if
				 * TCL_GLOBAL_ONLY is set, if qualName is
				 * found in the global :: namespace. NULL is
				 * stored otherwise. */
    Namespace **altNsPtrPtr,	/* Address where function stores a pointer to
				 * containing namespace if qualName is found
				 * starting from the global :: namespace.
				 * NULL is stored if qualName isn't found
				 * starting from :: or if the TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY, TCL_FIND_ONLY_NS,
				 * TCL_CREATE_NS_IF_UNKNOWN flag is set. */
    Namespace **actualCxtPtrPtr,/* Address where function stores a pointer to
				 * the actual namespace from which the search
				 * started. This is either cxtNsPtr, the ::
				 * namespace if TCL_GLOBAL_ONLY was specified,
				 * or the current namespace if cxtNsPtr was
				 * NULL. */
    const char **simpleNamePtr) /* Address where function stores the simple
				 * name at end of the qualName, or NULL if
				 * qualName is "::" or the flag
				 * TCL_FIND_ONLY_NS was specified. */
{
    Interp *iPtr = (Interp *) interp;
    Namespace *nsPtr = cxtNsPtr;
    Namespace *altNsPtr;
    Namespace *globalNsPtr = iPtr->globalNsPtr;
    const char *start, *end;
    const char *nsName;
    Tcl_HashEntry *entryPtr;
    Tcl_DString buffer;
    int len;

    /*
     * Determine the context namespace nsPtr in which to start the primary
     * search. If the qualName name starts with a "::" or TCL_GLOBAL_ONLY was
     * specified, search from the global namespace. Otherwise, use the
     * namespace given in cxtNsPtr, or if that is NULL, use the current
     * namespace context. Note that we always treat two or more adjacent ":"s
     * as a namespace separator.
     */

    if (flags & TCL_GLOBAL_ONLY) {
	nsPtr = globalNsPtr;
    } else if (nsPtr == NULL) {
	nsPtr = iPtr->varFramePtr->nsPtr;
    }

    start = qualName;			/* Points to start of qualifying
					 * namespace. */
    if ((*qualName == ':') && (*(qualName+1) == ':')) {
	start = qualName+2;		/* Skip over the initial :: */
	while (*start == ':') {
	    start++;			/* Skip over a subsequent : */
	}
	nsPtr = globalNsPtr;
	if (*start == '\0') {		/* qualName is just two or more
					 * ":"s. */
	    *nsPtrPtr = globalNsPtr;
	    *altNsPtrPtr = NULL;
	    *actualCxtPtrPtr = globalNsPtr;
	    *simpleNamePtr = start;	/* Points to empty string. */
	    return TCL_OK;
	}
    }
    *actualCxtPtrPtr = nsPtr;

    /*
     * Start an alternate search path starting with the global namespace.
     * However, if the starting context is the global namespace, or if the
     * flag is set to search only the namespace *cxtNsPtr, ignore the
     * alternate search path.
     */

    altNsPtr = globalNsPtr;
    if ((nsPtr == globalNsPtr)
	    || (flags & (TCL_NAMESPACE_ONLY | TCL_FIND_ONLY_NS))) {
	altNsPtr = NULL;
    }

    /*
     * Loop to resolve each namespace qualifier in qualName.
     */

    Tcl_DStringInit(&buffer);
    end = start;
    while (*start != '\0') {
	/*
	 * Find the next namespace qualifier (i.e., a name ending in "::") or
	 * the end of the qualified name (i.e., a name ending in "\0"). Set
	 * len to the number of characters, starting from start, in the name;
	 * set end to point after the "::"s or at the "\0".
	 */

	len = 0;
	for (end = start;  *end != '\0';  end++) {
	    if ((*end == ':') && (*(end+1) == ':')) {
		end += 2;		/* Skip over the initial :: */
		while (*end == ':') {
		    end++;		/* Skip over the subsequent : */
		}
		break;			/* Exit for loop; end is after ::'s */
	    }
	    len++;
	}

	if (*end=='\0' && !(end-start>=2 && *(end-1)==':' && *(end-2)==':')) {
	    /*
	     * qualName ended with a simple name at start. If TCL_FIND_ONLY_NS
	     * was specified, look this up as a namespace. Otherwise, start is
	     * the name of a cmd or var and we are done.
	     */

	    if (flags & TCL_FIND_ONLY_NS) {
		nsName = start;
	    } else {
		*nsPtrPtr = nsPtr;
		*altNsPtrPtr = altNsPtr;
		*simpleNamePtr = start;
		Tcl_DStringFree(&buffer);
		return TCL_OK;
	    }
	} else {
	    /*
	     * start points to the beginning of a namespace qualifier ending
	     * in "::". end points to the start of a name in that namespace
	     * that might be empty. Copy the namespace qualifier to a buffer
	     * so it can be null terminated. We can't modify the incoming
	     * qualName since it may be a string constant.
	     */

	    TclDStringClear(&buffer);
	    Tcl_DStringAppend(&buffer, start, len);
	    nsName = Tcl_DStringValue(&buffer);
	}

	/*
	 * Look up the namespace qualifier nsName in the current namespace
	 * context. If it isn't found but TCL_CREATE_NS_IF_UNKNOWN is set,
	 * create that qualifying namespace. This is needed for functions like
	 * Tcl_CreateCommand that cannot fail.
	 */

	if (nsPtr != NULL) {
#ifndef BREAK_NAMESPACE_COMPAT
	    entryPtr = Tcl_FindHashEntry(&nsPtr->childTable, nsName);
#else
	    if (nsPtr->childTablePtr == NULL) {
		entryPtr = NULL;
	    } else {
		entryPtr = Tcl_FindHashEntry(nsPtr->childTablePtr, nsName);
	    }
#endif
	    if (entryPtr != NULL) {
		nsPtr = Tcl_GetHashValue(entryPtr);
	    } else if (flags & TCL_CREATE_NS_IF_UNKNOWN) {
		Tcl_CallFrame *framePtr;

		(void) TclPushStackFrame(interp, &framePtr,
			(Tcl_Namespace *) nsPtr, /*isProcCallFrame*/ 0);

		nsPtr = (Namespace *)
			Tcl_CreateNamespace(interp, nsName, NULL, NULL);
		TclPopStackFrame(interp);

		if (nsPtr == NULL) {
		    Tcl_Panic("Could not create namespace '%s'", nsName);
		}
	    } else {			/* Namespace not found and was not
					 * created. */
		nsPtr = NULL;
	    }
	}

	/*
	 * Look up the namespace qualifier in the alternate search path too.
	 */

	if (altNsPtr != NULL) {
#ifndef BREAK_NAMESPACE_COMPAT
	    entryPtr = Tcl_FindHashEntry(&altNsPtr->childTable, nsName);
#else
	    if (altNsPtr->childTablePtr != NULL) {
		entryPtr = Tcl_FindHashEntry(altNsPtr->childTablePtr, nsName);
	    } else {
		entryPtr = NULL;
	    }
#endif
	    if (entryPtr != NULL) {
		altNsPtr = Tcl_GetHashValue(entryPtr);
	    } else {
		altNsPtr = NULL;
	    }
	}

	/*
	 * If both search paths have failed, return NULL results.
	 */

	if ((nsPtr == NULL) && (altNsPtr == NULL)) {
	    *nsPtrPtr = NULL;
	    *altNsPtrPtr = NULL;
	    *simpleNamePtr = NULL;
	    Tcl_DStringFree(&buffer);
	    return TCL_OK;
	}

	start = end;
    }

    /*
     * We ignore trailing "::"s in a namespace name, but in a command or
     * variable name, trailing "::"s refer to the cmd or var named {}.
     */

    if ((flags & TCL_FIND_ONLY_NS) || (end>start && *(end-1)!=':')) {
	*simpleNamePtr = NULL;		/* Found namespace name. */
    } else {
	*simpleNamePtr = end;		/* Found cmd/var: points to empty
					 * string. */
    }

    /*
     * As a special case, if we are looking for a namespace and qualName is ""
     * and the current active namespace (nsPtr) is not the global namespace,
     * return NULL (no namespace was found). This is because namespaces can
     * not have empty names except for the global namespace.
     */

    if ((flags & TCL_FIND_ONLY_NS) && (*qualName == '\0')
	    && (nsPtr != globalNsPtr)) {
	nsPtr = NULL;
    }

    *nsPtrPtr = nsPtr;
    *altNsPtrPtr = altNsPtr;
    Tcl_DStringFree(&buffer);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclEnsureNamespace --
 *
 *	Provide a namespace that is not deleted.
 *
 * Value
 *
 *	namespacePtr, if it is not scheduled for deletion, or a pointer to a
 *	new namespace with the same name otherwise.
 *
 * Effect
 *	None.
 *
 *----------------------------------------------------------------------
 */
Tcl_Namespace *
TclEnsureNamespace(
    Tcl_Interp *interp,
    Tcl_Namespace *namespacePtr)
{
    Namespace *nsPtr = (Namespace *) namespacePtr;
    if (!(nsPtr->flags & NS_DYING)) {
	    return namespacePtr;
    }
    return Tcl_CreateNamespace(interp, nsPtr->fullName, NULL, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FindNamespace --
 *
 *	Searches for a namespace.
 *
 * Results:
 *	Returns a pointer to the namespace if it is found. Otherwise, returns
 *	NULL and leaves an error message in the interpreter's result object if
 *	"flags" contains TCL_LEAVE_ERR_MSG.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Namespace *
Tcl_FindNamespace(
    Tcl_Interp *interp,		/* The interpreter in which to find the
				 * namespace. */
    const char *name,		/* Namespace name. If it starts with "::",
				 * will be looked up in global namespace.
				 * Else, looked up first in contextNsPtr
				 * (current namespace if contextNsPtr is
				 * NULL), then in global namespace. */
    Tcl_Namespace *contextNsPtr,/* Ignored if TCL_GLOBAL_ONLY flag is set or
				 * if the name starts with "::". Otherwise,
				 * points to namespace in which to resolve
				 * name; if NULL, look up name in the current
				 * namespace. */
    register int flags)		/* Flags controlling namespace lookup: an OR'd
				 * combination of TCL_GLOBAL_ONLY and
				 * TCL_LEAVE_ERR_MSG flags. */
{
    Namespace *nsPtr, *dummy1Ptr, *dummy2Ptr;
    const char *dummy;

    /*
     * Find the namespace(s) that contain the specified namespace name. Add
     * the TCL_FIND_ONLY_NS flag to resolve the name all the way down to its
     * last component, a namespace.
     */

    TclGetNamespaceForQualName(interp, name, (Namespace *) contextNsPtr,
	    flags|TCL_FIND_ONLY_NS, &nsPtr, &dummy1Ptr, &dummy2Ptr, &dummy);

    if (nsPtr != NULL) {
	return (Tcl_Namespace *) nsPtr;
    }

    if (flags & TCL_LEAVE_ERR_MSG) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "unknown namespace \"%s\"", name));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "NAMESPACE", name, NULL);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FindCommand --
 *
 *	Searches for a command.
 *
 * Results:
 *	Returns a token for the command if it is found. Otherwise, if it can't
 *	be found or there is an error, returns NULL and leaves an error
 *	message in the interpreter's result object if "flags" contains
 *	TCL_LEAVE_ERR_MSG.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
Tcl_FindCommand(
    Tcl_Interp *interp,		/* The interpreter in which to find the
				 * command and to report errors. */
    const char *name,		/* Command's name. If it starts with "::",
				 * will be looked up in global namespace.
				 * Else, looked up first in contextNsPtr
				 * (current namespace if contextNsPtr is
				 * NULL), then in global namespace. */
    Tcl_Namespace *contextNsPtr,/* Ignored if TCL_GLOBAL_ONLY flag set.
				 * Otherwise, points to namespace in which to
				 * resolve name. If NULL, look up name in the
				 * current namespace. */
    int flags)			/* An OR'd combination of flags:
				 * TCL_GLOBAL_ONLY (look up name only in
				 * global namespace), TCL_NAMESPACE_ONLY (look
				 * up only in contextNsPtr, or the current
				 * namespace if contextNsPtr is NULL), and
				 * TCL_LEAVE_ERR_MSG. If both TCL_GLOBAL_ONLY
				 * and TCL_NAMESPACE_ONLY are given,
				 * TCL_GLOBAL_ONLY is ignored. */
{
    Interp *iPtr = (Interp *) interp;
    Namespace *cxtNsPtr;
    register Tcl_HashEntry *entryPtr;
    register Command *cmdPtr;
    const char *simpleName;
    int result;

    /*
     * If this namespace has a command resolver, then give it first crack at
     * the command resolution. If the interpreter has any command resolvers,
     * consult them next. The command resolver functions may return a
     * Tcl_Command value, they may signal to continue onward, or they may
     * signal an error.
     */

    if ((flags & TCL_GLOBAL_ONLY) || !strncmp(name, "::", 2)) {
	cxtNsPtr = (Namespace *) TclGetGlobalNamespace(interp);
    } else if (contextNsPtr != NULL) {
	cxtNsPtr = (Namespace *) contextNsPtr;
    } else {
	cxtNsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    }

    if (cxtNsPtr->cmdResProc != NULL || iPtr->resolverPtr != NULL) {
	ResolverScheme *resPtr = iPtr->resolverPtr;
	Tcl_Command cmd;

	if (cxtNsPtr->cmdResProc) {
	    result = cxtNsPtr->cmdResProc(interp, name,
		    (Tcl_Namespace *) cxtNsPtr, flags, &cmd);
	} else {
	    result = TCL_CONTINUE;
	}

	while (result == TCL_CONTINUE && resPtr) {
	    if (resPtr->cmdResProc) {
		result = resPtr->cmdResProc(interp, name,
			(Tcl_Namespace *) cxtNsPtr, flags, &cmd);
	    }
	    resPtr = resPtr->nextPtr;
	}

	if (result == TCL_OK) {
	    ((Command *)cmd)->flags |= CMD_VIA_RESOLVER;
	    return cmd;

	} else if (result != TCL_CONTINUE) {
	    return NULL;
	}
    }

    /*
     * Find the namespace(s) that contain the command.
     */

    cmdPtr = NULL;
    if (cxtNsPtr->commandPathLength!=0 && strncmp(name, "::", 2)
	    && !(flags & TCL_NAMESPACE_ONLY)) {
	int i;
	Namespace *pathNsPtr, *realNsPtr, *dummyNsPtr;

	(void) TclGetNamespaceForQualName(interp, name, cxtNsPtr,
		TCL_NAMESPACE_ONLY, &realNsPtr, &dummyNsPtr, &dummyNsPtr,
		&simpleName);
	if ((realNsPtr != NULL) && (simpleName != NULL)) {
	    if ((cxtNsPtr == realNsPtr)
		    || !(realNsPtr->flags & NS_DYING)) {
		entryPtr = Tcl_FindHashEntry(&realNsPtr->cmdTable, simpleName);
		if (entryPtr != NULL) {
		    cmdPtr = Tcl_GetHashValue(entryPtr);
		}
	    }
	}

	/*
	 * Next, check along the path.
	 */

	for (i=0 ; i<cxtNsPtr->commandPathLength && cmdPtr==NULL ; i++) {
	    pathNsPtr = cxtNsPtr->commandPathArray[i].nsPtr;
	    if (pathNsPtr == NULL) {
		continue;
	    }
	    (void) TclGetNamespaceForQualName(interp, name, pathNsPtr,
		    TCL_NAMESPACE_ONLY, &realNsPtr, &dummyNsPtr, &dummyNsPtr,
		    &simpleName);
	    if ((realNsPtr != NULL) && (simpleName != NULL)
		    && !(realNsPtr->flags & NS_DYING)) {
		entryPtr = Tcl_FindHashEntry(&realNsPtr->cmdTable, simpleName);
		if (entryPtr != NULL) {
		    cmdPtr = Tcl_GetHashValue(entryPtr);
		}
	    }
	}

	/*
	 * If we've still not found the command, look in the global namespace
	 * as a last resort.
	 */

	if (cmdPtr == NULL) {
	    (void) TclGetNamespaceForQualName(interp, name, NULL,
		    TCL_GLOBAL_ONLY, &realNsPtr, &dummyNsPtr, &dummyNsPtr,
		    &simpleName);
	    if ((realNsPtr != NULL) && (simpleName != NULL)
		    && !(realNsPtr->flags & NS_DYING)) {
		entryPtr = Tcl_FindHashEntry(&realNsPtr->cmdTable, simpleName);
		if (entryPtr != NULL) {
		    cmdPtr = Tcl_GetHashValue(entryPtr);
		}
	    }
	}
    } else {
	Namespace *nsPtr[2];
	register int search;

	TclGetNamespaceForQualName(interp, name, cxtNsPtr,
		flags, &nsPtr[0], &nsPtr[1], &cxtNsPtr, &simpleName);

	/*
	 * Look for the command in the command table of its namespace. Be sure
	 * to check both possible search paths: from the specified namespace
	 * context and from the global namespace.
	 */

	for (search = 0;  (search < 2) && (cmdPtr == NULL);  search++) {
	    if ((nsPtr[search] != NULL) && (simpleName != NULL)) {
		entryPtr = Tcl_FindHashEntry(&nsPtr[search]->cmdTable,
			simpleName);
		if (entryPtr != NULL) {
		    cmdPtr = Tcl_GetHashValue(entryPtr);
		}
	    }
	}
    }

    if (cmdPtr != NULL) {
	cmdPtr->flags  &= ~CMD_VIA_RESOLVER;
	return (Tcl_Command) cmdPtr;
    }

    if (flags & TCL_LEAVE_ERR_MSG) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "unknown command \"%s\"", name));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "COMMAND", name, NULL);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclResetShadowedCmdRefs --
 *
 *	Called when a command is added to a namespace to check for existing
 *	command references that the new command may invalidate. Consider the
 *	following cases that could happen when you add a command "foo" to a
 *	namespace "b":
 *	   1. It could shadow a command named "foo" at the global scope. If
 *	      it does, all command references in the namespace "b" are
 *	      suspect.
 *	   2. Suppose the namespace "b" resides in a namespace "a". Then to
 *	      "a" the new command "b::foo" could shadow another command
 *	      "b::foo" in the global namespace. If so, then all command
 *	      references in "a" * are suspect.
 *	The same checks are applied to all parent namespaces, until we reach
 *	the global :: namespace.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the new command shadows an existing command, the cmdRefEpoch
 *	counter is incremented in each namespace that sees the shadow. This
 *	invalidates all command references that were previously cached in that
 *	namespace. The next time the commands are used, they are resolved from
 *	scratch.
 *
 *----------------------------------------------------------------------
 */

void
TclResetShadowedCmdRefs(
    Tcl_Interp *interp,		/* Interpreter containing the new command. */
    Command *newCmdPtr)		/* Points to the new command. */
{
    char *cmdName;
    Tcl_HashEntry *hPtr;
    register Namespace *nsPtr;
    Namespace *trailNsPtr, *shadowNsPtr;
    Namespace *globalNsPtr = (Namespace *) TclGetGlobalNamespace(interp);
    int found, i;
    int trailFront = -1;
    int trailSize = 5;		/* Formerly NUM_TRAIL_ELEMS. */
    Namespace **trailPtr = TclStackAlloc(interp,
	    trailSize * sizeof(Namespace *));

    /*
     * Start at the namespace containing the new command, and work up through
     * the list of parents. Stop just before the global namespace, since the
     * global namespace can't "shadow" its own entries.
     *
     * The namespace "trail" list we build consists of the names of each
     * namespace that encloses the new command, in order from outermost to
     * innermost: for example, "a" then "b". Each iteration of this loop
     * eventually extends the trail upwards by one namespace, nsPtr. We use
     * this trail list to see if nsPtr (e.g. "a" in 2. above) could have
     * now-invalid cached command references. This will happen if nsPtr
     * (e.g. "a") contains a sequence of child namespaces (e.g. "b") such that
     * there is a identically-named sequence of child namespaces starting from
     * :: (e.g. "::b") whose tail namespace contains a command also named
     * cmdName.
     */

    cmdName = Tcl_GetHashKey(newCmdPtr->hPtr->tablePtr, newCmdPtr->hPtr);
    for (nsPtr=newCmdPtr->nsPtr ; (nsPtr!=NULL) && (nsPtr!=globalNsPtr) ;
	    nsPtr=nsPtr->parentPtr) {
	/*
	 * Find the maximal sequence of child namespaces contained in nsPtr
	 * such that there is a identically-named sequence of child namespaces
	 * starting from ::. shadowNsPtr will be the tail of this sequence, or
	 * the deepest namespace under :: that might contain a command now
	 * shadowed by cmdName. We check below if shadowNsPtr actually
	 * contains a command cmdName.
	 */

	found = 1;
	shadowNsPtr = globalNsPtr;

	for (i = trailFront;  i >= 0;  i--) {
	    trailNsPtr = trailPtr[i];
#ifndef BREAK_NAMESPACE_COMPAT
	    hPtr = Tcl_FindHashEntry(&shadowNsPtr->childTable,
		    trailNsPtr->name);
#else
	    if (shadowNsPtr->childTablePtr != NULL) {
		hPtr = Tcl_FindHashEntry(shadowNsPtr->childTablePtr,
			trailNsPtr->name);
	    } else {
		hPtr = NULL;
	    }
#endif
	    if (hPtr != NULL) {
		shadowNsPtr = Tcl_GetHashValue(hPtr);
	    } else {
		found = 0;
		break;
	    }
	}

	/*
	 * If shadowNsPtr contains a command named cmdName, we invalidate all
	 * of the command refs cached in nsPtr. As a boundary case,
	 * shadowNsPtr is initially :: and we check for case 1. above.
	 */

	if (found) {
	    hPtr = Tcl_FindHashEntry(&shadowNsPtr->cmdTable, cmdName);
	    if (hPtr != NULL) {
		nsPtr->cmdRefEpoch++;
		TclInvalidateNsPath(nsPtr);

		/*
		 * If the shadowed command was compiled to bytecodes, we
		 * invalidate all the bytecodes in nsPtr, to force a new
		 * compilation. We use the resolverEpoch to signal the need
		 * for a fresh compilation of every bytecode.
		 */

		if (((Command *)Tcl_GetHashValue(hPtr))->compileProc != NULL){
		    nsPtr->resolverEpoch++;
		}
	    }
	}

	/*
	 * Insert nsPtr at the front of the trail list: i.e., at the end of
	 * the trailPtr array.
	 */

	trailFront++;
	if (trailFront == trailSize) {
	    int newSize = 2 * trailSize;

	    trailPtr = TclStackRealloc(interp, trailPtr,
		    newSize * sizeof(Namespace *));
	    trailSize = newSize;
	}
	trailPtr[trailFront] = nsPtr;
    }
    TclStackFree(interp, trailPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetNamespaceFromObj, GetNamespaceFromObj --
 *
 *	Gets the namespace specified by the name in a Tcl_Obj.
 *
 * Results:
 *	Returns TCL_OK if the namespace was resolved successfully, and stores
 *	a pointer to the namespace in the location specified by nsPtrPtr. If
 *	the namespace can't be found, or anything else goes wrong, this
 *	function returns TCL_ERROR and writes an error message to interp,
 *	if non-NULL.
 *
 * Side effects:
 *	May update the internal representation for the object, caching the
 *	namespace reference. The next time this function is called, the
 *	namespace value can be found quickly.
 *
 *----------------------------------------------------------------------
 */

int
TclGetNamespaceFromObj(
    Tcl_Interp *interp,		/* The current interpreter. */
    Tcl_Obj *objPtr,		/* The object to be resolved as the name of a
				 * namespace. */
    Tcl_Namespace **nsPtrPtr)	/* Result namespace pointer goes here. */
{
    if (GetNamespaceFromObj(interp, objPtr, nsPtrPtr) == TCL_ERROR) {
	const char *name = TclGetString(objPtr);

	if ((name[0] == ':') && (name[1] == ':')) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "namespace \"%s\" not found", name));
	} else {
	    /*
	     * Get the current namespace name.
	     */

	    NamespaceCurrentCmd(NULL, interp, 1, NULL);
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "namespace \"%s\" not found in \"%s\"", name,
		    Tcl_GetStringResult(interp)));
	}
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "NAMESPACE", name, NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetNamespaceFromObj(
    Tcl_Interp *interp,		/* The current interpreter. */
    Tcl_Obj *objPtr,		/* The object to be resolved as the name of a
				 * namespace. */
    Tcl_Namespace **nsPtrPtr)	/* Result namespace pointer goes here. */
{
    ResolvedNsName *resNamePtr;
    Namespace *nsPtr, *refNsPtr;

    if (objPtr->typePtr == &nsNameType) {
	/*
	 * Check that the ResolvedNsName is still valid; avoid letting the ref
	 * cross interps.
	 */

	resNamePtr = objPtr->internalRep.twoPtrValue.ptr1;
	nsPtr = resNamePtr->nsPtr;
	refNsPtr = resNamePtr->refNsPtr;
	if (!(nsPtr->flags & NS_DYING) && (interp == nsPtr->interp) &&
		(!refNsPtr || ((interp == refNsPtr->interp) &&
		(refNsPtr== (Namespace *) Tcl_GetCurrentNamespace(interp))))){
	    *nsPtrPtr = (Tcl_Namespace *) nsPtr;
	    return TCL_OK;
	}
    }
    if (SetNsNameFromAny(interp, objPtr) == TCL_OK) {
	resNamePtr = objPtr->internalRep.twoPtrValue.ptr1;
	*nsPtrPtr = (Tcl_Namespace *) resNamePtr->nsPtr;
	return TCL_OK;
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitNamespaceCmd --
 *
 *	This function is called to create the "namespace" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	Handle for the namespace command, or NULL on failure.
 *
 * Side effects:
 *	none
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
TclInitNamespaceCmd(
    Tcl_Interp *interp)		/* Current interpreter. */
{
    return TclMakeEnsemble(interp, "namespace", defaultNamespaceMap);
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceChildrenCmd --
 *
 *	Invoked to implement the "namespace children" command that returns a
 *	list containing the fully-qualified names of the child namespaces of a
 *	given namespace. Handles the following syntax:
 *
 *	    namespace children ?name? ?pattern?
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceChildrenCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Namespace *namespacePtr;
    Namespace *nsPtr, *childNsPtr;
    Namespace *globalNsPtr = (Namespace *) TclGetGlobalNamespace(interp);
    const char *pattern = NULL;
    Tcl_DString buffer;
    register Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;
    Tcl_Obj *listPtr, *elemPtr;

    /*
     * Get a pointer to the specified namespace, or the current namespace.
     */

    if (objc == 1) {
	nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    } else if ((objc == 2) || (objc == 3)) {
	if (TclGetNamespaceFromObj(interp, objv[1], &namespacePtr) != TCL_OK){
	    return TCL_ERROR;
	}
	nsPtr = (Namespace *) namespacePtr;
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?name? ?pattern?");
	return TCL_ERROR;
    }

    /*
     * Get the glob-style pattern, if any, used to narrow the search.
     */

    Tcl_DStringInit(&buffer);
    if (objc == 3) {
	const char *name = TclGetString(objv[2]);

	if ((*name == ':') && (*(name+1) == ':')) {
	    pattern = name;
	} else {
	    Tcl_DStringAppend(&buffer, nsPtr->fullName, -1);
	    if (nsPtr != globalNsPtr) {
		TclDStringAppendLiteral(&buffer, "::");
	    }
	    Tcl_DStringAppend(&buffer, name, -1);
	    pattern = Tcl_DStringValue(&buffer);
	}
    }

    /*
     * Create a list containing the full names of all child namespaces whose
     * names match the specified pattern, if any.
     */

    listPtr = Tcl_NewListObj(0, NULL);
    if ((pattern != NULL) && TclMatchIsTrivial(pattern)) {
	unsigned int length = strlen(nsPtr->fullName);

	if (strncmp(pattern, nsPtr->fullName, length) != 0) {
	    goto searchDone;
	}
	if (
#ifndef BREAK_NAMESPACE_COMPAT
	    Tcl_FindHashEntry(&nsPtr->childTable, pattern+length) != NULL
#else
	    nsPtr->childTablePtr != NULL &&
	    Tcl_FindHashEntry(nsPtr->childTablePtr, pattern+length) != NULL
#endif
	) {
	    Tcl_ListObjAppendElement(interp, listPtr,
		    Tcl_NewStringObj(pattern, -1));
	}
	goto searchDone;
    }
#ifndef BREAK_NAMESPACE_COMPAT
    entryPtr = Tcl_FirstHashEntry(&nsPtr->childTable, &search);
#else
    if (nsPtr->childTablePtr == NULL) {
	goto searchDone;
    }
    entryPtr = Tcl_FirstHashEntry(nsPtr->childTablePtr, &search);
#endif
    while (entryPtr != NULL) {
	childNsPtr = Tcl_GetHashValue(entryPtr);
	if ((pattern == NULL)
		|| Tcl_StringMatch(childNsPtr->fullName, pattern)) {
	    elemPtr = Tcl_NewStringObj(childNsPtr->fullName, -1);
	    Tcl_ListObjAppendElement(interp, listPtr, elemPtr);
	}
	entryPtr = Tcl_NextHashEntry(&search);
    }

  searchDone:
    Tcl_SetObjResult(interp, listPtr);
    Tcl_DStringFree(&buffer);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceCodeCmd --
 *
 *	Invoked to implement the "namespace code" command to capture the
 *	namespace context of a command. Handles the following syntax:
 *
 *	    namespace code arg
 *
 *	Here "arg" can be a list. "namespace code arg" produces a result
 *	equivalent to that produced by the command
 *
 *	    list ::namespace inscope [namespace current] $arg
 *
 *	However, if "arg" is itself a scoped value starting with "::namespace
 *	inscope", then the result is just "arg".
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	If anything goes wrong, this function returns an error message as the
 *	result in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceCodeCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Namespace *currNsPtr;
    Tcl_Obj *listPtr, *objPtr;
    register const char *arg;
    int length;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "arg");
	return TCL_ERROR;
    }

    /*
     * If "arg" is already a scoped value, then return it directly.
     * Take care to only check for scoping in precisely the style that
     * [::namespace code] generates it.  Anything more forgiving can have
     * the effect of failing in namespaces that contain their own custom
     " "namespace" command.  [Bug 3202171].
     */

    arg = TclGetStringFromObj(objv[1], &length);
    if (*arg==':' && length > 20
	    && strncmp(arg, "::namespace inscope ", 20) == 0) {
	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
    }

    /*
     * Otherwise, construct a scoped command by building a list with
     * "namespace inscope", the full name of the current namespace, and the
     * argument "arg". By constructing a list, we ensure that scoped commands
     * are interpreted properly when they are executed later, by the
     * "namespace inscope" command.
     */

    TclNewObj(listPtr);
    TclNewLiteralStringObj(objPtr, "::namespace");
    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
    TclNewLiteralStringObj(objPtr, "inscope");
    Tcl_ListObjAppendElement(interp, listPtr, objPtr);

    currNsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    if (currNsPtr == (Namespace *) TclGetGlobalNamespace(interp)) {
	TclNewLiteralStringObj(objPtr, "::");
    } else {
	objPtr = Tcl_NewStringObj(currNsPtr->fullName, -1);
    }
    Tcl_ListObjAppendElement(interp, listPtr, objPtr);

    Tcl_ListObjAppendElement(interp, listPtr, objv[1]);

    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceCurrentCmd --
 *
 *	Invoked to implement the "namespace current" command which returns the
 *	fully-qualified name of the current namespace. Handles the following
 *	syntax:
 *
 *	    namespace current
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceCurrentCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register Namespace *currNsPtr;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }

    /*
     * The "real" name of the global namespace ("::") is the null string, but
     * we return "::" for it as a convenience to programmers. Note that "" and
     * "::" are treated as synonyms by the namespace code so that it is still
     * easy to do things like:
     *
     *    namespace [namespace current]::bar { ... }
     */

    currNsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    if (currNsPtr == (Namespace *) TclGetGlobalNamespace(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("::", 2));
    } else {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(currNsPtr->fullName, -1));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceDeleteCmd --
 *
 *	Invoked to implement the "namespace delete" command to delete
 *	namespace(s). Handles the following syntax:
 *
 *	    namespace delete ?name name...?
 *
 *	Each name identifies a namespace. It may include a sequence of
 *	namespace qualifiers separated by "::"s. If a namespace is found, it
 *	is deleted: all variables and procedures contained in that namespace
 *	are deleted. If that namespace is being used on the call stack, it is
 *	kept alive (but logically deleted) until it is removed from the call
 *	stack: that is, it can no longer be referenced by name but any
 *	currently executing procedure that refers to it is allowed to do so
 *	until the procedure returns. If the namespace can't be found, this
 *	function returns an error. If no namespaces are specified, this
 *	command does nothing.
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Deletes the specified namespaces. If anything goes wrong, this
 *	function returns an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceDeleteCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Namespace *namespacePtr;
    const char *name;
    register int i;

    if (objc < 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "?name name...?");
	return TCL_ERROR;
    }

    /*
     * Destroying one namespace may cause another to be destroyed. Break this
     * into two passes: first check to make sure that all namespaces on the
     * command line are valid, and report any errors.
     */

    for (i = 1;  i < objc;  i++) {
	name = TclGetString(objv[i]);
	namespacePtr = Tcl_FindNamespace(interp, name, NULL, /*flags*/ 0);
	if ((namespacePtr == NULL)
		|| (((Namespace *) namespacePtr)->flags & NS_KILLED)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                    "unknown namespace \"%s\" in namespace delete command",
		    TclGetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "NAMESPACE",
		    TclGetString(objv[i]), NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Okay, now delete each namespace.
     */

    for (i = 1;  i < objc;  i++) {
	name = TclGetString(objv[i]);
	namespacePtr = Tcl_FindNamespace(interp, name, NULL, /* flags */ 0);
	if (namespacePtr) {
	    Tcl_DeleteNamespace(namespacePtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceEvalCmd --
 *
 *	Invoked to implement the "namespace eval" command. Executes commands
 *	in a namespace. If the namespace does not already exist, it is
 *	created. Handles the following syntax:
 *
 *	    namespace eval name arg ?arg...?
 *
 *	If more than one arg argument is specified, the command that is
 *	executed is the result of concatenating the arguments together with a
 *	space between each argument.
 *
 * Results:
 *	Returns TCL_OK if the namespace is found and the commands are executed
 *	successfully. Returns TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns the result of the command in the interpreter's result object.
 *	If anything goes wrong, this function returns an error message as the
 *	result.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceEvalCmd(
    ClientData clientData,	/* Arbitrary value passed to cmd. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, NRNamespaceEvalCmd, clientData, objc,
	    objv);
}

static int
NRNamespaceEvalCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    CmdFrame *invoker;
    int word;
    Tcl_Namespace *namespacePtr;
    CallFrame *framePtr, **framePtrPtr;
    Tcl_Obj *objPtr;
    int result;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "name arg ?arg...?");
	return TCL_ERROR;
    }

    /*
     * Try to resolve the namespace reference, caching the result in the
     * namespace object along the way.
     */

    result = GetNamespaceFromObj(interp, objv[1], &namespacePtr);

    /*
     * If the namespace wasn't found, try to create it.
     */

    if (result == TCL_ERROR) {
	const char *name = TclGetString(objv[1]);

	namespacePtr = Tcl_CreateNamespace(interp, name, NULL, NULL);
	if (namespacePtr == NULL) {
	    return TCL_ERROR;
	}
    }

    /*
     * Make the specified namespace the current namespace and evaluate the
     * command(s).
     */

    /* This is needed to satisfy GCC 3.3's strict aliasing rules */
    framePtrPtr = &framePtr;
    (void) TclPushStackFrame(interp, (Tcl_CallFrame **) framePtrPtr,
	    namespacePtr, /*isProcCallFrame*/ 0);

    framePtr->objv = TclFetchEnsembleRoot(interp, objv, objc, &framePtr->objc);

    if (objc == 3) {
	/*
	 * TIP #280: Make actual argument location available to eval'd script.
	 */

	objPtr = objv[2];
	invoker = iPtr->cmdFramePtr;
	word = 3;
	TclArgumentGet(interp, objPtr, &invoker, &word);
    } else {
	/*
	 * More than one argument: concatenate them together with spaces
	 * between, then evaluate the result. Tcl_EvalObjEx will delete the
	 * object when it decrements its refcount after eval'ing it.
	 */

	objPtr = Tcl_ConcatObj(objc-2, objv+2);
	invoker = NULL;
	word = 0;
    }

    /*
     * TIP #280: Make invoking context available to eval'd script.
     */

    TclNRAddCallback(interp, NsEval_Callback, namespacePtr, "eval",
	    NULL, NULL);
    return TclNREvalObjEx(interp, objPtr, 0, invoker, word);
}

static int
NsEval_Callback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Namespace *namespacePtr = data[0];

    if (result == TCL_ERROR) {
	int length = strlen(namespacePtr->fullName);
	int limit = 200;
	int overflow = (length > limit);
	char *cmd = data[1];

	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		"\n    (in namespace %s \"%.*s%s\" script line %d)",
		cmd,
		(overflow ? limit : length), namespacePtr->fullName,
		(overflow ? "..." : ""), Tcl_GetErrorLine(interp)));
    }

    /*
     * Restore the previous "current" namespace.
     */

    TclPopStackFrame(interp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceExistsCmd --
 *
 *	Invoked to implement the "namespace exists" command that returns true
 *	if the given namespace currently exists, and false otherwise. Handles
 *	the following syntax:
 *
 *	    namespace exists name
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceExistsCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Namespace *namespacePtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
	    GetNamespaceFromObj(interp, objv[1], &namespacePtr) == TCL_OK));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceExportCmd --
 *
 *	Invoked to implement the "namespace export" command that specifies
 *	which commands are exported from a namespace. The exported commands
 *	are those that can be imported into another namespace using "namespace
 *	import". Both commands defined in a namespace and commands the
 *	namespace has imported can be exported by a namespace. This command
 *	has the following syntax:
 *
 *	    namespace export ?-clear? ?pattern pattern...?
 *
 *	Each pattern may contain "string match"-style pattern matching special
 *	characters, but the pattern may not include any namespace qualifiers:
 *	that is, the pattern must specify commands in the current (exporting)
 *	namespace. The specified patterns are appended onto the namespace's
 *	list of export patterns.
 *
 *	To reset the namespace's export pattern list, specify the "-clear"
 *	flag.
 *
 *	If there are no export patterns and the "-clear" flag isn't given,
 *	this command returns the namespace's current export list.
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceExportCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int firstArg, i;

    if (objc < 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-clear? ?pattern pattern...?");
	return TCL_ERROR;
    }

    /*
     * If no pattern arguments are given, and "-clear" isn't specified, return
     * the namespace's current export pattern list.
     */

    if (objc == 1) {
	Tcl_Obj *listPtr = Tcl_NewObj();

	(void) Tcl_AppendExportList(interp, NULL, listPtr);
	Tcl_SetObjResult(interp, listPtr);
	return TCL_OK;
    }

    /*
     * Process the optional "-clear" argument.
     */

    firstArg = 1;
    if (strcmp("-clear", Tcl_GetString(objv[firstArg])) == 0) {
	Tcl_Export(interp, NULL, "::", 1);
	Tcl_ResetResult(interp);
	firstArg++;
    }

    /*
     * Add each pattern to the namespace's export pattern list.
     */

    for (i = firstArg;  i < objc;  i++) {
	int result = Tcl_Export(interp, NULL, Tcl_GetString(objv[i]), 0);
	if (result != TCL_OK) {
	    return result;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceForgetCmd --
 *
 *	Invoked to implement the "namespace forget" command to remove imported
 *	commands from a namespace. Handles the following syntax:
 *
 *	    namespace forget ?pattern pattern...?
 *
 *	Each pattern is a name like "foo::*" or "a::b::x*". That is, the
 *	pattern may include the special pattern matching characters recognized
 *	by the "string match" command, but only in the command name at the end
 *	of the qualified name; the special pattern characters may not appear
 *	in a namespace name. All of the commands that match that pattern are
 *	checked to see if they have an imported command in the current
 *	namespace that refers to the matched command. If there is an alias, it
 *	is removed.
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Imported commands are removed from the current namespace. If anything
 *	goes wrong, this function returns an error message in the
 *	interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceForgetCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *pattern;
    register int i, result;

    if (objc < 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "?pattern pattern...?");
	return TCL_ERROR;
    }

    for (i = 1;  i < objc;  i++) {
	pattern = TclGetString(objv[i]);
	result = Tcl_ForgetImport(interp, NULL, pattern);
	if (result != TCL_OK) {
	    return result;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceImportCmd --
 *
 *	Invoked to implement the "namespace import" command that imports
 *	commands into a namespace. Handles the following syntax:
 *
 *	    namespace import ?-force? ?pattern pattern...?
 *
 *	Each pattern is a namespace-qualified name like "foo::*", "a::b::x*",
 *	or "bar::p". That is, the pattern may include the special pattern
 *	matching characters recognized by the "string match" command, but only
 *	in the command name at the end of the qualified name; the special
 *	pattern characters may not appear in a namespace name. All of the
 *	commands that match the pattern and which are exported from their
 *	namespace are made accessible from the current namespace context. This
 *	is done by creating a new "imported command" in the current namespace
 *	that points to the real command in its original namespace; when the
 *	imported command is called, it invokes the real command.
 *
 *	If an imported command conflicts with an existing command, it is
 *	treated as an error. But if the "-force" option is included, then
 *	existing commands are overwritten by the imported commands.
 *
 *	If there are no pattern arguments and the "-force" flag isn't given,
 *	this command returns the list of commands currently imported in
 *	the current namespace.
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Adds imported commands to the current namespace. If anything goes
 *	wrong, this function returns an error message in the interpreter's
 *	result object.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceImportCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int allowOverwrite = 0;
    const char *string, *pattern;
    register int i, result;
    int firstArg;

    if (objc < 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-force? ?pattern pattern...?");
	return TCL_ERROR;
    }

    /*
     * Skip over the optional "-force" as the first argument.
     */

    firstArg = 1;
    if (firstArg < objc) {
	string = TclGetString(objv[firstArg]);
	if ((*string == '-') && (strcmp(string, "-force") == 0)) {
	    allowOverwrite = 1;
	    firstArg++;
	}
    } else {
	/*
	 * When objc == 1, command is just [namespace import]. Introspection
	 * form to return list of imported commands.
	 */

	Tcl_HashEntry *hPtr;
	Tcl_HashSearch search;
	Namespace *nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
	Tcl_Obj *listPtr;

	TclNewObj(listPtr);
	for (hPtr = Tcl_FirstHashEntry(&nsPtr->cmdTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Command *cmdPtr = Tcl_GetHashValue(hPtr);

	    if (cmdPtr->deleteProc == DeleteImportedCmd) {
		Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewStringObj(
			Tcl_GetHashKey(&nsPtr->cmdTable, hPtr) ,-1));
	    }
	}
	Tcl_SetObjResult(interp, listPtr);
	return TCL_OK;
    }

    /*
     * Handle the imports for each of the patterns.
     */

    for (i = firstArg;  i < objc;  i++) {
	pattern = TclGetString(objv[i]);
	result = Tcl_Import(interp, NULL, pattern, allowOverwrite);
	if (result != TCL_OK) {
	    return result;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceInscopeCmd --
 *
 *	Invoked to implement the "namespace inscope" command that executes a
 *	script in the context of a particular namespace. This command is not
 *	expected to be used directly by programmers; calls to it are generated
 *	implicitly when programs use "namespace code" commands to register
 *	callback scripts. Handles the following syntax:
 *
 *	    namespace inscope name arg ?arg...?
 *
 *	The "namespace inscope" command is much like the "namespace eval"
 *	command except that it has lappend semantics and the namespace must
 *	already exist. It treats the first argument as a list, and appends any
 *	arguments after the first onto the end as proper list elements. For
 *	example,
 *
 *	    namespace inscope ::foo {a b} c d e
 *
 *	is equivalent to
 *
 *	    namespace eval ::foo [concat {a b} [list c d e]]
 *
 *	This lappend semantics is important because many callback scripts are
 *	actually prefixes.
 *
 * Results:
 *	Returns TCL_OK to indicate success, or TCL_ERROR to indicate failure.
 *
 * Side effects:
 *	Returns a result in the Tcl interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceInscopeCmd(
    ClientData clientData,	/* Arbitrary value passed to cmd. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, NRNamespaceInscopeCmd, clientData, objc,
	    objv);
}

static int
NRNamespaceInscopeCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Namespace *namespacePtr;
    CallFrame *framePtr, **framePtrPtr;
    int i;
    Tcl_Obj *cmdObjPtr;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "name arg ?arg...?");
	return TCL_ERROR;
    }

    /*
     * Resolve the namespace reference.
     */

    if (TclGetNamespaceFromObj(interp, objv[1], &namespacePtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Make the specified namespace the current namespace.
     */

    framePtrPtr = &framePtr;		/* This is needed to satisfy GCC's
					 * strict aliasing rules. */
    (void) TclPushStackFrame(interp, (Tcl_CallFrame **) framePtrPtr,
	    namespacePtr, /*isProcCallFrame*/ 0);

    framePtr->objv = TclFetchEnsembleRoot(interp, objv, objc, &framePtr->objc);

    /*
     * Execute the command. If there is just one argument, just treat it as a
     * script and evaluate it. Otherwise, create a list from the arguments
     * after the first one, then concatenate the first argument and the list
     * of extra arguments to form the command to evaluate.
     */

    if (objc == 3) {
	cmdObjPtr = objv[2];
    } else {
	Tcl_Obj *concatObjv[2];
	register Tcl_Obj *listPtr;

	listPtr = Tcl_NewListObj(0, NULL);
	for (i = 3;  i < objc;  i++) {
	    if (Tcl_ListObjAppendElement(interp, listPtr, objv[i]) != TCL_OK){
		Tcl_DecrRefCount(listPtr);	/* Free unneeded obj. */
		return TCL_ERROR;
	    }
	}

	concatObjv[0] = objv[2];
	concatObjv[1] = listPtr;
	cmdObjPtr = Tcl_ConcatObj(2, concatObjv);
	Tcl_DecrRefCount(listPtr);    /* We're done with the list object. */
    }

    TclNRAddCallback(interp, NsEval_Callback, namespacePtr, "inscope",
	    NULL, NULL);
    return TclNREvalObjEx(interp, cmdObjPtr, 0, NULL, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceOriginCmd --
 *
 *	Invoked to implement the "namespace origin" command to return the
 *	fully-qualified name of the "real" command to which the specified
 *	"imported command" refers. Handles the following syntax:
 *
 *	    namespace origin name
 *
 * Results:
 *	An imported command is created in an namespace when that namespace
 *	imports a command from another namespace. If a command is imported
 *	into a sequence of namespaces a, b,...,n where each successive
 *	namespace just imports the command from the previous namespace, this
 *	command returns the fully-qualified name of the original command in
 *	the first namespace, a. If "name" does not refer to an alias, its
 *	fully-qualified name is returned. The returned name is stored in the
 *	interpreter's result object. This function returns TCL_OK if
 *	successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	If anything goes wrong, this function returns an error message in the
 *	interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceOriginCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Command command, origCommand;
    Tcl_Obj *resultPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name");
	return TCL_ERROR;
    }

    command = Tcl_GetCommandFromObj(interp, objv[1]);
    if (command == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "invalid command name \"%s\"", TclGetString(objv[1])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "COMMAND",
		TclGetString(objv[1]), NULL);
	return TCL_ERROR;
    }
    origCommand = TclGetOriginalCommand(command);
    TclNewObj(resultPtr);
    if (origCommand == NULL) {
	/*
	 * The specified command isn't an imported command. Return the
	 * command's name qualified by the full name of the namespace it was
	 * defined in.
	 */

	Tcl_GetCommandFullName(interp, command, resultPtr);
    } else {
	Tcl_GetCommandFullName(interp, origCommand, resultPtr);
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceParentCmd --
 *
 *	Invoked to implement the "namespace parent" command that returns the
 *	fully-qualified name of the parent namespace for a specified
 *	namespace. Handles the following syntax:
 *
 *	    namespace parent ?name?
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceParentCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Namespace *nsPtr;

    if (objc == 1) {
	nsPtr = TclGetCurrentNamespace(interp);
    } else if (objc == 2) {
	if (TclGetNamespaceFromObj(interp, objv[1], &nsPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "?name?");
	return TCL_ERROR;
    }

    /*
     * Report the parent of the specified namespace.
     */

    if (nsPtr->parentPtr != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		nsPtr->parentPtr->fullName, -1));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespacePathCmd --
 *
 *	Invoked to implement the "namespace path" command that reads and
 *	writes the current namespace's command resolution path. Has one
 *	optional argument: if present, it is a list of named namespaces to set
 *	the path to, and if absent, the current path should be returned.
 *	Handles the following syntax:
 *
 *	    namespace path ?nsList?
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong
 *	(most notably if the namespace list contains the name of something
 *	other than a namespace). In the successful-exit case, may set the
 *	interpreter result to the list of names of the namespaces on the
 *	current namespace's path.
 *
 * Side effects:
 *	May update the namespace path (triggering a recomputing of all command
 *	names that depend on the namespace for resolution).
 *
 *----------------------------------------------------------------------
 */

static int
NamespacePathCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Namespace *nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    int i, nsObjc, result = TCL_ERROR;
    Tcl_Obj **nsObjv;
    Tcl_Namespace **namespaceList = NULL;

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?pathList?");
	return TCL_ERROR;
    }

    /*
     * If no path is given, return the current path.
     */

    if (objc == 1) {
	Tcl_Obj *resultObj = Tcl_NewObj();

	for (i=0 ; i<nsPtr->commandPathLength ; i++) {
	    if (nsPtr->commandPathArray[i].nsPtr != NULL) {
		Tcl_ListObjAppendElement(NULL, resultObj, Tcl_NewStringObj(
			nsPtr->commandPathArray[i].nsPtr->fullName, -1));
	    }
	}
	Tcl_SetObjResult(interp, resultObj);
	return TCL_OK;
    }

    /*
     * There is a path given, so parse it into an array of namespace pointers.
     */

    if (TclListObjGetElements(interp, objv[1], &nsObjc, &nsObjv) != TCL_OK) {
	goto badNamespace;
    }
    if (nsObjc != 0) {
	namespaceList = TclStackAlloc(interp,
		sizeof(Tcl_Namespace *) * nsObjc);

	for (i=0 ; i<nsObjc ; i++) {
	    if (TclGetNamespaceFromObj(interp, nsObjv[i],
		    &namespaceList[i]) != TCL_OK) {
		goto badNamespace;
	    }
	}
    }

    /*
     * Now we have the list of valid namespaces, install it as the path.
     */

    TclSetNsPath(nsPtr, nsObjc, namespaceList);

    result = TCL_OK;
  badNamespace:
    if (namespaceList != NULL) {
	TclStackFree(interp, namespaceList);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclSetNsPath --
 *
 *	Sets the namespace command name resolution path to the given list of
 *	namespaces. If the list is empty (of zero length) the path is set to
 *	empty and the default old-style behaviour of command name resolution
 *	is used.
 *
 * Results:
 *	nothing
 *
 * Side effects:
 *	Invalidates the command name resolution caches for any command
 *	resolved in the given namespace.
 *
 *----------------------------------------------------------------------
 */

void
TclSetNsPath(
    Namespace *nsPtr,		/* Namespace whose path is to be set. */
    int pathLength,		/* Length of pathAry. */
    Tcl_Namespace *pathAry[])	/* Array of namespaces that are the path. */
{
    if (pathLength != 0) {
	NamespacePathEntry *tmpPathArray =
		ckalloc(sizeof(NamespacePathEntry) * pathLength);
	int i;

	for (i=0 ; i<pathLength ; i++) {
	    tmpPathArray[i].nsPtr = (Namespace *) pathAry[i];
	    tmpPathArray[i].creatorNsPtr = nsPtr;
	    tmpPathArray[i].prevPtr = NULL;
	    tmpPathArray[i].nextPtr =
		    tmpPathArray[i].nsPtr->commandPathSourceList;
	    if (tmpPathArray[i].nextPtr != NULL) {
		tmpPathArray[i].nextPtr->prevPtr = &tmpPathArray[i];
	    }
	    tmpPathArray[i].nsPtr->commandPathSourceList = &tmpPathArray[i];
	}
	if (nsPtr->commandPathLength != 0) {
	    UnlinkNsPath(nsPtr);
	}
	nsPtr->commandPathArray = tmpPathArray;
    } else {
	if (nsPtr->commandPathLength != 0) {
	    UnlinkNsPath(nsPtr);
	}
    }

    nsPtr->commandPathLength = pathLength;
    nsPtr->cmdRefEpoch++;
    nsPtr->resolverEpoch++;
}

/*
 *----------------------------------------------------------------------
 *
 * UnlinkNsPath --
 *
 *	Delete the given namespace's command name resolution path. Only call
 *	if the path is non-empty. Caller must reset the counter containing the
 *	path size.
 *
 * Results:
 *	nothing
 *
 * Side effects:
 *	Deletes the array of path entries and unlinks those path entries from
 *	the target namespace's list of interested namespaces.
 *
 *----------------------------------------------------------------------
 */

static void
UnlinkNsPath(
    Namespace *nsPtr)
{
    int i;
    for (i=0 ; i<nsPtr->commandPathLength ; i++) {
	NamespacePathEntry *nsPathPtr = &nsPtr->commandPathArray[i];

	if (nsPathPtr->prevPtr != NULL) {
	    nsPathPtr->prevPtr->nextPtr = nsPathPtr->nextPtr;
	}
	if (nsPathPtr->nextPtr != NULL) {
	    nsPathPtr->nextPtr->prevPtr = nsPathPtr->prevPtr;
	}
	if (nsPathPtr->nsPtr != NULL) {
	    if (nsPathPtr->nsPtr->commandPathSourceList == nsPathPtr) {
		nsPathPtr->nsPtr->commandPathSourceList = nsPathPtr->nextPtr;
	    }
	}
    }
    ckfree(nsPtr->commandPathArray);
}

/*
 *----------------------------------------------------------------------
 *
 * TclInvalidateNsPath --
 *
 *	Invalidate the name resolution caches for all names looked up in
 *	namespaces whose name path includes the given namespace.
 *
 * Results:
 *	nothing
 *
 * Side effects:
 *	Increments the command reference epoch in each namespace whose path
 *	includes the given namespace. This causes any cached resolved names
 *	whose root cacheing context starts at that namespace to be recomputed
 *	the next time they are used.
 *
 *----------------------------------------------------------------------
 */

void
TclInvalidateNsPath(
    Namespace *nsPtr)
{
    NamespacePathEntry *nsPathPtr = nsPtr->commandPathSourceList;

    while (nsPathPtr != NULL) {
	if (nsPathPtr->nsPtr != NULL) {
	    nsPathPtr->creatorNsPtr->cmdRefEpoch++;
	}
	nsPathPtr = nsPathPtr->nextPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceQualifiersCmd --
 *
 *	Invoked to implement the "namespace qualifiers" command that returns
 *	any leading namespace qualifiers in a string. These qualifiers are
 *	namespace names separated by "::"s. For example, for "::foo::p" this
 *	command returns "::foo", and for "::" it returns "". This command is
 *	the complement of the "namespace tail" command. Note that this command
 *	does not check whether the "namespace" names are, in fact, the names
 *	of currently defined namespaces. Handles the following syntax:
 *
 *	    namespace qualifiers string
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceQualifiersCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register const char *name, *p;
    int length;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "string");
	return TCL_ERROR;
    }

    /*
     * Find the end of the string, then work backward and find the start of
     * the last "::" qualifier.
     */

    name = TclGetString(objv[1]);
    for (p = name;  *p != '\0';  p++) {
	/* empty body */
    }
    while (--p >= name) {
	if ((*p == ':') && (p > name) && (*(p-1) == ':')) {
	    p -= 2;			/* Back up over the :: */
	    while ((p >= name) && (*p == ':')) {
		p--;			/* Back up over the preceeding : */
	    }
	    break;
	}
    }

    if (p >= name) {
	length = p-name+1;
	Tcl_SetObjResult(interp, Tcl_NewStringObj(name, length));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceUnknownCmd --
 *
 *	Invoked to implement the "namespace unknown" command (TIP 181) that
 *	sets or queries a per-namespace unknown command handler. This handler
 *	is called when command lookup fails (current and global ns). The
 *	default handler for the global namespace is ::unknown. The default
 *	handler for other namespaces is to call the global namespace unknown
 *	handler. Passing an empty list results in resetting the handler to its
 *	default.
 *
 *	    namespace unknown ?handler?
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	If no handler is specified, returns a result in the interpreter's
 *	result object, otherwise it sets the unknown handler pointer in the
 *	current namespace to the script fragment provided. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceUnknownCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Namespace *currNsPtr;
    Tcl_Obj *resultPtr;
    int rc;

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?script?");
	return TCL_ERROR;
    }

    currNsPtr = TclGetCurrentNamespace(interp);

    if (objc == 1) {
	/*
	 * Introspection - return the current namespace handler.
	 */

	resultPtr = Tcl_GetNamespaceUnknownHandler(interp, currNsPtr);
	if (resultPtr == NULL) {
	    TclNewObj(resultPtr);
	}
	Tcl_SetObjResult(interp, resultPtr);
    } else {
	rc = Tcl_SetNamespaceUnknownHandler(interp, currNsPtr, objv[1]);
	if (rc == TCL_OK) {
	    Tcl_SetObjResult(interp, objv[1]);
	}
	return rc;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetNamespaceUnknownHandler --
 *
 *	Returns the unknown command handler registered for the given
 *	namespace.
 *
 * Results:
 *	Returns the current unknown command handler, or NULL if none exists
 *	for the namespace.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_GetNamespaceUnknownHandler(
    Tcl_Interp *interp,		/* The interpreter in which the namespace
				 * exists. */
    Tcl_Namespace *nsPtr)	/* The namespace. */
{
    Namespace *currNsPtr = (Namespace *) nsPtr;

    if (currNsPtr->unknownHandlerPtr == NULL &&
	    currNsPtr == ((Interp *) interp)->globalNsPtr) {
	/*
	 * Default handler for global namespace is "::unknown". For all other
	 * namespaces, it is NULL (which falls back on the global unknown
	 * handler).
	 */

	TclNewLiteralStringObj(currNsPtr->unknownHandlerPtr, "::unknown");
	Tcl_IncrRefCount(currNsPtr->unknownHandlerPtr);
    }
    return currNsPtr->unknownHandlerPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetNamespaceUnknownHandler --
 *
 *	Sets the unknown command handler for the given namespace to the
 *	command prefix passed.
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Sets the namespace unknown command handler. If the passed in handler
 *	is NULL or an empty list, then the handler is reset to its default. If
 *	an error occurs, then an error message is left in the interpreter
 *	result.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetNamespaceUnknownHandler(
    Tcl_Interp *interp,		/* Interpreter in which the namespace
				 * exists. */
    Tcl_Namespace *nsPtr,	/* Namespace which is being updated. */
    Tcl_Obj *handlerPtr)	/* The new handler, or NULL to reset. */
{
    int lstlen = 0;
    Namespace *currNsPtr = (Namespace *) nsPtr;

    /*
     * Ensure that we check for errors *first* before we change anything.
     */

    if (handlerPtr != NULL) {
	if (TclListObjLength(interp, handlerPtr, &lstlen) != TCL_OK) {
	    /*
	     * Not a list.
	     */

	    return TCL_ERROR;
	}
	if (lstlen > 0) {
	    /*
	     * We are going to be saving this handler. Increment the reference
	     * count before decrementing the refcount on the previous handler,
	     * so that nothing strange can happen if we are told to set the
	     * handler to the previous value.
	     */

	    Tcl_IncrRefCount(handlerPtr);
	}
    }

    /*
     * Remove old handler next.
     */

    if (currNsPtr->unknownHandlerPtr != NULL) {
	Tcl_DecrRefCount(currNsPtr->unknownHandlerPtr);
    }

    /*
     * Install the new handler.
     */

    if (lstlen > 0) {
	/*
	 * Just store the handler. It already has the correct reference count.
	 */

	currNsPtr->unknownHandlerPtr = handlerPtr;
    } else {
	/*
	 * If NULL or an empty list is passed, this resets to the default
	 * handler.
	 */

	currNsPtr->unknownHandlerPtr = NULL;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceTailCmd --
 *
 *	Invoked to implement the "namespace tail" command that returns the
 *	trailing name at the end of a string with "::" namespace qualifiers.
 *	These qualifiers are namespace names separated by "::"s. For example,
 *	for "::foo::p" this command returns "p", and for "::" it returns "".
 *	This command is the complement of the "namespace qualifiers" command.
 *	Note that this command does not check whether the "namespace" names
 *	are, in fact, the names of currently defined namespaces. Handles the
 *	following syntax:
 *
 *	    namespace tail string
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceTailCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register const char *name, *p;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "string");
	return TCL_ERROR;
    }

    /*
     * Find the end of the string, then work backward and find the last "::"
     * qualifier.
     */

    name = TclGetString(objv[1]);
    for (p = name;  *p != '\0';  p++) {
	/* empty body */
    }
    while (--p > name) {
	if ((*p == ':') && (*(p-1) == ':')) {
	    p++;			/* Just after the last "::" */
	    break;
	}
    }

    if (p >= name) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(p, -1));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceUpvarCmd --
 *
 *	Invoked to implement the "namespace upvar" command, that creates
 *	variables in the current scope linked to variables in another
 *	namespace. Handles the following syntax:
 *
 *	    namespace upvar ns otherVar myVar ?otherVar myVar ...?
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Creates new variables in the current scope, linked to the
 *	corresponding variables in the stipulated nmamespace. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceUpvarCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Namespace *nsPtr, *savedNsPtr;
    Var *otherPtr, *arrayPtr;
    const char *myName;

    if (objc < 2 || (objc & 1)) {
	Tcl_WrongNumArgs(interp, 1, objv, "ns ?otherVar myVar ...?");
	return TCL_ERROR;
    }

    if (TclGetNamespaceFromObj(interp, objv[1], &nsPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    objc -= 2;
    objv += 2;

    for (; objc>0 ; objc-=2, objv+=2) {
	/*
	 * Locate the other variable.
	 */

	savedNsPtr = (Tcl_Namespace *) iPtr->varFramePtr->nsPtr;
	iPtr->varFramePtr->nsPtr = (Namespace *) nsPtr;
	otherPtr = TclObjLookupVarEx(interp, objv[0], NULL,
		(TCL_NAMESPACE_ONLY|TCL_LEAVE_ERR_MSG|TCL_AVOID_RESOLVERS),
		"access", /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
	iPtr->varFramePtr->nsPtr = (Namespace *) savedNsPtr;
	if (otherPtr == NULL) {
	    return TCL_ERROR;
	}

	/*
	 * Create the new variable and link it to otherPtr.
	 */

	myName = TclGetString(objv[1]);
	if (TclPtrMakeUpvar(interp, otherPtr, myName, 0, -1) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamespaceWhichCmd --
 *
 *	Invoked to implement the "namespace which" command that returns the
 *	fully-qualified name of a command or variable. If the specified
 *	command or variable does not exist, it returns "". Handles the
 *	following syntax:
 *
 *	    namespace which ?-command? ?-variable? name
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Returns a result in the interpreter's result object. If anything goes
 *	wrong, the result is an error message.
 *
 *----------------------------------------------------------------------
 */

static int
NamespaceWhichCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const opts[] = {
	"-command", "-variable", NULL
    };
    int lookupType = 0;
    Tcl_Obj *resultPtr;

    if (objc < 2 || objc > 3) {
    badArgs:
	Tcl_WrongNumArgs(interp, 1, objv, "?-command? ?-variable? name");
	return TCL_ERROR;
    } else if (objc == 3) {
	/*
	 * Look for a flag controlling the lookup.
	 */

	if (Tcl_GetIndexFromObj(interp, objv[1], opts, "option", 0,
		&lookupType) != TCL_OK) {
	    /*
	     * Preserve old style of error message!
	     */

	    Tcl_ResetResult(interp);
	    goto badArgs;
	}
    }

    TclNewObj(resultPtr);
    switch (lookupType) {
    case 0: {				/* -command */
	Tcl_Command cmd = Tcl_GetCommandFromObj(interp, objv[objc-1]);

	if (cmd != NULL) {
	    Tcl_GetCommandFullName(interp, cmd, resultPtr);
	}
	break;
    }
    case 1: {				/* -variable */
	Tcl_Var var = Tcl_FindNamespaceVar(interp,
		TclGetString(objv[objc-1]), NULL, /*flags*/ 0);

	if (var != NULL) {
	    Tcl_GetVariableFullName(interp, var, resultPtr);
	}
	break;
    }
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeNsNameInternalRep --
 *
 *	Frees the resources associated with a nsName object's internal
 *	representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Decrements the ref count of any Namespace structure pointed to by the
 *	nsName's internal representation. If there are no more references to
 *	the namespace, it's structure will be freed.
 *
 *----------------------------------------------------------------------
 */

static void
FreeNsNameInternalRep(
    register Tcl_Obj *objPtr)	/* nsName object with internal representation
				 * to free. */
{
    ResolvedNsName *resNamePtr = objPtr->internalRep.twoPtrValue.ptr1;

    /*
     * Decrement the reference count of the namespace. If there are no more
     * references, free it up.
     */

    resNamePtr->refCount--;
    if (resNamePtr->refCount == 0) {
	/*
	 * Decrement the reference count for the cached namespace. If the
	 * namespace is dead, and there are no more references to it, free
	 * it.
	 */

	TclNsDecrRefCount(resNamePtr->nsPtr);
	ckfree(resNamePtr);
    }
    objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DupNsNameInternalRep --
 *
 *	Initializes the internal representation of a nsName object to a copy
 *	of the internal representation of another nsName object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	copyPtr's internal rep is set to refer to the same namespace
 *	referenced by srcPtr's internal rep. Increments the ref count of the
 *	ResolvedNsName structure used to hold the namespace reference.
 *
 *----------------------------------------------------------------------
 */

static void
DupNsNameInternalRep(
    Tcl_Obj *srcPtr,		/* Object with internal rep to copy. */
    register Tcl_Obj *copyPtr)	/* Object with internal rep to set. */
{
    ResolvedNsName *resNamePtr = srcPtr->internalRep.twoPtrValue.ptr1;

    copyPtr->internalRep.twoPtrValue.ptr1 = resNamePtr;
    resNamePtr->refCount++;
    copyPtr->typePtr = &nsNameType;
}

/*
 *----------------------------------------------------------------------
 *
 * SetNsNameFromAny --
 *
 *	Attempt to generate a nsName internal representation for a Tcl object.
 *
 * Results:
 *	Returns TCL_OK if the value could be converted to a proper namespace
 *	reference. Otherwise, it returns TCL_ERROR, along with an error
 *	message in the interpreter's result object.
 *
 * Side effects:
 *	If successful, the object is made a nsName object. Its internal rep is
 *	set to point to a ResolvedNsName, which contains a cached pointer to
 *	the Namespace. Reference counts are kept on both the ResolvedNsName
 *	and the Namespace, so we can keep track of their usage and free them
 *	when appropriate.
 *
 *----------------------------------------------------------------------
 */

static int
SetNsNameFromAny(
    Tcl_Interp *interp,		/* Points to the namespace in which to resolve
				 * name. Also used for error reporting if not
				 * NULL. */
    register Tcl_Obj *objPtr)	/* The object to convert. */
{
    const char *dummy;
    Namespace *nsPtr, *dummy1Ptr, *dummy2Ptr;
    register ResolvedNsName *resNamePtr;
    const char *name;

    if (interp == NULL) {
	return TCL_ERROR;
    }

    name = TclGetString(objPtr);
    TclGetNamespaceForQualName(interp, name, NULL, TCL_FIND_ONLY_NS,
	     &nsPtr, &dummy1Ptr, &dummy2Ptr, &dummy);

    /*
     * If we found a namespace, then create a new ResolvedNsName structure
     * that holds a reference to it.
     */

    if ((nsPtr == NULL) || (nsPtr->flags & NS_DYING)) {
	/*
	 * Our failed lookup proves any previously cached nsName intrep is no
	 * longer valid. Get rid of it so we no longer waste memory storing
	 * it, nor time determining its invalidity again and again.
	 */

	if (objPtr->typePtr == &nsNameType) {
	    TclFreeIntRep(objPtr);
	}
	return TCL_ERROR;
    }

    nsPtr->refCount++;
    resNamePtr = ckalloc(sizeof(ResolvedNsName));
    resNamePtr->nsPtr = nsPtr;
    if ((name[0] == ':') && (name[1] == ':')) {
	resNamePtr->refNsPtr = NULL;
    } else {
	resNamePtr->refNsPtr = (Namespace *) Tcl_GetCurrentNamespace(interp);
    }
    resNamePtr->refCount = 1;
    TclFreeIntRep(objPtr);
    objPtr->internalRep.twoPtrValue.ptr1 = resNamePtr;
    objPtr->typePtr = &nsNameType;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetNamespaceCommandTable --
 *
 *	Returns the hash table of commands.
 *
 * Results:
 *	Pointer to the hash table.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashTable *
TclGetNamespaceCommandTable(
    Tcl_Namespace *nsPtr)
{
    return &((Namespace *) nsPtr)->cmdTable;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetNamespaceChildTable --
 *
 *	Returns the hash table of child namespaces.
 *
 * Results:
 *	Pointer to the hash table.
 *
 * Side effects:
 *	Might allocate memory.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashTable *
TclGetNamespaceChildTable(
    Tcl_Namespace *nsPtr)
{
    Namespace *nPtr = (Namespace *) nsPtr;
#ifndef BREAK_NAMESPACE_COMPAT
    return &nPtr->childTable;
#else
    if (nPtr->childTablePtr == NULL) {
	nPtr->childTablePtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(nPtr->childTablePtr, TCL_STRING_KEYS);
    }
    return nPtr->childTablePtr;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclLogCommandInfo --
 *
 *	This function is invoked after an error occurs in an interpreter. It
 *	adds information to iPtr->errorInfo/errorStack fields to describe the
 *	command that was being executed when the error occurred. When pc and
 *	tosPtr are non-NULL, conveying a bytecode execution "inner context",
 *	and the offending instruction is suitable, that inner context is
 *	recorded in errorStack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information about the command is added to errorInfo/errorStack and the
 *	line number stored internally in the interpreter is set.
 *
 *----------------------------------------------------------------------
 */

void
TclLogCommandInfo(
    Tcl_Interp *interp,		/* Interpreter in which to log information. */
    const char *script,		/* First character in script containing
				 * command (must be <= command). */
    const char *command,	/* First character in command that generated
				 * the error. */
    int length,			/* Number of bytes in command (-1 means use
				 * all bytes up to first null byte). */
    const unsigned char *pc,    /* Current pc of bytecode execution context */
    Tcl_Obj **tosPtr)		/* Current stack of bytecode execution
				 * context */
{
    register const char *p;
    Interp *iPtr = (Interp *) interp;
    int overflow, limit = 150;
    Var *varPtr, *arrayPtr;

    if (iPtr->flags & ERR_ALREADY_LOGGED) {
	/*
	 * Someone else has already logged error information for this command;
	 * we shouldn't add anything more.
	 */

	return;
    }

    if (command != NULL) {
	/*
	 * Compute the line number where the error occurred.
	 */

	iPtr->errorLine = 1;
	for (p = script; p != command; p++) {
	    if (*p == '\n') {
		iPtr->errorLine++;
	    }
	}

	if (length < 0) {
	    length = strlen(command);
	}
	overflow = (length > limit);
	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		"\n    %s\n\"%.*s%s\"", ((iPtr->errorInfo == NULL)
		? "while executing" : "invoked from within"),
		(overflow ? limit : length), command,
		(overflow ? "..." : "")));

	varPtr = TclObjLookupVarEx(interp, iPtr->eiVar, NULL, TCL_GLOBAL_ONLY,
		NULL, 0, 0, &arrayPtr);
	if ((varPtr == NULL) || !TclIsVarTraced(varPtr)) {
	    /*
	     * Should not happen.
	     */

	    return;
	} else {
	    Tcl_HashEntry *hPtr
		    = Tcl_FindHashEntry(&iPtr->varTraces, (char *) varPtr);
	    VarTrace *tracePtr = Tcl_GetHashValue(hPtr);

	    if (tracePtr->traceProc != EstablishErrorInfoTraces) {
		/*
		 * The most recent trace set on ::errorInfo is not the one the
		 * core itself puts on last. This means some other code is
		 * tracing the variable, and the additional trace(s) might be
		 * write traces that expect the timing of writes to
		 * ::errorInfo that existed Tcl releases before 8.5. To
		 * satisfy that compatibility need, we write the current
		 * -errorinfo value to the ::errorInfo variable.
		 */

		Tcl_ObjSetVar2(interp, iPtr->eiVar, NULL, iPtr->errorInfo,
			TCL_GLOBAL_ONLY);
	    }
	}
    }

    /*
     * TIP #348
     */

    if (Tcl_IsShared(iPtr->errorStack)) {
	Tcl_Obj *newObj;

	newObj = Tcl_DuplicateObj(iPtr->errorStack);
	Tcl_DecrRefCount(iPtr->errorStack);
	Tcl_IncrRefCount(newObj);
	iPtr->errorStack = newObj;
    }
    if (iPtr->resetErrorStack) {
	int len;

	iPtr->resetErrorStack = 0;
	Tcl_ListObjLength(interp, iPtr->errorStack, &len);

	/*
	 * Reset while keeping the list intrep as much as possible.
	 */

	Tcl_ListObjReplace(interp, iPtr->errorStack, 0, len, 0, NULL);
	if (pc != NULL) {
	    Tcl_Obj *innerContext;

	    innerContext = TclGetInnerContext(interp, pc, tosPtr);
	    if (innerContext != NULL) {
		Tcl_ListObjAppendElement(NULL, iPtr->errorStack,
			iPtr->innerLiteral);
		Tcl_ListObjAppendElement(NULL, iPtr->errorStack, innerContext);
	    }
	} else if (command != NULL) {
	    Tcl_ListObjAppendElement(NULL, iPtr->errorStack,
		    iPtr->innerLiteral);
	    Tcl_ListObjAppendElement(NULL, iPtr->errorStack,
		    Tcl_NewStringObj(command, length));
	}
    }

    if (!iPtr->framePtr->objc) {
	/*
	 * Special frame, nothing to report.
	 */
    } else if (iPtr->varFramePtr != iPtr->framePtr) {
	/*
	 * uplevel case, [lappend errorstack UP $relativelevel]
	 */

	Tcl_ListObjAppendElement(NULL, iPtr->errorStack, iPtr->upLiteral);
	Tcl_ListObjAppendElement(NULL, iPtr->errorStack, Tcl_NewIntObj(
		iPtr->framePtr->level - iPtr->varFramePtr->level));
    } else if (iPtr->framePtr != iPtr->rootFramePtr) {
	/*
	 * normal case, [lappend errorstack CALL [info level 0]]
	 */

	Tcl_ListObjAppendElement(NULL, iPtr->errorStack, iPtr->callLiteral);
	Tcl_ListObjAppendElement(NULL, iPtr->errorStack, Tcl_NewListObj(
		iPtr->framePtr->objc, iPtr->framePtr->objv));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclErrorStackResetIf --
 *
 *	The TIP 348 reset/no-bc part of TLCI, for specific use by
 *	TclCompileSyntaxError.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset errorstack if it needs be, and in that case remember the
 *	passed-in error message as inner context.
 *
 *----------------------------------------------------------------------
 */

void
TclErrorStackResetIf(
    Tcl_Interp *interp,
    const char *msg,
    int length)
{
    Interp *iPtr = (Interp *) interp;

    if (Tcl_IsShared(iPtr->errorStack)) {
	Tcl_Obj *newObj;

	newObj = Tcl_DuplicateObj(iPtr->errorStack);
	Tcl_DecrRefCount(iPtr->errorStack);
	Tcl_IncrRefCount(newObj);
	iPtr->errorStack = newObj;
    }
    if (iPtr->resetErrorStack) {
	int len;

	iPtr->resetErrorStack = 0;
	Tcl_ListObjLength(interp, iPtr->errorStack, &len);

	/*
	 * Reset while keeping the list intrep as much as possible.
	 */

	Tcl_ListObjReplace(interp, iPtr->errorStack, 0, len, 0, NULL);
	Tcl_ListObjAppendElement(NULL, iPtr->errorStack, iPtr->innerLiteral);
	Tcl_ListObjAppendElement(NULL, iPtr->errorStack,
		Tcl_NewStringObj(msg, length));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LogCommandInfo --
 *
 *	This function is invoked after an error occurs in an interpreter. It
 *	adds information to iPtr->errorInfo/errorStack fields to describe the
 *	command that was being executed when the error occurred.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information about the command is added to errorInfo/errorStack and the
 *	line number stored internally in the interpreter is set.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LogCommandInfo(
    Tcl_Interp *interp,		/* Interpreter in which to log information. */
    const char *script,		/* First character in script containing
				 * command (must be <= command). */
    const char *command,	/* First character in command that generated
				 * the error. */
    int length)			/* Number of bytes in command (-1 means use
				 * all bytes up to first null byte). */
{
    TclLogCommandInfo(interp, script, command, length, NULL, NULL);
}


/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
