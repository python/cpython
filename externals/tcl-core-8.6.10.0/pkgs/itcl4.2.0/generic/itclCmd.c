/*
 * ------------------------------------------------------------------------
 *      PACKAGE:  [incr Tcl]
 *  DESCRIPTION:  Object-Oriented Extensions to Tcl
 *
 *  [incr Tcl] provides object-oriented extensions to Tcl, much as
 *  C++ provides object-oriented extensions to C.  It provides a means
 *  of encapsulating related procedures together with their shared data
 *  in a local namespace that is hidden from the outside world.  It
 *  promotes code re-use through inheritance.  More than anything else,
 *  it encourages better organization of Tcl applications through the
 *  object-oriented paradigm, leading to code that is easier to
 *  understand and maintain.
 *
 *  This file defines information that tracks classes and objects
 *  at a global level for a given interpreter.
 *
 * ========================================================================
 *  AUTHOR:  Michael J. McLennan
 *           Bell Labs Innovations for Lucent Technologies
 *           mmclennan@lucent.com
 *           http://www.tcltk.com/itcl
 *
 *  overhauled version author: Arnulf Wiedemann
 * ========================================================================
 *           Copyright (c) 1993-1998  Lucent Technologies, Inc.
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include "tclInt.h"
#include "itclInt.h"
/*
 * ------------------------------------------------------------------------
 *  Itcl_ThisCmd()
 *
 *  Invoked by Tcl for fast access to itcl methods
 *  syntax:
 *
 *    this methodName args ....
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
NRThisCmd(
    ClientData clientData,   /* class info */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ClientData clientData2;
    Tcl_Object oPtr;
    ItclClass *iclsPtr;

    ItclShowArgs(1, "NRThisCmd", objc, objv);
    iclsPtr = (ItclClass *)clientData;
    clientData2 = Itcl_GetCallFrameClientData(interp);
    oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData2);
    return Itcl_PublicObjectCmd(oPtr, interp, iclsPtr->clsPtr, objc, objv);
}
/* ARGSUSED */
int
Itcl_ThisCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    FOREACH_HASH_DECLS;
    ClientData clientData2;
    Tcl_Object oPtr;
    Tcl_Obj **newObjv;
    ItclClass *iclsPtr;
    ItclDelegatedFunction *idmPtr;
    const char *funcName;
    const char *val;
    int result;

    if (objc == 1) {
        return Itcl_SelfCmd(clientData,interp, objc, objv);
    }
    ItclShowArgs(1, "Itcl_ThisCmd", objc, objv);
    iclsPtr = (ItclClass *)clientData;
    clientData2 = Itcl_GetCallFrameClientData(interp);
    if (clientData2 == NULL) {
	Tcl_AppendResult(interp,
	        "this cannot be invoked without an object context", NULL);
        return TCL_ERROR;
    }
    oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData2);
    if (oPtr == NULL) {
	Tcl_AppendResult(interp,
	        "this cannot be invoked without an object context", NULL);
        return TCL_ERROR;
    }
    if (objc == 1) {
	Tcl_Obj *namePtr = Tcl_NewObj();

        Tcl_GetCommandFullName(interp, Tcl_GetObjectCommand(oPtr), namePtr);
        Tcl_SetObjResult(interp, namePtr);
	return TCL_OK;
    }
    hPtr = Tcl_FindHashEntry(&iclsPtr->resolveCmds, (char *)objv[1]);
    funcName = Tcl_GetString(objv[1]);
    if (!(iclsPtr->flags & ITCL_CLASS)) {
        FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	    if (strcmp(Tcl_GetString(idmPtr->namePtr), funcName) == 0) {
		if (idmPtr->icPtr == NULL) {
		    if (idmPtr->usingPtr != NULL) {
                        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * objc);
		        newObjv[0] = idmPtr->usingPtr;
		        Tcl_IncrRefCount(newObjv[0]);
                        memcpy(newObjv+1, objv+2, sizeof(Tcl_Obj *) *
			         (objc - 2));
ItclShowArgs(1, "EVAL2", objc - 1, newObjv);
	                result = Tcl_EvalObjv(interp, objc - 1, newObjv, 0);
		        Tcl_DecrRefCount(newObjv[0]);
		        ckfree((char *)newObjv);
		    } else {
		       Tcl_AppendResult(interp,
		               "delegate has not yet been implemented in",
			       ": \"this\" method/command!", NULL);
		       return TCL_ERROR;
		    }
		} else {
                    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) *
		            (objc + 1));
		    newObjv[0] = Tcl_NewStringObj("this", -1);
		    Tcl_IncrRefCount(newObjv[0]);
		    val = Tcl_GetVar2(interp,
		            Tcl_GetString(idmPtr->icPtr->namePtr), NULL, 0);
		    newObjv[1] = Tcl_NewStringObj(val, -1);
		    Tcl_IncrRefCount(newObjv[1]);
                    memcpy(newObjv+2, objv+1, sizeof(Tcl_Obj *) * (objc -1));
ItclShowArgs(1, "EVAL2", objc+1, newObjv);
	            result = Tcl_EvalObjv(interp, objc+1, newObjv, 0);
		    Tcl_DecrRefCount(newObjv[1]);
		    Tcl_DecrRefCount(newObjv[0]);
		    ckfree((char *)newObjv);
		}
	        return result;
	    }
	}
    }
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "class \"", iclsPtr->nsPtr->fullName,
	        "\" has no method: \"", Tcl_GetString(objv[1]), "\"", NULL);
        return TCL_ERROR;
    }
    return Tcl_NRCallObjProc(interp, NRThisCmd, clientData, objc, objv);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_FindClassesCmd()
 *
 *  Invoked by Tcl whenever the user issues an "itcl::find classes"
 *  command to query the list of known classes.  Handles the following
 *  syntax:
 *
 *    find classes ?<pattern>?
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_FindClassesCmd(
    ClientData clientData,   /* class/object info */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Namespace *activeNs = Tcl_GetCurrentNamespace(interp);
    Tcl_Namespace *globalNs = Tcl_GetGlobalNamespace(interp);
    Tcl_HashTable unique;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch place;
    Tcl_Command cmd;
    Tcl_Command originalCmd;
    Tcl_Namespace *nsPtr;
    Tcl_Obj *objPtr;
    Itcl_Stack search;
    char *pattern;
    const char *cmdName;
    int newEntry;
    int handledActiveNs;
    int forceFullNames = 0;

    ItclShowArgs(2, "Itcl_FindClassesCmd", objc, objv);
    if (objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?pattern?");
        return TCL_ERROR;
    }

    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
        forceFullNames = (strstr(pattern, "::") != NULL);
    } else {
        pattern = NULL;
    }

    /*
     *  Search through all commands in the current namespace first,
     *  in the global namespace next, then in all child namespaces
     *  in this interpreter.  If we find any commands that
     *  represent classes, report them.
     */

    Itcl_InitStack(&search);
    Itcl_PushStack(globalNs, &search);
    Itcl_PushStack(activeNs, &search);  /* last in, first out! */

    Tcl_InitHashTable(&unique, TCL_ONE_WORD_KEYS);

    handledActiveNs = 0;
    while (Itcl_GetStackSize(&search) > 0) {
        nsPtr = (Tcl_Namespace *)Itcl_PopStack(&search);
        if (nsPtr == activeNs && handledActiveNs) {
            continue;
        }

        hPtr = Tcl_FirstHashEntry(Itcl_GetNamespaceCommandTable(nsPtr),
	        &place);
        while (hPtr) {
            cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
            if (Itcl_IsClass(cmd)) {
                originalCmd = Tcl_GetOriginalCommand(cmd);

                /*
                 *  Report full names if:
                 *  - the pattern has namespace qualifiers
                 *  - the class namespace is not in the current namespace
                 *  - the class's object creation command is imported from
                 *      another namespace.
                 *
                 *  Otherwise, report short names.
                 */
                if (forceFullNames || nsPtr != activeNs ||
                    originalCmd != NULL) {

                    objPtr = Tcl_NewStringObj(NULL, 0);
                    Tcl_GetCommandFullName(interp, cmd, objPtr);
                    cmdName = Tcl_GetString(objPtr);
                } else {
                    cmdName = Tcl_GetCommandName(interp, cmd);
                    objPtr = Tcl_NewStringObj((const char *)cmdName, -1);
                }

                if (originalCmd) {
                    cmd = originalCmd;
                }
                Tcl_CreateHashEntry(&unique, (char*)cmd, &newEntry);

                if (newEntry &&
			((pattern == NULL) ||
			Tcl_StringCaseMatch((const char *)cmdName, pattern, 0))) {
                    Tcl_ListObjAppendElement(NULL,
			    Tcl_GetObjResult(interp), objPtr);
                } else {
		    /* if not appended to the result, free objPtr. */
		    Tcl_DecrRefCount(objPtr);
		}

            }
            hPtr = Tcl_NextHashEntry(&place);
        }
        handledActiveNs = 1;  /* don't process the active namespace twice */

        /*
         *  Push any child namespaces onto the stack and continue
         *  the search in those namespaces.
         */
        hPtr = Tcl_FirstHashEntry(Itcl_GetNamespaceChildTable(nsPtr), &place);
        while (hPtr != NULL) {
            Itcl_PushStack(Tcl_GetHashValue(hPtr), &search);
            hPtr = Tcl_NextHashEntry(&place);
        }
    }
    Tcl_DeleteHashTable(&unique);
    Itcl_DeleteStack(&search);

    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_FindObjectsCmd()
 *
 *  Invoked by Tcl whenever the user issues an "itcl::find objects"
 *  command to query the list of known objects.  Handles the following
 *  syntax:
 *
 *    find objects ?-class <className>? ?-isa <className>? ?<pattern>?
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
int
Itcl_FindObjectsCmd(
    ClientData clientData,   /* class/object info */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Namespace *activeNs = Tcl_GetCurrentNamespace(interp);
    Tcl_Namespace *globalNs = Tcl_GetGlobalNamespace(interp);
    int forceFullNames = 0;

    char *pattern = NULL;
    ItclClass *iclsPtr = NULL;
    ItclClass *isaDefn = NULL;

    char *name = NULL;
    char *token = NULL;
    const char *cmdName = NULL;
    int pos;
    int newEntry;
    int match;
    int handledActiveNs;
    ItclObject *contextIoPtr;
    Tcl_HashTable unique;
    Tcl_HashEntry *entry;
    Tcl_HashSearch place;
    Itcl_Stack search;
    Tcl_Command cmd;
    Tcl_Command originalCmd;
    Tcl_CmdInfo cmdInfo;
    Tcl_Namespace *nsPtr;
    Tcl_Obj *objPtr;

    /*
     *  Parse arguments:
     *  objects ?-class <className>? ?-isa <className>? ?<pattern>?
     */
    pos = 0;
    while (++pos < objc) {
        token = Tcl_GetString(objv[pos]);
        if (*token != '-') {
            if (!pattern) {
                pattern = token;
                forceFullNames = (strstr(pattern, "::") != NULL);
            } else {
                break;
            }
        }
        else if ((pos+1 < objc) && (strcmp(token,"-class") == 0)) {
            name = Tcl_GetString(objv[pos+1]);
            iclsPtr = Itcl_FindClass(interp, name, /* autoload */ 1);
            if (iclsPtr == NULL) {
                return TCL_ERROR;
            }
            pos++;
        }
        else if ((pos+1 < objc) && (strcmp(token,"-isa") == 0)) {
            name = Tcl_GetString(objv[pos+1]);
            isaDefn = Itcl_FindClass(interp, name, /* autoload */ 1);
            if (isaDefn == NULL) {
                return TCL_ERROR;
            }
            pos++;
        } else {

            /*
             * Last token? Take it as the pattern, even if it starts
             * with a "-".  This allows us to match object names that
             * start with "-".
             */
            if (pos == objc-1 && !pattern) {
                pattern = token;
                forceFullNames = (strstr(pattern, "::") != NULL);
            } else {
                break;
	    }
        }
    }

    if (pos < objc) {
        Tcl_WrongNumArgs(interp, 1, objv,
            "?-class className? ?-isa className? ?pattern?");
        return TCL_ERROR;
    }

    /*
     *  Search through all commands in the current namespace first,
     *  in the global namespace next, then in all child namespaces
     *  in this interpreter.  If we find any commands that
     *  represent objects, report them.
     */

    Itcl_InitStack(&search);
    Itcl_PushStack(globalNs, &search);
    Itcl_PushStack(activeNs, &search);  /* last in, first out! */

    Tcl_InitHashTable(&unique, TCL_ONE_WORD_KEYS);

    handledActiveNs = 0;
    while (Itcl_GetStackSize(&search) > 0) {
        nsPtr = (Tcl_Namespace *)Itcl_PopStack(&search);
        if (nsPtr == activeNs && handledActiveNs) {
            continue;
        }

        entry = Tcl_FirstHashEntry(Itcl_GetNamespaceCommandTable(nsPtr), &place);
        while (entry) {
            cmd = (Tcl_Command)Tcl_GetHashValue(entry);
            if (Itcl_IsObject(cmd)) {
                originalCmd = Tcl_GetOriginalCommand(cmd);
                if (originalCmd) {
                    cmd = originalCmd;
                }
		Tcl_GetCommandInfoFromToken(cmd, &cmdInfo);
                contextIoPtr = (ItclObject*)cmdInfo.deleteData;

                /*
                 *  Report full names if:
                 *  - the pattern has namespace qualifiers
                 *  - the class namespace is not in the current namespace
                 *  - the class's object creation command is imported from
                 *      another namespace.
                 *
                 *  Otherwise, report short names.
                 */
                if (forceFullNames || nsPtr != activeNs ||
                    originalCmd != NULL) {

                    objPtr = Tcl_NewStringObj(NULL, 0);
                    Tcl_GetCommandFullName(interp, cmd, objPtr);
		    cmdName = Tcl_GetString(objPtr);
                } else {
                    cmdName = Tcl_GetCommandName(interp, cmd);
                    objPtr = Tcl_NewStringObj((const char *)cmdName, -1);
                }

                Tcl_CreateHashEntry(&unique, (char*)cmd, &newEntry);

                match = 0;
		if (newEntry &&
			(!pattern || Tcl_StringCaseMatch((const char *)cmdName,
			pattern, 0))) {
                    if ((iclsPtr == NULL) ||
		            (contextIoPtr->iclsPtr == iclsPtr)) {
                        if (isaDefn == NULL) {
                            match = 1;
                        } else {
                            entry = Tcl_FindHashEntry(
                                &contextIoPtr->iclsPtr->heritage,
                                (char*)isaDefn);

                            if (entry) {
                                match = 1;
                            }
                        }
                    }
                }

                if (match) {
                    Tcl_ListObjAppendElement(NULL,
                        Tcl_GetObjResult(interp), objPtr);
                } else {
                    Tcl_DecrRefCount(objPtr);  /* throw away the name */
                }
            }
            entry = Tcl_NextHashEntry(&place);
        }
        handledActiveNs = 1;  /* don't process the active namespace twice */

        /*
         *  Push any child namespaces onto the stack and continue
         *  the search in those namespaces.
         */
        entry = Tcl_FirstHashEntry(Itcl_GetNamespaceChildTable(nsPtr), &place);
        while (entry != NULL) {
            Itcl_PushStack(Tcl_GetHashValue(entry), &search);
            entry = Tcl_NextHashEntry(&place);
        }
    }
    Tcl_DeleteHashTable(&unique);
    Itcl_DeleteStack(&search);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DelClassCmd()
 *
 *  Part of the "delete" ensemble.  Invoked by Tcl whenever the
 *  user issues a "delete class" command to delete classes.
 *  Handles the following syntax:
 *
 *    delete class <name> ?<name>...?
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
NRDelClassCmd(
    ClientData clientData,   /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int i;
    char *name;
    ItclClass *iclsPtr;

    ItclShowArgs(1, "Itcl_DelClassCmd", objc, objv);
    /*
     *  Since destroying a base class will destroy all derived
     *  classes, calls like "destroy class Base Derived" could
     *  fail.  Break this into two passes:  first check to make
     *  sure that all classes on the command line are valid,
     *  then delete them.
     */
    for (i=1; i < objc; i++) {
        name = Tcl_GetString(objv[i]);
        iclsPtr = Itcl_FindClass(interp, name, /* autoload */ 1);
        if (iclsPtr == NULL) {
            return TCL_ERROR;
        }
    }

    for (i=1; i < objc; i++) {
        name = Tcl_GetString(objv[i]);
        iclsPtr = Itcl_FindClass(interp, name, /* autoload */ 0);

        if (iclsPtr) {
            Tcl_ResetResult(interp);
            if (Itcl_DeleteClass(interp, iclsPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    Tcl_ResetResult(interp);
    return TCL_OK;
}

/* ARGSUSED */
int
Itcl_DelClassCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    return Tcl_NRCallObjProc(interp, NRDelClassCmd, clientData, objc, objv);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_DelObjectCmd()
 *
 *  Part of the "delete" ensemble.  Invoked by Tcl whenever the user
 *  issues a "delete object" command to delete [incr Tcl] objects.
 *  Handles the following syntax:
 *
 *    delete object <name> ?<name>...?
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
static int
CallDeleteObject(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    ItclObject *contextIoPtr = (ItclObject *)data[0];
    if (contextIoPtr->destructorHasBeenCalled) {
	Tcl_AppendResult(interp, "can't delete an object while it is being ",
	        "destructed", NULL);
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        result = Itcl_DeleteObject(interp, contextIoPtr);
    }
    return result;
}

static int
NRDelObjectCmd(
    ClientData clientData,   /* object management info */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObject *contextIoPtr;
    char *name;
    void *callbackPtr;
    int i;
    int result;

    ItclShowArgs(1, "Itcl_DelObjectCmd", objc, objv);
    /*
     *  Scan through the list of objects and attempt to delete them.
     *  If anything goes wrong (i.e., destructors fail), then
     *  abort with an error.
     */
    for (i=1; i < objc; i++) {
        name = Tcl_GetString(objv[i]);
	contextIoPtr = NULL;
        if (Itcl_FindObject(interp, name, &contextIoPtr) != TCL_OK) {
            return TCL_ERROR;
        }

        if (contextIoPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "object \"", name, "\" not found",
                NULL);
            return TCL_ERROR;
        }

        callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
        Tcl_NRAddCallback(interp, CallDeleteObject, contextIoPtr,
	        NULL, NULL, NULL);
        result = Itcl_NRRunCallbacks(interp, callbackPtr);
	if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/* ARGSUSED */
int
Itcl_DelObjectCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    return Tcl_NRCallObjProc(interp, NRDelObjectCmd, clientData, objc, objv);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ScopeCmd()
 *
 *  Invoked by Tcl whenever the user issues a "scope" command to
 *  create a fully qualified variable name.  Handles the following
 *  syntax:
 *
 *    scope <variable>
 *
 *  If the input string is already fully qualified (starts with "::"),
 *  then this procedure does nothing.  Otherwise, it looks for a
 *  data member called <variable> and returns its fully qualified
 *  name.  If the <variable> is a common data member, this procedure
 *  returns a name of the form:
 *
 *    ::namesp::namesp::class::variable
 *
 *  If the <variable> is an instance variable, this procedure returns
 *  a name in a format that Tcl can use to find the same variable from
 *  any context.
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_ScopeCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Namespace *contextNsPtr;
    Tcl_HashEntry *hPtr;
    Tcl_Object oPtr;
    Tcl_Obj *objPtr2;
    Tcl_Var var;
    Tcl_HashEntry *entry;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclObjectInfo *infoPtr;
    ItclVarLookup *vlookup;
    char *openParen;
    char *p;
    char *token;
    int doAppend;
    int result;

    ItclShowArgs(1, "Itcl_ScopeCmd", objc, objv);
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "varname");
        return TCL_ERROR;
    }

    contextNsPtr = Tcl_GetCurrentNamespace(interp);
    openParen = NULL;
    result = TCL_OK;
    /*
     *  If this looks like a fully qualified name already,
     *  then return it as is.
     */
    token = Tcl_GetString(objv[1]);
    if (*token == ':' && *(token+1) == ':') {
        Tcl_SetObjResult(interp, objv[1]);
        return TCL_OK;
    }

    /*
     *  If the variable name is an array reference, pick out
     *  the array name and use that for the lookup operations
     *  below.
     */
    for (p=token; *p != '\0'; p++) {
        if (*p == '(') {
            openParen = p;
        }
        else if (*p == ')' && openParen) {
            *openParen = '\0';
            break;
        }
    }

    /*
     *  Figure out what context we're in.  If this is a class,
     *  then look up the variable in the class definition.
     *  If this is a namespace, then look up the variable in its
     *  varTable.  Note that the normal Itcl_GetContext function
     *  returns an error if we're not in a class context, so we
     *  perform a similar function here, the hard way.
     *
     *  TRICKY NOTE:  If this is an array reference, we'll get
     *    the array variable as the variable name.  We must be
     *    careful to add the index (everything from openParen
     *    onward) as well.
     */
    contextIoPtr = NULL;
    contextIclsPtr = NULL;
    oPtr = NULL;
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)contextNsPtr);
    if (hPtr != NULL) {
        contextIclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
    }
    if (Itcl_IsClassNamespace(contextNsPtr)) {
	ClientData clientData;

        entry = ItclResolveVarEntry(contextIclsPtr, token);
        if (!entry) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "variable \"", token, "\" not found in class \"",
                Tcl_GetString(contextIclsPtr->fullNamePtr), "\"",
                NULL);
            result = TCL_ERROR;
            goto scopeCmdDone;
        }
        vlookup = (ItclVarLookup*)Tcl_GetHashValue(entry);

        if (vlookup->ivPtr->flags & ITCL_COMMON) {
            Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);
	    if (vlookup->ivPtr->protection != ITCL_PUBLIC) {
	        Tcl_AppendToObj(resultPtr, ITCL_VARIABLES_NAMESPACE, -1);
	    }
	    Tcl_AppendToObj(resultPtr,
		    Tcl_GetString(vlookup->ivPtr->fullNamePtr), -1);
            if (openParen) {
                *openParen = '(';
                Tcl_AppendToObj(resultPtr, openParen, -1);
                openParen = NULL;
            }
            result = TCL_OK;
            goto scopeCmdDone;
        }

        /*
         *  If this is not a common variable, then we better have
         *  an object context.  Return the name as a fully qualified name.
         */
        infoPtr = contextIclsPtr->infoPtr;
        clientData = Itcl_GetCallFrameClientData(interp);
        if (clientData != NULL) {
            oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData);
            if (oPtr != NULL) {
                contextIoPtr = (ItclObject*)Tcl_ObjectGetMetadata(
	                oPtr, infoPtr->object_meta_type);
	    }
        }

        if (contextIoPtr == NULL) {
	    if (infoPtr->currIoPtr != NULL) {
	        contextIoPtr = infoPtr->currIoPtr;
	    }
	}
        if (contextIoPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "can't scope variable \"", token,
                "\": missing object context",
                NULL);
            result = TCL_ERROR;
            goto scopeCmdDone;
        }

        doAppend = 1;
        if (contextIclsPtr->flags & ITCL_ECLASS) {
            if (strcmp(token, "itcl_options") == 0) {
	        doAppend = 0;
            }
        }

        objPtr2 = Tcl_NewStringObj(NULL, 0);
        Tcl_IncrRefCount(objPtr2);
	Tcl_AppendToObj(objPtr2, ITCL_VARIABLES_NAMESPACE, -1);
	Tcl_AppendToObj(objPtr2,
		(Tcl_GetObjectNamespace(contextIoPtr->oPtr))->fullName, -1);

        if (doAppend) {
            Tcl_AppendToObj(objPtr2,
	            Tcl_GetString(vlookup->ivPtr->fullNamePtr), -1);
        } else {
            Tcl_AppendToObj(objPtr2, "::", -1);
            Tcl_AppendToObj(objPtr2,
	            Tcl_GetString(vlookup->ivPtr->namePtr), -1);
	}

        if (openParen) {
            *openParen = '(';
            Tcl_AppendToObj(objPtr2, openParen, -1);
            openParen = NULL;
        }
        /* fix for SF bug #238 use Tcl_AppendResult instead of Tcl_AppendElement */
        Tcl_AppendResult(interp, Tcl_GetString(objPtr2), NULL);
        Tcl_DecrRefCount(objPtr2);
    } else {

        /*
         *  We must be in an ordinary namespace context.  Resolve
         *  the variable using Tcl_FindNamespaceVar.
         *
         *  TRICKY NOTE:  If this is an array reference, we'll get
         *    the array variable as the variable name.  We must be
         *    careful to add the index (everything from openParen
         *    onward) as well.
         */
        Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);

        var = Itcl_FindNamespaceVar(interp, token, contextNsPtr,
            TCL_NAMESPACE_ONLY);

        if (!var) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "variable \"", token, "\" not found in namespace \"",
                contextNsPtr->fullName, "\"",
                NULL);
            result = TCL_ERROR;
            goto scopeCmdDone;
        }

        Itcl_GetVariableFullName(interp, var, resultPtr);
        if (openParen) {
            *openParen = '(';
            Tcl_AppendToObj(resultPtr, openParen, -1);
            openParen = NULL;
        }
    }

scopeCmdDone:
    if (openParen) {
        *openParen = '(';
    }
    return result;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_CodeCmd()
 *
 *  Invoked by Tcl whenever the user issues a "code" command to
 *  create a scoped command string.  Handles the following syntax:
 *
 *    code ?-namespace foo? arg ?arg arg ...?
 *
 *  Unlike the scope command, the code command DOES NOT look for
 *  scoping information at the beginning of the command.  So scopes
 *  will nest in the code command.
 *
 *  The code command is similar to the "namespace code" command in
 *  Tcl, but it preserves the list structure of the input arguments,
 *  so it is a lot more useful.
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_CodeCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Namespace *contextNs = Tcl_GetCurrentNamespace(interp);

    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    const char *token;
    int pos;

    ItclShowArgs(1, "Itcl_CodeCmd", objc, objv);
    /*
     *  Handle flags like "-namespace"...
     */
    for (pos=1; pos < objc; pos++) {
        token = Tcl_GetString(objv[pos]);
        if (*token != '-') {
            break;
        }

        if (strcmp(token, "-namespace") == 0) {
            if (objc == 2) {
                Tcl_WrongNumArgs(interp, 1, objv,
                    "?-namespace name? command ?arg arg...?");
                return TCL_ERROR;
            } else {
                token = Tcl_GetString(objv[pos+1]);
                contextNs = Tcl_FindNamespace(interp, token,
                    NULL, TCL_LEAVE_ERR_MSG);

                if (!contextNs) {
                    return TCL_ERROR;
                }
                pos++;
            }
        } else {
	    if (strcmp(token, "--") == 0) {
                pos++;
                break;
            } else {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "bad option \"", token, "\": should be -namespace or --",
                    NULL);
                return TCL_ERROR;
            }
        }
    }

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv,
            "?-namespace name? command ?arg arg...?");
        return TCL_ERROR;
    }

    /*
     *  Now construct a scoped command by integrating the
     *  current namespace context, and appending the remaining
     *  arguments AS A LIST...
     */
    listPtr = Tcl_NewListObj(0, NULL);

    Tcl_ListObjAppendElement(interp, listPtr,
        Tcl_NewStringObj("namespace", -1));
    Tcl_ListObjAppendElement(interp, listPtr,
        Tcl_NewStringObj("inscope", -1));

    if (contextNs == Tcl_GetGlobalNamespace(interp)) {
        objPtr = Tcl_NewStringObj("::", -1);
    } else {
        objPtr = Tcl_NewStringObj(contextNs->fullName, -1);
    }
    Tcl_ListObjAppendElement(interp, listPtr, objPtr);

    if (objc-pos == 1) {
        objPtr = objv[pos];
    } else {
        objPtr = Tcl_NewListObj(objc-pos, &objv[pos]);
    }
    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_IsObjectCmd()
 *
 *  Invoked by Tcl whenever the user issues an "itcl::is object"
 *  command to test whether the argument is an object or not.
 *  syntax:
 *
 *    itcl::is object ?-class classname? commandname
 *
 *  Returns 1 if it is an object, 0 otherwise
 * ------------------------------------------------------------------------
 */
int
Itcl_IsObjectCmd(
    ClientData clientData,   /* class/object info */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{

    int             classFlag = 0;
    int             idx = 0;
    char            *name = NULL;
    char            *cname;
    char            *cmdName;
    char            *token;
    Tcl_Command     cmd;
    Tcl_Namespace   *contextNs = NULL;
    ItclClass       *iclsPtr = NULL;

    /*
     *    Handle the arguments.
     *    objc needs to be either:
     *        2    itcl::is object commandname
     *        4    itcl::is object -class classname commandname
     */
    if (objc != 2 && objc != 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-class classname? commandname");
        return TCL_ERROR;
    }

    /*
     *    Parse the command args. Look for the -class
     *    keyword.
     */
    for (idx=1; idx < objc; idx++) {
        token = Tcl_GetString(objv[idx]);

        if (strcmp(token,"-class") == 0) {
            cname = Tcl_GetString(objv[idx+1]);
            iclsPtr = Itcl_FindClass(interp, cname, /* no autoload */ 0);

            if (iclsPtr == NULL) {
                    return TCL_ERROR;
            }

            idx++;
            classFlag = 1;
        } else {
            name = Tcl_GetString(objv[idx]);
        }

    } /* end for objc loop */


    /*
     *  The object name may be a scoped value of the form
     *  "namespace inscope <namesp> <command>".  If it is,
     *  decode it.
     */
    if (Itcl_DecodeScopedCommand(interp, name, &contextNs, &cmdName)
            != TCL_OK) {
        return TCL_ERROR;
    }

    cmd = Tcl_FindCommand(interp, cmdName, contextNs, /* flags */ 0);

    /*
     *    Need the NULL test, or the test will fail if cmd is NULL
     */
    if (cmd == NULL || ! Itcl_IsObject(cmd)) {
        Tcl_SetObjResult(interp, Tcl_NewWideIntObj(0));
	ckfree((char *)cmdName);
        return TCL_OK;
    }

    /*
     *    Handle the case when the -class flag is given
     */
    if (classFlag) {
	ItclObject *contextIoPtr;
        if (Itcl_FindObject(interp, cmdName, &contextIoPtr) != TCL_OK) {
            return TCL_ERROR;
        }
	if (contextIoPtr == NULL) {
	   /* seems that we are in constructor, so look for currIoPtr in info structure */
	   contextIoPtr = iclsPtr->infoPtr->currIoPtr;
	}
        if (! Itcl_ObjectIsa(contextIoPtr, iclsPtr)) {
            Tcl_SetObjResult(interp, Tcl_NewWideIntObj(0));
	    ckfree((char *)cmdName);
            return TCL_OK;
        }

    }

    /*
     *    Got this far, so assume that it is a valid object
     */
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(1));
    ckfree(cmdName);

    return TCL_OK;
}



/*
 * ------------------------------------------------------------------------
 *  Itcl_IsClassCmd()
 *
 *  Invoked by Tcl whenever the user issues an "itcl::is class"
 *  command to test whether the argument is an itcl class or not
 *  syntax:
 *
 *    itcl::is class commandname
 *
 *  Returns 1 if it is a class, 0 otherwise
 * ------------------------------------------------------------------------
 */
int
Itcl_IsClassCmd(
    ClientData clientData,   /* class/object info */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{

    char           *cname;
    char           *name;
    ItclClass      *iclsPtr = NULL;
    Tcl_Namespace  *contextNs = NULL;

    /*
     *    Need itcl::is class classname
     */
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "commandname");
        return TCL_ERROR;
    }

    name = Tcl_GetString(objv[1]);

    /*
     *    The object name may be a scoped value of the form
     *    "namespace inscope <namesp> <command>".  If it is,
     *    decode it.
     */
    if (Itcl_DecodeScopedCommand(interp, name, &contextNs, &cname) != TCL_OK) {
        return TCL_ERROR;
    }

    iclsPtr = Itcl_FindClass(interp, cname, /* no autoload */ 0);

    /*
     *    If classDefn is NULL, then it wasn't found, hence it
     *    isn't a class
     */
    if (iclsPtr != NULL) {
        Tcl_SetObjResult(interp, Tcl_NewWideIntObj(1));
    } else {
        Tcl_SetObjResult(interp, Tcl_NewWideIntObj(0));
    }

    ckfree(cname);

    return TCL_OK;

} /* end Itcl_IsClassCmd function */

/*
 * ------------------------------------------------------------------------
 *  Itcl_FilterCmd()
 *
 *  Used to add a filter command to an object which is called just before
 *  a method/proc of a class is executed
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_FilterAddCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj **newObjv;
    int result;

    ItclShowArgs(1, "Itcl_FilterCmd", objc, objv);
/*    Tcl_Namespace *contextNs = Tcl_GetCurrentNamespace(interp); */
/* FIXME need to change the chain command to do the same here as the TclOO next command !! */
    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "<className> <filterName> ?<filterName> ...?");
        return TCL_ERROR;
    }
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+1));
    newObjv[0] = Tcl_NewStringObj("::oo::define", -1);
    Tcl_IncrRefCount(newObjv[0]);
    newObjv[1] = objv[1];
    newObjv[2] = Tcl_NewStringObj("filter", -1);
    Tcl_IncrRefCount(newObjv[2]);
    memcpy(newObjv+3, objv+2, sizeof(Tcl_Obj *)*(objc-2));
    ItclShowArgs(1, "Itcl_FilterAddCmd2", objc+1, newObjv);
    result = Tcl_EvalObjv(interp, objc+1, newObjv, 0);
    Tcl_DecrRefCount(newObjv[0]);
    Tcl_DecrRefCount(newObjv[2]);

    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_FilterDeleteCmd()
 *
 *  used to delete filter commands of a class or object
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_FilterDeleteCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclShowArgs(1, "Itcl_FilterDeleteCmd", objc, objv);
/*    Tcl_Namespace *contextNs = Tcl_GetCurrentNamespace(interp); */

    Tcl_AppendResult(interp, "::itcl::filter delete command not yet implemented", NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ForwardAddCmd()
 *
 *  Used to similar to iterp alias to forward the call of a method
 *  to another method within the class
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_ForwardAddCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *prefixObj;
    Tcl_Method mPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;

    ItclShowArgs(1, "Itcl_ForwardAddCmd", objc, objv);
    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "<forwardName> <targetName> ?<arg> ...?");
        return TCL_ERROR;
    }
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
	Tcl_HashEntry *hPtr;
        hPtr = Tcl_FindHashEntry(&infoPtr->nameClasses, (char *)objv[1]);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "class: \"", Tcl_GetString(objv[1]),
	            "\" not found", NULL);
	    return TCL_ERROR;
	}
	iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
    }
    prefixObj = Tcl_NewListObj(objc-2, objv+2);
    mPtr = Itcl_NewForwardClassMethod(interp, iclsPtr->clsPtr, 1,
            objv[1], prefixObj);
    if (mPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ForwardDeleteCmd()
 *
 *  used to delete forwarded commands of a class or object
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_ForwardDeleteCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclShowArgs(1, "Itcl_ForwardDeleteCmd", objc, objv);
/*    Tcl_Namespace *contextNs = Tcl_GetCurrentNamespace(interp); */

    Tcl_AppendResult(interp, "::itcl::forward delete command not yet implemented", NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_MixinAddCmd()
 *
 *  Used to add the methods of a class to another class without heritance
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_MixinAddCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj **newObjv;
    int result;

    ItclShowArgs(1, "Itcl_MixinAddCmd", objc, objv);
    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "<className> <mixinName> ?<mixinName> ...?");
        return TCL_ERROR;
    }
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+1));
    newObjv[0] = Tcl_NewStringObj("::oo::define", -1);
    Tcl_IncrRefCount(newObjv[0]);
    newObjv[1] = objv[1];
    newObjv[2] = Tcl_NewStringObj("mixin", -1);
    Tcl_IncrRefCount(newObjv[2]);
    memcpy(newObjv+3, objv+2, sizeof(Tcl_Obj *)*(objc-2));
    ItclShowArgs(1, "Itcl_MixinAddCmd2", objc+1, newObjv);
    result = Tcl_EvalObjv(interp, objc+1, newObjv, 0);
    Tcl_DecrRefCount(newObjv[0]);
    Tcl_DecrRefCount(newObjv[2]);

    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_MixinDeleteCmd()
 *
 *  Used to delete the methods of a class to another class without heritance
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_MixinDeleteCmd(
    ClientData dummy,        /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclShowArgs(1, "Itcl_MixinDeleteCmd", objc, objv);
/*    Tcl_Namespace *contextNs = Tcl_GetCurrentNamespace(interp); */

    Tcl_AppendResult(interp, "::itcl::mixin delete command not yet implemented", NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_NWidgetCmd()
 *
 *  Used to build an [incr Tcl] nwidget
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_NWidgetCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *iclsPtr;
    int result;

    iclsPtr = NULL;
    ItclShowArgs(0, "Itcl_NWidgetCmd", objc-1, objv);
    result = ItclClassBaseCmd(clientData, interp, ITCL_ECLASS|ITCL_NWIDGET, objc, objv,
            &iclsPtr);
    if (result != TCL_OK) {
        return result;
    }
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Itcl_NWidgetCmd!iclsPtr == NULL\n", NULL);
        result = TCL_ERROR;
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AddOptionCmd()
 *
 *  Used to build an option to an [incr Tcl] nwidget/eclass
 *
 *  Syntax: ::itcl::addoption <nwidget class> <protection> <optionName> <defaultValue>
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_AddOptionCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    const char *protectionStr;
    int pLevel;
    int result;

    result = TCL_OK;
    infoPtr = (ItclObjectInfo *)clientData;
    ItclShowArgs(1, "Itcl_AddOptionCmd", objc, objv);
    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv,
	        "className protection option optionName ...");
	return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&infoPtr->nameClasses, (char *)objv[1]);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "class \"", Tcl_GetString(objv[1]),
	        "\" not found", NULL);
        return TCL_ERROR;
    }
    iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
    protectionStr = Tcl_GetString(objv[2]);
    pLevel = -1;
    if (strcmp(protectionStr, "public") == 0) {
        pLevel = ITCL_PUBLIC;
    }
    if (strcmp(protectionStr, "protected") == 0) {
        pLevel = ITCL_PROTECTED;
    }
    if (strcmp(protectionStr, "private") == 0) {
        pLevel = ITCL_PRIVATE;
    }
    if (pLevel == -1) {
	Tcl_AppendResult(interp, "bad protection \"", protectionStr, "\"",
	        NULL);
        return TCL_ERROR;
    }
    Itcl_PushStack(iclsPtr, &infoPtr->clsStack);
    result = Itcl_ClassOptionCmd(clientData, interp, objc-2, objv+2);
    Itcl_PopStack(&infoPtr->clsStack);
    if (result != TCL_OK) {
        return result;
    }
    result = DelegatedOptionsInstall(interp, iclsPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AddObjectOptionCmd()
 *
 *  Used to build an option for an [incr Tcl] object
 *
 *  Syntax: ::itcl::addobjectoption <object> <protection> option <optionSpec>
 *     ?-default <defaultValue>?
 *     ?-configuremethod <configuremethod>?
 *     ?-validatemethod <validatemethod>?
 *     ?-cgetmethod <cgetmethod>?
 *     ?-configuremethodvar <configuremethodvar>?
 *     ?-validatemethodvar <validatemethodvar>?
 *     ?-cgetmethodvar <cgetmethodvar>?
 *     ?-readonly?
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_AddObjectOptionCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_Command cmd;
    Tcl_Obj *objPtr;
    ItclObjectInfo *infoPtr;
    ItclObject *ioPtr;
    ItclOption *ioptPtr;
    const char *protectionStr;
    int pLevel;
    int isNew;

    ioptPtr = NULL;
    infoPtr = (ItclObjectInfo *)clientData;
    ItclShowArgs(1, "Itcl_AddObjectOptionCmd", objc, objv);
    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv,
	        "objectName protection option optionName ...");
	return TCL_ERROR;
    }

    cmd = Tcl_FindCommand(interp, Tcl_GetString(objv[1]), NULL, 0);
    if (cmd == NULL) {
	Tcl_AppendResult(interp, "object \"", Tcl_GetString(objv[1]),
	        "\" not found", NULL);
        return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&infoPtr->objectCmds, (char *)cmd);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "object \"", Tcl_GetString(objv[1]),
	        "\" not found", NULL);
        return TCL_ERROR;
    }
    ioPtr = (ItclObject *)Tcl_GetHashValue(hPtr);
    protectionStr = Tcl_GetString(objv[2]);
    pLevel = -1;
    if (strcmp(protectionStr, "public") == 0) {
        pLevel = ITCL_PUBLIC;
    }
    if (strcmp(protectionStr, "protected") == 0) {
        pLevel = ITCL_PROTECTED;
    }
    if (strcmp(protectionStr, "private") == 0) {
        pLevel = ITCL_PRIVATE;
    }
    if (pLevel == -1) {
	Tcl_AppendResult(interp, "bad protection \"", protectionStr, "\"",
	        NULL);
        return TCL_ERROR;
    }
    infoPtr->protection = pLevel;
    if (ItclParseOption(infoPtr, interp, objc-3, objv+3, NULL, ioPtr,
             &ioptPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    objPtr = Tcl_NewObj();
    Tcl_GetCommandFullName(interp, ioPtr->accessCmd, objPtr);
    ioptPtr->fullNamePtr = Tcl_NewStringObj(
            Tcl_GetString(ioPtr->namePtr), -1);
    Tcl_AppendToObj(ioptPtr->fullNamePtr, "::", 2);
    Tcl_AppendToObj(ioptPtr->fullNamePtr, Tcl_GetString(ioptPtr->namePtr), -1);
    Tcl_IncrRefCount(ioptPtr->fullNamePtr);
    hPtr = Tcl_CreateHashEntry(&ioPtr->objectOptions,
            (char *)ioptPtr->namePtr, &isNew);
    Tcl_SetHashValue(hPtr, ioptPtr);
    ItclSetInstanceVar(interp, "itcl_options",
            Tcl_GetString(ioptPtr->namePtr),
            Tcl_GetString(ioptPtr->defaultValuePtr), ioPtr, NULL);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AddDelegatedOptionCmd()
 *
 *  Used to build an option to an [incr Tcl] nwidget/eclass
 *
 *  Syntax: ::itcl::adddelegatedoption <nwidget object> <optionName> <defaultValue>
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_AddDelegatedOptionCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_Command cmd;
    ItclObjectInfo *infoPtr;
    ItclObject *ioPtr;
    ItclDelegatedOption *idoPtr;
    int isNew;
    int result;

    result = TCL_OK;
    infoPtr = (ItclObjectInfo *)clientData;
    ItclShowArgs(1, "Itcl_AddDelegatedOptionCmd", objc, objv);
    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv,
	        "className protection option optionName ...");
	return TCL_ERROR;
    }

    cmd = Tcl_FindCommand(interp, Tcl_GetString(objv[1]), NULL, 0);
    if (cmd == NULL) {
	Tcl_AppendResult(interp, "object \"", Tcl_GetString(objv[1]),
	        "\" not found", NULL);
        return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&infoPtr->objectCmds, (char *)cmd);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "object \"", Tcl_GetString(objv[1]),
	        "\" not found", NULL);
        return TCL_ERROR;
    }
    ioPtr = (ItclObject *)Tcl_GetHashValue(hPtr);
    result = Itcl_HandleDelegateOptionCmd(interp, ioPtr, NULL, &idoPtr,
            objc-3, objv+3);
    if (result != TCL_OK) {
        return result;
    }
    hPtr = Tcl_CreateHashEntry(&ioPtr->objectDelegatedOptions,
            (char *)idoPtr->namePtr, &isNew);
    Tcl_SetHashValue(hPtr, idoPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AddDelegatedFunctionCmd()
 *
 *  Used to build an function to an [incr Tcl] nwidget/eclass
 *
 *  Syntax: ::itcl::adddelegatedfunction <nwidget object> <fucntionName> ...
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_AddDelegatedFunctionCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_Command cmd;
    Tcl_Obj *componentNamePtr;
    ItclObjectInfo *infoPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclDelegatedFunction *idmPtr;
    ItclHierIter hier;
    const char *val;
    int isNew;
    int result;

    result = TCL_OK;
    infoPtr = (ItclObjectInfo *)clientData;
    ItclShowArgs(1, "Itcl_AddDelegatedFunctionCmd", objc, objv);
    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv,
	        "className protection method/proc functionName ...");
	return TCL_ERROR;
    }

    cmd = Tcl_FindCommand(interp, Tcl_GetString(objv[1]), NULL, 0);
    if (cmd == NULL) {
	Tcl_AppendResult(interp, "object \"", Tcl_GetString(objv[1]),
	        "\" not found", NULL);
        return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&infoPtr->objectCmds, (char *)cmd);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "object \"", Tcl_GetString(objv[1]),
	        "\" not found", NULL);
        return TCL_ERROR;
    }
    ioPtr = (ItclObject *)Tcl_GetHashValue(hPtr);
    result = Itcl_HandleDelegateMethodCmd(interp, ioPtr, NULL, &idmPtr,
            objc-3, objv+3);
    if (result != TCL_OK) {
        return result;
    }
    componentNamePtr = idmPtr->icPtr->namePtr;
    Itcl_InitHierIter(&hier, ioPtr->iclsPtr);
    while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
        hPtr = Tcl_FindHashEntry(&iclsPtr->components, (char *)
                componentNamePtr);
	if (hPtr != NULL) {
	    break;
	}
    }
    Itcl_DeleteHierIter(&hier);
    val = Itcl_GetInstanceVar(interp,
            Tcl_GetString(componentNamePtr), ioPtr, iclsPtr);
    componentNamePtr = Tcl_NewStringObj(val, -1);
    Tcl_IncrRefCount(componentNamePtr);
    DelegateFunction(interp, ioPtr, ioPtr->iclsPtr, componentNamePtr, idmPtr);
    hPtr = Tcl_CreateHashEntry(&ioPtr->objectDelegatedFunctions,
            (char *)idmPtr->namePtr, &isNew);
    Tcl_DecrRefCount(componentNamePtr);
    Tcl_SetHashValue(hPtr, idmPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AddComponentCmd()
 *
 *  Used to add a component to an [incr Tcl] nwidget/eclass
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_AddComponentCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_DString buffer;
    Tcl_DString buffer2;
    Tcl_Namespace *varNsPtr;
    Tcl_Namespace *nsPtr;
    Tcl_CallFrame frame;
    Tcl_Var varPtr;
    ItclVarLookup *vlookup;
    ItclObject *contextIoPtr;
    ItclClass *contextIclsPtr;
    ItclComponent *icPtr;
    ItclVariable *ivPtr;
    const char *varName;
    int isNew;
    int result;

    result = TCL_OK;
    contextIoPtr = NULL;
    ItclShowArgs(1, "Itcl_AddComponentCmd", objc, objv);
    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv,
	        "objectName componentName");
	return TCL_ERROR;
    }
    if (Itcl_FindObject(interp, Tcl_GetString(objv[1]), &contextIoPtr)
            != TCL_OK) {
        return TCL_ERROR;
    }
    if (contextIoPtr == NULL) {
	Tcl_AppendResult(interp, "Itcl_AddComponentCmd contextIoPtr "
	       "for \"", Tcl_GetString(objv[1]), "\" == NULL", NULL);
        return TCL_ERROR;
    }
    contextIclsPtr = contextIoPtr->iclsPtr;
    hPtr = Tcl_CreateHashEntry(&contextIoPtr->objectComponents, (char *)objv[2],
            &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "Itcl_AddComponentCmd component \"",
	        Tcl_GetString(objv[2]), "\" already exists for object \"",
		Tcl_GetString(objv[1]), "\"", NULL);
        return TCL_ERROR;
    }
    if (ItclCreateComponent(interp, contextIclsPtr, objv[2], /* not common */0,
            &icPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    ItclAddClassComponentDictInfo(interp, contextIclsPtr, icPtr);
    contextIclsPtr->numVariables++;
    Tcl_SetHashValue(hPtr, icPtr);
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
    Tcl_DStringAppend(&buffer,
	    (Tcl_GetObjectNamespace(contextIoPtr->oPtr))->fullName, -1);
    Tcl_DStringAppend(&buffer, contextIclsPtr->nsPtr->fullName, -1);
    varNsPtr = Tcl_FindNamespace(interp, Tcl_DStringValue(&buffer),
        NULL, 0);
    hPtr = Tcl_FindHashEntry(&contextIclsPtr->variables, (char *)objv[2]);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "Itcl_AddComponentCmd cannot find component",
	        " \"", Tcl_GetString(objv[2]), "\"in class variables", NULL);
        return TCL_ERROR;
    }
    ivPtr = (ItclVariable *)Tcl_GetHashValue(hPtr);
    /* add entry to the virtual tables */
    vlookup = (ItclVarLookup *)ckalloc(sizeof(ItclVarLookup));
    vlookup->ivPtr = ivPtr;
    vlookup->usage = 0;
    vlookup->leastQualName = NULL;

    /*
     *  If this variable is PRIVATE to another class scope,
     *  then mark it as "inaccessible".
     */
    vlookup->accessible = (ivPtr->protection != ITCL_PRIVATE ||
        ivPtr->iclsPtr == contextIclsPtr);

    vlookup->varNum = contextIclsPtr->numInstanceVars++;
    /*
     *  Create all possible names for this variable and enter
     *  them into the variable resolution table:
     *     var
     *     class::var
     *     namesp1::class::var
     *     namesp2::namesp1::class::var
     *     ...
     */
    Tcl_DStringSetLength(&buffer, 0);
    Tcl_DStringAppend(&buffer, Tcl_GetString(ivPtr->namePtr), -1);
    nsPtr = contextIclsPtr->nsPtr;

    Tcl_DStringInit(&buffer2);
    while (1) {
        hPtr = Tcl_CreateHashEntry(&contextIclsPtr->resolveVars,
            Tcl_DStringValue(&buffer), &isNew);

        if (isNew) {
            Tcl_SetHashValue(hPtr, vlookup);
            vlookup->usage++;

            if (!vlookup->leastQualName) {
                vlookup->leastQualName = (char *)
                    Tcl_GetHashKey(&contextIclsPtr->resolveVars, hPtr);
            }
        }

        if (nsPtr == NULL) {
            break;
        }
        Tcl_DStringSetLength(&buffer2, 0);
        Tcl_DStringAppend(&buffer2, Tcl_DStringValue(&buffer), -1);
        Tcl_DStringSetLength(&buffer, 0);
        Tcl_DStringAppend(&buffer, nsPtr->name, -1);
        Tcl_DStringAppend(&buffer, "::", -1);
        Tcl_DStringAppend(&buffer, Tcl_DStringValue(&buffer2), -1);

        nsPtr = nsPtr->parentPtr;
    }
    Tcl_DStringFree(&buffer2);
    Tcl_DStringFree(&buffer);



    varName = Tcl_GetString(ivPtr->namePtr);
    /* now initialize the variable */
    if (Itcl_PushCallFrame(interp, &frame, varNsPtr,
        /*isProcCallFrame*/0) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_SetVar2(interp, varName, NULL,
            "", TCL_NAMESPACE_ONLY) == NULL) {
        Tcl_AppendResult(interp, "INTERNAL ERROR cannot set",
                " variable \"", varName, "\"\n", NULL);
        result = TCL_ERROR;
    }
    Itcl_PopCallFrame(interp);
    varPtr = Tcl_NewNamespaceVar(interp, varNsPtr,
            Tcl_GetString(ivPtr->namePtr));
    hPtr = Tcl_CreateHashEntry(&contextIoPtr->objectVariables,
            (char *)ivPtr, &isNew);
    if (isNew) {
	Itcl_PreserveVar(varPtr);
        Tcl_SetHashValue(hPtr, varPtr);
    } else {
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_SetComponentCmd()
 *
 *  Used to set a component for an [incr Tcl] nwidget/eclass
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_SetComponentCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    FOREACH_HASH_DECLS;
    ItclClass *iclsPtr;
    ItclObject *contextIoPtr;
    ItclClass *contextIclsPtr;
    ItclComponent *icPtr;
    ItclDelegatedOption *idoPtr;
    ItclHierIter hier;
    const char *name;
    const char *val;
    int result;

    result = TCL_OK;
    contextIoPtr = NULL;
    ItclShowArgs(1, "Itcl_SetComponentCmd", objc, objv);
    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv,
	        "objectName componentName value");
	return TCL_ERROR;
    }
    name = Tcl_GetString(objv[1]);
    if (Itcl_FindObject(interp, name, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (contextIoPtr == NULL) {
	Tcl_AppendResult(interp, "Itcl_SetComponentCmd contextIoPtr "
	       "for \"", Tcl_GetString(objv[1]), "\" == NULL", NULL);
        return TCL_ERROR;
    }
    Itcl_InitHierIter(&hier, contextIoPtr->iclsPtr);
    hPtr = NULL;
    while ((contextIclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
        hPtr = Tcl_FindHashEntry(&contextIclsPtr->components, (char *)objv[2]);
        if (hPtr != NULL) {
	    break;
	}
    }
    Itcl_DeleteHierIter(&hier);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "object \"", Tcl_GetString(objv[1]),
	        "\" has no component \"", Tcl_GetString(objv[2]), "\"", NULL);
        return TCL_ERROR;
    }
    icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
    val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr), NULL,
            contextIoPtr, contextIclsPtr);
    if ((val != NULL) && (strlen(val) != 0)) {
        /* delete delegated options to the old component here !! */
        Itcl_InitHierIter(&hier, contextIoPtr->iclsPtr);
        while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
            FOREACH_HASH_VALUE(idoPtr, &iclsPtr->delegatedOptions) {
	        if (strcmp(Tcl_GetString(idoPtr->icPtr->namePtr),
		        Tcl_GetString(objv[2])) == 0) {
		    Tcl_DeleteHashEntry(hPtr);
	        }
	    }
        }
        Itcl_DeleteHierIter(&hier);
    }
    if (ItclSetInstanceVar(interp, Tcl_GetString(icPtr->namePtr), NULL,
             Tcl_GetString(objv[3]), contextIoPtr, contextIclsPtr) == NULL) {
	return TCL_ERROR;
    }
    val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr), NULL,
            contextIoPtr, contextIclsPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ExtendedClassCmd()
 *
 *  Used to create an [incr Tcl] extended class.
 *  An extended class is like a class with additional functionality/
 *  commands
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_ExtendedClassCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *iclsPtr;
    int result;

    ItclShowArgs(1, "Itcl_ExtendedClassCmd", objc-1, objv);
    result = ItclClassBaseCmd(clientData, interp, ITCL_ECLASS, objc, objv,
            &iclsPtr);
    if ((iclsPtr == NULL) && (result == TCL_OK)) {
        ItclShowArgs(0, "Itcl_ExtendedClassCmd iclsPtr == NULL", objc-1, objv);
        return TCL_ERROR;
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_TypeClassCmd()
 *
 *  Used to create an [incr Tcl] type class.
 *  An type class is like a class with additional functionality/
 *  commands. it has no methods and vars but only the equivalent
 *  of proc and common namely typemethod and typevariable
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_TypeClassCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *objPtr;
    ItclClass *iclsPtr;
    int result;

    ItclShowArgs(1, "Itcl_TypeClassCmd", objc-1, objv);
    result = ItclClassBaseCmd(clientData, interp, ITCL_TYPE, objc, objv,
            &iclsPtr);
    if ((iclsPtr == NULL) && (result == TCL_OK)) {
        ItclShowArgs(0, "Itcl_TypeClassCmd iclsPtr == NULL", objc-1, objv);
        return TCL_ERROR;
    }
    if (result != TCL_OK) {
        return result;
    }
    /* we handle create by ourself !! */
    objPtr = Tcl_NewStringObj("oo::objdefine ", -1);
    Tcl_AppendToObj(objPtr, iclsPtr->nsPtr->fullName, -1);
    Tcl_AppendToObj(objPtr, " unexport create", -1);
    Tcl_IncrRefCount(objPtr);
    result = Tcl_EvalObjEx(interp, objPtr, 0);
    Tcl_DecrRefCount(objPtr);
    objPtr = Tcl_NewStringObj(iclsPtr->nsPtr->fullName, -1);
    Tcl_SetObjResult(interp, objPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassHullTypeCmd()
 *
 *  Used to set a hulltype for a widget
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_ClassHullTypeCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *iclsPtr;
    ItclObjectInfo *infoPtr;
    const char *hullTypeName;
    int correctArg;

    infoPtr = (ItclObjectInfo *)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    ItclShowArgs(1, "Itcl_ClassHullTypeCmd", objc-1, objv);
    if (iclsPtr->flags & ITCL_TYPE) {
        Tcl_AppendResult(interp, "can't set hulltype for ::itcl::type",
	        NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_WIDGETADAPTOR) {
        Tcl_AppendResult(interp, "can't set hulltype for ",
	        "::itcl::widgetadaptor", NULL);
        return TCL_ERROR;
    }
    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # args should be: hulltype ",
	        "<hullTypeName>", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_WIDGET) {
        hullTypeName = Tcl_GetString(objv[1]);
        if (iclsPtr->hullTypePtr != NULL) {
	    Tcl_AppendResult(interp, "too many hulltype statements", NULL);
	    return TCL_ERROR;
	}
        correctArg = 0;
        if (strcmp(hullTypeName, "frame") == 0) {
	    iclsPtr->flags |= ITCL_WIDGET_FRAME;
            correctArg = 1;
        }
        if (strcmp(hullTypeName, "labelframe") == 0) {
	    iclsPtr->flags |= ITCL_WIDGET_LABEL_FRAME;
            correctArg = 1;
        }
        if (strcmp(hullTypeName, "toplevel") == 0) {
	    iclsPtr->flags |= ITCL_WIDGET_TOPLEVEL;
            correctArg = 1;
        }
        if (strcmp(hullTypeName, "ttk::frame") == 0) {
	    iclsPtr->flags |= ITCL_WIDGET_TTK_FRAME;
            correctArg = 1;
        }
        if (strcmp(hullTypeName, "ttk::labelframe") == 0) {
	    iclsPtr->flags |= ITCL_WIDGET_TTK_LABEL_FRAME;
            correctArg = 1;
        }
        if (strcmp(hullTypeName, "ttk::toplevel") == 0) {
	    iclsPtr->flags |= ITCL_WIDGET_TTK_TOPLEVEL;
            correctArg = 1;
        }
        if (!correctArg) {
            Tcl_AppendResult(interp,
	            "syntax: must be hulltype frame|toplevel|labelframe|",
		    "ttk::frame|ttk::toplevel|ttk::labelframe", NULL);
            return TCL_ERROR;
        }
        iclsPtr->hullTypePtr = Tcl_NewStringObj(hullTypeName, -1);
	Tcl_IncrRefCount(iclsPtr->hullTypePtr);
        return TCL_OK;
    }
    Tcl_AppendResult(interp, "invalid command name \"hulltype\"", NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassWidgetClassCmd()
 *
 *  Used to set a widgetclass for a widget
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_ClassWidgetClassCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *iclsPtr;
    ItclObjectInfo *infoPtr;
    const char *widgetClassName;

    infoPtr = (ItclObjectInfo *)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    ItclShowArgs(1, "Itcl_ClassWidgetClassCmd", objc-1, objv);
    if (iclsPtr->flags & ITCL_TYPE) {
        Tcl_AppendResult(interp, "can't set widgetclass for ::itcl::type",
	        NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_WIDGETADAPTOR) {
        Tcl_AppendResult(interp, "can't set widgetclass for ",
	        "::itcl::widgetadaptor", NULL);
        return TCL_ERROR;
    }
    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # args should be: widgetclass ",
	        "<widgetClassName>", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_WIDGET) {
        widgetClassName = Tcl_GetString(objv[1]);
	if (!isupper(UCHAR(*widgetClassName))) {
	    Tcl_AppendResult(interp, "widgetclass \"", widgetClassName,
	            "\" does not begin with an uppercase letter", NULL);
            return TCL_ERROR;
	}
        if (iclsPtr->widgetClassPtr != NULL) {
	    Tcl_AppendResult(interp, "too many widgetclass statements", NULL);
	    return TCL_ERROR;
	}
        iclsPtr->widgetClassPtr = Tcl_NewStringObj(widgetClassName, -1);
	Tcl_IncrRefCount(iclsPtr->widgetClassPtr);
        return TCL_OK;
    }
    Tcl_AppendResult(interp, "invalid command name \"widgetclass\"", NULL);
    return TCL_ERROR;
}
