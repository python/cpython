
/*	$Id: tixHLInd.c,v 1.2 2004/03/28 02:44:56 hobbs Exp $	*/

/*
 *  tixHLInd.c ---
 *
 *	Implements indicators inside tixHList widgets
 *
 * Copyright (c) 1994-1995 Ioi Kim Lam. All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixHList.h>

static TIX_DECLARE_SUBCMD(Tix_HLIndCreate);
static TIX_DECLARE_SUBCMD(Tix_HLIndConfig);
static TIX_DECLARE_SUBCMD(Tix_HLIndCGet);
static TIX_DECLARE_SUBCMD(Tix_HLIndDelete);
static TIX_DECLARE_SUBCMD(Tix_HLIndExists);
static TIX_DECLARE_SUBCMD(Tix_HLIndSize);


/*----------------------------------------------------------------------
 * "indicator" sub command
 *----------------------------------------------------------------------
 */
int
Tix_HLIndicator(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    static Tix_SubCmdInfo subCmdInfo[] = {
	{TIX_DEFAULT_LEN, "cget", 2, 2, Tix_HLIndCGet,
	   "entryPath option"},
	{TIX_DEFAULT_LEN, "configure", 1, TIX_VAR_ARGS, Tix_HLIndConfig,
	   "entryPath ?option? ?value ...?"},
	{TIX_DEFAULT_LEN, "create", 1, TIX_VAR_ARGS, Tix_HLIndCreate,
	   "entryPath ?option value ...?"},
	{TIX_DEFAULT_LEN, "delete", 1, 1, Tix_HLIndDelete,
	   "entryPath"},
	{TIX_DEFAULT_LEN, "exists", 1, 1, Tix_HLIndExists,
	   "entryPath"},
	{TIX_DEFAULT_LEN, "size", 1, 1, Tix_HLIndSize,
	   "entryPath"},
    };
    static Tix_CmdInfo cmdInfo = {
	Tix_ArraySize(subCmdInfo), 1, TIX_VAR_ARGS, "?option? ?arg ...?",
    };

    return Tix_HandleSubCmds(&cmdInfo, subCmdInfo, clientData,
	interp, argc+1, argv-1);
}

/*----------------------------------------------------------------------
 * "indicator cget" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLIndCGet(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if (chPtr->indicator == NULL) {
	Tcl_AppendResult(interp, "entry \"", argv[0],
	    "\" does not have an indicator", (char*)NULL); 
	return TCL_ERROR;
    }
    return Tk_ConfigureValue(interp, wPtr->dispData.tkwin, 
	chPtr->indicator->base.diTypePtr->itemConfigSpecs,
	(char *)chPtr->indicator, argv[1], 0);
}

/*----------------------------------------------------------------------
 * "indicator configure" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLIndConfig(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if (chPtr->indicator == NULL) {
	Tcl_AppendResult(interp, "entry \"", argv[0],
	    "\" does not have an indicator", (char*)NULL); 
	return TCL_ERROR;
    }
    if (argc == 1) {
	return Tk_ConfigureInfo(interp, wPtr->dispData.tkwin, 
	    chPtr->indicator->base.diTypePtr->itemConfigSpecs,
	    (char *)chPtr->indicator, NULL, 0);
    } else if (argc == 2) {
	return Tk_ConfigureInfo(interp, wPtr->dispData.tkwin, 
	    chPtr->indicator->base.diTypePtr->itemConfigSpecs,
	    (char *)chPtr->indicator, argv[1], 0);
    } else {
	Tix_HLMarkElementDirty(wPtr, chPtr);
	Tix_HLResizeWhenIdle(wPtr);

	return Tix_DItemConfigure(chPtr->indicator,
	    argc-1, argv+1, TK_CONFIG_ARGV_ONLY);
    }
}

/*----------------------------------------------------------------------
 * "indicator create" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLIndCreate(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    int i;
    size_t len;
    Tix_DItem * iPtr;
    CONST84 char * ditemType = NULL;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if ((argc %2) == 0) {
	Tcl_AppendResult(interp, "value for \"", argv[argc-1],
	    "\" missing", NULL);
	return TCL_ERROR;
    }
    for (i=1; i<argc; i+=2) {
	len = strlen(argv[i]);
	if (strncmp(argv[i], "-itemtype", len) == 0) {
	    ditemType = argv[i+1];
	}
    }
    if (ditemType == NULL) {
	ditemType = wPtr->diTypePtr->name;
    }

    iPtr = Tix_DItemCreate(&wPtr->dispData, ditemType);
    if (iPtr == NULL) {
	return TCL_ERROR;
    }
    if (Tix_DItemType(iPtr) == TIX_DITEM_WINDOW) {
	wPtr->needToRaise = 1;
    }

    iPtr->base.clientData = (ClientData)chPtr;
    if (Tix_DItemConfigure(iPtr, argc-1, argv+1, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    if (chPtr->indicator != NULL) {
	if (Tix_DItemType(chPtr->indicator) == TIX_DITEM_WINDOW) {
	    Tix_WindowItemListRemove(&wPtr->mappedWindows,
		chPtr->indicator);
	}
	Tix_DItemFree(chPtr->indicator);
    }
    chPtr->indicator = iPtr;
    Tix_HLMarkElementDirty(wPtr, chPtr);
    Tix_HLResizeWhenIdle(wPtr);

    return TCL_OK;
}
/*----------------------------------------------------------------------
 * "indicator delete" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLIndDelete(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if (chPtr->indicator == NULL) {
	Tcl_AppendResult(interp, "entry \"", argv[0],
	    "\" does not have an indicator", (char*)NULL); 
	return TCL_ERROR;
    }

    if (Tix_DItemType(chPtr->indicator) == TIX_DITEM_WINDOW) {
	Tix_WindowItemListRemove(&wPtr->mappedWindows,
	    chPtr->indicator);
    }

    /* Free the item and leave a blank! */

    Tix_DItemFree(chPtr->indicator);
    chPtr->indicator = NULL;

    Tix_HLMarkElementDirty(wPtr, chPtr);
    Tix_HLResizeWhenIdle(wPtr);

    return TCL_OK;
}
/*----------------------------------------------------------------------
 * "indicator exists" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLIndExists(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if (chPtr->indicator == NULL) {
	Tcl_AppendResult(interp, "0", NULL);
    } else {
	Tcl_AppendResult(interp, "1", NULL);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * "indicator size" sub command
 *----------------------------------------------------------------------
 */
static int
Tix_HLIndSize(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    WidgetPtr wPtr = (WidgetPtr) clientData;
    HListElement * chPtr;
    char buff[100];

    if ((chPtr = Tix_HLFindElement(interp, wPtr, argv[0])) == NULL) {
	return TCL_ERROR;
    }
    if (chPtr->indicator == NULL) {
	Tcl_AppendResult(interp, "entry \"", argv[0],
	    "\" does not have an indicator", (char*)NULL); 
	return TCL_ERROR;
    }
    sprintf(buff, "%d %d",
	Tix_DItemWidth(chPtr->indicator),
	Tix_DItemHeight(chPtr->indicator));
    Tcl_AppendResult(interp, buff, NULL);
    return TCL_OK;
}
