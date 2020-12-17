/*
 * ------------------------------------------------------------------------
 *      PACKAGE:  [incr Tcl]
 *  DESCRIPTION:  Object-Oriented Extensions to Tcl
 *
 *  This file contains procedures that use the internal Tcl core stubs
 *  entries.
 *
 * ========================================================================
 *  AUTHOR:  Arnulf Wiedemann
 *
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include <tclInt.h>
#include "itclInt.h"

Tcl_Command
_Tcl_GetOriginalCommand(
    Tcl_Command command)
{
    return TclGetOriginalCommand(command);
}

int
_Tcl_CreateProc(
    Tcl_Interp *interp,         /* Interpreter containing proc. */
    Tcl_Namespace *nsPtr,       /* Namespace containing this proc. */
    const char *procName,       /* Unqualified name of this proc. */
    Tcl_Obj *argsPtr,           /* Description of arguments. */
    Tcl_Obj *bodyPtr,           /* Command body. */
    Tcl_Proc *procPtrPtr)       /* Returns: pointer to proc data. */
{
    int code = TclCreateProc(interp, (Namespace *)nsPtr, procName, argsPtr,
            bodyPtr, (Proc **)procPtrPtr);
    (*(Proc **)procPtrPtr)->cmdPtr = NULL;
    return code;
}

void *
_Tcl_GetObjInterpProc(
    void)
{
    return (void *)TclGetObjInterpProc();
}

void
_Tcl_ProcDeleteProc(
    ClientData clientData)
{
    TclProcDeleteProc(clientData);
}

int
Itcl_RenameCommand(
    Tcl_Interp *interp,
    const char *oldName,
    const char *newName)
{
    return TclRenameCommand(interp, oldName, newName);
}

int
Itcl_PushCallFrame(
    Tcl_Interp * interp,
    Tcl_CallFrame * framePtr,
    Tcl_Namespace * nsPtr,
    int isProcCallFrame)
{
    return Tcl_PushCallFrame(interp, framePtr, nsPtr, isProcCallFrame);
}

void
Itcl_PopCallFrame(
    Tcl_Interp * interp)
{
    Tcl_PopCallFrame(interp);
}

void
Itcl_GetVariableFullName(
    Tcl_Interp * interp,
    Tcl_Var variable,
    Tcl_Obj * objPtr)
{
    Tcl_GetVariableFullName(interp, variable, objPtr);
}

Tcl_Var
Itcl_FindNamespaceVar(
    Tcl_Interp * interp,
    const char * name,
    Tcl_Namespace * contextNsPtr,
    int flags)
{
    return Tcl_FindNamespaceVar(interp, name, contextNsPtr, flags);
}

void
Itcl_SetNamespaceResolvers (
    Tcl_Namespace * namespacePtr,
    Tcl_ResolveCmdProc * cmdProc,
    Tcl_ResolveVarProc * varProc,
    Tcl_ResolveCompiledVarProc * compiledVarProc)
{
    Tcl_SetNamespaceResolvers(namespacePtr, cmdProc, varProc, compiledVarProc);
}

Tcl_HashTable *
Itcl_GetNamespaceCommandTable(
    Tcl_Namespace *nsPtr)
{
    return TclGetNamespaceCommandTable(nsPtr);
}

Tcl_HashTable *
Itcl_GetNamespaceChildTable(
    Tcl_Namespace *nsPtr)
{
    return TclGetNamespaceChildTable(nsPtr);
}

int
Itcl_InitRewriteEnsemble(
    Tcl_Interp *interp,
    int numRemoved,
    int numInserted,
    int objc,
    Tcl_Obj *const *objv)
{
    return TclInitRewriteEnsemble(interp, numRemoved, numInserted, objv);
}

void
Itcl_ResetRewriteEnsemble(
    Tcl_Interp *interp,
    int isRootEnsemble)
{
    TclResetRewriteEnsemble(interp, isRootEnsemble);
}


