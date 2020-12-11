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
 *  This segment handles "objects" which are instantiated from class
 *  definitions.  Objects contain public/protected/private data members
 *  from all classes in a derivation hierarchy.
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
#include <tclInt.h>
#include "itclInt.h"

/*
 *  FORWARD DECLARATIONS
 */
static char* ItclTraceThisVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);
static char* ItclTraceTypeVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);
static char* ItclTraceSelfVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);
static char* ItclTraceSelfnsVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);
static char* ItclTraceWinVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);
static char* ItclTraceOptionVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);
static char* ItclTraceComponentVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);
static char* ItclTraceItclHullVar(ClientData cdata, Tcl_Interp *interp,
	const char *name1, const char *name2, int flags);

static void ItclDestroyObject(ClientData clientData);
static void FreeObject(char *cdata);

static int ItclDestructBase(Tcl_Interp *interp, ItclObject *contextObj,
        ItclClass *contextClass, int flags);

static int ItclInitObjectVariables(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr);
static int ItclInitObjectCommands(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr, const char *name);
static int ItclInitExtendedClassOptions(Tcl_Interp *interp, ItclObject *ioPtr);
static int ItclInitObjectOptions(Tcl_Interp *interp, ItclObject *ioPtr,
        ItclClass *iclsPtr);
static const char * GetConstructorVar(Tcl_Interp *interp, ItclClass *iclsPtr,
        const char *varName);
static ItclClass * GetClassFromClassName(Tcl_Interp *interp,
	const char *className, ItclClass *iclsPtr);


/*
 * ------------------------------------------------------------------------
 *  ItclDeleteObjectMetadata()
 *
 *  Delete the metadata data if any
 *-------------------------------------------------------------------------
 */
void
ItclDeleteObjectMetadata(
    ClientData clientData)
{
    ItclObject *ioPtr = (ItclObject *)clientData;
    Tcl_HashEntry *hPtr;

    if (ioPtr == NULL) return;		/* Safety */
    if (ioPtr->oPtr == NULL) return;	/* Safety */

    hPtr = Tcl_FindHashEntry(&ioPtr->infoPtr->instances,
	(Tcl_GetObjectNamespace(ioPtr->oPtr))->fullName);

    if (hPtr == NULL) return;

    if (clientData != Tcl_GetHashValue(hPtr)) {
	Tcl_Panic("invalid instances entry");
    }
    Tcl_DeleteHashEntry(hPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ObjectRenamedTrace()
 *
 * ------------------------------------------------------------------------
 */

static void
ObjectRenamedTrace(
    ClientData clientData,      /* The object being deleted. */
    Tcl_Interp *interp,         /* The interpreter containing the object. */
    const char *oldName,        /* What the object was (last) called. */
    const char *newName,        /* Always NULL ??. not for itk!! */
    int flags)                  /* Why was the object deleted? */
{
    ItclObject *ioPtr = (ItclObject *)clientData;
    Itcl_InterpState istate;

    if (newName != NULL) {
	/* FIXME should enter the new name in the hashtables for objects etc. */
        return;
    }
    if (ioPtr->flags & ITCL_OBJECT_CLASS_DESTRUCTED) {
        return;
    }
    ioPtr->flags |= ITCL_OBJECT_IS_RENAMED;
    if (ioPtr->flags & ITCL_TCLOO_OBJECT_IS_DELETED) {
        ioPtr->oPtr = NULL;
    }
    if (!(ioPtr->flags & ITCL_OBJECT_CLASS_DESTRUCTED)) {
        /*
         *  Attempt to destruct the object, but ignore any errors.
         */
        istate = Itcl_SaveInterpState(ioPtr->interp, 0);
        Itcl_DestructObject(ioPtr->interp, ioPtr, ITCL_IGNORE_ERRS);
        Itcl_RestoreInterpState(ioPtr->interp, istate);
        ioPtr->flags |= ITCL_OBJECT_CLASS_DESTRUCTED;
    }
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_CreateObject()
 *
 */
int
Itcl_CreateObject(
    Tcl_Interp *interp,      /* interpreter mananging new object */
    const char* name,        /* name of new object */
    ItclClass *iclsPtr,      /* class for new object */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[],   /* argument objects */
    ItclObject **rioPtr)     /* the created object */
{
    int result;
    ItclObjectInfo * infoPtr;

    result = ItclCreateObject(interp, name, iclsPtr, objc, objv);
    if (result == TCL_OK) {
        if (!(iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, name, NULL);
        }
    }
    if (rioPtr != NULL) {
        if (result == TCL_OK) {
            infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
			                    ITCL_INTERP_DATA, NULL);
            *rioPtr = infoPtr->lastIoPtr;
	} else {
            *rioPtr = NULL;
	}
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclCreateObject()
 *
 *  Creates a new object instance belonging to the given class.
 *  Supports complex object names like "namesp::namesp::name" by
 *  following the namespace path and creating the object in the
 *  desired namespace.
 *
 *  Automatically creates and initializes data members, including the
 *  built-in protected "this" variable containing the object name.
 *  Installs an access command in the current namespace, and invokes
 *  the constructor to initialize the object.
 *
 *  If any errors are encountered, the object is destroyed and this
 *  procedure returns TCL_ERROR (along with an error message in the
 *  interpreter).  Otherwise, it returns TCL_OK
 * ------------------------------------------------------------------------
 */
int
ItclCreateObject(
    Tcl_Interp *interp,      /* interpreter mananging new object */
    const char* name,        /* name of new object */
    ItclClass *iclsPtr,        /* class for new object */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int result = TCL_OK;

    Tcl_DString buffer;
    Tcl_CmdInfo cmdInfo;
    Tcl_Command cmdPtr;
    Tcl_HashEntry *hPtr;
    Tcl_Obj **newObjv;
    Tcl_Obj *objPtr;
    Tcl_Obj *saveNsNamePtr = NULL;
    ItclObjectInfo *infoPtr;
    ItclObject *saveCurrIoPtr;
    ItclObject *ioPtr;
    Itcl_InterpState istate;
    const char *nsName;
    const char *objName;
    char unique[256];    /* buffer used for unique part of object names */
    int newEntry;
    ItclResolveInfo *resolveInfoPtr;
    /* objv[1]: class name */
    /* objv[2]: class full name */
    /* objv[3]: object name */

    infoPtr = NULL;
    ItclShowArgs(1, "ItclCreateObject", objc, objv);
    saveCurrIoPtr = NULL;
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
        /* check, if the object already exists and if yes delete it silently */
	cmdPtr = Tcl_FindCommand(interp, name, NULL, 0);
	if (cmdPtr != NULL) {
            Tcl_GetCommandInfoFromToken(cmdPtr, &cmdInfo);
	    if (cmdInfo.deleteProc == ItclDestroyObject) {
		Itcl_RenameCommand(interp, name, "");
	    }
        }
    }
    /* just init for the case of none ItclWidget objects */
    newObjv = (Tcl_Obj **)objv;
    infoPtr = iclsPtr->infoPtr;

    if (infoPtr != NULL) {
      infoPtr->lastIoPtr = NULL;
    }
    /*
     *  Create a new object and initialize it.
     */
    ioPtr = (ItclObject*)Itcl_Alloc(sizeof(ItclObject));
    Itcl_EventuallyFree(ioPtr, (Tcl_FreeProc *)FreeObject);
    ioPtr->iclsPtr = iclsPtr;
    ioPtr->interp = interp;
    ioPtr->infoPtr = infoPtr;
    ItclPreserveClass(iclsPtr);

    ioPtr->constructed = (Tcl_HashTable*)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitObjHashTable(ioPtr->constructed);

    ioPtr->oPtr = Tcl_NewObjectInstance(interp, iclsPtr->clsPtr, NULL,
            /* nsName */ NULL, /* objc */ -1, /* objv */ NULL, /* skip */ 0);
    if (ioPtr->oPtr == NULL) {
	Itcl_Free(ioPtr);
        return TCL_ERROR;
    }

    /*
     *  Add a command to the current namespace with the object name.
     *  This is done before invoking the constructors so that the
     *  command can be used during construction to query info.
     */
    Itcl_PreserveData(ioPtr);

    ioPtr->namePtr = Tcl_NewStringObj(name, -1);
    Tcl_IncrRefCount(ioPtr->namePtr);
    nsName = Tcl_GetCurrentNamespace(interp)->fullName;
    ioPtr->origNamePtr = Tcl_NewStringObj("", -1);
    if ((name[0] != ':') && (name[1] != ':')) {
        Tcl_AppendToObj(ioPtr->origNamePtr, nsName, -1);
        if (strcmp(nsName, "::") != 0) {
            Tcl_AppendToObj(ioPtr->origNamePtr, "::", -1);
        }
    }
    Tcl_AppendToObj(ioPtr->origNamePtr, name, -1);
    Tcl_IncrRefCount(ioPtr->origNamePtr);
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
    Tcl_DStringAppend(&buffer,
	    (Tcl_GetObjectNamespace(ioPtr->oPtr))->fullName, -1);
    ioPtr->varNsNamePtr = Tcl_NewStringObj(Tcl_DStringValue(&buffer), -1);
    Tcl_IncrRefCount(ioPtr->varNsNamePtr);
    Tcl_DStringFree(&buffer);

    Tcl_InitHashTable(&ioPtr->objectVariables, TCL_ONE_WORD_KEYS);
    Tcl_InitObjHashTable(&ioPtr->objectOptions);
    Tcl_InitObjHashTable(&ioPtr->objectComponents);
    Tcl_InitObjHashTable(&ioPtr->objectDelegatedOptions);
    Tcl_InitObjHashTable(&ioPtr->objectDelegatedFunctions);
    Tcl_InitObjHashTable(&ioPtr->objectMethodVariables);
    Tcl_InitHashTable(&ioPtr->contextCache, TCL_ONE_WORD_KEYS);

    Itcl_PreserveData(ioPtr);

    /*
     *  Install the class namespace and object context so that
     *  the object's data members can be initialized via simple
     *  "set" commands.
     */

    /* first create the object's class variables namespaces
     * and set all the init values for variables
     */

    if (ItclInitObjectVariables(interp, ioPtr, iclsPtr) != TCL_OK) {
	ioPtr->hadConstructorError = 11;
	result = TCL_ERROR;
        goto errorReturn;
    }
    if (ItclInitObjectCommands(interp, ioPtr, iclsPtr, name) != TCL_OK) {
	Tcl_AppendResult(interp, "error in ItclInitObjectCommands", NULL);
	ioPtr->hadConstructorError = 12;
	result = TCL_ERROR;
        goto errorReturn;
    }
    if (iclsPtr->flags & (ITCL_ECLASS|ITCL_NWIDGET|ITCL_WIDGET|
            ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
	if (iclsPtr->flags & (ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|
	        ITCL_WIDGETADAPTOR)) {
            ItclInitExtendedClassOptions(interp, ioPtr);
            if (ItclInitObjectOptions(interp, ioPtr, iclsPtr) != TCL_OK) {
	        Tcl_AppendResult(interp, "error in ItclInitObjectOptions",
		        NULL);
	        ioPtr->hadConstructorError = 13;
	        result = TCL_ERROR;
                goto errorReturn;
            }
        }
        if (ItclInitObjectMethodVariables(interp, ioPtr, iclsPtr, name)
	        != TCL_OK) {
	    Tcl_AppendResult(interp,
	            "error in ItclInitObjectMethodVariables", NULL);
	    ioPtr->hadConstructorError = 14;
	    result = TCL_ERROR;
            goto errorReturn;
        }

	if (iclsPtr->flags & (ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
	    saveNsNamePtr = Tcl_GetVar2Ex(interp,
		    "::itcl::internal::varNsName", name, 0);
	    if (saveNsNamePtr) {
		Tcl_IncrRefCount(saveNsNamePtr);
	    }
	    Tcl_SetVar2Ex(interp, "::itcl::internal::varNsName", name,
		    ioPtr->varNsNamePtr, 0);
	}

    }

    saveCurrIoPtr = infoPtr->currIoPtr;
    infoPtr->currIoPtr = ioPtr;
    if (iclsPtr->flags & ITCL_WIDGET) {
        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc + 5));
        newObjv[0] = Tcl_NewStringObj(
                "::itcl::internal::commands::hullandoptionsinstall", -1);
        newObjv[1] = ioPtr->namePtr;
        Tcl_IncrRefCount(newObjv[1]);
        newObjv[2] = ioPtr->iclsPtr->namePtr;
        Tcl_IncrRefCount(newObjv[2]);
        if (ioPtr->iclsPtr->widgetClassPtr != NULL) {
            newObjv[3] = ioPtr->iclsPtr->widgetClassPtr;
        } else {
            newObjv[3] = Tcl_NewStringObj("", -1);
        }
        Tcl_IncrRefCount(newObjv[3]);
        if (ioPtr->iclsPtr->hullTypePtr != NULL) {
            newObjv[4] = ioPtr->iclsPtr->hullTypePtr;
        } else {
            newObjv[4] = Tcl_NewStringObj("", -1);
        }
        Tcl_IncrRefCount(newObjv[4]);
        memcpy(newObjv + 5, objv, (objc * sizeof(Tcl_Obj *)));
        result = Tcl_EvalObjv(interp, objc+5, newObjv, 0);
        Tcl_DecrRefCount(newObjv[0]);
        Tcl_DecrRefCount(newObjv[1]);
        Tcl_DecrRefCount(newObjv[2]);
        Tcl_DecrRefCount(newObjv[3]);
        Tcl_DecrRefCount(newObjv[4]);
        ckfree((char *)newObjv);
        if (result != TCL_OK) {
	    ioPtr->hadConstructorError = 15;
            goto errorReturn;
        }
    }
    objName = name;
    if (iclsPtr->flags & ITCL_WIDGETADAPTOR) {
        /* use a temporary name here as widgetadaptors often hijack the
	 * name for use in installhull. Rename it after the constructor has
	 * been run to the wanted name
	 */
        /*
         *  Add a unique part, and keep
         *  incrementing a counter until a valid name is found.
         */
        do {
	    Tcl_CmdInfo dummy;

            sprintf(unique,"%.200s_%d", name, iclsPtr->unique++);
            unique[0] = tolower(UCHAR(unique[0]));

            Tcl_DStringSetLength(&buffer, 0);
            Tcl_DStringAppend(&buffer, unique, -1);
            objName = Tcl_DStringValue(&buffer);

	    /*
	     * [Fix 227811] Check for any command with the
	     * given name, not only objects.
	     */

            if (Tcl_GetCommandInfo (interp, objName, &dummy) == 0) {
                break;  /* if an error is found, bail out! */
            }
        } while (1);
	ioPtr->createNamePtr = Tcl_NewStringObj(objName, -1);
    }

    {
	Tcl_Obj *tmp = Tcl_NewObj();

	Tcl_GetCommandFullName(interp, Tcl_GetObjectCommand(ioPtr->oPtr), tmp);
	Itcl_RenameCommand(interp, Tcl_GetString(tmp), objName);
	Tcl_TraceCommand(interp, objName,
            TCL_TRACE_RENAME|TCL_TRACE_DELETE, ObjectRenamedTrace, ioPtr);
	Tcl_DecrRefCount(tmp);
    }
    Tcl_ObjectSetMethodNameMapper(ioPtr->oPtr, ItclMapMethodNameProc);

    ioPtr->accessCmd = Tcl_GetObjectCommand(ioPtr->oPtr);
    Tcl_GetCommandInfoFromToken(ioPtr->accessCmd, &cmdInfo);
    cmdInfo.deleteProc = ItclDestroyObject;
    cmdInfo.deleteData = ioPtr;
    Tcl_SetCommandInfoFromToken(ioPtr->accessCmd, &cmdInfo);
    ioPtr->resolvePtr = (Tcl_Resolve *)ckalloc(sizeof(Tcl_Resolve));
    ioPtr->resolvePtr->cmdProcPtr = Itcl_CmdAliasProc;
    ioPtr->resolvePtr->varProcPtr = Itcl_VarAliasProc;
    resolveInfoPtr = (ItclResolveInfo *)ckalloc(sizeof(ItclResolveInfo));
    memset (resolveInfoPtr, 0, sizeof(ItclResolveInfo));
    resolveInfoPtr->flags = ITCL_RESOLVE_OBJECT;
    resolveInfoPtr->ioPtr = ioPtr;
    ioPtr->resolvePtr->clientData = resolveInfoPtr;

    Tcl_ObjectSetMetadata(ioPtr->oPtr, iclsPtr->infoPtr->object_meta_type,
            ioPtr);

    /* make the object known, if it is used in the constructor already! */
    hPtr = Tcl_CreateHashEntry(&iclsPtr->infoPtr->objectCmds,
        (char*)ioPtr->accessCmd, &newEntry);
    Tcl_SetHashValue(hPtr, ioPtr);

    hPtr = Tcl_CreateHashEntry(&iclsPtr->infoPtr->objects,
        (char*)ioPtr, &newEntry);
    Tcl_SetHashValue(hPtr, ioPtr);

    /* Use the TclOO object namespaces as a unique key in case the
     * object is renamed. Used by mytypemethod, etc. */

    hPtr = Tcl_CreateHashEntry(&iclsPtr->infoPtr->instances,
	(Tcl_GetObjectNamespace(ioPtr->oPtr))->fullName, &newEntry);
    Tcl_SetHashValue(hPtr, ioPtr);

    /*
     *  Now construct the object.  Look for a constructor in the
     *  most-specific class, and if there is one, invoke it.
     *  This will cause a chain reaction, making sure that all
     *  base classes constructors are invoked as well, in order
     *  from least- to most-specific.  Any constructors that are
     *  not called out explicitly in "initCode" code fragments are
     *  invoked implicitly without arguments.
     */
    ItclShowArgs(1, "OBJECTCONSTRUCTOR", objc, objv);
    ioPtr->hadConstructorError = 0;
    result = Itcl_InvokeMethodIfExists(interp, "constructor",
        iclsPtr, ioPtr, objc, objv);
    if (ioPtr->hadConstructorError) {
        result = TCL_ERROR;
    }
    ioPtr->hadConstructorError = -1;
    if (result != TCL_OK) {
        istate = Itcl_SaveInterpState(interp, result);
	ItclDeleteObjectVariablesNamespace(interp, ioPtr);
	if (ioPtr->accessCmd != (Tcl_Command) NULL) {
	    Tcl_DeleteCommandFromToken(interp, ioPtr->accessCmd);
	    ioPtr->accessCmd = NULL;
	}
        result = Itcl_RestoreInterpState(interp, istate);
	infoPtr->currIoPtr = saveCurrIoPtr;
	/* need this for 2 ReleaseData at errorReturn!! */
	Itcl_PreserveData(ioPtr);
        goto errorReturn;
    } else {
	/* a constructor cannot return a result as the object name
	 * is returned as result */
        Tcl_ResetResult(interp);
    }

    /*
     *  If there is no constructor, construct the base classes
     *  in case they have constructors.  This will cause the
     *  same chain reaction.
     */
    objPtr = Tcl_NewStringObj("constructor", -1);
    if (Tcl_FindHashEntry(&iclsPtr->functions, (char *)objPtr) == NULL) {
        result = Itcl_ConstructBase(interp, ioPtr, iclsPtr);
    }
    Tcl_DecrRefCount(objPtr);

    if (iclsPtr->flags & ITCL_ECLASS) {
        ItclInitExtendedClassOptions(interp, ioPtr);
        if (ItclInitObjectOptions(interp, ioPtr, iclsPtr) != TCL_OK) {
                Tcl_AppendResult(interp, "error in ItclInitObjectOptions",
	        NULL);
            result = TCL_ERROR;
            goto errorReturn;
        }
    }
    /*
     *  If construction failed, then delete the object access
     *  command.  This will destruct the object and delete the
     *  object data.  Be careful to save and restore the interpreter
     *  state, since the destructors may generate errors of their own.
     */
    if (result != TCL_OK) {
        istate = Itcl_SaveInterpState(interp, result);

	/* Bug 227824.
	 * The constructor may destroy the object, possibly indirectly
	 * through the destruction of the main widget in the iTk
	 * megawidget it tried to construct. If this happens we must
	 * not try to destroy the access command a second time.
	 */
	if (ioPtr->accessCmd != (Tcl_Command) NULL) {
	    Tcl_DeleteCommandFromToken(interp, ioPtr->accessCmd);
	    ioPtr->accessCmd = NULL;
	}
        result = Itcl_RestoreInterpState(interp, istate);
	/* need this for 2 ReleaseData at errorReturn!! */
	Itcl_PreserveData(ioPtr);
        goto errorReturn;
    }

    if (iclsPtr->flags & ITCL_WIDGETADAPTOR) {

	if (saveNsNamePtr) {
	    Tcl_SetVar2Ex(interp, "::itcl::internal::varNsName", name,
		    saveNsNamePtr, 0);
	    Tcl_DecrRefCount(saveNsNamePtr);
	    saveNsNamePtr = NULL;
	}

        Itcl_RenameCommand(interp, objName, name);
        ioPtr->createNamePtr = NULL;
        Tcl_TraceCommand(interp, Tcl_GetString(ioPtr->namePtr),
                TCL_TRACE_RENAME|TCL_TRACE_DELETE, ObjectRenamedTrace, ioPtr);
    }
    if (iclsPtr->flags & (ITCL_WIDGETADAPTOR)) {
        /*
         * set all the init values for options
         */

        objPtr = Tcl_NewStringObj(
	        ITCL_NAMESPACE"::internal::commands::widgetinitobjectoptions ",
		-1);
	Tcl_AppendToObj(objPtr, Tcl_GetString(ioPtr->varNsNamePtr), -1);
	Tcl_AppendToObj(objPtr, " ", -1);
	Tcl_AppendToObj(objPtr, Tcl_GetString(ioPtr->namePtr), -1);
	Tcl_AppendToObj(objPtr, " ", -1);
	Tcl_AppendToObj(objPtr, Tcl_GetString(iclsPtr->fullNamePtr), -1);
	Tcl_IncrRefCount(objPtr);
        result = Tcl_EvalObjEx(interp, objPtr, 0);
	Tcl_DecrRefCount(objPtr);
	if (result != TCL_OK) {
	    infoPtr->currIoPtr = saveCurrIoPtr;
	    result = TCL_ERROR;
            goto errorReturn;
	}
    }
    if (iclsPtr->flags & (ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
	/* FIXME have to check for hierarchy if ITCL_ECLASS !! */
	result = ItclCheckForInitializedComponents(interp, ioPtr->iclsPtr,
	        ioPtr);
	if (result != TCL_OK) {
            istate = Itcl_SaveInterpState(interp, result);
	    if (ioPtr->accessCmd != (Tcl_Command) NULL) {
	        Tcl_DeleteCommandFromToken(interp, ioPtr->accessCmd);
	        ioPtr->accessCmd = NULL;
	    }
            result = Itcl_RestoreInterpState(interp, istate);
	    /* need this for 2 ReleaseData at errorReturn!! */
	    Itcl_PreserveData(ioPtr);
            goto errorReturn;
	}
    }

    /*
     *  Add it to the list of all known objects. The only
     *  tricky thing to watch out for is the case where the
     *  object deleted itself inside its own constructor.
     *  In that case, we don't want to add the object to
     *  the list of valid objects. We can determine that
     *  the object deleted itself by checking to see if
     *  its accessCmd member is NULL.
     */
    if (result == TCL_OK && (ioPtr->accessCmd != NULL))  {

	if (!(ioPtr->iclsPtr->flags & ITCL_CLASS)) {
	    result = DelegationInstall(interp, ioPtr, iclsPtr);
	    if (result != TCL_OK) {
		goto errorReturn;
	    }
	}
        hPtr = Tcl_CreateHashEntry(&iclsPtr->infoPtr->objectCmds,
                (char*)ioPtr->accessCmd, &newEntry);
        Tcl_SetHashValue(hPtr, ioPtr);
        hPtr = Tcl_CreateHashEntry(&iclsPtr->infoPtr->objects,
                (char*)ioPtr, &newEntry);
        Tcl_SetHashValue(hPtr, ioPtr);

	/*
	 * This is an inelegant hack, left behind until the need for it
	 * can be eliminated by getting the inheritance tree right.
	 */

        if (iclsPtr->flags
		& (ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
	    Tcl_NewInstanceMethod(interp, ioPtr->oPtr,
		    Tcl_NewStringObj("unknown", -1), 0,
		    &itclRootMethodType, (void *)ItclUnknownGuts);
	}

        if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
            Tcl_Obj *objPtr = Tcl_NewObj();
            Tcl_GetCommandFullName(interp, ioPtr->accessCmd, objPtr);
            if (iclsPtr->flags & ITCL_WIDGETADAPTOR) {
	        /* skip over the leading :: */
		char *objName;
		char *lastObjName;
		lastObjName = Tcl_GetString(objPtr);
		objName = lastObjName;
		while (1) {
		    objName = strstr(objName, "::");
		    if (objName == NULL) {
		        break;
		    }
		    objName += 2;
		    lastObjName = objName;
		}

                Tcl_AppendResult(interp, lastObjName, NULL);
            } else {
                Tcl_AppendResult(interp, Tcl_GetString(objPtr), NULL);
	    }
	    Tcl_DecrRefCount(objPtr);
	}
    } else {
        if (ioPtr->accessCmd != NULL) {
            hPtr = Tcl_FindHashEntry(&iclsPtr->infoPtr->objectCmds,
                (char*)ioPtr->accessCmd);
	    if (hPtr != NULL) {
                Tcl_DeleteHashEntry(hPtr);
	    }
        }
    }

    /*
     *  Release the object.  If it was destructed above, it will
     *  die at this point.
     */
    /*
     *  At this point, the object is fully constructed.
     *  Destroy the "constructed" table in the object data, since
     *  it is no longer needed.
     */
    if (infoPtr != NULL) {
        infoPtr->currIoPtr = saveCurrIoPtr;
    }
    infoPtr->lastIoPtr = ioPtr;
    Tcl_DeleteHashTable(ioPtr->constructed);
    ckfree((char*)ioPtr->constructed);
    ioPtr->constructed = NULL;
    ItclAddObjectsDictInfo(interp, ioPtr);
    Itcl_ReleaseData(ioPtr);
    return result;

errorReturn:
    /*
     *  At this point, the object is not constructed as there was an error.
     *  Destroy the "constructed" table in the object data, since
     *  it is no longer needed.
     */
	if (saveNsNamePtr) {
	    Tcl_SetVar2Ex(interp, "::itcl::internal::varNsName", name,
		    saveNsNamePtr, 0);
	    Tcl_DecrRefCount(saveNsNamePtr);
	    saveNsNamePtr = NULL;
	}
    if (infoPtr != NULL) {
        infoPtr->lastIoPtr = ioPtr;
        infoPtr->currIoPtr = saveCurrIoPtr;
    }
    if (ioPtr->constructed != NULL) {
        Tcl_DeleteHashTable(ioPtr->constructed);
        ckfree((char*)ioPtr->constructed);
        ioPtr->constructed = NULL;
    }
    ItclDeleteObjectVariablesNamespace(interp, ioPtr);
    Itcl_ReleaseData(ioPtr);
    Itcl_ReleaseData(ioPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclInitObjectCommands()
 *
 *  Init all instance commands.
 *  This is usually invoked automatically
 *  by Itcl_CreateObject(), when an object is created.
 * ------------------------------------------------------------------------
 */
static int
ItclInitObjectCommands(
   Tcl_Interp *interp,
   ItclObject *ioPtr,
   ItclClass *iclsPtr,
   const char *name)
{
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclInitObjectVariables()
 *
 *  Init all instance variables and create the necessary variable namespaces
 *  for the given object instance.  This is usually invoked automatically
 *  by Itcl_CreateObject(), when an object is created.
 * ------------------------------------------------------------------------
 */
static int
ItclInitObjectVariables(
   Tcl_Interp *interp,
   ItclObject *ioPtr,
   ItclClass *iclsPtr)
{
    Tcl_DString buffer;
    Tcl_DString buffer2;
    Tcl_HashEntry *hPtr;
    Tcl_HashEntry *hPtr2;
    Tcl_HashSearch place;
    Tcl_Namespace *varNsPtr;
    Tcl_Namespace *varNsPtr2;
    Tcl_CallFrame frame;
    Tcl_Var varPtr;
    ItclClass *iclsPtr2;
    ItclHierIter hier;
    ItclVariable *ivPtr;
    ItclComponent *icPtr;
    const char *varName;
    const char *inheritComponentName;
    int itclOptionsIsSet;
    int isNew;

    ivPtr = NULL;
    /*
     * create all the variables for each class in the
     * ::itcl::variables::<object namespace>::<class> namespace as an
     * undefined variable using the Tcl "variable xx" command
     */
    itclOptionsIsSet = 0;
    inheritComponentName = NULL;
    Itcl_InitHierIter(&hier, iclsPtr);
    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    Tcl_ResetResult(interp);
    while (iclsPtr2 != NULL) {
	Tcl_DStringInit(&buffer);
	Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	Tcl_DStringAppend(&buffer,
		(Tcl_GetObjectNamespace(ioPtr->oPtr))->fullName, -1);
	Tcl_DStringAppend(&buffer, iclsPtr2->nsPtr->fullName, -1);
	varNsPtr = Tcl_FindNamespace(interp, Tcl_DStringValue(&buffer),
	        NULL, 0);
	if (varNsPtr == NULL) {
	    varNsPtr = Tcl_CreateNamespace(interp, Tcl_DStringValue(&buffer),
	            NULL, 0);
	}
	/* now initialize the variables which have an init value */
        if (Itcl_PushCallFrame(interp, &frame, varNsPtr,
                /*isProcCallFrame*/0) != TCL_OK) {
	    goto errorCleanup2;
        }
        hPtr = Tcl_FirstHashEntry(&iclsPtr2->variables, &place);
        while (hPtr) {
            ivPtr = (ItclVariable*)Tcl_GetHashValue(hPtr);
	    varName = Tcl_GetString(ivPtr->namePtr);
            if ((ivPtr->flags & ITCL_OPTIONS_VAR) && !itclOptionsIsSet) {
                /* this is the special code for the "itcl_options" variable */
		itclOptionsIsSet = 1;
                Tcl_DStringInit(&buffer2);
	        Tcl_DStringAppend(&buffer2, ITCL_VARIABLES_NAMESPACE, -1);
		Tcl_DStringAppend(&buffer,
			(Tcl_GetObjectNamespace(ioPtr->oPtr))->fullName, -1);
	        varNsPtr2 = Tcl_FindNamespace(interp,
		        Tcl_DStringValue(&buffer2), NULL, 0);
	        if (varNsPtr2 == NULL) {
	            varNsPtr2 = Tcl_CreateNamespace(interp,
		            Tcl_DStringValue(&buffer2), NULL, 0);
	        }
                Tcl_DStringFree(&buffer2);
	        Itcl_PopCallFrame(interp);
	        /* now initialize the variables which have an init value */
                if (Itcl_PushCallFrame(interp, &frame, varNsPtr2,
                        /*isProcCallFrame*/0) != TCL_OK) {
		    goto errorCleanup2;
                }
                Tcl_TraceVar2(interp, "itcl_options",
                        NULL,
                        TCL_TRACE_READS|TCL_TRACE_WRITES,
                        ItclTraceOptionVar, ioPtr);
	        Itcl_PopCallFrame(interp);
                if (Itcl_PushCallFrame(interp, &frame, varNsPtr,
                        /*isProcCallFrame*/0) != TCL_OK) {
		    goto errorCleanup2;
                }
                hPtr = Tcl_NextHashEntry(&place);
	        continue;
            }
            if (ivPtr->flags & ITCL_COMPONENT_VAR) {
		hPtr2 = Tcl_FindHashEntry(&ivPtr->iclsPtr->components,
		        (char *)ivPtr->namePtr);
		if (hPtr2 == NULL) {
		    Tcl_AppendResult(interp, "cannot find component \"",
		            Tcl_GetString(ivPtr->namePtr), "\" in class \"",
			    Tcl_GetString(ivPtr->iclsPtr->namePtr), NULL);
		    goto errorCleanup;
		}
		icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr2);
		if (icPtr->flags & ITCL_COMPONENT_INHERIT) {
		    if (inheritComponentName != NULL) {
		        Tcl_AppendResult(interp, "object \"",
			        Tcl_GetString(ioPtr->namePtr),
				"\" can only have one component with inherit.",
				" Had already component \"",
				inheritComponentName,
				"\" now component \"",
				Tcl_GetString(icPtr->namePtr), "\"", NULL);
		        goto errorCleanup;

		    } else {
		        inheritComponentName = Tcl_GetString(icPtr->namePtr);
		    }
		}
                hPtr2 = Tcl_CreateHashEntry(&ioPtr->objectComponents,
                        (char *)ivPtr->namePtr, &isNew);
		if (isNew) {
		    Tcl_SetHashValue(hPtr2, icPtr);
		}
                /* this is a component variable */
		/* FIXME initialize it to the empty string */
		/* the initialization  is arguable, should it be done? */
                if (Tcl_SetVar2(interp, varName, NULL,
	                "", TCL_NAMESPACE_ONLY) == NULL) {
                    Tcl_AppendResult(interp, "INTERNAL ERROR cannot set",
		            " variable \"", varName, "\"\n", NULL);
		    goto errorCleanup;
                }
	    }
            hPtr2 = ItclResolveVarEntry(ivPtr->iclsPtr, varName);
            if (hPtr2 == NULL) {
                hPtr = Tcl_NextHashEntry(&place);
	        continue;
            }
	    if ((ivPtr->flags & ITCL_COMMON) == 0) {
                varPtr = Tcl_NewNamespaceVar(interp, varNsPtr,
                        Tcl_GetString(ivPtr->namePtr));
	        hPtr2 = Tcl_CreateHashEntry(&ioPtr->objectVariables,
		        (char *)ivPtr, &isNew);
	        if (isNew) {
		    Itcl_PreserveVar(varPtr);
		    Tcl_SetHashValue(hPtr2, varPtr);
		}
	        if (ivPtr->flags & (ITCL_THIS_VAR|ITCL_TYPE_VAR|
		        ITCL_SELF_VAR|ITCL_SELFNS_VAR|ITCL_WIN_VAR)) {
                    int isDone = 0;
		    if (Tcl_SetVar2(interp, varName, NULL,
		        "", TCL_NAMESPACE_ONLY) == NULL) {
                        Tcl_AppendResult(interp, "INTERNAL ERROR cannot set",
			        " variable \"", varNsPtr->fullName, "::",
				varName, "\"\n", NULL);
		        goto errorCleanup;
	            }
	        if (ivPtr->flags & ITCL_THIS_VAR) {
	            Tcl_TraceVar2(interp, varName, NULL,
		        TCL_TRACE_READS|TCL_TRACE_WRITES, ItclTraceThisVar,
		        ioPtr);
                    isDone = 1;
		}
	        if (!isDone && ivPtr->flags & ITCL_TYPE_VAR) {
	            Tcl_TraceVar2(interp, varName, NULL,
		        TCL_TRACE_READS|TCL_TRACE_WRITES, ItclTraceTypeVar,
		        ioPtr);
                    isDone = 1;
		}
	        if (!isDone && ivPtr->flags & ITCL_SELF_VAR) {
	            Tcl_TraceVar2(interp, varName, NULL,
		        TCL_TRACE_READS|TCL_TRACE_WRITES, ItclTraceSelfVar,
		        ioPtr);
                    isDone = 1;
		}
	        if (!isDone && ivPtr->flags & ITCL_SELFNS_VAR) {
	            Tcl_TraceVar2(interp, varName, NULL,
		        TCL_TRACE_READS|TCL_TRACE_WRITES, ItclTraceSelfnsVar,
		        ioPtr);
                    isDone = 1;
		}
	        if (!isDone && ivPtr->flags & ITCL_WIN_VAR) {
	            Tcl_TraceVar2(interp, varName, NULL,
		        TCL_TRACE_READS|TCL_TRACE_WRITES, ItclTraceWinVar,
		        ioPtr);
                    isDone = 1;
		}
		} else {
	            if (ivPtr->flags & ITCL_HULL_VAR) {
	                Tcl_TraceVar2(interp, varName, NULL,
		            TCL_TRACE_READS|TCL_TRACE_WRITES,
			    ItclTraceItclHullVar,
		            ioPtr);
		    } else {
	              if (ivPtr->init != NULL) {
			if (Tcl_SetVar2(interp,
			        Tcl_GetString(ivPtr->namePtr), NULL,
			        Tcl_GetString(ivPtr->init),
				TCL_NAMESPACE_ONLY) == NULL) {
			    goto errorCleanup;
	                }
	              }
	              if (ivPtr->arrayInitPtr != NULL) {
			Tcl_DString buffer3;
	                int i;
	                int argc;
	                const char **argv;
	                const char *val;

			Tcl_DStringInit(&buffer3);
                        Tcl_DStringAppend(&buffer3, varNsPtr->fullName, -1);
                        Tcl_DStringAppend(&buffer3, "::", -1);
                        Tcl_DStringAppend(&buffer3,
			        Tcl_GetString(ivPtr->namePtr), -1);
	                Tcl_SplitList(interp,
			        Tcl_GetString(ivPtr->arrayInitPtr),
	                        &argc, &argv);
	                for (i = 0; i < argc; i++) {
                            val = Tcl_SetVar2(interp,
			            Tcl_DStringValue(&buffer3), argv[i],
                                    argv[i + 1], TCL_NAMESPACE_ONLY);
                            if (!val) {
                                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                                    "cannot initialize variable \"",
                                    Tcl_GetString(ivPtr->namePtr), "\"",
                                    NULL);
                                return TCL_ERROR;
                            }
	                    i++;
                        }
			Tcl_DStringFree(&buffer3);
		        ckfree((char *)argv);
		        }
		      }
		    }
	        } else {
	            if (ivPtr->flags & ITCL_HULL_VAR) {
	                Tcl_TraceVar2(interp, varName, NULL,
		            TCL_TRACE_READS|TCL_TRACE_WRITES,
			    ItclTraceItclHullVar,
		            ioPtr);
		    }
	            hPtr2 = Tcl_FindHashEntry(&iclsPtr2->classCommons,
		            (char *)ivPtr);
		    if (hPtr2 == NULL) {
		        goto errorCleanup;
		    }
		    varPtr = (Tcl_Var)Tcl_GetHashValue(hPtr2);
	            hPtr2 = Tcl_CreateHashEntry(&ioPtr->objectVariables,
		            (char *)ivPtr, &isNew);
	            if (isNew) {
			Itcl_PreserveVar(varPtr);
		        Tcl_SetHashValue(hPtr2, varPtr);
	        }
	        if (ivPtr->flags & ITCL_COMPONENT_VAR) {
	            if (ivPtr->flags & ITCL_COMMON) {
                        Tcl_Obj *objPtr2;
		        objPtr2 = Tcl_NewStringObj(ITCL_VARIABLES_NAMESPACE,
			        -1);
			Tcl_AppendToObj(objPtr2, (Tcl_GetObjectNamespace(
				ivPtr->iclsPtr->oPtr))->fullName, -1);
                        Tcl_AppendToObj(objPtr2, "::", -1);
                        Tcl_AppendToObj(objPtr2, varName, -1);
			/* itcl_hull is traced in itclParse.c */
			if (strcmp(varName, "itcl_hull") == 0) {
                            Tcl_TraceVar2(interp,
                                    Tcl_GetString(objPtr2), NULL,
	                            TCL_TRACE_WRITES, ItclTraceItclHullVar,
	                            ioPtr);
			} else {
                            Tcl_TraceVar2(interp,
                                    Tcl_GetString(objPtr2), NULL,
	                            TCL_TRACE_WRITES, ItclTraceComponentVar,
	                            ioPtr);
		        }
		        Tcl_DecrRefCount(objPtr2);
		    } else {
                        Tcl_TraceVar2(interp,
		                varName, NULL,
	                        TCL_TRACE_WRITES, ItclTraceComponentVar,
	                        ioPtr);
	            }
	        }
	    }
            hPtr = Tcl_NextHashEntry(&place);
        }
	Itcl_PopCallFrame(interp);
        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    }
    Tcl_DStringFree(&buffer);
    Itcl_DeleteHierIter(&hier);
    return TCL_OK;
errorCleanup:
    Itcl_PopCallFrame(interp);
errorCleanup2:
    varNsPtr = Tcl_FindNamespace(interp, Tcl_GetString(ioPtr->varNsNamePtr),
            NULL, 0);
    if (varNsPtr != NULL) {
        Tcl_DeleteNamespace(varNsPtr);
    }
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  ItclInitObjectOptions()
 *
 *  Collect all instance options for the given object instance to allow
 *  faster runtime access to the options.
 *  if the same option name is used in more than one class the first one
 *  found is used (for initializing and for the class name)!!
 *  # It is assumed, that an option can only exist in one class??
 *  # So no duplicates allowed??
 *  This is usually invoked automatically by Itcl_CreateObject(),
 *  when an object is created.
 * ------------------------------------------------------------------------
 */
int
ItclInitObjectOptions(
   Tcl_Interp *interp,
   ItclObject *ioPtr,
   ItclClass *iclsPtr)
{
    Tcl_DString buffer;
    Tcl_HashEntry *hPtr;
    Tcl_HashEntry *hPtr2;
    Tcl_HashSearch place;
    Tcl_CallFrame frame;
    Tcl_Namespace *varNsPtr;
    ItclClass *iclsPtr2;
    ItclHierIter hier;
    ItclOption *ioptPtr;
    ItclDelegatedOption *idoPtr;
    int isNew;

    ioptPtr = NULL;
    Itcl_InitHierIter(&hier, iclsPtr);
    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    while (iclsPtr2 != NULL) {
	/* now initialize the options which have an init value */
        hPtr = Tcl_FirstHashEntry(&iclsPtr2->options, &place);
        while (hPtr) {
            ioptPtr = (ItclOption*)Tcl_GetHashValue(hPtr);
	    hPtr2 = Tcl_CreateHashEntry(&ioPtr->objectOptions,
	            (char *)ioptPtr->namePtr, &isNew);
	    if (isNew) {
		Tcl_SetHashValue(hPtr2, ioptPtr);
                Tcl_DStringInit(&buffer);
	        Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
		Tcl_DStringAppend(&buffer,
			(Tcl_GetObjectNamespace(ioPtr->oPtr)->fullName), -1);
	        varNsPtr = Tcl_FindNamespace(interp,
		        Tcl_DStringValue(&buffer), NULL, 0);
	        if (varNsPtr == NULL) {
	            varNsPtr = Tcl_CreateNamespace(interp,
		            Tcl_DStringValue(&buffer), NULL, 0);
		}
                Tcl_DStringFree(&buffer);
	        /* now initialize the options which have an init value */
                if (Itcl_PushCallFrame(interp, &frame, varNsPtr,
                        /*isProcCallFrame*/0) != TCL_OK) {
                    return TCL_ERROR;
                }
		if ((ioptPtr != NULL) && (ioptPtr->namePtr != NULL) &&
		        (ioptPtr->defaultValuePtr != NULL)) {
                    if (Tcl_SetVar2(interp, "itcl_options",
		            Tcl_GetString(ioptPtr->namePtr),
	                    Tcl_GetString(ioptPtr->defaultValuePtr),
			    TCL_NAMESPACE_ONLY) == NULL) {
	                Itcl_PopCallFrame(interp);
		        return TCL_ERROR;
                    }
                    Tcl_TraceVar2(interp, "itcl_options",
                            NULL,
                            TCL_TRACE_READS|TCL_TRACE_WRITES,
                            ItclTraceOptionVar, ioPtr);
		}
	        Itcl_PopCallFrame(interp);
            }
            hPtr = Tcl_NextHashEntry(&place);
        }
	/* now check for options which are delegated */
        hPtr = Tcl_FirstHashEntry(&iclsPtr2->delegatedOptions, &place);
        while (hPtr) {
            idoPtr = (ItclDelegatedOption*)Tcl_GetHashValue(hPtr);
	    hPtr2 = Tcl_CreateHashEntry(&ioPtr->objectDelegatedOptions,
	            (char *)idoPtr->namePtr, &isNew);
	    if (isNew) {
		Tcl_SetHashValue(hPtr2, idoPtr);
	    }
            hPtr = Tcl_NextHashEntry(&place);
        }
        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    }
    Itcl_DeleteHierIter(&hier);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclInitObjectMethodVariables()
 *
 *  Collect all instance methdovariables for the given object instance to allow
 *  faster runtime access to the methdovariables.
 *  This is usually invoked automatically by Itcl_CreateObject(),
 *  when an object is created.
 * ------------------------------------------------------------------------
 */
int
ItclInitObjectMethodVariables(
   Tcl_Interp *interp,
   ItclObject *ioPtr,
   ItclClass *iclsPtr,
   const char *name)
{
    ItclClass *iclsPtr2;
    ItclHierIter hier;
    ItclMethodVariable *imvPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashEntry *hPtr2;
    Tcl_HashSearch place;
    int isNew;

    imvPtr = NULL;
    Itcl_InitHierIter(&hier, iclsPtr);
    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    while (iclsPtr2 != NULL) {
        hPtr = Tcl_FirstHashEntry(&iclsPtr2->methodVariables, &place);
        while (hPtr) {
            imvPtr = (ItclMethodVariable*)Tcl_GetHashValue(hPtr);
	    hPtr2 = Tcl_CreateHashEntry(&ioPtr->objectMethodVariables,
	            (char *)imvPtr->namePtr, &isNew);
	    if (isNew) {
		Tcl_SetHashValue(hPtr2, imvPtr);
            }
            hPtr = Tcl_NextHashEntry(&place);
        }
        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
    }
    Itcl_DeleteHierIter(&hier);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_DeleteObject()
 *
 *  Attempts to delete an object by invoking its destructor.
 *
 *  If the destructor is successful, then the object is deleted by
 *  removing its access command, and this procedure returns TCL_OK.
 *  Otherwise, the object will remain alive, and this procedure
 *  returns TCL_ERROR (along with an error message in the interpreter).
 * ------------------------------------------------------------------------
 */
int
Itcl_DeleteObject(
    Tcl_Interp *interp,      /* interpreter mananging object */
    ItclObject *contextIoPtr)  /* object to be deleted */
{
    Tcl_CmdInfo cmdInfo;
    Tcl_HashEntry *hPtr;


    Tcl_GetCommandInfoFromToken(contextIoPtr->accessCmd, &cmdInfo);

    contextIoPtr->flags |= ITCL_OBJECT_IS_DELETED;
    Itcl_PreserveData(contextIoPtr);

    /*
     *  Invoke the object's destructors.
     */
    if (Itcl_DestructObject(interp, contextIoPtr, 0) != TCL_OK) {
	Itcl_ReleaseData(contextIoPtr);
	contextIoPtr->flags |=
	        ITCL_TCLOO_OBJECT_IS_DELETED|ITCL_OBJECT_DESTRUCT_ERROR;
        return TCL_ERROR;
    }
    /*
     *  Remove the object from the global list.
     */
    hPtr = Tcl_FindHashEntry(&contextIoPtr->infoPtr->objects,
        (char*)contextIoPtr);

    if (hPtr) {
        Tcl_DeleteHashEntry(hPtr);
    }

    /*
     *  Change the object's access command so that it can be
     *  safely deleted without attempting to destruct the object
     *  again.  Then delete the access command.  If this is
     *  the last use of the object data, the object will die here.
     */
    if ((contextIoPtr->accessCmd != NULL) && (!(contextIoPtr->flags &
            (ITCL_OBJECT_IS_RENAMED)))) {
    if (Tcl_GetCommandInfoFromToken(contextIoPtr->accessCmd, &cmdInfo) == 1) {
        cmdInfo.deleteProc = (Tcl_CmdDeleteProc *)Itcl_ReleaseData;
	Tcl_SetCommandInfoFromToken(contextIoPtr->accessCmd, &cmdInfo);

        Tcl_DeleteCommandFromToken(interp, contextIoPtr->accessCmd);
    }
    }
    contextIoPtr->oPtr = NULL;
    contextIoPtr->accessCmd = NULL;

    Itcl_ReleaseData(contextIoPtr);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclDeleteObjectVariablesNamespace()
 *
 * ------------------------------------------------------------------------
 */
void
ItclDeleteObjectVariablesNamespace(
    Tcl_Interp *interp,
    ItclObject *ioPtr)
{
    Tcl_Namespace *varNsPtr;

    if (ioPtr->callRefCount < 1) {
        /* free the object's variables namespace and variables in it */
	ioPtr->flags &= ~ITCL_OBJECT_SHOULD_VARNS_DELETE;
        varNsPtr = Tcl_FindNamespace(interp, Tcl_GetString(ioPtr->varNsNamePtr),
	        NULL, 0);
        if (varNsPtr != NULL) {
            Tcl_DeleteNamespace(varNsPtr);
        }
    } else {
        ioPtr->flags |= ITCL_OBJECT_SHOULD_VARNS_DELETE;
    }
}

static int
FinalizeDeleteObject(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    ItclObject *contextIoPtr = (ItclObject *)data[0];
    if (result == TCL_OK) {
	ItclDeleteObjectVariablesNamespace(interp, contextIoPtr);
        Tcl_ResetResult(interp);
    }

    Tcl_DeleteHashTable(contextIoPtr->destructed);
    ckfree((char*)contextIoPtr->destructed);
    contextIoPtr->destructed = NULL;
    return result;
}

static int
CallDestructBase(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Obj *objPtr;
    ItclObject *contextIoPtr = (ItclObject *)data[0];
    int flags = PTR2INT(data[1]);

    if (result != TCL_OK) {
        return result;
    }
    result = ItclDestructBase(interp, contextIoPtr, contextIoPtr->iclsPtr,
            flags);
    if (result != TCL_OK) {
        return result;
    }
    /* destroy the hull */
    if (contextIoPtr->hullWindowNamePtr != NULL) {
        objPtr = Tcl_NewStringObj("destroy ", -1);
        Tcl_AppendToObj(objPtr,
	        Tcl_GetString(contextIoPtr->hullWindowNamePtr), -1);
        result = Tcl_EvalObjEx(interp, objPtr, 0);
    }
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_DestructObject()
 *
 *  Invokes the destructor for a particular object.  Usually invoked
 *  by Itcl_DeleteObject() or Itcl_DestroyObject() as a part of the
 *  object destruction process.  If the ITCL_IGNORE_ERRS flag is
 *  included, all destructors are invoked even if errors are
 *  encountered, and the result will always be TCL_OK.
 *
 *  Returns TCL_OK on success, or TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
Itcl_DestructObject(
    Tcl_Interp *interp,      /* interpreter mananging new object */
    ItclObject *contextIoPtr,  /* object to be destructed */
    int flags)               /* flags: ITCL_IGNORE_ERRS */
{
    int result;

    if ((contextIoPtr->flags & (ITCL_OBJECT_IS_DESTRUCTED))) {
            return TCL_OK;
    }
    contextIoPtr->flags |= ITCL_OBJECT_IS_DESTRUCTED;
    /*
     *  If there is a "destructed" table, then this object is already
     *  being destructed.  Flag an error, unless errors are being
     *  ignored.
     */
    if (contextIoPtr->destructed) {
        if ((flags & ITCL_IGNORE_ERRS) == 0) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "can't delete an object while it is being destructed",
                NULL);
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    result = TCL_OK;
    if (contextIoPtr->oPtr != NULL) {
        void *callbackPtr;
        /*
         *  Create a "destructed" table to keep track of which destructors
         *  have been invoked.  This is used in ItclDestructBase to make
         *  sure that all base class destructors have been called,
         *  explicitly or implicitly.
         */
        contextIoPtr->destructed = (Tcl_HashTable*)ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitObjHashTable(contextIoPtr->destructed);

        /*
         *  Destruct the object starting from the most-specific class.
         *  If all goes well, return the null string as the result.
         */
        callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
        Tcl_NRAddCallback(interp, FinalizeDeleteObject, contextIoPtr,
	        NULL, NULL, NULL);
        Tcl_NRAddCallback(interp, CallDestructBase, contextIoPtr,
	        INT2PTR(flags), NULL, NULL);
        result = Itcl_NRRunCallbacks(interp, callbackPtr);
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclDestructBase()
 *
 *  Invoked by Itcl_DestructObject() to recursively destruct an object
 *  from the specified class level.  Finds and invokes the destructor
 *  for the specified class, and then recursively destructs all base
 *  classes.  If the ITCL_IGNORE_ERRS flag is included, all destructors
 *  are invoked even if errors are encountered, and the result will
 *  always be TCL_OK.
 *
 *  Returns TCL_OK on success, or TCL_ERROR (along with an error message
 *  in interp->result) on error.
 * ------------------------------------------------------------------------
 */
static int
ItclDestructBase(
    Tcl_Interp *interp,         /* interpreter */
    ItclObject *contextIoPtr,   /* object being destructed */
    ItclClass *contextIclsPtr,  /* current class being destructed */
    int flags)                  /* flags: ITCL_IGNORE_ERRS */
{
    int result;
    Itcl_ListElem *elem;
    ItclClass *iclsPtr;

    if (contextIoPtr->flags & ITCL_OBJECT_CLASS_DESTRUCTED) {
        return TCL_OK;
    }
    /*
     *  Look for a destructor in this class, and if found,
     *  invoke it.
     */
    if (Tcl_FindHashEntry(contextIoPtr->destructed,
            (char *)contextIclsPtr->namePtr) == NULL) {
        result = Itcl_InvokeMethodIfExists(interp, "destructor",
            contextIclsPtr, contextIoPtr, 0, NULL);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }

    /*
     *  Scan through the list of base classes recursively and destruct
     *  them.  Traverse the list in normal order, so that we destruct
     *  from most- to least-specific.
     */
    elem = Itcl_FirstListElem(&contextIclsPtr->bases);
    while (elem) {
        iclsPtr = (ItclClass*)Itcl_GetListValue(elem);

        if (ItclDestructBase(interp, contextIoPtr, iclsPtr, flags) != TCL_OK) {
            return TCL_ERROR;
        }
        elem = Itcl_NextListElem(elem);
    }

    /*
     *  Throw away any result from the destructors and return.
     */
    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_FindObject()
 *
 *  Searches for an object with the specified name, which have
 *  namespace scope qualifiers like "namesp::namesp::name", or may
 *  be a scoped value such as "namespace inscope ::foo obj".
 *
 *  If an error is encountered, this procedure returns TCL_ERROR
 *  along with an error message in the interpreter.  Otherwise, it
 *  returns TCL_OK.  If an object was found, "roPtr" returns a
 *  pointer to the object data.  Otherwise, it returns NULL.
 * ------------------------------------------------------------------------
 */
int
Itcl_FindObject(
    Tcl_Interp *interp,      /* interpreter containing this object */
    const char *name,        /* name of the object */
    ItclObject **roPtr)      /* returns: object data or NULL */
{
    Tcl_Command cmd;
    Tcl_CmdInfo cmdInfo;
    Tcl_Namespace *contextNs;
    char *cmdName;

    contextNs = NULL;
    cmdName = NULL;
    /*
     *  The object name may be a scoped value of the form
     *  "namespace inscope <namesp> <command>".  If it is,
     *  decode it.
     */
    if (Itcl_DecodeScopedCommand(interp, name, &contextNs, &cmdName)
            != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     *  Look for the object's access command, and see if it has
     *  the appropriate command handler.
     */
    cmd = Tcl_FindCommand(interp, cmdName, contextNs, /* flags */ 0);
    if (cmd != NULL && Itcl_IsObject(cmd)) {
        if (Tcl_GetCommandInfoFromToken(cmd, &cmdInfo) != 1) {
            *roPtr = NULL;
        }
        *roPtr = (ItclObject *)cmdInfo.deleteData;
    } else {
        *roPtr = NULL;
    }

    ckfree(cmdName);

    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_IsObject()
 *
 *  Checks the given Tcl command to see if it represents an itcl object.
 *  Returns non-zero if the command is associated with an object.
 * ------------------------------------------------------------------------
 */
int
Itcl_IsObject(
    Tcl_Command cmd)         /* command being tested */
{
    Tcl_CmdInfo cmdInfo;

    if (Tcl_GetCommandInfoFromToken(cmd, &cmdInfo) != 1) {
        return 0;
    }

    if ((void *)cmdInfo.deleteProc == (void *)ItclDestroyObject) {
        return 1;
    }

    /*
     *  This may be an imported command.  Try to get the real
     *  command and see if it represents an object.
     */
    cmd = Tcl_GetOriginalCommand(cmd);
    if (cmd != NULL) {
        if (Tcl_GetCommandInfoFromToken(cmd, &cmdInfo) != 1) {
            return 0;
        }

        if ((void *)cmdInfo.deleteProc == (void *)ItclDestroyObject) {
            return 1;
        }
    }
    return 0;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ObjectIsa()
 *
 *  Checks to see if an object belongs to the given class.  An object
 *  "is-a" member of the class if the class appears anywhere in its
 *  inheritance hierarchy.  Returns non-zero if the object belongs to
 *  the class, and zero otherwise.
 * ------------------------------------------------------------------------
 */
int
Itcl_ObjectIsa(
    ItclObject *contextIoPtr, /* object being tested */
    ItclClass *iclsPtr)       /* class to test for "is-a" relationship */
{
    Tcl_HashEntry *entry;

    if (contextIoPtr == NULL) {
        return 0;
    }
    entry = Tcl_FindHashEntry(&contextIoPtr->iclsPtr->heritage, (char*)iclsPtr);
    return (entry != NULL);
}

/*
 * ------------------------------------------------------------------------
 *  ItclGetInstanceVar()
 *
 *  Returns the current value for an object data member.  The member
 *  name is interpreted with respect to the given class scope, which
 *  is usually the most-specific class for the object.
 *
 *  If successful, this procedure returns a pointer to a string value
 *  which remains alive until the variable changes it value.  If
 *  anything goes wrong, this returns NULL.
 * ------------------------------------------------------------------------
 */
const char*
ItclGetInstanceVar(
    Tcl_Interp *interp,        /* current interpreter */
    const char *name1,         /* name of desired instance variable */
    const char *name2,         /* array element or NULL */
    ItclObject *contextIoPtr,  /* current object */
    ItclClass *contextIclsPtr) /* name is interpreted in this scope */
{
    Tcl_HashEntry *hPtr;
    Tcl_CallFrame frame;
    Tcl_CallFrame *framePtr;
    Tcl_Namespace *nsPtr;
    Tcl_DString buffer;
    ItclClass *iclsPtr;
    ItclVariable *ivPtr;
    ItclVarLookup *vlookup;
    const char *val;
    int isItclOptions;
    int doAppend;

    /*
     *  Make sure that the current namespace context includes an
     *  object that is being manipulated.
     */
    if (contextIoPtr == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "cannot access object-specific info without an object context",
            NULL);
        return NULL;
    }

    /* get the variable definition to check if that is an ITCL_COMMON */
    if (contextIclsPtr == NULL) {
        iclsPtr = contextIoPtr->iclsPtr;
    } else {
        iclsPtr = contextIclsPtr;
    }
    ivPtr = NULL;
    hPtr = ItclResolveVarEntry(iclsPtr, (char *)name1);
    if (hPtr != NULL) {
        vlookup = (ItclVarLookup *)Tcl_GetHashValue(hPtr);
        ivPtr = vlookup->ivPtr;
    /*
     *  Install the object context and access the data member
     *  like any other variable.
     */
    hPtr = Tcl_FindHashEntry(&contextIoPtr->objectVariables, (char *)ivPtr);
    if (hPtr) {
	Tcl_Obj *varName = Tcl_NewObj();
	Tcl_Var varPtr = (Tcl_Var)Tcl_GetHashValue(hPtr);
	Tcl_GetVariableFullName(interp, varPtr, varName);

	val = Tcl_GetVar2(interp, Tcl_GetString(varName), name2,
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(varName);
	if (val) {
	    return val;
	}
    }
    }

    isItclOptions = 0;
    if (strcmp(name1, "itcl_options") == 0) {
        isItclOptions = 1;
    }
    if (strcmp(name1, "itcl_option_components") == 0) {
        isItclOptions = 1;
    }
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, Tcl_GetString(contextIoPtr->varNsNamePtr), -1);
    doAppend = 1;
    if ((contextIclsPtr == NULL) || (contextIclsPtr->flags &
            (ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
        if (isItclOptions) {
	    doAppend = 0;
        }
    }
    if ((ivPtr != NULL) && (ivPtr->flags & ITCL_COMMON)) {
	if (!isItclOptions) {
            Tcl_DStringSetLength(&buffer, 0);
	    if (ivPtr->protection != ITCL_PUBLIC) {
	        Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	    }
	    doAppend = 1;
        }
    }
    if (doAppend) {
        Tcl_DStringAppend(&buffer, (Tcl_GetObjectNamespace(
		contextIclsPtr->oPtr))->fullName, -1);
    }
    nsPtr = Tcl_FindNamespace(interp, Tcl_DStringValue(&buffer), NULL, 0);
    Tcl_DStringFree(&buffer);
    val = NULL;
    if (nsPtr != NULL) {
	framePtr = &frame;
	Itcl_PushCallFrame(interp, framePtr, nsPtr, /*isProcCallFrame*/0);
        val = Tcl_GetVar2(interp, (const char *)name1, (char*)name2,
	        TCL_LEAVE_ERR_MSG);
        Itcl_PopCallFrame(interp);
    }

    return val;
}

/*
 * ------------------------------------------------------------------------
 *  ItclGetCommonInstanceVar()
 *
 *  Returns the current value for an object data member.  The member
 *  name is interpreted with respect to the given class scope, which
 *  is usually the most-specific class for the object.
 *
 *  If successful, this procedure returns a pointer to a string value
 *  which remains alive until the variable changes it value.  If
 *  anything goes wrong, this returns NULL.
 * ------------------------------------------------------------------------
 */
const char*
ItclGetCommonInstanceVar(
    Tcl_Interp *interp,        /* current interpreter */
    const char *name1,         /* name of desired instance variable */
    const char *name2,         /* array element or NULL */
    ItclObject *contextIoPtr,  /* current object */
    ItclClass *contextIclsPtr) /* name is interpreted in this scope */
{
    Tcl_CallFrame frame;
    Tcl_CallFrame *framePtr;
    Tcl_Namespace *nsPtr;
    Tcl_DString buffer;
    const char *val;
    int doAppend;

    /*
     *  Make sure that the current namespace context includes an
     *  object that is being manipulated.
     */
    if (contextIoPtr == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "cannot access object-specific info without an object context",
            NULL);
        return NULL;
    }

    /*
     *  Install the object context and access the data member
     *  like any other variable.
     */
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
    doAppend = 1;
    if ((contextIclsPtr == NULL) || (contextIclsPtr->flags &
            (ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGETADAPTOR))) {
        if (strcmp(name1, "itcl_options") == 0) {
	    doAppend = 0;
        }
        if (strcmp(name1, "itcl_option_components") == 0) {
	    doAppend = 0;
        }
    }
    if (doAppend) {
        Tcl_DStringAppend(&buffer, (Tcl_GetObjectNamespace(
		contextIclsPtr->oPtr))->fullName, -1);
    }
    nsPtr = Tcl_FindNamespace(interp, Tcl_DStringValue(&buffer), NULL, 0);
    Tcl_DStringFree(&buffer);
    val = NULL;
    if (nsPtr != NULL) {
	framePtr = &frame;
	Itcl_PushCallFrame(interp, framePtr, nsPtr, /*isProcCallFrame*/0);
        val = Tcl_GetVar2(interp, (const char *)name1, (char*)name2,
	        TCL_LEAVE_ERR_MSG);
        Itcl_PopCallFrame(interp);
    }

    return val;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_GetInstanceVar()
 *
 *  Returns the current value for an object data member.  The member
 *  name is interpreted with respect to the given class scope, which
 *  is usually the most-specific class for the object.
 *
 *  If successful, this procedure returns a pointer to a string value
 *  which remains alive until the variable changes it value.  If
 *  anything goes wrong, this returns NULL.
 * ------------------------------------------------------------------------
 */
const char*
Itcl_GetInstanceVar(
    Tcl_Interp *interp,        /* current interpreter */
    const char *name,          /* name of desired instance variable */
    ItclObject *contextIoPtr,  /* current object */
    ItclClass *contextIclsPtr) /* name is interpreted in this scope */
{
    return ItclGetInstanceVar(interp, name, NULL, contextIoPtr,
            contextIclsPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclSetInstanceVar()
 *
 *  Sets the current value for an object data member.  The member
 *  name is interpreted with respect to the given class scope, which
 *  is usually the most-specific class for the object.
 *
 *  If successful, this procedure returns a pointer to a string value
 *  which remains alive until the variable changes it value.  If
 *  anything goes wrong, this returns NULL.
 * ------------------------------------------------------------------------
 */
const char*
ItclSetInstanceVar(
    Tcl_Interp *interp,        /* current interpreter */
    const char *name1,         /* name of desired instance variable */
    const char *name2,         /* array member or NULL */
    const char *value,         /* the value to set */
    ItclObject *contextIoPtr,  /* current object */
    ItclClass *contextIclsPtr) /* name is interpreted in this scope */
{
    Tcl_HashEntry *hPtr;
    Tcl_CallFrame frame;
    Tcl_CallFrame *framePtr;
    Tcl_Namespace *nsPtr;
    Tcl_DString buffer;
    ItclVariable *ivPtr;
    ItclVarLookup *vlookup;
    ItclClass *iclsPtr;
    const char *val;
    int isItclOptions;
    int doAppend;

    ivPtr = NULL;
    /*
     *  Make sure that the current namespace context includes an
     *  object that is being manipulated.
     */
    if (contextIoPtr == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "cannot access object-specific info without an object context",
            NULL);
        return NULL;
    }
    /* get the variable definition to check if that is an ITCL_COMMON */
    if (contextIclsPtr == NULL) {
        iclsPtr = contextIoPtr->iclsPtr;
    } else {
        iclsPtr = contextIclsPtr;
    }
    hPtr = ItclResolveVarEntry(iclsPtr, (char *)name1);
    if (hPtr != NULL) {
        vlookup = (ItclVarLookup *)Tcl_GetHashValue(hPtr);
        ivPtr = vlookup->ivPtr;
    } else {
        return NULL;
    }
    /*
     *  Install the object context and access the data member
     *  like any other variable.
     */

    hPtr = Tcl_FindHashEntry(&contextIoPtr->objectVariables, (char *)ivPtr);
    if (hPtr) {
	Tcl_Obj *varName = Tcl_NewObj();
	Tcl_Var varPtr = (Tcl_Var)Tcl_GetHashValue(hPtr);
	Tcl_GetVariableFullName(interp, varPtr, varName);

	val = Tcl_SetVar2(interp, Tcl_GetString(varName), name2, value,
		TCL_LEAVE_ERR_MSG);
	Tcl_DecrRefCount(varName);
	return val;
    }

    isItclOptions = 0;
    if (strcmp(name1, "itcl_options") == 0) {
        isItclOptions = 1;
    }
    if (strcmp(name1, "itcl_option_components") == 0) {
        isItclOptions = 1;
    }
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, Tcl_GetString(contextIoPtr->varNsNamePtr), -1);
    doAppend = 1;
    if ((contextIclsPtr == NULL) ||
            (contextIclsPtr->flags & (ITCL_ECLASS|ITCL_TYPE|
	    ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
        if (isItclOptions) {
	    doAppend = 0;
        }
    }
    if (ivPtr->flags & ITCL_COMMON) {
	if (!isItclOptions) {
            Tcl_DStringSetLength(&buffer, 0);
	    if (ivPtr->protection != ITCL_PUBLIC) {
	        Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	    }
	    doAppend = 1;
        }
    }
    if (doAppend) {
        Tcl_DStringAppend(&buffer, (Tcl_GetObjectNamespace(
		contextIclsPtr->oPtr))->fullName, -1);
    }
    nsPtr = Tcl_FindNamespace(interp, Tcl_DStringValue(&buffer), NULL, 0);
    Tcl_DStringFree(&buffer);
    val = NULL;
    if (nsPtr != NULL) {
	framePtr = &frame;
	Itcl_PushCallFrame(interp, framePtr, nsPtr, /*isProcCallFrame*/0);
        val = Tcl_SetVar2(interp, (const char *)name1, (char*)name2,
	        value, TCL_LEAVE_ERR_MSG);
        Itcl_PopCallFrame(interp);
    }

    return val;
}

/*
 * ------------------------------------------------------------------------
 *  ItclReportObjectUsage()
 *
 *  Appends information to the given interp summarizing the usage
 *  for all of the methods available for this object.  Useful when
 *  reporting errors in Itcl_HandleInstance().
 * ------------------------------------------------------------------------
 */
void
ItclReportObjectUsage(
    Tcl_Interp *interp,           /* current interpreter */
    ItclObject *contextIoPtr,     /* current object */
    Tcl_Namespace *callerNsPtr,
    Tcl_Namespace *contextNsPtr)  /* the context namespace */
{
    Tcl_Obj *namePtr;
    Tcl_HashEntry *entry;
    Tcl_HashSearch place;
    Tcl_Obj *resultPtr;
    ItclClass *iclsPtr = NULL;
    Itcl_List cmdList;
    Itcl_ListElem *elem;
    ItclMemberFunc *imPtr;
    ItclMemberFunc *cmpFunc;
    ItclCmdLookup *clookup;
    ItclObjectInfo * infoPtr = NULL;
    char *name;
    int ignore;
    int cmp;

    if (contextIoPtr == NULL) {
        resultPtr = Tcl_GetObjResult(interp);
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
        if (infoPtr == NULL) {
            Tcl_AppendResult(interp, " PANIC cannot get Itcl AssocData in ItclReportObjectUsage", NULL);
	    return;
        }
	if (contextNsPtr == NULL) {
            Tcl_AppendResult(interp, " PANIC cannot get contextNsPtr in ItclReportObjectUsage", NULL);
	    return;
	}

	entry = Tcl_FindHashEntry(&infoPtr->namespaceClasses,
		(char *)contextNsPtr);
	if (entry) {
	    iclsPtr = (ItclClass *)Tcl_GetHashValue(entry);
	}

        if (iclsPtr == NULL) {
          Tcl_AppendResult(interp, " PANIC cannot get class from contextNsPtr ItclReportObjectUsage", NULL);
          return;
        }
    } else {
        iclsPtr = (ItclClass*)contextIoPtr->iclsPtr;
    }
    ignore = ITCL_CONSTRUCTOR | ITCL_DESTRUCTOR | ITCL_COMMON;
    /*
     *  Scan through all methods in the virtual table and sort
     *  them in alphabetical order.  Report only the methods
     *  that have simple names (no ::'s) and are accessible.
     */
    Itcl_InitList(&cmdList);
    entry = Tcl_FirstHashEntry(&iclsPtr->resolveCmds, &place);
    while (entry) {
        namePtr  = (Tcl_Obj *)Tcl_GetHashKey(&iclsPtr->resolveCmds, entry);
	name = Tcl_GetString(namePtr);
	clookup = (ItclCmdLookup *)Tcl_GetHashValue(entry);
	imPtr = clookup->imPtr;

        if (strstr(name,"::") || (imPtr->flags & ignore) != 0) {
            imPtr = NULL;
        } else {
	    if (imPtr->protection != ITCL_PUBLIC) {
		if (contextNsPtr != NULL) {
                    if (!Itcl_CanAccessFunc(imPtr, contextNsPtr)) {
                        imPtr = NULL;
                    }
                }
	    }
        }
        if ((imPtr != NULL) && (imPtr->codePtr != NULL)) {
	    if (imPtr->codePtr->flags & ITCL_BUILTIN) {
	        char *body;
	        if (imPtr->codePtr != NULL) {
	            body = Tcl_GetString(imPtr->codePtr->bodyPtr);
	            if (*body == '@') {
                        if (strcmp(body, "@itcl-builtin-setget") == 0) {
			    if (!(imPtr->iclsPtr->flags & ITCL_ECLASS)) {
	                        imPtr = NULL;
			    }
		        }
                        if (strcmp(body, "@itcl-builtin-installcomponent")
			        == 0) {
			    if (!(imPtr->iclsPtr->flags &
			            (ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
	                        imPtr = NULL;
			    }
		        }
	            }
	        }
	    }
	}

        if (imPtr) {
            elem = Itcl_FirstListElem(&cmdList);
            while (elem) {
                cmpFunc = (ItclMemberFunc*)Itcl_GetListValue(elem);
                cmp = strcmp(Tcl_GetString(imPtr->namePtr),
		        Tcl_GetString(cmpFunc->namePtr));
                if (cmp < 0) {
                    Itcl_InsertListElem(elem, imPtr);
                    imPtr = NULL;
                    break;
                } else {
		    if (cmp == 0) {
                        imPtr = NULL;
                        break;
		    }
                }
                elem = Itcl_NextListElem(elem);
            }
            if (imPtr) {
                Itcl_AppendList(&cmdList, imPtr);
            }
        }
        entry = Tcl_NextHashEntry(&place);
    }

    /*
     *  Add a series of statements showing usage info.
     */
    resultPtr = Tcl_GetObjResult(interp);
    elem = Itcl_FirstListElem(&cmdList);
    while (elem) {
        imPtr = (ItclMemberFunc*)Itcl_GetListValue(elem);
        Tcl_AppendToObj(resultPtr, "\n  ", -1);
        Itcl_GetMemberFuncUsage(imPtr, contextIoPtr, resultPtr);

        elem = Itcl_NextListElem(elem);
    }
    Itcl_DeleteList(&cmdList);
}

/*
 * ------------------------------------------------------------------------
 *  ItclTraceThisVar()
 *
 *  Invoked to handle read/write traces on the "this" variable built
 *  into each object.
 *
 *  On read, this procedure updates the "this" variable to contain the
 *  current object name.  This is done dynamically, since an object's
 *  identity can change if its access command is renamed.
 *
 *  On write, this procedure returns an error string, warning that
 *  the "this" variable cannot be set.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceThisVar(
    ClientData cdata,	  /* object instance data */
    Tcl_Interp *interp,	  /* interpreter managing this variable */
    const char *name1,    /* variable name */
    const char *name2,    /* unused */
    int flags)		    /* flags indicating read/write */
{
    ItclObject *contextIoPtr = (ItclObject*)cdata;
    Tcl_Obj *objPtr;
    const char *objName;

    /* because of SF bug #187 use a different trace handler for "this", "win", "type"
     * *self" and "selfns"
     */

    /*
     *  Handle read traces on "this"
     */
    if ((flags & TCL_TRACE_READS) != 0) {
        objPtr = Tcl_NewStringObj("", -1);
        if (contextIoPtr->accessCmd) {
            Tcl_GetCommandFullName(contextIoPtr->iclsPtr->interp,
                contextIoPtr->accessCmd, objPtr);
        }
        objName = Tcl_GetString(objPtr);
        Tcl_SetVar2(interp, name1, NULL, objName, 0);

        Tcl_DecrRefCount(objPtr);
        return NULL;
    }

    /*
     *  Handle write traces on "this"
     */
    if ((flags & TCL_TRACE_WRITES) != 0) {
        return (char *)"variable \"this\" cannot be modified";
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  ItclTraceWinVar()
 *
 *  Invoked to handle read/write traces on the "win" variable built
 *  into each object.
 *
 *  On read, this procedure updates the "win" variable to contain the
 *  current object name.  This is done dynamically, since an object's
 *  identity can change if its access command is renamed.
 *
 *  On write, this procedure returns an error string, warning that
 *  the "win" variable cannot be set.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceWinVar(
    ClientData cdata,	  /* object instance data */
    Tcl_Interp *interp,	  /* interpreter managing this variable */
    const char *name1,    /* variable name */
    const char *name2,    /* unused */
    int flags)		    /* flags indicating read/write */
{
    ItclObject *contextIoPtr = (ItclObject*)cdata;
    Tcl_DString buffer;
    Tcl_Obj *objPtr;
    const char *objName;
    const char *head;
    const char *tail;

    /*
     *  Handle read traces on "win"
     */
    if ((flags & TCL_TRACE_READS) != 0) {
        objPtr = Tcl_NewStringObj("", -1);
        /* a window path name must not contain namespace parts !! */
        Itcl_ParseNamespPath(Tcl_GetString(contextIoPtr->origNamePtr), &buffer, &head, &tail);
	if (tail == NULL) {
	    return (char *)" INTERNAL ERROR tail == NULL in ItclTraceThisVar for win";
	}
        Tcl_SetStringObj(objPtr, tail, -1);
        objName = Tcl_GetString(objPtr);
        Tcl_SetVar2(interp, name1, NULL, objName, 0);

        Tcl_DecrRefCount(objPtr);
        return NULL;
    }

    /*
     *  Handle write traces on "win"
     */
    if ((flags & TCL_TRACE_WRITES) != 0) {
        if (!(contextIoPtr->iclsPtr->flags & ITCL_ECLASS)) {
            return (char *)"variable \"win\" cannot be modified";
        }
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  ItclTraceTypeVar()
 *
 *  Invoked to handle read/write traces on the "type" variable built
 *  into each object.
 *
 *  On read, this procedure updates the "type" variable to contain the
 *  current object name.  This is done dynamically, since an object's
 *  identity can change if its access command is renamed.
 *
 *  On write, this procedure returns an error string, warning that
 *  the "type" variable cannot be set.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceTypeVar(
    ClientData cdata,	  /* object instance data */
    Tcl_Interp *interp,	  /* interpreter managing this variable */
    const char *name1,    /* variable name */
    const char *name2,    /* unused */
    int flags)		    /* flags indicating read/write */
{
    ItclObject *contextIoPtr = (ItclObject*)cdata;
    Tcl_Obj *objPtr;
    const char *objName;

    /*
     *  Handle read traces on "type"
     */
    if ((flags & TCL_TRACE_READS) != 0) {
        objPtr = Tcl_NewStringObj("", -1);
        Tcl_SetStringObj(objPtr,
        Tcl_GetCurrentNamespace(contextIoPtr->iclsPtr->interp)->fullName, -1);
        objName = Tcl_GetString(objPtr);
        Tcl_SetVar2(interp, name1, NULL, objName, 0);

        Tcl_DecrRefCount(objPtr);
        return NULL;
    }

    /*
     *  Handle write traces on "type"
     */
    if ((flags & TCL_TRACE_WRITES) != 0) {
        return (char *)"variable \"type\" cannot be modified";
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  ItclTraceSelfVar()
 *
 *  Invoked to handle read/write traces on the "self" variable built
 *  into each object.
 *
 *  On read, this procedure updates the "self" variable to contain the
 *  current object name.  This is done dynamically, since an object's
 *  identity can change if its access command is renamed.
 *
 *  On write, this procedure returns an error string, warning that
 *  the "self" variable cannot be set.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceSelfVar(
    ClientData cdata,	  /* object instance data */
    Tcl_Interp *interp,	  /* interpreter managing this variable */
    const char *name1,    /* variable name */
    const char *name2,    /* unused */
    int flags)		    /* flags indicating read/write */
{
    ItclObject *contextIoPtr = (ItclObject*)cdata;
    Tcl_Obj *objPtr;
    const char *objName;

    /*
     *  Handle read traces on "self"
     */
    if ((flags & TCL_TRACE_READS) != 0) {
        objPtr = Tcl_NewStringObj("", -1);
        if (contextIoPtr->iclsPtr->flags &
                (ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
            const char *objectName;

            objectName = ItclGetInstanceVar(
                    contextIoPtr->iclsPtr->interp,
                    "itcl_hull", NULL, contextIoPtr,
                    contextIoPtr->iclsPtr);
            if (strlen(objectName) == 0) {
	        objPtr = contextIoPtr->namePtr;
	        Tcl_IncrRefCount(objPtr);
            } else {
                Tcl_SetStringObj(objPtr, objectName, -1);
            }
        } else {
            Tcl_GetCommandFullName(contextIoPtr->iclsPtr->interp,
                    contextIoPtr->accessCmd, objPtr);
        }
        objName = Tcl_GetString(objPtr);
        Tcl_SetVar2(interp, name1, NULL, objName, 0);

        Tcl_DecrRefCount(objPtr);
        return NULL;
    }

    /*
     *  Handle write traces on "self"
     */
    if ((flags & TCL_TRACE_WRITES) != 0) {
        return (char *)"variable \"self\" cannot be modified";
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  ItclTraceSelfnsVar()
 *
 *  Invoked to handle read/write traces on the "selfns" variable built
 *  into each object.
 *
 *  On read, this procedure updates the "selfns" variable to contain the
 *  current object name.  This is done dynamically, since an object's
 *  identity can change if its access command is renamed.
 *
 *  On write, this procedure returns an error string, warning that
 *  the "selfns" variable cannot be set.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceSelfnsVar(
    ClientData cdata,	  /* object instance data */
    Tcl_Interp *interp,	  /* interpreter managing this variable */
    const char *name1,    /* variable name */
    const char *name2,    /* unused */
    int flags)	          /* flags indicating read/write */
{
    ItclObject *contextIoPtr = (ItclObject*)cdata;
    Tcl_Obj *objPtr;
    const char *objName;

    /*
     *  Handle read traces on "selfns"
     */
    if ((flags & TCL_TRACE_READS) != 0) {
        objPtr = Tcl_NewStringObj("", -1);
        Tcl_SetStringObj(objPtr, Tcl_GetString(contextIoPtr->varNsNamePtr), -1);
        Tcl_AppendToObj(objPtr,
                Tcl_GetString(contextIoPtr->iclsPtr->fullNamePtr), -1);
        objName = Tcl_GetString(objPtr);
        Tcl_SetVar2(interp, name1, NULL, objName, 0);

        Tcl_DecrRefCount(objPtr);
        return NULL;
    }

    /*
     *  Handle write traces on "selfns"
     */
    if ((flags & TCL_TRACE_WRITES) != 0) {
        return (char *)"variable \"selfns\" cannot be modified";
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  ItclTraceOptionVar()
 *
 *  Invoked to handle read/write traces on "option" variables
 *
 *  On read, this procedure checks if there is a cgetMethodPtr and calls it
 *  On write, this procedure checks if there is a configureMethodPtr
 *  or validateMethodPtr and calls it
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceOptionVar(
    ClientData cdata,	    /* object instance data */
    Tcl_Interp *interp,	    /* interpreter managing this variable */
    const char *name1,      /* variable name */
    const char *name2,      /* unused */
    int flags)		    /* flags indicating read/write */
{
    ItclObject *ioPtr;
    ItclOption *ioptPtr;

/* FIXME !!! */
/* don't know yet if ItclTraceOptionVar is really needed !! */
/* FIXME should free memory on unset or rename!! */
    if (cdata != NULL) {
        ioPtr = (ItclObject*)cdata;
	if (ioPtr == NULL) {
	}
    } else {
        ioptPtr = (ItclOption*)cdata;
	if (ioptPtr == NULL) {
	}
        /*
         *  Handle read traces "itcl_options"
         */
        if ((flags & TCL_TRACE_READS) != 0) {
            return NULL;
        }

        /*
         *  Handle write traces "itcl_options"
         */
        if ((flags & TCL_TRACE_WRITES) != 0) {
            return NULL;
        }
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  ItclTraceComponentVar()
 *
 *  Invoked to handle read/write traces on "component" variables
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceComponentVar(
    ClientData cdata,	    /* object instance data */
    Tcl_Interp *interp,	    /* interpreter managing this variable */
    const char *name1,      /* variable name */
    const char *name2,      /* unused */
    int flags)		    /* flags indicating read/write */
{
    FOREACH_HASH_DECLS;
    Tcl_HashEntry *hPtr2;
    Tcl_Obj *objPtr;
    Tcl_Obj *namePtr;
    Tcl_Obj *componentValuePtr;
    ItclObjectInfo *infoPtr;
    ItclObject *ioPtr;
    ItclComponent *icPtr;
    ItclDelegatedFunction *idmPtr;
    const char *val;

/* FIXME should free memory on unset or rename!! */
    if (cdata != NULL) {
        ioPtr = (ItclObject*)cdata;
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
        hPtr = Tcl_FindHashEntry(&infoPtr->objects, (char *)ioPtr);
	if (hPtr == NULL) {
	    /* object does no longer exist or is being destructed */
	    return NULL;
	}
        objPtr = Tcl_NewStringObj(name1, -1);
	hPtr = Tcl_FindHashEntry(&ioPtr->objectComponents, (char *)objPtr);
        Tcl_DecrRefCount(objPtr);

        /*
         *  Handle write traces
         */
        if ((flags & TCL_TRACE_WRITES) != 0) {
	    if (ioPtr->noComponentTrace) {
	        return NULL;
	    }
            /* need to redo the delegation for this component !! */
            if (hPtr == NULL) {
                return (char *)" INTERNAL ERROR cannot get component to write to";
            }
            icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
	    val = ItclGetInstanceVar(interp, name1, NULL, ioPtr,
                    ioPtr->iclsPtr);
	    if ((val == NULL) || (strlen(val) == 0)) {
	        return (char *)" INTERNAL ERROR cannot get value for component";
	    }
	    componentValuePtr = Tcl_NewStringObj(val, -1);
            Tcl_IncrRefCount(componentValuePtr);
	    namePtr = Tcl_NewStringObj(name1, -1);
            FOREACH_HASH_VALUE(idmPtr, &ioPtr->iclsPtr->delegatedFunctions) {
                if (idmPtr->icPtr == icPtr) {
		    hPtr2 = Tcl_FindHashEntry(&idmPtr->exceptions,
		            (char *)namePtr);
                    if (hPtr2 == NULL) {
                        DelegateFunction(interp, ioPtr, ioPtr->iclsPtr,
                                  componentValuePtr, idmPtr);
		     }
	        }
	    }
	    Tcl_DecrRefCount(componentValuePtr);
	    Tcl_DecrRefCount(namePtr);
            return NULL;
        }
        /*
         *  Handle read traces
         */
        if ((flags & TCL_TRACE_READS) != 0) {
        }

    } else {
        icPtr = (ItclComponent *)cdata;
        /*
         *  Handle read traces
         */
        if ((flags & TCL_TRACE_READS) != 0) {
            return NULL;
        }

        /*
         *  Handle write traces
         */
        if ((flags & TCL_TRACE_WRITES) != 0) {
            return NULL;
        }
    }
    return NULL;
}
/*
 * ------------------------------------------------------------------------
 *  ItclTraceItclHullVar()
 *
 *  Invoked to handle read/write traces on "itcl_hull" variables
 *
 *  On write, this procedure returns an error as "itcl_hull" may not be modfied
 *  after the first initialization
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static char*
ItclTraceItclHullVar(
    ClientData cdata,	    /* object instance data */
    Tcl_Interp *interp,	    /* interpreter managing this variable */
    const char *name1,      /* variable name */
    const char *name2,      /* unused */
    int flags)		    /* flags indicating read/write */
{
    Tcl_HashEntry *hPtr;
    Tcl_Obj *objPtr;
    ItclObjectInfo *infoPtr;
    ItclObject *ioPtr;
    ItclVariable *ivPtr;

/* FIXME !!! */
/* FIXME should free memory on unset or rename!! */
    if (cdata != NULL) {
        ioPtr = (ItclObject*)cdata;
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
        hPtr = Tcl_FindHashEntry(&infoPtr->objects, (char *)ioPtr);
	if (hPtr == NULL) {
	    /* object does no longer exist or is being destructed */
	    return NULL;
	}
        objPtr = Tcl_NewStringObj(name1, -1);
	hPtr = Tcl_FindHashEntry(&ioPtr->iclsPtr->variables, (char *)objPtr);
        Tcl_DecrRefCount(objPtr);
	if (hPtr == NULL) {
	    return (char *)"INTERNAL ERROR cannot find itcl_hull variable in class definition!!";
	}
	ivPtr = (ItclVariable *)Tcl_GetHashValue(hPtr);
        /*
         *  Handle write traces
         */
        if ((flags & TCL_TRACE_WRITES) != 0) {
	    if (ivPtr->initted == 0) {
		ivPtr->initted = 1;
                return NULL;
	    } else {
	        return (char *)"The itcl_hull component cannot be redefined";
	    }
        }

    } else {
        ivPtr = (ItclVariable *)cdata;
        /*
         *  Handle read traces
         */
        if ((flags & TCL_TRACE_READS) != 0) {
            return NULL;
        }

        /*
         *  Handle write traces
         */
        if ((flags & TCL_TRACE_WRITES) != 0) {
            return NULL;
        }
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  ItclDestroyObject()
 *
 *  Invoked when the object access command is deleted to implicitly
 *  destroy the object.  Invokes the object's destructors, ignoring
 *  any errors encountered along the way.  Removes the object from
 *  the list of all known objects and releases the access command's
 *  claim to the object data.
 *
 *  Note that the usual way to delete an object is via Itcl_DeleteObject().
 *  This procedure is provided as a back-up, to handle the case when
 *  an object is deleted by removing its access command.
 * ------------------------------------------------------------------------
 */
static void
ItclDestroyObject(
    ClientData cdata)  /* object instance data */
{
    ItclObject *contextIoPtr = (ItclObject*)cdata;
    Tcl_HashEntry *hPtr;
    Itcl_InterpState istate;

    if (contextIoPtr->flags & ITCL_OBJECT_IS_DESTROYED) {
        return;
    }
    contextIoPtr->flags |= ITCL_OBJECT_IS_DESTROYED;

    if (!(contextIoPtr->flags & ITCL_OBJECT_IS_DESTRUCTED)) {
        /*
         *  Attempt to destruct the object, but ignore any errors.
         */
        istate = Itcl_SaveInterpState(contextIoPtr->interp, 0);
        Itcl_DestructObject(contextIoPtr->interp, contextIoPtr,
                ITCL_IGNORE_ERRS);
        Itcl_RestoreInterpState(contextIoPtr->interp, istate);
    }

    /*
     *  Now, remove the object from the global object list.
     *  We're careful to do this here, after calling the destructors.
     *  Once the access command is nulled out, the "this" variable
     *  won't work properly.
     */
    if (contextIoPtr->accessCmd != NULL) {
        hPtr = Tcl_FindHashEntry(&contextIoPtr->infoPtr->objects,
            (char*)contextIoPtr);

        if (hPtr) {
            Tcl_DeleteHashEntry(hPtr);
        }
        contextIoPtr->accessCmd = NULL;
    }
    Itcl_ReleaseData(contextIoPtr);
}

/*
 * ------------------------------------------------------------------------
 *  FreeObject()
 *
 *  Deletes all instance variables and frees all memory associated with
 *  the given object instance.  Called when releases match preserves.
 * ------------------------------------------------------------------------
 */
static void
FreeObject(
    char * cdata)  /* object instance data */
{
    FOREACH_HASH_DECLS;
    Tcl_HashSearch place;
    ItclCallContext *callContextPtr;
    ItclObject *ioPtr;
    Tcl_Var var;

    ioPtr = (ItclObject*)cdata;

    /*
     *  Install the class namespace and object context so that
     *  the object's data members can be destroyed via simple
     *  "unset" commands.  This makes sure that traces work properly
     *  and all memory gets cleaned up.
     *
     *  NOTE:  Be careful to save and restore the interpreter state.
     *    Data can get freed in the middle of any operation, and
     *    we can't affort to clobber the interpreter with any errors
     *    from below.
     */

    ItclReleaseClass(ioPtr->iclsPtr);
    if (ioPtr->constructed) {
        Tcl_DeleteHashTable(ioPtr->constructed);
        ckfree((char*)ioPtr->constructed);
    }
    if (ioPtr->destructed) {
        Tcl_DeleteHashTable(ioPtr->destructed);
        ckfree((char*)ioPtr->destructed);
    }
    ItclDeleteObjectsDictInfo(ioPtr->interp, ioPtr);
    /*
     *  Delete all context definitions.
     */
    while (1) {
	hPtr = Tcl_FirstHashEntry(&ioPtr->contextCache, &place);
	if (hPtr == NULL) {
	    break;
	}
	callContextPtr = (ItclCallContext *)Tcl_GetHashValue(hPtr);
	Tcl_DeleteHashEntry(hPtr);
	ckfree((char *)callContextPtr);
    }
    FOREACH_HASH_VALUE(var, &ioPtr->objectVariables) {
	Itcl_ReleaseVar(var);
    }

    Tcl_DeleteHashTable(&ioPtr->contextCache);
    Tcl_DeleteHashTable(&ioPtr->objectVariables);
    Tcl_DeleteHashTable(&ioPtr->objectOptions);
    Tcl_DeleteHashTable(&ioPtr->objectComponents);
    Tcl_DeleteHashTable(&ioPtr->objectMethodVariables);
    Tcl_DeleteHashTable(&ioPtr->objectDelegatedOptions);
    Tcl_DeleteHashTable(&ioPtr->objectDelegatedFunctions);
    Tcl_DecrRefCount(ioPtr->namePtr);
    Tcl_DecrRefCount(ioPtr->origNamePtr);
    if (ioPtr->createNamePtr != NULL) {
        Tcl_DecrRefCount(ioPtr->createNamePtr);
    }
    if (ioPtr->hullWindowNamePtr != NULL) {
        Tcl_DecrRefCount(ioPtr->hullWindowNamePtr);
    }
    Tcl_DecrRefCount(ioPtr->varNsNamePtr);
    if (ioPtr->resolvePtr != NULL) {
	ckfree((char *)ioPtr->resolvePtr->clientData);
        ckfree((char*)ioPtr->resolvePtr);
    }
    Itcl_Free(ioPtr);
}

/*
 * ------------------------------------------------------------------------
 *  ItclObjectCmd()
 *
 * ------------------------------------------------------------------------
 */

static int
CallPublicObjectCmd(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Object *oPtr = (Tcl_Object *)data[0];
    Tcl_Class clsPtr = (Tcl_Class)data[1];
    Tcl_Obj *const *objv = (Tcl_Obj *const *)data[3];
    int objc = PTR2INT(data[2]);

    ItclShowArgs(1, "CallPublicObjectCmd", objc, objv);
    result = Itcl_PublicObjectCmd(oPtr, interp, clsPtr, objc, objv);
    ItclShowArgs(1, "CallPublicObjectCmd DONE", objc, objv);
    return result;
}

int
ItclObjectCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_Object oPtr,
    Tcl_Class clsPtr,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Obj *methodNamePtr;
    Tcl_Obj **newObjv;
    Tcl_DString buffer;
    Tcl_Obj *myPtr;
    ItclMemberFunc *imPtr;
    ItclClass *iclsPtr;
    Itcl_ListElem *elem;
    ItclClass *basePtr;
    void *callbackPtr;
    const char *className;
    const char *tail;
    const char *cp;
    int isDirectCall;
    int incr;
    int result;
    int found;

    ItclShowArgs(1, "ItclObjectCmd", objc, objv);

    incr = 0;
    found = 0;
    isDirectCall = 0;
    myPtr = NULL;
    imPtr = (ItclMemberFunc *)clientData;
    iclsPtr = imPtr->iclsPtr;
    if (oPtr == NULL) {
	ItclClass *icPtr = NULL;
	ItclObject *ioPtr = NULL;

	isDirectCall = (clsPtr == NULL);

	if ((imPtr->flags & ITCL_COMMON)
	        && (imPtr->codePtr != NULL)
	        && !(imPtr->codePtr->flags & ITCL_BUILTIN)) {
	    result = Itcl_InvokeProcedureMethod(imPtr->tmPtr, interp,
	            objc, objv);
            return result;
	}

	if (TCL_OK == Itcl_GetContext(interp, &icPtr, &ioPtr)) {
	    oPtr = ioPtr ? ioPtr->oPtr : icPtr->oPtr;
	} else {
	    Tcl_Panic("No Context");
	}
    }
    methodNamePtr = NULL;
    if (objv[0] != NULL) {
        Itcl_ParseNamespPath(Tcl_GetString(objv[0]), &buffer,
	        &className, &tail);
        if (className != NULL) {
            methodNamePtr = Tcl_NewStringObj(tail, -1);
	    /* look for the class in the hierarchy */
	    cp = className;
	    if ((*cp == ':') && (*(cp+1) == ':')) {
	        cp += 2;
	    }
            elem = Itcl_FirstListElem(&iclsPtr->bases);
	    if (elem == NULL) {
	        /* check the class itself */
		if (strcmp((const char *)cp,
		        (const char *)Tcl_GetString(iclsPtr->namePtr)) == 0) {
		    found = 1;
		    clsPtr = iclsPtr->clsPtr;
		}
	    }
            while (elem != NULL) {
                basePtr = (ItclClass*)Itcl_GetListValue(elem);
		if (strcmp((const char *)cp,
		        (const char *)Tcl_GetString(basePtr->namePtr)) == 0) {
		    clsPtr = basePtr->clsPtr;
		    found = 1;
		    break;
		}
                elem = Itcl_NextListElem(elem);
	    }
	    if (!found) {
		found = 1;
		clsPtr = iclsPtr->clsPtr;
	    }
        }
        Tcl_DStringFree(&buffer);
    } else {
	/* Can this happen? */
	Tcl_Panic("objv[0] is NULL?!");
	/* Panic above replaces obviously broken line below.  Creating
	 * a string value from uninitialized memory cannot possibly be
	 * a correct thing to do.

        methodNamePtr = Tcl_NewStringObj(tail, -1);
	 */
    }
    if (isDirectCall) {
	if (!found) {
	    if (methodNamePtr != NULL) {
	        Tcl_DecrRefCount(methodNamePtr);
	    }
            methodNamePtr = objv[0];
        }
    }
    callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
    newObjv = NULL;
    if (methodNamePtr != NULL) {
	if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
	    char *myName;
	    /* special handling for mytypemethod, mymethod, myproc */
	    myName = Tcl_GetString(methodNamePtr);
	    if (strcmp(myName, "mytypemethod") == 0) {
                result = Itcl_BiMyTypeMethodCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "mymethod") == 0) {
                result = Itcl_BiMyMethodCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "myproc") == 0) {
                result = Itcl_BiMyProcCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "mytypevar") == 0) {
                result = Itcl_BiMyTypeVarCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "myvar") == 0) {
                result = Itcl_BiMyVarCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "itcl_hull") == 0) {
                result = Itcl_BiItclHullCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "callinstance") == 0) {
                result = Itcl_BiCallInstanceCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "getinstancevar") == 0) {
                result = Itcl_BiGetInstanceVarCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	    if (strcmp(myName, "installcomponent") == 0) {
                result = Itcl_BiInstallComponentCmd(iclsPtr, interp, objc, objv);
		return result;
	    }
	}
        incr = 1;
        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+incr));
        myPtr = Tcl_NewStringObj("my", 2);
        Tcl_IncrRefCount(myPtr);
        Tcl_IncrRefCount(methodNamePtr);
        newObjv[0] = myPtr;
        newObjv[1] = methodNamePtr;
        memcpy(newObjv+incr+1, objv+1, (sizeof(Tcl_Obj*)*(objc-1)));
	ItclShowArgs(1, "run CallPublicObjectCmd1", objc+incr, newObjv);
	Tcl_NRAddCallback(interp, CallPublicObjectCmd, oPtr, clsPtr,
	        INT2PTR(objc+incr), newObjv);

    } else {
	ItclShowArgs(1, "run CallPublicObjectCmd2", objc, objv);
	Tcl_NRAddCallback(interp, CallPublicObjectCmd, oPtr, clsPtr,
		INT2PTR(objc), (void *)objv);
    }

    result = Itcl_NRRunCallbacks(interp, callbackPtr);
    if (methodNamePtr != NULL) {
        ckfree((char *)newObjv);
        Tcl_DecrRefCount(methodNamePtr);
    }
    if (myPtr != NULL) {
        Tcl_DecrRefCount(myPtr);
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  GetClassFromClassName()
 * ------------------------------------------------------------------------
 */

ItclClass *
GetClassFromClassName(
    Tcl_Interp *interp,
    const char *className,
    ItclClass *iclsPtr)
{
    Tcl_Obj *objPtr;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *basePtr;
    Itcl_ListElem *elem;
    const char *chkPtr;
    int chkLgth;
    int lgth;

    /* look for the class in the hierarchy */
    /* first check the class itself */
    if (iclsPtr != NULL) {
        if (strcmp(className,
                (const char *)Tcl_GetString(iclsPtr->namePtr)) == 0) {
	    return iclsPtr;
	}
        elem = Itcl_FirstListElem(&iclsPtr->bases);
        while (elem != NULL) {
            basePtr = (ItclClass*)Itcl_GetListValue(elem);
	    basePtr = GetClassFromClassName(interp, className, basePtr);
	    if (basePtr != NULL) {
	        return basePtr;
	    }
            elem = Itcl_NextListElem(elem);
        }
        /* now try to match the classes full name last part with the className */
        lgth = strlen(className);
        elem = Itcl_FirstListElem(&iclsPtr->bases);
        while (elem != NULL) {
            basePtr = (ItclClass*)Itcl_GetListValue(elem);
	    chkPtr = basePtr->nsPtr->fullName;
	    chkLgth = strlen(chkPtr);
	    if (chkLgth >= lgth) {
	        chkPtr = chkPtr + chkLgth - lgth;
	        if (strcmp(chkPtr, className) == 0) {
	            return basePtr;
	        }
	    }
            elem = Itcl_NextListElem(elem);
        }
        infoPtr = iclsPtr->infoPtr;
    } else {
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
    }
    /* as a last chance try with className in hash table */
    objPtr = Tcl_NewStringObj(className, -1);
    Tcl_IncrRefCount(objPtr);
    hPtr = Tcl_FindHashEntry(&infoPtr->nameClasses, (char *)objPtr);
    if (hPtr != NULL) {
        iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
    } else {
        iclsPtr = NULL;
    }
    Tcl_DecrRefCount(objPtr);
    return iclsPtr;
}

/*
 * ------------------------------------------------------------------------
 *  ItclMapMethodNameProc()
 * ------------------------------------------------------------------------
 */

int
ItclMapMethodNameProc(
    Tcl_Interp *interp,
    Tcl_Object oPtr,
    Tcl_Class *startClsPtr,
    Tcl_Obj *methodObj)
{
    Tcl_Obj *methodName;
    Tcl_Obj *className;
    Tcl_DString buffer;
    Tcl_HashEntry *hPtr;
    Tcl_Namespace * myNsPtr;
    ItclObject *ioPtr;
    ItclClass *iclsPtr;
    ItclClass *iclsPtr2;
    ItclObjectInfo *infoPtr;
    const char *head;
    const char *tail;
    const char *sp;

    iclsPtr = NULL;
    iclsPtr2 = NULL;
    methodName = NULL;
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
            ITCL_INTERP_DATA, NULL);
    ioPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
            infoPtr->object_meta_type);
    hPtr = Tcl_FindHashEntry(&infoPtr->objects, (char *)ioPtr);
    if ((hPtr == NULL) || (ioPtr == NULL)) {
        /* try to get the class (if a class is creating an object) */
        iclsPtr = (ItclClass *)Tcl_ObjectGetMetadata(oPtr,
            infoPtr->class_meta_type);
        hPtr = Tcl_FindHashEntry(&infoPtr->classes, (char *)iclsPtr);
	if (hPtr == NULL) {
	    char str[20];
	    sprintf(str, "%p", iclsPtr);
	    Tcl_AppendResult(interp, "context class has vanished 1", str, NULL);
            return TCL_ERROR;
	}
    } else {
        hPtr = Tcl_FindHashEntry(&infoPtr->classes, (char *)ioPtr->iclsPtr);
	if (hPtr == NULL) {
	    char str[20];
	    sprintf(str, "%p", ioPtr->iclsPtr);
	    Tcl_AppendResult(interp, "context class has vanished 2", str, NULL);
            return TCL_ERROR;
	}
        iclsPtr = ioPtr->iclsPtr;
    }
    sp = Tcl_GetString(methodObj);
    Itcl_ParseNamespPath(sp, &buffer, &head, &tail);
    if (head == NULL) {
        /* itcl bug #3600923 call private method in class
	 * without namespace
	 */
        myNsPtr = Tcl_GetCurrentNamespace(iclsPtr->interp);
	hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *) myNsPtr);
	if (hPtr) {
	    iclsPtr2 = (ItclClass *) Tcl_GetHashValue(hPtr);
	    if (Itcl_IsMethodCallFrame(iclsPtr->interp) > 0) {
		iclsPtr = iclsPtr2;
	    }
	}
    }
    if (head != NULL) {
        className = NULL;
        methodName = Tcl_NewStringObj(tail, -1);
        Tcl_IncrRefCount(methodName);
        className = Tcl_NewStringObj(head, -1);
        Tcl_IncrRefCount(className);
	if (strlen(head) > 0) {
	    iclsPtr2 = GetClassFromClassName(interp, head, iclsPtr);
	} else {
	    iclsPtr2 = NULL;
	}
	if (iclsPtr2 != NULL) {
	    *startClsPtr = iclsPtr2->clsPtr;
	    Tcl_SetStringObj(methodObj, Tcl_GetString(methodName), -1);
	}
        Tcl_DecrRefCount(className);
        Tcl_DecrRefCount(methodName);
    }
    hPtr = Tcl_FindHashEntry(&iclsPtr->resolveCmds, (char *)methodObj);
    if (hPtr == NULL) {
        /* special case: we found the class for the class command,
	 * for a relative or absolute class path name
	 * but we have no method in that class that fits.
	 * Problem of Rene Zaumseil when having the object
	 * for a class in a child namespace of the class
	 * fossil ticket id: 36577626c340ad59615f0a0238d67872c009a8c9
	 */
        *startClsPtr = NULL;
    } else {
	ItclMemberFunc *imPtr;
	Tcl_Namespace *nsPtr;
	ItclCmdLookup *clookup;

	nsPtr = Tcl_GetCurrentNamespace(interp);
	clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
	imPtr = clookup->imPtr;
        if (!Itcl_CanAccessFunc(imPtr, nsPtr)) {
	    char *token = Tcl_GetString(imPtr->namePtr);
	    if ((*token != 'i') || (strcmp(token, "info") != 0)) {
		/* needed for test protect-2.5 */
	        ItclMemberFunc *imPtr2 = NULL;
                Tcl_HashEntry *hPtr;
	        Tcl_ObjectContext context;
	        context = (Tcl_ObjectContext)Itcl_GetCallFrameClientData(interp);
                if (context != NULL) {
	            hPtr = Tcl_FindHashEntry(
		            &imPtr->iclsPtr->infoPtr->procMethods,
	                    (char *)Tcl_ObjectContextMethod(context));
	            if (hPtr != NULL) {
	                imPtr2 = (ItclMemberFunc *)Tcl_GetHashValue(hPtr);
	            }
		    if ((imPtr->protection & ITCL_PRIVATE) &&
		            (imPtr2 != NULL) &&
		            (imPtr->iclsPtr->nsPtr != imPtr2->iclsPtr->nsPtr)) {
                        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		                "invalid command name \"",
			        token,
			        "\"", NULL);
		        return TCL_ERROR;
		    }
                }
		/* END needed for test protect-2.5 */
                if (ioPtr == NULL) {
                    /* itcl in fossil ticket: 2cd667f270b68ef66d668338e09d144e20405e23 */
                    Tcl_HashEntry *hPtr;
                    Tcl_Obj * objPtr;
	            ItclMemberFunc *imPtr2 = NULL;
                    ItclCmdLookup *clookupPtr;

                    objPtr = Tcl_NewStringObj(token, -1);
                    hPtr = Tcl_FindHashEntry(&iclsPtr->resolveCmds, (char *)objPtr);
	            if (hPtr != NULL) {
	                clookupPtr = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
                        imPtr2 = clookupPtr->imPtr;
                    }
		    if ((imPtr->protection & ITCL_PRIVATE) &&
		            (imPtr2 != NULL) &&
		            (imPtr->iclsPtr->nsPtr == imPtr2->iclsPtr->nsPtr)) {
                        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		                "invalid command name \"",
			        token,
			        "\"", NULL);
		        return TCL_ERROR;
		    }
                } else {
                    Tcl_AppendResult(interp,
                           "bad option \"", token, "\": should be one of...",
                        NULL);
	            ItclReportObjectUsage(interp, ioPtr, nsPtr, nsPtr);
                    return TCL_ERROR;

                }
            }
        }
    }
    Tcl_DStringFree(&buffer);
    return TCL_OK;
}

int
ExpandDelegateAs(
    Tcl_Interp *interp,
    ItclObject *ioPtr,
    ItclClass *iclsPtr,
    ItclDelegatedFunction *idmPtr,
    const char *funcName,
    Tcl_Obj *listPtr)
{
    Tcl_Obj *componentNamePtr;
    Tcl_Obj *objPtr;
    const char **argv;
    const char *val;
    int argc;
    int j;


    if (idmPtr->icPtr == NULL) {
        componentNamePtr = NULL;
    } else {
        componentNamePtr = idmPtr->icPtr->namePtr;
    }
    if (idmPtr->asPtr != NULL) {
        Tcl_SplitList(interp, Tcl_GetString(idmPtr->asPtr),
	        &argc, &argv);
        for(j=0;j<argc;j++) {
            Tcl_ListObjAppendElement(interp, listPtr,
                    Tcl_NewStringObj(argv[j], -1));
        }
	ckfree((char *)argv);
    } else {
	if (idmPtr->usingPtr != NULL) {
	    char *cp;
	    char *ep;
	    int hadDoublePercent;
            Tcl_Obj *strPtr;

	    strPtr = NULL;
	    hadDoublePercent = 0;
	    cp = Tcl_GetString(idmPtr->usingPtr);
	    ep = cp;
	    strPtr = Tcl_NewStringObj("", -1);
	    while (*ep != '\0') {
	        if (*ep == '%') {
		    if (*(ep+1) == '%') {
			cp++;
			cp++;
		        ep++;
		        ep++;
			hadDoublePercent = 1;
			Tcl_AppendToObj(strPtr, "%", -1);
		        continue;
		    }
		    switch (*(ep+1)) {
		    case 'c':
			if (componentNamePtr == NULL) {
			    ep++;
			    continue;
			}
			if (ep-cp-1 > 0) {
		            Tcl_ListObjAppendElement(interp, listPtr,
			            Tcl_NewStringObj(cp, ep-cp-1));
			}
                        objPtr = Tcl_NewStringObj(ITCL_VARIABLES_NAMESPACE, -1);
			Tcl_AppendToObj(objPtr, (Tcl_GetObjectNamespace(
				iclsPtr->oPtr))->fullName, -1);
                        Tcl_AppendToObj(objPtr, "::", -1);
                        Tcl_AppendToObj(objPtr,
			        Tcl_GetString(componentNamePtr), -1);
                        val = Tcl_GetVar2(interp, Tcl_GetString(objPtr),
			        NULL, 0);
			Tcl_DecrRefCount(objPtr);
			Tcl_AppendToObj(strPtr,
			        val, -1);
		        break;
		    case 'j':
		    case 'm':
		    case 'M':
			if (ep-cp-1 > 0) {
		            Tcl_ListObjAppendElement(interp, listPtr,
			            Tcl_NewStringObj(cp, ep-cp-1));
			}
			if (strcmp(Tcl_GetString(idmPtr->namePtr), "*") == 0) {
			    Tcl_AppendToObj(strPtr, funcName, -1);
			} else {
			    Tcl_AppendToObj(strPtr,
			            Tcl_GetString(idmPtr->namePtr), -1);
			}
		        break;
		    case 'n':
			if (iclsPtr->flags & ITCL_TYPE) {
			    ep++;
			    continue;
			} else {
			    if (ep-cp-1 > 0) {
		                Tcl_ListObjAppendElement(interp, listPtr,
			                Tcl_NewStringObj(cp, ep-cp-1));
			    }
			    Tcl_AppendToObj(strPtr, iclsPtr->nsPtr->name, -1);
			}
		        break;
		    case 's':
			if (iclsPtr->flags & ITCL_TYPE) {
			    ep++;
			    continue;
			} else {
			    if (ep-cp-1 > 0) {
		                Tcl_ListObjAppendElement(interp, listPtr,
			                Tcl_NewStringObj(cp, ep-cp-1));
			    }
                            Tcl_AppendToObj(strPtr,
			            Tcl_GetString(ioPtr->namePtr), -1);
			}
		        break;
		    case 't':
			if (ep-cp-1 > 0) {
		            Tcl_ListObjAppendElement(interp, listPtr,
			            Tcl_NewStringObj(cp, ep-cp-1));
			}
                        Tcl_AppendToObj(strPtr, iclsPtr->nsPtr->fullName, -1);
		        break;
		    case 'w':
			if (iclsPtr->flags & ITCL_TYPE) {
			    ep++;
			    continue;
			} else {
			    if (ep-cp-1 > 0) {
		                Tcl_ListObjAppendElement(interp, listPtr,
			                Tcl_NewStringObj(cp, ep-cp-1));
			    }
		        }
		        break;
		    case ':':
		        /* substitute with contents of variable after ':' */
			if (iclsPtr->flags & ITCL_ECLASS) {
			    if (ep-cp-1 > 0) {
		                Tcl_ListObjAppendElement(interp, listPtr,
			                Tcl_NewStringObj(cp, ep-cp-1));
			    }
			    ep++;
			    cp = ep + 1;
			    while (*ep && (*ep != ' ')) {
			        ep++;
			    }
			    if (ep-cp > 0) {
				Tcl_Obj *my_obj;
				const char *cp2;

                                my_obj = Tcl_NewStringObj(cp, ep-cp);
				if (iclsPtr->infoPtr->currIoPtr != NULL) {
				  cp2 = GetConstructorVar(interp, iclsPtr,
				      Tcl_GetString(my_obj));
				} else {
                                  cp2 = ItclGetInstanceVar(interp,
                                      Tcl_GetString(my_obj), NULL, ioPtr,
				      iclsPtr);
				}
				if (cp2 != NULL) {
                                    Tcl_AppendToObj(strPtr, cp2, -1);
				}
			        ep -= 2; /* to fit for code after default !! */
			    }
		            break;
		        } else {
			    /* fall through */
			}
		    default:
		      {
			char buf[2];
			buf[1] = '\0';
			sprintf(buf, "%c", *(ep+1));
			Tcl_AppendResult(interp,
			        "there is no %%", buf, " substitution",
				NULL);
			if (strPtr != NULL) {
			    Tcl_DecrRefCount(strPtr);
			}
			return TCL_ERROR;
		      }
		    }
		    Tcl_ListObjAppendElement(interp, listPtr, strPtr);
		    hadDoublePercent = 0;
		    strPtr = Tcl_NewStringObj("", -1);
		    ep +=2;
		    cp = ep;
		} else {
		    if (*ep == ' ') {
                        if (strlen(Tcl_GetString(strPtr)) > 0) {
			    if (ep-cp == 0) {
			        Tcl_ListObjAppendElement(interp, listPtr,
				        strPtr);
			        strPtr = Tcl_NewStringObj("", -1);
			    }
			}
			if (ep-cp > 0) {
			    Tcl_AppendToObj(strPtr, cp, ep-cp);
		            Tcl_ListObjAppendElement(interp, listPtr, strPtr);
	                    strPtr = Tcl_NewStringObj("", -1);
			}
		        while((*ep != '\0') && (*ep == ' ')) {
			    ep++;
			}
			cp = ep;
		    } else {
		        ep++;
		    }
		}
	    }
	    if (hadDoublePercent) {
		    /* FIXME need code here */
	    }
	    if (cp != ep) {
		if (*ep == '\0') {
                    Tcl_ListObjAppendElement(interp, listPtr,
	                    Tcl_NewStringObj(cp, ep-cp));
		} else {
                    Tcl_ListObjAppendElement(interp, listPtr,
	                    Tcl_NewStringObj(cp, ep-cp-1));
	        }
	    }
	    if (strPtr != NULL) {
	        Tcl_DecrRefCount(strPtr);
	    }
	} else {
            Tcl_ListObjAppendElement(interp, listPtr, idmPtr->namePtr);
        }
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  DelegationFunction()
 * ------------------------------------------------------------------------
 */

int
DelegateFunction(
    Tcl_Interp *interp,
    ItclObject *ioPtr,
    ItclClass *iclsPtr,
    Tcl_Obj *componentValuePtr,
    ItclDelegatedFunction *idmPtr)
{
    Tcl_Obj *listPtr;
    const char *val;
    int result;
    Tcl_Method mPtr;

    listPtr = Tcl_NewListObj(0, NULL);
    if (componentValuePtr != NULL) {
	if (idmPtr->usingPtr == NULL) {
            Tcl_ListObjAppendElement(interp, listPtr, componentValuePtr);
        }
    }
    result = ExpandDelegateAs(interp, ioPtr, iclsPtr, idmPtr,
            Tcl_GetString(idmPtr->namePtr), listPtr);
    if (result != TCL_OK) {
	Tcl_DecrRefCount(listPtr);
        return result;
    }
    val = Tcl_GetString(listPtr);
    if (val == NULL) {
        /* FIXME need code here */
    }
    if (componentValuePtr != NULL) {
        mPtr = Itcl_NewForwardClassMethod(interp, iclsPtr->clsPtr, 1,
                idmPtr->namePtr, listPtr);
        if (mPtr != NULL) {
            return TCL_OK;
        }
    }
    if (idmPtr->usingPtr != NULL) {
        mPtr = Itcl_NewForwardClassMethod(interp, iclsPtr->clsPtr, 1,
                idmPtr->namePtr, listPtr);
        if (mPtr != NULL) {
            return TCL_OK;
        }
    }
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  DelegatedOptionsInstall()
 * ------------------------------------------------------------------------
 */

int
DelegatedOptionsInstall(
    Tcl_Interp *interp,
    ItclClass *iclsPtr)
{
    Tcl_HashEntry *hPtr2;
    Tcl_HashSearch search2;
    ItclDelegatedOption *idoPtr;
    ItclOption *ioptPtr;
    FOREACH_HASH_DECLS;
    char *optionName;

    FOREACH_HASH_VALUE(idoPtr, &iclsPtr->delegatedOptions) {
	optionName = Tcl_GetString(idoPtr->namePtr);
	if (*optionName == '*') {
	    /* allow nested FOREACH */
	    search2 = search;
	    FOREACH_HASH_VALUE(ioptPtr, &iclsPtr->options) {
	        if (Tcl_FindHashEntry(&idoPtr->exceptions,
		        (char *)idoPtr->namePtr) == NULL) {
		    ioptPtr->idoPtr = idoPtr;
		    Itcl_PreserveData(ioptPtr->idoPtr);
		}
	    }
	    search = search2;
	} else {
            hPtr2 = Tcl_FindHashEntry(&iclsPtr->options,
	            (char *)idoPtr->namePtr);
	    if (hPtr2 == NULL) {
		ioptPtr = NULL;
	    } else {
	        ioptPtr = (ItclOption *)Tcl_GetHashValue(hPtr2);
	        ioptPtr->idoPtr = idoPtr;
	    }
	    idoPtr->ioptPtr = ioptPtr;
        }
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  GetConstructorVar()
 *  get an object variable when in executing the constructor
 * ------------------------------------------------------------------------
 */

static const char *
GetConstructorVar(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    const char *varName)

{
    Tcl_HashEntry *hPtr;
    Tcl_Obj *objPtr;
    Tcl_DString buffer;
    ItclVarLookup *vlookup;
    ItclVariable *ivPtr;
    const char *val;

    hPtr = ItclResolveVarEntry(iclsPtr, (char *)varName);
    if (hPtr == NULL) {
	/* no such variable */
        return NULL;
    }
    vlookup = (ItclVarLookup *)Tcl_GetHashValue(hPtr);
    if (vlookup == NULL) {
        return NULL;
    }
    ivPtr = vlookup->ivPtr;
    if (ivPtr == NULL) {
        return NULL;
    }
    if (ivPtr->flags & ITCL_COMMON) {
        /* look for a common variable */
        objPtr = Tcl_NewStringObj(ITCL_VARIABLES_NAMESPACE, -1);
        Tcl_AppendToObj(objPtr, (Tcl_GetObjectNamespace(
		iclsPtr->oPtr))->fullName, -1);
        Tcl_AppendToObj(objPtr, "::", -1);
        Tcl_AppendToObj(objPtr, varName, -1);
        val = Tcl_GetVar2(interp, Tcl_GetString(objPtr), NULL, 0);
        Tcl_DecrRefCount(objPtr);
    } else {
        /* look for a normal variable */
        Tcl_DStringInit(&buffer);
        Tcl_DStringAppend(&buffer,
                Tcl_GetString(iclsPtr->infoPtr->currIoPtr->varNsNamePtr), -1);
        Tcl_DStringAppend(&buffer, ivPtr->iclsPtr->nsPtr->fullName, -1);
        Tcl_DStringAppend(&buffer, "::", -1);
        Tcl_DStringAppend(&buffer, varName, -1);
        val = Tcl_GetVar2(interp, Tcl_DStringValue(&buffer), NULL, 0);
        Tcl_DStringFree(&buffer);
    }
    return val;
}

/*
 * ------------------------------------------------------------------------
 *  DelegationInstall()
 * ------------------------------------------------------------------------
 */

int
DelegationInstall(
    Tcl_Interp *interp,
    ItclObject *ioPtr,
    ItclClass *iclsPtr)
{
    Tcl_HashEntry *hPtr2;
    Tcl_HashSearch search2;
    Tcl_Obj *componentValuePtr;
    Tcl_DString buffer;
    ItclDelegatedFunction *idmPtr;
    ItclMemberFunc *imPtr;
    ItclVariable *ivPtr;
    FOREACH_HASH_DECLS;
    char *methodName;
    const char *val;
    int result;
    int noDelegate;
    int delegateAll;

    result = TCL_OK;
    delegateAll = 0;
    ioPtr->noComponentTrace = 1;
    noDelegate = ITCL_CONSTRUCTOR|ITCL_DESTRUCTOR|ITCL_COMPONENT;
    componentValuePtr = NULL;
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	methodName = Tcl_GetString(idmPtr->namePtr);
	if (*methodName == '*') {
	    delegateAll = 1;
	}
	if (idmPtr->icPtr != NULL) {
	    Tcl_Obj *objPtr;
	    /* we cannot use Itcl_GetInstanceVar here as the object is not
	     * yet completely built. So use the varNsNamePtr
	     */
	    ivPtr = idmPtr->icPtr->ivPtr;
            if (ivPtr->flags & ITCL_COMMON) {
	        objPtr = Tcl_NewStringObj(ITCL_VARIABLES_NAMESPACE, -1);

	        Tcl_AppendToObj(objPtr, (Tcl_GetObjectNamespace(
			ivPtr->iclsPtr->oPtr))->fullName, -1);
	        Tcl_AppendToObj(objPtr, "::", -1);
	        Tcl_AppendToObj(objPtr,
		        Tcl_GetString(idmPtr->icPtr->namePtr), -1);
	        val = Tcl_GetVar2(interp, Tcl_GetString(objPtr), NULL, 0);
	        Tcl_DecrRefCount(objPtr);
	    } else {
                Tcl_DStringInit(&buffer);
                Tcl_DStringAppend(&buffer,
                        Tcl_GetString(ioPtr->varNsNamePtr), -1);
                Tcl_DStringAppend(&buffer,
                        Tcl_GetString(ivPtr->fullNamePtr), -1);
                val = Tcl_GetVar2(interp,
                            Tcl_DStringValue(&buffer), NULL, 0);
                Tcl_DStringFree(&buffer);
	    }
	    componentValuePtr = Tcl_NewStringObj(val, -1);
            Tcl_IncrRefCount(componentValuePtr);
	} else {
	    componentValuePtr = NULL;
	}
	if (!delegateAll) {
	    result = DelegateFunction(interp, ioPtr, iclsPtr,
	            componentValuePtr, idmPtr);
	    if (result != TCL_OK) {
                ioPtr->noComponentTrace = 0;
	        return result;
	    }
	} else {
	    /* save to allow nested FOREACH */
            search2 = search;
            FOREACH_HASH_VALUE(imPtr, &iclsPtr->functions) {
	        methodName = Tcl_GetString(imPtr->namePtr);
                if (imPtr->flags & noDelegate) {
		    continue;
		}
                if (strcmp(methodName, "info") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "isa") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "createhull") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "keepcomponentoption") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "ignorecomponentoption") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "renamecomponentoption") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "setupcomponent") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "itcl_initoptions") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "mytypemethod") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "mymethod") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "myproc") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "mytypevar") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "myvar") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "itcl_hull") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "callinstance") == 0) {
	            continue;
	        }
                if (strcmp(methodName, "getinstancevar") == 0) {
	            continue;
	        }
                hPtr2 = Tcl_FindHashEntry(&idmPtr->exceptions,
		        (char *)imPtr->namePtr);
                if (hPtr2 != NULL) {
		    continue;
		}
	        result = DelegateFunction(interp, ioPtr, iclsPtr,
	                componentValuePtr, idmPtr);
	        if (result != TCL_OK) {
	            break;
	        }
            }
            search = search2;
        }
	if (componentValuePtr != NULL) {
            Tcl_DecrRefCount(componentValuePtr);
        }
    }
    ioPtr->noComponentTrace = 0;
    result = DelegatedOptionsInstall(interp, iclsPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclInitExtendedClassOptions()
 * ------------------------------------------------------------------------
 */

static int
ItclInitExtendedClassOptions(
    Tcl_Interp *interp,
    ItclObject *ioPtr)
{
    ItclClass *iclsPtr;
    ItclOption *ioptPtr;
    ItclHierIter hier;
    FOREACH_HASH_DECLS;

    iclsPtr = ioPtr->iclsPtr;
    Itcl_InitHierIter(&hier, iclsPtr);
    while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
        FOREACH_HASH_VALUE(ioptPtr, &iclsPtr->options) {
            if (ioptPtr->defaultValuePtr != NULL) {
		if (ItclGetInstanceVar(interp, "itcl_options",
		        Tcl_GetString(ioptPtr->namePtr), ioPtr, iclsPtr)
			== NULL) {
	        }
            }
	}
    }
    Itcl_DeleteHierIter(&hier);
    return TCL_OK;
}

ItclClass *
ItclNamespace2Class(Tcl_Namespace *nsPtr)
{
    ItclObjectInfo * infoPtr;
    Tcl_HashEntry *hPtr;
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(((Namespace *)nsPtr)->interp,
	ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&(infoPtr->namespaceClasses), nsPtr);
    if (hPtr == NULL) {
	return NULL;
    }
    return (ItclClass *)Tcl_GetHashValue(hPtr);
}
