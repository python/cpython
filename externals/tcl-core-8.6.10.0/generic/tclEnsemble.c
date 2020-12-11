/*
 * tclEnsemble.c --
 *
 *	Contains support for ensembles (see TIP#112), which provide simple
 *	mechanism for creating composite commands on top of namespaces.
 *
 * Copyright (c) 2005-2013 Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"

/*
 * Declarations for functions local to this file:
 */

static inline Tcl_Obj *	NewNsObj(Tcl_Namespace *namespacePtr);
static inline int	EnsembleUnknownCallback(Tcl_Interp *interp,
			    EnsembleConfig *ensemblePtr, int objc,
			    Tcl_Obj *const objv[], Tcl_Obj **prefixObjPtr);
static int		NsEnsembleImplementationCmd(ClientData clientData,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static int		NsEnsembleImplementationCmdNR(ClientData clientData,
			    Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
static void		BuildEnsembleConfig(EnsembleConfig *ensemblePtr);
static int		NsEnsembleStringOrder(const void *strPtr1,
			    const void *strPtr2);
static void		DeleteEnsembleConfig(ClientData clientData);
static void		MakeCachedEnsembleCommand(Tcl_Obj *objPtr,
			    EnsembleConfig *ensemblePtr, Tcl_HashEntry *hPtr,
			    Tcl_Obj *fix);
static void		FreeEnsembleCmdRep(Tcl_Obj *objPtr);
static void		DupEnsembleCmdRep(Tcl_Obj *objPtr, Tcl_Obj *copyPtr);
static void		CompileToInvokedCommand(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, Tcl_Obj *replacements,
			    Command *cmdPtr, CompileEnv *envPtr);
static int		CompileBasicNArgCommand(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, Command *cmdPtr,
			    CompileEnv *envPtr);

static Tcl_NRPostProc	FreeER;

/*
 * The lists of subcommands and options for the [namespace ensemble] command.
 */

static const char *const ensembleSubcommands[] = {
    "configure", "create", "exists", NULL
};
enum EnsSubcmds {
    ENS_CONFIG, ENS_CREATE, ENS_EXISTS
};

static const char *const ensembleCreateOptions[] = {
    "-command", "-map", "-parameters", "-prefixes", "-subcommands",
    "-unknown", NULL
};
enum EnsCreateOpts {
    CRT_CMD, CRT_MAP, CRT_PARAM, CRT_PREFIX, CRT_SUBCMDS, CRT_UNKNOWN
};

static const char *const ensembleConfigOptions[] = {
    "-map", "-namespace", "-parameters", "-prefixes", "-subcommands",
    "-unknown", NULL
};
enum EnsConfigOpts {
    CONF_MAP, CONF_NAMESPACE, CONF_PARAM, CONF_PREFIX, CONF_SUBCMDS,
    CONF_UNKNOWN
};

/*
 * This structure defines a Tcl object type that contains a reference to an
 * ensemble subcommand (e.g. the "length" in [string length ab]). It is used
 * to cache the mapping between the subcommand itself and the real command
 * that implements it.
 */

static const Tcl_ObjType ensembleCmdType = {
    "ensembleCommand",		/* the type's name */
    FreeEnsembleCmdRep,		/* freeIntRepProc */
    DupEnsembleCmdRep,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

/*
 * The internal rep for caching ensemble subcommand lookups and
 * spell corrections.
 */

typedef struct {
    int epoch;                  /* Used to confirm when the data in this
                                 * really structure matches up with the
                                 * ensemble. */
    Command *token;             /* Reference to the command for which this
                                 * structure is a cache of the resolution. */
    Tcl_Obj *fix;               /* Corrected spelling, if needed. */
    Tcl_HashEntry *hPtr;        /* Direct link to entry in the subcommand
                                 * hash table. */
} EnsembleCmdRep;


static inline Tcl_Obj *
NewNsObj(
    Tcl_Namespace *namespacePtr)
{
    register Namespace *nsPtr = (Namespace *) namespacePtr;

    if (namespacePtr == TclGetGlobalNamespace(nsPtr->interp)) {
	return Tcl_NewStringObj("::", 2);
    } else {
	return Tcl_NewStringObj(nsPtr->fullName, -1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclNamespaceEnsembleCmd --
 *
 *	Invoked to implement the "namespace ensemble" command that creates and
 *	manipulates ensembles built on top of namespaces. Handles the
 *	following syntax:
 *
 *	    namespace ensemble name ?dictionary?
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	Creates the ensemble for the namespace if one did not previously
 *	exist. Alternatively, alters the way that the ensemble's subcommand =>
 *	implementation prefix is configured.
 *
 *----------------------------------------------------------------------
 */

int
TclNamespaceEnsembleCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_Namespace *namespacePtr;
    Namespace *nsPtr = (Namespace *) TclGetCurrentNamespace(interp), *cxtPtr,
    	*foundNsPtr, *altFoundNsPtr, *actualCxtPtr;
    Tcl_Command token;
    Tcl_DictSearch search;
    Tcl_Obj *listObj;
    const char *simpleName;
    int index, done;

    if (nsPtr == NULL || nsPtr->flags & NS_DYING) {
	if (!Tcl_InterpDeleted(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "tried to manipulate ensemble of deleted namespace",
		    -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "DEAD", NULL);
	}
	return TCL_ERROR;
    }

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], ensembleSubcommands,
	    "subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum EnsSubcmds) index) {
    case ENS_CREATE: {
	const char *name;
	int len, allocatedMapFlag = 0;
	/*
	 * Defaults
	 */
	Tcl_Obj *subcmdObj = NULL;
	Tcl_Obj *mapObj = NULL;
	int permitPrefix = 1;
	Tcl_Obj *unknownObj = NULL;
	Tcl_Obj *paramObj = NULL;

	/*
	 * Check that we've got option-value pairs... [Bug 1558654]
	 */

	if (objc & 1) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?option value ...?");
	    return TCL_ERROR;
	}
	objv += 2;
	objc -= 2;

	name = nsPtr->name;
	cxtPtr = (Namespace *) nsPtr->parentPtr;

	/*
	 * Parse the option list, applying type checks as we go. Note that we
	 * are not incrementing any reference counts in the objects at this
	 * stage, so the presence of an option multiple times won't cause any
	 * memory leaks.
	 */

	for (; objc>1 ; objc-=2,objv+=2) {
	    if (Tcl_GetIndexFromObj(interp, objv[0], ensembleCreateOptions,
		    "option", 0, &index) != TCL_OK) {
		if (allocatedMapFlag) {
		    Tcl_DecrRefCount(mapObj);
		}
		return TCL_ERROR;
	    }
	    switch ((enum EnsCreateOpts) index) {
	    case CRT_CMD:
		name = TclGetString(objv[1]);
		cxtPtr = nsPtr;
		continue;
	    case CRT_SUBCMDS:
		if (TclListObjLength(interp, objv[1], &len) != TCL_OK) {
		    if (allocatedMapFlag) {
			Tcl_DecrRefCount(mapObj);
		    }
		    return TCL_ERROR;
		}
		subcmdObj = (len > 0 ? objv[1] : NULL);
		continue;
	    case CRT_PARAM:
		if (TclListObjLength(interp, objv[1], &len) != TCL_OK) {
		    if (allocatedMapFlag) {
			Tcl_DecrRefCount(mapObj);
		    }
		    return TCL_ERROR;
		}
		paramObj = (len > 0 ? objv[1] : NULL);
		continue;
	    case CRT_MAP: {
		Tcl_Obj *patchedDict = NULL, *subcmdWordsObj;

		/*
		 * Verify that the map is sensible.
		 */

		if (Tcl_DictObjFirst(interp, objv[1], &search,
			&subcmdWordsObj, &listObj, &done) != TCL_OK) {
		    if (allocatedMapFlag) {
			Tcl_DecrRefCount(mapObj);
		    }
		    return TCL_ERROR;
		}
		if (done) {
		    mapObj = NULL;
		    continue;
		}
		do {
		    Tcl_Obj **listv;
		    const char *cmd;

		    if (TclListObjGetElements(interp, listObj, &len,
			    &listv) != TCL_OK) {
			Tcl_DictObjDone(&search);
			if (patchedDict) {
			    Tcl_DecrRefCount(patchedDict);
			}
			if (allocatedMapFlag) {
			    Tcl_DecrRefCount(mapObj);
			}
			return TCL_ERROR;
		    }
		    if (len < 1) {
			Tcl_SetObjResult(interp, Tcl_NewStringObj(
				"ensemble subcommand implementations "
				"must be non-empty lists", -1));
			Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE",
				"EMPTY_TARGET", NULL);
			Tcl_DictObjDone(&search);
			if (patchedDict) {
			    Tcl_DecrRefCount(patchedDict);
			}
			if (allocatedMapFlag) {
			    Tcl_DecrRefCount(mapObj);
			}
			return TCL_ERROR;
		    }
		    cmd = TclGetString(listv[0]);
		    if (!(cmd[0] == ':' && cmd[1] == ':')) {
			Tcl_Obj *newList = Tcl_NewListObj(len, listv);
			Tcl_Obj *newCmd = NewNsObj((Tcl_Namespace *) nsPtr);

			if (nsPtr->parentPtr) {
			    Tcl_AppendStringsToObj(newCmd, "::", NULL);
			}
			Tcl_AppendObjToObj(newCmd, listv[0]);
			Tcl_ListObjReplace(NULL, newList, 0, 1, 1, &newCmd);
			if (patchedDict == NULL) {
			    patchedDict = Tcl_DuplicateObj(objv[1]);
			}
			Tcl_DictObjPut(NULL, patchedDict, subcmdWordsObj,
				newList);
		    }
		    Tcl_DictObjNext(&search, &subcmdWordsObj,&listObj, &done);
		} while (!done);

		if (allocatedMapFlag) {
		    Tcl_DecrRefCount(mapObj);
		}
		mapObj = (patchedDict ? patchedDict : objv[1]);
		if (patchedDict) {
		    allocatedMapFlag = 1;
		}
		continue;
	    }
	    case CRT_PREFIX:
		if (Tcl_GetBooleanFromObj(interp, objv[1],
			&permitPrefix) != TCL_OK) {
		    if (allocatedMapFlag) {
			Tcl_DecrRefCount(mapObj);
		    }
		    return TCL_ERROR;
		}
		continue;
	    case CRT_UNKNOWN:
		if (TclListObjLength(interp, objv[1], &len) != TCL_OK) {
		    if (allocatedMapFlag) {
			Tcl_DecrRefCount(mapObj);
		    }
		    return TCL_ERROR;
		}
		unknownObj = (len > 0 ? objv[1] : NULL);
		continue;
	    }
	}

	TclGetNamespaceForQualName(interp, name, cxtPtr,
	TCL_CREATE_NS_IF_UNKNOWN, &foundNsPtr, &altFoundNsPtr, &actualCxtPtr,
	&simpleName);

	/*
	 * Create the ensemble. Note that this might delete another ensemble
	 * linked to the same namespace, so we must be careful. However, we
	 * should be OK because we only link the namespace into the list once
	 * we've created it (and after any deletions have occurred.)
	 */

	token = TclCreateEnsembleInNs(interp, simpleName,
	     (Tcl_Namespace *) foundNsPtr, (Tcl_Namespace *) nsPtr,
	     (permitPrefix ? TCL_ENSEMBLE_PREFIX : 0));
	Tcl_SetEnsembleSubcommandList(interp, token, subcmdObj);
	Tcl_SetEnsembleMappingDict(interp, token, mapObj);
	Tcl_SetEnsembleUnknownHandler(interp, token, unknownObj);
	Tcl_SetEnsembleParameterList(interp, token, paramObj);

	/*
	 * Tricky! Must ensure that the result is not shared (command delete
	 * traces could have corrupted the pristine object that we started
	 * with). [Snit test rename-1.5]
	 */

	Tcl_ResetResult(interp);
	Tcl_GetCommandFullName(interp, token, Tcl_GetObjResult(interp));
	return TCL_OK;
    }

    case ENS_EXISTS:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "cmdname");
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
		Tcl_FindEnsemble(interp, objv[2], 0) != NULL));
	return TCL_OK;

    case ENS_CONFIG:
	if (objc < 3 || (objc != 4 && !(objc & 1))) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "cmdname ?-option value ...? ?arg ...?");
	    return TCL_ERROR;
	}
	token = Tcl_FindEnsemble(interp, objv[2], TCL_LEAVE_ERR_MSG);
	if (token == NULL) {
	    return TCL_ERROR;
	}

	if (objc == 4) {
	    Tcl_Obj *resultObj = NULL;		/* silence gcc 4 warning */

	    if (Tcl_GetIndexFromObj(interp, objv[3], ensembleConfigOptions,
		    "option", 0, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum EnsConfigOpts) index) {
	    case CONF_SUBCMDS:
		Tcl_GetEnsembleSubcommandList(NULL, token, &resultObj);
		if (resultObj != NULL) {
		    Tcl_SetObjResult(interp, resultObj);
		}
		break;
	    case CONF_PARAM:
		Tcl_GetEnsembleParameterList(NULL, token, &resultObj);
		if (resultObj != NULL) {
		    Tcl_SetObjResult(interp, resultObj);
		}
		break;
	    case CONF_MAP:
		Tcl_GetEnsembleMappingDict(NULL, token, &resultObj);
		if (resultObj != NULL) {
		    Tcl_SetObjResult(interp, resultObj);
		}
		break;
	    case CONF_NAMESPACE:
		namespacePtr = NULL;		/* silence gcc 4 warning */
		Tcl_GetEnsembleNamespace(NULL, token, &namespacePtr);
		Tcl_SetObjResult(interp, NewNsObj(namespacePtr));
		break;
	    case CONF_PREFIX: {
		int flags = 0;			/* silence gcc 4 warning */

		Tcl_GetEnsembleFlags(NULL, token, &flags);
		Tcl_SetObjResult(interp,
			Tcl_NewBooleanObj(flags & TCL_ENSEMBLE_PREFIX));
		break;
	    }
	    case CONF_UNKNOWN:
		Tcl_GetEnsembleUnknownHandler(NULL, token, &resultObj);
		if (resultObj != NULL) {
		    Tcl_SetObjResult(interp, resultObj);
		}
		break;
	    }
	} else if (objc == 3) {
	    /*
	     * Produce list of all information.
	     */

	    Tcl_Obj *resultObj, *tmpObj = NULL;	/* silence gcc 4 warning */
	    int flags = 0;			/* silence gcc 4 warning */

	    TclNewObj(resultObj);

	    /* -map option */
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(ensembleConfigOptions[CONF_MAP], -1));
	    Tcl_GetEnsembleMappingDict(NULL, token, &tmpObj);
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    (tmpObj != NULL) ? tmpObj : Tcl_NewObj());

	    /* -namespace option */
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(ensembleConfigOptions[CONF_NAMESPACE],
			    -1));
	    namespacePtr = NULL;		/* silence gcc 4 warning */
	    Tcl_GetEnsembleNamespace(NULL, token, &namespacePtr);
	    Tcl_ListObjAppendElement(NULL, resultObj, NewNsObj(namespacePtr));

	    /* -parameters option */
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(ensembleConfigOptions[CONF_PARAM], -1));
	    Tcl_GetEnsembleParameterList(NULL, token, &tmpObj);
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    (tmpObj != NULL) ? tmpObj : Tcl_NewObj());

	    /* -prefix option */
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(ensembleConfigOptions[CONF_PREFIX], -1));
	    Tcl_GetEnsembleFlags(NULL, token, &flags);
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewBooleanObj(flags & TCL_ENSEMBLE_PREFIX));

	    /* -subcommands option */
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(ensembleConfigOptions[CONF_SUBCMDS],-1));
	    Tcl_GetEnsembleSubcommandList(NULL, token, &tmpObj);
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    (tmpObj != NULL) ? tmpObj : Tcl_NewObj());

	    /* -unknown option */
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(ensembleConfigOptions[CONF_UNKNOWN],-1));
	    Tcl_GetEnsembleUnknownHandler(NULL, token, &tmpObj);
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    (tmpObj != NULL) ? tmpObj : Tcl_NewObj());

	    Tcl_SetObjResult(interp, resultObj);
	} else {
	    int len, allocatedMapFlag = 0;
	    Tcl_Obj *subcmdObj = NULL, *mapObj = NULL, *paramObj = NULL,
		    *unknownObj = NULL; /* Defaults, silence gcc 4 warnings */
	    int permitPrefix, flags = 0;	/* silence gcc 4 warning */

	    Tcl_GetEnsembleSubcommandList(NULL, token, &subcmdObj);
	    Tcl_GetEnsembleMappingDict(NULL, token, &mapObj);
	    Tcl_GetEnsembleParameterList(NULL, token, &paramObj);
	    Tcl_GetEnsembleUnknownHandler(NULL, token, &unknownObj);
	    Tcl_GetEnsembleFlags(NULL, token, &flags);
	    permitPrefix = (flags & TCL_ENSEMBLE_PREFIX) != 0;

	    objv += 3;
	    objc -= 3;

	    /*
	     * Parse the option list, applying type checks as we go. Note that
	     * we are not incrementing any reference counts in the objects at
	     * this stage, so the presence of an option multiple times won't
	     * cause any memory leaks.
	     */

	    for (; objc>0 ; objc-=2,objv+=2) {
		if (Tcl_GetIndexFromObj(interp, objv[0],ensembleConfigOptions,
			"option", 0, &index) != TCL_OK) {
		freeMapAndError:
		    if (allocatedMapFlag) {
			Tcl_DecrRefCount(mapObj);
		    }
		    return TCL_ERROR;
		}
		switch ((enum EnsConfigOpts) index) {
		case CONF_SUBCMDS:
		    if (TclListObjLength(interp, objv[1], &len) != TCL_OK) {
			goto freeMapAndError;
		    }
		    subcmdObj = (len > 0 ? objv[1] : NULL);
		    continue;
		case CONF_PARAM:
		    if (TclListObjLength(interp, objv[1], &len) != TCL_OK) {
			goto freeMapAndError;
		    }
		    paramObj = (len > 0 ? objv[1] : NULL);
		    continue;
		case CONF_MAP: {
		    Tcl_Obj *patchedDict = NULL, *subcmdWordsObj, **listv;
		    const char *cmd;

		    /*
		     * Verify that the map is sensible.
		     */

		    if (Tcl_DictObjFirst(interp, objv[1], &search,
			    &subcmdWordsObj, &listObj, &done) != TCL_OK) {
			goto freeMapAndError;
		    }
		    if (done) {
			mapObj = NULL;
			continue;
		    }
		    do {
			if (TclListObjGetElements(interp, listObj, &len,
				&listv) != TCL_OK) {
			    Tcl_DictObjDone(&search);
			    if (patchedDict) {
				Tcl_DecrRefCount(patchedDict);
			    }
			    goto freeMapAndError;
			}
			if (len < 1) {
			    Tcl_SetObjResult(interp, Tcl_NewStringObj(
				    "ensemble subcommand implementations "
				    "must be non-empty lists", -1));
			    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE",
				    "EMPTY_TARGET", NULL);
			    Tcl_DictObjDone(&search);
			    if (patchedDict) {
				Tcl_DecrRefCount(patchedDict);
			    }
			    goto freeMapAndError;
			}
			cmd = TclGetString(listv[0]);
			if (!(cmd[0] == ':' && cmd[1] == ':')) {
			    Tcl_Obj *newList = Tcl_DuplicateObj(listObj);
			    Tcl_Obj *newCmd = NewNsObj((Tcl_Namespace*)nsPtr);

			    if (nsPtr->parentPtr) {
				Tcl_AppendStringsToObj(newCmd, "::", NULL);
			    }
			    Tcl_AppendObjToObj(newCmd, listv[0]);
			    Tcl_ListObjReplace(NULL, newList, 0,1, 1,&newCmd);
			    if (patchedDict == NULL) {
				patchedDict = Tcl_DuplicateObj(objv[1]);
			    }
			    Tcl_DictObjPut(NULL, patchedDict, subcmdWordsObj,
				    newList);
			}
			Tcl_DictObjNext(&search, &subcmdWordsObj, &listObj,
				&done);
		    } while (!done);
		    if (allocatedMapFlag) {
			Tcl_DecrRefCount(mapObj);
		    }
		    mapObj = (patchedDict ? patchedDict : objv[1]);
		    if (patchedDict) {
			allocatedMapFlag = 1;
		    }
		    continue;
		}
		case CONF_NAMESPACE:
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "option -namespace is read-only", -1));
		    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "READ_ONLY",
			    NULL);
		    goto freeMapAndError;
		case CONF_PREFIX:
		    if (Tcl_GetBooleanFromObj(interp, objv[1],
			    &permitPrefix) != TCL_OK) {
			goto freeMapAndError;
		    }
		    continue;
		case CONF_UNKNOWN:
		    if (TclListObjLength(interp, objv[1], &len) != TCL_OK) {
			goto freeMapAndError;
		    }
		    unknownObj = (len > 0 ? objv[1] : NULL);
		    continue;
		}
	    }

	    /*
	     * Update the namespace now that we've finished the parsing stage.
	     */

	    flags = (permitPrefix ? flags|TCL_ENSEMBLE_PREFIX
		    : flags&~TCL_ENSEMBLE_PREFIX);
	    Tcl_SetEnsembleSubcommandList(interp, token, subcmdObj);
	    Tcl_SetEnsembleMappingDict(interp, token, mapObj);
	    Tcl_SetEnsembleParameterList(interp, token, paramObj);
	    Tcl_SetEnsembleUnknownHandler(interp, token, unknownObj);
	    Tcl_SetEnsembleFlags(interp, token, flags);
	}
	return TCL_OK;

    default:
	Tcl_Panic("unexpected ensemble command");
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreateEnsembleInNs --
 *
 *	Like Tcl_CreateEnsemble, but additionally accepts as an argument the
 *	name of the namespace to create the command in.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
TclCreateEnsembleInNs(
    Tcl_Interp *interp,

    const char *name,   /* Simple name of command to create (no */
			/* namespace components). */
    Tcl_Namespace       /* Name of namespace to create the command in. */
    *nameNsPtr,
    Tcl_Namespace
    *ensembleNsPtr,	/* Name of the namespace for the ensemble. */
    int flags
    )
{
    Namespace *nsPtr = (Namespace *) ensembleNsPtr;
    EnsembleConfig *ensemblePtr;
    Tcl_Command token;

    ensemblePtr = ckalloc(sizeof(EnsembleConfig));
    token = TclNRCreateCommandInNs(interp, name,
	(Tcl_Namespace *) nameNsPtr, NsEnsembleImplementationCmd,
	NsEnsembleImplementationCmdNR, ensemblePtr, DeleteEnsembleConfig);
    if (token == NULL) {
	ckfree(ensemblePtr);
	return NULL;
    }

    ensemblePtr->nsPtr = nsPtr;
    ensemblePtr->epoch = 0;
    Tcl_InitHashTable(&ensemblePtr->subcommandTable, TCL_STRING_KEYS);
    ensemblePtr->subcommandArrayPtr = NULL;
    ensemblePtr->subcmdList = NULL;
    ensemblePtr->subcommandDict = NULL;
    ensemblePtr->flags = flags;
    ensemblePtr->numParameters = 0;
    ensemblePtr->parameterList = NULL;
    ensemblePtr->unknownHandler = NULL;
    ensemblePtr->token = token;
    ensemblePtr->next = (EnsembleConfig *) nsPtr->ensembles;
    nsPtr->ensembles = (Tcl_Ensemble *) ensemblePtr;

    /*
     * Trigger an eventual recomputation of the ensemble command set. Note
     * that this is slightly tricky, as it means that we are not actually
     * counting the number of namespace export actions, but it is the simplest
     * way to go!
     */

    nsPtr->exportLookupEpoch++;

    if (flags & ENSEMBLE_COMPILE) {
	((Command *) ensemblePtr->token)->compileProc = TclCompileEnsemble;
    }

    return ensemblePtr->token;

}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateEnsemble
 *
 *	Create a simple ensemble attached to the given namespace.
 *
 *	Deprecated by TclCreateEnsembleInNs.
 *
 * Value
 *
 *	The token for the command created.
 *
 * Effect
 *	The ensemble is created and marked for compilation.
 *
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
Tcl_CreateEnsemble(
    Tcl_Interp *interp,
    const char *name,
    Tcl_Namespace *namespacePtr,
    int flags)
{
    Namespace *nsPtr = (Namespace *)namespacePtr, *foundNsPtr, *altNsPtr,
    	*actualNsPtr;
    const char * simpleName;

    if (nsPtr == NULL) {
	nsPtr = (Namespace *) TclGetCurrentNamespace(interp);
    }

    TclGetNamespaceForQualName(interp, name, nsPtr, TCL_CREATE_NS_IF_UNKNOWN,
    	&foundNsPtr, &altNsPtr, &actualNsPtr, &simpleName);
    return TclCreateEnsembleInNs(interp, simpleName,
	(Tcl_Namespace *) foundNsPtr, (Tcl_Namespace *) nsPtr, flags);
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetEnsembleSubcommandList --
 *
 *	Set the subcommand list for a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an ensemble
 *	or the subcommand list - if non-NULL - is not a list).
 *
 * Side effects:
 *	The ensemble is updated and marked for recompilation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetEnsembleSubcommandList(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj *subcmdList)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;
    Tcl_Obj *oldList;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"command is not an ensemble", -1));
	Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	return TCL_ERROR;
    }
    if (subcmdList != NULL) {
	int length;

	if (TclListObjLength(interp, subcmdList, &length) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (length < 1) {
	    subcmdList = NULL;
	}
    }

    ensemblePtr = cmdPtr->objClientData;
    oldList = ensemblePtr->subcmdList;
    ensemblePtr->subcmdList = subcmdList;
    if (subcmdList != NULL) {
	Tcl_IncrRefCount(subcmdList);
    }
    if (oldList != NULL) {
	TclDecrRefCount(oldList);
    }

    /*
     * Trigger an eventual recomputation of the ensemble command set. Note
     * that this is slightly tricky, as it means that we are not actually
     * counting the number of namespace export actions, but it is the simplest
     * way to go!
     */

    ensemblePtr->nsPtr->exportLookupEpoch++;

    /*
     * Special hack to make compiling of [info exists] work when the
     * dictionary is modified.
     */

    if (cmdPtr->compileProc != NULL) {
	((Interp *) interp)->compileEpoch++;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetEnsembleParameterList --
 *
 *	Set the parameter list for a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an ensemble
 *	or the parameter list - if non-NULL - is not a list).
 *
 * Side effects:
 *	The ensemble is updated and marked for recompilation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetEnsembleParameterList(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj *paramList)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;
    Tcl_Obj *oldList;
    int length;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"command is not an ensemble", -1));
	Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	return TCL_ERROR;
    }
    if (paramList == NULL) {
	length = 0;
    } else {
	if (TclListObjLength(interp, paramList, &length) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (length < 1) {
	    paramList = NULL;
	}
    }

    ensemblePtr = cmdPtr->objClientData;
    oldList = ensemblePtr->parameterList;
    ensemblePtr->parameterList = paramList;
    if (paramList != NULL) {
	Tcl_IncrRefCount(paramList);
    }
    if (oldList != NULL) {
	TclDecrRefCount(oldList);
    }
    ensemblePtr->numParameters = length;

    /*
     * Trigger an eventual recomputation of the ensemble command set. Note
     * that this is slightly tricky, as it means that we are not actually
     * counting the number of namespace export actions, but it is the simplest
     * way to go!
     */

    ensemblePtr->nsPtr->exportLookupEpoch++;

    /*
     * Special hack to make compiling of [info exists] work when the
     * dictionary is modified.
     */

    if (cmdPtr->compileProc != NULL) {
	((Interp *) interp)->compileEpoch++;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetEnsembleMappingDict --
 *
 *	Set the mapping dictionary for a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an ensemble
 *	or the mapping - if non-NULL - is not a dict).
 *
 * Side effects:
 *	The ensemble is updated and marked for recompilation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetEnsembleMappingDict(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj *mapDict)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;
    Tcl_Obj *oldDict;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"command is not an ensemble", -1));
	Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	return TCL_ERROR;
    }
    if (mapDict != NULL) {
	int size, done;
	Tcl_DictSearch search;
	Tcl_Obj *valuePtr;

	if (Tcl_DictObjSize(interp, mapDict, &size) != TCL_OK) {
	    return TCL_ERROR;
	}

	for (Tcl_DictObjFirst(NULL, mapDict, &search, NULL, &valuePtr, &done);
		!done; Tcl_DictObjNext(&search, NULL, &valuePtr, &done)) {
	    Tcl_Obj *cmdObjPtr;
	    const char *bytes;

	    if (Tcl_ListObjIndex(interp, valuePtr, 0, &cmdObjPtr) != TCL_OK) {
		Tcl_DictObjDone(&search);
		return TCL_ERROR;
	    }
	    bytes = TclGetString(cmdObjPtr);
	    if (bytes[0] != ':' || bytes[1] != ':') {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"ensemble target is not a fully-qualified command",
			-1));
		Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE",
			"UNQUALIFIED_TARGET", NULL);
		Tcl_DictObjDone(&search);
		return TCL_ERROR;
	    }
	}

	if (size < 1) {
	    mapDict = NULL;
	}
    }

    ensemblePtr = cmdPtr->objClientData;
    oldDict = ensemblePtr->subcommandDict;
    ensemblePtr->subcommandDict = mapDict;
    if (mapDict != NULL) {
	Tcl_IncrRefCount(mapDict);
    }
    if (oldDict != NULL) {
	TclDecrRefCount(oldDict);
    }

    /*
     * Trigger an eventual recomputation of the ensemble command set. Note
     * that this is slightly tricky, as it means that we are not actually
     * counting the number of namespace export actions, but it is the simplest
     * way to go!
     */

    ensemblePtr->nsPtr->exportLookupEpoch++;

    /*
     * Special hack to make compiling of [info exists] work when the
     * dictionary is modified.
     */

    if (cmdPtr->compileProc != NULL) {
	((Interp *) interp)->compileEpoch++;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetEnsembleUnknownHandler --
 *
 *	Set the unknown handler for a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an ensemble
 *	or the unknown handler - if non-NULL - is not a list).
 *
 * Side effects:
 *	The ensemble is updated and marked for recompilation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetEnsembleUnknownHandler(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj *unknownList)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;
    Tcl_Obj *oldList;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"command is not an ensemble", -1));
	Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	return TCL_ERROR;
    }
    if (unknownList != NULL) {
	int length;

	if (TclListObjLength(interp, unknownList, &length) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (length < 1) {
	    unknownList = NULL;
	}
    }

    ensemblePtr = cmdPtr->objClientData;
    oldList = ensemblePtr->unknownHandler;
    ensemblePtr->unknownHandler = unknownList;
    if (unknownList != NULL) {
	Tcl_IncrRefCount(unknownList);
    }
    if (oldList != NULL) {
	TclDecrRefCount(oldList);
    }

    /*
     * Trigger an eventual recomputation of the ensemble command set. Note
     * that this is slightly tricky, as it means that we are not actually
     * counting the number of namespace export actions, but it is the simplest
     * way to go!
     */

    ensemblePtr->nsPtr->exportLookupEpoch++;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetEnsembleFlags --
 *
 *	Set the flags for a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an
 *	ensemble).
 *
 * Side effects:
 *	The ensemble is updated and marked for recompilation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetEnsembleFlags(
    Tcl_Interp *interp,
    Tcl_Command token,
    int flags)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;
    int wasCompiled;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"command is not an ensemble", -1));
	Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	return TCL_ERROR;
    }

    ensemblePtr = cmdPtr->objClientData;
    wasCompiled = ensemblePtr->flags & ENSEMBLE_COMPILE;

    /*
     * This API refuses to set the ENSEMBLE_DEAD flag...
     */

    ensemblePtr->flags &= ENSEMBLE_DEAD;
    ensemblePtr->flags |= flags & ~ENSEMBLE_DEAD;

    /*
     * Trigger an eventual recomputation of the ensemble command set. Note
     * that this is slightly tricky, as it means that we are not actually
     * counting the number of namespace export actions, but it is the simplest
     * way to go!
     */

    ensemblePtr->nsPtr->exportLookupEpoch++;

    /*
     * If the ENSEMBLE_COMPILE flag status was changed, install or remove the
     * compiler function and bump the interpreter's compilation epoch so that
     * bytecode gets regenerated.
     */

    if (flags & ENSEMBLE_COMPILE) {
	if (!wasCompiled) {
	    ((Command*) ensemblePtr->token)->compileProc = TclCompileEnsemble;
	    ((Interp *) interp)->compileEpoch++;
	}
    } else {
	if (wasCompiled) {
	    ((Command *) ensemblePtr->token)->compileProc = NULL;
	    ((Interp *) interp)->compileEpoch++;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEnsembleSubcommandList --
 *
 *	Get the list of subcommands associated with a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an
 *	ensemble). The list of subcommands is returned by updating the
 *	variable pointed to by the last parameter (NULL if this is to be
 *	derived from the mapping dictionary or the associated namespace's
 *	exported commands).
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetEnsembleSubcommandList(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj **subcmdListPtr)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "command is not an ensemble", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	}
	return TCL_ERROR;
    }

    ensemblePtr = cmdPtr->objClientData;
    *subcmdListPtr = ensemblePtr->subcmdList;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEnsembleParameterList --
 *
 *	Get the list of parameters associated with a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an
 *	ensemble). The list of parameters is returned by updating the
 *	variable pointed to by the last parameter (NULL if there are
 *	no parameters).
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetEnsembleParameterList(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj **paramListPtr)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "command is not an ensemble", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	}
	return TCL_ERROR;
    }

    ensemblePtr = cmdPtr->objClientData;
    *paramListPtr = ensemblePtr->parameterList;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEnsembleMappingDict --
 *
 *	Get the command mapping dictionary associated with a particular
 *	ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an
 *	ensemble). The mapping dict is returned by updating the variable
 *	pointed to by the last parameter (NULL if none is installed).
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetEnsembleMappingDict(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj **mapDictPtr)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "command is not an ensemble", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	}
	return TCL_ERROR;
    }

    ensemblePtr = cmdPtr->objClientData;
    *mapDictPtr = ensemblePtr->subcommandDict;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEnsembleUnknownHandler --
 *
 *	Get the unknown handler associated with a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an
 *	ensemble). The unknown handler is returned by updating the variable
 *	pointed to by the last parameter (NULL if no handler is installed).
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetEnsembleUnknownHandler(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Obj **unknownListPtr)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "command is not an ensemble", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	}
	return TCL_ERROR;
    }

    ensemblePtr = cmdPtr->objClientData;
    *unknownListPtr = ensemblePtr->unknownHandler;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEnsembleFlags --
 *
 *	Get the flags for a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an
 *	ensemble). The flags are returned by updating the variable pointed to
 *	by the last parameter.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetEnsembleFlags(
    Tcl_Interp *interp,
    Tcl_Command token,
    int *flagsPtr)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "command is not an ensemble", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	}
	return TCL_ERROR;
    }

    ensemblePtr = cmdPtr->objClientData;
    *flagsPtr = ensemblePtr->flags;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetEnsembleNamespace --
 *
 *	Get the namespace associated with a particular ensemble.
 *
 * Results:
 *	Tcl result code (error if command token does not indicate an
 *	ensemble). Namespace is returned by updating the variable pointed to
 *	by the last parameter.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetEnsembleNamespace(
    Tcl_Interp *interp,
    Tcl_Command token,
    Tcl_Namespace **namespacePtrPtr)
{
    Command *cmdPtr = (Command *) token;
    EnsembleConfig *ensemblePtr;

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "command is not an ensemble", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "NOT_ENSEMBLE", NULL);
	}
	return TCL_ERROR;
    }

    ensemblePtr = cmdPtr->objClientData;
    *namespacePtrPtr = (Tcl_Namespace *) ensemblePtr->nsPtr;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FindEnsemble --
 *
 *	Given a command name, get the ensemble token for it, allowing for
 *	[namespace import]s. [Bug 1017022]
 *
 * Results:
 *	The token for the ensemble command with the given name, or NULL if the
 *	command either does not exist or is not an ensemble (when an error
 *	message will be written into the interp if thats non-NULL).
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
Tcl_FindEnsemble(
    Tcl_Interp *interp,		/* Where to do the lookup, and where to write
				 * the errors if TCL_LEAVE_ERR_MSG is set in
				 * the flags. */
    Tcl_Obj *cmdNameObj,	/* Name of command to look up. */
    int flags)			/* Either 0 or TCL_LEAVE_ERR_MSG; other flags
				 * are probably not useful. */
{
    Command *cmdPtr;

    cmdPtr = (Command *)
	    Tcl_FindCommand(interp, TclGetString(cmdNameObj), NULL, flags);
    if (cmdPtr == NULL) {
	return NULL;
    }

    if (cmdPtr->objProc != NsEnsembleImplementationCmd) {
	/*
	 * Reuse existing infrastructure for following import link chains
	 * rather than duplicating it.
	 */

	cmdPtr = (Command *) TclGetOriginalCommand((Tcl_Command) cmdPtr);

	if (cmdPtr == NULL || cmdPtr->objProc != NsEnsembleImplementationCmd){
	    if (flags & TCL_LEAVE_ERR_MSG) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"\"%s\" is not an ensemble command",
			TclGetString(cmdNameObj)));
		Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ENSEMBLE",
			TclGetString(cmdNameObj), NULL);
	    }
	    return NULL;
	}
    }

    return (Tcl_Command) cmdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IsEnsemble --
 *
 *	Simple test for ensemble-hood that takes into account imported
 *	ensemble commands as well.
 *
 * Results:
 *	Boolean value
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
Tcl_IsEnsemble(
    Tcl_Command token)
{
    Command *cmdPtr = (Command *) token;

    if (cmdPtr->objProc == NsEnsembleImplementationCmd) {
	return 1;
    }
    cmdPtr = (Command *) TclGetOriginalCommand((Tcl_Command) cmdPtr);
    if (cmdPtr == NULL || cmdPtr->objProc != NsEnsembleImplementationCmd) {
	return 0;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMakeEnsemble --
 *
 *	Create an ensemble from a table of implementation commands. The
 *	ensemble will be subject to (limited) compilation if any of the
 *	implementation commands are compilable.
 *
 *	The 'name' parameter may be a single command name or a list if
 *	creating an ensemble subcommand (see the binary implementation).
 *
 *	Currently, the TCL_ENSEMBLE_PREFIX ensemble flag is only used on
 *	top-level ensemble commands.
 *
 * Results:
 *	Handle for the new ensemble, or NULL on failure.
 *
 * Side effects:
 *	May advance the bytecode compilation epoch.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
TclMakeEnsemble(
    Tcl_Interp *interp,
    const char *name,		 /* The ensemble name (as explained above) */
    const EnsembleImplMap map[]) /* The subcommands to create */
{
    Tcl_Command ensemble;
    Tcl_Namespace *ns;
    Tcl_DString buf, hiddenBuf;
    const char **nameParts = NULL;
    const char *cmdName = NULL;
    int i, nameCount = 0, ensembleFlags = 0, hiddenLen;

    /*
     * Construct the path for the ensemble namespace and create it.
     */

    Tcl_DStringInit(&buf);
    Tcl_DStringInit(&hiddenBuf);
    TclDStringAppendLiteral(&hiddenBuf, "tcl:");
    Tcl_DStringAppend(&hiddenBuf, name, -1);
    TclDStringAppendLiteral(&hiddenBuf, ":");
    hiddenLen = Tcl_DStringLength(&hiddenBuf);
    if (name[0] == ':' && name[1] == ':') {
	/*
	 * An absolute name, so use it directly.
	 */

	cmdName = name;
	Tcl_DStringAppend(&buf, name, -1);
	ensembleFlags = TCL_ENSEMBLE_PREFIX;
    } else {
	/*
	 * Not an absolute name, so do munging of it. Note that this treats a
	 * multi-word list differently to a single word.
	 */

	TclDStringAppendLiteral(&buf, "::tcl");

	if (Tcl_SplitList(NULL, name, &nameCount, &nameParts) != TCL_OK) {
	    Tcl_Panic("invalid ensemble name '%s'", name);
	}

	for (i = 0; i < nameCount; ++i) {
	    TclDStringAppendLiteral(&buf, "::");
	    Tcl_DStringAppend(&buf, nameParts[i], -1);
	}
    }

    ns = Tcl_FindNamespace(interp, Tcl_DStringValue(&buf), NULL,
	    TCL_CREATE_NS_IF_UNKNOWN);
    if (!ns) {
	Tcl_Panic("unable to find or create %s namespace!",
		Tcl_DStringValue(&buf));
    }

    /*
     * Create the named ensemble in the correct namespace
     */

    if (cmdName == NULL) {
	if (nameCount == 1) {
	    ensembleFlags = TCL_ENSEMBLE_PREFIX;
	    cmdName = Tcl_DStringValue(&buf) + 5;
	} else {
	    ns = ns->parentPtr;
	    cmdName = nameParts[nameCount - 1];
	}
    }

    /*
     * Switch on compilation always for core ensembles now that we can do
     * nice bytecode things with them.  Do it now.  Waiting until later will
     * just cause pointless epoch bumps.
     */

    ensembleFlags |= ENSEMBLE_COMPILE;
    ensemble = Tcl_CreateEnsemble(interp, cmdName, ns, ensembleFlags);

    /*
     * Create the ensemble mapping dictionary and the ensemble command procs.
     */

    if (ensemble != NULL) {
	Tcl_Obj *mapDict, *fromObj, *toObj;
	Command *cmdPtr;

	TclDStringAppendLiteral(&buf, "::");
	TclNewObj(mapDict);
	for (i=0 ; map[i].name != NULL ; i++) {
	    fromObj = Tcl_NewStringObj(map[i].name, -1);
	    TclNewStringObj(toObj, Tcl_DStringValue(&buf),
		    Tcl_DStringLength(&buf));
	    Tcl_AppendToObj(toObj, map[i].name, -1);
	    Tcl_DictObjPut(NULL, mapDict, fromObj, toObj);

	    if (map[i].proc || map[i].nreProc) {
		/*
		 * If the command is unsafe, hide it when we're in a safe
		 * interpreter. The code to do this is really hokey! It also
		 * doesn't work properly yet; this function is always
		 * currently called before the safe-interp flag is set so the
		 * Tcl_IsSafe check fails.
		 */

		if (map[i].unsafe && Tcl_IsSafe(interp)) {
		    cmdPtr = (Command *)
			    Tcl_NRCreateCommand(interp, "___tmp", map[i].proc,
			    map[i].nreProc, map[i].clientData, NULL);
		    Tcl_DStringSetLength(&hiddenBuf, hiddenLen);
		    if (Tcl_HideCommand(interp, "___tmp",
			    Tcl_DStringAppend(&hiddenBuf, map[i].name, -1))) {
			Tcl_Panic("%s", Tcl_GetString(Tcl_GetObjResult(interp)));
		    }
		} else {
		    /*
		     * Not hidden, so just create it. Yay!
		     */

		    cmdPtr = (Command *)
			    Tcl_NRCreateCommand(interp, TclGetString(toObj),
			    map[i].proc, map[i].nreProc, map[i].clientData,
			    NULL);
		}
		cmdPtr->compileProc = map[i].compileProc;
	    }
	}
	Tcl_SetEnsembleMappingDict(interp, ensemble, mapDict);
    }

    Tcl_DStringFree(&buf);
    Tcl_DStringFree(&hiddenBuf);
    if (nameParts != NULL) {
	ckfree((char *) nameParts);
    }
    return ensemble;
}

/*
 *----------------------------------------------------------------------
 *
 * NsEnsembleImplementationCmd --
 *
 *	Implements an ensemble of commands (being those exported by a
 *	namespace other than the global namespace) as a command with the same
 *	(short) name as the namespace in the parent namespace.
 *
 * Results:
 *	A standard Tcl result code. Will be TCL_ERROR if the command is not an
 *	unambiguous prefix of any command exported by the ensemble's
 *	namespace.
 *
 * Side effects:
 *	Depends on the command within the namespace that gets executed. If the
 *	ensemble itself returns TCL_ERROR, a descriptive error message will be
 *	placed in the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

static int
NsEnsembleImplementationCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    return Tcl_NRCallObjProc(interp, NsEnsembleImplementationCmdNR,
	    clientData, objc, objv);
}

static int
NsEnsembleImplementationCmdNR(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    EnsembleConfig *ensemblePtr = clientData;
				/* The ensemble itself. */
    Tcl_Obj *prefixObj;		/* An object containing the prefix words of
				 * the command that implements the
				 * subcommand. */
    Tcl_HashEntry *hPtr;	/* Used for efficient lookup of fully
				 * specified but not yet cached command
				 * names. */
    int reparseCount = 0;	/* Number of reparses. */
    Tcl_Obj *errorObj;		/* Used for building error messages. */
    Tcl_Obj *subObj;
    int subIdx;

    /*
     * Must recheck objc, since numParameters might have changed. Cf. test
     * namespace-53.9.
     */

  restartEnsembleParse:
    subIdx = 1 + ensemblePtr->numParameters;
    if (objc < subIdx + 1) {
	/*
	 * We don't have a subcommand argument. Make error message.
	 */

	Tcl_DString buf;	/* Message being built */

	Tcl_DStringInit(&buf);
	if (ensemblePtr->parameterList) {
	    Tcl_DStringAppend(&buf,
		    TclGetString(ensemblePtr->parameterList), -1);
	    TclDStringAppendLiteral(&buf, " ");
	}
	TclDStringAppendLiteral(&buf, "subcommand ?arg ...?");
	Tcl_WrongNumArgs(interp, 1, objv, Tcl_DStringValue(&buf));
	Tcl_DStringFree(&buf);

	return TCL_ERROR;
    }

    if (ensemblePtr->nsPtr->flags & NS_DYING) {
	/*
	 * Don't know how we got here, but make things give up quickly.
	 */

	if (!Tcl_InterpDeleted(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "ensemble activated for deleted namespace", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "DEAD", NULL);
	}
	return TCL_ERROR;
    }

    /*
     * Determine if the table of subcommands is right. If so, we can just look
     * up in there and go straight to dispatch.
     */

    subObj = objv[subIdx];

    if (ensemblePtr->epoch == ensemblePtr->nsPtr->exportLookupEpoch) {
	/*
	 * Table of subcommands is still valid; therefore there might be a
	 * valid cache of discovered information which we can reuse. Do the
	 * check here, and if we're still valid, we can jump straight to the
	 * part where we do the invocation of the subcommand.
	 */

	if (subObj->typePtr==&ensembleCmdType){
	    EnsembleCmdRep *ensembleCmd = subObj->internalRep.twoPtrValue.ptr1;

	    if (ensembleCmd->epoch == ensemblePtr->epoch &&
		    ensembleCmd->token == (Command *)ensemblePtr->token) {
		prefixObj = Tcl_GetHashValue(ensembleCmd->hPtr);
		Tcl_IncrRefCount(prefixObj);
		if (ensembleCmd->fix) {
		    TclSpellFix(interp, objv, objc, subIdx, subObj, ensembleCmd->fix);
		}
		goto runResultingSubcommand;
	    }
	}
    } else {
	BuildEnsembleConfig(ensemblePtr);
	ensemblePtr->epoch = ensemblePtr->nsPtr->exportLookupEpoch;
    }

    /*
     * Look in the hashtable for the subcommand name; this is the fastest way
     * of all if there is no cache in operation.
     */

    hPtr = Tcl_FindHashEntry(&ensemblePtr->subcommandTable,
	    TclGetString(subObj));
    if (hPtr != NULL) {

	/*
	 * Cache for later in the subcommand object.
	 */

	MakeCachedEnsembleCommand(subObj, ensemblePtr, hPtr, NULL);
    } else if (!(ensemblePtr->flags & TCL_ENSEMBLE_PREFIX)) {
	/*
	 * Could not map, no prefixing, go to unknown/error handling.
	 */

	goto unknownOrAmbiguousSubcommand;
    } else {
	/*
	 * If we've not already confirmed the command with the hash as part of
	 * building our export table, we need to scan the sorted array for
	 * matches.
	 */

	const char *subcmdName; /* Name of the subcommand, or unique prefix of
				 * it (will be an error for a non-unique
				 * prefix). */
	char *fullName = NULL;	/* Full name of the subcommand. */
	int stringLength, i;
	int tableLength = ensemblePtr->subcommandTable.numEntries;
	Tcl_Obj *fix;

	subcmdName = Tcl_GetStringFromObj(subObj, &stringLength);
	for (i=0 ; i<tableLength ; i++) {
	    register int cmp = strncmp(subcmdName,
		    ensemblePtr->subcommandArrayPtr[i],
		    (unsigned) stringLength);

	    if (cmp == 0) {
		if (fullName != NULL) {
		    /*
		     * Since there's never the exact-match case to worry about
		     * (hash search filters this), getting here indicates that
		     * our subcommand is an ambiguous prefix of (at least) two
		     * exported subcommands, which is an error case.
		     */

		    goto unknownOrAmbiguousSubcommand;
		}
		fullName = ensemblePtr->subcommandArrayPtr[i];
	    } else if (cmp < 0) {
		/*
		 * Because we are searching a sorted table, we can now stop
		 * searching because we have gone past anything that could
		 * possibly match.
		 */

		break;
	    }
	}
	if (fullName == NULL) {
	    /*
	     * The subcommand is not a prefix of anything, so bail out!
	     */

	    goto unknownOrAmbiguousSubcommand;
	}
	hPtr = Tcl_FindHashEntry(&ensemblePtr->subcommandTable, fullName);
	if (hPtr == NULL) {
	    Tcl_Panic("full name %s not found in supposedly synchronized hash",
		    fullName);
	}

	/*
	 * Record the spelling correction for usage message.
	 */

	fix = Tcl_NewStringObj(fullName, -1);

	/*
	 * Cache for later in the subcommand object.
	 */

	MakeCachedEnsembleCommand(subObj, ensemblePtr, hPtr, fix);
	TclSpellFix(interp, objv, objc, subIdx, subObj, fix);
    }

    prefixObj = Tcl_GetHashValue(hPtr);
    Tcl_IncrRefCount(prefixObj);
  runResultingSubcommand:

    /*
     * Do the real work of execution of the subcommand by building an array of
     * objects (note that this is potentially not the same length as the
     * number of arguments to this ensemble command), populating it and then
     * feeding it back through the main command-lookup engine. In theory, we
     * could look up the command in the namespace ourselves, as we already
     * have the namespace in which it is guaranteed to exist,
     *
     *   ((Q: That's not true if the -map option is used, is it?))
     *
     * but we don't do that (the cacheing of the command object used should
     * help with that.)
     */

    {
	Tcl_Obj *copyPtr;	/* The actual list of words to dispatch to.
				 * Will be freed by the dispatch engine. */
	Tcl_Obj **copyObjv;
	int copyObjc, prefixObjc;

	Tcl_ListObjLength(NULL, prefixObj, &prefixObjc);

	if (objc == 2) {
	    copyPtr = TclListObjCopy(NULL, prefixObj);
	} else {
	    copyPtr = Tcl_NewListObj(objc - 2 + prefixObjc, NULL);
	    Tcl_ListObjAppendList(NULL, copyPtr, prefixObj);
	    Tcl_ListObjReplace(NULL, copyPtr, LIST_MAX, 0,
		    ensemblePtr->numParameters, objv + 1);
	    Tcl_ListObjReplace(NULL, copyPtr, LIST_MAX, 0,
		    objc - 2 - ensemblePtr->numParameters,
		    objv + 2 + ensemblePtr->numParameters);
	}
	Tcl_IncrRefCount(copyPtr);
	TclNRAddCallback(interp, TclNRReleaseValues, copyPtr, NULL, NULL, NULL);
	TclDecrRefCount(prefixObj);

	/*
	 * Record what arguments the script sent in so that things like
	 * Tcl_WrongNumArgs can give the correct error message. Parameters
	 * count both as inserted and removed arguments.
	 */

	if (TclInitRewriteEnsemble(interp, 2 + ensemblePtr->numParameters,
		prefixObjc + ensemblePtr->numParameters, objv)) {
	    TclNRAddCallback(interp, TclClearRootEnsemble, NULL, NULL, NULL,
		    NULL);
	}

	/*
	 * Hand off to the target command.
	 */

	TclSkipTailcall(interp);
	Tcl_ListObjGetElements(NULL, copyPtr, &copyObjc, &copyObjv);
	((Interp *)interp)->lookupNsPtr = ensemblePtr->nsPtr;
	return TclNREvalObjv(interp, copyObjc, copyObjv, TCL_EVAL_INVOKE, NULL);
    }

  unknownOrAmbiguousSubcommand:
    /*
     * Have not been able to match the subcommand asked for with a real
     * subcommand that we export. See whether a handler has been registered
     * for dealing with this situation. Will only call (at most) once for any
     * particular ensemble invocation.
     */

    if (ensemblePtr->unknownHandler != NULL && reparseCount++ < 1) {
	switch (EnsembleUnknownCallback(interp, ensemblePtr, objc, objv,
		&prefixObj)) {
	case TCL_OK:
	    goto runResultingSubcommand;
	case TCL_ERROR:
	    return TCL_ERROR;
	case TCL_CONTINUE:
	    goto restartEnsembleParse;
	}
    }

    /*
     * We cannot determine what subcommand to hand off to, so generate a
     * (standard) failure message. Note the one odd case compared with
     * standard ensemble-like command, which is where a namespace has no
     * exported commands at all...
     */

    Tcl_ResetResult(interp);
    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "SUBCOMMAND",
	    TclGetString(subObj), NULL);
    if (ensemblePtr->subcommandTable.numEntries == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unknown subcommand \"%s\": namespace %s does not"
		" export any commands", TclGetString(subObj),
		ensemblePtr->nsPtr->fullName));
	return TCL_ERROR;
    }
    errorObj = Tcl_ObjPrintf("unknown%s subcommand \"%s\": must be ",
	    (ensemblePtr->flags & TCL_ENSEMBLE_PREFIX ? " or ambiguous" : ""),
	    TclGetString(subObj));
    if (ensemblePtr->subcommandTable.numEntries == 1) {
	Tcl_AppendToObj(errorObj, ensemblePtr->subcommandArrayPtr[0], -1);
    } else {
	int i;

	for (i=0 ; i<ensemblePtr->subcommandTable.numEntries-1 ; i++) {
	    Tcl_AppendToObj(errorObj, ensemblePtr->subcommandArrayPtr[i], -1);
	    Tcl_AppendToObj(errorObj, ", ", 2);
	}
	Tcl_AppendPrintfToObj(errorObj, "or %s",
		ensemblePtr->subcommandArrayPtr[i]);
    }
    Tcl_SetObjResult(interp, errorObj);
    return TCL_ERROR;
}

int
TclClearRootEnsemble(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    TclResetRewriteEnsemble(interp, 1);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitRewriteEnsemble --
 *
 *	Applies a rewrite of arguments so that an ensemble subcommand will
 *	report error messages correctly for the overall command.
 *
 * Results:
 *	Whether this is the first rewrite applied, a value which must be
 *	passed to TclResetRewriteEnsemble when undoing this command's
 *	behaviour.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclInitRewriteEnsemble(
    Tcl_Interp *interp,
    int numRemoved,
    int numInserted,
    Tcl_Obj *const *objv)
{
    Interp *iPtr = (Interp *) interp;

    int isRootEnsemble = (iPtr->ensembleRewrite.sourceObjs == NULL);

    if (isRootEnsemble) {
	iPtr->ensembleRewrite.sourceObjs = objv;
	iPtr->ensembleRewrite.numRemovedObjs = numRemoved;
	iPtr->ensembleRewrite.numInsertedObjs = numInserted;
    } else {
	int numIns = iPtr->ensembleRewrite.numInsertedObjs;

	if (numIns < numRemoved) {
	    iPtr->ensembleRewrite.numRemovedObjs += numRemoved - numIns;
	    iPtr->ensembleRewrite.numInsertedObjs = numInserted;
	} else {
	    iPtr->ensembleRewrite.numInsertedObjs += numInserted - numRemoved;
	}
    }
    return isRootEnsemble;
}

/*
 *----------------------------------------------------------------------
 *
 * TclResetRewriteEnsemble --
 *
 *	Removes any rewrites applied to support proper reporting of error
 *	messages used in ensembles. Should be paired with
 *	TclInitRewriteEnsemble.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclResetRewriteEnsemble(
    Tcl_Interp *interp,
    int isRootEnsemble)
{
    Interp *iPtr = (Interp *) interp;

    if (isRootEnsemble) {
	iPtr->ensembleRewrite.sourceObjs = NULL;
	iPtr->ensembleRewrite.numRemovedObjs = 0;
	iPtr->ensembleRewrite.numInsertedObjs = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclSpellFix --
 *
 *	Record a spelling correction that needs making in the generation of
 *	the WrongNumArgs usage message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Can create an alternative ensemble rewrite structure.
 *
 *----------------------------------------------------------------------
 */

static int
FreeER(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Obj **tmp = (Tcl_Obj **) data[0];
    Tcl_Obj **store = (Tcl_Obj **) data[1];

    ckfree(store);
    ckfree(tmp);
    return result;
}

void
TclSpellFix(
    Tcl_Interp *interp,
    Tcl_Obj *const *objv,
    int objc,
    int badIdx,
    Tcl_Obj *bad,
    Tcl_Obj *fix)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *const *search;
    Tcl_Obj **store;
    int idx;
    int size;

    if (iPtr->ensembleRewrite.sourceObjs == NULL) {
	iPtr->ensembleRewrite.sourceObjs = objv;
	iPtr->ensembleRewrite.numRemovedObjs = 0;
	iPtr->ensembleRewrite.numInsertedObjs = 0;
    }

    /*
     * Compute the valid length of the ensemble root.
     */

    size = iPtr->ensembleRewrite.numRemovedObjs + objc
		- iPtr->ensembleRewrite.numInsertedObjs;

    search = iPtr->ensembleRewrite.sourceObjs;
    if (search[0] == NULL) {
	/*
	 * Awful casting abuse here!
	 */

	search = (Tcl_Obj *const *) search[1];
    }

    if (badIdx < iPtr->ensembleRewrite.numInsertedObjs) {
	/*
	 * Misspelled value was inserted. We cannot directly jump to the bad
	 * value, but have to search.
	 */

	idx = 1;
	while (idx < size) {
	    if (search[idx] == bad) {
		break;
	    }
	    idx++;
	}
	if (idx == size) {
	    return;
	}
    } else {
	/*
	 * Jump to the misspelled value.
	 */

	idx = iPtr->ensembleRewrite.numRemovedObjs + badIdx
		- iPtr->ensembleRewrite.numInsertedObjs;

	/* Verify */
	if (search[idx] != bad) {
	    Tcl_Panic("SpellFix: programming error");
	}
    }

    search = iPtr->ensembleRewrite.sourceObjs;
    if (search[0] == NULL) {
	store = (Tcl_Obj **) search[2];
    }  else {
	Tcl_Obj **tmp = ckalloc(3 * sizeof(Tcl_Obj *));

	store = ckalloc(size * sizeof(Tcl_Obj *));
	memcpy(store, iPtr->ensembleRewrite.sourceObjs,
		size * sizeof(Tcl_Obj *));

	/*
	 * Awful casting abuse here! Note that the NULL in the first element
	 * indicates that the initial objects are a raw array in the second
	 * element and the rewritten ones are a raw array in the third.
	 */

	tmp[0] = NULL;
	tmp[1] = (Tcl_Obj *) iPtr->ensembleRewrite.sourceObjs;
	tmp[2] = (Tcl_Obj *) store;
	iPtr->ensembleRewrite.sourceObjs = (Tcl_Obj *const *) tmp;

	TclNRAddCallback(interp, FreeER, tmp, store, NULL, NULL);
    }

    store[idx] = fix;
    Tcl_IncrRefCount(fix);
    TclNRAddCallback(interp, TclNRReleaseValues, fix, NULL, NULL, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * TclFetchEnsembleRoot --
 *
 *	Returns the root of ensemble rewriting, if any.
 *	If no root exists, returns objv instead.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *const *
TclFetchEnsembleRoot(
    Tcl_Interp *interp,
    Tcl_Obj *const *objv,
    int objc,
    int *objcPtr)
{
    Interp *iPtr = (Interp *) interp;

    if (iPtr->ensembleRewrite.sourceObjs) {
	*objcPtr = objc + iPtr->ensembleRewrite.numRemovedObjs
		- iPtr->ensembleRewrite.numInsertedObjs;
	return iPtr->ensembleRewrite.sourceObjs;
    }
    *objcPtr = objc;
    return objv;
}

/*
 * ----------------------------------------------------------------------
 *
 * EnsmebleUnknownCallback --
 *
 *	Helper for the ensemble engine that handles the procesing of unknown
 *	callbacks. See the user documentation of the ensemble unknown handler
 *	for details; this function is only ever called when such a function is
 *	defined, and is only ever called once per ensemble dispatch (i.e. if a
 *	reparse still fails, this isn't called again).
 *
 * Results:
 *	TCL_OK -	*prefixObjPtr contains the command words to dispatch
 *			to.
 *	TCL_CONTINUE -	Need to reparse (*prefixObjPtr is invalid).
 *	TCL_ERROR -	Something went wrong! Error message in interpreter.
 *
 * Side effects:
 *	Calls the Tcl interpreter, so arbitrary.
 *
 * ----------------------------------------------------------------------
 */

static inline int
EnsembleUnknownCallback(
    Tcl_Interp *interp,
    EnsembleConfig *ensemblePtr,
    int objc,
    Tcl_Obj *const objv[],
    Tcl_Obj **prefixObjPtr)
{
    int paramc, i, result, prefixObjc;
    Tcl_Obj **paramv, *unknownCmd, *ensObj;

    /*
     * Create the unknown command callback to determine what to do.
     */

    unknownCmd = Tcl_DuplicateObj(ensemblePtr->unknownHandler);
    TclNewObj(ensObj);
    Tcl_GetCommandFullName(interp, ensemblePtr->token, ensObj);
    Tcl_ListObjAppendElement(NULL, unknownCmd, ensObj);
    for (i=1 ; i<objc ; i++) {
	Tcl_ListObjAppendElement(NULL, unknownCmd, objv[i]);
    }
    TclListObjGetElements(NULL, unknownCmd, &paramc, &paramv);
    Tcl_IncrRefCount(unknownCmd);

    /*
     * Now call the unknown handler. (We don't bother NRE-enabling this; deep
     * recursing through unknown handlers is horribly perverse.) Note that it
     * is always an error for an unknown handler to delete its ensemble; don't
     * do that!
     */

    Tcl_Preserve(ensemblePtr);
    TclSkipTailcall(interp);
    result = Tcl_EvalObjv(interp, paramc, paramv, 0);
    if ((result == TCL_OK) && (ensemblePtr->flags & ENSEMBLE_DEAD)) {
	if (!Tcl_InterpDeleted(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "unknown subcommand handler deleted its ensemble", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "UNKNOWN_DELETED",
		    NULL);
	}
	result = TCL_ERROR;
    }
    Tcl_Release(ensemblePtr);

    /*
     * If we succeeded, we should either have a list of words that form the
     * command to be executed, or an empty list. In the empty-list case, the
     * ensemble is believed to be updated so we should ask the ensemble engine
     * to reparse the original command.
     */

    if (result == TCL_OK) {
	*prefixObjPtr = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(*prefixObjPtr);
	TclDecrRefCount(unknownCmd);
	Tcl_ResetResult(interp);

	/*
	 * Namespace is still there. Check if the result is a valid list. If
	 * it is, and it is non-empty, that list is what we are using as our
	 * replacement.
	 */

	if (TclListObjLength(interp, *prefixObjPtr, &prefixObjc) != TCL_OK) {
	    TclDecrRefCount(*prefixObjPtr);
	    Tcl_AddErrorInfo(interp, "\n    while parsing result of "
		    "ensemble unknown subcommand handler");
	    return TCL_ERROR;
	}
	if (prefixObjc > 0) {
	    return TCL_OK;
	}

	/*
	 * Namespace alive & empty result => reparse.
	 */

	TclDecrRefCount(*prefixObjPtr);
	return TCL_CONTINUE;
    }

    /*
     * Oh no! An exceptional result. Convert to an error.
     */

    if (!Tcl_InterpDeleted(interp)) {
	if (result != TCL_ERROR) {
	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "unknown subcommand handler returned bad code: ", -1));
	    switch (result) {
	    case TCL_RETURN:
		Tcl_AppendToObj(Tcl_GetObjResult(interp), "return", -1);
		break;
	    case TCL_BREAK:
		Tcl_AppendToObj(Tcl_GetObjResult(interp), "break", -1);
		break;
	    case TCL_CONTINUE:
		Tcl_AppendToObj(Tcl_GetObjResult(interp), "continue", -1);
		break;
	    default:
		Tcl_AppendPrintfToObj(Tcl_GetObjResult(interp), "%d", result);
	    }
	    Tcl_AddErrorInfo(interp, "\n    result of "
		    "ensemble unknown subcommand handler: ");
	    Tcl_AppendObjToErrorInfo(interp, unknownCmd);
	    Tcl_SetErrorCode(interp, "TCL", "ENSEMBLE", "UNKNOWN_RESULT",
		    NULL);
	} else {
	    Tcl_AddErrorInfo(interp,
		    "\n    (ensemble unknown subcommand handler)");
	}
    }
    TclDecrRefCount(unknownCmd);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeCachedEnsembleCommand --
 *
 *	Cache what we've computed so far; it's not nice to repeatedly copy
 *	strings about. Note that to do this, we start by deleting any old
 *	representation that there was (though if it was an out of date
 *	ensemble rep, we can skip some of the deallocation process.)
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Alters the internal representation of the first object parameter.
 *
 *----------------------------------------------------------------------
 */

static void
MakeCachedEnsembleCommand(
    Tcl_Obj *objPtr,
    EnsembleConfig *ensemblePtr,
    Tcl_HashEntry *hPtr,
    Tcl_Obj *fix)
{
    register EnsembleCmdRep *ensembleCmd;

    if (objPtr->typePtr == &ensembleCmdType) {
	ensembleCmd = objPtr->internalRep.twoPtrValue.ptr1;
	TclCleanupCommandMacro(ensembleCmd->token);
	if (ensembleCmd->fix) {
	    Tcl_DecrRefCount(ensembleCmd->fix);
	}
    } else {
	/*
	 * Kill the old internal rep, and replace it with a brand new one of
	 * our own.
	 */

	TclFreeIntRep(objPtr);
	ensembleCmd = ckalloc(sizeof(EnsembleCmdRep));
	objPtr->internalRep.twoPtrValue.ptr1 = ensembleCmd;
	objPtr->typePtr = &ensembleCmdType;
    }

    /*
     * Populate the internal rep.
     */

    ensembleCmd->epoch = ensemblePtr->epoch;
    ensembleCmd->token = (Command *) ensemblePtr->token;
    ensembleCmd->token->refCount++;
    if (fix) {
	Tcl_IncrRefCount(fix);
    }
    ensembleCmd->fix = fix;
    ensembleCmd->hPtr = hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteEnsembleConfig --
 *
 *	Destroys the data structure used to represent an ensemble. This is
 *	called when the ensemble's command is deleted (which happens
 *	automatically if the ensemble's namespace is deleted.) Maintainers
 *	should note that ensembles should be deleted by deleting their
 *	commands.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is (eventually) deallocated.
 *
 *----------------------------------------------------------------------
 */

static void
ClearTable(
    EnsembleConfig *ensemblePtr)
{
    Tcl_HashTable *hash = &ensemblePtr->subcommandTable;

    if (hash->numEntries != 0) {
        Tcl_HashSearch search;
        Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(hash, &search);

        while (hPtr != NULL) {
            Tcl_Obj *prefixObj = Tcl_GetHashValue(hPtr);
            Tcl_DecrRefCount(prefixObj);
            hPtr = Tcl_NextHashEntry(&search);
        }
        ckfree((char *) ensemblePtr->subcommandArrayPtr);
    }
    Tcl_DeleteHashTable(hash);
}

static void
DeleteEnsembleConfig(
    ClientData clientData)
{
    EnsembleConfig *ensemblePtr = clientData;
    Namespace *nsPtr = ensemblePtr->nsPtr;

    /*
     * Unlink from the ensemble chain if it has not been marked as having been
     * done already.
     */

    if (ensemblePtr->next != ensemblePtr) {
	EnsembleConfig *ensPtr = (EnsembleConfig *) nsPtr->ensembles;

	if (ensPtr == ensemblePtr) {
	    nsPtr->ensembles = (Tcl_Ensemble *) ensemblePtr->next;
	} else {
	    while (ensPtr != NULL) {
		if (ensPtr->next == ensemblePtr) {
		    ensPtr->next = ensemblePtr->next;
		    break;
		}
		ensPtr = ensPtr->next;
	    }
	}
    }

    /*
     * Mark the namespace as dead so code that uses Tcl_Preserve() can tell
     * whether disaster happened anyway.
     */

    ensemblePtr->flags |= ENSEMBLE_DEAD;

    /*
     * Kill the pointer-containing fields.
     */

    ClearTable(ensemblePtr);
    if (ensemblePtr->subcmdList != NULL) {
	Tcl_DecrRefCount(ensemblePtr->subcmdList);
    }
    if (ensemblePtr->parameterList != NULL) {
	Tcl_DecrRefCount(ensemblePtr->parameterList);
    }
    if (ensemblePtr->subcommandDict != NULL) {
	Tcl_DecrRefCount(ensemblePtr->subcommandDict);
    }
    if (ensemblePtr->unknownHandler != NULL) {
	Tcl_DecrRefCount(ensemblePtr->unknownHandler);
    }

    /*
     * Arrange for the structure to be reclaimed. Note that this is complex
     * because we have to make sure that we can react sensibly when an
     * ensemble is deleted during the process of initialising the ensemble
     * (especially the unknown callback.)
     */

    Tcl_EventuallyFree(ensemblePtr, TCL_DYNAMIC);
}

/*
 *----------------------------------------------------------------------
 *
 * BuildEnsembleConfig --
 *
 *	Create the internal data structures that describe how an ensemble
 *	looks, being a hash mapping from the simple command name to the Tcl list
 *	that describes the implementation prefix words, and a sorted array of
 *	the names to allow for reasonably efficient unambiguous prefix handling.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reallocates and rebuilds the hash table and array stored at the
 *	ensemblePtr argument. For large ensembles or large namespaces, this is
 *	a potentially expensive operation.
 *
 *----------------------------------------------------------------------
 */

static void
BuildEnsembleConfig(
    EnsembleConfig *ensemblePtr)
{
    Tcl_HashSearch search;	/* Used for scanning the set of commands in
				 * the namespace that backs up this
				 * ensemble. */
    int i, j, isNew;
    Tcl_HashTable *hash = &ensemblePtr->subcommandTable;
    Tcl_HashEntry *hPtr;
    Tcl_Obj *mapDict = ensemblePtr->subcommandDict;
    Tcl_Obj *subList = ensemblePtr->subcmdList;

    ClearTable(ensemblePtr);
    Tcl_InitHashTable(hash, TCL_STRING_KEYS);

    if (subList) {
        int subc;
        Tcl_Obj **subv, *target, *cmdObj, *cmdPrefixObj;
        char *name;

        /*
         * There is a list of exactly what subcommands go in the table.
         * Must determine the target for each.
         */

        Tcl_ListObjGetElements(NULL, subList, &subc, &subv);
        if (subList == mapDict) {
            /*
             * Strange case where explicit list of subcommands is same value
             * as the dict mapping to targets.
             */

            for (i = 0; i < subc; i += 2) {
                name = TclGetString(subv[i]);
                hPtr = Tcl_CreateHashEntry(hash, name, &isNew);
                if (!isNew) {
                    cmdObj = (Tcl_Obj *)Tcl_GetHashValue(hPtr);
                    Tcl_DecrRefCount(cmdObj);
                }
                Tcl_SetHashValue(hPtr, subv[i+1]);
                Tcl_IncrRefCount(subv[i+1]);

                name = TclGetString(subv[i+1]);
                hPtr = Tcl_CreateHashEntry(hash, name, &isNew);
                if (isNew) {
                    cmdObj = Tcl_NewStringObj(name, -1);
                    cmdPrefixObj = Tcl_NewListObj(1, &cmdObj);
                    Tcl_SetHashValue(hPtr, cmdPrefixObj);
                    Tcl_IncrRefCount(cmdPrefixObj);
                }
            }
        } else {
            /* Usual case where we can freely act on the list and dict. */

            for (i = 0; i < subc; i++) {
                name = TclGetString(subv[i]);
                hPtr = Tcl_CreateHashEntry(hash, name, &isNew);
                if (!isNew) {
                    continue;
                }

                /* Lookup target in the dictionary */
                if (mapDict) {
                    Tcl_DictObjGet(NULL, mapDict, subv[i], &target);
                    if (target) {
                        Tcl_SetHashValue(hPtr, target);
                        Tcl_IncrRefCount(target);
                        continue;
                    }
                }

                /*
                 * target was not in the dictionary so map onto the namespace.
                 * Note in this case that we do not guarantee that the
                 * command is actually there; that is the programmer's
                 * responsibility (or [::unknown] of course).
                 */
                cmdObj = Tcl_NewStringObj(name, -1);
                cmdPrefixObj = Tcl_NewListObj(1, &cmdObj);
                Tcl_SetHashValue(hPtr, cmdPrefixObj);
                Tcl_IncrRefCount(cmdPrefixObj);
            }
        }
    } else if (mapDict) {
        /*
         * No subcmd list, but we do have a mapping dictionary so we should
         * use the keys of that. Convert the dictionary's contents into the
         * form required for the ensemble's internal hashtable.
         */

        Tcl_DictSearch dictSearch;
        Tcl_Obj *keyObj, *valueObj;
        int done;

        Tcl_DictObjFirst(NULL, ensemblePtr->subcommandDict, &dictSearch,
                &keyObj, &valueObj, &done);
        while (!done) {
            char *name = TclGetString(keyObj);

            hPtr = Tcl_CreateHashEntry(hash, name, &isNew);
            Tcl_SetHashValue(hPtr, valueObj);
            Tcl_IncrRefCount(valueObj);
            Tcl_DictObjNext(&dictSearch, &keyObj, &valueObj, &done);
        }
    } else {
	/*
	 * Discover what commands are actually exported by the namespace.
	 * What we have is an array of patterns and a hash table whose keys
	 * are the command names exported by the namespace (the contents do
	 * not matter here.) We must find out what commands are actually
	 * exported by filtering each command in the namespace against each of
	 * the patterns in the export list. Note that we use an intermediate
	 * hash table to make memory management easier, and because that makes
	 * exact matching far easier too.
	 *
	 * Suggestion for future enhancement: compute the unique prefixes and
	 * place them in the hash too, which should make for even faster
	 * matching.
	 */

	hPtr = Tcl_FirstHashEntry(&ensemblePtr->nsPtr->cmdTable, &search);
	for (; hPtr!= NULL ; hPtr=Tcl_NextHashEntry(&search)) {
	    char *nsCmdName =		/* Name of command in namespace. */
		    Tcl_GetHashKey(&ensemblePtr->nsPtr->cmdTable, hPtr);

	    for (i=0 ; i<ensemblePtr->nsPtr->numExportPatterns ; i++) {
		if (Tcl_StringMatch(nsCmdName,
			ensemblePtr->nsPtr->exportArrayPtr[i])) {
		    hPtr = Tcl_CreateHashEntry(hash, nsCmdName, &isNew);

		    /*
		     * Remember, hash entries have a full reference to the
		     * substituted part of the command (as a list) as their
		     * content!
		     */

		    if (isNew) {
			Tcl_Obj *cmdObj, *cmdPrefixObj;

			TclNewObj(cmdObj);
			Tcl_AppendStringsToObj(cmdObj,
				ensemblePtr->nsPtr->fullName,
				(ensemblePtr->nsPtr->parentPtr ? "::" : ""),
				nsCmdName, NULL);
			cmdPrefixObj = Tcl_NewListObj(1, &cmdObj);
			Tcl_SetHashValue(hPtr, cmdPrefixObj);
			Tcl_IncrRefCount(cmdPrefixObj);
		    }
		    break;
		}
	    }
	}
    }

    if (hash->numEntries == 0) {
	ensemblePtr->subcommandArrayPtr = NULL;
	return;
    }

    /*
     * Create a sorted array of all subcommands in the ensemble; hash tables
     * are all very well for a quick look for an exact match, but they can't
     * determine things like whether a string is a prefix of another (not
     * without lots of preparation anyway) and they're no good for when we're
     * generating the error message either.
     *
     * We do this by filling an array with the names (we use the hash keys
     * directly to save a copy, since any time we change the array we change
     * the hash too, and vice versa) and running quicksort over the array.
     */

    ensemblePtr->subcommandArrayPtr =
	    ckalloc(sizeof(char *) * hash->numEntries);

    /*
     * Fill array from both ends as this makes us less likely to end up with
     * performance problems in qsort(), which is good. Note that doing this
     * makes this code much more opaque, but the naive alternatve:
     *
     * for (hPtr=Tcl_FirstHashEntry(hash,&search),i=0 ;
     *	       hPtr!=NULL ; hPtr=Tcl_NextHashEntry(&search),i++) {
     *     ensemblePtr->subcommandArrayPtr[i] = Tcl_GetHashKey(hash, &hPtr);
     * }
     *
     * can produce long runs of precisely ordered table entries when the
     * commands in the namespace are declared in a sorted fashion (an ordering
     * some people like) and the hashing functions (or the command names
     * themselves) are fairly unfortunate. By filling from both ends, it
     * requires active malice (and probably a debugger) to get qsort() to have
     * awful runtime behaviour.
     */

    i = 0;
    j = hash->numEntries;
    hPtr = Tcl_FirstHashEntry(hash, &search);
    while (hPtr != NULL) {
	ensemblePtr->subcommandArrayPtr[i++] = Tcl_GetHashKey(hash, hPtr);
	hPtr = Tcl_NextHashEntry(&search);
	if (hPtr == NULL) {
	    break;
	}
	ensemblePtr->subcommandArrayPtr[--j] = Tcl_GetHashKey(hash, hPtr);
	hPtr = Tcl_NextHashEntry(&search);
    }
    if (hash->numEntries > 1) {
	qsort(ensemblePtr->subcommandArrayPtr, (unsigned) hash->numEntries,
		sizeof(char *), NsEnsembleStringOrder);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NsEnsembleStringOrder --
 *
 *	Helper function to compare two pointers to two strings for use with
 *	qsort().
 *
 * Results:
 *	-1 if the first string is smaller, 1 if the second string is smaller,
 *	and 0 if they are equal.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
NsEnsembleStringOrder(
    const void *strPtr1,
    const void *strPtr2)
{
    return strcmp(*(const char **)strPtr1, *(const char **)strPtr2);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeEnsembleCmdRep --
 *
 *	Destroys the internal representation of a Tcl_Obj that has been
 *	holding information about a command in an ensemble.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is deallocated. If this held the last reference to a
 *	namespace's main structure, that main structure will also be
 *	destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
FreeEnsembleCmdRep(
    Tcl_Obj *objPtr)
{
    EnsembleCmdRep *ensembleCmd = objPtr->internalRep.twoPtrValue.ptr1;

    TclCleanupCommandMacro(ensembleCmd->token);
    if (ensembleCmd->fix) {
	Tcl_DecrRefCount(ensembleCmd->fix);
    }
    ckfree(ensembleCmd);
    objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DupEnsembleCmdRep --
 *
 *	Makes one Tcl_Obj into a copy of another that is a subcommand of an
 *	ensemble.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated, and the namespace that the ensemble is built on
 *	top of gains another reference.
 *
 *----------------------------------------------------------------------
 */

static void
DupEnsembleCmdRep(
    Tcl_Obj *objPtr,
    Tcl_Obj *copyPtr)
{
    EnsembleCmdRep *ensembleCmd = objPtr->internalRep.twoPtrValue.ptr1;
    EnsembleCmdRep *ensembleCopy = ckalloc(sizeof(EnsembleCmdRep));

    copyPtr->typePtr = &ensembleCmdType;
    copyPtr->internalRep.twoPtrValue.ptr1 = ensembleCopy;
    ensembleCopy->epoch = ensembleCmd->epoch;
    ensembleCopy->token = ensembleCmd->token;
    ensembleCopy->token->refCount++;
    ensembleCopy->fix = ensembleCmd->fix;
    if (ensembleCopy->fix) {
	Tcl_IncrRefCount(ensembleCopy->fix);
    }
    ensembleCopy->hPtr = ensembleCmd->hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileEnsemble --
 *
 *	Procedure called to compile an ensemble command. Note that most
 *	ensembles are not compiled, since modifying a compiled ensemble causes
 *	a invalidation of all existing bytecode (expensive!) which is not
 *	normally warranted.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the subcommands of the
 *	ensemble at runtime if a compile-time mapping is possible.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileEnsemble(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);
    Tcl_Obj *mapObj, *subcmdObj, *targetCmdObj, *listObj, **elems;
    Tcl_Obj *replaced = Tcl_NewObj(), *replacement;
    Tcl_Command ensemble = (Tcl_Command) cmdPtr;
    Command *oldCmdPtr = cmdPtr, *newCmdPtr;
    int len, result, flags = 0, i, depth = 1, invokeAnyway = 0;
    int ourResult = TCL_ERROR;
    unsigned numBytes;
    const char *word;
    DefineLineInformation;

    Tcl_IncrRefCount(replaced);
    if (parsePtr->numWords < depth + 1) {
	goto failed;
    }
    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	/*
	 * Too hard.
	 */

	goto failed;
    }

    /*
     * This is where we return to if we are parsing multiple nested compiled
     * ensembles. [info object] is such a beast.
     */

  checkNextWord:
    word = tokenPtr[1].start;
    numBytes = tokenPtr[1].size;

    /*
     * There's a sporting chance we'll be able to compile this. But now we
     * must check properly. To do that, check that we're compiling an ensemble
     * that has a compilable command as its appropriate subcommand.
     */

    if (Tcl_GetEnsembleMappingDict(NULL, ensemble, &mapObj) != TCL_OK
	    || mapObj == NULL) {
	/*
	 * Either not an ensemble or a mapping isn't installed. Crud. Too hard
	 * to proceed.
	 */

	goto failed;
    }

    /*
     * Also refuse to compile anything that uses a formal parameter list for
     * now, on the grounds that it is too complex.
     */

    if (Tcl_GetEnsembleParameterList(NULL, ensemble, &listObj) != TCL_OK
	    || listObj != NULL) {
	/*
	 * Figuring out how to compile this has become too much. Bail out.
	 */

	goto failed;
    }

    /*
     * Next, get the flags. We need them on several code paths so that we can
     * know whether we're to do prefix matching.
     */

    (void) Tcl_GetEnsembleFlags(NULL, ensemble, &flags);

    /*
     * Check to see if there's also a subcommand list; must check to see if
     * the subcommand we are calling is in that list if it exists, since that
     * list filters the entries in the map.
     */

    (void) Tcl_GetEnsembleSubcommandList(NULL, ensemble, &listObj);
    if (listObj != NULL) {
	int sclen;
	const char *str;
	Tcl_Obj *matchObj = NULL;

	if (Tcl_ListObjGetElements(NULL, listObj, &len, &elems) != TCL_OK) {
	    goto failed;
	}
	for (i=0 ; i<len ; i++) {
	    str = Tcl_GetStringFromObj(elems[i], &sclen);
	    if ((sclen == (int) numBytes) && !memcmp(word, str, numBytes)) {
		/*
		 * Exact match! Excellent!
		 */

		result = Tcl_DictObjGet(NULL, mapObj,elems[i], &targetCmdObj);
		if (result != TCL_OK || targetCmdObj == NULL) {
		    goto failed;
		}
		replacement = elems[i];
		goto doneMapLookup;
	    }

	    /*
	     * Check to see if we've got a prefix match. A single prefix match
	     * is fine, and allows us to refine our dictionary lookup, but
	     * multiple prefix matches is a Bad Thing and will prevent us from
	     * making progress. Note that we cannot do the lookup immediately
	     * in the prefix case; might be another entry later in the list
	     * that causes things to fail.
	     */

	    if ((flags & TCL_ENSEMBLE_PREFIX)
		    && strncmp(word, str, numBytes) == 0) {
		if (matchObj != NULL) {
		    goto failed;
		}
		matchObj = elems[i];
	    }
	}
	if (matchObj == NULL) {
	    goto failed;
	}
	result = Tcl_DictObjGet(NULL, mapObj, matchObj, &targetCmdObj);
	if (result != TCL_OK || targetCmdObj == NULL) {
	    goto failed;
	}
	replacement = matchObj;
    } else {
	Tcl_DictSearch s;
	int done, matched;
	Tcl_Obj *tmpObj;

	/*
	 * No map, so check the dictionary directly.
	 */

	TclNewStringObj(subcmdObj, word, (int) numBytes);
	result = Tcl_DictObjGet(NULL, mapObj, subcmdObj, &targetCmdObj);
	if (result == TCL_OK && targetCmdObj != NULL) {
	    /*
	     * Got it. Skip the fiddling around with prefixes.
	     */

	    replacement = subcmdObj;
	    goto doneMapLookup;
	}
	TclDecrRefCount(subcmdObj);

	/*
	 * We've not literally got a valid subcommand. But maybe we have a
	 * prefix. Check if prefix matches are allowed.
	 */

	if (!(flags & TCL_ENSEMBLE_PREFIX)) {
	    goto failed;
	}

	/*
	 * Iterate over the keys in the dictionary, checking to see if we're a
	 * prefix.
	 */

	Tcl_DictObjFirst(NULL, mapObj, &s, &subcmdObj, &tmpObj, &done);
	matched = 0;
	replacement = NULL;		/* Silence, fool compiler! */
	while (!done) {
	    if (strncmp(TclGetString(subcmdObj), word, numBytes) == 0) {
		if (matched++) {
		    /*
		     * Must have matched twice! Not unique, so no point
		     * looking further.
		     */

		    break;
		}
		replacement = subcmdObj;
		targetCmdObj = tmpObj;
	    }
	    Tcl_DictObjNext(&s, &subcmdObj, &tmpObj, &done);
	}
	Tcl_DictObjDone(&s);

	/*
	 * If we have anything other than a single match, we've failed the
	 * unique prefix check.
	 */

	if (matched != 1) {
	    invokeAnyway = 1;
	    goto failed;
	}
    }

    /*
     * OK, we definitely map to something. But what?
     *
     * The command we map to is the first word out of the map element. Note
     * that we also reject dealing with multi-element rewrites if we are in a
     * safe interpreter, as there is otherwise a (highly gnarly!) way to make
     * Tcl crash open to exploit.
     */

  doneMapLookup:
    Tcl_ListObjAppendElement(NULL, replaced, replacement);
    if (Tcl_ListObjGetElements(NULL, targetCmdObj, &len, &elems) != TCL_OK) {
	goto failed;
    } else if (len != 1) {
	/*
	 * Note that at this point we know we can't issue any special
	 * instruction sequence as the mapping isn't one that we support at
	 * the compiled level.
	 */

	goto cleanup;
    }
    targetCmdObj = elems[0];

    oldCmdPtr = cmdPtr;
    Tcl_IncrRefCount(targetCmdObj);
    newCmdPtr = (Command *) Tcl_GetCommandFromObj(interp, targetCmdObj);
    TclDecrRefCount(targetCmdObj);
    if (newCmdPtr == NULL || Tcl_IsSafe(interp)
	    || newCmdPtr->nsPtr->flags & NS_SUPPRESS_COMPILATION
	    || newCmdPtr->flags & CMD_HAS_EXEC_TRACES
	    || ((Interp *)interp)->flags & DONT_COMPILE_CMDS_INLINE) {
	/*
	 * Maps to an undefined command or a command without a compiler.
	 * Cannot compile.
	 */

	goto cleanup;
    }
    cmdPtr = newCmdPtr;
    depth++;

    /*
     * See whether we have a nested ensemble. If we do, we can go round the
     * mulberry bush again, consuming the next word.
     */

    if (cmdPtr->compileProc == TclCompileEnsemble) {
	tokenPtr = TokenAfter(tokenPtr);
	if (parsePtr->numWords < depth + 1
		|| tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    /*
	     * Too hard because the user has done something unpleasant like
	     * omitting the sub-ensemble's command name or used a non-constant
	     * name for a sub-ensemble's command name; we respond by bailing
	     * out completely (this is a rare case). [Bug 6d2f249a01]
	     */

	    goto cleanup;
	}
	ensemble = (Tcl_Command) cmdPtr;
	goto checkNextWord;
    }

    /*
     * Now we've done the mapping process, can now actually try to compile.
     * If there is a subcommand compiler and that successfully produces code,
     * we'll use that. Otherwise, we fall back to generating opcodes to do the
     * invoke at runtime.
     */

    invokeAnyway = 1;
    if (TCL_OK == TclAttemptCompileProc(interp, parsePtr, depth, cmdPtr,
	    envPtr)) {
	ourResult = TCL_OK;
	goto cleanup;
    }

    /*
     * Throw out any line information generated by the failed compile attempt.
     */

    while (mapPtr->nuloc - 1 > eclIndex) {
        mapPtr->nuloc--;
        ckfree(mapPtr->loc[mapPtr->nuloc].line);
        mapPtr->loc[mapPtr->nuloc].line = NULL;
    }

    /*
     * Reset the index of next command.  Toss out any from failed nested
     * partial compiles.
     */

    envPtr->numCommands = mapPtr->nuloc;

    /*
     * Failed to do a full compile for some reason. Try to do a direct invoke
     * instead of going through the ensemble lookup process again.
     */

  failed:
    if (depth < 250) {
	if (depth > 1) {
	    if (!invokeAnyway) {
		cmdPtr = oldCmdPtr;
		depth--;
	    }
	}
	/*
	 * The length of the "replaced" list must be depth-1.  Trim back
	 * any extra elements that might have been appended by failing
	 * pathways above.
	 */
	(void) Tcl_ListObjReplace(NULL, replaced, depth-1, LIST_MAX, 0, NULL);

	/*
	 * TODO: Reconsider whether we ought to call CompileToInvokedCommand()
	 * when depth==1.  In that case we are choosing to emit the
	 * INST_INVOKE_REPLACE bytecode when there is in fact no replacing
	 * to be done.  It would be equally functional and presumably more
	 * performant to fall through to cleanup below, return TCL_ERROR,
	 * and let the compiler harness emit the INST_INVOKE_STK
	 * implementation for us.
	 */

	CompileToInvokedCommand(interp, parsePtr, replaced, cmdPtr, envPtr);
	ourResult = TCL_OK;
    }

    /*
     * Release the memory we allocated. If we've got here, we've either done
     * something useful or we're in a case that we can't compile at all and
     * we're just giving up.
     */

  cleanup:
    Tcl_DecrRefCount(replaced);
    return ourResult;
}

int
TclAttemptCompileProc(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    int depth,
    Command *cmdPtr,
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    int result, i;
    Tcl_Token *saveTokenPtr = parsePtr->tokenPtr;
    int savedStackDepth = envPtr->currStackDepth;
    unsigned savedCodeNext = envPtr->codeNext - envPtr->codeStart;
    int savedAuxDataArrayNext = envPtr->auxDataArrayNext;
    int savedExceptArrayNext = envPtr->exceptArrayNext;
#ifdef TCL_COMPILE_DEBUG
    int savedExceptDepth = envPtr->exceptDepth;
#endif
    DefineLineInformation;

    if (cmdPtr->compileProc == NULL) {
	return TCL_ERROR;
    }

    /*
     * Advance parsePtr->tokenPtr so that it points at the last subcommand.
     * This will be wrong, but it will not matter, and it will put the
     * tokens for the arguments in the right place without the needed to
     * allocate a synthetic Tcl_Parse struct, or copy tokens around.
     */

    for (i = 0; i < depth - 1; i++) {
	parsePtr->tokenPtr = TokenAfter(parsePtr->tokenPtr);
    }
    parsePtr->numWords -= (depth - 1);

    /*
     * Shift the line information arrays to account for different word
     * index values.
     */

    mapPtr->loc[eclIndex].line += (depth - 1);
    mapPtr->loc[eclIndex].next += (depth - 1);

    /*
     * Hand off compilation to the subcommand compiler. At last!
     */

    result = cmdPtr->compileProc(interp, parsePtr, cmdPtr, envPtr);

    /*
     * Undo the shift.
     */

    mapPtr->loc[eclIndex].line -= (depth - 1);
    mapPtr->loc[eclIndex].next -= (depth - 1);

    parsePtr->numWords += (depth - 1);
    parsePtr->tokenPtr = saveTokenPtr;

    /*
     * If our target failed to compile, revert any data from failed partial
     * compiles.  Note that envPtr->numCommands need not be checked because
     * we avoid compiling subcommands that recursively call TclCompileScript().
     */

#ifdef TCL_COMPILE_DEBUG
    if (envPtr->exceptDepth != savedExceptDepth) {
	Tcl_Panic("ExceptionRange Starts and Ends do not balance");
    }
#endif

    if (result != TCL_OK) {
	ExceptionAux *auxPtr = envPtr->exceptAuxArrayPtr;

	for (i = 0; i < savedExceptArrayNext; i++) {
	    while (auxPtr->numBreakTargets > 0
		    && auxPtr->breakTargets[auxPtr->numBreakTargets - 1]
		    >= savedCodeNext) {
		auxPtr->numBreakTargets--;
	    }
	    while (auxPtr->numContinueTargets > 0
		    && auxPtr->continueTargets[auxPtr->numContinueTargets - 1]
		    >= savedCodeNext) {
		auxPtr->numContinueTargets--;
	    }
	    auxPtr++;
	}
	envPtr->exceptArrayNext = savedExceptArrayNext;

	if (savedAuxDataArrayNext != envPtr->auxDataArrayNext) {
	    AuxData *auxDataPtr = envPtr->auxDataArrayPtr;
	    AuxData *auxDataEnd = auxDataPtr;

	    auxDataPtr += savedAuxDataArrayNext;
	    auxDataEnd += envPtr->auxDataArrayNext;

	    while (auxDataPtr < auxDataEnd) {
		if (auxDataPtr->type->freeProc != NULL) {
		    auxDataPtr->type->freeProc(auxDataPtr->clientData);
		}
		auxDataPtr++;
	    }
	    envPtr->auxDataArrayNext = savedAuxDataArrayNext;
	}
	envPtr->currStackDepth = savedStackDepth;
	envPtr->codeNext = envPtr->codeStart + savedCodeNext;
#ifdef TCL_COMPILE_DEBUG
    } else {
	/*
	 * Confirm that the command compiler generated a single value on
	 * the stack as its result. This is only done in debugging mode,
	 * as it *should* be correct and normal users have no reasonable
	 * way to fix it anyway.
	 */

	int diff = envPtr->currStackDepth - savedStackDepth;

	if (diff != 1) {
	    Tcl_Panic("bad stack adjustment when compiling"
		    " %.*s (was %d instead of 1)", parsePtr->tokenPtr->size,
		    parsePtr->tokenPtr->start, diff);
	}
#endif
    }

    return result;
}

/*
 * How to compile a subcommand to a _replacing_ invoke of its implementation
 * command.
 */

static void
CompileToInvokedCommand(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Tcl_Obj *replacements,
    Command *cmdPtr,
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokPtr;
    Tcl_Obj *objPtr, **words;
    char *bytes;
    int length, i, numWords, cmdLit, extraLiteralFlags = LITERAL_CMD_NAME;
    DefineLineInformation;

    /*
     * Push the words of the command. Take care; the command words may be
     * scripts that have backslashes in them, and [info frame 0] can see the
     * difference. Hence the call to TclContinuationsEnterDerived...
     */

    Tcl_ListObjGetElements(NULL, replacements, &numWords, &words);
    for (i = 0, tokPtr = parsePtr->tokenPtr; i < parsePtr->numWords;
	    i++, tokPtr = TokenAfter(tokPtr)) {
	if (i > 0 && i < numWords+1) {
	    bytes = Tcl_GetStringFromObj(words[i-1], &length);
	    PushLiteral(envPtr, bytes, length);
	    continue;
	}

	SetLineInformation(i);
	if (tokPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    int literal = TclRegisterNewLiteral(envPtr,
		    tokPtr[1].start, tokPtr[1].size);

	    if (envPtr->clNext) {
		TclContinuationsEnterDerived(
			TclFetchLiteral(envPtr, literal),
			tokPtr[1].start - envPtr->source,
			envPtr->clNext);
	    }
	    TclEmitPush(literal, envPtr);
	} else {
	    CompileTokens(envPtr, tokPtr, interp);
	}
    }

    /*
     * Push the name of the command we're actually dispatching to as part of
     * the implementation.
     */

    objPtr = Tcl_NewObj();
    Tcl_GetCommandFullName(interp, (Tcl_Command) cmdPtr, objPtr);
    bytes = Tcl_GetStringFromObj(objPtr, &length);
    if ((cmdPtr != NULL) && (cmdPtr->flags & CMD_VIA_RESOLVER)) {
	extraLiteralFlags |= LITERAL_UNSHARED;
    }
    cmdLit = TclRegisterLiteral(envPtr, (char *)bytes, length, extraLiteralFlags);
    TclSetCmdNameObj(interp, TclFetchLiteral(envPtr, cmdLit), cmdPtr);
    TclEmitPush(cmdLit, envPtr);
    TclDecrRefCount(objPtr);

    /*
     * Do the replacing dispatch.
     */

    TclEmitInvoke(envPtr, INST_INVOKE_REPLACE, parsePtr->numWords,numWords+1);
}

/*
 * Helpers that do issuing of instructions for commands that "don't have
 * compilers" (well, they do; these). They all work by just generating base
 * code to invoke the command; they're intended for ensemble subcommands so
 * that the costs of INST_INVOKE_REPLACE can be avoided where we can work out
 * that they're not needed.
 *
 * Note that these are NOT suitable for commands where there's an argument
 * that is a script, as an [info level] or [info frame] in the inner context
 * can see the difference.
 */

static int
CompileBasicNArgCommand(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Obj *objPtr = Tcl_NewObj();

    Tcl_IncrRefCount(objPtr);
    Tcl_GetCommandFullName(interp, (Tcl_Command) cmdPtr, objPtr);
    TclCompileInvocation(interp, parsePtr->tokenPtr, objPtr,
	    parsePtr->numWords, envPtr);
    Tcl_DecrRefCount(objPtr);
    return TCL_OK;
}

int
TclCompileBasic0ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords != 1) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic1ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic2ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic3ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords != 4) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic0Or1ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords != 1 && parsePtr->numWords != 2) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic1Or2ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords != 2 && parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic2Or3ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords != 3 && parsePtr->numWords != 4) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic0To2ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords < 1 || parsePtr->numWords > 3) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasic1To3ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords < 2 || parsePtr->numWords > 4) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasicMin0ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords < 1) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasicMin1ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords < 2) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileBasicMin2ArgCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Verify that the number of arguments is correct; that's the only case
     * that we know will avoid the call to Tcl_WrongNumArgs() at invoke time,
     * which is the only code that sees the shenanigans of ensemble dispatch.
     */

    if (parsePtr->numWords < 3) {
	return TCL_ERROR;
    }

    return CompileBasicNArgCommand(interp, parsePtr, cmdPtr, envPtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
