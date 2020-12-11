/*
 * tixWidget.c --
 *
 *	Constructs Tix-based mega widgets
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixWidget.c,v 1.7 2008/02/28 04:29:17 hobbs Exp $
 */

#include <tclInt.h>
#include <tixInt.h>

static int	ParseOptions(Tcl_Interp *interp, TixClassRecord *cPtr,
	CONST84 char *widRec, int argc, CONST84 char** argv);

TIX_DECLARE_CMD(Tix_InstanceCmd);

/*----------------------------------------------------------------------
 * Tix_CreateWidgetCmd
 *
 * 	Create an instance object of a Tix widget class.
 *
 * argv[0]  = object name.
 * argv[1+] = args
 *----------------------------------------------------------------------
 */
TIX_DEFINE_CMD(Tix_CreateWidgetCmd)
{
    TixClassRecord * cPtr =(TixClassRecord *)clientData;
    TixConfigSpec * spec;
    CONST84 char * value;
    CONST84 char * widRec = NULL;
    char * widCmd = NULL, * rootCmd = NULL;
    int i;
    int code = TCL_OK;
    Tk_Window mainWin = Tk_MainWindow(interp);

    if (argc <= 1) {
	return Tix_ArgcError(interp, argc, argv, 1, "pathname ?arg? ...");
    } else {
	widRec = argv[1];
    }

    if (strstr(argv[1], "::") != NULL) {
        /*
         * Cannot contain :: in widget name, otherwise all hell will
         * rise w.r.t. namespace
         */

        Tcl_AppendResult(interp, "invalid widget name \"", argv[1],
		"\": may not contain substring \"::\"", NULL);
        return TCL_ERROR;
    }

    Tcl_ResetResult(interp);
    if (Tk_NameToWindow(interp, widRec, mainWin) != NULL) {
	Tcl_AppendResult(interp, "window name \"", widRec,
	    "\" already exists", NULL);
	return TCL_ERROR;
    }

    /*
     * Before doing anything, let's reset the TCL result, errorInfo,
     * errorCode, etc.
     */
    Tcl_SetVar2(interp, "errorInfo", NULL, "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, "errorCode", NULL, "", TCL_GLOBAL_ONLY);

    /*
     * Set up the widget record
     *
     * TODO: avoid buffer allocation if possible.
     */
    widCmd = ckalloc(strlen(widRec) + 3);
    sprintf(widCmd, "::%s", widRec);
    rootCmd = ckalloc(strlen(widRec) + 8);
    sprintf(rootCmd, "::%s:root", widRec);

    Tcl_SetVar2(interp, widRec, "className", cPtr->className, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, widRec, "ClassName", cPtr->ClassName, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, widRec, "context",   cPtr->className, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, widRec, "w:root",    widRec,  	      TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, widRec, "rootCmd",   rootCmd,         TCL_GLOBAL_ONLY);

    /* We need to create the root widget in order to parse the options
     * database
     */
    if (Tix_CallMethod(interp, cPtr->className, widRec, "CreateRootWidget",
	    argc-2, argv+2, NULL) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    /* Parse the options specified in the option database and supplied
     * in the command line.
     */
    Tcl_ResetResult(interp);
    if (ParseOptions(interp, cPtr, widRec, argc-2, argv+2) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    /* Rename the root widget command and create a new TCL command for
     * this widget
     */

    if (TclRenameCommand(interp, widCmd, rootCmd) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    Tcl_CreateCommand(interp, widRec, Tix_InstanceCmd,
	(ClientData)cPtr, NULL);

    /* Now call the initialization methods defined by the Tix Intrinsics
     */
    if (Tix_CallMethod(interp, cPtr->className, widRec, "InitWidgetRec",
	    0, 0, NULL) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    if (Tix_CallMethod(interp, cPtr->className, widRec, "ConstructWidget",
	    0, 0, NULL) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    if (Tix_CallMethod(interp, cPtr->className, widRec, "SetBindings",
	    0, 0, NULL) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    /* The widget has been successfully initialized. Now call the config
     * method for all -forceCall options
     */
    for (i=0; i<cPtr->nSpecs; i++) {
	spec = cPtr->specs[i];
	if (spec->forceCall) {
	    value = Tcl_GetVar2(interp, widRec, spec->argvName,
		TCL_GLOBAL_ONLY);
	    if (Tix_CallConfigMethod(interp, cPtr, widRec, spec,
		    value)!=TCL_OK){
		code = TCL_ERROR;
		goto done;
	    }
	}
    }

    Tcl_SetResult(interp, (char *) widRec, TCL_VOLATILE);

  done:

    if (code != TCL_OK) {
	Tk_Window topLevel, tkwin;
	Tcl_SavedResult state;

	/* We need to save the current interp state as it may be changed by
	 * some of the following function calls.
	 */
	Tcl_SaveResult(interp, &state);
	Tcl_ResetResult(interp);

	/* (1) window */
	topLevel = cPtr->mainWindow;

	if (widRec != NULL) {
	    Display *display = NULL;

	    tkwin = Tk_NameToWindow(interp, widRec, topLevel);
	    if (tkwin != NULL) {
		display = Tk_Display(tkwin);
		Tk_DestroyWindow(tkwin);
	    }

	    /*
             * (2) Clean up widget command + root command. Because widCmd
             *     and rootCmd contains ::, the commands will be correctly
             *     deleted from the global namespace.
             */

	    Tcl_DeleteCommand(interp, widCmd);
	    Tcl_DeleteCommand(interp, rootCmd);

	    /* (3) widget record */
	    Tcl_UnsetVar(interp, widRec, TCL_GLOBAL_ONLY);

	    if (display) {
#if !defined(__WIN32__) && !defined(MAC_TCL) && !defined(MAC_OSX_TK) /* UNIX */
                /* TODO: why is this necessary?? */
		XSync(display, False);
#endif
		while (1) {
		    if (Tk_DoOneEvent(TK_X_EVENTS|TK_DONT_WAIT) == 0) {
			break;
		    }
		}
	    }
	}
	Tcl_RestoreResult(interp, &state);
    }
    if (widCmd) {
	ckfree(widCmd);
    }
    if (rootCmd) {
	ckfree(rootCmd);
    }

    return code;
}

/*----------------------------------------------------------------------
 * Subroutines for object instantiation.
 *
 *
 *----------------------------------------------------------------------
 */
static int ParseOptions(interp, cPtr, widRec, argc, argv)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char *widRec;
    int argc;
    CONST84 char** argv;
{
    int i;
    TixConfigSpec *spec;
    Tk_Window tkwin;
    CONST84 char * value;

    if ((argc %2) != 0) {
	Tcl_AppendResult(interp, "missing argument for \"", argv[argc-1],
	    "\"", NULL);
	return TCL_ERROR;
    }

    if ((tkwin = Tk_NameToWindow(interp, widRec, cPtr->mainWindow)) == NULL) {
	return TCL_ERROR;
    }

    /* Set all specs by their default values */
    /* BUG: default value may be override by options database */
    for (i=0; i<cPtr->nSpecs; i++) {
	spec = cPtr->specs[i];

	if (!spec->isAlias) {
	    if ((value=Tk_GetOption(tkwin,spec->dbName,spec->dbClass))==NULL) {
		value = spec->defValue;
	    }
	    if (Tix_ChangeOneOption(interp, cPtr, widRec, spec,
		value, 1, 0)!=TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }

    /* Set specs according to argument line values */
    for (i=0; i<argc; i+=2) {
	spec = Tix_FindConfigSpecByName(interp, cPtr, argv[i]);

	if (spec == NULL) {	/* this is an invalid flag */
	    return TCL_ERROR;
	}
	
	if (Tix_ChangeOneOption(interp, cPtr, widRec, spec,
		argv[i+1], 0, 1)!=TCL_OK) {
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}
