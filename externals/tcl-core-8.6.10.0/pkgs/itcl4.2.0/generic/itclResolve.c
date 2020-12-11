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
 *  These procedures handle command and variable resolution
 *
 * ========================================================================
 *  AUTHOR:  Michael J. McLennan
 *           Bell Labs Innovations for Lucent Technologies
 *           mmclennan@lucent.com
 *           http://www.tcltk.com/itcl
 * ========================================================================
 *           Copyright (c) 1993-1998  Lucent Technologies, Inc.
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include "itclInt.h"

/*
 * This structure is a subclass of Tcl_ResolvedVarInfo that contains the
 * ItclVarLookup info needed at runtime.
 */
typedef struct ItclResolvedVarInfo {
    Tcl_ResolvedVarInfo vinfo;        /* This must be the first element. */
    ItclVarLookup *vlookup;           /* Pointer to lookup info. */
} ItclResolvedVarInfo;

static Tcl_Var ItclClassRuntimeVarResolver(
    Tcl_Interp *interp, Tcl_ResolvedVarInfo *vinfoPtr);


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
Itcl_ClassCmdResolver(
    Tcl_Interp *interp,		/* current interpreter */
    const char* name,		/* name of the command being accessed */
    Tcl_Namespace *nsPtr,	/* namespace performing the resolution */
    int flags,			/* TCL_LEAVE_ERR_MSG => leave error messages
				 *   in interp if anything goes wrong */
    Tcl_Command *rPtr)		/* returns: resolved command */
{
    Tcl_HashEntry *hPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *namePtr;
    ItclClass *iclsPtr;
    ItclObjectInfo *infoPtr;
    ItclMemberFunc *imPtr;
    int inOptionHandling;
    int isCmdDeleted;

    if ((name[0] == 't') && (strcmp(name, "this") == 0)) {
        return TCL_CONTINUE;
    }
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }
    iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
    /*
     *  If the command is a member function
     */
    imPtr = NULL;
    objPtr = Tcl_NewStringObj(name, -1);
    hPtr = Tcl_FindHashEntry(&iclsPtr->resolveCmds, (char *)objPtr);
    Tcl_DecrRefCount(objPtr);
    if (hPtr == NULL) {
	ItclCmdLookup *clookup;
	if ((iclsPtr->flags & ITCL_ECLASS)) {
	    namePtr = Tcl_NewStringObj(name, -1);
	    hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions,
	            (char *)namePtr);
	    if (hPtr != NULL) {
                objPtr = Tcl_NewStringObj("unknown", -1);
                hPtr = Tcl_FindHashEntry(&iclsPtr->resolveCmds, (char *)objPtr);
                Tcl_DecrRefCount(objPtr);
	    }
	    Tcl_DecrRefCount(namePtr);
	}
        if (hPtr == NULL) {
            return TCL_CONTINUE;
        }
        clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
        imPtr = clookup->imPtr;
    } else {
        ItclCmdLookup *clookup;
        clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
        imPtr = clookup->imPtr;
    }

    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
	/* FIXME check if called from an (instance) method (not from a typemethod) and only then error */
	int isOk = 0;
	if (strcmp(name, "info") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "mytypemethod") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "myproc") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "mymethod") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "mytypevar") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "myvar") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "itcl_hull") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "callinstance") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "getinstancevar") == 0) {
	    isOk = 1;
	}
	if (strcmp(name, "installcomponent") == 0) {
	    isOk = 1;
	}
	if (! isOk) {
	    if ((imPtr->flags & ITCL_TYPE_METHOD) != 0) {
	        Tcl_AppendResult(interp, "invalid command name \"", name,
	                 "\"", NULL);
                return TCL_ERROR;
	    }
	    inOptionHandling = imPtr->iclsPtr->infoPtr->inOptionHandling;
	    if (((imPtr->flags & ITCL_COMMON) == 0) && !inOptionHandling) {
		/* a method cannot be called directly in ITCL_TYPE
		 * so look, if there is a corresponding proc in the
		 * namespace one level up (i.e. for example ::). If yes
		 * use that.
		 */
                Tcl_Namespace *nsPtr2;
		Tcl_Command cmdPtr;
		nsPtr2 = Itcl_GetUplevelNamespace(interp, 1);
		cmdPtr = NULL;
		if (nsPtr != nsPtr2) {
		    cmdPtr = Tcl_FindCommand(interp, name, nsPtr2, 0);
                }
		if (cmdPtr != NULL) {
		    *rPtr = cmdPtr;
		    return TCL_OK;
		}
	        Tcl_AppendResult(interp, "invalid command name \"", name,
	                 "\"", NULL);
                return TCL_ERROR;
	    }
        }
    }
    /*
     *  Looks like we found an accessible member function.
     *
     *  TRICKY NOTE:  Check to make sure that the command handle
     *    is still valid.  If someone has deleted or renamed the
     *    command, it may not be.  This is just the time to catch
     *    it--as it is being resolved again by the compiler.
     */

    /*
     * The following #if is needed so itcl can be compiled with
     * all versions of Tcl.  The integer "deleted" was renamed to
     * "flags" in tcl8.4a2.  This #if is also found in itcl_ensemble.c .
     * We're using a runtime check with itclCompatFlags to adjust for
     * the behavior of this change, too.
     *
     */
/* FIXME !!! */
isCmdDeleted = 0;
/*    isCmdDeleted = (!imPtr->accessCmd || imPtr->accessCmd->flags); */

    if (isCmdDeleted) {
	imPtr->accessCmd = NULL;

	if ((flags & TCL_LEAVE_ERR_MSG) != 0) {
	    Tcl_AppendResult(interp,
		"can't access \"", name, "\": deleted or redefined\n",
		"(use the \"body\" command to redefine methods/procs)",
		NULL);
	}
	return TCL_ERROR;   /* disallow access! */
    }
    *rPtr = imPtr->accessCmd;
    return TCL_OK;
}

/* #define VAR_DEBUG */

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
Itcl_ClassVarResolver(
    Tcl_Interp *interp,       /* current interpreter */
    const char* name,	      /* name of the variable being accessed */
    Tcl_Namespace *nsPtr,     /* namespace performing the resolution */
    int flags,                /* TCL_LEAVE_ERR_MSG => leave error messages
                               *   in interp if anything goes wrong */
    Tcl_Var *rPtr)            /* returns: resolved variable */
{
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclObject *contextIoPtr;
    Tcl_HashEntry *hPtr;
    ItclVarLookup *vlookup;

    contextIoPtr = NULL;
    /*
     *  If this is a global variable, handle it in the usual
     *  Tcl manner.
     */
    if (flags & TCL_GLOBAL_ONLY) {
        return TCL_CONTINUE;
    }

    /*
     *  See if this is a formal parameter in the current proc scope.
     *  If so, that variable has precedence.
     */
    if ((strstr(name,"::") == NULL) &&
            Itcl_IsCallFrameArgument(interp, name)) {
        return TCL_CONTINUE;
    }

    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp,
                ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses, (char *)nsPtr);
    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }
    iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);

    /*
     *  See if the variable is a known data member and accessible.
     */
    hPtr = ItclResolveVarEntry(iclsPtr, name);
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
	    *rPtr = (Tcl_Var)Tcl_GetHashValue(hPtr);
            return TCL_OK;
	}
    }

    /*
     *  If this is an instance variable, then we have to
     *  find the object context,
     */
    if (TCL_ERROR == Itcl_GetContext(interp, &iclsPtr, &contextIoPtr)
	    || (contextIoPtr == NULL)) {
	return TCL_CONTINUE;
    }
    /* Check that the object hasn't already been destroyed. */
    hPtr = Tcl_FindHashEntry(&infoPtr->objects, (char *)contextIoPtr);
    if (hPtr == NULL) {
	return TCL_CONTINUE;
    }
        if (contextIoPtr->iclsPtr != vlookup->ivPtr->iclsPtr) {
	    if (strcmp(Tcl_GetString(vlookup->ivPtr->namePtr), "this") == 0) {
                hPtr = ItclResolveVarEntry(contextIoPtr->iclsPtr,
                    Tcl_GetString(vlookup->ivPtr->namePtr));

                if (hPtr != NULL) {
                    vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
                }
            }
        }
        hPtr = Tcl_FindHashEntry(&contextIoPtr->objectVariables,
                (char *)vlookup->ivPtr);

    if (hPtr == NULL) {
        return TCL_CONTINUE;
    }
    if (strcmp(name, "this") == 0) {
        Tcl_Var varPtr;
        Tcl_DString buffer;

	Tcl_DStringInit(&buffer);
	Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	Tcl_DStringAppend(&buffer,
		(Tcl_GetObjectNamespace(contextIoPtr->oPtr)->fullName), -1);
	if (vlookup->ivPtr->iclsPtr->nsPtr == NULL) {
	    /* deletion of class is running */
	    Tcl_DStringAppend(&buffer,
	             Tcl_GetCurrentNamespace(interp)->fullName, -1);
        } else {
	    Tcl_DStringAppend(&buffer,
	             vlookup->ivPtr->iclsPtr->nsPtr->fullName, -1);
	}
	Tcl_DStringAppend(&buffer, "::this", 6);
	varPtr = Itcl_FindNamespaceVar(interp, Tcl_DStringValue(&buffer), NULL, 0);
        if (varPtr != NULL) {
            *rPtr = varPtr;
	    return TCL_OK;
        }
    }
    if (strcmp(name, "itcl_options") == 0) {
        Tcl_Var varPtr;
        Tcl_DString buffer;

	Tcl_DStringInit(&buffer);
	Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	Tcl_DStringAppend(&buffer,
		(Tcl_GetObjectNamespace(contextIoPtr->oPtr)->fullName), -1);
	Tcl_DStringAppend(&buffer, "::itcl_options", -1);
	varPtr = Itcl_FindNamespaceVar(interp, Tcl_DStringValue(&buffer), NULL, 0);
	Tcl_DStringFree(&buffer);
        if (varPtr != NULL) {
            *rPtr = varPtr;
	    return TCL_OK;
        }
    }
    if (strcmp(name, "itcl_option_components") == 0) {
        Tcl_Var varPtr;
        Tcl_DString buffer;

	Tcl_DStringInit(&buffer);
	Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	Tcl_DStringAppend(&buffer,
		(Tcl_GetObjectNamespace(contextIoPtr->oPtr)->fullName), -1);
	Tcl_DStringAppend(&buffer, "::itcl_option_components", -1);
	varPtr = Itcl_FindNamespaceVar(interp, Tcl_DStringValue(&buffer), NULL, 0);
	Tcl_DStringFree(&buffer);
        if (varPtr != NULL) {
            *rPtr = varPtr;
	    return TCL_OK;
        }
    }
    if (hPtr != NULL) {
        *rPtr = (Tcl_Var)Tcl_GetHashValue(hPtr);
        return TCL_OK;
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
Itcl_ClassCompiledVarResolver(
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
    iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
    /*
     *  Copy the name to local storage so we can NULL terminate it.
     *  If the name is long, allocate extra space for it.
     */
    if ((unsigned int)length < sizeof(storage)) {
        buffer = storage;
    } else {
        buffer = (char*)ckalloc((unsigned)(length+1));
    }
    memcpy((void*)buffer, (void*)name, (size_t)length);
    buffer[length] = '\0';

    hPtr = ItclResolveVarEntry(iclsPtr, buffer);

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
    (*rPtr)->fetchProc = ItclClassRuntimeVarResolver;
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
ItclClassRuntimeVarResolver(
    Tcl_Interp *interp,               /* current interpreter */
    Tcl_ResolvedVarInfo *resVarInfo)  /* contains ItclVarLookup rep
                                       * for variable */
{
    ItclVarLookup *vlookup = ((ItclResolvedVarInfo*)resVarInfo)->vlookup;
    ItclClass *iclsPtr;
    ItclObject *contextIoPtr;
    Tcl_HashEntry *hPtr;

    /*
     *  If this is a common data member, then the associated
     *  variable is known directly.
     */
    if ((vlookup->ivPtr->flags & ITCL_COMMON) != 0) {
	hPtr = Tcl_FindHashEntry(&vlookup->ivPtr->iclsPtr->classCommons,
	        (char *)vlookup->ivPtr);
	if (hPtr != NULL) {
	    return (Tcl_Var)Tcl_GetHashValue(hPtr);
	}
    }

    /*
     *  Otherwise, get the current object context and find the
     *  variable in its data table.
     *
     *  TRICKY NOTE:  Get the index for this variable using the
     *    virtual table for the MOST-SPECIFIC class.
     */
    if (TCL_ERROR == Itcl_GetContext(interp, &iclsPtr, &contextIoPtr)
	    || (contextIoPtr == NULL)) {
	return NULL;
    }

        if (contextIoPtr->iclsPtr != vlookup->ivPtr->iclsPtr) {
	    if (strcmp(Tcl_GetString(vlookup->ivPtr->namePtr), "this") == 0) {
	        /* only for the this variable we need the one of the
		 * contextIoPtr class */
                hPtr = ItclResolveVarEntry(contextIoPtr->iclsPtr,
                        Tcl_GetString(vlookup->ivPtr->namePtr));

                if (hPtr != NULL) {
                    vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
	        }
	    }
        }
        hPtr = Tcl_FindHashEntry(&contextIoPtr->objectVariables,
                (char *)vlookup->ivPtr);
        if (strcmp(Tcl_GetString(vlookup->ivPtr->namePtr), "this") == 0) {
            Tcl_Var varPtr;
            Tcl_DString buffer;

	    Tcl_DStringInit(&buffer);
	    Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	    Tcl_DStringAppend(&buffer,
		    (Tcl_GetObjectNamespace(contextIoPtr->oPtr)->fullName), -1);
	    if (vlookup->ivPtr->iclsPtr->nsPtr == NULL) {
	        Tcl_DStringAppend(&buffer,
	                Tcl_GetCurrentNamespace(interp)->fullName, -1);
	    } else {
	        Tcl_DStringAppend(&buffer,
	                vlookup->ivPtr->iclsPtr->nsPtr->fullName, -1);
	    }
	    Tcl_DStringAppend(&buffer, "::this", 6);
	    varPtr = Itcl_FindNamespaceVar(interp, Tcl_DStringValue(&buffer),
	            NULL, 0);
            if (varPtr != NULL) {
	        return varPtr;
            }
        }
        if (strcmp(Tcl_GetString(vlookup->ivPtr->namePtr),
	        "itcl_options") == 0) {
            Tcl_Var varPtr;
            Tcl_DString buffer;

	    Tcl_DStringInit(&buffer);
	    Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	    Tcl_DStringAppend(&buffer,
		    (Tcl_GetObjectNamespace(contextIoPtr->oPtr)->fullName), -1);
	    Tcl_DStringAppend(&buffer, "::itcl_options", -1);
	    varPtr = Itcl_FindNamespaceVar(interp, Tcl_DStringValue(&buffer),
	            NULL, 0);
	    Tcl_DStringFree(&buffer);
            if (varPtr != NULL) {
	        return varPtr;
            }
        }
        if (strcmp(Tcl_GetString(vlookup->ivPtr->namePtr),
	        "itcl_option_components") == 0) {
            Tcl_Var varPtr;
            Tcl_DString buffer;

	    Tcl_DStringInit(&buffer);
	    Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
	    Tcl_DStringAppend(&buffer,
		    (Tcl_GetObjectNamespace(contextIoPtr->oPtr)->fullName), -1);
	    Tcl_DStringAppend(&buffer, "::itcl_option_components", -1);
	    varPtr = Itcl_FindNamespaceVar(interp, Tcl_DStringValue(&buffer),
	            NULL, 0);
	    Tcl_DStringFree(&buffer);
            if (varPtr != NULL) {
	        return varPtr;
            }
        }
        if (hPtr != NULL) {
            return (Tcl_Var)Tcl_GetHashValue(hPtr);
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
Itcl_ParseVarResolver(
    Tcl_Interp *interp,        /* current interpreter */
    const char* name,        /* name of the variable being accessed */
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
    hPtr = ItclResolveVarEntry(iclsPtr, name);
    if (!hPtr) {
	return TCL_CONTINUE;
    }

    vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);

    if ((vlookup->ivPtr->flags & ITCL_COMMON) == 0) {
	return TCL_CONTINUE;
    }
    if (!vlookup->accessible) {
        Tcl_AppendResult(interp,
            "can't access \"", name, "\": ",
            Itcl_ProtectionStr(vlookup->ivPtr->protection),
            " variable", NULL);
        return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&vlookup->ivPtr->iclsPtr->classCommons,
        (char *)vlookup->ivPtr);
    if (!hPtr) {
	return TCL_CONTINUE;
    }
    *rPtr = (Tcl_Var)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}



int
ItclSetParserResolver(
    Tcl_Namespace *nsPtr)
{
    Itcl_SetNamespaceResolvers(nsPtr, NULL,
            Itcl_ParseVarResolver, NULL);
    return TCL_OK;
}
