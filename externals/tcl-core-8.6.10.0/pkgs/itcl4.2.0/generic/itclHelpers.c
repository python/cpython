/*
 * itclHelpers.c --
 *
 * This file contains the C-implemeted part of
 * Itcl
 *
 * Copyright (c) 2007 by Arnulf P. Wiedemann
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "itclInt.h"

void ItclDeleteArgList(ItclArgList *arglistPtr);
#ifdef ITCL_DEBUG
int _itcl_debug_level = 0;

/*
 * ------------------------------------------------------------------------
 *  ItclShowArgs()
 * ------------------------------------------------------------------------
 */

void
ItclShowArgs(
    int level,
    const char *str,
    int objc,
    Tcl_Obj * const* objv)
{
    int i;

    if (level > _itcl_debug_level) {
        return;
    }
    fprintf(stderr, "%s", str);
    for (i = 0; i < objc; i++) {
        fprintf(stderr, "!%s", objv[i] == NULL ? "??" :
                Tcl_GetString(objv[i]));
    }
    fprintf(stderr, "!\n");
}
#endif

/*
 * ------------------------------------------------------------------------
 *  Itcl_ProtectionStr()
 *
 *  Converts an integer protection code (ITCL_PUBLIC, ITCL_PROTECTED,
 *  or ITCL_PRIVATE) into a human-readable character string.  Returns
 *  a pointer to this string.
 * ------------------------------------------------------------------------
 */
const char*
Itcl_ProtectionStr(
    int pLevel)     /* protection level */
{
    switch (pLevel) {
    case ITCL_PUBLIC:
        return "public";
    case ITCL_PROTECTED:
        return "protected";
    case ITCL_PRIVATE:
        return "private";
    }
    return "<bad-protection-code>";
}

/*
 * ------------------------------------------------------------------------
 *  ItclCreateArgList()
 * ------------------------------------------------------------------------
 */

int
ItclCreateArgList(
    Tcl_Interp *interp,		/* interpreter managing this function */
    const char *str,		/* string representing argument list */
    int *argcPtr,		/* number of mandatory arguments */
    int *maxArgcPtr,		/* number of arguments parsed */
    Tcl_Obj **usagePtr,         /* store usage message for arguments here */
    ItclArgList **arglistPtrPtr,
    				/* returns pointer to parsed argument list */
    ItclMemberFunc *mPtr,
    const char *commandName)
{
    int argc;
    int defaultArgc;
    const char **argv;
    const char **defaultArgv;
    ItclArgList *arglistPtr;
    ItclArgList *lastArglistPtr;
    int i;
    int hadArgsArgument;
    int result;

    *arglistPtrPtr = NULL;
    lastArglistPtr = NULL;
    argc = 0;
    hadArgsArgument = 0;
    result = TCL_OK;
    *maxArgcPtr = 0;
    *argcPtr = 0;
    *usagePtr = Tcl_NewStringObj("", -1);
    if (str) {
        if (Tcl_SplitList(interp, (const char *)str, &argc, &argv)
	        != TCL_OK) {
	    return TCL_ERROR;
	}
	i = 0;
	if (argc == 0) {
	   /* signal there are 0 arguments */
            arglistPtr = (ItclArgList *)ckalloc(sizeof(ItclArgList));
	    memset(arglistPtr, 0, sizeof(ItclArgList));
	    *arglistPtrPtr = arglistPtr;
	}
        while (i < argc) {
            if (Tcl_SplitList(interp, argv[i], &defaultArgc, &defaultArgv)
	            != TCL_OK) {
	        result = TCL_ERROR;
	        break;
	    }
	    arglistPtr = NULL;
	    if (defaultArgc == 0 || defaultArgv[0][0] == '\0') {
		if (commandName != NULL) {
	            Tcl_AppendResult(interp, "procedure \"",
		            commandName,
			    "\" has argument with no name", NULL);
		} else {
		    char buf[TCL_INTEGER_SPACE];
		    sprintf(buf, "%d", i);
		    Tcl_AppendResult(interp, "argument #", buf,
		            " has no name", NULL);
		}
		ckfree((char *) defaultArgv);
	        result = TCL_ERROR;
	        break;
	    }
	    if (defaultArgc > 2) {
	        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	            "too many fields in argument specifier \"",
		    argv[i], "\"",
		    NULL);
		ckfree((char *) defaultArgv);
	        result = TCL_ERROR;
	        break;
	    }
	    if (strstr(defaultArgv[0],"::")) {
	        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		        "bad argument name \"", defaultArgv[0], "\"",
			NULL);
		ckfree((char *) defaultArgv);
		result = TCL_ERROR;
		break;
	    }
            arglistPtr = (ItclArgList *)ckalloc(sizeof(ItclArgList));
	    memset(arglistPtr, 0, sizeof(ItclArgList));
            if (*arglistPtrPtr == NULL) {
	         *arglistPtrPtr = arglistPtr;
	    } else {
	        lastArglistPtr->nextPtr = arglistPtr;
	        Tcl_AppendToObj(*usagePtr, " ", 1);
	    }
	    arglistPtr->namePtr =
	            Tcl_NewStringObj(defaultArgv[0], -1);
	    Tcl_IncrRefCount(arglistPtr->namePtr);
	    (*maxArgcPtr)++;
	    if (defaultArgc == 1) {
		(*argcPtr)++;
	        arglistPtr->defaultValuePtr = NULL;
		if ((strcmp(defaultArgv[0], "args") == 0) && (i == argc-1)) {
		    hadArgsArgument = 1;
		    (*argcPtr)--;
	            Tcl_AppendToObj(*usagePtr, "?arg arg ...?", -1);
		} else {
	            Tcl_AppendToObj(*usagePtr, defaultArgv[0], -1);
	        }
	    } else {
	        arglistPtr->defaultValuePtr =
		        Tcl_NewStringObj(defaultArgv[1], -1);
		Tcl_IncrRefCount(arglistPtr->defaultValuePtr);
	        Tcl_AppendToObj(*usagePtr, "?", 1);
	        Tcl_AppendToObj(*usagePtr, defaultArgv[0], -1);
	        Tcl_AppendToObj(*usagePtr, "?", 1);
	    }
            lastArglistPtr = arglistPtr;
	    i++;
	    ckfree((char *) defaultArgv);
        }
	ckfree((char *) argv);
    }
    /*
     *  If anything went wrong, destroy whatever arguments were
     *  created and return an error.
     */
    if (result != TCL_OK) {
        ItclDeleteArgList(*arglistPtrPtr);
        *arglistPtrPtr = NULL;
    }
    if (hadArgsArgument) {
        *maxArgcPtr = -1;
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteArgList()
 * ------------------------------------------------------------------------
 */

void
ItclDeleteArgList(
    ItclArgList *arglistPtr)	/* first argument in arg list chain */
{
    ItclArgList *currPtr;
    ItclArgList *nextPtr;

    for (currPtr=arglistPtr; currPtr; currPtr=nextPtr) {
	if (currPtr->defaultValuePtr != NULL) {
	    Tcl_DecrRefCount(currPtr->defaultValuePtr);
	}
	if (currPtr->namePtr != NULL) {
	    Tcl_DecrRefCount(currPtr->namePtr);
	}
        nextPtr = currPtr->nextPtr;
        ckfree((char *)currPtr);
    }
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_EvalArgs()
 *
 *  This procedure invokes a list of (objc,objv) arguments as a
 *  single command.  It is similar to Tcl_EvalObj, but it doesn't
 *  do any parsing or compilation.  It simply treats the first
 *  argument as a command and invokes that command in the current
 *  context.
 *
 *  Returns TCL_OK if successful.  Otherwise, this procedure returns
 *  TCL_ERROR along with an error message in the interpreter.
 * ------------------------------------------------------------------------
 */
int
Itcl_EvalArgs(
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Command cmd;
    Tcl_CmdInfo infoPtr;

    /*
     * Resolve the command by converting it to a CmdName object.
     * This caches a pointer to the Command structure for the
     * command, so if we need it again, it's ready to use.
     */
    cmd = Tcl_GetCommandFromObj(interp, objv[0]);

    /*
     * If the command is not found, we have no hope of a truly fast
     * dispatch, so the smart thing to do is just fall back to the
     * conventional tools.
     */
    if (cmd == NULL) {
	return Tcl_EvalObjv(interp, objc, objv, 0);
    }

    /*
     *  Finally, invoke the command's Tcl_ObjCmdProc.  Be careful
     *  to pass in the proper client data.
     */
    Tcl_GetCommandInfoFromToken(cmd, &infoPtr);
    return (infoPtr.objProc)(infoPtr.objClientData, interp, objc, objv);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_CreateArgs()
 *
 *  This procedure takes a string and a list of (objc,objv) arguments,
 *  and glues them together in a single list.  This is useful when
 *  a command word needs to be prepended or substituted into a command
 *  line before it is executed.  The arguments are returned in a single
 *  list object, and they can be retrieved by calling
 *  Tcl_ListObjGetElements.  When the arguments are no longer needed,
 *  they should be discarded by decrementing the reference count for
 *  the list object.
 *
 *  Returns a pointer to the list object containing the arguments.
 * ------------------------------------------------------------------------
 */
Tcl_Obj*
Itcl_CreateArgs(
    Tcl_Interp *interp,      /* current interpreter */
    const char *string,      /* first command word */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int i;
    Tcl_Obj *listPtr;

    ItclShowArgs(1, "Itcl_CreateArgs", objc, objv);
    listPtr = Tcl_NewListObj(objc+2, NULL);
    Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewStringObj("my", -1));
    Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewStringObj(string, -1));

    for (i=0; i < objc; i++) {
        Tcl_ListObjAppendElement(NULL, listPtr, objv[i]);
    }
    return listPtr;
}

/*
 * ------------------------------------------------------------------------
 *  ItclEnsembleSubCmd()
 * ------------------------------------------------------------------------
 */

int
ItclEnsembleSubCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    const char *ensembleName,
    int objc,
    Tcl_Obj *const *objv,
    const char *functionName)
{
    int result;
    Tcl_Obj **newObjv;
    int isRootEnsemble;
    ItclShowArgs(2, functionName, objc, objv);

    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc));
    isRootEnsemble = Itcl_InitRewriteEnsemble(interp, 1, 1, objc, objv);
    newObjv[0] = Tcl_NewStringObj("::itcl::builtin::Info", -1);
    Tcl_IncrRefCount(newObjv[0]);
    if (objc > 1) {
        memcpy(newObjv+1, objv+1, sizeof(Tcl_Obj *) * (objc-1));
    }
    result = Tcl_EvalObjv(interp, objc, newObjv, TCL_EVAL_INVOKE);
    Tcl_DecrRefCount(newObjv[0]);
    ckfree((char *)newObjv);
    Itcl_ResetRewriteEnsemble(interp, isRootEnsemble);
    return result;
}


/*
 * ------------------------------------------------------------------------
 *  ItclCapitalize()
 * ------------------------------------------------------------------------
 */

Tcl_Obj *
ItclCapitalize(
    const char *str)
{
    Tcl_Obj *objPtr;
    char buf[2];

    sprintf(buf, "%c", toupper(UCHAR(*str)));
    buf[1] = '\0';
    objPtr = Tcl_NewStringObj(buf, -1);
    Tcl_AppendToObj(objPtr, str+1, -1);
    return objPtr;
}
/*
 * ------------------------------------------------------------------------
 *  DeleteClassDictInfo()
 * ------------------------------------------------------------------------
 */
static int
DeleteClassDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    const char *varName)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;

    dictPtr = Tcl_GetVar2Ex(interp, varName, NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", varName, NULL);
	return TCL_ERROR;
    }
    keyPtr = iclsPtr->fullNamePtr;
    if (Tcl_DictObjRemove(interp, dictPtr, keyPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetVar2Ex(interp, varName, NULL, dictPtr, 0);
    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  AddDictEntry()
 * ------------------------------------------------------------------------
 */
static int
AddDictEntry(
    Tcl_Interp *interp,
    Tcl_Obj *dictPtr,
    const char *keyStr,
    Tcl_Obj *valuePtr)
{
    Tcl_Obj *keyPtr;
    int code;

    if (valuePtr == NULL) {
        return TCL_OK;
    }
    keyPtr = Tcl_NewStringObj(keyStr, -1);
    Tcl_IncrRefCount(keyPtr);
    code = Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr);
    Tcl_DecrRefCount(keyPtr);
    return code;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddClassesDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddClassesDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *keyPtr1;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    Tcl_Obj *listPtr;
    FOREACH_HASH_DECLS;
    ItclHierIter hier;
    ItclClass *iclsPtr2;
    void *value;
    int found;
    int newValue1;
    int haveHierarchy;

    found = 0;
    FOREACH_HASH(keyPtr1, value, &iclsPtr->infoPtr->classTypes) {
        if (iclsPtr->flags & PTR2INT(value)) {
	    found = 1;
	    break;
	}
    }
    if (! found) {
	Tcl_AppendResult(interp, "ItclAddClassesDictInfo bad class ",
	        "type for class \"", Tcl_GetString(iclsPtr->fullNamePtr),
	        "\"", NULL);
        return TCL_ERROR;
    }
    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classes", NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classes", NULL);
	return TCL_ERROR;
    }
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr1, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        newValue1 = 1;
        valuePtr1 = Tcl_NewDictObj();
    }
    keyPtr = iclsPtr->fullNamePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 != NULL) {
        if (Tcl_DictObjRemove(interp, valuePtr1, keyPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    valuePtr2 = Tcl_NewDictObj();
    if (AddDictEntry(interp, valuePtr2, "-name", iclsPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (AddDictEntry(interp, valuePtr2, "-fullname", iclsPtr->fullNamePtr)
            != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_InitHierIter(&hier, iclsPtr);
    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    haveHierarchy = 0;
    listPtr = Tcl_NewListObj(0, NULL);
    while (iclsPtr2 != NULL) {
        haveHierarchy = 1;
	if (Tcl_ListObjAppendElement(interp, listPtr, iclsPtr2->fullNamePtr)
	        != TCL_OK) {
	    return TCL_ERROR;
	}
        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    }
    Itcl_DeleteHierIter(&hier);
    if (haveHierarchy) {
        if (AddDictEntry(interp, valuePtr2, "-heritage", listPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        Tcl_DecrRefCount(listPtr);
    }
    if (iclsPtr->widgetClassPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-widget", iclsPtr->widgetClassPtr)
	        != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (iclsPtr->hullTypePtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-hulltype", iclsPtr->hullTypePtr)
	        != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (iclsPtr->typeConstructorPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-typeconstructor",
	        iclsPtr->typeConstructorPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    keyPtr = iclsPtr->fullNamePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
        if (Tcl_DictObjPut(interp, dictPtr, keyPtr1, valuePtr1) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetVar2Ex(interp, ITCL_NAMESPACE"::internal::dicts::classes",
            NULL, dictPtr, 0);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteClassesDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclDeleteClassesDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *valuePtr;
    FOREACH_HASH_DECLS;
    void* value;
    int found;

    found = 0;
    FOREACH_HASH(keyPtr, value, &iclsPtr->infoPtr->classTypes) {
        if (iclsPtr->flags & PTR2INT(value)) {
	    found = 1;
	    break;
	}
    }
    if (! found) {
	Tcl_AppendResult(interp, "ItclDeleteClassesDictInfo bad class ",
	        "type for class \"", Tcl_GetString(iclsPtr->fullNamePtr),
	        "\"", NULL);
        return TCL_ERROR;
    }
    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classes", NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classes", NULL);
	return TCL_ERROR;
    }
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr, &valuePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr == NULL) {
        /* there seems to have been an error during construction
	 * and no class has been created so ignore silently */
        return TCL_OK;
    }
    if (Tcl_DictObjRemove(interp, valuePtr, iclsPtr->fullNamePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetVar2Ex(interp, ITCL_NAMESPACE"::internal::dicts::classes",
            NULL, dictPtr, 0);
    DeleteClassDictInfo(interp, iclsPtr,
            ITCL_NAMESPACE"::internal::dicts::classOptions");
    DeleteClassDictInfo(interp, iclsPtr,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedOptions");
    DeleteClassDictInfo(interp, iclsPtr,
            ITCL_NAMESPACE"::internal::dicts::classVariables");
    DeleteClassDictInfo(interp, iclsPtr,
            ITCL_NAMESPACE"::internal::dicts::classComponents");
    DeleteClassDictInfo(interp, iclsPtr,
            ITCL_NAMESPACE"::internal::dicts::classFunctions");
    DeleteClassDictInfo(interp, iclsPtr,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedFunctions");
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddObjectsDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddObjectsDictInfo(
    Tcl_Interp *interp,
    ItclObject *ioPtr)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *keyPtr1;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    Tcl_Obj *objPtr;
    int newValue1;

    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::objects", NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::objects", NULL);
	return TCL_ERROR;
    }
    keyPtr1 = Tcl_NewStringObj("instances", -1);
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr1, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        newValue1 = 1;
        valuePtr1 = Tcl_NewDictObj();
    }
    keyPtr = ioPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 == NULL) {
        if (Tcl_DictObjRemove(interp, valuePtr1, keyPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    valuePtr2 = Tcl_NewDictObj();
    if (AddDictEntry(interp, valuePtr2, "-name", ioPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (AddDictEntry(interp, valuePtr2, "-origname", ioPtr->namePtr)
            != TCL_OK) {
        return TCL_ERROR;
    }
    if (AddDictEntry(interp, valuePtr2, "-class", ioPtr->iclsPtr->fullNamePtr)
            != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioPtr->hullWindowNamePtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-hullwindow",
	        ioPtr->hullWindowNamePtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (AddDictEntry(interp, valuePtr2, "-varns", ioPtr->varNsNamePtr)
            != TCL_OK) {
        return TCL_ERROR;
    }
    objPtr = Tcl_NewObj();
    Tcl_GetCommandFullName(interp, ioPtr->accessCmd, objPtr);
    if (AddDictEntry(interp, valuePtr2, "-command", objPtr) != TCL_OK) {
	Tcl_DecrRefCount(objPtr);
        return TCL_ERROR;
    }
    keyPtr = ioPtr->namePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
	/* Cannot fail. Screened non-dicts earlier. */
        Tcl_DictObjPut(interp, dictPtr, keyPtr1, valuePtr1);
    } else {
	/* Don't leak the key val... */
	Tcl_DecrRefCount(keyPtr1);
    }
    Tcl_SetVar2Ex(interp, ITCL_NAMESPACE"::internal::dicts::objects",
            NULL, dictPtr, 0);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteObjectsDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclDeleteObjectsDictInfo(
    Tcl_Interp *interp,
    ItclObject *ioPtr)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *keyPtr1;
    Tcl_Obj *valuePtr;
    Tcl_Obj *valuePtr1;

    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::objects", NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::objects", NULL);
	return TCL_ERROR;
    }
    keyPtr1 = Tcl_NewStringObj("instances", -1);
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr1, &valuePtr) != TCL_OK) {
	Tcl_DecrRefCount(keyPtr1);
        return TCL_ERROR;
    }
    if (valuePtr == NULL) {
	/* looks like no object has been registered yet
	 * so ignore and return OK */
	Tcl_DecrRefCount(keyPtr1);
        return TCL_OK;
    }
    keyPtr = ioPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr, keyPtr, &valuePtr1) != TCL_OK) {
	Tcl_DecrRefCount(keyPtr1);
        return TCL_ERROR;
    }
    if (valuePtr1 == NULL) {
	/* looks like the object has not been constructed successfully
	 * so ignore and return OK */
	Tcl_DecrRefCount(keyPtr1);
        return TCL_OK;
    }
    if (Tcl_DictObjRemove(interp, valuePtr, keyPtr) != TCL_OK) {
	Tcl_DecrRefCount(keyPtr1);
        return TCL_ERROR;
    }
    if (Tcl_DictObjPut(interp, dictPtr, keyPtr1, valuePtr) != TCL_OK) {
	/* This is very likely impossible. non-dict already screened. */
	Tcl_DecrRefCount(keyPtr1);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(keyPtr1);
    Tcl_SetVar2Ex(interp, ITCL_NAMESPACE"::internal::dicts::objects",
            NULL, dictPtr, 0);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddOptionDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddOptionDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclOption *ioptPtr)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    int newValue1;

    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classOptions", NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classOptions", NULL);
	return TCL_ERROR;
    }
    keyPtr = iclsPtr->fullNamePtr;
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        valuePtr1 = Tcl_NewDictObj();
        newValue1 = 1;
    }
    keyPtr = ioptPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 == NULL) {
        valuePtr2 = Tcl_NewDictObj();
    }
    if (AddDictEntry(interp, valuePtr2, "-name", ioptPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioptPtr->fullNamePtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-fullname", ioptPtr->fullNamePtr)
                != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (AddDictEntry(interp, valuePtr2, "-resource", ioptPtr->resourceNamePtr)
            != TCL_OK) {
        return TCL_ERROR;
    }
    if (AddDictEntry(interp, valuePtr2, "-class", ioptPtr->classNamePtr)
            != TCL_OK) {
        return TCL_ERROR;
    }
    if (ioptPtr->defaultValuePtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-default",
	        ioptPtr->defaultValuePtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ioptPtr->flags & ITCL_OPTION_READONLY) {
        if (AddDictEntry(interp, valuePtr2, "-readonly",
	        Tcl_NewStringObj("1", -1)) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ioptPtr->cgetMethodPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-cgetmethod",
	        ioptPtr->cgetMethodPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ioptPtr->cgetMethodVarPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-cgetmethodvar",
	        ioptPtr->cgetMethodVarPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ioptPtr->configureMethodPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-configuremethod",
	        ioptPtr->cgetMethodPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ioptPtr->configureMethodVarPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-configuremethodvar",
	        ioptPtr->configureMethodVarPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ioptPtr->validateMethodPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-validatemethod",
	        ioptPtr->validateMethodPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ioptPtr->validateMethodVarPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-validatemethodvar",
	        ioptPtr->validateMethodVarPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    keyPtr = ioptPtr->namePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
        keyPtr = iclsPtr->fullNamePtr;
        if (Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr1) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetVar2Ex(interp, ITCL_NAMESPACE"::internal::dicts::classOptions",
            NULL, dictPtr, 0);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddDelegatedOptionDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddDelegatedOptionDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclDelegatedOption *idoPtr)
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    Tcl_Obj *listPtr;
    void *value;
    int haveExceptions;
    int newValue1;

    keyPtr = iclsPtr->fullNamePtr;
    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classDelegatedOptions",
	     NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classDelegatedOptions", NULL);
	return TCL_ERROR;
    }
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        valuePtr1 = Tcl_NewDictObj();
        newValue1 = 1;
    }
    keyPtr = idoPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 == NULL) {
        valuePtr2 = Tcl_NewDictObj();
    }
    if (AddDictEntry(interp, valuePtr2, "-name", idoPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (idoPtr->resourceNamePtr != NULL) {
         if (AddDictEntry(interp, valuePtr2, "-resource",
	         idoPtr->resourceNamePtr) != TCL_OK) {
             return TCL_ERROR;
        }
    }
    if (idoPtr->classNamePtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-class", idoPtr->classNamePtr)
                != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (idoPtr->icPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-component",
	        idoPtr->icPtr->namePtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (idoPtr->asPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-as", idoPtr->asPtr)
                != TCL_OK) {
            return TCL_ERROR;
        }
    }
    listPtr = Tcl_NewListObj(0, NULL);
    haveExceptions = 0;
    FOREACH_HASH(keyPtr, value, &idoPtr->exceptions) {
        if (value == NULL) {
            /* FIXME need code here */
        }
        haveExceptions = 1;
	Tcl_ListObjAppendElement(interp, listPtr, keyPtr);
    }
    if (haveExceptions) {
        if (AddDictEntry(interp, valuePtr2, "-except", listPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        Tcl_DecrRefCount(listPtr);
    }
    keyPtr = idoPtr->namePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
        keyPtr = iclsPtr->fullNamePtr;
        if (Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr1) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetVar2Ex(interp,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedOptions",
            NULL, dictPtr, 0);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddClassComponentDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddClassComponentDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclComponent *icPtr)
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    Tcl_Obj *listPtr;
    void *value;
    int newValue1;

    keyPtr = iclsPtr->fullNamePtr;
    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classComponents",
	     NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classComponents", NULL);
	return TCL_ERROR;
    }
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        valuePtr1 = Tcl_NewDictObj();
        newValue1 = 1;
    }
    keyPtr = icPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 == NULL) {
        valuePtr2 = Tcl_NewDictObj();
    }
    if (AddDictEntry(interp, valuePtr2, "-name", icPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (AddDictEntry(interp, valuePtr2, "-variable", icPtr->ivPtr->fullNamePtr)
             != TCL_OK) {
        return TCL_ERROR;
    }
    if (icPtr->flags & ITCL_COMPONENT_INHERIT) {
        if (AddDictEntry(interp, valuePtr2, "-inherit",
	        Tcl_NewStringObj("1", -1)) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (icPtr->flags & ITCL_COMPONENT_PUBLIC) {
        if (AddDictEntry(interp, valuePtr2, "-public",
	        Tcl_NewStringObj("1", -1)) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (icPtr->haveKeptOptions) {
        listPtr = Tcl_NewListObj(0, NULL);
        FOREACH_HASH(keyPtr, value, &icPtr->keptOptions) {
            if (value == NULL) {
                /* FIXME need code here */
            }
	    Tcl_ListObjAppendElement(interp, listPtr, keyPtr);
        }
        if (AddDictEntry(interp, valuePtr2, "-keptoptions", listPtr)
                != TCL_OK) {
            return TCL_ERROR;
        }
    }
    keyPtr = icPtr->namePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
        keyPtr = iclsPtr->fullNamePtr;
        if (Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr1) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetVar2Ex(interp,
            ITCL_NAMESPACE"::internal::dicts::classComponents",
            NULL, dictPtr, 0);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddClassVariableDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddClassVariableDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclVariable *ivPtr)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    Tcl_Obj *listPtr;
    const char *cp;
    int haveFlags;
    int newValue1;

    keyPtr = iclsPtr->fullNamePtr;
    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classVariables",
	     NULL, TCL_GLOBAL_ONLY);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classVariables", NULL);
	return TCL_ERROR;
    }
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        valuePtr1 = Tcl_NewDictObj();
        newValue1 = 1;
    }
    keyPtr = ivPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 == NULL) {
        valuePtr2 = Tcl_NewDictObj();
    }
    if (AddDictEntry(interp, valuePtr2, "-name", ivPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (AddDictEntry(interp, valuePtr2, "-fullname", ivPtr->fullNamePtr)
             != TCL_OK) {
        return TCL_ERROR;
    }
    if (ivPtr->init != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-init", ivPtr->init)
                 != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (ivPtr->arrayInitPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-arrayinit", ivPtr->arrayInitPtr)
                 != TCL_OK) {
             return TCL_ERROR;
        }
    }
    cp = Itcl_ProtectionStr(ivPtr->protection);
    if (AddDictEntry(interp, valuePtr2, "-protection", Tcl_NewStringObj(cp, -1))
             != TCL_OK) {
        return TCL_ERROR;
    }
    cp = "variable";
    if (ivPtr->flags & ITCL_COMMON) {
        cp = "common";
    }
    if (ivPtr->flags & ITCL_VARIABLE) {
        cp = "variable";
    }
    if (ivPtr->flags & ITCL_TYPE_VARIABLE) {
        cp = "typevariable";
    }
    if (AddDictEntry(interp, valuePtr2, "-type", Tcl_NewStringObj(cp, -1))
             != TCL_OK) {
        return TCL_ERROR;
    }
    haveFlags = 0;
    listPtr = Tcl_NewListObj(0, NULL);
    if (ivPtr->flags & ITCL_THIS_VAR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("this", -1));
    }
    if (ivPtr->flags & ITCL_SELF_VAR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("self", -1));
    }
    if (ivPtr->flags & ITCL_SELFNS_VAR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("selfns", -1));
    }
    if (ivPtr->flags & ITCL_WIN_VAR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("win", -1));
    }
    if (ivPtr->flags & ITCL_COMPONENT_VAR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("component", -1));
    }
    if (ivPtr->flags & ITCL_OPTIONS_VAR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("itcl_options", -1));
    }
    if (ivPtr->flags & ITCL_HULL_VAR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("itcl_hull", -1));
    }
    if (ivPtr->flags & ITCL_OPTION_READONLY) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("option_read_only", -1));
    }
    if (haveFlags) {
        if (AddDictEntry(interp, valuePtr2, "-flags", listPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        Tcl_DecrRefCount(listPtr);
    }
    if (ivPtr->codePtr != NULL) {
        if (ivPtr->codePtr->bodyPtr != NULL) {
            if (AddDictEntry(interp, valuePtr2, "-code",
	            ivPtr->codePtr->bodyPtr) != TCL_OK) {
                return TCL_ERROR;
            }
	}
    }
    keyPtr = ivPtr->namePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
        keyPtr = iclsPtr->fullNamePtr;
        if (Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr1) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetVar2Ex(interp,
            ITCL_NAMESPACE"::internal::dicts::classVariables",
            NULL, dictPtr, TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddClassFunctionDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddClassFunctionDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclMemberFunc *imPtr)
{
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    Tcl_Obj *listPtr;
    const char *cp;
    int haveFlags;
    int newValue1;

    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classFunctions",
	     NULL, TCL_GLOBAL_ONLY);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classFunctions", NULL);
	return TCL_ERROR;
    }
    keyPtr = iclsPtr->fullNamePtr;
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        valuePtr1 = Tcl_NewDictObj();
        newValue1 = 1;
    }
    keyPtr = imPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 != NULL) {
        Tcl_DictObjRemove(interp, valuePtr1, keyPtr);
    }
    valuePtr2 = Tcl_NewDictObj();
    if (AddDictEntry(interp, valuePtr2, "-name", imPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (AddDictEntry(interp, valuePtr2, "-fullname", imPtr->fullNamePtr)
             != TCL_OK) {
        return TCL_ERROR;
    }
    cp = "";
    if (imPtr->protection == ITCL_PUBLIC) {
        cp = "public";
    }
    if (imPtr->protection == ITCL_PROTECTED) {
        cp = "protected";
    }
    if (imPtr->protection == ITCL_PRIVATE) {
        cp = "private";
    }
    if (AddDictEntry(interp, valuePtr2, "-protection", Tcl_NewStringObj(cp, -1))
             != TCL_OK) {
        return TCL_ERROR;
    }
    cp = "";
    if (imPtr->flags & ITCL_COMMON) {
        cp = "common";
    }
    if (imPtr->flags & ITCL_METHOD) {
        cp = "method";
    }
    if (imPtr->flags & ITCL_TYPE_METHOD) {
        cp = "typemethod";
    }
    if (AddDictEntry(interp, valuePtr2, "-type", Tcl_NewStringObj(cp, -1))
             != TCL_OK) {
        return TCL_ERROR;
    }
    haveFlags = 0;
    listPtr = Tcl_NewListObj(0, NULL);
    if (imPtr->flags & ITCL_CONSTRUCTOR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("constructor", -1));
    }
    if (imPtr->flags & ITCL_DESTRUCTOR) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("destructor", -1));
    }
    if (imPtr->flags & ITCL_ARG_SPEC) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("have_args", -1));
    }
    if (imPtr->flags & ITCL_BODY_SPEC) {
        haveFlags = 1;
        Tcl_ListObjAppendElement(interp, listPtr,
	        Tcl_NewStringObj("have_body", -1));
    }
    if (haveFlags) {
        if (AddDictEntry(interp, valuePtr2, "-flags", listPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        Tcl_DecrRefCount(listPtr);
    }
    if (imPtr->codePtr != NULL) {
        if (imPtr->codePtr->bodyPtr != NULL) {
            if (AddDictEntry(interp, valuePtr2, "-body",
	            imPtr->codePtr->bodyPtr) != TCL_OK) {
                return TCL_ERROR;
            }
	}
        if (imPtr->codePtr->argumentPtr != NULL) {
            if (AddDictEntry(interp, valuePtr2, "-args",
	            imPtr->codePtr->argumentPtr) != TCL_OK) {
                return TCL_ERROR;
            }
	}
        if (imPtr->codePtr->usagePtr != NULL) {
            if (AddDictEntry(interp, valuePtr2, "-usage",
	            imPtr->codePtr->usagePtr) != TCL_OK) {
                return TCL_ERROR;
            }
	}
	haveFlags = 0;
	listPtr = Tcl_NewListObj(0, NULL);
        if (imPtr->codePtr->flags & ITCL_BUILTIN) {
	    haveFlags = 1;
            Tcl_ListObjAppendElement(interp, listPtr,
	            Tcl_NewStringObj("builtin", -1));
	}
	if (haveFlags) {
            if (AddDictEntry(interp, valuePtr2, "-codeflags", listPtr)
	            != TCL_OK) {
                return TCL_ERROR;
            }
	} else {
            Tcl_DecrRefCount(listPtr);
	}
    }
    keyPtr = imPtr->namePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
        keyPtr = iclsPtr->fullNamePtr;
        if (Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr1) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetVar2Ex(interp,
            ITCL_NAMESPACE"::internal::dicts::classFunctions",
            NULL, dictPtr, TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclAddClassDelegatedFunctionDictInfo()
 * ------------------------------------------------------------------------
 */
int
ItclAddClassDelegatedFunctionDictInfo(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclDelegatedFunction *idmPtr)
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *dictPtr;
    Tcl_Obj *keyPtr;
    Tcl_Obj *valuePtr1;
    Tcl_Obj *valuePtr2;
    Tcl_Obj *listPtr;
    void *value;
    int haveExceptions;
    int newValue1;

    keyPtr = iclsPtr->fullNamePtr;
    dictPtr = Tcl_GetVar2Ex(interp,
             ITCL_NAMESPACE"::internal::dicts::classDelegatedFunctions",
	     NULL, 0);
    if (dictPtr == NULL) {
        Tcl_AppendResult(interp, "cannot get dict ", ITCL_NAMESPACE,
	        "::internal::dicts::classDelegatedFunctions", NULL);
	return TCL_ERROR;
    }
    if (Tcl_DictObjGet(interp, dictPtr, keyPtr, &valuePtr1) != TCL_OK) {
        return TCL_ERROR;
    }
    newValue1 = 0;
    if (valuePtr1 == NULL) {
        valuePtr1 = Tcl_NewDictObj();
        newValue1 = 1;
    }
    keyPtr = idmPtr->namePtr;
    if (Tcl_DictObjGet(interp, valuePtr1, keyPtr, &valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (valuePtr2 == NULL) {
        valuePtr2 = Tcl_NewDictObj();
    }
    if (AddDictEntry(interp, valuePtr2, "-name", idmPtr->namePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (idmPtr->icPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-component",
	         idmPtr->icPtr->ivPtr->fullNamePtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (idmPtr->asPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-as", idmPtr->asPtr)
                != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (idmPtr->usingPtr != NULL) {
        if (AddDictEntry(interp, valuePtr2, "-using", idmPtr->usingPtr)
                != TCL_OK) {
            return TCL_ERROR;
        }
    }
    haveExceptions = 0;
    listPtr = Tcl_NewListObj(0, NULL);
    FOREACH_HASH(keyPtr, value, &idmPtr->exceptions) {
        if (value == NULL) {
            /* FIXME need code here */
        }
        haveExceptions = 1;
        if (Tcl_ListObjAppendElement(interp, listPtr, keyPtr) != TCL_OK) {
            return TCL_ERROR;
	}
    }

    if (haveExceptions) {
        if (AddDictEntry(interp, valuePtr2, "-except", listPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        Tcl_DecrRefCount(listPtr);
    }
    keyPtr = idmPtr->namePtr;
    if (Tcl_DictObjPut(interp, valuePtr1, keyPtr, valuePtr2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (newValue1) {
        keyPtr = iclsPtr->fullNamePtr;
        if (Tcl_DictObjPut(interp, dictPtr, keyPtr, valuePtr1) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetVar2Ex(interp,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedFunctions",
            NULL, dictPtr, 0);
    return TCL_OK;
}
