/*
 * itclStubs.c --
 *
 *      This file contains the C-implemeted part of Itcl object-system
 *      Itcl
 *
 * Copyright (c) 2006 by Arnulf P. Wiedemann
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "itclInt.h"

static void ItclDeleteStub(ClientData cdata);
static int ItclHandleStubCmd(ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *const objv[]);


/*
 * ------------------------------------------------------------------------
 *  Itcl_IsStub()
 *
 *  Checks the given Tcl command to see if it represents an autoloading
 *  stub created by the "stub create" command.  Returns non-zero if
 *  the command is indeed a stub.
 * ------------------------------------------------------------------------
 */
int
Itcl_IsStub(
    Tcl_Command cmdPtr)      /* command being tested */
{
    Tcl_CmdInfo cmdInfo;

    /*
     *  This may be an imported command, but don't try to get the
     *  original.  Just check to see if this particular command
     *  is a stub.  If we really want the original command, we'll
     *  find it at a higher level.
     */
    if (Tcl_GetCommandInfoFromToken(cmdPtr, &cmdInfo) == 1) {
        if (cmdInfo.deleteProc == ItclDeleteStub) {
            return 1;
        }
    }
    return 0;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_StubCreateCmd()
 *
 *  Invoked by Tcl whenever the user issues a "stub create" command to
 *  create an autoloading stub for imported commands.  Handles the
 *  following syntax:
 *
 *    stub create <name>
 *
 *  Creates a command called <name>.  Executing this command will cause
 *  the real command <name> to be autoloaded.
 * ------------------------------------------------------------------------
 */
int
Itcl_StubCreateCmd(
    ClientData clientData,   /* not used */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Command cmdPtr;
    char *cmdName;
    Tcl_CmdInfo cmdInfo;

    ItclShowArgs(1, "Itcl_StubCreateCmd", objc, objv);
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "name");
        return TCL_ERROR;
    }
    cmdName = Tcl_GetString(objv[1]);

    /*
     *  Create a stub command with the characteristic ItclDeleteStub
     *  procedure.  That way, we can recognize this command later
     *  on as a stub.  Save the cmd token as client data, so we can
     *  get the full name of this command later on.
     */
    cmdPtr = Tcl_CreateObjCommand(interp, cmdName,
        ItclHandleStubCmd, (ClientData)NULL,
        (Tcl_CmdDeleteProc*)ItclDeleteStub);

    Tcl_GetCommandInfoFromToken(cmdPtr, &cmdInfo);
    cmdInfo.objClientData = cmdPtr;
    Tcl_SetCommandInfoFromToken(cmdPtr, &cmdInfo);

    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_StubExistsCmd()
 *
 *  Invoked by Tcl whenever the user issues a "stub exists" command to
 *  see if an existing command is an autoloading stub.  Handles the
 *  following syntax:
 *
 *    stub exists <name>
 *
 *  Looks for a command called <name> and checks to see if it is an
 *  autoloading stub.  Returns a boolean result.
 * ------------------------------------------------------------------------
 */
int
Itcl_StubExistsCmd(
    ClientData clientData,   /* not used */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Command cmdPtr;
    char *cmdName;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "name");
        return TCL_ERROR;
    }
    cmdName = Tcl_GetString(objv[1]);

    cmdPtr = Tcl_FindCommand(interp, cmdName, (Tcl_Namespace*)NULL, 0);

    if ((cmdPtr != NULL) && Itcl_IsStub(cmdPtr)) {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclHandleStubCmd()
 *
 *  Invoked by Tcl to handle commands created by "stub create".
 *  Calls "auto_load" with the full name of the current command to
 *  trigger autoloading of the real implementation.  Then, calls the
 *  command to handle its function.  If successful, this command
 *  returns TCL_OK along with the result from the real implementation
 *  of this command.  Otherwise, it returns TCL_ERROR, along with an
 *  error message in the interpreter.
 * ------------------------------------------------------------------------
 */
static int
ItclHandleStubCmd(
    ClientData clientData,   /* command token for this stub */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Command cmdPtr;
    Tcl_Obj **cmdlinev;
    Tcl_Obj *objAutoLoad[2];
    Tcl_Obj *objPtr;
    Tcl_Obj *cmdNamePtr;
    Tcl_Obj *cmdlinePtr;
    char *cmdName;
    int result;
    int loaded;
    int cmdlinec;

    ItclShowArgs(1, "ItclHandleStubCmd", objc, objv);
    cmdPtr = (Tcl_Command) clientData;
    cmdNamePtr = Tcl_NewStringObj((char*)NULL, 0);
    Tcl_IncrRefCount(cmdNamePtr);
    Tcl_GetCommandFullName(interp, cmdPtr, cmdNamePtr);
    cmdName = Tcl_GetString(cmdNamePtr);

    /*
     *  Try to autoload the real command for this stub.
     */
    objAutoLoad[0] = Tcl_NewStringObj("::auto_load", -1);
    objAutoLoad[1] = cmdNamePtr;
    result = Tcl_EvalObjv(interp, 2, objAutoLoad, 0);
    if (result != TCL_OK) {
        Tcl_DecrRefCount(cmdNamePtr);
        return TCL_ERROR;
    }

    objPtr = Tcl_GetObjResult(interp);
    result = Tcl_GetIntFromObj(interp, objPtr, &loaded);
    if ((result != TCL_OK) || !loaded) {
        Tcl_ResetResult(interp);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "can't autoload \"", cmdName, "\"", (char*)NULL);
        Tcl_DecrRefCount(cmdNamePtr);
        return TCL_ERROR;
    }

    /*
     *  At this point, the real implementation has been loaded.
     *  Invoke the command again with the arguments passed in.
     */
    cmdlinePtr = Itcl_CreateArgs(interp, cmdName, objc - 1, objv + 1);
    (void) Tcl_ListObjGetElements((Tcl_Interp*)NULL, cmdlinePtr,
        &cmdlinec, &cmdlinev);

    Tcl_DecrRefCount(cmdNamePtr);
    Tcl_ResetResult(interp);
    ItclShowArgs(1, "ItclHandleStubCmd", cmdlinec - 1, cmdlinev + 1);
    result = Tcl_EvalObjv(interp, cmdlinec - 1, cmdlinev + 1, TCL_EVAL_DIRECT);
    Tcl_DecrRefCount(cmdlinePtr);
    Tcl_DecrRefCount(objAutoLoad[0]);
    return result;
}


/*
 * ------------------------------------------------------------------------
 *  ItclDeleteStub()
 *
 *  Invoked by Tcl whenever a stub command is deleted.  This procedure
 *  does nothing, but its presence identifies a command as a stub.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ItclDeleteStub(
    ClientData cdata)      /* not used */
{
    /* do nothing */
}

