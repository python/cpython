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
 *  This part handles ensembles, which support compound commands in Tcl.
 *  The usual "info" command is an ensemble with parts like "info body"
 *  and "info globals".  Extension developers can extend commands like
 *  "info" by adding their own parts to the ensemble.
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

#define ITCL_ENSEMBLE_CUSTOM        0x01
#define ITCL_ENSEMBLE_ENSEMBLE      0x02

/*
 *  Data used to represent an ensemble:
 */
struct Ensemble;
typedef struct EnsemblePart {
    char *name;                 /* name of this part */
    Tcl_Obj *namePtr;
    Tcl_Command cmdPtr;         /* command handling this part */
    char *usage;                /* usage string describing syntax */
    struct Ensemble* ensemble;  /* ensemble containing this part */
    ItclArgList *arglistPtr;    /* the parsed argument list */
    Tcl_ObjCmdProc *objProc;    /* handling procedure for part */
    void *clientData;           /* the procPtr for the part */
    Tcl_CmdDeleteProc *deleteProc;
                                /* procedure used to destroy client data */
    int minChars;               /* chars needed to uniquely identify part */
    int flags;
    Tcl_Interp *interp;
    Tcl_Obj *mapNamePtr;
    Tcl_Obj *subEnsemblePtr;
    Tcl_Obj *newMapDict;
} EnsemblePart;

#define ENSEMBLE_DELETE_STARTED      0x1
#define ENSEMBLE_PART_DELETE_STARTED 0x2

/*
 *  Data used to represent an ensemble:
 */
typedef struct Ensemble {
    Tcl_Interp *interp;         /* interpreter containing this ensemble */
    EnsemblePart **parts;       /* list of parts in this ensemble */
    int numParts;               /* number of parts in part list */
    int maxParts;               /* current size of parts list */
    int ensembleId;             /* this ensembles id */
    Tcl_Command cmdPtr;         /* command representing this ensemble */
    EnsemblePart* parent;       /* parent part for sub-ensembles
                                 * NULL => toplevel ensemble */
    Tcl_Namespace *nsPtr;       /* namespace for ensemble part commands */
    int flags;
    Tcl_Obj *namePtr;
} Ensemble;

/*
 *  Data shared by ensemble access commands and ensemble parser:
 */
typedef struct EnsembleParser {
    Tcl_Interp* master;           /* master interp containing ensembles */
    Tcl_Interp* parser;           /* slave interp for parsing */
    Ensemble* ensData;            /* add parts to this ensemble */
} EnsembleParser;

static int EnsembleSubCmd(ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *const objv[]);
static int EnsembleUnknownCmd(ClientData dummy, Tcl_Interp *interp,
    int objc, Tcl_Obj *const objv[]);

/*
 *  Forward declarations for the procedures used in this file.
 */
static void GetEnsembleUsage (Tcl_Interp *interp,
    Ensemble *ensData, Tcl_Obj *objPtr);
static void GetEnsemblePartUsage (Tcl_Interp *interp,
    Ensemble *ensData, EnsemblePart *ensPart, Tcl_Obj *objPtr);
static int CreateEnsemble (Tcl_Interp *interp,
    Ensemble *parentEnsData, const char *ensName);
static int AddEnsemblePart (Tcl_Interp *interp,
    Ensemble* ensData, const char* partName, const char* usageInfo,
    Tcl_ObjCmdProc *objProc, ClientData clientData,
    Tcl_CmdDeleteProc *deleteProc, int flags, EnsemblePart **rVal);
static int FindEnsemble (Tcl_Interp *interp, const char **nameArgv,
    int nameArgc, Ensemble** ensDataPtr);
static int CreateEnsemblePart (Tcl_Interp *interp,
    Ensemble *ensData, const char* partName, EnsemblePart **ensPartPtr);
static void DeleteEnsemblePart (ClientData clientData);
static int FindEnsemblePart (Tcl_Interp *interp,
    Ensemble *ensData, const char* partName, EnsemblePart **rensPart);
static void DeleteEnsemble(ClientData clientData);
static int FindEnsemblePartIndex (Ensemble *ensData,
    const char *partName, int *posPtr);
static void ComputeMinChars (Ensemble *ensData, int pos);
static EnsembleParser* GetEnsembleParser (Tcl_Interp *interp);
static void DeleteEnsParser (ClientData clientData, Tcl_Interp* interp);


/*
 *----------------------------------------------------------------------
 *
 * Itcl_EnsembleInit --
 *
 *      Called when any interpreter is created to make sure that
 *      things are properly set up for ensembles.
 *
 * Results:
 *      Returns TCL_OK if successful, and TCL_ERROR if anything goes
 *      wrong.
 *
 * Side effects:
 *      On the first call, the "ensemble" object type is registered
 *      with the Tcl compiler.  If an error is encountered, an error
 *      is left as the result in the interpreter.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */
int
Itcl_EnsembleInit(
    Tcl_Interp *interp)         /* interpreter being initialized */
{
    Tcl_DString buffer;
    ItclObjectInfo *infoPtr;

    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    Tcl_CreateObjCommand(interp, "::itcl::ensemble",
        Itcl_EnsembleCmd, NULL, NULL);

    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, ITCL_COMMANDS_NAMESPACE, -1);
    Tcl_DStringAppend(&buffer, "::ensembles", -1);
    infoPtr->ensembleInfo->ensembleNsPtr = Tcl_CreateNamespace(interp,
            Tcl_DStringValue(&buffer), NULL, NULL);
    Tcl_DStringFree(&buffer);
    if (infoPtr->ensembleInfo->ensembleNsPtr == NULL) {
        Tcl_AppendResult(interp, "error in creating namespace: ",
	        Tcl_DStringValue(&buffer), NULL);
        return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp,
            ITCL_COMMANDS_NAMESPACE "::ensembles::unknown",
	    EnsembleUnknownCmd, NULL, NULL);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_CreateEnsemble --
 *
 *      Creates an ensemble command, or adds a sub-ensemble to an
 *      existing ensemble command.  The ensemble name is a space-
 *      separated list.  The first word in the list is the command
 *      name for the top-level ensemble.  Other names do not have
 *      commands associated with them; they are merely sub-ensembles
 *      within the ensemble.  So a name like "a::b::foo bar baz"
 *      represents an ensemble command called "foo" in the namespace
 *      "a::b" that has a sub-ensemble "bar", that has a sub-ensemble
 *      "baz".
 *
 *      If the name is a single word, then this procedure creates
 *      a top-level ensemble and installs an access command for it.
 *      If a command already exists with that name, it is deleted.
 *
 *      If the name has more than one word, then the leading words
 *      are treated as a path name for an existing ensemble.  The
 *      last word is treated as the name for a new sub-ensemble.
 *      If an part already exists with that name, it is an error.
 *
 * Results:
 *      Returns TCL_OK if successful, and TCL_ERROR if anything goes
 *      wrong.
 *
 * Side effects:
 *      If an error is encountered, an error is left as the result
 *      in the interpreter.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_CreateEnsemble(
    Tcl_Interp *interp,            /* interpreter to be updated */
    const char* ensName)           /* name of the new ensemble */
{
    const char **nameArgv = NULL;
    int nameArgc;
    Ensemble *parentEnsData;

    /*
     *  Split the ensemble name into its path components.
     */
    if (Tcl_SplitList(interp, (const char *)ensName, &nameArgc,
	    &nameArgv) != TCL_OK) {
        goto ensCreateFail;
    }
    if (nameArgc < 1) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "invalid ensemble name \"", ensName, "\"",
            NULL);
        goto ensCreateFail;
    }

    /*
     *  If there is more than one path component, then follow
     *  the path down to the last component, to find the containing
     *  ensemble.
     */
    parentEnsData = NULL;
    if (nameArgc > 1) {
        if (FindEnsemble(interp, nameArgv, nameArgc-1, &parentEnsData)
            != TCL_OK) {
            goto ensCreateFail;
        }

        if (parentEnsData == NULL) {
            char *pname = Tcl_Merge(nameArgc-1, nameArgv);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "invalid ensemble name \"", pname, "\"",
                NULL);
            ckfree(pname);
            goto ensCreateFail;
        }
    }

    /*
     *  Create the ensemble.
     */
    if (CreateEnsemble(interp, parentEnsData, nameArgv[nameArgc-1])
        != TCL_OK) {
        goto ensCreateFail;
    }

    ckfree((char*)nameArgv);
    return TCL_OK;

ensCreateFail:
    if (nameArgv) {
        ckfree((char*)nameArgv);
    }
    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
            "\n    (while creating ensemble \"%s\")",
            ensName));

    return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_AddEnsemblePart --
 *
 *      Adds a part to an ensemble which has been created by
 *      Itcl_CreateEnsemble.  Ensembles are addressed by name, as
 *      described in Itcl_CreateEnsemble.
 *
 *      If the ensemble already has a part with the specified name,
 *      this procedure returns an error.  Otherwise, it adds a new
 *      part to the ensemble.
 *
 *      Any client data specified is automatically passed to the
 *      handling procedure whenever the part is invoked.  It is
 *      automatically destroyed by the deleteProc when the part is
 *      deleted.
 *
 * Results:
 *      Returns TCL_OK if successful, and TCL_ERROR if anything goes
 *      wrong.
 *
 * Side effects:
 *      If an error is encountered, an error is left as the result
 *      in the interpreter.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_AddEnsemblePart(
    Tcl_Interp *interp,            /* interpreter to be updated */
    const char* ensName,           /* ensemble containing this part */
    const char* partName,          /* name of the new part */
    const char* usageInfo,         /* usage info for argument list */
    Tcl_ObjCmdProc *objProc,       /* handling procedure for part */
    ClientData clientData,         /* client data associated with part */
    Tcl_CmdDeleteProc *deleteProc) /* procedure used to destroy client data */
{
    const char **nameArgv = NULL;
    int nameArgc;
    Ensemble *ensData;
    EnsemblePart *ensPart;

    /*
     *  Parse the ensemble name and look for a containing ensemble.
     */
    if (Tcl_SplitList(interp, (const char *)ensName, &nameArgc,
	    &nameArgv) != TCL_OK) {
        goto ensPartFail;
    }
    if (FindEnsemble(interp, nameArgv, nameArgc, &ensData) != TCL_OK) {
        goto ensPartFail;
    }

    if (ensData == NULL) {
        char *pname = Tcl_Merge(nameArgc, nameArgv);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "invalid ensemble name \"", pname, "\"",
            NULL);
        ckfree(pname);
        goto ensPartFail;
    }

    /*
     *  Install the new part into the part list.
     */
    if (AddEnsemblePart(interp, ensData, partName, usageInfo,
            objProc, clientData, deleteProc, ITCL_ENSEMBLE_CUSTOM,
	    &ensPart) != TCL_OK) {
        goto ensPartFail;
    }

    ckfree((char*)nameArgv);
    return TCL_OK;

ensPartFail:
    if (nameArgv) {
        ckfree((char*)nameArgv);
    }
    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
            "\n    (while adding to ensemble \"%s\")",
            ensName));

    return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_GetEnsemblePart --
 *
 *      Looks for a part within an ensemble, and returns information
 *      about it.
 *
 * Results:
 *      If the ensemble and its part are found, this procedure
 *      loads information about the part into the "infoPtr" structure
 *      and returns 1.  Otherwise, it returns 0.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_GetEnsemblePart(
    Tcl_Interp *interp,            /* interpreter to be updated */
    const char *ensName,           /* ensemble containing the part */
    const char *partName,          /* name of the desired part */
    Tcl_CmdInfo *infoPtr)          /* returns: info associated with part */
{
    const char **nameArgv = NULL;
    int nameArgc;
    Ensemble *ensData;
    EnsemblePart *ensPart;
    Itcl_InterpState state;

    /*
     *  Parse the ensemble name and look for a containing ensemble.
     *  Save the interpreter state before we do this.  If we get any
     *  errors, we don't want them to affect the interpreter.
     */
    state = Itcl_SaveInterpState(interp, TCL_OK);

    if (Tcl_SplitList(interp, (const char *)ensName, &nameArgc,
	    &nameArgv) != TCL_OK) {
        goto ensGetFail;
    }
    if (FindEnsemble(interp, nameArgv, nameArgc, &ensData) != TCL_OK) {
        goto ensGetFail;
    }
    if (ensData == NULL) {
        goto ensGetFail;
    }

    /*
     *  Look for a part with the desired name.  If found, load
     *  its data into the "infoPtr" structure.
     */
    if (FindEnsemblePart(interp, ensData, partName, &ensPart)
        != TCL_OK || ensPart == NULL) {
        goto ensGetFail;
    }

    if (Tcl_GetCommandInfoFromToken(ensPart->cmdPtr, infoPtr) != 1) {
        goto ensGetFail;
    }

    Itcl_DiscardInterpState(state);
    ckfree((char *)nameArgv);
    return 1;

ensGetFail:
    if (nameArgv) {
	ckfree((char *)nameArgv);
    }
    Itcl_RestoreInterpState(interp, state);
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_IsEnsemble --
 *
 *      Determines whether or not an existing command is an ensemble.
 *
 * Results:
 *      Returns non-zero if the command is an ensemble, and zero
 *      otherwise.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_IsEnsemble(
    Tcl_CmdInfo* infoPtr)  /* command info from Tcl_GetCommandInfo() */
{
    if (infoPtr) {
/* FIXME use CMD and Tcl_IsEnsemble!! */
        return (infoPtr->deleteProc == DeleteEnsemble);
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_GetEnsembleUsage --
 *
 *      Returns a summary of all of the parts of an ensemble and
 *      the meaning of their arguments.  Each part is listed on
 *      a separate line.  Having this summary is sometimes useful
 *      when building error messages for the "@error" handler in
 *      an ensemble.
 *
 *      Ensembles are accessed by name, as described in
 *      Itcl_CreateEnsemble.
 *
 * Results:
 *      If the ensemble is found, its usage information is appended
 *      onto the object "objPtr", and this procedure returns
 *      non-zero.  It is the responsibility of the caller to
 *      initialize and free the object.  If anything goes wrong,
 *      this procedure returns 0.
 *
 * Side effects:
 *      Object passed in is modified.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_GetEnsembleUsage(
    Tcl_Interp *interp,    /* interpreter containing the ensemble */
    const char *ensName,   /* name of the ensemble */
    Tcl_Obj *objPtr)       /* returns: summary of usage info */
{
    const char **nameArgv = NULL;
    int nameArgc;
    Ensemble *ensData;
    Itcl_InterpState state;

    /*
     *  Parse the ensemble name and look for the ensemble.
     *  Save the interpreter state before we do this.  If we get
     *  any errors, we don't want them to affect the interpreter.
     */
    state = Itcl_SaveInterpState(interp, TCL_OK);

    if (Tcl_SplitList(interp, (const char *)ensName, &nameArgc,
	    &nameArgv) != TCL_OK) {
        goto ensUsageFail;
    }
    if (FindEnsemble(interp, nameArgv, nameArgc, &ensData) != TCL_OK) {
        goto ensUsageFail;
    }
    if (ensData == NULL) {
        goto ensUsageFail;
    }

    /*
     *  Add a summary of usage information to the return buffer.
     */
    GetEnsembleUsage(interp, ensData, objPtr);

    Itcl_DiscardInterpState(state);
    ckfree((char *)nameArgv);
    return 1;

ensUsageFail:
    if (nameArgv) {
	ckfree((char *)nameArgv);
    }
    Itcl_RestoreInterpState(interp, state);
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_GetEnsembleUsageForObj --
 *
 *      Returns a summary of all of the parts of an ensemble and
 *      the meaning of their arguments.  This procedure is just
 *      like Itcl_GetEnsembleUsage, but it determines the desired
 *      ensemble from a command line argument.  The argument should
 *      be the first argument on the command line--the ensemble
 *      command or one of its parts.
 *
 * Results:
 *      If the ensemble is found, its usage information is appended
 *      onto the object "objPtr", and this procedure returns
 *      non-zero.  It is the responsibility of the caller to
 *      initialize and free the object.  If anything goes wrong,
 *      this procedure returns 0.
 *
 * Side effects:
 *      Object passed in is modified.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_GetEnsembleUsageForObj(
    Tcl_Interp *interp,    /* interpreter containing the ensemble */
    Tcl_Obj *ensObjPtr,    /* argument representing ensemble */
    Tcl_Obj *objPtr)       /* returns: summary of usage info */
{
    Ensemble *ensData;
    Tcl_Obj *chainObj;
    Tcl_Command cmd;
    Tcl_CmdInfo infoPtr;

    /*
     *  If the argument is an ensemble part, then follow the chain
     *  back to the command word for the entire ensemble.
     */
    chainObj = ensObjPtr;

    if (chainObj) {
        cmd = Tcl_GetCommandFromObj(interp, chainObj);
        if (Tcl_GetCommandInfoFromToken(cmd, &infoPtr) != 1) {
            return 0;
        }
        if (infoPtr.deleteProc == DeleteEnsemble) {
            ensData = (Ensemble*)infoPtr.objClientData;
            GetEnsembleUsage(interp, ensData, objPtr);
            return 1;
        }
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * GetEnsembleUsage --
 *
 *
 *      Returns a summary of all of the parts of an ensemble and
 *      the meaning of their arguments.  Each part is listed on
 *      a separate line.  This procedure is used internally to
 *      generate usage information for error messages.
 *
 * Results:
 *      Appends usage information onto the object in "objPtr".
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static void
GetEnsembleUsage(
    Tcl_Interp *interp,
    Ensemble *ensData,     /* ensemble data */
    Tcl_Obj *objPtr)       /* returns: summary of usage info */
{
    const char *spaces = "  ";
    int isOpenEnded = 0;

    int i;
    EnsemblePart *ensPart;

    for (i=0; i < ensData->numParts; i++) {
        ensPart = ensData->parts[i];

        if ((*ensPart->name == '@') && (strcmp(ensPart->name,"@error") == 0)) {
            isOpenEnded = 1;
        } else {
            if ((*ensPart->name == '@') &&
	            (strcmp(ensPart->name,"@itcl-builtin_info") == 0)) {
		/* the builtin info command is not reported in [incr tcl] */
	        continue;
	    }
            Tcl_AppendToObj(objPtr, spaces, -1);
            GetEnsemblePartUsage(interp, ensData, ensPart, objPtr);
            spaces = "\n  ";
        }
    }
    if (isOpenEnded) {
        Tcl_AppendToObj(objPtr,
            "\n...and others described on the man page", -1);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * GetEnsemblePartUsage --
 *
 *      Determines the usage for a single part within an ensemble,
 *      and appends a summary onto a dynamic string.  The usage
 *      is a combination of the part name and the argument summary.
 *      It is the caller's responsibility to initialize and free
 *      the dynamic string.
 *
 * Results:
 *      Returns usage information in the object "objPtr".
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static void
GetEnsemblePartUsage(
    Tcl_Interp *interp,
    Ensemble *ensData,
    EnsemblePart *ensPart,   /* ensemble part for usage info */
    Tcl_Obj *objPtr)         /* returns: usage information */
{
    EnsemblePart *part;
    Tcl_Command cmdPtr;
    const char *name;
    Itcl_List trail;
    Itcl_ListElem *elem;
    Tcl_DString buffer;

    /*
     *  Build the trail of ensemble names leading to this part.
     */
    Tcl_DStringInit(&buffer);
    Itcl_InitList(&trail);
    for (part=ensPart; part; part=part->ensemble->parent) {
        Itcl_InsertList(&trail, part);
    }

    while (ensData->parent != NULL) {
        ensData = ensData->parent->ensemble;
    }
    cmdPtr = ensData->cmdPtr;
    name = Tcl_GetCommandName(interp, cmdPtr);
    Tcl_DStringAppendElement(&buffer, name);

    for (elem=Itcl_FirstListElem(&trail); elem; elem=Itcl_NextListElem(elem)) {
        part = (EnsemblePart*)Itcl_GetListValue(elem);
        Tcl_DStringAppendElement(&buffer, part->name);
    }
    Itcl_DeleteList(&trail);

    /*
     *  If the part has usage info, use it directly.
     */
    if (ensPart->usage && *ensPart->usage != '\0') {
        Tcl_DStringAppend(&buffer, " ", 1);
        Tcl_DStringAppend(&buffer, ensPart->usage, -1);
    } else {

        /*
         *  If the part is itself an ensemble, summarize its usage.
         */
        if (ensPart->cmdPtr != NULL) {
	    if (Tcl_IsEnsemble(ensPart->cmdPtr)) {
                Tcl_DStringAppend(&buffer, " option ?arg arg ...?", 21);
	    }
        }
    }

    Tcl_AppendToObj(objPtr, Tcl_DStringValue(&buffer),
        Tcl_DStringLength(&buffer));

    Tcl_DStringFree(&buffer);
}


/*
 *----------------------------------------------------------------------
 *
 * CreateEnsemble --
 *
 *      Creates an ensemble command, or adds a sub-ensemble to an
 *      existing ensemble command.  Works like Itcl_CreateEnsemble,
 *      except that the ensemble name is a single name, not a path.
 *      If a parent ensemble is specified, then a new ensemble is
 *      added to that parent.  If a part already exists with the
 *      same name, it is an error.  If a parent ensemble is not
 *      specified, then a top-level ensemble is created.  If a
 *      command already exists with the same name, it is deleted.
 *
 * Results:
 *      Returns TCL_OK if successful, and TCL_ERROR if anything goes
 *      wrong.
 *
 * Side effects:
 *      If an error is encountered, an error is left as the result
 *      in the interpreter.
 *
 *----------------------------------------------------------------------
 */
static int
CreateEnsemble(
    Tcl_Interp *interp,            /* interpreter to be updated */
    Ensemble *parentEnsData,       /* parent ensemble or NULL */
    const char *ensName)           /* name of the new ensemble */
{
    Tcl_Obj *objPtr;
    Tcl_DString buffer;
    Tcl_HashEntry *hPtr;
    Tcl_Obj *mapDict;
    Tcl_Obj *toObjPtr;
    ItclObjectInfo *infoPtr;
    Ensemble *ensData;
    EnsemblePart *ensPart;
    int result;
    int isNew;
    char buf[20];
    Tcl_Obj *unkObjPtr;

    /*
     *  Create the data associated with the ensemble.
     */
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    infoPtr->ensembleInfo->numEnsembles++;
    ensData = (Ensemble*)ckalloc(sizeof(Ensemble));
    memset(ensData, 0, sizeof(Ensemble));
    ensData->namePtr = Tcl_NewStringObj(ensName, -1);
    Tcl_IncrRefCount(ensData->namePtr);
    ensData->interp = interp;
    ensData->numParts = 0;
    ensData->maxParts = 10;
    ensData->ensembleId = infoPtr->ensembleInfo->numEnsembles;
    ensData->parts = (EnsemblePart**)ckalloc(
        (unsigned)(ensData->maxParts*sizeof(EnsemblePart*))
    );
    memset(ensData->parts, 0, ensData->maxParts*sizeof(EnsemblePart*));
    Tcl_DStringInit(&buffer);
    Tcl_DStringAppend(&buffer, ITCL_COMMANDS_NAMESPACE "::ensembles::", -1);
    sprintf(buf, "%d", ensData->ensembleId);
    Tcl_DStringAppend(&buffer, buf, -1);
    ensData->nsPtr = Tcl_CreateNamespace(interp, Tcl_DStringValue(&buffer),
            ensData, DeleteEnsemble);
    if (ensData->nsPtr == NULL) {
        Tcl_AppendResult(interp, "error in creating namespace: ",
	        Tcl_DStringValue(&buffer), NULL);
        result = TCL_ERROR;
        goto finish;
    }

    /*
     *  If there is no parent data, then this is a top-level
     *  ensemble.  Create the ensemble by installing its access
     *  command.
     */
    if (parentEnsData == NULL) {
	Tcl_Obj *unkObjPtr;
	ensData->cmdPtr = Tcl_CreateEnsemble(interp, ensName,
		Tcl_GetCurrentNamespace(interp), TCL_ENSEMBLE_PREFIX);
	hPtr = Tcl_CreateHashEntry(&infoPtr->ensembleInfo->ensembles,
		(char *)ensData->cmdPtr, &isNew);
	if (!isNew) {
	    result = TCL_ERROR;
	    goto finish;
	}
	Tcl_SetHashValue(hPtr, ensData);
	unkObjPtr = Tcl_NewStringObj(ITCL_COMMANDS_NAMESPACE, -1);
	Tcl_AppendToObj(unkObjPtr, "::ensembles::unknown", -1);
	if (Tcl_SetEnsembleUnknownHandler(NULL, ensData->cmdPtr,
		unkObjPtr) != TCL_OK) {
	    Tcl_DecrRefCount(unkObjPtr);
	    result = TCL_ERROR;
	    goto finish;
	}

	Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_DStringValue(&buffer), -1));
	result = TCL_OK;
	goto finish;
    }

    /*
     *  Otherwise, this ensemble is contained within another parent.
     *  Install the new ensemble as a part within its parent.
     */
    if (CreateEnsemblePart(interp, parentEnsData, ensName, &ensPart)
            != TCL_OK) {
        DeleteEnsemble(ensData);
        result = TCL_ERROR;
        goto finish;
    }
    Tcl_DStringSetLength(&buffer, 0);
    Tcl_DStringAppend(&buffer, infoPtr->ensembleInfo->ensembleNsPtr->fullName, -1);
    Tcl_DStringAppend(&buffer, "::subensembles::", -1);
    sprintf(buf, "%d", parentEnsData->ensembleId);
    Tcl_DStringAppend(&buffer, buf, -1);
    Tcl_DStringAppend(&buffer, "::", 2);
    Tcl_DStringAppend(&buffer, ensName, -1);
    objPtr = Tcl_NewStringObj(Tcl_DStringValue(&buffer), -1);
    hPtr = Tcl_CreateHashEntry(&infoPtr->ensembleInfo->subEnsembles,
            (char *)objPtr, &isNew);
    if (isNew) {
        Tcl_SetHashValue(hPtr, ensData);
    }

    ensPart->subEnsemblePtr = objPtr;
    Tcl_IncrRefCount(ensPart->subEnsemblePtr);
    ensPart->cmdPtr = Tcl_CreateEnsemble(interp, Tcl_DStringValue(&buffer),
            Tcl_GetCurrentNamespace(interp), TCL_ENSEMBLE_PREFIX);
    hPtr = Tcl_CreateHashEntry(&infoPtr->ensembleInfo->ensembles,
            (char *)ensPart->cmdPtr, &isNew);
    if (!isNew) {
        result = TCL_ERROR;
        goto finish;
    }
    Tcl_SetHashValue(hPtr, ensData);
    unkObjPtr = Tcl_NewStringObj(ITCL_COMMANDS_NAMESPACE, -1);
    Tcl_AppendToObj(unkObjPtr, "::ensembles::unknown", -1);
    if (Tcl_SetEnsembleUnknownHandler(NULL, ensPart->cmdPtr,
            unkObjPtr) != TCL_OK) {
        result = TCL_ERROR;
        goto finish;
    }

    Tcl_GetEnsembleMappingDict(NULL, parentEnsData->cmdPtr, &mapDict);
    if (mapDict == NULL) {
        mapDict = Tcl_NewObj();
    }
    toObjPtr = Tcl_NewStringObj(Tcl_DStringValue(&buffer), -1);
    Tcl_DictObjPut(NULL, mapDict, ensData->namePtr, toObjPtr);
    Tcl_SetEnsembleMappingDict(NULL, parentEnsData->cmdPtr, mapDict);
    ensData->cmdPtr = ensPart->cmdPtr;
    ensData->parent = ensPart;
    result = TCL_OK;

finish:
    Tcl_DStringFree(&buffer);
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * AddEnsemblePart --
 *
 *      Adds a part to an existing ensemble.  Works like
 *      Itcl_AddEnsemblePart, but the part name is a single word,
 *      not a path.
 *
 *      If the ensemble already has a part with the specified name,
 *      this procedure returns an error.  Otherwise, it adds a new
 *      part to the ensemble.
 *
 *      Any client data specified is automatically passed to the
 *      handling procedure whenever the part is invoked.  It is
 *      automatically destroyed by the deleteProc when the part is
 *      deleted.
 *
 * Results:
 *      Returns TCL_OK if successful, along with a pointer to the
 *      new part.  Returns TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *      If an error is encountered, an error is left as the result
 *      in the interpreter.
 *
 *----------------------------------------------------------------------
 */
static int
AddEnsemblePart(
    Tcl_Interp *interp,            /* interpreter to be updated */
    Ensemble* ensData,             /* ensemble that will contain this part */
    const char* partName,          /* name of the new part */
    const char* usageInfo,         /* usage info for argument list */
    Tcl_ObjCmdProc *objProc,       /* handling procedure for part */
    ClientData clientData,         /* client data associated with part */
    Tcl_CmdDeleteProc *deleteProc, /* procedure used to destroy client data */
    int flags,
    EnsemblePart **rVal)           /* returns: new ensemble part */
{
    Tcl_Obj *mapDict;
    Tcl_Command cmd;
    EnsemblePart *ensPart;

    /*
     *  Install the new part into the part list.
     */
    if (CreateEnsemblePart(interp, ensData, partName, &ensPart) != TCL_OK) {
        return TCL_ERROR;
    }

    if (usageInfo) {
        ensPart->usage = (char *)ckalloc(strlen(usageInfo)+1);
        strcpy(ensPart->usage, usageInfo);
    }
    ensPart->objProc = objProc;
    ensPart->clientData = clientData;
    ensPart->deleteProc = deleteProc;
    ensPart->flags = flags;

    mapDict = NULL;
    Tcl_GetEnsembleMappingDict(NULL, ensData->cmdPtr, &mapDict);
    if (mapDict == NULL) {
        mapDict = Tcl_NewObj();
        ensPart->newMapDict = mapDict;
    }
    ensPart->mapNamePtr = Tcl_NewStringObj(ensData->nsPtr->fullName, -1);
    Tcl_AppendToObj(ensPart->mapNamePtr, "::", 2);
    Tcl_AppendToObj(ensPart->mapNamePtr, partName, -1);
    Tcl_IncrRefCount(ensPart->namePtr);
    Tcl_IncrRefCount(ensPart->mapNamePtr);
    Tcl_DictObjPut(NULL, mapDict, ensPart->namePtr, ensPart->mapNamePtr);
    cmd = Tcl_CreateObjCommand(interp, Tcl_GetString(ensPart->mapNamePtr),
            EnsembleSubCmd, ensPart, DeleteEnsemblePart);
    if (cmd == NULL) {
        Tcl_DictObjRemove(NULL, mapDict, ensPart->namePtr);
        Tcl_DecrRefCount(ensPart->namePtr);
        Tcl_DecrRefCount(ensPart->mapNamePtr);
        return TCL_ERROR;
    }
    Tcl_SetEnsembleMappingDict(interp, ensData->cmdPtr, mapDict);
    *rVal = ensPart;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteEnsemble --
 *
 *      Invoked when the command associated with an ensemble is
 *      destroyed, to delete the ensemble.  Destroys all parts
 *      included in the ensemble, and frees all memory associated
 *      with it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static void
DeleteEnsemble(
    ClientData clientData)    /* ensemble data */
{
    FOREACH_HASH_DECLS;
    ItclObjectInfo *infoPtr;
    Ensemble* ensData;
    Ensemble* ensData2;

    ensData = (Ensemble*)clientData;
    /* remove the unknown handler if set to release the Tcl_Obj of the name */
    if (Tcl_FindCommand(ensData->interp, Tcl_GetString(ensData->namePtr),
            NULL, 0) != NULL) {
        Tcl_SetEnsembleUnknownHandler(NULL, ensData->cmdPtr, NULL);
    }
    /*
     *  BE CAREFUL:  Each ensemble part removes itself from the list.
     *    So keep deleting the first part until all parts are gone.
     */
    while (ensData->numParts > 0) {
        DeleteEnsemblePart(ensData->parts[0]);
    }
    Tcl_DecrRefCount(ensData->namePtr);
    ckfree((char*)ensData->parts);
    ensData->parts = NULL;
    ensData->numParts = 0;
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(ensData->interp, ITCL_INTERP_DATA, NULL);
    FOREACH_HASH_VALUE(ensData2, &infoPtr->ensembleInfo->ensembles) {
        if (ensData2 == ensData) {
            Tcl_DeleteHashEntry(hPtr);
	}
    }
    ckfree((char*)ensData);
}


/*
 *----------------------------------------------------------------------
 *
 * FindEnsemble --
 *
 *      Searches for an ensemble command and follows a path to
 *      sub-ensembles.
 *
 * Results:
 *      Returns TCL_OK if the ensemble was found, along with a
 *      pointer to the ensemble data in "ensDataPtr".  Returns
 *      TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *      If anything goes wrong, this procedure returns an error
 *      message as the result in the interpreter.
 *
 *----------------------------------------------------------------------
 */
static int
FindEnsemble(
    Tcl_Interp *interp,            /* interpreter containing the ensemble */
    const char **nameArgv,         /* path of names leading to ensemble */
    int nameArgc,                  /* number of strings in nameArgv */
    Ensemble** ensDataPtr)         /* returns: ensemble data */
{
    int i;
    Tcl_Command cmdPtr;
    Ensemble *ensData;
    EnsemblePart *ensPart;
    Tcl_Obj *objPtr;
    Tcl_CmdInfo cmdInfo;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;

    *ensDataPtr = NULL;  /* assume that no data will be found */

    /*
     *  If there are no names in the path, then return an error.
     */
    if (nameArgc < 1) {
        Tcl_AppendToObj(Tcl_GetObjResult(interp),
            "invalid ensemble name \"\"", -1);
        return TCL_ERROR;
    }

    /*
     *  Use the first name to find the command for the top-level
     *  ensemble.
     */
    objPtr = Tcl_NewStringObj(nameArgv[0], -1);
    cmdPtr = Tcl_FindEnsemble(interp, objPtr, 0);
    Tcl_DecrRefCount(objPtr);

    if (cmdPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "command \"", nameArgv[0], "\" is not an ensemble",
            NULL);
        return TCL_ERROR;
    }
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->ensembleInfo->ensembles, (char *)cmdPtr);
    if (hPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "command \"", nameArgv[0], "\" is not an ensemble",
            NULL);
        return TCL_ERROR;
    }
    ensData = (Ensemble *)Tcl_GetHashValue(hPtr);

    /*
     *  Follow the trail of sub-ensemble names.
     */
    for (i=1; i < nameArgc; i++) {
        if (FindEnsemblePart(interp, ensData, nameArgv[i], &ensPart)
            != TCL_OK) {
            return TCL_ERROR;
        }
        if (ensPart == NULL) {
            char *pname = Tcl_Merge(i, nameArgv);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "invalid ensemble name \"", pname, "\"",
                NULL);
            ckfree(pname);
            return TCL_ERROR;
        }

        cmdPtr = ensPart->cmdPtr;
        if (cmdPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "part \"", nameArgv[i], "\" is not an ensemble",
                NULL);
            return TCL_ERROR;
        }
	if (!Tcl_IsEnsemble(cmdPtr)) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "part \"", nameArgv[i], "\" is not an ensemble",
                NULL);
            return TCL_ERROR;
	}
        if (Tcl_GetCommandInfoFromToken(cmdPtr, &cmdInfo) != 1) {
            return TCL_ERROR;
        }
        ensData = (Ensemble*)cmdInfo.objClientData;
    }
    *ensDataPtr = ensData;

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * CreateEnsemblePart --
 *
 *      Creates a new part within an ensemble.
 *
 * Results:
 *      If successful, this procedure returns TCL_OK, along with a
 *      pointer to the new part in "ensPartPtr".  If a part with the
 *      same name already exists, this procedure returns TCL_ERROR.
 *
 * Side effects:
 *      If anything goes wrong, this procedure returns an error
 *      message as the result in the interpreter.
 *
 *----------------------------------------------------------------------
 */
static int
CreateEnsemblePart(
    Tcl_Interp *interp,          /* interpreter containing the ensemble */
    Ensemble *ensData,           /* ensemble being modified */
    const char* partName,        /* name of the new part */
    EnsemblePart **ensPartPtr)   /* returns: new ensemble part */
{
    int i;
    int pos;
    int size;
    EnsemblePart** partList;
    EnsemblePart* ensPart;

    /*
     *  If a matching entry was found, then return an error.
     */
    if (FindEnsemblePartIndex(ensData, partName, &pos)) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "part \"", partName, "\" already exists in ensemble",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  Otherwise, make room for a new entry.  Keep the parts in
     *  lexicographical order, so we can search them quickly
     *  later.
     */
    if (ensData->numParts >= ensData->maxParts) {
        size = ensData->maxParts*sizeof(EnsemblePart*);
        partList = (EnsemblePart**)ckalloc((unsigned)2*size);
        memcpy(partList, ensData->parts, (size_t)size);
        ckfree((char*)ensData->parts);

        ensData->parts = partList;
        ensData->maxParts *= 2;
    }

    for (i=ensData->numParts; i > pos; i--) {
        ensData->parts[i] = ensData->parts[i-1];
    }
    ensData->numParts++;

    ensPart = (EnsemblePart*)ckalloc(sizeof(EnsemblePart));
    memset(ensPart, 0, sizeof(EnsemblePart));
    ensPart->name = (char*)ckalloc(strlen(partName)+1);
    strcpy(ensPart->name, partName);
    ensPart->namePtr = Tcl_NewStringObj(ensPart->name, -1);
    ensPart->ensemble = ensData;
    ensPart->interp = interp;

    ensData->parts[pos] = ensPart;

    /*
     *  Compare the new part against the one on either side of
     *  it.  Determine how many letters are needed in each part
     *  to guarantee that an abbreviated form is unique.  Update
     *  the parts on either side as well, since they are influenced
     *  by the new part.
     */
    ComputeMinChars(ensData, pos);
    ComputeMinChars(ensData, pos-1);
    ComputeMinChars(ensData, pos+1);

    *ensPartPtr = ensPart;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteEnsemblePart --
 *
 *      Deletes a single part from an ensemble.  The part must have
 *      been created previously by CreateEnsemblePart.
 *
 *      If the part has a delete proc, then it is called to free the
 *      associated client data.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Delete proc is called.
 *
 *----------------------------------------------------------------------
 */
static void
DeleteEnsemblePart(
    ClientData clientData)     /* part being destroyed */
{
    Tcl_Obj *mapDict;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    Ensemble *ensData;
    Ensemble *ensData2;
    EnsemblePart *ensPart;
    int i;
    int pos;

    mapDict = NULL;
    ensPart = (EnsemblePart *)clientData;
    if (ensPart == NULL) {
        return;
    }
    ensData = ensPart->ensemble;

    /*
     *  If this part has a delete proc, then call it to free
     *  up the client data.
     */
    if ((ensPart->deleteProc != NULL) && (ensPart->clientData != NULL)) {
        (*ensPart->deleteProc)(ensPart->clientData);
    }

    /* if it is a subensemble remove the command to free the data */
    if (ensPart->subEnsemblePtr != NULL) {
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(ensData->interp, ITCL_INTERP_DATA, NULL);
	hPtr = Tcl_FindHashEntry(&infoPtr->ensembleInfo->subEnsembles,
	        (char *)ensPart->subEnsemblePtr);
        if (hPtr != NULL) {
	    ensData2 = (Ensemble *)Tcl_GetHashValue(hPtr);
	    Tcl_DeleteNamespace(ensData2->nsPtr);
	    Tcl_DeleteHashEntry(hPtr);
	}
        Tcl_SetEnsembleUnknownHandler(NULL, ensPart->cmdPtr, NULL);
	hPtr = Tcl_FindHashEntry(&infoPtr->ensembleInfo->ensembles,
	        (char *)ensPart->ensemble->cmdPtr);
        if (hPtr != NULL) {
	    ensData2 = (Ensemble *)Tcl_GetHashValue(hPtr);
            Tcl_GetEnsembleMappingDict(NULL, ensData2->cmdPtr, &mapDict);
            if (mapDict != NULL) {
	        Tcl_DictObjRemove(ensPart->interp, mapDict,
	                ensPart->namePtr);
	        Tcl_SetEnsembleMappingDict(NULL, ensData2->cmdPtr, mapDict);
	    }
	}
	Tcl_DecrRefCount(ensPart->subEnsemblePtr);
	if (ensPart->newMapDict != NULL) {
	    Tcl_DecrRefCount(ensPart->newMapDict);
	}
    }
    /*
     *  Find this part within its ensemble, and remove it from
     *  the list of parts.
     */
    if (FindEnsemblePartIndex(ensPart->ensemble, ensPart->name, &pos)) {
        ensData = ensPart->ensemble;
        for (i=pos; i < ensData->numParts-1; i++) {
            ensData->parts[i] = ensData->parts[i+1];
        }
        ensData->numParts--;
    }

    /*
     *  Free the memory associated with the part.
     */
    mapDict = NULL;
    if (Tcl_FindCommand(ensData->interp, Tcl_GetString(ensData->namePtr),
            NULL, 0) != NULL) {
        Tcl_GetEnsembleMappingDict(ensData->interp, ensData->cmdPtr, &mapDict);
        if (mapDict != NULL) {
	    if (!Tcl_IsShared(mapDict)) {
	        Tcl_DictObjRemove(ensPart->interp, mapDict, ensPart->namePtr);
                Tcl_SetEnsembleMappingDict(ensPart->interp, ensData->cmdPtr,
	                mapDict);
            }
        }
    }
    /* this is the map !!! */
    if (ensPart->mapNamePtr != NULL) {
        Tcl_DecrRefCount(ensPart->mapNamePtr);
    }
    Tcl_DecrRefCount(ensPart->namePtr);
    if (ensPart->usage != NULL) {
        ckfree(ensPart->usage);
    }
    ckfree(ensPart->name);
    ckfree((char*)ensPart);
}


/*
 *----------------------------------------------------------------------
 *
 * FindEnsemblePart --
 *
 *      Searches for a part name within an ensemble.  Recognizes
 *      unique abbreviations for part names.
 *
 * Results:
 *      If the part name is not a unique abbreviation, this procedure
 *      returns TCL_ERROR.  Otherwise, it returns TCL_OK.  If the
 *      part can be found, "rensPart" returns a pointer to the part.
 *      Otherwise, it returns NULL.
 *
 * Side effects:
 *      If anything goes wrong, this procedure returns an error
 *      message as the result in the interpreter.
 *
 *----------------------------------------------------------------------
 */
static int
FindEnsemblePart(
    Tcl_Interp *interp,       /* interpreter containing the ensemble */
    Ensemble *ensData,        /* ensemble being searched */
    const char* partName,     /* name of the desired part */
    EnsemblePart **rensPart)  /* returns:  pointer to the desired part */
{
    int pos = 0;
    int first, last, nlen;
    int i, cmp;

    *rensPart = NULL;

    /*
     *  Search for the desired part name.
     *  All parts are in lexicographical order, so use a
     *  binary search to find the part quickly.  Match only
     *  as many characters as are included in the specified
     *  part name.
     */
    first = 0;
    last  = ensData->numParts-1;
    nlen  = strlen(partName);

    while (last >= first) {
        pos = (first+last)/2;
        if (*partName == *ensData->parts[pos]->name) {
            cmp = strncmp(partName, ensData->parts[pos]->name, nlen);
            if (cmp == 0) {
                break;    /* found it! */
            }
        }
        else if (*partName < *ensData->parts[pos]->name) {
            cmp = -1;
        }
        else {
            cmp = 1;
        }

        if (cmp > 0) {
            first = pos+1;
        } else {
            last = pos-1;
        }
    }

    /*
     *  If a matching entry could not be found, then quit.
     */
    if (last < first) {
        return TCL_OK;
    }

    /*
     *  If a matching entry was found, there may be some ambiguity
     *  if the user did not specify enough characters.  Find the
     *  top-most match in the list, and see if the part name has
     *  enough characters.  If there are two parts like "foo"
     *  and "food", this allows us to match "foo" exactly.
     */
    if (nlen < ensData->parts[pos]->minChars) {
        while (pos > 0) {
            pos--;
            if (strncmp(partName, ensData->parts[pos]->name, nlen) != 0) {
                pos++;
                break;
            }
        }
    }
    if (nlen < ensData->parts[pos]->minChars) {
        Tcl_Obj *resultPtr = Tcl_NewStringObj(NULL, 0);

        Tcl_AppendStringsToObj(resultPtr,
            "ambiguous option \"", partName, "\": should be one of...",
            NULL);

        for (i=pos; i < ensData->numParts; i++) {
            if (strncmp(partName, ensData->parts[i]->name, nlen) != 0) {
                break;
            }
            Tcl_AppendToObj(resultPtr, "\n  ", 3);
            GetEnsemblePartUsage(interp, ensData, ensData->parts[i], resultPtr);
        }
        Tcl_SetObjResult(interp, resultPtr);
        return TCL_ERROR;
    }

    /*
     *  Found a match.  Return the desired part.
     */
    *rensPart = ensData->parts[pos];
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * FindEnsemblePartIndex --
 *
 *      Searches for a part name within an ensemble.  The part name
 *      must be an exact match for an existing part name in the
 *      ensemble.  This procedure is useful for managing (i.e.,
 *      creating and deleting) parts in an ensemble.
 *
 * Results:
 *      If an exact match is found, this procedure returns
 *      non-zero, along with the index of the part in posPtr.
 *      Otherwise, it returns zero, along with an index in posPtr
 *      indicating where the part should be.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static int
FindEnsemblePartIndex(
    Ensemble *ensData,        /* ensemble being searched */
    const char *partName,     /* name of desired part */
    int *posPtr)              /* returns: index for part */
{
    int pos = 0;
    int first, last;
    int cmp;

    /*
     *  Search for the desired part name.
     *  All parts are in lexicographical order, so use a
     *  binary search to find the part quickly.
     */
    first = 0;
    last  = ensData->numParts-1;

    while (last >= first) {
        pos = (first+last)/2;
        if (*partName == *ensData->parts[pos]->name) {
            cmp = strcmp(partName, ensData->parts[pos]->name);
            if (cmp == 0) {
                break;    /* found it! */
            }
        }
        else if (*partName < *ensData->parts[pos]->name) {
            cmp = -1;
        }
        else {
            cmp = 1;
        }

        if (cmp > 0) {
            first = pos+1;
        } else {
            last = pos-1;
        }
    }

    if (last >= first) {
        *posPtr = pos;
        return 1;
    }
    *posPtr = first;
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * ComputeMinChars --
 *
 *      Compares part names on an ensemble's part list and
 *      determines the minimum number of characters needed for a
 *      unique abbreviation.  The parts on either side of a
 *      particular part index are compared.  As long as there is
 *      a part on one side or the other, this procedure updates
 *      the parts to have the proper minimum abbreviations.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Updates three parts within the ensemble to remember
 *      the minimum abbreviations.
 *
 *----------------------------------------------------------------------
 */
static void
ComputeMinChars(
    Ensemble *ensData,        /* ensemble being modified */
    int pos)                  /* index of part being updated */
{
    int min, max;
    char *p, *q;

    /*
     *  If the position is invalid, do nothing.
     */
    if (pos < 0 || pos >= ensData->numParts) {
        return;
    }

    /*
     *  Start by assuming that only the first letter is required
     *  to uniquely identify this part.  Then compare the name
     *  against each neighboring part to determine the real minimum.
     */
    ensData->parts[pos]->minChars = 1;

    if (pos-1 >= 0) {
        p = ensData->parts[pos]->name;
        q = ensData->parts[pos-1]->name;
        for (min=1; *p == *q && *p != '\0' && *q != '\0'; min++) {
            p++;
            q++;
        }
        if (min > ensData->parts[pos]->minChars) {
            ensData->parts[pos]->minChars = min;
        }
    }

    if (pos+1 < ensData->numParts) {
        p = ensData->parts[pos]->name;
        q = ensData->parts[pos+1]->name;
        for (min=1; *p == *q && *p != '\0' && *q != '\0'; min++) {
            p++;
            q++;
        }
        if (min > ensData->parts[pos]->minChars) {
            ensData->parts[pos]->minChars = min;
        }
    }

    max = strlen(ensData->parts[pos]->name);
    if (ensData->parts[pos]->minChars > max) {
        ensData->parts[pos]->minChars = max;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_EnsembleCmd --
 *
 *      Invoked by Tcl whenever the user issues the "ensemble"
 *      command to manipulate an ensemble.  Handles the following
 *      syntax:
 *
 *        ensemble <ensName> ?<command> <arg> <arg>...?
 *        ensemble <ensName> {
 *            part <partName> <args> <body>
 *            ensemble <ensName> {
 *                ...
 *            }
 *        }
 *
 *      Finds or creates the ensemble <ensName>, and then executes
 *      the commands to add parts.
 *
 * Results:
 *      Returns TCL_OK if successful, and TCL_ERROR if anything
 *      goes wrong.
 *
 * Side effects:
 *      If anything goes wrong, this procedure returns an error
 *      message as the result in the interpreter.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_EnsembleCmd(
    ClientData clientData,   /* ensemble data */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int status;
    char *ensName;
    EnsembleParser *ensInfo;
    Ensemble *ensData;
    Ensemble *savedEnsData;
    EnsemblePart *ensPart;
    Tcl_Command cmd;
    Tcl_Obj *objPtr;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;

    ItclShowArgs(1, "Itcl_EnsembleCmd", objc, objv);
    /*
     *  Make sure that an ensemble name was specified.
     */
    if (objc < 2) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"",
            Tcl_GetString(objv[0]),
            " name ?command arg arg...?\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If this is the "ensemble" command in the main interpreter,
     *  then the client data will be null.  Otherwise, it is
     *  the "ensemble" command in the ensemble body parser, and
     *  the client data indicates which ensemble we are modifying.
     */
    if (clientData) {
        ensInfo = (EnsembleParser*)clientData;
    } else {
        ensInfo = GetEnsembleParser(interp);
    }
    ensData = ensInfo->ensData;

    /*
     *  Find or create the desired ensemble.  If an ensemble is
     *  being built, then this "ensemble" command is enclosed in
     *  another "ensemble" command.  Use the current ensemble as
     *  the parent, and find or create an ensemble part within it.
     */
    ensName = Tcl_GetString(objv[1]);

    if (ensData) {
        if (FindEnsemblePart(ensInfo->master, ensData, ensName, &ensPart) != TCL_OK) {
            ensPart = NULL;
        }
        if (ensPart == NULL) {
            if (CreateEnsemble(ensInfo->master, ensData, ensName) != TCL_OK) {
		Tcl_TransferResult(ensInfo->master, TCL_ERROR, interp);
                return TCL_ERROR;
            }
            if (FindEnsemblePart(ensInfo->master, ensData, ensName, &ensPart)
                    != TCL_OK) {
                Tcl_Panic("Itcl_EnsembleCmd: can't create ensemble");
            }
        }

        cmd = ensPart->cmdPtr;
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(ensInfo->master, ITCL_INTERP_DATA, NULL);
        hPtr = Tcl_FindHashEntry(&infoPtr->ensembleInfo->ensembles,
	        (char *)ensPart->cmdPtr);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "part \"", Tcl_GetString(objv[1]),
                "\" is not an ensemble",
                NULL);
            return TCL_ERROR;
	}
        ensData = (Ensemble *)Tcl_GetHashValue(hPtr);
    } else {

        /*
         *  Otherwise, the desired ensemble is a top-level ensemble.
         *  Find or create the access command for the ensemble, and
         *  then get its data.
         */
        cmd = Tcl_FindCommand(interp, ensName, NULL, 0);
        if (cmd == NULL) {
            if (CreateEnsemble(interp, NULL, ensName)
                != TCL_OK) {
                return TCL_ERROR;
            }
            cmd = Tcl_FindCommand(interp, ensName, NULL, 0);
        }

        if (cmd == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "command \"", Tcl_GetString(objv[1]),
                "\" is not an ensemble",
                NULL);
            return TCL_ERROR;
        }
        infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
        hPtr = Tcl_FindHashEntry(&infoPtr->ensembleInfo->ensembles, (char *)cmd);
        if (hPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "command \"", Tcl_GetString(objv[1]),
                "\" is not an ensemble",
                NULL);
            return TCL_ERROR;
        }
        ensData = (Ensemble *)Tcl_GetHashValue(hPtr);
    }

    /*
     *  At this point, we have the data for the ensemble that is
     *  being manipulated.  Plug this into the parser, and then
     *  interpret the rest of the arguments in the ensemble parser.
     */
    status = TCL_OK;
    savedEnsData = ensInfo->ensData;
    ensInfo->ensData = ensData;

    if (objc == 3) {
        status = Tcl_EvalObjEx(ensInfo->parser, objv[2], 0);
    } else {
        if (objc > 3) {
            objPtr = Tcl_NewListObj(objc-2, objv+2);
            Tcl_IncrRefCount(objPtr);  /* stop Eval trashing it */
            status = Tcl_EvalObjEx(ensInfo->parser, objPtr, 0);
            Tcl_DecrRefCount(objPtr);  /* we're done with the object */
        }
    }

    /*
     *  Copy the result from the parser interpreter to the
     *  master interpreter.  If an error was encountered,
     *  copy the error info first, and then set the result.
     *  Otherwise, the offending command is reported twice.
     */
    if (status == TCL_ERROR) {
	/* no longer needed, no extra interpreter !! */
        const char *errInfo = Tcl_GetVar2(ensInfo->parser, "::errorInfo",
            NULL, TCL_GLOBAL_ONLY);

        if (errInfo) {
        	Tcl_AppendObjToErrorInfo(interp, Tcl_NewStringObj(errInfo, -1));
        }

        if (objc == 3) {
	    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
	            "\n    (\"ensemble\" body line %d)",
		    Tcl_GetErrorLine(ensInfo->parser)));
        }
    }
    Tcl_SetObjResult(interp, Tcl_GetObjResult(ensInfo->parser));

    ensInfo->ensData = savedEnsData;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * GetEnsembleParser --
 *
 *      Returns the slave interpreter that acts as a parser for
 *      the body of an "ensemble" definition.  The first time that
 *      this is called for an interpreter, the parser is created
 *      and registered as associated data.  After that, it is
 *      simply returned.
 *
 * Results:
 *      Returns a pointer to the ensemble parser data structure.
 *
 * Side effects:
 *      On the first call, the ensemble parser is created and
 *      registered as "itcl_ensembleParser" with the interpreter.
 *
 *----------------------------------------------------------------------
 */
static EnsembleParser*
GetEnsembleParser(
    Tcl_Interp *interp)     /* interpreter handling the ensemble */
{
    EnsembleParser *ensInfo;

    /*
     *  Look for an existing ensemble parser.  If it is found,
     *  return it immediately.
     */
    ensInfo = (EnsembleParser*) Tcl_GetAssocData(interp,
        "itcl_ensembleParser", NULL);

    if (ensInfo) {
        return ensInfo;
    }

    /*
     *  Create a slave interpreter that can be used to parse
     *  the body of an ensemble definition.
     */
    ensInfo = (EnsembleParser*)ckalloc(sizeof(EnsembleParser));
    ensInfo->master = interp;
    ensInfo->parser = Tcl_CreateInterp();
    ensInfo->ensData = NULL;

    Tcl_DeleteNamespace(Tcl_GetGlobalNamespace(ensInfo->parser));
    /*
     *  Add the allowed commands to the parser interpreter:
     *  part, delete, ensemble
     */
    Tcl_CreateObjCommand(ensInfo->parser, "part", Itcl_EnsPartCmd,
        ensInfo, NULL);

    Tcl_CreateObjCommand(ensInfo->parser, "option", Itcl_EnsPartCmd,
        ensInfo, NULL);

    Tcl_CreateObjCommand(ensInfo->parser, "ensemble", Itcl_EnsembleCmd,
        ensInfo, NULL);

    /*
     *  Install the parser data, so we'll have it the next time
     *  we call this procedure.
     */
    (void) Tcl_SetAssocData(interp, "itcl_ensembleParser",
            DeleteEnsParser, ensInfo);

    return ensInfo;
}


/*
 *----------------------------------------------------------------------
 *
 * DeleteEnsParser --
 *
 *      Called when an interpreter is destroyed to clean up the
 *      ensemble parser within it.  Destroys the slave interpreter
 *      and frees up the data associated with it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */
static void
DeleteEnsParser(
    ClientData clientData,    /* client data for ensemble-related commands */
    Tcl_Interp *interp)       /* interpreter containing the data */
{
    EnsembleParser* ensInfo = (EnsembleParser*)clientData;
    Tcl_DeleteInterp(ensInfo->parser);
    ckfree((char*)ensInfo);
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_EnsPartCmd --
 *
 *      Invoked by Tcl whenever the user issues the "part" command
 *      to manipulate an ensemble.  This command can only be used
 *      inside the "ensemble" command, which handles ensembles.
 *      Handles the following syntax:
 *
 *        ensemble <ensName> {
 *            part <partName> <args> <body>
 *        }
 *
 *      Adds a new part called <partName> to the ensemble.  If a
 *      part already exists with that name, it is an error.  The
 *      new part is handled just like an ordinary Tcl proc, with
 *      a list of <args> and a <body> of code to execute.
 *
 * Results:
 *      Returns TCL_OK if successful, and TCL_ERROR if anything
 *      goes wrong.
 *
 * Side effects:
 *      If anything goes wrong, this procedure returns an error
 *      message as the result in the interpreter.
 *
 *----------------------------------------------------------------------
 */
int
Itcl_EnsPartCmd(
    ClientData clientData,   /* ensemble data */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *usagePtr;
    Tcl_Proc procPtr;
    EnsembleParser *ensInfo = (EnsembleParser*)clientData;
    Ensemble *ensData = (Ensemble*)ensInfo->ensData;
    EnsemblePart *ensPart;
    ItclArgList *arglistPtr;
    char *partName;
    char *usage;
    int result;
    int argc;
    int maxArgc;
    Tcl_CmdInfo cmdInfo;

    ItclShowArgs(1, "Itcl_EnsPartCmd", objc, objv);
    if (objc != 4) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"",
            Tcl_GetString(objv[0]),
            " name args body\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  Create a Tcl-style proc definition using the specified args
     *  and body.  This is not a proc in the usual sense.  It belongs
     *  to the namespace that contains the ensemble, but it is
     *  accessed through the ensemble, not through a Tcl command.
     */
    partName = Tcl_GetString(objv[1]);

    if (ItclCreateArgList(interp, Tcl_GetString(objv[2]), &argc, &maxArgc,
            &usagePtr, &arglistPtr, NULL, partName) != TCL_OK) {
	result = TCL_ERROR;
	goto errorOut;
    }
    if (Tcl_GetCommandInfoFromToken(ensData->cmdPtr, &cmdInfo) != 1) {
	result = TCL_ERROR;
	goto errorOut;
    }
    if (Tcl_CreateProc(ensInfo->master, cmdInfo.namespacePtr, partName, objv[2], objv[3],
            &procPtr) != TCL_OK) {
	Tcl_TransferResult(ensInfo->master, TCL_ERROR, interp);
	result = TCL_ERROR;
	goto errorOut;
    }

    usage = Tcl_GetString(usagePtr);

    /*
     *  Create a new part within the ensemble.  If successful,
     *  plug the command token into the proc; we'll need it later
     *  if we try to compile the Tcl code for the part.  If
     *  anything goes wrong, clean up before bailing out.
     */
    result = AddEnsemblePart(ensInfo->master, ensData, partName, usage,
        (Tcl_ObjCmdProc *)Tcl_GetObjInterpProc(), procPtr, _Tcl_ProcDeleteProc,
        ITCL_ENSEMBLE_ENSEMBLE, &ensPart);
    if (result == TCL_ERROR) {
	_Tcl_ProcDeleteProc(procPtr);
    }
    Tcl_TransferResult(ensInfo->master, result, interp);

errorOut:
    Tcl_DecrRefCount(usagePtr);
    ItclDeleteArgList(arglistPtr);
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * Itcl_EnsembleErrorCmd --
 *
 *      Invoked when the user tries to access an unknown part for
 *      an ensemble.  Acts as the default handler for the "@error"
 *      part.  Generates an error message like:
 *
 *          bad option "foo": should be one of...
 *            info args procname
 *            info body procname
 *            info cmdcount
 *            ...
 *
 * Results:
 *      Always returns TCL_OK.
 *
 * Side effects:
 *      Returns the error message as the result in the interpreter.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */
int
Itcl_EnsembleErrorCmd(
    ClientData clientData,   /* ensemble info */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Ensemble *ensData = (Ensemble*)clientData;

    char *cmdName;
    Tcl_Obj *objPtr;

    cmdName = Tcl_GetString(objv[0]);

    objPtr = Tcl_NewStringObj(NULL, 0);
    Tcl_AppendStringsToObj(objPtr,
        "bad option \"", cmdName, "\": should be one of...\n",
        NULL);
    GetEnsembleUsage(interp, ensData, objPtr);

    Tcl_SetObjResult(interp, objPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * EnsembleSubCmd --
 *
 *----------------------------------------------------------------------
 */

static int
CallInvokeEnsembleMethod(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Namespace *nsPtr = (Tcl_Namespace *)data[0];
    EnsemblePart *ensPart = (EnsemblePart *)data[1];
    int objc = PTR2INT(data[2]);
    Tcl_Obj *const *objv = (Tcl_Obj *const *)data[3];

    result = Itcl_InvokeEnsembleMethod(interp, nsPtr, ensPart->namePtr,
	        (Tcl_Proc *)ensPart->clientData, objc, objv);
    return result;
}

static int
CallInvokeEnsembleMethod2(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    EnsemblePart *ensPart = (EnsemblePart *)data[0];
    int objc = PTR2INT(data[1]);
    Tcl_Obj *const*objv = (Tcl_Obj *const*)data[2];
    result = (*ensPart->objProc)(ensPart->clientData, interp, objc, objv);
    return result;
}

static int
EnsembleSubCmd(
    ClientData clientData,      /* ensPart struct pointer */
    Tcl_Interp *interp,         /* Current interpreter. */
    int objc,                   /* Number of arguments. */
    Tcl_Obj *const objv[])      /* Argument objects. */
{
    int result;
    Tcl_Namespace *nsPtr;
    EnsemblePart *ensPart;
    void *callbackPtr;

    ItclShowArgs(1, "EnsembleSubCmd", objc, objv);
    result = TCL_OK;
    ensPart = (EnsemblePart *)clientData;
    nsPtr = Tcl_GetCurrentNamespace(interp);
    callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
    if (ensPart->flags & ITCL_ENSEMBLE_ENSEMBLE) {
	/* FIXME !!! */
	if (ensPart->clientData == NULL) {
	    return TCL_ERROR;
	}
	Tcl_NRAddCallback(interp, CallInvokeEnsembleMethod, nsPtr, ensPart, INT2PTR(objc), (void *)objv);
    } else {
	Tcl_NRAddCallback(interp, CallInvokeEnsembleMethod2, ensPart, INT2PTR(objc), (void *)objv, NULL);
    }
    result = Itcl_NRRunCallbacks(interp, callbackPtr);
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  EnsembleUnknownCmd()
 *
 *  the unknown handler for the ensemble commands
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
EnsembleUnknownCmd(
    ClientData dummy,        /* not used */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Command cmd;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    EnsemblePart *ensPart;
    Ensemble *ensData;

    ItclShowArgs(2, "EnsembleUnknownCmd", objc, objv);
    cmd = Tcl_GetCommandFromObj(interp, objv[1]);
    if (cmd == NULL) {
        Tcl_AppendResult(interp, "EnsembleUnknownCmd, ensemble not found!",
	        Tcl_GetString(objv[1]), NULL);
        return TCL_ERROR;
    }
    infoPtr = (ItclObjectInfo *)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    hPtr = Tcl_FindHashEntry(&infoPtr->ensembleInfo->ensembles, (char *)cmd);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "EnsembleUnknownCmd, ensemble struct not ",
	        "found!", Tcl_GetString(objv[1]), NULL);
        return TCL_ERROR;
    }
    ensData = (Ensemble *)Tcl_GetHashValue(hPtr);
    if (objc < 3) {
        /* produce usage message */
        Tcl_Obj *objPtr = Tcl_NewStringObj(
                "wrong # args: should be one of...\n", -1);
        GetEnsembleUsage(interp, ensData, objPtr);
        Tcl_SetObjResult(interp, objPtr);
        return TCL_ERROR;
    }
    if (FindEnsemblePart(interp, ensData, "@error", &ensPart) != TCL_OK) {
        Tcl_AppendResult(interp, "FindEnsemblePart error", NULL);
        return TCL_ERROR;
    }
    if (ensPart != NULL) {
        Tcl_Obj *listPtr;

	listPtr = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(NULL, listPtr, objv[1]);
	Tcl_ListObjAppendElement(NULL, listPtr, Tcl_NewStringObj("@error", -1));
	Tcl_ListObjAppendElement(NULL, listPtr, objv[2]);
	Tcl_SetObjResult(interp, listPtr);
        return TCL_OK;
    }

    return Itcl_EnsembleErrorCmd(ensData, interp, objc-2, objv+2);
}

/*
 *----------------------------------------------------------------------
 *
 * Itcl_EnsembleDeleteCmd --
 *
 *      Invoked when the user tries to delet an ensemble
 *----------------------------------------------------------------------
 */
int
Itcl_EnsembleDeleteCmd(
    ClientData clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_Command cmdPtr;
    Ensemble *ensData;
    ItclObjectInfo *infoPtr;
    int i;

    infoPtr = (ItclObjectInfo *)clientData;
    ItclShowArgs(1, "Itcl_EnsembleDeleteCmd", objc, objv);
    for (i = 1; i < objc; i++) {
        cmdPtr = Tcl_FindCommand(interp, Tcl_GetString(objv[i]), NULL, 0);
        if (cmdPtr == NULL) {
            Tcl_AppendResult(interp, "no such ensemble \"",
	    Tcl_GetString(objv[i]), "\"", NULL);
            return TCL_ERROR;
        }
        hPtr = Tcl_FindHashEntry(&infoPtr->ensembleInfo->ensembles, (char *)cmdPtr);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "no such ensemble \"",
	    Tcl_GetString(objv[i]), "\"", NULL);
            return TCL_ERROR;
        }
        ensData = (Ensemble *)Tcl_GetHashValue(hPtr);
        Itcl_RenameCommand(ensData->interp, Tcl_GetString(ensData->namePtr), "");
	if (Tcl_FindNamespace(interp, ensData->nsPtr->fullName, NULL, 0)
	        != NULL) {
	    Tcl_DeleteNamespace(ensData->nsPtr);
        }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Itcl_FinishEnsemble --
 *
 *      Invoked when itcl package is finished or ItclFinishCmd is called
 *----------------------------------------------------------------------
 */
void
ItclFinishEnsemble(
    ItclObjectInfo *infoPtr)
{
    Tcl_DeleteAssocData(infoPtr->interp, "itcl_ensembleParser");
}
