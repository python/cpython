/*
 * tkWinColor.c --
 *
 *	Functions to map color names to system color values.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * Copyright (c) 1994 Software Research Associates, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"
#include "tkColor.h"

/*
 * The following structure is used to keep track of each color that is
 * allocated by this module.
 */

typedef struct WinColor {
    TkColor info;		/* Generic color information. */
    int index;			/* Index for GetSysColor(), -1 if color is not
				 * a "live" system color. */
} WinColor;

/*
 * The sysColors array contains the names and index values for the Windows
 * indirect system color names. In use, all of the names will have the string
 * "System" prepended, but we omit it in the table to save space.
 */

typedef struct {
    const char *name;
    int index;
} SystemColorEntry;

static const SystemColorEntry sysColors[] = {
    {"3dDarkShadow",		COLOR_3DDKSHADOW},
    {"3dLight",			COLOR_3DLIGHT},
    {"ActiveBorder",		COLOR_ACTIVEBORDER},
    {"ActiveCaption",		COLOR_ACTIVECAPTION},
    {"AppWorkspace",		COLOR_APPWORKSPACE},
    {"Background",		COLOR_BACKGROUND},
    {"ButtonFace",		COLOR_BTNFACE},
    {"ButtonHighlight",		COLOR_BTNHIGHLIGHT},
    {"ButtonShadow",		COLOR_BTNSHADOW},
    {"ButtonText",		COLOR_BTNTEXT},
    {"CaptionText",		COLOR_CAPTIONTEXT},
    {"DisabledText",		COLOR_GRAYTEXT},
    {"GrayText",		COLOR_GRAYTEXT},
    {"Highlight",		COLOR_HIGHLIGHT},
    {"HighlightText",		COLOR_HIGHLIGHTTEXT},
    {"InactiveBorder",		COLOR_INACTIVEBORDER},
    {"InactiveCaption",		COLOR_INACTIVECAPTION},
    {"InactiveCaptionText",	COLOR_INACTIVECAPTIONTEXT},
    {"InfoBackground",		COLOR_INFOBK},
    {"InfoText",		COLOR_INFOTEXT},
    {"Menu",			COLOR_MENU},
    {"MenuText",		COLOR_MENUTEXT},
    {"Scrollbar",		COLOR_SCROLLBAR},
    {"Window",			COLOR_WINDOW},
    {"WindowFrame",		COLOR_WINDOWFRAME},
    {"WindowText",		COLOR_WINDOWTEXT}
};

/*
 * Forward declarations for functions defined later in this file.
 */

static int		FindSystemColor(const char *name, XColor *colorPtr,
			    int *indexPtr);

/*
 *----------------------------------------------------------------------
 *
 * FindSystemColor --
 *
 *	This routine finds the color entry that corresponds to the specified
 *	color.
 *
 * Results:
 *	Returns non-zero on success. The RGB values of the XColor will be
 *	initialized to the proper values on success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
FindSystemColor(
    const char *name,		/* Color name. */
    XColor *colorPtr,		/* Where to store results. */
    int *indexPtr)		/* Out parameter to store color index. */
{
    int l, u, r, i;
    int index;

    /*
     * Perform a binary search on the sorted array of colors.
     */

    l = 0;
    u = (sizeof(sysColors) / sizeof(sysColors[0])) - 1;
    while (l <= u) {
	i = (l + u) / 2;
	r = strcasecmp(name, sysColors[i].name);
	if (r == 0) {
	    break;
	} else if (r < 0) {
	    u = i-1;
	} else {
	    l = i+1;
	}
    }
    if (l > u) {
	return 0;
    }

    *indexPtr = index = sysColors[i].index;
    colorPtr->pixel = GetSysColor(index);

    /*
     * x257 is (value<<8 + value) to get the properly bit shifted and padded
     * value. [Bug: 4919]
     */

    colorPtr->red = GetRValue(colorPtr->pixel) * 257;
    colorPtr->green = GetGValue(colorPtr->pixel) * 257;
    colorPtr->blue = GetBValue(colorPtr->pixel) * 257;
    colorPtr->flags = DoRed|DoGreen|DoBlue;
    colorPtr->pad = 0;
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColor --
 *
 *	Allocate a new TkColor for the color with the given name.
 *
 * Results:
 *	Returns a newly allocated TkColor, or NULL on failure.
 *
 * Side effects:
 *	May invalidate the colormap cache associated with tkwin upon
 *	allocating a new colormap entry. Allocates a new TkColor structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColor(
    Tk_Window tkwin,		/* Window in which color will be used. */
    Tk_Uid name)		/* Name of color to allocated (in form
				 * suitable for passing to XParseColor). */
{
    WinColor *winColPtr;
    XColor color;
    int index = -1;		/* -1 indicates that this is not an indirect
				 * system color. */

    /*
     * Check to see if it is a system color or an X color string. If the color
     * is found, allocate a new WinColor and store the XColor and the system
     * color index.
     */

    if (((strncasecmp(name, "system", 6) == 0)
	    && FindSystemColor(name+6, &color, &index))
	    || TkParseColor(Tk_Display(tkwin), Tk_Colormap(tkwin), name,
		    &color)) {
	winColPtr = ckalloc(sizeof(WinColor));
	winColPtr->info.color = color;
	winColPtr->index = index;

	XAllocColor(Tk_Display(tkwin), Tk_Colormap(tkwin),
		&winColPtr->info.color);
 	return (TkColor *) winColPtr;
    }
    return (TkColor *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColorByValue --
 *
 *	Given a desired set of red-green-blue intensities for a color, locate
 *	a pixel value to use to draw that color in a given window.
 *
 * Results:
 *	The return value is a pointer to an TkColor structure that indicates
 *	the closest red, blue, and green intensities available to those
 *	specified in colorPtr, and also specifies a pixel value to use to draw
 *	in that color.
 *
 * Side effects:
 *	May invalidate the colormap cache for the specified window. Allocates
 *	a new TkColor structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColorByValue(
    Tk_Window tkwin,		/* Window in which color will be used. */
    XColor *colorPtr)		/* Red, green, and blue fields indicate
				 * desired color. */
{
    WinColor *tkColPtr = ckalloc(sizeof(WinColor));

    tkColPtr->info.color.red = colorPtr->red;
    tkColPtr->info.color.green = colorPtr->green;
    tkColPtr->info.color.blue = colorPtr->blue;
    tkColPtr->info.color.pixel = 0;
    tkColPtr->index = -1;
    XAllocColor(Tk_Display(tkwin), Tk_Colormap(tkwin), &tkColPtr->info.color);
    return (TkColor *) tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpFreeColor --
 *
 *	Release the specified color back to the system.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Invalidates the colormap cache for the colormap associated with the
 *	given color.
 *
 *----------------------------------------------------------------------
 */

void
TkpFreeColor(
    TkColor *tkColPtr)		/* Color to be released. Must have been
				 * allocated by TkpGetColor or
				 * TkpGetColorByValue. */
{
    Screen *screen = tkColPtr->screen;

    XFreeColors(DisplayOfScreen(screen), tkColPtr->colormap,
	    &tkColPtr->color.pixel, 1, 0L);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinIndexOfColor --
 *
 *	Given a color, return the system color index that was used to create
 *	the color.
 *
 * Results:
 *	If the color was allocated using a system indirect color name, then
 *	the corresponding GetSysColor() index is returned. Otherwise, -1 is
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkWinIndexOfColor(
    XColor *colorPtr)
{
    register WinColor *winColPtr = (WinColor *) colorPtr;
    if (winColPtr->info.magic == COLOR_MAGIC) {
	return winColPtr->index;
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * XAllocColor --
 *
 *	Find the closest available color to the specified XColor.
 *
 * Results:
 *	Updates the color argument and returns 1 on success. Otherwise returns
 *	0.
 *
 * Side effects:
 *	Allocates a new color in the palette.
 *
 *----------------------------------------------------------------------
 */

int
XAllocColor(
    Display *display,
    Colormap colormap,
    XColor *color)
{
    TkWinColormap *cmap = (TkWinColormap *) colormap;
    PALETTEENTRY entry, closeEntry;
    HDC dc = GetDC(NULL);

    entry.peRed = (color->red) >> 8;
    entry.peGreen = (color->green) >> 8;
    entry.peBlue = (color->blue) >> 8;
    entry.peFlags = 0;

    if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) {
	unsigned long sizePalette = GetDeviceCaps(dc, SIZEPALETTE);
	UINT newPixel, closePixel;
	int new;
	size_t refCount;
	Tcl_HashEntry *entryPtr;
	UINT index;

	/*
	 * Find the nearest existing palette entry.
	 */

	newPixel = RGB(entry.peRed, entry.peGreen, entry.peBlue);
	index = GetNearestPaletteIndex(cmap->palette, newPixel);
	GetPaletteEntries(cmap->palette, index, 1, &closeEntry);
	closePixel = RGB(closeEntry.peRed, closeEntry.peGreen,
		closeEntry.peBlue);

	/*
	 * If this is not a duplicate, allocate a new entry. Note that we may
	 * get values for index that are above the current size of the
	 * palette. This happens because we don't shrink the size of the
	 * palette object when we deallocate colors so there may be stale
	 * values that match in the upper slots. We should ignore those values
	 * and just put the new color in as if the colors had not matched.
	 */

	if ((index >= cmap->size) || (newPixel != closePixel)) {
	    if (cmap->size == sizePalette) {
		color->red   = closeEntry.peRed * 257;
		color->green = closeEntry.peGreen * 257;
		color->blue  = closeEntry.peBlue * 257;
		entry = closeEntry;
		if (index >= cmap->size) {
		    OutputDebugStringW(L"XAllocColor: Colormap is bigger than we thought");
		}
	    } else {
		cmap->size++;
		ResizePalette(cmap->palette, cmap->size);
		SetPaletteEntries(cmap->palette, cmap->size - 1, 1, &entry);
	    }
	}

	color->pixel = PALETTERGB(entry.peRed, entry.peGreen, entry.peBlue);
	entryPtr = Tcl_CreateHashEntry(&cmap->refCounts,
		INT2PTR(color->pixel), &new);
	if (new) {
	    refCount = 1;
	} else {
	    refCount = (size_t)Tcl_GetHashValue(entryPtr) + 1;
	}
	Tcl_SetHashValue(entryPtr, (void *)refCount);
    } else {
	/*
	 * Determine what color will actually be used on non-colormap systems.
	 */

	color->pixel = GetNearestColor(dc,
		RGB(entry.peRed, entry.peGreen, entry.peBlue));
	color->red    = GetRValue(color->pixel) * 257;
	color->green  = GetGValue(color->pixel) * 257;
	color->blue   = GetBValue(color->pixel) * 257;
    }

    ReleaseDC(NULL, dc);
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * XFreeColors --
 *
 *	Deallocate a block of colors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes entries for the current palette and compacts the remaining
 *	set.
 *
 *----------------------------------------------------------------------
 */

int
XFreeColors(
    Display *display,
    Colormap colormap,
    unsigned long *pixels,
    int npixels,
    unsigned long planes)
{
    TkWinColormap *cmap = (TkWinColormap *) colormap;
    COLORREF cref;
    UINT count, index;
    size_t refCount;
    int i;
    PALETTEENTRY entry, *entries;
    Tcl_HashEntry *entryPtr;
    HDC dc = GetDC(NULL);

    /*
     * We don't have to do anything for non-palette devices.
     */

    if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) {
	/*
	 * This is really slow for large values of npixels.
	 */

	for (i = 0; i < npixels; i++) {
	    entryPtr = Tcl_FindHashEntry(&cmap->refCounts, INT2PTR(pixels[i]));
	    if (!entryPtr) {
		Tcl_Panic("Tried to free a color that isn't allocated");
	    }
	    refCount = (size_t)Tcl_GetHashValue(entryPtr) - 1;
	    if (refCount == 0) {
		cref = pixels[i] & 0x00ffffff;
		index = GetNearestPaletteIndex(cmap->palette, cref);
		GetPaletteEntries(cmap->palette, index, 1, &entry);
		if (cref == RGB(entry.peRed, entry.peGreen, entry.peBlue)) {
		    count = cmap->size - index;
		    entries = ckalloc(sizeof(PALETTEENTRY) * count);
		    GetPaletteEntries(cmap->palette, index+1, count, entries);
		    SetPaletteEntries(cmap->palette, index, count, entries);
		    ckfree(entries);
		    cmap->size--;
		} else {
		    Tcl_Panic("Tried to free a color that isn't allocated");
		}
		Tcl_DeleteHashEntry(entryPtr);
	    } else {
		Tcl_SetHashValue(entryPtr, (size_t)refCount);
	    }
	}
    }
    ReleaseDC(NULL, dc);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XCreateColormap --
 *
 *	Allocate a new colormap.
 *
 * Results:
 *	Returns a newly allocated colormap.
 *
 * Side effects:
 *	Allocates an empty palette and color list.
 *
 *----------------------------------------------------------------------
 */

Colormap
XCreateColormap(
    Display *display,
    Window w,
    Visual *visual,
    int alloc)
{
    char logPalBuf[sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE *logPalettePtr;
    PALETTEENTRY *entryPtr;
    TkWinColormap *cmap;
    Tcl_HashEntry *hashPtr;
    int new;
    UINT i;
    HPALETTE sysPal;

    /*
     * Allocate a starting palette with all of the reserved colors.
     */

    logPalettePtr = (LOGPALETTE *) logPalBuf;
    logPalettePtr->palVersion = 0x300;
    sysPal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);
    logPalettePtr->palNumEntries = GetPaletteEntries(sysPal, 0, 256,
	    logPalettePtr->palPalEntry);

    cmap = ckalloc(sizeof(TkWinColormap));
    cmap->size = logPalettePtr->palNumEntries;
    cmap->stale = 0;
    cmap->palette = CreatePalette(logPalettePtr);

    /*
     * Add hash entries for each of the static colors.
     */

    Tcl_InitHashTable(&cmap->refCounts, TCL_ONE_WORD_KEYS);
    for (i = 0; i < logPalettePtr->palNumEntries; i++) {
	entryPtr = logPalettePtr->palPalEntry + i;
	hashPtr = Tcl_CreateHashEntry(&cmap->refCounts, INT2PTR(PALETTERGB(
		entryPtr->peRed, entryPtr->peGreen, entryPtr->peBlue)), &new);
	Tcl_SetHashValue(hashPtr, INT2PTR(1));
    }

    return (Colormap)cmap;
}

/*
 *----------------------------------------------------------------------
 *
 * XFreeColormap --
 *
 *	Frees the resources associated with the given colormap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the palette associated with the colormap. Note that the
 *	palette must not be selected into a device context when this occurs.
 *
 *----------------------------------------------------------------------
 */

int
XFreeColormap(
    Display *display,
    Colormap colormap)
{
    TkWinColormap *cmap = (TkWinColormap *) colormap;

    if (!DeleteObject(cmap->palette)) {
	Tcl_Panic("Unable to free colormap, palette is still selected");
    }
    Tcl_DeleteHashTable(&cmap->refCounts);
    ckfree(cmap);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinSelectPalette --
 *
 *	This function sets up the specified device context with a given
 *	palette. If the palette is stale, it realizes it in the background
 *	unless the palette is the current global palette.
 *
 * Results:
 *	Returns the previous palette selected into the device context.
 *
 * Side effects:
 *	May change the system palette.
 *
 *----------------------------------------------------------------------
 */

HPALETTE
TkWinSelectPalette(
    HDC dc,
    Colormap colormap)
{
    TkWinColormap *cmap = (TkWinColormap *) colormap;
    HPALETTE oldPalette;

    oldPalette = SelectPalette(dc, cmap->palette,
	    (cmap->palette == TkWinGetSystemPalette()) ? FALSE : TRUE);
    RealizePalette(dc);
    return oldPalette;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
