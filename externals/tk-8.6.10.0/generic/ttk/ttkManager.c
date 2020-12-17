/*
 * Copyright 2005, Joe English.  Freely redistributable.
 *
 * Support routines for geometry managers.
 */

#include <string.h>
#include <tk.h>
#include "ttkManager.h"

/*------------------------------------------------------------------------
 * +++ The Geometry Propagation Dance.
 *
 * When a slave window requests a new size or some other parameter changes,
 * the manager recomputes the required size for the master window and calls
 * Tk_GeometryRequest().  This is scheduled as an idle handler so multiple
 * updates can be processed as a single batch.
 *
 * If all goes well, the master's manager will process the request
 * (and so on up the chain to the toplevel window), and the master
 * window will eventually receive a <Configure> event.  At this point
 * it recomputes the size and position of all slaves and places them.
 *
 * If all does not go well, however, the master's request may be ignored
 * (typically because the top-level window has a fixed, user-specified size).
 * Tk doesn't provide any notification when this happens; to account for this,
 * we also schedule an idle handler to call the layout procedure
 * after making a geometry request.
 *
 * +++ Slave removal <<NOTE-LOSTSLAVE>>.
 *
 * There are three conditions under which a slave is removed:
 *
 * (1) Another GM claims control
 * (2) Manager voluntarily relinquishes control
 * (3) Slave is destroyed
 *
 * In case (1), Tk calls the manager's lostSlaveProc.
 * Case (2) is performed by calling Tk_ManageGeometry(slave,NULL,0);
 * in this case Tk does _not_ call the LostSlaveProc (documented behavior).
 * Tk doesn't handle case (3) either; to account for that we
 * register an event handler on the slave widget to track <Destroy> events.
 */

/* ++ Data structures.
 */
typedef struct
{
    Tk_Window 		slaveWindow;
    Ttk_Manager 	*manager;
    void 		*slaveData;
    unsigned		flags;
} Ttk_Slave;

/* slave->flags bits:
 */
#define SLAVE_MAPPED 		0x1	/* slave to be mapped when master is */

struct TtkManager_
{
    Ttk_ManagerSpec	*managerSpec;
    void 		*managerData;
    Tk_Window   	masterWindow;
    unsigned		flags;
    int 	 	nSlaves;
    Ttk_Slave 		**slaves;
};

/* manager->flags bits:
 */
#define MGR_UPDATE_PENDING	0x1
#define MGR_RESIZE_REQUIRED	0x2
#define MGR_RELAYOUT_REQUIRED	0x4

static void ManagerIdleProc(void *);	/* forward */

/* ++ ScheduleUpdate --
 * 	Schedule a call to recompute the size and/or layout,
 *	depending on flags.
 */
static void ScheduleUpdate(Ttk_Manager *mgr, unsigned flags)
{
    if (!(mgr->flags & MGR_UPDATE_PENDING)) {
	Tcl_DoWhenIdle(ManagerIdleProc, mgr);
	mgr->flags |= MGR_UPDATE_PENDING;
    }
    mgr->flags |= flags;
}

/* ++ RecomputeSize --
 * 	Recomputes the required size of the master window,
 * 	makes geometry request.
 */
static void RecomputeSize(Ttk_Manager *mgr)
{
    int width = 1, height = 1;

    if (mgr->managerSpec->RequestedSize(mgr->managerData, &width, &height)) {
	Tk_GeometryRequest(mgr->masterWindow, width, height);
	ScheduleUpdate(mgr, MGR_RELAYOUT_REQUIRED);
    }
    mgr->flags &= ~MGR_RESIZE_REQUIRED;
}

/* ++ RecomputeLayout --
 * 	Recompute geometry of all slaves.
 */
static void RecomputeLayout(Ttk_Manager *mgr)
{
    mgr->managerSpec->PlaceSlaves(mgr->managerData);
    mgr->flags &= ~MGR_RELAYOUT_REQUIRED;
}

/* ++ ManagerIdleProc --
 * 	DoWhenIdle procedure for deferred updates.
 */
static void ManagerIdleProc(ClientData clientData)
{
    Ttk_Manager *mgr = clientData;
    mgr->flags &= ~MGR_UPDATE_PENDING;

    if (mgr->flags & MGR_RESIZE_REQUIRED) {
	RecomputeSize(mgr);
    }
    if (mgr->flags & MGR_RELAYOUT_REQUIRED) {
	if (mgr->flags & MGR_UPDATE_PENDING) {
	    /* RecomputeSize has scheduled another update; relayout later */
	    return;
	}
	RecomputeLayout(mgr);
    }
}

/*------------------------------------------------------------------------
 * +++ Event handlers.
 */

/* ++ ManagerEventHandler --
 * 	Recompute slave layout when master widget is resized.
 * 	Keep the slave's map state in sync with the master's.
 */
static const int ManagerEventMask = StructureNotifyMask;
static void ManagerEventHandler(ClientData clientData, XEvent *eventPtr)
{
    Ttk_Manager *mgr = clientData;
    int i;

    switch (eventPtr->type)
    {
	case ConfigureNotify:
	    RecomputeLayout(mgr);
	    break;
	case MapNotify:
	    for (i = 0; i < mgr->nSlaves; ++i) {
		Ttk_Slave *slave = mgr->slaves[i];
		if (slave->flags & SLAVE_MAPPED) {
		    Tk_MapWindow(slave->slaveWindow);
		}
	    }
	    break;
	case UnmapNotify:
	    for (i = 0; i < mgr->nSlaves; ++i) {
		Ttk_Slave *slave = mgr->slaves[i];
		Tk_UnmapWindow(slave->slaveWindow);
	    }
	    break;
    }
}

/* ++ SlaveEventHandler --
 * 	Notifies manager when a slave is destroyed
 * 	(see <<NOTE-LOSTSLAVE>>).
 */
static const unsigned SlaveEventMask = StructureNotifyMask;
static void SlaveEventHandler(ClientData clientData, XEvent *eventPtr)
{
    Ttk_Slave *slave = clientData;
    if (eventPtr->type == DestroyNotify) {
	slave->manager->managerSpec->tkGeomMgr.lostSlaveProc(
	    slave->manager, slave->slaveWindow);
    }
}

/*------------------------------------------------------------------------
 * +++ Slave initialization and cleanup.
 */

static Ttk_Slave *NewSlave(
    Ttk_Manager *mgr, Tk_Window slaveWindow, void *slaveData)
{
    Ttk_Slave *slave = ckalloc(sizeof(*slave));

    slave->slaveWindow = slaveWindow;
    slave->manager = mgr;
    slave->flags = 0;
    slave->slaveData = slaveData;

    return slave;
}

static void DeleteSlave(Ttk_Slave *slave)
{
    ckfree(slave);
}

/*------------------------------------------------------------------------
 * +++ Manager initialization and cleanup.
 */

Ttk_Manager *Ttk_CreateManager(
    Ttk_ManagerSpec *managerSpec, void *managerData, Tk_Window masterWindow)
{
    Ttk_Manager *mgr = ckalloc(sizeof(*mgr));

    mgr->managerSpec 	= managerSpec;
    mgr->managerData	= managerData;
    mgr->masterWindow	= masterWindow;
    mgr->nSlaves 	= 0;
    mgr->slaves 	= NULL;
    mgr->flags  	= 0;

    Tk_CreateEventHandler(
	mgr->masterWindow, ManagerEventMask, ManagerEventHandler, mgr);

    return mgr;
}

void Ttk_DeleteManager(Ttk_Manager *mgr)
{
    Tk_DeleteEventHandler(
	mgr->masterWindow, ManagerEventMask, ManagerEventHandler, mgr);

    while (mgr->nSlaves > 0) {
	Ttk_ForgetSlave(mgr, mgr->nSlaves - 1);
    }
    if (mgr->slaves) {
	ckfree(mgr->slaves);
    }

    Tcl_CancelIdleCall(ManagerIdleProc, mgr);

    ckfree(mgr);
}

/*------------------------------------------------------------------------
 * +++ Slave management.
 */

/* ++ InsertSlave --
 * 	Adds slave to the list of managed windows.
 */
static void InsertSlave(Ttk_Manager *mgr, Ttk_Slave *slave, int index)
{
    int endIndex = mgr->nSlaves++;
    mgr->slaves = ckrealloc(mgr->slaves, mgr->nSlaves * sizeof(Ttk_Slave *));

    while (endIndex > index) {
	mgr->slaves[endIndex] = mgr->slaves[endIndex - 1];
	--endIndex;
    }

    mgr->slaves[index] = slave;

    Tk_ManageGeometry(slave->slaveWindow,
	&mgr->managerSpec->tkGeomMgr, (ClientData)mgr);

    Tk_CreateEventHandler(slave->slaveWindow,
	SlaveEventMask, SlaveEventHandler, (ClientData)slave);

    ScheduleUpdate(mgr, MGR_RESIZE_REQUIRED);
}

/* RemoveSlave --
 * 	Unmanage and delete the slave.
 *
 * NOTES/ASSUMPTIONS:
 *
 * [1] It's safe to call Tk_UnmapWindow / Tk_UnmaintainGeometry even if this
 * routine is called from the slave's DestroyNotify event handler.
 */
static void RemoveSlave(Ttk_Manager *mgr, int index)
{
    Ttk_Slave *slave = mgr->slaves[index];
    int i;

    /* Notify manager:
     */
    mgr->managerSpec->SlaveRemoved(mgr->managerData, index);

    /* Remove from array:
     */
    --mgr->nSlaves;
    for (i = index ; i < mgr->nSlaves; ++i) {
	mgr->slaves[i] = mgr->slaves[i+1];
    }

    /* Clean up:
     */
    Tk_DeleteEventHandler(
	slave->slaveWindow, SlaveEventMask, SlaveEventHandler, slave);

    /* Note [1] */
    Tk_UnmaintainGeometry(slave->slaveWindow, mgr->masterWindow);
    Tk_UnmapWindow(slave->slaveWindow);

    DeleteSlave(slave);

    ScheduleUpdate(mgr, MGR_RESIZE_REQUIRED);
}

/*------------------------------------------------------------------------
 * +++ Tk_GeomMgr hooks.
 */

void Ttk_GeometryRequestProc(ClientData clientData, Tk_Window slaveWindow)
{
    Ttk_Manager *mgr = clientData;
    int slaveIndex = Ttk_SlaveIndex(mgr, slaveWindow);
    int reqWidth = Tk_ReqWidth(slaveWindow);
    int reqHeight= Tk_ReqHeight(slaveWindow);

    if (mgr->managerSpec->SlaveRequest(
		mgr->managerData, slaveIndex, reqWidth, reqHeight)) 
    {
	ScheduleUpdate(mgr, MGR_RESIZE_REQUIRED);
    }
}

void Ttk_LostSlaveProc(ClientData clientData, Tk_Window slaveWindow)
{
    Ttk_Manager *mgr = clientData;
    int index = Ttk_SlaveIndex(mgr, slaveWindow);

    /* ASSERT: index >= 0 */
    RemoveSlave(mgr, index);
}

/*------------------------------------------------------------------------
 * +++ Public API.
 */

/* ++ Ttk_InsertSlave --
 * 	Add a new slave window at the specified index.
 */
void Ttk_InsertSlave(
    Ttk_Manager *mgr, int index, Tk_Window tkwin, void *slaveData)
{
    Ttk_Slave *slave = NewSlave(mgr, tkwin, slaveData);
    InsertSlave(mgr, slave, index);
}

/* ++ Ttk_ForgetSlave --
 * 	Unmanage the specified slave.
 */
void Ttk_ForgetSlave(Ttk_Manager *mgr, int slaveIndex)
{
    Tk_Window slaveWindow = mgr->slaves[slaveIndex]->slaveWindow;
    RemoveSlave(mgr, slaveIndex);
    Tk_ManageGeometry(slaveWindow, NULL, 0);
}

/* ++ Ttk_PlaceSlave --
 * 	Set the position and size of the specified slave window.
 *
 * NOTES:
 * 	Contrary to documentation, Tk_MaintainGeometry doesn't always
 * 	map the slave.
 */
void Ttk_PlaceSlave(
    Ttk_Manager *mgr, int slaveIndex, int x, int y, int width, int height)
{
    Ttk_Slave *slave = mgr->slaves[slaveIndex];
    Tk_MaintainGeometry(slave->slaveWindow,mgr->masterWindow,x,y,width,height);
    slave->flags |= SLAVE_MAPPED;
    if (Tk_IsMapped(mgr->masterWindow)) {
	Tk_MapWindow(slave->slaveWindow);
    }
}

/* ++ Ttk_UnmapSlave --
 * 	Unmap the specified slave, but leave it managed.
 */
void Ttk_UnmapSlave(Ttk_Manager *mgr, int slaveIndex)
{
    Ttk_Slave *slave = mgr->slaves[slaveIndex];
    Tk_UnmaintainGeometry(slave->slaveWindow, mgr->masterWindow);
    slave->flags &= ~SLAVE_MAPPED;
    /* Contrary to documentation, Tk_UnmaintainGeometry doesn't always
     * unmap the slave:
     */
    Tk_UnmapWindow(slave->slaveWindow);
}

/* LayoutChanged, SizeChanged --
 * 	Schedule a relayout, resp. resize request.
 */
void Ttk_ManagerLayoutChanged(Ttk_Manager *mgr)
{
    ScheduleUpdate(mgr, MGR_RELAYOUT_REQUIRED);
}

void Ttk_ManagerSizeChanged(Ttk_Manager *mgr)
{
    ScheduleUpdate(mgr, MGR_RESIZE_REQUIRED);
}

/* +++ Accessors.
 */
int Ttk_NumberSlaves(Ttk_Manager *mgr)
{
    return mgr->nSlaves;
}
void *Ttk_SlaveData(Ttk_Manager *mgr, int slaveIndex)
{
    return mgr->slaves[slaveIndex]->slaveData;
}
Tk_Window Ttk_SlaveWindow(Ttk_Manager *mgr, int slaveIndex)
{
    return mgr->slaves[slaveIndex]->slaveWindow;
}

/*------------------------------------------------------------------------
 * +++ Utility routines.
 */

/* ++ Ttk_SlaveIndex --
 * 	Returns the index of specified slave window, -1 if not found.
 */
int Ttk_SlaveIndex(Ttk_Manager *mgr, Tk_Window slaveWindow)
{
    int index;
    for (index = 0; index < mgr->nSlaves; ++index)
	if (mgr->slaves[index]->slaveWindow == slaveWindow)
	    return index;
    return -1;
}

/* ++ Ttk_GetSlaveIndexFromObj(interp, mgr, objPtr, indexPtr) --
 * 	Return the index of the slave specified by objPtr.
 * 	Slaves may be specified as an integer index or
 * 	as the name of the managed window.
 *
 * Returns:
 * 	Standard Tcl completion code.  Leaves an error message in case of error.
 */

int Ttk_GetSlaveIndexFromObj(
    Tcl_Interp *interp, Ttk_Manager *mgr, Tcl_Obj *objPtr, int *indexPtr)
{
    const char *string = Tcl_GetString(objPtr);
    int slaveIndex = 0;
    Tk_Window tkwin;

    /* Try interpreting as an integer first:
     */
    if (Tcl_GetIntFromObj(NULL, objPtr, &slaveIndex) == TCL_OK) {
	if (slaveIndex < 0 || slaveIndex >= mgr->nSlaves) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Slave index %d out of bounds", slaveIndex));
	    Tcl_SetErrorCode(interp, "TTK", "SLAVE", "INDEX", NULL);
	    return TCL_ERROR;
	}
	*indexPtr = slaveIndex;
	return TCL_OK;
    }

    /* Try interpreting as a slave window name;
     */
    if ((*string == '.') &&
	    (tkwin = Tk_NameToWindow(interp, string, mgr->masterWindow))) {
	slaveIndex = Ttk_SlaveIndex(mgr, tkwin);
	if (slaveIndex < 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "%s is not managed by %s", string,
		    Tk_PathName(mgr->masterWindow)));
	    Tcl_SetErrorCode(interp, "TTK", "SLAVE", "MANAGER", NULL);
	    return TCL_ERROR;
	}
	*indexPtr = slaveIndex;
	return TCL_OK;
    }

    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "Invalid slave specification %s", string));
    Tcl_SetErrorCode(interp, "TTK", "SLAVE", "SPEC", NULL);
    return TCL_ERROR;
}

/* ++ Ttk_ReorderSlave(mgr, fromIndex, toIndex) --
 * 	Change slave order.
 */
void Ttk_ReorderSlave(Ttk_Manager *mgr, int fromIndex, int toIndex)
{
    Ttk_Slave *moved = mgr->slaves[fromIndex];

    /* Shuffle down: */
    while (fromIndex > toIndex) {
	mgr->slaves[fromIndex] = mgr->slaves[fromIndex - 1];
	--fromIndex;
    }
    /* Or, shuffle up: */
    while (fromIndex < toIndex) {
	mgr->slaves[fromIndex] = mgr->slaves[fromIndex + 1];
	++fromIndex;
    }
    /* ASSERT: fromIndex == toIndex */
    mgr->slaves[fromIndex] = moved;

    /* Schedule a relayout.  In general, rearranging slaves
     * may also change the size:
     */
    ScheduleUpdate(mgr, MGR_RESIZE_REQUIRED);
}

/* ++ Ttk_Maintainable(interp, slave, master) --
 * 	Utility routine.  Verifies that 'master' may be used to maintain
 *	the geometry of 'slave' via Tk_MaintainGeometry:
 *
 * 	+ 'master' is either 'slave's parent -OR-
 * 	+ 'master is a descendant of 'slave's parent.
 * 	+ 'slave' is not a toplevel window
 * 	+ 'slave' belongs to the same toplevel as 'master'
 *
 * Returns: 1 if OK; otherwise 0, leaving an error message in 'interp'.
 */
int Ttk_Maintainable(Tcl_Interp *interp, Tk_Window slave, Tk_Window master)
{
    Tk_Window ancestor = master, parent = Tk_Parent(slave);

    if (Tk_IsTopLevel(slave) || slave == master) {
	goto badWindow;
    }

    while (ancestor != parent) {
	if (Tk_IsTopLevel(ancestor)) {
	    goto badWindow;
	}
	ancestor = Tk_Parent(ancestor);
    }

    return 1;

badWindow:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("can't add %s as slave of %s",
	    Tk_PathName(slave), Tk_PathName(master)));
    Tcl_SetErrorCode(interp, "TTK", "GEOMETRY", "MAINTAINABLE", NULL);
    return 0;
}

