/*
 * Copyright 2003, Joe English
 *
 * Simplified interface to Tcl_TraceVariable.
 *
 * PROBLEM: Can't distinguish "variable does not exist" (which is OK) 
 * from other errors (which are not).
 */

#include <tk.h>
#include "ttkTheme.h"
#include "ttkWidget.h"

struct TtkTraceHandle_
{
    Tcl_Interp		*interp;	/* Containing interpreter */
    Tcl_Obj 		*varnameObj;	/* Name of variable being traced */
    Ttk_TraceProc	callback;	/* Callback procedure */
    void		*clientData;	/* Data to pass to callback */
};

/*
 * Tcl_VarTraceProc for trace handles.
 */
static char *
VarTraceProc(
    ClientData clientData,	/* Widget record pointer */
    Tcl_Interp *interp, 	/* Interpreter containing variable. */
    const char *name1,		/* (unused) */
    const char *name2,		/* (unused) */
    int flags)			/* Information about what happened. */
{
    Ttk_TraceHandle *tracePtr = clientData;
    const char *name, *value;
    Tcl_Obj *valuePtr;

    if (Tcl_InterpDeleted(interp)) {
	return NULL;
    }

    name = Tcl_GetString(tracePtr->varnameObj);

    /*
     * If the variable is being unset, then re-establish the trace:
     */
    if (flags & TCL_TRACE_DESTROYED) {
	/*
	 * If a prior call to Ttk_UntraceVariable() left behind an
	 * indicator that we wanted this handler to be deleted (see below),
	 * cleanup the ClientData bits and exit.
	 */
	if (tracePtr->interp == NULL) {
	    Tcl_DecrRefCount(tracePtr->varnameObj);
	    ckfree((ClientData)tracePtr);
	    return NULL;
	}
	Tcl_TraceVar2(interp, name, NULL,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		VarTraceProc, clientData);
	tracePtr->callback(tracePtr->clientData, NULL);
	return NULL;
    }

    /*
     * Call the callback:
     */
    valuePtr = Tcl_GetVar2Ex(interp, name, NULL, TCL_GLOBAL_ONLY);
    value = valuePtr ?  Tcl_GetString(valuePtr) : NULL;
    tracePtr->callback(tracePtr->clientData, value);

    return NULL;
}

/* Ttk_TraceVariable(interp, varNameObj, callback, clientdata) --
 * 	Attach a write trace to the specified variable,
 * 	which will pass the variable's value to 'callback'
 * 	whenever the variable is set.
 *
 * 	When the variable is unset, passes NULL to the callback
 * 	and reattaches the trace.
 */
Ttk_TraceHandle *Ttk_TraceVariable(
    Tcl_Interp *interp,
    Tcl_Obj *varnameObj,
    Ttk_TraceProc callback,
    void *clientData)
{
    Ttk_TraceHandle *h = ckalloc(sizeof(*h));
    int status;

    h->interp = interp;
    h->varnameObj = Tcl_DuplicateObj(varnameObj);
    Tcl_IncrRefCount(h->varnameObj);
    h->clientData = clientData;
    h->callback = callback;

    status = Tcl_TraceVar2(interp, Tcl_GetString(varnameObj),
	    NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	    VarTraceProc, (ClientData)h);

    if (status != TCL_OK) {
	Tcl_DecrRefCount(h->varnameObj);
	ckfree(h);
	return NULL;
    }

    return h;
}

/*
 * Ttk_UntraceVariable --
 * 	Remove previously-registered trace and free the handle.
 */
void Ttk_UntraceVariable(Ttk_TraceHandle *h)
{
    if (h) {
	ClientData cd = NULL;

	/*
	 * Workaround for Tcl Bug 3062331.  The trace design problem is
	 * that when variable unset traces fire, Tcl documents that the
	 * traced variable has already been unset.  It's already gone.
	 * So from within an unset trace, if you try to call
	 * Tcl_UntraceVar() on that variable, it will do nothing, because
	 * the variable by that name can no longer be found.  It's gone.
	 * This means callers of Tcl_UntraceVar() that might be running
	 * in response to an unset trace have to handle the possibility
	 * that their Tcl_UntraceVar() call will do nothing.  In this case,
	 * we have to support the possibility that Tcl_UntraceVar() will
	 * leave the trace in place, so we need to leave the ClientData
	 * untouched so when that trace does fire it will not crash.
	 */

	/*
	 * Search the traces on the variable to see if the one we are tasked
	 * with removing is present.
	 */
	while ((cd = Tcl_VarTraceInfo(h->interp, Tcl_GetString(h->varnameObj),
		TCL_GLOBAL_ONLY, VarTraceProc, cd)) != NULL) {
	    if (cd == (ClientData) h) {
		break;
	    }
	}
	/*
	 * If the trace we wish to delete is not visible, Tcl_UntraceVar
	 * will do nothing, so don't try to call it.  Instead set an
	 * indicator in the Ttk_TraceHandle that we need to cleanup later.
	 */
	if (cd == NULL) {
	    h->interp = NULL;
	    return;
	}
	Tcl_UntraceVar2(h->interp, Tcl_GetString(h->varnameObj),
		NULL, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		VarTraceProc, (ClientData)h);
	Tcl_DecrRefCount(h->varnameObj);
	ckfree(h);
    }
}

/*
 * Ttk_FireTrace --
 * 	Executes a trace handle as if the variable has been written.
 *
 * 	Note: may reenter the interpreter.
 */
int Ttk_FireTrace(Ttk_TraceHandle *tracePtr)
{
    Tcl_Interp *interp = tracePtr->interp;
    void *clientData = tracePtr->clientData;
    const char *name = Tcl_GetString(tracePtr->varnameObj);
    Ttk_TraceProc callback = tracePtr->callback;
    Tcl_Obj *valuePtr;
    const char *value;

    /* Read the variable.
     * Note that this can reenter the interpreter, and anything can happen --
     * including the current trace handle being freed!
     */
    valuePtr = Tcl_GetVar2Ex(interp, name, NULL, TCL_GLOBAL_ONLY);
    value = valuePtr ? Tcl_GetString(valuePtr) : NULL;

    /* Call callback.
     */
    callback(clientData, value);

    return TCL_OK;
}

/*EOF*/
