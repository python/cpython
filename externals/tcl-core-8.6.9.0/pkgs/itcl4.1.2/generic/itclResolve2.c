/*
 * ------------------------------------------------------------------------
 *      PACKAGE:  [incr Tcl]
 *  DESCRIPTION:  Object-Oriented Extensions to Tcl
 *
 *  These procedures handle command and variable resolution
 *
 * ========================================================================
 *  AUTHOR:  Arnulf Wiedemann
 *
 * ========================================================================
 *           Copyright (c) Arnulf Wiedemann
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include <tclInt.h>
#include "itclInt.h"
#include "itclVCInt.h"

/*
 * This structure is a subclass of Tcl_ResolvedVarInfo that contains the
 * ItclVarLookup info needed at runtime.
 */
typedef struct ItclResolvedVarInfo {
    Tcl_ResolvedVarInfo vinfo;        /* This must be the first element. */
    ItclVarLookup *vlookup;           /* Pointer to lookup info. */
} ItclResolvedVarInfo;

static Tcl_Var ItclClassRuntimeVarResolver2 (
    Tcl_Interp *interp, Tcl_ResolvedVarInfo *vinfoPtr);

int
Itcl_CheckClassVariableProtection(
    Tcl_Interp *interp,
    Tcl_Namespace *nsPtr,
    const char *varName,
    ClientData clientData)
{
    ItclClassVarInfo *icviPtr;

    icviPtr = (ItclClassVarInfo *)clientData;
    if (icviPtr->protection == ITCL_PRIVATE) {
        if (icviPtr->declaringNsPtr != nsPtr) {
	    Tcl_AppendResult(interp, "can't read \"", varName,
	            "\": no such variable", NULL);
	    return TCL_ERROR;
        }
    }
    return TCL_OK;
}

int
Itcl_CheckClassCommandProtection(
    Tcl_Interp *interp,
    Tcl_Namespace *nsPtr,
    const char *commandName,
    ClientData clientData)
{
    /* FIXME need code here !!! */
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassCmdResolver()
 *
 *  Used by the class namespaces to handle name resolution for all
 *  commands.  This procedure looks for references to class methods
 *  and procs, and returns TCL_OK along with the appropriate Tcl
 *  command in the rPtr argument.  If a particular command is private,
 *  this procedure returns TCL_ERROR and access to the command is
 *  denied.  If a command is not recognized, this procedure returns
 *  TCL_CONTINUE, and lookup continues via the normal Tcl name
 *  resolution rules.
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassCmdResolver2(
    Tcl_Interp *interp,		/* current interpreter */
    const char* name,		/* name of the command being accessed */
    Tcl_Namespace *nsPtr,	/* namespace performing the resolution */
    int flags,			/* TCL_LEAVE_ERR_MSG => leave error messages
				 *   in interp if anything goes wrong */
    Tcl_Command *rPtr)		/* returns: resolved command */
{
    ItclClass *iclsPtr;
    ItclObjectInfo *infoPtr;
    ItclObject *contextIoPtr;

    Tcl_Command cmdPtr;
    ItclResolvingInfo *iriPtr;
    ObjectCmdTableInfo *octiPtr;
    ObjectCmdInfo *ociPtr;
    Tcl_HashEntry *hPtr;

    if ((name[0] == 't') && (strcmp(name, "this") == 0)) {
        return TCL_CONTINUE;
    }
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }
    iclsPtr = Tcl_GetHashValue(hPtr);
    ItclCallContext *callContextPtr;
    callContextPtr = Itcl_PeekStack(&infoPtr->contextStack);
    iriPtr = Tcl_GetAssocData(interp, ITCL_RESOLVE_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&iriPtr->resolveCmds , nsPtr->fullName);
    if (hPtr != NULL) {
	Tcl_HashTable *tablePtr;
	tablePtr = Tcl_GetHashValue(hPtr);
        hPtr = Tcl_FindHashEntry(tablePtr, name);
        if (hPtr != NULL) {
	    ItclClassCmdInfo *icciPtr = Tcl_GetHashValue(hPtr);
            if ((callContextPtr != NULL) && (callContextPtr->ioPtr != NULL)) {
                contextIoPtr = callContextPtr->ioPtr;
                hPtr = Tcl_FindHashEntry(&iriPtr->objectCmdsTables,
		        (char *)contextIoPtr);
	        if (hPtr != NULL) {
	            octiPtr = Tcl_GetHashValue(hPtr);
	            hPtr = Tcl_FindHashEntry(&octiPtr->cmdInfos,
		           (char *)icciPtr);
	            if (hPtr != NULL) {
			int ret;
			ociPtr = Tcl_GetHashValue(hPtr);
			ret = (* iriPtr->cmdProtFcn)(interp,
			        Tcl_GetCurrentNamespace(interp), name,
				(ClientData)icciPtr);
			if (ret != TCL_OK) {
			    return ret;
			}
		        cmdPtr = ociPtr->cmdPtr;
                        *rPtr = cmdPtr;
	                return TCL_OK;
		    }
	        }
	    }
	}
    }
    return TCL_CONTINUE;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassVarResolver()
 *
 *  Used by the class namespaces to handle name resolution for runtime
 *  variable accesses.  This procedure looks for references to both
 *  common variables and instance variables at runtime.  It is used as
 *  a second line of defense, to handle references that could not be
 *  resolved as compiled locals.
 *
 *  If a variable is found, this procedure returns TCL_OK along with
 *  the appropriate Tcl variable in the rPtr argument.  If a particular
 *  variable is private, this procedure returns TCL_ERROR and access
 *  to the variable is denied.  If a variable is not recognized, this
 *  procedure returns TCL_CONTINUE, and lookup continues via the normal
 *  Tcl name resolution rules.
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassVarResolver2(
    Tcl_Interp *interp,       /* current interpreter */
    const char* name,	      /* name of the variable being accessed */
    Tcl_Namespace *nsPtr,   /* namespace performing the resolution */
    int flags,                /* TCL_LEAVE_ERR_MSG => leave error messages
                               *   in interp if anything goes wrong */
    Tcl_Var *rPtr)            /* returns: resolved variable */
{
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclObject *contextIoPtr;
    Tcl_HashEntry *hPtr;
    ItclVarLookup *vlookup;

    Tcl_Var varPtr;
    ItclResolvingInfo *iriPtr;
    ObjectVarTableInfo *ovtiPtr;
    ObjectVarInfo *oviPtr;

    Tcl_Namespace *upNsPtr;
    upNsPtr = Itcl_GetUplevelNamespace(interp, 1);

    /*
     *  If this is a global variable, handle it in the usual
     *  Tcl manner.
     */
    if (flags & TCL_GLOBAL_ONLY) {
        return TCL_CONTINUE;
    }

    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }
    iclsPtr = Tcl_GetHashValue(hPtr);

    /*
     *  See if this is a formal parameter in the current proc scope.
     *  If so, that variable has precedence.  Look it up and return
     *  it here.  This duplicates some of the functionality of
     *  TclLookupVar, but we return it here (instead of returning
     *  TCL_CONTINUE) to avoid looking it up again later.
     */
    ItclCallContext *callContextPtr;
    callContextPtr = Itcl_PeekStack(&infoPtr->contextStack);
    if ((strstr(name,"::") == NULL) &&
            Itcl_IsCallFrameArgument(interp, name)) {
        return TCL_CONTINUE;
    }

    iriPtr = Tcl_GetAssocData(interp, ITCL_RESOLVE_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&iriPtr->resolveVars , nsPtr->fullName);
    if (hPtr != NULL) {
	Tcl_HashTable *tablePtr;
	tablePtr = Tcl_GetHashValue(hPtr);
        hPtr = Tcl_FindHashEntry(tablePtr , name);
        if (hPtr != NULL) {
	    int ret;
	    ItclClassVarInfo *icviPtr = Tcl_GetHashValue(hPtr);
	    ret = (* iriPtr->varProtFcn)(interp,
	            Tcl_GetCurrentNamespace(interp), name,
		    (ClientData)icviPtr);
	    if (ret != TCL_OK) {
	        return ret;
	    }
            /*
             *  If this is an instance variable, then we have to
             *  find the object context,
             */

            if ((callContextPtr != NULL) && (callContextPtr->ioPtr != NULL)) {
                contextIoPtr = callContextPtr->ioPtr;
                hPtr = Tcl_FindHashEntry(&iriPtr->objectVarsTables,
		        (char *)contextIoPtr);
	        if (hPtr != NULL) {
	            ovtiPtr = Tcl_GetHashValue(hPtr);
	            hPtr = Tcl_FindHashEntry(&ovtiPtr->varInfos,
		           (char *)icviPtr);
	            if (hPtr != NULL) {
			oviPtr = Tcl_GetHashValue(hPtr);
		        varPtr = oviPtr->varPtr;
                        *rPtr = varPtr;
	                return TCL_OK;
		    }
	        }
	    }
	}
    }
    /*
     *  See if the variable is a known data member and accessible.
     */
    hPtr = Tcl_FindHashEntry(&iclsPtr->resolveVars, name);
    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }

    vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
    if (!vlookup->accessible) {
        return TCL_CONTINUE;
    }

    /*
     * If this is a common data member, then its variable
     * is easy to find.  Return it directly.
     */
    if ((vlookup->ivPtr->flags & ITCL_COMMON) != 0) {
	hPtr = Tcl_FindHashEntry(&vlookup->ivPtr->iclsPtr->classCommons,
	        (char *)vlookup->ivPtr);
	if (hPtr != NULL) {
	    *rPtr = Tcl_GetHashValue(hPtr);
            return TCL_OK;
	}
    }

    return TCL_CONTINUE;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassCompiledVarResolver()
 *
 *  Used by the class namespaces to handle name resolution for compile
 *  time variable accesses.  This procedure looks for references to
 *  both common variables and instance variables at compile time.  If
 *  the variables are found, they are characterized in a generic way
 *  by their ItclVarLookup record.  At runtime, Tcl constructs the
 *  compiled local variables by calling ItclClassRuntimeVarResolver.
 *
 *  If a variable is found, this procedure returns TCL_OK along with
 *  information about the variable in the rPtr argument.  If a particular
 *  variable is private, this procedure returns TCL_ERROR and access
 *  to the variable is denied.  If a variable is not recognized, this
 *  procedure returns TCL_CONTINUE, and lookup continues via the normal
 *  Tcl name resolution rules.
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassCompiledVarResolver2(
    Tcl_Interp *interp,         /* current interpreter */
    const char* name,           /* name of the variable being accessed */
    int length,                 /* number of characters in name */
    Tcl_Namespace *nsPtr,       /* namespace performing the resolution */
    Tcl_ResolvedVarInfo **rPtr) /* returns: info that makes it possible to
                                 *   resolve the variable at runtime */
{
    ItclClass *iclsPtr;
    ItclObjectInfo *infoPtr;
    Tcl_HashEntry *hPtr;
    ItclVarLookup *vlookup;
    char *buffer;
    char storage[64];

    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }
    iclsPtr = Tcl_GetHashValue(hPtr);
    /*
     *  Copy the name to local storage so we can NULL terminate it.
     *  If the name is long, allocate extra space for it.
     */
    if (length < sizeof(storage)) {
        buffer = storage;
    } else {
        buffer = (char*)ckalloc((unsigned)(length+1));
    }
    memcpy((void*)buffer, (void*)name, (size_t)length);
    buffer[length] = '\0';

    hPtr = Tcl_FindHashEntry(&iclsPtr->resolveVars, buffer);

    if (buffer != storage) {
        ckfree(buffer);
    }

    /*
     *  If the name is not found, or if it is inaccessible,
     *  continue on with the normal Tcl name resolution rules.
     */
    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }

    vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
    if (!vlookup->accessible) {
        return TCL_CONTINUE;
    }

    /*
     *  Return the ItclVarLookup record.  At runtime, Tcl will
     *  call ItclClassRuntimeVarResolver with this record, to
     *  plug in the appropriate variable for the current object
     *  context.
     */
    (*rPtr) = (Tcl_ResolvedVarInfo *) ckalloc(sizeof(ItclResolvedVarInfo));
    (*rPtr)->fetchProc = ItclClassRuntimeVarResolver2;
    (*rPtr)->deleteProc = NULL;
    ((ItclResolvedVarInfo*)(*rPtr))->vlookup = vlookup;

    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  ItclClassRuntimeVarResolver()
 *
 *  Invoked when Tcl sets up the call frame for an [incr Tcl] method/proc
 *  at runtime.  Resolves data members identified earlier by
 *  Itcl_ClassCompiledVarResolver.  Returns the Tcl_Var representation
 *  for the data member.
 * ------------------------------------------------------------------------
 */
static Tcl_Var
ItclClassRuntimeVarResolver2(
    Tcl_Interp *interp,               /* current interpreter */
    Tcl_ResolvedVarInfo *resVarInfo)  /* contains ItclVarLookup rep
                                       * for variable */
{
    ItclVarLookup *vlookup = ((ItclResolvedVarInfo*)resVarInfo)->vlookup;

    ItclClass *iclsPtr;
    ItclObject *contextIoPtr;
    Tcl_HashEntry *hPtr;

    Tcl_Var varPtr;
    ItclResolvingInfo *iriPtr;
    ObjectVarTableInfo *ovtiPtr;
    ObjectVarInfo *oviPtr;

    /*
     *  If this is a common data member, then the associated
     *  variable is known directly.
     */
    if ((vlookup->ivPtr->flags & ITCL_COMMON) != 0) {
	hPtr = Tcl_FindHashEntry(&vlookup->ivPtr->iclsPtr->classCommons,
	        (char *)vlookup->ivPtr);
	if (hPtr != NULL) {
	    return Tcl_GetHashValue(hPtr);
	}
    }
    iclsPtr = vlookup->ivPtr->iclsPtr;

    /*
     *  Otherwise, get the current object context and find the
     *  variable in its data table.
     *
     *  TRICKY NOTE:  Get the index for this variable using the
     *    virtual table for the MOST-SPECIFIC class.
     */

    ItclCallContext *callContextPtr;

    callContextPtr = Itcl_PeekStack(&iclsPtr->infoPtr->contextStack);
    if (callContextPtr == NULL) {
        return NULL;
    }
    if (callContextPtr->ioPtr == NULL) {
        return NULL;
    }
    iriPtr = Tcl_GetAssocData(interp, ITCL_RESOLVE_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&iriPtr->resolveVars,
            Tcl_GetCurrentNamespace(interp)->fullName);
    if (hPtr != NULL) {
        Tcl_HashTable *tablePtr;
	tablePtr = Tcl_GetHashValue(hPtr);
        hPtr = Tcl_FindHashEntry(tablePtr,
	        Tcl_GetString(vlookup->ivPtr->namePtr));
        if (hPtr != NULL) {
	    ItclClassVarInfo *icviPtr = Tcl_GetHashValue(hPtr);
	    int ret;
	    ret = (* iriPtr->varProtFcn)(interp,
	            Tcl_GetCurrentNamespace(interp),
		    Tcl_GetString(vlookup->ivPtr->namePtr),
		    (ClientData)icviPtr);
	    if (ret != TCL_OK) {
	        return NULL;
	    }
            /*
             *  If this is an instance variable, then we have to
             *  find the object context,
             */

            contextIoPtr = callContextPtr->ioPtr;
            hPtr = Tcl_FindHashEntry(&iriPtr->objectVarsTables, (char *)contextIoPtr);
	    if (hPtr != NULL) {
	        ovtiPtr = Tcl_GetHashValue(hPtr);
	        hPtr = Tcl_FindHashEntry(&ovtiPtr->varInfos, (char *)icviPtr);
	        if (hPtr != NULL) {
	            oviPtr = Tcl_GetHashValue(hPtr);
		    varPtr = oviPtr->varPtr;
	            return varPtr;
	        }
	    }
	}
    }
    return NULL;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ParseVarResolver()
 *
 *  Used by the "parser" namespace to resolve variable accesses to
 *  common variables.  The runtime resolver procedure is consulted
 *  whenever a variable is accessed within the namespace.  It can
 *  deny access to certain variables, or perform special lookups itself.
 *
 *  This procedure allows access only to "common" class variables that
 *  have been declared within the class or inherited from another class.
 *  A "set" command can be used to initialized common data members within
 *  the body of the class definition itself:
 *
 *    itcl::class Foo {
 *        common colors
 *        set colors(red)   #ff0000
 *        set colors(green) #00ff00
 *        set colors(blue)  #0000ff
 *        ...
 *    }
 *
 *    itcl::class Bar {
 *        inherit Foo
 *        set colors(gray)  #a0a0a0
 *        set colors(white) #ffffff
 *
 *        common numbers
 *        set numbers(0) zero
 *        set numbers(1) one
 *    }
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_ParseVarResolver2(
    Tcl_Interp *interp,        /* current interpreter */
    const char* name,                /* name of the variable being accessed */
    Tcl_Namespace *contextNs,  /* namespace context */
    int flags,                 /* TCL_GLOBAL_ONLY => global variable
                                * TCL_NAMESPACE_ONLY => namespace variable */
    Tcl_Var* rPtr)             /* returns: Tcl_Var for desired variable */
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)contextNs->clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);

    Tcl_HashEntry *hPtr;
    ItclVarLookup *vlookup;

    /*
     *  See if the requested variable is a recognized "common" member.
     *  If it is, make sure that access is allowed.
     */
    hPtr = Tcl_FindHashEntry(&iclsPtr->resolveVars, name);
    if (hPtr) {
        vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);

        if ((vlookup->ivPtr->flags & ITCL_COMMON) != 0) {
            if (!vlookup->accessible) {
                Tcl_AppendResult(interp,
                    "can't access \"", name, "\": ",
                    Itcl_ProtectionStr(vlookup->ivPtr->protection),
                    " variable",
                    (char*)NULL);
                return TCL_ERROR;
            }
	    hPtr = Tcl_FindHashEntry(&vlookup->ivPtr->iclsPtr->classCommons,
	        (char *)vlookup->ivPtr);
	    if (hPtr != NULL) {
                *rPtr = Tcl_GetHashValue(hPtr);
                return TCL_OK;
	    }
        }
    }

    /*
     *  If the variable is not recognized, return TCL_CONTINUE and
     *  let lookup continue via the normal name resolution rules.
     *  This is important for variables like "errorInfo"
     *  that might get set while the parser namespace is active.
     */
    return TCL_CONTINUE;
}



int
ItclSetParserResolver2(
    Tcl_Namespace *nsPtr)
{
    Itcl_SetNamespaceResolvers(nsPtr, (Tcl_ResolveCmdProc*)NULL,
            Itcl_ParseVarResolver2, (Tcl_ResolveCompiledVarProc*)NULL);
    return TCL_OK;
}
