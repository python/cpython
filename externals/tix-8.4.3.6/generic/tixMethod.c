/*
 * tixMethod.c --
 *
 *	Handle the calling of class methods.
 *
 *	Implements the basic OOP class mechanism for the Tix Intrinsics.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixMethod.c,v 1.5 2008/02/28 04:05:29 hobbs Exp $
 */

#include <tk.h>
#include <tixPort.h>
#include <tixInt.h>

#define GetMethodTable(interp) \
    (TixGetHashTable(interp, "tixMethodTab", MethodTableDeleteProc, \
            TCL_STRING_KEYS))

static int		Tix_CallMethodByContext _ANSI_ARGS_((
			    Tcl_Interp *interp, CONST84 char *context,
			    CONST84 char *widRec, CONST84 char *method,
			    int argc, CONST84 char **argv));
static void		Tix_RestoreContext _ANSI_ARGS_((
			    Tcl_Interp *interp, CONST84 char *widRec,
			    CONST84 char *oldContext));
static void		Tix_SetContext _ANSI_ARGS_((
			    Tcl_Interp *interp, CONST84 char *widRec,
			    CONST84 char *newContext));
static char *		Tix_SaveContext _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST84 char *widRec));
static void		MethodTableDeleteProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));

/*
 *
 * argv[1] = widget record 
 * argv[2] = method
 * argv[3+] = args
 *
 */
TIX_DEFINE_CMD(Tix_CallMethodCmd)
{
    CONST84 char *context;
    CONST84 char *newContext;
    CONST84 char *widRec = argv[1];
    CONST84 char *method = argv[2];
    int    result;

    if (argc<3) {
	return Tix_ArgcError(interp, argc, argv, 1, "w method ...");
    }
 
    if ((context = GET_RECORD(interp, widRec, "className")) == NULL) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "invalid object reference \"", widRec,
	    "\"", (char*)NULL);
	return TCL_ERROR;
    }

    newContext = Tix_FindMethod(interp, context, method);

    if (newContext) {
	result = Tix_CallMethodByContext(interp, newContext, widRec, method,
	    argc-3, argv+3);
    } else {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "cannot call method \"", method,
	    "\" for context \"", context, "\".", (char*)NULL);
	Tcl_SetVar(interp, "errorInfo", Tcl_GetStringResult(interp),
		TCL_GLOBAL_ONLY);
	result = TCL_ERROR;
    }

    return result;
}

/*
 *
 * argv[1] = widget record 
 * argv[2] = method
 * argv[3+] = args
 *
 */
TIX_DEFINE_CMD(Tix_ChainMethodCmd)
{
    CONST84 char *context;
    CONST84 char *superClassContext;
    CONST84 char *newContext;
    CONST84 char *widRec = argv[1];
    CONST84 char *method = argv[2];
    int    result;

    if (argc<3) {
	return Tix_ArgcError(interp, argc, argv, 1, "w method ...");
    }

    if ((context = Tix_GetContext(interp, widRec)) == NULL) {
	return TCL_ERROR;
    }

    if (Tix_SuperClass(interp, context, &superClassContext) != TCL_OK) {
	return TCL_ERROR;
    }

    if (superClassContext == NULL) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "no superclass exists for context \"",
	    context, "\".", (char*)NULL);
	result = TCL_ERROR;
	goto done;
    }

    newContext = Tix_FindMethod(interp, superClassContext, method);

    if (newContext) {
	result = Tix_CallMethodByContext(interp, newContext, widRec,
	    method, argc-3, argv+3);
    } else {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "cannot chain method \"", method,
	    "\" for context \"", context, "\".", (char*)NULL);
	Tcl_SetVar(interp, "errorInfo", Tcl_GetStringResult(interp),
		TCL_GLOBAL_ONLY);
	result = TCL_ERROR;
	goto done;
    }

  done:
    return result;
}

/*
 *
 * argv[1] = widget record 
 * argv[2] = class (context)
 * argv[3] = method
 *
 */
TIX_DEFINE_CMD(Tix_GetMethodCmd)
{
    CONST84 char *newContext;
    CONST84 char *context= argv[2];
    CONST84 char *method = argv[3];
    CONST84 char *cmdName;

    if (argc!=4) {
	return Tix_ArgcError(interp, argc, argv, 1, "w class method");
    }

    newContext = Tix_FindMethod(interp, context, method);

    if (newContext) {
	cmdName = Tix_GetMethodFullName(newContext, method);
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, cmdName, NULL);
	ckfree((char *) cmdName);
    } else {
	Tcl_SetResult(interp, "", TCL_STATIC);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * Tix_FindMethod
 *
 *	Starting with class "context", find the first class that defines
 * the method. This class must be the same as the class "context" or
 * a superclass of the class "context".
 */
CONST84 char *
Tix_FindMethod(interp, context, method)
    Tcl_Interp *interp;
    CONST84 char *context;
    CONST84 char *method;
{
    CONST84 char      *theContext;
    int    	isNew;
    CONST84 char      *key;
    Tcl_HashEntry *hashPtr;

    key = Tix_GetMethodFullName(context, method);
    hashPtr = Tcl_CreateHashEntry(GetMethodTable(interp), key, &isNew);
    ckfree((char *) key);

    if (!isNew) {
	theContext = (char *) Tcl_GetHashValue(hashPtr);
    } else {
	for (theContext = context; theContext;) {
	    if (Tix_ExistMethod(interp, theContext, method)) {
		break;
	    }
	    /* Go to its superclass and see if it has the method */
	    if (Tix_SuperClass(interp, theContext, &theContext) != TCL_OK) {
		return NULL;
	    }
	    if (theContext == NULL) {
		return NULL;
	    }
	}

	if (theContext != NULL) {
	    /*
	     * theContext may point to the stack. We have to put it
	     * in some more permanent place.
	     */
	    theContext = tixStrDup(theContext);
	}
	Tcl_SetHashValue(hashPtr, (char*)theContext);
    }

    return theContext;
}

/*
 *----------------------------------------------------------------------
 * Tix_CallMethod
 *
 *	Starting with class "context", find the first class that defines
 *      the method. If found, call this method.
 *
 * Results:
 *	A standard Tcl completion code (TCL_OK or TCL_ERROR).  Also
 *	leaves information in the interp's result. 
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Tix_CallMethod(interp, context, widRec, method, argc, argv, foundPtr)
    Tcl_Interp *interp;	/* Tcl interpreter to execute the method in */
    CONST84 char *context;	/* context */
    CONST84 char *widRec;	/* Name of the widget record */
    CONST84 char *method;	/* Name of the method */
    int argc;			/* Number of arguments passed to the method */
    CONST84 char **argv;	/* Arguments */
    int *foundPtr;		/* If non-NULL. returns whether the
				 * method has been found */
{
    CONST84 char *targetContext;

    targetContext = Tix_FindMethod(interp, context, method);

    if (foundPtr != NULL) {
        *foundPtr =  (targetContext != NULL);
    }

    if (targetContext != NULL) {
	return Tix_CallMethodByContext(interp, targetContext, widRec, method,
	    argc, argv);
    } else {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "cannot call method \"", method,
	    "\" for context \"", context, "\".", (char*)NULL);
	Tcl_SetVar(interp, "errorInfo", Tcl_GetStringResult(interp),
		TCL_GLOBAL_ONLY);
	return TCL_ERROR;
    }
}

/*----------------------------------------------------------------------
 * Tix_FindConfigSpec
 *
 *	Starting with class "classRec", find the first class that defines
 * the option flag. This class must be the same as the class "classRec" or
 * a superclass of the class "classRec".
 */

/* save the old context: calling a method of a superclass will
 * change the context of a widget.
 */
static char *Tix_SaveContext(interp, widRec)
    Tcl_Interp *interp;
    CONST84 char *widRec;
{
    CONST84 char *context;

    if ((context = GET_RECORD(interp, widRec, "context")) == NULL) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "invalid object reference \"", widRec,
	    "\"", (char*)NULL);
	return NULL;
    }
    else {
	return tixStrDup(context);
    }
}

static void Tix_RestoreContext(interp, widRec, oldContext)
    Tcl_Interp *interp;
    CONST84 char *widRec;
    CONST84 char *oldContext;
{
    SET_RECORD(interp, widRec, "context", oldContext);
    ckfree((char *) oldContext);
}

static void Tix_SetContext(interp, widRec, newContext)
    Tcl_Interp *interp;
    CONST84 char *widRec;
    CONST84 char *newContext;
{
    SET_RECORD(interp, widRec, "context", newContext);
}


CONST84 char *
Tix_GetContext(interp, widRec)
    Tcl_Interp *interp;
    CONST84 char *widRec;
{
    CONST84 char *context;

    if ((context = GET_RECORD(interp, widRec, "context")) == NULL) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "invalid object reference \"", widRec,
	    "\"", (char*)NULL);
	return NULL;
    } else {
	return context;
    }
}

int
Tix_SuperClass(interp, class, superClass_ret)
    Tcl_Interp *interp;
    CONST84 char *class;
    CONST84 char **superClass_ret;
{
    CONST84 char *superclass;

    if ((superclass = GET_RECORD(interp, class, "superClass")) == NULL) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "invalid class \"", class,
	    "\"; ", (char*)NULL);
	return TCL_ERROR;
    }

    if (strlen(superclass) == 0) {
	*superClass_ret = (char*) NULL;
    } else {
	*superClass_ret =  superclass;
    }

    return TCL_OK;
}

CONST84 char *
Tix_GetMethodFullName(context, method)
    CONST84 char *context;
    CONST84 char *method;
{
    char *buff;
    int    max;
    int    conLen;

    conLen = strlen(context);
    max = conLen + strlen(method) + 3;
    buff = (char*)ckalloc(max * sizeof(char));

    strcpy(buff, context);
    strcpy(buff+conLen, ":");
    strcpy(buff+conLen+1, method);

    return buff;
}

int Tix_ExistMethod(interp, context, method)
    Tcl_Interp *interp;
    CONST84 char *context;
    CONST84 char *method;
{
    CONST84 char *cmdName;
    Tcl_CmdInfo dummy;
    int exist;

    /*
     * TODO: does Tcl_GetCommandInfo check in global namespace??
     */

    cmdName = Tix_GetMethodFullName(context, method);
    exist = Tcl_GetCommandInfo(interp, cmdName, &dummy);

    if (!exist) {
	if (Tix_GlobalVarEval(interp, "auto_load ", cmdName, 
	        (char*)NULL)!= TCL_OK) {
	    goto done;
	}
	if (strcmp(Tcl_GetStringResult(interp), "1") == 0) {
	    exist = 1;
	}
    }

  done:
    ckfree((char *) cmdName);
    Tcl_SetResult(interp, NULL, TCL_STATIC);
    return exist;
}

/* %% There is a dirty version that uses the old argv, without having to
 * malloc a new argv.
 */
static int
Tix_CallMethodByContext(interp, context, widRec, method, argc, argv)
    Tcl_Interp *interp;
    CONST84 char *context;
    CONST84 char *widRec;
    CONST84 char *method;
    int    argc;
    CONST84 char **argv;
{
    CONST84 char  *cmdName;
    int     i, result;
    CONST84 char  *oldContext;
    CONST84 char **newArgv;

    if ((oldContext = Tix_SaveContext(interp, widRec)) == NULL) {
	return TCL_ERROR;
    }
    Tix_SetContext(interp, widRec, context);

    cmdName = Tix_GetMethodFullName(context, method);

    /* Create a new argv list */
    newArgv = (CONST84 char**)ckalloc((argc+2)*sizeof(char*));
    newArgv[0] = cmdName;
    newArgv[1] = widRec;
    for (i=0; i< argc; i++) {
	newArgv[i+2] = argv[i];
    }
    result = Tix_EvalArgv(interp, argc+2, newArgv);

    Tix_RestoreContext(interp, widRec, oldContext);
    ckfree((char*)newArgv);
    ckfree((char*)cmdName);

    return result;
}

/*
 * Deprecated: use Tcl_EvalObjv instead. Will be removed.
 */

int Tix_EvalArgv(interp, argc, argv)
    Tcl_Interp *interp;
    int argc;
    CONST84 char **argv;
{
    register Tcl_Obj *objPtr;
    register int i;
    int result;

#define NUM_ARGS 20
    Tcl_Obj *(objStorage[NUM_ARGS]);
    register Tcl_Obj **objv = objStorage;

    /*
     * TODO: callers to this method should be changed to use Tcl_EvalObjv
     * directly.
     */

    /*
     * Create the object argument array "objv". Make sure objv is large
     * enough to hold the objc arguments plus 1 extra for the zero
     * end-of-objv word.
     */

    if ((argc + 1) > NUM_ARGS) {
	objv = (Tcl_Obj **)
	    ckalloc((unsigned)(argc + 1) * sizeof(Tcl_Obj *));
    }

    for (i = 0;  i < argc;  i++) {
	objv[i] = Tcl_NewStringObj(argv[i], -1);
	Tcl_IncrRefCount(objv[i]);
    }
    objv[argc] = NULL;

    result = Tcl_EvalObjv(interp, argc, objv, TCL_EVAL_GLOBAL);

    /*
     * Get the interpreter's string result. We do this because some
     * of our callers expect to find result inside interp->result.
     */

    Tcl_GetStringResult(interp);

    /*
     * Decrement the ref counts on the objv elements since we are done
     * with them.
     */

    for (i = 0;  i < argc;  i++) {
	objPtr = objv[i];
	Tcl_DecrRefCount(objPtr);
    }

    /*
     * Free the objv array if malloc'ed storage was used.
     */

    if (objv != objStorage) {
	ckfree((char *) objv);
    }
    return result;
#undef NUM_ARGS
}

char *
Tix_FindPublicMethod(interp, cPtr, method)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char *method;
{
    int i;
    unsigned int len = strlen(method);

    for (i=0; i<cPtr->nMethods; i++) {
	if (cPtr->methods[i][0] == method[0] &&
	    strncmp(cPtr->methods[i], method, len)==0) {
	    return cPtr->methods[i];
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 * MethodTableDeleteProc --
 *
 *	This procedure is called when the interp is about to
 *	be deleted. It cleans up the hash entries and destroys the hash
 *	table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All class method contexts are deleted for this interpreter.
 *----------------------------------------------------------------------
 */

static void
MethodTableDeleteProc(clientData, interp)
    ClientData clientData;
    Tcl_Interp *interp;
{
    Tcl_HashTable *methodTablePtr = (Tcl_HashTable*)clientData;
    Tcl_HashSearch hashSearch;
    Tcl_HashEntry *hashPtr;
    CONST84 char *context;

    for (hashPtr = Tcl_FirstHashEntry(methodTablePtr, &hashSearch);
	 hashPtr;
	 hashPtr = Tcl_NextHashEntry(&hashSearch)) {

	context = (char*)Tcl_GetHashValue(hashPtr);
	if (context) {
	    ckfree((char *) context);
	}
	Tcl_DeleteHashEntry(hashPtr);
    }
    Tcl_DeleteHashTable(methodTablePtr);
    ckfree((char*)methodTablePtr);
}
