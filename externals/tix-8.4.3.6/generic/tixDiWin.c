/*
 * tixDiWin.c --
 *
 *	This file implements one of the "Display Items" in the Tix library :
 *	WindowItem display items.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixDiWin.c,v 1.3 2004/03/28 02:44:56 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>

/*----------------------------------------------------------------------
 *
 * 		 Data structures used by this file
 *
 *----------------------------------------------------------------------
 */

/*----------------------------------------------------------------------
 *
 * 		      Private data definition
 *
 *----------------------------------------------------------------------
 */

static Tix_ListInfo mapWinListInfo = {
    Tk_Offset(TixWindowItem, next),
    TIX_UNDEFINED,
};

#define DEF_WINDOWITEM_WINDOW	 ""
#define DEF_WINDOWITEM_STYLE	 ""
#define DEF_WINDOWITEM_TYPE	 "window"

static Tk_ConfigSpec windowItemConfigSpecs[] = {
    {TK_CONFIG_CUSTOM, "-itemtype", "itemType", "ItemType", 
       DEF_WINDOWITEM_TYPE, Tk_Offset(TixWindowItem, diTypePtr),
       0, &tixConfigItemType},

    {TK_CONFIG_CUSTOM, "-style", "windowStyle", "WindowStyle",
       DEF_WINDOWITEM_STYLE, Tk_Offset(TixWindowItem, stylePtr),
       TK_CONFIG_NULL_OK, &tixConfigItemStyle},

    {TK_CONFIG_SYNONYM, "-widget", (char *) NULL, (char *) NULL,
       (char *)NULL, 0, 0},

    {TK_CONFIG_WINDOW, "-window", "window", "Window",
       DEF_WINDOWITEM_WINDOW, Tk_Offset(TixWindowItem, tkwin), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};
/*----------------------------------------------------------------------
 *
 * 		Configuration options for Window Styles
 *
 *----------------------------------------------------------------------
 */
#define DEF_WINDOWSTYLE_PADX	"0"
#define DEF_WINDOWSTYLE_PADY	"0"
#define DEF_WINDOWSTYLE_ANCHOR	"w"

#define SELECTED_BG SELECT_BG 
#define DISABLED_BG DISABLED  

#define DEF_WINDOWSTYLE_NORMAL_FG_COLOR		BLACK
#define DEF_WINDOWSTYLE_NORMAL_FG_MONO		BLACK
#define DEF_WINDOWSTYLE_NORMAL_BG_COLOR		NORMAL_BG
#define DEF_WINDOWSTYLE_NORMAL_BG_MONO		WHITE

#define DEF_WINDOWSTYLE_ACTIVE_FG_COLOR		BLACK
#define DEF_WINDOWSTYLE_ACTIVE_FG_MONO		WHITE
#define DEF_WINDOWSTYLE_ACTIVE_BG_COLOR		ACTIVE_BG
#define DEF_WINDOWSTYLE_ACTIVE_BG_MONO		BLACK

#define DEF_WINDOWSTYLE_SELECTED_FG_COLOR	BLACK
#define DEF_WINDOWSTYLE_SELECTED_FG_MONO		WHITE
#define DEF_WINDOWSTYLE_SELECTED_BG_COLOR	SELECTED_BG
#define DEF_WINDOWSTYLE_SELECTED_BG_MONO		BLACK

#define DEF_WINDOWSTYLE_DISABLED_FG_COLOR	BLACK
#define DEF_WINDOWSTYLE_DISABLED_FG_MONO		BLACK
#define DEF_WINDOWSTYLE_DISABLED_BG_COLOR	DISABLED_BG
#define DEF_WINDOWSTYLE_DISABLED_BG_MONO		WHITE

static Tk_ConfigSpec windowStyleConfigSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
       DEF_WINDOWSTYLE_ANCHOR, Tk_Offset(TixWindowStyle, anchor), 0},

    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
       (char *) NULL, 0, 0},
 
    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
       DEF_WINDOWSTYLE_PADX, Tk_Offset(TixWindowStyle, pad[0]), 0},

    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
       DEF_WINDOWSTYLE_PADY, Tk_Offset(TixWindowStyle, pad[1]), 0},

/* The following was automatically generated */
	{TK_CONFIG_COLOR,"-background","background","Background",
	DEF_WINDOWSTYLE_NORMAL_BG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_NORMAL].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-background","background","Background",
	DEF_WINDOWSTYLE_NORMAL_BG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_NORMAL].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-foreground","foreground","Foreground",
	DEF_WINDOWSTYLE_NORMAL_FG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_NORMAL].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-foreground","foreground","Foreground",
	DEF_WINDOWSTYLE_NORMAL_FG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_NORMAL].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-activebackground","activeBackground","ActiveBackground",
	DEF_WINDOWSTYLE_ACTIVE_BG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_ACTIVE].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-activebackground","activeBackground","ActiveBackground",
	DEF_WINDOWSTYLE_ACTIVE_BG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_ACTIVE].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-activeforeground","activeForeground","ActiveForeground",
	DEF_WINDOWSTYLE_ACTIVE_FG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_ACTIVE].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-activeforeground","activeForeground","ActiveForeground",
	DEF_WINDOWSTYLE_ACTIVE_FG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_ACTIVE].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-selectbackground","selectBackground","SelectBackground",
	DEF_WINDOWSTYLE_SELECTED_BG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_SELECTED].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-selectbackground","selectBackground","SelectBackground",
	DEF_WINDOWSTYLE_SELECTED_BG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_SELECTED].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-selectforeground","selectForeground","SelectForeground",
	DEF_WINDOWSTYLE_SELECTED_FG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_SELECTED].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-selectforeground","selectForeground","SelectForeground",
	DEF_WINDOWSTYLE_SELECTED_FG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_SELECTED].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-disabledbackground","disabledBackground","DisabledBackground",
	DEF_WINDOWSTYLE_DISABLED_BG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_DISABLED].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-disabledbackground","disabledBackground","DisabledBackground",
	DEF_WINDOWSTYLE_DISABLED_BG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_DISABLED].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-disabledforeground","disabledForeground","DisabledForeground",
	DEF_WINDOWSTYLE_DISABLED_FG_COLOR,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_DISABLED].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-disabledforeground","disabledForeground","DisabledForeground",
	DEF_WINDOWSTYLE_DISABLED_FG_MONO,
	Tk_Offset(TixImageStyle,colors[TIX_DITEM_DISABLED].fg),
	TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

/*----------------------------------------------------------------------
 * Forward declarations for procedures defined later in this file:
 *----------------------------------------------------------------------
 */
static void		SubWindowLostSlaveProc _ANSI_ARGS_((
			    ClientData clientData, Tk_Window tkwin));
static void		SubWindowRequestProc _ANSI_ARGS_((
			    ClientData clientData, Tk_Window tkwin));
static void		SubWindowStructureProc _ANSI_ARGS_((
			    ClientData clientData, XEvent *eventPtr));
static char *		Tix_WindowItemComponent  _ANSI_ARGS_((
			    Tix_DItem * iPtr, int x, int y));
static void		Tix_WindowItemCalculateSize  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static int 		Tix_WindowItemConfigure _ANSI_ARGS_((
			    Tix_DItem * iPtr, int argc, CONST84 char ** argv,
			    int flags));
static Tix_DItem * 	Tix_WindowItemCreate _ANSI_ARGS_((
			    Tix_DispData * ddPtr, Tix_DItemInfo * diTypePtr));
static void		Tix_WindowItemDisplay  _ANSI_ARGS_((
			    Drawable drawable, Tix_DItem * iPtr,
			    int x, int y, int width, int height,
                            int xOffset, int yOffset, int flags));
static void		Tix_WindowItemFree  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static void		Tix_WindowItemLostStyle  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static void 		Tix_WindowItemStyleChanged  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static void		Tix_WindowItemUnmap _ANSI_ARGS_((
			    TixWindowItem *itPtr));
static int 		Tix_WindowStyleConfigure _ANSI_ARGS_((
			    Tix_DItemStyle* style, int argc, CONST84 char ** argv,
			    int flags));
static Tix_DItemStyle *	Tix_WindowStyleCreate _ANSI_ARGS_((
			    Tcl_Interp *interp, Tk_Window tkwin,
			    Tix_DItemInfo * diTypePtr, char * name));
static void		Tix_WindowStyleFree _ANSI_ARGS_((
			    Tix_DItemStyle* style));
static void 		Tix_WindowStyleSetTemplate _ANSI_ARGS_((
			    Tix_DItemStyle* style,
			    Tix_StyleTemplate * tmplPtr));
static void		UnmanageWindow _ANSI_ARGS_((Tix_DItem * iPtr,
			    Tk_Window tkwin));
static void		ManageWindow _ANSI_ARGS_((Tix_DItem * iPtr,
			    Tk_Window tkwin));

Tix_DItemInfo tix_WindowItemType = {
    "window",			/* type */
    TIX_DITEM_WINDOW,
    Tix_WindowItemCreate,		/* createProc */
    Tix_WindowItemConfigure,
    Tix_WindowItemCalculateSize,
    Tix_WindowItemComponent,
    Tix_WindowItemDisplay,
    Tix_WindowItemFree,
    Tix_WindowItemStyleChanged,
    Tix_WindowItemLostStyle,

    Tix_WindowStyleCreate,
    Tix_WindowStyleConfigure,
    Tix_WindowStyleFree,
    Tix_WindowStyleSetTemplate,

    windowItemConfigSpecs,
    windowStyleConfigSpecs,

    NULL,				/*next */
};

/*
 * The structure below defines the official type record for the
 * placer:
 */
static Tk_GeomMgr windowItemGeomType = {
    "tixWindowItem",			/* name */
    SubWindowRequestProc,		/* requestProc */
    SubWindowLostSlaveProc,		/* lostSlaveProc */
};

/*----------------------------------------------------------------------
 * Tix_WindowItemCreate --
 *
 *
 *----------------------------------------------------------------------
 */
static Tix_DItem * Tix_WindowItemCreate(ddPtr, diTypePtr)
    Tix_DispData * ddPtr;
    Tix_DItemInfo * diTypePtr;
{
    TixWindowItem * itPtr;

    itPtr = (TixWindowItem*) ckalloc(sizeof(TixWindowItem));

    itPtr->diTypePtr	= diTypePtr;
    itPtr->ddPtr 	= ddPtr;
    itPtr->stylePtr     = NULL;
    itPtr->clientData	= 0;
    itPtr->size[0]	= 0;
    itPtr->size[1]	= 0;

    itPtr->tkwin	= NULL;

    return (Tix_DItem *)itPtr;
}

/* %% */
static void Tix_WindowItemFree(iPtr)
    Tix_DItem * iPtr;
{
    TixWindowItem * itPtr = (TixWindowItem *) iPtr;

    if (itPtr->tkwin) {
	Tk_DeleteEventHandler(itPtr->tkwin, StructureNotifyMask,
	    SubWindowStructureProc, (ClientData) itPtr);
	Tk_ManageGeometry(itPtr->tkwin, (Tk_GeomMgr *)NULL,
	    (ClientData) NULL);
	Tk_UnmapWindow(itPtr->tkwin);
    }
    if (itPtr->stylePtr) {
	TixDItemStyleFree(iPtr, (Tix_DItemStyle*)itPtr->stylePtr);
    }

    Tk_FreeOptions(windowItemConfigSpecs, (char *)itPtr,
	itPtr->ddPtr->display, 0);

    ckfree((char*)itPtr);
}

/*----------------------------------------------------------------------
 * ManageWindow --
 *
 *	Associate this sub-window with the window item.
 *
 * Side effect:
 *	itPtr->tkwin is changed.
 *----------------------------------------------------------------------
 */

static void
ManageWindow(iPtr, tkwin)
    Tix_DItem * iPtr;
    Tk_Window tkwin;
{
    TixWindowItem * itPtr = (TixWindowItem *) iPtr;

    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	SubWindowStructureProc, (ClientData) itPtr);
    Tk_ManageGeometry(tkwin, &windowItemGeomType,
	(ClientData) itPtr);

    itPtr->tkwin = tkwin;
}

/*----------------------------------------------------------------------
 * UnmanageWindow --
 *
 *	Disassociate this sub-window from the window item.
 *
 * Note:
 *	the tkwin parameter may not equal to itPtr->tkwin.
 *----------------------------------------------------------------------
 */

static void
UnmanageWindow(iPtr, tkwin)
    Tix_DItem * iPtr;
    Tk_Window tkwin;
{
    TixWindowItem * itPtr = (TixWindowItem *) iPtr;

    Tk_DeleteEventHandler(tkwin, StructureNotifyMask,
	SubWindowStructureProc, (ClientData) itPtr);
    Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL,
	(ClientData) NULL);
    Tk_UnmapWindow(tkwin);
}

static int Tix_WindowItemConfigure(iPtr, argc, argv, flags)
    Tix_DItem * iPtr;
    int argc;
    CONST84 char ** argv;
    int flags;
{
    TixWindowItem * itPtr = (TixWindowItem *) iPtr;
    TixWindowStyle * oldStyle = itPtr->stylePtr;
    Tk_Window oldWindow;

    oldWindow = itPtr->tkwin;

    if (Tk_ConfigureWidget(itPtr->ddPtr->interp, itPtr->ddPtr->tkwin,
	windowItemConfigSpecs,
	argc, argv, (char *)itPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (itPtr->stylePtr == NULL) {
	itPtr->stylePtr = (TixWindowStyle*)TixGetDefaultDItemStyle(
	    itPtr->ddPtr, &tix_WindowItemType, iPtr, NULL);
    }

    if (oldWindow != itPtr->tkwin) {
	if (oldWindow != NULL) {
	    UnmanageWindow(iPtr, oldWindow);
	}
	if (itPtr->tkwin != NULL) {
	    /*
	     * Make sure that the master window is the parent of the
	     * window associated with the item.
	     * Also, don't allow a top-level window to be
	     * managed inside a master window.
	     */
	    if (Tk_Parent(itPtr->tkwin) != itPtr->ddPtr->tkwin) {
		Tcl_AppendResult(itPtr->ddPtr->interp, "can't use ",
		    Tk_PathName(itPtr->tkwin),
		    " in a window item of the master widget: must be a child",
		    " of ", Tk_PathName(itPtr->ddPtr->tkwin), (char *) NULL);
		goto badWindow;
	    }
	    if (((Tk_FakeWin *) (itPtr->tkwin))->flags & TK_TOP_LEVEL) {
		Tcl_AppendResult(itPtr->ddPtr->interp, 
		    "can't manage toplevel window",
	 	    Tk_PathName(itPtr->tkwin),
		    " as a window item of ", Tk_PathName(itPtr->ddPtr->tkwin),
		    (char *) NULL);
		goto badWindow;
	    }
	    ManageWindow(iPtr, itPtr->tkwin);
	}
    }

    if (oldStyle != NULL && itPtr->stylePtr != oldStyle) {
	Tix_WindowItemStyleChanged(iPtr);
    }
    else {
	Tix_WindowItemCalculateSize((Tix_DItem*)itPtr);
    }

    return TCL_OK;

badWindow:

    itPtr->tkwin = NULL;
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_WindowItemDisplay --
 *
 *      Display an window item. {x, y, width, height} specifies a
 *      region for to display this item in. {xOffset, yOffset} gives
 *      the offset of the top-left corner of the window item relative
 *      to the top-left corder of the region.
 *
 *      Background and foreground of the item are displayed according
 *      to the flags parameter.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The geometry management calls such as Tk_MoveResizeWindow()
 *      may cause repaints in the parent window that owns this item,
 *      but it shouldn't enter an infinite loop.
 *      Tk_MoveResizeWindow (or the underlying windowing system?) will
 *      supress repaint events if the window's geometry is not changed
 *      by the Tk_MoveResizeWindow call.
 * 
 *----------------------------------------------------------------------
 */

static void
Tix_WindowItemDisplay(drawable, iPtr, x, y, width, height, xOffset, yOffset, 
        flags)
    Drawable drawable;
    Tix_DItem * iPtr;
    int x;
    int y;
    int width;
    int height;
    int flags;
{
    TixWindowItem *itPtr = (TixWindowItem *)iPtr;
    Display * display = itPtr->ddPtr->display;
    TixpSubRegion subReg;
    GC foreGC;
    int winX, winY, winH, winW;

    if (itPtr->tkwin == NULL) {
	return;
    }

    TixGetColorDItemGC(iPtr, NULL, &foreGC, NULL, flags);

    TixpStartSubRegionDraw(display, drawable, foreGC,
	    &subReg, 0, 0, x, y, width, height,
	    itPtr->size[0] + xOffset, itPtr->size[1] + yOffset);

    Tix_DItemDrawBackground(drawable, &subReg, iPtr, x, y, width, height,
           xOffset, yOffset, flags);

    /*
     * Calculate the location of the window according to anchor settings.
     */

    TixDItemGetAnchor(itPtr->stylePtr->anchor, x, y, width, height,
	    itPtr->size[0], itPtr->size[1], &x, &y);

    winX = xOffset + itPtr->stylePtr->pad[0];
    winY = yOffset + itPtr->stylePtr->pad[1];
    winW = itPtr->size[0] - 2*itPtr->stylePtr->pad[0];
    winH = itPtr->size[1] - 2*itPtr->stylePtr->pad[1];

    if (winW + winX > width) {
        winW = width - winX;
    }
    if (winH + winY > height) {
        winH = height - winY;
    }

    winX += x;
    winY += y;

    if (width < 1 || height < 1 || winW < 1 || winH < 1) {
	if (itPtr->ddPtr->tkwin != Tk_Parent(itPtr->tkwin)) {
	    Tk_UnmaintainGeometry(itPtr->tkwin, itPtr->ddPtr->tkwin);
	}

	Tk_UnmapWindow(itPtr->tkwin);
    } else {
        if (itPtr->ddPtr->tkwin == Tk_Parent(itPtr->tkwin)) {
            Tk_MapWindow(itPtr->tkwin);
            Tk_MoveResizeWindow(itPtr->tkwin, winX, winY, winW, winH);
        } else {
            Tk_MaintainGeometry(itPtr->tkwin, itPtr->ddPtr->tkwin,
	            winX, winY, winW, winH);
        }
    }

    TixpEndSubRegionDraw(display, drawable, foreGC,
	    &subReg);
}

static void
Tix_WindowItemCalculateSize(iPtr)
    Tix_DItem * iPtr;
{
    TixWindowItem *itPtr = (TixWindowItem*)iPtr;

    if (itPtr->tkwin != NULL) {
	itPtr->size[0] = Tk_ReqWidth (itPtr->tkwin);
	itPtr->size[1] = Tk_ReqHeight(itPtr->tkwin);
    } else {
	itPtr->size[0] = 0;
	itPtr->size[1] = 0;
    }

    itPtr->size[0] += 2*itPtr->stylePtr->pad[0];
    itPtr->size[1] += 2*itPtr->stylePtr->pad[1];

    itPtr->selX = 0;
    itPtr->selY = 0;
    itPtr->selW = itPtr->size[0];
    itPtr->selH = itPtr->size[1];
}

static char * Tix_WindowItemComponent(iPtr, x, y)
    Tix_DItem * iPtr;
    int x;
    int y;
{
#if 0
    TixWindowItem *itPtr = (TixWindowItem *)iPtr;
#endif
    static char * body = "body";

    return body;
}


static void Tix_WindowItemStyleChanged(iPtr)
    Tix_DItem * iPtr;
{
    TixWindowItem *itPtr = (TixWindowItem *)iPtr;

    if (itPtr->stylePtr == NULL) {
	/* Maybe we haven't set the style to default style yet */
	return;
    }
    Tix_WindowItemCalculateSize(iPtr);
    if (itPtr->ddPtr->sizeChangedProc != NULL) {
	itPtr->ddPtr->sizeChangedProc(iPtr);
    }
}
static void Tix_WindowItemLostStyle(iPtr)
    Tix_DItem * iPtr;
{
    TixWindowItem *itPtr = (TixWindowItem *)iPtr;

    itPtr->stylePtr = (TixWindowStyle*)TixGetDefaultDItemStyle(
	itPtr->ddPtr, &tix_WindowItemType, iPtr, NULL);

    Tix_WindowItemStyleChanged(iPtr);
}

/*
 *--------------------------------------------------------------
 *
 * SubWindowStructureProc --
 *
 *	This procedure is invoked whenever StructureNotify events
 *	occur for a window that's managed as part of a display
 *	item.  This procudure's only purpose is to clean up when
 *	windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is disassociated from the window item when it is
 *	deleted.
 *
 *--------------------------------------------------------------
 */
static void
SubWindowStructureProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to record describing window item. */
    XEvent *eventPtr;		/* Describes what just happened. */
{
    TixWindowItem * itPtr = (TixWindowItem *)clientData;
    int oldWidth, oldHeight;

    if (eventPtr->type == DestroyNotify) {
	itPtr->tkwin = NULL;
    }
    oldWidth  = itPtr->size[0];
    oldHeight = itPtr->size[1];
    Tix_WindowItemCalculateSize((Tix_DItem*)itPtr);

    if (oldWidth != itPtr->size[0] || oldHeight != itPtr->size[1]) {
	if (itPtr->ddPtr->sizeChangedProc != NULL) {
	    itPtr->ddPtr->sizeChangedProc((Tix_DItem*)itPtr);
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * SubWindowRequestProc --
 *
 *	This procedure is invoked whenever a window that's associated

 *	with a display item changes its requested dimensions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size and location on the screen of the window may change,
 *	depending on the options specified for the window item.
 *
 *--------------------------------------------------------------
 */
static void
SubWindowRequestProc(clientData, tkwin)
    ClientData clientData;		/* Pointer to record for window item.*/
    Tk_Window tkwin;			/* Window that changed its desired
					 * size. */
{
    TixWindowItem *itPtr = (TixWindowItem *) clientData;

    Tix_WindowItemCalculateSize((Tix_DItem*)itPtr);
    if (itPtr->ddPtr->sizeChangedProc != NULL) {
	itPtr->ddPtr->sizeChangedProc((Tix_DItem*)itPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * SubWindowLostSlaveProc --
 *
 *	This procedure is invoked by Tk whenever some other geometry
 *	claims control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all information about the slave.
 *
 *--------------------------------------------------------------
 */
	/* ARGSUSED */
static void
SubWindowLostSlaveProc(clientData, tkwin)
    ClientData clientData;	/* TixWindowItem structure for slave
				 * window that was stolen away. */
    Tk_Window tkwin;		/* Tk's handle for the slave window. */
{
    TixWindowItem *itPtr = (TixWindowItem *) clientData;

    if (itPtr->tkwin == NULL) {
	return;
    } else {
	itPtr->tkwin = NULL;
    }

    Tk_DeleteEventHandler(tkwin, StructureNotifyMask,
	SubWindowStructureProc, (ClientData) itPtr);
    Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL,
	(ClientData) NULL);
    if (itPtr->ddPtr->tkwin != Tk_Parent(tkwin)) {
	Tk_UnmaintainGeometry(tkwin, itPtr->ddPtr->tkwin);
    }
    Tk_UnmapWindow(tkwin);

    /*
     * Inform the owner that the size has changed
     */
    Tix_WindowItemCalculateSize((Tix_DItem*)itPtr);
    if (itPtr->ddPtr->sizeChangedProc != NULL) {
	itPtr->ddPtr->sizeChangedProc((Tix_DItem*)itPtr);
    }
}
/*----------------------------------------------------------------------
 *  Tix_WindowItemUnmap --
 *
 *
 *----------------------------------------------------------------------
 */
static void
Tix_WindowItemUnmap(itPtr)
    TixWindowItem *itPtr;
{
    Tk_Window tkwin = itPtr->tkwin;

    if (tkwin == NULL) {
	return;
    }

    if (itPtr->ddPtr->tkwin != Tk_Parent(tkwin)) {
	Tk_UnmaintainGeometry(tkwin, itPtr->ddPtr->tkwin);
    }
    Tk_UnmapWindow(tkwin);
}

/*----------------------------------------------------------------------
 *
 *
 *  			Display styles
 *
 *
 *----------------------------------------------------------------------
 */

static Tix_DItemStyle *
Tix_WindowStyleCreate(interp, tkwin, diTypePtr, name)
    Tcl_Interp * interp;
    Tk_Window tkwin;
    char * name;
    Tix_DItemInfo * diTypePtr;
{
    TixWindowStyle * stylePtr =
      (TixWindowStyle *)ckalloc(sizeof(TixWindowStyle));

    return (Tix_DItemStyle *)stylePtr;
}

static int
Tix_WindowStyleConfigure(style, argc, argv, flags)
    Tix_DItemStyle *style;
    int argc;
    CONST84 char ** argv;
    int flags;
{
    TixWindowStyle * stylePtr = (TixWindowStyle *)style;
    int oldPadX;
    int oldPadY;

    oldPadX = stylePtr->pad[0];
    oldPadY = stylePtr->pad[1];

    if (!(flags &TIX_DONT_CALL_CONFIG)) {
	if (Tk_ConfigureWidget(stylePtr->interp, stylePtr->tkwin,
	    windowStyleConfigSpecs,
	    argc, argv, (char *)stylePtr, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    TixDItemStyleConfigureGCs(style);

    if (oldPadX != stylePtr->pad[0] ||  oldPadY != stylePtr->pad[1]) {
	TixDItemStyleChanged(stylePtr->diTypePtr, (Tix_DItemStyle *)stylePtr);
    }

    return TCL_OK;
}

static void Tix_WindowStyleFree(style)
    Tix_DItemStyle *style;
{
    TixWindowStyle * stylePtr = (TixWindowStyle *)style;

    Tk_FreeOptions(windowStyleConfigSpecs, (char *)stylePtr,
	Tk_Display(stylePtr->tkwin), 0);
    ckfree((char *)stylePtr);
}

static int bg_flags [4] = {
    TIX_DITEM_NORMAL_BG,
    TIX_DITEM_ACTIVE_BG,
    TIX_DITEM_SELECTED_BG,
    TIX_DITEM_DISABLED_BG
};
static int fg_flags [4] = {
    TIX_DITEM_NORMAL_FG,
    TIX_DITEM_ACTIVE_FG,
    TIX_DITEM_SELECTED_FG,
    TIX_DITEM_DISABLED_FG
};

static void
Tix_WindowStyleSetTemplate(style, tmplPtr)
    Tix_DItemStyle* style;
    Tix_StyleTemplate * tmplPtr;
{
    TixWindowStyle * stylePtr = (TixWindowStyle *)style;
    int i;

    if (tmplPtr->flags & TIX_DITEM_PADX) {
	stylePtr->pad[0] = tmplPtr->pad[0];
    }
    if (tmplPtr->flags & TIX_DITEM_PADY) {
	stylePtr->pad[1] = tmplPtr->pad[1];
    }

    for (i=0; i<4; i++) {
	if (tmplPtr->flags & bg_flags[i]) {
	    if (stylePtr->colors[i].bg != NULL) {
		Tk_FreeColor(stylePtr->colors[i].bg);
	    }
	    stylePtr->colors[i].bg = Tk_GetColor(
		stylePtr->interp, stylePtr->tkwin,
		Tk_NameOfColor(tmplPtr->colors[i].bg));
	}
    }
    for (i=0; i<4; i++) {
	if (tmplPtr->flags & fg_flags[i]) {
	    if (stylePtr->colors[i].fg != NULL) {
		Tk_FreeColor(stylePtr->colors[i].fg);
	    }
	    stylePtr->colors[i].fg = Tk_GetColor(
		stylePtr->interp, stylePtr->tkwin,
		Tk_NameOfColor(tmplPtr->colors[i].fg));
	}
    }

    Tix_WindowStyleConfigure(style, 0, 0, TIX_DONT_CALL_CONFIG);
}

/*----------------------------------------------------------------------
 *
 *
 *                  Mapped Window List Handling
 *
 *
 * Maintaining a list of mapped window items. Every host widgets should
 * call these functions so that unwanted window items will not appear
 * on the screen.
 *
 *
 *----------------------------------------------------------------------
 */

void Tix_SetWindowItemSerial(lPtr, iPtr, serial)
    Tix_LinkList * lPtr;
    Tix_DItem * iPtr;
    int serial;
{
    TixWindowItem * itPtr = (TixWindowItem *)iPtr;
    TixWindowItem * curr;
    Tix_ListIterator li;
    Tix_LinkListIteratorInit(&li);

    itPtr->serial = serial;

    for (Tix_LinkListStart(&mapWinListInfo, lPtr, &li);
	 !Tix_LinkListDone(&li);
	 Tix_LinkListNext (&mapWinListInfo, lPtr, &li)) {

	curr = (TixWindowItem*)li.curr;

	if (curr == itPtr) {
	    /* Don't want any duplication */
	    return;
	}
    }
    Tix_LinkListAppend(&mapWinListInfo, lPtr, (char*)itPtr, 0);
}

/*
 *----------------------------------------------------------------------
 * UnmapWindows --
 *
 *	We need to unmap all those windows that were displayed last time
 *	but should be now invisible.
 *	Otherwise we will have some unwanted child windows floating 
 *	around.
 *----------------------------------------------------------------------
 */
void Tix_UnmapInvisibleWindowItems(lPtr, serial)
    Tix_LinkList * lPtr;
    int serial;
{
    TixWindowItem * curr;
    Tix_ListIterator li;
    Tix_LinkListIteratorInit(&li);

    for (Tix_LinkListStart(&mapWinListInfo, lPtr, &li);
	 !Tix_LinkListDone(&li);
	 Tix_LinkListNext (&mapWinListInfo, lPtr, &li)) {

	curr = (TixWindowItem*)li.curr;
	if (curr->serial != serial) {
	    Tix_WindowItemUnmap(curr);
	    Tix_LinkListDelete(&mapWinListInfo, lPtr, &li);
	}
    }
}

void
Tix_WindowItemListRemove(lPtr, iPtr)
    Tix_LinkList * lPtr;
    Tix_DItem * iPtr;
{
    TixWindowItem * curr;
    Tix_ListIterator li;
    Tix_LinkListIteratorInit(&li);

    for (Tix_LinkListStart(&mapWinListInfo, lPtr, &li);
	 !Tix_LinkListDone(&li);
	 Tix_LinkListNext (&mapWinListInfo, lPtr, &li)) {

	curr = (TixWindowItem*)li.curr;
	if (curr == (TixWindowItem*)iPtr) {
	    Tix_WindowItemUnmap(curr);
	    Tix_LinkListDelete(&mapWinListInfo, lPtr, &li);
	    return;
	}
    }
}
