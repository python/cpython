
/*	$Id: tixFormMisc.c,v 1.2 2004/03/28 02:44:56 hobbs Exp $	*/

/*
 * tixFormMisc.c --
 *
 *	Implements the tixForm geometry manager, which has similar
 *	capability as the Motif Form geometry manager. Please
 *	refer to the documentation for the use of tixForm.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tix.h>
#include <tixForm.h>

/*
 * SubCommands of the tixForm command.
 */
TIX_DECLARE_SUBCMD(TixFm_Info);

static void 		AttachInfo _ANSI_ARGS_((Tcl_Interp * interp,
			    FormInfo * clientPtr, int axis, int which));
static int		ConfigureAttachment _ANSI_ARGS_((FormInfo *clientPtr,
			    Tk_Window topLevel, Tcl_Interp* interp,
			    int axis, int which, CONST84 char *value));
static int		ConfigureFill _ANSI_ARGS_((
			    FormInfo *clientPtr, Tk_Window tkwin,
			    Tcl_Interp* interp, CONST84 char *value));
static int		ConfigurePadding _ANSI_ARGS_((
			    FormInfo *clientPtr, Tk_Window tkwin,
			    Tcl_Interp* interp, int axis, int which,
			    CONST84 char *value));
static int		ConfigureSpring _ANSI_ARGS_((FormInfo *clientPtr,
			    Tk_Window topLevel, Tcl_Interp* interp,
			    int axis, int which, CONST84 char *value));


/*----------------------------------------------------------------------
 * TixFm_Info --
 *
 *	Return the information about the attachment of a client window
 *----------------------------------------------------------------------
 */
int TixFm_Info(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window topLevel = (Tk_Window) clientData;
    FormInfo * clientPtr;
    char buff[256];
    int i,j;
    static CONST84 char *sideNames[2][2] = {
	{"-left", "-right"},
	{"-top", "-bottom"}
    };
    static CONST84 char *padNames[2][2] = {
	{"-padleft", "-padright"},
	{"-padtop", "-padbottom"}
    };

    clientPtr = TixFm_FindClientPtrByName(interp, argv[0], topLevel);
    if (clientPtr == NULL) {
	return TCL_ERROR;
    }

    if (argc == 2) {
	/* user wants some specific info
	 */

	for (i=0; i<2; i++) {
	    for (j=0; j<2; j++) {
		/* Do you want to know attachment? */
		if (strcmp(argv[1], sideNames[i][j]) == 0) {
		    AttachInfo(interp, clientPtr, i, j);
		    return TCL_OK;
		}

		/* Do you want to know padding? */
		if (strcmp(argv[1], padNames[i][j]) == 0) {
		    sprintf(buff, "%d", clientPtr->pad[i][j]);
		    Tcl_AppendResult(interp, buff, NULL);
		    return TCL_OK;
		}
	    }
	}
	Tcl_AppendResult(interp, "Unknown option \"", argv[1], "\"", NULL);
	return TCL_ERROR;
    }

    /* Otherwise, give full info */

    for (i=0; i<2; i++) {
	for (j=0; j<2; j++) {
	    /* The information about attachment */
	    Tcl_AppendResult(interp, sideNames[i][j], " ", NULL);
	    AttachInfo(interp, clientPtr, i, j);

	    /* The information about padding */
	    Tcl_AppendResult(interp, padNames[i][j], " ", NULL);
	    sprintf(buff, "%d", clientPtr->pad[i][j]);
	    Tcl_AppendResult(interp, buff, " ", NULL);
	}
    }
    return TCL_OK;
}

static void AttachInfo(interp, clientPtr, axis, which)
    Tcl_Interp * interp;
    FormInfo * clientPtr;
    int axis;
    int which;
{
    char buff[256];

    switch(clientPtr->attType[axis][which]) {
      case ATT_NONE:
	Tcl_AppendElement(interp, "none");
	break;

      case ATT_GRID:
	sprintf(buff, "{%%%d %d}", clientPtr->att[axis][which].grid,
	    clientPtr->off[axis][which]);
	Tcl_AppendResult(interp, buff, " ", NULL);
	break;

      case ATT_OPPOSITE:
	sprintf(buff, "%d", clientPtr->off[axis][which]);
	Tcl_AppendResult(interp, "{",
	    Tk_PathName(clientPtr->att[axis][which].widget->tkwin),
	    " ", buff, "} ", NULL);
	break;

      case ATT_PARALLEL:
	sprintf(buff, "%d", clientPtr->off[axis][which]);
	Tcl_AppendResult(interp, "{&",
	    Tk_PathName(clientPtr->att[axis][which].widget->tkwin),
	    " ", buff, "} ", NULL);
	break;
    }
}

/*----------------------------------------------------------------------
 * Form Parameter Configuration
 *
 *----------------------------------------------------------------------
 */
static int ConfigureAttachment(clientPtr, topLevel, interp, axis, which, value)
    FormInfo *clientPtr;
    Tk_Window topLevel;
    Tcl_Interp* interp;
    int axis, which;
    CONST84 char *value;
{
    Tk_Window tkwin;
    FormInfo * attWidget;
    int code = TCL_OK;
    int offset;
    int grid;
    int argc;
    CONST84 char ** argv;

    if (Tcl_SplitList(interp, value, &argc, &argv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (argc < 1 || argc > 2) {
	Tcl_AppendResult(interp, "Malformed attachment value \"", value,
	    "\"", NULL);
	code = TCL_ERROR;
	goto done;
    }

    switch (argv[0][0]) {
      case '#':		/* Attached to grid */
      case '%': 	/* Attached to percent (aka grid) */
	if (Tcl_GetInt(interp, argv[0]+1, &grid) == TCL_ERROR) {
	    code = TCL_ERROR;
	    goto done;
	}
	clientPtr->attType[axis][which]   = ATT_GRID;
	clientPtr->att[axis][which].grid  = grid;
	break;

      case '&': 		/* Attached to parallel widget */
	tkwin = Tk_NameToWindow(interp, argv[0]+1, topLevel);

	if (tkwin != NULL) {
	    if (Tk_IsTopLevel(tkwin)) {
		Tcl_AppendResult(interp, "can't attach to \"", value,
	    	    "\": it's a top-level window", (char *) NULL);
		code = TCL_ERROR;
		goto done;
	    }
	    attWidget = TixFm_GetFormInfo(tkwin, 1);
	    TixFm_AddToMaster(clientPtr->master, attWidget);

	    clientPtr->attType[axis][which]    = ATT_PARALLEL;
	    clientPtr->att[axis][which].widget = attWidget;
	} else {
	    code = TCL_ERROR;
	    goto done;
	}
	break;

      case '.': 		/* Attach to opposite widget */
	tkwin = Tk_NameToWindow(interp, argv[0], topLevel);

	if (tkwin != NULL) {
	    if (Tk_IsTopLevel(tkwin)) {
		Tcl_AppendResult(interp, "can't attach to \"", value,
	    	    "\": it's a top-level window", (char *) NULL);
		code = TCL_ERROR;
		goto done;
	    }
	    attWidget = TixFm_GetFormInfo(tkwin, 1);
	    TixFm_AddToMaster(clientPtr->master, attWidget);

	    clientPtr->attType[axis][which]    = ATT_OPPOSITE;
	    clientPtr->att[axis][which].widget = attWidget;
	} else {
	    code = TCL_ERROR;
	    goto done;
	}
	break;

      case 'n':		/* none */
	if (argc == 1 && strcmp(argv[0], "none") == 0) {
	    clientPtr->attType[axis][which]    = ATT_NONE;
	    goto done;
	} else {
	    Tcl_AppendResult(interp, "Malformed attachment value \"", value,
		"\"", NULL);
	    code = TCL_ERROR;
	    goto done;
	}
	break;

      default:		/* Check if is attached to pixel */
	/* If there is only one value, this can be the offset with implicit
	 * anchor point 0% or max_grid%
	 */
	if (argc != 1) {
	    Tcl_AppendResult(interp, "Malformed attachment value \"", value,
		"\"", NULL);
	    code = TCL_ERROR;
	    goto done;
	}
#if 1
	if (Tk_GetPixels(interp, topLevel, argv[0], &offset) != TCL_OK) {
#else
	if (Tcl_GetInt(interp, argv[0], &offset) == TCL_ERROR) {
#endif
	    code = TCL_ERROR;
	    goto done;
	}

	clientPtr->attType[axis][which]      = ATT_GRID;
	clientPtr->off    [axis][which]      = offset;
	if (offset < 0 || (offset == 0 && strcmp(argv[0], "-0") ==0)) {
	    clientPtr->att[axis][which].grid = clientPtr->master->grids[axis];
	} else {
	    clientPtr->att[axis][which].grid = 0;
	}

	goto done;	/* We have already gotten both anchor and offset */
    }

    if (argc  == 2) {
#if 1
	if (Tk_GetPixels(interp, topLevel, argv[1], &offset) != TCL_OK) {
#else
	if (Tcl_GetInt(interp, argv[1], &offset) == TCL_ERROR) {
#endif
	    code = TCL_ERROR;
	    goto done;
	}
    	clientPtr->off[axis][which] = offset;
    } else {
	clientPtr->off[axis][which] = 0;
    }

  done:
    if (argv) {
	ckfree((char*) argv);
    }
    if (code == TCL_ERROR) {
	clientPtr->attType[axis][which] = ATT_NONE;
	clientPtr->off[axis][which] = 0;
    }
    return code;
}

static int ConfigurePadding(clientPtr, tkwin, interp, axis, which, value)
    FormInfo *clientPtr;
    Tk_Window tkwin;
    Tcl_Interp* interp;
    int axis, which;
    CONST84 char *value;
{
    int p_value;

    if (Tk_GetPixels(interp, tkwin, value, &p_value) != TCL_OK) {
	return TCL_ERROR;
    }
    else {
	clientPtr->pad[axis][which] = p_value;
	return TCL_OK;
    }
}

static int ConfigureFill(clientPtr, tkwin, interp, value)
    FormInfo *clientPtr;
    Tk_Window tkwin;
    Tcl_Interp* interp;
    CONST84 char *value;
{
    size_t len = strlen(value);

    if (strncmp(value, "x", len) == 0) {
	clientPtr->fill[AXIS_X] = 1;
	clientPtr->fill[AXIS_Y] = 0;
    }
    else if (strncmp(value, "y", len) == 0) {
	clientPtr->fill[AXIS_X] = 0;
	clientPtr->fill[AXIS_Y] = 1;
    }
    else if (strncmp(value, "both", len) == 0) {
	clientPtr->fill[AXIS_X] = 1;
	clientPtr->fill[AXIS_Y] = 1;
    }
    else if (strncmp(value, "none", len) == 0) {
	clientPtr->fill[AXIS_X] = 0;
	clientPtr->fill[AXIS_Y] = 0;
    }
    else {
	Tcl_AppendResult(interp, "bad fill style \"", value,
	    "\": must be none, x, y, or both", NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
}

static int ConfigureSpring(clientPtr, topLevel, interp, axis, which, value)
    FormInfo *clientPtr;
    Tk_Window topLevel;
    Tcl_Interp* interp;
    int axis, which;
    CONST84 char *value;
{
    int         strength;
    int		i = axis, j = which;

    if (Tcl_GetInt(interp, value, &strength) != TCL_OK) {
	return TCL_ERROR;
    }

    clientPtr->spring[i][j] = strength;

    if (clientPtr->attType[i][j] == ATT_OPPOSITE) {
	FormInfo * oppo;

	oppo = clientPtr->att[i][j].widget;
	oppo->spring[i][!j]  = strength;

	if (strength != 0 && clientPtr->strWidget[i][j] == NULL) {
	    clientPtr->strWidget[i][j] = oppo;

	    if (oppo->strWidget[i][!j] != clientPtr) {
		if (oppo->strWidget[i][!j] != NULL) {
		    oppo->strWidget[i][!j]->strWidget[i][j] = NULL;
		    oppo->strWidget[i][!j]->spring[i][j]  = 0;
		}
	    }
	    oppo->strWidget[i][!j] = clientPtr;
	}
    }

    return TCL_OK;
}

int TixFm_Configure(clientPtr, topLevel, interp, argc, argv)
    FormInfo *clientPtr;
    Tk_Window topLevel;
    Tcl_Interp* interp;
    int argc;
    CONST84 char **argv;
{
    int i, flag, value;

    for (i=0; i< argc; i+=2) {
	flag  = i;
	value = i+1;

	if (strcmp(argv[flag], "-in") == 0) {
	    /* Reset the parent of the widget
	     */
	    Tcl_AppendResult(interp,
		"\"-in \" must be the first option given to tixForm", NULL);
	    return TCL_ERROR;
	} else if (strcmp(argv[flag], "-l") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
		0, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-left") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
		0, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-r") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
		0, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-right") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
		0, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-top") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
	        1, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-t") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
	        1, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-bottom") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
		1, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-b") == 0) {
	    if (ConfigureAttachment(clientPtr, topLevel, interp,
		1, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-padx") == 0) {
	    if (ConfigurePadding(clientPtr, topLevel, interp,
  	        0, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		0, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-pady") == 0) {
	    if (ConfigurePadding(clientPtr,topLevel,interp,
		1, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		1, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-padleft") == 0) {
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		0, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-lp") == 0) {
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		0, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-padright")== 0){
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		0, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-rp")== 0){
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		0, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-padtop")== 0) {
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		1, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-tp")== 0) {
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		1, 0, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag],"-padbottom")== 0){
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		1, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag],"-bp")== 0){
	    if (ConfigurePadding(clientPtr, topLevel, interp,
		1, 1, argv[value]) == TCL_ERROR) {

		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-leftspring") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		0, 0, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-ls") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		0, 0, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-rightspring") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		0, 1, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-rs") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		0, 1, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-topspring") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		1, 0, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-ts") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		1, 0, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-bottomspring") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		1, 1, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-bs") == 0) {
	    if (ConfigureSpring(clientPtr, topLevel, interp,
		1, 1, argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[flag], "-fill") == 0) {
	    if (ConfigureFill(clientPtr, topLevel, interp,
		argv[value]) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	} else {
	    Tcl_AppendResult(interp, "Wrong option \"",
		argv[i], "\".", (char *) NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Clear the previously set default attachment if the opposide
     * edge is attached.
     */

#if 0
    /* (1) The X axis */
    if ((clientPtr->attType[0][0] ==  ATT_DEFAULT_PIXEL)
	&&(clientPtr->attType[0][1]  !=  ATT_NONE)) {
	clientPtr->attType[0][0]   = ATT_NONE;
    }

    /* (2) The Y axis */
    if ((clientPtr->attType[1][0] ==  ATT_DEFAULT_PIXEL)
	&&(clientPtr->attType[1][1]  !=  ATT_NONE)) {
	clientPtr->attType[1][0]   = ATT_NONE;
    }
#endif

    return TCL_OK;
}

