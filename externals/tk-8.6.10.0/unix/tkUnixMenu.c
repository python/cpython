/*
 * tkUnixMenu.c --
 *
 *	This module implements the UNIX platform-specific features of menus.
 *
 * Copyright (c) 1996-1998 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkUnixInt.h"
#include "tkMenu.h"

/*
 * Constants used for menu drawing.
 */

#define MENU_MARGIN_WIDTH	2
#define MENU_DIVIDER_HEIGHT	2

/*
 * Platform specific flags for Unix.
 */

#define ENTRY_HELP_MENU		ENTRY_PLATFORM_FLAG1

/*
 * Shared with button widget.
 */

MODULE_SCOPE void	TkpDrawCheckIndicator(Tk_Window tkwin,
			    Display *display, Drawable d, int x, int y,
			    Tk_3DBorder bgBorder, XColor *indicatorColor,
			    XColor *selectColor, XColor *disColor, int on,
			    int disabled, int mode);
/*
 * Indicator Draw Modes
 */

#define CHECK_BUTTON 0
#define CHECK_MENU   1
#define RADIO_BUTTON 2
#define RADIO_MENU   3

/*
 * Procedures used internally.
 */

static void		SetHelpMenu(TkMenu *menuPtr);
static void		DrawMenuEntryAccelerator(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    Tk_Font tkfont, const Tk_FontMetrics *fmPtr,
			    Tk_3DBorder activeBorder, Tk_3DBorder bgBorder,
			    int x, int y, int width, int height, int drawArrow);
static void		DrawMenuEntryBackground(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d,
			    Tk_3DBorder activeBorder, Tk_3DBorder bgBorder,
			    int x, int y, int width, int heigth);
static void		DrawMenuEntryIndicator(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d,
			    Tk_3DBorder border, XColor *indicatorColor,
			    XColor *disableColor, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr, int x, int y,
			    int width, int height);
static void		DrawMenuEntryLabel(TkMenu * menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    Tk_Font tkfont, const Tk_FontMetrics *fmPtr,
			    int x, int y, int width, int height);
static void		DrawMenuSeparator(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    Tk_Font tkfont, const Tk_FontMetrics *fmPtr,
			    int x, int y, int width, int height);
static void		DrawTearoffEntry(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    Tk_Font tkfont, const Tk_FontMetrics *fmPtr,
			    int x, int y, int width, int height);
static void		DrawMenuUnderline(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    Tk_Font tkfont, const Tk_FontMetrics *fmPtr,
			    int x, int y, int width, int height);
static void		GetMenuAccelGeometry(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr, int *widthPtr,
			    int *heightPtr);
static void		GetMenuLabelGeometry(TkMenuEntry *mePtr,
			    Tk_Font tkfont, const Tk_FontMetrics *fmPtr,
			    int *widthPtr, int *heightPtr);
static void		GetMenuIndicatorGeometry(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr,
			    int *widthPtr, int *heightPtr);
static void		GetMenuSeparatorGeometry(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr,
			    int *widthPtr, int *heightPtr);
static void		GetTearoffEntryGeometry(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr, int *widthPtr,
			    int *heightPtr);

/*
 *----------------------------------------------------------------------
 *
 * TkpNewMenu --
 *
 *	Gets the platform-specific piece of the menu. Invoked during idle
 *	after the generic part of the menu has been created.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	Allocates any platform specific allocations and places them in the
 *	platformData field of the menuPtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpNewMenu(
    TkMenu *menuPtr)
{
    SetHelpMenu(menuPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenu --
 *
 *	Destroys platform-specific menu structures. Called when the generic
 *	menu structure is destroyed for the menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All platform-specific allocations are freed up.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyMenu(
    TkMenu *menuPtr)
{
    /*
     * Nothing to do.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenuEntry --
 *
 *	Cleans up platform-specific menu entry items. Called when entry is
 *	destroyed in the generic code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All platform specific allocations are freed up.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyMenuEntry(
    TkMenuEntry *mEntryPtr)
{
    /*
     * Nothing to do.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkpConfigureMenuEntry --
 *
 *	Processes configuration options for menu entries. Called when the
 *	generic options are processed for the menu.
 *
 * Results:
 *	Returns standard TCL result. If TCL_ERROR is returned, then the
 *	interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information get set for mePtr; old resources get freed,
 *	if any need it.
 *
 *----------------------------------------------------------------------
 */

int
TkpConfigureMenuEntry(
    register TkMenuEntry *mePtr)/* Information about menu entry; may or may
				 * not already have values for some fields. */
{
    /*
     * If this is a cascade menu, and the child menu exists, check to see if
     * the child menu is a help menu.
     */

    if ((mePtr->type == CASCADE_ENTRY) && (mePtr->namePtr != NULL)) {
	TkMenuReferences *menuRefPtr;

	menuRefPtr = TkFindMenuReferencesObj(mePtr->menuPtr->interp,
		mePtr->namePtr);
	if ((menuRefPtr != NULL) && (menuRefPtr->menuPtr != NULL)) {
	    SetHelpMenu(menuRefPtr->menuPtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNewEntry --
 *
 *	Called when a new entry is created in a menu. Fills in platform
 *	specific data for the entry. The platformEntryData field is used to
 *	store the indicator diameter for radio button and check box entries.
 *
 * Results:
 * 	Standard TCL error.
 *
 * Side effects:
 *	None on Unix.
 *
 *----------------------------------------------------------------------
 */

int
TkpMenuNewEntry(
    TkMenuEntry *mePtr)
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetWindowMenuBar --
 *
 *	Sets up the menu as a menubar in the given window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Recomputes geometry of given window.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetWindowMenuBar(
    Tk_Window tkwin,		/* The window we are setting */
    TkMenu *menuPtr)		/* The menu we are setting */
{
    if (menuPtr == NULL) {
	TkUnixSetMenubar(tkwin, NULL);
    } else {
	TkUnixSetMenubar(tkwin, menuPtr->tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetMainMenuBar --
 *
 *	Called when a toplevel widget is brought to front. On the Macintosh,
 *	sets up the menubar that goes accross the top of the main monitor. On
 *	other platforms, nothing is necessary.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Recompute geometry of given window.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetMainMenubar(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    const char *menuName)
{
    /*
     * Nothing to do.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuIndicatorGeometry --
 *
 *	Fills out the geometry of the indicator in a menu item. Note that the
 *	mePtr->height field must have already been filled in by
 *	GetMenuLabelGeometry since this height depends on the label height.
 *
 * Results:
 *	widthPtr and heightPtr point to the new geometry values.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuIndicatorGeometry(
    TkMenu *menuPtr,		/* The menu we are drawing. */
    TkMenuEntry *mePtr,		/* The entry we are interested in. */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated metrics */
    int *widthPtr,		/* The resulting width */
    int *heightPtr)		/* The resulting height */
{
    int borderWidth;

    if ((mePtr->type == CHECK_BUTTON_ENTRY)
	    || (mePtr->type == RADIO_BUTTON_ENTRY)) {
	if (!mePtr->hideMargin && mePtr->indicatorOn) {
	    if ((mePtr->image != NULL) || (mePtr->bitmapPtr != NULL)) {
		*widthPtr = (14 * mePtr->height) / 10;
		*heightPtr = mePtr->height;
		if (mePtr->type == CHECK_BUTTON_ENTRY) {
		    mePtr->platformEntryData = (TkMenuPlatformEntryData)
			    INT2PTR((65 * mePtr->height) / 100);
		} else {
		    mePtr->platformEntryData = (TkMenuPlatformEntryData)
			    INT2PTR((75 * mePtr->height) / 100);
		}
	    } else {
		*widthPtr = *heightPtr = mePtr->height;
		if (mePtr->type == CHECK_BUTTON_ENTRY) {
		    mePtr->platformEntryData = (TkMenuPlatformEntryData)
			    INT2PTR((80 * mePtr->height) / 100);
		} else {
		    mePtr->platformEntryData = (TkMenuPlatformEntryData)
			    INT2PTR(mePtr->height);
		}
	    }
	} else {
	    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
		    &borderWidth);
	    *heightPtr = 0;
	    *widthPtr = borderWidth;
	}
    } else {
	Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
		&borderWidth);
	*heightPtr = 0;
	*widthPtr = borderWidth;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuAccelGeometry --
 *
 *	Get the geometry of the accelerator area of a menu item.
 *
 * Results:
 *	heightPtr and widthPtr are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuAccelGeometry(
    TkMenu *menuPtr,		/* The menu was are drawing */
    TkMenuEntry *mePtr,		/* The entry we are getting the geometry for */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int *widthPtr,		/* The width of the acclerator area */
    int *heightPtr)		/* The height of the accelerator area */
{
    *heightPtr = fmPtr->linespace;
    if (mePtr->type == CASCADE_ENTRY) {
    	*widthPtr = 2 * CASCADE_ARROW_WIDTH;
    } else if ((menuPtr->menuType != MENUBAR) && (mePtr->accelPtr != NULL)) {
	const char *accel = Tcl_GetString(mePtr->accelPtr);

	*widthPtr = Tk_TextWidth(tkfont, accel, mePtr->accelLength);
    } else {
    	*widthPtr = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryBackground --
 *
 *	This procedure draws the background part of a menu.
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
DrawMenuEntryBackground(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing. */
    Drawable d,			/* The drawable we are drawing into */
    Tk_3DBorder activeBorder,	/* The border for an active item */
    Tk_3DBorder bgBorder,	/* The background border */
    int x,			/* Left coordinate of entry rect */
    int y,			/* Right coordinate of entry rect */
    int width,			/* Width of entry rect */
    int height)			/* Height of entry rect */
{
    if (mePtr->state == ENTRY_ACTIVE) {
	int relief;
	int activeBorderWidth;

    	bgBorder = activeBorder;

	if ((menuPtr->menuType == MENUBAR)
		&& ((menuPtr->postedCascade == NULL)
		|| (menuPtr->postedCascade != mePtr))) {
	    relief = TK_RELIEF_FLAT;
	} else {
	    relief = TK_RELIEF_RAISED;
	}

	Tk_GetPixelsFromObj(NULL, menuPtr->tkwin,
		menuPtr->activeBorderWidthPtr, &activeBorderWidth);
	Tk_Fill3DRectangle(menuPtr->tkwin, d, bgBorder, x, y, width, height,
		activeBorderWidth, relief);
    } else {
	Tk_Fill3DRectangle(menuPtr->tkwin, d, bgBorder, x, y, width, height,
		0, TK_RELIEF_FLAT);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryAccelerator --
 *
 *	This procedure draws the background part of a menu.
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
DrawMenuEntryAccelerator(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* The drawable we are drawing into */
    GC gc,			/* The precalculated gc to draw with */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated metrics */
    Tk_3DBorder activeBorder,	/* The border for an active item */
    Tk_3DBorder bgBorder,	/* The background border */
    int x,			/* Left coordinate of entry rect */
    int y,			/* Top coordinate of entry rect */
    int width,			/* Width of entry */
    int height,			/* Height of entry */
    int drawArrow)		/* Whether or not to draw arrow. */
{
    XPoint points[3];
    int borderWidth, activeBorderWidth;

    /*
     * Draw accelerator or cascade arrow.
     */

    if (menuPtr->menuType == MENUBAR) {
	return;
    }

    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
	    &borderWidth);
    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->activeBorderWidthPtr,
	    &activeBorderWidth);
    if ((mePtr->type == CASCADE_ENTRY) && drawArrow) {
    	points[0].x = x + width - borderWidth - activeBorderWidth
		- CASCADE_ARROW_WIDTH;
    	points[0].y = y + (height - CASCADE_ARROW_HEIGHT)/2;
    	points[1].x = points[0].x;
    	points[1].y = points[0].y + CASCADE_ARROW_HEIGHT;
    	points[2].x = points[0].x + CASCADE_ARROW_WIDTH;
    	points[2].y = points[0].y + CASCADE_ARROW_HEIGHT/2;
    	Tk_Fill3DPolygon(menuPtr->tkwin, d,
		(mePtr->state == ENTRY_ACTIVE) ? activeBorder : bgBorder,
		points, 3, DECORATION_BORDER_WIDTH,
	    	(menuPtr->postedCascade == mePtr)
	    	? TK_RELIEF_SUNKEN : TK_RELIEF_RAISED);
    } else if (mePtr->accelPtr != NULL) {
	const char *accel = Tcl_GetString(mePtr->accelPtr);
	int left = x + mePtr->labelWidth + activeBorderWidth
		+ mePtr->indicatorSpace;

	if (menuPtr->menuType == MENUBAR) {
	    left += 5;
	}
    	Tk_DrawChars(menuPtr->display, d, gc, tkfont, accel,
		mePtr->accelLength, left,
		(y + (height + fmPtr->ascent - fmPtr->descent) / 2));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryIndicator --
 *
 *	This procedure draws the background part of a menu.
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
DrawMenuEntryIndicator(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* The drawable to draw into */
    Tk_3DBorder border,		/* The background color */
    XColor *indicatorColor,	/* The color to draw indicators with */
    XColor *disableColor,	/* The color use use when disabled */
    Tk_Font tkfont,		/* The font to draw with */
    const Tk_FontMetrics *fmPtr,/* The font metrics of the font */
    int x,			/* The left of the entry rect */
    int y,			/* The top of the entry rect */
    int width,			/* Width of menu entry */
    int height)			/* Height of menu entry */
{
    /*
     * Draw check-button indicator.
     */

    if ((mePtr->type == CHECK_BUTTON_ENTRY) && mePtr->indicatorOn) {
	int top, left, activeBorderWidth;
	int disabled = (mePtr->state == ENTRY_DISABLED);
	XColor *bg;

	Tk_GetPixelsFromObj(NULL, menuPtr->tkwin,
		menuPtr->activeBorderWidthPtr, &activeBorderWidth);
	bg = Tk_3DBorderColor(border);
	top = y + height/2;
	left = x + activeBorderWidth + DECORATION_BORDER_WIDTH
		+ mePtr->indicatorSpace/2;

	TkpDrawCheckIndicator(menuPtr->tkwin, menuPtr->display, d, left, top,
		border, indicatorColor, bg, disableColor,
		(mePtr->entryFlags & ENTRY_SELECTED), disabled, CHECK_MENU);
    }

    /*
     * Draw radio-button indicator.
     */

    if ((mePtr->type == RADIO_BUTTON_ENTRY) && mePtr->indicatorOn) {
	int top, left, activeBorderWidth;
	int disabled = (mePtr->state == ENTRY_DISABLED);
	XColor *bg;

	Tk_GetPixelsFromObj(NULL, menuPtr->tkwin,
		menuPtr->activeBorderWidthPtr, &activeBorderWidth);
	bg = Tk_3DBorderColor(border);
	top = y + height/2;
	left = x + activeBorderWidth + DECORATION_BORDER_WIDTH
		+ mePtr->indicatorSpace/2;

	TkpDrawCheckIndicator(menuPtr->tkwin, menuPtr->display, d, left, top,
		border, indicatorColor, bg, disableColor,
		(mePtr->entryFlags & ENTRY_SELECTED), disabled, RADIO_MENU);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuSeparator --
 *
 *	This procedure draws a separator menu item.
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
DrawMenuSeparator(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* The drawable we are using */
    GC gc,			/* The gc to draw into */
    Tk_Font tkfont,		/* The font to draw with */
    const Tk_FontMetrics *fmPtr,/* The font metrics from the font */
    int x, int y,
    int width, int height)
{
    XPoint points[2];
    Tk_3DBorder border;

    if (menuPtr->menuType == MENUBAR) {
	return;
    }

    points[0].x = x;
    points[0].y = y + height/2;
    points[1].x = x + width - 1;
    points[1].y = points[0].y;
    border = Tk_Get3DBorderFromObj(menuPtr->tkwin, menuPtr->borderPtr);
    Tk_Draw3DPolygon(menuPtr->tkwin, d, border, points, 2, 1,
	    TK_RELIEF_RAISED);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryLabel --
 *
 *	This procedure draws the label part of a menu.
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
DrawMenuEntryLabel(
    TkMenu *menuPtr,		/* The menu we are drawing. */
    TkMenuEntry *mePtr,		/* The entry we are drawing. */
    Drawable d,			/* What we are drawing into. */
    GC gc,			/* The gc we are drawing into.*/
    Tk_Font tkfont,		/* The precalculated font. */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics. */
    int x,			/* Left edge. */
    int y,			/* Top edge. */
    int width,			/* width of entry. */
    int height)			/* height of entry. */
{
    int indicatorSpace = mePtr->indicatorSpace;
    int activeBorderWidth, leftEdge, imageHeight, imageWidth;
    int textHeight = 0, textWidth = 0;	/* stop GCC warning */
    int haveImage = 0, haveText = 0;
    int imageXOffset = 0, imageYOffset = 0;
    int textXOffset = 0, textYOffset = 0;

    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->activeBorderWidthPtr,
	    &activeBorderWidth);
    leftEdge = x + indicatorSpace + activeBorderWidth;
    if (menuPtr->menuType == MENUBAR) {
	leftEdge += 5;
    }

    /*
     * Work out what we will need to draw first.
     */

    if (mePtr->image != NULL) {
    	Tk_SizeOfImage(mePtr->image, &imageWidth, &imageHeight);
	haveImage = 1;
    } else if (mePtr->bitmapPtr != NULL) {
	Pixmap bitmap = Tk_GetBitmapFromObj(menuPtr->tkwin, mePtr->bitmapPtr);

	Tk_SizeOfBitmap(menuPtr->display, bitmap, &imageWidth, &imageHeight);
	haveImage = 1;
    }
    if (!haveImage || (mePtr->compound != COMPOUND_NONE)) {
	if (mePtr->labelLength > 0) {
	    const char *label = Tcl_GetString(mePtr->labelPtr);

	    textWidth = Tk_TextWidth(tkfont, label, mePtr->labelLength);
	    textHeight = fmPtr->linespace;
	    haveText = 1;
	}
    }

    /*
     * Now work out what the relative positions are.
     */

    if (haveImage && haveText) {
	int fullWidth = (imageWidth > textWidth ? imageWidth : textWidth);

	switch ((enum compound) mePtr->compound) {
	case COMPOUND_TOP:
	    textXOffset = (fullWidth - textWidth)/2;
	    textYOffset = imageHeight/2 + 2;
	    imageXOffset = (fullWidth - imageWidth)/2;
	    imageYOffset = -textHeight/2;
	    break;
	case COMPOUND_BOTTOM:
	    textXOffset = (fullWidth - textWidth)/2;
	    textYOffset = -imageHeight/2;
	    imageXOffset = (fullWidth - imageWidth)/2;
	    imageYOffset = textHeight/2 + 2;
	    break;
	case COMPOUND_LEFT:
	    /*
	     * Position image in the indicator space to the left of the
	     * entries, unless this entry is a radio|check button because then
	     * the indicator space will be used.
	     */

	    textXOffset = imageWidth + 2;
	    textYOffset = 0;
	    imageXOffset = 0;
	    imageYOffset = 0;
	    if ((mePtr->type != CHECK_BUTTON_ENTRY)
		    && (mePtr->type != RADIO_BUTTON_ENTRY)) {
		textXOffset -= indicatorSpace;
		if (textXOffset < 0) {
		    textXOffset = 0;
		}
		imageXOffset = -indicatorSpace;
	    }
	    break;
	case COMPOUND_RIGHT:
	    textXOffset = 0;
	    textYOffset = 0;
	    imageXOffset = textWidth + 2;
	    imageYOffset = 0;
	    break;
	case COMPOUND_CENTER:
	    textXOffset = (fullWidth - textWidth)/2;
	    textYOffset = 0;
	    imageXOffset = (fullWidth - imageWidth)/2;
	    imageYOffset = 0;
	    break;
	case COMPOUND_NONE:
	    break;
	}
    } else {
	textXOffset = 0;
	textYOffset = 0;
	imageXOffset = 0;
	imageYOffset = 0;
    }

    /*
     * Draw label and/or bitmap or image for entry.
     */

    if (mePtr->image != NULL) {
    	if ((mePtr->selectImage != NULL)
	    	&& (mePtr->entryFlags & ENTRY_SELECTED)) {
	    Tk_RedrawImage(mePtr->selectImage, 0, 0,
		    imageWidth, imageHeight, d, leftEdge + imageXOffset,
		    (int) (y + (mePtr->height-imageHeight)/2 + imageYOffset));
    	} else {
	    Tk_RedrawImage(mePtr->image, 0, 0, imageWidth,
		    imageHeight, d, leftEdge + imageXOffset,
		    (int) (y + (mePtr->height-imageHeight)/2 + imageYOffset));
    	}
    } else if (mePtr->bitmapPtr != None) {
	Pixmap bitmap = Tk_GetBitmapFromObj(menuPtr->tkwin, mePtr->bitmapPtr);

	XCopyPlane(menuPtr->display, bitmap, d,	gc, 0, 0,
		(unsigned) imageWidth, (unsigned) imageHeight,
		leftEdge + imageXOffset,
		(int) (y + (mePtr->height - imageHeight)/2 + imageYOffset), 1);
    }
    if ((mePtr->compound != COMPOUND_NONE) || !haveImage) {
	int baseline = y + (height + fmPtr->ascent - fmPtr->descent) / 2;

    	if (mePtr->labelLength > 0) {
	    const char *label = Tcl_GetString(mePtr->labelPtr);

	    Tk_DrawChars(menuPtr->display, d, gc, tkfont, label,
		    mePtr->labelLength, leftEdge + textXOffset,
		    baseline + textYOffset);
	    DrawMenuUnderline(menuPtr, mePtr, d, gc, tkfont, fmPtr,
		    x + textXOffset, y + textYOffset,
		    width, height);
    	}
    }

    if (mePtr->state == ENTRY_DISABLED) {
	if (menuPtr->disabledFgPtr == NULL) {
	    XFillRectangle(menuPtr->display, d, menuPtr->disabledGC, x, y,
		    (unsigned) width, (unsigned) height);
	} else if ((mePtr->image != NULL)
		&& (menuPtr->disabledImageGC != None)) {
	    XFillRectangle(menuPtr->display, d, menuPtr->disabledImageGC,
		    leftEdge + imageXOffset,
		    (int) (y + (mePtr->height - imageHeight)/2 + imageYOffset),
		    (unsigned) imageWidth, (unsigned) imageHeight);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuUnderline --
 *
 *	On appropriate platforms, draw the underline character for the menu.
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
DrawMenuUnderline(
    TkMenu *menuPtr,		/* The menu to draw into */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* What we are drawing into */
    GC gc,			/* The gc to draw into */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int x, int y,
    int width, int height)
{
    if ((mePtr->underline >= 0) && (mePtr->labelPtr != NULL)) {
	int len;

	len = Tcl_GetCharLength(mePtr->labelPtr);
	if (mePtr->underline < len) {
	    int activeBorderWidth, leftEdge;
	    const char *label, *start, *end;

	    label = Tcl_GetString(mePtr->labelPtr);
	    start = Tcl_UtfAtIndex(label, mePtr->underline);
	    end = Tcl_UtfNext(start);

	    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin,
		    menuPtr->activeBorderWidthPtr, &activeBorderWidth);
	    leftEdge = x + mePtr->indicatorSpace + activeBorderWidth;
	    if (menuPtr->menuType == MENUBAR) {
		leftEdge += 5;
	    }

	    Tk_UnderlineChars(menuPtr->display, d, gc, tkfont, label, leftEdge,
		    y + (height + fmPtr->ascent - fmPtr->descent) / 2,
		    start - label, end - label);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpPostMenu --
 *
 *	Posts a menu on the screen so that the top left corner of the
 *      specified entry is located at the point (x, y) in screen coordinates.
 *      If the entry parameter is negative, the upper left corner of the
 *      menu itself is placed at the point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu is posted and handled.
 *
 *----------------------------------------------------------------------
 */

int
TkpPostMenu(
    Tcl_Interp *interp,
    TkMenu *menuPtr,
    int x, int y, int index)
{
    return TkpPostTearoffMenu(interp, menuPtr, x, y, index);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpPostTearoffMenu --
 *
 *	Posts a tearoff menu on the screen so that the top left corner of the
 *      specified entry is located at the point (x, y) in screen coordinates.
 *      If the index parameter is negative, the upper left corner of the menu
 *      itself is placed at the point.  On unix this is called when posting
 *      any menu.  Adjusts the menu's position so that it fits on the screen,
 *      and maps and raises the menu.
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
TkpPostTearoffMenu(
    Tcl_Interp *interp,		/* The interpreter of the menu */
    TkMenu *menuPtr,		/* The menu we are posting */
    int x, int y, int index)	/* The root X,Y coordinates where the
				 * specified entry will be posted */
{
    int vRootX, vRootY, vRootWidth, vRootHeight;
    int result;

    if (index >= menuPtr->numEntries) {
	index = menuPtr->numEntries - 1;
    }
    if (index >= 0) {
	y -= menuPtr->entries[index]->y;
    }

    TkActivateMenuEntry(menuPtr, -1);
    TkRecomputeMenu(menuPtr);
    result = TkPostCommand(menuPtr);
    if (result != TCL_OK) {
    	return result;
    }

    /*
     * The post commands could have deleted the menu, which means we are dead
     * and should go away.
     */

    if (menuPtr->tkwin == NULL) {
    	return TCL_OK;
    }

    /*
     * Adjust the position of the menu if necessary to keep it visible on the
     * screen. There are two special tricks to make this work right:
     *
     * 1. If a virtual root window manager is being used then the coordinates
     *    are in the virtual root window of menuPtr's parent; since the menu
     *    uses override-redirect mode it will be in the *real* root window for
     *    the screen, so we have to map the coordinates from the virtual root
     *    (if any) to the real root. Can't get the virtual root from the menu
     *    itself (it will never be seen by the wm) so use its parent instead
     *    (it would be better to have an an option that names a window to use
     *    for this...).
     * 2. The menu may not have been mapped yet, so its current size might be
     *    the default 1x1. To compute how much space it needs, use its
     *    requested size, not its actual size.
     */

    Tk_GetVRootGeometry(Tk_Parent(menuPtr->tkwin), &vRootX, &vRootY,
	&vRootWidth, &vRootHeight);
    vRootWidth -= Tk_ReqWidth(menuPtr->tkwin);
    if (x > vRootX + vRootWidth) {
	x = vRootX + vRootWidth;
    }
    if (x < vRootX) {
	x = vRootX;
    }
    vRootHeight -= Tk_ReqHeight(menuPtr->tkwin);
    if (y > vRootY + vRootHeight) {
	y = vRootY + vRootHeight;
    }
    if (y < vRootY) {
	y = vRootY;
    }
    Tk_MoveToplevelWindow(menuPtr->tkwin, x, y);
    if (!Tk_IsMapped(menuPtr->tkwin)) {
	Tk_MapWindow(menuPtr->tkwin);
    }
    TkWmRestackToplevel((TkWindow *) menuPtr->tkwin, Above, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuSeparatorGeometry --
 *
 *	Gets the width and height of the indicator area of a menu.
 *
 * Results:
 *	widthPtr and heightPtr are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuSeparatorGeometry(
    TkMenu *menuPtr,		/* The menu we are measuring */
    TkMenuEntry *mePtr,		/* The entry we are measuring */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalcualted font metrics */
    int *widthPtr,		/* The resulting width */
    int *heightPtr)		/* The resulting height */
{
    *widthPtr = 0;
    *heightPtr = fmPtr->linespace;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTearoffEntryGeometry --
 *
 *	Gets the width and height of the indicator area of a menu.
 *
 * Results:
 *	widthPtr and heightPtr are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetTearoffEntryGeometry(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are measuring */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int *widthPtr,		/* The resulting width */
    int *heightPtr)		/* The resulting height */
{
    if (menuPtr->menuType != MASTER_MENU) {
	*heightPtr = 0;
	*widthPtr = 0;
    } else {
	*heightPtr = fmPtr->linespace;
	*widthPtr = Tk_TextWidth(tkfont, "W", 1);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpComputeMenubarGeometry --
 *
 *	This procedure is invoked to recompute the size and layout of a menu
 *	that is a menubar clone.
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

void
TkpComputeMenubarGeometry(
    TkMenu *menuPtr)		/* Structure describing menu. */
{
    Tk_Font tkfont, menuFont;
    Tk_FontMetrics menuMetrics, entryMetrics, *fmPtr;
    int width, height, i, j, x, y, currentRowHeight, maxWidth;
    int maxWindowWidth, lastRowBreak, lastEntry;
    int borderWidth, activeBorderWidth, helpMenuIndex = -1;
    TkMenuEntry *mePtr;

    if (menuPtr->tkwin == NULL) {
	return;
    }

    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
	    &borderWidth);
    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->activeBorderWidthPtr,
	    &activeBorderWidth);
    maxWidth = 0;
    if (menuPtr->numEntries == 0) {
	height = 0;
    } else {
	int borderWidth;

	maxWindowWidth = Tk_Width(menuPtr->tkwin);
	if (maxWindowWidth == 1) {
	    maxWindowWidth = 0x7ffffff;
	}
	currentRowHeight = 0;
	Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
		&borderWidth);
	x = y = borderWidth;
	lastRowBreak = 0;

	/*
	 * On the Mac especially, getting font metrics can be quite slow, so
	 * we want to do it intelligently. We are going to precalculate them
	 * and pass them down to all of the measureing and drawing routines.
	 * We will measure the font metrics of the menu once, and if an entry
	 * has a font set, we will measure it as we come to it, and then we
	 * decide which set to give the geometry routines.
	 */

	menuFont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
	Tk_GetFontMetrics(menuFont, &menuMetrics);

	for (i = 0; i < menuPtr->numEntries; i++) {
	    mePtr = menuPtr->entries[i];
	    mePtr->entryFlags &= ~ENTRY_LAST_COLUMN;
	    if (mePtr->fontPtr != NULL) {
		tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
		Tk_GetFontMetrics(tkfont, &entryMetrics);
		fmPtr = &entryMetrics;
	    } else {
		tkfont = menuFont;
		fmPtr = &menuMetrics;
	    }

	    /*
	     * For every entry, we need to check to see whether or not we
	     * wrap. If we do wrap, then we have to adjust all of the previous
	     * entries' height and y position, because when we see them the
	     * first time, we don't know how big its neighbor might be.
	     */

	    if ((mePtr->type == SEPARATOR_ENTRY)
		    || (mePtr->type == TEAROFF_ENTRY)) {
		mePtr->height = mePtr->width = 0;
	    } else {
		GetMenuLabelGeometry(mePtr, tkfont, fmPtr, &width, &height);
		mePtr->height = height + 2 * activeBorderWidth + 10;
		mePtr->width = width;

		GetMenuIndicatorGeometry(menuPtr, mePtr, tkfont, fmPtr,
			&width, &height);
		mePtr->indicatorSpace = width;
		if (width > 0) {
		    mePtr->width += width;
		}
		mePtr->width += 2 * activeBorderWidth + 10;
	    }
	    if (mePtr->entryFlags & ENTRY_HELP_MENU) {
		helpMenuIndex = i;
	    } else if (x + mePtr->width + borderWidth > maxWindowWidth) {
		if (i == lastRowBreak) {
		    mePtr->y = y;
		    mePtr->x = x;
		    lastRowBreak++;
		    y += mePtr->height;
		    currentRowHeight = 0;
		} else {
		    x = borderWidth;
		    for (j = lastRowBreak; j < i; j++) {
			menuPtr->entries[j]->y = y + currentRowHeight
				- menuPtr->entries[j]->height;
			menuPtr->entries[j]->x = x;
			x += menuPtr->entries[j]->width;
		    }
		    lastRowBreak = i;
		    y += currentRowHeight;
		    currentRowHeight = mePtr->height;
		}
		if (x > maxWidth) {
		    maxWidth = x;
		}
		x = borderWidth;
	    } else {
		x += mePtr->width;
		if (mePtr->height > currentRowHeight) {
		    currentRowHeight = mePtr->height;
		}
	    }
	}

	lastEntry = menuPtr->numEntries - 1;
	if (helpMenuIndex == lastEntry) {
	    lastEntry--;
	}
	if ((lastEntry >= 0) && (x + menuPtr->entries[lastEntry]->width
		+ borderWidth > maxWidth)) {
	    maxWidth = x + menuPtr->entries[lastEntry]->width + borderWidth;
	}
	x = borderWidth;
	for (j = lastRowBreak; j < menuPtr->numEntries; j++) {
	    if (j == helpMenuIndex) {
		continue;
	    }
	    menuPtr->entries[j]->y = y + currentRowHeight
		    - menuPtr->entries[j]->height;
	    menuPtr->entries[j]->x = x;
	    x += menuPtr->entries[j]->width;
	}


	if (helpMenuIndex != -1) {
	    mePtr = menuPtr->entries[helpMenuIndex];
	    if (x + mePtr->width + borderWidth > maxWindowWidth) {
		y += currentRowHeight;
		currentRowHeight = mePtr->height;
		x = borderWidth;
	    } else if (mePtr->height > currentRowHeight) {
		currentRowHeight = mePtr->height;
	    }
	    mePtr->x = maxWindowWidth - borderWidth - mePtr->width;
	    mePtr->y = y + currentRowHeight - mePtr->height;
	}
	height = y + currentRowHeight + borderWidth;
    }
    width = Tk_Width(menuPtr->tkwin);

    /*
     * The X server doesn't like zero dimensions, so round up to at least 1 (a
     * zero-sized menu should never really occur, anyway).
     */

    if (width <= 0) {
	width = 1;
    }
    if (height <= 0) {
	height = 1;
    }
    menuPtr->totalWidth = maxWidth;
    menuPtr->totalHeight = height;
}

/*
 *----------------------------------------------------------------------
 *
 * DrawTearoffEntry --
 *
 *	This procedure draws the background part of a menu.
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
DrawTearoffEntry(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* The drawable we are drawing into */
    GC gc,			/* The gc we are drawing with */
    Tk_Font tkfont,		/* The font we are drawing with */
    const Tk_FontMetrics *fmPtr,/* The metrics we are drawing with */
    int x, int y,
    int width, int height)
{
    XPoint points[2];
    int segmentWidth, maxX;
    Tk_3DBorder border;

    if (menuPtr->menuType != MASTER_MENU) {
	return;
    }

    points[0].x = x;
    points[0].y = y + height/2;
    points[1].y = points[0].y;
    segmentWidth = 6;
    maxX = x + width - 1;
    border = Tk_Get3DBorderFromObj(menuPtr->tkwin, menuPtr->borderPtr);

    while (points[0].x < maxX) {
	points[1].x = points[0].x + segmentWidth;
	if (points[1].x > maxX) {
	    points[1].x = maxX;
	}
	Tk_Draw3DPolygon(menuPtr->tkwin, d, border, points, 2, 1,
		TK_RELIEF_RAISED);
	points[0].x += 2 * segmentWidth;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpInitializeMenuBindings --
 *
 *	For every interp, initializes the bindings for Windows menus. Does
 *	nothing on Mac or XWindows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	C-level bindings are setup for the interp which will handle Alt-key
 *	sequences for menus without beeping or interfering with user-defined
 *	Alt-key bindings.
 *
 *--------------------------------------------------------------
 */

void
TkpInitializeMenuBindings(
    Tcl_Interp *interp,		/* The interpreter to set. */
    Tk_BindingTable bindingTable)
				/* The table to add to. */
{
    /*
     * Nothing to do.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * SetHelpMenu --
 *
 *	Given a menu, check to see whether or not it is a help menu cascade in
 *	a menubar. If it is, the entry that points to this menu will be
 *	marked.
 *
 * RESULTS:
 *	None.
 *
 * Side effects:
 *	Will set the ENTRY_HELP_MENU flag appropriately.
 *
 *----------------------------------------------------------------------
 */

static void
SetHelpMenu(
    TkMenu *menuPtr)		/* The menu we are checking */
{
    TkMenuEntry *cascadeEntryPtr;
    int useMotifHelp = 0;
    const char *option = NULL;
    if (menuPtr->tkwin) {
	option = Tk_GetOption(menuPtr->tkwin, "useMotifHelp", "UseMotifHelp");
	if (option != NULL) {
	    Tcl_GetBoolean(NULL, option, &useMotifHelp);
	}
    }

    if (!useMotifHelp) {
	return;
    }

    for (cascadeEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
	    cascadeEntryPtr != NULL;
	    cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr) {
	if ((cascadeEntryPtr->menuPtr->menuType == MENUBAR)
		&& (cascadeEntryPtr->menuPtr->masterMenuPtr->tkwin != NULL)
		&& (menuPtr->masterMenuPtr->tkwin != NULL)) {
	    TkMenu *masterMenuPtr = cascadeEntryPtr->menuPtr->masterMenuPtr;
	    char *helpMenuName = ckalloc(strlen(Tk_PathName(
		    masterMenuPtr->tkwin)) + strlen(".help") + 1);

	    strcpy(helpMenuName, Tk_PathName(masterMenuPtr->tkwin));
	    strcat(helpMenuName, ".help");
	    if (strcmp(helpMenuName,
		    Tk_PathName(menuPtr->masterMenuPtr->tkwin)) == 0) {
		cascadeEntryPtr->entryFlags |= ENTRY_HELP_MENU;
	    } else {
		cascadeEntryPtr->entryFlags &= ~ENTRY_HELP_MENU;
	    }
	    ckfree(helpMenuName);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDrawMenuEntry --
 *
 *	Draws the given menu entry at the given coordinates with the given
 *	attributes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	X Server commands are executed to display the menu entry.
 *
 *----------------------------------------------------------------------
 */

void
TkpDrawMenuEntry(
    TkMenuEntry *mePtr,		/* The entry to draw */
    Drawable d,			/* What to draw into */
    Tk_Font tkfont,		/* Precalculated font for menu */
    const Tk_FontMetrics *menuMetricsPtr,
				/* Precalculated metrics for menu */
    int x,			/* X-coordinate of topleft of entry */
    int y,			/* Y-coordinate of topleft of entry */
    int width,			/* Width of the entry rectangle */
    int height,			/* Height of the current rectangle */
    int strictMotif,		/* Boolean flag */
    int drawArrow)		/* Whether or not to draw the cascade arrow
				 * for cascade items. */
{
    GC gc, indicatorGC;
    XColor *indicatorColor, *disableColor = NULL;
    TkMenu *menuPtr = mePtr->menuPtr;
    Tk_3DBorder bgBorder, activeBorder;
    const Tk_FontMetrics *fmPtr;
    Tk_FontMetrics entryMetrics;
    int padY = (menuPtr->menuType == MENUBAR) ? 3 : 0;
    int adjustedY = y + padY;
    int adjustedHeight = height - 2 * padY;

    /*
     * Choose the gc for drawing the foreground part of the entry.
     */

    if ((mePtr->state == ENTRY_ACTIVE) && !strictMotif) {
	gc = mePtr->activeGC;
	if (gc == NULL) {
	    gc = menuPtr->activeGC;
	}
    } else {
    	TkMenuEntry *cascadeEntryPtr;
    	int parentDisabled = 0;

    	for (cascadeEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
    		cascadeEntryPtr != NULL;
    		cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr) {
	    if (cascadeEntryPtr->namePtr != NULL) {
		const char *name = Tcl_GetString(cascadeEntryPtr->namePtr);

		if (strcmp(name, Tk_PathName(menuPtr->tkwin)) == 0) {
		    if (cascadeEntryPtr->state == ENTRY_DISABLED) {
			parentDisabled = 1;
		    }
		    break;
    	    	}
    	    }
    	}

	if (((parentDisabled || (mePtr->state == ENTRY_DISABLED)))
		&& (menuPtr->disabledFgPtr != NULL)) {
	    gc = mePtr->disabledGC;
	    if (gc == NULL) {
		gc = menuPtr->disabledGC;
	    }
	} else {
	    gc = mePtr->textGC;
	    if (gc == NULL) {
		gc = menuPtr->textGC;
	    }
	}
    }
    indicatorGC = mePtr->indicatorGC;
    if (indicatorGC == NULL) {
	indicatorGC = menuPtr->indicatorGC;
    }
    if (mePtr->indicatorFgPtr) {
	indicatorColor = Tk_GetColorFromObj(menuPtr->tkwin,
		mePtr->indicatorFgPtr);
    } else {
	indicatorColor = Tk_GetColorFromObj(menuPtr->tkwin,
		menuPtr->indicatorFgPtr);
    }

    if (menuPtr->disabledFgPtr != NULL) {
	disableColor = Tk_GetColorFromObj(menuPtr->tkwin,
		menuPtr->disabledFgPtr);
    }

    bgBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    (mePtr->borderPtr == NULL)
	    ? menuPtr->borderPtr : mePtr->borderPtr);
    if (strictMotif) {
	activeBorder = bgBorder;
    } else {
	activeBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    (mePtr->activeBorderPtr == NULL)
	    ? menuPtr->activeBorderPtr : mePtr->activeBorderPtr);
    }

    if (mePtr->fontPtr == NULL) {
	fmPtr = menuMetricsPtr;
    } else {
	tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
	Tk_GetFontMetrics(tkfont, &entryMetrics);
	fmPtr = &entryMetrics;
    }

    /*
     * Need to draw the entire background, including padding. On Unix, for
     * menubars, we have to draw the rest of the entry taking into account the
     * padding.
     */

    DrawMenuEntryBackground(menuPtr, mePtr, d, activeBorder,
	    bgBorder, x, y, width, height);

    if (mePtr->type == SEPARATOR_ENTRY) {
	DrawMenuSeparator(menuPtr, mePtr, d, gc, tkfont,
		fmPtr, x, adjustedY, width, adjustedHeight);
    } else if (mePtr->type == TEAROFF_ENTRY) {
	DrawTearoffEntry(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, adjustedY,
		width, adjustedHeight);
    } else {
	DrawMenuEntryLabel(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, adjustedY,
		width, adjustedHeight);
	DrawMenuEntryAccelerator(menuPtr, mePtr, d, gc, tkfont, fmPtr,
		activeBorder, bgBorder, x, adjustedY, width, adjustedHeight,
		drawArrow);
	if (!mePtr->hideMargin) {
	    if (mePtr->state == ENTRY_ACTIVE) {
		bgBorder = activeBorder;
	    }
	    DrawMenuEntryIndicator(menuPtr, mePtr, d, bgBorder, indicatorColor,
		    disableColor, tkfont, fmPtr, x, adjustedY, width,
		    adjustedHeight);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuLabelGeometry --
 *
 *	Figures out the size of the label portion of a menu item.
 *
 * Results:
 *	widthPtr and heightPtr are filled in with the correct geometry
 *	information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuLabelGeometry(
    TkMenuEntry *mePtr,		/* The entry we are computing */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated metrics */
    int *widthPtr,		/* The resulting width of the label portion */
    int *heightPtr)		/* The resulting height of the label
				 * portion */
{
    TkMenu *menuPtr = mePtr->menuPtr;
    int haveImage = 0;

    if (mePtr->image != NULL) {
    	Tk_SizeOfImage(mePtr->image, widthPtr, heightPtr);
	haveImage = 1;
    } else if (mePtr->bitmapPtr != NULL) {
	Pixmap bitmap = Tk_GetBitmapFromObj(menuPtr->tkwin, mePtr->bitmapPtr);

    	Tk_SizeOfBitmap(menuPtr->display, bitmap, widthPtr, heightPtr);
	haveImage = 1;
    } else {
	*heightPtr = 0;
	*widthPtr = 0;
    }

    if (haveImage && (mePtr->compound == COMPOUND_NONE)) {
	/*
	 * We don't care about the text in this case.
	 */
    } else {
	/*
	 * Either it is compound or we don't have an image.
	 */

    	if (mePtr->labelPtr != NULL) {
	    int textWidth;
	    const char *label = Tcl_GetString(mePtr->labelPtr);

	    textWidth = Tk_TextWidth(tkfont, label, mePtr->labelLength);
	    if ((mePtr->compound != COMPOUND_NONE) && haveImage) {
		switch ((enum compound) mePtr->compound) {
		case COMPOUND_TOP:
		case COMPOUND_BOTTOM:
		    if (textWidth > *widthPtr) {
			*widthPtr = textWidth;
		    }

		    /*
		     * Add text and padding.
		     */

		    *heightPtr += fmPtr->linespace + 2;
		    break;
		case COMPOUND_LEFT:
		case COMPOUND_RIGHT:
		    if (fmPtr->linespace > *heightPtr) {
			*heightPtr = fmPtr->linespace;
		    }

		    /*
		     * Add text and padding.
		     */

		    *widthPtr += textWidth + 2;
		    break;
		case COMPOUND_CENTER:
		    if (fmPtr->linespace > *heightPtr) {
			*heightPtr = fmPtr->linespace;
		    }
		    if (textWidth > *widthPtr) {
			*widthPtr = textWidth;
		    }
		    break;
		case COMPOUND_NONE:
		    break;
		}
	    } else {
		/*
		 * We don't have an image or we're not compound.
		 */

		*heightPtr = fmPtr->linespace;
		*widthPtr = textWidth;
	    }
	} else {
	    /*
	     * An empty entry still has this height.
	     */

	    *heightPtr = fmPtr->linespace;
    	}
    }
    *heightPtr += 1;
}

/*
 *--------------------------------------------------------------
 *
 * TkpComputeStandardMenuGeometry --
 *
 *	This procedure is invoked to recompute the size and layout of a menu
 *	that is not a menubar clone.
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

void
TkpComputeStandardMenuGeometry(
    TkMenu *menuPtr)		/* Structure describing menu. */
{
    Tk_Font tkfont, menuFont;
    Tk_FontMetrics menuMetrics, entryMetrics, *fmPtr;
    int x, y, height, width, indicatorSpace, labelWidth, accelWidth;
    int windowWidth, windowHeight, accelSpace, i, j, lastColumnBreak = 0;
    TkMenuEntry *mePtr;
    int borderWidth, activeBorderWidth;

    if (menuPtr->tkwin == NULL) {
	return;
    }

    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
	    &borderWidth);
    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->activeBorderWidthPtr,
	    &activeBorderWidth);
    x = y = borderWidth;
    indicatorSpace = labelWidth = accelWidth = 0;
    windowHeight = windowWidth = 0;

    /*
     * On the Mac especially, getting font metrics can be quite slow, so we
     * want to do it intelligently. We are going to precalculate them and pass
     * them down to all of the measuring and drawing routines. We will measure
     * the font metrics of the menu once. If an entry does not have its own
     * font set, then we give the geometry/drawing routines the menu's font
     * and metrics. If an entry has its own font, we will measure that font
     * and give all of the geometry/drawing the entry's font and metrics.
     */

    menuFont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
    Tk_GetFontMetrics(menuFont, &menuMetrics);
    accelSpace = Tk_TextWidth(menuFont, "M", 1);

    for (i = 0; i < menuPtr->numEntries; i++) {
	mePtr = menuPtr->entries[i];
	if (mePtr->fontPtr == NULL) {
	    tkfont = menuFont;
	    fmPtr = &menuMetrics;
	} else {
	    tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
	    Tk_GetFontMetrics(tkfont, &entryMetrics);
	    fmPtr = &entryMetrics;
	}

	if ((i > 0) && mePtr->columnBreak) {
	    if (accelWidth != 0) {
		labelWidth += accelSpace;
	    }
	    for (j = lastColumnBreak; j < i; j++) {
		menuPtr->entries[j]->indicatorSpace = indicatorSpace;
		menuPtr->entries[j]->labelWidth = labelWidth;
		menuPtr->entries[j]->width = indicatorSpace + labelWidth
			+ accelWidth + 2 * activeBorderWidth;
		menuPtr->entries[j]->x = x;
		menuPtr->entries[j]->entryFlags &= ~ENTRY_LAST_COLUMN;
	    }
	    x += indicatorSpace + labelWidth + accelWidth
		    + 2 * activeBorderWidth;
	    windowWidth = x;
	    indicatorSpace = labelWidth = accelWidth = 0;
	    lastColumnBreak = i;
	    y = borderWidth;
	}

	if (mePtr->type == SEPARATOR_ENTRY) {
	    GetMenuSeparatorGeometry(menuPtr, mePtr, tkfont, fmPtr,
		    &width, &height);
	    mePtr->height = height;
	} else if (mePtr->type == TEAROFF_ENTRY) {
	    GetTearoffEntryGeometry(menuPtr, mePtr, tkfont, fmPtr,
		    &width, &height);
	    mePtr->height = height;
	    labelWidth = width;
	} else {
	    /*
	     * For each entry, compute the height required by that particular
	     * entry, plus three widths: the width of the label, the width to
	     * allow for an indicator to be displayed to the left of the label
	     * (if any), and the width of the accelerator to be displayed to
	     * the right of the label (if any). These sizes depend, of course,
	     * on the type of the entry.
	     */

	    GetMenuLabelGeometry(mePtr, tkfont, fmPtr, &width, &height);
	    mePtr->height = height;
	    if (!mePtr->hideMargin) {
		width += MENU_MARGIN_WIDTH;
	    }
	    if (width > labelWidth) {
	    	labelWidth = width;
	    }

	    GetMenuAccelGeometry(menuPtr, mePtr, tkfont,
		    fmPtr, &width, &height);
	    if (height > mePtr->height) {
	    	mePtr->height = height;
	    }
	    if (!mePtr->hideMargin) {
		width += MENU_MARGIN_WIDTH;
	    }
	    if (width > accelWidth) {
	    	accelWidth = width;
	    }

	    GetMenuIndicatorGeometry(menuPtr, mePtr, tkfont, fmPtr,
		    &width, &height);
	    if (height > mePtr->height) {
	    	mePtr->height = height;
	    }
	    if (!mePtr->hideMargin) {
		width += MENU_MARGIN_WIDTH;
	    }
	    if (width > indicatorSpace) {
	    	indicatorSpace = width;
	    }

	    mePtr->height += 2 * activeBorderWidth + MENU_DIVIDER_HEIGHT;
    	}
	mePtr->y = y;
	y += mePtr->height;
	if (y > windowHeight) {
	    windowHeight = y;
	}
    }

    if (accelWidth != 0) {
	labelWidth += accelSpace;
    }
    for (j = lastColumnBreak; j < menuPtr->numEntries; j++) {
	menuPtr->entries[j]->indicatorSpace = indicatorSpace;
	menuPtr->entries[j]->labelWidth = labelWidth;
	menuPtr->entries[j]->width = indicatorSpace + labelWidth
		+ accelWidth + 2 * activeBorderWidth;
	menuPtr->entries[j]->x = x;
	menuPtr->entries[j]->entryFlags |= ENTRY_LAST_COLUMN;
    }
    windowWidth = x + indicatorSpace + labelWidth + accelWidth
	    + 2 * activeBorderWidth + borderWidth;
    windowHeight += borderWidth;

    /*
     * The X server doesn't like zero dimensions, so round up to at least 1 (a
     * zero-sized menu should never really occur, anyway).
     */

    if (windowWidth <= 0) {
	windowWidth = 1;
    }
    if (windowHeight <= 0) {
	windowHeight = 1;
    }
    menuPtr->totalWidth = windowWidth;
    menuPtr->totalHeight = windowHeight;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNotifyToplevelCreate --
 *
 *	This routine reconfigures the menu and the clones indicated by
 *	menuName becuase a toplevel has been created and any system menus need
 *	to be created. Not applicable to UNIX.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An idle handler is set up to do the reconfiguration.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuNotifyToplevelCreate(
    Tcl_Interp *interp,		/* The interp the menu lives in. */
    const char *menuName)	/* The name of the menu to reconfigure. */
{
    /*
     * Nothing to do.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuInit --
 *
 *	Does platform-specific initialization of menus.
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
TkpMenuInit(void)
{
    /*
     * Nothing to do.
     */
}


/*
 *----------------------------------------------------------------------
 *
 * TkpMenuThreadInit --
 *
 *	Does platform-specific initialization of thread-specific menu state.
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
TkpMenuThreadInit(void)
{
    /*
     * Nothing to do.
     */
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
