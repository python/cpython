/*
 * tkMenuDraw.c --
 *
 *	This module implements the platform-independent drawing and geometry
 *	calculations of menu widgets.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkMenu.h"

/*
 * Forward declarations for functions defined later in this file:
 */

static void		AdjustMenuCoords(TkMenu *menuPtr, TkMenuEntry *mePtr,
			    int *xPtr, int *yPtr);
static void		ComputeMenuGeometry(ClientData clientData);
static void		DisplayMenu(ClientData clientData);

/*
 *----------------------------------------------------------------------
 *
 * TkMenuInitializeDrawingFields --
 *
 *	Fills in drawing fields of a new menu. Called when new menu is created
 *	by MenuCmd.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	menuPtr fields are initialized.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuInitializeDrawingFields(
    TkMenu *menuPtr)		/* The menu we are initializing. */
{
    menuPtr->textGC = NULL;
    menuPtr->gray = None;
    menuPtr->disabledGC = NULL;
    menuPtr->activeGC = NULL;
    menuPtr->indicatorGC = NULL;
    menuPtr->disabledImageGC = NULL;
    menuPtr->totalWidth = menuPtr->totalHeight = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuInitializeEntryDrawingFields --
 *
 *	Fills in drawing fields of a new menu entry. Called when an entry is
 *	created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuInitializeEntryDrawingFields(
    TkMenuEntry *mePtr)		/* The menu we are initializing. */
{
    mePtr->width = 0;
    mePtr->height = 0;
    mePtr->x = 0;
    mePtr->y = 0;
    mePtr->indicatorSpace = 0;
    mePtr->labelWidth = 0;
    mePtr->textGC = NULL;
    mePtr->activeGC = NULL;
    mePtr->disabledGC = NULL;
    mePtr->indicatorGC = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuFreeDrawOptions --
 *
 *	Frees up any structures allocated for the drawing of a menu. Called
 *	when menu is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Storage is released.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuFreeDrawOptions(
    TkMenu *menuPtr)
{
    if (menuPtr->textGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->textGC);
    }
    if (menuPtr->disabledImageGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->disabledImageGC);
    }
    if (menuPtr->gray != None) {
	Tk_FreeBitmap(menuPtr->display, menuPtr->gray);
    }
    if (menuPtr->disabledGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->disabledGC);
    }
    if (menuPtr->activeGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->activeGC);
    }
    if (menuPtr->indicatorGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->indicatorGC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuEntryFreeDrawOptions --
 *
 *	Frees up drawing structures for a menu entry. Called when menu entry
 *	is freed.
 *
 * RESULTS:
 *	None.
 *
 * Side effects:
 *	Storage is freed.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuEntryFreeDrawOptions(
    TkMenuEntry *mePtr)
{
    if (mePtr->textGC != NULL) {
	Tk_FreeGC(mePtr->menuPtr->display, mePtr->textGC);
    }
    if (mePtr->disabledGC != NULL) {
	Tk_FreeGC(mePtr->menuPtr->display, mePtr->disabledGC);
    }
    if (mePtr->activeGC != NULL) {
	Tk_FreeGC(mePtr->menuPtr->display, mePtr->activeGC);
    }
    if (mePtr->indicatorGC != NULL) {
	Tk_FreeGC(mePtr->menuPtr->display, mePtr->indicatorGC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuConfigureDrawOptions --
 *
 *	Sets the menu's drawing attributes in preparation for drawing the
 *	menu.
 *
 * RESULTS:
 *	None.
 *
 * Side effects:
 *	Storage is allocated.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuConfigureDrawOptions(
    TkMenu *menuPtr)		/* The menu we are configuring. */
{
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_3DBorder border, activeBorder;
    Tk_Font tkfont;
    XColor *fg, *activeFg, *indicatorFg;

    /*
     * A few options need special processing, such as setting the background
     * from a 3-D border, or filling in complicated defaults that couldn't be
     * specified to Tk_ConfigureWidget.
     */

    border = Tk_Get3DBorderFromObj(menuPtr->tkwin, menuPtr->borderPtr);
    Tk_SetBackgroundFromBorder(menuPtr->tkwin, border);

    tkfont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
    gcValues.font = Tk_FontId(tkfont);
    fg = Tk_GetColorFromObj(menuPtr->tkwin, menuPtr->fgPtr);
    gcValues.foreground = fg->pixel;
    gcValues.background = Tk_3DBorderColor(border)->pixel;
    newGC = Tk_GetGC(menuPtr->tkwin, GCForeground|GCBackground|GCFont,
	    &gcValues);
    if (menuPtr->textGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->textGC);
    }
    menuPtr->textGC = newGC;

    gcValues.font = Tk_FontId(tkfont);
    gcValues.background = Tk_3DBorderColor(border)->pixel;
    if (menuPtr->disabledFgPtr != NULL) {
	XColor *disabledFg;

	disabledFg = Tk_GetColorFromObj(menuPtr->tkwin,
		menuPtr->disabledFgPtr);
	gcValues.foreground = disabledFg->pixel;
	mask = GCForeground|GCBackground|GCFont;
    } else {
	gcValues.foreground = gcValues.background;
	mask = GCForeground;
	if (menuPtr->gray == None) {
	    menuPtr->gray = Tk_GetBitmap(menuPtr->interp, menuPtr->tkwin,
		    "gray50");
	}
	if (menuPtr->gray != None) {
	    gcValues.fill_style = FillStippled;
	    gcValues.stipple = menuPtr->gray;
	    mask = GCForeground|GCFillStyle|GCStipple;
	}
    }
    newGC = Tk_GetGC(menuPtr->tkwin, mask, &gcValues);
    if (menuPtr->disabledGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->disabledGC);
    }
    menuPtr->disabledGC = newGC;

    gcValues.foreground = Tk_3DBorderColor(border)->pixel;
    if (menuPtr->gray == None) {
	menuPtr->gray = Tk_GetBitmap(menuPtr->interp, menuPtr->tkwin,
		"gray50");
    }
    if (menuPtr->gray != None) {
	gcValues.fill_style = FillStippled;
	gcValues.stipple = menuPtr->gray;
	newGC = Tk_GetGC(menuPtr->tkwin,
	    GCForeground|GCFillStyle|GCStipple, &gcValues);
    }
    if (menuPtr->disabledImageGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->disabledImageGC);
    }
    menuPtr->disabledImageGC = newGC;

    gcValues.font = Tk_FontId(tkfont);
    activeFg = Tk_GetColorFromObj(menuPtr->tkwin, menuPtr->activeFgPtr);
    gcValues.foreground = activeFg->pixel;
    activeBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    menuPtr->activeBorderPtr);
    gcValues.background = Tk_3DBorderColor(activeBorder)->pixel;
    newGC = Tk_GetGC(menuPtr->tkwin, GCForeground|GCBackground|GCFont,
	    &gcValues);
    if (menuPtr->activeGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->activeGC);
    }
    menuPtr->activeGC = newGC;

    indicatorFg = Tk_GetColorFromObj(menuPtr->tkwin,
	    menuPtr->indicatorFgPtr);
    gcValues.foreground = indicatorFg->pixel;
    gcValues.background = Tk_3DBorderColor(border)->pixel;
    newGC = Tk_GetGC(menuPtr->tkwin, GCForeground|GCBackground|GCFont,
	    &gcValues);
    if (menuPtr->indicatorGC != NULL) {
	Tk_FreeGC(menuPtr->display, menuPtr->indicatorGC);
    }
    menuPtr->indicatorGC = newGC;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuConfigureEntryDrawOptions --
 *
 *	Calculates any entry-specific draw options for the given menu entry.
 *
 * Results:
 *	Returns a standard Tcl error.
 *
 * Side effects:
 *	Storage may be allocated.
 *
 *----------------------------------------------------------------------
 */

int
TkMenuConfigureEntryDrawOptions(
    TkMenuEntry *mePtr,
    int index)
{
    XGCValues gcValues;
    GC newGC, newActiveGC, newDisabledGC, newIndicatorGC;
    unsigned long mask;
    Tk_Font tkfont;
    TkMenu *menuPtr = mePtr->menuPtr;

    tkfont = Tk_GetFontFromObj(menuPtr->tkwin,
	    (mePtr->fontPtr != NULL) ? mePtr->fontPtr : menuPtr->fontPtr);

    if (mePtr->state == ENTRY_ACTIVE) {
	if (index != menuPtr->active) {
	    TkActivateMenuEntry(menuPtr, index);
	}
    } else {
	if (index == menuPtr->active) {
	    TkActivateMenuEntry(menuPtr, -1);
	}
    }

    if ((mePtr->fontPtr != NULL)
	    || (mePtr->borderPtr != NULL)
	    || (mePtr->fgPtr != NULL)
	    || (mePtr->activeBorderPtr != NULL)
	    || (mePtr->activeFgPtr != NULL)
	    || (mePtr->indicatorFgPtr != NULL)) {
	XColor *fg, *indicatorFg, *activeFg;
	Tk_3DBorder border, activeBorder;

	fg = Tk_GetColorFromObj(menuPtr->tkwin, (mePtr->fgPtr != NULL)
		? mePtr->fgPtr : menuPtr->fgPtr);
	gcValues.foreground = fg->pixel;
	border = Tk_Get3DBorderFromObj(menuPtr->tkwin,
		(mePtr->borderPtr != NULL) ? mePtr->borderPtr
		: menuPtr->borderPtr);
	gcValues.background = Tk_3DBorderColor(border)->pixel;

	gcValues.font = Tk_FontId(tkfont);

	/*
	 * Note: disable GraphicsExpose events; we know there won't be
	 * obscured areas when copying from an off-screen pixmap to the screen
	 * and this gets rid of unnecessary events.
	 */

	gcValues.graphics_exposures = False;
	newGC = Tk_GetGC(menuPtr->tkwin,
		GCForeground|GCBackground|GCFont|GCGraphicsExposures,
		&gcValues);

	indicatorFg = Tk_GetColorFromObj(menuPtr->tkwin,
		(mePtr->indicatorFgPtr != NULL) ? mePtr->indicatorFgPtr
		: menuPtr->indicatorFgPtr);
	gcValues.foreground = indicatorFg->pixel;
	newIndicatorGC = Tk_GetGC(menuPtr->tkwin,
		GCForeground|GCBackground|GCGraphicsExposures,
		&gcValues);

	if ((menuPtr->disabledFgPtr != NULL) || (mePtr->image != NULL)) {
	    XColor *disabledFg;

	    disabledFg = Tk_GetColorFromObj(menuPtr->tkwin,
		    menuPtr->disabledFgPtr);
	    gcValues.foreground = disabledFg->pixel;
	    mask = GCForeground|GCBackground|GCFont|GCGraphicsExposures;
	} else {
	    gcValues.foreground = gcValues.background;
	    gcValues.fill_style = FillStippled;
	    gcValues.stipple = menuPtr->gray;
	    mask = GCForeground|GCFillStyle|GCStipple;
	}
	newDisabledGC = Tk_GetGC(menuPtr->tkwin, mask, &gcValues);

	activeFg = Tk_GetColorFromObj(menuPtr->tkwin,
		(mePtr->activeFgPtr != NULL) ? mePtr->activeFgPtr
		: menuPtr->activeFgPtr);
	activeBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
		(mePtr->activeBorderPtr != NULL) ? mePtr->activeBorderPtr
		: menuPtr->activeBorderPtr);

	gcValues.foreground = activeFg->pixel;
	gcValues.background = Tk_3DBorderColor(activeBorder)->pixel;
	newActiveGC = Tk_GetGC(menuPtr->tkwin,
		GCForeground|GCBackground|GCFont|GCGraphicsExposures,
		&gcValues);
    } else {
	newGC = NULL;
	newActiveGC = NULL;
	newDisabledGC = NULL;
	newIndicatorGC = NULL;
    }
    if (mePtr->textGC != NULL) {
	Tk_FreeGC(menuPtr->display, mePtr->textGC);
    }
    mePtr->textGC = newGC;
    if (mePtr->activeGC != NULL) {
	Tk_FreeGC(menuPtr->display, mePtr->activeGC);
    }
    mePtr->activeGC = newActiveGC;
    if (mePtr->disabledGC != NULL) {
	Tk_FreeGC(menuPtr->display, mePtr->disabledGC);
    }
    mePtr->disabledGC = newDisabledGC;
    if (mePtr->indicatorGC != NULL) {
	Tk_FreeGC(menuPtr->display, mePtr->indicatorGC);
    }
    mePtr->indicatorGC = newIndicatorGC;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkEventuallyRecomputeMenu --
 *
 *	Tells Tcl to redo the geometry because this menu has changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Menu geometry is recomputed at idle time, and the menu will be
 *	redisplayed.
 *
 *----------------------------------------------------------------------
 */

void
TkEventuallyRecomputeMenu(
    TkMenu *menuPtr)
{
    if (!(menuPtr->menuFlags & RESIZE_PENDING)) {
	menuPtr->menuFlags |= RESIZE_PENDING;
	Tcl_DoWhenIdle(ComputeMenuGeometry, menuPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkRecomputeMenu --
 *
 *	Tells Tcl to redo the geometry because this menu has changed. Does it
 *	now; removes any ComputeMenuGeometries from the idler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Menu geometry is immediately reconfigured.
 *
 *----------------------------------------------------------------------
 */

void
TkRecomputeMenu(
    TkMenu *menuPtr)
{
    if (menuPtr->menuFlags & RESIZE_PENDING) {
	Tcl_CancelIdleCall(ComputeMenuGeometry, menuPtr);
	ComputeMenuGeometry(menuPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkEventuallyRedrawMenu --
 *
 *	Arrange for an entry of a menu, or the whole menu, to be redisplayed
 *	at some point in the future.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A when-idle handler is scheduled to do the redisplay, if there isn't
 *	one already scheduled.
 *
 *----------------------------------------------------------------------
 */

void
TkEventuallyRedrawMenu(
    register TkMenu *menuPtr,	/* Information about menu to redraw. */
    register TkMenuEntry *mePtr)/* Entry to redraw. NULL means redraw all the
				 * entries in the menu. */
{
    int i;

    if (menuPtr->tkwin == NULL) {
	return;
    }
    if (mePtr != NULL) {
	mePtr->entryFlags |= ENTRY_NEEDS_REDISPLAY;
    } else {
	for (i = 0; i < menuPtr->numEntries; i++) {
	    menuPtr->entries[i]->entryFlags |= ENTRY_NEEDS_REDISPLAY;
	}
    }
    if (!Tk_IsMapped(menuPtr->tkwin)
	    || (menuPtr->menuFlags & REDRAW_PENDING)) {
	return;
    }
    Tcl_DoWhenIdle(DisplayMenu, menuPtr);
    menuPtr->menuFlags |= REDRAW_PENDING;
}

/*
 *--------------------------------------------------------------
 *
 * ComputeMenuGeometry --
 *
 *	This function is invoked to recompute the size and layout of a menu.
 *	It is called as a when-idle handler so that it only gets done once,
 *	even if a group of changes is made to the menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fields of menu entries are changed to reflect their current positions,
 *	and the size of the menu window itself may be changed.
 *
 *--------------------------------------------------------------
 */

static void
ComputeMenuGeometry(
    ClientData clientData)	/* Structure describing menu. */
{
    TkMenu *menuPtr = clientData;

    if (menuPtr->tkwin == NULL) {
	return;
    }

    if (menuPtr->menuType == MENUBAR) {
	TkpComputeMenubarGeometry(menuPtr);
    } else {
	TkpComputeStandardMenuGeometry(menuPtr);
    }

    if ((menuPtr->totalWidth != Tk_ReqWidth(menuPtr->tkwin)) ||
	    (menuPtr->totalHeight != Tk_ReqHeight(menuPtr->tkwin))) {
	Tk_GeometryRequest(menuPtr->tkwin, menuPtr->totalWidth,
		menuPtr->totalHeight);
    }

    /*
     * Must always force a redisplay here if the window is mapped (even if the
     * size didn't change, something else might have changed in the menu, such
     * as a label or accelerator). The resize will force a redisplay above.
     */

    TkEventuallyRedrawMenu(menuPtr, NULL);

    menuPtr->menuFlags &= ~RESIZE_PENDING;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuSelectImageProc --
 *
 *	This function is invoked by the image code whenever the manager for an
 *	image does something that affects the size of contents of an image
 *	displayed in a menu entry when it is selected.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the menu to get redisplayed.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuSelectImageProc(
    ClientData clientData,	/* Pointer to widget record. */
    int x, int y,		/* Upper left pixel (within image) that must
				 * be redisplayed. */
    int width, int height,	/* Dimensions of area to redisplay (may be
				 * <=0). */
    int imgWidth, int imgHeight)/* New dimensions of image. */
{
    register TkMenuEntry *mePtr = clientData;

    if ((mePtr->entryFlags & ENTRY_SELECTED)
	    && !(mePtr->menuPtr->menuFlags & REDRAW_PENDING)) {
	mePtr->menuPtr->menuFlags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayMenu, mePtr->menuPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayMenu --
 *
 *	This function is invoked to display a menu widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayMenu(
    ClientData clientData)	/* Information about widget. */
{
    register TkMenu *menuPtr = clientData;
    register TkMenuEntry *mePtr;
    register Tk_Window tkwin = menuPtr->tkwin;
    int index, strictMotif;
    Tk_Font tkfont;
    Tk_FontMetrics menuMetrics;
    int width;
    int borderWidth;
    Tk_3DBorder border;
    int relief;


    menuPtr->menuFlags &= ~REDRAW_PENDING;
    if ((menuPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
	    &borderWidth);
    border = Tk_Get3DBorderFromObj(menuPtr->tkwin, menuPtr->borderPtr);

    if (menuPtr->menuType == MENUBAR) {
	Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), border, borderWidth,
		borderWidth, Tk_Width(tkwin) - 2 * borderWidth,
		Tk_Height(tkwin) - 2 * borderWidth, 0, TK_RELIEF_FLAT);
    }

    strictMotif = Tk_StrictMotif(menuPtr->tkwin);

    /*
     * See note in ComputeMenuGeometry. We don't want to be doing font metrics
     * all of the time.
     */

    tkfont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
    Tk_GetFontMetrics(tkfont, &menuMetrics);

    /*
     * Loop through all of the entries, drawing them one at a time.
     */

    for (index = 0; index < menuPtr->numEntries; index++) {
	mePtr = menuPtr->entries[index];
	if (menuPtr->menuType != MENUBAR) {
	    if (!(mePtr->entryFlags & ENTRY_NEEDS_REDISPLAY)) {
		continue;
	    }
	}
	mePtr->entryFlags &= ~ENTRY_NEEDS_REDISPLAY;

	TkpDrawMenuEntry(mePtr, Tk_WindowId(menuPtr->tkwin), tkfont,
		&menuMetrics, mePtr->x, mePtr->y, mePtr->width,
		mePtr->height, strictMotif, 1);
	if ((index > 0) && (menuPtr->menuType != MENUBAR)
		&& mePtr->columnBreak) {
	    mePtr = menuPtr->entries[index - 1];
	    Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), border,
		mePtr->x, mePtr->y + mePtr->height, mePtr->width,
		Tk_Height(tkwin) - mePtr->y - mePtr->height - borderWidth,
		0, TK_RELIEF_FLAT);
	}
    }

    if (menuPtr->menuType != MENUBAR) {
	int x, y, height;

	if (menuPtr->numEntries == 0) {
	    x = y = borderWidth;
	    width = Tk_Width(tkwin) - 2 * borderWidth;
	    height = Tk_Height(tkwin) - 2 * borderWidth;
	} else {
	    mePtr = menuPtr->entries[menuPtr->numEntries - 1];
	    Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin),
		border, mePtr->x, mePtr->y + mePtr->height, mePtr->width,
		Tk_Height(tkwin) - mePtr->y - mePtr->height - borderWidth,
		0, TK_RELIEF_FLAT);
	    x = mePtr->x + mePtr->width;
	    y = mePtr->y + mePtr->height;
	    width = Tk_Width(tkwin) - x - borderWidth;
	    height = Tk_Height(tkwin) - y - borderWidth;
	}
	Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), border, x, y,
		width, height, 0, TK_RELIEF_FLAT);
    }

    Tk_GetReliefFromObj(NULL, menuPtr->reliefPtr, &relief);
    Tk_Draw3DRectangle(menuPtr->tkwin, Tk_WindowId(tkwin),
	    border, 0, 0, Tk_Width(tkwin), Tk_Height(tkwin), borderWidth,
	    relief);
}

/*
 *--------------------------------------------------------------
 *
 * TkMenuEventProc --
 *
 *	This function is invoked by the Tk dispatcher for various events on
 *	menus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up. When
 *	it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

void
TkMenuEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkMenu *menuPtr = clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	TkEventuallyRedrawMenu(menuPtr, NULL);
    } else if (eventPtr->type == ConfigureNotify) {
	TkEventuallyRecomputeMenu(menuPtr);
	TkEventuallyRedrawMenu(menuPtr, NULL);
    } else if (eventPtr->type == ActivateNotify) {
	if (menuPtr->menuType == TEAROFF_MENU) {
	    TkpSetMainMenubar(menuPtr->interp, menuPtr->tkwin, NULL);
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (menuPtr->tkwin != NULL) {
	    if (!(menuPtr->menuFlags & MENU_DELETION_PENDING)) {
		TkDestroyMenu(menuPtr);
	    }
	    menuPtr->tkwin = NULL;
	}
	if (menuPtr->menuFlags & MENU_WIN_DESTRUCTION_PENDING) {
	    return;
	}
	menuPtr->menuFlags |= MENU_WIN_DESTRUCTION_PENDING;
	if (menuPtr->widgetCmd != NULL) {
	    Tcl_DeleteCommandFromToken(menuPtr->interp, menuPtr->widgetCmd);
	    menuPtr->widgetCmd = NULL;
	}
	if (menuPtr->menuFlags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayMenu, menuPtr);
	    menuPtr->menuFlags &= ~REDRAW_PENDING;
	}
	if (menuPtr->menuFlags & RESIZE_PENDING) {
	    Tcl_CancelIdleCall(ComputeMenuGeometry, menuPtr);
	    menuPtr->menuFlags &= ~RESIZE_PENDING;
	}
	Tcl_EventuallyFree(menuPtr, TCL_DYNAMIC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuImageProc --
 *
 *	This function is invoked by the image code whenever the manager for an
 *	image does something that affects the size of contents of an image
 *	displayed in a menu entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the menu to get redisplayed.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuImageProc(
    ClientData clientData,	/* Pointer to widget record. */
    int x, int y,		/* Upper left pixel (within image) that must
				 * be redisplayed. */
    int width, int height,	/* Dimensions of area to redisplay (may be
				 * <=0). */
    int imgWidth, int imgHeight)/* New dimensions of image. */
{
    register TkMenu *menuPtr = ((TkMenuEntry *) clientData)->menuPtr;

    if ((menuPtr->tkwin != NULL) && !(menuPtr->menuFlags & RESIZE_PENDING)) {
	menuPtr->menuFlags |= RESIZE_PENDING;
	Tcl_DoWhenIdle(ComputeMenuGeometry, menuPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkPostTearoffMenu --
 *
 *	Posts a tearoff menu on the screen. Adjusts the menu's position so
 *	that it fits on the screen, and maps and raises the menu.
 *
 * Results:
 *	Returns a standard Tcl Error.
 *
 * Side effects:
 *	The menu is posted.
 *
 *----------------------------------------------------------------------
 */

int
TkPostTearoffMenu(
    Tcl_Interp *interp,		/* The interpreter of the menu */
    TkMenu *menuPtr,		/* The menu we are posting */
    int x, int y)		/* The root X,Y coordinates where we are
				 * posting */
{
    return TkpPostTearoffMenu(interp, menuPtr, x, y, -1);
}

/*
 *--------------------------------------------------------------
 *
 * TkPostSubmenu --
 *
 *	This function arranges for a particular submenu (i.e. the menu
 *	corresponding to a given cascade entry) to be posted.
 *
 * Results:
 *	A standard Tcl return result. Errors may occur in the Tcl commands
 *	generated to post and unpost submenus.
 *
 * Side effects:
 *	If there is already a submenu posted, it is unposted. The new submenu
 *	is then posted.
 *
 *--------------------------------------------------------------
 */

int
TkPostSubmenu(
    Tcl_Interp *interp,		/* Used for invoking sub-commands and
				 * reporting errors. */
    register TkMenu *menuPtr,	/* Information about menu as a whole. */
    register TkMenuEntry *mePtr)/* Info about submenu that is to be posted.
				 * NULL means make sure that no submenu is
				 * posted. */
{
    int result, x, y;
    Tcl_Obj *subary[4];

    if (mePtr == menuPtr->postedCascade) {
	return TCL_OK;
    }

    if (menuPtr->postedCascade != NULL) {
	/*
	 * Note: when unposting a submenu, we have to redraw the entire parent
	 * menu. This is because of a combination of the following things:
	 * (a) the submenu partially overlaps the parent.
	 * (b) the submenu specifies "save under", which causes the X server
	 *     to make a copy of the information under it when it is posted.
	 *     When the submenu is unposted, the X server copies this data
	 *     back and doesn't generate any Expose events for the parent.
	 * (c) the parent may have redisplayed itself after the submenu was
	 *     posted, in which case the saved information is no longer
	 *     correct.
	 * The simplest solution is just force a complete redisplay of the
	 * parent.
	 */

	subary[0] = menuPtr->postedCascade->namePtr;
	subary[1] = Tcl_NewStringObj("unpost", -1);
	Tcl_IncrRefCount(subary[1]);
	TkEventuallyRedrawMenu(menuPtr, NULL);
	result = Tcl_EvalObjv(interp, 2, subary, 0);
	Tcl_DecrRefCount(subary[1]);
	menuPtr->postedCascade = NULL;
	if (result != TCL_OK) {
	    return result;
	}
    }

    if ((mePtr != NULL) && (mePtr->namePtr != NULL)
	    && Tk_IsMapped(menuPtr->tkwin)) {
	/*
	 * Position the cascade with its upper left corner slightly below and
	 * to the left of the upper right corner of the menu entry (this is an
	 * attempt to match Motif behavior).
	 *
	 * The menu has to redrawn so that the entry can change relief.
	 *
	 * Set postedCascade early to ensure tear-off submenus work on
	 * Windows. [Bug 873613]
	 */

	Tk_GetRootCoords(menuPtr->tkwin, &x, &y);
	AdjustMenuCoords(menuPtr, mePtr, &x, &y);

	menuPtr->postedCascade = mePtr;
	subary[0] = mePtr->namePtr;
	subary[1] = Tcl_NewStringObj("post", -1);
	subary[2] = Tcl_NewIntObj(x);
	subary[3] = Tcl_NewIntObj(y);
	Tcl_IncrRefCount(subary[1]);
	Tcl_IncrRefCount(subary[2]);
	Tcl_IncrRefCount(subary[3]);
	result = Tcl_EvalObjv(interp, 4, subary, 0);
	Tcl_DecrRefCount(subary[1]);
	Tcl_DecrRefCount(subary[2]);
	Tcl_DecrRefCount(subary[3]);
	if (result != TCL_OK) {
	    menuPtr->postedCascade = NULL;
	    return result;
	}
	TkEventuallyRedrawMenu(menuPtr, mePtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AdjustMenuCoords --
 *
 *	Adjusts the given coordinates down and the left to give a Motif look.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu is eventually redrawn if necessary.
 *
 *----------------------------------------------------------------------
 */

static void
AdjustMenuCoords(
    TkMenu *menuPtr,
    TkMenuEntry *mePtr,
    int *xPtr,
    int *yPtr)
{
    if (menuPtr->menuType == MENUBAR) {
	*xPtr += mePtr->x;
	*yPtr += mePtr->y + mePtr->height;
    } else {
	int borderWidth, activeBorderWidth;

	Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
		&borderWidth);
	Tk_GetPixelsFromObj(NULL, menuPtr->tkwin,
		menuPtr->activeBorderWidthPtr, &activeBorderWidth);
	*xPtr += Tk_Width(menuPtr->tkwin) - borderWidth	- activeBorderWidth
		- 2;
	*yPtr += mePtr->y + activeBorderWidth + 2;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
