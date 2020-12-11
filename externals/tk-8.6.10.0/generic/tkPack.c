/*
 * tkPack.c --
 *
 *	This file contains code to implement the "packer" geometry manager for
 *	Tk.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

typedef enum {TOP, BOTTOM, LEFT, RIGHT} Side;
static const char *const sideNames[] = {
    "top", "bottom", "left", "right", NULL
};

/*
 * For each window that the packer cares about (either because the window is
 * managed by the packer or because the window has slaves that are managed by
 * the packer), there is a structure of the following type:
 */

typedef struct Packer {
    Tk_Window tkwin;		/* Tk token for window. NULL means that the
				 * window has been deleted, but the packet
				 * hasn't had a chance to clean up yet because
				 * the structure is still in use. */
    struct Packer *masterPtr;	/* Master window within which this window is
				 * packed (NULL means this window isn't
				 * managed by the packer). */
    struct Packer *nextPtr;	/* Next window packed within same master. List
				 * is priority-ordered: first on list gets
				 * packed first. */
    struct Packer *slavePtr;	/* First in list of slaves packed inside this
				 * window (NULL means no packed slaves). */
    Side side;			/* Side of master against which this window is
				 * packed. */
    Tk_Anchor anchor;		/* If frame allocated for window is larger
				 * than window needs, this indicates how where
				 * to position window in frame. */
    int padX, padY;		/* Total additional pixels to leave around the
				 * window. Some is of this space is on each
				 * side. This is space *outside* the window:
				 * we'll allocate extra space in frame but
				 * won't enlarge window). */
    int padLeft, padTop;	/* The part of padX or padY to use on the left
				 * or top of the widget, respectively. By
				 * default, this is half of padX or padY. */
    int iPadX, iPadY;		/* Total extra pixels to allocate inside the
				 * window (half of this amount will appear on
				 * each side). */
    int doubleBw;		/* Twice the window's last known border width.
				 * If this changes, the window must be
				 * repacked within its master. */
    int *abortPtr;		/* If non-NULL, it means that there is a
				 * nested call to ArrangePacking already
				 * working on this window. *abortPtr may be
				 * set to 1 to abort that nested call. This
				 * happens, for example, if tkwin or any of
				 * its slaves is deleted. */
    int flags;			/* Miscellaneous flags; see below for
				 * definitions. */
} Packer;

/*
 * Flag values for Packer structures:
 *
 * REQUESTED_REPACK:		1 means a Tcl_DoWhenIdle request has already
 *				been made to repack all the slaves of this
 *				window.
 * FILLX:			1 means if frame allocated for window is wider
 *				than window needs, expand window to fill
 *				frame. 0 means don't make window any larger
 *				than needed.
 * FILLY:			Same as FILLX, except for height.
 * EXPAND:			1 means this window's frame will absorb any
 *				extra space in the master window.
 * OLD_STYLE:			1 means this window is being managed with the
 *				old-style packer algorithms (before Tk version
 *				3.3). The main difference is that padding and
 *				filling are done differently.
 * DONT_PROPAGATE:		1 means don't set this window's requested
 *				size. 0 means if this window is a master then
 *				Tk will set its requested size to fit the
 *				needs of its slaves.
 * ALLOCED_MASTER               1 means that Pack has allocated itself as
 *                              geometry master for this window.
 */

#define REQUESTED_REPACK	1
#define FILLX			2
#define FILLY			4
#define EXPAND			8
#define OLD_STYLE		16
#define DONT_PROPAGATE		32
#define ALLOCED_MASTER		64

/*
 * The following structure is the official type record for the packer:
 */

static void		PackReqProc(ClientData clientData, Tk_Window tkwin);
static void		PackLostSlaveProc(ClientData clientData,
			    Tk_Window tkwin);

static const Tk_GeomMgr packerType = {
    "pack",			/* name */
    PackReqProc,		/* requestProc */
    PackLostSlaveProc,		/* lostSlaveProc */
};

/*
 * Forward declarations for functions defined later in this file:
 */

static void		ArrangePacking(ClientData clientData);
static int		ConfigureSlaves(Tcl_Interp *interp, Tk_Window tkwin,
			    int objc, Tcl_Obj *const objv[]);
static void		DestroyPacker(void *memPtr);
static Packer *		GetPacker(Tk_Window tkwin);
static int		PackAfter(Tcl_Interp *interp, Packer *prevPtr,
			    Packer *masterPtr, int objc,Tcl_Obj *const objv[]);
static void		PackStructureProc(ClientData clientData,
			    XEvent *eventPtr);
static void		Unlink(Packer *packPtr);
static int		XExpansion(Packer *slavePtr, int cavityWidth);
static int		YExpansion(Packer *slavePtr, int cavityHeight);

/*
 *------------------------------------------------------------------------
 *
 * TkAppendPadAmount --
 *
 *	This function generates a text value that describes one of the -padx,
 *	-pady, -ipadx, or -ipady configuration options. The text value
 *	generated is appended to the given Tcl_Obj.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *------------------------------------------------------------------------
 */

void
TkAppendPadAmount(
    Tcl_Obj *bufferObj,		/* The interpreter into which the result is
				 * written. */
    const char *switchName,	/* One of "padx", "pady", "ipadx" or
				 * "ipady" */
    int halfSpace,		/* The left or top padding amount */
    int allSpace)		/* The total amount of padding */
{
    Tcl_Obj *padding[2];

    if (halfSpace*2 == allSpace) {
	Tcl_DictObjPut(NULL, bufferObj, Tcl_NewStringObj(switchName, -1),
		Tcl_NewIntObj(halfSpace));
    } else {
	padding[0] = Tcl_NewIntObj(halfSpace);
	padding[1] = Tcl_NewIntObj(allSpace - halfSpace);
	Tcl_DictObjPut(NULL, bufferObj, Tcl_NewStringObj(switchName, -1),
		Tcl_NewListObj(2, padding));
    }
}

/*
 *------------------------------------------------------------------------
 *
 * Tk_PackCmd --
 *
 *	This function is invoked to process the "pack" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *------------------------------------------------------------------------
 */

int
Tk_PackObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    const char *argv2;
    static const char *const optionStrings[] = {
	/* after, append, before and unpack are deprecated */
	"after", "append", "before", "unpack",
	"configure", "forget", "info", "propagate", "slaves", NULL };
    enum options {
	PACK_AFTER, PACK_APPEND, PACK_BEFORE, PACK_UNPACK,
	PACK_CONFIGURE, PACK_FORGET, PACK_INFO, PACK_PROPAGATE, PACK_SLAVES };
    int index;

    if (objc >= 2) {
	const char *string = Tcl_GetString(objv[1]);

	if (string[0] == '.') {
	    return ConfigureSlaves(interp, tkwin, objc-1, objv+1);
	}
    }
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], optionStrings,
	    sizeof(char *), "option", 0, &index) != TCL_OK) {
	/*
	 * Call it again without the deprecated ones to get a proper error
	 * message. This works well since there can't be any ambiguity between
	 * deprecated and new options.
	 */

	Tcl_ResetResult(interp);
	Tcl_GetIndexFromObjStruct(interp, objv[1], &optionStrings[4],
		sizeof(char *), "option", 0, &index);
	return TCL_ERROR;
    }

    argv2 = Tcl_GetString(objv[2]);
    switch ((enum options) index) {
    case PACK_AFTER: {
	Packer *prevPtr;
	Tk_Window tkwin2;

	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	prevPtr = GetPacker(tkwin2);
	if (prevPtr->masterPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" isn't packed", argv2));
	    Tcl_SetErrorCode(interp, "TK", "PACK", "NOT_PACKED", NULL);
	    return TCL_ERROR;
	}
	return PackAfter(interp, prevPtr, prevPtr->masterPtr, objc-3, objv+3);
    }
    case PACK_APPEND: {
	Packer *masterPtr;
	register Packer *prevPtr;
	Tk_Window tkwin2;

	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	masterPtr = GetPacker(tkwin2);
	prevPtr = masterPtr->slavePtr;
	if (prevPtr != NULL) {
	    while (prevPtr->nextPtr != NULL) {
		prevPtr = prevPtr->nextPtr;
	    }
	}
	return PackAfter(interp, prevPtr, masterPtr, objc-3, objv+3);
    }
    case PACK_BEFORE: {
	Packer *packPtr, *masterPtr;
	register Packer *prevPtr;
	Tk_Window tkwin2;

	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	packPtr = GetPacker(tkwin2);
	if (packPtr->masterPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" isn't packed", argv2));
	    Tcl_SetErrorCode(interp, "TK", "PACK", "NOT_PACKED", NULL);
	    return TCL_ERROR;
	}
	masterPtr = packPtr->masterPtr;
	prevPtr = masterPtr->slavePtr;
	if (prevPtr == packPtr) {
	    prevPtr = NULL;
	} else {
	    for ( ; ; prevPtr = prevPtr->nextPtr) {
		if (prevPtr == NULL) {
		    Tcl_Panic("\"pack before\" couldn't find predecessor");
		}
		if (prevPtr->nextPtr == packPtr) {
		    break;
		}
	    }
	}
	return PackAfter(interp, prevPtr, masterPtr, objc-3, objv+3);
    }
    case PACK_CONFIGURE:
	if (argv2[0] != '.') {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad argument \"%s\": must be name of window", argv2));
	    Tcl_SetErrorCode(interp, "TK", "VALUE", "WINDOW_PATH", NULL);
	    return TCL_ERROR;
	}
	return ConfigureSlaves(interp, tkwin, objc-2, objv+2);
    case PACK_FORGET: {
	Tk_Window slave;
	Packer *slavePtr;
	int i;

	for (i = 2; i < objc; i++) {
	    if (TkGetWindowFromObj(interp, tkwin, objv[i], &slave) != TCL_OK) {
		continue;
	    }
	    slavePtr = GetPacker(slave);
	    if ((slavePtr != NULL) && (slavePtr->masterPtr != NULL)) {
		Tk_ManageGeometry(slave, NULL, NULL);
		if (slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin)) {
		    Tk_UnmaintainGeometry(slavePtr->tkwin,
			    slavePtr->masterPtr->tkwin);
		}
		Unlink(slavePtr);
		Tk_UnmapWindow(slavePtr->tkwin);
	    }
	}
	break;
    }
    case PACK_INFO: {
	register Packer *slavePtr;
	Tk_Window slave;
	Tcl_Obj *infoObj;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	if (TkGetWindowFromObj(interp, tkwin, objv[2], &slave) != TCL_OK) {
	    return TCL_ERROR;
	}
	slavePtr = GetPacker(slave);
	if (slavePtr->masterPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "window \"%s\" isn't packed", argv2));
	    Tcl_SetErrorCode(interp, "TK", "PACK", "NOT_PACKED", NULL);
	    return TCL_ERROR;
	}

	infoObj = Tcl_NewObj();
	Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-in", -1),
		TkNewWindowObj(slavePtr->masterPtr->tkwin));
	Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-anchor", -1),
		Tcl_NewStringObj(Tk_NameOfAnchor(slavePtr->anchor), -1));
	Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-expand", -1),
		Tcl_NewBooleanObj(slavePtr->flags & EXPAND));
	switch (slavePtr->flags & (FILLX|FILLY)) {
	case 0:
	    Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-fill", -1),
		    Tcl_NewStringObj("none", -1));
	    break;
	case FILLX:
	    Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-fill", -1),
		    Tcl_NewStringObj("x", -1));
	    break;
	case FILLY:
	    Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-fill", -1),
		    Tcl_NewStringObj("y", -1));
	    break;
	case FILLX|FILLY:
	    Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-fill", -1),
		    Tcl_NewStringObj("both", -1));
	    break;
	}
	TkAppendPadAmount(infoObj, "-ipadx", slavePtr->iPadX/2, slavePtr->iPadX);
	TkAppendPadAmount(infoObj, "-ipady", slavePtr->iPadY/2, slavePtr->iPadY);
	TkAppendPadAmount(infoObj, "-padx", slavePtr->padLeft,slavePtr->padX);
	TkAppendPadAmount(infoObj, "-pady", slavePtr->padTop, slavePtr->padY);
	Tcl_DictObjPut(NULL, infoObj, Tcl_NewStringObj("-side", -1),
		Tcl_NewStringObj(sideNames[slavePtr->side], -1));
	Tcl_SetObjResult(interp, infoObj);
	break;
    }
    case PACK_PROPAGATE: {
	Tk_Window master;
	Packer *masterPtr;
	int propagate;

	if (objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window ?boolean?");
	    return TCL_ERROR;
	}
	if (TkGetWindowFromObj(interp, tkwin, objv[2], &master) != TCL_OK) {
	    return TCL_ERROR;
	}
	masterPtr = GetPacker(master);
	if (objc == 3) {
	    Tcl_SetObjResult(interp,
		    Tcl_NewBooleanObj(!(masterPtr->flags & DONT_PROPAGATE)));
	    return TCL_OK;
	}
	if (Tcl_GetBooleanFromObj(interp, objv[3], &propagate) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (propagate) {
	    /*
	     * If we have slaves, we need to register as geometry master.
	     */

	    if (masterPtr->slavePtr != NULL) {
		if (TkSetGeometryMaster(interp, master, "pack") != TCL_OK) {
		    return TCL_ERROR;
		}
		masterPtr->flags |= ALLOCED_MASTER;
	    }
	    masterPtr->flags &= ~DONT_PROPAGATE;

	    /*
	     * Repack the master to allow new geometry information to
	     * propagate upwards to the master's master.
	     */

	    if (masterPtr->abortPtr != NULL) {
		*masterPtr->abortPtr = 1;
	    }
	    if (!(masterPtr->flags & REQUESTED_REPACK)) {
		masterPtr->flags |= REQUESTED_REPACK;
		Tcl_DoWhenIdle(ArrangePacking, masterPtr);
	    }
	} else {
	    if (masterPtr->flags & ALLOCED_MASTER) {
		TkFreeGeometryMaster(master, "pack");
		masterPtr->flags &= ~ALLOCED_MASTER;
	    }
	    masterPtr->flags |= DONT_PROPAGATE;
	}
	break;
    }
    case PACK_SLAVES: {
	Tk_Window master;
	Packer *masterPtr, *slavePtr;
	Tcl_Obj *resultObj;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	if (TkGetWindowFromObj(interp, tkwin, objv[2], &master) != TCL_OK) {
	    return TCL_ERROR;
	}
	resultObj = Tcl_NewObj();
	masterPtr = GetPacker(master);
	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		slavePtr = slavePtr->nextPtr) {
	    Tcl_ListObjAppendElement(NULL, resultObj,
		    TkNewWindowObj(slavePtr->tkwin));
	}
	Tcl_SetObjResult(interp, resultObj);
	break;
    }
    case PACK_UNPACK: {
	Tk_Window tkwin2;
	Packer *packPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window");
	    return TCL_ERROR;
	}
	if (TkGetWindowFromObj(interp, tkwin, objv[2], &tkwin2) != TCL_OK) {
	    return TCL_ERROR;
	}
	packPtr = GetPacker(tkwin2);
	if ((packPtr != NULL) && (packPtr->masterPtr != NULL)) {
	    Tk_ManageGeometry(tkwin2, NULL, NULL);
	    if (packPtr->masterPtr->tkwin != Tk_Parent(packPtr->tkwin)) {
		Tk_UnmaintainGeometry(packPtr->tkwin,
			packPtr->masterPtr->tkwin);
	    }
	    Unlink(packPtr);
	    Tk_UnmapWindow(packPtr->tkwin);
	}
	break;
    }
    }

    return TCL_OK;
}

/*
 *------------------------------------------------------------------------
 *
 * PackReqProc --
 *
 *	This function is invoked by Tk_GeometryRequest for windows managed by
 *	the packer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for tkwin, and all its managed siblings, to be re-packed at
 *	the next idle point.
 *
 *------------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
PackReqProc(
    ClientData clientData,	/* Packer's information about window that got
				 * new preferred geometry.  */
    Tk_Window tkwin)		/* Other Tk-related information about the
				 * window. */
{
    register Packer *packPtr = clientData;

    packPtr = packPtr->masterPtr;
    if (!(packPtr->flags & REQUESTED_REPACK)) {
	packPtr->flags |= REQUESTED_REPACK;
	Tcl_DoWhenIdle(ArrangePacking, packPtr);
    }
}

/*
 *------------------------------------------------------------------------
 *
 * PackLostSlaveProc --
 *
 *	This function is invoked by Tk whenever some other geometry claims
 *	control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all packer-related information about the slave.
 *
 *------------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
PackLostSlaveProc(
    ClientData clientData,	/* Packer structure for slave window that was
				 * stolen away. */
    Tk_Window tkwin)		/* Tk's handle for the slave window. */
{
    register Packer *slavePtr = clientData;

    if (slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin)) {
	Tk_UnmaintainGeometry(slavePtr->tkwin, slavePtr->masterPtr->tkwin);
    }
    Unlink(slavePtr);
    Tk_UnmapWindow(slavePtr->tkwin);
}

/*
 *------------------------------------------------------------------------
 *
 * ArrangePacking --
 *
 *	This function is invoked (using the Tcl_DoWhenIdle mechanism) to
 *	re-layout a set of windows managed by the packer. It is invoked at
 *	idle time so that a series of packer requests can be merged into a
 *	single layout operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The packed slaves of masterPtr may get resized or moved.
 *
 *------------------------------------------------------------------------
 */

static void
ArrangePacking(
    ClientData clientData)	/* Structure describing master whose slaves
				 * are to be re-layed out. */
{
    register Packer *masterPtr = clientData;
    register Packer *slavePtr;
    int cavityX, cavityY, cavityWidth, cavityHeight;
				/* These variables keep track of the
				 * as-yet-unallocated space remaining in the
				 * middle of the master window. */
    int frameX, frameY, frameWidth, frameHeight;
				/* These variables keep track of the frame
				 * allocated to the current window. */
    int x, y, width, height;	/* These variables are used to hold the actual
				 * geometry of the current window. */
    int abort;			/* May get set to non-zero to abort this
				 * repacking operation. */
    int borderX, borderY;
    int borderTop, borderBtm;
    int borderLeft, borderRight;
    int maxWidth, maxHeight, tmp;

    masterPtr->flags &= ~REQUESTED_REPACK;

    /*
     * If the master has no slaves anymore, then don't do anything at all:
     * just leave the master's size as-is.
     */

    if (masterPtr->slavePtr == NULL) {
	return;
    }

    /*
     * Abort any nested call to ArrangePacking for this window, since we'll do
     * everything necessary here, and set up so this call can be aborted if
     * necessary.
     */

    if (masterPtr->abortPtr != NULL) {
	*masterPtr->abortPtr = 1;
    }
    masterPtr->abortPtr = &abort;
    abort = 0;
    Tcl_Preserve(masterPtr);

    /*
     * Pass #1: scan all the slaves to figure out the total amount of space
     * needed. Two separate width and height values are computed:
     *
     * width -		Holds the sum of the widths (plus padding) of all the
     *			slaves seen so far that were packed LEFT or RIGHT.
     * height -		Holds the sum of the heights (plus padding) of all the
     *			slaves seen so far that were packed TOP or BOTTOM.
     *
     * maxWidth -	Gradually builds up the width needed by the master to
     *			just barely satisfy all the slave's needs. For each
     *			slave, the code computes the width needed for all the
     *			slaves so far and updates maxWidth if the new value is
     *			greater.
     * maxHeight -	Same as maxWidth, except keeps height info.
     */

    width = maxWidth = Tk_InternalBorderLeft(masterPtr->tkwin) +
	    Tk_InternalBorderRight(masterPtr->tkwin);
    height = maxHeight = Tk_InternalBorderTop(masterPtr->tkwin) +
	    Tk_InternalBorderBottom(masterPtr->tkwin);
    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
	    slavePtr = slavePtr->nextPtr) {
	if ((slavePtr->side == TOP) || (slavePtr->side == BOTTOM)) {
	    tmp = Tk_ReqWidth(slavePtr->tkwin) + slavePtr->doubleBw
		    + slavePtr->padX + slavePtr->iPadX + width;
	    if (tmp > maxWidth) {
		maxWidth = tmp;
	    }
	    height += Tk_ReqHeight(slavePtr->tkwin) + slavePtr->doubleBw
		    + slavePtr->padY + slavePtr->iPadY;
	} else {
	    tmp = Tk_ReqHeight(slavePtr->tkwin) + slavePtr->doubleBw
		    + slavePtr->padY + slavePtr->iPadY + height;
	    if (tmp > maxHeight) {
		maxHeight = tmp;
	    }
	    width += Tk_ReqWidth(slavePtr->tkwin) + slavePtr->doubleBw
		    + slavePtr->padX + slavePtr->iPadX;
	}
    }
    if (width > maxWidth) {
	maxWidth = width;
    }
    if (height > maxHeight) {
	maxHeight = height;
    }

    if (maxWidth < Tk_MinReqWidth(masterPtr->tkwin)) {
	maxWidth = Tk_MinReqWidth(masterPtr->tkwin);
    }
    if (maxHeight < Tk_MinReqHeight(masterPtr->tkwin)) {
	maxHeight = Tk_MinReqHeight(masterPtr->tkwin);
    }

    /*
     * If the total amount of space needed in the master window has changed,
     * and if we're propagating geometry information, then notify the next
     * geometry manager up and requeue ourselves to start again after the
     * master has had a chance to resize us.
     */

    if (((maxWidth != Tk_ReqWidth(masterPtr->tkwin))
	    || (maxHeight != Tk_ReqHeight(masterPtr->tkwin)))
	    && !(masterPtr->flags & DONT_PROPAGATE)) {
	Tk_GeometryRequest(masterPtr->tkwin, maxWidth, maxHeight);
	masterPtr->flags |= REQUESTED_REPACK;
	Tcl_DoWhenIdle(ArrangePacking, masterPtr);
	goto done;
    }

    /*
     * Pass #2: scan the slaves a second time assigning new sizes. The
     * "cavity" variables keep track of the unclaimed space in the cavity of
     * the window; this shrinks inward as we allocate windows around the
     * edges. The "frame" variables keep track of the space allocated to the
     * current window and its frame. The current window is then placed
     * somewhere inside the frame, depending on anchor.
     */

    cavityX = x = Tk_InternalBorderLeft(masterPtr->tkwin);
    cavityY = y = Tk_InternalBorderTop(masterPtr->tkwin);
    cavityWidth = Tk_Width(masterPtr->tkwin) -
	    Tk_InternalBorderLeft(masterPtr->tkwin) -
	    Tk_InternalBorderRight(masterPtr->tkwin);
    cavityHeight = Tk_Height(masterPtr->tkwin) -
	    Tk_InternalBorderTop(masterPtr->tkwin) -
	    Tk_InternalBorderBottom(masterPtr->tkwin);
    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
	    slavePtr = slavePtr->nextPtr) {
	if ((slavePtr->side == TOP) || (slavePtr->side == BOTTOM)) {
	    frameWidth = cavityWidth;
	    frameHeight = Tk_ReqHeight(slavePtr->tkwin) + slavePtr->doubleBw
		    + slavePtr->padY + slavePtr->iPadY;
	    if (slavePtr->flags & EXPAND) {
		frameHeight += YExpansion(slavePtr, cavityHeight);
	    }
	    cavityHeight -= frameHeight;
	    if (cavityHeight < 0) {
		frameHeight += cavityHeight;
		cavityHeight = 0;
	    }
	    frameX = cavityX;
	    if (slavePtr->side == TOP) {
		frameY = cavityY;
		cavityY += frameHeight;
	    } else {
		frameY = cavityY + cavityHeight;
	    }
	} else {
	    frameHeight = cavityHeight;
	    frameWidth = Tk_ReqWidth(slavePtr->tkwin) + slavePtr->doubleBw
		    + slavePtr->padX + slavePtr->iPadX;
	    if (slavePtr->flags & EXPAND) {
		frameWidth += XExpansion(slavePtr, cavityWidth);
	    }
	    cavityWidth -= frameWidth;
	    if (cavityWidth < 0) {
		frameWidth += cavityWidth;
		cavityWidth = 0;
	    }
	    frameY = cavityY;
	    if (slavePtr->side == LEFT) {
		frameX = cavityX;
		cavityX += frameWidth;
	    } else {
		frameX = cavityX + cavityWidth;
	    }
	}

	/*
	 * Now that we've got the size of the frame for the window, compute
	 * the window's actual size and location using the fill, padding, and
	 * frame factors. The variables "borderX" and "borderY" are used to
	 * handle the differences between old-style packing and the new style
	 * (in old-style, iPadX and iPadY are always zero and padding is
	 * completely ignored except when computing frame size).
	 */

	if (slavePtr->flags & OLD_STYLE) {
	    borderX = borderY = 0;
	    borderTop = borderBtm = 0;
	    borderLeft = borderRight = 0;
	} else {
	    borderX = slavePtr->padX;
	    borderY = slavePtr->padY;
	    borderLeft = slavePtr->padLeft;
	    borderRight = borderX - borderLeft;
	    borderTop = slavePtr->padTop;
	    borderBtm = borderY - borderTop;
	}
	width = Tk_ReqWidth(slavePtr->tkwin) + slavePtr->doubleBw
		+ slavePtr->iPadX;
	if ((slavePtr->flags & FILLX)
		|| (width > (frameWidth - borderX))) {
	    width = frameWidth - borderX;
	}
	height = Tk_ReqHeight(slavePtr->tkwin) + slavePtr->doubleBw
		+ slavePtr->iPadY;
	if ((slavePtr->flags & FILLY)
		|| (height > (frameHeight - borderY))) {
	    height = frameHeight - borderY;
	}
	switch (slavePtr->anchor) {
	case TK_ANCHOR_N:
	    x = frameX + (borderLeft + frameWidth - width - borderRight)/2;
	    y = frameY + borderTop;
	    break;
	case TK_ANCHOR_NE:
	    x = frameX + frameWidth - width - borderRight;
	    y = frameY + borderTop;
	    break;
	case TK_ANCHOR_E:
	    x = frameX + frameWidth - width - borderRight;
	    y = frameY + (borderTop + frameHeight - height - borderBtm)/2;
	    break;
	case TK_ANCHOR_SE:
	    x = frameX + frameWidth - width - borderRight;
	    y = frameY + frameHeight - height - borderBtm;
	    break;
	case TK_ANCHOR_S:
	    x = frameX + (borderLeft + frameWidth - width - borderRight)/2;
	    y = frameY + frameHeight - height - borderBtm;
	    break;
	case TK_ANCHOR_SW:
	    x = frameX + borderLeft;
	    y = frameY + frameHeight - height - borderBtm;
	    break;
	case TK_ANCHOR_W:
	    x = frameX + borderLeft;
	    y = frameY + (borderTop + frameHeight - height - borderBtm)/2;
	    break;
	case TK_ANCHOR_NW:
	    x = frameX + borderLeft;
	    y = frameY + borderTop;
	    break;
	case TK_ANCHOR_CENTER:
	    x = frameX + (borderLeft + frameWidth - width - borderRight)/2;
	    y = frameY + (borderTop + frameHeight - height - borderBtm)/2;
	    break;
	default:
	    Tcl_Panic("bad frame factor in ArrangePacking");
	}
	width -= slavePtr->doubleBw;
	height -= slavePtr->doubleBw;

	/*
	 * The final step is to set the position, size, and mapped/unmapped
	 * state of the slave. If the slave is a child of the master, then do
	 * this here. Otherwise let Tk_MaintainGeometry do the work.
	 */

	if (masterPtr->tkwin == Tk_Parent(slavePtr->tkwin)) {
	    if ((width <= 0) || (height <= 0)) {
		Tk_UnmapWindow(slavePtr->tkwin);
	    } else {
		if ((x != Tk_X(slavePtr->tkwin))
			|| (y != Tk_Y(slavePtr->tkwin))
			|| (width != Tk_Width(slavePtr->tkwin))
			|| (height != Tk_Height(slavePtr->tkwin))) {
		    Tk_MoveResizeWindow(slavePtr->tkwin, x, y, width, height);
		}
		if (abort) {
		    goto done;
		}

		/*
		 * Don't map the slave if the master isn't mapped: wait until
		 * the master gets mapped later.
		 */

		if (Tk_IsMapped(masterPtr->tkwin)) {
		    Tk_MapWindow(slavePtr->tkwin);
		}
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

	/*
	 * Changes to the window's structure could cause almost anything to
	 * happen, including deleting the parent or child. If this happens,
	 * we'll be told to abort.
	 */

	if (abort) {
	    goto done;
	}
    }

  done:
    masterPtr->abortPtr = NULL;
    Tcl_Release(masterPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * XExpansion --
 *
 *	Given a list of packed slaves, the first of which is packed on the
 *	left or right and is expandable, compute how much to expand the child.
 *
 * Results:
 *	The return value is the number of additional pixels to give to the
 *	child.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
XExpansion(
    register Packer *slavePtr,	/* First in list of remaining slaves. */
    int cavityWidth)		/* Horizontal space left for all remaining
				 * slaves. */
{
    int numExpand, minExpand, curExpand;
    int childWidth;

    /*
     * This function is tricky because windows packed top or bottom can be
     * interspersed among expandable windows packed left or right. Scan
     * through the list, keeping a running sum of the widths of all left and
     * right windows (actually, count the cavity space not allocated) and a
     * running count of all expandable left and right windows. At each top or
     * bottom window, and at the end of the list, compute the expansion factor
     * that seems reasonable at that point. Return the smallest factor seen at
     * any of these points.
     */

    minExpand = cavityWidth;
    numExpand = 0;
    for ( ; slavePtr != NULL; slavePtr = slavePtr->nextPtr) {
	childWidth = Tk_ReqWidth(slavePtr->tkwin) + slavePtr->doubleBw
		+ slavePtr->padX + slavePtr->iPadX;
	if ((slavePtr->side == TOP) || (slavePtr->side == BOTTOM)) {
	    if (numExpand) {
		curExpand = (cavityWidth - childWidth)/numExpand;
		if (curExpand < minExpand) {
		    minExpand = curExpand;
		}
	    }
	} else {
	    cavityWidth -= childWidth;
	    if (slavePtr->flags & EXPAND) {
		numExpand++;
	    }
	}
    }
    if (numExpand) {
	curExpand = cavityWidth/numExpand;
	if (curExpand < minExpand) {
	    minExpand = curExpand;
	}
    }
    return (minExpand < 0) ? 0 : minExpand;
}

/*
 *----------------------------------------------------------------------
 *
 * YExpansion --
 *
 *	Given a list of packed slaves, the first of which is packed on the top
 *	or bottom and is expandable, compute how much to expand the child.
 *
 * Results:
 *	The return value is the number of additional pixels to give to the
 *	child.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
YExpansion(
    register Packer *slavePtr,	/* First in list of remaining slaves. */
    int cavityHeight)		/* Vertical space left for all remaining
				 * slaves. */
{
    int numExpand, minExpand, curExpand;
    int childHeight;

    /*
     * See comments for XExpansion.
     */

    minExpand = cavityHeight;
    numExpand = 0;
    for ( ; slavePtr != NULL; slavePtr = slavePtr->nextPtr) {
	childHeight = Tk_ReqHeight(slavePtr->tkwin) + slavePtr->doubleBw
		+ slavePtr->padY + slavePtr->iPadY;
	if ((slavePtr->side == LEFT) || (slavePtr->side == RIGHT)) {
	    if (numExpand) {
		curExpand = (cavityHeight - childHeight)/numExpand;
		if (curExpand < minExpand) {
		    minExpand = curExpand;
		}
	    }
	} else {
	    cavityHeight -= childHeight;
	    if (slavePtr->flags & EXPAND) {
		numExpand++;
	    }
	}
    }
    if (numExpand) {
	curExpand = cavityHeight/numExpand;
	if (curExpand < minExpand) {
	    minExpand = curExpand;
	}
    }
    return (minExpand < 0) ? 0 : minExpand;
}

/*
 *------------------------------------------------------------------------
 *
 * GetPacker --
 *
 *	This internal function is used to locate a Packer structure for a
 *	given window, creating one if one doesn't exist already.
 *
 * Results:
 *	The return value is a pointer to the Packer structure corresponding to
 *	tkwin.
 *
 * Side effects:
 *	A new packer structure may be created. If so, then a callback is set
 *	up to clean things up when the window is deleted.
 *
 *------------------------------------------------------------------------
 */

static Packer *
GetPacker(
    Tk_Window tkwin)		/* Token for window for which packer structure
				 * is desired. */
{
    register Packer *packPtr;
    Tcl_HashEntry *hPtr;
    int isNew;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (!dispPtr->packInit) {
	dispPtr->packInit = 1;
	Tcl_InitHashTable(&dispPtr->packerHashTable, TCL_ONE_WORD_KEYS);
    }

    /*
     * See if there's already packer for this window. If not, then create a
     * new one.
     */

    hPtr = Tcl_CreateHashEntry(&dispPtr->packerHashTable, (char *) tkwin,
	    &isNew);
    if (!isNew) {
	return Tcl_GetHashValue(hPtr);
    }
    packPtr = ckalloc(sizeof(Packer));
    packPtr->tkwin = tkwin;
    packPtr->masterPtr = NULL;
    packPtr->nextPtr = NULL;
    packPtr->slavePtr = NULL;
    packPtr->side = TOP;
    packPtr->anchor = TK_ANCHOR_CENTER;
    packPtr->padX = packPtr->padY = 0;
    packPtr->padLeft = packPtr->padTop = 0;
    packPtr->iPadX = packPtr->iPadY = 0;
    packPtr->doubleBw = 2*Tk_Changes(tkwin)->border_width;
    packPtr->abortPtr = NULL;
    packPtr->flags = 0;
    Tcl_SetHashValue(hPtr, packPtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	    PackStructureProc, packPtr);
    return packPtr;
}

/*
 *------------------------------------------------------------------------
 *
 * PackAfter --
 *
 *	This function does most of the real work of adding one or more windows
 *	into the packing order for its master.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The geometry of the specified windows may change, both now and again
 *	in the future.
 *
 *------------------------------------------------------------------------
 */

static int
PackAfter(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Packer *prevPtr,		/* Pack windows in argv just after this
				 * window; NULL means pack as first child of
				 * masterPtr. */
    Packer *masterPtr,		/* Master in which to pack windows. */
    int objc,			/* Number of elements in objv. */
    Tcl_Obj *const objv[])	/* Array of lists, each containing 2 elements:
				 * window name and side against which to
				 * pack. */
{
    register Packer *packPtr;
    Tk_Window tkwin, ancestor, parent;
    Tcl_Obj **options;
    int index, optionCount, c;

    /*
     * Iterate over all of the window specifiers, each consisting of two
     * arguments. The first argument contains the window name and the
     * additional arguments contain options such as "top" or "padx 20".
     */

    for ( ; objc > 0; objc -= 2, objv += 2, prevPtr = packPtr) {
	if (objc < 2) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "wrong # args: window \"%s\" should be followed by options",
		    Tcl_GetString(objv[0])));
	    Tcl_SetErrorCode(interp, "TCL", "WRONGARGS", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Find the packer for the window to be packed, and make sure that the
	 * window in which it will be packed is either its or a descendant of
	 * its parent.
	 */

	if (TkGetWindowFromObj(interp, masterPtr->tkwin, objv[0], &tkwin)
		!= TCL_OK) {
	    return TCL_ERROR;
	}

	parent = Tk_Parent(tkwin);
	for (ancestor = masterPtr->tkwin; ; ancestor = Tk_Parent(ancestor)) {
	    if (ancestor == parent) {
		break;
	    }
	    if (((Tk_FakeWin *) (ancestor))->flags & TK_TOP_HIERARCHY) {
	    badWindow:
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"can't pack %s inside %s", Tcl_GetString(objv[0]),
			Tk_PathName(masterPtr->tkwin)));
		Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "HIERARCHY", NULL);
		return TCL_ERROR;
	    }
	}
	if (((Tk_FakeWin *) (tkwin))->flags & TK_TOP_HIERARCHY) {
	    goto badWindow;
	}
	if (tkwin == masterPtr->tkwin) {
	    goto badWindow;
	}
	packPtr = GetPacker(tkwin);

	/*
	 * Process options for this window.
	 */

	if (Tcl_ListObjGetElements(interp, objv[1], &optionCount, &options)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	packPtr->side = TOP;
	packPtr->anchor = TK_ANCHOR_CENTER;
	packPtr->padX = packPtr->padY = 0;
	packPtr->padLeft = packPtr->padTop = 0;
	packPtr->iPadX = packPtr->iPadY = 0;
	packPtr->flags &= ~(FILLX|FILLY|EXPAND);
	packPtr->flags |= OLD_STYLE;
	for (index = 0 ; index < optionCount; index++) {
	    Tcl_Obj *curOptPtr = options[index];
	    const char *curOpt = Tcl_GetString(curOptPtr);
	    size_t length = curOptPtr->length;

	    c = curOpt[0];

	    if ((c == 't')
		    && (strncmp(curOpt, "top", length)) == 0) {
		packPtr->side = TOP;
	    } else if ((c == 'b')
		    && (strncmp(curOpt, "bottom", length)) == 0) {
		packPtr->side = BOTTOM;
	    } else if ((c == 'l')
		    && (strncmp(curOpt, "left", length)) == 0) {
		packPtr->side = LEFT;
	    } else if ((c == 'r')
		    && (strncmp(curOpt, "right", length)) == 0) {
		packPtr->side = RIGHT;
	    } else if ((c == 'e')
		    && (strncmp(curOpt, "expand", length)) == 0) {
		packPtr->flags |= EXPAND;
	    } else if ((c == 'f')
		    && (strcmp(curOpt, "fill")) == 0) {
		packPtr->flags |= FILLX|FILLY;
	    } else if ((length == 5) && (strcmp(curOpt, "fillx")) == 0) {
		packPtr->flags |= FILLX;
	    } else if ((length == 5) && (strcmp(curOpt, "filly")) == 0) {
		packPtr->flags |= FILLY;
	    } else if ((c == 'p') && (strcmp(curOpt, "padx")) == 0) {
		if (optionCount < (index+2)) {
		missingPad:
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "wrong # args: \"%s\" option must be"
			    " followed by screen distance", curOpt));
		    Tcl_SetErrorCode(interp, "TK", "OLDPACK", "BAD_PARAMETER",
			    NULL);
		    return TCL_ERROR;
		}
		if (TkParsePadAmount(interp, tkwin, options[index+1],
			&packPtr->padLeft, &packPtr->padX) != TCL_OK) {
		    return TCL_ERROR;
		}
		packPtr->padX /= 2;
		packPtr->padLeft /= 2;
		packPtr->iPadX = 0;
		index++;
	    } else if ((c == 'p') && (strcmp(curOpt, "pady")) == 0) {
		if (optionCount < (index+2)) {
		    goto missingPad;
		}
		if (TkParsePadAmount(interp, tkwin, options[index+1],
			&packPtr->padTop, &packPtr->padY) != TCL_OK) {
		    return TCL_ERROR;
		}
		packPtr->padY /= 2;
		packPtr->padTop /= 2;
		packPtr->iPadY = 0;
		index++;
	    } else if ((c == 'f') && (length > 1)
		    && (strncmp(curOpt, "frame", (size_t) length) == 0)) {
		if (optionCount < (index+2)) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "wrong # args: \"frame\""
			    " option must be followed by anchor point", -1));
		    Tcl_SetErrorCode(interp, "TK", "OLDPACK", "BAD_PARAMETER",
			    NULL);
		    return TCL_ERROR;
		}
		if (Tk_GetAnchorFromObj(interp, options[index+1],
			&packPtr->anchor) != TCL_OK) {
		    return TCL_ERROR;
		}
		index++;
	    } else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"bad option \"%s\": should be top, bottom, left,"
			" right, expand, fill, fillx, filly, padx, pady, or"
			" frame", curOpt));
		Tcl_SetErrorCode(interp, "TK", "OLDPACK", "BAD_PARAMETER",
			NULL);
		return TCL_ERROR;
	    }
	}

	if (packPtr != prevPtr) {
	    /*
	     * Unpack this window if it's currently packed.
	     */

	    if (packPtr->masterPtr != NULL) {
		if ((packPtr->masterPtr != masterPtr) &&
			(packPtr->masterPtr->tkwin
			!= Tk_Parent(packPtr->tkwin))) {
		    Tk_UnmaintainGeometry(packPtr->tkwin,
			    packPtr->masterPtr->tkwin);
		}
		Unlink(packPtr);
	    }

	    /*
	     * Add the window in the correct place in its master's packing
	     * order, then make sure that the window is managed by us.
	     */

	    packPtr->masterPtr = masterPtr;
	    if (prevPtr == NULL) {
		packPtr->nextPtr = masterPtr->slavePtr;
		masterPtr->slavePtr = packPtr;
	    } else {
		packPtr->nextPtr = prevPtr->nextPtr;
		prevPtr->nextPtr = packPtr;
	    }
	    Tk_ManageGeometry(tkwin, &packerType, packPtr);

	    if (!(masterPtr->flags & DONT_PROPAGATE)) {
		if (TkSetGeometryMaster(interp, masterPtr->tkwin, "pack")
			!= TCL_OK) {
		    Tk_ManageGeometry(tkwin, NULL, NULL);
		    Unlink(packPtr);
		    return TCL_ERROR;
		}
		masterPtr->flags |= ALLOCED_MASTER;
	    }
	}
    }

    /*
     * Arrange for the master to be re-packed at the first idle moment.
     */

    if (masterPtr->abortPtr != NULL) {
	*masterPtr->abortPtr = 1;
    }
    if (!(masterPtr->flags & REQUESTED_REPACK)) {
	masterPtr->flags |= REQUESTED_REPACK;
	Tcl_DoWhenIdle(ArrangePacking, masterPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Unlink --
 *
 *	Remove a packer from its master's list of slaves.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The master will be scheduled for repacking.
 *
 *----------------------------------------------------------------------
 */

static void
Unlink(
    register Packer *packPtr)	/* Window to unlink. */
{
    register Packer *masterPtr, *packPtr2;

    masterPtr = packPtr->masterPtr;
    if (masterPtr == NULL) {
	return;
    }
    if (masterPtr->slavePtr == packPtr) {
	masterPtr->slavePtr = packPtr->nextPtr;
    } else {
	for (packPtr2 = masterPtr->slavePtr; ; packPtr2 = packPtr2->nextPtr) {
	    if (packPtr2 == NULL) {
		Tcl_Panic("Unlink couldn't find previous window");
	    }
	    if (packPtr2->nextPtr == packPtr) {
		packPtr2->nextPtr = packPtr->nextPtr;
		break;
	    }
	}
    }
    if (!(masterPtr->flags & REQUESTED_REPACK)) {
	masterPtr->flags |= REQUESTED_REPACK;
	Tcl_DoWhenIdle(ArrangePacking, masterPtr);
    }
    if (masterPtr->abortPtr != NULL) {
	*masterPtr->abortPtr = 1;
    }

    packPtr->masterPtr = NULL;

    /*
     * If we have emptied this master from slaves it means we are no longer
     * handling it and should mark it as free.
     */

    if ((masterPtr->slavePtr == NULL) && (masterPtr->flags & ALLOCED_MASTER)) {
	TkFreeGeometryMaster(masterPtr->tkwin, "pack");
	masterPtr->flags &= ~ALLOCED_MASTER;
    }

}

/*
 *----------------------------------------------------------------------
 *
 * DestroyPacker --
 *
 *	This function is invoked by Tcl_EventuallyFree or Tcl_Release to clean
 *	up the internal structure of a packer at a safe time (when no-one is
 *	using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the packer is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyPacker(
    void *memPtr)		/* Info about packed window that is now
				 * dead. */
{
    register Packer *packPtr = memPtr;

    ckfree(packPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * PackStructureProc --
 *
 *	This function is invoked by the Tk event dispatcher in response to
 *	StructureNotify events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a window was just deleted, clean up all its packer-related
 *	information. If it was just resized, repack its slaves, if any.
 *
 *----------------------------------------------------------------------
 */

static void
PackStructureProc(
    ClientData clientData,	/* Our information about window referred to by
				 * eventPtr. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    register Packer *packPtr = clientData;

    if (eventPtr->type == ConfigureNotify) {
	if ((packPtr->slavePtr != NULL)
		&& !(packPtr->flags & REQUESTED_REPACK)) {
	    packPtr->flags |= REQUESTED_REPACK;
	    Tcl_DoWhenIdle(ArrangePacking, packPtr);
	}
	if ((packPtr->masterPtr != NULL)
	        && (packPtr->doubleBw != 2*Tk_Changes(packPtr->tkwin)->border_width)) {
	    if (!(packPtr->masterPtr->flags & REQUESTED_REPACK)) {
		packPtr->doubleBw = 2*Tk_Changes(packPtr->tkwin)->border_width;
		packPtr->masterPtr->flags |= REQUESTED_REPACK;
		Tcl_DoWhenIdle(ArrangePacking, packPtr->masterPtr);
	    }
	}
    } else if (eventPtr->type == DestroyNotify) {
	register Packer *slavePtr, *nextPtr;

	if (packPtr->masterPtr != NULL) {
	    Unlink(packPtr);
	}

	for (slavePtr = packPtr->slavePtr; slavePtr != NULL;
		slavePtr = nextPtr) {
	    Tk_ManageGeometry(slavePtr->tkwin, NULL, NULL);
	    Tk_UnmapWindow(slavePtr->tkwin);
	    slavePtr->masterPtr = NULL;
	    nextPtr = slavePtr->nextPtr;
	    slavePtr->nextPtr = NULL;
	}

	if (packPtr->tkwin != NULL) {
	    TkDisplay *dispPtr = ((TkWindow *) packPtr->tkwin)->dispPtr;
            Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->packerHashTable,
		    (char *) packPtr->tkwin));
	}

	if (packPtr->flags & REQUESTED_REPACK) {
	    Tcl_CancelIdleCall(ArrangePacking, packPtr);
	}
	packPtr->tkwin = NULL;
	Tcl_EventuallyFree(packPtr, (Tcl_FreeProc *) DestroyPacker);
    } else if (eventPtr->type == MapNotify) {
	/*
	 * When a master gets mapped, must redo the geometry computation so
	 * that all of its slaves get remapped.
	 */

	if ((packPtr->slavePtr != NULL)
		&& !(packPtr->flags & REQUESTED_REPACK)) {
	    packPtr->flags |= REQUESTED_REPACK;
	    Tcl_DoWhenIdle(ArrangePacking, packPtr);
	}
    } else if (eventPtr->type == UnmapNotify) {
	register Packer *packPtr2;

	/*
	 * Unmap all of the slaves when the master gets unmapped, so that they
	 * don't bother to keep redisplaying themselves.
	 */

	for (packPtr2 = packPtr->slavePtr; packPtr2 != NULL;
	     packPtr2 = packPtr2->nextPtr) {
	    Tk_UnmapWindow(packPtr2->tkwin);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureSlaves --
 *
 *	This implements the guts of the "pack configure" command. Given a list
 *	of slaves and configuration options, it arranges for the packer to
 *	manage the slaves and sets the specified options.
 *
 * Results:
 *	TCL_OK is returned if all went well. Otherwise, TCL_ERROR is returned
 *	and the interp's result is set to contain an error message.
 *
 * Side effects:
 *	Slave windows get taken over by the packer.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureSlaves(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Any window in application containing
				 * slaves. Used to look up slave names. */
    int objc,			/* Number of elements in argv. */
    Tcl_Obj *const objv[])	/* Argument objects: contains one or more
				 * window names followed by any number of
				 * "option value" pairs. Caller must make sure
				 * that there is at least one window name. */
{
    Packer *masterPtr, *slavePtr, *prevPtr, *otherPtr;
    Tk_Window other, slave, parent, ancestor;
    TkWindow *master;
    int i, j, numWindows, tmp, positionGiven;
    const char *string;
    static const char *const optionStrings[] = {
	"-after", "-anchor", "-before", "-expand", "-fill",
	"-in", "-ipadx", "-ipady", "-padx", "-pady", "-side", NULL };
    enum options {
	CONF_AFTER, CONF_ANCHOR, CONF_BEFORE, CONF_EXPAND, CONF_FILL,
	CONF_IN, CONF_IPADX, CONF_IPADY, CONF_PADX, CONF_PADY, CONF_SIDE };
    int index, side;

    /*
     * Find out how many windows are specified.
     */

    for (numWindows = 0; numWindows < objc; numWindows++) {
	string = Tcl_GetString(objv[numWindows]);
	if (string[0] != '.') {
	    break;
	}
    }

    /*
     * Iterate over all of the slave windows, parsing the configuration
     * options for each slave. It's a bit wasteful to re-parse the options for
     * each slave, but things get too messy if we try to parse the arguments
     * just once at the beginning. For example, if a slave already is packed
     * we want to just change a few existing values without resetting
     * everything. If there are multiple windows, the -after, -before, and -in
     * options only get processed for the first window.
     */

    masterPtr = NULL;
    prevPtr = NULL;
    positionGiven = 0;
    for (j = 0; j < numWindows; j++) {
	if (TkGetWindowFromObj(interp, tkwin, objv[j], &slave) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tk_TopWinHierarchy(slave)) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't pack \"%s\": it's a top-level window",
		    Tcl_GetString(objv[j])));
	    Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "TOPLEVEL", NULL);
	    return TCL_ERROR;
	}
	slavePtr = GetPacker(slave);
	slavePtr->flags &= ~OLD_STYLE;

	/*
	 * If the slave isn't currently packed, reset all of its configuration
	 * information to default values (there could be old values left from
	 * a previous packing).
	 */

	if (slavePtr->masterPtr == NULL) {
	    slavePtr->side = TOP;
	    slavePtr->anchor = TK_ANCHOR_CENTER;
	    slavePtr->padX = slavePtr->padY = 0;
	    slavePtr->padLeft = slavePtr->padTop = 0;
	    slavePtr->iPadX = slavePtr->iPadY = 0;
	    slavePtr->flags &= ~(FILLX|FILLY|EXPAND);
	}

	for (i = numWindows; i < objc; i+=2) {
	    if ((i+2) > objc) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"extra option \"%s\" (option with no value?)",
			Tcl_GetString(objv[i])));
		Tcl_SetErrorCode(interp, "TK", "PACK", "BAD_PARAMETER", NULL);
		return TCL_ERROR;
	    }
	    if (Tcl_GetIndexFromObjStruct(interp, objv[i], optionStrings,
		    sizeof(char *), "option", 0, &index) != TCL_OK) {
		return TCL_ERROR;
	    }

	    switch ((enum options) index) {
	    case CONF_AFTER:
		if (j == 0) {
		    if (TkGetWindowFromObj(interp, tkwin, objv[i+1], &other)
			    != TCL_OK) {
			return TCL_ERROR;
		    }
		    prevPtr = GetPacker(other);
		    if (prevPtr->masterPtr == NULL) {
		    notPacked:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf(
				"window \"%s\" isn't packed",
				Tcl_GetString(objv[i+1])));
			Tcl_SetErrorCode(interp, "TK", "PACK", "NOT_PACKED",
				NULL);
			return TCL_ERROR;
		    }
		    masterPtr = prevPtr->masterPtr;
		    positionGiven = 1;
		}
		break;
	    case CONF_ANCHOR:
		if (Tk_GetAnchorFromObj(interp, objv[i+1], &slavePtr->anchor)
			!= TCL_OK) {
		    return TCL_ERROR;
		}
		break;
	    case CONF_BEFORE:
		if (j == 0) {
		    if (TkGetWindowFromObj(interp, tkwin, objv[i+1], &other)
			    != TCL_OK) {
			return TCL_ERROR;
		    }
		    otherPtr = GetPacker(other);
		    if (otherPtr->masterPtr == NULL) {
			goto notPacked;
		    }
		    masterPtr = otherPtr->masterPtr;
		    prevPtr = masterPtr->slavePtr;
		    if (prevPtr == otherPtr) {
			prevPtr = NULL;
		    } else {
			while (prevPtr->nextPtr != otherPtr) {
			    prevPtr = prevPtr->nextPtr;
			}
		    }
		    positionGiven = 1;
		}
		break;
	    case CONF_EXPAND:
		if (Tcl_GetBooleanFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    return TCL_ERROR;
		}
		slavePtr->flags &= ~EXPAND;
		if (tmp) {
		    slavePtr->flags |= EXPAND;
		}
		break;
	    case CONF_FILL:
		string = Tcl_GetString(objv[i+1]);
		if (strcmp(string, "none") == 0) {
		    slavePtr->flags &= ~(FILLX|FILLY);
		} else if (strcmp(string, "x") == 0) {
		    slavePtr->flags = (slavePtr->flags & ~FILLY) | FILLX;
		} else if (strcmp(string, "y") == 0) {
		    slavePtr->flags = (slavePtr->flags & ~FILLX) | FILLY;
		} else if (strcmp(string, "both") == 0) {
		    slavePtr->flags |= FILLX|FILLY;
		} else {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "bad fill style \"%s\": must be "
			    "none, x, y, or both", string));
		    Tcl_SetErrorCode(interp, "TK", "VALUE", "FILL", NULL);
		    return TCL_ERROR;
		}
		break;
	    case CONF_IN:
		if (j == 0) {
		    if (TkGetWindowFromObj(interp, tkwin, objv[i+1], &other)
			    != TCL_OK) {
			return TCL_ERROR;
		    }
		    masterPtr = GetPacker(other);
		    prevPtr = masterPtr->slavePtr;
		    if (prevPtr != NULL) {
			while (prevPtr->nextPtr != NULL) {
			    prevPtr = prevPtr->nextPtr;
			}
		    }
		    positionGiven = 1;
		}
		break;
	    case CONF_IPADX:
		if ((Tk_GetPixelsFromObj(interp, slave, objv[i+1], &tmp)
			!= TCL_OK) || (tmp < 0)) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "bad ipadx value \"%s\": must be positive screen"
			    " distance", Tcl_GetString(objv[i+1])));
		    Tcl_SetErrorCode(interp, "TK", "VALUE", "INT_PAD", NULL);
		    return TCL_ERROR;
		}
		slavePtr->iPadX = tmp * 2;
		break;
	    case CONF_IPADY:
		if ((Tk_GetPixelsFromObj(interp, slave, objv[i+1], &tmp)
			!= TCL_OK) || (tmp < 0)) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "bad ipady value \"%s\": must be positive screen"
			    " distance", Tcl_GetString(objv[i+1])));
		    Tcl_SetErrorCode(interp, "TK", "VALUE", "INT_PAD", NULL);
		    return TCL_ERROR;
		}
		slavePtr->iPadY = tmp * 2;
		break;
	    case CONF_PADX:
		if (TkParsePadAmount(interp, slave, objv[i+1],
			&slavePtr->padLeft, &slavePtr->padX) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;
	    case CONF_PADY:
		if (TkParsePadAmount(interp, slave, objv[i+1],
			&slavePtr->padTop, &slavePtr->padY) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;
	    case CONF_SIDE:
		if (Tcl_GetIndexFromObjStruct(interp, objv[i+1], sideNames,
			sizeof(char *), "side", TCL_EXACT, &side) != TCL_OK) {
		    return TCL_ERROR;
		}
		slavePtr->side = (Side) side;
		break;
	    }
	}

	/*
	 * If no position in a packing list was specified and the slave is
	 * already packed, then leave it in its current location in its
	 * current packing list.
	 */

	if (!positionGiven && (slavePtr->masterPtr != NULL)) {
	    masterPtr = slavePtr->masterPtr;
	    goto scheduleLayout;
	}

	/*
	 * If the slave is going to be put back after itself or the same -in
	 * window is passed in again, then just skip the whole operation,
	 * since it won't work anyway.
	 */

	if (prevPtr == slavePtr) {
	    masterPtr = slavePtr->masterPtr;
	    goto scheduleLayout;
	}

	/*
	 * If none of the "-in", "-before", or "-after" options has been
	 * specified, arrange for the slave to go at the end of the order for
	 * its parent.
	 */

	if (!positionGiven) {
	    masterPtr = GetPacker(Tk_Parent(slave));
	    prevPtr = masterPtr->slavePtr;
	    if (prevPtr != NULL) {
		while (prevPtr->nextPtr != NULL) {
		    prevPtr = prevPtr->nextPtr;
		}
	    }
	}

	/*
	 * Make sure that the slave's parent is either the master or an
	 * ancestor of the master, and that the master and slave aren't the
	 * same.
	 */

	parent = Tk_Parent(slave);
	for (ancestor = masterPtr->tkwin; ; ancestor = Tk_Parent(ancestor)) {
	    if (ancestor == parent) {
		break;
	    }
	    if (Tk_TopWinHierarchy(ancestor)) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"can't pack %s inside %s", Tcl_GetString(objv[j]),
			Tk_PathName(masterPtr->tkwin)));
		Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "HIERARCHY", NULL);
		return TCL_ERROR;
	    }
	}
	if (slave == masterPtr->tkwin) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't pack %s inside itself", Tcl_GetString(objv[j])));
	    Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "SELF", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Check for management loops.
	 */

	for (master = (TkWindow *)masterPtr->tkwin; master != NULL;
	     master = (TkWindow *)TkGetGeomMaster(master)) {
	    if (master == (TkWindow *)slave) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't put %s inside %s, would cause management loop",
	            Tcl_GetString(objv[j]), Tk_PathName(masterPtr->tkwin)));
		Tcl_SetErrorCode(interp, "TK", "GEOMETRY", "LOOP", NULL);
		return TCL_ERROR;
	    }
	}
	if (masterPtr->tkwin != Tk_Parent(slave)) {
	    ((TkWindow *)slave)->maintainerPtr = (TkWindow *)masterPtr->tkwin;
	}

	/*
	 * Unpack the slave if it's currently packed, then position it after
	 * prevPtr.
	 */

	if (slavePtr->masterPtr != NULL) {
	    if ((slavePtr->masterPtr != masterPtr) &&
		    (slavePtr->masterPtr->tkwin
		    != Tk_Parent(slavePtr->tkwin))) {
		Tk_UnmaintainGeometry(slavePtr->tkwin,
			slavePtr->masterPtr->tkwin);
	    }
	    Unlink(slavePtr);
	}

	slavePtr->masterPtr = masterPtr;
	if (prevPtr == NULL) {
	    slavePtr->nextPtr = masterPtr->slavePtr;
	    masterPtr->slavePtr = slavePtr;
	} else {
	    slavePtr->nextPtr = prevPtr->nextPtr;
	    prevPtr->nextPtr = slavePtr;
	}
	Tk_ManageGeometry(slave, &packerType, slavePtr);
	prevPtr = slavePtr;

	if (!(masterPtr->flags & DONT_PROPAGATE)) {
	    if (TkSetGeometryMaster(interp, masterPtr->tkwin, "pack")
		    != TCL_OK) {
		Tk_ManageGeometry(slave, NULL, NULL);
		Unlink(slavePtr);
		return TCL_ERROR;
	    }
	    masterPtr->flags |= ALLOCED_MASTER;
	}

	/*
	 * Arrange for the master to be re-packed at the first idle moment.
	 */

    scheduleLayout:
	if (masterPtr->abortPtr != NULL) {
	    *masterPtr->abortPtr = 1;
	}
	if (!(masterPtr->flags & REQUESTED_REPACK)) {
	    masterPtr->flags |= REQUESTED_REPACK;
	    Tcl_DoWhenIdle(ArrangePacking, masterPtr);
	}
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
