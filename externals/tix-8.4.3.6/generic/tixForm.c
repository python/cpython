
/*	$Id: tixForm.c,v 1.3 2004/03/28 02:44:56 hobbs Exp $	*/

/*
 * tixForm.c --
 *
 *	Implements the tixForm geometry manager, which has similar
 *	capability as the Motif Form geometry manager. Please
 *	refer to the documentation for the use of tixForm.
 *
 * 	This file implements the basic algorithm of tixForm.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/*
 *
 * ToDo:
 *
 *     (1) Delete the master structure when there is no more client to manage
 *
 * Possible bugs:
 * (1) a client is deleted but the master doesn't know 
 *     (clientPtr->tkwin == NULL)
 * (2) Whan a client S is deleted or detached from the master, all other
 *     clients attached to S must delete their reference to S
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixForm.h>
#define DEBUG 0


typedef struct SpringLink {
    struct SpringLink * next;
    FormInfo *clientPtr;
} SpringLink;


typedef struct SpringList { 
    SpringLink * head, * tail;
    int num;
} SpringList;


/*
 * SubCommands of the tixForm command.
 */
static TIX_DECLARE_SUBCMD(TixFm_SetGrid);
static TIX_DECLARE_SUBCMD(TixFm_SetClient);
static TIX_DECLARE_SUBCMD(TixFm_Check);
static TIX_DECLARE_SUBCMD(TixFm_Forget);
EXTERN TIX_DECLARE_SUBCMD(TixFm_Info);
static TIX_DECLARE_SUBCMD(TixFm_Slaves);
static TIX_DECLARE_SUBCMD(TixFm_Spring);

static void 		ArrangeGeometry _ANSI_ARGS_((ClientData clientData));
static void 		ArrangeWhenIdle _ANSI_ARGS_((MasterInfo * masterPtr));
static void		CancelArrangeWhenIdle _ANSI_ARGS_((
			    MasterInfo * masterPtr));
static void 		CalculateMasterSize _ANSI_ARGS_((MasterInfo *master));
static void 		CheckIntergrity _ANSI_ARGS_((FormInfo * clientPtr));
static MasterInfo * 	GetMasterInfo _ANSI_ARGS_((Tk_Window tkwin,
			    int create));
static void 		MasterStructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent * eventPtr));
static int 		PinnClient _ANSI_ARGS_((FormInfo *clientPtr));
static int 		PinnClientSide _ANSI_ARGS_((FormInfo *clientPtr,
			    int axis, int which, int isSelf));
static int 		PlaceClientSide _ANSI_ARGS_((FormInfo *clientPtr,
			    int axis, int which, int isSelf));
static int 		TestAndArrange _ANSI_ARGS_((MasterInfo *masterPtr));
static int 		TixFm_CheckArgv _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST84 char ** argv));
static void		TixFm_LostSlaveProc _ANSI_ARGS_((
			    ClientData clientData, Tk_Window tkwin));
static void 		TixFm_ReqProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin));
static int 		PlaceAllClients _ANSI_ARGS_((MasterInfo *masterPtr));
static int		PlaceClient _ANSI_ARGS_((FormInfo *clientPtr));
static int		PlaceSide_AttOpposite _ANSI_ARGS_((
			    FormInfo *clientPtr, int axis,  int which));
static int 		PlaceSide_AttAbsolute _ANSI_ARGS_((
			    FormInfo *clientPtr, int axis,  int which));
static int 		PlaceSide_AttNone _ANSI_ARGS_((
			    FormInfo *clientPtr, int axis,  int which));
static int		PlaceSide_AttParallel _ANSI_ARGS_((FormInfo *clientPtr,
			    int axis, int which));
static int 		PlaceSimpleCase _ANSI_ARGS_((
			    FormInfo *clientPtr, int axis,  int which));
static int 		PlaceWithSpring _ANSI_ARGS_((
			    FormInfo *clientPtr, int axis,  int which));
static 			int ReqSize  _ANSI_ARGS_((Tk_Window tkwin,
			    int axis));
static void 		UnmapClient _ANSI_ARGS_((FormInfo *clientPtr));
static void		MapClient _ANSI_ARGS_((FormInfo *clientPtr,
			    int x, int y, int width, int height));
static int		PinnSide_AttNone _ANSI_ARGS_((FormInfo *clientPtr,
			    int axis, int which));
static int		PinnSide_AttPercent _ANSI_ARGS_((FormInfo *clientPtr,
			    int axis, int which));
static int		PinnSide_AttOpposite _ANSI_ARGS_((FormInfo *clientPtr,
			    int axis, int which));
static int		PinnSide_AttParallel _ANSI_ARGS_((FormInfo *clientPtr,
			    int axis, int which));
static SpringLink *	AllocSpringLink _ANSI_ARGS_((void));
static void		FreeSpringLink _ANSI_ARGS_((SpringLink * link));
static void		FreeSpringList _ANSI_ARGS_((SpringList * listPtr));
static void		AddRightSprings _ANSI_ARGS_((SpringList * listPtr,
			    FormInfo *clientPtr));
static void		AddLeftSprings _ANSI_ARGS_((SpringList * listPtr,
			    FormInfo *clientPtr));

/*
 * A macro used to simplify the "pinn client" code
 */
#define PINN_CLIENT_SIDE(client, axis, which, isSelf) \
    if (PinnClientSide(client, axis, which, isSelf) == TCL_ERROR) { \
	return TCL_ERROR; \
    }
/*
 * A macro used to simplify the "place client" code
 */
#define PLACE_CLIENT_SIDE(client, axis, which, isSelf) \
    if (PlaceClientSide(client, axis, which, isSelf) == TCL_ERROR) { \
	return TCL_ERROR; \
    }

/*
 * Information about the Form geometry manager.
 */
static Tk_GeomMgr formType = {
    "tixForm",			/* name */
    TixFm_ReqProc,		/* requestProc */
    TixFm_LostSlaveProc,	/* lostSlaveProc */
};

/*
 * Hash table used to map from Tk_Window tokens to corresponding
 * FormInfo structures:
 */
static Tcl_HashTable formInfoHashTable;
static Tcl_HashTable masterInfoHashTable;

/*
 * Have static variables in this module been initialized?
 */
static initialized = 0;

static int ReqSize(tkwin, axis)
    Tk_Window tkwin;
    int axis;
{
    if (axis == AXIS_X) {
	return Tk_ReqWidth(tkwin);
    } else {
	return Tk_ReqHeight(tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tix_FormCmd --
 *
 *	This procedure is invoked to process the "tixForm" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */
int
Tix_FormCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    static Tix_SubCmdInfo subCmdInfo[] = {
	{TIX_DEFAULT_LEN, "check", 1, 1, TixFm_Check,
	   "master",},
	{TIX_DEFAULT_LEN, "configure", 1, TIX_VAR_ARGS, TixFm_SetClient,
	   "slave ?-flag value ...?",},
	{TIX_DEFAULT_LEN, "forget", 1, TIX_VAR_ARGS, TixFm_Forget,
	   "slave ?slave ...?",},
	{TIX_DEFAULT_LEN, "grid", 1, TIX_VAR_ARGS, TixFm_SetGrid,
	   "master ?x_grids y_grids?"},
	{TIX_DEFAULT_LEN, "info", 1, 2, TixFm_Info,
	   "slave ?-flag?",},
	{TIX_DEFAULT_LEN, "slaves", 1, 1, TixFm_Slaves,
	   "master",},
	{TIX_DEFAULT_LEN, "spring", 3, 3, TixFm_Spring,
	   "slave side strength",},
	{TIX_DEFAULT_LEN, TIX_DEFAULT_SUBCMD, 0, 0, TixFm_SetClient, 0,
	    TixFm_CheckArgv,}
    };

    static Tix_CmdInfo cmdInfo = {
	Tix_ArraySize(subCmdInfo), 1, TIX_VAR_ARGS, "?option? arg ?arg ...?",
    };

    return Tix_HandleSubCmds(&cmdInfo, subCmdInfo, clientData,
	interp, argc, argv);
}

/*----------------------------------------------------------------------
 *
 * TixFm_SetGrid --
 *
 *	Sets some defaults for the master window
 *
 *----------------------------------------------------------------------
 */
static int TixFm_SetGrid(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    char buff[100];
    Tk_Window   topLevel = (Tk_Window) clientData;
    Tk_Window   master;
    MasterInfo* masterPtr;

    master = Tk_NameToWindow(interp, argv[0], topLevel);

    if (master == NULL) {
	return TCL_ERROR;
    } else {
	masterPtr = GetMasterInfo(master, 1);
    }

    if (argc != 1 && argc != 3) {
	Tcl_AppendResult(interp, "Wrong # of arguments, should be ",
	    "tixForm grid master ?x_grids y_grids?", NULL);
	return TCL_ERROR;
    }

    if (argc == 1) {
	sprintf(buff, "%d %d", masterPtr->grids[0], masterPtr->grids[1]);
	Tcl_AppendResult(interp, buff, NULL);
    }
    else {
	int x, y;
	if (Tcl_GetInt(interp, argv[1], &x) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[2], &y) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (x <=0 || y <=0) {
	    Tcl_AppendResult(interp, "Grid sizes must be positive integers",
		 NULL);
	    return TCL_ERROR;
	}
	masterPtr->grids[0] = x;
	masterPtr->grids[1] = y;

	ArrangeWhenIdle(masterPtr);
    }

    return TCL_OK;
}
/*----------------------------------------------------------------------
 *
 * TixFm_Forget --
 *
 *	Sets some defaults for the master window
 *
 *----------------------------------------------------------------------
 */
static int TixFm_Forget(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    FormInfo * clientPtr;
    Tk_Window topLevel = (Tk_Window) clientData;
    int i;

    for (i=0; i<argc; i++) {
	clientPtr = TixFm_FindClientPtrByName(interp, argv[i], topLevel);
	if (clientPtr == NULL) {
	    return TCL_ERROR;
	}
	else {
	    TixFm_ForgetOneClient(clientPtr);
	}
    }

    return TCL_OK;
}

void TixFm_ForgetOneClient(clientPtr)
    FormInfo * clientPtr;
{
    if (clientPtr != NULL) {
	Tk_DeleteEventHandler(clientPtr->tkwin, StructureNotifyMask,
	    TixFm_StructureProc, (ClientData) clientPtr);
	Tk_ManageGeometry(clientPtr->tkwin, (Tk_GeomMgr *) NULL,
	    (ClientData) NULL);
	if (clientPtr->master->tkwin != Tk_Parent(clientPtr->tkwin)) {
	    Tk_UnmaintainGeometry(clientPtr->tkwin,
		clientPtr->master->tkwin);
	}
	Tk_UnmapWindow(clientPtr->tkwin);
	TixFm_Unlink(clientPtr);
    }
}

/*----------------------------------------------------------------------
 *
 * TixFm_Slaves --
 *
 *	retuen the pathnames of the clients of a master window
 *
 *----------------------------------------------------------------------
 */
static int TixFm_Slaves(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window   topLevel = (Tk_Window) clientData;
    Tk_Window   master;
    MasterInfo* masterPtr;
    FormInfo  * clientPtr;

    master = Tk_NameToWindow(interp, argv[0], topLevel);

    if (master == NULL) {
	return TCL_ERROR;
    } else {
	masterPtr = GetMasterInfo(master, 0);
    }

    if (masterPtr == 0) {
	Tcl_AppendResult(interp, "Window \"", argv[0], 
	    "\" is not a tixForm master window", NULL);
	return TCL_ERROR;
    }

    for (clientPtr = masterPtr->client; clientPtr; clientPtr=clientPtr->next) {
	Tcl_AppendElement(interp, Tk_PathName(clientPtr->tkwin));
    }
    return TCL_OK;
}
/*----------------------------------------------------------------------
 *
 * TixFm_Spring --
 *
 *	Sets the spring strength of a slave's attachment sides
 *
 *----------------------------------------------------------------------
 */
static int TixFm_Spring(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window   topLevel = (Tk_Window) clientData;
    Tk_Window   tkwin;
    FormInfo  * clientPtr;
    int         strength;
    int		i, j;
    size_t	len;

    if ((tkwin = Tk_NameToWindow(interp, argv[0], topLevel)) == NULL) {
	return TCL_ERROR;
    }

    if ((clientPtr = TixFm_GetFormInfo(tkwin, 0)) == NULL) {
	Tcl_AppendResult(interp, "Window \"", argv[0], 
	    "\" is not managed by the tixForm manager", NULL);
	return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &strength) != TCL_OK) {
	return TCL_ERROR;
    }

    len = strlen(argv[1]);
    if (strncmp(argv[1], "-top", len) == 0) {
	i = 1; j = 0;
    }
    else if (strncmp(argv[1], "-bottom", len) == 0) {
	i = 1; j = 1;
    }
    else if (strncmp(argv[1], "-left", len) == 0) {
	i = 0; j = 0;
    }
    else if (strncmp(argv[1], "-right", len) == 0) {
	i = 0; j = 1;
    }
    else {
	Tcl_AppendResult(interp, "Unknown option \"", argv[1], 
	    "\"", NULL);
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

    ArrangeWhenIdle(clientPtr->master);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 * TixFm_Check --
 *
 *	Tests whether the master has circular reference.
 *
 *----------------------------------------------------------------------
 */
static int TixFm_Check(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    MasterInfo * masterPtr;
    Tk_Window topLevel = (Tk_Window) clientData;
    Tk_Window master;

    master = Tk_NameToWindow(interp, argv[0], topLevel);
    if (master == NULL) {
	return TCL_ERROR;
    }

    masterPtr = GetMasterInfo(master, 1);

    if (TestAndArrange(masterPtr) == TCL_OK) {
	/* OK: no circular dependency */ 
	Tcl_AppendResult(interp, "0", NULL);
    } else {
	/* Bad: circular dependency */ 
	Tcl_AppendResult(interp, "1", NULL);
    }
    return TCL_OK;
}

/* Check the arguments to the default subcommand: TixFm_SetClient()
 */
static int TixFm_CheckArgv(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    if ((argc >=1) && (argv[0][0] != '.')) {
	return 0;			/* sorry, we expect a window name */
    } else {
	return 1;
    }
}


static int TixFm_SetClient(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window topLevel = (Tk_Window) clientData;
    Tk_Window client, master;
    FormInfo * clientPtr;
    MasterInfo * masterPtr;
    CONST84 char *pathName;	/* path name of the client window */

    if (argc < 1 || (((argc-1) %2) != 0)) {
	Tcl_AppendResult(interp, "Wrong # of arguments, should be ",
	    "tixForm configure slave ?-flag value ...?", NULL);
	return TCL_ERROR;
    }
    pathName = argv[0];
    argc -=1;
    argv +=1;

    client = Tk_NameToWindow(interp, pathName, topLevel);

    if (client == NULL) {
	return TCL_ERROR;
    } else if (Tk_IsTopLevel(client)) {
	Tcl_AppendResult(interp, "can't put \"", pathName,
	    "\"in a form: it's a top-level window", (char *) NULL);
	return TCL_ERROR;
    } else {
	clientPtr = TixFm_GetFormInfo(client, 1);
    }

    /* Check if the first argument is "-in". If so, 
     * reset the master of this client
     */
    if (argc >= 2 && strcmp(argv[0], "-in")==0) {
	if ((master=Tk_NameToWindow(interp, argv[1], topLevel)) == NULL) {
	    return TCL_ERROR;
	}
	argc -= 2;
	argv += 2;
	masterPtr = GetMasterInfo(master, 1);
    }
    else if (clientPtr->master == NULL) {
	if ((master = Tk_Parent(client))==NULL) {
	    return TCL_ERROR;
	} 
	masterPtr = GetMasterInfo(master, 1);
    }
    else {
	masterPtr =clientPtr->master;
    } 

    if (clientPtr->master != masterPtr) {
	if (clientPtr->master != NULL) {
	    /* Take clientPtr from old master */
	    Tk_ManageGeometry(clientPtr->tkwin, (Tk_GeomMgr *) NULL,
		(ClientData) NULL);
	    if (clientPtr->master->tkwin != Tk_Parent(clientPtr->tkwin)) {
		Tk_UnmaintainGeometry(clientPtr->tkwin,
		    clientPtr->master->tkwin);
	    }
	    TixFm_UnlinkFromMaster(clientPtr);
	}
	
	/* attach the client to the master */
	TixFm_AddToMaster(masterPtr, clientPtr);
    }

    if (argc > 0) {
	if (TixFm_Configure(clientPtr, topLevel, interp, argc,
	    argv)==TCL_ERROR){
	    return TCL_ERROR;
	}
    }

    ArrangeWhenIdle(clientPtr->master);

    return TCL_OK;
}


/* The caller of this function needs to find out a pointer to a client
 * that is already managed by tixForm.
 */
FormInfo * TixFm_FindClientPtrByName(interp, name, topLevel)
    Tcl_Interp * interp;
    CONST84 char * name;
    Tk_Window topLevel;
{
    Tk_Window tkwin;
    FormInfo * clientPtr;

    if ((tkwin = Tk_NameToWindow(interp, name, topLevel)) == NULL) {
	return NULL;
    }

    if ((clientPtr = TixFm_GetFormInfo(tkwin, 0)) == NULL) {
	Tcl_AppendResult(interp, "Window \"", name, 
	    "\" is not managed by the tixForm manager", NULL);
	return NULL;
    }
    return clientPtr;
}


static int TestAndArrange(masterPtr)
    MasterInfo *masterPtr;
{
    FormInfo *clientPtr;
    int i,j;

    /*
     * First mark all clients as unpinned, and clean the opposite flags,
     * Check the attachment intergrity
     */
    for (clientPtr = masterPtr->client; clientPtr; clientPtr=clientPtr->next) {
	if (clientPtr->tkwin != NULL) {
	    for (i=0; i<2; i++) {
		for (j=0; j<2; j++) {
		    clientPtr->side[i][j].pcnt = 0;
		    clientPtr->side[i][j].disp = 0;
		}
		/* clear all flags */
		clientPtr->sideFlags[i] = 0;
	    }
	    clientPtr->depend = 0;
	    CheckIntergrity(clientPtr);
	}
    }

    /*
     * Try to determine all the client's geometry
     */
    for (clientPtr = masterPtr->client; clientPtr; clientPtr=clientPtr->next) {
	if (clientPtr->tkwin == NULL) { /* it was deleted */
	    continue;
	}
	for (i=0; i<2; i++) {
	    if ((clientPtr->sideFlags[i] & PINNED_ALL) != PINNED_ALL) {
		if (PinnClient(clientPtr) == TCL_ERROR) {
		    /*
		     * Detected circular dependency
		     */
		    return TCL_ERROR;
		}
		break;
	    }
	}
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 *  UnmapClient
 *
 *	Unmap the client from the screen, using different methods according to
 *	the relationship between the client and slave.
 */
static void UnmapClient(clientPtr)
    FormInfo *clientPtr;
{
    if (clientPtr->master->tkwin == Tk_Parent(clientPtr->tkwin)) {
	Tk_UnmapWindow(clientPtr->tkwin);
    }
    else {
	Tk_UnmaintainGeometry(clientPtr->tkwin, clientPtr->master->tkwin);
	Tk_UnmapWindow(clientPtr->tkwin);
    }
}

/*----------------------------------------------------------------------
 *  MapClient
 *
 *	Map the client to the screen, using different methods according to
 *	the relationship between the client and slave.
 */
static void MapClient(clientPtr, x, y, width, height)
    FormInfo *clientPtr;
    int x;
    int y;
    int width;
    int height;
{
    if (clientPtr->master->tkwin == Tk_Parent(clientPtr->tkwin)) {
	Tk_MoveResizeWindow(clientPtr->tkwin, x, y, width, height);
	Tk_MapWindow(clientPtr->tkwin);
    }
    else {
	Tk_MaintainGeometry(clientPtr->tkwin, clientPtr->master->tkwin,
	    x, y, width, height);
	Tk_MapWindow(clientPtr->tkwin);
    }
}

static void ArrangeWhenIdle(masterPtr)
    MasterInfo * masterPtr;
{
    if (!(masterPtr->flags.repackPending || masterPtr->flags.isDeleted)) {
	masterPtr->flags.repackPending = 1;
	Tk_DoWhenIdle(ArrangeGeometry, (ClientData) masterPtr);
    }
}

static void
CancelArrangeWhenIdle(masterPtr)
    MasterInfo * masterPtr;
{
    if (masterPtr->flags.repackPending) {
	Tk_CancelIdleCall(ArrangeGeometry, (ClientData) masterPtr);
	masterPtr->flags.repackPending = 0;
    }
}

/*----------------------------------------------------------------------
 * ArrangeGeometry --
 *
 *	The heart of the Form geometry manager: calculates the sizes of
 *	the clients and the master, then arrange the clients inside the
 *	master according to their attachments.
 */
static void ArrangeGeometry(clientData)
    ClientData clientData;	/* Structure describing parent whose clients
				 * are to be re-layed out. */
{
    MasterInfo *masterPtr;
    FormInfo *clientPtr;
    int i, j, coord[2][2];
    int mSize[2];			/* Size of master */
    int cSize[2];			/* Size of client */
    int intBWidth;			/* internal borderWidth of master */

    masterPtr = (MasterInfo *) clientData;

    if (((Tk_FakeWin *) (masterPtr->tkwin))->flags & TK_ALREADY_DEAD) {
	masterPtr->flags.repackPending = 0;
	return;
    }

    if (masterPtr->flags.isDeleted) {
	return;
    }

    if (masterPtr->numClients == 0) {
	masterPtr->flags.repackPending = 0;
	return;
    }

    if (TestAndArrange(masterPtr)) {	/* Detected circular dependency */
	fprintf(stderr, "circular dependency.\n");
	masterPtr->flags.repackPending = 0;
	return;
    }

    /*
     * Try to determine the required size of the master
     */
    CalculateMasterSize(masterPtr);

    /*
     * If the requested size is not equal to the actual size of the master,
     * we might have to ask TK to change the master's geometry
     */

    if ((masterPtr->reqSize[0] != Tk_ReqWidth(masterPtr->tkwin))
	|| (masterPtr->reqSize[1] != Tk_ReqHeight(masterPtr->tkwin))) {

	if (masterPtr->numRequests++ > 50) {
	    fprintf(stderr, 
		"(TixForm) Error:Trying to use more than one geometry\n\
          manager for the same master window.\n\
          Giving up after 50 iterations.\n");
	} else {
	    masterPtr->flags.repackPending = 0;
	    Tk_GeometryRequest(masterPtr->tkwin,
		masterPtr->reqSize[0], masterPtr->reqSize[1]);

	    ArrangeWhenIdle(masterPtr);
	    return;
	}
    }
	
    masterPtr->numRequests = 0;

    if (!Tk_IsMapped(masterPtr->tkwin)) {
	goto done;
    }

    intBWidth = Tk_InternalBorderWidth(masterPtr->tkwin);
    mSize[0] = Tk_Width(masterPtr->tkwin)  - 2*intBWidth;
    mSize[1] = Tk_Height(masterPtr->tkwin) - 2*intBWidth;

    if (mSize[0] < 1 || mSize[1] <1) {
	/* Master is not visible. Don't bother to place the clients
	 */
	masterPtr->flags.repackPending = 0;
	return;
    }

    /*
     * Now set all the client's geometry
     */
    if (PlaceAllClients(masterPtr) != TCL_OK) {
	panic("circular dependency");
    }

    for (clientPtr = masterPtr->client; clientPtr; clientPtr=clientPtr->next) {
	if (clientPtr->tkwin == NULL) {
	    continue;
	}
	for (i=0; i<2; i++) {
	    for (j=0; j<2; j++) {
		coord[i][j] = clientPtr->posn[i][j];
		if (j == 1) {
		    coord[i][j] -= 1;
		}
	    }
	    cSize[i] = coord[i][1] - coord[i][0] 
	      - clientPtr->pad[i][0] - clientPtr->pad[i][1] + 1;
	}

	if ((cSize[0] <= 0) || (cSize[1] <= 0)) {
	    /*
	     * Window is too small, don't even bother to map
	     */
	    UnmapClient(clientPtr);
	} else if ((coord[0][1] < 0) || (coord[1][1] < 0)) {
	    /*
	     * Window is outside of the master (left or top)
	     */
	    UnmapClient(clientPtr);
	} else if ((coord[0][0] > mSize[0]) || (coord[1][0] > mSize[1])) {
	    /*
	     * Window is outside of the master (bottom or right)
	     */
	    UnmapClient(clientPtr);
	} else {
	    /*
	     * Window is visible, then map it
	     */
	    MapClient(clientPtr,
		coord[0][0] + clientPtr->pad[0][0] + intBWidth,
		coord[1][0] + clientPtr->pad[1][0] + intBWidth,
		cSize[0], cSize[1]);
	}
    }

  done:
    masterPtr->flags.repackPending = 0;
}

static int
PinnSide_AttNone(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to pinn down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    int reqSize;

    if (clientPtr->attType[axis][NEXT_SIDE(which)] == ATT_NONE) {
	if (which == SIDE0) {
	    clientPtr->side[axis][which].pcnt = 0;
	    clientPtr->side[axis][which].disp = 0;
	    return TCL_OK;
	}
    }

    reqSize = ReqSize(clientPtr->tkwin, axis) +
      clientPtr->pad[axis][0] + clientPtr->pad[axis][1];

    PINN_CLIENT_SIDE(clientPtr, axis, NEXT_SIDE(which), 1);

    clientPtr->side[axis][which].pcnt =
      clientPtr->side[axis][NEXT_SIDE(which)].pcnt;  

    switch (which) {
      case SIDE0:
	clientPtr->side[axis][which].disp = 
	  clientPtr->side[axis][NEXT_SIDE(which)].disp - reqSize;
	break;

      case SIDE1:
	clientPtr->side[axis][which].disp = 
	  clientPtr->side[axis][NEXT_SIDE(which)].disp + reqSize;
	break;
    }

    return TCL_OK;
}

static int
PinnSide_AttPercent(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to pinn down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    clientPtr->side[axis][which].pcnt = clientPtr->att[axis][which].grid;
    clientPtr->side[axis][which].disp = clientPtr->off[axis][which];

    return TCL_OK;
}

static int
PinnSide_AttOpposite(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to pinn down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    FormInfo * attachPtr;

    attachPtr = clientPtr->att[axis][which].widget;

    PINN_CLIENT_SIDE(attachPtr, axis, NEXT_SIDE(which), 0);

    clientPtr->side[axis][which].pcnt = 
      attachPtr->side[axis][NEXT_SIDE(which)].pcnt;
    clientPtr->side[axis][which].disp =
      attachPtr->side[axis][NEXT_SIDE(which)].disp + 
      clientPtr->off[axis][which];

    return TCL_OK;
}

static int
PinnSide_AttParallel(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to pinn down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    FormInfo * attachPtr;

    attachPtr = clientPtr->att[axis][which].widget;

    PINN_CLIENT_SIDE(attachPtr, axis, which, 0);

    clientPtr->side[axis][which].pcnt = 
      attachPtr->side[axis][which].pcnt;
    clientPtr->side[axis][which].disp = 
      attachPtr->side[axis][which].disp + 
      clientPtr->off[axis][which];

    return TCL_OK;
}


static int PinnClientSide(clientPtr, axis, which, isSelf)
    FormInfo *clientPtr;	/* The client to pinn down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
    int isSelf;
{
    if ((which == SIDE0) && (clientPtr->sideFlags[axis] & PINNED_SIDE0)) {
	/* already pinned */
	return TCL_OK;
    }
    if ((which == SIDE1) && (clientPtr->sideFlags[axis] & PINNED_SIDE1)) {
	/* already pinned */
	return TCL_OK;
    }

    if ((clientPtr->depend > 0) && !isSelf) {
	/*
	 * circular dependency detected
	 */
	return TCL_ERROR;
    }
    clientPtr->depend ++;

    switch (clientPtr->attType[axis][which]) {
      case ATT_NONE:
	if (PinnSide_AttNone(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;

      case ATT_OPPOSITE:
	if (PinnSide_AttOpposite(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;

      case ATT_PARALLEL:
	if (PinnSide_AttParallel(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;

      case ATT_GRID:
	if (PinnSide_AttPercent(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;
    }

    if (which == SIDE0) {
	clientPtr->sideFlags[axis] |= PINNED_SIDE0;
    } else {
	clientPtr->sideFlags[axis] |= PINNED_SIDE1;
    }
    clientPtr->depend --;

    return TCL_OK;
}

static int PinnClient(clientPtr)
    FormInfo *clientPtr;
{
    int i;

    for (i=0; i<2; i++) {
	if (!(clientPtr->sideFlags[i] & PINNED_SIDE0)) {
	    PINN_CLIENT_SIDE(clientPtr, i, SIDE0, 0);
	}
	if (!(clientPtr->sideFlags[i] & PINNED_SIDE1)) {
	    PINN_CLIENT_SIDE(clientPtr, i, SIDE1, 0);
	}
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * CalculateMasterSize --
 *
 *	This internal procedure is used to find out the required
 *	size of a master window.
 *
 * Results:
 *	The return value is a pointer to the FormInfo structure
 *	corresponding to tkwin.
 *
 * Side effects:
 *	the reqSize[2] values in masterPtr is updated.
 *
 *--------------------------------------------------------------
 */
static void CalculateMasterSize(masterPtr)
    MasterInfo *masterPtr;
{
    FormInfo *clientPtr;
    int i, cSize[2];
    int req[2];
    int intBWidth;

    /* Information about the master window */
    intBWidth = Tk_InternalBorderWidth(masterPtr->tkwin);
    req[0] = req[1] = 2*intBWidth;

    for (clientPtr = masterPtr->client; clientPtr; clientPtr=clientPtr->next) {
	if (clientPtr->tkwin == NULL) {
	    continue;
	}
	cSize[0] = Tk_ReqWidth(clientPtr->tkwin);
	cSize[1] = Tk_ReqHeight(clientPtr->tkwin);
	cSize[0] += clientPtr->pad[0][0]+clientPtr->pad[0][1];
	cSize[1] += clientPtr->pad[1][0]+clientPtr->pad[1][1];

	for (i=0; i<2; i++) {
	    /* The required size of the master depends on 
	     *	(1) natural sizes of the clients
	     *  (2) perc anchor points of the clients
	     * Ideally, the master must include as much visible parts
	     * of the clients as possible. It should also have a size
	     * big enough so that all the clients' requested (natural)
	     * sizes are satisfied. The algorithm is fairly simple, but
	     * it took me quite a while to figure out and it quite difficult
	     * to explain here. Please look at the following in-line
	     * examples.
	     */
	    int p0 = clientPtr->side[i][0].pcnt;
	    int p1 = clientPtr->side[i][1].pcnt;
	    int d0 = clientPtr->side[i][0].disp;
	    int d1 = clientPtr->side[i][1].disp;

	    int req0 = 0;
	    int req1 = 0;
	    int reqx = 0;

	    if (d0 < 0 && p0 != 0) {
		req0 = -d0 * masterPtr->grids[i] / p0;
	    }
	    if (d1 > 0 && p1 != masterPtr->grids[i]) {
		req1 =  d1 * masterPtr->grids[i] / (masterPtr->grids[i] - p1);
	    }

	    if (p0 == p1) {
		/* case 1 */
		/* Example: p0 = p1 = 10%; d0 = -10, d1 = 10
		 * then mSize should at least be 100 pixels so that
		 * side 0 can be visible. They are calculated in the
		 * previous two if statements
		 * result:
		 * 	size  = 100
		 * 	side0 = 0;
		 *	side1 = 20;
		 */

		/* Two sides are attached to the same perc anchor point */
		if (d0 >= d1) {
		    /* widget invisible */
		    req0 = req1 = 0;
		}
	    }
	    else if (p0 < p1) {
		/* case 2 */
		/* Example: p0 10%,  p2 = 20%; cSize = 35, d0 = -5, d1 = 0
		 * then mSize should at least be 300 pixels so that
		 * cSize can be satisfied.
		 * result:
		 * 	size  = 300
		 * 	side0 = 25;
		 *	side1 = 60;
		 */
		int x = cSize[i];
		if (p0 != 0 || d0 > 0) {
		    x +=  d0;
		}
		if (p1 != masterPtr->grids[i] || d1 < 0) {
		    x += -d1;
		}
		if (x > 0) {
		    reqx = x * masterPtr->grids[i] / (p1 - p0);
		}
	    }
	    else {
		/* case 2 */
		/* This is very similar to case 1, except there are more cases
		 * in which the widget becomes invisible
		 */
		if (d0 >=0 || d1 <=0) {
		    /* widget invisible */
		    req0 = req1 = 0;
		}
	    }

	    if (req[i] < req0) {
		req[i] = req0;
	    }
	    if (req[i] < req1) {
		req[i] = req1;
	    }
	    if (req[i] < reqx) {
		req[i] = reqx;
	    }
	}
    }

    req[0] += 2*intBWidth;
    req[1] += 2*intBWidth;

    masterPtr->reqSize[0] = (req[0] > 0) ? req[0] : 1;
    masterPtr->reqSize[1] = (req[1] > 0) ? req[1] : 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TixFm_StructureProc --
 *
 *	This procedure is invoked by the Tk event dispatcher in response
 *	to StructureNotify events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a window was just deleted, clean up all its packer-related
 *	information.  If it was just resized, repack its clients, if
 *	any.
 *
 *----------------------------------------------------------------------
 */

void
TixFm_StructureProc(clientData, eventPtr)
    ClientData clientData;		/* Our information about window
					 * referred to by eventPtr. */
    XEvent *eventPtr;			/* Describes what just happened. */
{
    FormInfo *clientPtr = (FormInfo *) clientData;

    switch (eventPtr->type) {
      case ConfigureNotify:
	ArrangeWhenIdle(clientPtr->master);
	break;

      case DestroyNotify:
	if (clientPtr->master) {
	    TixFm_Unlink(clientPtr);
	}
	break;

      case MapNotify:
	break;

      case UnmapNotify:
	break;
    }
}

static void
TixFm_ReqProc(clientData, tkwin)
    ClientData clientData;	/* TixForm's information about
				 * window that got new preferred
				 * geometry.  */
    Tk_Window tkwin;		/* Other Tk-related information
				 * about the window. */
{
    FormInfo *clientPtr = (FormInfo *) clientData;

    if (clientPtr) {
	ArrangeWhenIdle(clientPtr->master);
    }
}

static void
MasterStructureProc(clientData, eventPtr)
    ClientData clientData;		/* Our information about window
					 * referred to by eventPtr. */
    XEvent *eventPtr;			/* Describes what just happened. */
{
    MasterInfo *masterPtr = (MasterInfo *) clientData;

    switch (eventPtr->type) {
      case ConfigureNotify:
	if (masterPtr->numClients > 0) {
	    ArrangeWhenIdle(masterPtr);
	}
	break;

      case DestroyNotify:
	TixFm_DeleteMaster(masterPtr);
	break;

      case MapNotify:
	break;

      case UnmapNotify:
	break;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TixFm_LostSlaveProc --
 *
 *	This procedure is invoked by Tk whenever some other geometry
 *	claims control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all packer-related information about the slave.
 *
 *--------------------------------------------------------------
 */
static void
TixFm_LostSlaveProc(clientData, tkwin)
    ClientData clientData;	/* Form structure for slave window that
				 * was stolen away. */
    Tk_Window tkwin;		/* Tk's handle for the slave window. */
{
    FormInfo *clientPtr = (FormInfo *) clientData;

    Tk_DeleteEventHandler(clientPtr->tkwin, StructureNotifyMask,
	TixFm_StructureProc, (ClientData) clientPtr);
    if (clientPtr->master->tkwin != Tk_Parent(clientPtr->tkwin)) {
	Tk_UnmaintainGeometry(clientPtr->tkwin, clientPtr->master->tkwin);
    }
    Tk_UnmapWindow(clientPtr->tkwin);
    TixFm_Unlink(clientPtr);
}

/*
 * Do some basic integrity checking
 * --> right, left cannot both attach to none
 * --> top, bottom cannot both attach to none.
 * Otherwise, top or left is always set to attach at {pixel 0}
 */
static void CheckIntergrity(clientPtr)
    FormInfo * clientPtr;
{
#if 0
    /* Check the X axis */
    if ((clientPtr->attType[0][0] ==  ATT_NONE)
	&&(clientPtr->attType[0][1]  ==  ATT_NONE)) {
	clientPtr->attType[0][0]   = ATT_DEFAULT_PIXEL;
	clientPtr->att[0][0].grid = 0;
    }

    /* Check the Y axis */
    if ((clientPtr->attType[1][0] ==  ATT_NONE)
	&&(clientPtr->attType[1][1]  ==  ATT_NONE)) {
	clientPtr->attType[1][0]   = ATT_DEFAULT_PIXEL;
	clientPtr->att[1][0].grid = 0;
    }
#endif
}

/*----------------------------------------------------------------------
 * Memory management routines
 *
 *----------------------------------------------------------------------
 */
void TixFm_AddToMaster(masterPtr, clientPtr)
    MasterInfo *masterPtr;
    FormInfo *clientPtr;
{
    if (clientPtr->master == masterPtr) {
	/* already in master */
	return;
    }

    if (clientPtr->master != NULL) {
	/* We have to migrate the widget to a different parent*/
    }

    clientPtr->master = masterPtr;

    if (masterPtr->client == NULL) {
	masterPtr->client      = clientPtr;
	masterPtr->client_tail = clientPtr;
    } else {
	masterPtr->client_tail->next = clientPtr;
    }
    clientPtr->next = NULL;
    masterPtr->client_tail = clientPtr;

    ++ masterPtr->numClients;

    /* Manage its geometry using my proc */
    Tk_ManageGeometry(clientPtr->tkwin, &formType, (ClientData)clientPtr);
}

void TixFm_UnlinkFromMaster(clientPtr)
    FormInfo *clientPtr;
{
    MasterInfo *masterPtr;
    FormInfo *ptr, *prev;

#if DEBUG
    fprintf(stderr, "unlinking %s\n", Tk_PathName(clientPtr->tkwin));
#endif

    masterPtr = clientPtr->master;

    /* First: get rid of the reference of this widget from other clients */
    for (ptr=masterPtr->client; ptr; ptr=ptr->next) {
	if (ptr != clientPtr) {
	    int i, j;
	    for (i=0; i<2; i++) {
		for (j=0; j<2; j++) {
		    switch (ptr->attType[i][j]) {
		      case ATT_OPPOSITE:
		      case ATT_PARALLEL:
			if (ptr->att[i][j].widget == clientPtr) {
			    ptr->attType[i][j] = ATT_GRID;
			    ptr->att[i][j].grid = 0;
			    ptr->off[i][j]      = ptr->posn[i][j];
			}
			break;
		    }
		}
		if (ptr->strWidget[i][j] == clientPtr) {
		    ptr->strWidget[i][j] = 0;
		}
	    }
	}
    }

    /* Second: delete this client from the list */
    for (prev=ptr=masterPtr->client; ptr; prev=ptr,ptr=ptr->next) {
	if (ptr == clientPtr) {
	    if (prev==ptr) {
		if (masterPtr->numClients == 1) {
		    masterPtr->client_tail = NULL;
		}
		masterPtr->client = ptr->next;
	    }
	    else {
		if (ptr->next == NULL) {
		    masterPtr->client_tail = prev;
		}
		prev->next = ptr->next;
	    }
	    break;
	}
    }
    -- masterPtr->numClients;
}

void TixFm_FreeMasterInfo(clientData)
    ClientData clientData;
{
    MasterInfo *masterPtr = (MasterInfo *)clientData;
    ckfree((char*)masterPtr);
}

void TixFm_DeleteMaster(masterPtr)
    MasterInfo *masterPtr;
{
    Tcl_HashEntry *hPtr;
    FormInfo *clientPtr, * toFree;

    if (masterPtr->flags.isDeleted) {
	return;
    }

    Tk_DeleteEventHandler(masterPtr->tkwin, StructureNotifyMask,
	MasterStructureProc, (ClientData) masterPtr);

    clientPtr=masterPtr->client;
    while(clientPtr) {
	toFree = clientPtr;
	clientPtr = clientPtr->next;
	TixFm_ForgetOneClient(toFree);
    }

    hPtr = Tcl_FindHashEntry(&masterInfoHashTable,(char*)masterPtr->tkwin);
    if (hPtr) {
	Tcl_DeleteHashEntry(hPtr);
    }
    CancelArrangeWhenIdle(masterPtr);
    masterPtr->flags.isDeleted = 1;
    Tk_EventuallyFree((ClientData)masterPtr,
	(Tix_FreeProc*)TixFm_FreeMasterInfo);
}


void TixFm_Unlink(clientPtr)
    FormInfo *clientPtr;
{
    Tcl_HashEntry *hPtr;
    MasterInfo *masterPtr;

    /* Delete this clientPtr from the master's list */
    TixFm_UnlinkFromMaster(clientPtr);

    /* Eventually free this clientPtr structure */
    hPtr = Tcl_FindHashEntry(&formInfoHashTable,(char*)clientPtr->tkwin);
    if (hPtr != NULL) {
	Tcl_DeleteHashEntry(hPtr);
    }
    clientPtr->tkwin = NULL;
    masterPtr = clientPtr->master;
    ckfree((char*)clientPtr);

    ArrangeWhenIdle(masterPtr);
}


/*
 *--------------------------------------------------------------
 *
 * TixFm_GetFormInfo --
 *
 *	This internal procedure is used to locate a FormInfo
 *	structure for a given window, creating one if one
 *	doesn't exist already.
 *
 * Results:
 *	The return value is a pointer to the FormInfo structure
 *	corresponding to tkwin.
 *
 * Side effects:
 *	A new FormInfo structure may be created.  If so, then
 *	a callback is set up to clean things up when the
 *	window is deleted.
 *
 *--------------------------------------------------------------
 */
FormInfo *
TixFm_GetFormInfo(tkwin, create)
    Tk_Window tkwin;		/* Token for window for which
				 * FormInfo structure is desired. */
    int create;
{
    FormInfo *clientPtr;
    Tcl_HashEntry *hPtr;
    int isNew;
    int i,j;

    if (!initialized) {
	initialized = 1;
	Tcl_InitHashTable(&formInfoHashTable, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&masterInfoHashTable, TCL_ONE_WORD_KEYS);
    }

    /*
     * See if there's already FormInfo for this window.  If not,
     * then create a new one.
     */
    if (!create) {
	hPtr = Tcl_FindHashEntry(&formInfoHashTable, (char *)tkwin);
	if (!hPtr) {
	    return NULL;
	} else {
	    return (FormInfo *) Tcl_GetHashValue(hPtr);
	}
    } else {
	hPtr = Tcl_CreateHashEntry(&formInfoHashTable, (char *) tkwin, &isNew);
	if (!isNew) {
	    return (FormInfo *) Tcl_GetHashValue(hPtr);
	} else {
	    clientPtr = (FormInfo *) ckalloc(sizeof(FormInfo));
	    clientPtr->tkwin	= tkwin;
	    clientPtr->master	= NULL;
	    clientPtr->next	= NULL;
	    
	    for (i=0; i< 2; i++) {
		for (j=0; j< 2; j++) {
		    clientPtr->attType[i][j]    = ATT_NONE;
		    clientPtr->att[i][j].grid   = 0;
		    clientPtr->att[i][j].widget = NULL;
		    clientPtr->off[i][j]	= 0;

		    clientPtr->pad[i][j]        = 0;
		    clientPtr->side[i][j].pcnt  = 0;
		    clientPtr->side[i][j].disp  = 0;
		    
		    clientPtr->spring[i][j]  	= -1;
		    clientPtr->strWidget[i][j]  = 0;
		}
		clientPtr->springFail[i]  	= 0;
		clientPtr->fill[i]  		= 0;
	    }
	    
	    Tcl_SetHashValue(hPtr, clientPtr);
	    
	    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
		TixFm_StructureProc, (ClientData) clientPtr);
	    
	    return clientPtr;
	}
    }
}

static MasterInfo *
GetMasterInfo(tkwin, create)
    Tk_Window tkwin;		/* Token for window for which
				 * FormInfo structure is desired. */
    int create;			/* Should I create the MasterInfo if it
				 * does not exist? */
{
    MasterInfo *masterPtr;
    Tcl_HashEntry *hPtr;
    int isNew;

    if (!initialized) {
	initialized = 1;
	Tcl_InitHashTable(&formInfoHashTable, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&masterInfoHashTable, TCL_ONE_WORD_KEYS);
    }

    /*
     * See if there's already FormInfo for this window.  If not,
     * then create a new one.
     */
    if (!create) {
	hPtr = Tcl_FindHashEntry(&masterInfoHashTable, (char *)tkwin);
	if (!hPtr) {
	    return NULL;
	} else {
	    return (MasterInfo *) Tcl_GetHashValue(hPtr);
	}
    } else {
	hPtr = Tcl_CreateHashEntry(&masterInfoHashTable, (char *)tkwin,
	    &isNew);
	if (!isNew) {
	    masterPtr =  (MasterInfo *) Tcl_GetHashValue(hPtr);
	}
	else {
	    masterPtr = (MasterInfo *) ckalloc(sizeof(MasterInfo));
	    masterPtr->tkwin	   		= tkwin;
	    masterPtr->client	   		= NULL;
	    masterPtr->client_tail 		= NULL;
	    masterPtr->flags.repackPending 	= 0;
	    masterPtr->flags.isDeleted 		= 0;
	    masterPtr->numClients  		= 0;
	    masterPtr->numRequests 		= 0;
	    masterPtr->grids[0]			= 100;
	    masterPtr->grids[1]			= 100;

	    Tcl_SetHashValue(hPtr, masterPtr);
	}
    }

    /* TK BUG:
     *
     * It seems like if you destroy some slaves TK will delete the event
     * handler. So for sure we just create it every time a slave is created.
     *
     * Note: calling Tk_CreateEventHandler with same arguments twice won't
     * create two instances of the same event handler: Thus safe to call
     * blindly.
     */
    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	MasterStructureProc, (ClientData) masterPtr);
#if 0
    Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)&masterType,
	(ClientData) masterPtr);
#endif
    return masterPtr;
}

/*----------------------------------------------------------------------
 * PLace the clients
 *----------------------------------------------------------------------
 */
static int PlaceSide_AttNone(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to Place down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    int reqSize;

    if (clientPtr->attType[axis][NEXT_SIDE(which)] == ATT_NONE) {
	if (which == SIDE0) {
	    clientPtr->posn[axis][which] = 0;
	    return TCL_OK;
	}
    }

    reqSize = ReqSize(clientPtr->tkwin, axis) +
      clientPtr->pad[axis][0] + clientPtr->pad[axis][1];


    PLACE_CLIENT_SIDE(clientPtr, axis, NEXT_SIDE(which), 1);

    switch (which) {
      case SIDE0:
	clientPtr->posn[axis][which] = 
	  clientPtr->posn[axis][NEXT_SIDE(which)] - reqSize;
	break;

      case SIDE1:
	clientPtr->posn[axis][which] = 
	  clientPtr->posn[axis][NEXT_SIDE(which)] + reqSize;
	break;
    }

    return TCL_OK;
}

static int PlaceSide_AttAbsolute(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to Place down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    int mSize[2];
    MasterInfo * masterPtr = clientPtr->master;
    int intBWidth = Tk_InternalBorderWidth(masterPtr->tkwin);
    mSize[0] = Tk_Width(masterPtr->tkwin)  - 2*intBWidth;
    mSize[1] = Tk_Height(masterPtr->tkwin) - 2*intBWidth;

    clientPtr->posn[axis][which] =
      mSize[axis] * clientPtr->side[axis][which].pcnt/masterPtr->grids[axis] +
      clientPtr->side[axis][which].disp;

    return TCL_OK;
}

static int PlaceSide_AttOpposite(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to Place down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    FormInfo * attachPtr;

    attachPtr = clientPtr->att[axis][which].widget;

    PLACE_CLIENT_SIDE(attachPtr, axis, NEXT_SIDE(which), 0);

    clientPtr->posn[axis][which] = attachPtr->posn[axis][NEXT_SIDE(which)];
    clientPtr->posn[axis][which] += clientPtr->off[axis][which];
    return TCL_OK;
}

static int PlaceSide_AttParallel(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to Place down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    FormInfo * attachPtr;

    attachPtr = clientPtr->att[axis][which].widget;

    PLACE_CLIENT_SIDE(attachPtr, axis, NEXT_SIDE(which), 0);

    clientPtr->posn[axis][which] = attachPtr->posn[axis][which];
    clientPtr->posn[axis][which] += clientPtr->off[axis][which];

    return TCL_OK;
}


static int PlaceSimpleCase(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to Place down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    clientPtr->depend ++;

    switch (clientPtr->attType[axis][which]) {
      case ATT_NONE:
	if (PlaceSide_AttNone(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;

      case ATT_GRID:
	if (PlaceSide_AttAbsolute(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;
 
      case ATT_OPPOSITE:
	if (PlaceSide_AttOpposite(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;
      case ATT_PARALLEL:
	if (PlaceSide_AttParallel(clientPtr, axis, which) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	break;
    }

    if (which == SIDE0) {
	clientPtr->sideFlags[axis] |= PINNED_SIDE0;
    } else {
	clientPtr->sideFlags[axis] |= PINNED_SIDE1;
    }
    clientPtr->depend --;

    return TCL_OK;
}

/* ToDo: I'll make this more efficient by pre-allocating some links */
static SpringLink *
AllocSpringLink()
{
    return (SpringLink *) ckalloc(sizeof(SpringLink));
}

static void
FreeSpringLink(link)
    SpringLink * link;
{
    ckfree((char*)link);
}

static void FreeSpringList(listPtr)
    SpringList * listPtr;
{
    SpringLink * link, * toFree; 

    for (link=listPtr->head; link; ) {
	toFree = link;
	link=link->next;
	FreeSpringLink(toFree);
    }
}

static void
AddRightSprings(listPtr, clientPtr)
    SpringList * listPtr;
    FormInfo *clientPtr;
{
    SpringLink * link = AllocSpringLink();

    link->next = NULL;
    link->clientPtr = clientPtr;

    if (listPtr->head == NULL) {
	listPtr->head = listPtr->tail = link;
    } else {
	listPtr->tail->next = link;
	listPtr->tail = link;
    }
    ++ listPtr->num;
}

static void
AddLeftSprings(listPtr,clientPtr)
    SpringList * listPtr;
    FormInfo *clientPtr;
{
    SpringLink * link = AllocSpringLink();

    link->next = listPtr->head;
    link->clientPtr = clientPtr;

    listPtr->head = link;
    ++ listPtr->num;
}

static int
PlaceWithSpring(clientPtr, axis, which)
    FormInfo *clientPtr;	/* The client to Place down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
{
    SpringList springs;
    SpringLink * link;
    FormInfo *ptr;
    float boundary[2];
    float totalSize, totalStrength;
    int mSize[2];
    float gap, disp;
    MasterInfo * masterPtr = clientPtr->master;
    int intBWidth = Tk_InternalBorderWidth(masterPtr->tkwin);

    springs.head = (SpringLink *)0;
    springs.tail = (SpringLink *)0;
    springs.num  = 0;

    mSize[0] = Tk_Width(masterPtr->tkwin)  - 2*intBWidth;
    mSize[1] = Tk_Height(masterPtr->tkwin) - 2*intBWidth;

    /* Expand the right side of the spring list */
    ptr = clientPtr;
    while (1) {
	switch (ptr->attType[axis][1]) {
	  case ATT_OPPOSITE:
	  case ATT_NONE:
	    /* Some attachments */
	    AddRightSprings(&springs, ptr);

	    if ((ptr = ptr->strWidget[axis][1]) == 0) {
		goto done1;
	    }
	    
	    switch (ptr->attType[axis][0]) {
	      case ATT_GRID:
	      case ATT_PARALLEL:
		goto done1;
	    }
	    break;

	  case ATT_GRID:
	  case ATT_PARALLEL:
	    AddRightSprings(&springs, ptr);
	    goto done1;
	}
    }

  done1:
    /* Expand the left side of the spring list */

    ptr = clientPtr;
    while (2) {
	switch (ptr->attType[axis][0]) {
	  case ATT_OPPOSITE:
	  case ATT_NONE:
	    /* Some attachments */
	    if (ptr != clientPtr) {
		AddLeftSprings(&springs, ptr);
	    }

	    if ((ptr = ptr->strWidget[axis][0]) == 0) {
		goto done2;
	    }

	    switch (ptr->attType[axis][1]) {
	      case ATT_PARALLEL:
		goto done2;
	    }
	    break;

	  case ATT_GRID:
	  case ATT_PARALLEL:
	    if (ptr != clientPtr) {
		AddLeftSprings(&springs, ptr);
	    }

	    goto done2;
	}
    }

  done2:

    /* Make sure this is a good list (neither ends are none) */
    if (springs.head == NULL) {
	/* this should never happen, just to make sure */
	goto fail;	
    }
    if (springs.head->clientPtr->attType[axis][0] == ATT_NONE) {
	goto fail;
    }
    if (springs.tail->clientPtr->attType[axis][1] == ATT_NONE) {
	goto fail;
    }

    /*
     * Now calculate the total requested sizes of the spring group
     */
    totalSize     = (float)(0.0);
    totalStrength = (float)(0.0);
    for (link=springs.head; link; link=link->next) {
	int size = ReqSize(link->clientPtr->tkwin, axis);

	totalSize += size + link->clientPtr->pad[axis][0] + 
	  link->clientPtr->pad[axis][1];
	
	if (link->clientPtr->spring[axis][0] > 0) {
	    totalStrength += link->clientPtr->spring[axis][0];
	}
    }
    if (springs.tail->clientPtr->spring[axis][1] > 0) {
	totalStrength += springs.tail->clientPtr->spring[axis][1];
    }

    boundary[0] = (float) mSize[axis] * 
      (float) springs.head->clientPtr->side[axis][0].pcnt /
      (float) masterPtr->grids[axis] +
      (float) springs.head->clientPtr->side[axis][0].disp;
    boundary[1] = (float) mSize[axis] * 
      (float) springs.tail->clientPtr->side[axis][1].pcnt /
      (float) masterPtr->grids[axis] +
      (float) springs.tail->clientPtr->side[axis][1].disp;

    /* (4) Now spread the sizes to the members of this list */
    gap = (float)(boundary[1] - boundary[0]) - totalSize;
    if (gap < 0) {
	goto fail;
    }

    disp = boundary[0];
    if (totalStrength <= 0.0) {
	totalStrength = (float)(1.0);
    }
    for (link=springs.head; link; link=link->next) {
	float spring0, spring1;
	int gap0, gap1;
	int adjust;		/* to overcome round-off errors */

	spring0 = (float)link->clientPtr->spring[axis][0];
	spring1 = (float)link->clientPtr->spring[axis][1];

	if (spring0 < (float)(0.0)) {
	    spring0 = (float)(0.0);
	}
	if (spring1 < (float)(0.0)) {
	    spring1 = (float)(0.0);
	}

	/* Divide by two: because two consecutive clients share the same
	 * spring; so each of them get a half.
	 */
	adjust = 0;
	if (link !=springs.head) {
	    if (spring0 > 0 && link->clientPtr->spring[axis][0] % 2 == 1) {
		adjust = 1;
	    }
	    spring0 /= (float)(2.0);
	}
	if (link !=springs.tail) {
	    spring1 /= (float)(2.0);
	}

	gap0 = (int)(gap * spring0 / totalStrength) + adjust;
	gap1 = (int)(gap * spring1 / totalStrength);

	if (link->clientPtr->fill[axis]) {
	    link->clientPtr->posn[axis][0] = (int)disp;
	    disp += gap0;
	    disp += gap1;
	    disp += ReqSize(link->clientPtr->tkwin, axis);

	    /* Somehow there may be a round-off right at the end of the
	     * list --> kludge*/
	    if (link->next == NULL) {
		disp = boundary[1];
	    }
	    link->clientPtr->posn[axis][1] = (int)disp;
	} else {
	    disp += gap0;
	    link->clientPtr->posn[axis][0] = (int)disp;
	    disp += ReqSize(link->clientPtr->tkwin, axis);
	    link->clientPtr->posn[axis][1] = (int)disp;
	    disp += gap1;

	    /*
	     * Somehow there may be a round-off right at the end of the
	     * list --> kludge
	     */
	    if (link->next == NULL && gap1 < 0.001) {
		link->clientPtr->posn[axis][1] =  (int)boundary[1];
	    }
	}

	link->clientPtr->sideFlags[axis] |= PINNED_SIDE0;
	link->clientPtr->sideFlags[axis] |= PINNED_SIDE1;
    }

    FreeSpringList(&springs);
    return TCL_OK;

  fail:
    for (link=springs.head; link; link=link->next) {
	link->clientPtr->springFail[axis] = 1;
    }
    FreeSpringList(&springs);
    return TCL_ERROR;
}

static int PlaceClientSide(clientPtr, axis, which, isSelf)
    FormInfo *clientPtr;	/* The client to Place down */
    int axis;				/* 0 = x axis, 1 = yaxis */
    int which;				/* 0 = min side, 1= max side */
    int isSelf;
{
    if ((which == SIDE0) && (clientPtr->sideFlags[axis] & PINNED_SIDE0)) {
	/* already Placeed */
	return TCL_OK;
    }
    if ((which == SIDE1) && (clientPtr->sideFlags[axis] & PINNED_SIDE1)) {
	/* already Placeed */
	return TCL_OK;
    }

    if ((clientPtr->depend > 0) && !isSelf) {
	/*
	 * circular dependency detected
	 */
	return TCL_ERROR;
    }

    /*  No spring : we just do a "simple case"
     *  The condition is ( (x || x) && (x || x) )
     */
    if ((clientPtr->spring[axis][0] < 0 ||
	 (clientPtr->sideFlags[axis] & PINNED_SIDE0)) &&
	(clientPtr->spring[axis][1] < 0 ||
	 (clientPtr->sideFlags[axis] & PINNED_SIDE1))) {
        return PlaceSimpleCase(clientPtr, axis, which);
    }
    if (clientPtr->springFail[axis]) {
	return PlaceSimpleCase(clientPtr, axis, which);
    }

    if (PlaceWithSpring(clientPtr, axis, which) != TCL_OK) {
	/* if comes to here : (1) Not enough space for the spring expansion 
	 * 		      (2) Not both end-sides are spring-attached */
	return PlaceSimpleCase(clientPtr, axis, which);
    } else {
	return TCL_OK;
    }
}

static int PlaceClient(clientPtr)
    FormInfo *clientPtr;
{
    int i;

    for (i=0; i<2; i++) {
	if (!(clientPtr->sideFlags[i] & PINNED_SIDE0)) {
	    PLACE_CLIENT_SIDE(clientPtr, i, SIDE0, 0);
	}
	if (!(clientPtr->sideFlags[i] & PINNED_SIDE1)) {
	    PLACE_CLIENT_SIDE(clientPtr, i, SIDE1, 0);
	}
    }

    return TCL_OK;
}

static int PlaceAllClients(masterPtr)
    MasterInfo * masterPtr;
{
    FormInfo *clientPtr;
    int i;

    /*
     * First mark all clients as unpinned, and clean the opposite flags,
     */
    for (clientPtr = masterPtr->client; clientPtr; clientPtr=clientPtr->next) {
	if (clientPtr->tkwin != NULL) {
	    for (i=0; i<2; i++) {
		/* clear all flags */
		clientPtr->sideFlags[i]  = 0;
		clientPtr->springFail[i] = 0;
	    }
	    clientPtr->depend = 0;
	}
    }

    /*
     * Now calculate their actual positions on the master
     */
    for (clientPtr = masterPtr->client; clientPtr; clientPtr=clientPtr->next) {
	if (clientPtr->tkwin == NULL) { /* it was deleted */
	    continue;
	}
	for (i=0; i<2; i++) {
	    if ((clientPtr->sideFlags[i] & PINNED_ALL) != PINNED_ALL) {
		if (PlaceClient(clientPtr) == TCL_ERROR) {
		    /*
		     * Detected circular dependency
		     */
		    return TCL_ERROR;
		}
		break;
	    }
	}
    }
    return TCL_OK;
}
