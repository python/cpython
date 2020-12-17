/*
 * tkUnixDialog.c --
 *
 *	Contains the Unix implementation of the common dialog boxes:
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"

/*
 * The wrapper code for Unix is actually set up in library/tk.tcl these days;
 * the procedure names used here are probably wrong too...
 */

#ifdef TK_OBSOLETE_UNIX_DIALOG_WRAPPERS

/*
 *----------------------------------------------------------------------
 *
 * EvalObjv --
 *
 *	Invokes the Tcl procedure with the arguments.
 *
 * Results:
 *	Returns the result of the evaluation of the command.
 *
 * Side effects:
 *	The command may be autoloaded.
 *
 *----------------------------------------------------------------------
 */

static int
EvalObjv(
    Tcl_Interp *interp,		/* Current interpreter. */
    char *cmdName,		/* Name of the TCL command to call */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments. */
{
    Tcl_Obj *cmdObj, **objs;
    int result;

    cmdObj = Tcl_NewStringObj(cmdName, -1);
    Tcl_IncrRefCount(cmdObj);
    objs = ckalloc(sizeof(Tcl_Obj *) * (objc+1));
    objs[0] = cmdObj;
    memcpy(objs+1, objv, sizeof(Tcl_Obj *) * (unsigned)objc);

    result = Tcl_EvalObjv(interp, objc+1, objs, 0);

    Tcl_DecrRefCount(cmdObj);
    ckfree(objs);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseColorObjCmd --
 *
 *	This procedure implements the color dialog box for the Unix platform.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A dialog window is created the first time this procedure is called.
 *	This window is not destroyed and will be reused the next time the
 *	application invokes the "tk_chooseColor" command.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ChooseColorObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments. */
{
    return EvalObjv(interp, "tk::ColorDialog", objc-1, objv+1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetOpenFileCmd --
 *
 *	This procedure implements the "open file" dialog box for the Unix
 *	platform. See the user documentation for details on what it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A dialog window is created the first this procedure is called. This
 *	window is not destroyed and will be reused the next time the
 *	application invokes the "tk_getOpenFile" or "tk_getSaveFile" command.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetOpenFileObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments. */
{
    Tk_Window tkwin = clientData;

    if (Tk_StrictMotif(tkwin)) {
	return EvalObjv(interp, "tk::MotifOpenFDialog", objc-1, objv+1);
    } else {
	return EvalObjv(interp, "tk::OpenFDialog", objc-1, objv+1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetSaveFileCmd --
 *
 *	Same as Tk_GetOpenFileCmd but opens a "save file" dialog box instead.
 *
 * Results:
 *	Same as Tk_GetOpenFileCmd.
 *
 * Side effects:
 *	Same as Tk_GetOpenFileCmd.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetSaveFileObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments. */
{
    Tk_Window tkwin = clientData;

    if (Tk_StrictMotif(tkwin)) {
	return EvalObjv(interp, "tk::MotifSaveFDialog", objc-1, objv+1);
    } else {
	return EvalObjv(interp, "tk::SaveFDialog", objc-1, objv+1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MessageBoxCmd --
 *
 *	This procedure implements the MessageBox window for the Unix
 *	platform. See the user documentation for details on what it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	None. The MessageBox window will be destroy before this procedure
 *	returns.
 *
 *----------------------------------------------------------------------
 */

int
Tk_MessageBoxCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Arguments. */
{
    return EvalObjv(interp, "tk::MessageBox", objc-1, objv+1);
}

#endif /* TK_OBSOLETE_UNIX_DIALOG_WRAPPERS */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
