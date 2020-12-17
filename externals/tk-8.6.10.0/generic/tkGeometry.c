/*
 * tkGeometry.c --
 *
 *	This file contains generic Tk code for geometry management (stuff
 *	that's used by all geometry managers).
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * Data structures of the following type are used by Tk_MaintainGeometry. For
 * each slave managed by Tk_MaintainGeometry, there is one of these structures
 * associated with its master.
 */

typedef struct MaintainSlave {
    Tk_Window slave;		/* The slave window being positioned. */
    Tk_Window master;		/* The master that determines slave's
				 * position; it must be a descendant of
				 * slave's parent. */
    int x, y;			/* Desired position of slave relative to
				 * master. */
    int width, height;		/* Desired dimensions of slave. */
    struct MaintainSlave *nextPtr;
				/* Next in list of Maintains associated with
				 * master. */
} MaintainSlave;

/*
 * For each window that has been specified as a master to Tk_MaintainGeometry,
 * there is a structure of the following type:
 */

typedef struct MaintainMaster {
    Tk_Window ancestor;		/* The lowest ancestor of this window for
				 * which we have *not* created a
				 * StructureNotify handler. May be the same as
				 * the window itself. */
    int checkScheduled;		/* Non-zero means that there is already a call
				 * to MaintainCheckProc scheduled as an idle
				 * handler. */
    MaintainSlave *slavePtr;	/* First in list of all slaves associated with
				 * this master. */
} MaintainMaster;

/*
 * Prototypes for static procedures in this file:
 */

static void		MaintainCheckProc(ClientData clientData);
static void		MaintainMasterProc(ClientData clientData,
			    XEvent *eventPtr);
static void		MaintainSlaveProc(ClientData clientData,
			    XEvent *eventPtr);

/*
 *--------------------------------------------------------------
 *
 * Tk_ManageGeometry --
 *
 *	Arrange for a particular procedure to manage the geometry of a given
 *	slave window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc becomes the new geometry manager for tkwin, replacing any
 *	previous geometry manager. The geometry manager will be notified (by
 *	calling procedures in *mgrPtr) when interesting things happen in the
 *	future. If there was an existing geometry manager for tkwin different
 *	from the new one, it is notified by calling its lostSlaveProc.
 *
 *--------------------------------------------------------------
 */

void
Tk_ManageGeometry(
    Tk_Window tkwin,		/* Window whose geometry is to be managed by
				 * proc. */
    const Tk_GeomMgr *mgrPtr,	/* Static structure describing the geometry
				 * manager. This structure must never go
				 * away. */
    ClientData clientData)	/* Arbitrary one-word argument to pass to
				 * geometry manager procedures. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if ((winPtr->geomMgrPtr != NULL) && (mgrPtr != NULL)
	    && ((winPtr->geomMgrPtr != mgrPtr)
		|| (winPtr->geomData != clientData))
	    && (winPtr->geomMgrPtr->lostSlaveProc != NULL)) {
	winPtr->geomMgrPtr->lostSlaveProc(winPtr->geomData, tkwin);
    }

    winPtr->geomMgrPtr = mgrPtr;
    winPtr->geomData = clientData;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_GeometryRequest --
 *
 *	This procedure is invoked by widget code to indicate its preferences
 *	about the size of a window it manages. In general, widget code should
 *	call this procedure rather than Tk_ResizeWindow.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The geometry manager for tkwin (if any) is invoked to handle the
 *	request. If possible, it will reconfigure tkwin and/or other windows
 *	to satisfy the request. The caller gets no indication of success or
 *	failure, but it will get X events if the window size was actually
 *	changed.
 *
 *--------------------------------------------------------------
 */

void
Tk_GeometryRequest(
    Tk_Window tkwin,		/* Window that geometry information pertains
				 * to. */
    int reqWidth, int reqHeight)/* Minimum desired dimensions for window, in
				 * pixels. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    /*
     * X gets very upset if a window requests a width or height of zero, so
     * rounds requested sizes up to at least 1.
     */

    if (reqWidth <= 0) {
	reqWidth = 1;
    }
    if (reqHeight <= 0) {
	reqHeight = 1;
    }
    if ((reqWidth == winPtr->reqWidth) && (reqHeight == winPtr->reqHeight)) {
	return;
    }
    winPtr->reqWidth = reqWidth;
    winPtr->reqHeight = reqHeight;
    if ((winPtr->geomMgrPtr != NULL)
	    && (winPtr->geomMgrPtr->requestProc != NULL)) {
	winPtr->geomMgrPtr->requestProc(winPtr->geomData, tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetInternalBorderEx --
 *
 *	Notify relevant geometry managers that a window has an internal border
 *	of a given width and that child windows should not be placed on that
 *	border.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The border widths are recorded for the window, and all geometry
 *	managers of all children are notified so that can re-layout, if
 *	necessary.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetInternalBorderEx(
    Tk_Window tkwin,		/* Window that will have internal border. */
    int left, int right,	/* Width of internal border, in pixels. */
    int top, int bottom)
{
    register TkWindow *winPtr = (TkWindow *) tkwin;
    register int changed = 0;

    if (left < 0) {
	left = 0;
    }
    if (left != winPtr->internalBorderLeft) {
	winPtr->internalBorderLeft = left;
	changed = 1;
    }

    if (right < 0) {
	right = 0;
    }
    if (right != winPtr->internalBorderRight) {
	winPtr->internalBorderRight = right;
	changed = 1;
    }

    if (top < 0) {
	top = 0;
    }
    if (top != winPtr->internalBorderTop) {
	winPtr->internalBorderTop = top;
	changed = 1;
    }

    if (bottom < 0) {
	bottom = 0;
    }
    if (bottom != winPtr->internalBorderBottom) {
	winPtr->internalBorderBottom = bottom;
	changed = 1;
    }

    /*
     * All the slaves for which this is the master window must now be
     * repositioned to take account of the new internal border width. To
     * signal all the geometry managers to do this, trigger a ConfigureNotify
     * event. This will cause geometry managers to recompute everything.
     */

    if (changed) {
	TkDoConfigureNotify(winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetInternalBorder --
 *
 *	Notify relevant geometry managers that a window has an internal border
 *	of a given width and that child windows should not be placed on that
 *	border.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The border width is recorded for the window, and all geometry managers
 *	of all children are notified so that can re-layout, if necessary.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetInternalBorder(
    Tk_Window tkwin,		/* Window that will have internal border. */
    int width)			/* Width of internal border, in pixels. */
{
    Tk_SetInternalBorderEx(tkwin, width, width, width, width);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetMinimumRequestSize --
 *
 *	Notify relevant geometry managers that a window has a minimum request
 *	size.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The minimum request size is recorded for the window, and a new size is
 *	requested for the window, if necessary.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetMinimumRequestSize(
    Tk_Window tkwin,		/* Window that will have internal border. */
    int minWidth, int minHeight)/* Minimum requested size, in pixels. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if ((winPtr->minReqWidth == minWidth) &&
	    (winPtr->minReqHeight == minHeight)) {
	return;
    }

    winPtr->minReqWidth = minWidth;
    winPtr->minReqHeight = minHeight;

    /*
     * The changed min size may cause geometry managers to get a different
     * result, so make them recompute. To signal all the geometry managers to
     * do this, just resize the window to its current size. The
     * ConfigureNotify event will cause geometry managers to recompute
     * everything.
     */

    Tk_ResizeWindow(tkwin, Tk_Width(tkwin), Tk_Height(tkwin));
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetGeometryMaster --
 *
 *	Set a geometry master for this window. Only one master may own
 *	a window at any time.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	The geometry master is recorded for the window.
 *
 *----------------------------------------------------------------------
 */

int
TkSetGeometryMaster(
    Tcl_Interp *interp,		/* Current interpreter, for error. */
    Tk_Window tkwin,		/* Window that will have geometry master
				 * set. */
    const char *master)		/* The master identity. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if (winPtr->geomMgrName != NULL &&
	    strcmp(winPtr->geomMgrName, master) == 0) {
	return TCL_OK;
    }
    if (winPtr->geomMgrName != NULL) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "cannot use geometry manager %s inside %s which already"
		    " has slaves managed by %s",
		    master, Tk_PathName(tkwin), winPtr->geomMgrName));
	    Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "FIGHT", NULL);
	}
	return TCL_ERROR;
    }

    winPtr->geomMgrName = ckalloc(strlen(master) + 1);
    strcpy(winPtr->geomMgrName, master);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFreeGeometryMaster --
 *
 *	Remove a geometry master for this window. Only one master may own
 *	a window at any time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The geometry master is cleared for the window.
 *
 *----------------------------------------------------------------------
 */

void
TkFreeGeometryMaster(
    Tk_Window tkwin,		/* Window that will have geometry master
				 * cleared. */
    const char *master)		/* The master identity. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    if (winPtr->geomMgrName != NULL &&
	    strcmp(winPtr->geomMgrName, master) != 0) {
	Tcl_Panic("Trying to free %s from geometry manager %s",
		winPtr->geomMgrName, master);
    }
    if (winPtr->geomMgrName != NULL) {
	ckfree(winPtr->geomMgrName);
	winPtr->geomMgrName = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MaintainGeometry --
 *
 *	This procedure is invoked by geometry managers to handle slaves whose
 *	master's are not their parents. It translates the desired geometry for
 *	the slave into the coordinate system of the parent and respositions
 *	the slave if it isn't already at the right place. Furthermore, it sets
 *	up event handlers so that if the master (or any of its ancestors up to
 *	the slave's parent) is mapped, unmapped, or moved, then the slave will
 *	be adjusted to match.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Event handlers are created and state is allocated to keep track of
 *	slave. Note: if slave was already managed for master by
 *	Tk_MaintainGeometry, then the previous information is replaced with
 *	the new information. The caller must eventually call
 *	Tk_UnmaintainGeometry to eliminate the correspondence (or, the state
 *	is automatically freed when either window is destroyed).
 *
 *----------------------------------------------------------------------
 */

void
Tk_MaintainGeometry(
    Tk_Window slave,		/* Slave for geometry management. */
    Tk_Window master,		/* Master for slave; must be a descendant of
				 * slave's parent. */
    int x, int y,		/* Desired position of slave within master. */
    int width, int height)	/* Desired dimensions for slave. */
{
    Tcl_HashEntry *hPtr;
    MaintainMaster *masterPtr;
    register MaintainSlave *slavePtr;
    int isNew, map;
    Tk_Window ancestor, parent;
    TkDisplay *dispPtr = ((TkWindow *) master)->dispPtr;

    ((TkWindow *)slave)->maintainerPtr = (TkWindow *)master;

    ((TkWindow *)slave)->maintainerPtr = (TkWindow *)master;
    if (master == Tk_Parent(slave)) {
	/*
	 * If the slave is a direct descendant of the master, don't bother
	 * setting up the extra infrastructure for management, just make a
	 * call to Tk_MoveResizeWindow; the parent/child relationship will
	 * take care of the rest.
	 */

	Tk_MoveResizeWindow(slave, x, y, width, height);

	/*
	 * Map the slave if the master is already mapped; otherwise, wait
	 * until the master is mapped later (in which case mapping the slave
	 * is taken care of elsewhere).
	 */

	if (Tk_IsMapped(master)) {
	    Tk_MapWindow(slave);
	}
	return;
    }

    if (!dispPtr->geomInit) {
	dispPtr->geomInit = 1;
	Tcl_InitHashTable(&dispPtr->maintainHashTable, TCL_ONE_WORD_KEYS);
    }

    /*
     * See if there is already a MaintainMaster structure for the master; if
     * not, then create one.
     */

    parent = Tk_Parent(slave);
    hPtr = Tcl_CreateHashEntry(&dispPtr->maintainHashTable,
	    (char *) master, &isNew);
    if (!isNew) {
	masterPtr = Tcl_GetHashValue(hPtr);
    } else {
	masterPtr = ckalloc(sizeof(MaintainMaster));
	masterPtr->ancestor = master;
	masterPtr->checkScheduled = 0;
	masterPtr->slavePtr = NULL;
	Tcl_SetHashValue(hPtr, masterPtr);
    }

    /*
     * Create a MaintainSlave structure for the slave if there isn't already
     * one.
     */

    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
	    slavePtr = slavePtr->nextPtr) {
	if (slavePtr->slave == slave) {
	    goto gotSlave;
	}
    }
    slavePtr = ckalloc(sizeof(MaintainSlave));
    slavePtr->slave = slave;
    slavePtr->master = master;
    slavePtr->nextPtr = masterPtr->slavePtr;
    masterPtr->slavePtr = slavePtr;
    Tk_CreateEventHandler(slave, StructureNotifyMask, MaintainSlaveProc,
	    slavePtr);

    /*
     * Make sure that there are event handlers registered for all the windows
     * between master and slave's parent (including master but not slave's
     * parent). There may already be handlers for master and some of its
     * ancestors (masterPtr->ancestor tells how many).
     */

    for (ancestor = master; ancestor != parent;
	    ancestor = Tk_Parent(ancestor)) {
	if (ancestor == masterPtr->ancestor) {
	    Tk_CreateEventHandler(ancestor, StructureNotifyMask,
		    MaintainMasterProc, masterPtr);
	    masterPtr->ancestor = Tk_Parent(ancestor);
	}
    }

    /*
     * Fill in up-to-date information in the structure, then update the window
     * if it's not currently in the right place or state.
     */

  gotSlave:
    slavePtr->x = x;
    slavePtr->y = y;
    slavePtr->width = width;
    slavePtr->height = height;
    map = 1;
    for (ancestor = slavePtr->master; ; ancestor = Tk_Parent(ancestor)) {
	if (!Tk_IsMapped(ancestor) && (ancestor != parent)) {
	    map = 0;
	}
	if (ancestor == parent) {
	    if ((x != Tk_X(slavePtr->slave))
		    || (y != Tk_Y(slavePtr->slave))
		    || (width != Tk_Width(slavePtr->slave))
		    || (height != Tk_Height(slavePtr->slave))) {
		Tk_MoveResizeWindow(slavePtr->slave, x, y, width, height);
	    }
	    if (map) {
		Tk_MapWindow(slavePtr->slave);
	    } else {
		Tk_UnmapWindow(slavePtr->slave);
	    }
	    break;
	}
	x += Tk_X(ancestor) + Tk_Changes(ancestor)->border_width;
	y += Tk_Y(ancestor) + Tk_Changes(ancestor)->border_width;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_UnmaintainGeometry --
 *
 *	This procedure cancels a previous Tk_MaintainGeometry call, so that
 *	the relationship between slave and master is no longer maintained.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The slave is unmapped and state is released, so that slave won't track
 *	master any more. If we weren't previously managing slave relative to
 *	master, then this procedure has no effect.
 *
 *----------------------------------------------------------------------
 */

void
Tk_UnmaintainGeometry(
    Tk_Window slave,		/* Slave for geometry management. */
    Tk_Window master)		/* Master for slave; must be a descendant of
				 * slave's parent. */
{
    Tcl_HashEntry *hPtr;
    MaintainMaster *masterPtr;
    register MaintainSlave *slavePtr, *prevPtr;
    Tk_Window ancestor;
    TkDisplay *dispPtr = ((TkWindow *) slave)->dispPtr;

    ((TkWindow *)slave)->maintainerPtr = NULL;

    ((TkWindow *)slave)->maintainerPtr = NULL;
    if (master == Tk_Parent(slave)) {
	/*
	 * If the slave is a direct descendant of the master,
	 * Tk_MaintainGeometry will not have set up any of the extra
	 * infrastructure. Don't even bother to look for it, just return.
	 */
	return;
    }

    if (!dispPtr->geomInit) {
	dispPtr->geomInit = 1;
	Tcl_InitHashTable(&dispPtr->maintainHashTable, TCL_ONE_WORD_KEYS);
    }

    if (!(((TkWindow *) slave)->flags & TK_ALREADY_DEAD)) {
	Tk_UnmapWindow(slave);
    }
    hPtr = Tcl_FindHashEntry(&dispPtr->maintainHashTable, (char *) master);
    if (hPtr == NULL) {
	return;
    }
    masterPtr = Tcl_GetHashValue(hPtr);
    slavePtr = masterPtr->slavePtr;
    if (slavePtr->slave == slave) {
	masterPtr->slavePtr = slavePtr->nextPtr;
    } else {
	for (prevPtr = slavePtr, slavePtr = slavePtr->nextPtr; ;
		prevPtr = slavePtr, slavePtr = slavePtr->nextPtr) {
	    if (slavePtr == NULL) {
		return;
	    }
	    if (slavePtr->slave == slave) {
		prevPtr->nextPtr = slavePtr->nextPtr;
		break;
	    }
	}
    }
    Tk_DeleteEventHandler(slavePtr->slave, StructureNotifyMask,
	    MaintainSlaveProc, slavePtr);
    ckfree(slavePtr);
    if (masterPtr->slavePtr == NULL) {
	if (masterPtr->ancestor != NULL) {
	    for (ancestor = master; ; ancestor = Tk_Parent(ancestor)) {
		Tk_DeleteEventHandler(ancestor, StructureNotifyMask,
			MaintainMasterProc, masterPtr);
		if (ancestor == masterPtr->ancestor) {
		    break;
		}
	    }
	}
	if (masterPtr->checkScheduled) {
	    Tcl_CancelIdleCall(MaintainCheckProc, masterPtr);
	}
	Tcl_DeleteHashEntry(hPtr);
	ckfree(masterPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MaintainMasterProc --
 *
 *	This procedure is invoked by the Tk event dispatcher in response to
 *	StructureNotify events on the master or one of its ancestors, on
 *	behalf of Tk_MaintainGeometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	It schedules a call to MaintainCheckProc, which will eventually caused
 *	the postions and mapped states to be recalculated for all the
 *	maintained slaves of the master. Or, if the master window is being
 *	deleted then state is cleaned up.
 *
 *----------------------------------------------------------------------
 */

static void
MaintainMasterProc(
    ClientData clientData,	/* Pointer to MaintainMaster structure for the
				 * master window. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    MaintainMaster *masterPtr = clientData;
    MaintainSlave *slavePtr;
    int done;

    if ((eventPtr->type == ConfigureNotify)
	    || (eventPtr->type == MapNotify)
	    || (eventPtr->type == UnmapNotify)) {
	if (!masterPtr->checkScheduled) {
	    masterPtr->checkScheduled = 1;
	    Tcl_DoWhenIdle(MaintainCheckProc, masterPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	/*
	 * Delete all of the state associated with this master, but be careful
	 * not to use masterPtr after the last slave is deleted, since its
	 * memory will have been freed.
	 */

	done = 0;
	do {
	    slavePtr = masterPtr->slavePtr;
	    if (slavePtr->nextPtr == NULL) {
		done = 1;
	    }
	    Tk_UnmaintainGeometry(slavePtr->slave, slavePtr->master);
	} while (!done);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MaintainSlaveProc --
 *
 *	This procedure is invoked by the Tk event dispatcher in response to
 *	StructureNotify events on a slave being managed by
 *	Tk_MaintainGeometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the event is a DestroyNotify event then the Maintain state and
 *	event handlers for this slave are deleted.
 *
 *----------------------------------------------------------------------
 */

static void
MaintainSlaveProc(
    ClientData clientData,	/* Pointer to MaintainSlave structure for
				 * master-slave pair. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    MaintainSlave *slavePtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	Tk_UnmaintainGeometry(slavePtr->slave, slavePtr->master);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MaintainCheckProc --
 *
 *	This procedure is invoked by the Tk event dispatcher as an idle
 *	handler, when a master or one of its ancestors has been reconfigured,
 *	mapped, or unmapped. Its job is to scan all of the slaves for the
 *	master and reposition them, map them, or unmap them as needed to
 *	maintain their geometry relative to the master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Slaves can get repositioned, mapped, or unmapped.
 *
 *----------------------------------------------------------------------
 */

static void
MaintainCheckProc(
    ClientData clientData)	/* Pointer to MaintainMaster structure for the
				 * master window. */
{
    MaintainMaster *masterPtr = clientData;
    MaintainSlave *slavePtr;
    Tk_Window ancestor, parent;
    int x, y, map;

    masterPtr->checkScheduled = 0;
    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
	    slavePtr = slavePtr->nextPtr) {
	parent = Tk_Parent(slavePtr->slave);
	x = slavePtr->x;
	y = slavePtr->y;
	map = 1;
	for (ancestor = slavePtr->master; ; ancestor = Tk_Parent(ancestor)) {
	    if (!Tk_IsMapped(ancestor) && (ancestor != parent)) {
		map = 0;
	    }
	    if (ancestor == parent) {
		if ((x != Tk_X(slavePtr->slave))
			|| (y != Tk_Y(slavePtr->slave))) {
		    Tk_MoveWindow(slavePtr->slave, x, y);
		}
		if (map) {
		    Tk_MapWindow(slavePtr->slave);
		} else {
		    Tk_UnmapWindow(slavePtr->slave);
		}
		break;
	    }
	    x += Tk_X(ancestor) + Tk_Changes(ancestor)->border_width;
	    y += Tk_Y(ancestor) + Tk_Changes(ancestor)->border_width;
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
