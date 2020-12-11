/*
 * tkPlace.c --
 *
 *	This file contains code to implement a simple geometry manager for Tk
 *	based on absolute placement or "rubber-sheet" placement.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * Border modes for relative placement:
 *
 * BM_INSIDE:		relative distances computed using area inside all
 *			borders of master window.
 * BM_OUTSIDE:		relative distances computed using outside area that
 *			includes all borders of master.
 * BM_IGNORE:		border issues are ignored: place relative to master's
 *			actual window size.
 */

static const char *const borderModeStrings[] = {
    "inside", "outside", "ignore", NULL
};

typedef enum {BM_INSIDE, BM_OUTSIDE, BM_IGNORE} BorderMode;

/*
 * For each window whose geometry is managed by the placer there is a
 * structure of the following type:
 */

typedef struct Slave {
    Tk_Window tkwin;		/* Tk's token for window. */
    Tk_Window inTkwin;		/* Token for the -in window. */
    struct Master *masterPtr;	/* Pointer to information for window relative
				 * to which tkwin is placed. This isn't
				 * necessarily the logical parent of tkwin.
				 * NULL means the master was deleted or never
				 * assigned. */
    struct Slave *nextPtr;	/* Next in list of windows placed relative to
				 * same master (NULL for end of list). */
    Tk_OptionTable optionTable;	/* Table that defines configuration options
				 * available for this command. */
    /*
     * Geometry information for window; where there are both relative and
     * absolute values for the same attribute (e.g. x and relX) only one of
     * them is actually used, depending on flags.
     */

    int x, y;			/* X and Y pixel coordinates for tkwin. */
    Tcl_Obj *xPtr, *yPtr;	/* Tcl_Obj rep's of x, y coords, to keep pixel
				 * spec. information. */
    double relX, relY;		/* X and Y coordinates relative to size of
				 * master. */
    int width, height;		/* Absolute dimensions for tkwin. */
    Tcl_Obj *widthPtr;		/* Tcl_Obj rep of width, to keep pixel
				 * spec. */
    Tcl_Obj *heightPtr;		/* Tcl_Obj rep of height, to keep pixel
				 * spec. */
    double relWidth, relHeight;	/* Dimensions for tkwin relative to size of
				 * master. */
    Tcl_Obj *relWidthPtr;
    Tcl_Obj *relHeightPtr;
    Tk_Anchor anchor;		/* Which point on tkwin is placed at the given
				 * position. */
    BorderMode borderMode;	/* How to treat borders of master window. */
    int flags;			/* Various flags; see below for bit
				 * definitions. */
} Slave;

/*
 * Type masks for options:
 */

#define IN_MASK		1

static const Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_ANCHOR, "-anchor", NULL, NULL, "nw", -1,
	 Tk_Offset(Slave, anchor), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-bordermode", NULL, NULL, "inside", -1,
	 Tk_Offset(Slave, borderMode), 0, borderModeStrings, 0},
    {TK_OPTION_PIXELS, "-height", NULL, NULL, "", Tk_Offset(Slave, heightPtr),
	 Tk_Offset(Slave, height), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_WINDOW, "-in", NULL, NULL, "", -1, Tk_Offset(Slave, inTkwin),
	 0, 0, IN_MASK},
    {TK_OPTION_DOUBLE, "-relheight", NULL, NULL, "",
	 Tk_Offset(Slave, relHeightPtr), Tk_Offset(Slave, relHeight),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-relwidth", NULL, NULL, "",
	 Tk_Offset(Slave, relWidthPtr), Tk_Offset(Slave, relWidth),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-relx", NULL, NULL, "0", -1,
	 Tk_Offset(Slave, relX), 0, 0, 0},
    {TK_OPTION_DOUBLE, "-rely", NULL, NULL, "0", -1,
	 Tk_Offset(Slave, relY), 0, 0, 0},
    {TK_OPTION_PIXELS, "-width", NULL, NULL, "", Tk_Offset(Slave, widthPtr),
	 Tk_Offset(Slave, width), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-x", NULL, NULL, "0", Tk_Offset(Slave, xPtr),
	 Tk_Offset(Slave, x), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-y", NULL, NULL, "0", Tk_Offset(Slave, yPtr),
	 Tk_Offset(Slave, y), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, -1, 0, 0, 0}
};

/*
 * Flag definitions for Slave structures:
 *
 * CHILD_WIDTH -		1 means -width was specified;
 * CHILD_REL_WIDTH -		1 means -relwidth was specified.
 * CHILD_HEIGHT -		1 means -height was specified;
 * CHILD_REL_HEIGHT -		1 means -relheight was specified.
 */

#define CHILD_WIDTH		1
#define CHILD_REL_WIDTH		2
#define CHILD_HEIGHT		4
#define CHILD_REL_HEIGHT	8

/*
 * For each master window that has a slave managed by the placer there is a
 * structure of the following form:
 */

typedef struct Master {
    Tk_Window tkwin;		/* Tk's token for master window. */
    struct Slave *slavePtr;	/* First in linked list of slaves placed
				 * relative to this master. */
    int *abortPtr;		/* If non-NULL, it means that there is a nested
				 * call to RecomputePlacement already working on
				 * this window.  *abortPtr may be set to 1 to
				 * abort that nested call.  This happens, for
				 * example, if tkwin or any of its slaves
				 * is deleted. */
    int flags;			/* See below for bit definitions. */
} Master;

/*
 * Flag definitions for masters:
 *
 * PARENT_RECONFIG_PENDING -	1 means that a call to RecomputePlacement is
 *				already pending via a Do_When_Idle handler.
 */

#define PARENT_RECONFIG_PENDING	1

/*
 * The following structure is the official type record for the placer:
 */

static void		PlaceRequestProc(ClientData clientData,
			    Tk_Window tkwin);
static void		PlaceLostSlaveProc(ClientData clientData,
			    Tk_Window tkwin);

static const Tk_GeomMgr placerType = {
    "place",			/* name */
    PlaceRequestProc,		/* requestProc */
    PlaceLostSlaveProc,		/* lostSlaveProc */
};

/*
 * Forward declarations for functions defined later in this file:
 */

static void		SlaveStructureProc(ClientData clientData,
			    XEvent *eventPtr);
static int		ConfigureSlave(Tcl_Interp *interp, Tk_Window tkwin,
			    Tk_OptionTable table, int objc,
			    Tcl_Obj *const objv[]);
static int		PlaceInfoCommand(Tcl_Interp *interp, Tk_Window tkwin);
static Slave *		CreateSlave(Tk_Window tkwin, Tk_OptionTable table);
static void		FreeSlave(Slave *slavePtr);
static Slave *		FindSlave(Tk_Window tkwin);
static Master *		CreateMaster(Tk_Window tkwin);
static Master *		FindMaster(Tk_Window tkwin);
static void		MasterStructureProc(ClientData clientData,
			    XEvent *eventPtr);
static void		RecomputePlacement(ClientData clientData);
static void		UnlinkSlave(Slave *slavePtr);

/*
 *--------------------------------------------------------------
 *
 * Tk_PlaceObjCmd --
 *
 *	This function is invoked to process the "place" Tcl commands. See the
 *	user documentation for details on what it does.
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
Tk_PlaceObjCmd(
    ClientData clientData,	/* Interpreter main window. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window main_win = clientData;
    Tk_Window tkwin;
    Slave *slavePtr;
    TkDisplay *dispPtr;
    Tk_OptionTable optionTable;
    static const char *const optionStrings[] = {
	"configure", "forget", "info", "slaves", NULL
    };
    enum options { PLACE_CONFIGURE, PLACE_FORGET, PLACE_INFO, PLACE_SLAVES };
    int index;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "option|pathName args");
	return TCL_ERROR;
    }

    /*
     * Create the option table for this widget class. If it has already been
     * created, the cached pointer will be returned.
     */

     optionTable = Tk_CreateOptionTable(interp, optionSpecs);

    /*
     * Handle special shortcut where window name is first argument.
     */

    if (Tcl_GetString(objv[1])[0] == '.') {
	if (TkGetWindowFromObj(interp, main_win, objv[1],
		&tkwin) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * Initialize, if that hasn't been done yet.
	 */

	dispPtr = ((TkWindow *) tkwin)->dispPtr;
	if (!dispPtr->placeInit) {
	    Tcl_InitHashTable(&dispPtr->masterTable, TCL_ONE_WORD_KEYS);
	    Tcl_InitHashTable(&dispPtr->slaveTable, TCL_ONE_WORD_KEYS);
	    dispPtr->placeInit = 1;
	}

	return ConfigureSlave(interp, tkwin, optionTable, objc-2, objv+2);
    }

    /*
     * Handle more general case of option followed by window name followed by
     * possible additional arguments.
     */

    if (TkGetWindowFromObj(interp, main_win, objv[2],
	    &tkwin) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Initialize, if that hasn't been done yet.
     */

    dispPtr = ((TkWindow *) tkwin)->dispPtr;
    if (!dispPtr->placeInit) {
	Tcl_InitHashTable(&dispPtr->masterTable, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&dispPtr->slaveTable, TCL_ONE_WORD_KEYS);
	dispPtr->placeInit = 1;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], optionStrings,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case PLACE_CONFIGURE:
	if (objc == 3 || objc == 4) {
	    Tcl_Obj *objPtr;

	    slavePtr = FindSlave(tkwin);
	    if (slavePtr == NULL) {
		return TCL_OK;
	    }
	    objPtr = Tk_GetOptionInfo(interp, (char *) slavePtr, optionTable,
		    (objc == 4) ? objv[3] : NULL, tkwin);
	    if (objPtr == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    return TCL_OK;
	}
	return ConfigureSlave(interp, tkwin, optionTable, objc-3, objv+3);

    case PLACE_FORGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "pathName");
	    return TCL_ERROR;
	}
	slavePtr = FindSlave(tkwin);
	if (slavePtr == NULL) {
	    return TCL_OK;
	}
	if ((slavePtr->masterPtr != NULL) &&
		(slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin))) {
	    Tk_UnmaintainGeometry(slavePtr->tkwin, slavePtr->masterPtr->tkwin);
	}
	UnlinkSlave(slavePtr);
	Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->slaveTable,
		(char *) tkwin));
	Tk_DeleteEventHandler(tkwin, StructureNotifyMask, SlaveStructureProc,
		slavePtr);
	Tk_ManageGeometry(tkwin, NULL, NULL);
	Tk_UnmapWindow(tkwin);
	FreeSlave(slavePtr);
	break;

    case PLACE_INFO:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "pathName");
	    return TCL_ERROR;
	}
	return PlaceInfoCommand(interp, tkwin);

    case PLACE_SLAVES: {
	Master *masterPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "pathName");
	    return TCL_ERROR;
	}
	masterPtr = FindMaster(tkwin);
	if (masterPtr != NULL) {
	    Tcl_Obj *listPtr = Tcl_NewObj();

	    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		    slavePtr = slavePtr->nextPtr) {
		Tcl_ListObjAppendElement(NULL, listPtr,
			TkNewWindowObj(slavePtr->tkwin));
	    }
	    Tcl_SetObjResult(interp, listPtr);
	}
	break;
    }
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateSlave --
 *
 *	Given a Tk_Window token, find the Slave structure corresponding to
 *	that token, creating a new one if necessary.
 *
 * Results:
 *	Pointer to the Slave structure.
 *
 * Side effects:
 *	A new Slave structure may be created.
 *
 *----------------------------------------------------------------------
 */

static Slave *
CreateSlave(
    Tk_Window tkwin,		/* Token for desired slave. */
    Tk_OptionTable table)
{
    Tcl_HashEntry *hPtr;
    register Slave *slavePtr;
    int isNew;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_CreateHashEntry(&dispPtr->slaveTable, (char *) tkwin, &isNew);
    if (!isNew) {
	return Tcl_GetHashValue(hPtr);
    }

    /*
     * No preexisting slave structure for that window, so make a new one and
     * populate it with some default values.
     */

    slavePtr = ckalloc(sizeof(Slave));
    memset(slavePtr, 0, sizeof(Slave));
    slavePtr->tkwin = tkwin;
    slavePtr->inTkwin = NULL;
    slavePtr->anchor = TK_ANCHOR_NW;
    slavePtr->borderMode = BM_INSIDE;
    slavePtr->optionTable = table;
    Tcl_SetHashValue(hPtr, slavePtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, SlaveStructureProc,
	    slavePtr);
    return slavePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeSlave --
 *
 *	Frees the resources held by a Slave structure.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory are freed.
 *
 *----------------------------------------------------------------------
 */

static void
FreeSlave(
    Slave *slavePtr)
{
    Tk_FreeConfigOptions((char *) slavePtr, slavePtr->optionTable,
	    slavePtr->tkwin);
    ckfree(slavePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FindSlave --
 *
 *	Given a Tk_Window token, find the Slave structure corresponding to
 *	that token. This is purely a lookup function; it will not create a
 *	record if one does not yet exist.
 *
 * Results:
 *	Pointer to Slave structure; NULL if none exists.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Slave *
FindSlave(
    Tk_Window tkwin)		/* Token for desired slave. */
{
    register Tcl_HashEntry *hPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_FindHashEntry(&dispPtr->slaveTable, (char *) tkwin);
    if (hPtr == NULL) {
	return NULL;
    }
    return Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * UnlinkSlave --
 *
 *	This function removes a slave window from the chain of slaves in its
 *	master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The slave list of slavePtr's master changes.
 *
 *----------------------------------------------------------------------
 */

static void
UnlinkSlave(
    Slave *slavePtr)		/* Slave structure to be unlinked. */
{
    register Master *masterPtr;
    register Slave *prevPtr;

    masterPtr = slavePtr->masterPtr;
    if (masterPtr == NULL) {
	return;
    }
    if (masterPtr->slavePtr == slavePtr) {
	masterPtr->slavePtr = slavePtr->nextPtr;
    } else {
	for (prevPtr = masterPtr->slavePtr; ; prevPtr = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
		Tcl_Panic("UnlinkSlave couldn't find slave to unlink");
	    }
	    if (prevPtr->nextPtr == slavePtr) {
		prevPtr->nextPtr = slavePtr->nextPtr;
		break;
	    }
	}
    }

    if (masterPtr->abortPtr != NULL) {
	*masterPtr->abortPtr = 1;
    }
    slavePtr->masterPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateMaster --
 *
 *	Given a Tk_Window token, find the Master structure corresponding to
 *	that token, creating a new one if necessary.
 *
 * Results:
 *	Pointer to the Master structure.
 *
 * Side effects:
 *	A new Master structure may be created.
 *
 *----------------------------------------------------------------------
 */

static Master *
CreateMaster(
    Tk_Window tkwin)		/* Token for desired master. */
{
    Tcl_HashEntry *hPtr;
    register Master *masterPtr;
    int isNew;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_CreateHashEntry(&dispPtr->masterTable, (char *) tkwin, &isNew);
    if (isNew) {
	masterPtr = ckalloc(sizeof(Master));
	masterPtr->tkwin = tkwin;
	masterPtr->slavePtr = NULL;
	masterPtr->abortPtr = NULL;
	masterPtr->flags = 0;
	Tcl_SetHashValue(hPtr, masterPtr);
	Tk_CreateEventHandler(masterPtr->tkwin, StructureNotifyMask,
		MasterStructureProc, masterPtr);
    } else {
	masterPtr = Tcl_GetHashValue(hPtr);
    }
    return masterPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FindMaster --
 *
 *	Given a Tk_Window token, find the Master structure corresponding to
 *	that token. This is simply a lookup function; a new record will not be
 *	created if one does not already exist.
 *
 * Results:
 *	Pointer to the Master structure; NULL if one does not exist for the
 *	given Tk_Window token.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Master *
FindMaster(
    Tk_Window tkwin)		/* Token for desired master. */
{
    register Tcl_HashEntry *hPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_FindHashEntry(&dispPtr->masterTable, (char *) tkwin);
    if (hPtr == NULL) {
	return NULL;
    }
    return Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureSlave --
 *
 *	This function is called to process an argv/argc list to reconfigure
 *	the placement of a window.
 *
 * Results:
 *	A standard Tcl result. If an error occurs then a message is left in
 *	the interp's result.
 *
 * Side effects:
 *	Information in slavePtr may change, and slavePtr's master is scheduled
 *	for reconfiguration.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureSlave(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_Window tkwin,		/* Token for the window to manipulate. */
    Tk_OptionTable table,	/* Token for option table. */
    int objc,			/* Number of config arguments. */
    Tcl_Obj *const objv[])	/* Object values for arguments. */
{
    register Master *masterPtr;
    Tk_SavedOptions savedOptions;
    int mask;
    Slave *slavePtr;
    Tk_Window masterWin = NULL;
    TkWindow *master;

    if (Tk_TopWinHierarchy(tkwin)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"can't use placer on top-level window \"%s\"; use "
		"wm command instead", Tk_PathName(tkwin)));
	Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "TOPLEVEL", NULL);
	return TCL_ERROR;
    }

    slavePtr = CreateSlave(tkwin, table);

    if (Tk_SetOptions(interp, (char *) slavePtr, table, objc, objv,
	    slavePtr->tkwin, &savedOptions, &mask) != TCL_OK) {
	goto error;
    }

    /*
     * Set slave flags. First clear the field, then add bits as needed.
     */

    slavePtr->flags = 0;
    if (slavePtr->heightPtr) {
	slavePtr->flags |= CHILD_HEIGHT;
    }

    if (slavePtr->relHeightPtr) {
	slavePtr->flags |= CHILD_REL_HEIGHT;
    }

    if (slavePtr->relWidthPtr) {
	slavePtr->flags |= CHILD_REL_WIDTH;
    }

    if (slavePtr->widthPtr) {
	slavePtr->flags |= CHILD_WIDTH;
    }

    if (!(mask & IN_MASK) && (slavePtr->masterPtr != NULL)) {
	/*
	 * If no -in option was passed and the slave is already placed then
	 * just recompute the placement.
	 */

	masterPtr = slavePtr->masterPtr;
	goto scheduleLayout;
    } else if (mask & IN_MASK) {
	/* -in changed */
	Tk_Window tkwin;
	Tk_Window ancestor;

	tkwin = slavePtr->inTkwin;

	/*
	 * Make sure that the new master is either the logical parent of the
	 * slave or a descendant of that window, and that the master and slave
	 * aren't the same.
	 */

	for (ancestor = tkwin; ; ancestor = Tk_Parent(ancestor)) {
	    if (ancestor == Tk_Parent(slavePtr->tkwin)) {
		break;
	    }
	    if (Tk_TopWinHierarchy(ancestor)) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"can't place %s relative to %s",
			Tk_PathName(slavePtr->tkwin), Tk_PathName(tkwin)));
		Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "HIERARCHY", NULL);
		goto error;
	    }
	}
	if (slavePtr->tkwin == tkwin) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't place %s relative to itself",
		    Tk_PathName(slavePtr->tkwin)));
	    Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "LOOP", NULL);
	    goto error;
	}

	/*
	 * Check for management loops.
	 */

	for (master = (TkWindow *)tkwin; master != NULL;
	     master = (TkWindow *)TkGetGeomMaster(master)) {
	    if (master == (TkWindow *)slavePtr->tkwin) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't put %s inside %s, would cause management loop",
	            Tk_PathName(slavePtr->tkwin), Tk_PathName(tkwin)));
		Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "LOOP", NULL);
		goto error;
	    }
	}
	if (tkwin != Tk_Parent(slavePtr->tkwin)) {
	    ((TkWindow *)slavePtr->tkwin)->maintainerPtr = (TkWindow *)tkwin;
	}

	if ((slavePtr->masterPtr != NULL)
		&& (slavePtr->masterPtr->tkwin == tkwin)) {
	    /*
	     * Re-using same old master. Nothing to do.
	     */

	    masterPtr = slavePtr->masterPtr;
	    goto scheduleLayout;
	}
	if ((slavePtr->masterPtr != NULL) &&
		(slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin))) {
	    Tk_UnmaintainGeometry(slavePtr->tkwin, slavePtr->masterPtr->tkwin);
	}
	UnlinkSlave(slavePtr);
	masterWin = tkwin;
    }

    /*
     * If there's no master specified for this slave, use its Tk_Parent.
     */

    if (masterWin == NULL) {
	masterWin = Tk_Parent(slavePtr->tkwin);
	slavePtr->inTkwin = masterWin;
    }

    /*
     * Manage the slave window in this master.
     */

    masterPtr = CreateMaster(masterWin);
    slavePtr->masterPtr = masterPtr;
    slavePtr->nextPtr = masterPtr->slavePtr;
    masterPtr->slavePtr = slavePtr;
    Tk_ManageGeometry(slavePtr->tkwin, &placerType, slavePtr);

    /*
     * Arrange for the master to be re-arranged at the first idle moment.
     */

  scheduleLayout:
    Tk_FreeSavedOptions(&savedOptions);

    if (!(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	masterPtr->flags |= PARENT_RECONFIG_PENDING;
	Tcl_DoWhenIdle(RecomputePlacement, masterPtr);
    }
    return TCL_OK;

    /*
     * Error while processing some option, cleanup and return.
     */

  error:
    Tk_RestoreSavedOptions(&savedOptions);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * PlaceInfoCommand --
 *
 *	Implementation of the [place info] subcommand. See the user
 *	documentation for information on what it does.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	If the given tkwin is managed by the placer, this function will put
 *	information about that placement in the interp's result.
 *
 *----------------------------------------------------------------------
 */

static int
PlaceInfoCommand(
    Tcl_Interp *interp,		/* Interp into which to place result. */
    Tk_Window tkwin)		/* Token for the window to get info on. */
{
    Slave *slavePtr;
    Tcl_Obj *infoObj;

    slavePtr = FindSlave(tkwin);
    if (slavePtr == NULL) {
	return TCL_OK;
    }
    infoObj = Tcl_NewObj();
    if (slavePtr->masterPtr != NULL) {
	Tcl_AppendToObj(infoObj, "-in", -1);
	Tcl_ListObjAppendElement(NULL, infoObj,
		TkNewWindowObj(slavePtr->masterPtr->tkwin));
	Tcl_AppendToObj(infoObj, " ", -1);
    }
    Tcl_AppendPrintfToObj(infoObj,
	    "-x %d -relx %.4g -y %d -rely %.4g",
	    slavePtr->x, slavePtr->relX, slavePtr->y, slavePtr->relY);
    if (slavePtr->flags & CHILD_WIDTH) {
	Tcl_AppendPrintfToObj(infoObj, " -width %d", slavePtr->width);
    } else {
	Tcl_AppendToObj(infoObj, " -width {}", -1);
    }
    if (slavePtr->flags & CHILD_REL_WIDTH) {
	Tcl_AppendPrintfToObj(infoObj,
		" -relwidth %.4g", slavePtr->relWidth);
    } else {
	Tcl_AppendToObj(infoObj, " -relwidth {}", -1);
    }
    if (slavePtr->flags & CHILD_HEIGHT) {
	Tcl_AppendPrintfToObj(infoObj, " -height %d", slavePtr->height);
    } else {
	Tcl_AppendToObj(infoObj, " -height {}", -1);
    }
    if (slavePtr->flags & CHILD_REL_HEIGHT) {
	Tcl_AppendPrintfToObj(infoObj,
		" -relheight %.4g", slavePtr->relHeight);
    } else {
	Tcl_AppendToObj(infoObj, " -relheight {}", -1);
    }

    Tcl_AppendPrintfToObj(infoObj, " -anchor %s -bordermode %s",
	    Tk_NameOfAnchor(slavePtr->anchor),
	    borderModeStrings[slavePtr->borderMode]);
    Tcl_SetObjResult(interp, infoObj);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RecomputePlacement --
 *
 *	This function is called as a when-idle handler. It recomputes the
 *	geometries of all the slaves of a given master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Windows may change size or shape.
 *
 *----------------------------------------------------------------------
 */

static void
RecomputePlacement(
    ClientData clientData)	/* Pointer to Master record. */
{
    register Master *masterPtr = clientData;
    register Slave *slavePtr;
    int x, y, width, height, tmp;
    int masterWidth, masterHeight, masterX, masterY;
    double x1, y1, x2, y2;
    int abort;			/* May get set to non-zero to abort this
				 * placement operation. */

    masterPtr->flags &= ~PARENT_RECONFIG_PENDING;

    /*
     * Abort any nested call to RecomputePlacement for this window, since
     * we'll do everything necessary here, and set up so this call can be
     * aborted if necessary.
     */

    if (masterPtr->abortPtr != NULL) {
	*masterPtr->abortPtr = 1;
    }
    masterPtr->abortPtr = &abort;
    abort = 0;
    Tcl_Preserve(masterPtr);

    /*
     * Iterate over all the slaves for the master. Each slave's geometry can
     * be computed independently of the other slaves. Changes to the window's
     * structure could cause almost anything to happen, including deleting the
     * parent or child. If this happens, we'll be told to abort.
     */

    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL && !abort;
	    slavePtr = slavePtr->nextPtr) {
	/*
	 * Step 1: compute size and borderwidth of master, taking into account
	 * desired border mode.
	 */

	masterX = masterY = 0;
	masterWidth = Tk_Width(masterPtr->tkwin);
	masterHeight = Tk_Height(masterPtr->tkwin);
	if (slavePtr->borderMode == BM_INSIDE) {
	    masterX = Tk_InternalBorderLeft(masterPtr->tkwin);
	    masterY = Tk_InternalBorderTop(masterPtr->tkwin);
	    masterWidth -= masterX + Tk_InternalBorderRight(masterPtr->tkwin);
	    masterHeight -= masterY +
		    Tk_InternalBorderBottom(masterPtr->tkwin);
	} else if (slavePtr->borderMode == BM_OUTSIDE) {
	    masterX = masterY = -Tk_Changes(masterPtr->tkwin)->border_width;
	    masterWidth -= 2 * masterX;
	    masterHeight -= 2 * masterY;
	}

	/*
	 * Step 2: compute size of slave (outside dimensions including border)
	 * and location of anchor point within master.
	 */

	x1 = slavePtr->x + masterX + (slavePtr->relX*masterWidth);
	x = (int) (x1 + ((x1 > 0) ? 0.5 : -0.5));
	y1 = slavePtr->y + masterY + (slavePtr->relY*masterHeight);
	y = (int) (y1 + ((y1 > 0) ? 0.5 : -0.5));
	if (slavePtr->flags & (CHILD_WIDTH|CHILD_REL_WIDTH)) {
	    width = 0;
	    if (slavePtr->flags & CHILD_WIDTH) {
		width += slavePtr->width;
	    }
	    if (slavePtr->flags & CHILD_REL_WIDTH) {
		/*
		 * The code below is a bit tricky. In order to round correctly
		 * when both relX and relWidth are specified, compute the
		 * location of the right edge and round that, then compute
		 * width. If we compute the width and round it, rounding
		 * errors in relX and relWidth accumulate.
		 */

		x2 = x1 + (slavePtr->relWidth*masterWidth);
		tmp = (int) (x2 + ((x2 > 0) ? 0.5 : -0.5));
		width += tmp - x;
	    }
	} else {
	    width = Tk_ReqWidth(slavePtr->tkwin)
		    + 2*Tk_Changes(slavePtr->tkwin)->border_width;
	}
	if (slavePtr->flags & (CHILD_HEIGHT|CHILD_REL_HEIGHT)) {
	    height = 0;
	    if (slavePtr->flags & CHILD_HEIGHT) {
		height += slavePtr->height;
	    }
	    if (slavePtr->flags & CHILD_REL_HEIGHT) {
		/*
		 * See note above for rounding errors in width computation.
		 */

		y2 = y1 + (slavePtr->relHeight*masterHeight);
		tmp = (int) (y2 + ((y2 > 0) ? 0.5 : -0.5));
		height += tmp - y;
	    }
	} else {
	    height = Tk_ReqHeight(slavePtr->tkwin)
		    + 2*Tk_Changes(slavePtr->tkwin)->border_width;
	}

	/*
	 * Step 3: adjust the x and y positions so that the desired anchor
	 * point on the slave appears at that position. Also adjust for the
	 * border mode and master's border.
	 */

	switch (slavePtr->anchor) {
	case TK_ANCHOR_N:
	    x -= width/2;
	    break;
	case TK_ANCHOR_NE:
	    x -= width;
	    break;
	case TK_ANCHOR_E:
	    x -= width;
	    y -= height/2;
	    break;
	case TK_ANCHOR_SE:
	    x -= width;
	    y -= height;
	    break;
	case TK_ANCHOR_S:
	    x -= width/2;
	    y -= height;
	    break;
	case TK_ANCHOR_SW:
	    y -= height;
	    break;
	case TK_ANCHOR_W:
	    y -= height/2;
	    break;
	case TK_ANCHOR_NW:
	    break;
	case TK_ANCHOR_CENTER:
	    x -= width/2;
	    y -= height/2;
	    break;
	}

	/*
	 * Step 4: adjust width and height again to reflect inside dimensions
	 * of window rather than outside. Also make sure that the width and
	 * height aren't zero.
	 */

	width -= 2*Tk_Changes(slavePtr->tkwin)->border_width;
	height -= 2*Tk_Changes(slavePtr->tkwin)->border_width;
	if (width <= 0) {
	    width = 1;
	}
	if (height <= 0) {
	    height = 1;
	}

	/*
	 * Step 5: reconfigure the window and map it if needed. If the slave
	 * is a child of the master, we do this ourselves. If the slave isn't
	 * a child of the master, let Tk_MaintainGeometry do the work (it will
	 * re-adjust things as relevant windows map, unmap, and move).
	 */

	if (masterPtr->tkwin == Tk_Parent(slavePtr->tkwin)) {
	    if ((x != Tk_X(slavePtr->tkwin))
		    || (y != Tk_Y(slavePtr->tkwin))
		    || (width != Tk_Width(slavePtr->tkwin))
		    || (height != Tk_Height(slavePtr->tkwin))) {
		Tk_MoveResizeWindow(slavePtr->tkwin, x, y, width, height);
	    }
            if (abort) {
                break;
            }

	    /*
	     * Don't map the slave unless the master is mapped: the slave will
	     * get mapped later, when the master is mapped.
	     */

	    if (Tk_IsMapped(masterPtr->tkwin)) {
		Tk_MapWindow(slavePtr->tkwin);
	    }
	} else {
	    if ((width <= 0) || (height <= 0)) {
		Tk_UnmaintainGeometry(slavePtr->tkwin, masterPtr->tkwin);
		Tk_UnmapWindow(slavePtr->tkwin);
	    } else {
		Tk_MaintainGeometry(slavePtr->tkwin, masterPtr->tkwin,
			x, y, width, height);
	    }
	}
    }

    masterPtr->abortPtr = NULL;
    Tcl_Release(masterPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * MasterStructureProc --
 *
 *	This function is invoked by the Tk event handler when StructureNotify
 *	events occur for a master window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Structures get cleaned up if the window was deleted. If the window was
 *	resized then slave geometries get recomputed.
 *
 *----------------------------------------------------------------------
 */

static void
MasterStructureProc(
    ClientData clientData,	/* Pointer to Master structure for window
				 * referred to by eventPtr. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    register Master *masterPtr = clientData;
    register Slave *slavePtr, *nextPtr;
    TkDisplay *dispPtr = ((TkWindow *) masterPtr->tkwin)->dispPtr;

    switch (eventPtr->type) {
    case ConfigureNotify:
	if ((masterPtr->slavePtr != NULL)
		&& !(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	    masterPtr->flags |= PARENT_RECONFIG_PENDING;
	    Tcl_DoWhenIdle(RecomputePlacement, masterPtr);
	}
	return;
    case DestroyNotify:
	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		slavePtr = nextPtr) {
	    slavePtr->masterPtr = NULL;
	    nextPtr = slavePtr->nextPtr;
	    slavePtr->nextPtr = NULL;
	}
	Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->masterTable,
		(char *) masterPtr->tkwin));
	if (masterPtr->flags & PARENT_RECONFIG_PENDING) {
	    Tcl_CancelIdleCall(RecomputePlacement, masterPtr);
	}
	masterPtr->tkwin = NULL;
	if (masterPtr->abortPtr != NULL) {
	    *masterPtr->abortPtr = 1;
	}
	Tcl_EventuallyFree(masterPtr, TCL_DYNAMIC);
	return;
    case MapNotify:
	/*
	 * When a master gets mapped, must redo the geometry computation so
	 * that all of its slaves get remapped.
	 */

	if ((masterPtr->slavePtr != NULL)
		&& !(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	    masterPtr->flags |= PARENT_RECONFIG_PENDING;
	    Tcl_DoWhenIdle(RecomputePlacement, masterPtr);
	}
	return;
    case UnmapNotify:
	/*
	 * Unmap all of the slaves when the master gets unmapped, so that they
	 * don't keep redisplaying themselves.
	 */

	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		slavePtr = slavePtr->nextPtr) {
	    Tk_UnmapWindow(slavePtr->tkwin);
	}
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveStructureProc --
 *
 *	This function is invoked by the Tk event handler when StructureNotify
 *	events occur for a slave window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Structures get cleaned up if the window was deleted.
 *
 *----------------------------------------------------------------------
 */

static void
SlaveStructureProc(
    ClientData clientData,	/* Pointer to Slave structure for window
				 * referred to by eventPtr. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    register Slave *slavePtr = clientData;
    TkDisplay *dispPtr = ((TkWindow *) slavePtr->tkwin)->dispPtr;

    if (eventPtr->type == DestroyNotify) {
	if (slavePtr->masterPtr != NULL) {
	    UnlinkSlave(slavePtr);
	}
	Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->slaveTable,
		(char *) slavePtr->tkwin));
	FreeSlave(slavePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PlaceRequestProc --
 *
 *	This function is invoked by Tk whenever a slave managed by us changes
 *	its requested geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window will get relayed out, if its requested size has anything to
 *	do with its actual size.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
PlaceRequestProc(
    ClientData clientData,	/* Pointer to our record for slave. */
    Tk_Window tkwin)		/* Window that changed its desired size. */
{
    Slave *slavePtr = clientData;
    Master *masterPtr;

    if ((slavePtr->flags & (CHILD_WIDTH|CHILD_REL_WIDTH))
	    && (slavePtr->flags & (CHILD_HEIGHT|CHILD_REL_HEIGHT))) {
        /*
         * Send a ConfigureNotify to indicate that the size change
         * request was rejected.
         */

        TkDoConfigureNotify((TkWindow *)(slavePtr->tkwin));
	return;
    }
    masterPtr = slavePtr->masterPtr;
    if (masterPtr == NULL) {
	return;
    }
    if (!(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	masterPtr->flags |= PARENT_RECONFIG_PENDING;
	Tcl_DoWhenIdle(RecomputePlacement, masterPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * PlaceLostSlaveProc --
 *
 *	This function is invoked by Tk whenever some other geometry claims
 *	control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all placer-related information about the slave.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
PlaceLostSlaveProc(
    ClientData clientData,	/* Slave structure for slave window that was
				 * stolen away. */
    Tk_Window tkwin)		/* Tk's handle for the slave window. */
{
    register Slave *slavePtr = clientData;
    TkDisplay *dispPtr = ((TkWindow *) slavePtr->tkwin)->dispPtr;

    if (slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin)) {
	Tk_UnmaintainGeometry(slavePtr->tkwin, slavePtr->masterPtr->tkwin);
    }
    Tk_UnmapWindow(tkwin);
    UnlinkSlave(slavePtr);
    Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->slaveTable,
	    (char *) tkwin));
    Tk_DeleteEventHandler(tkwin, StructureNotifyMask, SlaveStructureProc,
	    slavePtr);
    FreeSlave(slavePtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
