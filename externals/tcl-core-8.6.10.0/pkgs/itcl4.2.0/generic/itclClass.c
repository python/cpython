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
 *  These procedures handle class definitions.  Classes are composed of
 *  data members (public/protected/common) and the member functions
 *  (methods/procs) that operate on them.  Each class has its own
 *  namespace which manages the class scope.
 *
 * ========================================================================
 *  AUTHOR:  Michael J. McLennan
 *           Bell Labs Innovations for Lucent Technologies
 *           mmclennan@lucent.com
 *           http://www.tcltk.com/itcl
 *
 *  overhauled version author: Arnulf Wiedemann Copyright (c) 2007
 * ========================================================================
 *           Copyright (c) 1993-1998  Lucent Technologies, Inc.
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include "itclInt.h"

static Tcl_NamespaceDeleteProc* _TclOONamespaceDeleteProc = NULL;
static void ItclDeleteOption(char *cdata);

/*
 *  FORWARD DECLARATIONS
 */
static void ItclDestroyClass(ClientData cdata);
static void ItclFreeClass (char* cdata);
static void ItclDeleteFunction(ItclMemberFunc *imPtr);
static void ItclDeleteComponent(ItclComponent *icPtr);
static void ItclDeleteOption(char *cdata);

void
ItclPreserveClass(
    ItclClass *iclsPtr)
{
    iclsPtr->refCount++;
}

void
ItclReleaseClass(
    ClientData clientData)
{
    ItclClass *iclsPtr = (ItclClass *)clientData;

    if (iclsPtr->refCount-- <= 1) {
	ItclFreeClass((char *) clientData);
    }
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteMemberFunc()
 *
 * ------------------------------------------------------------------------
 */

void Itcl_DeleteMemberFunc (
    void *cdata)
{
    /* needed for stubs compatibility */
    ItclMemberFunc *imPtr;

    imPtr = (ItclMemberFunc *)cdata;
    if (imPtr == NULL) {
        /* FIXME need code here */
    }
    ItclDeleteFunction((ItclMemberFunc *)cdata);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDestroyClass2()
 *
 * ------------------------------------------------------------------------
 */

static void
ItclDestroyClass2(
    ClientData clientData)      /* The class being deleted. */
{
    ItclClass *iclsPtr = (ItclClass *)clientData;

    ItclDestroyClassNamesp(iclsPtr);
    ItclReleaseClass(iclsPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteClassMetadata()
 *
 *  Delete the metadata data if any
 *-------------------------------------------------------------------------
 */
void
ItclDeleteClassMetadata(
    ClientData clientData)
{
    /*
     * This is how we get alerted from TclOO that the object corresponding
     * to an Itcl class (or its namespace...) is being torn down.
     */

    ItclClass *iclsPtr = (ItclClass *)clientData;
    Tcl_Object oPtr = iclsPtr->oPtr;
    Tcl_Namespace *ooNsPtr = Tcl_GetObjectNamespace(oPtr);

    if (ooNsPtr != iclsPtr->nsPtr) {
	/*
	 * Itcl's idea of the class namespace is different from that of TclOO.
	 * Make sure both get torn down and pulled from tables.
	 */
	Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&iclsPtr->infoPtr->namespaceClasses,
		(char *)ooNsPtr);
	if (hPtr != NULL) {
	    Tcl_DeleteHashEntry(hPtr);
	}
	Tcl_DeleteNamespace(iclsPtr->nsPtr);
    } else {
	ItclDestroyClass2(iclsPtr);
    }
    ItclReleaseClass(iclsPtr);
}

static int
CallNewObjectInstance(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)data[0];
    const char *path = (const char *)data[1];
    Tcl_Object *oPtr = (Tcl_Object *)data[2];
    Tcl_Obj *nameObjPtr = (Tcl_Obj *)data[3];

    *oPtr = NULL;
    if (infoPtr->clazzClassPtr) {
	*oPtr = Tcl_NewObjectInstance(interp, infoPtr->clazzClassPtr,
                path, path, 0, NULL, 0);
    }
    if (*oPtr == NULL) {
        Tcl_AppendResult(interp,
                "ITCL: cannot create Tcl_NewObjectInstance for class \"",
                Tcl_GetString(nameObjPtr), "\"", NULL);
       return TCL_ERROR;
    }
    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_CreateClass()
 *
 *  Creates a namespace and its associated class definition data.
 *  If a namespace already exists with that name, then this routine
 *  returns TCL_ERROR, along with an error message in the interp.
 *  If successful, it returns TCL_OK and a pointer to the new class
 *  definition.
 * ------------------------------------------------------------------------
 */
int
Itcl_CreateClass(
    Tcl_Interp* interp,		/* interpreter that will contain new class */
    const char* path,		/* name of new class */
    ItclObjectInfo *infoPtr,	/* info for all known objects */
    ItclClass **rPtr)		/* returns: pointer to class definition */
{
    const char *head;
    const char *tail;
    Tcl_DString buffer;
    Tcl_Command cmd;
    Tcl_CmdInfo cmdInfo;
    Tcl_Namespace *classNs, *ooNs;
    Tcl_Object oPtr;
    Tcl_Obj *nameObjPtr;
    Tcl_Obj *namePtr;
    ItclClass *iclsPtr;
    ItclVariable *ivPtr;
    Tcl_HashEntry *hPtr;
    void *callbackPtr;
    int result;
    int newEntry;
    ItclResolveInfo *resolveInfoPtr;

    if (infoPtr->clazzObjectPtr == NULL) {
	Tcl_AppendResult(interp, "oo-subsystem is deleted", NULL);
	return TCL_ERROR;
    }

    /*
     * check for an empty class name to avoid a crash
     */
    if (strlen(path) == 0) {
	Tcl_AppendResult(interp, "invalid class name \"\"", NULL);
        return TCL_ERROR;
    }
    /*
     *  Make sure that a class with the given name does not
     *  already exist in the current namespace context.  If a
     *  namespace exists, that's okay.  It may have been created
     *  to contain stubs during a "namespace import" operation.
     *  We'll just replace the namespace data below with the
     *  proper class data.
     */
    classNs = Tcl_FindNamespace(interp, (const char *)path,
	    NULL, /* flags */ 0);

    if (classNs != NULL && Itcl_IsClassNamespace(classNs)) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "class \"", path, "\" already exists",
            NULL);
        return TCL_ERROR;
    }

    oPtr = NULL;
    /*
     *  Make sure that a command with the given class name does not
     *  already exist in the current namespace.  This prevents the
     *  usual Tcl commands from being clobbered when a programmer
     *  makes a bogus call like "class info".
     */
    cmd = Tcl_FindCommand(interp, (const char *)path,
	    NULL, /* flags */ TCL_NAMESPACE_ONLY);

    if (cmd != NULL && !Itcl_IsStub(cmd)) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "command \"", path, "\" already exists",
            NULL);

        if (strstr(path,"::") == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                " in namespace \"",
                Tcl_GetCurrentNamespace(interp)->fullName, "\"",
                NULL);
        }
        return TCL_ERROR;
    }

    /*
     *  Make sure that the class name does not have any goofy
     *  characters:
     *
     *    .  =>  reserved for member access like:  class.publicVar
     */
    Itcl_ParseNamespPath(path, &buffer, &head, &tail);

    if (strstr(tail,".")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad class name \"", tail, "\"",
            NULL);
        Tcl_DStringFree(&buffer);
        return TCL_ERROR;
    }
    Tcl_DStringFree(&buffer);

    /*
     *  Allocate class definition data.
     */
    iclsPtr = (ItclClass*)ckalloc(sizeof(ItclClass));
    memset(iclsPtr, 0, sizeof(ItclClass));
    iclsPtr->interp = interp;
    iclsPtr->infoPtr = infoPtr;
    Itcl_PreserveData(infoPtr);

    Tcl_InitObjHashTable(&iclsPtr->variables);
    Tcl_InitObjHashTable(&iclsPtr->functions);
    Tcl_InitObjHashTable(&iclsPtr->options);
    Tcl_InitObjHashTable(&iclsPtr->components);
    Tcl_InitObjHashTable(&iclsPtr->delegatedOptions);
    Tcl_InitObjHashTable(&iclsPtr->delegatedFunctions);
    Tcl_InitObjHashTable(&iclsPtr->methodVariables);
    Tcl_InitObjHashTable(&iclsPtr->resolveCmds);

    iclsPtr->numInstanceVars = 0;
    Tcl_InitHashTable(&iclsPtr->classCommons, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&iclsPtr->resolveVars, TCL_STRING_KEYS);
    Tcl_InitHashTable(&iclsPtr->contextCache, TCL_ONE_WORD_KEYS);

    Itcl_InitList(&iclsPtr->bases);
    Itcl_InitList(&iclsPtr->derived);

    resolveInfoPtr = (ItclResolveInfo *) ckalloc(sizeof(ItclResolveInfo));
    memset(resolveInfoPtr, 0, sizeof(ItclResolveInfo));
    resolveInfoPtr->flags = ITCL_RESOLVE_CLASS;
    resolveInfoPtr->iclsPtr = iclsPtr;
    iclsPtr->resolvePtr = (Tcl_Resolve *)ckalloc(sizeof(Tcl_Resolve));
    iclsPtr->resolvePtr->cmdProcPtr = Itcl_CmdAliasProc;
    iclsPtr->resolvePtr->varProcPtr = Itcl_VarAliasProc;
    iclsPtr->resolvePtr->clientData = resolveInfoPtr;
    iclsPtr->flags    = infoPtr->currClassFlags;

    /*
     *  Initialize the heritage info--each class starts with its
     *  own class definition in the heritage.  Base classes are
     *  added to the heritage from the "inherit" statement.
     */
    Tcl_InitHashTable(&iclsPtr->heritage, TCL_ONE_WORD_KEYS);
    (void) Tcl_CreateHashEntry(&iclsPtr->heritage, (char*)iclsPtr, &newEntry);

    /*
     *  Create a namespace to represent the class.  Add the class
     *  definition info as client data for the namespace.  If the
     *  namespace already exists, then replace any existing client
     *  data with the class data.
     */

    ItclPreserveClass(iclsPtr);

    nameObjPtr = Tcl_NewStringObj("", 0);
    Tcl_IncrRefCount(nameObjPtr);
    if ((path[0] != ':') || (path[1] != ':')) {
        Tcl_Namespace *currNsPtr = Tcl_GetCurrentNamespace(interp);
        Tcl_AppendToObj(nameObjPtr, currNsPtr->fullName, -1);
        if (currNsPtr->parentPtr != NULL) {
            Tcl_AppendToObj(nameObjPtr, "::", 2);
        }
    }
    Tcl_AppendToObj(nameObjPtr, path, -1);
    {
	/*
	 * TclOO machinery will refuse to overwrite an existing command
	 * with creation of a new object.  However, Itcl has a legacy
	 * "stubs" auto-importing mechanism that explicitly needs such
	 * overwriting.  So, check whether we have a stub, and if so,
	 * delete it before TclOO has a chance to object.
	 */
	Tcl_Command oldCmd = Tcl_FindCommand(interp, path, NULL, 0);

	if (Itcl_IsStub(oldCmd)) {
            Tcl_DeleteCommandFromToken(interp, oldCmd);
        }
    }
    callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
    /*
     *  Create a command in the current namespace to manage the class:
     *    <className>
     *    <className> <objName> ?<constructor-args>?
     */
    Tcl_NRAddCallback(interp, CallNewObjectInstance, infoPtr,
            (void *)path, &oPtr, nameObjPtr);
    result = Itcl_NRRunCallbacks(interp, callbackPtr);
    if (result == TCL_ERROR) {
	result = TCL_ERROR;
        goto errorOut;
    }
    iclsPtr->clsPtr = Tcl_GetObjectAsClass(oPtr);
    iclsPtr->oPtr = oPtr;
    ItclPreserveClass(iclsPtr);
    Tcl_ObjectSetMetadata(iclsPtr->oPtr, infoPtr->class_meta_type, iclsPtr);
    cmd = Tcl_GetObjectCommand(iclsPtr->oPtr);
    Tcl_GetCommandInfoFromToken(cmd, &cmdInfo);
    cmdInfo.deleteProc = ItclDestroyClass;
    cmdInfo.deleteData = iclsPtr;
    Tcl_SetCommandInfoFromToken(cmd, &cmdInfo);
    ooNs = Tcl_GetObjectNamespace(oPtr);
    classNs = Tcl_FindNamespace(interp, Tcl_GetString(nameObjPtr),
            NULL, /* flags */ 0);
    if (_TclOONamespaceDeleteProc == NULL) {
        _TclOONamespaceDeleteProc = ooNs->deleteProc;
    }

    if (classNs == NULL) {
	Tcl_AppendResult(interp,
	        "ITCL: cannot create/get class namespace for class \"",
		Tcl_GetString(iclsPtr->fullNamePtr), "\"", NULL);
        return TCL_ERROR;
    }

    if (iclsPtr->infoPtr->useOldResolvers) {
        Itcl_SetNamespaceResolvers(ooNs,
                (Tcl_ResolveCmdProc*)Itcl_ClassCmdResolver,
                (Tcl_ResolveVarProc*)Itcl_ClassVarResolver,
                (Tcl_ResolveCompiledVarProc*)Itcl_ClassCompiledVarResolver);
        Itcl_SetNamespaceResolvers(classNs,
                (Tcl_ResolveCmdProc*)Itcl_ClassCmdResolver,
                (Tcl_ResolveVarProc*)Itcl_ClassVarResolver,
                (Tcl_ResolveCompiledVarProc*)Itcl_ClassCompiledVarResolver);
    } else {
        Tcl_SetNamespaceResolver(ooNs, iclsPtr->resolvePtr);
        Tcl_SetNamespaceResolver(classNs, iclsPtr->resolvePtr);
    }
    iclsPtr->nsPtr = classNs;


    iclsPtr->namePtr = Tcl_NewStringObj(classNs->name, -1);
    Tcl_IncrRefCount(iclsPtr->namePtr);

    iclsPtr->fullNamePtr = Tcl_NewStringObj(classNs->fullName, -1);
    Tcl_IncrRefCount(iclsPtr->fullNamePtr);

    hPtr = Tcl_CreateHashEntry(&infoPtr->nameClasses,
            (char *)iclsPtr->fullNamePtr, &newEntry);
    Tcl_SetHashValue(hPtr, iclsPtr);


    hPtr = Tcl_CreateHashEntry(&infoPtr->namespaceClasses, (char *)classNs,
            &newEntry);
    Tcl_SetHashValue(hPtr, iclsPtr);
  if (classNs != ooNs) {
    hPtr = Tcl_CreateHashEntry(&infoPtr->namespaceClasses, (char *)ooNs,
            &newEntry);
    Tcl_SetHashValue(hPtr, iclsPtr);

    if (classNs->clientData && classNs->deleteProc) {
	(*classNs->deleteProc)(classNs->clientData);
    }
    classNs->clientData = iclsPtr;
    classNs->deleteProc = ItclDestroyClass2;
}

    hPtr = Tcl_CreateHashEntry(&infoPtr->classes, (char *)iclsPtr, &newEntry);
    Tcl_SetHashValue(hPtr, iclsPtr);

    /*
     * now build the namespace for the common private and protected variables
     * public variables go directly to the class namespace
     */
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
    Tcl_DStringAppend(&buffer,
	    (Tcl_GetObjectNamespace(iclsPtr->oPtr))->fullName, -1);
    if ((NULL == Tcl_FindNamespace(interp, Tcl_DStringValue(&buffer), NULL,
	    TCL_GLOBAL_ONLY)) && (NULL == Tcl_CreateNamespace(interp,
	    Tcl_DStringValue(&buffer), NULL, 0))) {
	Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "ITCL: cannot create variables namespace \"",
        Tcl_DStringValue(&buffer), "\"", NULL);
        result = TCL_ERROR;
        goto errorOut;
    }

    /*
     *  Add the built-in "this" command to the list of function members.
     */
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, Tcl_GetString(iclsPtr->fullNamePtr), -1);
    Tcl_DStringAppend(&buffer, "::this", -1);
    iclsPtr->thisCmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(&buffer),
            Itcl_ThisCmd, iclsPtr, NULL);

    /*
     *  Add the built-in "type" variable to the list of data members.
     */
    if (iclsPtr->flags & ITCL_TYPE) {
        namePtr = Tcl_NewStringObj("type", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
            NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_TYPE_VAR;       /* mark as "type" variable */
    }

    if (iclsPtr->flags & (ITCL_ECLASS)) {
        namePtr = Tcl_NewStringObj("win", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
            NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_WIN_VAR;        /* mark as "win" variable */
    }
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
        namePtr = Tcl_NewStringObj("self", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
            NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_SELF_VAR;       /* mark as "self" variable */

        namePtr = Tcl_NewStringObj("selfns", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
            NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_SELFNS_VAR;     /* mark as "selfns" variable */

        namePtr = Tcl_NewStringObj("win", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
            NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_WIN_VAR;        /* mark as "win" variable */
    }
    namePtr = Tcl_NewStringObj("this", -1);
    (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
            NULL, &ivPtr);
    ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
    ivPtr->flags |= ITCL_THIS_VAR;       /* mark as "this" variable */

    if (infoPtr->currClassFlags &
            (ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGETADAPTOR|ITCL_WIDGET)) {
        /*
         *  Add the built-in "itcl_options" variable to the list of
	 *  data members.
         */
        namePtr = Tcl_NewStringObj("itcl_options", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
                NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_OPTIONS_VAR;    /* mark as "itcl_options"
	                                      * variable */
    }
    if (infoPtr->currClassFlags &
            ITCL_ECLASS) {
        /*
         *  Add the built-in "itcl_option_components" variable to the list of
	 *  data members.
         */
        namePtr = Tcl_NewStringObj("itcl_option_components", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
                NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_OPTION_COMP_VAR; /* mark as "itcl_option_components"
	                                      * variable */
    }
    if (infoPtr->currClassFlags & (ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
        /*
         *  Add the built-in "thiswin" variable to the list of data members.
         */
        namePtr = Tcl_NewStringObj("thiswin", -1);
        (void) Itcl_CreateVariable(interp, iclsPtr, namePtr, NULL,
                NULL, &ivPtr);
        ivPtr->protection = ITCL_PROTECTED;  /* always "protected" */
        ivPtr->flags |= ITCL_THIS_VAR;       /* mark as "thiswin" variable */
    }
    if (infoPtr->currClassFlags & (ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
        /* create the itcl_hull component */
        ItclComponent *icPtr;
        namePtr = Tcl_NewStringObj("itcl_hull", 9);
	/* itcl_hull must not be an ITCL_COMMON!! */
        if (ItclCreateComponent(interp, iclsPtr, namePtr, 0, &icPtr) !=
	        TCL_OK) {
	    result = TCL_ERROR;
            goto errorOut;
        }
    }

    ItclPreserveClass(iclsPtr);
    iclsPtr->accessCmd = Tcl_GetObjectCommand(oPtr);

    /* FIXME should set the class objects unknown command to Itcl_HandleClass */

    *rPtr = iclsPtr;
    result = TCL_OK;
errorOut:
    Tcl_DecrRefCount(nameObjPtr);
    return result;
}


/*
 * ------------------------------------------------------------------------
 *  ItclDeleteClassVariablesNamespace()
 *
 * ------------------------------------------------------------------------
 */
void
ItclDeleteClassVariablesNamespace(
    Tcl_Interp *interp,
    ItclClass *iclsPtr)
{
    /* TODO: why is this being skipped? */
    return;
}

static int
CallDeleteOneObject(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch place;
    ItclClass *iclsPtr2 = NULL;
    ItclObject *contextIoPtr;
    ItclClass *iclsPtr = (ItclClass *)data[0];
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)data[1];
    void *callbackPtr;
    int classIsDeleted;

    if (result != TCL_OK) {
        return result;
    }
    classIsDeleted = 0;
    hPtr = Tcl_FindHashEntry(&infoPtr->classes, (char *)iclsPtr);
    if (hPtr == NULL) {
        classIsDeleted = 1;
    }
    if (classIsDeleted) {
        return result;
    }
    /*
     * Fix 227804: Whenever an object to delete was found we
     * have to reset the search to the beginning as the
     * current entry in the search was deleted and accessing it
     * is therefore not allowed anymore.
     */

    hPtr = Tcl_FirstHashEntry(&infoPtr->objects, &place);
    if (hPtr) {
        contextIoPtr = (ItclObject*)Tcl_GetHashValue(hPtr);

        while (contextIoPtr->iclsPtr != iclsPtr) {
            hPtr = Tcl_NextHashEntry(&place);
            if (hPtr == NULL) {
                break;
            }
        }
        if (hPtr) {
	    callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
            if (Itcl_DeleteObject(interp, contextIoPtr) != TCL_OK) {
                iclsPtr2 = iclsPtr;
                goto deleteClassFail;
            }

            Tcl_NRAddCallback(interp, CallDeleteOneObject, iclsPtr,
	            infoPtr, NULL, NULL);
            return Itcl_NRRunCallbacks(interp, callbackPtr);
        }

    }

    return TCL_OK;

deleteClassFail:
    /* check if class is not yet deleted */
    hPtr = Tcl_FindHashEntry(&infoPtr->classes, (char *)iclsPtr2);
    if (hPtr != NULL) {
        Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                "\n    (while deleting class \"%s\")",
                iclsPtr2->nsPtr->fullName));
    }
    return TCL_ERROR;
}

static int
CallDeleteOneClass(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_HashEntry *hPtr;
    ItclClass *iclsPtr = (ItclClass *)data[0];
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)data[1];
    int isDerivedReleased;

    if (result != TCL_OK) {
        return result;
    }
    isDerivedReleased = iclsPtr->flags & ITCL_CLASS_DERIVED_RELEASED;
    result = Itcl_DeleteClass(interp, iclsPtr);
    if (!isDerivedReleased) {
	if (result == TCL_OK) {
	    hPtr = Tcl_FindHashEntry(&infoPtr->classes, (char *)iclsPtr);
	    if (hPtr != NULL) {
	        /* release from derived reference */
		ItclReleaseClass(iclsPtr);
	    }
        }
    }
    if (result == TCL_OK) {
        return TCL_OK;
    }

    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
            "\n    (while deleting class \"%s\")",
            iclsPtr->nsPtr->fullName));
    return TCL_ERROR;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteClass()
 *
 *  Deletes a class by deleting all derived classes and all objects in
 *  that class, and finally, by destroying the class namespace.  This
 *  procedure provides a friendly way of doing this.  If any errors
 *  are detected along the way, the process is aborted.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
Itcl_DeleteClass(
    Tcl_Interp *interp,     /* interpreter managing this class */
    ItclClass *iclsPtr)    /* class */
{
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr2 = NULL;
    Itcl_ListElem *elem;
    void *callbackPtr;
    int result;

    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->classes, (char *)iclsPtr);
    if (hPtr == NULL) {
	/* class has already been deleted */
        return TCL_OK;
    }
    if (iclsPtr->flags & ITCL_CLASS_IS_DELETED) {
        return TCL_OK;
    }
    iclsPtr->flags |= ITCL_CLASS_IS_DELETED;
    /*
     *  Destroy all derived classes, since these lose their meaning
     *  when the base class goes away.  If anything goes wrong,
     *  abort with an error.
     *
     *  TRICKY NOTE:  When a derived class is destroyed, it
     *    automatically deletes itself from the "derived" list.
     */
    elem = Itcl_FirstListElem(&iclsPtr->derived);
    while (elem) {
        iclsPtr2 = (ItclClass*)Itcl_GetListValue(elem);
        elem = Itcl_NextListElem(elem);  /* advance here--elem will go away */

        callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
        Tcl_NRAddCallback(interp, CallDeleteOneClass, iclsPtr2,
	        iclsPtr2->infoPtr, NULL, NULL);
        result = Itcl_NRRunCallbacks(interp, callbackPtr);
        if (result != TCL_OK) {
            return result;
        }
    }

    /*
     *  Scan through and find all objects that belong to this class.
     *  Note that more specialized objects have already been
     *  destroyed above, when derived classes were destroyed.
     *  Destroy objects and report any errors.
     */
    /*
     * we have to enroll the while loop to fit for NRE
     * so we add a callback to delete the first element
     * and run this callback. At the end of the execution of that callback
     * we add eventually a callback for the next element and run that etc ...
     * if an error occurs we terminate the enrolled loop and return
     * otherwise we return at the end of the enrolled loop.
     */
    callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
    Tcl_NRAddCallback(interp, CallDeleteOneObject, iclsPtr,
            iclsPtr->infoPtr, NULL, NULL);
    result = Itcl_NRRunCallbacks(interp, callbackPtr);
    if (result != TCL_OK) {
        return result;
    }
    /*
     *  Destroy the namespace associated with this class.
     *
     *  TRICKY NOTE:
     *    The cleanup procedure associated with the namespace is
     *    invoked automatically.  It does all of the same things
     *    above, but it also disconnects this class from its
     *    base-class lists, and removes the class access command.
     */
    ItclDeleteClassVariablesNamespace(interp, iclsPtr);
    Tcl_DeleteNamespace(iclsPtr->nsPtr);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  ItclDestroyClass()
 *
 *  Invoked whenever the access command for a class is destroyed.
 *  Destroys the namespace associated with the class, which also
 *  destroys all objects in the class and all derived classes.
 *  Disconnects this class from the "derived" class lists of its
 *  base classes, and releases any claim to the class definition
 *  data.  If this is the last use of that data, the class will
 *  completely vanish at this point.
 * ------------------------------------------------------------------------
 */
static void
ItclDestroyClass(
    ClientData cdata)  /* class definition to be destroyed */
{
    ItclClass *iclsPtr = (ItclClass*)cdata;

    if (iclsPtr->flags & ITCL_CLASS_IS_DESTROYED) {
        return;
    }
    iclsPtr->flags |= ITCL_CLASS_IS_DESTROYED;
    if (!(iclsPtr->flags & ITCL_CLASS_NS_IS_DESTROYED)) {
	if (iclsPtr->accessCmd) {
	    Tcl_DeleteCommandFromToken(iclsPtr->interp, iclsPtr->accessCmd);
	    iclsPtr->accessCmd = NULL;
	}
        Tcl_DeleteNamespace(iclsPtr->nsPtr);
    }
    ItclReleaseClass(iclsPtr);
}


/*
 * ------------------------------------------------------------------------
 *  ItclDestroyClassNamesp()
 *
 *  Invoked whenever the namespace associated with a class is destroyed.
 *  Destroys all objects associated with this class and all derived
 *  classes.  Disconnects this class from the "derived" class lists
 *  of its base classes, and removes the class access command.  Releases
 *  any claim to the class definition data.  If this is the last use
 *  of that data, the class will completely vanish at this point.
 * ------------------------------------------------------------------------
 */
void
ItclDestroyClassNamesp(
    ClientData cdata)  /* class definition to be destroyed */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch place;
    Tcl_Command cmdPtr;
    ItclClass *iclsPtr;
    ItclObject *ioPtr;
    Itcl_ListElem *elem;
    Itcl_ListElem *belem;
    ItclClass *iclsPtr2;
    ItclClass *basePtr;
    ItclClass *derivedPtr;


    iclsPtr = (ItclClass*)cdata;
    if (iclsPtr->flags & ITCL_CLASS_NS_IS_DESTROYED) {
        return;
    }
    iclsPtr->flags |= ITCL_CLASS_NS_IS_DESTROYED;
    /*
     *  Destroy all derived classes, since these lose their meaning
     *  when the base class goes away.
     *
     *  TRICKY NOTE:  When a derived class is destroyed, it
     *    automatically deletes itself from the "derived" list.
     */
    elem = Itcl_FirstListElem(&iclsPtr->derived);
    while (elem) {
        iclsPtr2 = (ItclClass*)Itcl_GetListValue(elem);
	if (iclsPtr2->nsPtr != NULL) {
            Tcl_DeleteNamespace(iclsPtr2->nsPtr);
        }

	/* As the first namespace is now destroyed we have to get the
         * new first element of the hash table. We cannot go to the
         * next element from the current one, because the current one
         * is deleted. itcl Patch #593112, for Bug #577719.
	 */

        elem = Itcl_FirstListElem(&iclsPtr->derived);
    }

    /*
     *  Scan through and find all objects that belong to this class.
     *  Destroy them quietly by deleting their access command.
     */
    hPtr = Tcl_FirstHashEntry(&iclsPtr->infoPtr->objects, &place);
    while (hPtr) {
        ioPtr = (ItclObject*)Tcl_GetHashValue(hPtr);
        if (ioPtr->iclsPtr == iclsPtr) {
	    if ((ioPtr->accessCmd != NULL) && (!(ioPtr->flags &
	            (ITCL_OBJECT_IS_DESTRUCTED)))) {
		Itcl_PreserveData(ioPtr);
                Tcl_DeleteCommandFromToken(iclsPtr->interp, ioPtr->accessCmd);
	        ioPtr->accessCmd = NULL;
		Itcl_ReleaseData(ioPtr);
	        /*
	         * Fix 227804: Whenever an object to delete was found we
	         * have to reset the search to the beginning as the
	         * current entry in the search was deleted and accessing it
	         * is therefore not allowed anymore.
	         */

	        hPtr = Tcl_FirstHashEntry(&iclsPtr->infoPtr->objects, &place);
	        continue;
	    }
        }
        hPtr = Tcl_NextHashEntry(&place);
    }

    /*
     * Now there are no objects and inherited classes anymore, they could access a
     * private/protected common variables, so delete the internal namespace.
     */
    {
	Tcl_DString buffer;
	Tcl_Namespace *nsPtr;

	Tcl_DStringInit(&buffer);
	Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	Tcl_DStringAppend(&buffer,
	    (Tcl_GetObjectNamespace(iclsPtr->oPtr))->fullName, -1);
	nsPtr = Tcl_FindNamespace(iclsPtr->interp, Tcl_DStringValue(&buffer), NULL, 0);
	Tcl_DStringFree(&buffer);
	if (nsPtr != NULL) {
	    Tcl_DeleteNamespace(nsPtr);
	}
    }

    /*
     *  Next, remove this class from the "derived" list in
     *  all base classes.
     */
    belem = Itcl_FirstListElem(&iclsPtr->bases);
    while (belem) {
        basePtr = (ItclClass*)Itcl_GetListValue(belem);

        elem = Itcl_FirstListElem(&basePtr->derived);
        while (elem) {
            derivedPtr = (ItclClass*)Itcl_GetListValue(elem);
            if (derivedPtr == iclsPtr) {
		derivedPtr->flags |= ITCL_CLASS_DERIVED_RELEASED;
		ItclReleaseClass(derivedPtr);
                elem = Itcl_DeleteListElem(elem);
            } else {
                elem = Itcl_NextListElem(elem);
            }
        }
        belem = Itcl_NextListElem(belem);
    }

    /*
     *  Next, destroy the access command associated with the class.
     */
    iclsPtr->flags |= ITCL_CLASS_NS_TEARDOWN;
    if (iclsPtr->accessCmd) {
	cmdPtr = iclsPtr->accessCmd;
	iclsPtr->accessCmd = NULL;
        Tcl_DeleteCommandFromToken(iclsPtr->interp, cmdPtr);
    }

    /*
     *  Release the namespace's claim on the class definition.
     */
    ItclReleaseClass(iclsPtr);
}


/*
 * ------------------------------------------------------------------------
 *  ItclFreeClass()
 *
 *  Frees all memory associated with a class definition.  This is
 *  usually invoked automatically by Itcl_ReleaseData(), when class
 *  data is no longer being used.
 * ------------------------------------------------------------------------
 */
static void
ItclFreeClass(
    char *cdata)  /* class definition to be destroyed */
{
    FOREACH_HASH_DECLS;
    Tcl_HashSearch place;
    ItclClass *iclsPtr;
    ItclMemberFunc *imPtr;
    ItclVariable *ivPtr;
    ItclOption *ioptPtr;
    ItclComponent *icPtr;
    ItclDelegatedOption *idoPtr;
    ItclDelegatedFunction *idmPtr;
    Itcl_ListElem *elem;
    ItclVarLookup *vlookup;
    ItclCmdLookup *clookupPtr;
    Tcl_Var var;

    iclsPtr = (ItclClass*)cdata;
    if (iclsPtr->flags & ITCL_CLASS_IS_FREED) {
        return;
    }
    ItclDeleteClassesDictInfo(iclsPtr->interp, iclsPtr);
    iclsPtr->flags |= ITCL_CLASS_IS_FREED;

    /*
     *  Tear down the list of derived classes.  This list should
     *  really be empty if everything is working properly, but
     *  release it here just in case.
     */
    elem = Itcl_FirstListElem(&iclsPtr->derived);
    while (elem) {
	ItclReleaseClass( Itcl_GetListValue(elem) );
        elem = Itcl_NextListElem(elem);
    }
    Itcl_DeleteList(&iclsPtr->derived);

    /*
     *  Tear down the variable resolution table.  Some records
     *  appear multiple times in the table (for x, foo::x, etc.)
     *  so each one has a reference count.
     */
/*    Tcl_InitHashTable(&varTable, TCL_STRING_KEYS); */

    FOREACH_HASH_VALUE(vlookup, &iclsPtr->resolveVars) {
        if (--vlookup->usage == 0) {
            /*
             *  If this is a common variable owned by this class,
             *  then release the class's hold on it. FIXME !!!
             */
            ckfree((char*)vlookup);
        }
    }

    Tcl_DeleteHashTable(&iclsPtr->resolveVars);

    /*
     *  Tear down the virtual method table...
     */
    while (1) {
        hPtr = Tcl_FirstHashEntry(&iclsPtr->resolveCmds, &place);
        if (hPtr == NULL) {
            break;
        }
        clookupPtr = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
        ckfree((char *)clookupPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(&iclsPtr->resolveCmds);

    /*
     *  Delete all option definitions.
     */
    while (1) {
        hPtr = Tcl_FirstHashEntry(&iclsPtr->options, &place);
        if (hPtr == NULL) {
            break;
        }
        ioptPtr = (ItclOption *)Tcl_GetHashValue(hPtr);
	Tcl_DeleteHashEntry(hPtr);
        Itcl_ReleaseData(ioptPtr);
    }
    Tcl_DeleteHashTable(&iclsPtr->options);

    /*
     *  Delete all function definitions.
     */
    FOREACH_HASH_VALUE(imPtr, &iclsPtr->functions) {
	imPtr->iclsPtr = NULL;
        Itcl_ReleaseData(imPtr);
    }
    Tcl_DeleteHashTable(&iclsPtr->functions);

    /*
     *  Delete all delegated options.
     */
    FOREACH_HASH_VALUE(idoPtr, &iclsPtr->delegatedOptions) {
        Itcl_ReleaseData(idoPtr);
    }
    Tcl_DeleteHashTable(&iclsPtr->delegatedOptions);

    /*
     *  Delete all delegated functions.
     */
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	if ((idmPtr->icPtr == NULL)
		|| (idmPtr->icPtr->ivPtr->iclsPtr == iclsPtr)) {
            ItclDeleteDelegatedFunction(idmPtr);
	}
    }
    Tcl_DeleteHashTable(&iclsPtr->delegatedFunctions);

    /*
     *  Delete all components
     */
    while (1) {
	hPtr = Tcl_FirstHashEntry(&iclsPtr->components, &place);
	if (hPtr == NULL) {
	    break;
	}
	icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
	Tcl_DeleteHashEntry(hPtr);
	if (icPtr != NULL) {
            ItclDeleteComponent(icPtr);
	}
    }
    Tcl_DeleteHashTable(&iclsPtr->components);

    /*
     *  Delete all variable definitions.
     */
    while (1) {
        hPtr = Tcl_FirstHashEntry(&iclsPtr->variables, &place);
        if (hPtr == NULL) {
            break;
        }
        ivPtr = (ItclVariable *)Tcl_GetHashValue(hPtr);
	Tcl_DeleteHashEntry(hPtr);
	if (ivPtr != NULL) {
            Itcl_ReleaseData(ivPtr);
	}
    }
    Tcl_DeleteHashTable(&iclsPtr->variables);

    /*
     *  Release the claim on all base classes.
     */
    elem = Itcl_FirstListElem(&iclsPtr->bases);
    while (elem) {
	ItclReleaseClass( Itcl_GetListValue(elem) );
        elem = Itcl_NextListElem(elem);
    }
    Itcl_DeleteList(&iclsPtr->bases);
    Tcl_DeleteHashTable(&iclsPtr->heritage);

    /* remove owerself from the all classes entry */
    hPtr = Tcl_FindHashEntry(&iclsPtr->infoPtr->nameClasses,
            (char *)iclsPtr->fullNamePtr);
    if (hPtr != NULL) {
        Tcl_DeleteHashEntry(hPtr);
    }

    /* remove owerself from the all namespaceClasses entry */
    hPtr = Tcl_FindHashEntry(&iclsPtr->infoPtr->namespaceClasses,
            (char *)iclsPtr->nsPtr);
    if (hPtr != NULL) {
        Tcl_DeleteHashEntry(hPtr);
    }

    /* remove owerself from the all classes entry */
    hPtr = Tcl_FindHashEntry(&iclsPtr->infoPtr->classes, (char *)iclsPtr);
    if (hPtr != NULL) {
        Tcl_DeleteHashEntry(hPtr);
    }

    /* FIXME !!!
      free contextCache
      free resolvePtr -- this is only needed for CallFrame Resolvers
                      -- not used at the moment
     */

    FOREACH_HASH_VALUE(var, &iclsPtr->classCommons) {
	Itcl_ReleaseVar(var);
    }
    Tcl_DeleteHashTable(&iclsPtr->classCommons);

    /*
     *  Free up the widget class name
     */
    if (iclsPtr->widgetClassPtr != NULL) {
        Tcl_DecrRefCount(iclsPtr->widgetClassPtr);
    }

    /*
     *  Free up the widget hulltype name
     */
    if (iclsPtr->hullTypePtr != NULL) {
        Tcl_DecrRefCount(iclsPtr->hullTypePtr);
    }

    /*
     *  Free up the type typeconstrutor code
     */

    if (iclsPtr->typeConstructorPtr != NULL) {
        Tcl_DecrRefCount(iclsPtr->typeConstructorPtr);
    }

    /*
     *  Free up the object initialization code.
     */
    if (iclsPtr->initCode) {
        Tcl_DecrRefCount(iclsPtr->initCode);
    }

    Itcl_ReleaseData(iclsPtr->infoPtr);

    Tcl_DecrRefCount(iclsPtr->namePtr);
    Tcl_DecrRefCount(iclsPtr->fullNamePtr);

    if (iclsPtr->resolvePtr != NULL) {
        ckfree((char *)iclsPtr->resolvePtr->clientData);
        ckfree((char *)iclsPtr->resolvePtr);
    }
    ckfree(iclsPtr);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_IsClassNamespace()
 *
 *  Checks to see whether or not the given namespace represents an
 *  [incr Tcl] class.  Returns non-zero if so, and zero otherwise.
 * ------------------------------------------------------------------------
 */
int
Itcl_IsClassNamespace(
    Tcl_Namespace *nsPtr)  /* namespace being tested */
{
    ItclClass *iclsPtr = ItclNamespace2Class(nsPtr);
    return iclsPtr != NULL;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_IsClass()
 *
 *  Checks the given Tcl command to see if it represents an itcl class.
 *  Returns non-zero if the command is associated with a class.
 * ------------------------------------------------------------------------
 */
int
Itcl_IsClass(
    Tcl_Command cmd)         /* command being tested */
{
    Tcl_CmdInfo cmdInfo;

    if (Tcl_GetCommandInfoFromToken(cmd, &cmdInfo) == 0) {
        return 0;
    }
    if (cmdInfo.deleteProc == ItclDestroyClass) {
        return 1;
    }

    /*
     *  This may be an imported command.  Try to get the real
     *  command and see if it represents a class.
     */
    cmd = Tcl_GetOriginalCommand(cmd);
    if (cmd != NULL) {
	if (Tcl_GetCommandInfoFromToken(cmd, &cmdInfo) == 0) {
            return 0;
        }
        if (cmdInfo.deleteProc == ItclDestroyClass) {
            return 1;
        }
    }
    return 0;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_FindClass()
 *
 *  Searches for the specified class in the active namespace.  If the
 *  class is found, this procedure returns a pointer to the class
 *  definition.  Otherwise, if the autoload flag is non-zero, an
 *  attempt will be made to autoload the class definition.  If it
 *  still can't be found, this procedure returns NULL, along with an
 *  error message in the interpreter.
 * ------------------------------------------------------------------------
 */
ItclClass*
Itcl_FindClass(
    Tcl_Interp* interp,      /* interpreter containing class */
    const char* path,        /* path name for class */
    int autoload)
{
    /*
     *  Search for a namespace with the specified name, and if
     *  one is found, see if it is a class namespace.
     */

    Tcl_Namespace* classNs = Itcl_FindClassNamespace(interp, path);

    if (classNs) {
	ItclObjectInfo *infoPtr = (ItclObjectInfo *)
		Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
	Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses,
		(char *) classNs);
	if (hPtr) {
	    return (ItclClass *) Tcl_GetHashValue(hPtr);
	}
    }

    /*
     *  If the autoload flag is set, try to autoload the class
     *  definition, then search again.
     */
    if (autoload) {
        Tcl_DString buf;

        Tcl_DStringInit(&buf);
        Tcl_DStringAppend(&buf, "::auto_load ", -1);
        Tcl_DStringAppend(&buf, path, -1);
        if (Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0) != TCL_OK) {
            Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                    "\n    (while attempting to autoload class \"%s\")",
                    path));
            Tcl_DStringFree(&buf);
            return NULL;
        }
        Tcl_ResetResult(interp);
        Tcl_DStringFree(&buf);

	return Itcl_FindClass(interp, path, 0);
    }

    Tcl_AppendResult(interp, "class \"", path, "\" not found in context \"",
        Tcl_GetCurrentNamespace(interp)->fullName, "\"",
        NULL);

    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_FindClassNamespace()
 *
 *  Searches for the specified class namespace.  The normal Tcl procedure
 *  Tcl_FindNamespace also searches for namespaces, but only in the
 *  current namespace context.  This makes it hard to find one class
 *  from within another.  For example, suppose. you have two namespaces
 *  Foo and Bar.  If you're in the context of Foo and you look for
 *  Bar, you won't find it with Tcl_FindNamespace.  This behavior is
 *  okay for namespaces, but wrong for classes.
 *
 *  This procedure search for a class namespace.  If the name is
 *  absolute (i.e., starts with "::"), then that one name is checked,
 *  and the class is either found or not.  But if the name is relative,
 *  it is sought in the current namespace context and in the global
 *  context, just like the normal command lookup.
 *
 *  This procedure returns a pointer to the desired namespace, or
 *  NULL if the namespace was not found.
 * ------------------------------------------------------------------------
 */
Tcl_Namespace*
Itcl_FindClassNamespace(
    Tcl_Interp* interp,        /* interpreter containing class */
    const char* path)                /* path name for class */
{
    Tcl_Namespace* contextNs = Tcl_GetCurrentNamespace(interp);
    Tcl_Namespace *classNs = Tcl_FindNamespace(interp, path, NULL, 0);

    if ( !classNs /* We didn't find it... */
	    && contextNs->parentPtr != NULL	/* context is not global */
	    && (*path != ':' || *(path+1) != ':') /* path not FQ */
	) {

        if (strcmp(contextNs->name, path) == 0) {
            classNs = contextNs;
        } else {
            classNs = Tcl_FindNamespace(interp, path, NULL, TCL_GLOBAL_ONLY);
        }
    }
    return classNs;
}


static int
FinalizeCreateObject(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Obj *objNamePtr = (Tcl_Obj *)data[0];
    ItclClass *iclsPtr = (ItclClass *)data[1];
    if (result == TCL_OK) {
	if (!(iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
	    Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, Tcl_GetString(objNamePtr), NULL);
	}
    }
    Tcl_DecrRefCount(objNamePtr);
    return result;
}

static int
CallCreateObject(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Obj *objNamePtr = (Tcl_Obj *)data[0];
    ItclClass *iclsPtr = (ItclClass *)data[1];
    int objc = PTR2INT(data[2]);
    Tcl_Obj **objv = (Tcl_Obj **)data[3];

    if (result == TCL_OK) {
        result = ItclCreateObject(interp, Tcl_GetString(objNamePtr), iclsPtr,
                objc, objv);
    }
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_HandleClass()
 *
 *  first argument is ::itcl::parser::handleClass
 *  Invoked by Tcl whenever the user issues the command associated with
 *  a class name.  Handles the following syntax:
 *
 *    <className>
 *    <className> <objName> ?<args>...?
 *
 *  Without any arguments, the command does nothing.  In the olden days,
 *  this allowed the class name to be invoked by itself to prompt the
 *  autoloader to load the class definition.  Today, this behavior is
 *  retained for backward compatibility with old releases.
 *
 *  If arguments are specified, then this procedure creates a new
 *  object named <objName> in the appropriate class.  Note that if
 *  <objName> contains "#auto", that part is automatically replaced
 *  by a unique string built from the class name.
 * ------------------------------------------------------------------------
 */
int
Itcl_HandleClass(
    ClientData clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    if (objc > 3) {
	const char *token = Tcl_GetString(objv[3]);
	const char *nsEnd = NULL;
	const char *pos = token;
	const char *tail = pos;
	int fq = 0;
	int code = TCL_OK;
	Tcl_Obj *nsObj, *fqObj;

	while ((pos = strstr(pos, "::"))) {
	    if (pos == token) {
		fq = 1;
		nsEnd = token;
	    } else if (pos[-1] != ':') {
		nsEnd = pos - 1;
	    }
	    tail = pos + 2; pos++;
	}

	if (fq) {
	    nsObj = Tcl_NewStringObj(token, nsEnd-token);
	} else {
	    Tcl_Namespace *nsPtr = Tcl_GetCurrentNamespace(interp);

	    nsObj = Tcl_NewStringObj(nsPtr->fullName, -1);
	    if (nsEnd) {
		Tcl_AppendToObj(nsObj, "::", 2);
		Tcl_AppendToObj(nsObj, token, nsEnd-token);
	    }
	}

	fqObj = Tcl_DuplicateObj(nsObj);
	Tcl_AppendToObj(fqObj, "::", 2);
	Tcl_AppendToObj(fqObj, tail, -1);

	if (Tcl_GetCommandFromObj(interp, fqObj)) {
	    Tcl_AppendResult(interp, "command \"", tail,
		    "\" already exists in namespace \"", Tcl_GetString(nsObj),
		    "\"", NULL);
	    code = TCL_ERROR;
	}
	Tcl_DecrRefCount(fqObj);
	Tcl_DecrRefCount(nsObj);
	if (code != TCL_OK) {
	    return code;
	}
    }
    return ItclClassCreateObject(clientData, interp, objc, objv);
}

int
ItclClassCreateObject(
    ClientData clientData,   /* IclObjectInfo */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_DString buffer;  /* buffer used to build object names */
    Tcl_Obj *objNamePtr;
    Tcl_HashEntry *hPtr;
    Tcl_Obj **newObjv;
    ItclClass *iclsPtr;
    ItclObjectInfo *infoPtr;
    void *callbackPtr;
    char unique[256];    /* buffer used for unique part of object names */
    char *token;
    char *objName;
    char tmp;
    char *start;
    char *pos;
    const char *match;

    infoPtr = (ItclObjectInfo *)clientData;
    Tcl_ResetResult(interp);
    ItclShowArgs(1, "ItclClassCreateObject", objc, objv);
    /*
     *  If the command is invoked without an object name, then do nothing.
     *  This used to support autoloading--that the class name could be
     *  invoked as a command by itself, prompting the autoloader to
     *  load the class definition.  We retain the behavior here for
     *  backward-compatibility with earlier releases.
     */
    if (objc <= 3) {
        return TCL_OK;
    }

    hPtr = Tcl_FindHashEntry(&infoPtr->nameClasses, (char *)objv[2]);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "no such class: \"",
	        Tcl_GetString(objv[1]), "\"", NULL);
	return TCL_ERROR;
    }
    iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);

    /*
     *  If the object name is "::", and if this is an old-style class
     *  definition, then treat the remaining arguments as a command
     *  in the class namespace.  This used to be the way of invoking
     *  a class proc, but the new syntax is "class::proc" (without
     *  spaces).
     */
    token = Tcl_GetString(objv[3]);
    if ((*token == ':') && (strcmp(token,"::") == 0) && (objc > 4)) {
        /*
         *  If this is not an old-style class, then return an error
         *  describing the syntax change.
         */
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "syntax \"class :: proc\" is an anachronism\n",
            "[incr Tcl] no longer supports this syntax.\n",
            "Instead, remove the spaces from your procedure invocations:\n",
            "  ",
            Tcl_GetString(objv[1]), "::",
            Tcl_GetString(objv[4]), " ?args?",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  Otherwise, we have a proper object name.  Create a new instance
     *  with that name.  If the name contains "#auto", replace this with
     *  a uniquely generated string based on the class name.
     */
    Tcl_DStringInit(&buffer);
    objName = NULL;

    match = "#auto";
    start = token;
    for (pos=start; *pos != '\0'; pos++) {
        if (*pos == *match) {
            if (*(++match) == '\0') {
                tmp = *start;
                *start = '\0';  /* null-terminate first part */

                /*
                 *  Substitute a unique part in for "#auto", and keep
                 *  incrementing a counter until a valid name is found.
                 */
                do {
		    Tcl_CmdInfo dummy;

                    sprintf(unique,"%.200s%d", Tcl_GetString(iclsPtr->namePtr),
                        iclsPtr->unique++);
                    unique[0] = tolower(UCHAR(unique[0]));

                    Tcl_DStringSetLength(&buffer, 0);
                    Tcl_DStringAppend(&buffer, token, -1);
                    Tcl_DStringAppend(&buffer, unique, -1);
                    Tcl_DStringAppend(&buffer, start+5, -1);

                    objName = Tcl_DStringValue(&buffer);

		    /*
		     * [Fix 227811] Check for any command with the
		     * given name, not only objects.
		     */

                    if (Tcl_GetCommandInfo (interp, objName, &dummy) == 0) {
                        break;  /* if an error is found, bail out! */
                    }
                } while (1);

                *start = tmp;       /* undo null-termination */
                objName = Tcl_DStringValue(&buffer);
                break;              /* object name is ready to go! */
            }
        }
        else {
            match = "#auto";
            pos = start++;
        }
    }

    /*
     *  If "#auto" was not found, then just use object name as-is.
     */
    if (objName == NULL) {
        objName = token;
    }

    /*
     *  Try to create a new object.  If successful, return the
     *  object name as the result of this command.
     */
    objNamePtr = Tcl_NewStringObj(objName, -1);
    Tcl_IncrRefCount(objNamePtr);
    Tcl_DStringFree(&buffer);
    callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
    newObjv = (Tcl_Obj **)(objv+4);
    Tcl_NRAddCallback(interp, FinalizeCreateObject, objNamePtr, iclsPtr,
            NULL, NULL);
    Tcl_NRAddCallback(interp, CallCreateObject, objNamePtr, iclsPtr,
            INT2PTR(objc-4), newObjv);
    return Itcl_NRRunCallbacks(interp, callbackPtr);
}


/*
 * ------------------------------------------------------------------------
 *  ItclResolveVarEntry()
 *
 *  Side effect: (re)build part of resolver hash-table on demand.
 * ------------------------------------------------------------------------
 */
Tcl_HashEntry *
ItclResolveVarEntry(
    ItclClass* iclsPtr,       /* class definition where to resolve variable */
    const char *lookupName)      /* name of variable being resolved */
{
    Tcl_HashEntry *reshPtr, *hPtr;

    /* could be resolved directly */
    if ((reshPtr = Tcl_FindHashEntry(&iclsPtr->resolveVars, lookupName)) != NULL) {
	return reshPtr;
    } else {
	/* try to build virtual table for this var */
	const char *varName, *simpleName;
	Tcl_DString buffer, buffer2, *bufferC;
	ItclHierIter hier;
	ItclClass* iclsPtr2;
	ItclVarLookup *vlookup;
	ItclVariable *ivPtr;
	Tcl_Namespace* nsPtr;
	Tcl_Obj *vnObjPtr;
	int newEntry, processAncestors;
	size_t varLen;
      
	/* (de)qualify to simple name */
	varName = simpleName = lookupName;
	while(*varName) {
	    if (*varName++ == ':') {
		if (*varName++ == ':') { simpleName = varName; }
	    };
	}
	vnObjPtr = Tcl_NewStringObj(simpleName, -1);
	
	processAncestors = simpleName != lookupName;

	Tcl_DStringInit(&buffer);
	Tcl_DStringInit(&buffer2);

	/*
	 *  Scan through all classes in the hierarchy, from most to
	 *  least specific.  Add a lookup entry for each variable
	 *  into the table.
	 */
	Itcl_InitHierIter(&hier, iclsPtr);
	iclsPtr2 = Itcl_AdvanceHierIter(&hier);
	while (iclsPtr2 != NULL) {

	    hPtr = Tcl_FindHashEntry(&iclsPtr2->variables, vnObjPtr);
	    if (hPtr) {
		ivPtr = (ItclVariable*)Tcl_GetHashValue(hPtr);

		vlookup = NULL;

		/*
		 *  Create all possible names for this variable and enter
		 *  them into the variable resolution table:
		 *     var
		 *     class::var
		 *     namesp1::class::var
		 *     namesp2::namesp1::class::var
		 *     ...
		 */
		varName = simpleName; varLen = -1;
		bufferC = &buffer;
		nsPtr = iclsPtr2->nsPtr;

		while (1) {
		    hPtr = Tcl_CreateHashEntry(&iclsPtr->resolveVars,
			varName, &newEntry);

		    /* check for same name in current class */
		    if (!newEntry) {
			vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
			if (vlookup->ivPtr != ivPtr && iclsPtr2 == iclsPtr) {
			    /* if used multiple times - unbind, else - overwrite */
			    if (vlookup->usage > 1) {
				/* correct leastQualName */
				vlookup->leastQualName = NULL;
				processAncestors = 1; /* correction in progress */
				/* should create new lookup */
				--vlookup->usage;
				vlookup = NULL;
			    } else {
				/* correct values (overwrite) */
				vlookup->usage = 0;
				goto setResVar;
			    }
			    newEntry = 1;
			} else {
			    /* var exists and no correction necessary - next var */
			    if (!processAncestors) {
				break;
			    }
			    /* check leastQualName correction needed */
			    if (!vlookup->leastQualName) {
				vlookup->leastQualName = 
				    Tcl_GetHashKey(&iclsPtr->resolveVars, hPtr);
			    }
			    /* reset vlookup for full-qualified names - new lookup */
			    if (vlookup->ivPtr != ivPtr) {
				vlookup = NULL;
			    }
			}
		    }
		    if (newEntry) {
			if (!vlookup) {
			    /* create new (or overwrite) */
			    vlookup = (ItclVarLookup *)ckalloc(sizeof(ItclVarLookup));
			    vlookup->usage = 0;

			setResVar:

			    vlookup->ivPtr = ivPtr;
			    vlookup->leastQualName = 
				Tcl_GetHashKey(&iclsPtr->resolveVars, hPtr);

			    /*
			     *  If this variable is PRIVATE to another class scope,
			     *  then mark it as "inaccessible".
			     */
			    vlookup->accessible = (ivPtr->protection != ITCL_PRIVATE ||
				    ivPtr->iclsPtr == iclsPtr);

			    /*
			     *  Set aside the first object-specific slot for the built-in
			     *  "this" variable.  Only allocate one of these, even though
			     *  there is a definition for "this" in each class scope.
			     *  Set aside the second and third object-specific slot for the built-in
			     *  "itcl_options" and "itcl_option_components" variable.
			     */
			    if (!iclsPtr->numInstanceVars) {
				iclsPtr->numInstanceVars += 3;
			    }
			    /*
			     *  If this is a reference to the built-in "this"
			     *  variable, then its index is "0".  Otherwise,
			     *  add another slot to the end of the table.
			     */
			    if ((ivPtr->flags & ITCL_THIS_VAR) != 0) {
				vlookup->varNum = 0;
			    } else {
				if ((ivPtr->flags & ITCL_OPTIONS_VAR) != 0) {
				    vlookup->varNum = 1;
				} else {
				    vlookup->varNum = iclsPtr->numInstanceVars++;
				}
			    }
			}

			Tcl_SetHashValue(hPtr, vlookup);
			vlookup->usage++;
		    }

		    /* if we have found it */
		    if (simpleName == lookupName || strcmp(varName, lookupName) == 0) {
			if (!reshPtr) {
			    reshPtr = hPtr;
			}
			break;
		    }
		    if (nsPtr == NULL) {
			break;
		    }
		    Tcl_DStringSetLength(bufferC, 0);
		    Tcl_DStringAppend(bufferC, nsPtr->name, -1);
		    Tcl_DStringAppend(bufferC, "::", 2);
		    Tcl_DStringAppend(bufferC, varName, varLen);
		    varName = Tcl_DStringValue(bufferC);
		    varLen = Tcl_DStringLength(bufferC);
		    bufferC = (bufferC == &buffer) ? &buffer2 : &buffer;

		    nsPtr = nsPtr->parentPtr;
		}

	    }

	    /* Stop create vars for ancestors (if not needed) */
	    if (!processAncestors && reshPtr) {
		/* simple name - don't need to check ancestors */
		break;
	    }

	    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
	}
	Itcl_DeleteHierIter(&hier);

	Tcl_DStringFree(&buffer);
	Tcl_DStringFree(&buffer2);
	Tcl_DecrRefCount(vnObjPtr);

	if (reshPtr == NULL) {
	    reshPtr = Tcl_FindHashEntry(&iclsPtr->resolveVars, lookupName);
	}
	return reshPtr;
    }
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BuildVirtualTables()
 *
 *  Invoked whenever the class heritage changes or members are added or
 *  removed from a class definition to rebuild the member lookup
 *  tables.  There are two tables:
 *
 *  METHODS:  resolveCmds
 *    Used primarily in Itcl_ClassCmdResolver() to resolve all
 *    command references in a namespace.
 *
 *  DATA MEMBERS:  resolveVars (built on demand, moved to ItclResolveVarEntry)
 *    Used primarily in Itcl_ClassVarResolver() to quickly resolve
 *    variable references in each class scope.
 *
 *  These tables store every possible name for each command/variable
 *  (member, class::member, namesp::class::member, etc.).  Members
 *  in a derived class may shadow members with the same name in a
 *  base class.  In that case, the simple name in the resolution
 *  table will point to the most-specific member.
 * ------------------------------------------------------------------------
 */
void
Itcl_BuildVirtualTables(
    ItclClass* iclsPtr)       /* class definition being updated */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch place;
    Tcl_Namespace* nsPtr;
    Tcl_DString buffer, buffer2, *bufferC, *bufferC2, *bufferSwp;
    Tcl_Obj *objPtr;
    ItclMemberFunc *imPtr;
    ItclDelegatedFunction *idmPtr;
    ItclHierIter hier;
    ItclClass *iclsPtr2;
    ItclCmdLookup *clookupPtr;
    int newEntry;

    Tcl_DStringInit(&buffer);
    Tcl_DStringInit(&buffer2);

    /*
     *  Clear the command resolution table.
     */
    while (1) {
        hPtr = Tcl_FirstHashEntry(&iclsPtr->resolveCmds, &place);
        if (hPtr == NULL) {
            break;
        }
        clookupPtr = Tcl_GetHashValue(hPtr);
        ckfree((char *)clookupPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(&iclsPtr->resolveCmds);
    Tcl_InitObjHashTable(&iclsPtr->resolveCmds);

    /*
     *  Scan through all classes in the hierarchy, from most to
     *  least specific.  Look for the first (most-specific) definition
     *  of each member function, and enter it into the table.
     */
    Itcl_InitHierIter(&hier, iclsPtr);
    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    while (iclsPtr2 != NULL) {
        hPtr = Tcl_FirstHashEntry(&iclsPtr2->functions, &place);
        while (hPtr) {
            imPtr = (ItclMemberFunc*)Tcl_GetHashValue(hPtr);

            /*
             *  Create all possible names for this function and enter
             *  them into the command resolution table:
             *     func
             *     class::func
             *     namesp1::class::func
             *     namesp2::namesp1::class::func
             *     ...
             */
            Tcl_DStringSetLength(&buffer, 0);
            Tcl_DStringAppend(&buffer, Tcl_GetString(imPtr->namePtr), -1);
            bufferC = &buffer; bufferC2 = &buffer2;
            nsPtr = iclsPtr2->nsPtr;

            while (1) {
		objPtr = Tcl_NewStringObj(Tcl_DStringValue(bufferC),
				Tcl_DStringLength(bufferC));
                hPtr = Tcl_CreateHashEntry(&iclsPtr->resolveCmds,
                        (char *)objPtr, &newEntry);

                if (newEntry) {
		    clookupPtr = (ItclCmdLookup *)ckalloc(sizeof(ItclCmdLookup));
		    memset(clookupPtr, 0, sizeof(ItclCmdLookup));
		    clookupPtr->imPtr = imPtr;
                    Tcl_SetHashValue(hPtr, clookupPtr);
                } else {
		    Tcl_DecrRefCount(objPtr);
		}

                if (nsPtr == NULL) {
                    break;
                }

                Tcl_DStringSetLength(bufferC2, 0);
                Tcl_DStringAppend(bufferC2, nsPtr->name, -1);
                Tcl_DStringAppend(bufferC2, "::", 2);
                Tcl_DStringAppend(bufferC2, Tcl_DStringValue(bufferC),
				Tcl_DStringLength(bufferC));
                bufferSwp = bufferC; bufferC = bufferC2; bufferC2 = bufferSwp;

                nsPtr = nsPtr->parentPtr;
            }
            hPtr = Tcl_NextHashEntry(&place);
        }
        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    }
    Itcl_DeleteHierIter(&hier);

    /*
     *  Scan through all classes in the hierarchy, from most to
     *  least specific.  Look for the first (most-specific) definition
     *  of each delegated member function, and enter it into the table.
     */
    Itcl_InitHierIter(&hier, iclsPtr);
    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    while (iclsPtr2 != NULL) {
        hPtr = Tcl_FirstHashEntry(&iclsPtr2->delegatedFunctions, &place);
        while (hPtr) {
            idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
	    hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions,
	            (char *)idmPtr->namePtr);
	    if (hPtr == NULL) {
	        hPtr = Tcl_CreateHashEntry(&iclsPtr->delegatedFunctions,
		        (char *)idmPtr->namePtr, &newEntry);
                Tcl_SetHashValue(hPtr, idmPtr);
	    }
            hPtr = Tcl_NextHashEntry(&place);
        }
        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    }
    Itcl_DeleteHierIter(&hier);

    Tcl_DStringFree(&buffer);
    Tcl_DStringFree(&buffer2);
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_CreateVariable()
 *
 *  Creates a new class variable definition.  If this is a public
 *  variable, it may have a bit of "config" code that is used to
 *  update the object whenever the variable is modified via the
 *  built-in "configure" method.
 *
 *  Returns TCL_ERROR along with an error message in the specified
 *  interpreter if anything goes wrong.  Otherwise, this returns
 *  TCL_OK and a pointer to the new variable definition in "ivPtr".
 * ------------------------------------------------------------------------
 */
int
Itcl_CreateVariable(
    Tcl_Interp *interp,       /* interpreter managing this transaction */
    ItclClass* iclsPtr,       /* class containing this variable */
    Tcl_Obj* namePtr,         /* variable name */
    char* init,               /* initial value */
    char* config,             /* code invoked when variable is configured */
    ItclVariable** ivPtrPtr)  /* returns: new variable definition */
{
    int newEntry;
    ItclVariable *ivPtr;
    ItclMemberCode *mCodePtr;
    Tcl_HashEntry *hPtr;

    /*
     *  Add this variable to the variable table for the class.
     *  Make sure that the variable name does not already exist.
     */
    hPtr = Tcl_CreateHashEntry(&iclsPtr->variables, (char *)namePtr, &newEntry);
    if (!newEntry) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "variable name \"", Tcl_GetString(namePtr),
	    "\" already defined in class \"",
            Tcl_GetString(iclsPtr->fullNamePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If this variable has some "config" code, try to capture
     *  its implementation.
     */
    if (config) {
        if (Itcl_CreateMemberCode(interp, iclsPtr, NULL, config,
                &mCodePtr) != TCL_OK) {
            Tcl_DeleteHashEntry(hPtr);
            return TCL_ERROR;
        }
	Itcl_PreserveData(mCodePtr);
    } else {
        mCodePtr = NULL;
    }


    /*
     *  If everything looks good, create the variable definition.
     */
    ivPtr = (ItclVariable*)Itcl_Alloc(sizeof(ItclVariable));
    ivPtr->iclsPtr      = iclsPtr;
    ivPtr->infoPtr      = iclsPtr->infoPtr;
    ivPtr->protection   = Itcl_Protection(interp, 0);
    ivPtr->codePtr      = mCodePtr;
    ivPtr->namePtr      = namePtr;
    Tcl_IncrRefCount(ivPtr->namePtr);
    ivPtr->fullNamePtr = Tcl_NewStringObj(
            Tcl_GetString(iclsPtr->fullNamePtr), -1);
    Tcl_AppendToObj(ivPtr->fullNamePtr, "::", 2);
    Tcl_AppendToObj(ivPtr->fullNamePtr, Tcl_GetString(namePtr), -1);
    Tcl_IncrRefCount(ivPtr->fullNamePtr);

    if (ivPtr->protection == ITCL_DEFAULT_PROTECT) {
        ivPtr->protection = ITCL_PROTECTED;
    }

    if (init != NULL) {
        ivPtr->init = Tcl_NewStringObj(init, -1);
        Tcl_IncrRefCount(ivPtr->init);
    } else {
        ivPtr->init = NULL;
    }

    Tcl_SetHashValue(hPtr, ivPtr);
    Itcl_PreserveData(ivPtr);
    Itcl_EventuallyFree(ivPtr, (Tcl_FreeProc *) Itcl_DeleteVariable);

    *ivPtrPtr = ivPtr;
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_CreateOption()
 *
 *  Creates a new class option definition.  If this is a public
 *  option, it may have a bit of "config" code that is used to
 *  update the object whenever the option is modified via the
 *  built-in "configure" method.
 *
 *  Returns TCL_ERROR along with an error message in the specified
 *  interpreter if anything goes wrong.  Otherwise, this returns
 *  TCL_OK and a pointer to the new option definition in "ioptPtr".
 * ------------------------------------------------------------------------
 */
int
Itcl_CreateOption(
    Tcl_Interp *interp,       /* interpreter managing this transaction */
    ItclClass* iclsPtr,       /* class containing this variable */
    ItclOption* ioptPtr)      /* new option definition */
{
    int newEntry;
    Tcl_HashEntry *hPtr;

    /*
     *  Add this option to the options table for the class.
     *  Make sure that the option name does not already exist.
     */
    hPtr = Tcl_CreateHashEntry(&iclsPtr->options,
            (char *)ioptPtr->namePtr, &newEntry);
    if (!newEntry) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "option name \"", Tcl_GetString(ioptPtr->namePtr),
	    "\" already defined in class \"",
            Tcl_GetString(iclsPtr->fullNamePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    iclsPtr->numOptions++;
    ioptPtr->iclsPtr = iclsPtr;
    ioptPtr->codePtr = NULL;
    ioptPtr->fullNamePtr = Tcl_NewStringObj(
            Tcl_GetString(iclsPtr->fullNamePtr), -1);
    Tcl_AppendToObj(ioptPtr->fullNamePtr, "::", 2);
    Tcl_AppendToObj(ioptPtr->fullNamePtr, Tcl_GetString(ioptPtr->namePtr), -1);
    Tcl_IncrRefCount(ioptPtr->fullNamePtr);
    Tcl_SetHashValue(hPtr, ioptPtr);
    Itcl_PreserveData(ioptPtr);
    Itcl_EventuallyFree(ioptPtr, (Tcl_FreeProc *) ItclDeleteOption);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclCreateMethodVariable()
 *
 *  Creates a new class methdovariable definition.  If this is a public
 *  methodvariable,
 *
 *  Returns TCL_ERROR along with an error message in the specified
 *  interpreter if anything goes wrong.  Otherwise, this returns
 *  TCL_OK and a pointer to the new option definition in "imvPtr".
 * ------------------------------------------------------------------------
 */
int
ItclCreateMethodVariable(
    Tcl_Interp *interp,       /* interpreter managing this transaction */
    ItclVariable *ivPtr,      /* variable reference (from Itcl_CreateVariable) */
    Tcl_Obj* defaultPtr,      /* initial value */
    Tcl_Obj* callbackPtr,     /* code invoked when variable is set */
    ItclMethodVariable** imvPtrPtr)
                              /* returns: new methdovariable definition */
{
    int isNew;
    ItclMethodVariable *imvPtr;
    Tcl_HashEntry *hPtr;

    /*
     *  Add this methodvariable to the options table for the class.
     *  Make sure that the methodvariable name does not already exist.
     */
    hPtr = Tcl_CreateHashEntry(&ivPtr->iclsPtr->methodVariables,
            (char *)ivPtr->namePtr, &isNew);
    if (!isNew) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "methdovariable name \"", Tcl_GetString(ivPtr->namePtr),
	    "\" already defined in class \"",
            Tcl_GetString (ivPtr->iclsPtr->fullNamePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If everything looks good, create the option definition.
     */
    imvPtr = (ItclMethodVariable*)ckalloc(sizeof(ItclMethodVariable));
    memset(imvPtr, 0, sizeof(ItclMethodVariable));
    imvPtr->iclsPtr      = ivPtr->iclsPtr;
    imvPtr->protection   = Itcl_Protection(interp, 0);
    imvPtr->namePtr      = ivPtr->namePtr;
    Tcl_IncrRefCount(imvPtr->namePtr);
    imvPtr->fullNamePtr = ivPtr->fullNamePtr;
    Tcl_IncrRefCount(imvPtr->fullNamePtr);
    imvPtr->defaultValuePtr    = defaultPtr;
    if (defaultPtr != NULL) {
        Tcl_IncrRefCount(imvPtr->defaultValuePtr);
    }
    imvPtr->callbackPtr    = callbackPtr;
    if (callbackPtr != NULL) {
        Tcl_IncrRefCount(imvPtr->callbackPtr);
    }

    if (imvPtr->protection == ITCL_DEFAULT_PROTECT) {
        imvPtr->protection = ITCL_PROTECTED;
    }

    Tcl_SetHashValue(hPtr, imvPtr);

    *imvPtrPtr = imvPtr;
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_GetCommonVar()
 *
 *  Returns the current value for a common class variable.  The member
 *  name is interpreted with respect to the given class scope.  That
 *  scope is installed as the current context before querying the
 *  variable.  This by-passes the protection level in case the variable
 *  is "private".
 *
 *  If successful, this procedure returns a pointer to a string value
 *  which remains alive until the variable changes it value.  If
 *  anything goes wrong, this returns NULL.
 * ------------------------------------------------------------------------
 */
const char*
Itcl_GetCommonVar(
    Tcl_Interp *interp,        /* current interpreter */
    const char *name,          /* name of desired instance variable */
    ItclClass *contextIclsPtr) /* name is interpreted in this scope */
{
    const char *val = NULL;
    Tcl_HashEntry *hPtr;
    Tcl_DString buffer;
    Tcl_Obj *namePtr;
    ItclVariable *ivPtr;
    const char *cp;
    const char *lastCp;
    Tcl_Object oPtr = NULL;

    lastCp = name;
    cp = name;
    while (cp != NULL) {
        cp = strstr(lastCp, "::");
        if (cp != NULL) {
	    lastCp = cp + 2;
	}
    }
    namePtr = Tcl_NewStringObj(lastCp, -1);
    Tcl_IncrRefCount(namePtr);
    hPtr = Tcl_FindHashEntry(&contextIclsPtr->variables, (char *)namePtr);
    Tcl_DecrRefCount(namePtr);
    if (hPtr == NULL) {
        return NULL;
    }
    ivPtr = (ItclVariable *)Tcl_GetHashValue(hPtr);
    /*
     *  Activate the namespace for the given class.  That installs
     *  the appropriate name resolution rules and by-passes any
     *  security restrictions.
     */

    if (lastCp == name) {
	/* 'name' is a simple name (this is untested!!!!) */

	/* Use the context class passed in */
	oPtr = contextIclsPtr->oPtr;

    } else {
	int code = TCL_ERROR;
	Tcl_Obj *classObjPtr = Tcl_NewStringObj(name, lastCp - name - 2);
	oPtr = Tcl_GetObjectFromObj(interp, classObjPtr);

	if (oPtr) {
	    ItclClass *iclsPtr = (ItclClass *)Tcl_ObjectGetMetadata(oPtr,
		    contextIclsPtr->infoPtr->class_meta_type);
	    if (iclsPtr) {

		code = TCL_OK;
		assert(oPtr == iclsPtr->oPtr);

		/*
		 * If the caller gave us a qualified name into
		 * somewhere other than the context class, then
		 * things are really weird.  Consider an assertion
		 * to prevent, but for now keep the functioning
		 * unchanged.
		 *
		 * assert(iclsPtr == contextIclsPtr);
		 */

	    }

	}
	Tcl_DecrRefCount(classObjPtr);
	if (code != TCL_OK) {
	    return NULL;
	}

    }

    Tcl_DStringInit(&buffer);
    if (ivPtr->protection != ITCL_PUBLIC) {
        Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
    }
    Tcl_DStringAppend(&buffer, (Tcl_GetObjectNamespace(oPtr))->fullName, -1);
    Tcl_DStringAppend(&buffer, "::", -1);
    Tcl_DStringAppend(&buffer, lastCp, -1);

    val = Tcl_GetVar2(interp, (const char *)Tcl_DStringValue(&buffer),
            NULL, 0);
    Tcl_DStringFree(&buffer);
    return val;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_InitHierIter()
 *
 *  Initializes an iterator for traversing the hierarchy of the given
 *  class.  Subsequent calls to Itcl_AdvanceHierIter() will return
 *  the base classes in order from most-to-least specific.
 * ------------------------------------------------------------------------
 */
void
Itcl_InitHierIter(
    ItclHierIter *iter,   /* iterator used for traversal */
    ItclClass *iclsPtr)   /* class definition for start of traversal */
{
    Itcl_InitStack(&iter->stack);
    Itcl_PushStack(iclsPtr, &iter->stack);
    iter->current = iclsPtr;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteHierIter()
 *
 *  Destroys an iterator for traversing class hierarchies, freeing
 *  all memory associated with it.
 * ------------------------------------------------------------------------
 */
void
Itcl_DeleteHierIter(
    ItclHierIter *iter)  /* iterator used for traversal */
{
    Itcl_DeleteStack(&iter->stack);
    iter->current = NULL;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_AdvanceHierIter()
 *
 *  Moves a class hierarchy iterator forward to the next base class.
 *  Returns a pointer to the current class definition, or NULL when
 *  the end of the hierarchy has been reached.
 * ------------------------------------------------------------------------
 */
ItclClass*
Itcl_AdvanceHierIter(
    ItclHierIter *iter)  /* iterator used for traversal */
{
    Itcl_ListElem *elem;
    ItclClass *iclsPtr;

    iter->current = (ItclClass*)Itcl_PopStack(&iter->stack);

    /*
     *  Push classes onto the stack in reverse order, so that
     *  they will be popped off in the proper order.
     */
    if (iter->current) {
        iclsPtr = (ItclClass*)iter->current;
        elem = Itcl_LastListElem(&iclsPtr->bases);
        while (elem) {
            Itcl_PushStack(Itcl_GetListValue(elem), &iter->stack);
            elem = Itcl_PrevListElem(elem);
        }
    }
    return iter->current;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteVariable()
 *
 *  Destroys a variable definition created by Itcl_CreateVariable(),
 *  freeing all resources associated with it.
 * ------------------------------------------------------------------------
 */
void
Itcl_DeleteVariable(
    char *cdata)   /* variable definition to be destroyed */
{
    Tcl_HashEntry *hPtr;
    ItclVariable *ivPtr;

    ivPtr = (ItclVariable *)cdata;
    hPtr = Tcl_FindHashEntry(&ivPtr->infoPtr->classes, (char *)ivPtr->iclsPtr);
    if (hPtr != NULL) {
	/* unlink owerself from list of class variables */
        hPtr = Tcl_FindHashEntry(&ivPtr->iclsPtr->variables,
                (char *)ivPtr->namePtr);
        if (hPtr != NULL) {
            Tcl_DeleteHashEntry(hPtr);
        }
    }
    if (ivPtr->codePtr != NULL) {
	Itcl_ReleaseData(ivPtr->codePtr);
    }
    Tcl_DecrRefCount(ivPtr->namePtr);
    Tcl_DecrRefCount(ivPtr->fullNamePtr);
    if (ivPtr->init) {
        Tcl_DecrRefCount(ivPtr->init);
    }
    if (ivPtr->arrayInitPtr) {
        Tcl_DecrRefCount(ivPtr->arrayInitPtr);
    }
    Itcl_Free(ivPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteOption()
 *
 *  Destroys a option definition created by Itcl_CreateOption(),
 *  freeing all resources associated with it.
 * ------------------------------------------------------------------------
 */
static void
ItclDeleteOption(
    char *cdata)   /* option definition to be destroyed */
{
    ItclOption *ioptPtr;

    ioptPtr = (ItclOption *)cdata;
    Tcl_DecrRefCount(ioptPtr->namePtr);
    Tcl_DecrRefCount(ioptPtr->fullNamePtr);
    if (ioptPtr->resourceNamePtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->resourceNamePtr);
    }
    if (ioptPtr->resourceNamePtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->classNamePtr);
    }

    if (ioptPtr->codePtr) {
	Itcl_ReleaseData(ioptPtr->codePtr);
    }
    if (ioptPtr->defaultValuePtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->defaultValuePtr);
    }
    if (ioptPtr->cgetMethodPtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->cgetMethodPtr);
    }
    if (ioptPtr->cgetMethodVarPtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->cgetMethodVarPtr);
    }
    if (ioptPtr->configureMethodPtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->configureMethodPtr);
    }
    if (ioptPtr->configureMethodVarPtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->configureMethodVarPtr);
    }
    if (ioptPtr->validateMethodPtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->validateMethodPtr);
    }
    if (ioptPtr->validateMethodVarPtr != NULL) {
        Tcl_DecrRefCount(ioptPtr->validateMethodVarPtr);
    }
    Itcl_ReleaseData(ioptPtr->idoPtr);
    Itcl_Free(ioptPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteFunction()
 *
 *  fre data associated with a function
 * ------------------------------------------------------------------------
 */
static void
ItclDeleteFunction(
    ItclMemberFunc *imPtr)
{
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(&imPtr->infoPtr->procMethods,
	    (char *) imPtr->tmPtr);
    if (hPtr != NULL) {
	Tcl_DeleteHashEntry(hPtr);
    }
    hPtr = Tcl_FindHashEntry(&imPtr->infoPtr->classes, (char *)imPtr->iclsPtr);
    if (hPtr != NULL) {
	/* unlink owerself from list of class functions */
        hPtr = Tcl_FindHashEntry(&imPtr->iclsPtr->functions,
                (char *)imPtr->namePtr);
        if (hPtr != NULL) {
            Tcl_DeleteHashEntry(hPtr);
        }
    }
    if (imPtr->codePtr != NULL) {
        Itcl_ReleaseData(imPtr->codePtr);
    }
    Tcl_DecrRefCount(imPtr->namePtr);
    Tcl_DecrRefCount(imPtr->fullNamePtr);
    if (imPtr->usagePtr != NULL) {
        Tcl_DecrRefCount(imPtr->usagePtr);
    }
    if (imPtr->argumentPtr != NULL) {
        Tcl_DecrRefCount(imPtr->argumentPtr);
    }
    if (imPtr->origArgsPtr != NULL) {
        Tcl_DecrRefCount(imPtr->origArgsPtr);
    }
    if (imPtr->builtinArgumentPtr != NULL) {
        Tcl_DecrRefCount(imPtr->builtinArgumentPtr);
    }
    if (imPtr->bodyPtr != NULL) {
        Tcl_DecrRefCount(imPtr->bodyPtr);
    }
    if (imPtr->argListPtr != NULL) {
        ItclDeleteArgList(imPtr->argListPtr);
    }
    Itcl_Free(imPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteComponent()
 *
 *  free data associated with a component
 * ------------------------------------------------------------------------
 */
static void
ItclDeleteComponent(
    ItclComponent *icPtr)
{
    Tcl_Obj *objPtr;
    FOREACH_HASH_DECLS;

    Tcl_DecrRefCount(icPtr->namePtr);
    /* the variable and the command are freed when freeing variables,
     * functions */
    FOREACH_HASH_VALUE(objPtr, &icPtr->keptOptions) {
	if (objPtr != NULL) {
            Tcl_DecrRefCount(objPtr);
        }
    }
    Tcl_DeleteHashTable(&icPtr->keptOptions);
    ckfree((char*)icPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteDelegatedOption()
 *
 *  free data associated with a delegated option
 * ------------------------------------------------------------------------
 */
void
ItclDeleteDelegatedOption(
    char *cdata)
{
    Tcl_Obj *objPtr;
    FOREACH_HASH_DECLS;
    ItclDelegatedOption *idoPtr;

    idoPtr = (ItclDelegatedOption *)cdata;
    Tcl_DecrRefCount(idoPtr->namePtr);
    if (idoPtr->resourceNamePtr != NULL) {
        Tcl_DecrRefCount(idoPtr->resourceNamePtr);
    }
    if (idoPtr->classNamePtr != NULL) {
        Tcl_DecrRefCount(idoPtr->classNamePtr);
    }
    if (idoPtr->asPtr != NULL) {
        Tcl_DecrRefCount(idoPtr->asPtr);
    }
    FOREACH_HASH_VALUE(objPtr, &idoPtr->exceptions) {
	if (objPtr != NULL) {
            Tcl_DecrRefCount(objPtr);
        }
    }
    Tcl_DeleteHashTable(&idoPtr->exceptions);
    Itcl_Free(idoPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteDelegatedFunction()
 *
 *  free data associated with a delegated function
 * ------------------------------------------------------------------------
 */
void ItclDeleteDelegatedFunction(
    ItclDelegatedFunction *idmPtr)
{
    Tcl_Obj *objPtr;
    FOREACH_HASH_DECLS;

    Tcl_DecrRefCount(idmPtr->namePtr);
    if (idmPtr->asPtr != NULL) {
        Tcl_DecrRefCount(idmPtr->asPtr);
    }
    if (idmPtr->usingPtr != NULL) {
        Tcl_DecrRefCount(idmPtr->usingPtr);
    }
    FOREACH_HASH_VALUE(objPtr, &idmPtr->exceptions) {
	if (objPtr != NULL) {
            Tcl_DecrRefCount(objPtr);
        }
    }
    Tcl_DeleteHashTable(&idmPtr->exceptions);
    ckfree((char *)idmPtr);
}
