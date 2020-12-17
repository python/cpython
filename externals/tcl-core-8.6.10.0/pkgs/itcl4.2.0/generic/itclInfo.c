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
 *  These procedures handle built-in class methods, including the
 *  "isa" method (to query hierarchy info) and the "info" method
 *  (to query class/object data).
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
#include "itclInt.h"

static Tcl_ObjCmdProc Itcl_BiInfoClassOptionsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoComponentsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDefaultCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedCmd;
static Tcl_ObjCmdProc Itcl_BiInfoExtendedClassCmd;
static Tcl_ObjCmdProc Itcl_BiInfoInstancesCmd;
static Tcl_ObjCmdProc Itcl_BiInfoHullTypeCmd;
static Tcl_ObjCmdProc Itcl_BiInfoMethodCmd;
static Tcl_ObjCmdProc Itcl_BiInfoMethodsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoOptionsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoTypeCmd;
static Tcl_ObjCmdProc Itcl_BiInfoTypeMethodCmd;
static Tcl_ObjCmdProc Itcl_BiInfoTypeMethodsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoTypesCmd;
static Tcl_ObjCmdProc Itcl_BiInfoTypeVarsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoTypeVariableCmd;
static Tcl_ObjCmdProc Itcl_BiInfoVariablesCmd;
static Tcl_ObjCmdProc Itcl_BiInfoWidgetadaptorCmd;
static Tcl_ObjCmdProc Itcl_BiInfoWidgetCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedOptionsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedMethodsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedTypeMethodsCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedOptionCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedMethodCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedTypeMethodCmd;
static Tcl_ObjCmdProc Itcl_BiInfoDelegatedUnknownCmd;
static Tcl_ObjCmdProc Itcl_BiInfoContextCmd;

typedef struct InfoMethod {
    const char* name;        /* method name */
    const char* usage;       /* string describing usage */
    Tcl_ObjCmdProc *proc;    /* implementation C proc */
    int flags;               /* which class commands have it */
} InfoMethod;

static const InfoMethod InfoMethodList[] = {
    { "args",
        "procname",
	Itcl_BiInfoArgsCmd,
	ITCL_CLASS|ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "body",
        "procname",
	Itcl_BiInfoBodyCmd,
	ITCL_CLASS|ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "class",
        "",
	Itcl_BiInfoClassCmd,
	ITCL_CLASS|ITCL_WIDGET|ITCL_ECLASS
    },
    { "classoptions",
        "?pattern?",
	Itcl_BiInfoClassOptionsCmd,
	ITCL_ECLASS
    },
    { "component",
        "?name? ?-inherit? ?-value?",
        Itcl_BiInfoComponentCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "context",
        "",
        Itcl_BiInfoContextCmd,
	ITCL_ECLASS
    },
    { "components",
        "?pattern?",
	Itcl_BiInfoComponentsCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "default",
        "method aname varname",
	Itcl_BiInfoDefaultCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "delegated",
        "?name? ?-inherit? ?-value?",
        Itcl_BiInfoDelegatedCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "extendedclass",
        "",
        Itcl_BiInfoExtendedClassCmd,
	ITCL_ECLASS
    },
    { "function",
        "?name? ?-protection? ?-type? ?-name? ?-args? ?-body?",
        Itcl_BiInfoFunctionCmd,
	ITCL_CLASS|ITCL_ECLASS
    },
    { "heritage",
        "",
	Itcl_BiInfoHeritageCmd,
	ITCL_CLASS|ITCL_WIDGET|ITCL_ECLASS
    },
    { "hulltype",
        "",
	Itcl_BiInfoHullTypeCmd,
	ITCL_WIDGET
    },
    { "hulltypes",
        "?pattern?",
        Itcl_BiInfoUnknownCmd,
	ITCL_WIDGETADAPTOR|ITCL_WIDGET
    },
    { "inherit",
        "",
	Itcl_BiInfoInheritCmd,
	ITCL_CLASS|ITCL_WIDGET|ITCL_ECLASS
    },
    { "instances",
        "?pattern?",
        Itcl_BiInfoInstancesCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET
    },
    { "method",
        "?name? ?-protection? ?-type? ?-name? ?-args? ?-body?",
        Itcl_BiInfoMethodCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "methods",
        "?pattern?",
        Itcl_BiInfoMethodsCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "option",
        "?name? ?-protection? ?-resource? ?-class? ?-name? ?-default? \
?-cgetmethod? ?-configuremethod? ?-validatemethod? \
?-cgetmethodvar? ?-configuremethodvar? ?-validatemethodvar? \
?-value?",
        Itcl_BiInfoOptionCmd,
	ITCL_WIDGET|ITCL_ECLASS
    },
    { "options",
        "?pattern?",
	Itcl_BiInfoOptionsCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "type",
        "",
	Itcl_BiInfoTypeCmd,
	ITCL_TYPE|ITCL_WIDGET|ITCL_ECLASS
    },
    { "typemethod",
        "?name? ?-protection? ?-type? ?-name? ?-args? ?-body?",
        Itcl_BiInfoTypeMethodCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "typemethods",
        "?pattern?",
	Itcl_BiInfoTypeMethodsCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "types",
        "?pattern?",
	Itcl_BiInfoTypesCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "typevariable",
        "?name? ?-protection? ?-type? ?-name? ?-init? ?-value?",
        Itcl_BiInfoTypeVariableCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "typevars",
        "?pattern?",
	Itcl_BiInfoTypeVarsCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "variable",
        "?name? ?-protection? ?-type? ?-name? ?-init? ?-value? ?-config? ?-scope?",
        Itcl_BiInfoVariableCmd,
	ITCL_CLASS|ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "variables",
        "?pattern?",
        Itcl_BiInfoVariablesCmd,
	ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "vars",
        "?pattern?",
	Itcl_BiInfoVarsCmd,
	ITCL_CLASS|ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "widget",
        "",
        Itcl_BiInfoWidgetCmd,
	ITCL_WIDGET
    },
    { "widgets",
        "?pattern?",
        Itcl_BiInfoUnknownCmd,
	ITCL_WIDGET
    },
    { "widgetclasses",
        "?pattern?",
        Itcl_BiInfoUnknownCmd,
	ITCL_WIDGET
    },
    { "widgetadaptor",
        "",
        Itcl_BiInfoWidgetadaptorCmd,
	ITCL_WIDGETADAPTOR
    },
    { "widgetadaptors",
        "?pattern?",
        Itcl_BiInfoUnknownCmd,
	ITCL_WIDGETADAPTOR
    },
    { NULL,
        NULL,
	NULL,
	0
    }
};

struct NameProcMap2 {
    const char* name;        /* method name */
    const char* usage;       /* string describing usage */
    Tcl_ObjCmdProc *proc;    /* implementation C proc */
    int flags;               /* which class commands have it */
};

static const struct NameProcMap2 infoCmdsDelegated2[] = {
    { "::itcl::builtin::Info::delegated::methods",
	"?pattern?",
        Itcl_BiInfoDelegatedMethodsCmd,
        ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "::itcl::builtin::Info::delegated::typemethods",
	"?pattern?",
        Itcl_BiInfoDelegatedTypeMethodsCmd,
        ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "::itcl::builtin::Info::delegated::options",
	"?pattern?",
        Itcl_BiInfoDelegatedOptionsCmd,
        ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { "::itcl::builtin::Info::delegated::method",
	"methodName",
        Itcl_BiInfoDelegatedMethodCmd,
        ITCL_ECLASS
    },
    { "::itcl::builtin::Info::delegated::typemethod",
	"methodName",
        Itcl_BiInfoDelegatedTypeMethodCmd,
        ITCL_ECLASS
    },
    { "::itcl::builtin::Info::delegated::option",
	"methodName",
        Itcl_BiInfoDelegatedOptionCmd,
        ITCL_ECLASS
    },
    { "::itcl::builtin::Info::delegated::unknown",
	"",
        Itcl_BiInfoDelegatedUnknownCmd,
        ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS
    },
    { NULL, NULL, NULL, 0 }
};

static void ItclGetInfoUsage(Tcl_Interp *interp, Tcl_Obj*objPtr,
	ItclObjectInfo *infoPtr, ItclClass *iclsPtr);


/*
 * ------------------------------------------------------------------------
 *  ItclInfoInit()
 *
 *  Creates a namespace full of built-in methods/procs for [incr Tcl]
 *  classes.  This includes things like the "info"
 *  for querying class info.  Usually invoked by Itcl_Init() when
 *  [incr Tcl] is first installed into an interpreter.
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */

static int
InfoGutsFinish(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_CallFrame *framePtr = (Tcl_CallFrame *) data[0];
    ItclObjectInfo *infoPtr = (ItclObjectInfo *) data[1];
    ItclCallContext *cPtr = (ItclCallContext *) data[2];
    ItclCallContext *popped;

    Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&infoPtr->frameContext,
	    (char *) framePtr);

    Itcl_Stack *stackPtr = (Itcl_Stack *) Tcl_GetHashValue(hPtr);

    popped = (ItclCallContext *)Itcl_PopStack(stackPtr);

    if (Itcl_GetStackSize(stackPtr) == 0) {
	Itcl_DeleteStack(stackPtr);
	ckfree((char *)stackPtr);
	Tcl_DeleteHashEntry(hPtr);
    }

    if (cPtr != popped) {
	Tcl_Panic("Context stack mismatch!");
    }
    ckfree((char *) cPtr);

    return result;
}

int
ItclInfoGuts(
    ItclObject *ioPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    ItclObjectInfo *infoPtr = ioPtr->infoPtr;
    Tcl_CmdInfo info;
    ItclCallContext *cPtr;
    Tcl_CallFrame *framePtr;
    Tcl_HashEntry *hPtr;
    Itcl_Stack *stackPtr;
    int isNew;

    if (objc == 2) {
	/*
	 * No subcommand passed.  Give good usage message.  NOT the
	 * default message of a Tcl ensemble.
	 */

	Tcl_Obj *objPtr = Tcl_NewStringObj(
		"wrong # args: should be one of...\n", -1);
	ItclGetInfoUsage(interp, objPtr, infoPtr, ioPtr->iclsPtr);
	Tcl_SetObjResult(interp, objPtr);
	return TCL_ERROR;
    }

    framePtr = Itcl_GetUplevelCallFrame(interp, 0);

    hPtr = Tcl_CreateHashEntry(&infoPtr->frameContext, (char *)framePtr, &isNew);
    if (isNew) {
	stackPtr = (Itcl_Stack *) ckalloc(sizeof(Itcl_Stack));
	Itcl_InitStack(stackPtr);
	Tcl_SetHashValue(hPtr, stackPtr);
    } else {
	stackPtr = (Itcl_Stack *) Tcl_GetHashValue(hPtr);
    }

    cPtr = (ItclCallContext *) ckalloc(sizeof(ItclCallContext));
    cPtr->objectFlags = ITCL_OBJECT_ROOT_METHOD;
    cPtr->nsPtr = NULL;
    cPtr->ioPtr = ioPtr;
    cPtr->imPtr = NULL;
    cPtr->refCount = 1;

    Itcl_PushStack(cPtr, stackPtr);

    Tcl_NRAddCallback(interp, InfoGutsFinish, framePtr, infoPtr, cPtr, NULL);
    Tcl_GetCommandInfoFromToken(infoPtr->infoCmd, &info);
    return Tcl_NRCallObjProc(interp, info.objProc, info.objClientData,
	    objc-1, objv+1);
}

static int
NRInfoWrap(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_CmdInfo info;
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)clientData;

    if (!infoPtr->infoCmd) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"itcl info-subsystem is deleted", -1));
	return TCL_ERROR;
    }

    if (objc == 1) {
	/*
	 * No subcommand passed.  Give good usage message.  NOT the
	 * default message of a Tcl ensemble.
	 */

	Tcl_Obj *objPtr = Tcl_NewStringObj(
		"wrong # args: should be one of...\n", -1);
	ItclGetInfoUsage(interp, objPtr, infoPtr, NULL);
	Tcl_SetObjResult(interp, objPtr);
	return TCL_ERROR;
    }

    /* Have a subcommand.  Pass on to the ensemble */

    Tcl_GetCommandInfoFromToken(infoPtr->infoCmd, &info);
    return Tcl_NRCallObjProc(interp, info.objProc, info.objClientData,
	    objc, objv);
}

static int
InfoWrap(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    return Tcl_NRCallObjProc(interp, NRInfoWrap, clientData, objc, objv);
}

static void
InfoCmdDelete(
    ClientData clientData)
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)clientData;

    infoPtr->infoCmd = NULL;
}

int
ItclInfoInit(
    Tcl_Interp *interp,      /* current interpreter */
    ItclObjectInfo *infoPtr)
{
    Tcl_Namespace *nsPtr;
    Tcl_Command token;
    Tcl_CmdInfo info;
    Tcl_Obj *unkObjPtr;
    Tcl_Obj *ensObjPtr;
    int result;
    int i;

    /*
     * Build the ensemble used to implement [info].
     */

    nsPtr = Tcl_CreateNamespace(interp, "::itcl::builtin::Info", NULL, NULL);
    if (nsPtr == NULL) {
        Tcl_Panic("ITCL: error in creating namespace: ::itcl::builtin::Info \n");
    }
    if (infoPtr->infoCmd) {
	Tcl_Panic("Double init of info ensemble");
    }
    token = Tcl_CreateEnsemble(interp, nsPtr->fullName, nsPtr,
        TCL_ENSEMBLE_PREFIX);
    infoPtr->infoCmd = token;
    token = Tcl_NRCreateCommand(interp, "::itcl::builtin::info", InfoWrap,
	    NRInfoWrap, infoPtr, InfoCmdDelete);
    Tcl_GetCommandInfoFromToken(token, &info);

    /*
     * Make the C implementation of the "info" ensemble available as
     * a method body.  This makes all [$object info] become the
     * equivalent of [::itcl::builtin::Info] without any need for
     * tailcall to restore the right frame [87a1bc6e82].
     */
    Itcl_RegisterObjC(interp, "itcl-builtin-info", info.objProc,
	info.objClientData, NULL);

    Tcl_Export(interp, nsPtr, "[a-z]*", 1);
    ensObjPtr = Tcl_NewStringObj("::itcl::builtin::Info", -1);

    for (i=0 ; InfoMethodList[i].name!=NULL ; i++) {
	Tcl_Obj *cmdObjPtr = Tcl_DuplicateObj(ensObjPtr);

	Tcl_AppendToObj(cmdObjPtr, "::", 2);
	Tcl_AppendToObj(cmdObjPtr, InfoMethodList[i].name, -1);
        Tcl_CreateObjCommand(interp, Tcl_GetString(cmdObjPtr),
                InfoMethodList[i].proc, infoPtr,
                InfoMethodList[i].proc == Itcl_BiInfoVarsCmd ? ItclRestoreInfoVars : NULL);
	Tcl_DecrRefCount(cmdObjPtr);
    }
    unkObjPtr = Tcl_NewStringObj("::itcl::builtin::Info::unknown", -1);
    Tcl_CreateObjCommand(interp, Tcl_GetString(unkObjPtr),
	    Itcl_BiInfoUnknownCmd, infoPtr, NULL);
    if (Tcl_SetEnsembleUnknownHandler(NULL,
            Tcl_FindEnsemble(interp, ensObjPtr, TCL_LEAVE_ERR_MSG),
	    unkObjPtr) != TCL_OK) {
        Tcl_DecrRefCount(unkObjPtr);
        Tcl_DecrRefCount(ensObjPtr);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(ensObjPtr);

    /*
     * Build the ensemble used to implement [info delegated].
     */

    nsPtr = Tcl_CreateNamespace(interp, "::itcl::builtin::Info::delegated",
            NULL, NULL);
    if (nsPtr == NULL) {
        Tcl_Panic("ITCL: error in creating namespace: ::itcl::builtin::Info::delegated \n");
    }
    Tcl_CreateEnsemble(interp, nsPtr->fullName, nsPtr,
        TCL_ENSEMBLE_PREFIX);
    Tcl_Export(interp, nsPtr, "[a-z]*", 1);
    for (i=0 ; infoCmdsDelegated2[i].name!=NULL ; i++) {
        Tcl_CreateObjCommand(interp, infoCmdsDelegated2[i].name,
                infoCmdsDelegated2[i].proc, infoPtr, NULL);
    }
    ensObjPtr = Tcl_NewStringObj("::itcl::builtin::Info::delegated",
            -1);
    unkObjPtr = Tcl_NewStringObj(
            "::itcl::builtin::Info::delegated::unknown", -1);
    result = TCL_OK;
    if (Tcl_SetEnsembleUnknownHandler(NULL,
            Tcl_FindEnsemble(interp, ensObjPtr, TCL_LEAVE_ERR_MSG),
	    unkObjPtr) != TCL_OK) {
        result = TCL_ERROR;
    }
    Tcl_DecrRefCount(ensObjPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclGetInfoUsage()
 *
 * ------------------------------------------------------------------------
  */
void
ItclGetInfoUsage(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,       /* returns: summary of usage info */
    ItclObjectInfo *infoPtr,
    ItclClass *iclsPtr)
{
    const char *spaces = "  ";
    int i;

    ItclObject *ioPtr;
    if (iclsPtr == NULL) {
    if (TCL_ERROR == Itcl_GetContext(interp, &iclsPtr, &ioPtr)) {
	return;
    }
    }
    for (i=0; InfoMethodList[i].name != NULL; i++) {
	if (strcmp(InfoMethodList[i].name, "vars") == 0) {
	    /* we don't report that, as it is a special case
	     * it is only adding the protected and private commons
	     * to the ::info vars command */
	    continue;
	}
	    if (iclsPtr->flags & InfoMethodList[i].flags) {
                Tcl_AppendToObj(objPtr, spaces, -1);
                Tcl_AppendToObj(objPtr, "info ", -1);
                Tcl_AppendToObj(objPtr, InfoMethodList[i].name, -1);
	        if (strlen(InfoMethodList[i].usage) > 0) {
                  Tcl_AppendToObj(objPtr, " ", -1);
                  Tcl_AppendToObj(objPtr, InfoMethodList[i].usage, -1);
	        }
                spaces = "\n  ";
	    }
    }
        Tcl_AppendToObj(objPtr,
            "\n...and others described on the man page", -1);
}

/*
 * ------------------------------------------------------------------------
 *  ItclGetInfoDelegatedUsage()
 *
 * ------------------------------------------------------------------------
  */
static void
ItclGetInfoDelegatedUsage(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,       /* returns: summary of usage info */
    ItclObjectInfo *infoPtr)
{
    ItclClass *iclsPtr;
    const char *name;
    const char *lastName;
    const char *spaces = "  ";

    int i;

    ItclObject *ioPtr;
    if (TCL_ERROR == Itcl_GetContext(interp, &iclsPtr, &ioPtr)) {
	return;
    }
    for (i=0; infoCmdsDelegated2[i].name != NULL; i++) {
	name = infoCmdsDelegated2[i].name;
	lastName = name;
	while (name != NULL) {
	    name = strstr(name, "::");
	    if (name == NULL) {
	        break;
	    }
	    name += 2;
	    lastName = name;
	}
	name = lastName;
	if (strcmp(name, "unknown") == 0) {
	    /* we don't report that, as it is a special case */
	    continue;
	}
	    if (iclsPtr->flags & infoCmdsDelegated2[i].flags) {
                Tcl_AppendToObj(objPtr, spaces, -1);
                Tcl_AppendToObj(objPtr, "info ", -1);
                Tcl_AppendToObj(objPtr, name, -1);
	        if (strlen(infoCmdsDelegated2[i].usage) > 0) {
                  Tcl_AppendToObj(objPtr, " ", -1);
                  Tcl_AppendToObj(objPtr, infoCmdsDelegated2[i].usage, -1);
	        }
                spaces = "\n  ";
	    }
    }
        Tcl_AppendToObj(objPtr,
            "\n...and others described on the man page", -1);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoClassCmd()
 *
 *  Returns information regarding the class for an object.  This command
 *  can be invoked with or without an object context:
 *
 *    <objName> info class   <= returns most-specific class name
 *    info class             <= returns active namespace name
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoClassCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Namespace *contextNs = NULL;
    ItclClass *contextIclsPtr = NULL;
    ItclObject *contextIoPtr;

    char *name;

    ItclShowArgs(1, "Itcl_BiInfoClassCmd", objc, objv);
    if (objc != 1) {
	/* TODO: convert to NR-enabled fallback to [::info] */
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        /* try it the hard way */
	ClientData clientData;
        ItclObjectInfo *infoPtr;
        Tcl_Object oPtr;
	clientData = Itcl_GetCallFrameClientData(interp);
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
	if (clientData != NULL) {
            oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData);
            contextIoPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
	            infoPtr->object_meta_type);
            contextIclsPtr = contextIoPtr->iclsPtr;
	}
	if ((contextIoPtr == NULL) || (contextIclsPtr == NULL)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "\nget info like this instead: " \
		    "\n  namespace eval className { info class }", -1));
            return TCL_ERROR;
        }
    }

    /*
     *  If there is an object context, then return the most-specific
     *  class for the object.  Otherwise, return the class namespace
     *  name.  Use normal class names when possible.
     */
    if (contextIoPtr) {
        contextNs = contextIoPtr->iclsPtr->nsPtr;
    } else {
        assert(contextIclsPtr != NULL);
        assert(contextIclsPtr->nsPtr != NULL);
        contextNs = contextIclsPtr->nsPtr;
    }

    assert(contextNs);

        name = contextNs->fullName;

    Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoClassOptionsCmd()
 *
 *  Returns information regarding the options for a class.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoClassOptionsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_HashEntry *hPtr2;
    Tcl_Obj *listPtr;
    Tcl_Obj *listPtr2;
    Tcl_Obj *objPtr;
    Tcl_Obj **lObjv;
    Tcl_HashTable *tablePtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclOption *ioptPtr;
    ItclDelegatedOption *idoPtr;
    const char *name;
    const char *val;
    const char *pattern;
    int lObjc;
    int result;
    int i;

    ItclShowArgs(1, "Itcl_BiInfoClassOptionsCmd", objc, objv);
    iclsPtr = NULL;
    pattern = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        Tcl_AppendResult(interp, "cannot get context ", NULL);
        return TCL_ERROR;
    }
    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args should be: info options ",
	        "?pattern?", NULL);
        return TCL_ERROR;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    listPtr = Tcl_NewListObj(0, NULL);
    tablePtr = &iclsPtr->options;
    FOREACH_HASH_VALUE(ioptPtr, tablePtr) {
	name = Tcl_GetString(ioptPtr->namePtr);
	if ((pattern == NULL) ||
                 Tcl_StringCaseMatch(name, pattern, 0)) {
            Tcl_ListObjAppendElement(interp, listPtr,
	            Tcl_NewStringObj(Tcl_GetString(ioptPtr->namePtr), -1));
        }
    }
    tablePtr = &iclsPtr->delegatedOptions;
    FOREACH_HASH_VALUE(idoPtr, tablePtr) {
        name = Tcl_GetString(idoPtr->namePtr);
	if (strcmp(name, "*") != 0) {
	    if ((pattern == NULL) ||
                    Tcl_StringCaseMatch(name, pattern, 0)) {
                Tcl_ListObjAppendElement(interp, listPtr,
	                Tcl_NewStringObj(Tcl_GetString(idoPtr->namePtr), -1));
	    }
        } else {
	    if (idoPtr->icPtr == NULL) {
		Tcl_AppendResult(interp, "component \"",
		        Tcl_GetString(idoPtr->namePtr),
			"\" is not initialized", NULL);
	        return TCL_ERROR;
	    }
	    val = ItclGetInstanceVar(interp,
	            Tcl_GetString(idoPtr->icPtr->namePtr),
	            NULL, ioPtr, ioPtr->iclsPtr);
            if ((val != NULL) && (strlen(val) != 0)) {
		objPtr = Tcl_NewStringObj(val, -1);
		Tcl_AppendToObj(objPtr, " configure", -1);
		Tcl_IncrRefCount(objPtr);
		result = Tcl_EvalObjEx(interp, objPtr, 0);
		Tcl_DecrRefCount(objPtr);
		if (result != TCL_OK) {
		    return TCL_ERROR;
		}
	        listPtr2 = Tcl_GetObjResult(interp);
		Tcl_ListObjGetElements(interp, listPtr2, &lObjc, &lObjv);
		for (i = 0; i < lObjc; i++) {
		    Tcl_ListObjIndex(interp, lObjv[i], 0, &objPtr);
		    hPtr2 = Tcl_FindHashEntry(&idoPtr->exceptions,
		            (char *)objPtr);
		    if (hPtr2 == NULL) {
			name = Tcl_GetString(objPtr);
	                if ((pattern == NULL) ||
                                 Tcl_StringCaseMatch(name, pattern, 0)) {
		            Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		        }
		    }
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoContextCmd()
 *
 *  Returns information regarding the context object and class.  This command
 *  can only be invoked with an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoContextCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    ItclObject *ioPtr = NULL;
    ItclClass *iclsPtr;

    ItclShowArgs(1, "Itcl_BiInfoContextCmd", objc, objv);
    iclsPtr = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get object context ", NULL);
        return TCL_ERROR;
    }
    listPtr = Tcl_NewListObj(0, NULL);
    objPtr = Tcl_NewStringObj(Tcl_GetString(iclsPtr->namePtr), -1);
    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
    objPtr = Tcl_NewStringObj(Tcl_GetString(ioPtr->namePtr), -1);
    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoInheritCmd()
 *
 *  Returns the list of base classes for the current class context.
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoInheritCmd(
    ClientData clientdata, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    ItclClass *contextIclsPtr = NULL;
    ItclObject *contextIoPtr = NULL;
    Itcl_ListElem *elem;
    Tcl_Obj *listPtr;

    ItclShowArgs(2, "Itcl_BiInfoInheritCmd", objc, objv);
    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */

    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info inherit }", -1));
        return TCL_ERROR;
    }

    /*
     *  Return the list of base classes.
     */

    listPtr = Tcl_NewListObj(0, NULL);
    elem = Itcl_FirstListElem(&contextIclsPtr->bases);
    while (elem) {
	Tcl_Obj *objPtr;
	ItclClass *iclsPtr = (ItclClass*)Itcl_GetListValue(elem);
            objPtr = Tcl_NewStringObj(iclsPtr->nsPtr->fullName, -1);
        Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
        elem = Itcl_NextListElem(elem);
    }

    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoHeritageCmd()
 *
 *  Returns the entire derivation hierarchy for this class, presented
 *  in the order that classes are traversed for finding data members
 *  and member functions.
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoHeritageCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    ItclClass *contextIclsPtr = NULL;
    ItclObject *contextIoPtr = NULL;
    ItclHierIter hier;
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    ItclClass *iclsPtr;

    ItclShowArgs(2, "Itcl_BiInfoHeritageCmd", objc, objv);
    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info heritage }", -1));
        return TCL_ERROR;
    }

    /*
     *  Traverse through the derivation hierarchy and return
     *  base class names.
     */
    listPtr = Tcl_NewListObj(0, NULL);
    Itcl_InitHierIter(&hier, contextIclsPtr);
    while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
        if (iclsPtr->nsPtr == NULL) {
            Tcl_AppendResult(interp, "ITCL: iclsPtr->nsPtr == NULL",
	            Tcl_GetString(iclsPtr->fullNamePtr), NULL);
            return TCL_ERROR;
        }
            objPtr = Tcl_NewStringObj(iclsPtr->nsPtr->fullName, -1);
        Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
    }
    Itcl_DeleteHierIter(&hier);

    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoFunctionCmd()
 *
 *  Returns information regarding class member functions (methods/procs).
 *  Handles the following syntax:
 *
 *    info function ?cmdName? ?-protection? ?-type? ?-name? ?-args? ?-body?
 *
 *  If the ?cmdName? is not specified, then a list of all known
 *  command members is returned.  Otherwise, the information for
 *  a specific command is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoFunctionCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    char *cmdName = NULL;
    Tcl_Obj *resultPtr = NULL;
    Tcl_Obj *objPtr = NULL;

    static const char *options[] = {
        "-args", "-body", "-name", "-protection", "-type",
        NULL
    };
    enum BIfIdx {
        BIfArgsIdx, BIfBodyIdx, BIfNameIdx, BIfProtectIdx, BIfTypeIdx
    } *iflist, iflistStorage[5];

    static enum BIfIdx DefInfoFunction[5] = {
        BIfProtectIdx,
        BIfTypeIdx,
        BIfNameIdx,
        BIfArgsIdx,
        BIfBodyIdx
    };

    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    ItclClass *iclsPtr;
    int i;
    int result;
    const char *val;
    Tcl_HashSearch place;
    Tcl_HashEntry *entry;
    ItclMemberFunc *imPtr;
    ItclMemberCode *mcode;
    ItclHierIter hier;

    ItclShowArgs(2, "Itcl_InfoFunctionCmd", objc, objv);
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info function ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?cmdName? ?-protection? ?-type? ?-name? ?-args? ?-body?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        cmdName = Tcl_GetString(*objv);
        objc--; objv++;
    }

    /*
     *  Return info for a specific command.
     */
    if (cmdName) {
	ItclCmdLookup *clookup;
	objPtr = Tcl_NewStringObj(cmdName, -1);
        entry = Tcl_FindHashEntry(&contextIclsPtr->resolveCmds, (char *)objPtr);
	Tcl_DecrRefCount(objPtr);
	objPtr = NULL;
        if (entry == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", cmdName, "\" isn't a member function in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
        }

	clookup = (ItclCmdLookup *)Tcl_GetHashValue(entry);
	imPtr = clookup->imPtr;
        mcode = imPtr->codePtr;

        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            objc = 5;
            iflist = DefInfoFunction;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            iflist = &iflistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&iflist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (iflist[i]) {
                case BIfArgsIdx:
                    if (mcode && mcode->argListPtr) {
			if (imPtr->usagePtr == NULL) {
                            objPtr = Tcl_NewStringObj(
			            Tcl_GetString(mcode->usagePtr), -1);
			} else {
                            objPtr = Tcl_NewStringObj(
			            Tcl_GetString(imPtr->usagePtr), -1);
		        }
                    } else {
		        if ((imPtr->flags & ITCL_ARG_SPEC) != 0) {
			    if (imPtr->usagePtr == NULL) {
                                objPtr = Tcl_NewStringObj(
				        Tcl_GetString(mcode->usagePtr), -1);
			    } else {
			        objPtr = Tcl_NewStringObj(
				        Tcl_GetString(imPtr->usagePtr), -1);
			    }
                        } else {
                            objPtr = Tcl_NewStringObj("<undefined>", -1);
                        }
		    }
                    break;

                case BIfBodyIdx:
                    if (mcode && Itcl_IsMemberCodeImplemented(mcode)) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(mcode->bodyPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("<undefined>", -1);
                    }
                    break;

                case BIfNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(imPtr->fullNamePtr), -1);
                    break;

                case BIfProtectIdx:
                    val = Itcl_ProtectionStr(imPtr->protection);
                    objPtr = Tcl_NewStringObj(val, -1);
                    break;

                case BIfTypeIdx:
                    val = ((imPtr->flags & ITCL_COMMON) != 0)
                        ? "proc" : "method";
                    objPtr = Tcl_NewStringObj(val, -1);
                    break;
            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available commands.
         */
        resultPtr = Tcl_NewListObj(0, NULL);

        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            entry = Tcl_FirstHashEntry(&iclsPtr->functions, &place);
            while (entry) {
	        int useIt = 1;

                imPtr = (ItclMemberFunc*)Tcl_GetHashValue(entry);
		if (imPtr->codePtr && (imPtr->codePtr->flags & ITCL_BUILTIN)) {
		    if (strcmp(Tcl_GetString(imPtr->namePtr), "info") == 0) {
		        useIt = 0;
		    }
		    if (strcmp(Tcl_GetString(imPtr->namePtr), "setget") == 0) {
			if (!(imPtr->iclsPtr->flags & ITCL_ECLASS)) {
		            useIt = 0;
		        }
		    }
		    if (strcmp(Tcl_GetString(imPtr->namePtr),
		            "installcomponent") == 0) {
			if (!(imPtr->iclsPtr->flags &
			        (ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
		            useIt = 0;
		        }
		    }
		}
		if (useIt) {
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(imPtr->fullNamePtr), -1);
                    Tcl_ListObjAppendElement(NULL,
		            resultPtr, objPtr);
                }

                entry = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoVariableCmd()
 *
 *  Returns information regarding class data members (variables and
 *  commons).  Handles the following syntax:
 *
 *    info variable ?varName? ?-protection? ?-type? ?-name?
 *        ?-init? ?-config? ?-value?
 *
 *  If the ?varName? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/*&&&1*/
/* ARGSUSED */
int
Itcl_BiInfoVariableCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_HashSearch place;
    Tcl_HashEntry *entry;
    ItclClass *iclsPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclVariable *ivPtr;
    ItclVarLookup *vlookup;
    ItclHierIter hier;
    char *varName;
    const char *val;
    int i;
    int result;

    ClientData cfClientData;
    ItclObjectInfo *infoPtr;
    Tcl_Object oPtr;
    int doAppend;

    static const char *options[] = {
        "-config", "-init", "-name", "-protection", "-type",
        "-value", "-scope", NULL
    };
    enum BIvIdx {
        BIvConfigIdx, BIvInitIdx, BIvNameIdx, BIvProtectIdx,
        BIvTypeIdx, BIvValueIdx, BIvScopeIdx
    } *ivlist, ivlistStorage[7];

    static enum BIvIdx DefInfoVariable[5] = {
        BIvProtectIdx,
        BIvTypeIdx,
        BIvNameIdx,
        BIvInitIdx,
        BIvValueIdx
    };

    static enum BIvIdx DefInfoPubVariable[6] = {
        BIvProtectIdx,
        BIvTypeIdx,
        BIvNameIdx,
        BIvInitIdx,
        BIvConfigIdx,
        BIvValueIdx
    };

    ItclShowArgs(1, "Itcl_BiInfoVariableCmd", objc, objv);
    resultPtr = NULL;
    objPtr = NULL;
    varName = NULL;
    contextIclsPtr = NULL;
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info variable ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?varName? ?-protection? ?-type? ?-name? ?-init? ?-config? ?-value? ?-scope?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        varName = Tcl_GetString(*objv);
        objc--; objv++;
    }

    /*
     *  Return info for a specific variable.
     */
    if (varName) {
        entry = ItclResolveVarEntry(contextIclsPtr, varName);
        if (entry == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", varName, "\" isn't a variable in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
        }

        vlookup = (ItclVarLookup*)Tcl_GetHashValue(entry);
        ivPtr = vlookup->ivPtr;

        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            if (ivPtr->protection == ITCL_PUBLIC &&
                    ((ivPtr->flags & ITCL_COMMON) == 0)) {
                ivlist = DefInfoPubVariable;
                objc = 6;
            } else {
                ivlist = DefInfoVariable;
                objc = 5;
            }
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            ivlist = &ivlistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&ivlist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (ivlist[i]) {
                case BIvConfigIdx:
                    if (ivPtr->codePtr &&
		            Itcl_IsMemberCodeImplemented(ivPtr->codePtr)) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ivPtr->codePtr->bodyPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BIvInitIdx:
                    /*
                     *  If this is the built-in "this" variable, then
                     *  report the object name as its initialization string.
                     */
                    if ((ivPtr->flags & ITCL_THIS_VAR) != 0) {
                        if ((contextIoPtr != NULL) &&
			        (contextIoPtr->accessCmd != NULL)) {
                            objPtr = Tcl_NewStringObj(NULL, 0);
                            Tcl_GetCommandFullName(
                                contextIoPtr->iclsPtr->interp,
                                contextIoPtr->accessCmd, objPtr);
                        } else {
                            objPtr = Tcl_NewStringObj("<objectName>", -1);
                        }
                    } else {
		        if (vlookup->ivPtr->init) {
			    objPtr = Tcl_NewStringObj(
			            Tcl_GetString(vlookup->ivPtr->init), -1);
                        } else {
                            objPtr = Tcl_NewStringObj("<undefined>", -1);
		        }
                    }
                    break;

                case BIvNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(ivPtr->fullNamePtr), -1);
                    break;

                case BIvProtectIdx:
                    val = Itcl_ProtectionStr(ivPtr->protection);
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;

                case BIvTypeIdx:
                    val = ((ivPtr->flags & ITCL_COMMON) != 0)
                        ? "common" : "variable";
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;

                case BIvValueIdx:
                    if ((ivPtr->flags & ITCL_COMMON) != 0) {
                        val = Itcl_GetCommonVar(interp,
			        Tcl_GetString(ivPtr->fullNamePtr),
                                ivPtr->iclsPtr);
                    } else {
		        if (contextIoPtr == NULL) {
                            if (objc > 1) {
			        Tcl_DecrRefCount(resultPtr);
			    }
                            Tcl_ResetResult(interp);
                            Tcl_AppendResult(interp,
                                    "cannot access object-specific info ",
                                    "without an object context",
                                    NULL);
                            return TCL_ERROR;
                        } else {
                            val = Itcl_GetInstanceVar(interp,
			            Tcl_GetString(ivPtr->namePtr),
                                    contextIoPtr, ivPtr->iclsPtr);
                        }
                    }
                    if (val == NULL) {
                        val = "<undefined>";
                    }
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;

                case BIvScopeIdx:
                    entry = Tcl_FindHashEntry(&contextIclsPtr->resolveVars, varName);
                    if (!entry) {
                        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                              "variable \"", varName, "\" not found in class \"",
                              Tcl_GetString(contextIclsPtr->fullNamePtr), "\"",
                              (char*)NULL);
                        return TCL_ERROR;
                    }
                    vlookup = (ItclVarLookup*)Tcl_GetHashValue(entry);

                    if (vlookup->ivPtr->flags & ITCL_COMMON) {
                        objPtr = Tcl_NewStringObj("", -1);

                        if (vlookup->ivPtr->protection != ITCL_PUBLIC) {
                            Tcl_AppendToObj(objPtr, ITCL_VARIABLES_NAMESPACE, -1);
                        }
                        Tcl_AppendToObj(objPtr,
                                Tcl_GetString(vlookup->ivPtr->fullNamePtr), -1);
                    } else {
                        /*
                         *  If this is not a common variable, then we better have
                         *  an object context.  Return the name as a fully qualified name.
                         */
                        infoPtr = contextIclsPtr->infoPtr;
                        cfClientData = Itcl_GetCallFrameClientData(interp);
                        if (cfClientData != NULL) {
                            oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)cfClientData);
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
                                "can't scope variable \"", varName,
                                "\": missing object context",
                                (char*)NULL);
                            return TCL_ERROR;
                        }

                        doAppend = 1;
                        if (contextIclsPtr->flags & ITCL_ECLASS) {
                            if (strcmp(varName, "itcl_options") == 0) {
                                doAppend = 0;
                            }
                        }

                        objPtr = Tcl_NewStringObj((char*)NULL, 0);
                        Tcl_IncrRefCount(objPtr);
                        Tcl_AppendToObj(objPtr, ITCL_VARIABLES_NAMESPACE, -1);
                        Tcl_AppendToObj(objPtr,
                                (Tcl_GetObjectNamespace(contextIoPtr->oPtr))->fullName, -1);

                        if (doAppend) {
                            Tcl_AppendToObj(objPtr,
                                    Tcl_GetString(vlookup->ivPtr->fullNamePtr), -1);
                        } else {
                            Tcl_AppendToObj(objPtr, "::", -1);
                            Tcl_AppendToObj(objPtr,
                                    Tcl_GetString(vlookup->ivPtr->namePtr), -1);
                        }
                    }
                    break;
            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
            }
        }
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, Tcl_GetString(resultPtr), NULL);
	Tcl_DecrRefCount(resultPtr);
    } else {

        /*
         *  Return the list of available variables.  Report the built-in
         *  "this" variable only once, for the most-specific class.
         */
        resultPtr = Tcl_NewListObj(0, NULL);
        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            entry = Tcl_FirstHashEntry(&iclsPtr->variables, &place);
            while (entry) {
                ivPtr = (ItclVariable*)Tcl_GetHashValue(entry);
                if ((ivPtr->flags & ITCL_THIS_VAR) != 0) {
                    if (iclsPtr == contextIclsPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ivPtr->fullNamePtr), -1);
                        Tcl_ListObjAppendElement(NULL,
                            resultPtr, objPtr);
                    }
                } else {
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(ivPtr->fullNamePtr), -1);
                    Tcl_ListObjAppendElement(NULL,
                        resultPtr, objPtr);
                }
                entry = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoVarsCmd()
 *
 *  Returns information regarding variables
 *
 *    info vars ?pattern?
 *  uses ::info vars and adds Itcl common variables!!
 *
 *  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoVarsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    Tcl_Obj **newObjv;
    Tcl_Namespace *nsPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr = NULL;
    ItclVariable *ivPtr;
    const char *pattern;
    const char *name;
    int useGlobalInfo;
    int result;
    ItclObject *ioPtr;

    ItclShowArgs(1, "Itcl_BiInfoVars", objc, objv);
    result = TCL_OK;
    useGlobalInfo = 1;
    pattern = NULL;
    infoPtr = (ItclObjectInfo *)clientData;

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, " ?pattern?");
	return TCL_ERROR;
    }

    if (TCL_OK != Itcl_GetContext(interp, &iclsPtr, &ioPtr)) {
	/* Clear the error message */
	Tcl_ResetResult(interp);
    }
    if (iclsPtr) {
	    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET)) {
	        /* don't use the ::tcl::info::vars command */
	        useGlobalInfo = 0;
	        if (objc == 2) {
		    pattern = Tcl_GetString(objv[1]);
		}
	    }
    }
    if (useGlobalInfo) {
        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc));
        newObjv[0] = Tcl_NewStringObj("::tcl::info::vars", -1);
        Tcl_IncrRefCount(newObjv[0]);
        memcpy(newObjv+1, objv+1, sizeof(Tcl_Obj *)*(objc-1));
        result = Tcl_EvalObjv(interp, objc, newObjv, 0);
        Tcl_DecrRefCount(newObjv[0]);
        ckfree((char *)newObjv);
    } else {
	listPtr = Tcl_NewListObj(0, NULL);
	FOREACH_HASH_VALUE(ivPtr, &iclsPtr->variables) {
	    if ((ivPtr->flags & ITCL_VARIABLE) != 0) {
		name = Tcl_GetString(ivPtr->namePtr);
                if ((pattern == NULL) ||
		        Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
	            Tcl_ListObjAppendElement(interp, listPtr, ivPtr->namePtr);
	        }
            }
	}
	/* always add the itcl_options variable */
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("itcl_options", -1));
        Tcl_SetObjResult(interp, listPtr);
        return TCL_OK;
    }
    if (objc < 2) {
        return result;
    }
    if (result == TCL_OK) {
	Tcl_DString buffer;
	const char *head;
	const char *tail;
        /* check if the pattern contains a class namespace
	 * and if yes add the common private and protected vars
	 * and remove the ___DO_NOT_DELETE_THIS_VARIABLE var
	 */
	Itcl_ParseNamespPath(Tcl_GetString(objv[1]), &buffer, &head, &tail);
        if (head == NULL) {
	    nsPtr = Tcl_GetCurrentNamespace(interp);
	} else {
            nsPtr = Tcl_FindNamespace(interp, head, NULL, 0);
        }
	    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses,
	            (char *)nsPtr);
	    if (hPtr != NULL) {
		Itcl_List varList;
		Tcl_Obj *resultListPtr;
		Tcl_Obj *namePtr;
		int numElems;

		Itcl_InitList(&varList);
		iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
		resultListPtr = Tcl_GetObjResult(interp);
		numElems = 0;
/* FIXME !! should perhaps skip ___DO_NOT_DELETE_THIS_VARIABLE here !! */
		FOREACH_HASH_VALUE(ivPtr, &iclsPtr->variables) {
		    if ((ivPtr->flags & ITCL_VARIABLE) != 0) {
		        if (head != NULL) {
		            namePtr = Tcl_NewStringObj(
			            Tcl_GetString(ivPtr->fullNamePtr), -1);
		        } else {
		            namePtr = Tcl_NewStringObj(
			            Tcl_GetString(ivPtr->namePtr), -1);
		        }
		        Tcl_ListObjAppendElement(interp, resultListPtr,
		                namePtr);
		        numElems++;
		    }
		    if ((ivPtr->flags & ITCL_COMMON) != 0) {
		        if (ivPtr->protection != ITCL_PUBLIC) {
			    if (head != NULL) {
			        namePtr = Tcl_NewStringObj(
				        Tcl_GetString(ivPtr->fullNamePtr), -1);
			    } else {
			        namePtr = Tcl_NewStringObj(
				        Tcl_GetString(ivPtr->namePtr), -1);
			    }
		            Tcl_ListObjAppendElement(interp, resultListPtr,
			            namePtr);
			    numElems++;
		        }
		    }
		}
	    }
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoUnknownCmd()
 *
 *  the unknown handler for the ::itcl::builtin::Info ensemble
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoUnknownCmd(
    ClientData clientData,   /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *objPtr, *listObj;
    int usage = 1;
    int code = TCL_ERROR;

    ItclShowArgs(1, "Itcl_BiInfoUnknownCmd", objc, objv);

    if (objc < 2) {
	/* Namespace ensemble unknown callbacks never do this. */
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"unknown callback should not be called directly", -1));
	return TCL_ERROR;
    }

    /* Redirect to the [::info] command. */
    objPtr = Tcl_NewStringObj("::info", -1);
    listObj = Tcl_NewListObj(1, &objPtr);
    Tcl_IncrRefCount(listObj);
    if (Tcl_GetCommandFromObj(interp, objPtr)) {
	usage = 0;
	Tcl_ListObjReplace(NULL, listObj, 1, 0, objc-2, objv+2);
	code = Tcl_EvalObjEx(interp, listObj, 0);
	if (code == TCL_ERROR) {
	    /* Redirection to [::info] failed, but why? */
	    Tcl_Obj *optDict = Tcl_GetReturnOptions(interp, code);
	    Tcl_Obj *key = Tcl_NewStringObj("-errorcode", -1);
	    Tcl_Obj *val, *elem;

	    Tcl_DictObjGet(NULL, optDict, key, &val);
	    Tcl_DecrRefCount(key);
	    Tcl_ListObjIndex(NULL, val, 0, &elem);
	    if (elem && !strcmp(Tcl_GetString(elem), "TCL")) {
		Tcl_ListObjIndex(NULL, val, 1, &elem);
		if (elem && !strcmp(Tcl_GetString(elem), "LOOKUP")) {
		    Tcl_ListObjIndex(NULL, val, 2, &elem);
		    if (elem && !strcmp(Tcl_GetString(elem), "SUBCOMMAND")) {

			/* [::info didn't have that subcommand] */
			usage = 1;
			Tcl_ResetResult(interp);
		    }
		}
	    }
	}
    }
    Tcl_DecrRefCount(listObj);

    if (usage) {
        /* produce usage message */
        Tcl_Obj *objPtr = Tcl_NewStringObj(
	        "wrong # args: should be one of...\n", -1);
        ItclGetInfoUsage(interp, objPtr, (ItclObjectInfo *)clientData, NULL);
	Tcl_SetObjResult(interp, objPtr);
    }
    if (code == TCL_ERROR) {
	return TCL_ERROR;
    }

    /* Return a command to replicate the non-error redirect outcome */
    listObj = Tcl_NewStringObj(
	    "::apply {{o m args} {::tailcall ::return -options $o $m}}", -1);
    Tcl_ListObjAppendElement(NULL, listObj, Tcl_GetReturnOptions(interp,code));
    Tcl_ListObjAppendElement(NULL, listObj, Tcl_GetObjResult(interp));
    Tcl_SetObjResult(interp, listObj);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoBodyCmd()
 *
 *  Handles the usual "info body" request, returning the body for a
 *  specific proc.  Included here for backward compatibility, since
 *  otherwise Tcl would complain that class procs are not real "procs".
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoBodyCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_HashEntry *hPtr;
    ItclClass *contextIclsPtr = NULL;
    ItclObject *contextIoPtr;
    const char *what = "procedure";

    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	int code;
	Tcl_Obj *script;

	/*
	 * We lack the context for any specialized Itcl meaning for
	 * [info body], so fallback to Tcl's.
	 */

    fallback:
	script = Tcl_NewStringObj("::info body", -1);
	if (objc == 2) {
	    Tcl_ListObjAppendElement(NULL, script, objv[1]);
	}
	Tcl_IncrRefCount(script);
	code = Tcl_EvalObjEx(interp, script, 0);
	Tcl_DecrRefCount(script);
	if (code == TCL_ERROR && what) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"\"%s\" isn't a %s", Tcl_GetString(objv[1]), what));
	}
	return code;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    what = "function";
    if (contextIclsPtr->flags &
            (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS)) {
        what = "method";
    }
    if (objc != 2) {
	Tcl_AppendResult(interp,
	        "wrong # args: should be \"info body ", what, "\"",
	        NULL);
        return TCL_ERROR;
    }

    hPtr = Tcl_FindHashEntry(&contextIclsPtr->resolveCmds, (char *)objv[1]);
    if (hPtr) {
        ItclCmdLookup *clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
	ItclMemberFunc *imPtr = clookup->imPtr;
	ItclMemberCode *mcode = imPtr->codePtr;

        /*
         *  Return a string describing the implementation.
         */
        if (mcode && Itcl_IsMemberCodeImplemented(mcode)) {
            Tcl_SetObjResult(interp, mcode->bodyPtr);
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("<undefined>", -1));
        }
	return TCL_OK;
    }

	if (contextIclsPtr->flags &
	        (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS)) {
	    hPtr = Tcl_FindHashEntry(&contextIclsPtr->delegatedFunctions,
	            (char *)objv[1]);
	}

    if (hPtr) {
	ItclDelegatedFunction *idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
        Tcl_Obj *objPtr = Tcl_NewStringObj("delegated ", -1);

	if (idmPtr->flags & ITCL_TYPE_METHOD) {
	    what = "typemethod";
	}
	Tcl_AppendToObj(objPtr, what, -1);
	Tcl_AppendToObj(objPtr, " \"", -1);
	Tcl_AppendObjToObj(objPtr, objv[1]);
	Tcl_AppendToObj(objPtr, "\"", -1);
        Tcl_SetObjResult(interp, objPtr);
        return TCL_ERROR;
    }
    goto fallback;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoArgsCmd()
 *
 *  Handles the usual "info args" request, returning the argument list
 *  for a specific proc.  Included here for backward compatibility, since
 *  otherwise Tcl would complain that class procs are not real "procs".
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoArgsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_HashEntry *hPtr = NULL;
    ItclClass *contextIclsPtr = NULL;
    ItclObject *contextIoPtr;
    const char *what = NULL;

    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK
	    && objc > 1) {
	int code;
	Tcl_Obj *script;

	/*
	 * We lack the context for any specialized Itcl meaning for
	 * [info args], so fallback to Tcl's.
	 */

    fallback:
	script = Tcl_NewStringObj("::info args", -1);
	if (objc == 2) {
	    Tcl_ListObjAppendElement(NULL, script, objv[1]);
	}
	Tcl_IncrRefCount(script);
	code = Tcl_EvalObjEx(interp, script, 0);
	Tcl_DecrRefCount(script);
	if (code == TCL_ERROR && what) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"\"%s\" isn't a %s", Tcl_GetString(objv[1]), what));
	}
	return code;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    what = "function";
    if ((contextIclsPtr != NULL) && (contextIclsPtr->flags &
            (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET))) {
        what = "method";
    }
    if (objc != 2) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
            "wrong # args: should be \"info args %s\"", what));
        return TCL_ERROR;
    }

    hPtr = Tcl_FindHashEntry(&contextIclsPtr->resolveCmds, (char *)objv[1]);
    if (hPtr) {
	ItclCmdLookup *clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
	ItclMemberFunc *imPtr = clookup->imPtr;
	ItclMemberCode *mcode = imPtr->codePtr;

        /*
         *  Return a string describing the argument list.
         */
        if ((mcode && mcode->argListPtr != NULL)
		|| ((imPtr->flags & ITCL_ARG_SPEC) != 0)) {
	    Tcl_SetObjResult(interp, mcode->usagePtr);
        } else {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("<undefined>", -1));
        }
	return TCL_OK;
    }

    if (contextIclsPtr->flags
	    & (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS)) {
	hPtr = Tcl_FindHashEntry(&contextIclsPtr->delegatedFunctions,
		(char *)objv[1]);
    }

    if (hPtr) {
	ItclDelegatedFunction *idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
        Tcl_Obj *objPtr = Tcl_NewStringObj("delegated ", -1);

	if (idmPtr->flags & ITCL_TYPE_METHOD) {
	    what = "typemethod";
	}
	Tcl_AppendToObj(objPtr, what, -1);
	Tcl_AppendToObj(objPtr, " \"", -1);
	Tcl_AppendObjToObj(objPtr, objv[1]);
	Tcl_AppendToObj(objPtr, "\"", -1);
        Tcl_SetObjResult(interp, objPtr);
        return TCL_ERROR;
    }
    goto fallback;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoOptionCmd()
 *
 *  Returns information regarding class options.
 *  Handles the following syntax:
 *
 *    info option ?optionName? ?-protection? ?-name? ?-resource? ?-class?
 *        ?-default? ?-configmethod? ?-cgetmethod? ?-validatemethod? ?-value?
 *
 *  If the ?optionName? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoOptionCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    char *optionName = NULL;
    Tcl_Obj *resultPtr = NULL;
    Tcl_Obj *objPtr = NULL;
    Tcl_Obj *optionNamePtr;

    static const char *options[] = {
        "-cgetmethod", "-cgetmethodvar","-class",
	"-configuremethod", "-configuremethodvar",
	"-default",
	"-name", "-protection", "-resource",
	"-validatemethod", "-validatemethodvar",
        "-value", NULL
    };
    enum BOptIdx {
        BOptCgetMethodIdx,
        BOptCgetMethodVarIdx,
	BOptClassIdx,
	BOptConfigureMethodIdx,
	BOptConfigureMethodVarIdx,
	BOptDefaultIdx,
	BOptNameIdx,
	BOptProtectIdx,
	BOptResourceIdx,
	BOptValidateMethodIdx,
	BOptValidateMethodVarIdx,
	BOptValueIdx
    } *ioptlist, ioptlistStorage[12];

    static enum BOptIdx DefInfoOption[12] = {
        BOptProtectIdx,
        BOptNameIdx,
        BOptResourceIdx,
        BOptClassIdx,
        BOptDefaultIdx,
        BOptCgetMethodIdx,
        BOptCgetMethodVarIdx,
        BOptConfigureMethodIdx,
        BOptConfigureMethodVarIdx,
        BOptValidateMethodIdx,
        BOptValidateMethodVarIdx,
        BOptValueIdx
    };

    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclOption *ioptPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr;
    const char *val;
    int i;
    int result;

    ItclShowArgs(1, "Itcl_BiInfoOptionCmd", objc, objv);
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info option ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?optionName? ?-protection? ?-name? ?-resource? ?-class?
     * ?-default? ?-cgetmethod? ?-cgetmethodvar? ?-configuremethod?
     * ?-configuremethodvar? ?-validatemethod? ?-validatemethodvar? ?-value?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        optionName = Tcl_GetString(*objv);
        objc--;
	objv++;
    }

    /*
     *  Return info for a specific option.
     */
    if (optionName) {
	if (contextIoPtr == NULL) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "cannot access object-specific info ",
		    "without an object context", NULL);
	    return TCL_ERROR;
	}
	optionNamePtr = Tcl_NewStringObj(optionName, -1);
        hPtr = Tcl_FindHashEntry(&contextIoPtr->objectOptions,
	        (char *)optionNamePtr);
        Tcl_DecrRefCount(optionNamePtr);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                 "\"", optionName, "\" isn't a option in object \"",
                Tcl_GetString(contextIoPtr->namePtr), "\"",
                NULL);
            return TCL_ERROR;
        }
        ioptPtr = (ItclOption*)Tcl_GetHashValue(hPtr);

        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            ioptlist = DefInfoOption;
            objc = 9;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            ioptlist = &ioptlistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&ioptlist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (ioptlist[i]) {
                case BOptCgetMethodIdx:
                    if (ioptPtr->cgetMethodPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->cgetMethodPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptCgetMethodVarIdx:
                    if (ioptPtr->cgetMethodVarPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->cgetMethodVarPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptConfigureMethodIdx:
                    if (ioptPtr->configureMethodPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->configureMethodPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptConfigureMethodVarIdx:
                    if (ioptPtr->configureMethodVarPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->configureMethodVarPtr),
				-1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptValidateMethodIdx:
                    if (ioptPtr->validateMethodPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->validateMethodPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptValidateMethodVarIdx:
                    if (ioptPtr->validateMethodVarPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->validateMethodVarPtr),
				-1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptResourceIdx:
                    if (ioptPtr->resourceNamePtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->resourceNamePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptClassIdx:
                    if (ioptPtr->classNamePtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->classNamePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptDefaultIdx:
		    if (ioptPtr->defaultValuePtr != NULL) {
		        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(ioptPtr->defaultValuePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("<undefined>", -1);
		    }
                    break;

                case BOptNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(ioptPtr->fullNamePtr), -1);
                    break;

                case BOptProtectIdx:
                    val = Itcl_ProtectionStr(ioptPtr->protection);
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;

                case BOptValueIdx:
		    if (contextIoPtr == NULL) {
                        Tcl_ResetResult(interp);
                        Tcl_AppendResult(interp,
                                "cannot access object-specific info ",
                                "without an object context",
                                NULL);
                        return TCL_ERROR;
                    } else {
                        val = ItclGetInstanceVar(interp, "itcl_options",
		                Tcl_GetString(ioptPtr->namePtr),
                                    contextIoPtr, ioptPtr->iclsPtr);
                    }
                    if (val == NULL) {
                        val = "<undefined>";
                    }
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;
            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available options.
         */
        resultPtr = Tcl_NewListObj(0, NULL);
        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->options, &place);
            while (hPtr) {
                ioptPtr = (ItclOption*)Tcl_GetHashValue(hPtr);
                objPtr = ioptPtr->namePtr;
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoComponentCmd()
 *
 *  Returns information regarding class components.
 *  Handles the following syntax:
 *
 *    info component ?componentName? ?-inherit? ?-name? ?-value?
 *
 *  If the ?componentName? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInfoComponentCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    char *componentName = NULL;
    Tcl_Obj *resultPtr = NULL;
    Tcl_Obj *objPtr = NULL;
    Tcl_Obj *componentNamePtr;

    static const char *components[] = {
	"-name", "-inherit", "-value", NULL
    };
    enum BCompIdx {
	BCompNameIdx, BCompInheritIdx, BCompValueIdx
    } *icomplist, icomplistStorage[3];

    static enum BCompIdx DefInfoComponent[3] = {
        BCompNameIdx,
        BCompInheritIdx,
        BCompValueIdx
    };

    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclObjectInfo *infoPtr;

    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    Tcl_Namespace *nsPtr;
    ItclComponent *icPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr;
    const char *val;
    int i;
    int result;

    ItclShowArgs(1, "Itcl_BiInfoComponentCmd", objc, objv);
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info component ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    nsPtr = Itcl_GetUplevelNamespace(interp, 1);
    if (nsPtr->parentPtr == NULL) {
        /* :: namespace */
	nsPtr = contextIclsPtr->nsPtr;
    }
    infoPtr = contextIclsPtr->infoPtr;
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "cannot find class name for namespace \"",
	        nsPtr->fullName, "\"", NULL);
	return TCL_ERROR;
    }
    contextIclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);

    /*
     *  Process args:
     *  ?componentName? ?-inherit? ?-name? ?-value?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        componentName = Tcl_GetString(*objv);
        objc--;
	objv++;
    }

    /*
     *  Return info for a specific component.
     */
    if (componentName) {
	componentNamePtr = Tcl_NewStringObj(componentName, -1);
	if (contextIoPtr != NULL) {
	    Itcl_InitHierIter(&hier, contextIoPtr->iclsPtr);
	} else {
	    Itcl_InitHierIter(&hier, contextIclsPtr);
	}
	while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
	    hPtr = Tcl_FindHashEntry(&iclsPtr->components,
	            (char *)componentNamePtr);
	    if (hPtr != NULL) {
	        break;
	    }
	}
	Tcl_DecrRefCount(componentNamePtr);
	Itcl_DeleteHierIter(&hier);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", componentName, "\" isn't a component in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
        }
        icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);

        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            icomplist = DefInfoComponent;
            objc = 3;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            icomplist = &icomplistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    components, sizeof(char *), "component", 0, (int*)(&icomplist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (icomplist[i]) {
                case BCompNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(icPtr->ivPtr->fullNamePtr), -1);
                    break;

                case BCompInheritIdx:
		    if (icPtr->flags & ITCL_COMPONENT_INHERIT) {
                        val = "1";
		    } else {
                        val = "0";
		    }
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;

                case BCompValueIdx:
		    if (contextIoPtr == NULL) {
                        Tcl_ResetResult(interp);
                        Tcl_AppendResult(interp,
                                "cannot access object-specific info ",
                                "without an object context",
                                NULL);
                        return TCL_ERROR;
                    } else {
                        val = ItclGetInstanceVar(interp,
			        Tcl_GetString(icPtr->namePtr), NULL,
                                    contextIoPtr, icPtr->ivPtr->iclsPtr);
                    }
                    if (val == NULL) {
                        val = "<undefined>";
                    }
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;
            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available components.
         */
        resultPtr = Tcl_NewListObj(0, NULL);
        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->components, &place);
            while (hPtr) {
                icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
                objPtr = Tcl_NewStringObj(
		        Tcl_GetString(icPtr->ivPtr->fullNamePtr), -1);
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoWidgetCmd()
 *
 *  Returns information regarding widget classes.
 *  Handles the following syntax:
 *
 *    info widget ?widgetName?
 *
 *  If the ?widgetName? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoWidgetCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Namespace *contextNs = NULL;
    Tcl_Obj *objPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    char *name;

    ItclShowArgs(1, "Itcl_BiInfoWidgetCmd", objc, objv);
    if (objc != 1) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"info widget\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        /* try it the hard way */
	ClientData clientData;
        ItclObjectInfo *infoPtr;
        Tcl_Object oPtr;
	clientData = Itcl_GetCallFrameClientData(interp);
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
	if (clientData != NULL) {
            oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData);
            contextIoPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
	            infoPtr->object_meta_type);
            contextIclsPtr = contextIoPtr->iclsPtr;
	}
	if ((contextIoPtr == NULL) || (contextIclsPtr == NULL)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "\nget info like this instead: "
		    "\n  namespace eval className { info widget ... }", -1));
            return TCL_ERROR;
        }
    }

    /*
     *  If there is an object context, then return the most-specific
     *  class for the object.  Otherwise, return the class namespace
     *  name.  Use normal class names when possible.
     */
    if (contextIoPtr) {
        contextNs = contextIoPtr->iclsPtr->nsPtr;
    } else {
        assert(contextIclsPtr != NULL);
        assert(contextIclsPtr->nsPtr != NULL);
        if (contextIclsPtr->infoPtr->useOldResolvers) {
            contextNs = contextIclsPtr->nsPtr;
        } else {
            contextNs = contextIclsPtr->nsPtr;
        }
    }

            name = contextNs->fullName;
    if (!(contextIclsPtr->flags & ITCL_WIDGET)) {
	Tcl_AppendResult(interp, "object or class is no widget", NULL);
        return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj(name, -1);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoExtendedClassCmd()
 *
 *  Returns information regarding extendedclasses.
 *  Handles the following syntax:
 *
 *    info extendedclass ?className?
 *
 *  If the ?className? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoExtendedClassCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
#ifdef NOTYET
    static const char *components[] = {
	"-name", "-inherit", "-value", NULL
    };
    enum BCompIdx {
	BCompNameIdx, BCompInheritIdx, BCompValueIdx
    } *icomplist, icomplistStorage[3];

    static enum BCompIdx DefInfoComponent[3] = {
        BCompNameIdx,
        BCompInheritIdx,
        BCompValueIdx
    };

    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclObjectInfo *infoPtr;

    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    Tcl_Namespace *nsPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr;
    const char *name;
    int result;

    ItclShowArgs(1, "Itcl_BiInfoExtendedClassCmd", objc, objv);
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info extendedclass ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    nsPtr = Itcl_GetUplevelNamespace(interp, 1);
    if (nsPtr->parentPtr == NULL) {
        /* :: namespace */
	nsPtr = contextIclsPtr->nsPtr;
    }
    infoPtr = contextIclsPtr->infoPtr;
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "cannot find class name for namespace \"",
	        nsPtr->fullName, "\"", NULL);
	return TCL_ERROR;
    }
    contextIclsPtr = Tcl_GetHashValue(hPtr);

#endif

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDelegatedCmd()
 *
 *  Returns information regarding extendedclasses.
 *  Handles the following syntax:
 *
 *    info extendedclass ?className?
 *
 *  If the ?className? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
#ifdef NOTYET
    static const char *components[] = {
	"-name", "-inherit", "-value", NULL
    };
    enum BCompIdx {
	BCompNameIdx, BCompInheritIdx, BCompValueIdx
    } *icomplist, icomplistStorage[3];

    static enum BCompIdx DefInfoComponent[3] = {
        BCompNameIdx,
        BCompInheritIdx,
        BCompValueIdx
    };

    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclObjectInfo *infoPtr;

    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    Tcl_Namespace *nsPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr;
    const char *name;
    int result;

    ItclShowArgs(1, "Itcl_BiInfoDelegatedCmd", objc, objv);
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info delegated ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    nsPtr = Itcl_GetUplevelNamespace(interp, 1);
    if (nsPtr->parentPtr == NULL) {
        /* :: namespace */
	nsPtr = contextIclsPtr->nsPtr;
    }
    infoPtr = contextIclsPtr->infoPtr;
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "cannot find class name for namespace \"",
	        nsPtr->fullName, "\"", NULL);
	return TCL_ERROR;
    }
    contextIclsPtr = Tcl_GetHashValue(hPtr);

#endif

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoTypeCmd()
 *
 *  Returns information regarding the Type for an object.  This command
 *  can be invoked with or without an object context:
 *
 *    <objName> info type   <= returns most-specific class name
 *    info type             <= returns active namespace name
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoTypeCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Namespace *contextNs = NULL;
    Tcl_Obj *objPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    char *name;

    ItclShowArgs(1, "Itcl_BiInfoTypeCmd", objc, objv);
    if (objc != 1) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"info type\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        /* try it the hard way */
	ClientData clientData;
        ItclObjectInfo *infoPtr;
        Tcl_Object oPtr;
	clientData = Itcl_GetCallFrameClientData(interp);
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
	if (clientData != NULL) {
            oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData);
            contextIoPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
	            infoPtr->object_meta_type);
            contextIclsPtr = contextIoPtr->iclsPtr;
	}
	if ((contextIoPtr == NULL) || (contextIclsPtr == NULL)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "\nget info like this instead: "
		    "\n  namespace eval className { info type ...}", -1));
            return TCL_ERROR;
        }
    }

    /*
     *  If there is an object context, then return the most-specific
     *  class for the object.  Otherwise, return the class namespace
     *  name.  Use normal class names when possible.
     */
    if (contextIoPtr) {
        contextNs = contextIoPtr->iclsPtr->nsPtr;
    } else {
        assert(contextIclsPtr != NULL);
        assert(contextIclsPtr->nsPtr != NULL);
        if (contextIclsPtr->infoPtr->useOldResolvers) {
            contextNs = contextIclsPtr->nsPtr;
        } else {
            contextNs = contextIclsPtr->nsPtr;
        }
    }

            name = contextNs->fullName;
    if (!(contextIclsPtr->flags & ITCL_TYPE)) {
	Tcl_AppendResult(interp, "object or class is no type", NULL);
        return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj(name, -1);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoHullTypeCmd()
 *
 *  Returns information regarding the hulltype for an object.  This command
 *  can be invoked with or without an object context:
 *
 *    <objName> info hulltype   returns the hulltype name
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoHullTypeCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    ItclShowArgs(1, "Itcl_BiInfoHullTypeCmd", objc, objv);
    if (objc != 1) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"info hulltype\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        /* try it the hard way */
	ClientData clientData;
        ItclObjectInfo *infoPtr;
        Tcl_Object oPtr;
	clientData = Itcl_GetCallFrameClientData(interp);
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
	if (clientData != NULL) {
            oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData);
            contextIoPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
	            infoPtr->object_meta_type);
            contextIclsPtr = contextIoPtr->iclsPtr;
	}
	if ((contextIoPtr == NULL) || (contextIclsPtr == NULL)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "\nget info like this instead: "
		    "\n  namespace eval className { info hulltype ... }", -1));
            return TCL_ERROR;
        }
    }

    if (!(contextIclsPtr->flags & ITCL_WIDGET)) {
	Tcl_AppendResult(interp, "object or class is no widget.",
	        " Only ::itcl::widget has a hulltype.", NULL);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, contextIclsPtr->hullTypePtr);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDefaultCmd()
 *
 *  Returns information regarding the Type for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDefaultCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclMemberFunc *imPtr;
    ItclDelegatedFunction *idmPtr;
    ItclArgList *argListPtr;
    const char *methodName;
    const char *argName;
    const char *what;
    int found;

    ItclShowArgs(1, "Itcl_BiInfoDefaultCmd", objc, objv);
    iclsPtr = NULL;
    found = 0;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    if (objc != 4) {
	Tcl_AppendResult(interp, "wrong # args, should be info default ",
	        "<method> <argName> <varName>", NULL);
        return TCL_ERROR;
    }
    methodName = Tcl_GetString(objv[1]);
    argName = Tcl_GetString(objv[2]);
    FOREACH_HASH_VALUE(imPtr, &iclsPtr->functions) {
        if (strcmp(methodName, Tcl_GetString(imPtr->namePtr)) == 0) {
	    found = 1;
	    break;
	}
    }
    if (found) {
        argListPtr = imPtr->argListPtr;
	while (argListPtr != NULL) {
	    if (strcmp(argName, Tcl_GetString(argListPtr->namePtr)) == 0) {
	        if (argListPtr->defaultValuePtr != NULL) {
		    if (NULL == Tcl_ObjSetVar2(interp, objv[3], NULL,
			    argListPtr->defaultValuePtr, TCL_LEAVE_ERR_MSG)) {
			return TCL_ERROR;
		    }
		    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(1));
		    return TCL_OK;
	        } else {
	            Tcl_AppendResult(interp, "method \"", methodName,
		            "\" has no default value for argument \"",
			    argName, "\"", NULL);
		    return TCL_ERROR;
	        }
	    }
	    argListPtr = argListPtr->nextPtr;
	}
    }
    if (! found) {
        FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	    if (strcmp(methodName, Tcl_GetString(idmPtr->namePtr)) == 0) {
		what = "method";
		if (idmPtr->flags & ITCL_TYPE_METHOD) {
		    what = "typemethod";
		}
                Tcl_AppendResult(interp, "delegated ", what, " \"", methodName,
		        "\"", NULL);
                return TCL_ERROR;
	    }
	}
    }
    if (! found) {
        Tcl_AppendResult(interp, "unknown method \"", methodName, "\"", NULL);
        return TCL_ERROR;
    }
    Tcl_AppendResult(interp, "method \"", methodName, "\" has no argument \"",
            argName, "\"", NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoMethodCmd()
 *
 *  Returns information regarding a method for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoMethodCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclClass *iclsPtr;
    ItclMemberFunc *imPtr;
    ItclMemberCode *mcode;
    ItclHierIter hier;
    const char *val;
    char *cmdName;
    int i;
    int result;

    static const char *options[] = {
        "-args", "-body", "-name", "-protection", "-type",
        NULL
    };
    enum BIfIdx {
        BIfArgsIdx, BIfBodyIdx, BIfNameIdx, BIfProtectIdx, BIfTypeIdx
    } *iflist, iflistStorage[5];

    static enum BIfIdx DefInfoFunction[5] = {
        BIfProtectIdx,
        BIfTypeIdx,
        BIfNameIdx,
        BIfArgsIdx,
        BIfBodyIdx
    };

    ItclShowArgs(1, "Itcl_BiInfoMethodCmd", objc, objv);
    cmdName = NULL;
    objPtr = NULL;
    resultPtr = NULL;
    contextIclsPtr = NULL;
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info method ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?cmdName? ?-protection? ?-type? ?-name? ?-args? ?-body?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        cmdName = Tcl_GetString(*objv);
        objc--; objv++;
    }

    /*
     *  Return info for a specific command.
     */
    if (cmdName) {
	ItclCmdLookup *clookup;
	objPtr = Tcl_NewStringObj(cmdName, -1);
        hPtr = Tcl_FindHashEntry(&contextIclsPtr->resolveCmds, (char *)objPtr);
	Tcl_DecrRefCount(objPtr);
	objPtr = NULL;
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", cmdName, "\" isn't a method in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
        }

	clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
	imPtr = clookup->imPtr;
        mcode = imPtr->codePtr;
        if (imPtr->flags & ITCL_COMMON) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", cmdName, "\" isn't a method in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
	}

        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            objc = 5;
            iflist = DefInfoFunction;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            iflist = &iflistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&iflist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (iflist[i]) {
                case BIfArgsIdx:
                    if (mcode && mcode->argListPtr) {
			if (imPtr->usagePtr == NULL) {
                            objPtr = Tcl_NewStringObj(
			            Tcl_GetString(mcode->usagePtr), -1);
			} else {
                            objPtr = Tcl_NewStringObj(
			            Tcl_GetString(imPtr->usagePtr), -1);
		        }
                    } else {
		        if ((imPtr->flags & ITCL_ARG_SPEC) != 0) {
			    if (imPtr->usagePtr == NULL) {
                                objPtr = Tcl_NewStringObj(
				        Tcl_GetString(mcode->usagePtr), -1);
			    } else {
			        objPtr = Tcl_NewStringObj(
				        Tcl_GetString(imPtr->usagePtr), -1);
			    }
                        } else {
                            objPtr = Tcl_NewStringObj("<undefined>", -1);
                        }
		    }
                    break;

                case BIfBodyIdx:
                    if (mcode && Itcl_IsMemberCodeImplemented(mcode)) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(mcode->bodyPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("<undefined>", -1);
                    }
                    break;

                case BIfNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(imPtr->fullNamePtr), -1);
                    break;

                case BIfProtectIdx:
                    val = Itcl_ProtectionStr(imPtr->protection);
                    objPtr = Tcl_NewStringObj(val, -1);
                    break;

                case BIfTypeIdx:
                    val = "method";
                    objPtr = Tcl_NewStringObj(val, -1);
                    break;
            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available commands.
         */
        resultPtr = Tcl_NewListObj(0, NULL);

        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->functions, &place);
            while (hPtr) {
	        int useIt = 1;

                imPtr = (ItclMemberFunc*)Tcl_GetHashValue(hPtr);
		if (!(imPtr->flags & ITCL_METHOD)) {
		    useIt = 0;
		}
		if (useIt) {
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(imPtr->fullNamePtr), -1);
                    Tcl_ListObjAppendElement(NULL,
		            resultPtr, objPtr);
                }

                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoMethodsCmd()
 *
 *  Returns information regarding the methods for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoMethodsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclMemberFunc *imPtr;
    ItclDelegatedFunction *idmPtr;
    const char *name;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoMethodsCmd", objc, objv);
    iclsPtr = NULL;
    pattern = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        Tcl_AppendResult(interp, "cannot get context ", NULL);
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    listPtr = Tcl_NewListObj(0, NULL);
    name = "destroy";
    if ((pattern == NULL) || Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
        Tcl_ListObjAppendElement(interp, listPtr,
                Tcl_NewStringObj(name, -1));
    }
    name = "info";
    if ((pattern == NULL) || Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
        Tcl_ListObjAppendElement(interp, listPtr,
                Tcl_NewStringObj(name, -1));
    }
    FOREACH_HASH_VALUE(imPtr, &iclsPtr->functions) {
	name = Tcl_GetString(imPtr->namePtr);
        if (strcmp(name, "*") == 0) {
	    continue;
	}
        if (strcmp(name, "destroy") == 0) {
	    continue;
	}
        if (strcmp(name, "info") == 0) {
	    continue;
	}
        if ((imPtr->flags & ITCL_METHOD) &&
	        !(imPtr->flags & ITCL_CONSTRUCTOR) &&
	        !(imPtr->flags & ITCL_DESTRUCTOR) &&
		!(imPtr->flags & ITCL_COMMON) &&
	        !(imPtr->codePtr->flags & ITCL_BUILTIN)) {
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
	        Tcl_ListObjAppendElement(interp, listPtr,
		        Tcl_NewStringObj(Tcl_GetString(imPtr->namePtr), -1));
	    }
	}
    }
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	name = Tcl_GetString(idmPtr->namePtr);
        if (strcmp(name, "*") == 0) {
	    continue;
	}
        if (strcmp(name, "destroy") == 0) {
	    continue;
	}
        if (strcmp(name, "info") == 0) {
	    continue;
	}
        if (idmPtr->flags & ITCL_METHOD) {
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
	        Tcl_ListObjAppendElement(interp, listPtr,
		        Tcl_NewStringObj(Tcl_GetString(idmPtr->namePtr), -1));
	    }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoOptionsCmd()
 *
 *  Returns information regarding the Type for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoOptionsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_HashEntry *hPtr2;
    Tcl_Obj *listPtr;
    Tcl_Obj *listPtr2;
    Tcl_Obj *objPtr;
    Tcl_Obj **lObjv;
    Tcl_HashTable *tablePtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclOption *ioptPtr;
    ItclDelegatedOption *idoPtr;
    const char *name;
    const char *val;
    const char *pattern;
    int lObjc;
    int result;
    int i;

    ItclShowArgs(1, "Itcl_BiInfoOptionsCmd", objc, objv);
    iclsPtr = NULL;
    pattern = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        Tcl_AppendResult(interp, "cannot get context ", NULL);
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args should be: info options ",
	        "?pattern?", NULL);
        return TCL_ERROR;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    listPtr = Tcl_NewListObj(0, NULL);
    if (ioPtr == NULL) {
        tablePtr = &iclsPtr->options;
    } else {
        tablePtr = &ioPtr->objectOptions;
    }
    FOREACH_HASH_VALUE(ioptPtr, tablePtr) {
	name = Tcl_GetString(ioptPtr->namePtr);
	if ((pattern == NULL) ||
                 Tcl_StringCaseMatch(name, pattern, 0)) {
            Tcl_ListObjAppendElement(interp, listPtr,
	            Tcl_NewStringObj(Tcl_GetString(ioptPtr->namePtr), -1));
        }
    }
    if (ioPtr == NULL) {
        tablePtr = &iclsPtr->delegatedOptions;
    } else {
        tablePtr = &ioPtr->objectDelegatedOptions;
    }
    FOREACH_HASH_VALUE(idoPtr, tablePtr) {
        name = Tcl_GetString(idoPtr->namePtr);
	if (strcmp(name, "*") != 0) {
	    if ((pattern == NULL) ||
                    Tcl_StringCaseMatch(name, pattern, 0)) {
                Tcl_ListObjAppendElement(interp, listPtr,
	                Tcl_NewStringObj(Tcl_GetString(idoPtr->namePtr), -1));
	    }
        } else {
	    if (idoPtr->icPtr == NULL) {
		Tcl_AppendResult(interp, "component \"",
		        Tcl_GetString(idoPtr->namePtr),
			"\" is not initialized", NULL);
	        return TCL_ERROR;
	    }
	    val = ItclGetInstanceVar(interp,
	            Tcl_GetString(idoPtr->icPtr->namePtr),
	            NULL, ioPtr, ioPtr->iclsPtr);
            if ((val != NULL) && (strlen(val) != 0)) {
	        objPtr = Tcl_NewStringObj(val, -1);
		Tcl_AppendToObj(objPtr, " configure", -1);
		result = Tcl_EvalObjEx(interp, objPtr, 0);
	        if (result != TCL_OK) {
		    return TCL_ERROR;
		}
	        listPtr2 = Tcl_GetObjResult(interp);
		Tcl_ListObjGetElements(interp, listPtr2, &lObjc, &lObjv);
		for (i = 0; i < lObjc; i++) {
		    Tcl_ListObjIndex(interp, lObjv[i], 0, &objPtr);
		    hPtr2 = Tcl_FindHashEntry(&idoPtr->exceptions,
		            (char *)objPtr);
		    if (hPtr2 == NULL) {
			name = Tcl_GetString(objPtr);
	                if ((pattern == NULL) ||
                                 Tcl_StringCaseMatch(name, pattern, 0)) {
		            Tcl_ListObjAppendElement(interp, listPtr, objPtr);
		        }
		    }
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoTypesCmd()
 *
 *  Returns information regarding the Type for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoTypesCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    const char *name;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoTypesCmd", objc, objv);
    infoPtr = (ItclObjectInfo *)clientData;
    iclsPtr = NULL;
    pattern = NULL;
    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args should be: info types ",
	        "?pattern?", NULL);
        return TCL_ERROR;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    listPtr = Tcl_NewListObj(0, NULL);
    FOREACH_HASH_VALUE(iclsPtr, &infoPtr->nameClasses) {
	if (iclsPtr->flags & ITCL_TYPE) {
	    name = Tcl_GetString(iclsPtr->namePtr);
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch(name, pattern, 0)) {
                Tcl_ListObjAppendElement(interp, listPtr,
		        Tcl_NewStringObj(Tcl_GetString(iclsPtr->namePtr), -1));
            }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoComponentsCmd()
 *
 *  Returns information regarding the Components for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoComponentsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclComponent *icPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr2;
    const char *name;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoComponentsCmd", objc, objv);
    iclsPtr = NULL;
    pattern = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        Tcl_AppendResult(interp, "cannot get context ", NULL);
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "INTERNAL ERROR in Itcl_BiInfoComponentsCmd",
	        " iclsPtr == NULL", NULL);
        return TCL_ERROR;
    }
    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args should be: info components ",
	        "?pattern?", NULL);
        return TCL_ERROR;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    listPtr = Tcl_NewListObj(0, NULL);
    Itcl_InitHierIter(&hier, iclsPtr);
    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    while (iclsPtr2 != NULL) {
        FOREACH_HASH_VALUE(icPtr, &iclsPtr2->components) {
            name = Tcl_GetString(icPtr->namePtr);
            if ((pattern == NULL) ||
                     Tcl_StringCaseMatch(name, pattern, 0)) {
                Tcl_ListObjAppendElement(interp, listPtr,
	                Tcl_NewStringObj(Tcl_GetString(icPtr->namePtr), -1));
            }
        }
        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    }
    Itcl_DeleteHierIter(&hier);
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoTypeMethodCmd()
 *
 *  Returns information regarding a typemethod for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoTypeMethodCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclClass *iclsPtr;
    ItclMemberFunc *imPtr;
    ItclMemberCode *mcode;
    ItclHierIter hier;
    const char *val;
    char *cmdName;
    int i;
    int result;

    static const char *options[] = {
        "-args", "-body", "-name", "-protection", "-type",
        NULL
    };
    enum BIfIdx {
        BIfArgsIdx, BIfBodyIdx, BIfNameIdx, BIfProtectIdx, BIfTypeIdx
    } *iflist, iflistStorage[5];

    static enum BIfIdx DefInfoFunction[5] = {
        BIfProtectIdx,
        BIfTypeIdx,
        BIfNameIdx,
        BIfArgsIdx,
        BIfBodyIdx
    };


    ItclShowArgs(1, "Itcl_BiInfoTypeMethodCmd", objc, objv);
    resultPtr = NULL;
    objPtr = NULL;
    cmdName = NULL;
    contextIclsPtr = NULL;
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info function ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?cmdName? ?-protection? ?-type? ?-name? ?-args? ?-body?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        cmdName = Tcl_GetString(*objv);
        objc--; objv++;
    }

    /*
     *  Return info for a specific command.
     */
    if (cmdName) {
	ItclCmdLookup *clookup;
	objPtr = Tcl_NewStringObj(cmdName, -1);
        hPtr = Tcl_FindHashEntry(&contextIclsPtr->resolveCmds, (char *)objPtr);
	Tcl_DecrRefCount(objPtr);
	objPtr = NULL;
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", cmdName, "\" isn't a typemethod in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
        }

	clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
	imPtr = clookup->imPtr;
        mcode = imPtr->codePtr;
	if (!(imPtr->flags & ITCL_TYPE_METHOD)) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", cmdName, "\" isn't a typemethod in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
	}

        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            objc = 5;
            iflist = DefInfoFunction;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            iflist = &iflistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&iflist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (iflist[i]) {
                case BIfArgsIdx:
                    if (mcode && mcode->argListPtr) {
			if (imPtr->usagePtr == NULL) {
                            objPtr = Tcl_NewStringObj(
			            Tcl_GetString(mcode->usagePtr), -1);
			} else {
                            objPtr = Tcl_NewStringObj(
			            Tcl_GetString(imPtr->usagePtr), -1);
		        }
                    } else {
		        if ((imPtr->flags & ITCL_ARG_SPEC) != 0) {
			    if (imPtr->usagePtr == NULL) {
                                objPtr = Tcl_NewStringObj(
				        Tcl_GetString(mcode->usagePtr), -1);
			    } else {
			        objPtr = Tcl_NewStringObj(
				        Tcl_GetString(imPtr->usagePtr), -1);
			    }
                        } else {
                            objPtr = Tcl_NewStringObj("<undefined>", -1);
                        }
		    }
                    break;

                case BIfBodyIdx:
                    if (mcode && Itcl_IsMemberCodeImplemented(mcode)) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(mcode->bodyPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("<undefined>", -1);
                    }
                    break;

                case BIfNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(imPtr->fullNamePtr), -1);
                    break;

                case BIfProtectIdx:
                    val = Itcl_ProtectionStr(imPtr->protection);
                    objPtr = Tcl_NewStringObj(val, -1);
                    break;

                case BIfTypeIdx:
                    val = "typemethod";
                    objPtr = Tcl_NewStringObj(val, -1);
                    break;
            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available commands.
         */
        resultPtr = Tcl_NewListObj(0, NULL);

        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->functions, &place);
            while (hPtr) {
	        int useIt = 1;

                imPtr = (ItclMemberFunc*)Tcl_GetHashValue(hPtr);
		if (!(imPtr->flags & ITCL_TYPE_METHOD)) {
		    useIt = 0;
		}
		if (useIt) {
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(imPtr->fullNamePtr), -1);
                    Tcl_ListObjAppendElement(NULL,
		            resultPtr, objPtr);
                }

                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoMethodsCmd()
 *
 *  Returns information regarding the methods for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoTypeMethodsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclMemberFunc *imPtr;
    ItclDelegatedFunction *idmPtr;
    const char *name;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoTypeMethodsCmd", objc, objv);
    iclsPtr = NULL;
    pattern = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        Tcl_AppendResult(interp, "cannot get context ", NULL);
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    if (objc > 1) {
        pattern = Tcl_GetString(objv[1]);
    }
    listPtr = Tcl_NewListObj(0, NULL);
    name = "create";
    if ((pattern == NULL) || Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj(name, -1));
    }
    name = "destroy";
    if ((pattern == NULL) || Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
        Tcl_ListObjAppendElement(interp, listPtr,
                Tcl_NewStringObj(name, -1));
    }
    name = "info";
    if ((pattern == NULL) || Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
        Tcl_ListObjAppendElement(interp, listPtr,
                Tcl_NewStringObj(name, -1));
    }
    FOREACH_HASH_VALUE(imPtr, &iclsPtr->functions) {
	name = Tcl_GetString(imPtr->namePtr);
        if (strcmp(name, "*") == 0) {
	    continue;
	}
        if (strcmp(name, "create") == 0) {
	    continue;
	}
        if (strcmp(name, "destroy") == 0) {
	    continue;
	}
        if (strcmp(name, "info") == 0) {
	    continue;
	}
        if (imPtr->flags & ITCL_TYPE_METHOD) {
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
	        Tcl_ListObjAppendElement(interp, listPtr,
		        Tcl_NewStringObj(Tcl_GetString(imPtr->namePtr), -1));
	    }
	}
    }
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	name = Tcl_GetString(idmPtr->namePtr);
        if (strcmp(name, "*") == 0) {
	    continue;
	}
        if (strcmp(name, "create") == 0) {
	    continue;
	}
        if (strcmp(name, "destroy") == 0) {
	    continue;
	}
        if (strcmp(name, "info") == 0) {
	    continue;
	}
        if (idmPtr->flags & ITCL_TYPE_METHOD) {
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch((const char *)name, pattern, 0)) {
	        Tcl_ListObjAppendElement(interp, listPtr,
		        Tcl_NewStringObj(Tcl_GetString(idmPtr->namePtr), -1));
	    }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoTypeVarsCmd()
 *
 *  Returns information regarding variables for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoTypeVarsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclVariable *ivPtr;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoTypeVarsCmd", objc, objv);
    if (objc > 2) {
	Tcl_AppendResult(interp,
	        "wrong # args should be: info typevars ?pattern?", NULL);
        return TCL_ERROR;
    }
    iclsPtr = NULL;
    pattern = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        Tcl_AppendResult(interp, "cannot get context ", NULL);
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    listPtr = Tcl_NewListObj(0, NULL);
    FOREACH_HASH_VALUE(ivPtr, &iclsPtr->variables) {
        if ((pattern == NULL) ||
            Tcl_StringCaseMatch(Tcl_GetString(ivPtr->namePtr), pattern, 0)) {
	    if (ivPtr->flags & ITCL_TYPE_VARIABLE) {
                Tcl_ListObjAppendElement(interp, listPtr, ivPtr->fullNamePtr);
	    }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoTypeVariableCmd()
 *
 *  Returns information regarding a typevariable for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoTypeVariableCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    ItclClass *iclsPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclVariable *ivPtr;
    ItclVarLookup *vlookup;
    ItclHierIter hier;
    char *varName;
    const char *val;
    int i;
    int result;

    static const char *options[] = {
        "-init", "-name", "-protection", "-type",
        "-value", NULL
    };
    enum BIvIdx {
        BIvInitIdx,
	BIvNameIdx,
	BIvProtectIdx,
        BIvTypeIdx,
	BIvValueIdx
    } *ivlist, ivlistStorage[5];

    static enum BIvIdx DefInfoVariable[5] = {
        BIvProtectIdx,
        BIvTypeIdx,
        BIvNameIdx,
        BIvInitIdx,
        BIvValueIdx
    };

    ItclShowArgs(1, "Itcl_BiInfoTypeVariableCmd", objc, objv);
    resultPtr = NULL;
    objPtr = NULL;
    varName = NULL;
    contextIclsPtr = NULL;
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info typevariable ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?varName? ?-protection? ?-type? ?-name? ?-init? ?-config? ?-value?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        varName = Tcl_GetString(*objv);
        objc--; objv++;
    }

    /*
     *  Return info for a specific variable.
     */
    if (varName) {
        hPtr = ItclResolveVarEntry(contextIclsPtr, varName);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", varName, "\" isn't a typevariable in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
        }
        vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
        ivPtr = vlookup->ivPtr;
        if (!(ivPtr->flags & ITCL_TYPE_VARIABLE)) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "\"", varName, "\" isn't a typevariable in class \"",
                contextIclsPtr->nsPtr->fullName, "\"",
                NULL);
            return TCL_ERROR;
	}
        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            ivlist = DefInfoVariable;
            objc = 5;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            ivlist = &ivlistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&ivlist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (ivlist[i]) {
                case BIvInitIdx:
                    /*
                     *  If this is the built-in "this" variable, then
                     *  report the object name as its initialization string.
                     */
                    if ((ivPtr->flags & ITCL_THIS_VAR) != 0) {
                        if ((contextIoPtr != NULL) &&
			        (contextIoPtr->accessCmd != NULL)) {
                            objPtr = Tcl_NewStringObj(NULL, 0);
                            Tcl_GetCommandFullName(
                                contextIoPtr->iclsPtr->interp,
                                contextIoPtr->accessCmd, objPtr);
                        } else {
                            objPtr = Tcl_NewStringObj("<objectName>", -1);
                        }
                    } else {
		        if (vlookup->ivPtr->init) {
			    objPtr = Tcl_NewStringObj(
			            Tcl_GetString(vlookup->ivPtr->init), -1);
                        } else {
                            objPtr = Tcl_NewStringObj("<undefined>", -1);
		        }
                    }
                    break;

                case BIvNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(ivPtr->fullNamePtr), -1);
                    break;

                case BIvProtectIdx:
                    val = Itcl_ProtectionStr(ivPtr->protection);
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;

                case BIvTypeIdx:
                    val = ((ivPtr->flags & ITCL_COMMON) != 0)
                        ? "common" : "variable";
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;

                case BIvValueIdx:
                    if ((ivPtr->flags & ITCL_COMMON) != 0) {
                        val = Itcl_GetCommonVar(interp,
			        Tcl_GetString(ivPtr->fullNamePtr),
                                ivPtr->iclsPtr);
                    } else {
		        if (contextIoPtr == NULL) {
                            if (objc > 1) {
			        Tcl_DecrRefCount(resultPtr);
			    }
                            Tcl_ResetResult(interp);
                            Tcl_AppendResult(interp,
                                    "cannot access object-specific info ",
                                    "without an object context",
                                    NULL);
                            return TCL_ERROR;
                        } else {
                            val = Itcl_GetInstanceVar(interp,
			            Tcl_GetString(ivPtr->namePtr),
                                    contextIoPtr, ivPtr->iclsPtr);
                        }
                    }
                    if (val == NULL) {
                        val = "<undefined>";
                    }
                    objPtr = Tcl_NewStringObj((const char *)val, -1);
                    break;
            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
            }
        }
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, Tcl_GetString(resultPtr), NULL);
	Tcl_DecrRefCount(resultPtr);
    } else {

        /*
         *  Return the list of available variables.  Report the built-in
         *  "this" variable only once, for the most-specific class.
         */
        resultPtr = Tcl_NewListObj(0, NULL);
        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->variables, &place);
            while (hPtr) {
                ivPtr = (ItclVariable*)Tcl_GetHashValue(hPtr);
		if (ivPtr->flags & ITCL_TYPE_VAR) {
                    if ((ivPtr->flags & ITCL_THIS_VAR) != 0) {
                        if (iclsPtr == contextIclsPtr) {
                            objPtr = Tcl_NewStringObj(
			            Tcl_GetString(ivPtr->fullNamePtr), -1);
                            Tcl_ListObjAppendElement(NULL,
                                resultPtr, objPtr);
                        }
                    } else {
                        objPtr = Tcl_NewStringObj(
		                Tcl_GetString(ivPtr->fullNamePtr), -1);
                        Tcl_ListObjAppendElement(NULL,
                            resultPtr, objPtr);
                    }
		}
                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoVariablesCmd()
 *
 *  Returns information regarding typevariables for an object.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoVariablesCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    ItclShowArgs(1, "Itcl_BiInfoVariablesCmd", objc, objv);
    Tcl_AppendResult(interp, "Itcl_BiInfoVariablesCmd not yet implemented\n",
            NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoWidgetadaptorCmd()
 *
 *  Returns information regarding a widgetadaptor.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoWidgetadaptorCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    Tcl_Namespace *contextNs = NULL;
    Tcl_Obj *objPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    char *name;

    ItclShowArgs(1, "Itcl_BiInfoWidgetadaptorCmd", objc, objv);
    if (objc != 1) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"info widgetadaptor\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        /* try it the hard way */
	ClientData clientData;
        ItclObjectInfo *infoPtr;
        Tcl_Object oPtr;
	clientData = Itcl_GetCallFrameClientData(interp);
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
	if (clientData != NULL) {
            oPtr = Tcl_ObjectContextObject((Tcl_ObjectContext)clientData);
            contextIoPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
	            infoPtr->object_meta_type);
            contextIclsPtr = contextIoPtr->iclsPtr;
	}
	if ((contextIoPtr == NULL) || (contextIclsPtr == NULL)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "\nget info like this instead: "
		    "\n  namespace eval className { info widgetadaptor ... }", -1));
            return TCL_ERROR;
        }
    }

    /*
     *  If there is an object context, then return the most-specific
     *  class for the object.  Otherwise, return the class namespace
     *  name.  Use normal class names when possible.
     */
    if (contextIoPtr) {
        contextNs = contextIoPtr->iclsPtr->nsPtr;
    } else {
        assert(contextIclsPtr != NULL);
        assert(contextIclsPtr->nsPtr != NULL);
        if (contextIclsPtr->infoPtr->useOldResolvers) {
            contextNs = contextIclsPtr->nsPtr;
        } else {
            contextNs = contextIclsPtr->nsPtr;
        }
    }

            name = contextNs->fullName;
    if (!(contextIclsPtr->flags & ITCL_WIDGETADAPTOR)) {
	Tcl_AppendResult(interp, "object or class is no widgetadaptor", NULL);
        return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj(name, -1);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoInstancesCmd()
 *
 *  Returns information regarding instances.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoInstancesCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    ItclObjectInfo *infoPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoInstancesCmd", objc, objv);
    if (objc > 2) {
	Tcl_AppendResult(interp,
	        "wrong # args should be: info instances ?pattern?", NULL);
        return TCL_ERROR;
    }
    iclsPtr = NULL;
    pattern = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        Tcl_AppendResult(interp, "cannot get context ", NULL);
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    infoPtr = (ItclObjectInfo *)clientData;
    listPtr = Tcl_NewListObj(0, NULL);
    FOREACH_HASH_VALUE(ioPtr, &infoPtr->objects) {
	/* FIXME need to scan the inheritance too */
        if (ioPtr->iclsPtr == iclsPtr) {
	    if (ioPtr->iclsPtr->flags & ITCL_WIDGETADAPTOR) {
	        objPtr = Tcl_NewStringObj(Tcl_GetCommandName(interp,
		        ioPtr->accessCmd), -1);
	    } else {
	        objPtr = Tcl_NewObj();
	        Tcl_GetCommandFullName(interp, ioPtr->accessCmd, objPtr);
            }
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch(Tcl_GetString(objPtr), pattern, 0)) {
	        Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	    } else {
	        Tcl_DecrRefCount(objPtr);
	    }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDelegatedOptionsCmd()
 *
 *  Returns information regarding delegated options.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedOptionsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *objPtr2;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclDelegatedOption *idoPtr;
    const char *name;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoDelegatedOptionsCmd", objc, objv);
    pattern = NULL;
    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args should be: info delegated ",
	        "options ?pattern?", NULL);
        return TCL_ERROR;
    }

    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    listPtr = Tcl_NewListObj(0, NULL);
    FOREACH_HASH_VALUE(idoPtr, &iclsPtr->delegatedOptions) {
	if (iclsPtr->flags &
	        (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS)) {
	    name = Tcl_GetString(idoPtr->namePtr);
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch(name, pattern, 0)) {
		objPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, objPtr,
	                idoPtr->namePtr);
		if (idoPtr->icPtr != NULL) {
                    Tcl_ListObjAppendElement(interp, objPtr,
	                    idoPtr->icPtr->namePtr);
		} else {
		    objPtr2 = Tcl_NewStringObj("", -1);
                    Tcl_ListObjAppendElement(interp, objPtr, objPtr2);
		}
                Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	    }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDelegatedMethodsCmd()
 *
 *  Returns information regarding delegated methods.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedMethodsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *objPtr2;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclDelegatedFunction *idmPtr;
    const char *name;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoDelegatedMethodsCmd", objc, objv);
    pattern = NULL;
    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args should be: info delegated ",
	        "methods ?pattern?", NULL);
        return TCL_ERROR;
    }

    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    listPtr = Tcl_NewListObj(0, NULL);
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	if (iclsPtr->flags &
	        (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS)) {
	    name = Tcl_GetString(idmPtr->namePtr);
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch(name, pattern, 0)) {
		if ((idmPtr->flags & ITCL_TYPE_METHOD) == 0) {
		    objPtr = Tcl_NewListObj(0, NULL);
                    Tcl_ListObjAppendElement(interp, objPtr,
	                    idmPtr->namePtr);
		    if (idmPtr->icPtr != NULL) {
                        Tcl_ListObjAppendElement(interp, objPtr,
	                        idmPtr->icPtr->namePtr);
		    } else {
		        objPtr2 = Tcl_NewStringObj("", -1);
                        Tcl_ListObjAppendElement(interp, objPtr, objPtr2);
		    }
                    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	       }
	    }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoTypeMethodsCmd()
 *
 *  Returns information regarding delegated type methods.  This command
 *  can be invoked with or without an object context:
 *
 *
 *  Returns a status TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedTypeMethodsCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *objPtr2;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclDelegatedFunction *idmPtr;
    const char *name;
    const char *pattern;

    ItclShowArgs(1, "Itcl_BiInfoDelegatedTypeMethodsCmd", objc, objv);
    pattern = NULL;
    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args should be: info delegated ",
	        "typemethods ?pattern?", NULL);
        return TCL_ERROR;
    }

    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        iclsPtr = ioPtr->iclsPtr;
    }
    listPtr = Tcl_NewListObj(0, NULL);
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	if (iclsPtr->flags &
	        (ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET|ITCL_ECLASS)) {
	    name = Tcl_GetString(idmPtr->namePtr);
	    if ((pattern == NULL) ||
                     Tcl_StringCaseMatch(name, pattern, 0)) {
		if (idmPtr->flags & ITCL_TYPE_METHOD) {
		    objPtr = Tcl_NewListObj(0, NULL);
                    Tcl_ListObjAppendElement(interp, objPtr,
	                    idmPtr->namePtr);
		    if (idmPtr->icPtr != NULL) {
                        Tcl_ListObjAppendElement(interp, objPtr,
	                        idmPtr->icPtr->namePtr);
		    } else {
		            objPtr2 = Tcl_NewStringObj("", -1);
                        Tcl_ListObjAppendElement(interp, objPtr, objPtr2);
		    }
                    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	        }
	    }
        }
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDelegatedUnknownCmd()
 *
 *  the unknown handler for the ::itcl::builtin::Info::delagted ensemble
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedUnknownCmd(
    ClientData clientData,   /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *objPtr;

    ItclShowArgs(1, "Itcl_BiInfoDelegatedUnknownCmd", objc, objv);
    /* produce usage message */
    objPtr = Tcl_NewStringObj(
            "wrong # args: should be one of...\n", -1);
    ItclGetInfoDelegatedUsage(interp, objPtr, (ItclObjectInfo *)clientData);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDelegatedOptionCmd()
 *
 *  Returns information regarding class options.
 *  Handles the following syntax:
 *
 *    info delegated option ?optionName? ?-name? ?-resource? ?-class?
 *        ?-component? ?-as? ?-exceptions?
 *
 *  If the ?optionName? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedOptionCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_HashSearch place;
    Tcl_Namespace *nsPtr;
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *optionNamePtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclObjectInfo *infoPtr;
    ItclDelegatedOption *idoptPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr;
    char *optionName;
    int i;
    int result;

    static const char *options[] = {
        "-as", "-class", "-component", "-exceptions",
	"-name", "-resource", NULL
    };
    enum BOptIdx {
        BOptAsIdx, BOptClassIdx, BOptComponentIdx, BOptExceptionsIdx,
	BOptNameIdx, BOptResourceIdx
    } *ioptlist, ioptlistStorage[6];

    static enum BOptIdx DefInfoOption[6] = {
        BOptNameIdx,
        BOptResourceIdx,
        BOptClassIdx,
        BOptComponentIdx,
        BOptAsIdx,
        BOptExceptionsIdx
    };

    ItclShowArgs(1, "Itcl_BiInfoDelegatedOptionCmd", objc, objv);
    optionName = NULL;
    objPtr = NULL;
    resultPtr = NULL;
    contextIclsPtr = NULL;
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info delegated option ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    nsPtr = Itcl_GetUplevelNamespace(interp, 1);
    infoPtr = contextIclsPtr->infoPtr;
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "cannot find class name for namespace \"",
	        nsPtr->fullName, "\"", NULL);
	return TCL_ERROR;
    }
    contextIclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);

    /*
     *  Process args:
     *  ?optionName? ?-name? ?-resource? ?-class?
     * ?-as? ?-exceptions?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        optionName = Tcl_GetString(*objv);
        objc--;
	objv++;
    }

    /*
     *  Return info for a specific option.
     */
    if (optionName) {
	if (contextIoPtr == NULL) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "cannot access object-specific info ",
		    "without an object context", NULL);
	    return TCL_ERROR;
	}
	optionNamePtr = Tcl_NewStringObj(optionName, -1);
        hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedOptions,
	        (char *)optionNamePtr);
        Tcl_DecrRefCount(optionNamePtr);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                 "\"", optionName, "\" isn't an option in object \"",
                Tcl_GetString(contextIoPtr->namePtr), "\"",
                NULL);
            return TCL_ERROR;
        }
        idoptPtr = (ItclDelegatedOption*)Tcl_GetHashValue(hPtr);

        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            ioptlist = DefInfoOption;
            objc = 6;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            ioptlist = &ioptlistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&ioptlist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (ioptlist[i]) {
                case BOptAsIdx:
                    if (idoptPtr->asPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idoptPtr->asPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptExceptionsIdx:
		    {
		    Tcl_Obj *entryObj;
		    objPtr = Tcl_NewListObj(0, NULL);
		    FOREACH_HASH_VALUE(entryObj, &idoptPtr->exceptions) {
			Tcl_ListObjAppendElement(interp, objPtr, entryObj);
		    }
                    }
                    break;
                case BOptResourceIdx:
                    if (idoptPtr->resourceNamePtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idoptPtr->resourceNamePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptClassIdx:
                    if (idoptPtr->classNamePtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idoptPtr->classNamePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptComponentIdx:
                    if (idoptPtr->icPtr != NULL) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idoptPtr->icPtr->namePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(idoptPtr->namePtr), -1);
                    break;

            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available options.
         */
        resultPtr = Tcl_NewListObj(0, NULL);
        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->delegatedOptions, &place);
            while (hPtr) {
                idoptPtr = (ItclDelegatedOption*)Tcl_GetHashValue(hPtr);
                objPtr = idoptPtr->namePtr;
                Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDelegatedMethodCmd()
 *
 *  Returns information regarding class options.
 *  Handles the following syntax:
 *
 *    info delegated method ?methodName? ?-name?
 *        ?-component? ?-as? ?-using? ?-exceptions?
 *
 *  If the ?optionName? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedMethodCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_HashSearch place;
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *cmdNamePtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclDelegatedFunction *idmPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr;
    char *cmdName;
    int i;
    int result;

    static const char *options[] = {
        "-as", "-component", "-exceptions",
	"-name", "-using", NULL
    };
    enum BOptIdx {
        BOptAsIdx,
	BOptComponentIdx,
	BOptExceptionsIdx,
	BOptNameIdx,
	BOptUsingIdx
    } *ioptlist, ioptlistStorage[5];

    static enum BOptIdx DefInfoOption[5] = {
        BOptNameIdx,
        BOptComponentIdx,
        BOptAsIdx,
        BOptUsingIdx,
        BOptExceptionsIdx
    };

    ItclShowArgs(1, "Itcl_BiInfoDelegatedMethodCmd", objc, objv);
    cmdName = NULL;
    objPtr = NULL;
    resultPtr = NULL;
    contextIclsPtr = NULL;
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info delegated method ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?methodName? ?-name? ?-using?
     * ?-as? ?-component? ?-exceptions?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        cmdName = Tcl_GetString(*objv);
        objc--;
	objv++;
    }

    /*
     *  Return info for a specific option.
     */
    if (cmdName) {
	cmdNamePtr = Tcl_NewStringObj(cmdName, -1);
	if (contextIoPtr != NULL) {
            hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedFunctions,
	            (char *)cmdNamePtr);
	} else {
            hPtr = Tcl_FindHashEntry(&contextIclsPtr->delegatedFunctions,
	            (char *)cmdNamePtr);
	}
	Tcl_DecrRefCount(cmdNamePtr);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                 "\"", cmdName, "\" isn't a delegated method in object \"",
                Tcl_GetString(contextIoPtr->namePtr), "\"",
                NULL);
            return TCL_ERROR;
        }
        idmPtr = (ItclDelegatedFunction*)Tcl_GetHashValue(hPtr);
        if (!(idmPtr->flags & ITCL_METHOD)) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                 "\"", cmdName, "\" isn't a delegated method in object \"",
                Tcl_GetString(contextIoPtr->namePtr), "\"",
                NULL);
            return TCL_ERROR;
	}
        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            ioptlist = DefInfoOption;
            objc = 5;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            ioptlist = &ioptlistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&ioptlist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (ioptlist[i]) {
                case BOptAsIdx:
                    if (idmPtr->asPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idmPtr->asPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptExceptionsIdx:
		    {
		    Tcl_Obj *entryObj;
		    objPtr = Tcl_NewListObj(0, NULL);
		    FOREACH_HASH_VALUE(entryObj, &idmPtr->exceptions) {
			Tcl_ListObjAppendElement(interp, objPtr, entryObj);
		    }
                    }
                    break;
                case BOptUsingIdx:
                    if (idmPtr->usingPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idmPtr->usingPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptComponentIdx:
                    if (idmPtr->icPtr != NULL) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idmPtr->icPtr->namePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(idmPtr->namePtr), -1);
                    break;

            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available options.
         */
        resultPtr = Tcl_NewListObj(0, NULL);
        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->delegatedFunctions, &place);
            while (hPtr) {
                idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
		if (idmPtr->flags & ITCL_METHOD) {
                    objPtr = idmPtr->namePtr;
                    Tcl_ListObjAppendElement(NULL, resultPtr,
		            objPtr);
		}
                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInfoDelegatedTypeMethodCmd()
 *
 *  Returns information regarding class options.
 *  Handles the following syntax:
 *
 *    info delegated typemethod ?methodName? ?-name?
 *        ?-component? ?-as? ?-exceptions?
 *
 *  If the ?optionName? is not specified, then a list of all known
 *  data members is returned.  Otherwise, the information for a
 *  specific member is returned.  Returns a status TCL_OK/TCL_ERROR
 *  to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiInfoDelegatedTypeMethodCmd(
    ClientData clientData, /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,    /* current interpreter */
    int objc,              /* number of arguments */
    Tcl_Obj *const objv[]) /* argument objects */
{

    FOREACH_HASH_DECLS;
    Tcl_HashSearch place;
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *cmdNamePtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclDelegatedFunction *idmPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr;
    char *cmdName;
    int i;
    int result;

    static const char *options[] = {
        "-as", "-component", "-exceptions",
	"-name", "-using", NULL
    };
    enum BOptIdx {
        BOptAsIdx,
	BOptComponentIdx,
	BOptExceptionsIdx,
	BOptNameIdx,
	BOptUsingIdx
    } *ioptlist, ioptlistStorage[5];

    static enum BOptIdx DefInfoOption[5] = {
        BOptNameIdx,
        BOptComponentIdx,
        BOptAsIdx,
        BOptUsingIdx,
        BOptExceptionsIdx
    };

    ItclShowArgs(1, "Itcl_BiInfoDelegatedTypeMethodCmd", objc, objv);
    cmdName = NULL;
    objPtr = NULL;
    resultPtr = NULL;
    contextIclsPtr = NULL;
    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "\nget info like this instead: "
            "\n  namespace eval className { info delegated type method ... }", -1));
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    /*
     *  Process args:
     *  ?methodName? ?-name? ?-using?
     * ?-as? ?-component? ?-exceptions?
     */
    objv++;  /* skip over command name */
    objc--;

    if (objc > 0) {
        cmdName = Tcl_GetString(*objv);
        objc--;
	objv++;
    }

    /*
     *  Return info for a specific option.
     */
    if (cmdName) {
	cmdNamePtr = Tcl_NewStringObj(cmdName, -1);
	if (contextIoPtr != NULL) {
            hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedFunctions,
	            (char *)cmdNamePtr);
	} else {
            hPtr = Tcl_FindHashEntry(&contextIclsPtr->delegatedFunctions,
	            (char *)cmdNamePtr);
	}
	Tcl_DecrRefCount(cmdNamePtr);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                 "\"", cmdName, "\" isn't a delegated typemethod in ",
		contextIoPtr ? "object \"" : "class \"",
		contextIoPtr ?  Tcl_GetString(contextIoPtr->namePtr)
		: Tcl_GetString(contextIclsPtr->namePtr), "\"", NULL);
            return TCL_ERROR;
        }
        idmPtr = (ItclDelegatedFunction*)Tcl_GetHashValue(hPtr);
        if (!(idmPtr->flags & ITCL_TYPE_METHOD)) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                 "\"", cmdName, "\" isn't a delegated typemethod in ",
		contextIoPtr ? "object \"" : "class \"",
		contextIoPtr ?  Tcl_GetString(contextIoPtr->namePtr)
		: Tcl_GetString(contextIclsPtr->namePtr), "\"", NULL);
            return TCL_ERROR;
	}
        /*
         *  By default, return everything.
         */
        if (objc == 0) {
            ioptlist = DefInfoOption;
            objc = 5;
        } else {

            /*
             *  Otherwise, scan through all remaining flags and
             *  figure out what to return.
             */
            ioptlist = &ioptlistStorage[0];
            for (i=0 ; i < objc; i++) {
                result = Tcl_GetIndexFromObjStruct(interp, objv[i],
                    options, sizeof(char *), "option", 0, (int*)(&ioptlist[i]));
                if (result != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }

        if (objc > 1) {
            resultPtr = Tcl_NewListObj(0, NULL);
        }

        for (i=0 ; i < objc; i++) {
            switch (ioptlist[i]) {
                case BOptAsIdx:
                    if (idmPtr->asPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idmPtr->asPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptExceptionsIdx:
		    {
		    Tcl_Obj *entryObj;
		    objPtr = Tcl_NewListObj(0, NULL);
		    FOREACH_HASH_VALUE(entryObj, &idmPtr->exceptions) {
			Tcl_ListObjAppendElement(interp, objPtr, entryObj);
		    }
                    }
                    break;
                case BOptUsingIdx:
                    if (idmPtr->usingPtr) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idmPtr->usingPtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptComponentIdx:
                    if (idmPtr->icPtr != NULL) {
                        objPtr = Tcl_NewStringObj(
			        Tcl_GetString(idmPtr->icPtr->namePtr), -1);
                    } else {
                        objPtr = Tcl_NewStringObj("", -1);
                    }
                    break;

                case BOptNameIdx:
                    objPtr = Tcl_NewStringObj(
		            Tcl_GetString(idmPtr->namePtr), -1);
                    break;

            }

            if (objc == 1) {
                resultPtr = objPtr;
            } else {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    objPtr);
            }
        }
        Tcl_SetObjResult(interp, resultPtr);
    } else {

        /*
         *  Return the list of available options.
         */
        resultPtr = Tcl_NewListObj(0, NULL);
        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->delegatedFunctions, &place);
            while (hPtr) {
                idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
		if (idmPtr->flags & ITCL_TYPE_METHOD) {
                    objPtr = idmPtr->namePtr;
                    Tcl_ListObjAppendElement(NULL, resultPtr,
		            objPtr);
		}
                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}
