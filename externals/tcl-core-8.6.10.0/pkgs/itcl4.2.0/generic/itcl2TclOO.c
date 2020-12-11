/*
 * itcl2TclOO.c --
 *
 *	This file contains code to create and manage methods.
 *
 * Copyright (c) 2007 by Arnulf P. Wiedemann
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <tclInt.h>
#include <tclOOInt.h>
#undef FOREACH_HASH_DECLS
#undef FOREACH_HASH
#undef FOREACH_HASH_VALUE
#include "itclInt.h"

void *
Itcl_GetCurrentCallbackPtr(
    Tcl_Interp *interp)
{
    return TOP_CB(interp);
}

int
Itcl_NRRunCallbacks(
    Tcl_Interp *interp,
    void *rootPtr)
{
    return TclNRRunCallbacks(interp, TCL_OK, (NRE_callback*)rootPtr);
}

static int
CallFinalizePMCall(
    void *data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Namespace *nsPtr = (Tcl_Namespace *)data[0];
    TclOO_PostCallProc *postCallProc = (TclOO_PostCallProc *)data[1];
    void *clientData = data[2];

    /*
     * Give the post-call callback a chance to do some cleanup. Note that at
     * this point the call frame itself is invalid; it's already been popped.
     */

    return postCallProc(clientData, interp, NULL, nsPtr, result);
}

static int
FreeCommand(
    void *data[],
    Tcl_Interp *interp,
    int result)
{
    Command *cmdPtr = (Command *)data[0];
    Proc *procPtr = (Proc *)data[1];

    ckfree(cmdPtr);
    procPtr->cmdPtr = NULL;

    return result;
}

static int
Tcl_InvokeClassProcedureMethod(
    Tcl_Interp *interp,
    Tcl_Obj *namePtr,           /* name of the method */
    Tcl_Namespace *nsPtr,       /* namespace for calling method */
    ProcedureMethod *pmPtr,     /* method type specific data */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments as actually seen. */
{
    Proc *procPtr = pmPtr->procPtr;
    CallFrame *framePtr = NULL;
    CallFrame **framePtrPtr1 = &framePtr;
    Tcl_CallFrame **framePtrPtr = (Tcl_CallFrame **)framePtrPtr1;
    int result;

    if (procPtr->cmdPtr == NULL) {
	Command *cmdPtr = (Command *)ckalloc(sizeof(Command));

	memset(cmdPtr, 0, sizeof(Command));
	cmdPtr->nsPtr = (Namespace *) nsPtr;
	cmdPtr->clientData = NULL;
	procPtr->cmdPtr = cmdPtr;
	Tcl_NRAddCallback(interp, FreeCommand, cmdPtr, procPtr, NULL, NULL);
    }

    result = TclProcCompileProc(interp, procPtr, procPtr->bodyPtr,
	    (Namespace *) nsPtr, "body of method", Tcl_GetString(namePtr));
    if (result != TCL_OK) {
	return result;
    }
    /*
     * Make the stack frame and fill it out with information about this call.
     * This operation may fail.
     */


    result = TclPushStackFrame(interp, framePtrPtr, nsPtr, FRAME_IS_PROC);
    if (result != TCL_OK) {
	return result;
    }

    framePtr->clientData = NULL;
    framePtr->objc = objc;
    framePtr->objv = objv;
    framePtr->procPtr = procPtr;

    /*
     * Give the pre-call callback a chance to do some setup and, possibly,
     * veto the call.
     */

    if (pmPtr->preCallProc != NULL) {
	int isFinished;

	result = pmPtr->preCallProc(pmPtr->clientData, interp, NULL,
		(Tcl_CallFrame *) framePtr, &isFinished);
	if (isFinished || result != TCL_OK) {
	    Tcl_PopCallFrame(interp);
	    TclStackFree(interp, framePtr);
	    goto done;
	}
    }

    /*
     * Now invoke the body of the method. Note that we need to take special
     * action when doing unknown processing to ensure that the missing method
     * name is passed as an argument.
     */

    if (pmPtr->postCallProc) {
	Tcl_NRAddCallback(interp, CallFinalizePMCall, nsPtr,
		(void *)pmPtr->postCallProc, pmPtr->clientData, NULL);
    }
    return TclNRInterpProcCore(interp, namePtr, 1, pmPtr->errProc);

done:
    return result;
}

int
Itcl_InvokeProcedureMethod(
    void *clientData,	/* Pointer to some per-method context. */
    Tcl_Interp *interp,
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments as actually seen. */
{
    Tcl_Namespace *nsPtr;
    Method *mPtr;

    mPtr = (Method *)clientData;
    if (mPtr->declaringClassPtr == NULL) {
	/* that is the case for typemethods */
        nsPtr = mPtr->declaringObjectPtr->namespacePtr;
    } else {
        nsPtr = mPtr->declaringClassPtr->thisPtr->namespacePtr;
    }

    return Tcl_InvokeClassProcedureMethod(interp, mPtr->namePtr, nsPtr,
            (ProcedureMethod *)mPtr->clientData, objc, objv);
}

static int
FreeProcedureMethod(
    void *data[],
    Tcl_Interp *interp,
    int result)
{
    ProcedureMethod *pmPtr = (ProcedureMethod *)data[0];
    ckfree(pmPtr);
    return result;
}

static void
EnsembleErrorProc(
    Tcl_Interp *interp,
    Tcl_Obj *procNameObj)
{
    int overflow, limit = 60, nameLen;
    const char *procName = Tcl_GetStringFromObj(procNameObj, &nameLen);

    overflow = (nameLen > limit);
    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
            "\n    (itcl ensemble part \"%.*s%s\" line %d)",
            (overflow ? limit : nameLen), procName,
            (overflow ? "..." : ""), Tcl_GetErrorLine(interp)));
}

int
Itcl_InvokeEnsembleMethod(
    Tcl_Interp *interp,
    Tcl_Namespace *nsPtr,       /* namespace to call the method in */
    Tcl_Obj *namePtr,           /* name of the method */
    Tcl_Proc *procPtr,
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments as actually seen. */
{
    ProcedureMethod *pmPtr = (ProcedureMethod *)ckalloc(sizeof(ProcedureMethod));

    memset(pmPtr, 0, sizeof(ProcedureMethod));
    pmPtr->version = TCLOO_PROCEDURE_METHOD_VERSION;
    pmPtr->procPtr = (Proc *)procPtr;
    pmPtr->flags = USE_DECLARER_NS;
    pmPtr->errProc = EnsembleErrorProc;

    Tcl_NRAddCallback(interp, FreeProcedureMethod, pmPtr, NULL, NULL, NULL);
    return Tcl_InvokeClassProcedureMethod(interp, namePtr, nsPtr,
            pmPtr, objc, objv);
}


/*
 * ----------------------------------------------------------------------
 *
 * Itcl_PublicObjectCmd, Itcl_PrivateObjectCmd --
 *
 *	Main entry point for object invokations. The Public* and Private*
 *	wrapper functions are just thin wrappers around the main ObjectCmd
 *	function that does call chain creation, management and invokation.
 *
 * ----------------------------------------------------------------------
 */

int
Itcl_PublicObjectCmd(
    void *clientData,
    Tcl_Interp *interp,
    Tcl_Class clsPtr,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Object oPtr = (Tcl_Object)clientData;
    int result;

    result = TclOOInvokeObject(interp, oPtr, clsPtr, PUBLIC_METHOD,
            objc, objv);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * Itcl_NewProcClassMethod --
 *
 *	Create a new procedure-like method for a class for Itcl.
 *
 * ----------------------------------------------------------------------
 */

Tcl_Method
Itcl_NewProcClassMethod(
    Tcl_Interp *interp,		/* The interpreter containing the class. */
    Tcl_Class clsPtr,		/* The class to modify. */
    TclOO_PreCallProc *preCallPtr,
    TclOO_PostCallProc *postCallPtr,
    ProcErrorProc *errProc,
    void *clientData,
    Tcl_Obj *nameObj,		/* The name of the method, which may be NULL;
				 * if so, up to caller to manage storage
				 * (e.g., because it is a constructor or
				 * destructor). */
    Tcl_Obj *argsObj,		/* The formal argument list for the method,
				 * which may be NULL; if so, it is equivalent
				 * to an empty list. */
    Tcl_Obj *bodyObj,		/* The body of the method, which must not be
				 * NULL. */
    void **clientData2)
{
    Tcl_Method result;

    result = TclOONewProcMethodEx(interp, clsPtr, preCallPtr, postCallPtr,
           errProc, clientData, nameObj, argsObj, bodyObj,
           PUBLIC_METHOD | USE_DECLARER_NS, clientData2);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * Itcl_NewProcMethod --
 *
 *	Create a new procedure-like method for an object for Itcl.
 *
 * ----------------------------------------------------------------------
 */

Tcl_Method
Itcl_NewProcMethod(
    Tcl_Interp *interp,		/* The interpreter containing the object. */
    Tcl_Object oPtr,		/* The object to modify. */
    TclOO_PreCallProc *preCallPtr,
    TclOO_PostCallProc *postCallPtr,
    ProcErrorProc *errProc,
    void *clientData,
    Tcl_Obj *nameObj,		/* The name of the method, which must not be
				 * NULL. */
    Tcl_Obj *argsObj,		/* The formal argument list for the method,
				 * which must not be NULL. */
    Tcl_Obj *bodyObj,		/* The body of the method, which must not be
				 * NULL. */
    void **clientData2)
{
    return TclOONewProcInstanceMethodEx(interp, oPtr, preCallPtr, postCallPtr,
           errProc, clientData, nameObj, argsObj, bodyObj,
           PUBLIC_METHOD | USE_DECLARER_NS, clientData2);
}

/*
 * ----------------------------------------------------------------------
 *
 * Itcl_NewForwardClassMethod --
 *
 *	Create a new forwarded method for a class for Itcl.
 *
 * ----------------------------------------------------------------------
 */

Tcl_Method
Itcl_NewForwardClassMethod(
    Tcl_Interp *interp,
    Tcl_Class clsPtr,
    int flags,
    Tcl_Obj *nameObj,
    Tcl_Obj *prefixObj)
{
    return (Tcl_Method)TclOONewForwardMethod(interp, (Class *)clsPtr,
            flags, nameObj, prefixObj);
}


static Tcl_Obj *
Itcl_TclOOObjectName(
    Tcl_Interp *interp,
    Object *oPtr)
{
    Tcl_Obj *namePtr;

    if (oPtr->cachedNameObj) {
        return oPtr->cachedNameObj;
    }
    namePtr = Tcl_NewObj();
    Tcl_GetCommandFullName(interp, oPtr->command, namePtr);
    Tcl_IncrRefCount(namePtr);
    oPtr->cachedNameObj = namePtr;
    return namePtr;
}

int
Itcl_SelfCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Interp *iPtr = (Interp *) interp;
    CallFrame *framePtr = iPtr->varFramePtr;
    CallContext *contextPtr;

    if (!Itcl_IsMethodCallFrame(interp)) {
        Tcl_AppendResult(interp, TclGetString(objv[0]),
                " may only be called from inside a method", NULL);
        return TCL_ERROR;
    }

    contextPtr = (CallContext *)framePtr->clientData;

    if (objc == 1) {
        Tcl_SetObjResult(interp, Itcl_TclOOObjectName(interp, contextPtr->oPtr));
        return TCL_OK;
    }
    return TCL_ERROR;
}

int
Itcl_IsMethodCallFrame(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;
    CallFrame *framePtr = iPtr->varFramePtr;
    if (framePtr == NULL || !(framePtr->isProcCallFrame & FRAME_IS_METHOD)) {
        return 0;
    }
    return 1;
}
