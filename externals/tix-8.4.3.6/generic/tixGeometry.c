
/*	$Id: tixGeometry.c,v 1.4 2005/03/25 20:15:53 hobbs Exp $	*/

/* 
 * tixGeometry.c --
 *
 *	TCL bindings of TK Geometry Management functions.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>

static Tcl_HashTable clientTable;	/* hash table for geometry managers */

static void		StructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		GeoReqProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin));
static void		GeoLostSlaveProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin));

typedef struct ClientStruct {
    Tcl_Interp * interp;
    Tk_Window    tkwin;
    char       * command;
    unsigned     isDeleted : 1;
} ClientStruct;

static Tk_GeomMgr geoType = {
    "tixGeometry",		/* name */
    GeoReqProc,			/* requestProc */
    GeoLostSlaveProc,		/* lostSlaveProc */
};

/*----------------------------------------------------------------------
 *
 * 			Geometry Management Hooks
 *
 *
 *----------------------------------------------------------------------
 */
/*----------------------------------------------------------------------
 *
 * The following functions handles the geometry requests of the clients
 *
 *----------------------------------------------------------------------
 */

static void FreeClientStruct(clientData)
    ClientData clientData;
{
    ClientStruct * cnPtr = (ClientStruct *) clientData;

    ckfree((char*)cnPtr->command);
    ckfree((char*)cnPtr);
}

/* This function is called when the clients initiates a geometry
 * request i.e., a button changes its text and now needs a larger
 * width
 *
 */
static void
GeoReqProc(clientData, tkwin)
    ClientData clientData;	/* Information about
				 * window that got new preferred
				 * geometry.  */
    Tk_Window tkwin;		/* Other Tk-related information
				 * about the window. */
{
    ClientStruct * cnPtr = (ClientStruct *) clientData;
    int result;

    if (cnPtr->isDeleted) {
	return;
    }

    result = Tix_GlobalVarEval(cnPtr->interp,	cnPtr->command, " -request ", 
	Tk_PathName(cnPtr->tkwin), (char*)NULL);

    if (result != TCL_OK) {
	Tcl_AddErrorInfo(cnPtr->interp,
	    "\n    (geometry request command executed by tixManageGeometry)");
	Tk_BackgroundError(cnPtr->interp);
    }
}

/*
 * This function is called when the clients is grabbed by another 
 * geometry manager. %% Should inform with a -lost call
 */
static void
GeoLostSlaveProc(clientData, tkwin)
    ClientData clientData;	/* Information about
				 * window that got new preferred
				 * geometry.  */
    Tk_Window tkwin;		/* Other Tk-related information
				 * about the window. */
{
    ClientStruct  * cnPtr = (ClientStruct *) clientData;
    Tcl_HashEntry * hashPtr;
    int result;

    if (cnPtr->isDeleted) {
	return;
    }

    result = Tix_GlobalVarEval(cnPtr->interp, cnPtr->command, " -lostslave ", 
	Tk_PathName(cnPtr->tkwin), (char*)NULL);

    if (result != TCL_OK) {
	Tcl_AddErrorInfo(cnPtr->interp,
	    "\n    (geometry request command executed by tixManageGeometry)");
	Tk_BackgroundError(cnPtr->interp);
    }

    hashPtr = Tcl_FindHashEntry(&clientTable, (char *)tkwin);
    if (hashPtr) {
	Tcl_DeleteHashEntry(hashPtr);
    }
    cnPtr->isDeleted = 1;
    Tk_EventuallyFree((ClientData) cnPtr, (Tix_FreeProc*)FreeClientStruct);
}


static void StructureProc(clientData, eventPtr)
    ClientData clientData;
    XEvent *eventPtr;
{
    ClientStruct  * cnPtr = (ClientStruct *) clientData;
    Tcl_HashEntry * hashPtr;

    if (eventPtr->type == DestroyNotify) {
	if (cnPtr->isDeleted) {
	    return;
	}

	hashPtr = Tcl_FindHashEntry(&clientTable, (char *)cnPtr->tkwin);
	if (hashPtr) {
	    Tcl_DeleteHashEntry(hashPtr);
	}
	cnPtr->isDeleted = 1;
	Tk_EventuallyFree((ClientData) cnPtr, (Tix_FreeProc*)FreeClientStruct);
    }
}


/*
 *
 * argv[1] = clientPathName
 * argv[2] = managerCommand	<-- can have arguments
 *
 * %% add possibility to delete a manager
 *
 */
TIX_DEFINE_CMD(Tix_ManageGeometryCmd)
{
    Tk_Window 		topLevel = (Tk_Window)clientData;
    Tk_Window		tkwin;
    ClientStruct      * cnPtr;
    Tcl_HashEntry     * hashPtr;
    int			isNew;
    static int 	        inited = 0;

    if (argc!=3) {
	return Tix_ArgcError(interp, argc, argv, 1, "pathname command");
    }

    if ((tkwin = Tk_NameToWindow(interp, argv[1], topLevel)) == NULL) {
	return TCL_ERROR;
    }

    if (!inited) {
	Tcl_InitHashTable(&clientTable, TCL_ONE_WORD_KEYS);
	inited = 1;
    }

    hashPtr = Tcl_CreateHashEntry(&clientTable, (char *)tkwin, &isNew);

    if (!isNew) {
	cnPtr = (ClientStruct *) Tcl_GetHashValue(hashPtr);
	ckfree(cnPtr->command);
	cnPtr->command = tixStrDup(argv[2]);
    } else {
	cnPtr = (ClientStruct *) ckalloc(sizeof(ClientStruct));
	cnPtr->tkwin     = tkwin;
	cnPtr->interp    = interp;
	cnPtr->command   = tixStrDup(argv[2]);
	cnPtr->isDeleted = 0;
	Tcl_SetHashValue(hashPtr, cnPtr);

	Tk_ManageGeometry(tkwin, &geoType, (ClientData)cnPtr);
	Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	    StructureProc, (ClientData)cnPtr);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 *	The following are TCL bindings for the TK geometry functions.
 *
 *----------------------------------------------------------------------
 */

/*
 *
 * argv[1] = clientPathName
 * argv[2] = req width
 * argv[3] = req height
 *
 */
TIX_DEFINE_CMD(Tix_GeometryRequestCmd)
{
    Tk_Window topLevel = (Tk_Window)clientData;
    Tk_Window tkwin;
    int reqWidth;
    int reqHeight;

    if (argc != 4) {
	return Tix_ArgcError(interp, argc, argv, 1, 
	    "pathname reqwidth reqheight");
    }

    if ((tkwin = Tk_NameToWindow(interp, argv[1], topLevel)) == NULL) {
	return TCL_ERROR;
    }

    if (Tk_GetPixels(interp, tkwin, argv[2], &reqWidth) != TCL_OK) {
	return TCL_ERROR;
    }

    if (Tk_GetPixels(interp, tkwin, argv[3], &reqHeight) != TCL_OK) {
	return TCL_ERROR;
    }

    Tk_GeometryRequest(tkwin, reqWidth, reqHeight);
    return TCL_OK;
}

/*
 *
 * argv[1] = clientPathName
 * argv[2] = width
 * argv[3] = height
 * argv[4] = width
 * argv[5] = height
 *
 */
TIX_DEFINE_CMD(Tix_MoveResizeWindowCmd)
{
    Tk_Window topLevel = (Tk_Window)clientData;
    Tk_Window tkwin;
    int x, y;
    int width;
    int height;

    if (argc != 6) {
	return Tix_ArgcError(interp, argc, argv, 1,
			     "pathname x y width height");
    }

    if ((tkwin = Tk_NameToWindow(interp, argv[1], topLevel)) == NULL) {
	return TCL_ERROR;
    }

    if (Tk_GetPixels(interp, tkwin, argv[2], &x) != TCL_OK) {
	return TCL_ERROR;
    }

    if (Tk_GetPixels(interp, tkwin, argv[3], &y) != TCL_OK) {
	return TCL_ERROR;
    }

    if (Tk_GetPixels(interp, tkwin, argv[4], &width) != TCL_OK) {
	return TCL_ERROR;
    }

    if (Tk_GetPixels(interp, tkwin, argv[5], &height) != TCL_OK) {
	return TCL_ERROR;
    }

    Tk_MoveResizeWindow(tkwin, x, y, width, height);
    return TCL_OK;
}

/*
 *
 * argv[1] = clientPathName
 *
 */
TIX_DEFINE_CMD(Tix_MapWindowCmd)
{
    Tk_Window topLevel = (Tk_Window)clientData;
    Tk_Window tkwin;

    if (argc != 2) {
	return Tix_ArgcError(interp, argc, argv, 1, "pathname");
    }

    if ((tkwin = Tk_NameToWindow(interp, argv[1], topLevel)) == NULL) {
	return TCL_ERROR;
    }

    Tk_MapWindow(tkwin);
    return TCL_OK;
}

/*
 * Tix_FlushXCmd -- calls XFlush()
 * argv[1] = pathName
 *
 */
TIX_DEFINE_CMD(Tix_FlushXCmd)
{
    Tk_Window topLevel = (Tk_Window)clientData;
    Tk_Window tkwin;

    if (argc != 2) {
	return Tix_ArgcError(interp, argc, argv, 1, "pathname");
    }

    if ((tkwin = Tk_NameToWindow(interp, argv[1], topLevel)) == NULL) {
	return TCL_ERROR;
    }

#if !defined(__WIN32__) && !defined(MAC_TCL) && !defined(MAC_OSX_TK) /* UNIX */
    XFlush(Tk_Display(tkwin));
#endif
    return TCL_OK;
}

/*
 *
 * argv[1] = clientPathName
 *
 */
TIX_DEFINE_CMD(Tix_UnmapWindowCmd)
{
    Tk_Window 		topLevel = (Tk_Window)clientData;
    Tk_Window tkwin;

    if (argc != 2) {
	return Tix_ArgcError(interp, argc, argv, 1, "pathname");
    }

    if ((tkwin = Tk_NameToWindow(interp, argv[1], topLevel)) == NULL) {
	return TCL_ERROR;
    }

    Tk_UnmapWindow(tkwin);
    return TCL_OK;
}
