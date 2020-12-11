
/*	$Id: tixGrRC.c,v 1.3 2004/03/28 02:44:56 hobbs Exp $	*/

/* 
 * tixGrRC.c --
 *
 *	This module handles "size" sub-commands.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>
#include <tixGrid.h>

static TIX_DECLARE_SUBCMD(Tix_GrRCSize);
EXTERN TIX_DECLARE_SUBCMD(Tix_GrSetSize);

int
Tix_GrSetSize(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    static Tix_SubCmdInfo subCmdInfo[] = {
	{TIX_DEFAULT_LEN, "row",    1, TIX_VAR_ARGS, Tix_GrRCSize,
	   "index ?option value ...?"},
	{TIX_DEFAULT_LEN, "column", 1, TIX_VAR_ARGS, Tix_GrRCSize,
	   "index ?option value ...?"},
    };
    static Tix_CmdInfo cmdInfo = {
	Tix_ArraySize(subCmdInfo), 1, TIX_VAR_ARGS, "option index ?arg ...?",
    };

    return Tix_HandleSubCmds(&cmdInfo, subCmdInfo, clientData,
	interp, argc+1, argv-1);
}


static int
Tix_GrRCSize(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    int which, index, code;
    char errorMsg[300];
    int changed;

    /* TODO: can the index be <0?? */
    if (argv[-1][0] == 'c') {
	which = 0;
    } else {
	which = 1;
    }
    if (Tcl_GetInt(interp, argv[0], &index) != TCL_OK) {
	size_t len = strlen(argv[0]);

	Tcl_ResetResult(interp);
	if (strncmp(argv[0], "default", len) != 0) {
	    Tcl_AppendResult(interp, "unknown option \"", argv[0],
		"\"; must be an integer or \"default\"", NULL);
	    return TCL_ERROR;
	} else {
	    /*
             * Set the default sizes
             */

            /* Buffer size checked: test=grrc-1.2 */
	    sprintf(errorMsg, "%s %s ?option value ...?", argv[-2], argv[-1]);

	    code = Tix_GrConfigSize(interp, wPtr, argc-1, argv+1,
		    &wPtr->defSize[which], errorMsg, &changed);

	    /* Handling special cases */
	    if (code == TCL_OK) {
		switch (wPtr->defSize[which].sizeType) {
		  case TIX_GR_DEFAULT:
		    wPtr->defSize[which].sizeType = TIX_GR_DEFINED_CHAR;
		    if (which == 0) {
			wPtr->defSize[which].charValue = 10.0;
		    } else {
			wPtr->defSize[which].charValue = 1.1;
		    }
		}

		switch (wPtr->defSize[which].sizeType) {
		  case TIX_GR_DEFINED_PIXEL:
		    wPtr->defSize[which].pixels=wPtr->defSize[which].sizeValue;
		    break;

		  case TIX_GR_DEFINED_CHAR:
		    wPtr->defSize[which].pixels =
		       (int)(wPtr->defSize[which].charValue *
			     wPtr->fontSize[which]);
		    break;
		}
	    }
	}
    } else {
        /* Buffer size checked: test=grrc-1.3 */
	sprintf(errorMsg, "%s %s ?option value ...?", argv[-2], argv[-1]);

	code = TixGridDataConfigRowColSize(interp, wPtr, wPtr->dataSet,
	    which, index, argc-1, argv+1, errorMsg, &changed);
    }

    if (changed) {
	Tix_GrDoWhenIdle(wPtr, TIX_GR_RESIZE);
    }

    return code;
}
