/*
 * tclHistory.c --
 *
 *	This module and the Tcl library file history.tcl together implement
 *	Tcl command history. Tcl_RecordAndEval(Obj) can be called to record
 *	commands ("events") before they are executed. Commands defined in
 *	history.tcl may be used to perform history substitutions.
 *
 * Copyright (c) 1990-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Type of the assocData structure used to hold the reference to the [history
 * add] subcommand, used in Tcl_RecordAndEvalObj.
 */

typedef struct {
    Tcl_Obj *historyObj;	/* == "::history" */
    Tcl_Obj *addObj;		/* == "add" */
} HistoryObjs;

#define HISTORY_OBJS_KEY	"::tcl::HistoryObjs"

/*
 * Static functions in this file.
 */

static Tcl_InterpDeleteProc DeleteHistoryObjs;

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RecordAndEval --
 *
 *	This procedure adds its command argument to the current list of
 *	recorded events and then executes the command by calling Tcl_Eval.
 *
 * Results:
 *	The return value is a standard Tcl return value, the result of
 *	executing cmd.
 *
 * Side effects:
 *	The command is recorded and executed.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RecordAndEval(
    Tcl_Interp *interp,		/* Token for interpreter in which command will
				 * be executed. */
    const char *cmd,		/* Command to record. */
    int flags)			/* Additional flags. TCL_NO_EVAL means only
				 * record: don't execute command.
				 * TCL_EVAL_GLOBAL means use Tcl_GlobalEval
				 * instead of Tcl_Eval. */
{
    register Tcl_Obj *cmdPtr;
    int length = strlen(cmd);
    int result;

    if (length > 0) {
	/*
	 * Call Tcl_RecordAndEvalObj to do the actual work.
	 */

	cmdPtr = Tcl_NewStringObj(cmd, length);
	Tcl_IncrRefCount(cmdPtr);
	result = Tcl_RecordAndEvalObj(interp, cmdPtr, flags);

	/*
	 * Move the interpreter's object result to the string result, then
	 * reset the object result.
	 */

	(void) Tcl_GetStringResult(interp);

	/*
	 * Discard the Tcl object created to hold the command.
	 */

	Tcl_DecrRefCount(cmdPtr);
    } else {
	/*
	 * An empty string. Just reset the interpreter's result.
	 */

	Tcl_ResetResult(interp);
	result = TCL_OK;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RecordAndEvalObj --
 *
 *	This procedure adds the command held in its argument object to the
 *	current list of recorded events and then executes the command by
 *	calling Tcl_EvalObj.
 *
 * Results:
 *	The return value is a standard Tcl return value, the result of
 *	executing the command.
 *
 * Side effects:
 *	The command is recorded and executed.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RecordAndEvalObj(
    Tcl_Interp *interp,		/* Token for interpreter in which command will
				 * be executed. */
    Tcl_Obj *cmdPtr,		/* Points to object holding the command to
				 * record and execute. */
    int flags)			/* Additional flags. TCL_NO_EVAL means record
				 * only: don't execute the command.
				 * TCL_EVAL_GLOBAL means evaluate the script
				 * in global variable context instead of the
				 * current procedure. */
{
    int result, call = 1;
    Tcl_CmdInfo info;
    HistoryObjs *histObjsPtr =
	    Tcl_GetAssocData(interp, HISTORY_OBJS_KEY, NULL);

    /*
     * Create the references to the [::history add] command if necessary.
     */

    if (histObjsPtr == NULL) {
	histObjsPtr = ckalloc(sizeof(HistoryObjs));
	TclNewLiteralStringObj(histObjsPtr->historyObj, "::history");
	TclNewLiteralStringObj(histObjsPtr->addObj, "add");
	Tcl_IncrRefCount(histObjsPtr->historyObj);
	Tcl_IncrRefCount(histObjsPtr->addObj);
	Tcl_SetAssocData(interp, HISTORY_OBJS_KEY, DeleteHistoryObjs,
		histObjsPtr);
    }

    /*
     * Do not call [history] if it has been replaced by an empty proc
     */

    result = Tcl_GetCommandInfo(interp, "::history", &info);
    if (result && (info.deleteProc == TclProcDeleteProc)) {
	Proc *procPtr = (Proc *) info.objClientData;
	call = (procPtr->cmdPtr->compileProc != TclCompileNoOp);
    }

    if (call) {
	Tcl_Obj *list[3];

	/*
	 * Do recording by eval'ing a tcl history command: history add $cmd.
	 */

	list[0] = histObjsPtr->historyObj;
	list[1] = histObjsPtr->addObj;
	list[2] = cmdPtr;

	Tcl_IncrRefCount(cmdPtr);
	(void) Tcl_EvalObjv(interp, 3, list, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdPtr);

	/*
	 * One possible failure mode above: exceeding a resource limit.
	 */

	if (Tcl_LimitExceeded(interp)) {
	    return TCL_ERROR;
	}
    }

    /*
     * Execute the command.
     */

    result = TCL_OK;
    if (!(flags & TCL_NO_EVAL)) {
	result = Tcl_EvalObjEx(interp, cmdPtr, flags & TCL_EVAL_GLOBAL);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteHistoryObjs --
 *
 *	Called to delete the references to the constant words used when adding
 *	to the history.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The constant words may be deleted.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteHistoryObjs(
    ClientData clientData,
    Tcl_Interp *interp)
{
    register HistoryObjs *histObjsPtr = clientData;

    TclDecrRefCount(histObjsPtr->historyObj);
    TclDecrRefCount(histObjsPtr->addObj);
    ckfree(histObjsPtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
