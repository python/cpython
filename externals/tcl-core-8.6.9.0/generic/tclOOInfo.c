/*
 * tclOODefineCmds.c --
 *
 *	This file contains the implementation of the ::oo-related [info]
 *	subcommands.
 *
 * Copyright (c) 2006-2011 by Donal K. Fellows
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "tclInt.h"
#include "tclOOInt.h"

static inline Class *	GetClassFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr);
static Tcl_ObjCmdProc InfoObjectCallCmd;
static Tcl_ObjCmdProc InfoObjectClassCmd;
static Tcl_ObjCmdProc InfoObjectDefnCmd;
static Tcl_ObjCmdProc InfoObjectFiltersCmd;
static Tcl_ObjCmdProc InfoObjectForwardCmd;
static Tcl_ObjCmdProc InfoObjectIsACmd;
static Tcl_ObjCmdProc InfoObjectMethodsCmd;
static Tcl_ObjCmdProc InfoObjectMethodTypeCmd;
static Tcl_ObjCmdProc InfoObjectMixinsCmd;
static Tcl_ObjCmdProc InfoObjectNsCmd;
static Tcl_ObjCmdProc InfoObjectVarsCmd;
static Tcl_ObjCmdProc InfoObjectVariablesCmd;
static Tcl_ObjCmdProc InfoClassCallCmd;
static Tcl_ObjCmdProc InfoClassConstrCmd;
static Tcl_ObjCmdProc InfoClassDefnCmd;
static Tcl_ObjCmdProc InfoClassDestrCmd;
static Tcl_ObjCmdProc InfoClassFiltersCmd;
static Tcl_ObjCmdProc InfoClassForwardCmd;
static Tcl_ObjCmdProc InfoClassInstancesCmd;
static Tcl_ObjCmdProc InfoClassMethodsCmd;
static Tcl_ObjCmdProc InfoClassMethodTypeCmd;
static Tcl_ObjCmdProc InfoClassMixinsCmd;
static Tcl_ObjCmdProc InfoClassSubsCmd;
static Tcl_ObjCmdProc InfoClassSupersCmd;
static Tcl_ObjCmdProc InfoClassVariablesCmd;

/*
 * List of commands that are used to implement the [info object] subcommands.
 */

static const EnsembleImplMap infoObjectCmds[] = {
    {"call",	   InfoObjectCallCmd,	    TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"class",	   InfoObjectClassCmd,	    TclCompileInfoObjectClassCmd, NULL, NULL, 0},
    {"definition", InfoObjectDefnCmd,	    TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"filters",	   InfoObjectFiltersCmd,    TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"forward",	   InfoObjectForwardCmd,    TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"isa",	   InfoObjectIsACmd,	    TclCompileInfoObjectIsACmd, NULL, NULL, 0},
    {"methods",	   InfoObjectMethodsCmd,    TclCompileBasicMin1ArgCmd, NULL, NULL, 0},
    {"methodtype", InfoObjectMethodTypeCmd, TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"mixins",	   InfoObjectMixinsCmd,	    TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"namespace",  InfoObjectNsCmd,	    TclCompileInfoObjectNamespaceCmd, NULL, NULL, 0},
    {"variables",  InfoObjectVariablesCmd,  TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"vars",	   InfoObjectVarsCmd,	    TclCompileBasic1Or2ArgCmd, NULL, NULL, 0},
    {NULL, NULL, NULL, NULL, NULL, 0}
};

/*
 * List of commands that are used to implement the [info class] subcommands.
 */

static const EnsembleImplMap infoClassCmds[] = {
    {"call",	     InfoClassCallCmd,		TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"constructor",  InfoClassConstrCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"definition",   InfoClassDefnCmd,		TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"destructor",   InfoClassDestrCmd,		TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"filters",	     InfoClassFiltersCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"forward",	     InfoClassForwardCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"instances",    InfoClassInstancesCmd,	TclCompileBasic1Or2ArgCmd, NULL, NULL, 0},
    {"methods",	     InfoClassMethodsCmd,	TclCompileBasicMin1ArgCmd, NULL, NULL, 0},
    {"methodtype",   InfoClassMethodTypeCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
    {"mixins",	     InfoClassMixinsCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"subclasses",   InfoClassSubsCmd,		TclCompileBasic1Or2ArgCmd, NULL, NULL, 0},
    {"superclasses", InfoClassSupersCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {"variables",    InfoClassVariablesCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
    {NULL, NULL, NULL, NULL, NULL, 0}
};

/*
 * ----------------------------------------------------------------------
 *
 * TclOOInitInfo --
 *
 *	Adjusts the Tcl core [info] command to contain subcommands ("object"
 *	and "class") for introspection of objects and classes.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOInitInfo(
    Tcl_Interp *interp)
{
    Tcl_Command infoCmd;
    Tcl_Obj *mapDict;

    /*
     * Build the ensembles used to implement [info object] and [info class].
     */

    TclMakeEnsemble(interp, "::oo::InfoObject", infoObjectCmds);
    TclMakeEnsemble(interp, "::oo::InfoClass", infoClassCmds);

    /*
     * Install into the master [info] ensemble.
     */

    infoCmd = Tcl_FindCommand(interp, "info", NULL, TCL_GLOBAL_ONLY);
    Tcl_GetEnsembleMappingDict(NULL, infoCmd, &mapDict);
    Tcl_DictObjPut(NULL, mapDict, Tcl_NewStringObj("object", -1),
	    Tcl_NewStringObj("::oo::InfoObject", -1));
    Tcl_DictObjPut(NULL, mapDict, Tcl_NewStringObj("class", -1),
	    Tcl_NewStringObj("::oo::InfoClass", -1));
    Tcl_SetEnsembleMappingDict(interp, infoCmd, mapDict);
}

/*
 * ----------------------------------------------------------------------
 *
 * GetClassFromObj --
 *
 *	How to correctly get a class from a Tcl_Obj. Just a wrapper round
 *	Tcl_GetObjectFromObj, but this is an idiom that was used heavily.
 *
 * ----------------------------------------------------------------------
 */

static inline Class *
GetClassFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    Object *oPtr = (Object *) Tcl_GetObjectFromObj(interp, objPtr);

    if (oPtr == NULL) {
	return NULL;
    }
    if (oPtr->classPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"\"%s\" is not a class", TclGetString(objPtr)));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CLASS",
		TclGetString(objPtr), NULL);
	return NULL;
    }
    return oPtr->classPtr;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectClassCmd --
 *
 *	Implements [info object class $objName ?$className?]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectClassCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;

    if (objc != 2 && objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName ?className?");
	return TCL_ERROR;
    }

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    if (objc == 2) {
	Tcl_SetObjResult(interp,
		TclOOObjectName(interp, oPtr->selfCls->thisPtr));
	return TCL_OK;
    } else {
	Class *mixinPtr, *o2clsPtr;
	int i;

	o2clsPtr = GetClassFromObj(interp, objv[2]);
	if (o2clsPtr == NULL) {
	    return TCL_ERROR;
	}

	FOREACH(mixinPtr, oPtr->mixins) {
	    if (!mixinPtr) {
		continue;
	    }
	    if (TclOOIsReachable(o2clsPtr, mixinPtr)) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
		return TCL_OK;
	    }
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(
		TclOOIsReachable(o2clsPtr, oPtr->selfCls)));
	return TCL_OK;
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectDefnCmd --
 *
 *	Implements [info object definition $objName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectDefnCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    Tcl_HashEntry *hPtr;
    Proc *procPtr;
    CompiledLocal *localPtr;
    Tcl_Obj *resultObjs[2];

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName methodName");
	return TCL_ERROR;
    }

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    if (!oPtr->methodsPtr) {
	goto unknownMethod;
    }
    hPtr = Tcl_FindHashEntry(oPtr->methodsPtr, (char *) objv[2]);
    if (hPtr == NULL) {
    unknownMethod:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown method \"%s\"", TclGetString(objv[2])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }
    procPtr = TclOOGetProcFromMethod(Tcl_GetHashValue(hPtr));
    if (procPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"definition not available for this kind of method", -1));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }

    resultObjs[0] = Tcl_NewObj();
    for (localPtr=procPtr->firstLocalPtr; localPtr!=NULL;
	    localPtr=localPtr->nextPtr) {
	if (TclIsVarArgument(localPtr)) {
	    Tcl_Obj *argObj;

	    argObj = Tcl_NewObj();
	    Tcl_ListObjAppendElement(NULL, argObj,
		    Tcl_NewStringObj(localPtr->name, -1));
	    if (localPtr->defValuePtr != NULL) {
		Tcl_ListObjAppendElement(NULL, argObj, localPtr->defValuePtr);
	    }
	    Tcl_ListObjAppendElement(NULL, resultObjs[0], argObj);
	}
    }
    resultObjs[1] = TclOOGetMethodBody(Tcl_GetHashValue(hPtr));
    Tcl_SetObjResult(interp, Tcl_NewListObj(2, resultObjs));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectFiltersCmd --
 *
 *	Implements [info object filters $objName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectFiltersCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    int i;
    Tcl_Obj *filterObj, *resultObj;
    Object *oPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName");
	return TCL_ERROR;
    }

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    resultObj = Tcl_NewObj();

    FOREACH(filterObj, oPtr->filters) {
	Tcl_ListObjAppendElement(NULL, resultObj, filterObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectForwardCmd --
 *
 *	Implements [info object forward $objName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectForwardCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    Tcl_HashEntry *hPtr;
    Tcl_Obj *prefixObj;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName methodName");
	return TCL_ERROR;
    }

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    if (!oPtr->methodsPtr) {
	goto unknownMethod;
    }
    hPtr = Tcl_FindHashEntry(oPtr->methodsPtr, (char *) objv[2]);
    if (hPtr == NULL) {
    unknownMethod:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown method \"%s\"", TclGetString(objv[2])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }
    prefixObj = TclOOGetFwdFromMethod(Tcl_GetHashValue(hPtr));
    if (prefixObj == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"prefix argument list not available for this kind of method",
		-1));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, prefixObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectIsACmd --
 *
 *	Implements [info object isa $category $objName ...]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectIsACmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    static const char *const categories[] = {
	"class", "metaclass", "mixin", "object", "typeof", NULL
    };
    enum IsACats {
	IsClass, IsMetaclass, IsMixin, IsObject, IsType
    };
    Object *oPtr, *o2Ptr;
    int idx, i, result = 0;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "category objName ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], categories, "category", 0,
	    &idx) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Now we know what test we are doing, we can check we've got the right
     * number of arguments.
     */

    switch ((enum IsACats) idx) {
    case IsObject:
    case IsClass:
    case IsMetaclass:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "objName");
	    return TCL_ERROR;
	}
	break;
    case IsMixin:
    case IsType:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "objName className");
	    return TCL_ERROR;
	}
	break;
    }

    /*
     * Perform the check. Note that we can guarantee that we will not fail
     * from here on; "failures" result in a false-TCL_OK result.
     */

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[2]);
    if (oPtr == NULL) {
	goto failPrecondition;
    }

    switch ((enum IsACats) idx) {
    case IsObject:
	result = 1;
	break;
    case IsClass:
	result = (oPtr->classPtr != NULL);
	break;
    case IsMetaclass:
	if (oPtr->classPtr != NULL) {
	    result = TclOOIsReachable(TclOOGetFoundation(interp)->classCls,
		    oPtr->classPtr);
	}
	break;
    case IsMixin:
	o2Ptr = (Object *) Tcl_GetObjectFromObj(interp, objv[3]);
	if (o2Ptr == NULL) {
	    goto failPrecondition;
	}
	if (o2Ptr->classPtr != NULL) {
	    Class *mixinPtr;

	    FOREACH(mixinPtr, oPtr->mixins) {
		if (!mixinPtr) {
		    continue;
		}
		if (TclOOIsReachable(o2Ptr->classPtr, mixinPtr)) {
		    result = 1;
		    break;
		}
	    }
	}
	break;
    case IsType:
	o2Ptr = (Object *) Tcl_GetObjectFromObj(interp, objv[3]);
	if (o2Ptr == NULL) {
	    goto failPrecondition;
	}
	if (o2Ptr->classPtr != NULL) {
	    result = TclOOIsReachable(o2Ptr->classPtr, oPtr->selfCls);
	}
	break;
    }
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
    return TCL_OK;

  failPrecondition:
    Tcl_ResetResult(interp);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(0));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectMethodsCmd --
 *
 *	Implements [info object methods $objName ?$option ...?]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectMethodsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    int flag = PUBLIC_METHOD, recurse = 0;
    FOREACH_HASH_DECLS;
    Tcl_Obj *namePtr, *resultObj;
    Method *mPtr;
    static const char *const options[] = {
	"-all", "-localprivate", "-private", NULL
    };
    enum Options {
	OPT_ALL, OPT_LOCALPRIVATE, OPT_PRIVATE
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName ?-option value ...?");
	return TCL_ERROR;
    }
    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (objc != 2) {
	int i, idx;

	for (i=2 ; i<objc ; i++) {
	    if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0,
		    &idx) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum Options) idx) {
	    case OPT_ALL:
		recurse = 1;
		break;
	    case OPT_LOCALPRIVATE:
		flag = PRIVATE_METHOD;
		break;
	    case OPT_PRIVATE:
		flag = 0;
		break;
	    }
	}
    }

    resultObj = Tcl_NewObj();
    if (recurse) {
	const char **names;
	int i, numNames = TclOOGetSortedMethodList(oPtr, flag, &names);

	for (i=0 ; i<numNames ; i++) {
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(names[i], -1));
	}
	if (numNames > 0) {
	    ckfree(names);
	}
    } else if (oPtr->methodsPtr) {
	FOREACH_HASH(namePtr, mPtr, oPtr->methodsPtr) {
	    if (mPtr->typePtr != NULL && (mPtr->flags & flag) == flag) {
		Tcl_ListObjAppendElement(NULL, resultObj, namePtr);
	    }
	}
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectMethodTypeCmd --
 *
 *	Implements [info object methodtype $objName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectMethodTypeCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    Tcl_HashEntry *hPtr;
    Method *mPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName methodName");
	return TCL_ERROR;
    }

    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    if (!oPtr->methodsPtr) {
	goto unknownMethod;
    }
    hPtr = Tcl_FindHashEntry(oPtr->methodsPtr, (char *) objv[2]);
    if (hPtr == NULL) {
    unknownMethod:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown method \"%s\"", TclGetString(objv[2])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }
    mPtr = Tcl_GetHashValue(hPtr);
    if (mPtr->typePtr == NULL) {
	/*
	 * Special entry for visibility control: pretend the method doesnt
	 * exist.
	 */

	goto unknownMethod;
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(mPtr->typePtr->name, -1));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectMixinsCmd --
 *
 *	Implements [info object mixins $objName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectMixinsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Class *mixinPtr;
    Object *oPtr;
    Tcl_Obj *resultObj;
    int i;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName");
	return TCL_ERROR;
    }
    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(mixinPtr, oPtr->mixins) {
	if (!mixinPtr) {
	    continue;
	}
	Tcl_ListObjAppendElement(NULL, resultObj,
		TclOOObjectName(interp, mixinPtr->thisPtr));
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectNsCmd --
 *
 *	Implements [info object namespace $objName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectNsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName");
	return TCL_ERROR;
    }
    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp,
	    Tcl_NewStringObj(oPtr->namespacePtr->fullName, -1));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectVariablesCmd --
 *
 *	Implements [info object variables $objName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectVariablesCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    Tcl_Obj *variableObj, *resultObj;
    int i;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName");
	return TCL_ERROR;
    }
    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(variableObj, oPtr->variables) {
	Tcl_ListObjAppendElement(NULL, resultObj, variableObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectVarsCmd --
 *
 *	Implements [info object vars $objName ?$pattern?]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectVarsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    const char *pattern = NULL;
    FOREACH_HASH_DECLS;
    VarInHash *vihPtr;
    Tcl_Obj *nameObj, *resultObj;

    if (objc != 2 && objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName ?pattern?");
	return TCL_ERROR;
    }
    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	pattern = TclGetString(objv[2]);
    }
    resultObj = Tcl_NewObj();

    /*
     * Extract the information we need from the object's namespace's table of
     * variables. Note that this involves horrific knowledge of the guts of
     * tclVar.c, so we can't leverage our hash-iteration macros properly.
     */

    FOREACH_HASH_VALUE(vihPtr,
	    &((Namespace *) oPtr->namespacePtr)->varTable.table) {
	nameObj = vihPtr->entry.key.objPtr;

	if (TclIsVarUndefined(&vihPtr->var)
		|| !TclIsVarNamespaceVar(&vihPtr->var)) {
	    continue;
	}
	if (pattern != NULL
		&& !Tcl_StringMatch(TclGetString(nameObj), pattern)) {
	    continue;
	}
	Tcl_ListObjAppendElement(NULL, resultObj, nameObj);
    }

    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassConstrCmd --
 *
 *	Implements [info class constructor $clsName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassConstrCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Proc *procPtr;
    CompiledLocal *localPtr;
    Tcl_Obj *resultObjs[2];
    Class *clsPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }
    if (clsPtr->constructorPtr == NULL) {
	return TCL_OK;
    }
    procPtr = TclOOGetProcFromMethod(clsPtr->constructorPtr);
    if (procPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"definition not available for this kind of method", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "METHOD_TYPE", NULL);
	return TCL_ERROR;
    }

    resultObjs[0] = Tcl_NewObj();
    for (localPtr=procPtr->firstLocalPtr; localPtr!=NULL;
	    localPtr=localPtr->nextPtr) {
	if (TclIsVarArgument(localPtr)) {
	    Tcl_Obj *argObj;

	    argObj = Tcl_NewObj();
	    Tcl_ListObjAppendElement(NULL, argObj,
		    Tcl_NewStringObj(localPtr->name, -1));
	    if (localPtr->defValuePtr != NULL) {
		Tcl_ListObjAppendElement(NULL, argObj, localPtr->defValuePtr);
	    }
	    Tcl_ListObjAppendElement(NULL, resultObjs[0], argObj);
	}
    }
    resultObjs[1] = TclOOGetMethodBody(clsPtr->constructorPtr);
    Tcl_SetObjResult(interp, Tcl_NewListObj(2, resultObjs));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassDefnCmd --
 *
 *	Implements [info class definition $clsName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassDefnCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_HashEntry *hPtr;
    Proc *procPtr;
    CompiledLocal *localPtr;
    Tcl_Obj *resultObjs[2];
    Class *clsPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "className methodName");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&clsPtr->classMethods, (char *) objv[2]);
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown method \"%s\"", TclGetString(objv[2])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }
    procPtr = TclOOGetProcFromMethod(Tcl_GetHashValue(hPtr));
    if (procPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"definition not available for this kind of method", -1));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }

    resultObjs[0] = Tcl_NewObj();
    for (localPtr=procPtr->firstLocalPtr; localPtr!=NULL;
	    localPtr=localPtr->nextPtr) {
	if (TclIsVarArgument(localPtr)) {
	    Tcl_Obj *argObj;

	    argObj = Tcl_NewObj();
	    Tcl_ListObjAppendElement(NULL, argObj,
		    Tcl_NewStringObj(localPtr->name, -1));
	    if (localPtr->defValuePtr != NULL) {
		Tcl_ListObjAppendElement(NULL, argObj, localPtr->defValuePtr);
	    }
	    Tcl_ListObjAppendElement(NULL, resultObjs[0], argObj);
	}
    }
    resultObjs[1] = TclOOGetMethodBody(Tcl_GetHashValue(hPtr));
    Tcl_SetObjResult(interp, Tcl_NewListObj(2, resultObjs));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassDestrCmd --
 *
 *	Implements [info class destructor $clsName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassDestrCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Proc *procPtr;
    Class *clsPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }

    if (clsPtr->destructorPtr == NULL) {
	return TCL_OK;
    }
    procPtr = TclOOGetProcFromMethod(clsPtr->destructorPtr);
    if (procPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"definition not available for this kind of method", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "METHOD_TYPE", NULL);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, TclOOGetMethodBody(clsPtr->destructorPtr));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassFiltersCmd --
 *
 *	Implements [info class filters $clsName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassFiltersCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    int i;
    Tcl_Obj *filterObj, *resultObj;
    Class *clsPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(filterObj, clsPtr->filters) {
	Tcl_ListObjAppendElement(NULL, resultObj, filterObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassForwardCmd --
 *
 *	Implements [info class forward $clsName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassForwardCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_HashEntry *hPtr;
    Tcl_Obj *prefixObj;
    Class *clsPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "className methodName");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&clsPtr->classMethods, (char *) objv[2]);
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown method \"%s\"", TclGetString(objv[2])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }
    prefixObj = TclOOGetFwdFromMethod(Tcl_GetHashValue(hPtr));
    if (prefixObj == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"prefix argument list not available for this kind of method",
		-1));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, prefixObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassInstancesCmd --
 *
 *	Implements [info class instances $clsName ?$pattern?]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassInstancesCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    Class *clsPtr;
    int i;
    const char *pattern = NULL;
    Tcl_Obj *resultObj;

    if (objc != 2 && objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "className ?pattern?");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	pattern = TclGetString(objv[2]);
    }

    resultObj = Tcl_NewObj();
    FOREACH(oPtr, clsPtr->instances) {
	Tcl_Obj *tmpObj = TclOOObjectName(interp, oPtr);

	if (pattern && !Tcl_StringMatch(TclGetString(tmpObj), pattern)) {
	    continue;
	}
	Tcl_ListObjAppendElement(NULL, resultObj, tmpObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassMethodsCmd --
 *
 *	Implements [info class methods $clsName ?-private?]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassMethodsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    int flag = PUBLIC_METHOD, recurse = 0;
    Tcl_Obj *namePtr, *resultObj;
    Method *mPtr;
    Class *clsPtr;
    static const char *const options[] = {
	"-all", "-localprivate", "-private", NULL
    };
    enum Options {
	OPT_ALL, OPT_LOCALPRIVATE, OPT_PRIVATE
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className ?-option value ...?");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }
    if (objc != 2) {
	int i, idx;

	for (i=2 ; i<objc ; i++) {
	    if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0,
		    &idx) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum Options) idx) {
	    case OPT_ALL:
		recurse = 1;
		break;
	    case OPT_LOCALPRIVATE:
		flag = PRIVATE_METHOD;
		break;
	    case OPT_PRIVATE:
		flag = 0;
		break;
	    }
	}
    }

    resultObj = Tcl_NewObj();
    if (recurse) {
	const char **names;
	int i, numNames = TclOOGetSortedClassMethodList(clsPtr, flag, &names);

	for (i=0 ; i<numNames ; i++) {
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(names[i], -1));
	}
	if (numNames > 0) {
	    ckfree(names);
	}
    } else {
	FOREACH_HASH_DECLS;

	FOREACH_HASH(namePtr, mPtr, &clsPtr->classMethods) {
	    if (mPtr->typePtr != NULL && (mPtr->flags & flag) == flag) {
		Tcl_ListObjAppendElement(NULL, resultObj, namePtr);
	    }
	}
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassMethodTypeCmd --
 *
 *	Implements [info class methodtype $clsName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassMethodTypeCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_HashEntry *hPtr;
    Method *mPtr;
    Class *clsPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "className methodName");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }

    hPtr = Tcl_FindHashEntry(&clsPtr->classMethods, (char *) objv[2]);
    if (hPtr == NULL) {
    unknownMethod:
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown method \"%s\"", TclGetString(objv[2])));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		TclGetString(objv[2]), NULL);
	return TCL_ERROR;
    }
    mPtr = Tcl_GetHashValue(hPtr);
    if (mPtr->typePtr == NULL) {
	/*
	 * Special entry for visibility control: pretend the method doesnt
	 * exist.
	 */

	goto unknownMethod;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(mPtr->typePtr->name, -1));
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassMixinsCmd --
 *
 *	Implements [info class mixins $clsName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassMixinsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Class *clsPtr, *mixinPtr;
    Tcl_Obj *resultObj;
    int i;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(mixinPtr, clsPtr->mixins) {
	if (!mixinPtr) {
	    continue;
	}
	Tcl_ListObjAppendElement(NULL, resultObj,
		TclOOObjectName(interp, mixinPtr->thisPtr));
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassSubsCmd --
 *
 *	Implements [info class subclasses $clsName ?$pattern?]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassSubsCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Class *clsPtr, *subclassPtr;
    Tcl_Obj *resultObj;
    int i;
    const char *pattern = NULL;

    if (objc != 2 && objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "className ?pattern?");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	pattern = TclGetString(objv[2]);
    }

    resultObj = Tcl_NewObj();
    FOREACH(subclassPtr, clsPtr->subclasses) {
	Tcl_Obj *tmpObj = TclOOObjectName(interp, subclassPtr->thisPtr);

	if (pattern && !Tcl_StringMatch(TclGetString(tmpObj), pattern)) {
	    continue;
	}
	Tcl_ListObjAppendElement(NULL, resultObj, tmpObj);
    }
    FOREACH(subclassPtr, clsPtr->mixinSubs) {
	Tcl_Obj *tmpObj = TclOOObjectName(interp, subclassPtr->thisPtr);

	if (pattern && !Tcl_StringMatch(TclGetString(tmpObj), pattern)) {
	    continue;
	}
	Tcl_ListObjAppendElement(NULL, resultObj, tmpObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassSupersCmd --
 *
 *	Implements [info class superclasses $clsName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassSupersCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Class *clsPtr, *superPtr;
    Tcl_Obj *resultObj;
    int i;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(superPtr, clsPtr->superclasses) {
	Tcl_ListObjAppendElement(NULL, resultObj,
		TclOOObjectName(interp, superPtr->thisPtr));
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassVariablesCmd --
 *
 *	Implements [info class variables $clsName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassVariablesCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Class *clsPtr;
    Tcl_Obj *variableObj, *resultObj;
    int i;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "className");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }

    resultObj = Tcl_NewObj();
    FOREACH(variableObj, clsPtr->variables) {
	Tcl_ListObjAppendElement(NULL, resultObj, variableObj);
    }
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoObjectCallCmd --
 *
 *	Implements [info object call $objName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoObjectCallCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Object *oPtr;
    CallContext *contextPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "objName methodName");
	return TCL_ERROR;
    }
    oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[1]);
    if (oPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Get the call context and render its call chain.
     */

    contextPtr = TclOOGetCallContext(oPtr, objv[2], PUBLIC_METHOD, NULL);
    if (contextPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"cannot construct any call chain", -1));
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
	    TclOORenderCallChain(interp, contextPtr->callPtr));
    TclOODeleteContext(contextPtr);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * InfoClassCallCmd --
 *
 *	Implements [info class call $clsName $methodName]
 *
 * ----------------------------------------------------------------------
 */

static int
InfoClassCallCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Class *clsPtr;
    CallChain *callPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "className methodName");
	return TCL_ERROR;
    }
    clsPtr = GetClassFromObj(interp, objv[1]);
    if (clsPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Get an render the stereotypical call chain.
     */

    callPtr = TclOOGetStereotypeCallChain(clsPtr, objv[2], PUBLIC_METHOD);
    if (callPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"cannot construct any call chain", -1));
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, TclOORenderCallChain(interp, callPtr));
    TclOODeleteChain(callPtr);
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
