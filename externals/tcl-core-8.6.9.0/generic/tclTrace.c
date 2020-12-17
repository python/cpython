/*
 * tclTrace.c --
 *
 *	This file contains code to handle most trace management.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Scriptics Corporation.
 * Copyright (c) 2002 ActiveState Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Structures used to hold information about variable traces:
 */

typedef struct {
    int flags;			/* Operations for which Tcl command is to be
				 * invoked. */
    size_t length;		/* Number of non-NUL chars. in command. */
    char command[1];		/* Space for Tcl command to invoke. Actual
				 * size will be as large as necessary to hold
				 * command. This field must be the last in the
				 * structure, so that it can be larger than 1
				 * byte. */
} TraceVarInfo;

typedef struct {
    VarTrace traceInfo;
    TraceVarInfo traceCmdInfo;
} CombinedTraceVarInfo;

/*
 * Structure used to hold information about command traces:
 */

typedef struct {
    int flags;			/* Operations for which Tcl command is to be
				 * invoked. */
    size_t length;		/* Number of non-NUL chars. in command. */
    Tcl_Trace stepTrace;	/* Used for execution traces, when tracing
				 * inside the given command */
    int startLevel;		/* Used for bookkeeping with step execution
				 * traces, store the level at which the step
				 * trace was invoked */
    char *startCmd;		/* Used for bookkeeping with step execution
				 * traces, store the command name which
				 * invoked step trace */
    int curFlags;		/* Trace flags for the current command */
    int curCode;		/* Return code for the current command */
    int refCount;		/* Used to ensure this structure is not
				 * deleted too early. Keeps track of how many
				 * pieces of code have a pointer to this
				 * structure. */
    char command[1];		/* Space for Tcl command to invoke. Actual
				 * size will be as large as necessary to hold
				 * command. This field must be the last in the
				 * structure, so that it can be larger than 1
				 * byte. */
} TraceCommandInfo;

/*
 * Used by command execution traces. Note that we assume in the code that
 * TCL_TRACE_ENTER_DURING_EXEC == 4 * TCL_TRACE_ENTER_EXEC and that
 * TCL_TRACE_LEAVE_DURING_EXEC == 4 * TCL_TRACE_LEAVE_EXEC.
 *
 * TCL_TRACE_ENTER_DURING_EXEC  - Trace each command inside the command
 *				  currently being traced, before execution.
 * TCL_TRACE_LEAVE_DURING_EXEC  - Trace each command inside the command
 *				  currently being traced, after execution.
 * TCL_TRACE_ANY_EXEC		- OR'd combination of all EXEC flags.
 * TCL_TRACE_EXEC_IN_PROGRESS   - The callback function on this trace is
 *				  currently executing. Therefore we don't let
 *				  further traces execute.
 * TCL_TRACE_EXEC_DIRECT	- This execution trace is triggered directly
 *				  by the command being traced, not because of
 *				  an internal trace.
 * The flags 'TCL_TRACE_DESTROYED' and 'TCL_INTERP_DESTROYED' may also be used
 * in command execution traces.
 */

#define TCL_TRACE_ENTER_DURING_EXEC	4
#define TCL_TRACE_LEAVE_DURING_EXEC	8
#define TCL_TRACE_ANY_EXEC		15
#define TCL_TRACE_EXEC_IN_PROGRESS	0x10
#define TCL_TRACE_EXEC_DIRECT		0x20

/*
 * Forward declarations for functions defined in this file:
 */

typedef int (Tcl_TraceTypeObjCmd)(Tcl_Interp *interp, int optionIndex,
	int objc, Tcl_Obj *const objv[]);

static Tcl_TraceTypeObjCmd TraceVariableObjCmd;
static Tcl_TraceTypeObjCmd TraceCommandObjCmd;
static Tcl_TraceTypeObjCmd TraceExecutionObjCmd;

/*
 * Each subcommand has a number of 'types' to which it can apply. Currently
 * 'execution', 'command' and 'variable' are the only types supported. These
 * three arrays MUST be kept in sync! In the future we may provide an API to
 * add to the list of supported trace types.
 */

static const char *const traceTypeOptions[] = {
    "execution", "command", "variable", NULL
};
static Tcl_TraceTypeObjCmd *const traceSubCmds[] = {
    TraceExecutionObjCmd,
    TraceCommandObjCmd,
    TraceVariableObjCmd
};

/*
 * Declarations for local functions to this file:
 */

static int		CallTraceFunction(Tcl_Interp *interp, Trace *tracePtr,
			    Command *cmdPtr, const char *command, int numChars,
			    int objc, Tcl_Obj *const objv[]);
static char *		TraceVarProc(ClientData clientData, Tcl_Interp *interp,
			    const char *name1, const char *name2, int flags);
static void		TraceCommandProc(ClientData clientData,
			    Tcl_Interp *interp, const char *oldName,
			    const char *newName, int flags);
static Tcl_CmdObjTraceProc TraceExecutionProc;
static int		StringTraceProc(ClientData clientData,
			    Tcl_Interp *interp, int level,
			    const char *command, Tcl_Command commandInfo,
			    int objc, Tcl_Obj *const objv[]);
static void		StringTraceDeleteProc(ClientData clientData);
static void		DisposeTraceResult(int flags, char *result);
static int		TraceVarEx(Tcl_Interp *interp, const char *part1,
			    const char *part2, register VarTrace *tracePtr);

/*
 * The following structure holds the client data for string-based
 * trace procs
 */

typedef struct StringTraceData {
    ClientData clientData;	/* Client data from Tcl_CreateTrace */
    Tcl_CmdTraceProc *proc;	/* Trace function from Tcl_CreateTrace */
} StringTraceData;

/*
 * Convenience macros for iterating over the list of traces. Note that each of
 * these *must* be treated as a command, and *must* have a block following it.
 */

#define FOREACH_VAR_TRACE(interp, name, clientData) \
    (clientData) = NULL; \
    while (((clientData) = Tcl_VarTraceInfo2((interp), (name), NULL, \
	    0, TraceVarProc, (clientData))) != NULL)

#define FOREACH_COMMAND_TRACE(interp, name, clientData) \
    (clientData) = NULL; \
    while ((clientData = Tcl_CommandTraceInfo(interp, name, 0, \
	    TraceCommandProc, clientData)) != NULL)

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceObjCmd --
 *
 *	This function is invoked to process the "trace" Tcl command. See the
 *	user documentation for details on what it does.
 *
 *	Standard syntax as of Tcl 8.4 is:
 *	    trace {add|info|remove} {command|variable} name ops cmd
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_TraceObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int optionIndex;
#ifndef TCL_REMOVE_OBSOLETE_TRACES
    const char *name;
    const char *flagOps, *p;
#endif
    /* Main sub commands to 'trace' */
    static const char *const traceOptions[] = {
	"add", "info", "remove",
#ifndef TCL_REMOVE_OBSOLETE_TRACES
	"variable", "vdelete", "vinfo",
#endif
	NULL
    };
    /* 'OLD' options are pre-Tcl-8.4 style */
    enum traceOptions {
	TRACE_ADD, TRACE_INFO, TRACE_REMOVE,
#ifndef TCL_REMOVE_OBSOLETE_TRACES
	TRACE_OLD_VARIABLE, TRACE_OLD_VDELETE, TRACE_OLD_VINFO
#endif
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], traceOptions, "option", 0,
	    &optionIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum traceOptions) optionIndex) {
    case TRACE_ADD:
    case TRACE_REMOVE: {
	/*
	 * All sub commands of trace add/remove must take at least one more
	 * argument. Beyond that we let the subcommand itself control the
	 * argument structure.
	 */

	int typeIndex;

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "type ?arg ...?");
	    return TCL_ERROR;
	}
	if (Tcl_GetIndexFromObj(interp, objv[2], traceTypeOptions, "option",
		0, &typeIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	return traceSubCmds[typeIndex](interp, optionIndex, objc, objv);
    }
    case TRACE_INFO: {
	/*
	 * All sub commands of trace info must take exactly two more arguments
	 * which name the type of thing being traced and the name of the thing
	 * being traced.
	 */

	int typeIndex;
	if (objc < 3) {
	    /*
	     * Delegate other complaints to the type-specific code which can
	     * give a better error message.
	     */

	    Tcl_WrongNumArgs(interp, 2, objv, "type name");
	    return TCL_ERROR;
	}
	if (Tcl_GetIndexFromObj(interp, objv[2], traceTypeOptions, "option",
		0, &typeIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	return traceSubCmds[typeIndex](interp, optionIndex, objc, objv);
	break;
    }

#ifndef TCL_REMOVE_OBSOLETE_TRACES
    case TRACE_OLD_VARIABLE:
    case TRACE_OLD_VDELETE: {
	Tcl_Obj *copyObjv[6];
	Tcl_Obj *opsList;
	int code, numFlags;

	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "name ops command");
	    return TCL_ERROR;
	}

	opsList = Tcl_NewObj();
	Tcl_IncrRefCount(opsList);
	flagOps = Tcl_GetStringFromObj(objv[3], &numFlags);
	if (numFlags == 0) {
	    Tcl_DecrRefCount(opsList);
	    goto badVarOps;
	}
	for (p = flagOps; *p != 0; p++) {
	    Tcl_Obj *opObj;

	    if (*p == 'r') {
		TclNewLiteralStringObj(opObj, "read");
	    } else if (*p == 'w') {
		TclNewLiteralStringObj(opObj, "write");
	    } else if (*p == 'u') {
		TclNewLiteralStringObj(opObj, "unset");
	    } else if (*p == 'a') {
		TclNewLiteralStringObj(opObj, "array");
	    } else {
		Tcl_DecrRefCount(opsList);
		goto badVarOps;
	    }
	    Tcl_ListObjAppendElement(NULL, opsList, opObj);
	}
	copyObjv[0] = NULL;
	memcpy(copyObjv+1, objv, objc*sizeof(Tcl_Obj *));
	copyObjv[4] = opsList;
	if (optionIndex == TRACE_OLD_VARIABLE) {
	    code = traceSubCmds[2](interp, TRACE_ADD, objc+1, copyObjv);
	} else {
	    code = traceSubCmds[2](interp, TRACE_REMOVE, objc+1, copyObjv);
	}
	Tcl_DecrRefCount(opsList);
	return code;
    }
    case TRACE_OLD_VINFO: {
	ClientData clientData;
	char ops[5];
	Tcl_Obj *resultListPtr, *pairObjPtr, *elemObjPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "name");
	    return TCL_ERROR;
	}
	resultListPtr = Tcl_NewObj();
	name = Tcl_GetString(objv[2]);
	FOREACH_VAR_TRACE(interp, name, clientData) {
	    TraceVarInfo *tvarPtr = clientData;
	    char *q = ops;

	    pairObjPtr = Tcl_NewListObj(0, NULL);
	    if (tvarPtr->flags & TCL_TRACE_READS) {
		*q = 'r';
		q++;
	    }
	    if (tvarPtr->flags & TCL_TRACE_WRITES) {
		*q = 'w';
		q++;
	    }
	    if (tvarPtr->flags & TCL_TRACE_UNSETS) {
		*q = 'u';
		q++;
	    }
	    if (tvarPtr->flags & TCL_TRACE_ARRAY) {
		*q = 'a';
		q++;
	    }
	    *q = '\0';

	    /*
	     * Build a pair (2-item list) with the ops string as the first obj
	     * element and the tvarPtr->command string as the second obj
	     * element. Append the pair (as an element) to the end of the
	     * result object list.
	     */

	    elemObjPtr = Tcl_NewStringObj(ops, -1);
	    Tcl_ListObjAppendElement(NULL, pairObjPtr, elemObjPtr);
	    elemObjPtr = Tcl_NewStringObj(tvarPtr->command, -1);
	    Tcl_ListObjAppendElement(NULL, pairObjPtr, elemObjPtr);
	    Tcl_ListObjAppendElement(interp, resultListPtr, pairObjPtr);
	}
	Tcl_SetObjResult(interp, resultListPtr);
	break;
    }
#endif /* TCL_REMOVE_OBSOLETE_TRACES */
    }
    return TCL_OK;

#ifndef TCL_REMOVE_OBSOLETE_TRACES
  badVarOps:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "bad operations \"%s\": should be one or more of rwua",
	    flagOps));
    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRACE", "BADOPS", NULL);
    return TCL_ERROR;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TraceExecutionObjCmd --
 *
 *	Helper function for Tcl_TraceObjCmd; implements the [trace
 *	{add|remove|info} execution ...] subcommands. See the user
 *	documentation for details on what these do.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Depends on the operation (add, remove, or info) being performed; may
 *	add or remove command traces on a command.
 *
 *----------------------------------------------------------------------
 */

static int
TraceExecutionObjCmd(
    Tcl_Interp *interp,		/* Current interpreter. */
    int optionIndex,		/* Add, info or remove */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int commandLength, index;
    const char *name, *command;
    size_t length;
    enum traceOptions {
	TRACE_ADD, TRACE_INFO, TRACE_REMOVE
    };
    static const char *const opStrings[] = {
	"enter", "leave", "enterstep", "leavestep", NULL
    };
    enum operations {
	TRACE_EXEC_ENTER, TRACE_EXEC_LEAVE,
	TRACE_EXEC_ENTER_STEP, TRACE_EXEC_LEAVE_STEP
    };

    switch ((enum traceOptions) optionIndex) {
    case TRACE_ADD:
    case TRACE_REMOVE: {
	int flags = 0;
	int i, listLen, result;
	Tcl_Obj **elemPtrs;

	if (objc != 6) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name opList command");
	    return TCL_ERROR;
	}

	/*
	 * Make sure the ops argument is a list object; get its length and a
	 * pointer to its array of element pointers.
	 */

	result = Tcl_ListObjGetElements(interp, objv[4], &listLen, &elemPtrs);
	if (result != TCL_OK) {
	    return result;
	}
	if (listLen == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "bad operation list \"\": must be one or more of"
		    " enter, leave, enterstep, or leavestep", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRACE", "NOOPS",
		    NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < listLen; i++) {
	    if (Tcl_GetIndexFromObj(interp, elemPtrs[i], opStrings,
		    "operation", TCL_EXACT, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum operations) index) {
	    case TRACE_EXEC_ENTER:
		flags |= TCL_TRACE_ENTER_EXEC;
		break;
	    case TRACE_EXEC_LEAVE:
		flags |= TCL_TRACE_LEAVE_EXEC;
		break;
	    case TRACE_EXEC_ENTER_STEP:
		flags |= TCL_TRACE_ENTER_DURING_EXEC;
		break;
	    case TRACE_EXEC_LEAVE_STEP:
		flags |= TCL_TRACE_LEAVE_DURING_EXEC;
		break;
	    }
	}
	command = Tcl_GetStringFromObj(objv[5], &commandLength);
	length = (size_t) commandLength;
	if ((enum traceOptions) optionIndex == TRACE_ADD) {
	    TraceCommandInfo *tcmdPtr = ckalloc(
		    TclOffset(TraceCommandInfo, command) + 1 + length);

	    tcmdPtr->flags = flags;
	    tcmdPtr->stepTrace = NULL;
	    tcmdPtr->startLevel = 0;
	    tcmdPtr->startCmd = NULL;
	    tcmdPtr->length = length;
	    tcmdPtr->refCount = 1;
	    flags |= TCL_TRACE_DELETE;
	    if (flags & (TCL_TRACE_ENTER_DURING_EXEC |
		    TCL_TRACE_LEAVE_DURING_EXEC)) {
		flags |= (TCL_TRACE_ENTER_EXEC | TCL_TRACE_LEAVE_EXEC);
	    }
	    memcpy(tcmdPtr->command, command, length+1);
	    name = Tcl_GetString(objv[3]);
	    if (Tcl_TraceCommand(interp, name, flags, TraceCommandProc,
		    tcmdPtr) != TCL_OK) {
		ckfree(tcmdPtr);
		return TCL_ERROR;
	    }
	} else {
	    /*
	     * Search through all of our traces on this command to see if
	     * there's one with the given command. If so, then delete the
	     * first one that matches.
	     */

	    ClientData clientData;

	    /*
	     * First ensure the name given is valid.
	     */

	    name = Tcl_GetString(objv[3]);
	    if (Tcl_FindCommand(interp,name,NULL,TCL_LEAVE_ERR_MSG) == NULL) {
		return TCL_ERROR;
	    }

	    FOREACH_COMMAND_TRACE(interp, name, clientData) {
		TraceCommandInfo *tcmdPtr = clientData;

		/*
		 * In checking the 'flags' field we must remove any extraneous
		 * flags which may have been temporarily added by various
		 * pieces of the trace mechanism.
		 */

		if ((tcmdPtr->length == length)
			&& ((tcmdPtr->flags & (TCL_TRACE_ANY_EXEC |
				TCL_TRACE_RENAME | TCL_TRACE_DELETE)) == flags)
			&& (strncmp(command, tcmdPtr->command,
				(size_t) length) == 0)) {
		    flags |= TCL_TRACE_DELETE;
		    if (flags & (TCL_TRACE_ENTER_DURING_EXEC |
			    TCL_TRACE_LEAVE_DURING_EXEC)) {
			flags |= (TCL_TRACE_ENTER_EXEC | TCL_TRACE_LEAVE_EXEC);
		    }
		    Tcl_UntraceCommand(interp, name, flags,
			    TraceCommandProc, clientData);
		    if (tcmdPtr->stepTrace != NULL) {
			/*
			 * We need to remove the interpreter-wide trace which
			 * we created to allow 'step' traces.
			 */

			Tcl_DeleteTrace(interp, tcmdPtr->stepTrace);
			tcmdPtr->stepTrace = NULL;
			if (tcmdPtr->startCmd != NULL) {
			    ckfree(tcmdPtr->startCmd);
			}
		    }
		    if (tcmdPtr->flags & TCL_TRACE_EXEC_IN_PROGRESS) {
			/*
			 * Postpone deletion.
			 */

			tcmdPtr->flags = 0;
		    }
		    if (tcmdPtr->refCount-- <= 1) {
			ckfree(tcmdPtr);
		    }
		    break;
		}
	    }
	}
	break;
    }
    case TRACE_INFO: {
	ClientData clientData;
	Tcl_Obj *resultListPtr;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name");
	    return TCL_ERROR;
	}

	name = Tcl_GetString(objv[3]);

	/*
	 * First ensure the name given is valid.
	 */

	if (Tcl_FindCommand(interp, name, NULL, TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}

	resultListPtr = Tcl_NewListObj(0, NULL);
	FOREACH_COMMAND_TRACE(interp, name, clientData) {
	    int numOps = 0;
	    Tcl_Obj *opObj, *eachTraceObjPtr, *elemObjPtr;
	    TraceCommandInfo *tcmdPtr = clientData;

	    /*
	     * Build a list with the ops list as the first obj element and the
	     * tcmdPtr->command string as the second obj element. Append this
	     * list (as an element) to the end of the result object list.
	     */

	    elemObjPtr = Tcl_NewListObj(0, NULL);
	    Tcl_IncrRefCount(elemObjPtr);
	    if (tcmdPtr->flags & TCL_TRACE_ENTER_EXEC) {
		TclNewLiteralStringObj(opObj, "enter");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObj);
	    }
	    if (tcmdPtr->flags & TCL_TRACE_LEAVE_EXEC) {
		TclNewLiteralStringObj(opObj, "leave");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObj);
	    }
	    if (tcmdPtr->flags & TCL_TRACE_ENTER_DURING_EXEC) {
		TclNewLiteralStringObj(opObj, "enterstep");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObj);
	    }
	    if (tcmdPtr->flags & TCL_TRACE_LEAVE_DURING_EXEC) {
		TclNewLiteralStringObj(opObj, "leavestep");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObj);
	    }
	    Tcl_ListObjLength(NULL, elemObjPtr, &numOps);
	    if (0 == numOps) {
		Tcl_DecrRefCount(elemObjPtr);
		continue;
	    }
	    eachTraceObjPtr = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(NULL, eachTraceObjPtr, elemObjPtr);
	    Tcl_DecrRefCount(elemObjPtr);
	    elemObjPtr = NULL;

	    Tcl_ListObjAppendElement(NULL, eachTraceObjPtr,
		    Tcl_NewStringObj(tcmdPtr->command, -1));
	    Tcl_ListObjAppendElement(interp, resultListPtr, eachTraceObjPtr);
	}
	Tcl_SetObjResult(interp, resultListPtr);
	break;
    }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceCommandObjCmd --
 *
 *	Helper function for Tcl_TraceObjCmd; implements the [trace
 *	{add|info|remove} command ...] subcommands. See the user documentation
 *	for details on what these do.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Depends on the operation (add, remove, or info) being performed; may
 *	add or remove command traces on a command.
 *
 *----------------------------------------------------------------------
 */

static int
TraceCommandObjCmd(
    Tcl_Interp *interp,		/* Current interpreter. */
    int optionIndex,		/* Add, info or remove */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int commandLength, index;
    const char *name, *command;
    size_t length;
    enum traceOptions { TRACE_ADD, TRACE_INFO, TRACE_REMOVE };
    static const char *const opStrings[] = { "delete", "rename", NULL };
    enum operations { TRACE_CMD_DELETE, TRACE_CMD_RENAME };

    switch ((enum traceOptions) optionIndex) {
    case TRACE_ADD:
    case TRACE_REMOVE: {
	int flags = 0;
	int i, listLen, result;
	Tcl_Obj **elemPtrs;

	if (objc != 6) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name opList command");
	    return TCL_ERROR;
	}

	/*
	 * Make sure the ops argument is a list object; get its length and a
	 * pointer to its array of element pointers.
	 */

	result = Tcl_ListObjGetElements(interp, objv[4], &listLen, &elemPtrs);
	if (result != TCL_OK) {
	    return result;
	}
	if (listLen == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "bad operation list \"\": must be one or more of"
		    " delete or rename", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRACE", "NOOPS",
		    NULL);
	    return TCL_ERROR;
	}

	for (i = 0; i < listLen; i++) {
	    if (Tcl_GetIndexFromObj(interp, elemPtrs[i], opStrings,
		    "operation", TCL_EXACT, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum operations) index) {
	    case TRACE_CMD_RENAME:
		flags |= TCL_TRACE_RENAME;
		break;
	    case TRACE_CMD_DELETE:
		flags |= TCL_TRACE_DELETE;
		break;
	    }
	}

	command = Tcl_GetStringFromObj(objv[5], &commandLength);
	length = (size_t) commandLength;
	if ((enum traceOptions) optionIndex == TRACE_ADD) {
	    TraceCommandInfo *tcmdPtr = ckalloc(
		    TclOffset(TraceCommandInfo, command) + 1 + length);

	    tcmdPtr->flags = flags;
	    tcmdPtr->stepTrace = NULL;
	    tcmdPtr->startLevel = 0;
	    tcmdPtr->startCmd = NULL;
	    tcmdPtr->length = length;
	    tcmdPtr->refCount = 1;
	    flags |= TCL_TRACE_DELETE;
	    memcpy(tcmdPtr->command, command, length+1);
	    name = Tcl_GetString(objv[3]);
	    if (Tcl_TraceCommand(interp, name, flags, TraceCommandProc,
		    tcmdPtr) != TCL_OK) {
		ckfree(tcmdPtr);
		return TCL_ERROR;
	    }
	} else {
	    /*
	     * Search through all of our traces on this command to see if
	     * there's one with the given command. If so, then delete the
	     * first one that matches.
	     */

	    ClientData clientData;

	    /*
	     * First ensure the name given is valid.
	     */

	    name = Tcl_GetString(objv[3]);
	    if (Tcl_FindCommand(interp,name,NULL,TCL_LEAVE_ERR_MSG) == NULL) {
		return TCL_ERROR;
	    }

	    FOREACH_COMMAND_TRACE(interp, name, clientData) {
		TraceCommandInfo *tcmdPtr = clientData;

		if ((tcmdPtr->length == length) && (tcmdPtr->flags == flags)
			&& (strncmp(command, tcmdPtr->command,
				(size_t) length) == 0)) {
		    Tcl_UntraceCommand(interp, name, flags | TCL_TRACE_DELETE,
			    TraceCommandProc, clientData);
		    tcmdPtr->flags |= TCL_TRACE_DESTROYED;
		    if (tcmdPtr->refCount-- <= 1) {
			ckfree(tcmdPtr);
		    }
		    break;
		}
	    }
	}
	break;
    }
    case TRACE_INFO: {
	ClientData clientData;
	Tcl_Obj *resultListPtr;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name");
	    return TCL_ERROR;
	}

	/*
	 * First ensure the name given is valid.
	 */

	name = Tcl_GetString(objv[3]);
	if (Tcl_FindCommand(interp, name, NULL, TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR;
	}

	resultListPtr = Tcl_NewListObj(0, NULL);
	FOREACH_COMMAND_TRACE(interp, name, clientData) {
	    int numOps = 0;
	    Tcl_Obj *opObj, *eachTraceObjPtr, *elemObjPtr;
	    TraceCommandInfo *tcmdPtr = clientData;

	    /*
	     * Build a list with the ops list as the first obj element and the
	     * tcmdPtr->command string as the second obj element. Append this
	     * list (as an element) to the end of the result object list.
	     */

	    elemObjPtr = Tcl_NewListObj(0, NULL);
	    Tcl_IncrRefCount(elemObjPtr);
	    if (tcmdPtr->flags & TCL_TRACE_RENAME) {
		TclNewLiteralStringObj(opObj, "rename");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObj);
	    }
	    if (tcmdPtr->flags & TCL_TRACE_DELETE) {
		TclNewLiteralStringObj(opObj, "delete");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObj);
	    }
	    Tcl_ListObjLength(NULL, elemObjPtr, &numOps);
	    if (0 == numOps) {
		Tcl_DecrRefCount(elemObjPtr);
		continue;
	    }
	    eachTraceObjPtr = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(NULL, eachTraceObjPtr, elemObjPtr);
	    Tcl_DecrRefCount(elemObjPtr);

	    elemObjPtr = Tcl_NewStringObj(tcmdPtr->command, -1);
	    Tcl_ListObjAppendElement(NULL, eachTraceObjPtr, elemObjPtr);
	    Tcl_ListObjAppendElement(interp, resultListPtr, eachTraceObjPtr);
	}
	Tcl_SetObjResult(interp, resultListPtr);
	break;
    }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceVariableObjCmd --
 *
 *	Helper function for Tcl_TraceObjCmd; implements the [trace
 *	{add|info|remove} variable ...] subcommands. See the user
 *	documentation for details on what these do.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Depends on the operation (add, remove, or info) being performed; may
 *	add or remove variable traces on a variable.
 *
 *----------------------------------------------------------------------
 */

static int
TraceVariableObjCmd(
    Tcl_Interp *interp,		/* Current interpreter. */
    int optionIndex,		/* Add, info or remove */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int commandLength, index;
    const char *name, *command;
    size_t length;
    ClientData clientData;
    enum traceOptions { TRACE_ADD, TRACE_INFO, TRACE_REMOVE };
    static const char *const opStrings[] = {
	"array", "read", "unset", "write", NULL
    };
    enum operations {
	TRACE_VAR_ARRAY, TRACE_VAR_READ, TRACE_VAR_UNSET, TRACE_VAR_WRITE
    };

    switch ((enum traceOptions) optionIndex) {
    case TRACE_ADD:
    case TRACE_REMOVE: {
	int flags = 0;
	int i, listLen, result;
	Tcl_Obj **elemPtrs;

	if (objc != 6) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name opList command");
	    return TCL_ERROR;
	}

	/*
	 * Make sure the ops argument is a list object; get its length and a
	 * pointer to its array of element pointers.
	 */

	result = Tcl_ListObjGetElements(interp, objv[4], &listLen, &elemPtrs);
	if (result != TCL_OK) {
	    return result;
	}
	if (listLen == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "bad operation list \"\": must be one or more of"
		    " array, read, unset, or write", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRACE", "NOOPS",
		    NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < listLen ; i++) {
	    if (Tcl_GetIndexFromObj(interp, elemPtrs[i], opStrings,
		    "operation", TCL_EXACT, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum operations) index) {
	    case TRACE_VAR_ARRAY:
		flags |= TCL_TRACE_ARRAY;
		break;
	    case TRACE_VAR_READ:
		flags |= TCL_TRACE_READS;
		break;
	    case TRACE_VAR_UNSET:
		flags |= TCL_TRACE_UNSETS;
		break;
	    case TRACE_VAR_WRITE:
		flags |= TCL_TRACE_WRITES;
		break;
	    }
	}
	command = Tcl_GetStringFromObj(objv[5], &commandLength);
	length = (size_t) commandLength;
	if ((enum traceOptions) optionIndex == TRACE_ADD) {
	    CombinedTraceVarInfo *ctvarPtr = ckalloc(
		    TclOffset(CombinedTraceVarInfo, traceCmdInfo.command)
		    + 1 + length);

	    ctvarPtr->traceCmdInfo.flags = flags;
#ifndef TCL_REMOVE_OBSOLETE_TRACES
	    if (objv[0] == NULL) {
		ctvarPtr->traceCmdInfo.flags |= TCL_TRACE_OLD_STYLE;
	    }
#endif
	    ctvarPtr->traceCmdInfo.length = length;
	    flags |= TCL_TRACE_UNSETS | TCL_TRACE_RESULT_OBJECT;
	    memcpy(ctvarPtr->traceCmdInfo.command, command, length+1);
	    ctvarPtr->traceInfo.traceProc = TraceVarProc;
	    ctvarPtr->traceInfo.clientData = &ctvarPtr->traceCmdInfo;
	    ctvarPtr->traceInfo.flags = flags;
	    name = Tcl_GetString(objv[3]);
	    if (TraceVarEx(interp, name, NULL, (VarTrace *) ctvarPtr)
		    != TCL_OK) {
		ckfree(ctvarPtr);
		return TCL_ERROR;
	    }
	} else {
	    /*
	     * Search through all of our traces on this variable to see if
	     * there's one with the given command. If so, then delete the
	     * first one that matches.
	     */

	    name = Tcl_GetString(objv[3]);
	    FOREACH_VAR_TRACE(interp, name, clientData) {
		TraceVarInfo *tvarPtr = clientData;

		if ((tvarPtr->length == length)
			&& ((tvarPtr->flags
#ifndef TCL_REMOVE_OBSOLETE_TRACES
& ~TCL_TRACE_OLD_STYLE
#endif
						)==flags)
			&& (strncmp(command, tvarPtr->command,
				(size_t) length) == 0)) {
		    Tcl_UntraceVar2(interp, name, NULL,
			    flags | TCL_TRACE_UNSETS | TCL_TRACE_RESULT_OBJECT,
			    TraceVarProc, clientData);
		    break;
		}
	    }
	}
	break;
    }
    case TRACE_INFO: {
	Tcl_Obj *resultListPtr;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name");
	    return TCL_ERROR;
	}

	resultListPtr = Tcl_NewObj();
	name = Tcl_GetString(objv[3]);
	FOREACH_VAR_TRACE(interp, name, clientData) {
	    Tcl_Obj *opObjPtr, *eachTraceObjPtr, *elemObjPtr;
	    TraceVarInfo *tvarPtr = clientData;

	    /*
	     * Build a list with the ops list as the first obj element and the
	     * tcmdPtr->command string as the second obj element. Append this
	     * list (as an element) to the end of the result object list.
	     */

	    elemObjPtr = Tcl_NewListObj(0, NULL);
	    if (tvarPtr->flags & TCL_TRACE_ARRAY) {
		TclNewLiteralStringObj(opObjPtr, "array");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObjPtr);
	    }
	    if (tvarPtr->flags & TCL_TRACE_READS) {
		TclNewLiteralStringObj(opObjPtr, "read");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObjPtr);
	    }
	    if (tvarPtr->flags & TCL_TRACE_WRITES) {
		TclNewLiteralStringObj(opObjPtr, "write");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObjPtr);
	    }
	    if (tvarPtr->flags & TCL_TRACE_UNSETS) {
		TclNewLiteralStringObj(opObjPtr, "unset");
		Tcl_ListObjAppendElement(NULL, elemObjPtr, opObjPtr);
	    }
	    eachTraceObjPtr = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(NULL, eachTraceObjPtr, elemObjPtr);

	    elemObjPtr = Tcl_NewStringObj(tvarPtr->command, -1);
	    Tcl_ListObjAppendElement(NULL, eachTraceObjPtr, elemObjPtr);
	    Tcl_ListObjAppendElement(interp, resultListPtr,
		    eachTraceObjPtr);
	}
	Tcl_SetObjResult(interp, resultListPtr);
	break;
    }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CommandTraceInfo --
 *
 *	Return the clientData value associated with a trace on a command.
 *	This function can also be used to step through all of the traces on a
 *	particular command that have the same trace function.
 *
 * Results:
 *	The return value is the clientData value associated with a trace on
 *	the given command. Information will only be returned for a trace with
 *	proc as trace function. If the clientData argument is NULL then the
 *	first such trace is returned; otherwise, the next relevant one after
 *	the one given by clientData will be returned. If the command doesn't
 *	exist then an error message is left in the interpreter and NULL is
 *	returned. Also, if there are no (more) traces for the given command,
 *	NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_CommandTraceInfo(
    Tcl_Interp *interp,		/* Interpreter containing command. */
    const char *cmdName,	/* Name of command. */
    int flags,			/* OR-ed combo or TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY (can be 0). */
    Tcl_CommandTraceProc *proc,	/* Function assocated with trace. */
    ClientData prevClientData)	/* If non-NULL, gives last value returned by
				 * this function, so this call will return the
				 * next trace after that one. If NULL, this
				 * call will return the first trace. */
{
    Command *cmdPtr;
    register CommandTrace *tracePtr;

    cmdPtr = (Command *) Tcl_FindCommand(interp, cmdName, NULL,
	    TCL_LEAVE_ERR_MSG);
    if (cmdPtr == NULL) {
	return NULL;
    }

    /*
     * Find the relevant trace, if any, and return its clientData.
     */

    tracePtr = cmdPtr->tracePtr;
    if (prevClientData != NULL) {
	for (; tracePtr!=NULL ; tracePtr=tracePtr->nextPtr) {
	    if ((tracePtr->clientData == prevClientData)
		    && (tracePtr->traceProc == proc)) {
		tracePtr = tracePtr->nextPtr;
		break;
	    }
	}
    }
    for (; tracePtr!=NULL ; tracePtr=tracePtr->nextPtr) {
	if (tracePtr->traceProc == proc) {
	    return tracePtr->clientData;
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceCommand --
 *
 *	Arrange for rename/deletes to a command to cause a function to be
 *	invoked, which can monitor the operations.
 *
 *	Also optionally arrange for execution of that command to cause a
 *	function to be invoked.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	A trace is set up on the command given by cmdName, such that future
 *	changes to the command will be intermediated by proc. See the manual
 *	entry for complete details on the calling sequence for proc.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_TraceCommand(
    Tcl_Interp *interp,		/* Interpreter in which command is to be
				 * traced. */
    const char *cmdName,	/* Name of command. */
    int flags,			/* OR-ed collection of bits, including any of
				 * TCL_TRACE_RENAME, TCL_TRACE_DELETE, and any
				 * of the TRACE_*_EXEC flags */
    Tcl_CommandTraceProc *proc,	/* Function to call when specified ops are
				 * invoked upon cmdName. */
    ClientData clientData)	/* Arbitrary argument to pass to proc. */
{
    Command *cmdPtr;
    register CommandTrace *tracePtr;

    cmdPtr = (Command *) Tcl_FindCommand(interp, cmdName, NULL,
	    TCL_LEAVE_ERR_MSG);
    if (cmdPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Set up trace information.
     */

    tracePtr = ckalloc(sizeof(CommandTrace));
    tracePtr->traceProc = proc;
    tracePtr->clientData = clientData;
    tracePtr->flags = flags &
	    (TCL_TRACE_RENAME | TCL_TRACE_DELETE | TCL_TRACE_ANY_EXEC);
    tracePtr->nextPtr = cmdPtr->tracePtr;
    tracePtr->refCount = 1;
    cmdPtr->tracePtr = tracePtr;
    if (tracePtr->flags & TCL_TRACE_ANY_EXEC) {
	/*
	 * Bug 3484621: up the interp's epoch if this is a BC'ed command
	 */

	if ((cmdPtr->compileProc != NULL) && !(cmdPtr->flags & CMD_HAS_EXEC_TRACES)){
	    Interp *iPtr = (Interp *) interp;
	    iPtr->compileEpoch++;
	}
	cmdPtr->flags |= CMD_HAS_EXEC_TRACES;
    }


    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UntraceCommand --
 *
 *	Remove a previously-created trace for a command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there exists a trace for the command given by cmdName with the
 *	given flags, proc, and clientData, then that trace is removed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_UntraceCommand(
    Tcl_Interp *interp,		/* Interpreter containing command. */
    const char *cmdName,	/* Name of command. */
    int flags,			/* OR-ed collection of bits, including any of
				 * TCL_TRACE_RENAME, TCL_TRACE_DELETE, and any
				 * of the TRACE_*_EXEC flags */
    Tcl_CommandTraceProc *proc,	/* Function assocated with trace. */
    ClientData clientData)	/* Arbitrary argument to pass to proc. */
{
    register CommandTrace *tracePtr;
    CommandTrace *prevPtr;
    Command *cmdPtr;
    Interp *iPtr = (Interp *) interp;
    ActiveCommandTrace *activePtr;
    int hasExecTraces = 0;

    cmdPtr = (Command *) Tcl_FindCommand(interp, cmdName, NULL,
	    TCL_LEAVE_ERR_MSG);
    if (cmdPtr == NULL) {
	return;
    }

    flags &= (TCL_TRACE_RENAME | TCL_TRACE_DELETE | TCL_TRACE_ANY_EXEC);

    for (tracePtr = cmdPtr->tracePtr, prevPtr = NULL; ;
	    prevPtr = tracePtr, tracePtr = tracePtr->nextPtr) {
	if (tracePtr == NULL) {
	    return;
	}
	if ((tracePtr->traceProc == proc)
		&& ((tracePtr->flags & (TCL_TRACE_RENAME | TCL_TRACE_DELETE |
			TCL_TRACE_ANY_EXEC)) == flags)
		&& (tracePtr->clientData == clientData)) {
	    if (tracePtr->flags & TCL_TRACE_ANY_EXEC) {
		hasExecTraces = 1;
	    }
	    break;
	}
    }

    /*
     * The code below makes it possible to delete traces while traces are
     * active: it makes sure that the deleted trace won't be processed by
     * CallCommandTraces.
     */

    for (activePtr = iPtr->activeCmdTracePtr;  activePtr != NULL;
	    activePtr = activePtr->nextPtr) {
	if (activePtr->nextTracePtr == tracePtr) {
	    if (activePtr->reverseScan) {
		activePtr->nextTracePtr = prevPtr;
	    } else {
		activePtr->nextTracePtr = tracePtr->nextPtr;
	    }
	}
    }
    if (prevPtr == NULL) {
	cmdPtr->tracePtr = tracePtr->nextPtr;
    } else {
	prevPtr->nextPtr = tracePtr->nextPtr;
    }
    tracePtr->flags = 0;

    if (tracePtr->refCount-- <= 1) {
	ckfree(tracePtr);
    }

    if (hasExecTraces) {
	for (tracePtr = cmdPtr->tracePtr, prevPtr = NULL; tracePtr != NULL ;
		prevPtr = tracePtr, tracePtr = tracePtr->nextPtr) {
	    if (tracePtr->flags & TCL_TRACE_ANY_EXEC) {
		return;
	    }
	}

	/*
	 * None of the remaining traces on this command are execution traces.
	 * We therefore remove this flag:
	 */

	cmdPtr->flags &= ~CMD_HAS_EXEC_TRACES;

        /*
	 * Bug 3484621: up the interp's epoch if this is a BC'ed command
	 */

	if (cmdPtr->compileProc != NULL) {
	    Interp *iPtr = (Interp *) interp;
	    iPtr->compileEpoch++;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TraceCommandProc --
 *
 *	This function is called to handle command changes that have been
 *	traced using the "trace" command, when using the 'rename' or 'delete'
 *	options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the command associated with the trace.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
TraceCommandProc(
    ClientData clientData,	/* Information about the command trace. */
    Tcl_Interp *interp,		/* Interpreter containing command. */
    const char *oldName,	/* Name of command being changed. */
    const char *newName,	/* New name of command. Empty string or NULL
				 * means command is being deleted (renamed to
				 * ""). */
    int flags)			/* OR-ed bits giving operation and other
				 * information. */
{
    TraceCommandInfo *tcmdPtr = clientData;
    int code;
    Tcl_DString cmd;

    tcmdPtr->refCount++;

    if ((tcmdPtr->flags & flags) && !Tcl_InterpDeleted(interp)
	    && !Tcl_LimitExceeded(interp)) {
	/*
	 * Generate a command to execute by appending list elements for the
	 * old and new command name and the operation.
	 */

	Tcl_DStringInit(&cmd);
	Tcl_DStringAppend(&cmd, tcmdPtr->command, (int) tcmdPtr->length);
	Tcl_DStringAppendElement(&cmd, oldName);
	Tcl_DStringAppendElement(&cmd, (newName ? newName : ""));
	if (flags & TCL_TRACE_RENAME) {
	    TclDStringAppendLiteral(&cmd, " rename");
	} else if (flags & TCL_TRACE_DELETE) {
	    TclDStringAppendLiteral(&cmd, " delete");
	}

	/*
	 * Execute the command. We discard any object result the command
	 * returns.
	 *
	 * Add the TCL_TRACE_DESTROYED flag to tcmdPtr to indicate to other
	 * areas that this will be destroyed by us, otherwise a double-free
	 * might occur depending on what the eval does.
	 */

	if (flags & TCL_TRACE_DESTROYED) {
	    tcmdPtr->flags |= TCL_TRACE_DESTROYED;
	}
	code = Tcl_EvalEx(interp, Tcl_DStringValue(&cmd),
		Tcl_DStringLength(&cmd), 0);
	if (code != TCL_OK) {
	    /* We ignore errors in these traced commands */
	    /*** QUESTION: Use Tcl_BackgroundException(interp, code); instead? ***/
	}
	Tcl_DStringFree(&cmd);
    }

    /*
     * We delete when the trace was destroyed or if this is a delete trace,
     * because command deletes are unconditional, so the trace must go away.
     */

    if (flags & (TCL_TRACE_DESTROYED | TCL_TRACE_DELETE)) {
	int untraceFlags = tcmdPtr->flags;
	Tcl_InterpState state;

	if (tcmdPtr->stepTrace != NULL) {
	    Tcl_DeleteTrace(interp, tcmdPtr->stepTrace);
	    tcmdPtr->stepTrace = NULL;
	    if (tcmdPtr->startCmd != NULL) {
		ckfree(tcmdPtr->startCmd);
	    }
	}
	if (tcmdPtr->flags & TCL_TRACE_EXEC_IN_PROGRESS) {
	    /*
	     * Postpone deletion, until exec trace returns.
	     */

	    tcmdPtr->flags = 0;
	}

	/*
	 * We need to construct the same flags for Tcl_UntraceCommand as were
	 * passed to Tcl_TraceCommand. Reproduce the processing of [trace add
	 * execution/command]. Be careful to keep this code in sync with that.
	 */

	if (untraceFlags & TCL_TRACE_ANY_EXEC) {
	    untraceFlags |= TCL_TRACE_DELETE;
	    if (untraceFlags & (TCL_TRACE_ENTER_DURING_EXEC
		    | TCL_TRACE_LEAVE_DURING_EXEC)) {
		untraceFlags |= (TCL_TRACE_ENTER_EXEC | TCL_TRACE_LEAVE_EXEC);
	    }
	} else if (untraceFlags & TCL_TRACE_RENAME) {
	    untraceFlags |= TCL_TRACE_DELETE;
	}

	/*
	 * Remove the trace since TCL_TRACE_DESTROYED tells us to, or the
	 * command we're tracing has just gone away. Then decrement the
	 * clientData refCount that was set up by trace creation.
	 *
	 * Note that we save the (return) state of the interpreter to prevent
	 * bizarre error messages.
	 */

	state = Tcl_SaveInterpState(interp, TCL_OK);
	Tcl_UntraceCommand(interp, oldName, untraceFlags,
		TraceCommandProc, clientData);
	Tcl_RestoreInterpState(interp, state);
	tcmdPtr->refCount--;
    }
    if (tcmdPtr->refCount-- <= 1) {
	ckfree(tcmdPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclCheckExecutionTraces --
 *
 *	Checks on all current command execution traces, and invokes functions
 *	which have been registered. This function can be used by other code
 *	which performs execution to unify the tracing system, so that
 *	execution traces will function for that other code.
 *
 *	For instance extensions like [incr Tcl] which use their own execution
 *	technique can make use of Tcl's tracing.
 *
 *	This function is called by 'TclEvalObjvInternal'
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR, etc.
 *
 * Side effects:
 *	Those side effects made by any trace functions called.
 *
 *----------------------------------------------------------------------
 */

int
TclCheckExecutionTraces(
    Tcl_Interp *interp,		/* The current interpreter. */
    const char *command,	/* Pointer to beginning of the current command
				 * string. */
    int numChars,		/* The number of characters in 'command' which
				 * are part of the command string. */
    Command *cmdPtr,		/* Points to command's Command struct. */
    int code,			/* The current result code. */
    int traceFlags,		/* Current tracing situation. */
    int objc,			/* Number of arguments for the command. */
    Tcl_Obj *const objv[])	/* Pointers to Tcl_Obj of each argument. */
{
    Interp *iPtr = (Interp *) interp;
    CommandTrace *tracePtr, *lastTracePtr;
    ActiveCommandTrace active;
    int curLevel;
    int traceCode = TCL_OK;
    Tcl_InterpState state = NULL;

    if (cmdPtr->tracePtr == NULL) {
	return traceCode;
    }

    curLevel = iPtr->varFramePtr->level;

    active.nextPtr = iPtr->activeCmdTracePtr;
    iPtr->activeCmdTracePtr = &active;

    active.cmdPtr = cmdPtr;
    lastTracePtr = NULL;
    for (tracePtr = cmdPtr->tracePtr;
	    (traceCode == TCL_OK) && (tracePtr != NULL);
	    tracePtr = active.nextTracePtr) {
	if (traceFlags & TCL_TRACE_LEAVE_EXEC) {
	    /*
	     * Execute the trace command in order of creation for "leave".
	     */

	    active.reverseScan = 1;
	    active.nextTracePtr = NULL;
	    tracePtr = cmdPtr->tracePtr;
	    while (tracePtr->nextPtr != lastTracePtr) {
		active.nextTracePtr = tracePtr;
		tracePtr = tracePtr->nextPtr;
	    }
	} else {
	    active.reverseScan = 0;
	    active.nextTracePtr = tracePtr->nextPtr;
	}
	if (tracePtr->traceProc == TraceCommandProc) {
	    TraceCommandInfo *tcmdPtr = tracePtr->clientData;

	    if (tcmdPtr->flags != 0) {
		tcmdPtr->curFlags = traceFlags | TCL_TRACE_EXEC_DIRECT;
		tcmdPtr->curCode  = code;
		tcmdPtr->refCount++;
		if (state == NULL) {
		    state = Tcl_SaveInterpState(interp, code);
		}
		traceCode = TraceExecutionProc(tcmdPtr, interp, curLevel,
			command, (Tcl_Command) cmdPtr, objc, objv);
		if (tcmdPtr->refCount-- <= 1) {
		    ckfree(tcmdPtr);
		}
	    }
	}
	if (active.nextTracePtr) {
	    lastTracePtr = active.nextTracePtr->nextPtr;
	}
    }
    iPtr->activeCmdTracePtr = active.nextPtr;
    if (state) {
	if (traceCode == TCL_OK) {
	    (void) Tcl_RestoreInterpState(interp, state);
	} else {
	    Tcl_DiscardInterpState(state);
	}
    }

    return traceCode;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCheckInterpTraces --
 *
 *	Checks on all current traces, and invokes functions which have been
 *	registered. This function can be used by other code which performs
 *	execution to unify the tracing system. For instance extensions like
 *	[incr Tcl] which use their own execution technique can make use of
 *	Tcl's tracing.
 *
 *	This function is called by 'TclEvalObjvInternal'
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR, etc.
 *
 * Side effects:
 *	Those side effects made by any trace functions called.
 *
 *----------------------------------------------------------------------
 */

int
TclCheckInterpTraces(
    Tcl_Interp *interp,		/* The current interpreter. */
    const char *command,	/* Pointer to beginning of the current command
				 * string. */
    int numChars,		/* The number of characters in 'command' which
				 * are part of the command string. */
    Command *cmdPtr,		/* Points to command's Command struct. */
    int code,			/* The current result code. */
    int traceFlags,		/* Current tracing situation. */
    int objc,			/* Number of arguments for the command. */
    Tcl_Obj *const objv[])	/* Pointers to Tcl_Obj of each argument. */
{
    Interp *iPtr = (Interp *) interp;
    Trace *tracePtr, *lastTracePtr;
    ActiveInterpTrace active;
    int curLevel;
    int traceCode = TCL_OK;
    Tcl_InterpState state = NULL;

    if ((iPtr->tracePtr == NULL)
	    || (iPtr->flags & INTERP_TRACE_IN_PROGRESS)) {
	return(traceCode);
    }

    curLevel = iPtr->numLevels;

    active.nextPtr = iPtr->activeInterpTracePtr;
    iPtr->activeInterpTracePtr = &active;

    lastTracePtr = NULL;
    for (tracePtr = iPtr->tracePtr;
	    (traceCode == TCL_OK) && (tracePtr != NULL);
	    tracePtr = active.nextTracePtr) {
	if (traceFlags & TCL_TRACE_ENTER_EXEC) {
	    /*
	     * Execute the trace command in reverse order of creation for
	     * "enterstep" operation. The order is changed for "enterstep"
	     * instead of for "leavestep" as was done in
	     * TclCheckExecutionTraces because for step traces,
	     * Tcl_CreateObjTrace creates one more linked list of traces which
	     * results in one more reversal of trace invocation.
	     */

	    active.reverseScan = 1;
	    active.nextTracePtr = NULL;
	    tracePtr = iPtr->tracePtr;
	    while (tracePtr->nextPtr != lastTracePtr) {
		active.nextTracePtr = tracePtr;
		tracePtr = tracePtr->nextPtr;
	    }
	    if (active.nextTracePtr) {
		lastTracePtr = active.nextTracePtr->nextPtr;
	    }
	} else {
	    active.reverseScan = 0;
	    active.nextTracePtr = tracePtr->nextPtr;
	}

	if (tracePtr->level > 0 && curLevel > tracePtr->level) {
	    continue;
	}

	if (!(tracePtr->flags & TCL_TRACE_EXEC_IN_PROGRESS)) {
	    /*
	     * The proc invoked might delete the traced command which which
	     * might try to free tracePtr. We want to use tracePtr until the
	     * end of this if section, so we use Tcl_Preserve() and
	     * Tcl_Release() to be sure it is not freed while we still need
	     * it.
	     */

	    Tcl_Preserve(tracePtr);
	    tracePtr->flags |= TCL_TRACE_EXEC_IN_PROGRESS;
	    if (state == NULL) {
		state = Tcl_SaveInterpState(interp, code);
	    }

	    if (tracePtr->flags &
		    (TCL_TRACE_ENTER_EXEC | TCL_TRACE_LEAVE_EXEC)) {
		/*
		 * New style trace.
		 */

		if (tracePtr->flags & traceFlags) {
		    if (tracePtr->proc == TraceExecutionProc) {
			TraceCommandInfo *tcmdPtr = tracePtr->clientData;

			tcmdPtr->curFlags = traceFlags;
			tcmdPtr->curCode = code;
		    }
		    traceCode = tracePtr->proc(tracePtr->clientData, interp,
			    curLevel, command, (Tcl_Command) cmdPtr, objc,
			    objv);
		}
	    } else {
		/*
		 * Old-style trace.
		 */

		if (traceFlags & TCL_TRACE_ENTER_EXEC) {
		    /*
		     * Old-style interpreter-wide traces only trigger before
		     * the command is executed.
		     */

		    traceCode = CallTraceFunction(interp, tracePtr, cmdPtr,
			    command, numChars, objc, objv);
		}
	    }
	    tracePtr->flags &= ~TCL_TRACE_EXEC_IN_PROGRESS;
	    Tcl_Release(tracePtr);
	}
    }
    iPtr->activeInterpTracePtr = active.nextPtr;
    if (state) {
	if (traceCode == TCL_OK) {
	    Tcl_RestoreInterpState(interp, state);
	} else {
	    Tcl_DiscardInterpState(state);
	}
    }

    return traceCode;
}

/*
 *----------------------------------------------------------------------
 *
 * CallTraceFunction --
 *
 *	Invokes a trace function registered with an interpreter. These
 *	functions trace command execution. Currently this trace function is
 *	called with the address of the string-based Tcl_CmdProc for the
 *	command, not the Tcl_ObjCmdProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Those side effects made by the trace function.
 *
 *----------------------------------------------------------------------
 */

static int
CallTraceFunction(
    Tcl_Interp *interp,		/* The current interpreter. */
    register Trace *tracePtr,	/* Describes the trace function to call. */
    Command *cmdPtr,		/* Points to command's Command struct. */
    const char *command,	/* Points to the first character of the
				 * command's source before substitutions. */
    int numChars,		/* The number of characters in the command's
				 * source. */
    register int objc,		/* Number of arguments for the command. */
    Tcl_Obj *const objv[])	/* Pointers to Tcl_Obj of each argument. */
{
    Interp *iPtr = (Interp *) interp;
    char *commandCopy;
    int traceCode;

    /*
     * Copy the command characters into a new string.
     */

    commandCopy = TclStackAlloc(interp, (unsigned) numChars + 1);
    memcpy(commandCopy, command, (size_t) numChars);
    commandCopy[numChars] = '\0';

    /*
     * Call the trace function then free allocated storage.
     */

    traceCode = tracePtr->proc(tracePtr->clientData, (Tcl_Interp *) iPtr,
	    iPtr->numLevels, commandCopy, (Tcl_Command) cmdPtr, objc, objv);

    TclStackFree(interp, commandCopy);
    return traceCode;
}

/*
 *----------------------------------------------------------------------
 *
 * CommandObjTraceDeleted --
 *
 *	Ensure the trace is correctly deleted by decrementing its refCount and
 *	only deleting if no other references exist.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May release memory.
 *
 *----------------------------------------------------------------------
 */

static void
CommandObjTraceDeleted(
    ClientData clientData)
{
    TraceCommandInfo *tcmdPtr = clientData;

    if (tcmdPtr->refCount-- <= 1) {
	ckfree(tcmdPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TraceExecutionProc --
 *
 *	This function is invoked whenever code relevant to a 'trace execution'
 *	command is executed. It is called in one of two ways in Tcl's core:
 *
 *	(i) by the TclCheckExecutionTraces, when an execution trace has been
 *	triggered.
 *	(ii) by TclCheckInterpTraces, when a prior execution trace has created
 *	a trace of the internals of a procedure, passing in this function as
 *	the one to be called.
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR, etc.
 *
 * Side effects:
 *	May invoke an arbitrary Tcl procedure, and may create or delete an
 *	interpreter-wide trace.
 *
 *----------------------------------------------------------------------
 */

static int
TraceExecutionProc(
    ClientData clientData,
    Tcl_Interp *interp,
    int level,
    const char *command,
    Tcl_Command cmdInfo,
    int objc,
    struct Tcl_Obj *const objv[])
{
    int call = 0;
    Interp *iPtr = (Interp *) interp;
    TraceCommandInfo *tcmdPtr = clientData;
    int flags = tcmdPtr->curFlags;
    int code = tcmdPtr->curCode;
    int traceCode = TCL_OK;

    if (tcmdPtr->flags & TCL_TRACE_EXEC_IN_PROGRESS) {
	/*
	 * Inside any kind of execution trace callback, we do not allow any
	 * further execution trace callbacks to be called for the same trace.
	 */

	return traceCode;
    }

    if (!Tcl_InterpDeleted(interp) && !Tcl_LimitExceeded(interp)) {
	/*
	 * Check whether the current call is going to eval arbitrary Tcl code
	 * with a generated trace, or whether we are only going to setup
	 * interpreter-wide traces to implement the 'step' traces. This latter
	 * situation can happen if we create a command trace without either
	 * before or after operations, but with either of the step operations.
	 */

	if (flags & TCL_TRACE_EXEC_DIRECT) {
	    call = flags & tcmdPtr->flags &
		    (TCL_TRACE_ENTER_EXEC | TCL_TRACE_LEAVE_EXEC);
	} else {
	    call = 1;
	}

	/*
	 * First, if we have returned back to the level at which we created an
	 * interpreter trace for enterstep and/or leavestep execution traces,
	 * we remove it here.
	 */

	if ((flags & TCL_TRACE_LEAVE_EXEC) && (tcmdPtr->stepTrace != NULL)
		&& (level == tcmdPtr->startLevel)
		&& (strcmp(command, tcmdPtr->startCmd) == 0)) {
	    Tcl_DeleteTrace(interp, tcmdPtr->stepTrace);
	    tcmdPtr->stepTrace = NULL;
	    if (tcmdPtr->startCmd != NULL) {
		ckfree(tcmdPtr->startCmd);
	    }
	}

	/*
	 * Second, create the tcl callback, if required.
	 */

	if (call) {
	    Tcl_DString cmd, sub;
	    int i, saveInterpFlags;

	    Tcl_DStringInit(&cmd);
	    Tcl_DStringAppend(&cmd, tcmdPtr->command, (int)tcmdPtr->length);

	    /*
	     * Append command with arguments.
	     */

	    Tcl_DStringInit(&sub);
	    for (i = 0; i < objc; i++) {
		Tcl_DStringAppendElement(&sub, Tcl_GetString(objv[i]));
	    }
	    Tcl_DStringAppendElement(&cmd, Tcl_DStringValue(&sub));
	    Tcl_DStringFree(&sub);

	    if (flags & TCL_TRACE_ENTER_EXEC) {
		/*
		 * Append trace operation.
		 */

		if (flags & TCL_TRACE_EXEC_DIRECT) {
		    Tcl_DStringAppendElement(&cmd, "enter");
		} else {
		    Tcl_DStringAppendElement(&cmd, "enterstep");
		}
	    } else if (flags & TCL_TRACE_LEAVE_EXEC) {
		Tcl_Obj *resultCode;
		const char *resultCodeStr;

		/*
		 * Append result code.
		 */

		resultCode = Tcl_NewIntObj(code);
		resultCodeStr = Tcl_GetString(resultCode);
		Tcl_DStringAppendElement(&cmd, resultCodeStr);
		Tcl_DecrRefCount(resultCode);

		/*
		 * Append result string.
		 */

		Tcl_DStringAppendElement(&cmd, Tcl_GetStringResult(interp));

		/*
		 * Append trace operation.
		 */

		if (flags & TCL_TRACE_EXEC_DIRECT) {
		    Tcl_DStringAppendElement(&cmd, "leave");
		} else {
		    Tcl_DStringAppendElement(&cmd, "leavestep");
		}
	    } else {
		Tcl_Panic("TraceExecutionProc: bad flag combination");
	    }

	    /*
	     * Execute the command. We discard any object result the command
	     * returns.
	     */

	    saveInterpFlags = iPtr->flags;
	    iPtr->flags    |= INTERP_TRACE_IN_PROGRESS;
	    tcmdPtr->flags |= TCL_TRACE_EXEC_IN_PROGRESS;
	    tcmdPtr->refCount++;

	    /*
	     * This line can have quite arbitrary side-effects, including
	     * deleting the trace, the command being traced, or even the
	     * interpreter.
	     */

	    traceCode = Tcl_EvalEx(interp, Tcl_DStringValue(&cmd),
		    Tcl_DStringLength(&cmd), 0);
	    tcmdPtr->flags &= ~TCL_TRACE_EXEC_IN_PROGRESS;

	    /*
	     * Restore the interp tracing flag to prevent cmd traces from
	     * affecting interp traces.
	     */

	    iPtr->flags = saveInterpFlags;
	    if (tcmdPtr->flags == 0) {
		flags |= TCL_TRACE_DESTROYED;
	    }
	    Tcl_DStringFree(&cmd);
	}

	/*
	 * Third, if there are any step execution traces for this proc, we
	 * register an interpreter trace to invoke enterstep and/or leavestep
	 * traces. We also need to save the current stack level and the proc
	 * string in startLevel and startCmd so that we can delete this
	 * interpreter trace when it reaches the end of this proc.
	 */

	if ((flags & TCL_TRACE_ENTER_EXEC) && (tcmdPtr->stepTrace == NULL)
		&& (tcmdPtr->flags & (TCL_TRACE_ENTER_DURING_EXEC |
			TCL_TRACE_LEAVE_DURING_EXEC))) {
	    register unsigned len = strlen(command) + 1;

	    tcmdPtr->startLevel = level;
	    tcmdPtr->startCmd = ckalloc(len);
	    memcpy(tcmdPtr->startCmd, command, len);
	    tcmdPtr->refCount++;
	    tcmdPtr->stepTrace = Tcl_CreateObjTrace(interp, 0,
		   (tcmdPtr->flags & TCL_TRACE_ANY_EXEC) >> 2,
		   TraceExecutionProc, tcmdPtr, CommandObjTraceDeleted);
	}
    }
    if (flags & TCL_TRACE_DESTROYED) {
	if (tcmdPtr->stepTrace != NULL) {
	    Tcl_DeleteTrace(interp, tcmdPtr->stepTrace);
	    tcmdPtr->stepTrace = NULL;
	    if (tcmdPtr->startCmd != NULL) {
		ckfree(tcmdPtr->startCmd);
	    }
	}
    }
    if (call) {
	if (tcmdPtr->refCount-- <= 1) {
	    ckfree(tcmdPtr);
	}
    }
    return traceCode;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceVarProc --
 *
 *	This function is called to handle variable accesses that have been
 *	traced using the "trace" command.
 *
 * Results:
 *	Normally returns NULL. If the trace command returns an error, then
 *	this function returns an error string.
 *
 * Side effects:
 *	Depends on the command associated with the trace.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
TraceVarProc(
    ClientData clientData,	/* Information about the variable trace. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Name of variable or array. */
    const char *name2,		/* Name of element within array; NULL means
				 * scalar variable is being referenced. */
    int flags)			/* OR-ed bits giving operation and other
				 * information. */
{
    TraceVarInfo *tvarPtr = clientData;
    char *result;
    int code, destroy = 0;
    Tcl_DString cmd;
    int rewind = ((Interp *)interp)->execEnvPtr->rewind;

    /*
     * We might call Tcl_Eval() below, and that might evaluate [trace vdelete]
     * which might try to free tvarPtr. We want to use tvarPtr until the end
     * of this function, so we use Tcl_Preserve() and Tcl_Release() to be sure
     * it is not freed while we still need it.
     */

    result = NULL;
    if ((tvarPtr->flags & flags) && !Tcl_InterpDeleted(interp)
	    && !Tcl_LimitExceeded(interp)) {
	if (tvarPtr->length != (size_t) 0) {
	    /*
	     * Generate a command to execute by appending list elements for
	     * the two variable names and the operation.
	     */

	    Tcl_DStringInit(&cmd);
	    Tcl_DStringAppend(&cmd, tvarPtr->command, (int) tvarPtr->length);
	    Tcl_DStringAppendElement(&cmd, name1);
	    Tcl_DStringAppendElement(&cmd, (name2 ? name2 : ""));
#ifndef TCL_REMOVE_OBSOLETE_TRACES
	    if (tvarPtr->flags & TCL_TRACE_OLD_STYLE) {
		if (flags & TCL_TRACE_ARRAY) {
		    TclDStringAppendLiteral(&cmd, " a");
		} else if (flags & TCL_TRACE_READS) {
		    TclDStringAppendLiteral(&cmd, " r");
		} else if (flags & TCL_TRACE_WRITES) {
		    TclDStringAppendLiteral(&cmd, " w");
		} else if (flags & TCL_TRACE_UNSETS) {
		    TclDStringAppendLiteral(&cmd, " u");
		}
	    } else {
#endif
		if (flags & TCL_TRACE_ARRAY) {
		    TclDStringAppendLiteral(&cmd, " array");
		} else if (flags & TCL_TRACE_READS) {
		    TclDStringAppendLiteral(&cmd, " read");
		} else if (flags & TCL_TRACE_WRITES) {
		    TclDStringAppendLiteral(&cmd, " write");
		} else if (flags & TCL_TRACE_UNSETS) {
		    TclDStringAppendLiteral(&cmd, " unset");
		}
#ifndef TCL_REMOVE_OBSOLETE_TRACES
	    }
#endif

	    /*
	     * Execute the command. We discard any object result the command
	     * returns.
	     *
	     * Add the TCL_TRACE_DESTROYED flag to tvarPtr to indicate to
	     * other areas that this will be destroyed by us, otherwise a
	     * double-free might occur depending on what the eval does.
	     */

	    if ((flags & TCL_TRACE_DESTROYED)
		    && !(tvarPtr->flags & TCL_TRACE_DESTROYED)) {
		destroy = 1;
		tvarPtr->flags |= TCL_TRACE_DESTROYED;
	    }

	    /*
	     * Make sure that unset traces are rune even if the execEnv is
	     * rewinding (coroutine deletion, [Bug 2093947]
	     */

	    if (rewind && (flags & TCL_TRACE_UNSETS)) {
		((Interp *)interp)->execEnvPtr->rewind = 0;
	    }
	    code = Tcl_EvalEx(interp, Tcl_DStringValue(&cmd),
		    Tcl_DStringLength(&cmd), 0);
	    if (rewind) {
		((Interp *)interp)->execEnvPtr->rewind = rewind;
	    }
	    if (code != TCL_OK) {		/* copy error msg to result */
		Tcl_Obj *errMsgObj = Tcl_GetObjResult(interp);

		Tcl_IncrRefCount(errMsgObj);
		result = (char *) errMsgObj;
	    }
	    Tcl_DStringFree(&cmd);
	}
    }
    if (destroy && result != NULL) {
	register Tcl_Obj *errMsgObj = (Tcl_Obj *) result;

	Tcl_DecrRefCount(errMsgObj);
	result = NULL;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateObjTrace --
 *
 *	Arrange for a function to be called to trace command execution.
 *
 * Results:
 *	The return value is a token for the trace, which may be passed to
 *	Tcl_DeleteTrace to eliminate the trace.
 *
 * Side effects:
 *	From now on, proc will be called just before a command function is
 *	called to execute a Tcl command. Calls to proc will have the following
 *	form:
 *
 *	void proc(ClientData	 clientData,
 *		  Tcl_Interp *	 interp,
 *		  int		 level,
 *		  const char *	 command,
 *		  Tcl_Command	 commandInfo,
 *		  int		 objc,
 *		  Tcl_Obj *const objv[]);
 *
 *	The 'clientData' and 'interp' arguments to 'proc' will be the same as
 *	the arguments to Tcl_CreateObjTrace. The 'level' argument gives the
 *	nesting depth of command interpretation within the interpreter. The
 *	'command' argument is the ASCII text of the command being evaluated -
 *	before any substitutions are performed. The 'commandInfo' argument
 *	gives a handle to the command procedure that will be evaluated. The
 *	'objc' and 'objv' parameters give the parameter vector that will be
 *	passed to the command procedure. Proc does not return a value.
 *
 *	It is permissible for 'proc' to call Tcl_SetCommandTokenInfo to change
 *	the command procedure or client data for the command being evaluated,
 *	and these changes will take effect with the current evaluation.
 *
 *	The 'level' argument specifies the maximum nesting level of calls to
 *	be traced. If the execution depth of the interpreter exceeds 'level',
 *	the trace callback is not executed.
 *
 *	The 'flags' argument is either zero or the value,
 *	TCL_ALLOW_INLINE_COMPILATION. If the TCL_ALLOW_INLINE_COMPILATION flag
 *	is not present, the bytecode compiler will not generate inline code
 *	for Tcl's built-in commands. This behavior will have a significant
 *	impact on performance, but will ensure that all command evaluations
 *	are traced. If the TCL_ALLOW_INLINE_COMPILATION flag is present, the
 *	bytecode compiler will have its normal behavior of compiling in-line
 *	code for some of Tcl's built-in commands. In this case, the tracing
 *	will be imprecise - in-line code will not be traced - but run-time
 *	performance will be improved. The latter behavior is desired for many
 *	applications such as profiling of run time.
 *
 *	When the trace is deleted, the 'delProc' function will be invoked,
 *	passing it the original client data.
 *
 *----------------------------------------------------------------------
 */

Tcl_Trace
Tcl_CreateObjTrace(
    Tcl_Interp *interp,		/* Tcl interpreter */
    int level,			/* Maximum nesting level */
    int flags,			/* Flags, see above */
    Tcl_CmdObjTraceProc *proc,	/* Trace callback */
    ClientData clientData,	/* Client data for the callback */
    Tcl_CmdObjTraceDeleteProc *delProc)
				/* Function to call when trace is deleted */
{
    register Trace *tracePtr;
    register Interp *iPtr = (Interp *) interp;

    /*
     * Test if this trace allows inline compilation of commands.
     */

    if (!(flags & TCL_ALLOW_INLINE_COMPILATION)) {
	if (iPtr->tracesForbiddingInline == 0) {
	    /*
	     * When the first trace forbidding inline compilation is created,
	     * invalidate existing compiled code for this interpreter and
	     * arrange (by setting the DONT_COMPILE_CMDS_INLINE flag) that
	     * when compiling new code, no commands will be compiled inline
	     * (i.e., into an inline sequence of instructions). We do this
	     * because commands that were compiled inline will never result in
	     * a command trace being called.
	     */

	    iPtr->compileEpoch++;
	    iPtr->flags |= DONT_COMPILE_CMDS_INLINE;
	}
	iPtr->tracesForbiddingInline++;
    }

    tracePtr = ckalloc(sizeof(Trace));
    tracePtr->level = level;
    tracePtr->proc = proc;
    tracePtr->clientData = clientData;
    tracePtr->delProc = delProc;
    tracePtr->nextPtr = iPtr->tracePtr;
    tracePtr->flags = flags;
    iPtr->tracePtr = tracePtr;

    return (Tcl_Trace) tracePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateTrace --
 *
 *	Arrange for a function to be called to trace command execution.
 *
 * Results:
 *	The return value is a token for the trace, which may be passed to
 *	Tcl_DeleteTrace to eliminate the trace.
 *
 * Side effects:
 *	From now on, proc will be called just before a command procedure is
 *	called to execute a Tcl command. Calls to proc will have the following
 *	form:
 *
 *	void
 *	proc(clientData, interp, level, command, cmdProc, cmdClientData,
 *		argc, argv)
 *	    ClientData clientData;
 *	    Tcl_Interp *interp;
 *	    int level;
 *	    char *command;
 *	    int (*cmdProc)();
 *	    ClientData cmdClientData;
 *	    int argc;
 *	    char **argv;
 *	{
 *	}
 *
 *	The clientData and interp arguments to proc will be the same as the
 *	corresponding arguments to this function. Level gives the nesting
 *	level of command interpretation for this interpreter (0 corresponds to
 *	top level). Command gives the ASCII text of the raw command, cmdProc
 *	and cmdClientData give the function that will be called to process the
 *	command and the ClientData value it will receive, and argc and argv
 *	give the arguments to the command, after any argument parsing and
 *	substitution. Proc does not return a value.
 *
 *----------------------------------------------------------------------
 */

Tcl_Trace
Tcl_CreateTrace(
    Tcl_Interp *interp,		/* Interpreter in which to create trace. */
    int level,			/* Only call proc for commands at nesting
				 * level<=argument level (1=>top level). */
    Tcl_CmdTraceProc *proc,	/* Function to call before executing each
				 * command. */
    ClientData clientData)	/* Arbitrary value word to pass to proc. */
{
    StringTraceData *data = ckalloc(sizeof(StringTraceData));

    data->clientData = clientData;
    data->proc = proc;
    return Tcl_CreateObjTrace(interp, level, 0, StringTraceProc,
	    data, StringTraceDeleteProc);
}

/*
 *----------------------------------------------------------------------
 *
 * StringTraceProc --
 *
 *	Invoke a string-based trace function from an object-based callback.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the string-based trace function does.
 *
 *----------------------------------------------------------------------
 */

static int
StringTraceProc(
    ClientData clientData,
    Tcl_Interp *interp,
    int level,
    const char *command,
    Tcl_Command commandInfo,
    int objc,
    Tcl_Obj *const *objv)
{
    StringTraceData *data = clientData;
    Command *cmdPtr = (Command *) commandInfo;
    const char **argv;		/* Args to pass to string trace proc */
    int i;

    /*
     * This is a bit messy because we have to emulate the old trace interface,
     * which uses strings for everything.
     */

    argv = (const char **) TclStackAlloc(interp,
	    (unsigned) ((objc + 1) * sizeof(const char *)));
    for (i = 0; i < objc; i++) {
	argv[i] = Tcl_GetString(objv[i]);
    }
    argv[objc] = 0;

    /*
     * Invoke the command function. Note that we cast away const-ness on two
     * parameters for compatibility with legacy code; the code MUST NOT modify
     * either command or argv.
     */

    data->proc(data->clientData, interp, level, (char *) command,
	    cmdPtr->proc, cmdPtr->clientData, objc, argv);
    TclStackFree(interp, (void *) argv);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringTraceDeleteProc --
 *
 *	Clean up memory when a string-based trace is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocated memory is returned to the system.
 *
 *----------------------------------------------------------------------
 */

static void
StringTraceDeleteProc(
    ClientData clientData)
{
    ckfree(clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteTrace --
 *
 *	Remove a trace.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on there will be no more calls to the function given in
 *	trace.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteTrace(
    Tcl_Interp *interp,		/* Interpreter that contains trace. */
    Tcl_Trace trace)		/* Token for trace (returned previously by
				 * Tcl_CreateTrace). */
{
    Interp *iPtr = (Interp *) interp;
    Trace *prevPtr, *tracePtr = (Trace *) trace;
    register Trace **tracePtr2 = &iPtr->tracePtr;
    ActiveInterpTrace *activePtr;

    /*
     * Locate the trace entry in the interpreter's trace list, and remove it
     * from the list.
     */

    prevPtr = NULL;
    while (*tracePtr2 != NULL && *tracePtr2 != tracePtr) {
	prevPtr = *tracePtr2;
	tracePtr2 = &prevPtr->nextPtr;
    }
    if (*tracePtr2 == NULL) {
	return;
    }
    *tracePtr2 = (*tracePtr2)->nextPtr;

    /*
     * The code below makes it possible to delete traces while traces are
     * active: it makes sure that the deleted trace won't be processed by
     * TclCheckInterpTraces.
     */

    for (activePtr = iPtr->activeInterpTracePtr;  activePtr != NULL;
	    activePtr = activePtr->nextPtr) {
	if (activePtr->nextTracePtr == tracePtr) {
	    if (activePtr->reverseScan) {
		activePtr->nextTracePtr = prevPtr;
	    } else {
		activePtr->nextTracePtr = tracePtr->nextPtr;
	    }
	}
    }

    /*
     * If the trace forbids bytecode compilation, change the interpreter's
     * state. If bytecode compilation is now permitted, flag the fact and
     * advance the compilation epoch so that procs will be recompiled to take
     * advantage of it.
     */

    if (!(tracePtr->flags & TCL_ALLOW_INLINE_COMPILATION)) {
	iPtr->tracesForbiddingInline--;
	if (iPtr->tracesForbiddingInline == 0) {
	    iPtr->flags &= ~DONT_COMPILE_CMDS_INLINE;
	    iPtr->compileEpoch++;
	}
    }

    /*
     * Execute any delete callback.
     */

    if (tracePtr->delProc != NULL) {
	tracePtr->delProc(tracePtr->clientData);
    }

    /*
     * Delete the trace object.
     */

    Tcl_EventuallyFree((char *) tracePtr, TCL_DYNAMIC);
}

/*
 *----------------------------------------------------------------------
 *
 * TclTraceVarExists --
 *
 *	This is called from info exists. We need to trigger read and/or array
 *	traces because they may end up creating a variable that doesn't
 *	currently exist.
 *
 * Results:
 *	A pointer to the Var structure, or NULL.
 *
 * Side effects:
 *	May fill in error messages in the interp.
 *
 *----------------------------------------------------------------------
 */

Var *
TclVarTraceExists(
    Tcl_Interp *interp,		/* The interpreter */
    const char *varName)	/* The variable name */
{
    Var *varPtr, *arrayPtr;

    /*
     * The choice of "create" flag values is delicate here, and matches the
     * semantics of GetVar. Things are still not perfect, however, because if
     * you do "info exists x" you get a varPtr and therefore trigger traces.
     * However, if you do "info exists x(i)", then you only get a varPtr if x
     * is already known to be an array. Otherwise you get NULL, and no trace
     * is triggered. This matches Tcl 7.6 semantics.
     */

    varPtr = TclLookupVar(interp, varName, NULL, 0, "access",
	    /*createPart1*/ 0, /*createPart2*/ 1, &arrayPtr);

    if (varPtr == NULL) {
	return NULL;
    }

    if ((varPtr->flags & VAR_TRACED_READ)
	    || (arrayPtr && (arrayPtr->flags & VAR_TRACED_READ))) {
	TclCallVarTraces((Interp *) interp, arrayPtr, varPtr, varName, NULL,
		TCL_TRACE_READS, /* leaveErrMsg */ 0);
    }

    /*
     * If the variable doesn't exist anymore and no-one's using it, then free
     * up the relevant structures and hash table entries.
     */

    if (TclIsVarUndefined(varPtr)) {
	TclCleanupVar(varPtr, arrayPtr);
	return NULL;
    }

    return varPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCheckArrayTraces --
 *
 *	This function is invoked to when we operate on an array variable,
 *	to allow any array traces to fire.
 *
 * Results:
 *	Returns TCL_OK to indicate normal operation. Returns TCL_ERROR if
 *	invocation of a trace function indicated an error. When TCL_ERROR is
 *	returned, then error information is left in interp.
 *
 * Side effects:
 *	Almost anything can happen, depending on trace; this function itself
 *	doesn't have any side effects.
 *
 *----------------------------------------------------------------------
 */

int
TclCheckArrayTraces(
    Tcl_Interp *interp,
    Var *varPtr,
    Var *arrayPtr,
    Tcl_Obj *name,
    int index)
{
    int code = TCL_OK;

    if (varPtr && (varPtr->flags & VAR_TRACED_ARRAY)
	&& (TclIsVarArray(varPtr) || TclIsVarUndefined(varPtr))) {
	Interp *iPtr = (Interp *)interp;

	code = TclObjCallVarTraces(iPtr, arrayPtr, varPtr, name, NULL,
		(TCL_NAMESPACE_ONLY|TCL_GLOBAL_ONLY| TCL_TRACE_ARRAY),
		/* leaveErrMsg */ 1, index);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCallVarTraces --
 *
 *	This function is invoked to find and invoke relevant trace functions
 *	associated with a particular operation on a variable. This function
 *	invokes traces both on the variable and on its containing array (where
 *	relevant).
 *
 * Results:
 *	Returns TCL_OK to indicate normal operation. Returns TCL_ERROR if
 *	invocation of a trace function indicated an error. When TCL_ERROR is
 *	returned and leaveErrMsg is true, then the errorInfo field of iPtr has
 *	information about the error placed in it.
 *
 * Side effects:
 *	Almost anything can happen, depending on trace; this function itself
 *	doesn't have any side effects.
 *
 *----------------------------------------------------------------------
 */

int
TclObjCallVarTraces(
    Interp *iPtr,		/* Interpreter containing variable. */
    register Var *arrayPtr,	/* Pointer to array variable that contains the
				 * variable, or NULL if the variable isn't an
				 * element of an array. */
    Var *varPtr,		/* Variable whose traces are to be invoked. */
    Tcl_Obj *part1Ptr,
    Tcl_Obj *part2Ptr,		/* Variable's two-part name. */
    int flags,			/* Flags passed to trace functions: indicates
				 * what's happening to variable, plus maybe
				 * TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY */
    int leaveErrMsg,		/* If true, and one of the traces indicates an
				 * error, then leave an error message and
				 * stack trace information in *iPTr. */
    int index)			/* Index into the local variable table of the
				 * variable, or -1. Only used when part1Ptr is
				 * NULL. */
{
    const char *part1, *part2;

    if (!part1Ptr) {
	part1Ptr = localName(iPtr->varFramePtr, index);
    }
    if (!part1Ptr) {
	Tcl_Panic("Cannot trace a variable with no name");
    }
    part1 = TclGetString(part1Ptr);
    part2 = part2Ptr? TclGetString(part2Ptr) : NULL;

    return TclCallVarTraces(iPtr, arrayPtr, varPtr, part1, part2, flags,
	    leaveErrMsg);
}

int
TclCallVarTraces(
    Interp *iPtr,		/* Interpreter containing variable. */
    register Var *arrayPtr,	/* Pointer to array variable that contains the
				 * variable, or NULL if the variable isn't an
				 * element of an array. */
    Var *varPtr,		/* Variable whose traces are to be invoked. */
    const char *part1,
    const char *part2,		/* Variable's two-part name. */
    int flags,			/* Flags passed to trace functions: indicates
				 * what's happening to variable, plus maybe
				 * TCL_GLOBAL_ONLY or TCL_NAMESPACE_ONLY */
    int leaveErrMsg)		/* If true, and one of the traces indicates an
				 * error, then leave an error message and
				 * stack trace information in *iPTr. */
{
    register VarTrace *tracePtr;
    ActiveVarTrace active;
    char *result;
    const char *openParen, *p;
    Tcl_DString nameCopy;
    int copiedName;
    int code = TCL_OK;
    int disposeFlags = 0;
    Tcl_InterpState state = NULL;
    Tcl_HashEntry *hPtr;
    int traceflags = flags & VAR_ALL_TRACES;

    /*
     * If there are already similar trace functions active for the variable,
     * don't call them again.
     */

    if (TclIsVarTraceActive(varPtr)) {
	return code;
    }
    TclSetVarTraceActive(varPtr);
    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)++;
    }
    if (arrayPtr && TclIsVarInHash(arrayPtr)) {
	VarHashRefCount(arrayPtr)++;
    }

    /*
     * If the variable name hasn't been parsed into array name and element, do
     * it here. If there really is an array element, make a copy of the
     * original name so that NULLs can be inserted into it to separate the
     * names (can't modify the name string in place, because the string might
     * get used by the callbacks we invoke).
     */

    copiedName = 0;
    if (part2 == NULL) {
	for (p = part1; *p ; p++) {
	    if (*p == '(') {
		openParen = p;
		do {
		    p++;
		} while (*p != '\0');
		p--;
		if (*p == ')') {
		    int offset = (openParen - part1);
		    char *newPart1;

		    Tcl_DStringInit(&nameCopy);
		    Tcl_DStringAppend(&nameCopy, part1, p-part1);
		    newPart1 = Tcl_DStringValue(&nameCopy);
		    newPart1[offset] = 0;
		    part1 = newPart1;
		    part2 = newPart1 + offset + 1;
		    copiedName = 1;
		}
		break;
	    }
	}
    }

    /*
     * Ignore any caller-provided TCL_INTERP_DESTROYED flag.  Only we can
     * set it correctly.
     */

    flags &= ~TCL_INTERP_DESTROYED;

    /*
     * Invoke traces on the array containing the variable, if relevant.
     */

    result = NULL;
    active.nextPtr = iPtr->activeVarTracePtr;
    iPtr->activeVarTracePtr = &active;
    Tcl_Preserve(iPtr);
    if (arrayPtr && !TclIsVarTraceActive(arrayPtr)
	    && (arrayPtr->flags & traceflags)) {
	hPtr = Tcl_FindHashEntry(&iPtr->varTraces, (char *) arrayPtr);
	active.varPtr = arrayPtr;
	for (tracePtr = Tcl_GetHashValue(hPtr);
		tracePtr != NULL; tracePtr = active.nextTracePtr) {
	    active.nextTracePtr = tracePtr->nextPtr;
	    if (!(tracePtr->flags & flags)) {
		continue;
	    }
	    Tcl_Preserve(tracePtr);
	    if (state == NULL) {
		state = Tcl_SaveInterpState((Tcl_Interp *) iPtr, code);
	    }
	    if (Tcl_InterpDeleted((Tcl_Interp *) iPtr)) {
		flags |= TCL_INTERP_DESTROYED;
	    }
	    result = tracePtr->traceProc(tracePtr->clientData,
		    (Tcl_Interp *) iPtr, part1, part2, flags);
	    if (result != NULL) {
		if (flags & TCL_TRACE_UNSETS) {
		    /*
		     * Ignore errors in unset traces.
		     */

		    DisposeTraceResult(tracePtr->flags, result);
		} else {
		    disposeFlags = tracePtr->flags;
		    code = TCL_ERROR;
		}
	    }
	    Tcl_Release(tracePtr);
	    if (code == TCL_ERROR) {
		goto done;
	    }
	}
    }

    /*
     * Invoke traces on the variable itself.
     */

    if (flags & TCL_TRACE_UNSETS) {
	flags |= TCL_TRACE_DESTROYED;
    }
    active.varPtr = varPtr;
    if (varPtr->flags & traceflags) {
	hPtr = Tcl_FindHashEntry(&iPtr->varTraces, (char *) varPtr);
	for (tracePtr = Tcl_GetHashValue(hPtr);
		tracePtr != NULL; tracePtr = active.nextTracePtr) {
	    active.nextTracePtr = tracePtr->nextPtr;
	    if (!(tracePtr->flags & flags)) {
		continue;
	    }
	    Tcl_Preserve(tracePtr);
	    if (state == NULL) {
		state = Tcl_SaveInterpState((Tcl_Interp *) iPtr, code);
	    }
	    if (Tcl_InterpDeleted((Tcl_Interp *) iPtr)) {
		flags |= TCL_INTERP_DESTROYED;
	    }
	    result = tracePtr->traceProc(tracePtr->clientData,
		    (Tcl_Interp *) iPtr, part1, part2, flags);
	    if (result != NULL) {
		if (flags & TCL_TRACE_UNSETS) {
		    /*
		     * Ignore errors in unset traces.
		     */

		    DisposeTraceResult(tracePtr->flags, result);
		} else {
		    disposeFlags = tracePtr->flags;
		    code = TCL_ERROR;
		}
	    }
	    Tcl_Release(tracePtr);
	    if (code == TCL_ERROR) {
		goto done;
	    }
	}
    }

    /*
     * Restore the variable's flags, remove the record of our active traces,
     * and then return.
     */

  done:
    if (code == TCL_ERROR) {
	if (leaveErrMsg) {
	    const char *verb = "";
	    const char *type = "";

	    switch (flags&(TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_ARRAY)) {
	    case TCL_TRACE_READS:
		verb = "read";
		type = verb;
		break;
	    case TCL_TRACE_WRITES:
		verb = "set";
		type = "write";
		break;
	    case TCL_TRACE_ARRAY:
		verb = "trace array";
		type = "array";
		break;
	    }

	    if (disposeFlags & TCL_TRACE_RESULT_OBJECT) {
		Tcl_SetObjResult((Tcl_Interp *)iPtr, (Tcl_Obj *) result);
	    } else {
		Tcl_SetObjResult((Tcl_Interp *)iPtr,
			Tcl_NewStringObj(result, -1));
	    }
	    Tcl_AddErrorInfo((Tcl_Interp *)iPtr, "");

	    Tcl_AppendObjToErrorInfo((Tcl_Interp *)iPtr, Tcl_ObjPrintf(
		    "\n    (%s trace on \"%s%s%s%s\")", type, part1,
		    (part2 ? "(" : ""), (part2 ? part2 : ""),
		    (part2 ? ")" : "") ));
	    if (disposeFlags & TCL_TRACE_RESULT_OBJECT) {
		TclVarErrMsg((Tcl_Interp *) iPtr, part1, part2, verb,
			Tcl_GetString((Tcl_Obj *) result));
	    } else {
		TclVarErrMsg((Tcl_Interp *) iPtr, part1, part2, verb, result);
	    }
	    iPtr->flags &= ~(ERR_ALREADY_LOGGED);
	    Tcl_DiscardInterpState(state);
	} else {
	    Tcl_RestoreInterpState((Tcl_Interp *) iPtr, state);
	}
	DisposeTraceResult(disposeFlags,result);
    } else if (state) {
	if (code == TCL_OK) {
	    code = Tcl_RestoreInterpState((Tcl_Interp *) iPtr, state);
	} else {
	    Tcl_DiscardInterpState(state);
	}
    }

    if (arrayPtr && TclIsVarInHash(arrayPtr)) {
	VarHashRefCount(arrayPtr)--;
    }
    if (copiedName) {
	Tcl_DStringFree(&nameCopy);
    }
    TclClearVarTraceActive(varPtr);
    if (TclIsVarInHash(varPtr)) {
	VarHashRefCount(varPtr)--;
    }
    iPtr->activeVarTracePtr = active.nextPtr;
    Tcl_Release(iPtr);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * DisposeTraceResult--
 *
 *	This function is called to dispose of the result returned from a trace
 *	function. The disposal method appropriate to the type of result is
 *	determined by flags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The memory allocated for the trace result may be freed.
 *
 *----------------------------------------------------------------------
 */

static void
DisposeTraceResult(
    int flags,			/* Indicates type of result to determine
				 * proper disposal method. */
    char *result)		/* The result returned from a trace function
				 * to be disposed. */
{
    if (flags & TCL_TRACE_RESULT_DYNAMIC) {
	ckfree(result);
    } else if (flags & TCL_TRACE_RESULT_OBJECT) {
	Tcl_DecrRefCount((Tcl_Obj *) result);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UntraceVar --
 *
 *	Remove a previously-created trace for a variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there exists a trace for the variable given by varName with the
 *	given flags, proc, and clientData, then that trace is removed.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_UntraceVar
void
Tcl_UntraceVar(
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *varName,	/* Name of variable; may end with "(index)" to
				 * signify an array reference. */
    int flags,			/* OR-ed collection of bits describing current
				 * trace, including any of TCL_TRACE_READS,
				 * TCL_TRACE_WRITES, TCL_TRACE_UNSETS,
				 * TCL_GLOBAL_ONLY and TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc,	/* Function assocated with trace. */
    ClientData clientData)	/* Arbitrary argument to pass to proc. */
{
    Tcl_UntraceVar2(interp, varName, NULL, flags, proc, clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UntraceVar2 --
 *
 *	Remove a previously-created trace for a variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there exists a trace for the variable given by part1 and part2 with
 *	the given flags, proc, and clientData, then that trace is removed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_UntraceVar2(
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *part1,		/* Name of variable or array. */
    const char *part2,		/* Name of element within array; NULL means
				 * trace applies to scalar variable or array
				 * as-a-whole. */
    int flags,			/* OR-ed collection of bits describing current
				 * trace, including any of TCL_TRACE_READS,
				 * TCL_TRACE_WRITES, TCL_TRACE_UNSETS,
				 * TCL_GLOBAL_ONLY, and TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc,	/* Function assocated with trace. */
    ClientData clientData)	/* Arbitrary argument to pass to proc. */
{
    register VarTrace *tracePtr;
    VarTrace *prevPtr, *nextPtr;
    Var *varPtr, *arrayPtr;
    Interp *iPtr = (Interp *) interp;
    ActiveVarTrace *activePtr;
    int flagMask, allFlags = 0;
    Tcl_HashEntry *hPtr;

    /*
     * Set up a mask to mask out the parts of the flags that we are not
     * interested in now.
     */

    flagMask = TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY;
    varPtr = TclLookupVar(interp, part1, part2, flags & flagMask, /*msg*/ NULL,
	    /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
    if (varPtr == NULL || !(varPtr->flags & VAR_ALL_TRACES & flags)) {
	return;
    }

    /*
     * Set up a mask to mask out the parts of the flags that we are not
     * interested in now.
     */

    flagMask = TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS |
	  TCL_TRACE_ARRAY | TCL_TRACE_RESULT_DYNAMIC | TCL_TRACE_RESULT_OBJECT;
#ifndef TCL_REMOVE_OBSOLETE_TRACES
    flagMask |= TCL_TRACE_OLD_STYLE;
#endif
    flags &= flagMask;

    hPtr = Tcl_FindHashEntry(&iPtr->varTraces, (char *) varPtr);
    for (tracePtr = Tcl_GetHashValue(hPtr), prevPtr = NULL; ;
	    prevPtr = tracePtr, tracePtr = tracePtr->nextPtr) {
	if (tracePtr == NULL) {
	    goto updateFlags;
	}
	if ((tracePtr->traceProc == proc) && (tracePtr->flags == flags)
		&& (tracePtr->clientData == clientData)) {
	    break;
	}
	allFlags |= tracePtr->flags;
    }

    /*
     * The code below makes it possible to delete traces while traces are
     * active: it makes sure that the deleted trace won't be processed by
     * TclCallVarTraces.
     *
     * Caveat (Bug 3062331): When an unset trace handler on a variable
     * tries to delete a different unset trace handler on the same variable,
     * the results may be surprising.  When variable unset traces fire, the
     * traced variable is already gone.  So the TclLookupVar() call above
     * will not find that variable, and not finding it will never reach here
     * to perform the deletion.  This means callers of Tcl_UntraceVar*()
     * attempting to delete unset traces from within the handler of another
     * unset trace have to account for the possibility that their call to
     * Tcl_UntraceVar*() is a no-op.
     */

    for (activePtr = iPtr->activeVarTracePtr;  activePtr != NULL;
	    activePtr = activePtr->nextPtr) {
	if (activePtr->nextTracePtr == tracePtr) {
	    activePtr->nextTracePtr = tracePtr->nextPtr;
	}
    }
    nextPtr = tracePtr->nextPtr;
    if (prevPtr == NULL) {
	if (nextPtr) {
	    Tcl_SetHashValue(hPtr, nextPtr);
	} else {
	    Tcl_DeleteHashEntry(hPtr);
	}
    } else {
	prevPtr->nextPtr = nextPtr;
    }
    tracePtr->nextPtr = NULL;
    Tcl_EventuallyFree(tracePtr, TCL_DYNAMIC);

    for (tracePtr = nextPtr; tracePtr != NULL;
	    tracePtr = tracePtr->nextPtr) {
	allFlags |= tracePtr->flags;
    }

  updateFlags:
    varPtr->flags &= ~VAR_ALL_TRACES;
    if (allFlags & VAR_ALL_TRACES) {
	varPtr->flags |= (allFlags & VAR_ALL_TRACES);
    } else if (TclIsVarUndefined(varPtr)) {
	/*
	 * If this is the last trace on the variable, and the variable is
	 * unset and unused, then free up the variable.
	 */

	TclCleanupVar(varPtr, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VarTraceInfo --
 *
 *	Return the clientData value associated with a trace on a variable.
 *	This function can also be used to step through all of the traces on a
 *	particular variable that have the same trace function.
 *
 * Results:
 *	The return value is the clientData value associated with a trace on
 *	the given variable. Information will only be returned for a trace with
 *	proc as trace function. If the clientData argument is NULL then the
 *	first such trace is returned; otherwise, the next relevant one after
 *	the one given by clientData will be returned. If the variable doesn't
 *	exist, or if there are no (more) traces for it, then NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_VarTraceInfo
ClientData
Tcl_VarTraceInfo(
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *varName,	/* Name of variable; may end with "(index)" to
				 * signify an array reference. */
    int flags,			/* OR-ed combo or TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY (can be 0). */
    Tcl_VarTraceProc *proc,	/* Function assocated with trace. */
    ClientData prevClientData)	/* If non-NULL, gives last value returned by
				 * this function, so this call will return the
				 * next trace after that one. If NULL, this
				 * call will return the first trace. */
{
    return Tcl_VarTraceInfo2(interp, varName, NULL, flags, proc,
	    prevClientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VarTraceInfo2 --
 *
 *	Same as Tcl_VarTraceInfo, except takes name in two pieces instead of
 *	one.
 *
 * Results:
 *	Same as Tcl_VarTraceInfo.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_VarTraceInfo2(
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *part1,		/* Name of variable or array. */
    const char *part2,		/* Name of element within array; NULL means
				 * trace applies to scalar variable or array
				 * as-a-whole. */
    int flags,			/* OR-ed combination of TCL_GLOBAL_ONLY,
				 * TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc,	/* Function assocated with trace. */
    ClientData prevClientData)	/* If non-NULL, gives last value returned by
				 * this function, so this call will return the
				 * next trace after that one. If NULL, this
				 * call will return the first trace. */
{
    Interp *iPtr = (Interp *) interp;
    Var *varPtr, *arrayPtr;
    Tcl_HashEntry *hPtr;

    varPtr = TclLookupVar(interp, part1, part2,
	    flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY), /*msg*/ NULL,
	    /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
    if (varPtr == NULL) {
	return NULL;
    }

    /*
     * Find the relevant trace, if any, and return its clientData.
     */

    hPtr = Tcl_FindHashEntry(&iPtr->varTraces, (char *) varPtr);

    if (hPtr) {
	register VarTrace *tracePtr = Tcl_GetHashValue(hPtr);

	if (prevClientData != NULL) {
	    for (; tracePtr != NULL; tracePtr = tracePtr->nextPtr) {
		if ((tracePtr->clientData == prevClientData)
			&& (tracePtr->traceProc == proc)) {
		    tracePtr = tracePtr->nextPtr;
		    break;
		}
	    }
	}
	for (; tracePtr != NULL ; tracePtr = tracePtr->nextPtr) {
	    if (tracePtr->traceProc == proc) {
		return tracePtr->clientData;
	    }
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceVar --
 *
 *	Arrange for reads and/or writes to a variable to cause a function to
 *	be invoked, which can monitor the operations and/or change their
 *	actions.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	A trace is set up on the variable given by varName, such that future
 *	references to the variable will be intermediated by proc. See the
 *	manual entry for complete details on the calling sequence for proc.
 *     The variable's flags are updated.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_TraceVar
int
Tcl_TraceVar(
    Tcl_Interp *interp,		/* Interpreter in which variable is to be
				 * traced. */
    const char *varName,	/* Name of variable; may end with "(index)" to
				 * signify an array reference. */
    int flags,			/* OR-ed collection of bits, including any of
				 * TCL_TRACE_READS, TCL_TRACE_WRITES,
				 * TCL_TRACE_UNSETS, TCL_GLOBAL_ONLY, and
				 * TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc,	/* Function to call when specified ops are
				 * invoked upon varName. */
    ClientData clientData)	/* Arbitrary argument to pass to proc. */
{
    return Tcl_TraceVar2(interp, varName, NULL, flags, proc, clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TraceVar2 --
 *
 *	Arrange for reads and/or writes to a variable to cause a function to
 *	be invoked, which can monitor the operations and/or change their
 *	actions.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	A trace is set up on the variable given by part1 and part2, such that
 *	future references to the variable will be intermediated by proc. See
 *	the manual entry for complete details on the calling sequence for
 *	proc. The variable's flags are updated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_TraceVar2(
    Tcl_Interp *interp,		/* Interpreter in which variable is to be
				 * traced. */
    const char *part1,		/* Name of scalar variable or array. */
    const char *part2,		/* Name of element within array; NULL means
				 * trace applies to scalar variable or array
				 * as-a-whole. */
    int flags,			/* OR-ed collection of bits, including any of
				 * TCL_TRACE_READS, TCL_TRACE_WRITES,
				 * TCL_TRACE_UNSETS, TCL_GLOBAL_ONLY, and
				 * TCL_NAMESPACE_ONLY. */
    Tcl_VarTraceProc *proc,	/* Function to call when specified ops are
				 * invoked upon varName. */
    ClientData clientData)	/* Arbitrary argument to pass to proc. */
{
    register VarTrace *tracePtr;
    int result;

    tracePtr = ckalloc(sizeof(VarTrace));
    tracePtr->traceProc = proc;
    tracePtr->clientData = clientData;
    tracePtr->flags = flags;

    result = TraceVarEx(interp, part1, part2, tracePtr);

    if (result != TCL_OK) {
	ckfree(tracePtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TraceVarEx --
 *
 *	Arrange for reads and/or writes to a variable to cause a function to
 *	be invoked, which can monitor the operations and/or change their
 *	actions.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	A trace is set up on the variable given by part1 and part2, such that
 *	future references to the variable will be intermediated by the
 *	traceProc listed in tracePtr. See the manual entry for complete
 *	details on the calling sequence for proc.
 *
 *----------------------------------------------------------------------
 */

static int
TraceVarEx(
    Tcl_Interp *interp,		/* Interpreter in which variable is to be
				 * traced. */
    const char *part1,		/* Name of scalar variable or array. */
    const char *part2,		/* Name of element within array; NULL means
				 * trace applies to scalar variable or array
				 * as-a-whole. */
    register VarTrace *tracePtr)/* Structure containing flags, traceProc and
				 * clientData fields. Others should be left
				 * blank. Will be ckfree()d (eventually) if
				 * this function returns TCL_OK, and up to
				 * caller to free if this function returns
				 * TCL_ERROR. */
{
    Interp *iPtr = (Interp *) interp;
    Var *varPtr, *arrayPtr;
    int flagMask, isNew;
    Tcl_HashEntry *hPtr;

    /*
     * We strip 'flags' down to just the parts which are relevant to
     * TclLookupVar, to avoid conflicts between trace flags and internal
     * namespace flags such as 'TCL_FIND_ONLY_NS'. This can now occur since we
     * have trace flags with values 0x1000 and higher.
     */

    flagMask = TCL_GLOBAL_ONLY | TCL_NAMESPACE_ONLY;
    varPtr = TclLookupVar(interp, part1, part2,
	    (tracePtr->flags & flagMask) | TCL_LEAVE_ERR_MSG,
	    "trace", /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    if (varPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Check for a nonsense flag combination. Note that this is a Tcl_Panic()
     * because there should be no code path that ever sets both flags.
     */

    if ((tracePtr->flags & TCL_TRACE_RESULT_DYNAMIC)
	    && (tracePtr->flags & TCL_TRACE_RESULT_OBJECT)) {
	Tcl_Panic("bad result flag combination");
    }

    /*
     * Set up trace information.
     */

    flagMask = TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS |
	  TCL_TRACE_ARRAY | TCL_TRACE_RESULT_DYNAMIC | TCL_TRACE_RESULT_OBJECT;
#ifndef TCL_REMOVE_OBSOLETE_TRACES
    flagMask |= TCL_TRACE_OLD_STYLE;
#endif
    tracePtr->flags = tracePtr->flags & flagMask;

    hPtr = Tcl_CreateHashEntry(&iPtr->varTraces, varPtr, &isNew);
    if (isNew) {
	tracePtr->nextPtr = NULL;
    } else {
	tracePtr->nextPtr = Tcl_GetHashValue(hPtr);
    }
    Tcl_SetHashValue(hPtr, tracePtr);

    /*
     * Mark the variable as traced so we know to call them.
     */

    varPtr->flags |= (tracePtr->flags & VAR_ALL_TRACES);

    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
